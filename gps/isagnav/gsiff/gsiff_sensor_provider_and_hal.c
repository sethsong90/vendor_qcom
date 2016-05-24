/******************************************************************************
  @file:  gsiff_sensor_provider_and_hal.c
  @brief: GNSS / Sensor Interface Framework Support

  DESCRIPTION
    This file defines the implementation for GSIFF sensor provider interface
    using the generic Android Sensors API.

  INITIALIZATION AND SEQUENCING REQUIREMENTS

  -----------------------------------------------------------------------------
  Copyright (c) 2011 Qualcomm Technologies, Inc.
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
                    2. Support out-of-order sensor data when received in
                       a batch by sorting by timestamp.

======================================================================*/

#define LOG_TAG "gsiff_sp_and_hal"

#include "gsiff_sensor_provider_and_hal.h"
#include "gsiff_sensor_provider_common.h"
#include "log_util.h"

#include "gpsone_glue_msg.h"
#include "gsiff_msg.h"
#include "gpsone_thread_helper.h"

#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define NUM_SAMPLES_READ 5

#define UNIT_CONVERT_ACCELERATION 1.0
#define UNIT_CONVERT_GYRO         1.0

typedef struct android_sensor_state_t {
   int                           handle;             /* Opaque handle to Android sensor device */
   bool                          running;            /* Is reporting currently? */
   gsiff_msg                     data_msg;           /* Structure used to batch samples */
   uint32_t                      reporting_rate;     /* Current sampling rate in Hz */
   uint32_t                      batching_rate;      /* Current batching rate in Hz */
} android_sensor_state_t;

typedef struct android_control_t {
   struct sensors_module_t*      sensor_module;
   struct sensors_poll_device_t* sensor_device;
   struct gpsone_thelper         polling_task_helper;
   void*                         p_msg_q;                  /* Message Queue to add messages to. */
   android_sensor_state_t        accel_state;              /* All necessary state for accel sensor */
   android_sensor_state_t        gyro_state;               /* All necessary state for gyro sensor */
   pthread_mutex_t               ref_time_mutex;           /* Make sure both ref times are updated atomically */
   uint32_t                      sys_ref_time_ms;
   uint32_t                      sensor_ref_time_ms;
} android_control_t;

static android_control_t* g_android_control = NULL;

/*===========================================================================
FUNCTION    sp_and_hal_init_defaults

DESCRIPTION
  Initialization function for sensor1 internal state.

DEPENDENCIES
   N/A

RETURN VALUE
   N/A

SIDE EFFECTS
   N/A

===========================================================================*/
static bool sp_and_hal_init_defaults()
{
   if( g_android_control != NULL )
   {
      free(g_android_control);
      g_android_control = NULL;
   }

   /* Fill with default values */
   g_android_control = (android_control_t*)calloc(1, sizeof(*g_android_control));
   if( NULL == g_android_control )
   {
      LOC_LOGE("%s: Error: calloc error", __FUNCTION__);
      return false;
   }

   /* Set with bad file descriptor to start. */
   g_android_control->p_msg_q = NULL;

   pthread_mutex_init(&g_android_control->ref_time_mutex, NULL);

   /* Both handles have to be in range of 0-256 based on Android API */
   g_android_control->accel_state.handle = -1;
   g_android_control->gyro_state.handle = -1;

   /* Mark data as valid so LocApi will accept it - Only needs to happen once*/
   g_android_control->accel_state.data_msg.msg_type = GSIFF_INJECT_SENSOR_DATA_ACCEL_REQ;

   g_android_control->gyro_state.data_msg.msg_type = GSIFF_INJECT_SENSOR_DATA_GYRO_REQ;

   return true;
}

/*===========================================================================
FUNCTION    sensors_event_t_comparator

DESCRIPTION
   Comparator function to be used by qsort to place sensor samples in order.
   The ordering is based on the timestamp of each sample which should be monotonically
   increasing.

   a: Pointer to an sensors_event_t to sort in order
   b: Pointer to an sensors_event_t to sort in order

DEPENDENCIES
   N/A

RETURN VALUE
   -  : a's timestamp is less than b's timestamp
   0  : a's timestamp is equal to b's timestamp
   +  : a's timestamp is greater than b's timestamp

SIDE EFFECTS
   N/A

===========================================================================*/
static int sensors_event_t_comparator (const void * a, const void * b)
{
   sensors_event_t* as = (sensors_event_t*)a;
   sensors_event_t* bs = (sensors_event_t*)b;

   return (as->timestamp - bs->timestamp);
}

/*===========================================================================
FUNCTION    sp_and_hal_polling_task

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
static int sp_and_hal_polling_task(void* context)
{
   /* Do we want sensor data */
   if( g_android_control->accel_state.running || g_android_control->gyro_state.running )
   {
      sensors_event_t sensor_data_arr[NUM_SAMPLES_READ];
      int num_read = g_android_control->sensor_device->poll(g_android_control->sensor_device,
                                                            sensor_data_arr,
                                                            NUM_SAMPLES_READ);

      /* Do we want sensor data once we receive from blocking call. */
      if( num_read > 0 && ( g_android_control->accel_state.running || g_android_control->gyro_state.running ) )
      {
         /* Sort the array in order based on timestamps to ensure samples are in order. */
         qsort(sensor_data_arr, num_read, sizeof(sensors_event_t), sensors_event_t_comparator);

         int i = 0;
         for( i = 0; i < num_read; i++ )
         {
            sensors_event_t* sensor_data = &sensor_data_arr[i];
            gsiff_msg* p_gsiff_data_msg = NULL;
            qmiLoc3AxisSensorSampleListStructT_v02* p_sensor_data_tlv = NULL;
            uint32_t timestamp_ms = sensor_data->timestamp / 1000000;
            float sensor_samples[3] = {0, 0, 0};
            double batching_interval = 0;
            double sample_interval = 0;

            const char* sensor_str = (sensor_data->type==SENSOR_TYPE_ACCELEROMETER) ? "ACCEL_DATA" :
               (sensor_data->type==SENSOR_TYPE_GYROSCOPE) ? "GYRO_DATA" :
               (sensor_data->type==SENSOR_TYPE_MAGNETIC_FIELD) ? "MAG_DATA" :
               (sensor_data->type==SENSOR_TYPE_LIGHT) ? "LIGHT_DATA" :
               (sensor_data->type==SENSOR_TYPE_ORIENTATION) ? "ORIENT_DATA" :
               (sensor_data->type==SENSOR_TYPE_TEMPERATURE) ? "TEMP_DATA" :
               (sensor_data->type==SENSOR_TYPE_PROXIMITY) ? "PROX_DATA" :
               (sensor_data->type==SENSOR_TYPE_LINEAR_ACCELERATION) ? "LIN_ACCEL_DATA" :
               (sensor_data->type==SENSOR_TYPE_ROTATION_VECTOR) ? "ROT_VEC_DATA" :
               (sensor_data->type==SENSOR_TYPE_GRAVITY) ? "GRAV_DATA" :
               (sensor_data->type==SENSOR_TYPE_PRESSURE) ? "PRESSURE_DATA" :
               "invalid";

            switch( sensor_data->type )
            {
               case SENSOR_TYPE_ACCELEROMETER:
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

               case SENSOR_TYPE_GYROSCOPE:
                  {
                     p_gsiff_data_msg = &g_android_control->gyro_state.data_msg;
                     p_sensor_data_tlv = &g_android_control->gyro_state.data_msg.msg_data.sensor_data_samples;

                     sensor_samples[0] = (float)(sensor_data->gyro.x) * UNIT_CONVERT_GYRO;
                     sensor_samples[1] = (float)(sensor_data->gyro.y) * UNIT_CONVERT_GYRO;
                     sensor_samples[2] = (float)(sensor_data->gyro.z) * UNIT_CONVERT_GYRO;

                     batching_interval = (1000.0 / g_android_control->gyro_state.batching_rate);
                     sample_interval = (1000.0 / g_android_control->gyro_state.reporting_rate);
                  }
                  break;

               default:
                  LOC_LOGE("%s: Unexpected Android Sensor Type = %d str = %s!", __FUNCTION__, sensor_data->type, sensor_str);
                  break;
            }

            /* Common handling for Accel and Gyro Data */
            if( p_sensor_data_tlv != NULL )
            {
               /* Atomic access to sensor reference time and system reference time. */
               pthread_mutex_lock( &g_android_control->ref_time_mutex );
               /* Take time stamp to adjust for time sync later. */
               g_android_control->sys_ref_time_ms = sp_read_sys_time_ms();
               g_android_control->sensor_ref_time_ms = timestamp_ms;
               pthread_mutex_unlock( &g_android_control->ref_time_mutex );

               sp_process_raw_sensor_data(sensor_str, sensor_samples, timestamp_ms, p_sensor_data_tlv,NULL);

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
      else
      {
         LOC_LOGE("%s: Could not read Android Sensors err = %d!", __FUNCTION__, num_read);
      }
   }

   return 0;
}

/*===========================================================================
FUNCTION    sp_and_hal_update_polling_thread

DESCRIPTION
  Generic function to start/stop the sensor polling thread based on sensor
  type and whether it is starting or stoping.

  sensor_type:   What kind of sensor are we updating
  running:       TRUE - start sensor reporting, FALSE - stop sensor reporting

DEPENDENCIES
   N/A

RETURN VALUE
   0  : Failure
   1  : Success

SIDE EFFECTS
   N/A

===========================================================================*/
static bool sp_and_hal_update_polling_thread(e_sensor_type sensor_type, bool running)
{
   /* Start up polling thread to read the sensor data. */
   if( ( !g_android_control->gyro_state.running && running && sensor_type == ACCELEROMETER_SENSOR_TYPE )
       || ( !g_android_control->accel_state.running && running && sensor_type == GYROSCOPE_SENSOR_TYPE ) )
   {
      LOC_LOGI("%s: Starting sensor polling thread", __FUNCTION__);

      gpsone_launch_thelper(&g_android_control->polling_task_helper,
                            NULL,                    /* Initialize func */
                            NULL,                    /* Pre-Process func */
                            sp_and_hal_polling_task, /* Process func */
                            NULL,                    /* Post-Process func */
                            NULL);
   }
   /* Stop polling thread from reading sensor data */
   else if( ( !g_android_control->gyro_state.running && !running && sensor_type == ACCELEROMETER_SENSOR_TYPE )
            || ( !g_android_control->accel_state.running && !running && sensor_type == GYROSCOPE_SENSOR_TYPE ) )
   {
      LOC_LOGI("%s: Stopping sensor polling thread", __FUNCTION__);

      gpsone_unblock_thelper(&g_android_control->polling_task_helper);


      gpsone_join_thelper(&g_android_control->polling_task_helper);

      LOC_LOGI("%s: Finished joining sensor polling thread.", __FUNCTION__);
   }

   return true;
}

/*===========================================================================
FUNCTION    sp_and_hal_update_sensor_status

DESCRIPTION
  Generic function to start/stop a sensor based on provided sampling rate,
  batching rate, mounted state, and sensor information using Native Android API.

  running:       TRUE - start sensor reporting, FALSE - stop sensor reporting
  sampling_rate: Sampling rate in Hz
  batching_rate: Batch samples at this frequency in Hz
  mounted_state: 0 - Send Unmounted State, 1 - Send Mounted State, 2 - Do Not Send
  sensor_state:  The state associated with this sensor
  sensor_type:   What kind of sensor are we updating

DEPENDENCIES
   N/A

RETURN VALUE
   0  : Failure
   1  : Success

SIDE EFFECTS
   N/A

===========================================================================*/
static bool sp_and_hal_update_sensor_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, int32_t mounted_state,
                                    android_sensor_state_t* sensor_state, e_sensor_type sensor_type)
{
   const char* sensor_str;
   if( sensor_type == ACCELEROMETER_SENSOR_TYPE )
   {
      sensor_str = "accel";
   }
   else if( sensor_type == GYROSCOPE_SENSOR_TYPE )
   {
      sensor_str = "gyro";
   }
   else
   {
      LOC_LOGE("%s: Unknown Sensor Type %d", __FUNCTION__, sensor_type);
      return false;
   }

   /* No sensor state to use. */
   if( sensor_state == NULL )
   {
      LOC_LOGE("%s: No %s sensor state provided to start/stop", __FUNCTION__, sensor_str);
      return false;
   }

   /* No sensor to talk to. */
   if( sensor_state->handle < 0 )
   {
      LOC_LOGE("%s: No %s sensor to start/stop", __FUNCTION__, sensor_str);
      return false;
   }

   /* No State Change */
   if( !running && !sensor_state->running )
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

   int err = g_android_control->sensor_device->activate(g_android_control->sensor_device,
                                                        sensor_state->handle,
                                                        running);
   /* Unable to start/stop sensor */
   if( err != 0 )
   {
      LOC_LOGE("%s: Error starting/stopping %s sensor err = %d", __FUNCTION__, sensor_str, err);
      return false;
   }

   /* Since Accel/Gyro reporting is separate we only want to send XMSI once
      when starting sensor data injection. This will prevent an XMSI message
      if gyro data has already started. */
   if( !g_android_control->accel_state.running && !g_android_control->gyro_state.running &&
       running)
   {
      sp_send_msi_mounted_status(g_android_control->p_msg_q, mounted_state);
   }

   /* Update running state after performing MSI to make the function generic */
   sensor_state->running = running;

   /* Adjust sampling rate only if we are turning on the sensor */
   if( running && (sampling_rate != 0) )
   {
      int64_t sensor_delay = (int64_t)( (uint64_t)1000000000 / (uint64_t)sampling_rate );
      /* Change delay in samples to match sampling rate */
      err = g_android_control->sensor_device->setDelay(g_android_control->sensor_device,
                                                       sensor_state->handle,
                                                       sensor_delay);
      /* Unable to adjust sampling rate */
      if( err != 0 )
      {
         LOC_LOGE("%s: Error changing sensor delay = %ld on %s sensor err = %d", __FUNCTION__, sensor_delay, sensor_str, err);
         return false;
      }
   }

   /* Start up or stop polling thread if necessary. */
   sp_and_hal_update_polling_thread(sensor_type, sensor_state->running);

   return true;
}

/* ----------------------- END INTERNAL FUNCTIONS ---------------------------------------- */

/*===========================================================================

  FUNCTION:   sp_and_hal_init

  ===========================================================================*/
bool sp_and_hal_init(void* p_msg_q)
{
   LOC_LOGI("%s: Initializing Android HAL", __FUNCTION__);

   /* Initialize default values */
   if( sp_and_hal_init_defaults() == false )
   {
      LOC_LOGE("%s: Unable to initialize with default values!", __FUNCTION__);
      return false;
   }

   if( p_msg_q == NULL )
   {
      LOC_LOGE("%s: Invalid handle to msg queue passed in: %d!", __FUNCTION__, p_msg_q);
      sp_and_hal_destroy();
      return false;
   }

   g_android_control->p_msg_q = p_msg_q;

   /* Get the Android HW device module */
   int err = 0;
   struct sensors_module_t const* hw_module = NULL;

#ifndef DEBUG_X86
   err = hw_get_module(SENSORS_HARDWARE_MODULE_ID, (const struct hw_module_t **)&hw_module);
#endif

   if ( hw_module != NULL && err == 0 )
   {
      g_android_control->sensor_module = (struct sensors_module_t*)hw_module;
   }
   else
   {
      LOC_LOGE("%s: Could not initialize Android Sensor HW Module: %d!", __FUNCTION__, err);
      sp_and_hal_destroy();
      return false;
   }

   /* Get the Available Android Sensors. Look for Gyro and Accel Handles. */
   const struct sensor_t* sensor_list = NULL;
   int sensor_count = g_android_control->sensor_module->get_sensors_list(g_android_control->sensor_module, &sensor_list);
   int i = 0;
   int32_t min_accel_delay = INT_MAX;
   int32_t min_gyro_delay = INT_MAX;
   for( i = 0; sensor_list!= NULL && i < sensor_count; i++ )
   {
      LOC_LOGI("%s: Found Sensor Name: %s Vendor: %s Type: %d Min Delay: %d us",
               __FUNCTION__,
               sensor_list[i].name,
               sensor_list[i].vendor,
               sensor_list[i].type,
               sensor_list[i].minDelay);

      switch( sensor_list[i].type )
      {
         /* Pick sensors with best speed */
         case SENSOR_TYPE_ACCELEROMETER:
            if( sensor_list[i].minDelay > 0 && sensor_list[i].minDelay < min_accel_delay )
            {
               g_android_control->accel_state.handle = sensor_list[i].handle;
               min_accel_delay = sensor_list[i].minDelay;
            }
            break;
         case SENSOR_TYPE_GYROSCOPE:
            if( sensor_list[i].minDelay > 0 && sensor_list[i].minDelay < min_gyro_delay )
            {
               g_android_control->gyro_state.handle = sensor_list[i].handle;
               min_gyro_delay = sensor_list[i].minDelay;
            }
            break;
         default:
            break;
      }
   }

   /* Neither Accel or Gyro was found for use. */
   if( g_android_control->accel_state.handle < 0 && g_android_control->gyro_state.handle < 0 )
   {
      LOC_LOGE("%s: No Accel or Gyro to use for sensor data!", __FUNCTION__);
      sp_and_hal_destroy();
      return false;
   }

   /* Get handle to Android Sensor Data Module */
   struct sensors_poll_device_t* poll_device = NULL;
   err = sensors_open(&g_android_control->sensor_module->common, &poll_device);
   if( poll_device != NULL && err == 0 )
   {
      g_android_control->sensor_device = (struct sensors_poll_device_t*)poll_device;
   }
   else
   {
      LOC_LOGE("%s: Could not initialize Android Sensor Data Module: %d!", __FUNCTION__, err);
      sp_and_hal_destroy();
      return false;
   }

   return true;
}

/*===========================================================================

  FUNCTION:   sp_and_hal_update_accel_status

  ===========================================================================*/
bool sp_and_hal_update_accel_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state)
{
   if( g_android_control == NULL )
   {
      LOC_LOGE("%s: Initialization required to perform operation", __FUNCTION__);
      return false;
   }

   return sp_and_hal_update_sensor_status(running, sampling_rate, batching_rate, mounted_state, &g_android_control->accel_state, ACCELEROMETER_SENSOR_TYPE);
}

/*===========================================================================

  FUNCTION:   sp_and_hal_update_gyro_status

  ===========================================================================*/
bool sp_and_hal_update_gyro_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state)
{
   if( g_android_control == NULL )
   {
      LOC_LOGE("%s: Initialization required to perform operation", __FUNCTION__);
      return false;
   }

   return sp_and_hal_update_sensor_status(running, sampling_rate, batching_rate, mounted_state, &g_android_control->gyro_state, GYROSCOPE_SENSOR_TYPE);
}

/*===========================================================================

  FUNCTION:   sp_and_hal_destroy

  ===========================================================================*/
bool sp_and_hal_destroy()
{
   if ( g_android_control != NULL )
   {
      /* Turn off sensor data reporting if it is still on */
      if ( g_android_control->gyro_state.running )
      {
         if ( sp_and_hal_update_gyro_status(false, 0, 0, 0) != true )
         {
            LOC_LOGE("%s: sp_and_update_gyro_status() could not stop gyro_data", __FUNCTION__ );
            return false;
         }
      }

      if ( g_android_control->accel_state.running )
      {
         if ( sp_and_hal_update_accel_status(false, 0, 0, 0) != true )
         {
            LOC_LOGE("%s: sp_and_update_accel_status() could not stop accel_data", __FUNCTION__);
            return false;
         }
      }

      /* Close handle to Android Sensor Data Module */
      if ( g_android_control->sensor_device != NULL )
      {
         int err = sensors_close(g_android_control->sensor_device);
         if ( err == 0 )
         {
            g_android_control->sensor_device = NULL;
         }
         else
         {
            LOC_LOGE("%s: Could not destroy Android Sensor Data Module: %d!", __FUNCTION__, err);
            return false;
         }
      }

      /* Remove our handle to HW device */
      g_android_control->sensor_module = NULL;

      /* Remove reference time mutex */
      int mutex_rc = pthread_mutex_destroy(&g_android_control->ref_time_mutex);
      if ( mutex_rc != 0 )
      {
         LOC_LOGE("%s: Could not destroy ref_time mutex rc = %d errno = %d!", __FUNCTION__, mutex_rc, errno);
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

  FUNCTION:   sp_and_hal_get_sensor_time

  ===========================================================================*/
bool sp_and_hal_get_sensor_time(uint32_t* p_time_ms)
{
   if( g_android_control == NULL )
   {
      LOC_LOGE("%s: Initialization required to perform operation", __FUNCTION__);
      return false;
   }

   if( p_time_ms == NULL )
   {
      LOC_LOGE("%s: Invalid pointer passed for reading sensor time.", __FUNCTION__);
      return false;
   }

   /* Atomic access to sensor reference time and system reference time. */
   pthread_mutex_lock( &g_android_control->ref_time_mutex );

   /* Take local time stamp to approximate current sensor time. */
   uint32_t current_sys_time_ms = sp_read_sys_time_ms();
   uint32_t time_delta_ms = current_sys_time_ms - g_android_control->sys_ref_time_ms;
   uint32_t current_sensor_time_ms = g_android_control->sensor_ref_time_ms + time_delta_ms;

   LOC_LOGI("%s: Grabbing Sensor time: cur_sensor_time = %lu sys_ref = %lu cur_sys = %lu sensor_ref = %lu",
            __FUNCTION__,
            current_sensor_time_ms,
            g_android_control->sys_ref_time_ms,
            current_sys_time_ms,
            g_android_control->sensor_ref_time_ms);

   pthread_mutex_unlock( &g_android_control->ref_time_mutex );

   *p_time_ms = current_sensor_time_ms;

   return true;
}
