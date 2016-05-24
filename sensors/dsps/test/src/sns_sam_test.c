/*============================================================================
  @file sns_sam_test.c

  @brief
    Test file for SAM.

  Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
  ==========================================================================*/

#include "sensor1.h"
#include "sns_sam_amd_v01.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

sensor1_handle_s *my_handle;

pthread_mutex_t my_mutex;
pthread_cond_t my_cond;
int my_predicate = 0;
sensor1_error_e error;
sensor1_msg_header_s msg_hdr;
sigset_t set;
sns_sam_qmd_enable_req_msg_v01 *sam_qmd_enable_req;
sns_sam_qmd_disable_req_msg_v01 *sam_qmd_disable_req;
sns_sam_qmd_get_report_req_msg_v01 *sam_qmd_get_report_req;
void *sam_cancel_req;
int algo_instance_id;

int rcv_ind_count = 0;

void notify_data_cb( intptr_t data,
                     sensor1_msg_header_s *msg_hdr_ptr,
                     sensor1_msg_type_e msg_type,
                     void *msg_ptr )
{
  if( NULL == msg_hdr_ptr ) {
    printf("sam_test: received NULL msg_hdr_ptr!\n");
  } else {
    /*
    printf("sam_test: hdr.service_number: %u\n\thdr.msg_id: %d\n\t"
           "hdr.msg_type: %d\n\thdr.msg_size: %d\n\thdr.txn_id: %d\n",
           msg_hdr_ptr->service_number,
           msg_hdr_ptr->msg_id,
           msg_type,
           msg_hdr_ptr->msg_size,
           msg_hdr_ptr->txn_id );
    */
  }

  if( NULL == msg_ptr ) {
    printf("sam_test: received NULL msg_body!\n");
  }

  if( msg_type == SENSOR1_MSG_TYPE_RESP ) {
    // printf("sam_test: received RESP\n");
    pthread_mutex_lock( &my_mutex );
    my_predicate = 1;
    pthread_cond_signal( &my_cond );
    pthread_mutex_unlock( &my_mutex );

    if( msg_hdr_ptr->msg_id == SNS_SAM_AMD_ENABLE_RESP_V01 ) {
      sns_sam_qmd_enable_resp_msg_v01 *resp_ptr = msg_ptr;
      printf("Got ENABLE response, status = %d, id = %d\n",
             resp_ptr->resp.sns_result_t, resp_ptr->instance_id);
      algo_instance_id = resp_ptr->instance_id;
    } else if ( msg_hdr_ptr->msg_id == SNS_SAM_AMD_GET_REPORT_RESP_V01 ) {
      sns_sam_qmd_get_report_resp_msg_v01 *resp_ptr = msg_ptr;
      printf("Got GET_REPORT response, status = %d, id = %d, "
             "timestamp = %u, state = %d\n",
             resp_ptr->resp.sns_result_t, resp_ptr->instance_id,
             resp_ptr->timestamp, resp_ptr->state);
    } else if ( msg_hdr_ptr->msg_id == SNS_SAM_AMD_DISABLE_RESP_V01 ) {
      sns_sam_qmd_disable_resp_msg_v01 *resp_ptr = msg_ptr;
      printf("Got DISABLE response, status = %d, id = %d\n",
             resp_ptr->resp.sns_result_t, resp_ptr->instance_id);
    } else if ( msg_hdr_ptr->msg_id == SNS_SAM_AMD_CANCEL_RESP_V01 ) {
      sns_common_cancel_resp_msg_v01 *resp_ptr = msg_ptr;
      printf("Got CANCEL response, status = %d\n", resp_ptr->resp.sns_result_t);
    } else {
      printf("sam_test: Got INVALID response!!!\n");
    }
  } else if( msg_type == SENSOR1_MSG_TYPE_IND ) {
    // printf("sam_test: received IND\n");
    if ( msg_hdr_ptr->msg_id == SNS_SAM_AMD_REPORT_IND_V01 ) {
      sns_sam_qmd_report_ind_msg_v01 *ind_ptr = msg_ptr;
      rcv_ind_count++;
      printf("Got REPORT indication, id = %d, timestamp = %u, state = %d\n",
             ind_ptr->instance_id, ind_ptr->timestamp, ind_ptr->state);
    } else {
      printf("sam_test: Got INVALID indication!!!\n");
    }
  } else if( msg_type == SENSOR1_MSG_TYPE_BROKEN_PIPE ) {
    printf("sam_test: Got broken pipe!\n");
  } else {
    printf("sam_test: received INVALID MSG type!!!\n");
  }
  if( NULL != msg_ptr ) {
    sensor1_free_msg_buf( *((sensor1_handle_s**)data), msg_ptr );
  }
}

void test_init()
{
  sigemptyset( &set );
  sigaddset( &set, SIGALRM );
  sigprocmask( SIG_SETMASK, &set, NULL );

  error = sensor1_init();
  if( SENSOR1_SUCCESS != error ) {
    printf("sam_test: sensor1_init returned %d\n", error);
    exit(error);
  }

  pthread_mutex_init( &my_mutex, NULL );
  pthread_cond_init( &my_cond, NULL );
}

void test_open()
{
  error = sensor1_open( &my_handle,
                        notify_data_cb,
                        (intptr_t)(&my_handle) );
  if( SENSOR1_SUCCESS != error ) {
    printf("sam_test: sensor1_open returned %d\n", error);
    exit(error);
  }
}

void test_close()
{
  // close
  error = sensor1_close( my_handle );
  if( SENSOR1_SUCCESS != error ) {
    printf("sam_test: sensor1_close returned %d\n", error);
    exit(error);
  }
}

void send_enable_req()
{
  // alloc ENABLE request
  error = sensor1_alloc_msg_buf( my_handle,
                                 sizeof(sns_sam_qmd_enable_req_msg_v01),
                                 (void**)&sam_qmd_enable_req );
  if( SENSOR1_SUCCESS != error ) {
    printf("sam_test: sensor1_alloc_msg_buf returned %d\n", error);
    exit(error);
  }

  msg_hdr.service_number = SNS_SAM_AMD_SVC_ID_V01;
  msg_hdr.msg_id = SNS_SAM_AMD_ENABLE_REQ_V01;
  msg_hdr.msg_size = sizeof(sns_sam_qmd_enable_req_msg_v01);
  msg_hdr.txn_id = 1;
  sam_qmd_enable_req->report_period = (1<<16);  /* 1 second, Q16 format */

  //send ENABLE request
  error = sensor1_write( my_handle,
                         &msg_hdr,
                         sam_qmd_enable_req );
  if( SENSOR1_SUCCESS != error ) {
    printf("sam_test: sensor1_write returned %d\n", error);
    exit(error);
  }

  // wait for response
  pthread_mutex_lock( &my_mutex );
  while( my_predicate != 1 ) {
    error = pthread_cond_wait( &my_cond, &my_mutex );
  }
  my_predicate = 0;
  pthread_mutex_unlock( &my_mutex );
}

void send_get_report_req()
{
  // alloc GET_REPORT request
  error = sensor1_alloc_msg_buf( my_handle,
                                 sizeof(sns_sam_qmd_get_report_req_msg_v01),
                                 (void**)&sam_qmd_get_report_req );
  if( SENSOR1_SUCCESS != error ) {
    printf("sam_test: sensor1_alloc_msg_buf returned %d\n", error);
    exit(error);
  }

  msg_hdr.service_number = SNS_SAM_AMD_SVC_ID_V01;
  msg_hdr.msg_id = SNS_SAM_AMD_GET_REPORT_REQ_V01;
  msg_hdr.msg_size = sizeof(sns_sam_qmd_get_report_req_msg_v01);
  msg_hdr.txn_id = 2;
  sam_qmd_get_report_req->instance_id = algo_instance_id;

  //send GET_REPORT request
  error = sensor1_write( my_handle,
                         &msg_hdr,
                         sam_qmd_get_report_req );
  if( SENSOR1_SUCCESS != error ) {
    printf("sam_test: sensor1_write returned %d\n", error);
    exit(error);
  }

  // wait for response
  pthread_mutex_lock( &my_mutex );
  while( my_predicate != 1 ) {
    error = pthread_cond_wait( &my_cond, &my_mutex );
  }
  my_predicate = 0;
  pthread_mutex_unlock( &my_mutex );
}

void send_disable_req()
{
  // alloc DISABLE request
  error = sensor1_alloc_msg_buf( my_handle,
                                 sizeof(sns_sam_qmd_disable_req_msg_v01),
                                 (void**)&sam_qmd_disable_req );
  if( SENSOR1_SUCCESS != error ) {
    printf("sam_test: sensor1_alloc_msg_buf returned %d\n", error);
    exit(error);
  }

  msg_hdr.service_number = SNS_SAM_AMD_SVC_ID_V01;
  msg_hdr.msg_id = SNS_SAM_AMD_DISABLE_REQ_V01;
  msg_hdr.msg_size = sizeof(sns_sam_qmd_disable_req_msg_v01);
  msg_hdr.txn_id = 3;
  sam_qmd_disable_req->instance_id = algo_instance_id;

  //send DISABLE request
  error = sensor1_write( my_handle,
                         &msg_hdr,
                         sam_qmd_disable_req );
  if( SENSOR1_SUCCESS != error ) {
    printf("sam_test: sensor1_write returned %d\n", error);
    exit(error);
  }

  // wait for response
  pthread_mutex_lock( &my_mutex );
  while( my_predicate != 1 ) {
    error = pthread_cond_wait( &my_cond, &my_mutex );
  }
  my_predicate = 0;
  pthread_mutex_unlock( &my_mutex );
}

void send_cancel_req()
{
  // alloc CANCEL request
  error = sensor1_alloc_msg_buf( my_handle,
                                 0,
                                 (void**)&sam_cancel_req );
  if( SENSOR1_SUCCESS != error ) {
    printf("sam_test: sensor1_alloc_msg_buf returned %d\n", error);
    exit(error);
  }

  msg_hdr.service_number = SNS_SAM_AMD_SVC_ID_V01;
  msg_hdr.msg_id = SNS_SAM_AMD_CANCEL_REQ_V01;
  msg_hdr.msg_size = 0;  // cancel msg body is empty
  msg_hdr.txn_id = 4;

  //send CANCEL request
  error = sensor1_write( my_handle,
                         &msg_hdr,
                         sam_cancel_req );
  if( SENSOR1_SUCCESS != error ) {
    printf("sam_test: sensor1_write returned %d\n", error);
    exit(error);
  }

  // wait for response
  pthread_mutex_lock( &my_mutex );
  while( my_predicate != 1 ) {
    error = pthread_cond_wait( &my_cond, &my_mutex );
  }
  my_predicate = 0;
  pthread_mutex_unlock( &my_mutex );
}

int main( void )
{

  test_init();
  test_open();

  send_enable_req();
  send_get_report_req();

  // wait for some number of reports from SAM
  while (rcv_ind_count < 10) {
    sleep(1);
  }


  send_get_report_req();
  send_disable_req();
  send_cancel_req();

  sleep(1);

  test_close();

  printf("All tests passed\n");
  return 0;
}
