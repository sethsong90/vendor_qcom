#ifndef QCRIL_UIM_QUEUE_H
#define QCRIL_UIM_QUEUE_H

/*===========================================================================

  Copyright (c) 2010-2012 Qualcomm Technologies, Inc. All Rights Reserved

  Qualcomm Technologies Proprietary

  Export of this technology or software is regulated by the U.S. Government.
  Diversion contrary to U.S. law prohibited.

  All ideas, data and information contained in or disclosed by
  this document are confidential and proprietary information of
  Qualcomm Technologies, Inc. and all rights therein are expressly reserved.
  By accepting this material the recipient agrees that this material
  and the information contained therein are held in confidence and in
  trust and will not be used, copied, reproduced in whole or in part,
  nor its contents revealed in any manner to others without the express
  written permission of Qualcomm Technologies, Inc.

===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

$Header: //linux/pkgs/proprietary/qc-ril/main/source/qcril_uim_queue.h#1 $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
10/08/12   at      Support for ISIM Authentication API
04/09/12   at      Added support for RIL_REQUEST_SIM_GET_ATR
04/11/11   yt      Changes to support modem restart
03/30/11   at      Support for logical channel & send apdu commands
01/11/11   at      Added support for QCRIL_UIM_REQUEST_REFRESH_REGISTER
11/10/10   at      Initial version
===========================================================================*/

/*===========================================================================

                           INCLUDE FILES

===========================================================================*/

#include "qcril_uim.h"

/*===========================================================================

                           DEFINES

===========================================================================*/
/* This queue size definition is based on QMI's queue size. Based on this
   size, we decide to either enqueue requests or send directly */
#define QCRIL_UIM_MAX_QMI_QUEUE_SIZE                10

/*===========================================================================

                           TYPES

===========================================================================*/

/* -----------------------------------------------------------------------------
   ENUM:      QCRIL_UIM_REQUEST_TYPE

   DESCRIPTION:
     QMI operation to be performed.
-------------------------------------------------------------------------------*/
typedef enum
{
  QCRIL_UIM_REQUEST_READ_TRANSPARENT,
  QCRIL_UIM_REQUEST_READ_RECORD,
  QCRIL_UIM_REQUEST_WRITE_TRANSPARENT,
  QCRIL_UIM_REQUEST_WRITE_RECORD,
  QCRIL_UIM_REQUEST_GET_RESPONSE,
  QCRIL_UIM_REQUEST_REFRESH_REGISTER,
  QCRIL_UIM_REQUEST_GET_FDN,
  QCRIL_UIM_REQUEST_SET_FDN,
  QCRIL_UIM_REQUEST_VERIFY_PIN,
  QCRIL_UIM_REQUEST_UNBLOCK_PIN,
  QCRIL_UIM_REQUEST_CHANGE_PIN,
  QCRIL_UIM_REQUEST_SET_PIN,
  QCRIL_UIM_REQUEST_DEPERSO,
  QCRIL_UIM_REQUEST_POWER_UP,
  QCRIL_UIM_REQUEST_POWER_DOWN,
  QCRIL_UIM_REQUEST_CHANGE_PROV_SESSION,
  QCRIL_UIM_REQUEST_LOGICAL_CHANNEL,
  QCRIL_UIM_REQUEST_SEND_APDU,
  QCRIL_UIM_REQUEST_GET_ATR,
  QCRIL_UIM_REQUEST_AUTHENTICATE
} qcril_uim_request_type;


/* -----------------------------------------------------------------------------
   STRUCT:      QCRIL_UIM_QUEUE_REQUEST_ENTRY_TYPE

   DESCRIPTION:
     Structure used to specify information about a single incoming request entry
     that has to be stored in the queue.

-------------------------------------------------------------------------------*/
typedef struct qcril_uim_queue_request_entry_type
{
  qcril_uim_request_type                        request_type;
  qmi_client_handle_type                        qmi_handle;
  qmi_uim_user_async_cb_type                    callback_function_ptr;
  qcril_uim_original_request_type             * original_request_ptr;
  union
  {
    qmi_uim_read_transparent_params_type        read_transparent;
    qmi_uim_read_record_params_type             read_record;
    qmi_uim_write_transparent_params_type       write_transparent;
    qmi_uim_write_record_params_type            write_record;
    qmi_uim_get_file_attributes_params_type     get_attributes;
    qmi_uim_refresh_register_params_type        refresh_register;
    qmi_uim_set_service_status_params_type      set_service_status;
    qmi_uim_get_service_status_params_type      get_service_status;
    qmi_uim_verify_pin_params_type              verify_pin;
    qmi_uim_unblock_pin_params_type             unblock_pin;
    qmi_uim_change_pin_params_type              change_pin;
    qmi_uim_set_pin_protection_params_type      set_pin;
    qmi_uim_depersonalization_params_type       deperso;
    qmi_uim_power_up_params_type                power_up;
    qmi_uim_power_down_params_type              power_down;
    qmi_uim_change_prov_session_params_type     change_prov_session;
    qmi_uim_logical_channel_params_type         logical_channel;
    qmi_uim_send_apdu_params_type               send_apdu;
    qmi_uim_get_atr_params_type                 get_atr;
    qmi_uim_authenticate_params_type            authenticate;
  }                                             params;
  struct qcril_uim_queue_request_entry_type   * queue_next_ptr;
} qcril_uim_queue_request_entry_type;


/*=========================================================================

  FUNCTION:  qcril_uim_queue_send_request

===========================================================================*/
/*!
    @brief
    This function checks if the incoming request has to be:
    i.   Sent directly to the modem - when it is detemined that the number
         of QMI pending requests is low enough to proceed, or,
    ii.  Queued in the global buffer so that further processing will be
         performed later - this is when specified queue size -
         QCRIL_UIM_MAX_QMI_QUEUE_SIZE limit is reached.

    @return
    Error code returned by QMI API.
*/
/*=========================================================================*/
int qcril_uim_queue_send_request
(
  qcril_uim_request_type                     request_type,
  qmi_client_handle_type                     qmi_handle,
  const void                               * param_data_ptr,
  qmi_uim_user_async_cb_type                 callback_function_ptr,
  const qcril_uim_original_request_type    * original_request_ptr
);


/*=========================================================================

  FUNCTION:  qcril_uim_queue_complete_request

===========================================================================*/
/*!
    @brief
    This function is exedcuted at the end of each QMI request, when the
    request from the modem is received. This function takes care of
    sending a new request to QMI, if there is one in the queue, or
    decreases the counter of QMI pending requests.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_queue_complete_request
(
  void
);


/*=========================================================================

  FUNCTION:  qcril_uim_queue_cleanup

===========================================================================*/
/*!
    @brief
    This function checks if there are any requests in the queue. In case of
    sends of pending requests, it sends generic error to Android FW and
    removes the request from the queue.

    @return
    None
*/
/*=========================================================================*/
void qcril_uim_queue_cleanup
(
  void
);

#endif /* QCRIL_UIM_QUEUE_H */

