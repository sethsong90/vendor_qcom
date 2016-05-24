#ifndef SNS_SMR_PRIV_H
#define SNS_SMR_PRIV_H

/*============================================================================

  @file sns_smr_priv.h

  @brief
  This file contains internal definition for Sensor Message Router

  Copyright (c) 2010-2012 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

============================================================================*/

/*============================================================================

                                INCLUDE FILES

============================================================================*/
#include <stddef.h>
#include <stdbool.h>
#include "sns_em.h"
#ifdef SNS_BLAST
#include "sns_debug_str_mdm.h"
#else
#include "sns_debug_str.h"
#endif

/*============================================================================

                            INTERNAL DEFINITION AND TYPES

============================================================================*/
#define SMR_ENCODE_ON

#if defined(SNS_DSPS_BUILD) || defined(SNS_PCSIM)
#define SNS_MODULE_SMR SNS_MODULE_DSPS_SMR
#define SNS_SMR_DBG_MOD          SNS_DBG_MOD_DSPS_SMR
#else
#define SNS_MODULE_SMR SNS_MODULE_APPS_SMR
#define SNS_SMR_DBG_MOD          SNS_DBG_MOD_APPS_SMR
#endif

/* for debugging/logging purposes */
#define SMR_MSG_ENCODED   1
#define SMR_MSG_DECODED   0

/* Define the following to enable SMR debug messages for each packet */
/* #define SNS_SMR_EXTRA_DEBUG */

#ifdef SNS_SMR_EXTRA_DEBUG
#define SMR_MSG_PRINT     smr_msg_print
#else
#define SMR_MSG_PRINT(a,b)
#endif /* SNS_SMR_EXTRA_DEBUG */
/*============================================================================

                                  Constants

============================================================================*/
#define  SMR_MEM_RETRY_USEC       2000    /* Retry time interval when sns_malloc() was fail */
#define  SMR_NOMEM_MAX_RETRY      1000    /* The maximum attemp count to get memory to allocate */
#define  SMR_SEND_MAX_RETRY       1000    /* The maximum attemp count to send a message */

/* message length definition */
#define  SMR_RESERVE_LEN_FOR_LOG  10

#define SMR_INVALID_MODULE        0xff

#if defined(SNS_DSPS_BUILD) || defined(SNS_PCSIM) || defined(SNS_BLAST)
/* Signal definition for DSPS */
# define SMR_SIG_SMDL_APPS_DSPS_OPEN       0x0001
# define SMR_SIG_SMDL_APPS_DSPS_READ       0x0002
# define SMR_SIG_SMDL_APPS_DSPS_WRITE      0x0004
# define SMR_SIG_SMDL_APPS_DSPS_CLOSE      0x0008
# define SMR_SIG_SMDL_APPS_DSPS_ANY        0x000F
# define SMR_SIG_SMDL_MDM_DSPS_OPEN        0x0010
# define SMR_SIG_SMDL_MDM_DSPS_READ        0x0020
# define SMR_SIG_SMDL_MDM_DSPS_WRITE       0x0040
# define SMR_SIG_SMDL_MDM_DSPS_CLOSE       0x0080
# define SMR_SIG_SMDL_MDM_DSPS_ANY         0x00F0
# define SMR_SIG_SMR_MSGQ                  0x0100  /* A message arrived in SMR message queue */
# define SMR_SIG_GENERAL_TIMER             0x0200  /* For general oneshot timer */
# define SMR_SIG_ANY                       0xFFFF

/* SMDL FIFO size definition */
# define SMR_SMDL_FIFO_SIZE_APPS           (1024 * 8)
# define SMR_SMDL_FIFO_SIZE_MDM            (1024 * 8)


/* Definition of event index which is used for converting to a signal number */
typedef enum
{
  /** Called when the remote processor opens the port.
   * See smdl_open() */
  SMR_SMDL_EVENT_OPEN = 0,
  /** Called when data is available to be read from the FIFO.
   * See smdl_read() */
  SMR_SMDL_EVENT_READ,
  /** Called when space is available in the FIFO after a failed write.
   * See smdl_write() */
  SMR_SMDL_EVENT_WRITE,
  /** Called when the remote processor closes the port.
   * See smdl_close() */
  SMR_SMDL_EVENT_CLOSE,
  SMR_SMDL_EVENT_LAST
} smr_smdl_event_idx_e;

#define SMR_MAX_SVC_NUM                 (64-1)   
/* ----------------------------------------------------------------------------
 *  Definition of a smdl port structure which stores management info of the port
 * --------------------------------------------------------------------------*/
typedef struct smr_smdl_port_s
{
  smd_channel_type type;                /* SMD_APPS_DSPS or SMD_MDM_DSPS */
  smdl_handle_type hndl;
  OS_FLAGS         sig_flag[SMR_SMDL_EVENT_LAST];
  /** the maximum ext_clnt_id per svc id.
   * the values are used to clean all clients connections to SAM and SMGR
   * when there is an exception such as closing smd port by the other end.
   * Note: each valiue is initialized to 0xffff, and only 0-255 is valid range */
  int16_t           reqed_max_ext_clnt_id[SMR_MAX_SVC_NUM+1];
  boolean           remote_open_flag;       /* remote end open status */ 
} smr_smdl_port_s;
#endif  /* if defined(SNS_DSPS_BUILD) || defined(SNS_PCSIM) || defined(SNS_BLAST)*/

# if defined(SNS_LA) || defined(SNS_LA_SIM) || defined(_WIN32)
# define SMR_APPS_SIG_SMR_MSGQ          0x0100  /* A message arrived in SMR message queue */
# define SMR_APPS_SIG_NOMEM_TIMER       0x0001
# elif defined(SNS_BLAST)
# define SMR_MDM_SIG_SMR_MSGQ          0x0100  /* A message arrived in SMR message queue */
# define SMR_MDM_SIG_NOMEM_TIMER       0x0001
#endif /* if defined(SNS_BLAST) */


/* Definition of SMR state */
typedef enum
{
  SMR_STATE_NORMAL = 0,                 /* Normal state */
  SMR_STATE_NO_MEM,                     /* State when there is no memory to receive from SMD */
  SMR_STATE_INITIALIZING,               /* Initializing */
  SMR_STATE_CLOSED                      /* Terminating SMR */
} smr_state_e;

/* ----------------------------------------------------------------------------
 *  Definition of queue entry structure which stores queue management info of a module
 * --------------------------------------------------------------------------*/
typedef struct sns_smr_que_entry_s
{
  OS_FLAG_GRP     *sig_grp_ptr;         /* A pointer to the signal group which shall be used
                                           when a message was inserted into a queue */
  OS_FLAGS        sig_flag;             /* signal number to be used when a message was inserted
                                           into a queue */
  sns_q_s         queue[SNS_SMR_MSG_PRI_LAST];
  sns_q_s         *q_ptr[SNS_SMR_MSG_PRI_LAST];
  uint32_t        dropped_msg_cnt;
} smr_que_entry_s;

typedef struct smr_smdl_cnt_s
{
  uint32_t    smdl_write_cnt;
  uint32_t    smdl_read_cnt;
  uint32_t    smdl_open_cb_cnt;
  uint32_t    smdl_read_cb_cnt;
  uint32_t    smdl_write_cb_cnt;
  uint32_t    smdl_close_cb_cnt;
  uint32_t    smdl_unknown_cb_cnt;
  uint32_t    smdl_suspend_drop_cnt;
} smr_smdl_cnt_s;

/* ----------------------------------------------------------------------------
 *  Definition of SMR static data
 * --------------------------------------------------------------------------*/

typedef struct sns_smr_s
{
  smr_state_e        state;
  void               (*sig_hndl_ptr) (OS_FLAGS sig_flag);
  sns_em_timer_obj_t nomem_timer_ptr;
  sns_em_timer_obj_t general_timer_ptr;
  int32_t            nomem_retry_cnt;   /* memory retry count until memory is allocated */
  smr_que_entry_s    smr_que_tb[SNS_MODULE_CNT];
  smr_msg_s          *en_rx_msg_ptr;    /* message buffer for recieving data from SMD(L) */
  smr_msg_s          *en_tx_msg_ptr;    /* message buffer for writing data to SMD(L) */
  OS_EVENT           *que_mutex_ptr;    /* mutex used for queue accessing */
  OS_FLAG_GRP        *sig_grp_ptr;      /* for signal to SMR thread itself */
  OS_EVENT           *smd_tx_mutex_ptr; /* process one send request at a time */
#if defined(SNS_DSPS_BUILD) || defined(SNS_PCSIM)
  smr_smdl_port_s    smdl_app_port;     /* SMDL port between APPS and DSPS */
  smr_smdl_port_s    smdl_mdm_port;     /* SMDL port between MODEM and DSPS */
#elif defined(SNS_LA) || defined(SNS_LA_SIM)
  int                fd;
#elif defined(_WIN32)
  HANDLE             read_event;        /* Indicates data is ready to be read */
#elif defined(SNS_BLAST)
  smr_smdl_port_s    smdl_mdm_port;     /* SMDL port between MODEM and DSPS */
#endif
  smr_smdl_cnt_s     smdl_cnt;
} sns_smr_s;

extern sns_smr_s sns_smr;

/*===========================================================================

                    INTERNAL FUNCTION PROTOTYPES

===========================================================================*/
extern void smr_change_state(smr_state_e next_state);
extern smr_state_e smr_get_state(void);
extern sns_err_code_e smr_send_domestic(smr_msg_s *msg_ptr );
extern sns_err_code_e smr_process_rcvd_msg(const smr_msg_s *en_msg_ptr,
                                           smr_msg_s *de_msg_ptr,
                                           qmi_idl_service_object_type svc_obj,
                                           uint16_t de_body_len);
#if defined(SNS_DSPS_BUILD) || defined(SNS_PCSIM)
extern void smr_dsps_smdl_init (void);
extern void smr_dsps_smsm_init (void);
extern void smr_dsps_smdl_cb(smdl_handle_type hndl, smdl_event_type event, const smr_smdl_port_s *smdl_port_ptr);
extern void smr_dsps_thread_main(void *arg_ptr);
extern void smr_dsps_nomem_timer_cb(void * arg);
extern sns_err_code_e smr_dsps_normal_state_sig_hander(OS_FLAGS sig_flags);
extern void smr_dsps_log_the_packet(const sns_smr_header_s *header_ptr);
extern sns_err_code_e smr_dsps_send(void* body_ptr, qmi_idl_service_object_type svc_obj);
extern void smr_dsps_sig_handler(OS_FLAGS sig_flags);
#elif defined(SNS_LA) || defined( SNS_LA_SIM)
bool smr_apps_la_smd_init (void);
extern void smr_apps_la_thread_main (void *arg_ptr);
extern sns_err_code_e smr_app_la_send(void* body_ptr, qmi_idl_service_object_type svc_obj);
extern void sns_smr_init_app_android(void);
extern void smr_apps_log_the_packet(const sns_smr_header_s *header_ptr, boolean encoded);
extern sns_err_code_e smr_apps_la_send (void* body_ptr, qmi_idl_service_object_type svc_obj);
extern void smr_msg_print (smr_msg_s *msg_ptr, boolean encoded);
extern sns_err_code_e smr_la_close (void);
#elif defined(_WIN32)
bool smr_apps_win_smd_init (void);
extern void smr_apps_win_thread_main (void *arg_ptr);
extern sns_err_code_e smr_apps_win_send (void* body_ptr, qmi_idl_service_object_type svc_obj);
extern void sns_smr_init_app_win(void);
extern void smr_apps_log_the_packet(const sns_smr_header_s *header_ptr, boolean encoded);
extern void smr_msg_print (smr_msg_s *msg_ptr, boolean encoded);
extern sns_err_code_e smr_win_close (void);
#else
extern void smr_mdm_blast_smdl_init (void);
extern void smr_mdm_blast_thread_main (void *arg_ptr);
extern sns_err_code_e smr_mdm_blast_send (void* body_ptr, qmi_idl_service_object_type svc_obj);
extern void smr_mdm_log_the_packet (const sns_smr_header_s *header_ptr);
#endif

#endif /* SNS_SMR_PRIV_H */
