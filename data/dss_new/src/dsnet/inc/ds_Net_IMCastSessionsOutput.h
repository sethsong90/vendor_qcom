#ifndef DS_NET_IMCASTSESSIONSOUTPUT_H
#define DS_NET_IMCASTSESSIONSOUTPUT_H

/*============================================================================
  Copyright (c) 2010 Qualcomm Technologies, Inc.
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
#include "ds_Net_IMCastSession.h"
#define ds_Net_AEEIID_IMCastSessionsOutput 0x109b9a8

/** @interface ds_Net_IMCastSessionsOutput
  * 
  * ds Net IMCastSessionsOutput interface.
  * This interface represents a bundle of MCast Sessions.
  * The Bundle holds a reference to its MCast Sessions objects.
  * See IMCastManager_JoinBundle.
  */
#define INHERIT_ds_Net_IMCastSessionsOutput(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*GetNth)(iname* _pif, int index, ds_Net_IMCastSession** mcastSession); \
   AEEResult (*GetnumElements)(iname* _pif, int* value)
AEEINTERFACE_DEFINE(ds_Net_IMCastSessionsOutput);

/** @memberof ds_Net_IMCastSessionsOutput
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IMCastSessionsOutput_AddRef(ds_Net_IMCastSessionsOutput* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSessionsOutput)->AddRef(_pif);
}

/** @memberof ds_Net_IMCastSessionsOutput
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IMCastSessionsOutput_Release(ds_Net_IMCastSessionsOutput* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSessionsOutput)->Release(_pif);
}

/** @memberof ds_Net_IMCastSessionsOutput
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IMCastSessionsOutput_QueryInterface(ds_Net_IMCastSessionsOutput* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSessionsOutput)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IMCastSessionsOutput
  *     
  * This function provide the Nth IMCastSession object of the bundle.
  * The order of the objects corresponds to the order of MCast addresses
  * provided to IMCastManager_JoinBundle.          
  * @param _pif Pointer to interface
  * @param mcastSession The Nth Mcast session object of the bundle.
  * @param index number of Session in the bundle. Zero based.
  * @retval ds_SUCCESS The Nth Mcast session is successfully provided.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  * @See IMCastManager_JoinBundle.
  */
static __inline AEEResult ds_Net_IMCastSessionsOutput_GetNth(ds_Net_IMCastSessionsOutput* _pif, int index, ds_Net_IMCastSession** mcastSession)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSessionsOutput)->GetNth(_pif, index, mcastSession);
}

/** @memberof ds_Net_IMCastSessionsOutput
  * 
  * Number of elements in the bundle.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IMCastSessionsOutput_GetnumElements(ds_Net_IMCastSessionsOutput* _pif, int* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSessionsOutput)->GetnumElements(_pif, value);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_Def.h"
#include "ds_Net_IEventManager.h"
#include "ds_Addr_Def.h"
#include "ds_Net_MCast_Def.h"
#include "ds_Net_IMCastSession.h"
namespace ds
{
   namespace Net
   {
      const ::AEEIID AEEIID_IMCastSessionsOutput = 0x109b9a8;
      
      /** @interface IMCastSessionsOutput
        * 
        * ds Net IMCastSessionsOutput interface.
        * This interface represents a bundle of MCast Sessions.
        * The Bundle holds a reference to its MCast Sessions objects.
        * See IMCastManager::JoinBundle.
        */
      struct IMCastSessionsOutput : public ::IQI
      {
         
         /**    
           * This function provide the Nth IMCastSession object of the bundle.
           * The order of the objects corresponds to the order of MCast addresses
           * provided to IMCastManager::JoinBundle.          
           * @param mcastSession The Nth Mcast session object of the bundle.
           * @param index number of Session in the bundle. Zero based.
           * @retval ds::SUCCESS The Nth Mcast session is successfully provided.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           * @See IMCastManager::JoinBundle.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetNth(int index, ::ds::Net::IMCastSession** mcastSession) = 0;
         
         /**
           * Number of elements in the bundle.          
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetnumElements(int* value) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IMCASTSESSIONSOUTPUT_H
