#ifndef DS_NET_IQOSFLOWPRIV_H
#define DS_NET_IQOSFLOWPRIV_H

/*============================================================================
  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "AEEIQI.h"
typedef unsigned int ds_Net_QoSFlowOptIDType;
#define ds_Net_QoSFlowOptID_QDS_IP_TRAFFIC_CLASS 0x1
#define ds_Net_QoSFlowOptID_QDS_DATA_RATE_TOKEN_BUCKET 0x2
#define ds_Net_QoSFlowOptID_QDS_LATENCY 0x4
#define ds_Net_QoSFlowOptID_QDS_LATENCY_VARIANCE 0x8
#define ds_Net_QoSFlowOptID_QDS_PACKET_ERROR_RATE 0x10
#define ds_Net_QoSFlowOptID_QDS_MIN_POLICED_PACKET_SIZE 0x20
#define ds_Net_QoSFlowOptID_QDS_MAX_ALLOWED_PACKET_SIZE 0x40
#define ds_Net_QoSFlowOptID_QDS_UMTS_RESIDUAL_BIT_ERROR_RATE 0x80
#define ds_Net_QoSFlowOptID_QDS_UMTS_TRAFFIC_PRIORITY 0x100
#define ds_Net_QoSFlowOptID_QDS_CDMA_PROFILE_ID 0x200
#define ds_Net_QoSFlowOptID_QDS_WLAN_USER_PRI 0x400
#define ds_Net_QoSFlowOptID_QDS_WLAN_MIN_SERVICE_INTERVAL 0x800
#define ds_Net_QoSFlowOptID_QDS_WLAN_MAX_SERVICE_INTERVAL 0x1000
#define ds_Net_QoSFlowOptID_QDS_WLAN_INACTIVITY_INTERVAL 0x2000
#define ds_Net_QoSFlowOptID_QDS_NOMINAL_SDU_SIZE 0x4000
#define ds_Net_QoSFlowOptID_QDS_CDMA_FLOW_PRI 0x8000
#define ds_Net_QoSFlowOptID_QDS_UMTS_IMS_SIGNALING_CONTEXT 0x10000
#define ds_Net_QoSFlowOptID_QDS_UMTS_HIGH_PRIORITY_DATA 0x20000
#define ds_Net_QoSFlowOptID_QDS_DATA_RATE_MIN_MAX 0x40000
struct ds_Net__SeqQoSFlowOptIDType__seq_unsignedLong {
   ds_Net_QoSFlowOptIDType* data;
   int dataLen;
   int dataLenReq;
};
typedef struct ds_Net__SeqQoSFlowOptIDType__seq_unsignedLong ds_Net__SeqQoSFlowOptIDType__seq_unsignedLong;
typedef ds_Net__SeqQoSFlowOptIDType__seq_unsignedLong ds_Net_SeqQoSFlowOptIDType;
typedef int ds_Net_QoSFlowOptIPTrafficClassType;
#define ds_Net_QoSFlowOptIPTrafficClass_QDS_CONVERSATIONAL 0
#define ds_Net_QoSFlowOptIPTrafficClass_QDS_STREAMING 1
#define ds_Net_QoSFlowOptIPTrafficClass_QDS_INTERACTIVE 2
#define ds_Net_QoSFlowOptIPTrafficClass_QDS_BACKGROUND 3
typedef int ds_Net_QoSFlowOptLatencyType;
typedef int ds_Net_QoSFlowOptLatencyVarianceType;
typedef int ds_Net_QoSFlowOptMinPolicedPacketSizeType;
typedef int ds_Net_QoSFlowOptMaxAllowedPacketSizeType;
typedef int ds_Net_QoSFlowOptUMTSResidualBitErrorRateType;
#define ds_Net_QoSFlowOptUMTSResidualBitErrorRate_QDS_RATE1 0
#define ds_Net_QoSFlowOptUMTSResidualBitErrorRate_QDS_RATE2 1
#define ds_Net_QoSFlowOptUMTSResidualBitErrorRate_QDS_RATE3 2
#define ds_Net_QoSFlowOptUMTSResidualBitErrorRate_QDS_RATE4 3
#define ds_Net_QoSFlowOptUMTSResidualBitErrorRate_QDS_RATE5 4
#define ds_Net_QoSFlowOptUMTSResidualBitErrorRate_QDS_RATE6 5
#define ds_Net_QoSFlowOptUMTSResidualBitErrorRate_QDS_RATE7 6
#define ds_Net_QoSFlowOptUMTSResidualBitErrorRate_QDS_RATE8 7
#define ds_Net_QoSFlowOptUMTSResidualBitErrorRate_QDS_RATE9 8
#define ds_Net_QoSFlowOptUMTSResidualBitErrorRate_QDS_RATE10 9
typedef int ds_Net_QoSFlowOptUMTSTrafficPriorityType;
#define ds_Net_QoSFlowOptUMTSTrafficPriority_QDS_PRI1 0
#define ds_Net_QoSFlowOptUMTSTrafficPriority_QDS_PRI2 1
#define ds_Net_QoSFlowOptUMTSTrafficPriority_QDS_PRI3 2
typedef unsigned short ds_Net_QoSFlowOptCDMAProfileIDType;
typedef unsigned char ds_Net_QoSFlowOptCDMAFlowPriorityType;
typedef int ds_Net_QoSFlowOptWLANUserPriorityType;
#define ds_Net_QoSFlowOptWLANUserPriority_QDS_BEST_EFFORT 0
#define ds_Net_QoSFlowOptWLANUserPriority_QDS_BACKGROUND 1
#define ds_Net_QoSFlowOptWLANUserPriority_QDS_RESERVED 2
#define ds_Net_QoSFlowOptWLANUserPriority_QDS_EXCELLENT_EFFORT 3
#define ds_Net_QoSFlowOptWLANUserPriority_QDS_CONTROLLED_LOAD 4
#define ds_Net_QoSFlowOptWLANUserPriority_QDS_VIDEO 5
#define ds_Net_QoSFlowOptWLANUserPriority_QDS_VOICE 6
#define ds_Net_QoSFlowOptWLANUserPriority_QDS_NETWORK_CONTROL 7
typedef int ds_Net_QoSFlowOptWLANMinServiceIntervalType;
typedef int ds_Net_QoSFlowOptWLANMaxServiceIntervalType;
typedef int ds_Net_QoSFlowOptWLANInactivityIntervalType;
typedef boolean ds_Net_QoSFlowOptUMTSImsSignalingContextType;
typedef boolean ds_Net_QoSFlowOptUMTSHighPriorityDataType;
struct ds_Net_QoSFlowOptDataRateMinMaxType {
   int maxRate;
   int guaranteedRate;
};
typedef struct ds_Net_QoSFlowOptDataRateMinMaxType ds_Net_QoSFlowOptDataRateMinMaxType;
struct ds_Net_QoSFlowOptDataRateTokenBucketType {
   int peakRate;
   int tokenRate;
   int size;
};
typedef struct ds_Net_QoSFlowOptDataRateTokenBucketType ds_Net_QoSFlowOptDataRateTokenBucketType;
struct ds_Net_QoSFlowOptPacketErrorRateType {
   unsigned short multiplier;
   unsigned short exponent;
};
typedef struct ds_Net_QoSFlowOptPacketErrorRateType ds_Net_QoSFlowOptPacketErrorRateType;
struct ds_Net_QoSFlowOptNominalSDUSizeType {
   boolean fixed;
   AEEINTERFACE_PADMEMBERS(size, 3)
   int size;
};
typedef struct ds_Net_QoSFlowOptNominalSDUSizeType ds_Net_QoSFlowOptNominalSDUSizeType;
#define ds_Net_AEEIID_IQoSFlowPriv 0x106cd47

/** @interface ds_Net_IQoSFlowPriv
  * 
  * ds Net QoS Flow interface.
  */
#define INHERIT_ds_Net_IQoSFlowPriv(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*Clone)(iname* _pif, ds_Net_IQoSFlowPriv** flow); \
   AEEResult (*GetValidOptions)(iname* _pif, ds_Net_QoSFlowOptIDType* value, int valueLen, int* valueLenReq); \
   AEEResult (*GetErroneousOptions)(iname* _pif, ds_Net_QoSFlowOptIDType* value, int valueLen, int* valueLenReq); \
   AEEResult (*GetTrfClass)(iname* _pif, ds_Net_QoSFlowOptIPTrafficClassType* value); \
   AEEResult (*SetTrfClass)(iname* _pif, ds_Net_QoSFlowOptIPTrafficClassType value); \
   AEEResult (*GetLatency)(iname* _pif, ds_Net_QoSFlowOptLatencyType* value); \
   AEEResult (*SetLatency)(iname* _pif, ds_Net_QoSFlowOptLatencyType value); \
   AEEResult (*GetLatencyVar)(iname* _pif, ds_Net_QoSFlowOptLatencyVarianceType* value); \
   AEEResult (*SetLatencyVar)(iname* _pif, ds_Net_QoSFlowOptLatencyVarianceType value); \
   AEEResult (*GetMinPolicedPktSize)(iname* _pif, ds_Net_QoSFlowOptMinPolicedPacketSizeType* value); \
   AEEResult (*SetMinPolicedPktSize)(iname* _pif, ds_Net_QoSFlowOptMinPolicedPacketSizeType value); \
   AEEResult (*GetMaxAllowedPktSize)(iname* _pif, ds_Net_QoSFlowOptMaxAllowedPacketSizeType* value); \
   AEEResult (*SetMaxAllowedPktSize)(iname* _pif, ds_Net_QoSFlowOptMaxAllowedPacketSizeType value); \
   AEEResult (*GetUmtsResBer)(iname* _pif, ds_Net_QoSFlowOptUMTSResidualBitErrorRateType* value); \
   AEEResult (*SetUmtsResBer)(iname* _pif, ds_Net_QoSFlowOptUMTSResidualBitErrorRateType value); \
   AEEResult (*GetUmtsTrfPri)(iname* _pif, ds_Net_QoSFlowOptUMTSTrafficPriorityType* value); \
   AEEResult (*SetUmtsTrfPri)(iname* _pif, ds_Net_QoSFlowOptUMTSTrafficPriorityType value); \
   AEEResult (*GetCdmaProfileID)(iname* _pif, ds_Net_QoSFlowOptCDMAProfileIDType* value); \
   AEEResult (*SetCdmaProfileID)(iname* _pif, ds_Net_QoSFlowOptCDMAProfileIDType value); \
   AEEResult (*GetWlanUserPriority)(iname* _pif, ds_Net_QoSFlowOptWLANUserPriorityType* value); \
   AEEResult (*SetWlanUserPriority)(iname* _pif, ds_Net_QoSFlowOptWLANUserPriorityType value); \
   AEEResult (*GetWlanMinServiceInterval)(iname* _pif, ds_Net_QoSFlowOptWLANMinServiceIntervalType* value); \
   AEEResult (*SetWlanMinServiceInterval)(iname* _pif, ds_Net_QoSFlowOptWLANMinServiceIntervalType value); \
   AEEResult (*GetWlanMaxServiceInterval)(iname* _pif, ds_Net_QoSFlowOptWLANMaxServiceIntervalType* value); \
   AEEResult (*SetWlanMaxServiceInterval)(iname* _pif, ds_Net_QoSFlowOptWLANMaxServiceIntervalType value); \
   AEEResult (*GetWlanInactivityInterval)(iname* _pif, ds_Net_QoSFlowOptWLANInactivityIntervalType* value); \
   AEEResult (*SetWlanInactivityInterval)(iname* _pif, ds_Net_QoSFlowOptWLANInactivityIntervalType value); \
   AEEResult (*GetCdmaFlowPriority)(iname* _pif, ds_Net_QoSFlowOptCDMAFlowPriorityType* value); \
   AEEResult (*SetCdmaFlowPriority)(iname* _pif, ds_Net_QoSFlowOptCDMAFlowPriorityType value); \
   AEEResult (*GetUmtsImCnFlag)(iname* _pif, ds_Net_QoSFlowOptUMTSImsSignalingContextType* value); \
   AEEResult (*SetUmtsImCnFlag)(iname* _pif, ds_Net_QoSFlowOptUMTSImsSignalingContextType value); \
   AEEResult (*GetUmtsSigInd)(iname* _pif, ds_Net_QoSFlowOptUMTSHighPriorityDataType* value); \
   AEEResult (*SetUmtsSigInd)(iname* _pif, ds_Net_QoSFlowOptUMTSHighPriorityDataType value); \
   AEEResult (*GetDataRateMinMax)(iname* _pif, ds_Net_QoSFlowOptDataRateMinMaxType* value); \
   AEEResult (*SetDataRateMinMax)(iname* _pif, const ds_Net_QoSFlowOptDataRateMinMaxType* value); \
   AEEResult (*GetDataRateTokenBucket)(iname* _pif, ds_Net_QoSFlowOptDataRateTokenBucketType* value); \
   AEEResult (*SetDataRateTokenBucket)(iname* _pif, const ds_Net_QoSFlowOptDataRateTokenBucketType* value); \
   AEEResult (*GetPktErrRate)(iname* _pif, ds_Net_QoSFlowOptPacketErrorRateType* value); \
   AEEResult (*SetPktErrRate)(iname* _pif, const ds_Net_QoSFlowOptPacketErrorRateType* value); \
   AEEResult (*GetNominalSDUSize)(iname* _pif, ds_Net_QoSFlowOptNominalSDUSizeType* value); \
   AEEResult (*SetNominalSDUSize)(iname* _pif, const ds_Net_QoSFlowOptNominalSDUSizeType* value)
AEEINTERFACE_DEFINE(ds_Net_IQoSFlowPriv);

/** @memberof ds_Net_IQoSFlowPriv
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IQoSFlowPriv_AddRef(ds_Net_IQoSFlowPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->AddRef(_pif);
}

/** @memberof ds_Net_IQoSFlowPriv
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IQoSFlowPriv_Release(ds_Net_IQoSFlowPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->Release(_pif);
}

/** @memberof ds_Net_IQoSFlowPriv
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IQoSFlowPriv_QueryInterface(ds_Net_IQoSFlowPriv* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IQoSFlowPriv
  * 
  * This function creates an identical copy of the IQoSFlowPriv.
  * @param _pif Pointer to interface
  * @param flow The created IQoSFlowPriv.
  * @retval ds_SUCCESS IQoSFlowPriv cloned successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IQoSFlowPriv_Clone(ds_Net_IQoSFlowPriv* _pif, ds_Net_IQoSFlowPriv** flow)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->Clone(_pif, flow);
}

/** @memberof ds_Net_IQoSFlowPriv
  * 
  * This attribute represents the valid options - if an option was set,
  * its ID will be in this list.
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @param valueLen Length of sequence
  * @param valueLenReq Required length of sequence
  */
static __inline AEEResult ds_Net_IQoSFlowPriv_GetValidOptions(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptIDType* value, int valueLen, int* valueLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->GetValidOptions(_pif, value, valueLen, valueLenReq);
}

/** @memberof ds_Net_IQoSFlowPriv
  * 
  * This attribute represents a list of erroneous options into
  * the IQoSFlowPriv object.
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @param valueLen Length of sequence
  * @param valueLenReq Required length of sequence
  */
static __inline AEEResult ds_Net_IQoSFlowPriv_GetErroneousOptions(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptIDType* value, int valueLen, int* valueLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->GetErroneousOptions(_pif, value, valueLen, valueLenReq);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_GetTrfClass(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptIPTrafficClassType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->GetTrfClass(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_SetTrfClass(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptIPTrafficClassType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->SetTrfClass(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_GetLatency(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptLatencyType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->GetLatency(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_SetLatency(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptLatencyType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->SetLatency(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_GetLatencyVar(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptLatencyVarianceType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->GetLatencyVar(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_SetLatencyVar(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptLatencyVarianceType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->SetLatencyVar(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_GetMinPolicedPktSize(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptMinPolicedPacketSizeType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->GetMinPolicedPktSize(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_SetMinPolicedPktSize(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptMinPolicedPacketSizeType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->SetMinPolicedPktSize(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_GetMaxAllowedPktSize(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptMaxAllowedPacketSizeType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->GetMaxAllowedPktSize(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_SetMaxAllowedPktSize(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptMaxAllowedPacketSizeType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->SetMaxAllowedPktSize(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_GetUmtsResBer(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptUMTSResidualBitErrorRateType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->GetUmtsResBer(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_SetUmtsResBer(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptUMTSResidualBitErrorRateType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->SetUmtsResBer(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_GetUmtsTrfPri(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptUMTSTrafficPriorityType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->GetUmtsTrfPri(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_SetUmtsTrfPri(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptUMTSTrafficPriorityType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->SetUmtsTrfPri(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_GetCdmaProfileID(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptCDMAProfileIDType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->GetCdmaProfileID(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_SetCdmaProfileID(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptCDMAProfileIDType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->SetCdmaProfileID(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_GetWlanUserPriority(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptWLANUserPriorityType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->GetWlanUserPriority(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_SetWlanUserPriority(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptWLANUserPriorityType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->SetWlanUserPriority(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_GetWlanMinServiceInterval(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptWLANMinServiceIntervalType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->GetWlanMinServiceInterval(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_SetWlanMinServiceInterval(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptWLANMinServiceIntervalType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->SetWlanMinServiceInterval(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_GetWlanMaxServiceInterval(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptWLANMaxServiceIntervalType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->GetWlanMaxServiceInterval(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_SetWlanMaxServiceInterval(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptWLANMaxServiceIntervalType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->SetWlanMaxServiceInterval(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_GetWlanInactivityInterval(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptWLANInactivityIntervalType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->GetWlanInactivityInterval(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_SetWlanInactivityInterval(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptWLANInactivityIntervalType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->SetWlanInactivityInterval(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_GetCdmaFlowPriority(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptCDMAFlowPriorityType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->GetCdmaFlowPriority(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_SetCdmaFlowPriority(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptCDMAFlowPriorityType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->SetCdmaFlowPriority(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_GetUmtsImCnFlag(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptUMTSImsSignalingContextType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->GetUmtsImCnFlag(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_SetUmtsImCnFlag(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptUMTSImsSignalingContextType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->SetUmtsImCnFlag(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_GetUmtsSigInd(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptUMTSHighPriorityDataType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->GetUmtsSigInd(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_SetUmtsSigInd(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptUMTSHighPriorityDataType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->SetUmtsSigInd(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_GetDataRateMinMax(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptDataRateMinMaxType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->GetDataRateMinMax(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_SetDataRateMinMax(ds_Net_IQoSFlowPriv* _pif, const ds_Net_QoSFlowOptDataRateMinMaxType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->SetDataRateMinMax(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_GetDataRateTokenBucket(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptDataRateTokenBucketType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->GetDataRateTokenBucket(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_SetDataRateTokenBucket(ds_Net_IQoSFlowPriv* _pif, const ds_Net_QoSFlowOptDataRateTokenBucketType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->SetDataRateTokenBucket(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_GetPktErrRate(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptPacketErrorRateType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->GetPktErrRate(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_SetPktErrRate(ds_Net_IQoSFlowPriv* _pif, const ds_Net_QoSFlowOptPacketErrorRateType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->SetPktErrRate(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_GetNominalSDUSize(ds_Net_IQoSFlowPriv* _pif, ds_Net_QoSFlowOptNominalSDUSizeType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->GetNominalSDUSize(_pif, value);
}
static __inline AEEResult ds_Net_IQoSFlowPriv_SetNominalSDUSize(ds_Net_IQoSFlowPriv* _pif, const ds_Net_QoSFlowOptNominalSDUSizeType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSFlowPriv)->SetNominalSDUSize(_pif, value);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
namespace ds
{
   namespace Net
   {
      typedef unsigned int QoSFlowOptIDType;
      namespace QoSFlowOptID
      {
         const ::ds::Net::QoSFlowOptIDType QDS_IP_TRAFFIC_CLASS = 0x1;
         const ::ds::Net::QoSFlowOptIDType QDS_DATA_RATE_TOKEN_BUCKET = 0x2;
         const ::ds::Net::QoSFlowOptIDType QDS_LATENCY = 0x4;
         const ::ds::Net::QoSFlowOptIDType QDS_LATENCY_VARIANCE = 0x8;
         const ::ds::Net::QoSFlowOptIDType QDS_PACKET_ERROR_RATE = 0x10;
         const ::ds::Net::QoSFlowOptIDType QDS_MIN_POLICED_PACKET_SIZE = 0x20;
         const ::ds::Net::QoSFlowOptIDType QDS_MAX_ALLOWED_PACKET_SIZE = 0x40;
         const ::ds::Net::QoSFlowOptIDType QDS_UMTS_RESIDUAL_BIT_ERROR_RATE = 0x80;
         const ::ds::Net::QoSFlowOptIDType QDS_UMTS_TRAFFIC_PRIORITY = 0x100;
         const ::ds::Net::QoSFlowOptIDType QDS_CDMA_PROFILE_ID = 0x200;
         const ::ds::Net::QoSFlowOptIDType QDS_WLAN_USER_PRI = 0x400;
         const ::ds::Net::QoSFlowOptIDType QDS_WLAN_MIN_SERVICE_INTERVAL = 0x800;
         const ::ds::Net::QoSFlowOptIDType QDS_WLAN_MAX_SERVICE_INTERVAL = 0x1000;
         const ::ds::Net::QoSFlowOptIDType QDS_WLAN_INACTIVITY_INTERVAL = 0x2000;
         const ::ds::Net::QoSFlowOptIDType QDS_NOMINAL_SDU_SIZE = 0x4000;
         const ::ds::Net::QoSFlowOptIDType QDS_CDMA_FLOW_PRI = 0x8000;
         const ::ds::Net::QoSFlowOptIDType QDS_UMTS_IMS_SIGNALING_CONTEXT = 0x10000;
         const ::ds::Net::QoSFlowOptIDType QDS_UMTS_HIGH_PRIORITY_DATA = 0x20000;
         const ::ds::Net::QoSFlowOptIDType QDS_DATA_RATE_MIN_MAX = 0x40000;
      };
      struct _SeqQoSFlowOptIDType__seq_unsignedLong {
         QoSFlowOptIDType* data;
         int dataLen;
         int dataLenReq;
      };
      typedef _SeqQoSFlowOptIDType__seq_unsignedLong SeqQoSFlowOptIDType;
      typedef int QoSFlowOptIPTrafficClassType;
      namespace QoSFlowOptIPTrafficClass
      {
         const ::ds::Net::QoSFlowOptIPTrafficClassType QDS_CONVERSATIONAL = 0;
         const ::ds::Net::QoSFlowOptIPTrafficClassType QDS_STREAMING = 1;
         const ::ds::Net::QoSFlowOptIPTrafficClassType QDS_INTERACTIVE = 2;
         const ::ds::Net::QoSFlowOptIPTrafficClassType QDS_BACKGROUND = 3;
      };
      typedef int QoSFlowOptLatencyType;
      typedef int QoSFlowOptLatencyVarianceType;
      typedef int QoSFlowOptMinPolicedPacketSizeType;
      typedef int QoSFlowOptMaxAllowedPacketSizeType;
      typedef int QoSFlowOptUMTSResidualBitErrorRateType;
      namespace QoSFlowOptUMTSResidualBitErrorRate
      {
         const ::ds::Net::QoSFlowOptUMTSResidualBitErrorRateType QDS_RATE1 = 0;
         const ::ds::Net::QoSFlowOptUMTSResidualBitErrorRateType QDS_RATE2 = 1;
         const ::ds::Net::QoSFlowOptUMTSResidualBitErrorRateType QDS_RATE3 = 2;
         const ::ds::Net::QoSFlowOptUMTSResidualBitErrorRateType QDS_RATE4 = 3;
         const ::ds::Net::QoSFlowOptUMTSResidualBitErrorRateType QDS_RATE5 = 4;
         const ::ds::Net::QoSFlowOptUMTSResidualBitErrorRateType QDS_RATE6 = 5;
         const ::ds::Net::QoSFlowOptUMTSResidualBitErrorRateType QDS_RATE7 = 6;
         const ::ds::Net::QoSFlowOptUMTSResidualBitErrorRateType QDS_RATE8 = 7;
         const ::ds::Net::QoSFlowOptUMTSResidualBitErrorRateType QDS_RATE9 = 8;
         const ::ds::Net::QoSFlowOptUMTSResidualBitErrorRateType QDS_RATE10 = 9;
      };
      typedef int QoSFlowOptUMTSTrafficPriorityType;
      namespace QoSFlowOptUMTSTrafficPriority
      {
         const ::ds::Net::QoSFlowOptUMTSTrafficPriorityType QDS_PRI1 = 0;
         const ::ds::Net::QoSFlowOptUMTSTrafficPriorityType QDS_PRI2 = 1;
         const ::ds::Net::QoSFlowOptUMTSTrafficPriorityType QDS_PRI3 = 2;
      };
      typedef unsigned short QoSFlowOptCDMAProfileIDType;
      typedef unsigned char QoSFlowOptCDMAFlowPriorityType;
      typedef int QoSFlowOptWLANUserPriorityType;
      namespace QoSFlowOptWLANUserPriority
      {
         const ::ds::Net::QoSFlowOptWLANUserPriorityType QDS_BEST_EFFORT = 0;
         const ::ds::Net::QoSFlowOptWLANUserPriorityType QDS_BACKGROUND = 1;
         const ::ds::Net::QoSFlowOptWLANUserPriorityType QDS_RESERVED = 2;
         const ::ds::Net::QoSFlowOptWLANUserPriorityType QDS_EXCELLENT_EFFORT = 3;
         const ::ds::Net::QoSFlowOptWLANUserPriorityType QDS_CONTROLLED_LOAD = 4;
         const ::ds::Net::QoSFlowOptWLANUserPriorityType QDS_VIDEO = 5;
         const ::ds::Net::QoSFlowOptWLANUserPriorityType QDS_VOICE = 6;
         const ::ds::Net::QoSFlowOptWLANUserPriorityType QDS_NETWORK_CONTROL = 7;
      };
      typedef int QoSFlowOptWLANMinServiceIntervalType;
      typedef int QoSFlowOptWLANMaxServiceIntervalType;
      typedef int QoSFlowOptWLANInactivityIntervalType;
      typedef boolean QoSFlowOptUMTSImsSignalingContextType;
      typedef boolean QoSFlowOptUMTSHighPriorityDataType;
      struct QoSFlowOptDataRateMinMaxType {
         int maxRate;
         int guaranteedRate;
      };
      struct QoSFlowOptDataRateTokenBucketType {
         int peakRate;
         int tokenRate;
         int size;
      };
      struct QoSFlowOptPacketErrorRateType {
         unsigned short multiplier;
         unsigned short exponent;
      };
      struct QoSFlowOptNominalSDUSizeType {
         boolean fixed;
         AEEINTERFACE_PADMEMBERS(size, 3)
         int size;
      };
      const ::AEEIID AEEIID_IQoSFlowPriv = 0x106cd47;
      
      /** @interface IQoSFlowPriv
        * 
        * ds Net QoS Flow interface.
        */
      struct IQoSFlowPriv : public ::IQI
      {
         
         /**
           * This function creates an identical copy of the IQoSFlowPriv.
           * @param flow The created IQoSFlowPriv.
           * @retval ds::SUCCESS IQoSFlowPriv cloned successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Clone(IQoSFlowPriv** flow) = 0;
         
         /**
           * This attribute represents the valid options - if an option was set,
           * its ID will be in this list.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetValidOptions(::ds::Net::QoSFlowOptIDType* value, int valueLen, int* valueLenReq) = 0;
         
         /**
           * This attribute represents a list of erroneous options into
           * the IQoSFlowPriv object.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetErroneousOptions(::ds::Net::QoSFlowOptIDType* value, int valueLen, int* valueLenReq) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetTrfClass(::ds::Net::QoSFlowOptIPTrafficClassType* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetTrfClass(::ds::Net::QoSFlowOptIPTrafficClassType value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetLatency(::ds::Net::QoSFlowOptLatencyType* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetLatency(::ds::Net::QoSFlowOptLatencyType value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetLatencyVar(::ds::Net::QoSFlowOptLatencyVarianceType* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetLatencyVar(::ds::Net::QoSFlowOptLatencyVarianceType value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetMinPolicedPktSize(::ds::Net::QoSFlowOptMinPolicedPacketSizeType* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetMinPolicedPktSize(::ds::Net::QoSFlowOptMinPolicedPacketSizeType value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetMaxAllowedPktSize(::ds::Net::QoSFlowOptMaxAllowedPacketSizeType* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetMaxAllowedPktSize(::ds::Net::QoSFlowOptMaxAllowedPacketSizeType value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetUmtsResBer(::ds::Net::QoSFlowOptUMTSResidualBitErrorRateType* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetUmtsResBer(::ds::Net::QoSFlowOptUMTSResidualBitErrorRateType value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetUmtsTrfPri(::ds::Net::QoSFlowOptUMTSTrafficPriorityType* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetUmtsTrfPri(::ds::Net::QoSFlowOptUMTSTrafficPriorityType value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetCdmaProfileID(::ds::Net::QoSFlowOptCDMAProfileIDType* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetCdmaProfileID(::ds::Net::QoSFlowOptCDMAProfileIDType value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetWlanUserPriority(::ds::Net::QoSFlowOptWLANUserPriorityType* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetWlanUserPriority(::ds::Net::QoSFlowOptWLANUserPriorityType value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetWlanMinServiceInterval(::ds::Net::QoSFlowOptWLANMinServiceIntervalType* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetWlanMinServiceInterval(::ds::Net::QoSFlowOptWLANMinServiceIntervalType value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetWlanMaxServiceInterval(::ds::Net::QoSFlowOptWLANMaxServiceIntervalType* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetWlanMaxServiceInterval(::ds::Net::QoSFlowOptWLANMaxServiceIntervalType value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetWlanInactivityInterval(::ds::Net::QoSFlowOptWLANInactivityIntervalType* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetWlanInactivityInterval(::ds::Net::QoSFlowOptWLANInactivityIntervalType value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetCdmaFlowPriority(::ds::Net::QoSFlowOptCDMAFlowPriorityType* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetCdmaFlowPriority(::ds::Net::QoSFlowOptCDMAFlowPriorityType value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetUmtsImCnFlag(::ds::Net::QoSFlowOptUMTSImsSignalingContextType* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetUmtsImCnFlag(::ds::Net::QoSFlowOptUMTSImsSignalingContextType value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetUmtsSigInd(::ds::Net::QoSFlowOptUMTSHighPriorityDataType* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetUmtsSigInd(::ds::Net::QoSFlowOptUMTSHighPriorityDataType value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetDataRateMinMax(::ds::Net::QoSFlowOptDataRateMinMaxType* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetDataRateMinMax(const ::ds::Net::QoSFlowOptDataRateMinMaxType* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetDataRateTokenBucket(::ds::Net::QoSFlowOptDataRateTokenBucketType* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetDataRateTokenBucket(const ::ds::Net::QoSFlowOptDataRateTokenBucketType* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetPktErrRate(::ds::Net::QoSFlowOptPacketErrorRateType* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetPktErrRate(const ::ds::Net::QoSFlowOptPacketErrorRateType* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetNominalSDUSize(::ds::Net::QoSFlowOptNominalSDUSizeType* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetNominalSDUSize(const ::ds::Net::QoSFlowOptNominalSDUSizeType* value) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IQOSFLOWPRIV_H
