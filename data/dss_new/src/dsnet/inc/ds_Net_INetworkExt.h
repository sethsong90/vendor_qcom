#ifndef DS_NET_INETWORKEXT_H
#define DS_NET_INETWORKEXT_H

/*============================================================================
  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "ds_Net_Def.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_QoS_Def.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_IQoS.h"
#include "ds_Net_IQoSDefault.h"
#include "ds_Net_IQoSSecondary.h"
#include "ds_Net_IQoSSecondariesInput.h"
#include "ds_Net_IQoSSecondariesOutput.h"
#include "ds_Net_IQoSManager.h"
#include "ds_Addr_Def.h"
#include "ds_Net_MCast_Def.h"
#include "ds_Net_IMCastSession.h"
#include "ds_Net_IMCastManager.h"
#include "ds_Net_IBearerInfo.h"
#define ds_Net_NetworkExtEvent_QDS_EV_QOS_AWARENESS 0x106e611
#define ds_Net_NetworkExtEvent_QDS_EV_BEARER_TECH_CHANGED 0x106e616
typedef int ds_Net_QoSModeType;
#define ds_Net_QoSMode_UE_ONLY 0
#define ds_Net_QoSMode_UE_AND_NW 1
#define ds_Net_QoSMode_NW_ONLY 2
#define ds_Net_AEEIID_INetworkExt 0x109a8af

/** @interface ds_Net_INetworkExt
  * 
  * ds NetworkExt interface.
  * This Network Extended interface is an auxiliary interface to INetwork,
  * supporting advanced Network operations, including QoS, MCast, BearerTech and more.
  * All QoS and MCast objects are created via this interface. @See CreateQoSManager and
  * CreateMCastManager.
  *
  * Events that can be registered on this interface via OnStateChange (as part of IEventManager interface): 
  * - ds_Net_NetworkExtEvent_QDS_EV_QOS_AWARENESS. Use IsQoSAware to fetch the QOS aware status information.       
  * - ds_Net_NetworkExtEvent_QDS_EV_BEARER_TECH_CHANGED. Use GetBearerInfo to fetch the status information.
  */
#define INHERIT_ds_Net_INetworkExt(iname) \
   INHERIT_ds_Net_IEventManager(iname); \
   AEEResult (*GetBearerInfo)(iname* _pif, ds_Net_IBearerInfo** bearerTech); \
   AEEResult (*GetQosAware)(iname* _pif, boolean* value); \
   AEEResult (*GetQoSMode)(iname* _pif, ds_Net_QoSModeType* qosMode); \
   AEEResult (*CreateQoSManager)(iname* _pif, ds_Net_IQoSManager** newQoSManager); \
   AEEResult (*CreateMCastManager)(iname* _pif, ds_Net_IMCastManager** newMCastManager)
AEEINTERFACE_DEFINE(ds_Net_INetworkExt);

/** @memberof ds_Net_INetworkExt
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INetworkExt_AddRef(ds_Net_INetworkExt* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkExt)->AddRef(_pif);
}

/** @memberof ds_Net_INetworkExt
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INetworkExt_Release(ds_Net_INetworkExt* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkExt)->Release(_pif);
}

/** @memberof ds_Net_INetworkExt
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_INetworkExt_QueryInterface(ds_Net_INetworkExt* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkExt)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_INetworkExt
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
static __inline AEEResult ds_Net_INetworkExt_OnStateChange(ds_Net_INetworkExt* _pif, ISignal* signal, ds_Net_EventType eventID, IQI** regObj)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkExt)->OnStateChange(_pif, signal, eventID, regObj);
}

/** @memberof ds_Net_INetworkExt
  * 
  * This function returns the bearer technology information.
  * @param _pif Pointer to interface
  * @param bearerTech Object that will hold the bearer information.
  * @see IBearerInfo
  * @retval ds_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkExt_GetBearerInfo(ds_Net_INetworkExt* _pif, ds_Net_IBearerInfo** bearerTech)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkExt)->GetBearerInfo(_pif, bearerTech);
}

/** @memberof ds_Net_INetworkExt
  *           
  * This attribute indicates whether or not the current system is
  * CDMA QoS Aware system.
  * TRUE if the mobile is on a QoS Aware system
  * FALSE if the mobile is on a QoS Unaware system.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_INetworkExt_GetQosAware(ds_Net_INetworkExt* _pif, boolean* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkExt)->GetQosAware(_pif, value);
}

/** @memberof ds_Net_INetworkExt
  * 
  * This function Gets the QoS mode
  * @param _pif Pointer to interface
  * @param qosMode Output The current QosMode
  * @retval ds_SUCCESS QosMode retrieved successfully 
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkExt_GetQoSMode(ds_Net_INetworkExt* _pif, ds_Net_QoSModeType* qosMode)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkExt)->GetQoSMode(_pif, qosMode);
}

/** @memberof ds_Net_INetworkExt
  *     
  * This function creates an instance of IQoSManager. IQoSManager creation
  * is supported only via INetworkExt.    
  * @param _pif Pointer to interface
  * @param newQoSManager Output The newly created IQoSManager instance.
  * @retval ds_SUCCESS IQoSManager created successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkExt_CreateQoSManager(ds_Net_INetworkExt* _pif, ds_Net_IQoSManager** newQoSManager)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkExt)->CreateQoSManager(_pif, newQoSManager);
}

/** @memberof ds_Net_INetworkExt
  *     
  * This function creates an instance of IMCastManager. IMCastManager creation
  * is supported only via INetworkExt.    
  * @param _pif Pointer to interface
  * @param newMCastManager Output The newly created IMCastManager instance.
  * @retval ds_SUCCESS IMCastManager created successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkExt_CreateMCastManager(ds_Net_INetworkExt* _pif, ds_Net_IMCastManager** newMCastManager)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkExt)->CreateMCastManager(_pif, newMCastManager);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "ds_Net_Def.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_QoS_Def.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_IQoS.h"
#include "ds_Net_IQoSDefault.h"
#include "ds_Net_IQoSSecondary.h"
#include "ds_Net_IQoSSecondariesInput.h"
#include "ds_Net_IQoSSecondariesOutput.h"
#include "ds_Net_IQoSManager.h"
#include "ds_Addr_Def.h"
#include "ds_Net_MCast_Def.h"
#include "ds_Net_IMCastSession.h"
#include "ds_Net_IMCastManager.h"
#include "ds_Net_IBearerInfo.h"
namespace ds
{
   namespace Net
   {
      namespace NetworkExtEvent
      {
         const ::ds::Net::EventType QDS_EV_QOS_AWARENESS = 0x106e611;
         const ::ds::Net::EventType QDS_EV_BEARER_TECH_CHANGED = 0x106e616;
      };
      typedef int QoSModeType;
      namespace QoSMode
      {
         const ::ds::Net::QoSModeType UE_ONLY = 0;
         const ::ds::Net::QoSModeType UE_AND_NW = 1;
         const ::ds::Net::QoSModeType NW_ONLY = 2;
      };
      const ::AEEIID AEEIID_INetworkExt = 0x109a8af;
      
      /** @interface INetworkExt
        * 
        * ds NetworkExt interface.
        * This Network Extended interface is an auxiliary interface to INetwork,
        * supporting advanced Network operations, including QoS, MCast, BearerTech and more.
        * All QoS and MCast objects are created via this interface. @See CreateQoSManager and
        * CreateMCastManager.
        *
        * Events that can be registered on this interface via OnStateChange (as part of IEventManager interface): 
        * - ds::Net::NetworkExtEvent::QDS_EV_QOS_AWARENESS. Use IsQoSAware to fetch the QOS aware status information.       
        * - ds::Net::NetworkExtEvent::QDS_EV_BEARER_TECH_CHANGED. Use GetBearerInfo to fetch the status information.
        */
      struct INetworkExt : public ::ds::Net::IEventManager
      {
         
         /**
           * This function returns the bearer technology information.
           * @param bearerTech Object that will hold the bearer information.
           * @see IBearerInfo
           * @retval ds::SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetBearerInfo(::ds::Net::IBearerInfo** bearerTech) = 0;
         
         /**          
           * This attribute indicates whether or not the current system is
           * CDMA QoS Aware system.
           * TRUE if the mobile is on a QoS Aware system
           * FALSE if the mobile is on a QoS Unaware system.          
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetQosAware(boolean* value) = 0;
         
         /**
           * This function Gets the QoS mode
           * @param qosMode Output The current QosMode
           * @retval ds::SUCCESS QosMode retrieved successfully 
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetQoSMode(::ds::Net::QoSModeType* qosMode) = 0;
         
         /**    
           * This function creates an instance of IQoSManager. IQoSManager creation
           * is supported only via INetworkExt.    
           * @param newQoSManager Output The newly created IQoSManager instance.
           * @retval ds::SUCCESS IQoSManager created successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL CreateQoSManager(::ds::Net::IQoSManager** newQoSManager) = 0;
         
         /**    
           * This function creates an instance of IMCastManager. IMCastManager creation
           * is supported only via INetworkExt.    
           * @param newMCastManager Output The newly created IMCastManager instance.
           * @retval ds::SUCCESS IMCastManager created successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL CreateMCastManager(::ds::Net::IMCastManager** newMCastManager) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_INETWORKEXT_H
