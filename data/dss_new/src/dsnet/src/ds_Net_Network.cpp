/*===========================================================================
  FILE: Network.cpp

  OVERVIEW: This file provides implementation of the Network class.

  DEPENDENCIES: None

  Copyright (c) 2008-2012 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_Network.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-06-02 cp  IOCTL changes for Soft AP DHCP server.
  2009-02-27 hm  Added IPv6 priv addr and prefixes support.
  2008-03-10 hm  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "AEEstd.h"
#include "AEEStdErr.h"
#include "ds_Net_INetwork1x.h"
#include "ds_Net_INetwork1xPriv.h"
#include "ds_Net_INetworkUMTS.h"
#include "ds_Net_IMCastMBMSCtrlPriv.h"
#include "ds_Net_INetworkControl.h"
#include "ds_Net_IFirewallManager.h"
#include "ds_Net_INatSession.h"
#include "ds_Utils_DebugMsg.h"
#include "ds_Utils_Conversion.h"
#include "ds_Net_Network.h"
#include "ds_Net_Platform.h"
#include "ds_Net_BearerTech.h"
#include "ds_Net_EventDefs.h"
#include "ds_Net_EventManager.h"
#include "ds_Net_Network1X.h"
#include "ds_Net_NetworkUMTS.h"
#include "ds_Net_MCastMBMSCtrl.h"
#include "ds_Net_NetworkIPv6.h"
#include "ds_Net_NetworkFactory.h"
#include "ds_Net_QoSDefault.h"
#include "ds_Net_QoSSecondary.h"
#include "ds_Net_MCastSession.h"
#include "ds_Net_IPFilterSpec.h"
#include "ds_Net_FirewallManager.h"
#include "ds_Net_Conversion.h"
#include "ds_Net_NatSession.h"
#include "ds_Utils_CreateInstance.h"
#include "AEECSignalBus.h"
#include "ds_Net_Privileges_Def.h"
#include "AEEIPrivSet.h"
#include "ps_system_heap.h"
#include "ds_Utils_SockAddrInternalTypes.h"
#include "ps_policy_mgr.h"

using namespace ds::Net;
using namespace ds::Net::Conversion;
using namespace ds::Error;
using namespace NetPlatform;

/*===========================================================================

                     PRIVATE FUNCTION DEFINITIONS

===========================================================================*/
ds::ErrorType Network::IPAddrIOCTL
(
  IfaceIoctlEnumType   ioctlName,
  ds::IPAddrType*      pDSIPAddr
)
{
  ds::ErrorType        result;
  PSIPAddrType         psIPAddr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_FUNCTION_ENTRY ("IP Addr IOCTL %d", ioctlName, 0, 0);

  /*-------------------------------------------------------------------------
    Validation.
  -------------------------------------------------------------------------*/
  if (NULL == pDSIPAddr)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Get the iface handle and call the relevant IOCTL
  -------------------------------------------------------------------------*/
  memset (&psIPAddr, 0, sizeof(PSIPAddrType));
  result = IfaceIoctl (GetHandle(),
                       ioctlName,
                       static_cast <void *> (&psIPAddr));
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Convert IOCTL result back to ds::IPAddrType
  -------------------------------------------------------------------------*/
  result = PS2DSIPAddr (&psIPAddr, pDSIPAddr);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err 0x%x in IOCTL %d", result, ioctlName, 0);
  return result;

} /* IPAddrIOCTL() */

/*===========================================================================

                     PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
/*---------------------------------------------------------------------------
  CONSTRUCTOR/DESTRUCTOR
---------------------------------------------------------------------------*/
Network::Network
(
  Policy           *argPolicyPtr,
  IPrivSet         *privSetPtr,
  NetworkModeType  networkMode
) : Handle(),
    mNetworkMode(networkMode),
    mFlags(0),
    mLastNetDownReason(0),
    mpPolicy (argPolicyPtr),
    mpIPFilterReg (NULL),
    mBringupAgain (FALSE),
    mBringupFirst (FALSE),
    mTeardown (FALSE),
    mOutageEventOccurred(FALSE),
    mpSigBusStateChange(0),
    mpSigBusBearerTech(0),
    mpSigBusIPAddr(0),
    mpSigBusOutage(0),
    mpSigBusQoSAware(0),
    mpSigBusRFConditions(0),
    mpSigBusExtendedIPConfig(0),
    mpSigBusIdle(0),
    mpTechObjHandle (NULL),
    mpTechObjNetwork1x (NULL),
    mpTechObjNetwork1xPriv (NULL),
    mpNetworkUMTS (NULL),
    mpNetworkIPv6 (NULL),
    mCachedPreviousState(NetworkState::QDS_CLOSED),
    mAppPriority(PS_POLICY_MGR_LEGACY_PRIORITY),
    mpPrivSet(privSetPtr),
    refCnt(1),
    weakRefCnt(1)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Creating Network 0x%p", this, 0, 0);

  if (NULL == argPolicyPtr || 0 == privSetPtr)
  {
    ASSERT (0);
  }

  (void) mpPolicy->AddRef();

  (void) mpPrivSet->AddRef();

  mLastNetDownReason = NetDownReason::QDS_NOT_SPECIFIED;

} /* Network::Network() */

ds::ErrorType Network::Init
(
  void
)
{
  int32                     result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Init object 0x%p", this, 0, 0);

  /* Create signal buses to hold event registration info */
  result = DS_Utils_CreateInstance(0, AEECLSID_CSignalBus,
                                   0, (void **) &mpSigBusStateChange);
  if (AEE_SUCCESS != result)
  {
    return result;
  }

  result = DS_Utils_CreateInstance(0, AEECLSID_CSignalBus,
                                   0, (void **) &mpSigBusBearerTech);
  if (AEE_SUCCESS != result)
  {
    return result;
  }

  result = DS_Utils_CreateInstance(0, AEECLSID_CSignalBus,
                                   0, (void **) &mpSigBusIPAddr);
  if (AEE_SUCCESS != result)
  {
    return result;
  }

  result = DS_Utils_CreateInstance(0, AEECLSID_CSignalBus,
                                   0, (void **) &mpSigBusOutage);
  if (AEE_SUCCESS != result)
  {
    return result;
  }

  result = DS_Utils_CreateInstance(0, AEECLSID_CSignalBus,
                                   0, (void **) &mpSigBusQoSAware);
  if (AEE_SUCCESS != result)
  {
    return result;
  }

  result = DS_Utils_CreateInstance(0, AEECLSID_CSignalBus,
                                   0, (void **) &mpSigBusRFConditions);
  if (AEE_SUCCESS != result)
  {
    return result;
  }

  result = DS_Utils_CreateInstance(0, AEECLSID_CSignalBus,
                                   0, (void **) &mpSigBusExtendedIPConfig);
  if (AEE_SUCCESS != result)
  {
    return result;
  }

  result = DS_Utils_CreateInstance(0, AEECLSID_CSignalBus,
                                   0, (void **) &mpSigBusIdle);
  if (AEE_SUCCESS != result)
  {
    return result;
  }

  Handle::Init(EventManager::networkObjList);
  return AEE_SUCCESS;
}

void Network::Destructor
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting Network 0x%p", this, 0, 0);

  /*lint -save -e1550, -e1551 */

  /*-------------------------------------------------------------------------
    Release underlying PS_IFACE, if any is associated. Base class
    destructor is invoked after derived class is destroyed. So we can be
    confident that mpICritSect (maintained by Handle base object) would be
    valid during the course of destructor.
  -------------------------------------------------------------------------*/
  (void) Stop();

  if (NULL != mpTechObjHandle)
  {
    (void) mpTechObjHandle->Release();
    mpTechObjNetwork1x = NULL;
    mpTechObjNetwork1xPriv = NULL;
  }
  /*lint -restore */

  DS_UTILS_RELEASEIF (mpPrivSet);
  DS_UTILS_RELEASEIF (mpPolicy);
  DS_UTILS_RELEASEIF (mpNetworkUMTS);
  DS_UTILS_RELEASEIF (mpNetworkIPv6);
  DS_UTILS_RELEASEIF (mpIPFilterReg);
  DS_UTILS_RELEASEIF (mpSigBusStateChange);
  DS_UTILS_RELEASEIF (mpSigBusBearerTech);
  DS_UTILS_RELEASEIF (mpSigBusIPAddr);
  DS_UTILS_RELEASEIF (mpSigBusOutage);
  DS_UTILS_RELEASEIF (mpSigBusQoSAware);
  DS_UTILS_RELEASEIF (mpSigBusRFConditions);
  DS_UTILS_RELEASEIF (mpSigBusExtendedIPConfig);
  DS_UTILS_RELEASEIF (mpSigBusIdle);

 //Should be last statement. Call destructor for the base class.
  Handle::Destructor();

} /* Network::Destructor() */

Network::~Network
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  mpPolicy = NULL;
  mpIPFilterReg = NULL;
  mpTechObjHandle = NULL;
  mpTechObjNetwork1x = NULL;
  mpTechObjNetwork1xPriv = NULL;
  mpNetworkUMTS = NULL;
  mpNetworkIPv6 = NULL;
  mpPrivSet = NULL;

} /* Network::~Network() */

ds::ErrorType Network::CreateQoSManager
(
  IQoSManager**     ppIQoSMgr
)
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
#ifndef FEATURE_DSS_LINUX
  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  if (NULL == ppIQoSMgr)
  {
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
  check client provided privileges
  -------------------------------------------------------------------------*/
  if (AEE_SUCCESS != mpPrivSet->CheckPrivileges
                                              (
                                                (AEEPRIVID *)&AEEPRIVID_PQoS,
                                                sizeof(AEEPRIVID_PQoS)
                                              ))
  {
    LOG_MSG_ERROR( "No privilege for CreateQoSManager operation",
      0, 0, 0);
    return AEE_EPRIVLEVEL;
  }

  /*-------------------------------------------------------------------------
  Create a new QoSManager object all the time. If the network object's
  handle changes anytime, old QoSManager does not make sense.
  -------------------------------------------------------------------------*/
  *ppIQoSMgr = static_cast <IQoSManager *> (new QoSManager
                                                         (
                                                           GetHandle(),
                                                           mpPrivSet,
                                                           mNetworkMode
                                                         ));
  if (NULL == *ppIQoSMgr)
  {
    return AEE_ENOMEMORY;
  }

  return AEE_SUCCESS;
#else
  LOG_MSG_ERROR( "QOS not yet supported", 0, 0, 0);
  return AEE_EFAILED;
#endif

} /* CreateQoSManager() */

ds::ErrorType Network::CreateMCastManager
(
 IMCastManager**   ppIMCastMgr
 )
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  if (NULL == ppIMCastMgr)
  {
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
  check client provided privileges
  -------------------------------------------------------------------------*/
  if (AEE_SUCCESS != mpPrivSet->CheckPrivileges
                                              (
                                                (AEEPRIVID *)&AEEPRIVID_PMCastClient,
                                                sizeof(AEEPRIVID_PMCastClient)
                                                ))
  {
    LOG_MSG_ERROR( "No privilege for CreateMCastManager operation",
      0, 0, 0);
    return AEE_EPRIVLEVEL;
  }

  /*-------------------------------------------------------------------------
  Create a new MCastManager object all the time. If the network object's
  handle changes anytime, old MCastManager does not make sense.
  -------------------------------------------------------------------------*/
  *ppIMCastMgr = static_cast<IMCastManager *> (new MCastManager
                                                              (
                                                                GetHandle(),
                                                                mpPrivSet,
                                                                mNetworkMode
                                                              ));
  if (NULL == *ppIMCastMgr)
  {
    return AEE_ENOMEMORY;
  }

  return AEE_SUCCESS;
} /* CreateMCastManager() */

ds::ErrorType Network::LookupInterface
(
  void
)
{
  int32              result;
  int32              ifaceHandle;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
    If the interface is already brought up/looked up, return SUCCESS
  -------------------------------------------------------------------------*/
  if (0 != GetHandle())
  {
     return AEE_SUCCESS;
  }

  /*-----------------------------------------------------------------------
    Perform routing lookup and bring up the network by policy.
  -----------------------------------------------------------------------*/
  result = IfaceLookUpByPolicy (static_cast <IPolicy *>(mpPolicy),
                                &ifaceHandle);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Set the interface obtained as the handle in Handle object.
  -----------------------------------------------------------------------*/
  SetHandle(ifaceHandle);
  LOG_MSG_FUNCTION_EXIT ("Obj 0x%p, handle 0x%x", this, ifaceHandle, 0);
  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err 0x%x for LookupNetwork", result, 0, 0);
  return result;

} /* LookupInterface() */

ds::ErrorType Network::GetPolicy
(
  IPolicy** ppIDSNetPolicy
)
{
  if (NULL == ppIDSNetPolicy)
  {
    return QDS_EFAULT;
  }

  *ppIDSNetPolicy = static_cast <IPolicy *> (mpPolicy);
  (void) mpPolicy->AddRef();
  return AEE_SUCCESS;

} /* GetPolicy() */

ds::ErrorType Network::SetPolicy
(
  IPolicy* pIPolicy
)
{
  if (NULL == pIPolicy)
  {
    return QDS_EFAULT;
  }

  mpICritSect->Enter();

  if (NULL != mpPolicy)
  {
    (void) mpPolicy->Release();
  }

  mpPolicy = reinterpret_cast <Policy *> (pIPolicy);
  (void) mpPolicy->AddRef();

  mpICritSect->Leave();

  return AEE_SUCCESS;

} /* GetPolicy() */

/*---------------------------------------------------------------------------
  Functions inherited from INetwork
---------------------------------------------------------------------------*/
ds::ErrorType Network::GetIfaceState
(
  NetworkStateType* argNetState
)
{
  int32                  ifaceHandle;
  IfaceStateEnumType     ifaceState;
  int32                  result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == argNetState)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  ifaceHandle = GetHandle();
  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, ifaceHandle, 0);

  if (0 == ifaceHandle)
  {
    *argNetState = NetworkState::QDS_CLOSED;
    return AEE_SUCCESS;
  }

  /*-------------------------------------------------------------------------
    Call network platform IOCTL to get the interface state.
  -------------------------------------------------------------------------*/
  result = IfaceIoctl (ifaceHandle,
                       IFACE_IOCTL_GET_STATE,
                       &ifaceState);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Map Iface state to network state.
  -------------------------------------------------------------------------*/
  result = PS2DSIfaceState(ifaceState,argNetState);
  if (AEE_SUCCESS != result)
  {
     goto bail;
  }

  LOG_MSG_INFO1 ("If handle 0x%x, netstatus %d",
    ifaceHandle, (int)*argNetState, 0);
  return result;

bail:
  LOG_MSG_ERROR ("Invalid Iface state %d", ifaceState, 0, 0);
  return result;

} /* GetIfaceState() */

ds::ErrorType Network::GetLastNetDownReason
(
  /*rout */ NetDownReasonType* argLastNetDownReason
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, reason %d", this, mLastNetDownReason, 0);

  if (NULL == argLastNetDownReason)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  mpICritSect->Enter();
  *argLastNetDownReason = mLastNetDownReason;
  mpICritSect->Leave();

  return AEE_SUCCESS;
}/* GetLastNetDownReason() */


ds::ErrorType Network::GetIPAddr
(
  ds::IPAddrType* argIPAddress
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return IPAddrIOCTL (IFACE_IOCTL_GET_IP_ADDR, argIPAddress);

} /* GetIPAddr() */

ds::ErrorType Network::GetNatPublicIPAddress
(
  ds::IPAddrType* argIPAddress
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return IPAddrIOCTL (IFACE_IOCTL_GET_NAT_PUBLIC_IP_ADDR, argIPAddress);

} /* GetIPAddress() */

ds::ErrorType Network::DhcpArpCacheUpdate
(
  const ds::Net::DhcpArpCacheUpdateType* dhcpArpCacheUpdateParam
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);
  return IfaceIoctl (GetHandle(),
                     IFACE_IOCTL_DHCP_ARP_CACHE_UPDATE,
                     (void *)dhcpArpCacheUpdateParam);
} /* DhcpArpCacheUpdate() */

ds::ErrorType Network::DhcpArpCacheClear
(
  const ds::Net::DhcpArpCacheClearType* dhcpArpCacheClearParam
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);
  return IfaceIoctl (GetHandle(),
                     IFACE_IOCTL_DHCP_ARP_CACHE_CLEAR,
                     (void *)dhcpArpCacheClearParam);
} /* DhcpArpCacheClear() */

ds::ErrorType Network::GetDhcpDeviceInfo
(
 ds::Net::DhcpGetDeviceInfoType * connDevInfo
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  if ( ( connDevInfo == NULL ) || ( connDevInfo->dev_info == NULL )
    || ( connDevInfo->dev_infoLen == 0 )
    || ( connDevInfo->dev_infoLenReq != 0 ))
  {
    return QDS_EFAULT;
  }
  return IfaceIoctl (GetHandle(),
                     IFACE_IOCTL_DHCP_SERVER_GET_DEVICE_INFO,
                     (void *)connDevInfo);
} /* GetDhcpDeviceInfo() */

ds::ErrorType Network::GetSIPServerDomainNames
(
  DomainName* pSipDomainNames,
  int sipServerDomainNamesLen,
  int* pSipDomainNamesLenReq
)
{
  int32                         result;
  int                           index;
  SipServerDomainNameInfoType   psSipDomainNames;
  int                           seqNumItems = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  /*-----------------------------------------------------------------------
    Prepare the IOCTL arguments to get the SIP server domain names.
  -----------------------------------------------------------------------*/
  memset (&psSipDomainNames, 0, sizeof (psSipDomainNames));
  psSipDomainNames.name_array =
    (dss_iface_ioctl_domain_name_type *)
    ps_system_heap_mem_alloc
    (
      sizeof (DomainNameType) * PS_IFACE_MAX_SIP_SERVER_DOMAIN_NAMES
    );

  if (NULL == psSipDomainNames.name_array)
  {
    /*---------------------------------------------------------------------
      Memory configuration is incorrect. This is an ASSERT condition.
    ---------------------------------------------------------------------*/
    ASSERT (0);
    result = AEE_ENOMEMORY;
    goto bail;
  }
  psSipDomainNames.count = PS_IFACE_MAX_SIP_SERVER_DOMAIN_NAMES;

  /*-----------------------------------------------------------------------
    Call the iface IOCTL to get SIP server domain names
  -----------------------------------------------------------------------*/
  result = IfaceIoctl (GetHandle(),
                       IFACE_IOCTL_GET_SIP_SERV_DOMAIN_NAMES,
                       static_cast <void *> (&psSipDomainNames));
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  result =
    ds::Utils::Conversion::ProcessLenReq
    (
      sipServerDomainNamesLen,
      pSipDomainNamesLenReq,
      psSipDomainNames.count,
      &seqNumItems
    );
  if(AEE_SUCCESS != result)
  {
    goto bail;
  }
  /*-----------------------------------------------------------------------
    Convert the individual SIP domain names into out argument.
  -----------------------------------------------------------------------*/
  for (index = 0; index < seqNumItems; index++)
  {
    memcpy(pSipDomainNames[index],
           psSipDomainNames.name_array[index].domain_name,
           sizeof(DomainName));
  }

  /* Fall through */

bail:
  if (NULL != psSipDomainNames.name_array)
  {
    PS_SYSTEM_HEAP_MEM_FREE(psSipDomainNames.name_array);
  }

  LOG_MSG_FUNCTION_EXIT ("Return 0x%x", result, 0, 0);
  return result;
}

ds::ErrorType Network::GetSIPServerAddr
(
  ds::IPAddrType* pSipServerAddr,
  int sipServerAddrLen,
  int* pSipServerAddrLenReq
)
{
  SipServerAddrInfoType sipServerAddr;
  int32                 result;
  int32                 index;
  int                   seqNumItems = 0;
  int               numCopied = 0;
  int32             copyResult = AEE_SUCCESS;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  memset (&sipServerAddr, 0, sizeof (sipServerAddr));
  sipServerAddr.addr_array =
    (ip_addr_type *)
    ps_system_heap_mem_alloc
    (
      sizeof (ip_addr_type) * PS_IFACE_MAX_SIP_SERVER_ADDRESSES
    );

  if (NULL == sipServerAddr.addr_array)
  {
    /*---------------------------------------------------------------------
      Memory configuration is incorrect. This is an ASSERT condition.
    ---------------------------------------------------------------------*/
    ASSERT (0);
    result = AEE_ENOMEMORY;
    goto bail;
  }
  sipServerAddr.count = PS_IFACE_MAX_SIP_SERVER_ADDRESSES;

  /*-----------------------------------------------------------------------
    Call the iface IOCTL to get SIP server addresses
  -----------------------------------------------------------------------*/
  result = IfaceIoctl (GetHandle(),
                       IFACE_IOCTL_GET_SIP_SERV_ADDR,
                       static_cast <void *> (&sipServerAddr));
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Convert results to ds format
  -----------------------------------------------------------------------*/
  result =
    ds::Utils::Conversion::ProcessLenReq
    (
      sipServerAddrLen,
      pSipServerAddrLenReq,
      sipServerAddr.count,
      &seqNumItems
    );
  if(AEE_SUCCESS != result)
  {
    goto bail;
  }
  for (index = 0; index < seqNumItems; index++)
  {
    copyResult = PS2DSIPAddr (&(sipServerAddr.addr_array[index]),
                              &pSipServerAddr[numCopied]);
    if(AEE_SUCCESS != copyResult)
    {
      /*-----------------------------------------------------------------------
        do not copy invalid address
      -----------------------------------------------------------------------*/
      continue;
    }

    numCopied++;
  }


  *pSipServerAddrLenReq = numCopied;

  /* Fall through */

bail:

  if(NULL != sipServerAddr.addr_array)
  {
    PS_SYSTEM_HEAP_MEM_FREE(sipServerAddr.addr_array);
  }
  LOG_MSG_FUNCTION_EXIT ("Return 0x%x", result, 0, 0);
  return result;
}

ds::ErrorType Network::GetQosAware
(
  boolean* pGetQosAware
)
{
  int32  ifaceHandle;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  /*-------------------------------------------------------------------------
    check client provided privileges
  -------------------------------------------------------------------------*/
  if (AEE_SUCCESS != mpPrivSet->CheckPrivileges
                                (
                                  (AEEPRIVID *)&AEEPRIVID_PQoS,
                                  sizeof(AEEPRIVID_PQoS)
                                ))
  {
    LOG_MSG_ERROR( "No privilege for GetQosAware operation",
                   0, 0, 0);
    return AEE_EPRIVLEVEL;
  }

  /*-------------------------------------------------------------------------
    HACK to make event propagation work even after call goes down
  -------------------------------------------------------------------------*/
  ifaceHandle = GetHandle();
  if (0 == ifaceHandle && 0 != mStaleIfaceHandle)
  {
    ifaceHandle = mStaleIfaceHandle;
    LOG_MSG_INFO1("Using stale handle 0x%x for obj 0x%p",
                  mStaleIfaceHandle, this, 0);
  }

  return IfaceIoctlNonNullArg (ifaceHandle,
                               IFACE_IOCTL_ON_QOS_AWARE_SYSTEM,
                               (void *)pGetQosAware);
} /* GetQosAware() */


ds::ErrorType Network::GetOutageInfo
(
  OutageInfoType* pOutageInfo
)
{
  int32                        result;
  OutageNotificationInfoType   outageInfo;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  if (NULL == pOutageInfo)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    In case that outage event didn't occurred, return invalid state
  -----------------------------------------------------------------------*/
  if (FALSE == mOutageEventOccurred)
  {
    pOutageInfo->state = OutageState::QDS_INVALID;
    pOutageInfo->timeToOutage = 0;
    pOutageInfo->duration = 0;
    return AEE_SUCCESS;
  }

  /*-----------------------------------------------------------------------
    Call the iface IOCTL to get outage info.
  -----------------------------------------------------------------------*/
  result = IfaceIoctl (GetHandle(),
                       IFACE_IOCTL_GET_OUTAGE_NOTIFICATION_INFO,
                      (void *) (&outageInfo));
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-----------------------------------------------------------------------
  Convert to out-params.
  -----------------------------------------------------------------------*/
  pOutageInfo->timeToOutage = outageInfo.time_to_outage;
  pOutageInfo->duration     = outageInfo.duration;

  if (0 != pOutageInfo->timeToOutage)
  {
    pOutageInfo->state = OutageState::QDS_VALID;
  }
  else if (0 != pOutageInfo->duration)
  {
    pOutageInfo->state = OutageState::QDS_STARTED;
  }
  else
  {
    pOutageInfo->state = OutageState::QDS_EXPIRED;
  }

  return result;

bail:
  LOG_MSG_ERROR ("Err 0x%x", result, 0, 0);
  return result;

} /* GetOutageInfo() */

ds::ErrorType Network::GetBearerInfo
(
  IBearerInfo**  ppIDSNetBearerInfo
)
{
  BearerTechType           bearerTechInfo;
  BearerTechRateType       pDataBearerRate;
  int32                    result;
  int32                    ifaceHandle;
  ds::Net::IfaceNameType   ifaceName;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  if (NULL == ppIDSNetBearerInfo)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  *ppIDSNetBearerInfo = NULL;
  memset (&bearerTechInfo, 0, sizeof (BearerTechType));

  /*-------------------------------------------------------------------------
    HACK to make event propagation work even after call goes down
  -------------------------------------------------------------------------*/
  ifaceHandle = GetHandle();
  if (0 == ifaceHandle && 0 != mStaleIfaceHandle)
  {
    ifaceHandle = mStaleIfaceHandle;
    LOG_MSG_INFO1("Using stale handle 0x%x for obj 0x%p",
                  mStaleIfaceHandle, this, 0);
  }

  /*-------------------------------------------------------------------------
    Call the iface IOCTL to get bearer technology.
  -------------------------------------------------------------------------*/
  result = IfaceIoctl (ifaceHandle,
                       IFACE_IOCTL_GET_BEARER_TECHNOLOGY,
                       static_cast <void *> (&bearerTechInfo));
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
  Get bearer technology rate.
  -------------------------------------------------------------------------*/
  result = IfaceIoctlNonNullArg (ifaceHandle,
                                 IFACE_IOCTL_GET_DATA_BEARER_RATE,
                                 static_cast <void *> (&pDataBearerRate));
  if (AEE_SUCCESS != result)
  {
     goto bail;
  }

  /*-----------------------------------------------------------------------
    Convert the network type to iface name.
  -----------------------------------------------------------------------*/
  switch (bearerTechInfo.current_network)
  {
    case PS_IFACE_NETWORK_CDMA:
      ifaceName = IfaceName::IFACE_CDMA_SN;
      break;
    case PS_IFACE_NETWORK_UMTS:
      ifaceName = IfaceName::IFACE_UMTS;
      break;
    case PS_IFACE_NETWORK_WLAN:
      ifaceName = IfaceName::IFACE_WLAN;
      break;
    default:
       result = QDS_INTERNAL;
       goto bail;
  }

  /*-----------------------------------------------------------------------
    Construct the Bearer tech object using the IOCTL result.
  -----------------------------------------------------------------------*/
  *ppIDSNetBearerInfo = static_cast <IBearerInfo *>
    (new BearerTech (ifaceName,
                     bearerTechInfo.data.cdma_type.rat_mask,
                     bearerTechInfo.data.cdma_type.so_mask,
                     bearerTechInfo.data.umts_type.rat_mask,
                     &pDataBearerRate));
  if (NULL == *ppIDSNetBearerInfo)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err 0x%x", result, 0, 0);
  return result;
}

ds::ErrorType Network::GetAddressFamily
(
  ds::AddrFamilyType * argAddressFamily
)
{
  int32                result;
  PSIPAddrFamilyType   AddressFamily;
  ds::ErrorType        nRet;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  if (NULL == argAddressFamily)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Call the iface IOCTL to get address family
  -----------------------------------------------------------------------*/
  memset (&AddressFamily, 0, sizeof (PSIPAddrFamilyType));
  result = IfaceIoctl (GetHandle(),
                       IFACE_IOCTL_GET_IP_FAMILY,
                       static_cast <void *> (&AddressFamily));
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  nRet = PS2DSAddrFamily(AddressFamily, argAddressFamily);
  if (AEE_SUCCESS != nRet){
     ASSERT(0);
     return nRet;
  }

  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err 0x%x", result, 0, 0);
  return result;

} /* GetAddressFamily() */

ds::ErrorType Network::GetDomainNameSearchList
(
  DomainName* pDomainNameSearchList,
  int domainNameSearchListLen,
  int* pDomainNameSearchListLenReq
)
{
  int32                     result;
  int                       index;
  DomainNameSearchListType  domainNameSearchList;
  int                       seqNumItems = 0;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  memset (&domainNameSearchList, 0, sizeof (domainNameSearchList));

  domainNameSearchList.name_array =
    (dss_iface_ioctl_domain_name_type *)
    ps_system_heap_mem_alloc
    (
      sizeof (DomainNameType) *
        PS_IFACE_MAX_SEARCH_LIST_DOMAIN_NAMES
    );

  if (NULL == domainNameSearchList.name_array)
  {
    /*---------------------------------------------------------------------
      Memory configuration is incorrect. This is an ASSERT condition.
    ---------------------------------------------------------------------*/
    ASSERT (0);
    result = AEE_ENOMEMORY;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Call the iface IOCTL to get domain name search list
  -----------------------------------------------------------------------*/
  result = IfaceIoctl (GetHandle(),
                       IFACE_IOCTL_GET_DOMAIN_NAME_SEARCH_LIST,
                       static_cast <void *> (&domainNameSearchList));
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Convert domain name search list into ds format.
  -----------------------------------------------------------------------*/
  result =
    ds::Utils::Conversion::ProcessLenReq
    (
      domainNameSearchListLen,
      pDomainNameSearchListLenReq,
      domainNameSearchList.count,
      &seqNumItems
    );
  if(AEE_SUCCESS != result)
  {
    goto bail;
  }
  /*-----------------------------------------------------------------------
    Convert the individual domain name search list items into out argument.
  -----------------------------------------------------------------------*/
  for (index = 0; index < seqNumItems; index++)
  {
    memcpy(pDomainNameSearchList[index],
           domainNameSearchList.name_array[index].domain_name,
           sizeof(DomainName));
  }

  /* Fall through */

bail:

  if(NULL != domainNameSearchList.name_array)
  {
    PS_SYSTEM_HEAP_MEM_FREE(domainNameSearchList.name_array);
  }
  LOG_MSG_FUNCTION_EXIT ("Return 0x%x", result, 0, 0);
  return result;
}

ds::ErrorType Network::GetNetMTU
(
  int* pMTU
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  return IfaceIoctlNonNullArg (GetHandle(),
                               IFACE_IOCTL_GET_MTU,
                               (void *)pMTU);

} /* GetNetMTU() */

ds::ErrorType Network::GetHWAddress
(
  HWAddressType* pHWAddr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  return IfaceIoctlNonNullArg (GetHandle(),
                               IFACE_IOCTL_GET_HW_ADDR,
                               (void *)pHWAddr);

} /* GetHWAddress() */

ds::ErrorType Network::IsLaptopCallActive
(
  boolean* pIsActive
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  return IfaceIoctlNonNullArg (GetHandle(),
                               IFACE_IOCTL_IS_LAPTOP_CALL_ACTIVE,
                               (void *)pIsActive);
} /* IsLaptopCallActive() */


ds::ErrorType Network::GoDormant
(
  DormantReasonType dormantReason
)
{
  IPhysLink*     txPhysLink = NULL;
  int                 result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  result = GetTXPhysLink(&txPhysLink);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Call go dormant on the TX phys link.
  -------------------------------------------------------------------------*/
  result = txPhysLink->GoDormant(dormantReason);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  (void) txPhysLink->Release();
  return AEE_SUCCESS;

bail:

  if (NULL != txPhysLink)
  {
    (void) txPhysLink->Release();
  }
  LOG_MSG_ERROR ("Err 0x%x", result, 0, 0);
  return result;

} /* GoDormant() */

ds::ErrorType Network::EnableDNSDuringIPCP
(
  boolean enable
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);
  return IfaceIoctlNonNullArg (GetHandle(),
                               IFACE_IOCTL_IPCP_DNS_OPT,
                               (void *)&enable);

} /* EnableDNSDuringIPCP() */

ds::ErrorType Network::RefreshDHCPConfigInfo
(
  void
)
{
  int32   result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  /*-------------------------------------------------------------------------
    check client provided privileges
  -------------------------------------------------------------------------*/
  if (AEE_SUCCESS != mpPrivSet->CheckPrivileges
                                (
                                  (AEEPRIVID *)&AEEPRIVID_PNetOTA,
                                  sizeof(AEEPRIVID_PNetOTA)
                                ))
  {
    LOG_MSG_ERROR( "No privilege for RefreshDHCPConfigInfo operation",
                   0, 0, 0);
    return AEE_EPRIVLEVEL;
  }

  /*-------------------------------------------------------------------------
    check client provided privileges
  -------------------------------------------------------------------------*/
  if (AEE_SUCCESS != mpPrivSet->CheckPrivileges
                                (
                                  (AEEPRIVID *)&AEEPRIVID_PNet,
                                  sizeof(AEEPRIVID_PNet)
                                ))
  {
    LOG_MSG_ERROR( "No privilege for RefreshDHCPConfigInfo operation",
                   0, 0, 0);
    return AEE_EPRIVLEVEL;
  }

  /*-----------------------------------------------------------------------
    Call the iface IOCTL to get hardware address.
  -----------------------------------------------------------------------*/
  result = IfaceIoctl (GetHandle(),
                       IFACE_IOCTL_REFRESH_DHCP_CONFIG_INFO,
                       NULL);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err 0x%x", result, 0, 0);
  return result;
}

ds::ErrorType Network::GetCurrRFCondition
(
  RFConditionType* pDSRFCondition
)
{
  RFConditionsType     rfCondition;
  int32                result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  if (NULL == pDSRFCondition)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Call the iface IOCTL to get RF conditions.
  -------------------------------------------------------------------------*/
  memset (&rfCondition, 0, sizeof (RFConditionsType));
  result = IfaceIoctl (GetHandle(),
                       IFACE_IOCTL_GET_RF_CONDITIONS,
                       static_cast <void *> (&rfCondition));
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Convert to out args.
  -------------------------------------------------------------------------*/
  *pDSRFCondition = (RFConditionType)rfCondition.rf_conditions;
  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err 0x%x", result, 0, 0);
  return result;
}


ds::ErrorType Network::GetIfaceName
(
  ds::Net::IfaceNameType* pIfaceName
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  if (NULL == pIfaceName)
  {
    LOG_MSG_ERROR ("NULL input", 0, 0, 0);
    return QDS_EFAULT;
  }

  memset (pIfaceName, 0, sizeof (ds::Net::IfaceNameType));

  return IfaceIoctl(GetHandle(),
                               IFACE_IOCTL_GET_IFACE_NAME,
                               (void *)pIfaceName);

} /* GetIfaceName() */


ds::ErrorType Network::GetDeviceInfo
(
  void *argval
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("GetDeviceInfo:Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  if (NULL == argval)
  {
    LOG_MSG_ERROR ("NULL input param", 0, 0, 0);
    return QDS_EFAULT;
  }

  return IfaceIoctl(GetHandle(),
                    IFACE_IOCTL_GET_DEVICE_INFO,
                    (void *)argval);

} /* GetDeviceInfo() */

ds::ErrorType Network::GetInterfaceGwAddr
(
  void *argval
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("GetInterfaceGwAddr:Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  if (NULL == argval)
  {
    LOG_MSG_ERROR ("NULL input param", 0, 0, 0);
    return QDS_EFAULT;
  }

  return IfaceIoctl(GetHandle(),
                    IFACE_IOCTL_GET_INTERFACE_GATEWAY_V4_ADDR,
                    (void *)argval);

} /* GetDeviceInfo() */

ds::ErrorType Network::GetDNSAddr
(
  ::ds::IPAddrType  *pDNSAddrs,
  int               dnsAddrsLen,
  int               *pDNSAddrsLenReq
)
{
  int32             result;
  int               index;
  AllDNSAddrsType   allDNSAddrs;
  int               seqNumItems = 0;
  int               numCopied = 0;
  int32             copyResult = AEE_SUCCESS;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  /*-----------------------------------------------------------------------
    Prepare the IOCTL arguments to get the SIP server domain names.
  -----------------------------------------------------------------------*/
  memset (&allDNSAddrs, 0, sizeof (allDNSAddrs));
  allDNSAddrs.dns_addrs_ptr =
    (ip_addr_type *)
    ps_system_heap_mem_alloc
    (
      sizeof (ip_addr_type) * PS_IFACE_NUM_DNS_ADDRS
    );

  if (NULL == allDNSAddrs.dns_addrs_ptr)
  {
    /*---------------------------------------------------------------------
      Memory configuration is incorrect. This is an ASSERT condition.
    ---------------------------------------------------------------------*/
    ASSERT (0);
    result = AEE_ENOMEMORY;
    goto bail;
  }
  allDNSAddrs.num_dns_addrs = PS_IFACE_NUM_DNS_ADDRS;

  /*-----------------------------------------------------------------------
    Call the iface IOCTL to get all DNS addresses
  -----------------------------------------------------------------------*/
  result = IfaceIoctl (GetHandle(),
                       IFACE_IOCTL_GET_ALL_DNS_ADDRS,
    static_cast <void *> (&allDNSAddrs));
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  result =
    ds::Utils::Conversion::ProcessLenReq
    (
      dnsAddrsLen,
      pDNSAddrsLenReq,
      allDNSAddrs.num_dns_addrs,
      &seqNumItems
    );
  if(AEE_SUCCESS != result)
  {
    goto bail;
  }
  /*-----------------------------------------------------------------------
    Convert to out params.
  -----------------------------------------------------------------------*/
  for (index = 0; index < seqNumItems; index++)
  {
    copyResult = PS2DSIPAddr (&allDNSAddrs.dns_addrs_ptr[index],
                          &pDNSAddrs[numCopied]);
    if(AEE_SUCCESS != copyResult)
    {
      /*-----------------------------------------------------------------------
        do not copy invalid address
      -----------------------------------------------------------------------*/
      continue;
    }

    numCopied++;
  }

  /* Fall through */

bail:
  if(NULL != allDNSAddrs.dns_addrs_ptr)
  {
    PS_SYSTEM_HEAP_MEM_FREE(allDNSAddrs.dns_addrs_ptr);
  }
  LOG_MSG_FUNCTION_EXIT ("Return 0x%x", result, 0, 0);
  return result;

} /* GetDNSAddr() */

ds::ErrorType CDECL Network::GetNetworkStatistics
(
  NetworkStatsType* stats
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return NetPlatform::GetIfaceStats(GetHandle(), stats);
}

ds::ErrorType Network::ResetNetworkStatistics
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return NetPlatform::ResetIfaceStats(GetHandle());
}

ds::ErrorType Network::GetDHCPRefreshResult
(
  boolean* res
)
{
  //TODO: Refresh results are obtained through events.
  //Store this information in the platform layer.
  (void) res;
  return 0;
} /* GetDHCPRefreshResult() */


ds::ErrorType Network::GetTXPhysLink
(
  ::IPhysLink** txPhysLinkObj
)
{
  int32                   result;
  IQoSManager*       pQoSManager = NULL;
  IQoSDefault*       pQoSDefault = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  result = CreateQoSManager (&pQoSManager);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  result = pQoSManager->GetQosDefault (&pQoSDefault);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  *txPhysLinkObj = NULL;
  result = pQoSDefault->GetTXPhysLink(txPhysLinkObj);

bail:
  DS_UTILS_RELEASEIF(pQoSManager);
  DS_UTILS_RELEASEIF(pQoSDefault);

  return result;
}
 /* GetTXPhysLink() */

ds::ErrorType Network::GetRXPhysLink
(
  ::IPhysLink** rxPhysLinkObj
)
{
  int32                   result;
  IQoSManager*       pQoSManager = NULL;
  IQoSDefault*       pQoSDefault = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  result = CreateQoSManager (&pQoSManager);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  result = pQoSManager->GetQosDefault(&pQoSDefault);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  *rxPhysLinkObj = NULL;
  result = pQoSDefault->GetRXPhysLink(rxPhysLinkObj);

bail:
  DS_UTILS_RELEASEIF(pQoSManager);
  DS_UTILS_RELEASEIF(pQoSDefault);

  return result;

} /* GetRXPhysLink() */


/*---------------------------------------------------------------------------
  Inherited function from ds::Utils::INode interface.
---------------------------------------------------------------------------*/
boolean Network::Process
(
  void* userDataPtr
)
{
  boolean                 result;
  EventInfoType*          eventInfoPtr;
  int32                   ifaceHandle;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == userDataPtr)
  {
    LOG_MSG_ERROR ("NULL ev info", 0, 0, 0);
    return FALSE;
  }

  eventInfoPtr = static_cast <EventInfoType *>(userDataPtr);

  ifaceHandle = GetHandle();

  LOG_MSG_INFO1 ("iface handle 0x%x, ev name %d, ev handle 0x%x",
    ifaceHandle, eventInfoPtr->eventName, eventInfoPtr->handle);

  /*-------------------------------------------------------------------------
    Process the event only if:
      It is Network Event
      If this object registered for the event
  -------------------------------------------------------------------------*/
  switch (eventInfoPtr->eventGroup)
  {
    case EVENT_GROUP_NETWORK:
      if (NetworkEvent::QDS_EV_STATE_CHANGED == eventInfoPtr->eventName)
      {
        ProcessIfaceStateEvent (eventInfoPtr);
      }
      else if (ifaceHandle == eventInfoPtr->handle)
      {
        if (NetworkEvent::QDS_EV_OUTAGE == eventInfoPtr->eventName)
        {
          mOutageEventOccurred = TRUE;
        }

        Notify (eventInfoPtr->eventName);
      }
      else if (ifaceHandle == 0 &&
               mStaleIfaceHandle == eventInfoPtr->handle &&
               (NetworkExtEvent::QDS_EV_BEARER_TECH_CHANGED ==
                  eventInfoPtr->eventName ||
                NetworkExtEvent::QDS_EV_QOS_AWARENESS ==
                  eventInfoPtr->eventName))
      {
        LOG_MSG_INFO1("Posting event 0x%x even though handle is 0 because of "
                      "stale handle 0x%x match",
                      eventInfoPtr->eventName, mStaleIfaceHandle, 0);
        Notify (eventInfoPtr->eventName);
      }

      result = TRUE;
      break;

    default:
      result = FALSE;
      break;

  } /* switch */

  return result;

} /* Process() */

ds::ErrorType Network::OnStateChange
(
 ISignal*                                signalObj,
 EventType                               eventID,
 IQI**                                   regObj
 )
{
  AEEPRIVID  privToCheck = AEEPRIVID_PNet;
  boolean bCheck = TRUE;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  switch (eventID)
  {
  case NetworkEvent::QDS_EV_STATE_CHANGED:
  case NetworkEvent::QDS_EV_IP_ADDR_CHANGED:
  case NetworkEvent::QDS_EV_OUTAGE:
  case NetworkEvent::QDS_EV_RF_CONDITIONS_CHANGED:
  case NetworkExtEvent::QDS_EV_QOS_AWARENESS:
  case NetworkExtEvent::QDS_EV_BEARER_TECH_CHANGED:
  case NetworkEvent::QDS_EV_IDLE:
  case NetworkControlEvent::QDS_EV_EXTENDED_IP_CONFIG:
    break;

  default:
    return AEE_EUNSUPPORTED;
  }

  /*-------------------------------------------------------------------------
  check client provided privileges according to eventID
  -------------------------------------------------------------------------*/
  switch (eventID)
  {
  case NetworkExtEvent::QDS_EV_QOS_AWARENESS:
    privToCheck = AEEPRIVID_PQoS;
    break;
    /*-------------------------------------------------------------------------
    call can be made via Monitored Network, so check PNet privilege
    -------------------------------------------------------------------------*/
  case NetworkControlEvent::QDS_EV_EXTENDED_IP_CONFIG:
    privToCheck = AEEPRIVID_PNet;
    break;
  default:
    bCheck = FALSE;
    break;
  }

  if(TRUE == bCheck)
  {
    if (AEE_SUCCESS != mpPrivSet->CheckPrivileges
      (
      (AEEPRIVID *)&privToCheck,
      sizeof(privToCheck)
      ))
    {
      LOG_MSG_ERROR( "No privilege for server bind operations require %0x priv",
        privToCheck, 0, 0);
      return AEE_EPRIVLEVEL;
    }
  }

  if (NetworkExtEvent::QDS_EV_BEARER_TECH_CHANGED == eventID ||
      (NetworkEvent::QDS_EV_STATE_CHANGED == eventID && TRUE == mBringupFirst))
  {
     /* Set the signal and let APP query for network state */
     (void) signalObj->AddRef();
     (void) signalObj->Set();
     (void) signalObj->Release();
  }

  return Handle::OnStateChange(signalObj, eventID, regObj);
} /* OnStateChange() */

/*---------------------------------------------------------------------------
  Methods from INetworkPriv interface.
---------------------------------------------------------------------------*/
ds::ErrorType Network::GetIfaceId
(
  IfaceIdType* pIfaceId
)
{
  int32 handle;
  int32 result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  if (NULL == pIfaceId)
  {
    return QDS_EFAULT;
  }

  handle = GetHandle();

  if (0 == handle)
  {
    /* Network lookup/bringup has not happened. Perform network lookup */
    result = this->LookupInterface();
    if (AEE_SUCCESS != result)
    {
      goto bail;
    }

    handle = GetHandle();
  }


  result = PSGetIfaceId (handle, (int32 *)pIfaceId);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  LOG_MSG_INFO1 ("handle 0x%x, id 0x%x", handle, *pIfaceId, 0);
  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err 0x%x", result, 0, 0);
  return result;
}

ds::ErrorType Network::GetPreviousState
(
  NetworkStateType* argNetState
)
{
  int32                  ifaceHandle;
  IfaceStateEnumType     ifaceState;
  int                    result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  if (NULL == argNetState)
  {
    return QDS_EFAULT;
  }

  ifaceHandle = GetHandle();
  if (0 == ifaceHandle)
  {
    *argNetState = mCachedPreviousState;
    return AEE_SUCCESS;
  }

  /*-------------------------------------------------------------------------
    Call network platform IOCTL to get the interface state.
  -------------------------------------------------------------------------*/
  result = IfaceIoctl (ifaceHandle,
                       IFACE_IOCTL_GET_PREVIOUS_STATE,
                       &ifaceState);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Map Iface state to network state.
  -------------------------------------------------------------------------*/
  result = PS2DSIfaceState(ifaceState,argNetState);
  if (AEE_SUCCESS != result)
  {
     goto bail;
  }

  LOG_MSG_INFO1 ("netstatus is %d", *argNetState, 0, 0);
  return result;

bail:
  LOG_MSG_ERROR ("Invalid Iface state %d", ifaceState, 0, 0);
  return result;

}

ds::ErrorType Network::GetState
(
  NetworkStateType* argNetState
)
{
  int32                  ifaceHandle;
  boolean                closeInProgress;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == argNetState)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    If the handle is not set in the network object, return QDS_CLOSED.
  -------------------------------------------------------------------------*/
  ifaceHandle = GetHandle();
  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, ifaceHandle, 0);

  if (0 == ifaceHandle)
  {
    *argNetState = NetworkState::QDS_CLOSED;
    return AEE_SUCCESS;
  }

  /*-------------------------------------------------------------------------
    If the bring up flag is set, return QDS_OPEN_IN_PROGRESS
  -------------------------------------------------------------------------*/
  mpICritSect->Enter();
  closeInProgress = mBringupAgain;
  mpICritSect->Leave();

  if (TRUE == closeInProgress)
  {
    *argNetState = NetworkState::QDS_OPEN_IN_PROGRESS;
    return AEE_SUCCESS;
  }

  /*-------------------------------------------------------------------------
    Otherwise, return the associated iface state.
  -------------------------------------------------------------------------*/
  return GetIfaceState (argNetState);
} /* GetState() */


ds::ErrorType Network::GetPreviousIPAddr
(
  ds::IPAddrType* argIPAddress
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return IPAddrIOCTL (IFACE_IOCTL_GET_PREVIOUS_IP_ADDR, argIPAddress);

} /* GetPreviousIPAddr() */


ds::ErrorType Network::GetPreviousBearerInfo
(
  IBearerInfo**  ppIDSNetBearerInfo
)
{
  int32                    result;
  int32                    ifaceHandle;
  BearerTechType           bearerTechInfo;
  ds::Net::IfaceNameType   ifaceName;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  if (NULL == ppIDSNetBearerInfo)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    HACK to make event propagation work even after call goes down
  -------------------------------------------------------------------------*/
  ifaceHandle = GetHandle();
  if (0 == ifaceHandle && 0 != mStaleIfaceHandle)
  {
    ifaceHandle = mStaleIfaceHandle;
    LOG_MSG_INFO1("Using stale handle 0x%x for obj 0x%p",
                  mStaleIfaceHandle, this, 0);
  }

  *ppIDSNetBearerInfo = NULL;
  memset (&bearerTechInfo, 0, sizeof (BearerTechType));

  /*-------------------------------------------------------------------------
    Call the iface IOCTL to get bearer technology.
  -------------------------------------------------------------------------*/
  result = IfaceIoctl (ifaceHandle,
                       IFACE_IOCTL_GET_PREVIOUS_BEARER_TECHNOLOGY,
                       static_cast <void *> (&bearerTechInfo));
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Convert the network type to iface name.
  -----------------------------------------------------------------------*/
  switch (bearerTechInfo.current_network)
  {
    case PS_IFACE_NETWORK_CDMA:
      ifaceName = IfaceName::IFACE_CDMA_SN;
      break;
    case PS_IFACE_NETWORK_UMTS:
      ifaceName = IfaceName::IFACE_UMTS;
      break;
    case PS_IFACE_NETWORK_WLAN:
      ifaceName = IfaceName::IFACE_WLAN;
      break;
    default:
       result = QDS_INTERNAL;
       goto bail;
  }

  /*-----------------------------------------------------------------------
    Construct the Bearer tech object using the IOCTL result.
  -----------------------------------------------------------------------*/
  *ppIDSNetBearerInfo = static_cast <IBearerInfo *>
    (new BearerTech (ifaceName,
                     bearerTechInfo.data.cdma_type.rat_mask,
                     bearerTechInfo.data.cdma_type.so_mask,
                     bearerTechInfo.data.umts_type.rat_mask,
                     NULL));
  if (NULL == *ppIDSNetBearerInfo)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err 0x%x", result, 0, 0);
  return result;

} /* GetPreviousBearerInfo() */

ds::ErrorType Network::GetQoSAwareInfoCode
(
  QoSInfoCodeType* infoCode
)
{
  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);
  return IfaceIoctlNonNullArg (GetHandle(),
                               IFACE_IOCTL_GET_QOS_AWARE_INFO_CODE,
                               static_cast <void *> (infoCode));
} /* GetQoSAwareInfoCode() */


ds::ErrorType Network::GetAllIfaces
(
  IfaceIdType *ifaceIdArr,
  int         ifaceIdArrLen,
  int         *ifaceIdArrLenReq
)
{
  int                        result;
  PSAllIfacesType            allIfacesInfo;
  int                        index;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  memset (&allIfacesInfo, 0, sizeof(allIfacesInfo));

  /*-----------------------------------------------------------------------
    Call the IOCTL.
  -----------------------------------------------------------------------*/
  result = IfaceIoctl (GetHandle(),
                       IFACE_IOCTL_GET_ALL_IFACES,
                       (void *) &allIfacesInfo);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  if(NULL != ifaceIdArrLenReq)
  {
    *ifaceIdArrLenReq = (int)allIfacesInfo.number_of_ifaces;
  }
  if (NULL == ifaceIdArr)
  {
    if(0 != ifaceIdArrLen)
    {
      result = QDS_EFAULT;
      goto bail;
    }
    return AEE_SUCCESS;
  }
  if(0 == ifaceIdArrLen)
  {
    return AEE_SUCCESS;
  }
  /*-----------------------------------------------------------------------
    Convert to out params.
  -----------------------------------------------------------------------*/
  for (index = 0;
       index < (int)allIfacesInfo.number_of_ifaces && index < ifaceIdArrLen;
       index++)
  {
    ifaceIdArr[index] = allIfacesInfo.ifaces[index];
  } /* for */

  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err 0x%x", result, 0, 0);
  return result;
} /* GetAllIfaces() */


ds::ErrorType Network::QueryInterface
(
  AEEIID iid,
  void **ppo
)
{
  int result = AEE_SUCCESS;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1  ("Obj 0x%p, ref cnt %d, iid 0x%x", this, refCnt, iid);

  if (NULL == ppo)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  *ppo = NULL;

  switch (iid)
  {
  case AEEIID_INetwork:
    *ppo = static_cast <INetwork *> (this);
    (void) AddRef();
    break;

  case AEEIID_INetworkPriv:
    *ppo = static_cast <INetworkPriv *>(this);
    (void) AddRef();
    break;

  case AEEIID_INetworkControl:
     *ppo = static_cast <INetworkControl *>(this);
     (void) AddRef();
     break;

  case AEEIID_INetworkExt:
     *ppo = static_cast <INetworkExt *>(this);
     (void) AddRef();
     break;

  case AEEIID_INetworkExt2:
    *ppo = static_cast <INetworkExt2 *>(this);
    (void) AddRef();
    break;

  case AEEIID_IQI:
    *ppo = reinterpret_cast <IQI *> (this);
    (void) AddRef();
    break;

  case AEEIID_IIPFilterManagerPriv:
    *ppo = static_cast <IIPFilterManagerPriv *> (this);
    (void) AddRef();
    break;

  case AEEIID_IQoSManager:
    result = CreateQoSManager (
      reinterpret_cast <IQoSManager **>(ppo));
    break;

  case AEEIID_IMCastManager:
    result = CreateMCastManager (
      reinterpret_cast <IMCastManager **>(ppo));
    break;

  case AEEIID_IFirewallManager:
    result = CreateNetFirewallManager (
      reinterpret_cast <IFirewallManager **>(ppo));
    break;

  case AEEIID_INatSession:
    result = CreateNetNatSession (
      reinterpret_cast <INatSession **>(ppo));
    break;

  case AEEIID_INetwork1x:
  case AEEIID_INetwork1xPriv:
  case AEEIID_INetworkUMTS:
  case AEEIID_INetworkIPv6:
  case AEEIID_INetworkIPv6Priv:
    (void) GetTechObject(iid, (void**)ppo);
    if (NULL == *ppo)
    {
      return AEE_ECLASSNOTSUPPORT;
    }
    break;

  default:
    return AEE_ECLASSNOTSUPPORT;
  }

  return result;
}

ds::ErrorType Network::CreateNetFirewallManager
(
  IFirewallManager**   ppIFirewallMgr
)
{
  int32                ifaceHandle;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
#ifndef FEATURE_DSS_LINUX
  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  if (NULL == ppIFirewallMgr)
  {
    LOG_MSG_ERROR ("NULL arg", 0, 0, 0);
    return QDS_EFAULT;
  }

  ifaceHandle = GetHandle();
  if (0 == ifaceHandle)
  {
    LOG_MSG_ERROR ("Iface handle is 0", 0, 0, 0);
    return QDS_EINVAL;
  }

  /*-------------------------------------------------------------------------
    Create a new FirewallManager object all the time. If the network object's
    handle changes anytime, old FirewallManager does not make sense.
  -------------------------------------------------------------------------*/
  *ppIFirewallMgr = static_cast<IFirewallManager *>
                    (new FirewallManager (ifaceHandle));
  if (NULL == *ppIFirewallMgr)
  {
    return AEE_ENOMEMORY;
  }

  return AEE_SUCCESS;
#else
  LOG_MSG_ERROR ("Firewall Manager is not supported yet", 0, 0, 0);
  return AEE_EFAILED;
#endif

} /* CreateNetFirewallManager() */

ds::ErrorType Network::CreateNetNatSession
(
  INatSession**   ppINatSession
)
{
  int32                ifaceHandle;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  if (NULL == ppINatSession)
  {
    LOG_MSG_ERROR ("NULL arg", 0, 0, 0);
    return QDS_EFAULT;
  }

  ifaceHandle = GetHandle();
  if (0 == ifaceHandle)
  {
    LOG_MSG_ERROR ("Iface handle is 0", 0, 0, 0);
    return QDS_EINVAL;
  }

  /*-------------------------------------------------------------------------
    Create a new NatSession object all the time. If the network object's
    handle changes anytime, old NatSession does not make sense.
  -------------------------------------------------------------------------*/
  *ppINatSession = static_cast<INatSession *>
                    (new NatSession (ifaceHandle));
  if (NULL == *ppINatSession)
  {
    return AEE_ENOMEMORY;
  }

  return AEE_SUCCESS;
} /* CreateNetNatSession() */

ds::ErrorType Network::GetNetworkIPv6
(
  AEEIID iid,
  void** ppo
)
{
#ifdef FEATURE_DATA_PS_IPV6
  int32 ifaceHandle;
  int32 result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  if (NULL == ppo)
  {
    return QDS_EFAULT;
  }

  if (AEEIID_INetworkIPv6 != iid &&
      AEEIID_INetworkIPv6Priv != iid)
  {
    return AEE_ECLASSNOTSUPPORT;
  }


  if (NULL == mpNetworkIPv6)
  {
    ifaceHandle = GetHandle();
    mpNetworkIPv6 = new NetworkIPv6(ifaceHandle, mpPrivSet);
    if (NULL == mpNetworkIPv6)
    {
      return AEE_ENOMEMORY;
    }

    result = mpNetworkIPv6->Init();
    if (AEE_SUCCESS != result)
    {
      DS_UTILS_RELEASEIF(mpNetworkIPv6);
      return result;
    }
  }

  /*-------------------------------------------------------------------------
    We are returning an interface. Perform AddRef()
  -------------------------------------------------------------------------*/
  (void) mpNetworkIPv6->AddRef();

  if (AEEIID_INetworkIPv6 == iid)
  {
    *ppo = static_cast <INetworkIPv6 *> (mpNetworkIPv6);
  }
  else
  {
    *ppo = static_cast <INetworkIPv6Priv *> (mpNetworkIPv6);
  }

  return AEE_SUCCESS;

#else
  return AEE_ECLASSNOTSUPPORT;
#endif /* FEATURE_DATA_PS_IPV6 */

} /* GetNetworkIPv6() */

ds::ErrorType Network::GetNetworkUMTS
(
  AEEIID iid,
  void** ppo
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  if (0 == ppo)
  {
    return QDS_EFAULT;
  }

  if (AEEIID_INetworkUMTS != iid)
  {
    return AEE_ECLASSNOTSUPPORT;
  }


  if (0 == mpNetworkUMTS)
  {
    mpNetworkUMTS = new NetworkUMTS(this);
    if (0 == mpNetworkUMTS)
    {
      return AEE_ENOMEMORY;
    }
  }

  /*-------------------------------------------------------------------------
  We are returning an interface. Perform AddRef()
  -------------------------------------------------------------------------*/
  (void) mpNetworkUMTS->AddRef();

  *ppo = static_cast <INetworkUMTS *> (mpNetworkUMTS);

  return AEE_SUCCESS;
}

ds::ErrorType Network::GetTechObject
(
  AEEIID iid,
  void** ppo
)
{
  int32                         result;
  ds::Net::IfaceNameType        ifaceName;
  Network1X*                    pNetwork1x;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  if (NULL == ppo)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  if (AEEIID_INetworkIPv6 == iid ||
      AEEIID_INetworkIPv6Priv == iid)
  {
    return GetNetworkIPv6 (iid, (void**)ppo);
  }

  /*-------------------------------------------------------------------------
    Find out the iface name. Depending on the iface name, we create the
    technology object. Store the technology object and its IID.
  -------------------------------------------------------------------------*/
  result = GetIfaceName(&ifaceName);
  if (AEE_SUCCESS != result)
  {
    LOG_MSG_ERROR ("Err 0x%x getting iface name", result, 0, 0);
    return result;
  }

  if((IfaceName::IFACE_UMTS == ifaceName ||
      IfaceName::IFACE_STA  == ifaceName) &&
      (AEEIID_INetworkUMTS == iid))
  {
    return GetNetworkUMTS (iid, (void**)ppo);
  }

  /*-------------------------------------------------------------------------
    If technology object is already set, return the same.
  -------------------------------------------------------------------------*/
  *ppo = NULL;
  if (NULL != mpTechObjHandle)
  {
    switch (iid)
    {
      case AEEIID_INetwork1x:
        *ppo = mpTechObjNetwork1x;
        break;

      case AEEIID_INetwork1xPriv:
        *ppo = mpTechObjNetwork1xPriv;
        break;

      default:
        ASSERT (0);
        return AEE_ECLASSNOTSUPPORT;
    }
    (void) mpTechObjHandle->AddRef();
    return AEE_SUCCESS;
  }

  if ((IfaceName::IFACE_CDMA_AN == ifaceName ||
       IfaceName::IFACE_CDMA_SN == ifaceName ||
       IfaceName::IFACE_STA     == ifaceName ||
       IfaceName::IFACE_CDMA_BCAST == ifaceName) &&
      (AEEIID_INetwork1x  == iid ||
       AEEIID_INetwork1xPriv == iid))
  {

    pNetwork1x = new Network1X (this);
    if (NULL == pNetwork1x)
    {
      LOG_MSG_ERROR("Out of mem creating network 1x from 0x%p", this, 0, 0);
      return AEE_ENOMEMORY;
    }

    result = pNetwork1x->Init();
    if (AEE_SUCCESS != result)
    {
      DS_UTILS_RELEASEIF(pNetwork1x);
      return result;
    }

    mpTechObjNetwork1x = static_cast <INetwork1x *> (pNetwork1x);
    mpTechObjNetwork1xPriv = static_cast <INetwork1xPriv *> (pNetwork1x);
    mpTechObjHandle = static_cast <Handle *>(pNetwork1x);

    if (AEEIID_INetwork1x == iid)
    {
      *ppo = mpTechObjNetwork1x;
    }
    else
    {
      *ppo = mpTechObjNetwork1xPriv;
    }
  }

  if (NULL == mpTechObjHandle)
  {
    return AEE_ECLASSNOTSUPPORT;
  }

  (void) mpTechObjHandle->AddRef();
  return AEE_SUCCESS;

} /* GetTechObject() */

/*---------------------------------------------------------------------------
  Inherited functions from IIPFilterMgr.
---------------------------------------------------------------------------*/
ds::ErrorType Network::RegisterFilters
(
  int fi_result,
  IIPFilterPriv** ppIIPFilterSpec,
  int filtersLen,
  IIPFilterRegPriv **ppIIPFfilterReg
)
{
  int32                       result;
  int32                       ifaceHandle = 0;
  int                         index;
  PSIfaceIPFilterAddParamType filterAddParam;
  bool                        deregCleanup = false;
  int32                       fltrHandle = 0 ;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  memset (&filterAddParam, 0, sizeof(filterAddParam));


  /*-------------------------------------------------------------------------
    Validation.
  -------------------------------------------------------------------------*/
  if (NULL == ppIIPFilterSpec || 0 >= filtersLen)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  mpICritSect->Enter();
  /*-------------------------------------------------------------------------
    If filters are already installed, uninstall them.
  -------------------------------------------------------------------------*/
  DS_UTILS_RELEASEIF(mpIPFilterReg);

  mpICritSect->Leave();

  ifaceHandle = GetHandle();

  /*-------------------------------------------------------------------------
    Allocate memory to hold array to ip_filter_type
  -------------------------------------------------------------------------*/
  filterAddParam.fi_ptr_arr =
    ps_system_heap_mem_alloc
    (
      sizeof (ip_filter_type) * filtersLen
    );
  if  (NULL == filterAddParam.fi_ptr_arr)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  filterAddParam.enable = TRUE;
  filterAddParam.is_validated = FALSE;
  filterAddParam.num_filters = (uint8) filtersLen;
  filterAddParam.subset_id = 0;
  filterAddParam.fi_result = fi_result;
  filterAddParam.filter_type = IPFLTR_DEFAULT_TYPE;
  filterAddParam.fltr_compare_f_ptr = NULL;
  filterAddParam.fltr_priority = PS_IFACE_IPFLTR_PRIORITY_DEFAULT;

  for (index = 0; index < filtersLen; index++)
  {
    (void) DS2PSIPFilterSpec
    (
      ppIIPFilterSpec[index],
      &(((ip_filter_type *)filterAddParam.fi_ptr_arr)[index])
    );
  }

  result = PSIfaceIPFilterAdd (ifaceHandle,
                               IP_FLTR_CLIENT_SOCKETS,
                               &filterAddParam,
                               &fltrHandle);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  if (PS_IFACE_IPFLTR_INVALID_HANDLE == fltrHandle)
  {
    result = QDS_EINVAL;
    goto bail;
  }

  mpICritSect->Enter();
  mpIPFilterReg = new IPFilterReg (ifaceHandle, fltrHandle);
  mpICritSect->Leave();

  if (NULL == mpIPFilterReg)
  {
    deregCleanup = true;
    result = AEE_ENOMEMORY;
    goto bail;
  }

  *ppIIPFfilterReg = static_cast <IIPFilterRegPriv *> (mpIPFilterReg);
  (void) mpIPFilterReg->AddRef();
  PS_SYSTEM_HEAP_MEM_FREE (filterAddParam.fi_ptr_arr);
  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err 0x%x installing filters", result, 0, 0);

  DS_UTILS_RELEASEIF(mpIPFilterReg);

  if (NULL != filterAddParam.fi_ptr_arr)
  {
    PS_SYSTEM_HEAP_MEM_FREE (filterAddParam.fi_ptr_arr);
  }

  if (deregCleanup)
  {
    LOG_MSG_INFO1 ("Uninstalling fltr 0x%x, iface 0x%x",
      fltrHandle, ifaceHandle, 0);

    (void) PSIfaceIPFilterDelete (ifaceHandle,
                                  IP_FLTR_CLIENT_SOCKETS,
                                  fltrHandle);
  }
  return result;

} /* RegisterFilters() */

ds::ErrorType CDECL Network::GetQoSMode
(
   QoSModeType* qosMode
)
{
  /*-------------------------------------------------------------------------
    currently this API is not supported
  -------------------------------------------------------------------------*/

  return AEE_EUNSUPPORTED;
}

ds::ErrorType CDECL Network::GetDormancyInfoCode
(
  DormancyInfoCodeType *pDormancyInfoCode
)
{
  PhysLink*           pPhysLink = NULL;
  IPhysLink*          pIPhysLink = NULL;
  ds::ErrorType       result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  result = GetTXPhysLink (&pIPhysLink);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  pPhysLink = reinterpret_cast <PhysLink *> (pIPhysLink);

  /*-------------------------------------------------------------------------
    Get Dormancy info code from phys link.
  -------------------------------------------------------------------------*/
  result = pPhysLink->GetDormancyInfoCode (pDormancyInfoCode);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  (void) pIPhysLink->Release();
  return AEE_SUCCESS;

bail:

  if (NULL != pIPhysLink)
  {
    (void) pIPhysLink->Release();
  }

  LOG_MSG_ERROR ("Err 0x%x", result, 0, 0);
  return result;
}

ds::ErrorType Network::GetSignalBus
(
  ds::Net::EventType  eventID,
  ISignalBus **       ppISigBus
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == ppISigBus)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    ASSERT (0);
    return QDS_EFAULT;
  }

  switch (eventID)
  {
    case  NetworkEvent::QDS_EV_STATE_CHANGED:
      *ppISigBus = mpSigBusStateChange;
      break;

    case  NetworkExtEvent::QDS_EV_BEARER_TECH_CHANGED:
      *ppISigBus = mpSigBusBearerTech;
      break;

    case  NetworkEvent::QDS_EV_IP_ADDR_CHANGED:
      *ppISigBus = mpSigBusIPAddr;
      break;

    case  NetworkEvent::QDS_EV_OUTAGE:
      *ppISigBus = mpSigBusOutage;
      break;

    case  NetworkExtEvent::QDS_EV_QOS_AWARENESS:
      *ppISigBus = mpSigBusQoSAware;
      break;

    case  NetworkEvent::QDS_EV_RF_CONDITIONS_CHANGED:
      *ppISigBus = mpSigBusRFConditions;
      break;

    case  NetworkControlEvent::QDS_EV_EXTENDED_IP_CONFIG:
      *ppISigBus = mpSigBusExtendedIPConfig;
      break;

    case  NetworkEvent::QDS_EV_IDLE:
      *ppISigBus = mpSigBusIdle;
      break;

    default:
      *ppISigBus = NULL;
      return QDS_EINVAL;
  }

  (void)(*ppISigBus)->AddRef();
  return AEE_SUCCESS;

} /* GetSignalBus() */

IPrivSet* Network::GetPrivSet
(
  void
)
{
  if(0 != mpPrivSet)
  {
    (void) mpPrivSet->AddRef();
  }

  return mpPrivSet;
}/* GetPrivSet() */

ds::ErrorType Network::GoNull
(
  ReleaseReasonType nullReason
)
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  /*-------------------------------------------------------------------------
    check client provided privileges
  -------------------------------------------------------------------------*/
  if (AEE_SUCCESS != mpPrivSet->CheckPrivileges((AEEPRIVID *)&AEEPRIVID_PGoNull,
                                                 sizeof(AEEPRIVID_PGoNull)))
  {
    LOG_MSG_ERROR( "No privilege for GoNull operation", 0, 0, 0);
    return AEE_EPRIVLEVEL;
  }

  return IfaceIoctlNonNullArg (GetHandle(),
                               IFACE_IOCTL_GO_NULL,
                               (void *)&nullReason);
} /* GoNull() */


void Network::SetHandle
(
  int32 objHandle
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2 ("Obj 0x%p, handle=0x%x", this, objHandle, 0);

  mpICritSect->Enter();

  /*-----------------------------------------------------------------------
    Reset mBringupAgain, mBringupFirst, mTeardown flags.
  -----------------------------------------------------------------------*/
  if (0 == objHandle)
  {
    mBringupFirst = FALSE;
    mBringupAgain = FALSE;
    mTeardown     = FALSE;
  }

  mpICritSect->Leave();

  /*-----------------------------------------------------------------------
    Call parent method.
  -----------------------------------------------------------------------*/
  Handle::SetHandle(objHandle);
  return;

} /* SetHandle() */

ds::ErrorType Network::SetFMCTunnelParams
(
  const FMCTunnelParamsType* tunnelParams
)
{
  NetPlatform::PSFMCTunnelParamsType psTunnelParams;
  ds::SockAddrInternalType               tempAddr;
  ds::SockAddrIN6InternalType            v6RemoteAddr;
  ds::ErrorType result = AEE_SUCCESS;
  uint16        psFamily = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  memset(&psTunnelParams, 0, sizeof(NetPlatform::PSFMCTunnelParamsType));

  /* convert to PS socket address */
  memcpy((void*)&tempAddr,(void*)tunnelParams->tunnelEndPointAddr,sizeof(ds::SockAddrStorageType));

  tempAddr.v4.family = ps_ntohs(tempAddr.v4.family);

  switch(tempAddr.v4.family)
  {
  case AddrFamily::QDS_AF_INET:
    result = DS2PSSockAddrFamily(tempAddr.v4.family, &psFamily);
    if(AEE_SUCCESS != result)
    {
      LOG_MSG_ERROR("wrong address family : %d", tempAddr.v4.family, 0, 0);
      goto bail;
    }
    tempAddr.v4.family = psFamily;
    psTunnelParams.tunnel_end_point_info_ptr =
      reinterpret_cast<ps_sockaddr *>(&tempAddr);
    break;
  case AddrFamily::QDS_AF_INET6:
    memcpy((void*)&v6RemoteAddr, (void*)tunnelParams->tunnelEndPointAddr, sizeof(ds::SockAddrIN6InternalType));
    v6RemoteAddr.family = ps_ntohs(v6RemoteAddr.family);
    result = DS2PSSockAddrFamily(v6RemoteAddr.family, &psFamily);
    if(AEE_SUCCESS != result)
    {
      LOG_MSG_ERROR("wrong address family : %d", v6RemoteAddr.family, 0, 0);
      goto bail;
    }
    v6RemoteAddr.family = psFamily;
    psTunnelParams.tunnel_end_point_info_ptr =
      reinterpret_cast<ps_sockaddr *>(&v6RemoteAddr);
    break;
  case AddrFamily::QDS_AF_UNSPEC:
  default:
    LOG_MSG_ERROR("wrong address family : %d", tempAddr.v4.family, 0, 0);
    goto bail;
  }

  psTunnelParams.addr_len = tunnelParams->addrLen;
  psTunnelParams.is_nat_present = tunnelParams->IsNatPresent;
  psTunnelParams.stream_id = tunnelParams->streamId;

  result = IfaceIoctlNonNullArg (GetHandle(),
                                 IFACE_IOCTL_UW_FMC_SET_TUNNEL_PARAMS,
                                 (void *)&psTunnelParams);

/* fall through */

bail:

  LOG_MSG_FUNCTION_EXIT ("Return 0x%x", result, 0, 0);

  return result;

} /* SetFMCTunnelParams() */

ds::ErrorType Network::ResetFMCTunnelParams
(
  void
)
{

  /*-------------------------------------------------------------------------
    iface ioctl implementation validates ioctl argument to be not NULL,
    but afterwards does not use it in any way, so introducing dummy variable
    just to pass ioctl implementation check
  -------------------------------------------------------------------------*/
  int dummy = 0;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  return IfaceIoctl(GetHandle(),
                    IFACE_IOCTL_UW_FMC_RESET_TUNNEL_PARAMS,
                    (void *)&dummy);

}/* ResetFMCTunnelParams() */
