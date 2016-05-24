/******************************************************************************
  @file  oncrpc_init_test
  @brief Linux user-space test to verify oncrpc library initialization

  DESCRIPTION
  Oncrpc test program for Linux user-space .

  -----------------------------------------------------------------------------
  Copyright (c) 2009-2010 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------

******************************************************************************/
/*=====================================================================
                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why
--------   ---      -------------------------------------------------------
04/15/10   nnv      Remove mutex usage. stop the task before reading the
                    number of ping callbacks received.
04/06/09   rr       Initial version

======================================================================*/
/*=====================================================================

                     INCLUDE FILES FOR MODULE

======================================================================*/
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "oncrpc.h"
#include "ping_mdm_rpc.h"
#include "ping_mdm_rpc_rpc.h"

/*=====================================================================
     External declarations
======================================================================*/
static unsigned int ping_mdm_num_cb_received;
static unsigned int ping_mdm_errcnt;
/*=====================================================================
      Constants and Macros
======================================================================*/
static char *VersionStr="1.1";


/*===========================================================================
  FUNCTION  ping_mdm_cb_function
===========================================================================*/
/*!
@brief
  ping callback funtion to be called remotely from the ping server

@return
  N/A

@note
  - Dependencies
    - ONCRPC
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
  unsigned int num_test_to_run = 10;
  unsigned int num_start_stop = 3;
  unsigned int i;
  unsigned int local_num_ping_received;
  unsigned int num_pings_wanted = 10;


  printf("Oncrpc Test, version %s\n",VersionStr);
  printf("Usage: oncrpc_test <num pings> <num_start_stop>, default = %d %d \n\n",
         num_test_to_run,num_start_stop);

  switch(argc)
  {
     case 3:
        num_start_stop = atoi(argv[2]);
     case 2:
        num_test_to_run = atoi(argv[1]);
      break;
  }


  printf("Initialize Library \n");
  oncrpc_init();
  ping_mdm_rpccb_app_init();

  printf("Testing Task Start/Stop Sequence \n");
  oncrpc_task_start();

  printf("Running Ping_mdm_rpc_null %d times \n",(unsigned int)num_test_to_run);
  printf("ping: ");
  for(i=0;i<num_test_to_run;i++)
  {
     ping_mdm_rpc_null();
     printf(".");
  }
  printf("\n");
  printf("Completed Pings \n");

  printf("Register for callback \n");
     ping_mdm_register_data_cb(
         ping_mdm_data_cb_function,
         num_pings_wanted,        /* Num callbacks */
         100,       /* Num words */
         10,         /* Interval ms*/
         1);        /* Num tasks, only on ver 0x00010002*/

  printf("Task Stop \n");
  oncrpc_task_stop();
  local_num_ping_received = ping_mdm_num_cb_received;
  printf("Received %d pings and stopped oncrpc_tasks, sleeping 1 \n",local_num_ping_received);
  sleep(1);
  if(local_num_ping_received != ping_mdm_num_cb_received)
  {
     printf("FAILURE: We received callbacks while tasks were stopped, %d != %d \n",
            local_num_ping_received,ping_mdm_num_cb_received);
     ping_mdm_errcnt++;

  }
  else
  {
     printf("OK, no pings received while tasks stopped \n");
  }

  printf("Task Start \n");
  oncrpc_task_start();
  sleep(1);

  if(num_pings_wanted == ping_mdm_num_cb_received)
  {
     printf("Received all pings \n");
  }
  else
  {
     printf("FAILURE: Did not receive all pings \n");
     ping_mdm_errcnt++;
  }

  printf("Testing start stop \n");

  printf("Verify Starting multiple times (%d) \n",(unsigned int)num_start_stop);
  for(i=0;i<num_start_stop;i++)
  {
     oncrpc_task_start();
  }

  printf("Verify Stopping multiple times (%d) \n",(unsigned int)num_start_stop);
  for(i=0;i<num_start_stop;i++)
  {
     oncrpc_task_stop();
  }

  oncrpc_task_start();
  printf("Testing Ping again...\n");
  printf("ping: ");
  for(i=0;i<num_test_to_run;i++)
  {
     ping_mdm_rpc_null();
     printf(".");
  }
  printf("\n");

  printf("ONCRPC TEST COMPLETE...\n");
  printf("CLOSING CONNECTIONS ... \n");
  oncrpc_deinit();
  if(ping_mdm_errcnt == 0)
  {
     printf("PASS\n");
     return 0;
  }
  else
  {
     printf("FAIL\n");
     return -ping_mdm_errcnt;
  }

}


