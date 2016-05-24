#ifndef DS_NET_IQOSSECONDARY_H
#define DS_NET_IQOSSECONDARY_H

/*============================================================================
  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "ds_Net_Def.h"
#include "ds_Net_QoS_Def.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_IQoS.h"
#define ds_Net_QoSSecondaryEvent_QDS_EV_STATE_CHANGED 0x106dee4
typedef int ds_Net_QoSSecondaryStateType;
#define ds_Net_QoSSecondaryState_QDS_AVAILABLE_MODIFIED 1
#define ds_Net_QoSSecondaryState_QDS_SUSPENDING 2
#define ds_Net_QoSSecondaryState_QDS_UNAVAILABLE 3
#define ds_Net_QoSSecondaryState_QDS_INVALID 4
#define ds_Net_QoSSecondaryState_QDS_ACTIVATING 5
#define ds_Net_QoSSecondaryState_QDS_SUSPENDED 6
#define ds_Net_QoSSecondaryState_QDS_RELEASING 7
#define ds_Net_QoSSecondaryState_QDS_CONFIGURING 8
struct ds_Net_QoSSecondaryStateInfoType {
   ds_Net_QoSSecondaryStateType state;
   ds_Net_QoSInfoCodeType infoCode;
};
typedef struct ds_Net_QoSSecondaryStateInfoType ds_Net_QoSSecondaryStateInfoType;
#define ds_Net_AEEIID_IQoSSecondary 0x106cd45

/** @interface ds_Net_IQoSSecondary
  * 
  * ds QoS Secondary interface.
  * Events that can be registered on this interface via OnStateChange(as part of IEventManager interface):
  * - ds_Net_QoSSecondaryEvent_STATE_CHANGED. Use GetState to fetch the current state of the QoS session.
  */
#define INHERIT_ds_Net_IQoSSecondary(iname) \
   INHERIT_ds_Net_IQoS(iname); \
   AEEResult (*Close)(iname* _pif); \
   AEEResult (*Modify)(iname* _pif, const char* requestedSpec, ds_Net_QoSModifyMaskType modifyMask, char* errSpec, int errSpecLen, int* errSpecLenReq); \
   AEEResult (*Resume)(iname* _pif); \
   AEEResult (*Suspend)(iname* _pif); \
   AEEResult (*GetState)(iname* _pif, ds_Net_QoSSecondaryStateInfoType* value)
AEEINTERFACE_DEFINE(ds_Net_IQoSSecondary);

/** @memberof ds_Net_IQoSSecondary
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IQoSSecondary_AddRef(ds_Net_IQoSSecondary* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondary)->AddRef(_pif);
}

/** @memberof ds_Net_IQoSSecondary
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IQoSSecondary_Release(ds_Net_IQoSSecondary* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondary)->Release(_pif);
}

/** @memberof ds_Net_IQoSSecondary
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IQoSSecondary_QueryInterface(ds_Net_IQoSSecondary* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondary)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IQoSSecondary
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
static __inline AEEResult ds_Net_IQoSSecondary_OnStateChange(ds_Net_IQoSSecondary* _pif, ISignal* signal, ds_Net_EventType eventID, IQI** regObj)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondary)->OnStateChange(_pif, signal, eventID, regObj);
}

/** @memberof ds_Net_IQoSSecondary
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
static __inline AEEResult ds_Net_IQoSSecondary_GetGrantedFlowSpec(ds_Net_IQoSSecondary* _pif, char* rxFlow, int rxFlowLen, int* rxFlowLenReq, char* txFlow, int txFlowLen, int* txFlowLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondary)->GetGrantedFlowSpec(_pif, rxFlow, rxFlowLen, rxFlowLenReq, txFlow, txFlowLen, txFlowLenReq);
}

/** @memberof ds_Net_IQoSSecondary
  * 
  * This attribute provides the Tx physical link object.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IQoSSecondary_GetTXPhysLink(ds_Net_IQoSSecondary* _pif, ds_Net_IPhysLink** value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondary)->GetTXPhysLink(_pif, value);
}

/** @memberof ds_Net_IQoSSecondary
  * 
  * This attribute provides the Rx physical link object.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IQoSSecondary_GetRXPhysLink(ds_Net_IQoSSecondary* _pif, ds_Net_IPhysLink** value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondary)->GetRXPhysLink(_pif, value);
}

/** @memberof ds_Net_IQoSSecondary
  * 
  * This attribute can be used to fetch the result of last 
  * IQoSSecondary_Modify or IQoSDefault_Modify operation when a
  * ds_Net_QoS_Event_MODIFY_RESULT
  * event is received.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @See ds_Net_QoS_ResultType.
  */
static __inline AEEResult ds_Net_IQoSSecondary_GetModifyResult(ds_Net_IQoSSecondary* _pif, ds_Net_QoSResultType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondary)->GetModifyResult(_pif, value);
}

/** @memberof ds_Net_IQoSSecondary
  *     
  * This attribute can be used to fetch updated infoCode information
  * for the QoS flow when a ds_Net_QoS_Event_INFO_CODE_UPDATED
  * event is received.
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @See ds_Net_QoS_InfoCodeType.
  */
static __inline AEEResult ds_Net_IQoSSecondary_GetUpdatedInfoCode(ds_Net_IQoSSecondary* _pif, ds_Net_QoSInfoCodeType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondary)->GetUpdatedInfoCode(_pif, value);
}

/** @memberof ds_Net_IQoSSecondary
  * 
  * This attribute represents the TX queue level.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IQoSSecondary_GetTXQueueLevel(ds_Net_IQoSSecondary* _pif, ds_Net_QoSTXQueueLevelType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondary)->GetTXQueueLevel(_pif, value);
}

/** @memberof ds_Net_IQoSSecondary
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
static __inline AEEResult ds_Net_IQoSSecondary_GetTechObject(ds_Net_IQoSSecondary* _pif, AEEIID techObj_iid, void** techObj)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondary)->GetTechObject(_pif, techObj_iid, techObj);
}

/** @memberof ds_Net_IQoSSecondary
  * 
  * This function issues a request for closing a QoS Session. The request is 
  * forwarded to lower layers in order to release the QoS link resources.
  * @param _pif Pointer to interface
  * @retval ds_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IQoSSecondary_Close(ds_Net_IQoSSecondary* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondary)->Close(_pif);
}

/** @memberof ds_Net_IQoSSecondary
  * 
  * This function issues the following modification requests:
  *  - Modify an existing QoS Session either in Rx, Tx or both directions.
  *  - Add a new QoS in a specific direction.
  *  - Remove a granted QoS in a specific direction.
  * Modification requests can be performed on Rx blocks, Tx blocks or both.
  * @param _pif Pointer to interface
  * @param requestedSpec Specification for modifying the QoS instance.
  *                      See ds_Net_QoSSpec.json schema.
  * @param modifyMask bit mask which indicates the QoS specification
  *                   blocks (RxFlows, TxFlows, RxFilters, TxFilters) 
  *                   to be modified. For modified blocks, the full
  *                   new specification for the block must be provided
  *                   in requestedSpec (including parts of the
  *                   specification that are not modified from previous
  *                   request. For deleted blocks, the current block
  *                   specification may be omitted from requestedSpec.
  * @param errSpec specification of flow options and filter options 
  *                for which invalid values were specified in
  *                requestedSpec.
  *                See ds_Net_QoSSpecErr.json schema.
  * @param errSpecLen Length of sequence
  * @param errSpecLenReq Required length of sequence
  * @retval ds_SUCCESS Request received successfully.
  * @retval ds_EBADOPTS Bad options.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IQoSSecondary_Modify(ds_Net_IQoSSecondary* _pif, const char* requestedSpec, ds_Net_QoSModifyMaskType modifyMask, char* errSpec, int errSpecLen, int* errSpecLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondary)->Modify(_pif, requestedSpec, modifyMask, errSpec, errSpecLen, errSpecLenReq);
}

/** @memberof ds_Net_IQoSSecondary
  * 
  * This function is used in order to activate the QoS link (from dormant mode).
  * @param _pif Pointer to interface
  * @retval ds_SUCCESS The request was successfully issued. This doesn't indicate that the QoS
  *                     Session is already activated.
  */
static __inline AEEResult ds_Net_IQoSSecondary_Resume(ds_Net_IQoSSecondary* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondary)->Resume(_pif);
}

/** @memberof ds_Net_IQoSSecondary
  * 
  * This function is used in order to suspend a granted QoS Session.
  * @param _pif Pointer to interface
  * @retval ds_SUCCESS The request was successfully issued. This doesn't indicate that the QoS
  *                     Session is already suspended.
  */
static __inline AEEResult ds_Net_IQoSSecondary_Suspend(ds_Net_IQoSSecondary* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondary)->Suspend(_pif);
}

/** @memberof ds_Net_IQoSSecondary
  *     
  * This attribute can be used to fetch the current state of the QoS
  * session when a ds_Net_QoSSecondary_EVENT_STATE_CHANGED event
  * is received.
  * It provides the current state of the QoS Session and additional
  * info code.
  * See ds_Net_QoS_StateType and ds_Net_QoS_InfoCodeType.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IQoSSecondary_GetState(ds_Net_IQoSSecondary* _pif, ds_Net_QoSSecondaryStateInfoType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondary)->GetState(_pif, value);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "ds_Net_Def.h"
#include "ds_Net_QoS_Def.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_IQoS.h"
namespace ds
{
   namespace Net
   {
      namespace QoSSecondaryEvent
      {
         const ::ds::Net::EventType QDS_EV_STATE_CHANGED = 0x106dee4;
      };
      typedef int QoSSecondaryStateType;
      namespace QoSSecondaryState
      {
         const ::ds::Net::QoSSecondaryStateType QDS_AVAILABLE_MODIFIED = 1;
         const ::ds::Net::QoSSecondaryStateType QDS_SUSPENDING = 2;
         const ::ds::Net::QoSSecondaryStateType QDS_UNAVAILABLE = 3;
         const ::ds::Net::QoSSecondaryStateType QDS_INVALID = 4;
         const ::ds::Net::QoSSecondaryStateType QDS_ACTIVATING = 5;
         const ::ds::Net::QoSSecondaryStateType QDS_SUSPENDED = 6;
         const ::ds::Net::QoSSecondaryStateType QDS_RELEASING = 7;
         const ::ds::Net::QoSSecondaryStateType QDS_CONFIGURING = 8;
      };
      struct QoSSecondaryStateInfoType {
         QoSSecondaryStateType state;
         QoSInfoCodeType infoCode;
      };
      const ::AEEIID AEEIID_IQoSSecondary = 0x106cd45;
      
      /** @interface IQoSSecondary
        * 
        * ds QoS Secondary interface.
        * Events that can be registered on this interface via OnStateChange(as part of IEventManager interface):
        * - ds::Net::QoSSecondaryEvent::STATE_CHANGED. Use GetState to fetch the current state of the QoS session.
        */
      struct IQoSSecondary : public ::ds::Net::IQoS
      {
         
         /**
           * This function issues a request for closing a QoS Session. The request is 
           * forwarded to lower layers in order to release the QoS link resources.
           * @retval ds::SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Close() = 0;
         
         /**
           * This function issues the following modification requests:
           *  - Modify an existing QoS Session either in Rx, Tx or both directions.
           *  - Add a new QoS in a specific direction.
           *  - Remove a granted QoS in a specific direction.
           * Modification requests can be performed on Rx blocks, Tx blocks or both.
           * @param requestedSpec Specification for modifying the QoS instance.
           *                      See ds_Net_QoSSpec.json schema.
           * @param modifyMask bit mask which indicates the QoS specification
           *                   blocks (RxFlows, TxFlows, RxFilters, TxFilters) 
           *                   to be modified. For modified blocks, the full
           *                   new specification for the block must be provided
           *                   in requestedSpec (including parts of the
           *                   specification that are not modified from previous
           *                   request. For deleted blocks, the current block
           *                   specification may be omitted from requestedSpec.
           * @param errSpec specification of flow options and filter options 
           *                for which invalid values were specified in
           *                requestedSpec.
           *                See ds_Net_QoSSpecErr.json schema.
           * @retval ds::SUCCESS Request received successfully.
           * @retval ds::EBADOPTS Bad options.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Modify(const char* requestedSpec, ::ds::Net::QoSModifyMaskType modifyMask, char* errSpec, int errSpecLen, int* errSpecLenReq) = 0;
         
         /**
           * This function is used in order to activate the QoS link (from dormant mode).
           * @retval ds::SUCCESS The request was successfully issued. This doesn't indicate that the QoS
           *                     Session is already activated.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Resume() = 0;
         
         /**
           * This function is used in order to suspend a granted QoS Session.
           * @retval ds::SUCCESS The request was successfully issued. This doesn't indicate that the QoS
           *                     Session is already suspended.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Suspend() = 0;
         
         /**    
           * This attribute can be used to fetch the current state of the QoS
           * session when a ds::Net::QoSSecondary::EVENT::STATE_CHANGED event
           * is received.
           * It provides the current state of the QoS Session and additional
           * info code.
           * See ds::Net::QoS::StateType and ds::Net::QoS::InfoCodeType.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetState(::ds::Net::QoSSecondaryStateInfoType* value) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IQOSSECONDARY_H
