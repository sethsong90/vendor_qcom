/*===========================================================================
  FILE: DS_Sock_AddrUtils.cpp

  OVERVIEW: This file provides implementation of the AddrUtils class.

  DEPENDENCIES: None

  Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_AddrUtils.cpp#1 $
  $DateTime: 2009/09/28 14:45:31 $$Author: dmudgal $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-05-14 msr Created module

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"

#ifdef FEATURE_DATA_PS
#include "DS_Sock_AddrUtils.h"
#include "DS_Utils_DebugMsg.h"
#include "ps_in.h"

#include <string.h>

using namespace DS::Sock;
using namespace DS::Error;


/*===========================================================================

                     PUBLIC MEMBER FUNCTIONS

===========================================================================*/
DS::ErrorType AddrUtils::GetSockAddrStorage
(
  const SockAddrIN6Type *  v6SockAddrPtr,
  FamilyType               family,
  SockAddrStorageType *    sockAddrStoragePtr
)
throw()
{
  SockAddrINType *   v4SockAddrStoragePtr;
  SockAddrIN6Type *  v6SockAddrStoragePtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == v6SockAddrPtr || 0 == sockAddrStoragePtr)
  {
    LOG_MSG_ERROR( "NULL args", 0, 0, 0);
    return DSS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    If family is V4, populate SockAddrStorageType with a V4 address, else with
    a V6 address
  -------------------------------------------------------------------------*/
  if (Family::INET == family)
  {
    /*-----------------------------------------------------------------------
      Since V6 address can't be returned as a V4 address, fail the operation
      if v6SockAddrPtr is a V6 address. Otherwise, return V4 address
    -----------------------------------------------------------------------*/
    if (false == PS_IN6_ARE_ADDR_EQUAL( v6SockAddrPtr->addr, &ps_in6addr_any) &&
        false == PS_IN6_IS_ADDR_V4MAPPED( v6SockAddrPtr->addr))
    {
      LOG_MSG_INVALID_INPUT( "Can't copy V6 addr in to V4 storage", 0, 0, 0);
      return DSS_EINVAL;
    }

    v4SockAddrStoragePtr =
      reinterpret_cast <SockAddrINType *> ( sockAddrStoragePtr);

    v4SockAddrStoragePtr->family = DS::DSS_AF_INET;
    v4SockAddrStoragePtr->port   = v6SockAddrPtr->port;
    v4SockAddrStoragePtr->addr   =
      PS_IN6_GET_V4_FROM_V4_MAPPED_V6( v6SockAddrPtr->addr);
  }
  else
  {
    v6SockAddrStoragePtr =
      reinterpret_cast <SockAddrIN6Type *> ( sockAddrStoragePtr);

    memcpy( v6SockAddrStoragePtr, v6SockAddrPtr, sizeof( SockAddrIN6Type));
    v6SockAddrStoragePtr->family = DS::DSS_AF_INET6;
  }

  return SUCCESS;
} /* AddrUtils::GetSockAddrStorage() */


DS::ErrorType AddrUtils::GetSockAddrIN6
(
  const SockAddrStorageType *  sockAddrStoragePtr,
  SockAddrIN6Type *            v6SockAddrPtr
)
{
  const SockAddrINType *   v4SockAddrStoragePtr;
  const SockAddrIN6Type *  v6SockAddrStoragePtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == v6SockAddrPtr)
  {
    LOG_MSG_ERROR( "NULL v6SockAddr", 0, 0, 0);
    return DSS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Initialize v6SockAddrPtr to 0 so that all fields have a default value.
    Else flowInfo or scopeID will have garbage values which could be
    misinterpreted
  -------------------------------------------------------------------------*/
  memset (v6SockAddrPtr, 0, sizeof( SockAddrIN6Type));

  /*-------------------------------------------------------------------------
    If sockAddrStoragePtr is NULL, return ps_in6addr_any and port 0
  -------------------------------------------------------------------------*/
  if (0 == sockAddrStoragePtr)
  {
    v6SockAddrPtr->family = DS::DSS_AF_INET6;
    v6SockAddrPtr->port   = 0;
    memcpy( v6SockAddrPtr->addr, &ps_in6addr_any, sizeof( INAddr6Type));
    return SUCCESS;
  }

  /*-------------------------------------------------------------------------
    If family in sockAddrStoragePtr is DSS_AF_INET, copy IPv4 mapped IPv6 address
    in to v6SockAddrPtr. If it is DSS_AF_INET6, cast sockAddrStoragePtr to
    SockAddrIN6Type and copy in to v6SockAddrPtr
  -------------------------------------------------------------------------*/
  if (DS::DSS_AF_INET == sockAddrStoragePtr->family)
  {
    v4SockAddrStoragePtr =
      reinterpret_cast <const SockAddrINType *> ( sockAddrStoragePtr);

    v6SockAddrPtr->family = DS::DSS_AF_INET6;
    v6SockAddrPtr->port   = v4SockAddrStoragePtr->port;

    PS_IN6_GET_V4_MAPPED_V6_FROM_V4( v6SockAddrPtr->addr,
                                     v4SockAddrStoragePtr->addr);
  }
  else if (DS::DSS_AF_INET6 == sockAddrStoragePtr->family)
  {
    v6SockAddrStoragePtr =
      reinterpret_cast <const SockAddrIN6Type *> ( sockAddrStoragePtr);

    memcpy( v6SockAddrPtr, v6SockAddrStoragePtr, sizeof( SockAddrIN6Type));
  }
  else
  {
    LOG_MSG_INVALID_INPUT( "Unknown addr family %d",
                           sockAddrStoragePtr->family, 0, 0);
    return DSS_EAFNOSUPPORT;
  }

  return SUCCESS;
} /* AddrUtils::GetSockAddrIN6() */

#endif /* FEATURE_DATA_PS */
