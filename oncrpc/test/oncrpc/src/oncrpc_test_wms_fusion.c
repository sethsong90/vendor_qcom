/******************************************************************************
  @file  oncrpc_test_wms_fusion
  @brief Linux user-space wms test to verify wms fusion

  DESCRIPTION
  Oncrpc test program for Linux user-space .

  -----------------------------------------------------------------------------
  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------
******************************************************************************/
/*=====================================================================
                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when         who      what, where, why
--------     ---      -------------------------------------------------------
04/09/2010   rr       Initial version, based on oncrpc_test


======================================================================*/
/*=====================================================================

                     INCLUDE FILES FOR MODULE

======================================================================*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "oncrpc.h"
#include "wms_fusion.h"
#include "wms_fusion_rpc.h"

/*=========================================================================*/
int main(int argc, char *argv[])
{
   int api_not_found_error = 0;
   oncrpc_init();
   oncrpc_task_start();
   wms_fusioncb_app_init();

   printf("ONCRPC_TEST_WMS_FUSION STARTED \n");
   printf("calling wms_fusion_null()\n");
   if(wms_fusion_null()) {
      printf("WMS_FUSION Found\n");
   } else {
      printf("ERROR WMS_FUSION Not Found\n");
      api_not_found_error++;
   }
   oncrpc_deinit();
   if(api_not_found_error == 0) {
      printf("PASS\n");
   } else {
      printf("FAIL\n");
   }
   return(-api_not_found_error);
}
