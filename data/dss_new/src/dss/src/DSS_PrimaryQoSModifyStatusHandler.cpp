/*======================================================

FILE:  DSS_PrimaryQoSModifyStatusHandler.cpp

GENERAL DESCRIPTION:
   Implementation of DSSPrimaryQoSModifyStatusHandler functions

=====================================================

Copyright (c) 2008 - 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_PrimaryQoSModifyStatusHandler.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_PrimaryQoSModifyStatusHandler.h"
#include "DSS_Common.h"
#include "DSS_GenScope.h"
#include "ps_mem.h"
#include "ds_Errors_Def.h"

using namespace ds::Net;
using namespace ds::Error;

DSSPrimaryQoSModifyStatusHandler::DSSPrimaryQoSModifyStatusHandler()
{
   mEt = EVENT_HANDLER_QOS_PRI_MOD_STAT;
}

void DSSPrimaryQoSModifyStatusHandler::EventOccurred()
{

   if (mEdClone->m_Ed->bReg)
   {
      QoSResultType result;
      IQoSDefault* piNetQoSDefault = 0;
      DSSWeakRefScope WeakRefScope;

      if(!WeakRefScope.Init(parentNetApp)) {
         return;
      }

      AEEResult res = parentNetApp->GetQoSDefault(&piNetQoSDefault);
      if ((AEE_SUCCESS != res) || (NULL == piNetQoSDefault)) {
         LOG_MSG_ERROR("GetQoSDefault() failed: %d", res, 0, 0);
         return ;
      }
      DSSGenScope scopeQosDefault(piNetQoSDefault,DSSGenScope::IDSIQI_TYPE);

      res = piNetQoSDefault->GetModifyResult(&result);
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

      if (DSS_IFACE_IOCTL_QOS_MODIFY_ACCEPTED_EV == eventStatus ||
         DSS_IFACE_IOCTL_QOS_MODIFY_REJECTED_EV == eventStatus) {
         parentNetApp->RemovePrimaryQoSModifyStatusHandler();
      }

      DispatchCB(eventStatus, mEdClone->m_Ed, &eventInfo);
   }
}

AEEResult DSSPrimaryQoSModifyStatusHandler::RegisterIDL()
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
   
   LOG_MSG_INFO1("Registering to QDS_EV_MODIFY_RESULT, QoSDefault obj 0x%p", piNetQoSDefault, 0, 0);
   IDS_ERR_RET(piNetQoSDefault->OnStateChange(piSignal, QoSEvent::QDS_EV_MODIFY_RESULT, &mRegObj));
   return AEE_SUCCESS;
}

DSSPrimaryQoSModifyStatusHandler* DSSPrimaryQoSModifyStatusHandler::CreateInstance()
{
   return new DSSPrimaryQoSModifyStatusHandler;
}

