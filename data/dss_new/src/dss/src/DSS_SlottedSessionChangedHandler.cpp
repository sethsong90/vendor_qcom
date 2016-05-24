/*======================================================

FILE:  DSS_SlottedSessionChangedHandler.cpp

GENERAL DESCRIPTION:
   Implementation of DSSSlottedSessionChangedHandler functions

=====================================================

Copyright (c) 2008 - 2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_SlottedSessionChangedHandler.cpp#2 $
  $DateTime: 2011/06/30 17:39:13 $$Author: brijeshd $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_SlottedSessionChangedHandler.h"
#include "DSS_Common.h"
#include "DSS_IDSNetworkScope.h"
#include "DSS_IDSNetwork1xPrivScope.h"

using namespace ds::Net;

DSSSlottedSessionChangedHandler::DSSSlottedSessionChangedHandler()
{
   mEt = EVENT_HANDLER_SLOTT_SESS_CHNG;
}

void DSSSlottedSessionChangedHandler::EventOccurred()
{
   if (mEd->m_Ed && mEd->m_Ed->bReg)
   {
      DSSIDSNetworkScope IDSNetworkScope;
      DSSIDSNetwork1xPrivScope IDSNetwork1xPrivScope;
      dss_iface_ioctl_event_enum_type event;
      dss_iface_ioctl_event_info_union_type eventInfo;
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

      uint32 sci;
      AEEResult res = IDSNetwork1xPrivScope.Fetch()->GetHDRSlottedModeCycleIndex(&sci);
      if (AEE_SUCCESS != res) {
         // TODO: Need to add error message.
         return;
      }

      // memset the eventinfo.
      memset(&eventInfo,0,sizeof(dss_iface_ioctl_event_info_union_type));

      event = DSS_IFACE_IOCTL_707_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_SESSION_CHANGED_EV;
      eventInfo.slotted_mode_info.sm_current_sci = sci;

      DispatchCB(event, mEdClone->m_Ed, &eventInfo);
   }
}

AEEResult DSSSlottedSessionChangedHandler::RegisterIDL()
{
   DSSIDSNetworkScope IDSNetworkScope;
   DSSIDSNetwork1xPrivScope IDSNetwork1xPrivScope;
   DSSWeakRefScope WeakRefScope;

   if(!WeakRefScope.Init(parentNetApp)) {
      return AEE_EFAILED;
   }

   IDS_ERR_RET(IDSNetworkScope.Init(parentNetApp));
   IDS_ERR_RET(IDSNetwork1xPrivScope.Init(IDSNetworkScope.Fetch()));
   
   LOG_MSG_INFO1("Registering to SLOTTED_MODE_CHANGED, Network1xPriv obj 0x%p", IDSNetwork1xPrivScope.Fetch(), 0, 0);
   IDS_ERR_RET(IDSNetwork1xPrivScope.Fetch()->OnStateChange(piSignal, Network1xPrivEvent::SLOTTED_MODE_CHANGED, &mRegObj));
   return AEE_SUCCESS;
}

DSSSlottedSessionChangedHandler* DSSSlottedSessionChangedHandler::CreateInstance()
{
   return new DSSSlottedSessionChangedHandler;
}

