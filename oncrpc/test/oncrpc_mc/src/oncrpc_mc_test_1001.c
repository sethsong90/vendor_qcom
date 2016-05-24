/******************************************************************************
  @file  oncrpc_mc_test_1001
  @brief Linux user-space oncrpc multi-process test to verify multi-core
         safety of ONCRPC Stack

  DESCRIPTION
  This test proceeds with a parent process launching a child process.
  Both of these processes perform ping_mdm_data() RPC calls concurrently
  and wait for the reply from the remote server.

  -----------------------------------------------------------------------------
  Copyright (c) 2010, Qualcomm Technologies, Inc., All rights reserved.
  -----------------------------------------------------------------------------
******************************************************************************/
/*=====================================================================

                     INCLUDE FILES FOR MODULE

======================================================================*/
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
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
static pid_t *child_pids = NULL;

static int set_timer_ms(struct itimerval *val, int ms)
{
  val->it_value.tv_sec = ms/1000;
  val->it_value.tv_usec = (ms * 1000) % 1000000;
  val->it_interval.tv_sec = 0;
  val->it_interval.tv_usec = 0;

  return setitimer(ITIMER_REAL, val, NULL);
}

void timer_handler(int a)
{
  test_duration_timer_expired = 1;
}

/*========================================================================
FUNCTION: ping_test
========================================================================*/
/*
Description: Main thread which is responsible for Ping tests
*/
/*========================================================================*/
int ping_test(int data_size_words, int test_duration_ms, int num_iterations)
{
  struct itimerval test_duration_timer;
  int i;
  int *status;
  int rc = 0;

  signal(SIGALRM, timer_handler);
  rc = set_timer_ms(&test_duration_timer, test_duration_ms);
  if(rc)
  {
    perror("settimer");
    return -1;
  }

  if(test_duration_ms)
  {
    while(!test_duration_timer_expired)
    {
      rc |= ping_data_common(data_size_words);
    }
  }
  else
  {
    for(i=0;i<num_iterations;i++)
    {
      rc |= ping_data_common(data_size_words);
    }
  }

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
  int i;
  int numIterations = 0;
  int numThreads = NUM_OF_CORES;
  int testDurationMs = 2000;
  int dataSizeWords = 200;
  int rc;
  int status = 0;

  printf("\nOncrpc Multi-Core Test 1001, version %s\n",VersionStr);
  printf("Usage: oncrpc_mc_test_1001 <num iterations> [%d] <test duration(ms)> [%d] <num threads> [%d] <data size words> [%d]\n\n",
          numIterations, testDurationMs, numThreads, dataSizeWords);

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

  if (numIterations > 0)
    testDurationMs = 0;

  printf("Running for numIterations:%d Test Duration(ms):%d, numThreads:%d dataSizeWords:%d\n",
          numIterations, testDurationMs, numThreads, dataSizeWords);

  child_pids = (pid_t *)malloc(numThreads * sizeof(pid_t));
  if(!child_pids)
    return -1;

  for(i = 0; i < numThreads - 1; i++)
  {
    child_pids[i] = fork();
    if(!child_pids[i])
    {
      printf("Child Process %x created\n", (unsigned int)getpid());
      free(child_pids);
      child_pids = NULL;
      break;
    }
  }

  oncrpc_init();
  oncrpc_task_start();
  if( ping_mdm_rpc_null())
  {
    printf("Verified ping_mdm_rpc is available \n");
  }
  else
  {
    printf("Error ping_mdm_rpc is not availabe \n");
    exit(-1);
  }

  rc = ping_test(dataSizeWords,testDurationMs,numIterations);
  printf("Process %x: Ping Test Completed\n", (unsigned int)getpid());

  if(rc)
    printf("Process %x: Ping Test Failed\n", (unsigned int)getpid());

  oncrpc_task_stop();
  oncrpc_deinit();

  if(!child_pids)
  {
    exit(rc);
  }

  for(i = 0; i < numThreads - 1; i++)
  {
    waitpid(child_pids[i], &status, 0);
    rc |= status;
  }

  printf("ONCRPC_MC_TEST_1001 COMPLETE...\n");
  if(!rc)
    printf("PASS\n");
  else
    printf("FAIL\n");

  free(child_pids);
  return(0);
}
