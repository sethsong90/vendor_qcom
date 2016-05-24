/*===========================================================================
  FILE: DS_Sock_Socket.cpp

  OVERVIEW: This file provides implementation of the Socket class.

  DEPENDENCIES: None

  Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_Socket.cpp#22 $
  $DateTime: 2010/05/26 18:47:42 $$Author: smudired $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-05-14 msr Created module
  2010-01-05 ss  IP Multicast Loop feature addition.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"

#ifdef FEATURE_DATA_PS
#include "DS_Sock_Socket.h"
#include "DS_Sock_SocketDef.h"
#include "DS_Sock_SocketFactory.h"
#include "DS_Sock_RoutingManager.h"
#include "DS_Sock_SocketIPSec.h"
#include "PS_Sock_Platform_ISocketFactory.h"
#include "PS_Sock_Platform_ISocket.h"
#include "DS_Net_Platform.h"
#include "DS_Sock_AddrUtils.h"
#include "DS_Utils_DebugMsg.h"
#include "DS_Sock_EventDefs.h"
#include "DS_Net_INetworkPriv.h"
#include "AEEISignalCBFactory.h"
#include "DS_Utils_SignalCBFactory.bid"
#include "DS_Utils_SignalBus.bid"
#include "DS_Utils_CreateInstance.h"
#include "DS_Net_CreateInstance.h"

#include "ps_in.h"
#include "ps_socki_defs.h"
#include "ps_iface_defs.h"

#ifdef FEATURE_DSS_LINUX
/* Suppress Linux macro for constants used in DSS */
#ifdef NAME_MAX
#undef NAME_MAX
#endif
#endif /* FEATURE_DSS_LINUX */

/*===========================================================================

                        LOCAL DATA DEFINITIONS

===========================================================================*/
namespace DS
{
  namespace Sock
  {
    namespace OptName
    {
      const OptNameType NAME_MIN = 0;
      const OptNameType NAME_MAX = 25;
    }

    const int32  OPT_MAX_VAL = 0x7FFFFFFF;

    typedef struct
    {
      OptLevelType    optLevel;
      int32           defaultVal;
      int32           minVal;
      int32           maxVal;
    } OptInfoType;

    const OptInfoType sockOptInfoArr[ ] =
    {
      /* DSSOCK_SO_KEEPALIVE */
      { OptLevel::SOCKET, SO_KEEPALIVE_DEF_VAL, 0, OPT_MAX_VAL },

      /* DSSOCK_SO_REUSEADDR */
      { OptLevel::SOCKET, SO_REUSEADDR_DEF_VAL, 0, OPT_MAX_VAL },

      /* DSSOCK_SO_ERROR_ENABLE */
      { OptLevel::SOCKET, SO_ERROR_ENABLE_DEF_VAL, 0, OPT_MAX_VAL },

      /* DSSOCK_SO_TX_IFACE */
      { OptLevel::SOCKET, 0, 0, OPT_MAX_VAL },

      /* DSSOCK_SO_RCVBUF */
      { OptLevel::SOCKET, SO_RCVBUF_DEF_VAL, SO_RCVBUF_MIN_VAL,
          SO_RCVBUF_MAX_VAL },

      /* DSSOCK_SO_SNDBUF */
      { OptLevel::SOCKET, SO_SNDBUF_DEF_VAL, SO_SNDBUF_MIN_VAL,
          SO_SNDBUF_MAX_VAL },

      /* DSSOCK_SO_ERROR */
      { OptLevel::SOCKET, 0, 0, 0 },

      /* DSSOCK_TCP_NODELAY */
      { OptLevel::TCP, TCP_NODELAY_DEF_VAL, 0,  OPT_MAX_VAL },

      /* DSSOCK_TCP_DELAYED_ACK */
      { OptLevel::TCP, TCP_DELAYED_ACK_DEF_VAL, 0,  OPT_MAX_VAL },

      /* DSSOCK_TCP_SACK */
      { OptLevel::TCP, TCP_SACK_DEF_VAL, 0,  OPT_MAX_VAL },

      /* DSSOCK_TCP_TIMESTAMP */
      { OptLevel::TCP, TCP_TIMESTAMP_DEF_VAL, 0,  OPT_MAX_VAL },

      /* DSSOCK_TCP_FIONREAD */
      { OptLevel::TCP, 0, 0, SO_RCVBUF_MAX_VAL },

      /* DSSOCK_TCP_MAXSEG */
      { OptLevel::TCP, TCP_MAXSEG_DEF_VAL, TCP_MAXSEG_MIN_VAL,
        TCP_MAXSEG_MAX_VAL },

      /* DSSOCK_IP_RECVIF */
      { OptLevel::IP, IP_RECVIF_DEF_VAL, 0, OPT_MAX_VAL },

      /* DSSOCK_IP_RECVERR */
      { OptLevel::IP, IP_RECVERR_DEF_VAL, 0, OPT_MAX_VAL },

      /* DSSOCK_IP_TOS */
      { OptLevel::IP, IP_TOS_DEF_VAL, 0, 255 },

      /* DSSOCK_IP_TTL */
      { OptLevel::IP, IP_TTL_DEF_VAL, 1, 255 },

      /* DSSOCK_IP_MULTICAST_TTL */
      { OptLevel::IP, IP_MCAST_TTL_DEF_VAL, 1, 255 },

      /* DSSOCK_IPV6_RECVERR */
      { OptLevel::IPV6, IPV6_RECVERR_DEF_VAL, 0, OPT_MAX_VAL },

      /* DSSOCK_IPV6_TCLASS */
      { OptLevel::IPV6, IPV6_TCLASS_DEF_VAL, 0, 255 },

      /* DSSOCK_ICMP_TYPE */
      { OptLevel::ICMP, 0, 0, 255 },

      /* DSSOCK_ICMP_CODE */
      { OptLevel::ICMP, 0, 0, 255 },

      /* DSSOCK_ICMP_ECHO_ID */
      { OptLevel::ICMP, 0, 0, OPT_MAX_VAL },

      /* DSSOCK_ICMP_ECHO_SEQ_NUM */
      { OptLevel::ICMP, 0, 0, OPT_MAX_VAL },

      /* DSSOCK_IP_MULTICAST_LOOP */
      { OptLevel::IP, IP_MCAST_LOOP_DEF_VAL, 0, OPT_MAX_VAL }
    };

  } /* namespace Sock */
} /* namespace DS */

using namespace PS::Sock;
using namespace DS::Sock;
using namespace DS::Net;
using namespace DS::Error;
using namespace DS::Utils;


/*===========================================================================

                         PUBLIC MEMBER FUNCTIONS

===========================================================================*/
DS::ErrorType CDECL Socket::SetNetPolicy
(
  IPolicy *  newPolicyPtr
)
{
  ps_route_scope_type    routeScope;
  DS::ErrorType          dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);

  if (0 == newPolicyPtr)
  {
    LOG_MSG_INVALID_INPUT( "NULL policy, sock 0x%x", this, 0, 0);
    return DSS_EFAULT;
  }

  critSectPtr->Enter();

  /*-------------------------------------------------------------------------
    Make sure that platform socket is not closed yet. Else if platform socket
    is deleted, a 0 address would be accessed causing a crash
  -------------------------------------------------------------------------*/
  if (0 == platformSockPtr)
  {
    LOG_MSG_INVALID_INPUT( "Sock 0x%x closed", this, 0, 0);
    dsErrno = DSS_EINVAL;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Check if it is allowed to update policy
  -------------------------------------------------------------------------*/
  if (false == IsSetNetPolicySupported())
  {
    LOG_MSG_INVALID_INPUT( "Not supported, sock 0x%x", this, 0, 0);
    dsErrno = DSS_EOPNOTSUPP;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Generate route scope and set it in platform
  -------------------------------------------------------------------------*/
  dsErrno = NetPlatform::GetRouteScopeByPolicy( newPolicyPtr, &routeScope);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "GetRouteScopeByPolicy failed, sock 0x%x err 0x%x",
                   this, dsErrno, 0);
    goto bail;
  }

  dsErrno = platformSockPtr->SetRouteScope( &routeScope);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "SetRouteScope failed, sock 0x%x err 0x%x",
                   this, dsErrno, 0);
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Decrement refCnt of current policy object since it is not cached any more
  -------------------------------------------------------------------------*/
  if (0 != policyPtr)
  {
    (void) policyPtr->Release();
  }

  /*-------------------------------------------------------------------------
    Cache policy and increment refCnt
  -------------------------------------------------------------------------*/
  policyPtr = newPolicyPtr;
  (void) policyPtr->AddRef();

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x" , this, 0, 0);

  critSectPtr->Leave();
  return SUCCESS;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  critSectPtr->Leave();
  return dsErrno;

} /* Socket::SetNetPolicy() */


DS::ErrorType Socket::SetNetwork
(
  INetwork *  dsNetObjPtr
)
{
  INetworkPriv *        networkPrivObjPtr = 0;
  ps_route_scope_type   routeScope;
  DS::Net::IfaceIdType  ifaceId;
  DS::ErrorType         dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);

  if (0 == dsNetObjPtr)
  {
    LOG_MSG_INVALID_INPUT( "NULL net obj, sock 0x%x", this, 0, 0);
    return DSS_EFAULT;
  }

  critSectPtr->Enter();

  /*-------------------------------------------------------------------------
    It is invalid to associate a socket with a INetwork object if socket is
    already associated with one
  -------------------------------------------------------------------------*/
  if (0 != netObjPtr)
  {
    LOG_MSG_INVALID_INPUT( "Sock 0x%x bound to network", this, 0, 0);
    dsErrno = DSS_EOPNOTSUPP;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    It is invalid to associate a socket with a INetwork object if socket is
    already associated with an IPolicy object
  -------------------------------------------------------------------------*/
  if (0 != policyPtr)
  {
    LOG_MSG_INVALID_INPUT( "Sock 0x%x bound to policy", this, 0, 0);
    dsErrno = DSS_EOPNOTSUPP;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Fetch policy object associated with netObjPtr

    policyPtr's refCnt is incremented by GetPolicy()
  -------------------------------------------------------------------------*/
  dsErrno = dsNetObjPtr->QueryInterface
            (
              AEEIID_INetworkPriv,
              reinterpret_cast <void **> ( &networkPrivObjPtr)
            );
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "QueryInterface failed, netObj 0x%x", dsNetObjPtr, 0, 0);
    goto bail;
  }

  dsErrno = networkPrivObjPtr->GetPolicy( &policyPtr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "GetPolicy failed, netObj 0x%x", networkPrivObjPtr, 0, 0);
    goto bail;
  }

  (void) networkPrivObjPtr->Release();

  /*-------------------------------------------------------------------------
    Cache INetwork object and increment refCnt
  -------------------------------------------------------------------------*/
  netObjPtr = dsNetObjPtr;
  (void) netObjPtr->AddRef();

  /*-------------------------------------------------------------------------
    Generate route scope and set it in platform
  -------------------------------------------------------------------------*/
  (void) netObjPtr->GetIfaceId( &ifaceId);
  dsErrno = NetPlatform::GetRouteScopeByIfaceId( ifaceId,
                                                 ( Family::INET6 == family),
                                                 &routeScope);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "GetRouteScopeByIfaceHandle failed, sock 0x%x err 0x%x",
                   this, dsErrno, 0);
    goto bail;
  }

  dsErrno = platformSockPtr->SetRouteScope( &routeScope);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "SetRouteScope failed, sock 0x%x err 0x%x",
                   this, dsErrno, 0);
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register with network object for state change events. This event is used
    to take appropriate action as network state changes. For example,
    connected TCP sockets are closed once network object goes DOWN
  -------------------------------------------------------------------------*/
  dsErrno = RegNetworkStateChangedEvent();
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "RegNetworkStateChangedEvent failed, sock 0x%x err 0x%x",
                   this, dsErrno, 0);
    goto bail;
  }

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);

  critSectPtr->Leave();
  return SUCCESS;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  if (0 != networkPrivObjPtr)
  {
    (void) networkPrivObjPtr->Release();
  }

  if (0 != netObjPtr)
  {
    (void) netObjPtr->Release();
    netObjPtr = 0;
  }

  critSectPtr->Leave();
  return dsErrno;

} /* Socket::SetNetwork() */


DS::ErrorType CDECL Socket::Bind
(
  const SockAddrStorageType *  localAddrPtr
)
{
  const SockAddrINType *     v4LocalAddrPtr;
  const SockAddrIN6Type *    v6LocalAddrPtr;
  DS::ErrorType              dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);

  if (0 == localAddrPtr)
  {
    LOG_MSG_INVALID_INPUT( "NULL addr, sock 0x%x", this, 0, 0);
    return DSS_EFAULT;
  }

  critSectPtr->Enter();

  /*-------------------------------------------------------------------------
    Make sure that platform socket is not closed yet. Else if platform socket
    is deleted, a 0 address would be accessed causing a crash
  -------------------------------------------------------------------------*/
  if (0 == platformSockPtr)
  {
    LOG_MSG_INVALID_INPUT( "Sock 0x%x closed", this, 0, 0);
    dsErrno = DSS_EINVAL;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Validate localAddrPtr
  -------------------------------------------------------------------------*/
  switch (localAddrPtr->family)
  {
    case DS::DSS_AF_INET6:
    {
      /*---------------------------------------------------------------------
        Make sure that socket is V6, as a V6 address can't be bound to a
        V4 socket
      ---------------------------------------------------------------------*/
      if (Family::INET == family)
      {
        LOG_MSG_INVALID_INPUT( "Can't bind V6 addr to a V4 sock 0x%x",
                               this, 0, 0);
        dsErrno = DSS_EINVAL;
        goto bail;
      }

      /*---------------------------------------------------------------------
        If the application is trying to bind a socket to a multicast
        address, make sure that socket supports multicast (Only UDP supports
        multicast). Since it could be a V4 mapped V6 address, make sure that
        the address is neither V6 multicast nor V4 mapped V6 multicast.

        Applications are not allowed to bind to a specific IPv4 unicast
        address as the IP address of the interface in routingCache can
        change at any time. If they are allowed, socket must be closed if
        interface's IP address changes. So make sure that the application
        didn't pass V4 mapped V6 unicast address.

        Since applications can use private IPv6 addresses, they are allowed
        to bind to a specific IPv6 unicast address
      ---------------------------------------------------------------------*/
      v6LocalAddrPtr =
        reinterpret_cast <const SockAddrIN6Type *> ( localAddrPtr);

      if (PS_IN6_IS_V4_MAPPED_V6_ADDR_MULTICAST( v6LocalAddrPtr->addr) ||
          PS_IN6_IS_ADDR_MULTICAST( v6LocalAddrPtr->addr))
      {
        if (false == IsMulticastSupported())
        {
          LOG_MSG_INVALID_INPUT( "Sock 0x%x doesn't support multicast",
                                 this, 0, 0);
          dsErrno = DSS_EINVAL;
          goto bail;
        }
      }
      else if (PS_IN6_IS_ADDR_V4MAPPED( v6LocalAddrPtr->addr))
      {
        LOG_MSG_INVALID_INPUT( "V4 unicast must be INADDR_ANY, sock 0x%x",
                               this, 0, 0);
        dsErrno = DSS_EOPNOTSUPP;
        goto bail;
      }
      else if (false == PS_IN6_IS_ADDR_UNSPECIFIED( v6LocalAddrPtr->addr))
      {
        /*-------------------------------------------------------------------
          Iface address management layer needs to keep track of how many
          clients are using each IPv6 address so that it can manage the
          lifetime of addresses. So increment ref cnt since client is binding
          to a IPv6 address.
        -------------------------------------------------------------------*/
        /*lint -e{545} */
        if (DSS_ERROR ==
              NetPlatform::IPv6PrivAddrIncRefCnt( &( v6LocalAddrPtr->addr),
                                                  &dsErrno))
        {
          LOG_MSG_INFO3( "IPv6PrivAddrIncRefCnt failed, sock 0x%x err 0x%x",
                         this, dsErrno, 0);
          goto bail;
        }
      }

      break;
    }

    case DS::DSS_AF_INET:
    {
      /*---------------------------------------------------------------------
        If the application is trying to bind a socket to a multicast
        address, make sure that socket supports multicast (Only UDP supports
        multicast).

        Applications are not allowed to bind to a specific IPv4 unicast
        address as the IP address of the interface in routingCache can
        change at any time. If they are allowed to bind, socket must be closed
        when interface's IP address changes.
      ---------------------------------------------------------------------*/
      v4LocalAddrPtr =
        reinterpret_cast <const SockAddrINType *> ( localAddrPtr);

      if (PS_IN_IS_ADDR_MULTICAST( v4LocalAddrPtr->addr))
      {
        if (false == IsMulticastSupported())
        {
          LOG_MSG_INVALID_INPUT( "Sock 0x%x doesn't support multicast",
                                 this, 0, 0);
          dsErrno = DSS_EINVAL;
          goto bail;
        }
      }
      else if (PS_INADDR_ANY != v4LocalAddrPtr->addr)
      {
        LOG_MSG_INVALID_INPUT( "V4 unicast must be INADDR_ANY, sock 0x%x",
                               this, 0, 0);
        dsErrno = DSS_EOPNOTSUPP;
        goto bail;
      }

      break;
    }

    default:
    {
      LOG_MSG_INVALID_INPUT( "Unsupported family %d, sock 0x%x",
                             localAddrPtr->family, this, 0);
      dsErrno = DSS_EAFNOSUPPORT;
      goto bail;
    }
  } /* switch (localAddr's family) */

  /*-------------------------------------------------------------------------
    Call platform specific Bind()
  -------------------------------------------------------------------------*/
  dsErrno = platformSockPtr->Bind( localAddrPtr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "Bind failed, sock 0x%x err 0x%x", this, dsErrno, 0);
    goto bail;
  }

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);

  critSectPtr->Leave();
  return SUCCESS;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  critSectPtr->Leave();
  return dsErrno;

} /* Socket::Bind() */


DS::ErrorType CDECL Socket::Connect
(
  const SockAddrStorageType *  newRemoteAddrPtr
)
{
  void *               routingCachePtr;
  SockAddrIN6Type      newV6RemoteAddr;
  ps_route_scope_type  routeScope;
  DS::ErrorType        dsErrno;
  DS::ErrorType        tmpDSErrno;
  int32                routingCache;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);

  if (0 == newRemoteAddrPtr)
  {
    LOG_MSG_INVALID_INPUT( "NULL remote addr, sock 0x%x", this, 0, 0);
    return DSS_EFAULT;
  }

  dsErrno = AddrUtils::GetSockAddrIN6( newRemoteAddrPtr, &newV6RemoteAddr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "GetSockAddrIN6 failed, sock 0x%x err 0x%x",
                   this, dsErrno, 0);
    return dsErrno;
  }

  critSectPtr->Enter();

  /*-------------------------------------------------------------------------
    Make sure that platform socket is not closed yet. Else if platform socket
    is deleted, a 0 address would be accessed causing a crash
  -------------------------------------------------------------------------*/
  if (0 == platformSockPtr)
  {
    LOG_MSG_INVALID_INPUT( "Sock 0x%x closed", this, 0, 0);
    dsErrno = DSS_EINVAL;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Check if connect is supported on this socket as
      1. TCP doesn't support disconnecting a socket
      2. ICMP doesn't support Connect() at all

    Note that since this a protocol specific behaviour, and not platform
    specific, this check is done in sockets library
  -------------------------------------------------------------------------*/
  if (false ==
        IsConnectSupported( const_cast <SockAddrIN6Type *> ( &newV6RemoteAddr),
                            &dsErrno))
  {
    LOG_MSG_ERROR( "Connect not supported, err 0x%x sock 0x%x",
                   dsErrno, this, 0);
    goto bail;
  }

  /*-------------------------------------------------------------------------
    If remote addr is not ps_in6addr_any, socket is trying to connect to a peer
  -------------------------------------------------------------------------*/
  if (false == PS_IN6_ARE_ADDR_EQUAL( newV6RemoteAddr.addr, &ps_in6addr_any))
  {
    /*-----------------------------------------------------------------------
      0 is not a valid port and hence a socket can't communicate with it
    -----------------------------------------------------------------------*/
    if (0 == newV6RemoteAddr.port)
    {
      LOG_MSG_INVALID_INPUT( "0 remote port, sock 0x%x", this, 0, 0);
      dsErrno = DSS_EINVAL;
      goto bail;
    }

    /*-----------------------------------------------------------------------
      Perform routing only if routing cache is NULL. Otherwise, routing would
      be performed repeatedly on a TCP socket, if app polls for Connect() to
      succeed instead of waiting for WRITE_EV
    -----------------------------------------------------------------------*/
    if (0 != ( Event::WRITE_BIT_MASK_NULL_ROUTING_INFO &
                 eventBitMask[Event::WRITE]))
    {
      dsErrno = RoutePacket( newRemoteAddrPtr);
      if (SUCCESS != dsErrno)
      {
        LOG_MSG_INFO3( "RoutePacket failed, sock 0x%x err 0x%x",
                       this, dsErrno, 0);
        goto bail;
      }
    }
  }

  /*-------------------------------------------------------------------------
    Call platform specific Connect()
  -------------------------------------------------------------------------*/
  dsErrno = platformSockPtr->Connect( newRemoteAddrPtr);
  if (SUCCESS != dsErrno)
  {
    if (DSS_EWOULDBLOCK != dsErrno)
    {
      LOG_MSG_INFO3( "Connect failed, sock 0x%x err 0x%x", this, dsErrno, 0);
      goto bail;
    }
  }
  else
  {
    dsErrno = SUCCESS;
  }

  /*-------------------------------------------------------------------------
    Generate route scope and set it in platform
  -------------------------------------------------------------------------*/
  routingCachePtr = PS_RT_META_GET_ROUTING_CACHE( rtMetaInfoPtr);
  routingCache    = reinterpret_cast <int32> ( routingCachePtr);

  tmpDSErrno =
    NetPlatform::GetRouteScopeByIfaceHandle( routingCache,
                                             ( Family::INET6 == family),
                                             &routeScope);
  if (SUCCESS != tmpDSErrno)
  {
    LOG_MSG_INFO3( "GetRouteScopeByIfaceHandle failed, sock 0x%x err 0x%x",
                   this, tmpDSErrno, 0);
    dsErrno = tmpDSErrno;
    goto bail;
  }

  tmpDSErrno = platformSockPtr->SetRouteScope( &routeScope);
  if (SUCCESS != tmpDSErrno)
  {
    LOG_MSG_INFO3( "SetRouteScope failed, sock 0x%x err 0x%x",
                   this, tmpDSErrno, 0);
    dsErrno = tmpDSErrno;
    goto bail;
  }

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);

  critSectPtr->Leave();
  return dsErrno;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  critSectPtr->Leave();
  return dsErrno;

} /* Socket::Connect() */


DS::ErrorType CDECL Socket::Listen
(
  int32  backlog
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void) backlog;
  return DSS_EOPNOTSUPP;

} /* Socket::Listen() */


DS::ErrorType CDECL Socket::Accept
(
  SockAddrStorageType *  remoteAddrPtr,
  ISocket **             newSockPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void) remoteAddrPtr;
  (void) newSockPtr;
  return DSS_EOPNOTSUPP;

} /* Socket::Accept() */


DS::ErrorType CDECL Socket::GetSockName
(
  SockAddrStorageType *  localAddrPtr
)
{
  DS::ErrorType  dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);

  if (0 == localAddrPtr)
  {
    LOG_MSG_INVALID_INPUT( "NULL addr, sock 0x%x", this, 0, 0);
    return DSS_EFAULT;
  }

  critSectPtr->Enter();

  /*-------------------------------------------------------------------------
    Make sure that platform socket is not closed yet. Else if platform socket
    is deleted, a 0 address would be accessed causing a crash
  -------------------------------------------------------------------------*/
  if (0 == platformSockPtr)
  {
    LOG_MSG_INVALID_INPUT( "Sock 0x%x closed", this, 0, 0);
    dsErrno = DSS_EINVAL;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Call platform specific GetSockName(...) and return
  -------------------------------------------------------------------------*/
  dsErrno = platformSockPtr->GetSockName( localAddrPtr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "GetSockName failed, sock 0x%x err 0x%x", this, dsErrno, 0);
    goto bail;
  }

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);

  critSectPtr->Leave();
  return SUCCESS;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  critSectPtr->Leave();
  return dsErrno;

} /* Socket::GetSockName() */


DS::ErrorType CDECL Socket::GetPeerName
(
  SockAddrStorageType *  remoteAddrPtr
)
{
  DS::ErrorType  dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);

  if (0 == remoteAddrPtr)
  {
    LOG_MSG_INVALID_INPUT( "NULL addr, sock 0x%x", this, 0, 0);
    return DSS_EFAULT;
  }

  critSectPtr->Enter();

  /*-------------------------------------------------------------------------
    Make sure that platform socket is not closed yet. Else if platform socket
    is deleted, a 0 address would be accessed causing a crash
  -------------------------------------------------------------------------*/
  if (0 == platformSockPtr)
  {
    LOG_MSG_INVALID_INPUT( "Sock 0x%x closed", this, 0, 0);
    dsErrno = DSS_EINVAL;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Call platform specific GetPeerName(...) and return
  -------------------------------------------------------------------------*/
  dsErrno = platformSockPtr->GetPeerName( remoteAddrPtr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_ERROR( "GetPeerName failed, sock 0x%x err 0x%x", this, dsErrno, 0);
    goto bail;
  }

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);

  critSectPtr->Leave();
  return SUCCESS;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  critSectPtr->Leave();
  return dsErrno;

} /* Socket::GetPeerName() */


DS::ErrorType CDECL Socket::SetOpt
(
  OptLevelType  optLevel,
  OptNameType   optName,
  int32         optVal,
  int32         optLen
)
{
  DS::ErrorType  dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY_6( "Sock 0x%x level %d name %d val %d len %d",
                            this, optLevel, optName, optVal,
                            optLen, 0);

  /*-------------------------------------------------------------------------
    Validate that optName is in the supported range
  -------------------------------------------------------------------------*/
  if (OptName::NAME_MIN > optName || OptName::NAME_MAX <= optName)
  {
    LOG_MSG_INVALID_INPUT( "Invalid name %d, sock 0x%x", optName, this, 0);
    return DSS_EINVAL;
  }

  /*-------------------------------------------------------------------------
    Return error if option is not supported
  -------------------------------------------------------------------------*/
  if (false == IsOptSupported( optLevel, optName))
  {
    return DSS_ENOPROTOOPT;
  }

  /*-------------------------------------------------------------------------
    Validate that
      1. optLevel is same as the defined option level for optName
      2. optLen is 4 bytes (A separate API is defined to support socket
         options with optVal > 4 bytes
      3. minVal <= optVal <= maxVal
      4. Socket's family is V6 if optLevel is V6
  -------------------------------------------------------------------------*/
  if (optLevel != sockOptInfoArr[optName].optLevel)
  {
    LOG_MSG_INVALID_INPUT( "Invalid level %d name %d sock 0x%x",
                           optLevel, optName, this);
    return DSS_EINVAL;
  }

  if (sizeof( int) != optLen)
  {
    LOG_MSG_INVALID_INPUT( "Invalid len %d name %d sock 0x%x",
                           optLen, optName, this);
    return DSS_EINVAL;
  }

  if (sockOptInfoArr[optName].minVal > optVal ||
      sockOptInfoArr[optName].maxVal < optVal)
  {
    LOG_MSG_INVALID_INPUT_6( "Val %d is out of range (%d - %d), "
                             "name %d sock 0x%x",
                             optVal, sockOptInfoArr[optName].minVal,
                             sockOptInfoArr[optName].maxVal,
                             optName, this, 0);
    return DSS_EINVAL;
  }

  if (OptLevel::IPV6 == optLevel && Family::INET6 != family)
  {
    LOG_MSG_INVALID_INPUT( "Opt %d - invalid on V4 sock 0x%x",
                           optName, this, 0);
    return DSS_ENOPROTOOPT;
  }

  /*-------------------------------------------------------------------------
    It is not allowed to set values for following options:
      SO_TX_IFACE
      SO_ERROR
      TCP_FIONREAD
  -------------------------------------------------------------------------*/
  if (OptName::DSSOCK_SO_TX_IFACE == optName ||
      OptName::DSSOCK_SO_ERROR == optName ||
      OptName::DSSOCK_TCP_FIONREAD == optName)
  {
    LOG_MSG_INVALID_INPUT( "Not supported, name %d sock 0x%x",
                           optName, this, 0);
    return DSS_EOPNOTSUPP;
  }

  critSectPtr->Enter();

  /*-------------------------------------------------------------------------
    Make sure that platform socket is not closed yet. Else if platform socket
    is deleted, a 0 address would be accessed causing a crash
  -------------------------------------------------------------------------*/
  if (0 == platformSockPtr)
  {
    LOG_MSG_INVALID_INPUT( "Sock 0x%x closed", this, 0, 0);
    dsErrno = DSS_EINVAL;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Call platform specific SetOpt(...) and return
  -------------------------------------------------------------------------*/
  dsErrno = platformSockPtr->SetOpt( optLevel, optName, optVal, optLen);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "SetOpt failed, sock 0x%x err 0x%x", this, dsErrno, 0);
    goto bail;
  }

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);

  critSectPtr->Leave();
  return SUCCESS;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  critSectPtr->Leave();
  return dsErrno;

} /* Socket::SetOpt() */


DS::ErrorType CDECL Socket::GetOpt
(
  OptLevelType  optLevel,
  OptNameType   optName,
  int32 *       optValPtr,
  int32 *       optLenPtr
)
{
  DS::ErrorType  dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x level %d name %d",
                          this, optLevel, optName);

  /*-------------------------------------------------------------------------
    Validate that optName is in the supported range
  -------------------------------------------------------------------------*/
  if (OptName::NAME_MIN > optName || OptName::NAME_MAX <= optName)
  {
    LOG_MSG_INVALID_INPUT( "Invalid name %d, sock 0x%x", optName, this, 0);
    return DSS_EINVAL;
  }

  /*-------------------------------------------------------------------------
    Return error if option is not supported
  -------------------------------------------------------------------------*/
  if (false == IsOptSupported( optLevel, optName))
  {
    return DSS_ENOPROTOOPT;
  }

  /*-------------------------------------------------------------------------
    Validate that
      1. optLevel is same as the defined option level for optName
      2. Socket's family is V6 if optLevel is V6
  -------------------------------------------------------------------------*/
  if (optLevel != sockOptInfoArr[optName].optLevel)
  {
    LOG_MSG_INVALID_INPUT( "Invalid level %d, name %d sock 0x%x",
                           optLevel, optName, this);
    return DSS_EINVAL;
  }

  if (OptLevel::IPV6 == optLevel && Family::INET6 != family)
  {
    LOG_MSG_INVALID_INPUT( "Opt %d - invalid on V4 sock 0x%x",
                           optName, this, 0);
    return DSS_ENOPROTOOPT;
  }

  /*-------------------------------------------------------------------------
    It is not allowed to get values for ICMP level options
  -------------------------------------------------------------------------*/
  if (OptLevel::ICMP == optLevel)
  {
    LOG_MSG_INVALID_INPUT( "Not supported, name %d sock 0x%x",
                           optName, this, 0);
    return DSS_EOPNOTSUPP;
  }

  critSectPtr->Enter();

  if (OptName::DSSOCK_SO_TX_IFACE == optName)
  {
    if (0 != rtMetaInfoPtr)
    {
      *optValPtr =
        reinterpret_cast <int32> (PS_RT_META_GET_ROUTING_CACHE( rtMetaInfoPtr));
    }
    else
    {
      *optValPtr = 0;
    }

    *optLenPtr = sizeof( int32);
  }
  else
  {
    /*-----------------------------------------------------------------------
      Make sure that platform socket is not closed yet. Else if platform socket
      is deleted, a 0 address would be accessed causing a crash
    -----------------------------------------------------------------------*/
    if (0 == platformSockPtr)
    {
      LOG_MSG_INVALID_INPUT( "Sock 0x%x closed", this, 0, 0);
      dsErrno = DSS_EINVAL;
      goto bail;
    }

    /*-----------------------------------------------------------------------
      Call platform specific GetOpt(...) and return
    -----------------------------------------------------------------------*/
    dsErrno = platformSockPtr->GetOpt( optLevel,
                                       optName,
                                       reinterpret_cast <int32 *> ( optValPtr),
                                       reinterpret_cast <int32 *> ( optLenPtr));
    if (SUCCESS != dsErrno)
    {
      LOG_MSG_INFO3( "GetOpt failed, sock 0x%x err 0x%x", this, dsErrno, 0);
      goto bail;
    }
  }

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);

  critSectPtr->Leave();
  return SUCCESS;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  critSectPtr->Leave();
  return dsErrno;

} /* Socket::GetOpt() */


DS::ErrorType CDECL Socket::Shutdown
(
  ShutdownDirType  shutdownDir
)
{
  DS::ErrorType  dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x dir %d", this, shutdownDir, 0);

  if (ShutdownDir::RD != shutdownDir && ShutdownDir::WR != shutdownDir &&
      ShutdownDir::RD_WR != shutdownDir)
  {
    LOG_MSG_INVALID_INPUT( "Invalid dir %d, sock 0x%x", shutdownDir, this, 0);
    return DSS_EINVAL;
  }

  critSectPtr->Enter();

  /*-------------------------------------------------------------------------
    Make sure that platform socket is not closed yet. Else if platform socket
    is deleted, a 0 address would be accessed causing a crash
  -------------------------------------------------------------------------*/
  if (0 == platformSockPtr)
  {
    LOG_MSG_INVALID_INPUT( "Sock 0x%x closed", this, 0, 0);
    dsErrno = DSS_EINVAL;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Call platform specific Shutdown(...)
  -------------------------------------------------------------------------*/
  dsErrno = platformSockPtr->Shutdown( shutdownDir);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_ERROR( "Shutdown failed, sock 0x%x err 0x%x", this, dsErrno, 0);
    goto bail;
  }

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);

  critSectPtr->Leave();
  return SUCCESS;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  critSectPtr->Leave();
  return dsErrno;

} /* Socket::Shutdown() */


DS::ErrorType CDECL Socket::Close
(
  void
)
throw()
{
  SockAddrStorageType  localAddr;
  SockAddrIN6Type *    v6LocalAddrPtr;
  DS::ErrorType        dsErrno;
  DS::ErrorType        tmpDSErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);

  critSectPtr->Enter(); /*lint !e1550 */

  /*-------------------------------------------------------------------------
    Make sure that platform socket is not closed yet. Else if platform socket
    is deleted, a 0 address would be accessed causing a crash
  -------------------------------------------------------------------------*/
  if (0 == platformSockPtr)
  {
    LOG_MSG_INVALID_INPUT( "Sock 0x%x closed", this, 0, 0);
    //TODO Should we fail this or succeed
    dsErrno = SUCCESS;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Fetch the address to which socket is bound to. If it is a IPv6 address,
    its refcnt may need to be decremented later
  -------------------------------------------------------------------------*/
  dsErrno = platformSockPtr->GetSockName( &localAddr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "GetSockName failed, sock 0x%x err 0x%x", this, dsErrno, 0);
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Call platform specific Close(...)

    Do not treat DSS_EWOULDBLOCK as error as TCP socket may return it if the
    socket was connected
  -------------------------------------------------------------------------*/
  dsErrno = platformSockPtr->Close();
  if (SUCCESS == dsErrno)
  {
    platformSockPtr = 0;
    dsErrno         = SUCCESS;

    /*-----------------------------------------------------------------------
      Iface address management layer needs to keep track of how many
      clients are using each IPv6 address so that it can manage the
      lifetime of addresses. So decrement ref cnt since client has
      successfully closed the socket.

      If ref cnt is decremented before Close() returns success, private
      address could be deleted and TCP socket won't be able to send FIN out
      because address is not valid anymore.

      It is not necessary to check if application explicitly bound to this
      address. Ref cnt is maintained only for private addresses, and unless
      an application calls Bind(), socket will not be bound to a private
      address.
    -----------------------------------------------------------------------*/
    if (DS::DSS_AF_INET6 == localAddr.family)
    {
      v6LocalAddrPtr = reinterpret_cast <SockAddrIN6Type *> ( &localAddr);

      if (PS_IN6_IS_ADDR_V6( v6LocalAddrPtr->addr) &&
          false == PS_IN6_IS_ADDR_MULTICAST( v6LocalAddrPtr->addr))
      {
        /*lint -e{545} */
        (void) NetPlatform::IPv6PrivAddrDecRefCnt( &( v6LocalAddrPtr->addr),
                                                   &tmpDSErrno);
      }
    }

    if (0 != networkStateChangedSignalCtlPtr)
    {
      (void) networkStateChangedSignalCtlPtr->Detach();      /*lint !e1550 */
      (void) networkStateChangedSignalCtlPtr->Release();     /*lint !e1550 */
      networkStateChangedSignalCtlPtr = 0;

      (void) Release();
    }
  }
  else if (DSS_EWOULDBLOCK != dsErrno)
  {
    LOG_MSG_INFO3( "Close failed, sock 0x%x err 0x%x", this, dsErrno, 0);
    goto bail;
  }

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);

  critSectPtr->Leave(); /*lint !e1550 */
  return dsErrno;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  critSectPtr->Leave(); /*lint !e1550 */
  return dsErrno;

} /* Socket::Close() */


DS::ErrorType CDECL Socket::RegEvent
(
  ISignal *            signalObjPtr,
  DS::Sock::EventType  event
)
{
  Platform::EventType  platformEvent;
  DS::ErrorType        dsErrno;
  uint32               bitMask         = 0;
  bool                 setSignal       = false;
  bool                 regWithPlatform = true;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x event %d", this, event, 0);

  if (0 == signalObjPtr)
  {
    LOG_MSG_INVALID_INPUT( "NULL signal, sock 0x%x", this, 0, 0);
    return DSS_EFAULT;
  }

  if (Event::MAX_EV <= event)
  {
    LOG_MSG_INVALID_INPUT( "Invalid ev %d, sock 0x%x", event, this, 0);
    return DSS_EINVAL;
  }

  critSectPtr->Enter();

  /*-------------------------------------------------------------------------
    Make sure that platform socket is not closed yet. Else if platform socket
    is deleted, a 0 address would be accessed causing a crash
  -------------------------------------------------------------------------*/
  if (0 == platformSockPtr)
  {
    LOG_MSG_INVALID_INPUT( "Sock 0x%x closed", this, 0, 0);
    dsErrno = DSS_EINVAL;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Check if signal can be posted immediately. Otherwise, look at each event
    and determine the event to register with the platform
  -------------------------------------------------------------------------*/
  if (0 != ( eventBitMask[event] & TRANSIENT_ERR_BIT_MASK) &&
      0 == ( eventBitMask[event] & MEM_CONTROL_EVENT_BIT_MASK))
  {
    setSignal = true;
  }
  else
  {
    /*-----------------------------------------------------------------------
      Initialize to random value to shut up lint and to catch errors if for
      some reason platformEvent is not set to correct value
    -----------------------------------------------------------------------*/
    platformEvent = 0xFF;

    switch (event)
    {
      case Event::READ:
      {
        bitMask       = Event::READ_BIT_MASK_SOCKET_PLATFORM_FLOW_CONTROLLED;
        platformEvent = Platform::Event::READ;
        break;
      }

      case Event::WRITE:
      {
        bitMask       = Event::WRITE_BIT_MASK_SOCKET_PLATFORM_FLOW_CONTROLLED;
        platformEvent = Platform::Event::WRITE;
        break;
      }

      case Event::CLOSE:
      {
        bitMask       = Event::CLOSE_BIT_MASK_SOCKET_PLATFORM_FLOW_CONTROLLED;
        platformEvent = Platform::Event::CLOSE;
        break;
      }

      case Event::ACCEPT:
      {
        bitMask       = Event::ACCEPT_BIT_MASK_SOCKET_PLATFORM_FLOW_CONTROLLED;
        platformEvent = Platform::Event::ACCEPT;
        break;
      }

      case Event::DOS_ACK:
      {
        regWithPlatform = false;
        break;
      }

      default:
      {
        LOG_MSG_INVALID_INPUT( "Unknown ev %d, sock 0x%x", event, this, 0);
        dsErrno = DSS_EINVAL;
        goto bail;
      }
    }

    /*-----------------------------------------------------------------------
      Register for the event with the platform
    -----------------------------------------------------------------------*/
    if (regWithPlatform)
    {
      SetEventBitMask( event, bitMask);

      dsErrno = platformSockPtr->AsyncSelect( platformEvent);
      if (SUCCESS != dsErrno)
      {
        LOG_MSG_INFO3( "AsyncSelect failed, sock 0x%x err 0x%x",
                       this, dsErrno, 0);
        goto bail;
      }

      if (0 == ( eventBitMask[event] & FLOW_CONTROL_EVENT_BIT_MASK))
      {
        setSignal = true;
      }
    }
  }

  /*-------------------------------------------------------------------------
    Post signal if event can be asserted, else enqueue signal in to the
    signal bus
  -------------------------------------------------------------------------*/
  if (setSignal)
  {
    (void) signalObjPtr->Set();
    LOG_MSG_INFO1( "Set signal for ev %d, sock 0x%x", event, this, 0);
  }
  else
  {
    /*-----------------------------------------------------------------------
      Create a SignalBus if one isn't already created
    -----------------------------------------------------------------------*/
    if (0 == eventSignalBusPtr[event])
    {
      dsErrno = DSCreateInstance( 0,
                                  AEECLSID_DSUtilsSignalBus,
                                  0,
                                  (void **) &eventSignalBusPtr[event]);
      if (SUCCESS != dsErrno)
      {
        LOG_MSG_INFO3( "Couldn't create SignalBus, sock 0x%x", this, 0, 0);
        goto bail;
      }
    }

    (void) eventSignalBusPtr[event]->Add( signalObjPtr);
    LOG_MSG_INFO1( "Enq signal for ev %d, sock 0x%x", event, this, 0);
  }

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);

  critSectPtr->Leave();
  return SUCCESS;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  if (0 != bitMask)
  {
    ResetEventBitMask( event, bitMask);
  }

  critSectPtr->Leave();
  return dsErrno;

} /* Socket::RegEvent() */


DS::ErrorType CDECL Socket::Write
(
  const byte  bufArr[],
  int         bufLen,
  int32 *     numWrittenPtr
)
{
  SockAddrStorageType  remoteSockAddr;
  SeqBytesType         ioVec[1];
  DS::ErrorType        dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO4( "Sock 0x%x len %d", this, bufLen, 0);

  if (0 == numWrittenPtr)
  {
    LOG_MSG_INVALID_INPUT( "NULL arg, sock 0x%x", this, 0, 0);
    dsErrno = DSS_EFAULT;
    goto bail;
  }

  if (NULL == bufArr && 0 != bufLen)
  {
    LOG_MSG_INVALID_INPUT( "0 len, sock 0x%x", this, 0, 0);
    dsErrno = DSS_EINVAL;
    goto bail;
  }

  critSectPtr->Enter();

  /*-------------------------------------------------------------------------
    Make sure that platform socket is not closed yet. Else if platform socket
    is deleted, a 0 address would be accessed causing a crash
  -------------------------------------------------------------------------*/
  if (0 == platformSockPtr)
  {
    LOG_MSG_INVALID_INPUT( "Sock 0x%x closed", this, 0, 0);
    dsErrno = DSS_ENETDOWN;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    If Write event bit mask is set, handle the transient/flow control errors.
    Return error if all the errors couldn't be handled
  -------------------------------------------------------------------------*/
  if (0 != eventBitMask[Event::WRITE])
  {
    /*-----------------------------------------------------------------------
      Routing relies on destination address. So get peer address and also
      make sure that socket is connected
      //TODO handles all cases?? (states)
    -----------------------------------------------------------------------*/
    dsErrno = platformSockPtr->GetPeerName( &remoteSockAddr);
    if (SUCCESS != dsErrno)
    {
      LOG_MSG_INFO3( "GetPeerName failed, sock 0x%x err 0x%x",
                     this, dsErrno, 0);
      goto bail;
    }

    dsErrno = HandleNonZeroWriteEventBitMask( &remoteSockAddr);
    if (0 != eventBitMask[Event::WRITE])
    {
      LOG_MSG_INFO1( "Write ev bit mask 0x%x, sock 0x%x err 0x%x",
                     eventBitMask[Event::WRITE], this, dsErrno);
      goto bail;
    }
  }

  /*-------------------------------------------------------------------------
    Call platform specific SendMsg(...) and return
  -------------------------------------------------------------------------*/
  ioVec[0].data       = const_cast <byte *> ( bufArr);
  ioVec[0].dataLen    = bufLen;
  ioVec[0].dataLenReq = 0;

  dsErrno =
    platformSockPtr->SendMsg( 0,
                              ioVec,
                              1,
                              reinterpret_cast <int32 *> ( numWrittenPtr),
                              0,
                              0,
                              0,
                              0,
                              0);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "SendMsg failed, sock 0x%x err 0x%x", this, dsErrno, 0);
    goto bail;
  }

  critSectPtr->Leave();

  LOG_MSG_INFO4( "Success, sock 0x%x wrote %d", this, *numWrittenPtr, 0);
  return SUCCESS;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  critSectPtr->Leave();
  return dsErrno;

} /* Socket::Write() */


DS::ErrorType CDECL Socket::WriteV
(
  const SeqBytesType  ioVecArr[],
  int                 numIOVec,
  int32 *             numWrittenPtr
)
{
  SockAddrStorageType  remoteSockAddr;
  DS::ErrorType        dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO4( "Sock 0x%x len %d", this, 0, 0);

  if (0 == numWrittenPtr)
  {
    LOG_MSG_INVALID_INPUT( "NULL arg, sock 0x%x", this, 0, 0);
    return DSS_EFAULT;
  }

  critSectPtr->Enter();

  /*-------------------------------------------------------------------------
    Make sure that platform socket is not closed yet. Else if platform socket
    is deleted, a 0 address would be accessed causing a crash
  -------------------------------------------------------------------------*/
  if (0 == platformSockPtr)
  {
    LOG_MSG_INVALID_INPUT( "Sock 0x%x closed", this, 0, 0);
    dsErrno = DSS_ENETDOWN;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    If Write event bit mask is set, handle the transient/flow control errors.
    Return error if all the errors couldn't be handled
  -------------------------------------------------------------------------*/
  if (0 != eventBitMask[Event::WRITE])
  {
    /*-----------------------------------------------------------------------
      Routing relies on destination address. So get peer address and also
      make sure that socket is connected
      //TODO handles all cases?? (states)
    -----------------------------------------------------------------------*/
    dsErrno = platformSockPtr->GetPeerName( &remoteSockAddr);
    if (SUCCESS != dsErrno)
    {
      LOG_MSG_INFO3( "GetPeerName failed, sock 0x%x err 0x%x",
                     this, dsErrno, 0);
      goto bail;
    }

    dsErrno = HandleNonZeroWriteEventBitMask( &remoteSockAddr);
    if (0 != eventBitMask[Event::WRITE])
    {
      LOG_MSG_INFO1( "Write ev bit mask 0x%x, sock 0x%x err 0x%x",
                     eventBitMask[Event::WRITE], this, dsErrno);
      goto bail;
    }
  }

  /*-------------------------------------------------------------------------
    Call platform specific SendMsg(...) and return
  -------------------------------------------------------------------------*/
  dsErrno =
    platformSockPtr->SendMsg( 0,
                              ioVecArr,
                              numIOVec,
                              reinterpret_cast <int32 *> ( numWrittenPtr),
                              0,
                              0,
                              0,
                              0,
                              0);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "SendMsg failed, sock 0x%x err 0x%x", this, dsErrno, 0);
    goto bail;
  }

  critSectPtr->Leave();

  LOG_MSG_INFO4( "Success, sock 0x%x wrote %d", this, *numWrittenPtr, 0);
  return SUCCESS;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  critSectPtr->Leave();
  return dsErrno;

} /* Socket::WriteV() */


DS::ErrorType CDECL Socket::WriteDSMChain
(
  dsm_item_type **  dsmItemPtrPtr,
  int32 *           numBytesWrittenPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void) dsmItemPtrPtr;
  (void) numBytesWrittenPtr;
  return DSS_EOPNOTSUPP;
} /* Socket::WriteDSMChain() */


DS::ErrorType Socket::SendMsg
(
  const SockAddrStorageType *  remoteAddrPtr,
  const SeqBytesType           ioVecArr[],
  int                          numIOVec,
  int32 *                      numWrittenPtr,
  IAncData **                  inAncillaryDataPtrPtr,
  int                          inAncillaryDataLen,
  uint32                       inFlags,
  uint32                       flags
)
{
  const SockAddrStorageType *  peerAddrPtr;
  ps_pkt_meta_info_type *      pktMetaInfoPtr = 0;
  SockAddrStorageType          remoteSockAddr;
  SockAddrIN6Type              v6RemoteAddr;
  DS::ErrorType                dsErrno;
  bool                         localSendPktMetaInfo = sendPktMetaInfo;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO4( "Sock 0x%x", this, 0, 0);

  if (0 == remoteAddrPtr)
  {
    LOG_MSG_INVALID_INPUT( "NULL remote addr, sock 0x%x", this, 0, 0);
    return DSS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Ancillary data is not supported for SendMsg
  -------------------------------------------------------------------------*/
  if (( 0 != inAncillaryDataPtrPtr && 0 != *inAncillaryDataPtrPtr) ||
      0 != inFlags)
  {
    LOG_MSG_INVALID_INPUT( "Anc Data isn't supported, sock 0x%x", this, 0, 0);
    return DSS_EOPNOTSUPP;
  }

  /*-------------------------------------------------------------------------
    Only MSG_EXPEDITE and MSG_FAST_EXPEDITE are supported in tx path
  -------------------------------------------------------------------------*/
  if (0 != flags)
  {
    if (SendFlags::MSG_SOCK_EXPEDITE != flags &&
        SendFlags::MSG_FAST_SOCK_EXPEDITE != flags)
    {
      LOG_MSG_INVALID_INPUT( "Unsupported flag 0x%x, sock 0x%x",
                             flags, this, 0);
      return DSS_EOPNOTSUPP;
    }

    localSendPktMetaInfo = true;
  }

  critSectPtr->Enter();

  /*-------------------------------------------------------------------------
    Make sure that platform socket is not closed yet. Else if platform socket
    is deleted, a 0 address would be accessed causing a crash
  -------------------------------------------------------------------------*/
  if (0 == platformSockPtr)
  {
    LOG_MSG_INVALID_INPUT( "Sock 0x%x closed", this, 0, 0);
    dsErrno = DSS_EINVAL;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    If destination address is not specified, make sure that socket is
    connected
  -------------------------------------------------------------------------*/
  dsErrno = AddrUtils::GetSockAddrIN6( remoteAddrPtr, &v6RemoteAddr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "GetSockAddrIN6 failed, err 0x%x", dsErrno, 0, 0);
    goto bail;
  }

  if (PS_IN6_ARE_ADDR_EQUAL( v6RemoteAddr.addr, &ps_in6addr_any))
  {
    dsErrno = platformSockPtr->GetPeerName( &remoteSockAddr);
    if (SUCCESS != dsErrno)
    {
      LOG_MSG_INFO3( "GetPeerName failed, sock 0x%x err 0x%x",
                     this, dsErrno, 0);
      dsErrno = DSS_EADDRREQ;
      goto bail;
    }

    peerAddrPtr = &remoteSockAddr;
  }
  else
  {
    /*-----------------------------------------------------------------------
      If packet info differs, set NULL_ROUTING_INFO bit so that a fresh
      routing is performed
    -----------------------------------------------------------------------*/
    if (IsPktInfoDifferent( remoteAddrPtr))
    {
      SetEventBitMask( Event::WRITE, Event::WRITE_BIT_MASK_NULL_ROUTING_INFO);
    }

    peerAddrPtr = remoteAddrPtr;
  }

  /*-------------------------------------------------------------------------
    If Write event bit mask is set, handle the transient/flow control errors.
    Return error if all the errors couldn't be handled
  -------------------------------------------------------------------------*/
  if (0 != eventBitMask[Event::WRITE])
  {
    dsErrno = HandleNonZeroWriteEventBitMask( peerAddrPtr);
    if (0 != eventBitMask[Event::WRITE])
    {
      LOG_MSG_INFO1( "Write ev bit mask 0x%x, sock 0x%x err 0x%x",
                     eventBitMask[Event::WRITE], this, dsErrno);
      goto bail;
    }
  }

  if (localSendPktMetaInfo)
  {
    dsErrno = GeneratePktMetaInfo( &pktMetaInfoPtr, flags);
    if (SUCCESS != dsErrno)
    {
      LOG_MSG_INFO3( "GeneratePktMetaInfo failed, sock 0x%x err 0x%x",
                     this, dsErrno, 0);
      goto bail;
    }
  }

  /*-------------------------------------------------------------------------
    Call platform specific SendMsg(...) and return
  -------------------------------------------------------------------------*/
  dsErrno =
    platformSockPtr->SendMsg( remoteAddrPtr,
                              ioVecArr,
                              numIOVec,
                              reinterpret_cast <int32 *> ( numWrittenPtr),
                              inAncillaryDataPtrPtr,
                              inAncillaryDataLen,
                              inFlags,
                              flags,
                              pktMetaInfoPtr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "SendMsg failed, sock 0x%x err 0x%x", this, dsErrno, 0);
    goto bail;
  }

  PS_PKT_META_INFO_FREE( &pktMetaInfoPtr);

  critSectPtr->Leave();
  LOG_MSG_INFO4( "Success, sock 0x%x wrote %d", this, *numWrittenPtr, 0);

  return SUCCESS;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  if (0 != pktMetaInfoPtr)
  {
    PS_PKT_META_INFO_FREE( &pktMetaInfoPtr);
  }

  critSectPtr->Leave();
  return dsErrno;

} /* Socket::SendMsg() */


DS::ErrorType CDECL Socket::SendTo
(
  const byte                   bufArr[],
  int                          bufLen,
  const SockAddrStorageType *  remoteAddrPtr,
  uint32                       flags,
  int32 *                      numWrittenPtr
)
{
  SeqBytesType   ioVecArr[1];
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Fill the IOVEC array
  -------------------------------------------------------------------------*/
  ioVecArr[0].data       = const_cast <byte *> ( bufArr);
  ioVecArr[0].dataLen    = bufLen;
  ioVecArr[0].dataLenReq = 0;

  /*-------------------------------------------------------------------------
    Call Base Socket SendMsg
  -------------------------------------------------------------------------*/
  return SendMsg( remoteAddrPtr, ioVecArr, 1, numWrittenPtr, 0, 0, 0, flags);

} /* Socket::SendTo() */


DS::ErrorType CDECL Socket::SendToDSMChain
(
  dsm_item_type **             dsmItemPtrPtr,
  const SockAddrStorageType *  remoteAddrPtr,
  unsigned int                 flags,
  int32 *                      numBytesSentPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void) dsmItemPtrPtr;
  (void) remoteAddrPtr;
  (void) flags;
  (void) numBytesSentPtr;

  return DSS_EOPNOTSUPP;
} /* Socket::SendToDSMChain() */


DS::ErrorType CDECL Socket::Read
(
  byte   bufArr[],
  int    bufLen,
  int *  numReadPtr
)
{
  SeqBytesType   ioVecArr[1];
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ioVecArr[0].data       = bufArr;
  ioVecArr[0].dataLen    = bufLen;
  ioVecArr[0].dataLenReq = 0;

  return RecvMsg( 0, ioVecArr, 1, numReadPtr, 0, 0, 0, 0, 0);
} /* Socket::Read() */


DS::ErrorType CDECL Socket::ReadV
(
  SeqBytesType  ioVecArr[],
  int           numIOVec,
  int *         numReadPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return RecvMsg( 0, ioVecArr, numIOVec, numReadPtr, 0, 0, 0, 0, 0);
} /* Socket::ReadV() */


DS::ErrorType CDECL Socket::ReadDSMChain
(
  dsm_item_type **  dsmItemPtrPtr,
  int32 *           numBytesReadPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void) dsmItemPtrPtr;
  (void) numBytesReadPtr;

  return DSS_EOPNOTSUPP;
} /* Socket::ReadDSMChain() */


DS::ErrorType CDECL Socket::RecvFrom
(
  byte                   bufArr[],
  int                    bufLen,
  int *                  numReadPtr,
  uint32                 flags,
  SockAddrStorageType *  remoteAddrPtr
)
{
  SeqBytesType   ioVecArr[1];
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ioVecArr[0].data       = bufArr;
  ioVecArr[0].dataLen    = bufLen;
  ioVecArr[0].dataLenReq = 0;

  return RecvMsg( remoteAddrPtr, ioVecArr, 1, numReadPtr, 0, 0, 0, 0, flags);
} /* Socket::RecvFrom() */


DS::ErrorType CDECL Socket::RecvFromDSMChain
(
  dsm_item_type **       dsmItemPtrPtr,
  SockAddrStorageType *  remoteAddrPtr,
  unsigned int           flags,
  int32 *                numBytesRcvdPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void) dsmItemPtrPtr;
  (void) remoteAddrPtr;
  (void) flags;
  (void) numBytesRcvdPtr;

  return DSS_EOPNOTSUPP;
} /* Socket::RecvFromDSMChain() */


DS::ErrorType CDECL Socket::RecvMsg
(
  SockAddrStorageType *  remoteAddrPtr,
  SeqBytesType           ioVecArr[],
  int                    numIOVec,
  int *                  numReadPtr,
  IAncData **            outAncillaryDataPtrPtr,
  int                    outAncillaryDataLen,
  int *                  outAncillaryDataLenReqPtr,
  uint32 *               outFlagsPtr,
  uint32                 flags
)
{
  DS::ErrorType        dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO4 ("Sock 0x%x", this, 0, 0);

  /*-------------------------------------------------------------------------
    Only MSG_ERRQUEUE is supported by RecvMSg
  -------------------------------------------------------------------------*/
  if (0 != ( flags & ~( RecvFlags::MSG_ERRQUEUE)))
  {
    LOG_MSG_INVALID_INPUT( "Invalid flags 0x%x, sock 0x%x", flags, this, 0);
    return DSS_EOPNOTSUPP;
  }

  critSectPtr->Enter();

  /*-------------------------------------------------------------------------
    Make sure that platform socket is not closed yet. Else if platform socket
    is deleted, a 0 address would be accessed causing a crash
  -------------------------------------------------------------------------*/
  if (0 == platformSockPtr)
  {
    LOG_MSG_INVALID_INPUT( "Sock 0x%x closed", this, 0, 0);
    dsErrno = DSS_ENETDOWN;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Return error if socket is blocked for reading
      1. Return EWOULDBLOCK if socket is flow controlled
      2. ASSERT if err mask is unknown
  -------------------------------------------------------------------------*/
  if (0 != eventBitMask[Event::READ])
  {
    if (0 != ( FLOW_CONTROL_EVENT_BIT_MASK & eventBitMask[Event::READ]))
    {
      LOG_MSG_INFO1( "Read is blocked, sock 0x%x err mask 0x%x",
                     this, eventBitMask[Event::READ], 0);
      dsErrno = DSS_EWOULDBLOCK;
      goto bail;
    }
    else
    {
      LOG_MSG_INFO1( "Unknown err mask 0x%x, sock 0x%x",
                     eventBitMask[Event::READ], this, 0);
      ASSERT( 0);
      dsErrno = DSS_EWOULDBLOCK;
      goto bail;
    }
  }

  /*-------------------------------------------------------------------------
    Call platform specific RecvMsg(...) and return
  -------------------------------------------------------------------------*/
  dsErrno = platformSockPtr->RecvMsg( remoteAddrPtr,
                                      ioVecArr,
                                      numIOVec,
                                      reinterpret_cast <int32 *> ( numReadPtr),
                                      outAncillaryDataPtrPtr,
                                      outAncillaryDataLen,
                                      (int32 *) outAncillaryDataLenReqPtr,
                                      (uint32 *) outFlagsPtr,
                                      flags);

  critSectPtr->Leave();

  LOG_MSG_INFO4( "Sock 0x%x read %d err 0x%x", this, *numReadPtr, dsErrno);
  return dsErrno;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  critSectPtr->Leave();
  return dsErrno;

} /* Socket::RecvMsg() */


DS::ErrorType CDECL Socket::AddIPMembership
(
  const IPMembershipInfoType *  ipMembershipPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void) ipMembershipPtr;
  return DSS_EINVAL;
} /* Socket::AddIPMembership() */


DS::ErrorType CDECL Socket::DropIPMembership
(
  const IPMembershipInfoType *  ipMembershipPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void) ipMembershipPtr;
  return DSS_EINVAL;
} /* Socket::DropIPMembership() */


DS::ErrorType CDECL Socket::GetSOLingerReset
(
  LingerType *  soLingerPtr
)
{
  DS::ErrorType  dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);

  critSectPtr->Enter();

  /*-------------------------------------------------------------------------
    Make sure that platform socket is not closed yet. Else if platform socket
    is deleted, a 0 address would be accessed causing a crash
  -------------------------------------------------------------------------*/
  if (0 == platformSockPtr)
  {
    LOG_MSG_INVALID_INPUT( "Sock 0x%x closed", this, 0, 0);
    dsErrno = DSS_EINVAL;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Call platform specific GetSOLingerReset(...) and return
  -------------------------------------------------------------------------*/
  dsErrno = platformSockPtr->GetSOLingerReset( soLingerPtr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "GetSOLingerReset failed, sock 0x%x err 0x%x",
                   this, dsErrno, 0);
    goto bail;
  }

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);

  critSectPtr->Leave();
  return SUCCESS;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  critSectPtr->Leave();
  return dsErrno;

} /* Socket::GetSOLingerReset() */


DS::ErrorType CDECL Socket::SetSOLingerReset
(
  const LingerType *  soLingerPtr
)
{
  DS::ErrorType  dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);

  critSectPtr->Enter();

  /*-------------------------------------------------------------------------
    Make sure that platform socket is not closed yet. Else if platform socket
    is deleted, a 0 address would be accessed causing a crash
  -------------------------------------------------------------------------*/
  if (0 == platformSockPtr)
  {
    LOG_MSG_INVALID_INPUT( "Sock 0x%x closed", this, 0, 0);
    dsErrno = DSS_EINVAL;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Call platform specific SetSOLingerReset(...) and return
  -------------------------------------------------------------------------*/
  dsErrno = platformSockPtr->SetSOLingerReset( soLingerPtr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "SetSOLingerReset failed, sock 0x%x err 0x%x",
                   this, dsErrno, 0);
    goto bail;
  }

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);

  critSectPtr->Leave();
  return SUCCESS;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  critSectPtr->Leave();
  return dsErrno;

} /* Socket::GetSOLingerReset() */


DS::ErrorType CDECL Socket::GetDoSAckInfo
(
  DoSAckStatusType *  dosAckStatusPtr,
  uint32 *            overflowPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void) dosAckStatusPtr;
  (void) overflowPtr;

  return DSS_EOPNOTSUPP;
} /* Socket::GetDoSAckInfo() */


DS::ErrorType CDECL Socket::GetSystemOption
(
  boolean *  isSystemSocketPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);

  if (0 == isSystemSocketPtr)
  {
    LOG_MSG_INVALID_INPUT( "NULL arg", 0, 0, 0);
    return DSS_EFAULT;
  }

  critSectPtr->Enter();

  if (isSystemSocket)
  {
    *isSystemSocketPtr = TRUE;
  }
  else
  {
    *isSystemSocketPtr = FALSE;
  }

  critSectPtr->Leave();

  LOG_MSG_FUNCTION_EXIT( "Success, isSysSock %d sock 0x%x",
                         *isSystemSocketPtr, this, 0);
  return SUCCESS;

} /* Socket::GetSystemOption() */


DS::ErrorType CDECL Socket::SetSystemOption
(
  boolean  _isSystemSocket
)
{
  DS::ErrorType  dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x isSysSock %d", this, _isSystemSocket, 0);

  critSectPtr->Enter();

  /*-------------------------------------------------------------------------
    Make sure that platform socket is not closed yet. Else if platform socket
    is deleted, a 0 address would be accessed causing a crash
  -------------------------------------------------------------------------*/
  if (0 == platformSockPtr)
  {
    LOG_MSG_INVALID_INPUT( "Sock 0x%x closed", this, 0, 0);
    dsErrno = DSS_EINVAL;
    goto bail;
  }

  if (TRUE == _isSystemSocket)
  {
    isSystemSocket = true;
  }
  else
  {
    isSystemSocket = false;
  }

  /*-------------------------------------------------------------------------
    Call platform specific SetSystemOption(...) and return
  -------------------------------------------------------------------------*/
  platformSockPtr->SetSystemOption();

  critSectPtr->Leave();

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);
  return SUCCESS;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  critSectPtr->Leave();
  return dsErrno;

} /* Socket::SetSystemOption() */


void Socket::ProcessEvent
(
  Platform::EventType  event
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1( "Processing ev %d, sock 0x%x", event, this, 0);

  /*-------------------------------------------------------------------------
    Make sure that the object is not deleted.
  -------------------------------------------------------------------------*/
  if (NULL == critSectPtr || 0 == refCnt)
  {
    LOG_MSG_INFO1 ("Obj %p already deleted, return", this, 0, 0);
    return;
  }

  /*-------------------------------------------------------------------------
    Increment refCnt. Otherwise, if app calls Release() once signal is posted,
    refCnt could become 0 and socket object would be destroyed. This becomes
    an issue since object could be accessed by sockets library even after
    signal is posted to clean up state. So increment refCnt before posting an
    event and release after event is posted
  -------------------------------------------------------------------------*/
  (void) AddRef();

  switch (event)
  {
    case Platform::Event::PLATFORM_ENABLED:
    {
      ResetEventBitMask( Event::WRITE,
                         Event::WRITE_BIT_MASK_SOCKET_PLATFORM_DISABLED);
      break;
    }

    case Platform::Event::PLATFORM_DISABLED:
    {
      SetEventBitMask( Event::WRITE,
                       Event::WRITE_BIT_MASK_SOCKET_PLATFORM_DISABLED);
      break;
    }

    case Platform::Event::WRITE:
    {
      ResetEventBitMask( Event::WRITE,
                         Event::WRITE_BIT_MASK_SOCKET_PLATFORM_FLOW_CONTROLLED);
      break;
    }

    case Platform::Event::READ:
    {
      ResetEventBitMask( Event::READ,
                         Event::READ_BIT_MASK_SOCKET_PLATFORM_FLOW_CONTROLLED);
      break;
    }

    case Platform::Event::ACCEPT:
    {
      ResetEventBitMask(Event::ACCEPT,
                        Event::ACCEPT_BIT_MASK_SOCKET_PLATFORM_FLOW_CONTROLLED);
      break;
    }

    case Platform::Event::CLOSE:
    {
      ResetEventBitMask( Event::CLOSE,
                         Event::CLOSE_BIT_MASK_SOCKET_PLATFORM_FLOW_CONTROLLED);
      break;
    }

    default:
    {
      LOG_MSG_ERROR( "Unknown ev %d", event, 0, 0);
      ASSERT( 0);
      break;
    }
  }

  /*-------------------------------------------------------------------------
    Decrement refCnt since event has been posted and since the control is
    back to sockets library
  -------------------------------------------------------------------------*/
  (void) Release();
  return;

} /* Socket::ProcessEvent() */


boolean Socket::Process
(
  void *  userDataPtr
)
{
  Event::EventInfo *        eventInfoPtr;
  Event::DoSAckEventInfo *  dosAckEventInfoPtr;
  void *                    routingCachePtr;
  int32                     routingCache;
  int32                     defaultPSFlowHandle;
  int32                     psFlowHandle;
  int32                     physLinkHandle;
  int32                     origPhysLinkHandle;
  uint32                    reqIfaceState;
  phys_link_state_type      physLinkState;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == userDataPtr)
  {
    LOG_MSG_ERROR( "NULL arg", 0, 0, 0);
    ASSERT( 0);
    return FALSE;
  }

  eventInfoPtr = reinterpret_cast <Event::EventInfo *> ( userDataPtr);

  /*-------------------------------------------------------------------------
    Perform AddRef() to ensure that object does not get deleted
    in the middle of event processing.
  -------------------------------------------------------------------------*/
  if (NULL == critSectPtr || 0 == refCnt)
  {
    LOG_MSG_INFO1 ("Obj %p already deleted", this, 0, 0);
    return TRUE;
  }

  (void) AddRef();

  switch (eventInfoPtr->eventGroup)
  {
    case Event::EVENT_GROUP_NETWORK:
    {
      if (IFACE_CONFIGURING_EV == eventInfoPtr->eventMask ||
          IFACE_UP_EV == eventInfoPtr->eventMask ||
          IFACE_ROUTEABLE_EV == eventInfoPtr->eventMask)
      {
        UpdateRouteScopeByPolicy();
      }

      if (0 == rtMetaInfoPtr)
      {
        break;
      }

      switch (eventInfoPtr->eventMask)
      {
        case IFACE_CONFIGURING_EV:
        {
          routingCachePtr = PS_RT_META_GET_ROUTING_CACHE( rtMetaInfoPtr);
          routingCache    = reinterpret_cast <int32> ( routingCachePtr);

          if (eventInfoPtr->handle == routingCache ||
              SocketIPSec::IsHandleInIfaceList( eventInfoPtr->handle,
                                                rtMetaInfoPtr))
          {
            if (isSystemSocket)
            {
              reqIfaceState = IFACE_CONFIGURING | IFACE_UP;
              if (0 != ( reqIfaceState &
                           NetPlatform::GetIfaceState( routingCache)) &&
                  SocketIPSec::IsIfaceListInReqState( rtMetaInfoPtr,
                                                      reqIfaceState))
              {
                ResetEventBitMask( Event::WRITE,
                                   Event::WRITE_BIT_MASK_IFACE_NOT_WRITEABLE);
              }
            }
            else
            {
              SetEventBitMask( Event::WRITE,
                               Event::WRITE_BIT_MASK_IFACE_NOT_WRITEABLE);
            }
          }

          break;
        }

        case IFACE_UP_EV:
        {
          routingCachePtr = PS_RT_META_GET_ROUTING_CACHE( rtMetaInfoPtr);
          routingCache    = reinterpret_cast <int32> ( routingCachePtr);

          if (eventInfoPtr->handle == routingCache ||
              SocketIPSec::IsHandleInIfaceList( eventInfoPtr->handle,
                                                rtMetaInfoPtr))
          {
            reqIfaceState = IFACE_UP;
            if (isSystemSocket)
            {
              reqIfaceState |= IFACE_CONFIGURING;
            }

            if (0 != ( reqIfaceState &
                         NetPlatform::GetIfaceState( routingCache)) &&
                SocketIPSec::IsIfaceListInReqState( rtMetaInfoPtr,
                                                    reqIfaceState))
            {
              ResetEventBitMask( Event::WRITE,
                                 Event::WRITE_BIT_MASK_IFACE_NOT_WRITEABLE);
            }
          }

          break;
        }

        case IFACE_ROUTEABLE_EV:
        {
          routingCachePtr = PS_RT_META_GET_ROUTING_CACHE( rtMetaInfoPtr);
          routingCache    = reinterpret_cast <int32> ( routingCachePtr);

          if (eventInfoPtr->handle == routingCache)
          {
            ResetEventBitMask( Event::WRITE,
                               Event::WRITE_BIT_MASK_IFACE_NOT_WRITEABLE);
          }

          break;
        }

        case IFACE_GOING_DOWN_EV:
        case IFACE_LINGERING_EV:
        {
          routingCachePtr = PS_RT_META_GET_ROUTING_CACHE( rtMetaInfoPtr);
          routingCache    = reinterpret_cast <int32> ( routingCachePtr);

          if (eventInfoPtr->handle == routingCache ||
              SocketIPSec::IsHandleInIfaceList( eventInfoPtr->handle,
                                                rtMetaInfoPtr))
          {
            if (false == isSystemSocket)
            {
              SocketIPSec::ClearIfaceList( rtMetaInfoPtr);
              ProcessNetworkConfigChangedEvent( DSS_ENETDOWN);
            }
          }

          break;
        }

        case IFACE_DOWN_EV:
        {
          routingCachePtr = PS_RT_META_GET_ROUTING_CACHE( rtMetaInfoPtr);
          routingCache    = reinterpret_cast <int32> ( routingCachePtr);

          if (eventInfoPtr->handle == routingCache ||
              SocketIPSec::IsHandleInIfaceList( eventInfoPtr->handle,
                                                rtMetaInfoPtr))
          {
            DeleteRoutingCacheFromRouteScope( routingCache);

            SocketIPSec::ClearIfaceList( rtMetaInfoPtr);
            ProcessNetworkConfigChangedEvent( DSS_ENETDOWN);
          }

          break;
        }

        case IFACE_ADDR_CHANGED_EV:
        case IFACE_ADDR_FAMILY_CHANGED_EV:
        case IFACE_PREFIX_UPDATE_EV:
        {
          routingCachePtr = PS_RT_META_GET_ROUTING_CACHE( rtMetaInfoPtr);
          routingCache    = reinterpret_cast <int32> ( routingCachePtr);

          if (eventInfoPtr->handle == routingCache ||
              SocketIPSec::IsHandleInIfaceList( eventInfoPtr->handle,
                                                rtMetaInfoPtr))
          {
            ProcessNetworkConfigChangedEvent( DSS_EIPADDRCHANGED);
          }

          break;
        }

        case IFACE_IPFLTR_UPDATED_EV:
        {
          routingCachePtr = PS_RT_META_GET_ROUTING_CACHE( rtMetaInfoPtr);
          routingCache    = reinterpret_cast <int32> ( routingCachePtr);

          /*-----------------------------------------------------------------
            //TODO update comments
            IPSec and HC are global filters and QoS filters r on routing cache
            so no need to check in IPSec iface list
          -----------------------------------------------------------------*/
          if (eventInfoPtr->handle == routingCache)
          {
            if (IP_FLTR_CLIENT_QOS_OUTPUT == eventInfoPtr->handle ||
                IP_FLTR_CLIENT_IPSEC_OUTPUT == eventInfoPtr->handle ||
                IP_FLTR_CLIENT_HEADER_COMP == eventInfoPtr->handle)
            {
              SocketIPSec::TearDownIfaceList( rtMetaInfoPtr);
              SetEventBitMask( Event::WRITE,
                               Event::WRITE_BIT_MASK_NULL_ROUTING_INFO);
            }
          }

          break;
        }

        case IFACE_FLOW_DISABLED_EV:
        {
          routingCachePtr = PS_RT_META_GET_ROUTING_CACHE( rtMetaInfoPtr);
          routingCache    = reinterpret_cast <int32> ( routingCachePtr);

          if (eventInfoPtr->handle == routingCache ||
              SocketIPSec::IsHandleInIfaceList( eventInfoPtr->handle,
                                                rtMetaInfoPtr))
          {
            SetEventBitMask( Event::WRITE,
                             Event::WRITE_BIT_MASK_FLOW_DISABLED);
          }

          break;
        }

        case IFACE_FLOW_ENABLED_EV:
        {
          routingCachePtr = PS_RT_META_GET_ROUTING_CACHE( rtMetaInfoPtr);
          routingCache    = reinterpret_cast <int32> ( routingCachePtr);

          if (eventInfoPtr->handle == routingCache ||
              SocketIPSec::IsHandleInIfaceList( eventInfoPtr->handle,
                                                rtMetaInfoPtr))
          {
            if (IsFlowEnabled())
            {
              ResetEventBitMask( Event::WRITE,
                                 Event::WRITE_BIT_MASK_FLOW_DISABLED);
            }
          }

          break;
        }

        case FLOW_ACTIVATED_EV:
        {
          psFlowHandle = NetPlatform::GetPSFlowFromRtMetaInfo( rtMetaInfoPtr);

          if (eventInfoPtr->handle == psFlowHandle)
          {
            ResetEventBitMask( Event::WRITE,
                               Event::WRITE_BIT_MASK_PS_FLOW_SUSPENDED);
          }
          else if (eventInfoPtr->handle == origFltrResult)
          {
            if (SUCCESS != UseOrigFltrResult())
            {
              LOG_MSG_INFO3( "UseOrigFltrResult failed, sock 0x%x",
                             this, 0, 0);
              break;
            }
          }

          break;
        }

        case FLOW_SUSPENDED_EV:
        {
          psFlowHandle = NetPlatform::GetPSFlowFromRtMetaInfo( rtMetaInfoPtr);

          if (eventInfoPtr->handle == psFlowHandle)
          {
            if (SUCCESS != UpdateRtMetaInfoWithFlowFwding())
            {
              LOG_MSG_INFO3( "UpdateRtMetaInfoWithFlowFwding failed, "
                             "sock 0x%x", this, 0, 0);
              break;
            }
          }

          break;
        }

        case FLOW_TX_DISABLED_EV:
        {
          psFlowHandle = NetPlatform::GetPSFlowFromRtMetaInfo( rtMetaInfoPtr);

          if (eventInfoPtr->handle == psFlowHandle)
          {
            if (FLOW_ACTIVATED != NetPlatform::GetPSFlowState( psFlowHandle))
            {
              if (SUCCESS != UpdateRtMetaInfoWithFlowFwding())
              {
                LOG_MSG_INFO3( "UpdateRtMetaInfoWithFlowFwding failed, "
                               "sock 0x%x", this, 0, 0);
                break;
              }
            }
            else
            {
              SetEventBitMask( Event::WRITE,
                               Event::WRITE_BIT_MASK_FLOW_DISABLED);
            }
          }

          break;
        }

        case FLOW_TX_ENABLED_EV:
        {
          psFlowHandle = NetPlatform::GetPSFlowFromRtMetaInfo( rtMetaInfoPtr);

          if (eventInfoPtr->handle == psFlowHandle)
          {
            if (IsFlowEnabled())
            {
              ResetEventBitMask( Event::WRITE,
                                 Event::WRITE_BIT_MASK_FLOW_DISABLED);
              ResetEventBitMask( Event::WRITE,
                                 Event::WRITE_BIT_MASK_FLOW_FWDING_DISABLED);
            }
          }

          break;
        }

        case PHYS_LINK_UP_EV:
        {
          physLinkHandle =
            NetPlatform::GetPhysLinkFromRtMetaInfo( rtMetaInfoPtr);

          if (eventInfoPtr->handle == physLinkHandle)
          {
            ResetEventBitMask( Event::WRITE,
                               Event::WRITE_BIT_MASK_DORMANT_TRAFFIC_CHANNEL);
          }
          else
          {
            (void) NetPlatform::PSGetPhysLinkFromFlow( origFltrResult,
                                                       &origPhysLinkHandle);
            if (eventInfoPtr->handle == origPhysLinkHandle)
            {
              if (SUCCESS != UseOrigFltrResult())
              {
                LOG_MSG_INFO3( "UseOrigFltrResult failed, sock 0x%x",
                               this, 0, 0);
                break;
              }
            }
          }

          break;
        }

        case PHYS_LINK_DOWN_EV:
        {
          physLinkHandle =
            NetPlatform::GetPhysLinkFromRtMetaInfo( rtMetaInfoPtr);

          if (eventInfoPtr->handle == physLinkHandle)
          {
            psFlowHandle = NetPlatform::GetPSFlowFromRtMetaInfo( rtMetaInfoPtr);
            defaultPSFlowHandle =
              NetPlatform::GetDefaultPSFlowFromRtMetaInfo( rtMetaInfoPtr);

            if (psFlowHandle != defaultPSFlowHandle)
            {
              if (SUCCESS != UpdateRtMetaInfoWithFlowFwding())
              {
                LOG_MSG_INFO3( "UpdateRtMetaInfoWithFlowFwding failed, sock 0x%x",
                               this, 0, 0);
                break;
              }
            }
            else
            {
              if (0 == origFltrResult)
              {
                SetEventBitMask( Event::WRITE,
                                 Event::WRITE_BIT_MASK_DORMANT_TRAFFIC_CHANNEL);
              }
            }
          }

          break;
        }

        case PHYS_LINK_FLOW_DISABLED_EV:
        {
          physLinkHandle =
            NetPlatform::GetPhysLinkFromRtMetaInfo( rtMetaInfoPtr);

          if (eventInfoPtr->handle == physLinkHandle)
          {
            (void) NetPlatform::PhysLinkIoctl
                   (
                     physLinkHandle,
                     NetPlatform::PHYS_LINK_IOCTL_GET_STATE,
                     (void *) &physLinkState
                   );

            psFlowHandle = NetPlatform::GetPSFlowFromRtMetaInfo( rtMetaInfoPtr);
            defaultPSFlowHandle =
              NetPlatform::GetDefaultPSFlowFromRtMetaInfo( rtMetaInfoPtr);

            if (psFlowHandle != defaultPSFlowHandle &&
                PHYS_LINK_UP != physLinkState)
            {
              if (SUCCESS != UpdateRtMetaInfoWithFlowFwding())
              {
                LOG_MSG_INFO3( "UpdateRtMetaInfoWithFlowFwding failed, "
                               "sock 0x%x", this, 0, 0);
                break;
              }
            }
            else
            {
              SetEventBitMask( Event::WRITE,
                               Event::WRITE_BIT_MASK_FLOW_DISABLED);
            }
          }

          break;
        }

        case PHYS_LINK_FLOW_ENABLED_EV:
        {
          physLinkHandle =
            NetPlatform::GetPhysLinkFromRtMetaInfo( rtMetaInfoPtr);

          if (eventInfoPtr->handle == physLinkHandle)
          {
            if (IsFlowEnabled())
            {
              ResetEventBitMask( Event::WRITE,
                                 Event::WRITE_BIT_MASK_FLOW_DISABLED);
              ResetEventBitMask( Event::WRITE,
                                 Event::WRITE_BIT_MASK_FLOW_FWDING_DISABLED);
            }
          }

          break;
        }

        case PHYS_LINK_707_DOS_ACK_EV:
        {
          dosAckEventInfoPtr =
            reinterpret_cast <Event::DoSAckEventInfo *> ( userDataPtr);

          if (dosAckEventInfoPtr->handle == reinterpret_cast <int32> ( this))
          {
            ProcessDoSAckEvent( dosAckEventInfoPtr);
          }

          break;
        }
      }

      break;
    }

    case Event::EVENT_GROUP_PS_MEM:
    {
      ResetEventBitMask( Event::WRITE,
                         Event::WRITE_BIT_MASK_PS_MEM_BUF_NOT_AVAILABLE);
      break;
    }
    default:
      LOG_MSG_INFO1( "eventGroup type %d ignored", 
                     eventInfoPtr->eventGroup, 0, 0);
      break;
  } /* switch (eventInfoPtr->eventGroup)  */

  (void) Release();

  return TRUE;
} /* Socket::Process() */


DS::ErrorType Socket::QueryInterface
(
  AEEIID   iid,
  void **  objPtrPtr
)
{
  DS::ErrorType  dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x iid 0x%x", this, iid, 0);

  if (0 == objPtrPtr)
  {
    MSG_ERROR ("NULL args", 0, 0, 0);
    return DSS_EFAULT;
  }

  critSectPtr->Enter();

  switch (iid)
  {
    case AEEIID_ISocket:
    {
      *objPtrPtr = static_cast <ISocket *> ( this);
      break;
    }

    case AEEIID_ISocketPriv:
    {
      *objPtrPtr = static_cast <ISocketPriv *> ( this);
      break;
    }

    case DS::AEEIID_IDSMUtils:
    {
      *objPtrPtr = static_cast <DS::IDSMUtils *> ( this);
      break;
    }

    case AEEIID_IQI:
    {
      //TODO Fix this. fails if static_cast is used coz IQI is present from
      //multiple base interfaces
      *objPtrPtr = reinterpret_cast <IQI *> ( this);
      break;
    }

    default:
    {
      LOG_MSG_INVALID_INPUT( "Unknown iid 0x%x, sock 0x%x", iid, this, 0);
      dsErrno = AEE_ECLASSNOTSUPPORT;
      goto bail;
    }
  } /* switch (iid) */

  (void) AddRef();
  critSectPtr->Leave();

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);
  return SUCCESS;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  critSectPtr->Leave();
  return dsErrno;

} /* Socket::QueryInterface() */


/*===========================================================================

                         PROTECTED MEMBER FUNCTIONS

===========================================================================*/
Socket::Socket
(
  void
) :
    refCnt( 0),
    critSectPtr( 0),
    platformSockPtr( 0),
    rtMetaInfoPtr( 0),
    sendPktMetaInfo( false),
    policyPtr( 0),
    netObjPtr( 0),
    networkStateChangedSignalCtlPtr( 0),
    origFltrResult( 0)
{
  int32  idx;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Set members to initial values
  -------------------------------------------------------------------------*/
  for (idx = 0; idx < Event::MAX_EV; idx++)
  {
    eventBitMask[idx]      = 0;
    eventSignalBusPtr[idx] = 0;
  }

  /*-------------------------------------------------------------------------
    Using explicit scope to shut up lint. It complains because AddRef() is
    declared as virtual to shut up compiler
  -------------------------------------------------------------------------*/
  (void) Socket::AddRef();

  LOG_MSG_FUNCTION_EXIT( "Created sock 0x%p", this, 0, 0);
  return;

} /* Socket::Socket() */


DS::ErrorType Socket::Init
(
  FamilyType     _family,
  SocketType     sockType,
  ProtocolType   protocol
)
{
  Platform::ISocketFactory *  platformSockFactoryPtr;
  Platform::IEventListener *  eventListenerPtr;
  ps_route_scope_type         routeScope;
  DS::ErrorType               dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY_6( "Initing sock 0x%p _family %d type %d proto %d",
                            this, _family, sockType, protocol, 0, 0);

  /*-------------------------------------------------------------------------
    There is no need to validate (family, sockType, protocol) as they are
    already validated by SocketFactory::CreateSocket(...) and that function
    is the only entry point to create a socket
  -------------------------------------------------------------------------*/

  /*-------------------------------------------------------------------------
    Create critical section
  -------------------------------------------------------------------------*/
  if (SUCCESS != DS_Utils_CreateInstance( 0,
                                          AEECLSID_DSUtilsCritSect,
                                          0,
                                          (void **) &critSectPtr))
  {
    LOG_MSG_INFO3( "Couldn't create critsect, sock 0x%x", this, 0, 0);
    dsErrno = DSS_ENOMEM;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Create a Socket at platform layer and store the handle

    Cast to IEventListener before calling CreateSocket(). Otherwise, since
    platform gets an object whose vtable is pointing to Socket class's methods,
    when it invokes methods of IEventListener, methods of Socket object will
    be invoked.
  -------------------------------------------------------------------------*/
  platformSockFactoryPtr = Platform::ISocketFactory::CreateInstance();
  ASSERT( 0 != platformSockFactoryPtr);

  platformSockPtr = platformSockFactoryPtr->CreateSocket( _family,
                                                          sockType,
                                                          protocol,
                                                          critSectPtr,
                                                          &dsErrno);
  if (0 == platformSockPtr)
  {
    goto bail;
  }

  platformSockFactoryPtr->DeleteInstance();

  /*-------------------------------------------------------------------------
    Register an event listener with platform socket
  -------------------------------------------------------------------------*/
  eventListenerPtr = static_cast <Platform::IEventListener *> ( this);
  platformSockPtr->RegEventListener( eventListenerPtr);

  /*-------------------------------------------------------------------------
    Register with socket platform for PLATFORM_ENABLED event
  -------------------------------------------------------------------------*/
  dsErrno = platformSockPtr->AsyncSelect( Platform::Event::PLATFORM_ENABLED);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "AsyncSelect failed, sock 0x%x err 0x%x", this, dsErrno, 0);
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register with socket platform for PLATFORM_DISABLED event
  -------------------------------------------------------------------------*/
  dsErrno = platformSockPtr->AsyncSelect( Platform::Event::PLATFORM_DISABLED);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "AsyncSelect failed, sock 0x%x err 0x%x", this, dsErrno, 0);
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Generate route scope and set it in platform
  -------------------------------------------------------------------------*/
  dsErrno = NetPlatform::GetRouteScopeByPolicy( 0, &routeScope);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "GetRouteScopeByPolicy failed, sock 0x%x err 0x%x",
                   this, dsErrno, 0);
    goto bail;
  }

  dsErrno = platformSockPtr->SetRouteScope( &routeScope);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "SetRouteScope failed, sock 0x%x err 0x%x",
                   this, dsErrno, 0);
    goto bail;
  }

  family = _family;

  /*-------------------------------------------------------------------------
    Since routing hasn't been performed yet, set
    WRITE_BIT_MASK_NULL_ROUTING_INFO
  -------------------------------------------------------------------------*/
  SetEventBitMask( Event::WRITE, Event::WRITE_BIT_MASK_NULL_ROUTING_INFO);

  LOG_MSG_FUNCTION_EXIT( "Inited sock 0x%x", this, 0, 0);
  return SUCCESS;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  return dsErrno;

} /* Socket::Init() */


Socket::~Socket
(
  void
)
throw()
{
  SocketFactory *  sockFactoryPtr;
  LingerType       soLinger;
  DS::ErrorType    dsErrno;
  int32            idx;
  ICritSect *      localCritSectPtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);

  /*lint -save -e1550, -e1551 */

  if (0 == critSectPtr)
  {
    return;
  }

  localCritSectPtr = critSectPtr;
  localCritSectPtr->Enter();

  /*-------------------------------------------------------------------------
    Clean up all the resources which are still held by the socket

    Release() is called on the resources which are cached and delete is used
    on resources which are locally allocated and not shared with anybody else
  -------------------------------------------------------------------------*/

  /*-------------------------------------------------------------------------
    Close platform socket if it hasn't been closed already. Also set
    SO_LINGER_RESET option so that socket is closed immediately as application
    is not waiting for socket to close gracefully
  -------------------------------------------------------------------------*/
  if (0 != platformSockPtr)
  {
    soLinger.isLingerOn = TRUE;
    soLinger.timeInSec  = 0;

    dsErrno = platformSockPtr->SetSOLingerReset( &soLinger);
    if (SUCCESS != dsErrno)
    {
      LOG_MSG_INFO3( "SetSOLingerReset failed, sock 0x%x err 0x%x",
                     this, dsErrno, 0);
    }

    (void) Socket::Close();

    platformSockPtr = 0;
  }

  if (0 != policyPtr)
  {
    (void) policyPtr->Release();                      /*lint !e1550 !e1551 */
    policyPtr = 0;
  }

  if (0 != netObjPtr)
  {
    (void) netObjPtr->Release();                      /*lint !e1550 !e1551 */
    netObjPtr = 0;
  }

  if (0 != networkStateChangedSignalCtlPtr)
  {
    (void) networkStateChangedSignalCtlPtr->Release(); /*lint !e1550 !e1551 */
    networkStateChangedSignalCtlPtr = 0;
  }

  for (idx = 0; idx < Event::MAX_EV; idx++)
  {
    if (0 != eventSignalBusPtr[idx])
    {
      (void) eventSignalBusPtr[idx]->Release();       /*lint !e1550 !e1551 */
      eventSignalBusPtr[idx] = 0;
    }
  }

  if (0 != rtMetaInfoPtr)
  {
    PS_RT_META_INFO_FREE( &rtMetaInfoPtr);            /*lint !e1550 !e1551 */
  }

  /*-------------------------------------------------------------------------
    Delete socket from socket factory's list
  -------------------------------------------------------------------------*/
  sockFactoryPtr = SocketFactory::CreateInstance();
  ASSERT( 0 != sockFactoryPtr);

  (void) sockFactoryPtr->DeleteSocket( this);
  (void) sockFactoryPtr->Release();

  /*-------------------------------------------------------------------------
    Perform reset of object's critSectPtr within critical section.
  -------------------------------------------------------------------------*/
  critSectPtr = 0;

  localCritSectPtr->Leave();
  (void) localCritSectPtr->Release();

  /*lint -restore */

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);
  return;

} /* Socket::~Socket() */


DS::ErrorType Socket::CloneSocket
(
  Socket *  sockPtr
)
{
  DS::ErrorType       dsErrno;
  int32  idx;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);

  if (0 == sockPtr)
  {
    LOG_MSG_ERROR( "NULL sock", 0, 0, 0);
    ASSERT( 0);
    return DSS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Create critical section
  -------------------------------------------------------------------------*/
  if (SUCCESS != DS_Utils_CreateInstance( 0,
                                          AEECLSID_DSUtilsCritSect,
                                          0,
                                          (void **) &critSectPtr))
  {
    LOG_MSG_INFO3( "Couldn't create critsect, sock 0x%x", this, 0, 0);
    return DSS_ENOMEM;
  }

  /*-------------------------------------------------------------------------
    Copy event bit masks form sockPtr in to socket object. Event registrations
    are not copied though
  -------------------------------------------------------------------------*/
  for (idx = 0; idx < Event::MAX_EV; idx++)
  {
    eventBitMask[idx]      = sockPtr->eventBitMask[idx];
    eventSignalBusPtr[idx] = 0;
  }

  /*-------------------------------------------------------------------------
    Copy policy and network objects. Since objects are cached in yet another
    socket object, increment the refCnt
  -------------------------------------------------------------------------*/
  if (0 != policyPtr)
  {
    policyPtr = sockPtr->policyPtr;
    (void) policyPtr->AddRef();
  }

  if (0 != sockPtr->netObjPtr)
  {
    netObjPtr = sockPtr->netObjPtr;
    (void) netObjPtr->AddRef();

    dsErrno = RegNetworkStateChangedEvent();
    if (SUCCESS != dsErrno)
    {
      LOG_MSG_INFO3( "RegNetworkStateChangedEvent failed, sock 0x%x err 0x%x",
                     this, dsErrno, 0);
      return dsErrno;
    }
  }

  //TODO Check why metainfo can't be copied
  rtMetaInfoPtr = 0;

  family         = sockPtr->family;
  isSystemSocket = sockPtr->isSystemSocket;

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);
  return SUCCESS;

} /* Socket::CloneSocket() */


bool Socket::IsMulticastSupported
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return false;
} /* Socket::IsMulticastSupported() */


bool Socket::IsSetNetPolicySupported
(
  void
)
{
  SockAddrStorageType  remoteSockAddr;
  SockAddrIN6Type      v6RemoteSockAddr;
  DS::ErrorType        dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Socket's policy can't be updated if
      1. Socket is created via SocketFactory::CreateSocketByNetwork() OR
      2. Socket is created via Accept() OR
      3. Connect() had been called OR
      4. Listen() had been called OR
      5. Close() had been called

    In cases 1 & 2, socket's peer address is not ps_in6addr_any.
    //TODO Cases 3 & 4 are not supported as ENOTCONN are returned and we
    //return true today
    //TODO Update doc
    //Seems totally broken. GetPeerName returns ENOTCONN in OPENING state
  -------------------------------------------------------------------------*/

  /*-------------------------------------------------------------------------
    In case 1, socket object's netObjPtr is non-0. As, socket must use the
    interface pointed to by INetwork object in this case, it is invalid to
    update policy
  -------------------------------------------------------------------------*/
  if (0 != netObjPtr)
  {
    LOG_MSG_INFO1( "Sock 0x%x bound to network", this, 0, 0);
    return false;
  }

  /*-------------------------------------------------------------------------
    Fetch socket's peer address
  -------------------------------------------------------------------------*/
  dsErrno = platformSockPtr->GetPeerName( &remoteSockAddr);
  if (SUCCESS != dsErrno)
  {
    /*-----------------------------------------------------------------------
      Since GetPeerName() returns DSS_ENOTCONN if socket is not connected,
      return true
    -----------------------------------------------------------------------*/
    if (DSS_ENOTCONN == dsErrno)
    {
      return true;
    }
    else
    {
      LOG_MSG_INFO3( "GetPeerName failed, sock 0x%x err ox%x",
                     this, dsErrno, 0);
      return false;
    }
  }

  dsErrno = AddrUtils::GetSockAddrIN6( &remoteSockAddr, &v6RemoteSockAddr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "GetSockAddrIN6 failed, sock 0x%x err 0x%x",
                   this, dsErrno, 0);
    return false;
  }

  //TODO what is the point of this check?
  return PS_IN6_ARE_ADDR_EQUAL( v6RemoteSockAddr.addr, &ps_in6addr_any);

} /* Socket::IsSetNetPolicySupported() */


void Socket::SetEventBitMask
(
  DS::Sock::EventType  event,
  uint32               bitMask
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  eventBitMask[event] |= bitMask;

  LOG_MSG_INFO1( "Sock 0x%x ev %d ev mask 0x%x",
                 this, event, eventBitMask[event]);

  /*-------------------------------------------------------------------------
    Post event if bitMask corresponds to one of the transient errors

    Technically an event is posted if any of transient error bits are set or
    if none of the event error bits are set, i.e.
      0 != ( Event::TRANSIENT_ERR_BIT_MASK & eventBitMask[event]) ||
      0 == ( Event::EVENT_BIT_MASK & eventBitMask[event])

    The following check is an optimized version of the above check
    //TODO explain how this optimization works
  -------------------------------------------------------------------------*/
  if (0 != ( TRANSIENT_ERR_BIT_MASK & bitMask) &&
      0 == ( MEM_CONTROL_EVENT_BIT_MASK & bitMask))
  {
    PostEvent( event);
  }

  return;

} /* Socket::SetEventBitMask() */


void Socket::ResetEventBitMask
(
  DS::Sock::EventType  event,
  uint32               bitMask
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  eventBitMask[event] &= ~bitMask;

  LOG_MSG_INFO1( "Sock 0x%x ev %d ev mask 0x%x",
                 this, event, eventBitMask[event]);

  /*-------------------------------------------------------------------------
    Post event if none of the event errors are set

    Technically an event is posted if any of error bits are set or if all of
    the event error bits are not set, i.e.
      0 != ( Event::ERR_BIT_MASK & eventBitMask[event]) ||
      0 == ( Event::EVENT_BIT_MASK & eventBitMask[event])

    The following check is an optimized version of the above check
    //TODO explain how this optimization works
  -------------------------------------------------------------------------*/
  if (0 == ( FLOW_CONTROL_EVENT_BIT_MASK & eventBitMask[event]))
  {
    PostEvent( event);
  }

  return;

} /* Socket::ResetEventBitMask() */


/*===========================================================================

                         PRIVATE MEMBER FUNCTIONS

===========================================================================*/
DS::ErrorType Socket::HandleNonZeroWriteEventBitMask
(
  const SockAddrStorageType *  remoteAddrPtr
)
{
  DS::ErrorType        dsErrno;
  int32                physLinkHandle;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x ev mask 0x%x",
                          this, eventBitMask[Event::WRITE], 0);

  if (0 != ( TRANSIENT_ERR_BIT_MASK & eventBitMask[Event::WRITE]))
  {
    /*-----------------------------------------------------------------------
      Initializing to an error so that if a new transient error is added to
      the list and if it is not handled in the function, a legal value is
      returned
    -----------------------------------------------------------------------*/
    dsErrno = DSS_EINVAL;

    /*-----------------------------------------------------------------------
      Generate routing info if it is NULL
    -----------------------------------------------------------------------*/
    if (0 != ( Event::WRITE_BIT_MASK_NULL_ROUTING_INFO &
                 eventBitMask[Event::WRITE]))
    {
      dsErrno = RoutePacket( remoteAddrPtr);
      if (SUCCESS != dsErrno)
      {
        LOG_MSG_INFO3( "Routing failed, sock 0x%x", this, 0, 0);
        goto bail;
      }
    }

    /*-----------------------------------------------------------------------
      Bring up the traffic channel if it is dormant. It is important to
      check for this condition independently of NULL_ROUTING_INFO because,
      a dormant traffic channel could be part of new routing info
    -----------------------------------------------------------------------*/
    if (0 != ( Event::WRITE_BIT_MASK_DORMANT_TRAFFIC_CHANNEL &
                 eventBitMask[Event::WRITE]))
    {
      physLinkHandle =
        NetPlatform::GetPhysLinkFromRtMetaInfo( rtMetaInfoPtr);

      dsErrno = NetPlatform::PhysLinkUpCmd( physLinkHandle, 0);
      if (SUCCESS != dsErrno && DSS_EWOULDBLOCK != dsErrno)
      {
        LOG_MSG_INFO3( "PhysLinkBringUpCmd failed, sock 0x%x err 0x%x",
                       this, dsErrno, 0);
        goto bail;
      }

      /*---------------------------------------------------------------------
        Since traffic channel is coming out of dormancy, reset the transient
        error. If traffic channel is not up immediately, phys link would go
        to COMING_UP state and posts flow disabled event which will disable
        app from writing data
      ---------------------------------------------------------------------*/
      ResetEventBitMask( Event::WRITE,
                         Event::WRITE_BIT_MASK_DORMANT_TRAFFIC_CHANNEL);
    }
  } /* if (transient errors) */
  else /* if (flow control events) */
  {
    dsErrno = DSS_EWOULDBLOCK;
  }

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x ev mask 0x%x",
                         this, eventBitMask[Event::WRITE], 0);
  return dsErrno;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  return dsErrno;
} /* Socket::HandleNonZeroWriteEventBitMask() */


bool Socket::IsPktInfoDifferent
(
  const SockAddrStorageType *  remoteAddrPtr
)
{
  const SockAddrIN6Type *  v6RemoteAddrPtr;
  const SockAddrINType  *  v4RemoteAddrPtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == rtMetaInfoPtr)
  {
    return true;
  }

  if (DS::DSS_AF_INET6 == remoteAddrPtr->family)
  {
    v6RemoteAddrPtr =
      reinterpret_cast <const SockAddrIN6Type *> ( remoteAddrPtr);

    if (PS_IN6_IS_ADDR_V4MAPPED( v6RemoteAddrPtr->addr))
    {
      if (rtMetaInfoPtr->pkt_info.ip_hdr.v4.dest.ps_s_addr !=
            PS_IN6_GET_V4_FROM_V4_MAPPED_V6( v6RemoteAddrPtr->addr))
      {
        return true;
      }
    }
    else
    {
      if (false ==
            PS_IN6_ARE_ADDR_EQUAL
            (
              &( rtMetaInfoPtr->pkt_info.ip_hdr.v6.hdr_body.base_hdr.dst_addr),
              v6RemoteAddrPtr->addr
            ))
      {
        return true;
      }

      /*---------------------------------------------------------------------
        Make sure that scopeId in remote addr is same as the cached scopeId.
        Scope Id is specified for link local addresses
      ---------------------------------------------------------------------*/
      if (PS_IN6_IS_ADDR_MC_LINKLOCAL( v6RemoteAddrPtr->addr) ||
          PS_IN6_IS_ADDR_LINKLOCAL( v6RemoteAddrPtr->addr))
      {
        if (0 != v6RemoteAddrPtr->scopeId &&
            rtMetaInfoPtr->pkt_info.if_ptr !=
              NetPlatform::PSIfaceGetHandle( v6RemoteAddrPtr->scopeId))
        {
          return true;
        }
      }
    }
  }
  else
  {
    v4RemoteAddrPtr =
      reinterpret_cast <const SockAddrINType *> ( remoteAddrPtr);

    if (rtMetaInfoPtr->pkt_info.ip_hdr.v4.dest.ps_s_addr != v4RemoteAddrPtr->addr)
    {
      return true;
    }
  }

  return false;
} /* Socket::IsPktInfoDifferent() */


DS::ErrorType Socket::RoutePacket
(
  const SockAddrStorageType *  remoteAddrPtr
)
{
  ps_rt_meta_info_type *  newRtMetaInfoPtr;
  SockAddrIN6Type         v6RemoteAddr;
  DS::ErrorType           dsErrno = SUCCESS;
  int32                   psFlowHandle;
  int32                   physLinkHandle;
  phys_link_state_type    physLinkState;
  bool                    fwdDataToDefaultPSFlow = false;
  bool                    reorigPhysLink         = false;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x write ev mask 0x%x",
                          this, eventBitMask[Event::WRITE], 0);

  /*-------------------------------------------------------------------------
    Since a fresh routing is performed, rtMetaInfo needs to be updated. So
    allocate a new buffer and update it with routing results
  -------------------------------------------------------------------------*/
  PS_RT_META_INFO_GET( newRtMetaInfoPtr);
  if (0 == newRtMetaInfoPtr)
  {
    LOG_MSG_ERROR( "No mem for Rt meta info", 0, 0, 0);
    //TODO Wrong causes infinite loop
    SetEventBitMask( Event::WRITE,
                     Event::WRITE_BIT_MASK_PS_MEM_BUF_NOT_AVAILABLE);

    /*-----------------------------------------------------------------------
      Return DSS_EWOULDBLOCK instead of ENOMEM because this is a recoverable
      error. Memory might be available next time application writes a data
      packet
    -----------------------------------------------------------------------*/
    dsErrno = DSS_EWOULDBLOCK;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Generate pkt info, which is used in routing and filtering
  -------------------------------------------------------------------------*/
  dsErrno = AddrUtils::GetSockAddrIN6( remoteAddrPtr, &v6RemoteAddr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "GetSockAddrIN6 failed, sock 0x%x err 0x%x",
                   this, dsErrno, 0);
    goto bail;
  }

  dsErrno = GeneratePktInfo( &v6RemoteAddr,
                             &( PS_RT_META_GET_PKT_INFO( newRtMetaInfoPtr)));
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "GeneratePktInfo failed, sock 0x%x err 0x%x",
                   this, dsErrno, 0);
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Route the packet and fetch routing cache
  -------------------------------------------------------------------------*/
  dsErrno = RoutingManager::RoutePacket( this,
                                         isSystemSocket,
                                         policyPtr,
                                         newRtMetaInfoPtr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "RoutingManager::RoutePacket failed, sock 0x%x err 0x%x",
                   this, dsErrno, 0);
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Update the pkt info with source address if it isn't already set
  -------------------------------------------------------------------------*/
  dsErrno = UpdateSrcAddrInPktInfo( &v6RemoteAddr, newRtMetaInfoPtr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "UpdateSrcAddrInPktInfo failed, sock 0x%x err 0x%x",
                   this, dsErrno, 0);
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Filter various clients and store the results in rtMetaInfo
  -------------------------------------------------------------------------*/
  RoutingManager::FltrClient( this,
                              IP_FLTR_CLIENT_QOS_OUTPUT,
                              newRtMetaInfoPtr);
  SocketIPSec::FltrIPSecClient( this, newRtMetaInfoPtr, rtMetaInfoPtr);
  RoutingManager::FltrClient( this,
                              IP_FLTR_CLIENT_HEADER_COMP,
                              newRtMetaInfoPtr);

  /*-------------------------------------------------------------------------
    Send the new rtMetaInfo to platform
        //TODO What err handling?
  -------------------------------------------------------------------------*/
  dsErrno = platformSockPtr->SetRtMetaInfo( newRtMetaInfoPtr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "SetRtMetaInfo failed, sock 0x%x err 0x%x",
                   this, dsErrno, 0);
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Free the old rtMetaInfo and store the new one in the socket
  -------------------------------------------------------------------------*/
  PS_RT_META_INFO_FREE( &rtMetaInfoPtr);
  rtMetaInfoPtr = newRtMetaInfoPtr;
  newRtMetaInfoPtr = 0;

  /*-------------------------------------------------------------------------
    Block socket from writing data if IPSec ifaces are not in UP state
  -------------------------------------------------------------------------*/
  if (false == SocketIPSec::IsIfaceListInReqState( rtMetaInfoPtr, IFACE_UP))
  {
    SetEventBitMask( Event::WRITE, Event::WRITE_BIT_MASK_IFACE_NOT_WRITEABLE);
    dsErrno = DSS_EWOULDBLOCK;
  }

  /*-------------------------------------------------------------------------
    If ps_flow is not in ACTIVATED state, data can't be transmitted and hence
    set PS_FLOW_SUSPENDED bit mask
  -------------------------------------------------------------------------*/
  psFlowHandle = NetPlatform::GetPSFlowFromRtMetaInfo( rtMetaInfoPtr);
  if (FLOW_ACTIVATED != NetPlatform::GetPSFlowState( psFlowHandle))
  {
    fwdDataToDefaultPSFlow = true;
  }

  /*-------------------------------------------------------------------------
    If phys link is in DOWN state, data can't be transmitted as
    traffic channel is dormant. So set DORMANT_TRAFFIC_CHANNEL bit mask
  -------------------------------------------------------------------------*/
  (void) NetPlatform::PSGetPhysLinkFromFlow( psFlowHandle, &physLinkHandle);
  if (0 != physLinkHandle)
  {
    (void) NetPlatform::PhysLinkIoctl( physLinkHandle,
                                       NetPlatform::PHYS_LINK_IOCTL_GET_STATE,
                                       (void *) &physLinkState);
    if (PHYS_LINK_UP != physLinkState)
    {
      if (PHYS_LINK_DOWN == physLinkState)
      {
        reorigPhysLink = true;
      }

      fwdDataToDefaultPSFlow = true;
    }
  }
  else
  {
    fwdDataToDefaultPSFlow = true;
  }

  if (fwdDataToDefaultPSFlow)
  {
    UseDefaultPSFlow();
  }

  /*-------------------------------------------------------------------------
    Otherwise PHYS_LINK_FLOW_DISABLED_EV wud be rcved and coz it matches
    meta info FLOW_DISABLED will be set
  -------------------------------------------------------------------------*/
  if (reorigPhysLink)
  {
    dsErrno = NetPlatform::PhysLinkUpCmd( physLinkHandle, 0);
    if (SUCCESS != dsErrno && DSS_EWOULDBLOCK != dsErrno)
    {
      LOG_MSG_INFO3( "PhysLinkBringUpCmd failed, sock 0x%x err 0x%x",
                     this, dsErrno, 0);
      goto bail;
    }
  }

  /*-------------------------------------------------------------------------
    Check if any entity in data path (routing cache, ps_flow and phys link)
    is flow controlled. This must be done only after socket is updated with
    newRtMetaInfoPtr as the method uses rtMetaInfoPtr member of socket.
    Otherwise, stale rt meta info is used in determining if data path is
    flow controlled
  -------------------------------------------------------------------------*/
  if (false == IsFlowEnabled())
  {
    if (0 == origFltrResult)
    {
      SetEventBitMask( Event::WRITE, Event::WRITE_BIT_MASK_FLOW_DISABLED);
    }
    else
    {
      SetEventBitMask( Event::WRITE,
                       Event::WRITE_BIT_MASK_FLOW_FWDING_DISABLED);
    }

    dsErrno = DSS_EWOULDBLOCK;
  }

  /*-------------------------------------------------------------------------
    Finally, reset NULL_ROUTING_INFO bit as all the information is
    available now
  -------------------------------------------------------------------------*/
  ResetEventBitMask( Event::WRITE, Event::WRITE_BIT_MASK_NULL_ROUTING_INFO);

  /*-------------------------------------------------------------------------
    Check if pktMetaInfo needs to be generated for every packet
  -------------------------------------------------------------------------*/
  sendPktMetaInfo = NetPlatform::IsPSFlowDelaySensitive( psFlowHandle);

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);
  return dsErrno;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  if (0 != newRtMetaInfoPtr)
  {
    PS_RT_META_INFO_FREE( &newRtMetaInfoPtr);
  }

  return dsErrno;

} /* Socket::RoutePacket() */


DS::ErrorType Socket::GeneratePktInfo
(
  const SockAddrIN6Type *  v6RemoteAddrPtr,
  ip_pkt_info_type *       pktInfoPtr
)
{
  SockAddrStorageType  localSockAddr;
  SockAddrIN6Type      tmpV6LocalSockAddr;
  SockAddrIN6Type *    v6LocalSockAddrPtr;
  SockAddrINType *     v4LocalSockAddrPtr;
  DS::ErrorType        dsErrno;
  int32                optVal;
  int32                optLen;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);

  memset( pktInfoPtr, 0, sizeof( ip_pkt_info_type));

  if (false == PS_IN6_IS_ADDR_V4MAPPED( v6RemoteAddrPtr->addr))
  {
    /*-----------------------------------------------------------------------
      Populate IP Version and destination address
    -----------------------------------------------------------------------*/
    pktInfoPtr->ip_vsn             = IP_V6;
    pktInfoPtr->ip_hdr.v6.hdr_type = PS_IPV6_BASE_HDR;

    pktInfoPtr->ip_hdr.v6.hdr_body.base_hdr.version  = IP_V6;
    memcpy( &( pktInfoPtr->ip_hdr.v6.hdr_body.base_hdr.dst_addr),
            v6RemoteAddrPtr->addr,
            sizeof( DS::INAddr6Type));

    /*-----------------------------------------------------------------------
      Populate trafic class by fetching it from platform
    -----------------------------------------------------------------------*/
    dsErrno = platformSockPtr->GetOpt( OptLevel::IPV6,
                                       OptName::DSSOCK_IPV6_TCLASS,
                                       &optVal,
                                       &optLen);
    if (SUCCESS != dsErrno)
    {
      LOG_MSG_INFO3( "GetOpt failed for IPV6_TCLASS, sock 0x%x err 0x%x",
                     this, dsErrno, 0);
      pktInfoPtr->ip_hdr.v6.hdr_body.base_hdr.trf_cls = 0;
    }
    else
    {
      pktInfoPtr->ip_hdr.v6.hdr_body.base_hdr.trf_cls =
        static_cast <uint8> ( optVal);
    }
  }
  else
  {
    /*-----------------------------------------------------------------------
      Populate IP Version and destination address
    -----------------------------------------------------------------------*/
    pktInfoPtr->ip_vsn                = IP_V4;
    pktInfoPtr->ip_hdr.v4.dest.ps_s_addr =
      PS_IN6_GET_V4_FROM_V4_MAPPED_V6( v6RemoteAddrPtr->addr);

    /*-----------------------------------------------------------------------
      Populate TOS by fetching it from platform
    -----------------------------------------------------------------------*/
    dsErrno = platformSockPtr->GetOpt( OptLevel::IP,
                                       OptName::DSSOCK_IP_TOS,
                                       &optVal,
                                       &optLen);
    if (SUCCESS != dsErrno)
    {
      LOG_MSG_INFO3( "GetOpt failed for IP_TOS, sock 0x%x err 0x%x",
                     this, dsErrno, 0);
      goto bail;
    }

    pktInfoPtr->ip_hdr.v4.tos = static_cast < uint8> ( optVal);
  }

  /*-------------------------------------------------------------------------
    Make sure that valid scopeId is specified if remote addr is linklocal
  -------------------------------------------------------------------------*/
  if (PS_IN6_IS_ADDR_MC_LINKLOCAL( v6RemoteAddrPtr->addr) ||
      PS_IN6_IS_ADDR_LINKLOCAL( v6RemoteAddrPtr->addr))
  {
    if (0 == v6RemoteAddrPtr->scopeId)
    {
      LOG_MSG_INVALID_INPUT( "ScopeId not specified for linklocal addr, "
                             "sock 0x%x", this, 0, 0);
      dsErrno = DSS_ENOROUTE;
      goto bail;
    }

    pktInfoPtr->if_ptr =
      NetPlatform::PSIfaceGetHandle( v6RemoteAddrPtr->scopeId);

    if (0 == pktInfoPtr->if_ptr)
    {
      LOG_MSG_INVALID_INPUT( "Invalid scopeId %d specified for linklocal "
                             "addr, sock 0x%x",
                             v6RemoteAddrPtr->scopeId, this, 0);
      dsErrno = DSS_EINVAL;
      goto bail;
    }
  }

  /*-------------------------------------------------------------------------
    If socket is bound to an address, populate src address in pkt info.
    Otherwise leave it as 0.
  -------------------------------------------------------------------------*/
  dsErrno = platformSockPtr->GetSockName( &localSockAddr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "GetSockName failed, sock 0x%x err 0x%x", this, dsErrno, 0);
    goto bail;
  }

  if (localSockAddr.family == DS::DSS_AF_INET)
  {
    v4LocalSockAddrPtr = reinterpret_cast <SockAddrINType *> ( &localSockAddr);
    pktInfoPtr->ip_hdr.v4.source.ps_s_addr = v4LocalSockAddrPtr->addr;
  }
  else
  {
    v6LocalSockAddrPtr = reinterpret_cast <SockAddrIN6Type *> ( &localSockAddr);
    memcpy( &( pktInfoPtr->ip_hdr.v6.hdr_body.base_hdr.src_addr),
            v6LocalSockAddrPtr->addr,
            sizeof( DS::INAddr6Type));
  }

  /*-------------------------------------------------------------------------
    Populate protocol specfic info
  -------------------------------------------------------------------------*/
  dsErrno = AddrUtils::GetSockAddrIN6( &localSockAddr, &tmpV6LocalSockAddr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "GetSockAddrIN6 failed, sock 0x%x err 0x%x",
                   this, dsErrno, 0);
    return dsErrno;
  }

  dsErrno = FillProtocolInfoInPktInfo( v6RemoteAddrPtr,
                                       &tmpV6LocalSockAddr,
                                       pktInfoPtr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "FillProtocolInfoInPktInfo failed, sock 0x%x err 0x%x",
                   this, dsErrno, 0);
    goto bail;
  }

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);
  return SUCCESS;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  return dsErrno;

} /* Socket::GeneratePktInfo() */


DS::ErrorType Socket::UpdateSrcAddrInPktInfo
(
  const SockAddrIN6Type *  v6RemoteAddrPtr,
  ps_rt_meta_info_type *   newRtMetaInfoPtr
)
{
  void *               routingCachePtr;
  ip_pkt_info_type *   pktInfoPtr;
  SockAddrStorageType  localSockAddr;
  SockAddrIN6Type      tmpV6LocalSockAddr;
  DS::INAddr6Type      ifaceIPAddr;
  DS::ErrorType        dsErrno;
  int32                routingCache;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);

  dsErrno = platformSockPtr->GetSockName( &localSockAddr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "GetSockName failed, sock 0x%x err 0x%x", this, dsErrno, 0);
    goto bail;
  }

  dsErrno = AddrUtils::GetSockAddrIN6( &localSockAddr, &tmpV6LocalSockAddr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "GetSockAddrIN6 failed, sock 0x%x err 0x%x",
                   this, dsErrno, 0);
    goto bail;
  }

  if (true == PS_IN6_IS_ADDR_UNSPECIFIED( tmpV6LocalSockAddr.addr))
  {
    pktInfoPtr = &( PS_RT_META_GET_PKT_INFO( newRtMetaInfoPtr));

    routingCachePtr = PS_RT_META_GET_ROUTING_CACHE( newRtMetaInfoPtr);
    routingCache    = reinterpret_cast <int32> ( routingCachePtr);

    memset(&ifaceIPAddr, 0, sizeof(&ifaceIPAddr));
    dsErrno = NetPlatform::IfaceGetAddr( routingCache,
                                         &( v6RemoteAddrPtr->addr),
                                         &ifaceIPAddr);
    if (SUCCESS != dsErrno)
    {
      LOG_MSG_INFO3( "NetPlatform::IfaceGetAddr failed, sock 0x%x err 0x%x",
                     this, dsErrno, 0);
      goto bail;
    }

    if (false == PS_IN6_IS_ADDR_UNSPECIFIED( ifaceIPAddr) &&
        false == PS_IN6_IS_ADDR_V4MAPPED( ifaceIPAddr))
    {
      /*---------------------------------------------------------------------
        If no address was bound then the default prefix is used. Ensure that
        the prefix is in a valid state to be used. (ie. It cannot be in the
        DEPRECATED or any other state than VALID)
      ---------------------------------------------------------------------*/
      if (false == NetPlatform::IsDefaultIPv6AddrStateValid( routingCache) &&
          false == PS_IN6_IS_ADDR_LINKLOCAL( v6RemoteAddrPtr->addr))
      {
        LOG_MSG_ERROR( "Can't use src IP addr in invalid state, sock 0x%x",
                       this, 0, 0);
        dsErrno = DSS_ENOSRCADDR;
        goto bail;
      }

      memcpy( &( pktInfoPtr->ip_hdr.v6.hdr_body.base_hdr.src_addr),
              ifaceIPAddr,
              sizeof( DS::INAddr6Type));
    }
    else
    {
      pktInfoPtr->ip_hdr.v4.source.ps_s_addr =
        PS_IN6_GET_V4_FROM_V4_MAPPED_V6( ifaceIPAddr);
    }
  }

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);
  return SUCCESS;

bail:
  LOG_MSG_FUNCTION_EXIT( "Fail, sock 0x%x err 0x%x", this, dsErrno, 0);
  return dsErrno;

} /* Socket::UpdateSrcAddrInPktInfo() */


DS::ErrorType Socket::GeneratePktMetaInfo
(
  ps_pkt_meta_info_type **  pktMetaInfoPtrPtr,
  uint32                    flags
)
{
  time_type  timestamp;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == pktMetaInfoPtrPtr)
  {
    LOG_MSG_ERROR( "NULL arg, sock 0x%x", this , 0, 0);
    ASSERT( 0);
    return DSS_EFAULT;
  }

  PS_PKT_META_INFO_GET( *pktMetaInfoPtrPtr);
  if (0 == *pktMetaInfoPtrPtr)
  {
    LOG_MSG_ERROR( "No mem for Pkt meta info", 0, 0, 0);
    SetEventBitMask( Event::WRITE,
                     Event::WRITE_BIT_MASK_PS_MEM_BUF_NOT_AVAILABLE);

    /*-----------------------------------------------------------------------
      Return EWOULDBLOCK instead of ENOMEM because this is a recoverable
      error. Memory might be available next time application writes a data
      packet
    -----------------------------------------------------------------------*/
    return DSS_EWOULDBLOCK;
  }

  /*-----------------------------------------------------------------------
    Set the transmit flags and socket file descriptor
  -----------------------------------------------------------------------*/
  PS_PKT_META_SET_TX_FLAGS( *pktMetaInfoPtrPtr, flags);
  PS_PKT_META_SET_DOS_ACK_HANDLE( *pktMetaInfoPtrPtr,
                                  reinterpret_cast <int32> ( this));

  /*-------------------------------------------------------------------------
    Set time stamp
  -----------------------------------------------------------------------*/
  /*lint -save -e10, -e534 */
  time_get_ms( timestamp);
  PS_PKT_META_SET_TIMESTAMP( *pktMetaInfoPtrPtr, timestamp);
  /*lint -restore */

  return SUCCESS;
} /* Socket::GeneratePktMetaInfo() */


void Socket::PostEvent
(
  DS::Sock::EventType  event
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x ev %d", this, event, 0);

  /*-------------------------------------------------------------------------
    Post event if a signal had been registered for this event
  -------------------------------------------------------------------------*/
  if (0 != eventSignalBusPtr[event])
  {
    LOG_MSG_INFO1( "Posting ev %d, sock 0x%x", event, this, 0);
    (void) eventSignalBusPtr[event]->Set();           /*lint !e1550 !e1551 */

    /*-----------------------------------------------------------------------
      Since event registrations on socket are one-shot, clear them as event
      is posted. Application has to call RegEvent() again if it needs an event
    -----------------------------------------------------------------------*/
    (void) eventSignalBusPtr[event]->Release();       /*lint !e1550 !e1551 */
    eventSignalBusPtr[event] = 0;
  }

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);
  return;

} /* Socket::PostEvent() */


bool Socket::IsFlowEnabled
(
  void
)
{
  void *  routingCachePtr;
  int32   routingCache;
  int32   psFlowHandle;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  routingCachePtr = PS_RT_META_GET_ROUTING_CACHE( rtMetaInfoPtr);
  routingCache    = reinterpret_cast <int32> ( routingCachePtr);

  psFlowHandle = NetPlatform::GetPSFlowFromRtMetaInfo( rtMetaInfoPtr);

  if (NetPlatform::IsPSIfaceCommonTxEnabled( routingCache, psFlowHandle) &&
      SocketIPSec::IsIfaceListFlowEnabled( rtMetaInfoPtr))
  {
    return true;
  }

  return false;
} /* Socket::IsFlowEnabled() */


void Socket::ProcessDoSAckEvent
(
  DS::Sock::Event::DoSAckEventInfo *  dosAckEventInfoPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void) dosAckEventInfoPtr;
  LOG_MSG_ERROR( "DOS_ACK_EV not supported", 0, 0, 0);
  ASSERT( 0);

  return;
} /* Socket::ProcessDoSAckEvent() */


DS::ErrorType Socket::RegNetworkStateChangedEvent
(
  void
)
{
  ISignal *           networkStateChangedSignalPtr = 0;
  ISignalCBFactory *  signalCBFactoryPtr = 0;
  DS::ErrorType       dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);

  /*-------------------------------------------------------------------------
    Create a SignalCBFactory object
  -------------------------------------------------------------------------*/
  dsErrno =
    DSCreateInstance( 0,
                      AEECLSID_DSUtilsSignalCBFactory,
                      0,
                      reinterpret_cast <void **> ( &signalCBFactoryPtr));
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "Couldn't create SignalCBFactory, sock 0x%x", this, 0, 0);
    dsErrno = DSS_ENOMEM;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Create a signal using signalCBFactory
  -------------------------------------------------------------------------*/
  dsErrno =
    signalCBFactoryPtr->CreateSignal( NetworkStateChangedEventCback,
                                      reinterpret_cast <void *> ( this),
                                      &networkStateChangedSignalPtr,
                                      &networkStateChangedSignalCtlPtr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "Couldn't create Signal, sock 0x%x", this, 0, 0);
    dsErrno = DSS_ENOMEM;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register for STATE event with network object
  -------------------------------------------------------------------------*/
  dsErrno = netObjPtr->OnStateChange( networkStateChangedSignalPtr,
                                      NetworkEvent::STATE);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "Couldn't reg ev, sock 0x%x", this, 0, 0);
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Release the Signal object as it is not not needed anymore. Enabling and
    disabling the signal can be done by using the Singnal Ctl object
  -------------------------------------------------------------------------*/
  (void) networkStateChangedSignalPtr->Release();
  networkStateChangedSignalPtr = 0;

  /*-------------------------------------------------------------------------
    Delete SignalCBFactory object. Could have created a SignalCBFactory
    object at power up and not ever delete it but this object is not needed
    most of the time and by deleting the object, less memory is consumed
  -------------------------------------------------------------------------*/
  (void) signalCBFactoryPtr->Release();

  /*-------------------------------------------------------------------------
    Increment socket object's ref cnt as newly created Signal object is also
    maintaing a reference to this object
  -------------------------------------------------------------------------*/
  (void) AddRef();

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x sig ctl 0x%x",
                         this, networkStateChangedSignalCtlPtr, 0);
  return SUCCESS;

  /*-------------------------------------------------------------------------
    Common error handling code - Release all objects
  -------------------------------------------------------------------------*/
bail:
  if (0 != networkStateChangedSignalCtlPtr)
  {
    (void) networkStateChangedSignalCtlPtr->Release();
    networkStateChangedSignalCtlPtr = 0;
  }

  if (0 != networkStateChangedSignalPtr)
  {
    (void) networkStateChangedSignalPtr->Release();
    networkStateChangedSignalPtr = 0;
  }

  if (0 != signalCBFactoryPtr)
  {
    (void) signalCBFactoryPtr->Release();
  }

  return dsErrno;
} /* Socket::RegNetworkStateChangedEvent() */


void Socket::NetworkStateChangedEventCback
(
  void *  userDataPtr
)
{
  Socket *  sockPtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == userDataPtr)
  {
    LOG_MSG_ERROR( "NULL arg", 0, 0, 0);
    ASSERT( 0);
    return;
  }

  sockPtr = reinterpret_cast <Socket *> ( userDataPtr);
  sockPtr->ProcessNetworkStateChangedEvent();

  return;
} /* Socket::NetworkStateChangedEventCback() */


void Socket::ProcessNetworkStateChangedEvent
(
  void
)
{
  ps_route_scope_type     routeScope;
  DS::ErrorType           dsErrno;
  IfaceIdType             ifaceId;
  NetworkStateType        netObjState;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);

  /*-------------------------------------------------------------------------
    Make sure that the object is not deleted.
  -------------------------------------------------------------------------*/
  if (NULL == critSectPtr || 0 == refCnt)
  {
    LOG_MSG_INFO1 ("Obj %p already deleted, return", this, 0, 0);
    return;
  }

  critSectPtr->Enter();

  /*-------------------------------------------------------------------------
    Make sure that platform socket is not closed yet. Else if platform socket
    is deleted, a 0 address would be accessed causing a crash
  -------------------------------------------------------------------------*/
  if (0 == platformSockPtr)
  {
    LOG_MSG_INFO1( "Sock 0x%x closed", this, 0, 0);
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Obtain network object's new state and if it is
      1. OPEN, then update the route scope with the interface pointed to by
         the network object
      2. CLOSED, treat it like receiving IFACE_DOWN_EV
  -------------------------------------------------------------------------*/
  if (SUCCESS != netObjPtr->GetState( &netObjState))
  {
    LOG_MSG_INFO3( "GetState failed, sock 0x%x", this, 0, 0);
    goto bail;
  }

  if (NetworkState::OPEN == netObjState)
  {
    /*-----------------------------------------------------------------------
      Generate route scope and set it in platform
    -----------------------------------------------------------------------*/
    (void) netObjPtr->GetIfaceId( &ifaceId);
    dsErrno = NetPlatform::GetRouteScopeByIfaceId( ifaceId,
                                                   ( Family::INET6 == family),
                                                   &routeScope);
    if (SUCCESS != dsErrno)
    {
      LOG_MSG_INFO3( "GetRouteScopeByIfaceHandle failed, sock 0x%x err 0x%x",
                     this, dsErrno, 0);
      goto bail;
    }

    dsErrno = platformSockPtr->SetRouteScope( &routeScope);
    if (SUCCESS != dsErrno)
    {
      LOG_MSG_INFO3( "SetRouteScope failed, sock 0x%x err 0x%x",
                     this, dsErrno, 0);
      goto bail;
    }
  }
  else if (NetworkState::CLOSED == netObjState)
  {
    /*-----------------------------------------------------------------------
      Set route scope to loopback interfaces
    -----------------------------------------------------------------------*/
    memset( &routeScope, 0, sizeof( ps_route_scope_type));
    (void) NetPlatform::AddLoopbackToRouteScope( ( Family::INET6 == family),
                                                 &routeScope);

    dsErrno = platformSockPtr->SetRouteScope( &routeScope);
    if (SUCCESS != dsErrno)
    {
      LOG_MSG_INFO3( "SetRouteScope failed, sock 0x%x err 0x%x",
                     this, dsErrno, 0);
      goto bail;
    }

    SocketIPSec::ClearIfaceList( rtMetaInfoPtr);
    ProcessNetworkConfigChangedEvent( DSS_ENETDOWN);
  }

  /*-------------------------------------------------------------------------
    Re-enable the signal so that subsequent state changes are received
  -------------------------------------------------------------------------*/
  (void) networkStateChangedSignalCtlPtr->Enable();

  critSectPtr->Leave();

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);
  return;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  critSectPtr->Leave();
  return;

} /* Socket::ProcessNetworkStateChangedEvent() */


void Socket::UpdateRouteScopeByPolicy
(
  void
)
{
  ps_route_scope_type  routeScope;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);

  if (0 != platformSockPtr)
  {
    if (0 == netObjPtr)
    {
      /*---------------------------------------------------------------------
        Generate route scope and set it in platform
      ---------------------------------------------------------------------*/
      if (SUCCESS !=
            NetPlatform::GetRouteScopeByPolicy( policyPtr, &routeScope))
      {
        LOG_MSG_INFO3( "GetRouteScopeByPolicy failed, sock 0x%x", this, 0, 0);
        return;
      }

      if (SUCCESS != platformSockPtr->SetRouteScope( &routeScope))
      {
        LOG_MSG_INFO3( "SetRouteScope failed, sock 0x%x", this, 0, 0);
        return;
      }
    }
  }

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);
  return;

} /* Socket::UpdateRouteScopeByPolicy() */


void Socket::DeleteRoutingCacheFromRouteScope
(
  int32  routingCache
)
{
  ps_route_scope_type  routeScope;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);

  critSectPtr->Enter();

  if (0 != platformSockPtr)
  {
    /*-----------------------------------------------------------------------
      Delete routing cache from route scope and set it in platform
    -----------------------------------------------------------------------*/
    if (SUCCESS != platformSockPtr->GetRouteScope( &routeScope))
    {
      LOG_MSG_INFO3( "GetRouteScope failed, sock 0x%x", this, 0, 0);
      goto bail;
    }

    if (SUCCESS != NetPlatform::DeleteIfaceHandleFromRouteScope( &routeScope,
                                                                 routingCache))
    {
      LOG_MSG_INFO3( "DeleteIfaceHandleFromRouteScope failed, sock 0x%x",
                     this, 0, 0);
      goto bail;
    }

    if (SUCCESS != platformSockPtr->SetRouteScope( &routeScope))
    {
      LOG_MSG_INFO3( "SetRouteScope failed, sock 0x%x", this, 0, 0);
      goto bail;
    }
  }

  critSectPtr->Leave();

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);
  return;

bail:
  critSectPtr->Leave();
  return;

} /* Socket::DeleteRoutingCacheFromRouteScope() */


DS::ErrorType Socket::UpdateRtMetaInfoWithFlowFwding
(
  void
)
{
  ps_rt_meta_info_type *  newRtMetaInfoPtr;
  int32                   defaultPSFlowHandle;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);

  if (0 != platformSockPtr)
  {
    /*-----------------------------------------------------------------------
      Since flow is forwarded on to default flow, rtMetaInfo needs to be
      updated to reflect it
    -----------------------------------------------------------------------*/
    PS_RT_META_INFO_GET( newRtMetaInfoPtr);
    if (0 == newRtMetaInfoPtr)
    {
      LOG_MSG_ERROR( "No mem for Rt meta info", 0, 0, 0);
      //TODO err handling
      return DSS_ENOMEM;
    }

    memcpy( newRtMetaInfoPtr, rtMetaInfoPtr, sizeof( ps_rt_meta_info_type));

    defaultPSFlowHandle =
      NetPlatform::GetDefaultPSFlowFromRtMetaInfo( rtMetaInfoPtr);
    PS_RT_META_SET_FILTER_RESULT( newRtMetaInfoPtr,
                                  IP_FLTR_CLIENT_QOS_OUTPUT,
                                  defaultPSFlowHandle);

    /*-----------------------------------------------------------------------
      Send the new rtMetaInfo to platform
    -----------------------------------------------------------------------*/
    //TODO Look in to error handling
    (void) platformSockPtr->SetRtMetaInfo( newRtMetaInfoPtr);

    /*-----------------------------------------------------------------------
      Store the actual filter result for subsequent event handling
    -----------------------------------------------------------------------*/
    origFltrResult = NetPlatform::GetPSFlowFromRtMetaInfo( rtMetaInfoPtr);

    /*-----------------------------------------------------------------------
      Free the old rtMetaInfo and store the new one in the socket
    -----------------------------------------------------------------------*/
    PS_RT_META_INFO_FREE( &rtMetaInfoPtr);
    rtMetaInfoPtr  = newRtMetaInfoPtr;

    LOG_MSG_INFO1( "Fwding sock 0x%x to default flow", this, 0, 0);

    if (false == IsFlowEnabled())
    {
      SetEventBitMask( Event::WRITE,
                       Event::WRITE_BIT_MASK_FLOW_FWDING_DISABLED);
    }

    ResetEventBitMask( Event::WRITE, Event::WRITE_BIT_MASK_FLOW_DISABLED);
  }

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);
  return SUCCESS;

} /* Socket::UpdateRtMetaInfoWithFlowFwding() */


DS::ErrorType Socket::UseOrigFltrResult
(
  void
)
{
  ps_rt_meta_info_type *   newRtMetaInfoPtr;
  int32                    physLinkHandle;
  ps_flow_state_enum_type  psFlowState;
  phys_link_state_type     physLinkState;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 != platformSockPtr)
  {
    psFlowState  = NetPlatform::GetPSFlowState( origFltrResult);

    (void) NetPlatform::PSGetPhysLinkFromFlow( origFltrResult, &physLinkHandle);
    (void) NetPlatform::PhysLinkIoctl( physLinkHandle,
                                       NetPlatform::PHYS_LINK_IOCTL_GET_STATE,
                                       (void *) &physLinkState);

    if (FLOW_ACTIVATED == psFlowState && PHYS_LINK_UP == physLinkState)
    {
      /*---------------------------------------------------------------------
        Since flow is forwarded on to default flow, rtMetaInfo needs to be
        updated to reflect it
      ---------------------------------------------------------------------*/
      PS_RT_META_INFO_GET( newRtMetaInfoPtr);
      if (0 == newRtMetaInfoPtr)
      {
        LOG_MSG_ERROR( "No mem for Rt meta info", 0, 0, 0);
        //TODO what to do?
        return DSS_ENOMEM;
      }

      memcpy( newRtMetaInfoPtr, rtMetaInfoPtr, sizeof( ps_rt_meta_info_type));
      PS_RT_META_SET_FILTER_RESULT( newRtMetaInfoPtr,
                                    IP_FLTR_CLIENT_QOS_OUTPUT,
                                    origFltrResult);

      /*---------------------------------------------------------------------
        Send the new rtMetaInfo to platform
      ---------------------------------------------------------------------*/
      //TODO Look in to error handling
      (void) platformSockPtr->SetRtMetaInfo( newRtMetaInfoPtr);

      /*---------------------------------------------------------------------
        Free the old rtMetaInfo and store the new one in the socket
      ---------------------------------------------------------------------*/
      PS_RT_META_INFO_FREE( &rtMetaInfoPtr);
      rtMetaInfoPtr  = newRtMetaInfoPtr;
      origFltrResult = 0;

      LOG_MSG_INFO1( "Using origFltrResult, sock 0x%x", this, 0, 0);

      if (false == IsFlowEnabled())
      {
        SetEventBitMask( Event::WRITE, Event::WRITE_BIT_MASK_FLOW_DISABLED);
      }
      else
      {
        ResetEventBitMask( Event::WRITE, Event::WRITE_BIT_MASK_FLOW_DISABLED);
      }

      ResetEventBitMask( Event::WRITE,
                         Event::WRITE_BIT_MASK_FLOW_FWDING_DISABLED);
    }
#if 0
    else
    {
      if (PHYS_LINK_DOWN == physLinkState)
      {
        SetEventBitMask( Event::WRITE,
                         Event::WRITE_BIT_MASK_DORMANT_TRAFFIC_CHANNEL);
      }
    }
#endif
  }

  return SUCCESS;

} /* Socket::UseOrigFltrResult() */


void Socket::UseDefaultPSFlow
(
  void
)
{
  int32  psFlowHandle;
  int32  defaultPSFlowHandle;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  psFlowHandle = NetPlatform::GetPSFlowFromRtMetaInfo( rtMetaInfoPtr);
  defaultPSFlowHandle =
    NetPlatform::GetDefaultPSFlowFromRtMetaInfo( rtMetaInfoPtr);

  if (psFlowHandle != defaultPSFlowHandle)
  {
    (void) UpdateRtMetaInfoWithFlowFwding();
  }

  return;
} /* Socket::UseDefaultPSFlow() */

#endif /* FEATURE_DATA_PS */
