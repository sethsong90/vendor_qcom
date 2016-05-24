/******************************************************************************
  @file  ping_mdm_client_test_0001
  @brief Linux user-space ping server client performance test

 DESCRIPTION
  Test program for Linux user-space forward call testing with data.
  User can specify the length of the test, the size of the data.
  Defaults are test runs for 2 seconds, sending packets of 1000 words.

  Usage: ping_client_test_0001 test_duration_ms(2000 msec)

  -----------------------------------------------------------------------------
  Copyright (c) 2008-2011 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------
******************************************************************************/
/*=====================================================================
                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/modem-api/main/source/test/oncrpc/ping/src/ping_mdm_client_test_0001.c#8 $


  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why
--------   ---      -------------------------------------------------------
06/09/2011   eh     Add server-restarted callback support
10/01/2008   rr     Remove oncrpc_os.h no longer needed
09/23/2008   zp     Factored out OS abstraction layer
04/18/2008   rr     Initial version, ping client

======================================================================*/
/*=====================================================================

                     INCLUDE FILES FOR MODULE

======================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "oncrpc.h"
#include "ping_mdm_rpc.h"
#include "ping_mdm_rpc_rpc.h"
#include "ping_mdm_client_common.h"

/*=====================================================================
      Typedefs
======================================================================*/
/*=====================================================================
      Constants and Macros
======================================================================*/
static char *VersionStr="2.0";
volatile int TestExpired=0;


unsigned int timer_cb_func_called_cnt = 0;
void timer_cb_func(unsigned long a)
{
  printf("Timer Test Time Expired Callback \n");
  TestExpired=1;
}

/*=====================================================================
     External declarations
======================================================================*/

/*=====================================================================
      Variables
======================================================================*/

volatile int test_completed = 0;

/*=====================================================================
     Forward Declaration
======================================================================*/
static int ping_test_main(int duration_ms, int num_loops);
static void cleanup_cb( void *handle, void *data );



/*===========================================================================
  FUNCTION  test_completed_timer_cb_func
===========================================================================*/
/*!
@brief
  Test is complete
@return
  N/A

@note

*/
/*=========================================================================*/
void test_completion_timer_cb_func(unsigned long a)
{
  test_completed=1;
}


/*===========================================================================
  FUNCTION  ping_test_main
===========================================================================*/
/*!
@brief
  Main body of ping client

@return
  0 for pass, -1 for fail

@note
  - Dependencies
    - N/A
  - Side Effects
*/
/*=========================================================================*/
static int ping_test_main(int test_duration_ms, int test_length)
{
  uint32 rc;
  uint32 i;
  uint32 data_size_words;
  printf("\n\n\nFORWARD MODEM PING TEST STARTED...\n");

  ping_client_results_type results;
  ping_client_common_print_stats(&results,PING_SINGLE_LINE_HEADER_PRINT_FORMAT);

  printf("test_duration %d, test_length %d \n",(int)test_duration_ms, (int)test_length);
  for(i=1;i<(1024 * 8);i=i<<1)
  {
    data_size_words = i;
    rc = ping_mdm_client_common_data_test(test_length,data_size_words,test_duration_ms,0,&results);
    if(rc == 0)
    {
      ping_client_common_print_stats(&results,PING_SINGLE_LINE_PRINT_FORMAT);
    }
    else
    {
      printf("Test Failed, \n");
    }
    usleep(500000);
  }

  printf("PING TEST COMPLETE...\n");

  return(rc );
}

/*===========================================================================
  FUNCTION  ping_cleanup_cb
===========================================================================*/
/*!
@brief
  ping_cleanup_cb

@return
  N/A

@note
  - Dependencies
    - N/A
  - Side Effects
*/
/*=========================================================================*/
static void cleanup_cb( void *handle, void *data )
{
  printf("cleanup_cb called with addr %p!\n", handle);

  /* Wake up main thread to register callback */

  /* This should be in the glue code */
  oncrpc_cleanup_done(handle);
  printf("cleanup_cb done!\n");

  /* Re-register for future notifications */
  oncrpc_register_server_exit_notification_cb( PING_MDM_RPCPROG, PING_MDM_RPCVERS, cleanup_cb, NULL);
}

/*===========================================================================
  FUNCTION  restart_cb
===========================================================================*/
/*!
@brief
  Called after a restart occurs and the ONCRPC framework is back up and
  initialized.

@return
  N/A

@note
  - Dependencies
    - N/A
  - Side Effects
*/
/*=========================================================================*/
static void restart_cb( void *handle, void *data )
{
  printf("restart_cb called with addr %p!\n", handle);

  /* Re-register for future notifications */
  oncrpc_register_server_restart_notification_cb( PING_MDM_RPCPROG, PING_MDM_RPCVERS, restart_cb, NULL);
}

/*===========================================================================
  FUNCTION  main
===========================================================================*/
/*!
@brief
  program enty

@return
  rc

@note
  - Dependencies
    - N/A
  - Side Effects
*/
/*=========================================================================*/
int main(int argc, char *argv[])
{
  int rc;
  uint32 test_duration_ms;
  uint32 num_loops;
  printf("\n\n %s version %s\n",__FILE__,VersionStr);
  printf("Usage: ping_mdm_clnt_test_0001 test_duration_ms(500 msec) num_loops(50)\n");
  oncrpc_init();
  oncrpc_task_start();
  ping_mdm_rpccb_app_init();

 /* Setup defaults */
  test_duration_ms=500;
  num_loops=50;

  if(! ping_mdm_rpc_null())
  {
    printf("Ping MDM server not found on remote processor, exiting \n");
    return(-1);
  }

  switch(argc)
  {
    case 3:
      num_loops = atoi(argv[2]);
    case 2:
      test_duration_ms = atoi(argv[1]);
    break;
    default:
      printf("Using defaults  num_loops:%d, test_duration_ms:%d\n",
             (unsigned int)num_loops ,(unsigned int)test_duration_ms);
  }

  oncrpc_register_server_exit_notification_cb( PING_MDM_RPCPROG, PING_MDM_RPCVERS, cleanup_cb, NULL);
  oncrpc_register_server_restart_notification_cb( PING_MDM_RPCPROG, PING_MDM_RPCVERS, restart_cb, NULL);

  printf("\n\n\nPING TEST 0001 STARTED...\n");
  printf("Test parameters\n");

  rc =  ping_test_main(test_duration_ms,num_loops);
  if( rc == 0 )
  {
    printf("PASS\n");
  }
  else
  {
    printf("FAIL\n");
  }
  usleep(500000);
  return(rc);
}

