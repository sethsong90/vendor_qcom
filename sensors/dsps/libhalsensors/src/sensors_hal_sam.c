/*============================================================================

  @file sensors_hal_sam.c

  @brief
  Functionality and processing for SAM-based sensors.

  To add a new SAM sensor to the HAL:
  1.) Create new handle (if applicable).  See HANDLE_GRAVITY.
  2.) Implement hal_sam_*_info, hal_sam_parse_ind_func, hal_sam_init_func
      for the given algorithm in a new file.
  3.) Add new function declaration for hal_sam_*_info to sensors_hal.h.
  4.) Add the new file to ../Android.mk.
  5.) Add cases for the new sensor in gen_sample_funcs and hal_sam_init().
  6.) Update hal_report_delete, hal_apply_rate, and hal_sensor1_data_cb for
      the new handle/service cases.

  FUTURE WORK:
  - Create common functions for *_send_enable, *_send_delete, *_send_batch,
    *_send_batch_update. (Improves #2 above)
  - Move all explicit checks in #6 to the "else" block.  (Removes #6 above).

  Copyright (c) 2013-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
============================================================================*/

/*===========================================================================
                           INCLUDE FILES
===========================================================================*/
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include "sensors_hal.h"
#include "sensor1.h"
#include "fixed_point.h"

/*===========================================================================
                           PREPROCESSOR DEFINITIONS
===========================================================================*/
#define SNS_ROT_VEC_HEAD_ERR_EST_RAD (M_PI / 6.f)

/*===========================================================================
                         STATIC VARIABLES
===========================================================================*/

/**
 * Static list of SAM sensors.  svc_num will be set to -1 for invalid indices
 * Indexed on service number instead of sensor handle because:
 *  - SAM sensor is only looked-up by handle during activation
 *  - SAM sensor is looked-up for every indication by service number
 */
static hal_sam_sensor_t sam_sensors[ MAX_SAM_SERVICES ];

/*===========================================================================
                         TYPE DEFINITIONS
===========================================================================*/

typedef int (*hal_sam_gen_sample)
  ( struct hal_sensor_control_t* sensor_ctl, hal_sam_sample_t *sam_sample, sensors_event_t *la_sample );

/*==========================================================================
                         STATIC FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION:  hal_sam_find_fastest
===========================================================================*/
/**
  @brief
  Given sensor handles, this function returns the fastest of the handles.
  Ignores inactive handles.

  @param[i] sensor_ctl sensor control device
  @param[i] handle Android Sensor handles; -1 if invalid
  @param[i] search_values Array in which to search; indexed using values in handle.

  @return Fastest sensor handle amoung the handles passed as arguments.  -1
          if none of the sensors are enabled.
*/
int hal_sam_find_fastest( hal_sensor_control_t *sensor_ctl,
    int handle[ MAX_SAM_SHARED_ALGO ], uint32_t search_values[] )
{

  int fastest = -1;
  unsigned int max_freq = 0;
  unsigned int i;
  unsigned int freq[MAX_SAM_SHARED_ALGO];

  for( i = 0; i < MAX_SAM_SHARED_ALGO; i++ )
  {
    if( -1 == handle[ i ] )
    {
      continue;
    }

    freq[ i ] = search_values[ handle[ i ] ];

    if( max_freq < freq[ i ] &&
        sensor_ctl->active_sensors & (1ULL << handle[ i ]) )
    {
      max_freq = freq[ i ];
      fastest = i;
    }
  }

  return ( -1 == fastest ) ? -1 : handle[ fastest ];
}

/*===========================================================================
  FUNCTION:  hal_sam_algo_activate
===========================================================================*/
/**
 * Update the HAL's use of the applicable function.
 * Must hold cb_mutex prior to calling this function.
 *
 * @param[i] sensor_ctl Global HAL state
 * @param[i] algo_funcs Pointers to algorithm-specific functions and
 *                      Android Sensor Handles using this SAM algo
 *
 * @return 0 upon success, error otherwise
 */
static int
hal_sam_algo_activate( hal_sensor_control_t *sensor_ctl, hal_sam_sensor_t *sam_sensor )
{
  uint32_t sample_rate[ MAX_SAM_SHARED_ALGO ],
           report_rate[ MAX_SAM_SHARED_ALGO ],
           fastest_freq = 0;
  bool wuff[ MAX_SAM_SHARED_ALGO ],
       batching = true;
  int i,
      handle_fastest,
      wuff_handle[ MAX_SAM_SHARED_ALGO ];

  for( i = 0; i < MAX_SAM_SHARED_ALGO; i++ )
  {
    if( -1 != sam_sensor->handles[ i ] &&
        sensor_ctl->active_sensors & (1ULL << sam_sensor->handles[ i ]) )
    {
      sample_rate[ i ] = sensor_ctl->current_freq[ sam_sensor->handles[ i ] ];
      report_rate[ i ] = sensor_ctl->current_rpt_rate[ sam_sensor->handles[ i ] ];
      wuff[ i ] = sensor_ctl->current_WuFF[ sam_sensor->handles[ i ] ];
      wuff_handle[ i ] = wuff[ i ] ? sam_sensor->handles[ i ] : -1;
      batching &= sensor_ctl->current_batching[ sam_sensor->handles[ i ] ];
      HAL_LOG_VERBOSE( "%s: Inspecting Sensor %i(%i); sample rate: %i; report rate: %i, WUFF: %i",
                       __FUNCTION__, sam_sensor->handles[ i ], i, sample_rate[ i ], report_rate[ i ], wuff[ i ] );
    }
    else
    {
      sample_rate[ i ] = 0;
      report_rate[ i ] = 0;
      wuff[ i ] = false;
      wuff_handle[ i ] = -1;
      batching &= true;
    }
  }

  handle_fastest = hal_sam_find_fastest( sensor_ctl, sam_sensor->handles,
                                         sensor_ctl->current_freq );

  // Disable algo if there are no more clients
  if( -1 == handle_fastest )
  {
    HAL_LOG_DEBUG( "%s: Disabling algo due to no clients (%i)",
                     __FUNCTION__, sam_sensor->curr_report_rate );

    hal_sam_send_cancel( sensor_ctl->hndl, sam_sensor->svc_num );
    sam_sensor->curr_report_rate = 0;
  }
  // Update algo freq if faster client is active
  else if( sam_sensor->curr_report_rate != sensor_ctl->current_freq[ handle_fastest ] )
  {
    HAL_LOG_DEBUG( "%s: Update freq %i -> %i",
                     __FUNCTION__, sam_sensor->curr_report_rate,
                     sensor_ctl->current_freq[ handle_fastest ] );

    fastest_freq = sensor_ctl->current_freq[ handle_fastest ];

    // Always cancel first, just in case
    hal_sam_send_cancel( sensor_ctl->hndl, sam_sensor->svc_num );

    if( 0 != sam_sensor->enable_func( sensor_ctl, fastest_freq ) )
    {
      HAL_LOG_ERROR( "%s: Unable to enable %s", __FUNCTION__, sam_sensor->algo_name );
      return -1;
    }
    sam_sensor->curr_report_rate = fastest_freq;
  }

  // Update batching parameters (if any sensor is still active)
  if( 0 != sam_sensor->curr_report_rate )
  {
    sam_sensor->get_report_func( sensor_ctl );

    handle_fastest = hal_sam_find_fastest( sensor_ctl, wuff_handle,
                                           sensor_ctl->current_rpt_rate );
    if( -1 == handle_fastest )
    {
      handle_fastest = hal_sam_find_fastest( sensor_ctl, sam_sensor->handles,
                                             sensor_ctl->current_rpt_rate );
    }

    sam_sensor->batch_func( sensor_ctl, batching,
        sensor_ctl->current_rpt_rate[ handle_fastest ],
        sensor_ctl->current_WuFF[ handle_fastest ] );

    if( batching )
    {
      handle_fastest = hal_sam_find_fastest( sensor_ctl, sam_sensor->handles,
                                             sensor_ctl->current_rpt_rate );
      sam_sensor->batch_update_func( sensor_ctl,
          sensor_ctl->current_rpt_rate[ handle_fastest ] );
    }
  }

  return 0;
}

/*===========================================================================
  FUNCTION:  hal_sam_lookup_svc
===========================================================================*/
/**
 * Lookup SAM processing information based on service ID
 *
 * @param[i] sensor_ctl Global HAL state
 * @param[i] svc_num Sensor1 service number
 *
 * @return sam_sensor if found, NULL otherwise
 */
static hal_sam_sensor_t*
hal_sam_lookup_svc( hal_sensor_control_t *sensor_ctl, int svc_num )
{
  if( -1 != sam_sensors[ svc_num ].svc_num )
  {
    return &sam_sensors[ svc_num ];
  }

  return NULL;
}

/*===========================================================================
  FUNCTION:  hal_sam_lookup_handle
===========================================================================*/
/**
 * Lookup SAM processing information based on the Android Sensor handle
 *
 * @param[i] sensor_ctl Global HAL state
 * @param[i] handle Android Sensor handle
 *
 * @return sam_sensor if found, NULL otherwise
 */
static hal_sam_sensor_t*
hal_sam_lookup_handle( hal_sensor_control_t* sensor_ctl, int handle )
{
  int i, j;

  for( i = 0; i < MAX_SAM_SERVICES; i++ )
  {
    if( -1 != sam_sensors[ i ].svc_num )
    {
      for( j = 0; j < MAX_SAM_SHARED_ALGO; j++ )
      {
        if( -1 == sam_sensors[ i ].handles[ j ] )
        {
          break;
        }
        else if( sam_sensors[ i ].handles[ j ] == handle )
        {
          return &sam_sensors[ i ];
        }
      }
    }
  }

  return NULL;
}

/*===========================================================================
  FUNCTION:  quat_to_rot_mat
  Convert quaternion to rotation matrix

    Quaternion
                Q = |X Y Z W|

    Rotation Matrix
                /  R[ 0]   R[ 1]   R[ 2]   0  \
                |  R[ 4]   R[ 5]   R[ 6]   0  |
                |  R[ 8]   R[ 9]   R[10]   0  |
                \  0       0       0       1  /

   M = 1- 2(Y*Y + Z*Z)  2XY - 2ZW       2XZ + 2YW       0
       2XY + 2ZW        1 - 2(XX + ZZ)  2YZ - 2XW       0
       2XZ - 2YW        2YZ + 2XW       1 - 2(XX + ZZ)  0
       0                0               0               1
===========================================================================*/
static void quat_to_rot_mat(float rot_mat[9], float quat[4])
{
  float X = quat[0];
  float Y = quat[1];
  float Z = quat[2];
  float W = quat[3];

  float xx      = X * X;
  float xy      = X * Y;
  float xz      = X * Z;
  float xw      = X * W;
  float yy      = Y * Y;
  float yz      = Y * Z;
  float yw      = Y * W;
  float zz      = Z * Z;
  float zw      = Z * W;

  //HAL_LOG_ERROR("%s: X=%f, Y=%f, Z=%f, W=%f", __FUNCTION__, X, Y, Z, W);

  rot_mat[0]  = 1 - 2 * ( yy + zz );
  rot_mat[1]  =     2 * ( xy - zw );
  rot_mat[2]  =     2 * ( xz + yw );
  rot_mat[3]  =     2 * ( xy + zw );
  rot_mat[4]  = 1 - 2 * ( xx + zz );
  rot_mat[5]  =     2 * ( yz - xw );
  rot_mat[6]  =     2 * ( xz - yw );
  rot_mat[7]  =     2 * ( yz + xw );
  rot_mat[8]  = 1 - 2 * ( xx + yy );
}

/*===========================================================================

  FUNCTION:  rot_mat_to_orient
  Convert rotation matrix to Orientation Sensor as defined in Sensor.TYPE_ORIENTATION:

    values[0]: Azimuth, angle between the magnetic north direction and the y-axis,
    around the z-axis (0 to 359). 0=North, 90=East, 180=South, 270=West

    values[1]: Pitch, rotation around x-axis (-180 to 180),
    with positive values when the z-axis moves toward the y-axis.

    values[2]: Roll, rotation around y-axis (-90 to 90),
    with positive values when the x-axis moves toward the z-axis.

===========================================================================*/
static void rot_mat_to_orient(float values[3], float rot_mat[9])
{
  float xunit[3] = {rot_mat[0], rot_mat[3], rot_mat[6]};
  float yunit[3] = {rot_mat[1], rot_mat[4], rot_mat[7]};
  float zunit[3] = {rot_mat[2], rot_mat[5], rot_mat[8]};

  float xnorm = sqrt(xunit[0]*xunit[0] + xunit[1]*xunit[1]);

  if( fabs(zunit[2]) < MIN_FLT_TO_AVOID_SINGULARITY)
  {
    zunit[2] = MIN_FLT_TO_AVOID_SINGULARITY * (zunit[2] < 0 ? -1 : 1);
  }

  if( fabs(xunit[0]) < MIN_FLT_TO_AVOID_SINGULARITY)
  {
    xunit[0] = MIN_FLT_TO_AVOID_SINGULARITY * (xunit[0] < 0 ? -1 : 1);
  }

  if( fabs(xnorm) < MIN_FLT_TO_AVOID_SINGULARITY)
  {
    xnorm = MIN_FLT_TO_AVOID_SINGULARITY * (xnorm < 0 ? -1 : 1);
  }

  values[0] = RAD2DEG * atan2(xunit[1], xunit[0]);
  values[0] = fmodf(360.0f - values[0], 360.0f);
  values[1] = -RAD2DEG * atan2(yunit[2], zunit[2]);
  values[2] = RAD2DEG * atan2(xunit[2], xnorm);
}

/*===========================================================================
  FUNCTION:  hal_sam_gen_rotv
===========================================================================*/
/**
 * Generate Rotation Vector sensor sample for Android
 *
 * @param[i] sensor_ctl Global HAL state
 * @param[i] sam_sample Parsed SAM sample
 * @param[o] la_sample Android Sensor handle
 *
 * @return 0 upon success, < 0 otherwise
 */
static int
hal_sam_gen_rotv( hal_sensor_control_t *sensor_ctl, hal_sam_sample_t *sam_sample,
                  sensors_event_t *la_sample )
{
  la_sample->type = SENSOR_TYPE_ROTATION_VECTOR;
  la_sample->sensor = HANDLE_ROTATION_VECTOR;

  la_sample->data[0] = sam_sample->data[0];
  la_sample->data[1] = sam_sample->data[1];
  la_sample->data[2] = sam_sample->data[2];
  la_sample->data[3] = sam_sample->data[3];
  la_sample->data[4] =
      ( sam_sample->accuracy > 0 && sam_sample->accuracy < 4 )
        ? SNS_ROT_VEC_HEAD_ERR_EST_RAD / sam_sample->accuracy
        : M_PI;

  HAL_LOG_VERBOSE( "%s: sensor %d [0]: %f [1]: %f [2]: %f [3]: %f [4]: %f acc: %d ts: %u",
                    __FUNCTION__, la_sample->type, la_sample->data[0], la_sample->data[1],
                    la_sample->data[2], la_sample->data[3], la_sample->data[4],
                    sam_sample->accuracy, sam_sample->timestamp );

  return 0;
}

/*===========================================================================
  FUNCTION:  hal_sam_gen_ori
===========================================================================*/
/**
 * Generate Orientation sensor sample for Android
 *
 * @param[i] sensor_ctl Global HAL state
 * @param[i] sam_sample Parsed SAM sample
 * @param[o] la_sample Android Sensor handle
 *
 * @return 0 upon success, < 0 otherwise
 */
static int
hal_sam_gen_ori( hal_sensor_control_t *sensor_ctl, hal_sam_sample_t *sam_sample,
                 sensors_event_t *la_sample )
{
  float quat[4];
  float rot_mat[9];
  float orient[3];

  la_sample->type = SENSOR_TYPE_ORIENTATION;
  la_sample->sensor = HANDLE_ORIENTATION;

  quat[0] = sam_sample->data[0];
  quat[1] = sam_sample->data[1];
  quat[2] = sam_sample->data[2];
  quat[3] = sam_sample->data[3];

  // convert rotation vector to rotation matrix
  quat_to_rot_mat( rot_mat, quat );
  // convert rotation matrix to orientation
  rot_mat_to_orient( orient, rot_mat );

  la_sample->orientation.x = orient[0];
  la_sample->orientation.y = orient[1];
  la_sample->orientation.z = orient[2];

  la_sample->orientation.status = sam_sample->accuracy;

  HAL_LOG_VERBOSE( "%s: sensor: %d [0]: %f [1]: %f [2]: %f acc: %d ts: %u",
                    __FUNCTION__, la_sample->type, la_sample->data[0], la_sample->data[1],
                      la_sample->data[2], sam_sample->accuracy, sam_sample->timestamp );

  return 0;
}

/*===========================================================================
  FUNCTION:  hal_sam_gen_georv
===========================================================================*/
/**
 * Generate Geomagnetic Rotation Vector sensor sample for Android
 *
 * @param[i] sensor_ctl Global HAL state
 * @param[i] sam_sample Parsed SAM sample
 * @param[o] la_sample Android Sensor handle
 *
 * @return 0 upon success, < 0 otherwise
 */
static int
hal_sam_gen_georv( hal_sensor_control_t *sensor_ctl, hal_sam_sample_t *sam_sample,
                   sensors_event_t *la_sample )
{
  la_sample->type = SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR;
  la_sample->sensor = HANDLE_GEOMAGNETIC_ROTATION_VECTOR;

  la_sample->data[0] = sam_sample->data[0];
  la_sample->data[1] = sam_sample->data[1];
  la_sample->data[2] = sam_sample->data[2];
  la_sample->data[3] = sam_sample->data[3];
  la_sample->data[4] =
      ( sam_sample->accuracy > 0 && sam_sample->accuracy < 4 )
        ? SNS_ROT_VEC_HEAD_ERR_EST_RAD / sam_sample->accuracy
        : M_PI;

  HAL_LOG_VERBOSE( "%s: sensor %d: [0]: %f [1]: %f [2]: %f acc: %d ts: %u", __FUNCTION__,
                    la_sample->type, la_sample->data[0], la_sample->data[1],
                    la_sample->data[2], sam_sample->accuracy, sam_sample->timestamp );

  return 0;
}

/*===========================================================================
  FUNCTION:  hal_sam_gen_grav
===========================================================================*/
/**
 * Generate Gravity sensor sample for Android
 *
 * @param[i] sensor_ctl Global HAL state
 * @param[i] sam_sample Parsed SAM sample
 * @param[o] la_sample Android Sensor handle
 *
 * @return 0 upon success, < 0 otherwise
 */
static int
hal_sam_gen_grav( hal_sensor_control_t *sensor_ctl, hal_sam_sample_t *sam_sample,
                  sensors_event_t *la_sample )
{
  la_sample->type = SENSOR_TYPE_GRAVITY;
  la_sample->sensor = HANDLE_GRAVITY;

  la_sample->acceleration.x = sam_sample->data[8];
  la_sample->acceleration.y = sam_sample->data[7];
  la_sample->acceleration.z = -sam_sample->data[9];
  la_sample->acceleration.status = sam_sample->accuracy;

  HAL_LOG_VERBOSE( "%s: GravityX: %f GravityY: %f GravityZ: %f acc: %d ts: %u",
                    __FUNCTION__, la_sample->data[0], la_sample->data[1], la_sample->data[2],
                    sam_sample->accuracy, sam_sample->timestamp );

  return 0;
}

/*===========================================================================
  FUNCTION:  hal_sam_gen_laccel
===========================================================================*/
/**
 * Generate Linear Accel sensor sample for Android
 *
 * @param[i] sensor_ctl Global HAL state
 * @param[i] sam_sample Parsed SAM sample
 * @param[o] la_sample Android Sensor handle
 *
 * @return 0 upon success, < 0 otherwise
 */
static int
hal_sam_gen_laccel( hal_sensor_control_t *sensor_ctl, hal_sam_sample_t *sam_sample,
                    sensors_event_t *la_sample )
{
  la_sample->type = SENSOR_TYPE_LINEAR_ACCELERATION;
  la_sample->sensor = HANDLE_LINEAR_ACCEL;

  la_sample->acceleration.x = sam_sample->data[5];
  la_sample->acceleration.y = sam_sample->data[4];
  la_sample->acceleration.z = -sam_sample->data[6];
  la_sample->acceleration.status = sam_sample->accuracy;

  HAL_LOG_VERBOSE( "%s: LinearAccelX: %f LinearAccelY: %f LinearAccelZ: %f acc: %d ts: %u",
                   __FUNCTION__, la_sample->data[0], la_sample->data[1], la_sample->data[2],
                   sam_sample->accuracy, sam_sample->timestamp );

  return 0;
}

/*===========================================================================
  FUNCTION:  hal_sam_gen_gamerv
===========================================================================*/
/**
 * Generate Game Rotation Vector sensor sample for Android
 *
 * @param[i] sensor_ctl Global HAL state
 * @param[i] sam_sample Parsed SAM sample
 * @param[o] la_sample Android Sensor handle
 *
 * @return 0 upon success, < 0 otherwise
 */
static int
hal_sam_gen_gamerv( hal_sensor_control_t *sensor_ctl, hal_sam_sample_t *sam_sample,
                    sensors_event_t *la_sample )
{
  la_sample->type = SENSOR_TYPE_GAME_ROTATION_VECTOR;
  la_sample->sensor = HANDLE_GAME_ROTATION_VECTOR;

  la_sample->data[0] = sam_sample->data[0];
  la_sample->data[1] = sam_sample->data[1];
  la_sample->data[2] = sam_sample->data[2];
  la_sample->data[3] = sam_sample->data[3];
  la_sample->data[4] = 0;

  HAL_LOG_VERBOSE( "%s: sensor %d: [0]: %f [1]: %f [2]: %f acc: %d ts: %u", __FUNCTION__,
                   la_sample->type, la_sample->data[0], la_sample->data[1],
                   la_sample->data[2], sam_sample->accuracy, sam_sample->timestamp );

  return 0;
}

/*===========================================================================
  FUNCTION:  hal_sam_gen_ped
===========================================================================*/
/**
 * Generate Pedometer sensor sample for Android
 *
 * @param[i] sensor_ctl Global HAL state
 * @param[i] sam_sample Parsed SAM sample
 * @param[o] la_sample Android Sensor handle
 *
 * @return 0 upon success, < 0 otherwise
 */
static int
hal_sam_gen_ped( hal_sensor_control_t *sensor_ctl, hal_sam_sample_t *sam_sample,
                 sensors_event_t *la_sample )
{
  la_sample->type = SENSOR_TYPE_PEDOMETER;
  la_sample->sensor = HANDLE_PEDOMETER;

  la_sample->data[0] = sam_sample->data[0];
  la_sample->data[1] = sam_sample->data[1];
  la_sample->data[2] = sam_sample->data[2];

  HAL_LOG_VERBOSE( "%s: sensor %d, step count %f", __FUNCTION__,
                   la_sample->type, la_sample->data[0] );

  return 0;
}

/*===========================================================================
  FUNCTION:  hal_sam_gen_stepc
===========================================================================*/
/**
 * Generate Step Counter sensor sample for Android
 *
 * @param[i] sensor_ctl Global HAL state
 * @param[i] sam_sample Parsed SAM sample
 * @param[o] la_sample Android Sensor handle
 *
 * @return 0 upon success, < 0 otherwise
 */
static int
hal_sam_gen_stepc( hal_sensor_control_t *sensor_ctl, hal_sam_sample_t *sam_sample,
                  sensors_event_t *la_sample )
{
  /* As we update all sensors associated with an algo when SAM sends a response,
   * step counter needs a special handling to avoid spurious events generated
   * from step detector or pedometer when either of them are registered/de-registered.
   */
  if( ( sensor_ctl->step_counter_running_total == 0 && sensor_ctl->stepc_activated == false ) ||
        sensor_ctl->step_counter_current_instance != sam_sample->data[0] )
  {
    sensor_ctl->stepc_activated = true;
    sensor_ctl->step_counter_current_instance = sam_sample->data[0];
    la_sample->type = SENSOR_TYPE_STEP_COUNTER;
    la_sample->sensor = HANDLE_SAM_STEP_COUNTER;

    uint64_t steps = ( sensor_ctl->step_counter_running_total + sam_sample->data[0] );
#ifdef SENSORS_DEVICE_API_VERSION_1_1
    la_sample->u64.step_counter = steps;
#else
    la_sample->step_counter = steps;
#endif /* #else SENSORS_DEVICE_API_VERSION_1_1 */
    sensor_ctl->step_counter_running_instance = steps;
    sensor_ctl->stepc_last_ts = la_sample->timestamp;

    HAL_LOG_VERBOSE( "%s: sensor %d, step_counter=%" PRIu64 "step_counter_running_total=%" PRIu64,
                     __FUNCTION__, la_sample->type, steps,
                     sensor_ctl->step_counter_running_total );
  }

  return 0;
}

/*===========================================================================
  FUNCTION:  hal_sam_gen_stepd
===========================================================================*/
/**
 * Generate Step Detector sensor sample for Android
 *
 * @param[i] sensor_ctl Global HAL state
 * @param[i] sam_sample Parsed SAM sample
 * @param[o] la_sample Android Sensor handle
 *
 * @return 0 upon success, < 0 otherwise
 */
static int
hal_sam_gen_stepd( hal_sensor_control_t *sensor_ctl, hal_sam_sample_t *sam_sample,
                  sensors_event_t *la_sample )
{
  if( sam_sample->data[3] )  // If report.step_event is true
  {
    la_sample->type = SENSOR_TYPE_STEP_DETECTOR;
    la_sample->sensor = HANDLE_SAM_STEP_DETECTOR;

    la_sample->data[0] = sam_sample->data[3];

    HAL_LOG_VERBOSE( "%s: sensor %d, step count %f", __FUNCTION__,
                     la_sample->type, la_sample->data[0] );
  }
  else
  {
    HAL_LOG_DEBUG( "%s: No step event", __FUNCTION__ );
  }

  return 0;
}

/*===========================================================================
  FUNCTION:  hal_sam_gen_prox
===========================================================================*/
/**
 * Generate Proximity sensor sample for Android
 *
 * @param[i] sensor_ctl Global HAL state
 * @param[i] sam_sample Parsed SAM sample
 * @param[o] la_sample Android Sensor handle
 *
 * @return 0 upon success, < 0 otherwise
 */
static int
hal_sam_gen_prox( hal_sensor_control_t *sensor_ctl, hal_sam_sample_t *sam_sample,
                  sensors_event_t *la_sample )
{
  la_sample->type = SENSOR_TYPE_PROXIMITY;
  la_sample->sensor = HANDLE_PROXIMITY;

#ifndef HAL_SUPPORT_DISTANCE
  if( 0 == sam_sample->data[0] * UNIT_CONVERT_Q16 )  /* far */
  {
    la_sample->distance = sensor_ctl->sensor_list[ HANDLE_PROXIMITY ].max_range;
  }
  else  /* near */
  {
    la_sample->distance = 0;
  }
#else
  la_sample->distance =
    (float)(sam_sample->data[1]) * UNIT_CONVERT_PROXIMITY;
#endif
  HAL_LOG_VERBOSE( "%s: prox data: %f %f %f", __FUNCTION__,
                   sam_sample->data[0], sam_sample->data[1],
                   la_sample->distance );
  return 0;
}

/**
 * Mapping of sensor handles to the functions that process incoming SAM
 * indications for that sensor.
 */
static hal_sam_gen_sample gen_sample_funcs[ MAX_NUM_SENSORS ] =
  { [HANDLE_ROTATION_VECTOR] = hal_sam_gen_rotv,
    [HANDLE_ORIENTATION] = hal_sam_gen_ori,
    [HANDLE_GEOMAGNETIC_ROTATION_VECTOR] = hal_sam_gen_georv,
    [HANDLE_GRAVITY] = hal_sam_gen_grav,
    [HANDLE_LINEAR_ACCEL] = hal_sam_gen_laccel,
    [HANDLE_GAME_ROTATION_VECTOR] = hal_sam_gen_gamerv,
    [HANDLE_PEDOMETER] = hal_sam_gen_ped,
    [HANDLE_SAM_STEP_COUNTER] = hal_sam_gen_stepc,
    [HANDLE_SAM_STEP_DETECTOR] = hal_sam_gen_stepd,
    [HANDLE_PROXIMITY] = hal_sam_gen_prox
  };

/*==========================================================================
                         PUBLIC FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION:  hal_sam_activate
===========================================================================*/
int
hal_sam_activate( hal_sensor_control_t* sensor_ctl, int handle )
{
  hal_sam_sensor_t *sam_sensor;
  int rv = -1;

  HAL_LOG_DEBUG( "%s: Activating sensor handle %i", __FUNCTION__, handle );
  sam_sensor = hal_sam_lookup_handle( sensor_ctl, handle );

  if( NULL == sam_sensor )
  {
    HAL_LOG_ERROR( "%s: Unable to identify algorithm for handle %i", __FUNCTION__, handle );
  }
  else
  {
    rv = hal_sam_algo_activate( sensor_ctl, sam_sensor );
  }

  return rv;
}

/*===========================================================================
  FUNCTION:  hal_sam_parse_ind
===========================================================================*/
int
hal_sam_parse_ind( hal_sensor_control_t *sensor_ctl, sensor1_msg_header_s *msg_hdr, void *msg_ptr )
{
  hal_sam_sample_t *sample_list = NULL,
                   *curr_sample;
  hal_sam_sensor_t *sam_sensor;
  int rv = -1,
      i, j,
      count = 0,
      err;

  sam_sensor = hal_sam_lookup_svc( sensor_ctl, msg_hdr->service_number );

  if( NULL == sam_sensor )
  {
    HAL_LOG_WARN( "%s: Cannot find handler for SAM service %i",
                  __FUNCTION__, msg_hdr->service_number );
  }
  else
  {
    sample_list = sam_sensor->parse_ind_func( msg_hdr, msg_ptr, &count );
    HAL_LOG_VERBOSE("%s: Received %i samples", __FUNCTION__, count );

    for( j = 0; j < count; j++ )
    {
      curr_sample = &sample_list[ j ];
      for( i = 0; i < MAX_SAM_SHARED_ALGO; i++ )
      {
        sensors_event_t la_sample;
        // To flag unknown handle-types
        la_sample.type = -1;

        if( -1 == sam_sensor->handles[ i ] )
        {
          continue;
        }
        else if( NULL == gen_sample_funcs[ sam_sensor->handles[ i ] ] )
        {
          HAL_LOG_WARN( "%s: No processing function for handle %i",
                        __FUNCTION__, sam_sensor->handles[ i ] );
        }
        else if( sensor_ctl->active_sensors & (1ULL << sam_sensor->handles[ i ]) )
        {
          gen_sample_funcs[ sam_sensor->handles[ i ] ]
            ( sensor_ctl, curr_sample, &la_sample );
        }

        if( -1 != la_sample.type ) /* No error */
        {
          la_sample.version = sizeof(sensors_event_t);

          la_sample.timestamp = hal_timestamp_calc(
              (uint64_t)curr_sample->timestamp, la_sample.sensor );
          if( hal_insert_queue( &la_sample ) )
          {
            hal_signal_ind( &sensor_ctl->data_arrived_cond );
          }
        }
      }
    }
    rv = 0;
    free( sample_list );
  }

  return rv;
}

/*===========================================================================
  FUNCTION:  hal_sam_init
===========================================================================*/
int
hal_sam_init( hal_sensor_control_t *sensor_ctl )
{
  int i, j;

  for( i = 0; i < MAX_SAM_SERVICES; i++ )
  {
    hal_sam_sensor_t *sam_sensor = &sam_sensors[ i ];
    for( j = 0; j < MAX_SAM_SHARED_ALGO; j++ )
    {
      sam_sensor->handles[ j ] = -1;
    }

    if( SNS_SAM_ROTATION_VECTOR_SVC_ID_V01 == i )
    {
      hal_sam_rotv_info( sam_sensor );
      if( hal_is_gyro_available() )
      {
        sam_sensor->handles[0] = HANDLE_ROTATION_VECTOR;
        sam_sensor->handles[1] = HANDLE_ORIENTATION;
      }
    }
    else if( SNS_SAM_PED_SVC_ID_V01 == i )
    {
      hal_sam_ped_info( sam_sensor );
      sam_sensor->handles[0] = HANDLE_PEDOMETER;
      sam_sensor->handles[1] = HANDLE_SAM_STEP_DETECTOR;
      sam_sensor->handles[2] = HANDLE_SAM_STEP_COUNTER;
    }
    else if( SNS_SAM_GAME_ROTATION_VECTOR_SVC_ID_V01 == i )
    {
      hal_sam_gamerv_info( sam_sensor );
      sam_sensor->handles[0] = HANDLE_GAME_ROTATION_VECTOR;
    }
    else if( SNS_SAM_GRAVITY_VECTOR_SVC_ID_V01 == i )
    {
      hal_sam_grav_info( sam_sensor );
      if( hal_is_gyro_available() )
      {
        sam_sensor->handles[0] = HANDLE_GRAVITY;
        sam_sensor->handles[1] = HANDLE_LINEAR_ACCEL;
      }
    }
    else if( SNS_SAM_ORIENTATION_SVC_ID_V01 == i )
    {
      hal_sam_ori_info( sam_sensor );
      sam_sensor->handles[0] = HANDLE_GEOMAGNETIC_ROTATION_VECTOR;
      if( !hal_is_gyro_available() )
      {
        sam_sensor->handles[1] = HANDLE_ORIENTATION;
        sam_sensor->handles[2] = HANDLE_ROTATION_VECTOR;
        sam_sensor->handles[3] = HANDLE_GRAVITY;
        sam_sensor->handles[4] = HANDLE_LINEAR_ACCEL;
      }
    }
    else if( SNS_SAM_SENSOR_THRESH_SVC_ID_V01 == i )
    {
      hal_sam_thresh_info( sam_sensor );
      sam_sensor->handles[0] = HANDLE_PROXIMITY;
    }
    else
    {
      sam_sensor->svc_num = -1;
    }

    if( NULL != sam_sensor->init_func )
    {
      sam_sensor->init_func( sam_sensor );
    }
  }

  return 0;
}

/*===========================================================================
  FUNCTION:  hal_sam_send_cancel
===========================================================================*/
void
hal_sam_send_cancel( sensor1_handle_s *sensor1_hndl, int svc_num )
{
  sensor1_error_e error;
  sensor1_msg_header_s req_hdr;
  sns_common_cancel_req_msg_v01 *cancel_msg;

  HAL_LOG_DEBUG( "%s: Sending cancel to %d", __FUNCTION__, svc_num );

  error = sensor1_alloc_msg_buf( sensor1_hndl,
                                 sizeof(sns_common_cancel_req_msg_v01),
                                 (void**)&cancel_msg );

  if( SENSOR1_SUCCESS != error )
  {
    HAL_LOG_ERROR( "%s: sensor1_alloc_msg_buf() error: %d", __FUNCTION__, error );
  }
  else
  {
    req_hdr.service_number = svc_num;
    req_hdr.msg_id = 0; // Message ID for Cancel Request is 0 for all services
    req_hdr.msg_size = sizeof( sns_common_cancel_req_msg_v01 );
    req_hdr.txn_id = TXN_ID_NO_RESP_SIGNALLED;

    if( SENSOR1_SUCCESS !=
        (error = sensor1_write( sensor1_hndl, &req_hdr, cancel_msg )) )
    {
      sensor1_free_msg_buf( sensor1_hndl, cancel_msg );
      HAL_LOG_ERROR( "%s: sensor1_write() error: %d", __FUNCTION__, error );
    }
  }
}
