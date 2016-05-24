/*===========================================================================
  FILE: DS_Sock_ICMPSocket.cpp

  OVERVIEW: This file provides implementation of the ICMPSocket class.

  DEPENDENCIES: None

  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_ICMPSocket.cpp#2 $
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
#include "DS_Sock_ICMPSocket.h"
#include "DS_Utils_DebugMsg.h"
#include "DS_Errors.h"
#include "ps_mem.h"

using namespace DS::Sock;
using namespace DS::Error;


/*===========================================================================

                     PUBLIC MEMBER FUNCTIONS

===========================================================================*/
ICMPSocket * ICMPSocket::CreateInstance
(
  FamilyType    _family
)
{
  ICMPSocket *  icmpSocketPtr = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Family %d", _family, 0, 0);

  /*-------------------------------------------------------------------------
    Create a ICMP Socket and initialize it
  -------------------------------------------------------------------------*/
  icmpSocketPtr= new ICMPSocket();
  if (0 == icmpSocketPtr)
  {
    LOG_MSG_ERROR( "No mem for ICMP Sock", 0, 0, 0);
    goto bail;
  }

  if (SUCCESS != icmpSocketPtr->Init( _family, Type::DGRAM, Protocol::ICMP))
  {
    goto bail;
  }

  return icmpSocketPtr;

  /*-------------------------------------------------------------------------
    Common error handling code - Delete the Socket instance if it was created
  -------------------------------------------------------------------------*/
bail:
  if (0 != icmpSocketPtr)
  {
    delete icmpSocketPtr;
  }

  return 0;
} /* ICMPSocket::CreateInstance() */


DS::ErrorType CDECL ICMPSocket::Shutdown
(
  ShutdownDirType  shutdownDir
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void)shutdownDir;
  return DSS_EOPNOTSUPP;
} /* ICMPSocket::Shutdown() */


/*===========================================================================

                      PROTECTED MEMBER FUNCTIONS

===========================================================================*/
bool ICMPSocket::IsConnectSupported
(
  const SockAddrIN6Type *  v6RemoteAddrPtr,
  DS::ErrorType *          dsErrnoPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", this, 0, 0);
  (void)v6RemoteAddrPtr;

  if (0 == dsErrnoPtr)
  {
    LOG_MSG_ERROR( "NULL errno, sock 0x%x", this, 0, 0);
    ASSERT( 0);
    return false;
  }

  /*-------------------------------------------------------------------------
    ICMP doesn't support Connect(). So always return false
  -------------------------------------------------------------------------*/
  *dsErrnoPtr = DSS_EINVAL;

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", this, 0, 0);
  return false;

} /* ICMPSocket::IsConnectSupported() */


bool ICMPSocket::IsOptSupported
(
  OptLevelType  optLevel,
  OptNameType   optName
)
{
  bool  isOptSupported;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x level %d, name %d",
                          this, optLevel, optName);

  if (OptLevel::ICMP == optLevel)
  {
    isOptSupported = true;
  }
  else
  {
    switch (optName)
    {
      case OptName::DSSOCK_IP_TOS:
      case OptName::DSSOCK_IP_TTL:
      case OptName::DSSOCK_IPV6_TCLASS:
      case OptName::DSSOCK_SO_REUSEADDR:
      case OptName::DSSOCK_SO_ERROR:
      case OptName::DSSOCK_SO_ERROR_ENABLE:
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

} /* ICMPSocket::IsOptSupported() */


bool ICMPSocket::IsSetNetPolicySupported
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return true;
} /* ICMPSocket::IsSetNetPolicySupported() */


DS::ErrorType ICMPSocket::FillProtocolInfoInPktInfo
(
  const SockAddrIN6Type *  v6RemoteAddrPtr,
  const SockAddrIN6Type *  v6LocalAddrPtr,
  ip_pkt_info_type *       pktInfoPtr
)
{
  DS::ErrorType  dsErrno;
  int32          optVal;
  int32          optLen;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == v6RemoteAddrPtr || 0 == v6LocalAddrPtr || 0 == pktInfoPtr)
  {
    LOG_MSG_ERROR( "NULL args, sock 0x%x", this, 0, 0);
    ASSERT( 0);
    return DSS_EFAULT;
  }

  if (IP_V6 == pktInfoPtr->ip_vsn)
  {
    pktInfoPtr->ip_hdr.v6.hdr_body.base_hdr.next_hdr = PS_IPPROTO_ICMP;
  }
  else
  {
    pktInfoPtr->ip_hdr.v4.protocol = PS_IPPROTO_ICMP;
  }

  /*-------------------------------------------------------------------------
    Populate ICMP type by fetching it from platform
  -------------------------------------------------------------------------*/
  dsErrno = platformSockPtr->GetOpt( OptLevel::ICMP,
                                     OptName::DSSOCK_ICMP_TYPE,
                                     &optVal,
                                     &optLen);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "GetOpt failed for ICMP_TYPE, sock 0x%x err 0x%x",
                   this, dsErrno, 0);
    return dsErrno;
  }

  pktInfoPtr->ptcl_info.icmp.type = static_cast <uint8> ( optVal);

  /*-------------------------------------------------------------------------
    Populate ICMP code by fetching it from platform
  -------------------------------------------------------------------------*/
  dsErrno = platformSockPtr->GetOpt( OptLevel::ICMP,
                                     OptName::DSSOCK_ICMP_CODE,
                                     &optVal,
                                     &optLen);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "GetOpt failed for ICMP_CODE, sock 0x%x err 0x%x",
                   this, dsErrno, 0);
    return dsErrno;
  }

  pktInfoPtr->ptcl_info.icmp.code = static_cast <uint8> ( optVal);

  return SUCCESS;
} /* ICMPSocket::FillProtocolInfoInPktInfo() */


void ICMPSocket::ProcessNetworkConfigChangedEvent
(
  DS::ErrorType  reasonForChange
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void)reasonForChange;
  SetEventBitMask( Event::WRITE, Event::WRITE_BIT_MASK_NULL_ROUTING_INFO);
  return;

} /* ICMPSocket::ProcessNetworkConfigChangedEvent() */


/*===========================================================================

                         PRIVATE MEMBER FUNCTIONS

===========================================================================*/
void * ICMPSocket::operator new
(
  unsigned int numBytes
) throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void)numBytes;
  return ps_mem_get_buf( PS_MEM_ICMP_SOCKET_TYPE);
} /* ICMPSocket::operator new() */


void ICMPSocket::operator delete
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

} /* ICMPSocket::operator delete() */

#endif /* FEATURE_DATA_PS */
