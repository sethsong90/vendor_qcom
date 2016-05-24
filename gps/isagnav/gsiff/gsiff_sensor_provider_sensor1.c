/******************************************************************************
  @file:  gsiff_sensor_provider_sensor1.c
  @brief: GNSS / Sensor Interface Framework Support

  DESCRIPTION
    This file defines the implementation for GSIFF sensor provider interface
    using the Qualcomm proprietary DSPS Sensor1 API.

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
                    2. Fixed bug with out-of-order sensor samples when
                       more than 1 sample is received at a time. Samples
                       are sorted into monotonically increasing order based
                       on timestamp and then processed.
10/24/12   vr       1. Adding temperature streaming.
                    2. Adding motion data streming.
                    3. Adding pedometer streaming.
======================================================================*/

#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "gsiff_sensor_provider_sensor1.h"
#include "gsiff_sensor_provider_common.h"
#include "gsiff_msg.h"

#define LOG_TAG "gsiff_sp_sns1"

#include "log_util.h"

#include "sns_sam_qmd_v01.h"
#include "sns_sam_amd_v01.h"
#include "sns_sam_rmd_v01.h"
#include "sns_sam_vmd_v01.h"
#include "sns_sam_ped_v01.h"
#include "sns_smgr_api_v01.h"
#include "gpsone_glue_msg.h"
#include "sns_time_api_v02.h"
#include "sns_reg_api_v02.h"

#ifdef DEBUG_X86
/* Grab canned DSPS time tick from stubbed out sensor1 API */
extern uint32_t get_sensor_clk_ticks();
#endif

/* 32 KHz clock */
#define DSPS_CLK_RATE 32768
#define MOTION_DATA_TLV_AGE 100
#define MOTION_DATA_TLV_TIMEOUT 65535

/* timeout in ms used in mutex wait */
#define TIME_OUT_MS 1000

/**
 * Converts from DSPS clock ticks to milliseconds
 * clkval >> 15 gives the number of full seconds
 *
 * clkval & 0x7FFF gives clock ticks for fractional seconds
 */
#define DSPS_TICK_TO_MSEC(clkval) (((((uint64_t)(clkval)) >> 15) * 1000) + \
                                  (((((uint64_t)(clkval)) & 0x7FFF) * 1000) >> 15))

#define UNIT_CONVERT_Q16          (1.0 / 65536.0)
#define UNIT_CONVERT_ACCELERATION UNIT_CONVERT_Q16
#define UNIT_CONVERT_GYRO         UNIT_CONVERT_Q16
#define UNIT_CONVERT_PRESSURE     UNIT_CONVERT_Q16

/* Report Ids used by Sensor1. Easier to track if always consistent */
#define ACCEL_REPORTING_ID 1
#define ACCEL_TEMPERATURE_REPORTING_ID 2
#define GYRO_REPORTING_ID  3
#define GYRO_TEMPERATURE_REPORTING_ID 4
#define PRESSURE_REPORTING_ID 5

typedef struct sensor1_sensor_state_t {
   bool                          running;            /* Is reporting currently? */
   gsiff_msg                     data_msg;           /* Structure used to batch samples */
   uint32_t                      reporting_rate;     /* Current sampling rate in Hz */
   uint32_t                      batching_rate;      /* Current batching rate in Hz */
   float                         reporting_interval; /* Current sampling interval in ms */
   float                         batching_interval;  /* Current batching interval in ms */
} sensor1_sensor_state_t;

typedef struct sensor1_motion_state_t {
   bool                        running;                  /* Motion data updates coming from Sensor1? */
   uint8_t                     amd_algo_instance_id;     /* AMD algorithm instance id. Needed to disable motion data reporting */
   uint8_t                     rmd_algo_instance_id;     /* RMD algorithm instance id. Needed to disable motion data reporting */
   sns_sam_motion_state_e_v01  amd;                      /* AMD algorithm state */
   sns_sam_motion_state_e_v01  rmd;                      /* RMD algorithm state */
} sensor1_motion_state_t;

typedef struct sensor1_pedometer_state_t {
   bool                        running;                  /* Pedometer updates coming from Sensor1? */
   uint8_t                     algo_instance_id;         /* Pedometer algorithm instance id. Needed to disable motion data reporting */
   uint32_t                    step_count_threshold;     /* Step count threshold. */
   struct timeval              reset_time;               /* The latest reset time */
} sensor1_pedometer_state_t;

typedef struct sensor1_control_t {
    sensor1_handle_s*       sensor1_client_handle;    /* Opaque Sensor1 Client Handle */
    bool                    spi_running;              /* SPI updates coming from Sensor1? */
    int32_t                 spi_algo_instance_id;     /* SPI Algorithm instance id. Needed to disable SPI reporting */
    void*                   p_msg_q;                  /* Message Queue to add messages to. */
    sensor1_motion_state_t  motion_state;             /* All necessary for the motion state */
    sensor1_pedometer_state_t  pedometer_state;       /* All necessary for the pedometer */
    sensor1_sensor_state_t  gyro_state;               /* All necessary state for accel sensor */
    sensor1_sensor_state_t  accel_state;              /* All necessary state for gyro sensor */
    sensor1_sensor_state_t  baro_state;               /* All necessary state for gyro sensor */
    sensor1_sensor_state_t  gyro_temperature_state;   /* All necessary state for gyro temperature sensor */
    sensor1_sensor_state_t  accel_temperature_state;  /* All necessary state for accel temperature sensor */
    bool                    error;                    /* boolean directive */
    bool                    is_resp_arrived;           /* Flag to indicate that a response has arrived */
    uint32_t                dsps_clock_ticks;         /* The most recent Sensor Core clock ticks response */
} sensor1_control_t;

static sensor1_control_t* g_sensor1_control = NULL;

/* Static function declarations */
static void process_time_resp( sensor1_msg_header_s *msg_hdr, void *msg_ptr );
static bool sns1_wait_for_response( int timeout,pthread_mutex_t* cb_mutex_ptr,
                       pthread_cond_t*  cond_ptr,bool *cond_var );
static bool sns1_time_req();

extern slimBaroDataPayloadStructT* p_baro_data;
extern access_control_t* g_access_control;

/*===========================================================================
FUNCTION    sp_sns1_init_defaults

DESCRIPTION
  Initialization function for sensor1 internal state.

DEPENDENCIES
   N/A

RETURN VALUE
   N/A

SIDE EFFECTS
   N/A

===========================================================================*/
static bool sp_sns1_init_defaults()
{
    if( g_sensor1_control != NULL )
    {
        free(g_sensor1_control);
        g_sensor1_control = NULL;
    }

    /* Fill with default values */
    g_sensor1_control = (sensor1_control_t*)calloc(1, sizeof(*g_sensor1_control));

    if( NULL == g_sensor1_control )
    {
        LOC_LOGE("%s: Error: calloc error", __FUNCTION__);
        return false;
    }

    g_sensor1_control->spi_algo_instance_id = -1;
    g_sensor1_control->p_msg_q = NULL;

    /* Mark data as valid so LocApi will accept it - Only needs to happen once*/
    g_sensor1_control->accel_state.data_msg.msg_type = GSIFF_INJECT_SENSOR_DATA_ACCEL_REQ;

    g_sensor1_control->gyro_state.data_msg.msg_type = GSIFF_INJECT_SENSOR_DATA_GYRO_REQ;

    g_sensor1_control->accel_temperature_state.data_msg.msg_type = GSIFF_INJECT_SENSOR_TEMPERATURE_ACCEL_REQ;

    g_sensor1_control->gyro_temperature_state.data_msg.msg_type = GSIFF_INJECT_SENSOR_TEMPERATURE_GYRO_REQ;

    g_sensor1_control->baro_state.data_msg.msg_type = GSIFF_BAROMETER_REQ;

    return true;
}

/*===========================================================================
FUNCTION    sns_smgr_data_item_s_comparator

DESCRIPTION
   Comparator function to be used by qsort to place sensor samples in order.
   The ordering is based on the timestamp of each sample which should be monotonically
   increasing.

   a: Pointer to an sns_smgr_data_item_s to sort in order
   b: Pointer to an sns_smgr_data_item_s to sort in order

DEPENDENCIES
   N/A

RETURN VALUE
   -  : a's timestamp is less than b's timestamp
   0  : a's timestamp is equal to b's timestamp
   +  : a's timestamp is greater than b's timestamp

SIDE EFFECTS
   N/A

===========================================================================*/
static int sns_smgr_data_item_s_comparator (const void * a, const void * b)
{
   sns_smgr_data_item_s_v01* as = (sns_smgr_data_item_s_v01*)a;
   sns_smgr_data_item_s_v01* bs = (sns_smgr_data_item_s_v01*)b;

   return (as->TimeStamp - bs->TimeStamp);
}

/*===========================================================================

  FUNCTION:   sp_sns1_update_motion_data

  ===========================================================================*/
bool sp_sns1_update_motion_data(bool running)
{
   sns_sam_qmd_enable_req_msg_v01* ena_amd_sam_req = NULL;
   sns_sam_qmd_disable_req_msg_v01* dis_amd_sam_req = NULL;

   void* rmd_sam_req = NULL;
   sensor1_error_e error;
   bool messages_sent = true;

   /* Message headers */
   sensor1_msg_header_s amd_req_hdr;
   sensor1_msg_header_s rmd_req_hdr;

   if( g_sensor1_control == NULL )
   {
      LOC_LOGE("%s: Initialization required to perform operation", __FUNCTION__);
      return false;
   }

   /* Turn on AMD and RMD reporting */
   if( running && !g_sensor1_control->motion_state.running )
   {
      LOC_LOGI("%s: Turning on AMD reporting", __FUNCTION__);

      amd_req_hdr.service_number = SNS_SAM_AMD_SVC_ID_V01;
      amd_req_hdr.txn_id = 0;
      amd_req_hdr.msg_id = SNS_SAM_AMD_ENABLE_REQ_V01;
      amd_req_hdr.msg_size = sizeof( sns_sam_qmd_enable_req_msg_v01 );

      error = sensor1_alloc_msg_buf( g_sensor1_control->sensor1_client_handle,
                                                     sizeof(sns_sam_qmd_enable_req_msg_v01),
                                                     (void**)&ena_amd_sam_req );
      if ( SENSOR1_SUCCESS != error )
      {
         LOC_LOGE("%s:%u] sensor1_alloc_msg_buf add returned %d", __FUNCTION__,__LINE__, error);
         return false;
      }

      sns_sam_qmd_enable_req_msg_v01* enable_amd_sam_req = (sns_sam_qmd_enable_req_msg_v01*)ena_amd_sam_req;
      /* Only report on new AMD events */
      enable_amd_sam_req->report_period = 0;

      /* Send across Sensor1 Interface */
      if ( (error = sensor1_write( g_sensor1_control->sensor1_client_handle, &amd_req_hdr,
                                   ena_amd_sam_req )) != SENSOR1_SUCCESS )
      {
         /* Error so we deallocate QMI message */
         sensor1_free_msg_buf( g_sensor1_control->sensor1_client_handle, ena_amd_sam_req );
         LOC_LOGE("%s:%u] sensor1_write() error: %u", __FUNCTION__,__LINE__, error );
         messages_sent = false;
      }

      LOC_LOGI("%s: Turning on RMD reporting", __FUNCTION__);

      rmd_req_hdr.service_number = SNS_SAM_RMD_SVC_ID_V01;
      rmd_req_hdr.txn_id = 0;
      rmd_req_hdr.msg_id = SNS_SAM_RMD_ENABLE_REQ_V01;
      rmd_req_hdr.msg_size = sizeof( sns_sam_qmd_enable_req_msg_v01 );

      error = sensor1_alloc_msg_buf( g_sensor1_control->sensor1_client_handle,
                                                     sizeof(sns_sam_qmd_enable_req_msg_v01),
                                                     (void**)&rmd_sam_req );
      if ( SENSOR1_SUCCESS != error )
      {
         LOC_LOGE("%s:%u] sensor1_alloc_msg_buf add returned %d", __FUNCTION__,__LINE__, error);
         return false;
      }

      sns_sam_qmd_enable_req_msg_v01* enable_rmd_sam_req = (sns_sam_qmd_enable_req_msg_v01*)rmd_sam_req;
      /* Only report on new RMD events */
      enable_rmd_sam_req->report_period = 0;

      /* Send across Sensor1 Interface */
      if ( (error = sensor1_write( g_sensor1_control->sensor1_client_handle, &rmd_req_hdr,
                                   rmd_sam_req )) != SENSOR1_SUCCESS )
      {
         /* Error so we deallocate QMI message */
         sensor1_free_msg_buf( g_sensor1_control->sensor1_client_handle, rmd_sam_req );
         LOC_LOGE("%s:%u] sensor1_write() error: %u", __FUNCTION__,__LINE__, error );
         messages_sent = false;
      }
   }
   /* Turn off AMD amd RMD reporting */
   else if( !running && g_sensor1_control->motion_state.running )
   {
      LOC_LOGI("%s: Turning off AMD reporting", __FUNCTION__);

      amd_req_hdr.service_number = SNS_SAM_AMD_SVC_ID_V01;
      amd_req_hdr.txn_id = 0;
      amd_req_hdr.msg_id = SNS_SAM_AMD_DISABLE_REQ_V01;
      amd_req_hdr.msg_size = sizeof( sns_sam_qmd_disable_req_msg_v01 );
      error = sensor1_alloc_msg_buf( g_sensor1_control->sensor1_client_handle,
                                                     sizeof(sns_sam_qmd_disable_req_msg_v01),
                                                     (void**)&dis_amd_sam_req );

      if ( SENSOR1_SUCCESS != error )
      {
         LOC_LOGE("%s:%u] sensor1_alloc_msg_buf add returned %d", __FUNCTION__,__LINE__, error);
         return false;
      }

      sns_sam_qmd_disable_req_msg_v01* disable_amd_sam_req = (sns_sam_qmd_disable_req_msg_v01*)dis_amd_sam_req;
      disable_amd_sam_req->instance_id = g_sensor1_control->motion_state.amd_algo_instance_id;

      /* Send across Sensor1 Interface */
      if ( (error = sensor1_write( g_sensor1_control->sensor1_client_handle, &amd_req_hdr,
                                   dis_amd_sam_req )) != SENSOR1_SUCCESS )
      {
         /* Error so we deallocate QMI message */
         sensor1_free_msg_buf( g_sensor1_control->sensor1_client_handle, dis_amd_sam_req );
         LOC_LOGE("%s:%u] sensor1_write() error: %u", __FUNCTION__,__LINE__, error );
         messages_sent = false;
      }

      LOC_LOGI("%s: Turning off RMD reporting", __FUNCTION__);

      rmd_req_hdr.service_number = SNS_SAM_RMD_SVC_ID_V01;
      rmd_req_hdr.txn_id = 0;
      rmd_req_hdr.msg_id = SNS_SAM_RMD_DISABLE_REQ_V01;
      rmd_req_hdr.msg_size = sizeof( sns_sam_qmd_disable_req_msg_v01 );
      error = sensor1_alloc_msg_buf( g_sensor1_control->sensor1_client_handle,
                                                     sizeof(sns_sam_qmd_disable_req_msg_v01),
                                                     (void**)&rmd_sam_req );

      if ( SENSOR1_SUCCESS != error )
      {
         LOC_LOGE("%s:%u] sensor1_alloc_msg_buf add returned %d", __FUNCTION__,__LINE__, error);
         return false;
      }

      sns_sam_qmd_disable_req_msg_v01* disable_rmd_sam_req = (sns_sam_qmd_disable_req_msg_v01*)rmd_sam_req;
      disable_rmd_sam_req->instance_id = g_sensor1_control->motion_state.rmd_algo_instance_id;

      /* Send across Sensor1 Interface */
      if ( (error = sensor1_write( g_sensor1_control->sensor1_client_handle, &rmd_req_hdr,
                                   rmd_sam_req )) != SENSOR1_SUCCESS )
      {
         /* Error so we deallocate QMI message */
         sensor1_free_msg_buf( g_sensor1_control->sensor1_client_handle, rmd_sam_req );
         LOC_LOGE("%s:%u] sensor1_write() error: %u", __FUNCTION__,__LINE__, error );
         messages_sent = false;
      }
   }
   /* Enabling if already enabled or disabling while already disabled. */
   else
   {
      return true;
   }

   if (messages_sent)
   {
     g_sensor1_control->motion_state.running = running;
   }
   g_sensor1_control->motion_state.amd = SNS_SAM_MOTION_UNKNOWN_V01;
   g_sensor1_control->motion_state.rmd = SNS_SAM_MOTION_UNKNOWN_V01;

   return true;
}

/*===============================================================i============

FUNCTION    sp_sns1_update_pedometer

===========================================================================*/
bool sp_sns1_update_pedometer(bool running, uint8_t reset_step_count, uint32_t step_count_threshold)
{
   if( g_sensor1_control == NULL )
   {
      LOC_LOGE("%s: Initialization required to perform operation", __FUNCTION__);
      return false;
   }

   void* sam_req = NULL;
   sensor1_error_e error;

   /* Message header */
   sensor1_msg_header_s sam_req_hdr;
   sam_req_hdr.service_number = SNS_SAM_PED_SVC_ID_V01;
   sam_req_hdr.txn_id = 0;

   /* Turn off Pedometer reporting */
   if( (!running && g_sensor1_control->pedometer_state.running) ||
       (running && g_sensor1_control->pedometer_state.running && step_count_threshold != g_sensor1_control->pedometer_state.step_count_threshold))
   {
      LOC_LOGI("%s: Turning off Pedometer reporting", __FUNCTION__);

      sam_req_hdr.msg_id = SNS_SAM_PED_DISABLE_REQ_V01;
      sam_req_hdr.msg_size = sizeof( sns_sam_ped_disable_req_msg_v01 );
      error = sensor1_alloc_msg_buf( g_sensor1_control->sensor1_client_handle,
                                                     sizeof(sns_sam_ped_disable_req_msg_v01),
                                                     (void**)&sam_req );

      if ( SENSOR1_SUCCESS != error )
      {
         LOC_LOGE("%s: sensor1_alloc_msg_buf add returned %d", __FUNCTION__, error);
         return false;
      }

      sns_sam_ped_disable_req_msg_v01* disable_ped_sam_req = (sns_sam_ped_disable_req_msg_v01*)sam_req;
      disable_ped_sam_req->instance_id = g_sensor1_control->pedometer_state.algo_instance_id;

      /* Send across Sensor1 Interface */
      if ( (error = sensor1_write( g_sensor1_control->sensor1_client_handle, &sam_req_hdr,
                                   sam_req )) != SENSOR1_SUCCESS )
      {
         /* Error so we deallocate QMI message */
         sensor1_free_msg_buf( g_sensor1_control->sensor1_client_handle, sam_req );
         LOC_LOGE("%s: sensor1_write() error: %u", __FUNCTION__, error );
         return false;
      }
      else
      {
        g_sensor1_control->pedometer_state.running = false;
      }
   }
   /* Resetting the step count. */
   else if( running && g_sensor1_control->pedometer_state.running && reset_step_count )
   {
      LOC_LOGI("%s: Resetting the Pedometer step count.", __FUNCTION__);

      sam_req_hdr.msg_id = SNS_SAM_PED_RESET_REQ_V01;
      sam_req_hdr.msg_size = sizeof( sns_sam_ped_reset_req_msg_v01 );
      error = sensor1_alloc_msg_buf( g_sensor1_control->sensor1_client_handle,
                                                     sizeof(sns_sam_ped_reset_req_msg_v01),
                                                     (void**)&sam_req );

      if ( SENSOR1_SUCCESS != error )
      {
         LOC_LOGE("%s: sensor1_alloc_msg_buf add returned %d", __FUNCTION__, error);
         return false;
      }

      sns_sam_ped_reset_req_msg_v01* reset_ped_sam_req = (sns_sam_ped_reset_req_msg_v01*)sam_req;
      reset_ped_sam_req->instance_id = g_sensor1_control->pedometer_state.algo_instance_id;

      /* Send across Sensor1 Interface */
      if ( (error = sensor1_write( g_sensor1_control->sensor1_client_handle, &sam_req_hdr,
                                   sam_req )) != SENSOR1_SUCCESS )
      {
         /* Error so we deallocate QMI message */
         sensor1_free_msg_buf( g_sensor1_control->sensor1_client_handle, sam_req );
         LOC_LOGE("%s: sensor1_write() error: %u", __FUNCTION__, error );
         return false;
      }
   }
   /* Turn on Pedometer reporting */
   if( running && !g_sensor1_control->pedometer_state.running )
   {
      LOC_LOGI("%s: Turning on Pedometer reporting", __FUNCTION__);

      sam_req_hdr.msg_id = SNS_SAM_PED_ENABLE_REQ_V01;
      sam_req_hdr.msg_size = sizeof( sns_sam_ped_enable_req_msg_v01 );
      error = sensor1_alloc_msg_buf( g_sensor1_control->sensor1_client_handle,
                                                     sizeof(sns_sam_ped_enable_req_msg_v01),
                                                     (void**)&sam_req );
      if ( SENSOR1_SUCCESS != error )
      {
         LOC_LOGE("%s: sensor1_alloc_msg_buf add returned %d", __FUNCTION__, error);
         return false;
      }

      sns_sam_ped_enable_req_msg_v01* enable_ped_sam_req = (sns_sam_ped_enable_req_msg_v01*)sam_req;
      /* GSIFF supports only event based pedometer reports. */
      enable_ped_sam_req->report_period = 0;
      if (step_count_threshold > 0)
      {
        enable_ped_sam_req->step_count_threshold_valid = true;
        enable_ped_sam_req->step_count_threshold = step_count_threshold;
      }

      /* Send across Sensor1 Interface */
      if ( (error = sensor1_write( g_sensor1_control->sensor1_client_handle, &sam_req_hdr,
                                   sam_req )) != SENSOR1_SUCCESS )
      {
         /* Error so we deallocate QMI message */
         sensor1_free_msg_buf( g_sensor1_control->sensor1_client_handle, sam_req );
         LOC_LOGE("%s: sensor1_write() error: %u", __FUNCTION__, error );
         return false;
      }
      else
      {
        g_sensor1_control->pedometer_state.running = running;
      }
   }

   return true;
}

/*===========================================================================
FUNCTION    update_spi_reporting_status

DESCRIPTION
  Function updates the current running status of spi updates from this sensor
  provider

  running:       TRUE - start spi reporting, FALSE - stop sensor reporting

  Note: Function should not be called from a time sensitive thread as this
        function may block for some time to start up spi reporting.

DEPENDENCIES
   N/A

RETURN VALUE
   0/FALSE : Failure
   1/TRUE  : Successful

SIDE EFFECTS
   N/A


===========================================================================*/
static bool update_spi_reporting_status(bool running)
{
   void* sam_req = NULL;
   sensor1_error_e error;

   /* Message header */
   sensor1_msg_header_s req_hdr;
   req_hdr.service_number = SNS_SAM_VMD_SVC_ID_V01;
   req_hdr.txn_id = 0;

   /* Turn on spi reporting */
   if( running && !g_sensor1_control->spi_running )
   {
      LOC_LOGI("%s: Turning on spi reporting", __FUNCTION__);

      req_hdr.msg_id = SNS_SAM_VMD_ENABLE_REQ_V01;
      req_hdr.msg_size = sizeof( sns_sam_qmd_enable_req_msg_v01 );

      error = sensor1_alloc_msg_buf( g_sensor1_control->sensor1_client_handle,
                                                     sizeof(sns_sam_qmd_enable_req_msg_v01),
                                                     (void**)&sam_req );
      if ( SENSOR1_SUCCESS != error )
      {
         LOC_LOGE("%s: sensor1_alloc_msg_buf add returned %d", __FUNCTION__, error);
         return false;
      }

      sns_sam_qmd_enable_req_msg_v01* enable_sam_req = (sns_sam_qmd_enable_req_msg_v01*)sam_req;
      enable_sam_req->report_period = 0;               /* Only report on new spi events */
   }
   /* Turn off spi reporting */
   else if( !running && g_sensor1_control->spi_running )
   {
      LOC_LOGI("%s: Turning off spi reporting", __FUNCTION__);

      req_hdr.msg_id = SNS_SAM_VMD_DISABLE_REQ_V01;
      req_hdr.msg_size = sizeof( sns_sam_qmd_disable_req_msg_v01 );
      error = sensor1_alloc_msg_buf( g_sensor1_control->sensor1_client_handle,
                                                     sizeof(sns_sam_qmd_disable_req_msg_v01),
                                                     (void**)&sam_req );

      if ( SENSOR1_SUCCESS != error )
      {
         LOC_LOGE("%s: sensor1_alloc_msg_buf add returned %d", __FUNCTION__, error);
         return false;
      }

      sns_sam_qmd_disable_req_msg_v01* disable_sam_req = (sns_sam_qmd_disable_req_msg_v01*)sam_req;
      disable_sam_req->instance_id = g_sensor1_control->spi_algo_instance_id;
   }
   /* Enabling if already enabled or disabling while already disabled. */
   else
   {
      return true;
   }

   /* Send across Sensor1 Interface */
   if ( (error = sensor1_write( g_sensor1_control->sensor1_client_handle, &req_hdr,
                                sam_req )) != SENSOR1_SUCCESS )
   {
      /* Error so we deallocate QMI message */
      sensor1_free_msg_buf( g_sensor1_control->sensor1_client_handle, sam_req );
      LOC_LOGE("%s: sensor1_write() error: %u", __FUNCTION__, error );
      return false;
   }
   else
   {
      g_sensor1_control->spi_running = running;
   }

   return true;
}

/*===========================================================================
FUNCTION    sp_sns1_update_sensor_status

DESCRIPTION
  Generic function to start/stop a sensor based on provided sampling rate,
  batching rate, mounted state, and sensor information using Sensor1 API.

  running:       TRUE - start sensor reporting, FALSE - stop sensor reporting
  sampling_rate: Sampling rate in Hz
  batching_rate: Batch samples at this frequency in Hz
  mounted_state: 0 - Send Unmounted State, 1 - Send Mounted State, 2 - Do Not Send
  sensor_state:  The state associated with this sensor
  sensor_type:   What kind of sensor are we updating

DEPENDENCIES
   N/A

RETURN VALUE
   0/FALSE : Failure
   1/TRUE  : Successful

SIDE EFFECTS
   N/A

===========================================================================*/
static bool sp_sns1_update_sensor_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state,
                                    sensor1_sensor_state_t* sensor_state, e_sensor_type sensor_type)
{
   const char* sensor_str;
   uint8_t sns1_sensor_id;
   uint8_t sns1_reporting_id;
   uint8_t sns1_data_type;

   switch (sensor_type)
   {
     case ACCELEROMETER_SENSOR_TYPE:
       sensor_str = "accel";
       sns1_sensor_id = SNS_SMGR_ID_ACCEL_V01;
       sns1_reporting_id = ACCEL_REPORTING_ID;
       sns1_data_type = SNS_SMGR_DATA_TYPE_PRIMARY_V01;
     break;

     case GYROSCOPE_SENSOR_TYPE:
       sensor_str = "gyro";
       sns1_sensor_id = SNS_SMGR_ID_GYRO_V01;
       sns1_reporting_id = GYRO_REPORTING_ID;
       sns1_data_type = SNS_SMGR_DATA_TYPE_PRIMARY_V01;
     break;

     case ACCELEROMETER_TEMPERATURE_SENSOR_TYPE:
       sensor_str = "accel_temperature";
       sns1_sensor_id = SNS_SMGR_ID_ACCEL_V01;
       sns1_reporting_id = ACCEL_TEMPERATURE_REPORTING_ID;
       sns1_data_type = SNS_SMGR_DATA_TYPE_SECONDARY_V01;
     break;

     case GYROSCOPE_TEMPERATURE_SENSOR_TYPE:
       sensor_str = "gyro_temperature";
       sns1_sensor_id = SNS_SMGR_ID_GYRO_V01;
       sns1_reporting_id = GYRO_TEMPERATURE_REPORTING_ID;
       sns1_data_type = SNS_SMGR_DATA_TYPE_SECONDARY_V01;
     break;

     case BAROMETER_SENSOR_TYPE:
       sensor_str = "pressure";
       sns1_sensor_id = SNS_SMGR_ID_PRESSURE_V01;
       sns1_reporting_id = PRESSURE_REPORTING_ID;
       sns1_data_type = SNS_SMGR_DATA_TYPE_PRIMARY_V01;
     break;

     default:
       LOC_LOGE("%s: Unknown Sensor Type %d", __FUNCTION__, sensor_type);
       return false;
     break;
   }

   /* No sensor state to use. */
   if( sensor_state == NULL )
   {
      LOC_LOGE("%s: No %s sensor state provided to start/stop", __FUNCTION__, sensor_str);
      return false;
   }

   /* No State Change */
   if( !running && !sensor_state->running )
   {
      /* Sensors stay Off */
      return true;
   }
   /* Sensors stay On but no change in sensor sampling rate */
   else if ( (running && sensor_state->running) && (sampling_rate == sensor_state->reporting_rate) )
   {
      return true;
   }

   sensor1_msg_header_s req_hdr;
   sns_smgr_periodic_report_req_msg_v01* smgr_req = NULL;

   sensor1_error_e error = sensor1_alloc_msg_buf( g_sensor1_control->sensor1_client_handle,
                                                  sizeof(sns_smgr_periodic_report_req_msg_v01),
                                                  (void**)&smgr_req );
   if ( SENSOR1_SUCCESS != error )
   {
      LOC_LOGE("%s: sensor1_alloc_msg_buf add returned %d", __FUNCTION__, error);
      return false;
   }

   /* Message header */
   req_hdr.service_number = SNS_SMGR_SVC_ID_V01;
   req_hdr.msg_id = SNS_SMGR_REPORT_REQ_V01;
   req_hdr.msg_size = sizeof( sns_smgr_periodic_report_req_msg_v01 );
   req_hdr.txn_id = 0;

   if( running )
   {
      /* Turn on sensor */
      if ( !sensor_state->running )
      {
         LOC_LOGI("%s: Turning on %s reporting", __FUNCTION__, sensor_str);
      }
      /* Sensor sampling rate change */
      else if ( sensor_state->running )
      {
         LOC_LOGI("%s: Changing sampling rate on %s reporting", __FUNCTION__, sensor_str);
      }
      sensor_state->reporting_rate = sampling_rate;
      sensor_state->batching_rate = batching_rate;
      sensor_state->reporting_interval = 1000.0/sampling_rate;
      sensor_state->batching_interval = 1000.0/batching_rate;

      smgr_req->ReportId = sns1_reporting_id;
      smgr_req->Action = SNS_SMGR_REPORT_ACTION_ADD_V01;
      smgr_req->ReportRate = sampling_rate;
      smgr_req->BufferFactor = 0;
      smgr_req->Item_len = 1;
      smgr_req->Item[0].SensorId = sns1_sensor_id;
      smgr_req->Item[0].DataType = sns1_data_type;
      smgr_req->Item[0].Sensitivity = 0; /* Default */
      smgr_req->Item[0].Decimation = SNS_SMGR_DECIMATION_RECENT_SAMPLE_V01;
      smgr_req->Item[0].MinSampleRate = 0;
      smgr_req->Item[0].StationaryOption = SNS_SMGR_REST_OPTION_REPORT_FULL_V01;
      smgr_req->Item[0].DoThresholdTest = 0;
      smgr_req->Item[0].ThresholdOutsideMinMax = 0;
      smgr_req->Item[0].ThresholdDelta = 0;
      smgr_req->Item[0].ThresholdAllAxes = 0;
      smgr_req->Item[0].ThresholdMinMax[0] = 0;
      smgr_req->Item[0].ThresholdMinMax[1] = 0;
      /* DSPS sensor calibration selection */
      smgr_req->cal_sel_valid = 1;
      smgr_req->cal_sel_len = 1;
      smgr_req->cal_sel[0] = SNS_SMGR_CAL_SEL_FACTORY_CAL_V01;
      /*enable sensor streaming even when the processor is in suspend state */
      smgr_req->notify_suspend_valid = 1;
      smgr_req->notify_suspend.send_indications_during_suspend = 1;
      smgr_req->notify_suspend.proc_type = SNS_PROC_APPS_V01;
   }
   /* Turn off sensor */
   else if( !running && sensor_state->running )
   {
      LOC_LOGI("%s: Turning off %s reporting", __FUNCTION__, sensor_str);

      smgr_req->ReportId = sns1_reporting_id;
      smgr_req->Action = SNS_SMGR_REPORT_ACTION_DELETE_V01;
      smgr_req->ReportRate = 0;
      smgr_req->BufferFactor = 0;
      smgr_req->Item_len = 0;
   }

   /* Clear out any old samples that may be lingering from previous starts/stops. */
   memset(&sensor_state->data_msg.msg_data,
          0,
          sizeof(sensor_state->data_msg.msg_data));

   /* Send across Sensor1 Interface */
   if ( (error = sensor1_write( g_sensor1_control->sensor1_client_handle, &req_hdr,
                                smgr_req )) != SENSOR1_SUCCESS )
   {
      /* Error so we deallocate QMI message */
      sensor1_free_msg_buf( g_sensor1_control->sensor1_client_handle, smgr_req );
      LOC_LOGE("%s: sensor1_write() error: %u", __FUNCTION__, error );
      return false;
   }
   else
   {
      /* Since Accel/Gyro reporting is separate we only want to send XMSI once
      when starting sensor data injection. This will prevent an XMSI message
      if gyro/accel data has already started. */
      if( !g_sensor1_control->accel_state.running && !g_sensor1_control->gyro_state.running &&
          !g_sensor1_control->accel_temperature_state.running &&
          !g_sensor1_control->gyro_temperature_state.running && running)
      {
         sp_send_msi_mounted_status(g_sensor1_control->p_msg_q, mounted_state);
      }

      /* Update running state after performing MSI to make the function generic */
      sensor_state->running = running;
   }

   return true;
}

/*===========================================================================

  FUNCTION:   sp_sns1_update_spi_status

  ===========================================================================*/
bool sp_sns1_update_spi_status(bool running)
{
   if( g_sensor1_control == NULL )
   {
      LOC_LOGE("%s: Initialization required to perform operation", __FUNCTION__);
      return false;
   }

   return update_spi_reporting_status(running);
}

/*===========================================================================
FUNCTION    process_smgr_resp

DESCRIPTION
  Handler for Sensor1 SMGR (Sensor Manager) Service Responses.

DEPENDENCIES
   N/A

RETURN VALUE
   N/A

SIDE EFFECTS
   N/A

===========================================================================*/
static void process_smgr_resp(sensor1_msg_header_s *msg_hdr, void* msg_ptr)
{
   LOC_LOGI("%s: msg_id = %d", __FUNCTION__, msg_hdr->msg_id );
   bool error = false;
   int status = SLIM_BARO_STATUS_RESP_OK;

   if ( msg_hdr->msg_id == SNS_SMGR_REPORT_RESP_V01 )
   {
      sns_smgr_periodic_report_resp_msg_v01* smgr_resp = (sns_smgr_periodic_report_resp_msg_v01*)msg_ptr;

      if ( smgr_resp->Resp.sns_result_t != SNS_RESULT_SUCCESS_V01 || smgr_resp->Resp.sns_err_t != SENSOR1_SUCCESS )
      {
         LOC_LOGE("%s: Result: %u, Error: %u", __FUNCTION__,
              smgr_resp->Resp.sns_result_t, smgr_resp->Resp.sns_err_t );
         error = true;
      }

      if ( smgr_resp->AckNak != SNS_SMGR_RESPONSE_ACK_SUCCESS_V01 &&
           smgr_resp->AckNak != SNS_SMGR_RESPONSE_ACK_MODIFIED_V01 )
      {
         LOC_LOGE("%s: %d Error: %u Reason: %u", __FUNCTION__, smgr_resp->ReportId,
              smgr_resp->AckNak, smgr_resp->ReasonPair[0].Reason );
         error = true;
      }

      LOC_LOGI("%s: ReportId: %u Resp: %u", __FUNCTION__, smgr_resp->ReportId,
           smgr_resp->AckNak  );

      //send baro response status back to PIP Engine
      if(smgr_resp->ReportId == PRESSURE_REPORTING_ID)
      {
          if(error)
          {
              status = SLIM_BARO_STATUS_RESP_GENERIC_ERROR;
              gsiff_send_baro_resp(status);
          }
          else
              gsiff_send_baro_resp(status);
      }
   }
   else
   {
      LOC_LOGE("%s: Received unexpected smgr resp msg_id = %d",
               __FUNCTION__, msg_hdr->msg_id);
   }
}

/*===========================================================================
FUNCTION    process_smgr_ind

DESCRIPTION
  Handler for Sensor1 SMGR (Sensor Manager) Service Indications. Sensor data
  input is handled here.

DEPENDENCIES
   N/A

RETURN VALUE
   N/A

SIDE EFFECTS
   N/A

===========================================================================*/
void process_smgr_ind(sensor1_msg_header_s *msg_hdr, void* msg_ptr)
{
    LOC_LOGD("%s: msg_id = %d", __FUNCTION__, msg_hdr->msg_id );

    /* Received sensor data */
    if( msg_hdr->msg_id == SNS_SMGR_REPORT_IND_V01 )
    {
        uint32_t i = 0;
        sns_smgr_periodic_report_ind_msg_v01* smgr_ind = (sns_smgr_periodic_report_ind_msg_v01*) msg_ptr;

        LOC_LOGI("%s: St: %d ReportId: %d Rate: %d, Len: %d", __FUNCTION__,
                 smgr_ind->status, smgr_ind->ReportId,
                 smgr_ind->CurrentRate, smgr_ind->Item_len );

        /* Check report status */
        if ( SNS_SMGR_REPORT_OK_V01 != smgr_ind->status )
        {
            LOC_LOGE("%s: Report Status: %u", __FUNCTION__, smgr_ind->status );
            return;
        }

        /* Don't want the sensor data anymore */
        if( !g_sensor1_control->accel_state.running && !g_sensor1_control->gyro_state.running && !g_sensor1_control->baro_state.running)
        {
            return;
        }

        /* Sort the array in order based on timestamps to ensure samples are in order. */
        qsort(smgr_ind->Item, smgr_ind->Item_len, sizeof(sns_smgr_data_item_s_v01), sns_smgr_data_item_s_comparator);

        for( i = 0; i < smgr_ind->Item_len; ++i )
        {
            sns_smgr_data_item_s_v01* smgr_data = &smgr_ind->Item[i];

            const char* sensor_str;

            switch (smgr_data->SensorId)
            {
            case SNS_SMGR_ID_ACCEL_V01:
                switch (smgr_data->DataType)
                {
                case SNS_SMGR_DATA_TYPE_PRIMARY_V01:
                    sensor_str = "ACCEL_DATA";
                    break;
                case SNS_SMGR_DATA_TYPE_SECONDARY_V01:
                    sensor_str = "ACCEL_TEMPERATURE_DATA";
                    break;
                default:
                    sensor_str = "invalid";
                    break;
                }
                break;

            case SNS_SMGR_ID_GYRO_V01:
                switch (smgr_data->DataType)
                {
                case SNS_SMGR_DATA_TYPE_PRIMARY_V01:
                    sensor_str = "GYRO_DATA";
                    break;
                case SNS_SMGR_DATA_TYPE_SECONDARY_V01:
                    sensor_str = "GYRO_TEMPERATURE_DATA";
                    break;
                default:
                    sensor_str = "invalid";
                    break;
                }
                break;

            case SNS_SMGR_ID_MAG_V01:
                sensor_str = "MAG_DATA";
                break;

            case SNS_SMGR_ID_PROX_LIGHT_V01:
                sensor_str = "PROX_LIGHT_DATA";
                break;

            case SNS_SMGR_ID_PRESSURE_V01:
                sensor_str = "PRESSURE_DATA";
                break;

            default:
                sensor_str = "invalid";
                break;
            }

            LOC_LOGD("%s: %s: Ty: %u Q: %u", __FUNCTION__,
                     sensor_str, smgr_data->DataType, smgr_data->ItemQuality );

            /* Validity checks on sensor data */
            if ( smgr_data->ItemQuality != SNS_SMGR_ITEM_QUALITY_INVALID_FAILED_SENSOR_V01 &&
                 smgr_data->ItemQuality != SNS_SMGR_ITEM_QUALITY_INVALID_NOT_READY_V01 &&
                 smgr_data->ItemQuality != SNS_SMGR_ITEM_QUALITY_INVALID_SUSPENDED_V01 )
            {
                gsiff_msg* p_gsiff_data_msg = NULL;
                qmiLoc3AxisSensorSampleListStructT_v02* p_sensor_data_tlv = NULL;
                qmiLocSensorTemperatureSampleListStructT_v02* p_sensor_temp_tlv = NULL;

                uint32_t timestamp_ms = DSPS_TICK_TO_MSEC(smgr_data->TimeStamp);
                float sensor_samples[3] = {0, 0, 0};
                double batching_interval = 0;
                double sample_interval = 0;

                switch ( smgr_data->SensorId )
                {
                case SNS_SMGR_ID_ACCEL_V01:
                    if ( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == smgr_data->DataType )
                    {
                        p_gsiff_data_msg = &g_sensor1_control->accel_state.data_msg;
                        p_sensor_data_tlv = &g_sensor1_control->accel_state.data_msg.msg_data.sensor_data_samples;

                        sensor_samples[0] = (float)(smgr_data->ItemData[1]) * UNIT_CONVERT_ACCELERATION;
                        sensor_samples[1] = (float)(smgr_data->ItemData[0]) * UNIT_CONVERT_ACCELERATION;
                        sensor_samples[2] = (float)(-smgr_data->ItemData[2]) * UNIT_CONVERT_ACCELERATION;

                        batching_interval = g_sensor1_control->accel_state.batching_interval;
                        sample_interval = g_sensor1_control->accel_state.reporting_interval;
                    }
                    else if ( SNS_SMGR_DATA_TYPE_SECONDARY_V01 == smgr_data->DataType )
                    {
                        p_gsiff_data_msg = &g_sensor1_control->accel_temperature_state.data_msg;
                        p_sensor_temp_tlv = &g_sensor1_control->accel_temperature_state.data_msg.msg_data.sensor_temp_samples;

                        sensor_samples[0] = (float)smgr_data->ItemData[0] * UNIT_CONVERT_Q16;

                        batching_interval = g_sensor1_control->accel_temperature_state.batching_interval;
                        sample_interval = g_sensor1_control->accel_temperature_state.reporting_interval;
                    }
                    else
                    {
                        LOC_LOGE("%s: Unexpected data type = %d (%s)", __FUNCTION__, smgr_data->DataType, sensor_str);
                    }
                    break;

                case SNS_SMGR_ID_GYRO_V01:
                    if ( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == smgr_data->DataType )
                    {
                        p_gsiff_data_msg = &g_sensor1_control->gyro_state.data_msg;
                        p_sensor_data_tlv = &g_sensor1_control->gyro_state.data_msg.msg_data.sensor_data_samples;

                        sensor_samples[0] = (float)(smgr_data->ItemData[1]) * UNIT_CONVERT_GYRO;
                        sensor_samples[1] = (float)(smgr_data->ItemData[0]) * UNIT_CONVERT_GYRO;
                        sensor_samples[2] = (float)(-smgr_data->ItemData[2]) * UNIT_CONVERT_GYRO;

                        batching_interval = g_sensor1_control->gyro_state.batching_interval;
                        sample_interval = g_sensor1_control->gyro_state.reporting_interval;
                    }
                    else if ( SNS_SMGR_DATA_TYPE_SECONDARY_V01 == smgr_data->DataType )
                    {
                        p_gsiff_data_msg = &g_sensor1_control->gyro_temperature_state.data_msg;
                        p_sensor_temp_tlv = &g_sensor1_control->gyro_temperature_state.data_msg.msg_data.sensor_temp_samples;

                        sensor_samples[0] = (float)smgr_data->ItemData[0] * UNIT_CONVERT_Q16;

                        batching_interval = g_sensor1_control->gyro_temperature_state.batching_interval;
                        sample_interval = g_sensor1_control->gyro_temperature_state.reporting_interval;
                    }
                    else
                    {
                        LOC_LOGE("%s: Unexpected data type = %d (%s)", __FUNCTION__, smgr_data->DataType, sensor_str);
                    }
                    break;
                case SNS_SMGR_ID_PRESSURE_V01:
                    if ( SNS_SMGR_DATA_TYPE_PRIMARY_V01 == smgr_data->DataType )
                    {
                        uint32_t reporting_rate = 0;
                        sensor_samples[0] = (float)(smgr_data->ItemData[0]) * UNIT_CONVERT_PRESSURE;
                        batching_interval = g_sensor1_control->baro_state.batching_interval;
                        sample_interval = g_sensor1_control->baro_state.reporting_interval;
                        reporting_rate = g_sensor1_control->baro_state.reporting_rate;
                        LOC_LOGV("%s: baro data = %f, batching interval = %f, sample interval = %f, reporting_rate = %d",
                                 __FUNCTION__, sensor_samples[0], batching_interval, sample_interval, reporting_rate);

                        if( p_baro_data != NULL)
                        {
                            sp_process_raw_baro_data(sensor_str, sensor_samples[0], timestamp_ms, &p_baro_data);

                            sp_send_baro_data_batch(g_sensor1_control->p_msg_q,
                                                    sensor_str,    NULL,
                                                    &p_baro_data,
                                                    sample_interval,
                                                    batching_interval,
                                                    reporting_rate);
                        }
                    }
                    break;
                default:
                    LOC_LOGE("%s: Unexpected sensor type = %d (%s)", __FUNCTION__, smgr_data->SensorId, sensor_str);
                    break;
                }

                if( p_sensor_data_tlv != NULL)
                {
                    /* Indicate in the sensor batch that the time-source is driven by Modem time. */
                    p_sensor_data_tlv->flags |= QMI_LOC_SENSOR_DATA_FLAG_SENSOR_TIME_IS_MODEM_TIME_V02;
                }

                /* Common handling for Accel, Gyro as well as Accel and Gyro Temperature Data */
                if( p_sensor_data_tlv != NULL ||  p_sensor_temp_tlv != NULL)
                {
                    sp_process_raw_sensor_data(sensor_str, sensor_samples, timestamp_ms, p_sensor_data_tlv, p_sensor_temp_tlv);

                    sp_send_sensor_data_batch(g_sensor1_control->p_msg_q,
                                              sensor_str,
                                              p_gsiff_data_msg,
                                              p_sensor_data_tlv,
                                              p_sensor_temp_tlv,
                                              sample_interval,
                                              batching_interval);
                }

            }
            else
            {
                LOC_LOGE("%s: Bad item quality: %u Sensor: %s", __FUNCTION__, smgr_data->ItemQuality, sensor_str);
            }
        }
    }
    else
    {
        LOC_LOGE("%s: Received invalid indication, msg_id = %d",
                 __FUNCTION__, msg_hdr->msg_id);
    }


}

/*===========================================================================
FUNCTION    process_sam_resp

DESCRIPTION
  Handler for Sensor1 SAM Service Responses.

DEPENDENCIES
   N/A

RETURN VALUE
   N/A

SIDE EFFECTS
   N/A

===========================================================================*/
static void process_sam_resp(sensor1_msg_header_s *msg_hdr, void* msg_ptr)
{
   LOC_LOGD("%s: msg_id = %d", __FUNCTION__, msg_hdr->msg_id );

   switch ( msg_hdr->service_number )
   {
     case SNS_SAM_AMD_SVC_ID_V01:
       if ( msg_hdr->msg_id == SNS_SAM_AMD_ENABLE_RESP_V01)
       {
         sns_sam_qmd_enable_resp_msg_v01* resp_ptr = (sns_sam_qmd_enable_resp_msg_v01*)msg_ptr;

         if ( resp_ptr->resp.sns_result_t != SNS_RESULT_SUCCESS_V01 || resp_ptr->resp.sns_err_t != SENSOR1_SUCCESS )
         {
           LOC_LOGE("%s: Bad Enable response result = %u, error = %u", __FUNCTION__,
                     resp_ptr->resp.sns_result_t, resp_ptr->resp.sns_err_t  );
         }
         else
         {
           LOC_LOGI("%s: Got ENABLE response, status = %d, instance_id = %d",
                     __FUNCTION__, resp_ptr->resp.sns_result_t, resp_ptr->instance_id);
           g_sensor1_control->motion_state.amd_algo_instance_id = resp_ptr->instance_id;
         }
       }
       else if ( msg_hdr->msg_id == SNS_SAM_AMD_DISABLE_RESP_V01)
       {
         sns_sam_qmd_disable_resp_msg_v01* resp_ptr = (sns_sam_qmd_disable_resp_msg_v01*)msg_ptr;

         if ( resp_ptr->resp.sns_result_t != SNS_RESULT_SUCCESS_V01 || resp_ptr->resp.sns_err_t != SENSOR1_SUCCESS )
         {
            LOC_LOGE("%s: Bad Disable response result = %u, error = %u", __FUNCTION__,
                     resp_ptr->resp.sns_result_t, resp_ptr->resp.sns_err_t  );
         }
         else
         {
            LOC_LOGI("%s: Got DISABLE response, status = %d, instance_id = %d",
                     __FUNCTION__, resp_ptr->resp.sns_result_t, resp_ptr->instance_id);
         }
       }
       else
       {
         LOC_LOGE("%s: Received invalid sam response, msg_id = %d",
                 __FUNCTION__, msg_hdr->msg_id);
       }
     break;

     case SNS_SAM_RMD_SVC_ID_V01:
       if ( msg_hdr->msg_id == SNS_SAM_RMD_ENABLE_RESP_V01)
       {
         sns_sam_qmd_enable_resp_msg_v01* resp_ptr = (sns_sam_qmd_enable_resp_msg_v01*)msg_ptr;

         if ( resp_ptr->resp.sns_result_t != SNS_RESULT_SUCCESS_V01 || resp_ptr->resp.sns_err_t != SENSOR1_SUCCESS )
         {
           LOC_LOGE("%s: Bad Enable response result = %u, error = %u", __FUNCTION__,
                     resp_ptr->resp.sns_result_t, resp_ptr->resp.sns_err_t  );
         }
         else
         {
           LOC_LOGI("%s: Got ENABLE response, status = %d, instance_id = %d",
                     __FUNCTION__, resp_ptr->resp.sns_result_t, resp_ptr->instance_id);
           g_sensor1_control->motion_state.rmd_algo_instance_id = resp_ptr->instance_id;
         }
       }
       else if ( msg_hdr->msg_id == SNS_SAM_RMD_DISABLE_RESP_V01)
       {
         sns_sam_qmd_disable_resp_msg_v01* resp_ptr = (sns_sam_qmd_disable_resp_msg_v01*)msg_ptr;

         if ( resp_ptr->resp.sns_result_t != SNS_RESULT_SUCCESS_V01 || resp_ptr->resp.sns_err_t != SENSOR1_SUCCESS )
         {
            LOC_LOGE("%s: Bad Disable response result = %u, error = %u", __FUNCTION__,
                     resp_ptr->resp.sns_result_t, resp_ptr->resp.sns_err_t  );
         }
         else
         {
            LOC_LOGI("%s: Got DISABLE response, status = %d, instance_id = %d",
                     __FUNCTION__, resp_ptr->resp.sns_result_t, resp_ptr->instance_id);
         }
       }
       else
       {
         LOC_LOGE("%s: Received invalid sam response, msg_id = %d",
                 __FUNCTION__, msg_hdr->msg_id);
       }
     break;

     case SNS_SAM_VMD_SVC_ID_V01:
       if ( msg_hdr->msg_id == SNS_SAM_VMD_ENABLE_RESP_V01)
       {
         sns_sam_qmd_enable_resp_msg_v01* resp_ptr = (sns_sam_qmd_enable_resp_msg_v01*)msg_ptr;

         if ( resp_ptr->resp.sns_result_t != SNS_RESULT_SUCCESS_V01 || resp_ptr->resp.sns_err_t != SENSOR1_SUCCESS )
         {
           LOC_LOGE("%s: Bad Enable response result = %u, error = %u", __FUNCTION__,
                     resp_ptr->resp.sns_result_t, resp_ptr->resp.sns_err_t  );
         }
         else
         {
           LOC_LOGI("%s: Got ENABLE response, status = %d, instance_id = %d",
                     __FUNCTION__, resp_ptr->resp.sns_result_t, resp_ptr->instance_id);
           g_sensor1_control->spi_algo_instance_id = resp_ptr->instance_id;
         }
       }
       else if ( msg_hdr->msg_id == SNS_SAM_VMD_DISABLE_RESP_V01)
       {
         sns_sam_qmd_disable_resp_msg_v01* resp_ptr = (sns_sam_qmd_disable_resp_msg_v01*)msg_ptr;

         if ( resp_ptr->resp.sns_result_t != SNS_RESULT_SUCCESS_V01 || resp_ptr->resp.sns_err_t != SENSOR1_SUCCESS )
         {
            LOC_LOGE("%s: Bad Disable response result = %u, error = %u", __FUNCTION__,
                     resp_ptr->resp.sns_result_t, resp_ptr->resp.sns_err_t  );
         }
         else
         {
            LOC_LOGI("%s: Got DISABLE response, status = %d, instance_id = %d",
                     __FUNCTION__, resp_ptr->resp.sns_result_t, resp_ptr->instance_id);
         }
       }
       else
       {
         LOC_LOGE("%s: Received invalid sam response, msg_id = %d",
                 __FUNCTION__, msg_hdr->msg_id);
       }
     break;

     case SNS_SAM_PED_SVC_ID_V01:
       if ( msg_hdr->msg_id == SNS_SAM_PED_ENABLE_RESP_V01)
       {
         sns_sam_ped_enable_resp_msg_v01* resp_ptr = (sns_sam_ped_enable_resp_msg_v01*)msg_ptr;

         if ( resp_ptr->resp.sns_result_t != SNS_RESULT_SUCCESS_V01 || resp_ptr->resp.sns_err_t != SENSOR1_SUCCESS )
         {
           LOC_LOGE("%s: Bad ENABLE response result = %u, error = %u", __FUNCTION__,
                     resp_ptr->resp.sns_result_t, resp_ptr->resp.sns_err_t  );
           g_sensor1_control->pedometer_state.running = false;
         }
         else
         {
           LOC_LOGI("%s: Got ENABLE response, status = %d, instance_id = %d",
                     __FUNCTION__, resp_ptr->resp.sns_result_t, resp_ptr->instance_id);
           gettimeofday(&(g_sensor1_control->pedometer_state.reset_time),NULL);
           g_sensor1_control->pedometer_state.algo_instance_id = resp_ptr->instance_id;

         }
       }
       else if ( msg_hdr->msg_id == SNS_SAM_PED_DISABLE_RESP_V01)
       {
         sns_sam_ped_disable_resp_msg_v01* resp_ptr = (sns_sam_ped_disable_resp_msg_v01*)msg_ptr;

         if ( resp_ptr->resp.sns_result_t != SNS_RESULT_SUCCESS_V01 || resp_ptr->resp.sns_err_t != SENSOR1_SUCCESS )
         {
            LOC_LOGE("%s: Bad DISABLE response result = %u, error = %u", __FUNCTION__,
                     resp_ptr->resp.sns_result_t, resp_ptr->resp.sns_err_t  );
         }
         else
         {
            LOC_LOGI("%s: Got DISABLE response, status = %d, instance_id = %d",
                     __FUNCTION__, resp_ptr->resp.sns_result_t, resp_ptr->instance_id);
         }
       }
       else if ( msg_hdr->msg_id == SNS_SAM_PED_RESET_RESP_V01)
       {
         sns_sam_ped_reset_resp_msg_v01* resp_ptr = (sns_sam_ped_reset_resp_msg_v01*)msg_ptr;

         if ( resp_ptr->resp.sns_result_t != SNS_RESULT_SUCCESS_V01 || resp_ptr->resp.sns_err_t != SENSOR1_SUCCESS )
         {
            LOC_LOGE("%s: Bad RESET response result = %u, error = %u", __FUNCTION__,
                     resp_ptr->resp.sns_result_t, resp_ptr->resp.sns_err_t  );
         }
         else
         {
            LOC_LOGI("%s: Got RESET response, status = %d, instance_id = %d",
                     __FUNCTION__, resp_ptr->resp.sns_result_t, resp_ptr->instance_id);
            gettimeofday(&(g_sensor1_control->pedometer_state.reset_time),NULL);
         }
       }
       else
       {
         LOC_LOGE("%s: Received invalid sam response, msg_id = %d",
                 __FUNCTION__, msg_hdr->msg_id);
       }
     break;

     default:
     break;
   }

   return;
}

/*===========================================================================
FUNCTION    process_amd_and_rmd_ind

DESCRIPTION
  Handler for Sensor1 SAM Service Indications.

DEPENDENCIES
   N/A

RETURN VALUE
   N/A

SIDE EFFECTS
   N/A

===========================================================================*/
static void process_amd_and_rmd_ind(sensor1_msg_header_s *msg_hdr, void* msg_ptr, bool error)
{
  if( NULL == msg_ptr || NULL == msg_hdr)
  {
    LOC_LOGD("%s:msg_hdr(%p) or msg_ptr(%p) is NULL",__FUNCTION__, msg_hdr,msg_ptr );
    return;
  }

  LOC_LOGD("%s: msg_id = %d", __FUNCTION__, msg_hdr->msg_id );

  switch (msg_hdr->msg_id)
  {
    case SNS_SAM_AMD_REPORT_IND_V01:
    {
      sns_sam_qmd_report_ind_msg_v01* ind_ptr = (sns_sam_qmd_report_ind_msg_v01*)msg_ptr;
      uint32_t dsps_time_ms = DSPS_TICK_TO_MSEC(ind_ptr->timestamp);

      if (error)
      {
        g_sensor1_control->motion_state.amd = SNS_SAM_MOTION_UNKNOWN_V01;
        g_sensor1_control->motion_state.rmd = SNS_SAM_MOTION_UNKNOWN_V01;
      }
      else if (msg_hdr->service_number == SNS_SAM_AMD_SVC_ID_V01)
      {
        LOC_LOGI("%s: Received AMD state indication, instance_id = %d, timestamp = %lu, state = %d",
                __FUNCTION__, ind_ptr->instance_id, dsps_time_ms, ind_ptr->state);
        g_sensor1_control->motion_state.amd = ind_ptr->state;
      }
      else if (msg_hdr->service_number == SNS_SAM_RMD_SVC_ID_V01)
      {
        LOC_LOGI("%s: Received RMD state indication, instance_id = %d, timestamp = %lu, state = %d",
                __FUNCTION__, ind_ptr->instance_id, dsps_time_ms, ind_ptr->state);
        g_sensor1_control->motion_state.rmd = ind_ptr->state;
      }

      /* Convert the Sensor Core motion states to QMI_LOC motion states. */
      gsiff_msg motion_data_msg;
      motion_data_msg.msg_type = GSIFF_MOTION_DATA_REQ;
      qmiLocMotionDataStructT_v02* p_motion_data = &motion_data_msg.msg_data.motion_data_req.motion_data;

      if (g_sensor1_control->motion_state.amd == SNS_SAM_MOTION_UNKNOWN_V01 )
      {
        p_motion_data->motion_state = eQMI_LOC_MOTION_STATE_UNKNOWN_V02 ;
        p_motion_data->probability_of_state = 99.9;
      }
      else if (g_sensor1_control->motion_state.amd == SNS_SAM_MOTION_REST_V01 )
      {
        p_motion_data->motion_state = eQMI_LOC_MOTION_STATE_STATIONARY_V02;
        p_motion_data->probability_of_state = 99.9;
      }
      else if (g_sensor1_control->motion_state.rmd == SNS_SAM_MOTION_UNKNOWN_V01 )
      {
        p_motion_data->motion_state = eQMI_LOC_MOTION_STATE_UNKNOWN_V02 ;
        p_motion_data->probability_of_state = 99.9;
      }
      else if (g_sensor1_control->motion_state.rmd == SNS_SAM_MOTION_REST_V01 )
      {
        p_motion_data->motion_state = eQMI_LOC_MOTION_STATE_STATIONARY_V02;
        p_motion_data->probability_of_state = 90.0;
      }
      else if (g_sensor1_control->motion_state.rmd == SNS_SAM_MOTION_MOVE_V01 )
      {
        p_motion_data->motion_state = eQMI_LOC_MOTION_STATE_IN_MOTION_V02;
        p_motion_data->probability_of_state = 99.9;
      }

      /* Send to gsiff_ctl task through message queue */
      p_motion_data->motion_mode = eQMI_LOC_MOTION_MODE_UNKNOWN_V02;
      p_motion_data->age = MOTION_DATA_TLV_AGE;
      p_motion_data->timeout = MOTION_DATA_TLV_TIMEOUT;

      LOC_LOGI("%s: Motion data update sent with state = %d and probability = %f", __FUNCTION__,p_motion_data->motion_state,p_motion_data->probability_of_state);
      bool rv = sp_msg_q_snd(g_sensor1_control->p_msg_q, &motion_data_msg, sizeof(motion_data_msg));
      if ( rv == false )
      {
         LOC_LOGE("%s: Invalid Msg Queue send!", __FUNCTION__);
      }
    }
    break;

    default:
    {
      LOC_LOGE("%s: Received invalid indication, msg_id = %d",
               __FUNCTION__, msg_hdr->msg_id);
    }
    break;
  }
}

/*===========================================================================
FUNCTION    process_vmd_ind

DESCRIPTION
  Handler for Sensor1 SAM Service Indications.

DEPENDENCIES
   N/A

RETURN VALUE
   N/A

SIDE EFFECTS
   N/A

===========================================================================*/
static void process_vmd_ind(sensor1_msg_header_s *msg_hdr, void* msg_ptr)
{
  LOC_LOGD("%s: msg_id = %d", __FUNCTION__, msg_hdr->msg_id );

  switch (msg_hdr->msg_id)
  {
    case SNS_SAM_VMD_REPORT_IND_V01:
    {
      sns_sam_qmd_report_ind_msg_v01* ind_ptr = (sns_sam_qmd_report_ind_msg_v01*)msg_ptr;
      uint32_t dsps_time_ms = DSPS_TICK_TO_MSEC(ind_ptr->timestamp);

      LOC_LOGI("%s: Received SPI indication, instance_id = %d, timestamp = %lu, state = %d",
              __FUNCTION__, ind_ptr->instance_id, dsps_time_ms, ind_ptr->state);

      g_sensor1_control->spi_algo_instance_id = ind_ptr->instance_id;

      /* Only send SPI updates for moving or stationary */
      if( ind_ptr->state == SNS_SAM_MOTION_REST_V01 || ind_ptr->state == SNS_SAM_MOTION_MOVE_V01 )
      {
         uint8_t is_stationary = 0xFF;
         if( ind_ptr->state == SNS_SAM_MOTION_REST_V01 )
         {
            is_stationary = 1;
         }
         else if( ind_ptr->state == SNS_SAM_MOTION_MOVE_V01 )
         {
            is_stationary = 0;
         }

         /* Send to gsiff_ctl task through message queue */
         gsiff_msg spi_data_msg;
         spi_data_msg.msg_type = GSIFF_SPI_STATUS_REQ;
         spi_data_msg.msg_data.spi_status_req.confidenceStationary_valid = false;
         spi_data_msg.msg_data.spi_status_req.stationary = is_stationary;

         LOC_LOGI("%s: SPI update sent with stationary status = %d", __FUNCTION__, spi_data_msg.msg_data.spi_status_req.stationary);
         bool rv = sp_msg_q_snd(g_sensor1_control->p_msg_q, &spi_data_msg, sizeof(spi_data_msg));
         if ( rv == false )
         {
            LOC_LOGE("%s: Invalid Msg Queue send!", __FUNCTION__);
         }
      }
    }
    break;

    default:
    {
      LOC_LOGE("%s: Received invalid indication, msg_id = %d",
               __FUNCTION__, msg_hdr->msg_id);
    }
    break;
  }
}

/*===========================================================================
FUNCTION    process_ped_ind

DESCRIPTION
  Handler for Sensor1 SAM Service Indications.

DEPENDENCIES
   N/A

RETURN VALUE
   N/A

SIDE EFFECTS
   N/A

===========================================================================*/
static void process_ped_ind(sensor1_msg_header_s *msg_hdr, void* msg_ptr)
{
  LOC_LOGD("%s: msg_id = %d", __FUNCTION__, msg_hdr->msg_id );

  switch (msg_hdr->msg_id)
  {
    case SNS_SAM_PED_REPORT_IND_V01:
    {
      sns_sam_ped_report_ind_msg_v01* ind_ptr = (sns_sam_ped_report_ind_msg_v01*)msg_ptr;
      uint32_t dsps_time_ms = DSPS_TICK_TO_MSEC(ind_ptr->timestamp);
      struct timeval current_time;
      struct timeval diff;
      gettimeofday(&current_time,NULL);
      timersub(&current_time,&(g_sensor1_control->pedometer_state.reset_time),&diff);
      uint32_t time_interval = (uint32_t)(diff.tv_sec*1000 + (float)diff.tv_usec/1000);

      LOC_LOGI("%s: Received Pedometer indication, instance_id = %d, timestamp = %lu, time_interval = %lu ms, step_event = %u, step_confidence = %u, step_count = %lu, step_count_error = %ld, step_rate = %4.2f Hz", __FUNCTION__, ind_ptr->instance_id, dsps_time_ms, time_interval,
          ind_ptr->report_data.step_event,
          ind_ptr->report_data.step_confidence,
          ind_ptr->report_data.step_count,
          ind_ptr->report_data.step_count_error,
          ind_ptr->report_data.step_rate);

      /* GSIFF does not support periodic pedometer reports. A report will be
         generated only when a step event is detected. */
      if (ind_ptr->report_data.step_event)
      {
        /* Send the step event message to gsiff_ctl task through message queue */
        gsiff_msg step_event_msg;
        step_event_msg.msg_type = GSIFF_PEDOMETER_REQ;
        step_event_msg.msg_data.pedometer_req.timeSource = eQMI_LOC_SENSOR_TIME_SOURCE_UNSPECIFIED_V02;
        step_event_msg.msg_data.pedometer_req.timestamp = dsps_time_ms;
        step_event_msg.msg_data.pedometer_req.timeInterval = time_interval;
        step_event_msg.msg_data.pedometer_req.stepCount = ind_ptr->report_data.step_count;
        step_event_msg.msg_data.pedometer_req.stepConfidence_valid = true;
        step_event_msg.msg_data.pedometer_req.stepConfidence = ind_ptr->report_data.step_confidence;
        step_event_msg.msg_data.pedometer_req.stepCountUncertainty_valid = true;
        step_event_msg.msg_data.pedometer_req.stepCountUncertainty = (float) ind_ptr->report_data.step_count_error;
        step_event_msg.msg_data.pedometer_req.stepRate_valid = true;
        step_event_msg.msg_data.pedometer_req.stepRate = ind_ptr->report_data.step_rate;

        bool rv = sp_msg_q_snd(g_sensor1_control->p_msg_q, &step_event_msg, sizeof(step_event_msg));
        if ( rv == false )
        {
          LOC_LOGE("%s: Invalid Msg Queue send!", __FUNCTION__);
        }
        else
        {
          LOC_LOGI("%s: Pedometer step event update sent.", __FUNCTION__);
        }
      }
      else
      {
        LOC_LOGI("%s: Pedometer indication ignored.", __FUNCTION__);
      }
    }
    break;

    default:
    {
      LOC_LOGE("%s: Received invalid indication, msg_id = %d",
               __FUNCTION__, msg_hdr->msg_id);
    }
    break;
  }
}

/*===========================================================================
FUNCTION    process_time_resp

DESCRIPTION
  Handler for Sensor1 SAM Service Time Indications.

DEPENDENCIES
   N/A

RETURN VALUE
   N/A

SIDE EFFECTS
   N/A

===========================================================================*/
void process_time_resp( sensor1_msg_header_s *msg_hdr, void *msg_ptr )
{
  bool error = false;

  if( SNS_TIME_TIMESTAMP_RESP_V02 == msg_hdr->msg_id )
  {
    sns_time_timestamp_resp_msg_v02 *time_msg_ptr =
           (sns_time_timestamp_resp_msg_v02*) msg_ptr;

    if( 0 == time_msg_ptr->resp.sns_result_t )
    {
      if( true == time_msg_ptr->timestamp_dsps_valid )
      {
        g_sensor1_control->dsps_clock_ticks = time_msg_ptr->timestamp_dsps;
        LOC_LOGI( "%s: Apps: %llu; DSPS: %u;",
                     __FUNCTION__, time_msg_ptr->timestamp_apps,time_msg_ptr->timestamp_dsps);
      }
      else if( true == time_msg_ptr->error_code_valid )
      {
        LOC_LOGE( "%s: Error in time response: %i", __FUNCTION__, time_msg_ptr->error_code );
        error = true;
      }
      else
      {
        LOC_LOGE( "%s: Unknown erorr in time response", __FUNCTION__ );
        error = true;
      }
    }
    else
    {
       LOC_LOGE( "%s: Received 'Failed' in time response result", __FUNCTION__ );
       error = true;
    }
  }
  else
  {
    error = true;
    LOC_LOGE( "%s: Unhandled message id received: %i",
          __FUNCTION__, msg_hdr->msg_id );
  }

  g_sensor1_control->error = error;
  g_sensor1_control->is_resp_arrived = true;
}

/* ----------------------- END INTERNAL FUNCTIONS ---------------------------------------- */

void sensor1_notify_data_callback(intptr_t cb_data,
                                  sensor1_msg_header_s *msg_hdr,
                                  sensor1_msg_type_e msg_type,
                                  void* msg_ptr)
{
   LOC_LOGD("%s: Message Type = %d", __FUNCTION__, msg_type );

   if ( msg_hdr != NULL )
   {
      LOC_LOGD("%s: Service Num = %d, Message id = %d", __FUNCTION__,
           msg_hdr->service_number, msg_hdr->msg_id );
   }

   switch ( msg_type )
   {
   case SENSOR1_MSG_TYPE_RETRY_OPEN:
      pthread_mutex_lock( &g_access_control->cb_mutex );

      sensor1_open_result = sensor1_open(&p_sensor1_hndl,sensor1_notify_data_callback,(intptr_t)NULL);
      LOC_LOGV("%s: Sensor1 open: %d", __FUNCTION__, sensor1_open_result);

      pthread_cond_signal( &g_access_control->cb_arrived_cond );
      pthread_mutex_unlock( &g_access_control->cb_mutex );

      break;

   case SENSOR1_MSG_TYPE_RESP:
      if( NULL == msg_ptr || NULL == msg_hdr)
      {
         LOC_LOGE("%s: Invalid msg_ptr/msg_hdr for resp", __FUNCTION__);
      }
      else if ( SNS_SMGR_SVC_ID_V01 == msg_hdr->service_number )
      {
         process_smgr_resp(msg_hdr, msg_ptr);
      }
      else if ( SNS_SAM_AMD_SVC_ID_V01 == msg_hdr->service_number ||
                SNS_SAM_RMD_SVC_ID_V01 == msg_hdr->service_number ||
                SNS_SAM_VMD_SVC_ID_V01 == msg_hdr->service_number ||
                SNS_SAM_PED_SVC_ID_V01 == msg_hdr->service_number )
      {
         process_sam_resp(msg_hdr, msg_ptr);
      }
      else if ( SNS_TIME2_SVC_ID_V01 == msg_hdr->service_number )
      {
         pthread_mutex_lock( &g_access_control->cb_mutex );
         process_time_resp( msg_hdr, msg_ptr );
         pthread_cond_signal( &g_access_control->cb_arrived_cond );
         pthread_mutex_unlock( &g_access_control->cb_mutex );
      }
      else
      {
         LOC_LOGE("%s: Unexpected resp service_number = %u", __FUNCTION__, msg_hdr->service_number);
      }
      break;

   case SENSOR1_MSG_TYPE_IND:
      if ( NULL == msg_ptr || NULL == msg_hdr)
      {
         LOC_LOGE("%s: Invalid msg_ptr/msg_hdr for ind", __FUNCTION__);
      }
      else if ( SNS_SMGR_SVC_ID_V01 == msg_hdr->service_number )
      {
         process_smgr_ind(msg_hdr, msg_ptr);
      }
      else if ( SNS_SAM_AMD_SVC_ID_V01 == msg_hdr->service_number ||
                SNS_SAM_RMD_SVC_ID_V01 == msg_hdr->service_number )
      {
         process_amd_and_rmd_ind(msg_hdr, msg_ptr,false);
      }
      else if ( SNS_SAM_VMD_SVC_ID_V01 == msg_hdr->service_number )
      {
         process_vmd_ind(msg_hdr, msg_ptr);
      }
      else if ( SNS_SAM_PED_SVC_ID_V01 == msg_hdr->service_number )
      {
         process_ped_ind(msg_hdr, msg_ptr);
      }
      else
      {
         LOC_LOGE("%s: Unexpected ind service_number = %u", __FUNCTION__, msg_hdr->service_number);
      }
      break;

   case SENSOR1_MSG_TYPE_BROKEN_PIPE:
      LOC_LOGE("%s: Sensor1 Broken Pipe msg type in cb. Closing Sensor1", __FUNCTION__);
      process_amd_and_rmd_ind(msg_hdr, msg_ptr, true);
      sp_sns1_destroy();
      break;

   case SENSOR1_MSG_TYPE_RESP_INT_ERR:
   case SENSOR1_MSG_TYPE_REQ:
   default:
      LOC_LOGE("%s: Invalid msg type in cb = %u", __FUNCTION__, msg_type );
      break;
   }

   /* Free message received as we are done with it and to comply with Sensor1 Documentation */
   if( NULL != msg_ptr )
   {
      sensor1_free_msg_buf(g_sensor1_control->sensor1_client_handle, msg_ptr);
   }
}

/*===========================================================================

  FUNCTION:   sp_sns1_init

  ===========================================================================*/
bool sp_sns1_init(void* p_msg_q)
{
   LOC_LOGI("%s: Initializing Sensor1", __FUNCTION__);

   /* Initialize Sensor1 API */
   if( sp_sns1_init_defaults() == false )
   {
      LOC_LOGE("%s: Unable to initialize with default values!", __FUNCTION__);
      return false;
   }

   if( p_msg_q == NULL )
   {
      LOC_LOGE("%s: Invalid handle to msg queue passed in: %d!", __FUNCTION__, p_msg_q);
      sp_sns1_destroy();
      return false;
   }

   g_sensor1_control->p_msg_q = p_msg_q;

   if (sensor1_open_result == SENSOR1_EWOULDBLOCK)
   {
     LOC_LOGW("sensor process is not up and running yet!");

     pthread_mutex_lock( &g_access_control->cb_mutex );
     pthread_cond_wait( &g_access_control->cb_arrived_cond, &g_access_control->cb_mutex );
     pthread_mutex_unlock( &g_access_control->cb_mutex );
   }

   // Check that sensor1 client was successfully created
   if(sensor1_open_result != SENSOR1_SUCCESS)
   {
      LOC_LOGE("Fatal: Could not open Sensor1 Client sensor1_open = %d!", sensor1_open_result);
      sp_sns1_destroy();
      return false;
   }

   g_sensor1_control->sensor1_client_handle = p_sensor1_hndl;

   return true;
}

/*===========================================================================

  FUNCTION:   sp_sns1_update_accel_status

  ===========================================================================*/
bool sp_sns1_update_accel_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state)
{
   if ( g_sensor1_control == NULL )
   {
      LOC_LOGE("%s: Initialization required to perform operation", __FUNCTION__);
      return false;
   }

   return sp_sns1_update_sensor_status(running, sampling_rate, batching_rate, mounted_state, &g_sensor1_control->accel_state, ACCELEROMETER_SENSOR_TYPE);
}

/*===========================================================================

  FUNCTION:   sp_sns1_update_gyro_status

  ===========================================================================*/
bool sp_sns1_update_gyro_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state)
{
   if( g_sensor1_control == NULL )
   {
      LOC_LOGE("%s: Initialization required to perform operation", __FUNCTION__);
      return false;
   }

   return sp_sns1_update_sensor_status(running, sampling_rate, batching_rate, mounted_state, &g_sensor1_control->gyro_state, GYROSCOPE_SENSOR_TYPE);
}

/*===========================================================================

  FUNCTION:   sp_sns1_update_accel_temperature_status

  ===========================================================================*/
bool sp_sns1_update_accel_temperature_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state)
{
   if ( g_sensor1_control == NULL )
   {
      LOC_LOGE("%s: Initialization required to perform operation", __FUNCTION__);
      return false;
   }

   return sp_sns1_update_sensor_status(running, sampling_rate, batching_rate, mounted_state, &g_sensor1_control->accel_temperature_state, ACCELEROMETER_TEMPERATURE_SENSOR_TYPE);
}

/*===========================================================================

  FUNCTION:   sp_sns1_update_gyro_temperature_status

  ===========================================================================*/
bool sp_sns1_update_gyro_temperature_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state)
{
   if( g_sensor1_control == NULL )
   {
      LOC_LOGE("%s: Initialization required to perform operation", __FUNCTION__);
      return false;
   }

   return sp_sns1_update_sensor_status(running, sampling_rate, batching_rate, mounted_state, &g_sensor1_control->gyro_temperature_state, GYROSCOPE_TEMPERATURE_SENSOR_TYPE);
}

/*===========================================================================

  FUNCTION:   sp_sns1_update_baro_status

  ===========================================================================*/
bool sp_sns1_update_baro_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state)
{
   if ( g_sensor1_control == NULL )
   {
      LOC_LOGE("%s: Initialization required to perform operation", __FUNCTION__);
      return false;
   }

   return sp_sns1_update_sensor_status(running, sampling_rate, batching_rate, mounted_state, &g_sensor1_control->baro_state, BAROMETER_SENSOR_TYPE);
}

/*===========================================================================

  FUNCTION:   sp_sns1_destroy

  ===========================================================================*/
bool sp_sns1_destroy()
{
   if( g_sensor1_control != NULL )
   {
      /* Turn off sensor data reporting if it is still on */
      if ( g_sensor1_control->gyro_state.running )
      {
         if ( sp_sns1_update_gyro_status(false, 0, 0, 0) != true )
         {
            LOC_LOGE("%s: sp_sns1_update_gyro_status() could not stop gyro_data", __FUNCTION__ );
            return false;
         }
      }

      if ( g_sensor1_control->accel_state.running )
      {
         if ( sp_sns1_update_accel_status(false, 0, 0, 0) != true )
         {
            LOC_LOGE("%s: sp_sns1_update_accel_status() could not stop accel_data", __FUNCTION__);
            return false;
         }
      }

      if ( g_sensor1_control->baro_state.running )
      {
         if ( sp_sns1_update_baro_status(false, 0, 0, 0) != true )
         {
            LOC_LOGE("%s: sp_sns1_update_baro_status() could not stop baro_data", __FUNCTION__);
            return false;
         }
      }

      if( g_sensor1_control->sensor1_client_handle != NULL && sensor1_open_result == SENSOR1_SUCCESS)
      {
         sensor1_error_e error = sensor1_close(g_sensor1_control->sensor1_client_handle);
         if ( SENSOR1_SUCCESS != error )
         {
            LOC_LOGE("%s: sensor1_close() error: %u", __FUNCTION__, error);
            return false;
         }
      }

      /* Deallocate memory */
      free(g_sensor1_control);
      g_sensor1_control = NULL;

      return true;
   }
   else
   {
      LOC_LOGE("%s: Initialization required to perform operation", __FUNCTION__);
      return false;
   }
}

/*===========================================================================

  FUNCTION:   sp_sns1_get_sensor_time

  ===========================================================================*/
bool sp_sns1_get_sensor_time(uint32_t* p_time_ms)
{
   uint32_t dsps_clk_ticks = 0;

   if( g_sensor1_control == NULL )
   {
      LOC_LOGE("%s: Initialization required to perform operation", __FUNCTION__);
      return false;
   }

   if( p_time_ms == NULL )
   {
      LOC_LOGE("%s: Invalid pointer passed for reading sensor time.", __FUNCTION__);
      return false;
   }

#ifdef DEBUG_X86
   dsps_clk_ticks = get_sensor_clk_ticks();
#else
   if (sns1_time_req() != true)
   {
      LOC_LOGE("%s: Sensor1 time request failed.", __FUNCTION__);
      return false;
   }
   else
   {
     dsps_clk_ticks = g_sensor1_control->dsps_clock_ticks;
   }
#endif

   *p_time_ms = DSPS_TICK_TO_MSEC(dsps_clk_ticks);
   return true;
}

/*===========================================================================

FUNCTION:  sns1_time_req

DESCRIPTION
  This function requests Sensor Core time ticks from the Sensor1.

DEPENDENCIES
   N/A

RETURN VALUE
   N/A

SIDE EFFECTS
  If a response is received, the Sensor Core time ticks are stored
  in the global variable g_sensor1_control.dsps_clock_ticks.


===========================================================================*/

bool sns1_time_req()
{
  sensor1_error_e                 error;
  sensor1_msg_header_s            msg_hdr;
  sns_reg_single_read_req_msg_v02 *msg_ptr = NULL;

  error = sensor1_alloc_msg_buf( g_sensor1_control->sensor1_client_handle, 0, (void**)&msg_ptr );
  if( SENSOR1_SUCCESS != error )
  {
    LOC_LOGE( "%s: sensor1_alloc_msg_buf returned(get) %d", __FUNCTION__, error );
    return false;
  }

  /* Create the message header */
  msg_hdr.service_number = SNS_TIME2_SVC_ID_V01;
  msg_hdr.msg_id = SNS_TIME_TIMESTAMP_REQ_V02;
  msg_hdr.msg_size = 0;
  msg_hdr.txn_id = 1;

  pthread_mutex_lock( &g_access_control->cb_mutex );

  g_sensor1_control->error = false;
  error = sensor1_write( g_sensor1_control->sensor1_client_handle, &msg_hdr, msg_ptr );
  if( SENSOR1_SUCCESS != error )
  {
    LOC_LOGE( "%s: sensor1_write returned %d", __FUNCTION__, error );
    sensor1_free_msg_buf( g_sensor1_control->sensor1_client_handle, msg_ptr );
    pthread_mutex_unlock( &g_access_control->cb_mutex );
    return false;
  }

  /* waiting for response */
  if (sns1_wait_for_response( TIME_OUT_MS,
                             &g_access_control->cb_mutex,
                             &g_access_control->cb_arrived_cond,
                             &g_sensor1_control->is_resp_arrived ) == false )
  {
    LOC_LOGE( "%s: ERROR: No response from request %d", __FUNCTION__,
         SNS_TIME_TIMESTAMP_REQ_V02 );
    pthread_mutex_unlock( &g_access_control->cb_mutex );
    return false;
  }

  pthread_mutex_unlock( &g_access_control->cb_mutex );

  if( g_sensor1_control->error == true )
  {
    LOC_LOGE( "%s: Error occurred", __FUNCTION__ );
    return false;
  }

  return true;
}

/*===========================================================================

FUNCTION:  sns1_wait_for_response

DESCRIPTION

  Blocks waiting for sensor response, either sensor1 callback to arrvive or timeout

INPUT
  timeout            timeout in ms
  cb_mutex_ptr       pointer to locked mutex
  cond_ptr           pointer to condition variable

INPUT/OUTPUT
  cond_var            boolean predicate.

RETURN
  true if callback arrived, false if timeout

DEPENDENCIES
  Caller needs to lock the cb_mutex_ptr before
  calling this function. Another thread must set cond_var
  to true before signalling the condition variable.
===========================================================================*/
bool sns1_wait_for_response( int timeout,
                       pthread_mutex_t* cb_mutex_ptr,
                       pthread_cond_t*  cond_ptr,
                       bool *cond_var )
{
  bool    ret_val = false;               /* the return value of this function */
  int     rc = 0;                        /* return code from pthread calls */
  struct  timeval    present_time;
  struct  timespec   expire_time;

  LOC_LOGI("%s: timeout=%d", __FUNCTION__, timeout );

  /* special case where callback is issued before the main function
     can wait on cond */
  if (*cond_var == true)
    {
        LOC_LOGI("%s: cb has arrived without waiting", __FUNCTION__ );
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
          LOC_LOGE("%s: pthread_cond_timedwait() rc=%d", __FUNCTION__, rc);
        }
        ret_val = (rc == 0) ? true:false;
      }
    }

  *cond_var = false;

  return ret_val;
}

