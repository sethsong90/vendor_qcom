/******************************************************************************
  @file  ping_client_test_1000
  @brief Linux user-space inter-process-communication data transfer regresion
  and performance test

  DESCRIPTION
  Test program for Linux user-space callback testing.

  DEPENDENCIES
  Requires that ping_svr is running, start it in the background on LINUX.

  -----------------------------------------------------------------------------
  Copyright (c) 2007-2010 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------
******************************************************************************/
/*=====================================================================
                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/modem-api/main/source/test/oncrpc/ping/src/ping_lte_client_test_1000.c#6 $

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why
--------   ---      -------------------------------------------------------
03/15/2010   aa     Changed for LTE Test
02/10/2008   rr     Add support for num tasks (steal the split_packet_ms which
02/10/2008   rr     which was not being used).
02/10/2008   rr     Remove sleep from test.
01/13/2008   rr     Set interval to 1 to get better coverage with defaults
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
#include "ping_lte_rpc.h"
#include "ping_lte_rpc_rpc.h"
#include "ping_lte_client_common.h"

/*=====================================================================
      Typedefs
======================================================================*/
/*=====================================================================
      Constants and Macros
======================================================================*/
static char *VersionStr="1.0";


#define PING_LTE_CLIENT_RESTART_SIG        (0x01)
#define PING_LTE_CLIENT_TEST_CALLBACK_SIG  (0x02)
#define PING_LTE_CLIENT_TEST_END_SIG       (0x04)

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
static uint32 multiple_task_supported=0;

static int num_cb_expected = 0;
static int num_cb_received = 0;
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
static int ping_lte_test_main(uint32 data_size_words, uint32 test_duration_ms, uint32 interval_ms, uint32 num_tasks);
static void ping_lte_client_register(uint32 num,uint32 size_words, uint32 inerval_ms, uint32 split_packet_ms);
static void ping_lte_client_data_unregister(void);
static void cleanup_cb( void *handle, void *data );

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
void timer_cb_func(int a)
{
   printf("Timer Test Time Expired Callback \n");
   set_signal(PING_LTE_CLIENT_TEST_END_SIG);
}
/*===========================================================================
  FUNCTION  ping_lte_cb_function
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
static boolean ping_lte_data_cb_function(uint32 *data, uint32 data_size_words, uint32 sum_cb)
{
   uint32 i;
   uint32 sum=0;
   //printf("Ping callback received from server, data_size:%d\n",(int)data_size_words);
   num_cb_received++;
   for( i=0;i<data_size_words;i++ )
   {
      sum=sum^data[i];
      //printf("data:0x%08x\n",(int)data[i]);
   }

   if( sum != sum_cb )
   {
      printf("checksum error, received 0x%08x, expected 0x%08x\n",(int)sum_cb,(int)sum);
      errcnt++;
   }
   else
   {

      //printf("PASS checksum, received 0x%08x, expected 0x%08x\n",(int)sum_cb,(int)sum);
   }

   ///if( num_cb_received == num_cb_expected )
   {
      /* Wake up main thread */
      set_signal(PING_LTE_CLIENT_TEST_CALLBACK_SIG);
      pthread_mutex_unlock(&thread_flag_mutex);
   }
   return( sum == sum_cb );
}


/*===========================================================================
  FUNCTION  ping_lte_client_unregister
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
static void ping_lte_client_data_unregister(void)
{
   if( registerd_state == 1 )
   {
      ping_lte_un_register_data_cb(ping_lte_data_cb_function);
      registerd_state = 0;
   }
}

/*===========================================================================
  FUNCTION  ping_lte_client_register
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
static void ping_lte_client_register(uint32 num,uint32 size_words, uint32 interval_ms, uint32 split_packet_ms)
{
   if( registerd_state == 0 )
   {
      printf("Auto-Register with server \n");

      ping_lte_register_data_cb(
         ping_lte_data_cb_function,
         num,
         size_words,
         interval_ms,
         split_packet_ms);

   }
   else
   {
      printf("Auto-UnRegister with server \n");
      ping_lte_un_register_data_cb(ping_lte_data_cb_function);
      ping_lte_register_data_cb(
         ping_lte_data_cb_function,
         num,
         size_words,
         interval_ms,
         split_packet_ms);
   }
}

/*===========================================================================
  FUNCTION  ping_lte_test_main
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
static int ping_lte_test_main(uint32 data_size_words,
   uint32 test_duration_ms,
   uint32 interval_ms,
   uint32 num_tasks)
{
   uint32 num;
   int rc=0;
   int sigs=0;
   ping_client_results_type results;
   int timeout_ms=test_duration_ms;
   struct timespec time_info_last;
   struct timespec time_info_current;
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

   printf("\n\n\nPING TEST STARTED...\n");
   printf("Test parameters\n");
   printf("   Duration     : %6d ms\n",(int)test_duration_ms);
   printf("   Data Size    : %6d words\n",(int)data_size_words);
   printf("   Interval     : %6d ms\n",(int)interval_ms);
   printf("   Num Tasks    : %6d \n",(int)num_tasks);
   printf("   Fail timeout : %6d ms\n",(int)timeout_ms);
   printf("\n");

   signal(SIGALRM, timer_cb_func);
   ret = set_timer_ms(&delay, timeout_ms);
   if( ret )
   {
      perror("setitimer");
      errcnt++;
      return -errcnt;
   }

   ping_lte_client_register(num,data_size_words,interval_ms,num_tasks);
   printf("Waiting for ping...\n");
   clock_gettime(CLOCK_REALTIME,&time_info_last);
   while( (sigs & PING_LTE_CLIENT_TEST_END_SIG) == 0 )
   {
      sigs = wait_for();
      if( sigs & PING_LTE_CLIENT_TEST_END_SIG )
      {
         ping_lte_client_data_unregister();
      }
      else if( sigs & PING_LTE_CLIENT_RESTART_SIG )
      {
         printf("Waking up on server exit! Re-register\n");

         /* clear flag */
         clr_flag(PING_LTE_CLIENT_RESTART_SIG);

         registerd_state = 0;
         oncrpc_register_server_exit_notification_cb( PING_LTE_RPCPROG, PING_LTE_RPCVERS, cleanup_cb, NULL);
         ping_lte_client_register(num_cb_expected - num_cb_received,data_size_words,interval_ms,num_tasks);
      }
      else if( sigs & PING_LTE_CLIENT_TEST_CALLBACK_SIG )
      {
         clock_gettime(CLOCK_REALTIME,&time_info_current);
         if( time_info_current.tv_sec > (time_info_last.tv_sec ) )
         {
            time_info_last.tv_sec  = time_info_current.tv_sec ;
            printf("Rpc callbacks receive %5d Num Errors:%d\n",num_cb_received,(int)errcnt);
         }
      }

      /* clear flag */
      clr_flag(sigs);
   }
   printf("\n");

   ret = zero_timer(&delay);
   if( ret )
   {
      perror("setitimer");
      errcnt++;
      return -errcnt;
   }

   ping_lte_client_data_unregister();

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




   ping_client_common_calc_stats(num_cb_received,data_size_words,1,test_duration_ms,&results);
   ping_client_common_print_stats(&results, PING_STANDARD_PRINT_FORMAT );

   return(rc);
}

/*===========================================================================
  FUNCTION  ping_lte_cleanup_cb
===========================================================================*/
/*!
@brief
  ping_lte_cleanup_cb

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
   set_signal(PING_LTE_CLIENT_RESTART_SIG);
   oncrpc_cleanup_done(handle);
   printf("cleanup_cb done!\n");
}


/*===========================================================================
  FUNCTION  get_verions_info
===========================================================================*/
/*!
@brief
  Get version information from remote node for ping_lte

@return
  pointer to versions_info

@note
  - Dependencies
    - N/A
  - Side Effects
    Populates versions_array and num_versions
*/
/*=========================================================================*/
static int get_versions_info()
{
   uint32 i;
   versions_array = ping_lte_rpc_api_versions(&num_versions);
   printf("Version Supported :");
   for( i=0;i<num_versions;i++ )
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
   uint32 data_size_words=64;
   uint32 test_duration_ms=2000;
   uint32 interval=1;
   uint32 num_tasks=1;
   printf("\n\n Ping Clnt Test, version %s\n",VersionStr);
   printf("Usage: ping_lte_client_test_1 test_duration_ms(%d msec) data_size_words(%d) interval(%d ms) num_tasks(%d)\n",
      (unsigned int)test_duration_ms,
      (unsigned int)data_size_words,
      (unsigned int)interval,
      (unsigned int)num_tasks);
   oncrpc_init();
   oncrpc_task_start();
   ping_lte_rpccb_app_init();

   init_signaling();

   if( ! ping_lte_rpc_null() )
   {
      printf("Ping LTE MDM server not found on remote processor, exiting \n");
      return(-1);
   }

   if( ! get_versions_info() )
   {
      printf("Invalid data returned from get_versions_info \n");
      return(-2);
   }

   oncrpc_register_server_exit_notification_cb( PING_LTE_RPCPROG, PING_LTE_RPCVERS, cleanup_cb, NULL);

   switch( argc )
   {
      case 5:
         if( !multiple_task_supported )
         {
            printf("Parameter num_tasks ignored, not supported by modem \n");
         }
         else
         {
            num_tasks = atoi(argv[4]);
         }
      case 4:
         interval = atoi(argv[3]);
      case 3:
         data_size_words = atoi(argv[2]);
      case 2:
         test_duration_ms = atoi(argv[1]);
         break;
      default:
         printf("Using defaults interval:%d, data_size_words:%d, test_duration_ms:%d, num_tasks:%d\n",
            (unsigned int)interval ,(unsigned int)data_size_words,(unsigned int)test_duration_ms,
            (unsigned int)num_tasks);
         break;
   }

   if(data_size_words > PING_LTE_MAX_WORDS)
   {
      data_size_words = PING_LTE_MAX_WORDS;
   }

   if( (!multiple_task_supported) || (num_tasks < 1) )
   {
      num_tasks=1;
   }

   rc = ping_lte_test_main(data_size_words, test_duration_ms,interval,num_tasks);
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




