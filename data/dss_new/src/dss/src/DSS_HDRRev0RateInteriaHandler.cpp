/*======================================================

FILE:  DSSHDRRev0RateInteriaHandler.cpp

GENERAL DESCRIPTION:
   Implementation of DSSHDRRev0RateInteriaHandler functions

=====================================================

Copyright (c) 2008 - 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_HDRRev0RateInteriaHandler.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_HDRRev0RateInteriaHandler.h"
#include "DSS_Common.h"
#include "DSS_IDSNetworkScope.h"
#include "DSS_IDSNetwork1xPrivScope.h"
#include "DSS_IDSNetwork1xScope.h"
#include "ps_mem.h"

#include "ds_Utils_DebugMsg.h"

using namespace ds::Net;
DSSHDRRev0RateInteriaHandler::DSSHDRRev0RateInteriaHandler()
{
  mEt = EVENT_HANDLER_RATE_INTERIA;
}

void DSSHDRRev0RateInteriaHandler::EventOccurred()
{
   DSSIDSNetworkScope IDSNetworkScope;
   DSSIDSNetwork1xPrivScope IDSNetwork1xPrivScope;
   AEEResult res = AEE_SUCCESS;

   if (mEdClone->m_Ed->bReg) {
      DSSWeakRefScope WeakRefScope;

      if(!WeakRefScope.Init(parentNetApp)) {
         return;
      }
      
      if (AEE_SUCCESS != IDSNetworkScope.Init(parentNetApp)) {
         return;
      }

      if (AEE_SUCCESS != IDSNetwork1xPrivScope.Init(IDSNetworkScope.Fetch())) {
         return;
      }

      boolean registrationSuccess;
      // TODO: check if piNetQoS is NULL and return error
      res = IDSNetwork1xPrivScope.Fetch()->GetHDRRev0RateInertiaResult(&registrationSuccess);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("GetUpdatedInfoCode() failed: %d", res, 0, 0);
         return;
      }

      // Dispatch the user callback according to the received event.
      dss_iface_ioctl_event_enum_type eventStatus = registrationSuccess ?
                                                    DSS_IFACE_IOCTL_707_ENABLE_HDR_REV0_RATE_INERTIA_SUCCESS_EV:
                                                    DSS_IFACE_IOCTL_707_ENABLE_HDR_REV0_RATE_INERTIA_FAILURE_EV;
      dss_iface_ioctl_event_info_union_type eventInfo;
      if (registrationSuccess) {
         memset(&eventInfo, 0, sizeof(dss_iface_ioctl_event_info_union_type));
      }
      else {
         Network1xPrivHDRRev0RateInertiaFailureCodeType failureCode;
         res = IDSNetwork1xPrivScope.Fetch()->GetHDRRev0RateInertiaResultInfoCode(&failureCode);
         if(AEE_SUCCESS != res){
            LOG_MSG_ERROR("GetHDRRev0RateInertiaResultInfoCode in DSSHDRRev0RateInteriaHandler::EventOccurred() Faild", 0, 0, 0);
            return;
         }

         res = DSSConversion::IDS2DSInertiaFailureInfoCode(failureCode, &(eventInfo.hdr_rev0_rate_inertia_failure_code));
         if (AEE_SUCCESS != res){
            LOG_MSG_ERROR("Conversion of Failure Info Code Faild", 0, 0, 0);
            return;
         }
      }

      DispatchCB(eventStatus, mEdClone->m_Ed, &eventInfo);
   }
}

AEEResult DSSHDRRev0RateInteriaHandler::RegisterIDL()
{
   DSSIDSNetworkScope IDSNetworkScope;
   DSSIDSNetwork1xPrivScope IDSNetwork1xPrivScope;
   DSSWeakRefScope WeakRefScope;

   if(!WeakRefScope.Init(parentNetApp)) {
      return AEE_EFAILED;
   }

   IDS_ERR_RET(IDSNetworkScope.Init(parentNetApp));
   IDS_ERR_RET(IDSNetwork1xPrivScope.Init(IDSNetworkScope.Fetch()));
   LOG_MSG_INFO1("Registering to HDR_REV0_RATE_INERTIA_RESULT, Network1xPriv obj 0x%p", IDSNetwork1xPrivScope.Fetch(), 0, 0);
   IDS_ERR_RET(IDSNetwork1xPrivScope.Fetch()->OnStateChange(piSignal, Network1xPrivEvent::HDR_REV0_RATE_INERTIA_RESULT, &mRegObj));
   return AEE_SUCCESS;
}

DSSHDRRev0RateInteriaHandler* DSSHDRRev0RateInteriaHandler::CreateInstance()
{
   return new DSSHDRRev0RateInteriaHandler;
}

