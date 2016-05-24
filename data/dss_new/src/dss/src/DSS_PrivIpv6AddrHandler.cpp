/*======================================================

FILE:  DSS_PrivIpv6AddrHandler.cpp

GENERAL DESCRIPTION:
   Implementation of DSSPrivIpv6AddrHandler functions

=====================================================

Copyright (c) 2008 - 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_PrivIpv6AddrHandler.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_Common.h"
#include "ds_Net_IIPv6Address.h"
#include "ps_mem.h"
#include "DSS_PrivIpv6AddrHandler.h"
#include "DSS_GenScope.h"
#include "DSS_MemoryManagement.h"

using namespace ds::Net;

DSSPrivIpv6AddrHandler::DSSPrivIpv6AddrHandler()
{
   mEt = EVENT_HANDLER_IP6PRIV_ADDR;
}

void DSSPrivIpv6AddrHandler::EventOccurred()
{
   AEEResult res;

   if ((mEdClone->m_Ed)->bReg)
   {
      IPv6AddrStateType state;
      IIPv6Address* piNetIpv6Address = 0;
      DSSWeakRefScope WeakRefScope;

      if(!WeakRefScope.Init(parentNetApp)) {
         return;
      }

      res = parentNetApp->GetIDSNetIpv6Address(&piNetIpv6Address);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("GetIDSNetIpv6Address() failed: %d", res, 0, 0);
         return;
      }
      DSSGenScope scopeIPv6Address (piNetIpv6Address,DSSGenScope::IDSIQI_TYPE);

      if (NULL != piNetIpv6Address)
      {
         res = piNetIpv6Address->GetState(&state);
         if (AEE_SUCCESS != res) {
            LOG_MSG_ERROR("GetState() failed: %d", res, 0, 0);
            return;
         }
      }
      else
      {
        return;
      }

      // Dispatch the user callback according to the received event.
      dss_iface_ioctl_event_info_union_type eventInfo;
      dss_iface_ioctl_event_enum_type eventStatus;


      if (IPv6AddrState::PRIV_ADDR_WAITING != state) {
         ::ds::INAddr6Type ip6Addr;
         parentNetApp->GetIsUnique(&eventInfo.priv_ipv6_addr.is_unique);
         if (AEE_SUCCESS != piNetIpv6Address->GetAddress(ip6Addr)) {
            return;
         }
         if (AEE_SUCCESS != DSSConversion::IDS2DSIp6Addr(ip6Addr,&eventInfo.priv_ipv6_addr.ip_addr)){
            return;
         }
      }


      // update the status
      switch(state)
      {
         case IPv6AddrState::PRIV_ADDR_AVAILABLE:
            eventStatus = DSS_IFACE_IOCTL_IPV6_PRIV_ADDR_GENERATED_EV;
            break;
         case IPv6AddrState::PRIV_ADDR_DEPRECATED:
            eventStatus = DSS_IFACE_IOCTL_IPV6_PRIV_ADDR_DEPRECATED_EV;

            break;
         case IPv6AddrState::PRIV_ADDR_DELETED:
            eventStatus = DSS_IFACE_IOCTL_IPV6_PRIV_ADDR_DELETED_EV;
            break;
         case IPv6AddrState::PRIV_ADDR_WAITING:
            // This status do not require event
            return;
         default:
            LOG_MSG_ERROR("Unsupported Ipv6 Address event was received: %d", res, 0, 0);
            return;
      }

      if (DSS_IFACE_IOCTL_IPV6_PRIV_ADDR_DELETED_EV == eventStatus) {
            DSSNetApp* pNetApp;

            res = parentNetApp->GetDSSNetApp(&pNetApp);
            if (AEE_SUCCESS != res) {
            LOG_MSG_ERROR("GetDSSNetApp() failed: %d", res, 0, 0);
               return;
            }

         res = pNetApp->RemoveDSSPrivIpv6Addr(piNetIpv6Address);
         if (AEE_SUCCESS != res) {
            LOG_MSG_ERROR("RemoveDSSPrivIpv6Addr() failed: %d", res, 0, 0);
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

AEEResult DSSPrivIpv6AddrHandler::RegisterIDL()
{
   IIPv6Address* piNetIpv6Address = NULL;
   DSSWeakRefScope WeakRefScope;

   if(!WeakRefScope.Init(parentNetApp)) {
      return AEE_EFAILED;
   }

   IDS_ERR_RET(parentNetApp->GetIDSNetIpv6Address(&piNetIpv6Address));

   LOG_MSG_INFO1("Registering to QDS_EV_STATE_CHANGED, IPv6Address obj 0x%p", piNetIpv6Address, 0, 0);
   IDS_ERR_RET(piNetIpv6Address->OnStateChange(piSignal, IPv6AddrEvent::QDS_EV_STATE_CHANGED, &mRegObj));
   DSSCommon::ReleaseIf((IQI**)&piNetIpv6Address);

   return DSS_SUCCESS;
}

DSSPrivIpv6AddrHandler* DSSPrivIpv6AddrHandler::CreateInstance()
{
   return new DSSPrivIpv6AddrHandler;
}

