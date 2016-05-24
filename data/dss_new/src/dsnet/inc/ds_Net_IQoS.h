#ifndef DS_NET_IQOS_H
#define DS_NET_IQOS_H

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
#define ds_Net_AEEIID_IQoS 0x106d74f

/** @interface ds_Net_IQoS
  * 
  * ds Net QoS interface.
  * Events that can be registered on this interface via OnStateChange (as part of IEventManager interface): 
  * - ds_Net_QoSEvent_MODIFY_RESULT. Use GetModifyResult to fetch the result information.
  * - ds_Net_QoSEvent_INFO_CODE_UPDATED. Use GetUpdatedInfoCode to fetch the information.
  */
#define INHERIT_ds_Net_IQoS(iname) \
   INHERIT_ds_Net_IEventManager(iname); \
   AEEResult (*GetGrantedFlowSpec)(iname* _pif, char* rxFlow, int rxFlowLen, int* rxFlowLenReq, char* txFlow, int txFlowLen, int* txFlowLenReq); \
   AEEResult (*GetTXPhysLink)(iname* _pif, ds_Net_IPhysLink** value); \
   AEEResult (*GetRXPhysLink)(iname* _pif, ds_Net_IPhysLink** value); \
   AEEResult (*GetModifyResult)(iname* _pif, ds_Net_QoSResultType* value); \
   AEEResult (*GetUpdatedInfoCode)(iname* _pif, ds_Net_QoSInfoCodeType* value); \
   AEEResult (*GetTXQueueLevel)(iname* _pif, ds_Net_QoSTXQueueLevelType* value); \
   AEEResult (*GetTechObject)(iname* _pif, AEEIID techObj_iid, void** techObj)
AEEINTERFACE_DEFINE(ds_Net_IQoS);

/** @memberof ds_Net_IQoS
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IQoS_AddRef(ds_Net_IQoS* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoS)->AddRef(_pif);
}

/** @memberof ds_Net_IQoS
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IQoS_Release(ds_Net_IQoS* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoS)->Release(_pif);
}

/** @memberof ds_Net_IQoS
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IQoS_QueryInterface(ds_Net_IQoS* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoS)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IQoS
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
static __inline AEEResult ds_Net_IQoS_OnStateChange(ds_Net_IQoS* _pif, ISignal* signal, ds_Net_EventType eventID, IQI** regObj)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoS)->OnStateChange(_pif, signal, eventID, regObj);
}

/** @memberof ds_Net_IQoS
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
static __inline AEEResult ds_Net_IQoS_GetGrantedFlowSpec(ds_Net_IQoS* _pif, char* rxFlow, int rxFlowLen, int* rxFlowLenReq, char* txFlow, int txFlowLen, int* txFlowLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoS)->GetGrantedFlowSpec(_pif, rxFlow, rxFlowLen, rxFlowLenReq, txFlow, txFlowLen, txFlowLenReq);
}

/** @memberof ds_Net_IQoS
  * 
  * This attribute provides the Tx physical link object.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IQoS_GetTXPhysLink(ds_Net_IQoS* _pif, ds_Net_IPhysLink** value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoS)->GetTXPhysLink(_pif, value);
}

/** @memberof ds_Net_IQoS
  * 
  * This attribute provides the Rx physical link object.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IQoS_GetRXPhysLink(ds_Net_IQoS* _pif, ds_Net_IPhysLink** value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoS)->GetRXPhysLink(_pif, value);
}

/** @memberof ds_Net_IQoS
  * 
  * This attribute can be used to fetch the result of last 
  * IQoSSecondary_Modify or IQoSDefault_Modify operation when a
  * ds_Net_QoS_Event_MODIFY_RESULT
  * event is received.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @See ds_Net_QoS_ResultType.
  */
static __inline AEEResult ds_Net_IQoS_GetModifyResult(ds_Net_IQoS* _pif, ds_Net_QoSResultType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoS)->GetModifyResult(_pif, value);
}

/** @memberof ds_Net_IQoS
  *     
  * This attribute can be used to fetch updated infoCode information
  * for the QoS flow when a ds_Net_QoS_Event_INFO_CODE_UPDATED
  * event is received.
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @See ds_Net_QoS_InfoCodeType.
  */
static __inline AEEResult ds_Net_IQoS_GetUpdatedInfoCode(ds_Net_IQoS* _pif, ds_Net_QoSInfoCodeType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoS)->GetUpdatedInfoCode(_pif, value);
}

/** @memberof ds_Net_IQoS
  * 
  * This attribute represents the TX queue level.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IQoS_GetTXQueueLevel(ds_Net_IQoS* _pif, ds_Net_QoSTXQueueLevelType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoS)->GetTXQueueLevel(_pif, value);
}

/** @memberof ds_Net_IQoS
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
static __inline AEEResult ds_Net_IQoS_GetTechObject(ds_Net_IQoS* _pif, AEEIID techObj_iid, void** techObj)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoS)->GetTechObject(_pif, techObj_iid, techObj);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_Def.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_QoS_Def.h"
#include "ds_Net_IPhysLink.h"
namespace ds
{
   namespace Net
   {
      const ::AEEIID AEEIID_IQoS = 0x106d74f;
      
      /** @interface IQoS
        * 
        * ds Net QoS interface.
        * Events that can be registered on this interface via OnStateChange (as part of IEventManager interface): 
        * - ds::Net::QoSEvent::MODIFY_RESULT. Use GetModifyResult to fetch the result information.
        * - ds::Net::QoSEvent::INFO_CODE_UPDATED. Use GetUpdatedInfoCode to fetch the information.
        */
      struct IQoS : public ::ds::Net::IEventManager
      {
         
         /**
           * This function returns the current Rx and Tx flow specification.
           * @param rxFlow the current Rx flow specification.
           *               See ds_Net_QoSFlow.json schema.
           * @param txFlow the current Tx flow specification.
           *               See ds_Net_QoSFlow.json schema.
           * @retval ds::SUCCESS Request received successfully.
           * @retval ds::EBADOPTS Bad options.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetGrantedFlowSpec(char* rxFlow, int rxFlowLen, int* rxFlowLenReq, char* txFlow, int txFlowLen, int* txFlowLenReq) = 0;
         
         /**
           * This attribute provides the Tx physical link object.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetTXPhysLink(::ds::Net::IPhysLink** value) = 0;
         
         /**
           * This attribute provides the Rx physical link object.          
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetRXPhysLink(::ds::Net::IPhysLink** value) = 0;
         
         /**
           * This attribute can be used to fetch the result of last 
           * IQoSSecondary::Modify or IQoSDefault::Modify operation when a
           * ds::Net::QoS::Event::MODIFY_RESULT
           * event is received.          
           * @param value Attribute value
           * @See ds::Net::QoS::ResultType.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetModifyResult(::ds::Net::QoSResultType* value) = 0;
         
         /**    
           * This attribute can be used to fetch updated infoCode information
           * for the QoS flow when a ds::Net::QoS::Event::INFO_CODE_UPDATED
           * event is received.
           * @param value Attribute value
           * @See ds::Net::QoS::InfoCodeType.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetUpdatedInfoCode(::ds::Net::QoSInfoCodeType* value) = 0;
         
         /**
           * This attribute represents the TX queue level.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetTXQueueLevel(::ds::Net::QoSTXQueueLevelType* value) = 0;
         
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
#endif //DS_NET_IQOS_H
