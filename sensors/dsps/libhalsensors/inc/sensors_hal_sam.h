#ifndef SENSORS_HAL_SAM_H
#define SENSORS_HAL_SAM_H

/*============================================================================
  @file sensors_hal_sam.h

  @brief
  Function declarations and data structures for SAM HAL functionality.

  Copyright (c) 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
============================================================================*/

/*===========================================================================
                           INCLUDE FILES
===========================================================================*/

#include <cutils/log.h>
#include <sensors.h>
#include <stdbool.h>
#include <math.h>
#include "log.h"
#include "sensor1.h"

/*===========================================================================
                   PREPROCESSOR DEFINTIONS
===========================================================================*/

/* Maximum number of android Sensors sharing one SAM algo.
 * When changing this value, other arrays throughout the code must also change. */
#define MAX_SAM_SHARED_ALGO 5

/* max number of SAM service ID's */
#define MAX_SAM_SERVICES 60

/* Maximum data fields per SAM algorithm report */
#define SAM_MAX_DATA_LENGTH 16

/* Maximum length to represent an algorithm name */
#define HAL_SAM_NAME_MAX_LEN 10

/*===========================================================================
                   DATA TYPES
===========================================================================*/

struct hal_sensor_control_t;
struct hal_sam_sensor;

typedef struct hal_sam_sample {
  float data[SAM_MAX_DATA_LENGTH];
  uint32_t timestamp;
  uint8_t accuracy;
} hal_sam_sample_t;

/**
 * Parse indication messages received from the applicable SAM service.
 *
 * @param[i] msg_hdr Sensor1 message header
 * @param[i] msg_ptr Pointer to the SAM report or batch indication
 * @param[o] count Number of samples returned
 *
 * @return Pointer to array of samples of size count
 */
typedef hal_sam_sample_t* (*hal_sam_parse_ind_func)
  ( sensor1_msg_header_s *msg_hdr, void *msg_ptr, int *count );

/**
 * Reset the state associated with the SAM algorithm
 */
typedef void (*hal_sam_init_func) ( struct hal_sam_sensor *sam_sensor );

/**
 * Enable a SAM algorithm at the specified rate
 *
 * @param[i] sensor_ctl sensor control device
 * @param[i] report_rate
 *
 * @return 0 upon sucess, otherwise error
 */
typedef int (*hal_sam_enable_func) ( struct hal_sensor_control_t* sensor_ctl, uint32_t report_rate );

/**
 * Update an existing stream with the specified batching parameters.
 *
 * @param[i] sensor_ctl sensor control device
 * @param[i] batching Whether batching should be enabled
 * @param[i] batch_rate Batch rate to be used when AP is asleep
 * @param[i] wake_upon_fifo_full Whether to wake-up AP when buffer is full
 *
 * @return 0 upon sucess, otherwise error
 */
typedef int (*hal_sam_batch_func) ( struct hal_sensor_control_t* sensor_ctl, bool batching, uint32_t batch_rate, bool wake_upon_fifo_full );

/**
 * Send a batch update request message.
 *
 * @param[i] sensor_ctl sensor control device
 * @param[i] batch_rate Batch rate to be used when AP is awake.
 *
 * @return 0 upon sucess, otherwise error
 */
typedef int (*hal_sam_batch_update_func) ( struct hal_sensor_control_t* sensor_ctl, uint32_t batch_rate );

/**
 * Request a report from SAM.
 *
 * @param[i] sensor_ctl sensor control device
 *
 * @return 0 upon sucess, otherwise error
 */
typedef int (*hal_sam_get_report_func) ( struct hal_sensor_control_t* sensor_ctl );

typedef struct hal_sam_sensor {
  hal_sam_enable_func enable_func;
  hal_sam_batch_func batch_func;
  hal_sam_batch_update_func batch_update_func;
  hal_sam_parse_ind_func parse_ind_func;
  hal_sam_init_func init_func;
  hal_sam_get_report_func get_report_func;

  /** Present streaming rate requesting from SAM */
  uint32_t curr_report_rate;

  /** Android Sensor handles that this algorithm serves */
  int handles[ MAX_SAM_SHARED_ALGO ];

  /** SAM service number associated with this virtual sensor */
  int svc_num;

  /** Short name of the algorithm; for logging purposes */
  char algo_name[ HAL_SAM_NAME_MAX_LEN];
} hal_sam_sensor_t;

/*===========================================================================
                    FUNCTION DECLARATIONS
===========================================================================*/

/*===========================================================================
  FUNCTION:  hal_sam_activate
===========================================================================*/
/*!
  @brief
  Update the appropriate SAM algorithm according to the current sensors
  control struct state.  cb_mutex must be held.

  @param[i] sensor_ctl sensor control device
  @param[i] handle is the handle of the sensor to change.

  @return 0 on success, negative value otherwise
  */
int hal_sam_activate( struct hal_sensor_control_t* sensor_ctl, int handle );

/*===========================================================================
  FUNCTION:  hal_sam_parse_ind
===========================================================================*/
/**
 * Parse and handle an indication received from a SAM service.
 *
 * @param[i] sensor_ctl Global HAL state
 * @param[i] msg_hdr Sensor1 message header
 * @param[i] msg_ptr Sensor1 message pointer
 *
 * @return 0 on success, negative value otherwise
 */
int hal_sam_parse_ind( struct hal_sensor_control_t *sensor_ctl,
    sensor1_msg_header_s *msg_hdr, void *msg_ptr );

/*===========================================================================
  FUNCTION:  hal_sam_init
===========================================================================*/
/**
 * Reset and initializes the HAL SAM framework; resets SAM sensor state.
 * No messages are sent.
 *
 * @param[i] sensor_ctl Global HAL state
 *
 * @return 0 upon success, error otherwise
 */
int hal_sam_init( struct hal_sensor_control_t *sensor_ctl );

/*===========================================================================
  FUNCTION:  hal_sam_send_cancel
===========================================================================*/
/**
 * Send a Sensor1 cancel message to the specified service.  The active stream
 * should stop.
 *
 * @param[i] sensor1_hndl Sensor1 connection handle
 * @param[i] svc_num Sensor1 service number
 */
void hal_sam_send_cancel( sensor1_handle_s *sensor1_hndl, int svc_num );

/*===========================================================================
  FUNCTION:  hal_sam_*_info
===========================================================================*/
/**
 * Collect the function pointers associated with the various actions of
 * this algorithm.
 *
 * @param[o] lookup_info Struct to fill-out
 *
 * @return 0 upon success, otherwise error
 */
int hal_sam_rotv_info( hal_sam_sensor_t *lookup_info );
int hal_sam_gamerv_info( hal_sam_sensor_t *lookup_info );
int hal_sam_ped_info( hal_sam_sensor_t *lookup_info );
int hal_sam_ori_info( hal_sam_sensor_t *lookup_info );
int hal_sam_grav_info( hal_sam_sensor_t *lookup_info );
int hal_sam_thresh_info( hal_sam_sensor_t *lookup_info );

#endif /* SENSORS_HAL_SAM_H */
