/*======================================================

FILE:  DSS_NetIOCTLDSAPI.cpp

SERVICES:

GENERAL DESCRIPTION:
Implementation of Iface IOCTLs related ds functions.

=====================================================

Copyright (c) 2008-2012 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_NetIOCTLDSAPI.cpp#4 $
  $DateTime: 2011/07/11 01:35:14 $$Author: vmordoho $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-06-30 cp  IOCTL changes for Soft AP DHCP server.
  2010-04-18 en  History added.

===========================================================================*/

#include "customer.h"
#include "dssocket.h"


#include "DSS_Conversion.h"
#include "DSS_NetApp.h"
#include "DSS_Globals.h"
#include "DSS_Common.h"
#include "DSS_IDSNetworkPrivScope.h"
#include "DSS_IDSNetworkScope.h"
#include "DSS_IDSNetworkExtScope.h"
#include "DSS_IDSNetwork1xPrivScope.h"
#include "DSS_IDSNetwork1xScope.h"
#include "DSS_IDSNetworkUMTSScope.h"
#include "DSS_MemoryManagement.h"
#include "ds_Net_INetwork1x.h"
#include "ds_Net_INetwork1xPriv.h"
#include "ds_Net_IQoS1x.h"
#include "ds_Net_INetworkFactoryPriv.h"
#include "ds_Net_CNetworkFactoryPriv.h"
#include "ds_Net_INetworkIPv6.h"
#include "ds_Net_INetworkIPv6Priv.h"
#include "ds_Net_INetworkControl.h"
#include "ds_Net_IFirewallManager.h"
#include "ds_Net_INatSession.h"
#include "DSS_GenScope.h"
#include "DSS_QoSRequestExScope.h"
#include "ps_iface_defs.h"
#include "ps_iface_flow.h"
#include "ds_Net_IMCastManagerBCMCS.h"
#include "ds_Sock_AddrUtils.h"
#include "ds_Net_CreateInstance.h"
#include "ds_Net_INetworkUMTS.h"
#include "ds_Net_IMCastManagerMBMSPriv.h"
#include "ds_Net_IBearerInfo.h"
#include "ds_Addr_Def.h"
#include "ds_Net_IMCastManagerBCMCS.h"
#include "ds_Net_IQoSManagerPriv.h"
#include "ds_Net_IQoSSecondariesOutput.h"
#include "ds_Net_IQoSSecondariesInput.h"
#include "ds_Net_IQoSSecondaryPriv.h"
#include "ds_Net_IQoSDefaultPriv.h"
#include "ds_Net_INetworkExt2.h"
#include "ds_Utils_DebugMsg.h"

using namespace ds;
using namespace ds::Net;

#ifndef min
#define min(a,b)  ((a) < (b) ? (a):(b))
#endif

#ifndef max
#define max(a,b)  ((a) < (b) ? (b):(a))
#endif

/* functions for handling different cases */
static sint15 dss_iface_ioctl_get_ipv4_addr
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
  LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_ipv4_addr(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   dss_iface_ioctl_ipv4_addr_type* pIpv4Addr = reinterpret_cast<dss_iface_ioctl_ipv4_addr_type*>(argval_ptr);
   IDS_ERR_RET_ERRNO((*pNetApp)->GetIPv4Addr(pIpv4Addr));
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_ipv6_addr
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_ipv6_addr(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   dss_iface_ioctl_ipv6_addr_type* pIpv6Addr = reinterpret_cast<dss_iface_ioctl_ipv6_addr_type*>(argval_ptr);
   IDS_ERR_RET_ERRNO((*pNetApp)->GetIPv6Addr(pIpv6Addr));
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_ipv4_prim_dns_addr
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_ipv4_prim_dns_addr(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   dss_iface_ioctl_ipv4_prim_dns_addr_type* pIpv4Addr = reinterpret_cast<dss_iface_ioctl_ipv4_prim_dns_addr_type*>(argval_ptr);
   IDS_ERR_RET_ERRNO((*pNetApp)->GetIPv4PrimDnsAddr(pIpv4Addr));
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_ipv6_prim_dns_addr
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_ipv6_prim_dns_addr(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   dss_iface_ioctl_ipv6_prim_dns_addr_type* pIpv6Addr = reinterpret_cast<dss_iface_ioctl_ipv6_prim_dns_addr_type*>(argval_ptr);
   IDS_ERR_RET_ERRNO((*pNetApp)->GetIPv6PrimDnsAddr(pIpv6Addr));
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_ipv4_seco_dns_addr
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_ipv4_seco_dns_addr(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   dss_iface_ioctl_ipv4_seco_dns_addr_type* pIpv4Addr = reinterpret_cast<dss_iface_ioctl_ipv4_seco_dns_addr_type*>(argval_ptr);
   IDS_ERR_RET_ERRNO((*pNetApp)->GetIPv4SecoDnsAddr(pIpv4Addr));
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_ipv6_seco_dns_addr
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_ipv6_seco_dns_addr(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   dss_iface_ioctl_ipv6_seco_dns_addr_type* pIpv6Addr = reinterpret_cast<dss_iface_ioctl_ipv6_seco_dns_addr_type*>(argval_ptr);
   IDS_ERR_RET_ERRNO((*pNetApp)->GetIPv6SecoDnsAddr(pIpv6Addr));
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_all_dns_addrs
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_all_dns_addrs(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   dss_iface_ioctl_get_all_dns_addrs_type* pDnsAddrs = reinterpret_cast<dss_iface_ioctl_get_all_dns_addrs_type*>(argval_ptr);
   IDS_ERR_RET_ERRNO((*pNetApp)->GetAllDnsAddrs(pDnsAddrs));
   return DSS_SUCCESS;
}


static sint15 dss_iface_ioctl_get_mtu
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_mtu(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   dss_iface_ioctl_mtu_type* pMtu = reinterpret_cast<dss_iface_ioctl_mtu_type*>(argval_ptr);
   IDS_ERR_RET_ERRNO((*pNetApp)->GetMtu(pMtu));
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_ip_addr
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_ip_addr(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   dss_iface_ioctl_get_ip_addr_type* pIpAddr = reinterpret_cast<dss_iface_ioctl_get_ip_addr_type*>(argval_ptr);
   IDS_ERR_RET_ERRNO((*pNetApp)->GetIPAddress(pIpAddr));
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_state
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_state(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   dss_iface_ioctl_state_type* pIfaceState = reinterpret_cast<dss_iface_ioctl_state_type*>(argval_ptr);
   IDS_ERR_RET_ERRNO((*pNetApp)->GetIfaceState(pIfaceState));
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_phys_link_state
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno,
   dss_iface_id_type  iface_id
)
{
   IPhysLink* piPhysLink = NULL;
   AEEResult res = AEE_SUCCESS;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_phys_link_state(): iface_id:%u, argval_ptr:0x%p", iface_id, argval_ptr, 0);

   dss_iface_ioctl_phys_link_state_type* pPhyslinkState =
   reinterpret_cast<dss_iface_ioctl_phys_link_state_type*>(argval_ptr);
   PhysLinkStateType ds_state = 0;
   
   res = (*pNetApp)->GetPhysLinkObject(iface_id, &piPhysLink);
   if(AEE_SUCCESS != res){
      if(QDS_ENETDOWN == res){
         *pPhyslinkState = PHYS_LINK_DOWN;
         return DSS_SUCCESS;
      }
      else{
         *dss_errno = DSSConversion::IDS2DSErrorCode(res); 
         return DSS_ERROR;
      }
   }

   DSSGenScope GenScopeObject(piPhysLink, DSSGenScope::IDSIQI_TYPE);

   IDS_ERR_RET_ERRNO(piPhysLink->GetState(&ds_state));


   if (AEE_SUCCESS != DSSConversion::IDS2DSPhyslinkState(ds_state, pPhyslinkState)) {
         LOG_MSG_ERROR("Can't allocate sipServerDomainNames ", 0, 0, 0);
      }

   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_rf_conditions
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_rf_conditions(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   IDS_ERR_RET_ERRNO((*pNetApp)->GetRFConditions(reinterpret_cast<dss_iface_ioctl_rf_conditions_type*>(argval_ptr)));
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_reg_event_cb
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_reg_event_cb(): event: %d", (int)((reinterpret_cast<dss_iface_ioctl_ev_cb_type*>(argval_ptr))->event), 0, 0);
   IDS_ERR_RET_ERRNO((*pNetApp)->RegEventCB(reinterpret_cast<dss_iface_ioctl_ev_cb_type*>(argval_ptr)));
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_dereg_event_cb
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_dereg_event_cb(): event: %d", (int)((reinterpret_cast<dss_iface_ioctl_ev_cb_type*>(argval_ptr))->event), 0, 0);
   IDS_ERR_RET_ERRNO((*pNetApp)->DeregEventCB(reinterpret_cast<dss_iface_ioctl_ev_cb_type*>(argval_ptr)));
   return DSS_SUCCESS;
}

 //TODO: the types are different in the IDS and ds APIs - need to ask
static sint15 dss_iface_ioctl_get_hw_addr
(
   void *argval_ptr,
   DSSIDSNetworkScope* IDSNetworkScope,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_hw_addr(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   HWAddressType idsHwAddress;
   memset(&idsHwAddress, 0, sizeof(HWAddressType));
   //TODO: is the len field an input parameter?
   IDS_ERR_RET_ERRNO(IDSNetworkScope->Fetch()->GetHWAddress(&idsHwAddress));
   dss_iface_ioctl_hw_addr_type* pdssHwAddress =
      reinterpret_cast<dss_iface_ioctl_hw_addr_type*>(argval_ptr);
   if (pdssHwAddress->hw_addr_len < idsHwAddress.len) {
      *dss_errno = DS_EINVAL; //TODO: what's the appropriate error?
      return DSS_ERROR;
   }
   pdssHwAddress->hw_addr_len = idsHwAddress.len; //TODO: ok?
   //TODO: what memmove/memcpy should be used?
   (void)memmove(pdssHwAddress->hw_addr, idsHwAddress.hwAddr, idsHwAddress.len);
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_iface_name
(
   void *argval_ptr,
   DSSIDSNetworkScope* IDSNetworkScope,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_iface_name(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   ds::Net::IfaceNameType idsIface = 0;
   IDS_ERR_RET_ERRNO(IDSNetworkScope->Fetch()->GetIfaceName(&idsIface));
   (void)memmove(argval_ptr, &idsIface, sizeof(dss_iface_ioctl_iface_name_type));
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_device_info
(
   void *argval_ptr,
   DSSIDSNetworkScope* IDSNetworkScope,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_device_info(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   IDS_ERR_RET_ERRNO(IDSNetworkScope->Fetch()->GetDeviceInfo(argval_ptr));
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_ipv4_gateway_info
(
   void *argval_ptr,
   DSSIDSNetworkScope* IDSNetworkScope,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_ipv4_gateway_info(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   IDS_ERR_RET_ERRNO(IDSNetworkScope->Fetch()->GetInterfaceGwAddr(argval_ptr));
   return DSS_SUCCESS;
}


static sint15 dss_iface_ioctl_get_ip_family
(
   void *argval_ptr,
   DSSIDSNetworkScope* IDSNetworkScope,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_ip_family(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   ds::AddrFamilyType ipFamily = 0;
   IDS_ERR_RET_ERRNO(IDSNetworkScope->Fetch()->GetAddressFamily(&ipFamily));
   IDS_ERR_RET_ERRNO(DSSConversion::IDS2DSEnumAddrFamily(ipFamily, reinterpret_cast<dss_iface_ioctl_ip_family_type*>(argval_ptr)));
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_bearer_technology
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_bearer_technology(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   IDS_ERR_RET_ERRNO((*pNetApp)->GetBearerTech(reinterpret_cast<dss_iface_ioctl_bearer_tech_type*>(argval_ptr)));
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_data_bearer_rate
(
   void *argval_ptr,
   DSSIDSNetworkExtScope* IDSNetworkExtScope,
   sint15  *dss_errno
)
{
   IBearerInfo* bearerInfo;
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_data_bearer_rate(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   // The types are identical in the old and new APIs.
   IDS_ERR_RET_ERRNO(IDSNetworkExtScope->Fetch()->GetBearerInfo(&bearerInfo));
   IDS_ERR_RET_ERRNO(bearerInfo->GetRate(reinterpret_cast<BearerTechRateType*>(argval_ptr)));
   return DSS_SUCCESS;
}
static sint15 dss_iface_ioctl_is_laptop_call_active
(
   void *argval_ptr,
   DSSIDSNetworkPrivScope* IDSNetworkPrivScope,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_is_laptop_call_active(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   IDS_ERR_RET_ERRNO(IDSNetworkPrivScope->Fetch()->IsLaptopCallActive(reinterpret_cast<boolean*>(argval_ptr)));
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_sip_serv_addr
(
   void *argval_ptr,
   DSSIDSNetworkScope* IDSNetworkScope,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_sip_serv_addr(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   dss_iface_ioctl_sip_serv_addr_info_type* arg =
      reinterpret_cast<dss_iface_ioctl_sip_serv_addr_info_type*>(argval_ptr);
   int nAvailable = 0;
   // TODO: memory allocation ok? need to check ret value? throws exception?

   ds::IPAddrType* sipServerAddresses = NULL;
   if (arg->count > 0) {
      sipServerAddresses = (ds::IPAddrType*)ps_system_heap_mem_alloc(sizeof(ds::IPAddrType)*(arg->count));
      if (NULL == sipServerAddresses) {
         LOG_MSG_ERROR("Can't allocate sipServerAddresses", 0, 0, 0);
         *dss_errno = DS_ENOMEM;
         return DSS_ERROR;
      }
   }

   DSSGenScope GenScopeObject(sipServerAddresses,DSSGenScope::GEN_SCRATCHPAD_ARRAY);

   IDS_ERR_RET_ERRNO(IDSNetworkScope->Fetch()->GetSIPServerAddr(sipServerAddresses,
                                                                    static_cast<int>(arg->count),
                                                                    &nAvailable));
   arg->count = min(arg->count, (uint32)nAvailable);
   IDS_ERR_RET_ERRNO(DSSConversion::IDS2DSArray(sipServerAddresses, arg->addr_array,
      static_cast<int>(arg->count),
      DSSConversion::IDS2DSIpAddr));

   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_sip_serv_domain_names
(
   void *argval_ptr,
   DSSIDSNetworkScope* IDSNetworkScope,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_sip_serv_domain_names(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   dss_iface_ioctl_sip_serv_domain_name_info_type* arg =
      reinterpret_cast<dss_iface_ioctl_sip_serv_domain_name_info_type*>(argval_ptr);
   int nAvailable = 0;

   DomainName* sipServerDomainNames = NULL;
   if (arg->count > 0) {
      sipServerDomainNames = (DomainName*)ps_system_heap_mem_alloc(sizeof(DomainName)*(arg->count));
      if (NULL == sipServerDomainNames) {
         LOG_MSG_ERROR("Can't allocate sipServerDomainNames ", 0, 0, 0);
         *dss_errno = DS_ENOMEM;
         return DSS_ERROR;
      }
   }

   DSSGenScope GenScopeObject(sipServerDomainNames, DSSGenScope::DS_Network_DomainName_ARRAY_PTR, arg->count);

   IDS_ERR_RET_ERRNO(IDSNetworkScope->Fetch()->GetSIPServerDomainNames(sipServerDomainNames,
                                                          static_cast<int>(arg->count),
                                                          &nAvailable));
   arg->count = min(arg->count, (uint32)nAvailable);
   IDS_ERR_RET_ERRNO(DSSConversion::IDS2DSArray(static_cast<const DomainName*>(sipServerDomainNames),
                                                arg->name_array,
                                                static_cast<int>(arg->count),
                                                DSSConversion::IDS2DSDomainName));

   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_domain_name_search_list
(
   void *argval_ptr,
   DSSIDSNetworkScope* IDSNetworkScope,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_domain_name_search_list(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   dss_iface_ioctl_domain_name_search_list_type* arg =
      reinterpret_cast<dss_iface_ioctl_domain_name_search_list_type*>(argval_ptr);
   int nAvailable = 0;
   // TODO: memory allocation ok? need to check ret value? throws exception?
   DomainName* domainNameSearchList = NULL;
   if (arg->count > 0) {
      domainNameSearchList = (DomainName *)ps_system_heap_mem_alloc(sizeof(DomainName)*(arg->count));

      if (NULL == domainNameSearchList) {
         LOG_MSG_ERROR("Can't allocate domainNameSearchList ", 0, 0, 0);
         *dss_errno = DS_ENOMEM;
         return DSS_ERROR;
      }
   }

   DSSGenScope GenScopeObject(domainNameSearchList, DSSGenScope::DS_Network_DomainName_ARRAY_PTR, arg->count);

   IDS_ERR_RET_ERRNO(IDSNetworkScope->Fetch()->GetDomainNameSearchList(domainNameSearchList,
                                                          static_cast<int>(arg->count),
                                                          &nAvailable));

   arg->count = min((uint32)nAvailable, arg->count);
   IDS_ERR_RET_ERRNO(DSSConversion::IDS2DSArray(static_cast<const DomainName*>(domainNameSearchList),
                                                arg->name_array,
                                                static_cast<int>(arg->count),
                                                DSSConversion::IDS2DSDomainName));

   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_refresh_dhcp_config_info
(
   DSSIDSNetworkScope* IDSNetworkScope,
   sint15  *dss_errno
)
{
   INetworkControl *pNetworkControl = NULL;
   AEEResult res = AEE_SUCCESS;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_refresh_dhcp_config_info(): ", 0, 0, 0);

   IDS_ERR_RET_ERRNO(IDSNetworkScope->Fetch()->QueryInterface(AEEIID_INetworkControl, (void**)&pNetworkControl));

   res = pNetworkControl->RefreshDHCPConfigInfo();

   DSSCommon::ReleaseIf((IQI**)&pNetworkControl);

   if (AEE_SUCCESS != res) {
     *dss_errno = DSSConversion::IDS2DSErrorCode(res);
     return DSS_ERROR;
   }

   return DSS_SUCCESS;
}

#ifdef FEATURE_DATA_PS_DHCP_SERVER_SOFTAP
static sint15 dss_iface_ioctl_dhcp_arp_cache_update
(
   void *argval_ptr,
   DSSIDSNetworkPrivScope* IDSNetworkPrivScope,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_dhcp_arp_cache_update", 0, 0, 0);

   DhcpArpCacheUpdateType dhcpArpCacheUpdateParam;
   dss_iface_ioctl_dhcp_arp_cache_update_type* arg =
      reinterpret_cast<dss_iface_ioctl_dhcp_arp_cache_update_type*>(argval_ptr);

   dhcpArpCacheUpdateParam.ip_addr = arg->ip_addr;
   memcpy(dhcpArpCacheUpdateParam.hw_addr,arg->hw_addr,arg->hw_addr_len);
   dhcpArpCacheUpdateParam.hw_addr_len = arg->hw_addr_len;

   IDS_ERR_RET_ERRNO(IDSNetworkPrivScope->Fetch()->DhcpArpCacheUpdate(&dhcpArpCacheUpdateParam));

   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_dhcp_arp_cache_clear
(
   void *argval_ptr,
   DSSIDSNetworkPrivScope* IDSNetworkPrivScope,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_dhcp_arp_cache_clear", 0, 0, 0);

   DhcpArpCacheClearType dhcpArpCacheClearParam;
   dss_iface_ioctl_dhcp_arp_cache_clear_type* arg =
      reinterpret_cast<dss_iface_ioctl_dhcp_arp_cache_clear_type*>
      (argval_ptr);

   dhcpArpCacheClearParam.ip_addr = arg->ip_addr;
   memcpy(dhcpArpCacheClearParam.hw_addr,arg->hw_addr,arg->hw_addr_len);
   dhcpArpCacheClearParam.hw_addr_len = arg->hw_addr_len;

   IDS_ERR_RET_ERRNO(IDSNetworkPrivScope->Fetch()->DhcpArpCacheClear(&dhcpArpCacheClearParam));

   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_dhcp_get_device_info
(
   void *argval_ptr,
   DSSIDSNetworkPrivScope* IDSNetworkPrivScope,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_dhcp_get_device_info", 0, 0, 0);
   DhcpGetDeviceInfoType DhcpGetDeviceInfoParam;
   dss_iface_ioctl_dhcp_get_device_info_type * arg =
      reinterpret_cast<dss_iface_ioctl_dhcp_get_device_info_type*>(argval_ptr);

   DhcpGetDeviceInfoParam.dev_info =
      reinterpret_cast<DhcpServerConnDevicesInfo*>(arg->device_info);
   DhcpGetDeviceInfoParam.dev_infoLen = arg->num_devices;
   DhcpGetDeviceInfoParam.dev_infoLenReq = arg->num_devices_returned;

   IDS_ERR_RET_ERRNO(IDSNetworkPrivScope->Fetch()->GetDhcpDeviceInfo(&DhcpGetDeviceInfoParam));

   arg->num_devices_returned = DhcpGetDeviceInfoParam.dev_infoLenReq;

   return DSS_SUCCESS;
}
#endif /* FEATURE_DATA_PS_DHCP_SERVER_SOFTAP */

static sint15 dss_iface_ioctl_go_active
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno,
   dss_iface_id_type  iface_id
)
{
   IPhysLink* piPhysLink = NULL;
   AEEResult res = AEE_SUCCESS;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_go_active(): iface_id:%u, argval_ptr:0x%p", iface_id, argval_ptr, 0);

   res = (*pNetApp)->GetPhysLinkObject(iface_id, &piPhysLink);
   if (AEE_SUCCESS != res){
      // these lines are for backward compatibility
      // GetPhysLinkObject was modified to return QDS_ENETDOWN
      // instead of AEE_EFAILED
      if(QDS_ENETDOWN == res){
         *dss_errno = DSSConversion::IDS2DSErrorCode(AEE_EFAILED);
         return DSS_ERROR;
      }
      else{
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         return DSS_ERROR;
      }
   }

   DSSGenScope GenScopeObject(piPhysLink, DSSGenScope::IDSIQI_TYPE);

   IDS_ERR_RET_ERRNO(piPhysLink->GoActive());
   return DSS_SUCCESS;
}
static sint15 dss_iface_ioctl_go_dormant
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno,
   dss_iface_id_type  iface_id
)
{
   IPhysLink* piPhysLink = NULL;
   DormantReasonType  dormantReason = DormantReason::QDS_NONE;
   AEEResult res = AEE_SUCCESS;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_go_dormant(): iface_id:%u, argval_ptr:0x%p", iface_id, argval_ptr, 0);

   res = (*pNetApp)->GetPhysLinkObject(iface_id, &piPhysLink);
   if (AEE_SUCCESS != res){
      // these lines are for backward compatibility
      // GetPhysLinkObject was modified to return QDS_ENETDOWN
      // instead of AEE_EFAILED
      if(QDS_ENETDOWN == res){
        *dss_errno = DSSConversion::IDS2DSErrorCode(AEE_EFAILED);
        return DSS_ERROR;
      }
      else{
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         return DSS_ERROR;
      }
   }
   
   DSSGenScope GenScopeObject(piPhysLink, DSSGenScope::IDSIQI_TYPE);

   IDS_ERR_RET_ERRNO(piPhysLink->GoDormant(dormantReason));
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_go_null
(
   void *argval_ptr,
   DSSIDSNetworkScope* IDSNetworkScope,
   sint15  *dss_errno
)
{
   INetworkControl *pNetworkControl = NULL;
   AEEResult res = AEE_SUCCESS;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_go_null(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   dss_iface_ioctl_null_arg_type* arg = reinterpret_cast<dss_iface_ioctl_null_arg_type*>(argval_ptr);
   ReleaseReasonType nArg = *(reinterpret_cast<int*>(arg));

   IDS_ERR_RET_ERRNO(IDSNetworkScope->Fetch()->QueryInterface(AEEIID_INetworkControl, (void**)&pNetworkControl));

   res = pNetworkControl->GoNull(nArg);

   DSSCommon::ReleaseIf((IQI**)&pNetworkControl);

   if (AEE_SUCCESS != res) {
     *dss_errno = DSSConversion::IDS2DSErrorCode(res);
     return DSS_ERROR;
   }

   return DSS_SUCCESS;
}
static sint15 dss_iface_ioctl_ipcp_dns_opt
(
   void *argval_ptr,
   DSSIDSNetworkPrivScope* IDSNetworkPrivScope,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_ipcp_dns_opt(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   IDS_ERR_RET_ERRNO(IDSNetworkPrivScope->Fetch()->EnableDNSDuringIPCP(*reinterpret_cast<boolean*>(argval_ptr)));
   return DSS_SUCCESS;
}

#ifndef FEATURE_DSS_LINUX
static sint15 dss_iface_ioctl_get_tx_queue_level
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno,
   dss_iface_id_type  iface_id
)
{
   IfaceIdType flowID;
   IQoSDefault* pNetQoSDefault = NULL;
   DSSNetQoSSecondary* pDSSNetQoSSecondary = NULL;
   DSSIDSNetworkScope IDSNetworkScope;
   IQoSSecondary* pNetQoSSecondary = NULL;
   QoSTXQueueLevelType TXQueueLevel;
   dss_iface_ioctl_tx_queue_level_type* pTxQueueLevel = reinterpret_cast<dss_iface_ioctl_tx_queue_level_type*>(argval_ptr);

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_tx_queue_level(): iface_id:%u, argval_ptr:0x%p", iface_id, argval_ptr, 0);

   // Initialize the TXQueueLevel struct
   TXQueueLevel.currentNewDataCnt = 0;
   TXQueueLevel.totalPendingCnt = 0;
   TXQueueLevel.wmFreeCnt = 0;

   // check if the appropriate QoS to be used is the primary.
   IDS_ERR_RET_ERRNO(IDSNetworkScope.Init(*pNetApp));
   IDS_ERR_RET_ERRNO(IDSNetworkScope.Fetch()->GetIfaceId(&flowID));

   if (CompareIfaceIds((dss_iface_id_type)flowID,iface_id)) {
      IDS_ERR_RET_ERRNO((*pNetApp)->GetQoSDefault(&pNetQoSDefault));
      DSSGenScope scopeNetQosManager(pNetQoSDefault,DSSGenScope::IDSIQI_TYPE);

      IDS_ERR_RET_ERRNO(pNetQoSDefault->GetTXQueueLevel(&TXQueueLevel));

      pTxQueueLevel->current_new_data_cnt = TXQueueLevel.currentNewDataCnt;
      pTxQueueLevel->wm_free_cnt = TXQueueLevel.wmFreeCnt;
      pTxQueueLevel->total_pending_cnt = TXQueueLevel.totalPendingCnt;
      return DSS_SUCCESS;
   }


   // Look for the appropriate secondary QoS.
   IDS_ERR_RET_ERRNO((*pNetApp)->GetDSSNetQoSSecondary(iface_id, &pDSSNetQoSSecondary));

   if (NULL != pDSSNetQoSSecondary){
      IDS_ERR_RET_ERRNO(pDSSNetQoSSecondary->GetNetQoSSecondary(&pNetQoSSecondary));
   }

   DSSGenScope GenScopeObject(pNetQoSSecondary, DSSGenScope::IDSIQI_TYPE);

   if (NULL != pNetQoSSecondary){
      IDS_ERR_RET_ERRNO(pNetQoSSecondary->GetTXQueueLevel(&TXQueueLevel));
   }

   pTxQueueLevel->current_new_data_cnt = TXQueueLevel.currentNewDataCnt;
   pTxQueueLevel->wm_free_cnt = TXQueueLevel.wmFreeCnt;
   pTxQueueLevel->total_pending_cnt = TXQueueLevel.totalPendingCnt;
   return DSS_SUCCESS;
}
#ifdef FEATURE_BCMCS
static sint15 dss_iface_ioctl_bcmcs_db_update
(
   void *argval_ptr,
   DSSIDSNetworkScope* IDSNetworkScope,
   sint15  *dss_errno
)
{
   BCMCSDBSpecType       updateParam;
   INetworkExt         * piNetworkExt        = 0;
   IMCastManager       * piMCastManager      = 0;
   IMCastManagerBCMCS  * piMCastManagerBCMCS = 0;
   AEEResult           res                   = AEE_SUCCESS;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_bcmcs_db_update(): argval_ptr: 0x%p", argval_ptr, 0, 0);

   IDS_ERR_RET_ERRNO(IDSNetworkScope->Fetch()->QueryInterface(AEEIID_INetworkExt,
      (void**)&piNetworkExt));
   res = piNetworkExt->CreateMCastManager(&piMCastManager);

   DSSCommon::ReleaseIf((IQI**)&piNetworkExt);

   if(AEE_SUCCESS != res){
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return DSS_ERROR;
   }

   res = piMCastManager->GetTechObject(AEEIID_IMCastManagerBCMCS,
      (void**)&piMCastManagerBCMCS);

   DSSCommon::ReleaseIf((IQI**)&piMCastManager);

   if(AEE_SUCCESS != res){
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return DSS_ERROR;
   }

   res = DSSConversion::DS2IDSUpdateParamBCMCSDB( reinterpret_cast<dss_iface_ioctl_bcmcs_db_update_type*>(argval_ptr), &updateParam);
   if(AEE_SUCCESS != res){
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      DSSCommon::ReleaseIf((IQI**)&piMCastManagerBCMCS);
      return DSS_ERROR;
   }

   res = piMCastManagerBCMCS->UpdateDB(&updateParam);

   DSSCommon::ReleaseIf((IQI**)&piMCastManagerBCMCS);

   if(AEE_SUCCESS != res){
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return DSS_ERROR;
   }

   return DSS_SUCCESS;
}
#endif // FEATURE_BCMCS

static sint15 dss_iface_ioctl_qos_request
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   DSSIDSNetworkPrivScope* IDSNetworkPrivScope,
   sint15  *dss_errno,
   dss_iface_id_type  iface_id
)
{
   IQoSManager* pNetQoSManager = NULL;
   IQoSManagerPriv* pNetQoSManagerPriv = NULL;
   IQoSSecondary* pNetQoSSecondary = NULL;

   QoSSpecType requestedQoSSpec = {0};
   uint8 flowID;

   DSSGenScope ScopeRxFlows;
   DSSGenScope ScopeTxFlows;
   DSSGenScope ScopeRxFilter;
   DSSGenScope ScopeTxFilter;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_qos_request(): iface_id:%u, argval_ptr:0x%p", iface_id, argval_ptr, 0);

   dss_iface_ioctl_qos_request_type* qos_request =
      (dss_iface_ioctl_qos_request_type*)argval_ptr;

   // Convert the old qos_request to the new SpecType.
   IDS_ERR_RET_ERRNO(DSSConversion::DS2IDSQoSSpec(qos_request, //The old QoSSpec request type
                                                  &requestedQoSSpec, // The new ds QoSSpec
                                                  FALSE, // We want to create new QoS requst, no modify operation
                                                  NULL)); // Ignore the modify mask.

   ScopeRxFlows.SetParams(requestedQoSSpec.rxFlows, DSSGenScope::IDSNetQoSFlow_ARRAY_PTR, requestedQoSSpec.rxFlowsLen);
   ScopeTxFlows.SetParams(requestedQoSSpec.txFlows, DSSGenScope::IDSNetQoSFlow_ARRAY_PTR, requestedQoSSpec.txFlowsLen);
   ScopeRxFilter.SetParams(requestedQoSSpec.rxFilter, DSSGenScope::IDSNetIPFilter_ARRAY_PTR, qos_request->qos.rx.fltr_template.num_filters);
   ScopeTxFilter.SetParams(requestedQoSSpec.txFilter, DSSGenScope::IDSNetIPFilter_ARRAY_PTR, qos_request->qos.tx.fltr_template.num_filters);



   IDS_ERR_RET((*pNetApp)->GetNetQoSManager(&pNetQoSManager));
   DSSGenScope scopeNetQosManager(pNetQoSManager,DSSGenScope::IDSIQI_TYPE);

   IDS_ERR_RET(pNetQoSManager->QueryInterface(AEEIID_IQoSManagerPriv, (void**)&pNetQoSManagerPriv));
   DSSGenScope scopeNetQosManagerPriv(pNetQoSManagerPriv,DSSGenScope::IDSIQI_TYPE);


   // Create the NetQoSSecondary object.
   ds::ErrorType nRes2 = pNetQoSManagerPriv->RequestSecondary(&requestedQoSSpec, &pNetQoSSecondary);
   DSSGenScope scopeNetQosSecondary(pNetQoSSecondary,DSSGenScope::IDSIQI_TYPE);

   if (AEE_SUCCESS != nRes2) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(nRes2);
      (void) DSSConversion::IDS2DSQoSSpecErrMask(&requestedQoSSpec,qos_request);
      return DSS_ERROR;
   }

   // find a free flow ID and Add it as the 8 LSB of iface_ID
   flowID = 0;
   IDS_ERR_RET_ERRNO(DSSGlobals::GetFreeQoSFlowID(&flowID));

   // We can't be sure that iface id already contains the app id , let's build it again , and add a flow id .
   uint8 ifaceIndex = 0;
   sint15 app_id = 0;
   ifaceIndex = (uint8)(iface_id >> 24);
   (*pNetApp)->GetNetHandle(&app_id);
   iface_id = BuildIfaceIdWithAppId(ifaceIndex,app_id);

   iface_id |= flowID;

   // update the qos handle for the user
   qos_request->handle = iface_id;

   // Add the new created pNetQoSSecondary to pNetApp.
   nRes2 = (*pNetApp)->AddNetQoSSecondary(pNetQoSSecondary,
      iface_id,
      qos_request->cback_fn,
      qos_request->user_data);
   if (AEE_SUCCESS != nRes2){
      DSSGlobals::ReleaseQoSFlowID(flowID); // flowID must be released
      *dss_errno = DSSConversion::IDS2DSErrorCode(nRes2);
      return DSS_ERROR;
   }

   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_qos_release
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   DSSIDSNetworkPrivScope* IDSNetworkPrivScope,
   sint15  *dss_errno
)
{
   DSSNetQoSSecondary* pDSSNetQoSSecondary = NULL;
   IQoSSecondary* pIDSNetQoSSecondary = NULL;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_qos_release(): argval_ptr: 0x%p", argval_ptr, 0, 0);

   dss_iface_ioctl_qos_release_type* qos_release =
      (dss_iface_ioctl_qos_release_type*)argval_ptr;

   // Get the DSSNetQoSSecondary Structure according to the provided QoSHandle.
   IDS_ERR_RET_ERRNO((*pNetApp)->GetDSSNetQoSSecondary(qos_release->handle, &pDSSNetQoSSecondary));

   // get the NetQoSSecondary object
   if (NULL != pDSSNetQoSSecondary){
      IDS_ERR_RET_ERRNO(pDSSNetQoSSecondary->GetNetQoSSecondary(&pIDSNetQoSSecondary));
   }

   DSSGenScope GenScopeObject(pIDSNetQoSSecondary, DSSGenScope::IDSIQI_TYPE);

   if (NULL != pIDSNetQoSSecondary){
      IDS_ERR_RET_ERRNO(pIDSNetQoSSecondary->Close());
   }

   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_qos_get_granted_flow_spec
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   DSSNetQoSSecondary* pDSSNetQoSSecondary = NULL;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_qos_get_granted_flow_spec(): argval_ptr: 0x%p", argval_ptr, 0, 0);

   dss_iface_ioctl_qos_get_flow_spec_type* qos_flow_spec =
      (dss_iface_ioctl_qos_get_flow_spec_type*)argval_ptr;

   // Get the DSSNetQoSSecondary Structure according to the provided QoSHandle.
   IDS_ERR_RET_ERRNO((*pNetApp)->GetDSSNetQoSSecondary(qos_flow_spec->handle, &pDSSNetQoSSecondary));

   IQoSSecondary* pIDSNetQoSSecondary = NULL;

   // get the NetQoSSecondary object
   if (NULL != pDSSNetQoSSecondary){
      IDS_ERR_RET_ERRNO(pDSSNetQoSSecondary->GetNetQoSSecondary(&pIDSNetQoSSecondary));
   }

   DSSGenScope GenScopeObject(pIDSNetQoSSecondary, DSSGenScope::IDSIQI_TYPE);

   // allow the operation only if the QoSSession is AVAILABLE_MODIFIED
   QoSSecondaryStateInfoType statusInfo = {0};

   // get the NetQoSSecondary object
    if (NULL != pIDSNetQoSSecondary){
      IDS_ERR_RET_ERRNO(pIDSNetQoSSecondary->GetState(&statusInfo));
    }

   if (QoSSecondaryState::QDS_AVAILABLE_MODIFIED != statusInfo.state) {
      memset (&qos_flow_spec->rx_flow, 0, sizeof(qos_flow_spec->rx_flow));
      memset (&qos_flow_spec->tx_flow, 0, sizeof(qos_flow_spec->tx_flow));
      return DSS_SUCCESS;
   }

   IQoSSecondaryPriv* pIDSNetQoSSecondaryPriv = NULL;
   IDS_ERR_RET_ERRNO(pIDSNetQoSSecondary->QueryInterface(AEEIID_IQoSSecondaryPriv, (void**)&pIDSNetQoSSecondaryPriv));
   DSSGenScope GenScopeSecondaryPriv(pIDSNetQoSSecondaryPriv, DSSGenScope::IDSIQI_TYPE);

   IQoSFlowPriv *txQoSFlow = NULL, *rxQoSFlow = NULL;
   IDS_ERR_RET_ERRNO(pIDSNetQoSSecondaryPriv->GetGrantedFlowSpecPriv(&rxQoSFlow, &txQoSFlow));

   // Convert new QoS types to old QoS Types
   IDS_ERR_RET_ERRNO(DSSConversion::IDS2DSQoSSpecFlow(rxQoSFlow, &(qos_flow_spec->rx_flow)));
   IDS_ERR_RET_ERRNO(DSSConversion::IDS2DSQoSSpecFlow(txQoSFlow, &(qos_flow_spec->tx_flow)));

   DSSCommon::ReleaseIf((IQI**)&(txQoSFlow));
   DSSCommon::ReleaseIf((IQI**)&(rxQoSFlow));
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_qos_get_granted_flow_spec2
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_qos_get_granted_flow_spec2(): argval_ptr: 0x%p", argval_ptr, 0, 0);

   DSSNetQoSSecondary* pDSSNetQoSSecondary = NULL;
   dss_iface_ioctl_qos_get_granted_flow_spec2_type* qos_getGrantedFlowSpec2 =
      (dss_iface_ioctl_qos_get_granted_flow_spec2_type*)argval_ptr;


   // Get the DSSNetQoSSecondary Structure according to the provided QoSHandle.
   IDS_ERR_RET_ERRNO((*pNetApp)->GetDSSNetQoSSecondary(qos_getGrantedFlowSpec2->handle, &pDSSNetQoSSecondary));

   IQoSSecondary* pIDSNetQoSSecondary = NULL;

   // get the NetQoSSecondary object
   if (NULL != pDSSNetQoSSecondary){
      IDS_ERR_RET_ERRNO(pDSSNetQoSSecondary->GetNetQoSSecondary(&pIDSNetQoSSecondary));
   }

   DSSGenScope GenScopeObject(pIDSNetQoSSecondary,DSSGenScope::IDSIQI_TYPE);

   if (NULL == pIDSNetQoSSecondary){
      LOG_MSG_ERROR("GetNetQoSSecondary failed.", 0, 0, 0);
      return DSS_ERROR;
   }

   IQoSFlowPriv *txQoSFlow = NULL, *rxQoSFlow = NULL;

   IQoSSecondaryPriv* pIDSNetQoSSecondaryPriv = NULL;
   IDS_ERR_RET_ERRNO(pIDSNetQoSSecondary->QueryInterface(AEEIID_IQoSSecondaryPriv, (void**)&pIDSNetQoSSecondaryPriv));
   DSSGenScope GenScopeSecondaryPriv(pIDSNetQoSSecondaryPriv, DSSGenScope::IDSIQI_TYPE);

   IDS_ERR_RET_ERRNO(pIDSNetQoSSecondaryPriv->GetGrantedFlowSpecPriv(&rxQoSFlow, &txQoSFlow));

   // Convert new QoS types to old QoS Types
   IDS_ERR_RET_ERRNO(DSSConversion::IDS2DSQoSSpecFlow(rxQoSFlow, &(qos_getGrantedFlowSpec2->rx_flow)));
   IDS_ERR_RET_ERRNO(DSSConversion::IDS2DSQoSSpecFlow(txQoSFlow, &(qos_getGrantedFlowSpec2->tx_flow)));

   DSSCommon::ReleaseIf((IQI**)&(txQoSFlow));
   DSSCommon::ReleaseIf((IQI**)&(rxQoSFlow));


   QoSSecondaryStateInfoType statusInfo = {0};
   // Get the QoS Status
   IDS_ERR_RET_ERRNO(pIDSNetQoSSecondary->GetState(&statusInfo));

   // TODO: move to DSSConversion.cpp
   switch(statusInfo.state)
   {
   case QoSSecondaryState::QDS_AVAILABLE_MODIFIED:
      qos_getGrantedFlowSpec2->qos_status = QOS_AVAILABLE;
      break;
   case QoSSecondaryState::QDS_SUSPENDING:
      qos_getGrantedFlowSpec2->qos_status = QOS_SUSPENDING;
      break;
   case QoSSecondaryState::QDS_UNAVAILABLE:
      qos_getGrantedFlowSpec2->qos_status = QOS_UNAVAILABLE;
      break;
   case QoSSecondaryState::QDS_INVALID:
      qos_getGrantedFlowSpec2->qos_status = QOS_STATE_INVALID;
      break;
   case QoSSecondaryState::QDS_ACTIVATING:
      qos_getGrantedFlowSpec2->qos_status = QOS_ACTIVATING;
      break;
   case QoSSecondaryState::QDS_SUSPENDED:
      qos_getGrantedFlowSpec2->qos_status = QOS_DEACTIVATED;
      break;
   case QoSSecondaryState::QDS_RELEASING:
      qos_getGrantedFlowSpec2->qos_status = QOS_RELEASING;
      break;
   case QoSSecondaryState::QDS_CONFIGURING:
      qos_getGrantedFlowSpec2->qos_status = QOS_CONFIGURING;
      break;
   default:
      // TODO: update the errno
      return DSS_ERROR;
   }
   return DSS_SUCCESS;
}


static sint15 dss_iface_ioctl_qos_resume
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_qos_resume(): argval_ptr: 0x%p", argval_ptr, 0, 0);

   DSSNetQoSSecondary *pDSSNetQoSSecondary = NULL;
   dss_iface_ioctl_qos_resume_type* qos_resume =
      (dss_iface_ioctl_qos_resume_type*)argval_ptr;

   // Get the DSSNetQoSSecondary Structure according to the provided QoSHandle.
   IDS_ERR_RET_ERRNO((*pNetApp)->GetDSSNetQoSSecondary(qos_resume->handle, &pDSSNetQoSSecondary));

   IQoSSecondary* pIDSNetQoSSecondary = NULL;

   // get the NetQoSSecondary object
   if (NULL != pDSSNetQoSSecondary){
      IDS_ERR_RET_ERRNO(pDSSNetQoSSecondary->GetNetQoSSecondary(&pIDSNetQoSSecondary));
   }

   DSSGenScope GenScopeObject(pIDSNetQoSSecondary,DSSGenScope::IDSIQI_TYPE);

   // release the QoS Session
   if (NULL != pIDSNetQoSSecondary){
      IDS_ERR_RET_ERRNO(pIDSNetQoSSecondary->Resume());
   }
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_qos_modify
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{

   DSSNetQoSSecondary *pDSSNetQoSSecondary = NULL;
   dss_iface_ioctl_qos_modify_type* qos_modify =
      (dss_iface_ioctl_qos_modify_type*)argval_ptr;
   sint15 nRet = DSS_SUCCESS;
   ds::ErrorType dsErrno;
   QoSModifyMaskType modifyMask;


   DSSGenScope scopeRxFlows;
   DSSGenScope scopeTxFlows;
   DSSGenScope scopeRxFilter;
   DSSGenScope scopeTxFilter;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_qos_modify(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   memset(&modifyMask,0,sizeof(QoSModifyMaskType));

   // Get the DSSNetQoSSecondary Structure according to the provided QoSHandle.
   IDS_ERR_RET_ERRNO((*pNetApp)->GetDSSNetQoSSecondary(qos_modify->handle, &pDSSNetQoSSecondary));

   IQoSSecondary* pIDSNetQoSSecondary = NULL;
   IQoSSecondaryPriv* pIDSNetQoSSecondaryPriv = NULL;

   // get the NetQoSSecondary object
   if (NULL != pDSSNetQoSSecondary){
      IDS_ERR_RET_ERRNO(pDSSNetQoSSecondary->GetNetQoSSecondary(&pIDSNetQoSSecondary));
   }

   DSSGenScope GenScopeObject(pIDSNetQoSSecondary, DSSGenScope::IDSIQI_TYPE);

   if (NULL != pIDSNetQoSSecondary){
     IDS_ERR_RET_ERRNO(pIDSNetQoSSecondary->QueryInterface(AEEIID_IQoSSecondaryPriv, (void**)&pIDSNetQoSSecondaryPriv));
   }

   DSSGenScope GenScopeQoSSecondaryPriv(pIDSNetQoSSecondaryPriv, DSSGenScope::IDSIQI_TYPE);

   dss_iface_ioctl_qos_request_type qos_req;
   qos_req.qos = qos_modify->qos;

   QoSSpecType requestedQoSSpec = {0};

   // convert the old qos_request to the new SpecType.
   BAIL_ERRNO(DSSConversion::DS2IDSQoSSpec(&qos_req, &requestedQoSSpec, TRUE, &modifyMask));

   scopeRxFlows.SetParams(requestedQoSSpec.rxFlows, DSSGenScope::IDSNetQoSFlow_ARRAY_PTR, requestedQoSSpec.rxFlowsLen);
   scopeTxFlows.SetParams(requestedQoSSpec.txFlows, DSSGenScope::IDSNetQoSFlow_ARRAY_PTR, requestedQoSSpec.txFlowsLen);
   scopeRxFilter.SetParams(requestedQoSSpec.rxFilter, DSSGenScope::IDSNetIPFilter_ARRAY_PTR, qos_req.qos.rx.fltr_template.num_filters);
   scopeTxFilter.SetParams(requestedQoSSpec.txFilter, DSSGenScope::IDSNetIPFilter_ARRAY_PTR, qos_req.qos.tx.fltr_template.num_filters);

   if (NULL != pIDSNetQoSSecondaryPriv){
      dsErrno = pIDSNetQoSSecondaryPriv->ModifySecondaryPriv(&requestedQoSSpec, modifyMask);
      if (AEE_SUCCESS != dsErrno) {
         *dss_errno = DSSConversion::IDS2DSErrorCode(dsErrno);
         (void) DSSConversion::IDS2DSQoSSpecErrMask(&requestedQoSSpec,&qos_req);
         nRet = DSS_ERROR;
         goto bail;
      }
   }

bail:
   memcpy(&(qos_modify->qos), &(qos_req.qos), sizeof(qos_req.qos));
   return nRet;
}

static sint15 dss_iface_ioctl_qos_suspend
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_qos_suspend(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   DSSNetQoSSecondary *pDSSNetQoSSecondary = NULL;
   dss_iface_ioctl_qos_suspend_type* qos_suspend =
      (dss_iface_ioctl_qos_suspend_type*)argval_ptr;

   // Get the DSSNetQoSSecondary Structure according to the provided QoSHandle.
   IDS_ERR_RET_ERRNO((*pNetApp)->GetDSSNetQoSSecondary(qos_suspend->handle, &pDSSNetQoSSecondary));

   IQoSSecondary* pIDSNetQoSSecondary = NULL;

   // get the NetQoSSecondary object
   if (NULL != pDSSNetQoSSecondary){
      IDS_ERR_RET_ERRNO(pDSSNetQoSSecondary->GetNetQoSSecondary(&pIDSNetQoSSecondary));
   }

   DSSGenScope GenScopeObject(pIDSNetQoSSecondary, DSSGenScope::IDSIQI_TYPE);

   // release the QoS Session
   if (NULL != pIDSNetQoSSecondary){
      IDS_ERR_RET_ERRNO(pIDSNetQoSSecondary->Suspend());
   }

   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_qos_get_status
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_qos_get_status(): argval_ptr: 0x%p", argval_ptr, 0, 0);
   DSSNetQoSSecondary *pDSSNetQoSSecondary = NULL;
   dss_iface_ioctl_qos_get_status_type* pqos_status_type =
      (     dss_iface_ioctl_qos_get_status_type*)argval_ptr;

   // Get the DSSNetQoSSecondary Structure according to the provided QoSHandle.
   IDS_ERR_RET_ERRNO((*pNetApp)->GetDSSNetQoSSecondary(pqos_status_type->handle,
      &pDSSNetQoSSecondary));

   IQoSSecondary* pIDSNetQoSSecondary = NULL;

   // get the NetQoSSecondary object
   if (NULL != pDSSNetQoSSecondary){
      IDS_ERR_RET_ERRNO(pDSSNetQoSSecondary->GetNetQoSSecondary(&pIDSNetQoSSecondary));
   }

   DSSGenScope GenScopeObject(pIDSNetQoSSecondary, DSSGenScope::IDSIQI_TYPE);

   QoSSecondaryStateInfoType statusInfo = {0};
   // Get the QoS Status
   if (NULL != pIDSNetQoSSecondary){
      IDS_ERR_RET_ERRNO(pIDSNetQoSSecondary->GetState(&statusInfo));
   }

   // TODO: move to DSSConversion.cpp
   switch(statusInfo.state)
   {
   case QoSSecondaryState::QDS_AVAILABLE_MODIFIED:
      pqos_status_type->qos_status = QOS_AVAILABLE;
      break;
   case QoSSecondaryState::QDS_SUSPENDING:
      pqos_status_type->qos_status = QOS_SUSPENDING;
      break;
   case QoSSecondaryState::QDS_UNAVAILABLE:
      pqos_status_type->qos_status = QOS_UNAVAILABLE;
      break;
   case QoSSecondaryState::QDS_INVALID:
      pqos_status_type->qos_status = QOS_STATE_INVALID;
      break;
   case QoSSecondaryState::QDS_ACTIVATING:
      pqos_status_type->qos_status = QOS_ACTIVATING;
      break;
   case QoSSecondaryState::QDS_SUSPENDED:
      pqos_status_type->qos_status = QOS_DEACTIVATED;
      break;
   case QoSSecondaryState::QDS_RELEASING:
      pqos_status_type->qos_status = QOS_RELEASING;
      break;
   case QoSSecondaryState::QDS_CONFIGURING:
      pqos_status_type->qos_status = QOS_CONFIGURING;
      break;
   default:
      // TODO: Update the errno
      return DSS_ERROR;
   }

   return DSS_SUCCESS;
}





static sint15 dss_iface_ioctl_qos_request_ex
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   DSSIDSNetworkPrivScope* IDSNetworkPrivScope,
   sint15  *dss_errno,
   dss_iface_id_type  iface_id
)
{
   ds::ErrorType dsErrno;
   uint8 flowID;
   // We can't be sure that iface id already contains the app id , let's build it again , and add a flow id .
   uint8 ifaceIndex = 0;
   sint15 app_id = 0;
   IQoSManager* pNetQoSManager = NULL;
   IQoSManagerPriv* pNetQoSManagerPriv = NULL;

   dss_iface_ioctl_qos_request_ex_type* qos_request_ex =
      (dss_iface_ioctl_qos_request_ex_type*)argval_ptr;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_qos_request_ex(): iface_id:%u, argval_ptr:0x%p ", iface_id, argval_ptr, 0);

   // prepare an array SpecType
   SeqQoSSpecType requestedQoSSpecs ;
   memset(&requestedQoSSpecs,0,sizeof(SeqQoSSpecType));

   if (qos_request_ex->num_qos_specs > 0) {
      requestedQoSSpecs.data = (QoSSpecType *)ps_system_heap_mem_alloc(sizeof(QoSSpecType)*(qos_request_ex->num_qos_specs));

      if (NULL == requestedQoSSpecs.data) {
         LOG_MSG_ERROR("Can't allocate requestedQoSSpecs.data ", 0, 0, 0);
         *dss_errno = DS_ENOMEM;
         return DSS_ERROR;
      }
   }

   requestedQoSSpecs.dataLen = qos_request_ex->num_qos_specs;

   memset((void*)(requestedQoSSpecs.data), 0, sizeof(QoSSpecType)*(qos_request_ex->num_qos_specs));

   DSSGenScope ScopeQosSpecs(requestedQoSSpecs.data, DSSGenScope::GEN_SCRATCHPAD_ARRAY);

   // convert the old qos_request to the new SpecType.
   IDS_ERR_RET_ERRNO(DSSConversion::DS2IDSQoSSpecBundle(qos_request_ex, requestedQoSSpecs));

   dss_iface_ioctl_qos_request_type tmpReqType;
   DSSQoSRequestExScope QoSRequestExScope(qos_request_ex->num_qos_specs);

   for (int i=0;i<qos_request_ex->num_qos_specs;i++) {
      memcpy(&(tmpReqType.qos), &(qos_request_ex->qos_specs_ptr[i]), sizeof(tmpReqType.qos));
      QoSRequestExScope.SetNthQoSSpec(i,
                                      requestedQoSSpecs.data[i].rxFilter,
                                      tmpReqType.qos.rx.fltr_template.num_filters,
                                      requestedQoSSpecs.data[i].txFilter,
                                      tmpReqType.qos.tx.fltr_template.num_filters,
                                      requestedQoSSpecs.data[i].rxFlows,
                                      requestedQoSSpecs.data[i].rxFlowsLen,
                                      requestedQoSSpecs.data[i].txFlows,
                                      requestedQoSSpecs.data[i].txFlowsLen);
   }

   IQoSSecondariesOutput* QoSSecondaryList = NULL;
   QoSRequestOpCodeType opCode = qos_request_ex->opcode == DSS_IFACE_IOCTL_QOS_REQUEST_OP ? QoSRequestOpCode::QDS_REQUEST : QoSRequestOpCode::QDS_CONFIGURE;

   IDS_ERR_RET((*pNetApp)->GetNetQoSManager(&pNetQoSManager));
   DSSGenScope scopeNetQosManager(pNetQoSManager,DSSGenScope::IDSIQI_TYPE);

   IDS_ERR_RET(pNetQoSManager->QueryInterface(AEEIID_IQoSManagerPriv, (void**)&pNetQoSManagerPriv));
   DSSGenScope scopeNetQosManagerPriv(pNetQoSManagerPriv,DSSGenScope::IDSIQI_TYPE);

   dsErrno = pNetQoSManagerPriv->RequestBundle(requestedQoSSpecs.data,
                                               requestedQoSSpecs.dataLen,
                                               opCode,
                                               &QoSSecondaryList);
   if (AEE_SUCCESS != dsErrno) {
      IDS_ERR_RET_ERRNO(DSSConversion::IDS2DSQoSSpecBundleErrMask(&requestedQoSSpecs,qos_request_ex));
      *dss_errno = DSSConversion::IDS2DSErrorCode(dsErrno);
      return DSS_ERROR;
   }

   // We can't be sure that iface id already contains the app id , let's build it again , and add a flow id .
   ifaceIndex = (uint8)(iface_id >> 24);
   (*pNetApp)->GetNetHandle(&app_id);
   dss_iface_id_type iface_id_temp = BuildIfaceIdWithAppId(ifaceIndex,app_id);

   for (int i=0; i<qos_request_ex->num_qos_specs ; i++)
   {
      // find a free flow ID and Add it as the 8 LSB of iface_ID
      flowID = 0;
      IDS_ERR_RET_ERRNO(DSSGlobals::GetFreeQoSFlowID(&flowID));

      iface_id = iface_id_temp | flowID;

      // update the qos handle for the user
      qos_request_ex->handles_ptr[i] = iface_id;

      // Add the new created pNetQoSSecondary to pNetApp.
      // In case of failure QoSSecondaryList.data is released via ScopeSecondaryList object
      IQoSSecondary* qosSecondary = NULL;
      dsErrno = QoSSecondaryList->GetNth(i,
                                         &qosSecondary);
      if (AEE_SUCCESS != dsErrno){
         DSSGlobals::ReleaseQoSFlowID(flowID); // flowID must be released
         *dss_errno = DSSConversion::IDS2DSErrorCode(dsErrno);
         return DSS_ERROR;
      }

      dsErrno = (*pNetApp)->AddNetQoSSecondary(qosSecondary,
                                               iface_id,
                                               qos_request_ex->cback_fn,
                                               qos_request_ex->user_data);
      if (AEE_SUCCESS != dsErrno){
         DSSGlobals::ReleaseQoSFlowID(flowID); // flowID must be released
         *dss_errno = DSSConversion::IDS2DSErrorCode(dsErrno);
         return DSS_ERROR;
      }

   }
   return DSS_SUCCESS;

}
static sint15 dss_iface_ioctl_qos_release_ex
(
      void *argval_ptr,
      DSSNetApp**  pNetApp,
      DSSIDSNetworkPrivScope* IDSNetworkPrivScope,
      sint15  *dss_errno,
      dss_iface_id_type  iface_id
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_qos_release_ex(): iface_id:%u, argval_ptr:0x%p ", iface_id, argval_ptr, 0);
   dss_iface_ioctl_qos_release_ex_type* qos_release_ex =
      (dss_iface_ioctl_qos_release_ex_type*)argval_ptr;

   IQoSSecondary* pIDSNetQoSSecondary = NULL;

   // Get the IDSNetQoSManager
   IQoSManager* pNetQoSManager = NULL;
   DSSGenScope scopeNetQosManager;
   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetQoSManager(&pNetQoSManager));

   scopeNetQosManager.SetParams(pNetQoSManager,DSSGenScope::IDSIQI_TYPE);

   IQoSSecondariesInput* pQoSSecondariesInput = NULL;
   DSSGenScope scopeQosInput(pQoSSecondariesInput,DSSGenScope::IDSIQI_TYPE);
   IDS_ERR_RET_ERRNO(pNetQoSManager->CreateQoSSecondariesInput(&pQoSSecondariesInput));

   DSSNetQoSSecondary* pDSSNetQoSSecondary = NULL;

   // prepare the IQoSSecondary sessions array
   for (int i=0 ; i<qos_release_ex->num_handles; i++)
   {
      // Get the DSSNetQoSSecondary Structure according to the provided QoSHandle.
      IDS_ERR_RET_ERRNO((*pNetApp)->GetDSSNetQoSSecondary(qos_release_ex->handles_ptr[i], &pDSSNetQoSSecondary));

      // get the NetQoSSecondary object
      if (NULL != pDSSNetQoSSecondary){
         IDS_ERR_RET_ERRNO(pDSSNetQoSSecondary->GetNetQoSSecondary(&pIDSNetQoSSecondary));
      }

      pQoSSecondariesInput->Associate(pIDSNetQoSSecondary);
   }

   IDS_ERR_RET_ERRNO(pNetQoSManager->Close(pQoSSecondariesInput));

   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_qos_suspend_ex
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   DSSIDSNetworkPrivScope* IDSNetworkPrivScope,
   sint15  *dss_errno,
   dss_iface_id_type  iface_id
 )
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_qos_suspend_ex(): iface_id:%u, argval_ptr:0x%p ", iface_id, argval_ptr, 0);
   dss_iface_ioctl_qos_suspend_ex_type* qos_suspend_ex =
      (dss_iface_ioctl_qos_suspend_ex_type*)argval_ptr;

   IQoSSecondary* pIDSNetQoSSecondary = NULL;
   // Get the IDSNetQoSManager
   IQoSManager* pNetQoSManager = NULL;
   DSSGenScope scopeNetQosManager;
   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetQoSManager(&pNetQoSManager));

   scopeNetQosManager.SetParams(pNetQoSManager,DSSGenScope::IDSIQI_TYPE);

   IQoSSecondariesInput* pQoSSecondariesInput = NULL;
   DSSGenScope scopeQosInput(pQoSSecondariesInput,DSSGenScope::IDSIQI_TYPE);
   IDS_ERR_RET_ERRNO(pNetQoSManager->CreateQoSSecondariesInput(&pQoSSecondariesInput));

   DSSNetQoSSecondary* pDSSNetQoSSecondary = NULL;

   // prepare the IQoSSecondary sessions array
   for (int i=0 ; i<qos_suspend_ex->num_handles; i++)
   {
      // Get the DSSNetQoSSecondary Structure according to the provided QoSHandle.
      IDS_ERR_RET_ERRNO((*pNetApp)->GetDSSNetQoSSecondary(qos_suspend_ex->handles_ptr[i], &pDSSNetQoSSecondary));

      // get the NetQoSSecondary object
      if (NULL != pDSSNetQoSSecondary){
         IDS_ERR_RET_ERRNO(pDSSNetQoSSecondary->GetNetQoSSecondary(&pIDSNetQoSSecondary));
      }

      pQoSSecondariesInput->Associate(pIDSNetQoSSecondary);
   }

   IDS_ERR_RET_ERRNO(pNetQoSManager->Suspend(pQoSSecondariesInput));

   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_qos_resume_ex
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   DSSIDSNetworkExtScope* IDSNetworkExtScope,
   sint15  *dss_errno,
   dss_iface_id_type  iface_id
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_qos_resume_ex(): iface_id:%u, argval_ptr:0x%p ", iface_id, argval_ptr, 0);
   dss_iface_ioctl_qos_suspend_ex_type* qos_resume_ex =
      (dss_iface_ioctl_qos_suspend_ex_type*)argval_ptr;

   IQoSSecondary* pIDSNetQoSSecondary = NULL;

   // Get the IDSNetQoSManager
   IQoSManager* pNetQoSManager = NULL;
   DSSGenScope scopeNetQosManager;
   if(AEE_SUCCESS != (*pNetApp)->GetNetQoSManager(&pNetQoSManager))
   {
     // Create a new NetQoSManager since pNetApp does not have
     // a NetQoSManager instance.
     IDS_ERR_RET_ERRNO(IDSNetworkExtScope->Fetch()->CreateQoSManager(&pNetQoSManager));

     // Add the new created pNetQoSManager to pNetApp.
     IDS_ERR_RET_ERRNO((*pNetApp)->SetNetQoSManager(pNetQoSManager));
   }

   scopeNetQosManager.SetParams(pNetQoSManager,DSSGenScope::IDSIQI_TYPE);

   IQoSSecondariesInput* pQoSSecondariesInput = NULL;
   DSSGenScope scopeQosInput(pQoSSecondariesInput,DSSGenScope::IDSIQI_TYPE);
   IDS_ERR_RET_ERRNO(pNetQoSManager->CreateQoSSecondariesInput(&pQoSSecondariesInput));

   DSSNetQoSSecondary* pDSSNetQoSSecondary = NULL;

   // prepare the IQoSSecondary sessions array
   for (int i=0 ; i<qos_resume_ex->num_handles; i++)
   {
      // Get the DSSNetQoSSecondary Structure according to the provided QoSHandle.
      IDS_ERR_RET_ERRNO((*pNetApp)->GetDSSNetQoSSecondary(qos_resume_ex->handles_ptr[i], &pDSSNetQoSSecondary));

      // get the NetQoSSecondary object
      if (NULL != pDSSNetQoSSecondary){
         IDS_ERR_RET_ERRNO(pDSSNetQoSSecondary->GetNetQoSSecondary(&pIDSNetQoSSecondary));
      }

      pQoSSecondariesInput->Associate(pIDSNetQoSSecondary);
   }

   IDS_ERR_RET_ERRNO(pNetQoSManager->Resume(pQoSSecondariesInput));

   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_primary_qos_modify

(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno,
   dss_iface_id_type  iface_id
)
{
   IQoSDefault* pIDSNetQoSDefault;
   IQoSDefaultPriv* pIDSNetQoSDefaultPriv = NULL;
   sint15 nRet = DSS_SUCCESS;
   ds::ErrorType dsErrno;
   QoSModifyMaskType modifyMask;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_primary_qos_modify(): iface_id:%u, argval_ptr:0x%p ", iface_id, argval_ptr, 0);
   memset(&modifyMask,0,sizeof(QoSModifyMaskType));


   // get the NetQoSSecondary object
   IDS_ERR_RET_ERRNO((*pNetApp)->GetQoSDefault(&pIDSNetQoSDefault));
   DSSGenScope scopeNetQosDefault(pIDSNetQoSDefault,DSSGenScope::IDSIQI_TYPE);

   IDS_ERR_RET_ERRNO(pIDSNetQoSDefault->QueryInterface(AEEIID_IQoSDefaultPriv, (void**)&pIDSNetQoSDefaultPriv));
   DSSGenScope scopeNetQosDefaultPriv(pIDSNetQoSDefaultPriv,DSSGenScope::IDSIQI_TYPE);

   dss_iface_ioctl_primary_qos_modify_type* qos_modify =
      (dss_iface_ioctl_primary_qos_modify_type*)argval_ptr;

   if (NULL == qos_modify->cback_fn) {
     LOG_MSG_ERROR ("NULL cback for qos modify", 0, 0, 0);
     *dss_errno = DS_EFAULT;
     return -1;
   }

   QoSSpecPrimaryType requestedQoSSpec = {0};

   // convert the old qos_request to the new SpecType.
   IDS_ERR_RET_ERRNO(DSSConversion::DS2IDSQoSSpecPrimary(qos_modify, &requestedQoSSpec, TRUE, &modifyMask));

   DSSGenScope ScopeRxFlows(requestedQoSSpec.rxFlows, DSSGenScope::IDSNetQoSFlow_ARRAY_PTR, requestedQoSSpec.rxFlowsLen);
   DSSGenScope ScopeTxFlows(requestedQoSSpec.txFlows, DSSGenScope::IDSNetQoSFlow_ARRAY_PTR, requestedQoSSpec.txFlowsLen);

   QoSModifyMaskType mask;
   // convert the old qos mask to the new mask.
   IDS_ERR_RET_ERRNO(DSSConversion::DS2IDSQoSMask(qos_modify->primary_qos_spec.field_mask, &mask));
   // release the QoS Session
   
   // Register to primary QoS status events. (Accepted / rejected). the following command
   // causes the DSS_ to register to ds::NetQoS::Event::MODIFY_RESULT event.
   BAIL_ERRNO((*pNetApp)->RegEventCBPrimary(DSS_IFACE_IOCTL_QOS_MODIFY_ACCEPTED_EV, qos_modify->cback_fn, 
      qos_modify->user_data_ptr));

   dsErrno = pIDSNetQoSDefaultPriv->ModifyDefaultPriv(&requestedQoSSpec, mask );
   if (AEE_SUCCESS != dsErrno) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(dsErrno);
      (void) DSSConversion::IDS2DSQoSSpecPrimaryErrMask(&requestedQoSSpec,qos_modify);
      nRet = DSS_ERROR;
      goto bail;
   }

bail:
   return nRet;
}
static sint15 dss_iface_ioctl_primary_qos_get_granted_flow_spec
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno,
   dss_iface_id_type  iface_id
)
{
   IQoSDefault* pIDSNetQoSDefault;
   IQoSSecondaryPriv* pIDSNetQoSDefaultPriv = NULL;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_primary_qos_get_granted_flow_spec(): iface_id:%u, argval_ptr:0x%p ", iface_id, argval_ptr, 0);

   // get the NetQoSDefault object
   IDS_ERR_RET_ERRNO((*pNetApp)->GetQoSDefault(&pIDSNetQoSDefault));
   DSSGenScope scopeNetQoSDefault(pIDSNetQoSDefault,DSSGenScope::IDSIQI_TYPE);


   IDS_ERR_RET_ERRNO(pIDSNetQoSDefault->QueryInterface(AEEIID_IQoSDefaultPriv, (void**)&pIDSNetQoSDefaultPriv));
   DSSGenScope scopeNetQoSDefaultPriv(pIDSNetQoSDefaultPriv, DSSGenScope::IDSIQI_TYPE);

   dss_iface_ioctl_primary_qos_get_granted_flow_spec_type* pPrimaryQoSGetFlowType =
      (dss_iface_ioctl_primary_qos_get_granted_flow_spec_type*)argval_ptr;

   IQoSFlowPriv *txQoSFlow = NULL, *rxQoSFlow = NULL;

   IDS_ERR_RET_ERRNO(pIDSNetQoSDefaultPriv->GetGrantedFlowSpecPriv(&rxQoSFlow, &txQoSFlow));

   // Convert new QoS types to old QoS Types
   IDS_ERR_RET_ERRNO(DSSConversion::IDS2DSQoSSpecFlow(rxQoSFlow, &(pPrimaryQoSGetFlowType->rx_ip_flow)));
   IDS_ERR_RET_ERRNO(DSSConversion::IDS2DSQoSSpecFlow(txQoSFlow, &(pPrimaryQoSGetFlowType->tx_ip_flow)));

   DSSCommon::ReleaseIf((IQI**)&(txQoSFlow));
   DSSCommon::ReleaseIf((IQI**)&(rxQoSFlow));

   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_on_qos_aware_system
(
   void *argval_ptr,
   DSSIDSNetworkExtScope* IDSNetworkExtScope,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_on_qos_aware_system(): argval_ptr:0x%p ", argval_ptr, 0, 0);
   dss_iface_ioctl_on_qos_aware_system_type* bIsAwareSystem =
      (dss_iface_ioctl_on_qos_aware_system_type*)argval_ptr;

   boolean qosAwareUnaware;
   // Ask the network if it's QoS aware or unaware
   IDS_ERR_RET_ERRNO(IDSNetworkExtScope->Fetch()->GetQosAware(&qosAwareUnaware));

   *bIsAwareSystem = qosAwareUnaware;

   return DSS_SUCCESS;
}
static sint15 dss_iface_ioctl_get_network_supported_qos_profiles
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   IQoSManager *pNetQoSManager = NULL;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_network_supported_qos_profiles(): argval_ptr:0x%p ", argval_ptr, 0, 0);

   dss_iface_ioctl_get_network_supported_qos_profiles_type* supp_qos_profiles =
      (dss_iface_ioctl_get_network_supported_qos_profiles_type*)argval_ptr;

   // allocate the profileValues array
   QoSProfileIdType* profileValues = (QoSProfileIdType*)ps_system_heap_mem_alloc(sizeof(QoSProfileIdType)*(QoSProfileIds::MAX_SUPPORTED_PROFILES));

   if (NULL == profileValues) {
      LOG_MSG_ERROR("Can't allocate profileValues ", 0, 0, 0);
      *dss_errno = DS_ENOMEM;
      return DSS_ERROR;
   }

   DSSGenScope GenScopeObject(profileValues, DSSGenScope::GEN_SCRATCHPAD_ARRAY);

   int profileIdsLenReq;

   // Get the QoS Profile values
   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetQoSManager(&pNetQoSManager));
   DSSGenScope scopeNetQosManager(pNetQoSManager,DSSGenScope::IDSIQI_TYPE);

   IDS_ERR_RET_ERRNO(pNetQoSManager->GetSupportedProfiles(profileValues,
      QoSProfileIds::MAX_SUPPORTED_PROFILES,
      &profileIdsLenReq));

   // Set the user struct to the values we got from the GetSupportedProfiles call
   supp_qos_profiles->profile_count = (uint8)min(profileIdsLenReq, QoSProfileIds::MAX_SUPPORTED_PROFILES);

   for (int i = 0; i < supp_qos_profiles->profile_count; i++){
      supp_qos_profiles->profile_value[i] = (uint16)profileValues[i];
   }

   return DSS_SUCCESS;
}
#endif

static sint15 dss_iface_ioctl_707_get_hdr_1x_handdown_option
(
   void *argval_ptr,
   DSSIDSNetworkScope* IDSNetworkScope,
   sint15  *dss_errno
)
{
   DSSIDSNetwork1xPrivScope IDSNetwork1xPrivScope;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_707_get_hdr_1x_handdown_option(): argval_ptr:0x%p ", argval_ptr, 0, 0);

   dss_iface_ioctl_707_hdr_1x_handdown_option_type* handdownOption =
      (dss_iface_ioctl_707_hdr_1x_handdown_option_type*)argval_ptr;

   IDS_ERR_RET_ERRNO(IDSNetwork1xPrivScope.Init(IDSNetworkScope->Fetch()));

   boolean bHandDownOption;
   IDS_ERR_RET_ERRNO(IDSNetwork1xPrivScope.Fetch()->GetHDR1xHandDownOption(&bHandDownOption));

   *handdownOption = bHandDownOption;

   return DSS_SUCCESS;
}
static sint15 dss_iface_ioctl_707_set_hdr_1x_handdown_option
(
   void *argval_ptr,
   DSSIDSNetworkScope* IDSNetworkScope,
   sint15  *dss_errno
)
{
   DSSIDSNetwork1xPrivScope IDSNetwork1xPrivScope;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_707_set_hdr_1x_handdown_option(): argval_ptr:0x%p ", argval_ptr, 0, 0);

   dss_iface_ioctl_707_hdr_1x_handdown_option_type* handdownOption =
      (dss_iface_ioctl_707_hdr_1x_handdown_option_type*)argval_ptr;

   IDS_ERR_RET_ERRNO(IDSNetwork1xPrivScope.Init(IDSNetworkScope->Fetch()));

   boolean bHandDownOption = *handdownOption;
   IDS_ERR_RET_ERRNO(IDSNetwork1xPrivScope.Fetch()->SetHDR1xHandDownOption(bHandDownOption));

   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_mcast_join
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   DSSIDSNetworkPrivScope* IDSNetworkPrivScope,
   sint15  *dss_errno,
   dss_iface_id_type  iface_id
)
{
   IMCastManager *pNetMCastManager = NULL;
   IMCastManagerPriv* pNetMCastManagerPriv = NULL;
   IMCastSessionPriv* session = NULL;
   ds::AddrFamilyType family =ds::AddrFamily::QDS_AF_UNSPEC;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_mcast_join(): iface_id:%u, argval_ptr:0x%p ", iface_id, argval_ptr, 0);

   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetMCastManager(&pNetMCastManager));
   DSSGenScope scopeNetMCastManager(pNetMCastManager,DSSGenScope::IDSIQI_TYPE);

   IDS_ERR_RET_ERRNO(pNetMCastManager->GetTechObject(AEEIID_IMCastManagerPriv,
                                                     (void**)&pNetMCastManagerPriv));
   DSSGenScope scopeNetMCastManagerPriv(pNetMCastManagerPriv,DSSGenScope::IDSIQI_TYPE);


   dss_iface_ioctl_mcast_join_type* mcast_join = (dss_iface_ioctl_mcast_join_type*)argval_ptr;

   ds::SockAddrStorageType addr;

   IQI* pInfo = NULL;

   IDS_ERR_RET_ERRNO(DSSConversion::DS2IDSMCast(mcast_join, addr, pNetMCastManagerPriv, &pInfo));

   DSSGenScope GenScopeObject(pInfo, DSSGenScope::IDSIQI_TYPE);

   (void)ds::Sock::AddrUtils::GetFamily(addr, &family);

   IDS_ERR_RET_ERRNO(pNetMCastManagerPriv->Join(addr, pInfo, &session));
   DSSGenScope scopeSession(session,DSSGenScope::IDSIQI_TYPE);

   // find a free flow ID and Add it as the 8 LSB of iface_ID
   uint8 flowID = 0;
   IDS_ERR_RET_ERRNO(DSSGlobals::GetFreeMCastFlowID(&flowID));
   iface_id |= flowID;

   IDS_ERR_RET_ERRNO((*pNetApp)->AddDSSMCast(session, iface_id, mcast_join->event_cb, mcast_join->user_data_ptr));

   mcast_join->handle = iface_id;

   return DSS_SUCCESS;
}
static sint15 dss_iface_ioctl_mcast_join_ex
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno,
   dss_iface_id_type  iface_id
)
{
   sint15 nRet = DSS_SUCCESS;
   AEEResult result = AEE_SUCCESS;
   IMCastManager* pNetMCastManager = NULL;
   IMCastManagerPriv* pNetMCastManagerPriv = NULL;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_mcast_join_ex(): iface_id:%u, argval_ptr:0x%p ", iface_id, argval_ptr, 0);

   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetMCastManager(&pNetMCastManager));
   DSSGenScope scopeNetMCastManager(pNetMCastManager,DSSGenScope::IDSIQI_TYPE);

   IDS_ERR_RET_ERRNO(pNetMCastManager->GetTechObject(AEEIID_IMCastManagerPriv,
                                                     (void**)&pNetMCastManagerPriv));
   DSSGenScope scopeNetMCastManagerPriv(pNetMCastManagerPriv,DSSGenScope::IDSIQI_TYPE);

   dss_iface_ioctl_mcast_join_ex_type* mcast_Control_Bundle = (dss_iface_ioctl_mcast_join_ex_type*)argval_ptr;

   ds::SockAddrStorageType addrSeq[DSS_IFACE_MAX_MCAST_FLOWS_PER_IOCTL] = {{0}};
   int addrSeqLen = mcast_Control_Bundle->num_flows;
   IQI* infoSeq[DSS_IFACE_MAX_MCAST_FLOWS_PER_IOCTL];
   int infoSeqLen = mcast_Control_Bundle->num_flows;
   MCastJoinFlagsType mcastFlags[DSS_IFACE_MAX_MCAST_FLOWS_PER_IOCTL] = {0};
   int mcastFlagsLen = mcast_Control_Bundle->num_flows;

   result = DSSConversion::DS2IDSMCastBundle(mcast_Control_Bundle,
                                             addrSeq,
                                             mcastFlags,
                                             pNetMCastManagerPriv,
                                             infoSeq);
   if (AEE_SUCCESS != result) {
      LOG_MSG_ERROR("Can't convert Mcast Bundle", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(result);
      nRet = DSS_ERROR;
      return nRet;
   }

   infoSeqLen = addrSeqLen;

   IMCastSessionPriv* sessions[DSS_IFACE_MAX_MCAST_FLOWS_PER_IOCTL];
   int sessionsLen = mcast_Control_Bundle->num_flows;
   int sessionsLenReq;

   for( uint32 i = 0; mcast_Control_Bundle->num_flows > i; i++ ) {
      sessions[i] = NULL ;
   }

   IDS_ERR_RET_ERRNO( pNetMCastManagerPriv->JoinBundle (addrSeq,
                                                        addrSeqLen,
                                                        infoSeq,
                                                        infoSeqLen,
                                                        mcastFlags,
                                                        mcastFlagsLen,
                                                        sessions,   // or &sessions[0]
                                                        sessionsLen,
                                                        &sessionsLenReq));

   uint8 flowID = 0;
   dss_iface_id_type  tmp_iface_id;


   for( uint32 i = 0; mcast_Control_Bundle->num_flows > i; i++ )
   {
      // find a free flow ID and Add it as the 8 LSB of iface_ID
      BAIL_ERRNO(DSSGlobals::GetFreeMCastFlowID(&flowID));
      tmp_iface_id = iface_id;
      tmp_iface_id |= flowID;

      //AddDSSMCast adds refCnt on each session, so session shall be released
      BAIL_ERRNO((*pNetApp)->AddDSSMCast(sessions[i],
                                         tmp_iface_id,
                                         mcast_Control_Bundle->event_cb,
                                         mcast_Control_Bundle->user_data_ptr));

      // update the mcast handle for the user. handle field in mcast_Control_Bundle is an output parameter.
      mcast_Control_Bundle->handle[i] = tmp_iface_id;
   }

/* fall through */

bail :

   for( uint32 i = 0; mcast_Control_Bundle->num_flows > i; i++ ) {
      DSSCommon::ReleaseIf((IQI**)&(sessions[i]));
   }

   for( uint32 i = 0; mcast_Control_Bundle->num_flows > i; i++ ) {
     DSSCommon::ReleaseIf((IQI**)&(infoSeq[i]));
   }

   return nRet;

}
static sint15 dss_iface_ioctl_mcast_leave
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   DSSMCast* pDSSMCast = NULL;
   dss_iface_ioctl_mcast_leave_type* mcast_leave =
      (dss_iface_ioctl_mcast_leave_type*)argval_ptr;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_mcast_leave(): argval_ptr:0x%p ", argval_ptr, 0, 0);

   // Get the DSSMCast class according to the provided MCastHandle.
   IDS_ERR_RET_ERRNO((*pNetApp)->GetDSSMCast(mcast_leave->handle, &pDSSMCast));

   IMCastSessionPriv* pIDSNetMCastSession = NULL;

   // get the NetQoSSecondary object
   if (NULL != pDSSMCast){
      IDS_ERR_RET_ERRNO(pDSSMCast->GetMCastSession(&pIDSNetMCastSession));
   }

   // Leave the MCast Session.
   if (NULL != pIDSNetMCastSession){
      IDS_ERR_RET_ERRNO(pIDSNetMCastSession->Leave());
   }

   DSSCommon::ReleaseIf((IQI**)&pIDSNetMCastSession);

   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_mcast_leave_ex
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   dss_iface_ioctl_mcast_leave_ex_type* mcast_Control_Bundle = (dss_iface_ioctl_mcast_leave_ex_type*)argval_ptr;

   IMCastManager* pIDSNetMCastManager = NULL;
   IMCastManagerPriv * pIDSNetMcastManagerPriv = NULL;

   DSSMCast* pDSSMCast = NULL;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_mcast_leave_ex(): argval_ptr:0x%p ", argval_ptr, 0, 0);

   // Get the DSSMCast class according to the provided MCastHandle.
   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetMCastManager(&pIDSNetMCastManager));
   DSSGenScope scopeNetMCastManager(pIDSNetMCastManager, DSSGenScope::IDSIQI_TYPE);

   IDS_ERR_RET_ERRNO(pIDSNetMCastManager->GetTechObject(AEEIID_IMCastManagerPriv,
                                                        (void**)&pIDSNetMcastManagerPriv));
   DSSGenScope scopeNetMCastManagerPriv(pIDSNetMcastManagerPriv, DSSGenScope::IDSIQI_TYPE);

   IMCastSessionPriv** pIDSNetMCastSession = NULL;
   if (mcast_Control_Bundle->num_flows > 0) {
      pIDSNetMCastSession = (IMCastSessionPriv **)ps_system_heap_mem_alloc(sizeof(IMCastSessionPriv*)*(mcast_Control_Bundle->num_flows));
      if (NULL == pIDSNetMCastSession) {
         LOG_MSG_ERROR("Can't allocate pIDSNetMCastSession ", 0, 0, 0);
         *dss_errno = DS_ENOMEM;
         return DSS_ERROR;
      }
   }

   int mcastIndex;

   for(mcastIndex = 0; mcast_Control_Bundle->num_flows > mcastIndex; mcastIndex++ ) {
      pIDSNetMCastSession[mcastIndex] = NULL;
   }

   DSSGenScope GenScopeObject(pIDSNetMCastSession, DSSGenScope::IDSNetMCastSession_ARRAY_PTR, mcast_Control_Bundle->num_flows);

   for(mcastIndex = 0; mcast_Control_Bundle->num_flows > mcastIndex; mcastIndex++ ) {
      IDS_ERR_RET_ERRNO((*pNetApp)->GetDSSMCast(mcast_Control_Bundle->handle[mcastIndex], &pDSSMCast));
      if (NULL != pDSSMCast){
         IDS_ERR_RET_ERRNO(pDSSMCast->GetMCastSession(&pIDSNetMCastSession[mcastIndex]));
      }
   }

   // Leave the MCast Session.
   IDS_ERR_RET_ERRNO(pIDSNetMcastManagerPriv->LeaveBundle (pIDSNetMCastSession,
                                                           mcast_Control_Bundle->num_flows));

   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_mcast_register_ex
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_mcast_register_ex(): argval_ptr:0x%p ", argval_ptr, 0, 0);

   dss_iface_ioctl_mcast_register_ex_type* mcast_Control_Bundle =
      (dss_iface_ioctl_mcast_register_ex_type*)argval_ptr;

   DSSMCast* pDSSMCast = NULL;

   IMCastManager* pIDSNetMCastManager = NULL;
   IMCastManagerPriv * pIDSNetMcastManagerPriv = NULL;

   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetMCastManager(&pIDSNetMCastManager));
   DSSGenScope scopeNetMCastManager(pIDSNetMCastManager,DSSGenScope::IDSIQI_TYPE);

   IDS_ERR_RET_ERRNO(pIDSNetMCastManager->GetTechObject(AEEIID_IMCastManagerPriv,
     (void**)&pIDSNetMcastManagerPriv));
   DSSGenScope scopeNetMCastManagerPriv(pIDSNetMcastManagerPriv, DSSGenScope::IDSIQI_TYPE);

   IMCastSessionPriv** ppIDSNetMCastSession = NULL;
   if (mcast_Control_Bundle->num_flows > 0) {
      ppIDSNetMCastSession = (IMCastSessionPriv **)ps_system_heap_mem_alloc(sizeof(IMCastSessionPriv*)*(mcast_Control_Bundle->num_flows));
      if (NULL == ppIDSNetMCastSession) {
         LOG_MSG_ERROR("Can't allocate ppIDSNetMCastSession ", 0, 0, 0);
         *dss_errno = DS_ENOMEM;
         return DSS_ERROR;
      }
   }

   DSSGenScope GenScopeObject(ppIDSNetMCastSession, DSSGenScope::IDSNetMCastSession_ARRAY_PTR, mcast_Control_Bundle->num_flows);

   for( int i = 0; mcast_Control_Bundle->num_flows > i; i++ ) {
      IDS_ERR_RET_ERRNO((*pNetApp)->GetDSSMCast(mcast_Control_Bundle->handle[i], &pDSSMCast));
      if (NULL != pDSSMCast){
         IDS_ERR_RET_ERRNO(pDSSMCast->GetMCastSession(&(ppIDSNetMCastSession[i])));
      }
   }

   IDS_ERR_RET_ERRNO(pIDSNetMcastManagerPriv->RegisterBundle(ppIDSNetMCastSession, (mcast_Control_Bundle->num_flows)));

   return DSS_SUCCESS;
}
static sint15 dss_iface_ioctl_mbms_mcast_context_activate
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_mbms_mcast_context_activate(): argval_ptr:0x%p ", argval_ptr, 0, 0);

   dss_iface_ioctl_mbms_mcast_context_act_type* mbms_act =
      (dss_iface_ioctl_mbms_mcast_context_act_type*)argval_ptr;

   IMCastManager* pNetMCastManager = 0;
   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetMCastManager(&pNetMCastManager));
   DSSGenScope scopeNetMCastManager(pNetMCastManager,DSSGenScope::IDSIQI_TYPE);


   IMCastManagerMBMSPriv* pIDSMBMSManager = 0;
   IDS_ERR_RET_ERRNO(pNetMCastManager->QueryInterface(AEEIID_IMCastManagerMBMSPriv, (void **)(&pIDSMBMSManager)));

   DSSGenScope GenScopeObject(pIDSMBMSManager, DSSGenScope::IDSIQI_TYPE);

   IMCastMBMSCtrlPriv* pNetMCastMBMSCtrl = 0;
   ds::IPAddrType addr;

   IDS_ERR_RET_ERRNO(DSSConversion::DS2IDSIpAddr(&(mbms_act->ip_addr), &addr));

   IDS_ERR_RET_ERRNO(pIDSMBMSManager->Activate(&addr, mbms_act->profile_id, &pNetMCastMBMSCtrl));
   DSSGenScope scopeNetMCastMBMSCtrl(pNetMCastMBMSCtrl,DSSGenScope::IDSIQI_TYPE);

   mbms_act->handle = (uint32)pNetMCastMBMSCtrl;

   IDS_ERR_RET_ERRNO((*pNetApp)->AddDSSMCastMBMSCtrl(pNetMCastMBMSCtrl, mbms_act->handle, mbms_act->event_cb, mbms_act->user_data_ptr));

   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_mbms_mcast_context_deactivate
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_mbms_mcast_context_deactivate(): argval_ptr:0x%p ", argval_ptr, 0, 0);

   dss_iface_ioctl_mbms_mcast_context_deact_type* mbms_act =
      (dss_iface_ioctl_mbms_mcast_context_deact_type*)argval_ptr;

   IMCastMBMSCtrlPriv* pNetMCastMBMSCtrl = NULL;

   IDS_ERR_RET_ERRNO((*pNetApp)->GetMCastMBMSCtrl( mbms_act->nethandle, &pNetMCastMBMSCtrl ));
   DSSGenScope scopeNetMCastMBMSCtrl(pNetMCastMBMSCtrl,DSSGenScope::IDSIQI_TYPE);

   if (NULL != pNetMCastMBMSCtrl){
      IDS_ERR_RET_ERRNO(pNetMCastMBMSCtrl->DeActivate());
   }

   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_bcmcs_enable_handoff_reg
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   sint15 nRet = DSS_SUCCESS;
   AEEResult res = AEE_SUCCESS;
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_bcmcs_enable_handoff_reg(): argval_ptr:0x%p ", argval_ptr, 0, 0);

   // Get the MCAST Manager
   IMCastManager* pNetMCastManager = NULL;
   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetMCastManager(&pNetMCastManager));
   DSSGenScope scopeNetMCastManager(pNetMCastManager,DSSGenScope::IDSIQI_TYPE);

   // Use the MCast manager to get the BCMCS Manager
   IMCastManagerBCMCS* pIDSBCMCSManager;
   IDS_ERR_RET_ERRNO(pNetMCastManager->QueryInterface(AEEIID_IMCastManagerBCMCS, (void **)(&pIDSBCMCSManager)));

   dss_iface_ioctl_bcmcs_enable_handoff_reg_type* handoff_reg =
      (dss_iface_ioctl_bcmcs_enable_handoff_reg_type*)argval_ptr;

   ds::SockAddrStorageType addrSeq[DSS_IFACE_MAX_MCAST_FLOWS_PER_IOCTL] = {{0}};

   // Create a sequence of addresses.
   for (uint32 i = 0; i < handoff_reg->num_mcast_addr; i++) {
      res = DSSConversion::DS2IDSSockIpAddr(&(handoff_reg->mcast_addr_info[i].ip_addr), (addrSeq[i]));
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("Invalid Address Sequence", 0, 0, 0);
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         nRet = DSS_ERROR;
         return nRet;
      }
   }

   // Register with handoff optimization.
   IDS_ERR_RET_ERRNO(pIDSBCMCSManager->RegisterUsingHandoffOpt(addrSeq, handoff_reg->num_mcast_addr));

   return nRet;
}

static sint15 dss_iface_ioctl_mt_reg_cb
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_mt_reg_cb(): argval_ptr:0x%p ", argval_ptr, 0, 0);
   dss_iface_ioctl_mt_reg_cb_type* mt_reg_ptr = reinterpret_cast<dss_iface_ioctl_mt_reg_cb_type*>(argval_ptr);

   if (NULL == mt_reg_ptr) {
      *dss_errno = DS_EFAULT;
      return DSS_ERROR;
   }
   // This call causes to register to MTPD event and update the MTPD handle.
   IDS_ERR_RET_ERRNO((*pNetApp)->RegMTPDEventCB(mt_reg_ptr));

   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_mt_dereg_cb
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_mt_dereg_cb(): argval_ptr:0x%p ", argval_ptr, 0, 0);

   dss_iface_ioctl_mt_dereg_cb_type* mt_reg_ptr = reinterpret_cast<dss_iface_ioctl_mt_dereg_cb_type*>(argval_ptr);

   if (NULL == mt_reg_ptr) {
      *dss_errno = DS_EFAULT;
      return DSS_ERROR;
   }
   // This call causes to de-register to MTPD event
   IDS_ERR_RET_ERRNO((*pNetApp)->DeRegMTPDEventCB(mt_reg_ptr));

   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_dormancy_info_code
(
   void *argval_ptr,
   DSSIDSNetworkScope* IDSNetworkScope,
   sint15  *dss_errno
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_dormancy_info_code(): argval_ptr:0x%p ", argval_ptr, 0, 0);

   DormancyInfoCodeType IDSDormancyInfoCode;
   dss_iface_ioctl_dormancy_info_code_enum_type dormancyInfoCodeToReturn;
   IDS_ERR_RET_ERRNO(IDSNetworkScope->Fetch()->GetDormancyInfoCode(&IDSDormancyInfoCode));
   AEEResult res = DSSConversion::IDS2DSDormancyInfoCode(IDSDormancyInfoCode, &dormancyInfoCodeToReturn);
   if(AEE_SUCCESS != res){
      return DSSConversion::IDS2DSErrorCode(res);
   }
   *reinterpret_cast<dss_iface_ioctl_dormancy_info_code_enum_type*>(argval_ptr) = dormancyInfoCodeToReturn;
   return DSS_SUCCESS;
}

#ifndef FEATURE_DSS_LINUX
static sint15 dss_iface_ioctl_qos_get_mode
(
 void *argval_ptr,
 DSSIDSNetworkScope* IDSNetworkScope,
 sint15  *dss_errno
 )
{
   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_qos_get_mode(): argval_ptr:0x%p ", argval_ptr, 0, 0);

   AEEResult res;
   QoSModeType IDSQoSMode;
   INetworkExt* pIDSNetworkExt = NULL;
   dss_iface_ioctl_qos_mode_type qosModeToReturn;
   IDS_ERR_RET_ERRNO(IDSNetworkScope->Fetch()->QueryInterface(AEEIID_INetworkExt, (void**)&pIDSNetworkExt));

   res = pIDSNetworkExt->GetQoSMode(&IDSQoSMode);
   DSSCommon::ReleaseIf((IQI**)&pIDSNetworkExt);
   IDS_ERR_RET_ERRNO(res);

   res = DSSConversion::IDS2DSQoSMode(IDSQoSMode, &qosModeToReturn);
   if(AEE_SUCCESS != res){
      return DSSConversion::IDS2DSErrorCode(res);
   }
   *reinterpret_cast<dss_iface_ioctl_qos_mode_type*>(argval_ptr) = qosModeToReturn;
   return DSS_SUCCESS;
}
#endif

static sint15 dss_iface_ioctl_umts_get_im_cn_flag
(
   void *argval_ptr,
   DSSIDSNetworkScope* IDSNetworkScope,
   sint15  *dss_errno
)
{
   DSSIDSNetworkUMTSScope IDSNetworkUMTSScope;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_umts_get_im_cn_flag(): argval_ptr:0x%p ", argval_ptr, 0, 0);

   IDS_ERR_RET_ERRNO(IDSNetworkUMTSScope.Init(IDSNetworkScope->Fetch()));

   // Get the IM_CN flag
   boolean IM_CNFlag;
   IDS_ERR_RET_ERRNO(IDSNetworkUMTSScope.Fetch()->GetIMCNFlag((UMTSIMCNFlagType*)&IM_CNFlag));

   // Update argval with the received IM CN flag
   *(uint32*)(argval_ptr) = IM_CNFlag;

   return DSS_SUCCESS;
}


static sint15 dss_iface_ioctl_generate_priv_ipv6_addr
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   DSSIDSNetworkScope* IDSNetworkScope,
   sint15  *dss_errno,
   dss_iface_id_type  iface_id
)
{
   INetworkIPv6* pNetworkIpv6 = NULL;
   IIPv6Address* pNetIpv6Address = NULL;
   ::ds::INAddr6Type ip6Addr ;
   sint15 nRet = DSS_SUCCESS;
   AEEResult res;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_generate_priv_ipv6_addr(): iface_id:%u, argval_ptr:0x%p ", iface_id, argval_ptr, 0);

   IDS_ERR_RET(IDSNetworkScope->Fetch()->GetTechObject(AEEIID_INetworkIPv6, (void**)&pNetworkIpv6));
   if (NULL == pNetworkIpv6) {
      *dss_errno = DS_EINVAL;
      return DSS_ERROR;
   }

   DSSGenScope scopeIPv6 (pNetworkIpv6,DSSGenScope::IDSIQI_TYPE);

   dss_iface_ioctl_priv_ipv6_addr_type* priv_addr = reinterpret_cast<dss_iface_ioctl_priv_ipv6_addr_type*>(argval_ptr);

   res = pNetworkIpv6->GeneratePrivAddr(priv_addr->iid_params.is_unique,&pNetIpv6Address);
   if (AEE_SUCCESS != res && AEE_EWOULDBLOCK != res) {
      LOG_MSG_ERROR("IPv6 address generation failed: %d", res,0,0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return DSS_ERROR;
   }

   // assuming pNetIpv6Address gets some value even in case of WOULDBLOCK
   // TODO verify
   if (pNetIpv6Address == NULL) {
      LOG_MSG_ERROR("IPv6 address generation failed. Expecting valid object despite WOULDBLOCK: %d", 0,0,0);
      *dss_errno = DS_EINVAL;
      return DSS_ERROR;
   }

   DSSGenScope scopeIPv6address (pNetIpv6Address,DSSGenScope::IDSIQI_TYPE);

   if (AEE_EWOULDBLOCK != res) {
      IDS_ERR_RET_ERRNO(pNetIpv6Address->GetAddress(ip6Addr));
      IDS_ERR_RET_ERRNO(DSSConversion::IDS2DSIp6Addr(ip6Addr,priv_addr->ip_addr));
   }

   // ipv6address may not exist yet at that point in case of WOULDBLOCK
   IDS_ERR_RET_ERRNO((*pNetApp)->AddIpv6PrivAddr(pNetIpv6Address,
                                          iface_id,
                                          priv_addr->iid_params.is_unique,
                                          priv_addr->event_cb,
                                          priv_addr->user_data_ptr));

   *dss_errno = DSSConversion::IDS2DSErrorCode(res);
   if (*dss_errno != DSS_SUCCESS) {
      nRet = DSS_ERROR;
   }

   return nRet;
}

static sint15 dss_iface_ioctl_get_all_v6_prefixes
(
 void *argval_ptr,
 DSSNetApp**  pNetApp,
 DSSIDSNetworkScope* IDSNetworkScope,
 sint15  *dss_errno
)
{
   INetworkIPv6Priv* pNetworkIPv6Priv = NULL;
   IPv6PrivPrefixInfoType* prefixes = NULL;
   sint15 nRet = DSS_SUCCESS;
   int nReq = 0;
   int nMin = 0;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_get_all_v6_prefixes(): argval_ptr:0x%p ", argval_ptr, 0, 0);

   IDS_ERR_RET_ERRNO(IDSNetworkScope->Fetch()->GetTechObject(AEEIID_INetworkIPv6Priv, (void**)&pNetworkIPv6Priv));
   if (NULL == pNetworkIPv6Priv) {
      *dss_errno = DS_EINVAL;
      return DSS_ERROR;
   }

   DSSGenScope scopeIPV6prefixes(pNetworkIPv6Priv,DSSGenScope::IDSIQI_TYPE);

   dss_iface_ioctl_get_all_v6_prefixes_type* all_v6_prefixes = reinterpret_cast<dss_iface_ioctl_get_all_v6_prefixes_type*>(argval_ptr);

   prefixes = NULL;

   if(all_v6_prefixes->num_prefixes == 0)
   {
      LOG_MSG_ERROR("No memory alloc'd for prefixes!", 0,0,0);
      *dss_errno = DS_EFAULT;
      return DSS_ERROR;
   }

   prefixes = (IPv6PrivPrefixInfoType *)ps_system_heap_mem_alloc(sizeof(IPv6PrivPrefixInfoType)*(all_v6_prefixes->num_prefixes));
   if (NULL == prefixes) {
      LOG_MSG_ERROR("Can't allocate memory for the prefixes", 0, 0, 0);
      *dss_errno = DS_ENOMEM;
      goto bail;
   }

   BAIL_ERRNO(pNetworkIPv6Priv->GetAllIPv6Prefixes(prefixes,all_v6_prefixes->num_prefixes,&nReq));
   // at this point nReq can be < all_v6_prefixes->num_prefixes
   nMin = min(all_v6_prefixes->num_prefixes, nReq);
   for(int i = 0; i < nMin; i++)
   {
      memcpy(all_v6_prefixes->prefix_info_ptr[i].prefix.ps_s6_addr,
             prefixes[i].prefix,
             sizeof(struct ps_in6_addr));

      BAIL_ERRNO(DSSConversion::IDS2DSIPv6PrefixState(prefixes[i].prefixType,&(all_v6_prefixes->prefix_info_ptr[i].prefix_state)));

      all_v6_prefixes->prefix_info_ptr[i].prefix_len =
        (uint8) prefixes[i].prefixLen;
   }

   // num_prefixes returned to app should be at most the number app required , even though NetworkIPv6Priv may actually return more
   // backward compatibility issues
   all_v6_prefixes->num_prefixes = nMin;

bail:

   PS_SYSTEM_HEAP_MEM_FREE (prefixes);

   return nRet;
}

static sint15 dss_iface_ioctl_add_static_nat_entry
(
   void *argval_ptr,
   DSSNetApp **pNetApp,
   sint15  *dss_errno
)
{
   INatSession* pNetNatSession = NULL;
   ds::Net::IPNatStaticEntryType staticNatEntry;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_add_static_nat_entry(): Ioctl: Add static NAT entry, argval %p", argval_ptr, 0, 0);

   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetNatSession(&pNetNatSession));
   DSSGenScope scopeNetNatSession(pNetNatSession,DSSGenScope::IDSIQI_TYPE);

   (void) DSSConversion::DS2IDSStaticNatEntry((ps_iface_ioctl_static_nat_entry_type*)argval_ptr, &staticNatEntry);
   IDS_ERR_RET_ERRNO(pNetNatSession->AddStaticNatEntry(&staticNatEntry));

   LOG_MSG_FUNCTION_EXIT("Success", 0, 0, 0);
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_delete_static_nat_entry
(
   void *argval_ptr,
   DSSNetApp **pNetApp,
   sint15  *dss_errno
)
{
   INatSession* pNetNatSession = NULL;
   ds::Net::IPNatStaticEntryType staticNatEntry;

   LOG_MSG_FUNCTION_ENTRY("Ioctl: Delete static NAT entry, argval %p", argval_ptr, 0, 0);

   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetNatSession(&pNetNatSession));
   DSSGenScope scopeNetNatSession(pNetNatSession,DSSGenScope::IDSIQI_TYPE);

   (void) DSSConversion::DS2IDSStaticNatEntry((ps_iface_ioctl_static_nat_entry_type*)argval_ptr, &staticNatEntry);
   IDS_ERR_RET_ERRNO(pNetNatSession->DeleteStaticNatEntry(&staticNatEntry));

   LOG_MSG_FUNCTION_EXIT("Success", 0, 0, 0);
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_static_nat_entry
(
   void *argval_ptr,
   DSSNetApp **pNetApp,
   sint15  *dss_errno
)
{
   INatSession* pNetNatSession = NULL;
   ds::Net::IPNatStaticEntryType *staticNatEntryArr = NULL;
   ps_iface_ioctl_get_static_nat_entry_type* getStaticNatEntryArg; 
   int totalNatEntries;
   int i;

   LOG_MSG_FUNCTION_ENTRY("Ioctl: Get static NAT entry, argval %p", argval_ptr, 0, 0);

   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetNatSession(&pNetNatSession));
   DSSGenScope scopeNetNatSession(pNetNatSession,DSSGenScope::IDSIQI_TYPE);

   getStaticNatEntryArg = (ps_iface_ioctl_get_static_nat_entry_type*) argval_ptr;

   if (0 != getStaticNatEntryArg->num_entries) {
      staticNatEntryArr = (ds::Net::IPNatStaticEntryType*)
               ps_system_heap_mem_alloc(
                                       sizeof(ds::Net::IPNatStaticEntryType) *
                                          (getStaticNatEntryArg->num_entries));
      if (NULL == staticNatEntryArr) {
         LOG_MSG_ERROR ("Cannot alloc mem for getting static nat entries", 0, 0, 0);
         *dss_errno = DS_ENOMEM;
         return DSS_ERROR;
      }
   }
    
   (void) pNetNatSession->GetStaticNatEntry(staticNatEntryArr, getStaticNatEntryArg->num_entries, &totalNatEntries);
   for (i = 0; i < MIN (getStaticNatEntryArg->num_entries, (uint8)totalNatEntries); i++)
   {
      (void) DSSConversion::IDS2DSStaticNatEntry(staticNatEntryArr + i, &(getStaticNatEntryArg->entries_arr[i]));
   }
    
   getStaticNatEntryArg->total_entries = (uint8) totalNatEntries;
   PS_SYSTEM_HEAP_MEM_FREE (staticNatEntryArr);

   LOG_MSG_FUNCTION_EXIT("Success", 0, 0, 0);
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_dynamic_nat_entry_timeout
(
   void *argval_ptr,
   DSSNetApp **pNetApp,
   sint15  *dss_errno
)
{
   INatSession* pNetNatSession = NULL;
   ps_iface_ioctl_get_dynamic_nat_entry_timeout_type* pTimeout;

   LOG_MSG_FUNCTION_ENTRY("Ioctl: Get dynamic NAT entry timeout, argval %p", argval_ptr, 0, 0);

   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetNatSession(&pNetNatSession));
   DSSGenScope scopeNetNatSession(pNetNatSession,DSSGenScope::IDSIQI_TYPE);

   pTimeout = reinterpret_cast<ps_iface_ioctl_get_dynamic_nat_entry_timeout_type *>(argval_ptr);
   IDS_ERR_RET_ERRNO(pNetNatSession->GetDynamicNatEntryTimeout(&pTimeout->timeout));

   LOG_MSG_FUNCTION_EXIT("Success", 0, 0, 0);
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_set_dynamic_nat_entry_timeout
(
   void *argval_ptr,
   DSSNetApp **pNetApp,
   sint15  *dss_errno
)
{
   INatSession* pNetNatSession = NULL;
   ps_iface_ioctl_set_dynamic_nat_entry_timeout_type* pTimeout;

   LOG_MSG_FUNCTION_ENTRY("Ioctl: Set dynamic NAT entry timeout, argval %p", argval_ptr, 0, 0);

   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetNatSession(&pNetNatSession));
   DSSGenScope scopeNetNatSession(pNetNatSession,DSSGenScope::IDSIQI_TYPE);

   pTimeout = reinterpret_cast<ps_iface_ioctl_set_dynamic_nat_entry_timeout_type *>(argval_ptr);
   IDS_ERR_RET_ERRNO(pNetNatSession->SetDynamicNatEntryTimeout(pTimeout->timeout));

   LOG_MSG_FUNCTION_EXIT("Success", 0, 0, 0);
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_nat_ipsec_vpn_pass_through
(
   void *argval_ptr,
   DSSNetApp **pNetApp,
   sint15  *dss_errno
)
{
   INatSession* pNetNatSession = NULL;
   ps_iface_ioctl_nat_ipsec_vpn_pass_through_type* pVpnPassThrough;

   LOG_MSG_FUNCTION_ENTRY("Ioctl: Get dynamic NAT entry timeout, argval %p", argval_ptr, 0, 0);

   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetNatSession(&pNetNatSession));
   DSSGenScope scopeNetNatSession(pNetNatSession,DSSGenScope::IDSIQI_TYPE);

   pVpnPassThrough = reinterpret_cast<ps_iface_ioctl_nat_ipsec_vpn_pass_through_type *>(argval_ptr);
   IDS_ERR_RET_ERRNO(pNetNatSession->GetIpSecVpnPassThrough(&pVpnPassThrough->is_vpn_passthrough));

   LOG_MSG_FUNCTION_EXIT("Success", 0, 0, 0);
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_set_nat_ipsec_vpn_pass_through
(
   void *argval_ptr,
   DSSNetApp **pNetApp,
   sint15  *dss_errno
)
{
   INatSession* pNetNatSession = NULL;
   ps_iface_ioctl_nat_ipsec_vpn_pass_through_type* pVpnPassThrough;

   LOG_MSG_FUNCTION_ENTRY("Ioctl: Set dynamic NAT entry timeout, argval %p", argval_ptr, 0, 0);

   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetNatSession(&pNetNatSession));
   DSSGenScope scopeNetNatSession(pNetNatSession,DSSGenScope::IDSIQI_TYPE);

   pVpnPassThrough = reinterpret_cast<ps_iface_ioctl_nat_ipsec_vpn_pass_through_type *>(argval_ptr);
   IDS_ERR_RET_ERRNO(pNetNatSession->SetIpSecVpnPassThrough(pVpnPassThrough->is_vpn_passthrough));

   LOG_MSG_FUNCTION_EXIT("Success", 0, 0, 0);
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_nat_l2tp_vpn_pass_through
(
   void *argval_ptr,
   DSSNetApp **pNetApp,
   sint15  *dss_errno
)
{
   INatSession* pNetNatSession = NULL;
   ps_iface_ioctl_nat_l2tp_vpn_pass_through_type* pVpnPassThrough;

   LOG_MSG_FUNCTION_ENTRY("Ioctl: Get L2TP VPN Pass through, argval %p", argval_ptr, 0, 0);

   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetNatSession(&pNetNatSession));
   DSSGenScope scopeNetNatSession(pNetNatSession,DSSGenScope::IDSIQI_TYPE);

   pVpnPassThrough = reinterpret_cast<ps_iface_ioctl_nat_l2tp_vpn_pass_through_type *>(argval_ptr);
   IDS_ERR_RET_ERRNO(pNetNatSession->GetL2TPVpnPassThrough(&pVpnPassThrough->is_l2tp_vpn_passthrough));

   LOG_MSG_FUNCTION_EXIT("Success", 0, 0, 0);
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_set_nat_l2tp_vpn_pass_through
(
   void *argval_ptr,
   DSSNetApp **pNetApp,
   sint15  *dss_errno
)
{
   INatSession* pNetNatSession = NULL;
   ps_iface_ioctl_nat_l2tp_vpn_pass_through_type* pVpnPassThrough;

   LOG_MSG_FUNCTION_ENTRY("Ioctl: Set L2TP VPN Pass through, argval %p", argval_ptr, 0, 0);

   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetNatSession(&pNetNatSession));
   DSSGenScope scopeNetNatSession(pNetNatSession,DSSGenScope::IDSIQI_TYPE);

   pVpnPassThrough = reinterpret_cast<ps_iface_ioctl_nat_l2tp_vpn_pass_through_type *>(argval_ptr);
   IDS_ERR_RET_ERRNO(pNetNatSession->SetL2TPVpnPassThrough(pVpnPassThrough->is_l2tp_vpn_passthrough));

   LOG_MSG_FUNCTION_EXIT("Success", 0, 0, 0);
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_nat_pptp_vpn_pass_through
(
   void *argval_ptr,
   DSSNetApp **pNetApp,
   sint15  *dss_errno
)
{
   INatSession* pNetNatSession = NULL;
   ps_iface_ioctl_nat_pptp_vpn_pass_through_type* pVpnPassThrough;

   LOG_MSG_FUNCTION_ENTRY("Ioctl: Get PPTP VPN Pass through, argval %p", argval_ptr, 0, 0);

   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetNatSession(&pNetNatSession));
   DSSGenScope scopeNetNatSession(pNetNatSession,DSSGenScope::IDSIQI_TYPE);

   pVpnPassThrough = reinterpret_cast<ps_iface_ioctl_nat_pptp_vpn_pass_through_type *>(argval_ptr);
   IDS_ERR_RET_ERRNO(pNetNatSession->GetPPTPVpnPassThrough(&pVpnPassThrough->is_pptp_vpn_passthrough));

   LOG_MSG_FUNCTION_EXIT("Success", 0, 0, 0);
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_set_nat_pptp_vpn_pass_through
(
   void *argval_ptr,
   DSSNetApp **pNetApp,
   sint15  *dss_errno
)
{
   INatSession* pNetNatSession = NULL;
   ps_iface_ioctl_nat_pptp_vpn_pass_through_type* pVpnPassThrough;

   LOG_MSG_FUNCTION_ENTRY("Ioctl: Set PPTP VPN Pass through, argval %p", argval_ptr, 0, 0);

   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetNatSession(&pNetNatSession));
   DSSGenScope scopeNetNatSession(pNetNatSession,DSSGenScope::IDSIQI_TYPE);

   pVpnPassThrough = reinterpret_cast<ps_iface_ioctl_nat_pptp_vpn_pass_through_type *>(argval_ptr);
   IDS_ERR_RET_ERRNO(pNetNatSession->SetPPTPVpnPassThrough(pVpnPassThrough->is_pptp_vpn_passthrough));

   LOG_MSG_FUNCTION_EXIT("Success", 0, 0, 0);
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_add_dmz
(
   void *argval_ptr,
   DSSNetApp **pNetApp,
   sint15  *dss_errno
)
{
   INatSession* pNetNatSession = NULL;
   ds::Net::DMZEntryType dmzEntry;
   dss_iface_ioctl_dmz_type* addDMZEntry; 

   LOG_MSG_FUNCTION_ENTRY("Ioctl: Add DMZ, argval %p", argval_ptr, 0, 0);

   addDMZEntry = (dss_iface_ioctl_dmz_type*) argval_ptr;

   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetNatSession(&pNetNatSession));
   DSSGenScope scopeNetNatSession(pNetNatSession,DSSGenScope::IDSIQI_TYPE);

   if ((addDMZEntry->dmz_ip_addr.type == IPV4_ADDR))
   {
     (void) DSSConversion::DS2IDSDMZEntry((dss_iface_ioctl_dmz_type*)argval_ptr, &dmzEntry);
     IDS_ERR_RET_ERRNO(pNetNatSession->AddDMZ(&dmzEntry));
   }
   else
   {
     LOG_MSG_ERROR("DMZ address cannot be a V6 address", 0, 0, 0);
     *dss_errno = DS_EFAULT;
     return DSS_ERROR;
   }

   LOG_MSG_FUNCTION_EXIT("Success", 0, 0, 0);
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_dmz
(
   void *argval_ptr,
   DSSNetApp **pNetApp,
   sint15  *dss_errno
)
{
   INatSession* pNetNatSession = NULL;
   ds::Net::DMZEntryType *dmzEntry = NULL;
   dss_iface_ioctl_dmz_type* getDMZEntry; 
   sint15 nRet = DSS_SUCCESS;

   LOG_MSG_FUNCTION_ENTRY("Ioctl: Get DMZ, argval %p", argval_ptr, 0, 0);

   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetNatSession(&pNetNatSession));
   DSSGenScope scopeNetNatSession(pNetNatSession,DSSGenScope::IDSIQI_TYPE);

   dmzEntry = (ds::Net::DMZEntryType*)ps_system_heap_mem_alloc(
                                      sizeof(ds::Net::DMZEntryType));

   if (NULL == dmzEntry) {
      LOG_MSG_ERROR ("Cannot alloc mem for getting DMZ entry", 0, 0, 0);
      *dss_errno = DS_ENOMEM;
      return DSS_ERROR;
   }

   getDMZEntry = (dss_iface_ioctl_dmz_type*) argval_ptr;

   BAIL_ERRNO(pNetNatSession->GetDMZ(dmzEntry));

   memset (&getDMZEntry->dmz_ip_addr, 0, sizeof(ip_addr_type));
   DSSConversion::IDS2DSDMZEntry(dmzEntry, getDMZEntry );

   PS_SYSTEM_HEAP_MEM_FREE(dmzEntry);

   LOG_MSG_FUNCTION_EXIT("Success", 0, 0, 0);
   return nRet;

bail:
   PS_SYSTEM_HEAP_MEM_FREE(dmzEntry);
   return nRet;
}

static sint15 dss_iface_ioctl_delete_dmz
(
   void *argval_ptr,
   DSSNetApp **pNetApp,
   sint15  *dss_errno
)
{
   INatSession* pNetNatSession = NULL;

   LOG_MSG_FUNCTION_ENTRY("Ioctl: Delete DMZ, argval %p", argval_ptr, 0, 0);

   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetNatSession(&pNetNatSession));
   DSSGenScope scopeNetNatSession(pNetNatSession,DSSGenScope::IDSIQI_TYPE);

   IDS_ERR_RET_ERRNO(pNetNatSession->DeleteDMZ());

   LOG_MSG_FUNCTION_EXIT("Success", 0, 0, 0);
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_nat_public_ip_addr
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{      
   LOG_MSG_FUNCTION_ENTRY("Ioctl: Getr NAT Public IP address, argval_ptr: 0x%p", argval_ptr, 0, 0);
   dss_iface_ioctl_nat_public_ip_addr_type* pIpAddr = reinterpret_cast<dss_iface_ioctl_nat_public_ip_addr_type*>(argval_ptr);
   IDS_ERR_RET_ERRNO((*pNetApp)->GetNatPublicIPAddress(pIpAddr));
   LOG_MSG_FUNCTION_EXIT("Success", 0, 0, 0);
   return DSS_SUCCESS;
} 

static sint15 dss_iface_ioctl_enable_firewall
(
   void *argval_ptr,
   DSSNetApp **pNetApp,
   sint15  *dss_errno
)
{
   IFirewallManager* pNetFirewallManager = NULL;

   //DM when do we need reinterpret cast? Is it required here
   dss_iface_ioctl_enable_firewall_type* pEnableFirewall = (dss_iface_ioctl_enable_firewall_type*)argval_ptr;;

   LOG_MSG_FUNCTION_ENTRY("Ioctl: Enable Firewall, Allow pkts: %d", pEnableFirewall->is_pkts_allowed, 0, 0);

   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetFirewallManager(&pNetFirewallManager));
   DSSGenScope scopeNetFirewallManager(pNetFirewallManager,DSSGenScope::IDSIQI_TYPE);

   IDS_ERR_RET_ERRNO(pNetFirewallManager->EnableFirewall(pEnableFirewall->is_pkts_allowed));

   LOG_MSG_FUNCTION_EXIT("Success", 0, 0, 0);
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_disable_firewall
(
   void *argval_ptr,
   DSSNetApp **pNetApp,
   sint15  *dss_errno
)
{
   IFirewallManager* pNetFirewallManager = NULL;
   LOG_MSG_FUNCTION_ENTRY("Ioctl: Disable Firewall", 0, 0, 0);

   (void) argval_ptr;

   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetFirewallManager(&pNetFirewallManager));
   DSSGenScope scopeNetFirewallManager(pNetFirewallManager,DSSGenScope::IDSIQI_TYPE);

   IDS_ERR_RET_ERRNO(pNetFirewallManager->DisableFirewall());

   LOG_MSG_FUNCTION_EXIT("Success", 0, 0, 0);
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_add_firewall_rule
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   DSSIDSNetworkPrivScope* IDSNetworkPrivScope,
   sint15  *dss_errno
)
{
   IFirewallManager*          pIFirewallManager = NULL;
   IFirewallRule*             pIFirewallRule = NULL;
   IIPFilterPriv*             pIIPFilterPriv = NULL;
   INetworkFactoryPriv*       pINetworkFactoryPriv = NULL;

   LOG_MSG_FUNCTION_ENTRY("IOCTL: Add firewall rule, argval %p", argval_ptr, 0, 0);

   dss_iface_ioctl_add_firewall_rule_type* add_firewall_rule = (dss_iface_ioctl_add_firewall_rule_type*)argval_ptr;
   if (NULL == add_firewall_rule) {
      *dss_errno = DS_EFAULT;
      return DSS_ERROR;
   }

   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetFirewallManager(&pIFirewallManager));
   DSSGenScope scopeNetFirewallManager(pIFirewallManager,DSSGenScope::IDSIQI_TYPE);

   // Get the NetworkFactory from DSSGlobals
   DSSGlobals::Instance()->GetNetworkFactoryPriv(&pINetworkFactoryPriv);
   DSSGenScope scopeNetworkFactoryPriv(pINetworkFactoryPriv,DSSGenScope::IDSIQI_TYPE);

   // Create a new ds::Net::IIPFilter interface
   IDS_ERR_RET_ERRNO (pINetworkFactoryPriv->CreateIPFilterSpec(reinterpret_cast<IIPFilterPriv**>(&pIIPFilterPriv)));
   DSSGenScope GenScopeObject(pIIPFilterPriv, DSSGenScope::IDSIQI_TYPE);

   // Pass over the filters and convert them to the new API filters in order
   // to set the filters on the corresponding IDSNetwork object.
   IDS_ERR_RET_ERRNO(DSSConversion::DS2IDSIPFilter(&(add_firewall_rule->fltr_spec), pIIPFilterPriv));

   //Add the firewall rule on the Firewall Manager object
   IDS_ERR_RET_ERRNO(pIFirewallManager->AddFirewallRule(pIIPFilterPriv, &pIFirewallRule));
   DSSGenScope scopeFirewallRule(pIFirewallRule,DSSGenScope::IDSIQI_TYPE);

   //Add the FirewallRule object to the DSS layer along with handle
   IDS_ERR_RET_ERRNO((*pNetApp)->AddDSSFirewallRule(pIFirewallRule, &add_firewall_rule->handle));

   LOG_MSG_FUNCTION_EXIT("Success, handle %d", add_firewall_rule->handle, 0, 0);
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_delete_firewall_rule
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{
   dss_iface_ioctl_delete_firewall_rule_type* delete_firewall_rule =
      (dss_iface_ioctl_delete_firewall_rule_type*)argval_ptr;
   IFirewallRule*         pIFirewallRule = NULL;

   if (NULL == delete_firewall_rule) {
      *dss_errno = DS_EFAULT;
      return DSS_ERROR;
   }

   LOG_MSG_FUNCTION_ENTRY("Ioctl: Delete firewall rule, handle %d", delete_firewall_rule->handle, 0, 0);

   // Get the FirewallRule object from handle passed in.
   IDS_ERR_RET_ERRNO((*pNetApp)->GetDSSFirewallRule(delete_firewall_rule->handle, &pIFirewallRule));
   DSSGenScope scopeFirewallRule(pIFirewallRule,DSSGenScope::IDSIQI_TYPE);

   IDS_ERR_RET_ERRNO((*pNetApp)->DeleteDSSFirewallRule(delete_firewall_rule->handle));

   LOG_MSG_FUNCTION_EXIT("Success", 0, 0, 0);
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_firewall_rule
(
   void *argval_ptr,
   DSSNetApp** pNetApp,
   DSSIDSNetworkPrivScope* IDSNetworkPrivScope,
   sint15  *dss_errno
)
{
   IFirewallRule* pIFirewallRule = NULL;
   IIPFilterPriv*     pIIPFilterSpec = NULL;
   dss_iface_ioctl_get_firewall_rule_type* get_firewall_rule =
      (dss_iface_ioctl_get_firewall_rule_type*)argval_ptr;

   if (NULL == get_firewall_rule) {
      *dss_errno = DS_EFAULT;
      return DSS_ERROR;
   }

   LOG_MSG_FUNCTION_ENTRY("Ioctl: Get firewall rule, handle %d", get_firewall_rule->handle, 0, 0);

   // Get the IFirewall rule object corresponding to this handle
   IDS_ERR_RET_ERRNO((*pNetApp)->GetDSSFirewallRule(get_firewall_rule->handle, &pIFirewallRule));
   DSSGenScope scopeFirewallRule(pIFirewallRule,DSSGenScope::IDSIQI_TYPE);

   // Get the IIPFilterSpec associated with this firewall rule.
   if (NULL == pIFirewallRule) 
   {
      *dss_errno = DS_EFAULT;
      return DSS_ERROR;
   }

   IDS_ERR_RET_ERRNO(pIFirewallRule->GetFirewallRule(&pIIPFilterSpec));
   DSSGenScope scopeFilterSpec(pIIPFilterSpec,DSSGenScope::IDSIQI_TYPE);

   // Convert IIPPFilterSepc interface into DSS format.
   memset (&get_firewall_rule->fltr_spec, 0, sizeof(ip_filter_type));
   IDS_ERR_RET_ERRNO(DSSConversion::IDS2DSIPFilterSpec(pIIPFilterSpec, &get_firewall_rule->fltr_spec));

   LOG_MSG_FUNCTION_EXIT("Success", 0, 0, 0);
   return DSS_SUCCESS;
}

static sint15 dss_iface_ioctl_get_firewall_table
(
   void *argval_ptr,
   DSSNetApp** pNetApp,
   DSSIDSNetworkPrivScope* IDSNetworkPrivScope,
   sint15  *dss_errno
)
{
   int                index;
   IFirewallManager*  pIFirewallManager = NULL;
   IIPFilterPriv*     pIIPFilterSpec = NULL;
   AEEResult          res            = AEE_SUCCESS;
   sint15             ret;

   LOG_MSG_FUNCTION_ENTRY("Ioctl: Get firewall table", 0, 0, 0);

   dss_iface_ioctl_get_firewall_table_type* get_firewall_table =
      (dss_iface_ioctl_get_firewall_table_type*)argval_ptr;

   // Create the IFirewallRule objects.
   IFirewallManager::SeqFirewallRulesType FirewallRulesList; 
   memset(&FirewallRulesList, 0, 
            sizeof( IFirewallManager::SeqFirewallRulesType)); 
   if (get_firewall_table->num_fltrs > 0) {
      FirewallRulesList.data = (IFirewallRule **)
         ps_system_heap_mem_alloc(
                                 sizeof(IFirewallRule*)*(get_firewall_table->num_fltrs));
      if (NULL ==  FirewallRulesList.data) {
         LOG_MSG_ERROR("Can't allocate FirewallRulesList.data ", 0, 0, 0);
         *dss_errno = DS_ENOMEM;
         return DSS_ERROR;
      }
   }

   FirewallRulesList.dataLen = get_firewall_table->num_fltrs;

   DSSGenScope GenScopeObject(FirewallRulesList.data, DSSGenScope::IDSNetFirewallRule_ARRAY_PTR, get_firewall_table->num_fltrs);

   res = (*pNetApp)->GetNetFirewallManager(&pIFirewallManager);

   DSSGenScope scopeNetFirewallManager(pIFirewallManager,DSSGenScope::IDSIQI_TYPE);

   if(AEE_SUCCESS != res){
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      ret = DSS_ERROR;
      goto bail;
   }

   res = pIFirewallManager->GetFirewallTable(FirewallRulesList.data,
                                             FirewallRulesList.dataLen,
                                             &FirewallRulesList.dataLenReq);

   if(AEE_SUCCESS != res){
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      ret = DSS_ERROR;
      goto bail;
   }

   for (index = 0; index < MIN (FirewallRulesList.dataLen, FirewallRulesList.dataLenReq); index++)
   {
      res = (FirewallRulesList.data[index])->GetFirewallRule(&pIIPFilterSpec);
      if(AEE_SUCCESS != res){
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         ret = DSS_ERROR;
         goto bail;
      }

      res = DSSConversion::IDS2DSIPFilterSpec(pIIPFilterSpec, &(get_firewall_table->fltr_spec_arr[index]));
      if(AEE_SUCCESS != res){
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         ret = DSS_ERROR;
         goto bail;
      }
   }

   get_firewall_table->avail_num_fltrs = FirewallRulesList.dataLenReq;

   LOG_MSG_FUNCTION_EXIT("Success", 0, 0, 0);

   return DSS_SUCCESS;

bail:
   if (get_firewall_table->num_fltrs > 0)
   {
     PS_SYSTEM_HEAP_MEM_FREE (FirewallRulesList.data);
   }

   return ret;
}

static sint15 dss_iface_ioctl_get_iface_stats
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{      
   LOG_MSG_FUNCTION_ENTRY("Ioctl: Get Iface stats", 0, 0, 0);
   dss_iface_ioctl_get_iface_stats_type* pStats = reinterpret_cast<dss_iface_ioctl_get_iface_stats_type*>(argval_ptr);
   IDS_ERR_RET_ERRNO((*pNetApp)->GetNetworkStatistics(pStats));
   LOG_MSG_FUNCTION_EXIT("Success", 0, 0, 0);
   return DSS_SUCCESS;
} 

static sint15 dss_iface_ioctl_reset_iface_stats
(
   void *argval_ptr,
   DSSNetApp**  pNetApp,
   sint15  *dss_errno
)
{      
   LOG_MSG_FUNCTION_ENTRY("Ioctl: Reset Iface stats", 0, 0, 0);
   IDS_ERR_RET_ERRNO((*pNetApp)->ResetNetworkStatistics());
   LOG_MSG_FUNCTION_EXIT("Success", 0, 0, 0);
   return DSS_SUCCESS;
} 

static sint15 dss_iface_ioctl_set_fmc_tunnel_params
(
   void       * argval_ptr,
   DSSNetApp  ** pNetApp,
   sint15     * dss_errno
)
{
   INetworkExt2 *pNetworkExt2 = 0;
   FMCTunnelParamsType pIDSFMCTunnelParams;
   AEEResult result = AEE_SUCCESS;

   
   LOG_MSG_FUNCTION_ENTRY("NetApp obj : 0x%x argval_ptr : 0x%x", pNetApp, argval_ptr, 0);

   dss_iface_ioctl_uw_fmc_tunnel_params_type * pDSFMCTunnelParams = 
      (dss_iface_ioctl_uw_fmc_tunnel_params_type *)argval_ptr;

   memset(&pIDSFMCTunnelParams, 0, sizeof(FMCTunnelParamsType));

   IDS_ERR_RET_ERRNO(DSSConversion::DS2IDSFMCTunnelParams(pDSFMCTunnelParams, &pIDSFMCTunnelParams));

   IDS_ERR_RET((*pNetApp)->GetIDSNetworkExt2Object(&pNetworkExt2));

   result = pNetworkExt2->SetFMCTunnelParams(&pIDSFMCTunnelParams);
   if(AEE_SUCCESS != result){
      LOG_MSG_ERROR("Cannot SetFMCTunnelParams, result = 0x%x", result, 0, 0);
      goto bail;
   }

   DSSCommon::ReleaseIf((IQI**)&pNetworkExt2);

   LOG_MSG_FUNCTION_EXIT("Success", 0, 0, 0);

   return DSS_SUCCESS;

bail:

   DSSCommon::ReleaseIf((IQI**)&pNetworkExt2);

   *dss_errno = DSSConversion::IDS2DSErrorCode(result); 

   return DSS_ERROR;
}

static sint15 dss_iface_ioctl_reset_fmc_tunnel_params
(
  void       * argval_ptr,
  DSSNetApp  ** pNetApp,
  sint15     * dss_errno
)
{
   INetworkExt2 *pNetworkExt2 = 0;
   AEEResult result = AEE_SUCCESS;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_reset_fmc_tunnel_params(): NetApp obj : 0x%x argval_ptr : 0x%x", pNetApp, argval_ptr, 0);

   // this IOCTL does not use IOCTL argument
   (void) argval_ptr;

   IDS_ERR_RET((*pNetApp)->GetIDSNetworkExt2Object(&pNetworkExt2));

   result = pNetworkExt2->ResetFMCTunnelParams();
   if(AEE_SUCCESS != result){
     LOG_MSG_ERROR("Cannot ResetFMCTunnelParams, result = 0x%x", result, 0, 0);
     goto bail;
   }

   DSSCommon::ReleaseIf((IQI**)&pNetworkExt2);

   LOG_MSG_FUNCTION_EXIT("Success", 0, 0, 0);

   return DSS_SUCCESS;

bail:

   DSSCommon::ReleaseIf((IQI**)&pNetworkExt2);

   *dss_errno = DSSConversion::IDS2DSErrorCode(result); 

   return DSS_ERROR;
}

//===================================================================
//  FUNCTION:   IfaceIoctl707
//
//  DESCRIPTION:
//
//===================================================================
static int IfaceIoctl707(DSSNetApp** pNetApp, INetwork* piNetwork, dss_iface_ioctl_type ioctlName,
                         void* pArgVal, sint15* sErrno, dss_iface_id_type iface_id)
{
   DSSIDSNetwork1xScope IDSNetwork1xScope;
   DSSIDSNetwork1xPrivScope IDSNetwork1xPrivScope;
   IQoS1x             * piQoS1x = 0;
   DSSNetQoSSecondary * pDSSNetQoSSecondary = 0;
   IQoSSecondary      * pIDSNetQoSSecondary = 0;
   IQoSDefault        * pIDSNetQoSDefault = 0;
   uint32             flowID = 0;

   AEEResult res = AEE_SUCCESS;

   sint15    result = DSS_SUCCESS;

   LOG_MSG_FUNCTION_ENTRY("IfaceIoctl707(): Network: 0x%p, ioctlName:%d", piNetwork, ioctlName, 0);

   if (NULL == piNetwork){
      LOG_MSG_ERROR("IfaceIoctl707 :: piNetwork is NULL", 0,0,0);
      res = QDS_EFAULT;
      goto bail;
   }

   // Fetch the right interface for the ioctl.
   switch (ioctlName) {
      // These IOCTLs use IDSNetwork1x.
      case DSS_IFACE_IOCTL_707_GET_MDR:
      case DSS_IFACE_IOCTL_707_SET_MDR:
      case DSS_IFACE_IOCTL_707_GET_RLP_ALL_CURR_NAK:
      case DSS_IFACE_IOCTL_707_SET_RLP_ALL_CURR_NAK:
      case DSS_IFACE_IOCTL_707_GET_RLP_ALL_DEF_NAK:
      case DSS_IFACE_IOCTL_707_SET_RLP_ALL_DEF_NAK:
      case DSS_IFACE_IOCTL_707_GET_RLP_ALL_NEG_NAK:
      case DSS_IFACE_IOCTL_707_GET_RLP_QOS_NA_PRI:
      case DSS_IFACE_IOCTL_707_SET_RLP_QOS_NA_PRI:
      case DSS_IFACE_IOCTL_707_SDB_SUPPORT_QUERY:
         res = IDSNetwork1xScope.Init(piNetwork);
         if (AEE_SUCCESS != res) {
            goto bail;
         }
         break;

      // These IOCTLs use IDSNetwork1xPriv.
      case DSS_IFACE_IOCTL_GET_SESSION_TIMER:
      case DSS_IFACE_IOCTL_SET_SESSION_TIMER:
      case DSS_IFACE_IOCTL_707_GET_DORM_TIMER:
      case DSS_IFACE_IOCTL_707_SET_DORM_TIMER:
      case DSS_IFACE_IOCTL_707_ENABLE_HDR_HPT_MODE:
      case DSS_IFACE_IOCTL_707_ENABLE_HDR_REV0_RATE_INTERIA:
      case DSS_IFACE_IOCTL_707_ENABLE_HOLDDOWN:

      case DSS_IFACE_IOCTL_707_ENABLE_HDR_SLOTTED_MODE:
         res = IDSNetwork1xPrivScope.Init(piNetwork);
         if (AEE_SUCCESS != res) {
            goto bail;
         }
         break;
        
#ifndef FEATURE_DSS_LINUX
      // These IOCTLs use IQoS1x.
      case DSS_IFACE_IOCTL_707_HDR_GET_RMAC3_INFO:
      case DSS_IFACE_IOCTL_707_GET_TX_STATUS:
      case DSS_IFACE_IOCTL_707_GET_INACTIVITY_TIMER:
      case DSS_IFACE_IOCTL_707_SET_INACTIVITY_TIMER:
         // handle default flow case
         flowID = PS_IFACE_GET_FLOW_INST_FROM_ID(iface_id);
         if (PS_IFACE_DEFAULT_FLOW_INST == flowID){
            // Get the QoSDefault Structure from NetApp.
            res = (*pNetApp)->GetQoSDefault(&pIDSNetQoSDefault);
            if(AEE_SUCCESS != res){
               goto bail;
            }
            res = pIDSNetQoSDefault->GetTechObject(AEEIID_IQoS1x, (void**)&piQoS1x);
            if (AEE_SUCCESS != res) {
               goto bail;
            }
         }
         // handle secondary flow case
         else{
            // Get the DSSNetQoSSecondary Structure according to the provided QoSHandle.
            res = (*pNetApp)->GetDSSNetQoSSecondary(flowID, &pDSSNetQoSSecondary);
            if(AEE_SUCCESS != res){
               goto bail;
            }
            // get the NetQoSSecondary object
            if (NULL != pDSSNetQoSSecondary){
               res = pDSSNetQoSSecondary->GetNetQoSSecondary(&pIDSNetQoSSecondary);
               if(AEE_SUCCESS != res){
                  goto bail;
               }
               res = pIDSNetQoSSecondary->GetTechObject(AEEIID_IQoS1x, (void**)&piQoS1x);
               if (AEE_SUCCESS != res) {
                  goto bail;
               }
            }
         }
         break;
#endif

      default:
         break;// of IQoS1x IOCTLs case
   }


   // Call the right function for the ioctl.
   switch (ioctlName) {
      // TODO: for MDR & DORM_TIMER - the original ioctl expects uint* (not int*)
      case DSS_IFACE_IOCTL_707_GET_MDR:
         res = IDSNetwork1xScope.Fetch()->GetMDR(reinterpret_cast<int*>(pArgVal));
         break;
      case DSS_IFACE_IOCTL_707_SET_MDR:
         res = IDSNetwork1xScope.Fetch()->SetMDR(*reinterpret_cast<int*>(pArgVal));
         break;
      case DSS_IFACE_IOCTL_707_GET_DORM_TIMER:
         res = IDSNetwork1xPrivScope.Fetch()->GetDormancyTimer(reinterpret_cast<int*>(pArgVal));
         break;
      case DSS_IFACE_IOCTL_707_SET_DORM_TIMER:
         res = IDSNetwork1xPrivScope.Fetch()->SetDormancyTimer(*reinterpret_cast<int*>(pArgVal));
         break;
      case DSS_IFACE_IOCTL_707_GET_RLP_ALL_CURR_NAK:
         res = IDSNetwork1xScope.Fetch()->GetRLPAllCurrentNAK(reinterpret_cast<Network1xRLPOptionType*>(pArgVal));
         break;
      case DSS_IFACE_IOCTL_707_SET_RLP_ALL_CURR_NAK:
         res = IDSNetwork1xScope.Fetch()->SetRLPAllCurrentNAK(reinterpret_cast<Network1xRLPOptionType*>(pArgVal));
         break;
      case DSS_IFACE_IOCTL_707_GET_RLP_ALL_DEF_NAK:
         res = IDSNetwork1xScope.Fetch()->GetRLPDefCurrentNAK(reinterpret_cast<Network1xRLPOptionType*>(pArgVal));
         break;
      case DSS_IFACE_IOCTL_707_SET_RLP_ALL_DEF_NAK:
         res = IDSNetwork1xScope.Fetch()->SetRLPDefCurrentNAK(reinterpret_cast<Network1xRLPOptionType*>(pArgVal));
         break;
      case DSS_IFACE_IOCTL_707_GET_RLP_ALL_NEG_NAK:
         res = IDSNetwork1xScope.Fetch()->GetRLPNegCurrentNAK(reinterpret_cast<Network1xRLPOptionType*>(pArgVal));
         break;
      // TODO: the original ioctl expects byte* (not int*) (MUST be solved!)

#ifndef FEATURE_DSS_LINUX
      case DSS_IFACE_IOCTL_707_GET_RLP_QOS_NA_PRI:
         res = IDSNetwork1xScope.Fetch()->GetQoSNAPriority(reinterpret_cast<int*>(pArgVal));
         break;
      case DSS_IFACE_IOCTL_707_SET_RLP_QOS_NA_PRI:
         res = IDSNetwork1xScope.Fetch()->SetQoSNAPriority(*reinterpret_cast<int*>(pArgVal));
         break;
#endif
      case DSS_IFACE_IOCTL_707_SDB_SUPPORT_QUERY:
      {
         dss_iface_ioctl_707_sdb_support_query_type* pssq =
            reinterpret_cast<dss_iface_ioctl_707_sdb_support_query_type*>(pArgVal);
         res = IDSNetwork1xScope.Fetch()->QueryDoSSupport(DSSConversion::DS2IDSSDBFlags(pssq->flags), &pssq->can_do_sdb);
         break;
      }
      case DSS_IFACE_IOCTL_707_ENABLE_HOLDDOWN:
         res = IDSNetwork1xPrivScope.Fetch()->EnableHoldDown(*reinterpret_cast<boolean*>(pArgVal));
         break;

      case DSS_IFACE_IOCTL_707_ENABLE_HDR_HPT_MODE:
      {
         dss_iface_ioctl_707_enable_hdr_hpt_mode_type  *hdr_hpt_mode = 
            (dss_iface_ioctl_707_enable_hdr_hpt_mode_type*) pArgVal;

         res = IDSNetwork1xPrivScope.Fetch()->EnableHDRHPTMode
               (
                  hdr_hpt_mode->enable
               );
         break;
      }

      case DSS_IFACE_IOCTL_707_ENABLE_HDR_SLOTTED_MODE:
      {
         dss_iface_ioctl_707_enable_hdr_slotted_mode_type* hdr_slotted_mode_ptr;

         if(pArgVal == NULL) {
            res = QDS_EFAULT;
            LOG_MSG_ERROR("Null argval_ptr", 0,0,0);
            break;
         }

         hdr_slotted_mode_ptr =(dss_iface_ioctl_707_enable_hdr_slotted_mode_type*) pArgVal;

         LOG_MSG_INFO1_6("IfaceIoctl707():dss_net_handle: %d," 
                        "enable:%d, slotted_mode_opion: %d, "
                        "get_slotted_mode:%d, event_cb:0x%p, user_data:0x%p ",  
                        hdr_slotted_mode_ptr->dss_nethandle,   
                        hdr_slotted_mode_ptr->enable,
                        hdr_slotted_mode_ptr->slotted_mode_option,
                        hdr_slotted_mode_ptr->get_slotted_mode, 
                        hdr_slotted_mode_ptr->event_cb, 
                        hdr_slotted_mode_ptr->user_data);
						
         /* Check whether intent is to get current slotted mode value */
         if(hdr_slotted_mode_ptr->get_slotted_mode == TRUE) 
         {
            uint32 sci;
            res = IDSNetwork1xPrivScope.Fetch()->GetHDRSlottedModeCycleIndex(&sci);
            if (AEE_SUCCESS != res) 
            {
               break;
            }
            hdr_slotted_mode_ptr->slotted_mode_option = sci;
         }
         else if (hdr_slotted_mode_ptr->enable) 
         {
            ds::Net::Network1xPrivHDRSlottedModeArg arg;

            if (hdr_slotted_mode_ptr->event_cb)
            {
               res = (*pNetApp)->RegEventCB
                     (
                        DSS_IFACE_IOCTL_707_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_SUCCESS_EV,
                        hdr_slotted_mode_ptr->event_cb, hdr_slotted_mode_ptr->user_data
                     );

               res = (*pNetApp)->RegEventCB
                     (
                        DSS_IFACE_IOCTL_707_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_FAILURE_EV,
                        hdr_slotted_mode_ptr->event_cb, hdr_slotted_mode_ptr->user_data
                     );
            }
            (*pNetApp)->SetIDSNetwork1xPrivObject(IDSNetwork1xPrivScope.Fetch());
			
            arg.enable = hdr_slotted_mode_ptr->enable;
            arg.slottedModeOption = hdr_slotted_mode_ptr->slotted_mode_option;
            arg.getSlottedMode = FALSE;
            res = IDSNetwork1xPrivScope.Fetch()->EnableHDRSlottedMode(&arg);
        } 
        else 
	{
            // De-Register slotted modeRresult event.
            res = (*pNetApp)->DeregEventCB(DSS_IFACE_IOCTL_707_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_SUCCESS_EV);
            if (res != AEE_SUCCESS) {
               break;
            }
            res = (*pNetApp)->DeregEventCB(DSS_IFACE_IOCTL_707_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_FAILURE_EV);
         }
         break;
      }

      case DSS_IFACE_IOCTL_707_ENABLE_HDR_REV0_RATE_INTERIA:
      {
         dss_iface_ioctl_707_enable_hdr_rev0_rate_inertia_type *rev0_inertia = 
	   (dss_iface_ioctl_707_enable_hdr_rev0_rate_inertia_type *)pArgVal; 

	  if (rev0_inertia->event_cb)
          {
             res = (*pNetApp)->RegEventCB
                   (
                      DSS_IFACE_IOCTL_707_ENABLE_HDR_REV0_RATE_INERTIA_SUCCESS_EV,
                      rev0_inertia->event_cb, rev0_inertia->user_data
                   );

             res = (*pNetApp)->RegEventCB
                   (
                      DSS_IFACE_IOCTL_707_ENABLE_HDR_REV0_RATE_INERTIA_FAILURE_EV,
                      rev0_inertia->event_cb, rev0_inertia->user_data
                   );
          }

         (*pNetApp)->SetIDSNetwork1xPrivObject
                     (IDSNetwork1xPrivScope.Fetch());

         res = IDSNetwork1xPrivScope.Fetch()->EnableHDRRev0RateInertia(rev0_inertia->enable);
      }
      break;
      // TODO: the new RMAC3 info fields are of type int - originally they were uint16
#ifndef FEATURE_DSS_LINUX
      case DSS_IFACE_IOCTL_707_HDR_GET_RMAC3_INFO:
      {
         IQoS1x::RMAC3InfoType idsInfo = {0};
         dss_iface_ioctl_707_hdr_rmac3_info_type* pdssInfo =
            reinterpret_cast<dss_iface_ioctl_707_hdr_rmac3_info_type*>(pArgVal);
         // this cannot happen cause in the switch above we create QoS1x for this case
         // (case DSS_IFACE_IOCTL_707_HDR_GET_RMAC3_INFO:), shut up KW
         if(NULL == piQoS1x){
            ASSERT(0);
            goto bail;
         }
         else{
            res = piQoS1x->GetRMAC3Info(&idsInfo);
            pdssInfo->ps_headroom_payload_size = (uint16)idsInfo.headroomPayloadSize;
            pdssInfo->bucket_level_payload_size = (uint16)idsInfo.bucketLevelPayloadSize;
            pdssInfo->t2p_inflow_payload_size = (uint16)idsInfo.t2pInflowPayloadSize;
         }
         break;
      }
      case DSS_IFACE_IOCTL_707_GET_TX_STATUS:
         // this cannot happen cause in the switch above we create QoS1x for this case
         // (case DSS_IFACE_IOCTL_707_GET_TX_STATUS:), shut up KW
         if(NULL == piQoS1x){
            ASSERT(0);
            goto bail;
         }
         else{
            res = piQoS1x->GetTXStatus(reinterpret_cast<boolean*>(pArgVal));
         }
         break;
      // TODO: for INACTIVITY_TIMER - the original ioctl expects uint32* (not int*)
      case DSS_IFACE_IOCTL_707_GET_INACTIVITY_TIMER:
         // this cannot happen cause in the switch above we create QoS1x for this case
         // (case DSS_IFACE_IOCTL_707_GET_INACTIVITY_TIMER:), shut up KW
         if(NULL == piQoS1x){
            ASSERT(0);
            goto bail;
         }
         else{
            res = piQoS1x->GetInactivityTimer(reinterpret_cast<int*>(pArgVal));
         }
         break;
      case DSS_IFACE_IOCTL_707_SET_INACTIVITY_TIMER:
         // this cannot happen cause in the switch above we create QoS1x for this case
         // (case DSS_IFACE_IOCTL_707_SET_INACTIVITY_TIMER:), shut up KW
         if(NULL == piQoS1x){
            ASSERT(0);
            goto bail;
         }
         else{
            res = piQoS1x->SetInactivityTimer(*reinterpret_cast<int*>(pArgVal));
         }
         break;
#endif
      case DSS_IFACE_IOCTL_GET_SESSION_TIMER:
      {
         Network1xPrivSessionTimerType idsTimer;
         memset(&idsTimer, 0, 
                  sizeof(Network1xPrivSessionTimerType));
         dss_session_timer_type* pdssTimer =
            reinterpret_cast<dss_session_timer_type*>(pArgVal);
         res = DSSConversion::DS2IDSSessionTimerSelect(pdssTimer->timer_select, &(idsTimer.select));
         if (AEE_SUCCESS != res){
            break;
         }
         res = IDSNetwork1xPrivScope.Fetch()->GetSessionTimer(&idsTimer);
         pdssTimer->timer_val = idsTimer.value;
         break;
      }
      case DSS_IFACE_IOCTL_SET_SESSION_TIMER:
      {
         Network1xPrivSessionTimerType idsTimer;
         memset(&idsTimer, 0, 
                  sizeof(Network1xPrivSessionTimerType));
         dss_session_timer_type* pdssTimer =
            reinterpret_cast<dss_session_timer_type*>(pArgVal);
         res = DSSConversion::DS2IDSSessionTimerSelect(pdssTimer->timer_select,&(idsTimer.select));
         if (AEE_SUCCESS != res){
            break;
         }
         idsTimer.value = pdssTimer->timer_val;
         res = IDSNetwork1xPrivScope.Fetch()->SetSessionTimer(&idsTimer);
         break;
      }
      case DSS_IFACE_IOCTL_707_SET_HYSTERESIS_ACT_TIMER:
      {
         res = IDSNetwork1xScope.Fetch()->SetHysteresisTimer(*reinterpret_cast<Network1xPrivHysteresisTimerType*>(pArgVal));
         break;
      }
      case DSS_IFACE_IOCTL_707_GET_HYSTERESIS_ACT_TIMER:
      {
         res = IDSNetwork1xScope.Fetch()->GetHysteresisTimer(reinterpret_cast<Network1xPrivHysteresisTimerType*>(pArgVal));
         break;
      }

      default:
         res = QDS_EOPNOTSUPP;
         break;
   }

/* fall through */

bail:

   DSSCommon::ReleaseIf((IQI**)&piQoS1x);

   DSSCommon::ReleaseIf((IQI**)&pIDSNetQoSDefault);

   DSSCommon::ReleaseIf((IQI**)&pIDSNetQoSSecondary);

   if(0 != pDSSNetQoSSecondary){
      delete pDSSNetQoSSecondary;
   }

   if(AEE_SUCCESS != res){
      result = DSS_ERROR;
   }
   else{
      result = DSS_SUCCESS;
   }

   *sErrno = DSSConversion::IDS2DSErrorCode(res);

   return result;
}

int dss_iface_ioctl_internal(dss_iface_id_type        iface_id,
                             dss_iface_ioctl_type     ioctl_name,
                             void                     *argval_ptr,
                             sint15                   *dss_errno,
                             DSSNetApp**              pNetApp,
                             bool*                    bIsTempDSSNetApp)
{
   sint15                 netHandle = -1;
   DSSIDSNetworkPrivScope IDSNetworkPrivScope;
   DSSIDSNetworkScope     IDSNetworkScope;
   DSSIDSNetworkExtScope  IDSNetworkExtScope;
   DSSIDSNetwork1xScope   IDSNetwork1xScope;
   bool                   bAllowSecondary = FALSE;
   bool                   bForbidTemporary = FALSE;
   AEEResult              res = AEE_SUCCESS;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl_internal(): iface_id:%u, ioctl_name:%d, argval_ptr:0x%p", iface_id, ioctl_name, argval_ptr);

   // Initialize returned error code
   *dss_errno = DSS_SUCCESS;

   // Handle the ioctls that doesn't care about the iface id before checking validity.
   if (DSS_IFACE_IOCTL_GET_ALL_IFACES == ioctl_name) {
      dss_iface_ioctl_all_ifaces_type* arg = reinterpret_cast<dss_iface_ioctl_all_ifaces_type*>(argval_ptr);
      IfaceIdType             aNetTypes[MAX_SYSTEM_IFACES];
      int                              nLen = 0;
      INetworkFactoryPriv*           pIDSNetworkFactoryPriv = NULL;
      INetworkPriv*                  pIDSNetworkPriv = NULL;
      sint15                           ret = DSS_SUCCESS;

      res = DSSGlobals::Instance()->GetNetworkFactoryPriv(&pIDSNetworkFactoryPriv);
      if (AEE_SUCCESS != res) {
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         ret = DSS_ERROR;
         goto bail_get_all_ifaces;
      }

      res = pIDSNetworkFactoryPriv->CreateNetworkPriv(NULL,&pIDSNetworkPriv);
      if (AEE_SUCCESS != res) {
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         ret = DSS_ERROR;
         goto bail_get_all_ifaces;
      }

      res = pIDSNetworkPriv->GetAllIfaces(aNetTypes, MAX_SYSTEM_IFACES, &nLen);
      if (AEE_SUCCESS != res) {
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         ret = DSS_ERROR;
         goto bail_get_all_ifaces;
      }
      if (MAX_SYSTEM_IFACES < nLen) {
         LOG_MSG_INFO1("There are more ifaces than allowed", 0, 0, 0);
      }
      nLen = min(nLen, MAX_SYSTEM_IFACES);
      arg->number_of_ifaces = nLen;
      for (int i = 0; i < nLen; i++) {
         arg->ifaces[i] = (dss_iface_id_type)aNetTypes[i];
      }

bail_get_all_ifaces:
      DSSCommon::ReleaseIf((IQI**)&pIDSNetworkPriv);
      return ret;
   }

   // For IOCTLs that get app_id in argval fetch the app_id from the argval
   // Forbid temporary , unless ioctl is not in the list
   bForbidTemporary = TRUE;
   switch (ioctl_name) {
      case DSS_IFACE_IOCTL_REG_EVENT_CB:
      {
         dss_iface_ioctl_ev_cb_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_ev_cb_type*>(argval_ptr);
         netHandle = pEvRegArgval->app_id;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }
         bAllowSecondary = TRUE;
         break;
      }

      case DSS_IFACE_IOCTL_DEREG_EVENT_CB:
      {
         dss_iface_ioctl_ev_cb_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_ev_cb_type*>(argval_ptr);
         netHandle = pEvRegArgval->app_id;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }
         bAllowSecondary = TRUE;
         break;
      }


#ifndef FEATURE_DSS_LINUX
      case DSS_IFACE_IOCTL_QOS_REQUEST:
      {
         dss_iface_ioctl_qos_request_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_qos_request_type*>(argval_ptr);
         netHandle = pEvRegArgval->app_id;
         if (0 > netHandle) {
            // Logging message.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }

         break;
      }

      case DSS_IFACE_IOCTL_QOS_REQUEST_EX:
      {
         dss_iface_ioctl_qos_request_ex_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_qos_request_ex_type*>(argval_ptr);
         netHandle = pEvRegArgval->app_id;
         if (0 > netHandle) {
            // Logging message.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }

         break;
      }

      case DSS_IFACE_IOCTL_PRIMARY_QOS_MODIFY:
      {
         dss_iface_ioctl_primary_qos_modify_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_primary_qos_modify_type*>(argval_ptr);
         netHandle = pEvRegArgval->dss_nethandle;
         if (0 > netHandle) {
            // Logging message.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }

         break;
      }

      case DSS_IFACE_IOCTL_MCAST_JOIN:
      {
         dss_iface_ioctl_mcast_join_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_mcast_join_type*>(argval_ptr);
         netHandle = pEvRegArgval->app_id;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }

         break;
      }


      case DSS_IFACE_IOCTL_MCAST_LEAVE:
      {
         dss_iface_ioctl_mcast_leave_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_mcast_leave_type*>(argval_ptr);
         netHandle = pEvRegArgval->app_id;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }

         break;
      }

      case DSS_IFACE_IOCTL_MCAST_JOIN_EX:
      {
         dss_iface_ioctl_mcast_join_ex_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_mcast_join_ex_type*>(argval_ptr);
         netHandle = pEvRegArgval->dss_nethandle;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }

         break;
      }

      case DSS_IFACE_IOCTL_MCAST_LEAVE_EX:
      {
         dss_iface_ioctl_mcast_leave_ex_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_mcast_leave_ex_type*>(argval_ptr);
         netHandle = pEvRegArgval->dss_nethandle;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }

         break;
      }

      case DSS_IFACE_IOCTL_MCAST_REGISTER_EX:
      {
         dss_iface_ioctl_mcast_register_ex_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_mcast_register_ex_type*>(argval_ptr);
         netHandle = pEvRegArgval->dss_nethandle;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }

         break;
      }

      case DSS_IFACE_IOCTL_MBMS_MCAST_CONTEXT_ACTIVATE:
      {
         dss_iface_ioctl_mbms_mcast_context_act_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_mbms_mcast_context_act_type*>(argval_ptr);
         netHandle = pEvRegArgval->nethandle;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }

         break;
      }


      case DSS_IFACE_IOCTL_MBMS_MCAST_CONTEXT_DEACTIVATE:
      {
         dss_iface_ioctl_mbms_mcast_context_deact_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_mbms_mcast_context_deact_type*>(argval_ptr);
         netHandle = pEvRegArgval->nethandle;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }

         break;
      }

      case DSS_IFACE_IOCTL_MT_REG_CB:
      {
         dss_iface_ioctl_mt_reg_cb_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_mt_reg_cb_type*>(argval_ptr);
         netHandle = pEvRegArgval->app_id;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }

         break;
      }

      case DSS_IFACE_IOCTL_MT_DEREG_CB:
      {
         dss_iface_ioctl_mt_dereg_cb_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_mt_dereg_cb_type*>(argval_ptr);
         netHandle = pEvRegArgval->app_id;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }

         break;
      }
#endif
      case DSS_IFACE_IOCTL_707_ENABLE_HDR_HPT_MODE:
      {
         dss_iface_ioctl_707_enable_hdr_hpt_mode_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_707_enable_hdr_hpt_mode_type*>(argval_ptr);
         netHandle = pEvRegArgval->dss_nethandle;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }

         break;
      }

      case DSS_IFACE_IOCTL_707_ENABLE_HDR_REV0_RATE_INTERIA:
      {
         dss_iface_ioctl_707_enable_hdr_rev0_rate_inertia_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_707_enable_hdr_rev0_rate_inertia_type*>(argval_ptr);
         netHandle = pEvRegArgval->dss_nethandle;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }

         break;
      }

      case DSS_IFACE_IOCTL_707_ENABLE_HDR_SLOTTED_MODE:
      {
         dss_iface_ioctl_707_enable_hdr_slotted_mode_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_707_enable_hdr_slotted_mode_type*>(argval_ptr);
         netHandle = pEvRegArgval->dss_nethandle;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }

         break;
      }

      case DSS_IFACE_IOCTL_ENABLE_FIREWALL:
      {
         dss_iface_ioctl_enable_firewall_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_enable_firewall_type*>(argval_ptr);
         netHandle = pEvRegArgval->dss_nethandle;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }
    
         break;
      }

      case DSS_IFACE_IOCTL_DISABLE_FIREWALL:
      {
         dss_iface_ioctl_disable_firewall_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_disable_firewall_type*>(argval_ptr);
         netHandle = pEvRegArgval->dss_nethandle;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }

         break;
      }

      case DSS_IFACE_IOCTL_ADD_FIREWALL_RULE:
      {
         dss_iface_ioctl_add_firewall_rule_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_add_firewall_rule_type*>(argval_ptr);
         netHandle = pEvRegArgval->dss_nethandle;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }

         break;
      }

      case DSS_IFACE_IOCTL_DELETE_FIREWALL_RULE:
      {
         dss_iface_ioctl_delete_firewall_rule_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_delete_firewall_rule_type*>(argval_ptr);
         netHandle = pEvRegArgval->dss_nethandle;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }

         break;
      }

      case DSS_IFACE_IOCTL_GET_FIREWALL_RULE:
      {
         dss_iface_ioctl_get_firewall_rule_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_get_firewall_rule_type*>(argval_ptr);
         netHandle = pEvRegArgval->dss_nethandle;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }

         break;
      }

      case DSS_IFACE_IOCTL_GET_FIREWALL_TABLE:
      {
         dss_iface_ioctl_get_firewall_table_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_get_firewall_table_type*>(argval_ptr);
         netHandle = pEvRegArgval->dss_nethandle;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }

         break;
      }

      case DSS_IFACE_IOCTL_ADD_STATIC_NAT_ENTRY:
      {
         dss_iface_ioctl_add_static_nat_entry_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_add_static_nat_entry_type*>(argval_ptr);
         netHandle = pEvRegArgval->nethandle;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }
  
         break;     
       }

      case DSS_IFACE_IOCTL_DELETE_STATIC_NAT_ENTRY:
      {
         dss_iface_ioctl_delete_static_nat_entry_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_delete_static_nat_entry_type*>(argval_ptr);
         netHandle = pEvRegArgval->nethandle;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }
  
         break;     
      }


      case DSS_IFACE_IOCTL_GET_STATIC_NAT_ENTRY:
      {
         dss_iface_ioctl_get_static_nat_entry_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_get_static_nat_entry_type*>(argval_ptr);
         netHandle = pEvRegArgval->nethandle;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }

         break;     
      }

      case DSS_IFACE_IOCTL_GET_DYNAMIC_NAT_ENTRY_TIMEOUT:
      {
         dss_iface_ioctl_get_dynamic_nat_entry_timeout_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_get_dynamic_nat_entry_timeout_type*>(argval_ptr);
         netHandle = pEvRegArgval->nethandle;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }
  
         break;     
      }

      case DSS_IFACE_IOCTL_SET_DYNAMIC_NAT_ENTRY_TIMEOUT:
      {
         dss_iface_ioctl_set_dynamic_nat_entry_timeout_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_set_dynamic_nat_entry_timeout_type*>(argval_ptr);
         netHandle = pEvRegArgval->nethandle;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }

         break;     
      }

      case DSS_IFACE_IOCTL_GET_NAT_IPSEC_VPN_PASS_THROUGH:
      {
         dss_iface_ioctl_nat_ipsec_vpn_pass_through_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_nat_ipsec_vpn_pass_through_type*>(argval_ptr);
         netHandle = pEvRegArgval->nethandle;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }
  
         break;     
      }

      case DSS_IFACE_IOCTL_SET_NAT_IPSEC_VPN_PASS_THROUGH:
      {
         dss_iface_ioctl_nat_ipsec_vpn_pass_through_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_nat_ipsec_vpn_pass_through_type*>(argval_ptr);
         netHandle = pEvRegArgval->nethandle;
         if (0 > netHandle) {
            // Logging messge.
            *dss_errno = DS_EBADAPP; // TODO: Correct?
            return DSS_ERROR;
         }

         break;     
      }

     case DSS_IFACE_IOCTL_GET_NAT_L2TP_VPN_PASS_THROUGH:
     {
        dss_iface_ioctl_nat_l2tp_vpn_pass_through_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_nat_l2tp_vpn_pass_through_type*>(argval_ptr);
        netHandle = pEvRegArgval->nethandle;
        if (0 > netHandle) {
           // Logging messge.
           *dss_errno = DS_EBADAPP; // TODO: Correct?
           return DSS_ERROR;
        }
  
        break;     
     }
  
     case DSS_IFACE_IOCTL_SET_NAT_L2TP_VPN_PASS_THROUGH:
     {
        dss_iface_ioctl_nat_l2tp_vpn_pass_through_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_nat_l2tp_vpn_pass_through_type*>(argval_ptr);
        netHandle = pEvRegArgval->nethandle;
        if (0 > netHandle) {
           // Logging messge.
           *dss_errno = DS_EBADAPP; // TODO: Correct?
           return DSS_ERROR;
        }
  
        break;     
     }

     case DSS_IFACE_IOCTL_GET_NAT_PPTP_VPN_PASS_THROUGH:
     {
        dss_iface_ioctl_nat_pptp_vpn_pass_through_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_nat_pptp_vpn_pass_through_type*>(argval_ptr);
        netHandle = pEvRegArgval->nethandle;
        if (0 > netHandle) {
           // Logging messge.
           *dss_errno = DS_EBADAPP; // TODO: Correct?
           return DSS_ERROR;
        }
  
        break;     
     }
  
     case DSS_IFACE_IOCTL_SET_NAT_PPTP_VPN_PASS_THROUGH:
     {
        dss_iface_ioctl_nat_pptp_vpn_pass_through_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_nat_pptp_vpn_pass_through_type*>(argval_ptr);
        netHandle = pEvRegArgval->nethandle;
        if (0 > netHandle) {
           // Logging messge.
           *dss_errno = DS_EBADAPP; // TODO: Correct?
           return DSS_ERROR;
        }
  
        break;     
     }

     case DSS_IFACE_IOCTL_ADD_DMZ:
     {
        dss_iface_ioctl_dmz_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_dmz_type*>(argval_ptr);
        netHandle = pEvRegArgval->nethandle;
        if (0 > netHandle) {
           // Logging messge.
           *dss_errno = DS_EBADAPP; // TODO: Correct?
           return DSS_ERROR;
        }
  
        break;     
      }

     case DSS_IFACE_IOCTL_GET_DMZ:
     {
        dss_iface_ioctl_dmz_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_dmz_type*>(argval_ptr);
        netHandle = pEvRegArgval->nethandle;
        if (0 > netHandle) {
           // Logging messge.
           *dss_errno = DS_EBADAPP; // TODO: Correct?
           return DSS_ERROR;
        }
  
        break;     
      }

     case DSS_IFACE_IOCTL_DELETE_DMZ:
     {
        dss_iface_ioctl_delete_dmz_type* pEvRegArgval = reinterpret_cast<dss_iface_ioctl_delete_dmz_type*>(argval_ptr);
        netHandle = pEvRegArgval->nethandle;
        if (0 > netHandle) {
           // Logging messge.
           *dss_errno = DS_EBADAPP; // TODO: Correct?
           return DSS_ERROR;
        }
  
        break;     
      }

      default:
         // Ioctl is not in the list , temporary will not be forbidden
         bForbidTemporary = FALSE;
   }

   // TODO (security!): write enhancement to keep a list of iface_ids provided to apps, and verify iface_id specified by apps
   // against that list before applying IOCTLs.
   // TODO: consider adding a flag for secondary creation
   bForbidTemporary = bForbidTemporary || IsQoSIoctl(ioctl_name);
   res = DSSGlobals::Instance()->GetNetApp(netHandle, iface_id, bAllowSecondary,bForbidTemporary,pNetApp, bIsTempDSSNetApp,ioctl_name,argval_ptr);
   if ( (AEE_SUCCESS != res) || (NULL == *pNetApp) ) {
      // TODO: consider a better error
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return DSS_ERROR;
   }

   // Note to the developer: IDSNetworkPrivScope.Fetch can only be used after IDSNetworkPrivScope.Init is called.
   // If IDSNetworkPrivScope.Fetch is needed before this point be sure to move IDSNetworkPrivScope.Init before it.
   res = IDSNetworkPrivScope.Init(*pNetApp);
   if (AEE_SUCCESS != res) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return DSS_ERROR;
   }

   res = IDSNetworkScope.Init(*pNetApp);
   if (AEE_SUCCESS != res) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return DSS_ERROR;
   }

   res = IDSNetworkExtScope.Init(*pNetApp);
   if (AEE_SUCCESS != res) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return DSS_ERROR;
   }

   switch (ioctl_name) {
      case DSS_IFACE_IOCTL_GET_IPV4_ADDR:
         return dss_iface_ioctl_get_ipv4_addr(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_GET_IPV6_ADDR:
         return dss_iface_ioctl_get_ipv6_addr(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_GET_IPV4_PRIM_DNS_ADDR:
         return dss_iface_ioctl_get_ipv4_prim_dns_addr(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_GET_IPV6_PRIM_DNS_ADDR:
         return dss_iface_ioctl_get_ipv6_prim_dns_addr(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_GET_IPV4_SECO_DNS_ADDR:
         return dss_iface_ioctl_get_ipv4_seco_dns_addr(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_GET_IPV6_SECO_DNS_ADDR:
         return dss_iface_ioctl_get_ipv6_seco_dns_addr(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_GET_ALL_DNS_ADDRS:
         return dss_iface_ioctl_get_all_dns_addrs(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_GET_MTU:
         return dss_iface_ioctl_get_mtu(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_GET_IP_ADDR:
         return dss_iface_ioctl_get_ip_addr(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_GET_STATE:
         return dss_iface_ioctl_get_state(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_GET_PHYS_LINK_STATE:
         return dss_iface_ioctl_get_phys_link_state(argval_ptr, pNetApp, dss_errno,iface_id);

      case DSS_IFACE_IOCTL_GET_RF_CONDITIONS:
         return dss_iface_ioctl_get_rf_conditions(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_REG_EVENT_CB:
         return dss_iface_ioctl_reg_event_cb(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_DEREG_EVENT_CB:
         return dss_iface_ioctl_dereg_event_cb(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_GET_HW_ADDR:
         return dss_iface_ioctl_get_hw_addr(argval_ptr, &IDSNetworkScope, dss_errno);

      case DSS_IFACE_IOCTL_GET_IFACE_NAME:
         return dss_iface_ioctl_get_iface_name(argval_ptr, &IDSNetworkScope, dss_errno);

     case DSS_IFACE_IOCTL_GET_DEVICE_INFO:
        return dss_iface_ioctl_get_device_info(argval_ptr, &IDSNetworkScope, dss_errno);

     case DSS_IFACE_IOCTL_GET_INTERFACE_GATEWAY_V4_ADDR:
        return dss_iface_ioctl_get_ipv4_gateway_info(argval_ptr, &IDSNetworkScope, dss_errno);

      case DSS_IFACE_IOCTL_GET_IP_FAMILY:
         return dss_iface_ioctl_get_ip_family(argval_ptr, &IDSNetworkScope, dss_errno);

      case DSS_IFACE_IOCTL_GET_BEARER_TECHNOLOGY:
         return dss_iface_ioctl_get_bearer_technology(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_GET_DATA_BEARER_RATE:
         return dss_iface_ioctl_get_data_bearer_rate(argval_ptr, &IDSNetworkExtScope, dss_errno);

      case DSS_IFACE_IOCTL_IS_LAPTOP_CALL_ACTIVE:
         return dss_iface_ioctl_is_laptop_call_active(argval_ptr, &IDSNetworkPrivScope, dss_errno);

      case DSS_IFACE_IOCTL_GET_SIP_SERV_ADDR:
         return dss_iface_ioctl_get_sip_serv_addr(argval_ptr, &IDSNetworkScope, dss_errno);

      case DSS_IFACE_IOCTL_GET_SIP_SERV_DOMAIN_NAMES:
         return dss_iface_ioctl_get_sip_serv_domain_names(argval_ptr, &IDSNetworkScope, dss_errno);

      case DSS_IFACE_IOCTL_GENERATE_PRIV_IPV6_ADDR:
        return dss_iface_ioctl_generate_priv_ipv6_addr(argval_ptr, pNetApp, &IDSNetworkScope, dss_errno,iface_id);

      case DSS_IFACE_IOCTL_GET_ALL_V6_PREFIXES:
        return dss_iface_ioctl_get_all_v6_prefixes(argval_ptr, pNetApp, &IDSNetworkScope, dss_errno);


      case DSS_IFACE_IOCTL_GET_DOMAIN_NAME_SEARCH_LIST:
         return dss_iface_ioctl_get_domain_name_search_list(argval_ptr, &IDSNetworkScope, dss_errno);


      case DSS_IFACE_IOCTL_REFRESH_DHCP_CONFIG_INFO:
         return dss_iface_ioctl_refresh_dhcp_config_info(&IDSNetworkScope, dss_errno);

      case DSS_IFACE_IOCTL_GO_ACTIVE:
         return dss_iface_ioctl_go_active(argval_ptr, pNetApp, dss_errno, iface_id);


      case DSS_IFACE_IOCTL_GO_DORMANT:
         return dss_iface_ioctl_go_dormant(argval_ptr, pNetApp, dss_errno, iface_id);


      case DSS_IFACE_IOCTL_GO_NULL:
         return dss_iface_ioctl_go_null(argval_ptr, &IDSNetworkScope, dss_errno);


      case DSS_IFACE_IOCTL_IPCP_DNS_OPT:
         return dss_iface_ioctl_ipcp_dns_opt(argval_ptr, &IDSNetworkPrivScope, dss_errno);


#ifndef FEATURE_DSS_LINUX
      case DSS_IFACE_IOCTL_GET_TX_QUEUE_LEVEL:
         return dss_iface_ioctl_get_tx_queue_level(argval_ptr, pNetApp, dss_errno, iface_id);
#endif
      // All 707 ioctls fall through to a common handler.
      case DSS_IFACE_IOCTL_707_GET_MDR:
      case DSS_IFACE_IOCTL_707_SET_MDR:
      case DSS_IFACE_IOCTL_707_GET_DORM_TIMER:
      case DSS_IFACE_IOCTL_707_SET_DORM_TIMER:
      case DSS_IFACE_IOCTL_707_GET_RLP_ALL_CURR_NAK:
      case DSS_IFACE_IOCTL_707_SET_RLP_ALL_CURR_NAK:
      case DSS_IFACE_IOCTL_707_GET_RLP_ALL_DEF_NAK:
      case DSS_IFACE_IOCTL_707_SET_RLP_ALL_DEF_NAK:
      case DSS_IFACE_IOCTL_707_GET_RLP_ALL_NEG_NAK:
      case DSS_IFACE_IOCTL_707_GET_RLP_QOS_NA_PRI:
      case DSS_IFACE_IOCTL_707_SET_RLP_QOS_NA_PRI:
      case DSS_IFACE_IOCTL_707_SDB_SUPPORT_QUERY:
      case DSS_IFACE_IOCTL_707_ENABLE_HOLDDOWN:
      case DSS_IFACE_IOCTL_707_ENABLE_HDR_HPT_MODE:
      case DSS_IFACE_IOCTL_707_ENABLE_HDR_REV0_RATE_INTERIA:
      case DSS_IFACE_IOCTL_707_ENABLE_HDR_SLOTTED_MODE:
      case DSS_IFACE_IOCTL_707_HDR_GET_RMAC3_INFO:
      case DSS_IFACE_IOCTL_707_GET_TX_STATUS:
      case DSS_IFACE_IOCTL_GET_SESSION_TIMER: //TODO: is it ok? (no 707 in the name)
      case DSS_IFACE_IOCTL_SET_SESSION_TIMER:
      case DSS_IFACE_IOCTL_707_GET_INACTIVITY_TIMER:
      case DSS_IFACE_IOCTL_707_SET_INACTIVITY_TIMER:
         return IfaceIoctl707(pNetApp, IDSNetworkScope.Fetch(), ioctl_name, argval_ptr, dss_errno, iface_id);
#ifndef FEATURE_DSS_LINUX
#ifdef FEATURE_BCMCS
      case DSS_IFACE_IOCTL_BCMCS_DB_UPDATE:
         return dss_iface_ioctl_bcmcs_db_update(argval_ptr, &IDSNetworkScope, dss_errno);
#endif // FEATURE_BCMCS

      case DSS_IFACE_IOCTL_QOS_REQUEST:
         return dss_iface_ioctl_qos_request(argval_ptr, pNetApp, &IDSNetworkPrivScope, dss_errno, iface_id);

      case DSS_IFACE_IOCTL_QOS_RELEASE:
         return dss_iface_ioctl_qos_release(argval_ptr, pNetApp, &IDSNetworkPrivScope, dss_errno);

      case DSS_IFACE_IOCTL_QOS_GET_GRANTED_FLOW_SPEC:
         return dss_iface_ioctl_qos_get_granted_flow_spec(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_QOS_GET_GRANTED_FLOW_SPEC2:
         return dss_iface_ioctl_qos_get_granted_flow_spec2(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_QOS_RESUME:
         return dss_iface_ioctl_qos_resume(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_QOS_MODIFY:
         return dss_iface_ioctl_qos_modify(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_QOS_SUSPEND:
         return dss_iface_ioctl_qos_suspend(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_QOS_GET_STATUS:
         return dss_iface_ioctl_qos_get_status(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_QOS_REQUEST_EX:
         return dss_iface_ioctl_qos_request_ex(argval_ptr, pNetApp, &IDSNetworkPrivScope, dss_errno,iface_id);

      case DSS_IFACE_IOCTL_QOS_RELEASE_EX:
         return dss_iface_ioctl_qos_release_ex(argval_ptr, pNetApp, &IDSNetworkPrivScope, dss_errno, iface_id);

      case DSS_IFACE_IOCTL_QOS_SUSPEND_EX:
         return dss_iface_ioctl_qos_suspend_ex(argval_ptr, pNetApp, &IDSNetworkPrivScope, dss_errno, iface_id);

      case DSS_IFACE_IOCTL_QOS_RESUME_EX:
         return dss_iface_ioctl_qos_resume_ex(argval_ptr, pNetApp, &IDSNetworkExtScope, dss_errno, iface_id);

      case DSS_IFACE_IOCTL_PRIMARY_QOS_MODIFY:
         return dss_iface_ioctl_primary_qos_modify(argval_ptr,pNetApp,dss_errno,iface_id);

      case DSS_IFACE_IOCTL_PRIMARY_QOS_GET_GRANTED_FLOW_SPEC:
         return dss_iface_ioctl_primary_qos_get_granted_flow_spec(argval_ptr, pNetApp, dss_errno, iface_id);

      case DSS_IFACE_IOCTL_ON_QOS_AWARE_SYSTEM:
         return dss_iface_ioctl_on_qos_aware_system(argval_ptr, &IDSNetworkExtScope, dss_errno);

      case DSS_IFACE_IOCTL_GET_NETWORK_SUPPORTED_QOS_PROFILES:
         return dss_iface_ioctl_get_network_supported_qos_profiles(argval_ptr, pNetApp, dss_errno);
#endif

      case DSS_IFACE_IOCTL_707_GET_HDR_1X_HANDDOWN_OPTION:
         return dss_iface_ioctl_707_get_hdr_1x_handdown_option(argval_ptr, &IDSNetworkScope, dss_errno);

      case DSS_IFACE_IOCTL_707_SET_HDR_1X_HANDDOWN_OPTION:
         return dss_iface_ioctl_707_set_hdr_1x_handdown_option(argval_ptr, &IDSNetworkScope, dss_errno);

#ifndef FEATURE_DSS_LINUX
      case DSS_IFACE_IOCTL_MCAST_JOIN:
         return dss_iface_ioctl_mcast_join(argval_ptr, pNetApp, &IDSNetworkPrivScope, dss_errno, iface_id);

      case DSS_IFACE_IOCTL_MCAST_JOIN_EX:
         return dss_iface_ioctl_mcast_join_ex(argval_ptr, pNetApp, dss_errno, iface_id);

      case DSS_IFACE_IOCTL_MCAST_LEAVE:
         return dss_iface_ioctl_mcast_leave(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_MCAST_LEAVE_EX:
         return dss_iface_ioctl_mcast_leave_ex(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_MCAST_REGISTER_EX:
         return dss_iface_ioctl_mcast_register_ex(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_MBMS_MCAST_CONTEXT_ACTIVATE:
         return dss_iface_ioctl_mbms_mcast_context_activate(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_MBMS_MCAST_CONTEXT_DEACTIVATE:
         return dss_iface_ioctl_mbms_mcast_context_deactivate(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_BCMCS_ENABLE_HANDOFF_REG:
         return dss_iface_ioctl_bcmcs_enable_handoff_reg(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_MT_REG_CB:
         return dss_iface_ioctl_mt_reg_cb(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_MT_DEREG_CB:
         return dss_iface_ioctl_mt_dereg_cb(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_QOS_GET_MODE:
         return dss_iface_ioctl_qos_get_mode(argval_ptr, &IDSNetworkScope, dss_errno);
#endif
      case DSS_IFACE_IOCTL_GET_DORMANCY_INFO_CODE:
         return dss_iface_ioctl_get_dormancy_info_code(argval_ptr, &IDSNetworkScope, dss_errno);

      case DSS_IFACE_IOCTL_UMTS_GET_IM_CN_FLAG:
         return dss_iface_ioctl_umts_get_im_cn_flag(argval_ptr, &IDSNetworkScope, dss_errno);

      case DSS_IFACE_IOCTL_ENABLE_FIREWALL:
        return dss_iface_ioctl_enable_firewall(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_DISABLE_FIREWALL:
        return dss_iface_ioctl_disable_firewall(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_ADD_FIREWALL_RULE:
        return dss_iface_ioctl_add_firewall_rule(argval_ptr, pNetApp, &IDSNetworkPrivScope, dss_errno);

      case DSS_IFACE_IOCTL_DELETE_FIREWALL_RULE:
        return dss_iface_ioctl_delete_firewall_rule(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_GET_FIREWALL_RULE:
        return dss_iface_ioctl_get_firewall_rule(argval_ptr, pNetApp, &IDSNetworkPrivScope, dss_errno);

      case DSS_IFACE_IOCTL_GET_FIREWALL_TABLE:
        return dss_iface_ioctl_get_firewall_table(argval_ptr, pNetApp, &IDSNetworkPrivScope, dss_errno);

      case DSS_IFACE_IOCTL_ADD_STATIC_NAT_ENTRY:
        return dss_iface_ioctl_add_static_nat_entry(argval_ptr, pNetApp, dss_errno);
  
      case DSS_IFACE_IOCTL_DELETE_STATIC_NAT_ENTRY:
        return dss_iface_ioctl_delete_static_nat_entry(argval_ptr, pNetApp, dss_errno);
    
      case DSS_IFACE_IOCTL_GET_STATIC_NAT_ENTRY:
        return dss_iface_ioctl_get_static_nat_entry(argval_ptr, pNetApp, dss_errno);
    
      case DSS_IFACE_IOCTL_GET_DYNAMIC_NAT_ENTRY_TIMEOUT:
        return dss_iface_ioctl_get_dynamic_nat_entry_timeout(argval_ptr, pNetApp, dss_errno);
    
      case DSS_IFACE_IOCTL_SET_DYNAMIC_NAT_ENTRY_TIMEOUT:
        return dss_iface_ioctl_set_dynamic_nat_entry_timeout(argval_ptr, pNetApp, dss_errno);
    
      case DSS_IFACE_IOCTL_GET_NAT_IPSEC_VPN_PASS_THROUGH:
        return dss_iface_ioctl_get_nat_ipsec_vpn_pass_through(argval_ptr, pNetApp, dss_errno);
  
      case DSS_IFACE_IOCTL_SET_NAT_IPSEC_VPN_PASS_THROUGH:
        return dss_iface_ioctl_set_nat_ipsec_vpn_pass_through(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_GET_NAT_L2TP_VPN_PASS_THROUGH:
        return dss_iface_ioctl_get_nat_l2tp_vpn_pass_through(argval_ptr, pNetApp, dss_errno);
  
      case DSS_IFACE_IOCTL_SET_NAT_L2TP_VPN_PASS_THROUGH:
        return dss_iface_ioctl_set_nat_l2tp_vpn_pass_through(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_GET_NAT_PPTP_VPN_PASS_THROUGH:
        return dss_iface_ioctl_get_nat_pptp_vpn_pass_through(argval_ptr, pNetApp, dss_errno);
  
      case DSS_IFACE_IOCTL_SET_NAT_PPTP_VPN_PASS_THROUGH:
        return dss_iface_ioctl_set_nat_pptp_vpn_pass_through(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_ADD_DMZ:
        return dss_iface_ioctl_add_dmz(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_GET_DMZ:
        return dss_iface_ioctl_get_dmz(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_DELETE_DMZ:
        return dss_iface_ioctl_delete_dmz(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_GET_IFACE_STATS:
        return dss_iface_ioctl_get_iface_stats(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_RESET_IFACE_STATS:
        return dss_iface_ioctl_reset_iface_stats(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_GET_NAT_PUBLIC_IP_ADDR:
        return dss_iface_ioctl_get_nat_public_ip_addr(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_UW_FMC_SET_TUNNEL_PARAMS:
        return dss_iface_ioctl_set_fmc_tunnel_params(argval_ptr, pNetApp, dss_errno);

      case DSS_IFACE_IOCTL_UW_FMC_RESET_TUNNEL_PARAMS:
        return dss_iface_ioctl_reset_fmc_tunnel_params(argval_ptr, pNetApp, dss_errno);

#ifdef FEATURE_DATA_PS_DHCP_SERVER_SOFTAP

      case DSS_IFACE_IOCTL_DHCP_ARP_CACHE_UPDATE:
        return dss_iface_ioctl_dhcp_arp_cache_update(argval_ptr, &IDSNetworkPrivScope, dss_errno);

      case DSS_IFACE_IOCTL_DHCP_ARP_CACHE_CLEAR:
        return dss_iface_ioctl_dhcp_arp_cache_clear(argval_ptr, &IDSNetworkPrivScope, dss_errno);

      case DSS_IFACE_IOCTL_DHCP_SERVER_GET_DEVICE_INFO:
        return dss_iface_ioctl_dhcp_get_device_info(argval_ptr, &IDSNetworkPrivScope, dss_errno);

#endif /* FEATURE_DATA_PS_DHCP_SERVER_SOFTAP */

      default:
         *dss_errno = DS_EOPNOTSUPP;
         return DSS_ERROR;
   }
}

/*===========================================================================
FUNCTION DSS_IFACE_IOCTL()

DESCRIPTION
This function determines the ps_iface_ptr associated with the passed in
identifier. It then calls ps_iface_ioctl().

DEPENDENCIES
None.

PARAMETERS

dss_iface_id_type         - Interface ID on which the specified operations
is to be performed

dss_iface_ioctl_type      - The operation name

void*                     - Pointer to operation specific structure

sint15*                   - Error code returned in case of failure (Error
values are those defined in dserrno.h)

DS_EBADF - Returned by dss_iface_ioctl() if the
specified id_ptr is invalid (i.e. id_ptr does
not map to a valid ps_iface_ptr).

DS_EINVAL - Returned by dss_iface_ioctl() when
the specified IOCTL does not belong to the
common set of IOCTLs and there is no IOCTL mode
handler registered for the specified interface.

DS_EOPNOTSUPP - Returned by the lower level
IOCTL mode handler when specified IOCTL is not
supported by the interface. For instance, this
would be returned by interfaces that do not
support a certain "iface specific common IOCTL"
(i.e. these are common IOCTLs, but the
implementation is mode specific, for example,
GO_DORMANT).

DS_EFAULT - This error code is returned if the
specified arguments for the IOCTL are incorrect
or if dss_iface_ioctl() or a mode handler
encounters an error while executing the IOCTL..
For instance, if the 1X interface cannot
"GO_DORMANT" it would return this error.

DS_NOMEMORY - This error code is returned if we
run out of buffers during execution.

RETURN VALUE
0 - on success
-1 - on failure

SIDE EFFECTS
None.

===========================================================================*/
int dss_iface_ioctl
(
 dss_iface_id_type        iface_id,
 dss_iface_ioctl_type     ioctl_name,
 void                     *argval_ptr,
 sint15                   *dss_errno
)
{
   bool        bIsTempDSSNetApp = FALSE;  // TODO: move inside DSSNetApp
   int         nRet = DSS_SUCCESS;

   // IMPL: Call function to find out if iface_id contains app_id or not. Set bAppIdIdentified accordingly
   DSSNetApp*  pNetApp = NULL;

   LOG_MSG_FUNCTION_ENTRY("dss_iface_ioctl - iface_id:%u, ioctl_name:%d, argval_ptr:0x%p", iface_id, ioctl_name, argval_ptr);

   nRet = dss_iface_ioctl_internal(iface_id,
                                   ioctl_name,
                                   argval_ptr,
                                   dss_errno,
                                   &pNetApp,
                                   &bIsTempDSSNetApp);

   if (TRUE == bIsTempDSSNetApp)
   {
      DSSCommon::ReleaseIf((IQI**)&pNetApp);
   }

   return nRet;
}
