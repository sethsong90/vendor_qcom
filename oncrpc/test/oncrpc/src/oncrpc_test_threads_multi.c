/******************************************************************************
  @file  oncrpc_test_threads multi
  @brief Linux user-space oncrpc multiple threads test

  DESCRIPTION
  Oncrpc test program for Linux user-space .

  -----------------------------------------------------------------------------
  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------
******************************************************************************/
/*=====================================================================
                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why
--------   ---      -------------------------------------------------------
11/17/09   kr       Updated code to synchronize main and client threads
08/18/08   rr       Initial version, ping client

======================================================================*/
/*=====================================================================

                     INCLUDE FILES FOR MODULE

======================================================================*/
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "oncrpc.h"
#include "ping_mdm_rpc.h"
#include "ping_mdm_rpc_rpc.h"

/*=====================================================================
     External declarations
======================================================================*/


/*=====================================================================
      Constants and Macros
======================================================================*/
static char *VersionStr="1.3";
static unsigned int sync_counter = 0;
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

/*=====================================================================
FUNCTION: work_func
======================================================================*/
/*
Description: Client thread function to perform the test
*/
/*====================================================================*/
void * work_func(void *num_iterations)
{
  uint32 i;
  uint32 local_num_iterations = *(unsigned int *)num_iterations;
  signal_main_thread();
  printf("Client Thread 0x%08x: signalled main thread and waiting\n",(unsigned int)pthread_self());
  pthread_mutex_lock(&start_mutex);
  pthread_mutex_unlock(&start_mutex);
  for(i=0;i<local_num_iterations;i++)
  {
    ping_mdm_rpc_null();
  }
  printf("Client Thread completed 0x%08x\n",(unsigned int)pthread_self());
  pthread_exit(0);
  return (void *)0;
}

/*========================================================================
FUNCTION: ping_thread_multiple
========================================================================*/
/*
Description: Main thread which launches client threads responsible for
             performing ping test
*/
/*========================================================================*/
void ping_thread_multiple(int num_threads, int num_iterations)
{
  pthread_t *workthreads;
  uint32 i;
  workthreads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
  int status;

  pthread_mutex_lock(&start_mutex);
  for(i=0;i<num_threads;i++)
  {
    printf("Creating client thread %d\n",(unsigned int)i);
    pthread_create(&workthreads[i], NULL, work_func, (void *)&num_iterations);
  }

  printf("Waiting for client threads to be initialized.\n");
  pthread_mutex_lock(&sync_mutex);
  while(sync_counter < num_threads)
  {
    pthread_cond_wait(&sync_cond, &sync_mutex);
  }
  pthread_mutex_unlock(&sync_mutex);

  printf("\nSignal client threads to start their tests.\n");

  pthread_mutex_unlock(&start_mutex);
  printf("Waiting for client threads to complete \n");
  for(i=0;i<num_threads;i++)
  {
    pthread_join(workthreads[i],NULL);
  }
  printf("All client threads joined \n");
  free(workthreads);
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
  int numTestsToRun = 5;
  int numThreads = 5;
  int maxThreads = 15;

  printf("Oncrpc Test Threads Multi, version %s\n",VersionStr);
  printf("Usage: oncrpc_test_threads_multi <num iterations>[10], <num threads>[10] \n\n");

  oncrpc_init();
  oncrpc_task_start();

  switch(argc)
  {
    case 3:
      numThreads = atoi(argv[2]);
    case 2:
      numTestsToRun = atoi(argv[1]);
      break;
   default:
      printf("Using defaults numTestsToRun:%d, numThreads:%d \n",numTestsToRun,numThreads);
      break;
  }

  if(numThreads > maxThreads)
  {
     printf("Maximum number of threads supported is %d, you specified %d, setting to %d \n",maxThreads,numThreads,maxThreads);
     numThreads = maxThreads;
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

  printf("\n\nRunning for %d iterations with %d threads ...\n",numTestsToRun, numThreads);
  ping_thread_multiple(numThreads,numTestsToRun);

  pthread_cond_destroy(&sync_cond);

  printf("ONCRPC TEST WITH MULTIPLE THREADS COMPLETE...\n");
  printf("PASS\n");
  usleep(500000);
  return(0);
}
