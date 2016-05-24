#ifndef QCRIL_UIM_QCCI_H
#define QCRIL_UIM_QCCI_H

/******************************************************************************
  @file    qcril_uim_qcci.h
  @brief   QMI message library UIM QCCI function definitions

  $Id$

  DESCRIPTION
  This file contains common, external header file definitions for QMI
  interface library.

  INITIALIZATION AND SEQUENCING REQUIREMENTS
  qcril_qmi_uim_srvc_init_client() must be called to create one or more clients
  qcril_qmi_uim_srvc_release_client() must be called to delete each client when
  finished.

  ---------------------------------------------------------------------------
  Copyright (c) 2012 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------
******************************************************************************/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

when       who     what, where, why
--------   ---     ----------------------------------------------------------
04/09/12   at      Added support for RIL_REQUEST_SIM_GET_ATR
02/14/12   tl      Initial version
===========================================================================*/

/*===========================================================================

                           INCLUDE FILES

===========================================================================*/

#include "qmi.h"
#include "qcril_uim_srvc.h"

/*===========================================================================
  FUNCTION  qcril_qmi_uim_srvc_init_client
===========================================================================*/
/*!
@brief
  This function is called to initialize the UIM service.  This function
  must be called prior to calling any other UIM service functions.
  For the time being, the indication handler callback and user data
  should be set to NULL until this is implemented.  Also note that this
  function may be called multiple times to allow for multiple, independent
  clients.

@return
  0 if operation was sucessful, < 0 if not.  If return code is
  QMI_INTERNAL_ERR, then the qmi_err_code will be valid and will
  indicate which QMI error occurred.

@note
  - Dependencies
    - qmi_connection_init() must be called for the associated port first.

  - Side Effects
    - Talks to modem processor
*/
/*=========================================================================*/
EXTERN qmi_client_handle_type qcril_qmi_uim_srvc_init_client
(
  const char                   * dev_id,
  qmi_uim_indication_hdlr_type   user_rx_ind_msg_hdlr,
  void                         * user_rx_ind_msg_hdlr_user_data,
  int                          * qmi_err_code
);

/*===========================================================================
  FUNCTION  qcril_qmi_uim_srvc_release_client
===========================================================================*/
/*!
@brief
  This function is called to release a client created by the
  qcril_qmi_uim_srvc_init_client() function.  This function should be called
  for any client created when terminating a client process, especially
  if the modem processor is not reset.  The modem side QMI server has
  a limited number of clients that it will allocate, and if they are not
  released, we will run out.

@return
  0 if operation was sucessful, < 0 if not.  If return code is
  QMI_INTERNAL_ERR, then the qmi_err_code will be valid and will
  indicate which QMI error occurred.

@note
  - Dependencies
    - qmi_connection_init() must be called for the associated port first.

  - Side Effects
    - Talks to modem processor
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_srvc_release_client
(
  int      user_handle,
  int    * qmi_err_code
);

/*===========================================================================
  FUNCTION  qcril_qmi_uim_reset
===========================================================================*/
/*!
@brief
  Resets UIM service.  If the user_cb function pointer is set to NULL,
  then this function will be invoked synchronously, otherwise it will
  be invoked asynchronously.

@return
  In the synchronous case, if return code < 0, the operation failed.  In
  the failure case, if the return code is QMI_SERVICE_ERR, then the
  qmi_err_code value in rsp_data  will give you the QMI error reason.
  Otherwise, qmi_err_code will have meaningless data.

  In the asynchronous case, if the return code < 0, the operation failed and
  you will not get an asynchronous response.  If the operation doesn't fail
  (return code >=0), the returned value will be a transaction handle which
  can be used to cancel the transaction via the qmi_uim_abort() command.

@note

  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - Resets UIM service
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_reset
(
  int                           user_handle,
  qmi_uim_user_async_cb_type    user_cb,
  void                        * user_data,
  int                         * qmi_err_code
);

/*===========================================================================
  FUNCTION  qcril_qmi_uim_read_transparent
===========================================================================*/
/*!
@brief
  Read a transparent file from the card.  If the user_cb function
  pointer is set to NULL, this function will be invoked synchronously.
  Otherwise it will be invoked asynchronously.

@return
  In the synchronous case, if return code < 0, the operation failed.  In
  the failure case, if the return code is QMI_SERVICE_ERR, then the
  qmi_err_code value will give you the QMI error reason.  Otherwise,
  qmi_err_code will have meaningless data.

  In the asynchronous case, if the return code < 0, the operation failed and
  you will not get an asynchronous response.  If the operation doesn't fail
  (return code >=0), the returned value will be a transaction handle which
  can be used to cancel the transaction via the qmi_uim_abort() command.

@note
  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_read_transparent
(
  int                                           client_handle,
  const qmi_uim_read_transparent_params_type  * params,
  qmi_uim_user_async_cb_type                    user_cb,
  void                                        * user_data,
  qmi_uim_rsp_data_type                       * rsp_data
);

/*===========================================================================
  FUNCTION  qcril_qmi_uim_read_record
===========================================================================*/
/*!
@brief
  Read a record from a linear/cyclic file from the card.  If the user_cb
  function pointer is set to NULL, this function will be invoked synchronously.
  Otherwise it will be invoked asynchronously.

@return
  In the synchronous case, if return code < 0, the operation failed.  In
  the failure case, if the return code is QMI_SERVICE_ERR, then the
  qmi_err_code value will give you the QMI error reason.  Otherwise,
  qmi_err_code will have meaningless data.

  In the asynchronous case, if the return code < 0, the operation failed and
  you will not get an asynchronous response.  If the operation doesn't fail
  (return code >=0), the returned value will be a transaction handle which
  can be used to cancel the transaction via the qmi_uim_abort() command.

@note
  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_read_record
(
  int                                           client_handle,
  const qmi_uim_read_record_params_type       * params,
  qmi_uim_user_async_cb_type                    user_cb,
  void                                        * user_data,
  qmi_uim_rsp_data_type                       * rsp_data
);

/*===========================================================================
  FUNCTION  qcril_qmi_uim_write_transparent
===========================================================================*/
/*!
@brief
  Write a transparent file to the card.  If the user_cb function
  pointer is set to NULL, this function will be invoked synchronously.
  Otherwise it will be invoked asynchronously.

@return
  In the synchronous case, if return code < 0, the operation failed.  In
  the failure case, if the return code is QMI_SERVICE_ERR, then the
  qmi_err_code value will give you the QMI error reason.  Otherwise,
  qmi_err_code will have meaningless data.

  In the asynchronous case, if the return code < 0, the operation failed and
  you will not get an asynchronous response.  If the operation doesn't fail
  (return code >=0), the returned value will be a transaction handle which
  can be used to cancel the transaction via the qmi_uim_abort() command.

@note
  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_write_transparent
(
  int                                           client_handle,
  const qmi_uim_write_transparent_params_type * params,
  qmi_uim_user_async_cb_type                    user_cb,
  void                                        * user_data,
  qmi_uim_rsp_data_type                       * rsp_data
);

/*===========================================================================
  FUNCTION  qcril_qmi_uim_write_record
===========================================================================*/
/*!
@brief
  Writes a record to a linear/cyclic file from the card.  If the user_cb
  function pointer is set to NULL, this function will be invoked synchronously.
  Otherwise it will be invoked asynchronously.

@return
  In the synchronous case, if return code < 0, the operation failed.  In
  the failure case, if the return code is QMI_SERVICE_ERR, then the
  qmi_err_code value will give you the QMI error reason.  Otherwise,
  qmi_err_code will have meaningless data.

  In the asynchronous case, if the return code < 0, the operation failed and
  you will not get an asynchronous response.  If the operation doesn't fail
  (return code >=0), the returned value will be a transaction handle which
  can be used to cancel the transaction via the qmi_uim_abort() command.

@note
  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_write_record
(
  int                                               client_handle,
  const qmi_uim_write_record_params_type          * params,
  qmi_uim_user_async_cb_type                        user_cb,
  void                                            * user_data,
  qmi_uim_rsp_data_type                           * rsp_data
);

/*===========================================================================
  FUNCTION  qcril_qmi_uim_get_file_attributes
===========================================================================*/
/*!
@brief
  Gets the file sttributes for a file on the card.  If the user_cb
  function pointer is set to NULL, this function will be invoked synchronously.
  Otherwise it will be invoked asynchronously.

@return
  In the synchronous case, if return code < 0, the operation failed.  In
  the failure case, if the return code is QMI_SERVICE_ERR, then the
  qmi_err_code value will give you the QMI error reason.  Otherwise,
  qmi_err_code will have meaningless data.

  In the asynchronous case, if the return code < 0, the operation failed and
  you will not get an asynchronous response.  If the operation doesn't fail
  (return code >=0), the returned value will be a transaction handle which
  can be used to cancel the transaction via the qmi_uim_abort() command.

@note
  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_get_file_attributes
(
  int                                                 client_handle,
  const qmi_uim_get_file_attributes_params_type     * params,
  qmi_uim_user_async_cb_type                          user_cb,
  void                                              * user_data,
  qmi_uim_rsp_data_type                             * rsp_data
);

/*===========================================================================
  FUNCTION  qcril_qmi_uim_set_pin_protection
===========================================================================*/
/*!
@brief
  Enables or disables specified pin on the card.  If the user_cb function
  pointer is set to NULL, this function will be invoked synchronously.
  Otherwise it will be invoked asynchronously.

@return
  In the synchronous case, if return code < 0, the operation failed.  In
  the failure case, if the return code is QMI_SERVICE_ERR, then the
  qmi_err_code value will give you the QMI error reason.  Otherwise,
  qmi_err_code will have meaningless data.

  In the asynchronous case, if the return code < 0, the operation failed and
  you will not get an asynchronous response.  If the operation doesn't fail
  (return code >=0), the returned value will be a transaction handle which
  can be used to cancel the transaction via the qmi_uim_abort() command.

@note
  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_set_pin_protection
(
  int                                               client_handle,
  const qmi_uim_set_pin_protection_params_type    * params,
  qmi_uim_user_async_cb_type                        user_cb,
  void                                            * user_data,
  qmi_uim_rsp_data_type                           * rsp_data
);

/*===========================================================================
  FUNCTION  qcril_qmi_uim_verify_pin
===========================================================================*/
/*!
@brief
  Verifies the specified pin on the card.  If the user_cb function
  pointer is set to NULL, this function will be invoked synchronously.
  Otherwise it will be invoked asynchronously.

@return
  In the synchronous case, if return code < 0, the operation failed.  In
  the failure case, if the return code is QMI_SERVICE_ERR, then the
  qmi_err_code value will give you the QMI error reason.  Otherwise,
  qmi_err_code will have meaningless data.

  In the asynchronous case, if the return code < 0, the operation failed and
  you will not get an asynchronous response.  If the operation doesn't fail
  (return code >=0), the returned value will be a transaction handle which
  can be used to cancel the transaction via the qmi_uim_abort() command.

@note
  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_verify_pin
(
  int                                               client_handle,
  const qmi_uim_verify_pin_params_type            * params,
  qmi_uim_user_async_cb_type                        user_cb,
  void                                            * user_data,
  qmi_uim_rsp_data_type                           * rsp_data
);

/*===========================================================================
  FUNCTION  qcril_qmi_uim_unblock_pin
===========================================================================*/
/*!
@brief
  Unblocks the specified pin on the card.  If the user_cb function
  pointer is set to NULL, this function will be invoked synchronously.
  Otherwise it will be invoked asynchronously.

@return
  In the synchronous case, if return code < 0, the operation failed.  In
  the failure case, if the return code is QMI_SERVICE_ERR, then the
  qmi_err_code value will give you the QMI error reason.  Otherwise,
  qmi_err_code will have meaningless data.

  In the asynchronous case, if the return code < 0, the operation failed and
  you will not get an asynchronous response.  If the operation doesn't fail
  (return code >=0), the returned value will be a transaction handle which
  can be used to cancel the transaction via the qmi_uim_abort() command.

@note
  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_unblock_pin
(
  int                                                 client_handle,
  const qmi_uim_unblock_pin_params_type             * params,
  qmi_uim_user_async_cb_type                          user_cb,
  void                                              * user_data,
  qmi_uim_rsp_data_type                             * rsp_data
);

/*===========================================================================
  FUNCTION  qcril_qmi_uim_change_pin
===========================================================================*/
/*!
@brief
  Changes the specified pin on the card.  If the user_cb function
  pointer is set to NULL, this function will be invoked synchronously.
  Otherwise it will be invoked asynchronously.

@return
  In the synchronous case, if return code < 0, the operation failed.  In
  the failure case, if the return code is QMI_SERVICE_ERR, then the
  qmi_err_code value will give you the QMI error reason.  Otherwise,
  qmi_err_code will have meaningless data.

  In the asynchronous case, if the return code < 0, the operation failed and
  you will not get an asynchronous response.  If the operation doesn't fail
  (return code >=0), the returned value will be a transaction handle which
  can be used to cancel the transaction via the qmi_uim_abort() command.

@note
  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_change_pin
(
  int                                               client_handle,
  const qmi_uim_change_pin_params_type            * params,
  qmi_uim_user_async_cb_type                        user_cb,
  void                                            * user_data,
  qmi_uim_rsp_data_type                           * rsp_data
);

/*===========================================================================
  FUNCTION  qcril_qmi_uim_depersonalization
===========================================================================*/
/*!
@brief
  Deactivates or unblocks specified personalization on the phone.  If the
  user_cb function pointer is set to NULL, this function will be invoked
  synchronously. Otherwise it will be invoked asynchronously.

@return
  In the synchronous case, if return code < 0, the operation failed.  In
  the failure case, if the return code is QMI_SERVICE_ERR, then the
  qmi_err_code value will give you the QMI error reason.  Otherwise,
  qmi_err_code will have meaningless data.

  In the asynchronous case, if the return code < 0, the operation failed and
  you will not get an asynchronous response.  If the operation doesn't fail
  (return code >=0), the returned value will be a transaction handle which
  can be used to cancel the transaction via the qmi_uim_abort() command.

@note
  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_depersonalization
(
  int                                               client_handle,
  const qmi_uim_depersonalization_params_type     * params,
  qmi_uim_user_async_cb_type                        user_cb,
  void                                            * user_data,
  qmi_uim_rsp_data_type                           * rsp_data
);

/*===========================================================================
  FUNCTION  qcril_qmi_uim_power_down
===========================================================================*/
/*!
@brief
  Powers down the card.  If the user_cb function pointer is set to NULL,
  this function will be invoked synchronously. Otherwise it will be invoked
  asynchronously.

@return
  In the synchronous case, if return code < 0, the operation failed.  In
  the failure case, if the return code is QMI_SERVICE_ERR, then the
  qmi_err_code value will give you the QMI error reason.  Otherwise,
  qmi_err_code will have meaningless data.

  In the asynchronous case, if the return code < 0, the operation failed and
  you will not get an asynchronous response.  If the operation doesn't fail
  (return code >=0), the returned value will be a transaction handle which
  can be used to cancel the transaction via the qmi_uim_abort() command.

@note
  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_power_down
(
  int                                               client_handle,
  const qmi_uim_power_down_params_type            * params,
  qmi_uim_user_async_cb_type                        user_cb,
  void                                            * user_data,
  qmi_uim_rsp_data_type                           * rsp_data
);

/*===========================================================================
  FUNCTION  qcril_qmi_uim_power_up
===========================================================================*/
/*!
@brief
  Powers up the card.  If the user_cb function pointer is set to NULL,
  this function will be invoked synchronously. Otherwise it will be invoked
  asynchronously.

@return
  In the synchronous case, if return code < 0, the operation failed.  In
  the failure case, if the return code is QMI_SERVICE_ERR, then the
  qmi_err_code value will give you the QMI error reason.  Otherwise,
  qmi_err_code will have meaningless data.

  In the asynchronous case, if the return code < 0, the operation failed and
  you will not get an asynchronous response.  If the operation doesn't fail
  (return code >=0), the returned value will be a transaction handle which
  can be used to cancel the transaction via the qmi_uim_abort() command.

@note
  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_power_up
(
  int                                               client_handle,
  const qmi_uim_power_up_params_type              * params,
  qmi_uim_user_async_cb_type                        user_cb,
  void                                            * user_data,
  qmi_uim_rsp_data_type                           * rsp_data
);

/*===========================================================================
  FUNCTION  qcril_qmi_uim_get_card_status
===========================================================================*/
/*!
@brief
  Gets the card status.  If the user_cb function pointer is set to NULL,
  this function will be invoked synchronously. Otherwise it will be invoked
  asynchronously.

@return
  In the synchronous case, if return code < 0, the operation failed.  In
  the failure case, if the return code is QMI_SERVICE_ERR, then the
  qmi_err_code value will give you the QMI error reason.  Otherwise,
  qmi_err_code will have meaningless data.

  In the asynchronous case, if the return code < 0, the operation failed and
  you will not get an asynchronous response.  If the operation doesn't fail
  (return code >=0), the returned value will be a transaction handle which
  can be used to cancel the transaction via the qmi_uim_abort() command.

@note
  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_get_card_status
(
  int                                           client_handle,
  qmi_uim_user_async_cb_type                    user_cb,
  void                                        * user_data,
  qmi_uim_rsp_data_type                       * rsp_data
);

/*===========================================================================
  FUNCTION  qcril_qmi_uim_event_reg
===========================================================================*/
/*!
@brief
  Gets the card status.  If the user_cb function pointer is set to NULL,
  this function will be invoked synchronously. Otherwise it will be invoked
  asynchronously.

@return
  In the synchronous case, if return code < 0, the operation failed.  In
  the failure case, if the return code is QMI_SERVICE_ERR, then the
  qmi_err_code value will give you the QMI error reason.  Otherwise,
  qmi_err_code will have meaningless data.

  In the asynchronous case, if the return code < 0, the operation failed and
  you will not get an asynchronous response.  If the operation doesn't fail
  (return code >=0), the returned value will be a transaction handle which
  can be used to cancel the transaction via the qmi_uim_abort() command.

@note
  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_event_reg
(
  int                                               client_handle,
  const qmi_uim_event_reg_params_type             * params,
  qmi_uim_rsp_data_type                           * rsp_data
);

/*===========================================================================
  FUNCTION  qcril_qmi_uim_refresh_register
===========================================================================*/
/*!
@brief
  Registers for file change notifications triggered by the card. If the
  user_cb function pointer is set to NULL, this function will be invoked
  synchronously. Otherwise it will be invoked asynchronously.

@return
  In the synchronous case, if return code < 0, the operation failed.  In
  the failure case, if the return code is QMI_SERVICE_ERR, then the
  qmi_err_code value will give you the QMI error reason.  Otherwise,
  qmi_err_code will have meaningless data.

  In the asynchronous case, if the return code < 0, the operation failed and
  you will not get an asynchronous response.  If the operation doesn't fail
  (return code >=0), the returned value will be a transaction handle which
  can be used to cancel the transaction via the qmi_uim_abort() command.

@note
  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_refresh_register
(
  int                                               client_handle,
  const qmi_uim_refresh_register_params_type      * params,
  qmi_uim_user_async_cb_type                        user_cb,
  void                                            * user_data,
  qmi_uim_rsp_data_type                           * rsp_data
);

/*===========================================================================
  FUNCTION  qcril_qmi_uim_refresh_ok
===========================================================================*/
/*!
@brief
  Enables the client to indicate if it is OK to start the refresh procedure.
  This function is supported only synchronously.

@return
  If return code < 0, the operation failed. In the failure case, if the
  return code is QMI_SERVICE_ERR, then the qmi_err_code value will give you
  the QMI error reason.  Otherwise, qmi_err_code will have meaningless data.

@note
  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_refresh_ok
(
  int                                               client_handle,
  const qmi_uim_refresh_ok_params_type            * params,
  qmi_uim_rsp_data_type                           * rsp_data
);

/*===========================================================================
  FUNCTION  qcril_qmi_uim_refresh_complete
===========================================================================*/
/*!
@brief
  Indicates to the modem that the client has finished the refresh procedure
  after re-reading all the cached files. This function is supported only
  synchronously.

@return
  If return code < 0, the operation failed. In the failure case, if the
  return code is QMI_SERVICE_ERR, then the qmi_err_code value will give you
  the QMI error reason.  Otherwise, qmi_err_code will have meaningless data.

@note
  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_refresh_complete
(
  int                                               client_handle,
  const qmi_uim_refresh_complete_params_type      * params,
  qmi_uim_rsp_data_type                           * rsp_data
);

/*===========================================================================
  FUNCTION  qcril_qmi_uim_refresh_get_last_event
===========================================================================*/
/*!
@brief
  Retreives the latest refresh event. This function is supported only
  synchronously.

@return
  If return code < 0, the operation failed. In the failure case, if the
  return code is QMI_SERVICE_ERR, then the qmi_err_code value will give you
  the QMI error reason.  Otherwise, qmi_err_code will have meaningless data.

@note
  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_refresh_get_last_event
(
  int                                                 client_handle,
  const qmi_uim_refresh_get_last_event_params_type  * params,
  qmi_uim_rsp_data_type                             * rsp_data
);

/*===========================================================================
  FUNCTION  qcril_qmi_uim_authenticate
===========================================================================*/
/*!
@brief
  Issues the authenticate request on the card. If the user_cb function
  pointer is set to NULL, this function will be invoked synchronously.
  Otherwise it will be invoked asynchronously.

@return
  In the synchronous case, if return code < 0, the operation failed.  In
  the failure case, if the return code is QMI_SERVICE_ERR, then the
  qmi_err_code value will give you the QMI error reason.  Otherwise,
  qmi_err_code will have meaningless data.

  In the asynchronous case, if the return code < 0, the operation failed and
  you will not get an asynchronous response.  If the operation doesn't fail
  (return code >=0), the returned value will be a transaction handle which
  can be used to cancel the transaction via the qmi_uim_abort() command.

@note

  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_authenticate
(
  int                                               client_handle,
  const qmi_uim_authenticate_params_type          * params,
  qmi_uim_user_async_cb_type                        user_cb,
  void                                            * user_data,
  qmi_uim_rsp_data_type                           * rsp_data
);

/*===========================================================================
  FUNCTION  qcril_qmi_uim_get_service_status
===========================================================================*/
/*!
@brief
  Issues the Service status command that queries for the status of particular
  services in the card e.g FDN, BDN etc. Note that currently only FDN query
  is supported. If the user_cb function pointer is set to NULL,
  this function will be invoked synchronously. Otherwise it will be invoked
  asynchronously.

@return
  In the synchronous case, if return code < 0, the operation failed.  In
  the failure case, if the return code is QMI_SERVICE_ERR, then the
  qmi_err_code value will give you the QMI error reason.  Otherwise,
  qmi_err_code will have meaningless data.

  In the asynchronous case, if the return code < 0, the operation failed and
  you will not get an asynchronous response.  If the operation doesn't fail
  (return code >=0), the returned value will be a transaction handle which
  can be used to cancel the transaction via the qmi_uim_abort() command.

@note
  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_get_service_status
(
  int                                               client_handle,
  const qmi_uim_get_service_status_params_type    * params,
  qmi_uim_user_async_cb_type                        user_cb,
  void                                            * user_data,
  qmi_uim_rsp_data_type                           * rsp_data
);


/*===========================================================================
  FUNCTION  qcril_qmi_uim_set_service_status
===========================================================================*/
/*!
@brief
  Issues the Service status command that sets the status of particular
  services in the card e.g FDN, BDN etc. Note that currently only FDN
  service is supported. If the user_cb function pointer is set to NULL,
  this function will be invoked synchronously. Otherwise it will be invoked
  asynchronously.

@return
  In the synchronous case, if return code < 0, the operation failed.  In
  the failure case, if the return code is QMI_SERVICE_ERR, then the
  qmi_err_code value will give you the QMI error reason.  Otherwise,
  qmi_err_code will have meaningless data.

  In the asynchronous case, if the return code < 0, the operation failed and
  you will not get an asynchronous response.  If the operation doesn't fail
  (return code >=0), the returned value will be a transaction handle which
  can be used to cancel the transaction via the qmi_uim_abort() command.

@note

  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_set_service_status
(
  int                                               client_handle,
  const qmi_uim_set_service_status_params_type    * params,
  qmi_uim_user_async_cb_type                        user_cb,
  void                                            * user_data,
  qmi_uim_rsp_data_type                           * rsp_data
);


/*===========================================================================
  FUNCTION  qcril_qmi_uim_change_provisioning_session
===========================================================================*/
/*!
@brief
  Issues the change provisioning session command that is used to activate,
  deactivate or switch the provisioning sessions. If the user_cb function
  pointer is set to NULL, this function will be invoked synchronously.
  Otherwise it will be invoked asynchronously.

@return
  In the synchronous case, if return code < 0, the operation failed.  In
  the failure case, if the return code is QMI_SERVICE_ERR, then the
  qmi_err_code value will give you the QMI error reason.  Otherwise,
  qmi_err_code will have meaningless data.

  In the asynchronous case, if the return code < 0, the operation failed and
  you will not get an asynchronous response.  If the operation doesn't fail
  (return code >=0), the returned value will be a transaction handle which
  can be used to cancel the transaction via the qmi_uim_abort() command.

@note

  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_change_provisioning_session
(
  int                                                     client_handle,
  const qmi_uim_change_prov_session_params_type         * params,
  qmi_uim_user_async_cb_type                              user_cb,
  void                                                  * user_data,
  qmi_uim_rsp_data_type                                 * rsp_data
);


/*===========================================================================
  FUNCTION  qcril_qmi_uim_get_label
===========================================================================*/
/*!
@brief
  Issues the get label command that retrieves the label of an application
  from EF-DIR on the UICC card. Note that it will return an error if used on
  an ICC card. If the user_cb function pointer is set to NULL, this function
  will be invoked synchronously. Otherwise it will be invoked asynchronously.

@return
  In the synchronous case, if return code < 0, the operation failed.  In
  the failure case, if the return code is QMI_SERVICE_ERR, then the
  qmi_err_code value will give you the QMI error reason.  Otherwise,
  qmi_err_code will have meaningless data.

  In the asynchronous case, if the return code < 0, the operation failed and
  you will not get an asynchronous response.  If the operation doesn't fail
  (return code >=0), the returned value will be a transaction handle which
  can be used to cancel the transaction via the qmi_uim_abort() command.

@note

  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_get_label
(
  int                                               client_handle,
  const qmi_uim_get_label_params_type             * params,
  qmi_uim_user_async_cb_type                        user_cb,
  void                                            * user_data,
  qmi_uim_rsp_data_type                           * rsp_data
);


/*===========================================================================
  FUNCTION  qcril_qmi_uim_close_session
===========================================================================*/
/*!
@brief
  Issues the close session command for a non-provisioning session that may
  have been opened before by the client. This function is supported only
  synchronously.

@return
  If return code < 0, the operation failed. In the failure case, if the
  return code is QMI_SERVICE_ERR, then the qmi_err_code value will give you
  the QMI error reason.  Otherwise, qmi_err_code will have meaningless data.

@note

  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_close_session
(
  int                                               client_handle,
  const qmi_uim_close_session_params_type         * params,
  qmi_uim_rsp_data_type                           * rsp_data
);


/*===========================================================================
  FUNCTION  qcril_qmi_uim_send_apdu
===========================================================================*/
/*!
@brief
  Issues the request for sending raw APDUs to the card. An optional channel
  id parameter can be used when a logical channel is already opened previously
  using the qcril_qmi_uim_logical_channel command. If the user_cb function pointer
  is set to NULL, this function will be invoked synchronously. Otherwise it
  will be invoked asynchronously.

@return
  In the synchronous case, if return code < 0, the operation failed.  In
  the failure case, if the return code is QMI_SERVICE_ERR, then the
  qmi_err_code value will give you the QMI error reason.  Otherwise,
  qmi_err_code will have meaningless data.

  In the asynchronous case, if the return code < 0, the operation failed and
  you will not get an asynchronous response.  If the operation doesn't fail
  (return code >=0), the returned value will be a transaction handle which
  can be used to cancel the transaction via the qmi_uim_abort() command.

@note

  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_send_apdu
(
  int                                               client_handle,
  const qmi_uim_send_apdu_params_type             * params,
  qmi_uim_user_async_cb_type                        user_cb,
  void                                            * user_data,
  qmi_uim_rsp_data_type                           * rsp_data
);


/*===========================================================================
  FUNCTION  qcril_qmi_uim_logical_channel
===========================================================================*/
/*!
@brief
  Issues the request for open or close logical channel on a particlar
  application on the UICC card. Note that it will return an error if used on
  an ICC card. If the user_cb function pointer is set to NULL, this function
  will be invoked synchronously. Otherwise it will be invoked asynchronously.

@return
  In the synchronous case, if return code < 0, the operation failed.  In
  the failure case, if the return code is QMI_SERVICE_ERR, then the
  qmi_err_code value will give you the QMI error reason.  Otherwise,
  qmi_err_code will have meaningless data.

  In the asynchronous case, if the return code < 0, the operation failed and
  you will not get an asynchronous response.  If the operation doesn't fail
  (return code >=0), the returned value will be a transaction handle which
  can be used to cancel the transaction via the qmi_uim_abort() command.

@note

  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_logical_channel
(
  int                                               client_handle,
  const qmi_uim_logical_channel_params_type       * params,
  qmi_uim_user_async_cb_type                        user_cb,
  void                                            * user_data,
  qmi_uim_rsp_data_type                           * rsp_data
);
    
    
/*===========================================================================
  FUNCTION  qcril_qmi_uim_get_atr
===========================================================================*/
/*!
@brief
  Issues the request to retrieve the ATR of the specified card. If the
  user_cb function pointer is set to NULL, this function will be invoked
  synchronously. Otherwise it will be invoked asynchronously.

@return
  In the synchronous case, if return code < 0, the operation failed.  In
  the failure case, if the return code is QMI_SERVICE_ERR, then the
  qmi_err_code value will give you the QMI error reason.  Otherwise,
  qmi_err_code will have meaningless data.

  In the asynchronous case, if the return code < 0, the operation failed and
  you will not get an asynchronous response.  If the operation doesn't fail
  (return code >=0), the returned value will be a transaction handle which
  can be used to cancel the transaction via the qmi_uim_abort() command.

@note

  - Dependencies
    - qcril_qmi_uim_srvc_init_client() must be called before calling this.

  - Side Effects
    - None
*/
/*=========================================================================*/
EXTERN int qcril_qmi_uim_get_atr
(
  int                                               client_handle,
  const qmi_uim_get_atr_params_type               * params,
  qmi_uim_user_async_cb_type                        user_cb,
  void                                            * user_data,
  qmi_uim_rsp_data_type                           * rsp_data
);

#endif

