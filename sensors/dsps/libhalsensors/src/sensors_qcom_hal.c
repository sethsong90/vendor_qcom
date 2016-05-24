/*============================================================================
  @file sensors_qcom_hal.c

  @brief
  This file contains the sensors HAL implementation.

  Copyright (c) 2010-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
============================================================================*/

/*===========================================================================

                           INCLUDE FILES

===========================================================================*/
#include <cutils/native_handle.h>
#include <cutils/properties.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <hardware.h>
#include <inttypes.h>
#include <math.h>
#include <sensors.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "sensors_hal.h"
#include "sensor_reg.h"
#include "common_log.h"
#include "sensor1.h"
#include "sns_reg_api_v02.h"
#include "sns_smgr_api_v01.h"
#include "sns_sam_amd_v01.h"
#include "sns_sam_rmd_v01.h"
#include "sns_sam_vmd_v01.h"
#include "sns_sam_basic_gestures_v01.h"
#include "sns_sam_fns_v01.h"
#include "sns_sam_bte_v01.h"
#include "sns_sam_tap_v01.h"
#include "sns_sam_gyro_tap2_v01.h"
#include "sns_sam_facing_v01.h"
#include "sns_sam_integ_angle_v01.h"
#include "sns_sam_gravity_vector_v01.h"
#include "sns_sam_rotation_vector_v01.h"
#include "sns_sam_orientation_v01.h"
#include "sns_sam_mag_cal_v01.h"
#include "sns_sam_sensor_thresh_v01.h"
#include "sns_sam_ped_v01.h"
#include "sns_sam_pam_v01.h"
#include "sns_sam_smd_v01.h"
#include "sns_sam_game_rotation_vector_v01.h"
#include "sns_sam_cmc_v01.h"

#define FEATURE_SNS_HAL_SAM_OTN

#include <linux/msm_dsps.h>

#include "log_codes.h"
#include "sns_log_types.h"
#include "sns_reg_common.h"

/*===========================================================================

                   INTERNAL DEFINITIONS AND TYPES

===========================================================================*/
/* Properties used by the HAL */

/* Properties to enable sensor features */
#define HAL_PROP_QMD            "ro.qualcomm.sensors.qmd"
#define HAL_PROP_DEBUG          "debug.qualcomm.sns.hal"
#define HAL_PROP_GESTURES       "ro.qc.sdk.sensors.gestures"
#define HAL_PROP_PEDOMETER      "ro.qualcomm.sensors.pedometer"
#define HAL_PROP_STEP_DETECTOR  "ro.qc.sensors.step_detector"
#define HAL_PROP_STEP_COUNTER   "ro.qc.sensors.step_counter"
#define HAL_PROP_PAM            "ro.qualcomm.sensors.pam"
#define HAL_PROP_SCRN_ORTN      "ro.qualcomm.sensors.scrn_ortn"
#define HAL_PROP_SMD            "ro.qualcomm.sensors.smd"
#define HAL_PROP_GRV            "ro.qualcomm.sensors.game_rv"
#define HAL_PROP_GEOMAGNETIC_RV "ro.qualcomm.sensors.georv"
#define HAL_PROP_CMC            "ro.qualcomm.sensors.cmc"

#define HAL_PROP_BTE            "ro.qualcomm.sensors.bte"
#define HAL_PROP_FNS            "ro.qualcomm.sensors.fns"
#define HAL_PROP_VMD            "ro.qualcomm.sensors.vmd"
#define HAL_PROP_GTAP           "ro.qualcomm.sensors.gtap"

/* Properties to set max rates. Limit length of prop name to PROP_NAME_MAX */
#define HAL_PROP_MAX_ACCEL          "ro.qc.sensors.max_accel_rate"
#define HAL_PROP_MAX_GYRO           "ro.qc.sensors.max_gyro_rate"
#define HAL_PROP_MAX_MAG            "ro.qc.sensors.max_mag_rate"
#define HAL_PROP_MAX_GRAV           "ro.qc.sensors.max_grav_rate"
#define HAL_PROP_MAX_GRAV_NO_GYRO   "ro.qc.sensors.max_grav_no_gyro"
#define HAL_PROP_MAX_ROTVEC         "ro.qc.sensors.max_rotvec_rate"
#define HAL_PROP_MAX_GEOMAG_ROTVEC  "ro.qc.sensors.max_geomag_rotvec"
#define HAL_PROP_MAX_GAMERV         "ro.qc.sensors.max_gamerv_rate"

typedef enum
{
  SENSOR_MODE_CONT,
  SENSOR_MODE_EVENT,
  SENSOR_MODE_TRIG
} sensor_trigger_mode;

/* Enable pressure sensor support by default */
#define FEATURE_SNS_HAL_SMGR_PRESSURE

// The new JB-MR2 sensor types are defined in sensors.h
// Temporarilty define here in case we have not picked up JB-MR2 sensors.h
#ifndef SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED
#define SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED     (14)
#endif /* SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED */
#ifndef SENSOR_TYPE_GAME_ROTATION_VECTOR
#define SENSOR_TYPE_GAME_ROTATION_VECTOR            (15)
#endif /* SENSOR_TYPE_GAME_ROTATION_VECTOR */
#ifndef SENSOR_TYPE_GYROSCOPE_UNCALIBRATED
#define SENSOR_TYPE_GYROSCOPE_UNCALIBRATED          (16)
#endif /* SENSOR_TYPE_GYROSCOPE_UNCALIBRATED */
#ifndef SENSOR_TYPE_SIGNIFICANT_MOTION
#define SENSOR_TYPE_SIGNIFICANT_MOTION              (17)
#endif /* SENSOR_TYPE_SIGNIFICANT_MOTION */
#ifndef SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR
#define SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR     (20)
#endif /* SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR */
#ifndef SENSORS_BATCH_DRY_RUN
#define SENSORS_BATCH_DRY_RUN                       0x01
#endif /* SENSORS_BATCH_WAKE_UPON_FIFO_FULL */
#ifndef SENSORS_BATCH_WAKE_UPON_FIFO_FULL
#define SENSORS_BATCH_WAKE_UPON_FIFO_FULL           0x02
#endif /* SENSORS_BATCH_WAKE_UPON_FIFO_FULL */

/* Duration that must be supported for Wake upon Fifo Full flag */
#define WUFF_MIN_DURATION_SEC 10

/* Sensor data rate in Hz */
#define FREQ_FASTEST_HZ   200.0f
#define FREQ_GAME_HZ      50.0f
#define FREQ_UI_HZ        15.0f
#define FREQ_NORMAL_HZ    5.0f

/* Min rate for qfusion sensors */
#define MIN_FREQ_QFUSION_HZ   1

/* Default max rates for sensors, unless overridden by property value */
#define MAX_ACCEL_RATE   "120"
#define MAX_GYRO_RATE    "200"
#define MAX_MAG_RATE     "60"
#define MAX_QFUSION_RATE "120"

#define FREQ_ACCEL_DEF_HZ_STR          MAX_ACCEL_RATE
#define FREQ_GYRO_DEF_HZ_STR           MAX_GYRO_RATE
#define FREQ_MAG_DEF_HZ_STR            MAX_MAG_RATE
#define FREQ_GRAV_DEF_HZ_STR           MAX_QFUSION_RATE
#define FREQ_GRAV_NO_GYRO_DEF_HZ_STR   MAX_MAG_RATE
#define FREQ_ORIENT_DEF_HZ_STR         MAX_QFUSION_RATE
#define FREQ_ROTVEC_DEF_HZ_STR         MAX_QFUSION_RATE
#define FREQ_GEOMAG_ROTVEC_DEF_HZ_STR  MAX_MAG_RATE
#define FREQ_GAMERV_DEF_HZ_STR         MAX_QFUSION_RATE

/* Indicates if a client exists for RMD.
 * cb_mutex should be locked before calling this. */
#define RMD_IS_ACTIVE ((1ULL << HANDLE_MOTION_RELATIVE) & (g_sensor_control->active_sensors))
#define AMD_IS_ACTIVE ((1ULL << HANDLE_MOTION_ABSOLUTE) & (g_sensor_control->active_sensors))

#ifndef MAX
#define MAX(a,b)                \
  ({ __typeof__ (a) _a = (a);   \
    __typeof__ (b) _b = (b);    \
    _a > _b ? _a : _b; })
#endif /* MAX */

#ifndef MIN
#define MIN(a,b)                \
  ({ __typeof__ (a) _a = (a);   \
    __typeof__ (b) _b = (b);    \
    _a < _b ? _a : _b; })
#endif /* MIN */

#define SNS_ROT_VEC_HEAD_ERR_EST_RAD (M_PI / 6.f)

#define ARRAY_SIZE(x) (int)(sizeof(x) / sizeof((x)[0]))

/* Delay between activate/batch/setDelay and making the changes take
   effect */
#define HAL_ACQUIRE_RESOURCES_DELAY_NSEC 10000000L /* 10ms */

/*===========================================================================

                    FUNCTION PROTOTYPES

===========================================================================*/

/* functions defined by Sensors HAL in (hardware/libhardware/include/hardware/sensors.h) */
static int _hal_sensors_open( const struct hw_module_t* module,
    const char* name, struct hw_device_t* *device);
static int _hal_sensors_close( struct hw_device_t *dev );
static int _hal_sensors_activate( struct sensors_poll_device_t *dev,
    int handle, int enabled);
static int _hal_sensors_set_delay( struct sensors_poll_device_t *dev,
    int handle, int64_t ns);
static int _hal_sensors_data_poll( struct sensors_poll_device_t *dev,
    sensors_event_t* data, int count);
static int _hal_sensors_batch( sensors_poll_device_1* dev,
    int handle, int flags, int64_t period_ns, int64_t timeout);
static int _hal_sensors_flush( sensors_poll_device_1* dev, int handle );

static bool hal_smgr_report_add( int handle,
                                 hal_sensor_control_t* sensor_ctl, uint32_t sample_rate,
                                 uint32_t report_rate, bool wake_upon_fifo_full, bool do_batching,
                                 bool wait_for_resp );
static bool hal_smgr_report_delete( int handle,
    hal_sensor_control_t* sensor_ctl );
static bool hal_sam_qmd_report_add( int handle,
    hal_sensor_control_t* sensor_ctl, uint32_t report_rate );

static sensor1_error_e hal_sensor1_open( hal_sensor_control_t* sensor_ctl );
static void hal_sensor1_close( hal_sensor_control_t *sensor_ctl );
static void* hal_oem_data_poll( void *threadid );

static void hal_cleanup_resources();
static int hal_reinit();
static void* hal_acquire_resources( void* );

static bool hal_is_event_sensor( int handle, uint32_t report_rate );

static uint32_t
hal_calc_sample_rate( hal_sensor_control_t *sensor_ctl, int handle, uint64_t ns );
static bool is_active( int handle );

/*===========================================================================

                         GLOBAL VARIABLES

===========================================================================*/

static hal_sensor_control_t*  g_sensor_control = NULL;   /* sensor control device.
                                                            g_sensor_control->cb_mutex
                                                            MUST be used to protect access
                                                            to members of this structure  */
static uint8_t                g_ss_responses;            /* keeps track of single sensor
                                                            responses */

static hal_oem_sensor_info_t  g_oem_sensor_control;

hal_log_level_e               g_hal_log_level = HAL_LOG_LEVEL_WARN;
hal_log_level_e               *g_hal_log_level_ptr = &g_hal_log_level; /* Log level for debug printfs */

/* This variable determines the order in which sensors will be placed (if it
 * is available) into the Android sensor list.
 * In general: If there is more than one type of the same sensor, place
 * Qualcomm-based sensors and algorithms first */
static const int g_sensor_list_order[] = {
  /* Physical sensors: */
  HANDLE_ACCELERATION,
  HANDLE_MAGNETIC_FIELD,
  HANDLE_MAGNETIC_FIELD_UNCALIBRATED,
  HANDLE_MAGNETIC_FIELD_SAM,
  HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM,
  HANDLE_GYRO,
  HANDLE_GYRO_UNCALIBRATED,
  HANDLE_OEM_PROXIMITY,
  HANDLE_PROXIMITY,
  HANDLE_OEM_LIGHT,
  HANDLE_LIGHT,
  HANDLE_PRESSURE,
  HANDLE_RELATIVE_HUMIDITY,
  HANDLE_AMBIENT_TEMPERATURE,

  /* Android virtual sensors via SAM: */
  HANDLE_GRAVITY,
  HANDLE_LINEAR_ACCEL,
  HANDLE_ROTATION_VECTOR,
  HANDLE_SAM_STEP_DETECTOR,
  HANDLE_SAM_STEP_COUNTER,
  HANDLE_SIGNIFICANT_MOTION,
  HANDLE_GAME_ROTATION_VECTOR,
  HANDLE_GEOMAGNETIC_ROTATION_VECTOR,
  HANDLE_ORIENTATION,

  /* Android virtual sensors via SMGR: */
  HANDLE_SMGR_STEP_DETECTOR,
  HANDLE_SMGR_STEP_COUNT,
  HANDLE_SMGR_SMD,
  HANDLE_SMGR_GAME_RV,

  /* Qualcomm value added sensors: */
  HANDLE_GESTURE_FACE_N_SHAKE,
  HANDLE_GESTURE_BRING_TO_EAR,
  HANDLE_MOTION_ABSOLUTE,
  HANDLE_MOTION_RELATIVE,
  HANDLE_MOTION_VEHICLE,
  HANDLE_GESTURE_BASIC_GESTURES,
  HANDLE_GESTURE_TAP,
  HANDLE_GESTURE_FACING,
  HANDLE_GESTURE_TILT,
  HANDLE_GESTURE_GYRO_TAP,
  HANDLE_PEDOMETER,
  HANDLE_PAM,
  HANDLE_MOTION_ACCEL,
  HANDLE_CMC,
  HANDLE_RGB,
  HANDLE_IR_GESTURE,
  HANDLE_SAR
};


/*==========================================================================

                                FUNCTIONS

===========================================================================*/

/*===========================================================================

  FUNCTION:  hal_is_mag_available

===========================================================================*/
/*!
  @brief
  Lets the caller know if a magnetometer is present on the phone

  @return True, if a mag device is supported, false otherwise
*/

bool hal_is_mag_available( void )
{
  return(g_sensor_control->sensor_list[HANDLE_MAGNETIC_FIELD].handle == HANDLE_MAGNETIC_FIELD);
}

/*===========================================================================

  FUNCTION:  hal_is_gyro_available

===========================================================================*/
bool hal_is_gyro_available( void )
{
  return(g_sensor_control->sensor_list[HANDLE_GYRO].handle == HANDLE_GYRO);
}

/*===========================================================================

  FUNCTION:  hal_insert_queue

===========================================================================*/
bool
hal_insert_queue( sensors_event_t const * data_ptr )
{
  hal_sensor_dataq_t* q_ptr;
  bool rv = true;

  q_ptr = malloc( sizeof(hal_sensor_dataq_t) );
  if( NULL != q_ptr )
  {
    q_ptr->next = NULL;
    q_ptr->data = *data_ptr;

    if( g_sensor_control->q_head_ptr == NULL )
    {
      /* queue is empty */
      g_sensor_control->q_tail_ptr = q_ptr;
      g_sensor_control->q_head_ptr = q_ptr;
    }
    else
    {
      /* append to tail and update tail ptr */
      g_sensor_control->q_tail_ptr->next = q_ptr;
      g_sensor_control->q_tail_ptr = q_ptr;
    }
  }
  else
  {
    HAL_LOG_ERROR("%s: malloc() error", __FUNCTION__ );
    rv = false;
  }

  memcpy( &g_sensor_control->last_event[data_ptr->sensor], data_ptr, sizeof(sensors_event_t) );

  return (rv);
}

/*===========================================================================

  FUNCTION:  hal_remove_from_queue

===========================================================================*/
/*!
  @brief
  Removes sensor data from head of the queue
  Helper function

  @param data_ptr: pointer to data

  @return true if data exists in the queue

  @dependencies Caller needs to lock the g_sensor_control->data_mutex before
                calling this function
*/
static bool
hal_remove_from_queue( sensors_event_t* data_ptr )
{
  hal_sensor_dataq_t* q_ptr;
  bool rv = false;

  if( NULL != g_sensor_control->q_head_ptr )
  {
    /* copy the data from head */
    q_ptr = g_sensor_control->q_head_ptr;
    *data_ptr = g_sensor_control->q_head_ptr->data;

    /* update the pointers */
    if( g_sensor_control->q_head_ptr == g_sensor_control->q_tail_ptr )
    {
      /* queue has only one element */
      g_sensor_control->q_tail_ptr = NULL;
    }
    g_sensor_control->q_head_ptr = g_sensor_control->q_head_ptr->next;

    free( q_ptr );
    rv = true;
  }

  return (rv);
}


/*===========================================================================

  FUNCTION:  hal_wait_for_response

===========================================================================*/
bool
hal_wait_for_response( int timeout,
                       pthread_mutex_t* cb_mutex_ptr,
                       pthread_cond_t*  cond_ptr,
                       bool *cond_var )
{
  bool    ret_val = false;               /* the return value of this function */
  int     rc = 0;                        /* return code from pthread calls */
  struct timeval    present_time;
  struct timespec   expire_time;

  HAL_LOG_DEBUG("%s: timeout=%d", __FUNCTION__, timeout );

  /* special case where callback is issued before the main function
     can wait on cond */
  if (*cond_var == true)
  {
    HAL_LOG_DEBUG("%s: cb has arrived without waiting", __FUNCTION__ );
    ret_val = true;
  }
  else
  {
    /* Calculate absolute expire time */
    gettimeofday(&present_time, NULL);

    /* Convert from timeval to timespec */
    expire_time.tv_sec  = present_time.tv_sec;
    expire_time.tv_nsec = present_time.tv_usec * 1000;
    expire_time.tv_sec += timeout / 1000;

    /* calculate carry over */
    if ( (present_time.tv_usec + (timeout % 1000) * 1000) >= 1000000)
    {
      expire_time.tv_sec += 1;
    }
    expire_time.tv_nsec = (expire_time.tv_nsec + (timeout % 1000) * 1000000) % 1000000000;

    while( *cond_var != true && rc != ETIMEDOUT )
    {
      if( 0 == timeout )
      {
        rc = pthread_cond_wait( cond_ptr, cb_mutex_ptr );
      }
      else
      {
        /* Wait for the callback until timeout expires */
        rc = pthread_cond_timedwait( cond_ptr, cb_mutex_ptr,
                                     &expire_time);
      }
      if( 0 != rc )
      {
        HAL_LOG_ERROR( "%s: pthread_cond_timedwait() rc=%d (cond: %i)",
                       __FUNCTION__, rc, *cond_var );
      }
      ret_val = ( rc == 0 || *cond_var ) ? true : false;
    }
  }

  *cond_var = false;

  return ret_val;
}

/*===========================================================================

  FUNCTION:  hal_signal_response()

===========================================================================*/
static void
hal_signal_response( bool error,
                     pthread_cond_t* cond_ptr )
{
  g_sensor_control->error = error;
  g_sensor_control->is_resp_arrived = true;

  pthread_cond_signal( cond_ptr );
}

/*===========================================================================

  FUNCTION:  hal_signal_ind()

===========================================================================*/
void
hal_signal_ind( pthread_cond_t* cond_ptr )
{
  g_sensor_control->is_ind_arrived = true;
  pthread_cond_signal( cond_ptr );
}

/*===========================================================================

  FUNCTION:  hal_single_sensor_info_request()

===========================================================================*/
static void hal_single_sensor_info_request( int req_id, uint8_t sensor_id )
{
  sensor1_error_e                           error;
  sensor1_msg_header_s                      req_hdr;
  sns_smgr_single_sensor_info_req_msg_v01*  smgr_req;

  HAL_LOG_DEBUG("%s: %d", __FUNCTION__, sensor_id );

  error = hal_sensor1_open( g_sensor_control );
  if( SENSOR1_SUCCESS != error  )
  {
    HAL_LOG_ERROR( "%s: hal_sensor1_open failed %d", __FUNCTION__, error );
  }
  else
  {
    req_hdr.service_number = SNS_SMGR_SVC_ID_V01;
    req_hdr.msg_id = SNS_SMGR_SINGLE_SENSOR_INFO_REQ_V01;
    req_hdr.msg_size = sizeof( sns_smgr_single_sensor_info_req_msg_v01 );
    req_hdr.txn_id = req_id;

    error = sensor1_alloc_msg_buf( g_sensor_control->hndl,
                                  sizeof(sns_smgr_single_sensor_info_req_msg_v01),
                                  (void**)&smgr_req );
    if( SENSOR1_SUCCESS != error )
    {
      HAL_LOG_ERROR("%s: msg alloc failed: %d", __FUNCTION__, error );
      return;
    }

    smgr_req->SensorID = sensor_id;
    HAL_LOG_DEBUG("%s: txn_id: %d sensor_id: %u", __FUNCTION__,
                  req_id, smgr_req->SensorID );

    if( (error = sensor1_write( g_sensor_control->hndl, &req_hdr,
                                smgr_req )) != SENSOR1_SUCCESS )
    {
      /* free the message buffer */
      sensor1_free_msg_buf( g_sensor_control->hndl, smgr_req );
    }
  }
}

/*===========================================================================

  FUNCTION:  hal_process_all_sensor_info_resp

===========================================================================*/
/*!
*/
static void
hal_process_all_sensor_info_resp( sns_smgr_all_sensor_info_resp_msg_v01* smgr_info_resp )
{
  uint32_t                                  i;

  HAL_LOG_DEBUG("%s: SensorInfo_len: %d", __FUNCTION__, smgr_info_resp->SensorInfo_len );

  if( smgr_info_resp->Resp.sns_result_t != 0 )
  {
    HAL_LOG_ERROR("%s: R: %u, E: %u", __FUNCTION__, smgr_info_resp->Resp.sns_result_t,
         smgr_info_resp->Resp.sns_err_t );
    hal_signal_response( true, &g_sensor_control->cb_arrived_cond );
    return;
  }

  if( smgr_info_resp->SensorInfo_len > 0 )
  {
    g_sensor_control->num_smgr_sensors = smgr_info_resp->SensorInfo_len;
    for(i=0; i<MAX_NUM_SENSORS; i++ )
    {
      g_sensor_control->sensor_list[i].is_attrib_ok = false;
    }

    for(i=0; i<g_sensor_control->num_smgr_sensors; i++ )
    {
      hal_single_sensor_info_request( i, smgr_info_resp->SensorInfo[i].SensorID );
    }
  }
  else
  {
    hal_signal_response( false, &g_sensor_control->cb_arrived_cond );
  }
}

/*===========================================================================

  FUNCTION:  hal_process_single_sensor_info_resp

===========================================================================*/
/*!
*/
static void
hal_process_single_sensor_info_resp ( uint8_t txn_id,
                                      sns_smgr_single_sensor_info_resp_msg_v01* smgr_resp )
{
  int                                  i;
  sns_smgr_sensor_datatype_info_s_v01* sensor_datatype;
  hal_sensor_info_t*                   sensor_info = NULL;
  char                                 max_rate_prop_value[PROPERTY_VALUE_MAX];
  char                                *strtol_endptr;
  int                                  tmp_max_rate;

  g_ss_responses ++;

  HAL_LOG_DEBUG("%s: data_type_info_len: %d", __FUNCTION__,
                smgr_resp->SensorInfo.data_type_info_len );

  if( smgr_resp->Resp.sns_result_t != 0 )
  {
    /* error */
    HAL_LOG_ERROR("%s: Error: %u ", __FUNCTION__,
         smgr_resp->Resp.sns_result_t  );
    hal_signal_response( true, &g_sensor_control->cb_arrived_cond );
    return;
  }

  for( i=0; i<(int)smgr_resp->SensorInfo.data_type_info_len; i++ )
  {
    /* todo .. add support for multiple sensors of the same type */
    sensor_info = NULL;
    sensor_datatype = &smgr_resp->SensorInfo.data_type_info[i];

    HAL_LOG_DEBUG("%s: txn: %u, ns: %u", __FUNCTION__, txn_id,
                  (unsigned int)g_sensor_control->num_smgr_sensors );

    switch( sensor_datatype->SensorID )
    {
      case SNS_SMGR_ID_ACCEL_V01:
        HAL_LOG_DEBUG("%s: Accel, DTy: %d", __FUNCTION__, sensor_datatype->DataType );
        if( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == sensor_datatype->DataType )
        {
          sensor_info = &g_sensor_control->sensor_list[ HANDLE_ACCELERATION ];
          sensor_info->handle = HANDLE_ACCELERATION;
          sensor_info->type = SENSOR_TYPE_ACCELEROMETER;
          sensor_info->resolution = (float)((float)sensor_datatype->Resolution *
                                            UNIT_CONVERT_ACCELERATION);
          sensor_info->max_range = (float)((float)sensor_datatype->MaxRange *
                                           UNIT_CONVERT_ACCELERATION);
        }
        break;
      case SNS_SMGR_ID_MAG_V01:
        HAL_LOG_DEBUG("%s: MAG DTy: %d", __FUNCTION__, sensor_datatype->DataType );
        if( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == sensor_datatype->DataType )
        {
          sensor_info = &g_sensor_control->sensor_list[HANDLE_MAGNETIC_FIELD];
          /* Other sensors use the information here - retain this information */
          sensor_info->handle = HANDLE_MAGNETIC_FIELD;
          sensor_info->type = SENSOR_TYPE_MAGNETIC_FIELD;
          sensor_info->resolution = (float)((float)sensor_datatype->Resolution *
                                            UNIT_CONVERT_MAGNETIC_FIELD);
          sensor_info->max_range = (float)((float)sensor_datatype->MaxRange *
                                           UNIT_CONVERT_MAGNETIC_FIELD);
        }
        else if( SNS_SMGR_DATA_TYPE_SECONDARY_V01 == sensor_datatype->DataType )
        {
          /* we dont handle mag temperature */
        }
        break;
      case SNS_SMGR_ID_PROX_LIGHT_V01:
        HAL_LOG_DEBUG("%s: ALS DTy: %d", __FUNCTION__, sensor_datatype->DataType );
        if( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == sensor_datatype->DataType )
        {
          sensor_info = &g_sensor_control->sensor_list[ HANDLE_PROXIMITY ];
          sensor_info->handle = HANDLE_PROXIMITY;
          sensor_info->type = SENSOR_TYPE_PROXIMITY;
          sensor_info->resolution = (float)((float)sensor_datatype->Resolution *
                                            UNIT_CONVERT_PROXIMITY);
          sensor_info->max_range = (float)((float)sensor_datatype->MaxRange *
                                           UNIT_CONVERT_PROXIMITY);
        }
        else if( SNS_SMGR_DATA_TYPE_SECONDARY_V01 == sensor_datatype->DataType )
        {
          sensor_info = &g_sensor_control->sensor_list[ HANDLE_LIGHT ];
          sensor_info->handle = HANDLE_LIGHT;
          sensor_info->type = SENSOR_TYPE_LIGHT;
          sensor_info->resolution = (float)((float)sensor_datatype->Resolution *
                                            UNIT_CONVERT_LIGHT);
          sensor_info->max_range = (float)((float)sensor_datatype->MaxRange *
                                           UNIT_CONVERT_LIGHT);
        }
        break;
      case SNS_SMGR_ID_GYRO_V01:
        HAL_LOG_DEBUG("%s: GYRO DTy: %d", __FUNCTION__, sensor_datatype->DataType );
        if( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == sensor_datatype->DataType )
        {
          sensor_info = &g_sensor_control->sensor_list[ HANDLE_GYRO ];
          sensor_info->handle = HANDLE_GYRO;
          sensor_info->type = SENSOR_TYPE_GYROSCOPE;
          sensor_info->resolution = (float)((float)sensor_datatype->Resolution * UNIT_CONVERT_GYRO);
          sensor_info->max_range = (float)((float)sensor_datatype->MaxRange * UNIT_CONVERT_GYRO);
        }
        break;
#ifdef FEATURE_SNS_HAL_SMGR_PRESSURE
      case SNS_SMGR_ID_PRESSURE_V01:
        HAL_LOG_DEBUG("%s: PRESSURE DTy: %d", __FUNCTION__, sensor_datatype->DataType );
        if( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == sensor_datatype->DataType )
        {
          sensor_info = &g_sensor_control->sensor_list[ HANDLE_PRESSURE ];
          sensor_info->handle = HANDLE_PRESSURE;
          sensor_info->type = SENSOR_TYPE_PRESSURE;
          sensor_info->resolution = (float)((float)sensor_datatype->Resolution * UNIT_CONVERT_PRESSURE);
          sensor_info->max_range = (float)((float)sensor_datatype->MaxRange * UNIT_CONVERT_PRESSURE);
        }
        else if( SNS_SMGR_DATA_TYPE_SECONDARY_V01 == sensor_datatype->DataType )
        {
          /* dont handle secondary data */
        }
        break;
#endif
      case SNS_SMGR_ID_HUMIDITY_V01:
        HAL_LOG_DEBUG("%s: HUMIDITY DTy: %d", __FUNCTION__, sensor_datatype->DataType );
        if( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == sensor_datatype->DataType )
        {
          sensor_info = &g_sensor_control->sensor_list[ HANDLE_RELATIVE_HUMIDITY ];
          sensor_info->handle = HANDLE_RELATIVE_HUMIDITY;
          sensor_info->type = SENSOR_TYPE_RELATIVE_HUMIDITY;
          sensor_info->resolution = (float)((float)sensor_datatype->Resolution *
                                            UNIT_CONVERT_RELATIVE_HUMIDITY);
          sensor_info->max_range = (float)((float)sensor_datatype->MaxRange *
                                           UNIT_CONVERT_RELATIVE_HUMIDITY);
        }
        else if( SNS_SMGR_DATA_TYPE_SECONDARY_V01 == sensor_datatype->DataType )
        {
          sensor_info = &g_sensor_control->sensor_list[ HANDLE_AMBIENT_TEMPERATURE ];
          sensor_info->handle = HANDLE_AMBIENT_TEMPERATURE;
          sensor_info->type = SENSOR_TYPE_AMBIENT_TEMPERATURE;
          sensor_info->resolution = (float)((float)sensor_datatype->Resolution * UNIT_CONVERT_TEMPERATURE);
          sensor_info->max_range = (float)((float)sensor_datatype->MaxRange * UNIT_CONVERT_TEMPERATURE);
        }
        break;
      case SNS_SMGR_ID_STEP_EVENT_V01:
        if( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == sensor_datatype->DataType )
        {
          HAL_LOG_DEBUG("%s: Step Event DTy: %d", __FUNCTION__, sensor_datatype->DataType );
          sensor_info = &g_sensor_control->sensor_list[ HANDLE_SMGR_STEP_DETECTOR ];
          sensor_info->handle = HANDLE_SMGR_STEP_DETECTOR;
          sensor_info->type = SENSOR_TYPE_STEP_DETECTOR;
          sensor_info->resolution = 1;
          sensor_info->max_range = 1;
        }
        break;
      case SNS_SMGR_ID_SMD_V01:
        HAL_LOG_DEBUG("%s: Sig Motion DTy: %d", __FUNCTION__, sensor_datatype->DataType );
        if( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == sensor_datatype->DataType )
        {
          sensor_info = &g_sensor_control->sensor_list[ HANDLE_SMGR_SMD ];
          sensor_info->handle = HANDLE_SMGR_SMD;
          sensor_info->type = SENSOR_TYPE_SIGNIFICANT_MOTION;
          sensor_info->resolution = 1;
          sensor_info->max_range = 1;
        }
        break;
      case SNS_SMGR_ID_STEP_COUNT_V01:
        if( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == sensor_datatype->DataType )
        {
          HAL_LOG_DEBUG("%s: Step Count DTy: %d", __FUNCTION__, sensor_datatype->DataType );
          sensor_info = &g_sensor_control->sensor_list[ HANDLE_SMGR_STEP_COUNT ];
          sensor_info->handle = HANDLE_SMGR_STEP_COUNT;
          sensor_info->type = SENSOR_TYPE_STEP_COUNTER;
          sensor_info->resolution = 1;
          sensor_info->max_range = 1;
        }
        break;
      case SNS_SMGR_ID_GAME_ROTATION_VECTOR_V01:
        HAL_LOG_DEBUG("%s: Game RV DTy: %d", __FUNCTION__, sensor_datatype->DataType );
        if( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == sensor_datatype->DataType )
        {
          sensor_info = &g_sensor_control->sensor_list[ HANDLE_SMGR_GAME_RV ];
          sensor_info->handle = HANDLE_SMGR_GAME_RV;
          sensor_info->type = SENSOR_TYPE_GAME_ROTATION_VECTOR;
          sensor_info->resolution = 1;
          sensor_info->max_range = 1;
        }
        break;
      case SNS_SMGR_ID_RGB_V01:
        HAL_LOG_DEBUG("%s: RGB DTy: %d", __FUNCTION__, sensor_datatype->DataType );
        if( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == sensor_datatype->DataType )
        {
          sensor_info = &g_sensor_control->sensor_list[ HANDLE_RGB ];
          sensor_info->handle = HANDLE_RGB;
          sensor_info->type = SENSOR_TYPE_RGB;
          sensor_info->resolution = (float)((float)sensor_datatype->Resolution * UNIT_CONVERT_Q16);
          sensor_info->max_range = (float)((float)sensor_datatype->MaxRange * UNIT_CONVERT_Q16);
        }
        break;
      case SNS_SMGR_ID_IR_GESTURE_V01:
        HAL_LOG_DEBUG("%s: IR GESTURE DTy: %d", __FUNCTION__, sensor_datatype->DataType );
        if( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == sensor_datatype->DataType )
        {
          sensor_info = &g_sensor_control->sensor_list[ HANDLE_IR_GESTURE ];
          sensor_info->handle = HANDLE_IR_GESTURE;
          sensor_info->type = SENSOR_TYPE_IR_GESTURE;
          sensor_info->resolution = (float)((float)sensor_datatype->Resolution *
                                            UNIT_CONVERT_Q16);
          sensor_info->max_range = (float)((float)sensor_datatype->MaxRange *
                                           UNIT_CONVERT_Q16);
        }
        break;
      case SNS_SMGR_ID_SAR_V01:
        HAL_LOG_DEBUG("%s: SAR DTy: %d", __FUNCTION__, sensor_datatype->DataType );
        if( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == sensor_datatype->DataType )
        {
          sensor_info = &g_sensor_control->sensor_list[ HANDLE_SAR ];
          sensor_info->handle = HANDLE_SAR;
          sensor_info->type = SENSOR_TYPE_SAR;
          sensor_info->resolution = (float)((float)sensor_datatype->Resolution *
                                            UNIT_CONVERT_Q16);
          sensor_info->max_range = (float)((float)sensor_datatype->MaxRange *
                                           UNIT_CONVERT_Q16);
        }
        break;
      default:
        HAL_LOG_ERROR(" %s Unknown sensor type: %d", __FUNCTION__, sensor_datatype->DataType );
        break;
    }

    if( sensor_info != NULL )
    {
      strlcpy( sensor_info->name, sensor_datatype->SensorName,
               SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
      strlcpy( sensor_info->vendor, sensor_datatype->VendorName,
              SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );

      sensor_info->version = sensor_datatype->Version;

      if( sensor_datatype->MaxSampleRate <= 500 )
      {
        sensor_info->max_freq = sensor_datatype->MaxSampleRate;
      }
      else if( sensor_datatype->MaxSampleRate >= 2000 )
      {
        sensor_info->max_freq = 1000.0/sensor_datatype->MaxSampleRate;
      }
      else
      {
        sensor_info->max_freq = 1;
        HAL_LOG_ERROR(" %s Invalid sample rate: %u", __FUNCTION__, sensor_datatype->MaxSampleRate );
      }

      /* Lower max rate if property overrides value returned from sensor */
      switch( sensor_info->type ) {
        case SENSOR_TYPE_ACCELEROMETER:
          property_get( HAL_PROP_MAX_ACCEL, max_rate_prop_value,
                        FREQ_ACCEL_DEF_HZ_STR );
          break;
        case SENSOR_TYPE_MAGNETIC_FIELD:
        case SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED:
          property_get( HAL_PROP_MAX_MAG, max_rate_prop_value,
                        FREQ_MAG_DEF_HZ_STR );
          break;
        case SENSOR_TYPE_GYROSCOPE:
          property_get( HAL_PROP_MAX_GYRO, max_rate_prop_value,
                        FREQ_GYRO_DEF_HZ_STR );
          break;
        default:
          max_rate_prop_value[0] = 0;
          break;
      }
      errno = 0;
      tmp_max_rate = strtol( max_rate_prop_value, &strtol_endptr, 0 );
      if( 0 == errno && strtol_endptr != max_rate_prop_value )
      {
        sensor_info->max_freq = MIN( sensor_info->max_freq,
                                     tmp_max_rate );
      }
      if( sensor_info->max_freq >= FREQ_FASTEST_HZ )
      {
        sensor_info->max_freq = FREQ_FASTEST_HZ;
      }

      sensor_info->min_freq = SNS_SMGR_REPORT_RATE_MIN_HZ_V01;
      sensor_info->power = (float)((float)sensor_datatype->MaxPower * UNIT_CONVERT_POWER);

      if ( sensor_info->type == SENSOR_TYPE_MAGNETIC_FIELD &&
           (g_sensor_control->mag_cal_src != HAL_MAG_CAL_SRC_SMGR &&
            g_sensor_control->mag_cal_src != HAL_MAG_CAL_SRC_NONE))
      {
        HAL_LOG_DEBUG("%s: No Cal support for MAG through SMGR, not enabled", __FUNCTION__);
        sensor_info->is_attrib_ok = false;
      }
      else
      {
        sensor_info->is_attrib_ok = true;
      }

      if( !smgr_resp->num_buffered_reports_valid ||
          HANDLE_PROXIMITY == sensor_info->handle )
      {
        sensor_info->max_buffered_samples = 0;
      }
      else
      {
        sensor_info->max_buffered_samples = smgr_resp->num_buffered_reports[i];
      }

      HAL_LOG_INFO("%s: name: %s, vendor: %s, maxRange: %u, res: %u, power: %u, max_freq: %u max_buffered_samples: %u", __FUNCTION__,
                   sensor_datatype->SensorName, sensor_datatype->VendorName,
                   sensor_datatype->MaxRange, sensor_datatype->Resolution, sensor_datatype->MaxPower,
                   sensor_datatype->MaxSampleRate,
                   sensor_info->max_buffered_samples );
      HAL_LOG_DEBUG("%s: name: %s, vendor: %s, maxRange: %f, res: %f, power: %f, max_freq: %f", __FUNCTION__,
                    sensor_info->name, sensor_info->vendor,
                    sensor_info->max_range, sensor_info->resolution, sensor_info->power, sensor_info->max_freq );
    }
  }

  HAL_LOG_DEBUG("%s: txn: %u, ns: %u, ss_resp: %u", __FUNCTION__, txn_id,
       (unsigned int)g_sensor_control->num_smgr_sensors,
       g_ss_responses );

  if( g_ss_responses == g_sensor_control->num_smgr_sensors )
  {
    /* All responses received */
    hal_signal_response( false, &g_sensor_control->cb_arrived_cond );
  }
}

/*===========================================================================

  FUNCTION:  hal_flush_send_cmplt

===========================================================================*/
/*!
  @brief
  Sends META_DATA_FLUSH_COMPLETE event for a handle.  Must hold data_mutex.

  */
static void hal_flush_send_cmplt( int handle )
{
#ifdef SENSORS_DEVICE_API_VERSION_1_1
  sensors_event_t flush_evt = {
    .version = META_DATA_VERSION,
    .sensor = 0,
    .type = SENSOR_TYPE_META_DATA,
    .reserved0 = 0,
    .timestamp = 0,
    .meta_data = {
      .what = META_DATA_FLUSH_COMPLETE,
      .sensor = handle
    }
  };

  if(true == g_sensor_control->flush_requested[handle] )
  {
    HAL_LOG_DEBUG("%s: handle=%d", __FUNCTION__, handle);
    g_sensor_control->flush_requested[handle] = false;
    if( hal_insert_queue( &flush_evt ) )
    {
      hal_signal_ind( &g_sensor_control->data_arrived_cond );
    }
  }
#endif /* SENSORS_DEVICE_API_VERSION_1_1 */
}

/*===========================================================================

  FUNCTION:  hal_process_buffering_resp

  This function processes SMGR response messages for adding/deleting a report.

  Response needs to be signalled to hal_main
  thread that initiated the add/delete. And this signalling needs to use
  cb_mutex.
===========================================================================*/
/*!
*/
static void
hal_process_buffering_resp( sns_smgr_buffering_resp_msg_v01* smgr_resp,
                            sensor1_msg_header_s* msg_hdr)
{
  bool error = false;
  int i;

  if( smgr_resp->Resp.sns_result_t != 0 )
  {
    HAL_LOG_ERROR("%s: Result: %u, Error: %u", __FUNCTION__,
                  smgr_resp->Resp.sns_result_t, smgr_resp->Resp.sns_err_t  );
    error = true;
  }

  if( smgr_resp->AckNak != SNS_SMGR_RESPONSE_ACK_SUCCESS_V01 &&
      smgr_resp->AckNak != SNS_SMGR_RESPONSE_ACK_MODIFIED_V01 )
  {
    HAL_LOG_ERROR( "%s: %d Error: %u Reason: %u", __FUNCTION__, smgr_resp->ReportId,
                   smgr_resp->AckNak, smgr_resp->ReasonPair[0].Reason );
    error = true;
  }

  if( smgr_resp->AckNak == SNS_SMGR_RESPONSE_NAK_REPORT_ID_V01 )
  {
    error = false;
  }

  HAL_LOG_DEBUG( "%s: Id: %u Resp: %u txn id %d", __FUNCTION__,
                 smgr_resp->ReportId, smgr_resp->AckNak, msg_hdr->txn_id );

  pthread_mutex_lock( &g_sensor_control->cb_mutex );
  if( msg_hdr->txn_id != TXN_ID_NO_RESP_SIGNALLED )
  {
    hal_signal_response( error, &g_sensor_control->cb_arrived_cond );
  }
  pthread_mutex_unlock( &g_sensor_control->cb_mutex );

  pthread_mutex_lock( &g_sensor_control->data_mutex );
  for( i = 0; i < SAM_HANDLE_BASE; i++ )
  {
    hal_flush_send_cmplt( i );
  }
  pthread_mutex_unlock( &g_sensor_control->data_mutex );
}

/*===========================================================================

  FUNCTION:  hal_process_report_ind

===========================================================================*/
/*!
 * Process a report indication from SMGR
 *
 * @param[i] smgr_ind Periodic indication message
*/
static void
hal_process_report_ind( sns_smgr_periodic_report_ind_msg_v01* smgr_ind )
{
  sns_smgr_data_item_s_v01* smgr_data;
  sensors_event_t           sensor_data;

  HAL_LOG_VERBOSE("%s: St: %d ReportId: %d Rate: %d, Len: %d", __FUNCTION__,
                  smgr_ind->status, smgr_ind->ReportId,
                  smgr_ind->CurrentRate, smgr_ind->Item_len );

  /* Check report status */
  if( smgr_ind->status != SNS_SMGR_REPORT_OK_V01 )
  {
    HAL_LOG_ERROR("%s: Report Status: %u", __FUNCTION__, smgr_ind->status );
    return;
  }

  /* the HAL requests one item per report for most sensors */
  smgr_data = &smgr_ind->Item[0];

  HAL_LOG_VERBOSE("%s: Id: %s: Ty: %u Q: %u", __FUNCTION__,
       ((smgr_data->SensorId==SNS_SMGR_ID_ACCEL_V01)      ? "ACCEL_DATA" :
        (smgr_data->SensorId==SNS_SMGR_ID_MAG_V01)        ? "MAG_DATA" :
        (smgr_data->SensorId==SNS_SMGR_ID_PROX_LIGHT_V01) ? "PROX_LIGHT_DATA" :
        (smgr_data->SensorId==SNS_SMGR_ID_GYRO_V01)       ? "GYRO_DATA" :
        (smgr_data->SensorId==SNS_SMGR_ID_PRESSURE_V01)   ? "PRESSURE_DATA" :
        (smgr_data->SensorId==SNS_SMGR_ID_STEP_EVENT_V01)  ? "STEP_EVT" :
        (smgr_data->SensorId==SNS_SMGR_ID_SMD_V01)  ? "SIG_MOTION" :
        (smgr_data->SensorId==SNS_SMGR_ID_STEP_COUNT_V01)  ? "STEP_CNT" :
        (smgr_data->SensorId==SNS_SMGR_ID_GAME_ROTATION_VECTOR_V01)  ? "GAME_RV" :
        (smgr_data->SensorId==SNS_SMGR_ID_HUMIDITY_V01)   ? "HUMIDITY_DATA" :
        (smgr_data->SensorId==SNS_SMGR_ID_RGB_V01)   ? "RBG_DATA" :
        (smgr_data->SensorId==SNS_SMGR_ID_SAR_V01)   ? "SAR_DATA" :
        (smgr_data->SensorId==SNS_SMGR_ID_IR_GESTURE_V01) ? "IR_GESTURE_DATA" :
        "invalid"), smgr_data->DataType, smgr_data->ItemQuality );

  /* todo .. item sensitivity and item flag (rail) ?? */
  if( smgr_data->ItemQuality != SNS_SMGR_ITEM_QUALITY_INVALID_FAILED_SENSOR_V01 &&
      smgr_data->ItemQuality != SNS_SMGR_ITEM_QUALITY_INVALID_NOT_READY_V01 &&
      smgr_data->ItemQuality != SNS_SMGR_ITEM_QUALITY_INVALID_SUSPENDED_V01 )
  {
    memset( &sensor_data, 0, sizeof(sensor_data) );
    switch( smgr_data->SensorId )
    {
      case SNS_SMGR_ID_ACCEL_V01:
        if( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == smgr_data->DataType )
        {
          sensor_data.type = SENSOR_TYPE_ACCELEROMETER;
          sensor_data.sensor = HANDLE_ACCELERATION;
          /* Convert from SAE to Android co-ordinates and scale
            x' = y; y' = x; z' = -z;                                        */
          sensor_data.acceleration.x =
            (float)(smgr_data->ItemData[1]) * UNIT_CONVERT_ACCELERATION;
          sensor_data.acceleration.y =
            (float)(smgr_data->ItemData[0]) * UNIT_CONVERT_ACCELERATION;
          sensor_data.acceleration.z =
            (float)(-smgr_data->ItemData[2]) * UNIT_CONVERT_ACCELERATION;
          HAL_LOG_VERBOSE("%s: X: %f Y: %f Z: %f ", __FUNCTION__,
                          sensor_data.acceleration.x,
                          sensor_data.acceleration.y,
                          sensor_data.acceleration.z);

          if( smgr_ind->ReportId == g_sensor_control->report_ids[HANDLE_MOTION_ACCEL] )
          {
            /* Corresponds to screen orientation req, fill in the right type */
            sensor_data.type = SENSOR_TYPE_SCREEN_ORIENTATION;
            sensor_data.sensor = HANDLE_MOTION_ACCEL;
          }
        }
        break;

      case SNS_SMGR_ID_MAG_V01:
        if (smgr_ind->ReportId == g_sensor_control->report_ids[HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM])
        {
          if (!(smgr_data->ItemFlags & SNS_SMGR_ITEM_FLAG_AUTO_CAL_V01))
          {
            /* Convert from SAE to Android co-ordinates and scale
              x' = y; y' = x; z' = -z;                                        */
            sensor_data.type = SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED;
            sensor_data.sensor = HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM;
            g_sensor_control->mag_cal_cur_sample.sample.x_uncalib =
              (float)(smgr_data->ItemData[1]) * UNIT_CONVERT_MAGNETIC_FIELD;
            g_sensor_control->mag_cal_cur_sample.sample.y_uncalib =
              (float)(smgr_data->ItemData[0]) * UNIT_CONVERT_MAGNETIC_FIELD;
            g_sensor_control->mag_cal_cur_sample.sample.z_uncalib =
              (float)(-smgr_data->ItemData[2]) * UNIT_CONVERT_MAGNETIC_FIELD;
            g_sensor_control->mag_cal_cur_sample.smgr_ts = smgr_data->TimeStamp;
            HAL_LOG_VERBOSE("%s: raw_X: %f raw_Y: %f raw_Z: %f", __FUNCTION__,
                            g_sensor_control->mag_cal_cur_sample.sample.x_uncalib,
                            g_sensor_control->mag_cal_cur_sample.sample.y_uncalib,
                            g_sensor_control->mag_cal_cur_sample.sample.z_uncalib );
          }
          else
          {
            HAL_LOG_ERROR( "%s: Incorrect flag %d", __FUNCTION__, smgr_data->ItemFlags );
          }
        }
        else if (smgr_ind->ReportId == g_sensor_control->report_ids[HANDLE_MAGNETIC_FIELD_UNCALIBRATED])
        {
          if ( smgr_ind->Item_len == 2 )
          {
            float x_bias, y_bias, z_bias;
            sns_smgr_data_item_s_v01* factory_smgr_data;
            sns_smgr_data_item_s_v01* calibrated_smgr_data;

            if( smgr_ind->Item[0].ItemFlags & SNS_SMGR_ITEM_FLAG_AUTO_CAL_V01 )
            {
              calibrated_smgr_data = &smgr_ind->Item[0];
              factory_smgr_data = &smgr_ind->Item[1];
            }
            else
            {
              factory_smgr_data = &smgr_ind->Item[0];
              calibrated_smgr_data = &smgr_ind->Item[1];
            }

            sensor_data.type = SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED;
            sensor_data.sensor = HANDLE_MAGNETIC_FIELD_UNCALIBRATED;

            /* Convert from SAE to Android co-ordinates and scale
              x' = y; y' = x; z' = -z;                                        */
            sensor_data.uncalibrated_magnetic.x_uncalib =
              (float)(factory_smgr_data->ItemData[1]) * UNIT_CONVERT_MAGNETIC_FIELD;
            sensor_data.uncalibrated_magnetic.y_uncalib =
              (float)(factory_smgr_data->ItemData[0]) * UNIT_CONVERT_MAGNETIC_FIELD;
            sensor_data.uncalibrated_magnetic.z_uncalib =
              (float)(-factory_smgr_data->ItemData[2]) * UNIT_CONVERT_MAGNETIC_FIELD;

            x_bias = (float)(factory_smgr_data->ItemData[1] - calibrated_smgr_data->ItemData[1]) * UNIT_CONVERT_MAGNETIC_FIELD;
            y_bias = (float)(factory_smgr_data->ItemData[0] - calibrated_smgr_data->ItemData[0]) * UNIT_CONVERT_MAGNETIC_FIELD;
            z_bias = (float)-(factory_smgr_data->ItemData[2] - calibrated_smgr_data->ItemData[2]) * UNIT_CONVERT_MAGNETIC_FIELD;

            sensor_data.uncalibrated_magnetic.x_bias = x_bias;
            sensor_data.uncalibrated_magnetic.y_bias = y_bias;
            sensor_data.uncalibrated_magnetic.z_bias = z_bias;

            HAL_LOG_VERBOSE( "%s: Uncal mag x:%f y:%f z:%f xb:%f yb:%f zb:%f",
                             __FUNCTION__,
                             sensor_data.uncalibrated_magnetic.x_uncalib,
                             sensor_data.uncalibrated_magnetic.y_uncalib,
                             sensor_data.uncalibrated_magnetic.z_uncalib,
                             sensor_data.uncalibrated_magnetic.x_bias,
                             sensor_data.uncalibrated_magnetic.y_bias,
                             sensor_data.uncalibrated_magnetic.z_bias );
          }
          else
          {
            HAL_LOG_ERROR( "%s: Incorrect item len %d", __FUNCTION__,
                           smgr_ind->Item_len );
          }
        }
        else if (smgr_ind->ReportId == g_sensor_control->report_ids[HANDLE_MAGNETIC_FIELD])
        {
          sensor_data.type = SENSOR_TYPE_MAGNETIC_FIELD;
          sensor_data.sensor = HANDLE_MAGNETIC_FIELD;

          /* Convert from SAE to Android co-ordinates and scale
            x' = y; y' = x; z' = -z;                                        */
          sensor_data.magnetic.x =
            (float)(smgr_data->ItemData[1]) * UNIT_CONVERT_MAGNETIC_FIELD;
          sensor_data.magnetic.y =
            (float)(smgr_data->ItemData[0]) * UNIT_CONVERT_MAGNETIC_FIELD;
          sensor_data.magnetic.z =
            (float)(-smgr_data->ItemData[2]) * UNIT_CONVERT_MAGNETIC_FIELD;
          HAL_LOG_VERBOSE( "%s: Calibrated Mag, %f, %f, %f",
                           __FUNCTION__,
                           sensor_data.magnetic.x,
                           sensor_data.magnetic.y,
                           sensor_data.magnetic.z);
        }
        else
        {
          HAL_LOG_ERROR( "%s: MAG Unknown report ID",__FUNCTION__);
        }
        break;

      case SNS_SMGR_ID_PROX_LIGHT_V01:
        if( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == smgr_data->DataType )
        {
          uint8_t temp;
          sensor_data.type = SENSOR_TYPE_PROXIMITY;
          sensor_data.sensor = HANDLE_PROXIMITY;
#ifndef HAL_SUPPORT_DISTANCE
          /* The proximity driver does not support distance yet but reports near/far */
          temp = smgr_data->ItemData[0] * UNIT_CONVERT_Q16;
          if( 0 == temp )
          {
            /* far */
            sensor_data.distance = g_sensor_control->sensor_list[ HANDLE_PROXIMITY ].max_range;
          }
          else
          {
            /* near */
            sensor_data.distance = 0;
          }
#else
          sensor_data.distance =
            (float)(smgr_data->ItemData[1]) * UNIT_CONVERT_PROXIMITY;
#endif
          HAL_LOG_VERBOSE("%s: prox data: %x %x %f", __FUNCTION__, smgr_data->ItemData[0], smgr_data->ItemData[1],
                          sensor_data.distance );
        }
        else if( SNS_SMGR_DATA_TYPE_SECONDARY_V01 == smgr_data->DataType )
        {
          sensor_data.type = SENSOR_TYPE_LIGHT;
          sensor_data.sensor = HANDLE_LIGHT;
          sensor_data.light =
            (float)(smgr_data->ItemData[0]) * UNIT_CONVERT_LIGHT;
          HAL_LOG_VERBOSE("%s: %x %f", __FUNCTION__, smgr_data->ItemData[0],
                          sensor_data.light );
        }
        break;

      case SNS_SMGR_ID_GYRO_V01:
        if (smgr_ind->ReportId == g_sensor_control->report_ids[HANDLE_GYRO_UNCALIBRATED])
        {
          if ( smgr_ind->Item_len == 2 )
          {
            float x_bias, y_bias, z_bias;
            sns_smgr_data_item_s_v01* factory_smgr_data;
            sns_smgr_data_item_s_v01* calibrated_smgr_data;

            if( smgr_ind->Item[0].ItemFlags & SNS_SMGR_ITEM_FLAG_AUTO_CAL_V01 )
            {
              calibrated_smgr_data = &smgr_ind->Item[0];
              factory_smgr_data = &smgr_ind->Item[1];
            }
            else
            {
              factory_smgr_data = &smgr_ind->Item[0];
              calibrated_smgr_data = &smgr_ind->Item[1];
            }

            sensor_data.type = SENSOR_TYPE_GYROSCOPE_UNCALIBRATED;
            sensor_data.sensor = HANDLE_GYRO_UNCALIBRATED;

            /* Convert from SAE to Android co-ordinates and scale
              x' = y; y' = x; z' = -z;                                        */
            sensor_data.uncalibrated_gyro.x_uncalib =
              (float)(factory_smgr_data->ItemData[1]) * UNIT_CONVERT_GYRO;
            sensor_data.uncalibrated_gyro.y_uncalib =
              (float)(factory_smgr_data->ItemData[0]) * UNIT_CONVERT_GYRO;
            sensor_data.uncalibrated_gyro.z_uncalib =
              (float)(-factory_smgr_data->ItemData[2]) * UNIT_CONVERT_GYRO;

            x_bias = (float)(factory_smgr_data->ItemData[1] - calibrated_smgr_data->ItemData[1]) * UNIT_CONVERT_GYRO;
            y_bias = (float)(factory_smgr_data->ItemData[0] - calibrated_smgr_data->ItemData[0]) * UNIT_CONVERT_GYRO;
            z_bias = (float)-(factory_smgr_data->ItemData[2] - calibrated_smgr_data->ItemData[2]) * UNIT_CONVERT_GYRO;

            sensor_data.uncalibrated_gyro.x_bias = x_bias;
            sensor_data.uncalibrated_gyro.y_bias = y_bias;
            sensor_data.uncalibrated_gyro.z_bias = z_bias;

            HAL_LOG_VERBOSE( "%s: Uncal gyro x:%f y:%f z:%f xb:%f yb:%f zb:%f",
                             __FUNCTION__,
                             sensor_data.uncalibrated_gyro.x_uncalib,
                             sensor_data.uncalibrated_gyro.y_uncalib,
                             sensor_data.uncalibrated_gyro.z_uncalib,
                             sensor_data.uncalibrated_gyro.x_bias,
                             sensor_data.uncalibrated_gyro.y_bias,
                             sensor_data.uncalibrated_gyro.z_bias );
          }
          else
          {
            HAL_LOG_ERROR( "%s: Incorrect item len %d", __FUNCTION__,
                           smgr_ind->Item_len );
          }
        }
        else
        {
          sensor_data.type = SENSOR_TYPE_GYROSCOPE;
          sensor_data.sensor = HANDLE_GYRO;
          /* Convert from SAE to Android co-ordinates and scale
            x' = y; y' = x; z' = -z;                                        */
          sensor_data.gyro.x =
            (float)(smgr_data->ItemData[1]) * UNIT_CONVERT_GYRO;
          sensor_data.gyro.y =
            (float)(smgr_data->ItemData[0]) * UNIT_CONVERT_GYRO;
          sensor_data.gyro.z =
            (float)(-smgr_data->ItemData[2]) * UNIT_CONVERT_GYRO;
        }
        break;

#ifdef FEATURE_SNS_HAL_SMGR_PRESSURE
      case SNS_SMGR_ID_PRESSURE_V01:
        if( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == smgr_data->DataType )
        {
          sensor_data.type = SENSOR_TYPE_PRESSURE;
          sensor_data.sensor = HANDLE_PRESSURE;

          sensor_data.pressure =
            (float)(smgr_data->ItemData[0]) * UNIT_CONVERT_PRESSURE;
          HAL_LOG_VERBOSE("%s: P: %f", __FUNCTION__, sensor_data.pressure );
        }
        else if( SNS_SMGR_DATA_TYPE_SECONDARY_V01 == smgr_data->DataType )
        {
          /* dont handle secondary data */
        }
        break;
#endif
      case SNS_SMGR_ID_HUMIDITY_V01:
        if( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == smgr_data->DataType )
        {
          sensor_data.type = SENSOR_TYPE_RELATIVE_HUMIDITY;
          sensor_data.sensor = HANDLE_RELATIVE_HUMIDITY;

          sensor_data.relative_humidity =
            (float)(smgr_data->ItemData[0]) * UNIT_CONVERT_RELATIVE_HUMIDITY;
          HAL_LOG_DEBUG("%s: x: P: %f", __FUNCTION__, sensor_data.relative_humidity );
        }
        else if( SNS_SMGR_DATA_TYPE_SECONDARY_V01 == smgr_data->DataType )
        {
          sensor_data.type = SENSOR_TYPE_AMBIENT_TEMPERATURE;
          sensor_data.sensor = HANDLE_AMBIENT_TEMPERATURE;

          sensor_data.temperature =
            (float)(smgr_data->ItemData[0]) * UNIT_CONVERT_TEMPERATURE;
          HAL_LOG_VERBOSE("%s: Tempr: %f", __FUNCTION__, sensor_data.temperature );
        }
        break;
      case SNS_SMGR_ID_STEP_COUNT_V01:
        if( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == smgr_data->DataType )
        {
          sensor_data.type = SENSOR_TYPE_STEP_COUNTER;
          sensor_data.sensor = HANDLE_SMGR_STEP_COUNT;
#ifdef SENSORS_DEVICE_API_VERSION_1_1
          sensor_data.u64.step_counter = smgr_data->ItemData[0];
#else
          sensor_data.step_counter = smgr_data->ItemData[0];
#endif /* #else SENSORS_DEVICE_API_VERSION_1_1 */
          HAL_LOG_DEBUG("%s: Step count %d", __FUNCTION__, smgr_data->ItemData[0]);
        }
        break;
      case SNS_SMGR_ID_SMD_V01:
        if( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == smgr_data->DataType )
        {
          sns_smgr_buffering_req_msg_v01*  smgr_req;
          sensor1_error_e       error;
          sensor1_msg_header_s  req_hdr;
          sensor_data.type = SENSOR_TYPE_SIGNIFICANT_MOTION;
          sensor_data.sensor = HANDLE_SMGR_SMD;

          sensor_data.data[0] = 1;
          HAL_LOG_DEBUG("%s: Sig Motion detected", __FUNCTION__);

          /* SMD is one-shot. Manually delete SMD report here */
          g_sensor_control->current_freq[HANDLE_SMGR_SMD] = 0;
          g_sensor_control->current_rpt_rate[HANDLE_SMGR_SMD] = 0;
          g_sensor_control->last_event[HANDLE_SMGR_SMD].timestamp = 0;
          g_sensor_control->active_sensors &= ~(1ULL << HANDLE_SMGR_SMD);
          error = sensor1_alloc_msg_buf( g_sensor_control->hndl,
                                         sizeof(sns_smgr_buffering_req_msg_v01),
                                         (void**)&smgr_req );
          if( SENSOR1_SUCCESS != error )
          {
            HAL_LOG_ERROR("%s: sensor1_alloc_msg_buf() failed: %u", __FUNCTION__, error );
            break;
          }
          /* Message header */
          req_hdr.service_number = SNS_SMGR_SVC_ID_V01;
          req_hdr.msg_id = SNS_SMGR_BUFFERING_REQ_V01;
          req_hdr.msg_size = sizeof( sns_smgr_buffering_req_msg_v01 );
          /* Set txn_id to TXN_ID_NO_RESP_SIGNALLED so that a response is not signaled */
          req_hdr.txn_id = TXN_ID_NO_RESP_SIGNALLED;

          /* Message body */
          smgr_req->ReportId = g_sensor_control->report_ids[HANDLE_SMGR_SMD];
          smgr_req->Action = SNS_SMGR_BUFFERING_ACTION_DELETE_V01;

          /* Send request */
          g_sensor_control->error = false;
          if( (error = sensor1_write( g_sensor_control->hndl, &req_hdr,
                                      smgr_req )) != SENSOR1_SUCCESS )
          {
            sensor1_free_msg_buf( g_sensor_control->hndl, smgr_req );
            HAL_LOG_ERROR("%s: sensor1_write() error: %u", __FUNCTION__, error );
            break;
          }
        }
        /* Don't wait for a response */
        break;
      case SNS_SMGR_ID_STEP_EVENT_V01:
        if( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == smgr_data->DataType )
        {
          sensor_data.type = SENSOR_TYPE_STEP_DETECTOR;
          sensor_data.sensor = HANDLE_SMGR_STEP_DETECTOR;

          sensor_data.data[0] = 1;
          HAL_LOG_DEBUG("%s: Step detected", __FUNCTION__);
        }
        break;
      case SNS_SMGR_ID_GAME_ROTATION_VECTOR_V01:
        if( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == smgr_data->DataType )
        {
          sensor_data.type = SENSOR_TYPE_GAME_ROTATION_VECTOR;
          sensor_data.sensor = HANDLE_SMGR_GAME_RV;

          sensor_data.data[0] = smgr_data->ItemData[0] * UNIT_CONVERT_Q16;
          sensor_data.data[1] = smgr_data->ItemData[1] * UNIT_CONVERT_Q16;
          sensor_data.data[2] = smgr_data->ItemData[2] * UNIT_CONVERT_Q16;
          sensor_data.data[3] = sqrtf( sensor_data.data[0] * sensor_data.data[0] +
                                       sensor_data.data[1] * sensor_data.data[1] +
                                       sensor_data.data[2] * sensor_data.data[2] );
          HAL_LOG_VERBOSE("%s: Game RV x:%f y:%f z:%f w:%f", __FUNCTION__,
                          sensor_data.data[0], sensor_data.data[1],
                          sensor_data.data[2], sensor_data.data[3] );
        }
        break;
      case SNS_SMGR_ID_RGB_V01:
        if (smgr_ind->ReportId == g_sensor_control->report_ids[HANDLE_RGB])
        {
          if ( smgr_ind->Item_len == 2 )
          {
            sns_smgr_data_item_s_v01* rgb_data = &smgr_ind->Item[0];
            sns_smgr_data_item_s_v01* ct_c_data = &smgr_ind->Item[1];

            sensor_data.type = SENSOR_TYPE_RGB;
            sensor_data.sensor = HANDLE_RGB;

            /* RGB data - raw ADC counts */
            sensor_data.data[0] = (float)(rgb_data->ItemData[0]);
            sensor_data.data[1] = (float)(rgb_data->ItemData[1]);
            sensor_data.data[2] = (float)(rgb_data->ItemData[2]);

            /* Color temp - in Q16 */
            sensor_data.data[3] = (float)(ct_c_data->ItemData[0])* UNIT_CONVERT_COLOR_TEMP;
            /* clear data in raw ADC counts */
            sensor_data.data[4] = (float)(ct_c_data->ItemData[1]);

            HAL_LOG_VERBOSE("%s: R: %f, G: %f, B: %f, CT: %f, C: %f", __FUNCTION__, sensor_data.data[0],
                                 sensor_data.data[1], sensor_data.data[2], sensor_data.data[3],
                                 sensor_data.data[4] );
          }
          else
          {
            HAL_LOG_ERROR( "%s: Incorrect item len %d", __FUNCTION__,
                           smgr_ind->Item_len );
          }
        }
        break;
      case SNS_SMGR_ID_IR_GESTURE_V01:
        if( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == smgr_data->DataType )
        {
          sensor_data.type = SENSOR_TYPE_IR_GESTURE;
          sensor_data.sensor = HANDLE_IR_GESTURE;

          sensor_data.data[0] =
            (float)(smgr_data->ItemData[0]) * UNIT_CONVERT_Q16;
          HAL_LOG_VERBOSE("%s: x: P: %f", __FUNCTION__, sensor_data.data[0] );
        }
        break;
      case SNS_SMGR_ID_SAR_V01:
        if( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == smgr_data->DataType )
        {
          sensor_data.type = SENSOR_TYPE_SAR;
          sensor_data.sensor = HANDLE_SAR;

          sensor_data.data[0] =
            (float)(smgr_data->ItemData[0]) * UNIT_CONVERT_Q16;

          HAL_LOG_VERBOSE("%s: x: P: %f", __FUNCTION__, sensor_data.data[0] );
        }
        break;
      default:
        break;
    }

    sensor_data.version = sizeof(sensors_event_t);

    sensor_data.timestamp = hal_timestamp_calc((uint64_t)smgr_data->TimeStamp, sensor_data.sensor );
    if(g_sensor_control->is_ltcy_measure_enabled == true)
    {
      if( sensor_data.sensor == HANDLE_ACCELERATION ||
          sensor_data.sensor == HANDLE_MAGNETIC_FIELD ||
          sensor_data.sensor == HANDLE_MAGNETIC_FIELD_UNCALIBRATED ||
          sensor_data.sensor == HANDLE_MAGNETIC_FIELD_SAM ||
          sensor_data.sensor == HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM ||
          sensor_data.sensor == HANDLE_PROXIMITY ||
          sensor_data.sensor == HANDLE_GYRO ||
          sensor_data.sensor == HANDLE_PRESSURE ||
          sensor_data.sensor == HANDLE_RELATIVE_HUMIDITY ||
          sensor_data.sensor == HANDLE_AMBIENT_TEMPERATURE ||
          sensor_data.sensor == HANDLE_RGB ||
          sensor_data.sensor == HANDLE_IR_GESTURE ||
          sensor_data.sensor == HANDLE_SAR )
      {
        /* Right now latency measurement is supported for only one sensor at a time. Thus, when it's enabled,
         * HAL will receive one sensor data everytime. Record it's DSPS ticks here for latency measurement
         * purpose. */
        g_sensor_control->ltcy_measure_dsps_tick = smgr_data->TimeStamp;
      }
    }

    /* Only fill in the accuracy field if the sensor uses a sensors_vec_t: */
    if( sensor_data.type == SENSOR_TYPE_ACCELEROMETER ||
        sensor_data.type == SENSOR_TYPE_MAGNETIC_FIELD ||
        sensor_data.type == SENSOR_TYPE_GYROSCOPE )
    {

      if( smgr_data->SensorId==SNS_SMGR_ID_MAG_V01 )
      {
        if( smgr_data->ItemFlags == SNS_SMGR_ITEM_FLAG_AUTO_CAL_V01)
        {
          ((sensors_vec_t*)(sensor_data.data))->status = SENSOR_STATUS_ACCURACY_HIGH;
        }
        else
        {
          ((sensors_vec_t*)(sensor_data.data))->status = SENSOR_STATUS_UNRELIABLE;
        }
      }
      else
      {
        /* accuracy .. is this good ?? */
        if( SNS_SMGR_ITEM_QUALITY_CURRENT_SAMPLE_V01 == smgr_data->ItemQuality )
        {
          ((sensors_vec_t*)(sensor_data.data))->status = SENSOR_STATUS_ACCURACY_HIGH;
        }
        else
        {
          ((sensors_vec_t*)(sensor_data.data))->status = SENSOR_STATUS_ACCURACY_MEDIUM;
        }
      }
    }

    if(sensor_data.sensor == HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM)
    {
      HAL_LOG_VERBOSE("%s: smgr_ts: %u sam_ts: %u apps_ts: %lld", __FUNCTION__,
                      g_sensor_control->mag_cal_cur_sample.smgr_ts,
                      g_sensor_control->mag_cal_cur_sample.sam_ts,
                      sensor_data.timestamp );
      if(g_sensor_control->mag_cal_cur_sample.smgr_ts ==
         g_sensor_control->mag_cal_cur_sample.sam_ts)
      {
        /* Report the uncal mag sample if the SAM and SMGR timestamps are the same */
        sensor_data.uncalibrated_magnetic = g_sensor_control->mag_cal_cur_sample.sample;
      } else {
        /* Bail out if the timestamps are not the same */
        return;
      }
    }

    /* Proximity and Light are only updated on changes.
     * Proximity is handled by SAM on DSPS. Check for light changes here.
     * Update the timestamp in the saved event to the current time, and then compare
     * sensor events. If the events are identical, don't report them */
    g_sensor_control->last_event[sensor_data.sensor].timestamp = sensor_data.timestamp;

    /* SMD is a one-shot sensor. Make sure we don't repeat any samples*/
    g_sensor_control->last_event[HANDLE_SMGR_SMD].timestamp = 0;
    g_sensor_control->last_event[HANDLE_SIGNIFICANT_MOTION].timestamp = 0;

    if( sensor_data.sensor != HANDLE_LIGHT ||
        (memcmp( &sensor_data,
                 &g_sensor_control->last_event[sensor_data.sensor],
                 sizeof(sensor_data) ) != 0) )
    {
      if( hal_insert_queue( &sensor_data ) )
      {
        hal_signal_ind( &g_sensor_control->data_arrived_cond );
      }
    }
  }
  else if( smgr_data->ItemQuality == SNS_SMGR_ITEM_QUALITY_INVALID_NOT_READY_V01 )
  {
    HAL_LOG_DEBUG( "%s: Received invalid/not ready sample for sensor ID %i",
                   __FUNCTION__, smgr_data->SensorId );
  }
  else
  {
    HAL_LOG_ERROR("%s: Bad item quality: %u ", __FUNCTION__, smgr_data->ItemQuality );
  }
}

 /*===========================================================================

  FUNCTION:  hal_process_buffering_ind

===========================================================================*/
/*!
*/
void
hal_process_buffering_ind( sns_smgr_buffering_ind_msg_v01* smgr_ind )
{
  sns_smgr_periodic_report_ind_msg_v01 report_msg;
  uint32_t i,j;
  uint_fast16_t max_reports_per_index = 0;
  /* This array keeps track of the current index to the item requested
   * in the buffering request */
  uint32_t ts_offset[ SNS_SMGR_BUFFERING_REQUEST_MAX_ITEMS_V01 ] = {0,};

  pthread_mutex_lock( &g_sensor_control->data_mutex );

  for( i = 0; i < smgr_ind->Indices_len; i++ )
  {
    max_reports_per_index = MAX( max_reports_per_index,
                                 smgr_ind->Indices[i].SampleCount );
  }
  HAL_LOG_DEBUG("%s: Samples_len=%d Items=%d max_reports_per_index=%d",
               __FUNCTION__,  smgr_ind->Samples_len, smgr_ind->Indices_len,
               max_reports_per_index );

  for( i = 0; i < max_reports_per_index; i++ )
  {
    report_msg.ReportId = smgr_ind->ReportId;
    report_msg.status = SNS_SMGR_REPORT_OK_V01;
    /* The CurrentRate isn't used for much -- just a debug printout.
     * This conversion may be wrong if there's more than one sampling rate,
     * but the only side effect is an incorrect debug print */
    report_msg.CurrentRate = smgr_ind->Indices[0].SamplingRate;
    report_msg.Item_len = 0;
    for( j = 0; j < smgr_ind->Indices_len; j++ )
    {
      if( i < smgr_ind->Indices[j].SampleCount )
      {
        report_msg.Item_len++;
        report_msg.Item[j].SensorId = smgr_ind->Indices[j].SensorId;
        report_msg.Item[j].DataType = smgr_ind->Indices[j].DataType;
        report_msg.Item[j].ItemData[0] = smgr_ind->Samples[smgr_ind->Indices[j].FirstSampleIdx+i].Data[0];
        report_msg.Item[j].ItemData[1] = smgr_ind->Samples[smgr_ind->Indices[j].FirstSampleIdx+i].Data[1];
        report_msg.Item[j].ItemData[2] = smgr_ind->Samples[smgr_ind->Indices[j].FirstSampleIdx+i].Data[2];
        ts_offset[j] += smgr_ind->Samples[smgr_ind->Indices[j].FirstSampleIdx+i].TimeStampOffset;
        report_msg.Item[j].TimeStamp = ( smgr_ind->Indices[j].FirstSampleTimestamp +
                                         ts_offset[j] );
        report_msg.Item[j].ItemFlags = smgr_ind->Samples[smgr_ind->Indices[j].FirstSampleIdx+i].Flags;
        report_msg.Item[j].ItemQuality = smgr_ind->Samples[smgr_ind->Indices[j].FirstSampleIdx+i].Quality;
        report_msg.Item[j].ItemSensitivity = 0;
      }
    }
    hal_process_report_ind( &report_msg );
  }

  pthread_mutex_unlock( &g_sensor_control->data_mutex );
}

/*===========================================================================

  FUNCTION:  hal_process_smgr_resp

===========================================================================*/
/*!
*/
static void
hal_process_smgr_resp( sensor1_msg_header_s *msg_hdr,
                       void *msg_ptr )
{
  HAL_LOG_DEBUG( "%s: %d", __FUNCTION__,  msg_hdr->msg_id );

  switch( msg_hdr->msg_id )
  {
    case SNS_SMGR_ALL_SENSOR_INFO_RESP_V01:
      pthread_mutex_lock( &g_sensor_control->cb_mutex );
      hal_process_all_sensor_info_resp(
        (sns_smgr_all_sensor_info_resp_msg_v01*) msg_ptr );
      pthread_mutex_unlock( &g_sensor_control->cb_mutex );
      break;

    case SNS_SMGR_SINGLE_SENSOR_INFO_RESP_V01:
      pthread_mutex_lock( &g_sensor_control->cb_mutex );
      hal_process_single_sensor_info_resp(
        msg_hdr->txn_id,
        (sns_smgr_single_sensor_info_resp_msg_v01*) msg_ptr );
      pthread_mutex_unlock( &g_sensor_control->cb_mutex );
      break;

    case SNS_SMGR_BUFFERING_RESP_V01:
      hal_process_buffering_resp( (sns_smgr_buffering_resp_msg_v01*) msg_ptr, msg_hdr );
      break;
  }
}


/*===========================================================================

  FUNCTION:  hal_process_smgr_ind

===========================================================================*/
/*!
*/
static void
hal_process_smgr_ind( sensor1_msg_header_s *msg_hdr,
                      void *msg_ptr )
{
  switch( msg_hdr->msg_id )
  {
    case SNS_SMGR_BUFFERING_IND_V01:
      hal_process_buffering_ind( (sns_smgr_buffering_ind_msg_v01*) msg_ptr );
      break;
    case SNS_SMGR_SENSOR_POWER_STATUS_IND_V01:
      break;
    default:
      break;
  }
}



/*===========================================================================

  FUNCTION:  hal_sam_get_gravity_vector_service_batch_size

===========================================================================*/
/*!
*/
bool hal_sam_get_gravity_vector_service_batch_size( void )
{
  sensor1_error_e                    error;
  sensor1_msg_header_s               req_hdr;
  sns_sam_gravity_batch_req_msg_v01 *batch_req;

  req_hdr.service_number = SNS_SAM_GRAVITY_VECTOR_SVC_ID_V01;
  req_hdr.msg_id = SNS_SAM_GRAVITY_BATCH_REQ_V01;
  req_hdr.msg_size = sizeof( *batch_req );
  req_hdr.txn_id = 0;

  error = sensor1_alloc_msg_buf( g_sensor_control->hndl,
                                 sizeof( *batch_req ),
                                 (void**)&batch_req );
  if( SENSOR1_SUCCESS != error )
  {
    HAL_LOG_ERROR("%s: sensor1_alloc_msg_buf() error: %d", __FUNCTION__, error );
    return (false);
  }

  batch_req->instance_id = 0;
  batch_req->batch_period = 0;
  batch_req->req_type_valid = true;
  batch_req->req_type = 2; /* Get buffer depth */

  if( (error = sensor1_write( g_sensor_control->hndl, &req_hdr,
                              batch_req )) != SENSOR1_SUCCESS )
  {
    sensor1_free_msg_buf( g_sensor_control->hndl, batch_req );
    return false;
  }

  /* The response handler will set the global variable for batch size */
  if (hal_wait_for_response( TIME_OUT_MS,
                             &g_sensor_control->cb_mutex,
                             &g_sensor_control->cb_arrived_cond,
                             &g_sensor_control->is_resp_arrived ) == false )
  {
    HAL_LOG_ERROR( "%s: ERROR: No response from request %d", __FUNCTION__,
                   SNS_SMGR_REPORT_ACTION_ADD_V01 );
    return (false);
  }

  HAL_LOG_DEBUG( "%s: Received Response: %d", __FUNCTION__, g_sensor_control->error );

  /* received response */
  if( g_sensor_control->error == true )
  {
    return (false);
  }

  return true;
}

/*===========================================================================

  FUNCTION:  hal_sam_get_rotation_vector_service_batch_size

===========================================================================*/
/*!
*/
bool hal_sam_get_rotation_vector_service_batch_size( void )
{
  sensor1_error_e                            error;
  sensor1_msg_header_s                       req_hdr;
  sns_sam_rotation_vector_batch_req_msg_v01 *batch_req;

  req_hdr.service_number = SNS_SAM_ROTATION_VECTOR_SVC_ID_V01;
  req_hdr.msg_id = SNS_SAM_ROTATION_VECTOR_BATCH_REQ_V01;
  req_hdr.msg_size = sizeof( *batch_req );
  req_hdr.txn_id = 0;

  error = sensor1_alloc_msg_buf( g_sensor_control->hndl,
                                 sizeof( *batch_req ),
                                 (void**)&batch_req );
  if( SENSOR1_SUCCESS != error )
  {
    HAL_LOG_ERROR("%s: sensor1_alloc_msg_buf() error: %d", __FUNCTION__, error );
    return (false);
  }

  batch_req->instance_id = 0;
  batch_req->batch_period = 0;
  batch_req->req_type_valid = true;
  batch_req->req_type = 2; /* Get buffer depth */

  if( (error = sensor1_write( g_sensor_control->hndl, &req_hdr,
                              batch_req )) != SENSOR1_SUCCESS )
  {
    sensor1_free_msg_buf( g_sensor_control->hndl, batch_req );
    return false;
  }

  /* The response handler will set the global variable for batch size */
  if (hal_wait_for_response( TIME_OUT_MS,
                             &g_sensor_control->cb_mutex,
                             &g_sensor_control->cb_arrived_cond,
                             &g_sensor_control->is_resp_arrived ) == false )
  {
    HAL_LOG_ERROR( "%s: ERROR: No response from request %d", __FUNCTION__,
                   SNS_SMGR_REPORT_ACTION_ADD_V01 );
    return (false);
  }

  HAL_LOG_DEBUG( "%s: Received Response: %d", __FUNCTION__, g_sensor_control->error );

  /* received response */
  if( g_sensor_control->error == true )
  {
    return (false);
  }

  return true;
}

/*===========================================================================

  FUNCTION:  hal_sam_get_game_rotation_vector_service_batch_size

===========================================================================*/
/*!
*/
bool hal_sam_get_game_rotation_vector_service_batch_size( void )
{
  sensor1_error_e                            error;
  sensor1_msg_header_s                       req_hdr;
  sns_sam_game_rotation_vector_batch_req_msg_v01 *batch_req;

  req_hdr.service_number = SNS_SAM_GAME_ROTATION_VECTOR_SVC_ID_V01;
  req_hdr.msg_id = SNS_SAM_GAME_ROTATION_VECTOR_BATCH_REQ_V01;
  req_hdr.msg_size = sizeof( *batch_req );
  req_hdr.txn_id = 0;

  error = sensor1_alloc_msg_buf( g_sensor_control->hndl,
                                 sizeof( *batch_req ),
                                 (void**)&batch_req );
  if( SENSOR1_SUCCESS != error )
  {
    HAL_LOG_ERROR("%s: sensor1_alloc_msg_buf() error: %d", __FUNCTION__, error );
    return (false);
  }

  batch_req->instance_id = 0;
  batch_req->batch_period = 0;
  batch_req->req_type_valid = true;
  batch_req->req_type = 2; /* Get buffer depth */

  if( (error = sensor1_write( g_sensor_control->hndl, &req_hdr,
                              batch_req )) != SENSOR1_SUCCESS )
  {
    sensor1_free_msg_buf( g_sensor_control->hndl, batch_req );
    return false;
  }

  /* The response handler will set the global variable for batch size */
  if (hal_wait_for_response( TIME_OUT_MS,
                             &g_sensor_control->cb_mutex,
                             &g_sensor_control->cb_arrived_cond,
                             &g_sensor_control->is_resp_arrived ) == false )
  {
    HAL_LOG_ERROR( "%s: ERROR: No response from request %d", __FUNCTION__,
                   SNS_SMGR_REPORT_ACTION_ADD_V01 );
    return (false);
  }

  HAL_LOG_DEBUG( "%s: Received Response: %d", __FUNCTION__, g_sensor_control->error );

  /* received response */
  if( g_sensor_control->error == true )
  {
    return (false);
  }

  return true;
}

/*===========================================================================

  FUNCTION:  hal_sam_get_orientation_service_batch_size

===========================================================================*/
/*!
*/
bool hal_sam_get_orientation_service_batch_size( void )
{
  sensor1_error_e                        error;
  sensor1_msg_header_s                   req_hdr;
  sns_sam_orientation_batch_req_msg_v01 *batch_req;

  req_hdr.service_number = SNS_SAM_ORIENTATION_SVC_ID_V01;
  req_hdr.msg_id = SNS_SAM_ORIENTATION_BATCH_REQ_V01;
  req_hdr.msg_size = sizeof( *batch_req );
  req_hdr.txn_id = 0;

  error = sensor1_alloc_msg_buf( g_sensor_control->hndl,
                                 sizeof( *batch_req ),
                                 (void**)&batch_req );
  if( SENSOR1_SUCCESS != error )
  {
    HAL_LOG_ERROR("%s: sensor1_alloc_msg_buf() error: %d", __FUNCTION__, error );
    return (false);
  }

  batch_req->instance_id = 0;
  batch_req->batch_period = 0;
  batch_req->req_type_valid = true;
  batch_req->req_type = 2; /* Get buffer depth */

  if( (error = sensor1_write( g_sensor_control->hndl, &req_hdr,
                              batch_req )) != SENSOR1_SUCCESS )
  {
    sensor1_free_msg_buf( g_sensor_control->hndl, batch_req );
    return false;
  }

  /* The response handler will set the global variable for batch size */
  if (hal_wait_for_response( TIME_OUT_MS,
                             &g_sensor_control->cb_mutex,
                             &g_sensor_control->cb_arrived_cond,
                             &g_sensor_control->is_resp_arrived ) == false )
  {
    HAL_LOG_ERROR( "%s: ERROR: No response from request %d", __FUNCTION__,
                   SNS_SMGR_REPORT_ACTION_ADD_V01 );
    return (false);
  }

  HAL_LOG_DEBUG( "%s: Received Response: %d", __FUNCTION__, g_sensor_control->error );

  /* received response */
  if( g_sensor_control->error == true )
  {
    return (false);
  }

  return true;
}

/*===========================================================================

  FUNCTION:  hal_sam_get_step_detector_service_batch_size

===========================================================================*/
/*!
*/
bool hal_sam_get_step_detector_service_batch_size( void )
{
  sensor1_error_e                error;
  sensor1_msg_header_s           req_hdr;
  sns_sam_ped_batch_req_msg_v01* batch_req;

  req_hdr.service_number = SNS_SAM_PED_SVC_ID_V01;
  req_hdr.msg_id = SNS_SAM_PED_BATCH_REQ_V01;
  req_hdr.msg_size = sizeof( *batch_req );
  req_hdr.txn_id = 0;

  error = sensor1_alloc_msg_buf( g_sensor_control->hndl,
                                 sizeof( *batch_req ),
                                 (void**)&batch_req );
  if( SENSOR1_SUCCESS != error )
  {
    HAL_LOG_ERROR("%s: sensor1_alloc_msg_buf() error: %d", __FUNCTION__, error );
    return (false);
  }

  batch_req->instance_id = 0;
  batch_req->batch_period = 0;
  batch_req->req_type_valid = true;
  batch_req->req_type = 2; /* Get buffer depth */

  if( (error = sensor1_write( g_sensor_control->hndl, &req_hdr,
                              batch_req )) != SENSOR1_SUCCESS )
  {
    sensor1_free_msg_buf( g_sensor_control->hndl, batch_req );
    return false;
  }

  /* The response handler will set the global variable for batch size */
  if (hal_wait_for_response( TIME_OUT_MS,
                             &g_sensor_control->cb_mutex,
                             &g_sensor_control->cb_arrived_cond,
                             &g_sensor_control->is_resp_arrived ) == false )
  {
    HAL_LOG_ERROR( "%s: ERROR: No response from request", __FUNCTION__ );
    return (false);
  }

  HAL_LOG_DEBUG( "%s: Received Response: %d", __FUNCTION__, g_sensor_control->error );

  /* received response */
  if( g_sensor_control->error == true )
  {
    return (false);
  }

  return true;
}

/*===========================================================================

  FUNCTION:  hal_sam_add_sensor_fusion_sensors
  Return true if the device has gyro and can support 9-axis sensor fusion
  Otherwise returns false and HAL will use the default 6-axis Google implementation

===========================================================================*/
/*!
*/
void hal_sam_add_sensor_fusion_sensors( void )
{
    hal_sensor_info_t*    sensor_info;
    char                  max_rate_prop_value[PROPERTY_VALUE_MAX];
    int                   max_rate;
    char*                 strtol_endptr;

    /* Add - Gravity */
    sensor_info = &g_sensor_control->sensor_list[ HANDLE_GRAVITY ];
    strlcpy( sensor_info->name, "Gravity", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
    strlcpy( sensor_info->vendor, "Qualcomm", SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
    sensor_info->version = 1;
    sensor_info->handle = HANDLE_GRAVITY;
    sensor_info->type = SENSOR_TYPE_GRAVITY; /* Need to match sensors.h */
    sensor_info->max_range = g_sensor_control->sensor_list[ HANDLE_ACCELERATION ].max_range;
    sensor_info->resolution = g_sensor_control->sensor_list[ HANDLE_ACCELERATION ].resolution;
    if( hal_is_gyro_available() )
    {
      hal_sam_get_gravity_vector_service_batch_size();
    }

    memset(max_rate_prop_value, 0, PROPERTY_VALUE_MAX);
    if( hal_is_gyro_available() )
    {
      // power: accel + gyro
      sensor_info->power = g_sensor_control->sensor_list[ HANDLE_ACCELERATION ].power +
                         g_sensor_control->sensor_list[ HANDLE_GYRO ].power;

      // max freq
      property_get( HAL_PROP_MAX_GRAV, max_rate_prop_value,
                    FREQ_GRAV_DEF_HZ_STR );
      sensor_info->max_freq = strtol( max_rate_prop_value, &strtol_endptr, 0 );
    }
    else
    {
      // power = accel
      sensor_info->power = g_sensor_control->sensor_list[HANDLE_ACCELERATION].power;

      // max freq
      property_get( HAL_PROP_MAX_GRAV_NO_GYRO, max_rate_prop_value,
                    FREQ_GRAV_NO_GYRO_DEF_HZ_STR );
      sensor_info->max_freq = strtol( max_rate_prop_value, &strtol_endptr, 0 );
    }
    // min freq
    sensor_info->min_freq = MIN_FREQ_QFUSION_HZ;

    sensor_info->is_attrib_ok = true;

    /* Add - Linear Accel */
    sensor_info = &g_sensor_control->sensor_list[ HANDLE_LINEAR_ACCEL ];
    strlcpy( sensor_info->name, "Linear Acceleration", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
    strlcpy( sensor_info->vendor, "Qualcomm", SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
    sensor_info->version = 1;
    sensor_info->handle = HANDLE_LINEAR_ACCEL;
    sensor_info->type = SENSOR_TYPE_LINEAR_ACCELERATION; /* Need to match sensors.h */
    sensor_info->max_range = g_sensor_control->sensor_list[ HANDLE_ACCELERATION ].max_range;
    sensor_info->resolution = g_sensor_control->sensor_list[ HANDLE_ACCELERATION ].resolution;

    memset(max_rate_prop_value, 0, PROPERTY_VALUE_MAX);
    if (hal_is_gyro_available())
    {
      // power: Linear Accel = accel + gyro
      sensor_info->power = g_sensor_control->sensor_list[ HANDLE_ACCELERATION ].power +
                           g_sensor_control->sensor_list[ HANDLE_GYRO ].power;

      // max freq - same as Gravity
      property_get( HAL_PROP_MAX_GRAV, max_rate_prop_value,
                    FREQ_GRAV_DEF_HZ_STR );
      sensor_info->max_freq = strtol( max_rate_prop_value, &strtol_endptr, 0 );
    }
    else
    {
      // power = accel
      sensor_info->power = g_sensor_control->sensor_list[HANDLE_ACCELERATION].power;

      // max freq - same as Gravity
      property_get( HAL_PROP_MAX_GRAV_NO_GYRO, max_rate_prop_value,
                    FREQ_GRAV_NO_GYRO_DEF_HZ_STR );
      sensor_info->max_freq = strtol( max_rate_prop_value, &strtol_endptr, 0 );
    }
    // min freq
    sensor_info->min_freq = MIN_FREQ_QFUSION_HZ;

    sensor_info->is_attrib_ok = true;

    if (hal_is_mag_available())
    {
      /* Add - Rotation Vector */
      sensor_info = &g_sensor_control->sensor_list[ HANDLE_ROTATION_VECTOR];
      strlcpy( sensor_info->name, "Rotation Vector", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
      strlcpy( sensor_info->vendor, "Qualcomm", SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
      sensor_info->version = 1;
      sensor_info->handle = HANDLE_ROTATION_VECTOR;
      sensor_info->type = SENSOR_TYPE_ROTATION_VECTOR; /* Need to match sensors.h */
      sensor_info->max_range = 1;
      sensor_info->resolution = 1 / pow(2, 24);

      if( hal_is_gyro_available() )
      {
        hal_sam_get_rotation_vector_service_batch_size();
      }

      memset(max_rate_prop_value, 0, PROPERTY_VALUE_MAX);
      if( hal_is_gyro_available() )
      {
        // power: rot vector = accel + mag + gyro
        sensor_info->power = g_sensor_control->sensor_list[ HANDLE_GRAVITY ].power +
                             g_sensor_control->sensor_list[ HANDLE_MAGNETIC_FIELD ].power;

        // max freq
        property_get( HAL_PROP_MAX_ROTVEC, max_rate_prop_value,
                      FREQ_ROTVEC_DEF_HZ_STR );
        sensor_info->max_freq = strtol( max_rate_prop_value, &strtol_endptr, 0 );
      }
      else
      {
        // power = accel + mag
        sensor_info->power = g_sensor_control->sensor_list[HANDLE_ACCELERATION].power +
                             g_sensor_control->sensor_list[ HANDLE_MAGNETIC_FIELD ].power;

        // max freq - taken from Geomagnetic Rotation Vector
        property_get( HAL_PROP_MAX_GEOMAG_ROTVEC, max_rate_prop_value,
                      FREQ_GEOMAG_ROTVEC_DEF_HZ_STR );
        sensor_info->max_freq = strtol( max_rate_prop_value, &strtol_endptr, 0 );
      }
      // min freq
      sensor_info->min_freq = MIN_FREQ_QFUSION_HZ;

      sensor_info->is_attrib_ok = true;
    }
    else
    {
      HAL_LOG_DEBUG("%s: Rotation Vector disabled!", __FUNCTION__);
    }

    /* Add - Game Rotation Vector */
    char grv_enabled[PROPERTY_VALUE_MAX] = "false";

    property_get(HAL_PROP_GRV, grv_enabled, "true");
    if( !strncmp("true", grv_enabled, 4) && hal_is_gyro_available() )
    {
      HAL_LOG_DEBUG("%s: GameRV enabled", __FUNCTION__);

      sensor_info = &g_sensor_control->sensor_list[ HANDLE_GAME_ROTATION_VECTOR];
      strlcpy( sensor_info->name, "Game Rotation Vector", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
      strlcpy( sensor_info->vendor, "Qualcomm", SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
      sensor_info->version = 1;
      sensor_info->handle = HANDLE_GAME_ROTATION_VECTOR;
      sensor_info->type = SENSOR_TYPE_GAME_ROTATION_VECTOR;
      sensor_info->max_range = 1;
      sensor_info->resolution = 1 / pow(2, 24);

      // power: rot vector = accel + gyro
      sensor_info->power =  g_sensor_control->sensor_list[ HANDLE_ACCELERATION ].power +
                            g_sensor_control->sensor_list[ HANDLE_GYRO ].power;
      // max/min freq
      sensor_info->max_freq = g_sensor_control->sensor_list[ HANDLE_GYRO ].max_freq;
      property_get( HAL_PROP_MAX_GAMERV, max_rate_prop_value,
                    FREQ_GAMERV_DEF_HZ_STR );
      sensor_info->max_freq = strtol( max_rate_prop_value, &strtol_endptr, 0 );
      sensor_info->min_freq = MIN_FREQ_QFUSION_HZ;

      hal_sam_get_game_rotation_vector_service_batch_size();

      sensor_info->is_attrib_ok = true;
    }
    else
    {
      HAL_LOG_DEBUG("%s: GameRV disabled!", __FUNCTION__);
    }

    if (hal_is_mag_available())
    {
      /* Add - Orientation sensor */
      HAL_LOG_DEBUG("%s: Adding Orientation", __FUNCTION__);

      sensor_info = &g_sensor_control->sensor_list[ HANDLE_ORIENTATION];
      strlcpy( sensor_info->name, "Orientation", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
      strlcpy( sensor_info->vendor, "Qualcomm", SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
      sensor_info->version = 1;
      sensor_info->handle = HANDLE_ORIENTATION;
      sensor_info->type = SENSOR_TYPE_ORIENTATION; /* Need to match sensors.h */
      sensor_info->max_range = 360.0; // unit: degree
      sensor_info->resolution = 0.1;  // unit: degree
      hal_sam_get_orientation_service_batch_size();

      memset(max_rate_prop_value, 0, PROPERTY_VALUE_MAX);
      if (hal_is_gyro_available())
      {
        // power = accel + mag + gyro
        sensor_info->power = g_sensor_control->sensor_list[ HANDLE_GRAVITY ].power +
                             g_sensor_control->sensor_list[ HANDLE_MAGNETIC_FIELD ].power;

        // max freq
        property_get( HAL_PROP_MAX_ROTVEC, max_rate_prop_value,
                      FREQ_ORIENT_DEF_HZ_STR );
        sensor_info->max_freq = strtol( max_rate_prop_value, &strtol_endptr, 0 );
      }
      else
      {
        // power = accel + mag
        sensor_info->power = g_sensor_control->sensor_list[HANDLE_ACCELERATION].power +
                             g_sensor_control->sensor_list[ HANDLE_MAGNETIC_FIELD ].power;

        // max freq - taken from Geomagnetic Rotation Vector
        property_get( HAL_PROP_MAX_GEOMAG_ROTVEC, max_rate_prop_value,
                      FREQ_GEOMAG_ROTVEC_DEF_HZ_STR );
        sensor_info->max_freq = strtol( max_rate_prop_value, &strtol_endptr, 0 );
      }
      // min freq
      sensor_info->min_freq = MIN_FREQ_QFUSION_HZ;

      sensor_info->is_attrib_ok = true;
    }
    else
    {
      HAL_LOG_DEBUG("%s: Orientation disabled!", __FUNCTION__);
    }
}

/*===========================================================================

  FUNCTION:  hal_sam_add_geomagnetic_sensor

===========================================================================*/
/*!
*/
void hal_sam_add_geomagnetic_sensor( void )
{
  hal_sensor_info_t*    sensor_info;
  char geomagnetic_rv_enabled[PROPERTY_VALUE_MAX] = "false";
  char                  max_rate_prop_value[PROPERTY_VALUE_MAX];
  int                   max_rate;
  char*                 strtol_endptr;

  HAL_LOG_DEBUG("%s:", __FUNCTION__);
  property_get(HAL_PROP_GEOMAGNETIC_RV, geomagnetic_rv_enabled, "true");
  if (!strncmp("true", geomagnetic_rv_enabled, 4) && hal_is_mag_available())
  {
    HAL_LOG_DEBUG("%s: Geomagnetic RV enabled", __FUNCTION__);
  }
  else
  {
    HAL_LOG_DEBUG("%s: Geomagnetic RV disabled!", __FUNCTION__);
    return;
  }

  // max freq
  memset(max_rate_prop_value, 0, PROPERTY_VALUE_MAX);
  property_get( HAL_PROP_MAX_GEOMAG_ROTVEC, max_rate_prop_value, FREQ_GEOMAG_ROTVEC_DEF_HZ_STR );

  sensor_info = &g_sensor_control->sensor_list[ HANDLE_GEOMAGNETIC_ROTATION_VECTOR ];
  strlcpy( sensor_info->name, "Geomagnetic Rotation Vector", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
  strlcpy( sensor_info->vendor, "Qualcomm", SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
  sensor_info->version = 1;
  sensor_info->handle = HANDLE_GEOMAGNETIC_ROTATION_VECTOR;
  sensor_info->type = SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR;
  sensor_info->max_range = 1;
  sensor_info->resolution = 1 / pow(2, 24);
  sensor_info->power = g_sensor_control->sensor_list[HANDLE_ACCELERATION].power +
                       g_sensor_control->sensor_list[HANDLE_MAGNETIC_FIELD].power;
  sensor_info->min_freq = g_sensor_control->sensor_list[ HANDLE_ACCELERATION ].min_freq;
  sensor_info->max_freq = strtol( max_rate_prop_value, &strtol_endptr, 0 );
  hal_sam_get_orientation_service_batch_size();
  sensor_info->is_attrib_ok = true;
}

/*===========================================================================

  FUNCTION:  hal_add_oem_sensors

===========================================================================*/
/*!
*/
void hal_add_oem_sensors( void )
{
  struct sensor_t const *OEMList;
  hal_sensor_info_t*    sensor_info;
  int                   num_OEMSensors = 0;
  HAL_LOG_DEBUG("%s", __FUNCTION__ );

  if( NULL == g_oem_sensor_control.OEMModule )
    return;

  if( (num_OEMSensors = g_oem_sensor_control.OEMModule->get_sensors_list(g_oem_sensor_control.OEMModule, &OEMList)) > 0 )
  {
    int i;
    for (i=0; i < num_OEMSensors; i++)
    {
      if( SENSOR_TYPE_LIGHT == OEMList[i].type )
      {
        HAL_LOG_DEBUG("%s: Got type SENSOR_TYPE_LIGHT", __FUNCTION__);
        sensor_info = &g_sensor_control->sensor_list[ HANDLE_OEM_LIGHT ];
        strlcpy( sensor_info->name, OEMList[i].name, SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
        strlcpy( sensor_info->vendor, OEMList[i].vendor, SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
        sensor_info->version = OEMList[i].version;
        sensor_info->handle = HANDLE_OEM_LIGHT;
        sensor_info->oem_handle = OEMList[i].handle;
        sensor_info->type = SENSOR_TYPE_LIGHT; /* Need to match sensors.h */
        sensor_info->max_range = OEMList[i].maxRange;
        sensor_info->resolution = OEMList[i].resolution;
        sensor_info->power = OEMList[i].power;
        sensor_info->max_freq = (OEMList[i].minDelay == 0)? FREQ_FASTEST_HZ: (1000/(USEC_TO_MSEC(OEMList[i].minDelay)));
        sensor_info->min_freq = 1;
        sensor_info->is_attrib_ok = true;
      }
      else if( SENSOR_TYPE_PROXIMITY == OEMList[i].type )
      {
        HAL_LOG_DEBUG("%s: Got type SENSOR_TYPE_PROXIMITY", __FUNCTION__);
        sensor_info = &g_sensor_control->sensor_list[ HANDLE_OEM_PROXIMITY ];
        strlcpy( sensor_info->name, OEMList[i].name, SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
        strlcpy( sensor_info->vendor, OEMList[i].vendor, SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
        sensor_info->version = OEMList[i].version;
        sensor_info->handle = HANDLE_OEM_PROXIMITY;
        sensor_info->oem_handle = OEMList[i].handle;
        sensor_info->type = SENSOR_TYPE_PROXIMITY; /* Need to match sensors.h */
        sensor_info->max_range = OEMList[i].maxRange;
        sensor_info->resolution = OEMList[i].resolution;
        sensor_info->power = OEMList[i].power;
        sensor_info->max_freq = (OEMList[i].minDelay == 0)? FREQ_FASTEST_HZ: (1000/USEC_TO_MSEC(OEMList[i].minDelay));
        sensor_info->min_freq = 1;
        sensor_info->is_attrib_ok = true;
      }
    }
  }
  HAL_LOG_DEBUG("%s: get_sensors_list returned num_OEMSensors=%d", __FUNCTION__,num_OEMSensors );
}

/*===========================================================================

  FUNCTION:  hal_sam_add_sensors

===========================================================================*/
/*!
*/
void hal_sam_add_sensors( void )
{
  hal_sensor_info_t*    sensor_info;
  hal_sensor_info_t*    related_sensor;

  /* Add calibrated mag through SAM, if we have a mag device and we are
     supporting calibration through the SAM
  */
  if( hal_is_mag_available() &&
      (g_sensor_control->mag_cal_src == HAL_MAG_CAL_SRC_SAM))
  {
    sensor_info = &g_sensor_control->sensor_list[ HANDLE_MAGNETIC_FIELD_SAM ];
    related_sensor = &g_sensor_control->sensor_list[ HANDLE_MAGNETIC_FIELD ];
    strlcpy( sensor_info->name, related_sensor->vendor, SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
    strlcat( sensor_info->name, " Calibration Lib", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
    strlcpy( sensor_info->vendor, related_sensor->vendor, SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
    sensor_info->version = 1;
    sensor_info->handle = HANDLE_MAGNETIC_FIELD_SAM;
    sensor_info->type = SENSOR_TYPE_MAGNETIC_FIELD;
    sensor_info->max_range = related_sensor->max_range;
    sensor_info->resolution = related_sensor->resolution;
    sensor_info->power = related_sensor->power;
    sensor_info->max_freq = related_sensor->max_freq;
    sensor_info->min_freq = related_sensor->min_freq;
    sensor_info->is_attrib_ok = true;
    sensor_info->max_buffered_samples = 0;
  }

#ifdef FEATURE_SNS_HAL_SAM_INT
  char fns_enabled[PROPERTY_VALUE_MAX] = "false";
  property_get( HAL_PROP_FNS, fns_enabled, "false" );
  if( !strncmp( "true", fns_enabled, 4 ) )
  {
    sensor_info = &g_sensor_control->sensor_list[ HANDLE_GESTURE_FACE_N_SHAKE ];
    strlcpy( sensor_info->name, "Face-N-Shake", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
    strlcpy( sensor_info->vendor, "Qualcomm", SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
    sensor_info->version = 1;
    sensor_info->handle = HANDLE_GESTURE_FACE_N_SHAKE;
    sensor_info->type = SENSOR_TYPE_GESTURE_FACE_N_SHAKE;
    sensor_info->max_range = 1;
    sensor_info->resolution = 1;
    sensor_info->power = 1;
    sensor_info->max_freq = 100;
    sensor_info->min_freq = 1;
    sensor_info->is_attrib_ok = true;
    sensor_info->max_buffered_samples = 0;
  }

  char bte_enabled[PROPERTY_VALUE_MAX] = "false";
  property_get( HAL_PROP_BTE, bte_enabled, "false" );
  if( !strncmp( "true", bte_enabled, 4 ) )
  {
    sensor_info = &g_sensor_control->sensor_list[ HANDLE_GESTURE_BRING_TO_EAR ];
    strlcpy( sensor_info->name, "Bring-to-Ear", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
    strlcpy( sensor_info->vendor, "Qualcomm", SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
    sensor_info->version = 1;
    sensor_info->handle = HANDLE_GESTURE_BRING_TO_EAR;
    sensor_info->type = SENSOR_TYPE_GESTURE_BRING_TO_EAR;
    sensor_info->max_range = 1;
    sensor_info->resolution = 1;
    sensor_info->power = 1;
    sensor_info->max_freq = 100;
    sensor_info->min_freq = 1;
    sensor_info->is_attrib_ok = true;
    sensor_info->max_buffered_samples = 0;
  }
#endif /* FEATURE_SNS_HAL_SAM_INT */

  char cmc_enabled[PROPERTY_VALUE_MAX] = "false";

  property_get(HAL_PROP_CMC, cmc_enabled, "false");
  if (!strncmp("true", cmc_enabled, 4))
  {
    HAL_LOG_DEBUG("%s: CMC enabled", __FUNCTION__);

    /* Add cmc */
    sensor_info = &g_sensor_control->sensor_list[ HANDLE_CMC ];
    strlcpy( sensor_info->name, "Coarse Motion Classifier", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
    strlcpy( sensor_info->vendor, "Qualcomm", SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
    sensor_info->version = 1;
    sensor_info->handle = HANDLE_CMC;
    sensor_info->type = SENSOR_TYPE_CMC;
    sensor_info->max_range = SNS_SAM_CMC_MS_E_MAX_ENUM_VAL_V01; //range of enum values
    sensor_info->resolution = 1;
    sensor_info->power = g_sensor_control->sensor_list[HANDLE_ACCELERATION].power;
    sensor_info->max_freq = 0;
    sensor_info->min_freq = 1;
    sensor_info->is_attrib_ok = true;
    sensor_info->max_buffered_samples = 0;
  }
  else
  {
    HAL_LOG_DEBUG("%s: CMC disabled!", __FUNCTION__);
  }

  char ped_enabled[PROPERTY_VALUE_MAX] = "false";

  property_get(HAL_PROP_PEDOMETER, ped_enabled, "true");
  if (!strncmp("true", ped_enabled, 4))
  {
    HAL_LOG_DEBUG("%s: Pedometer enabled", __FUNCTION__);

    /* Add pedometer */
    sensor_info = &g_sensor_control->sensor_list[ HANDLE_PEDOMETER ];
    strlcpy( sensor_info->name, "Pedometer", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
    strlcpy( sensor_info->vendor, "Qualcomm", SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
    sensor_info->version = 1;
    sensor_info->handle = HANDLE_PEDOMETER;
    sensor_info->type = SENSOR_TYPE_PEDOMETER;
    sensor_info->max_range = 1;
    sensor_info->resolution = 1;
    sensor_info->power = g_sensor_control->sensor_list[HANDLE_ACCELERATION].power;
    sensor_info->max_freq = 0;
    sensor_info->min_freq = 1;
    sensor_info->is_attrib_ok = true;
    sensor_info->max_buffered_samples = 0;
  }
  else
  {
    HAL_LOG_DEBUG("%s: Pedometer disabled!", __FUNCTION__);
  }

  char pam_enabled[PROPERTY_VALUE_MAX] = "false";

  property_get(HAL_PROP_PAM, pam_enabled, "true");
  if (!strncmp("true", pam_enabled, 4))
  {
    HAL_LOG_DEBUG("%s: PAM enabled", __FUNCTION__);

    /* Add pam */
    sensor_info = &g_sensor_control->sensor_list[ HANDLE_PAM ];
    strlcpy( sensor_info->name, "PEDESTRIAN-ACTIVITY-MONITOR", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
    strlcpy( sensor_info->vendor, "Qualcomm", SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
    sensor_info->version = 1;
    sensor_info->handle = HANDLE_PAM;
    sensor_info->type = SENSOR_TYPE_PAM;
    sensor_info->max_range = 65535;
    sensor_info->resolution = 1;
    sensor_info->power = g_sensor_control->sensor_list[ HANDLE_ACCELERATION ].power;
    sensor_info->max_freq = 0;
    sensor_info->min_freq = 0;
    sensor_info->is_attrib_ok = true;
    sensor_info->max_buffered_samples = 0;
  }
  else
  {
    HAL_LOG_DEBUG("%s: PAM disabled!", __FUNCTION__);
  }

#ifdef FEATURE_SNS_HAL_SAM_OTN
  char scrn_ortn_enabled[PROPERTY_VALUE_MAX] = "false";

  property_get(HAL_PROP_SCRN_ORTN, scrn_ortn_enabled, "true");
  if(!strncmp("true", scrn_ortn_enabled, 4))
  {
    HAL_LOG_DEBUG("%s: Screen Orientation enabled", __FUNCTION__);
    sensor_info = &g_sensor_control->sensor_list[ HANDLE_MOTION_ACCEL ];
    strlcpy( sensor_info->name, "Motion Accel", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
    strlcpy( sensor_info->vendor, "Qualcomm", SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
    sensor_info->version = 1;
    sensor_info->handle = HANDLE_MOTION_ACCEL;
    sensor_info->type = SENSOR_TYPE_SCREEN_ORIENTATION;
    sensor_info->max_range = g_sensor_control->sensor_list[ HANDLE_ACCELERATION ].max_range; // range of enum values
    sensor_info->resolution = g_sensor_control->sensor_list[ HANDLE_ACCELERATION ].resolution;
    sensor_info->power = g_sensor_control->sensor_list[ HANDLE_ACCELERATION ].power;
    sensor_info->max_freq = g_sensor_control->sensor_list[ HANDLE_ACCELERATION ].max_freq;
    sensor_info->min_freq = g_sensor_control->sensor_list[ HANDLE_ACCELERATION ].min_freq;
    sensor_info->is_attrib_ok = true;
    sensor_info->max_buffered_samples = 0;
  }
  else
  {
    HAL_LOG_DEBUG("%s: Screen Orientation disabled", __FUNCTION__);
  }
#endif /* FEATURE_SNS_HAL_SAM_OTN */

}

/*===========================================================================

  FUNCTION:  hal_sam_add_qmd_sensors

===========================================================================*/
/*!
*/
void hal_sam_add_qmd_sensors( void )
{
  hal_sensor_info_t*    sensor_info;

  char qmd_enabled[PROPERTY_VALUE_MAX] = "false";
  property_get( HAL_PROP_QMD, qmd_enabled, "true" );
  if( !strncmp( "true", qmd_enabled, 4 ) )
  {
    HAL_LOG_DEBUG("%s: QMD enabled", __FUNCTION__);
  }
  else
  {
    HAL_LOG_DEBUG("%s: QMD disabled!", __FUNCTION__);
    return ;
  }

  /* Add QMD - AMD */
  sensor_info = &g_sensor_control->sensor_list[ HANDLE_MOTION_ABSOLUTE ];
  strlcpy( sensor_info->name, "AMD", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
  strlcpy( sensor_info->vendor, "Qualcomm", SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
  sensor_info->version = 1;
  sensor_info->handle = HANDLE_MOTION_ABSOLUTE;
  sensor_info->type = SENSOR_TYPE_MOTION_ABSOLUTE;
  sensor_info->max_range = 1;
  sensor_info->resolution = 1;
  sensor_info->power = 1;
  sensor_info->max_freq = 0;
  sensor_info->min_freq = 0;
  sensor_info->is_attrib_ok = true;
  sensor_info->max_buffered_samples = 0;

  /* Add QMD - RMD */
  sensor_info = &g_sensor_control->sensor_list[ HANDLE_MOTION_RELATIVE ];
  strlcpy( sensor_info->name, "RMD", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
  strlcpy( sensor_info->vendor, "Qualcomm", SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
  sensor_info->version = 1;
  sensor_info->handle = HANDLE_MOTION_RELATIVE;
  sensor_info->type = SENSOR_TYPE_MOTION_RELATIVE;
  sensor_info->max_range = 1;
  sensor_info->resolution = 1;
  sensor_info->power = 1;
  sensor_info->max_freq = 0;
  sensor_info->min_freq = 0;
  sensor_info->is_attrib_ok = true;
  sensor_info->max_buffered_samples = 0;

  char vmd_enabled[PROPERTY_VALUE_MAX] = "false";
  property_get( HAL_PROP_VMD, vmd_enabled, "false" );
  if( !strncmp( "true", vmd_enabled, 4 ) )
  {
    sensor_info = &g_sensor_control->sensor_list[ HANDLE_MOTION_VEHICLE ];
    strlcpy( sensor_info->name, "VMD", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
    strlcpy( sensor_info->vendor, "Qualcomm", SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
    sensor_info->version = 1;
    sensor_info->handle = HANDLE_MOTION_VEHICLE;
    sensor_info->type = SENSOR_TYPE_MOTION_VEHICLE;
    sensor_info->max_range = 1;
    sensor_info->resolution = 1;
    sensor_info->power = 1;
    sensor_info->max_freq = 0;
    sensor_info->min_freq = 0;
    sensor_info->is_attrib_ok = true;
    sensor_info->max_buffered_samples = 0;
  }
}

/*===========================================================================

  FUNCTION:  hal_sam_add_smd_sensor

===========================================================================*/
/*!
*/
void hal_sam_add_smd_sensor( void )
{
  hal_sensor_info_t*    sensor_info;
  char smd_enabled[PROPERTY_VALUE_MAX] = "false";
  HAL_LOG_DEBUG("%s:", __FUNCTION__);

  property_get(HAL_PROP_SMD, smd_enabled, "true");
  if (!strncmp("true", smd_enabled, 4))
  {
    HAL_LOG_DEBUG("%s: SMD enabled", __FUNCTION__);
  }
  else
  {
    HAL_LOG_DEBUG("%s: SMD disabled!", __FUNCTION__);
    return;
  }

  sensor_info = &g_sensor_control->sensor_list[ HANDLE_SIGNIFICANT_MOTION ];
  strlcpy( sensor_info->name, "Significant Motion Detector", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
  strlcpy( sensor_info->vendor, "Qualcomm", SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
  sensor_info->version = 1;
  sensor_info->handle = HANDLE_SIGNIFICANT_MOTION;
  sensor_info->type = SENSOR_TYPE_SIGNIFICANT_MOTION;
  sensor_info->max_range = 1;
  sensor_info->resolution = 1;
  sensor_info->power = g_sensor_control->sensor_list[ HANDLE_ACCELERATION ].power;
  sensor_info->max_freq = 1;
  sensor_info->min_freq = 0;
  sensor_info->is_attrib_ok = true;
  sensor_info->max_buffered_samples = 0;
}

/*===========================================================================

  FUNCTION:  hal_sam_add_step_detector_sensor

===========================================================================*/
/*!
*/
void hal_sam_add_step_detector_sensor( void )
{
  hal_sensor_info_t*    sensor_info;
  char step_detector_enabled[PROPERTY_VALUE_MAX] = "false";
  HAL_LOG_DEBUG("%s:", __FUNCTION__);

  property_get(HAL_PROP_STEP_DETECTOR, step_detector_enabled, "true");
  if (!strncmp("true", step_detector_enabled, 4))
  {
    HAL_LOG_DEBUG("%s: STEP DETECTOR enabled", __FUNCTION__);
  }
  else
  {
    HAL_LOG_DEBUG("%s: STEP DETECTOR disabled!", __FUNCTION__);
    return;
  }

  sensor_info = &g_sensor_control->sensor_list[ HANDLE_SAM_STEP_DETECTOR ];
  strlcpy( sensor_info->name, "Step Detector", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
  strlcpy( sensor_info->vendor, "Qualcomm", SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
  sensor_info->version = 1;
  sensor_info->handle = HANDLE_SAM_STEP_DETECTOR;
  sensor_info->type = SENSOR_TYPE_STEP_DETECTOR;
  sensor_info->max_range = 1;
  sensor_info->resolution = 1;
  sensor_info->power = g_sensor_control->sensor_list[ HANDLE_ACCELERATION ].power;
  sensor_info->max_freq = 0;
  sensor_info->min_freq = 1;
  sensor_info->is_attrib_ok = true;
  hal_sam_get_step_detector_service_batch_size();
}

/*===========================================================================

  FUNCTION:  hal_sam_add_step_counter_sensor

===========================================================================*/
/*!
*/
void hal_sam_add_step_counter_sensor( void )
{
  hal_sensor_info_t*    sensor_info;
  char step_counter_enabled[PROPERTY_VALUE_MAX] = "false";
  HAL_LOG_DEBUG("%s:", __FUNCTION__);

  property_get(HAL_PROP_STEP_COUNTER, step_counter_enabled, "true");
  if (!strncmp("true", step_counter_enabled, 4))
  {
    HAL_LOG_DEBUG("%s: STEP COUNTER enabled", __FUNCTION__);
  }
  else
  {
    HAL_LOG_DEBUG("%s: STEP COUNTER disabled!", __FUNCTION__);
    return;
  }

  sensor_info = &g_sensor_control->sensor_list[ HANDLE_SAM_STEP_COUNTER ];
  strlcpy( sensor_info->name, "Step Counter", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
  strlcpy( sensor_info->vendor, "Qualcomm", SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
  sensor_info->version = 1;
  sensor_info->handle = HANDLE_SAM_STEP_COUNTER;
  sensor_info->type = SENSOR_TYPE_STEP_COUNTER;
  sensor_info->max_range = 1;
  sensor_info->resolution = 1;
  sensor_info->power = g_sensor_control->sensor_list[ HANDLE_ACCELERATION ].power;
  sensor_info->max_freq = 0;
  sensor_info->min_freq = 1;
  sensor_info->is_attrib_ok = true;
  sensor_info->max_buffered_samples = 0;
}

void  hal_sam_add_gestures_sensors(void)
{
  hal_sensor_info_t*    sensor_info;

  char gestures_enabled[PROPERTY_VALUE_MAX] = "false";
  property_get( HAL_PROP_GESTURES,gestures_enabled, "true" );
  if( !strncmp( "true", gestures_enabled, 4) )
  {
    HAL_LOG_DEBUG("%s: gestures enabled", __FUNCTION__);
  }
  else
  {
    HAL_LOG_DEBUG("%s: gestures disabled!", __FUNCTION__);
    return;
  }

  sensor_info = &g_sensor_control->sensor_list[ HANDLE_GESTURE_BASIC_GESTURES ];
  strlcpy( sensor_info->name, "Basic Gestures", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
  strlcpy( sensor_info->vendor, "Qualcomm", SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
  sensor_info->version = 1;
  sensor_info->handle = HANDLE_GESTURE_BASIC_GESTURES;
  sensor_info->type = SENSOR_TYPE_BASIC_GESTURES;
  sensor_info->max_range = 7; // range of enum values
  sensor_info->resolution = 1;
  sensor_info->power = g_sensor_control->sensor_list[ HANDLE_ACCELERATION ].power;
  sensor_info->max_freq = 100;
  sensor_info->min_freq = 1;
  sensor_info->is_attrib_ok = true;
  sensor_info->max_buffered_samples = 0;

#ifdef FEATURE_SNS_HAL_SAM_TAP
  sensor_info = &g_sensor_control->sensor_list[ HANDLE_GESTURE_TAP ];
  strlcpy( sensor_info->name, "Tap", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
  strlcpy( sensor_info->vendor, "Qualcomm", SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
  sensor_info->version = 1;
  sensor_info->handle = HANDLE_GESTURE_TAP;
  sensor_info->type = SENSOR_TYPE_TAP;
  sensor_info->max_range = 6; // range of enum values
  sensor_info->resolution = 1;
  sensor_info->power = g_sensor_control->sensor_list[ HANDLE_ACCELERATION ].power;
  sensor_info->max_freq = 200;
  sensor_info->min_freq = 1;
  sensor_info->is_attrib_ok = true;
  sensor_info->max_buffered_samples = 0;
#endif /* FEATURE_SNS_HAL_SAM_TAP */

  char gtap_enabled[PROPERTY_VALUE_MAX] = "false";
  property_get( HAL_PROP_GTAP, gtap_enabled, "false" );
  if( !strncmp( "true", gtap_enabled, 4 ) )
  {
    sensor_info = &g_sensor_control->sensor_list[ HANDLE_GESTURE_GYRO_TAP ];
    strlcpy( sensor_info->name, "Gyro Tap", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
    strlcpy( sensor_info->vendor, "Qualcomm", SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
    sensor_info->version = 1;
    sensor_info->handle = HANDLE_GESTURE_GYRO_TAP;
    sensor_info->type = SENSOR_TYPE_TAP;
    sensor_info->max_range = 6; // Enum, 1-6 denote tap directions
    sensor_info->resolution = 1;
    sensor_info->power = g_sensor_control->sensor_list[HANDLE_ACCELERATION].power
                      + g_sensor_control->sensor_list[HANDLE_GYRO].power;
    sensor_info->max_freq = 200;
    sensor_info->min_freq = 1;
    sensor_info->is_attrib_ok = true;
    sensor_info->max_buffered_samples = 0;
  }

  sensor_info = &g_sensor_control->sensor_list[ HANDLE_GESTURE_FACING ];
  strlcpy( sensor_info->name, "Facing", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
  strlcpy( sensor_info->vendor, "Qualcomm", SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
  sensor_info->version = 1;
  sensor_info->handle = HANDLE_GESTURE_FACING;
  sensor_info->type = SENSOR_TYPE_FACING;
  sensor_info->max_range = 3; // range of enum values
  sensor_info->resolution = 1;
  sensor_info->power = g_sensor_control->sensor_list[ HANDLE_ACCELERATION ].power;
  sensor_info->max_freq = 100;
  sensor_info->min_freq = 1;
  sensor_info->is_attrib_ok = true;
  sensor_info->max_buffered_samples = 0;

  // ASJ : Add :: only add this sensor if we have gyros
  sensor_info = &g_sensor_control->sensor_list[ HANDLE_GESTURE_TILT ];
  strlcpy( sensor_info->name, "Tilt", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
  strlcpy( sensor_info->vendor, "Qualcomm", SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
  sensor_info->version = 1;
  sensor_info->handle = HANDLE_GESTURE_TILT;
  sensor_info->type = SENSOR_TYPE_TILT;
  sensor_info->max_range = 180.0; // unit in degree
  sensor_info->resolution = 0.1;  // unit in degree
  sensor_info->power = g_sensor_control->sensor_list[ HANDLE_GYRO ].power;
  sensor_info->max_freq = 100;
  sensor_info->min_freq = 1;
  sensor_info->is_attrib_ok = true;
  sensor_info->max_buffered_samples = 0;
}

/*===========================================================================

  FUNCTION:  hal_add_uncalibrated_sensors

===========================================================================*/
static void
hal_add_uncalibrated_sensors( void )
{
  hal_sensor_info_t*    sensor_info;

  bool sam_supports_cal_mag = (g_sensor_control->sensor_list[HANDLE_MAGNETIC_FIELD_SAM].handle == HANDLE_MAGNETIC_FIELD_SAM) &&
                              hal_is_mag_available();

  bool smgr_supports_cal_mag = hal_is_mag_available() &&
                               ((g_sensor_control->mag_cal_src == HAL_MAG_CAL_SRC_SMGR) ||
                                (g_sensor_control->mag_cal_src == HAL_MAG_CAL_SRC_NONE));

  HAL_LOG_DEBUG("%s:, Mag Cal support : SMGR %d, SAM %d",
                __FUNCTION__, smgr_supports_cal_mag, sam_supports_cal_mag );

  /* uncalibrated mag */
  if (sam_supports_cal_mag )
  {
    hal_sensor_info_t* mag_sensor_info = &g_sensor_control->sensor_list[ HANDLE_MAGNETIC_FIELD ];
    sensor_info = &g_sensor_control->sensor_list[ HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM ];
    strlcpy( sensor_info->name, mag_sensor_info->name, SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
    strlcat( sensor_info->name, " Uncalibrated", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
    strlcpy( sensor_info->vendor, mag_sensor_info->vendor, SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
    sensor_info->version = mag_sensor_info->version;
    sensor_info->handle = HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM;
    sensor_info->type = SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED;
    sensor_info->max_range = mag_sensor_info->max_range;
    sensor_info->resolution = mag_sensor_info->resolution;
    sensor_info->power = mag_sensor_info->power;
    sensor_info->max_freq = mag_sensor_info->max_freq;
    sensor_info->min_freq = mag_sensor_info->min_freq;
    sensor_info->is_attrib_ok = true;
    sensor_info->max_buffered_samples = 0;
    HAL_LOG_DEBUG("%s:, Added Uncal Mag from SAM", __FUNCTION__);
  }

  if (smgr_supports_cal_mag)
  {
    hal_sensor_info_t* mag_sensor_info = &g_sensor_control->sensor_list[ HANDLE_MAGNETIC_FIELD ];
    sensor_info = &g_sensor_control->sensor_list[ HANDLE_MAGNETIC_FIELD_UNCALIBRATED ];

    strlcpy( sensor_info->name, mag_sensor_info->name, SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
    strlcat( sensor_info->name, " Uncalibrated", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
    strlcpy( sensor_info->vendor, mag_sensor_info->vendor, SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
    sensor_info->version = mag_sensor_info->version;
    sensor_info->handle = HANDLE_MAGNETIC_FIELD_UNCALIBRATED;
    sensor_info->type = SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED;
    sensor_info->max_range = mag_sensor_info->max_range;
    sensor_info->resolution = mag_sensor_info->resolution;
    sensor_info->power = mag_sensor_info->power;
    sensor_info->max_freq = mag_sensor_info->max_freq;
    sensor_info->min_freq = mag_sensor_info->min_freq;
    sensor_info->is_attrib_ok = mag_sensor_info->is_attrib_ok;
    sensor_info->max_buffered_samples = mag_sensor_info->max_buffered_samples;
  }

  /* uncalibrated gyro */
  if (hal_is_gyro_available())
  {
    hal_sensor_info_t* gyro_sensor_info = &g_sensor_control->sensor_list[ HANDLE_GYRO ];
    sensor_info = &g_sensor_control->sensor_list[ HANDLE_GYRO_UNCALIBRATED ];
    strlcpy( sensor_info->name, gyro_sensor_info->name, SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
    strlcat( sensor_info->name, " Uncalibrated", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
    strlcpy( sensor_info->vendor, gyro_sensor_info->vendor, SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 );
    sensor_info->version = gyro_sensor_info->version;
    sensor_info->handle = HANDLE_GYRO_UNCALIBRATED;
    sensor_info->type = SENSOR_TYPE_GYROSCOPE_UNCALIBRATED;
    sensor_info->max_range = gyro_sensor_info->max_range;
    sensor_info->resolution = gyro_sensor_info->resolution;
    sensor_info->power = gyro_sensor_info->power;
    sensor_info->max_freq = gyro_sensor_info->max_freq;
    sensor_info->min_freq = gyro_sensor_info->min_freq;
    sensor_info->is_attrib_ok = gyro_sensor_info->is_attrib_ok;
    sensor_info->max_buffered_samples = gyro_sensor_info->max_buffered_samples;
  }
}

/*===========================================================================

  FUNCTION:  hal_is_sam_mag_cal_available

===========================================================================*/
/*!
  @brief
  Check if SAM supports mag cal

  Note that:
    hal_is_sam_mag_cal_available must be called befor HAL has been initialized.
    The cb_mutex must be held before this function is called
*/
static bool
hal_is_sam_mag_cal_available( hal_sensor_control_t* sensor_ctl )
{
  sensor1_error_e error;

  if( hal_sensor1_open( sensor_ctl ) != SENSOR1_SUCCESS )
  {
    HAL_LOG_ERROR("%s: HAL control init failed", __FUNCTION__ );
    return false;
  }
  else
  {
    sensor1_msg_header_s    req_hdr;
    sns_common_version_req_msg_v01* sam_req;

    error = sensor1_alloc_msg_buf(sensor_ctl->hndl,
                          sizeof(sns_common_version_req_msg_v01),
                          (void**)&sam_req);
    if( SENSOR1_SUCCESS != error )
    {
      HAL_LOG_ERROR("%s: sensor1_alloc_msg_buf() error: %d", __FUNCTION__, error );
      hal_sensor1_close( sensor_ctl );
      return (false);
    }

    req_hdr.service_number = SNS_SAM_MAG_CAL_SVC_ID_V01;
    req_hdr.msg_id = SNS_SAM_MAG_CAL_VERSION_REQ_V01;
    req_hdr.msg_size = sizeof(sns_common_version_req_msg_v01);
    req_hdr.txn_id = 0;

    /* Send request */
    sensor_ctl->error = false;
    if( ( sensor1_write( sensor_ctl->hndl, &req_hdr,
                                sam_req )) != SENSOR1_SUCCESS )
    {
      /* free the message buffer */
      sensor1_free_msg_buf( sensor_ctl->hndl, sam_req );
      HAL_LOG_ERROR( "%s: sensor1_write() error: %d", __FUNCTION__, error );
      hal_sensor1_close( sensor_ctl );
      return (false);
    }

    /* waiting for response */
    if (hal_wait_for_response( TIME_OUT_MS,
                               &sensor_ctl->cb_mutex,
                               &sensor_ctl->cb_arrived_cond,
                               &g_sensor_control->is_resp_arrived ) == false )
    {
      HAL_LOG_ERROR( "%s: ERROR: No response from request %d", __FUNCTION__,
                     SNS_SAM_MAG_CAL_VERSION_REQ_V01 );
      hal_sensor1_close( sensor_ctl );
      return (false);
    }

    HAL_LOG_DEBUG( "%s: Received Response: %d", __FUNCTION__, sensor_ctl->error );

    hal_sensor1_close( sensor_ctl );
    return ((bool)(sensor_ctl->mag_cal_src == HAL_MAG_CAL_SRC_SAM));
  }
}

/*===========================================================================

  FUNCTION:  hal_sam_mag_cal_report_add

===========================================================================*/
/*!
*/
static bool
hal_sam_mag_cal_report_add( int handle, hal_sensor_control_t* sensor_ctl, uint32_t report_rate)
{
  sensor1_error_e error;
  sensor1_msg_header_s req_hdr;
  sns_sam_mag_cal_enable_req_msg_v01 *sam_req;

  HAL_LOG_DEBUG( "%s: hal_sam_mag_cal_report_add: Hz %d",
                 __FUNCTION__,report_rate );

  error = sensor1_alloc_msg_buf( sensor_ctl->hndl,
                                 sizeof(sns_sam_mag_cal_enable_req_msg_v01),
                                 (void**)&sam_req );
  if( SENSOR1_SUCCESS != error )
  {
    HAL_LOG_ERROR( "%s: sensor1_alloc_msg_buf() error: %d", __FUNCTION__, error );
    return false;
  }

  hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_MAG_CAL_SVC_ID_V01 );

  req_hdr.service_number = SNS_SAM_MAG_CAL_SVC_ID_V01;
  req_hdr.msg_id = SNS_SAM_MAG_CAL_ENABLE_REQ_V01;
  req_hdr.msg_size = sizeof( sns_sam_mag_cal_enable_req_msg_v01 );

  req_hdr.txn_id = 0;
  sam_req->report_period = (uint32_t)(UNIT_Q16/report_rate);
  sam_req->sample_rate_valid = true;
  sam_req->sample_rate = (uint32_t) MAX(FREQ_NORMAL_HZ, report_rate) * UNIT_Q16;

  sensor_ctl->error = false;
  if( (error = sensor1_write( sensor_ctl->hndl, &req_hdr,
                              sam_req )) != SENSOR1_SUCCESS )
  {
    sensor1_free_msg_buf( sensor_ctl->hndl, sam_req );
    HAL_LOG_ERROR( "%s: sensor1_write() error: %d", __FUNCTION__, error );
    return false;
  }

  if (hal_wait_for_response( TIME_OUT_MS,
                             &sensor_ctl->cb_mutex,
                             &sensor_ctl->cb_arrived_cond,
                             &g_sensor_control->is_resp_arrived ) == false )
  {
    HAL_LOG_ERROR( "%s: ERROR: No response from request %d", __FUNCTION__,
                   SNS_SMGR_REPORT_ACTION_ADD_V01 );
    return false;
  }

  HAL_LOG_DEBUG( "%s: Received Response: %d", __FUNCTION__, sensor_ctl->error );

  return !sensor_ctl->error;
}

/*===========================================================================

  FUNCTION:  hal_flush_request

===========================================================================*/
/*!
  @brief
  Sends flush command to handler for this sensor.  Must hold cb_mutex and set
  g_sensor_control->flush_requested beforehand.

  */
static void hal_flush_request( int handle )
{
  if( SAM_HANDLE_BASE > handle ) {
    /* SMGR-based sensor. Send flush command to SMGR */
    hal_smgr_report_add( handle, g_sensor_control,
                         g_sensor_control->current_freq[ handle ],
                         g_sensor_control->current_rpt_rate[ handle ],
                         g_sensor_control->current_WuFF[ handle ],
                         g_sensor_control->current_batching[ handle ],
                         false );
  } else {
    /* SAM-based sensor. Send flush command to correct algo.
     * Even if no values have changed since the last call to activate,
     * a new batch enable request will still be sent out. */
    hal_sam_activate( g_sensor_control, handle );
  }
}

/*===========================================================================

  FUNCTION:  hal_sam_qmd_report_add

  This function adds QMD services based on handle.

===========================================================================*/
/*!
*/
static bool
hal_sam_qmd_report_add( int handle, hal_sensor_control_t* sensor_ctl, uint32_t report_rate )
{
  sensor1_error_e         error;
  sensor1_msg_header_s    req_hdr;
  sns_sam_qmd_enable_req_msg_v01*  sam_req;
  bool resp = false;

  HAL_LOG_DEBUG("%s: handle=%d, report_rate=%d", __FUNCTION__, handle, report_rate);

  /* Message Body */
  error = sensor1_alloc_msg_buf( sensor_ctl->hndl,
                                 sizeof(sns_sam_qmd_enable_req_msg_v01),
                                 (void**)&sam_req );
  if( SENSOR1_SUCCESS != error )
  {
    HAL_LOG_ERROR("%s: sensor1_alloc_msg_buf() error: %d", __FUNCTION__, error );
    return (false);
  }

  switch( handle )
  {
    case HANDLE_MOTION_ABSOLUTE:
      /* Message header */
      req_hdr.service_number = SNS_SAM_AMD_SVC_ID_V01;
      req_hdr.msg_id = SNS_SAM_AMD_ENABLE_REQ_V01;
      break;

    case HANDLE_MOTION_RELATIVE:
      /* Message header */
      req_hdr.service_number = SNS_SAM_RMD_SVC_ID_V01;
      req_hdr.msg_id = SNS_SAM_RMD_ENABLE_REQ_V01;
      break;

    case HANDLE_MOTION_VEHICLE:
      /* Message header */
      req_hdr.service_number = SNS_SAM_VMD_SVC_ID_V01;
      req_hdr.msg_id = SNS_SAM_VMD_ENABLE_REQ_V01;
      break;

    case HANDLE_MOTION_ACCEL:
      /* Message header */
      req_hdr.service_number = SNS_SAM_RMD_SVC_ID_V01;
      req_hdr.msg_id = SNS_SAM_RMD_ENABLE_REQ_V01;
      break;

    default:
      HAL_LOG_ERROR("%s: Error: invalid sensor handle %d", __FUNCTION__, handle);
      sensor1_free_msg_buf( sensor_ctl->hndl, sam_req );
      return (false);
  }

  // Always cancel first, just in case
  hal_sam_send_cancel( sensor_ctl->hndl, req_hdr.service_number );

  req_hdr.msg_size = sizeof( sns_sam_qmd_enable_req_msg_v01 );
  sam_req->config_valid = false;

  /* set default behavior for indications during suspend */
  sam_req->notify_suspend_valid = true;
  sam_req->notify_suspend.proc_type = SNS_PROC_APPS_V01;
  sam_req->notify_suspend.send_indications_during_suspend = false;

  /* Operate in asynchronous mode */
  sam_req->report_period = 0;

  req_hdr.txn_id = 0;

  /* Send request */
  sensor_ctl->error = false;
  if( (error = sensor1_write( sensor_ctl->hndl, &req_hdr,
                              sam_req )) != SENSOR1_SUCCESS )
  {
    /* free the message buffer */
    sensor1_free_msg_buf( sensor_ctl->hndl, sam_req );
    HAL_LOG_ERROR("%s: sensor1_write() error: %d", __FUNCTION__, error );
    return (false);
  }

  resp = hal_wait_for_response( TIME_OUT_MS,
                        &(sensor_ctl->cb_mutex),
                        &(sensor_ctl->cb_arrived_cond),
                        &(sensor_ctl->is_resp_arrived));

  if( resp == false )
  {
    HAL_LOG_ERROR("%s: ERROR: No response from request qmd_report_add for svc %d", __FUNCTION__,
          req_hdr.service_number);
    return (false);
  }

  HAL_LOG_DEBUG("%s: Received QMD Response: %d", __FUNCTION__, sensor_ctl->error);

  /* received response */
  if( sensor_ctl->error == true )
  {
    return (false);
  }

  return (true);
}

/*===========================================================================

  FUNCTION:  hal_sam_smd_report_add

===========================================================================*/
/*!
*/
static bool
hal_sam_smd_report_add( int handle, hal_sensor_control_t* sensor_ctl, uint32_t report_rate )
{
  sensor1_error_e         error;
  sensor1_msg_header_s    req_hdr;
  sns_sam_smd_enable_req_msg_v01 *sam_req;

  HAL_LOG_DEBUG("%s: handle=%d, report_rate=%d", __FUNCTION__, handle, report_rate);

  // Always cancel first, just in case
  hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_SMD_SVC_ID_V01 );

  /* Message Body */
  error = sensor1_alloc_msg_buf( sensor_ctl->hndl,
                                 sizeof(sns_sam_smd_enable_req_msg_v01),
                                 (void**)&sam_req );
  if( SENSOR1_SUCCESS != error )
  {
    HAL_LOG_ERROR("%s: sensor1_alloc_msg_buf() error: %d", __FUNCTION__, error );
    return (false);
  }

  /* Message header */
  req_hdr.service_number = SNS_SAM_SMD_SVC_ID_V01;
  req_hdr.msg_id = SNS_SAM_SMD_ENABLE_REQ_V01;
  req_hdr.msg_size = sizeof( sns_sam_smd_enable_req_msg_v01 );
  req_hdr.txn_id = 0;

  sam_req->notify_suspend_valid = true;
  sam_req->notify_suspend.proc_type = SNS_PROC_APPS_V01;
  sam_req->notify_suspend.send_indications_during_suspend = true;

  /* Send request */
  sensor_ctl->error = false;
  if( (error = sensor1_write( sensor_ctl->hndl, &req_hdr,
                              sam_req )) != SENSOR1_SUCCESS )
  {
    /* free the message buffer */
    sensor1_free_msg_buf( sensor_ctl->hndl, sam_req );
    HAL_LOG_ERROR("%s: sensor1_write() error: %d", __FUNCTION__, error );
    return (false);
  }

  /* waiting for response */
  if (hal_wait_for_response( TIME_OUT_MS,
                             &sensor_ctl->cb_mutex,
                             &sensor_ctl->cb_arrived_cond,
                             &g_sensor_control->is_resp_arrived ) == false )
  {
    HAL_LOG_ERROR("%s: ERROR: No response from request %d", __FUNCTION__,
         SNS_SMGR_REPORT_ACTION_ADD_V01);
    return (false);
  }

  HAL_LOG_DEBUG("%s: Received Response: %d", __FUNCTION__, sensor_ctl->error);

  /* received response */
  if( sensor_ctl->error == true )
  {
    return (false);
  }

  return (true);
}

/*===========================================================================

FUNCTION: hal_sam_cmc_report_add

===========================================================================*/
/*!
*/
static bool
hal_sam_cmc_report_add( int handle, hal_sensor_control_t* sensor_ctl, uint32_t report_rate )
{
  sensor1_error_e error;
  sensor1_msg_header_s req_hdr;
  sns_sam_cmc_enable_req_msg_v01* sam_req;

  // Always cancel first, just in case
  hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_CMC_SVC_ID_V01 );

  HAL_LOG_DEBUG("%s: handle=%d", __FUNCTION__, handle);

  /* Message Body */
  error = sensor1_alloc_msg_buf( sensor_ctl->hndl,
                                 sizeof(sns_sam_cmc_enable_req_msg_v01),
                                 (void**)&sam_req );
  if( SENSOR1_SUCCESS != error )
  {
    HAL_LOG_ERROR("%s: sensor1_alloc_msg_buf() error: %d", __FUNCTION__, error );
    return (false);
  }

  req_hdr.service_number = SNS_SAM_CMC_SVC_ID_V01;
  req_hdr.msg_id = SNS_SAM_CMC_ENABLE_REQ_V01;
  req_hdr.msg_size = sizeof(sns_sam_cmc_enable_req_msg_v01);
  req_hdr.txn_id = HANDLE_CMC;

  /* set defualt behavior for indications during suspend */
  sam_req->notify_suspend_valid = true;
  sam_req->notify_suspend.proc_type = SNS_PROC_APPS_V01;
  sam_req->notify_suspend.send_indications_during_suspend = true;

  /* Send request */
  sensor_ctl->error = false;
  if( (error = sensor1_write( sensor_ctl->hndl, &req_hdr,
                              sam_req )) != SENSOR1_SUCCESS )
  {
    /* free the message buffer */
    sensor1_free_msg_buf( sensor_ctl->hndl, sam_req );
    HAL_LOG_ERROR("%s: sensor1_write() error: %d", __FUNCTION__, error );
    return (false);
  }

  /* waiting for response */
  if (hal_wait_for_response( TIME_OUT_MS,
                             &sensor_ctl->cb_mutex,
                             &sensor_ctl->cb_arrived_cond,
                             &g_sensor_control->is_resp_arrived ) == false )
  {
    HAL_LOG_ERROR("%s: ERROR: No reponse from CMC enable request %d", __FUNCTION__, SNS_SAM_CMC_ENABLE_REQ_V01);
    return (false);
  }

  HAL_LOG_DEBUG("%s: Received CMC enable response: %d", __FUNCTION__, sensor_ctl->error);

  /* received response */
  if( sensor_ctl->error == true )
  {
    return (false);
  }

  return (true);
}

/*===========================================================================

  FUNCTION:  hal_sam_pam_report_add

===========================================================================*/
/*!
*/
static bool
hal_sam_pam_report_add( int handle, hal_sensor_control_t* sensor_ctl, uint32_t report_rate )
{
  sensor1_error_e         error;
  sensor1_msg_header_s    req_hdr;
  sns_sam_pam_enable_req_msg_v01*  sam_req;
  uint32_t ms = NSEC_TO_MSEC( HZ_TO_NSEC( report_rate ) );

  // Always cancel first, just in case
  hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_PAM_SVC_ID_V01 );

  HAL_LOG_DEBUG("%s: handle=%d", __FUNCTION__, handle);

  /* Message Body */
  error = sensor1_alloc_msg_buf( sensor_ctl->hndl,
                                 sizeof(sns_sam_pam_enable_req_msg_v01),
                                 (void**)&sam_req );
  if( SENSOR1_SUCCESS != error )
  {
    HAL_LOG_ERROR("%s: sensor1_alloc_msg_buf() error: %d", __FUNCTION__, error );
    return (false);
  }

  req_hdr.service_number = SNS_SAM_PAM_SVC_ID_V01;
  req_hdr.msg_id = SNS_SAM_PAM_ENABLE_REQ_V01;
  req_hdr.msg_size = sizeof(sns_sam_pam_enable_req_msg_v01);
  req_hdr.txn_id = 1;

  if(ms >= 15000 && ms < 1800000) /* min = 15 sec, max = 30 min*/
  {
    sam_req->measurement_period = (uint32_t)(ms/1000.0); // sec
  }
  else
  {
    sam_req->measurement_period = 20; // sec
  }

  /* set default behavior for indications during suspend */
  sam_req->notify_suspend_valid = true;
  sam_req->notify_suspend.proc_type = SNS_PROC_APPS_V01;
  sam_req->notify_suspend.send_indications_during_suspend = false;

  sam_req->step_count_threshold = 2;/*specify the threshold for user activity in terms
                                    of steps per measurement period. PAM will generate a
                                    report only when the steps per measurement period differs
                                    the last reported value by a value greater than this threshold*/

  /* Send request */
  sensor_ctl->error = false;
  if( (error = sensor1_write( sensor_ctl->hndl, &req_hdr,
                              sam_req )) != SENSOR1_SUCCESS )
  {
    /* free the message buffer */
    sensor1_free_msg_buf( sensor_ctl->hndl, sam_req );
    HAL_LOG_ERROR("%s: sensor1_write() error: %d", __FUNCTION__, error );
    return (false);
  }

  /* waiting for response */
  if (hal_wait_for_response( TIME_OUT_MS,
                             &sensor_ctl->cb_mutex,
                             &sensor_ctl->cb_arrived_cond,
                             &g_sensor_control->is_resp_arrived ) == false )
  {
    HAL_LOG_ERROR("%s: ERROR: No reponse from request %d", __FUNCTION__,
         SNS_SMGR_REPORT_ACTION_ADD_V01);
    return (false);
  }

  HAL_LOG_DEBUG("%s: Received Response: %d", __FUNCTION__, sensor_ctl->error);

  /* received response */
  if( sensor_ctl->error == true )
  {
    return (false);
  }

  return (true);
}

#ifdef FEATURE_SNS_HAL_SAM_INT
/*===========================================================================

  FUNCTION:  hal_sam_bte_report_add

===========================================================================*/
/*!
*/
static bool
hal_sam_bte_report_add( int handle, hal_sensor_control_t* sensor_ctl )
{
  sensor1_error_e         error;
  sensor1_msg_header_s    req_hdr;
  sns_sam_bte_enable_req_msg_v01*  sam_req;

  // Always cancel first, just in case
  hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_BTE_SVC_ID_V01 );

  HAL_LOG_DEBUG("%s: handle=%d", __FUNCTION__, handle);
  sensor_ctl->current_freq[ handle ] = 1;

  /* Message Body */
  error = sensor1_alloc_msg_buf( sensor_ctl->hndl,
                                 sizeof(sns_sam_bte_enable_req_msg_v01),
                                 (void**)&sam_req );
  if( SENSOR1_SUCCESS != error )
  {
    HAL_LOG_ERROR("%s: sensor1_alloc_msg_buf() error: %d", __FUNCTION__, error );
    return (false);
  }

  /* Message header */
  req_hdr.service_number = SNS_SAM_BTE_SVC_ID_V01;
  req_hdr.msg_id = SNS_SAM_BTE_ENABLE_REQ_V01;
  req_hdr.msg_size = sizeof( sns_sam_bte_enable_req_msg_v01 );
  req_hdr.txn_id = 0;

  /* set default behavior for indications during suspend */
  sam_req->notify_suspend_valid = true;
  sam_req->notify_suspend.proc_type = SNS_PROC_APPS_V01;
  sam_req->notify_suspend.send_indications_during_suspend = false;

  /* Send request */
  sensor_ctl->error = false;
  if( (error = sensor1_write( sensor_ctl->hndl, &req_hdr,
                              sam_req )) != SENSOR1_SUCCESS )
  {
    /* free the message buffer */
    sensor1_free_msg_buf( sensor_ctl->hndl, sam_req );
    HAL_LOG_ERROR("%s: sensor1_write() error: %d", __FUNCTION__, error );
    return (false);
  }

  /* waiting for response */
  if (hal_wait_for_response( TIME_OUT_MS,
                             &sensor_ctl->cb_mutex,
                             &sensor_ctl->cb_arrived_cond,
                             &g_sensor_control->is_resp_arrived ) == false )
  {
    HAL_LOG_ERROR("%s: ERROR: No response from request %d", __FUNCTION__,
         SNS_SMGR_REPORT_ACTION_ADD_V01);
    return (false);
  }

  HAL_LOG_DEBUG("%s: Received Response: %d", __FUNCTION__, sensor_ctl->error);

  /* received response */
  if( sensor_ctl->error == true )
  {
    return (false);
  }

  return (true);
}

/*===========================================================================

  FUNCTION:  hal_sam_fns_report_add

===========================================================================*/
/*!
*/
static bool
hal_sam_fns_report_add( int handle, hal_sensor_control_t* sensor_ctl )
{
  sensor1_error_e         error;
  sensor1_msg_header_s    req_hdr;
  sns_sam_fns_enable_req_msg_v01*  sam_req;

  // Always cancel first, just in case
  hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_FNS_SVC_ID_V01 );

  HAL_LOG_DEBUG("%s: handle=%d", __FUNCTION__, handle);
  sensor_ctl->current_freq[ handle ] = 1;

  /* Message Body */
  error = sensor1_alloc_msg_buf( sensor_ctl->hndl,
                                 sizeof(sns_sam_fns_enable_req_msg_v01),
                                 (void**)&sam_req );
  if( SENSOR1_SUCCESS != error )
  {
    HAL_LOG_ERROR("%s: sensor1_alloc_msg_buf() error: %d", __FUNCTION__, error );
    return (false);
  }

  /* Message header */
  req_hdr.service_number = SNS_SAM_FNS_SVC_ID_V01;
  req_hdr.msg_id = SNS_SAM_FNS_ENABLE_REQ_V01;
  req_hdr.msg_size = sizeof( sns_sam_fns_enable_req_msg_v01 );
  req_hdr.txn_id = 0;

  /* set default behavior for indications during suspend */
  sam_req->notify_suspend_valid = true;
  sam_req->notify_suspend.proc_type = SNS_PROC_APPS_V01;
  sam_req->notify_suspend.send_indications_during_suspend = false;

  /* Send request */
  sensor_ctl->error = false;
  if( (error = sensor1_write( sensor_ctl->hndl, &req_hdr,
                              sam_req )) != SENSOR1_SUCCESS )
  {
    /* free the message buffer */
    sensor1_free_msg_buf( sensor_ctl->hndl, sam_req );
    HAL_LOG_ERROR("%s: sensor1_write() error: %d", __FUNCTION__, error );
    return (false);
  }

  /* waiting for response */
  if (hal_wait_for_response( TIME_OUT_MS,
                             &sensor_ctl->cb_mutex,
                             &sensor_ctl->cb_arrived_cond,
                             &g_sensor_control->is_resp_arrived ) == false )
  {
    HAL_LOG_ERROR("%s: ERROR: No response from request %d", __FUNCTION__,
         SNS_SMGR_REPORT_ACTION_ADD_V01);
    return (false);
  }

  HAL_LOG_DEBUG("%s: Received Response: %d", __FUNCTION__, sensor_ctl->error);

  /* received response */
  if( sensor_ctl->error == true )
  {
    return (false);
  }

  return (true);
}
#endif /* FEATURE_SNS_HAL_SAM_INT */

/*===========================================================================

  FUNCTION:  hal_sam_basic_gestures_report_add

===========================================================================*/
/*!
*/
static bool
hal_sam_basic_gestures_report_add( int handle, hal_sensor_control_t* sensor_ctl )
{
  sensor1_error_e         error;
  sensor1_msg_header_s    req_hdr;
  sns_sam_basic_gestures_enable_req_msg_v01*  sam_req;

  // Always cancel first, just in case
  hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_BASIC_GESTURES_SVC_ID_V01 );

  HAL_LOG_DEBUG("%s: handle=%d", __FUNCTION__, handle );
  sensor_ctl->current_freq[ handle ] = 1;

  /* Message Body */
  error = sensor1_alloc_msg_buf( sensor_ctl->hndl,
                                 sizeof(sns_sam_basic_gestures_enable_req_msg_v01),
                                 (void**)&sam_req );
  if( SENSOR1_SUCCESS != error )
  {
    HAL_LOG_ERROR("%s: sensor1_alloc_msg_buf() error: %d", __FUNCTION__, error );
    return (false);
  }

  switch( handle )
  {
    case HANDLE_GESTURE_BASIC_GESTURES:
      /* Message header */
      req_hdr.service_number = SNS_SAM_BASIC_GESTURES_SVC_ID_V01;
      req_hdr.msg_id = SNS_SAM_BASIC_GESTURES_ENABLE_REQ_V01;
      req_hdr.msg_size = sizeof( sns_sam_basic_gestures_enable_req_msg_v01 );

      /* set default behavior for indications during suspend */
      sam_req->notify_suspend_valid = true;
      sam_req->notify_suspend.proc_type = SNS_PROC_APPS_V01;
      sam_req->notify_suspend.send_indications_during_suspend = true;
      break;

    default:
      HAL_LOG_ERROR("%s: Error: invalid sensor handle %d", __FUNCTION__, handle);
      sensor1_free_msg_buf( sensor_ctl->hndl, sam_req );
      return (false);
  }

  req_hdr.txn_id = 0;

  /* Send request */
  sensor_ctl->error = false;
  if( (error = sensor1_write( sensor_ctl->hndl, &req_hdr,
                              sam_req )) != SENSOR1_SUCCESS )
  {
    /* free the message buffer */
    sensor1_free_msg_buf( sensor_ctl->hndl, sam_req );
    HAL_LOG_ERROR("%s: sensor1_write() error: %d", __FUNCTION__, error );
    return (false);
  }

  /* waiting for response */
  if (hal_wait_for_response( TIME_OUT_MS,
                             &sensor_ctl->cb_mutex,
                             &sensor_ctl->cb_arrived_cond,
                             &g_sensor_control->is_resp_arrived ) == false )


  {
    HAL_LOG_ERROR("%s: ERROR: No response from request %d", __FUNCTION__,
         SNS_SMGR_REPORT_ACTION_ADD_V01);
    return (false);
  }

  HAL_LOG_DEBUG("%s: Received Response: %d", __FUNCTION__, sensor_ctl->error);

  /* received response */
  if( sensor_ctl->error == true )
  {
    return (false);
  }

  return (true);
}

/*===========================================================================

  FUNCTION:  hal_sam_tap_report_add

===========================================================================*/
/*!
*/
static bool
hal_sam_tap_report_add( int handle, hal_sensor_control_t* sensor_ctl )
{
  sensor1_error_e         error;
  sensor1_msg_header_s    req_hdr;
  sns_sam_tap_enable_req_msg_v01*  sam_req;

  // Always cancel first, just in case
  hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_TAP_SVC_ID_V01 );

  HAL_LOG_DEBUG("%s: handle=%d", __FUNCTION__, handle );
  sensor_ctl->current_freq[ handle ] = 1;

  /* Message Body */
  error = sensor1_alloc_msg_buf( sensor_ctl->hndl,
                                 sizeof(sns_sam_tap_enable_req_msg_v01),
                                 (void**)&sam_req );
  if( SENSOR1_SUCCESS != error )
  {
    HAL_LOG_ERROR("%s: sensor1_alloc_msg_buf() error: %d", __FUNCTION__, error );
    return (false);
  }

  switch( handle )
  {
    case HANDLE_GESTURE_TAP:
      /* Message header */
      req_hdr.service_number = SNS_SAM_TAP_SVC_ID_V01;
      req_hdr.msg_id = SNS_SAM_TAP_ENABLE_REQ_V01;
      req_hdr.msg_size = sizeof( sns_sam_tap_enable_req_msg_v01 );
      break;

    default:
      HAL_LOG_ERROR("%s: Error: invalid sensor handle %d", __FUNCTION__, handle);
      sensor1_free_msg_buf( sensor_ctl->hndl, sam_req );
      return (false);
  }

  req_hdr.txn_id = 0;

  /* Send request */
  sensor_ctl->error = false;
  if( (error = sensor1_write( sensor_ctl->hndl, &req_hdr,
                              sam_req )) != SENSOR1_SUCCESS )
  {
    /* free the message buffer */
    sensor1_free_msg_buf( sensor_ctl->hndl, sam_req );
    HAL_LOG_ERROR("%s: sensor1_write() error: %d", __FUNCTION__, error );
    return (false);
  }

  /* waiting for response */
  if (hal_wait_for_response( TIME_OUT_MS,
                             &sensor_ctl->cb_mutex,
                             &sensor_ctl->cb_arrived_cond,
                             &g_sensor_control->is_resp_arrived ) == false )
  {
    HAL_LOG_ERROR("%s: ERROR: No response from request %d", __FUNCTION__,
         SNS_SMGR_REPORT_ACTION_ADD_V01);
    return (false);
  }

  HAL_LOG_DEBUG("%s: Received Response: %d", __FUNCTION__, sensor_ctl->error);

  /* received response */
  if( sensor_ctl->error == true )
  {
    return (false);
  }

  return (true);
}

/*===========================================================================

  FUNCTION:  hal_sam_gyro_tap_report_add

===========================================================================*/
/*!
*/
static bool
hal_sam_gyro_tap_report_add( int handle, hal_sensor_control_t* sensor_ctl )
{
  sensor1_error_e         error;
  sensor1_msg_header_s    req_hdr;
  sns_sam_gyro_tap2_enable_req_msg_v01*  sam_req;

  // Always cancel first, just in case
  hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_GYRO_TAP2_SVC_ID_V01 );

  HAL_LOG_DEBUG("%s: handle=%d", __FUNCTION__, handle );
  sensor_ctl->current_freq[ handle ] = 1;

  /* Message Body */
  error = sensor1_alloc_msg_buf( sensor_ctl->hndl,
                                 sizeof(sns_sam_gyro_tap2_enable_req_msg_v01),
                                 (void**)&sam_req );
  if( SENSOR1_SUCCESS != error )
  {
    HAL_LOG_ERROR("%s: sensor1_alloc_msg_buf() error: %d", __FUNCTION__, error );
    return (false);
  }

  switch( handle )
  {
    case HANDLE_GESTURE_GYRO_TAP:
      /* Message header */
      req_hdr.service_number = SNS_SAM_GYRO_TAP2_SVC_ID_V01;
      req_hdr.msg_id = SNS_SAM_GYRO_TAP2_ENABLE_REQ_V01;
      req_hdr.msg_size = sizeof( sns_sam_gyro_tap2_enable_req_msg_v01 );

      /* We use a named scenario -this allows the parameters for this scenario
         to be modified through the registry. If the scenario is not named, the
         default parameters cannot be modified from the registry.
      */
      sam_req->scenario = 1;
      sam_req->scenario_valid = true;
      HAL_LOG_DEBUG("%s: handle=%d constructed gyro_tap_enable_req", __FUNCTION__, handle );

      /* set default behavior for indications during suspend */
      sam_req->notify_suspend_valid = true;
      sam_req->notify_suspend.proc_type = SNS_PROC_APPS_V01;
      sam_req->notify_suspend.send_indications_during_suspend = false;

      break;

    default:
      HAL_LOG_ERROR("%s: Error: invalid sensor handle %d", __FUNCTION__, handle);
      sensor1_free_msg_buf( sensor_ctl->hndl, sam_req );
      return (false);
  }

  req_hdr.txn_id = 0;

  /* Send request */
  sensor_ctl->error = false;
  HAL_LOG_DEBUG("%s: handle=%d Sending gyro_tap_enable_req", __FUNCTION__, handle );
  if( (error = sensor1_write( sensor_ctl->hndl, &req_hdr,
                              sam_req )) != SENSOR1_SUCCESS )
  {
    /* free the message buffer */
    sensor1_free_msg_buf( sensor_ctl->hndl, sam_req );
    HAL_LOG_ERROR("%s: sensor1_write() error: %d", __FUNCTION__, error );
    return (false);
  }

  /* waiting for response */
  if (hal_wait_for_response( TIME_OUT_MS,
                             &sensor_ctl->cb_mutex,
                             &sensor_ctl->cb_arrived_cond,
                             &g_sensor_control->is_resp_arrived ) == false )
  {
    HAL_LOG_ERROR("%s: ERROR: No response from request %d", __FUNCTION__,
         SNS_SMGR_REPORT_ACTION_ADD_V01);
    return (false);
  }

  HAL_LOG_DEBUG("%s: Received Response: %d", __FUNCTION__, sensor_ctl->error);

  /* received response */
  if( sensor_ctl->error == true )
  {
    return (false);
  }

  return (true);
}

/*===========================================================================

  FUNCTION:  hal_sam_facing_report_add

===========================================================================*/
/*!
*/
static bool
hal_sam_facing_report_add( int handle, hal_sensor_control_t* sensor_ctl )
{
  sensor1_error_e         error;
  sensor1_msg_header_s    req_hdr;
  sns_sam_facing_enable_req_msg_v01*  sam_req;

  // Always cancel first, just in case
  hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_FACING_SVC_ID_V01 );

  HAL_LOG_DEBUG("%s: handle=%d", __FUNCTION__, handle );
  sensor_ctl->current_freq[ handle ] = 1;

  /* Message Body */
  error = sensor1_alloc_msg_buf( sensor_ctl->hndl,
                                 sizeof(sns_sam_facing_enable_req_msg_v01),
                                 (void**)&sam_req );
  if( SENSOR1_SUCCESS != error )
  {
    HAL_LOG_ERROR("%s: sensor1_alloc_msg_buf() error: %d", __FUNCTION__, error );
    return (false);
  }

  switch( handle )
  {
    case HANDLE_GESTURE_FACING:
      /* Message header */
      req_hdr.service_number = SNS_SAM_FACING_SVC_ID_V01;
      req_hdr.msg_id = SNS_SAM_FACING_ENABLE_REQ_V01;
      req_hdr.msg_size = sizeof( sns_sam_facing_enable_req_msg_v01 );

      /* set default behavior for indications during suspend */
      sam_req->notify_suspend_valid = true;
      sam_req->notify_suspend.proc_type = SNS_PROC_APPS_V01;
      sam_req->notify_suspend.send_indications_during_suspend = true;

      break;

    default:
      HAL_LOG_ERROR("%s: Error: invalid sensor handle %d", __FUNCTION__, handle);
      sensor1_free_msg_buf( sensor_ctl->hndl, sam_req );
      return (false);
  }

  req_hdr.txn_id = 0;

  /* Send request */
  sensor_ctl->error = false;
  if( (error = sensor1_write( sensor_ctl->hndl, &req_hdr,
                              sam_req )) != SENSOR1_SUCCESS )
  {
    /* free the message buffer */
    sensor1_free_msg_buf( sensor_ctl->hndl, sam_req );
    HAL_LOG_ERROR("%s: sensor1_write() error: %d", __FUNCTION__, error );
    return (false);
  }

  /* waiting for response */
  if (hal_wait_for_response( TIME_OUT_MS,
                             &sensor_ctl->cb_mutex,
                             &sensor_ctl->cb_arrived_cond,
                             &g_sensor_control->is_resp_arrived ) == false )
  {
    HAL_LOG_ERROR("%s: ERROR: No response from request %d", __FUNCTION__,
         SNS_SMGR_REPORT_ACTION_ADD_V01);
    return (false);
  }

  HAL_LOG_DEBUG("%s: Received Response: %d", __FUNCTION__, sensor_ctl->error);

  /* received response */
  if( sensor_ctl->error == true )
  {
    return (false);
  }

  return (true);
}

/*===========================================================================

  FUNCTION:  hal_sam_tilt_report_add

===========================================================================*/
/*!
*/
static bool
hal_sam_tilt_report_add( int handle, hal_sensor_control_t* sensor_ctl,
                         uint32_t report_rate )
{
  sensor1_error_e         error;
  sensor1_msg_header_s    req_hdr;
  sns_sam_integ_angle_enable_req_msg_v01*  sam_req;

  // Always cancel first, just in case
  hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_INTEG_ANGLE_SVC_ID_V01 );

  HAL_LOG_DEBUG("%s: handle=%d, report_rate=%d", __FUNCTION__, handle, report_rate );
  /* Message Body */
  error = sensor1_alloc_msg_buf( sensor_ctl->hndl,
                                 sizeof(sns_sam_integ_angle_enable_req_msg_v01),
                                 (void**)&sam_req );
  if( SENSOR1_SUCCESS != error )
  {
    HAL_LOG_ERROR("%s: sensor1_alloc_msg_buf() error: %d", __FUNCTION__, error );
    return (false);
  }

  switch( handle )
  {
    case HANDLE_GESTURE_TILT:
      /* Message header */
      req_hdr.service_number = SNS_SAM_INTEG_ANGLE_SVC_ID_V01;
      req_hdr.msg_id = SNS_SAM_INTEG_ANGLE_ENABLE_REQ_V01;
      req_hdr.msg_size = sizeof( sns_sam_integ_angle_enable_req_msg_v01 );
      break;

    default:
      HAL_LOG_ERROR("%s: Error: invalid sensor handle %d", __FUNCTION__, handle);
      sensor1_free_msg_buf( sensor_ctl->hndl, sam_req );
      return (false);
  }

  req_hdr.txn_id = 0;

  /* Set the sampling rate to the requested report rate */
  /* For tilt, the HAL is an intelligent HAL  - revisit */
  sam_req->sample_rate_valid = true;
  sam_req->sample_rate       = (report_rate << 16);
  sam_req->report_period = (uint32_t)(UNIT_Q16/report_rate);

  /* Send request */
  sensor_ctl->error = false;
  if( (error = sensor1_write( sensor_ctl->hndl, &req_hdr,
                              sam_req )) != SENSOR1_SUCCESS )
  {
    /* free the message buffer */
    sensor1_free_msg_buf( sensor_ctl->hndl, sam_req );
    HAL_LOG_ERROR("%s: sensor1_write() error: %d", __FUNCTION__, error );
    return (false);
  }

  /* waiting for response */
  if (hal_wait_for_response( TIME_OUT_MS,
                             &sensor_ctl->cb_mutex,
                             &sensor_ctl->cb_arrived_cond,
                             &g_sensor_control->is_resp_arrived ) == false )
  {
    HAL_LOG_ERROR("%s: ERROR: No response from request %d", __FUNCTION__,
         SNS_SMGR_REPORT_ACTION_ADD_V01);
    return (false);
  }

  HAL_LOG_DEBUG("%s: Received Response: %d", __FUNCTION__, sensor_ctl->error);

  /* received response */
  if( sensor_ctl->error == true )
  {
    return (false);
  }

  return (true);
}

/*===========================================================================

  FUNCTION:  hal_sam_deactivate_smd

===========================================================================*/
static void hal_sam_deactivate_smd()
{
  sensor1_error_e       error;
  sensor1_msg_header_s  req_hdr;
  sns_sam_smd_disable_req_msg_v01*  sam_req;
  uint64_t mask, sensors;
  uint64_t active_sensors, new_sensors;
  hal_sensor_control_t*  sensor_ctl = g_sensor_control;
  int enabled = 0;
  int handle = HANDLE_SIGNIFICANT_MOTION;

  HAL_LOG_DEBUG("%s", __FUNCTION__ );

  pthread_mutex_lock( &sensor_ctl->cb_mutex );
  mask = (1ULL << handle);
  sensors = enabled ? mask : 0;
  active_sensors = sensor_ctl->active_sensors;
  new_sensors = (active_sensors & ~mask) | (sensors & mask);
  sensor_ctl->active_sensors = new_sensors;

  // send request to disable algo
  error = sensor1_alloc_msg_buf( sensor_ctl->hndl,
                                 sizeof(sns_sam_smd_disable_req_msg_v01),
                                 (void**)&sam_req );
  if( SENSOR1_SUCCESS == error )
  {
    /* Message header */
    req_hdr.service_number = SNS_SAM_SMD_SVC_ID_V01;
    req_hdr.msg_id = SNS_SAM_SMD_DISABLE_REQ_V01;
    req_hdr.msg_size = sizeof( sns_sam_smd_disable_req_msg_v01 );
    req_hdr.txn_id = TXN_ID_NO_RESP_SIGNALLED;
    sam_req->instance_id = g_sensor_control->sam_service[ SNS_SAM_SMD_SVC_ID_V01 ].instance_id;

    HAL_LOG_VERBOSE("%s: Sending SMD disable request. Instance ID %d",
                    __FUNCTION__, sam_req->instance_id );
    /* Send request */
    sensor_ctl->error = false;
    if( (error = sensor1_write( sensor_ctl->hndl, &req_hdr,
                                sam_req )) != SENSOR1_SUCCESS )
    {
      sensor1_free_msg_buf( sensor_ctl->hndl, sam_req );
      HAL_LOG_ERROR("%s: sensor1_write() error: %u", __FUNCTION__, error );
    }
  }
  else
  {
    HAL_LOG_ERROR("%s: failed to allocated disable msg", __FUNCTION__ );
  }

  sensor_ctl->current_freq[handle] = 0;
  sensor_ctl->current_rpt_rate[handle] = 0;
  sensor_ctl->current_batching[handle] = 0;
  sensor_ctl->current_WuFF[handle] = 0;

  pthread_mutex_unlock( &sensor_ctl->cb_mutex );
}

/*===========================================================================

  FUNCTION:  hal_process_sam_mag_cal_resp

===========================================================================*/
/*!
*/
static void
hal_process_sam_mag_cal_resp( sensor1_msg_header_s *msg_hdr,
                              void *msg_ptr )
{
  const sns_common_resp_s_v01*  crsp_ptr = (sns_common_resp_s_v01*) msg_ptr;
  bool                          error = false;

  if( SENSOR1_ENOTALLOWED == crsp_ptr->sns_err_t )
  {
    HAL_LOG_DEBUG( "%s: Algorithm instance ID not found by SAM", __FUNCTION__ );
  }
  else if( SNS_RESULT_SUCCESS_V01 != crsp_ptr->sns_result_t )
  {
    HAL_LOG_ERROR( "%s: Msg %i; Result: %u, Error: %u", __FUNCTION__,
                   msg_hdr->msg_id, crsp_ptr->sns_result_t, crsp_ptr->sns_err_t );
    error = true;
  }
  else
  {
    switch( msg_hdr->msg_id )
    {
      case SNS_SAM_MAG_CAL_ENABLE_RESP_V01:
        if( ((sns_sam_mag_cal_enable_resp_msg_v01*) msg_ptr)->instance_id_valid )
        {
          g_sensor_control->sam_service[ SNS_SAM_MAG_CAL_SVC_ID_V01 ].instance_id  =
            ((sns_sam_mag_cal_enable_resp_msg_v01*) msg_ptr)->instance_id;
          HAL_LOG_DEBUG( "%s: Instance ID of Mag Cal is %d", __FUNCTION__,
                g_sensor_control->sam_service[ SNS_SAM_MAG_CAL_SVC_ID_V01 ].instance_id );
        }
        break;
      case SNS_SAM_MAG_CAL_DISABLE_RESP_V01:
      case SNS_SAM_MAG_CAL_CANCEL_RESP_V01:
        /* Reset instance ID */
        g_sensor_control->sam_service[ SNS_SAM_MAG_CAL_SVC_ID_V01 ].instance_id = 0xFF;
        break;
      case SNS_SAM_MAG_CAL_VERSION_RESP_V01:
         if((((sns_common_version_resp_msg_v01*) msg_ptr)->resp.sns_result_t == SNS_RESULT_SUCCESS_V01) &&
            (((sns_common_version_resp_msg_v01*) msg_ptr)->interface_version_number > 0 ))
         {
           g_sensor_control->mag_cal_src = HAL_MAG_CAL_SRC_SAM;
         }
         break;
      default:
        HAL_LOG_ERROR( "%s: Unknown message ID = %d", __FUNCTION__, msg_hdr->msg_id );
        return;
    }
  }

  if( msg_hdr->txn_id != TXN_ID_NO_RESP_SIGNALLED ) {
    hal_signal_response( error, &g_sensor_control->cb_arrived_cond );
  }
}

/*===========================================================================

  FUNCTION:  hal_process_sam_sensor_thresh_resp

===========================================================================*/
/*!
*/
static void
hal_process_sam_sensor_thresh_resp( sensor1_msg_header_s *msg_hdr,
                                    void *msg_ptr )
{
  const sns_common_resp_s_v01*  crsp_ptr = (sns_common_resp_s_v01*) msg_ptr;
  bool                          error = false;

  if( crsp_ptr->sns_result_t != 0 )
  {
    HAL_LOG_ERROR( "%s: Msg %i; Result: %u, Error: %u", __FUNCTION__,
                   msg_hdr->msg_id, crsp_ptr->sns_result_t, crsp_ptr->sns_err_t );
    error = true;
  }

  if ( true != error )
  {
    switch( msg_hdr->service_number )
    {
      case SNS_SAM_SENSOR_THRESH_SVC_ID_V01:
      {
        switch( msg_hdr->msg_id )
        {
          case SNS_SAM_SENSOR_THRESH_ENABLE_RESP_V01:
            if(((sns_sam_sensor_thresh_enable_resp_msg_v01*) msg_ptr)->instance_id_valid)
            {
              g_sensor_control->sam_service[ SNS_SAM_SENSOR_THRESH_SVC_ID_V01 ].instance_id =
                ((sns_sam_sensor_thresh_enable_resp_msg_v01*) msg_ptr)->instance_id;
              HAL_LOG_DEBUG("%s: Instance ID of Sensor Threshold is %d", __FUNCTION__, g_sensor_control->sam_service[ SNS_SAM_SENSOR_THRESH_SVC_ID_V01 ].instance_id);
            }
            break;
          case SNS_SAM_SENSOR_THRESH_DISABLE_RESP_V01:
          case SNS_SAM_SENSOR_THRESH_CANCEL_RESP_V01:
            /* Reset instance ID */
            g_sensor_control->sam_service[ SNS_SAM_SENSOR_THRESH_SVC_ID_V01 ].instance_id = 0xFF;
            break;
          default:
            return;
        }
        break;
      }

      default:
        return;
    }
  }

  if( msg_hdr->txn_id != TXN_ID_NO_RESP_SIGNALLED ) {
    hal_signal_response( error, &g_sensor_control->cb_arrived_cond );
  }
}

/*===========================================================================

  FUNCTION:  hal_process_sam_rotation_vector_resp

===========================================================================*/
/*!
*/
static void
hal_process_sam_rotation_vector_resp( sensor1_msg_header_s *msg_hdr,
                                      void *msg_ptr )
{
  const sns_common_resp_s_v01*  crsp_ptr = (sns_common_resp_s_v01*) msg_ptr;
  bool                          error = false;

  HAL_LOG_DEBUG("%s: service_number=%d, msg_id=%d", __FUNCTION__, msg_hdr->service_number,  msg_hdr->msg_id );

  if( crsp_ptr->sns_result_t != 0 )
  {
    HAL_LOG_ERROR( "%s: Msg %i; Result: %u, Error: %u", __FUNCTION__,
                   msg_hdr->msg_id, crsp_ptr->sns_result_t, crsp_ptr->sns_err_t );
    error = true;

    if( msg_hdr->msg_id == SNS_SAM_ROTATION_VECTOR_BATCH_RESP_V01 &&
        ( crsp_ptr->sns_err_t == SENSOR1_EBAD_SVC_ID ||
          crsp_ptr->sns_err_t == SENSOR1_EUNKNOWN ) )
    {
      // Proceed if batching is not supported
      error = false;
    }
  }

  if( !error )
  {
    switch( msg_hdr->msg_id )
    {
      case SNS_SAM_ROTATION_VECTOR_ENABLE_RESP_V01:
        if(((sns_sam_rotation_vector_enable_resp_msg_v01*)msg_ptr)->instance_id_valid)
        {
          g_sensor_control->sam_service[ SNS_SAM_ROTATION_VECTOR_SVC_ID_V01 ].instance_id =
            ((sns_sam_rotation_vector_enable_resp_msg_v01*) msg_ptr)->instance_id;
        }
        break;
      case SNS_SAM_ROTATION_VECTOR_CANCEL_RESP_V01:
      case SNS_SAM_ROTATION_VECTOR_DISABLE_RESP_V01:
        /* Reset instance ID */
        g_sensor_control->sam_service[ SNS_SAM_ROTATION_VECTOR_SVC_ID_V01 ].instance_id = 0xFF;
        break;
      case SNS_SAM_ROTATION_VECTOR_BATCH_RESP_V01:
        HAL_LOG_DEBUG("%s: Received SNS_SAM_ROTATION_VECTOR_BATCH_RESP_V01", __FUNCTION__);
        if( ((sns_sam_rotation_vector_batch_resp_msg_v01*)msg_ptr)->max_batch_size_valid )
        {
          int max_samples = ((sns_sam_rotation_vector_batch_resp_msg_v01*)msg_ptr)->max_batch_size;
          if( hal_is_gyro_available() )
          {
            g_sensor_control->sensor_list[ HANDLE_ROTATION_VECTOR ].max_buffered_samples = max_samples;
            g_sensor_control->sensor_list[ HANDLE_ORIENTATION ].max_buffered_samples = max_samples;
          }
          else
          {
            HAL_LOG_ERROR("%s: Unexpected SNS_SAM_ROTATION_VECTOR_BATCH_RESP_V01", __FUNCTION__);
          }
        }

        pthread_mutex_unlock( &g_sensor_control->cb_mutex );
        pthread_mutex_lock( &g_sensor_control->data_mutex );
        hal_flush_send_cmplt( HANDLE_ROTATION_VECTOR );
        hal_flush_send_cmplt( HANDLE_ORIENTATION );
        pthread_mutex_unlock( &g_sensor_control->data_mutex );
        pthread_mutex_lock( &g_sensor_control->cb_mutex );
        break;

      case SNS_SAM_ROT_VEC_UPDATE_BATCH_PERIOD_RESP_V01:
        HAL_LOG_DEBUG("%s: Received SNS_SAM_ROT_VEC_UPDATE_BATCH_PERIOD_RESP_V01", __FUNCTION__);
        break;

      default:
        return;
    }
  }
  if( msg_hdr->txn_id != TXN_ID_NO_RESP_SIGNALLED ) {
    hal_signal_response( error, &g_sensor_control->cb_arrived_cond );
  }
}

/*===========================================================================

  FUNCTION:  hal_process_sam_orientation_resp

===========================================================================*/
/*!
*/
static void
hal_process_sam_orientation_resp( sensor1_msg_header_s *msg_hdr,
                                  void *msg_ptr )
{
  const sns_common_resp_s_v01*  crsp_ptr = (sns_common_resp_s_v01*) msg_ptr;
  bool                          error = false;

  HAL_LOG_DEBUG("%s: service_number=%d, msg_id=%d", __FUNCTION__, msg_hdr->service_number,  msg_hdr->msg_id );

  if( crsp_ptr->sns_result_t != 0 )
  {
    HAL_LOG_ERROR( "%s: Msg %i; Result: %u, Error: %u", __FUNCTION__,
                   msg_hdr->msg_id, crsp_ptr->sns_result_t, crsp_ptr->sns_err_t );
    error = true;

    if( msg_hdr->msg_id == SNS_SAM_ORIENTATION_BATCH_RESP_V01 &&
        ( crsp_ptr->sns_err_t != SENSOR1_SUCCESS ) )
    {
      // Proceed if batching is not supported
      error = false;
    }
  }

  if ( true != error )
  {
    switch( msg_hdr->msg_id )
    {
      case SNS_SAM_ORIENTATION_ENABLE_RESP_V01:
        if(((sns_sam_orientation_enable_resp_msg_v01*)msg_ptr)->instance_id_valid)
        {
          g_sensor_control->sam_service[ SNS_SAM_ORIENTATION_SVC_ID_V01 ].instance_id =
            ((sns_sam_orientation_enable_resp_msg_v01*) msg_ptr)->instance_id;
        }
        break;
      case SNS_SAM_ORIENTATION_CANCEL_RESP_V01:
      case SNS_SAM_ORIENTATION_DISABLE_RESP_V01:
        /* Reset instance ID */
        g_sensor_control->sam_service[ SNS_SAM_ORIENTATION_SVC_ID_V01 ].instance_id = 0xFF;
        break;
      case SNS_SAM_ORIENTATION_BATCH_RESP_V01:
        HAL_LOG_DEBUG("%s: Received SNS_SAM_ORIENTATION_BATCH_RESP_V01", __FUNCTION__);
        bool gyro_available = hal_is_gyro_available();
        if( ((sns_sam_orientation_batch_resp_msg_v01*)msg_ptr)->max_batch_size_valid )
        {
          int max_samples = ((sns_sam_orientation_batch_resp_msg_v01*)msg_ptr)->max_batch_size;
          g_sensor_control->sensor_list[ HANDLE_GEOMAGNETIC_ROTATION_VECTOR ].max_buffered_samples = max_samples;
          if( !gyro_available )
          {
            g_sensor_control->sensor_list[ HANDLE_GRAVITY ].max_buffered_samples = max_samples;
            g_sensor_control->sensor_list[ HANDLE_LINEAR_ACCEL ].max_buffered_samples = max_samples;
            g_sensor_control->sensor_list[ HANDLE_ROTATION_VECTOR ].max_buffered_samples = max_samples;
            g_sensor_control->sensor_list[ HANDLE_ORIENTATION ].max_buffered_samples = max_samples;
          }
        }

        pthread_mutex_unlock( &g_sensor_control->cb_mutex );
        pthread_mutex_lock( &g_sensor_control->data_mutex );
        hal_flush_send_cmplt( HANDLE_GEOMAGNETIC_ROTATION_VECTOR );
        if( !gyro_available )
        {
          hal_flush_send_cmplt( HANDLE_GRAVITY );
          hal_flush_send_cmplt( HANDLE_LINEAR_ACCEL );
          hal_flush_send_cmplt( HANDLE_ROTATION_VECTOR );
          hal_flush_send_cmplt( HANDLE_ORIENTATION );
        }
        pthread_mutex_unlock( &g_sensor_control->data_mutex );
        pthread_mutex_lock( &g_sensor_control->cb_mutex );

        break;

      case SNS_SAM_ORIENT_UPDATE_BATCH_PERIOD_RESP_V01:
        HAL_LOG_DEBUG("%s: Received SNS_SAM_ORIENT_UPDATE_BATCH_PERIOD_RESP_V01", __FUNCTION__);
        break;

      default:
        return;
    }
  }

  if( msg_hdr->txn_id != TXN_ID_NO_RESP_SIGNALLED ) {
    hal_signal_response( error, &g_sensor_control->cb_arrived_cond );
  }
}

/*===========================================================================

  FUNCTION:  hal_get_raw_data_mode

===========================================================================*/
/*!
*/
static bool
hal_get_raw_data_mode(int handle, hal_sensor_control_t* sensor_ctl)
{
  uint8_t *rdm_value = NULL,
          rdm_length = 0;
  int     rdm_error;
  bool    rv = false;

  rdm_error = sensor_reg_read( SNS_REG_ITEM_RAW_DATA_MODE_V02, 1,
                               &rdm_value, &rdm_length );
  if( SENSOR_REG_SUCCESS != rdm_error || NULL == rdm_value )
  {
    HAL_LOG_ERROR( "%s: Error in sensor_reg_read(): %i",
                   __FUNCTION__, rdm_error );
  }
  else
  {
    rv = ( *rdm_value == 1);
  }

  free( rdm_value );
  return rv;
}

/*===========================================================================

  FUNCTION:  hal_process_sam_gravity_resp

===========================================================================*/
/*!
*/
static void
hal_process_sam_gravity_resp( sensor1_msg_header_s *msg_hdr,
                              void *msg_ptr )
{
  const sns_common_resp_s_v01*  crsp_ptr = (sns_common_resp_s_v01*) msg_ptr;
  bool                          error = false;

  HAL_LOG_DEBUG("%s: %d", __FUNCTION__,  msg_hdr->msg_id );

  if( crsp_ptr->sns_result_t != 0 )
  {
    HAL_LOG_ERROR( "%s: Msg %i; Result: %u, Error: %u", __FUNCTION__,
                   msg_hdr->msg_id, crsp_ptr->sns_result_t, crsp_ptr->sns_err_t );
    error = true;

    if( msg_hdr->msg_id == SNS_SAM_GRAVITY_BATCH_RESP_V01 &&
        ( crsp_ptr->sns_err_t == SENSOR1_EBAD_SVC_ID ||
          crsp_ptr->sns_err_t == SENSOR1_EUNKNOWN ) )
    {
      // Proceed if batching is not supported
      error = false;
    }
  }

  if( !error )
  {
    switch( msg_hdr->msg_id )
    {
      case SNS_SAM_GRAVITY_ENABLE_RESP_V01:
        // setting both gravity and lin accel
        if(((sns_sam_gravity_enable_resp_msg_v01*) msg_ptr)->instance_id_valid)
        {
          g_sensor_control->sam_service[ SNS_SAM_GRAVITY_VECTOR_SVC_ID_V01 ].instance_id =
              ((sns_sam_gravity_enable_resp_msg_v01*) msg_ptr)->instance_id;
        }
        break;
      case SNS_SAM_GRAVITY_CANCEL_RESP_V01:
      case SNS_SAM_GRAVITY_DISABLE_RESP_V01:
        /* Reset instance ID */
        g_sensor_control->sam_service[ SNS_SAM_GRAVITY_VECTOR_SVC_ID_V01 ].instance_id = 0xFF;
        break;
      case SNS_SAM_GRAVITY_BATCH_RESP_V01:
        HAL_LOG_DEBUG("%s: Received SNS_SAM_GRAVITY_BATCH_RESP_V01", __FUNCTION__);
        if( ((sns_sam_gravity_batch_resp_msg_v01*)msg_ptr)->max_batch_size_valid )
        {
          int max_samples = ((sns_sam_gravity_batch_resp_msg_v01*)msg_ptr)->max_batch_size;
          if( hal_is_gyro_available() )
          {
            g_sensor_control->sensor_list[ HANDLE_GRAVITY ].max_buffered_samples = max_samples;
            g_sensor_control->sensor_list[ HANDLE_LINEAR_ACCEL ].max_buffered_samples = max_samples;
          }
          else
          {
            HAL_LOG_ERROR("%s: Unexpected SNS_SAM_GRAVITY_BATCH_RESP_V01", __FUNCTION__);
          }
        }

        pthread_mutex_unlock( &g_sensor_control->cb_mutex );
        pthread_mutex_lock( &g_sensor_control->data_mutex );
        hal_flush_send_cmplt( HANDLE_GRAVITY );
        hal_flush_send_cmplt( HANDLE_LINEAR_ACCEL );
        pthread_mutex_unlock( &g_sensor_control->data_mutex );
        pthread_mutex_lock( &g_sensor_control->cb_mutex );

        break;
      case SNS_SAM_GRAV_UPDATE_BATCH_PERIOD_RESP_V01:
        HAL_LOG_DEBUG("%s: Received SNS_SAM_GRAV_UPDATE_BATCH_PERIOD_RESP_V01", __FUNCTION__);
        break;
      default:
        return;
    }
  }

  if( msg_hdr->txn_id != TXN_ID_NO_RESP_SIGNALLED ) {
    hal_signal_response( error, &g_sensor_control->cb_arrived_cond );
  }
}

/*===========================================================================

  FUNCTION:  hal_process_sam_qmd_resp

   This function processes QMD response messages for adding/deleting a report.

===========================================================================*/
/*!
*/
static void
hal_process_sam_qmd_resp( sensor1_msg_header_s *msg_hdr,
                          sns_sam_qmd_enable_resp_msg_v01 *msg_ptr )
{
  pthread_mutex_t *mutex;
  bool error = true;

  HAL_LOG_DEBUG( "%s: Callback: QMD resp msg id %d, txn id %d",
                 __FUNCTION__, msg_hdr->msg_id, msg_hdr->txn_id );

  pthread_mutex_lock(  &g_sensor_control->cb_mutex );
  if( 0 != msg_ptr->resp.sns_result_t &&
      SNS_SAM_RMD_CANCEL_RESP_V01 != msg_hdr->msg_id &&
      SNS_SAM_VMD_CANCEL_RESP_V01 != msg_hdr->msg_id &&
      SNS_SAM_AMD_CANCEL_RESP_V01 != msg_hdr->msg_id )
  {
    HAL_LOG_ERROR( "%s: Msg %i; Result: %u, Error: %u", __FUNCTION__,
                   msg_hdr->msg_id, msg_ptr->resp.sns_result_t, msg_ptr->resp.sns_err_t );
  }
  else
  {
    if( SNS_SAM_RMD_ENABLE_RESP_V01 == msg_hdr->msg_id ||
        SNS_SAM_VMD_ENABLE_RESP_V01 == msg_hdr->msg_id ||
        SNS_SAM_AMD_ENABLE_RESP_V01 == msg_hdr->msg_id )
    {
      g_sensor_control->sam_service[ msg_hdr->service_number ].instance_id =
        ((sns_sam_qmd_enable_resp_msg_v01*) msg_ptr)->instance_id;
      error = false;
    }
    else if( SNS_SAM_RMD_DISABLE_RESP_V01 == msg_hdr->msg_id ||
             SNS_SAM_VMD_DISABLE_RESP_V01 == msg_hdr->msg_id ||
             SNS_SAM_AMD_DISABLE_RESP_V01 == msg_hdr->msg_id ||
             SNS_SAM_RMD_CANCEL_RESP_V01 == msg_hdr->msg_id ||
             SNS_SAM_VMD_CANCEL_RESP_V01 == msg_hdr->msg_id ||
             SNS_SAM_AMD_CANCEL_RESP_V01 == msg_hdr->msg_id )
    {
      g_sensor_control->sam_service[ msg_hdr->service_number ].instance_id = 0xFF;  /* Reset instance ID */
      error = false;
    }
  }

  if( msg_hdr->txn_id != TXN_ID_NO_RESP_SIGNALLED ) {
    hal_signal_response( error, &g_sensor_control->cb_arrived_cond );
  }

  pthread_mutex_unlock( &g_sensor_control->cb_mutex );
}

/*===========================================================================

  FUNCTION:  hal_process_sam_smd_resp

===========================================================================*/
/*!
*/
static void
hal_process_sam_smd_resp( sensor1_msg_header_s *msg_hdr,
                          void *msg_ptr )
{
  sns_sam_smd_enable_resp_msg_v01* rsp_ptr = (sns_sam_smd_enable_resp_msg_v01*) msg_ptr;
  bool                          error = false;

  if( 0 != rsp_ptr->resp.sns_result_t &&
     SNS_SAM_SMD_CANCEL_RESP_V01 != msg_hdr->msg_id )
  {
    HAL_LOG_ERROR( "%s: Msg %i; Result: %u, Error: %u", __FUNCTION__,
                   msg_hdr->msg_id, rsp_ptr->resp.sns_result_t, rsp_ptr->resp.sns_err_t );

    /* TODO: workaround: SMD response always returns error 8 although the algo is enabled
    successfully. For now just ignore this error until we find the root cause */
    if (rsp_ptr->resp.sns_err_t != 8)
      error = true;

    // Disregard SMD failures when algo is already disabled; avoid delete-retry cycles
    if( SNS_SAM_SMD_DISABLE_RESP_V01 == msg_hdr->msg_id &&
        SENSOR1_ENOTALLOWED == rsp_ptr->resp.sns_err_t )
    {
      error = false;
    }
  }

  if ( true != error )
  {
    switch( msg_hdr->service_number )
    {
      case SNS_SAM_SMD_SVC_ID_V01:
      {
        switch( msg_hdr->msg_id )
        {
          case SNS_SAM_SMD_ENABLE_RESP_V01:
            g_sensor_control->sam_service[ SNS_SAM_SMD_SVC_ID_V01 ].instance_id =
              ((sns_sam_smd_enable_resp_msg_v01*) msg_ptr)->instance_id;
            break;
          case SNS_SAM_SMD_DISABLE_RESP_V01:
          case SNS_SAM_SMD_CANCEL_RESP_V01:
            /* Reset instance ID */
            g_sensor_control->sam_service[ SNS_SAM_SMD_SVC_ID_V01 ].instance_id = 0xFF;
            if( msg_hdr->txn_id == TXN_ID_NO_RESP_SIGNALLED )
            {
              /* This disable response is because SMD was "auto-disabled" due to receiving
               * an SMD indication. Don't signal a response here, since the HAL isn't
               * expecting one for this case */
              HAL_LOG_VERBOSE("%s: SMD disable response. SMD auto-disabled due to indication",
                              __FUNCTION__ );
              return;
            }
            HAL_LOG_VERBOSE("%s: SMD disable response. SMD disabled due to HAL command",
                            __FUNCTION__ );
            break;
          default:
            return;
        }
        break;
      }
      default:
        return;
    }
  }

  if( msg_hdr->txn_id != TXN_ID_NO_RESP_SIGNALLED ) {
    hal_signal_response( error, &g_sensor_control->cb_arrived_cond );
  }
}

/*===========================================================================

  FUNCTION:  hal_process_sam_cmc_resp

===========================================================================*/
/*!
*/
static void
hal_process_sam_cmc_resp( sensor1_msg_header_s *msg_hdr,
                          void *msg_ptr )
{
  const sns_common_resp_s_v01*  crsp_ptr = (sns_common_resp_s_v01*) msg_ptr;
  bool                          error = false;

  HAL_LOG_DEBUG("%s: %d", __FUNCTION__,  msg_hdr->msg_id );

  if( crsp_ptr->sns_result_t != 0 &&
      msg_hdr->msg_id != SNS_SAM_CMC_CANCEL_RESP_V01)
  {
    HAL_LOG_ERROR( "%s: Msg %i; Result: %u, Error: %u", __FUNCTION__,
                   msg_hdr->msg_id, crsp_ptr->sns_result_t, crsp_ptr->sns_err_t );
  }

  if( true != error )
  {
    switch( msg_hdr->msg_id )
    {
      case SNS_SAM_CMC_ENABLE_RESP_V01:
        g_sensor_control->sam_service[ SNS_SAM_CMC_SVC_ID_V01 ].instance_id =
          ((sns_sam_cmc_enable_resp_msg_v01*) msg_ptr)->instance_id;
        break;
      case SNS_SAM_CMC_CANCEL_RESP_V01:
        /* Reset instance ID */
        g_sensor_control->sam_service[ SNS_SAM_CMC_SVC_ID_V01 ].instance_id = 0xFF;
        break;
      default:
        HAL_LOG_ERROR("%s: Unknown msg id: %d", __FUNCTION__, msg_hdr->msg_id );
        return;
    }
  }

  if( msg_hdr->txn_id != TXN_ID_NO_RESP_SIGNALLED ) {
    hal_signal_response( error, &g_sensor_control->cb_arrived_cond );
  }
}

/*===========================================================================

  FUNCTION:  hal_process_sam_ped_resp

===========================================================================*/
/*!
*/
static void
hal_process_sam_ped_resp( sensor1_msg_header_s *msg_hdr,
                          void *msg_ptr )
{
  const sns_common_resp_s_v01*  crsp_ptr = (sns_common_resp_s_v01*) msg_ptr;
  bool                          error = false;

  if( crsp_ptr->sns_result_t != 0 )
  {
    HAL_LOG_ERROR( "%s: Msg %i; Result: %u, Error: %u", __FUNCTION__,
                   msg_hdr->msg_id, crsp_ptr->sns_result_t, crsp_ptr->sns_err_t );
    error = true;

    if( msg_hdr->msg_id == SNS_SAM_PED_BATCH_RESP_V01 &&
        ( crsp_ptr->sns_err_t == SENSOR1_EBAD_SVC_ID ||
          crsp_ptr->sns_err_t == SENSOR1_EUNKNOWN ) )
    {
      // Proceed if batching is not supported
      error = false;
    }
  }

  if( SNS_SAM_PED_CANCEL_RESP_V01 == msg_hdr->msg_id ||
      SNS_SAM_PED_DISABLE_RESP_V01 == msg_hdr->msg_id )
  {
    /* Note, it's possible that we continue to receive/process indications after this.
     * However, they will update the running_instance, and should not be lost in
     * subsequent iterations */
    g_sensor_control->step_counter_running_total = g_sensor_control->step_counter_running_instance;
  }

  if( true != error )
  {
    switch( msg_hdr->msg_id )
    {
      case SNS_SAM_PED_ENABLE_RESP_V01:
        g_sensor_control->sam_service[ SNS_SAM_PED_SVC_ID_V01 ].instance_id =
          ((sns_sam_ped_enable_resp_msg_v01*) msg_ptr)->instance_id;
        break;
      case SNS_SAM_PED_GET_REPORT_RESP_V01:
        hal_sam_parse_ind( g_sensor_control, msg_hdr, msg_ptr );
        break;
      case SNS_SAM_PED_CANCEL_RESP_V01:
      case SNS_SAM_PED_DISABLE_RESP_V01:
        /* Reset instance ID */
        g_sensor_control->sam_service[ SNS_SAM_PED_SVC_ID_V01 ].instance_id = 0xFF;
        break;
      case SNS_SAM_PED_BATCH_RESP_V01:
        if( ((sns_sam_ped_batch_resp_msg_v01*)msg_ptr)->max_batch_size_valid )
        {
          int max_samples = ((sns_sam_ped_batch_resp_msg_v01*)msg_ptr)->max_batch_size;
          g_sensor_control->sensor_list[ HANDLE_SAM_STEP_DETECTOR ].max_buffered_samples = max_samples;
        }

        pthread_mutex_unlock( &g_sensor_control->cb_mutex );
        pthread_mutex_lock( &g_sensor_control->data_mutex );
        hal_flush_send_cmplt( HANDLE_SAM_STEP_DETECTOR );
        pthread_mutex_unlock( &g_sensor_control->data_mutex );
        pthread_mutex_lock( &g_sensor_control->cb_mutex );

        break;
      case SNS_SAM_PED_UPDATE_BATCH_PERIOD_RESP_V01:
        HAL_LOG_DEBUG("%s: Received SNS_SAM_PED_UPDATE_BATCH_PERIOD_RESP_V01", __FUNCTION__);
        break;
      default:
        HAL_LOG_ERROR("%s: Unknown msg id: %d", __FUNCTION__, msg_hdr->msg_id );
        return;
    }
  }

  if( msg_hdr->txn_id != TXN_ID_NO_RESP_SIGNALLED ) {
    hal_signal_response( error, &g_sensor_control->cb_arrived_cond );
  }
}

/*===========================================================================

  FUNCTION:  hal_process_sam_pam_resp

===========================================================================*/
/*!
*/
static void
hal_process_sam_pam_resp( sensor1_msg_header_s *msg_hdr,
                          void *msg_ptr )
{
  const sns_common_resp_s_v01*  crsp_ptr = (sns_common_resp_s_v01*) msg_ptr;
  bool                          error = false;

  HAL_LOG_DEBUG("%s: %d", __FUNCTION__,  msg_hdr->msg_id );

  if( crsp_ptr->sns_result_t != 0 &&
      msg_hdr->msg_id != SNS_SAM_PAM_CANCEL_RESP_V01 )
  {
    HAL_LOG_ERROR( "%s: Msg %i; Result: %u, Error: %u", __FUNCTION__,
                   msg_hdr->msg_id, crsp_ptr->sns_result_t, crsp_ptr->sns_err_t );
    error = true;
  }

  if ( true != error )
  {
    switch( msg_hdr->msg_id )
    {
      case SNS_SAM_PAM_ENABLE_RESP_V01:
        g_sensor_control->sam_service[ SNS_SAM_PAM_SVC_ID_V01 ].instance_id =
          ((sns_sam_pam_enable_resp_msg_v01*) msg_ptr)->instance_id;
        break;
      case SNS_SAM_PAM_CANCEL_RESP_V01:
      case SNS_SAM_PAM_DISABLE_RESP_V01:
        /* Reset instance ID */
        g_sensor_control->sam_service[ SNS_SAM_PAM_SVC_ID_V01 ].instance_id = 0xFF;
        break;
      default:
        return;
    }
  }

  if( msg_hdr->txn_id != TXN_ID_NO_RESP_SIGNALLED ) {
    hal_signal_response( error, &g_sensor_control->cb_arrived_cond );
  }
}

/*===========================================================================

  FUNCTION:  hal_process_sam_game_rotation_vector_resp

===========================================================================*/
/*!
*/
static void
hal_process_sam_game_rotation_vector_resp( sensor1_msg_header_s *msg_hdr,
                                           void *msg_ptr )
{
  const sns_common_resp_s_v01*  crsp_ptr = (sns_common_resp_s_v01*) msg_ptr;
  bool                          error = false;

  HAL_LOG_DEBUG("%s: service_number=%d, msg_id=%d", __FUNCTION__, msg_hdr->service_number,  msg_hdr->msg_id );

  if( crsp_ptr->sns_result_t != 0 )
  {
    HAL_LOG_ERROR( "%s: Msg %i; Result: %u, Error: %u", __FUNCTION__,
                   msg_hdr->msg_id, crsp_ptr->sns_result_t, crsp_ptr->sns_err_t );

    error = true;

    if( msg_hdr->msg_id == SNS_SAM_GAME_ROTATION_VECTOR_BATCH_RESP_V01 &&
        ( crsp_ptr->sns_err_t == SENSOR1_EBAD_SVC_ID ||
          crsp_ptr->sns_err_t == SENSOR1_EUNKNOWN ) )
    {
      // Proceed if batching is not supported
      error = false;
    }
  }
  else
  {
    switch( msg_hdr->msg_id )
    {
      case SNS_SAM_GAME_ROTATION_VECTOR_ENABLE_RESP_V01:
      {
        int instance_id = -2;
        if(((sns_sam_game_rotation_vector_enable_resp_msg_v01*)msg_ptr)->instance_id_valid)
        {
          instance_id = ((sns_sam_game_rotation_vector_enable_resp_msg_v01*) msg_ptr)->instance_id;
          g_sensor_control->sam_service[ SNS_SAM_GAME_ROTATION_VECTOR_SVC_ID_V01 ].instance_id = instance_id;
          HAL_LOG_DEBUG( "%s: Received SNS_SAM_GAME_ROTATION_VECTOR_ENABLE_RESP_V01 (instance id %d)",
                         __FUNCTION__, instance_id);
        }
        break;
      }
      case SNS_SAM_GAME_ROTATION_VECTOR_BATCH_RESP_V01:
        HAL_LOG_DEBUG("%s: Received SNS_SAM_GAME_ROTATION_VECTOR_BATCH_RESP_V01", __FUNCTION__);
        if( ((sns_sam_game_rotation_vector_batch_resp_msg_v01*)msg_ptr)->max_batch_size_valid )
        {
          int max_samples = ((sns_sam_game_rotation_vector_batch_resp_msg_v01*)msg_ptr)->max_batch_size;
          g_sensor_control->sensor_list[ HANDLE_GAME_ROTATION_VECTOR ].max_buffered_samples = max_samples;
        }

        pthread_mutex_unlock( &g_sensor_control->cb_mutex );
        pthread_mutex_lock( &g_sensor_control->data_mutex );
        hal_flush_send_cmplt( HANDLE_GAME_ROTATION_VECTOR );
        pthread_mutex_unlock( &g_sensor_control->data_mutex );
        pthread_mutex_lock( &g_sensor_control->cb_mutex );

        break;
      case SNS_SAM_GAME_ROTATION_VECTOR_CANCEL_RESP_V01:
      case SNS_SAM_GAME_ROTATION_VECTOR_DISABLE_RESP_V01:
        /* Reset instance ID */
        g_sensor_control->sam_service[ SNS_SAM_GAME_ROTATION_VECTOR_SVC_ID_V01 ].instance_id = 0xFF;
        break;
      case SNS_SAM_GAME_RV_UPDATE_BATCH_PERIOD_RESP_V01:
        HAL_LOG_DEBUG("%s: Received SNS_SAM_GAME_RV_UPDATE_BATCH_PERIOD_RESP_V01", __FUNCTION__);
        break;
      default:
        HAL_LOG_ERROR("%s: Unknown message ID %d for game rotation vector service",
                      __FUNCTION__, msg_hdr->msg_id);
        return;
    }
  }

  if( msg_hdr->txn_id != TXN_ID_NO_RESP_SIGNALLED ) {
    hal_signal_response( error, &g_sensor_control->cb_arrived_cond );
  }
}

#ifdef  FEATURE_SNS_HAL_SAM_INT
/*===========================================================================

  FUNCTION:  hal_process_sam_bte_resp

===========================================================================*/
/*!
*/
static void
hal_process_sam_bte_resp( sensor1_msg_header_s *msg_hdr,
                          void *msg_ptr )
{
  const sns_common_resp_s_v01*  crsp_ptr = (sns_common_resp_s_v01*) msg_ptr;
  bool                          error = false;

  if( crsp_ptr->sns_result_t != 0 &&
      msg_hdr->msg_id != SNS_SAM_BTE_CANCEL_RESP_V01 )
  {
    HAL_LOG_ERROR( "%s: Msg %i; Result: %u, Error: %u", __FUNCTION__,
                   msg_hdr->msg_id, crsp_ptr->sns_result_t, crsp_ptr->sns_err_t );
    error = true;
  }

  if ( true != error )
  {
    switch( msg_hdr->msg_id )
    {
      /* BTE Enable Responses */
      case SNS_SAM_BTE_ENABLE_RESP_V01:
        g_sensor_control->sam_service[ SNS_SAM_BTE_SVC_ID_V01 ].instance_id =
          ((sns_sam_bte_enable_resp_msg_v01*) msg_ptr)->instance_id;
        break;

      /* BTE Cancel Responses */
      case SNS_SAM_BTE_CANCEL_RESP_V01:
        /* Reset instance ID */
        g_sensor_control->sam_service[ SNS_SAM_BTE_SVC_ID_V01 ].instance_id = 0xFF;
        break;

      default:
        return;
    }
  }

  if( msg_hdr->txn_id != TXN_ID_NO_RESP_SIGNALLED ) {
    hal_signal_response( error, &g_sensor_control->cb_arrived_cond );
  }
}

/*===========================================================================

  FUNCTION:  hal_process_sam_fns_resp

===========================================================================*/
/*!
*/
static void
hal_process_sam_fns_resp( sensor1_msg_header_s *msg_hdr,
                          void *msg_ptr )
{
  const sns_common_resp_s_v01*  crsp_ptr = (sns_common_resp_s_v01*) msg_ptr;
  bool                          error = false;

  if( crsp_ptr->sns_result_t != 0 &&
      msg_hdr->msg_id != SNS_SAM_FNS_CANCEL_RESP_V01 )
  {
    HAL_LOG_ERROR( "%s: Msg %i; Result: %u, Error: %u", __FUNCTION__,
                   msg_hdr->msg_id, crsp_ptr->sns_result_t, crsp_ptr->sns_err_t );
    error = true;
  }

  if ( true != error )
  {
    switch( msg_hdr->msg_id )
    {
      /* FNS Enable Responses */
      case SNS_SAM_FNS_ENABLE_RESP_V01:
        g_sensor_control->sam_service[ SNS_SAM_FNS_SVC_ID_V01 ].instance_id =
          ((sns_sam_fns_enable_resp_msg_v01*) msg_ptr)->instance_id;
        break;

      /* FNS Cancel Responses */
      case SNS_SAM_FNS_CANCEL_RESP_V01:
        /* Reset instance ID */
        g_sensor_control->sam_service[ SNS_SAM_FNS_SVC_ID_V01 ].instance_id = 0xFF;
        break;

      default:
        return;
    }
  }

  if( msg_hdr->txn_id != TXN_ID_NO_RESP_SIGNALLED ) {
    hal_signal_response( error, &g_sensor_control->cb_arrived_cond );
  }
}
#endif /* FEATURE_SNS_HAL_SAM_INT */

/*===========================================================================

  FUNCTION:  hal_process_sam_basic_gestures_resp

===========================================================================*/
/*!
*/
static void
hal_process_sam_basic_gestures_resp( sensor1_msg_header_s *msg_hdr,
                          void *msg_ptr )
{
  const sns_common_resp_s_v01*  crsp_ptr = (sns_common_resp_s_v01*) msg_ptr;
  bool                          error = false;

  HAL_LOG_DEBUG("%s: msg_id=%d", __FUNCTION__,  msg_hdr->msg_id );

  if( crsp_ptr->sns_result_t != 0 &&
      msg_hdr->msg_id != SNS_SAM_BASIC_GESTURES_CANCEL_RESP_V01 )
  {
    HAL_LOG_ERROR( "%s: Msg %i; Result: %u, Error: %u", __FUNCTION__,
                   msg_hdr->msg_id, crsp_ptr->sns_result_t, crsp_ptr->sns_err_t );
    error = true;
  }

  if ( true != error )
  {
    switch( msg_hdr->msg_id )
    {
      /* Enable Responses */
      case SNS_SAM_BASIC_GESTURES_ENABLE_RESP_V01:
        g_sensor_control->sam_service[ SNS_SAM_BASIC_GESTURES_SVC_ID_V01 ].instance_id =
          ((sns_sam_basic_gestures_enable_resp_msg_v01*) msg_ptr)->instance_id;
        break;

      /* Cancel Responses */
      case SNS_SAM_BASIC_GESTURES_CANCEL_RESP_V01:
        /* Reset instance ID */
        g_sensor_control->sam_service[ SNS_SAM_BASIC_GESTURES_SVC_ID_V01 ].instance_id = 0xFF;
        break;

      default:
        return;
    }
  }

  if( msg_hdr->txn_id != TXN_ID_NO_RESP_SIGNALLED ) {
    hal_signal_response( error, &g_sensor_control->cb_arrived_cond );
  }
}

/*===========================================================================

  FUNCTION:  hal_process_sam_tap_resp

===========================================================================*/
/*!
*/
static void
hal_process_sam_tap_resp( sensor1_msg_header_s *msg_hdr,
                          void *msg_ptr )
{
  const sns_common_resp_s_v01*  crsp_ptr = (sns_common_resp_s_v01*) msg_ptr;
  bool                          error = false;

  HAL_LOG_DEBUG("%s: msg_id=%d", __FUNCTION__,  msg_hdr->msg_id );

  if( crsp_ptr->sns_result_t != 0 &&
      msg_hdr->msg_id != SNS_SAM_TAP_CANCEL_RESP_V01 )
  {
    HAL_LOG_ERROR( "%s: Msg %i; Result: %u, Error: %u", __FUNCTION__,
                   msg_hdr->msg_id, crsp_ptr->sns_result_t, crsp_ptr->sns_err_t );
    error = true;
  }

  if ( true != error )
  {
    switch( msg_hdr->msg_id )
    {
      /* Enable Responses */
      case SNS_SAM_TAP_ENABLE_RESP_V01:
        g_sensor_control->sam_service[ SNS_SAM_TAP_SVC_ID_V01 ].instance_id =
          ((sns_sam_tap_enable_resp_msg_v01*) msg_ptr)->instance_id;
        break;

      /* Cancel Responses */
      case SNS_SAM_TAP_CANCEL_RESP_V01:
        /* Reset instance ID */
        g_sensor_control->sam_service[ SNS_SAM_TAP_SVC_ID_V01 ].instance_id = 0xFF;
        break;

      default:
        return;
    }
  }

  if( msg_hdr->txn_id != TXN_ID_NO_RESP_SIGNALLED ) {
    hal_signal_response( error, &g_sensor_control->cb_arrived_cond );
  }
}

/*===========================================================================

  FUNCTION:  hal_process_sam_gyro_tap_resp

===========================================================================*/
/*!
*/
static void
hal_process_sam_gyro_tap_resp( sensor1_msg_header_s *msg_hdr,
                               void *msg_ptr )
{
  const sns_common_resp_s_v01*  crsp_ptr = (sns_common_resp_s_v01*) msg_ptr;
  bool                          error = false;

  HAL_LOG_DEBUG("%s: msg_id=%d", __FUNCTION__,  msg_hdr->msg_id );

  if( crsp_ptr->sns_result_t != 0 &&
      msg_hdr->msg_id != SNS_SAM_GYRO_TAP2_CANCEL_RESP_V01 )
  {
    HAL_LOG_ERROR( "%s: Msg %i; Result: %u, Error: %u", __FUNCTION__,
                   msg_hdr->msg_id, crsp_ptr->sns_result_t, crsp_ptr->sns_err_t );
    error = true;
  }

  if ( true != error )
  {
    switch( msg_hdr->msg_id )
    {
      /* Enable Responses */
      case SNS_SAM_GYRO_TAP2_ENABLE_RESP_V01:
      g_sensor_control->sam_service[ SNS_SAM_GYRO_TAP2_SVC_ID_V01 ].instance_id =
               ((sns_sam_gyro_tap2_enable_resp_msg_v01*) msg_ptr)->instance_id;
      break;

      /* Cancel Responses */
      case SNS_SAM_GYRO_TAP2_CANCEL_RESP_V01:
      /* Reset instance ID */
      g_sensor_control->sam_service[ SNS_SAM_GYRO_TAP2_SVC_ID_V01 ].instance_id = 0xFF;
      break;

      default:
        error = true;
        return;
    }
  }

  if( msg_hdr->txn_id != TXN_ID_NO_RESP_SIGNALLED ) {
    hal_signal_response( error, &g_sensor_control->cb_arrived_cond );
  }
}

/*===========================================================================

  FUNCTION:  hal_process_sam_facing_resp

===========================================================================*/
/*!
*/
static void
hal_process_sam_facing_resp( sensor1_msg_header_s *msg_hdr,
                          void *msg_ptr )
{
  const sns_common_resp_s_v01*  crsp_ptr = (sns_common_resp_s_v01*) msg_ptr;
  bool                          error = false;

  HAL_LOG_DEBUG("%s: msg_id=%d", __FUNCTION__,  msg_hdr->msg_id );

  if( crsp_ptr->sns_result_t != 0 &&
      msg_hdr->msg_id != SNS_SAM_FACING_CANCEL_RESP_V01 )
  {
    HAL_LOG_ERROR( "%s: Msg %i; Result: %u, Error: %u", __FUNCTION__,
                   msg_hdr->msg_id, crsp_ptr->sns_result_t, crsp_ptr->sns_err_t );
    error = true;
  }

  if ( true != error )
  {
    switch( msg_hdr->msg_id )
    {
      /* Enable Responses */
      case SNS_SAM_FACING_ENABLE_RESP_V01:
        g_sensor_control->sam_service[ SNS_SAM_FACING_SVC_ID_V01 ].instance_id =
          ((sns_sam_facing_enable_resp_msg_v01*) msg_ptr)->instance_id;
        break;

      /* Cancel Responses */
      case SNS_SAM_FACING_CANCEL_RESP_V01:
        /* Reset instance ID */
        g_sensor_control->sam_service[ SNS_SAM_FACING_SVC_ID_V01 ].instance_id = 0xFF;
        break;

      default:
        return;
    }
  }

  if( msg_hdr->txn_id != TXN_ID_NO_RESP_SIGNALLED ) {
    hal_signal_response( error, &g_sensor_control->cb_arrived_cond );
  }
}

/*===========================================================================

  FUNCTION:  hal_process_sam_tilt_resp

===========================================================================*/
/*!
*/
static void
hal_process_sam_tilt_resp( sensor1_msg_header_s *msg_hdr,
                          void *msg_ptr )
{
  const sns_common_resp_s_v01*  crsp_ptr = (sns_common_resp_s_v01*) msg_ptr;
  bool                          error = false;

  HAL_LOG_DEBUG("%s: msg_id=%d", __FUNCTION__,  msg_hdr->msg_id );

  if( crsp_ptr->sns_result_t != 0 &&
      msg_hdr->msg_id != SNS_SAM_INTEG_ANGLE_CANCEL_RESP_V01 )
  {
    HAL_LOG_ERROR( "%s: Msg %i; Result: %u, Error: %u", __FUNCTION__,
                   msg_hdr->msg_id, crsp_ptr->sns_result_t, crsp_ptr->sns_err_t );
    error = true;
  }

  if ( true != error )
  {
    switch( msg_hdr->msg_id )
    {
      /* Enable Responses */
      case SNS_SAM_INTEG_ANGLE_ENABLE_RESP_V01:
        g_sensor_control->sam_service[ SNS_SAM_INTEG_ANGLE_SVC_ID_V01 ].instance_id =
          ((sns_sam_integ_angle_enable_resp_msg_v01*) msg_ptr)->instance_id;
        break;

      /* Cancel Responses */
      case SNS_SAM_INTEG_ANGLE_CANCEL_RESP_V01:
        /* Reset instance ID */
        g_sensor_control->sam_service[ SNS_SAM_INTEG_ANGLE_SVC_ID_V01 ].instance_id = 0xFF;
        break;

      default:
        return;
    }
  }

  if( msg_hdr->txn_id != TXN_ID_NO_RESP_SIGNALLED ) {
    hal_signal_response( error, &g_sensor_control->cb_arrived_cond );
  }
}

/*===========================================================================

  FUNCTION:  hal_process_sam_mag_cal_ind

===========================================================================*/
/*!
*/
static void
hal_process_sam_mag_cal_ind( sensor1_msg_header_s *msg_hdr,
                                void *msg_ptr )
{
  sns_sam_mag_cal_report_ind_msg_v01*  sam_mag_cal_rpt_ptr =
                                     (sns_sam_mag_cal_report_ind_msg_v01*) msg_ptr;
  bool             error = false;
  uint32_t         timestamp = 0;
  sensors_event_t  sensor_data;

  switch( msg_hdr->service_number )
  {
    case SNS_SAM_MAG_CAL_SVC_ID_V01:
    {
      switch( msg_hdr->msg_id )
      {
        case SNS_SAM_MAG_CAL_REPORT_IND_V01:
          sensor_data.type = SENSOR_TYPE_MAGNETIC_FIELD;
          sensor_data.sensor = HANDLE_MAGNETIC_FIELD_SAM;
          /* Convert from SAE to Android co-ordinates and scale
            x' = y; y' = x; z' = -z;                                        */
          sensor_data.magnetic.x = (float)(sam_mag_cal_rpt_ptr->result.m[1] * UNIT_CONVERT_MAGNETIC_FIELD);
          sensor_data.magnetic.y = (float)(sam_mag_cal_rpt_ptr->result.m[0] * UNIT_CONVERT_MAGNETIC_FIELD);
          sensor_data.magnetic.z = (float)(-sam_mag_cal_rpt_ptr->result.m[2] * UNIT_CONVERT_MAGNETIC_FIELD);

          /* Convert from SAE to Android co-ordinates and scale
            x' = y; y' = x; z' = -z;                                        */
          g_sensor_control->mag_cal_cur_sample.sample.x_bias = (float)(sam_mag_cal_rpt_ptr->result.b[1] * UNIT_CONVERT_MAGNETIC_FIELD);
          g_sensor_control->mag_cal_cur_sample.sample.y_bias = (float)(sam_mag_cal_rpt_ptr->result.b[0] * UNIT_CONVERT_MAGNETIC_FIELD);
          g_sensor_control->mag_cal_cur_sample.sample.z_bias = (float)(-sam_mag_cal_rpt_ptr->result.b[2] * UNIT_CONVERT_MAGNETIC_FIELD);
          g_sensor_control->mag_cal_cur_sample.sam_ts = sam_mag_cal_rpt_ptr->timestamp;

          HAL_LOG_VERBOSE("%s: Mag X: %f Mag Y: %f Mag Z: %f ", __FUNCTION__,
                          sensor_data.magnetic.x,
                          sensor_data.magnetic.y,
                          sensor_data.magnetic.z);

          HAL_LOG_VERBOSE("%s: Bias X: %f Bias Y: %f Bias Z: %f ", __FUNCTION__,
                          (float)sam_mag_cal_rpt_ptr->result.b[1],
                          (float)sam_mag_cal_rpt_ptr->result.b[0],
                          -(float)sam_mag_cal_rpt_ptr->result.b[2]);

          sensor_data.magnetic.status = sam_mag_cal_rpt_ptr->result.accuracy;

          timestamp = sam_mag_cal_rpt_ptr->timestamp;

          break;
        case SNS_SAM_MAG_CAL_ERROR_IND_V01:
          error = true;
          break;
        default:
          error = true;
          break;
      }
      break;
    }
    default:
      error = true;
      break;
  }

  if ( error == false ) /* No error */
  {
    /* Check instance ID to make sure it matches sensor handle */
    if ( sam_mag_cal_rpt_ptr->instance_id ==
           g_sensor_control->sam_service[ SNS_SAM_MAG_CAL_SVC_ID_V01 ].instance_id )
    {
      sensor_data.version = sizeof(sensors_event_t);

      sensor_data.timestamp = hal_timestamp_calc((uint64_t)timestamp, sensor_data.sensor );
      if( (g_sensor_control->active_sensors) & (1ULL << HANDLE_MAGNETIC_FIELD_SAM) )
      {
        if( hal_insert_queue( &sensor_data ) )
        {
          hal_signal_ind( &g_sensor_control->data_arrived_cond );
        }
      }

      if( (g_sensor_control->active_sensors) & (1ULL << HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM) )
      {
        HAL_LOG_VERBOSE("%s: smgr_ts: %u sam_ts: %u apps_ts: %lld", __FUNCTION__,
                        g_sensor_control->mag_cal_cur_sample.smgr_ts,
                        g_sensor_control->mag_cal_cur_sample.sam_ts,
                        sensor_data.timestamp );

        if(g_sensor_control->mag_cal_cur_sample.smgr_ts ==
           g_sensor_control->mag_cal_cur_sample.sam_ts)
        {
          sensor_data.type = SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED;
          sensor_data.sensor = HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM;
          sensor_data.uncalibrated_magnetic = g_sensor_control->mag_cal_cur_sample.sample;

          if( hal_insert_queue( &sensor_data ) )
          {
            hal_signal_ind( &g_sensor_control->data_arrived_cond );
          }
        }
      }
    }
    else
    {
      HAL_LOG_ERROR("%s: Instance Id mismatch id=%d id=%d",__FUNCTION__,
                                                  sam_mag_cal_rpt_ptr->instance_id,
                                                  g_sensor_control->sam_service[ SNS_SAM_MAG_CAL_SVC_ID_V01 ].instance_id);
    }
  }
  else
  {
    HAL_LOG_ERROR("%s: Logged Error", __FUNCTION__);
  }
}

/*===========================================================================

  FUNCTION:  hal_process_sam_qmd_ind

===========================================================================*/
/*!
*/
static void
hal_process_sam_qmd_ind( sensor1_msg_header_s *msg_hdr,
                         void *msg_ptr )
{
  sns_sam_qmd_report_ind_msg_v01*  sam_qmd_rpt_ptr =
                                     (sns_sam_qmd_report_ind_msg_v01*) msg_ptr;
  bool             error = false;
  uint32_t         timestamp;
  sensors_event_t  sensor_data;

  switch( msg_hdr->service_number )
  {
    case SNS_SAM_AMD_SVC_ID_V01:
    {
      switch( msg_hdr->msg_id )
      {
        case SNS_SAM_AMD_REPORT_IND_V01:
          sensor_data.type = SENSOR_TYPE_MOTION_ABSOLUTE;
          sensor_data.sensor = HANDLE_MOTION_ABSOLUTE;
          sensor_data.data[0] = sam_qmd_rpt_ptr->state;
          HAL_LOG_VERBOSE("%s: sensor %d, motion %f", __FUNCTION__, sensor_data.type, sensor_data.data[0]);
          timestamp = sam_qmd_rpt_ptr->timestamp;
          break;
        case SNS_SAM_AMD_ERROR_IND_V01:
          error = true;
          break;
        default:
          error = true;
          break;
      }
      break;
    }
    case SNS_SAM_RMD_SVC_ID_V01:
    {
      switch( msg_hdr->msg_id )
      {
        case SNS_SAM_RMD_REPORT_IND_V01:
          sensor_data.type = SENSOR_TYPE_MOTION_RELATIVE;
          sensor_data.sensor = HANDLE_MOTION_RELATIVE;
          sensor_data.data[0] = sam_qmd_rpt_ptr->state;
          timestamp = sam_qmd_rpt_ptr->timestamp;
          HAL_LOG_VERBOSE("%s: sensor %d, motion %f", __FUNCTION__, sensor_data.type, sensor_data.data[0]);
          break;
        case SNS_SAM_RMD_ERROR_IND_V01:
          error = true;
          break;
        default:
          error = true;
          break;
      }
      break;
    }
    case SNS_SAM_VMD_SVC_ID_V01:
    {
      switch( msg_hdr->msg_id )
      {
        case SNS_SAM_VMD_REPORT_IND_V01:
          sensor_data.type = SENSOR_TYPE_MOTION_VEHICLE;
          sensor_data.sensor = HANDLE_MOTION_VEHICLE;
          sensor_data.data[0] = sam_qmd_rpt_ptr->state;
          timestamp = sam_qmd_rpt_ptr->timestamp;
          HAL_LOG_VERBOSE("%s: sensor %d, motion %f", __FUNCTION__, sensor_data.type, sensor_data.data[0]);
          break;
        case SNS_SAM_VMD_ERROR_IND_V01:
          error = true;
          break;
        default:
          error = true;
          break;
      }
      break;
    }
    default:
      error = true;
      break;
  }

  /* Lock on cb_mutex before checking active sensors */
  pthread_mutex_lock( &g_sensor_control->cb_mutex );
  if( (RMD_IS_ACTIVE) || (AMD_IS_ACTIVE))
  {
    pthread_mutex_unlock( &g_sensor_control->cb_mutex );
    if ( error == false ) /* No error */
    {
      /* Check instance ID to make sure it matches sensor handle */
      if ( sam_qmd_rpt_ptr->instance_id ==
             g_sensor_control->sam_service[ msg_hdr->service_number ].instance_id )
      {
        sensor_data.version = sizeof(sensors_event_t);

        sensor_data.timestamp = hal_timestamp_calc( (uint64_t)timestamp, sensor_data.sensor );
        if( hal_insert_queue( &sensor_data ) )
        {
          hal_signal_ind( &g_sensor_control->data_arrived_cond );
        }
      }
      else
      {
        HAL_LOG_ERROR("%s: Instance Id mismatch, report instance_id %d, sam_service instance id %d",__FUNCTION__,
                      sam_qmd_rpt_ptr->instance_id,
                      g_sensor_control->sam_service[ msg_hdr->service_number ].instance_id);
      }
    }
    else
    {
      HAL_LOG_ERROR("%s: Logged Error",__FUNCTION__);
    }
  }
  else
  {
    pthread_mutex_unlock( &g_sensor_control->cb_mutex );
  }
}

/*===========================================================================

  FUNCTION:  hal_process_sam_smd_ind

===========================================================================*/
/*!
*/
static void
hal_process_sam_smd_ind( sensor1_msg_header_s *msg_hdr,
                         void *msg_ptr )
{
  sns_sam_smd_report_ind_msg_v01*  sam_smd_rpt_ptr =
                                     (sns_sam_smd_report_ind_msg_v01*) msg_ptr;
  bool             error = false;
  uint32_t         timestamp = 0;
  sensors_event_t  sensor_data;

  switch( msg_hdr->msg_id )
  {
    case SNS_SAM_SMD_REPORT_IND_V01:
      sensor_data.type = SENSOR_TYPE_SIGNIFICANT_MOTION;
      sensor_data.sensor = HANDLE_SIGNIFICANT_MOTION;
      if (sam_smd_rpt_ptr->report_data.motion_state == SNS_SAM_SMD_STATE_MOTION_V01)
      {
        sensor_data.data[0] = 1;
        sensor_data.data[1] = 0;
        sensor_data.data[2] = 0;
        HAL_LOG_VERBOSE("%s: sensor %d, motion %d", __FUNCTION__,
                        sensor_data.type,
                        sam_smd_rpt_ptr->report_data.motion_state);
        timestamp = sam_smd_rpt_ptr->timestamp;
      }
      else
      {
        HAL_LOG_ERROR("%s: Invalid motion state %d",__FUNCTION__, sam_smd_rpt_ptr->report_data.motion_state);
      }
      break;
    case SNS_SAM_SMD_ERROR_IND_V01:
      error = true;
      break;
    default:
      error = true;
      break;
  }

  if ( error == false ) /* No error */
  {
    /* Check instance ID to make sure it matches sensor handle */
    if ( sam_smd_rpt_ptr->instance_id ==
           g_sensor_control->sam_service[ msg_hdr->service_number ].instance_id )
    {
      // Deactivate SMD since it's an one-shot sensor
      hal_sam_deactivate_smd();

      sensor_data.version = sizeof(sensors_event_t);

      sensor_data.timestamp = hal_timestamp_calc( (uint64_t)timestamp, sensor_data.sensor );
      if( hal_insert_queue( &sensor_data ) )
      {
        hal_signal_ind( &g_sensor_control->data_arrived_cond );
      }
    }
    else
    {
      HAL_LOG_ERROR("%s: Instance Id mismatch",__FUNCTION__);
    }
  }
}

/*===========================================================================

  FUNCTION:  hal_process_sam_cmc_ind

===========================================================================*/
/*!
*/
static void
hal_process_sam_cmc_ind( sensor1_msg_header_s *msg_hdr,
                         void *msg_ptr )
{
  sns_sam_cmc_report_ind_msg_v01*  sam_cmc_rpt_ptr =
                                     (sns_sam_cmc_report_ind_msg_v01*) msg_ptr;
  bool             error = false;
  uint32_t         timestamp;
  sensors_event_t  sensor_data;

  switch( msg_hdr->msg_id )
  {
    case SNS_SAM_CMC_REPORT_IND_V01:
      sensor_data.type = SENSOR_TYPE_CMC;
      sensor_data.sensor = HANDLE_CMC;
      sensor_data.data[0] = sam_cmc_rpt_ptr->report_data.motion_state;
      sensor_data.data[1] = sam_cmc_rpt_ptr->report_data.motion_state_probability;
      HAL_LOG_VERBOSE("%s: sensor %d, motion state %f, probability %f", __FUNCTION__,
                      sensor_data.type, sensor_data.data[0], sensor_data.data[1]);
      timestamp = sam_cmc_rpt_ptr->timestamp;
      break;
    case SNS_SAM_CMC_ERROR_IND_V01:
      error = true;
      break;
    default:
      error = true;
      break;
  }

  if ( error == false ) /* No error */
  {
    /* Check instance ID to make sure it matches sensor handle */
    if ( sam_cmc_rpt_ptr->instance_id ==
           g_sensor_control->sam_service[ msg_hdr->service_number ].instance_id )
    {
      sensor_data.version = sizeof(sensors_event_t);

      sensor_data.timestamp = hal_timestamp_calc( (uint64_t)timestamp, sensor_data.sensor );
      if( hal_insert_queue( &sensor_data ) )
      {
        hal_signal_ind( &g_sensor_control->data_arrived_cond );
      }
    }
    else
    {
      HAL_LOG_ERROR("%s: Instance Id mismatch id1=%d id2=%d",__FUNCTION__,
                    sam_cmc_rpt_ptr->instance_id,
                    g_sensor_control->sam_service[ msg_hdr->service_number ].instance_id);
    }
  }
  else
  {
    HAL_LOG_ERROR("%s: Logged Error",__FUNCTION__);
  }
}

/*===========================================================================

  FUNCTION:  hal_process_sam_pam_ind

===========================================================================*/
/*!
*/
static void
hal_process_sam_pam_ind( sensor1_msg_header_s *msg_hdr,
                         void *msg_ptr )
{
  sns_sam_pam_report_ind_msg_v01*  sam_pam_rpt_ptr =
                                     (sns_sam_pam_report_ind_msg_v01*) msg_ptr;
  bool             error = false;
  uint32_t         timestamp;
  sensors_event_t  sensor_data;

  switch( msg_hdr->msg_id )
  {
    case SNS_SAM_PAM_REPORT_IND_V01:
      sensor_data.type = SENSOR_TYPE_PAM;
      sensor_data.sensor = HANDLE_PAM;
      sensor_data.data[0] = sam_pam_rpt_ptr->step_count;
      sensor_data.data[1] = 0;
      sensor_data.data[2] = 0;

      HAL_LOG_VERBOSE("%s: sensor %d, step count %f", __FUNCTION__, sensor_data.type, sensor_data.data[0]);
      timestamp = sam_pam_rpt_ptr->timestamp;
      break;
    case SNS_SAM_PAM_ERROR_IND_V01:
    default:
      error = true;
      break;
  }

  if ( error == false ) /* No error */
  {
    /* Check instance ID to make sure it matches sensor handle */
    if ( sam_pam_rpt_ptr->instance_id ==
           g_sensor_control->sam_service[ msg_hdr->service_number ].instance_id )
    {
      sensor_data.version = sizeof(sensors_event_t);

      /* convert time to nanosec */
      sensor_data.timestamp = hal_timestamp_calc( (uint64_t)timestamp, sensor_data.sensor );

      if( hal_insert_queue( &sensor_data ) )
      {
        hal_signal_ind( &g_sensor_control->data_arrived_cond );
      }
    }
    else
    {
      HAL_LOG_ERROR("%s: Instance Id mismatch, Report Ind Inst ID: %d,HAL Handle ID: %d",
                    __FUNCTION__, sam_pam_rpt_ptr->instance_id,
                    g_sensor_control->sam_service[ msg_hdr->service_number ].instance_id);
    }
  }
  else
  {
    HAL_LOG_ERROR("%s: Logged Error",__FUNCTION__);
  }
}



#ifdef FEATURE_SNS_HAL_SAM_INT
/*===========================================================================

  FUNCTION:  hal_process_sam_bte_ind

===========================================================================*/
/*!
*/
static void
hal_process_sam_bte_ind( sensor1_msg_header_s *msg_hdr,
                         void *msg_ptr )
{
  sns_sam_bte_report_ind_msg_v01*  sam_bte_rpt_ptr =
                                     (sns_sam_bte_report_ind_msg_v01*) msg_ptr;
  bool                             error = false;
  uint32_t                         timestamp;
  sensors_event_t                  sensor_data;

  switch( msg_hdr->msg_id )
  {
    case SNS_SAM_BTE_REPORT_IND_V01:
      sensor_data.type = SENSOR_TYPE_GESTURE_BRING_TO_EAR;
      sensor_data.sensor = HANDLE_GESTURE_BRING_TO_EAR;
      sensor_data.data[0] = sam_bte_rpt_ptr->state;
      timestamp = sam_bte_rpt_ptr->timestamp;
      break;
    case SNS_SAM_BTE_ERROR_IND_V01:
      error = true;
      break;
    default:
      error = true;
      break;
  }

  if ( error == false ) /* No error */
  {
    /* Check instance ID to make sure it matches sensor handle */
    if ( sam_bte_rpt_ptr->instance_id ==
           g_sensor_control->sam_service[ SNS_SAM_BTE_SVC_ID_V01 ].instance_id )
    {
      sensor_data.version = sizeof(sensors_event_t);

      sensor_data.timestamp = hal_timestamp_calc( (uint64_t)timestamp, sensor_data.sensor );
      if( hal_insert_queue( &sensor_data ) )
      {
        hal_signal_ind( &g_sensor_control->data_arrived_cond );
      }
    }
  }
}

/*===========================================================================

  FUNCTION:  hal_process_sam_fns_ind

===========================================================================*/
/*!
*/
static void
hal_process_sam_fns_ind( sensor1_msg_header_s *msg_hdr,
                         void *msg_ptr )
{
  sns_sam_fns_report_ind_msg_v01*  sam_fns_rpt_ptr =
                                     (sns_sam_fns_report_ind_msg_v01*) msg_ptr;
  bool                             error = false;
  uint32_t                         timestamp;
  sensors_event_t                  sensor_data;

  switch( msg_hdr->msg_id )
  {
    case SNS_SAM_FNS_REPORT_IND_V01:
      sensor_data.type = SENSOR_TYPE_GESTURE_FACE_N_SHAKE;
      sensor_data.sensor = HANDLE_GESTURE_FACE_N_SHAKE;
      sensor_data.data[0] = sam_fns_rpt_ptr->state;
      timestamp = sam_fns_rpt_ptr->timestamp;
      break;
    case SNS_SAM_FNS_ERROR_IND_V01:
      error = true;
      break;
    default:
      error = true;
      break;
  }

  if ( error == false ) /* No error */
  {
    /* Check instance ID to make sure it matches sensor handle */
    if ( sam_fns_rpt_ptr->instance_id ==
           g_sensor_control->sam_service[ SNS_SAM_FNS_SVC_ID_V01 ].instance_id )
    {
      sensor_data.version = sizeof(sensors_event_t);

      sensor_data.timestamp = hal_timestamp_calc( (uint64_t)timestamp, sensor_data.sensor );
      if( hal_insert_queue( &sensor_data ) )
      {
        hal_signal_ind( &g_sensor_control->data_arrived_cond );
      }
    }
  }
}
#endif /* FEATURE_SNS_HAL_SAM_INT */

/*===========================================================================

  FUNCTION:  hal_process_sam_basic_gestures_ind

===========================================================================*/
/*!
*/
static void
hal_process_sam_basic_gestures_ind( sensor1_msg_header_s *msg_hdr,
                         void *msg_ptr )
{
  sns_sam_basic_gestures_report_ind_msg_v01*  sam_basic_gestures_rpt_ptr =
                                     (sns_sam_basic_gestures_report_ind_msg_v01*) msg_ptr;
  bool                             error = false;
  uint32_t                         timestamp;
  sensors_event_t                  sensor_data;

  memset(&sensor_data, 0, sizeof(sensors_event_t));

  switch( msg_hdr->msg_id )
  {
    case SNS_SAM_BASIC_GESTURES_REPORT_IND_V01:
      sensor_data.type = SENSOR_TYPE_BASIC_GESTURES;
      sensor_data.sensor = HANDLE_GESTURE_BASIC_GESTURES;
      sensor_data.data[0] = sam_basic_gestures_rpt_ptr->state;
      timestamp = sam_basic_gestures_rpt_ptr->timestamp;

      HAL_LOG_VERBOSE("%s: BASIC GESTURES=%f,", __FUNCTION__, sensor_data.data[0]);
      break;
    case SNS_SAM_BASIC_GESTURES_ERROR_IND_V01:
      error = true;
      break;
    default:
      error = true;
      break;
  }

  if ( error == false ) /* No error */
  {
    /* Check instance ID to make sure it matches sensor handle */
    if ( sam_basic_gestures_rpt_ptr->instance_id ==
           g_sensor_control->sam_service[ SNS_SAM_BASIC_GESTURES_SVC_ID_V01 ].instance_id )
    {
      sensor_data.version = sizeof(sensors_event_t);

      /* convert time to nanosec */
      sensor_data.timestamp = hal_timestamp_calc( (uint64_t)timestamp, sensor_data.sensor );
      if( hal_insert_queue( &sensor_data ) )
      {
        hal_signal_ind( &g_sensor_control->data_arrived_cond );
      }
    }
  }
}

/*===========================================================================

  FUNCTION:  hal_process_sam_tap_ind

===========================================================================*/
/*!
*/
static void
hal_process_sam_tap_ind( sensor1_msg_header_s *msg_hdr,
                         void *msg_ptr )
{
  sns_sam_tap_report_ind_msg_v01*  sam_tap_rpt_ptr =
                                     (sns_sam_tap_report_ind_msg_v01*) msg_ptr;
  bool                             error = false;
  uint32_t                         timestamp;
  sensors_event_t                  sensor_data;

  memset(&sensor_data, 0, sizeof(sensors_event_t));

  switch( msg_hdr->msg_id )
  {
    case SNS_SAM_TAP_REPORT_IND_V01:
      sensor_data.type = SENSOR_TYPE_TAP;
      sensor_data.sensor = HANDLE_GESTURE_TAP;
      sensor_data.data[0] = sam_tap_rpt_ptr->state;
      timestamp = sam_tap_rpt_ptr->timestamp;

      HAL_LOG_VERBOSE("%s: TAP=%f,", __FUNCTION__, sensor_data.data[0]);
      break;
    case SNS_SAM_TAP_ERROR_IND_V01:
      error = true;
      break;
    default:
      error = true;
      break;
  }

  if ( error == false ) /* No error */
  {
    /* Check instance ID to make sure it matches sensor handle */
    if ( sam_tap_rpt_ptr->instance_id ==
           g_sensor_control->sam_service[ SNS_SAM_TAP_SVC_ID_V01 ].instance_id )
    {
      sensor_data.version = sizeof(sensors_event_t);

      sensor_data.timestamp = hal_timestamp_calc( (uint64_t)timestamp, sensor_data.sensor );
      if( hal_insert_queue( &sensor_data ) )
      {
        hal_signal_ind( &g_sensor_control->data_arrived_cond );
      }
    }
  }
}

/*===========================================================================

  FUNCTION:  hal_process_sam_gyro_tap_ind

===========================================================================*/
/*!
*/
static void
hal_process_sam_gyro_tap_ind( sensor1_msg_header_s *msg_hdr,
                              void *msg_ptr )
{
  sns_sam_gyro_tap2_report_ind_msg_v01*  sam_tap_rpt_ptr =
                                     (sns_sam_gyro_tap2_report_ind_msg_v01*) msg_ptr;
  bool                             error = false;
  uint32_t                         timestamp;
  sensors_event_t                  sensor_data;

  memset(&sensor_data, 0, sizeof(sensors_event_t));

  switch( msg_hdr->msg_id )
  {
    case SNS_SAM_GYRO_TAP2_REPORT_IND_V01:
      sensor_data.type = SENSOR_TYPE_TAP;
      sensor_data.sensor = HANDLE_GESTURE_GYRO_TAP;
      sensor_data.data[0] = sam_tap_rpt_ptr->tap_event;
      timestamp = sam_tap_rpt_ptr->timestamp;

      HAL_LOG_VERBOSE("%s: GYRO TAP=%f,", __FUNCTION__, sensor_data.data[0]);
      break;
    case SNS_SAM_GYRO_TAP2_ERROR_IND_V01:
      error = true;
      break;
    default:
      error = true;
      break;
  }

  if ( error == false ) /* No error */
  {
    /* Check instance ID to make sure it matches sensor handle */
    if ( sam_tap_rpt_ptr->instance_id ==
           g_sensor_control->sam_service[ SNS_SAM_GYRO_TAP2_SVC_ID_V01 ].instance_id )
    {
      sensor_data.version = sizeof(sensors_event_t);

      sensor_data.timestamp = hal_timestamp_calc( (uint64_t)timestamp, sensor_data.sensor );
      if( hal_insert_queue( &sensor_data ) )
      {
        hal_signal_ind( &g_sensor_control->data_arrived_cond );
      }
    }
  }
}

/*===========================================================================

  FUNCTION:  hal_process_sam_facing_ind

===========================================================================*/
/*!
*/
static void
hal_process_sam_facing_ind( sensor1_msg_header_s *msg_hdr,
                         void *msg_ptr )
{
  sns_sam_facing_report_ind_msg_v01*  sam_facing_rpt_ptr =
                                     (sns_sam_facing_report_ind_msg_v01*) msg_ptr;

  bool                             error = false;
  uint32_t                         timestamp;
  sensors_event_t                  sensor_data;

  memset(&sensor_data, 0, sizeof(sensors_event_t));

  switch( msg_hdr->msg_id )
  {
    case SNS_SAM_FACING_REPORT_IND_V01:
      sensor_data.type = SENSOR_TYPE_FACING;
      sensor_data.sensor = HANDLE_GESTURE_FACING;
      sensor_data.data[0] = sam_facing_rpt_ptr->state;
      timestamp = sam_facing_rpt_ptr->timestamp;

      HAL_LOG_VERBOSE("%s: FACING=%f,", __FUNCTION__, sensor_data.data[0]);
      break;

    case SNS_SAM_FACING_ERROR_IND_V01:
      error = true;
      break;
    default:
      error = true;
      break;
  }

  if ( error == false ) /* No error */
  {
    /* Check instance ID to make sure it matches sensor handle */
    if ( sam_facing_rpt_ptr->instance_id ==
           g_sensor_control->sam_service[ SNS_SAM_FACING_SVC_ID_V01 ].instance_id )
    {
      sensor_data.version = sizeof(sensors_event_t);

      sensor_data.timestamp = hal_timestamp_calc( (uint64_t)timestamp, sensor_data.sensor );
      if( hal_insert_queue( &sensor_data ) )
      {
        hal_signal_ind( &g_sensor_control->data_arrived_cond );
      }
    }
  }
}

/*===========================================================================

  FUNCTION:  hal_process_sam_tilt_ind

===========================================================================*/
/*!
*/
static void
hal_process_sam_tilt_ind( sensor1_msg_header_s *msg_hdr,
                         void *msg_ptr )
{
  sns_sam_integ_angle_report_ind_msg_v01*  rpt_ptr =
                                     (sns_sam_integ_angle_report_ind_msg_v01*) msg_ptr;

  bool                             error = false;
  uint32_t                         timestamp;
  sensors_event_t                  sensor_data;

  memset(&sensor_data, 0, sizeof(sensors_event_t));

  switch( msg_hdr->msg_id )
  {
    case SNS_SAM_INTEG_ANGLE_REPORT_IND_V01:
      sensor_data.type = SENSOR_TYPE_TILT;
      sensor_data.sensor = HANDLE_GESTURE_TILT;

      /* Converting coordinate systems: QCOM to Android: */
      sensor_data.data[0] = rpt_ptr->angle[1] * RAD_Q16_TO_DEG_FLT;
      sensor_data.data[1] = rpt_ptr->angle[0] * RAD_Q16_TO_DEG_FLT;
      sensor_data.data[2] = -1 * rpt_ptr->angle[2] * RAD_Q16_TO_DEG_FLT;
      timestamp = rpt_ptr->timestamp;

      HAL_LOG_VERBOSE("%s: TILT =%f,%f, %f", __FUNCTION__, sensor_data.data[0],
                      sensor_data.data[1],
                      sensor_data.data[2]);
      break;

    case SNS_SAM_INTEG_ANGLE_ERROR_IND_V01:
      error = true;
      break;
    default:
      error = true;
      break;
  }

  if ( error == false ) /* No error */
  {
    /* Check instance ID to make sure it matches sensor handle */
    if ( rpt_ptr->instance_id ==
           g_sensor_control->sam_service[ SNS_SAM_INTEG_ANGLE_SVC_ID_V01 ].instance_id )
    {
      sensor_data.version = sizeof(sensors_event_t);

      sensor_data.timestamp = hal_timestamp_calc( (uint64_t)timestamp, sensor_data.sensor );
      if( hal_insert_queue( &sensor_data ) )
      {
        hal_signal_ind( &g_sensor_control->data_arrived_cond );
      }
    }
    else
    {
      HAL_LOG_ERROR("%s: instance id mismatched! instance_id=%d, instance_id=%d", __FUNCTION__,
                    rpt_ptr->instance_id ,
                    g_sensor_control->sam_service[ SNS_SAM_INTEG_ANGLE_SVC_ID_V01 ].instance_id);
    }
  }
}

/*===========================================================================

  FUNCTION:  hal_handle_broken_pipe

===========================================================================*/
/*!
*/
static void hal_handle_broken_pipe()
{
  int ret = SENSOR1_EFAILED;
  HAL_LOG_DEBUG( "%s", __FUNCTION__ );

  // clean up resources upon the daemon DOWN
  hal_cleanup_resources();
  hal_time_stop();

  // Mark all sensors as changed, so they get reset when the daemon is ready.
  g_sensor_control->changed_sensors = g_sensor_control->available_sensors;

  // re-initialize and re-acquire resources upon the daemon UP
  // hal_reinit will be invoked again in the callback if the daemon is not ready
  if( SENSOR1_SUCCESS != ( ret = hal_reinit() ) )
  {
    HAL_LOG_ERROR( "%s: hal_reinit() failed ret=%d", __FUNCTION__, ret );
  }
}


/*===========================================================================

  FUNCTION:  hal_sensor1_data_cb

===========================================================================*/
/*!
*/
static void
hal_sensor1_data_cb (intptr_t cb_data,
                     sensor1_msg_header_s *msg_hdr,
                     sensor1_msg_type_e msg_type,
                     void *msg_ptr)
{
  if( msg_hdr != NULL )
  {
    HAL_LOG_VERBOSE("%s: msg_type %d, Sn %d, msg Id %d, txn Id %d", __FUNCTION__,
                    msg_type, msg_hdr->service_number, msg_hdr->msg_id, msg_hdr->txn_id );
  }
  else
  {
    if( msg_type != SENSOR1_MSG_TYPE_BROKEN_PIPE &&
        msg_type != SENSOR1_MSG_TYPE_REQ &&
        msg_type != SENSOR1_MSG_TYPE_RETRY_OPEN )
    {
      HAL_LOG_ERROR("%s: Error - invalid msg type with NULL msg_hdr: %u",
            __FUNCTION__, msg_type );
      return ;
    }
    else
    {
      HAL_LOG_VERBOSE("%s: msg_type %d", __FUNCTION__, msg_type);
    }
  }

  switch( msg_type )
  {
    case SENSOR1_MSG_TYPE_RESP_INT_ERR:
      if ( ( msg_hdr->service_number == SNS_SMGR_SVC_ID_V01 )
           || ( msg_hdr->service_number == SNS_SAM_AMD_SVC_ID_V01 )
           || ( msg_hdr->service_number == SNS_SAM_RMD_SVC_ID_V01 )
           || ( msg_hdr->service_number == SNS_SAM_VMD_SVC_ID_V01 )
#ifdef FEATURE_SNS_HAL_SAM_INT
           || ( msg_hdr->service_number == SNS_SAM_FNS_SVC_ID_V01 )
           || ( msg_hdr->service_number == SNS_SAM_BTE_SVC_ID_V01 )
#endif /* FEATURE_SNS_HAL_SAM_INT */
           || ( msg_hdr->service_number == SNS_SAM_BASIC_GESTURES_SVC_ID_V01 )
           || ( msg_hdr->service_number == SNS_SAM_TAP_SVC_ID_V01 )
           || ( msg_hdr->service_number == SNS_SAM_GYRO_TAP2_SVC_ID_V01 )
           || ( msg_hdr->service_number == SNS_SAM_FACING_SVC_ID_V01 )
           || ( msg_hdr->service_number == SNS_SAM_INTEG_ANGLE_SVC_ID_V01 )
           || ( msg_hdr->service_number == SNS_SAM_GRAVITY_VECTOR_SVC_ID_V01)
           || ( msg_hdr->service_number == SNS_SAM_ROTATION_VECTOR_SVC_ID_V01)
           || ( msg_hdr->service_number == SNS_SAM_ORIENTATION_SVC_ID_V01)
           || ( msg_hdr->service_number == SNS_SAM_MAG_CAL_SVC_ID_V01 )
           || ( msg_hdr->service_number == SNS_SAM_SENSOR_THRESH_SVC_ID_V01 )
           || ( msg_hdr->service_number == SNS_SAM_PED_SVC_ID_V01 )
           || ( msg_hdr->service_number == SNS_SAM_PAM_SVC_ID_V01 )
           || ( msg_hdr->service_number == SNS_SAM_SMD_SVC_ID_V01 )
           || ( msg_hdr->service_number == SNS_SAM_GAME_ROTATION_VECTOR_SVC_ID_V01 )
           || ( msg_hdr->service_number == SNS_SAM_CMC_SVC_ID_V01 )
           )
      {
        pthread_mutex_lock( &g_sensor_control->cb_mutex );
        hal_signal_response( true, &g_sensor_control->cb_arrived_cond );
        pthread_mutex_unlock( &g_sensor_control->cb_mutex );
      }
      break;

    case SENSOR1_MSG_TYPE_RESP:
      if( msg_hdr->service_number == SNS_SMGR_SVC_ID_V01 )
      {
        hal_process_smgr_resp( msg_hdr, msg_ptr );
      }
      else if ( ( msg_hdr->service_number == SNS_SAM_AMD_SVC_ID_V01 ) ||
                ( msg_hdr->service_number == SNS_SAM_RMD_SVC_ID_V01 ) ||
                ( msg_hdr->service_number == SNS_SAM_VMD_SVC_ID_V01 ) )
      {
        hal_process_sam_qmd_resp( msg_hdr, (sns_sam_qmd_enable_resp_msg_v01*)msg_ptr );
      }
      else
      {
        pthread_mutex_lock( &g_sensor_control->cb_mutex );
        if ( msg_hdr->service_number == SNS_SAM_GRAVITY_VECTOR_SVC_ID_V01 )
        {
          hal_process_sam_gravity_resp( msg_hdr, msg_ptr );
        }
        else if ( msg_hdr->service_number == SNS_SAM_ROTATION_VECTOR_SVC_ID_V01 )
        {
          hal_process_sam_rotation_vector_resp( msg_hdr, msg_ptr );
        }
        else if ( msg_hdr->service_number == SNS_SAM_ORIENTATION_SVC_ID_V01 )
        {
          hal_process_sam_orientation_resp( msg_hdr, msg_ptr );
        }
        else if ( msg_hdr->service_number == SNS_SAM_MAG_CAL_SVC_ID_V01 )
        {
          hal_process_sam_mag_cal_resp( msg_hdr, msg_ptr );
        }
        else if ( msg_hdr->service_number == SNS_SAM_SENSOR_THRESH_SVC_ID_V01 )
        {
          hal_process_sam_sensor_thresh_resp( msg_hdr, msg_ptr );
        }
#ifdef FEATURE_SNS_HAL_SAM_INT
        else if ( msg_hdr->service_number == SNS_SAM_FNS_SVC_ID_V01 )
        {
          hal_process_sam_fns_resp( msg_hdr, msg_ptr );
        }
        else if ( msg_hdr->service_number == SNS_SAM_BTE_SVC_ID_V01 )
        {
          hal_process_sam_bte_resp( msg_hdr, msg_ptr );
        }
#endif /* FEATURE_SNS_HAL_SAM_INT */
        else if ( msg_hdr->service_number == SNS_SAM_BASIC_GESTURES_SVC_ID_V01 )
        {
          hal_process_sam_basic_gestures_resp( msg_hdr, msg_ptr );
        }
        else if ( msg_hdr->service_number == SNS_SAM_TAP_SVC_ID_V01 )
        {
          hal_process_sam_tap_resp( msg_hdr, msg_ptr );
        }
        else if ( msg_hdr->service_number == SNS_SAM_GYRO_TAP2_SVC_ID_V01 )
        {
          hal_process_sam_gyro_tap_resp( msg_hdr, msg_ptr );
        }
        else if ( msg_hdr->service_number == SNS_SAM_FACING_SVC_ID_V01 )
        {
          hal_process_sam_facing_resp( msg_hdr, msg_ptr );
        }
        else if ( msg_hdr->service_number == SNS_SAM_INTEG_ANGLE_SVC_ID_V01 )
        {
          hal_process_sam_tilt_resp( msg_hdr, msg_ptr );
        }
        else if( msg_hdr->service_number == SNS_SAM_PED_SVC_ID_V01 )
        {
          hal_process_sam_ped_resp( msg_hdr, msg_ptr );
        }
        else if( msg_hdr->service_number == SNS_SAM_PAM_SVC_ID_V01 )
        {
          hal_process_sam_pam_resp( msg_hdr, msg_ptr );
        }
        else if ( ( msg_hdr->service_number == SNS_SAM_SMD_SVC_ID_V01 ) )
        {
          hal_process_sam_smd_resp( msg_hdr, msg_ptr );
        }
        else if ( msg_hdr->service_number == SNS_SAM_GAME_ROTATION_VECTOR_SVC_ID_V01 )
        {
          hal_process_sam_game_rotation_vector_resp( msg_hdr, msg_ptr );
        }
        else if ( ( msg_hdr->service_number == SNS_SAM_CMC_SVC_ID_V01 ) )
        {
          hal_process_sam_cmc_resp( msg_hdr, msg_ptr );
        }
        pthread_mutex_unlock( &g_sensor_control->cb_mutex );
      }
      break;

    case SENSOR1_MSG_TYPE_IND:
      if( msg_hdr->service_number == SNS_SMGR_SVC_ID_V01 )
      {
        hal_process_smgr_ind( msg_hdr, msg_ptr );
      }

      pthread_mutex_lock( &g_sensor_control->data_mutex );
      if ( msg_hdr->service_number == SNS_SAM_PAM_SVC_ID_V01 )
      {
        hal_process_sam_pam_ind( msg_hdr, msg_ptr );
      }
      else if ( ( msg_hdr->service_number == SNS_SAM_AMD_SVC_ID_V01 ) ||
                ( msg_hdr->service_number == SNS_SAM_RMD_SVC_ID_V01 ) ||
                ( msg_hdr->service_number == SNS_SAM_VMD_SVC_ID_V01 ) )
      {
        hal_process_sam_qmd_ind( msg_hdr, msg_ptr );
      }
      else if ( ( msg_hdr->service_number == SNS_SAM_SMD_SVC_ID_V01 ) )
      {
        hal_process_sam_smd_ind( msg_hdr, msg_ptr );
      }
      else if ( ( msg_hdr->service_number == SNS_SAM_CMC_SVC_ID_V01 ) )
      {
        hal_process_sam_cmc_ind( msg_hdr, msg_ptr );
      }
#ifdef FEATURE_SNS_HAL_SAM_INT
      else if ( msg_hdr->service_number == SNS_SAM_FNS_SVC_ID_V01 )
      {
        hal_process_sam_fns_ind( msg_hdr, msg_ptr );
      }
      else if ( msg_hdr->service_number == SNS_SAM_BTE_SVC_ID_V01 )
      {
        hal_process_sam_bte_ind( msg_hdr, msg_ptr );
      }
#endif /* FEATURE_SNS_HAL_SAM_INT */
      else if ( msg_hdr->service_number == SNS_SAM_BASIC_GESTURES_SVC_ID_V01 )
      {
        hal_process_sam_basic_gestures_ind( msg_hdr, msg_ptr );
      }
      else if ( msg_hdr->service_number == SNS_SAM_TAP_SVC_ID_V01 )
      {
        hal_process_sam_tap_ind( msg_hdr, msg_ptr );
      }
      else if ( msg_hdr->service_number == SNS_SAM_GYRO_TAP2_SVC_ID_V01 )
      {
        hal_process_sam_gyro_tap_ind( msg_hdr, msg_ptr );
      }
      else if ( msg_hdr->service_number == SNS_SAM_FACING_SVC_ID_V01 )
      {
        hal_process_sam_facing_ind( msg_hdr, msg_ptr );
      }
      else if ( msg_hdr->service_number == SNS_SAM_INTEG_ANGLE_SVC_ID_V01 )
      {
        hal_process_sam_tilt_ind( msg_hdr, msg_ptr );
      }
      else if ( msg_hdr->service_number == SNS_SAM_ORIENTATION_SVC_ID_V01 ||
                msg_hdr->service_number == SNS_SAM_ROTATION_VECTOR_SVC_ID_V01 ||
                msg_hdr->service_number == SNS_SAM_GAME_ROTATION_VECTOR_SVC_ID_V01 ||
                msg_hdr->service_number == SNS_SAM_GRAVITY_VECTOR_SVC_ID_V01 ||
                msg_hdr->service_number == SNS_SAM_PED_SVC_ID_V01 ||
                msg_hdr->service_number == SNS_SAM_SENSOR_THRESH_SVC_ID_V01 )
      {
        hal_sam_parse_ind( g_sensor_control, msg_hdr, msg_ptr );
      }
      else if ( msg_hdr->service_number == SNS_SAM_MAG_CAL_SVC_ID_V01 )
      {
        hal_process_sam_mag_cal_ind( msg_hdr, msg_ptr );
      }
      pthread_mutex_unlock( &g_sensor_control->data_mutex );
      break;

    case SENSOR1_MSG_TYPE_BROKEN_PIPE:
      HAL_LOG_WARN("%s: SENSOR1_MSG_TYPE_BROKEN_PIPE", __FUNCTION__);
      pthread_mutex_lock( &g_sensor_control->cb_mutex );
      hal_handle_broken_pipe();
      pthread_mutex_unlock( &g_sensor_control->cb_mutex );
      break;

    case SENSOR1_MSG_TYPE_RETRY_OPEN:
      HAL_LOG_WARN("%s: SENSOR1_MSG_TYPE_RETRY_OPEN", __FUNCTION__);
      pthread_mutex_lock( &g_sensor_control->cb_mutex );
      hal_reinit();
      pthread_mutex_unlock( &g_sensor_control->cb_mutex );
      break;

    case SENSOR1_MSG_TYPE_REQ:
    default:
      HAL_LOG_ERROR("%s: Error - invalid msg type in cb: %u", __FUNCTION__, msg_type );
      break;
  }

  pthread_mutex_lock( &g_sensor_control->cb_mutex );
  if( NULL != msg_ptr && g_sensor_control->hndl )
  {
    sensor1_free_msg_buf( g_sensor_control->hndl, msg_ptr );
  }
  pthread_mutex_unlock( &g_sensor_control->cb_mutex );
}


/*===========================================================================

  FUNCTION:  hal_smgr_prepare_report_req

  This function creates a buffered report request for SMGR.

  @param handle[i]: sensor handle
  @param sample_rate[i]: sampling rate for sensor
  @param report_rate[i]: reporting rate for SMGR indications
  @param wake_upon_fifo_full[i]: the WuFF flag from Android
  @param buff_req[o]: Address of a pointer to the buffer req. Will be allocated

  @return
  true if success
  false if error -- no messages will be allocated

===========================================================================*/
/*!
*/
static bool
hal_smgr_prepare_report_req( int handle, hal_sensor_control_t* sensor_ctl, uint32_t sample_rate,
                             uint32_t report_rate, bool wake_upon_fifo_full,
                             sns_smgr_buffering_req_msg_v01 **buff_req )
{
  sensor1_error_e         error;
  /* Message Body */
  error = sensor1_alloc_msg_buf( sensor_ctl->hndl,
                                 sizeof(sns_smgr_buffering_req_msg_v01),
                                 (void**)buff_req );
  if( SENSOR1_SUCCESS != error )
  {
    HAL_LOG_ERROR( "%s: sensor1_alloc_msg_buf() error: %d", __FUNCTION__, error );
    (*buff_req) = NULL;
    return false;
  }

  /* Report Request */
  (*buff_req)->ReportId = sensor_ctl->report_ids[handle];
  (*buff_req)->Action = SNS_SMGR_BUFFERING_ACTION_ADD_V01;

  if( report_rate == 0 ) {
    report_rate = sample_rate*UNIT_Q16;
  }
  (*buff_req)->ReportRate = report_rate;

  /* Most HAL apis don't use a 2nd report item */
  (*buff_req)->Item_len = 1;

  /* Most requests are for primary data */
  (*buff_req)->Item[0].DataType = SNS_SMGR_DATA_TYPE_PRIMARY_V01;

  /* set default behavior for indications during suspend */
  (*buff_req)->notify_suspend_valid = true;
  (*buff_req)->notify_suspend.proc_type = SNS_PROC_APPS_V01;
  (*buff_req)->notify_suspend.send_indications_during_suspend = wake_upon_fifo_full;

  (*buff_req)->Item[0].SamplingRate = sample_rate;
  (*buff_req)->Item[1].SamplingRate = sample_rate;
  (*buff_req)->Item[0].Calibration = SNS_SMGR_CAL_SEL_FULL_CAL_V01;
  (*buff_req)->Item[0].Decimation = SNS_SMGR_DECIMATION_FILTER_V01;

  if( hal_get_raw_data_mode( handle, sensor_ctl ) == true )
  {
    (*buff_req)->Item[0].Calibration = SNS_SMGR_CAL_SEL_RAW_V01;
    (*buff_req)->Item[0].Decimation = SNS_SMGR_DECIMATION_RECENT_SAMPLE_V01;
    HAL_LOG_WARN( "%s: Sending RAW Data", __FUNCTION__ );
  }

  switch( handle )
  {
    case HANDLE_ACCELERATION:
      (*buff_req)->Item[0].SensorId = SNS_SMGR_ID_ACCEL_V01;
      break;

    case HANDLE_LIGHT:
      (*buff_req)->Item[0].DataType = SNS_SMGR_DATA_TYPE_SECONDARY_V01;
      (*buff_req)->Item[0].SensorId = SNS_SMGR_ID_PROX_LIGHT_V01;
      break;

    case HANDLE_PROXIMITY:
      (*buff_req)->Item[0].SensorId = SNS_SMGR_ID_PROX_LIGHT_V01;

      /* allow proximity indications to wake up apps processor */
      (*buff_req)->notify_suspend.send_indications_during_suspend = true;
      break;

    case HANDLE_GYRO:
      (*buff_req)->Item[0].SensorId = SNS_SMGR_ID_GYRO_V01;
      break;

#ifdef FEATURE_SNS_HAL_SMGR_PRESSURE
    case HANDLE_PRESSURE:
      (*buff_req)->Item[0].SensorId = SNS_SMGR_ID_PRESSURE_V01;
      break;
#endif

    case HANDLE_RELATIVE_HUMIDITY:
      (*buff_req)->Item[0].SensorId = SNS_SMGR_ID_HUMIDITY_V01;
      break;

    case HANDLE_AMBIENT_TEMPERATURE:
      (*buff_req)->Item[0].SensorId = SNS_SMGR_ID_HUMIDITY_V01;
      (*buff_req)->Item[0].DataType = SNS_SMGR_DATA_TYPE_SECONDARY_V01;
      break;

    case HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM:
      (*buff_req)->Item[0].SensorId    = SNS_SMGR_ID_MAG_V01;
      (*buff_req)->Item[0].Calibration = SNS_SMGR_CAL_SEL_FACTORY_CAL_V01;
      (*buff_req)->Item[0].Decimation  = SNS_SMGR_DECIMATION_RECENT_SAMPLE_V01;
     break;

    case HANDLE_SMGR_STEP_DETECTOR:
      (*buff_req)->Item[0].DataType = SNS_SMGR_DATA_TYPE_SECONDARY_V01;
      (*buff_req)->Item[0].SensorId = SNS_SMGR_ID_STEP_EVENT_V01;
      (*buff_req)->Item[0].Decimation = SNS_SMGR_DECIMATION_RECENT_SAMPLE_V01;
      break;

    case HANDLE_SMGR_STEP_COUNT:
      (*buff_req)->Item[0].SensorId = SNS_SMGR_ID_STEP_COUNT_V01;
      (*buff_req)->Item[0].Decimation = SNS_SMGR_DECIMATION_RECENT_SAMPLE_V01;
      break;

    case HANDLE_SMGR_SMD:
      (*buff_req)->Item[0].SensorId = SNS_SMGR_ID_SMD_V01;
      (*buff_req)->Item[0].Decimation = SNS_SMGR_DECIMATION_RECENT_SAMPLE_V01;

      /* allow SMD indications to wake up apps processor */
      (*buff_req)->notify_suspend.send_indications_during_suspend = true;
      break;

    case HANDLE_SMGR_GAME_RV:
      (*buff_req)->Item[0].SensorId = SNS_SMGR_ID_GAME_ROTATION_VECTOR_V01;
      (*buff_req)->Item[0].Decimation = SNS_SMGR_DECIMATION_RECENT_SAMPLE_V01;
      break;

    case HANDLE_MAGNETIC_FIELD:
      (*buff_req)->Item[0].SensorId = SNS_SMGR_ID_MAG_V01;
      (*buff_req)->Item[0].Decimation = SNS_SMGR_DECIMATION_RECENT_SAMPLE_V01;
      break;

    case HANDLE_MAGNETIC_FIELD_UNCALIBRATED:
      (*buff_req)->Item[0].DataType    = SNS_SMGR_DATA_TYPE_PRIMARY_V01;
      (*buff_req)->Item[0].SensorId    = SNS_SMGR_ID_MAG_V01;
      (*buff_req)->Item[0].Calibration = SNS_SMGR_CAL_SEL_FACTORY_CAL_V01;
      (*buff_req)->Item[0].Decimation  = SNS_SMGR_DECIMATION_RECENT_SAMPLE_V01;

      (*buff_req)->Item[1].DataType     = SNS_SMGR_DATA_TYPE_PRIMARY_V01;
      (*buff_req)->Item[1].SensorId     = SNS_SMGR_ID_MAG_V01;
      (*buff_req)->Item[1].SamplingRate = sample_rate;
      (*buff_req)->Item[1].Calibration  = SNS_SMGR_CAL_SEL_FULL_CAL_V01;
      (*buff_req)->Item[1].Decimation   = SNS_SMGR_DECIMATION_RECENT_SAMPLE_V01;

      (*buff_req)->Item_len = 2;
      break;

    case HANDLE_GYRO_UNCALIBRATED:
      /* Request for both cal & uncal gyro in one report. Both are needed
         to report gyro bias offsets */
      (*buff_req)->Item_len = 2;
      (*buff_req)->Item[0].SensorId = SNS_SMGR_ID_GYRO_V01;
      (*buff_req)->Item[0].Calibration = SNS_SMGR_CAL_SEL_FACTORY_CAL_V01;
      (*buff_req)->Item[0].Decimation = SNS_SMGR_DECIMATION_RECENT_SAMPLE_V01;
      (*buff_req)->Item[1].DataType = SNS_SMGR_DATA_TYPE_PRIMARY_V01;
      (*buff_req)->Item[1].SensorId = SNS_SMGR_ID_GYRO_V01;
      (*buff_req)->Item[1].Calibration = SNS_SMGR_CAL_SEL_FULL_CAL_V01;
      (*buff_req)->Item[1].Decimation = SNS_SMGR_DECIMATION_RECENT_SAMPLE_V01;
       break;

     case HANDLE_RGB:
      (*buff_req)->Item_len = 2;
      (*buff_req)->Item[0].SensorId = SNS_SMGR_ID_RGB_V01;
      (*buff_req)->Item[0].DataType = SNS_SMGR_DATA_TYPE_PRIMARY_V01;
      (*buff_req)->Item[1].SensorId = SNS_SMGR_ID_RGB_V01;
      (*buff_req)->Item[1].DataType = SNS_SMGR_DATA_TYPE_SECONDARY_V01;
      (*buff_req)->Item[1].Calibration = SNS_SMGR_CAL_SEL_FULL_CAL_V01;
      (*buff_req)->Item[1].Decimation = SNS_SMGR_DECIMATION_RECENT_SAMPLE_V01;
      break;

    case HANDLE_IR_GESTURE:
      (*buff_req)->Item[0].SensorId  = SNS_SMGR_ID_IR_GESTURE_V01;
      break;

    case HANDLE_SAR:
      (*buff_req)->Item[0].SensorId  = SNS_SMGR_ID_SAR_V01;
      break;

    case HANDLE_ORIENTATION:
    default:
      HAL_LOG_ERROR("%s: Error: invalid sensor handle %d", __FUNCTION__, handle);
      sensor1_free_msg_buf( sensor_ctl->hndl, (*buff_req) );
      (*buff_req) = NULL;
      return false;
  }
  return true;
}


/*===========================================================================

  FUNCTION:  hal_smgr_report_add

  This function sends request to SMGR for adding a buffered report for sensor
  specified in the handle.

===========================================================================*/
/*!
*/
static bool
hal_smgr_report_add( int handle, hal_sensor_control_t* sensor_ctl,
                     uint32_t sample_rate, uint32_t report_rate,
                     bool wake_upon_fifo_full, bool buffer, bool wait_for_resp )
{
  sensor1_error_e         error;
  sensor1_msg_header_s    req_hdr;
  sns_smgr_buffering_req_msg_v01 *smgr_buffering_req;
  bool resp = false;

  HAL_LOG_DEBUG("%s: handle=%d, sample_rate=%d report_rate=%d WuFF=%d buffer=%d",
                __FUNCTION__, handle, sample_rate, report_rate,
                wake_upon_fifo_full, buffer);

  if( !hal_smgr_prepare_report_req( handle, sensor_ctl, sample_rate, report_rate,
                                    wake_upon_fifo_full, &smgr_buffering_req ) )
  {
    return false;
  }

  req_hdr.txn_id = ( wait_for_resp ) ? handle : TXN_ID_NO_RESP_SIGNALLED;
  sensor_ctl->error = false;

  req_hdr.service_number = SNS_SMGR_SVC_ID_V01;
  req_hdr.msg_id = SNS_SMGR_BUFFERING_REQ_V01;
  req_hdr.msg_size = sizeof( sns_smgr_buffering_req_msg_v01 );
  if( (error = sensor1_write( sensor_ctl->hndl, &req_hdr,
                              smgr_buffering_req )) != SENSOR1_SUCCESS )
  {
    sensor1_free_msg_buf( sensor_ctl->hndl, smgr_buffering_req );
    HAL_LOG_ERROR("%s: sensor1_write() error: %d", __FUNCTION__, error );
    return false;
  }

  if( wait_for_resp )
  {
    resp = hal_wait_for_response( TIME_OUT_MS,
                                  &(sensor_ctl->cb_mutex),
                                  &(sensor_ctl->cb_arrived_cond),
                                  &(sensor_ctl->is_resp_arrived));

    if( !resp )
    {
      HAL_LOG_ERROR( "%s: ERROR: No response from request", __FUNCTION__ );
      return false;
    }

    HAL_LOG_DEBUG( "%s: Received Response: %d", __FUNCTION__, sensor_ctl->error );
    /* received response */
    if( sensor_ctl->error )
    {
      return false;
    }
  }
  return true;
}


/*===========================================================================

  FUNCTION:  hal_smgr_report_delete

===========================================================================*/
/*!
*/
static bool
hal_smgr_report_delete( int handle, hal_sensor_control_t* sensor_ctl )
{
  sensor1_error_e       error;
  sensor1_msg_header_s  req_hdr;
  sns_smgr_buffering_req_msg_v01 *smgr_req;
  bool resp = false;

  HAL_LOG_DEBUG( "%s: handle=%d", __FUNCTION__, handle );

  error = sensor1_alloc_msg_buf( sensor_ctl->hndl,
                                 sizeof(sns_smgr_buffering_req_msg_v01),
                                 (void**)&smgr_req );
  if( SENSOR1_SUCCESS != error )
  {
    HAL_LOG_ERROR( "%s: sensor1_alloc_msg_buf() failed: %u", __FUNCTION__, error );
    return false;
  }
  /* Message header */
  req_hdr.service_number = SNS_SMGR_SVC_ID_V01;
  req_hdr.msg_id = SNS_SMGR_BUFFERING_REQ_V01;
  req_hdr.msg_size = sizeof( sns_smgr_buffering_req_msg_v01 );
  req_hdr.txn_id = handle;

  /* Message body */
  smgr_req->ReportId = sensor_ctl->report_ids[handle];
  smgr_req->Action = SNS_SMGR_BUFFERING_ACTION_DELETE_V01;

  /* Send request */
  sensor_ctl->error = false;
  if( (error = sensor1_write( sensor_ctl->hndl, &req_hdr,
                              smgr_req )) != SENSOR1_SUCCESS )
  {
    sensor1_free_msg_buf( sensor_ctl->hndl, smgr_req );
    HAL_LOG_ERROR( "%s: sensor1_write() error: %u", __FUNCTION__, error );
    return false;
  }

   resp = hal_wait_for_response( TIME_OUT_MS,
                       &(sensor_ctl->cb_mutex),
                       &(sensor_ctl->cb_arrived_cond),
                       &(sensor_ctl->is_resp_arrived));

  if( !resp )
  {
    HAL_LOG_ERROR( "%s: ERROR: no response from request", __FUNCTION__ );
  }
  else if( sensor_ctl->error )
  {
    HAL_LOG_ERROR( "%s: Error in report delete", __FUNCTION__ );
  }
  else
  {
    HAL_LOG_DEBUG( "%s: Rcvd success response from request", __FUNCTION__ );
    return true;
  }

  return false;
}

/*===========================================================================

  FUNCTION:  hal_sensor1_open

===========================================================================*/
/*!
  @brief
  This function is called to initialize and open the sensor1 interface.
  cb_mutex should be held.
*/
static sensor1_error_e hal_sensor1_open( hal_sensor_control_t* sensor_ctl )
{
  int             ret = -1;
  sensor1_error_e error;

  if( sensor_ctl->hndl == (sensor1_handle_s*)-1 )
  {
    error = sensor1_open( &sensor_ctl->hndl, &hal_sensor1_data_cb,
                          (intptr_t)sensor_ctl );
    HAL_LOG_INFO("Sensor1 open: %d %"PRIuPTR, error, (uintptr_t)sensor_ctl->hndl );
    if( SENSOR1_SUCCESS != error )
    {
      // return error from sensor1_open() including WOULDBLOCK in case of
      // sensors daemon restart
      return error;
    }
  }

  ret = SENSOR1_SUCCESS;
  return ret;
}

/*===========================================================================

  FUNCTION:  hal_sensor1_close

===========================================================================*/
/*!
  @brief
  This function is called to close the sensor1 interface.  cb_mutex should
  be held.
*/
static void hal_sensor1_close( hal_sensor_control_t *sensor_ctl )
{
  sensor1_handle_s* hndl = sensor_ctl->hndl;
  sensor_ctl->hndl = (sensor1_handle_s*)-1;

  if( (sensor1_handle_s*)-1 != hndl )
  {
    HAL_LOG_INFO( "%s: closing sensor1...", __FUNCTION__ );
    sensor1_close( hndl );
  }
}

/*===========================================================================

  FUNCTION:  hal_load_oem_lib

===========================================================================*/
static int hal_load_oem_lib(const struct hw_module_t **module)
{
  int status = -1;
  void *oem_hal_lib_handle;
  struct hw_module_t *hmi;

  oem_hal_lib_handle = dlopen(OEM_LIB_PATH, RTLD_NOW);
  if( NULL == oem_hal_lib_handle )
  {
    HAL_LOG_DEBUG("%s: Could not open OEM HAL library %s", __FUNCTION__, OEM_LIB_PATH );
    return (status);
  }

  hmi = (struct hw_module_t *)dlsym(oem_hal_lib_handle, HAL_MODULE_INFO_SYM_AS_STR);
  if (NULL == hmi)
  {
    HAL_LOG_ERROR("%s: ERROR: Could not find symbol %s", __FUNCTION__,HAL_MODULE_INFO_SYM_AS_STR );
    dlclose(oem_hal_lib_handle);
    oem_hal_lib_handle = NULL;
    return (status);
  }

  hmi->dso = oem_hal_lib_handle;
  status = 0;

  *module = hmi;
  HAL_LOG_DEBUG("loaded OEM HAL path=%s hmi=%p handle=%p",
               OEM_LIB_PATH, *module, oem_hal_lib_handle);


  return status;
}

/*===========================================================================

  FUNCTION: hal_check_ltcy_ssi_reg_flag

===========================================================================*/
/*!
  @brief
  checks a particular CFG group about whether latency measurement enabled or
  not for the corresponding sensor, then update the latency enable control
  member and latency measurement enable table in global variable g_sensor_control.

  @param[i] ssi_flag_id CFG group flag ID
  @param[i] ssi_sensor_id CFG group sensor ID
*/
static void hal_check_ltcy_ssi_reg_flag( int ssi_flag_id, int ssi_sensor_id )
{
  uint8_t *reg_value = NULL,
           reg_length = 0;
  int error;

  error = sensor_reg_read( ssi_flag_id, 1, &reg_value, &reg_length );
  if( SENSOR_REG_SUCCESS != error || NULL == reg_value )
  {
    HAL_LOG_ERROR( "%s: Error in sensor_reg_read() for reg flag: %i",
                   __FUNCTION__, error );
  }
  else
  {
    if( *reg_value & SNS_REG_SSI_FLAG_LTCY_ENABLE )
    {
      free( reg_value );
      error = sensor_reg_read( ssi_sensor_id, 1, &reg_value, &reg_length );
      if( SENSOR_REG_SUCCESS != error )
      {
        HAL_LOG_ERROR( "%s: Error in sensor_reg_read() for reg sensor id: %i",
                       __FUNCTION__, error );
      }
      else
      {
        switch( *reg_value )
        {
          case SNS_SMGR_ID_ACCEL_V01:
            g_sensor_control->ltcy_en_table[HAL_LTCY_MEASURE_ACCEL] = true;
            break;
          case SNS_SMGR_ID_GYRO_V01:
            g_sensor_control->ltcy_en_table[HAL_LTCY_MEASURE_GYRO] = true;
            break;
          case SNS_SMGR_ID_MAG_V01:
            g_sensor_control->ltcy_en_table[HAL_LTCY_MEASURE_MAG] = true;
            break;
          case SNS_SMGR_ID_PRESSURE_V01:
            g_sensor_control->ltcy_en_table[HAL_LTCY_MEASURE_PRESSURE] = true;
            break;
          case SNS_SMGR_ID_PROX_LIGHT_V01:
            g_sensor_control->ltcy_en_table[HAL_LTCY_MEASURE_PROX_LIGHT] = true;
            break;
          case SNS_SMGR_ID_HUMIDITY_V01:
            g_sensor_control->ltcy_en_table[HAL_LTCY_MEASURE_HUMIDITY] = true;
            break;
          case SNS_SMGR_ID_RGB_V01:
            g_sensor_control->ltcy_en_table[HAL_LTCY_MEASURE_RGB] = true;
            break;
          case SNS_SMGR_ID_IR_GESTURE_V01:
            g_sensor_control->ltcy_en_table[HAL_LTCY_MEASURE_IR_GESTURES] = true;
            break;
          case SNS_SMGR_ID_SAR_V01:
            g_sensor_control->ltcy_en_table[HAL_LTCY_MEASURE_SAR] = true;
            break;
          default:
            break;
        }
        g_sensor_control->is_ltcy_measure_enabled = true;
      }
    }
  }
  free( reg_value );
}

/*===========================================================================

  FUNCTION: hal_check_ltcy_measure

  This function checks whether latency measurement enabled or not for each
  physical sensor.

===========================================================================*/
static void hal_check_ltcy_measure( void )
{
  int cfg_flag_id[] = { SNS_REG_ITEM_SSI_SMGR_CFG0_FLAGS_V02,
                        SNS_REG_ITEM_SSI_SMGR_CFG1_FLAGS_V02,
                        SNS_REG_ITEM_SSI_SMGR_CFG2_FLAGS_V02,
                        SNS_REG_ITEM_SSI_SMGR_CFG3_FLAGS_V02,
                        SNS_REG_ITEM_SSI_SMGR_CFG4_FLAGS_V02 };
  int cfg_sensor_id[] = { SNS_REG_ITEM_SSI_SMGR_CFG0_SENSOR_ID_V02,
                          SNS_REG_ITEM_SSI_SMGR_CFG1_SENSOR_ID_V02,
                          SNS_REG_ITEM_SSI_SMGR_CFG2_SENSOR_ID_V02,
                          SNS_REG_ITEM_SSI_SMGR_CFG3_SENSOR_ID_V02,
                          SNS_REG_ITEM_SSI_SMGR_CFG4_SENSOR_ID_V02 };

  int i, num = sizeof(cfg_flag_id) / sizeof(int);
  for ( i=0; i<num; i++ )
  {
     hal_check_ltcy_ssi_reg_flag( cfg_flag_id[i], cfg_sensor_id[i] );
  }
}

/*===========================================================================

  FUNCTION: hal_ltcy_measure

===========================================================================*/
/*!
  @brief
  measure the delivery latency and report the result using
  log packet

  @param[i] curr_ts current apps timestamp
  @param[i] data[] array of sensor data got by hal_sensors_data_poll
  @param[i] rcv_data_num is the number of data got from hal_sensors_data_poll
*/
static void hal_ltcy_measure( uint64_t curr_ts, const sensors_event_t data[], int rcv_data_num )
{
  int err, j;
  int32_t sensor_id = SNS_SMGR_ID_ACCEL_V01;  //set the default one as accel
  hal_ltcy_measure_t ltcy_measure_type = HAL_LTCY_NUM_TYPES; //default value

  for( j=0; j<rcv_data_num; j++ )
  {
    switch (data[j].sensor)
    {
      case HANDLE_ACCELERATION:
        sensor_id = SNS_SMGR_ID_ACCEL_V01;
        ltcy_measure_type = HAL_LTCY_MEASURE_ACCEL;
        break;
      case HANDLE_MAGNETIC_FIELD:
      case HANDLE_MAGNETIC_FIELD_UNCALIBRATED:
      case HANDLE_MAGNETIC_FIELD_SAM:
      case HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM:
        sensor_id = SNS_SMGR_ID_MAG_V01;
        ltcy_measure_type = HAL_LTCY_MEASURE_MAG;
        break;
      case HANDLE_PROXIMITY:
        sensor_id = SNS_SMGR_ID_PROX_LIGHT_V01;
        ltcy_measure_type = HAL_LTCY_MEASURE_PROX_LIGHT;
        break;
      case HANDLE_GYRO:
        sensor_id = SNS_SMGR_ID_GYRO_V01;
        ltcy_measure_type = HAL_LTCY_MEASURE_GYRO;
        break;
      case HANDLE_PRESSURE:
        sensor_id = SNS_SMGR_ID_PRESSURE_V01;
        ltcy_measure_type = HAL_LTCY_MEASURE_PRESSURE;
        break;
      case HANDLE_RELATIVE_HUMIDITY:
      case HANDLE_AMBIENT_TEMPERATURE:
        sensor_id = SNS_SMGR_ID_HUMIDITY_V01;
        ltcy_measure_type = HAL_LTCY_MEASURE_HUMIDITY;
        break;
      case HANDLE_RGB:
        sensor_id = SNS_SMGR_ID_RGB_V01;
        ltcy_measure_type = HAL_LTCY_MEASURE_RGB;
        break;
      case HANDLE_IR_GESTURE:
        sensor_id = SNS_SMGR_ID_IR_GESTURE_V01;
        ltcy_measure_type = HAL_LTCY_MEASURE_IR_GESTURES;
        break;
      case HANDLE_SAR:
        sensor_id = SNS_SMGR_ID_SAR_V01;
        ltcy_measure_type = HAL_LTCY_MEASURE_SAR;
        break;
      default:
        break;
    }

    if( ltcy_measure_type >= HAL_LTCY_NUM_TYPES )
    {
      HAL_LOG_ERROR( "%s: reading data[].sensor error with handle: %d", __FUNCTION__, data[j].sensor );
      break;
    }

    if( g_sensor_control->ltcy_en_table[ltcy_measure_type] )
    {
      sns_log_latency_delivery_s  *log_ptr;     //test with sensor1 response log packet type
      void                        *temp_ptr;
      sns_log_id_e                 log_type;
      uint32_t                     logpkt_size;

      log_type = SNS_LOG_LATENCY_DELIVERY;
      logpkt_size = SNS_LOG_MAX_SIZE + sizeof(sns_log_latency_delivery_s);

      //log packet malloc
      temp_ptr = log_alloc( LOG_SNS_LATENCY_DELIVERY_C, logpkt_size );
      if( temp_ptr == NULL )
      {
        HAL_LOG_ERROR( "log_alloc error!" );
        break;
      }
      else
      {
        log_ptr = (sns_log_latency_delivery_s *)temp_ptr;

        //allocate the log packet
        log_ptr->version = SNS_LOG_LATENCY_DELIVERY_VERSION;
        log_ptr->timestamp = curr_ts;
        log_ptr->sensor_id = sensor_id;
        log_ptr->data_timestamp = g_sensor_control->ltcy_measure_dsps_tick;
        log_ptr->delivery_latency = (int64_t)curr_ts - data[j].timestamp;

        //log packet commit
        log_commit( (void*)log_ptr);
      }
    }
  }
}

/*===========================================================================

  FUNCTION: hal_init

===========================================================================*/
static void hal_init( void ) __attribute__((constructor));
static void hal_init( void )
{
  int i;
  int rv = -1;
  int rdm_error = -1;
  int status = ENOENT;
  pthread_mutexattr_t attr;
  struct stat statbuf;

  hal_sensor_control_t* sensor_ctl;

  char debug_prop[PROPERTY_VALUE_MAX];
  int  debug_prop_len;
  int smgr_mag_cal_support_prop_len;
  char smgr_mag_cal_support_prop[PROPERTY_VALUE_MAX] = "false";

  struct sigevent acquire_resources_event;

  memset( &acquire_resources_event, 0, sizeof(acquire_resources_event) );

  sensor1_init();

  debug_prop_len = property_get( HAL_PROP_DEBUG, debug_prop, "" );
  if( debug_prop_len == 1 ) {
    switch( debug_prop[0] ) {
      case '0':
        g_hal_log_level = HAL_LOG_LEVEL_DISABLED;
        break;
      case '1':
        g_hal_log_level = HAL_LOG_LEVEL_ALL;
        break;
      case 'v':
      case 'V':
        g_hal_log_level = HAL_LOG_LEVEL_VERBOSE;
        break;
      case 'd':
      case 'D':
        g_hal_log_level = HAL_LOG_LEVEL_DEBUG;
        break;
      case 'i':
      case 'I':
        g_hal_log_level = HAL_LOG_LEVEL_INFO;
        break;
      case 'w':
      case 'W':
        g_hal_log_level = HAL_LOG_LEVEL_WARN;
        break;
      case 'e':
      case 'E':
        g_hal_log_level = HAL_LOG_LEVEL_ERROR;
        break;
      default:
        break;
    }
    LOGI("%s: Setting log level to %d", __FUNCTION__, g_hal_log_level);
  } else if( debug_prop_len > 1 ) {
    LOGE("%s: invalid value for %s: %s. Enabling all logs", __FUNCTION__,
         HAL_PROP_DEBUG, debug_prop );
    g_hal_log_level = HAL_LOG_LEVEL_ALL;
  }

  sensor_ctl = malloc(sizeof(*sensor_ctl));
  if (sensor_ctl == NULL)
  {
    HAL_LOG_ERROR("%s: ERROR: malloc error", __FUNCTION__ );
    return;
  }

  memset(sensor_ctl, 0, sizeof(*sensor_ctl));

  sensor_ctl->hndl = (sensor1_handle_s*)-1;
  for( i=0; i<MAX_NUM_SENSORS; i++ )
  {
    sensor_ctl->report_ids[i] = i;
    sensor_ctl->current_freq[i] = 0;
    sensor_ctl->sensor_list[i].handle = -1;
    sensor_ctl->sensor_list[i].max_buffered_samples = 0;
    sensor_ctl->last_event[i].timestamp = 0;
  }
  sensor_ctl->is_resp_arrived = false;
  sensor_ctl->is_ind_arrived = false;
  sensor_ctl->error = false;
  sensor_ctl->active_sensors = 0;
  sensor_ctl->available_sensors = 0;
  sensor_ctl->num_smgr_sensors = 0;
  sensor_ctl->step_counter_running_total = 0;
  sensor_ctl->step_counter_running_instance = 0;
  sensor_ctl->step_counter_current_instance = 0;
  sensor_ctl->stepc_last_ts = 0;
  sensor_ctl->stepc_activated = false;

  sensor_ctl->device.common.tag       = HARDWARE_DEVICE_TAG;
#ifdef SENSORS_DEVICE_API_VERSION_1_1
  sensor_ctl->device.common.version   = SENSORS_DEVICE_API_VERSION_1_1;
#else
  sensor_ctl->device.common.version   = SENSORS_DEVICE_API_VERSION_1_0;
#endif /* #else SENSORS_DEVICE_API_VERSION_1_1 */
  sensor_ctl->device.common.close     = _hal_sensors_close;
  sensor_ctl->device.activate         = _hal_sensors_activate;
  sensor_ctl->device.poll             = _hal_sensors_data_poll;
  sensor_ctl->device.setDelay         = _hal_sensors_set_delay;
  sensor_ctl->device.batch            = _hal_sensors_batch;
#ifdef SENSORS_DEVICE_API_VERSION_1_1
  sensor_ctl->device.flush            = _hal_sensors_flush;
#endif /* SENSORS_DEVICE_API_VERSION_1_1 */

  sensor_ctl->mag_cal_src = HAL_MAG_CAL_SRC_UNKNOWN;
  rdm_error = sensor_reg_open();
  if(rdm_error == SENSOR_REG_SUCCESS)
  {
    uint8 reg_len = 0;
    uint8 *preg_val = NULL;
    int reg_error;
    reg_error = sensor_reg_read(SNS_REG_ITEM_QMAG_CAL_ENABLE_ALGO_V02, 1, &preg_val, &reg_len);
    if(reg_error == SENSOR_REG_SUCCESS &&
       preg_val != NULL)
    {
      if(*preg_val) //Qmag_cal is enabled in registry
      {
        //ENABLE QMAG CAL
        sensor_ctl->mag_cal_src = HAL_MAG_CAL_SRC_SMGR;
      }
    }
    else
    {
      HAL_LOG_ERROR( "%s: Error in sensor_reg_read(): %i",
      __FUNCTION__, reg_error );
    }
    free(preg_val);
  }
  else
  {
    HAL_LOG_ERROR( "%s: Error in sensor_reg_open(): %i",
          __FUNCTION__, rdm_error );
  }

  HAL_LOG_DEBUG("%s: smgr support for mag cal %d", __FUNCTION__,
                (sensor_ctl->mag_cal_src == HAL_MAG_CAL_SRC_SMGR));

  pthread_mutexattr_init( &attr );
  pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
  pthread_mutex_init( &(sensor_ctl->cb_mutex), &attr );
  pthread_cond_init( &(sensor_ctl->cb_arrived_cond), NULL );
  pthread_mutexattr_destroy(&attr);

  pthread_mutex_init( &(sensor_ctl->data_mutex), NULL );
  pthread_cond_init( &(sensor_ctl->data_arrived_cond), NULL );

  pthread_mutex_init( &(sensor_ctl->acquire_resources_mutex), NULL );


  acquire_resources_event.sigev_notify = SIGEV_THREAD;
  acquire_resources_event.sigev_notify_function = (void*)hal_acquire_resources;

  if( 0 != timer_create( CLOCK_MONOTONIC,
                         &acquire_resources_event,
                         &sensor_ctl->acquire_resources_timer ) )
  {
    HAL_LOG_ERROR("%s: ERROR: timer_create failed", __FUNCTION__ );
    free(sensor_ctl);
    return;
  }

  /* Time Service */
  hal_time_init();

  /* Set the last event for proximity and light to invalid values */
  sensor_ctl->last_event[HANDLE_PROXIMITY].distance = -1;
  sensor_ctl->last_event[HANDLE_LIGHT].light = -1;

  sensor_ctl->sam_service[ SNS_SAM_GAME_ROTATION_VECTOR_SVC_ID_V01 ].freq        = 0;
  sensor_ctl->sam_service[ SNS_SAM_GAME_ROTATION_VECTOR_SVC_ID_V01 ].instance_id = INVALID_INSTANCE_ID;
  sensor_ctl->sam_service[ SNS_SAM_ROTATION_VECTOR_SVC_ID_V01 ].freq        = 0;
  sensor_ctl->sam_service[ SNS_SAM_ROTATION_VECTOR_SVC_ID_V01 ].instance_id = INVALID_INSTANCE_ID;
  sensor_ctl->sam_service[ SNS_SAM_ORIENTATION_SVC_ID_V01 ].freq            = 0;
  sensor_ctl->sam_service[ SNS_SAM_ORIENTATION_SVC_ID_V01 ].instance_id     = INVALID_INSTANCE_ID;
  sensor_ctl->sam_service[ SNS_SAM_GRAVITY_VECTOR_SVC_ID_V01 ].freq         = 0;
  sensor_ctl->sam_service[ SNS_SAM_GRAVITY_VECTOR_SVC_ID_V01 ].instance_id  = INVALID_INSTANCE_ID;
  sensor_ctl->sam_service[ SNS_SAM_PED_SVC_ID_V01 ].instance_id             = INVALID_INSTANCE_ID;
  sensor_ctl->sam_service[ SNS_SAM_PED_SVC_ID_V01 ].ref_count               = 0;

  sensor_ctl->is_ltcy_measure_enabled = false;
  for( i=0; i<HAL_LTCY_NUM_TYPES; i++ )
  {
    sensor_ctl->ltcy_en_table[i] = false;
  }

  /* save to global variable */
  g_sensor_control = sensor_ctl;

  /* oem sensor members */
  g_oem_sensor_control.OEMModule = NULL;
  g_oem_sensor_control.OEMDevice = NULL;
  g_oem_sensor_control.OEM_poll_thread = 0;
  g_oem_sensor_control.threadCounter = 0;

  /* Load the OEM shared library */
  status = hal_load_oem_lib( (hw_module_t const**)&g_oem_sensor_control.OEMModule );
  if( status != 0 )
  {
    g_oem_sensor_control.OEMModule = NULL;
  }
  else
  {
    HAL_LOG_DEBUG( "%s: Hal opened lib %s", __FUNCTION__, OEM_LIB_PATH );
  }

  /* Initialize raw data mode to be disabled */
  uint8_t rdm_value = 0;


  // PENDING: If we had access to sns_reg.h in this file, we could reset the
  //          registry value to the true default.

  if(rdm_error == SENSOR_REG_SUCCESS)
  {
     rdm_error = sensor_reg_write( SNS_REG_ITEM_RAW_DATA_MODE_V02,
                                   &rdm_value, 1, 1 );
     if( SENSOR_REG_SUCCESS != rdm_error )
     {
       HAL_LOG_ERROR( "%s: Error in sensor_reg_write(): %i",
             __FUNCTION__, rdm_error );
     }
  }

  /* Motion Accel */
  hal_ma_init( true );

  hal_check_ltcy_measure();
}

/*===========================================================================

  FUNCTION: _hal_sensors_open

===========================================================================*/
/*!
  @brief
  Initialize the sensor module
  HAL API - Note that:
    sensors_open(SENSORS_HARDWARE_POLL) called from SensorService's context

  @param module sensor module
  @param name SENSORS_HARDWARE_POLL
  @param device sensor device

  @return 0 if successful, <0 if failed
 */
static int
_hal_sensors_open( const struct hw_module_t* module,
                   const char* name, struct hw_device_t* *device)
{
  int ret = -1;

  HAL_LOG_DEBUG( "%s: name=%s", __FUNCTION__, name );

  if( !strcmp( name, SENSORS_HARDWARE_POLL ) )
  {
    if( NULL == g_sensor_control )
    {
      HAL_LOG_ERROR("%s: HAL control init failed", __FUNCTION__ );
      return ret;
    }

    /* open the kernel driver instance */
    if( g_oem_sensor_control.OEMModule != NULL )
    {
      int err;
      err = sensors_open( &g_oem_sensor_control.OEMModule->common, (struct sensors_poll_device_t**)&g_oem_sensor_control.OEMDevice );
      if( err != 0 )
      {
          HAL_LOG_ERROR( "%s: sensors_open failure %d", __FUNCTION__, err );
          return ret;
      }
      HAL_LOG_DEBUG( "%s: sensors_open success", __FUNCTION__ );
    }

    g_sensor_control->device.common.module = (struct hw_module_t*) module;
    *device = &g_sensor_control->device.common;
    ret = 0;
  }

  return ret;
}

/*===========================================================================

  FUNCTION:  hal_get_trigger_mode

===========================================================================*/
/*!
  @brief
  Determine the Android sensor trigger mode.

  @param handle Handle provided by Android Sensor Service

  @return Trigger mode
  */
static sensor_trigger_mode
hal_get_trigger_mode( int handle )
{
  if( !hal_is_event_sensor( handle, 0 ) )
  {
    return SENSOR_MODE_CONT;
  }
  else if( HANDLE_SMGR_SMD == handle ||
           HANDLE_SIGNIFICANT_MOTION == handle )
  {
    return SENSOR_MODE_TRIG;
  }
  else
  {
    return SENSOR_MODE_EVENT;
  }
}

/*===========================================================================

  FUNCTION:  hal_schedule_acquire_resource

===========================================================================*/
/*!
  @brief
  Schedules for the hal_acquire_resources function to be called at a later
  time.

  */
static void
hal_schedule_acquire_resources( void )
{
  const struct itimerspec new_val = {
    .it_interval = { .tv_sec = 0,
                     .tv_nsec = 0},
    .it_value = { .tv_sec = 0,
                  .tv_nsec = HAL_ACQUIRE_RESOURCES_DELAY_NSEC }
  };
  struct itimerspec old_val;

  timer_settime( g_sensor_control->acquire_resources_timer, 0,
                 &new_val,
                 &old_val );

  if( 0 != old_val.it_value.tv_nsec )
  {
    HAL_LOG_DEBUG("%s: Resetting acquire resources timer for %ld usec. old_value = %ld usec",
                  __FUNCTION__, new_val.it_value.tv_nsec / 1000L,
                  old_val.it_value.tv_nsec / 1000L );
  } else {
    HAL_LOG_DEBUG("%s: Setting acquire resources timer for %ld usec", __FUNCTION__,
                  new_val.it_value.tv_nsec / 1000L );
  }
}

/*===========================================================================

  FUNCTION: _hal_sensors_get_sensors_list

===========================================================================*/
/*!
  @brief
  Enumerate all available sensors. The list is returned in "list"
  This function sends request to server to retrieve the sensor list

  HAL API - This function is called from application's context

  @param module sensors module
  @param list sensor list

  @return number of sensors in the list
*/
static int
_hal_sensors_get_sensors_list( struct sensors_module_t* module,
                               struct sensor_t const** list )
{
  int                   i,j;
  sensor1_error_e       error;
  sensor1_msg_header_s  msg_hdr;
  static struct sensor_t sensor_list[MAX_NUM_SENSORS];
  static int            sensor_list_len = 0; /* Number of valid entries in array */
  int                   sensor_type;

  HAL_LOG_INFO("%s", __FUNCTION__ );

  if( g_sensor_control == NULL )
  {
    HAL_LOG_ERROR("%s: HAL control init failed", __FUNCTION__ );
    return 0;
  }

  pthread_mutex_lock( &g_sensor_control->cb_mutex );

  /* Determine source of mag cal */
  if (g_sensor_control->mag_cal_src == HAL_MAG_CAL_SRC_UNKNOWN)
  {
     if (hal_is_sam_mag_cal_available(g_sensor_control))
     {
         HAL_LOG_DEBUG("%s: SAM provides calibrated MAG", __FUNCTION__);
     }
     else
     {
         g_sensor_control->mag_cal_src = HAL_MAG_CAL_SRC_NONE;
         HAL_LOG_DEBUG("%s: Using SMGR uncalibrated MAG", __FUNCTION__);
     }
  }
  else
  {
    HAL_LOG_DEBUG("%s: SMGR provides calibrated MAG", __FUNCTION__);
  }

  if( sensor_list_len != 0 )
  {
    HAL_LOG_INFO("%s: Already have the list of sensors", __FUNCTION__ );
    *list = sensor_list;
  }
  else if( hal_sensor1_open( g_sensor_control ) != SENSOR1_SUCCESS )
  {
    HAL_LOG_ERROR("%s: sensor1 init failed", __FUNCTION__ );
    *list = NULL;
  }
  else
  {
    sns_smgr_all_sensor_info_req_msg_v01 * smgr_req;

    msg_hdr.service_number = SNS_SMGR_SVC_ID_V01;
    msg_hdr.msg_id = SNS_SMGR_ALL_SENSOR_INFO_REQ_V01;
    msg_hdr.msg_size = sizeof( sns_smgr_all_sensor_info_req_msg_v01 );
    msg_hdr.txn_id = 0;

    error = sensor1_alloc_msg_buf( g_sensor_control->hndl,
                                  sizeof(sns_smgr_all_sensor_info_req_msg_v01),
                                  (void**)&smgr_req );
    if( SENSOR1_SUCCESS != error )
    {
      HAL_LOG_ERROR("%s: msg alloc failed: %d", __FUNCTION__, error );
      pthread_mutex_unlock( &g_sensor_control->cb_mutex );
      return 0;
    }

    g_sensor_control->error = false;
    if( (error = sensor1_write( g_sensor_control->hndl,
                                &msg_hdr, smgr_req )) != SENSOR1_SUCCESS )
    {
      HAL_LOG_ERROR("%s: Error in sensor1_write() %s", __FUNCTION__, strerror(errno));
    }

    /* waiting for response */
    if (hal_wait_for_response( TIME_OUT_MS*10,
                               &g_sensor_control->cb_mutex,
                               &g_sensor_control->cb_arrived_cond,
                               &g_sensor_control->is_resp_arrived ) == false )
    {
      HAL_LOG_ERROR("%s: Request timed out", __FUNCTION__ );
    }
    else
    {
      HAL_LOG_DEBUG("%s: Received Response", __FUNCTION__ );


      hal_sam_add_sensors();
      hal_add_oem_sensors();
      hal_sam_add_sensor_fusion_sensors();
      hal_sam_add_gestures_sensors();
      hal_sam_add_qmd_sensors();
      hal_sam_add_smd_sensor();
      hal_sam_add_geomagnetic_sensor();
      hal_sam_add_step_detector_sensor();
      hal_sam_add_step_counter_sensor();
      hal_add_uncalibrated_sensors();

      hal_sam_init( g_sensor_control );

      if( !g_sensor_control->error )
      {
        for (i=0; i < ARRAY_SIZE(g_sensor_list_order); i++)
        {
          int handle = g_sensor_list_order[i];
          if( g_sensor_control->sensor_list[handle].is_attrib_ok )
          {
            g_sensor_control->available_sensors |= (1ULL << handle);

            sensor_list[sensor_list_len].name = g_sensor_control->sensor_list[handle].name;
            sensor_list[sensor_list_len].vendor = g_sensor_control->sensor_list[handle].vendor;
            sensor_list[sensor_list_len].version = g_sensor_control->sensor_list[handle].version;
            sensor_list[sensor_list_len].handle = g_sensor_control->sensor_list[handle].handle;
            sensor_list[sensor_list_len].type = g_sensor_control->sensor_list[handle].type;
            sensor_list[sensor_list_len].maxRange = g_sensor_control->sensor_list[handle].max_range;
            sensor_list[sensor_list_len].resolution = g_sensor_control->sensor_list[handle].resolution;
            sensor_list[sensor_list_len].power = g_sensor_control->sensor_list[handle].power;

            sensor_type = hal_get_trigger_mode( sensor_list[sensor_list_len].handle );
            sensor_list[sensor_list_len].minDelay =
              ( SENSOR_MODE_EVENT == sensor_type ) ? 0 :
              ( SENSOR_MODE_TRIG == sensor_type ) ? -1 :
              HZ_TO_USEC( g_sensor_control->sensor_list[handle].max_freq );

            /* If we already have a sensor of this type, add " Secondary" to the name */
            for (j=0; j<sensor_list_len; j++)
            {
              if( sensor_list[j].type == sensor_list[sensor_list_len].type )
              {
                strlcat( g_sensor_control->sensor_list[handle].name,
                         " Secondary", SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 );
              }
            }
#ifdef SENSORS_DEVICE_API_VERSION_1_1
            sensor_list[sensor_list_len].fifoReservedEventCount =
              g_sensor_control->sensor_list[handle].max_buffered_samples;
            sensor_list[sensor_list_len].fifoMaxEventCount =
              g_sensor_control->sensor_list[handle].max_buffered_samples;
#endif /* SENSORS_DEVICE_API_VERSION_1_1 */
            sensor_list_len++;
          }
        }
        *list = sensor_list;
      }
      else
      {
        *list = NULL;
        HAL_LOG_ERROR("%s: Error: No sensors found: Er: %d", __FUNCTION__,
             g_sensor_control->error );
      }
    }

    if( g_sensor_control->active_sensors == 0 )
    {
      hal_sensor1_close( g_sensor_control );
    }
  }
  pthread_mutex_unlock( &g_sensor_control->cb_mutex );

  HAL_LOG_INFO( "%s: Number of sensors: %d: ", __FUNCTION__, sensor_list_len );
  return sensor_list_len;
}

/*===========================================================================

  FUNCTION:  _hal_sensors_close

===========================================================================*/
/*!
  @brief
  Closes the sensor control device

  HAL API - This function is called from SensorService's context

  @param dev device
  */
static int
_hal_sensors_close( struct hw_device_t *dev )
{
  hal_sensor_control_t*  sensor_ctl = (void*)dev;

  HAL_LOG_DEBUG("%s", __FUNCTION__ );

  /* free up resources */
  if( sensor_ctl != NULL )
  {
    /* close kernel driver instance if enabled */
    if( g_oem_sensor_control.OEMDevice != NULL )
    {
      int err;
      err = sensors_close( (struct sensors_poll_device_t*)g_oem_sensor_control.OEMDevice );
      if( err != 0 ) {
        HAL_LOG_ERROR( "%s sensors_close() failed %d (%s)", __FUNCTION__, err, strerror(-err) );
      }
    }

    /* close sensor1 */
    sensor1_close( sensor_ctl->hndl );

    /* clean up mutex and cond var  */
    pthread_mutex_destroy( &sensor_ctl->cb_mutex );
    pthread_cond_destroy( &sensor_ctl->cb_arrived_cond );
    pthread_mutex_destroy( &sensor_ctl->data_mutex );
    pthread_cond_destroy( &sensor_ctl->data_arrived_cond );

    /* Time Service */
    hal_time_stop();

    /* Motion Accel */
    hal_ma_destroy();

    /* free up memory */
    free(sensor_ctl);
    g_sensor_control = NULL;
  }

  return SENSOR1_SUCCESS;
}

/*===========================================================================

  FUNCTION:  hal_oem_activate

===========================================================================*/
static bool hal_oem_activate( int handle, int enabled )
{
  int err;
  HAL_LOG_DEBUG( "%s %d", __FUNCTION__, enabled );
  if( NULL == g_oem_sensor_control.OEMDevice )
    return false;

  /* activate/deactivate the kernel driver */
  err = g_oem_sensor_control.OEMDevice->activate( (struct sensors_poll_device_t*)g_oem_sensor_control.OEMDevice, handle, enabled );
  if( err != 0 )
  {
    HAL_LOG_ERROR( "%s: hal_oem_activate() for handle  %d failed with error %d (%s)", __FUNCTION__, handle, err, strerror(-err) );
    return false;
  }
  return true;
}

/*===========================================================================

  FUNCTION:  hal_oem_set_delay

===========================================================================*/
static bool hal_oem_set_delay( int handle, int64_t ns )
{
  int err;
  HAL_LOG_DEBUG("%s %"PRId64, __FUNCTION__, ns );
  if( NULL == g_oem_sensor_control.OEMDevice )
    return false;
  /* activate/deactivate the kernel driver */
  err = g_oem_sensor_control.OEMDevice->setDelay((struct sensors_poll_device_t*)g_oem_sensor_control.OEMDevice, handle, ns);
  if (err != 0)
  {
    HAL_LOG_ERROR("%s: hal_oem_set_delay() for handle  %d failed with error %d (%s)", __FUNCTION__, handle, err, strerror(-err));
    return (false);
  }
  return true;
}

/*===========================================================================

  FUNCTION:  hal_report_delete

===========================================================================*/
/*!
  @brief
  Send a disable/delete request, and update the global state upon success.
  cb_mutex must be held

  @param sensor_ctl sensor control device
  @param handle is the handle of the sensor to change.

  @return 0 on success, negative errno code otherwise
  */
static int
hal_report_delete( hal_sensor_control_t* sensor_ctl, int handle )
{
  int rv = 0, sns1_err;
  const bool wait_for_resp = true;

  sns1_err = hal_sensor1_open( sensor_ctl );
  if( SENSOR1_SUCCESS != sns1_err && SENSOR1_EWOULDBLOCK != sns1_err )
  {
    HAL_LOG_ERROR( "%s: hal_sensor1_open failed ret=%d", __FUNCTION__, sns1_err );
    rv = -1; /* todo - return appropriate errno */
  }
  else if( SENSOR1_EWOULDBLOCK == sns1_err )
  {
    HAL_LOG_WARN( "%s: SENSOR1_EWOULDBLOCK, daemon not ready", __FUNCTION__ );
    rv = 0;
  }
  else if( HANDLE_MAGNETIC_FIELD_SAM == handle )
  {
    /* Do not disable if HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM is enabled */
    if( sensor_ctl->active_sensors & (1ULL << HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM ) )
    {
      HAL_LOG_DEBUG("Disabling MAG_SAM while UNCAL_MAG_SAM active");
      /* May need to adjust the rate */
      if( MAX(sensor_ctl->current_freq[HANDLE_MAGNETIC_FIELD_SAM],
              sensor_ctl->current_freq[HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM] ) !=
          sensor_ctl->current_freq[HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM] )
      {
        HAL_LOG_DEBUG("Disabling MAG_SAM while UNCAL_MAG_SAM active. New uncal rate: %d",
                      sensor_ctl->current_freq[HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM] );
        hal_smgr_report_add( HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM, sensor_ctl,
                             sensor_ctl->current_freq[HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM],
                             sensor_ctl->current_rpt_rate[HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM],
                             sensor_ctl->current_WuFF[HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM],
                             sensor_ctl->current_batching[HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM], wait_for_resp );
        hal_sam_mag_cal_report_add( handle, sensor_ctl,
                                    sensor_ctl->current_freq[HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM] );
      }
    }
    else
    {
      HAL_LOG_DEBUG("Disabling MAG_SAM while UNCAL_MAG_SAM not active");
      hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_MAG_CAL_SVC_ID_V01 );
      rv = 0;
    }
  }
  else if ( HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM == handle )
  {
    /* Do not disable SAM if HANDLE_MAGNETIC_FIELD_SAM is enabled */
    if( sensor_ctl->active_sensors & (1ULL << HANDLE_MAGNETIC_FIELD_SAM ) )
    {
      /* May need to adjust the rate */
      /* May need to adjust the rate */
      if( MAX(sensor_ctl->current_freq[HANDLE_MAGNETIC_FIELD_SAM],
              sensor_ctl->current_freq[HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM] ) !=
          sensor_ctl->current_freq[HANDLE_MAGNETIC_FIELD_SAM] )
      {
        hal_sam_mag_cal_report_add( HANDLE_MAGNETIC_FIELD_SAM, sensor_ctl,
                                    sensor_ctl->current_freq[HANDLE_MAGNETIC_FIELD_SAM] );
      }
    }
    else
    {
      hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_MAG_CAL_SVC_ID_V01 );
      rv = 0;
    }
    rv = hal_smgr_report_delete( handle, sensor_ctl ) ?  0 : -1;
  }
  else if( handle < SAM_HANDLE_BASE )
  {
      rv = hal_smgr_report_delete( handle, sensor_ctl ) ? 0 : -1;
  }
  else if( HANDLE_CMC == handle )
    {
    hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_CMC_SVC_ID_V01 );
    rv = 0;
  }
  else if( HANDLE_PAM == handle )
  {
    hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_PAM_SVC_ID_V01 );
    rv = 0;
  }
#ifdef FEATURE_SNS_HAL_SAM_INT
  else if( HANDLE_GESTURE_FACE_N_SHAKE == handle )
  {
    hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_FNS_SVC_ID_V01 );
    rv = 0;
  }
  else if( HANDLE_GESTURE_BRING_TO_EAR == handle )
  {
    hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_BTE_SVC_ID_V01 );
    rv = 0;
  }
#endif /* FEATURE_SNS_HAL_SAM_INT */
  else if( HANDLE_MOTION_ABSOLUTE == handle )
  {
    hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_AMD_SVC_ID_V01 );
    rv = 0;
  }
  else if( HANDLE_MOTION_VEHICLE  == handle )
  {
    hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_VMD_SVC_ID_V01 );
    rv = 0;
  }
  else if( HANDLE_MOTION_RELATIVE == handle )
  {
    hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_RMD_SVC_ID_V01 );
    rv = 0;
  }
  else if( HANDLE_GESTURE_BASIC_GESTURES == handle )
  {
    hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_BASIC_GESTURES_SVC_ID_V01 );
    rv = 0;
  }
  else if( HANDLE_GESTURE_TAP == handle )
  {
    hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_TAP_SVC_ID_V01 );
    rv = 0;
  }
  else if( HANDLE_GESTURE_GYRO_TAP == handle )
  {
    hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_GYRO_TAP2_SVC_ID_V01 );
    rv = 0;
  }
  else if( HANDLE_GESTURE_FACING == handle )
  {
    hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_FACING_SVC_ID_V01 );
    rv = 0;
  }
  else if( HANDLE_GESTURE_TILT == handle )
  {
    hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_INTEG_ANGLE_SVC_ID_V01 );
    rv = 0;
  }
  else if( HANDLE_GRAVITY == handle ||
           HANDLE_LINEAR_ACCEL == handle ||
           HANDLE_GEOMAGNETIC_ROTATION_VECTOR == handle ||
           HANDLE_ROTATION_VECTOR == handle ||
           HANDLE_ORIENTATION == handle ||
           HANDLE_GAME_ROTATION_VECTOR == handle ||
           HANDLE_PEDOMETER == handle ||
           HANDLE_SAM_STEP_DETECTOR == handle ||
           HANDLE_SAM_STEP_COUNTER == handle ||
           HANDLE_PROXIMITY == handle )
  {
    /* This must be updated prior to calling hal_sam_activate */
    sensor_ctl->active_sensors &= ~(1ULL << handle);

    rv = hal_sam_activate( sensor_ctl, handle );
  }
  else if( HANDLE_MOTION_ACCEL == handle )
  {
     rv = hal_ma_activate( false );
  }
  else if ( HANDLE_SIGNIFICANT_MOTION == handle )
  {
    hal_sam_send_cancel( sensor_ctl->hndl, SNS_SAM_SMD_SVC_ID_V01 );
    rv = 0;
  }
  else if( (HANDLE_OEM_LIGHT == handle || HANDLE_OEM_PROXIMITY == handle) &&
           (g_oem_sensor_control.OEMModule != NULL) )
  {
    rv = hal_oem_activate( g_sensor_control->sensor_list[handle].oem_handle, false ) ? 0 : -1;
  }
  else
  {
    HAL_LOG_WARN( "%s: Unknown sensor handle %i", __FUNCTION__, handle );
  }
  sensor_ctl->current_freq[handle] = 0;
  sensor_ctl->current_rpt_rate[handle] = 0;
  sensor_ctl->current_batching[handle] = 0;
  sensor_ctl->current_WuFF[handle] = 0;

  return rv;
}

/*===========================================================================

  FUNCTION:  is_active

===========================================================================*/
/*!
  @brief
  Returns true if a sensor is active.

  @param handle is the handle of the sensor

  @return true if active. false if not.
  */
static bool
is_active( int handle )
{
  return (g_sensor_control->active_sensors & (1ULL << handle) );
}

/*===========================================================================

  FUNCTION:  _hal_sensors_activate

===========================================================================*/
/*!
  @brief
  Activate/deactivate one sensor

  HAL API - This function is called from SensorService's context

  @param dev sensor control device
  @param handle is the handle of the sensor to change.
  @param enabled set to 1 to enable, or 0 to disable the sensor.

  @return 0 on success, negative errno code otherwise
  */
static int
_hal_sensors_activate( struct sensors_poll_device_t *dev,
                       int handle, int enabled )

{
  int                   ret = -1;
  uint64_t              mask, sensors;
  uint64_t              active_sensors, new_sensors, changed;
  hal_sensor_control_t* sensor_ctl = (void*)dev;
  const uint32_t        report_rate_fastest = 0;

  HAL_LOG_INFO( "%s: handle=%d, enabled=%d", __FUNCTION__, handle, enabled );

  pthread_mutex_lock( &sensor_ctl->cb_mutex );

  /* check current active sensors, enable/disable only if changed */
  mask = (1ULL << handle);
  sensors = enabled ? mask : 0;
  active_sensors = sensor_ctl->active_sensors;
  new_sensors = (active_sensors & ~mask) | (sensors & mask);
  changed = active_sensors ^ new_sensors;

  if( enabled )
  {
    hal_time_start();
  }

  if( !changed )
  {
    HAL_LOG_DEBUG( "%s: No change required: Ac: %llu, New: %llu",
                   __FUNCTION__, active_sensors, new_sensors );
    ret = 0;
  }
  else
  {
    HAL_LOG_INFO( "%s: active_sensors mask=0x%llx handle=%d", __FUNCTION__,
                  sensor_ctl->active_sensors, handle );
    if( !enabled )
    {
      if( (sensor_ctl->changed_sensors & (1ULL << handle)) != 0 )
      {
        sensor_ctl->changed_sensors &= (~(1ULL << handle));
        HAL_LOG_WARN( "%s: Deactivating dirty sensor %d", __FUNCTION__, handle );
      }
      else
      {
        sensor_ctl->changed_sensors |= (1ULL << handle);
      }
      ret = 0;
    }
    else
    {
      sensor_ctl->changed_sensors |= (1ULL << handle);

      if( (HANDLE_OEM_LIGHT == handle || HANDLE_OEM_PROXIMITY == handle) &&
          (g_oem_sensor_control.OEMModule != NULL) )
      {
        if( !hal_oem_activate( sensor_ctl->sensor_list[handle].oem_handle, enabled ) )
        {
          HAL_LOG_ERROR( "%s: activate() for handle  %d failed", __FUNCTION__,
                         sensor_ctl->sensor_list[handle].handle );
          ret = -1;
        }
      }
      else
      {
        ret = 0;
      }
    }
  }

  sensor_ctl->active_sensors = new_sensors;
  pthread_mutex_unlock( &sensor_ctl->cb_mutex );

  if( ret == 0 && changed )
  {
    hal_schedule_acquire_resources();
  }

  return ret;
}

/*===========================================================================

  FUNCTION:  hal_is_event_sensor

===========================================================================*/
/*!
  @brief
  Checks if the given sensor at the specified rate streams data constantly,
  or only upon some specific event.

  @param handle Handle provided by Android Sensor Service
  @param report_rate Streaming rate (in Hz)

  @return True if an event-based sensor, false if streaming sensor.
  */
static bool
hal_is_event_sensor( int handle, uint32_t report_rate )
{
  bool rv = false;

  // Sensors that are always event-based
  if( HANDLE_PAM == handle ||
      HANDLE_PROXIMITY == handle ||
      HANDLE_LIGHT == handle ||
      HANDLE_GESTURE_BASIC_GESTURES == handle ||
      HANDLE_GESTURE_TAP == handle ||
      HANDLE_GESTURE_FACING == handle ||
      HANDLE_GESTURE_FACE_N_SHAKE == handle ||
      HANDLE_GESTURE_BRING_TO_EAR == handle ||
      HANDLE_GESTURE_GYRO_TAP == handle ||
      HANDLE_MOTION_ACCEL == handle ||
      HANDLE_SIGNIFICANT_MOTION == handle ||
      HANDLE_SMGR_STEP_DETECTOR == handle ||
      HANDLE_SMGR_STEP_COUNT == handle ||
      HANDLE_SAM_STEP_DETECTOR == handle ||
      HANDLE_SAM_STEP_COUNTER == handle ||
      HANDLE_SMGR_SMD == handle ||
      HANDLE_RELATIVE_HUMIDITY == handle ||
      HANDLE_AMBIENT_TEMPERATURE == handle ||
      HANDLE_CMC == handle ||
      HANDLE_RGB == handle ||
      HANDLE_IR_GESTURE == handle ||
      HANDLE_SAR == handle )
  {
    rv = true;
  }
  // Sensors that may be event-based
  else if( HANDLE_PEDOMETER == handle ||
           HANDLE_MOTION_ABSOLUTE == handle ||
           HANDLE_MOTION_RELATIVE == handle ||
           HANDLE_MOTION_VEHICLE == handle )
  {
    if( 0 == report_rate )
    {
      rv = true;
    }
    else
    {
      rv = false;
    }
  }
  // Sensors that are always streaming
  else if( HANDLE_ACCELERATION == handle ||
           HANDLE_MAGNETIC_FIELD_SAM == handle ||
           HANDLE_GESTURE_TILT == handle ||
           HANDLE_MAGNETIC_FIELD == handle ||
           HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM == handle ||
           HANDLE_MAGNETIC_FIELD_UNCALIBRATED == handle ||
           HANDLE_ORIENTATION == handle ||
           HANDLE_GYRO == handle ||
           HANDLE_GYRO_UNCALIBRATED == handle ||
           HANDLE_PRESSURE == handle ||
           HANDLE_GRAVITY == handle ||
           HANDLE_LINEAR_ACCEL == handle ||
           HANDLE_ROTATION_VECTOR == handle ||
           HANDLE_GEOMAGNETIC_ROTATION_VECTOR == handle ||
           HANDLE_SMGR_GAME_RV == handle ||
           HANDLE_GAME_ROTATION_VECTOR == handle )
  {
    rv = false;
  }
  else
  {
    HAL_LOG_ERROR( "%s: !!!Unknown Sensor!!! handle: %i rate: %i",
                   __FUNCTION__, handle, report_rate );
    rv = false;
  }

  return rv;
}

/*===========================================================================

  FUNCTION:  hal_calc_sample_rate

===========================================================================*/
/*!
  @brief
  Calculates the appropriate reporting rate based on sensor type and other
  criteria.

  @param sensor_ctl sensor control device
  @param handle Handle provided by Android Sensor Service
  @param ns Streaming rate requested in ns (note value is >= 0)

  @return Calculated report rate (in Hz)
  */
static uint32_t
hal_calc_sample_rate( hal_sensor_control_t *sensor_ctl, int handle, uint64_t ns )
{
  int max_freq = (int)sensor_ctl->sensor_list[ handle ].max_freq;
  uint32_t sample_rate = 0;

  max_freq = ( 0 == max_freq ) ? 1 : max_freq;

  if( hal_is_event_sensor( handle, 0 ) )
  {
    sample_rate = max_freq;
  }
  /* Convert frequency from Android values to sensor1 values. Only support
   * rates bettween "fastest" and 1Hz. If it's slower than 1Hz, request 1Hz */
  else if( ns <= 1000000000 )
  {
    if( ns == 0 )
    {
      ns = 1;
    }

    /* frequency in Hz */
    sample_rate = lroundf( NSEC_TO_HZ(ns) );

    /* limit rate to max allowed */
    if( sample_rate > (uint32_t)max_freq )
    {
      sample_rate = (uint32_t)max_freq;
    }
  }
  else
  {
    /* default to 1 Hz */
    sample_rate = 1;
  }

  return sample_rate;
}

/*===========================================================================

  FUNCTION:  hal_apply_rate

===========================================================================*/
/*!
  @brief
  Determines which SMGR or SAM sensor to enable, and sends the appropriate
  request.  cb_mutex must be held.

  @param[i] sensor_ctl sensor control device
  @param[i] handle Handle provided by Android Sensor Service
  @param[i] report_rate Streaming rate in ns (note value is >= 0)
  @param[i] ignore_prev_value Whether to take into account the previous streaming
              value; used for SSR and non-streaming sensors.

  @return True if sensor was enabled sucessfully, false otherwise.
  */
static bool
hal_apply_rate( hal_sensor_control_t *sensor_ctl, int handle,
                uint32_t sample_rate, uint32_t rpt_rate, bool WuFF, bool do_buffering, bool ignore_prev_value )
{
  bool result = false;
  const bool wait_for_resp = true;

  HAL_LOG_DEBUG( "%s: handle %d, sample rate:%d report rate:%d WuFF:%d buffering:%d ignore_prev:%d",
                 __FUNCTION__, handle, sample_rate, rpt_rate, WuFF, do_buffering, ignore_prev_value );

  if( handle < SAM_HANDLE_BASE )
  {
    result = hal_smgr_report_add( handle, sensor_ctl, sample_rate, rpt_rate, WuFF, do_buffering, wait_for_resp );
  }
  else if( handle == HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM )
  {
    uint32_t max_sample_rate = MAX( sample_rate, sensor_ctl->current_freq[HANDLE_MAGNETIC_FIELD_SAM] );
    hal_smgr_report_add( handle, sensor_ctl, max_sample_rate, rpt_rate, WuFF, do_buffering, wait_for_resp );
    result = hal_sam_mag_cal_report_add( HANDLE_MAGNETIC_FIELD_SAM, sensor_ctl, max_sample_rate );
  }
  else if( handle == HANDLE_MAGNETIC_FIELD_SAM )
  {
    uint32_t max_sample_rate = MAX( sample_rate, sensor_ctl->current_freq[HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM] );
    if( sensor_ctl->active_sensors & (1ULL << HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM) )
    {
      /* If uncalibrated mag is active, set SMGR to the same rate */
      hal_smgr_report_add( HANDLE_MAGNETIC_FIELD_UNCALIBRATED_SAM,
                           sensor_ctl, max_sample_rate, rpt_rate, WuFF, do_buffering, wait_for_resp );
    }
    result = hal_sam_mag_cal_report_add( handle, sensor_ctl, max_sample_rate );
  }
  else if( handle == HANDLE_CMC )
  {
    result = hal_sam_cmc_report_add( handle, sensor_ctl, sample_rate );
  }
  else if( handle == HANDLE_PAM )
  {
    result = hal_sam_pam_report_add( handle, sensor_ctl, sample_rate );
  }
  else if ( ( handle == HANDLE_SIGNIFICANT_MOTION ) )
  {
    result = hal_sam_smd_report_add( handle, sensor_ctl, sample_rate );
  }
#ifdef FEATURE_SNS_HAL_SAM_INT
  else if( handle == HANDLE_GESTURE_FACE_N_SHAKE )
  {
    result = hal_sam_fns_report_add( handle, sensor_ctl ); /* FNS doesn't support report period */
  }
  else if( handle == HANDLE_GESTURE_BRING_TO_EAR )
  {
    result = hal_sam_bte_report_add( handle, sensor_ctl ); /* FNS doesn't support report period */
  }
#endif /* FEATURE_SNS_HAL_SAM_INT */
  else if( ( handle == HANDLE_MOTION_ABSOLUTE ) ||
            ( handle == HANDLE_MOTION_VEHICLE ) )
  {
    result = hal_sam_qmd_report_add( handle, sensor_ctl, sample_rate );
  }
  else if( handle == HANDLE_MOTION_RELATIVE )
  {
    result = hal_sam_qmd_report_add( handle, sensor_ctl, sample_rate );
  }
  else if( handle == HANDLE_GESTURE_BASIC_GESTURES )
  {
    /*  Basic Gestures doesn't support report period */
    result = ( 1 == sensor_ctl->current_freq[ handle ] && !ignore_prev_value )
      ? true : hal_sam_basic_gestures_report_add( handle, sensor_ctl );
  }
  else if( handle == HANDLE_GESTURE_TAP )
  {
    /* Tap doesn't support report period */
    result = ( 1 == sensor_ctl->current_freq[ handle ] && !ignore_prev_value )
      ? true : hal_sam_tap_report_add( handle, sensor_ctl );
  }
  else if( handle == HANDLE_GESTURE_GYRO_TAP )
  {
    /* Gyro Tap doesn't support report period */
    result = ( 1 == sensor_ctl->current_freq[ handle ] && !ignore_prev_value )
      ? true : hal_sam_gyro_tap_report_add( handle, sensor_ctl );
  }
  else if( handle == HANDLE_GESTURE_FACING )
  {
    /* Facing doesn't support report period */
    result = ( 1 == sensor_ctl->current_freq[ handle ] && !ignore_prev_value )
      ? true : hal_sam_facing_report_add( handle, sensor_ctl );
  }
  else if( handle == HANDLE_GESTURE_TILT )
  {
    result = hal_sam_tilt_report_add( handle, sensor_ctl, sample_rate );
  }
  else if( HANDLE_GRAVITY == handle ||
           HANDLE_LINEAR_ACCEL == handle ||
           HANDLE_ROTATION_VECTOR == handle ||
           HANDLE_ORIENTATION == handle ||
           HANDLE_GEOMAGNETIC_ROTATION_VECTOR == handle ||
           HANDLE_GAME_ROTATION_VECTOR == handle ||
           HANDLE_PEDOMETER == handle ||
           HANDLE_SAM_STEP_DETECTOR == handle ||
           HANDLE_SAM_STEP_COUNTER == handle ||
           HANDLE_PROXIMITY == handle )
  {
    result = (0 == hal_sam_activate( sensor_ctl, handle )) ? true : false;
  }
  else if( HANDLE_MOTION_ACCEL == handle )
  {
    result = 0 == hal_ma_activate( true ) ? true : false;
  }
  else
  {
    HAL_LOG_ERROR( "%s, invalid handle %d", __FUNCTION__, handle );
    result = false;
  }

  return result;
}


/*===========================================================================

  FUNCTION:  _hal_sensors_set_delay

===========================================================================*/
/*!
  @brief
  Set the delay between sensor events in ns

  HAL API - This function is called from SensorService's context

  @param dev sensor control device
  @param ms data sampling rate in ns

  @return 0 if successful, < 0 on error
  */
static int
_hal_sensors_set_delay( struct sensors_poll_device_t *dev, int handle, int64_t ns )
{
  const int flags = 0;
  const int timeout = 0;
  hal_sensor_control_t  *sensor_ctl = (void*)dev;
  HAL_LOG_INFO( "%s: period_ns=%"PRId64" Hndl: %"PRIdPTR" sns_hndl: %d", __FUNCTION__,
                ns, (intptr_t)(sensor_ctl->hndl), handle );
  return _hal_sensors_batch( (sensors_poll_device_1*) dev, handle, flags, ns, timeout );
}

/*===========================================================================

  FUNCTION:  hal_check_batch_params

===========================================================================*/
/*!
  @brief
  Set batching parameters for a sensor.

  HAL API - This function is called from SensorService's context

  @return 0 if successful, < 0 on error
  */
static int hal_check_batch_params( sensors_poll_device_1* dev,
    int handle, int flags, int64_t period_ns, int64_t timeout_ns)
{
  int num_samples;
  uint32_t sample_rate_hz;
  hal_sensor_control_t  *sensor_ctl = (void*)dev;

  if( timeout_ns == 0 ) {
    return 0;
  }
  if( sensor_ctl->sensor_list[handle].max_buffered_samples == 0 ) {
    HAL_LOG_INFO("%s: Batch request for non-batching sensor. handle %d", __FUNCTION__, handle );
    return -EINVAL;
  }
  if( !(flags & SENSORS_BATCH_WAKE_UPON_FIFO_FULL ) ) {
    return 0;
  }

  sample_rate_hz = hal_calc_sample_rate( sensor_ctl, handle, period_ns);
  num_samples = period_ns * sample_rate_hz / NSEC_PER_SEC;

  if( num_samples < 1 ) {
    num_samples = 1;
  }

  if( sensor_ctl->sensor_list[handle].max_buffered_samples/num_samples >= WUFF_MIN_DURATION_SEC ) {
    return 0;
  }

  return -EINVAL;
}

/*===========================================================================

  FUNCTION:  _hal_sensors_batch

===========================================================================*/
/*!
  @brief
  Set batching parameters for a sensor.

  HAL API - This function is called from SensorService's context

  @return 0 if successful, < 0 on error
  */
static int _hal_sensors_batch( sensors_poll_device_1* dev,
    int handle, int flags, int64_t period_ns, int64_t timeout)
{
  sensor1_error_e       error;
  uint32_t              sample_rate = 0;
  uint32_t              report_rate = 0;
  hal_sensor_control_t  *sensor_ctl = (void*)dev;
  const bool            do_buffering = timeout != 0;
  const bool            WuFF = (flags & SENSORS_BATCH_WAKE_UPON_FIFO_FULL) == SENSORS_BATCH_WAKE_UPON_FIFO_FULL;
  int                   batch_params_ok;

  HAL_LOG_INFO( "%s: period_ns=%"PRId64" sns_hndl: %d timeout: %"PRId64" flags:0x%x",
                __FUNCTION__, period_ns, handle, timeout, flags );

  if( 0 > period_ns )
  {
    HAL_LOG_ERROR( "Error in %s: argument ns < 0", __FUNCTION__ );
    return -1;
  }
  sample_rate = hal_calc_sample_rate( sensor_ctl, handle, (uint64_t)period_ns );
  if( timeout == 0 ) {
    report_rate = 0;
  } else {
    report_rate = FX_FLTTOFIX_Q16( NSEC_TO_HZ(timeout) );
  }

  batch_params_ok = hal_check_batch_params( dev, handle, flags, period_ns, timeout);
  if( (batch_params_ok != 0) ||
      (flags & SENSORS_BATCH_DRY_RUN) ) {
    return batch_params_ok;
  }

  /* Force the next sample-equality test to fail, even if the data
   * is the same (Light Sensor) */
  pthread_mutex_lock( &g_sensor_control->data_mutex );
  sensor_ctl->last_event[handle].reserved1[0] += 1;
  pthread_mutex_unlock( &g_sensor_control->data_mutex );

  pthread_mutex_lock( &sensor_ctl->cb_mutex );

  HAL_LOG_DEBUG( "%s: sample_rate=%u report_rate=%u curr sample rate:%u cur rpt rate:%u max:%f min:%f",
                 __FUNCTION__, sample_rate, report_rate,
                 sensor_ctl->current_freq[ handle ],
                 sensor_ctl->current_rpt_rate[ handle ],
                 sensor_ctl->sensor_list[ handle ].max_freq,
                 sensor_ctl->sensor_list[ handle ].min_freq );

  if( sample_rate  == sensor_ctl->current_freq[handle] &&
      report_rate  == sensor_ctl->current_rpt_rate[handle] &&
      do_buffering == sensor_ctl->current_batching[handle] &&
      WuFF         == sensor_ctl->current_WuFF[handle] )
  {
    pthread_mutex_unlock( &sensor_ctl->cb_mutex );
    HAL_LOG_INFO( "%s: current sample & report rate, buffering, and WuFF are equal to requested (%i,%i,%d,%d)",
                  __FUNCTION__, sample_rate, report_rate, do_buffering, WuFF );
  }
  else
  {
    sensor_ctl->current_freq[ handle ] = sample_rate;
    sensor_ctl->current_rpt_rate[handle] = report_rate;
    sensor_ctl->current_batching[handle] = do_buffering;
    sensor_ctl->current_WuFF[handle] = WuFF;
    sensor_ctl->changed_sensors |= (1ULL << handle );
    pthread_mutex_unlock( &sensor_ctl->cb_mutex );
    hal_schedule_acquire_resources();
  }

  return 0;
}


/*===========================================================================

  FUNCTION:  _hal_sensors_flush

===========================================================================*/
/*!
  @brief
  Flushes FIFO for a sensor

  HAL API - This function is called from SensorService's context

  @return 0 if successful, < 0 on error
  */
static int _hal_sensors_flush( sensors_poll_device_1* dev, int handle )
{
  int rv = -EINVAL;
  bool sensor_active,
       current_batching;

  HAL_LOG_INFO( "%s: handle=%d", __FUNCTION__, handle );

  pthread_mutex_lock( &g_sensor_control->data_mutex );
  g_sensor_control->flush_requested[handle] = true;
  pthread_mutex_unlock( &g_sensor_control->data_mutex );

  pthread_mutex_lock( &g_sensor_control->cb_mutex );
  sensor_active = ((1ULL << handle) & g_sensor_control->active_sensors) != 0;
  current_batching = g_sensor_control->current_batching[handle];

  if( sensor_active && current_batching )
  {
    hal_flush_request( handle );
    rv = 0;
  }
  pthread_mutex_unlock( &g_sensor_control->cb_mutex );

  pthread_mutex_lock( &g_sensor_control->data_mutex );
  if( !sensor_active )
  {
    g_sensor_control->flush_requested[handle] = false;
    HAL_LOG_DEBUG( "%s: handle %d is inactive", __FUNCTION__, handle );
  }
  else if( !current_batching )
  {
    HAL_LOG_DEBUG( "%s: handle %d is not batching", __FUNCTION__, handle );
    g_sensor_control->flush_requested[handle] = true;
    hal_flush_send_cmplt( handle );
    rv = 0;
  }
  pthread_mutex_unlock( &g_sensor_control->data_mutex );

  return rv;
}

/*===========================================================================

  FUNCTION:  hal_oem_data_poll

===========================================================================*/
static void*
hal_oem_data_poll( void *ptr )
{
  sensors_event_t buf[2];
  int n;
  int count;
  bool result;
  HAL_LOG_INFO("%s", __FUNCTION__);

  pthread_mutex_lock( &g_sensor_control->data_mutex );
  g_oem_sensor_control.threadCounter = 1;
  pthread_mutex_unlock( &g_sensor_control->data_mutex );

  if( NULL == g_oem_sensor_control.OEMDevice )
    return( 0 );

  while(1)
  {
    n = g_oem_sensor_control.OEMDevice->poll( (struct sensors_poll_device_t*)g_oem_sensor_control.OEMDevice, buf, 2 );
    if( n< 0 )
    {
      HAL_LOG_ERROR("%s: poll() failed", __FUNCTION__ );
      break;
    }

    for(count=0;count<n;count++)
    {
      if( buf[count].type != SENSOR_TYPE_LIGHT && buf[count].type != SENSOR_TYPE_PROXIMITY )
        continue;

      if( SENSOR_TYPE_LIGHT == buf[count].type )
      {
        buf[count].sensor = HANDLE_OEM_LIGHT;
        HAL_LOG_DEBUG("%s: Received LIGHT DATA loopCnt-%d light value-%f", __FUNCTION__,count, buf[count].light);
      }
      if( SENSOR_TYPE_PROXIMITY == buf[count].type )
      {
        buf[count].sensor = HANDLE_OEM_PROXIMITY;
        HAL_LOG_DEBUG("%s: Received PROXIMITY DATA loopCnt-%d prox value-%f", __FUNCTION__,count, buf[count].distance);
      }

      pthread_mutex_lock( &g_sensor_control->data_mutex );

      result = hal_insert_queue( &buf[count] );
      if( result )
      {
        hal_signal_ind( &g_sensor_control->data_arrived_cond );
      }
      else
      {
        HAL_LOG_ERROR("%s: Unable to insert message to queue", __FUNCTION__ );
      }
      pthread_mutex_unlock( &g_sensor_control->data_mutex );
    }
  }
  return( 0 );
}

/*===========================================================================

  FUNCTION:  _hal_sensors_data_poll

===========================================================================*/
/*!
  @brief
  Returns an array of sensor data. This function must block until events are available.
  HAL API - This function is called from application's context

  @return the number of events read on success, or -errno in case of an error.
  This function should never return 0 (no event).
 */
static int
_hal_sensors_data_poll( struct sensors_poll_device_t *dev,
                        sensors_event_t* data, int count )
{
  int i = 0;
  hal_sensor_control_t*   sensor_ctl = (void*)dev;
  pthread_attr_t          thread_attr;

  HAL_LOG_DEBUG("%s: cnt: %d", __FUNCTION__, count );

  /* create the oem polling thread */
  if( g_oem_sensor_control.OEMModule != NULL )
  {
    if(g_oem_sensor_control.threadCounter == 0)
    {
      if( 0 == pthread_attr_init( &thread_attr) )
      {
        if( 0 == pthread_attr_setdetachstate( &thread_attr, PTHREAD_CREATE_DETACHED ) )
        {
          pthread_create( &g_oem_sensor_control.OEM_poll_thread, &thread_attr,
                          hal_oem_data_poll, NULL );
          HAL_LOG_INFO("%s: Created OEM poll thread", __FUNCTION__ );
        }
        pthread_attr_destroy( &thread_attr );
      }
    }
  }

  if( NULL == g_sensor_control )
  {
    HAL_LOG_ERROR("%s: HAL control init failed", __FUNCTION__ );
    return -1;
  }

  pthread_mutex_lock( &sensor_ctl->data_mutex );

  while( i < count )
  {
    /* check if any responses have been buffered */
    if( !hal_remove_from_queue( &data[i] ) )
    {
      break;
    }
    i++;
  }

  while( i == 0 )
  {
    sensor_ctl->is_ind_arrived = false;

    /* wait for notify cb - wait indefinitely */
    if( hal_wait_for_response( 0, &sensor_ctl->data_mutex,
                               &sensor_ctl->data_arrived_cond,
                               &sensor_ctl->is_ind_arrived ) == false)
    {
      pthread_mutex_unlock( &sensor_ctl->data_mutex );
      return (-ETIMEDOUT);
    }

    /* Data received */
    while( i < count && hal_remove_from_queue( &data[i] ) )
    {
      i++;
    }
  }

  if( g_sensor_control->is_ltcy_measure_enabled )
  {
    struct timespec current_time;
    uint64_t curr_timestamp;
    int apps_err = clock_gettime( CLOCK_REALTIME, &current_time );
    if( 0 != apps_err )
    {
      HAL_LOG_ERROR( "%s: Apps time error %s(%i)", __FUNCTION__, strerror(errno), errno );
    }
    else
    {
      curr_timestamp = ((uint64_t)current_time.tv_sec * 1000000000) + current_time.tv_nsec;
      hal_ltcy_measure( curr_timestamp, data, i );
    }
  }

  pthread_mutex_unlock( &sensor_ctl->data_mutex );
  HAL_LOG_DEBUG("%s: poll data:%d;data[0].sensor:%d .type:%d .x:%f .y:%f .z:%f", __FUNCTION__, i,
       data[0].sensor, data[0].type, data[0].acceleration.x, data[0].acceleration.y, data[0].acceleration.z );
  return (i);
}

/*===========================================================================



===========================================================================*/
static struct hw_module_methods_t sensors_module_methods = {
    .open = _hal_sensors_open
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = SENSORS_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = SENSORS_HARDWARE_MODULE_ID,
        .name = "Qualcomm Sensors Module",
        .author = "Qualcomm Technologies, Inc.",
        .methods = &sensors_module_methods,
    },
    .get_sensors_list = _hal_sensors_get_sensors_list
};

/*===========================================================================

  FUNCTION:  hal_cleanup_resources
  Cleans up resources such as sensor1 connection, and internal state variables

  This function is called when the sensor daemon goes down, i.e. upon
  receiving BROKEN_PIPE message

  Return void

===========================================================================*/
static void hal_cleanup_resources()
{
  int i;
  hal_sensor_control_t* sensor_ctl = g_sensor_control;
  HAL_LOG_DEBUG( "%s", __FUNCTION__ );

  // close sensor1 connection
  if( sensor_ctl->hndl != (sensor1_handle_s*)-1)
  {
    sensor1_close( sensor_ctl->hndl );
    sensor_ctl->hndl = (sensor1_handle_s*)-1;
  }

  // Signal any waiting request
  hal_signal_response( false, &g_sensor_control->cb_arrived_cond );

  // reset some variables
  sensor_ctl->is_ind_arrived = false;
  sensor_ctl->error = false;
  sensor_ctl->last_event[HANDLE_PROXIMITY].distance = -1;
  sensor_ctl->last_event[HANDLE_LIGHT].light = -1;

  /* Motion Accel */
  hal_ma_init( true );

  for( i=0; i<MAX_NUM_SENSORS; i++ )
  {
    sensor_ctl->last_event[i].timestamp = 0;
  }

  hal_sam_init( sensor_ctl );

  for( i=0; i<MAX_SAM_SERVICES; i++ )
  {
    sensor_ctl->sam_service[ i ].instance_id = INVALID_INSTANCE_ID;
  }
}

/*===========================================================================

  FUNCTION:  hal_reinit

  Re-intializes the HAL. This is called when the sensor daemon
  has crashed and restarted.  Must hold cb_mutex

  Returns error code if error, otherwise SENSOR1_SUCCESS if success

===========================================================================*/
static int hal_reinit()
{
  hal_sensor_control_t* sensor_ctl = g_sensor_control;
  int ret = SENSOR1_EFAILED;

  HAL_LOG_DEBUG("%s", __FUNCTION__ );

  if( sensor_ctl->hndl == (sensor1_handle_s*)-1 )
  {
    ret = sensor1_open( &sensor_ctl->hndl, &hal_sensor1_data_cb,
                          (intptr_t)sensor_ctl );
    HAL_LOG_DEBUG("%s: sensor1_open() ret=%d hndl=%"PRIuPTR, __FUNCTION__, ret,
                  (uintptr_t)(sensor_ctl->hndl) );

    // re-acquire resources if the daemon is ready
    // otherwise callback will invoked with RETRY and we need to call this again
    if( ret == SENSOR1_SUCCESS )
    {
      hal_ma_init(true);
      hal_schedule_acquire_resources();
    }
    else if( ret == SENSOR1_EWOULDBLOCK )
    {
      HAL_LOG_ERROR( "%s: sensor1_open returned EWOULDBLOCK. Daemon not ready, will try again", __FUNCTION__ );
    }
    else
    {
      HAL_LOG_ERROR( "%s: sensor1_open() failed ret=%d", __FUNCTION__, ret );
    }
  }

  return ret;
}

/*===========================================================================

  FUNCTION:  hal_acquire_resources

  Aquire resources by sending SMRG/SAM enable requests for each active
  sensor in the list

  This function MUST be called in a seperate thread to avoid blocking the
  sensor1 callback

  Return error if any of the enabling request failed

===========================================================================*/
static void *hal_acquire_resources( void *unused )
{
  sensor1_error_e rv = SENSOR1_SUCCESS;
  sensor1_error_e sns1_err;
  int handle;
  int rpt_rate;
  int sample_rate;
  bool WuFF;
  bool batching;
  hal_sensor_control_t* sensor_ctl = g_sensor_control;
  const bool wait_for_resp = true;

  HAL_LOG_DEBUG( "%s", __FUNCTION__ );

  pthread_mutex_lock( &sensor_ctl->acquire_resources_mutex );
  pthread_mutex_lock( &sensor_ctl->cb_mutex );
  sensor_ctl->is_resp_arrived = false;

  // go thru the active list and re-request sensor data
  for( handle = 0; handle < MAX_NUM_SENSORS; handle++ )
  {
    if( sensor_ctl->changed_sensors & (1ULL << handle) )
    {
      sns1_err = hal_sensor1_open( sensor_ctl );
      if( SENSOR1_SUCCESS != sns1_err )
      {
        HAL_LOG_ERROR( "%s: hal_sensor1_open failed %d", __FUNCTION__, sns1_err );
        rv = sns1_err;
        break;
      }

      sensor_ctl->changed_sensors &= ~(1ULL << handle);
      if( sensor_ctl->active_sensors & (1ULL << handle) )
      {
        hal_time_start();

        if( 0 == sensor_ctl->current_freq[handle] )
        {
          /* By default, enable sensors at the fastest rate */
          sensor_ctl->current_freq[handle] = hal_calc_sample_rate( sensor_ctl, handle, 0 );
        }

        sample_rate = sensor_ctl->current_freq[handle];
        rpt_rate = sensor_ctl->current_rpt_rate[handle];
        batching = sensor_ctl->current_batching[handle];
        WuFF = sensor_ctl->current_WuFF[handle];
        HAL_LOG_DEBUG( "%s: found active sensor handle=%d, sample_rate=%d report_rate=%d WuFF=%d batched=%d",
                       __FUNCTION__, handle, sample_rate, rpt_rate, WuFF, batching );

        if( !hal_apply_rate( sensor_ctl, handle, sample_rate, rpt_rate, WuFF, batching, wait_for_resp ) )
        {
          HAL_LOG_ERROR( "%s: Failed for handle %d @ samp %d Hz rpt %d Hz WuFF %d batched %d",
                         __FUNCTION__, handle, sample_rate, rpt_rate, WuFF, batching );
          sensor_ctl->changed_sensors |= (1ULL << handle);
        }
      }
      else
      {
        HAL_LOG_DEBUG( "%s: Deactivating sensor handle=%d",
                       __FUNCTION__, handle );
        if( 0 != hal_report_delete( sensor_ctl, handle ) )
        {
          HAL_LOG_ERROR( "%s: Failed to deactivate sensor handle=%d",
                         __FUNCTION__, handle );
        }
      }
    }
  }

  if( 0 != sensor_ctl->changed_sensors )
  {
    hal_schedule_acquire_resources();
  }

  if( 0 == sensor_ctl->active_sensors )
  {
    hal_sensor1_close( g_sensor_control );
    hal_time_stop();
  }

  pthread_mutex_unlock( &sensor_ctl->cb_mutex );
  pthread_mutex_unlock( &sensor_ctl->acquire_resources_mutex );

  return (void*)((intptr_t)rv);
}
