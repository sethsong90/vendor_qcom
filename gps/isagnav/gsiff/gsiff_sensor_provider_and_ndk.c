/******************************************************************************
  @file:  gsiff_sensor_provider_and_ndk.c
  @brief: GNSS / Sensor Interface Framework Support

  DESCRIPTION
    This file defines the implementation for GSIFF sensor provider interface
    using the generic Android Sensors API.

  INITIALIZATION AND SEQUENCING REQUIREMENTS

  -----------------------------------------------------------------------------
  Copyright (c) 2011-2012 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------

******************************************************************************/
/*
  * Copyright (c) 2012 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

/*=====================================================================
                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why
--------   ---      -------------------------------------------------------
05/01/11   jb       Initial version
08/01/11   jb       1. Misc typedef changes
                    2. Fixed bug with out-of-order sensor samples if
                       received in batches. This sorts the samples in
                       monotonically increasing order by timestamp before
                       processing.
                    3. Added 1-state KF for AP-SNS clock offset to better
                       estimate time sync responses instead of just using
                       the last sample.
======================================================================*/

#define LOG_TAG "gsiff_sp_and_ndk"

#include "gsiff_sensor_provider_and_ndk.h"
#include "gsiff_sensor_provider_common.h"
#include "log_util.h"

#include "gpsone_glue_msg.h"
#include "gsiff_msg.h"
#include "gpsone_thread_helper.h"
#include "os_kf.h"

#include <android/sensor.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define NUM_SAMPLES_READ 5
#define SENSOR_LOOPER_ID 89

#define UNIT_CONVERT_ACCELERATION 1.0
#define UNIT_CONVERT_GYRO         1.0
#define UNIT_CONVERT_PRESSURE     1.0

/*Unique constant value to identify TYPE_PRESSURE in Sensor envent
  It is consistent with the value in Sensor class of Android framework
  Needs to be defined as no special enum defined in android/sensor.h file
  like in accel or gyro type
*/
#define SENSOR_TYPE_PRESSURE 6

/* The process noise for the TS Offset Kalman filter is defined as a
   ramp function with following parameters
*/
#define US_TO_SEC                     (1000000.0)
#define US_TO_MS                      (1000.0)
#define NDK_MIN_TS_OFFSET_Q_DT_US     (1 * US_TO_SEC)  /* time limit for minimum process noise */

#define NDK_MIN_TS_OFFSET_Q_US2      (250.0)    /* minimum process noise */
#define NDK_TS_OFFSET_Q_SF_US2US     (0.15)   /* process noise rate between time limits (us^2)/us */

#define NDK_TS_OFFSET_MEAS_NOISE_US2 (400.0*400.0) /* measurement noise */
#define NDK_TS_OFFSET_R_SF_US2US     (0.5)

/* Parameters for outlier detection and reset for TS offset KF */
#define NDK_MIN_TS_OFFSET_DIVERGE_THRESHOLD_US (1.25 * US_TO_MS)
#define NDK_TS_OFFSET_N_VAR_THRESH             (25)
#define NDK_NUM_OUTLIER_KF_RESET               (15)
#define NDK_TIME_UPDATE_INTERVAL_US            (1 * US_TO_SEC)
#define NDK_TS_OFFSET_MIN_UNC_TO_INIT_US       (10)

typedef struct android_sensor_state_t
{
   const ASensor*                handle;             /* Opaque handle to Android sensor device */
   bool                          running;            /* Is reporting currently? */
   gsiff_msg                     data_msg;           /* Structure used to batch samples */
   uint32_t                      reporting_rate;     /* Current sampling rate in Hz */
   uint32_t                      batching_rate;      /* Current batching rate in Hz */
} android_sensor_state_t;

typedef struct android_control_t
{
    ALooper*                      looper;
    ASensorManager*               sensor_manager;
    ASensorEventQueue*            sensor_event_queue;
    struct gpsone_thelper         polling_task_helper;
    bool                          polling_task_started;
    void*                         p_msg_q;                  /* Message Queue to add messages to. */
    android_sensor_state_t        accel_state;              /* All necessary state for accel sensor */
    android_sensor_state_t        gyro_state;               /* All necessary state for gyro sensor */
    android_sensor_state_t        baro_state;               /* All necessary state for baro sensor */
    pthread_mutex_t               ref_time_mutex;           /* Make sure both ref times are updated atomically */
    void*                         sns_ap_offset_kf;
} android_control_t;

static android_control_t* g_android_control = NULL;
static void* p_message_queue = NULL;

extern slimBaroDataPayloadStructT* p_baro_data;

/*===========================================================================
FUNCTION    ndk_proc_noise

DESCRIPTION
  Function to estimate the process noise in the SNS to AP clock offset
  for the one state KF used based on the elasped time between updates.

  d_DeltaTimeUs: elapsed time between updates in microseconds.

DEPENDENCIES
   N/A

RETURN VALUE
   Process noise of clock offset in microseconds.

SIDE EFFECTS
   N/A

===========================================================================*/
static double ndk_proc_noise(double d_DeltaTimeUs)
{
   /* Add process noise to input uncertainty. */
   if ( d_DeltaTimeUs <= NDK_MIN_TS_OFFSET_Q_DT_US )
   {
      return NDK_MIN_TS_OFFSET_Q_US2;
   }
   else
   {
      return(NDK_TS_OFFSET_Q_SF_US2US * (d_DeltaTimeUs - NDK_MIN_TS_OFFSET_Q_DT_US)) + NDK_MIN_TS_OFFSET_Q_US2;
   }
}

/*===========================================================================
FUNCTION    ndk_meas_noise

DESCRIPTION
  Function to estimate the measurement noise in the SNS to AP clock offset
  for the one state KF used based on the elasped time between updates.

  d_DeltaTimeUs: elapsed time between updates in microseconds.

DEPENDENCIES
   N/A

RETURN VALUE
   Measurement noise of clock offset in microseconds.

SIDE EFFECTS
   N/A

===========================================================================*/
static double ndk_meas_noise(double d_DeltaTimeUs)
{
   /* Add measurement noise to input uncertainty. */
   if ( d_DeltaTimeUs <= NDK_MIN_TS_OFFSET_Q_DT_US )
   {
      return NDK_TS_OFFSET_MEAS_NOISE_US2;
   }
   else
   {
      return(NDK_TS_OFFSET_R_SF_US2US * (d_DeltaTimeUs - NDK_MIN_TS_OFFSET_Q_DT_US)) + NDK_TS_OFFSET_MEAS_NOISE_US2;
   }
}

/*===========================================================================
FUNCTION    sp_and_ndk_init_defaults

DESCRIPTION
  Initialization function for sensor1 internal state.

DEPENDENCIES
   N/A

RETURN VALUE
   N/A

SIDE EFFECTS
   N/A

===========================================================================*/
static bool sp_and_ndk_init_defaults()
{
   if ( g_android_control != NULL )
   {
      free(g_android_control);
      g_android_control = NULL;
   }

   /* Fill with default values */
   g_android_control = (android_control_t*)calloc(1, sizeof(*g_android_control));
   if ( NULL == g_android_control )
   {
      LOC_LOGE("%s: Error: calloc error", __FUNCTION__);
      return false;
   }

   os_kf_ctor_type init_params;
   init_params.diverge_threshold = NDK_MIN_TS_OFFSET_DIVERGE_THRESHOLD_US;
   init_params.meas_noise_func = ndk_meas_noise;
   init_params.proc_noise_func = ndk_proc_noise;
   init_params.min_unc_to_init = NDK_TS_OFFSET_MIN_UNC_TO_INIT_US;
   init_params.n_var_thresh = NDK_TS_OFFSET_N_VAR_THRESH;
   init_params.outlier_limit = NDK_NUM_OUTLIER_KF_RESET;
   init_params.time_update_interval = NDK_TIME_UPDATE_INTERVAL_US;

   if ( os_kf_ctor(&g_android_control->sns_ap_offset_kf, &init_params) != 0 )
   {
      LOC_LOGE("%s: Unable to initialize KF!", __FUNCTION__);
      return false;
   }

   /* Set with bad file descriptor to start. */
   g_android_control->p_msg_q = NULL;

   pthread_mutex_init(&g_android_control->ref_time_mutex, NULL);

   g_android_control->accel_state.handle = NULL;
   g_android_control->gyro_state.handle = NULL;
   g_android_control->baro_state.handle = NULL;

   /* Mark data as valid so LocApi will accept it - Only needs to happen once*/
   g_android_control->accel_state.data_msg.msg_type = GSIFF_INJECT_SENSOR_DATA_ACCEL_REQ;

   g_android_control->gyro_state.data_msg.msg_type = GSIFF_INJECT_SENSOR_DATA_GYRO_REQ;

   g_android_control->baro_state.data_msg.msg_type =  GSIFF_BAROMETER_REQ;

   return true;
}

/*===========================================================================
FUNCTION    sp_and_ndk_polling_task_init

DESCRIPTION
   Performed when the polling thread is started. This is used for initialization
   that only happens once when the thread starts. This function is called when
   the NDK sensor provider is initialized and only once when the daemon starts.

   context:       Not used

DEPENDENCIES
   N/A

RETURN VALUE
   < 0  : Failure
   0 >= : Success

SIDE EFFECTS
   N/A

===========================================================================*/
static int sp_and_ndk_polling_task_init(void* context)
{
   /*
    * NOTE: Looper and sensor event queue need to be created from the polling threads context.
    * If not, then Android complains you are polling without a looper for this thread!
    */
   LOC_LOGD("%s: Initializing polling task!", __FUNCTION__);

   /* Get a looper in order to retrieve sensor data from the event queue. */
   g_android_control->looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
   if ( g_android_control->looper == NULL )
   {
      LOC_LOGE("%s: Could not get Android Sensor Looper!", __FUNCTION__);
      sp_and_ndk_destroy();
      return -1;
   }

   /* Create an event queue to hold the sensor data that will be polled. */
   g_android_control->sensor_event_queue = ASensorManager_createEventQueue(g_android_control->sensor_manager,
                                                                           g_android_control->looper, SENSOR_LOOPER_ID, NULL, NULL);
   if ( g_android_control->sensor_event_queue == NULL )
   {
      LOC_LOGE("%s: Could not get Android Sensor Event Queue!", __FUNCTION__);
      sp_and_ndk_destroy();
      return -1;
   }

   return 0;
}

/*===========================================================================
FUNCTION    sp_and_ndk_polling_task_deinit

DESCRIPTION
   Performed when the polling thread is stopped. This is used to clean up any
   initialization that is performed for this task. This function is called when
   the NDK sensor provider is destroyed and only once when the daemon closes.

   context:       Not used

DEPENDENCIES
   N/A

RETURN VALUE
   < 0  : Failure
   0 >= : Success

SIDE EFFECTS
   N/A

===========================================================================*/
static int sp_and_ndk_polling_task_deinit(void* context)
{
   /*
    * Clean up sensor event queue since we are done polling for now.
    */
   LOC_LOGD("%s: Destroying polling task!", __FUNCTION__);

   if ( g_android_control->sensor_manager != NULL && g_android_control->sensor_event_queue != NULL )
   {
      if ( ASensorManager_destroyEventQueue( g_android_control->sensor_manager, g_android_control->sensor_event_queue ) != 0)
      {
         LOC_LOGE("%s: Could not destroy Sensor Event Queue!", __FUNCTION__);
         return -1;
      }

      g_android_control->sensor_event_queue = NULL;
   }

   return 0;
}

/*===========================================================================
FUNCTION    ASensorEvent_comparator

DESCRIPTION
   Comparator function to be used by qsort to place sensor samples in order.
   The ordering is based on the timestamp of each sample which should be monotonically
   increasing.

   a: Pointer to an ASensorEvent to sort in order
   b: Pointer to an ASensorEvent to sort in order

DEPENDENCIES
   N/A

RETURN VALUE
   -  : a's timestamp is less than b's timestamp
   0  : a's timestamp is equal to b's timestamp
   +  : a's timestamp is greater than b's timestamp

SIDE EFFECTS
   N/A

===========================================================================*/
static int ASensorEvent_comparator (const void * a, const void * b)
{
   ASensorEvent* as = (ASensorEvent*)a;
   ASensorEvent* bs = (ASensorEvent*)b;

   return (as->timestamp - bs->timestamp);
}

/*===========================================================================
FUNCTION    sp_and_ndk_polling_task

DESCRIPTION
   Task that polls the sensor for sensor data. This thread will read a sample,
   convert it and batch to send to the GSIFF control task to send over Loc API.

   context:       Not used

DEPENDENCIES
   N/A

RETURN VALUE
   < 0  : Failure
   0 >= : Success

SIDE EFFECTS
   N/A

===========================================================================*/
static int sp_and_ndk_polling_task(void* context)
{
    /* Read all pending events. */
    int ident;

    /* Wait indefinitely until sensor data is available. */
    while (( g_android_control->accel_state.running || g_android_control->gyro_state.running
             || g_android_control->baro_state.running)
           && ((ident = ALooper_pollOnce(-1, NULL, NULL, NULL)) >= 0))
    {
        LOC_LOGV("%s: ALooper_pollOnce ident = %d", __FUNCTION__, ident);

        /* Was this a sensor event? */
        if ( ident == SENSOR_LOOPER_ID )
        {
            ASensorEvent event_buffer[NUM_SAMPLES_READ];
            int num_read = ASensorEventQueue_getEvents(g_android_control->sensor_event_queue,
                                                       event_buffer, NUM_SAMPLES_READ);

            LOC_LOGI("%s: Num Samples read = %d!", __FUNCTION__, num_read);

            /* Do we want sensor data once we return from blocking call. */
            if ( num_read > 0 )
            {
                /* Sort the array in order based on timestamps to ensure samples are in order. */
                qsort(event_buffer, num_read, sizeof(ASensorEvent), ASensorEvent_comparator);

                int i;
                for ( i = 0; i < num_read; i++ )
                {
                    ASensorEvent* sensor_data = &event_buffer[i];
                    gsiff_msg* p_gsiff_data_msg = NULL;
                    qmiLoc3AxisSensorSampleListStructT_v02* p_sensor_data_tlv = NULL;
                    uint32_t timestamp_ms = sensor_data->timestamp / 1000000LL;
                    uint64_t timestamp_us = sensor_data->timestamp / 1000LL;
                    int64_t sns_ap_clock_offset;
                    float sensor_samples[3] = {0, 0, 0};
                    double batching_interval = 0;
                    double sample_interval = 0;
                    uint32_t reporting_rate = 0;

                    const char* sensor_str = (sensor_data->type==ASENSOR_TYPE_ACCELEROMETER) ? "ACCEL_DATA" :
                        (sensor_data->type==ASENSOR_TYPE_GYROSCOPE) ? "GYRO_DATA" :
                        (sensor_data->type==ASENSOR_TYPE_MAGNETIC_FIELD) ? "MAG_DATA" :
                        (sensor_data->type==ASENSOR_TYPE_LIGHT) ? "LIGHT_DATA" :
                        (sensor_data->type==ASENSOR_TYPE_PROXIMITY) ? "PROX_DATA" :
                        "invalid";

                    switch ( sensor_data->type )
                    {
                    case ASENSOR_TYPE_ACCELEROMETER:
                    {
                        p_gsiff_data_msg = &g_android_control->accel_state.data_msg;
                        p_sensor_data_tlv = &g_android_control->accel_state.data_msg.msg_data.sensor_data_samples;

                        /* Need to flip all the axes to adhere to coordinate system of ISAGNAV */
                        sensor_samples[0] = (float)(sensor_data->acceleration.x) * UNIT_CONVERT_ACCELERATION;
                        sensor_samples[1] = (float)(sensor_data->acceleration.y) * UNIT_CONVERT_ACCELERATION;
                        sensor_samples[2] = (float)(sensor_data->acceleration.z) * UNIT_CONVERT_ACCELERATION;

                        batching_interval = (1000.0 / g_android_control->accel_state.batching_rate);
                        sample_interval = (1000.0 / g_android_control->accel_state.reporting_rate);
                    }
                    break;

                    case ASENSOR_TYPE_GYROSCOPE:
                    {
                        p_gsiff_data_msg = &g_android_control->gyro_state.data_msg;
                        p_sensor_data_tlv = &g_android_control->gyro_state.data_msg.msg_data.sensor_data_samples;

                        sensor_samples[0] = (float)(sensor_data->vector.x) * UNIT_CONVERT_GYRO;
                        sensor_samples[1] = (float)(sensor_data->vector.y) * UNIT_CONVERT_GYRO;
                        sensor_samples[2] = (float)(sensor_data->vector.z) * UNIT_CONVERT_GYRO;

                        batching_interval = (1000.0 / g_android_control->gyro_state.batching_rate);
                        sample_interval = (1000.0 / g_android_control->gyro_state.reporting_rate);
                    }
                    break;

                    case SENSOR_TYPE_PRESSURE:
                    {
                        sensor_str = "PRESSURE_DATA";
                        sensor_samples[0] = (float)(sensor_data->pressure) * UNIT_CONVERT_PRESSURE;

                        LOC_LOGV("%s: Got baro event with baro value = %f", __FUNCTION__, sensor_samples[0]);
                        batching_interval = (1000.0 / g_android_control->baro_state.batching_rate);
                        sample_interval = (1000.0 / g_android_control->baro_state.reporting_rate);
                        reporting_rate = g_android_control->baro_state.reporting_rate;

                        LOC_LOGV("%s: baro data = %f, batching interval = %f, sample interval = %f, reporting_rate = %d",
                                 __FUNCTION__, sensor_samples[0], batching_interval, sample_interval, reporting_rate);

                        if( p_baro_data != NULL)
                        {
                            /* Atomic access to sensor reference time and system reference time. */
                            pthread_mutex_lock( &g_android_control->ref_time_mutex );

                            uint64_t sys_time_us = sp_read_sys_time_us();
                            /* Take time stamp to adjust for time sync later. */
                            sns_ap_clock_offset = (int64_t)sys_time_us - (int64_t)timestamp_us;

                            os_kf_meas unfilt_meas;
                            unfilt_meas.d_Meas = (double)sns_ap_clock_offset;
                            unfilt_meas.d_MeasUnc = 0;

                            /* KF Initialize if not done */
                            os_kf_init_filter(g_android_control->sns_ap_offset_kf,
                                              &unfilt_meas,
                                              sys_time_us);

                            /* KF Update */
                            os_kf_filter_update(g_android_control->sns_ap_offset_kf,
                                                &unfilt_meas,
                                                sys_time_us);

                            pthread_mutex_unlock( &g_android_control->ref_time_mutex );

                            sp_process_raw_baro_data(sensor_str, sensor_samples[0], timestamp_ms, &p_baro_data);

                            sp_send_baro_data_batch(g_android_control->p_msg_q,
                                                    sensor_str,    NULL,
                                                    &p_baro_data,
                                                    sample_interval,
                                                    batching_interval,
                                                    reporting_rate);
                        }

                    }
                    break;

                    default:
                        LOC_LOGE("%s: Unexpected Android Sensor Type = %d str = %s!", __FUNCTION__, sensor_data->type, sensor_str);
                        break;
                    }

                    /* Common handling for Accel and Gyro Data */
                    if ( p_sensor_data_tlv != NULL )
                    {
                        /* Atomic access to sensor reference time and system reference time. */
                        pthread_mutex_lock( &g_android_control->ref_time_mutex );

                        uint64_t sys_time_us = sp_read_sys_time_us();
                        /* Take time stamp to adjust for time sync later. */
                        sns_ap_clock_offset = (int64_t)sys_time_us - (int64_t)timestamp_us;

                        os_kf_meas unfilt_meas;
                        unfilt_meas.d_Meas = (double)sns_ap_clock_offset;
                        unfilt_meas.d_MeasUnc = 0;

                        /* KF Initialize if not done */
                        os_kf_init_filter(g_android_control->sns_ap_offset_kf,
                                          &unfilt_meas,
                                          sys_time_us);

                        /* KF Update */
                        os_kf_filter_update(g_android_control->sns_ap_offset_kf,
                                            &unfilt_meas,
                                            sys_time_us);

                        pthread_mutex_unlock( &g_android_control->ref_time_mutex );

                        if(sensor_data->type == ASENSOR_TYPE_ACCELEROMETER || sensor_data->type == ASENSOR_TYPE_GYROSCOPE)
                        {
                            sp_process_raw_sensor_data(sensor_str, sensor_samples, timestamp_ms, p_sensor_data_tlv, NULL);

                            sp_send_sensor_data_batch(g_android_control->p_msg_q,
                                                      sensor_str,
                                                      p_gsiff_data_msg,
                                                      p_sensor_data_tlv,
                                                      NULL,
                                                      sample_interval,
                                                      batching_interval);
                        }
                    }
                }

            }
            else
            {
                sleep(10);
                LOC_LOGE("%s: Could not read Android Sensors! Exit...", __FUNCTION__);
                exit(0);
            }
        }
    }

    LOC_LOGV("%s: Polling task loop ended!", __FUNCTION__);
    return 0;
}

/*===========================================================================
FUNCTION    sp_and_ndk_update_sensor_status

DESCRIPTION
  Generic function to start/stop a sensor based on provided sampling rate,
  batching rate, mounted state, and sensor information using Native Android API.

  running:       TRUE - start sensor reporting, FALSE - stop sensor reporting
  sampling_rate: Sampling rate in Hz
  batching_rate: Batch samples at this frequency in Hz
  mounted_state: 0 - Send Unmounted State, 1 - Send Mounted State, 2 - Do Not Send
  sensor_type:   What kind of sensor are we updating

DEPENDENCIES
   N/A

RETURN VALUE
   0  : Failure
   1  : Success

SIDE EFFECTS
   N/A

===========================================================================*/
static bool sp_and_ndk_update_sensor_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, int32_t mounted_state, e_sensor_type sensor_type)
{
   android_sensor_state_t* sensor_state = NULL;
   const char* sensor_str;

   // If this is the first update request, initialize NDK.
   if (g_android_control == NULL)
   {
       if (!running)
       {
           LOC_LOGE("%s: session already stopped!", __FUNCTION__);
           return false;
       }
       else if (!sp_and_ndk_init_after_request())
       {
           LOC_LOGE("%s: Unable to initialize NDK!", __FUNCTION__);
           return false;
       }
   }

   // Define sensor string and state based on the sensor type.
   if ( sensor_type == ACCELEROMETER_SENSOR_TYPE )
   {
      sensor_str = "accel";
      sensor_state = &g_android_control->accel_state;
   }
   else if ( sensor_type == GYROSCOPE_SENSOR_TYPE )
   {
      sensor_str = "gyro";
      sensor_state = &g_android_control->gyro_state;
   }
    else if ( sensor_type == BAROMETER_SENSOR_TYPE )
    {
        sensor_str = "pressure";
        sensor_state = &g_android_control->baro_state;
    }
   else
   {
      LOC_LOGE("%s: Unknown Sensor Type %d", __FUNCTION__, sensor_type);
      return false;
   }

   /* No sensor state to use. */
   if ( sensor_state == NULL )
   {
      LOC_LOGE("%s: No %s sensor state provided to start/stop", __FUNCTION__, sensor_str);
      return false;
   }

   /* No sensor to talk to. */
   if ( sensor_state->handle == NULL )
   {
      LOC_LOGE("%s: No %s sensor to start/stop", __FUNCTION__, sensor_str);
      return false;
   }

   /* No State Change */
   if ( !running && !sensor_state->running )
   {
      return true;
   }
   /* Sensors stay On but no change in sensor sampling rate */
   else if ( (running && sensor_state->running) && (sampling_rate == sensor_state->reporting_rate) )
   {
      return true;
   }

   LOC_LOGI("%s: Updating on %s reporting = %d sampling_rate = %lu", __FUNCTION__, sensor_str, running, sampling_rate);
   sensor_state->reporting_rate = sampling_rate;
   sensor_state->batching_rate = batching_rate;

   /* Clear out any old samples that may be lingering from previous starts/stops. */
   memset(&sensor_state->data_msg.msg_data,
          0,
          sizeof(sensor_state->data_msg.msg_data));

   int err;

   if ( running )
   {
      LOC_LOGD("%s: Enabling Sensor!", __FUNCTION__);
      err = ASensorEventQueue_enableSensor(g_android_control->sensor_event_queue,
                                           sensor_state->handle);
   }
   else
   {
      LOC_LOGD("%s: Disabling Sensor!", __FUNCTION__);
      err = ASensorEventQueue_disableSensor(g_android_control->sensor_event_queue,
                                            sensor_state->handle);
   }

    //send baro response status back to PIP Engine
    if( sensor_type == BAROMETER_SENSOR_TYPE )
    {
        int status = SLIM_BARO_STATUS_RESP_OK;
        if(err != 0)
        {
            status = SLIM_BARO_STATUS_RESP_GENERIC_ERROR;
            gsiff_send_baro_resp(status);
        }
        else
            gsiff_send_baro_resp(status);
    }

   /* Unable to start/stop sensor */
   if ( err != 0 )
   {
      LOC_LOGE("%s: Error starting/stopping %s sensor err = %d", __FUNCTION__, sensor_str, err);
      return false;
   }

   /* Since Accel/Gyro reporting is separate we only want to send XMSI once
      when starting sensor data injection. This will prevent an XMSI message
      if gyro data has already started. */
   if ( !g_android_control->accel_state.running && !g_android_control->gyro_state.running &&
        running)
   {
      sp_send_msi_mounted_status(g_android_control->p_msg_q, mounted_state);
   }

   /* Update running state after performing MSI to make the function generic */
   sensor_state->running = running;

   /* Adjust sampling rate only if we are turning on the sensor */
   if ( running && (sampling_rate != 0) )
   {
      int64_t sensor_rate_us = (int64_t)( (uint64_t)1000000 / (uint64_t)sampling_rate );
      /* Change delay in samples to match sampling rate */
      err = ASensorEventQueue_setEventRate(g_android_control->sensor_event_queue,
                                           sensor_state->handle, sensor_rate_us);

      /* Unable to adjust sampling rate */
      if ( err != 0 )
      {
         LOC_LOGE("%s: Error changing sensor rate = %ld on %s sensor err = %d", __FUNCTION__, sensor_rate_us, sensor_str, err);
         return false;
      }
   }

   if (!g_android_control->accel_state.running && !g_android_control->gyro_state.running && !g_android_control->baro_state.running)
   {
     LOC_LOGV("%s: Finishing Android NDK!", __FUNCTION__);
     sp_and_ndk_destroy();
   }

   return true;
}

/* ----------------------- END INTERNAL FUNCTIONS ---------------------------------------- */

/*===========================================================================

  FUNCTION:   sp_and_ndk_init

===========================================================================*/
bool sp_and_ndk_init(void* p_msg_q)
{
   LOC_LOGI("%s: Initializing message queue for Android NDK", __FUNCTION__);

   p_message_queue = p_msg_q;

   return true;
}

/*===========================================================================

  FUNCTION:   sp_and_ndk_init_after_request

  ===========================================================================*/
bool sp_and_ndk_init_after_request()
{
   LOC_LOGI("%s: Initializing Android Sensor NDK Instance", __FUNCTION__);

   /* Initialize default values */
   if ( sp_and_ndk_init_defaults() == false )
   {
      LOC_LOGE("%s: Unable to initialize with default values!", __FUNCTION__);
      return false;
   }

   if ( p_message_queue == NULL )
   {
      LOC_LOGE("%s: Invalid handle to msg queue passed in: %d!", __FUNCTION__, p_message_queue);
      sp_and_ndk_destroy();
      return false;
   }

   g_android_control->p_msg_q = p_message_queue;

   /* Get the Android Sensor Manager needed to access sensors */
   g_android_control->sensor_manager = ASensorManager_getInstance();
   if ( g_android_control->sensor_manager == NULL )
   {
      LOC_LOGE("%s: Could not get Android Sensor NDK Instance!", __FUNCTION__);
      sp_and_ndk_destroy();
      return false;
   }

   /* Get the available sensors */
   ASensorList sensor_list;
   int sensor_count = ASensorManager_getSensorList(g_android_control->sensor_manager, &sensor_list);
   LOC_LOGD("%s: Found %d sensors.", __FUNCTION__, sensor_count);

   /* Get the Available Android Sensors. Look for Gyro and Accel and baro Handles. */
   int i = 0;
   int32_t min_accel_delay = INT_MAX;
   int32_t min_gyro_delay = INT_MAX;
   int32_t min_baro_delay = INT_MAX;
   for ( i = 0; sensor_list != NULL && i < sensor_count; i++ )
   {
       int sensor_type = ASensor_getType(sensor_list[i]);
       int sensor_min_delay = ASensor_getMinDelay(sensor_list[i]);
       const char* sensor_vendor = ASensor_getVendor(sensor_list[i]);
       const char* sensor_name = ASensor_getName(sensor_list[i]);

       LOC_LOGI("%s: Found Sensor Name: %s Vendor: %s Type: %d Min Delay: %d us handle: 0x%08X",
                __FUNCTION__,
                sensor_name,
                sensor_vendor,
                sensor_type,
                sensor_min_delay,
                sensor_list[i]);

       switch ( sensor_type )
       {
           /* Pick sensors with best speed */
       case ASENSOR_TYPE_ACCELEROMETER:
           if ( sensor_min_delay > 0 && sensor_min_delay < min_accel_delay )
           {
               g_android_control->accel_state.handle = sensor_list[i];
               min_accel_delay = sensor_min_delay;
           }
           break;
       case ASENSOR_TYPE_GYROSCOPE:
           if ( sensor_min_delay > 0 && sensor_min_delay < min_gyro_delay )
           {
               g_android_control->gyro_state.handle = sensor_list[i];
               min_gyro_delay = sensor_min_delay;
           }
           break;
       case SENSOR_TYPE_PRESSURE:
           if ( sensor_min_delay > 0 && sensor_min_delay < min_baro_delay )
           {
               g_android_control->baro_state.handle = sensor_list[i];
               min_baro_delay = sensor_min_delay;
           }
           break;
       default:
           break;
       }
   }

   /* Neither Accel or Gyro was found for use. */
   if ( g_android_control->accel_state.handle == NULL && g_android_control->gyro_state.handle == NULL && g_android_control->baro_state.handle == NULL)
   {
      LOC_LOGE("%s: No Accel nor Gyro nor baro to use for sensor data!", __FUNCTION__);
      sp_and_ndk_destroy();
      return false;
   }

   LOC_LOGI("%s: Starting sensor polling thread", __FUNCTION__);
   int rv = gpsone_launch_thelper(&g_android_control->polling_task_helper,
                                  sp_and_ndk_polling_task_init,   /* Initialize func */
                                  NULL,                           /* Pre-Process func */
                                  sp_and_ndk_polling_task,        /* Process func */
                                  sp_and_ndk_polling_task_deinit, /* Post-Process func */
                                  NULL);
   if ( rv != 0 )
   {
      LOC_LOGE("%s: Could not properly start the sensor polling thread!", __FUNCTION__);
      sp_and_ndk_destroy();
      return false;
   }

   g_android_control->polling_task_started = true;

   return true;
}

/*===========================================================================

  FUNCTION:   sp_and_ndk_update_accel_status

  ===========================================================================*/
bool sp_and_ndk_update_accel_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state)
{
   return sp_and_ndk_update_sensor_status(running, sampling_rate, batching_rate, mounted_state,ACCELEROMETER_SENSOR_TYPE);
}

/*===========================================================================

  FUNCTION:   sp_and_ndk_update_gyro_status

  ===========================================================================*/
bool sp_and_ndk_update_gyro_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state)
{
   return sp_and_ndk_update_sensor_status(running, sampling_rate, batching_rate, mounted_state,GYROSCOPE_SENSOR_TYPE);
}

/*===========================================================================

  FUNCTION:   sp_and_ndk_update_baro_status

  ===========================================================================*/
bool sp_and_ndk_update_baro_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state)
{
   return sp_and_ndk_update_sensor_status(running, sampling_rate, batching_rate, mounted_state, BAROMETER_SENSOR_TYPE);
}

/*===========================================================================

  FUNCTION:   sp_and_ndk_destroy

  ===========================================================================*/
bool sp_and_ndk_destroy()
{
   if ( g_android_control != NULL )
   {
      /* Turn off sensor data reporting if it is still on */
      if ( g_android_control->gyro_state.running )
      {
         if ( sp_and_ndk_update_gyro_status(false, 0, 0, 0) != true )
         {
            LOC_LOGE("%s: sp_and_update_gyro_status() could not stop gyro_data", __FUNCTION__ );
            return false;
         }
      }

      if ( g_android_control->accel_state.running )
      {
         if ( sp_and_ndk_update_accel_status(false, 0, 0, 0) != true )
         {
            LOC_LOGE("%s: sp_and_update_accel_status() could not stop accel_data", __FUNCTION__);
            return false;
         }
      }

      if ( g_android_control->gyro_state.running )
      {
         if ( sp_and_ndk_update_baro_status(false, 0, 0, 0) != true )
         {
            LOC_LOGE("%s: sp_and_update_baro_status() could not stop baro_data", __FUNCTION__ );
            return false;
         }
      }

      if ( g_android_control->polling_task_started == true )
      {
         LOC_LOGI("%s: Stopping sensor polling thread", __FUNCTION__);

         gpsone_unblock_thelper(&g_android_control->polling_task_helper);

         /* Wake up the polling thread if we are waiting on sensor data. */
         ALooper_wake(g_android_control->looper);

         gpsone_join_thelper(&g_android_control->polling_task_helper);

         LOC_LOGI("%s: Finished joining sensor polling thread.", __FUNCTION__);
      }

      if ( g_android_control->sensor_manager != NULL && g_android_control->sensor_event_queue != NULL )
      {
         if ( ASensorManager_destroyEventQueue( g_android_control->sensor_manager, g_android_control->sensor_event_queue ) != 0)
         {
            LOC_LOGE("%s: Could not destroy Sensor Event Queue!", __FUNCTION__);
            return false;
         }

         g_android_control->sensor_event_queue = NULL;
      }

      /* Remove reference time mutex */
      int mutex_rc = pthread_mutex_destroy(&g_android_control->ref_time_mutex);
      if ( mutex_rc != 0 )
      {
         LOC_LOGE("%s: Could not destroy ref_time mutex rc = %d errno = %d!", __FUNCTION__, mutex_rc, errno);
         return false;
      }

      if ( os_kf_destroy(&g_android_control->sns_ap_offset_kf) != 0 )
      {
         LOC_LOGE("%s: Unable to destroy KF!", __FUNCTION__);
         return false;
      }

      /* Deallocate memory */
      free(g_android_control);
      g_android_control = NULL;

      return true;
   }
   else
   {
      LOC_LOGE("%s: Initialization required to perform operation", __FUNCTION__);
      return false;
   }
}

/*===========================================================================

  FUNCTION:   sp_and_ndk_get_sensor_time

  ===========================================================================*/
bool sp_and_ndk_get_sensor_time(uint32_t* p_time_ms)
{
   if ( g_android_control == NULL )
   {
      LOC_LOGE("%s: Initialization required to perform operation", __FUNCTION__);
      return false;
   }

   if ( p_time_ms == NULL )
   {
      LOC_LOGE("%s: Invalid pointer passed for reading sensor time.", __FUNCTION__);
      return false;
   }

   /* Atomic access to sensor reference time and system reference time. */
   pthread_mutex_lock( &g_android_control->ref_time_mutex );

   /* Take local time stamp to approximate current sensor time. */
   uint64_t current_sys_time_us = sp_read_sys_time_us();

   double sns_ap_offset = 0;
   os_kf_get_filt_meas(g_android_control->sns_ap_offset_kf, &sns_ap_offset);

   pthread_mutex_unlock( &g_android_control->ref_time_mutex );

   if ( current_sys_time_us - sns_ap_offset < 0 )
   {
      LOC_LOGE("%s: Invalid Sensor-to-AP offset.", __FUNCTION__);
      return false;
   }
   uint64_t current_sensor_time_us = current_sys_time_us - sns_ap_offset;
   uint32_t current_sensor_time_ms = current_sensor_time_us / 1000LL;

   LOC_LOGI("%s: Grabbing Sensor time: cur_sensor_time = %lu sns_ap_offset = %f cur_sys = %lu",
             __FUNCTION__,
             current_sensor_time_ms,
             sns_ap_offset,
             current_sys_time_us / 1000);

   *p_time_ms = current_sensor_time_ms;

   return true;
}


