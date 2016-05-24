/*===========================================================================
  FILE: DS_Sock_TCPSocket.cpp

  OVERVIEW: This file provides implementation of the TCPSocket class.

  DEPENDENCIES: None

  Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_TCPSocket.cpp#7 $
  $DateTime: 2010/05/13 07:57:24 $$Author: smudired $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-05-14 msr Created module

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"

#ifdef FEATURE_DATA_PS
#include "DS_Sock_SocketFactory.h"
#include "DS_Sock_TCPSocket.h"
#include "DS_Sock_SocketDef.h"
#include "DS_Utils_DebugMsg.h"
#include "PS_Sock_Platform_ISocket.h"
#include "ps_mem.h"

using namespace PS::Sock;
using namespace DS::Sock;
using namespace DS::Error;
using namespace DS::Utils;


/*===========================================================================

                     PUBLIC MEMBER FUNCTIONS

===========================================================================*/
TCPSocket * TCPSocket::CreateInstance
(
  FamilyType  _family
)
{
  TCPSocket *  tcpSocketPtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Family %d", _family, 0, 0);

  /*-------------------------------------------------------------------------
    Create a TCP Socket
  -------------------------------------------------------------------------*/
  tcpSocketPtr = new TCPSocket();
  if (0 == tcpSocketPtr)
  {
    LOG_MSG_ERROR( "No mem for TCP Sock", 0, 0, 0);
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Initiaize the socket
  -------------------------------------------------------------------------*/
  if (SUCCESS != tcpSocketPtr->Init( _family, Type::STREAM, Protocol::TCP))
  {
    goto bail;
  }

  return tcpSocketPtr;

  /*-------------------------------------------------------------------------
    Common error handling code - Delete the Socket instance if it was created
  -------------------------------------------------------------------------*/
bail:
  if (0 != tcpSocketPtr)
  {
    delete tcpSocketPtr;
  }

  return 0;
} /* TCPSocket::CreateInstance() */


DS::ErrorType CDECL TCPSocket::Listen
(
  int32  backlog
)
{
  DS::ErrorType    dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x backlog %d", this, backlog, 0);

  /*-------------------------------------------------------------------------
    Make sure that backlog > 0
  -------------------------------------------------------------------------*/
  if (0 >= backlog)
  {
    LOG_MSG_INVALID_INPUT( "Invalid backlog %d, sock 0x%x", backlog, this, 0);
    return DSS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Truncate backlog to SO_MAX_CONN and call platform specific Listen(...)
  -------------------------------------------------------------------------*/
  if (backlog > DSSOCK_SOMAXCONN)
  {
    backlog = DSSOCK_SOMAXCONN;
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

  dsErrno = platformSockPtr->Listen( backlog);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_ERROR( "Listen failed, sock 0x%x err 0x%x", this, dsErrno, 0);
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

} /* TCPSocket::Listen() */


DS::ErrorType CDECL TCPSocket::Accept
(
  SockAddrStorageType *  remoteAddrPtr,
  ISocket **             newSockPtr
)
{
  TCPSocket *                 newTCPSockPtr      = 0;
  Platform::ISocket *         newPlatformSockPtr = 0;
  Platform::IEventListener *  eventListenerPtr;
  INode *                     iNodePtr;
  SocketFactory *             sockFactoryPtr;
  LingerType                  soLinger;
  DS::ErrorType               dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);

  if (0 == remoteAddrPtr || 0 == newSockPtr)
  {
    LOG_MSG_INVALID_INPUT( "NULL args, sock 0x%x", this, 0, 0);
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
    Return DSS_EWOULDBLOCK if ACCEPT event is flow controlled
  -------------------------------------------------------------------------*/
  if (0 != eventBitMask[Event::ACCEPT])
  {
    dsErrno = DSS_EWOULDBLOCK;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Check if a new connection is established
  -------------------------------------------------------------------------*/
  dsErrno = platformSockPtr->Accept( &newPlatformSockPtr, remoteAddrPtr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "Accept failed, sock 0x%x err 0x%x", this, dsErrno, 0);

    goto bail;
  }

  /*-------------------------------------------------------------------------
    A new connection is available. As newly accepted socket should have exact
    same configuration as the listener socket, clone the current socket and
    set platformSockPtr
  -------------------------------------------------------------------------*/
  newTCPSockPtr = new TCPSocket();
  if (0 == newTCPSockPtr)
  {
    LOG_MSG_INFO3( "No mem for sock", 0, 0, 0);
    dsErrno = DSS_ENOMEM;
    goto bail;
  }

  newTCPSockPtr->platformSockPtr = newPlatformSockPtr;
  dsErrno = newTCPSockPtr->CloneSocket( this);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "CloneSocket failed, sock 0x%x err 0x%x", this, dsErrno, 0);
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register an event listener with platform socket
  -------------------------------------------------------------------------*/
  eventListenerPtr = static_cast <Platform::IEventListener *> ( newTCPSockPtr);
  newPlatformSockPtr->RegEventListener( eventListenerPtr);

  /*-------------------------------------------------------------------------
    Update critical section for the platform socket
  -------------------------------------------------------------------------*/
  newPlatformSockPtr->SetCritSection( newTCPSockPtr->critSectPtr);

  /*-------------------------------------------------------------------------
    Register with socket platform for PLATFORM_ENABLED event
  -------------------------------------------------------------------------*/
  dsErrno = newPlatformSockPtr->AsyncSelect( Platform::Event::PLATFORM_ENABLED);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "AsyncSelect failed, sock 0x%x err 0x%x", this, dsErrno, 0);
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register with socket platform for PLATFORM_DISABLED event
  -------------------------------------------------------------------------*/
  dsErrno =
    newPlatformSockPtr->AsyncSelect( Platform::Event::PLATFORM_DISABLED);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "AsyncSelect failed, sock 0x%x err 0x%x", this, dsErrno, 0);
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Add the Accept Socket to the SocketFactory list
  -------------------------------------------------------------------------*/
  sockFactoryPtr = SocketFactory::CreateInstance();
  ASSERT( 0 != sockFactoryPtr);

  iNodePtr = static_cast <DS::Utils::INode *> ( newTCPSockPtr);

  (void) sockFactoryPtr->AddItem( iNodePtr);
  (void) sockFactoryPtr->Release();

  *newSockPtr = static_cast <ISocket *> ( newTCPSockPtr);
  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x new sock 0x%x",
                         this, newTCPSockPtr, 0);

  critSectPtr->Leave();
  return SUCCESS;

  /*-------------------------------------------------------------------------
    Common error handling code - Free accepted socket
  -------------------------------------------------------------------------*/
bail:
  if (0 != newPlatformSockPtr)
  {
    /*-----------------------------------------------------------------------
      Set linger timeout to 0 to force silent close. Otherwise, platform will
      merely change the socket to CLOSED state and wait for sock lib to call
      Close() again to delete it. Since the app doesn't know about this
      socket, a second Close() will never be called resulting in memory leak
    -----------------------------------------------------------------------*/
    soLinger.isLingerOn = TRUE;
    soLinger.timeInSec  = 0;

    (void) newPlatformSockPtr->SetSOLingerReset( &soLinger);
    (void) newPlatformSockPtr->Close();
  }

  if (0 != newTCPSockPtr)
  {
    delete newTCPSockPtr;
  }

  critSectPtr->Leave();
  return dsErrno;

} /* TCPSocket::Accept() */


DS::ErrorType CDECL TCPSocket::WriteDSMChain
(
  dsm_item_type **  dsmItemPtrPtr,
  int32 *           numWrittenPtr
)
{
  SockAddrStorageType  remoteSockAddr;
  DS::ErrorType        dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO4( "Sock 0x%x", this, 0, 0);

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
    Call platform specific SendDSMChain(...) and return
  -------------------------------------------------------------------------*/
  dsErrno = platformSockPtr->SendDSMChain( 0,
                                           dsmItemPtrPtr,
                                           0,
                                           numWrittenPtr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "SendDSMChain failed, sock 0x%x err 0x%x",
                   this, dsErrno, 0);
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

} /* TCPSocket::WriteDSMChain() */


DS::ErrorType CDECL TCPSocket::ReadDSMChain
(
  dsm_item_type **  dsmItemPtrPtr,
  int32 *           numReadPtr
)
{
  DS::ErrorType  dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO4( "Sock 0x%x", this, 0, 0);

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
    Call platform specific RecvDSMChain(...) and return
  -------------------------------------------------------------------------*/
  dsErrno = platformSockPtr->RecvDSMChain( 0, dsmItemPtrPtr, 0, numReadPtr);

  /*-------------------------------------------------------------------------
    Return ENETDOWN as error if
      1. EWOULDBLOCK is returned from platform AND
      2. Routing cache is NULL AND
  -------------------------------------------------------------------------*/
  if (DSS_EWOULDBLOCK == dsErrno)
  {
    if (0 == rtMetaInfoPtr ||
        0 == PS_RT_META_GET_ROUTING_CACHE( rtMetaInfoPtr))
    {
      LOG_MSG_INFO1( "NULL routing info, sock 0x%x", this, 0, 0);
      dsErrno = DSS_ENETDOWN;
    }
  }

  critSectPtr->Leave();

  LOG_MSG_INFO4( "Sock 0x%x read %d err 0x%x", this, *numReadPtr, dsErrno);
  return dsErrno;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  critSectPtr->Leave();
  return dsErrno;

} /* TCPSocket::ReadDSMChain() */


/*===========================================================================

                     PROTECTED MEMBER FUNCTIONS

===========================================================================*/
bool TCPSocket::IsConnectSupported
(
  const SockAddrIN6Type *  v6RemoteAddrPtr,
  DS::ErrorType *          dsErrnoPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);

  if (0 == dsErrnoPtr)
  {
    LOG_MSG_ERROR( "NULL errno, sock 0x%x", this, 0, 0);
    ASSERT( 0);
    return false;
  }

  if (0 == v6RemoteAddrPtr)
  {
    LOG_MSG_ERROR( "NULL peer addr, sock 0x%x", this, 0, 0);
    ASSERT( 0);
    *dsErrnoPtr = DSS_EFAULT;
    return false;
  }

  /*-------------------------------------------------------------------------
    App is trying to disconnect a socket, if it passes ps_in6addr_any as remote
    addr. Since TCP doesn't support disconnecting a socket, return false
  -------------------------------------------------------------------------*/
  if (PS_IN6_ARE_ADDR_EQUAL( v6RemoteAddrPtr->addr, &ps_in6addr_any))
  {
    LOG_MSG_ERROR( "Disconnect not supported, sock 0x%x", this, 0, 0);
    *dsErrnoPtr = DSS_EADDRREQ;
    return false;
  }

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);
  return true;

} /* TCPSocket::IsConnectSupported() */


bool TCPSocket::IsOptSupported
(
  OptLevelType  optLevel,
  OptNameType   optName
)
{
  bool  isOptSupported;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x level %d, name %d",
                          this, optLevel, optName);

  if (OptLevel::TCP == optLevel)
  {
    isOptSupported = true;
  }
  else
  {
    switch (optName)
    {
      case OptName::DSSOCK_SO_KEEPALIVE:
      case OptName::DSSOCK_SO_TX_IFACE:
      case OptName::DSSOCK_SO_RCVBUF:
      case OptName::DSSOCK_SO_SNDBUF:
      case OptName::DSSOCK_IP_TOS:
      case OptName::DSSOCK_IP_TTL:
      case OptName::DSSOCK_IPV6_TCLASS:
      {
        isOptSupported = true;
        break;
      }

      default:
      {
        isOptSupported = false;
        break;
      }
    }
  }

  LOG_MSG_FUNCTION_EXIT( "Sock 0x%x ret %d", this, isOptSupported, 0);
  return isOptSupported;

} /* TCPSocket::IsOptSupported() */


bool TCPSocket::IsPktInfoDifferent
(
  const SockAddrStorageType * remoteAddrPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void)remoteAddrPtr;
  return false;
} /* TCPSocket::IsPktInfoDifferent() */


DS::ErrorType TCPSocket::FillProtocolInfoInPktInfo
(
  const SockAddrIN6Type *  v6RemoteAddrPtr,
  const SockAddrIN6Type *  v6LocalAddrPtr,
  ip_pkt_info_type *       pktInfoPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == v6RemoteAddrPtr || 0 == v6LocalAddrPtr || 0 == pktInfoPtr)
  {
    LOG_MSG_ERROR( "NULL args, sock 0x%x", this, 0, 0);
    ASSERT( 0);
    return DSS_EFAULT;
  }

  if (IP_V6 == pktInfoPtr->ip_vsn)
  {
    pktInfoPtr->ip_hdr.v6.hdr_body.base_hdr.next_hdr = PS_IPPROTO_TCP;
  }
  else
  {
    pktInfoPtr->ip_hdr.v4.protocol = PS_IPPROTO_TCP;
  }

  pktInfoPtr->ptcl_info.tcp.src_port = v6LocalAddrPtr->port;
  pktInfoPtr->ptcl_info.tcp.dst_port = v6RemoteAddrPtr->port;

  return SUCCESS;
} /* TCPSocket::FillProtocolInfoInPktInfo() */


void TCPSocket::ProcessNetworkConfigChangedEvent
(
  DS::ErrorType  reasonForChange
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 != platformSockPtr)
  {
    /*-------------------------------------------------------------------------
      Call in to platform to abort connection
    -------------------------------------------------------------------------*/
    if (SUCCESS != platformSockPtr->AbortConnection( reasonForChange))
    {
      LOG_MSG_INFO3( "AbortConnection failed, sock 0x%x", this, 0, 0);
    }
    else
    {
      platformSockPtr = 0;
    }
  }

  return;
} /* TCPSocket::ProcessNetworkConfigChangedEvent() */


/*===========================================================================

                         PRIVATE MEMBER FUNCTIONS

===========================================================================*/
void * TCPSocket::operator new
(
  unsigned int numBytes
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void)numBytes;
  return ps_mem_get_buf( PS_MEM_TCP_SOCKET_TYPE);
} /* TCPSocket::operator new() */


void TCPSocket::operator delete
(
  void *  bufPtr
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == bufPtr)
  {
    LOG_MSG_ERROR( "NULL ptr", 0, 0, 0);
    ASSERT( 0);
    return;
  }

  PS_MEM_FREE( bufPtr);
  return;

} /* TCPSocket::operator delete() */

#endif /* FEATURE_DATA_PS */
