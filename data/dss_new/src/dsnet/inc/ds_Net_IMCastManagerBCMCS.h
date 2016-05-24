#ifndef DS_NET_IMCASTMANAGERBCMCS_H
#define DS_NET_IMCASTMANAGERBCMCS_H

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
typedef int ds_Net_BCMCSZoneValsType;
#define ds_Net_BCMCSZone_QDS_CDMA1X 0
#define ds_Net_BCMCSZone_QDS_HDR 1
#define ds_Net_BCMCS_HDR_ADDR_LEN 16
typedef unsigned char ds_Net_BCMCSHDRAddrType[16];
struct ds_Net_BCMCSZoneType {
   ds_Net_BCMCSZoneValsType type;
   ds_Net_BCMCSHDRAddrType subnet;
};
typedef struct ds_Net_BCMCSZoneType ds_Net_BCMCSZoneType;
typedef int ds_Net_BCMCSFramingValsType;

/** @memberof ds_Net_BCMCSFraming
  * 
  * Framing not specified. Service shall use a default value.
  */
#define ds_Net_BCMCSFraming_FRMV_NOT_SET -1

/** @memberof ds_Net_BCMCSFraming
  * 
  * Segment based framing.
  */
#define ds_Net_BCMCSFraming_FRMV_SEGMENT 0

/** @memberof ds_Net_BCMCSFraming
  * 
  * HDLC-like framing.
  */
#define ds_Net_BCMCSFraming_FRMV_HDLC 1
typedef int ds_Net_BCMCSProtocolValsType;

/** @memberof ds_Net_BCMCSProtocol
  * 
  * Protocol not specified. Service shall use a default value.
  */
#define ds_Net_BCMCSProtocol_PRTV_NOT_SET -1

/** @memberof ds_Net_BCMCSProtocol
  * 
  * The packet is PPP encapsulated (there are one or two protocol bytes
  * preceding the IP packet).
  */
#define ds_Net_BCMCSProtocol_PRTV_PPP 0

/** @memberof ds_Net_BCMCSProtocol
  * 
  * The packet obtained after deframing is an IPv4 packet without any
  * further encapsulation.
  */
#define ds_Net_BCMCSProtocol_PRTV_IPV4 1

/** @memberof ds_Net_BCMCSProtocol
  * 
  * The packet obtained after deframing is an IPv6 packet without any
  * further encapsulation.
  */
#define ds_Net_BCMCSProtocol_PRTV_IPV6 2
struct ds_Net_BCMCSDBSpecType {
   ds_Net_BCMCSZoneType zone;
   ds_IPAddrType multicastAddr;
   AEEINTERFACE_PADMEMBERS(programID, 2)
   int programID;
   unsigned char programIDLength;
   unsigned char flowDiscriminatorLength;
   unsigned char flowDiscriminator;
   AEEINTERFACE_PADMEMBERS(BCMCSPort, 1)
   unsigned short BCMCSPort;
   AEEINTERFACE_PADMEMBERS(framing, 2)
   ds_Net_BCMCSFramingValsType framing;
   ds_Net_BCMCSProtocolValsType protocol;
   unsigned char crcLength;
   unsigned char flowFormat;
   AEEINTERFACE_PADMEMBERS(flowID, 2)
   unsigned int flowID;
   unsigned char flowIDLen;
   boolean overwrite;
   unsigned char flowPriority;
   AEEINTERFACE_PADMEMBERS(__pad, 1)
};
typedef struct ds_Net_BCMCSDBSpecType ds_Net_BCMCSDBSpecType;

/**
  * Zone type corresponding to the entry. Mandatory field.
  */
/**
  * Multicast address of the flow. Mandatory field.
  */
/**
  * Identifier for the BCMCS program. Ignored if flowFormat is 0.
  * The program ID shall uniquely identify a program under a BCMCS
  * controller.
  */
/**
  * Program ID length in bits. Ignored if flowFormat is 0.
  */
/**
  * Flow discriminator length in bits. Ignored if flowFormat is 0.
  * Within the scope of an operator∆s network for a given program, the
  * length of flow id and the length of flow discriminator shall be
  * fixed for all flows.
  */
/**
  * Identifier for the BCMCS IP flow. Ignored if flowFormat is 0.
  * Flow discriminator is irrelevant if there is only one flow in a
  * program.
  * For a given program, the value of the flow discriminator shall
  * uniquely identify a flow.
  */
/**
  * Port number of the flow. Mandatory field.
  */
/**
  * Type of framing. Default value of FRMV_HDLC is used if
  * FRMV_NOT_SET is specified.
  */
/**
  * Encapsulating protocol. Default value of PRTV_PPP is used if
  * PRTV_NOT_SET is specified.
  */
/**
  * crc length in bytes. Default value of 2 is used if 0xFF is
  * specified.
  */
/**
  * flow_format. Valid values are 0 or 1. Mandatory field.
  * 0 - Flow id is specified by flowID. The Service shall directly use 
  *     the content of flowID to update the database.
  * 1 - Flow id is specified by programID and flowDiscriminator. The
  *     service shall create internally the flow id based on the
  *     provided program ID and flow discriminator and use that flow id
  *     to update the database.         
  */
/**
  * Flow id corresponding to multicastAddr:BCMCSPort pair.
  * Three MSBs that specify the length of the flow discriminator.
  * Variable length program ID that is an identifier for the BCMCS
  * program.
  * Flow discriminator that is an identifier for the BCMCS IP flow.
  *
  * When there is only one flow in a program, flow discriminator shall
  * not be specified in the BCMCS flow ID and the value of the three
  * MSBs of the flow ID shall be set to 000.
  *
  * Within the scope of an operator∆s network for a given program, the
  * length of flow id and the length of flow discriminator shall be
  * fixed for all flows.
  *
  * The program ID shall uniquely identify a program under a BCMCS
  * controller. For a given program, the value of the flow
  * discriminator shall uniquely identify a flow.
  */
/**
  * Flow id length in bits. Valid values are 16, 24 and 32 bits.
  */
/**
  * TRUE means to overwrite a matching entry in the database. In
  * addition, if the database is full and there is no mathching entry,
  * the oldest entry in the database shall be replaces with the new one.
  *
  * FALSE means the new entry shall not be added to the database if
  * there is already a matching entry. In addition, if the
  * database is full (and there is no mathching entry) the new entry
  * shall not be added to the database.
  * UpdateDB shall fail on both of those cases.
  * 
  * Matching criteria covers only the following fields:
  * - multicastAddr
  * - BCMCSPort
  * - zone          
  */
/** @memberof ds_Net
  * 
  * Flow priority.
  */
#define ds_Net_AEEIID_IMCastManagerBCMCS 0x107fc38
struct ds_Net_IMCastManagerBCMCS__SeqSockAddrStorageType__seq_octet_ds {
   ds_SockAddrStorageType* data;
   int dataLen;
   int dataLenReq;
};
typedef struct ds_Net_IMCastManagerBCMCS__SeqSockAddrStorageType__seq_octet_ds ds_Net_IMCastManagerBCMCS__SeqSockAddrStorageType__seq_octet_ds;
typedef ds_Net_IMCastManagerBCMCS__SeqSockAddrStorageType__seq_octet_ds ds_Net_IMCastManagerBCMCS_SeqSockAddrStorageType;

/** @interface ds_Net_IMCastManagerBCMCS
  * 
  * ds Net MCast Manager BCMCS interface.
  * @See IMCastManager_GetTechObject.
  * @See IMCastManagerExt_GetTechObject.
  * IMCastManagerBCMCS may be obtained (via IMCastManager/IMCastManager)
  * from a Network that was set with BCAST iface name/group policy).
  */
#define INHERIT_ds_Net_IMCastManagerBCMCS(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*UpdateDB)(iname* _pif, const ds_Net_BCMCSDBSpecType* bcmcsDBSpec); \
   AEEResult (*RegisterUsingHandoffOpt)(iname* _pif, const ds_SockAddrStorageType* addrSeq, int addrSeqLen)
AEEINTERFACE_DEFINE(ds_Net_IMCastManagerBCMCS);

/** @memberof ds_Net_IMCastManagerBCMCS
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IMCastManagerBCMCS_AddRef(ds_Net_IMCastManagerBCMCS* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerBCMCS)->AddRef(_pif);
}

/** @memberof ds_Net_IMCastManagerBCMCS
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IMCastManagerBCMCS_Release(ds_Net_IMCastManagerBCMCS* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerBCMCS)->Release(_pif);
}

/** @memberof ds_Net_IMCastManagerBCMCS
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IMCastManagerBCMCS_QueryInterface(ds_Net_IMCastManagerBCMCS* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerBCMCS)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IMCastManagerBCMCS
  * 
  * This function appends the given spec to the device's BCMCS database.
  * Based on the value of "overwrite" parameter in bcmcsDBSpec, a 
  * matching record in the BCMCS database may be overwritten by the
  * BCMCS specification provided in this API. In addition, the oldest
  * entry may be deleted from the database to allow addition of the new
  * one.          
  * 
  * @param _pif Pointer to interface
  * @param bcmcsDBSpec Structure that holds all the information to 
  * be updated.          
  * @retval AEE_SUCCESS Request received successfully.
  * @retval QDS_BCMCSDB_OVERFLOW The DataBase is full.
  * @retval QDS_BCMCSDB_NO_OVERWRITE BCMCSDB specification already
  *                                  present in the DB (and overwrite
  *                                  flag is set to FALSE).
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IMCastManagerBCMCS_UpdateDB(ds_Net_IMCastManagerBCMCS* _pif, const ds_Net_BCMCSDBSpecType* bcmcsDBSpec)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerBCMCS)->UpdateDB(_pif, bcmcsDBSpec);
}

/** @memberof ds_Net_IMCastManagerBCMCS
  *     
  * This function instructs setup of the provided MCast addresses for
  * registration to multicast groups using handoff optimization.
  * It is supported in conjunction with the API in IMcastManagerExt: 
  * The application should Join the Multicast groups and register to
  * them using the API in IMCastManagerExt in addition to the call to
  * this API.
  * @param _pif Pointer to interface
  * @param addrSeq Multicast group addresses.          
  * @param addrSeqLen Length of sequence
  * @see IMCastSession
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IMCastManagerBCMCS_RegisterUsingHandoffOpt(ds_Net_IMCastManagerBCMCS* _pif, const ds_SockAddrStorageType* addrSeq, int addrSeqLen)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerBCMCS)->RegisterUsingHandoffOpt(_pif, addrSeq, addrSeqLen);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Addr_Def.h"
namespace ds
{
   namespace Net
   {
      typedef int BCMCSZoneValsType;
      namespace BCMCSZone
      {
         const ::ds::Net::BCMCSZoneValsType QDS_CDMA1X = 0;
         const ::ds::Net::BCMCSZoneValsType QDS_HDR = 1;
      };
      const int BCMCS_HDR_ADDR_LEN = 16;
      typedef unsigned char BCMCSHDRAddrType[16];
      struct BCMCSZoneType {
         BCMCSZoneValsType type;
         BCMCSHDRAddrType subnet;
      };
      typedef int BCMCSFramingValsType;
      namespace BCMCSFraming
      {
         
         /**
           * Framing not specified. Service shall use a default value.
           */
         const ::ds::Net::BCMCSFramingValsType FRMV_NOT_SET = -1;
         
         /**
           * Segment based framing.
           */
         const ::ds::Net::BCMCSFramingValsType FRMV_SEGMENT = 0;
         
         /**
           * HDLC-like framing.
           */
         const ::ds::Net::BCMCSFramingValsType FRMV_HDLC = 1;
      };
      typedef int BCMCSProtocolValsType;
      namespace BCMCSProtocol
      {
         
         /**
           * Protocol not specified. Service shall use a default value.
           */
         const ::ds::Net::BCMCSProtocolValsType PRTV_NOT_SET = -1;
         
         /**
           * The packet is PPP encapsulated (there are one or two protocol bytes
           * preceding the IP packet).
           */
         const ::ds::Net::BCMCSProtocolValsType PRTV_PPP = 0;
         
         /**
           * The packet obtained after deframing is an IPv4 packet without any
           * further encapsulation.
           */
         const ::ds::Net::BCMCSProtocolValsType PRTV_IPV4 = 1;
         
         /**
           * The packet obtained after deframing is an IPv6 packet without any
           * further encapsulation.
           */
         const ::ds::Net::BCMCSProtocolValsType PRTV_IPV6 = 2;
      };
      struct BCMCSDBSpecType {
         BCMCSZoneType zone;
         ::ds::IPAddrType multicastAddr;
         AEEINTERFACE_PADMEMBERS(programID, 2)
         int programID;
         unsigned char programIDLength;
         unsigned char flowDiscriminatorLength;
         unsigned char flowDiscriminator;
         AEEINTERFACE_PADMEMBERS(BCMCSPort, 1)
         unsigned short BCMCSPort;
         AEEINTERFACE_PADMEMBERS(framing, 2)
         BCMCSFramingValsType framing;
         BCMCSProtocolValsType protocol;
         unsigned char crcLength;
         unsigned char flowFormat;
         AEEINTERFACE_PADMEMBERS(flowID, 2)
         unsigned int flowID;
         unsigned char flowIDLen;
         boolean overwrite;
         unsigned char flowPriority;
         AEEINTERFACE_PADMEMBERS(__pad, 1)
      };
      
      /**
        * Zone type corresponding to the entry. Mandatory field.
        */
      /**
        * Multicast address of the flow. Mandatory field.
        */
      /**
        * Identifier for the BCMCS program. Ignored if flowFormat is 0.
        * The program ID shall uniquely identify a program under a BCMCS
        * controller.
        */
      /**
        * Program ID length in bits. Ignored if flowFormat is 0.
        */
      /**
        * Flow discriminator length in bits. Ignored if flowFormat is 0.
        * Within the scope of an operator∆s network for a given program, the
        * length of flow id and the length of flow discriminator shall be
        * fixed for all flows.
        */
      /**
        * Identifier for the BCMCS IP flow. Ignored if flowFormat is 0.
        * Flow discriminator is irrelevant if there is only one flow in a
        * program.
        * For a given program, the value of the flow discriminator shall
        * uniquely identify a flow.
        */
      /**
        * Port number of the flow. Mandatory field.
        */
      /**
        * Type of framing. Default value of FRMV_HDLC is used if
        * FRMV_NOT_SET is specified.
        */
      /**
        * Encapsulating protocol. Default value of PRTV_PPP is used if
        * PRTV_NOT_SET is specified.
        */
      /**
        * crc length in bytes. Default value of 2 is used if 0xFF is
        * specified.
        */
      /**
        * flow_format. Valid values are 0 or 1. Mandatory field.
        * 0 - Flow id is specified by flowID. The Service shall directly use 
        *     the content of flowID to update the database.
        * 1 - Flow id is specified by programID and flowDiscriminator. The
        *     service shall create internally the flow id based on the
        *     provided program ID and flow discriminator and use that flow id
        *     to update the database.         
        */
      /**
        * Flow id corresponding to multicastAddr:BCMCSPort pair.
        * Three MSBs that specify the length of the flow discriminator.
        * Variable length program ID that is an identifier for the BCMCS
        * program.
        * Flow discriminator that is an identifier for the BCMCS IP flow.
        *
        * When there is only one flow in a program, flow discriminator shall
        * not be specified in the BCMCS flow ID and the value of the three
        * MSBs of the flow ID shall be set to 000.
        *
        * Within the scope of an operator∆s network for a given program, the
        * length of flow id and the length of flow discriminator shall be
        * fixed for all flows.
        *
        * The program ID shall uniquely identify a program under a BCMCS
        * controller. For a given program, the value of the flow
        * discriminator shall uniquely identify a flow.
        */
      /**
        * Flow id length in bits. Valid values are 16, 24 and 32 bits.
        */
      /**
        * TRUE means to overwrite a matching entry in the database. In
        * addition, if the database is full and there is no mathching entry,
        * the oldest entry in the database shall be replaces with the new one.
        *
        * FALSE means the new entry shall not be added to the database if
        * there is already a matching entry. In addition, if the
        * database is full (and there is no mathching entry) the new entry
        * shall not be added to the database.
        * UpdateDB shall fail on both of those cases.
        * 
        * Matching criteria covers only the following fields:
        * - multicastAddr
        * - BCMCSPort
        * - zone          
        */
      /**
        * Flow priority.
        */
      const ::AEEIID AEEIID_IMCastManagerBCMCS = 0x107fc38;
      
      /** @interface IMCastManagerBCMCS
        * 
        * ds Net MCast Manager BCMCS interface.
        * @See IMCastManager::GetTechObject.
        * @See IMCastManagerExt::GetTechObject.
        * IMCastManagerBCMCS may be obtained (via IMCastManager/IMCastManager)
        * from a Network that was set with BCAST iface name/group policy).
        */
      struct IMCastManagerBCMCS : public ::IQI
      {
         struct _SeqSockAddrStorageType__seq_octet_ds {
            ::ds::SockAddrStorageType* data;
            int dataLen;
            int dataLenReq;
         };
         typedef _SeqSockAddrStorageType__seq_octet_ds SeqSockAddrStorageType;
         
         /**
           * This function appends the given spec to the device's BCMCS database.
           * Based on the value of "overwrite" parameter in bcmcsDBSpec, a 
           * matching record in the BCMCS database may be overwritten by the
           * BCMCS specification provided in this API. In addition, the oldest
           * entry may be deleted from the database to allow addition of the new
           * one.          
           * 
           * @param bcmcsDBSpec Structure that holds all the information to 
           * be updated.          
           * @retval AEE_SUCCESS Request received successfully.
           * @retval QDS_BCMCSDB_OVERFLOW The DataBase is full.
           * @retval QDS_BCMCSDB_NO_OVERWRITE BCMCSDB specification already
           *                                  present in the DB (and overwrite
           *                                  flag is set to FALSE).
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL UpdateDB(const ::ds::Net::BCMCSDBSpecType* bcmcsDBSpec) = 0;
         
         /**    
           * This function instructs setup of the provided MCast addresses for
           * registration to multicast groups using handoff optimization.
           * It is supported in conjunction with the API in IMcastManagerExt: 
           * The application should Join the Multicast groups and register to
           * them using the API in IMCastManagerExt in addition to the call to
           * this API.
           * @param addrSeq Multicast group addresses.          
           * @see IMCastSession
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL RegisterUsingHandoffOpt(const ::ds::SockAddrStorageType* addrSeq, int addrSeqLen) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IMCASTMANAGERBCMCS_H
