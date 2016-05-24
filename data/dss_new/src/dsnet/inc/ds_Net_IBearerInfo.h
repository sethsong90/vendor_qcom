#ifndef DS_NET_IBEARERINFO_H
#define DS_NET_IBEARERINFO_H

/*============================================================================
  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Net_Def.h"

/** @memberof ds_Net_BearerTechCDMA
  * 
  * CDMA sub-technologies. 
  * Since a mobile device may simultaneously support more than one
  * technology, the following may be ORed together to indicate all the
  * technologies supported by the bearer.
  */
#define ds_Net_BearerTechCDMA_SUBTECH_1X 0x1
#define ds_Net_BearerTechCDMA_SUBTECH_EVDO_REV0 0x2
#define ds_Net_BearerTechCDMA_SUBTECH_EVDO_REVA 0x4
#define ds_Net_BearerTechCDMA_SUBTECH_EVDO_REVB 0x8

/** @memberof ds_Net_BearerTechCDMA
  * 
  * SUBTECH_NULL shall be indicated if the application queries for the
  * the bearer technology but there is currently no active Network 
  * connection.
  */
#define ds_Net_BearerTechCDMA_SUBTECH_NULL 0x8000

/** @memberof ds_Net_BearerTechCDMA
  * 
  * CDMA 1x specific service options.
  * Since a mobile device may simultaneously support more than one
  * service options, the following may be ORed together to indicate all
  * the service options supported by the bearer.
  */
#define ds_Net_BearerTechCDMA_SO_1X_IS95 0x1
#define ds_Net_BearerTechCDMA_SO_1X_IS2000 0x2
#define ds_Net_BearerTechCDMA_SO_1X_IS2000_REL_A 0x4

/** @memberof ds_Net_BearerTechCDMA
  * 
  * CDMA EVDO (HDR) specific service options. Packet Applications are
  * not dependent on Rev0/RevA/RevB, therefore the constants are reused
  * for all revisions.
  * Since a mobile device may simultaneously support more than one
  * service options, the following may be ORed together to indicate all
  * the service options supported by the bearer.
  */
#define ds_Net_BearerTechCDMA_SO_EVDO_DPA 0x1
#define ds_Net_BearerTechCDMA_SO_EVDO_MFPA 0x2
#define ds_Net_BearerTechCDMA_SO_EVDO_EMPA 0x4
#define ds_Net_BearerTechCDMA_SO_EVDO_EMPA_EHRPD 0x8
#define ds_Net_BearerTechCDMA_SO_EVDO_MMPA 0x10
#define ds_Net_BearerTechCDMA_SO_EVDO_MMPA_EHRPD 0x20

/** @memberof ds_Net_BearerTechUMTS
  * 
  * UMTS sub-technologies. 
  * Since a mobile device may simultaneously support more than one
  * technology, the following may be ORed together to indicate all the
  * technologies supported by the bearer.
  * For example, in WCDMA the WCDMA subtechnology is set; in addition,
  * HSUPA and HSDPA flags may be set when the respective subtechnologies
  * are available.
  */
#define ds_Net_BearerTechUMTS_SUBTECH_WCDMA 0x1
#define ds_Net_BearerTechUMTS_SUBTECH_GPRS 0x2
#define ds_Net_BearerTechUMTS_SUBTECH_HSDPA 0x4
#define ds_Net_BearerTechUMTS_SUBTECH_HSUPA 0x8
#define ds_Net_BearerTechUMTS_SUBTECH_EDGE 0x10
#define ds_Net_BearerTechUMTS_SUBTECH_LTE 0x20

/** @memberof ds_Net_BearerTechUMTS
  * 
  * HSDPA+ bearer
  */
#define ds_Net_BearerTechUMTS_SUBTECH_HSDPAPLUS 0x40

/** @memberof ds_Net_BearerTechUMTS
  * 
  * Dual Carrier HSDPA+ bearer
  */
#define ds_Net_BearerTechUMTS_SUBTECH_DC_HSDPAPLUS 0x80

/** @memberof ds_Net_BearerTechUMTS
  * 
  * SUBTECH_NULL shall be indicated if the application queries for the
  * the bearer technology but there is currently no active Network 
  * connection.
  */
#define ds_Net_BearerTechUMTS_SUBTECH_NULL 0x8000

/** Data bearer rate definitions */
struct ds_Net_BearerTechRateType {
   int maxTxDataRate; /**< Max Tx bearer data rate */
   int maxRxDataRate; /**< Max Rx bearer data rate */
   int avgTxDataRate; /**< Average Tx bearer data rate */
   int avgRxDataRate; /**< Average Rx bearer data rate */
   int currentTxDataRate; /**< Current Tx bearer data rate */
   int currentRxDataRate; /**< Current Rx bearer data rate */
};
typedef struct ds_Net_BearerTechRateType ds_Net_BearerTechRateType;
#define ds_Net_AEEIID_IBearerInfo 0x106c6a5
#define INHERIT_ds_Net_IBearerInfo(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*GetNetwork)(iname* _pif, ds_Net_IfaceNameType* value); \
   AEEResult (*GetCDMATypeMask)(iname* _pif, unsigned int* value); \
   AEEResult (*GetCDMAServiceOptionsMask)(iname* _pif, unsigned int* value); \
   AEEResult (*GetUMTSTypeMask)(iname* _pif, unsigned int* value); \
   AEEResult (*GetRate)(iname* _pif, ds_Net_BearerTechRateType* value); \
   AEEResult (*GetBearerIsNull)(iname* _pif, boolean* value)
AEEINTERFACE_DEFINE(ds_Net_IBearerInfo);

/** @memberof ds_Net_IBearerInfo
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IBearerInfo_AddRef(ds_Net_IBearerInfo* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IBearerInfo)->AddRef(_pif);
}

/** @memberof ds_Net_IBearerInfo
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IBearerInfo_Release(ds_Net_IBearerInfo* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IBearerInfo)->Release(_pif);
}

/** @memberof ds_Net_IBearerInfo
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IBearerInfo_QueryInterface(ds_Net_IBearerInfo* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IBearerInfo)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IBearerInfo
  * 
  * This attribute indicates the network type of the data connection (bearer)
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IBearerInfo_GetNetwork(ds_Net_IBearerInfo* _pif, ds_Net_IfaceNameType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IBearerInfo)->GetNetwork(_pif, value);
}

/** @memberof ds_Net_IBearerInfo
  * 
  * This attribute indicates the CDMA sub technologies of the bearer.
  * It is relevant if the bearer Network type is CDMA.
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @see CDMA sub-technologies.
  */
static __inline AEEResult ds_Net_IBearerInfo_GetCDMATypeMask(ds_Net_IBearerInfo* _pif, unsigned int* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IBearerInfo)->GetCDMATypeMask(_pif, value);
}

/** @memberof ds_Net_IBearerInfo
  * 
  * This attribute indicates the CDMA service options of the bearer.
  * It is relevant if the bearer Network type is CDMA.
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @see CDMA service options.
  */
static __inline AEEResult ds_Net_IBearerInfo_GetCDMAServiceOptionsMask(ds_Net_IBearerInfo* _pif, unsigned int* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IBearerInfo)->GetCDMAServiceOptionsMask(_pif, value);
}

/** @memberof ds_Net_IBearerInfo
  * 
  * This attribute indicates the UMTS sub technologies of the bearer.
  * It is relevant if the bearer Network type is UMTS.
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @see UMTS sub-technologies.
  */
static __inline AEEResult ds_Net_IBearerInfo_GetUMTSTypeMask(ds_Net_IBearerInfo* _pif, unsigned int* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IBearerInfo)->GetUMTSTypeMask(_pif, value);
}

/** @memberof ds_Net_IBearerInfo
  * 
  * This attribute represents the bearer technology rate for this network.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IBearerInfo_GetRate(ds_Net_IBearerInfo* _pif, ds_Net_BearerTechRateType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IBearerInfo)->GetRate(_pif, value);
}

/** @memberof ds_Net_IBearerInfo
  * 
  * Indicates if the bearer (subtechnology) is the null bearer 
  * value for the technology described by this instance.
  * Bearer shall be NULL if there is currently no active Network
  * connection.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IBearerInfo_GetBearerIsNull(ds_Net_IBearerInfo* _pif, boolean* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IBearerInfo)->GetBearerIsNull(_pif, value);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Net_Def.h"
namespace ds
{
   namespace Net
   {
      namespace BearerTechCDMA
      {
         
         /**
           * CDMA sub-technologies. 
           * Since a mobile device may simultaneously support more than one
           * technology, the following may be ORed together to indicate all the
           * technologies supported by the bearer.
           */
         const unsigned int SUBTECH_1X = 0x1;
         const unsigned int SUBTECH_EVDO_REV0 = 0x2;
         const unsigned int SUBTECH_EVDO_REVA = 0x4;
         const unsigned int SUBTECH_EVDO_REVB = 0x8;
         
         /**
           * SUBTECH_NULL shall be indicated if the application queries for the
           * the bearer technology but there is currently no active Network 
           * connection.
           */
         const unsigned int SUBTECH_NULL = 0x8000;
         
         /**
           * CDMA 1x specific service options.
           * Since a mobile device may simultaneously support more than one
           * service options, the following may be ORed together to indicate all
           * the service options supported by the bearer.
           */
         const unsigned int SO_1X_IS95 = 0x1;
         const unsigned int SO_1X_IS2000 = 0x2;
         const unsigned int SO_1X_IS2000_REL_A = 0x4;
         
         /**
           * CDMA EVDO (HDR) specific service options. Packet Applications are
           * not dependent on Rev0/RevA/RevB, therefore the constants are reused
           * for all revisions.
           * Since a mobile device may simultaneously support more than one
           * service options, the following may be ORed together to indicate all
           * the service options supported by the bearer.
           */
         const unsigned int SO_EVDO_DPA = 0x1;
         const unsigned int SO_EVDO_MFPA = 0x2;
         const unsigned int SO_EVDO_EMPA = 0x4;
         const unsigned int SO_EVDO_EMPA_EHRPD = 0x8;
         const unsigned int SO_EVDO_MMPA = 0x10;
         const unsigned int SO_EVDO_MMPA_EHRPD = 0x20;
      };
      namespace BearerTechUMTS
      {
         
         /**
           * UMTS sub-technologies. 
           * Since a mobile device may simultaneously support more than one
           * technology, the following may be ORed together to indicate all the
           * technologies supported by the bearer.
           * For example, in WCDMA the WCDMA subtechnology is set; in addition,
           * HSUPA and HSDPA flags may be set when the respective subtechnologies
           * are available.
           */
         const unsigned int SUBTECH_WCDMA = 0x1;
         const unsigned int SUBTECH_GPRS = 0x2;
         const unsigned int SUBTECH_HSDPA = 0x4;
         const unsigned int SUBTECH_HSUPA = 0x8;
         const unsigned int SUBTECH_EDGE = 0x10;
         const unsigned int SUBTECH_LTE = 0x20;
         
         /**
           * HSDPA+ bearer
           */
         const unsigned int SUBTECH_HSDPAPLUS = 0x40;
         
         /**
           * Dual Carrier HSDPA+ bearer
           */
         const unsigned int SUBTECH_DC_HSDPAPLUS = 0x80;
         
         /**
           * SUBTECH_NULL shall be indicated if the application queries for the
           * the bearer technology but there is currently no active Network 
           * connection.
           */
         const unsigned int SUBTECH_NULL = 0x8000;
      };
      
      /** Data bearer rate definitions */
      struct BearerTechRateType {
         int maxTxDataRate; /**< Max Tx bearer data rate */
         int maxRxDataRate; /**< Max Rx bearer data rate */
         int avgTxDataRate; /**< Average Tx bearer data rate */
         int avgRxDataRate; /**< Average Rx bearer data rate */
         int currentTxDataRate; /**< Current Tx bearer data rate */
         int currentRxDataRate; /**< Current Rx bearer data rate */
      };
      const ::AEEIID AEEIID_IBearerInfo = 0x106c6a5;
      struct IBearerInfo : public ::IQI
      {
         
         /**
           * This attribute indicates the network type of the data connection (bearer)
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetNetwork(::ds::Net::IfaceNameType* value) = 0;
         
         /**
           * This attribute indicates the CDMA sub technologies of the bearer.
           * It is relevant if the bearer Network type is CDMA.
           * @param value Attribute value
           * @see CDMA sub-technologies.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetCDMATypeMask(unsigned int* value) = 0;
         
         /**
           * This attribute indicates the CDMA service options of the bearer.
           * It is relevant if the bearer Network type is CDMA.
           * @param value Attribute value
           * @see CDMA service options.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetCDMAServiceOptionsMask(unsigned int* value) = 0;
         
         /**
           * This attribute indicates the UMTS sub technologies of the bearer.
           * It is relevant if the bearer Network type is UMTS.
           * @param value Attribute value
           * @see UMTS sub-technologies.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetUMTSTypeMask(unsigned int* value) = 0;
         
         /**
           * This attribute represents the bearer technology rate for this network.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetRate(::ds::Net::BearerTechRateType* value) = 0;
         
         /**
           * Indicates if the bearer (subtechnology) is the null bearer 
           * value for the technology described by this instance.
           * Bearer shall be NULL if there is currently no active Network
           * connection.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetBearerIsNull(boolean* value) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IBEARERINFO_H
