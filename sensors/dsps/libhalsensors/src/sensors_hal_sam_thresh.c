/*============================================================================
  @file sensors_hal_sam_thresh.c

  @brief
  File handles all requests to the SAM Threshold Algorithm.  Presently, this
  implementation only supports the proximity sensor.

  Copyright (c) 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
============================================================================*/

/*===========================================================================
                           INCLUDE FILES
===========================================================================*/
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include "sensors_hal.h"
#include "sns_sam_sensor_thresh_v01.h"
#include "sensor1.h"
#include "fixed_point.h"

/*===========================================================================
                           PREPROCESSOR DEFINITIONS
===========================================================================*/

#define SVC_NUM SNS_SAM_SENSOR_THRESH_SVC_ID_V01

/*===========================================================================
                         STATIC VARIABLES
===========================================================================*/

/*==========================================================================
                         STATIC FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION:  hal_sam_thresh_send_enable
===========================================================================*/
static int
hal_sam_thresh_send_enable( hal_sensor_control_t* sensor_ctl, uint32_t report_rate )
{
  sensor1_error_e error;
  sensor1_msg_header_s req_hdr;
  sns_sam_sensor_thresh_enable_req_msg_v01 *sam_req;

  error = sensor1_alloc_msg_buf( sensor_ctl->hndl,
                                 sizeof(sns_sam_sensor_thresh_enable_req_msg_v01),
                                 (void**)&sam_req );
  if( SENSOR1_SUCCESS != error )
  {
    HAL_LOG_ERROR( "%s: sensor1_alloc_msg_buf() error: %d", __FUNCTION__, error );
    return -1;
  }

  req_hdr.service_number = SVC_NUM;
  req_hdr.msg_id = SNS_SAM_SENSOR_THRESH_ENABLE_REQ_V01;
  req_hdr.msg_size = sizeof( sns_sam_sensor_thresh_enable_req_msg_v01 );
  req_hdr.txn_id = 0;

  /* Report Request */
  sam_req->sensor_id = SNS_SMGR_ID_PROX_LIGHT_V01;
  sam_req->data_type = SNS_SMGR_DATA_TYPE_PRIMARY_V01;
  sam_req->sample_rate = (5 << 16); /* ignore report_rate, use 5Hz */
  sam_req->threshold[0] = 0xFFFF;   /* threshold for binary states in Q16 */
  sam_req->threshold[1] = sam_req->threshold[2] = 0;  /* for completeness */

  /* allow sensor threshold indications to wake up apps processor */
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
    HAL_LOG_ERROR( "%s: sensor1_write() error: %d", __FUNCTION__, error );
    return -1;
  }

  /* waiting for response */
  if (hal_wait_for_response( TIME_OUT_MS,
                             &sensor_ctl->cb_mutex,
                             &sensor_ctl->cb_arrived_cond,
                             &sensor_ctl->is_resp_arrived ) == false )
  {
    HAL_LOG_ERROR( "%s: ERROR: No response from SENSOR THRESH enable request",
                   __FUNCTION__ );
    return -1;
  }

  HAL_LOG_DEBUG( "%s: Received Response: %d", __FUNCTION__, sensor_ctl->error );

  return sensor_ctl->error ? -1 : 0;
}

/*===========================================================================
  FUNCTION:  hal_sam_ped_send_batch
===========================================================================*/
static int
hal_sam_thresh_send_batch( hal_sensor_control_t* sensor_ctl,
    bool batching, uint32_t batch_rate, bool wake_upon_fifo_full )
{
  /* Threshold service does not support batching, ignore request */
  return 0;
}

/*===========================================================================
  FUNCTION:  hal_sam_ped_send_batch_update
===========================================================================*/
int
hal_sam_thresh_send_batch_update( hal_sensor_control_t* sensor_ctl, uint32_t batch_rate )
{
  /* Threshold service does not support batching, ignore request */
  return 0;
}

/*===========================================================================
  FUNCTION:  hal_sam_thresh_get_report
===========================================================================*/
static int
hal_sam_thresh_get_report( hal_sensor_control_t* sensor_ctl )
{
  return -1;
}

/*==========================================================================
                         PUBLIC FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION:  hal_sam_thresh_parse_ind
===========================================================================*/
hal_sam_sample_t*
hal_sam_thresh_parse_ind( sensor1_msg_header_s *msg_hdr, void *msg_ptr, int *count )
{
  hal_sam_sample_t *sample_list = NULL;
  uint32_t i = 0;
  *count = 0;

  if( SNS_SAM_SENSOR_THRESH_REPORT_IND_V01 == msg_hdr->msg_id )
  {
    sns_sam_sensor_thresh_report_ind_msg_v01* sam_ind =
      (sns_sam_sensor_thresh_report_ind_msg_v01*)msg_ptr;

    sample_list = malloc( sizeof(hal_sam_sample_t) );
    if( NULL == sample_list )
    {
      HAL_LOG_ERROR( "%s: Malloc error", __FUNCTION__ );
    }
    else
    {
      *count = 1;

      sample_list->data[0] = sam_ind->sample_value[0];
      sample_list->data[1] = sam_ind->sample_value[1];
      sample_list->data[2] = sam_ind->sample_value[2];

      sample_list->timestamp = sam_ind->timestamp;
    }
  }
  else
  {
    HAL_LOG_ERROR( "%s: Unknown message ID = %d", __FUNCTION__, msg_hdr->msg_id );
  }

  return sample_list;
}

/*===========================================================================
  FUNCTION:  hal_sam_thresh_init
===========================================================================*/
void hal_sam_thresh_init( hal_sam_sensor_t *sam_sensor )
{
  sam_sensor->curr_report_rate = 0;
}

/*===========================================================================
  FUNCTION:  hal_sam_thresh_info
===========================================================================*/
int
hal_sam_thresh_info( hal_sam_sensor_t *sam_sensor )
{
  sam_sensor->enable_func = &hal_sam_thresh_send_enable;
  sam_sensor->get_report_func = &hal_sam_thresh_get_report;
  sam_sensor->batch_func = &hal_sam_thresh_send_batch;
  sam_sensor->batch_update_func = &hal_sam_thresh_send_batch_update;
  sam_sensor->parse_ind_func = &hal_sam_thresh_parse_ind;
  sam_sensor->init_func = &hal_sam_thresh_init;
  sam_sensor->svc_num = SVC_NUM;
  strlcpy( sam_sensor->algo_name, "thresh", HAL_SAM_NAME_MAX_LEN - 1 );

  return 0;
}
