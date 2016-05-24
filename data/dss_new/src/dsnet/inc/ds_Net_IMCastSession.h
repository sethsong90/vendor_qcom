#ifndef DS_NET_IMCASTSESSION_H
#define DS_NET_IMCASTSESSION_H

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

/** @memberof ds_Net_MCastSessionEvent
  * 
  * QDS_EV_REGISTRATION_STATE: Register to this event to be notified
  * on changes in the registration state of this MCast Session.
  * Registration state change information contains both a sepcification
  * of the new registration state and an information code (status code
  * that may specify further information such as registration failure
  * reason.
  * @See MCastStateChangedType, MCastRegStateType, MCastInfoCodeType
  */
#define ds_Net_MCastSessionEvent_QDS_EV_REGISTRATION_STATE 0x106e60d

/** @memberof ds_Net_MCastSessionEvent
  * 
  * QDS_EV_TECHNOLOGY_STATUS: Register to this event to be notified
  * on status changes of this MCast Session. 
  * @See MCastInfoCodeType
  */
#define ds_Net_MCastSessionEvent_QDS_EV_TECHNOLOGY_STATUS 0x106e60e
#define ds_Net_AEEIID_IMCastSession 0x106cf95

/** @interface ds_Net_IMCastSession
  * 
  * ds Net MultiCast interface.
  * Events that can be registered on this interface via OnStateChange (as part of IEventManager interface): 
  * - ds_Net_MCastSessionEvent_REGISTRATION_STATE. Use
  *   GetRegStateAndInfoCode to fetch the state information.
  * - ds_Net_MCastSessionEvent_TECHNOLOGY_STATUS. Use
  *   GetTechnologyStatus to fetch the status information.
  */
#define INHERIT_ds_Net_IMCastSession(iname) \
   INHERIT_ds_Net_IEventManager(iname); \
   AEEResult (*Leave)(iname* _pif); \
   AEEResult (*GetRegStateAndInfoCode)(iname* _pif, ds_Net_MCastStateChangedType* value); \
   AEEResult (*GetTechStatusInfoCode)(iname* _pif, ds_Net_MCastInfoCodeType* value)
AEEINTERFACE_DEFINE(ds_Net_IMCastSession);

/** @memberof ds_Net_IMCastSession
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IMCastSession_AddRef(ds_Net_IMCastSession* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSession)->AddRef(_pif);
}

/** @memberof ds_Net_IMCastSession
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IMCastSession_Release(ds_Net_IMCastSession* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSession)->Release(_pif);
}

/** @memberof ds_Net_IMCastSession
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IMCastSession_QueryInterface(ds_Net_IMCastSession* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSession)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IMCastSession
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
static __inline AEEResult ds_Net_IMCastSession_OnStateChange(ds_Net_IMCastSession* _pif, ISignal* signal, ds_Net_EventType eventID, IQI** regObj)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSession)->OnStateChange(_pif, signal, eventID, regObj);
}

/** @memberof ds_Net_IMCastSession
  * 
  * This function issues a request for leaving a multicast group.
  * @param _pif Pointer to interface
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IMCastSession_Leave(ds_Net_IMCastSession* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSession)->Leave(_pif);
}

/** @memberof ds_Net_IMCastSession
  * 
  * This attribute indicates the MCast session registration state and
  * information code.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IMCastSession_GetRegStateAndInfoCode(ds_Net_IMCastSession* _pif, ds_Net_MCastStateChangedType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSession)->GetRegStateAndInfoCode(_pif, value);
}

/** @memberof ds_Net_IMCastSession
  * 
  * This attribute indicates the information code for the status of the
  * MCast technology session.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IMCastSession_GetTechStatusInfoCode(ds_Net_IMCastSession* _pif, ds_Net_MCastInfoCodeType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSession)->GetTechStatusInfoCode(_pif, value);
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
      namespace MCastSessionEvent
      {
         
         /**
           * QDS_EV_REGISTRATION_STATE: Register to this event to be notified
           * on changes in the registration state of this MCast Session.
           * Registration state change information contains both a sepcification
           * of the new registration state and an information code (status code
           * that may specify further information such as registration failure
           * reason.
           * @See MCastStateChangedType, MCastRegStateType, MCastInfoCodeType
           */
         const ::ds::Net::EventType QDS_EV_REGISTRATION_STATE = 0x106e60d;
         
         /**
           * QDS_EV_TECHNOLOGY_STATUS: Register to this event to be notified
           * on status changes of this MCast Session. 
           * @See MCastInfoCodeType
           */
         const ::ds::Net::EventType QDS_EV_TECHNOLOGY_STATUS = 0x106e60e;
      };
      const ::AEEIID AEEIID_IMCastSession = 0x106cf95;
      
      /** @interface IMCastSession
        * 
        * ds Net MultiCast interface.
        * Events that can be registered on this interface via OnStateChange (as part of IEventManager interface): 
        * - ds::Net::MCastSessionEvent::REGISTRATION_STATE. Use
        *   GetRegStateAndInfoCode to fetch the state information.
        * - ds::Net::MCastSessionEvent::TECHNOLOGY_STATUS. Use
        *   GetTechnologyStatus to fetch the status information.
        */
      struct IMCastSession : public ::ds::Net::IEventManager
      {
         
         /**
           * This function issues a request for leaving a multicast group.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Leave() = 0;
         
         /**
           * This attribute indicates the MCast session registration state and
           * information code.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetRegStateAndInfoCode(::ds::Net::MCastStateChangedType* value) = 0;
         
         /**
           * This attribute indicates the information code for the status of the
           * MCast technology session.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetTechStatusInfoCode(::ds::Net::MCastInfoCodeType* value) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IMCASTSESSION_H
