#ifndef LINUX_PLATFORM_SOCKET_H
#define LINUX_PLATFORM_SOCKET_H
/*===========================================================================
  @file LinuxPlatformSocket.h

  Copyright (c) 2008,2010 Qualcomm Technologies, Inc.
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
  05/04/10   ar  Adapted for Linux platform
  05/02/08   hm  Created module.

===========================================================================*/

#include <sys/socket.h>
#include "comdef.h"
#include "ds_Addr_Def.h"
#include "ds_Sock_Def.h"
#include "ds_Errors_Def.h"
#include "ds_Utils_Factory.h"
#include "PS_Sock_Platform_EventDefs.h"
#include "PS_Sock_Platform_ISocket.h"
#include "PS_Sock_Platform_ISocketFactory.h"
#include "PS_Sock_Platform_IEventListener.h"
#include "ps_pkt_info.h"
#include "ps_iface_ipfltr.h"

#ifdef FEATURE_SEC_IPSEC
#include "secips.h"
#endif 

/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/

/* Error return codes */
#define LINUX_SUCCESS  (0)
#define LINUX_ERROR    (-1)

#define INVALID_SOCKET (-1)

/* Define platform-specific types. */
typedef int                SOCKET;   /* socket file descriptor */
typedef unsigned int       EVENT;
typedef unsigned int       HANDLE; 

namespace PS
{
  namespace Sock
  {
    namespace Platform
    {
      void PowerUpInit(void);
      void Init(void);

      typedef ::int32 SockStateType;
      const SockStateType SOCK_STATE_NONE      = 0x00;
      const SockStateType SOCK_STATE_BOUND     = 0x02;
      const SockStateType SOCK_STATE_CONNECTED = 0x04;
      const SockStateType SOCK_STATE_LISTENING = 0x08;
      const SockStateType SOCK_STATE_CLOSED    = 0x10;

      /* Pending event tracking */
      typedef ::int32 SockEventType;
      const SockEventType SOCK_EVENT_NONE    = 0x00;
      const SockEventType SOCK_EVENT_WRITE   = 0x01;
      const SockEventType SOCK_EVENT_READ    = 0x02;
      const SockEventType SOCK_EVENT_CONNECT = 0x04; /* Maps to WRITE */
      const SockEventType SOCK_EVENT_LISTEN  = 0x08; /* Maps to READ */
      const SockEventType SOCK_EVENT_ACCEPT  = 0x10;
      const SockEventType SOCK_EVENT_CLOSE   = 0x20;
      
      class LinuxSocket: public ISocket
      {
        public:
          LinuxSocket
          (
            SOCKET s, 
            ds::AddrFamilyType ft,
            ds::Sock::SocketType st,
            ds::Sock::ProtocolType pt            
          );
          ~LinuxSocket();

          void RegEventListener
          (
            PS::Sock::Platform::IEventListener *  eventListenerPtr
          );

          void SetCritSection
          (
            ICritSect *  critSectPtr
          );

          ds::ErrorType Bind
          (
            const ds::SockAddrInternalType  *  localAddrPtr
          );

          ds::ErrorType Listen
          (
            int32   backlog
          );

          ds::ErrorType Accept
          (
            Platform::ISocket **             newPlatformSockPtrPtr,
            ds::SockAddrInternalType        *  remoteAddrPtr
          );

          ds::ErrorType Connect
          (
            const ds::SockAddrInternalType *  remoteAddrPtr
          );

          ds::ErrorType GetSockName
          (
            ds::SockAddrInternalType *  localAddrPtr
          ) throw();

          ds::ErrorType GetPeerName
          (
            ds::SockAddrInternalType *  remoteAddrPtr
          );

          ds::ErrorType SetOpt
          (
            ds::Sock::OptLevelType   optLevel,
            ds::Sock::OptNameType    optName,
            int32                    optVal,
            int32                    optLen
          );

          ds::ErrorType GetOpt
          (
            ds::Sock::OptLevelType   optLevel,
            ds::Sock::OptNameType    optName,
            int32 *                  optValPtr,
            int32 *                  optLenPtr
          );

          ds::ErrorType SetSOLingerReset
          (
            const ds::Sock::LingerType *  soLingerPtr
          ) throw();

          ds::ErrorType GetSOLingerReset
          (
            ds::Sock::LingerType *     soLingerPtr
          );

          ds::ErrorType AsyncSelect
          (
            Platform::EventType  event
          );

          ds::ErrorType SetRtMetaInfo
          (
            ps_rt_meta_info_type *  routeMetaInfoPtr
          );

          ds::ErrorType SetRouteScope
          (
            ps_route_scope_type *  routeScopePtr
          );

          ds::ErrorType GetRouteScope
          (
            ps_route_scope_type *  routeScopePtr
          );

          void SetSystemOption
          (
            void
          );

          ds::ErrorType SendMsg
          (
            const ds::SockAddrInternalType *       remoteAddrPtr,
            const IPort1::SeqBytes                 ioVecArr[],
            int32                                  numIOVec,
            int32 *                                numWrittenPtr,
            ds::Sock::IAncDataPriv **              inAncillaryDataPtrPtr,
            int32                                  inAncillaryDataLen,
            uint32                                 inFlags,
            int32                                  flags,
            ps_pkt_meta_info_type *                pktMetaInfoPtr
          );

          ds::ErrorType RecvMsg
          (
            ds::SockAddrInternalType *         remoteAddrPtr,
            IPort1::SeqBytes                   ioVecArr[],
            int32                              numIOVec,
            int32 *                            numReadPtr,
            ds::Sock::IAncDataPriv **          outAncillaryDataPtrPtr,
            int32                              outAncillaryDataLen,
            int32 *                            outAncillaryDataLenReqPtr,
            uint32 *                           outFlagsPtr,
            int32                              flags
          );

          ds::ErrorType SendDSMChain
          (
            const ds::SockAddrInternalType *   remoteAddrPtr,
            dsm_item_type **                   dsmItemPtrPtr,
            int32                              flags,
            int32 *                            numWrittenPtr
          );

          ds::ErrorType RecvDSMChain
          (
            ds::SockAddrInternalType *         remoteAddrPtr,
            dsm_item_type **                   dsmItemPtrPtr,
            int32                              flags,
            int32 *                            numReadPtr
          );

          ds::ErrorType Shutdown
          (
            ds::Sock::ShutdownDirType  shutdownDir
          );

          ds::ErrorType Close
          (
          ) throw();

          ds::ErrorType AbortConnection
          (
            ds::ErrorType  reasonForAbort
          );

          boolean Process
          (
            void *  userDataPtr
          );

          /* Retrieve the Linux socket descriptor */
          SOCKET GetLinuxSock () { return this->sSocket; }

          /* Retrieve the event listener object */
          PS::Sock::Platform::IEventListener * GetIEventListener() 
          { return this->eventListenerPtr; }

          /* Get/Set socket state */
          SockStateType GetState() { return sockstate; }
          void SetState(SockStateType state) { sockstate |= state; }
          void ClearState(SockStateType state) { sockstate &= ~state; }
          boolean IsStateSet(SockStateType state) { return (sockstate & state); }

          /* Get/Set/Clear pending events */
          SockEventType GetPendingEvents() { return pendingEvents; }
          void SetPendingEvent(SockEventType event) { pendingEvents |= event; }
          void ClearPendingEvent(SockEventType event) { pendingEvents &= ~event; }

        private:
          SOCKET                                sSocket;
          ds::Sock::FamilyType                  family;
          ds::Sock::SocketType                  sockType;
          ds::Sock::ProtocolType                protocol;
          SockEventType                         pendingEvents;
          PS::Sock::Platform::IEventListener  * eventListenerPtr;
          SockStateType                         sockstate;
          bool                                  bMcastBcast;
          ICritSect                           * critSectPtr;

          /* Socket Option Params */
          bool                                  bRecvErr;

       
          /* RtMetaInfo Params */
#ifdef FEATURE_SEC_IPSEC
          secips_ipsec_info_type                ipsec_info;
#endif
          ip_pkt_info_type                      pkt_info;
          uint32                                fi_mask;
          ps_iface_ipfltr_result_type           fi_result[IP_FLTR_CLIENT_OUTPUT_MAX];
          ps_iface_ipfltr_subset_id_type        subset_id;
          ps_ip_addr_type                       next_hop_addr;
          ip_addr_scope_enum_type               ip_addr_scope;
          void                                * routing_cache;
      }; /* Class LinuxSocket */


      class LinuxSocketFactory: public ISocketFactory
      {
        public:
       
          static LinuxSocketFactory * CreateInstance
          (
            void
          ); /* CreateInstance() */
          
          /* This function creates a platform and Linux socket */
          ISocket * CreateSocket
          (
            DS::Sock::FamilyType    family,
            DS::Sock::SocketType    sockType,
            DS::Sock::ProtocolType  protocol,
            ICritSect *             critSectPtr,
            DS::ErrorType *         dsErrnoPtr
          );

          void DeleteSocket
          (
            LinuxSocket *  sockPtr
          );

          void DeleteInstance
          (
            void
          );

        private:
          static LinuxSocketFactory *  sockFactoryPtr;
          static uint8            refCnt;

      }; /* Class SocketFactory */
    } /* namespace Platform */
  } /* namespace Sock */
} /* namespace PS */

#endif /* LINUX_PLATFORM_SOCKET_H */
