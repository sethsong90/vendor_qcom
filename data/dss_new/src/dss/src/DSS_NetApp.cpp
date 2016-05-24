/*======================================================

FILE:  DSSNetApp.cpp

SERVICES:

GENERAL DESCRIPTION:
Implementation of DSSNetApp class

=====================================================

Copyright (c) 2008-2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_NetApp.cpp#5 $
  $DateTime: 2011/08/04 04:13:26 $$Author: eazriel $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2011-07-15 rp  QShrink2 changes: MSG_* macros are removed from inline functions.
  2010-04-18 en  History added.

===========================================================================*/


//===================================================================
//   Includes and Public Data Declarations
//===================================================================

#include "DSS_NetApp.h"
#include "DSS_Globals.h"
#include "DSS_Conversion.h"
#include "DSS_Common.h"

#include "DSS_NetworkStateHandler.h"
#include "DSS_NetworkIPHandler.h"
#include "DSS_ExtendedIPConfigHandler.h"
#include "DSS_RFConditionsHandler.h"
#include "DSS_BearerTechHandler.h"
#include "DSS_OutageHandler.h"
#include "DSS_PhysLinkStateHandler.h"
#include "DSS_QoSAwareUnAwareHandler.h"
#include "DSS_PrimaryQoSModifyHandler.h"
#include "DSS_PrimaryQoSModifyStatusHandler.h"
#include "DSS_MTPDRequestHandler.h"
#include "DSS_SlottedResultHandler.h"
#include "DSS_SlottedSessionChangedHandler.h"
#include "DSS_HDRRev0RateInteriaHandler.h"
#include "DSS_QoSProfileChangedHandler.h"
#include "DSS_IPv6PrefixChangedStateHandler.h"
#include "DSS_IDSNetworkPrivScope.h"
#include "DSS_IDSNetworkScope.h"
#include "DSS_MemoryManagement.h"
#include "DSS_PrivIpv6Addr.h"
#include "ds_Net_IIPv6Address.h"
#include "DSS_GenScope.h"

#include "ds_Net_INetworkPriv.h"

#include "DSS_MCast.h"

#include "DSS_EventHandlerMCastMBMSCtrl.h"
#include "DSS_NetMCastMBMSCtrl.h"

#include "ds_Addr_Def.h"
#include "AEECCritSect.h"
#include "ds_Utils_Atomic.h"
#include "ds_Utils_CreateInstance.h"

using namespace ds::Net;
using namespace ds::Error;
//-------------------------------------------------------------------
// Include Files
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// Constant / Define Declarations
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// Type Declarations (typedef, struct, enum, etc.)
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// Global Constant Data Declarations
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// Global Data Declarations
//-------------------------------------------------------------------


//-------------------------------------------------------------------
// Forward Declarations
//-------------------------------------------------------------------

//===================================================================
//              Macro Definitions
//===================================================================
#define DSS_DNS_PRIMARY_ADDR_IDX   0

#define DSS_DNS_SECONDARY_ADDR_IDX 1

#ifndef min
#define min(a,b)  ((a) < (b) ? (a):(b))
#endif

//===================================================================
//            DSSNetApp Functions Definitions
//===================================================================

// TODO: There's nothing preventing this class from being instantiated
// (has default operator new and operator delete)

//===================================================================
//  FUNCTION:   DSSNetApp::DSSNetApp
//
//  DESCRIPTION:
//  Constructor of the DSSNetApp class.
//===================================================================
// TODO: this should be constructed in two phases (never-failing constructor and
// an init() method).

DSSNetApp::DSSNetApp(): mNetHandle(-1), mIfaceId(0), 
   mEventReportIfaceId(0), mpIDSNetworkPriv(NULL),
   mpIDSNetwork(NULL), mpIDSNetworkExt(NULL), mpIDSNetworkExt2(NULL),  mNetCb(NULL),
   mNetUserData(NULL), // TODO: can't initialize mSockCb here since it's a structure
   mpIDSNetPolicy(0), mpDSNetQoSManager(NULL), mpDSSQoSSecondaryList(NULL),
   mpDSSPrivIpv6AddrList(NULL), mpDSSNetQoSDefault(NULL), mpDSSMCastList(NULL),
   mpDSSNetMCastMBMSList(NULL), mpDSNetMCastManager(NULL),mpDSNetFirewallManager(NULL),
   mpDSNetNatSession(NULL),mpIDSNetwork1xPriv(NULL),mNext(NULL),
   mnNumOfSockets(0), mpNetStateHandler(NULL), mpNetIPHandler(NULL),
   mpExtendedIPConfigHandler(NULL), mpRFConditionsHandler(NULL),
   mpBearerTechHandler(NULL), mpOutageHandler(NULL), mpPhysLinkStateHandler(NULL),
   mpQoSAwareUnAwareHandler(NULL), mpPrimaryQoSModifyHandler(NULL),
   mpPrimaryQoSModifyStatusHandler(NULL), mpMTPDRequestHandler(NULL),
   mpSlottedResultHandler(NULL),mpSlottedSessionChangedHandler(NULL),
   mpQoSProfileChangedHandler(NULL), mpHDRRev0RateInteriaHandler(NULL),
   mpDSSIPv6PrefixChangedStateHandler(NULL), mbIpsecDisabled(FALSE),
   mpIfaceUpOrDownSignal(NULL),mpIfaceUpOrDownSignalCtl(NULL),
   mpCritSect(NULL), mbNetworkIsUp(FALSE), mbIsPPPOpen(FALSE), 
   mbLastIfaceStateSentToApp(IFACE_STATE_INVALID), mbAutoEventsRegistered(FALSE),
   mRegObj(NULL),mpDSNetPhysLink(0), mpDSNetTechUMTS(0)
{
   memset(DSSFirewallHandleToObject, 0, 
          DSS_MAX_FIREWALL_HANDLES * sizeof(DSSFirewallHandleToObjectMapping));
   
   // DSSNetApp::Destructor makes use of mpCritSect before Releasing it
   // So we ASSERT here in case mpCritSect fails, otherwise 
   // DSSNetApp::Destructor will access uninitialized mpCritSect
   if (AEE_SUCCESS != DSSGlobals::Instance()->GetCritSect(&mpCritSect)) {
      LOG_MSG_ERROR( "Couldn't fetch CritSect object from DSSGlobals", 0, 0, 0);
      ASSERT(0);
   }
}

AEEResult DSSNetApp::GetPolicy(IPolicy** ppIDSNetPolicy)
{
   DSSCritScope cs(*mpCritSect);

   if (NULL == mpIDSNetworkPriv) {
      return QDS_EINVAL;
   }

   if (NULL == mpIDSNetPolicy) {
      return QDS_EINVAL;
   }

   *ppIDSNetPolicy = mpIDSNetPolicy;
   (void)mpIDSNetPolicy->AddRef();
   return AEE_SUCCESS;
}

AEEResult DSSNetApp::SetPolicy(ds::Net::IPolicy* pIDSNetPolicy)
{
   ds::Net::IPolicy* pIDSNetPolicyCopy = NULL;
   mpCritSect->Enter();

   // If mpIDSNetPolicy is not NULL then before changing the pointer
   // we release the current reference to avoid memory leak.
   if (NULL != mpIDSNetPolicy) {
      DSSCommon::ReleaseIf((IQI**)&mpIDSNetPolicy);
   }

   mpIDSNetPolicy = pIDSNetPolicy;

   if (NULL != mpIDSNetPolicy)
   {
      (void)mpIDSNetPolicy->AddRef();
   }

   pIDSNetPolicyCopy = mpIDSNetPolicy;
   mpCritSect->Leave();

   if (NULL != mpIDSNetworkPriv)
   {
      (void)mpIDSNetworkPriv->SetPolicy(pIDSNetPolicyCopy);
   }

   return AEE_SUCCESS;
}

AEEResult DSSNetApp::GetIDSNetworkPrivObject(INetworkPriv** ppIDSNetworkPriv)
{
   // TODO: maybe it should not be inline?
   DSSCritScope cs(*mpCritSect);
   *ppIDSNetworkPriv = mpIDSNetworkPriv;
   if (NULL == mpIDSNetworkPriv) {
      return QDS_EINVAL;
   }
   (void)mpIDSNetworkPriv->AddRef();
   return AEE_SUCCESS;
}

AEEResult DSSNetApp::GetIDSNetworkObject(INetwork** ppIDSNetwork)
{
   // TODO: maybe it should not be inline?
   DSSCritScope cs(*mpCritSect);
   *ppIDSNetwork = mpIDSNetwork;
   if (NULL == mpIDSNetwork) {
      return QDS_EINVAL;
   }
   (void)mpIDSNetwork->AddRef();
   return AEE_SUCCESS;
}

AEEResult DSSNetApp::GetIDSNetworkExtObject(INetworkExt** ppIDSNetworkExt)
{
   // TODO: maybe it should not be inline?
   DSSCritScope cs(*mpCritSect);
   *ppIDSNetworkExt = mpIDSNetworkExt;
   if (NULL == mpIDSNetworkExt) {
      return QDS_EINVAL;
   }
   (void)mpIDSNetworkExt->AddRef();
   return AEE_SUCCESS;
}

AEEResult DSSNetApp::GetIDSNetworkExt2Object(INetworkExt2** ppIDSNetworkExt2)
{
  // TODO: maybe it should not be inline?
  DSSCritScope cs(*mpCritSect);
  *ppIDSNetworkExt2 = mpIDSNetworkExt2;
  if (NULL == mpIDSNetworkExt2) {
    return QDS_EINVAL;
  }
  (void)mpIDSNetworkExt2->AddRef();
  return AEE_SUCCESS;
}

//===================================================================
//  FUNCTION:   DSSNetApp::Init
//
//  DESCRIPTION:
//  Init the DSSNetApp class.
//===================================================================
AEEResult DSSNetApp::Init(INetworkPriv* pIDSNetworkPriv)
{
   // TODO: implement the initialization of the rest of the fields
   ISignalFactory *pSignalFactory = NULL;
   INetwork* pIDSNetwork = NULL;
   INetworkExt* pIDSNetworkExt = NULL;
   INetworkExt2* pIDSNetworkExt2 = NULL;
   AEEResult res = AEE_SUCCESS;

   res = DSSGlobals::Instance()->GetSignalFactory(&pSignalFactory);

   if (NULL == pSignalFactory || AEE_SUCCESS != res) {
      LOG_MSG_ERROR( "Couldn't fetch pSignalFactory from DSSGlobals result = %d", res, 0, 0);
      goto bail;
   }

   res = pSignalFactory->CreateSignal(&signalHandler,
                                       reinterpret_cast <uint32> (IfaceUpOrDownSignalFcn),
                                       reinterpret_cast <uint32> (this),
                                       &mpIfaceUpOrDownSignal,
                                       &mpIfaceUpOrDownSignalCtl);

   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR( "Couldn't create mpIfaceUpOrDownSignal result = %d", res, 0, 0);
      goto bail;
   }

   SetIDSNetworkPrivObject(pIDSNetworkPriv);   // DSSNetApp takes care to AddRef pIDSNetworkPriv once it is set


   res = pIDSNetworkPriv->QueryInterface(AEEIID_INetwork, (void**)&pIDSNetwork);

   if(AEE_SUCCESS != res){
      LOG_MSG_ERROR( "Couldn't QueryInterface for AEEIID_INetwork result = %d", res, 0, 0);
      goto bail;
   }

   SetIDSNetworkObject(pIDSNetwork);   // DSSNetApp takes care to AddRef pIDSNetwork once it is set

   res = pIDSNetwork->QueryInterface(AEEIID_INetworkExt, (void**)&pIDSNetworkExt);

   if(AEE_SUCCESS != res){
      LOG_MSG_ERROR( "Couldn't QueryInterface for AEEIID_INetworkExt result = %d", res, 0, 0);
      goto bail;
   }

   SetIDSNetworkExtObject(pIDSNetworkExt);   // DSSNetApp takes care to AddRef pIDSNetworkExt once it is set

   res = pIDSNetwork->QueryInterface(AEEIID_INetworkExt2, (void**)&pIDSNetworkExt2);

   if(AEE_SUCCESS != res){
      LOG_MSG_ERROR( "Couldn't QueryInterface for AEEIID_INetworkExt2 result = %d", res, 0, 0);
     goto bail;
   }

   SetIDSNetworkExt2Object(pIDSNetworkExt2);   // DSSNetApp takes care to AddRef pIDSNetworkExt2 once it is set

   res = AEE_SUCCESS;

/* fall through */

bail:

   DSSCommon::ReleaseIf((IQI**)&pIDSNetwork);
   DSSCommon::ReleaseIf((IQI**)&pIDSNetworkExt);
   DSSCommon::ReleaseIf((IQI**)&pIDSNetworkExt2);
   DSSCommon::ReleaseIf((IQI**)&pSignalFactory);

   return res;
}

//===================================================================
//  FUNCTION:   DSSNetApp::~DSSNetApp
//
//  DESCRIPTION:
//  Destructor of the DSSNetApp class.
//===================================================================
/*lint -e{1551} */
void DSSNetApp::Destructor() throw()
{
   PS_MEM_RELEASE(mpNetStateHandler);
   PS_MEM_RELEASE(mpNetIPHandler);
   PS_MEM_RELEASE(mpExtendedIPConfigHandler);
   PS_MEM_RELEASE(mpRFConditionsHandler);
   PS_MEM_RELEASE(mpBearerTechHandler);
   PS_MEM_RELEASE(mpOutageHandler);
   PS_MEM_RELEASE(mpPhysLinkStateHandler);
   PS_MEM_RELEASE(mpQoSAwareUnAwareHandler);
   PS_MEM_RELEASE(mpPrimaryQoSModifyHandler);
   PS_MEM_RELEASE(mpPrimaryQoSModifyStatusHandler);
   PS_MEM_RELEASE(mpMTPDRequestHandler);
   PS_MEM_RELEASE(mpSlottedResultHandler);
   PS_MEM_RELEASE(mpSlottedSessionChangedHandler);
   PS_MEM_RELEASE(mpQoSProfileChangedHandler);
   PS_MEM_RELEASE(mpHDRRev0RateInteriaHandler);
   PS_MEM_RELEASE(mpDSSIPv6PrefixChangedStateHandler);

   PS_MEM_DELETE(mpDSSNetQoSDefault);

   DSSCommon::ReleaseIf((IQI**)&mpIfaceUpOrDownSignal);
   DSSCommon::ReleaseIf((IQI**)&mpIfaceUpOrDownSignalCtl);
   DSSCommon::ReleaseIf((IQI**)&mpDSNetQoSManager);
   DSSCommon::ReleaseIf((IQI**)&mpIDSNetworkPriv);
   DSSCommon::ReleaseIf((IQI**)&mpIDSNetwork);
   DSSCommon::ReleaseIf((IQI**)&mpIDSNetworkExt);
   DSSCommon::ReleaseIf((IQI**)&mpIDSNetworkExt2);
   DSSCommon::ReleaseIf((IQI**)&mpIDSNetPolicy);
   DSSCommon::ReleaseIf((IQI**)&mpDSNetTechUMTS);
   DSSCommon::ReleaseIf((IQI**)&mpDSNetPhysLink);
   DSSCommon::ReleaseIf((IQI**)&mpDSNetMCastManager);
   DSSCommon::ReleaseIf((IQI**)&mpDSNetNatSession);
   DSSCommon::ReleaseIf((IQI**)&mpIDSNetwork1xPriv);
   // needs to be before mpCritSect releasing, because FreeLists() is using it
   FreeLists();

   DSSCommon::ReleaseIf((IQI**)&mpCritSect);
   SignalHandlerBase::Destructor();
}
/*lint –restore */

AEEResult DSSNetApp::RegAutoEvents()
{
   if (NULL == mpIDSNetworkPriv) {
      return QDS_EINVAL;
   }

   // Allow registration of auto events only once.
   if (TRUE == mbAutoEventsRegistered) {
      return AEE_SUCCESS;
   }

   mbAutoEventsRegistered = TRUE;

   // There are 2 events that every network is automatically registered to: iface up & iface down
   LOG_MSG_INFO1("Registering to QDS_EV_STATE_CHANGED, NetworkPriv obj 0x%p", mpIDSNetworkPriv, 0, 0);
   IDS_ERR_RET(mpIDSNetworkPriv->OnStateChange(mpIfaceUpOrDownSignal, NetworkEvent::QDS_EV_STATE_CHANGED, &mRegObj));

   return AEE_SUCCESS;
}

//===================================================================
//  FUNCTION:   DSSNetApp::GetIPv4Addr
//
//  DESCRIPTION:
//  Serves DSS_IFACE_IOCTL_GET_IPV4_ADDR
//===================================================================

AEEResult DSSNetApp::GetIPv4Addr(dss_iface_ioctl_ipv4_addr_type* pIpv4Addr)
{
   if (NULL == mpIDSNetworkPriv) {
      return QDS_EINVAL;
   }

   ds::IPAddrType ipAddr;
   IDS_ERR_RET(mpIDSNetwork->GetIPAddr(&ipAddr));

   if (ds::AddrFamily::QDS_AF_INET != ipAddr.family) {
      return QDS_EINVAL;
   }

   return DSSConversion::IDS2DSIpAddr(&ipAddr, pIpv4Addr);
}

//===================================================================
//  FUNCTION:   DSSNetApp::GetIPv6Addr
//
//  DESCRIPTION:
//  Serves DSS_IFACE_IOCTL_GET_IPV6_ADDR
//===================================================================

AEEResult DSSNetApp::GetIPv6Addr(dss_iface_ioctl_ipv6_addr_type* pIpv6Addr)
{
   if (NULL == mpIDSNetworkPriv) {
      return QDS_EINVAL;
   }

   ds::IPAddrType ipAddr;
   IDS_ERR_RET(mpIDSNetwork->GetIPAddr(&ipAddr));

   if (ds::AddrFamily::QDS_AF_INET6 != ipAddr.family) {
      return QDS_EINVAL;
   }

   return DSSConversion::IDS2DSIpAddr(&ipAddr, pIpv6Addr);
}

//===================================================================
//  FUNCTION:   DSSNetApp::GetIPv4PrimDnsAddr
//
//  DESCRIPTION:
//  Serves DSS_IFACE_IOCTL_GET_IPV4_PRIM_DNS_ADDR
//===================================================================

AEEResult DSSNetApp::GetIPv4PrimDnsAddr(dss_iface_ioctl_ipv4_prim_dns_addr_type* pIpv4Addr)
{
   if (NULL == mpIDSNetworkPriv) {
      return QDS_EINVAL;
   }

   ds::IPAddrType ipAddr[PS_IFACE_NUM_DNS_ADDRS] = {{0,{0}}};
   int lenReq = 0;
   int numOfAddrAvailable = 0;
   
   IDS_ERR_RET(mpIDSNetwork->GetDNSAddr(ipAddr, PS_IFACE_NUM_DNS_ADDRS, &lenReq));
   
   numOfAddrAvailable = min(PS_IFACE_NUM_DNS_ADDRS, lenReq);

   if(0 == numOfAddrAvailable){
     return QDS_EFAULT;
   }

   if (ds::AddrFamily::QDS_AF_INET != ipAddr[DSS_DNS_PRIMARY_ADDR_IDX].family) {
      return QDS_EINVAL;
   }

   return DSSConversion::IDS2DSIpAddr(&ipAddr[DSS_DNS_PRIMARY_ADDR_IDX], pIpv4Addr);
}

//===================================================================
//  FUNCTION:   DSSNetApp::GetIPv6PrimDnsAddr
//
//  DESCRIPTION:
//  Serves DSS_IFACE_IOCTL_GET_IPV6_PRIM_DNS_ADDR
//===================================================================

AEEResult DSSNetApp::GetIPv6PrimDnsAddr(dss_iface_ioctl_ipv6_prim_dns_addr_type* pIpv6Addr)
{
   if (NULL == mpIDSNetworkPriv) {
      return QDS_EINVAL;
   }

   ds::IPAddrType ipAddr[PS_IFACE_NUM_DNS_ADDRS] = {{0,{0}}};
   int lenReq = 0;
   int numOfAddrAvailable = 0;

   IDS_ERR_RET(mpIDSNetwork->GetDNSAddr(ipAddr, PS_IFACE_NUM_DNS_ADDRS, &lenReq));

   numOfAddrAvailable = min(PS_IFACE_NUM_DNS_ADDRS, lenReq);

   if(0 == numOfAddrAvailable){
     return QDS_EFAULT;
   }

   if (ds::AddrFamily::QDS_AF_INET6 != ipAddr[DSS_DNS_PRIMARY_ADDR_IDX].family) {
     return QDS_EINVAL;
   }

   return DSSConversion::IDS2DSIpAddr(&ipAddr[DSS_DNS_PRIMARY_ADDR_IDX], pIpv6Addr);
}

//===================================================================
//  FUNCTION:   DSSNetApp::GetIPv4SecoDnsAddr
//
//  DESCRIPTION:
//  Serves DSS_IFACE_IOCTL_GET_IPV4_SECO_DNS_ADDR
//===================================================================

AEEResult DSSNetApp::GetIPv4SecoDnsAddr(dss_iface_ioctl_ipv4_seco_dns_addr_type* pIpv4Addr)
{
   if (NULL == mpIDSNetworkPriv) {
      return QDS_EINVAL;
   }

   ds::IPAddrType ipAddr[PS_IFACE_NUM_DNS_ADDRS] =  {{0,{0}}};
   int lenReq = 0;
   int numOfAddrAvailable = 0;

   IDS_ERR_RET(mpIDSNetwork->GetDNSAddr(ipAddr, PS_IFACE_NUM_DNS_ADDRS, &lenReq));

   numOfAddrAvailable = min(PS_IFACE_NUM_DNS_ADDRS, lenReq);

   if(numOfAddrAvailable < PS_IFACE_NUM_DNS_ADDRS){
     return QDS_EFAULT;
   }

   if (ds::AddrFamily::QDS_AF_INET != ipAddr[DSS_DNS_SECONDARY_ADDR_IDX].family) {
      return QDS_EINVAL;
   }

   return DSSConversion::IDS2DSIpAddr(&ipAddr[DSS_DNS_SECONDARY_ADDR_IDX], pIpv4Addr);
}

//===================================================================
//  FUNCTION:   DSSNetApp::GetIPv6SecoDnsAddr
//
//  DESCRIPTION:
//  Serves DSS_IFACE_IOCTL_GET_IPV6_SECO_DNS_ADDR
//===================================================================

AEEResult DSSNetApp::GetIPv6SecoDnsAddr(dss_iface_ioctl_ipv6_seco_dns_addr_type* pIpv6Addr)
{
   if (NULL == mpIDSNetworkPriv) {
      return QDS_EINVAL;
   }

   ds::IPAddrType ipAddr[PS_IFACE_NUM_DNS_ADDRS] =  {{0,{0}}};
   int lenReq = 0;
   int numOfAddrAvailable = 0;

   IDS_ERR_RET(mpIDSNetwork->GetDNSAddr(ipAddr, PS_IFACE_NUM_DNS_ADDRS, &lenReq));

   numOfAddrAvailable = min(PS_IFACE_NUM_DNS_ADDRS, lenReq);

   if(numOfAddrAvailable < PS_IFACE_NUM_DNS_ADDRS){
     return QDS_EFAULT;
   }

   if (ds::AddrFamily::QDS_AF_INET6 != ipAddr[DSS_DNS_SECONDARY_ADDR_IDX].family) {
     return QDS_EINVAL;
   }

   return DSSConversion::IDS2DSIpAddr(&ipAddr[DSS_DNS_SECONDARY_ADDR_IDX], pIpv6Addr);
}

//===================================================================
//  FUNCTION:   DSSNetApp::GetNatPublicIPAddress
//
//  DESCRIPTION:
//  Serves DSS_IFACE_IOCTL_GET_NAT_PUBLIC_IP_ADDR
//===================================================================

AEEResult DSSNetApp::GetNatPublicIPAddress(dss_iface_ioctl_nat_public_ip_addr_type* pIpAddr)
{
   if (NULL == mpIDSNetworkPriv) {
      return ds::Error::QDS_EINVAL; 
   }

   ds::IPAddrType ipAddr;
   IDS_ERR_RET(mpIDSNetwork->GetNatPublicIPAddress(&ipAddr));

   IDS_ERR_RET(DSSConversion::IDS2DSIpAddr(&ipAddr, pIpAddr));

   return AEE_SUCCESS;
}

//===================================================================
//  FUNCTION:   DSSNetApp::GetAllDnsAddrs
//
//  DESCRIPTION:
//  Serves DSS_IFACE_IOCTL_GET_ALL_DNS_ADDRS
//===================================================================

AEEResult DSSNetApp::GetAllDnsAddrs(dss_iface_ioctl_get_all_dns_addrs_type* pDnsAddrs)
{
   if (NULL == mpIDSNetworkPriv) {
      return QDS_EINVAL;
   }

   ds::IPAddrType ipAddr[PS_IFACE_NUM_DNS_ADDRS] = {{0,{0}}};
   int lenReq = 0;
   int numOfAddrAvailable = 0;
   pDnsAddrs->num_dns_addrs = 0;

   IDS_ERR_RET(mpIDSNetwork->GetDNSAddr(ipAddr, PS_IFACE_NUM_DNS_ADDRS, &lenReq));

   numOfAddrAvailable = min(PS_IFACE_NUM_DNS_ADDRS, lenReq);

   if(0 == numOfAddrAvailable){
     return QDS_EFAULT;
   }

   //convert primary DNS address
   IDS_ERR_RET(DSSConversion::IDS2DSIpAddr(&ipAddr[DSS_DNS_PRIMARY_ADDR_IDX], &pDnsAddrs->dns_addrs_ptr[0]));
   pDnsAddrs->num_dns_addrs++;

   //convert secondary DNS address if available
   if(numOfAddrAvailable > 1){
      IDS_ERR_RET(DSSConversion::IDS2DSIpAddr(&ipAddr[DSS_DNS_SECONDARY_ADDR_IDX], &pDnsAddrs->dns_addrs_ptr[1]));
      pDnsAddrs->num_dns_addrs++;
   }

   return AEE_SUCCESS;
}

//===================================================================
//  FUNCTION:   DSSNetApp::GetMtu
//
//  DESCRIPTION:
//  Serves DSS_IFACE_IOCTL_GET_MTU
//===================================================================

AEEResult DSSNetApp::GetMtu(dss_iface_ioctl_mtu_type* pMtu)
{
   if (NULL == mpIDSNetworkPriv) {
      // TODO: log
      return QDS_EINVAL;
   }

   IDS_ERR_RET(mpIDSNetwork->GetNetMTU((int*)pMtu));

   return AEE_SUCCESS;
}

//===================================================================
//  FUNCTION:   DSSNetApp::GetIPAddress
//
//  DESCRIPTION:
//  Serves DSS_IFACE_IOCTL_GET_MTU
//===================================================================

AEEResult DSSNetApp::GetIPAddress(dss_iface_ioctl_get_ip_addr_type* pIpAddr)
{
   if (NULL == mpIDSNetworkPriv) {
      // TODO: log
      return QDS_EINVAL;
   }

   ds::IPAddrType ipAddr;
   IDS_ERR_RET(mpIDSNetwork->GetIPAddr(&ipAddr));

   IDS_ERR_RET(DSSConversion::IDS2DSIpAddr(&ipAddr, pIpAddr));

   return AEE_SUCCESS;
}

// TODO: code duplication; GetIPAddress and GetPreviousIPAddress are using the same code
//===================================================================
//  FUNCTION:   DSSNetApp::GetPreviousIPAddress
//
//  DESCRIPTION:
//  Used when DSS_IFACE_IOCTL_ADDR_CHANGED_EV is received
//===================================================================

AEEResult DSSNetApp::GetPreviousIPAddress(dss_iface_ioctl_get_ip_addr_type* pIpAddr)
{
   if (NULL == mpIDSNetworkPriv) {
      // TODO: log
      return QDS_EINVAL;
   }

   ds::IPAddrType ipAddr;
   IDS_ERR_RET(mpIDSNetworkPriv->GetPreviousIPAddr(&ipAddr));

   IDS_ERR_RET(DSSConversion::IDS2DSIpAddr(&ipAddr, pIpAddr));

   return AEE_SUCCESS;
}

//===================================================================
//  FUNCTION:   DSSNetApp::GetIfaceState
//
//  DESCRIPTION:
//  Serves DSS_IFACE_IOCTL_GET_STATE
//===================================================================

AEEResult DSSNetApp::GetIfaceState(dss_iface_ioctl_state_type* pIfaceState)
{
   if (NULL == mpIDSNetworkPriv) {
      // TODO: log
      return QDS_EINVAL;
   }

   NetworkStateType netState;
   IDS_ERR_RET(mpIDSNetwork->GetState(&netState));

   IDS_ERR_RET(DSSConversion::IDS2DSIfaceState(netState, pIfaceState));

   return AEE_SUCCESS;
}

AEEResult DSSNetApp::GetBearerTech(dss_iface_ioctl_bearer_tech_type* pBearerTech)
{
   IBearerInfo* piBearerInfo = 0;

   if (NULL == mpIDSNetworkPriv) {
      // TODO: log
      return QDS_EINVAL;
   }

   IDS_ERR_RET(mpIDSNetworkExt->GetBearerInfo(&piBearerInfo));
   AEEResult res = DSSConversion::IDS2DSBearerInfo(piBearerInfo, pBearerTech);
   DSSCommon::ReleaseIf((IQI**)&piBearerInfo);
   return res;
}

// TODO: code duplication (GetBearerTech and GetPreviousBearerTech use the same code)
AEEResult DSSNetApp::GetPreviousBearerTech(dss_iface_ioctl_bearer_tech_type* pBearerTech)
{
   IBearerInfo* piBearerInfo = 0;
   if (NULL == mpIDSNetworkPriv) {
      // TODO: log
      return QDS_EINVAL;
   }

   IDS_ERR_RET(mpIDSNetworkPriv->GetPreviousBearerInfo(&piBearerInfo));
   AEEResult res = DSSConversion::IDS2DSBearerInfo(piBearerInfo, pBearerTech);
   DSSCommon::ReleaseIf((IQI**)&piBearerInfo);
   return res;
}

AEEResult DSSNetApp::GetNetQoSManager(IQoSManager** ppNetQosManager)
{
   ds::Net::IQoSManager* pDSNetQoSManager = NULL;
   if(NULL == mpDSNetQoSManager) {

      // Create a new NetQoSManager since pNetApp does not have
      // a NetQoSManager instance.
      IDS_ERR_RET(mpIDSNetworkExt->CreateQoSManager(&pDSNetQoSManager));

   }

   DSSCritScope cs(*mpCritSect);
   if(NULL == mpDSNetQoSManager) {
     mpDSNetQoSManager = pDSNetQoSManager;
   }
   else{
     DSSCommon::ReleaseIf((IQI**)&pDSNetQoSManager);
   }
   *ppNetQosManager = mpDSNetQoSManager;
   (void) mpDSNetQoSManager->AddRef();
 
   return AEE_SUCCESS;
}

AEEResult DSSNetApp::SetNetQoSManager(IQoSManager* pNetQosManager)
{
   DSSCritScope cs(*mpCritSect);

   mpDSNetQoSManager = pNetQosManager;
   (void)mpDSNetQoSManager->AddRef();

   return AEE_SUCCESS;
}

AEEResult DSSNetApp::SetQoSDefault(IQoSDefault* pNetQoSDefault)
{
   if (NULL != mpDSSNetQoSDefault) {
      // do nothing if QosDefault already exists
      return AEE_SUCCESS;
   }
   // AddRef is done inside
   mpDSSNetQoSDefault = PS_MEM_NEW(DSSNetQoSDefault(pNetQoSDefault));
   if (NULL == mpDSSNetQoSDefault) {
      LOG_MSG_ERROR( "Couldn't allocate DSSNetQoSDefault", 0, 0, 0);
      return AEE_ENOMEMORY;
   }
   return AEE_SUCCESS;
}

AEEResult DSSNetApp::GetQoSDefault(IQoSDefault** ppNetQoSDefault)
{
   if(NULL == mpDSSNetQoSDefault) {

      // Create a new DSSNetQosDefault since pNetApp does not have
      // a DSSNetQosDefault instance.
      IQoSDefault* pNetQoSDefault = NULL;
      IQoSManager* pNetQoSManager = NULL;

      IDS_ERR_RET(GetNetQoSManager(&pNetQoSManager));
      DSSGenScope scopeNetQosManager(pNetQoSManager,DSSGenScope::IDSIQI_TYPE);

      // Get the primary QoS
      IDS_ERR_RET(pNetQoSManager->GetQosDefault(&pNetQoSDefault));
      DSSGenScope scopeNetQosDefault(pNetQoSDefault,DSSGenScope::IDSIQI_TYPE);

      // Set the primary QoS in DSSNetApp (set the mpDSSNetQoSDefault)
      IDS_ERR_RET(SetQoSDefault(pNetQoSDefault));
   }

   return mpDSSNetQoSDefault->GetNetQoSDefault(ppNetQoSDefault);
}

AEEResult DSSNetApp::AddNetQoSSecondary(IQoSSecondary* pNetQoSSecondary,
                                     uint32 flowID,
                                     dss_iface_ioctl_event_cb cback_fn,
                                     void* user_data)
{
   dss_iface_ioctl_ev_cb_type cbType;
   AEEResult res;

   // allocate a new DSSQoS object and store the new pNetQoSSecondary object in it
   // mpNetQoSSecondary->AddRef() is done inside
   DSSNetQoSSecondary* pDSSNetQoSSecondary = PS_MEM_NEW(DSSNetQoSSecondary(pNetQoSSecondary, flowID));
   if (NULL == pDSSNetQoSSecondary) {
      LOG_MSG_ERROR( "Couldn't allocate DSSNetQoSSecondary", 0, 0, 0);
      return AEE_ENOMEMORY;
   }

   cbType.event_cb = cback_fn;
   cbType.user_data_ptr = user_data;

   // put the new DSSQoS at the beginning of the DSSQoS list.
   pDSSNetQoSSecondary->SetDSSNetApp(this);
   pDSSNetQoSSecondary->InsertToList(mpDSSQoSSecondaryList);
   mpDSSQoSSecondaryList = pDSSNetQoSSecondary;

   // registering DSS_IFACE_IOCTL_QOS_MODIFY_ACCEPTED_EV will cause to register also:
   // DSS_IFACE_IOCTL_QOS_MODIFY_REJECTED_EV
   cbType.event = DSS_IFACE_IOCTL_QOS_MODIFY_ACCEPTED_EV;
   res = pDSSNetQoSSecondary->RegEventCB(&cbType);
   if (AEE_SUCCESS != res) {
      goto bail;
   }

   cbType.event = DSS_IFACE_IOCTL_QOS_INFO_CODE_UPDATED_EV;
   res = pDSSNetQoSSecondary->RegEventCB(&cbType);
   if (AEE_SUCCESS != res) {
      goto bail;
   }

   // registering DSSIFACE_IOCTL_QOS_AVAILABLE_MODIFIED_EV will cause to register also:
   // DSSIFACE_IOCTL_QOS_AVAILABLE_DEACTIVATED_EV
   // DSSIFACE_IOCTL_QOS_UNAVAILABLE_EV
   cbType.event = DSS_IFACE_IOCTL_QOS_AVAILABLE_MODIFIED_EV;
   res = pDSSNetQoSSecondary->RegEventCB(&cbType);
   if (AEE_SUCCESS != res) {
      goto bail;
   }

   return AEE_SUCCESS;

bail:
   res = RemoveDSSNetQoSSecondary(flowID);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("RemoveDSSNetQoSSecondary() failed: %d", res, 0, 0);
   }

   delete pDSSNetQoSSecondary;
   return res;
}

AEEResult DSSNetApp::AddIpv6PrivAddr(IIPv6Address* pNetIpv6Address,
                                     uint32 flowID,
                                     boolean isUnique,
                                     dss_iface_ioctl_event_cb cback_fn,
                                     void* user_data)
{

   dss_iface_ioctl_ev_cb_type cbType;
   AEEResult res;

   // allocate a new DSS Private IPv6 address object and store the new IIPv6Address object in it
   // AddRef() is inside
   DSSPrivIpv6Addr* pDSSPrivIpv6Addr = PS_MEM_NEW(DSSPrivIpv6Addr(pNetIpv6Address,flowID,isUnique));
   if (NULL == pDSSPrivIpv6Addr) {
      LOG_MSG_ERROR( "Couldn't allocate pDSSPrivIpv6Addr", 0, 0, 0);
      return AEE_ENOMEMORY;
   }

   cbType.event_cb = cback_fn;
   cbType.user_data_ptr = user_data;

   // registering DSS_IFACE_IOCTL_IPV6_PRIV_ADDR_GENERATED_EV will cause to register also:
   // DSS_IFACE_IOCTL_IPV6_PRIV_ADDR_DELETED_EV
   // DSS_IFACE_IOCTL_IPV6_PRIV_ADDR_DEPRECATED_EV
   cbType.event = DSS_IFACE_IOCTL_IPV6_PRIV_ADDR_GENERATED_EV;
   res = pDSSPrivIpv6Addr->RegEventCB(&cbType);
   if (AEE_SUCCESS != res) {
      goto bail;
   }

   // put the new IPv6 private address in the IPv6 private addresses list.
   pDSSPrivIpv6Addr->InsertToList(mpDSSPrivIpv6AddrList);
   pDSSPrivIpv6Addr->SetDSSNetApp(this);
   mpDSSPrivIpv6AddrList = pDSSPrivIpv6Addr;

   return AEE_SUCCESS;

bail:
   PS_MEM_RELEASE(pDSSPrivIpv6Addr);
   return res;

}

#ifndef FEATURE_DSS_LINUX
AEEResult DSSNetApp::AddDSSMCast(IMCastSessionPriv* pMCastSession, uint32 MCastHandle, dss_iface_ioctl_event_cb event_cb, void *user_data_ptr)
{
   // allocate a new DSSMCast object and store the new pMCastSession object in it
   // AddRef is done inside
   DSSMCast* pDSSMCast = PS_MEM_NEW(DSSMCast(pMCastSession, MCastHandle));
   if (NULL == pDSSMCast) {
      LOG_MSG_ERROR( "Couldn't allocate DSSMCast", 0, 0, 0);
      return AEE_ENOMEMORY;
   }

   AEEResult res = pDSSMCast->RegEventCB(event_cb, user_data_ptr, DSS_IFACE_IOCTL_MCAST_REGISTER_SUCCESS_EV);
   if(AEE_SUCCESS != res) {
      goto bail;
   }

   res = pDSSMCast->RegEventCB(event_cb, user_data_ptr, DSS_IFACE_IOCTL_MCAST_STATUS_EV);
   if(AEE_SUCCESS != res) {
      goto bail;
   }

   // put the new DSSMCast at the beginning of the DSSMCast list.
   pDSSMCast->InsertToList(mpDSSMCastList);
   pDSSMCast->SetDSSNetApp(this);
   mpDSSMCastList = pDSSMCast;

   return AEE_SUCCESS;
bail:

   PS_MEM_RELEASE(pDSSMCast);
   return res;
}
#endif
AEEResult DSSNetApp::AddDSSFirewallRule(IFirewallRule* pIFirewallRule, uint32 *pHandle)
{
   int index;
   static uint32 firewallHandle = 1;
   DSSCritScope cs(*mpCritSect);

   ASSERT (NULL != pIFirewallRule);
   ASSERT (NULL != pHandle);

   for (index = 0; index < DSS_MAX_FIREWALL_HANDLES; index++) {
      
      if (0 == DSSFirewallHandleToObject[index].handle) {

         DSSFirewallHandleToObject[index].handle = firewallHandle;
         DSSFirewallHandleToObject[index].pIFirewallRule = pIFirewallRule;
         (void)DSSFirewallHandleToObject[index].pIFirewallRule->AddRef();
         *pHandle = firewallHandle;
         break;
      }
   }

   firewallHandle++;

   if (index == DSS_MAX_FIREWALL_HANDLES) {
      return AEE_ENOMEMORY;
   }

   return AEE_SUCCESS;
}


AEEResult DSSNetApp::GetDSSFirewallRule(uint32 firewallHandle, IFirewallRule** ppIFirewallRule)
{
   int index;
   DSSCritScope cs(*mpCritSect);

   ASSERT (NULL != ppIFirewallRule);
   *ppIFirewallRule = 0;

   for (index = 0; index < DSS_MAX_FIREWALL_HANDLES; index++) {

      if (firewallHandle == DSSFirewallHandleToObject[index].handle) {
         *ppIFirewallRule = DSSFirewallHandleToObject[index].pIFirewallRule;
         (void)DSSFirewallHandleToObject[index].pIFirewallRule->AddRef();
         break;
      }
   }

   if (index == DSS_MAX_FIREWALL_HANDLES) {
      return QDS_EBADF;
   }

   return AEE_SUCCESS;
}

AEEResult DSSNetApp::DeleteDSSFirewallRule(uint32 firewallHandle)
{
   int index;
   DSSCritScope cs(*mpCritSect);

   for (index = 0; index < DSS_MAX_FIREWALL_HANDLES; index++) {

      if (firewallHandle == DSSFirewallHandleToObject[index].handle) {
         DSSFirewallHandleToObject[index].handle = 0;
         DSSCommon::ReleaseIf ((IQI **) (&DSSFirewallHandleToObject[index].pIFirewallRule));
         break;
      }
   }

   if (index == DSS_MAX_FIREWALL_HANDLES) {
      return QDS_EBADF;
   }

   return AEE_SUCCESS;
}

AEEResult DSSNetApp::FlushDSSFirewallRuleTable(void)
{
  int index;
  DSSCritScope cs(*mpCritSect);

  for (index = 0; index < DSS_MAX_FIREWALL_HANDLES; index++) {
      DSSFirewallHandleToObject[index].handle = 0;
      DSSCommon::ReleaseIf ((IQI **) (&DSSFirewallHandleToObject[index].pIFirewallRule));
  }

  return AEE_SUCCESS;
}

#ifndef FEATURE_DSS_LINUX
AEEResult DSSNetApp::AddDSSMCastMBMSCtrl(IMCastMBMSCtrlPriv* pMCastMBMSCtrl, uint32 MCastMBMSHandle, dss_iface_ioctl_event_cb event_cb, void *user_data_ptr)
{
   // allocate a new DSSMCast object and store the new pMCastSession object in it
   // AddRef is done inside
   DSSNetMCastMBMSCtrl* pDSSNetMCastMBMSCtrl = PS_MEM_NEW(DSSNetMCastMBMSCtrl(pMCastMBMSCtrl, MCastMBMSHandle));
   if (NULL == pDSSNetMCastMBMSCtrl) {
      LOG_MSG_ERROR( "Couldn't allocate DSSNetMCastMBMSCtrl", 0, 0, 0);
      return AEE_ENOMEMORY;
   }


   AEEResult res = pDSSNetMCastMBMSCtrl->RegEventCB(event_cb, user_data_ptr);
   if(AEE_SUCCESS != res) {
      goto bail;
   }

   // put the new DSSNetMCastMBMSCtrl at the beginning of the DSSNetMCastMBMSCtrl list.
   pDSSNetMCastMBMSCtrl->InsertToList(mpDSSNetMCastMBMSList);
   pDSSNetMCastMBMSCtrl->SetDSSNetApp(this);
   mpDSSNetMCastMBMSList = pDSSNetMCastMBMSCtrl;

   return AEE_SUCCESS;

bail:
   PS_MEM_RELEASE(pDSSNetMCastMBMSCtrl);
   return res;
}

AEEResult DSSNetApp::GetMCastMBMSCtrl(uint32 iface_id, IMCastMBMSCtrlPriv** ppDSSMCastMBMSCtrl)
{
   DSSNetMCastMBMSCtrl* pDSSMCastMBMS;
   pDSSMCastMBMS = mpDSSNetMCastMBMSList;
   uint32 tmpMCastMBMSHandle = 0;

   while(NULL != pDSSMCastMBMS)
   {
      // get the Flow id of the current node in the DSSMCast list
      pDSSMCastMBMS->GetMCastMBMSHandle(&tmpMCastMBMSHandle);

      // if the flow ID matches the QoSHandle the used provided then we found the
      // DSSMCast
      if (tmpMCastMBMSHandle == iface_id)
      {
         IDS_ERR_RET(pDSSMCastMBMS->GetMBMSCtrl(ppDSSMCastMBMSCtrl));
         return AEE_SUCCESS;
      }

      IDS_ERR_RET(pDSSMCastMBMS->GetNext(&pDSSMCastMBMS));
   }

   // We haven't found any DSSMCast that matches the provided QoS Handle
   *ppDSSMCastMBMSCtrl = NULL;
   return QDS_EFAULT;
}

AEEResult DSSNetApp::GetDSSMCast(uint32 iface_id, DSSMCast** ppDSSMCast)
{
   DSSMCast* pDSSMCast = mpDSSMCastList;
   uint32 tmpMCastHandle = 0;

   while(NULL != pDSSMCast)
   {
      // get the Flow id of the current node in the DSSMCast list
      pDSSMCast->GetMCastHandle(&tmpMCastHandle);

      // if the flow ID matches the QoSHandle the used provided then we found the
      // DSSMCast
      if (tmpMCastHandle == iface_id)
      {
         *ppDSSMCast = pDSSMCast;
         return AEE_SUCCESS;
      }

      IDS_ERR_RET(pDSSMCast->GetNext(&pDSSMCast));
   }

   // We haven't found any DSSMCast that matches the provided QoS Handle
   *ppDSSMCast = NULL;
   return QDS_EBADF;
}
#endif

FlowID DSSNetApp::ExtractFlowIDFromIFaceID(uint32 ifaceid)
{
   return static_cast<FlowID>(ifaceid & 0xff);
}

AEEResult DSSNetApp::GetPhysLinkObject(uint32 ifaceid, IPhysLink** ppPhyslink)
{
   FlowID flowid                        = 0;
   DSSNetQoSSecondary *pDSSQosSecondary = NULL;
   IQoSSecondary      *pIDSQosSecondary = NULL;
   AEEResult res                        = AEE_SUCCESS;
   DSSIDSNetworkScope IDSNetworkScope;

   flowid = ExtractFlowIDFromIFaceID(ifaceid);

   if (0 == flowid) { // No flow id defined
      res = IDSNetworkScope.Init(this);
      if (AEE_SUCCESS != res) {
         goto bail;
      }

      res = IDSNetworkScope.Fetch()->GetTXPhysLink(ppPhyslink);
      if (AEE_SUCCESS != res) {
         LOG_MSG_INFO1("GetPhysLinkObject - physLink cannot be created", 0, 0, 0);
         res = QDS_ENETDOWN;
         goto bail;
      }
   } else {           // Flow id is real - extract corresponding qos obj
      res = GetDSSNetQoSSecondary(ifaceid, &pDSSQosSecondary);
      if ((AEE_SUCCESS != res) || (NULL == pDSSQosSecondary)){
         goto bail;
      }
      res = pDSSQosSecondary->GetNetQoSSecondary(&pIDSQosSecondary);
      DSSGenScope GenScopeObject(pIDSQosSecondary, DSSGenScope::IDSIQI_TYPE);
      if (AEE_SUCCESS != res) {
         goto bail;
      }
      res = pIDSQosSecondary->GetTXPhysLink(ppPhyslink);
      if (AEE_SUCCESS != res) {
         goto bail;
      }
   }

bail:
   if (AEE_SUCCESS != res) {
      *ppPhyslink = NULL;
   }
   return res;
}

AEEResult DSSNetApp::GetDSSNetQoSSecondary(uint32 iface_id, DSSNetQoSSecondary** ppDSSNetQoSSecondary)
{
   DSSNetQoSSecondary* pDSSNetQoSSecondary = mpDSSQoSSecondaryList;
   uint32 tmpFlowID = 0;

   while(NULL != pDSSNetQoSSecondary)
   {
      // get the Flow id of the current node in the DSSNetQoSSecondary list
      pDSSNetQoSSecondary->GetFlowID(&tmpFlowID);

      // if the flow ID matches the QoSHandle the used provided then we found the
      // DSSNetQoSSecondary
      if (tmpFlowID == iface_id)
      {
         *ppDSSNetQoSSecondary = pDSSNetQoSSecondary;
         return AEE_SUCCESS;
      }

      IDS_ERR_RET(pDSSNetQoSSecondary->GetNext(&pDSSNetQoSSecondary));
   }

   // We haven't found any DSSNetQoSSecondary that matches the provided QoS Handle
   *ppDSSNetQoSSecondary = NULL;
   // The error value here sould be QDS_EBADAPP but to be backward compatible
   // with the legacy API we return QDS_EBADF
   return QDS_EBADF;
}

AEEResult DSSNetApp::RemoveDSSNetQoSSecondary(uint32 iface_id)
{
   DSSNetQoSSecondary *pDSSNetQoSSecondary = mpDSSQoSSecondaryList, *ptmpDSSNetQoSSecondary = mpDSSQoSSecondaryList;
   uint32 tmpFlowID = 0;

   // if we have an empty list
   if (NULL == pDSSNetQoSSecondary) {
      return QDS_EINVAL;
   }

   while(NULL != pDSSNetQoSSecondary)
   {
      // get the Flow id of the current node in the DSSNetQoSSecondary list
      pDSSNetQoSSecondary->GetFlowID(&tmpFlowID);

      // if the flow ID matches the QoSHandle the used provided then we found the
      // DSSNetQoSSecondary that we need to remove
      if (tmpFlowID == iface_id)
      {
         // if this is the first node in the list
         if (ptmpDSSNetQoSSecondary == pDSSNetQoSSecondary)
         {
            IDS_ERR_RET(pDSSNetQoSSecondary->GetNext(&mpDSSQoSSecondaryList));
            PS_MEM_RELEASE(pDSSNetQoSSecondary);
            return AEE_SUCCESS;
         }

         // Get the node after the node that is to be removed
         DSSNetQoSSecondary* pNextDSSQoS = NULL;
         IDS_ERR_RET(pDSSNetQoSSecondary->GetNext(&pNextDSSQoS));

         // Connect the two parts of the list (before and after the removed node).
         ptmpDSSNetQoSSecondary->InsertToList(pNextDSSQoS);

         // delete the DSSNetQoSSecondary instance.
         PS_MEM_RELEASE(pDSSNetQoSSecondary);

         return AEE_SUCCESS;
      }

      // Initialize pointers for next iteration
      ptmpDSSNetQoSSecondary = pDSSNetQoSSecondary;
      IDS_ERR_RET(pDSSNetQoSSecondary->GetNext(&pDSSNetQoSSecondary));
   }

   // We haven't found any DSSNetQoSSecondary that matches the provided QoS Handle
   return QDS_EINVAL;
}

AEEResult DSSNetApp::RemoveDSSPrivIpv6Addr(IIPv6Address *pIDSNetIpv6Address)
{
   DSSPrivIpv6Addr *pDSSPrivIpv6Addr = mpDSSPrivIpv6AddrList, *ptmpDSSPrivIpv6Addr = mpDSSPrivIpv6AddrList;
   IIPv6Address         *tmpIvp6Addr =  NULL;

   // if we have an empty list
   if (NULL == mpDSSPrivIpv6AddrList) {
      return QDS_EINVAL;
   }

   while(NULL != pDSSPrivIpv6Addr)
   {
      // get the IDSNetIpv6Address of the current node in the DSSPrivIpv6Addr list
      DSSCommon::ReleaseIf(reinterpret_cast<IQI**>(&tmpIvp6Addr));
      IDS_ERR_RET(pDSSPrivIpv6Addr->GetIDSNetIpv6Address(&tmpIvp6Addr));

      // if the IDSNetIpv6Address matches the IDSNetIpv6Address the used provided then we found the
      // DSSPrivIpv6Addr that we need to remove
      if (tmpIvp6Addr == pIDSNetIpv6Address)
      {
         // if this is the first node in the list
         if (ptmpDSSPrivIpv6Addr == pDSSPrivIpv6Addr)
         {
            IDS_ERR_RET(pDSSPrivIpv6Addr->GetNext(&mpDSSPrivIpv6AddrList));
            PS_MEM_RELEASE(pDSSPrivIpv6Addr);
            DSSCommon::ReleaseIf((IQI**)&tmpIvp6Addr);
            return AEE_SUCCESS;
         }

         // Get the node after the node that is to be removed
         DSSPrivIpv6Addr* pNextDSSPrivIpv6 = NULL;
         IDS_ERR_RET(pDSSPrivIpv6Addr->GetNext(&pNextDSSPrivIpv6));

         // Connect the two parts of the list (before and after the removed node).
         ptmpDSSPrivIpv6Addr->InsertToList(pNextDSSPrivIpv6);

         // delete the DSSPrivIpv6Addr instance.
         PS_MEM_RELEASE(pDSSPrivIpv6Addr);
         DSSCommon::ReleaseIf((IQI**)&tmpIvp6Addr);
         return AEE_SUCCESS;
      }

      // Initialize pointers for next iteration
      ptmpDSSPrivIpv6Addr = pDSSPrivIpv6Addr;
      IDS_ERR_RET(pDSSPrivIpv6Addr->GetNext(&pDSSPrivIpv6Addr));
   }

   // We haven't found any DSSNetQoSSecondary that matches the provided QoS Handle
   DSSCommon::ReleaseIf((IQI**)&tmpIvp6Addr);
   return QDS_EINVAL;
}

void DSSNetApp::RemovePrimaryQoSModifyStatusHandler()
{
   PS_MEM_RELEASE(mpPrimaryQoSModifyStatusHandler);
}

#ifndef FEATURE_DSS_LINUX
AEEResult DSSNetApp::RemoveDSSMCast(uint32 iface_id, ds::Net::EventType handlerType)
{
   DSSMCast *pDSSMCast = mpDSSMCastList, *ptmpDSSMCast = mpDSSMCastList;
   uint32 tmpFlowID = 0;

   // if we have an empty list
   if (NULL == pDSSMCast) {
      return QDS_EINVAL;
   }

   while(NULL != pDSSMCast)
   {
      // get the Flow id of the current node in the DSSMCast list
      pDSSMCast->GetMCastHandle(&tmpFlowID);

      // if the flow ID matches the MCastHandle the used provided then we found the
      // DSSMCast that we need to remove
      if (tmpFlowID == iface_id)
      {
         // if this is the first node in the list
         if (ptmpDSSMCast == pDSSMCast)
         {
            IDS_ERR_RET(pDSSMCast->GetNext(&mpDSSMCastList));
            if(TRUE == pDSSMCast->DeregisterHandler(handlerType)){
               PS_MEM_RELEASE(pDSSMCast);
            }
            else/* GetNext moves mpDSSMCastList to NULL */
            {
               mpDSSMCastList = pDSSMCast;
            }
            return AEE_SUCCESS;
         }

         // Get the node after the node that is to be removed
         DSSMCast* pNextDSSMCast = NULL;
         IDS_ERR_RET(pDSSMCast->GetNext(&pNextDSSMCast));

         // delete the DSSMCast instance.
         if(TRUE == pDSSMCast->DeregisterHandler(handlerType)){

           // Connect the two parts of the list (before and after the removed node).
           ptmpDSSMCast->InsertToList(pNextDSSMCast);

           PS_MEM_RELEASE(pDSSMCast);
         }
         else/* GetNext moves pNextDSSMCast to next item */
         {
            pNextDSSMCast = pDSSMCast;
         }

         return AEE_SUCCESS;
      }

      // Initialize pointers for next iteration
      ptmpDSSMCast = pDSSMCast;
      IDS_ERR_RET(pDSSMCast->GetNext(&pDSSMCast));
   }

   // We haven't found any DSSMCast that matches the provided MCast Handle
   return QDS_EINVAL;
}

AEEResult DSSNetApp::RemoveDSSMcastMBMSCtrl(IMCastMBMSCtrlPriv* pIDSNetMCastMBMS)
{
   DSSNetMCastMBMSCtrl *pMCastMBMS = mpDSSNetMCastMBMSList, *pTmpCastMBMS = mpDSSNetMCastMBMSList;
   IMCastMBMSCtrlPriv      *pTmpIDSMCastMBMS = NULL;

   // if we have an empty list
   if (NULL == mpDSSNetMCastMBMSList) {
      return QDS_EINVAL;
   }

   while(NULL != pTmpCastMBMS)
   {
      // get the IDSNetMCastMBMSCtrl of the current node in the DSSNetMCastMBMSCtrl list
      DSSCommon::ReleaseIf(reinterpret_cast<IQI**>(&pTmpIDSMCastMBMS));
      IDS_ERR_RET(pMCastMBMS->GetMBMSCtrl(&pTmpIDSMCastMBMS));

      // if the IDSNetMCastMBMSCtrl matches the IDSNetMCastMBMSCtrl the used provided then we found the
      // DSSNetMCastMBMSCtrl that we need to remove
      if (pTmpIDSMCastMBMS == pIDSNetMCastMBMS)
      {
         // if this is the first node in the list
         if (pTmpCastMBMS == pMCastMBMS)
         {
            IDS_ERR_RET(pMCastMBMS->GetNext(&mpDSSNetMCastMBMSList));
            PS_MEM_RELEASE(pMCastMBMS);
            DSSCommon::ReleaseIf((IQI**)&pTmpIDSMCastMBMS);
            return AEE_SUCCESS;
         }

         // Get the node after the node that is to be removed
         DSSNetMCastMBMSCtrl* pNextMCastMBMS = NULL;
         IDS_ERR_RET(pMCastMBMS->GetNext(&pNextMCastMBMS));

         // Connect the two parts of the list (before and after the removed node).
         pTmpCastMBMS->InsertToList(pNextMCastMBMS);

         // delete the DSSPrivIpv6Addr instance.
         PS_MEM_RELEASE(pMCastMBMS);
         DSSCommon::ReleaseIf((IQI**)&pTmpIDSMCastMBMS);
         return AEE_SUCCESS;
      }

      // Initialize pointers for next iteration
      pTmpCastMBMS = pMCastMBMS;
      IDS_ERR_RET(pMCastMBMS->GetNext(&pMCastMBMS));
   }

   // We haven't found any DSSNetQoSSecondary that matches the provided QoS Handle
   DSSCommon::ReleaseIf((IQI**)&pTmpIDSMCastMBMS);
   return QDS_EINVAL;
}

AEEResult DSSNetApp::GetNetMCastManager(IMCastManager** ppNetMCastManager)
{
   if(NULL != mpDSNetMCastManager)
   {
      *ppNetMCastManager = mpDSNetMCastManager;
      (void)mpDSNetMCastManager->AddRef();
      return AEE_SUCCESS;
   }

   // Create a new NetQoSManager since pNetApp does not have
   // a NetQoSManager instance.
   IDS_ERR_RET(mpIDSNetworkExt->CreateMCastManager(ppNetMCastManager));
   // Add the new created pNetQoSManager to pNetApp.
   if (AEE_SUCCESS != SetNetMCastManager(*ppNetMCastManager)) {
      DSSCommon::ReleaseIf((IQI**)ppNetMCastManager);
   }

   return AEE_SUCCESS;
}

AEEResult DSSNetApp::SetNetMCastManager(IMCastManager* pNetMCastManager)
{
   mpDSNetMCastManager = pNetMCastManager;
   (void)mpDSNetMCastManager->AddRef();

   return AEE_SUCCESS;
}
#endif

AEEResult DSSNetApp::GetRFConditions(dss_iface_ioctl_rf_conditions_type* pRFConds)
{
   RFConditionType rfCondition;

   IDS_ERR_RET(GetBearerTech(&pRFConds->bearer_tech));
   IDS_ERR_RET(mpIDSNetwork->GetCurrRFCondition(&rfCondition));
   IDS_ERR_RET(DSSConversion::IDS2DSRFCondition(rfCondition, &pRFConds->rf_conditions));
   return AEE_SUCCESS;
}

AEEResult DSSNetApp::GetNetFirewallManager(IFirewallManager** ppNetFirewallManager)
{
   if(NULL != mpDSNetFirewallManager)
   {
      *ppNetFirewallManager = mpDSNetFirewallManager;
      (void)mpDSNetFirewallManager->AddRef();
      return AEE_SUCCESS;
   }

   // Create a new NetFirewallManager since pNetApp does not have 
   // a NetFirewallManager instance.  
   IDS_ERR_RET(mpIDSNetworkExt2->CreateNetFirewallManager(ppNetFirewallManager));
   // Add the new created pNetFirewallManager to pNetApp.
   if (AEE_SUCCESS != SetNetFirewallManager(*ppNetFirewallManager)) {
      DSSCommon::ReleaseIf((IQI**)ppNetFirewallManager);
   }

   return AEE_SUCCESS;
}

AEEResult DSSNetApp::SetNetFirewallManager(IFirewallManager* pNetFirewallManager)
{
   mpDSNetFirewallManager = pNetFirewallManager;
   (void)mpDSNetFirewallManager->AddRef();

   return AEE_SUCCESS;
}

AEEResult DSSNetApp::GetNetNatSession(INatSession** ppNetNatSession)
{
   if(NULL != mpDSNetNatSession)
   {
      *ppNetNatSession = mpDSNetNatSession;
      (void)mpDSNetNatSession->AddRef();
      return AEE_SUCCESS;
   }

   // Create a new NetNatSession since pNetApp does not have 
   // a NetNatSession instance.  
   IDS_ERR_RET(mpIDSNetworkExt2->CreateNetNatSession(ppNetNatSession));
   // Add the new created pNetNatSession to pNetApp.
   if (AEE_SUCCESS != SetNetNatSession(*ppNetNatSession)) {
      DSSCommon::ReleaseIf((IQI**)ppNetNatSession);
   }

   return AEE_SUCCESS;
}

AEEResult DSSNetApp::SetNetNatSession(INatSession* pNetNatSession)
{
   mpDSNetNatSession = pNetNatSession;
   (void)mpDSNetNatSession->AddRef();

   return AEE_SUCCESS;
}


//===================================================================
//  FUNCTION:    DSSNetApp::RegEventCB
//
//  DESCRIPTION: Serves DSS_IFACE_IOCTL_REG_EVENT_CB
//===================================================================
AEEResult DSSNetApp::RegEventCB(dss_iface_ioctl_ev_cb_type* pEvArg)
{
   //TODO: implement
   // outline: ???
   // (1)(a) create a network, if necessary.
   // (1)(b) find the right DSSEventHandler instance

   // Done in DSSEventHandler:
   // (2) fetch the right interface for the event.
   // (3) register for the event using a single ISignal (per event "group").
   // (4) save the user's callback, it's just a CB/signal pair because only one CB is allowed per app.
   // (5) when the event comes, the handler dispatches the callback with the needed data.
   return RegEventCB(pEvArg->event,pEvArg->event_cb,pEvArg->user_data_ptr);
}

//===================================================================
//  FUNCTION:    DSSNetApp::RegEventCB
//
//  DESCRIPTION: Serves DSS_IFACE_IOCTL_REG_EVENT_CB and other IOCTLs
//===================================================================
AEEResult DSSNetApp::RegEventCB(dss_iface_ioctl_event_enum_type event, dss_iface_ioctl_event_cb event_cb, void* user_data_ptr)
{
   DSSEventHandler* ppEventHandler = 0;
   sint15 netHandle;

   LOG_MSG_FUNCTION_ENTRY("NetHandle :%d , Event :%d ", mNetHandle, event, 0);

   AEEResult res = GetEventHandler(event, &ppEventHandler, true);
   if (AEE_SUCCESS != res) {
      GetNetHandle(&netHandle);
      LOG_MSG_ERROR("Error while registering app %d for event %d",netHandle, event,0);
      return res;
   }

   res = ppEventHandler->Register(event, event_cb, user_data_ptr);
   if (AEE_SUCCESS != res) {
      GetNetHandle(&netHandle);
      if (QDS_EINPROGRESS == res) {
         LOG_MSG_ERROR("App %d already has registered for event %d",netHandle, event,0);
         res = QDS_EFAULT;
      } else {
         LOG_MSG_ERROR("Error while registering app %d for event %d",netHandle,event,0);
      }
      return res;
   }

   return AEE_SUCCESS;
}

//===================================================================
//  FUNCTION:    DSSNetApp::RegEventCBPrimary
//
//  DESCRIPTION: Serves DSS_IFACE_IOCTL_REG_EVENT_CB
//===================================================================
AEEResult DSSNetApp::RegEventCBPrimary(dss_iface_ioctl_event_enum_type event, dss_iface_ioctl_event_cb event_cb, void* user_data_ptr)
{
   DSSEventHandler* pEventHandler = 0;
   sint15 netHandle;

#ifndef FEATURE_DSS_LINUX
   if (DSS_IFACE_IOCTL_QOS_MODIFY_ACCEPTED_EV == event || DSS_IFACE_IOCTL_QOS_MODIFY_REJECTED_EV == event) {
      IDS_ERR_RET(FetchHandler(&mpPrimaryQoSModifyStatusHandler, &pEventHandler, true));
   } else {
      LOG_MSG_ERROR("RegEventCBPrimary: trying to register for unknown event %d",event, 0,0);
      ASSERT(0);
      return QDS_INTERNAL;
   }
#endif
   // Register to the event.
   AEEResult res = (pEventHandler->Register(event, event_cb, user_data_ptr));
   if (AEE_SUCCESS != res) {
      GetNetHandle(&netHandle);
      if (QDS_EINPROGRESS == res) {
         LOG_MSG_ERROR("App %d already has registered for event %d",netHandle, event,0);
      } else {
         LOG_MSG_ERROR("Error while registering app %d for event %d",netHandle,event,0);
      }
      return res;
   }

   return AEE_SUCCESS;
}

//===================================================================
//  FUNCTION:    DSSNetApp::DeregEventCB
//
//  DESCRIPTION: Serves DSS_IFACE_IOCTL_DEREG_EVENT_CB
//===================================================================
AEEResult DSSNetApp::DeregEventCB(dss_iface_ioctl_ev_cb_type* pEvArg)
{
   return DeregEventCB(pEvArg->event);
}

//===================================================================
//  FUNCTION:    DSSNetApp::DeregEventCB
//
//  DESCRIPTION: Serves DSS_IFACE_IOCTL_DEREG_EVENT_CB
//===================================================================
AEEResult DSSNetApp::DeregEventCB(dss_iface_ioctl_event_enum_type event)
{
   DSSEventHandler* ppEventHandler = 0;
   sint15 netHandle;

   LOG_MSG_FUNCTION_ENTRY("NetHandle :%d , Event :%d ", mNetHandle, event, 0);

   AEEResult res = GetEventHandler(event, &ppEventHandler, false);
   if (AEE_SUCCESS != res) {
      GetNetHandle(&netHandle);
      LOG_MSG_ERROR("App %d couldn't deregister for event %d",netHandle, event,0);
      return res;
   }

   // De-Register to the event.
   if (NULL != ppEventHandler) {
      res = ppEventHandler->DeRegister(event);
      if (AEE_SUCCESS != res) {
         GetNetHandle(&netHandle);
         if (QDS_ENOTCONN == res) {
            LOG_MSG_ERROR("App %d already is not registered for event %d",netHandle, event,0);
            res = QDS_EFAULT;
         } else {
            LOG_MSG_ERROR("Error while deregistering app %d for event %d",netHandle, event,0);
         }
         return res;
      }
   }

   return AEE_SUCCESS;
}
//===================================================================
//  FUNCTION:    DSSNetApp::RegMTPDEventCB
//
//  DESCRIPTION: Serves DSS_IFACE_IOCTL_MT_REG_CB
//===================================================================
AEEResult DSSNetApp::RegMTPDEventCB(dss_iface_ioctl_mt_reg_cb_type* pEvArg)
{
   // Get the MTPD event handler
   DSSEventHandler* ppEventHandler = 0;
   sint15 netHandle;

   AEEResult res = GetEventHandler(DSS_IFACE_IOCTL_MT_REQUEST_EV, &ppEventHandler, true);
   if (AEE_SUCCESS != res) {
      GetNetHandle(&netHandle);
      LOG_MSG_ERROR("Error while registering app %d for event %d",netHandle, DSS_IFACE_IOCTL_MT_REQUEST_EV,0);
      return res;
   }

   // The pointer will be the MTPD handle that is returned to the caller.
   pEvArg->handle = (dss_iface_ioctl_mt_handle_type*)ppEventHandler;

   // Register to the event.
   res = ppEventHandler->Register(DSS_IFACE_IOCTL_MT_REQUEST_EV, pEvArg->event_cb, pEvArg->user_data_ptr);
   if (AEE_SUCCESS != res) {
      GetNetHandle(&netHandle);
      if (QDS_EINPROGRESS == res) {
         LOG_MSG_ERROR("App %d already has registered for event %d",netHandle, DSS_IFACE_IOCTL_MT_REQUEST_EV,0);
         res = QDS_EFAULT;
      } else {
         LOG_MSG_ERROR("Error while registering app %d for event %d",netHandle,DSS_IFACE_IOCTL_MT_REQUEST_EV,0);
      }
      return res;
   }

   return AEE_SUCCESS;
}

//===================================================================
//  FUNCTION:    DSSNetApp::DeRegMTPDEventCB
//
//  DESCRIPTION: DeRegister DSS_IFACE_IOCTL_MT_REG_CB
//===================================================================
AEEResult DSSNetApp::DeRegMTPDEventCB(dss_iface_ioctl_mt_dereg_cb_type* pEvArg)
{
   if (NULL == mpMTPDRequestHandler) {
      return DS_EFAULT;
   }

   // Check if this is the same handle that was already allocated.
   if (pEvArg->handle != (dss_iface_ioctl_mt_handle_type)mpMTPDRequestHandler) {
      return DS_EFAULT;
   }

   PS_MEM_RELEASE(mpMTPDRequestHandler);

   return AEE_SUCCESS;
}
template<typename HandlerType>
inline AEEResult DSSNetApp::FetchHandler(HandlerType** pHandler, DSSEventHandler** ppEventHandler, bool bInit)
{
   if (bInit) {
      if (NULL == *pHandler) {
         *pHandler = HandlerType::CreateInstance();
         if (NULL == *pHandler) {
            /* LOG_MSG_ERROR( "Couldn't allocate HandlerType", 0, 0, 0);*/
            return AEE_ENOMEMORY;
         }
         IDS_ERR_RET((*pHandler)->Init(this));
      }
   } else {
      // bInit is false , we expect pHandler to have a value here
      if (0 == *pHandler) {
         return QDS_EFAULT;
      }
   }
   *ppEventHandler = *pHandler;
   return AEE_SUCCESS;
}

//===================================================================
//  FUNCTION:    DSSNetApp::GetEventHandler
//
//  DESCRIPTION: Returns the DSSEventHandler for the specified event.
//               If bInit is true, this function will also initialize
//               the handler if it's not initialized.
//===================================================================
AEEResult DSSNetApp::GetEventHandler(dss_iface_ioctl_event_enum_type event,
                                DSSEventHandler** ppEventHandler, bool bInit)
{
   switch (event) {
      case DSS_IFACE_IOCTL_DOWN_EV:
      case DSS_IFACE_IOCTL_UP_EV:
      case DSS_IFACE_IOCTL_COMING_UP_EV:
      case DSS_IFACE_IOCTL_GOING_DOWN_EV:
         return FetchHandler(&mpNetStateHandler, ppEventHandler, bInit);
      case DSS_IFACE_IOCTL_ADDR_CHANGED_EV:
         return FetchHandler(&mpNetIPHandler, ppEventHandler, bInit);
      case DSS_IFACE_IOCTL_EXTENDED_IP_CONFIG_EV:
         return FetchHandler(&mpExtendedIPConfigHandler, ppEventHandler, bInit);
      case DSS_IFACE_IOCTL_RF_CONDITIONS_CHANGED_EV:
         return FetchHandler(&mpRFConditionsHandler, ppEventHandler, bInit);
      case DSS_IFACE_IOCTL_BEARER_TECH_CHANGED_EV:
         return FetchHandler(&mpBearerTechHandler, ppEventHandler, bInit);
      case DSS_IFACE_IOCTL_PHYS_LINK_DOWN_EV:
      case DSS_IFACE_IOCTL_PHYS_LINK_UP_EV:
      case DSS_IFACE_IOCTL_PHYS_LINK_COMING_UP_EV:
      case DSS_IFACE_IOCTL_PHYS_LINK_GOING_DOWN_EV:
         return FetchHandler(&mpPhysLinkStateHandler, ppEventHandler, bInit);
#ifndef FEATURE_DSS_LINUX
      case DSS_IFACE_IOCTL_QOS_AWARE_SYSTEM_EV:
      case DSS_IFACE_IOCTL_QOS_UNAWARE_SYSTEM_EV:
         return FetchHandler(&mpQoSAwareUnAwareHandler, ppEventHandler, bInit);
      case DSS_IFACE_IOCTL_QOS_ADDED_ON_IFACE_EV:
         return AEE_EUNSUPPORTED;
      case DSS_IFACE_IOCTL_QOS_DELETED_ON_IFACE_EV:
         return AEE_EUNSUPPORTED;
      case DSS_IFACE_IOCTL_707_NETWORK_SUPPORTED_QOS_PROFILES_CHANGED_EV:
         return FetchHandler(&mpQoSProfileChangedHandler, ppEventHandler, bInit);  // TODO change handler name to 1x something
     case DSS_IFACE_IOCTL_PRIMARY_QOS_MODIFY_RESULT_EV:
         return FetchHandler(&mpPrimaryQoSModifyHandler, ppEventHandler, bInit);
#endif
     case DSS_IFACE_IOCTL_OUTAGE_NOTIFICATION_EV:
        return FetchHandler(&mpOutageHandler, ppEventHandler, bInit);
#ifdef FEATURE_DATA_PS_IPV6
      case DSS_IFACE_IOCTL_PREFIX_UPDATE_EV:
         return FetchHandler(&mpDSSIPv6PrefixChangedStateHandler, ppEventHandler, bInit);
#endif

      case DSS_IFACE_IOCTL_MT_REQUEST_EV:
         return FetchHandler(&mpMTPDRequestHandler, ppEventHandler, bInit);
      case DSS_IFACE_IOCTL_707_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_SUCCESS_EV:
      case DSS_IFACE_IOCTL_707_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_FAILURE_EV:
         return FetchHandler(&mpSlottedResultHandler, ppEventHandler, bInit);
      case DSS_IFACE_IOCTL_707_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_SESSION_CHANGED_EV:
         return FetchHandler(&mpSlottedSessionChangedHandler, ppEventHandler, bInit);
      case DSS_IFACE_IOCTL_707_ENABLE_HDR_REV0_RATE_INERTIA_SUCCESS_EV:
      case DSS_IFACE_IOCTL_707_ENABLE_HDR_REV0_RATE_INERTIA_FAILURE_EV:
         return FetchHandler(&mpHDRRev0RateInteriaHandler, ppEventHandler, bInit);

      default:
         return QDS_EFAULT;
   }
}

// TODO: documentation
void DSSNetApp::IfaceUpOrDownSignalFcn(void *pNetAppInstance)
{
   DSSNetApp *pNetApp = (DSSNetApp*)pNetAppInstance;
   dss_iface_ioctl_state_type state;
   sint15 dss_errno = DS_ENETNONET;
   bool invokeCback = false;
   dss_iface_id_type ifaceId = DSS_IFACE_INVALID_ID;
   dss_net_cb_fcn  pNetCb;
   sint15 netHandle;
   void *pUserData;
   boolean bIsPPPOpen;


   LOG_MSG_FUNCTION_ENTRY("NetApp_ptr:0x%p", pNetAppInstance, 0, 0);

   // TODO: temp check to try and avoid a case where the object was released
   // while the signal was already set and queued
   if (NULL == pNetApp || NULL == pNetApp->mpIDSNetworkPriv) {
      LOG_MSG_ERROR("Invalid user data", 0, 0, 0);
      return;
   }

   pNetApp->mpCritSect->Enter();

   if (0 != pNetApp->mpIfaceUpOrDownSignalCtl) {
      (void)(pNetApp->mpIfaceUpOrDownSignalCtl)->Enable();
   }

   pNetApp->GetIsPPPOpen(&bIsPPPOpen);
   if (FALSE == bIsPPPOpen) {
      pNetApp->mpCritSect->Leave();
      LOG_MSG_INFO1("DSSNetApp::IfaceUpOrDownSignalFcn PPP open not called, do not post event",0,0,0);
      return;
   }

   (void)pNetApp->GetIfaceState(&state);
   netHandle = pNetApp->mNetHandle;
   if (IFACE_UP == state) {
      (void)SetIfaceIDOnIfaceUPEvent(pNetApp->mpIDSNetwork , pNetApp, pNetApp->mNetHandle);
      pNetApp->GetIfaceId (&ifaceId);

      // Avoid sending multiple consecutive IFACE_UP events to the application.
      // Such may happen due to real time check in dsnet which sets the signal immediately
      // upon dss_pppopen, when DSS registers for those events. Possibly, until DSS gets
      // to check the iface state here above, that iface already went UP causing this
      // function to send event to app. However, when iface went UP it also set the signal
      // triggering another IFACE UP event that arrives to this function subsequently.
      if (IFACE_UP != pNetApp->mbLastIfaceStateSentToApp) {         
         dss_errno = DS_ENETISCONN;    
         invokeCback = true;
         pNetApp->mbLastIfaceStateSentToApp = IFACE_UP;
      }
      else {
         LOG_MSG_INFO1("Avoid sending multiple consecutive IFACE_UP events to app, netHandle: %d, ifaceId: %u", netHandle, ifaceId, 0);
      }

   }
   else if (IFACE_DOWN == state) {
      // Invalidate the iface id field of the DSSNetAPP
      pNetApp->GetIfaceId (&ifaceId);
      // save iface_id for event reporting after the iface is torn down
      // see usage in DSSEventHandlerNetApp
      pNetApp->mEventReportIfaceId = ifaceId;
      pNetApp->SetIfaceId(DSS_IFACE_INVALID_ID);
      pNetApp->FreeLists();
#ifndef FEATURE_DSS_LINUX
      pNetApp->RemovePrimaryQoSModifyStatusHandler();
#endif
      pNetApp->SetIsPPPOpen(FALSE);
      // Avoid sending multiple consecutive IFACE_DOWN events to the application.
      if (IFACE_DOWN != pNetApp->mbLastIfaceStateSentToApp) {   
         dss_errno = DS_ENETNONET;
         invokeCback = true;
         pNetApp->mbLastIfaceStateSentToApp = IFACE_DOWN;
      }
      else {
         LOG_MSG_INFO1("Avoid sending multiple consecutive IFACE_DOWN events to app, netHandle: %d, ifaceId: %u", netHandle, ifaceId, 0);
      }
   }

   // If app provided network events CB function invoke it.
   // Leave the critical section before calling APP callback to avoid the following race condition:
   // 1. Callback is invoked for IFACE_DOWN (DS_ENETNONET) event.
   // 2. App calls dss_close_netlib() which ends up releasing the pNetApp object.
   // 3. Control returns to this function and we try to access released critical section.

   // Store these before releasing the lock in order to safely use in CB.
   pNetCb = pNetApp->mNetCb;
   pUserData = pNetApp->mNetUserData;

   pNetApp->mpCritSect->Leave();

   if (true == invokeCback && NULL != pNetCb) {
      LOG_MSG_INFO1("DSS calld App callback, netHandle: %d, ifaceId: %u, state=%d", netHandle, ifaceId, state);
      (void) pNetCb (netHandle, 
                     ifaceId,
                     dss_errno,
                     pUserData);
   }
}

AEEResult DSSNetApp::SetIfaceIDOnIfaceUPEvent(INetwork* pIDSNetwork, DSSNetApp* pNetApp, sint15 appid)
{
   AEEResult res = AEE_SUCCESS;
   dss_iface_id_type iface_id;

   if (NULL != pIDSNetwork) {
      IDS_ERR_RET(BuildIfaceId(pIDSNetwork,appid,&iface_id));

      // Save the iface index in the DSSNetApp structure
      pNetApp->SetIfaceId(iface_id);
   }

   if (AEE_SUCCESS != res) {
      return res;
   }
   return AEE_SUCCESS;
}


void DSSNetApp::FreeLists()
{
#ifndef FEATURE_DSS_LINUX
   DSSNetQoSSecondary* tempNetQosSec = NULL;
   DSSMCast* tempMcast = NULL;
   DSSNetMCastMBMSCtrl* tempNetMcastCtrl = NULL;
#endif
   DSSPrivIpv6Addr* tempPrivIpv6 = NULL;
   int ret;

#ifndef FEATURE_DSS_LINUX
   while (NULL != mpDSSQoSSecondaryList)
   {
      ret = mpDSSQoSSecondaryList->GetNext(&tempNetQosSec);
      if (AEE_SUCCESS != ret)
      {
         return;
      }
      PS_MEM_RELEASE(mpDSSQoSSecondaryList);
      mpDSSQoSSecondaryList = tempNetQosSec;
   }


   while (NULL != mpDSSMCastList) {
      ret = mpDSSMCastList->GetNext(&tempMcast);
      if (AEE_SUCCESS != ret)
      {
         return;
      }
      PS_MEM_RELEASE(mpDSSMCastList);
      mpDSSMCastList = tempMcast;
   }

   while (NULL != mpDSSNetMCastMBMSList) {
      ret = mpDSSNetMCastMBMSList->GetNext(&tempNetMcastCtrl);
      if (AEE_SUCCESS != ret)
      {
         return;
      }
      PS_MEM_RELEASE(mpDSSNetMCastMBMSList);
      mpDSSNetMCastMBMSList = tempNetMcastCtrl;
   }
#endif
   while (NULL != mpDSSPrivIpv6AddrList) {
      ret = mpDSSPrivIpv6AddrList->GetNext(&tempPrivIpv6);
      if (AEE_SUCCESS != ret)
      {
         return;
      }
      PS_MEM_RELEASE(mpDSSPrivIpv6AddrList);
      mpDSSPrivIpv6AddrList = tempPrivIpv6;
   }

   (void) FlushDSSFirewallRuleTable();
}

AEEResult DSSNetApp::GetPhysLink
(
   ds::Net::IPhysLink** pIDSPhysLink
)
{
   if(0 == mpDSNetPhysLink){
      if (NULL == mpIDSNetwork){
         return QDS_EINVAL;
      }

      if(AEE_SUCCESS != mpIDSNetwork->GetTXPhysLink(&mpDSNetPhysLink)){
         LOG_MSG_INFO1("GetPhysLinkObject - iface down, physLink cannot be created", 0, 0, 0);
         return QDS_ENETDOWN;
      }
   }

   (void)mpDSNetPhysLink->AddRef();
   
   *pIDSPhysLink = mpDSNetPhysLink;

   return AEE_SUCCESS;
}

AEEResult DSSNetApp::GetTechUMTS
(
   ds::Net::ITechUMTS** pIDSTechUMTS
)
{
   if(0 == mpDSNetTechUMTS){
      INetworkFactory*       piNetworkFactory = NULL;
      AEEResult              res = AEE_SUCCESS;

      res = DSSGlobals::Instance()->GetNetworkFactory(&piNetworkFactory);
      if (AEE_SUCCESS != res || 0 == piNetworkFactory) {
         return QDS_EFAULT;
      }

      res = piNetworkFactory->CreateTechUMTS(&mpDSNetTechUMTS);
      if(AEE_SUCCESS != res){
         DSSCommon::ReleaseIf((IQI**)&piNetworkFactory);
         return QDS_EFAULT;
      }

      DSSCommon::ReleaseIf((IQI**)&piNetworkFactory);
}

   (void)mpDSNetTechUMTS->AddRef();

   *pIDSTechUMTS = mpDSNetTechUMTS;
   
   return AEE_SUCCESS;
}

//===================================================================
//  FUNCTION:   DSSNetApp::GetNetworkStatistics
//
//  DESCRIPTION:
//  Serves DSS_IFACE_IOCTL_GET_IFACE_STATS
//===================================================================

AEEResult DSSNetApp::GetNetworkStatistics(dss_iface_ioctl_get_iface_stats_type* pStats)
{
   if (NULL == mpIDSNetworkPriv) {
      return ds::Error::QDS_EINVAL; 
   }

   NetworkStatsType statsType;
   IDS_ERR_RET(mpIDSNetwork->GetNetworkStatistics(&statsType));

   IDS_ERR_RET(DSSConversion::IDS2DSStatistics(&statsType, pStats));

   return AEE_SUCCESS;
}


//===================================================================
//  FUNCTION:   DSSNetApp::ResetNetworkStatistics
//
//  DESCRIPTION:
//  Serves DSS_IFACE_IOCTL_RESET_IFACE_STATS
//===================================================================

AEEResult DSSNetApp::ResetNetworkStatistics(void)
{
   if (NULL == mpIDSNetworkPriv) {
      return ds::Error::QDS_EINVAL; 
   }

   IDS_ERR_RET(mpIDSNetwork->ResetNetworkStatistics());

   return AEE_SUCCESS;
}

void DSSNetApp::ReleasePhysLink(void)
{
   DSSCommon::ReleaseIf((IQI**)&mpDSNetPhysLink);
}

