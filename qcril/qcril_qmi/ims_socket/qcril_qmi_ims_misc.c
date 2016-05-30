/******************************************************************************
  @file    qcril_qmi_ims_misc.c
  @brief   qcril qmi - ims misc

  DESCRIPTION
    Utility functions for ims socket.

  ---------------------------------------------------------------------------

  Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------
******************************************************************************/

#include "qcril_qmi_ims_misc.h"
#include "qcril_log.h"
#include "qcrili.h"

//===========================================================================
// qcril_qmi_ims_map_event_to_request
//===========================================================================
Ims__MsgId qcril_qmi_ims_map_event_to_request(int event)
{
  Ims__MsgId ret;

  switch ( event )
  {
    case QCRIL_EVT_IMS_SOCKET_IMS_REGISTRATION_STATE:
      ret = IMS__MSG_ID__REQUEST_IMS_REGISTRATION_STATE;
      break;

    case QCRIL_EVT_IMS_SOCKET_REQ_DIAL:
      ret = IMS__MSG_ID__REQUEST_DIAL;
      break;

    case QCRIL_EVT_IMS_SOCKET_REQ_ANSWER:
      ret = IMS__MSG_ID__REQUEST_ANSWER;
      break;

    case QCRIL_EVT_IMS_SOCKET_REQ_HANGUP:
      ret = IMS__MSG_ID__REQUEST_HANGUP;
      break;

    case QCRIL_EVT_IMS_SOCKET_REQ_LAST_CALL_FAIL_CAUSE:
      ret = IMS__MSG_ID__REQUEST_LAST_CALL_FAIL_CAUSE;
      break;

    case QCRIL_EVT_IMS_SOCKET_REQ_GET_CURRENT_CALLS:
      ret = IMS__MSG_ID__REQUEST_GET_CURRENT_CALLS;
      break;

    case QCRIL_EVT_IMS_SOCKET_REQ_HANGUP_WAITING_OR_BACKGROUND:
      ret = IMS__MSG_ID__REQUEST_HANGUP_WAITING_OR_BACKGROUND;
      break;

    case QCRIL_EVT_IMS_SOCKET_REQ_HANGUP_FOREGROUND_RESUME_BACKGROUND:
      ret = IMS__MSG_ID__REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND;
      break;

    case QCRIL_EVT_IMS_SOCKET_REQ_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE:
      ret = IMS__MSG_ID__REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE;
      break;

    case QCRIL_EVT_IMS_SOCKET_REQ_CONFERENCE:
        ret = IMS__MSG_ID__REQUEST_CONFERENCE;
        break;

    case QCRIL_EVT_IMS_SOCKET_REQ_EXIT_ECBM:
        ret = IMS__MSG_ID__REQUEST_EXIT_EMERGENCY_CALLBACK_MODE;
        break;

    case QCRIL_EVT_IMS_SOCKET_REQ_DTMF:
    case QCRIL_EVT_IMS_SOCKET_REQ_STOP_CONT_DTMF:
        ret = IMS__MSG_ID__REQUEST_DTMF;
        break;

    case QCRIL_EVT_IMS_SOCKET_REQ_DTMF_START:
        ret = IMS__MSG_ID__REQUEST_DTMF_START;
        break;

    case QCRIL_EVT_IMS_SOCKET_REQ_DTMF_STOP:
        ret = IMS__MSG_ID__REQUEST_DTMF_STOP;
        break;

    case QCRIL_EVT_IMS_SOCKET_REQ_MODIFY_CALL_INITIATE:
        ret = IMS__MSG_ID__REQUEST_MODIFY_CALL_INITIATE;
        break;

    case QCRIL_EVT_IMS_SOCKET_REQ_MODIFY_CALL_CONFIRM:
        ret = IMS__MSG_ID__REQUEST_MODIFY_CALL_CONFIRM;
        break;

    case QCRIL_EVT_IMS_SOCKET_REQ_QUERY_CLIP:
        ret = IMS__MSG_ID__REQUEST_QUERY_CLIP;
        break;

    case QCRIL_EVT_IMS_SOCKET_REQ_GET_CLIR:
        ret = IMS__MSG_ID__REQUEST_GET_CLIR;
        break;

    case QCRIL_EVT_IMS_SOCKET_REQ_SET_CLIR:
        ret = IMS__MSG_ID__REQUEST_SET_CLIR;
        break;

    case QCRIL_EVT_IMS_SOCKET_REQ_QUERY_CALL_FORWARD_STATUS:
        ret = IMS__MSG_ID__REQUEST_QUERY_CALL_FORWARD_STATUS;
        break;

    case QCRIL_EVT_IMS_SOCKET_REQ_SET_CALL_FORWARD_STATUS:
        ret = IMS__MSG_ID__REQUEST_SET_CALL_FORWARD_STATUS;
        break;

    case QCRIL_EVT_IMS_SOCKET_REQ_QUERY_CALL_WAITING:
        ret = IMS__MSG_ID__REQUEST_QUERY_CALL_WAITING;
        break;

    case QCRIL_EVT_IMS_SOCKET_REQ_SET_CALL_WAITING:
        ret = IMS__MSG_ID__REQUEST_SET_CALL_WAITING;
        break;

    case QCRIL_EVT_IMS_SOCKET_REQ_IMS_REG_STATE_CHANGE:
      ret = IMS__MSG_ID__REQUEST_IMS_REG_STATE_CHANGE;
      break;

    case QCRIL_EVT_IMS_SOCKET_REQ_SET_SUPP_SVC_NOTIFICATION:
      ret = IMS__MSG_ID__REQUEST_SET_SUPP_SVC_NOTIFICATION;
      break;

    case QCRIL_EVT_IMS_SOCKET_REQ_ADD_PARTICIPANT:
      ret = IMS__MSG_ID__REQUEST_ADD_PARTICIPANT;
      break;

    case QCRIL_EVT_IMS_SOCKET_REQ_QUERY_SERVICE_STATUS:
      ret = IMS__MSG_ID__REQUEST_QUERY_SERVICE_STATUS;
      break;

    case QCRIL_EVT_IMS_SOCKET_REQ_SET_SERVICE_STATUS:
      ret = IMS__MSG_ID__REQUEST_SET_SERVICE_STATUS;
      break;

    case QCRIL_EVT_IMS_SOCKET_REQ_SUPP_SVC_STATUS:
      ret = IMS__MSG_ID__REQUEST_SUPP_SVC_STATUS;
      break;

    case QCRIL_EVT_IMS_SOCKET_REQ_CALL_DEFLECTION:
      ret = IMS__MSG_ID__REQUEST_DEFLECT_CALL;
      break;

    case QCRIL_EVT_IMS_SOCKET_REQ_QUERY_VT_CALL_QUALITY:
      ret = IMS__MSG_ID__REQUEST_QUERY_VT_CALL_QUALITY;
      break;

    case QCRIL_EVT_IMS_SOCKET_REQ_SET_VT_CALL_QUALITY:
      ret = IMS__MSG_ID__REQUEST_SET_VT_CALL_QUALITY;
      break;

    default:
      QCRIL_LOG_DEBUG("didn't find direct mapping for event %d", event);
      if ( event > QCRIL_EVT_IMS_SOCKET_REQ_BASE && event < QCRIL_EVT_IMS_SOCKET_REQ_MAX )
      {
        ret = event - QCRIL_EVT_IMS_SOCKET_REQ_BASE;
      }
      else
      {
        ret = IMS__MSG_ID__UNKNOWN_REQ;
      }
  }

  QCRIL_LOG_INFO("event %d mapped to ims_msg %d", event, ret);
  return ret;
} // qcril_qmi_ims_map_event_to_request

//===========================================================================
// qcril_qmi_ims_map_ril_error_to_ims_error
//===========================================================================
Ims__Error qcril_qmi_ims_map_ril_error_to_ims_error(int ril_error)
{
  Ims__Error ret;

  switch ( ril_error )
  {
    case RIL_E_SUCCESS:
      ret = IMS__ERROR__E_SUCCESS;
      break;

    case RIL_E_RADIO_NOT_AVAILABLE:
      ret = IMS__ERROR__E_RADIO_NOT_AVAILABLE;
      break;

    case RIL_E_GENERIC_FAILURE:
      ret = IMS__ERROR__E_GENERIC_FAILURE;
      break;

    case RIL_E_REQUEST_NOT_SUPPORTED:
      ret = IMS__ERROR__E_REQUEST_NOT_SUPPORTED;
      break;

    case RIL_E_CANCELLED:
      ret = IMS__ERROR__E_CANCELLED;
      break;

    case RIL_E_UNUSED:
      ret = IMS__ERROR__E_UNUSED;
      break;

    default:
      ret = IMS__ERROR__E_GENERIC_FAILURE;
  }

  QCRIL_LOG_INFO("ril error %d mapped to ims error %d", ril_error, ret);
  return ret;
} // qcril_qmi_ims_map_ril_error_to_ims_error

//===========================================================================
// qcril_qmi_ims_map_ril_reg_state_to_ims_reg_state
//===========================================================================
Ims__Registration__RegState qcril_qmi_ims_map_ril_reg_state_to_ims_reg_state(int state)
{
  Ims__Registration__RegState ret;

  switch ( state )
  {
    case 0:
      ret = IMS__REGISTRATION__REG_STATE__NOT_REGISTERED;
      break;

    case 1:
      ret = IMS__REGISTRATION__REG_STATE__REGISTERED;
      break;

    default:
      ret = IMS__REGISTRATION__REG_STATE__NOT_REGISTERED;
  }

  QCRIL_LOG_INFO("ril reg state %d mapped to ims reg state %d", state, ret);

  return ret;
} // qcril_qmi_ims_map_ril_reg_state_to_ims_reg_state

//===========================================================================
// qcril_qmi_ims_map_ril_call_type_to_ims_call_type
//===========================================================================
Ims__CallType qcril_qmi_ims_map_ril_call_type_to_ims_call_type(RIL_Call_Type call_type)
{
  Ims__CallType ret;

  switch ( call_type )
  {
    case RIL_CALL_TYPE_VOICE:
      ret = IMS__CALL_TYPE__CALL_TYPE_VOICE;
      break;

    case RIL_CALL_TYPE_VS_TX:
      ret = IMS__CALL_TYPE__CALL_TYPE_VT_TX;
      break;

    case RIL_CALL_TYPE_VS_RX:
      ret = IMS__CALL_TYPE__CALL_TYPE_VT_RX;
      break;

    case RIL_CALL_TYPE_VT:
      ret = IMS__CALL_TYPE__CALL_TYPE_VT;
      break;

    case RIL_CALL_TYPE_VT_NODIR:
      ret = IMS__CALL_TYPE__CALL_TYPE_VT_NODIR;
      break;

    default:
      ret = call_type; // do not do any mapping if it does not fill in any above categories.
  }

  QCRIL_LOG_INFO("ril call_type %d mapped to ims call_type %d", call_type, ret);

  return ret;
} // qcril_qmi_ims_map_ril_call_type_to_ims_call_type

//===========================================================================
// qcril_qmi_ims_map_ril_call_domain_to_ims_call_domain
//===========================================================================
Ims__CallDomain qcril_qmi_ims_map_ril_call_domain_to_ims_call_domain(RIL_Call_Domain call_domain)
{
  Ims__CallDomain ret;

  switch ( call_domain )
  {
    case RIL_CALL_DOMAIN_UNKNOWN:
      ret = IMS__CALL_DOMAIN__CALL_DOMAIN_UNKNOWN;
      break;

    case RIL_CALL_DOMAIN_CS:
      ret = IMS__CALL_DOMAIN__CALL_DOMAIN_CS;
      break;

    case RIL_CALL_DOMAIN_PS:
      ret = IMS__CALL_DOMAIN__CALL_DOMAIN_PS;
      break;

    case RIL_CALL_DOMAIN_AUTOMATIC:
      ret = IMS__CALL_DOMAIN__CALL_DOMAIN_AUTOMATIC;
      break;

    default:
      ret = IMS__CALL_DOMAIN__CALL_DOMAIN_UNKNOWN;
  }

  QCRIL_LOG_INFO("ril call_domain %d mapped to ims call_domain %d", call_domain, ret);

  return ret;
} // qcril_qmi_ims_map_ril_call_domain_to_ims_call_domain

//===========================================================================
// qcril_qmi_ims_map_ims_call_type_to_ril_call_type
//===========================================================================
RIL_Call_Type qcril_qmi_ims_map_ims_call_type_to_ril_call_type(boolean has_call_type, Ims__CallType call_type)
{
  Ims__CallType ret = IMS__CALL_TYPE__CALL_TYPE_VOICE;

  if (has_call_type)
  {
     switch ( call_type )
     {
       case IMS__CALL_TYPE__CALL_TYPE_VOICE:
         ret = RIL_CALL_TYPE_VOICE;
         break;

       case IMS__CALL_TYPE__CALL_TYPE_VT_TX:
         ret = RIL_CALL_TYPE_VS_TX;
         break;

       case IMS__CALL_TYPE__CALL_TYPE_VT_RX:
         ret = RIL_CALL_TYPE_VS_RX;
         break;

       case IMS__CALL_TYPE__CALL_TYPE_VT:
         ret = RIL_CALL_TYPE_VT;
         break;

       case IMS__CALL_TYPE__CALL_TYPE_VT_NODIR:
         ret = RIL_CALL_TYPE_VT_NODIR;
         break;

       default:
         ret = RIL_CALL_TYPE_VOICE;
     }
  }

  QCRIL_LOG_INFO("ims has_call_type %d, call_type %d mapped to ril call_type %d", has_call_type, call_type, ret);

  return ret;
} // qcril_qmi_ims_map_ims_call_type_to_ril_call_type

//===========================================================================
// qcril_qmi_ims_map_ims_call_domain_to_ril_call_domain
//===========================================================================
RIL_Call_Domain qcril_qmi_ims_map_ims_call_domain_to_ril_call_domain(boolean has_call_domain, Ims__CallDomain call_domain)
{
  RIL_Call_Domain ret = RIL_CALL_DOMAIN_UNKNOWN;

  if (has_call_domain)
  {
     switch ( call_domain )
     {
       case IMS__CALL_DOMAIN__CALL_DOMAIN_UNKNOWN:
         ret = RIL_CALL_DOMAIN_UNKNOWN;
         break;

       case IMS__CALL_DOMAIN__CALL_DOMAIN_CS:
         ret = RIL_CALL_DOMAIN_CS;
         break;

       case IMS__CALL_DOMAIN__CALL_DOMAIN_PS:
         ret = RIL_CALL_DOMAIN_PS;
         break;

       case IMS__CALL_DOMAIN__CALL_DOMAIN_AUTOMATIC:
         ret = RIL_CALL_DOMAIN_AUTOMATIC;
         break;

       default:
         ret = RIL_CALL_DOMAIN_UNKNOWN;
     }
  }

  QCRIL_LOG_INFO("ims has_call_domain %d, call_domain %d mapped to ims call_domain %d", has_call_domain, call_domain, ret);

  return ret;
} // qcril_qmi_ims_map_ims_call_domain_to_ril_call_domain

//===========================================================================
// qcril_qmi_ims_convert_ims_token_to_ril_token
//===========================================================================
RIL_Token qcril_qmi_ims_convert_ims_token_to_ril_token(uint32_t ims_token)
{
  RIL_Token ret = qcril_malloc(sizeof(uint32_t));
  if (NULL != ret)
  {
    uint32_t *tmp = (uint32_t*) ret;
    *tmp = ims_token ^ 0x80000000;
  }
  return ret;
} // qcril_qmi_ims_convert_ims_token_to_ril_token

//===========================================================================
// qcril_qmi_ims_free_and_convert_ril_token_to_ims_token
//===========================================================================
uint32_t qcril_qmi_ims_free_and_convert_ril_token_to_ims_token(RIL_Token ril_token)
{
  uint32_t ret = 0xFFFFFFFF;
  if (ril_token)
  {
      ret = (*((uint32_t *) ril_token)) ^ 0x80000000;
      QCRIL_LOG_INFO("ims token: %d", ret);
      qcril_free((void*) ril_token);
  }
  else
  {
      QCRIL_LOG_INFO("ril_token is NULL");
  }

  return ret;
} // qcril_qmi_ims_free_and_convert_ril_token_to_ims_token

//===========================================================================
// qcril_qmi_ims_translate_ims_calldetails_to_ril_calldetails
//===========================================================================
void qcril_qmi_ims_translate_ims_calldetails_to_ril_calldetails(const Ims__CallDetails *ims_data, RIL_Call_Details* ril_data)
{
  if (NULL == ril_data || NULL == ims_data)
  {
    QCRIL_LOG_ERROR("ril_data or ims_data is NULL");
    if (NULL != ril_data)
    {
       QCRIL_LOG_INFO("ril_data is not NULL, set it to default value");
       ril_data->callType = qcril_qmi_ims_map_ims_call_type_to_ril_call_type(FALSE, 0);
       ril_data->callDomain = qcril_qmi_ims_map_ims_call_domain_to_ril_call_domain(FALSE, 0);
    }
  }
  else
  {
    ril_data->callType = qcril_qmi_ims_map_ims_call_type_to_ril_call_type(ims_data->has_calltype, ims_data->calltype);
    ril_data->callDomain = qcril_qmi_ims_map_ims_call_domain_to_ril_call_domain(ims_data->has_calldomain, ims_data->calldomain);
  }
} // qcril_qmi_ims_translate_ims_calldetails_to_ril_calldetails

//===========================================================================
// qcril_qmi_ims_translate_ims_calldetails_to_ril_calldetails
//===========================================================================
void qcril_qmi_ims_translate_ril_callmodify_to_ims_callmodify(const RIL_Call_Modify* ril_data, Ims__CallModify* ims_data)
{
  if (NULL == ril_data || NULL == ims_data)
  {
    QCRIL_LOG_ERROR("ril_data or ims_data is NULL");
    if (NULL != ims_data)
    {
       QCRIL_LOG_INFO("ims_data is not NULL, set it to default value");
       ims_data->has_callindex = FALSE;
       qcril_qmi_ims_translate_ril_calldetails_to_ims_calldetails(NULL, ims_data->calldetails);
    }
  }
  else
  {
    ims_data->has_callindex = TRUE;
    ims_data->callindex = ril_data->callIndex;
    qcril_qmi_ims_translate_ril_calldetails_to_ims_calldetails(ril_data->callDetails, ims_data->calldetails);
  }
} // qcril_qmi_ims_translate_ril_callmodify_to_ims_callmodify

//===========================================================================
// qcril_qmi_ims_translate_ril_calldetails_to_ims_calldetails
//===========================================================================
void qcril_qmi_ims_translate_ril_calldetails_to_ims_calldetails(const RIL_Call_Details* ril_data, Ims__CallDetails* ims_data)
{
  if (NULL == ril_data || NULL == ims_data)
  {
    QCRIL_LOG_ERROR("ril_data or ims_data is NULL");
    if (NULL != ims_data)
    {
       QCRIL_LOG_INFO("ims_data is not NULL, set it to default value");
       ims_data->has_calltype = FALSE;
       ims_data->has_calldomain = FALSE;
    }
  }
  else
  {
    ims_data->has_calltype = TRUE;
    ims_data->calltype = qcril_qmi_ims_map_ril_call_type_to_ims_call_type(ril_data->callType);
    ims_data->has_calldomain = TRUE;
    ims_data->calldomain = qcril_qmi_ims_map_ril_call_domain_to_ims_call_domain(ril_data->callDomain);
  }
} // qcril_qmi_ims_translate_ril_calldetails_to_ims_calldetails

//===========================================================================
// qcril_qmi_ims_translate_ril_callforwdinfo_to_ims_callforwdinfo
//===========================================================================
void qcril_qmi_ims_translate_ril_callforwdinfo_to_ims_callforwdinfo(const qcril_qmi_voice_callforwd_info_param_u_type* ril_data, int num, Ims__CallForwardInfoList* ims_data)
{
  int i;

  if (NULL == ril_data || NULL == ims_data || num < 0)
  {
    QCRIL_LOG_ERROR("ril_data or ims_data is NULL, or num < 0");
  }
  else
  {
    Ims__CallForwardInfoList tmp_cfil = IMS__CALL_FORWARD_INFO_LIST__INIT;
    memcpy(ims_data, &tmp_cfil, sizeof(Ims__CallForwardInfoList));
    ims_data->n_info = num;

    if (num > 0)
    {
      ims_data->info = qcril_malloc(sizeof(Ims__CallForwardInfoList__CallForwardInfo*) * num);
      if (ims_data->info)
      {
        Ims__CallForwardInfoList__CallForwardInfo* cfi_array = qcril_malloc(sizeof(Ims__CallForwardInfoList__CallForwardInfo) * num);
        if (cfi_array)
        {
          Ims__CallForwardInfoList__CallForwardInfo tmp_cfil_cfi = IMS__CALL_FORWARD_INFO_LIST__CALL_FORWARD_INFO__INIT;
          for (i = 0; i < num; i++)
          {
            memcpy(&(cfi_array[i]), &tmp_cfil_cfi, sizeof(Ims__CallForwardInfoList__CallForwardInfo));

            cfi_array[i].has_status = TRUE;
            cfi_array[i].status = ril_data[i].status;

            cfi_array[i].has_reason = TRUE;
            cfi_array[i].reason = ril_data[i].reason;

            cfi_array[i].has_service_class = TRUE;
            cfi_array[i].service_class = ril_data[i].service_class;

            cfi_array[i].has_toa = TRUE;
            cfi_array[i].toa = ril_data[i].toa;

            if (ril_data[i].number)
            {
              cfi_array[i].number = qmi_ril_util_str_clone(ril_data[i].number);
            }

            cfi_array[i].has_time_seconds = TRUE;
            cfi_array[i].time_seconds = ril_data[i].no_reply_timer;

            ims_data->info[i] = &(cfi_array[i]);
          }
        }
        else
        {
          QCRIL_LOG_FATAL("malloc failed");
          qcril_free(ims_data->info);
          ims_data->n_info = 0;
        }
      }
      else
      {
        QCRIL_LOG_FATAL("malloc failed");
      }
    }
  }
} // qcril_qmi_ims_translate_ril_calldetails_to_ims_calldetails

//===========================================================================
// qcril_qmi_ims_translate_ril_service_status_class_to_ims_callwaitinginfo
//===========================================================================
void qcril_qmi_ims_translate_ril_service_status_class_to_ims_callwaitinginfo(int service_status, int service_class, Ims__CallWaitingInfo* ims_data)
{
  int i;

  if (NULL == ims_data)
  {
    QCRIL_LOG_ERROR("ims_data is NULL");
  }
  else
  {
    Ims__CallWaitingInfo tmp_cwi = IMS__CALL_WAITING_INFO__INIT;
    memcpy(ims_data, &tmp_cwi, sizeof(Ims__CallWaitingInfo));

    ims_data->has_service_status = TRUE;
    ims_data->service_status = service_status;

    if (service_status)
    {
      ims_data->service_class = qcril_malloc(sizeof(Ims__ServiceClass));
      if (ims_data->service_class)
      {
        Ims__ServiceClass tmp_sc = IMS__SERVICE_CLASS__INIT;
        memcpy(ims_data->service_class, &tmp_sc, sizeof(Ims__ServiceClass));
        ims_data->service_class->has_service_class = TRUE;
        ims_data->service_class->service_class = service_class;
      }
      else
      {
        QCRIL_LOG_FATAL("malloc failed");
      }
    }
  }
} // qcril_qmi_ims_translate_ril_service_status_class_to_ims_callwaitinginfo

//===========================================================================
// qcril_qmi_ims_map_qmi_ims_reg_state_to_ims_reg_state
//===========================================================================
Ims__Registration__RegState qcril_qmi_ims_map_qmi_ims_reg_state_to_ims_reg_state(uint8_t ims_registered)
{
   Ims__Registration__RegState ret;

   if (ims_registered)
   {
      ret = IMS__REGISTRATION__REG_STATE__REGISTERED;
   }
   else
   {
      ret = IMS__REGISTRATION__REG_STATE__NOT_REGISTERED;
   }
   QCRIL_LOG_INFO("qmi ims_reg_state %d mapped to ims ims_reg_state %d", ims_registered, ret);

   return ret;
} // qcril_qmi_ims_map_qmi_ims_reg_state_to_ims_reg_state

//===========================================================================
// qcril_qmi_ims_translate_ril_suppsvcnotification_to_ims_suppsvcnotification
//===========================================================================
void qcril_qmi_ims_translate_ril_suppsvcnotification_to_ims_suppsvcnotification(const RIL_SuppSvcNotification* ril_data, Ims__SuppSvcNotification* ims_data)
{
  if (NULL == ril_data || NULL == ims_data)
  {
    QCRIL_LOG_ERROR("ril_data or ims_data is NULL");
    if (NULL != ims_data)
    {
       QCRIL_LOG_INFO("ims_data is not NULL, set it to default value");
       ims_data->has_notificationtype = FALSE;
       ims_data->has_code = FALSE;
       ims_data->has_index = FALSE;
       ims_data->has_type = FALSE;
       ims_data->number = NULL;
    }
  }
  else
  {
    ims_data->has_notificationtype = TRUE;
    ims_data->notificationtype = ril_data->notificationType;

    ims_data->has_code = TRUE;
    ims_data->code = ril_data->code;

    ims_data->has_index = TRUE;
    ims_data->index = ril_data->index;

    ims_data->has_type = TRUE;
    ims_data->type = ril_data->type;

    ims_data->number = qmi_ril_util_str_clone(ril_data->number);
  }
}

//===========================================================================
// qcril_qmi_ims_translate_ril_callcapabilities_to_ims_srvstatusinfo
//===========================================================================
void qcril_qmi_ims_translate_ril_callcapabilities_to_ims_srvstatusinfo(const voice_ip_call_capabilities_info_type_v02* ril_data, Ims__SrvStatusList* ims_data)
{
  Ims__Info tmp_ims_info = IMS__INFO__INIT;

  if (NULL == ril_data || NULL == ims_data)
  {
    QCRIL_LOG_ERROR("ril_data or ims_data is NULL");
  }
  else
  {
    ims_data->srvstatusinfo = qcril_malloc( 2 * sizeof(Ims__Info*) ); // 2 - one for voip capability and one for VT capability

    if(NULL != ims_data->srvstatusinfo)
    {
      ims_data->srvstatusinfo[ims_data->n_srvstatusinfo] = qcril_malloc(sizeof(Ims__Info));
      memcpy(ims_data->srvstatusinfo[ims_data->n_srvstatusinfo], &tmp_ims_info, sizeof(Ims__Info));

      ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->has_isvalid = TRUE;
      ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->isvalid = TRUE;

      ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->has_restrictcause = TRUE;
      ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->restrictcause = ril_data->audio_cause;

      if( ( VOICE_CALL_ATTRIB_TX_V02 == ril_data->audio_attrib ) ||
          ( VOICE_CALL_ATTRIB_RX_V02 == ril_data->audio_attrib ) ||
          ( (VOICE_CALL_ATTRIB_TX_V02 | VOICE_CALL_ATTRIB_RX_V02) == ril_data->audio_attrib ) )
      {
        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->has_calltype = TRUE;
        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->calltype = IMS__CALL_TYPE__CALL_TYPE_VOICE;

        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->has_status = TRUE;
        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->status = IMS__STATUS_TYPE__STATUS_ENABLED;
      }
      else
      {
        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->has_calltype = TRUE;
        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->calltype = IMS__CALL_TYPE__CALL_TYPE_VOICE;

        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->has_status = TRUE;
        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->status = IMS__STATUS_TYPE__STATUS_DISABLED;
      }

      if( NULL != ims_data->srvstatusinfo[ims_data->n_srvstatusinfo] )
      {
        QCRIL_LOG_INFO("%d - calltype: %d status: %d restrictcause: %d", ims_data->n_srvstatusinfo,
                                       ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->calltype,
                                       ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->status,
                                       ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->restrictcause);
      }
      ims_data->n_srvstatusinfo++;

      ims_data->srvstatusinfo[ims_data->n_srvstatusinfo] = qcril_malloc(sizeof(Ims__Info));
      memcpy(ims_data->srvstatusinfo[ims_data->n_srvstatusinfo], &tmp_ims_info, sizeof(Ims__Info));

      ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->has_isvalid = TRUE;
      ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->isvalid = TRUE;

      ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->has_restrictcause = TRUE;
      ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->restrictcause = ril_data->video_cause;

      if( VOICE_CALL_ATTRIB_TX_V02 == ril_data->video_attrib )
      {
        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->has_calltype = TRUE;
        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->calltype = IMS__CALL_TYPE__CALL_TYPE_VT_TX;

        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->has_status = TRUE;
        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->status = IMS__STATUS_TYPE__STATUS_PARTIALLY_ENABLED;
      }
      else if ( VOICE_CALL_ATTRIB_RX_V02 == ril_data->video_attrib )
      {
        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->has_calltype = TRUE;
        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->calltype = IMS__CALL_TYPE__CALL_TYPE_VT_RX;

        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->has_status = TRUE;
        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->status = IMS__STATUS_TYPE__STATUS_PARTIALLY_ENABLED;
      }
      else if ( (VOICE_CALL_ATTRIB_TX_V02 | VOICE_CALL_ATTRIB_RX_V02) == ril_data->video_attrib )
      {
        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->has_calltype = TRUE;
        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->calltype = IMS__CALL_TYPE__CALL_TYPE_VT;

        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->has_status = TRUE;
        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->status = IMS__STATUS_TYPE__STATUS_ENABLED;
      }
      else
      {
        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->has_calltype = TRUE;
        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->calltype = IMS__CALL_TYPE__CALL_TYPE_VT;

        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->has_status = TRUE;
        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->status = IMS__STATUS_TYPE__STATUS_DISABLED;
      }

      if( NULL != ims_data->srvstatusinfo[ims_data->n_srvstatusinfo] )
      {
         QCRIL_LOG_INFO("%d - calltype: %d status: %d restrictcause: %d", ims_data->n_srvstatusinfo,
                                        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->calltype,
                                        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->status,
                                        ims_data->srvstatusinfo[ims_data->n_srvstatusinfo]->restrictcause);
      }
      ims_data->n_srvstatusinfo++;
    }
  }
}

//===========================================================================
// qcril_qmi_ims_create_ims_info
//===========================================================================
Ims__Info* qcril_qmi_ims_create_ims_info(
    Ims__CallType type,
    imsa_service_status_enum_v01 status,
    boolean rat_valid,
    imsa_service_rat_enum_v01 rat
)
{
    boolean failure = FALSE;
    Ims__Info *ims_info_ptr = NULL;

    do
    {
        ims_info_ptr = qcril_malloc(sizeof(*ims_info_ptr));
        if (NULL == ims_info_ptr)
        {
            failure = TRUE;
            break;
        }
        Ims__Info ims_info_copy = IMS__INFO__INIT;
        memcpy(ims_info_ptr, &ims_info_copy, sizeof(*ims_info_ptr));

        ims_info_ptr->has_isvalid = TRUE;
        ims_info_ptr->isvalid = TRUE;

        ims_info_ptr->has_calltype = TRUE;
        ims_info_ptr->calltype = type;

        ims_info_ptr->n_acctechstatus = 1; // only have one possible rat per current IMSA design
        ims_info_ptr->acctechstatus =
            qcril_malloc( sizeof(*ims_info_ptr->acctechstatus) * ims_info_ptr->n_acctechstatus );
        if (NULL == ims_info_ptr->acctechstatus)
        {
            failure = TRUE;
            break;
        }

        ims_info_ptr->acctechstatus[0] = qcril_malloc(sizeof(*ims_info_ptr->acctechstatus[0]));
        if (NULL == ims_info_ptr->acctechstatus[0])
        {
            failure = TRUE;
            break;
        }
        Ims__StatusForAccessTech ims_status_copy = IMS__STATUS_FOR_ACCESS_TECH__INIT;
        memcpy( ims_info_ptr->acctechstatus[0],
                &ims_status_copy,
                sizeof(*ims_info_ptr->acctechstatus[0]) );

        ims_info_ptr->acctechstatus[0]->registered =
            qcril_malloc(sizeof(*ims_info_ptr->acctechstatus[0]->registered));
        if (NULL == ims_info_ptr->acctechstatus[0]->registered)
        {
            failure = TRUE;
            break;
        }
        Ims__Registration ims_reg_copy = IMS__REGISTRATION__INIT;
        memcpy( ims_info_ptr->acctechstatus[0]->registered,
                &ims_reg_copy,
                sizeof(*ims_info_ptr->acctechstatus[0]->registered) );

        ims_info_ptr->acctechstatus[0]->has_status = TRUE;
        ims_info_ptr->acctechstatus[0]->registered->has_state = TRUE;
        switch (status)
        {
            case IMSA_NO_SERVICE_V01:
                ims_info_ptr->acctechstatus[0]->status = IMS__STATUS_TYPE__STATUS_DISABLED;
                ims_info_ptr->acctechstatus[0]->registered->state = IMS__REGISTRATION__REG_STATE__NOT_REGISTERED;
                break;
            case IMSA_LIMITED_SERVICE_V01:
                ims_info_ptr->acctechstatus[0]->status = IMS__STATUS_TYPE__STATUS_PARTIALLY_ENABLED;
                ims_info_ptr->acctechstatus[0]->registered->state = IMS__REGISTRATION__REG_STATE__REGISTERED;
                break;
            case IMSA_FULL_SERVICE_V01:
                ims_info_ptr->acctechstatus[0]->status = IMS__STATUS_TYPE__STATUS_ENABLED;
                ims_info_ptr->acctechstatus[0]->registered->state = IMS__REGISTRATION__REG_STATE__REGISTERED;
                break;
            default:
                QCRIL_LOG_DEBUG("no matched status");
                ims_info_ptr->acctechstatus[0]->status = IMS__STATUS_TYPE__STATUS_ENABLED;
                ims_info_ptr->acctechstatus[0]->registered->state = IMS__REGISTRATION__REG_STATE__REGISTERED;
        }

        if (rat_valid)
        {
            switch (rat)
            {
                case IMSA_WLAN_V01:
                    ims_info_ptr->acctechstatus[0]->has_networkmode = TRUE;
                    ims_info_ptr->acctechstatus[0]->networkmode = IMS__RADIO_TECH_TYPE__RADIO_TECH_WIFI;
                    break;
                case IMSA_WWAN_V01:
                    ims_info_ptr->acctechstatus[0]->has_networkmode = TRUE;
                    ims_info_ptr->acctechstatus[0]->networkmode = IMS__RADIO_TECH_TYPE__RADIO_TECH_LTE;
                    break;
                default:
                    QCRIL_LOG_DEBUG("no matched rat");
            }
        }
    } while (FALSE);

    if (failure)
    {
        qcril_qmi_ims_free_ims_info(ims_info_ptr);
        ims_info_ptr = NULL;
    }
    else
    {
        QCRIL_LOG_INFO( "calltype: %d %d, n_acctechstatus: %d, networkmode: %d, %d, "
                        "restrictioncause: %d, %d, status: %d, %d, reg state: %d, %d",
                        ims_info_ptr->has_calltype,
                        ims_info_ptr->calltype,
                        ims_info_ptr->n_acctechstatus,
                        ims_info_ptr->acctechstatus[0]->has_networkmode,
                        ims_info_ptr->acctechstatus[0]->networkmode,
                        ims_info_ptr->acctechstatus[0]->has_restrictioncause,
                        ims_info_ptr->acctechstatus[0]->restrictioncause,
                        ims_info_ptr->acctechstatus[0]->has_status,
                        ims_info_ptr->acctechstatus[0]->status,
                        ims_info_ptr->acctechstatus[0]->registered->has_state,
                        ims_info_ptr->acctechstatus[0]->registered->state );
    }
    return ims_info_ptr;
} // qcril_qmi_ims_create_ims_info

//===========================================================================
// qcril_qmi_ims_create_ims_srvstatusinfo
//===========================================================================
Ims__SrvStatusList* qcril_qmi_ims_create_ims_srvstatusinfo(const qcril_qmi_imsa_srv_status_type* qmi_data)
{
    Ims__SrvStatusList* ims_srv_status_list_ptr = NULL;
    if (NULL == qmi_data)
    {
        QCRIL_LOG_DEBUG("qmi_data is NULL");
    }
    else
    {
        boolean failure = FALSE;
        do
        {
            ims_srv_status_list_ptr = qcril_malloc(sizeof(*ims_srv_status_list_ptr));

            if (NULL == ims_srv_status_list_ptr)
            {
                failure = TRUE;
                break;
            }

            Ims__SrvStatusList srv_status_list_copy = IMS__SRV_STATUS_LIST__INIT;
            memcpy( ims_srv_status_list_ptr, &srv_status_list_copy, sizeof(*ims_srv_status_list_ptr) );

            ims_srv_status_list_ptr->n_srvstatusinfo = qmi_data->sms_service_status_valid +
                                                       qmi_data->voip_service_status_valid +
                                                       qmi_data->vt_service_status_valid * 3 ; // we need to fill three types if vt status is valid

            if (ims_srv_status_list_ptr->n_srvstatusinfo > 0)
            {
                ims_srv_status_list_ptr->srvstatusinfo = qcril_malloc( sizeof(Ims__Info*) * ims_srv_status_list_ptr->n_srvstatusinfo );
            }

            if (NULL == ims_srv_status_list_ptr->srvstatusinfo)
            {
                failure = TRUE;
                break;
            }

            int idx = 0;

            if (qmi_data->sms_service_status_valid)
            {
                ims_srv_status_list_ptr->srvstatusinfo[idx] = qcril_qmi_ims_create_ims_info(
                                                                  IMS__CALL_TYPE__CALL_TYPE_SMS,
                                                                  qmi_data->sms_service_status,
                                                                  qmi_data->sms_service_rat_valid,
                                                                  qmi_data->sms_service_rat );
                if (NULL == ims_srv_status_list_ptr->srvstatusinfo[idx])
                {
                    failure = TRUE;
                    break;
                }
                idx++;
            }

            if (qmi_data->voip_service_status_valid)
            {
                ims_srv_status_list_ptr->srvstatusinfo[idx] = qcril_qmi_ims_create_ims_info(
                                                                  IMS__CALL_TYPE__CALL_TYPE_VOICE,
                                                                  qmi_data->voip_service_status,
                                                                  qmi_data->voip_service_rat_valid,
                                                                  qmi_data->voip_service_rat );
                if (NULL == ims_srv_status_list_ptr->srvstatusinfo[idx])
                {
                    failure = TRUE;
                    break;
                }
                idx++;
            }

            if (qmi_data->vt_service_status_valid)
            {
                ims_srv_status_list_ptr->srvstatusinfo[idx] = qcril_qmi_ims_create_ims_info(
                                                                  IMS__CALL_TYPE__CALL_TYPE_VT,
                                                                  qmi_data->vt_service_status,
                                                                  qmi_data->vt_service_rat_valid,
                                                                  qmi_data->vt_service_rat );
                if (NULL == ims_srv_status_list_ptr->srvstatusinfo[idx])
                {
                    failure = TRUE;
                    break;
                }
                idx++;

                ims_srv_status_list_ptr->srvstatusinfo[idx] = qcril_qmi_ims_create_ims_info(
                                                                  IMS__CALL_TYPE__CALL_TYPE_VT_TX,
                                                                  qmi_data->vt_service_status,
                                                                  qmi_data->vt_service_rat_valid,
                                                                  qmi_data->vt_service_rat );
                if (NULL == ims_srv_status_list_ptr->srvstatusinfo[idx])
                {
                    failure = TRUE;
                    break;
                }
                idx++;

                ims_srv_status_list_ptr->srvstatusinfo[idx] = qcril_qmi_ims_create_ims_info(
                                                                  IMS__CALL_TYPE__CALL_TYPE_VT_RX,
                                                                  qmi_data->vt_service_status,
                                                                  qmi_data->vt_service_rat_valid,
                                                                  qmi_data->vt_service_rat );
                if (NULL == ims_srv_status_list_ptr->srvstatusinfo[idx])
                {
                    failure = TRUE;
                    break;
                }
                idx++;
            }
        } while (FALSE);

        if (failure)
        {
            qcril_qmi_ims_free_srvstatuslist(ims_srv_status_list_ptr);
            ims_srv_status_list_ptr = NULL;
        }
    }

    QCRIL_LOG_FUNC_RETURN();
    return ims_srv_status_list_ptr;
} // qcril_qmi_ims_create_ims_srvstatusinfo

//===========================================================================
// qcril_qmi_ims_free_srvstatuslist
//===========================================================================
void qcril_qmi_ims_free_srvstatuslist(Ims__SrvStatusList* ims_srv_status_list_ptr)
{
    if (NULL != ims_srv_status_list_ptr)
    {
        if (NULL != ims_srv_status_list_ptr->srvstatusinfo)
        {
            size_t i;
            for (i = 0; i<ims_srv_status_list_ptr->n_srvstatusinfo; i++)
            {
                if (ims_srv_status_list_ptr->srvstatusinfo[i])
                {
                    qcril_qmi_ims_free_ims_info(ims_srv_status_list_ptr->srvstatusinfo[i]);
                }
            }
            qcril_free(ims_srv_status_list_ptr->srvstatusinfo);
        }
        qcril_free(ims_srv_status_list_ptr);
    }
    else
    {
        QCRIL_LOG_DEBUG("ims_srv_status_list_ptr is NULL")   ;
    }
} // qcril_qmi_ims_free_srvstatuslist

//===========================================================================
// qcril_qmi_ims_free_ims_info
//===========================================================================
void qcril_qmi_ims_free_ims_info(Ims__Info* ims_info_ptr)
{
    if (NULL != ims_info_ptr)
    {
        if (NULL != ims_info_ptr->acctechstatus)
        {
            size_t i;
            for (i=0; i<ims_info_ptr->n_acctechstatus; i++)
            {
                if (NULL != ims_info_ptr->acctechstatus[i])
                {
                    if (NULL != ims_info_ptr->acctechstatus[i]->registered)
                    {
                        qcril_free(ims_info_ptr->acctechstatus[i]->registered);
                    }
                    qcril_free(ims_info_ptr->acctechstatus[i]);
                }
            }
            qcril_free(ims_info_ptr->acctechstatus);
        }
        qcril_free(ims_info_ptr);
    }
    else
    {
        QCRIL_LOG_DEBUG("ims_info_ptr is NULL")   ;
    }
} // qcril_qmi_ims_free_ims_info

//===========================================================================
// qcril_qmi_ims_mics_map_imsa_rat_to_ims_rat
//===========================================================================
Ims__RadioTechType qcril_qmi_ims_map_imsa_rat_to_ims_rat(imsa_service_rat_enum_v01 imsa_rat)
{
    if (IMSA_WLAN_V01 == imsa_rat)
    {
        return IMS__RADIO_TECH_TYPE__RADIO_TECH_WIFI;
    }
    else if (IMSA_IWLAN_V01 == imsa_rat)
    {
        return IMS__RADIO_TECH_TYPE__RADIO_TECH_IWLAN;
    }
    else
    {
        return IMS__RADIO_TECH_TYPE__RADIO_TECH_LTE;
    }
} // qcril_qmi_ims_mics_map_imsa_rat_to_ims_rat

//===========================================================================
// qcril_qmi_ims_create_ims_handover_from_imsa_rat_info
//===========================================================================
Ims__Handover* qcril_qmi_ims_create_ims_handover_from_imsa_rat_info(const imsa_rat_handover_status_info_v01* qmi_data)
{
    Ims__Handover* ims_handover_ptr = NULL;
    if (NULL == qmi_data)
    {
        QCRIL_LOG_DEBUG("qmi_data is NULL");
    }
    else
    {
        boolean failure = FALSE;
        do
        {
            ims_handover_ptr = qcril_malloc(sizeof(*ims_handover_ptr));

            if (NULL == ims_handover_ptr)
            {
                failure = TRUE;
                break;
            }

            ims__handover__init(ims_handover_ptr);

            ims_handover_ptr->has_type = TRUE;
            ims_handover_ptr->type = (IMSA_STATUS_RAT_HO_SUCCESS_V01 == qmi_data->rat_ho_status) ?
                                     IMS__HANDOVER__MSG__TYPE__COMPLETE_SUCCESS :
                                     IMS__HANDOVER__MSG__TYPE__COMPLETE_FAIL;

            ims_handover_ptr->has_srctech = TRUE;
            ims_handover_ptr->srctech = qcril_qmi_ims_map_imsa_rat_to_ims_rat(qmi_data->source_rat);

            ims_handover_ptr->has_targettech = TRUE;
            ims_handover_ptr->targettech = qcril_qmi_ims_map_imsa_rat_to_ims_rat(qmi_data->target_rat);

            if (strlen(qmi_data->cause_code))
            {
                ims_handover_ptr->hoextra = qcril_malloc(sizeof(*ims_handover_ptr->hoextra));
                if (NULL == ims_handover_ptr->hoextra)
                {
                    failure = TRUE;
                    break;
                }

                ims__extra__init(ims_handover_ptr->hoextra);

                ims_handover_ptr->hoextra->has_type = TRUE;
                ims_handover_ptr->hoextra->type = IMS__EXTRA__TYPE__LTE_TO_IWLAN_HO_FAIL;

                ims_handover_ptr->hoextra->has_extrainfo = TRUE;
                ims_handover_ptr->hoextra->extrainfo.len = strlen(qmi_data->cause_code);
                ims_handover_ptr->hoextra->extrainfo.data = qcril_malloc(ims_handover_ptr->hoextra->extrainfo.len);
                if (NULL == ims_handover_ptr->hoextra->extrainfo.data)
                {
                    failure = TRUE;
                    break;
                }
                memcpy( ims_handover_ptr->hoextra->extrainfo.data,
                        qmi_data->cause_code,
                        ims_handover_ptr->hoextra->extrainfo.len );
            }
        } while (FALSE);

        if (failure)
        {
            qcril_qmi_ims_free_ims_handover(ims_handover_ptr);
            ims_handover_ptr = NULL;
        }
    }

    return ims_handover_ptr;
} // qcril_qmi_ims_create_ims_handover_from_imsa_rat_info

//===========================================================================
// qcril_qmi_ims_free_ims_handover
//===========================================================================
void qcril_qmi_ims_free_ims_handover(Ims__Handover* ims_handover_ptr)
{
    if (NULL != ims_handover_ptr)
    {
        if (NULL != ims_handover_ptr->hoextra)
        {
            if (NULL != ims_handover_ptr->hoextra->extrainfo.data)
            {
                qcril_free(ims_handover_ptr->hoextra->extrainfo.data);
            }
            qcril_free(ims_handover_ptr->hoextra);
        }
        qcril_free(ims_handover_ptr);
    }
    else
    {
        QCRIL_LOG_DEBUG("ims_handover_ptr is NULL")   ;
    }
} // qcril_qmi_ims_free_ims_handover

//===========================================================================
// qcril_qmi_ims_map_ril_failcause_to_ims_failcause
//===========================================================================
Ims__CallFailCause qcril_qmi_ims_map_ril_failcause_to_ims_failcause(RIL_LastCallFailCause ril_failcause, int ims_extended_error_code)
{
  Ims__CallFailCause ret = ril_failcause;

  if( CALL_FAIL_ERROR_UNSPECIFIED == ril_failcause )
  {
    switch ( ims_extended_error_code )
    {
      case CALL_END_CAUSE_ANSWERED_ELSEWHERE_V02:
      case CALL_END_CAUSE_CALL_DEFLECTED_V02:
        ret = IMS__CALL_FAIL_CAUSE__CALL_FAIL_ANSWERED_ELSEWHERE;
        break;

      default:
        ret = IMS__CALL_FAIL_CAUSE__CALL_FAIL_ERROR_UNSPECIFIED;
        break;
    }
  }

  QCRIL_LOG_INFO("RIL_LastCallFailCause %d with extended error code %d mapped to Ims__CallFailCause %d", ril_failcause, ims_extended_error_code, ret);
  return ret;
}

/*===========================================================================

  FUNCTION:  qcril_qmi_voice_map_qmi_reason_to_ims_facility

===========================================================================*/
/*!
    @brief
    Maps reason code from QMI Voice Get Call Barring response message
    to corresponding IMS Ims__SuppSvcFacilityType.

    @return
    Success: Ims__SuppSvcFacilityType.
    Error:   0
*/
/*=========================================================================*/
Ims__SuppSvcFacilityType qcril_qmi_voice_map_qmi_reason_to_ims_facility
(
  /* Reason code from QMI Voice Get Call Barring response message */
  voice_cc_sups_result_reason_enum_v02 reason
)
{
  switch (reason)
  {

    /* Bar All Outgoing Calls */
    case VOICE_CC_SUPS_RESULT_REASON_BARR_ALLOUTGOING_V02:
      return IMS__SUPP_SVC_FACILITY_TYPE__FACILITY_BAOC;

    /* Bar All Outgoing International Calls */
    case VOICE_CC_SUPS_RESULT_REASON_BARR_OUTGOINGINT_V02:
      return IMS__SUPP_SVC_FACILITY_TYPE__FACILITY_BAOIC;

    /* Bar all Outgoing International Calls except those
       directed to home PLMN country */
    case VOICE_CC_SUPS_RESULT_REASON_BARR_OUTGOINGINTEXTOHOME_V02:
      return IMS__SUPP_SVC_FACILITY_TYPE__FACILITY_BAOICxH;

    /* Bar All Incoming Calls */
    case VOICE_CC_SUPS_RESULT_REASON_BARR_ALLINCOMING_V02:
      return IMS__SUPP_SVC_FACILITY_TYPE__FACILITY_BAIC;

    /* Bar All Incoming Calls when Roaming outside
       the home PLMN country */
    case VOICE_CC_SUPS_RESULT_REASON_BARR_INCOMINGROAMING_V02:
      return IMS__SUPP_SVC_FACILITY_TYPE__FACILITY_BAICr;

    /* Bar All incoming & outgoing Calls */
    case VOICE_CC_SUPS_RESULT_REASON_BARR_ALLBARRING_V02:
      return IMS__SUPP_SVC_FACILITY_TYPE__FACILITY_BA_ALL;

    /* Bar All Outgoing Calls */
    case VOICE_CC_SUPS_RESULT_REASON_BARR_ALLOUTGOINGBARRING_V02:
      return IMS__SUPP_SVC_FACILITY_TYPE__FACILITY_BA_MO;

    /* Bar All Incoming Calls */
    case VOICE_CC_SUPS_RESULT_REASON_BARR_ALLINCOMINGBARRING_V02:
      return IMS__SUPP_SVC_FACILITY_TYPE__FACILITY_BA_MT;

    /* Bar Specific Incoming Calls */
    case VOICE_CC_SUPS_RESULT_REASON_BARR_INCOMING_NUMBER_V02:
      return IMS__SUPP_SVC_FACILITY_TYPE__FACILITY_BS_MT;

    /* Bar Anonymous Incoming Calls */
    case VOICE_CC_SUPS_RESULT_REASON_BARR_INCOMING_ANONYMOUS_V02:
      return IMS__SUPP_SVC_FACILITY_TYPE__FACILITY_BAICa;

    default:
      return 0;
  }
} /* qcril_qmi_voice_map_qmi_reason_to_ims_facility */
