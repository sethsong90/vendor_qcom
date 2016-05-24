/*===========================================================================
  FILE: NetworkIPv6.cpp

  OVERVIEW: This file provides implementation of the NetworkIPv6 class.

  DEPENDENCIES: None

  Copyright (c) 2007 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_NetworkIPv6.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2009-02-27 hm  Added IPv6 priv addr and prefixes support.
  2008-04-07 hm  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "ds_Utils_DebugMsg.h"
#include "ds_Utils_Conversion.h"
#include "AEEStdErr.h"
#include "ds_Net_NetworkIPv6.h"
#include "ds_Net_Platform.h"
#include "ds_Net_EventDefs.h"
#include "ds_Net_EventManager.h"
#include "ds_Net_NetworkIPv6Address.h"
#include "ds_Net_Conversion.h"
#include "ds_Utils_CreateInstance.h"
#include "AEECSignalBus.h"
#include "ps_ip6_addr.h"
#include "ds_Net_Privileges_Def.h"
#include "AEEIPrivSet.h"
#include "ps_system_heap.h"

using namespace ds::Net;
using namespace ds::Error;
using namespace ds::Net::Conversion;
using namespace NetPlatform;

/*===========================================================================

                     PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
/*---------------------------------------------------------------------------
  CONSTRUCTOR/DESTRUCTOR
---------------------------------------------------------------------------*/
NetworkIPv6::NetworkIPv6
(
  int32      ifaceHandle,
  IPrivSet * pPrivSet
)
: Handle (ifaceHandle),
  mpSigBusPrefixChanged (0),
  mpPrivSet(pPrivSet),
  refCnt (1),
  weakRefCnt (1)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Creating object 0x%p", this, 0, 0);

  ASSERT(0 != mpPrivSet);

  (void) mpPrivSet->AddRef();

} /* NetworkIPv6::NetworkIPv6() */

ds::ErrorType NetworkIPv6::Init
(
  void
)
{
  int32  result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Init object 0x%p", this, 0, 0);

  result = DS_Utils_CreateInstance(0, AEECLSID_CSignalBus,
                                   0, (void **) &mpSigBusPrefixChanged);

  Handle::Init(EventManager::networkIPv6ObjList);

  return result;

} /* NetworkIPv6::Init() */

void NetworkIPv6::Destructor
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);

  DS_UTILS_RELEASEIF (mpPrivSet);

  DS_UTILS_RELEASEIF (mpSigBusPrefixChanged);

  //Should be last statement. Call destructor for the base class.
  Handle::Destructor();

} /* NetworkIPv6::Destructor() */

NetworkIPv6::~NetworkIPv6
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  mpPrivSet = NULL;

} /* NetworkIPv6::~NetworkIPv6() */

/*---------------------------------------------------------------------------
  Functions inherited from IQI interface.
---------------------------------------------------------------------------*/
ds::ErrorType NetworkIPv6::QueryInterface
(
  AEEIID iid,
  void **ppo
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1  ("Obj 0x%p, ref cnt %d", this, refCnt, 0);


  if (NULL == ppo)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  *ppo = NULL;

  switch (iid)
  {
    case AEEIID_INetworkIPv6:
      *ppo = static_cast <INetworkIPv6 *> (this);
      (void) AddRef();
      break;

    case AEEIID_INetworkIPv6Priv:
    *ppo = static_cast <INetworkIPv6Priv *>(this);
    (void) AddRef();
    break;

    case AEEIID_IQI:
      *ppo = reinterpret_cast <IQI *> (this);
      (void) AddRef();
      break;

    default:
      *ppo = NULL;
      return AEE_ECLASSNOTSUPPORT;

  } /* switch */

  return AEE_SUCCESS;

} /* QueryInterface() */


/*---------------------------------------------------------------------------
  Forwarder functions for Handle object.
---------------------------------------------------------------------------*/
ds::ErrorType NetworkIPv6::OnStateChange
(
  ISignal*                                signalObj,
  ds::Net::EventType                      eventID,
  IQI**                                   regObj
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1  ("Obj 0x%p, ev %d, sig 0x%p",
    this, eventID, signalObj);

  if (IPv6PrivEvent::QDS_EV_PREFIX_CHANGED == eventID)
  {
    return Handle::OnStateChange(signalObj, eventID, regObj);
  }

  return QDS_EINVAL;

} /* OnStateChange() */

ds::ErrorType NetworkIPv6::GeneratePrivAddr
(
  boolean        isUnique,
  IIPv6Address** ppIPv6PrivAddr
)
{
  ds::ErrorType               result;
  ds::ErrorType               initResult;
  int32                       ifaceHandle;
  IPv6PrivAddrIoctlArgType    ioctlArg;
  PSIPAddrType                psPrivIPv6Addr;
  ds::INAddr6Type             dsPrivIPv6Addr = {0};
  NetworkIPv6Address *        pNetIPv6Addr; /*lint -esym(429,pNetIPv6Addr) */
  IPv6AddrStateType           AddrState;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  if (NULL == ppIPv6PrivAddr)
  {
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    check client provided privileges
  -------------------------------------------------------------------------*/
  if (AEE_SUCCESS != mpPrivSet->CheckPrivileges
                                (
                                  (AEEPRIVID *)&AEEPRIVID_PPrivateAddr,
                                  sizeof(AEEPRIVID_PPrivateAddr)
                                ))
  {
    LOG_MSG_ERROR( "No privilege for GeneratePrivAddr operation", 0, 0, 0);
    return AEE_EPRIVLEVEL;
  }


  /*-------------------------------------------------------------------------
    Call the IOCTL to generate IPv6 Priv Addr
  -------------------------------------------------------------------------*/
  ifaceHandle = GetHandle();

  memset (&ioctlArg, 0, sizeof (ioctlArg));
  memset (&psPrivIPv6Addr, 0, sizeof (psPrivIPv6Addr));

  /* Caution: assiging the pointer to a local stack variable */
  ioctlArg.ip_addr = (ip_addr_type *) &psPrivIPv6Addr;
  ioctlArg.iid_params.is_unique = isUnique;

  result = IfaceIoctl (ifaceHandle,
                       IFACE_IOCTL_GENERATE_PRIV_IPV6_ADDR,
                       static_cast <void *> (&ioctlArg));

  /*-------------------------------------------------------------------------
    The psPrivIPv6Addr var would now contain the new IPv6 address generated.
  -------------------------------------------------------------------------*/
  if (AEE_SUCCESS == result)
  {
    /*-----------------------------------------------------------------------
      SUCCESS means an address is AVAILABLE.
    -----------------------------------------------------------------------*/
    (void) PS2DSIPAddr (&psPrivIPv6Addr, dsPrivIPv6Addr);
    AddrState = IPv6AddrState::PRIV_ADDR_AVAILABLE;
  }
  else if (AEE_EWOULDBLOCK == result)
  {
    /*-----------------------------------------------------------------------
      AEE_EWOULDBLOCK means we are WAITING for an address.
    -----------------------------------------------------------------------*/
    AddrState = IPv6AddrState::PRIV_ADDR_WAITING;
  }
  else
  {
    /*-----------------------------------------------------------------------
      Priv addr generation failed for some other reason.
      Return error to application.
    -----------------------------------------------------------------------*/
    LOG_MSG_ERROR ("Priv addr gen failed, err 0x%x", result, 0, 0);
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Generate a NetworkIPv6Addr object in the correct state.
  -------------------------------------------------------------------------*/
  pNetIPv6Addr = new NetworkIPv6Address (dsPrivIPv6Addr,
                                         AddrState,
                                         ifaceHandle);
  if (NULL == pNetIPv6Addr)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  initResult = pNetIPv6Addr->Init();
  if (AEE_SUCCESS != initResult)
  {
    result = initResult;
    DS_UTILS_RELEASEIF(pNetIPv6Addr);
    goto bail;
  }

  *ppIPv6PrivAddr = static_cast <IIPv6Address *>(pNetIPv6Addr);
  return result;

bail:
  LOG_MSG_ERROR ("Err %d", result, 0, 0);
  return result;

} /* GeneratePrivAddr() */

ds::ErrorType NetworkIPv6::GetIPv6Address
(
  const ds::INAddr6Type  inAddr6,
  IIPv6Address           ** addrObj
)
{
  ds::ErrorType                      result = QDS_EFAULT;
  IPv6PrivPrefixInfoType *           pPrefixes = NULL;
  int                                numOfPrefixes = 0;
  int                                lenReq = 0;
  int                                index = 0;
  boolean                            found = FALSE;
  int32                              ifaceHandle = 0;
  NetworkIPv6Address *               pAddr = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  if (NULL == addrObj)
  {
    return QDS_EFAULT;
  }

  ifaceHandle = GetHandle();

  /*-------------------------------------------------------------------------
    Check the state of provided IPv6 address, if not found - return error
  -------------------------------------------------------------------------*/
  result = GetAllIPv6Prefixes(NULL, 0, &numOfPrefixes);
  if(AEE_SUCCESS != result || 0 == numOfPrefixes)
  {
    result = QDS_EFAULT;
    goto bail;
  }
  pPrefixes = (IPv6PrivPrefixInfoType *)ps_system_heap_mem_alloc(
                 sizeof(IPv6PrivPrefixInfoType) *
                 numOfPrefixes);

  if (NULL == pPrefixes) {
    LOG_MSG_ERROR("Can't allocate Prefixes", 0, 0, 0);
    result = AEE_ENOMEMORY;
    goto bail;
  }

  result = GetAllIPv6Prefixes(pPrefixes,
                              numOfPrefixes,
                              &lenReq);
  if(AEE_SUCCESS != result)
  {
    result = QDS_EFAULT;
    goto bail;
  }
  numOfPrefixes = MIN(numOfPrefixes, lenReq);
  for (index = 0; index < numOfPrefixes; index++)
  {
    if(IN6_ARE_PREFIX_EQUAL((struct ps_in6_addr *)pPrefixes[index].prefix,
                            (struct ps_in6_addr *)inAddr6,
                             pPrefixes[index].prefixLen))
    {
      found = TRUE;
      break;
    }
  }
  if(found)
  {
    pAddr = new NetworkIPv6Address ( inAddr6,
                                     pPrefixes[index].prefixType,
                                     ifaceHandle);
    if (NULL == pAddr)
    {
      result = AEE_ENOMEMORY;
    }
    else /* object allocated successfully */
    {
      result = pAddr->Init();
      if (AEE_SUCCESS != result)
      {
        DS_UTILS_RELEASEIF(pAddr);
      }
      else /* object initialized successfully */
      {
        *addrObj = static_cast <IIPv6Address *> (pAddr);
        result = AEE_SUCCESS;
      }
    }
  }

/* fall through */

bail:

  if(0 != pPrefixes)
  {
    PS_SYSTEM_HEAP_MEM_FREE(pPrefixes);
  }
  return result;
}

/*-------------------------------------------------------------------------
  Methods from INetworkIPv6Priv interface
-------------------------------------------------------------------------*/
ds::ErrorType NetworkIPv6::GetAllIPv6Prefixes
(
  IPv6PrivPrefixInfoType  *pPrefixes,
  int                      allPrefixesLen,
  int                      *allPrefixesLenReq
)
{
  ds::ErrorType           result;
  IPv6GetAllPrefixesType  ioctlArg;
  int                     index;
  int                     seqNumItems = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  /*-----------------------------------------------------------------------
    Prepare the IOCTL arguments to get the SIP server domain names.
  -----------------------------------------------------------------------*/
  memset (&ioctlArg, 0, sizeof (ioctlArg));
  ioctlArg.prefix_info_ptr =
    (IPv6PrefixInfoType *)
    ps_system_heap_mem_alloc
    (
      sizeof(IPv6PrefixInfoType) * MAX_IPV6_PREFIXES
    );


  if (NULL == ioctlArg.prefix_info_ptr)
  {
    /*---------------------------------------------------------------------
      Memory configuration is incorrect. This is an ASSERT condition.
    ---------------------------------------------------------------------*/
    ASSERT (0);
    result = AEE_ENOMEMORY;
    goto bail;
  }
  ioctlArg.num_prefixes = MAX_IPV6_PREFIXES;

  /*-----------------------------------------------------------------------
    Call the iface IOCTL to get SIP server domain names
  -----------------------------------------------------------------------*/
  result = IfaceIoctl (GetHandle(),
    IFACE_IOCTL_GET_ALL_V6_PREFIXES,
    static_cast <void *> (&ioctlArg));
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
      allPrefixesLen,
      allPrefixesLenReq,
      ioctlArg.num_prefixes,
      &seqNumItems
    );
  if(AEE_SUCCESS != result)
  {
    goto bail;
  }
  for (index =0; index < seqNumItems && 0 != pPrefixes; index++)
  {
     PS2DSIPv6Prefix (&ioctlArg.prefix_info_ptr[index], &pPrefixes[index]);
  }

  /* Fall through */

bail:

  if (NULL != ioctlArg.prefix_info_ptr)
  {
    PS_SYSTEM_HEAP_MEM_FREE(ioctlArg.prefix_info_ptr);
  }
  LOG_MSG_FUNCTION_EXIT ("Return 0x%x", result, 0, 0);
  return result;

} /* GetAllIPv6Prefixes() */

ds::ErrorType NetworkIPv6::GetSignalBus
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
    case IPv6PrivEvent::QDS_EV_PREFIX_CHANGED:
      *ppISigBus = mpSigBusPrefixChanged;
      (void)(*ppISigBus)->AddRef();
      return AEE_SUCCESS;

    default:
      *ppISigBus = NULL;
      return QDS_EINVAL;
  }

} /* GetSignalBus() */

