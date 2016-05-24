/******************************************************************************
  @file  ping_mdm_lte_test_0000
  @brief Linux user-space ping server client test

 DESCRIPTION
  Test program for Linux user-space forward call testing with data.

  This test sends a command to the remote modem from which the test is executed
  such that there are 2 jumps.
  Host >>SMD>> Modem
               Modem Run Test >>HSUART>>  Remote Modem
                              << Results
  Host Results printed.

  -----------------------------------------------------------------------------
  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------

******************************************************************************/

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
static char *VersionStr="1.0";
volatile int TestExpired=0;
volatile int CbReceived = 0;


unsigned int timer_cb_func_called_cnt = 0;
void timer_cb_func(unsigned long a)
{
  printf("Timer Test Time Expired Callback \n");
  TestExpired=1;
}

#ifdef PING_MDM_START_PING_LTE_TEST_VERS
boolean ping_mdm_ping_lte_results_cb (
                               ping_mdm_ping_lte_test_type test,
                               ping_mdm_ping_lte_results_type results
                               )
{
  ping_mdm_ping_lte_rc_type rc = results.rc;
  ping_client_results_type calc_results;

  switch(rc)
  {
    case PING_MDM_SUCCESS:
      printf("Ping test successful!\n");
      ping_client_common_calc_stats(results.num_loops_completed,
                                    results.data_size_words,
                                    results.ret_data_size_words,
                                    results.test_duration_ms,
                                    &calc_results);

      ping_client_common_print_stats(&calc_results, PING_STANDARD_PRINT_FORMAT);
      break;
    case PING_MDM_ERR_NOSRV:
      printf("ERROR: ping_lte server not found...\n");
      break;
    case PING_MDM_ERR_CHKSUM:
      printf("ERROR: chksum error in data transmit\n");
      break;
    case PING_MDM_ERR_NOCB:
      printf("No callbacks received... This should not be the case for forward tests.\n");
      break;
    case PING_MDM_ERR_UNKNOWN:
      default:
      printf("ERROR: Unknown cause\n");
  }

  CbReceived = 1;

  return 1;
}
#endif

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
static int ping_test_main(int num, int size_words, int duration_ms);
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
static int ping_test_main(int test_length, int data_size_words, int test_duration_ms)
{
  uint32 rc;
#ifdef PING_MDM_START_PING_LTE_TEST_VERS
  ping_mdm_ping_lte_parm_type  param;
  ping_client_results_type results;

  printf("\n\n\nPING TEST STARTED...\n");

  param.data_size_words = data_size_words;
  param.test_duration_ms = test_duration_ms;
  CbReceived = 0;
  rc = ping_mdm_start_ping_lte_test(PING_MDM_TEST_0000,param,ping_mdm_ping_lte_results_cb);
  if(rc == 0)
  {
    printf("Starting Test....\n");
  }
  else
  {
    printf("Test Failed, \n");
    return -1;
  }
  while(CbReceived == 0)
  {
    sleep(1);
  }

  printf("Stopping Test \n");
  ping_mdm_stop_ping_lte_test(ping_mdm_ping_lte_results_cb);
  printf("PING TEST COMPLETE...\n");
#else
  printf("ping_mdm_lte_test not available\n");
  rc = 0;
#endif
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
  /* Setup defaults */
  uint32 test_length=0;
  uint32 data_size_words=100;
  uint32 test_duration_ms=2000;

  printf("\n\n ping_mdm_lte_test_0000 version %s\n",VersionStr);
  printf("Usage: ping_mdm_lte_test_0000 test_duration_ms(2000 msec) data_size_words(1000) test_length(0) \n");
  oncrpc_init();
  oncrpc_task_start();
  ping_mdm_rpccb_app_init();


  if(! ping_mdm_rpc_null())
  {
    printf("Ping LTE MDM server not found on remote processor, exiting \n");
    return(-1);
  }



  oncrpc_register_server_exit_notification_cb( PING_MDM_RPCPROG, PING_MDM_RPCVERS, cleanup_cb, NULL);
  switch(argc)
  {
    case 4:
      test_length = atoi(argv[3]);
    case 3:
      data_size_words = atoi(argv[2]);
    case 2:
      test_duration_ms = atoi(argv[1]);
      break;
    default:
      printf("Using defaults test_length:%d, data_size_words:%d, test_duration_ms:%d\n",
             (unsigned int)test_length ,(unsigned int)data_size_words,(unsigned int)test_duration_ms);
  }



  printf("\n\n\nTEST STARTED...\n");
  printf("Test parameters\n");
  if( (test_duration_ms == 0) && ( test_length == 0) )
  {
    printf("   Duration     : inifinite\n");
  }
  printf("   Duration     : %6d ms\n",(int)test_duration_ms);
  printf("   Data Size    : %6d words\n",(int)data_size_words);
  if(argc > 3)
  {
    printf("   Test Length  : %6d words\n",(int)test_length);
  }
  printf("\n");

  rc =  ping_test_main( test_length,data_size_words,test_duration_ms);
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

