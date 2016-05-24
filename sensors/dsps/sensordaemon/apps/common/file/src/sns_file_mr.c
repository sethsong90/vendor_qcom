/*============================================================================
  @file sns_file_mr.c

  @brief
  Implementes the message routing and handling portion of the
  sensors file service.

  <br><br>

  Copyright (c) 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
============================================================================*/

/*============================================================================
  INCLUDE FILES
============================================================================*/
#include "sensor1.h"
#include "sns_common.h"
#include "sns_smr.h"
#include "sns_debug_str.h"
#include "sns_file.h"
#include "sns_memmgr.h"
#include "sns_file_internal_v01.h"
#include "sns_init.h"
#include <stdbool.h>
#if defined(SNS_LA) || defined(SNS_LA_SIM)
#include <unistd.h>
#endif /* defined(SNS_LA) || defined(SNS_LA_SIM) */

#ifdef SNS_QMI_ENABLE
#include <qmi_csi.h>

static qmi_csi_os_params os_params;
static qmi_csi_service_handle service_handle;

#ifdef _WIN32
  static KEVENT qmi_event;  /* event for os_params */
  static KEVENT exit_event; /* event to exit thread */
#endif

/**
 * Callback function for QMI framework - Not used
 */
qmi_csi_cb_error
sns_file_mr_csi_connect( qmi_client_handle client_handle, void *service_cookie,
    void **connection_handle )
{
  UNREFERENCED_PARAMETER(client_handle);
  UNREFERENCED_PARAMETER(service_cookie);
  UNREFERENCED_PARAMETER(connection_handle);

  /* Do Nothing */
  SNS_PRINTF_STRING_LOW_0( SNS_DBG_MOD_APPS_FILE, "Received connect" );
  return 0;
}

/**
 * Callback function for QMI framework - Not used
 */
void
sns_file_mr_csi_disconnect( void *connection_handle, void *service_cookie )
{
  UNREFERENCED_PARAMETER(service_cookie);
  UNREFERENCED_PARAMETER(connection_handle);

  /* Do Nothing */
  SNS_PRINTF_STRING_LOW_0( SNS_DBG_MOD_APPS_FILE, "Received disconnect" );
}

/*============================================================================
  FUNCTION:   sns_file_mr_csi_process_req
  ==========================================================================*/
/*!
  @brief  Callback function requests made to the file service.  Handle the
  request and send back the response.

  See QMI documentation for function parameters and return values.

*/
/*==========================================================================*/
qmi_csi_cb_error
sns_file_mr_csi_process_req( void *connection_handle, qmi_req_handle req_handle,
    unsigned int msg_id, void *req_c_struct, unsigned int req_c_struct_len,
    void *service_cookie )
{
  qmi_csi_error csi_err;
  sns_smr_header_s smr_header;
  void *smr_resp_msg;
  sns_err_code_e smr_err;

  UNREFERENCED_PARAMETER(service_cookie);
  UNREFERENCED_PARAMETER(connection_handle);

  SNS_PRINTF_STRING_LOW_2( SNS_DBG_MOD_APPS_FILE, "Received request %i (len %i)",
                           msg_id, req_c_struct_len );

  smr_header.dst_module = SNS_MODULE_APPS_FILE;
  smr_header.msg_type = SNS_SMR_MSG_TYPE_REQ;
  smr_header.svc_num = SNS_FILE_INTERNAL_SVC_ID_V01;
  smr_header.msg_id = (uint16_t)msg_id;
  smr_header.body_len = (uint16_t)req_c_struct_len;

  smr_err = sns_file_handle( &smr_header, req_c_struct, &smr_resp_msg );
  if( SNS_SUCCESS != smr_err )
  {
    SNS_PRINTF_STRING_ERROR_1( SNS_DBG_MOD_APPS_FILE, "Error handling request: %i", smr_err );
  }

  csi_err = qmi_csi_send_resp( req_handle, msg_id, smr_resp_msg, smr_header.body_len );

  if( SNS_SUCCESS == smr_err )
  {
    sns_smr_msg_free( smr_resp_msg );
  }

  if( QMI_CSI_NO_ERR != csi_err )
  {
    SNS_PRINTF_STRING_ERROR_1( SNS_DBG_MOD_APPS_FILE, "CSI send failure %i", csi_err );
    return QMI_CSI_INTERNAL_ERR;
  }

  return QMI_CSI_NO_ERR;
}

/*============================================================================
  FUNCTION:   sns_file_mr_thread
  ==========================================================================*/
void
sns_file_mr_thread( void *p_arg )
{
  qmi_csi_error csi_error;
#if defined(_WIN32)
  PVOID wait_events[2];

  /* Set the events we want to wait for in the WaitEvents array */
  wait_events[0] = &qmi_event;
  wait_events[1] = &exit_event;
#elif defined (SNS_LA) || defined(SNS_LA_SIM)
  qmi_csi_os_params os_params_in;
  fd_set fds;
#endif

  UNREFERENCED_PARAMETER( p_arg );

  SNS_PRINTF_STRING_LOW_0( SNS_DBG_MOD_APPS_FILE, "File thread started" );
  sns_init_done();

  for( ; ; )
  {
#if defined(_WIN32)
    NTSTATUS status = KeWaitForMultipleObjects(2, wait_events, WaitAny, Executive,
                                               KernelMode, FALSE, NULL, NULL);
    if (status == STATUS_WAIT_0)
    {
        // QMI Event
        KeClearEvent( &qmi_event );
        qmi_csi_handle_event( service_handle, &os_params );
    }
    else if (status == STATUS_WAIT_1)
    {
        // Exit Event
        break;
    }
#elif defined(SNS_LA) || defined(SNS_LA_SIM)
    fds = os_params.fds;
    select( os_params.max_fd+1, &fds, NULL, NULL, NULL );
    /* TODO: Create exit FD and exit thread if set */
    os_params_in.fds = fds;
    qmi_csi_handle_event( service_handle, &os_params_in );
#endif
  }

  csi_error = qmi_csi_unregister( service_handle );
  if (QMI_CSI_NO_ERR != csi_error)
  {
      SNS_PRINTF_STRING_ERROR_1( SNS_DBG_MOD_APPS_FILE, "Failed to unregister from csi %d", csi_error );
  }

  sns_os_task_del(SNS_MODULE_PRI_APPS_FILE);
}

/*============================================================================
  FUNCTION:   sns_file_mr_init
  ==========================================================================*/
sns_err_code_e
sns_file_mr_init()
{
  uint32_t *service_cookie = SNS_OS_MALLOC( SNS_DBG_MOD_APPS_FILE, sizeof(uint32_t) );
  qmi_csi_error csi_err;
  qmi_idl_service_object_type service_obj =
    sns_smr_get_svc_obj( SNS_FILE_INTERNAL_SVC_ID_V01 );

  if( NULL == service_cookie )
  {
    SNS_PRINTF_STRING_ERROR_0( SNS_DBG_MOD_APPS_FILE, "Alloc failure" );
    return SNS_ERR_FAILED;
  }
  else if( NULL == service_obj )
  {
    SNS_PRINTF_STRING_ERROR_1( SNS_DBG_MOD_APPS_FILE,
                               "Unable to get svc obj (%i)",
                               SNS_FILE_INTERNAL_SVC_ID_V01 );
    SNS_OS_FREE( service_cookie );
    return SNS_ERR_FAILED;
  }

#ifdef _WIN32
  // Initialize a notification for QMI events (requests from client)
  KeInitializeEvent(&qmi_event, NotificationEvent, FALSE);
  os_params.event = &qmi_event;

  // Initialize an event for allowing thread to exit
  KeInitializeEvent(&exit_event, NotificationEvent, FALSE);
#endif

  *service_cookie = SNS_FILE_INTERNAL_SVC_ID_V01;
  csi_err = qmi_csi_register( service_obj, sns_file_mr_csi_connect,
      sns_file_mr_csi_disconnect, sns_file_mr_csi_process_req, service_cookie,
      &os_params, &service_handle );

  if( QMI_CSI_NO_ERR != csi_err )
  {
    SNS_PRINTF_STRING_ERROR_1( SNS_DBG_MOD_APPS_FILE,
                               "CSI init failure %i", csi_err );
    SNS_OS_FREE( service_cookie );
    return SNS_ERR_FAILED;
  }

  return SNS_SUCCESS;
}

/*============================================================================
  FUNCTION:   sns_file_mr_deinit
  ==========================================================================*/
sns_err_code_e
sns_file_mr_deinit( void )
{
#if defined(_WIN32)
  // Set exit event
  KeSetEvent(&exit_event, 0, FALSE);
#elif defined(SNS_LA) || defined(SNS_LA_SIM)
  // TODO: exit handling
#endif

  return SNS_SUCCESS;
}

#else /* ifdef SNS_QMI_ENABLE */

/*============================================================================
  Preprocessor Definitions and Constants
  ============================================================================*/
#define SNS_FILE_SMR_FLAG    0x0001

/*============================================================================
  Static Variable Definitions
  ============================================================================*/
/** OS flag group for the Sensors file task */
static OS_FLAG_GRP *sns_file_sig_grp;

/*============================================================================
Function Definitions and Documentation
============================================================================*/

/*============================================================================
  FUNCTION:   sns_file_mr_init
  ==========================================================================*/
sns_err_code_e
sns_file_mr_init()
{
  sns_err_code_e  err;
  uint8_t         os_err;

  sns_file_sig_grp = sns_os_sigs_create( (OS_FLAGS)0x0, &os_err );
  if( ( NULL == sns_file_sig_grp ) || ( OS_ERR_NONE != os_err ) )
  {
    SNS_PRINTF_STRING_FATAL_1( SNS_DBG_MOD_APPS_FILE,
                               "Can't create OS signal %i", os_err );
    return SNS_ERR_FAILED;
  }

  err = sns_smr_register ( SNS_MODULE_APPS_FILE,
                           sns_file_sig_grp, SNS_FILE_SMR_FLAG );
  if( SNS_SUCCESS != err )
  {
    SNS_PRINTF_STRING_FATAL_1( SNS_DBG_MOD_APPS_FILE,
                               "SMR register file failed %i", err );
    return SNS_ERR_FAILED;
  }

  return SNS_SUCCESS;
}

/*============================================================================
  FUNCTION:   sns_file_mr_thread
  ==========================================================================*/
void
sns_file_mr_thread( void *p_arg )
{
  OS_FLAGS        flags;
  sns_err_code_e  err;
  uint8_t         os_err;
  void *msg_ptr, *resp_msg_ptr;
  sns_smr_header_s smr_header;

  SNS_PRINTF_STRING_LOW_0( SNS_DBG_MOD_APPS_FILE,
                           "File service thread started" );

  sns_init_done();

  while( true )
  {
    flags = sns_os_sigs_pend( sns_file_sig_grp,
                              SNS_FILE_SMR_FLAG,
                              OS_FLAG_WAIT_SET_ANY | OS_FLAG_CONSUME,
                              0, &os_err );

    if( OS_ERR_NONE != os_err )
    {
      SNS_PRINTF_STRING_ERROR_1( SNS_DBG_MOD_APPS_FILE,
                                 "Error in sigs_pend %u", (uint32)os_err );
    }
    if( flags & SNS_FILE_SMR_FLAG )
    {
      SNS_PRINTF_STRING_LOW_0( SNS_DBG_MOD_APPS_FILE, "rx msg from SMR" );

      msg_ptr = sns_smr_rcv( SNS_MODULE_APPS_FILE );
      while( NULL != msg_ptr )
      {
        err = sns_smr_get_hdr( &smr_header, msg_ptr );
        if( SNS_SUCCESS == err )
        {
          err = sns_file_handle( &smr_header, msg_ptr, &resp_msg_ptr );
          if( SNS_SUCCESS == err )
          {
            sns_smr_msg_free( msg_ptr );
          }
          sns_smr_set_hdr( &smr_header, resp_msg_ptr );

          err = sns_smr_send( resp_msg_ptr );
        }
        if( SNS_SUCCESS != err )
        {
          SNS_PRINTF_STRING_ERROR_1( SNS_DBG_MOD_APPS_FILE,
                                     "Error sending response %i", err );
          sns_smr_msg_free( resp_msg_ptr );
        }

        /* get the next message in the queue */
        msg_ptr = sns_smr_rcv( SNS_MODULE_APPS_FILE );
      }
    }
  }
}

/*============================================================================
  FUNCTION:   sns_file_mr_deinit
  ==========================================================================*/
sns_err_code_e
sns_file_mr_deinit( void )
{
#if defined(_WIN32)
  // Set exit event
  KeSetEvent(&exit_event, 0, FALSE);
#elif defined(SNS_LA) || defined(SNS_LA_SIM)
  // TODO: exit handling
#endif

  return SNS_SUCCESS;
}

#endif /* ifdef SNS_QMI_ENABLE */
