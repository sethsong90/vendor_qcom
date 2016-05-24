/*======================================================

FILE:  DSS_NetworkIPHandler.cpp

GENERAL DESCRIPTION:
   Implementation of DSS_NetworkIPHandler functions

=====================================================

Copyright (c) 2008 - 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_NetworkIPHandler.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_NetworkIPHandler.h"
#include "DSS_Common.h"
#include "DSS_IDSNetworkPrivScope.h"

using namespace ds::Net;

DSSNetworkIPHandler::DSSNetworkIPHandler()
{
   mEt = EVENT_HANDLER_NETWORK_IP;
}

void DSSNetworkIPHandler::EventOccurred()
{

   if (mEdClone->m_Ed->bReg) {
      dss_iface_ioctl_event_info_union_type eventInfo;
      DSSWeakRefScope WeakRefScope;

      if(!WeakRefScope.Init(parentNetApp)) {
         return;
      }
      
      // The info code should contain the previous IP address
      AEEResult res = parentNetApp->GetPreviousIPAddress(&eventInfo.addr_change_info);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("GetIPAddress() failed: %d", res, 0, 0);
         return;
      }
      DispatchCB(DSS_IFACE_IOCTL_ADDR_CHANGED_EV, mEdClone->m_Ed, &eventInfo);
   }
}

AEEResult DSSNetworkIPHandler::RegisterIDL()
{
   DSSIDSNetworkPrivScope IDSNetworkPrivScope;
   DSSWeakRefScope WeakRefScope;

   if(!WeakRefScope.Init(parentNetApp)) {
      return AEE_EFAILED;
   }

   IDS_ERR_RET(IDSNetworkPrivScope.Init(parentNetApp));
   
   LOG_MSG_INFO1("Registering to QDS_EV_IP_ADDR_CHANGED, NetworkPriv obj 0x%p", IDSNetworkPrivScope.Fetch(), 0, 0);
   IDS_ERR_RET(IDSNetworkPrivScope.Fetch()->OnStateChange(piSignal, NetworkEvent::QDS_EV_IP_ADDR_CHANGED, &mRegObj));
   return AEE_SUCCESS;
}

DSSNetworkIPHandler* DSSNetworkIPHandler::CreateInstance()
{
   return new DSSNetworkIPHandler;
}

