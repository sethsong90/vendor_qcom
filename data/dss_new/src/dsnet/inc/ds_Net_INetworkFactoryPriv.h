#ifndef DS_NET_INETWORKFACTORYPRIV_H
#define DS_NET_INETWORKFACTORYPRIV_H

/*============================================================================
  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Net_DownReasons_Def.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_INetwork.h"
#include "ds_Net_IPolicy.h"
#include "ds_Net_IPolicyPriv.h"
#include "ds_Net_QoS_Def.h"
#include "ds_Net_IBearerInfo.h"
#include "ds_Net_INetworkPriv.h"
#include "ds_Net_IIPFilterPriv.h"
#include "ds_Net_IQoSFlowPriv.h"
#include "ds_Net_INatSessionPriv.h"
#define ds_Net_AEEIID_INetworkFactoryPriv 0x1072cf0
#define INHERIT_ds_Net_INetworkFactoryPriv(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*CreateNetworkPriv)(iname* _pif, ds_Net_IPolicyPriv* policy, ds_Net_INetworkPriv** newNetworkPriv); \
   AEEResult (*CreatePolicyPriv)(iname* _pif, ds_Net_IPolicyPriv** newPolicyPriv); \
   AEEResult (*CreateIPFilterSpec)(iname* _pif, ds_Net_IIPFilterPriv** newIPFilterSpec); \
   AEEResult (*CreateQoSFlowSpec)(iname* _pif, ds_Net_IQoSFlowPriv** newQoSFlowSpec); \
   AEEResult (*CreateNatSessionPriv)(iname* _pif, ds_Net_IPolicyPriv* pIPolicy, ds_Net_INatSessionPriv** newNatSessionPriv)
AEEINTERFACE_DEFINE(ds_Net_INetworkFactoryPriv);

/** @memberof ds_Net_INetworkFactoryPriv
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INetworkFactoryPriv_AddRef(ds_Net_INetworkFactoryPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkFactoryPriv)->AddRef(_pif);
}

/** @memberof ds_Net_INetworkFactoryPriv
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INetworkFactoryPriv_Release(ds_Net_INetworkFactoryPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkFactoryPriv)->Release(_pif);
}

/** @memberof ds_Net_INetworkFactoryPriv
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_INetworkFactoryPriv_QueryInterface(ds_Net_INetworkFactoryPriv* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkFactoryPriv)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_INetworkFactoryPriv
  *     
  * NOTE: THIS API IS NOT PUBLIC (shall not be available for user applications)
  * This function creates an instance of INetworkPriv. INetworkPriv creation is supported only
  * via INetworkFactoryPriv. In this API the Application specifies the network policy to be used
  * for the creation of the network object.
  * @param _pif Pointer to interface
  * @param policy Network policy. If NULL default policy shall be used.
  * @param newNetworkPriv Output The newly created INetworkPriv instance.
  * @retval AEE_SUCCESS INetworkPriv created successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkFactoryPriv_CreateNetworkPriv(ds_Net_INetworkFactoryPriv* _pif, ds_Net_IPolicyPriv* policy, ds_Net_INetworkPriv** newNetworkPriv)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkFactoryPriv)->CreateNetworkPriv(_pif, policy, newNetworkPriv);
}

/** @memberof ds_Net_INetworkFactoryPriv
  *     
  * This function creates an instance of PolicyPriv. PolicyPriv creation is supported only
  * via INetworkFactory. 
  * @param _pif Pointer to interface
  * @param newPolicyPriv Output The newly created PolicyPriv instance.
  * @retval AEE_SUCCESS PolicyPriv created successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkFactoryPriv_CreatePolicyPriv(ds_Net_INetworkFactoryPriv* _pif, ds_Net_IPolicyPriv** newPolicyPriv)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkFactoryPriv)->CreatePolicyPriv(_pif, newPolicyPriv);
}

/** @memberof ds_Net_INetworkFactoryPriv
  *     
  * This function creates an instance of IPFilterSpec. IPFilterSpec creation is supported only
  * via INetworkFactory. 
  * @param _pif Pointer to interface
  * @param newIPFilterSpec Output The newly created IPFilterSpec instance.
  * @retval ds_SUCCESS IPFilterSpec created successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkFactoryPriv_CreateIPFilterSpec(ds_Net_INetworkFactoryPriv* _pif, ds_Net_IIPFilterPriv** newIPFilterSpec)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkFactoryPriv)->CreateIPFilterSpec(_pif, newIPFilterSpec);
}

/** @memberof ds_Net_INetworkFactoryPriv
  *     
  * This function creates an instance of QoSFlowSpec. QoSFlowSpec creation is supported only
  * via INetworkFactory. 
  * @param _pif Pointer to interface
  * @param newQoSFlowSpec Output The newly created QoSFlowSpec instance.
  * @retval AEE_SUCCESS QoSFlowSpec created successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkFactoryPriv_CreateQoSFlowSpec(ds_Net_INetworkFactoryPriv* _pif, ds_Net_IQoSFlowPriv** newQoSFlowSpec)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkFactoryPriv)->CreateQoSFlowSpec(_pif, newQoSFlowSpec);
}

/** @memberof ds_Net_INetworkFactoryPriv
  *     
  * This function creates an instance of NatSessionPriv. NatSessionPriv creation is supported only
  * via INetworkFactory. 
  * @param _pif Pointer to interface
  * @param policy Network policy.
  * @param newNatSessionPriv Output The newly created NatSessionPriv instance.
  * @retval AEE_SUCCESS NatSessionPriv created successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkFactoryPriv_CreateNatSessionPriv(ds_Net_INetworkFactoryPriv* _pif, ds_Net_IPolicyPriv* pIPolicy, ds_Net_INatSessionPriv** newNatSessionPriv)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkFactoryPriv)->CreateNatSessionPriv(_pif, pIPolicy, newNatSessionPriv);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Net_DownReasons_Def.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_INetwork.h"
#include "ds_Net_IPolicy.h"
#include "ds_Net_IPolicyPriv.h"
#include "ds_Net_QoS_Def.h"
#include "ds_Net_IBearerInfo.h"
#include "ds_Net_INetworkPriv.h"
#include "ds_Net_IIPFilterPriv.h"
#include "ds_Net_IQoSFlowPriv.h"
#include "ds_Net_INatSessionPriv.h"
namespace ds
{
   namespace Net
   {
      const ::AEEIID AEEIID_INetworkFactoryPriv = 0x1072cf0;
      struct INetworkFactoryPriv : public ::IQI
      {
         
         /**    
           * NOTE: THIS API IS NOT PUBLIC (shall not be available for user applications)
           * This function creates an instance of INetworkPriv. INetworkPriv creation is supported only
           * via INetworkFactoryPriv. In this API the Application specifies the network policy to be used
           * for the creation of the network object.
           * @param policy Network policy. If NULL default policy shall be used.
           * @param newNetworkPriv Output The newly created INetworkPriv instance.
           * @retval AEE_SUCCESS INetworkPriv created successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL CreateNetworkPriv(::ds::Net::IPolicyPriv* policy, ::ds::Net::INetworkPriv** newNetworkPriv) = 0;
         
         /**    
           * This function creates an instance of PolicyPriv. PolicyPriv creation is supported only
           * via INetworkFactory. 
           * @param newPolicyPriv Output The newly created PolicyPriv instance.
           * @retval AEE_SUCCESS PolicyPriv created successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL CreatePolicyPriv(::ds::Net::IPolicyPriv** newPolicyPriv) = 0;
         
         /**    
           * This function creates an instance of IPFilterSpec. IPFilterSpec creation is supported only
           * via INetworkFactory. 
           * @param newIPFilterSpec Output The newly created IPFilterSpec instance.
           * @retval ds::SUCCESS IPFilterSpec created successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL CreateIPFilterSpec(::ds::Net::IIPFilterPriv** newIPFilterSpec) = 0;
         
         /**    
           * This function creates an instance of QoSFlowSpec. QoSFlowSpec creation is supported only
           * via INetworkFactory. 
           * @param newQoSFlowSpec Output The newly created QoSFlowSpec instance.
           * @retval AEE_SUCCESS QoSFlowSpec created successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL CreateQoSFlowSpec(::ds::Net::IQoSFlowPriv** newQoSFlowSpec) = 0;
         
         /**    
           * This function creates an instance of NatSessionPriv. NatSessionPriv creation is supported only
           * via INetworkFactory. 
           * @param policy Network policy.
           * @param newNatSessionPriv Output The newly created NatSessionPriv instance.
           * @retval AEE_SUCCESS NatSessionPriv created successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL CreateNatSessionPriv(::ds::Net::IPolicyPriv* pIPolicy, ::ds::Net::INatSessionPriv** newNatSessionPriv) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_INETWORKFACTORYPRIV_H
