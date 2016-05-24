/*===========================================================================
  FILE: DS_Sock_UDPSocket.cpp

  OVERVIEW: This file provides implementation of the UDPSocket class.

  DEPENDENCIES: None

  Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_UDPSocket.cpp#7 $
  $DateTime: 2010/02/09 20:58:35 $$Author: smudired $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-05-14 msr Created module

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"

#ifdef FEATURE_DATA_PS
#include "DS_Sock_AddrUtils.h"
#include "DS_Sock_UDPSocket.h"
#include "DS_Utils_DebugMsg.h"
#include "DS_Errors.h"
#include "PS_Sock_Platform_ISocket.h"
#include "ps_mem.h"
#include "ps_iface_ioctl.h"
#include "DS_Net_Platform.h"

using namespace PS::Sock;
using namespace DS::Sock;
using namespace DS::Error;
using namespace NetPlatform;

/*===========================================================================

                     PUBLIC MEMBER FUNCTIONS

===========================================================================*/
UDPSocket * UDPSocket::CreateInstance
(
  FamilyType    _family
)
{
  UDPSocket *  udpSocketPtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Family %d", _family, 0, 0);

  /*-------------------------------------------------------------------------
    Create a UDP Socket and initialize it
  -------------------------------------------------------------------------*/
  udpSocketPtr = new UDPSocket();
  if (0 == udpSocketPtr)
  {
    LOG_MSG_ERROR( "No mem for UDP Sock", 0, 0, 0);
    goto bail;
  }

  if (SUCCESS != udpSocketPtr->Init( _family, Type::DGRAM, Protocol::UDP))
  {
    goto bail;
  }

  return udpSocketPtr;

  /*-------------------------------------------------------------------------
    Common error handling code - Delete the Socket instance if it was created
  -------------------------------------------------------------------------*/
bail:
  if (0 != udpSocketPtr)
  {
    delete udpSocketPtr;
  }

  return 0;
} /* UDPSocket::CreateInstance() */


DS::ErrorType CDECL UDPSocket::SendToDSMChain
(
  dsm_item_type **             dsmItemPtrPtr,
  const SockAddrStorageType *  remoteAddrPtr,
  unsigned int                 flags,
  int32 *                      numWrittenPtr
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
    Call platform specific SendDSMChain(...) and return
  -------------------------------------------------------------------------*/
  dsErrno = platformSockPtr->SendDSMChain( remoteAddrPtr,
                                           dsmItemPtrPtr,
                                           flags,
                                           numWrittenPtr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "SendDSMChain failed, sock 0x%x err 0x%x",
                   this, dsErrno, 0);
    goto bail;
  }

  PS_PKT_META_INFO_FREE( &pktMetaInfoPtr);

  critSectPtr->Leave();

  LOG_MSG_INFO4 ("Success, sock 0x%x wrote %d", this, *numWrittenPtr, 0);
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

} /* UDPSocket::SendToDSMChain() */


DS::ErrorType CDECL UDPSocket::RecvFromDSMChain
(
  dsm_item_type **       dsmItemPtrPtr,
  SockAddrStorageType *  remoteAddrPtr,
  unsigned int           flags,
  int32 *                numReadPtr
)
{
  SockAddrStorageType  remoteSockAddr;
  DS::ErrorType        dsErrno;
  DS::ErrorType        tmpDSErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO4( "Sock 0x%x", this, 0, 0);

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
    dsErrno = DSS_EINVAL;
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
  dsErrno = platformSockPtr->RecvDSMChain( remoteAddrPtr,
                                           dsmItemPtrPtr,
                                           flags,
                                           numReadPtr);

  /*-------------------------------------------------------------------------
    Return ENETDOWN as error if
      1. EWOULDBLOCK is returned from platform AND
      2. Routing cache is NULL AND
      3. Socket is not connected to a peer
  -------------------------------------------------------------------------*/
  if (DSS_EWOULDBLOCK == dsErrno)
  {
    if (0 == rtMetaInfoPtr ||
        0 == PS_RT_META_GET_ROUTING_CACHE( rtMetaInfoPtr))
    {
      tmpDSErrno = platformSockPtr->GetPeerName( &remoteSockAddr);
      if (SUCCESS != tmpDSErrno)
      {
        LOG_MSG_INFO1( "NULL routing info, sock 0x%x", this, 0, 0);
        dsErrno = DSS_ENETDOWN;
      }
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

} /* UDPSocket::RecvFromDSMChain() */


DS::ErrorType CDECL UDPSocket::AddIPMembership
(
  const IPMembershipInfoType * ipMembershipPtr
)
{
  SockAddrStorageType *            localAddrPtr = NULL;
  SockAddrIN6Type                  v6LocalAddr;
  ps_iface_ioctl_mcast_join_type   mcastJoinInfo;
  DS::ErrorType                    dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);

  if (0 == ipMembershipPtr)
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
    dsErrno = DSS_EINVAL;
    goto bail;
  }

  if (0 != mcastHandle)
  {
    LOG_MSG_ERROR( "Sock 0x%x registered for Multicast already, w/handle %d",
                   this, mcastHandle, 0);
    dsErrno = DSS_EINVAL;
    goto bail;
  }

  v6LocalAddr.family = Family::INET6;
  localAddrPtr = reinterpret_cast <SockAddrStorageType *> ( &v6LocalAddr);

  dsErrno = platformSockPtr->GetSockName( localAddrPtr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "GetSockName failed, sock 0x%x err 0x%x", this, dsErrno, 0);
    goto bail;
  }

  if (DS::DSS_AF_INET == ipMembershipPtr->mcastGroup.family)
  {
    mcastJoinInfo.ip_addr.type    = IPV4_ADDR;
    memcpy( &( mcastJoinInfo.ip_addr.addr.v4),
            ipMembershipPtr->mcastGroup.addr,
            sizeof(uint32));
  }

  if (DS::DSS_AF_INET6 == ipMembershipPtr->mcastGroup.family)
  {
     mcastJoinInfo.ip_addr.type    = IPV6_ADDR;
     memcpy( mcastJoinInfo.ip_addr.addr.v6,
             ipMembershipPtr->mcastGroup.addr,
             16);
  }

  mcastJoinInfo.port            = v6LocalAddr.port;
  mcastJoinInfo.mcast_param_ptr = NULL;
  mcastJoinInfo.handle          = 0;

  dsErrno =
    NetPlatform::IfaceIoctlByIfaceId( ipMembershipPtr->ifaceId,
                                      NetPlatform::IFACE_IOCTL_MCAST_JOIN,
                                      static_cast < void *> ( &mcastJoinInfo));
  if (SUCCESS != dsErrno)
  {
    goto bail;
  }

  mcastHandle = mcastJoinInfo.handle;

  critSectPtr->Leave();
  return SUCCESS;

bail:
  critSectPtr->Leave();
  return dsErrno;

} /* UDPSocket::AddIPMembership() */


DS::ErrorType CDECL UDPSocket::DropIPMembership
(
  const IPMembershipInfoType * ipMembershipPtr
)
{
  ps_iface_ioctl_mcast_leave_type  mcastLeaveInfo;
  DS::ErrorType                    dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);

  if (0 == ipMembershipPtr)
  {
    LOG_MSG_INVALID_INPUT( "NULL arg, sock 0x%x", this, 0, 0);
    return DSS_EFAULT;
  }

  critSectPtr->Enter();

  if (0 == mcastHandle)
  {
    dsErrno = SUCCESS;
    goto bail;
  }

  mcastLeaveInfo.handle = mcastHandle;

  dsErrno =
    NetPlatform::IfaceIoctlByIfaceId( ipMembershipPtr->ifaceId,
                                      NetPlatform::IFACE_IOCTL_MCAST_LEAVE,
                                      static_cast < void *> ( &mcastLeaveInfo));
  if (SUCCESS != dsErrno)
  {
    goto bail;
  }

  mcastHandle = 0;

  critSectPtr->Leave();
  return SUCCESS;

bail:
  critSectPtr->Leave();
  return dsErrno;

} /* UDPSocket::DropIPMembership() */


DS::ErrorType CDECL UDPSocket::GetDoSAckInfo
(
  DoSAckStatusType *  dosAckStatusPtr,
  uint32 *            overflowPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);

  if (0 == dosAckStatusPtr || 0 == overflowPtr)
  {
    LOG_MSG_INVALID_INPUT( "NULL arg, sock 0x%x", this, 0, 0);
    return DSS_EFAULT;
  }

  critSectPtr->Enter();

  /*-------------------------------------------------------------------------
    Update out parameters
  -------------------------------------------------------------------------*/
  *dosAckStatusPtr = dosAckStatus;
  *overflowPtr     = overflow;

  /*-------------------------------------------------------------------------
    Reset to default values
  -------------------------------------------------------------------------*/
  dosAckStatus = DoSAckStatus::NONE;
  overflow     = 0;

  ResetEventBitMask( Event::DOS_ACK, Event::DOS_ACK_BIT_MASK_INFO_AVAILABLE);

  critSectPtr->Leave();

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x status %d overflow %d",
                         this, *dosAckStatusPtr, *overflowPtr);
  return SUCCESS;
} /* UDPSocket::GetDoSAckInfo() */


/*===========================================================================

                     PROTECTED MEMBER FUNCTIONS

===========================================================================*/
inline bool UDPSocket::IsMulticastSupported
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return true;
} /* UDPSocket::IsMulticastSupported() */


inline bool UDPSocket::IsConnectSupported
(
  const SockAddrIN6Type *  v6RemoteAddrPtr,
  DS::ErrorType *          dsErrnoPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);
  (void)v6RemoteAddrPtr;
  (void)dsErrnoPtr;

  /*-------------------------------------------------------------------------
    UDP supports both disconnecting a socket and calling Connect() more than
    once on a socket. So always return true without performing any checks.

    Clear routing cache so that route scope is recalculated. Otherwise, socket
    will not be able to receive data from all the interfaces which match
    it's netpolicy
  -------------------------------------------------------------------------*/
  SetEventBitMask( Event::WRITE, Event::WRITE_BIT_MASK_NULL_ROUTING_INFO);

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);
  return true;

} /* UDPSocket::IsConnectSupported() */


bool UDPSocket::IsOptSupported
(
  OptLevelType  optLevel,
  OptNameType   optName
)
{
  bool  isOptSupported;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x level %d, name %d",
                          this, optLevel, optName);

  if (OptLevel::IP == optLevel || OptLevel::IPV6 == optLevel)
  {
    isOptSupported = true;
  }
  else
  {
    switch (optName)
    {
      case OptName::DSSOCK_SO_REUSEADDR:
      case OptName::DSSOCK_SO_ERROR_ENABLE:
      case OptName::DSSOCK_SO_ERROR:
      case OptName::DSSOCK_SO_TX_IFACE:
      case OptName::DSSOCK_SO_RCVBUF:
      case OptName::DSSOCK_SO_SNDBUF:
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

} /* UDPSocket::IsOptSupported() */


bool UDPSocket::IsPktInfoDifferent
(
  const SockAddrStorageType * remoteAddrPtr
)
{
  const SockAddrIN6Type *  v6RemoteAddrPtr;
  const SockAddrINType  *  v4RemoteAddrPtr;
  uint16                   remotePort;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (false == Socket::IsPktInfoDifferent( remoteAddrPtr))
  {
    if (DS::DSS_AF_INET6 == remoteAddrPtr->family)
    {
      v6RemoteAddrPtr =
        reinterpret_cast <const SockAddrIN6Type *> ( remoteAddrPtr);
      remotePort = v6RemoteAddrPtr->port;
    }
    else
    {
      v4RemoteAddrPtr =
        reinterpret_cast <const SockAddrINType *> ( remoteAddrPtr);
      remotePort = v4RemoteAddrPtr->port;
    }

    if (rtMetaInfoPtr->pkt_info.ptcl_info.udp.dst_port != remotePort)
    {
      return true;
    }

    return false;
  }

  return true;
} /* UDPSocket::IsPktInfoDifferent() */


DS::ErrorType UDPSocket::FillProtocolInfoInPktInfo
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
    pktInfoPtr->ip_hdr.v6.hdr_body.base_hdr.next_hdr = PS_IPPROTO_UDP;
  }
  else
  {
    pktInfoPtr->ip_hdr.v4.protocol = PS_IPPROTO_UDP;
  }

  pktInfoPtr->ptcl_info.udp.src_port = v6LocalAddrPtr->port;
  pktInfoPtr->ptcl_info.udp.dst_port = v6RemoteAddrPtr->port;

  return SUCCESS;
} /* UDPSocket::FillProtocolInfoInPktInfo() */


void UDPSocket::ProcessNetworkConfigChangedEvent
(
  DS::ErrorType  reasonForChange
)
{
  SockAddrStorageType  remoteAddr;
  SockAddrIN6Type *    v6RemoteAddrPtr;
  DS::ErrorType        dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void)reasonForChange;

  v6RemoteAddrPtr = reinterpret_cast <SockAddrIN6Type *> ( &remoteAddr);

  memset( v6RemoteAddrPtr, 0, sizeof( SockAddrIN6Type));
  v6RemoteAddrPtr->family = DS::DSS_AF_INET6;

  /*-------------------------------------------------------------------------
    Make sure that platform socket is not closed yet. Else if platform socket
    is deleted, a 0 address would be accessed causing a crash
  -------------------------------------------------------------------------*/
  if (0 == platformSockPtr)
  {
    LOG_MSG_INFO1( "Sock 0x%x closed", this, 0, 0);
    return;
  }

  /*-------------------------------------------------------------------------
    Disconnect the platform socket as local address has changed
  -------------------------------------------------------------------------*/
  dsErrno = platformSockPtr->Connect( &remoteAddr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "Connect failed, sock 0x%x err 0x%x", this, dsErrno, 0);
    return;
  }

  SetEventBitMask( Event::WRITE, Event::WRITE_BIT_MASK_NULL_ROUTING_INFO);
  return;

} /* UDPSocket::ProcessNetworkConfigChangedEvent() */


void UDPSocket::ProcessDoSAckEvent
(
  Event::DoSAckEventInfo *  dosAckEventInfoPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ASSERT( 0 != dosAckEventInfoPtr);

  dosAckStatus = dosAckEventInfoPtr->dosAckStatus;
  overflow     = dosAckEventInfoPtr->overflow;

  SetEventBitMask( Event::DOS_ACK, Event::DOS_ACK_BIT_MASK_INFO_AVAILABLE);
  return;

} /* UDPSocket::ProcessDoSAckEvent() */


/*===========================================================================

                         PRIVATE MEMBER FUNCTIONS

===========================================================================*/
void * UDPSocket::operator new
(
  unsigned int numBytes
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void)numBytes;
  return ps_mem_get_buf( PS_MEM_UDP_SOCKET_TYPE);
} /* UDPSocket::operator new() */


void UDPSocket::operator delete
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

} /* UDPSocket::operator delete() */

#endif /* FEATURE_DATA_PS */
