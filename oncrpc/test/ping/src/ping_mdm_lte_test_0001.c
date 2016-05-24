/******************************************************************************
  @file  ping_mdm_lte_test_0001
  @brief Linux user-space ping server client performance test

 DESCRIPTION
  Test program for Linux user-space forward call testing with data.

  This test sends a command to the remote modem from which the test is executed
  such that there are 2 jumps.
  Host >>SMD>> Modem
               Modem Run Test >>HSUART>>  Remote Modem
                              << Results
  Host Results printed.

  This test runs a performance test with multiple packet size, increasing
  by a factor or 2 on each test.
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
volatile int CbReceived = 0;
static pthread_cond_t thread_flag_cv;
static pthread_mutex_t thread_flag_mutex;

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
			ping_client_common_calc_stats(results.num_loops_completed,
						      results.data_size_words,
						      results.ret_data_size_words,
						      results.test_duration_ms,
						      &calc_results);

			ping_client_common_print_stats(&calc_results, PING_SINGLE_LINE_PRINT_FORMAT);


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


	pthread_mutex_lock(&thread_flag_mutex);
	CbReceived =1;
	pthread_cond_signal(&thread_flag_cv);
	pthread_mutex_unlock(&thread_flag_mutex);
	return 1;
}
#endif

/*=====================================================================
     Forward Declaration
======================================================================*/
static int ping_test_main(int num, int duration_ms);
static void cleanup_cb( void *handle, void *data );


static inline void init(void)
{
	pthread_mutex_init(&thread_flag_mutex, NULL);
	pthread_cond_init(&thread_flag_cv, NULL);
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
static int ping_test_main(int test_length, int test_duration_ms)
{
	uint32 rc,i;
	struct timespec timewait;
#ifdef PING_MDM_START_PING_LTE_TEST_VERS
	ping_mdm_ping_lte_parm_type  param;
	ping_client_results_type results;

	ping_client_common_print_stats(&results,PING_SINGLE_LINE_HEADER_PRINT_FORMAT);

	timewait.tv_nsec=0;
	timewait.tv_sec = 1;

	for(i=1; i < (1024 * 2); i = i<<1)
	{
		param.data_size_words = i;
		param.test_duration_ms = test_duration_ms;
		pthread_mutex_lock(&thread_flag_mutex);
		CbReceived = 0;
		pthread_mutex_unlock(&thread_flag_mutex);
		rc = ping_mdm_start_ping_lte_test(PING_MDM_TEST_0000,param,ping_mdm_ping_lte_results_cb);
		if(rc != 0)
		{
			printf("Test Failed: rc %d\n",(int)rc);
			ping_mdm_stop_ping_lte_test(ping_mdm_ping_lte_results_cb);
			return rc;
		}
		pthread_mutex_lock(&thread_flag_mutex);
		while(CbReceived == 0)
		{
			pthread_cond_wait(&thread_flag_cv, &thread_flag_mutex);
		}
		pthread_mutex_unlock(&thread_flag_mutex);
		sleep(1);
		ping_mdm_stop_ping_lte_test(ping_mdm_ping_lte_results_cb);

	}

	printf("Stopping Test \n");
	printf("PING TEST COMPLETE...\n");
#else
	printf("ping_mdm_lte_test not available\n");
	rc = 0;
#endif


	return(rc );
}

/*===========================================================================
  FUNCTION  cleanup_cb
===========================================================================*/
/*!
@brief
  cleanup_cb

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
  FUNCTION  compare
===========================================================================*/
/*!
@brief
  compare

@return
  N/A

@note
  - Dependencies
    - N/A
  - Side Effects
*/
/*=========================================================================*/
boolean compare(void *d1, void *d2)
{
	uint32 result = 1;
	return result;
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
	uint32 test_duration_ms=100;

	printf("\n\n ping_mdm_lte_test_0001 version %s\n",VersionStr);
	printf("Usage: ping_mdm_lte_test_0001 test_duration_ms(%d msec) per data size \n",(int)test_duration_ms);
	init();
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
		case 3:
			test_length = atoi(argv[3]);
		case 2:
			test_duration_ms = atoi(argv[1]);
			break;
		default:
			printf("Using defaults test_length:%d, test_duration_ms:%d\n",
			       (unsigned int)test_length ,(unsigned int)test_duration_ms);
	}

	printf("\n\n\nTEST STARTED...\n");
	printf("Test parameters\n");
	if((test_duration_ms == 0) && ( test_length == 0))
	{
		printf("   Duration     : inifinite\n");
	}
	printf("   Duration     : %6d ms\n",(int)test_duration_ms);
	if(argc > 2)
	{
		printf("   Test Length  : %6d words\n",(int)test_length);
	}
	printf("\n");

	rc =  ping_test_main( test_length,test_duration_ms);
	if(rc == 0)
	{
		printf("PASS\n");
	}
	else
	{
		printf("FAIL\n");
	}
	oncrpc_unregister_server_exit_notification_cb(PING_MDM_RPCPROG, PING_MDM_RPCVERS, cleanup_cb, NULL, compare);
	return(rc);
}

