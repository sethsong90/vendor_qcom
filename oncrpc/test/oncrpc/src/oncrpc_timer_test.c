/******************************************************************************
  @file  oncrpc_timer_test
  @brief Linux user-space to verify oncrpc tiemrs

  DESCRIPTION
  Oncrpc test to verify oncrpc timers.
  
  -----------------------------------------------------------------------------
  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------
******************************************************************************/
/*=====================================================================
                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why 
--------   ---      ------------------------------------------------------- 
10/01/08   rr       Remove header files not needed.
05/22/08   rr       Remove dependencies on SND
04/04/08   rr       Fix for automation
02/01/08   rr       Remove initialization of oncrpc library, automatically done
                    now.
11/08/07   rr       Change to use snd.lib library
10/05/07   rr       Initial version

======================================================================*/ 
/*=====================================================================

                     INCLUDE FILES FOR MODULE

======================================================================*/
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "oncrpc.h"
#include "oncrpc_os.h"



/*=====================================================================
     External declarations
======================================================================*/
/* @todo RR, these should be in an include file somewhere */
extern void oncrpc_task_start(void);
extern void oncrpc_init(void);

/*=====================================================================
      Constants and Macros 
======================================================================*/
static char *VersionStr="1.2";
long ExpectedCallbackValue; 
static volatile int TestComplete=0;
static const int TestNumIterrations=2;
static const int ExpectedTestDurationSec=20;
static int ErrorCount=0;
extern void oncrpc_timer_info
(
  oncrpc_timer_ptr  ptimer         /*!< Pointer to a timer structure        */
);

unsigned int timer_cb_func_called_cnt = 0;
void timer_cb_func(unsigned long a)
{
  printf("Timer callback called with param: %lu \n",a);
  if(a != ExpectedCallbackValue)
  {
    ErrorCount++;
  }
  timer_cb_func_called_cnt ++;
}

#define TIMER1_EVENT_SIG  (1)
#define TIMER2_EVENT_SIG  (2)

/*===========================================================================
  FUNCTION  timer_test_thread
===========================================================================*/
/*!
@brief
  Timer test thread

@return
  0

@note
  - Dependencies
    - ONCRPC
  - Side Effects
*/
/*=========================================================================*/
void * timer_test_thread(void *unused)
{
  int i=0;
  oncrpc_thread_handle this_thread_handle;
  oncrpc_timer_ptr  timer_1;
  oncrpc_timer_ptr  timer_2;
  oncrpc_timer_ptr  timer_3;
  this_thread_handle = oncrpc_thread_handle_get();

  printf("\n\n");
  printf("-------------------------------------------\n");
  printf("Testing Timer signal interface \n");
  printf("-------------------------------------------\n");

  oncrpc_timer_new(&timer_1,TIMER1_EVENT_SIG);
  oncrpc_timer_new(&timer_2,TIMER2_EVENT_SIG);
    
  while(i < TestNumIterrations )
  {
    oncrpc_timer_set(timer_1,2020);
    oncrpc_timer_set(timer_2,4040);
    oncrpc_event_wait(this_thread_handle,TIMER1_EVENT_SIG);
    oncrpc_event_clr(this_thread_handle,TIMER1_EVENT_SIG);
    oncrpc_event_wait(this_thread_handle,TIMER2_EVENT_SIG);
    oncrpc_event_clr(this_thread_handle,TIMER2_EVENT_SIG);
    printf("oncrpc_timer_test *** Received timer signals *** Continuing... \n");
 
    i++;
  }
  printf("\n\n");
  printf("-------------------------------------------\n");
  printf("Testing Timer callback interface \n");
  printf("-------------------------------------------\n");
  timer_cb_func_called_cnt = 0;
  oncrpc_timer_new_cb(&timer_3,timer_cb_func,ExpectedCallbackValue);
  oncrpc_timer_set(timer_3,4040);

  while(timer_cb_func_called_cnt == 0)
  {
    sleep(1);
  }

  sleep(2);
  printf("Calling Free on Timer 1 \n");
  oncrpc_timer_free(timer_1);
  printf("Calling Free on Timer 2 \n");
  oncrpc_timer_free(timer_2);
  printf("Calling Free on Timer 3 \n");
  oncrpc_timer_free(timer_3);

  TestComplete=1;
  return 0;
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
  pthread_t  thread;
  oncrpc_init();
  oncrpc_task_start();
  int rc=0;
  int time_sec=0;

  ExpectedCallbackValue = rand();
  
  printf(" Test Version %s \n",VersionStr);
  pthread_create( &thread, 
                   NULL, 
                   timer_test_thread, 
                  (void *)0
                );

  while( (TestComplete == 0) && (time_sec < ExpectedTestDurationSec) )
  {  
      time_sec++;
      sleep(1);
  }
  
  if( (TestComplete != 1) || (ErrorCount != 0) )
  {
    rc = -1;
    if(TestComplete != 1)
    {
      printf("Test did not complete in the expected time \n");
    }
    if(0 != ErrorCount)
    {
      printf("Test had %d errors \n",ErrorCount);
    }
    printf("FAIL\n");
  }
  else
  {
    rc = 0;
    printf("PASS\n");
  }  
  return(rc);
}


