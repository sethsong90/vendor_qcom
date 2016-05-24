#ifndef DS_NET_IIPFILTERPRIV_H
#define DS_NET_IIPFILTERPRIV_H

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
typedef int ds_Net_IPFilterIDType;
#define ds_Net_IPFilterID_QDS_SRC_ADDR 101
#define ds_Net_IPFilterID_QDS_DST_ADDR 102
#define ds_Net_IPFilterID_QDS_NEXT_HDR_PROTO 103
#define ds_Net_IPFilterID_QDS_IPV4_TOS 201
#define ds_Net_IPFilterID_QDS_IPV6_TRF_CLASS 301
#define ds_Net_IPFilterID_QDS_IPV6_FLOW_LABEL 302
#define ds_Net_IPFilterID_QDS_TCP_SRC_PORT 401
#define ds_Net_IPFilterID_QDS_TCP_DST_PORT 402
#define ds_Net_IPFilterID_QDS_UDP_SRC_PORT 501
#define ds_Net_IPFilterID_QDS_UDP_DST_PORT 502
#define ds_Net_IPFilterID_QDS_ESP_SPI 601
#define ds_Net_IPFilterID_QDS_ICMP_MTYPE 1000
#define ds_Net_IPFilterID_QDS_ICMP_MCODE 1001
struct ds_Net__SeqIPFilterIDType__seq_long {
   ds_Net_IPFilterIDType* data;
   int dataLen;
   int dataLenReq;
};
typedef struct ds_Net__SeqIPFilterIDType__seq_long ds_Net__SeqIPFilterIDType__seq_long;
typedef ds_Net__SeqIPFilterIDType__seq_long ds_Net_SeqIPFilterIDType;
typedef unsigned char ds_Net_IPFilterIPVersionType;
#define ds_Net_IPFilterIPVersion_QDS_FILTER_IPV4 4
#define ds_Net_IPFilterIPVersion_QDS_FILTER_IPV6 6
typedef unsigned char ds_Net_IPFilterIPNextProtocolType;
#define ds_Net_IPFilterIPNextProtocol_QDS_FILTER_ICMP 1
#define ds_Net_IPFilterIPNextProtocol_QDS_FILTER_TCP 6
#define ds_Net_IPFilterIPNextProtocol_QDS_FILTER_UDP 17
#define ds_Net_IPFilterIPNextProtocol_QDS_FILTER_ESP 50
typedef int ds_Net_IPFilterIPv6FlowLabelType;
typedef unsigned int ds_Net_IPFilterSpiType;
struct ds_Net_IPFilterIPv4AddrType {
   ds_Net_INAddrType addr;
   ds_Net_INAddrType subnetMask;
};
typedef struct ds_Net_IPFilterIPv4AddrType ds_Net_IPFilterIPv4AddrType;
struct ds_Net_IPFilterTOSType {
   unsigned char val;
   unsigned char mask;
};
typedef struct ds_Net_IPFilterTOSType ds_Net_IPFilterTOSType;
struct ds_Net_IPFilterPortType {
   ds_Net_INPortType port;
   unsigned short range;
};
typedef struct ds_Net_IPFilterPortType ds_Net_IPFilterPortType;
struct ds_Net_IPFilterIPv6AddrType {
   ds_INAddr6Type addr;
   unsigned char prefixLen;
};
typedef struct ds_Net_IPFilterIPv6AddrType ds_Net_IPFilterIPv6AddrType;
struct ds_Net_IPFilterIPv6TrafficClassType {
   unsigned char val;
   unsigned char mask;
};
typedef struct ds_Net_IPFilterIPv6TrafficClassType ds_Net_IPFilterIPv6TrafficClassType;
struct ds_Net_IPFilterAuxInfoType {
   unsigned short fi_id;
   unsigned short fi_precedence;
};
typedef struct ds_Net_IPFilterAuxInfoType ds_Net_IPFilterAuxInfoType;
#define ds_Net_AEEIID_IIPFilterPriv 0x106dcc4

/** @interface ds_Net_IIPFilterPriv
  * 
  * ds IP Filter interface.
  * This interface provides a common base for all the possible values of
  * IP filter. 
  */
#define INHERIT_ds_Net_IIPFilterPriv(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*Clone)(iname* _pif, ds_Net_IIPFilterPriv** filter); \
   AEEResult (*GetValidOptions)(iname* _pif, ds_Net_IPFilterIDType* value, int valueLen, int* valueLenReq); \
   AEEResult (*GetErroneousOptions)(iname* _pif, ds_Net_IPFilterIDType* value, int valueLen, int* valueLenReq); \
   AEEResult (*GetIPVsn)(iname* _pif, ds_Net_IPFilterIPVersionType* value); \
   AEEResult (*SetIPVsn)(iname* _pif, ds_Net_IPFilterIPVersionType value); \
   AEEResult (*GetNextHdrProt)(iname* _pif, ds_Net_IPFilterIPNextProtocolType* value); \
   AEEResult (*SetNextHdrProt)(iname* _pif, ds_Net_IPFilterIPNextProtocolType value); \
   AEEResult (*GetSrcPort)(iname* _pif, ds_Net_IPFilterPortType* value); \
   AEEResult (*SetSrcPort)(iname* _pif, const ds_Net_IPFilterPortType* value); \
   AEEResult (*GetDstPort)(iname* _pif, ds_Net_IPFilterPortType* value); \
   AEEResult (*SetDstPort)(iname* _pif, const ds_Net_IPFilterPortType* value); \
   AEEResult (*GetSrcV4)(iname* _pif, ds_Net_IPFilterIPv4AddrType* value); \
   AEEResult (*SetSrcV4)(iname* _pif, const ds_Net_IPFilterIPv4AddrType* value); \
   AEEResult (*GetDstV4)(iname* _pif, ds_Net_IPFilterIPv4AddrType* value); \
   AEEResult (*SetDstV4)(iname* _pif, const ds_Net_IPFilterIPv4AddrType* value); \
   AEEResult (*GetTos)(iname* _pif, ds_Net_IPFilterTOSType* value); \
   AEEResult (*SetTos)(iname* _pif, const ds_Net_IPFilterTOSType* value); \
   AEEResult (*GetFlowLabel)(iname* _pif, ds_Net_IPFilterIPv6FlowLabelType* value); \
   AEEResult (*SetFlowLabel)(iname* _pif, ds_Net_IPFilterIPv6FlowLabelType value); \
   AEEResult (*GetSrcV6)(iname* _pif, ds_Net_IPFilterIPv6AddrType* value); \
   AEEResult (*SetSrcV6)(iname* _pif, const ds_Net_IPFilterIPv6AddrType* value); \
   AEEResult (*GetDstV6)(iname* _pif, ds_Net_IPFilterIPv6AddrType* value); \
   AEEResult (*SetDstV6)(iname* _pif, const ds_Net_IPFilterIPv6AddrType* value); \
   AEEResult (*GetTrafficClass)(iname* _pif, ds_Net_IPFilterIPv6TrafficClassType* value); \
   AEEResult (*SetTrafficClass)(iname* _pif, const ds_Net_IPFilterIPv6TrafficClassType* value); \
   AEEResult (*GetEspSpi)(iname* _pif, int* value); \
   AEEResult (*SetEspSpi)(iname* _pif, int value); \
   AEEResult (*GetICMPType)(iname* _pif, unsigned char* value); \
   AEEResult (*SetICMPType)(iname* _pif, unsigned char value); \
   AEEResult (*GetICMPCode)(iname* _pif, unsigned char* value); \
   AEEResult (*SetICMPCode)(iname* _pif, unsigned char value); \
   AEEResult (*GetAuxInfo)(iname* _pif, ds_Net_IPFilterAuxInfoType* value)
AEEINTERFACE_DEFINE(ds_Net_IIPFilterPriv);

/** @memberof ds_Net_IIPFilterPriv
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IIPFilterPriv_AddRef(ds_Net_IIPFilterPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->AddRef(_pif);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IIPFilterPriv_Release(ds_Net_IIPFilterPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->Release(_pif);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IIPFilterPriv_QueryInterface(ds_Net_IIPFilterPriv* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This function creates an identical copy of the IIPFilter.
  * @param _pif Pointer to interface
  * @param filter The created IIPFilter.
  * @retval ds_SUCCESS IIPFilter cloned successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IIPFilterPriv_Clone(ds_Net_IIPFilterPriv* _pif, ds_Net_IIPFilterPriv** filter)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->Clone(_pif, filter);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute represents the valid options - if an option was set,
  * its ID will be in this list.
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @param valueLen Length of sequence
  * @param valueLenReq Required length of sequence
  */
static __inline AEEResult ds_Net_IIPFilterPriv_GetValidOptions(ds_Net_IIPFilterPriv* _pif, ds_Net_IPFilterIDType* value, int valueLen, int* valueLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->GetValidOptions(_pif, value, valueLen, valueLenReq);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute represents a list of erroneous options into
  * the IIPFilterPriv object.
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @param valueLen Length of sequence
  * @param valueLenReq Required length of sequence
  */
static __inline AEEResult ds_Net_IIPFilterPriv_GetErroneousOptions(ds_Net_IIPFilterPriv* _pif, ds_Net_IPFilterIDType* value, int valueLen, int* valueLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->GetErroneousOptions(_pif, value, valueLen, valueLenReq);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the IP version.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_GetIPVsn(ds_Net_IIPFilterPriv* _pif, ds_Net_IPFilterIPVersionType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->GetIPVsn(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the IP version.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_SetIPVsn(ds_Net_IIPFilterPriv* _pif, ds_Net_IPFilterIPVersionType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->SetIPVsn(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the next header protocol.
  * It identifies the higher layer protocol (TCP/UDP) that needs to be
  * considered for filtering an IP packet. If this field is specified,
  * only IP packets belonging to the specified higher layer protocol
  * are considered for filtering. The filtering can be further enhanced
  * by specifying parameters from that protocol header fields.
  * Only parameters from the NextHdrProt are considered (other protocol
  * header fields are ignored).
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_GetNextHdrProt(ds_Net_IIPFilterPriv* _pif, ds_Net_IPFilterIPNextProtocolType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->GetNextHdrProt(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the next header protocol.
  * It identifies the higher layer protocol (TCP/UDP) that needs to be
  * considered for filtering an IP packet. If this field is specified,
  * only IP packets belonging to the specified higher layer protocol
  * are considered for filtering. The filtering can be further enhanced
  * by specifying parameters from that protocol header fields.
  * Only parameters from the NextHdrProt are considered (other protocol
  * header fields are ignored).
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_SetNextHdrProt(ds_Net_IIPFilterPriv* _pif, ds_Net_IPFilterIPNextProtocolType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->SetNextHdrProt(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the source port.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_GetSrcPort(ds_Net_IIPFilterPriv* _pif, ds_Net_IPFilterPortType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->GetSrcPort(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the source port.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_SetSrcPort(ds_Net_IIPFilterPriv* _pif, const ds_Net_IPFilterPortType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->SetSrcPort(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the destination port.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_GetDstPort(ds_Net_IIPFilterPriv* _pif, ds_Net_IPFilterPortType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->GetDstPort(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the destination port.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_SetDstPort(ds_Net_IIPFilterPriv* _pif, const ds_Net_IPFilterPortType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->SetDstPort(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the source IPv4 address.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_GetSrcV4(ds_Net_IIPFilterPriv* _pif, ds_Net_IPFilterIPv4AddrType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->GetSrcV4(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the source IPv4 address.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_SetSrcV4(ds_Net_IIPFilterPriv* _pif, const ds_Net_IPFilterIPv4AddrType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->SetSrcV4(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the destination IPv4 address.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_GetDstV4(ds_Net_IIPFilterPriv* _pif, ds_Net_IPFilterIPv4AddrType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->GetDstV4(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the destination IPv4 address.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_SetDstV4(ds_Net_IIPFilterPriv* _pif, const ds_Net_IPFilterIPv4AddrType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->SetDstV4(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the type of service.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_GetTos(ds_Net_IIPFilterPriv* _pif, ds_Net_IPFilterTOSType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->GetTos(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the type of service.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_SetTos(ds_Net_IIPFilterPriv* _pif, const ds_Net_IPFilterTOSType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->SetTos(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the IPv6 flow label.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_GetFlowLabel(ds_Net_IIPFilterPriv* _pif, ds_Net_IPFilterIPv6FlowLabelType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->GetFlowLabel(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the IPv6 flow label.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_SetFlowLabel(ds_Net_IIPFilterPriv* _pif, ds_Net_IPFilterIPv6FlowLabelType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->SetFlowLabel(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the source IPv6 address.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_GetSrcV6(ds_Net_IIPFilterPriv* _pif, ds_Net_IPFilterIPv6AddrType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->GetSrcV6(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the source IPv6 address.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_SetSrcV6(ds_Net_IIPFilterPriv* _pif, const ds_Net_IPFilterIPv6AddrType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->SetSrcV6(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the destination IPv6 address.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_GetDstV6(ds_Net_IIPFilterPriv* _pif, ds_Net_IPFilterIPv6AddrType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->GetDstV6(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the destination IPv6 address.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_SetDstV6(ds_Net_IIPFilterPriv* _pif, const ds_Net_IPFilterIPv6AddrType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->SetDstV6(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the traffic class.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_GetTrafficClass(ds_Net_IIPFilterPriv* _pif, ds_Net_IPFilterIPv6TrafficClassType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->GetTrafficClass(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the traffic class.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_SetTrafficClass(ds_Net_IIPFilterPriv* _pif, const ds_Net_IPFilterIPv6TrafficClassType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->SetTrafficClass(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the ESP SPI parameter value.
  * This is a Security Parameter Index as defined in RFC 2406.
  * If specified, the SPI field in ESP header shall be considered for
  * filtering an IP packet.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_GetEspSpi(ds_Net_IIPFilterPriv* _pif, int* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->GetEspSpi(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the ESP SPI parameter value.
  * This is a Security Parameter Index as defined in RFC 2406.
  * If specified, the SPI field in ESP header shall be considered for
  * filtering an IP packet.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_SetEspSpi(ds_Net_IIPFilterPriv* _pif, int value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->SetEspSpi(_pif, value);
}

/**
  * This attribute indicates the Filter ID and precedence value.
  */
/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the ICMP type.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_GetICMPType(ds_Net_IIPFilterPriv* _pif, unsigned char* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->GetICMPType(_pif, value);
}

/**
  * This attribute indicates the Filter ID and precedence value.
  */
/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the ICMP type.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_SetICMPType(ds_Net_IIPFilterPriv* _pif, unsigned char value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->SetICMPType(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the ICMP code.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_GetICMPCode(ds_Net_IIPFilterPriv* _pif, unsigned char* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->GetICMPCode(_pif, value);
}

/** @memberof ds_Net_IIPFilterPriv
  * 
  * This attribute indicates the ICMP code.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPFilterPriv_SetICMPCode(ds_Net_IIPFilterPriv* _pif, unsigned char value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->SetICMPCode(_pif, value);
}
static __inline AEEResult ds_Net_IIPFilterPriv_GetAuxInfo(ds_Net_IIPFilterPriv* _pif, ds_Net_IPFilterAuxInfoType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterPriv)->GetAuxInfo(_pif, value);
}
struct ds_Net__SeqIIPFilterPrivType__seq_IIPFilterPriv_Net_ds {
   struct ds_Net_IIPFilterPriv** data;
   int dataLen;
   int dataLenReq;
};
typedef struct ds_Net__SeqIIPFilterPrivType__seq_IIPFilterPriv_Net_ds ds_Net__SeqIIPFilterPrivType__seq_IIPFilterPriv_Net_ds;
typedef ds_Net__SeqIIPFilterPrivType__seq_IIPFilterPriv_Net_ds ds_Net_SeqIIPFilterPrivType;
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
namespace ds
{
   namespace Net
   {
      typedef int IPFilterIDType;
      namespace IPFilterID
      {
         const ::ds::Net::IPFilterIDType QDS_SRC_ADDR = 101;
         const ::ds::Net::IPFilterIDType QDS_DST_ADDR = 102;
         const ::ds::Net::IPFilterIDType QDS_NEXT_HDR_PROTO = 103;
         const ::ds::Net::IPFilterIDType QDS_IPV4_TOS = 201;
         const ::ds::Net::IPFilterIDType QDS_IPV6_TRF_CLASS = 301;
         const ::ds::Net::IPFilterIDType QDS_IPV6_FLOW_LABEL = 302;
         const ::ds::Net::IPFilterIDType QDS_TCP_SRC_PORT = 401;
         const ::ds::Net::IPFilterIDType QDS_TCP_DST_PORT = 402;
         const ::ds::Net::IPFilterIDType QDS_UDP_SRC_PORT = 501;
         const ::ds::Net::IPFilterIDType QDS_UDP_DST_PORT = 502;
         const ::ds::Net::IPFilterIDType QDS_ESP_SPI = 601;
         const ::ds::Net::IPFilterIDType QDS_ICMP_MTYPE = 1000;
         const ::ds::Net::IPFilterIDType QDS_ICMP_MCODE = 1001;
      };
      struct _SeqIPFilterIDType__seq_long {
         IPFilterIDType* data;
         int dataLen;
         int dataLenReq;
      };
      typedef _SeqIPFilterIDType__seq_long SeqIPFilterIDType;
      typedef unsigned char IPFilterIPVersionType;
      namespace IPFilterIPVersion
      {
         const ::ds::Net::IPFilterIPVersionType QDS_FILTER_IPV4 = 4;
         const ::ds::Net::IPFilterIPVersionType QDS_FILTER_IPV6 = 6;
      };
      typedef unsigned char IPFilterIPNextProtocolType;
      namespace IPFilterIPNextProtocol
      {
         const ::ds::Net::IPFilterIPNextProtocolType QDS_FILTER_ICMP = 1;
         const ::ds::Net::IPFilterIPNextProtocolType QDS_FILTER_TCP = 6;
         const ::ds::Net::IPFilterIPNextProtocolType QDS_FILTER_UDP = 17;
         const ::ds::Net::IPFilterIPNextProtocolType QDS_FILTER_ESP = 50;
      };
      typedef int IPFilterIPv6FlowLabelType;
      typedef unsigned int IPFilterSpiType;
      struct IPFilterIPv4AddrType {
         INAddrType addr;
         INAddrType subnetMask;
      };
      struct IPFilterTOSType {
         unsigned char val;
         unsigned char mask;
      };
      struct IPFilterPortType {
         INPortType port;
         unsigned short range;
      };
      struct IPFilterIPv6AddrType {
         ::ds::INAddr6Type addr;
         unsigned char prefixLen;
      };
      struct IPFilterIPv6TrafficClassType {
         unsigned char val;
         unsigned char mask;
      };
      struct IPFilterAuxInfoType {
         unsigned short fi_id;
         unsigned short fi_precedence;
      };
      const ::AEEIID AEEIID_IIPFilterPriv = 0x106dcc4;
      
      /** @interface IIPFilterPriv
        * 
        * ds IP Filter interface.
        * This interface provides a common base for all the possible values of
        * IP filter. 
        */
      struct IIPFilterPriv : public ::IQI
      {
         
         /**
           * This function creates an identical copy of the IIPFilter.
           * @param filter The created IIPFilter.
           * @retval ds::SUCCESS IIPFilter cloned successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Clone(IIPFilterPriv** filter) = 0;
         
         /**
           * This attribute represents the valid options - if an option was set,
           * its ID will be in this list.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetValidOptions(::ds::Net::IPFilterIDType* value, int valueLen, int* valueLenReq) = 0;
         
         /**
           * This attribute represents a list of erroneous options into
           * the IIPFilterPriv object.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetErroneousOptions(::ds::Net::IPFilterIDType* value, int valueLen, int* valueLenReq) = 0;
         
         /**
           * This attribute indicates the IP version.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetIPVsn(::ds::Net::IPFilterIPVersionType* value) = 0;
         
         /**
           * This attribute indicates the IP version.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetIPVsn(::ds::Net::IPFilterIPVersionType value) = 0;
         
         /**
           * This attribute indicates the next header protocol.
           * It identifies the higher layer protocol (TCP/UDP) that needs to be
           * considered for filtering an IP packet. If this field is specified,
           * only IP packets belonging to the specified higher layer protocol
           * are considered for filtering. The filtering can be further enhanced
           * by specifying parameters from that protocol header fields.
           * Only parameters from the NextHdrProt are considered (other protocol
           * header fields are ignored).
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetNextHdrProt(::ds::Net::IPFilterIPNextProtocolType* value) = 0;
         
         /**
           * This attribute indicates the next header protocol.
           * It identifies the higher layer protocol (TCP/UDP) that needs to be
           * considered for filtering an IP packet. If this field is specified,
           * only IP packets belonging to the specified higher layer protocol
           * are considered for filtering. The filtering can be further enhanced
           * by specifying parameters from that protocol header fields.
           * Only parameters from the NextHdrProt are considered (other protocol
           * header fields are ignored).
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetNextHdrProt(::ds::Net::IPFilterIPNextProtocolType value) = 0;
         
         /**
           * This attribute indicates the source port.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetSrcPort(::ds::Net::IPFilterPortType* value) = 0;
         
         /**
           * This attribute indicates the source port.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetSrcPort(const ::ds::Net::IPFilterPortType* value) = 0;
         
         /**
           * This attribute indicates the destination port.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetDstPort(::ds::Net::IPFilterPortType* value) = 0;
         
         /**
           * This attribute indicates the destination port.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetDstPort(const ::ds::Net::IPFilterPortType* value) = 0;
         
         /**
           * This attribute indicates the source IPv4 address.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetSrcV4(::ds::Net::IPFilterIPv4AddrType* value) = 0;
         
         /**
           * This attribute indicates the source IPv4 address.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetSrcV4(const ::ds::Net::IPFilterIPv4AddrType* value) = 0;
         
         /**
           * This attribute indicates the destination IPv4 address.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetDstV4(::ds::Net::IPFilterIPv4AddrType* value) = 0;
         
         /**
           * This attribute indicates the destination IPv4 address.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetDstV4(const ::ds::Net::IPFilterIPv4AddrType* value) = 0;
         
         /**
           * This attribute indicates the type of service.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetTos(::ds::Net::IPFilterTOSType* value) = 0;
         
         /**
           * This attribute indicates the type of service.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetTos(const ::ds::Net::IPFilterTOSType* value) = 0;
         
         /**
           * This attribute indicates the IPv6 flow label.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetFlowLabel(::ds::Net::IPFilterIPv6FlowLabelType* value) = 0;
         
         /**
           * This attribute indicates the IPv6 flow label.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetFlowLabel(::ds::Net::IPFilterIPv6FlowLabelType value) = 0;
         
         /**
           * This attribute indicates the source IPv6 address.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetSrcV6(::ds::Net::IPFilterIPv6AddrType* value) = 0;
         
         /**
           * This attribute indicates the source IPv6 address.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetSrcV6(const ::ds::Net::IPFilterIPv6AddrType* value) = 0;
         
         /**
           * This attribute indicates the destination IPv6 address.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetDstV6(::ds::Net::IPFilterIPv6AddrType* value) = 0;
         
         /**
           * This attribute indicates the destination IPv6 address.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetDstV6(const ::ds::Net::IPFilterIPv6AddrType* value) = 0;
         
         /**
           * This attribute indicates the traffic class.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetTrafficClass(::ds::Net::IPFilterIPv6TrafficClassType* value) = 0;
         
         /**
           * This attribute indicates the traffic class.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetTrafficClass(const ::ds::Net::IPFilterIPv6TrafficClassType* value) = 0;
         
         /**
           * This attribute indicates the ESP SPI parameter value.
           * This is a Security Parameter Index as defined in RFC 2406.
           * If specified, the SPI field in ESP header shall be considered for
           * filtering an IP packet.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetEspSpi(int* value) = 0;
         
         /**
           * This attribute indicates the ESP SPI parameter value.
           * This is a Security Parameter Index as defined in RFC 2406.
           * If specified, the SPI field in ESP header shall be considered for
           * filtering an IP packet.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetEspSpi(int value) = 0;
         
         /**
           * This attribute indicates the Filter ID and precedence value.
           */
         /**
           * This attribute indicates the ICMP type.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetICMPType(unsigned char* value) = 0;
         
         /**
           * This attribute indicates the Filter ID and precedence value.
           */
         /**
           * This attribute indicates the ICMP type.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetICMPType(unsigned char value) = 0;
         
         /**
           * This attribute indicates the ICMP code.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetICMPCode(unsigned char* value) = 0;
         
         /**
           * This attribute indicates the ICMP code.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetICMPCode(unsigned char value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetAuxInfo(::ds::Net::IPFilterAuxInfoType* value) = 0;
      };
      struct _SeqIIPFilterPrivType__seq_IIPFilterPriv_Net_ds {
         IIPFilterPriv** data;
         int dataLen;
         int dataLenReq;
      };
      typedef _SeqIIPFilterPrivType__seq_IIPFilterPriv_Net_ds SeqIIPFilterPrivType;
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IIPFILTERPRIV_H
