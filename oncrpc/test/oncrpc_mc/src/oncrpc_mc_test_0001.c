/******************************************************************************
  @file  oncrpc_mc_test_0001
  @brief Linux user-space oncrpc multi-thread test to verify multi-core
         safety of ONCRPC Stack

  DESCRIPTION
  This test proceeds with the parallet threads of execution making a
  series of ping_mdm_data() RPC calls. This will test how the ONCRPC stack
  handles concurrent write to/read from the RPC router. This will also test
  how the RPC router handles the concurrent call and callback for the same
  ONCRPC device. The ONCRPC stack and RPC router are thus tested for:
	Concurrent reads and writes
	Handling of critical sections
	RPC packet fragmentation, interleaving and reassembly

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
static int test_duration_timer_expired = 0;

/*=======================================================================
FUNCTION: work_func
=======================================================================*/
/*
Description: Thread to perform ping test
*/
/*=====================================================================*/
void * work_func(void *args)
{
  uint32 i;
  int rc = 0;
  struct rpc_thread_arg *arg = (struct rpc_thread_arg *)args;
  printf("Client Thread ID:0x%08x\n",(unsigned int)pthread_self());
  if(arg->test_duration_ms)
  {
    while(!test_duration_timer_expired)
    {
      rc |= ping_data_common(arg->data_size_words);
    }
  }
  else
  {
    for(i=0;i<arg->num_iterations;i++)
    {
      rc |= ping_data_common(arg->data_size_words);
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
Description: Main thread which launches client threads
             responsible for Ping tests
*/
/*========================================================================*/
int ping_thread_multiple(int data_size_words, int num_threads, int test_duration_ms, int num_iterations)
{
  pthread_t *workthreads;
  struct timespec test_duration_timer;
  struct rpc_thread_arg arg;
  int i;
  int *status;
  int rc = 0;
  workthreads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));

  arg.num_iterations = num_iterations;
  arg.test_duration_ms = test_duration_ms;
  arg.data_size_words = data_size_words;

  for(i=0;i<num_threads;i++)
  {
    printf("Creating client thread %d\n",(unsigned int)i);
    pthread_create(&workthreads[i], NULL,work_func, (void *)&arg);
  }

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
    if (status)
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

  printf("\nOncrpc Multi-Core Test 0001, version %s\n",VersionStr);
  printf("Usage: oncrpc_mc_test_0001 <num iterations> [%d] <test duration(ms)> [%d] <num threads> [%d] <data size words> [%d]\n\n",
          numIterations, testDurationMs, numThreads, dataSizeWords);

  oncrpc_init();
  oncrpc_task_start();

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

  if (numIterations > 0)
    testDurationMs = 0;

  printf("Running for numIterations:%d Test Duration(ms):%d, numThreads:%d dataSizeWords:%d\n",
          numIterations, testDurationMs, numThreads, dataSizeWords);

  rc = ping_thread_multiple(dataSizeWords,numThreads,testDurationMs,numIterations);

  printf("ONCRPC_MC_TEST_0001 COMPLETE...\n");
  if(!rc)
    printf("PASS\n");
  else
    printf("FAIL\n");

  oncrpc_task_stop();
  oncrpc_deinit();
  return(0);
}
