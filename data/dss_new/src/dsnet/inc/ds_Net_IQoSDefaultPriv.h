#ifndef DS_NET_IQOSDEFAULTPRIV_H
#define DS_NET_IQOSDEFAULTPRIV_H

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
#include "ds_Net_IQoSPriv.h"
#include "ds_Net_Def.h"
#include "ds_Net_QoS_Def.h"
#include "ds_Addr_Def.h"
#include "ds_Net_IIPFilterPriv.h"
#include "ds_Net_QoS_DefPriv.h"
#define ds_Net_AEEIID_IQoSDefaultPriv 0x10a32b2

/** @interface ds_Net_IQoSDefaultPriv
  * 
  * ds Net QoS Default Private interface.
  */
#define INHERIT_ds_Net_IQoSDefaultPriv(iname) \
   INHERIT_ds_Net_IQoSPriv(iname); \
   AEEResult (*ModifyDefaultPriv)(iname* _pif, const ds_Net_QoSSpecPrimaryType* requestedSpec, ds_Net_QoSModifyMaskType mask)
AEEINTERFACE_DEFINE(ds_Net_IQoSDefaultPriv);

/** @memberof ds_Net_IQoSDefaultPriv
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IQoSDefaultPriv_AddRef(ds_Net_IQoSDefaultPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSDefaultPriv)->AddRef(_pif);
}

/** @memberof ds_Net_IQoSDefaultPriv
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IQoSDefaultPriv_Release(ds_Net_IQoSDefaultPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSDefaultPriv)->Release(_pif);
}

/** @memberof ds_Net_IQoSDefaultPriv
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IQoSDefaultPriv_QueryInterface(ds_Net_IQoSDefaultPriv* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSDefaultPriv)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IQoSDefaultPriv
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
static __inline AEEResult ds_Net_IQoSDefaultPriv_GetGrantedFlowSpecPriv(ds_Net_IQoSDefaultPriv* _pif, ds_Net_IQoSFlowPriv** rxFlow, ds_Net_IQoSFlowPriv** txFlow)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSDefaultPriv)->GetGrantedFlowSpecPriv(_pif, rxFlow, txFlow);
}

/** @memberof ds_Net_IQoSDefaultPriv
  * 
  * This function issues a request to modify the default QoS Flow specification 
  * in either Rx direction, Tx direction or both directions.
  * Note that this is a privileged operation.
  * @param _pif Pointer to interface
  * @param requestedSpec the QoS specification used for modifying the default QoS instance.
  * @param mask The modification direction, can be RX_FLOW, TX_FLOW or RX_FLOW|TX_FLOW.
  * @retval AEE_SUCCESS Modification request received successfully.
  * @retval ds_EBADOPTS Bad options.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IQoSDefaultPriv_ModifyDefaultPriv(ds_Net_IQoSDefaultPriv* _pif, const ds_Net_QoSSpecPrimaryType* requestedSpec, ds_Net_QoSModifyMaskType mask)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSDefaultPriv)->ModifyDefaultPriv(_pif, requestedSpec, mask);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Net_IQoSFlowPriv.h"
#include "ds_Net_IQoSPriv.h"
#include "ds_Net_Def.h"
#include "ds_Net_QoS_Def.h"
#include "ds_Addr_Def.h"
#include "ds_Net_IIPFilterPriv.h"
#include "ds_Net_QoS_DefPriv.h"
namespace ds
{
   namespace Net
   {
      const ::AEEIID AEEIID_IQoSDefaultPriv = 0x10a32b2;
      
      /** @interface IQoSDefaultPriv
        * 
        * ds Net QoS Default Private interface.
        */
      struct IQoSDefaultPriv : public ::ds::Net::IQoSPriv
      {
         
         /**
           * This function issues a request to modify the default QoS Flow specification 
           * in either Rx direction, Tx direction or both directions.
           * Note that this is a privileged operation.
           * @param requestedSpec the QoS specification used for modifying the default QoS instance.
           * @param mask The modification direction, can be RX_FLOW, TX_FLOW or RX_FLOW|TX_FLOW.
           * @retval AEE_SUCCESS Modification request received successfully.
           * @retval ds::EBADOPTS Bad options.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL ModifyDefaultPriv(const ::ds::Net::QoSSpecPrimaryType* requestedSpec, ::ds::Net::QoSModifyMaskType mask) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IQOSDEFAULTPRIV_H
