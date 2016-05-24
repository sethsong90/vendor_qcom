/*======================================================

FILE:  DSS_MCastRegisterHandler_2_0.cpp

GENERAL DESCRIPTION:
Implementation of DSSMCastRegisterHandler2_0 functions

=====================================================

Copyright (c) 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_MCastRegisterHandler_2_0.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-07-29 vm  Module created.

===========================================================================*/

#include "comdef.h"
#include "customer.h"
#include "target.h"

#include "DSS_MCastRegisterHandler_2_0.h"
#include "DSS_Common.h"
#include "ds_Net_IMCastSessionPriv.h"
#include "DSS_MCast.h"
#include "DSS_GenScope.h"
#include "DSS_MemoryManagement.h"

using namespace ds::Net;
DSSMCastRegisterHandler2_0::DSSMCastRegisterHandler2_0()
{
   mEt = EVENT_HANDLER_MCAST_REGISTER;
}

void DSSMCastRegisterHandler2_0::EventOccurred()
{

   if (mEdClone->m_Ed->bReg)
   {
      MCastStateChangedType stateInfo;
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

      res = piNetMCast->GetRegStateAndInfoCodeBCMCS2_0(&stateInfo);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("GetRegStateAndInfoCodeBCMCS2_0() failed: %d", res, 0, 0);
         return;
      }

      // Dispatch the user callback according to the received event.
      dss_iface_ioctl_event_info_union_type eventInfo;
      dss_iface_ioctl_event_enum_type eventStatus;

      eventInfo.mcast_info.force_dereg_cbacks = false;
      // update the status
      switch(stateInfo.regState) {
         case MCastRegState::MCAST_REGISTER_SUCCESS:
            eventStatus = DSS_IFACE_IOCTL_MCAST_REGISTER_SUCCESS_EV;
            break;
         case MCastRegState::MCAST_REGISTER_FAILURE:
            eventStatus = DSS_IFACE_IOCTL_MCAST_REGISTER_FAILURE_EV;
            eventInfo.mcast_info.force_dereg_cbacks = true;
            break;
         case MCastRegState::MCAST_DEREGISTERED:
            eventStatus = DSS_IFACE_IOCTL_MCAST_DEREGISTERED_EV;
            eventInfo.mcast_info.force_dereg_cbacks = true;
            break;
         default:
            LOG_MSG_ERROR("Unsupported MCastEvent was received: %d", stateInfo.regState, 0, 0);
            return;
      }

      parentNetApp->GetMCastHandle(&eventInfo.mcast_info.handle);

      //update the info code
      LOG_MSG_INFO3("MCastEvent info code was received: %d", stateInfo.infoCode, 0, 0);
      eventInfo.mcast_info.info_code = (dss_iface_ioctl_mcast_info_code_enum_type)stateInfo.infoCode;
   
      if (DSS_IFACE_IOCTL_MCAST_DEREGISTERED_EV == eventStatus) {
         DSSNetApp* pNetApp;
         uint32 MCastHandle;
         res = parentNetApp->GetDSSNetApp(&pNetApp);
         if (AEE_SUCCESS != res) {
            LOG_MSG_ERROR("GetDSSNetApp() failed: %d", res, 0, 0);
            return;
         }

         parentNetApp->GetMCastHandle(&MCastHandle);

         res = pNetApp->RemoveDSSMCast
                        (
                           MCastHandle,
                           MCastSessionPrivEvent::QDS_EV_REGISTRATION_STATE_PRIV_BCMCS2_0
                        );
         if (AEE_SUCCESS != res) {
            LOG_MSG_ERROR("RemoveDSSMCast() failed: %d", res, 0, 0);
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

AEEResult DSSMCastRegisterHandler2_0::RegisterIDL()
{
   IMCastSessionPriv* piNetMCast = 0;
   DSSWeakRefScope ParentNetAppWeakRefScope;

   if(!ParentNetAppWeakRefScope.Init(parentNetApp)) {
      return AEE_EFAILED;
   }

   AEEResult res = parentNetApp->GetMCastSession(&piNetMCast);
   DSSGenScope scopeNetMCast(piNetMCast,DSSGenScope::IDSIQI_TYPE);

   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("GetMCastSession() failed: %d", res, 0, 0);
      return res;
   }
   
   LOG_MSG_INFO1("Registering to QDS_EV_REGISTRATION_STATE_PRIV_BCMCS2_0, MCastSessionPriv obj 0x%p", piNetMCast, 0, 0);
   IDS_ERR_RET(piNetMCast->OnStateChange(piSignal, MCastSessionPrivEvent::QDS_EV_REGISTRATION_STATE_PRIV_BCMCS2_0, &mRegObj));
   return AEE_SUCCESS;
}

DSSMCastRegisterHandler2_0* DSSMCastRegisterHandler2_0::CreateInstance()
{
   return new DSSMCastRegisterHandler2_0;
}
