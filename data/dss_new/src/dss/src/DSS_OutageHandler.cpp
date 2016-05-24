/*======================================================

FILE:  DSS_OutageHandler.cpp

GENERAL DESCRIPTION:
   Implementation of DSS_OutageHandler functions

=====================================================

Copyright (c) 2008 - 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_OutageHandler.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_OutageHandler.h"
#include "DSS_Common.h"
#include "DSS_IDSNetworkScope.h"
#include "DSS_IDSNetworkPrivScope.h"

using namespace ds::Net;

DSSOutageHandler::DSSOutageHandler()
{
   mEt = EVENT_HANDLER_OUTAGE;
}

void DSSOutageHandler::EventOccurred()
{

   DSSIDSNetworkScope IDSNetworkScope;

   if (mEdClone->m_Ed->bReg) {
      DSSWeakRefScope WeakRefScope;

      if(!WeakRefScope.Init(parentNetApp)) {
         return;
      }
      
      if ( AEE_SUCCESS != IDSNetworkScope.Init(parentNetApp) ) {
         return;
      }

      OutageInfoType outageInfo;
      AEEResult res = IDSNetworkScope.Fetch()->GetOutageInfo(&outageInfo);
      if ((AEE_SUCCESS != res) &&
          ((OutageState::QDS_INVALID == outageInfo.state) ||
           (OutageState::QDS_EXPIRED == outageInfo.state))) {
         LOG_MSG_ERROR("GetOutageInfo() failed: %d or unexpected status: %d",
                   res, outageInfo.state, 0);
         return;
      }
      dss_iface_ioctl_event_info_union_type eventInfo;
      eventInfo.outage_notification_info.time_to_outage = outageInfo.timeToOutage;
      eventInfo.outage_notification_info.duration = outageInfo.duration;
      DispatchCB(DSS_IFACE_IOCTL_OUTAGE_NOTIFICATION_EV, mEdClone->m_Ed, &eventInfo);
   }
}

AEEResult DSSOutageHandler::RegisterIDL()
{
   DSSIDSNetworkPrivScope IDSNetworkPrivScope;
   DSSWeakRefScope WeakRefScope;

   if(!WeakRefScope.Init(parentNetApp)) {
      return AEE_EFAILED;
   }

   IDS_ERR_RET(IDSNetworkPrivScope.Init(parentNetApp));
   
   LOG_MSG_INFO1("Registering to QDS_EV_OUTAGE, NetworkPriv obj 0x%p", IDSNetworkPrivScope.Fetch(), 0, 0);
   IDS_ERR_RET(IDSNetworkPrivScope.Fetch()->OnStateChange(piSignal, NetworkEvent::QDS_EV_OUTAGE, &mRegObj));
   return AEE_SUCCESS;
}

DSSOutageHandler* DSSOutageHandler::CreateInstance()
{
   return new DSSOutageHandler;
}

