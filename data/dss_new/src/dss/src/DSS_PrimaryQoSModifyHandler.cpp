/*======================================================

FILE:  DSS_PrimaryQoSModifyHandler.cpp

GENERAL DESCRIPTION:
   Implementation of DSSPrimaryQoSModifyHandler functions

=====================================================

Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_PrimaryQoSModifyHandler.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_PrimaryQoSModifyHandler.h"
#include "DSS_Common.h"
#include "ps_mem.h"
#include "DSS_GenScope.h"
#include "ds_Errors_Def.h"

using namespace ds::Net;
using namespace ds::Error;

DSSPrimaryQoSModifyHandler::DSSPrimaryQoSModifyHandler()
{
   mEt = EVENT_HANDLER_QOS_MODIFY;
}

void DSSPrimaryQoSModifyHandler::EventOccurred()
{
   if (mEdClone->m_Ed->bReg) {
      QoSResultType result;
      IQoSDefault* piNetQoS = 0;
      DSSWeakRefScope WeakRefScope;

      if(!WeakRefScope.Init(parentNetApp)) {
         return;
      }

      AEEResult res = parentNetApp->GetQoSDefault(&piNetQoS);
      if ((AEE_SUCCESS != res) || (NULL == piNetQoS)) {
         LOG_MSG_ERROR("GetQoSDefault() failed: %d", res, 0, 0);
         return ;
      }
      DSSGenScope scopeQosDefault(piNetQoS,DSSGenScope::IDSIQI_TYPE);
      res = piNetQoS->GetModifyResult(&result);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("GetModifyResult() failed: %d", res, 0, 0);
         return;
      }

      // Dispatch the user callback according to the received event.
      dss_iface_ioctl_event_enum_type eventStatus = DSS_IFACE_IOCTL_PRIMARY_QOS_MODIFY_RESULT_EV;
      dss_iface_ioctl_event_info_union_type eventInfo;

      // update the status
      switch(result) {
         case QoSModifyResult::QDS_ACCEPTED:
            eventInfo.primary_qos_modify_result_info.is_modify_succeeded = true;
            break;
         case QoSModifyResult::QDS_REJECTED:
            eventInfo.primary_qos_modify_result_info.is_modify_succeeded = false;
            break;
         default:
            LOG_MSG_ERROR("Unsupported QoSEvent was received: %d", res, 0, 0);
            return;
      }

      DispatchCB(eventStatus, mEdClone->m_Ed, &eventInfo);
   }
}

AEEResult DSSPrimaryQoSModifyHandler::RegisterIDL()
{
   IQoSDefault* piNetQoSDefault = 0;
   DSSWeakRefScope WeakRefScope;

   if(!WeakRefScope.Init(parentNetApp)) {
      return AEE_EFAILED;
   }
   
   IDS_ERR_RET(parentNetApp->GetQoSDefault(&piNetQoSDefault));
   if (NULL == piNetQoSDefault) {
      return QDS_EINVAL;
   }
   DSSGenScope scopeQosDefault(piNetQoSDefault,DSSGenScope::IDSIQI_TYPE);

   LOG_MSG_INFO1("Registering to QDS_EV_MODIFIED, QoSDefault obj 0x%p", piNetQoSDefault, 0, 0);
   IDS_ERR_RET(piNetQoSDefault->OnStateChange(piSignal, QoSDefaultEvent::QDS_EV_MODIFIED, &mRegObj));
   return AEE_SUCCESS;
}

DSSPrimaryQoSModifyHandler* DSSPrimaryQoSModifyHandler::CreateInstance()
{
   return new DSSPrimaryQoSModifyHandler;
}

