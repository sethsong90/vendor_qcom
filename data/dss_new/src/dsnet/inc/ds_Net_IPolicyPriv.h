#ifndef DS_NET_IPOLICYPRIV_H
#define DS_NET_IPOLICYPRIV_H

/*============================================================================
  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "ds_Net_Def.h"
#include "AEEIQI.h"
#include "ds_Addr_Def.h"
#include "ds_Net_IPolicy.h"
#define ds_Net_AEEIID_IPolicyPriv 0x1072d07

/** @interface ds_Net_IPolicyPriv
  * 
  * ds Network Policy Privileged interface.
  * This interface allows for additional, non public, policy attributes.
  */
#define INHERIT_ds_Net_IPolicyPriv(iname) \
   INHERIT_ds_Net_IPolicy(iname); \
   AEEResult (*GetIfaceId)(iname* _pif, ds_Net_IfaceIdType* value); \
   AEEResult (*SetIfaceId)(iname* _pif, ds_Net_IfaceIdType value); \
   AEEResult (*GetUMTSAPNName)(iname* _pif, char* value, int valueLen, int* valueLenReq); \
   AEEResult (*SetUMTSAPNName)(iname* _pif, const char* value)
AEEINTERFACE_DEFINE(ds_Net_IPolicyPriv);

/** @memberof ds_Net_IPolicyPriv
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IPolicyPriv_AddRef(ds_Net_IPolicyPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->AddRef(_pif);
}

/** @memberof ds_Net_IPolicyPriv
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IPolicyPriv_Release(ds_Net_IPolicyPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->Release(_pif);
}

/** @memberof ds_Net_IPolicyPriv
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IPolicyPriv_QueryInterface(ds_Net_IPolicyPriv* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IPolicyPriv
  *           
  * This attribute represents the interface name (Network Type) of the
  * policy.
  * Either Network Group *or* Network Type is taken into account when
  * the policy takes effect.
  * If both are set, the last one being set takes effect.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @see SetIfaceGroup
  */
static __inline AEEResult ds_Net_IPolicyPriv_GetIfaceName(ds_Net_IPolicyPriv* _pif, ds_Net_IfaceNameType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->GetIfaceName(_pif, value);
}

/** @memberof ds_Net_IPolicyPriv
  *           
  * This attribute represents the interface name (Network Type) of the
  * policy.
  * Either Network Group *or* Network Type is taken into account when
  * the policy takes effect.
  * If both are set, the last one being set takes effect.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @see SetIfaceGroup
  */
static __inline AEEResult ds_Net_IPolicyPriv_SetIfaceName(ds_Net_IPolicyPriv* _pif, ds_Net_IfaceNameType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->SetIfaceName(_pif, value);
}

/** @memberof ds_Net_IPolicyPriv
  *           
  * This attribute represents the interface group (Network Group) of
  * the policy.
  * Either Network Group *or* Network Type is taken into account when
  * the policy takes effect.
  * If both are set, the last one being set takes effect.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @see SetIfaceName
  */
static __inline AEEResult ds_Net_IPolicyPriv_GetIfaceGroup(ds_Net_IPolicyPriv* _pif, ds_Net_IfaceGroupType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->GetIfaceGroup(_pif, value);
}

/** @memberof ds_Net_IPolicyPriv
  *           
  * This attribute represents the interface group (Network Group) of
  * the policy.
  * Either Network Group *or* Network Type is taken into account when
  * the policy takes effect.
  * If both are set, the last one being set takes effect.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @see SetIfaceName
  */
static __inline AEEResult ds_Net_IPolicyPriv_SetIfaceGroup(ds_Net_IPolicyPriv* _pif, ds_Net_IfaceGroupType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->SetIfaceGroup(_pif, value);
}

/** @memberof ds_Net_IPolicyPriv
  * 
  * This attribute indicates the policy flags as part of the network
  * policy.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IPolicyPriv_GetPolicyFlag(ds_Net_IPolicyPriv* _pif, ds_Net_PolicyFlagType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->GetPolicyFlag(_pif, value);
}

/** @memberof ds_Net_IPolicyPriv
  * 
  * This attribute indicates the policy flags as part of the network
  * policy.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IPolicyPriv_SetPolicyFlag(ds_Net_IPolicyPriv* _pif, ds_Net_PolicyFlagType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->SetPolicyFlag(_pif, value);
}

/** @memberof ds_Net_IPolicyPriv
  * 
  * This attribute indicates the address family as part of the network
  * policy.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IPolicyPriv_GetAddressFamily(ds_Net_IPolicyPriv* _pif, ds_AddrFamilyType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->GetAddressFamily(_pif, value);
}

/** @memberof ds_Net_IPolicyPriv
  * 
  * This attribute indicates the address family as part of the network
  * policy.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IPolicyPriv_SetAddressFamily(ds_Net_IPolicyPriv* _pif, ds_AddrFamilyType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->SetAddressFamily(_pif, value);
}

/** @memberof ds_Net_IPolicyPriv
  * 
  * This attribute indicates the CDMA profile number as part of the
  * network policy. In general, profile numbers are carrier specific.
  *
  * In CDMA, when the service processes the policy, it always uses a
  * profile number. If the application does not specify any, the
  * service uses the default profile number. The application should
  * take care that the attributes it specifies in the policy are
  * compatible to the specifications in the profile specified in the 
  * policy (or to the default profile if no profile is specified).
  *          
  * In CDMA the profile number may be related to the AppType (OMH).
  * There are cases where both this attribute and
  * UMTSProfileNumber are set in a single policy, for example, when app
  * specifies IfaceGroup that includes both CDMA and UMTS interfaces, 
  * or when the policy is set to support some inter technology hand off
  * scenarios.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IPolicyPriv_GetCDMAProfileNumber(ds_Net_IPolicyPriv* _pif, int* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->GetCDMAProfileNumber(_pif, value);
}

/** @memberof ds_Net_IPolicyPriv
  * 
  * This attribute indicates the CDMA profile number as part of the
  * network policy. In general, profile numbers are carrier specific.
  *
  * In CDMA, when the service processes the policy, it always uses a
  * profile number. If the application does not specify any, the
  * service uses the default profile number. The application should
  * take care that the attributes it specifies in the policy are
  * compatible to the specifications in the profile specified in the 
  * policy (or to the default profile if no profile is specified).
  *          
  * In CDMA the profile number may be related to the AppType (OMH).
  * There are cases where both this attribute and
  * UMTSProfileNumber are set in a single policy, for example, when app
  * specifies IfaceGroup that includes both CDMA and UMTS interfaces, 
  * or when the policy is set to support some inter technology hand off
  * scenarios.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IPolicyPriv_SetCDMAProfileNumber(ds_Net_IPolicyPriv* _pif, int value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->SetCDMAProfileNumber(_pif, value);
}

/** @memberof ds_Net_IPolicyPriv
  * 
  * This attribute indicates the UMTS profile number as part of the
  * network policy. In general, profile numbers are carrier specific.
  *
  * In UMTS, when the service processes the policy, it always uses a
  * profile number. If the application does not specify any, the
  * service uses the default profile number. The application should
  * take care that the attributes it specifies in the policy are
  * compatible to the specifications in the profile specified in the 
  * policy (or to the default profile if no profile is specified).
  *          
  * There are cases where both this attribute and CDMAProfileNumber are
  * set in a single policy, for example, when app specifies IfaceGroup
  * that includes both CDMA and UMTS interfaces, or when the policy is
  * set to support some inter technology hand off scenarios.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IPolicyPriv_GetUMTSProfileNumber(ds_Net_IPolicyPriv* _pif, int* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->GetUMTSProfileNumber(_pif, value);
}

/** @memberof ds_Net_IPolicyPriv
  * 
  * This attribute indicates the UMTS profile number as part of the
  * network policy. In general, profile numbers are carrier specific.
  *
  * In UMTS, when the service processes the policy, it always uses a
  * profile number. If the application does not specify any, the
  * service uses the default profile number. The application should
  * take care that the attributes it specifies in the policy are
  * compatible to the specifications in the profile specified in the 
  * policy (or to the default profile if no profile is specified).
  *          
  * There are cases where both this attribute and CDMAProfileNumber are
  * set in a single policy, for example, when app specifies IfaceGroup
  * that includes both CDMA and UMTS interfaces, or when the policy is
  * set to support some inter technology hand off scenarios.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IPolicyPriv_SetUMTSProfileNumber(ds_Net_IPolicyPriv* _pif, int value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->SetUMTSProfileNumber(_pif, value);
}

/** @memberof ds_Net_IPolicyPriv
  * 
  * This attribute indicates a list of profile numbers that define the
  * policy.
  * In general, profile numbers are carrier specific.
  * In CDMA the profile number may be related to the AppType (OMH)
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @param valueLen Length of sequence
  * @param valueLenReq Required length of sequence
  */
static __inline AEEResult ds_Net_IPolicyPriv_GetProfileList(ds_Net_IPolicyPriv* _pif, ds_Net_PolicyProfileType* value, int valueLen, int* valueLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->GetProfileList(_pif, value, valueLen, valueLenReq);
}

/** @memberof ds_Net_IPolicyPriv
  * 
  * This attribute indicates a list of profile numbers that define the
  * policy.
  * In general, profile numbers are carrier specific.
  * In CDMA the profile number may be related to the AppType (OMH)
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @param valueLen Length of sequence
  */
static __inline AEEResult ds_Net_IPolicyPriv_SetProfileList(ds_Net_IPolicyPriv* _pif, const ds_Net_PolicyProfileType* value, int valueLen)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->SetProfileList(_pif, value, valueLen);
}

/** @memberof ds_Net_IPolicyPriv
  * 
  * Set to TRUE to select a routable interface. An interface is routable
  * if it is brought up by a tethered device. In default policy this
  * field is set to FALSE.
  * Only applications that need to modify the behavior of a tethered
  * call should set this flag.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IPolicyPriv_GetRoutable(ds_Net_IPolicyPriv* _pif, boolean* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->GetRoutable(_pif, value);
}

/** @memberof ds_Net_IPolicyPriv
  * 
  * Set to TRUE to select a routable interface. An interface is routable
  * if it is brought up by a tethered device. In default policy this
  * field is set to FALSE.
  * Only applications that need to modify the behavior of a tethered
  * call should set this flag.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IPolicyPriv_SetRoutable(ds_Net_IPolicyPriv* _pif, boolean value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->SetRoutable(_pif, value);
}

/** @memberof ds_Net_IPolicyPriv
  * 
  * This attribute indicates the application type which is used for
  * network usage arbitration.
  * Application Types used on a carrier network shall be based on the
  * standard applicable to that network. One such standard, for example,
  * is C.S0023-D v1.0 R-UIM (See "APPLICATIONS" under EFSIPUPPExt - 
  * SimpleIP User Profile Parameters 1 Extension, in chapter "Multi-Mode
  * R-UIM Dedicated File (DF) and Elementary File (EF) Structure").
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IPolicyPriv_GetAppType(ds_Net_IPolicyPriv* _pif, int* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->GetAppType(_pif, value);
}

/** @memberof ds_Net_IPolicyPriv
  * 
  * This attribute indicates the application type which is used for
  * network usage arbitration.
  * Application Types used on a carrier network shall be based on the
  * standard applicable to that network. One such standard, for example,
  * is C.S0023-D v1.0 R-UIM (See "APPLICATIONS" under EFSIPUPPExt - 
  * SimpleIP User Profile Parameters 1 Extension, in chapter "Multi-Mode
  * R-UIM Dedicated File (DF) and Elementary File (EF) Structure").
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IPolicyPriv_SetAppType(ds_Net_IPolicyPriv* _pif, int value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->SetAppType(_pif, value);
}

/** @memberof ds_Net_IPolicyPriv
  * 
  * This attribute indicates an iface id for the policy.
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @See INetwork_IfaceId
  */
static __inline AEEResult ds_Net_IPolicyPriv_GetIfaceId(ds_Net_IPolicyPriv* _pif, ds_Net_IfaceIdType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->GetIfaceId(_pif, value);
}

/** @memberof ds_Net_IPolicyPriv
  * 
  * This attribute indicates an iface id for the policy.
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @See INetwork_IfaceId
  */
static __inline AEEResult ds_Net_IPolicyPriv_SetIfaceId(ds_Net_IPolicyPriv* _pif, ds_Net_IfaceIdType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->SetIfaceId(_pif, value);
}

/** @memberof ds_Net_IPolicyPriv
  * 
  * This attribute indicates the UMTS Access Point name as part of the
  * network policy.
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @param valueLen Length of sequence
  * @param valueLenReq Required length of sequence
  */
static __inline AEEResult ds_Net_IPolicyPriv_GetUMTSAPNName(ds_Net_IPolicyPriv* _pif, char* value, int valueLen, int* valueLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->GetUMTSAPNName(_pif, value, valueLen, valueLenReq);
}

/** @memberof ds_Net_IPolicyPriv
  * 
  * This attribute indicates the UMTS Access Point name as part of the
  * network policy.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IPolicyPriv_SetUMTSAPNName(ds_Net_IPolicyPriv* _pif, const char* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicyPriv)->SetUMTSAPNName(_pif, value);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "ds_Net_Def.h"
#include "AEEIQI.h"
#include "ds_Addr_Def.h"
#include "ds_Net_IPolicy.h"
namespace ds
{
   namespace Net
   {
      const ::AEEIID AEEIID_IPolicyPriv = 0x1072d07;
      
      /** @interface IPolicyPriv
        * 
        * ds Network Policy Privileged interface.
        * This interface allows for additional, non public, policy attributes.
        */
      struct IPolicyPriv : public ::ds::Net::IPolicy
      {
         
         /**
           * This attribute indicates an iface id for the policy.
           * @param value Attribute value
           * @See INetwork::IfaceId
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetIfaceId(::ds::Net::IfaceIdType* value) = 0;
         
         /**
           * This attribute indicates an iface id for the policy.
           * @param value Attribute value
           * @See INetwork::IfaceId
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetIfaceId(::ds::Net::IfaceIdType value) = 0;
         
         /**
           * This attribute indicates the UMTS Access Point name as part of the
           * network policy.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetUMTSAPNName(char* value, int valueLen, int* valueLenReq) = 0;
         
         /**
           * This attribute indicates the UMTS Access Point name as part of the
           * network policy.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetUMTSAPNName(const char* value) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IPOLICYPRIV_H
