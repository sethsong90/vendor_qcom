/*======================================================

FILE:  DSS_SlottedResultHandler.cpp

GENERAL DESCRIPTION:
   Implementation of DSSSlottedResultHandler functions

=====================================================

Copyright (c) 2008 - 2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_SlottedResultHandler.cpp#2 $
  $DateTime: 2011/06/30 17:39:13 $$Author: brijeshd $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_SlottedResultHandler.h"
#include "DSS_Common.h"
#include "DSS_IDSNetworkScope.h"
#include "DSS_IDSNetwork1xPrivScope.h"

using namespace ds::Net;

DSSSlottedResultHandler::DSSSlottedResultHandler()
{
   mEt = EVENT_HANDLER_SLOTT_RES;
}

void DSSSlottedResultHandler::EventOccurred()
{
   if (mEd->m_Ed && mEd->m_Ed->bReg) 
   {
      DSSIDSNetworkScope IDSNetworkScope;
      DSSIDSNetwork1xPrivScope IDSNetwork1xPrivScope;
      dss_iface_ioctl_event_enum_type event = 
         DSS_IFACE_IOCTL_707_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_FAILURE_EV; // to avoid Lint error
      dss_iface_ioctl_event_info_union_type eventInfo;
      ds::Net::Network1xPrivResultCodeType resultCode;
      DSSWeakRefScope WeakRefScope;

      if(!WeakRefScope.Init(parentNetApp)) 
      {
         return;
      }
   
      if (AEE_SUCCESS != IDSNetworkScope.Init(parentNetApp)) 
      {
         return;
      }
      if (AEE_SUCCESS != IDSNetwork1xPrivScope.Init(IDSNetworkScope.Fetch())) 
      {
        return;
      }

      AEEResult nRet = IDSNetwork1xPrivScope.Fetch()->GetHDRSlottedModeResult(&resultCode);
      if (AEE_SUCCESS != nRet) 
      {
         // TODO: Need to add error message.
         return;
      }

      // memset the eventinfo.
      memset(&eventInfo,0,sizeof(dss_iface_ioctl_event_info_union_type));

      if (ds::Net::Network1xPrivSlottedMode::REQUEST_SUCCEES == resultCode) 
      {
         event = DSS_IFACE_IOCTL_707_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_SUCCESS_EV;
      }
      if (ds::Net::Network1xPrivSlottedMode::REQUEST_REJECTED == resultCode) 
      {
         event = DSS_IFACE_IOCTL_707_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_FAILURE_EV;
         eventInfo.slotted_mode_info.hdr_slotted_mode_failure_code =
            DSS_IFACE_IOCTL_HDR_SLOTTED_MODE_REQUEST_REJECTED;
      }
      if (ds::Net::Network1xPrivSlottedMode::REQUEST_FAILED_TX == resultCode) 
      {
         event = DSS_IFACE_IOCTL_707_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_FAILURE_EV;
         eventInfo.slotted_mode_info.hdr_slotted_mode_failure_code = 
            DSS_IFACE_IOCTL_HDR_SLOTTED_MODE_REQUEST_FAILED_TX;
      }

       DispatchCB(event, mEdClone->m_Ed, &eventInfo);
    }
}

AEEResult DSSSlottedResultHandler::RegisterIDL()
{
   DSSIDSNetworkScope IDSNetworkScope;
   DSSIDSNetwork1xPrivScope IDSNetwork1xPrivScope;
   DSSWeakRefScope WeakRefScope;

   if(!WeakRefScope.Init(parentNetApp)) {
      return AEE_EFAILED;
   }

   IDS_ERR_RET(IDSNetworkScope.Init(parentNetApp));
   IDS_ERR_RET(IDSNetwork1xPrivScope.Init(IDSNetworkScope.Fetch()));
   
   LOG_MSG_INFO1("Registering to SLOTTED_MODE_RESULT, Network1xPriv obj 0x%p", IDSNetwork1xPrivScope.Fetch(), 0, 0);
   IDS_ERR_RET(IDSNetwork1xPrivScope.Fetch()->OnStateChange(piSignal, Network1xPrivEvent::SLOTTED_MODE_RESULT, &mRegObj));
   return AEE_SUCCESS;
}

DSSSlottedResultHandler* DSSSlottedResultHandler::CreateInstance()
{
   return new DSSSlottedResultHandler;
}

