/******************************************************************************
  @file  ping_client_test_0000
  @brief Linux user-space ping server client test

 DESCRIPTION
  Test program for Linux user-space forward call testing with data.
  User can specify the length of the test, the size of the data.
  Defaults are test runs for 2 seconds, sending packets of 1000 words.

  Usage: ping_client_test_0 test_duration_ms(2000 msec) data_size_words(1000)

  -----------------------------------------------------------------------------
  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------

******************************************************************************/

/*=====================================================================
                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/modem-api/main/source/test/oncrpc/ping/src/ping_lte_client_test_0002.c#6 $

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.



when       who      what, where, why
--------   ---      -------------------------------------------------------
03/15/2010   aa     Changed for LTE Test
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
#include "ping_lte_rpc.h"
#include "ping_lte_rpc_rpc.h"
#include "ping_lte_client_common.h"

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
static int ping_test_main(int duration_ms);
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
static int ping_test_main(int test_duration_ms)
{
  uint32 rc;
  printf("\n\n\nPING TEST STARTED...\n");

  ping_client_results_type results;

  rc = ping_lte_client_common_data_test_random(test_duration_ms,1,&results);
    if(rc == 0)
    {
      printf("Test Passed, completed after %d ms \n",test_duration_ms);
      printf("Performance: \n");
       ping_client_common_print_stats(&results, PING_STANDARD_PRINT_FORMAT);
    }
    else
    {
      printf("Test Failed, \n");
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
  uint32 test_length;
  uint32 data_size_words;
  uint32 test_duration_ms;
  printf("\n\n %s version %s\n",__FILE__,VersionStr);
  printf("Usage: %s test_duration_ms(2000 msec) \n",argv[0]);
  printf("- To run for infinite time, set duration = 0\n");

  oncrpc_init();
  oncrpc_task_start();
  ping_lte_rpccb_app_init();

 /* Setup defaults */
  test_length = 1;
  data_size_words=1000;
  test_duration_ms=2000;

  if(! ping_lte_rpc_null())
  {
    printf("Ping LTE MDM server not found on remote processor, exiting \n");
    return(-1);
  }



  oncrpc_register_server_exit_notification_cb( PING_LTE_RPCPROG, PING_LTE_RPCVERS, cleanup_cb, NULL);
  switch(argc)
  {
    case 2:
      test_duration_ms = atoi(argv[1]);
      break;
    default:
      printf("Using defaults  test_duration_ms:%d\n",
             (unsigned int)test_duration_ms);
      break;

  }



  printf("\n\n\nPING TEST 0000 STARTED...\n");
  printf("Test parameters\n");
  printf("   Duration     : %6d ms\n",(int)test_duration_ms);

  printf("\n");

  rc =  ping_test_main( test_duration_ms);
  if( rc == 0 )
  {
    printf("PASS\n");
  }
  else
  {
    printf("FAIL\n");
  }

  return(rc);
}

