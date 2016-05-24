/******************************************************************************
  @file:  gsiff_sensor_provider_android_test.c

  DESCRIPTION
    Unit test for gsiff_sensor_provider_android implementation.

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
#include "gsiff_sensor_provider_and_hal.h"

#include "test.h"

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "msg_q.h"

/* All these test cases should fail because sensor provider is not initialized yet */
static void test_sp_interface_pre_init()
{
   uint32_t time_ms;
   TEST(sp_and_hal_get_sensor_time(&time_ms) == false);

   TEST(sp_and_hal_update_accel_status(true, 60, 4, SP_MSI_UNMOUNTED) == false);
   TEST(sp_and_hal_update_gyro_status(true, 60, 4, SP_MSI_UNMOUNTED) == false);

   TEST(sp_and_hal_destroy() == false);
}

static void test_sp_interface_init(void* msg_q)
{
   /* Bad message queue id */
   TEST(sp_and_hal_init(NULL) == false);

   /* Good message queue - Does not work off-target so this fails currently */
   TEST(sp_and_hal_init(msg_q) == true);
}

static void test_get_sensor_time()
{
   /* Should succeed since we are initialized*/
   uint32_t time_ms;
   TEST(sp_and_hal_get_sensor_time(&time_ms) == true);

   /* Should fail due to bad pointer */
   TEST(sp_and_hal_get_sensor_time(NULL) == false);
}

static void test_sp_interface_destroy()
{
   /* Destroying after initialized properly */
   TEST(sp_and_hal_destroy() == true);

   /* Destroying a 2nd time should fail */
   TEST(sp_and_hal_destroy() == false);
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

   test_get_sensor_time();

   test_sp_interface_destroy();

   msg_q_destroy(&msg_q);

   return(0);
}

