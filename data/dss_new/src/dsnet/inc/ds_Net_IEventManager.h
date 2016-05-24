#ifndef DS_NET_IEVENTMANAGER_H
#define DS_NET_IEVENTMANAGER_H

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
#define ds_Net_AEEIID_IEventManager 0x106d860

/** @interface ds_Net_IEventManager
  * 
  * ds Net Handle interface.
  */
#define INHERIT_ds_Net_IEventManager(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*OnStateChange)(iname* _pif, ISignal* signal, ds_Net_EventType eventID, IQI** regObj)
AEEINTERFACE_DEFINE(ds_Net_IEventManager);

/** @memberof ds_Net_IEventManager
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IEventManager_AddRef(ds_Net_IEventManager* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IEventManager)->AddRef(_pif);
}

/** @memberof ds_Net_IEventManager
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IEventManager_Release(ds_Net_IEventManager* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IEventManager)->Release(_pif);
}

/** @memberof ds_Net_IEventManager
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IEventManager_QueryInterface(ds_Net_IEventManager* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IEventManager)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IEventManager
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
static __inline AEEResult ds_Net_IEventManager_OnStateChange(ds_Net_IEventManager* _pif, ISignal* signal, ds_Net_EventType eventID, IQI** regObj)
{
   return AEEGETPVTBL(_pif, ds_Net_IEventManager)->OnStateChange(_pif, signal, eventID, regObj);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_Def.h"
namespace ds
{
   namespace Net
   {
      const ::AEEIID AEEIID_IEventManager = 0x106d860;
      
      /** @interface IEventManager
        * 
        * ds Net Handle interface.
        */
      struct IEventManager : public ::IQI
      {
         
         /**
           * This function registers a signal to be set when event eventID occurs.
           * Cannot use the signal for more than one eventID. Can use more than one
           * signal for the same event.
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
         virtual ::AEEResult AEEINTERFACE_CDECL OnStateChange(::ISignal* signal, ::ds::Net::EventType eventID, ::IQI** regObj) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IEVENTMANAGER_H
