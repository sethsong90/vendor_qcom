#ifndef DS_NET_IQOSSECONDARIESOUTPUT_H
#define DS_NET_IQOSSECONDARIESOUTPUT_H

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
#define ds_Net_AEEIID_IQoSSecondariesOutput 0x109b9a7

/** @interface ds_Net_IQoSSecondariesOutput
  * 
  * ds Net IQoSSecondariesOutput interface.
  * This interface represents a bundle of QoS Secondary Sessions.
  * The Bundle holds a reference to its QoS Secondary objects.
  * See IQoSManager_Request.
  */
#define INHERIT_ds_Net_IQoSSecondariesOutput(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*GetNth)(iname* _pif, int index, ds_Net_IQoSSecondary** qosSecondary); \
   AEEResult (*GetnumElements)(iname* _pif, int* value)
AEEINTERFACE_DEFINE(ds_Net_IQoSSecondariesOutput);

/** @memberof ds_Net_IQoSSecondariesOutput
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IQoSSecondariesOutput_AddRef(ds_Net_IQoSSecondariesOutput* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondariesOutput)->AddRef(_pif);
}

/** @memberof ds_Net_IQoSSecondariesOutput
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IQoSSecondariesOutput_Release(ds_Net_IQoSSecondariesOutput* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondariesOutput)->Release(_pif);
}

/** @memberof ds_Net_IQoSSecondariesOutput
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IQoSSecondariesOutput_QueryInterface(ds_Net_IQoSSecondariesOutput* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondariesOutput)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IQoSSecondariesOutput
  *     
  * This function provide the Nth IQoSSecondary object of the bundle.
  * The order of the objects corresponds to the order of QoS Specs
  * provided to IQoSManager_Request.          
  * @param _pif Pointer to interface
  * @param qosSecondary The Nth QoS Secondary object of the bundle.
  * @param index number of Session in the bundle. Zero based.
  * @retval ds_SUCCESS The Nth QoS Secondary is successfully provided.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  * @See IQoSManager_Request.
  */
static __inline AEEResult ds_Net_IQoSSecondariesOutput_GetNth(ds_Net_IQoSSecondariesOutput* _pif, int index, ds_Net_IQoSSecondary** qosSecondary)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondariesOutput)->GetNth(_pif, index, qosSecondary);
}

/** @memberof ds_Net_IQoSSecondariesOutput
  * 
  * Number of elements in the bundle.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IQoSSecondariesOutput_GetnumElements(ds_Net_IQoSSecondariesOutput* _pif, int* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondariesOutput)->GetnumElements(_pif, value);
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
      const ::AEEIID AEEIID_IQoSSecondariesOutput = 0x109b9a7;
      
      /** @interface IQoSSecondariesOutput
        * 
        * ds Net IQoSSecondariesOutput interface.
        * This interface represents a bundle of QoS Secondary Sessions.
        * The Bundle holds a reference to its QoS Secondary objects.
        * See IQoSManager::Request.
        */
      struct IQoSSecondariesOutput : public ::IQI
      {
         
         /**    
           * This function provide the Nth IQoSSecondary object of the bundle.
           * The order of the objects corresponds to the order of QoS Specs
           * provided to IQoSManager::Request.          
           * @param qosSecondary The Nth QoS Secondary object of the bundle.
           * @param index number of Session in the bundle. Zero based.
           * @retval ds::SUCCESS The Nth QoS Secondary is successfully provided.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           * @See IQoSManager::Request.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetNth(int index, ::ds::Net::IQoSSecondary** qosSecondary) = 0;
         
         /**
           * Number of elements in the bundle.          
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetnumElements(int* value) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IQOSSECONDARIESOUTPUT_H
