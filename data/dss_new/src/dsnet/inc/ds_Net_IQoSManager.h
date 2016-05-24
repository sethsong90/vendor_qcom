#ifndef DS_NET_IQOSMANAGER_H
#define DS_NET_IQOSMANAGER_H

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
#include "ds_Net_IQoSDefault.h"
#include "ds_Net_IQoSSecondary.h"
#include "ds_Net_IQoSSecondariesInput.h"
#include "ds_Net_IQoSSecondariesOutput.h"
#define ds_Net_QoSMgrEvent_QDS_EV_PROFILES_CHANGED 0x106e615
#define ds_Net_AEEIID_IQoSManager 0x106cd44
struct ds_Net_IQoSManager__SeqQoSProfileIdType__seq_long {
   ds_Net_QoSProfileIdType* data;
   int dataLen;
   int dataLenReq;
};
typedef struct ds_Net_IQoSManager__SeqQoSProfileIdType__seq_long ds_Net_IQoSManager__SeqQoSProfileIdType__seq_long;
typedef ds_Net_IQoSManager__SeqQoSProfileIdType__seq_long ds_Net_IQoSManager_SeqQoSProfileIdType;

/** @interface ds_Net_IQoSManager
  * 
  * ds Net QoS Manager interface.
  * Events that can be registered on this interface via OnStateChange (as part of IEventManager interface): 
  * - ds_Net_QoSMgrEvent_PROFILES_CHANGED. Use GetSupportedProfiles to fetch the current set of supported QoS Profiles. 
  */
#define INHERIT_ds_Net_IQoSManager(iname) \
   INHERIT_ds_Net_IEventManager(iname); \
   AEEResult (*GetQosDefault)(iname* _pif, ds_Net_IQoSDefault** value); \
   AEEResult (*Request)(iname* _pif, const char* specs, ds_Net_QoSRequestOpCodeType opCode, ds_Net_IQoSSecondariesOutput** qosSessions, char* errSpec, int errSpecLen, int* errSpecLenReq); \
   AEEResult (*Close)(iname* _pif, ds_Net_IQoSSecondariesInput* qosSessions); \
   AEEResult (*Resume)(iname* _pif, ds_Net_IQoSSecondariesInput* qosSessions); \
   AEEResult (*Suspend)(iname* _pif, ds_Net_IQoSSecondariesInput* qosSessions); \
   AEEResult (*GetSupportedProfiles)(iname* _pif, ds_Net_QoSProfileIdType* value, int valueLen, int* valueLenReq); \
   AEEResult (*CreateQoSSecondariesInput)(iname* _pif, ds_Net_IQoSSecondariesInput** newQoSSecondariesInput)
AEEINTERFACE_DEFINE(ds_Net_IQoSManager);

/** @memberof ds_Net_IQoSManager
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IQoSManager_AddRef(ds_Net_IQoSManager* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSManager)->AddRef(_pif);
}

/** @memberof ds_Net_IQoSManager
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IQoSManager_Release(ds_Net_IQoSManager* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSManager)->Release(_pif);
}

/** @memberof ds_Net_IQoSManager
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IQoSManager_QueryInterface(ds_Net_IQoSManager* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSManager)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IQoSManager
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
static __inline AEEResult ds_Net_IQoSManager_OnStateChange(ds_Net_IQoSManager* _pif, ISignal* signal, ds_Net_EventType eventID, IQI** regObj)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSManager)->OnStateChange(_pif, signal, eventID, regObj);
}

/** @memberof ds_Net_IQoSManager
  * 
  * This attribute returns an object that represents the default QoS link.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @see IQoSDefault
  */
static __inline AEEResult ds_Net_IQoSManager_GetQosDefault(ds_Net_IQoSManager* _pif, ds_Net_IQoSDefault** value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSManager)->GetQosDefault(_pif, value);
}

/** @memberof ds_Net_IQoSManager
  * 
  * This function issues QoS requests for multiple 
  * or single QoS sessions.
  * @param _pif Pointer to interface
  * @param specs A JSON string that specifies the QoS Specification
  *              for all the required QoS Sessions (or session).
  *              See ds_Net_QoSRequest.json schema.
  * @param opCode The operation.
  * @param qosSessions Output The QoS sessions. 
  *                    The order of sessions in IQoSSecondariesOutput 
  *                    shall correspond to the order of QoS specs in
  *                    specs.
  * @param errSpec specification of flow options and filter options 
  *                for which invalid values were specified in specs.
  *                See ds_Net_QoSRequestErr.json schema.
  * @param errSpecLen Length of sequence
  * @param errSpecLenReq Required length of sequence
  * @retval ds_SUCCESS Request received successfully.
  * @retval ds_EBADOPTS Bad options.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  * @see ds_Net_QoSRequestOpCodeType
  */
static __inline AEEResult ds_Net_IQoSManager_Request(ds_Net_IQoSManager* _pif, const char* specs, ds_Net_QoSRequestOpCodeType opCode, ds_Net_IQoSSecondariesOutput** qosSessions, char* errSpec, int errSpecLen, int* errSpecLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSManager)->Request(_pif, specs, opCode, qosSessions, errSpec, errSpecLen, errSpecLenReq);
}

/** @memberof ds_Net_IQoSManager
  * 
  * The function closes one or more QoS sessions.
  * @param _pif Pointer to interface
  * @param qosSessions IQoSSecondary objects to Close.
  * @retval ds_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IQoSManager_Close(ds_Net_IQoSManager* _pif, ds_Net_IQoSSecondariesInput* qosSessions)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSManager)->Close(_pif, qosSessions);
}

/** @memberof ds_Net_IQoSManager
  * 
  * The function resumes one or more QoS sessions.
  * @param _pif Pointer to interface
  * @param qosSessions IQoSSecondary objects to Resume.
  * @retval ds_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IQoSManager_Resume(ds_Net_IQoSManager* _pif, ds_Net_IQoSSecondariesInput* qosSessions)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSManager)->Resume(_pif, qosSessions);
}

/** @memberof ds_Net_IQoSManager
  * 
  * The function suspends one or more QoS sessions.
  * @param _pif Pointer to interface
  * @param qosSessions IQoSSecondary objects to Suspend.
  * @retval ds_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IQoSManager_Suspend(ds_Net_IQoSManager* _pif, ds_Net_IQoSSecondariesInput* qosSessions)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSManager)->Suspend(_pif, qosSessions);
}

/** @memberof ds_Net_IQoSManager
  * 
  * This attribute returns the QoS Profiles that are currently supported
  * by the network.           
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @param valueLen Length of sequence
  * @param valueLenReq Required length of sequence
  * @see SeqQoSProfileIdType
  */
static __inline AEEResult ds_Net_IQoSManager_GetSupportedProfiles(ds_Net_IQoSManager* _pif, ds_Net_QoSProfileIdType* value, int valueLen, int* valueLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSManager)->GetSupportedProfiles(_pif, value, valueLen, valueLenReq);
}

/** @memberof ds_Net_IQoSManager
  *     
  * This function creates an instance of IQoSSecondariesInput.
  * IQoSSecondariesInput creation is supported only via IQoSManager. 
  * @param _pif Pointer to interface
  * @param new newQoSSecondariesInput Output The newly created 
  *                                          IQoSSecondariesInput instance.
  * @retval ds_SUCCESS IQoSSecondariesInput created successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IQoSManager_CreateQoSSecondariesInput(ds_Net_IQoSManager* _pif, ds_Net_IQoSSecondariesInput** newQoSSecondariesInput)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSManager)->CreateQoSSecondariesInput(_pif, newQoSSecondariesInput);
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
#include "ds_Net_IQoSDefault.h"
#include "ds_Net_IQoSSecondary.h"
#include "ds_Net_IQoSSecondariesInput.h"
#include "ds_Net_IQoSSecondariesOutput.h"
namespace ds
{
   namespace Net
   {
      namespace QoSMgrEvent
      {
         const ::ds::Net::EventType QDS_EV_PROFILES_CHANGED = 0x106e615;
      };
      const ::AEEIID AEEIID_IQoSManager = 0x106cd44;
      
      /** @interface IQoSManager
        * 
        * ds Net QoS Manager interface.
        * Events that can be registered on this interface via OnStateChange (as part of IEventManager interface): 
        * - ds::Net::QoSMgrEvent::PROFILES_CHANGED. Use GetSupportedProfiles to fetch the current set of supported QoS Profiles. 
        */
      struct IQoSManager : public ::ds::Net::IEventManager
      {
         struct _SeqQoSProfileIdType__seq_long {
            ::ds::Net::QoSProfileIdType* data;
            int dataLen;
            int dataLenReq;
         };
         typedef _SeqQoSProfileIdType__seq_long SeqQoSProfileIdType;
         
         /**
           * This attribute returns an object that represents the default QoS link.          
           * @param value Attribute value
           * @see IQoSDefault
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetQosDefault(::ds::Net::IQoSDefault** value) = 0;
         
         /**
           * This function issues QoS requests for multiple 
           * or single QoS sessions.
           * @param specs A JSON string that specifies the QoS Specification
           *              for all the required QoS Sessions (or session).
           *              See ds_Net_QoSRequest.json schema.
           * @param opCode The operation.
           * @param qosSessions Output The QoS sessions. 
           *                    The order of sessions in IQoSSecondariesOutput 
           *                    shall correspond to the order of QoS specs in
           *                    specs.
           * @param errSpec specification of flow options and filter options 
           *                for which invalid values were specified in specs.
           *                See ds_Net_QoSRequestErr.json schema.
           * @retval ds::SUCCESS Request received successfully.
           * @retval ds::EBADOPTS Bad options.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           * @see ds::Net::QoSRequestOpCodeType
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Request(const char* specs, ::ds::Net::QoSRequestOpCodeType opCode, ::ds::Net::IQoSSecondariesOutput** qosSessions, char* errSpec, int errSpecLen, int* errSpecLenReq) = 0;
         
         /**
           * The function closes one or more QoS sessions.
           * @param qosSessions IQoSSecondary objects to Close.
           * @retval ds::SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Close(::ds::Net::IQoSSecondariesInput* qosSessions) = 0;
         
         /**
           * The function resumes one or more QoS sessions.
           * @param qosSessions IQoSSecondary objects to Resume.
           * @retval ds::SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Resume(::ds::Net::IQoSSecondariesInput* qosSessions) = 0;
         
         /**
           * The function suspends one or more QoS sessions.
           * @param qosSessions IQoSSecondary objects to Suspend.
           * @retval ds::SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Suspend(::ds::Net::IQoSSecondariesInput* qosSessions) = 0;
         
         /**
           * This attribute returns the QoS Profiles that are currently supported
           * by the network.           
           * @param value Attribute value
           * @see SeqQoSProfileIdType
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetSupportedProfiles(::ds::Net::QoSProfileIdType* value, int valueLen, int* valueLenReq) = 0;
         
         /**    
           * This function creates an instance of IQoSSecondariesInput.
           * IQoSSecondariesInput creation is supported only via IQoSManager. 
           * @param new newQoSSecondariesInput Output The newly created 
           *                                          IQoSSecondariesInput instance.
           * @retval ds::SUCCESS IQoSSecondariesInput created successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL CreateQoSSecondariesInput(::ds::Net::IQoSSecondariesInput** newQoSSecondariesInput) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IQOSMANAGER_H
