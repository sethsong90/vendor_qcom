#ifndef DS_NET_IQOSDEFAULT_H
#define DS_NET_IQOSDEFAULT_H

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
#include "ds_Net_Def.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_QoS_Def.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_IQoS.h"
#define ds_Net_QoSDefaultEvent_QDS_EV_MODIFIED 0x106dee7
#define ds_Net_AEEIID_IQoSDefault 0x106d74d

/** @interface ds_Net_IQoSDefault
  * 
  * ds Net QoS Default interface.
  * Events that can be registered on this interface via OnStateChange (as part of IEventManager interface): 
  * - ds_Net_QoSDefaultEvent_MODIFIED. Use GetGrantedFlowSpec to fetch the current flow specification.
  */
#define INHERIT_ds_Net_IQoSDefault(iname) \
   INHERIT_ds_Net_IQoS(iname); \
   AEEResult (*Modify)(iname* _pif, const char* requestedSpec, ds_Net_QoSModifyMaskType mask, char* errSpec, int errSpecLen, int* errSpecLenReq)
AEEINTERFACE_DEFINE(ds_Net_IQoSDefault);

/** @memberof ds_Net_IQoSDefault
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IQoSDefault_AddRef(ds_Net_IQoSDefault* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSDefault)->AddRef(_pif);
}

/** @memberof ds_Net_IQoSDefault
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IQoSDefault_Release(ds_Net_IQoSDefault* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSDefault)->Release(_pif);
}

/** @memberof ds_Net_IQoSDefault
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IQoSDefault_QueryInterface(ds_Net_IQoSDefault* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSDefault)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IQoSDefault
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
static __inline AEEResult ds_Net_IQoSDefault_OnStateChange(ds_Net_IQoSDefault* _pif, ISignal* signal, ds_Net_EventType eventID, IQI** regObj)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSDefault)->OnStateChange(_pif, signal, eventID, regObj);
}

/** @memberof ds_Net_IQoSDefault
  * 
  * This function returns the current Rx and Tx flow specification.
  * @param _pif Pointer to interface
  * @param rxFlow the current Rx flow specification.
  *               See ds_Net_QoSFlow.json schema.
  * @param rxFlowLen Length of sequence
  * @param rxFlowLenReq Required length of sequence
  * @param txFlow the current Tx flow specification.
  *               See ds_Net_QoSFlow.json schema.
  * @param txFlowLen Length of sequence
  * @param txFlowLenReq Required length of sequence
  * @retval ds_SUCCESS Request received successfully.
  * @retval ds_EBADOPTS Bad options.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IQoSDefault_GetGrantedFlowSpec(ds_Net_IQoSDefault* _pif, char* rxFlow, int rxFlowLen, int* rxFlowLenReq, char* txFlow, int txFlowLen, int* txFlowLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSDefault)->GetGrantedFlowSpec(_pif, rxFlow, rxFlowLen, rxFlowLenReq, txFlow, txFlowLen, txFlowLenReq);
}

/** @memberof ds_Net_IQoSDefault
  * 
  * This attribute provides the Tx physical link object.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IQoSDefault_GetTXPhysLink(ds_Net_IQoSDefault* _pif, ds_Net_IPhysLink** value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSDefault)->GetTXPhysLink(_pif, value);
}

/** @memberof ds_Net_IQoSDefault
  * 
  * This attribute provides the Rx physical link object.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IQoSDefault_GetRXPhysLink(ds_Net_IQoSDefault* _pif, ds_Net_IPhysLink** value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSDefault)->GetRXPhysLink(_pif, value);
}

/** @memberof ds_Net_IQoSDefault
  * 
  * This attribute can be used to fetch the result of last 
  * IQoSSecondary_Modify or IQoSDefault_Modify operation when a
  * ds_Net_QoS_Event_MODIFY_RESULT
  * event is received.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @See ds_Net_QoS_ResultType.
  */
static __inline AEEResult ds_Net_IQoSDefault_GetModifyResult(ds_Net_IQoSDefault* _pif, ds_Net_QoSResultType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSDefault)->GetModifyResult(_pif, value);
}

/** @memberof ds_Net_IQoSDefault
  *     
  * This attribute can be used to fetch updated infoCode information
  * for the QoS flow when a ds_Net_QoS_Event_INFO_CODE_UPDATED
  * event is received.
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @See ds_Net_QoS_InfoCodeType.
  */
static __inline AEEResult ds_Net_IQoSDefault_GetUpdatedInfoCode(ds_Net_IQoSDefault* _pif, ds_Net_QoSInfoCodeType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSDefault)->GetUpdatedInfoCode(_pif, value);
}

/** @memberof ds_Net_IQoSDefault
  * 
  * This attribute represents the TX queue level.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IQoSDefault_GetTXQueueLevel(ds_Net_IQoSDefault* _pif, ds_Net_QoSTXQueueLevelType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSDefault)->GetTXQueueLevel(_pif, value);
}

/** @memberof ds_Net_IQoSDefault
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
static __inline AEEResult ds_Net_IQoSDefault_GetTechObject(ds_Net_IQoSDefault* _pif, AEEIID techObj_iid, void** techObj)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSDefault)->GetTechObject(_pif, techObj_iid, techObj);
}

/** @memberof ds_Net_IQoSDefault
  * 
  * This function issues a request to modify the default QoS Flow
  * specification in either Rx direction, Tx direction or both directions.
  * Note that this is a privileged operation.
  * @param _pif Pointer to interface
  * @param requestedSpec QoS specification for modifying the default QoS
  *                      instance (default QoS).
  *                      See ds_Net_QoSSpec.json schema.
  *                      Depending on the technology, filters may or may
  *                      not be applicable to Default QoS. If filters
  *                      are not applicable, any filters specification
  *                      in this parameter shall be ignored.
  * @param mask The modification direction, can be RX_FLOW, TX_FLOW or
  *             RX_FLOW|TX_FLOW for modification of both directions.
  * @param errSpec specification of flow options and filter options 
  *                for which invalid values were specified in
  *                requestedSpec.
  *                See ds_Net_QoSSpecErr.json schema.          
  * @param errSpecLen Length of sequence
  * @param errSpecLenReq Required length of sequence
  * @retval ds_SUCCESS Modification request received successfully.
  * @retval ds_EBADOPTS Bad options.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IQoSDefault_Modify(ds_Net_IQoSDefault* _pif, const char* requestedSpec, ds_Net_QoSModifyMaskType mask, char* errSpec, int errSpecLen, int* errSpecLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSDefault)->Modify(_pif, requestedSpec, mask, errSpec, errSpecLen, errSpecLenReq);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_Def.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_QoS_Def.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_IQoS.h"
namespace ds
{
   namespace Net
   {
      namespace QoSDefaultEvent
      {
         const ::ds::Net::EventType QDS_EV_MODIFIED = 0x106dee7;
      };
      const ::AEEIID AEEIID_IQoSDefault = 0x106d74d;
      
      /** @interface IQoSDefault
        * 
        * ds Net QoS Default interface.
        * Events that can be registered on this interface via OnStateChange (as part of IEventManager interface): 
        * - ds::Net::QoSDefaultEvent::MODIFIED. Use GetGrantedFlowSpec to fetch the current flow specification.
        */
      struct IQoSDefault : public ::ds::Net::IQoS
      {
         
         /**
           * This function issues a request to modify the default QoS Flow
           * specification in either Rx direction, Tx direction or both directions.
           * Note that this is a privileged operation.
           * @param requestedSpec QoS specification for modifying the default QoS
           *                      instance (default QoS).
           *                      See ds_Net_QoSSpec.json schema.
           *                      Depending on the technology, filters may or may
           *                      not be applicable to Default QoS. If filters
           *                      are not applicable, any filters specification
           *                      in this parameter shall be ignored.
           * @param mask The modification direction, can be RX_FLOW, TX_FLOW or
           *             RX_FLOW|TX_FLOW for modification of both directions.
           * @param errSpec specification of flow options and filter options 
           *                for which invalid values were specified in
           *                requestedSpec.
           *                See ds_Net_QoSSpecErr.json schema.          
           * @retval ds::SUCCESS Modification request received successfully.
           * @retval ds::EBADOPTS Bad options.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Modify(const char* requestedSpec, ::ds::Net::QoSModifyMaskType mask, char* errSpec, int errSpecLen, int* errSpecLenReq) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IQOSDEFAULT_H
