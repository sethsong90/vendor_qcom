/******************************************************************************
  @file  oncrpc_mc_test_0005
  @brief Linux user-space oncrpc multi-thread test to verify multi-core
         safety of ONCRPC Stack

  DESCRIPTION
  This test proceeds by launching two threads, within the same process,
  which execute ping_mdm_data RPC calls concurrently with one thread issuing
  a single packet-based RPC call, and the other thread issuing a
  multipacket-based RPC call. Since both the threads share the ONCRPC stack,
  this will test how the ONCRPC stack handles two concurrent different-sized
  packets writes to it. Also, since both the threads write to the same ONCRPC
  device, this will test how the RPC router handles two different sized packet
  writes to a single device. This will also test the ONCRPC stack for critical
  section handling. Hence, this test case will test for:
          Concurrent write access in ONCRPC stack and RPC router
          Handling of critical sections in ONCRPC stack


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

  printf("Client Thread ID:0x%08x - Type Single Packet\n",(unsigned int)pthread_self());
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

  printf("Client Thread ID:0x%08x - Type Double Packet\n",(unsigned int)pthread_self());
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

  for(i=0;i<num_threads;i++)
  {
    printf("Creating client thread %d\n",(unsigned int)i);
    if(i%2)
      pthread_create(&workthreads[i], NULL,work_func_single, (void *)&arg);
    else
      pthread_create(&workthreads[i], NULL,work_func_double, (void *)&arg);
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

  printf("\nOncrpc Multi-Core Test 0005, version %s\n",VersionStr);
  printf("Usage: oncrpc_mc_test_0005 <num iterations> [%d] <test duration(ms)> [%d] <num threads> [%d]\n\n",
          numIterations, testDurationMs, numThreads);

  oncrpc_init();
  oncrpc_task_start();

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

  if (numIterations > 0)
    testDurationMs = 0;

  printf("Running for numIterations:%d Test Duration(ms):%d, numThreads:%d\n",
          numIterations, testDurationMs, numThreads);

  rc = ping_thread_multiple(numThreads,testDurationMs,numIterations);

  pthread_cond_destroy(&sync_cond);
  printf("ONCRPC_MC_TEST_0005 COMPLETE...\n");
  if(!rc)
    printf("PASS\n");
  else
    printf("FAIL\n");

  oncrpc_task_stop();
  oncrpc_deinit();
  return(0);
}
