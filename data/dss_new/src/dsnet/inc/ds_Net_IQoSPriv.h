#ifndef DS_NET_IQOSPRIV_H
#define DS_NET_IQOSPRIV_H

/*============================================================================
  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Net_IQoSFlowPriv.h"
#define ds_Net_AEEIID_IQoSPriv 0x10a32b0

/** @interface ds_Net_IQoSPriv
  * 
  * ds Net QoS Private interface.
  */
#define INHERIT_ds_Net_IQoSPriv(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*GetGrantedFlowSpecPriv)(iname* _pif, ds_Net_IQoSFlowPriv** rxFlow, ds_Net_IQoSFlowPriv** txFlow)
AEEINTERFACE_DEFINE(ds_Net_IQoSPriv);

/** @memberof ds_Net_IQoSPriv
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IQoSPriv_AddRef(ds_Net_IQoSPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSPriv)->AddRef(_pif);
}

/** @memberof ds_Net_IQoSPriv
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IQoSPriv_Release(ds_Net_IQoSPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSPriv)->Release(_pif);
}

/** @memberof ds_Net_IQoSPriv
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IQoSPriv_QueryInterface(ds_Net_IQoSPriv* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSPriv)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IQoSPriv
  * 
  * This function returns the current Rx and Tx flow specification.
  * @param _pif Pointer to interface
  * @param rxFlow the current Rx flow specification.
  * @param txFlow the current Tx flow specification.
  * @retval AEE_SUCCESS Request received successfully.
  * @retval ds_EBADOPTS Bad options.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IQoSPriv_GetGrantedFlowSpecPriv(ds_Net_IQoSPriv* _pif, ds_Net_IQoSFlowPriv** rxFlow, ds_Net_IQoSFlowPriv** txFlow)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSPriv)->GetGrantedFlowSpecPriv(_pif, rxFlow, txFlow);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Net_IQoSFlowPriv.h"
namespace ds
{
   namespace Net
   {
      const ::AEEIID AEEIID_IQoSPriv = 0x10a32b0;
      
      /** @interface IQoSPriv
        * 
        * ds Net QoS Private interface.
        */
      struct IQoSPriv : public ::IQI
      {
         
         /**
           * This function returns the current Rx and Tx flow specification.
           * @param rxFlow the current Rx flow specification.
           * @param txFlow the current Tx flow specification.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval ds::EBADOPTS Bad options.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetGrantedFlowSpecPriv(::ds::Net::IQoSFlowPriv** rxFlow, ::ds::Net::IQoSFlowPriv** txFlow) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IQOSPRIV_H
