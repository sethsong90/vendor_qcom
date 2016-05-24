/******************************************************************************
  @file  oncrpc_test_threads
  @brief Linux user-space oncrpc threads test

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
static char *VersionStr="1.1";


void * work_func(void *unused)
{
  printf("Exectuing workfunc \n");
  ping_mdm_rpc_null();
  pthread_exit(0);
  return (void *)0;
}

void thread_ping(void)
{
  pthread_t workthread;
  int status;
  pthread_create(&workthread, NULL, work_func, NULL);
  pthread_join(workthread, (void *)&status);
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
  int cnt=0;
  int nunTestsToRun = 1;



  printf("Oncrpc Test, version %s\n",VersionStr);
  printf("Usage: oncrpc_test <num iterations>, default = 1 \n\n");

  oncrpc_init();
  oncrpc_task_start();

  if( argc > 1 )
  {
    nunTestsToRun = atoi(argv[1]);
  }


  if( ping_mdm_rpc_null())
  {
    printf("Verified ping_mdm_rpc is available \n");
  }
  else
  {
    printf("Error ping_mdm_rpc is not availabe \n");
    printf("FAIL\n");
    return -1;
  }
  printf("\n\nRunning for %d iterations ...\n",nunTestsToRun);
  printf("ONCRPC TEST STARTED...\n");

  if( nunTestsToRun == 0 )
  {
    while( 1 )
    {
      printf("Launch RPC call, test#%d\n",cnt);
      cnt++;
      thread_ping();
    }
  }
  else
  {
    for( cnt=0;cnt < nunTestsToRun;cnt++ )
    {
      printf("Launch RPC call, test#%d\n",cnt);
      thread_ping();
    }
  }

  printf("ONCRPC TEST COMPLETE...\n");
  printf("PASS\n");
  return(0);
}
