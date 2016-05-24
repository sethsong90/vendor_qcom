/******************************************************************************
  @file  ping_apps_client_common
  @brief common ping client functions
  DESCRIPTION
  Provide a more detailed description of the source file here
  INITIALIZATION AND SEQUENCING REQUIREMENTS
  Please detail this out here
  -----------------------------------------------------------------------------
  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------
******************************************************************************/

/*=====================================================================
                        EDIT HISTORY FOR MODULE

  $Id: $

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why
--------   ---      -------------------------------------------------------
09/29/2009   fr     Adapted this file for ping_apps test.
09/23/2008   zp     Factored out OS abstraction layer
04/18/2008   rr     Initial version

======================================================================*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include "oncrpc.h"
#include "ping_apps.h"
#include "ping_apps_rpc.h"
#include "ping_apps_client_common.h"


static volatile int test_completed = 0;
static struct itimerval  timer_test_completion;

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
  val->it_value.tv_usec = (ms*1000) % 1000000;
  val->it_interval.tv_sec = 0;
  val->it_interval.tv_usec = 0;

  return setitimer(ITIMER_REAL, val, NULL);
}

/*===========================================================================
  FUNCTION   test_completion_timer_cb_func
===========================================================================*/
/*!
@brief
  Test is complete
@return
  N/A

@note

*/
/*=========================================================================*/
static void test_completion_timer_cb_func(int a)
{
  int ret;
  test_completed=1;
  //printf("Timer callback called \n");

  ret = zero_timer(&timer_test_completion);
  if(ret)
  {
    perror("setitimer");
  }
}

/*===========================================================================
  FUNCTION  ping_apps_client_common_data_test
===========================================================================*/
/*!
@brief
  Common data test
  Runs a ping data test for with parameters:
  test_length : max number of rpc calls
  data_size_words: size of data block to send
  test_duration_ms: duration of test

@return
  0 for pass, -1 for fail
  returns statistics of test in  ping_client_results_type

@note
  - Dependencies  ping_client_results_type must be pre-allocated

  - Side Effects
*/
/*=========================================================================*/
int ping_apps_client_common_data_test(
  uint32 test_length,
  uint32 data_size_words,
  uint32 test_duration_ms,
  boolean verbose,
  ping_client_results_type *results
  )
{
  uint32 *data;
  data = malloc(data_size_words * sizeof(uint32));
  uint32 sum=0;
  uint32 i,retsum,errcnt=0;
  struct timespec time_info_start;
  struct timespec time_info_end;
  struct timespec time_info_last;
  struct timespec time_info_current;
  test_completed = 0;
  uint32 num_loops=0;
  uint32 byte_data=0;

  /* Setup data block */
  for( i=0;i<data_size_words;i++ )
  {
    byte_data = i&0xff;
    data[i]=byte_data << 24 | i;
    sum=sum^data[i];
  }

  /* If the test is specified with a test_duration_ms
     Then execute the test for this duration, if not,
     loop for test_length */

  //printf("Test Duration %d length %d\n",(unsigned int)test_duration_ms, (unsigned int)test_length);
  clock_gettime(CLOCK_REALTIME,&time_info_last);
  if( ( 0 == test_duration_ms ) && ( 0 == test_length ) )
  {
    clock_gettime(CLOCK_REALTIME,&time_info_start);
    while( 1 )
    {
      retsum = ping_apps_data(data,data_size_words);
      if( verbose )
      {
        clock_gettime(CLOCK_REALTIME,&time_info_current);
        if( time_info_current.tv_sec > (time_info_last.tv_sec ) )
        {
          time_info_last.tv_sec  = time_info_current.tv_sec ;
          printf("Rpc calls completed: %5d Num Errors:%d\n",(int)num_loops,(int)errcnt);
        }
      }

      num_loops++;

      if( retsum != sum )
      {
        errcnt++;
      }
    }
    clock_gettime(CLOCK_REALTIME,&time_info_end);
    ping_client_common_calc_stats_timespec(test_length,data_size_words,1,&time_info_start,&time_info_end,results);
  }
  else if( 0 == test_duration_ms )
  {
    clock_gettime(CLOCK_REALTIME,&time_info_start);
    for( i=0;i<test_length;i++ )
    {
      retsum = ping_apps_data(data,data_size_words);
      if( retsum != sum )
      {
        errcnt++;
      }
    }
    clock_gettime(CLOCK_REALTIME,&time_info_end);
    ping_client_common_calc_stats_timespec(test_length,data_size_words,1,&time_info_start,&time_info_end,results);
  }
  else
  {
    int ret;

    signal(SIGALRM, test_completion_timer_cb_func);
    ret = set_timer_ms(&timer_test_completion, test_duration_ms);
    if(ret) {
      perror("setitimer");
      errcnt++;
      free(data);
      return -errcnt;
    }

    if( test_length == 0 )
    {
      test_length = test_duration_ms;
    }


    /* Test ran for a fixed period of time*/

    while( (0 == test_completed) && (0 == errcnt) )
    {
      retsum = ping_apps_data(data,data_size_words);
      if( verbose )
      {

        clock_gettime(CLOCK_REALTIME,&time_info_current);
        if( time_info_current.tv_sec > (time_info_last.tv_sec ) )
        {
          time_info_last.tv_sec  = time_info_current.tv_sec ;
          printf("Rpc calls completed: %5d Num Errors:%d\n",(int)num_loops,(int)errcnt);
        }
      }

      num_loops++;
      if( retsum != sum )
      {
        errcnt++;
        printf("Data Sent, Size %u Bytes, Sum failed 0x%08x \n",(unsigned int)data_size_words*4, (int)sum);
      }
#ifndef FEATURE_PING_MDM_TIMER_BASED_TEST
      if( num_loops >= test_length )
      {
        test_completed = 1;
      }
#endif
    }

    if( errcnt == 0 )
    {
      ping_client_common_calc_stats(num_loops,data_size_words,1,test_duration_ms,results);
      results->rc = -errcnt;
    }
    else
    {
      memset(results,0,sizeof(ping_client_results_type));
      results->rc = -errcnt;
    }
    ret = zero_timer(&timer_test_completion);
    if(ret) {
      perror("setitimer");
      errcnt++;
      free(data);
      return -errcnt;
    }
  }
  free(data);

  return(- errcnt );
}


/*===========================================================================
  FUNCTION  ping_apps_client_common_data_test_random
===========================================================================*/
/*!
@brief
  Main body of ping_apps_client_common_data_test

@return
  0 for pass, -1 for fail

@note
  - Dependencies
    - N/A
  - Side Effects
*/
/*=========================================================================*/
int ping_apps_client_common_data_test_random(
  uint32 test_duration_ms,
  boolean verbose,
  ping_client_results_type *results)
{
  uint32 *data;
  uint32  max_data_size_words = (1024*16/4);
  uint32 data_size_words;
  data = malloc(max_data_size_words * sizeof(uint32));
  uint32 sum=0;
  uint32 i,retsum,errcnt=0;
  uint32 data_size_words_stats_min=0xFFFFFFFF;
  uint32 data_size_words_stats_max=0;
  uint32 data_size_words_stats_average=0;
  uint32 data_size_words_stats_total=0;
  uint32 data_size_words_stats_num_loops=0;
  //struct timespec time_info_start;
  //struct timespec time_info_end;
  struct timespec time_info_last;
  struct timespec time_info_current;
  uint32 test_length=0;
  test_completed = 0;
  uint32 num_loops=0;
  uint32 byte_data=0;

  /* Setup data block */
  for( i=0;i<max_data_size_words;i++ )
  {
    byte_data = i&0xff;
    data[i]=byte_data << 24 | i;
    sum=sum^data[i];
  }

  /* If the test is specified with a test_duration_ms
     Then execute the test for this duration, if not,
     loop for test_length */

  //printf("Test Duration %d length %d\n",(unsigned int)test_duration_ms, (unsigned int)test_length);
  clock_gettime(CLOCK_REALTIME,&time_info_last);
  {
    int ret;

    signal(SIGALRM, test_completion_timer_cb_func);
    ret = set_timer_ms(&timer_test_completion, test_duration_ms);
    if(ret) {
      perror("setitimer");
      errcnt++;
      free(data);
      return -errcnt;
    }

    if( test_length == 0 )
    {
      test_length = test_duration_ms;
    }


    /* Test ran for a fixed period of time*/

    while((0 == test_completed) && (0 == errcnt) )
    {
      data_size_words = rand() % max_data_size_words;
      retsum = ping_apps_data(data,data_size_words);

      data_size_words_stats_num_loops++;
      sum=0;
        for( i=0;i<data_size_words;i++ )
        {
          sum=sum^data[i];
        }

        if(data_size_words > data_size_words_stats_max)
        {
           data_size_words_stats_max = data_size_words;
        }

        if(data_size_words < data_size_words_stats_min)
        {
          data_size_words_stats_min = data_size_words;
        }
       data_size_words_stats_total += data_size_words;

      if( verbose )
      {
        clock_gettime(CLOCK_REALTIME,&time_info_current);
        if( time_info_current.tv_sec > (time_info_last.tv_sec ) )
        {
          time_info_last.tv_sec  = time_info_current.tv_sec ;

          if(num_loops > 0)
          {
            data_size_words_stats_average = data_size_words_stats_total /data_size_words_stats_num_loops;
          }
          else
          {
            data_size_words_stats_average = 0;

          }

          printf("Rpc calls completed: %5d Num Errors:%d bytes: min:%4d max:%4d avg:%4d\n",(int)num_loops,(int)errcnt,(int)data_size_words_stats_min*4,
                 (unsigned int)data_size_words_stats_max*4,(unsigned int)data_size_words_stats_average*4 );
          data_size_words_stats_min = 0xffffffff;
          data_size_words_stats_max = 0;
          data_size_words_stats_total  = 0;
          data_size_words_stats_num_loops=0;
        }
      }

      num_loops++;
      if( retsum != sum )
      {
        errcnt++;
        printf("Data Sent, Size %u Bytes, Sum failed 0x%08x \n",(unsigned int)data_size_words*4, (int)sum);
      }
#ifndef FEATURE_PING_MDM_TIMER_BASED_TEST
      if( num_loops >= test_length )
      {
        test_completed = 1;
      }
#endif
    }

    if( errcnt == 0 )
    {
      ping_apps_client_common_calc_stats(num_loops,data_size_words,1,test_duration_ms,results);
      results->rc = -errcnt;
    }
    else
    {
      memset(results,0,sizeof(ping_client_results_type));
      results->rc = -errcnt;
    }
  }
  free(data);

  return(- errcnt );
}


/*===========================================================================
  FUNCTION  ping_apps_client_common_calc_stats
===========================================================================*/
/*!
@brief
  Calculate Stats of test

@return
  returns statistics of test in  ping_client_results_type

@note
  - Dependencies  ping_client_results_type must be pre-allocated

  - Side Effects
*/
/*=========================================================================*/
void ping_apps_client_common_calc_stats(
  uint32 num_loops,
  uint32 data_size_words,
  uint32 ret_data_size_words,
  uint32 test_duration_ms,
  ping_client_results_type *results)
{
  uint32 num_bytes;
  num_bytes = (data_size_words*4 + 4)*num_loops;
  results->data_transfer_rate_kbps = (double)(num_bytes ) / (double)test_duration_ms;
  results->num_bytes_transfered = num_bytes;
  results->num_fwd_bytes_per_call = data_size_words*4;
  results->num_loops_completed = num_loops;
  results->num_ret_bytes_per_call = ret_data_size_words*4;
  results->rpc_call_duration_ms = (double)test_duration_ms/(double)num_loops;
  results->test_duration_ms=test_duration_ms;
}


/*===========================================================================
  FUNCTION  ping_client_common_calc_stats_timespec
===========================================================================*/
/*!
@brief
  Calculate Stats of test using timespec of start and end times

@return
  returns statistics of test in  ping_client_results_type

@note
  - Dependencies  ping_client_results_type must be pre-allocated

  - Side Effects
*/
/*=========================================================================*/
void ping_client_common_calc_stats_timespec(
  uint32 num_loops,
  uint32 data_size_words,
  uint32 ret_data_size_words,
  struct timespec *start_time_info,
  struct timespec *end_time_info,
  ping_client_results_type *results)
{

  double start_time_us;
  double end_time_us;
  double delta_us;
  double test_duration_ms;
  uint32 num_bytes;
  num_bytes = (data_size_words*4 + 4)*num_loops;

  end_time_us = (double)end_time_info->tv_nsec / (double)1000 + (double)end_time_info->tv_sec * (double)1000000;
  start_time_us = (double)start_time_info->tv_nsec / (double)1000 + (double)start_time_info->tv_sec * (double)1000000;

  delta_us = end_time_us - start_time_us;
  test_duration_ms = (delta_us)/1000;

  num_bytes = (data_size_words*4 + 4)*num_loops;
  results->data_transfer_rate_kbps = (double)(num_bytes ) / (double)test_duration_ms;
  results->num_bytes_transfered = num_bytes;
  results->num_fwd_bytes_per_call = data_size_words*4;
  results->num_loops_completed = num_loops;
  results->num_ret_bytes_per_call = ret_data_size_words*4;
  results->rpc_call_duration_ms = (double)test_duration_ms/(double)num_loops;
  results->test_duration_ms=test_duration_ms;

}
