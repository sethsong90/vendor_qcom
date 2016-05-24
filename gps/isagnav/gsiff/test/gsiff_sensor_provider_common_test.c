/******************************************************************************
  @file:  gsiff_sensor_provider_common_test.c

  DESCRIPTION
    Unit test for gsiff_sensor_provider_common implementation.

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

#include "gsiff_sensor_provider_common.h"
#include "gsiff_sensor_provider_glue.h"

#include "test.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <stdbool.h>

#include "msg_q.h"

#define GPSONE_TEST_MSG_Q_PATH "/tmp/gpsone_test_q"

static void test_read_sys_time()
{
   TEST(sp_read_sys_time_ms() != 0);
}

static void test_send_mounted_state(void* msg_q)
{
   /* Does not send even if bad msg_q_id. Should be successful */
   TEST(sp_send_msi_mounted_status(NULL, SP_MSI_DO_NOT_SEND) == true);

   TEST(sp_send_msi_mounted_status(NULL, 27) == false);

   TEST(sp_send_msi_mounted_status(NULL, SP_MSI_MOUNTED) == false);

   TEST(sp_send_msi_mounted_status(msg_q, SP_MSI_MOUNTED) == true);
   TEST(sp_send_msi_mounted_status(msg_q, SP_MSI_UNMOUNTED) == true);
   TEST(sp_send_msi_mounted_status(msg_q, SP_MSI_UNKNOWN) == true);
   TEST(sp_send_msi_mounted_status(msg_q, SP_MSI_DO_NOT_SEND) == true);

   /* Bad mounted state provided */
   TEST(sp_send_msi_mounted_status(msg_q, 27) == false);
}

static void test_send_sensor_data_batch(void* msg_q)
{
   const char* sensor_str = "ACCEL_DATA";

   double sample_interval = 1000.0 / 60.0;
   double batching_interval = 1000.0 / 4.0;
   gsiff_msg gsiff_data_msg;
   qmiLoc3AxisSensorSampleListStructT_v02* p_sensor_data_tlv = &gsiff_data_msg.msg_data.sensor_data_samples;

   memset(&gsiff_data_msg, 0, sizeof(gsiff_data_msg));

   /* Test when there are more than the max number of samples - Should clear out tlv. */
   p_sensor_data_tlv->sensorData_len = 51;

   sp_send_sensor_data_batch(msg_q,
                             sensor_str,
                             &gsiff_data_msg,
                             p_sensor_data_tlv,
                             sample_interval,
                             batching_interval);

   TEST(p_sensor_data_tlv->sensorData_len == 0);

   /* Test when there are equal to the max number of samples - Should clear out tlv. */
   p_sensor_data_tlv->sensorData_len = 50;

   sp_send_sensor_data_batch(msg_q,
                             sensor_str,
                             &gsiff_data_msg,
                             p_sensor_data_tlv,
                             sample_interval,
                             batching_interval);

   TEST(p_sensor_data_tlv->sensorData_len == 0);

   /* Test when there are less than the max number of samples - Should not alter the tlv. */
   p_sensor_data_tlv->sensorData_len = 37;

   sp_send_sensor_data_batch(msg_q,
                             sensor_str,
                             &gsiff_data_msg,
                             p_sensor_data_tlv,
                             sample_interval,
                             batching_interval);

   TEST(p_sensor_data_tlv->sensorData_len == 37);

   /* Test when the samples are inside the batching interval */
   p_sensor_data_tlv->timeOfFirstSample = 100;
   p_sensor_data_tlv->sensorData_len = 2;
   p_sensor_data_tlv->sensorData[0].timeOffset = 0;
   p_sensor_data_tlv->sensorData[1].timeOffset = 100;

   sp_send_sensor_data_batch(msg_q,
                             sensor_str,
                             &gsiff_data_msg,
                             p_sensor_data_tlv,
                             sample_interval,
                             batching_interval);

   TEST(p_sensor_data_tlv->sensorData_len == 2);

   /* Test when the samples are outside the batching interval */
   p_sensor_data_tlv->timeOfFirstSample = 100;
   p_sensor_data_tlv->sensorData_len = 3;
   p_sensor_data_tlv->sensorData[0].timeOffset = 0;
   p_sensor_data_tlv->sensorData[1].timeOffset = 100;
   p_sensor_data_tlv->sensorData[2].timeOffset = 250;

   sp_send_sensor_data_batch(msg_q,
                             sensor_str,
                             &gsiff_data_msg,
                             p_sensor_data_tlv,
                             sample_interval,
                             batching_interval);

   TEST(p_sensor_data_tlv->sensorData_len == 0);

}

static void test_calc_time_diff_ms()
{
   /* Test normal case */
   TEST(sp_calc_time_diff_ms(3, 5) == 2);
   TEST(sp_calc_time_diff_ms(50000, 53753) == 3753);

   /* Test rollover cases */
   TEST(sp_calc_time_diff_ms(UINT_MAX, 0) == 1);
   TEST(sp_calc_time_diff_ms(UINT_MAX, 5) == 6);
   TEST(sp_calc_time_diff_ms(UINT_MAX - 25, 5) == 31);
}

int main (int argc, char *argv[])
{
   void* msg_q = NULL;
   msq_q_err_type rv = msg_q_init(&msg_q);
   if( rv != eMSG_Q_SUCCESS )
   {
      fprintf(stderr, "\n\t**** FATAL: Could not initialized message queue. ****\n");
      exit(-1);
   }

   test_read_sys_time();

   test_send_mounted_state(msg_q);

   test_send_sensor_data_batch(msg_q);

   test_calc_time_diff_ms();

   return(0);
}

