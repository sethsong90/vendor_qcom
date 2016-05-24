#ifndef DS_NET_IMCASTSESSIONSINPUT_H
#define DS_NET_IMCASTSESSIONSINPUT_H

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
#define ds_Net_AEEIID_IMCastSessionsInput 0x109b66a

/** @interface ds_Net_IMCastSessionsInput
  * 
  * ds Net IMCastSessionsInput interface.
  * This interface represents a group of MCast Sessions.
  * The Group does not hold a reference to the MCast Sessions objects.
  * A MCast Session object is disassociated from the group upon its
  * destruction.
  * @See IMCastManager_CreateMCastSessionsInput.
  * @See IMCastManager_JoinBundle.
  * @See IMCastManager_LeaveBundle.*
  * @See IMCastManager_RegisterBundle.
  */
#define INHERIT_ds_Net_IMCastSessionsInput(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*Associate)(iname* _pif, ds_Net_IMCastSession* mcastSession); \
   AEEResult (*Disassociate)(iname* _pif, ds_Net_IMCastSession* mcastSession)
AEEINTERFACE_DEFINE(ds_Net_IMCastSessionsInput);

/** @memberof ds_Net_IMCastSessionsInput
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IMCastSessionsInput_AddRef(ds_Net_IMCastSessionsInput* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSessionsInput)->AddRef(_pif);
}

/** @memberof ds_Net_IMCastSessionsInput
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IMCastSessionsInput_Release(ds_Net_IMCastSessionsInput* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSessionsInput)->Release(_pif);
}

/** @memberof ds_Net_IMCastSessionsInput
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IMCastSessionsInput_QueryInterface(ds_Net_IMCastSessionsInput* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSessionsInput)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IMCastSessionsInput
  *     
  * This function associates a mcastSession with the group.
  * @param _pif Pointer to interface
  * @param mcastSession MCast Session to be associated with the group.
  * @retval ds_SUCCESS Mcast Session successfully associated.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.          
  */
static __inline AEEResult ds_Net_IMCastSessionsInput_Associate(ds_Net_IMCastSessionsInput* _pif, ds_Net_IMCastSession* mcastSession)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSessionsInput)->Associate(_pif, mcastSession);
}

/** @memberof ds_Net_IMCastSessionsInput
  *     
  * This function disassociates a mcastSession from the group.
  * @param _pif Pointer to interface
  * @param mcastSession MCast Session to be disassociated from the group.
  * @retval ds_SUCCESS Mcast Session successfully disassociated.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.          
  */
static __inline AEEResult ds_Net_IMCastSessionsInput_Disassociate(ds_Net_IMCastSessionsInput* _pif, ds_Net_IMCastSession* mcastSession)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastSessionsInput)->Disassociate(_pif, mcastSession);
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
      const ::AEEIID AEEIID_IMCastSessionsInput = 0x109b66a;
      
      /** @interface IMCastSessionsInput
        * 
        * ds Net IMCastSessionsInput interface.
        * This interface represents a group of MCast Sessions.
        * The Group does not hold a reference to the MCast Sessions objects.
        * A MCast Session object is disassociated from the group upon its
        * destruction.
        * @See IMCastManager::CreateMCastSessionsInput.
        * @See IMCastManager::JoinBundle.
        * @See IMCastManager::LeaveBundle.*
        * @See IMCastManager::RegisterBundle.
        */
      struct IMCastSessionsInput : public ::IQI
      {
         
         /**    
           * This function associates a mcastSession with the group.
           * @param mcastSession MCast Session to be associated with the group.
           * @retval ds::SUCCESS Mcast Session successfully associated.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.          
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Associate(::ds::Net::IMCastSession* mcastSession) = 0;
         
         /**    
           * This function disassociates a mcastSession from the group.
           * @param mcastSession MCast Session to be disassociated from the group.
           * @retval ds::SUCCESS Mcast Session successfully disassociated.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.          
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Disassociate(::ds::Net::IMCastSession* mcastSession) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IMCASTSESSIONSINPUT_H
