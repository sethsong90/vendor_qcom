/******************************************************************************
  @file    qcril_qmi_imsa.c
  @brief   qcril qmi - IMS Application

  DESCRIPTION
    Handles RIL requests, Callbacks, indications for QMI IMS Application.

  ---------------------------------------------------------------------------

  Copyright (c) 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

  ---------------------------------------------------------------------------
******************************************************************************/

#include "qcril_qmi_imsa.h"
#include "qcril_qmi_client.h"
#include "qcril_log.h"
#include "ip_multimedia_subsystem_application_v01.h"
#include "qcril_qmi_ims_socket.h"
#include "qcril_qmi_ims_misc.h"
#include "qcril_reqlist.h"
#include "qcril_am.h"

typedef struct
{
   pthread_mutex_t       imsa_info_lock_mutex;
   boolean inited;
   boolean ims_status_change_registered;
   uint8_t ims_registered_valid;
   uint8_t ims_registered;
   uint8_t ims_srv_status_valid;
   qcril_qmi_imsa_srv_status_type ims_srv_status;
} qcril_qmi_imsa_info_type;

qcril_qmi_imsa_info_type qcril_qmi_imsa_info;

static void qcril_qmi_imsa_store_service_status_cl(
    uint8_t sms_service_status_valid,
    imsa_service_status_enum_v01 sms_service_status,
    uint8_t voip_service_status_valid,
    imsa_service_status_enum_v01 voip_service_status,
    uint8_t vt_service_status_valid,
    imsa_service_status_enum_v01 vt_service_status,
    uint8_t sms_service_rat_valid,
    imsa_service_rat_enum_v01 sms_service_rat,
    uint8_t voip_service_rat_valid,
    imsa_service_rat_enum_v01 voip_service_rat,
    uint8_t vt_service_rat_valid,
    imsa_service_rat_enum_v01 vt_service_rat
);

//===========================================================================
// qcril_qmi_imsa_pre_init
//===========================================================================
void qcril_qmi_imsa_pre_init(void)
{
   pthread_mutexattr_t mutex_attr;
   pthread_mutexattr_init( &mutex_attr );
   pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
   pthread_mutex_init( &qcril_qmi_imsa_info.imsa_info_lock_mutex, &mutex_attr);
   pthread_mutexattr_destroy(&mutex_attr);
} // qcril_qmi_imsa_pre_init

//===========================================================================
// qcril_qmi_imsa_info_lock
//===========================================================================
void qcril_qmi_imsa_info_lock()
{
  pthread_mutex_lock( &qcril_qmi_imsa_info.imsa_info_lock_mutex );
} // qcril_qmi_imsa_info_lock

//===========================================================================
// qcril_qmi_imsa_info_unlock
//===========================================================================
void qcril_qmi_imsa_info_unlock()
{
  pthread_mutex_unlock( &qcril_qmi_imsa_info.imsa_info_lock_mutex );
} // qcril_qmi_imsa_info_unlock

//===========================================================================
// qcril_qmi_imsa_set_init_state
//===========================================================================
void qcril_qmi_imsa_set_init_state(boolean inited)
{
   qcril_qmi_imsa_info_lock();
   qcril_qmi_imsa_info.inited = inited;
   qcril_qmi_imsa_info_unlock();
} // qcril_qmi_imsa_set_init_state

//===========================================================================
// qcril_qmi_imsa_init
//===========================================================================
void qcril_qmi_imsa_init()
{
   QCRIL_LOG_FUNC_ENTRY();

   qcril_qmi_imsa_info_lock();
   qcril_qmi_imsa_info.inited = TRUE;
   qcril_qmi_imsa_info.ims_registered_valid = FALSE;
   qcril_qmi_imsa_info.ims_status_change_registered = FALSE;
   qcril_qmi_imsa_info.ims_srv_status_valid = FALSE;
   qcril_qmi_imsa_info_unlock();

   // register for indication
   qmi_client_error_type ret = QMI_NO_ERR;
   imsa_ind_reg_req_msg_v01 ind_reg_req;
   imsa_ind_reg_rsp_msg_v01 *ind_reg_resp_ptr = qcril_malloc(sizeof(imsa_ind_reg_rsp_msg_v01));

   if (ind_reg_resp_ptr)
   {
      memset(&ind_reg_req, 0, sizeof(ind_reg_req));
      ind_reg_req.reg_status_config_valid = TRUE;
      ind_reg_req.reg_status_config = 1;  // enable
      ind_reg_req.service_status_config_valid = TRUE;
      ind_reg_req.service_status_config = 1;  // enable
      ind_reg_req.rat_handover_config_valid = TRUE;
      ind_reg_req.rat_handover_config = 1;  // enable

      ret = qcril_qmi_client_send_msg_async( QCRIL_QMI_CLIENT_IMSA,
                                             QMI_IMSA_IND_REG_REQ_V01,
                                             (void*) &ind_reg_req,
                                             sizeof(ind_reg_req),
                                             (void*) ind_reg_resp_ptr,
                                             sizeof(*ind_reg_resp_ptr),
                                             (void*)0
                                            );
      if (QMI_NO_ERR != ret)
      {
         QCRIL_LOG_ERROR("registration for indication failed %d", ret);
         qcril_free(ind_reg_resp_ptr);
      }
   }
   else
   {
      QCRIL_LOG_ERROR("ind_reg_resp_ptr malloc failed");
   }

   QCRIL_LOG_FUNC_RETURN();
} // qcril_qmi_imsa_init

//===========================================================================
// qcril_qmi_imsa_reg_status_ind_hdlr
//===========================================================================
void qcril_qmi_imsa_reg_status_ind_hdlr(void *ind_data_ptr)
{
   imsa_registration_status_ind_msg_v01 *reg_ind_msg_ptr = (imsa_registration_status_ind_msg_v01*) ind_data_ptr;
   QCRIL_LOG_FUNC_ENTRY();
   if (NULL != reg_ind_msg_ptr)
   {
      qcril_qmi_imsa_info_lock();
      qcril_qmi_imsa_info.ims_registered_valid = TRUE;
      qcril_qmi_imsa_info.ims_registered = reg_ind_msg_ptr->ims_registered;
      QCRIL_LOG_INFO("ims_registered: %d", qcril_qmi_imsa_info.ims_registered);
      qcril_qmi_imsa_info_unlock();
      qcril_qmi_ims_socket_send(0, IMS__MSG_TYPE__UNSOL_RESPONSE, IMS__MSG_ID__UNSOL_RESPONSE_IMS_NETWORK_STATE_CHANGED, IMS__ERROR__E_SUCCESS, NULL, 0);
   }
   else
   {
      QCRIL_LOG_ERROR("ind_data_ptr is NULL");
   }
   QCRIL_LOG_FUNC_RETURN();
} // qcril_qmi_ima_reg_status_ind_hdlr

//===========================================================================
// qcril_qmi_imsa_store_service_status_cl
//===========================================================================
void qcril_qmi_imsa_store_service_status_cl(
    uint8_t sms_service_status_valid,
    imsa_service_status_enum_v01 sms_service_status,
    uint8_t voip_service_status_valid,
    imsa_service_status_enum_v01 voip_service_status,
    uint8_t vt_service_status_valid,
    imsa_service_status_enum_v01 vt_service_status,
    uint8_t sms_service_rat_valid,
    imsa_service_rat_enum_v01 sms_service_rat,
    uint8_t voip_service_rat_valid,
    imsa_service_rat_enum_v01 voip_service_rat,
    uint8_t vt_service_rat_valid,
    imsa_service_rat_enum_v01 vt_service_rat
)
{
    qcril_qmi_imsa_info.ims_srv_status_valid = TRUE;

    qcril_qmi_imsa_info.ims_srv_status.sms_service_status_valid = sms_service_status_valid;
    qcril_qmi_imsa_info.ims_srv_status.sms_service_status = sms_service_status;

    qcril_qmi_imsa_info.ims_srv_status.voip_service_status_valid = voip_service_status_valid;
    qcril_qmi_imsa_info.ims_srv_status.voip_service_status = voip_service_status;

    qcril_qmi_imsa_info.ims_srv_status.vt_service_status_valid = vt_service_status_valid;
    qcril_qmi_imsa_info.ims_srv_status.vt_service_status = vt_service_status;

    qcril_qmi_imsa_info.ims_srv_status.sms_service_rat_valid = sms_service_rat_valid;
    qcril_qmi_imsa_info.ims_srv_status.sms_service_rat = sms_service_rat;

    qcril_qmi_imsa_info.ims_srv_status.voip_service_rat_valid = voip_service_rat_valid;
    qcril_qmi_imsa_info.ims_srv_status.voip_service_rat = voip_service_rat;

    qcril_qmi_imsa_info.ims_srv_status.vt_service_rat_valid = vt_service_rat_valid;
    qcril_qmi_imsa_info.ims_srv_status.vt_service_rat = vt_service_rat;
} // qcril_qmi_imsa_store_service_status_cl

//===========================================================================
// qcril_qmi_imsa_service_status_ind_hdlr
//===========================================================================
void qcril_qmi_imsa_service_status_ind_hdlr(void *ind_data_ptr)
{
    imsa_service_status_ind_msg_v01 *service_ind_msg_ptr = (imsa_service_status_ind_msg_v01*) ind_data_ptr;
    QCRIL_LOG_FUNC_ENTRY();
    if (NULL != service_ind_msg_ptr)
    {
        qcril_qmi_imsa_info_lock();

        qcril_qmi_imsa_store_service_status_cl(
            service_ind_msg_ptr->sms_service_status_valid,
            service_ind_msg_ptr->sms_service_status,
            service_ind_msg_ptr->voip_service_status_valid,
            service_ind_msg_ptr->voip_service_status,
            service_ind_msg_ptr->vt_service_status_valid,
            service_ind_msg_ptr->vt_service_status,
            service_ind_msg_ptr->sms_service_rat_valid,
            service_ind_msg_ptr->sms_service_rat,
            service_ind_msg_ptr->voip_service_rat_valid,
            service_ind_msg_ptr->voip_service_rat,
            service_ind_msg_ptr->vt_service_rat_valid,
            service_ind_msg_ptr->vt_service_rat );

        Ims__SrvStatusList *ims_srv_status_list_ptr = qcril_qmi_ims_create_ims_srvstatusinfo(
                                                          &qcril_qmi_imsa_info.ims_srv_status );

        if (ims_srv_status_list_ptr)
        {
            qcril_qmi_ims_socket_send(
                0,
                IMS__MSG_TYPE__UNSOL_RESPONSE,
                IMS__MSG_ID__UNSOL_SRV_STATUS_UPDATE,
                IMS__ERROR__E_SUCCESS,
                ims_srv_status_list_ptr,
                sizeof(*ims_srv_status_list_ptr) );
            qcril_qmi_ims_free_srvstatuslist(ims_srv_status_list_ptr);

            qcril_am_handle_event(QCRIL_AM_EVENT_IMS_SRV_CHANGED, NULL);
        }

        qcril_qmi_imsa_info_unlock();
    }
    else
    {
        QCRIL_LOG_ERROR("ind_data_ptr is NULL");
    }
    QCRIL_LOG_FUNC_RETURN();
} // qcril_qmi_imsa_service_status_ind_hdlr

//===========================================================================
// qcril_qmi_imsa_rat_handover_status_ind_hdlr
//===========================================================================
static void qcril_qmi_imsa_rat_handover_status_ind_hdlr(void *ind_data_ptr)
{
    QCRIL_LOG_FUNC_ENTRY();
    imsa_rat_handover_status_ind_msg_v01 *rat_ho_ind_msg_ptr = (imsa_rat_handover_status_ind_msg_v01*) ind_data_ptr;
    if (NULL != rat_ho_ind_msg_ptr)
    {
        if (rat_ho_ind_msg_ptr->rat_ho_status_info_valid)
        {
            Ims__Handover *ims_handover_ptr = qcril_qmi_ims_create_ims_handover_from_imsa_rat_info(&rat_ho_ind_msg_ptr->rat_ho_status_info);

            if (ims_handover_ptr)
            {
                qcril_qmi_ims_socket_send(
                    0,
                    IMS__MSG_TYPE__UNSOL_RESPONSE,
                    IMS__MSG_ID__UNSOL_RESPONSE_HANDOVER,
                    IMS__ERROR__E_SUCCESS,
                    ims_handover_ptr,
                    sizeof(*ims_handover_ptr) );
                qcril_qmi_ims_free_ims_handover(ims_handover_ptr);
            }
            else
            {
                QCRIL_LOG_DEBUG("ims_handover_ptr creation failed");
            }
        }
    }
    else
    {
        QCRIL_LOG_ERROR("ind_data_ptr is NULL");
    }
    QCRIL_LOG_FUNC_RETURN();
} // qcril_qmi_imsa_rat_handover_status_ind_hdlr

//===========================================================================
// qcril_qmi_imsa_unsol_ind_cb_helper
//===========================================================================
void qcril_qmi_imsa_unsol_ind_cb_helper
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr // Output parameter
)
{
  qmi_ind_callback_type * qmi_callback = NULL;

  uint32_t decoded_payload_len = 0;
  qmi_client_error_type qmi_err = QMI_NO_ERR;
  void* decoded_payload = NULL;

  QCRIL_LOG_FUNC_ENTRY();

  if( NULL != params_ptr && NULL != params_ptr->data )
  {
    qmi_callback = (qmi_ind_callback_type*) params_ptr->data;
    qmi_idl_get_message_c_struct_len(
                     qcril_qmi_client_get_service_object(QCRIL_QMI_CLIENT_IMSA),
                     QMI_IDL_INDICATION,
                     qmi_callback->msg_id,
                     &decoded_payload_len);

    if(decoded_payload_len)
    {
      decoded_payload = qcril_malloc(decoded_payload_len);
    }

    if ( decoded_payload || !decoded_payload_len )
    {
        if( decoded_payload_len )
        {
          qmi_err = qmi_client_message_decode(qcril_qmi_client_get_user_handle(QCRIL_QMI_CLIENT_IMSA),
                                              QMI_IDL_INDICATION,
                                              qmi_callback->msg_id,
                                              qmi_callback->data_buf,
                                              qmi_callback->data_buf_len,
                                              decoded_payload,
                                              (int)decoded_payload_len);
        }

        if ( QMI_NO_ERR == qmi_err )
        {
          switch(qmi_callback->msg_id)
          {
              case QMI_IMSA_REGISTRATION_STATUS_IND_V01:
                qcril_qmi_imsa_reg_status_ind_hdlr(decoded_payload);
                break;

              case QMI_IMSA_SERVICE_STATUS_IND_V01:
                qcril_qmi_imsa_service_status_ind_hdlr(decoded_payload);
                break;

              case QMI_IMSA_RAT_HANDOVER_STATUS_IND_V01:
                qcril_qmi_imsa_rat_handover_status_ind_hdlr(decoded_payload);
                break;

              default:
                QCRIL_LOG_INFO("Unknown QMI IMSA indication %d", qmi_callback->msg_id);
                break;
            }
          }
          else
          {
              QCRIL_LOG_INFO("Indication decode failed for msg %d of svc %d with error %d", qmi_callback->msg_id, QCRIL_QMI_CLIENT_IMSA, qmi_err );
          }

          if( decoded_payload_len )
          {
            qcril_free(decoded_payload);
          }
    }

    if( qmi_callback->data_buf )
    {
      qcril_free(qmi_callback->data_buf);
    }
  }
  else
  {
    QCRIL_LOG_ERROR("params_ptr or params_ptr->data is NULL");
  }

  QCRIL_LOG_FUNC_RETURN();

} // qcril_qmi_imsa_unsol_ind_cb_helper

//===========================================================================
// qcril_qmi_imsa_unsol_ind_cb
//===========================================================================
void qcril_qmi_imsa_unsol_ind_cb
(
  qmi_client_type                user_handle,
  unsigned long                  msg_id,
  unsigned char                  *ind_buf,
  int                            ind_buf_len,
  void                           *ind_cb_data
)
{
  qmi_ind_callback_type qmi_callback;

  QCRIL_LOG_FUNC_ENTRY();

  qmi_callback.data_buf = qcril_malloc(ind_buf_len);

  if( qmi_callback.data_buf )
  {
    qmi_callback.user_handle = user_handle;
    qmi_callback.msg_id = msg_id;
    memcpy(qmi_callback.data_buf,ind_buf,ind_buf_len);
    qmi_callback.data_buf_len = ind_buf_len;
    qmi_callback.cb_data = ind_cb_data;

    qcril_event_queue( QCRIL_DEFAULT_INSTANCE_ID,
                   QCRIL_DEFAULT_MODEM_ID,
                   QCRIL_DATA_ON_STACK,
                   QCRIL_EVT_QMI_IMSA_HANDLE_INDICATIONS,
                   (void*) &qmi_callback,
                   sizeof(qmi_callback),
                   (RIL_Token) QCRIL_TOKEN_ID_INTERNAL );
  }
  else
  {
    QCRIL_LOG_FATAL("malloc failed");
  }

  QCRIL_LOG_FUNC_RETURN();
} // qcril_qmi_imsa_unsol_ind_cb

//===========================================================================
// qcril_qmi_imsa_get_reg_status_resp_hdlr
//===========================================================================
void qcril_qmi_imsa_get_reg_status_resp_hdlr
(
  const qcril_request_params_type *const params_ptr
)
{
   imsa_get_registration_status_resp_msg_v01 *resp_msg_ptr = NULL;

   QCRIL_LOG_FUNC_ENTRY();
   if( NULL != params_ptr && NULL != params_ptr->data )
   {
      resp_msg_ptr = (imsa_get_registration_status_resp_msg_v01 *)params_ptr->data;
      if (QMI_RESULT_SUCCESS_V01 == resp_msg_ptr->resp.result && resp_msg_ptr->ims_registered_valid )
      {
         qcril_qmi_imsa_info_lock();
         qcril_qmi_imsa_info.ims_registered_valid = TRUE;
         qcril_qmi_imsa_info.ims_registered = resp_msg_ptr->ims_registered;
         QCRIL_LOG_INFO("ims_registered: %d", qcril_qmi_imsa_info.ims_registered);

         Ims__Registration reg = IMS__REGISTRATION__INIT;
         reg.has_state = TRUE;
         reg.state = qcril_qmi_ims_map_qmi_ims_reg_state_to_ims_reg_state(qcril_qmi_imsa_info.ims_registered);
         qcril_qmi_ims_socket_send(params_ptr->t, IMS__MSG_TYPE__RESPONSE, IMS__MSG_ID__REQUEST_IMS_REGISTRATION_STATE, IMS__ERROR__E_SUCCESS, &reg, sizeof(reg));
         qcril_qmi_imsa_info_unlock();
      }
      else
      {
         QCRIL_LOG_INFO("error: %d", resp_msg_ptr->resp.error);
         qcril_qmi_imsa_info_lock();
         qcril_qmi_imsa_info.ims_registered_valid = FALSE;
         qcril_qmi_ims_socket_send(params_ptr->t, IMS__MSG_TYPE__RESPONSE, IMS__MSG_ID__REQUEST_IMS_REGISTRATION_STATE, IMS__ERROR__E_GENERIC_FAILURE, NULL, 0);
         qcril_qmi_imsa_info_unlock();
      }
   }
   else
   {
      QCRIL_LOG_ERROR("params_ptr or params_ptr->data is NULL");
   }
   QCRIL_LOG_FUNC_RETURN();
} // qcril_qmi_imsa_get_reg_status_resp_hdlr

//===========================================================================
// qcril_qmi_imsa_ind_reg_resp_hdlr
//===========================================================================
void qcril_qmi_imsa_ind_reg_resp_hdlr
(
  const qcril_request_params_type *const params_ptr
)
{
   imsa_ind_reg_rsp_msg_v01 *resp_msg_ptr = NULL;
   QCRIL_LOG_FUNC_ENTRY();
   if( NULL != params_ptr && NULL != params_ptr->data )
   {
      resp_msg_ptr = (imsa_ind_reg_rsp_msg_v01 *)params_ptr->data;
      QCRIL_LOG_INFO("result: %d, error: %d", resp_msg_ptr->resp.result, resp_msg_ptr->resp.error);
      if (QMI_RESULT_SUCCESS_V01 == resp_msg_ptr->resp.result)
      {
         qcril_qmi_imsa_info_lock();
         qcril_qmi_imsa_info.ims_status_change_registered = TRUE;
         qcril_qmi_imsa_info_unlock();
      }
   }
   else
   {
      QCRIL_LOG_ERROR("params_ptr or params_ptr->data is NULL");
   }
   QCRIL_LOG_FUNC_RETURN();
} // qcril_qmi_imsa_ind_reg_resp_hdlr

//===========================================================================
// qcril_qmi_imsa_service_status_resp_hdlr
//===========================================================================
void qcril_qmi_imsa_get_service_status_resp_hdlr
(
  const qcril_request_params_type *const params_ptr
)
{
    imsa_get_service_status_resp_msg_v01 *resp_msg_ptr = NULL;

    QCRIL_LOG_FUNC_ENTRY();
    if( NULL != params_ptr && NULL != params_ptr->data )
    {
        resp_msg_ptr = (imsa_get_service_status_resp_msg_v01 *)params_ptr->data;
        if (QMI_RESULT_SUCCESS_V01 == resp_msg_ptr->resp.result)
        {
            qcril_qmi_imsa_info_lock();
            qcril_qmi_imsa_store_service_status_cl(
                resp_msg_ptr->sms_service_status_valid,
                resp_msg_ptr->sms_service_status,
                resp_msg_ptr->voip_service_status_valid,
                resp_msg_ptr->voip_service_status,
                resp_msg_ptr->vt_service_status_valid,
                resp_msg_ptr->vt_service_status,
                resp_msg_ptr->sms_service_rat_valid,
                resp_msg_ptr->sms_service_rat,
                resp_msg_ptr->voip_service_rat_valid,
                resp_msg_ptr->voip_service_rat,
                resp_msg_ptr->vt_service_rat_valid,
                resp_msg_ptr->vt_service_rat );

            Ims__SrvStatusList *ims_srv_status_list_ptr = qcril_qmi_ims_create_ims_srvstatusinfo(
                                                              &qcril_qmi_imsa_info.ims_srv_status );

            if (ims_srv_status_list_ptr)
            {
                qcril_qmi_ims_socket_send(
                    params_ptr->t,
                    IMS__MSG_TYPE__RESPONSE,
                    IMS__MSG_ID__REQUEST_QUERY_SERVICE_STATUS,
                    IMS__ERROR__E_SUCCESS,
                    ims_srv_status_list_ptr,
                    sizeof(*ims_srv_status_list_ptr) );
                qcril_qmi_ims_free_srvstatuslist(ims_srv_status_list_ptr);

                qcril_am_handle_event(QCRIL_AM_EVENT_IMS_SRV_CHANGED, NULL);
            }
            else
            {
                qcril_qmi_ims_socket_send(
                    params_ptr->t,
                    IMS__MSG_TYPE__RESPONSE,
                    IMS__MSG_ID__REQUEST_QUERY_SERVICE_STATUS,
                    IMS__ERROR__E_GENERIC_FAILURE,
                    NULL,
                    0 );
            }
            qcril_qmi_imsa_info_unlock();
        }
        else
        {
            QCRIL_LOG_INFO("error: %d", resp_msg_ptr->resp.error);
            qcril_qmi_imsa_info_lock();
            qcril_qmi_imsa_info.ims_srv_status_valid = FALSE;
            qcril_qmi_imsa_info_unlock();
            qcril_qmi_ims_socket_send(
                params_ptr->t,
                IMS__MSG_TYPE__RESPONSE,
                IMS__MSG_ID__REQUEST_QUERY_SERVICE_STATUS,
                IMS__ERROR__E_GENERIC_FAILURE,
                NULL,
                0 );
        }
    }
    else
    {
        QCRIL_LOG_ERROR("params_ptr or params_ptr->data is NULL");
    }
    QCRIL_LOG_FUNC_RETURN();
} // qcril_qmi_imsa_service_status_resp_hdlr

//===========================================================================
// qcril_qmi_imsa_command_cb_helper
//===========================================================================
void qcril_qmi_imsa_command_cb_helper
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr // Output parameter
)
{
  qcril_instance_id_e_type instance_id;
  uint32 user_data;
  uint16 req_id;
  uint16 dtmf_req_id;
  qcril_reqlist_public_type req_info;
  qcril_request_params_type req_data;
  qmi_resp_callback_type * qmi_resp_callback = NULL;

  QCRIL_LOG_FUNC_ENTRY();

  if( NULL != params_ptr && NULL != params_ptr->data )
  {
    qmi_resp_callback = (qmi_resp_callback_type *) params_ptr->data;
    if ( qmi_resp_callback->data_buf != NULL )
    {
       memset(&req_data, 0, sizeof(req_data));
       req_data.datalen = qmi_resp_callback->data_buf_len;
       req_data.data = qmi_resp_callback->data_buf;

       QCRIL_LOG_INFO("msg_id 0x%.2x", qmi_resp_callback->msg_id);

       if (QMI_IMSA_IND_REG_RSP_V01 == qmi_resp_callback->msg_id)
       {
          if( qmi_resp_callback->transp_err != QMI_NO_ERR )
          {
             QCRIL_LOG_INFO("Transp error (%d) recieved", qmi_resp_callback->transp_err);
          }
          else
          {
             qcril_qmi_imsa_ind_reg_resp_hdlr(&req_data);
          }
       }
       else
       {
          if ( qmi_resp_callback->cb_data != NULL )
          {
             user_data = ( uint32 ) qmi_resp_callback->cb_data;
             instance_id = QCRIL_EXTRACT_INSTANCE_ID_FROM_USER_DATA( user_data );
             req_id = QCRIL_EXTRACT_USER_ID_FROM_USER_DATA( user_data );
             req_data.instance_id = instance_id;
             req_data.modem_id = QCRIL_DEFAULT_MODEM_ID;
             /* Lookup the Token ID */
             if ( qcril_reqlist_query_by_req_id( req_id, &instance_id, &req_info ) == E_SUCCESS )
             {
               if( qmi_resp_callback->transp_err != QMI_NO_ERR )
               {
                 QCRIL_LOG_INFO("Transp error (%d) recieved from QMI for RIL request %d", qmi_resp_callback->transp_err, req_info.request);
                 qcril_send_empty_payload_request_response( instance_id, req_info.t, req_info.request, RIL_E_GENERIC_FAILURE ); // Send GENERIC_FAILURE response
               }
               else
               {
                 req_data.t = req_info.t;
                 req_data.event_id = req_info.request;
                 switch(qmi_resp_callback->msg_id)
                 {
                 case QMI_IMSA_GET_REGISTRATION_STATUS_RSP_V01:
                   qcril_qmi_imsa_get_reg_status_resp_hdlr(&req_data);
                   break;
                 case QMI_IMSA_GET_SERVICE_STATUS_RSP_V01:
                   qcril_qmi_imsa_get_service_status_resp_hdlr(&req_data);
                   break;

                 default:
                     QCRIL_LOG_INFO("Unsupported QMI IMSA message %d", qmi_resp_callback->msg_id);
                     break;
                 }
               }
             }
             else
             {
               QCRIL_LOG_ERROR( "Req ID: %d not found", req_id );
             }
          }
          else
          {
             QCRIL_LOG_ERROR( "qmi_resp_callback->cb_data is NULL" );
          }
       }
       qcril_free( qmi_resp_callback->data_buf );
    }
    else
    {
       QCRIL_LOG_ERROR("qmi_resp_callback->cb_data is NULL");
    }
  }
  else
  {
    QCRIL_LOG_ERROR("params_ptr or params_ptr->data is NULL");
  }

  QCRIL_LOG_FUNC_RETURN();
} // qcril_qmi_imsa_command_cb_helper

//===========================================================================
// qcril_qmi_imsa_command_cb
//===========================================================================
void qcril_qmi_imsa_command_cb
(
   qmi_client_type              user_handle,
   unsigned long                msg_id,
   void                         *resp_c_struct,
   int                          resp_c_struct_len,
   void                         *resp_cb_data,
   qmi_client_error_type        transp_err
)
{
  qmi_resp_callback_type qmi_resp_callback;
  memset(&qmi_resp_callback, 0, sizeof(qmi_resp_callback));

  QCRIL_LOG_FUNC_ENTRY();

  QCRIL_LOG_INFO(".. msg id %d", (int) msg_id );

  qmi_resp_callback.user_handle = user_handle;
  qmi_resp_callback.msg_id = msg_id;
  qmi_resp_callback.data_buf = (void*) resp_c_struct;
  qmi_resp_callback.data_buf_len = resp_c_struct_len;
  qmi_resp_callback.cb_data = resp_cb_data;
  qmi_resp_callback.transp_err = transp_err;

  qcril_event_queue( QCRIL_DEFAULT_INSTANCE_ID,
                 QCRIL_DEFAULT_MODEM_ID,
                 QCRIL_DATA_ON_STACK,
                 QCRIL_EVT_QMI_IMSA_HANDLE_COMM_CALLBACKS,
                 (void*) &qmi_resp_callback,
                 sizeof(qmi_resp_callback),
                 (RIL_Token) QCRIL_TOKEN_ID_INTERNAL );

  QCRIL_LOG_FUNC_RETURN();
} // qcril_qmi_imsa_command_cb

//===========================================================================
// qcril_qmi_imsa_request_ims_registration_state
//===========================================================================
void qcril_qmi_imsa_request_ims_registration_state
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr /*!< Output parameter */
)
{
  qcril_instance_id_e_type instance_id = QCRIL_DEFAULT_INSTANCE_ID;
  uint32 user_data;
  qcril_reqlist_public_type reqlist_entry;
  imsa_get_registration_status_resp_msg_v01* response_msg = NULL;

  QCRIL_NOTUSED( ret_ptr );

  QCRIL_LOG_FUNC_ENTRY();

  qcril_qmi_imsa_info_lock();

  if( NULL != params_ptr )
  {
    if (qcril_qmi_imsa_info.inited)
    {
       if (qcril_qmi_imsa_info.ims_registered_valid && qcril_qmi_imsa_info.ims_status_change_registered)
       {
          QCRIL_LOG_INFO("ims_registered: %d", qcril_qmi_imsa_info.ims_registered);
          Ims__Registration reg = IMS__REGISTRATION__INIT;
          reg.has_state = TRUE;
          reg.state = qcril_qmi_ims_map_qmi_ims_reg_state_to_ims_reg_state(qcril_qmi_imsa_info.ims_registered);
          qcril_qmi_ims_socket_send(params_ptr->t, IMS__MSG_TYPE__RESPONSE, IMS__MSG_ID__REQUEST_IMS_REGISTRATION_STATE, RIL_E_SUCCESS, &reg, sizeof(reg));
       }
       else
       {
          do
          {
            /* Add entry to ReqList */
            qcril_reqlist_default_entry( params_ptr->t, params_ptr->event_id,
                                         QCRIL_DEFAULT_MODEM_ID,
                                         QCRIL_REQ_AWAITING_CALLBACK,
                                         QCRIL_EVT_NONE, NULL, &reqlist_entry );

            if ( qcril_reqlist_new( instance_id, &reqlist_entry ) != E_SUCCESS )
            {
              /* Fail to add to ReqList */
              qcril_send_empty_payload_request_response( instance_id, params_ptr->t,
                                                         params_ptr->event_id, RIL_E_GENERIC_FAILURE );
              break;
            }

            user_data = QCRIL_COMPOSE_USER_DATA( instance_id, QCRIL_DEFAULT_MODEM_ID,
                                                 reqlist_entry.req_id );

            response_msg = qcril_malloc( sizeof(*response_msg) );
            if( response_msg == NULL )
            {
              qcril_send_empty_payload_request_response( instance_id, params_ptr->t,
                                  params_ptr->event_id, RIL_E_GENERIC_FAILURE );
              break;
            }

            if (qcril_qmi_client_send_msg_async(
                                          QCRIL_QMI_CLIENT_IMSA,
                                          QMI_IMSA_GET_REGISTRATION_STATUS_REQ_V01,
                                          NULL,
                                          0,
                                          response_msg,
                                          sizeof(*response_msg),
                                          (void *) user_data) != QMI_NO_ERR)
            {
              qcril_send_empty_payload_request_response( instance_id, params_ptr->t,
                                  params_ptr->event_id, RIL_E_GENERIC_FAILURE );
              if( response_msg != NULL )
              {
                  qcril_free( response_msg );
              }
            }

          } while(0);
       }
    }
    else
    {
       qcril_send_empty_payload_request_response( instance_id, params_ptr->t, params_ptr->event_id, RIL_E_GENERIC_FAILURE );
    }
  }
  else
  {
    QCRIL_LOG_ERROR("params_ptr is NULL");
  }

  qcril_qmi_imsa_info_unlock();
  QCRIL_LOG_FUNC_RETURN();
} // qcril_qmi_imsa_request_ims_registration_state

//===========================================================================
// qcril_qmi_imsa_request_query_ims_srv_status
//===========================================================================
void qcril_qmi_imsa_request_query_ims_srv_status
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr /*!< Output parameter */
)
{
    qcril_instance_id_e_type instance_id = QCRIL_DEFAULT_INSTANCE_ID;
    uint32 user_data;
    qcril_reqlist_public_type reqlist_entry;
    imsa_get_service_status_resp_msg_v01* response_msg = NULL;
    boolean failed = FALSE;

    QCRIL_NOTUSED( ret_ptr );

    QCRIL_LOG_FUNC_ENTRY();
    qcril_qmi_imsa_info_lock();

    if( NULL != params_ptr )
    {
        if (qcril_qmi_imsa_info.inited)
        {
            if (qcril_qmi_imsa_info.ims_srv_status_valid && qcril_qmi_imsa_info.ims_status_change_registered)
            { // Use cached info to reply back
                Ims__SrvStatusList *ims_srv_status_list_ptr =
                    qcril_qmi_ims_create_ims_srvstatusinfo(&qcril_qmi_imsa_info.ims_srv_status);
                qcril_qmi_ims_socket_send(
                    params_ptr->t,
                    IMS__MSG_TYPE__RESPONSE,
                    IMS__MSG_ID__REQUEST_QUERY_SERVICE_STATUS,
                    IMS__ERROR__E_SUCCESS,
                    ims_srv_status_list_ptr,
                    sizeof(*ims_srv_status_list_ptr) );
                qcril_qmi_ims_free_srvstatuslist(ims_srv_status_list_ptr);
            }
            else
            {
                do
                {
                    /* Add entry to ReqList */
                    qcril_reqlist_default_entry( params_ptr->t, params_ptr->event_id,
                                                 QCRIL_DEFAULT_MODEM_ID,
                                                 QCRIL_REQ_AWAITING_CALLBACK,
                                                 QCRIL_EVT_NONE,
                                                 NULL,
                                                 &reqlist_entry );

                    if ( qcril_reqlist_new( instance_id, &reqlist_entry ) != E_SUCCESS )
                    {
                        /* Fail to add to ReqList */
                        failed = TRUE;
                        break;
                    }
                    user_data = QCRIL_COMPOSE_USER_DATA( instance_id,
                                                         QCRIL_DEFAULT_MODEM_ID,
                                                         reqlist_entry.req_id );

                    response_msg = qcril_malloc( sizeof(*response_msg) );
                    if( response_msg == NULL )
                    {
                        failed = TRUE;
                        break;
                    }

                    if ( qcril_qmi_client_send_msg_async(
                             QCRIL_QMI_CLIENT_IMSA,
                             QMI_IMSA_GET_SERVICE_STATUS_REQ_V01,
                             NULL, 0,
                             response_msg, sizeof(*response_msg),
                             (void *) user_data) != QMI_NO_ERR )
                    {
                        failed = TRUE;
                        break;
                    }
                } while(0);
            }
        }
        else
        {
            failed = TRUE;
        }
        qcril_qmi_imsa_info_unlock();

        if (failed)
        {
            qcril_send_empty_payload_request_response(
                instance_id,
                params_ptr->t,
                params_ptr->event_id,
                RIL_E_GENERIC_FAILURE );

            if( response_msg != NULL )
            {
                qcril_free( response_msg );
            }
        }
    }
    else
    {
        QCRIL_LOG_ERROR("params_ptr is NULL");
    }

    QCRIL_LOG_FUNC_RETURN();
} // qcril_qmi_imsa_request_query_ims_srv_status

//===========================================================================
// qcril_qmi_imsa_is_ims_voip_registered
//===========================================================================
boolean qcril_qmi_imsa_is_ims_voip_in_service()
{
    boolean ret = FALSE;
    qcril_qmi_imsa_info_lock();
    if (qcril_qmi_imsa_info.ims_registered_valid &&
        (IMS__REGISTRATION__REG_STATE__REGISTERED ==
         qcril_qmi_ims_map_qmi_ims_reg_state_to_ims_reg_state(qcril_qmi_imsa_info.ims_registered))&&
        qcril_qmi_imsa_info.ims_srv_status_valid &&
        qcril_qmi_imsa_info.ims_srv_status.voip_service_status_valid &&
        (qcril_qmi_imsa_info.ims_srv_status.voip_service_status != IMSA_NO_SERVICE_V01))
    {
        ret = TRUE;
    }
    qcril_qmi_imsa_info_unlock();
    return ret;
} // qcril_qmi_imsa_is_ims_voip_in_service

//===========================================================================
// qcril_qmi_imsa_get_srv_status
//===========================================================================
qcril_qmi_imsa_srv_status_type qcril_qmi_imsa_get_srv_status()
{
    qcril_qmi_imsa_srv_status_type srv_status;
    qcril_qmi_imsa_info_lock();
    if (qcril_qmi_imsa_info.ims_srv_status_valid)
    {
        memcpy(&srv_status, &qcril_qmi_imsa_info.ims_srv_status, sizeof(srv_status));
    }
    else
    {
        memset(&srv_status, 0, sizeof(srv_status));
    }
    qcril_qmi_imsa_info_unlock();
    return srv_status;
} // qcril_qmi_imsa_get_srv_status
