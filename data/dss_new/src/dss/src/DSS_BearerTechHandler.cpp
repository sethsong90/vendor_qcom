/*======================================================

FILE:  DSS_BearerTechHandler.cpp

GENERAL DESCRIPTION:
   Implementation of DSS_BearerTechHandler functions

=====================================================

Copyright (c) 2008 - 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_BearerTechHandler.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-13 en  History added.

===========================================================================*/

#include "DSS_BearerTechHandler.h"
#include "DSS_Common.h"
#include "DSS_IDSNetworkScope.h"
#include "ds_Net_INetworkExt.h"

using namespace ds::Net;

DSSBearerTechHandler::DSSBearerTechHandler()
{
   mEt = EVENT_HANDLER_BEARER_TECH;
}

void DSSBearerTechHandler::EventOccurred()
{

   if (mEdClone->m_Ed->bReg) {
      dss_iface_ioctl_event_info_union_type eventInfo;
      DSSWeakRefScope WeakRefScope;

      if(!WeakRefScope.Init(parentNetApp)) {
         return;
      }
      
      AEEResult res = parentNetApp->GetBearerTech(&eventInfo.tech_change_info.new_bearer_tech);

      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("GetBearerTech() failed: %d", res, 0, 0);
         return;
      }
      res = parentNetApp->GetPreviousBearerTech(&eventInfo.tech_change_info.old_bearer_tech);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("GetPreviousBearerTech() failed: %d", res, 0, 0);
         return;
      }

      DispatchCB(DSS_IFACE_IOCTL_BEARER_TECH_CHANGED_EV, mEdClone->m_Ed, &eventInfo);
   }
}

AEEResult DSSBearerTechHandler::RegisterIDL()
{
   DSSIDSNetworkScope IDSNetworkScope;
   INetworkExt *piNetworkExt = 0;
   AEEResult res = AEE_SUCCESS;
   DSSWeakRefScope WeakRefScope;

   if(!WeakRefScope.Init(parentNetApp)) {
      return AEE_EFAILED;
   }

   IDS_ERR_RET(IDSNetworkScope.Init(parentNetApp));

   IDS_ERR_RET(IDSNetworkScope.Fetch()->QueryInterface(AEEIID_INetworkExt,
                                                       (void**)&piNetworkExt));

   LOG_MSG_INFO1("Registering to QDS_EV_BEARER_TECH_CHANGED, NetworkExt obj 0x%p", piNetworkExt, 0, 0);
   res = piNetworkExt->OnStateChange(piSignal, NetworkExtEvent::QDS_EV_BEARER_TECH_CHANGED, &mRegObj);

   DSSCommon::ReleaseIf((IQI**)&piNetworkExt);

   return res;
}

DSSBearerTechHandler* DSSBearerTechHandler::CreateInstance()
{
   return new DSSBearerTechHandler;
}

