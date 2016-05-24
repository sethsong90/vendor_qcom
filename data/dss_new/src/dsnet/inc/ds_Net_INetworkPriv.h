#ifndef DS_NET_INETWORKPRIV_H
#define DS_NET_INETWORKPRIV_H

/*============================================================================
  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Net_DownReasons_Def.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_INetwork.h"
#include "ds_Net_IPolicy.h"
#include "ds_Net_IPolicyPriv.h"
#include "ds_Net_QoS_Def.h"
#include "ds_Net_IBearerInfo.h"
struct ds_Net__SeqIfaceIdType__seq_unsignedLong {
   ds_Net_IfaceIdType* data;
   int dataLen;
   int dataLenReq;
};
typedef struct ds_Net__SeqIfaceIdType__seq_unsignedLong ds_Net__SeqIfaceIdType__seq_unsignedLong;
typedef ds_Net__SeqIfaceIdType__seq_unsignedLong ds_Net_SeqIfaceIdType;
typedef int ds_Net_IfaceNameTypePriv;

/** @memberof ds_Net_IfaceName
  * 
  * IfaceNameTypePriv is part of the enumeration in IfaceNameType.
  * MBMS iface is moved here until MBMS feature is released publicly.
  */
#define ds_Net_IfaceName_IFACE_MBMS 0x8801

/** @memberof ds_Net_NetworkEvent
  * 
  * IDLE event is fired when number of sockets using underlying 
  * network interface is dropped to zero.
  * Registration for this event can be performed 
  * after creation of Network object.
  */
#define ds_Net_NetworkEvent_QDS_EV_IDLE 0x109d887
struct ds_Net_Ipv4Addr {
   unsigned int ps_s_addr;
};
typedef struct ds_Net_Ipv4Addr ds_Net_Ipv4Addr;
#define ds_Net_ChaddrLen_MAX_CHADDR_LEN 16
#define ds_Net_ChostnameLen_MAX_CHOSTNAME_LEN 32
struct ds_Net_DhcpServerConnDevicesInfo {
   ds_Net_Ipv4Addr client_ip;
   unsigned char client_hw[16];
   unsigned int client_hw_len;
   unsigned char client_hostname[32];
   unsigned int client_hostname_len;
   unsigned int client_connected_time;
};
typedef struct ds_Net_DhcpServerConnDevicesInfo ds_Net_DhcpServerConnDevicesInfo;
struct ds_Net__DhcpGetDeviceInfoListType__seq_DhcpServerConnDevicesInfo_Net_ds {
   ds_Net_DhcpServerConnDevicesInfo* data;
   int dataLen;
   int dataLenReq;
};
typedef struct ds_Net__DhcpGetDeviceInfoListType__seq_DhcpServerConnDevicesInfo_Net_ds ds_Net__DhcpGetDeviceInfoListType__seq_DhcpServerConnDevicesInfo_Net_ds;
typedef ds_Net__DhcpGetDeviceInfoListType__seq_DhcpServerConnDevicesInfo_Net_ds ds_Net_DhcpGetDeviceInfoListType;
struct ds_Net_DhcpGetDeviceInfoType {
   ds_Net_DhcpServerConnDevicesInfo* dev_info;
   int dev_infoLen;
   int dev_infoLenReq;
};
typedef struct ds_Net_DhcpGetDeviceInfoType ds_Net_DhcpGetDeviceInfoType;
struct ds_Net_DhcpArpCacheType {
   unsigned int ip_addr;
   byte hw_addr[16];
   int hw_addr_len;
};
typedef struct ds_Net_DhcpArpCacheType ds_Net_DhcpArpCacheType;
typedef ds_Net_DhcpArpCacheType ds_Net_DhcpArpCacheUpdateType;
typedef ds_Net_DhcpArpCacheType ds_Net_DhcpArpCacheClearType;
#define ds_Net_AEEIID_INetworkPriv 0x1072cf1

/** @interface ds_Net_INetworkPriv
  * 
  * ds Net Network Privileged interface.
  * This interface is intended for internal usage for backward compatibility purposes.
  * It provides additional functionalities to INetwork.
  * It exhibits different behavior than INetwork, specifically - the Data Network is not brought up
  * automatically when INetworkPriv is instantiated.
  */
#define INHERIT_ds_Net_INetworkPriv(iname) \
   INHERIT_ds_Net_IEventManager(iname); \
   AEEResult (*BringUpInterface)(iname* _pif); \
   AEEResult (*LookupInterface)(iname* _pif); \
   AEEResult (*IsLaptopCallActive)(iname* _pif, boolean* isActive); \
   AEEResult (*GetAllIfaces)(iname* _pif, ds_Net_IfaceIdType* allIfaces, int allIfacesLen, int* allIfacesLenReq); \
   AEEResult (*GetPreviousState)(iname* _pif, ds_Net_NetworkStateType* netState); \
   AEEResult (*GetPreviousIPAddr)(iname* _pif, ds_IPAddrType* address); \
   AEEResult (*GetPreviousBearerInfo)(iname* _pif, ds_Net_IBearerInfo** bearerTech); \
   AEEResult (*GetQoSAwareInfoCode)(iname* _pif, ds_Net_QoSInfoCodeType* infoCode); \
   AEEResult (*GetPolicy)(iname* _pif, ds_Net_IPolicy** policy); \
   AEEResult (*SetPolicy)(iname* _pif, ds_Net_IPolicy* policy); \
   AEEResult (*EnableDNSDuringIPCP)(iname* _pif, boolean enable); \
   AEEResult (*GetDhcpDeviceInfo)(iname* _pif, ds_Net_DhcpGetDeviceInfoType* connDevInfo); \
   AEEResult (*DhcpArpCacheUpdate)(iname* _pif, const ds_Net_DhcpArpCacheUpdateType* dhcpArpCacheUpdateParam); \
   AEEResult (*DhcpArpCacheClear)(iname* _pif, const ds_Net_DhcpArpCacheClearType* dhcpArpCacheClearParam)
AEEINTERFACE_DEFINE(ds_Net_INetworkPriv);

/** @memberof ds_Net_INetworkPriv
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INetworkPriv_AddRef(ds_Net_INetworkPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkPriv)->AddRef(_pif);
}

/** @memberof ds_Net_INetworkPriv
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INetworkPriv_Release(ds_Net_INetworkPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkPriv)->Release(_pif);
}

/** @memberof ds_Net_INetworkPriv
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_INetworkPriv_QueryInterface(ds_Net_INetworkPriv* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkPriv)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_INetworkPriv
  * 
  * This function registers a signal to be set when event eventID occurs.
  * Cannot use the signal for more than one eventID. Can use more than one
  * signal for the same event.
  * @param _pif Pointer to interface
  * @param signal The signal to Set() when the state associated with the eventID changes.
  *               To cancel the registration the application should release this signal. 
  * @param eventID The event for which the signal shall be fired.
  * @param regObj Output The application must hold this output registration object to ensure its
  *                      Signal stays registered. The application can release this object once
  *                      it has released the signal object.
  * @retval AEE_SUCCESS Signal set completed successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkPriv_OnStateChange(ds_Net_INetworkPriv* _pif, ISignal* signal, ds_Net_EventType eventID, IQI** regObj)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkPriv)->OnStateChange(_pif, signal, eventID, regObj);
}

/** @memberof ds_Net_INetworkPriv
  * 
  * This function explicitly brings up the network.        
  * @param _pif Pointer to interface
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkPriv_BringUpInterface(ds_Net_INetworkPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkPriv)->BringUpInterface(_pif);
}

/** @memberof ds_Net_INetworkPriv
  * 
  * This function performs a route lookup and binds the INetworkPriv object to an underlying interface object.
  * Route lookup is required if an application wants the INetworkPriv object to be correctly bound to the appropriate 
  * underlying interface after it has changed the policy for the INetworkPriv object.
  * @param _pif Pointer to interface
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkPriv_LookupInterface(ds_Net_INetworkPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkPriv)->LookupInterface(_pif);
}

/** @memberof ds_Net_INetworkPriv
  * 
  * This function returns if laptop call active. "Laptop call" describes 
  * the use case of mobile device being used as a modem to connect
  * to the network. In that use case a computer connects to the mobile
  * device, for example, via USB.
  * This API can be used by privileged network applications that can 
  * piggyback a laptop connection rather than bring up an embedded data call. 
  * Since ACTIVE network objects automatically bring up an embedded data call, 
  * there is no point of using this API on ACTIVE mode network objects. 
  * An application can use a MONITORED mode network object to find out 
  * if laptop call is active and, if not, create an ACTIVE mode network object 
  * in order to bring up an embedded data call and use it.
  * @param _pif Pointer to interface
  * @param isActive Argument to hold if laptop call is active.
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkPriv_IsLaptopCallActive(ds_Net_INetworkPriv* _pif, boolean* isActive)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkPriv)->IsLaptopCallActive(_pif, isActive);
}

/** @memberof ds_Net_INetworkPriv
  * 
  * This function is used to get iface ids of all the ifaces available
  * in the system.
  * @param _pif Pointer to interface
  * @param allIfaces List of all the iface ids
  * @param allIfacesLen Length of sequence
  * @param allIfacesLenReq Required length of sequence
  * @retval AEE_SUCCESS Request received successfully.    
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkPriv_GetAllIfaces(ds_Net_INetworkPriv* _pif, ds_Net_IfaceIdType* allIfaces, int allIfacesLen, int* allIfacesLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkPriv)->GetAllIfaces(_pif, allIfaces, allIfacesLen, allIfacesLenReq);
}

/** @memberof ds_Net_INetworkPriv
  * 
  * This function is used to get the previous network state.
  * @param _pif Pointer to interface
  * @param netState Holds the previous network state.
  * @see ds_Net_Network_StateType.
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkPriv_GetPreviousState(ds_Net_INetworkPriv* _pif, ds_Net_NetworkStateType* netState)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkPriv)->GetPreviousState(_pif, netState);
}

/** @memberof ds_Net_INetworkPriv
  * 
  * This function is used to return the previous IP address 
  * of the interface.
  * @param _pif Pointer to interface
  * @param address Buffer that holds the IP address.
  * @see ds_IPAddrType.
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkPriv_GetPreviousIPAddr(ds_Net_INetworkPriv* _pif, ds_IPAddrType* address)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkPriv)->GetPreviousIPAddr(_pif, address);
}

/** @memberof ds_Net_INetworkPriv
  * 
  * This function returns the bearer technology information.
  * Note: The Bearer Rate information in this case is not applicable through the
  * bearerTech.
  * @param _pif Pointer to interface
  * @param bearerTech Object that will hold the bearer information.
  * @see IBearerInfo
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkPriv_GetPreviousBearerInfo(ds_Net_INetworkPriv* _pif, ds_Net_IBearerInfo** bearerTech)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkPriv)->GetPreviousBearerInfo(_pif, bearerTech);
}

/** @memberof ds_Net_INetworkPriv
  * 
  * This function can be used to fetch infoCode regarding QOS_AWARE or
  * QOS_UNAWARE event.
  * @param _pif Pointer to interface
  * @param infoCode Infocode that was cached from QOS_AWARE/QOS_UNAWARE 
  * event
  * See ds_Net_QoS_InfoCodeType.
  * @retval AEE_SUCCESS The current info code was successfully fetched
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkPriv_GetQoSAwareInfoCode(ds_Net_INetworkPriv* _pif, ds_Net_QoSInfoCodeType* infoCode)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkPriv)->GetQoSAwareInfoCode(_pif, infoCode);
}

/** @memberof ds_Net_INetworkPriv
  * 
  * This function gets the policy associated with the network.
  * This function is internal and is to be used by the sockets library
  * only. 
  * @param _pif Pointer to interface
  * @param policy - Policy object.
  * See INetPolicy.
  * @retval AEE_SUCCESS The current info code was successfully fetched
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkPriv_GetPolicy(ds_Net_INetworkPriv* _pif, ds_Net_IPolicy** policy)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkPriv)->GetPolicy(_pif, policy);
}

/** @memberof ds_Net_INetworkPriv
  * 
  * This function sets the policy associated with the network.
  * This function is internal and is to be used by the sockets library
  * only. 
  * @param _pif Pointer to interface
  * @param policy - Policy object.
  * See INetPolicy.
  * @retval AEE_SUCCESS The current info code was successfully fetched
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkPriv_SetPolicy(ds_Net_INetworkPriv* _pif, ds_Net_IPolicy* policy)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkPriv)->SetPolicy(_pif, policy);
}

/** @memberof ds_Net_INetworkPriv
  * 
  * This function used to enable/disable DNS negotiation during
  * ipcp configuration.
  * @param _pif Pointer to interface
  * @param enable Spicify if to enable / disable.
  * @retval ds_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkPriv_EnableDNSDuringIPCP(ds_Net_INetworkPriv* _pif, boolean enable)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkPriv)->EnableDNSDuringIPCP(_pif, enable);
}

/** @memberof ds_Net_INetworkPriv
  * 
  * This function used to get the devices information which are 
  * connected to DHCP at any given point of time.
  * @param _pif Pointer to interface
  * @param connDevInfo Contains the Connected devices information.
  * @retval DS_SUCCESS Request received successfully.
  * @retval Other DS designated error codes might be returned.
  * @see ds_Errors.idl.
  */
static __inline AEEResult ds_Net_INetworkPriv_GetDhcpDeviceInfo(ds_Net_INetworkPriv* _pif, ds_Net_DhcpGetDeviceInfoType* connDevInfo)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkPriv)->GetDhcpDeviceInfo(_pif, connDevInfo);
}

/** @memberof ds_Net_INetworkPriv
  * 
  * This function is used to to update device ARP cache with DHCP 
  * client's IP/MAC as per the lease offered by the server. This is 
  * used typically for cases where mode handler uses LAN LLC as 
  * MAC level protocol. Here when DHCP server assigns a lease to a 
  * client, it replies with a unicast IP packet to that client. 
  * This triggers LAN LLC to generate unicast ethernet type frames
  * for which the destination MAC needs to be resolved and thus, 
  * triggers ARP requests for the offered unicast address.Since the 
  * DHCP client would not have yet received the offer, it wouldn't 
  * reply to the ARP request by device and DHCP handshake would not
  * complete.If User specifies the callback, DHCP server updates the 
  * device ARP cache before sending unicast OFFERs/ACKs. 
  * Also while free-ing the lease, server would clear the same update 
  * from ARP cache.
  * @param _pif Pointer to interface
  * @param dhcpArpCacheUpdateParam Contains the Entry information 
  * to be updated.
  * @retval DS_SUCCESS Request received successfully.
  * @retval Other DS designated error codes might be returned.
  * @see ds_Errors.idl.
  */
static __inline AEEResult ds_Net_INetworkPriv_DhcpArpCacheUpdate(ds_Net_INetworkPriv* _pif, const ds_Net_DhcpArpCacheUpdateType* dhcpArpCacheUpdateParam)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkPriv)->DhcpArpCacheUpdate(_pif, dhcpArpCacheUpdateParam);
}

/** @memberof ds_Net_INetworkPriv
  * 
  * This function is used to to clear device ARP cache with DHCP 
  * client's IP/MAC as per the lease offered by the server. This is 
  * used typically for cases where mode handler uses LAN LLC as 
  * MAC level protocol. Here when DHCP server assigns a lease to a 
  * client, it replies with a unicast IP packet to that client. 
  * This triggers LAN LLC to generate unicast ethernet type frames
  * for which the destination MAC needs to be resolved and thus, 
  * triggers ARP requests for the offered unicast address.Since the 
  * DHCP client would not have yet received the offer, it wouldn't 
  * reply to the ARP request by device and DHCP handshake would not
  * complete.If User specifies the callback, DHCP server updates the 
  * device ARP cache before sending unicast OFFERs/ACKs. 
  * Also while free-ing the lease, server would clear the same update 
  * from ARP cache.
  * @param _pif Pointer to interface
  * @param dhcpArpCacheClearParam Contains the Entry information 
  * to be cleared.
  * @retval DS_SUCCESS Request received successfully.
  * @retval Other DS designated error codes might be returned.
  * @see ds_Errors.idl.
  */
static __inline AEEResult ds_Net_INetworkPriv_DhcpArpCacheClear(ds_Net_INetworkPriv* _pif, const ds_Net_DhcpArpCacheClearType* dhcpArpCacheClearParam)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkPriv)->DhcpArpCacheClear(_pif, dhcpArpCacheClearParam);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Net_DownReasons_Def.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_INetwork.h"
#include "ds_Net_IPolicy.h"
#include "ds_Net_IPolicyPriv.h"
#include "ds_Net_QoS_Def.h"
#include "ds_Net_IBearerInfo.h"
namespace ds
{
   namespace Net
   {
      struct _SeqIfaceIdType__seq_unsignedLong {
         IfaceIdType* data;
         int dataLen;
         int dataLenReq;
      };
      typedef _SeqIfaceIdType__seq_unsignedLong SeqIfaceIdType;
      typedef int IfaceNameTypePriv;
      namespace IfaceName
      {
         
         /**
           * IfaceNameTypePriv is part of the enumeration in IfaceNameType.
           * MBMS iface is moved here until MBMS feature is released publicly.
           */
         const ::ds::Net::IfaceNameTypePriv IFACE_MBMS = 0x8801;
      };
      namespace NetworkEvent
      {
         
         /**
           * IDLE event is fired when number of sockets using underlying 
           * network interface is dropped to zero.
           * Registration for this event can be performed 
           * after creation of Network object.
           */
         const ::ds::Net::EventType QDS_EV_IDLE = 0x109d887;
      };
      struct Ipv4Addr {
         unsigned int ps_s_addr;
      };
      namespace ChaddrLen
      {
         const int MAX_CHADDR_LEN = 16;
      };
      namespace ChostnameLen
      {
         const int MAX_CHOSTNAME_LEN = 32;
      };
      struct DhcpServerConnDevicesInfo {
         Ipv4Addr client_ip;
         unsigned char client_hw[16];
         unsigned int client_hw_len;
         unsigned char client_hostname[32];
         unsigned int client_hostname_len;
         unsigned int client_connected_time;
      };
      struct _DhcpGetDeviceInfoListType__seq_DhcpServerConnDevicesInfo_Net_ds {
         DhcpServerConnDevicesInfo* data;
         int dataLen;
         int dataLenReq;
      };
      typedef _DhcpGetDeviceInfoListType__seq_DhcpServerConnDevicesInfo_Net_ds DhcpGetDeviceInfoListType;
      struct DhcpGetDeviceInfoType {
         DhcpServerConnDevicesInfo* dev_info;
         int dev_infoLen;
         int dev_infoLenReq;
      };
      struct DhcpArpCacheType {
         unsigned int ip_addr;
         ::byte hw_addr[16];
         int hw_addr_len;
      };
      typedef DhcpArpCacheType DhcpArpCacheUpdateType;
      typedef DhcpArpCacheType DhcpArpCacheClearType;
      const ::AEEIID AEEIID_INetworkPriv = 0x1072cf1;
      
      /** @interface INetworkPriv
        * 
        * ds Net Network Privileged interface.
        * This interface is intended for internal usage for backward compatibility purposes.
        * It provides additional functionalities to INetwork.
        * It exhibits different behavior than INetwork, specifically - the Data Network is not brought up
        * automatically when INetworkPriv is instantiated.
        */
      struct INetworkPriv : public ::ds::Net::IEventManager
      {
         
         /**
           * This function explicitly brings up the network.        
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL BringUpInterface() = 0;
         
         /**
           * This function performs a route lookup and binds the INetworkPriv object to an underlying interface object.
           * Route lookup is required if an application wants the INetworkPriv object to be correctly bound to the appropriate 
           * underlying interface after it has changed the policy for the INetworkPriv object.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL LookupInterface() = 0;
         
         /**
           * This function returns if laptop call active. "Laptop call" describes 
           * the use case of mobile device being used as a modem to connect
           * to the network. In that use case a computer connects to the mobile
           * device, for example, via USB.
           * This API can be used by privileged network applications that can 
           * piggyback a laptop connection rather than bring up an embedded data call. 
           * Since ACTIVE network objects automatically bring up an embedded data call, 
           * there is no point of using this API on ACTIVE mode network objects. 
           * An application can use a MONITORED mode network object to find out 
           * if laptop call is active and, if not, create an ACTIVE mode network object 
           * in order to bring up an embedded data call and use it.
           * @param isActive Argument to hold if laptop call is active.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL IsLaptopCallActive(boolean* isActive) = 0;
         
         /**
           * This function is used to get iface ids of all the ifaces available
           * in the system.
           * @param allIfaces List of all the iface ids
           * @retval AEE_SUCCESS Request received successfully.    
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetAllIfaces(::ds::Net::IfaceIdType* allIfaces, int allIfacesLen, int* allIfacesLenReq) = 0;
         
         /**
           * This function is used to get the previous network state.
           * @param netState Holds the previous network state.
           * @see ds::Net::Network::StateType.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetPreviousState(::ds::Net::NetworkStateType* netState) = 0;
         
         /**
           * This function is used to return the previous IP address 
           * of the interface.
           * @param address Buffer that holds the IP address.
           * @see ds::IPAddrType.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetPreviousIPAddr(::ds::IPAddrType* address) = 0;
         
         /**
           * This function returns the bearer technology information.
           * Note: The Bearer Rate information in this case is not applicable through the
           * bearerTech.
           * @param bearerTech Object that will hold the bearer information.
           * @see IBearerInfo
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetPreviousBearerInfo(::ds::Net::IBearerInfo** bearerTech) = 0;
         
         /**
           * This function can be used to fetch infoCode regarding QOS_AWARE or
           * QOS_UNAWARE event.
           * @param infoCode Infocode that was cached from QOS_AWARE/QOS_UNAWARE 
           * event
           * See ds::Net::QoS::InfoCodeType.
           * @retval AEE_SUCCESS The current info code was successfully fetched
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetQoSAwareInfoCode(::ds::Net::QoSInfoCodeType* infoCode) = 0;
         
         /**
           * This function gets the policy associated with the network.
           * This function is internal and is to be used by the sockets library
           * only. 
           * @param policy - Policy object.
           * See INetPolicy.
           * @retval AEE_SUCCESS The current info code was successfully fetched
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetPolicy(::ds::Net::IPolicy** policy) = 0;
         
         /**
           * This function sets the policy associated with the network.
           * This function is internal and is to be used by the sockets library
           * only. 
           * @param policy - Policy object.
           * See INetPolicy.
           * @retval AEE_SUCCESS The current info code was successfully fetched
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetPolicy(::ds::Net::IPolicy* policy) = 0;
         
         /**
           * This function used to enable/disable DNS negotiation during
           * ipcp configuration.
           * @param enable Spicify if to enable / disable.
           * @retval ds::SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL EnableDNSDuringIPCP(boolean enable) = 0;
         
         /**
           * This function used to get the devices information which are 
           * connected to DHCP at any given point of time.
           * @param connDevInfo Contains the Connected devices information.
           * @retval DS::SUCCESS Request received successfully.
           * @retval Other DS designated error codes might be returned.
           * @see ds_Errors.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetDhcpDeviceInfo(::ds::Net::DhcpGetDeviceInfoType* connDevInfo) = 0;
         
         /**
           * This function is used to to update device ARP cache with DHCP 
           * client's IP/MAC as per the lease offered by the server. This is 
           * used typically for cases where mode handler uses LAN LLC as 
           * MAC level protocol. Here when DHCP server assigns a lease to a 
           * client, it replies with a unicast IP packet to that client. 
           * This triggers LAN LLC to generate unicast ethernet type frames
           * for which the destination MAC needs to be resolved and thus, 
           * triggers ARP requests for the offered unicast address.Since the 
           * DHCP client would not have yet received the offer, it wouldn't 
           * reply to the ARP request by device and DHCP handshake would not
           * complete.If User specifies the callback, DHCP server updates the 
           * device ARP cache before sending unicast OFFERs/ACKs. 
           * Also while free-ing the lease, server would clear the same update 
           * from ARP cache.
           * @param dhcpArpCacheUpdateParam Contains the Entry information 
           * to be updated.
           * @retval DS::SUCCESS Request received successfully.
           * @retval Other DS designated error codes might be returned.
           * @see ds_Errors.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL DhcpArpCacheUpdate(const ::ds::Net::DhcpArpCacheUpdateType* dhcpArpCacheUpdateParam) = 0;
         
         /**
           * This function is used to to clear device ARP cache with DHCP 
           * client's IP/MAC as per the lease offered by the server. This is 
           * used typically for cases where mode handler uses LAN LLC as 
           * MAC level protocol. Here when DHCP server assigns a lease to a 
           * client, it replies with a unicast IP packet to that client. 
           * This triggers LAN LLC to generate unicast ethernet type frames
           * for which the destination MAC needs to be resolved and thus, 
           * triggers ARP requests for the offered unicast address.Since the 
           * DHCP client would not have yet received the offer, it wouldn't 
           * reply to the ARP request by device and DHCP handshake would not
           * complete.If User specifies the callback, DHCP server updates the 
           * device ARP cache before sending unicast OFFERs/ACKs. 
           * Also while free-ing the lease, server would clear the same update 
           * from ARP cache.
           * @param dhcpArpCacheClearParam Contains the Entry information 
           * to be cleared.
           * @retval DS::SUCCESS Request received successfully.
           * @retval Other DS designated error codes might be returned.
           * @see ds_Errors.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL DhcpArpCacheClear(const ::ds::Net::DhcpArpCacheClearType* dhcpArpCacheClearParam) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_INETWORKPRIV_H
