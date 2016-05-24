#ifndef DS_NET_INETWORK_H
#define DS_NET_INETWORK_H

/*============================================================================
  Copyright (c) 2008-2012 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Net_DownReasons_Def.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IPhysLink.h"

/** @memberof ds_Net_NetworkEvent
  * 
  * Data network state change
  */
#define ds_Net_NetworkEvent_QDS_EV_STATE_CHANGED 0x106e60f

/** @memberof ds_Net_NetworkEvent
  * 
  * IP address change
  */
#define ds_Net_NetworkEvent_QDS_EV_IP_ADDR_CHANGED 0x106e610

/** @memberof ds_Net_NetworkEvent
  * 
  * Outage (mobile handoff) event
  */
#define ds_Net_NetworkEvent_QDS_EV_OUTAGE 0x106e614

/** @memberof ds_Net_NetworkEvent
  * 
  * // RF Condition event
  */
#define ds_Net_NetworkEvent_QDS_EV_RF_CONDITIONS_CHANGED 0x106e617
typedef int ds_Net_NetworkStateType;

/** @memberof ds_Net_NetworkState
  * 
  * CLOSED: When an ACTIVE MODE Network object reaches this state it
  *         should be released. 
  *         MONITORED MODE Network object shall be in this state when
  *         the network connection it is monitoring is down.
  *         It is valid for MONITORED MODE Network object to remain in
  *         this state (as the application may wish to continue
  *         monitoring the network interface).
  *         Most operations invoked while the Network object is in this
  *         state will fail.
  *         @See INetwork_Stop
  */
#define ds_Net_NetworkState_QDS_CLOSED 0x2

/** @memberof ds_Net_NetworkState
  * 
  * OPEN_IN_PROGRESS: An ACTIVE MODE Network object may be in this
  *                   state upon its creation.
  *                   MONITORED MODE Network object shall be in this
  *                   state when the network connection it is monitoring 
  *                   is in the process of being established.          
  *                   Most operations invoked while the Network object
  *                   is in this state will fail.
  */
#define ds_Net_NetworkState_QDS_OPEN_IN_PROGRESS 0x4

/** @memberof ds_Net_NetworkState
  * 
  * OPEN: This is the normal state of an ACTIVE MODE Network object.
  *       MONITORED MODE Network object shall be in this state when
  *       the network connection it is monitoring is established.          
  *       @See INetworkControl_GoDormant
  */
#define ds_Net_NetworkState_QDS_OPEN 0x20

/** @memberof ds_Net_NetworkState
  * 
  * CLOSE_IN_PROGRESS: An ACTIVE MODE Network object may be in this
  *                    state after Stop was called.
  *                    A MONITORED MODE Network object may be in this
  *                    state after GoNull was called, or, in general,
  *                    when the network connection it is monitoring
  *                    is in the process of being torn down.
  *                    Most operations invoked while the Network object
  *                    is in this state will fail.
  */
#define ds_Net_NetworkState_QDS_CLOSE_IN_PROGRESS 0x40

/** @memberof ds_Net_NetworkState
  * 
  * LINGERING: The underlying network interface is not used anymore by
  *            any client, but it stays in OPEN state for a while to
  *            optimize a scenario where a client starts using it again
  *            closely after the last client stopped using it.
  *            This state is applicable only for MONITORED MODE Network
  *            objects.
  */
#define ds_Net_NetworkState_QDS_LINGERING 0x80
typedef int ds_Net_RFConditionType;
#define ds_Net_RFCondition_QDS_BAD 1
#define ds_Net_RFCondition_QDS_GOOD 2
typedef unsigned char ds_Net_DomainName[256];
typedef int ds_Net_OutageStateType;

/** @memberof ds_Net_OutageState
  * 
  * The state of network outage cannot be determined and information
  * regarding expected outage cannot be provided.
  * The timeToOutage provided is not valid and should be ignored.
  */
#define ds_Net_OutageState_QDS_INVALID 0

/** @memberof ds_Net_OutageState
  * 
  * The OutageInfo provided is valid.
  */
#define ds_Net_OutageState_QDS_VALID 1

/** @memberof ds_Net_OutageState
  * 
  * By the time the application has queried for the network outage
  * information, the outage has actually already started.
  * i.e. The device is currently in network outage state.
  */
#define ds_Net_OutageState_QDS_STARTED 2

/** @memberof ds_Net_OutageState
  * 
  * By the time the application has queried for the network outage
  * information, the outage has already occurred and finished.
  * The device is not in network outage state anymore
  */
#define ds_Net_OutageState_QDS_EXPIRED 3
struct ds_Net_OutageInfoType {
   ds_Net_OutageStateType state;
   int timeToOutage;
   int duration;
};
typedef struct ds_Net_OutageInfoType ds_Net_OutageInfoType;
#define ds_Net_HWAddress_QDS_MAX_HW_ADDR 6
struct ds_Net_HWAddressType {
   unsigned char len;
   unsigned char hwAddr[6];
};
typedef struct ds_Net_HWAddressType ds_Net_HWAddressType;
struct ds_Net_NetworkStatsType {
   uint64 bytesRX;
   uint64 bytesTX;
   uint64 totalBytesTransferredSincePowerUp;
   uint64 bytesTransferred;
   unsigned int pktsRX;
   unsigned int mcastPktsRX;
   unsigned int pktsDroppedRX;
   unsigned int pktsTX;
   unsigned int mcastPktsTX;
   unsigned int pktsDroppedTX;
   unsigned int totalOpenTimeSincePowerUp;
   unsigned int totalActiveTimeSincePowerUp;
   unsigned int lastUpStateTimeStamp;
   unsigned int firstIOTimeStamp;
   unsigned short state;
   AEEINTERFACE_PADMEMBERS(__pad, 6)
};
typedef struct ds_Net_NetworkStatsType ds_Net_NetworkStatsType;

/**
  * Number of bytes received on this iface since the last reset. 
  */
/**
  * Number of bytes sent via this iface since the last reset. 
  */
/**
  * Total bytes transferred via this iface (since last reset upto now). 
  */
/**
  * Number of bytes transferred via this iface during current connection. 
  */
/**
  * Number of packets received on this iface since the last reset. 
  */
/**
  * Number of multicast packets received on this iface since the last reset. 
  */
/**
  * Number of dropped packets received on this iface since the last reset. 
  */
/**
  * Number of packets sent via this iface since the last reset. 
  */
/**
  * Number of multicast packets sent via this iface since the last reset. 
  */
/**
  * Number of dropped packets sent via this iface since the last reset. 
  */
/**
  * Time in seconds for this iface been open (since last reset upto now). 
  */
/**
  * Time in seconds for this iface been active (between first IO after last reset upto last IO).
  */
/**
  * Time in seconds since the last IO on this iface. 
  */
/**
  * Time in seconds since last time this iface was brought up. 
  */
/** @memberof ds_Net
  * 
  * State of this iface since the last reset. 
  */
#define ds_Net_AEEIID_INetwork 0x106c546
struct ds_Net_INetwork__SeqIPAddrType__seq_IPAddrType_ds {
   ds_IPAddrType* data;
   int dataLen;
   int dataLenReq;
};
typedef struct ds_Net_INetwork__SeqIPAddrType__seq_IPAddrType_ds ds_Net_INetwork__SeqIPAddrType__seq_IPAddrType_ds;
typedef ds_Net_INetwork__SeqIPAddrType__seq_IPAddrType_ds ds_Net_INetwork_SeqIPAddrType;
struct ds_Net_INetwork__SeqDomainNameType__seq_octet_Net_ds {
   ds_Net_DomainName* data;
   int dataLen;
   int dataLenReq;
};
typedef struct ds_Net_INetwork__SeqDomainNameType__seq_octet_Net_ds ds_Net_INetwork__SeqDomainNameType__seq_octet_Net_ds;
typedef ds_Net_INetwork__SeqDomainNameType__seq_octet_Net_ds ds_Net_INetwork_SeqDomainNameType;

/** @interface ds_Net_INetwork
  * 
  * ds Network interface.
  * This is the main interface for accessing information of a Data
  * Connection and controlling it.
  * A instance of this interface can only be created via INetworkFactory.
  * If created in ACTIVE mode, the network is brought up upon creation
  * (@See CreateNetwork). The ACTIVE object is not reusable once the
  * network has been shut down.
  * For creating QoS and MCast objects @See interface
  * ds_Net_INetworkExt.
  *
  * Events that can be registered on this interface via OnStateChange
  * (as part of IEventManager interface): 
  * - ds_Net_NetworkEvent_STATE_CHANGED. Use GetState to fetch the
  *   state information.
  * - ds_Net_NetworkEvent_IP_ADDR_CHANGED. Use GetIPAddr to fetch the
  *   IP address.
  * - ds_Net_NetworkEvent_OUTAGE. Use GetOutageInfo to fetch Outage
  *   information.
  * - ds_Net_NetworkEvent_RF_CONDITIONS_CHANGED. Use GetCurrRFCondition
  *   to fetch current RF conditions.
  */
#define INHERIT_ds_Net_INetwork(iname) \
   INHERIT_ds_Net_IEventManager(iname); \
   AEEResult (*GetState)(iname* _pif, ds_Net_NetworkStateType* value); \
   AEEResult (*GetIfaceName)(iname* _pif, ds_Net_IfaceNameType* value); \
   AEEResult (*GetDeviceInfo)(iname* _pif, void* value); \
   AEEResult (*GetInterfaceGwAddr)(iname* _pif, void* value); \
   AEEResult (*GetIfaceId)(iname* _pif, ds_Net_IfaceIdType* value); \
   AEEResult (*GetLastNetDownReason)(iname* _pif, ds_Net_NetDownReasonType* value); \
   AEEResult (*GetIPAddr)(iname* _pif, ds_IPAddrType* value); \
   AEEResult (*GetDNSAddr)(iname* _pif, ds_IPAddrType* DNSAddresses, int DNSAddressesLen, int* DNSAddressesLenReq); \
   AEEResult (*GetSIPServerDomainNames)(iname* _pif, ds_Net_DomainName* sipServerDomainNames, int sipServerDomainNamesLen, int* sipServerDomainNamesLenReq); \
   AEEResult (*GetSIPServerAddr)(iname* _pif, ds_IPAddrType* sipServerAddresses, int sipServerAddressesLen, int* sipServerAddressesLenReq); \
   AEEResult (*GetNatPublicIPAddress)(iname* _pif, ds_IPAddrType* address); \
   AEEResult (*GetOutageInfo)(iname* _pif, ds_Net_OutageInfoType* value); \
   AEEResult (*GetAddressFamily)(iname* _pif, ds_AddrFamilyType* value); \
   AEEResult (*GetDomainNameSearchList)(iname* _pif, ds_Net_DomainName* domainNameSearchList, int domainNameSearchListLen, int* domainNameSearchListLenReq); \
   AEEResult (*GetNetMTU)(iname* _pif, int* value); \
   AEEResult (*GetHWAddress)(iname* _pif, ds_Net_HWAddressType* value); \
   AEEResult (*Stop)(iname* _pif); \
   AEEResult (*GetCurrRFCondition)(iname* _pif, ds_Net_RFConditionType* value); \
   AEEResult (*GetTXPhysLink)(iname* _pif, ds_Net_IPhysLink** value); \
   AEEResult (*GetRXPhysLink)(iname* _pif, ds_Net_IPhysLink** value); \
   AEEResult (*GetDormancyInfoCode)(iname* _pif, ds_Net_DormancyInfoCodeType* value); \
   AEEResult (*GetNetworkStatistics)(iname* _pif, ds_Net_NetworkStatsType* value); \
   AEEResult (*ResetNetworkStatistics)(iname* _pif); \
   AEEResult (*GetTechObject)(iname* _pif, AEEIID techObj_iid, void** techObj)
AEEINTERFACE_DEFINE(ds_Net_INetwork);

/** @memberof ds_Net_INetwork
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INetwork_AddRef(ds_Net_INetwork* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->AddRef(_pif);
}

/** @memberof ds_Net_INetwork
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INetwork_Release(ds_Net_INetwork* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->Release(_pif);
}

/** @memberof ds_Net_INetwork
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_INetwork_QueryInterface(ds_Net_INetwork* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_INetwork
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
static __inline AEEResult ds_Net_INetwork_OnStateChange(ds_Net_INetwork* _pif, ISignal* signal, ds_Net_EventType eventID, IQI** regObj)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->OnStateChange(_pif, signal, eventID, regObj);
}

/** @memberof ds_Net_INetwork
  * 
  * This attribute indicates the network state.
  * When an ACTIVE MODE Network object reach a CLOSED state it should
  * be released.
  * Operations invoked on such object in such state shall fail.
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @see ds_Net_NetworkStateType.
  */
static __inline AEEResult ds_Net_INetwork_GetState(ds_Net_INetwork* _pif, ds_Net_NetworkStateType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->GetState(_pif, value);
}

/** @memberof ds_Net_INetwork
  *
  * This attribute indicates the name of the iface
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @see ds_Network_IfaceName
  */
static __inline AEEResult ds_Net_INetwork_GetIfaceName(ds_Net_INetwork* _pif, ds_Net_IfaceNameType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->GetIfaceName(_pif, value);
}

/** @memberof ds_Net_INetwork
  *
  * This attribute indicates the network interface information
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @see ds_Network_DeviceInfo
  */
static __inline AEEResult ds_Net_INetwork_GetDeviceInfo(ds_Net_INetwork* _pif, void* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->GetDeviceInfo(_pif, value);
}

/** @memberof ds_Net_INetwork
  *
  * This attribute indicates the network interface information
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @see ds_Net_INetwork_GetInterfaceGwAddr
  */
static __inline AEEResult ds_Net_INetwork_GetInterfaceGwAddr(ds_Net_INetwork* _pif, void* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->GetInterfaceGwAddr(_pif, value);
}

/** @memberof ds_Net_INetwork
  * 
  * This attribute indicates an ID identifying the underlying interface
  * bound to this INetwork object.
  * Iface ID is relevant for socket options SO_TX_IFACE and IP_RECVIF
  * as well as for using iface id as scopeId in IPv6 addresses.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_INetwork_GetIfaceId(ds_Net_INetwork* _pif, ds_Net_IfaceIdType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->GetIfaceId(_pif, value);
}

/** @memberof ds_Net_INetwork
  * 
  * This attribute indicates the last network down reason.
  * This attribute has a valid value if the network went down at least
  * once.
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @see ds_Net_NetDownReasonType.
  */
static __inline AEEResult ds_Net_INetwork_GetLastNetDownReason(ds_Net_INetwork* _pif, ds_Net_NetDownReasonType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->GetLastNetDownReason(_pif, value);
}

/** @memberof ds_Net_INetwork
  * 
  * This attribute indicates the IP address of the local host or device,
  * in network byte order.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @see ds_IPAddrType.
  */
static __inline AEEResult ds_Net_INetwork_GetIPAddr(ds_Net_INetwork* _pif, ds_IPAddrType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->GetIPAddr(_pif, value);
}

/** @memberof ds_Net_INetwork
  * 
  * This function is used to return all the DNS server addresses for
  * the network interface associated with the network object.
  * If available, the Primary DNS address shall always be specified
  * in the returned sequence before the Secondary address.
  * If only a single address is specified in the returned sequence
  * it should be treated as the Primary address.
  * @param _pif Pointer to interface
  * @param addressList Buffer that holds the IP addresses.
  * @param DNSAddressesLen Length of sequence
  * @param DNSAddressesLenReq Required length of sequence
  * @see SeqIPAddrType.
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetwork_GetDNSAddr(ds_Net_INetwork* _pif, ds_IPAddrType* DNSAddresses, int DNSAddressesLen, int* DNSAddressesLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->GetDNSAddr(_pif, DNSAddresses, DNSAddressesLen, DNSAddressesLenReq);
}

/** @memberof ds_Net_INetwork
  * 
  * This function returns the SIP (Session Initiation Protocol) server
  * Domain Names for this network.
  * @param _pif Pointer to interface
  * @param sipServerDomainNames Buffer array that holds the SIP server
  * domain names.
  * @param sipServerDomainNamesLen Length of sequence
  * @param sipServerDomainNamesLenReq Required length of sequence
  * @see SeqDomainNameType.
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetwork_GetSIPServerDomainNames(ds_Net_INetwork* _pif, ds_Net_DomainName* sipServerDomainNames, int sipServerDomainNamesLen, int* sipServerDomainNamesLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->GetSIPServerDomainNames(_pif, sipServerDomainNames, sipServerDomainNamesLen, sipServerDomainNamesLenReq);
}

/** @memberof ds_Net_INetwork
  * 
  * This function returns the SIP (Session Initiation Protocol) server IP
  * addresses for this network.
  * @param _pif Pointer to interface
  * @param sipServerAddresses Buffer array that holds the SIP server
  * addresses.
  * @param sipServerAddressesLen Length of sequence
  * @param sipServerAddressesLenReq Required length of sequence
  * @see SeqIPAddrType.
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetwork_GetSIPServerAddr(ds_Net_INetwork* _pif, ds_IPAddrType* sipServerAddresses, int sipServerAddressesLen, int* sipServerAddressesLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->GetSIPServerAddr(_pif, sipServerAddresses, sipServerAddressesLen, sipServerAddressesLenReq);
}

/** @memberof ds_Net_INetwork
  * 
  * This function is used to return the public IP address of the NAT iface, in
  * network byte order.
  * @param _pif Pointer to interface
  * @param address Buffer that holds the IP address.
  * @see DS_IPAddrType.
  * @retval DS_SUCCESS Request received successfully.
  * @retval Other DS designated error codes might be returned.
  * @see DS_Errors.idl.
  */
static __inline AEEResult ds_Net_INetwork_GetNatPublicIPAddress(ds_Net_INetwork* _pif, ds_IPAddrType* address)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->GetNatPublicIPAddress(_pif, address);
}

/** @memberof ds_Net_INetwork
  * 
  * This attribute holds information of the latest outage event
  * associated with this data network.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @see ds_Network_Outage_Info
  */
static __inline AEEResult ds_Net_INetwork_GetOutageInfo(ds_Net_INetwork* _pif, ds_Net_OutageInfoType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->GetOutageInfo(_pif, value);
}

/** @memberof ds_Net_INetwork
  * 
  * This attribute indicates the IP Address family of this INetwork
  * object.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_INetwork_GetAddressFamily(ds_Net_INetwork* _pif, ds_AddrFamilyType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->GetAddressFamily(_pif, value);
}

/** @memberof ds_Net_INetwork
  * 
  * This function returns the Domain Name Search List for this network.
  * @param _pif Pointer to interface
  * @param domainNameSearchList Buffer array that holds the domain name
  * search list.
  * @param domainNameSearchListLen Length of sequence
  * @param domainNameSearchListLenReq Required length of sequence
  * @see SeqDomainNameType.
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetwork_GetDomainNameSearchList(ds_Net_INetwork* _pif, ds_Net_DomainName* domainNameSearchList, int domainNameSearchListLen, int* domainNameSearchListLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->GetDomainNameSearchList(_pif, domainNameSearchList, domainNameSearchListLen, domainNameSearchListLenReq);
}

/** @memberof ds_Net_INetwork
  * 
  * This attribute returns the IP Layer Maximum Transmission Unit
  * associated with this network.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_INetwork_GetNetMTU(ds_Net_INetwork* _pif, int* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->GetNetMTU(_pif, value);
}

/** @memberof ds_Net_INetwork
  * 
  * This attribute indicates the HW address associated with this network.
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @retval AEE_SUCCESS Request received successfully.
  * @retval QDS_EINVAL Iface does not support HW address.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Net_Network_HWAddressType.
  */
static __inline AEEResult ds_Net_INetwork_GetHWAddress(ds_Net_INetwork* _pif, ds_Net_HWAddressType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->GetHWAddress(_pif, value);
}

/** @memberof ds_Net_INetwork
  * 
  * This function is used to explicitly bring down the network
  * connection.
  * Note that the applicable Network interface shall be brought down as
  * well ONLY if no other entities are using it when this function is
  * called. Still, the application can expect STATE_CHANGED event if it
  * is registered. The application may hold the Network object after
  * calling this API, but it should release it once the Network moves 
  * to QDS_CLOSED state.          
  * 
  * @param _pif Pointer to interface
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetwork_Stop(ds_Net_INetwork* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->Stop(_pif);
}

/** @memberof ds_Net_INetwork
  * 
  * This attribute indicates the current RF condition.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @see ds_Net_Network_RFConditionType.
  */
static __inline AEEResult ds_Net_INetwork_GetCurrRFCondition(ds_Net_INetwork* _pif, ds_Net_RFConditionType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->GetCurrRFCondition(_pif, value);
}

/** @memberof ds_Net_INetwork
  * 
  * This attribute points the Tx physical link object.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_INetwork_GetTXPhysLink(ds_Net_INetwork* _pif, ds_Net_IPhysLink** value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->GetTXPhysLink(_pif, value);
}

/** @memberof ds_Net_INetwork
  * 
  * This attribute points the Rx physical link object.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_INetwork_GetRXPhysLink(ds_Net_INetwork* _pif, ds_Net_IPhysLink** value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->GetRXPhysLink(_pif, value);
}

/** @memberof ds_Net_INetwork
  * 
  * This attribute indicates the dormancy info code when default phys
  * link is dormant.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_INetwork_GetDormancyInfoCode(ds_Net_INetwork* _pif, ds_Net_DormancyInfoCodeType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->GetDormancyInfoCode(_pif, value);
}

/** @memberof ds_Net_INetwork
  * 
  * This attribute provides info on Network related statistics
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @param stats Output Network Statistics
  */
static __inline AEEResult ds_Net_INetwork_GetNetworkStatistics(ds_Net_INetwork* _pif, ds_Net_NetworkStatsType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->GetNetworkStatistics(_pif, value);
}

/** @memberof ds_Net_INetwork
  * 
  * This attribute resets Network related statistics
  * @param _pif Pointer to interface
  * @param None
  */
static __inline AEEResult ds_Net_INetwork_ResetNetworkStatistics(ds_Net_INetwork* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->ResetNetworkStatistics(_pif);
}

/** @memberof ds_Net_INetwork
  * 
  * This function allows object extensibility.
  * For supported interfaces, objects implementing those interfaces may be 
  * fetched via this function. The supported interfaces are documented in
  * the DS_NET_NetworkFactory.bid file. GetTechObject-supported interface
  * does not imply any similar contract in regard to QueryInterface for the
  * respective interface.
  * Unlike IQI, the availability of a specific interface depends on some 
  * factors, e.g. current network type. Moreover, there is no guaranty 
  * for transitivity or symmetry. 
  * Note: 'interface' parameter will map to iid and techObj.
  * @param _pif Pointer to interface
  * @param techObj_iid Interface ID of requested interface
  * @param iid The interface that should be retrieved.
  * @param techObj On success, will contain the requested interface instance.
  * @retval ds_SUCCESS Interface retrieved successfully.
  * @retval AEE_ECLASSNOTSUPPORT Specified interface unsupported.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetwork_GetTechObject(ds_Net_INetwork* _pif, AEEIID techObj_iid, void** techObj)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork)->GetTechObject(_pif, techObj_iid, techObj);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Net_DownReasons_Def.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IPhysLink.h"
namespace ds
{
   namespace Net
   {
      namespace NetworkEvent
      {
         
         /**
           * Data network state change
           */
         const ::ds::Net::EventType QDS_EV_STATE_CHANGED = 0x106e60f;
         
         /**
           * IP address change
           */
         const ::ds::Net::EventType QDS_EV_IP_ADDR_CHANGED = 0x106e610;
         
         /**
           * Outage (mobile handoff) event
           */
         const ::ds::Net::EventType QDS_EV_OUTAGE = 0x106e614;
         
         /**
           * // RF Condition event
           */
         const ::ds::Net::EventType QDS_EV_RF_CONDITIONS_CHANGED = 0x106e617;
      };
      typedef int NetworkStateType;
      namespace NetworkState
      {
         
         /**
           * CLOSED: When an ACTIVE MODE Network object reaches this state it
           *         should be released. 
           *         MONITORED MODE Network object shall be in this state when
           *         the network connection it is monitoring is down.
           *         It is valid for MONITORED MODE Network object to remain in
           *         this state (as the application may wish to continue
           *         monitoring the network interface).
           *         Most operations invoked while the Network object is in this
           *         state will fail.
           *         @See INetwork::Stop
           */
         const ::ds::Net::NetworkStateType QDS_CLOSED = 0x2;
         
         /**
           * OPEN_IN_PROGRESS: An ACTIVE MODE Network object may be in this
           *                   state upon its creation.
           *                   MONITORED MODE Network object shall be in this
           *                   state when the network connection it is monitoring 
           *                   is in the process of being established.          
           *                   Most operations invoked while the Network object
           *                   is in this state will fail.
           */
         const ::ds::Net::NetworkStateType QDS_OPEN_IN_PROGRESS = 0x4;
         
         /**
           * OPEN: This is the normal state of an ACTIVE MODE Network object.
           *       MONITORED MODE Network object shall be in this state when
           *       the network connection it is monitoring is established.          
           *       @See INetworkControl::GoDormant
           */
         const ::ds::Net::NetworkStateType QDS_OPEN = 0x20;
         
         /**
           * CLOSE_IN_PROGRESS: An ACTIVE MODE Network object may be in this
           *                    state after Stop was called.
           *                    A MONITORED MODE Network object may be in this
           *                    state after GoNull was called, or, in general,
           *                    when the network connection it is monitoring
           *                    is in the process of being torn down.
           *                    Most operations invoked while the Network object
           *                    is in this state will fail.
           */
         const ::ds::Net::NetworkStateType QDS_CLOSE_IN_PROGRESS = 0x40;
         
         /**
           * LINGERING: The underlying network interface is not used anymore by
           *            any client, but it stays in OPEN state for a while to
           *            optimize a scenario where a client starts using it again
           *            closely after the last client stopped using it.
           *            This state is applicable only for MONITORED MODE Network
           *            objects.
           */
         const ::ds::Net::NetworkStateType QDS_LINGERING = 0x80;
      };
      typedef int RFConditionType;
      namespace RFCondition
      {
         const ::ds::Net::RFConditionType QDS_BAD = 1;
         const ::ds::Net::RFConditionType QDS_GOOD = 2;
      };
      typedef unsigned char DomainName[256];
      typedef int OutageStateType;
      namespace OutageState
      {
         
         /**
           * The state of network outage cannot be determined and information
           * regarding expected outage cannot be provided.
           * The timeToOutage provided is not valid and should be ignored.
           */
         const ::ds::Net::OutageStateType QDS_INVALID = 0;
         
         /**
           * The OutageInfo provided is valid.
           */
         const ::ds::Net::OutageStateType QDS_VALID = 1;
         
         /**
           * By the time the application has queried for the network outage
           * information, the outage has actually already started.
           * i.e. The device is currently in network outage state.
           */
         const ::ds::Net::OutageStateType QDS_STARTED = 2;
         
         /**
           * By the time the application has queried for the network outage
           * information, the outage has already occurred and finished.
           * The device is not in network outage state anymore
           */
         const ::ds::Net::OutageStateType QDS_EXPIRED = 3;
      };
      struct OutageInfoType {
         OutageStateType state;
         int timeToOutage;
         int duration;
      };
      namespace HWAddress
      {
         const int QDS_MAX_HW_ADDR = 6;
      };
      struct HWAddressType {
         unsigned char len;
         unsigned char hwAddr[6];
      };
      struct NetworkStatsType {
         uint64 bytesRX;
         uint64 bytesTX;
         uint64 totalBytesTransferredSincePowerUp;
         uint64 bytesTransferred;
         unsigned int pktsRX;
         unsigned int mcastPktsRX;
         unsigned int pktsDroppedRX;
         unsigned int pktsTX;
         unsigned int mcastPktsTX;
         unsigned int pktsDroppedTX;
         unsigned int totalOpenTimeSincePowerUp;
         unsigned int totalActiveTimeSincePowerUp;
         unsigned int lastUpStateTimeStamp;
         unsigned int firstIOTimeStamp;
         unsigned short state;
         AEEINTERFACE_PADMEMBERS(__pad, 6)
      };
      
      /**
        * Number of bytes received on this iface since the last reset. 
        */
      /**
        * Number of bytes sent via this iface since the last reset. 
        */
      /**
        * Total bytes transferred via this iface (since last reset upto now). 
        */
      /**
        * Number of bytes transferred via this iface during current connection. 
        */
      /**
        * Number of packets received on this iface since the last reset. 
        */
      /**
        * Number of multicast packets received on this iface since the last reset. 
        */
      /**
        * Number of dropped packets received on this iface since the last reset. 
        */
      /**
        * Number of packets sent via this iface since the last reset. 
        */
      /**
        * Number of multicast packets sent via this iface since the last reset. 
        */
      /**
        * Number of dropped packets sent via this iface since the last reset. 
        */
      /**
        * Time in seconds for this iface been open (since last reset upto now). 
        */
      /**
        * Time in seconds for this iface been active (between first IO after last reset upto last IO).
        */
      /**
        * Time in seconds since the last IO on this iface. 
        */
      /**
        * Time in seconds since last time this iface was brought up. 
        */
      /**
        * State of this iface since the last reset. 
        */
      const ::AEEIID AEEIID_INetwork = 0x106c546;
      
      /** @interface INetwork
        * 
        * ds Network interface.
        * This is the main interface for accessing information of a Data
        * Connection and controlling it.
        * A instance of this interface can only be created via INetworkFactory.
        * If created in ACTIVE mode, the network is brought up upon creation
        * (@See CreateNetwork). The ACTIVE object is not reusable once the
        * network has been shut down.
        * For creating QoS and MCast objects @See interface
        * ds_Net_INetworkExt.
        *
        * Events that can be registered on this interface via OnStateChange
        * (as part of IEventManager interface): 
        * - ds::Net::NetworkEvent::STATE_CHANGED. Use GetState to fetch the
        *   state information.
        * - ds::Net::NetworkEvent::IP_ADDR_CHANGED. Use GetIPAddr to fetch the
        *   IP address.
        * - ds::Net::NetworkEvent::OUTAGE. Use GetOutageInfo to fetch Outage
        *   information.
        * - ds::Net::NetworkEvent::RF_CONDITIONS_CHANGED. Use GetCurrRFCondition
        *   to fetch current RF conditions.
        */
      struct INetwork : public ::ds::Net::IEventManager
      {
         struct _SeqIPAddrType__seq_IPAddrType_ds {
            ::ds::IPAddrType* data;
            int dataLen;
            int dataLenReq;
         };
         typedef _SeqIPAddrType__seq_IPAddrType_ds SeqIPAddrType;
         struct _SeqDomainNameType__seq_octet_Net_ds {
            ::ds::Net::DomainName* data;
            int dataLen;
            int dataLenReq;
         };
         typedef _SeqDomainNameType__seq_octet_Net_ds SeqDomainNameType;
         
         /**
           * This attribute indicates the network state.
           * When an ACTIVE MODE Network object reach a CLOSED state it should
           * be released.
           * Operations invoked on such object in such state shall fail.
           * @param value Attribute value
           * @see ds::Net::NetworkStateType.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetState(::ds::Net::NetworkStateType* value) = 0;
         
         /**
           * This attribute indicates the name of the iface.          
           * @param value Attribute value
           * @see ds::Network::IfaceName
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetIfaceName(::ds::Net::IfaceNameType* value) = 0;

         /**
           * This attribute indicates informtion of the network interface
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetDeviceInfo(void* value) = 0;

         /**
           * This attribute indicates informtion of the network interface 
           * gateway address 
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetInterfaceGwAddr(void* value) = 0;

         /**
           * This attribute indicates an ID identifying the underlying interface
           * bound to this INetwork object.
           * Iface ID is relevant for socket options SO_TX_IFACE and IP_RECVIF
           * as well as for using iface id as scopeId in IPv6 addresses.          
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetIfaceId(::ds::Net::IfaceIdType* value) = 0;
         
         /**
           * This attribute indicates the last network down reason.
           * This attribute has a valid value if the network went down at least
           * once.
           * @param value Attribute value
           * @see ds::Net::NetDownReasonType.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetLastNetDownReason(::ds::Net::NetDownReasonType* value) = 0;
         
         /**
           * This attribute indicates the IP address of the local host or device,
           * in network byte order.          
           * @param value Attribute value
           * @see ds::IPAddrType.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetIPAddr(::ds::IPAddrType* value) = 0;
         
         /**
           * This function is used to return all the DNS server addresses for
           * the network interface associated with the network object.
           * If available, the Primary DNS address shall always be specified
           * in the returned sequence before the Secondary address.
           * If only a single address is specified in the returned sequence
           * it should be treated as the Primary address.
           * @param addressList Buffer that holds the IP addresses.
           * @see SeqIPAddrType.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetDNSAddr(::ds::IPAddrType* DNSAddresses, int DNSAddressesLen, int* DNSAddressesLenReq) = 0;
         
         /**
           * This function returns the SIP (Session Initiation Protocol) server
           * Domain Names for this network.
           * @param sipServerDomainNames Buffer array that holds the SIP server
           * domain names.
           * @see SeqDomainNameType.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetSIPServerDomainNames(::ds::Net::DomainName* sipServerDomainNames, int sipServerDomainNamesLen, int* sipServerDomainNamesLenReq) = 0;
         
         /**
           * This function returns the SIP (Session Initiation Protocol) server IP
           * addresses for this network.
           * @param sipServerAddresses Buffer array that holds the SIP server
           * addresses.
           * @see SeqIPAddrType.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetSIPServerAddr(::ds::IPAddrType* sipServerAddresses, int sipServerAddressesLen, int* sipServerAddressesLenReq) = 0;
         
         /**
           * This function is used to return the public IP address of the NAT iface, in
           * network byte order.
           * @param address Buffer that holds the IP address.
           * @see DS::IPAddrType.
           * @retval DS::SUCCESS Request received successfully.
           * @retval Other DS designated error codes might be returned.
           * @see DS_Errors.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetNatPublicIPAddress(::ds::IPAddrType* address) = 0;
         
         /**
           * This attribute holds information of the latest outage event
           * associated with this data network.          
           * @param value Attribute value
           * @see ds::Network::Outage::Info
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetOutageInfo(::ds::Net::OutageInfoType* value) = 0;
         
         /**
           * This attribute indicates the IP Address family of this INetwork
           * object.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetAddressFamily(::ds::AddrFamilyType* value) = 0;
         
         /**
           * This function returns the Domain Name Search List for this network.
           * @param domainNameSearchList Buffer array that holds the domain name
           * search list.
           * @see SeqDomainNameType.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetDomainNameSearchList(::ds::Net::DomainName* domainNameSearchList, int domainNameSearchListLen, int* domainNameSearchListLenReq) = 0;
         
         /**
           * This attribute returns the IP Layer Maximum Transmission Unit
           * associated with this network.          
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetNetMTU(int* value) = 0;
         
         /**
           * This attribute indicates the HW address associated with this network.
           * @param value Attribute value
           * @retval AEE_SUCCESS Request received successfully.
           * @retval QDS_EINVAL Iface does not support HW address.
           * @retval Other ds designated error codes might be returned.
           * @see ds::Net::Network::HWAddressType.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetHWAddress(::ds::Net::HWAddressType* value) = 0;
         
         /**
           * This function is used to explicitly bring down the network
           * connection.
           * Note that the applicable Network interface shall be brought down as
           * well ONLY if no other entities are using it when this function is
           * called. Still, the application can expect STATE_CHANGED event if it
           * is registered. The application may hold the Network object after
           * calling this API, but it should release it once the Network moves 
           * to QDS_CLOSED state.          
           * 
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Stop() = 0;
         
         /**
           * This attribute indicates the current RF condition.          
           * @param value Attribute value
           * @see ds::Net::Network::RFConditionType.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetCurrRFCondition(::ds::Net::RFConditionType* value) = 0;
         
         /**
           * This attribute points the Tx physical link object.          
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetTXPhysLink(::ds::Net::IPhysLink** value) = 0;
         
         /**
           * This attribute points the Rx physical link object.          
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetRXPhysLink(::ds::Net::IPhysLink** value) = 0;
         
         /**
           * This attribute indicates the dormancy info code when default phys
           * link is dormant.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetDormancyInfoCode(::ds::Net::DormancyInfoCodeType* value) = 0;
         
         /**
           * This attribute provides info on Network related statistics
           * @param value Attribute value
           * @param stats Output Network Statistics
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetNetworkStatistics(::ds::Net::NetworkStatsType* value) = 0;
         
         /**
           * This attribute resets Network related statistics
           * @param None
           */
         virtual ::AEEResult AEEINTERFACE_CDECL ResetNetworkStatistics() = 0;
         
         /**
           * This function allows object extensibility.
           * For supported interfaces, objects implementing those interfaces may be 
           * fetched via this function. The supported interfaces are documented in
           * the DS_NET_NetworkFactory.bid file. GetTechObject-supported interface
           * does not imply any similar contract in regard to QueryInterface for the
           * respective interface.
           * Unlike IQI, the availability of a specific interface depends on some 
           * factors, e.g. current network type. Moreover, there is no guaranty 
           * for transitivity or symmetry. 
           * Note: 'interface' parameter will map to iid and techObj.
           * @param iid The interface that should be retrieved.
           * @param techObj On success, will contain the requested interface instance.
           * @retval ds::SUCCESS Interface retrieved successfully.
           * @retval AEE_ECLASSNOTSUPPORT Specified interface unsupported.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetTechObject(AEEIID techObj_iid, void** techObj) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_INETWORK_H
