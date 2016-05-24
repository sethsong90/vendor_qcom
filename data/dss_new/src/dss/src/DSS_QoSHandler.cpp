/*======================================================

FILE:  DSS_QoSHandler.cpp

GENERAL DESCRIPTION:
   Implementation of DSSQoSHandler functions

=====================================================

Copyright (c) 2008 - 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_QoSHandler.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_QoSHandler.h"
#include "DSS_Common.h"
#include "DSS_GenScope.h"
#include "ds_Net_IQoSSecondary.h"
#include "ps_mem.h"
#include "DSS_MemoryManagement.h"

using namespace ds::Net;

DSSQoSHandler::DSSQoSHandler()
{
   mEt = EVENT_HANDLER_QOS;
}

void DSSQoSHandler::EventOccurred()
{
   AEEResult res;

   if (mEdClone->m_Ed->bReg)
   {
      QoSSecondaryStateInfoType statusInfo;
      IQoSSecondary* piNetQoSSecondary = 0;
      DSSWeakRefScope ParentNetAppWeakRefScope;

      if(!ParentNetAppWeakRefScope.Init(parentNetApp)) {
         return;
      }

      res = parentNetApp->GetNetQoSSecondary(&piNetQoSSecondary);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("GetNetQoSSecondary() failed: %d", res, 0, 0);
         return;
      }
      DSSGenScope scopeNetQosSecondary(piNetQoSSecondary, DSSGenScope::IDSIQI_TYPE);

      if (NULL != piNetQoSSecondary)
      {
         res = piNetQoSSecondary->GetState(&statusInfo);
         if (AEE_SUCCESS != res) {
            LOG_MSG_ERROR("GetState() failed: %d", res, 0, 0);
            return;
         }
      }
      else
      {
        return;
      }

      // Dispatch the user callback according to the received event.
      dss_iface_ioctl_event_info_union_type eventInfo;
      dss_iface_ioctl_event_enum_type eventStatus;

      // update the status
      switch(statusInfo.state)
      {
         case QoSSecondaryState::QDS_AVAILABLE_MODIFIED:
            eventStatus = DSS_IFACE_IOCTL_QOS_AVAILABLE_MODIFIED_EV;
            break;
         case QoSSecondaryState::QDS_SUSPENDED:
            eventStatus = DSS_IFACE_IOCTL_QOS_AVAILABLE_DEACTIVATED_EV;
            break;
         case QoSSecondaryState::QDS_UNAVAILABLE:
            eventStatus = DSS_IFACE_IOCTL_QOS_UNAVAILABLE_EV;
            break;
         case QoSSecondaryState::QDS_ACTIVATING:
         case QoSSecondaryState::QDS_SUSPENDING: 
         case QoSSecondaryState::QDS_RELEASING:
         case QoSSecondaryState::QDS_CONFIGURING:
            // These statuses do not require event
            return;
         case QoSSecondaryState::QDS_INVALID: // fallthrough
         default:
            LOG_MSG_ERROR("Unsupported QoSEvent was received: %d", res, 0, 0);
            return;
      }

      // TODO: define a DSSConversion function that makes that translation. This function should be used also in DSSQoSInfoCodeUpdatedHandler.cpp
      //update the info code
      parentNetApp->GetFlowID(&eventInfo.qos_info.handle);
      switch(statusInfo.infoCode)
      {
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

      if (DSS_IFACE_IOCTL_QOS_UNAVAILABLE_EV == eventStatus) {
         DSSNetApp* pNetApp;
         res = parentNetApp->GetDSSNetApp(&pNetApp);
         if (AEE_SUCCESS != res) {
            LOG_MSG_ERROR("GetDSSNetApp() failed: %d", res, 0, 0);
            return;
         }

         res = pNetApp->RemoveDSSNetQoSSecondary(eventInfo.qos_info.handle);
         if (AEE_SUCCESS != res) {
            LOG_MSG_ERROR("RemoveDSSNetQoSSecondary() failed: %d", res, 0, 0);
            //GetDSSNetApp() gets strong ref, need to release
            PS_MEM_RELEASE(pNetApp);
            return;
         }
         //GetDSSNetApp() gets strong ref, need to release
         PS_MEM_RELEASE(pNetApp);
      }

      DispatchCB(eventStatus, mEdClone->m_Ed, &eventInfo);

   }
}

AEEResult DSSQoSHandler::RegisterIDL()
{
   IQoSSecondary* piNetQoSSecondary = NULL;
   DSSWeakRefScope ParentNetAppWeakRefScope;

   if(!ParentNetAppWeakRefScope.Init(parentNetApp)) {
      return AEE_EFAILED;
   }

   IDS_ERR_RET(parentNetApp->GetNetQoSSecondary(&piNetQoSSecondary));
   DSSGenScope scopeNetQosSecondary(piNetQoSSecondary, DSSGenScope::IDSIQI_TYPE);

   LOG_MSG_INFO1("Registering to QDS_EV_STATE_CHANGED, QoSSecondary obj 0x%p", piNetQoSSecondary, 0, 0);
   IDS_ERR_RET(piNetQoSSecondary->OnStateChange(piSignal, QoSSecondaryEvent::QDS_EV_STATE_CHANGED, &mRegObj));

   return DSS_SUCCESS;
}

DSSQoSHandler* DSSQoSHandler::CreateInstance()
{
   return new DSSQoSHandler;
}

