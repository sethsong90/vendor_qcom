/******************************************************************************
  @file  oncrpc_mc_test_0007
  @brief Linux user-space oncrpc multi-thread test to verify multi-core
         safety of ONCRPC Stack

  DESCRIPTION
  This test proceeds with a main thread issuing a ping_mdm_register_data_cb
  RPC call followed by an oem_rapi_streaming_function RPC call. The callback
  from the ping_mdm_server leads to a read operation, while the
  oem_rapi_streaming_function performs the forward call. The ONCRPC stack
  is thus tested for:
          Concurrent read and write access

  -----------------------------------------------------------------------------
  Copyright (c) 2010, Qualcomm Technologies, Inc., All rights reserved.
  -----------------------------------------------------------------------------
******************************************************************************/
/*=====================================================================

                     INCLUDE FILES FOR MODULE

======================================================================*/
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "oncrpc.h"
#include "oncrpc_mc_common.h"
#include "ping_mdm_rpc.h"
#include "ping_mdm_rpc_rpc.h"

/*=====================================================================
     External declarations
======================================================================*/


/*=====================================================================
      Constants and Macros
======================================================================*/
static char *VersionStr="1.0";
static unsigned int sync_counter = 0;
static int test_duration_timer_expired = 0;
pthread_mutex_t start_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sync_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sync_cond = PTHREAD_COND_INITIALIZER;

static int registerd_state = 0;
static int num_cb_expected = 0;
static int num_cb_received = 0;
static int errcnt = 0;

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

/*===========================================================================
  FUNCTION  timer_thread_func
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
void *timer_thread_func(void *arg)
{
   struct timeval timeout;
   uint32 timeout_ms = *((uint32 *)arg);

   timeout.tv_sec = timeout_ms/1000;
   timeout.tv_usec = (timeout_ms % 1000) * 1000;
   select(0, NULL, NULL, NULL, &timeout);

   set_signal(PING_MDM_CLIENT_TEST_END_SIG);
   test_duration_timer_expired = 1;
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

   /* Wake up main thread */
   set_signal(PING_MDM_CLIENT_TEST_CALLBACK_SIG);
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
   if( registerd_state == 1 )
   {
      ping_mdm_un_register_data_cb(ping_mdm_data_cb_function);
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
         ping_mdm_data_cb_function,
         num,
         size_words,
         interval_ms,
         split_packet_ms);

   }
   else
   {
      printf("Auto-UnRegister with server \n");
      ping_mdm_un_register_data_cb(ping_mdm_data_cb_function);
      ping_mdm_register_data_cb(
         ping_mdm_data_cb_function,
         num,
         size_words,
         interval_ms,
         split_packet_ms);
   }
   registerd_state = 1;
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

/*====================================================================
FUNCTION: signal_main_thread
====================================================================*/
/*
Description: Function for client threads to signal main thread of their
             initialization
*/
/*==================================================================*/
void signal_main_thread()
{
  pthread_mutex_lock(&sync_mutex);
  sync_counter++;
  pthread_cond_signal(&sync_cond);
  pthread_mutex_unlock(&sync_mutex);
}

/*=====================================================================
FUNCTION: work_func_cb
======================================================================*/
/*
Description: Thread to perform a callback ping test
*/
/*====================================================================*/
void * work_func_cb(void *args)
{
   uint32 num;
   struct rpc_thread_arg *arg = (struct rpc_thread_arg *)args;
   int sigs=0;
   int rc = 0;
   uint32 interval_ms = 1;
   int timeout_ms = arg->test_duration_ms;
   pthread_t timer_thread;

   num=(arg->test_duration_ms + arg->num_iterations)/interval_ms;
   if (num == 0)
   {
     num = 1;
   }
   num_cb_expected = num;

   printf("Client Thread ID:0x%08x\n",(unsigned int)pthread_self());
   signal_main_thread();
   pthread_mutex_lock(&start_mutex);
   pthread_mutex_unlock(&start_mutex);

   pthread_create(&timer_thread, NULL, timer_thread_func, (void *)&timeout_ms);

   ping_mdm_client_register(num,arg->data_size_words,interval_ms,arg->num_threads);
   printf("Waiting for ping...\n");
   while( (sigs & PING_MDM_CLIENT_TEST_END_SIG) == 0 )
   {
      sigs = wait_for();
      if( sigs & PING_MDM_CLIENT_TEST_END_SIG )
      {
         ping_mdm_client_data_unregister();
      }
      else if( sigs & PING_MDM_CLIENT_RESTART_SIG )
      {
         printf("Waking up on server exit! Re-register\n");

         /* clear flag */
         clr_flag(PING_MDM_CLIENT_RESTART_SIG);

         registerd_state = 0;
         oncrpc_register_server_exit_notification_cb( PING_MDM_RPCPROG, PING_MDM_RPCVERS, cleanup_cb, NULL);
         ping_mdm_client_register(num_cb_expected - num_cb_received,arg->data_size_words,interval_ms,arg->num_threads);
      }
      /* clear flag */
      clr_flag(sigs);
   }
   printf("\n");

   ping_mdm_client_data_unregister();

   if( num_cb_received == 0 )
   {
      printf("No callbacks recevied. FAIL\n");
      rc = -1;
   }
   else
   {
      if( errcnt != 0 )
      {
        printf("Call back thread FAIL\n");
        rc = -1;
      }
   }

   printf("Client Thread completed 0x%08x\n",(unsigned int)pthread_self());
   pthread_exit((void *)rc);
   return((void *)0);
}

/*=======================================================================
FUNCTION: work_func_forward
=======================================================================*/
/*
Description: Thread to perform a forward oem_rapi test
*/
/*=====================================================================*/
void * work_func_forward(void *args)
{
  uint32 i;
  int rc = 0;
  struct rpc_thread_arg *arg = (struct rpc_thread_arg *)args;

  printf("Client Thread ID:0x%08x\n",(unsigned int)pthread_self());
  signal_main_thread();
  pthread_mutex_lock(&start_mutex);
  pthread_mutex_unlock(&start_mutex);

  if(arg->test_duration_ms)
  {
    while(!test_duration_timer_expired)
    {
      rc |= oem_rapi_streaming_function_Test(arg->data_size_words);
    }
  }
  else
  {
    for(i=0;i<arg->num_iterations;i++)
    {
      rc |= oem_rapi_streaming_function_Test(arg->data_size_words);
    }
  }
  printf("Client Thread completed 0x%08x\n",(unsigned int)pthread_self());
  pthread_exit((void *)rc);
  return (void *)0;
}

/*========================================================================
FUNCTION: ping_thread_multiple
========================================================================*/
/*
Description: Main thread which launches 2 types of client threads
             responsible for forward and callback Ping tests
*/
/*========================================================================*/
int ping_thread_multiple(int data_size_words, int num_threads, int test_duration_ms, int num_iterations)
{
  pthread_t *workthreads;
  struct rpc_thread_arg arg;
  int i;
  workthreads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
  int *status;
  int rc = 0;

  arg.data_size_words = data_size_words;
  arg.num_iterations = num_iterations;
  arg.test_duration_ms = test_duration_ms;
  arg.num_threads = 1;

  pthread_mutex_lock(&start_mutex);

  pthread_create(&workthreads[0], NULL, work_func_cb, (void *)&arg);
  for(i=1;i<num_threads;i++)
  {
    printf("Creating client thread %d\n",(unsigned int)i);
    pthread_create(&workthreads[i], NULL,work_func_forward, (void *)&arg);
  }

  printf("Waiting for client threads to be initialized.\n");
  pthread_mutex_lock(&sync_mutex);
  while(sync_counter < num_threads)
  {
    pthread_cond_wait(&sync_cond, &sync_mutex);
  }
  pthread_mutex_unlock(&sync_mutex);

  printf("\nSignal client threads to start their tests\n");

  pthread_mutex_unlock(&start_mutex);
  printf("Waiting for client threads to complete \n");
  for(i=0;i<num_threads;i++)
  {
    pthread_join(workthreads[i], (void **)&status);
    if(status)
    {
      rc = -1;
    }
  }
  printf("All client threads joined \n");
  free(workthreads);
  return rc;
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
  int numIterations = 0;
  int numThreads = NUM_OF_CORES;
  int testDurationMs = 2000;
  int dataSizeWords = 200;
  int rc;

  printf("\nOncrpc Multi-Core Test 0007, version %s\n",VersionStr);
  printf("Usage: oncrpc_mc_test_0007 <num iterations> [%d] <test duration(ms)> [%d] <num threads> [%d] <data size words> [%d]\n\n",
          numIterations, testDurationMs, numThreads, dataSizeWords);

  oncrpc_init();
  oncrpc_task_start();
  ping_mdm_rpccb_app_init();
  oem_rapicb_app_init();

  init_signaling();

  switch(argc)
  {
    case 5:
      dataSizeWords = atoi(argv[4]);
    case 4:
      numThreads = atoi(argv[3]);
    case 3:
      testDurationMs = atoi(argv[2]);
    case 2:
      numIterations = atoi(argv[1]);
      break;
   default:
      printf("Using defaults numTestsToRun:%d Test Duration:%d numThreads:%d dataSizeWords:%d\n",
              numIterations, testDurationMs, numThreads, dataSizeWords);
      break;
  }

  if( ping_mdm_rpc_null())
  {
    printf("Verified ping_mdm_rpc is available \n");
  }
  else
  {
    printf("Error ping_mdm_rpc is not availabe \n");
    return -1;
  }

  if( oem_rapi_null())
  {
    printf("Verified oem_rapi is available\n");
  }
  else
  {
    printf("Error oem_rapi is not available\n");
    return -1;
  }

  oncrpc_register_server_exit_notification_cb( PING_MDM_RPCPROG, PING_MDM_RPCVERS, cleanup_cb, NULL);

  if (numIterations > 0)
    testDurationMs = 0;

  printf("Running for numIterations:%d Test Duration(ms):%d, numThreads:%d dataSizeWords:%d\n",
          numIterations, testDurationMs, numThreads, dataSizeWords);

  rc = ping_thread_multiple(dataSizeWords,numThreads,testDurationMs,numIterations);

  pthread_cond_destroy(&sync_cond);
  printf("ONCRPC_MC_TEST_0007 COMPLETE...\n");
  if(!rc)
    printf("PASS\n");
  else
    printf("FAIL\n");

  oncrpc_task_stop();
  oncrpc_deinit();
  return(0);
}
