/*======================================================

FILE:  DSS_QoSModifyHandler.cpp

GENERAL DESCRIPTION:
   Implementation of DSSQoSModifyHandler functions

=====================================================

Copyright (c) 2008 - 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_QoSModifyHandler.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_QoSModifyHandler.h"
#include "DSS_Common.h"
#include "DSS_GenScope.h"
#include "ds_Net_IQoSSecondary.h"
#include "ps_mem.h"

using namespace ds::Net;

DSSQoSModifyHandler::DSSQoSModifyHandler()
{
   mEt = EVENT_HANDLER_QOS_MODIFY;
}

void DSSQoSModifyHandler::EventOccurred()
{
   if (mEdClone->m_Ed->bReg)
   {
      QoSResultType result;
      IQoSSecondary* piNetQoSSecondary = 0;
      AEEResult res;
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

      res = piNetQoSSecondary->GetModifyResult(&result);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("GetModifyResult() failed: %d", res, 0, 0);
         return;
      }

      // Dispatch the user callback according to the received event.
      dss_iface_ioctl_event_enum_type eventStatus;
      dss_iface_ioctl_event_info_union_type eventInfo;
      memset(&eventInfo, 0, sizeof(dss_iface_ioctl_event_info_union_type));

      // update the status
      switch(result)
      {
         case QoSModifyResult::QDS_ACCEPTED:
            eventStatus = DSS_IFACE_IOCTL_QOS_MODIFY_ACCEPTED_EV;
            break;
         case QoSModifyResult::QDS_REJECTED:
            eventStatus = DSS_IFACE_IOCTL_QOS_MODIFY_REJECTED_EV;
            break;
         default:
            LOG_MSG_ERROR("Unsupported QoSEvent was received: %d", res, 0, 0);
            return;
      }

      DispatchCB(eventStatus, mEdClone->m_Ed, &eventInfo);
   }
}

AEEResult DSSQoSModifyHandler::RegisterIDL()
{
   IQoSSecondary* piNetQoSSecondary = 0;
   DSSWeakRefScope ParentNetAppWeakRefScope;

   if(!ParentNetAppWeakRefScope.Init(parentNetApp)) {
      return AEE_EFAILED;
   }

   IDS_ERR_RET(parentNetApp->GetNetQoSSecondary(&piNetQoSSecondary));
   DSSGenScope scopeNetQosSecondary(piNetQoSSecondary, DSSGenScope::IDSIQI_TYPE);

   LOG_MSG_INFO1("Registering to QDS_EV_MODIFY_RESULT, QoSSecondary obj 0x%p", piNetQoSSecondary, 0, 0);
   AEEResult res = piNetQoSSecondary->OnStateChange(piSignal, QoSEvent::QDS_EV_MODIFY_RESULT, &mRegObj);
   if (AEE_SUCCESS != res) {
      return res;
   }

   return AEE_SUCCESS;
}

DSSQoSModifyHandler* DSSQoSModifyHandler::CreateInstance()
{
   return new DSSQoSModifyHandler;
}

