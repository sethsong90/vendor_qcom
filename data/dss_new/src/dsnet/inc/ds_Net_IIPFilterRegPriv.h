#ifndef DS_NET_IIPFILTERREGPRIV_H
#define DS_NET_IIPFILTERREGPRIV_H

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
#define ds_Net_IPFilterEvent_QDS_EV_STATE_CHANGED 0x1098bc7
#define ds_Net_AEEIID_IIPFilterRegPriv 0x107d26f

/** @interface ds_Net_IIPFilterRegPriv
  * 
  * ds Filter Registration interface.
  * This interface derives from IEventManager to get the event registration functions.
  * This interface can be used to register for IP FILTER STATE_CHANGED Event.
  * The release of this interface deregister the Filters installed.
  * IMPORTANT: If and when this functionality is made public (i.e. for extenral
  *            apps, this interface will have to be changed / used differently
  *            as it merely redefines IEventManager. The release of filters
  *            may have to be done in a different manner.
  */
#define INHERIT_ds_Net_IIPFilterRegPriv(iname) \
   INHERIT_ds_Net_IEventManager(iname)
AEEINTERFACE_DEFINE(ds_Net_IIPFilterRegPriv);

/** @memberof ds_Net_IIPFilterRegPriv
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IIPFilterRegPriv_AddRef(ds_Net_IIPFilterRegPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterRegPriv)->AddRef(_pif);
}

/** @memberof ds_Net_IIPFilterRegPriv
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IIPFilterRegPriv_Release(ds_Net_IIPFilterRegPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterRegPriv)->Release(_pif);
}

/** @memberof ds_Net_IIPFilterRegPriv
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IIPFilterRegPriv_QueryInterface(ds_Net_IIPFilterRegPriv* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterRegPriv)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IIPFilterRegPriv
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
static __inline AEEResult ds_Net_IIPFilterRegPriv_OnStateChange(ds_Net_IIPFilterRegPriv* _pif, ISignal* signal, ds_Net_EventType eventID, IQI** regObj)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterRegPriv)->OnStateChange(_pif, signal, eventID, regObj);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_Def.h"
#include "ds_Net_IEventManager.h"
namespace ds
{
   namespace Net
   {
      namespace IPFilterEvent
      {
         const ::ds::Net::EventType QDS_EV_STATE_CHANGED = 0x1098bc7;
      };
      const ::AEEIID AEEIID_IIPFilterRegPriv = 0x107d26f;
      
      /** @interface IIPFilterRegPriv
        * 
        * ds Filter Registration interface.
        * This interface derives from IEventManager to get the event registration functions.
        * This interface can be used to register for IP FILTER STATE_CHANGED Event.
        * The release of this interface deregister the Filters installed.
        * IMPORTANT: If and when this functionality is made public (i.e. for extenral
        *            apps, this interface will have to be changed / used differently
        *            as it merely redefines IEventManager. The release of filters
        *            may have to be done in a different manner.
        */
      struct IIPFilterRegPriv : public ::ds::Net::IEventManager
      {
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IIPFILTERREGPRIV_H
