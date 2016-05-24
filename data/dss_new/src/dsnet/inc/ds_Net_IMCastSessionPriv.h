#ifndef DS_NET_IMCASTSESSIONPRIV_H
#define DS_NET_IMCASTSESSIONPRIV_H

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
#include "ds_Addr_Def.h"
#include "ds_Net_MCast_Def.h"

/** @memberof ds_Net_MCastSessionPrivEvent
  * 
  * QDS_EV_REGISTRATION_STATE_PRIV_BCMCS1_0: 
  * DSS shall register to this event to be notified on MCast events with
  * BCMCS1.0 ("legacy") info code. DSS shall register to this event
  * regardless if the MCastSession object was obtained via call to
  * IMCastManagerPriv_Join or IMCastManagerPriv_JoinBundle.
  * Once the signal of this event registration is set, DSS shall fetch
  * both the registration state and info code together in a single call
  * to GetRegStateAndBCMCS1_0InfoCode.
  */
#define ds_Net_MCastSessionPrivEvent_QDS_EV_REGISTRATION_STATE_PRIV_BCMCS1_0 0x10a4640

/** @memberof ds_Net_MCastSessionPrivEvent
  * 
  * QDS_EV_REGISTRATION_STATE_PRIV_BCMCS2_0: 
  * DSS shall register to this event to be notified on MCast events with
  * BCMCS2.0 ("new") info code. DSS shall register to this event
  * regardless if the MCastSession object was obtained via call to
  * IMCastManagerPriv_Join or IMCastManagerPriv_JoinBundle.
  * Once the signal of this event registration is set, DSS shall fetch
  * both the registration state and info code together in a single call
  * to GetRegStateAndBCMCS2_0InfoCode.
  */
#define ds_Net_MCastSessionPrivEvent_QDS_EV_REGISTRATION_STATE_PRIV_BCMCS2_0 0x10a463f

/** @memberof ds_Net_MCastSessionPrivEvent
  * 
  * QDS_EV_TECHNOLOGY_STATUS_PRIV: 
  * DSS shall register to this event to be notified on MCast events with
  * BCMCS2.0 ("new") info code. DSS shall register to this event
  * regardless if the MCastSession object was obtained via call to
  * IMCastManagerPriv_Join or IMCastManagerPriv_JoinBundle.
  * Once the signal of this event registration is set, DSS shall fetch
  * both the registration state and info code together in a single call
  * to GetRegStateAndBCMCS2_0InfoCode.
  */
#define ds_Net_MCastSessionPrivEvent_QDS_EV_TECHNOLOGY_STATUS_PRIV 0x10a4641
#define ds_Net_AEEIID_IMCastSessionPriv 0x10a38d4

/** @interface ds_Net_IMCastSessionPriv
  * 
  * ds Net MultiCast interface.
  * Events that can be registered on this interface via OnStateChange (as part of IEventManager interface): 
  * - ds_Net_MCastSessionEvent_REGISTRATION_STATE. Use GetRegistrationState to fetch the state information.
  * - ds_Net_MCastSessionEvent_TECHNOLOGY_STATUS. Use GetTechnologyStatus to fetch the status information.
  */
#define INHERIT_ds_Net_IMCastSessionPriv(iname) \
   INHERIT_ds_Net_IEventManager(iname); \
   AEEResult (*Leave)(iname* _pif); \
   AEEResult (*GetRegStateAndInfoCodeBCMCS1_0)(iname* _pif, ds_Net_MCastStateChangedType* value); \
   AEEResult (*GetRegStateAndInfoCodeBCMCS2_0)(iname* _pif, ds_Net_MCastStateChangedType* value); \
   AEEResult (*GetTechStatusInfoCode)(iname* _pif, ds_Net_MCastInfoCodeType* value)
AEEINTERFACE_DEFINE(ds_Net_IMCastSessionPriv);

/** @memberof ds_Net_IMCastSessionPriv
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IMCastSessionPriv_AddRef(ds_Net_IMCastSessionPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSessionPriv)->AddRef(_pif);
}

/** @memberof ds_Net_IMCastSessionPriv
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IMCastSessionPriv_Release(ds_Net_IMCastSessionPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSessionPriv)->Release(_pif);
}

/** @memberof ds_Net_IMCastSessionPriv
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IMCastSessionPriv_QueryInterface(ds_Net_IMCastSessionPriv* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSessionPriv)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IMCastSessionPriv
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
static __inline AEEResult ds_Net_IMCastSessionPriv_OnStateChange(ds_Net_IMCastSessionPriv* _pif, ISignal* signal, ds_Net_EventType eventID, IQI** regObj)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSessionPriv)->OnStateChange(_pif, signal, eventID, regObj);
}

/** @memberof ds_Net_IMCastSessionPriv
  * 
  * This function issues a request for leaving a multicast group.
  * @param _pif Pointer to interface
  * @retval ds_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IMCastSessionPriv_Leave(ds_Net_IMCastSessionPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSessionPriv)->Leave(_pif);
}

/** @memberof ds_Net_IMCastSessionPriv
  * 
  * This attribute indicates the MCast session registration state and
  * information code. DSS fetch this information once 
  * QDS_EV_REGISTRATION_STATE_PRIV_BCMCS1_0 events occurs.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IMCastSessionPriv_GetRegStateAndInfoCodeBCMCS1_0(ds_Net_IMCastSessionPriv* _pif, ds_Net_MCastStateChangedType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSessionPriv)->GetRegStateAndInfoCodeBCMCS1_0(_pif, value);
}

/** @memberof ds_Net_IMCastSessionPriv
  * 
  * This attribute indicates the MCast session registration state and
  * information code. DSS fetch this information once 
  * QDS_EV_REGISTRATION_STATE_PRIV_BCMCS2_0 events occurs.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IMCastSessionPriv_GetRegStateAndInfoCodeBCMCS2_0(ds_Net_IMCastSessionPriv* _pif, ds_Net_MCastStateChangedType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSessionPriv)->GetRegStateAndInfoCodeBCMCS2_0(_pif, value);
}

/** @memberof ds_Net_IMCastSessionPriv
  * 
  * This attribute indicates the information code for the status of the
  * MCast technology session.
  * DSS fetch this information onceQDS_EV_TECHNOLOGY_STATUS_PRIV event
  * occurs.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IMCastSessionPriv_GetTechStatusInfoCode(ds_Net_IMCastSessionPriv* _pif, ds_Net_MCastInfoCodeType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSessionPriv)->GetTechStatusInfoCode(_pif, value);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_Def.h"
#include "ds_Net_IEventManager.h"
#include "ds_Addr_Def.h"
#include "ds_Net_MCast_Def.h"
namespace ds
{
   namespace Net
   {
      namespace MCastSessionPrivEvent
      {
         
         /**
           * QDS_EV_REGISTRATION_STATE_PRIV_BCMCS1_0: 
           * DSS shall register to this event to be notified on MCast events with
           * BCMCS1.0 ("legacy") info code. DSS shall register to this event
           * regardless if the MCastSession object was obtained via call to
           * IMCastManagerPriv::Join or IMCastManagerPriv::JoinBundle.
           * Once the signal of this event registration is set, DSS shall fetch
           * both the registration state and info code together in a single call
           * to GetRegStateAndBCMCS1_0InfoCode.
           */
         const ::ds::Net::EventType QDS_EV_REGISTRATION_STATE_PRIV_BCMCS1_0 = 0x10a4640;
         
         /**
           * QDS_EV_REGISTRATION_STATE_PRIV_BCMCS2_0: 
           * DSS shall register to this event to be notified on MCast events with
           * BCMCS2.0 ("new") info code. DSS shall register to this event
           * regardless if the MCastSession object was obtained via call to
           * IMCastManagerPriv::Join or IMCastManagerPriv::JoinBundle.
           * Once the signal of this event registration is set, DSS shall fetch
           * both the registration state and info code together in a single call
           * to GetRegStateAndBCMCS2_0InfoCode.
           */
         const ::ds::Net::EventType QDS_EV_REGISTRATION_STATE_PRIV_BCMCS2_0 = 0x10a463f;
         
         /**
           * QDS_EV_TECHNOLOGY_STATUS_PRIV: 
           * DSS shall register to this event to be notified on MCast events with
           * BCMCS2.0 ("new") info code. DSS shall register to this event
           * regardless if the MCastSession object was obtained via call to
           * IMCastManagerPriv::Join or IMCastManagerPriv::JoinBundle.
           * Once the signal of this event registration is set, DSS shall fetch
           * both the registration state and info code together in a single call
           * to GetRegStateAndBCMCS2_0InfoCode.
           */
         const ::ds::Net::EventType QDS_EV_TECHNOLOGY_STATUS_PRIV = 0x10a4641;
      };
      const ::AEEIID AEEIID_IMCastSessionPriv = 0x10a38d4;
      
      /** @interface IMCastSessionPriv
        * 
        * ds Net MultiCast interface.
        * Events that can be registered on this interface via OnStateChange (as part of IEventManager interface): 
        * - ds::Net::MCastSessionEvent::REGISTRATION_STATE. Use GetRegistrationState to fetch the state information.
        * - ds::Net::MCastSessionEvent::TECHNOLOGY_STATUS. Use GetTechnologyStatus to fetch the status information.
        */
      struct IMCastSessionPriv : public ::ds::Net::IEventManager
      {
         
         /**
           * This function issues a request for leaving a multicast group.
           * @retval ds::SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Leave() = 0;
         
         /**
           * This attribute indicates the MCast session registration state and
           * information code. DSS fetch this information once 
           * QDS_EV_REGISTRATION_STATE_PRIV_BCMCS1_0 events occurs.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetRegStateAndInfoCodeBCMCS1_0(::ds::Net::MCastStateChangedType* value) = 0;
         
         /**
           * This attribute indicates the MCast session registration state and
           * information code. DSS fetch this information once 
           * QDS_EV_REGISTRATION_STATE_PRIV_BCMCS2_0 events occurs.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetRegStateAndInfoCodeBCMCS2_0(::ds::Net::MCastStateChangedType* value) = 0;
         
         /**
           * This attribute indicates the information code for the status of the
           * MCast technology session.
           * DSS fetch this information onceQDS_EV_TECHNOLOGY_STATUS_PRIV event
           * occurs.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetTechStatusInfoCode(::ds::Net::MCastInfoCodeType* value) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IMCASTSESSIONPRIV_H
