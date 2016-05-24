/*======================================================

FILE:  DSS_RFConditionsHandler.cpp

GENERAL DESCRIPTION:
   Implementation of DSSRFConditionsHandler functions

=====================================================

Copyright (c) 2008 - 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_RFConditionsHandler.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_RFConditionsHandler.h"
#include "DSS_Common.h"
#include "DSS_IDSNetworkPrivScope.h"

using namespace ds::Net;

DSSRFConditionsHandler::DSSRFConditionsHandler()
{
   mEt = EVENT_HANDLER_RFC_COND;
}

void DSSRFConditionsHandler::EventOccurred()
{
   if (mEdClone->m_Ed->bReg) {
      dss_iface_ioctl_event_info_union_type eventInfo;
      DSSWeakRefScope WeakRefScope;

      if(!WeakRefScope.Init(parentNetApp)) {
         return;
      }

      AEEResult res = parentNetApp->GetRFConditions(&eventInfo.rf_conditions_change_info);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("GetRFConditions() failed: %d", res, 0, 0);
         return;
      }
      DispatchCB(DSS_IFACE_IOCTL_RF_CONDITIONS_CHANGED_EV, mEdClone->m_Ed, &eventInfo);
   }
}

AEEResult DSSRFConditionsHandler::RegisterIDL()
{
   DSSIDSNetworkPrivScope IDSNetworkPrivScope;
   DSSWeakRefScope WeakRefScope;

   if(!WeakRefScope.Init(parentNetApp)) {
      return AEE_EFAILED;
   }
   IDS_ERR_RET(IDSNetworkPrivScope.Init(parentNetApp));
   LOG_MSG_INFO1("Registering to QDS_EV_RF_CONDITIONS_CHANGED, NetworkPriv obj 0x%p", IDSNetworkPrivScope.Fetch(), 0, 0);
   IDS_ERR_RET(IDSNetworkPrivScope.Fetch()->OnStateChange(piSignal, NetworkEvent::QDS_EV_RF_CONDITIONS_CHANGED, &mRegObj));
   return AEE_SUCCESS;
}

DSSRFConditionsHandler* DSSRFConditionsHandler::CreateInstance()
{
   return new DSSRFConditionsHandler;
}

