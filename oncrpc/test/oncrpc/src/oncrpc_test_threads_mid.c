/******************************************************************************
  @file  oncrpc_test_threads mid
  @brief Linux user-space oncrpc multiple threads test to verify mid

  DESCRIPTION
  A bug was identified in the kernel, this test exposes this bug:

  A first write (A) being a multi-packet message is started.
  A second call to write (B) is called while the first write
  is in progress.  In the buggy version, the packet (B1) would
  use the same MID as (A1) and would cause message (A) to be
  corrupted at the far end when re-assembled.  Further more, the
  last packet of (A) would have a different mid, causing the far-end
  to never receive the completed message.

  This test exposes this bug by running 2 or more threads, one sending messages
  of 1 packet size, the other sending messages of 2 packet size.
  The command line options are <num iterations> <num threads>
  with at least 100 iterations and 2 threads, the test hits the case everytime.

  To be safe, the default options are 400 iterations.  If the test fails, one
  of the theads will hang and not complete. The test will hang. If it passes
  all threads complete.

  -----------------------------------------------------------------------------
  Copyright (c) 2009, Qualcomm Technologies, Inc., All rights reserved.
  -----------------------------------------------------------------------------
******************************************************************************/
/*=====================================================================
                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why
--------   ---      -------------------------------------------------------
11/16/09   kr       Updated code to synchronize main and client threads
09/25/09   rr       Initial version.

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
#define PING_MAX_WORDS (4096-16)

/*======================================================================
FUNCTION: ping_data_common
======================================================================*/
/*
Description: Function to initialize the data and start the ping test
*/
/*====================================================================*/
int ping_data_common(unsigned int data_size_words)
{
  uint32 sum=0,retsum;
  uint32 *data,i;
  int rc=0;
  /* Setup data block */
  if(data_size_words > PING_MAX_WORDS) {
     data_size_words = PING_MAX_WORDS;
  }

  data = malloc(data_size_words * sizeof(uint32));
  for( i=0;i<data_size_words;i++ )
  {
    data[i]=rand();
    sum=sum^data[i];
  }
  retsum = ping_mdm_data(data,data_size_words);
  if(sum!=retsum) {
     rc = -1;
  }
  free(data);
  return rc;

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
FUNCTION: work_func_double
======================================================================*/
/*
Description: Thread to perform a double packet ping test
*/
/*====================================================================*/
void * work_func_double(void *num_iterations)
{
  uint32 i;
  const uint32 num_words = 200;
  uint32 local_num_iterations = *(unsigned int *)num_iterations;
  printf("Client Thread ID:0x%08x Type - Double Packet %d words\n",
         (unsigned int)pthread_self(),(unsigned int)num_words);
  signal_main_thread();
  printf("Client Thread 0x%08x: signalled main thread and waiting.\n",(unsigned int)pthread_self());
  pthread_mutex_lock(&start_mutex);
  pthread_mutex_unlock(&start_mutex);
  for(i=0;i<local_num_iterations;i++)
  {
     ping_data_common(num_words);
  }
  printf("Client Thread completed 0x%08x\n",(unsigned int)pthread_self());
  pthread_exit(0);
  return (void *)0;
}

/*=======================================================================
FUNCTION: work_func_single
=======================================================================*/
/*
Description: Thread to perform a single packet ping test
*/
/*=====================================================================*/
void * work_func_single(void *num_iterations)
{
  uint32 i;
  uint32 local_num_iterations = *(unsigned int *)num_iterations;
  printf("Client Thread ID:0x%08x Type - Single Packet\n",(unsigned int)pthread_self());
  signal_main_thread();
  printf("Client Thread 0x%08x: signalled main thread and waiting.\n",(unsigned int)pthread_self());
  pthread_mutex_lock(&start_mutex);
  pthread_mutex_unlock(&start_mutex);
  for(i=0;i<local_num_iterations;i++)
  {
    ping_data_common(1);
  }
  printf("Client Thread completed 0x%08x\n",(unsigned int)pthread_self());
  pthread_exit(0);
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
void ping_thread_multiple(int num_threads, int num_iterations)
{
  pthread_t *workthreads;
  int i;
  workthreads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
  int status;

  pthread_mutex_lock(&start_mutex);

  for(i=0;i<num_threads;i++)
  {
    printf("Creating client thread %d\n",(unsigned int)i);
    if( (i%2) ==0 )
    {
       pthread_create(&workthreads[i], NULL, work_func_single, (void *)&num_iterations);
    }
    else
    {
       pthread_create(&workthreads[i], NULL,work_func_double, (void *)&num_iterations);

    }
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
  int numIterations = 100;
  int numThreads = 2;
  int maxThreads = 4;

  printf("\nOncrpc Test Threads Mid, version %s\n",VersionStr);
  printf("Usage: oncrpc_test_threads_mid <num iterations>[%d], <num threads>[%d] \n\n",numIterations,numThreads);

  oncrpc_init();
  oncrpc_task_start();

  switch(argc)
  {
    case 3:
      numThreads = atoi(argv[2]);
    case 2:
      numIterations = atoi(argv[1]);
      break;
   default:
      printf("Using defaults numTestsToRun:%d, numThreads:%d \n",numIterations,numThreads);
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

  printf("\nRunning for %d iterations with %d threads ...\n",numIterations, numThreads);
  ping_thread_multiple(numThreads,numIterations);

  pthread_cond_destroy(&sync_cond);

  printf("ONCRPC TEST WITH MULTIPLE THREADS COMPLETE...\n");
  printf("PASS\n");
  usleep(500000);
  return(0);
}
