/*======================================================

FILE:  DSS_QoSInfoCodeUpdatedHandler.cpp

GENERAL DESCRIPTION:
   Implementation of DSSQoSInfoCodeUpdatedHandler functions

=====================================================

Copyright (c) 2008 - 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_QoSInfoCodeUpdatedHandler.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_QoSInfoCodeUpdatedHandler.h"
#include "DSS_Common.h"
#include "DSS_GenScope.h"
#include "ps_mem.h"

using namespace ds::Net;

DSSQoSInfoCodeUpdatedHandler::DSSQoSInfoCodeUpdatedHandler()
{
   mEt = EVENT_HANDLER_QOS_INF_CODE_UPD;
}

void DSSQoSInfoCodeUpdatedHandler::EventOccurred()
{
   if (mEdClone->m_Ed->bReg) {
      QoSInfoCodeType infoCode;
      IQoSSecondary* piNetQoSSecondary = 0;
      AEEResult res;
      DSSWeakRefScope ParentNetAppWeakRefScope;

      if(!ParentNetAppWeakRefScope.Init(parentNetApp)) {
         return;
      }

      res = parentNetApp->GetNetQoSSecondary(&piNetQoSSecondary);
      DSSGenScope scopeNetQosSecondary(piNetQoSSecondary, DSSGenScope::IDSIQI_TYPE);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("GetNetQoSSecondary() failed: %d", res, 0, 0);
         return;
      }
      res = piNetQoSSecondary->GetUpdatedInfoCode(&infoCode);

      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("GetUpdatedInfoCode() failed: %d", res, 0, 0);
         return;
      }

      // Dispatch the user callback according to the received event.
      dss_iface_ioctl_event_enum_type eventStatus = DSS_IFACE_IOCTL_QOS_INFO_CODE_UPDATED_EV;
      dss_iface_ioctl_event_info_union_type eventInfo;

      parentNetApp->GetFlowID(&eventInfo.qos_info.handle);
      // update the info code
      switch(infoCode) {
         // TODO: move to DSSConversion
         case QoSInfoCode::QDS_NOT_SPECIFIED:
            eventInfo.qos_info.info_code = DSS_IFACE_IOCTL_EIC_NOT_SPECIFIED;
            break;
         case QoSInfoCode::QDS_NOT_SUPPORTED:
            eventInfo.qos_info.info_code = DSS_IFACE_IOCTL_EIC_QOS_NOT_SUPPORTED;
            break;
         case QoSInfoCode::QDS_NOT_AVAILABLE:
            eventInfo.qos_info.info_code = DSS_IFACE_IOCTL_EIC_QOS_NOT_AVAILABLE;
            break;
         case QoSInfoCode::QDS_NOT_GUARANTEED:
            eventInfo.qos_info.info_code = DSS_IFACE_IOCTL_EIC_QOS_NOT_GUARANTEED;
            break;
         case QoSInfoCode::QDS_INVALID_PARAMS:
            eventInfo.qos_info.info_code = DSS_IFACE_IOCTL_EIC_QOS_INVALID_PARAMS;
            break;
         case QoSInfoCode::QDS_INTERNAL_ERROR:
            eventInfo.qos_info.info_code = DSS_IFACE_IOCTL_EIC_QOS_INTERNAL_ERROR;
            break;
         case QoSInfoCode::QDS_INSUFFICIENT_NET_RESOURCES:
            eventInfo.qos_info.info_code = DSS_IFACE_IOCTL_EIC_QOS_INSUFFICIENT_NET_RESOURCES;
            break;
         case QoSInfoCode::QDS_INSUFFICIENT_LOCAL_RESOURCES:
            eventInfo.qos_info.info_code = DSS_IFACE_IOCTL_EIC_QOS_INSUFFICIENT_LOCAL_RESOURCES;
            break;
         case QoSInfoCode::QDS_AWARE_SYSTEM:
            eventInfo.qos_info.info_code = DSS_IFACE_IOCTL_EIC_QOS_AWARE_SYSTEM;
            break;
         case QoSInfoCode::QDS_UNAWARE_SYSTEM:
            eventInfo.qos_info.info_code = DSS_IFACE_IOCTL_EIC_QOS_UNAWARE_SYSTEM;
            break;
         case QoSInfoCode::QDS_REJECTED_OPERATION:
            eventInfo.qos_info.info_code = DSS_IFACE_IOCTL_EIC_QOS_REJECTED_OPERATION;
            break;
         case QoSInfoCode::QDS_TIMED_OUT_OPERATION:
            eventInfo.qos_info.info_code = DSS_IFACE_IOCTL_EIC_QOS_TIMED_OUT_OPERATION;
            break;
         case QoSInfoCode::QDS_WILL_GRANT_WHEN_RESUMED:
            eventInfo.qos_info.info_code = DSS_IFACE_IOCTL_EIC_QOS_WILL_GRANT_WHEN_QOS_RESUMED;
            break;
         case QoSInfoCode::QDS_INTERNAL_CALL_ENDED:
            eventInfo.qos_info.info_code = DSS_IFACE_IOCTL_EIC_QOS_INTERNAL_CALL_ENDED;
            break;
         case QoSInfoCode::QDS_INTERNAL_UNKNOWN_CAUSE_CODE:
            eventInfo.qos_info.info_code = DSS_IFACE_IOCTL_EIC_QOS_INTERNAL_UNKNOWN_CAUSE_CODE;
            break;
         case QoSInfoCode::QDS_INTERNAL_MODIFY_IN_PROGRESS:
            eventInfo.qos_info.info_code = DSS_IFACE_IOCTL_EIC_QOS_INTERNAL_MODIFY_IN_PROGRESS;
            break;
         case QoSInfoCode::QDS_NETWORK_CALL_ENDED:
            eventInfo.qos_info.info_code = DSS_IFACE_IOCTL_EIC_QOS_NETWORK_CALL_ENDED;
            break;
         case QoSInfoCode::QDS_NETWORK_SVC_NOT_AVAILABLE:
            eventInfo.qos_info.info_code = DSS_IFACE_IOCTL_EIC_QOS_NETWORK_SVC_NOT_AVAILABLE;
            break;
         case QoSInfoCode::QDS_NETWORK_L2_LINK_RELEASED:
            eventInfo.qos_info.info_code = DSS_IFACE_IOCTL_EIC_QOS_NETWORK_L2_LINK_RELEASED;
            break;
         case QoSInfoCode::QDS_NETWORK_L2_LINK_REESTAB_REJ:
            eventInfo.qos_info.info_code = DSS_IFACE_IOCTL_EIC_QOS_NETWORK_L2_LINK_REESTAB_REJ;
            break;
         case QoSInfoCode::QDS_NETWORK_L2_LINK_REESTAB_IND:
            eventInfo.qos_info.info_code = DSS_IFACE_IOCTL_EIC_QOS_NETWORK_L2_LINK_REESTAB_IND;
            break;
         case QoSInfoCode::QDS_NETWORK_UNKNOWN_CAUSE_CODE:
            eventInfo.qos_info.info_code = DSS_IFACE_IOCTL_EIC_QOS_NETWORK_UNKNOWN_CAUSE_CODE;
            break;
         default:
            LOG_MSG_ERROR("Unsupported QoSEvent info code was received: %d", res, 0, 0);
            return;
      }

      DispatchCB(eventStatus, mEdClone->m_Ed, &eventInfo);
   }
}

AEEResult DSSQoSInfoCodeUpdatedHandler::RegisterIDL()
{
   IQoSSecondary* piNetQoSSecondary = NULL;
   DSSWeakRefScope ParentNetAppWeakRefScope;

   if(!ParentNetAppWeakRefScope.Init(parentNetApp)) {
      return AEE_EFAILED;
   }

   IDS_ERR_RET(parentNetApp->GetNetQoSSecondary(&piNetQoSSecondary));
   DSSGenScope scopeNetQosSecondary(piNetQoSSecondary, DSSGenScope::IDSIQI_TYPE);

   LOG_MSG_INFO1("Registering to QDS_EV_INFO_CODE_UPDATED, QoSSecondary obj 0x%p", piNetQoSSecondary, 0, 0);
   AEEResult res = piNetQoSSecondary->OnStateChange(piSignal, QoSEvent::QDS_EV_INFO_CODE_UPDATED, &mRegObj);
   if (AEE_SUCCESS != res) {
      return res;
   }

   return AEE_SUCCESS;
}

DSSQoSInfoCodeUpdatedHandler* DSSQoSInfoCodeUpdatedHandler::CreateInstance()
{
   return new DSSQoSInfoCodeUpdatedHandler;
}

