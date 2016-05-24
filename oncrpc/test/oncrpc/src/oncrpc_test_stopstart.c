/******************************************************************************
  @file  oncrpc_test_stopstart
  @brief Linux user-space test to verify the stop / start of oncrpc threads

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
05/27/08   rr       Initial version, based on oncrpc_test

======================================================================*/
/*=====================================================================

                     INCLUDE FILES FOR MODULE

======================================================================*/
#include <stdio.h>
#include <unistd.h>
#include "oncrpc.h"
#include "ping_mdm_rpc.h"
#include "ping_mdm_rpc_rpc.h"

/*=====================================================================
     External declarations
======================================================================*/


/*=====================================================================
      Constants and Macros
======================================================================*/
static char *VersionStr="1.1";
static unsigned int ping_mdm_num_cb_received;
static unsigned int ping_mdm_errcnt;


/*===========================================================================
  FUNCTION  ping_mdm_data_cb_function
===========================================================================*/
/*!
@brief
  ping callback function to be called remotely from the ping server

@return
  N/A

@note
  - Dependencies
    - ONCRPC, PING_MDM
  - Side Effects
*/
/*=========================================================================*/
static boolean ping_mdm_data_cb_function(uint32 *data, uint32 data_size_words, uint32 sum_cb)
{
   uint32 i;
   uint32 sum=0;
   printf("Ping callback received, data_size:%d ",(int)data_size_words);
   ping_mdm_num_cb_received++;
   for( i=0;i<data_size_words;i++ )
   {
      sum=sum^data[i];
   }

   if( sum != sum_cb )
   {
      printf("checksum error, received 0x%08x, expected 0x%08x\n",(int)sum_cb,(int)sum);
      ping_mdm_errcnt++;
   }
   else
   {
      printf("...data verified \n");
   }
   return( sum == sum_cb );
}

/*===========================================================================
  FUNCTION test_cb
===========================================================================*/
/*!
@brief
  test_cb

@return
  0

@note
  - Dependencies
    - ONCRPC
  - Side Effects
*/
/*=========================================================================*/
void test_cb()
{
   printf("Testing callback, registering \n");
     ping_mdm_register_data_cb(
         ping_mdm_data_cb_function,
         10,        /* Num callbacks */
         100,       /* Num words */
         10,         /* Interval ms*/
         1);        /* Num tasks, only on ver 0x00010002*/

   sleep(1);
   printf("Testing callback, unregistering \n");
   ping_mdm_un_register_data_cb(ping_mdm_data_cb_function);
}

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
  int numLoopsStartStop = 3;
  int i;


  printf("Oncrpc Test, version %s\n",VersionStr);
  printf("Usage: oncrpc_test <num iterations>, default = 1 \n\n");

  oncrpc_init();
  oncrpc_task_start();
  ping_mdm_rpccb_app_init();


  switch(argc)
  {
     case 3:
        numLoopsStartStop = atoi(argv[2]);
    case 2:
      nunTestsToRun = atoi(argv[1]);
    break;
    default:
      printf("Using defaults  num calls:%d, num start stop loops :%d\n",
             (unsigned int)nunTestsToRun ,(unsigned int)numLoopsStartStop);
  }


  if( ping_mdm_rpc_null())
  {
    printf("Verified ping_mdm_rpc is available \n");
  }
  else
  {
    printf("Error ping_mdm_rpc is not availabe \n");
    printf("FAIL\n");
    return -1;
  }
  printf("\n\nRunning for %d iterations ...\n",nunTestsToRun);
  printf("ONCRPC TEST STARTED...\n");

  if( nunTestsToRun == 0 )
  {
    while( 1 )
    {
      printf("Launch RPC call, test#%d\n",cnt);
      cnt++;
      ping_mdm_rpc_null();
    }
  }
  else
  {
    for( cnt=0;cnt < nunTestsToRun;cnt++ )
    {
      printf("Launch RPC call, test#%d\n",cnt);
      ping_mdm_rpc_null();
    }
  }

  test_cb();

  printf("ONCRPC Verifying task start/stop \n");
  for(i=0;i<numLoopsStartStop;i++)
  {
     printf(".");
     oncrpc_task_stop();
     oncrpc_task_start();
     test_cb();
  }

  printf("Stopping tasks\n");
  oncrpc_task_stop();

  printf("Restarting tasks\n");
  oncrpc_task_start();
 test_cb();

  printf("Make RPC Call after restarting tasks\n");
  ping_mdm_rpc_null();

  printf("Final task stop \n");
  oncrpc_task_stop();

  printf("ONCRPC TEST STOP START COMPLETE...\n");
  printf("CLOSING CONNECTIONS ... \n");
  oncrpc_deinit();
  printf("PASS\n");
  return(0);
}



