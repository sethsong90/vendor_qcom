#define __SNS_MODULE__ SNS_SMR

#define SNS_SMR_LA_C
/*============================================================================
  @file sns_smr_la.c

  @brief
  This file contains the implementation of SMR internal
  functions for Linux Andorid

  Copyright (c) 2010-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
============================================================================*/

/*============================================================================

                                INCLUDE FILES

============================================================================*/
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "oi_support_init.h"
#include "sns_common.h"
#include "sns_debug_api.h"
#include "sns_debug_str.h"
#include "sns_em.h"
#include "sns_init.h"
#include "sns_log_api.h"
#include "sns_log_types.h"
#include "sns_main.h"
#include "sns_memmgr.h"
#include "sns_osa.h"
#include "sns_pwr.h"
#include "sns_queue.h"
#include "sns_smr.h"
#include "sns_smr_priv.h"

/* definition of millisecond sleep */
#define sns_os_msleep(x) usleep((x)*1000)

/* Amount of time to sleep between tries to open the SMD pipe to the DSPS */
#define SNS_SMR_SMD_OPEN_RETRY_MS 100
/* Maximium amount of time to retry */
#define SNS_SMR_SMD_OPEN_RETRY_MAX_MS 5001
/* Maximum message ID value. This should be updated if any service ever has more
 * than this number of messages */
#define SNS_SMR_MAX_MSG_ID 255

/* Time between receiving a packet from SMD and releasing a wakelock.
 * Should be greater than the maximum amount of time all clients need to
 * process the event. */
#define SNS_SMR_WL_TIMEOUT_USEC 1500000 /* 1500ms */

#define SNS_SMD_NAME_MAX_LEN 25

/* Android defines PRI.PTR incorrectly. Fix it here */
#ifdef SNS_LA
#  undef PRIxPTR
#  define PRIxPTR "x"
#endif /* SNS_LA */

/*===========================================================================

                              EXTERNAL VARIABLES

===========================================================================*/

/*===========================================================================

                         INTERNAL DEFINITION AND TYPES

===========================================================================*/

/*===========================================================================

                              STATIC VARIABLES

===========================================================================*/

/**
 * When this timer expires, it is time to release the wakelock
 */
static sns_em_timer_obj_t smr_apps_wl_tmr_ptr;

/**
 * Used to disable writes when crashing
 */
static volatile int sns_smr_la_write_disabled = 0;

/** Pipe to wake up SMR reader thread when exiting */
static int sns_smr_la_wakeup_pipe[2];

/*===========================================================================

                         INTERNAL FUNCTION PROTOTYPES

===========================================================================*/

/*===========================================================================

  FUNCTION:   smr_print_heap_summary

===========================================================================*/
/*!
  @brief This function prints the summary of the heap memory

  @detail

  @param[i] none

  @return
   None
*/
/*=========================================================================*/
#ifdef USE_NATIVE_MALLOC
void smr_print_heap_summary (void)
{
  return;
}
#else
#define NUM_BLOCK 10
extern uint32_t PoolFreeCnt[];
extern const OI_MEMMGR_POOL_CONFIG memmgrPoolTable[];

void smr_print_heap_summary (void)
{
  static bool     is_block_counted = false;
  static uint32_t tot_block_cnt = 0;
  uint32_t        i, tot_free_cnt = 0;

  SNS_PRINTF_STRING_FATAL_0(SNS_DBG_MOD_APPS_SMR,
                            "prints heap_summary");
  if ( !is_block_counted )
  {
    for ( i = 0; i < NUM_BLOCK; i++)
    {
      tot_block_cnt += memmgrPoolTable[i].numBlocks;
    }
    is_block_counted = true;
  }
  for ( i = 0; i < NUM_BLOCK; i++)
  {
    tot_free_cnt += PoolFreeCnt[i];
    SNS_PRINTF_STRING_FATAL_3(SNS_DBG_MOD_APPS_SMR,
                              "Free Cnt[%5d] = %d/%d",
                              memmgrPoolTable[i].blockSize, PoolFreeCnt[i], memmgrPoolTable[i].numBlocks);
  }
  SNS_PRINTF_STRING_FATAL_2(SNS_DBG_MOD_APPS_SMR,
                            "Total Free Cnt  = %d/%d", tot_free_cnt, tot_block_cnt);
}
#endif

/*===========================================================================

  FUNCTION:   smr_print_queue_summary

===========================================================================*/
/*!
  @brief This function prints the summary of SMR queues

  @detail

  @param[i] none

  @return
   None
*/
/*=========================================================================*/
static void smr_print_queue_summary (void)
{
#if !defined(SNS_QMI_ENABLE)  // TODO: A more elegant approach
  uint32_t        i;

  SNS_PRINTF_STRING_FATAL_0(SNS_DBG_MOD_APPS_SMR,
                            "prints msg cnt in queue: module");
  /* prints SMR queue summary  */
  for ( i = 0; i < SNS_MODULE_CNT; i++ )
  {
    uint32_t que_cnt;
    sns_smr_msg_pri_e      pri_idx;
    smr_que_entry_s *que_entry_ptr;
    que_entry_ptr = &sns_smr.smr_que_tb[i];

#ifdef SMR_PRIORITY_QUE_ON
    for ( pri_idx = SNS_SMR_MSG_PRI_LOW; pri_idx <= SNS_SMR_MSG_PRI_HIGH; pri_idx++)
#else
    for ( pri_idx = SNS_SMR_MSG_PRI_LOW; pri_idx <= SNS_SMR_MSG_PRI_LOW; pri_idx++)
#endif
    {
      que_cnt = sns_q_cnt (que_entry_ptr->q_ptr[pri_idx]);
      if ( que_cnt )
      {
        SNS_PRINTF_STRING_FATAL_3(SNS_DBG_MOD_APPS_SMR,
                              "msg cnt in queue: module = %d, pri=%d, cnt=%d", i, pri_idx, que_cnt);
      }
    }
  }
#endif /* !defined(SNS_QMI_ENABLED) */
}

/*===========================================================================

  FUNCTION:   smr_print_queue_and_heap_summary

===========================================================================*/
/*!
  @brief This function prints the summary of SMR queues and the heap memory

  @detail

  @param[i] none

  @return
   None
*/
/*=========================================================================*/
int smr_print_queue_and_heap_summary (void)
{
  smr_print_heap_summary();
  smr_print_queue_summary();
  return 0;
}

/*===========================================================================

  FUNCTION:   smr_apps_log_the_packet

===========================================================================*/
/*!
  @brief This function logs the message

  @detail

  @param[i] header_ptr: A pointer to the message header
  @param[i] encoded:  Flag which tells if the message is encoded or decoded

  @return
   None
*/
/*=========================================================================*/
void smr_apps_log_the_packet(const sns_smr_header_s *header_ptr, boolean encoded)
{
#if 0
  sns_err_code_e sns_err;

  if ( 1 ) /* log filter is enabled */
  {
    if (header_ptr->dst_module != SNS_MODULE_APPS_DIAG)
    {
      sns_log_smr_pkt_s  *log_pkt_ptr;
      sns_log_id_e log_type;
      uint32_t logpkt_size;

      SNS_PRINTF_STRING_LOW_0(SNS_DBG_MOD_APPS_SMR, "smr_la: smr_apps_log_the_packet is running\n");
      logpkt_size = sizeof(sns_log_smr_pkt_s) + header_ptr->body_len - 1;
      switch (header_ptr->msg_type)
      {
        case SNS_SMR_MSG_TYPE_REQ:
          log_type = SNS_LOG_SMR_REQUEST;
          break;
        case SNS_SMR_MSG_TYPE_RESP:
          log_type = SNS_LOG_SMR_RESPONSE;
          break;
        case SNS_SMR_MSG_TYPE_IND:
          log_type = SNS_LOG_SMR_INDICATION;
          break;
        default:
          SNS_ASSERT_DBG(log_type - log_type);
          log_type = SNS_LOG_SMR_REQUEST;
          break;
      }
      /* LOG the packet */
      /* Allocate memory for log packet */
      sns_err = sns_logpkt_malloc (log_type,
                                   logpkt_size,
                                   (void**)(&log_pkt_ptr));

      if ( (log_pkt_ptr == NULL) || (sns_err != SNS_SUCCESS) )
      {
        /* Could not allocate memory, return */
        SNS_PRINTF_STRING_LOW_2(SNS_DBG_MOD_APPS_SMR,
                                "smr_la: couldn't alloc logpkt, pkt_ptr(0x%"PRIxPTR"), sns_err(%d)\n",
                                (intptr_t)log_pkt_ptr, sns_err);
        return;
      }

      /* Construct the log packet */
      log_pkt_ptr->version = 1;
      log_pkt_ptr->logging_processor = 2;
      log_pkt_ptr->timestamp      = sns_em_get_timestamp()/1000;
      log_pkt_ptr->dst_module     = header_ptr->dst_module;
      log_pkt_ptr->src_module     = header_ptr->src_module;
      log_pkt_ptr->priority       = header_ptr->priority;
      log_pkt_ptr->txn_id         = header_ptr->txn_id;
      log_pkt_ptr->ext_client_id  = header_ptr->ext_clnt_id;
      log_pkt_ptr->svc_num        = header_ptr->svc_num;
      log_pkt_ptr->msg_id         = header_ptr->msg_id;
      log_pkt_ptr->msg_type       = header_ptr->msg_type;
      log_pkt_ptr->pkt_size       = header_ptr->body_len;

      /* copy the message body */
      SNS_OS_MEMCOPY(log_pkt_ptr->pkt, (uint8_t*)header_ptr+sizeof(sns_smr_header_s), header_ptr->body_len);

      /* Commit log packet */
      sns_err = sns_logpkt_commit(log_type, log_pkt_ptr);

      if (sns_err != SNS_SUCCESS)
      {
        SNS_PRINTF_STRING_LOW_0(SNS_DBG_MOD_APPS_SMR,
                                "smr_la: couldn't alloc logpkt mem at smr_apps_log_the_packet\n");
      }
    }
  }
#else /* 0 */
  UNREFERENCED_PARAMETER(header_ptr);
  UNREFERENCED_PARAMETER(encoded);
#endif /* else 0 */
}


/*===========================================================================

  FUNCTION:   smr_apps_wl_timer_cb

===========================================================================*/
/*!
  @brief This function is the callback registered through sns_em_create_timer_obj().
       EM module calls this callback function when the timer expires

  @detail Release the wakelock used for processing SMD messages

  @param[i] arg is an argument for the callback which was passed through
            sns_em_create_timer_obj().
            In this implementation, the arg is ignored

  @return
   None

*/
/*=========================================================================*/
void smr_apps_wl_timer_cb (void * arg)
{
  UNREFERENCED_PARAMETER(arg);
  sns_pwr_set_wake_lock( false );
}

/*===========================================================================

  FUNCTION:   smr_apps_nomem_timer_cb

===========================================================================*/
/*!
  @brief This function is the callback registered through sns_em_create_timer_obj().
       EM module calls this callback function when the timer expires

  @detail Send the signal which is delivered through arg.

  @param[i] arg is an argument for the callback which was passed through
            sns_em_create_timer_obj().
            In this implementation, the arg is the signal flag to be posted.

  @return
   None

*/
/*=========================================================================*/
void smr_apps_nomem_timer_cb (void * arg)
{
  uint8_t   os_err;
  sns_os_sigs_post(sns_smr.sig_grp_ptr, (OS_FLAGS)(uintptr_t)arg, OS_FLAG_SET, &os_err);
  SNS_ASSERT (OS_ERR_NONE == os_err);
}

/*===========================================================================

  FUNCTION:  smr_apps_la_thread_main

===========================================================================*/
/**
  @brief This function is APPS SMR thread start routine.

  @param[i] arg_ptr A pointer to the argument

  @detail

  @return None

*/
/*=========================================================================*/
void smr_apps_la_thread_main (void *arg_ptr)
{
  sns_err_code_e sns_err;
  smr_msg_s     *de_msg_ptr;
  uint32_t       start_ts = 0;
  struct pollfd  pollfd[2];

  UNREFERENCED_PARAMETER(arg_ptr);

  sns_err = sns_em_create_timer_obj( smr_apps_wl_timer_cb, NULL,
                                     SNS_EM_TIMER_TYPE_ONESHOT, &smr_apps_wl_tmr_ptr);
  SNS_ASSERT ( sns_err == SNS_SUCCESS );


  sns_err = sns_em_create_timer_obj(smr_apps_nomem_timer_cb, (void*)SMR_APPS_SIG_NOMEM_TIMER,
                            SNS_EM_TIMER_TYPE_ONESHOT, &sns_smr.nomem_timer_ptr);
  SNS_ASSERT ( sns_err == SNS_SUCCESS );

  if( pipe2( sns_smr_la_wakeup_pipe, O_NONBLOCK ) != 0 ) {
    SNS_PRINTF_STRING_FATAL_1(SNS_DBG_MOD_APPS_SMR, "Pipe2 error %d", errno );
    SNS_ASSERT( 0 );
  }

  SNS_PRINTF_STRING_FATAL_2(SNS_DBG_MOD_APPS_SMR,
                            "smr_la: smr_apps_la_thread_main is starting, fd=%d, sns_smr.en_rx_msg_ptr=%"PRIxPTR,
                            sns_smr.fd, (intptr_t)sns_smr.en_rx_msg_ptr);



  pollfd[0].fd = sns_smr.fd;
  pollfd[0].events = POLLIN | POLLPRI | POLLERR;
  pollfd[1].fd = sns_smr_la_wakeup_pipe[0];
  pollfd[1].events = POLLIN | POLLPRI | POLLERR;


  sns_init_done();

  while (1)
  {
    ssize_t rcvd_size;
    void * de_body_ptr;
#if !defined(SNS_LA_SIM)
    uint8_t os_err;

    poll( pollfd, 1, -1 );
    if( ((pollfd[0].revents & POLLERR) == POLLERR) || (pollfd[1].revents != 0) ) {
      SNS_PRINTF_STRING_FATAL_3(SNS_DBG_MOD_APPS_SMR, "smr_la: poll return revents: 0x%x/%x errno: %d",
                                pollfd[0].revents, pollfd[1].revents, errno);
      sns_os_mutex_pend(sns_smr.smd_tx_mutex_ptr, 0, &os_err);
      sns_smr_la_write_disabled = 1;
      close( sns_smr.fd );
      sns_smr.fd = -1;

      sns_main_exit();
      sns_os_task_del( SNS_MODULE_PRI_APPS_SMR );
      return;
    }
    sns_pwr_set_wake_lock( true );
    sns_em_register_timer( smr_apps_wl_tmr_ptr,
                           sns_em_convert_usec_to_localtick(SNS_SMR_WL_TIMEOUT_USEC) );
    rcvd_size = read(sns_smr.fd, &sns_smr.en_rx_msg_ptr->header, sns_smr_get_max_msg_len());

#if defined(SNS_SMR_EXTRA_DEBUG)
#ifdef SMR_ENCODE_ON
    SNS_PRINTF_STRING_LOW_0(SNS_DBG_MOD_APPS_SMR, "smr_la: rcvd a ENCODED message at smr_apps_la_thread_main");
    SMR_MSG_PRINT(sns_smr.en_rx_msg_ptr, SMR_MSG_ENCODED);
#else
    SNS_PRINTF_STRING_LOW_0(SNS_DBG_MOD_APPS_SMR, "smr_la: rcvd a DECODED message at smr_apps_la_thread_main");
    SMR_MSG_PRINT(sns_smr.en_rx_msg_ptr, SMR_MSG_DECODED);
#endif /* SMR_ENCODE_ON */
#endif /* SNS_SMR_EXTRA_DEBUG */

#else
    sleep (1000);
    continue;
#endif
    if ((ssize_t)sizeof(sns_smr_header_s) <= rcvd_size)
    {
      qmi_idl_service_object_type svc_obj;
      uint32_t struct_len, qmi_result;

      if( sns_smr.en_rx_msg_ptr->header.priority != SNS_SMR_MSG_PRI_HIGH )
      {
        /* If this is not a high priority message, release the wakelock */
        sns_pwr_set_wake_lock( false );
        sns_em_cancel_timer( smr_apps_wl_tmr_ptr );
      }

#ifdef SMR_ENCODE_ON
      /* process the message. i.e. decode the msg, and send it to the dest module */
      if( sns_smr.en_rx_msg_ptr->header.msg_type > SENSOR1_MSG_TYPE_RESP_INT_ERR ||
          sns_smr.en_rx_msg_ptr->header.msg_id > SNS_SMR_MAX_MSG_ID )
      {
        /* There is a problem with the data from the DSPS:
         * Either the message type is invalid, or the message ID is very large */
        SNS_PRINTF_STRING_FATAL_2(SNS_DBG_MOD_APPS_SMR,
                                  "smr_la: rx bad message type(%d), qmi_id(0x%x). Skipping message",
                   sns_smr.en_rx_msg_ptr->header.msg_type, sns_smr.en_rx_msg_ptr->header.msg_id);
        continue;
      }
      svc_obj = sns_smr_get_svc_obj(sns_smr.en_rx_msg_ptr->header.svc_num);
      if (NULL == svc_obj)
      {
        SNS_PRINTF_STRING_FATAL_1(SNS_DBG_MOD_APPS_SMR,
                                  "smr_la: rx bad message svc_num(%d). Skipping message",
                                  sns_smr.en_rx_msg_ptr->header.svc_num);
        continue;
      }
      if (SNS_SMR_MSG_TYPE_RESP_INT_ERR == sns_smr.en_rx_msg_ptr->header.msg_type)
      {
        struct_len = 0;
      }
      else
      {
        qmi_result = qmi_idl_get_message_c_struct_len ( svc_obj,
                                        (qmi_idl_type_of_message_type)sns_smr.en_rx_msg_ptr->header.msg_type,
                                        sns_smr.en_rx_msg_ptr->header.msg_id,
                                        &struct_len);
        if (0 !=  qmi_result)
        {
          SNS_PRINTF_STRING_FATAL_1(SNS_DBG_MOD_APPS_SMR, "smr_la: err returned from qmi_idl_get_message_c_struct_len, result(%d)",qmi_result);
          SNS_PRINTF_STRING_FATAL_3(SNS_DBG_MOD_APPS_SMR, "smr_la: svc_num(%d), type(%d), qmi_id(0x%x). Skipping message",
                   sns_smr.en_rx_msg_ptr->header.svc_num, sns_smr.en_rx_msg_ptr->header.msg_type, sns_smr.en_rx_msg_ptr->header.msg_id);
          /* SNS_ASSERT_DBG (qmi_result - qmi_result); */
          continue;
        }
      }
#else
      struct_len = sns_smr.en_rx_msg_ptr->header.body_len;
      svc_obj = NULL;
#endif
      while (NULL == (de_body_ptr = sns_smr_msg_alloc(SNS_DBG_MOD_APPS_SMR, struct_len)))/* try until be able to get memory */
      {
        OS_FLAGS sig_flags;
        uint8_t  os_err;

        if ( 0 == sns_smr.nomem_retry_cnt )
        {
          start_ts = sns_em_get_timestamp();
        }

        if (!(sns_smr.nomem_retry_cnt % 10))
        {
          SNS_PRINTF_STRING_FATAL_2(SNS_DBG_MOD_APPS_SMR, "smr_la: no memory is available, size=%d, cnt=%d",
              struct_len, sns_smr.nomem_retry_cnt);
        }
        sns_em_register_timer (sns_smr.nomem_timer_ptr, sns_em_convert_usec_to_localtick(SMR_MEM_RETRY_USEC));
        sns_smr.nomem_retry_cnt++;
        SNS_ASSERT((sns_smr.nomem_retry_cnt < SMR_NOMEM_MAX_RETRY) &&
                   (sns_em_convert_localtick_to_usec(sns_em_get_timestamp()-start_ts)
                    < 2000000 )); /* limit 2 secs */
        sig_flags = sns_os_sigs_pend ( sns_smr.sig_grp_ptr, SMR_APPS_SIG_NOMEM_TIMER,
                    OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME, 0, &os_err);
        SNS_ASSERT (os_err == OS_ERR_NONE);
      }
      sns_smr.nomem_retry_cnt = 0;                        /* reset the count */
      de_msg_ptr = GET_SMR_MSG_PTR(de_body_ptr);
      /* process the message. i.e. decode the msg, and send it to the dest module */
      sns_err = smr_process_rcvd_msg(sns_smr.en_rx_msg_ptr, de_msg_ptr, svc_obj, struct_len);
      if (sns_err != SNS_SUCCESS)
      {
        SNS_PRINTF_STRING_FATAL_1(SNS_DBG_MOD_APPS_SMR, "smr_la: error returned from smr_process_rcvd_msg(err%d)", sns_err);
        sns_smr_msg_free(de_body_ptr);
        /* SNS_ASSERT_DBG (sns_err - sns_err); */
      }
    }
    else
    {
      SNS_PRINTF_STRING_FATAL_3(SNS_DBG_MOD_APPS_SMR, "smr_la: error at read(), rcvd_size=%d, errno=%d, fd=%d",
              rcvd_size, errno, sns_smr.fd);
      SNS_PRINTF_STRING_FATAL_1(SNS_DBG_MOD_APPS_SMR,
                                "smr_la: the message buffer pointer=%"PRIxPTR,
                                (intptr_t)sns_smr.en_rx_msg_ptr);

      sns_pwr_set_wake_lock( false );
      sns_em_cancel_timer( smr_apps_wl_tmr_ptr );

      sns_os_msleep(250);

      /* SNS_ASSERT_DBG(rcvd_size - rcvd_size); */
    }
  }
}

/*===========================================================================

  FUNCTION:  smr_apps_la_smd_init

===========================================================================*/
/**
  @brief
    Initialize all the necessary data structure of SMR(Sensor Message Routor)

  @detail
    Opens the shared memory pipe to the DSPS. Will delay and retry until the
    pipe is open, or the retries time out.

  @return Boolean success condition
  false -- an error occured
  true  -- no error


*/
/*=========================================================================*/
bool smr_apps_la_smd_init (void)
{
#if !defined(SNS_LA_SIM)
  int fd;
  int ms_retries = 0;
  char sns_smd_dev[SNS_SMD_NAME_MAX_LEN];

  if(sns_pwr_get_msm_type().msm_id == SNS_MSM_8974)
  {
   strlcpy(sns_smd_dev,"/dev/smd_sns_adsp",SNS_SMD_NAME_MAX_LEN);
  }
  else
  {
   strlcpy(sns_smd_dev,"/dev/smd_sns_dsps",SNS_SMD_NAME_MAX_LEN);
  }
  while ((fd = open(sns_smd_dev, O_RDWR | O_APPEND)) == -1)
  {
    ms_retries += SNS_SMR_SMD_OPEN_RETRY_MS;
    SNS_PRINTF_STRING_ERROR_2(SNS_DBG_MOD_APPS_SMR,
                              "SMD Open error, errno=%d ms_retries=%d\n",
                              errno, ms_retries);

    if( ms_retries > SNS_SMR_SMD_OPEN_RETRY_MAX_MS )
    {
      return false;
    }
    sns_os_msleep (SNS_SMR_SMD_OPEN_RETRY_MS);
  }
  SNS_PRINTF_STRING_LOW_1(SNS_DBG_MOD_APPS_SMR, "SMD Open success, fd=%d\n",
                          fd);
  sns_smr.fd = fd;
#endif

  return true;
}

/*===========================================================================

  FUNCTION:   smr_apps_la_send

===========================================================================*/
/*!
  @brief This function transfers the message to a module in DSPS or MODEM

  @param[i] body_ptr: A pointer to the message body allocated by sns_smr_msg_alloc()
  @param[i] svc_obj The QMI service object that associated with the message body
  @detail
    dst_module in header was already set by the caller of this function

  @return
   - SNS_SUCCESS if the message header was gotten successfully.
   - SNS_ERR_WOULDBLOCK if some resource is tentatively unavailable.
   - All other values indicate an error has occurred.

*/
/*=========================================================================*/
sns_err_code_e smr_apps_la_send (void* body_ptr, qmi_idl_service_object_type svc_obj)
{
  smr_msg_s *de_msg_ptr, * en_msg_ptr;
  sns_smr_header_s *de_header_ptr;
  int32_t  packet_len;
  int32_t  encode_result = 0;
  uint32_t encoded_len;
  uint8_t  os_err;

  sns_err_code_e rc = SNS_SUCCESS;
  en_msg_ptr = sns_smr.en_tx_msg_ptr;      /* encoded msg buffer */
  de_msg_ptr = GET_SMR_MSG_PTR(body_ptr);
  de_header_ptr = &de_msg_ptr->header;
#if defined(SNS_SMR_EXTRA_DEBUG)
  SNS_PRINTF_STRING_LOW_0(SNS_DBG_MOD_APPS_SMR, "smr_apps_la_send is running\n");
#endif

  /* Process only one request at a time for sharing the encodeding msg buffer and tx_peek()  */
  sns_os_mutex_pend(sns_smr.smd_tx_mutex_ptr, 0, &os_err);
  SNS_ASSERT(OS_ERR_NONE == os_err);
#ifdef SMR_ENCODE_ON
  if (de_header_ptr->body_len)
  {
    encode_result = qmi_idl_message_encode(svc_obj, de_header_ptr->msg_type, de_header_ptr->msg_id,
                    de_msg_ptr->body, de_header_ptr->body_len,
                    en_msg_ptr->body, SMR_MAX_BODY_LEN, &encoded_len);
    if( 0 != encode_result)
    {
      SNS_PRINTF_STRING_HIGH_1(SNS_DBG_MOD_APPS_SMR, "smr_apps_la_send: encoding was failed(%d)\n", encode_result);
      SNS_ASSERT(false);
    }
  }
  else
  {
    encoded_len = 0;
  }
  SNS_OS_MEMCOPY (&en_msg_ptr->header, de_header_ptr,  sizeof(sns_smr_header_s));
  en_msg_ptr->header.body_len = (uint16_t)encoded_len;
#else
  encoded_len = de_header_ptr->body_len;
  en_msg_ptr = de_msg_ptr;
#endif
  if (0 == encode_result)
  {
    ssize_t written_len = -1;
    packet_len = encoded_len + sizeof(sns_smr_header_s);
/* $TODO: if 0 */
#if !defined(SNS_LA_SIM)
# ifdef SMR_ENCODE_ON
    SMR_MSG_PRINT(en_msg_ptr, SMR_MSG_ENCODED);
# else
    SMR_MSG_PRINT(en_msg_ptr, SMR_MSG_DECODED);
# endif
    if( sns_smr_la_write_disabled == 0 ) {
      written_len = write(sns_smr.fd, &en_msg_ptr->header, packet_len);
    }
#else
    written_len = -1;
#endif
    if (written_len == packet_len)
    {
      smr_apps_log_the_packet(&en_msg_ptr->header, SMR_MSG_ENCODED);
#if defined(SNS_SMR_EXTRA_DEBUG)
      SNS_PRINTF_STRING_LOW_1(SNS_DBG_MOD_APPS_SMR, "write success, len(%d)\n", written_len);
#endif
      sns_smr_msg_free(body_ptr);
    }
    else if (written_len == -1)
    {
      SNS_PRINTF_STRING_FATAL_2(SNS_DBG_MOD_APPS_SMR, "SMD write error, errno=%d, fd=%d\n", errno, sns_smr.fd);
      rc = SNS_ERR_FAILED;
    }
    else
    {
      SNS_PRINTF_STRING_LOW_0(SNS_DBG_MOD_APPS_SMR, "write warning, 0 len was written\n");
      rc = SNS_ERR_WOULDBLOCK;
    }
  }
  else
  {
    SNS_PRINTF_STRING_LOW_0(SNS_DBG_MOD_APPS_SMR, "smr_apps_la_send: encoding was failed\n");
    rc = SNS_ERR_FAILED;
  }
  os_err = sns_os_mutex_post(sns_smr.smd_tx_mutex_ptr);
  SNS_ASSERT (os_err == OS_ERR_NONE);
  return rc;
}

/*===========================================================================

  FUNCTION:   smr_la_close

===========================================================================*/
/*!
  @brief  Close the SMD port

  May be called from signal handler.

  @param[i] None

  @detail

  @return
   - SNS_SUCCESS if close system call was success.
   - SNS_ERR_FAILED if an error occurred.


*/
/*=========================================================================*/
sns_err_code_e smr_la_close (void)
{
  char wr_data = 1;
  int fd;

  /* Wake up the reader thread, so it may exit */
  write( sns_smr_la_wakeup_pipe[1], &wr_data, 1 );

  /* There is no OSA function for trylock */
  pthread_mutex_trylock( &(sns_smr.smd_tx_mutex_ptr->mutex) );

  sns_smr_la_write_disabled = 1;

  fd = sns_smr.fd;
  sns_smr.fd = -1;

  close(fd);

  return SNS_SUCCESS;
}
