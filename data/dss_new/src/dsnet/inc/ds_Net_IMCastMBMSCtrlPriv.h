#ifndef DS_NET_IMCASTMBMSCTRLPRIV_H
#define DS_NET_IMCASTMBMSCTRLPRIV_H

/*============================================================================
  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Net_DownReasons_Def.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_INetwork.h"
#define ds_Net_MBMSEvent_QDS_EV_ACTIVATE_STATE 0x106e618
typedef int ds_Net_MBMSStateType;
#define ds_Net_MBMSState_QDS_ACTIVATED 0
#define ds_Net_MBMSState_QDS_DEACTIVATED 1
#define ds_Net_AEEIID_IMCastMBMSCtrlPriv 0x107a97c

/** @interface ds_Net_IMCastMBMSCtrlPriv
  * 
  * ds Net MBMS Control interface.
  * Events that can be registered on this interface via OnStateChange (as part of IEventManager interface): 
  * - ds_Net_MBMSEvent_ACTIVATE_STATE. Use GetState to get activate state information.
  */
#define INHERIT_ds_Net_IMCastMBMSCtrlPriv(iname) \
   INHERIT_ds_Net_IEventManager(iname); \
   AEEResult (*DeActivate)(iname* _pif); \
   AEEResult (*GetState)(iname* _pif, ds_Net_MBMSStateType* state)
AEEINTERFACE_DEFINE(ds_Net_IMCastMBMSCtrlPriv);

/** @memberof ds_Net_IMCastMBMSCtrlPriv
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IMCastMBMSCtrlPriv_AddRef(ds_Net_IMCastMBMSCtrlPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastMBMSCtrlPriv)->AddRef(_pif);
}

/** @memberof ds_Net_IMCastMBMSCtrlPriv
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IMCastMBMSCtrlPriv_Release(ds_Net_IMCastMBMSCtrlPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastMBMSCtrlPriv)->Release(_pif);
}

/** @memberof ds_Net_IMCastMBMSCtrlPriv
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IMCastMBMSCtrlPriv_QueryInterface(ds_Net_IMCastMBMSCtrlPriv* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastMBMSCtrlPriv)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IMCastMBMSCtrlPriv
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
static __inline AEEResult ds_Net_IMCastMBMSCtrlPriv_OnStateChange(ds_Net_IMCastMBMSCtrlPriv* _pif, ISignal* signal, ds_Net_EventType eventID, IQI** regObj)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastMBMSCtrlPriv)->OnStateChange(_pif, signal, eventID, regObj);
}

/** @memberof ds_Net_IMCastMBMSCtrlPriv
  * 
  * This function is used to deactivate the MBMS context over the air.
  * @param _pif Pointer to interface
  * @retval ds_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IMCastMBMSCtrlPriv_DeActivate(ds_Net_IMCastMBMSCtrlPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastMBMSCtrlPriv)->DeActivate(_pif);
}

/** @memberof ds_Net_IMCastMBMSCtrlPriv
  * 
  * This function is used to retrieve the state of the MBMS context.
  * @param _pif Pointer to interface
  * @param state structure that will hold the MBMS context state.
  * @see ds_Net_MBMS_StateType.
  * @retval ds_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IMCastMBMSCtrlPriv_GetState(ds_Net_IMCastMBMSCtrlPriv* _pif, ds_Net_MBMSStateType* state)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastMBMSCtrlPriv)->GetState(_pif, state);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Net_DownReasons_Def.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_INetwork.h"
namespace ds
{
   namespace Net
   {
      namespace MBMSEvent
      {
         const ::ds::Net::EventType QDS_EV_ACTIVATE_STATE = 0x106e618;
      };
      typedef int MBMSStateType;
      namespace MBMSState
      {
         const ::ds::Net::MBMSStateType QDS_ACTIVATED = 0;
         const ::ds::Net::MBMSStateType QDS_DEACTIVATED = 1;
      };
      const ::AEEIID AEEIID_IMCastMBMSCtrlPriv = 0x107a97c;
      
      /** @interface IMCastMBMSCtrlPriv
        * 
        * ds Net MBMS Control interface.
        * Events that can be registered on this interface via OnStateChange (as part of IEventManager interface): 
        * - ds::Net::MBMSEvent::ACTIVATE_STATE. Use GetState to get activate state information.
        */
      struct IMCastMBMSCtrlPriv : public ::ds::Net::IEventManager
      {
         
         /**
           * This function is used to deactivate the MBMS context over the air.
           * @retval ds::SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL DeActivate() = 0;
         
         /**
           * This function is used to retrieve the state of the MBMS context.
           * @param state structure that will hold the MBMS context state.
           * @see ds::Net::MBMS::StateType.
           * @retval ds::SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetState(::ds::Net::MBMSStateType* state) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IMCASTMBMSCTRLPRIV_H
