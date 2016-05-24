/******************************************************************************
  @file  oncrpc_mc_test_0008
  @brief Linux user-space oncrpc multi-thread test to verify multi-core
         safety of ONCRPC Stack

  DESCRIPTION
  This test proceeds with a main thread launching four threads with two of
  them invoking 6 ping_mdm_data RPC calls and the other two invoking
  oem_rapi_streaming_function RPC calls. The first thread issues a single
  packet ping_mdm_data RPC call. The second issues thread a multipacket
  ping_mdm_data RPC call. The third thread issues a single packet
  oem_rapi_streaming_function RPC call. The fourth thread issues a multipacket
  oem_rapi_streaming_function RPC call. Since all of these threads share the
  ONCRPC stack, this test case will test the ONCRPC stack for concurrent writes.
  Also, the threads invoking the same type of RPC call will test the concurrent
  writes to RPC router. Therefore, this test case will test:
        Concurrent write access in ONCRPC stack and RPC router
        Critical section handling in ONCRPC stack

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

/*=======================================================================
FUNCTION: work_func_single
=======================================================================*/
/*
Description: Thread to perform a single packet ping test
*/
/*=====================================================================*/
void * work_func_single(void *args)
{
  uint32 i;
  int rc = 0;
  struct rpc_thread_arg *arg = (struct rpc_thread_arg *)args;

  printf("Client Thread ID:0x%08x - Type Ping Single Packet\n",(unsigned int)pthread_self());
  signal_main_thread();
  pthread_mutex_lock(&start_mutex);
  pthread_mutex_unlock(&start_mutex);

  if(arg->test_duration_ms)
  {
    while(!test_duration_timer_expired)
    {
      rc |= ping_data_common(1);
    }
  }
  else
  {
    for(i=0;i<arg->num_iterations;i++)
    {
      rc |= ping_data_common(1);
    }
  }
  printf("Client Thread completed 0x%08x\n",(unsigned int)pthread_self());
  pthread_exit((void *)rc);
  return (void *)0;
}

/*=======================================================================
FUNCTION: work_func_double
=======================================================================*/
/*
Description: Thread to perform a double packet ping test
*/
/*=====================================================================*/
void * work_func_double(void *args)
{
  uint32 i;
  int rc = 0;
  uint32 data_size_words = 200;
  struct rpc_thread_arg *arg = (struct rpc_thread_arg *)args;

  printf("Client Thread ID:0x%08x - Type Ping Double Packet\n",(unsigned int)pthread_self());
  signal_main_thread();
  pthread_mutex_lock(&start_mutex);
  pthread_mutex_unlock(&start_mutex);

  if(arg->test_duration_ms)
  {
    while(!test_duration_timer_expired)
    {
      rc |= ping_data_common(data_size_words);
    }
  }
  else
  {
    for(i=0;i<arg->num_iterations;i++)
    {
      rc |= ping_data_common(data_size_words);
    }
  }
  printf("Client Thread completed 0x%08x\n",(unsigned int)pthread_self());
  pthread_exit((void *)rc);
  return (void *)0;
}


/*=======================================================================
FUNCTION: work_func_oem_rapi
=======================================================================*/
/*
Description: Thread to perform a oem_rapi test
*/
/*=====================================================================*/
void * work_func_oem_rapi(void *args)
{
  uint32 i;
  int rc = 0;
  uint32 data_size_words = 200;
  struct rpc_thread_arg *arg = (struct rpc_thread_arg *)args;

  printf("Client Thread ID:0x%08x - Type Oem Rapi\n",(unsigned int)pthread_self());
  signal_main_thread();
  pthread_mutex_lock(&start_mutex);
  pthread_mutex_unlock(&start_mutex);

  if(arg->test_duration_ms)
  {
    while(!test_duration_timer_expired)
    {
      rc |= oem_rapi_streaming_function_Test(data_size_words);
    }
  }
  else
  {
    for(i=0;i<arg->num_iterations;i++)
    {
      rc |= oem_rapi_streaming_function_Test(data_size_words);
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
             responsible for Single & Double Packet Ping tests
*/
/*========================================================================*/
int ping_thread_multiple(int num_threads, int test_duration_ms, int num_iterations)
{
  pthread_t *workthreads;
  struct timespec test_duration_timer;
  struct rpc_thread_arg arg;
  int i;
  workthreads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
  int *status;
  int rc = 0;

  arg.num_iterations = num_iterations;
  arg.test_duration_ms = test_duration_ms;

  pthread_mutex_lock(&start_mutex);

  for(i=0;i<num_threads;i=i+3)
  {
    pthread_create(&workthreads[i], NULL,work_func_single, (void *)&arg);
    pthread_create(&workthreads[i+1], NULL,work_func_double, (void *)&arg);
    pthread_create(&workthreads[i+2], NULL, work_func_oem_rapi, (void *)&arg);
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
  if (test_duration_ms)
  {
    test_duration_timer.tv_sec = test_duration_ms/1000;
    test_duration_timer.tv_nsec = (test_duration_ms%1000) * 1000000;
    nanosleep(&test_duration_timer, NULL);
    test_duration_timer_expired = 1;
  }

  for(i=0;i<num_threads;i++)
  {
    pthread_join(workthreads[i],(void **)&status);
    if(status)
      rc = -1;
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
  int rc = 0;

  printf("\nOncrpc Multi-Core Test 0008, version %s\n",VersionStr);
  printf("Usage: oncrpc_mc_test_0008 <num iterations> [%d] <test duration(ms)> [%d] <num threads> [%d]\n\n",
          numIterations, testDurationMs, numThreads);

  oncrpc_init();
  oncrpc_task_start();
  oem_rapicb_app_init();

  switch(argc)
  {
    case 4:
      numThreads = atoi(argv[3]);
    case 3:
      testDurationMs = atoi(argv[2]);
    case 2:
      numIterations = atoi(argv[1]);
      break;
   default:
      printf("Using defaults numTestsToRun:%d Test Duration:%d numThreads:%d\n",
              numIterations, testDurationMs, numThreads);
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
    printf("Verified oem_rapi is available \n");
  }
  else
  {
    printf("Error oem_rapi is not availabe \n");
    return -1;
  }

  if (numIterations > 0)
    testDurationMs = 0;

  if(numThreads < 3)
    numThreads = 3;
  else
    numThreads = (numThreads - (numThreads%3));

  printf("Running for numIterations:%d Test Duration(ms):%d, numThreads:%d\n",
          numIterations, testDurationMs, numThreads);

  rc = ping_thread_multiple(numThreads,testDurationMs,numIterations);

  pthread_cond_destroy(&sync_cond);
  printf("ONCRPC_MC_TEST_0008 COMPLETE...\n");
  if(!rc)
    printf("PASS\n");
  else
    printf("FAIL\n");

  oncrpc_task_stop();
  oncrpc_deinit();
  return(0);
}
