/******************************************************************************
  @file  oncrpc_test_cm_fusion
  @brief Linux user-space cm test to verify cm fusion

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
#include "cm_fusion.h"
#include "cm_fusion_rpc.h"

/*=========================================================================*/
int main(int argc, char *argv[])
{
   int api_not_found_error = 0;
   oncrpc_init();
   oncrpc_task_start();
   cm_fusioncb_app_init();

   printf("ONCRPC_TEST_CM__FUSION STARTED \n");
   printf("calling cm_fusion_null()\n");
   if(cm_fusion_null()) {
      printf("CM_FUSION Found\n");
   } else {
      printf("ERROR CM_FUSION Not Found\n");
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
