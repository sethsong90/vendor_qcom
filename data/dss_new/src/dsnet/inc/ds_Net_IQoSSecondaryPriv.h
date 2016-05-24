#ifndef DS_NET_IQOSSECONDARYPRIV_H
#define DS_NET_IQOSSECONDARYPRIV_H

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
#define ds_Net_AEEIID_IQoSSecondaryPriv 0x10a32b3

/** @interface ds_Net_IQoSSecondaryPriv
  * 
  * ds QoS Secondary Private interface.
  */
#define INHERIT_ds_Net_IQoSSecondaryPriv(iname) \
   INHERIT_ds_Net_IQoSPriv(iname); \
   AEEResult (*ModifySecondaryPriv)(iname* _pif, const ds_Net_QoSSpecType* requestedSpec, ds_Net_QoSModifyMaskType modifyMask)
AEEINTERFACE_DEFINE(ds_Net_IQoSSecondaryPriv);

/** @memberof ds_Net_IQoSSecondaryPriv
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IQoSSecondaryPriv_AddRef(ds_Net_IQoSSecondaryPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondaryPriv)->AddRef(_pif);
}

/** @memberof ds_Net_IQoSSecondaryPriv
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IQoSSecondaryPriv_Release(ds_Net_IQoSSecondaryPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondaryPriv)->Release(_pif);
}

/** @memberof ds_Net_IQoSSecondaryPriv
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IQoSSecondaryPriv_QueryInterface(ds_Net_IQoSSecondaryPriv* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondaryPriv)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IQoSSecondaryPriv
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
static __inline AEEResult ds_Net_IQoSSecondaryPriv_GetGrantedFlowSpecPriv(ds_Net_IQoSSecondaryPriv* _pif, ds_Net_IQoSFlowPriv** rxFlow, ds_Net_IQoSFlowPriv** txFlow)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondaryPriv)->GetGrantedFlowSpecPriv(_pif, rxFlow, txFlow);
}

/** @memberof ds_Net_IQoSSecondaryPriv
  * 
  * This function issues the following modification requests:
  *  - Modify an existing QoS Session either in Rx, Tx or both directions.
  *  - Add a new QoS in a specific direction.
  *  - Remove a granted QoS in a specific direction.
  * The modification requests can be performed on Rx blocks, Tx blocks or both.
  * @param _pif Pointer to interface
  * @param requestedSpec the QoS specification used for modifying the QoS instance
  * @param modifyMask bit mask which indicates the QoS specification blocks to be modified 
  * @retval AEE_SUCCESS Request received successfully.
  * @retval ds_EBADOPTS Bad options.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IQoSSecondaryPriv_ModifySecondaryPriv(ds_Net_IQoSSecondaryPriv* _pif, const ds_Net_QoSSpecType* requestedSpec, ds_Net_QoSModifyMaskType modifyMask)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSSecondaryPriv)->ModifySecondaryPriv(_pif, requestedSpec, modifyMask);
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
      const ::AEEIID AEEIID_IQoSSecondaryPriv = 0x10a32b3;
      
      /** @interface IQoSSecondaryPriv
        * 
        * ds QoS Secondary Private interface.
        */
      struct IQoSSecondaryPriv : public ::ds::Net::IQoSPriv
      {
         
         /**
           * This function issues the following modification requests:
           *  - Modify an existing QoS Session either in Rx, Tx or both directions.
           *  - Add a new QoS in a specific direction.
           *  - Remove a granted QoS in a specific direction.
           * The modification requests can be performed on Rx blocks, Tx blocks or both.
           * @param requestedSpec the QoS specification used for modifying the QoS instance
           * @param modifyMask bit mask which indicates the QoS specification blocks to be modified 
           * @retval AEE_SUCCESS Request received successfully.
           * @retval ds::EBADOPTS Bad options.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL ModifySecondaryPriv(const ::ds::Net::QoSSpecType* requestedSpec, ::ds::Net::QoSModifyMaskType modifyMask) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IQOSSECONDARYPRIV_H
