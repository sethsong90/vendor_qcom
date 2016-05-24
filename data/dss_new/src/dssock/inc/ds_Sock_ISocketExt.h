#ifndef DS_SOCK_ISOCKETEXT_H
#define DS_SOCK_ISOCKETEXT_H

/*============================================================================
  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Net_IPolicy.h"

/**
  * This module includes socket events
  */
typedef int ds_Sock_SocketEventType;

/** @memberof ds_Sock_SocketEvent
  * 
  * Socket events to be used with RegEvent API.
  * The values do not imply any specific meaning.
  */
#define ds_Sock_SocketEvent_QDS_EV_WRITE 0x1
#define ds_Sock_SocketEvent_QDS_EV_READ 0x2
#define ds_Sock_SocketEvent_QDS_EV_CLOSE 0x4
#define ds_Sock_SocketEvent_QDS_EV_ACCEPT 0x8
#define ds_Sock_SocketEvent_QDS_EV_DOS_ACK 0x8000
struct ds_Sock_IPMembershipInfoType {
   ds_Net_IfaceIdType ifaceId;
   ds_IPAddrType mcastGroup; /**< IP address of the Multicast group. */
   AEEINTERFACE_PADMEMBERS(__pad, 2)
};
typedef struct ds_Sock_IPMembershipInfoType ds_Sock_IPMembershipInfoType;
typedef int ds_Sock_DoSAckStatusType;

/** @memberof ds_Sock_DoSAckStatus
  * 
  * Status is not available.
  */
#define ds_Sock_DoSAckStatus_DOSACK_NONE -1

/** @memberof ds_Sock_DoSAckStatus
  * 
  * Packet is sent successfully.
  */
#define ds_Sock_DoSAckStatus_DOSACK_OK 0

/** @memberof ds_Sock_DoSAckStatus
  * 
  * Hold orig timer expired and hence failed to send the packet.
  */
#define ds_Sock_DoSAckStatus_HOLD_ORIG_RETRY_TIMEOUT 1

/** @memberof ds_Sock_DoSAckStatus
  * 
  * Unable to process the packet because hold orig is true.
  */
#define ds_Sock_DoSAckStatus_HOLD_ORIG 2

/** @memberof ds_Sock_DoSAckStatus
  * 
  * Failed to send the packet due to lack of service.
  */
#define ds_Sock_DoSAckStatus_DOSACK_NO_SRV 3

/** @memberof ds_Sock_DoSAckStatus
  * 
  * Aborted the operation.
  */
#define ds_Sock_DoSAckStatus_DOSACK_ABORT 4

/** @memberof ds_Sock_DoSAckStatus
  * 
  * DOS is not supported in analog mode.
  */
#define ds_Sock_DoSAckStatus_NOT_ALLOWED_IN_AMPS 5

/** @memberof ds_Sock_DoSAckStatus
  * 
  * DOS is not supported when in a HDR call.
  */
#define ds_Sock_DoSAckStatus_NOT_ALLOWED_IN_HDR 6

/** @memberof ds_Sock_DoSAckStatus
  * 
  * Failed to receive Layer 2 ACK.
  */
#define ds_Sock_DoSAckStatus_L2_ACK_FAILURE 7

/** @memberof ds_Sock_DoSAckStatus
  * 
  * Unable to process the packet because of lack of resources.
  */
#define ds_Sock_DoSAckStatus_OUT_OF_RESOURCES 8

/** @memberof ds_Sock_DoSAckStatus
  * 
  * Packet is too big to be sent over access channel.
  */
#define ds_Sock_DoSAckStatus_ACCESS_TOO_LARGE 9

/** @memberof ds_Sock_DoSAckStatus
  * 
  * Packet is too big to be sent over DTC.
  */
#define ds_Sock_DoSAckStatus_DTC_TOO_LARGE 10

/** @memberof ds_Sock_DoSAckStatus
  * 
  * Access channel is blocked for traffic based on service option.
  */
#define ds_Sock_DoSAckStatus_ACCT_BLOCK 11

/** @memberof ds_Sock_DoSAckStatus
  * 
  * Failed to receive Layer 3 ACK.
  */
#define ds_Sock_DoSAckStatus_L3_ACK_FAILURE 12

/** @memberof ds_Sock_DoSAckStatus
  * 
  * Failed for some other reason.
  */
#define ds_Sock_DoSAckStatus_DOSACK_OTHER 13
#define ds_Sock_AEEIID_ISocketExt 0x1098bc6

/** @interface ds_Sock_ISocketExt
  * 
  * The ISocketExt interface provides extended socket functionality.
  */
#define INHERIT_ds_Sock_ISocketExt(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*RegEvent)(iname* _pif, ISignal* signal, ds_Sock_SocketEventType eventID, IQI** regObj); \
   AEEResult (*AddIPMembership)(iname* _pif, const ds_Sock_IPMembershipInfoType* ipMembership); \
   AEEResult (*DropIPMembership)(iname* _pif, const ds_Sock_IPMembershipInfoType* ipMembership); \
   AEEResult (*GetDoSAckInfo)(iname* _pif, ds_Sock_DoSAckStatusType* dosAckInfo, int* overflow); \
   AEEResult (*SetNetPolicy)(iname* _pif, ds_Net_IPolicy* policy)
AEEINTERFACE_DEFINE(ds_Sock_ISocketExt);

/** @memberof ds_Sock_ISocketExt
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Sock_ISocketExt_AddRef(ds_Sock_ISocketExt* _pif)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocketExt)->AddRef(_pif);
}

/** @memberof ds_Sock_ISocketExt
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Sock_ISocketExt_Release(ds_Sock_ISocketExt* _pif)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocketExt)->Release(_pif);
}

/** @memberof ds_Sock_ISocketExt
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Sock_ISocketExt_QueryInterface(ds_Sock_ISocketExt* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocketExt)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Sock_ISocketExt
  * 
  * This function registers a signal to be set when event eventID occurs.
  * Cannot use the signal for more than one eventID. Can use more than one
  * signal for the same event.
  * @param _pif Pointer to interface
  * @param signal The signal to Set() when the state associated with
  *               the eventID changes. To cancel the registration the
  *               application should release this signal. 
  * @param eventID The event for which the signal shall be fired.
  * @param regObj Output The application must hold this output registration
  *                      object to ensure its Signal stays registered. The
  *                      application should release this object once it has
  *                      released the signal object.
  * @retval AEE_SUCCESS Signal set completed successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Sock_ISocketExt_RegEvent(ds_Sock_ISocketExt* _pif, ISignal* signal, ds_Sock_SocketEventType eventID, IQI** regObj)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocketExt)->RegEvent(_pif, signal, eventID, regObj);
}

/** @memberof ds_Sock_ISocketExt
  * 
  * Join a multicast group.
  * @param _pif Pointer to interface
  * @param ipMembership Information of the Multicast group to join and
  *                     an iface identification. If supported, 
  *                     applications can join the same Multicast group
  *                     on different ifaces.
  *                     If INVALID_IFACE_ID is specified for ifaceId an
  *                     appropriate iface would be selected for the
  *                     operation or an applicable error returned if 
  *                     appropriate iface cannot be found. 
  * @retval AEE_SUCCESS The operation completed successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Sock_ISocketExt_AddIPMembership(ds_Sock_ISocketExt* _pif, const ds_Sock_IPMembershipInfoType* ipMembership)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocketExt)->AddIPMembership(_pif, ipMembership);
}

/** @memberof ds_Sock_ISocketExt
  * 
  * Leave a multicast group.
  * @param _pif Pointer to interface
  * @param ipMembership Information of the multicast group to leave.
  *                     @See AddIPMembership
  * @retval AEE_SUCCESS The operation completed successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Sock_ISocketExt_DropIPMembership(ds_Sock_ISocketExt* _pif, const ds_Sock_IPMembershipInfoType* ipMembership)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocketExt)->DropIPMembership(_pif, ipMembership);
}

/** @memberof ds_Sock_ISocketExt
  * 
  * Get DoS (Data Over Signaling) Ack information.
  * DoS and SDB (Short Data Burst) are parallel terms.
  * DoS is the term used in 1xEVDO
  * SDB is the term used in CDMA 1x.      
  * @param _pif Pointer to interface
  * @param dosAckInfo Information of DoS Ack.
  * @param overflow Set to a nonzero value, if the number of outstanding
  *                 SDB/DOS packets (the packets for which mobile is
  *                 still waiting for an ACK) is more than that the
  *                 mobile can handle.
  * @retval AEE_SUCCESS The operation completed successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Sock_DoSAckStatusType
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Sock_ISocketExt_GetDoSAckInfo(ds_Sock_ISocketExt* _pif, ds_Sock_DoSAckStatusType* dosAckInfo, int* overflow)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocketExt)->GetDoSAckInfo(_pif, dosAckInfo, overflow);
}

/** @memberof ds_Sock_ISocketExt
  * 
  * This function sets the network policy to be used with the socket. For TCP sockets,
  * policy change is not supported after listen or connect have been issued on the socket.
  * For UDP sockets, policy change is supported as long as the socket is not connected.
  * @param _pif Pointer to interface
  * @param policy Network Policy (specifies technology, profile etc.). See IPolicy.
  * @retval AEE_SUCCESS Network Selection completed successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Sock_ISocketExt_SetNetPolicy(ds_Sock_ISocketExt* _pif, ds_Net_IPolicy* policy)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocketExt)->SetNetPolicy(_pif, policy);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Net_IPolicy.h"
namespace ds
{
   namespace Sock
   {
      
      /**
        * This module includes socket events
        */
      typedef int SocketEventType;
      namespace SocketEvent
      {
         
         /**
           * Socket events to be used with RegEvent API.
           * The values do not imply any specific meaning.
           */
         const ::ds::Sock::SocketEventType QDS_EV_WRITE = 0x1;
         const ::ds::Sock::SocketEventType QDS_EV_READ = 0x2;
         const ::ds::Sock::SocketEventType QDS_EV_CLOSE = 0x4;
         const ::ds::Sock::SocketEventType QDS_EV_ACCEPT = 0x8;
         const ::ds::Sock::SocketEventType QDS_EV_DOS_ACK = 0x8000;
      };
      struct IPMembershipInfoType {
         ::ds::Net::IfaceIdType ifaceId;
         ::ds::IPAddrType mcastGroup; /**< IP address of the Multicast group. */
         AEEINTERFACE_PADMEMBERS(__pad, 2)
      };
      typedef int DoSAckStatusType;
      namespace DoSAckStatus
      {
         
         /**
           * Status is not available.
           */
         const ::ds::Sock::DoSAckStatusType DOSACK_NONE = -1;
         
         /**
           * Packet is sent successfully.
           */
         const ::ds::Sock::DoSAckStatusType DOSACK_OK = 0;
         
         /**
           * Hold orig timer expired and hence failed to send the packet.
           */
         const ::ds::Sock::DoSAckStatusType HOLD_ORIG_RETRY_TIMEOUT = 1;
         
         /**
           * Unable to process the packet because hold orig is true.
           */
         const ::ds::Sock::DoSAckStatusType HOLD_ORIG = 2;
         
         /**
           * Failed to send the packet due to lack of service.
           */
         const ::ds::Sock::DoSAckStatusType DOSACK_NO_SRV = 3;
         
         /**
           * Aborted the operation.
           */
         const ::ds::Sock::DoSAckStatusType DOSACK_ABORT = 4;
         
         /**
           * DOS is not supported in analog mode.
           */
         const ::ds::Sock::DoSAckStatusType NOT_ALLOWED_IN_AMPS = 5;
         
         /**
           * DOS is not supported when in a HDR call.
           */
         const ::ds::Sock::DoSAckStatusType NOT_ALLOWED_IN_HDR = 6;
         
         /**
           * Failed to receive Layer 2 ACK.
           */
         const ::ds::Sock::DoSAckStatusType L2_ACK_FAILURE = 7;
         
         /**
           * Unable to process the packet because of lack of resources.
           */
         const ::ds::Sock::DoSAckStatusType OUT_OF_RESOURCES = 8;
         
         /**
           * Packet is too big to be sent over access channel.
           */
         const ::ds::Sock::DoSAckStatusType ACCESS_TOO_LARGE = 9;
         
         /**
           * Packet is too big to be sent over DTC.
           */
         const ::ds::Sock::DoSAckStatusType DTC_TOO_LARGE = 10;
         
         /**
           * Access channel is blocked for traffic based on service option.
           */
         const ::ds::Sock::DoSAckStatusType ACCT_BLOCK = 11;
         
         /**
           * Failed to receive Layer 3 ACK.
           */
         const ::ds::Sock::DoSAckStatusType L3_ACK_FAILURE = 12;
         
         /**
           * Failed for some other reason.
           */
         const ::ds::Sock::DoSAckStatusType DOSACK_OTHER = 13;
      };
      const ::AEEIID AEEIID_ISocketExt = 0x1098bc6;
      
      /** @interface ISocketExt
        * 
        * The ISocketExt interface provides extended socket functionality.
        */
      struct ISocketExt : public ::IQI
      {
         
         /**
           * This function registers a signal to be set when event eventID occurs.
           * Cannot use the signal for more than one eventID. Can use more than one
           * signal for the same event.
           * @param signal The signal to Set() when the state associated with
           *               the eventID changes. To cancel the registration the
           *               application should release this signal. 
           * @param eventID The event for which the signal shall be fired.
           * @param regObj Output The application must hold this output registration
           *                      object to ensure its Signal stays registered. The
           *                      application should release this object once it has
           *                      released the signal object.
           * @retval AEE_SUCCESS Signal set completed successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL RegEvent(::ISignal* signal, ::ds::Sock::SocketEventType eventID, ::IQI** regObj) = 0;
         
         /**
           * Join a multicast group.
           * @param ipMembership Information of the Multicast group to join and
           *                     an iface identification. If supported, 
           *                     applications can join the same Multicast group
           *                     on different ifaces.
           *                     If INVALID_IFACE_ID is specified for ifaceId an
           *                     appropriate iface would be selected for the
           *                     operation or an applicable error returned if 
           *                     appropriate iface cannot be found. 
           * @retval AEE_SUCCESS The operation completed successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL AddIPMembership(const ::ds::Sock::IPMembershipInfoType* ipMembership) = 0;
         
         /**
           * Leave a multicast group.
           * @param ipMembership Information of the multicast group to leave.
           *                     @See AddIPMembership
           * @retval AEE_SUCCESS The operation completed successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL DropIPMembership(const ::ds::Sock::IPMembershipInfoType* ipMembership) = 0;
         
         /**
           * Get DoS (Data Over Signaling) Ack information.
           * DoS and SDB (Short Data Burst) are parallel terms.
           * DoS is the term used in 1xEVDO
           * SDB is the term used in CDMA 1x.      
           * @param dosAckInfo Information of DoS Ack.
           * @param overflow Set to a nonzero value, if the number of outstanding
           *                 SDB/DOS packets (the packets for which mobile is
           *                 still waiting for an ACK) is more than that the
           *                 mobile can handle.
           * @retval AEE_SUCCESS The operation completed successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Sock_DoSAckStatusType
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetDoSAckInfo(::ds::Sock::DoSAckStatusType* dosAckInfo, int* overflow) = 0;
         
         /**
           * This function sets the network policy to be used with the socket. For TCP sockets,
           * policy change is not supported after listen or connect have been issued on the socket.
           * For UDP sockets, policy change is supported as long as the socket is not connected.
           * @param policy Network Policy (specifies technology, profile etc.). See IPolicy.
           * @retval AEE_SUCCESS Network Selection completed successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetNetPolicy(::ds::Net::IPolicy* policy) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_SOCK_ISOCKETEXT_H
