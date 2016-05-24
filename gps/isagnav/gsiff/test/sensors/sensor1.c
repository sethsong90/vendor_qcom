/******************************************************************************
  @file:  sensor1.c

  DESCRIPTION
    Test framework implementation for sensor1 to enable off-target testing.

  INITIALIZATION AND SEQUENCING REQUIREMENTS

  -----------------------------------------------------------------------------
  Copyright (c) 2011 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------

******************************************************************************/

/*=====================================================================
                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why
--------   ---      -------------------------------------------------------
05/01/11   jb       Initial version
08/01/11   jb       1. Misc typedef changes

======================================================================*/

/*============================================================================
  INCLUDE FILES
  ============================================================================*/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include <stdint.h>
#include "log_util.h"

#include "sensor1.h"
#include "sns_sam_qmd_v01.h"
#include "sns_sam_vmd_v01.h"
#include "sns_smgr_api_v01.h"
#include "gpsone_thread_helper.h"

/*============================================================================
  Preprocessor Definitions and Constants
============================================================================*/
#define SENSOR1_HANDLE (sensor1_handle_s*)0xDEADBEEF
#define SAM_ALGO_INSTANCE_ID 0xAB
#define SPECIAL_BYTE '\xAE'
#define DSPS_CLK_RATE 32768

static intptr_t g_cb_data;
static sensor1_notify_data_cb_t g_data_cbf = NULL;
static bool initialized = false;
static bool accel_running = false;
static bool gyro_running = false;
static bool clock_running = false;
static bool spi_running = false;
static uint32_t accel_report_id = 0;
static uint32_t gyro_report_id = 0;
static struct gpsone_thelper gyro_task_helper;
static struct gpsone_thelper accel_task_helper;
static struct gpsone_thelper clock_task_helper;
static struct gpsone_thelper spi_task_helper;
static uint32_t reporting_rate = 0;
static uint32_t dsps_clk_tick = 0;
static uint32_t spi_status = 0;

/*============================================================================
   Type Declarations
============================================================================*/

/*===========================================================================
  Function Declarations and Documentation
  ===========================================================================*/

/*===========================================================================

  FUNCTION:   clock_ctl_task

  ===========================================================================*/
static int
clock_ctl_task(void* context)
{
   const uint32_t CLOCK_TICK_INTERVAL = 100;
   const useconds_t sleep_time = (useconds_t)((1.0 * CLOCK_TICK_INTERVAL / DSPS_CLK_RATE) * 1000000);

   if( clock_running )
   {
      /* Mimic the DSPS clock by incrementing clock ticks on a schedule */
      dsps_clk_tick += CLOCK_TICK_INTERVAL;
      usleep(sleep_time);
   }

   return 0;
}

/*===========================================================================

  FUNCTION:   get_sensor_clk_ticks

  ===========================================================================*/
uint32_t
get_sensor_clk_ticks()
{
   return dsps_clk_tick;
}

/*===========================================================================

  FUNCTION:   sensor1_open

  ===========================================================================*/
sensor1_error_e
sensor1_open( sensor1_handle_s **hndl,
              sensor1_notify_data_cb_t data_cbf,
              intptr_t cb_data )
{
   if( !initialized )
   {
      return SENSOR1_ENOTALLOWED;
   }

   if( NULL == data_cbf )
   {
      return SENSOR1_EBAD_PTR;
   }

   if( NULL == hndl )
   {
      return SENSOR1_EBAD_PTR;
   }

   *hndl = SENSOR1_HANDLE;
   g_cb_data = cb_data;
   g_data_cbf = data_cbf;

   if( !clock_running )
   {
      dsps_clk_tick = 0;
      clock_running = true;

      /* Create task to mimic dsps clock */
      gpsone_launch_thelper(&clock_task_helper,
                            NULL,           /* Initialize func */
                            NULL,           /* Pre-Process func */
                            clock_ctl_task, /* Process func */
                            NULL,           /* Post-Process func */
                            NULL);
   }

   return SENSOR1_SUCCESS;
}

/*===========================================================================

  FUNCTION:   sensor1_attach

  ===========================================================================*/
sensor1_error_e
sensor1_attach( sensor1_handle_s *hndl,
                sensor1_notify_data_cb_t data_cbf,
                intptr_t cb_data )
{
   if( !initialized )
   {
      return SENSOR1_ENOTALLOWED;
   }

   if( SENSOR1_HANDLE != hndl )
   {
      return SENSOR1_EINVALID_CLIENT;
   }

   if( NULL == data_cbf )
   {
      return SENSOR1_EBAD_PTR;
   }

   /* Not implemented for testing*/
   return SENSOR1_EFAILED;
}

/*===========================================================================

  FUNCTION:   sensor1_close

  ===========================================================================*/
sensor1_error_e
sensor1_close( sensor1_handle_s *hndl )
{
   if( !initialized )
   {
      return SENSOR1_ENOTALLOWED;
   }

   if( SENSOR1_HANDLE != hndl )
   {
      return SENSOR1_EINVALID_CLIENT;
   }

   /* Stop DSPS clock thread*/
   if( clock_running )
   {
      clock_running = false;
      gpsone_unblock_thelper(&clock_task_helper);
   }

   /* Stop data threads */
   if( gyro_running )
   {
      gyro_running = false;
      gpsone_unblock_thelper(&gyro_task_helper);
   }

   if( accel_running )
   {
      accel_running = false;
      gpsone_unblock_thelper(&accel_task_helper);
   }

   if( spi_running )
   {
      spi_running = false;
      gpsone_unblock_thelper(&spi_task_helper);
   }

   memset(&g_cb_data, 0, sizeof(g_cb_data));
   g_data_cbf = NULL;

   return SENSOR1_SUCCESS;
}

/*===========================================================================

  FUNCTION:   sensor1_detach

  ===========================================================================*/
sensor1_error_e
sensor1_detach( sensor1_handle_s *hndl )
{
   if( !initialized )
   {
      return SENSOR1_ENOTALLOWED;
   }

   if( SENSOR1_HANDLE != hndl )
   {
      return SENSOR1_EINVALID_CLIENT;
   }

   /* Not implemented for testing*/
   return SENSOR1_EFAILED;
}

/*===========================================================================

  FUNCTION:   spi_ctl_task

  ===========================================================================*/
static int
spi_ctl_task(void* context)
{
   sensor1_msg_header_s msg_hdr;

   if( spi_running )
   {
      if( g_data_cbf != NULL )
      {
         msg_hdr.msg_id = SNS_SAM_VMD_REPORT_IND_V01;
         msg_hdr.msg_size = sizeof(sns_sam_qmd_report_ind_msg_v01);
         msg_hdr.service_number = SNS_SAM_VMD_SVC_ID_V01;
         msg_hdr.txn_id = 0;

         sns_sam_qmd_report_ind_msg_v01* sam_ind = NULL;
         sensor1_error_e error = sensor1_alloc_msg_buf( SENSOR1_HANDLE,
                                                        sizeof(sns_sam_qmd_report_ind_msg_v01),
                                                        (void**)&sam_ind );

         if ( SENSOR1_SUCCESS != error )
         {
            LOC_LOGE("%s: sensor1_alloc_msg_buf add returned %d", __FUNCTION__, error);
            return -1;
         }

         /* Fake spi data - Loop through 3 states */
         spi_status = (spi_status + 1) % 3;
         sam_ind->instance_id = SAM_ALGO_INSTANCE_ID;
         sam_ind->state = (sns_sam_motion_state_e_v01)spi_status;
         sam_ind->timestamp = get_sensor_clk_ticks();

         g_data_cbf(g_cb_data,
                    &msg_hdr,
                    SENSOR1_MSG_TYPE_IND,
                    sam_ind);

         /* Sleep interval based on sampling rate - every 30 seconds */
         usleep((useconds_t)((30) * 1000000));
      }
   }

   return 0;
}

/*===========================================================================

  FUNCTION:   gyro_ctl_task

  ===========================================================================*/
static int
gyro_ctl_task(void* context)
{
   sensor1_msg_header_s msg_hdr;

   if( gyro_running )
   {
      if( g_data_cbf != NULL )
      {
         msg_hdr.msg_id = SNS_SMGR_REPORT_IND_V01;
         msg_hdr.msg_size = sizeof(sns_smgr_periodic_report_ind_msg_v01);
         msg_hdr.service_number = SNS_SMGR_SVC_ID_V01;
         msg_hdr.txn_id = 0;

         sns_smgr_periodic_report_ind_msg_v01* smgr_ind = NULL;
         sensor1_error_e error = sensor1_alloc_msg_buf( SENSOR1_HANDLE,
                                                        sizeof(sns_smgr_periodic_report_ind_msg_v01),
                                                        (void**)&smgr_ind );

         if ( SENSOR1_SUCCESS != error )
         {
            LOC_LOGE("%s: sensor1_alloc_msg_buf add returned %d", __FUNCTION__, error);
            return -1;
         }

         /* Fake sensor data */
         smgr_ind->CurrentRate = reporting_rate;
         smgr_ind->Item_len = 1;
         smgr_ind->ReportId = gyro_report_id;
         smgr_ind->status = SNS_SMGR_REPORT_OK_V01;
         smgr_ind->Item[0].SensorId = SNS_SMGR_ID_GYRO_V01;
         smgr_ind->Item[0].DataType = SNS_SMGR_DATA_TYPE_PRIMARY_V01;
         smgr_ind->Item[0].ItemQuality = SNS_SMGR_ITEM_QUALITY_CURRENT_SAMPLE_V01;
         smgr_ind->Item[0].TimeStamp = dsps_clk_tick;
         SENSOR1_Q16_FROM_FLOAT(smgr_ind->Item[0].ItemData[0], 1.0f);
         SENSOR1_Q16_FROM_FLOAT(smgr_ind->Item[0].ItemData[1], 2.0f);
         SENSOR1_Q16_FROM_FLOAT(smgr_ind->Item[0].ItemData[2], 3.0f);

         g_data_cbf(g_cb_data,
                    &msg_hdr,
                    SENSOR1_MSG_TYPE_IND,
                    smgr_ind);

         /* Sleep interval based on sampling rate */
         usleep((useconds_t)((1.0 / reporting_rate) * 1000000));
      }

   }

   return 0;
}

/*===========================================================================

  FUNCTION:   accel_ctl_task

  ===========================================================================*/
static int
accel_ctl_task(void* context)
{
   sensor1_msg_header_s msg_hdr;

   if( accel_running )
   {
      if( g_data_cbf != NULL )
      {
         msg_hdr.msg_id = SNS_SMGR_REPORT_IND_V01;
         msg_hdr.msg_size = sizeof(sns_smgr_periodic_report_ind_msg_v01);
         msg_hdr.service_number = SNS_SMGR_SVC_ID_V01;
         msg_hdr.txn_id = 0;

         sns_smgr_periodic_report_ind_msg_v01* smgr_ind = NULL;
         sensor1_error_e error = sensor1_alloc_msg_buf( SENSOR1_HANDLE,
                                                        sizeof(sns_smgr_periodic_report_ind_msg_v01),
                                                        (void**)&smgr_ind );

         if ( SENSOR1_SUCCESS != error )
         {
            LOC_LOGE("%s: sensor1_alloc_msg_buf add returned %d", __FUNCTION__, error);
            return -1;
         }

         /* Fake sensor data */
         smgr_ind->CurrentRate = reporting_rate;
         smgr_ind->Item_len = 1;
         smgr_ind->ReportId = accel_report_id;
         smgr_ind->status = SNS_SMGR_REPORT_OK_V01;
         smgr_ind->Item[0].SensorId = SNS_SMGR_ID_ACCEL_V01;
         smgr_ind->Item[0].DataType = SNS_SMGR_DATA_TYPE_PRIMARY_V01;
         smgr_ind->Item[0].ItemQuality = SNS_SMGR_ITEM_QUALITY_CURRENT_SAMPLE_V01;
         smgr_ind->Item[0].TimeStamp = dsps_clk_tick;
         SENSOR1_Q16_FROM_FLOAT(smgr_ind->Item[0].ItemData[0], 2.0f);
         SENSOR1_Q16_FROM_FLOAT(smgr_ind->Item[0].ItemData[1], 4.0f);
         SENSOR1_Q16_FROM_FLOAT(smgr_ind->Item[0].ItemData[2], 6.0f);

         g_data_cbf(g_cb_data,
                    &msg_hdr,
                    SENSOR1_MSG_TYPE_IND,
                    smgr_ind);

         /* Sleep interval based on sampling rate */
         usleep((useconds_t)((1.0 / reporting_rate) * 1000000));
      }

   }

   return 0;
}

/*===========================================================================

  FUNCTION:   sensor1_write

  ===========================================================================*/
sensor1_error_e
sensor1_write( sensor1_handle_s     *hndl,
               sensor1_msg_header_s *msg_hdr,
               void                 *msg_ptr )
{
   if( !initialized )
   {
      return SENSOR1_ENOTALLOWED;
   }

   if( SENSOR1_HANDLE != hndl )
   {
      return SENSOR1_EINVALID_CLIENT;
   }

   if( NULL == msg_ptr )
   {
      return SENSOR1_EBAD_PTR;
   }

   if( NULL == msg_hdr )
   {
      return SENSOR1_EBAD_PTR;
   }

   /* Check that buffer was allocated using Sensor1 functions */
   if(*(((char*)msg_ptr)-1) != SPECIAL_BYTE)
   {
      return SENSOR1_EBUFFER;
   }

   /* TODO: Read messages and start up "canned" data threads. */
   if( SNS_SMGR_SVC_ID_V01 == msg_hdr->service_number )
   {
      if( SNS_SMGR_REPORT_REQ_V01 == msg_hdr->msg_id && msg_hdr->msg_size == sizeof(sns_smgr_periodic_report_req_msg_v01) )
      {
         sns_smgr_periodic_report_req_msg_v01* smgr_req = (sns_smgr_periodic_report_req_msg_v01*) msg_ptr;

         /* Start Sensor Data */
         if( smgr_req->Action == SNS_SMGR_REPORT_ACTION_ADD_V01 )
         {
            uint32_t i = 0;
            for( i = 0; i < smgr_req->Item_len; ++i )
            {
               sns_smgr_periodic_report_item_s_v01* smgr_item = &smgr_req->Item[i];

               /* Start Gyro Data */
               if( smgr_item->SensorId == SNS_SMGR_ID_GYRO_V01 && !gyro_running )
               {
                  LOC_LOGI("%s: Turning on gyro reporting.", __FUNCTION__);
                  gyro_running = true;
                  reporting_rate = smgr_req->ReportRate;
                  gyro_report_id = smgr_req->ReportId;

                  /* Create task to produce gyro data */
                  gpsone_launch_thelper(&gyro_task_helper,
                                        NULL,           /* Initialize func */
                                        NULL,           /* Pre-Process func */
                                        gyro_ctl_task, /* Process func */
                                        NULL,           /* Post-Process func */
                                        NULL);
               }
               /* Start Accel Data */
               else if( smgr_item->SensorId == SNS_SMGR_ID_ACCEL_V01 && !accel_running )
               {
                  LOC_LOGI("%s: Turning on accel reporting.", __FUNCTION__);
                  accel_running = true;
                  reporting_rate = smgr_req->ReportRate;
                  accel_report_id = smgr_req->ReportId;

                  /* Create task to produce accel data */
                  gpsone_launch_thelper(&accel_task_helper,
                                        NULL,           /* Initialize func */
                                        NULL,           /* Pre-Process func */
                                        accel_ctl_task, /* Process func */
                                        NULL,           /* Post-Process func */
                                        NULL);
               }

            }

         }
         /* Stop Sensor Data */
         else if( smgr_req->Action == SNS_SMGR_REPORT_ACTION_DELETE_V01 )
         {
            if ( smgr_req->ReportId == accel_report_id && accel_running )
            {
               LOC_LOGI("%s: Turning off accel reporting.", __FUNCTION__);
               /* Stop Canned Accel Data thread*/
               accel_running = FALSE;
               gpsone_unblock_thelper(&accel_task_helper);
            }
            else if ( smgr_req->ReportId == gyro_report_id && gyro_running )
            {
               LOC_LOGI("%s: Turning off gyro reporting.", __FUNCTION__);
               /* Stop Canned Gyro Data thread*/
               gyro_running = false;
               gpsone_unblock_thelper(&gyro_task_helper);
            }
         }
      }
   }
   /* Stationary position indicator updates */
   else if( SNS_SAM_VMD_SVC_ID_V01 == msg_hdr->service_number )
   {
      if( SNS_SAM_VMD_ENABLE_REQ_V01 == msg_hdr->msg_id && msg_hdr->msg_size == sizeof(sns_sam_qmd_enable_req_msg_v01) )
      {
         LOC_LOGI("%s: Turning on spi reporting.", __FUNCTION__);
         spi_running = true;

         /* Create task to produce spi updates */
         gpsone_launch_thelper(&spi_task_helper,
                               NULL,           /* Initialize func */
                               NULL,           /* Pre-Process func */
                               spi_ctl_task, /* Process func */
                               NULL,           /* Post-Process func */
                               NULL);

         /* Push a response through callback so they can stop the spi thread later. */
         sensor1_msg_header_s msg_hdr;
         if ( g_data_cbf != NULL )
         {
            msg_hdr.msg_id = SNS_SAM_VMD_ENABLE_RESP_V01;
            msg_hdr.msg_size = sizeof(sns_sam_qmd_enable_resp_msg_v01);
            msg_hdr.service_number = SNS_SAM_VMD_SVC_ID_V01;
            msg_hdr.txn_id = 0;

            sns_sam_qmd_enable_resp_msg_v01* sam_resp = NULL;
            sensor1_error_e error = sensor1_alloc_msg_buf( SENSOR1_HANDLE,
                                                           sizeof(sns_sam_qmd_enable_resp_msg_v01),
                                                           (void**)&sam_resp );

            if ( SENSOR1_SUCCESS != error )
            {
               LOC_LOGE("%s: sensor1_alloc_msg_buf add returned %d", __FUNCTION__, error);
               return SENSOR1_EFAILED;
            }

            /* Fake spi data - Loop through 3 states */
            sam_resp->instance_id = SAM_ALGO_INSTANCE_ID;
            sam_resp->resp.sns_result_t = SNS_RESULT_SUCCESS_V01;

            g_data_cbf(g_cb_data,
                       &msg_hdr,
                       SENSOR1_MSG_TYPE_RESP,
                       sam_resp);
         }
      }
      else if( SNS_SAM_VMD_DISABLE_REQ_V01 == msg_hdr->msg_id && msg_hdr->msg_size == sizeof(sns_sam_qmd_disable_req_msg_v01) )
      {
         LOC_LOGI("%s: Turning off spi reporting.", __FUNCTION__);
         sns_sam_qmd_disable_req_msg_v01* sam_req = (sns_sam_qmd_disable_req_msg_v01*) msg_ptr;

         if( sam_req->instance_id == SAM_ALGO_INSTANCE_ID )
         {
            /* Stop Canned SPI Data thread*/
            spi_running = false;
            gpsone_unblock_thelper(&spi_task_helper);


            /* Push a response through callback so they can stop the spi thread later. */
            sensor1_msg_header_s msg_hdr;
            if ( g_data_cbf != NULL )
            {
               msg_hdr.msg_id = SNS_SAM_VMD_DISABLE_RESP_V01;
               msg_hdr.msg_size = sizeof(sns_sam_qmd_disable_resp_msg_v01);
               msg_hdr.service_number = SNS_SAM_VMD_SVC_ID_V01;
               msg_hdr.txn_id = 0;

               sns_sam_qmd_disable_resp_msg_v01* sam_resp = NULL;
               sensor1_error_e error = sensor1_alloc_msg_buf( SENSOR1_HANDLE,
                                                              sizeof(sns_sam_qmd_disable_resp_msg_v01),
                                                              (void**)&sam_resp );

               if ( SENSOR1_SUCCESS != error )
               {
                  LOC_LOGE("%s: sensor1_alloc_msg_buf add returned %d", __FUNCTION__, error);
                  return SENSOR1_EFAILED;
               }

               /* Fake spi data - Loop through 3 states */
               sam_resp->instance_id = SAM_ALGO_INSTANCE_ID;
               sam_resp->resp.sns_result_t = SNS_RESULT_SUCCESS_V01;

               g_data_cbf(g_cb_data,
                          &msg_hdr,
                          SENSOR1_MSG_TYPE_RESP,
                          sam_resp);
            }

         }
         else
         {
            LOC_LOGE("%s: Bad instance algorithm id passed to sensor1 = %u.", __FUNCTION__, sam_req->instance_id);
         }
      }

   }

   if( NULL != msg_ptr )
   {
       sensor1_free_msg_buf( hndl, msg_ptr );
   }

   return SENSOR1_SUCCESS;
}

/*===========================================================================

  FUNCTION:   sensor1_writable

  ===========================================================================*/
sensor1_error_e
sensor1_writable( sensor1_handle_s  *hndl,
                  sensor1_write_cb_t cbf,
                  intptr_t           cb_data,
                  uint32_t           service_number )
{
   if( !initialized )
   {
      return SENSOR1_ENOTALLOWED;
   }

   if( SENSOR1_HANDLE != hndl )
   {
      return SENSOR1_EINVALID_CLIENT;
   }

   if( NULL != cbf )
   {
      return SENSOR1_EBAD_PTR;
   }

   /* Not implemented for testing*/
   return SENSOR1_EFAILED;
}

/*===========================================================================

  FUNCTION:   sensor1_alloc_msg_buf

  ===========================================================================*/
sensor1_error_e
sensor1_alloc_msg_buf(sensor1_handle_s *hndl,
                      uint16_t          size,
                      void            **buffer )
{
   if( !initialized )
   {
      return SENSOR1_ENOTALLOWED;
   }

   if( SENSOR1_HANDLE != hndl )
   {
      return SENSOR1_EINVALID_CLIENT;
   }

   if( 0 == size )
   {
      return SENSOR1_ENOMEM;
   }

   if( NULL == buffer )
   {
      return SENSOR1_EBAD_PTR;
   }

   char* buf = (char*)calloc(1, sizeof(char) * (size + 1));
   if( NULL == buf )
   {
      return SENSOR1_ENOMEM;
   }

   buf[0] = SPECIAL_BYTE;
   *buffer = buf + 1;

   return SENSOR1_SUCCESS;
}

/*===========================================================================

  FUNCTION:   sensor1_free_msg_buf

  ===========================================================================*/
sensor1_error_e
sensor1_free_msg_buf(sensor1_handle_s *hndl,
                     void             *msg_buf )
{
   if( !initialized )
   {
      return SENSOR1_ENOTALLOWED;
   }

   if( SENSOR1_HANDLE != hndl )
   {
      return SENSOR1_EINVALID_CLIENT;
   }

   if( NULL == msg_buf )
   {
      return SENSOR1_EBUFFER;
   }

   char* buf = (char*)msg_buf;
   if( buf[-1] != SPECIAL_BYTE)
   {
      return SENSOR1_EBUFFER;
   }

   free(buf-1);

   return SENSOR1_SUCCESS;
}

/*===========================================================================

  FUNCTION:   sensor1_init

  ===========================================================================*/
sensor1_error_e
sensor1_init( void )
{
   initialized = true;
   accel_running = false;
   gyro_running = false;

   return SENSOR1_SUCCESS;
}

