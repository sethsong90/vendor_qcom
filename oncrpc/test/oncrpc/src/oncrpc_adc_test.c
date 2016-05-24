/******************************************************************************
  @file  oncrpc_adc_test
  @brief Linux user-space simple sanity test for adc

  DESCRIPTION
  Oncrpc test program for Linux user-space .

  -----------------------------------------------------------------------------
  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------

******************************************************************************/
/*=====================================================================
                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why
--------   ---      -------------------------------------------------------
05/22/08   rr       ADC Test Initial


======================================================================*/
/*=====================================================================

                     INCLUDE FILES FOR MODULE

======================================================================*/
#include <stdio.h>
#include <unistd.h>
#include "oncrpc.h"
#include "adc.h"
#include "adc_rpc.h"

/*=====================================================================
     External declarations
======================================================================*/


/*=====================================================================
      Constants and Macros
======================================================================*/
static char *VersionStr="1.0";


/*===========================================================================
  FUNCTION  main
===========================================================================*/
/*!
@brief
  entry to test

@return
  0

@note
  - Dependencies
    - ONCRPC
  - Side Effects
*/
/*=========================================================================*/
int main(int argc, char *argv[])
{
  int cnt=0;
  int nunTestsToRun = 1;

  int  adc_value;//ADC_RETURN_TYPE

  printf("Oncrpc Test, version %s\n",VersionStr);
  printf("Usage: oncrpc_adc_test <num iterations>, default = 1 \n\n");

  oncrpc_init();
  oncrpc_task_start();
  if( argc == 1 )
  {
      nunTestsToRun = 1;
  }
  else
  {
    nunTestsToRun = atoi(argv[1]);
  }


  if( adc_null())
  {
    printf("Verified adc_rpc is available \n");
  }
  else
  {
    printf("Error adc_rpc is not availabe \n");
    printf("FAIL\n");
    return -1;
  }
  printf("\n\nRunning for %d iterations ...\n",nunTestsToRun);
  printf("ONCRPC ADC TEST STARTED...\n");

  if( nunTestsToRun == 0 )
  {
    while( 1 )
    {
      cnt++;
      adc_value = adc_read(0);
      printf("ADC Value %d \n",adc_value);
    }
  }
  else
  {
    for( cnt=0;cnt < nunTestsToRun;cnt++ )
    {
      adc_value = adc_read(0);
      printf("ADC Value %d \n",adc_value);
    }
  }

  adc_value = adc_read(11);
  printf("ADC value for vbatt %d \n", adc_value);
  adc_value = adc_read(12);
  printf("ADC value for vchgr %d \n", adc_value);
  adc_value = adc_read(18);
  printf("ADC value for pmic_therm %d \n", adc_value);
  adc_value = adc_read(23);
  printf("ADC value for msm_therm %d \n", adc_value);

  printf("ONCRPC TEST COMPLETE...\n");
  printf("CLOSING CONNECTIONS ... \n");
  oncrpc_task_stop();
  oncrpc_deinit();
  printf("PASS\n");
  return(0);
}


