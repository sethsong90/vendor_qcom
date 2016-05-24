/*======================================================

FILE:  DSS_QoSProfileChangedHandler.cpp

GENERAL DESCRIPTION:
   Implementation of DSSQoSProfileChangedHandler functions

=====================================================

Copyright (c) 2008 - 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_QoSProfileChangedHandler.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_QoSProfileChangedHandler.h"
#include "DSS_Common.h"
#include "ds_Net_IMCastSession.h"
#include "DSS_MCast.h"
#include "DSS_IDSNetworkScope.h"
#include "DSS_IDSNetwork1xPrivScope.h"
#include "ds_Net_IQoSManager.h"

using namespace ds::Net;

DSSQoSProfileChangedHandler::DSSQoSProfileChangedHandler()
{
   mEt = EVENT_HANDLER_QOS_PROF_CHNG;
}

void DSSQoSProfileChangedHandler::EventOccurred()
{
   if (mEdClone->m_Ed->bReg)
   {

      dss_iface_ioctl_event_enum_type eventStatus = DSS_IFACE_IOCTL_707_NETWORK_SUPPORTED_QOS_PROFILES_CHANGED_EV;
      dss_iface_ioctl_event_info_union_type eventInfo;
      DSSWeakRefScope WeakRefScope;

      if(!WeakRefScope.Init(parentNetApp)) {
         return;
      }

      memset(&eventInfo, 0, sizeof(dss_iface_ioctl_event_info_union_type));

      DispatchCB(eventStatus, mEdClone->m_Ed, &eventInfo);
   }
}

AEEResult DSSQoSProfileChangedHandler::RegisterIDL()
{
   IQoSManager* piQoSManager = 0;
   AEEResult res = AEE_SUCCESS;
   DSSWeakRefScope WeakRefScope;

   if(!WeakRefScope.Init(parentNetApp)) {
      return AEE_EFAILED;
   }

   IDS_ERR_RET(parentNetApp->GetNetQoSManager(&piQoSManager));

   LOG_MSG_INFO1("Registering to QDS_EV_PROFILES_CHANGED, QoSManager obj 0x%p", piQoSManager, 0, 0);
   res = piQoSManager->OnStateChange(piSignal, QoSMgrEvent::QDS_EV_PROFILES_CHANGED, &mRegObj);

   DSSCommon::ReleaseIf((IQI**)&piQoSManager);

   return res;
}

DSSQoSProfileChangedHandler* DSSQoSProfileChangedHandler::CreateInstance()
{
   return new DSSQoSProfileChangedHandler;
}

