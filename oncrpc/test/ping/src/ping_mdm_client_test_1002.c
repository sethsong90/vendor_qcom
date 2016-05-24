/******************************************************************************
  @file  ping_client_test_1002
  @brief Linux user-space inter-process-communication data transfer regresion
  and performance test

  DESCRIPTION
  Test program for Linux user-space callback testing.

  DEPENDENCIES
  Requires that ping_svr is running, start it in the background on LINUX.

  -----------------------------------------------------------------------------
  Copyright (c) 2007-2011 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------
******************************************************************************/
/*=====================================================================
                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/modem-api/main/source/test/oncrpc/ping/src/ping_mdm_client_test_1002.c#4 $

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why
--------   ---      -------------------------------------------------------
06/09/2011   eh     Add server-restarted callback support
05/13/2010   kr     Modify timeout handling mechanism to avoid deadlocks
09/23/2008   zp     Factored out OS abstraction layer
04/18/2008   rr     Initilal version.

======================================================================*/
/*=====================================================================

                     INCLUDE FILES FOR MODULE

======================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>
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


#define PING_MDM_CLIENT_RESTART_SIG        (0x01)
#define PING_MDM_CLIENT_TEST_END_SIG       (0x02)
#define PING_MDM_CLIENT_TEST_CALLBACK_SIG_1  (0x04)
#define PING_MDM_CLIENT_TEST_CALLBACK_SIG_2  (0x08)

unsigned int timer_cb_func_called_cnt = 0;


/*=====================================================================
     External declarations
======================================================================*/

/*=====================================================================
      Variables
======================================================================*/
static int num_cb_expected = 0;
static int num_cb_received[2] = {0,0};
static int errcnt = 0;
static int registerd_state = 0;

static int thread_flag;
static pthread_cond_t thread_flag_cv;
static pthread_mutex_t thread_flag_mutex;


static inline void init_signaling(void)
{
  pthread_mutex_init(&thread_flag_mutex, NULL);
  pthread_cond_init(&thread_flag_cv, NULL);
  thread_flag = 0;
}

static inline void set_signal(int val)
{
  pthread_mutex_lock(&thread_flag_mutex);
  thread_flag |= val;
  pthread_cond_signal(&thread_flag_cv);
  pthread_mutex_unlock(&thread_flag_mutex);
}

static inline void clr_flag(int val)
{
  pthread_mutex_lock(&thread_flag_mutex);
  thread_flag &= ~val;
  pthread_mutex_unlock(&thread_flag_mutex);
}

static inline int wait_for(void)
{
  int sigs;

  pthread_mutex_lock(&thread_flag_mutex);
  while(!thread_flag)
  {
    pthread_cond_wait(&thread_flag_cv, &thread_flag_mutex);
  }

  sigs = thread_flag;
  pthread_mutex_unlock(&thread_flag_mutex);

  return sigs;
}

static inline int zero_timer(struct itimerval *val)
{
  val->it_value.tv_sec = 0;
  val->it_value.tv_usec = 0;
  val->it_interval.tv_sec = 0;
  val->it_interval.tv_usec = 0;

  return setitimer(ITIMER_REAL, val, NULL);
}

static inline int set_timer_ms(struct itimerval *val, int ms)
{
  val->it_value.tv_sec = ms/1000;
  val->it_value.tv_usec = ms*1000 % 1000000;
  val->it_interval.tv_sec = 0;
  val->it_interval.tv_usec = 0;

  return setitimer(ITIMER_REAL, val, NULL);
}


/*=====================================================================
     Forward Declaration
======================================================================*/
static int ping_mdm_test_main(uint32 data_size_words, uint32 test_duration_ms, uint32 interval_ms);
static void ping_mdm_client_register(uint32 num,uint32 size_words, uint32 inerval_ms, uint32 split_packet_ms);
static void ping_mdm_client_data_unregister(void);
static void cleanup_cb( void *handle, void *data );
static void restart_cb( void *handle, void *data );

/*===========================================================================
  FUNCTION  timer_cb_func
===========================================================================*/
/*!
@brief
  Timer callback when test expieres

@return
  N/A

@note
  - Dependencies
    - ONCRPC
  - Side Effects
*/
/*=========================================================================*/
void *timer_cb_func(void *arg)
{
   struct timeval timeout;
   uint32 timeout_ms = (uint32)arg;

   timeout.tv_sec = timeout_ms/1000;
   timeout.tv_usec = (timeout_ms % 1000) * 1000;
   select(0, NULL, NULL, NULL, &timeout);

   set_signal(PING_MDM_CLIENT_TEST_END_SIG);
   return ((void *) 0);
}
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
static boolean ping_mdm_data_cb_function_1(uint32 *data, uint32 data_size_words, uint32 sum_cb)
{
  uint32 i;
  uint32 sum=0;
  num_cb_received[0]++;
  for( i=0;i<data_size_words;i++ )
  {
    sum=sum^data[i];
  }

  if( sum != sum_cb )
  {
    printf("checksum error, received 0x%08x, expected 0x%08x\n",(int)sum_cb,(int)sum);
    errcnt++;
  }
  set_signal(PING_MDM_CLIENT_TEST_CALLBACK_SIG_1);

  return( sum == sum_cb );
}



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
static boolean ping_mdm_data_cb_function_2(uint32 *data, uint32 data_size_words, uint32 sum_cb)
{
  uint32 i;
  uint32 sum=0;
  num_cb_received[1]++;
  for( i=0;i<data_size_words;i++ )
  {
    sum=sum^data[i];
  }

  if( sum != sum_cb )
  {
    printf("checksum error, received 0x%08x, expected 0x%08x\n",(int)sum_cb,(int)sum);
    errcnt++;
  }

  /* Wake up main thread */
  set_signal(PING_MDM_CLIENT_TEST_CALLBACK_SIG_2);
  return( sum == sum_cb );
}

/*===========================================================================
  FUNCTION  ping_mdm_client_unregister
===========================================================================*/
/*!
@brief
  ping client local unregister functions, maintains state.

@return
  N/A

@note
  - Dependencies
    - N/A
  - Side Effects
*/
/*=========================================================================*/
static void ping_mdm_client_data_unregister()
{
   if(registerd_state == 1) {
      ping_mdm_un_register_data_cb(ping_mdm_data_cb_function_1);
      ping_mdm_un_register_data_cb(ping_mdm_data_cb_function_2);
      registerd_state = 0;
   }
}

/*===========================================================================
  FUNCTION  ping_mdm_client_register
===========================================================================*/
/*!
@brief
  ping client local register functions, maintains state.

@return
  N/A

@note
  - Dependencies
    - N/A
  - Side Effects
*/
/*=========================================================================*/
static void ping_mdm_client_register(uint32 num,uint32 size_words, uint32 interval_ms, uint32 split_packet_ms)
{
  if( registerd_state == 0 )
  {
    printf("Auto-Register with server \n");

    ping_mdm_register_data_cb(
      ping_mdm_data_cb_function_1,
      num,
      size_words,
      interval_ms,
      split_packet_ms);
    ping_mdm_register_data_cb(
      ping_mdm_data_cb_function_2,
      num,
      size_words,
      interval_ms,
      split_packet_ms);

  }
  else
  {
    printf("Auto-UnRegister with server \n");
    ping_mdm_un_register_data_cb(ping_mdm_data_cb_function_1);
    ping_mdm_un_register_data_cb(ping_mdm_data_cb_function_2);
    ping_mdm_register_data_cb(
      ping_mdm_data_cb_function_1,
      num,
      size_words,
      interval_ms,
      split_packet_ms);
    ping_mdm_register_data_cb(
      ping_mdm_data_cb_function_2,
      num,
      size_words,
      interval_ms,
      split_packet_ms);
  }
  registerd_state = 1;
}

/*===========================================================================
  FUNCTION  ping_mdm_test_main
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
static int ping_mdm_test_main(uint32 data_size_words, uint32 test_duration_ms, uint32 interval_ms)
{
  uint32 num;
  int rc=0;
  uint32 split_packet_ms;
  int sigs=0;
  ping_client_results_type results;
  int timeout_ms=test_duration_ms;
  struct timespec time_info_last;
  struct timespec time_info_current;
  pthread_t timer_thread;
  struct itimerval delay;
  int ret;


  if( interval_ms > 0 )
  {
    num=(test_duration_ms + 200)/interval_ms;
  }
  else
  {
    /* Assume at least 100 us per RPC call */
    num = test_duration_ms * 10;
  }

  if( num == 0 )
  {
    num =1;
  }
  num_cb_expected = num;
  split_packet_ms = 0;

  printf("\n\n\nPING TEST STARTED...\n");
  printf("Test parameters\n");
  printf("   Duration     : %6d ms\n",(int)test_duration_ms);
  printf("   Data Size    : %6d words\n",(int)data_size_words);
  printf("   Interval     : %6d ms\n",(int)interval_ms);
  printf("   Fail timeout : %6d ms\n",(int)timeout_ms);
  printf("\n");

  pthread_create(&timer_thread, NULL, timer_cb_func, (void *)timeout_ms);

  ping_mdm_client_register(num,data_size_words,interval_ms,split_packet_ms);
  oncrpc_register_server_exit_notification_cb( PING_MDM_RPCPROG, PING_MDM_RPCVERS, cleanup_cb, NULL);
  oncrpc_register_server_restart_notification_cb( PING_MDM_RPCPROG, PING_MDM_RPCVERS, restart_cb, NULL);

  printf("Waiting for ping...\n");
  clock_gettime(CLOCK_REALTIME,&time_info_last);
  while( (sigs & PING_MDM_CLIENT_TEST_END_SIG) == 0 )
  {
    sigs = wait_for();

    if( sigs & PING_MDM_CLIENT_TEST_END_SIG )
    {
      ping_mdm_client_data_unregister();
    }
    else if( sigs & PING_MDM_CLIENT_RESTART_SIG )
    {
      ping_mdm_client_data_unregister();

      printf("Waking up on server exit! Re-register\n");
      registerd_state = 0;
      oncrpc_register_server_exit_notification_cb( PING_MDM_RPCPROG, PING_MDM_RPCVERS, cleanup_cb, NULL);
      oncrpc_register_server_restart_notification_cb( PING_MDM_RPCPROG, PING_MDM_RPCVERS, restart_cb, NULL);
      ping_mdm_client_register(num_cb_expected - num_cb_received[0],data_size_words,interval_ms,split_packet_ms);
    }else if( sigs & PING_MDM_CLIENT_TEST_CALLBACK_SIG_1 )
    {
      clock_gettime(CLOCK_REALTIME,&time_info_current);
      if(time_info_current.tv_sec > (time_info_last.tv_sec ))
      {
        time_info_last.tv_sec  = time_info_current.tv_sec ;
        printf("Rpc callbacks receive A:%5d B:%5d Num Errors:%d\n",num_cb_received[0],num_cb_received[1],(int)errcnt);
      }
    }
    /* clear flag */
    clr_flag(sigs);
  }
  printf("\n");

    /* NOTE Remove this sleep... */
  /* In this particular test, we tell the MDM server to send large RPC calls continually and we abruptly
   terminate the client.   The Server may be in the process of writing a multiple-packet message to the
   router, and the router is waiting for flow control from the remote client which has died.

   Currently the router waits indefinitely, which may cause a deadlock in the server which is waiting for
   the flow control from a non-existing client.

   We want to fix this situation, but for the moment, add a sleep in the client test to allow the server
   to process the de-registration before we exit */

  printf("Sleeping before exit to allow server to process de-registration \n");
  sleep(1);

  if( errcnt == 0 )
  {
    printf("PING TEST COMPLETE...\n");
  }
  else
  {
    printf("PINT TEST FAILED, %d errors \n",errcnt);
  }
  rc = -errcnt;

  ping_client_common_calc_stats(num_cb_received[0],data_size_words,1,test_duration_ms,&results);
  ping_client_common_print_stats(&results, PING_STANDARD_PRINT_FORMAT );

  return(rc);
}

/*===========================================================================
  FUNCTION  ping_mdm_cleanup_cb
===========================================================================*/
/*!
@brief
  ping_mdm_cleanup_cb

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
  set_signal(PING_MDM_CLIENT_RESTART_SIG);

  oncrpc_cleanup_done(handle);
  printf("cleanup_cb done!\n");
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
  uint32 data_size_words;
  uint32 test_duration_ms;
  uint32 interval;
  printf("\n\n Ping Clnt Test, version %s\n",VersionStr);
  printf("Usage: ping_mdm_client_test_1 test_duration_ms(1000 msec) data_size_words(64) interval(1000 ms)\n");

  oncrpc_init();
  oncrpc_task_start();
  ping_mdm_rpccb_app_init();

  init_signaling();

  if( ! ping_mdm_rpc_null() )
  {
    printf("Ping MDM server not found on remote processor, exiting \n");
    return(-1);
  }

  data_size_words=64;
  test_duration_ms=2000;
  interval = 1000;
  if( argc > 1 )
  {
    test_duration_ms = atoi(argv[1]);
  }
  if( argc > 2 )
  {
    data_size_words = atoi(argv[2]);
  }
  if( argc > 3 )
  {
    interval = atoi(argv[3]);
  }

  rc = ping_mdm_test_main(data_size_words, test_duration_ms,interval);
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




