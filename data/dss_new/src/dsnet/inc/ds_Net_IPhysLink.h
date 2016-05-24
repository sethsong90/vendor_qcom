#ifndef DS_NET_IPHYSLINK_H
#define DS_NET_IPHYSLINK_H

/*============================================================================
  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
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
#define ds_Net_PhysLinkEvent_QDS_EV_STATE_CHANGED 0
typedef unsigned int ds_Net_PhysLinkStateType;
#define ds_Net_PhysLinkState_QDS_DOWN 0x1
#define ds_Net_PhysLinkState_QDS_RESUMING 0x2
#define ds_Net_PhysLinkState_QDS_UP 0x4
#define ds_Net_PhysLinkState_QDS_GOING_DOWN 0x8
#define ds_Net_AEEIID_IPhysLink 0x106d750

/** @interface ds_Net_IPhysLink
  * 
  * ds physical link interface.
  */
#define INHERIT_ds_Net_IPhysLink(iname) \
   INHERIT_ds_Net_IEventManager(iname); \
   AEEResult (*GoActive)(iname* _pif); \
   AEEResult (*GoDormant)(iname* _pif, ds_Net_DormantReasonType dormantReason); \
   AEEResult (*GetState)(iname* _pif, ds_Net_PhysLinkStateType* value); \
   AEEResult (*GetPreviousState)(iname* _pif, ds_Net_PhysLinkStateType* value)
AEEINTERFACE_DEFINE(ds_Net_IPhysLink);

/** @memberof ds_Net_IPhysLink
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IPhysLink_AddRef(ds_Net_IPhysLink* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IPhysLink)->AddRef(_pif);
}

/** @memberof ds_Net_IPhysLink
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IPhysLink_Release(ds_Net_IPhysLink* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IPhysLink)->Release(_pif);
}

/** @memberof ds_Net_IPhysLink
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IPhysLink_QueryInterface(ds_Net_IPhysLink* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IPhysLink)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IPhysLink
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
static __inline AEEResult ds_Net_IPhysLink_OnStateChange(ds_Net_IPhysLink* _pif, ISignal* signal, ds_Net_EventType eventID, IQI** regObj)
{
   return AEEGETPVTBL(_pif, ds_Net_IPhysLink)->OnStateChange(_pif, signal, eventID, regObj);
}

/** @memberof ds_Net_IPhysLink
  * 
  * This function is used to activate a dormant physical link.
  * @param _pif Pointer to interface
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IPhysLink_GoActive(ds_Net_IPhysLink* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IPhysLink)->GoActive(_pif);
}

/** @memberof ds_Net_IPhysLink
  * 
  * This function is used to explicitly put into dormancy the Physical
  * Link *regardless* of other entities that are using it when this
  * function is called.
  * Dormancy on a link implies the releasing of the traffic channel and
  * other air interface channels to free up appropriate radio resources.
  * The link is automatically brought out of dormancy when applicable
  * operations are applied on it.
  * This function requires special privilege. 
  * @param _pif Pointer to interface
  * @param dormantReason Reason why attempting to put the Physical Link
  * into dormancy.
  * @retval AEE_SUCCESS Request received successfully.
  * @retval AEE_EWOULDBLOCK Request is processed asynchronous.
  *                         Application may register for Physical Link
  *                         STATE_CHANGED events in order to identify
  *                         when the Physical Link is DOWN.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IPhysLink_GoDormant(ds_Net_IPhysLink* _pif, ds_Net_DormantReasonType dormantReason)
{
   return AEEGETPVTBL(_pif, ds_Net_IPhysLink)->GoDormant(_pif, dormantReason);
}

/** @memberof ds_Net_IPhysLink
  * 
  * This attribute represents the state of the physical link.
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @see ds_Net_PhysLink_StateType
  */
static __inline AEEResult ds_Net_IPhysLink_GetState(ds_Net_IPhysLink* _pif, ds_Net_PhysLinkStateType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPhysLink)->GetState(_pif, value);
}

/** @memberof ds_Net_IPhysLink
  * 
  * This attribute represents the previous phys link state.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @see ds_Net_PhysLink_StateType
  */
static __inline AEEResult ds_Net_IPhysLink_GetPreviousState(ds_Net_IPhysLink* _pif, ds_Net_PhysLinkStateType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPhysLink)->GetPreviousState(_pif, value);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "ds_Net_Def.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
namespace ds
{
   namespace Net
   {
      namespace PhysLinkEvent
      {
         const ::ds::Net::EventType QDS_EV_STATE_CHANGED = 0;
      };
      typedef unsigned int PhysLinkStateType;
      namespace PhysLinkState
      {
         const ::ds::Net::PhysLinkStateType QDS_DOWN = 0x1;
         const ::ds::Net::PhysLinkStateType QDS_RESUMING = 0x2;
         const ::ds::Net::PhysLinkStateType QDS_UP = 0x4;
         const ::ds::Net::PhysLinkStateType QDS_GOING_DOWN = 0x8;
      };
      const ::AEEIID AEEIID_IPhysLink = 0x106d750;
      
      /** @interface IPhysLink
        * 
        * ds physical link interface.
        */
      struct IPhysLink : public ::ds::Net::IEventManager
      {
         
         /**
           * This function is used to activate a dormant physical link.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GoActive() = 0;
         
         /**
           * This function is used to explicitly put into dormancy the Physical
           * Link *regardless* of other entities that are using it when this
           * function is called.
           * Dormancy on a link implies the releasing of the traffic channel and
           * other air interface channels to free up appropriate radio resources.
           * The link is automatically brought out of dormancy when applicable
           * operations are applied on it.
           * This function requires special privilege. 
           * @param dormantReason Reason why attempting to put the Physical Link
           * into dormancy.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval AEE_EWOULDBLOCK Request is processed asynchronous.
           *                         Application may register for Physical Link
           *                         STATE_CHANGED events in order to identify
           *                         when the Physical Link is DOWN.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GoDormant(::ds::Net::DormantReasonType dormantReason) = 0;
         
         /**
           * This attribute represents the state of the physical link.
           * @param value Attribute value
           * @see ds::Net::PhysLink::StateType
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetState(::ds::Net::PhysLinkStateType* value) = 0;
         
         /**
           * This attribute represents the previous phys link state.          
           * @param value Attribute value
           * @see ds::Net::PhysLink::StateType
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetPreviousState(::ds::Net::PhysLinkStateType* value) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IPHYSLINK_H
