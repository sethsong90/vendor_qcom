#ifndef DS_NET_IQOSMANAGERPRIV_H
#define DS_NET_IQOSMANAGERPRIV_H

/*============================================================================
  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "ds_Net_Def.h"
#include "ds_Net_QoS_Def.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_IQoS.h"
#include "ds_Net_IQoSSecondary.h"
#include "ds_Net_IQoSFlowPriv.h"
#include "ds_Addr_Def.h"
#include "ds_Net_IIPFilterPriv.h"
#include "ds_Net_QoS_DefPriv.h"
#include "ds_Net_IQoSSecondariesInput.h"
#include "ds_Net_IQoSSecondariesOutput.h"
#define ds_Net_AEEIID_IQoSManagerPriv 0x10a32b1

/** @interface ds_Net_IQoSManagerPriv
  * 
  * ds Net QoS Manager Private interface.
  */
#define INHERIT_ds_Net_IQoSManagerPriv(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*RequestSecondary)(iname* _pif, const ds_Net_QoSSpecType* requestedQoSSpec, ds_Net_IQoSSecondary** session); \
   AEEResult (*RequestBundle)(iname* _pif, const ds_Net_QoSSpecType* specs, int specsLen, ds_Net_QoSRequestOpCodeType opCode, ds_Net_IQoSSecondariesOutput** sessions); \
   AEEResult (*Close)(iname* _pif, ds_Net_IQoSSecondariesInput* qosSessions); \
   AEEResult (*Resume)(iname* _pif, ds_Net_IQoSSecondariesInput* qosSessions); \
   AEEResult (*Suspend)(iname* _pif, ds_Net_IQoSSecondariesInput* qosSessions)
AEEINTERFACE_DEFINE(ds_Net_IQoSManagerPriv);

/** @memberof ds_Net_IQoSManagerPriv
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IQoSManagerPriv_AddRef(ds_Net_IQoSManagerPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSManagerPriv)->AddRef(_pif);
}

/** @memberof ds_Net_IQoSManagerPriv
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IQoSManagerPriv_Release(ds_Net_IQoSManagerPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSManagerPriv)->Release(_pif);
}

/** @memberof ds_Net_IQoSManagerPriv
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IQoSManagerPriv_QueryInterface(ds_Net_IQoSManagerPriv* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSManagerPriv)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IQoSManagerPriv
  * 
  * This function issues a secondary QoS request.
  * @param _pif Pointer to interface
  * @param requestedQoSSpec The requested QoS specification.
  * @param session The secondary QoS session opened for handling the request.
  * @see IQoSSecondary
  * @retval AEE_SUCCESS Request received successfully.
  * @retval ds_EBADOPTS Bad options.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IQoSManagerPriv_RequestSecondary(ds_Net_IQoSManagerPriv* _pif, const ds_Net_QoSSpecType* requestedQoSSpec, ds_Net_IQoSSecondary** session)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSManagerPriv)->RequestSecondary(_pif, requestedQoSSpec, session);
}

/** @memberof ds_Net_IQoSManagerPriv
  * 
  * This function issues QoS requests for multiple QoS sessions.
  * @param _pif Pointer to interface
  * @param specs The QoS specifications.
  * @param specsLen Length of sequence
  * @param opCode The operation.
  * @param sessions The QoS sessions.
  * @see ds_Net_QoS_BundleOpCodeType
  * @retval AEE_SUCCESS Request received successfully.
  * @retval ds_EBADOPTS Bad options.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IQoSManagerPriv_RequestBundle(ds_Net_IQoSManagerPriv* _pif, const ds_Net_QoSSpecType* specs, int specsLen, ds_Net_QoSRequestOpCodeType opCode, ds_Net_IQoSSecondariesOutput** sessions)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSManagerPriv)->RequestBundle(_pif, specs, specsLen, opCode, sessions);
}

/** @memberof ds_Net_IQoSManagerPriv
  * 
  * The function closes multiple QoS sessions.
  * @param _pif Pointer to interface
  * @param sessions The QoS sessions.
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IQoSManagerPriv_Close(ds_Net_IQoSManagerPriv* _pif, ds_Net_IQoSSecondariesInput* qosSessions)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSManagerPriv)->Close(_pif, qosSessions);
}

/** @memberof ds_Net_IQoSManagerPriv
  * 
  * The function resumes multiple QoS sessions.
  * @param _pif Pointer to interface
  * @param sessions The QoS sessions.
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IQoSManagerPriv_Resume(ds_Net_IQoSManagerPriv* _pif, ds_Net_IQoSSecondariesInput* qosSessions)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSManagerPriv)->Resume(_pif, qosSessions);
}

/** @memberof ds_Net_IQoSManagerPriv
  * 
  * The function suspends multiple QoS sessions.
  * @param _pif Pointer to interface
  * @param sessions The QoS sessions.
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IQoSManagerPriv_Suspend(ds_Net_IQoSManagerPriv* _pif, ds_Net_IQoSSecondariesInput* qosSessions)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoSManagerPriv)->Suspend(_pif, qosSessions);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "ds_Net_Def.h"
#include "ds_Net_QoS_Def.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_IQoS.h"
#include "ds_Net_IQoSSecondary.h"
#include "ds_Net_IQoSFlowPriv.h"
#include "ds_Addr_Def.h"
#include "ds_Net_IIPFilterPriv.h"
#include "ds_Net_QoS_DefPriv.h"
#include "ds_Net_IQoSSecondariesInput.h"
#include "ds_Net_IQoSSecondariesOutput.h"
namespace ds
{
   namespace Net
   {
      const ::AEEIID AEEIID_IQoSManagerPriv = 0x10a32b1;
      
      /** @interface IQoSManagerPriv
        * 
        * ds Net QoS Manager Private interface.
        */
      struct IQoSManagerPriv : public ::IQI
      {
         
         /**
           * This function issues a secondary QoS request.
           * @param requestedQoSSpec The requested QoS specification.
           * @param session The secondary QoS session opened for handling the request.
           * @see IQoSSecondary
           * @retval AEE_SUCCESS Request received successfully.
           * @retval ds::EBADOPTS Bad options.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL RequestSecondary(const ::ds::Net::QoSSpecType* requestedQoSSpec, ::ds::Net::IQoSSecondary** session) = 0;
         
         /**
           * This function issues QoS requests for multiple QoS sessions.
           * @param specs The QoS specifications.
           * @param opCode The operation.
           * @param sessions The QoS sessions.
           * @see ds::Net::QoS::BundleOpCodeType
           * @retval AEE_SUCCESS Request received successfully.
           * @retval ds::EBADOPTS Bad options.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL RequestBundle(const ::ds::Net::QoSSpecType* specs, int specsLen, ::ds::Net::QoSRequestOpCodeType opCode, ::ds::Net::IQoSSecondariesOutput** sessions) = 0;
         
         /**
           * The function closes multiple QoS sessions.
           * @param sessions The QoS sessions.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Close(::ds::Net::IQoSSecondariesInput* qosSessions) = 0;
         
         /**
           * The function resumes multiple QoS sessions.
           * @param sessions The QoS sessions.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Resume(::ds::Net::IQoSSecondariesInput* qosSessions) = 0;
         
         /**
           * The function suspends multiple QoS sessions.
           * @param sessions The QoS sessions.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Suspend(::ds::Net::IQoSSecondariesInput* qosSessions) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IQOSMANAGERPRIV_H
