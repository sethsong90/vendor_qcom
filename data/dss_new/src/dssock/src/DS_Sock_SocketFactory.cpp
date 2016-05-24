/*===========================================================================
  FILE: DS_Sock_SocketFactory.cpp

  OVERVIEW: This file provides implementation of the SocketFactory class.

  DEPENDENCIES: None

  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_SocketFactory.cpp#1 $
  $DateTime: 2009/09/28 14:45:31 $$Author: dmudgal $

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
#include "DS_Sock_UDPSocket.h"
#include "DS_Sock_ICMPSocket.h"
#include "DS_Utils_DebugMsg.h"
#include "DS_Errors.h"
#include "ps_mem.h"

using namespace DS::Sock;
using namespace DS::Error;
using namespace DS::Utils;
using namespace DS::Net;


/*===========================================================================

                         PUBLIC DATA DECLARATIONS

===========================================================================*/
/*---------------------------------------------------------------------------
  Declaration of static member of SocketFactory class
---------------------------------------------------------------------------*/
SocketFactory * SocketFactory::sockFactoryPtr = 0;


/*===========================================================================

                         PUBLIC MEMBER FUNCTIONS

===========================================================================*/
SocketFactory * SocketFactory::CreateInstance
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Allocate a SocketFactory object if it is not already allocated.

    Since factory pattern is used, a new object is not allocated each time
    CreateInstance() is called
  -------------------------------------------------------------------------*/
  if (0 == sockFactoryPtr)
  {
    sockFactoryPtr = new SocketFactory();

    if (0 == sockFactoryPtr)
    {
      LOG_MSG_ERROR( "No mem for SocketFactory", 0, 0, 0);
      goto bail;
    }
  }

  LOG_MSG_FUNCTION_EXIT( "Returning 0x%p", sockFactoryPtr, 0, 0);
  return sockFactoryPtr;

bail:
  return 0;

} /* SocketFactory::CreateInstance() */


DS::ErrorType CDECL SocketFactory::CreateSocket
(
  FamilyType    family,
  SocketType    socketType,
  ProtocolType  protocol,
  ISocket **    newSockPtrPtr
)
{
  Socket *  sockPtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Family %d type %d proto %d",
                          family, socketType, protocol);

  /*-------------------------------------------------------------------------
    Validate the input.
      1. Family must be either INET or INET6
      2. Socket type must be either STREAM or DGRAM
      3. Protocol must be one of TCP, UDP, ICMP, or UNSPEC
      4. Protocol could be unspecified in which case TCP is used for STREAM
         sockets and UDP is used for DGRAM sockets
      5. (socketType, protocol) tuple must be either (STREAM, TCP), or
         (DGRAM, UDP), or (DGRAM, ICMP)
      6. newSockPtrPtr must be non-NULL
  -------------------------------------------------------------------------*/
  if (Family::INET != family && Family::INET6 != family)
  {
    LOG_MSG_INVALID_INPUT( "Invalid family %d", family, 0, 0);
    return DSS_EAFNOSUPPORT;
  }

  if (Type::STREAM != socketType && Type::DGRAM != socketType)
  {
    LOG_MSG_INVALID_INPUT( "Invalid type %d", socketType, 0, 0);
    return DSS_ESOCKNOSUPPORT;
  }

  if (Protocol::UNSPEC == protocol)
  {
    protocol = (socketType == Type::STREAM) ? Protocol::TCP : Protocol::UDP;
  }

  if (false == ( Protocol::TCP == protocol || Protocol::UDP == protocol ||
                 ( true == IsICMPSupported() && Protocol::ICMP == protocol)))
  {
    LOG_MSG_INVALID_INPUT( "Invalid proto %d", protocol, 0, 0);
    return DSS_EPROTONOSUPPORT;
  }

  if (( Type::STREAM == socketType && Protocol::TCP != protocol) ||
      ( Type::DGRAM == socketType &&
        ( Protocol::UDP != protocol && Protocol::ICMP != protocol)))
  {
    LOG_MSG_INVALID_INPUT( "Invalid (type %d, proto %d) combo",
                           socketType, protocol, 0);
    return DSS_EPROTOTYPE;
  }

  if (0 == newSockPtrPtr)
  {
    LOG_MSG_INVALID_INPUT( "NULL ptr", 0, 0, 0);
    return DSS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Create socket
  -------------------------------------------------------------------------*/
  if (Protocol::TCP == protocol)
  {
    sockPtr = static_cast <Socket *> ( TCPSocket::CreateInstance( family));
  }
  else if (Protocol::UDP == protocol)
  {
    sockPtr = static_cast <Socket *> ( UDPSocket::CreateInstance( family));
  }
  else
  {
    sockPtr = static_cast <Socket *> ( ICMPSocket::CreateInstance( family));
  }

  if (0 == sockPtr)
  {
    LOG_MSG_ERROR( "No mem for Sock", 0, 0, 0);
    return DSS_EMFILE;
  }

  /*-------------------------------------------------------------------------
    Add socket to the list of Socket objects
  -------------------------------------------------------------------------*/
  (void) AddItem( static_cast <INode *> ( sockPtr));

  *newSockPtrPtr = static_cast <ISocket *> ( sockPtr);
  LOG_MSG_FUNCTION_EXIT( "Returning sock 0x%p", *newSockPtrPtr, 0, 0);
  return SUCCESS;

} /* SocketFactory::CreateSocket() */


DS::ErrorType CDECL SocketFactory::CreateSocketByPolicy
(
  FamilyType      family,
  SocketType      socketType,
  ProtocolType    protocol,
  IPolicy *       netPolicyPtr,
  ISocket **      newSockPtrPtr
)
{
  DS::ErrorType  dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Family %d type %d proto %d",
                          family, socketType, protocol);

  if (0 == netPolicyPtr)
  {
    LOG_MSG_INVALID_INPUT( "NULL policy", 0, 0, 0);
    return DSS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Create socket with a default policy
  -------------------------------------------------------------------------*/
  dsErrno = CreateSocket( family, socketType, protocol, newSockPtrPtr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "Couldn't create sock, err %d", dsErrno, 0, 0);
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Update socket's policy to the user passed in policy
  -------------------------------------------------------------------------*/
  dsErrno = (*newSockPtrPtr)->SetNetPolicy( netPolicyPtr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "Couldn't set policy, sock 0x%p err %d",
                   *newSockPtrPtr, dsErrno, 0);
    goto bail;
  }

  LOG_MSG_FUNCTION_EXIT( "Returning sock 0x%p", *newSockPtrPtr, 0, 0);
  return SUCCESS;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  if (0 != *newSockPtrPtr)
  {
    (void) (*newSockPtrPtr)->Release();
  }

  return dsErrno;

} /* SocketFactory::CreateSocketByPolicy() */


DS::ErrorType CDECL SocketFactory::CreateSocketByNetwork
(
  FamilyType      family,
  SocketType      socketType,
  ProtocolType    protocol,
  INetwork *      dsNetObjPtr,
  ISocket **      newSockPtrPtr
)
{
  Socket *       sockPtr;
  DS::ErrorType  dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Family %d type %d proto %d",
                          family, socketType, protocol);

  if (0 == dsNetObjPtr)
  {
    LOG_MSG_INVALID_INPUT( "NULL net obj", 0, 0, 0);
    return DSS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Create socket
  -------------------------------------------------------------------------*/
  dsErrno = CreateSocket( family, socketType, protocol, newSockPtrPtr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_INFO3( "Couldn't create sock, err %d", dsErrno, 0, 0);
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Force socket to use interface pointed to by the user passed in network
    object.

    Since SetNetwork is not a method of ISocket interface, ISocket object is
    cast to Socket object
  -------------------------------------------------------------------------*/
  sockPtr = reinterpret_cast <Socket *> ( *newSockPtrPtr);
  dsErrno = sockPtr->SetNetwork( dsNetObjPtr);
  if (SUCCESS != dsErrno)
  {
    LOG_MSG_ERROR( "Couldn't set network, sock 0x%p err %d",
                   *newSockPtrPtr, dsErrno, 0);
    goto bail;
  }

  LOG_MSG_FUNCTION_EXIT( "Returning sock 0x%p", *newSockPtrPtr, 0, 0);
  return SUCCESS;

  /*-------------------------------------------------------------------------
    Common error handling code
  -------------------------------------------------------------------------*/
bail:
  if (0 != *newSockPtrPtr)
  {
    (void) (*newSockPtrPtr)->Release();
  }

  return dsErrno;

} /* SocketFactory::CreateSocketByNetwork() */


DS::ErrorType SocketFactory::DeleteSocket
(
  Socket *  sockPtr
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == sockPtr)
  {
    LOG_MSG_ERROR( "NULL arg", 0, 0, 0);
    ASSERT( 0);
    return DSS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Remove object from the list of socket objects
  -------------------------------------------------------------------------*/
  RemoveItem( static_cast <INode *> ( sockPtr));
  return SUCCESS;

} /* SocketFactory::DeleteSocket() */


/*===========================================================================

                          PROTECTED MEMBER FUNCTIONS

===========================================================================*/
SocketFactory::SocketFactory
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_EXIT( "Created factory 0x%p", this, 0, 0);

} /* SocketFactory::SocketFactory() */


SocketFactory::~SocketFactory
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  sockFactoryPtr = 0;
  LOG_MSG_FUNCTION_EXIT( "Deleted factory 0x%p", this, 0, 0);

} /* SocketFactory::~SocketFactory() */


/*===========================================================================

                         PRIVATE MEMBER FUNCTIONS

===========================================================================*/
inline void * SocketFactory::operator new
(
  unsigned int numBytes
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void) numBytes;
  return ps_mem_get_buf( PS_MEM_SOCKET_FACTORY_TYPE);

} /* SocketFactory::operator new() */


inline void SocketFactory::operator delete
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

  PS_MEM_FREE( sockFactoryPtr);
  return;

} /* SocketFactory::operator delete() */


bool SocketFactory::IsICMPSupported
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return false;
} /* SocketFactory::IsICMPSupported() */

#endif /* FEATURE_DATA_PS */
