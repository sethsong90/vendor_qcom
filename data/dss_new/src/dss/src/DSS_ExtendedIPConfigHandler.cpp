/*======================================================

FILE:  DSS_ExtendedIPConfigHandler.cpp

GENERAL DESCRIPTION:
   Implementation of DSS_ExtendedIPConfigHandler functions

=====================================================

Copyright (c) 2008 - 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_ExtendedIPConfigHandler.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_ExtendedIPConfigHandler.h"
#include "DSS_Common.h"
#include "ds_Net_INetworkControl.h"
#include "DSS_IDSNetworkScope.h"
#include "ps_mem.h"

using namespace ds::Net;

DSSExtendedIPConfigHandler::DSSExtendedIPConfigHandler()
{
   mEt = EVENT_HANDLER_EXT_IP_CONFIG;
}

void DSSExtendedIPConfigHandler::EventOccurred()
{
   DSSIDSNetworkScope IDSNetworkScope;
   INetworkControl *pNetworkControl = NULL;
   AEEResult res = AEE_SUCCESS;

   if (mEdClone->m_Ed->bReg) {
      dss_iface_ioctl_event_info_union_type eventInfo;
      DSSWeakRefScope WeakRefScope;

      if(!WeakRefScope.Init(parentNetApp)) {
         return;
      }

      if (AEE_SUCCESS != IDSNetworkScope.Init(parentNetApp)) {
         return;
      }

      res = IDSNetworkScope.Fetch()->QueryInterface(AEEIID_INetworkControl,
                                                    (void**)&pNetworkControl);
      if (AEE_SUCCESS != res) {
        LOG_MSG_ERROR("QueryInterface() for INetworkControl failed: %d", res, 0, 0);
        return;
      }
      res = pNetworkControl->GetDHCPRefreshResult(&eventInfo.extended_ip_config_info);

      DSSCommon::ReleaseIf((IQI**)&pNetworkControl);

      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("GetDHCPRefreshResult() failed: %d", res, 0, 0);
         return;
      }
      DispatchCB(DSS_IFACE_IOCTL_EXTENDED_IP_CONFIG_EV, mEdClone->m_Ed, &eventInfo);
   }
}

AEEResult DSSExtendedIPConfigHandler::RegisterIDL()
{
   DSSIDSNetworkScope IDSNetworkScope;
   INetworkControl *piNetworkControl = 0;
   AEEResult res = AEE_SUCCESS;
   DSSWeakRefScope WeakRefScope;

   if(!WeakRefScope.Init(parentNetApp)) {
      return AEE_EFAILED;
   }

   IDS_ERR_RET(IDSNetworkScope.Init(parentNetApp));

   IDS_ERR_RET(IDSNetworkScope.Fetch()->QueryInterface(AEEIID_INetworkControl,
                                                 (void**)&piNetworkControl));

   LOG_MSG_INFO1("Registering to QDS_EV_EXTENDED_IP_CONFIG, NetworkControl obj 0x%p", piNetworkControl, 0, 0);
   res = piNetworkControl->OnStateChange(piSignal, NetworkControlEvent::QDS_EV_EXTENDED_IP_CONFIG, &mRegObj);

   DSSCommon::ReleaseIf((IQI**)&piNetworkControl);

   return res;
}

DSSExtendedIPConfigHandler* DSSExtendedIPConfigHandler::CreateInstance()
{
   return new DSSExtendedIPConfigHandler;
}

