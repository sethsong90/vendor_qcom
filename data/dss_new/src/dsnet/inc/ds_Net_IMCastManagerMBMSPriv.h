#ifndef DS_NET_IMCASTMANAGERMBMSPRIV_H
#define DS_NET_IMCASTMANAGERMBMSPRIV_H

/*============================================================================
  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Addr_Def.h"
#include "AEEISignal.h"
#include "ds_Net_Def.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_MCast_Def.h"
#include "ds_Net_IMCastSession.h"
#include "ds_Net_DownReasons_Def.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_INetwork.h"
#include "ds_Net_IMCastMBMSCtrlPriv.h"
typedef int ds_Net_MCastInfoCodeTypePriv;

/** @memberof ds_Net_MCastInfoCode
  * 
  * MCastInfoCodeTypePriv is part of the enumeration in MCastInfoCodeType.
  * MBMS related info code is moved here until MBMS feature is released
  * publicly.
  */
#define ds_Net_MCastInfoCode_MBMS_SYSTEM_UNAVAILABLE 1151
#define ds_Net_AEEIID_IMCastManagerMBMSPriv 0x106df81

/** @interface ds_Net_IMCastManagerMBMSPriv
  * 
  * ds Net MCast Manager MBMS interface.
  * This Object is received from IMCastManagerPriv via Query Interface.
  */
#define INHERIT_ds_Net_IMCastManagerMBMSPriv(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*Activate)(iname* _pif, const ds_IPAddrType* addr, int pdpNumber, ds_Net_IMCastMBMSCtrlPriv** ppMCastMBMSCtrlPriv)
AEEINTERFACE_DEFINE(ds_Net_IMCastManagerMBMSPriv);

/** @memberof ds_Net_IMCastManagerMBMSPriv
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IMCastManagerMBMSPriv_AddRef(ds_Net_IMCastManagerMBMSPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerMBMSPriv)->AddRef(_pif);
}

/** @memberof ds_Net_IMCastManagerMBMSPriv
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IMCastManagerMBMSPriv_Release(ds_Net_IMCastManagerMBMSPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerMBMSPriv)->Release(_pif);
}

/** @memberof ds_Net_IMCastManagerMBMSPriv
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IMCastManagerMBMSPriv_QueryInterface(ds_Net_IMCastManagerMBMSPriv* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerMBMSPriv)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IMCastManagerMBMSPriv
  * 
  * This function is used to setup the MBMS context over the air.
  * @param _pif Pointer to interface
  * @param addr Multicast address to be used.
  * @param pdpNumber PDP number to be used.
  * @see ds_IPAddrType.
  * @retval ds_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IMCastManagerMBMSPriv_Activate(ds_Net_IMCastManagerMBMSPriv* _pif, const ds_IPAddrType* addr, int pdpNumber, ds_Net_IMCastMBMSCtrlPriv** ppMCastMBMSCtrlPriv)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerMBMSPriv)->Activate(_pif, addr, pdpNumber, ppMCastMBMSCtrlPriv);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Addr_Def.h"
#include "AEEISignal.h"
#include "ds_Net_Def.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_MCast_Def.h"
#include "ds_Net_IMCastSession.h"
#include "ds_Net_DownReasons_Def.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_INetwork.h"
#include "ds_Net_IMCastMBMSCtrlPriv.h"
namespace ds
{
   namespace Net
   {
      typedef int MCastInfoCodeTypePriv;
      namespace MCastInfoCode
      {
         
         /**
           * MCastInfoCodeTypePriv is part of the enumeration in MCastInfoCodeType.
           * MBMS related info code is moved here until MBMS feature is released
           * publicly.
           */
         const ::ds::Net::MCastInfoCodeTypePriv MBMS_SYSTEM_UNAVAILABLE = 1151;
      };
      const ::AEEIID AEEIID_IMCastManagerMBMSPriv = 0x106df81;
      
      /** @interface IMCastManagerMBMSPriv
        * 
        * ds Net MCast Manager MBMS interface.
        * This Object is received from IMCastManagerPriv via Query Interface.
        */
      struct IMCastManagerMBMSPriv : public ::IQI
      {
         
         /**
           * This function is used to setup the MBMS context over the air.
           * @param addr Multicast address to be used.
           * @param pdpNumber PDP number to be used.
           * @see ds::IPAddrType.
           * @retval ds::SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Activate(const ::ds::IPAddrType* addr, int pdpNumber, ::ds::Net::IMCastMBMSCtrlPriv** ppMCastMBMSCtrlPriv) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IMCASTMANAGERMBMSPRIV_H
