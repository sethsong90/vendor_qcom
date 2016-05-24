/******************************************************************************
  @file:  gsiff_sensor_provider_sensor1_test.c

  DESCRIPTION
    Unit test for gsiff_sensor_provider_sensor1 implementation.

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

#include "gsiff_sensor_provider_glue.h"
#include "gsiff_sensor_provider_sensor1.h"

#include "test.h"
#include "sensor1.h"

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "sns_sam_qmd_v01.h"
#include "sns_sam_vmd_v01.h"
#include "sns_smgr_api_v01.h"
#include "msg_q.h"

#define SENSOR1_HANDLE (sensor1_handle_s*)0xDEADBEEF

/* Main callback function for sensor1 data input. */
extern void sensor1_notify_data_callback(intptr_t cb_data,
                                  sensor1_msg_header_s *msg_hdr,
                                  sensor1_msg_type_e msg_type,
                                  void* msg_ptr);

static void test_data_callback_push()
{

   sensor1_msg_header_s msg_hdr;
   sensor1_msg_type_e msg_type;
   void* msg_ptr = NULL;

   msg_hdr.msg_id = 0;
   msg_hdr.msg_size = 0;
   msg_hdr.service_number = 0;
   msg_hdr.txn_id = 0;

   msg_type = SENSOR1_MSG_TYPE_REQ;

   /* Test bad message type - Should only be RESP or IND */
   fprintf(stderr, "\n\t**** Invalid Message Type ***\n");
   sensor1_notify_data_callback((intptr_t)NULL,
                                &msg_hdr,
                                msg_type,
                                msg_ptr);

   msg_hdr.msg_id = 0;
   msg_hdr.msg_size = 0;
   msg_hdr.service_number = 0;
   msg_hdr.txn_id = 0;

   msg_type = SENSOR1_MSG_TYPE_RESP;

   /* Test invalid msg_ptr for response */
   fprintf(stderr, "\n\t**** Invalid msg_ptr for Resp ***\n");
   sensor1_notify_data_callback((intptr_t)NULL,
                                &msg_hdr,
                                msg_type,
                                NULL);

   /* Test invalid msg_hdr for response */
   fprintf(stderr, "\n\t**** Invalid msg_hdr for Resp ***\n");
   sensor1_notify_data_callback((intptr_t)NULL,
                                NULL,
                                msg_type,
                                msg_ptr);

   msg_hdr.msg_id = 0;
   msg_hdr.msg_size = 0;
   msg_hdr.service_number = 0;
   msg_hdr.txn_id = 0;

   msg_type = SENSOR1_MSG_TYPE_IND;

   /* Test invalid msg_hdr for indication */
   fprintf(stderr, "\n\t**** Invalid msg_hdr for Ind ***\n");
   sensor1_notify_data_callback((intptr_t)NULL,
                                NULL,
                                msg_type,
                                msg_ptr);

   /* Test invalid msg_ptr for indication */
   fprintf(stderr, "\n\t**** Invalid msg_ptr for Ind ***\n");
   sensor1_notify_data_callback((intptr_t)NULL,
                                &msg_hdr,
                                msg_type,
                                NULL);

   /* Test VALID responses */

   /* Valid SMGR response */
   fprintf(stderr, "\n\t**** VALID Smgr Resp ***\n");
   msg_hdr.msg_id = SNS_SMGR_REPORT_RESP_V01;
   msg_hdr.msg_size = sizeof(sns_smgr_periodic_report_resp_msg_v01);
   msg_hdr.service_number = SNS_SMGR_SVC_ID_V01;
   msg_hdr.txn_id = 0;

   msg_type = SENSOR1_MSG_TYPE_RESP;

   sns_smgr_periodic_report_resp_msg_v01* smgr_rpt = NULL;
   sensor1_alloc_msg_buf(SENSOR1_HANDLE, sizeof(sns_smgr_periodic_report_resp_msg_v01), (void**)&smgr_rpt);
   smgr_rpt->AckNak = SNS_SMGR_RESPONSE_ACK_SUCCESS_V01;
   smgr_rpt->Resp.sns_result_t = SNS_RESULT_SUCCESS_V01;
   msg_ptr = (void*)smgr_rpt;

   sensor1_notify_data_callback((intptr_t)NULL,
                                &msg_hdr,
                                msg_type,
                                msg_ptr);

   /* Valid Sam ENABLE reporting response */
   fprintf(stderr, "\n\t**** VALID Sam Enable Resp ***\n");
   msg_hdr.msg_id = SNS_SAM_VMD_ENABLE_RESP_V01;
   msg_hdr.msg_size = sizeof(sns_sam_qmd_enable_resp_msg_v01);
   msg_hdr.service_number = SNS_SAM_VMD_SVC_ID_V01;
   msg_hdr.txn_id = 0;

   msg_type = SENSOR1_MSG_TYPE_RESP;

   sns_sam_qmd_enable_resp_msg_v01* sam_enable_rpt = NULL;
   sensor1_alloc_msg_buf(SENSOR1_HANDLE, sizeof(sns_sam_qmd_enable_resp_msg_v01), (void**)&sam_enable_rpt);
   sam_enable_rpt->instance_id = 5;
   sam_enable_rpt->resp.sns_result_t = SNS_RESULT_SUCCESS_V01;
   msg_ptr = (void*)sam_enable_rpt;

   sensor1_notify_data_callback((intptr_t)NULL,
                                &msg_hdr,
                                msg_type,
                                msg_ptr);

   /* Valid Sam DISABLE reporting response */
   fprintf(stderr, "\n\t**** VALID Sam Disable Resp ***\n");
   msg_hdr.msg_id = SNS_SAM_VMD_DISABLE_RESP_V01;
   msg_hdr.msg_size = sizeof(sns_sam_qmd_disable_resp_msg_v01);
   msg_hdr.service_number = SNS_SAM_VMD_SVC_ID_V01;
   msg_hdr.txn_id = 0;

   msg_type = SENSOR1_MSG_TYPE_RESP;

   sns_sam_qmd_disable_resp_msg_v01* sam_disable_rpt = NULL;
   sensor1_alloc_msg_buf(SENSOR1_HANDLE, sizeof(sns_sam_qmd_disable_resp_msg_v01), (void**)&sam_disable_rpt);
   sam_disable_rpt->instance_id = 5;
   sam_disable_rpt->resp.sns_result_t = SNS_RESULT_SUCCESS_V01;
   msg_ptr = (void*)sam_disable_rpt;

   sensor1_notify_data_callback((intptr_t)NULL,
                                &msg_hdr,
                                msg_type,
                                msg_ptr);

   /* Invalid Sam ENABLE reporting response */
   fprintf(stderr, "\n\t**** INVALID Sam Enable Resp ***\n");
   msg_hdr.msg_id = SNS_SAM_VMD_ENABLE_RESP_V01;
   msg_hdr.msg_size = sizeof(sns_sam_qmd_enable_resp_msg_v01);
   msg_hdr.service_number = SNS_SAM_VMD_SVC_ID_V01;
   msg_hdr.txn_id = 0;

   msg_type = SENSOR1_MSG_TYPE_RESP;

   sensor1_alloc_msg_buf(SENSOR1_HANDLE, sizeof(sns_sam_qmd_enable_resp_msg_v01), (void**)&sam_enable_rpt);
   sam_enable_rpt->instance_id = 5;
   sam_enable_rpt->resp.sns_result_t = SNS_RESULT_FAILURE_V01;
   msg_ptr = (void*)sam_enable_rpt;

   sensor1_notify_data_callback((intptr_t)NULL,
                                &msg_hdr,
                                msg_type,
                                msg_ptr);

   /* Invalid Sam DISABLE reporting response */
   fprintf(stderr, "\n\t**** INVALID Sam Disable Resp ***\n");
   msg_hdr.msg_id = SNS_SAM_VMD_DISABLE_RESP_V01;
   msg_hdr.msg_size = sizeof(sns_sam_qmd_disable_resp_msg_v01);
   msg_hdr.service_number = SNS_SAM_VMD_SVC_ID_V01;
   msg_hdr.txn_id = 0;

   msg_type = SENSOR1_MSG_TYPE_RESP;

   sensor1_alloc_msg_buf(SENSOR1_HANDLE, sizeof(sns_sam_qmd_disable_resp_msg_v01), (void**)&sam_disable_rpt);
   sam_disable_rpt->instance_id = 5;
   sam_disable_rpt->resp.sns_result_t = SNS_RESULT_FAILURE_V01;
   msg_ptr = (void*)sam_disable_rpt;

   sensor1_notify_data_callback((intptr_t)NULL,
                                &msg_hdr,
                                msg_type,
                                msg_ptr);

   /* Valid Sam state change indications */
   fprintf(stderr, "\n\t**** VALID Sam State Change Ind ***\n");
   msg_hdr.msg_id = SNS_SAM_VMD_REPORT_IND_V01;
   msg_hdr.msg_size = sizeof(sns_sam_qmd_report_ind_msg_v01);
   msg_hdr.service_number = SNS_SAM_VMD_SVC_ID_V01;
   msg_hdr.txn_id = 0;

   msg_type = SENSOR1_MSG_TYPE_IND;

   sns_sam_qmd_report_ind_msg_v01* sam_rpt_ind;
   sensor1_alloc_msg_buf(SENSOR1_HANDLE, sizeof(sns_sam_qmd_report_ind_msg_v01), (void**)&sam_rpt_ind);
   sam_rpt_ind->instance_id = 5;
   sam_rpt_ind->state = SNS_SAM_MOTION_REST_V01;
   sam_rpt_ind->timestamp = 50;
   msg_ptr = (void*)sam_rpt_ind;

   sensor1_notify_data_callback((intptr_t)NULL,
                                &msg_hdr,
                                msg_type,
                                msg_ptr);

   /* Invalid Sam indication - Unexpected Message */
   fprintf(stderr, "\n\t**** INVALID Sam Ind - Unexpected Message ***\n");
   msg_hdr.msg_id = SNS_SAM_VMD_ERROR_IND_V01;
   msg_hdr.msg_size = sizeof(sns_sam_qmd_error_ind_msg_v01);
   msg_hdr.service_number = SNS_SAM_VMD_SVC_ID_V01;
   msg_hdr.txn_id = 0;

   msg_type = SENSOR1_MSG_TYPE_IND;

   sns_sam_qmd_error_ind_msg_v01* sam_err_ind;
   sensor1_alloc_msg_buf(SENSOR1_HANDLE, sizeof(sns_sam_qmd_error_ind_msg_v01), (void**)&sam_err_ind);
   msg_ptr = (void*)sam_err_ind;

   sensor1_notify_data_callback((intptr_t)NULL,
                                &msg_hdr,
                                msg_type,
                                msg_ptr);

   /* Valid Smgr sensor data indications with 1 sample */
   fprintf(stderr, "\n\t**** VALID Smgr Sensor Data Ind w/ 1 Sample ***\n");
   msg_hdr.msg_id = SNS_SMGR_REPORT_IND_V01;
   msg_hdr.msg_size = sizeof(sns_smgr_periodic_report_ind_msg_v01);
   msg_hdr.service_number = SNS_SMGR_SVC_ID_V01;
   msg_hdr.txn_id = 0;

   msg_type = SENSOR1_MSG_TYPE_IND;

   sns_smgr_periodic_report_ind_msg_v01* smgr_rpt_ind;
   sensor1_alloc_msg_buf(SENSOR1_HANDLE, sizeof(sns_smgr_periodic_report_ind_msg_v01), (void**)&smgr_rpt_ind);
   smgr_rpt_ind->CurrentRate = 60;
   smgr_rpt_ind->Item_len = 1;
   smgr_rpt_ind->Item[0].DataType = SNS_SMGR_DATA_TYPE_PRIMARY_V01;
   SENSOR1_Q16_FROM_FLOAT(smgr_rpt_ind->Item[0].ItemData[0], 1.0);
   SENSOR1_Q16_FROM_FLOAT(smgr_rpt_ind->Item[0].ItemData[1], 2.0);
   SENSOR1_Q16_FROM_FLOAT(smgr_rpt_ind->Item[0].ItemData[2], 3.0);
   smgr_rpt_ind->Item[0].ItemFlags = 0;
   smgr_rpt_ind->Item[0].ItemQuality = SNS_SMGR_ITEM_QUALITY_CURRENT_SAMPLE_V01;
   smgr_rpt_ind->Item[0].ItemSensitivity = 2;
   smgr_rpt_ind->Item[0].SensorId = SNS_SMGR_ID_GYRO_V01;
   smgr_rpt_ind->Item[0].TimeStamp = 35;

   msg_ptr = (void*)smgr_rpt_ind;

   sensor1_notify_data_callback((intptr_t)NULL,
                                &msg_hdr,
                                msg_type,
                                msg_ptr);

   /* Valid Smgr sensor data indications with 3 samples */
   fprintf(stderr, "\n\t**** VALID Smgr Sensor Data Ind w/ 3 Samples ***\n");
   msg_hdr.msg_id = SNS_SMGR_REPORT_IND_V01;
   msg_hdr.msg_size = sizeof(sns_smgr_periodic_report_ind_msg_v01);
   msg_hdr.service_number = SNS_SMGR_SVC_ID_V01;
   msg_hdr.txn_id = 0;

   msg_type = SENSOR1_MSG_TYPE_IND;

   sensor1_alloc_msg_buf(SENSOR1_HANDLE, sizeof(sns_smgr_periodic_report_ind_msg_v01), (void**)&smgr_rpt_ind);
   smgr_rpt_ind->CurrentRate = 60;
   smgr_rpt_ind->Item_len = 3;
   smgr_rpt_ind->Item[0].DataType = SNS_SMGR_DATA_TYPE_PRIMARY_V01;
   SENSOR1_Q16_FROM_FLOAT(smgr_rpt_ind->Item[0].ItemData[0], 1.0);
   SENSOR1_Q16_FROM_FLOAT(smgr_rpt_ind->Item[0].ItemData[1], 2.0);
   SENSOR1_Q16_FROM_FLOAT(smgr_rpt_ind->Item[0].ItemData[2], 3.0);
   smgr_rpt_ind->Item[0].ItemFlags = 0;
   smgr_rpt_ind->Item[0].ItemQuality = SNS_SMGR_ITEM_QUALITY_CURRENT_SAMPLE_V01;
   smgr_rpt_ind->Item[0].ItemSensitivity = 2;
   smgr_rpt_ind->Item[0].SensorId = SNS_SMGR_ID_GYRO_V01;
   smgr_rpt_ind->Item[0].TimeStamp = 5000;

   smgr_rpt_ind->Item[1].DataType = SNS_SMGR_DATA_TYPE_PRIMARY_V01;
   SENSOR1_Q16_FROM_FLOAT(smgr_rpt_ind->Item[1].ItemData[0], 2.0);
   SENSOR1_Q16_FROM_FLOAT(smgr_rpt_ind->Item[1].ItemData[1], 4.0);
   SENSOR1_Q16_FROM_FLOAT(smgr_rpt_ind->Item[1].ItemData[2], 5.0);
   smgr_rpt_ind->Item[1].ItemFlags = 0;
   smgr_rpt_ind->Item[1].ItemQuality = SNS_SMGR_ITEM_QUALITY_CURRENT_SAMPLE_V01;
   smgr_rpt_ind->Item[1].ItemSensitivity = 2;
   smgr_rpt_ind->Item[1].SensorId = SNS_SMGR_ID_ACCEL_V01;
   smgr_rpt_ind->Item[1].TimeStamp = 4950;

   smgr_rpt_ind->Item[2].DataType = SNS_SMGR_DATA_TYPE_PRIMARY_V01;
   SENSOR1_Q16_FROM_FLOAT(smgr_rpt_ind->Item[2].ItemData[0], 3.0);
   SENSOR1_Q16_FROM_FLOAT(smgr_rpt_ind->Item[2].ItemData[1], 6.0);
   SENSOR1_Q16_FROM_FLOAT(smgr_rpt_ind->Item[2].ItemData[2], 9.0);
   smgr_rpt_ind->Item[2].ItemFlags = 0;
   smgr_rpt_ind->Item[2].ItemQuality = SNS_SMGR_ITEM_QUALITY_CURRENT_SAMPLE_V01;
   smgr_rpt_ind->Item[2].ItemSensitivity = 2;
   smgr_rpt_ind->Item[2].SensorId = SNS_SMGR_ID_ACCEL_V01;
   smgr_rpt_ind->Item[2].TimeStamp = 5100;

   msg_ptr = (void*)smgr_rpt_ind;

   sensor1_notify_data_callback((intptr_t)NULL,
                                &msg_hdr,
                                msg_type,
                                msg_ptr);

   /* Invalid Smgr indications - Unexpected Message */
   fprintf(stderr, "\n\t**** INVALID Smgr Ind - Unexpected Message ***\n");
   msg_hdr.msg_id = SNS_SMGR_SENSOR_POWER_STATUS_IND_V01;
   msg_hdr.msg_size = sizeof(sns_smgr_sensor_power_status_ind_msg_v01);
   msg_hdr.service_number = SNS_SMGR_SVC_ID_V01;
   msg_hdr.txn_id = 0;

   msg_type = SENSOR1_MSG_TYPE_IND;

   sns_smgr_sensor_power_status_ind_msg_v01* smgr_pwr_ind;
   sensor1_alloc_msg_buf(SENSOR1_HANDLE, sizeof(sns_smgr_sensor_power_status_ind_msg_v01), (void**)&smgr_pwr_ind);

   msg_ptr = (void*)smgr_pwr_ind;

   sensor1_notify_data_callback((intptr_t)NULL,
                                &msg_hdr,
                                msg_type,
                                msg_ptr);

}

/* All these test cases should fail because sensor provider is not initialized yet */
static void test_sp_interface_pre_init()
{
   uint32_t time_ms;
   TEST(sp_sns1_get_sensor_time(&time_ms) == false);

   TEST(sp_sns1_update_accel_status(true, 60, 4, SP_MSI_UNMOUNTED) == false);
   TEST(sp_sns1_update_gyro_status(true, 60, 4, SP_MSI_UNMOUNTED) == false);

   TEST(sp_sns1_destroy() == false);
}

static void test_sp_interface_init(void* msg_q)
{
   /* Bad message queue id */
   TEST(sp_sns1_init(NULL) == true);

   /* Good message queue */
   TEST(sp_sns1_init(msg_q) == true);
}

static void test_get_sensor_time()
{
   /* Should succeed since we are initialized*/
   uint32_t time_ms;
   TEST(sp_sns1_get_sensor_time(&time_ms) == true);

   /* Should fail due to bad pointer */
   TEST(sp_sns1_get_sensor_time(NULL) == false);
}

static void test_sp_interface_destroy()
{
   /* Destroying after initialized properly */
   TEST(sp_sns1_destroy() == true);

   /* Destroying a 2nd time should fail */
   TEST(sp_sns1_destroy() == false);
}

int main (int argc, char *argv[])
{
   void* msg_q = NULL;
   if( msg_q_init(&msg_q) )
   {
      fprintf(stderr, "%s: Could not initialize msg_q!\n", __FUNCTION__);
      exit(-1);
   }

   test_sp_interface_pre_init();

   test_sp_interface_init(msg_q);

   test_data_callback_push();

   test_get_sensor_time();

   test_sp_interface_destroy();

   msg_q_destroy(&msg_q);

   return(0);
}

