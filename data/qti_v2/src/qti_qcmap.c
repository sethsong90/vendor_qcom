/*=============================================================================

FILE:  qti_qcmap.c

SERVICES: Implementation of QTI interface with QCMAP
===============================================================================

Copyright (c) 2012-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.

=============================================================================*/

/*=============================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  when       who        what, where, why
  --------   ---        ------------------------------------------------------
  04/04/13   sb         Add support for dynamic switching between USB PIDs
  11/13/12   sb         Created module.

=============================================================================*/


/*=============================================================================
                               INCLUDE FILES
==============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/select.h>
#include <pthread.h>
#include "ds_util.h"
#include "ds_string.h"
#include "qualcomm_mobile_access_point_msgr_v01.h"
#include "qti.h"
#include "stringl.h"
#include "comdef.h"
#include "amssassert.h"
#include "ds_Utils_DebugMsg.h"
#include "dsnet_lib.h"
#include "dssock_lib.h"

/*=============================================================================
                                MACRO DEFINITIONS
==============================================================================*/


/*=============================================================================
                                VARIABLE DEFINITIONS
==============================================================================*/
static qti_conf_t * qti_qcmap_conf;

/*=============================================================================
                                FUNCTION FORWARD DECLARATION
==============================================================================*/
qti_qcmap_msgr_ind_cb
(
 qmi_client_type user_handle,                    /* QMI user handle       */
 unsigned int    msg_id,                         /* Indicator message ID  */
 void           *ind_buf,                        /* Raw indication data   */
 unsigned int    ind_buf_len,                    /* Raw data length       */
 void           *ind_cb_data                     /* User call back handle */
 );

/*=============================================================================
                                FUNCTION DEFINITIONS
==============================================================================*/

/*=============================================================================
  FUNCTION QTI_QCMAP_INIT()

  DESCRIPTION

  This function initializes QTI interface to QCMAP

  DEPENDENCIES
  None.

  RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure

  SIDE EFFECTS
  None
==============================================================================*/
int qti_qcmap_init(qti_conf_t * qti_conf)
{
  qmi_idl_service_object_type                            qti_qcmap_msgr_service_object;
  uint32_t                                               num_services = 0, num_entries = 0;
  qmi_service_info                                       info[10];
  qmi_client_error_type                                  qmi_error, qmi_err_code = QMI_NO_ERR;
  qcmap_msgr_mobile_ap_status_ind_register_req_msg_v01   qcmap_mobile_ap_status_ind_reg;
  qcmap_msgr_mobile_ap_status_ind_register_resp_msg_v01  qcmap_mobile_ap_status_ind_rsp;
  qcmap_msgr_wwan_status_ind_register_req_msg_v01        wwan_status_ind_reg;
  qcmap_msgr_wwan_status_ind_register_resp_msg_v01       wwan_status_ind_rsp;
  qmi_client_type                                        qti_qcmap_msgr_notifier;
  qmi_cci_os_signal_type                                 qti_qcmap_msgr_os_params;
  int                                                    retry_count = 0;
/*---------------------------------------------------------------------------*/


  ds_assert(qti_conf != NULL);

  LOG_MSG_INFO1("qti_qcmap_init()", 0, 0, 0);

/*-----------------------------------------------------------------------------
  Static pointer to QTI configuration variable
------------------------------------------------------------------------------*/
  qti_qcmap_conf = qti_conf;

/*-----------------------------------------------------------------------------
  Obtain a QCMAP messenger service client for QTI
  - get the service object
  - notify the client
  - get service list
  - obtain the client
------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
  Get the service object
------------------------------------------------------------------------------*/
  qti_qcmap_msgr_service_object = qcmap_msgr_get_service_object_v01();
  if (qti_qcmap_msgr_service_object == NULL)
  {
    LOG_MSG_ERROR("QTI QCMAP messenger service object not available",
                   0, 0, 0);
    return QTI_FAILURE;
  }

/*-----------------------------------------------------------------------------
  Notify the client
------------------------------------------------------------------------------*/

  qmi_error = qmi_client_notifier_init(qti_qcmap_msgr_service_object,
                                       &qti_qcmap_msgr_os_params,
                                       &qti_qcmap_msgr_notifier);
  if (qmi_error < 0)
  {
    LOG_MSG_ERROR("QTI:qmi_client_notifier_init(qcmap_msgr) returned %d",
                   qmi_error, 0, 0);
    return QTI_FAILURE;
  }

/*----------------------------------------------------------------------------
  Check if the service is up, if not wait on a signal
-----------------------------------------------------------------------------*/
  while(retry_count < QTI_QCMAP_MAX_RETRY)
  {
    qmi_error = qmi_client_get_service_list(qti_qcmap_msgr_service_object,
                                            NULL,
                                            NULL,
                                            &num_services);
    LOG_MSG_INFO1(" QTI: qmi_client_get_service_list: %d", qmi_error, 0, 0);

    if(qmi_error == QMI_NO_ERR)
      break;

/*----------------------------------------------------------------------------
     wait for server to come up
-----------------------------------------------------------------------------*/
    QMI_CCI_OS_SIGNAL_WAIT(&qti_qcmap_msgr_os_params, QTI_QCMAP_MAX_TIMEOUT_MS);
    QMI_CCI_OS_SIGNAL_CLEAR(&qti_qcmap_msgr_os_params);
    LOG_MSG_INFO1("Returned from os signal wait", 0, 0, 0);
    retry_count++;
  }

  if(retry_count == QTI_QCMAP_MAX_RETRY )
  {
    qmi_client_release(qti_qcmap_msgr_notifier);
    qti_qcmap_msgr_notifier = NULL;
    LOG_MSG_ERROR("Reached maximum retry attempts %d", retry_count, 0, 0);
    return QTI_FAILURE;
  }

  num_entries = num_services;

  LOG_MSG_INFO1(" QTI: qmi_client_get_service_list: num_e %d num_s %d",
                num_entries, num_services, 0);

/*----------------------------------------------------------------------------
   The server has come up, store the information in info variable
------------------------------------------------------------------------------*/
  qmi_error = qmi_client_get_service_list(qti_qcmap_msgr_service_object,
                                          info,
                                          &num_entries,
                                          &num_services);

  LOG_MSG_INFO1("qmi_client_get_service_list: num_e %d num_s %d error %d",
                num_entries, num_services, qmi_error);

  if (qmi_error != QMI_NO_ERR)
  {
    qmi_client_release(qti_qcmap_msgr_notifier);
    qti_qcmap_msgr_notifier = NULL;
    LOG_MSG_ERROR("Can not get qcmap_msgr service list %d",
                  qmi_error, 0, 0);
    return QTI_FAILURE;
  }

/*----------------------------------------------------------------------------
  Obtain a QCMAP messenger client for QTI
------------------------------------------------------------------------------*/
  qmi_error = qmi_client_init(&info[0],
                              qti_qcmap_msgr_service_object,
                              qti_qcmap_msgr_ind_cb,
                              NULL,
                              NULL,
                              &(qti_qcmap_conf->qti_qcmap_msgr_handle));

  LOG_MSG_INFO1("qmi_client_init: %d", qmi_error, 0, 0);


  if (qmi_error != QMI_NO_ERR)
  {
    qmi_client_release(qti_qcmap_msgr_notifier);
    qti_qcmap_msgr_notifier = NULL;
    LOG_MSG_ERROR("Can not init qcmap_msgr client %d", qmi_error, 0, 0);
    return QTI_FAILURE;
  }

/*----------------------------------------------------------------------------
  Release QCMAP notifier as it acts as a client also
------------------------------------------------------------------------------*/
  qmi_error = qmi_client_release(qti_qcmap_msgr_notifier);
  qti_qcmap_msgr_notifier = NULL;

  if (qmi_error != QMI_NO_ERR)
  {
    LOG_MSG_ERROR("Can not release client qcmap notifier %d",
                  qmi_error,0,0);
  }

/*-----------------------------------------------------------------------------
  Register for WWAN indications from QCMAP
-----------------------------------------------------------------------------*/
  memset(&wwan_status_ind_reg,0,
         sizeof(qcmap_msgr_wwan_status_ind_register_req_msg_v01));

  memset(&wwan_status_ind_rsp,0,
         sizeof(qcmap_msgr_wwan_status_ind_register_resp_msg_v01));

  wwan_status_ind_reg.register_indication = 1;
  qmi_error = qmi_client_send_msg_sync(qti_qcmap_conf->qti_qcmap_msgr_handle,
                                       QMI_QCMAP_MSGR_WWAN_STATUS_IND_REG_REQ_V01,
                                       (void*)&wwan_status_ind_reg,
                                       sizeof(qcmap_msgr_wwan_status_ind_register_req_msg_v01),
                                       (void*)&wwan_status_ind_rsp,
                                       sizeof(qcmap_msgr_wwan_status_ind_register_resp_msg_v01),
                                       QCMAP_MSGR_QMI_TIMEOUT_VALUE);

  LOG_MSG_ERROR("qmi_client_send_msg_sync(enable): error %d result %d",
      qmi_error, wwan_status_ind_rsp.resp.result,0);

  if ((qmi_error != QMI_NO_ERR) ||
      (wwan_status_ind_rsp.resp.result != QMI_NO_ERR))
  {
    LOG_MSG_ERROR("Can not register for wwan status %d : %d",
        qmi_error, wwan_status_ind_rsp.resp.error,0);
    return QTI_FAILURE;
  }

  LOG_MSG_INFO1("Done registering for wwan status",0,0,0);



/*-----------------------------------------------------------------------------
  Register for MOBILE AP state indications from QCMAP
-----------------------------------------------------------------------------*/
  memset(&qcmap_mobile_ap_status_ind_reg, 0,
         sizeof(qcmap_msgr_mobile_ap_status_ind_register_req_msg_v01));
  memset(&qcmap_mobile_ap_status_ind_rsp, 0,
         sizeof(qcmap_msgr_mobile_ap_status_ind_register_resp_msg_v01));

  qcmap_mobile_ap_status_ind_reg.register_indication = 1;
  qmi_error = qmi_client_send_msg_sync(qti_qcmap_conf->qti_qcmap_msgr_handle,
                                       QMI_QCMAP_MSGR_MOBILE_AP_STATUS_IND_REG_REQ_V01,
                                       (void*)&qcmap_mobile_ap_status_ind_reg,
                                       sizeof(qcmap_msgr_mobile_ap_status_ind_register_req_msg_v01),
                                       (void*)&qcmap_mobile_ap_status_ind_rsp,
                                       sizeof(qcmap_msgr_mobile_ap_status_ind_register_resp_msg_v01),
                                       QCMAP_MSGR_QMI_TIMEOUT_VALUE);

  LOG_MSG_INFO1("qmi_client_send_msg_sync(enable): error %d result %d",
      qmi_error, qcmap_mobile_ap_status_ind_rsp.resp.result, 0);

  if ((qmi_error != QMI_NO_ERR) ||
      (qcmap_mobile_ap_status_ind_rsp.resp.result != QMI_NO_ERR))
  {
    LOG_MSG_ERROR("Can not register for mobile ap status %d : %d",
                  qmi_error,
                  qcmap_mobile_ap_status_ind_rsp.resp.error, 0);
    return QTI_FAILURE;
  }

  LOG_MSG_INFO1("Done registering for mobile ap status",0,0,0);

  return QTI_SUCCESS;
}

/*=============================================================================
  FUNCTION QTI_QCMAP_EXIT()

  DESCRIPTION

  This function releases QCMAP client

  DEPENDENCIES
  None.

  RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure

  SIDE EFFECTS
  None
==============================================================================*/
 int qti_qcmap_exit()
{
  qmi_client_error_type     qmi_error;

/*----------------------------------------------------------------------------*/

  qmi_error = qmi_client_release(qti_qcmap_conf->qti_qcmap_msgr_handle);
  qti_qcmap_conf->qti_qcmap_msgr_handle = NULL;

  if (qmi_error != QMI_NO_ERR)
  {
    LOG_MSG_ERROR("Can not release client qcmap client handle %d",
                  qmi_error,0,0);
    return QTI_FAILURE;
  }

  return QTI_SUCCESS;
}


/*===========================================================================
  FUNCTION  qcmap_msgr_qmi_qcmap_ind
  ===========================================================================*/
/*!
  @brief
  Processes an incoming QMI QCMAP Indication.

  @return
  void

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
qti_qcmap_msgr_ind_cb
(
 qmi_client_type user_handle,                    /* QMI user handle       */
 unsigned int    msg_id,                         /* Indicator message ID  */
 void           *ind_buf,                        /* Raw indication data   */
 unsigned int    ind_buf_len,                    /* Raw data length       */
 void           *ind_cb_data                     /* User call back handle */
 )
{
  qmi_client_error_type qmi_error;

  LOG_MSG_INFO1("qcmap_msgr_qmi_qcmap_ind: user_handle %X msg_id %d ind_buf_len %d.",
      user_handle, msg_id, ind_buf_len);

  switch (msg_id)
  {
    case QMI_QCMAP_MSGR_BRING_UP_WWAN_IND_V01:
    {
      qcmap_msgr_bring_up_wwan_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(qcmap_msgr_bring_up_wwan_ind_msg_v01));
      if (qmi_error != QMI_NO_ERR)
      {
        LOG_MSG_ERROR("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d",
            qmi_error,0,0);
        break;
      }


      /* Process packet service status indication for WWAN for QCMAP*/
      if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_CONNECTED_V01)
      {
        if (ind_data.mobile_ap_handle == qti_qcmap_conf->qti_mobile_ap_handle)
        {
          LOG_MSG_INFO1("qcmap_msgr_qmi_qcmap_ind: WWAN Connected",0,0,0);
          return;
        }
      }
      else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_CONNECTING_FAIL_V01)
      {
        if (ind_data.mobile_ap_handle == qti_qcmap_conf->qti_mobile_ap_handle)
        {
          LOG_MSG_INFO1("qcmap_msgr_qmi_qcmap_ind: WWAN Connecting Failed...",0,0,0);
          return;
        }
      }

      break;
    }
    case QMI_QCMAP_MSGR_TEAR_DOWN_WWAN_IND_V01:
    {
      qcmap_msgr_tear_down_wwan_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(qcmap_msgr_tear_down_wwan_ind_msg_v01));
      if (qmi_error != QMI_NO_ERR)
      {
        LOG_MSG_ERROR("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d",
            qmi_error,0,0);
        break;
      }

      if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_DISCONNECTED_V01)
      {
        if (ind_data.mobile_ap_handle == qti_qcmap_conf->qti_mobile_ap_handle)
        {
          LOG_MSG_INFO1("qcmap_msgr_qmi_qcmap_ind: WWAN Disconnected",0,0,0);
          return;
        }
      }
      else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_DISCONNECTING_FAIL_V01)
      {
        if (ind_data.mobile_ap_handle == qti_qcmap_conf->qti_mobile_ap_handle)
        {
          LOG_MSG_INFO1("qcmap_msgr_qmi_qcmap_ind: WWAN Disconnecting Failed...",0,0,0);
          return;
        }
      }

      break;
    }
    case QMI_QCMAP_MSGR_WWAN_STATUS_IND_V01:
    {
      qcmap_msgr_wwan_status_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(qcmap_msgr_wwan_status_ind_msg_v01));
      if (qmi_error != QMI_NO_ERR)
      {
        LOG_MSG_ERROR("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d",
            qmi_error,0,0);
        break;
      }

      if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_DISCONNECTED_V01)
      {
        LOG_MSG_INFO1("qcmap_msgr_qmi_qcmap_ind: WWAN Disconnected...",0,0,0);
        return;
      }
      else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_DISCONNECTING_FAIL_V01)
      {
        LOG_MSG_INFO1("qcmap_msgr_qmi_qcmap_ind: WWAN Disconnecting Failed...",0,0,0);
        return;
      }
      else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_CONNECTED_V01)
      {
        LOG_MSG_INFO1("qcmap_msgr_qmi_qcmap_ind: WWAN Connected...",0,0,0);
        return;
      }
      else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_CONNECTING_FAIL_V01)
      {
        LOG_MSG_INFO1("qcmap_msgr_qmi_qcmap_ind: WWAN Connecting Failed...",0,0,0);
        return;
      }

      break;
    }
  case QMI_QCMAP_MSGR_MOBILE_AP_STATUS_IND_V01:
    {
      qcmap_msgr_mobile_ap_status_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(qcmap_msgr_mobile_ap_status_ind_msg_v01));
      if (qmi_error != QMI_NO_ERR)
      {
        LOG_MSG_ERROR("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d",
            qmi_error,0,0);
        break;
      }

      if (ind_data.mobile_ap_status == QCMAP_MSGR_MOBILE_AP_STATUS_CONNECTED_V01)
      {
        LOG_MSG_INFO1("qcmap_msgr_qmi_qcmap_ind: Mobile AP Connected...",0,0,0);
        return;
      }
      else if (ind_data.mobile_ap_status == QCMAP_MSGR_MOBILE_AP_STATUS_DISCONNECTED_V01)
      {
        LOG_MSG_INFO1("qcmap_msgr_qmi_qcmap_ind: Mobile AP Disconnected...",0,0,0);
        return;
      }
      break;
    }

  default:
    break;
}

  return;
}



/*===========================================================================

FUNCTION enable_mobile_ap()

DESCRIPTION

  This function enables QC Mobile AP
  QTI uses the services of QC Mobile AP

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

/*==========================================================================*/

static int qti_enable_mobile_ap()
{
  qmi_client_error_type                        qmi_error;
  qmi_client_error_type                        qmi_err_code = QMI_NO_ERR;
  qcmap_msgr_mobile_ap_enable_resp_msg_v01     qcmap_enable_resp_msg_v01;

/*--------------------------------------------------------------------------*/

  memset(&qcmap_enable_resp_msg_v01,
         0,
         sizeof(qcmap_msgr_mobile_ap_enable_resp_msg_v01));

/*---------------------------------------------------------------------------
  Enable Mobile AP
----------------------------------------------------------------------------*/
  qmi_error = qmi_client_send_msg_sync(qti_qcmap_conf->qti_qcmap_msgr_handle,
                                       QMI_QCMAP_MSGR_MOBILE_AP_ENABLE_REQ_V01,
                                       NULL,
                                       0,
                                       (void*)&qcmap_enable_resp_msg_v01,
                                       sizeof(qcmap_msgr_mobile_ap_enable_resp_msg_v01),
                                       QCMAP_MSGR_QMI_TIMEOUT_VALUE);

  LOG_MSG_INFO1("qmi_client_send_msg_sync(enable): error %d result %d valid %d",
                qmi_error,
                qcmap_enable_resp_msg_v01.resp.result,
                qcmap_enable_resp_msg_v01.mobile_ap_handle_valid);

  if ((qmi_error != QMI_NO_ERR) ||
      (qcmap_enable_resp_msg_v01.resp.result != QMI_NO_ERR) ||
      (qcmap_enable_resp_msg_v01.mobile_ap_handle_valid != TRUE))
  {
    LOG_MSG_ERROR("Can not enable qcmap %d : %d",
        qmi_error, qcmap_enable_resp_msg_v01.resp.error,0);
    return QTI_FAILURE;
  }

/*---------------------------------------------------------------------------
  Assign the Mobile AP handle which is used by QTI
----------------------------------------------------------------------------*/
  if (qcmap_enable_resp_msg_v01.mobile_ap_handle > 0)
  {
    qti_qcmap_conf->qti_mobile_ap_handle = qcmap_enable_resp_msg_v01.mobile_ap_handle;
    LOG_MSG_INFO1(" QTI QCMAP Enabled",0,0,0);
    return QTI_SUCCESS;
  }
  else
  {
    LOG_MSG_INFO1("QCMAP Enable Failure",0,0,0);
	return QTI_FAILURE;
  }
}

/*===========================================================================

FUNCTION disable_mobile_ap()

DESCRIPTION

  This function disables QC Mobile AP

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

/*==========================================================================*/

static int qti_disable_mobile_ap()
{
  qcmap_msgr_mobile_ap_disable_req_msg_v01    qcmap_disable_req_msg_v01;
  qcmap_msgr_mobile_ap_disable_resp_msg_v01   qcmap_disable_resp_msg_v01;
  qmi_client_error_type                       qmi_error;

/*--------------------------------------------------------------------------*/

  memset(&qcmap_disable_req_msg_v01,
         0,
         sizeof(qcmap_msgr_mobile_ap_disable_req_msg_v01));

  memset(&qcmap_disable_resp_msg_v01,
         0,
         sizeof(qcmap_msgr_mobile_ap_disable_resp_msg_v01));

 // TO DO : check the status of mobile ap
/*--------------------------------------------------------------------------
  Disable mobile AP
---------------------------------------------------------------------------*/
  qcmap_disable_req_msg_v01.mobile_ap_handle = qti_qcmap_conf->qti_mobile_ap_handle;

  qmi_error = qmi_client_send_msg_sync(qti_qcmap_conf->qti_qcmap_msgr_handle,
                                       QMI_QCMAP_MSGR_MOBILE_AP_DISABLE_REQ_V01,
                                       &qcmap_disable_req_msg_v01,
                                       sizeof(qcmap_msgr_mobile_ap_disable_req_msg_v01),
                                       &qcmap_disable_resp_msg_v01,
                                       sizeof(qcmap_msgr_mobile_ap_disable_resp_msg_v01),
                                       QCMAP_MSGR_QMI_TIMEOUT_VALUE);

  LOG_MSG_INFO1("qmi_client_send_msg_sync(disable): error %d result %d",
                qmi_error,
                qcmap_disable_resp_msg_v01.resp.result,
				0);


  if ((qmi_error != QMI_NO_ERR) ||
      (qcmap_disable_resp_msg_v01.resp.result != QMI_NO_ERR))
  {
    LOG_MSG_ERROR( "Can not disable qcmap %d : %d",
        qmi_error, qcmap_disable_resp_msg_v01.resp.error,0);
    return QTI_FAILURE;
  }

  LOG_MSG_INFO1( "QCMAP disabled", 0, 0, 0);


  return QTI_SUCCESS;
}

/*===========================================================================

FUNCTION QTI_USB_LINK_UP()

DESCRIPTION

  This function sends a message to QCMAP setup the USB link for RNDIS/ECM
  tethering

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

/*==========================================================================*/
static int qti_usb_link_up
(
  qti_interface_e interface
)
{
  qcmap_msgr_usb_link_up_req_msg_v01   qcmap_qti_usb_link_up_req_msg;
  qcmap_msgr_usb_link_up_resp_msg_v01  qcmap_qti_usb_link_up_resp_msg;
  qmi_client_error_type                qmi_error;
/*--------------------------------------------------------------------------*/

  LOG_MSG_INFO1("Setup USB link start", 0, 0, 0);

  memset(&qcmap_qti_usb_link_up_req_msg,
         0,
         sizeof(qcmap_msgr_usb_link_up_req_msg_v01));

  memset(&qcmap_qti_usb_link_up_resp_msg,
         0,
         sizeof(qcmap_msgr_usb_link_up_resp_msg_v01));

/*-------------------------------------------------------------------------
  Setup USB link - RNDIS/ECM
---------------------------------------------------------------------------*/
  qcmap_qti_usb_link_up_req_msg.mobile_ap_handle =
                                         qti_qcmap_conf->qti_mobile_ap_handle;


  qcmap_qti_usb_link_up_req_msg.usb_link = interface;

  qmi_error = qmi_client_send_msg_sync(qti_qcmap_conf->qti_qcmap_msgr_handle,
                                       QMI_QCMAP_MSGR_USB_LINK_UP_REQ_V01,
                                       &qcmap_qti_usb_link_up_req_msg,
                                       sizeof(qcmap_msgr_usb_link_up_req_msg_v01),
                                       &qcmap_qti_usb_link_up_resp_msg,
                                       sizeof(qcmap_msgr_usb_link_up_resp_msg_v01),
                                       QCMAP_MSGR_QMI_TIMEOUT_VALUE);

  LOG_MSG_INFO1("Setup USB link. Error values: %d , %d",
        qmi_error, qcmap_qti_usb_link_up_resp_msg.resp.error, 0);

  if (( qmi_error != QMI_TIMEOUT_ERR ) && ((qmi_error != QMI_NO_ERR) ||
        (qcmap_qti_usb_link_up_resp_msg.resp.result != QMI_NO_ERR)))
  {
    LOG_MSG_ERROR("Setup USB link failed. Error values: %d , %d",
        qmi_error, qcmap_qti_usb_link_up_resp_msg.resp.error,0);
    return QTI_FAILURE;
  }

  LOG_MSG_INFO1("Setup USB link succeeded", 0, 0, 0);

  return QTI_SUCCESS;
}


/*===========================================================================

FUNCTION QTI_USB_LINK_DOWN()

DESCRIPTION

  This function sends a message to QCMAP to bring down the RNDIS/ECM USB link

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

/*==========================================================================*/
static int qti_usb_link_down
(
  qti_interface_e interface
)
{
  qcmap_msgr_usb_link_down_req_msg_v01   qcmap_qti_usb_link_down_req_msg;
  qcmap_msgr_usb_link_down_resp_msg_v01  qcmap_qti_usb_link_down_resp_msg;
  qmi_client_error_type                  qmi_error;
/*--------------------------------------------------------------------------*/

  LOG_MSG_INFO1("Tear down USB link start", 0, 0, 0);

  memset(&qcmap_qti_usb_link_down_req_msg,
         0,
         sizeof(qcmap_msgr_usb_link_down_req_msg_v01));

  memset(&qcmap_qti_usb_link_down_resp_msg,
         0,
         sizeof(qcmap_msgr_usb_link_down_resp_msg_v01));

/*-------------------------------------------------------------------------
  Setup USB link - RNDIS/ECM
---------------------------------------------------------------------------*/
  qcmap_qti_usb_link_down_req_msg.mobile_ap_handle =
                                         qti_qcmap_conf->qti_mobile_ap_handle;

  qcmap_qti_usb_link_down_req_msg.usb_link = interface;

  qmi_error = qmi_client_send_msg_sync(qti_qcmap_conf->qti_qcmap_msgr_handle,
                                       QMI_QCMAP_MSGR_USB_LINK_DOWN_REQ_V01,
                                       &qcmap_qti_usb_link_down_req_msg,
                                       sizeof(qcmap_msgr_usb_link_down_req_msg_v01),
                                       &qcmap_qti_usb_link_down_resp_msg,
                                       sizeof(qcmap_msgr_usb_link_down_resp_msg_v01),
                                       QCMAP_MSGR_QMI_TIMEOUT_VALUE);

  if (( qmi_error != QMI_TIMEOUT_ERR ) && ((qmi_error != QMI_NO_ERR) ||
        (qcmap_qti_usb_link_down_resp_msg.resp.result != QMI_NO_ERR)))
  {
    LOG_MSG_ERROR("Bring down USB link failed. Error values: %d , %d",
        qmi_error, qcmap_qti_usb_link_down_resp_msg.resp.error,0);
    return QTI_FAILURE;
  }

  LOG_MSG_INFO1("Bring down USB link succeeded", 0, 0, 0);

  return QTI_SUCCESS;
}

/*===========================================================================

FUNCTION QTI_QMI_CMD_EXEC()

DESCRIPTION

  This function performs the execution of commands present in command queue.
  It mainly is involved in sending required QCMAP messages to QCMAP daemon to
  perform QCMAP specific operations

DEPENDENCIES
  None.

RETURN VALUE
  QTI_SUCCESS on success
  QTI_FAILURE on failure


SIDE EFFECTS
  None

/*=========================================================================*/

int qti_qcmap_cmd_exec
(
  qti_qcmap_event_e    event,
  qti_interface_e      interface
)
{
  int                ret_val;
/*------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------
  Handle different events and perform QCMAP specific operations
--------------------------------------------------------------------------*/
  switch(event)
  {
/*-------------------------------------------------------------------------
  This event is processed when we get netlink up event upon USB cable
  plug in.
  - Here we enable QC Mobile AP
  - Enable RNDIS/ECM tethering
  - Connect backhaul
---------------------------------------------------------------------------*/
    case QTI_LINK_UP_EVENT:
    {
/*--------------------------------------------------------------------------
  Avoid link up down twice
---------------------------------------------------------------------------*/
      if(qti_qcmap_conf->state == QTI_LINK_DOWN_WAIT)
      {
        LOG_MSG_INFO1("Received duplicate LINK_UP event.Ignoring", 0, 0, 0);
        break;
      }

      LOG_MSG_INFO1("Got USB link up event", 0, 0, 0);
/*--------------------------------------------------------------------------
  Enable QC Mobile AP
---------------------------------------------------------------------------*/
      ret_val = qti_enable_mobile_ap();
      if (ret_val==QTI_SUCCESS)
      {
        LOG_MSG_INFO1(" Mobile AP enable: successful", 0, 0, 0);
      }
      else
      {
        LOG_MSG_ERROR(" Mobile AP enable: unsuccessful. Aborting", 0, 0, 0);
        return QTI_FAILURE;
      }

/*--------------------------------------------------------------------------
  Setup RNDIS/ECM tethering
---------------------------------------------------------------------------*/
      ret_val = qti_usb_link_up(interface);
      if (ret_val==QTI_SUCCESS)
      {
        LOG_MSG_INFO1(" Setup USB tethering: successful. Interface = %d",
                            interface, 0, 0);
      }
      else
      {
        LOG_MSG_ERROR(" Setup USB tethering: unsuccessful. Interface = %d",
                            interface, 0, 0);
        return QTI_FAILURE;
      }

      qti_qcmap_conf->state=QTI_LINK_DOWN_WAIT;

/*--------------------------------------------------------------------------
  Write to dmesg log. It will help in debugging customer issues quickly.
  But we need to make sure we dont write too many messages to dmesg.
---------------------------------------------------------------------------*/
      ds_system_call("echo QTI:LINK_DOWN_WAIT state > /dev/kmsg",
                     strlen("echo QTI:LINK_DOWN_WAIT state > /dev/kmsg"));
      break;
    }
/*-------------------------------------------------------------------------
  Processes netlink down event which happens upon USB cable plug out
  - Disable RNDIS/ECM tethering
  - Here we disconnect backhaul
  - Disable QC Mobile AP
--------------------------------------------------------------------------*/
    case QTI_LINK_DOWN_EVENT:
    {
/*--------------------------------------------------------------------------
  Avoid link bring down twice
---------------------------------------------------------------------------*/
      if(qti_qcmap_conf->state == QTI_LINK_UP_WAIT)
      {
        LOG_MSG_INFO1("Received duplicate LINK_DOWN event.Ignoring", 0, 0, 0);
        break;
      }

      LOG_MSG_INFO1(" Got USB link down event", 0, 0, 0);
/*--------------------------------------------------------------------------
  Bring down RNDIS/ECM tethering
---------------------------------------------------------------------------*/
      ret_val = qti_usb_link_down(interface);
      if (ret_val==QTI_SUCCESS)
      {
        LOG_MSG_INFO1(" Bring down USB tethering: successful. Interface = %d",
                            interface, 0, 0);
      }
      else
      {
        LOG_MSG_ERROR(" Bring down USB tethering: unsuccessful. Interface = %d",
                            interface, 0, 0);
      }

/*--------------------------------------------------------------------------
  Disable QC Mobile AP
---------------------------------------------------------------------------*/
      ret_val = qti_disable_mobile_ap();
      if (ret_val==QTI_SUCCESS)
      {
        LOG_MSG_INFO1(" Mobile AP disable: successful", 0, 0, 0);
      }
      else
      {
        LOG_MSG_ERROR(" Mobile AP disable: unsuccessful. Aborting", 0, 0, 0);
        return QTI_FAILURE;
      }

      qti_qcmap_conf->state = QTI_LINK_UP_WAIT;

/*--------------------------------------------------------------------------
  Write to dmesg log. It will help in debugging customer issues quickly.
  But we need to make sure we dont write too many messages to dmesg.
---------------------------------------------------------------------------*/
      ds_system_call("echo QTI:LINK_UP_WAIT state > /dev/kmsg",
                     strlen("echo QTI:LINK_UP_WAIT state > /dev/kmsg"));
      break;
    }

    default:
      LOG_MSG_INFO1("Ignoring event %d received",event,0,0);
      break;
  }

  LOG_MSG_INFO1("Succeed handle QCMAP event",0,0,0);
  return QTI_SUCCESS;
}

