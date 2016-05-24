/*======================================================

FILE:  DSS_MCastMBMSCtrlHandler.cpp

GENERAL DESCRIPTION:
   Implementation of DSSMCastMBMSCtrlHandler functions

=====================================================

Copyright (c) 2008 - 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_MCastMBMSCtrlHandler.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "comdef.h"
#include "customer.h"
#include "target.h"
#include "DSS_MCastMBMSCtrlHandler.h"
#include "DSS_Common.h"
#include "ds_Net_IMCastSession.h"
#include "ds_Net_IMCastMBMSCtrlPriv.h"
#include "DSS_EventHandlerMCastMBMSCtrl.h"
#include "DSS_NetMCastMBMSCtrl.h"
#include "DSS_GenScope.h"

using namespace ds::Net;

DSSMCastMBMSCtrlHandler::DSSMCastMBMSCtrlHandler()
{
   mEt = EVENT_HANDLER_RATE_INTERIA;
}

void DSSMCastMBMSCtrlHandler::EventOccurred()
{
   if (mEdClone->m_Ed->bReg)
   {
      MBMSStateType status;
      DSSWeakRefScope ParentNetAppWeakRefScope;

      if(!ParentNetAppWeakRefScope.Init(parentNetApp)) {
         return;
      }

      IMCastMBMSCtrlPriv* piNetMCastMBMS = 0;
      AEEResult res = parentNetApp->GetMBMSCtrl(&piNetMCastMBMS);
      DSSGenScope scopeMBMSCtrl(piNetMCastMBMS,DSSGenScope::IDSIQI_TYPE);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("GetMBMSCtrl() failed: %d", res, 0, 0);
         return;
      }
      res = piNetMCastMBMS->GetState(&status);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("GetState() failed: %d", res, 0, 0);
         return;
      }

      // Dispatch the user callback according to the received event.
      dss_iface_ioctl_event_info_union_type eventInfo;
      dss_iface_ioctl_event_enum_type eventStatus;

      // update the status
      switch(status)
      {
         case MBMSState::QDS_ACTIVATED:
            eventStatus = DSS_IFACE_IOCTL_MBMS_CONTEXT_ACT_SUCCESS_EV;
            break;
         case MBMSState::QDS_DEACTIVATED:
            eventStatus = DSS_IFACE_IOCTL_MBMS_CONTEXT_DEACT_SUCCESS_EV;
            break;
         default:
            LOG_MSG_ERROR("Unsupported MCastMBMSEvent was received: %d", res, 0, 0);
            return;
      }
      if (DSS_IFACE_IOCTL_MBMS_CONTEXT_DEACT_SUCCESS_EV != eventStatus) {
         DSSNetApp* pNetApp;
         res = parentNetApp->GetDSSNetApp(&pNetApp);
         if (AEE_SUCCESS != res) {
            LOG_MSG_ERROR("GetDSSNetApp() failed: %d", res, 0, 0);
            return;
         }

         res = pNetApp->RemoveDSSMcastMBMSCtrl(piNetMCastMBMS);
         if (AEE_SUCCESS != res) {
            LOG_MSG_ERROR("RemoveDSSMcastMBMSCtrl() failed: %d", res, 0, 0);
            //GetDSSNetApp() gets strong ref, need to release
            PS_MEM_RELEASE(pNetApp);
            return;
         }

         res = pNetApp->RemoveDSSMcastMBMSCtrl(piNetMCastMBMS);
         if (AEE_SUCCESS != res) {
            LOG_MSG_ERROR("RemoveDSSMcastMBMSCtrl() failed: %d", res, 0, 0);
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

AEEResult DSSMCastMBMSCtrlHandler::RegisterIDL()
{
   IMCastMBMSCtrlPriv* piNetMCastMBMS = 0;
   DSSWeakRefScope ParentNetAppWeakRefScope;

   if(!ParentNetAppWeakRefScope.Init(parentNetApp)) {
      return AEE_EFAILED;
   }

   AEEResult res = parentNetApp->GetMBMSCtrl(&piNetMCastMBMS);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("GetMBMSCtrl() failed: %d", res, 0, 0);
      return res;
   }
   
   LOG_MSG_INFO1("Registering to QDS_EV_ACTIVATE_STATE, MCastMBMSCtrlPriv obj 0x%p", piNetMCastMBMS, 0, 0);
   res = piNetMCastMBMS->OnStateChange(piSignal, MBMSEvent::QDS_EV_ACTIVATE_STATE, &mRegObj);
   DSSGenScope scopeMBMSCtrl(piNetMCastMBMS,DSSGenScope::IDSIQI_TYPE);
   return res;
}

DSSMCastMBMSCtrlHandler* DSSMCastMBMSCtrlHandler::CreateInstance()
{
   return new DSSMCastMBMSCtrlHandler;
}
