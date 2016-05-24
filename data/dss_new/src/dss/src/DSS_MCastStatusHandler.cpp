/*======================================================

FILE:  DSS_MCastStatusHandler.cpp

GENERAL DESCRIPTION:
   Implementation of DSS_MCastStatusHandler functions

=====================================================

Copyright (c) 2008 - 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_MCastStatusHandler.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "comdef.h"
#include "customer.h"
#include "target.h"
#include "DSS_MCastStatusHandler.h"
#include "DSS_Common.h"
#include "ds_Net_IMCastSession.h"
#include "DSS_MCast.h"
#include "DSS_GenScope.h"
#include "DSS_WeakRefScope.h"

using namespace ds::Net;

DSSMCastStatusHandler::DSSMCastStatusHandler()
{
   mEt = EVENT_HANDLER_MCAST_STATUS;
}

void DSSMCastStatusHandler::EventOccurred()
{

   if (mEdClone->m_Ed->bReg)
   {
      MCastInfoCodeType infoCode;
      IMCastSessionPriv* piNetMCast = 0;
      DSSWeakRefScope ParentNetAppWeakRefScope;

      if(!ParentNetAppWeakRefScope.Init(parentNetApp)) {
         return;
      }

      AEEResult res = parentNetApp->GetMCastSession(&piNetMCast);
      DSSGenScope scopeNetMCast(piNetMCast,DSSGenScope::IDSIQI_TYPE);

      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("GetMCastSession() failed: %d", res, 0, 0);
         return;
      }
      res = piNetMCast->GetTechStatusInfoCode(&infoCode);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("GetTechStatusInfoCode() failed: %d", res, 0, 0);
         return;
      }

      // Dispatch the user callback according to the received event.
      dss_iface_ioctl_event_info_union_type eventInfo;
      dss_iface_ioctl_event_enum_type eventStatus = DSS_IFACE_IOCTL_MCAST_STATUS_EV;

	    parentNetApp->GetMCastHandle(&eventInfo.mcast_info.handle);

      LOG_MSG_INFO3("MCastEvent info code was received: %d", infoCode, 0, 0);
      //update the info code
    
      eventInfo.mcast_info.info_code = (dss_iface_ioctl_mcast_info_code_enum_type)infoCode;

      DispatchCB(eventStatus, mEdClone->m_Ed, &eventInfo);
   }
}

AEEResult DSSMCastStatusHandler::RegisterIDL()
{
   IMCastSessionPriv* piNetMCast = 0;
   DSSWeakRefScope ParentNetAppWeakRefScope;

   if(!ParentNetAppWeakRefScope.Init(parentNetApp)) {
      return AEE_EFAILED;
   }

   IDS_ERR_RET(parentNetApp->GetMCastSession(&piNetMCast));
   DSSGenScope scopeNetMCast(piNetMCast,DSSGenScope::IDSIQI_TYPE);

   LOG_MSG_INFO1("Registering to QDS_EV_TECHNOLOGY_STATUS_PRIV, MCastSessionPriv obj 0x%p", piNetMCast, 0, 0);
   AEEResult res = piNetMCast->OnStateChange(piSignal, MCastSessionPrivEvent::QDS_EV_TECHNOLOGY_STATUS_PRIV, &mRegObj);

   return res;
}

DSSMCastStatusHandler* DSSMCastStatusHandler::CreateInstance()
{
   return new DSSMCastStatusHandler;
}
