
/******************************************************************************
  @file  ping_client_test_1000
  @brief Linux user-space inter-process-communication data transfer regresion
  and performance test

  DESCRIPTION
  Test program for Linux user-space callback testing.

  DEPENDENCIES
  Requires that ping_svr is running, start it in the background on LINUX.

  -----------------------------------------------------------------------------
  Copyright (c) 2009-2011 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------
******************************************************************************/
/*=====================================================================
                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why
--------   ---      -------------------------------------------------------
06/09/2011   eh     Add server-restarted callback support
05/13/2010   kr     Modify timeout handling mechanism to avoid deadlock
02/10/2008   rr     Add support for num tasks (steal the split_packet_ms which
02/10/2008   rr     which was not being used).
02/10/2008   rr     Remove sleep from test.
01/13/2009   rr     Fixed failing test due to uninitialized parameter
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
static char *VersionStr="2.0";



#define PING_MDM_CLIENT_RESTART_SIG        (0x01)
#define PING_MDM_CLIENT_TEST_CALLBACK_SIG  (0x02)
#define PING_MDM_CLIENT_TEST_END_SIG       (0x04)
unsigned int timer_cb_func_called_cnt = 0;


/*=====================================================================
     External declarations
======================================================================*/
extern void oncrpc_task_start(void);
extern void oncrpc_init(void);

/*=====================================================================
      Variables
======================================================================*/
static uint32 *versions_array;
static uint32 num_versions;
static uint32 multiple_task_supported;

static int num_cb_expected;
static int num_cb_received;
static int errcnt;
static int register_state;

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
   while( !thread_flag )
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
static int ping_mdm_test_main( uint32 test_duration_ms,uint32 num_tasks);
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
static boolean ping_mdm_data_cb_function(uint32 *data, uint32 data_size_words, uint32 sum_cb)
{
   uint32 i;
   uint32 sum=0;
   num_cb_received++;
   for( i=0;i<data_size_words;i++ )
   {
      sum=sum^data[i];
   }

   if( sum != sum_cb )
   {
      printf("checksum error, received 0x%08x, expected 0x%08x\n",(int)sum_cb,(int)sum);
      errcnt++;
   }

   if( num_cb_received == num_cb_expected )
   {
      /* Wake up main thread */
      set_signal(PING_MDM_CLIENT_TEST_CALLBACK_SIG);
   }
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
static void ping_mdm_client_data_unregister(void)
{
   if( register_state == 1 )
   {
      ping_mdm_un_register_data_cb(ping_mdm_data_cb_function);
      register_state = 0;
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
   if( register_state == 0 )
   {
      ping_mdm_register_data_cb(
         ping_mdm_data_cb_function,
         num,
         size_words,
         interval_ms,
         split_packet_ms);

   }
   else
   {
      ping_mdm_un_register_data_cb(ping_mdm_data_cb_function);
      ping_mdm_register_data_cb(
         ping_mdm_data_cb_function,
         num,
         size_words,
         interval_ms,
         split_packet_ms);
   }
   register_state = 1;

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
static int ping_mdm_test_main(uint32 test_duration_ms,uint32 num_tasks)
{
   uint32 num;
   uint32 i;
   uint32 interval_ms = 1;
   uint32 data_size_words;
   int rc=0;
   int sigs=0;
   ping_client_results_type results;
   pthread_t timer_thread;
   int timeout_ms=test_duration_ms;
   struct itimerval delay;
   int ret;

   /* Request enough callbacks to flood, we'll stop the test with the timer
      and un-register */
   num = ( test_duration_ms / interval_ms) * num_tasks;

   if( num == 0 )
   {
      num =1;
   }
   num_cb_expected = num;

   printf("\n\n\nPING MDM CALLBACK TEST STARTED with %d tasks ...\n",(unsigned int)num_tasks);

   ping_client_common_print_stats(&results,PING_SINGLE_LINE_HEADER_PRINT_FORMAT);
   for( i=1;i<(1024*8);i=i<<1 )
   {
      data_size_words = i;
      num_cb_received=0;
      sigs =0;
      /*
      printf("Register num %d, data_size %d, interval %d, num_tasks %d \n",
             (unsigned int)num,(unsigned int)data_size_words,(unsigned int)interval_ms,
             (unsigned int)num_tasks);
             */
      pthread_create(&timer_thread, NULL, timer_cb_func, (void *)timeout_ms);

      oncrpc_register_server_exit_notification_cb( PING_MDM_RPCPROG, PING_MDM_RPCVERS, cleanup_cb, NULL);
      oncrpc_register_server_restart_notification_cb( PING_MDM_RPCPROG, PING_MDM_RPCVERS, restart_cb, NULL);
      ping_mdm_client_register(num,data_size_words,interval_ms,num_tasks);

      while( (sigs & PING_MDM_CLIENT_TEST_END_SIG) == 0 )
      {
         //printf("Wait for 0x%08x \n",(unsigned int)sigs);
         sigs = wait_for();
         //printf("Got 0x%08x \n",(unsigned int)sigs);

         if( sigs & PING_MDM_CLIENT_TEST_END_SIG )
         {
            //printf("Received Test End Signal \n");
            clr_flag(PING_MDM_CLIENT_TEST_END_SIG);
            //printf("Un-register \n");
            ping_mdm_client_data_unregister();
         }
         else if( sigs & PING_MDM_CLIENT_RESTART_SIG )
         {
            clr_flag(PING_MDM_CLIENT_RESTART_SIG);
            printf("Waking up on server exit! Re-register\n");
            oncrpc_register_server_exit_notification_cb( PING_MDM_RPCPROG, PING_MDM_RPCVERS, cleanup_cb, NULL);
            oncrpc_register_server_restart_notification_cb( PING_MDM_RPCPROG, PING_MDM_RPCVERS, restart_cb, NULL);
            ping_mdm_client_register(num_cb_expected - num_cb_received,data_size_words,interval_ms,num_tasks);
         }
      }
      ping_client_common_calc_stats(num_cb_received,data_size_words,1,test_duration_ms,&results);
      ping_client_common_print_stats(&results, PING_SINGLE_LINE_PRINT_FORMAT );
      pthread_join(timer_thread, NULL);
   }
   printf("\n");


   if( num_cb_received == 0 )
   {
      printf("PINT TEST FAILED, no callbacks received \n");
      rc = -1;
   }
   else
   {
      if( errcnt == 0 )
      {
         printf("PING TEST COMPLETE...\n");
      }
      else
      {
         printf("PINT TEST FAILED, %d errors \n",errcnt);
      }
      rc = -errcnt;
   }
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
  FUNCTION  get_verions_info
===========================================================================*/
/*!
@brief
  Get version information from remote node for ping_mdm

@return
  pointer to versions_info

@note
  - Dependencies
    - N/A
  - Side Effects
    Populates versions_array and num_versions
*/
/*=========================================================================*/
static int get_verions_info()
{
   uint32 i;
   versions_array = ping_mdm_rpc_api_versions(&num_versions);
   printf("Version Supported :");
   for(i=0;i<num_versions;i++)
   {
      printf("0x%08x  \n",(unsigned int)versions_array[i]);
      if( ( ( (versions_array[i] & 0x0fff0000) == 0x00010000) &&
            ( (versions_array[i] & 0x0000ffff) >= 0x00000002) ) ||
          ( ( (versions_array[i] & 0x0fff0000) > 0x00010000)  )
          )
      {
         multiple_task_supported=1;
         printf("Multiple Task Supported with version 0x%08x \n",
                (unsigned int)versions_array[i]);
      }
   }
   return((int)versions_array);
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
   uint32 test_duration_ms=500;
   uint32 num_tasks=1;

   printf("\n\n Ping Clnt Test, version %s\n",VersionStr);
   printf("Usage: ping_mdm_clnt_test_1001 test_duration_ms(%d msec) num_tasks (%d) \n",(unsigned int)test_duration_ms,(unsigned int)num_tasks);
   oncrpc_init();
   oncrpc_task_start();
   ping_mdm_rpccb_app_init();

   init_signaling();

   if( ! ping_mdm_rpc_null() )
   {
      printf("Ping MDM server not found on remote processor, exiting \n");
      return(-1);
   }

   if(! get_verions_info())
   {
      printf("Invalid data returned from get_versions_info \n");
      return(-2);
   }


   switch( argc )
   {
      case 3:
         if(!multiple_task_supported)
         {
            printf("Parameter num_tasks ignored, not supported by modem \n");
         }
         else
         {
            num_tasks = atoi(argv[2]);
         }
      case 2:
         test_duration_ms = atoi(argv[1]);
         break;
      default:
         printf("Using defaults  test_duration_ms:%d num_tasks:%d\n",
            (unsigned int)test_duration_ms,(unsigned int)num_tasks);
         break;
   }

   if( (!multiple_task_supported) || (num_tasks < 1) )
   {
      num_tasks=1;
   }

   rc = ping_mdm_test_main(test_duration_ms,num_tasks);
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




