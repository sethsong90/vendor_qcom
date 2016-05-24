#ifndef DS_NET_IQOSSECONDARIESINPUT_H
#define DS_NET_IQOSSECONDARIESINPUT_H

/*============================================================================
  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Net_Def.h"
#include "ds_Net_QoS_Def.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_IQoS.h"
#include "ds_Net_IQoSSecondary.h"
#define ds_Net_AEEIID_IQoSSecondariesInput 0x109b6d0

/** @interface ds_Net_IQoSSecondariesInput
  * 
  * ds Net IQoSSecondariesInput interface.
  * This interface represents a group of QoS Secondary Sessions.
  * The Group does not hold a reference to the QoS Secondary objects.
  * A QoS Secondary object is disassociated from the group upon its
  * destruction.
  * @See IQoSManager_Close, IQoSManager_Resume, 
  *      IQoSManager_Suspend.
  */
#define INHERIT_ds_Net_IQoSSecondariesInput(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*Associate)(iname* _pif, ds_Net_IQoSSecondary* qosSecondary); \
   AEEResult (*Disassociate)(iname* _pif, ds_Net_IQoSSecondary* qosSecondary)
AEEINTERFACE_DEFINE(ds_Net_IQoSSecondariesInput);

/** @memberof ds_Net_IQoSSecondariesInput
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IQoSSecondariesInput_AddRef(ds_Net_IQoSSecondariesInput* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondariesInput)->AddRef(_pif);
}

/** @memberof ds_Net_IQoSSecondariesInput
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IQoSSecondariesInput_Release(ds_Net_IQoSSecondariesInput* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondariesInput)->Release(_pif);
}

/** @memberof ds_Net_IQoSSecondariesInput
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IQoSSecondariesInput_QueryInterface(ds_Net_IQoSSecondariesInput* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondariesInput)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IQoSSecondariesInput
  *     
  * This function associates a QoS Secondary with the group.
  * @param _pif Pointer to interface
  * @param qosSecondary QoS Secondary to be associated with the group.
  * @retval ds_SUCCESS QoS Secondary successfully associated.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IQoSSecondariesInput_Associate(ds_Net_IQoSSecondariesInput* _pif, ds_Net_IQoSSecondary* qosSecondary)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondariesInput)->Associate(_pif, qosSecondary);
}

/** @memberof ds_Net_IQoSSecondariesInput
  *     
  * This function disassociates a QoS Secondary from the group.
  * @param _pif Pointer to interface
  * @param qosSecondary QoS Secondary to be disassociated from the group.
  * @retval ds_SUCCESS QoS Secondary successfully disassociated.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IQoSSecondariesInput_Disassociate(ds_Net_IQoSSecondariesInput* _pif, ds_Net_IQoSSecondary* qosSecondary)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondariesInput)->Disassociate(_pif, qosSecondary);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Net_Def.h"
#include "ds_Net_QoS_Def.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_IQoS.h"
#include "ds_Net_IQoSSecondary.h"
namespace ds
{
   namespace Net
   {
      const ::AEEIID AEEIID_IQoSSecondariesInput = 0x109b6d0;
      
      /** @interface IQoSSecondariesInput
        * 
        * ds Net IQoSSecondariesInput interface.
        * This interface represents a group of QoS Secondary Sessions.
        * The Group does not hold a reference to the QoS Secondary objects.
        * A QoS Secondary object is disassociated from the group upon its
        * destruction.
        * @See IQoSManager::Close, IQoSManager::Resume, 
        *      IQoSManager::Suspend.
        */
      struct IQoSSecondariesInput : public ::IQI
      {
         
         /**    
           * This function associates a QoS Secondary with the group.
           * @param qosSecondary QoS Secondary to be associated with the group.
           * @retval ds::SUCCESS QoS Secondary successfully associated.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Associate(::ds::Net::IQoSSecondary* qosSecondary) = 0;
         
         /**    
           * This function disassociates a QoS Secondary from the group.
           * @param qosSecondary QoS Secondary to be disassociated from the group.
           * @retval ds::SUCCESS QoS Secondary successfully disassociated.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Disassociate(::ds::Net::IQoSSecondary* qosSecondary) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IQOSSECONDARIESINPUT_H
