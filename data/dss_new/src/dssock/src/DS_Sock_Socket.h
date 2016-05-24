#ifndef DS_SOCK_SOCKET_H
#define DS_SOCK_SOCKET_H
/*===========================================================================
  @file DS_Sock_Socket.h

  This file defines the class that implements the ISocket interface.

  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_Socket.h#5 $
  $DateTime: 2010/05/20 06:52:24 $ $Author: smudired $

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"

#ifdef FEATURE_DATA_PS
#include "DS_Utils_CSSupport.h"
#include "DS_Utils_ICritSect.h"
#include "DS_Net_IPolicy.h"
#include "DS_Sock_ISocket.h"
#include "DS_Sock_ISocketPriv.h"
#include "IDSMUtils.h"
#include "PS_Sock_Platform_ISocket.h"
#include "PS_Sock_Platform_IEventListener.h"
#include "DS_Sock_EventDefs.h"
#include "AEEISignalBus.h"
#include "AEEISignal.h"
#include "AEEISignalCtl.h"
#include "DS_Utils_INode.h"
#include "DS_Net_INetwork.h"
#include "ps_rt_meta_info.h"
#include "ps_pkt_info.h"
#include "dsm.h"


/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/
//MSR draw a state diagram look at TCP RFC
//Add it to coding guidelines
namespace DS
{
  namespace Sock
  {
    typedef int32  PSMemEventType;

    namespace PSMemEvent
    {
      const PSMemEventType  PS_MEM_BUF_AVAILABLE = 0;
    }

    namespace Event
    {
      const EventType MAX_EV = Event::DOS_ACK + 1;

      typedef uint32  BitMaskType;

      #define TRANSIENT_ERR_BIT_MASK       0x0000FFFF
      #define FLOW_CONTROL_EVENT_BIT_MASK  0xFFFF0000
      #define MEM_CONTROL_EVENT_BIT_MASK   0xFF000000

      typedef enum
      {
        /*------------------------------------------------------------------
          Transient errors
        ------------------------------------------------------------------*/
        WRITE_BIT_MASK_NULL_ROUTING_INFO               = 0x00000001,
        WRITE_BIT_MASK_DORMANT_TRAFFIC_CHANNEL         = 0x00000002,

        /*------------------------------------------------------------------
          Flow control events
        ------------------------------------------------------------------*/
        WRITE_BIT_MASK_IFACE_NOT_WRITEABLE             = 0x00010000,
        WRITE_BIT_MASK_FLOW_DISABLED                   = 0x00020000,
        WRITE_BIT_MASK_PS_FLOW_SUSPENDED               = 0x00040000,
        WRITE_BIT_MASK_SOCKET_PLATFORM_DISABLED        = 0x00080000,
        WRITE_BIT_MASK_FLOW_FWDING_DISABLED            = 0x00100000,

        /*------------------------------------------------------------------
          Memory control events (these are also flow-control events.
          If any of these bits are set, then we do not post the event
          even if transient error bit masks might be set.)
        ------------------------------------------------------------------*/
        WRITE_BIT_MASK_SOCKET_PLATFORM_FLOW_CONTROLLED = 0x01000000,
        WRITE_BIT_MASK_PS_MEM_BUF_NOT_AVAILABLE        = 0x02000000

      } WriteBitMaskEnumType;

      typedef enum
      {

        /*-------------------------------------------------------------------
          Flow control events
        -------------------------------------------------------------------*/
        READ_BIT_MASK_SOCKET_PLATFORM_FLOW_CONTROLLED  = 0x00010000

      } ReadBitMaskEnumType;

      typedef enum
      {
        /*-------------------------------------------------------------------
          Flow control events
        -------------------------------------------------------------------*/
        ACCEPT_BIT_MASK_SOCKET_PLATFORM_FLOW_CONTROLLED  = 0x00010000

      } AcceptBitMaskEnumType;

      typedef enum
      {
        /*-------------------------------------------------------------------
          Flow control events
        -------------------------------------------------------------------*/
        CLOSE_BIT_MASK_SOCKET_PLATFORM_FLOW_CONTROLLED   = 0x00010000

      } CloseBitMaskEnumType;

      typedef enum
      {
        /*-------------------------------------------------------------------
          Transient errors
        -------------------------------------------------------------------*/
        DOS_ACK_BIT_MASK_INFO_AVAILABLE  = 0x00000001

      } DoSAckBitMaskEnumType;

    } /* namespace Event */

    /*lint -esym(1510, IQI) */
    /*lint -esym(1510, ISocketPriv) */
    class Socket : public ISocket,
                   public ISocketPriv,
                   public IDSMUtils,
                   public PS::Sock::Platform::IEventListener,
                   public DS::Utils::INode
    {
      public:
        /**
           @brief Binds a socket to an adress/port.

           This function associates a local IP address and a port number to the socket
           identified by the platformHandle. A random port is used to bind if zero port
           is used. This function also makes sure that the port is not in use by
           any other socket.

           @param[in] platformHandle Platform level socket descriptor
           @param[in] localAddrPtr Local IP address and port to be used in binding
           @see DS::Sock::SockAddrStorageType
           @param[out] dsErrnoPtr Updated with error value if the operation fails
           @see DS::Error

           @retval 0 The operation completed successfully
           @retval -1 The operation failed and dsErrnoPtr is updated with an error code
        */

        virtual DS::ErrorType CDECL Bind
        (
          const SockAddrStorageType *  localAddrPtr
        );

        virtual DS::ErrorType CDECL Connect
        (
          const SockAddrStorageType *  remoteAddrPtr
        );

        virtual DS::ErrorType CDECL Listen
        (
          int32  backlog
        );

        virtual DS::ErrorType CDECL Accept
        (
          SockAddrStorageType *  remoteAddrPtr,
          ISocket **             newSockPtr
        );

        /**
          @brief Returns socket's local address/port.

          Returns the local IP address and the port to which a socket is
          bound to.

          Caller must allocate an instance of SockAddrStorageType and pass it
          to this function. It is incorrect to allocate either SockAddrINType
          or SockAddrIN6Type, cast it to SockAddrStorageType and pass it to
          this function.

          If socket's family is INET, address family in localAddrPtr is set
          to DS::DSS_AF_INET and addr field is populated with IPv4 address,
          else address family is set to DS::DSS_AF_INET6 and addr field is
          populated with IPv6 address.

          @param[out] localAddrPtr  Local IP address and port to which socket
                                    is bound to

          @retval DS::Error::SUCCESS     Address/port is obtained successfully
          @retval DS::Error::DSS_EFAULT  localAddrPtr is NULL
        */
        virtual DS::ErrorType CDECL GetSockName
        (
          SockAddrStorageType *  localAddrPtr
        );

        /**
          @brief Returns socket's peer address/port.

          Returns the peer IP address and the port to which a socket is
          connected to. This function succeeds only when a socket is connected
          to a peer, i.e. it fails if socket is still trying to connect
          (as in caller called Connect() but 3-way handshake is not complete)
          or if socket was connected, but Close() is called.

          Caller must allocate an instance of SockAddrStorageType and pass it
          to this function. It is incorrect to allocate either SockAddrINType
          or SockAddrIN6Type, cast it to SockAddrStorageType and pass it to
          this function.

          If socket's family is INET, address family in remoteAddrPtr is set
          to DS::DSS_AF_INET and addr field is populated with IPv4 address,
          else address family is set to DS::DSS_AF_INET6 and addr field is
          populated with IPv6 address.

          @param[out] remoteAddrPtr  Peer IP address and port to which socket
                                     is bound to

          @retval DS::Error::SUCCESS       Address/port is obtained
                                           successfully
          @retval DS::Error::DSS_ENOTCONN  Socket is not in connected state
          @retval DS::Error::DSS_EFAULT    remoteAddrPtr is NULL
        */
        virtual DS::ErrorType CDECL GetPeerName
        (
          SockAddrStorageType *  remoteAddrPtr
        );

        virtual DS::ErrorType CDECL Write
        (
          const byte  bufArr[],
          int         bufLen,
          int32 *     numWrittenPtr
        );

        virtual DS::ErrorType CDECL WriteV
        (
          const SeqBytesType  ioVecArr[],
          int                 numIOVec,
          int32 *             numWrittenPtr
        );

        virtual DS::ErrorType CDECL SendTo
        (
          const byte                    bufArr[],
          int                           bufLen,
          const SockAddrStorageType *   remoteAddrPtr,
          uint32                        flags,
          int32 *                       numWrittenPtr
        );

        virtual DS::ErrorType CDECL Read
        (
          byte   bufArr[],
          int    bufLen,
          int *  numReadPtr
        );

        virtual DS::ErrorType CDECL ReadV
        (
          SeqBytesType  ioVecArr[],
          int               numIOVec,
          int *             numReadPtr
        );

        virtual DS::ErrorType CDECL RecvFrom
        (
          byte                   bufArr[],
          int                    bufLen,
          int *                  numReadPtr,
          uint32                 flags,
          SockAddrStorageType *  remoteAddrPtr
        );

        virtual DS::ErrorType CDECL SendMsg
        (
          const SockAddrStorageType *  remoteAddrPtr,
          const SeqBytesType           ioVecArr[],
          int                          numIOVec,
          int32 *                      numWrittenPtr,
          IAncData **                  inAncillaryDataPtrPtr,
          int                          inAncillaryDataLen,
          uint32                       inFlags,
          uint32                       flags
        );

        virtual DS::ErrorType CDECL RecvMsg
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
        );

        virtual DS::ErrorType CDECL RegEvent
        (
          ISignal *  signalObjPtr,
          EventType  event
        );

        virtual DS::ErrorType CDECL Shutdown
        (
          ShutdownDirType  shutdownDir
        );

        virtual DS::ErrorType CDECL SetOpt
        (
          OptLevelType  optLevel,
          OptNameType   optName,
          int32         optVal,
          int32         optLen
        );

        virtual DS::ErrorType CDECL GetOpt
        (
          OptLevelType  optLevel,
          OptNameType   optName,
          int32 *       optValPtr,
          int32 *       optLenPtr
        );

        /**
          @brief Updates network policy.

          Updates socket's network policy.
          //TODO Copy all info from IsSetNetPolicySupported

          @param[in] netPolicyPtr  Ptr to IPolicy object

          @retval DS::Error::SUCCESS         Network policy is updated
                                             successfully
          @retval DS::Error::DSS_EFAULT      netPolicyPtr is NULL
          @retval DS::Error::DSS_EOPNOTSUPP  Network policy can't be updated
                                             in current state

          @see DS::Net::IPolicy, DS::Net::INetwork, DS::Error
        */
        virtual DS::ErrorType CDECL SetNetPolicy
        (
          DS::Net::IPolicy *  netPolicyPtr
        );

        /**
          @brief Associates socket with a network object.

          Associates socket with the passed in network object. From this
          point, socket must use the network interface pointed to by INetwork
          object in data path. Socket can never use any other interface.

          It is not allowed to perform this operation if
            1. SetNetPolicy() was called before OR
            2. SetNetwork() was called before OR
            3. Connect(), Listen(), or Close() were called
            //TODO Case 3 is not handled

          @param[in] netObjPtr  Ptr to INetwork object

          @retval DS::Error::SUCCESS         Socket is associated with
                                             INetwork object successfully
          @retval DS::Error::DSS_EFAULT      netObjPtr is NULL
          @retval DS::Error::DSS_EOPNOTSUPP  Socket can't be associated with
                                             INetwork object in current state
          @retval DS::Error::DSS_EINVAL      IPolicy object couldn't be
                                             fetched from INetwork object

          @see DS::Net::INetwork, DS::Net::IPolicy, DS::Error
        */
        virtual DS::ErrorType SetNetwork
        (
          DS::Net::INetwork *  netObjPtr
        );

        /**
          @brief Non-blocking API to close socket.

          Provides a non-blocking API to close a socket. It returns SUCCESS
          if socket can be closed immediately. For Connected TCP socket, it
          returns DSS_EWOULDBLOCK as the socket needs to run connection
          termination procedure. In this case, caller is expected to register
          for CLOSE_EV and wait for it. When socket is successfully closed,
          sockets library posts CLOSE_EV.

          Once the socket is closed, it is invalid to perform any operations
          on it.

          //TODO Talk about linger reset
          @param None

          @retval DS::Error::SUCCESS          Socket is closed successfully
          @retval DS::Error::DSS_EWOULDBLOCK  Socket needs to be closed
                                              asynchronously
          @retval DS::Error::DSS_ENOMEM       Out of resources

          @see DS::Sock::Event, DS::Sock::Socket::RegEvent, DS::Error
        */
        virtual DS::ErrorType CDECL Close
        (
          void
        ) throw();

        virtual DS::ErrorType CDECL AddIPMembership
        (
          const IPMembershipInfoType *  ipMembershipPtr
        );

        virtual DS::ErrorType CDECL DropIPMembership
        (
          const IPMembershipInfoType *  ipMembershipPtr
        );

        virtual DS::ErrorType CDECL GetSOLingerReset
        (
          LingerType *  soLingerPtr
        );

        virtual DS::ErrorType CDECL SetSOLingerReset
        (
          const LingerType *  soLingerPtr
        );

        virtual DS::ErrorType CDECL GetDoSAckInfo
        (
          DS::Sock::DoSAckStatusType *  dosAckStatusPtr,
          uint32 *                      overflowPtr
        );

        /*-------------------------------------------------------------------
          Platform::IEventListener methods
        -------------------------------------------------------------------*/
        virtual void ProcessEvent
        (
          PS::Sock::Platform::EventType  event
        ) throw();

        /*-------------------------------------------------------------------
          DS::Utils::INode methods
        -------------------------------------------------------------------*/
        virtual boolean Process
        (
          void *  userDataPtr
        );

        /*-------------------------------------------------------------------
          ISocketPriv methods
        -------------------------------------------------------------------*/
        virtual DS::ErrorType CDECL GetSystemOption
        (
          boolean *  isSystemSocketPtr
        );

        virtual DS::ErrorType CDECL SetSystemOption
        (
          boolean  isSystemSocket
        );

        /*-------------------------------------------------------------------
          IDSMUtils methods
        -------------------------------------------------------------------*/
        virtual DS::ErrorType CDECL ReadDSMChain
        (
          dsm_item_type **  dsmItemPtrPtr,
          int32 *           numBytesReadPtr
        );

        virtual DS::ErrorType CDECL WriteDSMChain
        (
          dsm_item_type **  dsmItemPtrPtr,
          int32 *           numBytesWrittenPtr
        );

        virtual DS::ErrorType CDECL SendToDSMChain
        (
          dsm_item_type **                       dsmItemPtrPtr,
          const DS::Sock::SockAddrStorageType *  remoteAddrPtr,
          unsigned int                           flags,
          int32 *                                numBytesSentPtr
        );

        virtual DS::ErrorType CDECL RecvFromDSMChain
        (
          dsm_item_type **                 dsmItemPtrPtr,
          DS::Sock::SockAddrStorageType *  remoteAddrPtr,
          unsigned int                     flags,
          int32 *                          numBytesRcvdPtr
        );

        /*-------------------------------------------------------------------
          IQI interface Methods
        -------------------------------------------------------------------*/
        DSIQI_DECL_LOCALS()
        DSIQI_ADDREF()
        DSIQI_RELEASE()
        virtual DS::ErrorType CDECL QueryInterface
        (
          AEEIID   iid,
          void **  objPtrPtr
        );

      protected:
        /**
          @brief Constructor for Socket class.

          Initializes a Socket object.
        */
        Socket
        (
          void
        );

        /**
          @brief Destructor for Socket class.

          Resets the socket object. Method is defined as virtual
          so that destructors of derived classes are called when Socket
          object is deleted.
        */
        virtual ~Socket
        (
          void
        ) throw();

        /**
          @brief Initializes a socket object.

          Initializes the rest of the socket object. These operations could
          fail and since constructor can't fail, these operations can't be
          performed as part of constructor.

          This method performs following operations
            @li Creates a platform socket
            @li Registers for PLATFORM_ENABLED and PLATFORM_DISABLED events
                with platform socket

          Socket object is deleted if any of the above operations fail.

          @param[in] family    The family of socket to be created
          @param[in] sockType  The type of socket to be created
          @param[in] protocol  The protocol to be used

          @retval DS::Error::SUCCESS  Socket is initialized successfully
          @retval DS::Error::EMFILE   Out of memory

          @see DS::Sock::Family, DS::Sock::Type, DS::Sock::Protocol, DS::Error
        */
        virtual DS::ErrorType Init
        (
          FamilyType     family,
          SocketType     sockType,
          ProtocolType   protocol
        );

        /**
          @brief Copies information from a socket object.

          Copies members from passed in socket object. This method is used
          after a new TCP socket is accepted.

          @param[in] sockPtr  Socket object from which members are to be copied

          @retval None
        */
        DS::ErrorType CloneSocket
        (
          Socket *  sockPtr
        );

        virtual bool IsConnectSupported
        (
          const SockAddrIN6Type *  v6RemoteAddr,
          DS::ErrorType *          dsErrnoPtr
        ) = 0;

        virtual bool IsOptSupported
        (
          OptLevelType  optLevel,
          OptNameType   optName
        ) = 0;

        /**
          @brief Checks if socket object's NetPolicy can be updated

          Socket's policy can't be updated if
            1. Socket is created via SocketFactory::CreateSocketByNetwork() OR
            2. Socket is created via Accept() OR
            3. Connect() had been called OR
            4. Listen() had been called OR
            5. Close() had been called

          In case 1, socket object's netObjPtr is non-0. As, socket must use
          the interface pointed to by INetwork object in this case, it is
          invalid to update policy

          In cases 2 & 3, socket is connected to a peer and updating policy
          make make this peer unreachable.
          //TODO Why case 4?

          @param None

          @retval true   If policy can be updated
          @retval false  If policy can't be updated

          @see DS::Net::IPolicy
        */
        virtual bool IsSetNetPolicySupported
        (
          void
        );

        /**
          @brief Checks if Multicast is supported.

          Multicast is supported only by UDP. So this method returns if a
          socket object supports Multicast.

          @param None

          @retval true   If Multicast is supported
          @retval false  If Multicast is not supported
        */
        virtual bool IsMulticastSupported
        (
          void
        );

        virtual DS::ErrorType FillProtocolInfoInPktInfo
        (
          const SockAddrIN6Type *  v6RemoteAddrPtr,
          const SockAddrIN6Type *  v6LocalAddrPtr,
          ip_pkt_info_type *       pktInfoPtr
        ) = 0;

        /**
          @brief Sets an event bit mask for the given event.

          Sets an event bit mask for the given event. After setting the event
          bit mask, posts event if
            1. Any of the transient error bits are set OR
            2. None of the event error bits are set

          @param[in] event    Event whose bit mask need to be updated
          @param[in] bitMask  bit mask to set

          @retval None

          @see DS::Sock::Event
        */
        void SetEventBitMask
        (
          EventType  event,
          uint32     bitMask
        ) throw();

        /**
          @brief Resets an event bit mask for the given event.

          Resets an event bit mask for the given event. After resetting the
          event bit mask, posts event if
            1. Any of the transient error bits are set OR
            2. None of the event error bits are set

          @param[in] event    Event whose bit mask need to be updated
          @param[in] bitMask  bit mask to reset

          @retval None

          @see DS::Sock::Event
        */
        void ResetEventBitMask
        (
          EventType  event,
          uint32     bitMask
        ) throw();

        virtual bool IsPktInfoDifferent
        (
          const SockAddrStorageType *  remoteAddrPtr
        );

        bool IsFlowEnabled
        (
          void
        );

        virtual DS::ErrorType HandleNonZeroWriteEventBitMask
        (
          const SockAddrStorageType *  remoteAddrPtr
        );

        DS::ErrorType GeneratePktMetaInfo
        (
          ps_pkt_meta_info_type **  pktMetaInfoPtr,
          uint32                    flags
        );


        virtual void ProcessNetworkConfigChangedEvent
        (
          DS::ErrorType  reasonForChange
        ) = 0;

        virtual void ProcessDoSAckEvent
        (
          DS::Sock::Event::DoSAckEventInfo *  dosEventInfoPtr
        );

        ICritSect *                    critSectPtr;
        PS::Sock::Platform::ISocket *  platformSockPtr;
        Event::BitMaskType             eventBitMask[Event::MAX_EV];
        ps_rt_meta_info_type *         rtMetaInfoPtr;
        bool                           sendPktMetaInfo;

      private:
        virtual DS::ErrorType RoutePacket
        (
          const SockAddrStorageType *  remoteAddrPtr
        );

        virtual DS::ErrorType GeneratePktInfo
        (
          const SockAddrIN6Type *    v6RemoteAddrPtr,
          ip_pkt_info_type *         pktInfoPtr
        );

        DS::ErrorType UpdateSrcAddrInPktInfo
        (
          const SockAddrIN6Type *  v6RemoteAddrPtr,
          ps_rt_meta_info_type *   rtMetaInfoPtr
        );

        /**
          @brief Posts an event.

          If one or more signals had been registered for this event, those
          signals are set.

          @param[in] event  Event to be posted

          @retval None

          @see DS::Sock::Event, DS::Utils::Signal
        */
        void PostEvent
        (
          EventType  event
        ) throw();

        DS::ErrorType RegNetworkStateChangedEvent
        (
          void
        );

        static void NetworkStateChangedEventCback
        (
          void *  userDataPtr
        );

        void ProcessNetworkStateChangedEvent
        (
          void
        );

        void UpdateRouteScopeByPolicy
        (
          void
        );

        void DeleteRoutingCacheFromRouteScope
        (
          int32  routingCache
        );

        DS::ErrorType UpdateRtMetaInfoWithFlowFwding
        (
          void
        );

        DS::ErrorType UseOrigFltrResult
        (
          void
        );

        void UseDefaultPSFlow
        (
          void
        );

        FamilyType            family;
        DS::Net::IPolicy *    policyPtr;
        DS::Net::INetwork *   netObjPtr;
        ISignalCtl *          networkStateChangedSignalCtlPtr;
        ISignalBus *          eventSignalBusPtr[Event::MAX_EV];
        int32                 origFltrResult;
        bool                  isSystemSocket;
    };
  }
}

#endif /* FEATURE_DATA_PS */
#endif /* DS_SOCK_SOCKET_H */

