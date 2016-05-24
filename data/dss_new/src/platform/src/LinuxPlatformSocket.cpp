/*===========================================================================
  @file LinuxPlatformSocket.cpp

  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header:$
  $DateTime:$

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  05/04/10   ar  Created module.

===========================================================================*/

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <asm/ioctls.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "amssassert.h"
#include "LinuxPlatformSocket.h"
#include "LinuxPlatformSocketEventManager.h"
#include "DS_Utils_DebugMsg.h"
#include "DS_Utils_Factory.h"

extern "C"
{
  #include "dssocket_defs.h"
  #include "ps_iface.h"
}


using namespace PS::Sock::Platform;
using namespace PS::Sock::Platform::EventManager;

#define DSS_RECV_BUFFER_SIZE  (2000)

LinuxSocketFactory * LinuxSocketFactory::sockFactoryPtr = NULL;
uint8                LinuxSocketFactory::refCnt = 0;

/*-----------------------------------------------------------------------------
  Socket Platform Initialization Functions
-----------------------------------------------------------------------------*/
void PS::Sock::Platform::PowerUpInit(void)
{
  /* Init Event Manager for Platform sockets */
  EventManager::Init();

  //MCT needed?
  //MemoryManager::Init();

} /* PowerUpInit() */

void PS::Sock::Platform::Init(void)
{
  //sock factory create instance, get global event listener ptr, delete instance
  //reg events...
  
} /* Init() */

extern "C" {
  PS::Sock::Platform::ISocketFactory *SFptr = NULL;

  void PlatformSockPowerUp( void )
  {
    SFptr = PS::Sock::Platform::ISocketFactory::CreateInstance();
    PS::Sock::Platform::PowerUpInit();
  }
  
  void PlatformSockPowerDown( void )
  {
    SFptr->DeleteInstance();
  }

  void PlatformSockInit( void )
  {
    PS::Sock::Platform::Init();
  }

  void PlatformSockDeInit( void )
  {
  }
}

/*-----------------------------------------------------------------------------
  Platform <- DS -> Platform  - Platform type mapping functions
-----------------------------------------------------------------------------*/
DS::ErrorType LinuxGetError( int errCode )
{
  using namespace PS::Sock::Platform;

  switch(errCode)
  {
    case ENETDOWN:          // The network subsystem has failed.
      return DS::Error::DSS_ENETDOWN;      // network subsytem unavailable
    case EADDRINUSE:        // The socket's local address is already in use and the socket was not marked to allow address reuse with SO_REUSEADDR. This error usually occurs when executing bind, but could be delayed until this function 
      return DS::Error::DSS_EADDRINUSE;    // address already in use
    case EINPROGRESS:       // Operation now in progress.
    case EALREADY:          // A nonblocking connect call is in progress on the specified socket.
    case EWOULDBLOCK:       //The socket is marked as nonblocking and the connection cannot be completed immediately.
//  case EAGAIN:            //(same as EWOULDBLOCK) The socket is marked as nonblocking and no connection pending for accept 
      return DS::Error::DSS_EWOULDBLOCK;   // connection already established
    case EADDRNOTAVAIL:     // The remote address is not a valid address (such as ADDR_ANY).
      return DS::Error::DSS_EFAULT;        // bad argument
    case EAFNOSUPPORT:      // Addresses in the specified family cannot be used with this socket.
      return DS::Error::DSS_EAFNOSUPPORT;  // address family not supported
    case ECONNREFUSED:      //The attempt to connect was forcefully rejected.
      return DS::Error::DSS_ECONNREFUSED;  //connection attempt refused
    case EFAULT:            //The sockaddr structure pointed to by the name contains incorrect address format for the associated address family or the namelen parameter is too small. 
      return DS::Error::DSS_EFAULT;        // invalid argument
    case EINVAL:            //Invalid argument specified
       return DS::Error::DSS_EINVAL;       //Socket parameter not supported
    case EISCONN:           //The socket is already connected (connection-oriented sockets only).
      return DS::Error::DSS_EISCONN;       //connection already established
    case ENETUNREACH:       //The network cannot be reached from this host at this time.
      ASSERT(0);                           // Verify semantics with Satish
      return DS::Error::DSS_ENETUNREACH;   //network is unreachable
    case EHOSTUNREACH:      //A socket operation was attempted to an unreachable host.
      ASSERT(0);  // Verify semantics with Satish
      return DS::Error::DSS_EHOSTUNREACH;  // no route to host
    case ENOBUFS:           //The output queue for a network interface was full
    case ENOMEM:            //No memory available
      return DS::Error::DSS_ENOMEM;        // out of memory
    case ENOTSOCK:          //The descriptor specified in the s parameter is not a socket.
    case EBADF:            // Invalid descriptor specified
      return DS::Error::DSS_EBADF;         // invalid socket descriptor
    case ETIMEDOUT:         //An attempt to connect timed out without establishing a connection.
      return DS::Error::DSS_ETIMEDOUT;     // Connection attempt timed out
    case EACCES:            //An attempt to connect a datagram socket to broadcast address failed because setsockopt option SO_BROADCAST is not enabled.
      ASSERT(0);                           // Verify semantics with Satish
      return DS::Error::DSS_EINVAL;        //
    case ENOTCONN:          //The socket is not connected.
      return DS::Error::DSS_ENOTCONN;       //socket not connected
    case EOPNOTSUPP:        // listen is not supported by this socket
      return DS::Error::DSS_ESOCKNOSUPPORT;// Socket parameter not supported
    case EMFILE:            // no more socket descriptors available
      return DS::Error::DSS_EMFILE;        // No more sockets available for opening
    case EMSGSIZE:          // socket requires message be sent atomically, but message size made this impossible.
      return DS::Error::DSS_EMSGSIZE;      // Message size limit exceeded
    case EPIPE:             // local end has been shut down on a connection oriented socket
      return DS::Error::DSS_EPIPE;         // Connection closed
    case EINTR:             // A signal occurred.
    default:
    {
      MSG_ERROR("Unhandled type: %d", errCode,0,0);
      return DS::Error::DSS_ESYSTEM;
    }
  }
} /* LinuxGetError() */

static int LinuxMapSOLevel(DS::Sock::OptLevelType optLevel)
{
  switch(optLevel)
  {
    case DS::Sock::OptLevel::SOCKET:
      return SOL_SOCKET;
    case DS::Sock::OptLevel::TCP:
      return IPPROTO_TCP;
    case DS::Sock::OptLevel::UDP:
      return IPPROTO_UDP;
    case DS::Sock::OptLevel::IP:
      return IPPROTO_IP;
    case DS::Sock::OptLevel::IPV6:
      return IPPROTO_IPV6;
    default:
      MSG_ERROR("Unhandled type: %d", optLevel,0,0);
      return DSS_ERROR;
  }
  return DSS_ERROR;
} /* LinuxMapSOLevel() */

static int LinuxMapSOName(DS::Sock::OptNameType optName)
{
  switch(optName)
  {
    case DS::Sock::OptName::DSSOCK_SO_KEEPALIVE:
      return SO_KEEPALIVE;
    case DS::Sock::OptName::DSSOCK_SO_REUSEADDR:
      return SO_REUSEADDR;
    case DS::Sock::OptName::DSSOCK_SO_RCVBUF:
      return SO_RCVBUF;
    case DS::Sock::OptName::DSSOCK_SO_SNDBUF:
      return SO_SNDBUF;
    case DS::Sock::OptName::DSSOCK_TCP_FIONREAD:
      return FIONREAD;
    case DS::Sock::OptName::DSSOCK_IP_MULTICAST_TTL:
      return IP_MULTICAST_TTL;
    case DS::Sock::OptName::DSSOCK_IP_MULTICAST_LOOP:
      return IP_MULTICAST_LOOP;
    case DS::Sock::OptName::DSSOCK_TCP_NODELAY:
    case DS::Sock::OptName::DSSOCK_TCP_DELAYED_ACK:
      return TCP_NODELAY;
    case DS::Sock::OptName::DSSOCK_TCP_MAXSEG:
      return TCP_MAXSEG;
    case DS::Sock::OptName::DSSOCK_IP_TOS:
      return IP_TOS;
    case DS::Sock::OptName::DSSOCK_IP_TTL:
      return IP_TTL;

    case DS::Sock::OptName::DSSOCK_TCP_TIMESTAMP:
    case DS::Sock::OptName::DSSOCK_TCP_SACK:
    case DS::Sock::OptName::DSSOCK_IP_RECVIF:
    case DS::Sock::OptName::DSSOCK_IP_RECVERR:
    case DS::Sock::OptName::DSSOCK_SO_ERROR_ENABLE:
    case DS::Sock::OptName::DSSOCK_SO_ERROR:
    case DS::Sock::OptName::DSSOCK_IPV6_RECVERR:
    case DS::Sock::OptName::DSSOCK_IPV6_TCLASS:
    case DS::Sock::OptName::DSSOCK_ICMP_TYPE:
    case DS::Sock::OptName::DSSOCK_ICMP_CODE:
      /* All the above options are currently unsupported */
      return DS::Error::DSS_EOPNOTSUPP;

    default:
      MSG_ERROR("Unhandled type: %d", optName,0,0);
      return DSS_ERROR;
  }
  return DSS_ERROR;
} /* LinuxMapSOName() */

#if 0
static int LinuxMapAddrFamilyType(DS::AddrFamilyType family)
{
  switch(family)
  {
    case DS::DSS_AF_UNSPEC:
    case DS::DSS_AF_ANY:
      return AF_UNSPEC;
    case DS::DSS_AF_INET:
      return AF_INET;
    case DS::DSS_AF_INET6:
      return AF_INET6;
    default:
      MSG_ERROR("Unhandled type: %d", family,0,0);
      return DSS_ERROR;
  }

  return DSS_ERROR;
} /* LinuxMapAddrFamilyType() */
#endif

static int LinuxMapFamilyTypeToPlatform(DS::Sock::FamilyType family)
{
  switch(family)
  {    
    case DS::Sock::Family::INET:
      return AF_INET;
    case DS::Sock::Family::INET6:
      return AF_INET6;
    default:
      MSG_ERROR("Unhandled type: %d", family,0,0);
      return DSS_ERROR;
  }

  return DSS_ERROR;
} /* LinuxMapFamilyTypeToPlatform() */

static DS::Sock::FamilyType LinuxMapFamilyTypeFromPlatform(int family)
{
  switch(family)
  {    
    case AF_INET:
      return DS::Sock::Family::INET;
    case AF_INET6:
      return DS::Sock::Family::INET6;
    default:
      MSG_ERROR("Unhandled type: %d", family,0,0);
      return DSS_ERROR;
  }

  return DSS_ERROR;
} /* LinuxMapFamilyTypeFromPlatform() */

static int LinuxMapSocketType(DS::Sock::SocketType sockType)
{
  switch(sockType)
  {
//MCT is returning error ok for unspec?
    case DS::Sock::Type::UNSPEC:
      return DSS_ERROR;
    case DS::Sock::Type::STREAM:
      return SOCK_STREAM;
    case DS::Sock::Type::DGRAM:
      return SOCK_DGRAM;
    default:
      MSG_ERROR("Unhandled type: %d", sockType,0,0);
      return DSS_ERROR;
  }

  return DSS_ERROR;
} /* LinuxMapSocketType() */

static int LinuxMapProtocolType(DS::Sock::ProtocolType protocol)
{
  switch(protocol)
  {
    case DS::Sock::Protocol::UNSPEC:
      return IPPROTO_IP; //MCT ???
    case DS::Sock::Protocol::TCP:
      return IPPROTO_TCP;
    case DS::Sock::Protocol::UDP:
      return IPPROTO_UDP;
    case DS::Sock::Protocol::ICMP:
      return IPPROTO_RAW;
    default:
      MSG_ERROR("Unhandled type: %d", protocol,0,0);
      return DSS_ERROR;
  }
  return DSS_ERROR;
} /* LinuxMapProtocolType() */


static DS::ErrorType LinuxMapDSSockAddr2SockAddr
(
  const DS::Sock::SockAddrStorageType *pSockAddrIn,
  struct sockaddr_storage             *pSockAddrOut,
  int                                 *nLen
)
{
  void* pSaIn;
  void* pSaOut;
  int   nAddr;

  if( DS::Sock::Family::INET == pSockAddrIn->family ) 
  {
    pSaIn = (void*)pSockAddrIn;
    pSaOut = (void*)pSockAddrOut;
    nAddr = MIN(sizeof(DS::Sock::SockAddrINType), sizeof(struct sockaddr_in));
    *nLen = sizeof(struct sockaddr_in);
    MSG_MED( "Port: %d", 
             ntohs(((DS::Sock::SockAddrINType*)pSaIn)->port),0,0 );
    LOG_MSG_IPV4_ADDR( LOG_MSG_INFO1_LEVEL,
                       ((DS::Sock::SockAddrINType*)pSaIn)->addr );
  } 
  else if( DS::Sock::Family::INET6 == pSockAddrIn->family ) 
  {
    pSaIn = (void*)pSockAddrIn;
    pSaOut = (void*)pSockAddrOut;
    nAddr = MIN(sizeof(DS::Sock::SockAddrIN6Type), sizeof(struct sockaddr_in6));
    *nLen = sizeof(struct sockaddr_in6);
    MSG_MED( "Port: %d", 
             ntohs(((DS::Sock::SockAddrIN6Type*)pSaIn)->port),0,0 );
    LOG_MSG_IPV6_ADDR( LOG_MSG_INFO1_LEVEL,
                       ((uint64*)((DS::Sock::SockAddrIN6Type*)pSaIn)->addr) );
  } 
  else 
  {
    return DSS_ERROR;
  }
  
  memcpy( pSaOut, pSaIn, nAddr );
  
  if( DSS_ERROR == LinuxMapFamilyTypeToPlatform(pSockAddrIn->family) )
  {
    return DSS_ERROR;
  }
  pSockAddrOut->ss_family = LinuxMapFamilyTypeToPlatform(pSockAddrIn->family);
  
  return DSS_SUCCESS;
} /* LinuxMapDSSockAddr2SockAddr() */

static DS::ErrorType LinuxMapSockAddr2DSSockAddr
(
  struct sockaddr_storage       *pSockAddrIn, 
  DS::Sock::SockAddrStorageType *pSockAddrOut
)
{
  void*              pSaIn;
  void*              pSaOut;
  int                nAddr;
  DS::AddrFamilyType family;

  if( (DS::Sock::FamilyType)DSS_ERROR ==
      LinuxMapFamilyTypeFromPlatform( pSockAddrIn->ss_family ) )
  {
    return DSS_ERROR;
  }
  family = (DS::AddrFamilyType)LinuxMapFamilyTypeFromPlatform(pSockAddrIn->ss_family);
  
  if( AF_INET == pSockAddrIn->ss_family ) 
  {
    pSaIn = (void*)pSockAddrIn;
    pSaOut = (void*)pSockAddrOut;
    nAddr = MIN(sizeof(DS::Sock::SockAddrINType), sizeof(struct sockaddr_in));
  } 
  else if( AF_INET6 == pSockAddrIn->ss_family ) 
  {
    pSaIn = (void*)pSockAddrIn;
    pSaOut = (void*)pSockAddrOut;
    nAddr = MIN(sizeof(DS::Sock::SockAddrIN6Type), sizeof(struct sockaddr_in6));
  } 
  else 
  {
    return DSS_ERROR;
  }
  
  memcpy( pSaOut, pSaIn, nAddr );
  pSockAddrOut->family = family;

  MSG_MED( "Port: %d", 
           ntohs(((DS::Sock::SockAddrINType*)pSaOut)->port),0,0 );
  if( AF_INET6 == pSockAddrIn->ss_family ) 
  {
    LOG_MSG_IPV6_ADDR( LOG_MSG_INFO1_LEVEL,
                       ((uint64*)((DS::Sock::SockAddrIN6Type*)pSockAddrOut)->addr) );
  } else {
    LOG_MSG_IPV4_ADDR( LOG_MSG_INFO1_LEVEL,
                       ((DS::Sock::SockAddrINType*)pSaOut)->addr );
  }
  
  return DSS_SUCCESS;
}

/*-----------------------------------------------------------------------------
  BEGIN LinuxSocket Interface implementation.
-----------------------------------------------------------------------------*/

/* Constructor for LinuxSocket */
PS::Sock::Platform::LinuxSocket::LinuxSocket
(
  SOCKET s, 
  DS::AddrFamilyType ft,
  DS::Sock::SocketType st,
  DS::Sock::ProtocolType pt
)
{
  sSocket = s;
  family = ft;
  sockType = st;
  protocol = pt;

  pendingEvents = SOCK_EVENT_NONE;
  sockstate = SOCK_STATE_NONE;
  bMcastBcast = FALSE;
  bRecvErr = FALSE;
  critSectPtr = NULL;
  memset(&next_hop_addr, 0x0, sizeof(next_hop_addr));
  memset(&pkt_info, 0x0, sizeof(pkt_info));
  fi_mask = 0;
  subset_id = 0;
  routing_cache = NULL;
  eventListenerPtr = NULL;
  ip_addr_scope = IP_ADDR_INVALID_SCOPE;
  
  /* DSS expects non-blocking socket operations */
  fcntl( sSocket, F_SETFL, fcntl( sSocket, F_GETFL, 0) | O_NONBLOCK );

  if(0 != LinuxSocketEventManager( LINUX_SOCKET_REG, this ))
  {
    MSG_ERROR( "LinuxSocket: can't create event. Socket creation failed!",0,0,0 );
    ASSERT(0);
  }
} /* LinuxSocket() */

/* Deconstructor for LinuxSocket */
PS::Sock::Platform::LinuxSocket::~LinuxSocket()
{
  LinuxSocketFactory *  platformSockFactoryPtr;

  if(0 != LinuxSocketEventManager(LINUX_SOCKET_DEREG, this))
  {
    MSG_ERROR( "LinuxSocket: can't delete event!",0,0,0 );
    ASSERT(0);
  }
  
  /*-----------------------------------------------------------------------
    Delete self from SocketFactory
  -----------------------------------------------------------------------*/
  platformSockFactoryPtr = LinuxSocketFactory::CreateInstance();
  ASSERT( 0 != platformSockFactoryPtr);

  platformSockFactoryPtr->DeleteSocket( this );
  platformSockFactoryPtr->DeleteInstance();

  return;
} /* ~LinuxSocket*/

void LinuxSocket::RegEventListener
(
  PS::Sock::Platform::IEventListener *  newEventListenerPtr
)
{
  if(newEventListenerPtr == NULL)
  {
    MSG_ERROR("LinuxSocket::RegEventListener, eventListenerPtr is NULL",0,0,0);
    ASSERT(0);
    return;
  }

  /* Register the event listener for the socket (internal DSsock socket) */
  eventListenerPtr = newEventListenerPtr;
  
  eventListenerPtr->ProcessEvent(PS::Sock::Platform::Event::PLATFORM_ENABLED);
} /* RegEventListener() */

void LinuxSocket::SetCritSection
(
  ICritSect *  newCritSectPtr
)
{
  if(newCritSectPtr == NULL)
  {
    MSG_ERROR("LinuxSocket::SetCritSection, critSectPtr is NULL",0,0,0);
    ASSERT(0);
    return;
  }
  
  /* Register the critical section for the socket */
  critSectPtr = newCritSectPtr;
}

DS::ErrorType LinuxSocket::Bind
(
  const DS::Sock::SockAddrStorageType  *  localAddrPtr
)
{
  struct sockaddr_storage SockAddr;
  int addrLen = 0;
  int err = 0;

  MSG_LOW("LinuxSocket::Bind()",0,0,0);

  if(localAddrPtr == NULL)
  {
    MSG_ERROR("LinuxSocket::Bind, localAddrPtr is NULL",0,0,0);
    ASSERT(0);
    return DSS_ERROR;
  }

  if(DSS_ERROR == LinuxMapDSSockAddr2SockAddr(localAddrPtr, 
                                              &SockAddr, 
                                              &addrLen))
  {
    return DSS_ERROR;
  }

  if( LINUX_ERROR == bind( sSocket, (struct sockaddr*)&SockAddr, addrLen ) )
  {
    err = LinuxGetError( errno );
    MSG_ERROR("LinuxSocket::Bind reported error: (%d) %s",errno, strerror(errno),0);
    return err;
  }

  SetState( SOCK_STATE_BOUND );
  return DSS_SUCCESS;
} /* Bind() */

DS::ErrorType LinuxSocket::Listen
(
  int32            backlog
)
{
  int err = 0;

  MSG_LOW("LinuxSocket::Listen()",0,0,0);

  /* Check that socket is bound to a source address.  DSS expects
   * EINVAL otherwise.*/
  if( !IsStateSet(SOCK_STATE_BOUND) )
  {
    MSG_ERROR("LinuxSocket::Listen, socket not bound",0,0,0);
    return DS::Error::DSS_EINVAL;
  }

  if( LINUX_ERROR == listen( sSocket, backlog ) )
  {
    err = LinuxGetError( errno );
    MSG_ERROR("LinuxSocket::Listen, failure with err code: 0x%x",err,0,0);
    return err;
  }

  /* Register socket for monitoring of READ event.  When event
   * occurs, socket's EventListener will be notified. */
  SetPendingEvent( SOCK_EVENT_LISTEN );
  if( LINUX_ERROR ==
      LinuxSocketEventManager_Monitor( this, Event::READ )) {
    MSG_ERROR("Cannot register for event %d", SOCK_EVENT_LISTEN, 0,0 );
    ClearPendingEvent( SOCK_EVENT_LISTEN );
    return DSS_ERROR;
  }

  SetState( SOCK_STATE_LISTENING );
  
  return DSS_SUCCESS;
} /* Listen() */


DS::ErrorType LinuxSocket::Accept
(
  ISocket                      ** newPlatformSockPtrPtr,
  DS::Sock::SockAddrStorageType * remoteAddrPtr
)
{
  LinuxSocket      * newLinuxSock;
  SOCKET             newSock;
  struct sockaddr_storage SockAddr;
  int                addrlen;
  int                err = 0;

  MSG_LOW("LinuxSocket::Accept()",0,0,0);

  if(newPlatformSockPtrPtr == NULL)
  {
    MSG_ERROR("LinuxSocket::Accept, newPlatformSockPtrPtr is NULL",0,0,0);
    ASSERT(0);
    return DSS_ERROR;
  }

  if(remoteAddrPtr == NULL)
  {
    MSG_ERROR("LinuxSocket::Accept, remoteAddrPtr is NULL",0,0,0);
    ASSERT(0);
    return DSS_ERROR;
  }

  if (DS::Sock::Family::INET == family) 
  {
    addrlen = sizeof(struct sockaddr_in);
  } 
  else if (DS::Sock::Family::INET6 == family) 
  {
    addrlen = sizeof(struct sockaddr_in6);
  } 
  else 
  {
    MSG_ERROR( "LinuxSocket::Accept invalid family type",0,0,0 );
    return DSS_ERROR;
  }

  /* Accept will be non-blocking */
  newSock = accept( sSocket, (struct sockaddr*)&SockAddr, &addrlen );

  if( LINUX_ERROR == newSock )
  {
    err = LinuxGetError( errno );
    MSG_ERROR("LinuxSocket::Accept, failure with err code: 0x%x",err,0,0);
    return err;
  }

  /* Save LinuxSocket and create the new Platform Socket */
  newLinuxSock = new LinuxSocket(newSock, family, sockType, protocol);
  if( newLinuxSock == NULL)
  {
    return DS::Error::DSS_EMFILE;
  }

  if(DSS_ERROR == LinuxMapSockAddr2DSSockAddr( &SockAddr, remoteAddrPtr ))
  {
    MSG_ERROR( "LinuxSocket::Accept couldn't map sockaddrs!",0,0,0 );   
    delete newLinuxSock;
    return DSS_ERROR;
  }

  *newPlatformSockPtrPtr = newLinuxSock;

  return DSS_SUCCESS;
} /* Accept() */


DS::ErrorType LinuxSocket::Connect
(
  const DS::Sock::SockAddrStorageType *  remoteAddrPtr
)
{
  struct sockaddr_storage SockAddr;
  int addrLen = sizeof(SockAddr);
  int err = DSS_SUCCESS;

  MSG_LOW("LinuxSocket::Connect()",0,0,0);

  if(remoteAddrPtr == NULL)
  {
    MSG_ERROR("LinuxSocket::Connect, remoteAddrPtr is NULL",0,0,0);
    ASSERT(0);
    return DSS_ERROR;
  }

  if(DSS_ERROR == LinuxMapDSSockAddr2SockAddr(remoteAddrPtr, 
                                              &SockAddr, 
                                              &addrLen))
  {
    MSG_ERROR("LinuxSocket::Connect, failed on address mapping",0,0,0);
    return DSS_ERROR;
  }

  /* Check for already connected state */
  if( IsStateSet( SOCK_STATE_CONNECTED ) )
  {
    MSG_ERROR( "LinuxSocket::Connect, socket already connected",0,0,0 );
    return DS::Error::DSS_EISCONN;
  }
  
  /* Non-blocking connect, as socket made non-blocking in constructor */
  if( LINUX_ERROR == connect( sSocket, (struct sockaddr*)&SockAddr, addrLen ) )
  {
    err = LinuxGetError( errno );

    /* Process error for non-blocking socket */
    if(errno != EINPROGRESS)
    {
      //MCT
      /* Map to different DSS err code to adhere to DSS expections */
      if(err == DS::Error::DSS_EFAULT)
      {
        err = DS::Error::DSS_EINVAL;
      }
      MSG_ERROR( "LinuxSocket::Connect, failure with err code: 0x%x",err,0,0 );
      return err;
    }
  }

  SetState( SOCK_STATE_BOUND );
  
  /* Register socket for monitoring of WRITE event.  When event
   * occurs, socket's EventListener will be notified. */
  SetPendingEvent( SOCK_EVENT_CONNECT );
  if( LINUX_ERROR ==
      LinuxSocketEventManager_Monitor( this, Event::WRITE )) {
    MSG_ERROR("Cannot register for event %d", SOCK_EVENT_CONNECT, 0,0 );
    ClearPendingEvent( SOCK_EVENT_CONNECT );
    return DSS_ERROR;
  }
 
  return err;
} /* Connect() */

DS::ErrorType LinuxSocket::GetSockName
(
  DS::Sock::SockAddrStorageType *  localAddrPtr
)
{
  struct sockaddr_storage SockAddr;
  int addrLen;
  int err = 0;

  MSG_LOW("LinuxSocket::GetSockName()",0,0,0);

  if(localAddrPtr == NULL)
  {
    MSG_ERROR("LinuxSocket::GetSockName, localAddrPtr is NULL",0,0,0);
    ASSERT(0);
    return DSS_ERROR;
  }

 if (DS::Sock::Family::INET == family) 
  {
    addrLen = sizeof(struct sockaddr_in);
    memset(localAddrPtr, 0x0, sizeof(DS::Sock::SockAddrINType));
  } 
  else if (DS::Sock::Family::INET6 == family) 
  {
    addrLen = sizeof(struct sockaddr_in6);
    memset(localAddrPtr, 0x0, sizeof(DS::Sock::SockAddrIN6Type));
  } 
  else 
  {
    MSG_ERROR( "LinuxSocket::GetSockName() invalid family type %d", family,0,0 );
    return DSS_ERROR;
  }

  if( LINUX_ERROR == getsockname( sSocket, (struct sockaddr*)&SockAddr, &addrLen ) )
  {
    err = LinuxGetError( errno );

#if 0 // TODO: determine if needed for Linux
    /* If socket is not bound WSAEINVAL is returned. This is a valid
     * in DSS.  Set the addr family manually. If it's EFAULT, ???
     * Also when socket is invalid and getsockname is closed for
     * success for force DSS to close their socket */
    if(dsErrno != DS::Error::DSS_EFAULT &&
       dsErrno != DS::Error::DSS_EINVAL &&
       !(dsErrno == DS::Error::DSS_EBADF && INVALID_SOCKET == sSocket))
    {
      MSG_ERROR("LinuxSocket::GetSockName, failure with err code: 0x%x",err,0,0);
      return err;
    }
    else
    {
      localAddrPtr->family = family;
      return DSS_SUCCESS;
    }
#else
    MSG_ERROR("LinuxSocket::GetSockName, failure with err code: 0x%x",err,0,0);
    return err;
#endif
  }

  /* In the successful retrieval of the Linux socket name map it to the DSS structure. We
   * need to automatically map the multicast address back since Linux returns ANY */
  if(bMcastBcast) 
  {
    //copy addr for the bcast mcast stuff
    //MCT memcpy(psaLocal, &saMCast, sizeof(saMCast));
  }
  else 
  {
    if(DSS_ERROR == LinuxMapSockAddr2DSSockAddr( &SockAddr, localAddrPtr ))
    {
      MSG_ERROR( "LinuxSocket::GetSockName() couldn't map sockaddrs!",0,0,0 );   
      return DSS_ERROR;
    }
  }
  return DSS_SUCCESS;
} /* GetSockName() */


DS::ErrorType LinuxSocket::GetPeerName
(
  DS::Sock::SockAddrStorageType *  remoteAddrPtr
)
{
  struct sockaddr_storage SockAddr;
  int addrLen;
  int err = 0;

  if(remoteAddrPtr == NULL)
  {
    MSG_ERROR("LinuxSocket::GetPeerName, remoteAddrPtr is NULL",0,0,0);
    ASSERT(0);
    return DSS_ERROR;
  }

  if (DS::Sock::Family::INET == family) 
  {
    addrLen = sizeof(struct sockaddr_in);
    memset(remoteAddrPtr, 0x0, sizeof(DS::Sock::SockAddrINType));
  } 
  else if (DS::Sock::Family::INET6 == family) 
  {
    addrLen = sizeof(struct sockaddr_in6);
    memset(remoteAddrPtr, 0x0, sizeof(DS::Sock::SockAddrIN6Type));
  } 
  else 
  {
    MSG_ERROR( "LinuxSocket::GetSockName() invalid family type %d", family,0,0 );
    return DSS_ERROR;
  }

  if( LINUX_ERROR == getpeername( sSocket, (struct sockaddr*)&SockAddr, &addrLen) )
  {
    MSG_ERROR("LinuxSocket::GetPeerName reported error: %s",strerror(errno),0,0);
    err = LinuxGetError( errno );
    return err;
  }

  if(DSS_ERROR == LinuxMapSockAddr2DSSockAddr( &SockAddr, remoteAddrPtr ))
  {
    MSG_ERROR( "LinuxSocket::GetPeerName couldn't map sockaddrs!",0,0,0 );   
    return DSS_ERROR;
  }
  
  return DSS_SUCCESS;
} /* GetPeerName() */


DS::ErrorType LinuxSocket::SetOpt
(
  DS::Sock::OptLevelType   optLevel,
  DS::Sock::OptNameType    optName,
  int32                    optVal,
  int32                    optLen
)
{
  int level, name;

  if(optVal == 0)
  {
    MSG_ERROR("LinuxSocket::SetOpt, optVal is NULL",0,0,0);
    ASSERT(0);
    return DSS_ERROR;
  }

  /* Certain parameters can't be set as Linux doesn't support them. In these 
     cases they must be "faked" and set within the platform socket */
  if (DS::Sock::OptName::DSSOCK_IP_RECVERR == optName) 
  {
    bRecvErr = TRUE;
    return DSS_SUCCESS;
  }

  /* Map DS values to Linux values */
  level = LinuxMapSOLevel(optLevel);
  name = LinuxMapSOName(optName);

  /* DSS_SUCCESS indicates we should fake the success of the option */
  if( DSS_SUCCESS == name || DS::Error::DSS_EOPNOTSUPP == name)
  {
    return name;
  }
  /* DSS_ERROR indicates an unknown option */
  if( DSS_ERROR == name || DSS_ERROR == level )
  {
    return DSS_ERROR;
  }

  if( LINUX_ERROR ==
      setsockopt( sSocket, level, name, (char*) optVal, optLen ) )
  {
    MSG_ERROR("LinuxSocket::SetOpt reported error: %s",strerror(errno),0,0);
    return LinuxGetError( errno );
  }

  return DSS_SUCCESS;
} /* SetOpt() */


DS::ErrorType LinuxSocket::GetOpt
(
  DS::Sock::OptLevelType   optLevel,
  DS::Sock::OptNameType    optName,
  int32                   *optValPtr,
  int32                   *optLenPtr
)
{
  int level, name;
  u_long ioSockOptVal;

  if(optValPtr == NULL)
  {
    MSG_ERROR("LinuxSocket::GetOpt, optValPtr is NULL",0,0,0);
    ASSERT(0);
    return DSS_ERROR;
  }

  if(optLenPtr == NULL)
  {
    MSG_ERROR("LinuxSocket::GetOpt, optLenPtr is NULL",0,0,0);
    ASSERT(0);
    return DSS_ERROR;
  }

  /* Certain parameters can't be retrieved as Linux doesn't support them. In
     these cases they must be "faked" */
  if (DS::Sock::OptName::DSSOCK_IP_RECVIF == optName) 
  {
    *optValPtr = 0;
    *optLenPtr = sizeof(unsigned long);
    return DSS_SUCCESS;
  }
  
  if (DS::Sock::OptName::DSSOCK_IP_RECVERR == optName) 
  {
    *optValPtr = (int32)bRecvErr;
    *optLenPtr = sizeof(bRecvErr);
    return DSS_SUCCESS;
  }

  /* Map DS values to WM values */
  level = LinuxMapSOLevel(optLevel);
  name = LinuxMapSOName(optName);

  if( DS::Error::DSS_EOPNOTSUPP == name)
  {
    MSG_ERROR("LinuxSocket::GetOpt, no supprot for option 0x%x",optName,0,0);
    return name;
  }
  /* DSS_ERROR indicates an unknown option */
  if( DSS_ERROR == name || DSS_ERROR == level )
  {
    MSG_ERROR("LinuxSocket::GetOpt, error on mapping option 0x%x level %d",
              optName,optLevel,0);
    return DSS_ERROR;
  }

  /* All ioctl socket commands are processed here */
  if(name == FIONREAD)
  {
    ioSockOptVal = (u_long) *optValPtr;
    if( LINUX_ERROR == ioctl( sSocket, name, &ioSockOptVal ) )
    {
      MSG_ERROR("LinuxSocket::GetOpt reported error on FIONREAD: %s",strerror(errno),0,0);
      return LinuxGetError( errno );
    }
    (*optValPtr) = (int32) ioSockOptVal;
    (*optLenPtr) = sizeof(unsigned long);
    return DSS_SUCCESS;
  }

  if( LINUX_ERROR ==
      getsockopt( sSocket, level, name, (char*) optValPtr, (int*) optLenPtr ) )
  {
    MSG_ERROR("LinuxSocket::SetOpt reported error: %s",strerror(errno),0,0);
    return LinuxGetError( errno );
  }

  return DSS_SUCCESS;
} /* GetOpt() */


DS::ErrorType LinuxSocket::SetSOLingerReset
(
  const DS::Sock::LingerType *  soLingerPtr
)
{
  struct linger optVal;

  if(soLingerPtr == NULL)
  {
    MSG_ERROR("LinuxSocket::SetSOLingerReset, soLingerPtr is NULL",0,0,0);
    ASSERT(0);
    return DSS_ERROR;
  }

  /* If greater than MAX WM linger timer return an error */
  if(soLingerPtr->timeInSec > 0xFFFF)
  {
    MSG_ERROR("LinuxSocket::SetSOLingerReset, linger timer is > 0xFFFF",0,0,0);
    return DS::Error::DSS_EFAULT;
  }

  optVal.l_onoff  = soLingerPtr->isLingerOn;
  optVal.l_linger = (unsigned short) soLingerPtr->timeInSec;

  if( LINUX_ERROR ==
      setsockopt( sSocket, SOL_SOCKET, SO_LINGER, (char*) &optVal, sizeof(optVal) ) )
  {
    MSG_ERROR("LinuxSocket::SetSOLingerReset reported error: %s",strerror(errno),0,0);
    return LinuxGetError( errno );
  }

  return DSS_SUCCESS;
} /* SetSOLingerReset() */


DS::ErrorType LinuxSocket::GetSOLingerReset
(
  DS::Sock::LingerType *     soLingerPtr
)
{
  struct linger optVal;
  int optLen = sizeof(optVal);;
  
  if(soLingerPtr == NULL)
  {
    MSG_ERROR("LinuxSocket::GetSOLingerReset, soLingerPtr is NULL",0,0,0);
    ASSERT(0);
    return DSS_ERROR;
  }

  if( LINUX_ERROR ==
      getsockopt( sSocket, SOL_SOCKET, SO_LINGER, (char*) &optVal, &optLen ) )
  {
    memset(soLingerPtr, 0, sizeof(DS::Sock::LingerType));
    MSG_ERROR("LinuxSocket::GetSOLingerReset reported error: %s",strerror(errno),0,0);
    return LinuxGetError( errno );
  }

  soLingerPtr->isLingerOn = (boolean) optVal.l_onoff;
  soLingerPtr->timeInSec  = optVal.l_linger;

  return DSS_SUCCESS;
} /* GetSOLingerReset() */


DS::ErrorType LinuxSocket::AsyncSelect
(
  EventType            event
)
{
  SockEventType  sockevent = SOCK_EVENT_NONE;

  /* Register socket for monitoring of specified event.  When event
   * occurs, socket's EventListener will be notified. Note connect and
   * listen operations are handled in the respective methods. */
  switch( event )
  {
    case Event::READ:
      sockevent = SOCK_EVENT_READ;
      break;

    case Event::WRITE:
      sockevent = SOCK_EVENT_WRITE;
      break;

    case Event::ACCEPT:
      event = Event::READ;
      sockevent = SOCK_EVENT_ACCEPT;
      break;
      
    case Event::CLOSE:
      if( IsStateSet(SOCK_STATE_CLOSED) )
      {
        /* Post event to upper layers */
        eventListenerPtr->ProcessEvent( PS::Sock::Platform::Event::CLOSE );
      } else {
        event = Event::READ;
        sockevent = SOCK_EVENT_CLOSE;
      }
      break;

    case Event::PLATFORM_ENABLED:
    case Event::PLATFORM_DISABLED:
      MSG_LOW("AsyncSelect ignoring unsupproted event %d", (int)event,0,0 );
      break;

    default:
      MSG_ERROR("AsyncSelect requested for unsupproted event %d", (int)event,0,0 );
      return DSS_ERROR;
  }

  if( SOCK_EVENT_NONE != sockevent )
  {
    SetPendingEvent( sockevent );

    if( LINUX_ERROR ==
        LinuxSocketEventManager_Monitor( this, event )) {
      MSG_ERROR("Cannot register for event %d", event, 0,0 );
      ClearPendingEvent( sockevent );
      return DSS_ERROR;
    }
  }

  return DSS_SUCCESS;
} /* AsyncSelect() */


DS::ErrorType LinuxSocket::SetRtMetaInfo
(
  ps_rt_meta_info_type *  routeMetaInfoPtr
)
{
  unsigned int i;

  if (routeMetaInfoPtr == NULL) 
  {
      MSG_ERROR("LinuxSocket::SetRtMetaInfo, routeMetaInfoPtr is NULL",0,0,0);
      return DSS_ERROR;
  }
  
#ifdef FEATURE_SEC_IPSEC
  memcpy(&ipsec_info, &(routeMetaInfoPtr->ipsec_info), sizeof(ipsec_info));
#endif  
  pkt_info      = routeMetaInfoPtr->pkt_info;
  fi_mask       = routeMetaInfoPtr->fi_mask;
  subset_id     = routeMetaInfoPtr->subset_id;
  next_hop_addr = routeMetaInfoPtr->next_hop_addr;
  ip_addr_scope = routeMetaInfoPtr->ip_addr_scope;
  routing_cache = routeMetaInfoPtr->routing_cache;

  for( i=0; i<IP_FLTR_CLIENT_OUTPUT_MAX; i++ ) 
  {
    fi_result[i] = routeMetaInfoPtr->fi_result[i];
  }

  /* Bind the socket to the proper interface, only for unicast interfaces */
  if( routing_cache && !bMcastBcast )
  {
    DS::Sock::SockAddrStorageType localAddr;
    DS::Sock::SockAddrINType      *pSaIn = NULL;
    DS::Sock::SockAddrIN6Type     *pSaIn6 = NULL;
    ps_ip_addr_type               ip_addr;

    /* Retrieve any current bindings to ports/ip, etc */
    if(DSS_SUCCESS != LinuxSocket::GetSockName(&localAddr))
      goto bail;

    if (DS::Sock::Family::INET == family) 
    {
      pSaIn = (DS::Sock::SockAddrINType*) &localAddr;
      ip_addr.type = IPV4_ADDR;
      ps_iface_get_addr( (ps_iface_type*)routing_cache, &ip_addr );
      /* Only if address is empty do we populate with the interface ip addr */
      if(pSaIn->addr == INADDR_ANY)
      {
        pSaIn->addr = ip_addr.addr.v4.ps_s_addr;
      }
    } 
    else if (DS::Sock::Family::INET6 == family) 
    {
      //TODO check this logic when v6 supported. IID may be different, eg generated by Linux.
      pSaIn6 = (DS::Sock::SockAddrIN6Type*) &localAddr;
      ip_addr.type = IPV6_ADDR;
      ps_iface_get_addr( (ps_iface_type *)routing_cache, &ip_addr );
      /* Only if address is empty do we populate with the interface ip addr */
      if(pSaIn6->addr == INADDR_ANY)
      {
        memcpy(&pSaIn->addr, &ip_addr.addr.v6.ps_s6_addr, sizeof(pSaIn->addr));
      }
    } 
    else 
    {
      MSG_ERROR( "Invalid family type %d", family,0,0 );
      goto bail;
    }

    /* Bind to the proper interface so packets are routed over the proper iface */
    if(DSS_SUCCESS != LinuxSocket::Bind(&localAddr)) {
      /* Error here may be ignored as socket may be already bound */
      MSG_HIGH( "Failed on bind for packet routing, ignoring",0,0,0 );
    }
  }
  return DSS_SUCCESS;

bail:
  return DSS_ERROR;
} /* SetRtMetaInfo() */

DS::ErrorType LinuxSocket::SetRouteScope
(
  ps_route_scope_type *  routeScopePtr
)
{
  if (routeScopePtr == 0) 
  {
      MSG_ERROR("LinuxSocket::SetRouteScope, routeScopePtr is NULL",0,0,0);
      return DS::Error::DSS_EFAULT;
  }
  return DSS_SUCCESS;
}

DS::ErrorType LinuxSocket::GetRouteScope
(
  ps_route_scope_type *  routeScopePtr
)
{
  if (routeScopePtr == 0) 
  {
      MSG_ERROR("LinuxSocket::GetRouteScope, routeScopePtr is NULL",0,0,0);
      return DS::Error::DSS_EFAULT;
  }
  //MCT *routeScopePtr = routeScope;
  return DSS_SUCCESS;
}

void LinuxSocket::SetSystemOption
(
  void
)
{
//MCT  isSystemSocket = true;
  return;
}
//end

DS::ErrorType LinuxSocket::SendMsg
(
  const DS::Sock::SockAddrStorageType *  remoteAddrPtr,
  const DS::Sock::SeqBytesType ioVecArr[],
  int32                        numIOVec,
  int32 *                      numWrittenPtr,
  DS::Sock::IAncData **        inAncillaryDataPtrPtr,
  int32                        inAncillaryDataLen,
  uint32                       inFlags,
  int32                        flags,
  ps_pkt_meta_info_type *      pktMetaInfoPtr
)
{
  struct sockaddr_storage    SockAddr;
  struct sockaddr_storage   *pSockAddr = NULL;
  int                iIndex;
  int                addrLen;
  int                bytesSent = 0;
  DS::ErrorType      dsErrno = DSS_SUCCESS;
  struct msghdr      sendhdr;
  struct iovec      *iov_array;
  unsigned int       bufsize;

  (void)inAncillaryDataPtrPtr;
  (void)inAncillaryDataLen;
  (void)pktMetaInfoPtr;

  if(numWrittenPtr == NULL)
  {
    MSG_ERROR("LinuxSocket::SendMsg, numWrittenPtr is NULL",0,0,0);
    ASSERT(0);
    return DSS_ERROR;
  }

#if 0 /* TODO: AncillaryData not supported at this time */
  if(inAncillaryDataPtrPtr == NULL || *inAncillaryDataPtrPtr == NULL)
  {
    MSG_ERROR("LinuxSocket::SendMsg, inAncillaryDataPtrPtr is NULL",0,0,0);
    ASSERT(0);
    return DSS_ERROR;
  }

  if(pktMetaInfoPtr == NULL)
  {
    MSG_ERROR("LinuxSocket::SendMsg, pktMetaInfoPtr is NULL",0,0,0);
    ASSERT(0);
    return DSS_ERROR;
  }
#endif
  
  if(remoteAddrPtr != NULL)
  {
    if(DSS_ERROR == LinuxMapDSSockAddr2SockAddr(remoteAddrPtr, 
                                                &SockAddr, 
                                                &addrLen))
    {
      return DSS_ERROR;
    }
    pSockAddr = &SockAddr;
  }

  /* Allocate IOV array from heap memory */
  bufsize =  numIOVec * sizeof(struct iovec);
  iov_array =  (struct iovec*)ps_system_heap_mem_alloc( bufsize );
  if( NULL == iov_array )
  {
    MSG_ERROR("LinuxSocket::SendMsg, IOV buffer allocation failed (%d)",bufsize,0,0);
    ASSERT(0);
    return DSS_ERROR;
  }

  /* Populate IOV array */
  for( iIndex = 0; iIndex < numIOVec; iIndex++ )
  {
    iov_array[iIndex].iov_base = (void*)ioVecArr[iIndex].data;
    iov_array[iIndex].iov_len = ioVecArr[iIndex].dataLen;
  }
  
  sendhdr.msg_name = (void*)pSockAddr;
  sendhdr.msg_namelen = (pSockAddr)? addrLen : 0;
  sendhdr.msg_iov = iov_array;
  sendhdr.msg_iovlen = numIOVec;
  sendhdr.msg_control = NULL;  /* TODO: AncillaryData not supported at this time */
  sendhdr.msg_controllen = 0;
  sendhdr.msg_flags = inFlags;

  bytesSent = sendmsg( sSocket, &sendhdr, flags );
  if( LINUX_ERROR == bytesSent )
  {
    dsErrno = LinuxGetError( errno );
    /* Map to different DSS err code to adhere to DSS expections */
    if(dsErrno == DS::Error::DSS_EPIPE)
      dsErrno = DS::Error::DSS_ESHUTDOWN;
    goto bail;
  }

  SetState( SOCK_STATE_BOUND );
  
bail:
  ps_system_heap_mem_free( (void**)&iov_array );
  *numWrittenPtr = (int32)bytesSent;
  return dsErrno;
} /* SendMsg() */


DS::ErrorType LinuxSocket::RecvMsg
(
  DS::Sock::SockAddrStorageType *    remoteAddrPtr,
  DS::Sock::SeqBytesType   ioVecArr[],
  int32                    numIOVec,
  int32 *                  numReadPtr,
  DS::Sock::IAncData **    outAncillaryDataPtrPtr,
  int32                    outAncillaryDataLen,
  int32 *                  outAncillaryDataLenReqPtr,
  uint32 *                 outFlagsPtr,
  int32                    flags
)
{
  struct sockaddr_storage  SockAddr;
  int              addrLen;
  int              iIndex;
  int              bytesRecv;
  DS::ErrorType    dsErrno = DSS_SUCCESS;
  struct msghdr    recvhdr;
  struct iovec    *iov_array;
  unsigned int     bufsize;

  (void)outAncillaryDataPtrPtr;
  (void)outAncillaryDataLen;

  if(numReadPtr == NULL)
  {
    MSG_ERROR("LinuxSocket::RecvMsg, numReadPtr is NULL",0,0,0);
    ASSERT(0);
    return DSS_ERROR;
  }

#if 0 /* TODO: Output AncillaryData & Flags not supported at this time */
  if(outAncillaryDataPtrPtr == NULL || *outAncillaryDataPtrPtr == NULL)
  {
    MSG_ERROR("LinuxSocket::RecvMsg, outAncillaryDataPtrPtr is NULL",0,0,0);
    ASSERT(0);
    return DSS_ERROR;
  }

  if(outAncillaryDataLenReqPtr == NULL)
  {
    MSG_ERROR("LinuxSocket::RecvMsg, outAncillaryDataLenReqPtr is NULL",0,0,0);
    ASSERT(0);
    return DSS_ERROR;
  }
  
  if(outFlagsPtr == NULL)
  {
    MSG_ERROR("LinuxSocket::RecvMsg, outFlagsPtr is NULL",0,0,0);
    ASSERT(0);
    return DSS_ERROR;
  }
#endif

  bytesRecv = 0;

  /* Allocate IOV array from heap memory */
  bufsize =  numIOVec * sizeof(struct iovec);
  iov_array =  (struct iovec*)ps_system_heap_mem_alloc( bufsize );
  if( NULL == iov_array )
  {
    MSG_ERROR("LinuxSocket::SendMsg, IOV buffer allocation failed (%d)",bufsize,0,0);
    ASSERT(0);
    return DSS_ERROR;
  }
  /* Populate IOV array */
  for( iIndex = 0; iIndex < numIOVec; iIndex++ )
  {
    iov_array[iIndex].iov_base = (void*)ioVecArr[iIndex].data;
    iov_array[iIndex].iov_len = ioVecArr[iIndex].dataLen;
  }

  if (DS::Sock::Family::INET == family) 
  {
    addrLen = sizeof(struct sockaddr_in);
  } 
  else if (DS::Sock::Family::INET6 == family) 
  {
    addrLen = sizeof(struct sockaddr_in6);
  } 
  else 
  {
    MSG_ERROR( "LinuxSocket::RecvMsg() invalid family type %d", family,0,0 );
    dsErrno = DSS_ERROR;
    goto bail;
  }

  recvhdr.msg_name = (void*)&SockAddr;
  recvhdr.msg_namelen = addrLen;
  recvhdr.msg_iov = iov_array;
  recvhdr.msg_iovlen = numIOVec;
  recvhdr.msg_control = NULL;  /* TODO: Output AncillaryData & Flags not supported at this time */
  recvhdr.msg_controllen = 0;
  recvhdr.msg_flags = NULL;
  
  bytesRecv = recvmsg( sSocket, &recvhdr, flags );
  if( LINUX_ERROR == bytesRecv )
  {
    dsErrno = LinuxGetError( errno );
    //MCT
    /* Map to different DSS err codes to adhere to DSS expections */
    if( IsStateSet(SOCK_STATE_LISTENING) && dsErrno == DS::Error::DSS_ENOTCONN)
      dsErrno = DS::Error::DSS_EINVAL;
    else if( dsErrno == DS::Error::DSS_ESHUTDOWN ||
             dsErrno == DS::Error::DSS_ECONNRESET)
      dsErrno = DS::Error::DSS_EEOF;
    goto bail;
  }

  if(outAncillaryDataLenReqPtr != NULL)
  {
    // TODO when using dgram sockets ancillary data must be populated
    *outAncillaryDataLenReqPtr = 0;
  }
  
  if(outFlagsPtr != NULL)
  {
    *outFlagsPtr = flags;
  }

 /* Map the Linux sockaddr structure to DSS */
  if(remoteAddrPtr != NULL)
  {
    if(DSS_ERROR == LinuxMapSockAddr2DSSockAddr( &SockAddr, remoteAddrPtr ))
    {
      MSG_ERROR( "LinuxSocket::RecvMsg() couldn't map sockaddrs!",0,0,0 );   
      return DSS_ERROR;
    }
  }
  
bail:
  ps_system_heap_mem_free( (void**)&iov_array );
  *numReadPtr = (int32)bytesRecv;
  return dsErrno;
} /* RecvMsg() */


DS::ErrorType LinuxSocket::SendDSMChain
(
  const DS::Sock::SockAddrStorageType * remoteAddrPtr,
  dsm_item_type                      ** dsmItemPtrPtr,
  int32                                 flags,
  int32                               * numWrittenPtr
)
{
  DS::ErrorType    dsErrno     = DSS_SUCCESS;
  uint32           dsmLength   = 0;
  DS::Sock::SeqBytesType ioVecArr[1];
  int32            numIOVec    = 1;
  uint16           bytesCopied = 0;
  
  if(remoteAddrPtr == NULL)
  {
    MSG_ERROR("remoteAddrPtr is NULL",0,0,0);
    return DS::Error::DSS_EFAULT;
  }

  if(dsmItemPtrPtr == NULL || *dsmItemPtrPtr == NULL)
  {
    MSG_ERROR("dsmItemPtrPtr is NULL",0,0,0);
    return DS::Error::DSS_EFAULT;
  }

  if(numWrittenPtr == NULL)
  {
    MSG_ERROR("LinuxSocket::SendDSMChain, numWrittenPtr is NULL",0,0,0);
    return DS::Error::DSS_EFAULT;
  }
  
  /* Determine the length of the DSM packet */
  dsmLength = dsm_length_packet(*dsmItemPtrPtr);

  ioVecArr[0].data       = new byte[dsmLength];
  ioVecArr[0].dataLen    = dsmLength;
  ioVecArr[0].dataLenReq = 0;

  if( ioVecArr[0].dataLen != (bytesCopied = 
      dsm_pullup(dsmItemPtrPtr, (void *)ioVecArr[0].data, ioVecArr[0].dataLen)))
  {
    MSG_ERROR("dsm_pullup only copied %d / %d bytes!", bytesCopied, ioVecArr[0].dataLen,0);
    goto cleanup;
  }

  /* Call internal SendMsg to simplify error handling */
  if( DSS_SUCCESS != (dsErrno = LinuxSocket::SendMsg(remoteAddrPtr, ioVecArr, 
                                                     numIOVec, numWrittenPtr,
                                                     NULL, 0, 0, flags, NULL)))
  {
    goto cleanup;
  }
   
cleanup:
  delete ioVecArr[0].data;
  return dsErrno;
} /* SendDSMChain() */


DS::ErrorType LinuxSocket::RecvDSMChain
(
  DS::Sock::SockAddrStorageType *    remoteAddrPtr,
  dsm_item_type **                   dsmItemPtrPtr,
  int32                              flags,
  int32 *                            numReadPtr
)
{
  DS::ErrorType    dsErrno     = DSS_SUCCESS;
  DS::Sock::SeqBytesType ioVecArr[1];
  int32            numIOVec    = 1;
  uint16           bytesCopied = 0;

  if(remoteAddrPtr == NULL)
  {
    MSG_ERROR("remoteAddrPtr is NULL",0,0,0);
    return DS::Error::DSS_EFAULT;
  }

  /* Other *dsm item should be NULL since we are allocating it here */
  if(dsmItemPtrPtr == NULL || *dsmItemPtrPtr != NULL)
  {
    MSG_ERROR("dsmItemPtrPtr is NULL",0,0,0);
    return DS::Error::DSS_EFAULT;
  }

  if(numReadPtr == NULL)
  {
    MSG_ERROR("LinuxSocket::RecvDSMChain, numReadPtr is NULL",0,0,0);
    return DS::Error::DSS_EFAULT;
  }
  
  /* Setup buffer to copy into. Using 2000 bytes, should provide enough space for most packets */
  ioVecArr[0].data       = new byte[DSS_RECV_BUFFER_SIZE];
  ioVecArr[0].dataLen    = DSS_RECV_BUFFER_SIZE;
  ioVecArr[0].dataLenReq = 0;

  /* Call internal RecvMsg to prevent duplicated code */
  if(DSS_SUCCESS != (dsErrno = LinuxSocket::RecvMsg(remoteAddrPtr, ioVecArr, numIOVec, 
                                                    numReadPtr, NULL, 0, NULL, NULL, flags)))
  {
    goto cleanup;
  }

  /* Convert back to a DSM item */
  if((uint16) *numReadPtr != (bytesCopied = dsm_pushdown(dsmItemPtrPtr, 
                                                         (void*)  ioVecArr[0].data, 
                                                         (uint16) *numReadPtr,
                                                         DSM_DS_SMALL_ITEM_POOL)))
  {
    MSG_ERROR("dsm_pushdown only copied %d / %d bytes!", bytesCopied, *numReadPtr,0);
    *numReadPtr = bytesCopied;
    dsErrno = DS::Error::DSS_EMSGSIZE;
    goto cleanup;
  }

cleanup:
  delete ioVecArr[0].data;
  return dsErrno;
} /* RecvDSMChain() */


DS::ErrorType LinuxSocket::Shutdown
(
  DS::Sock::ShutdownDirType  shutdownDir
)
{
  int direction;

  using namespace DS::Sock::ShutdownDir;

  switch( shutdownDir )
  {
    case RD:
      direction = SHUT_RD;
      break;
    case WR:
      direction = SHUT_WR;
      break;
    case RD_WR:
      direction = SHUT_RDWR;
      break;
    default:
      return LinuxGetError( EINVAL );
  }

  if( LINUX_ERROR == shutdown( sSocket, direction ) )
  {
    return LinuxGetError( errno );
  }

  return DSS_SUCCESS;
} /* Shutdown() */


DS::ErrorType LinuxSocket::Close()
{
  DS::Sock::LingerType  soLinger;

  MSG_LOW("LinuxSocket::Close, Closing sockfd %d!",sSocket,0,0);

  /* Faking the DSS behavior of EWOULDBLOCK for all "connected TCP sockets" without
     linger being set.
  */
  if( IsStateSet(SOCK_STATE_CONNECTED) )
  {
    if( DSS_SUCCESS == LinuxSocket::GetSOLingerReset( &soLinger ) )
    {
       /* If linger is enabled and a non-zero time is set EWOULDBLOCK will be returned
       automatically by Linux, and therefore no need to "fake" the graceful close
       and EWOULDBLOCK */
      if(soLinger.isLingerOn == false || soLinger.timeInSec == 0)
      {
        /* Reset connected since subsequent calls to close should terminate the socket */
        ClearState( SOCK_STATE_CONNECTED );
        SetState( SOCK_STATE_CLOSED );
        /* Post event to upper layers. Note if app has not done
         * RegEvent before this, this CLOSE event will be lost. To
         * satisfy DSS expectations, AsyncSelect() will check for
         * CONNECTED state and post CLOSE event if not connected. */
        eventListenerPtr->ProcessEvent( PS::Sock::Platform::Event::CLOSE );
        return DS::Error::DSS_EWOULDBLOCK;
      }
    }
    else
    {
      MSG_ERROR( "LinuxSocket::Close() couldn't retrieve linger setting",0,0,0 );
    }
  }
  
  if( (INVALID_SOCKET != sSocket) &&
      (LINUX_ERROR == close(sSocket)) )
  {
    MSG_ERROR("LinuxSocket::Close failure!",0,0,0);
    return LinuxGetError( errno );
  }

  sSocket = INVALID_SOCKET;

  delete this;

  return DSS_SUCCESS;
} /* Close() */


DS::ErrorType LinuxSocket::AbortConnection
(
  DS::ErrorType  reasonForAbort
)
{
  if (reasonForAbort) 
  {
      return LinuxSocket::Close();
  }
  return DSS_ERROR;
} /* AbortConnection() */


boolean LinuxSocket::Process
(
  void *  userDataPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void) userDataPtr;
  MSG_ERROR("LinuxSocket::Process, Unexpected call!!",0,0,0);
  ASSERT(0);
  return TRUE;
} /* Socket::Process() */


/*----------------------------------------------------------
   Socket Factory Functions 
----------------------------------------------------------*/
ISocket * LinuxSocketFactory::CreateSocket
(
  DS::Sock::FamilyType    family,
  DS::Sock::SocketType    sockType,
  DS::Sock::ProtocolType  protocol,
  ICritSect *             critSectPtr,
  DS::ErrorType *         dsErrnoPtr
)
{
  LinuxSocket        *newSock;
  SOCKET              sSocket;

  (void) critSectPtr;
  if(dsErrnoPtr == NULL)
  {
    MSG_ERROR("LinuxSocket::CreateSocket, dsErrnoPtr is NULL",0,0,0);
    ASSERT(0);
    return NULL;
  }

  //TODO enter crit section?

  /* Create the new Linux Socket */
  sSocket = socket( LinuxMapFamilyTypeToPlatform(family),
                    LinuxMapSocketType(sockType),
                    LinuxMapProtocolType(protocol) );

  if(sSocket == LINUX_ERROR)
  {
    MSG_ERROR("LinuxSocket::CreateSocket failed!",0,0,0);
    *dsErrnoPtr = LinuxGetError( errno );
    return NULL;
  }

  /* Create the new Platform Socket */
  newSock = new LinuxSocket(sSocket, family, sockType, protocol);

  if( newSock == NULL)
  {
    *dsErrnoPtr = DS::Error::DSS_EMFILE;
    close(sSocket);
    return NULL;
  }

  MSG_LOW("LinuxSocket::CreateSocket, sockfd %d created!",sSocket,0,0);
  return (static_cast <ISocket *> (newSock));
} /* CreateSocket() */


void LinuxSocketFactory::DeleteSocket
(
  LinuxSocket *  sockPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == sockPtr)
  {
    MSG_ERROR("SocketFactory::DeleteSocket sockfd is 0!",0,0,0);
    ASSERT(0);
    return;
  }

  return;
} /* SocketFactory::DeleteSocket() */


LinuxSocketFactory * LinuxSocketFactory::CreateInstance
(
  void
)
{
  /*-------------------------------------------------------------------------
    Allocate a SocketFactory object
  -------------------------------------------------------------------------*/
  if( NULL == sockFactoryPtr )
  {
    MSG_MED("SocketFactory::CreateInstance, Creating new SocketFactory.",0,0,0);

    sockFactoryPtr = new LinuxSocketFactory();
    if(!sockFactoryPtr)
    {
      MSG_ERROR("SocketFactory::CreateInstance, failed to create sockFactoryPtr!",0,0,0);
      ASSERT(0);
    }

    MSG_MED("SocketFactory::CreateInstance, created sockFactoryPtr: %p",sockFactoryPtr,0,0);
  }

  /* Inc reference count to keep track of who's using the SocketFactory */
  refCnt++;

  return sockFactoryPtr;
} /* CreateInstance() */

void LinuxSocketFactory::DeleteInstance
(
  void
)
{ 
  /* Do nothing until the last delete since we use a singleton for the
   * socket factory */
  if(0 == --refCnt)
  {
    MSG_LOW("SocketFactory::DeleteInstance, deleting sockFactoryPtr: %p",sockFactoryPtr,0,0);
    delete sockFactoryPtr;
    sockFactoryPtr = NULL;
  }

  return;
} /* DeleteInstance() */

ISocketFactory * ISocketFactory::CreateInstance
(
  void
)
{ 
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return (static_cast <ISocketFactory *> (LinuxSocketFactory::CreateInstance()));
} /* ISocketFactory::CreateInstance() */
