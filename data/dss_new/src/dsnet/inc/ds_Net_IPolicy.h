#ifndef DS_NET_IPOLICY_H
#define DS_NET_IPOLICY_H

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

/**
  * PolicyFlag: Specifies network interface compatibility to the policy in
  * respect to the interface's Data Connection.
  */
typedef int ds_Net_PolicyFlagType;

/** @memberof ds_Net_PolicyFlag
  * 
  * ANY: The state of the Data Connection is irrelevant for selecting a
  * Network interface compatible to the policy.
  */
#define ds_Net_PolicyFlag_QDS_ANY 0

/** @memberof ds_Net_PolicyFlag
  * 
  * UP_ONLY: A Network interface shall be compatible to the policy only
  * if all its characteristics are compatible to the policy AND data
  * connection is already established on the interface.
  * INetwork creation per that policy shall fail if there are interfaces
  * compatible to the policy but none of them in UP state.
  */
#define ds_Net_PolicyFlag_QDS_UP_ONLY 1

/** @memberof ds_Net_PolicyFlag
  * 
  * UP_PREFERRED: If there are several interfaces compatible with the
  * policy specifications, but some of them in UP state and some not, an
  * interface which is in UP state shall be selected. If Data Connection
  * is not established for all of the compatible interfaces, one of them
  * shall still be selected for the operation where the policy was
  * specified (the operation will not fail).
  */
#define ds_Net_PolicyFlag_QDS_UP_PREFERRED 2

/**
  * PolicyProfileType: This type is used in the ProfileList member of the
  * Policy interface.
  */
struct ds_Net_PolicyProfileType {
   int ProfileID;
   int ProfileValue;
};
typedef struct ds_Net_PolicyProfileType ds_Net_PolicyProfileType;
struct ds_Net__SeqPolicyProfileType__seq_PolicyProfileType_Net_ds {
   ds_Net_PolicyProfileType* data;
   int dataLen;
   int dataLenReq;
};
typedef struct ds_Net__SeqPolicyProfileType__seq_PolicyProfileType_Net_ds ds_Net__SeqPolicyProfileType__seq_PolicyProfileType_Net_ds;
typedef ds_Net__SeqPolicyProfileType__seq_PolicyProfileType_Net_ds ds_Net_SeqPolicyProfileType;
#define ds_Net_AEEIID_IPolicy 0x106c547

/** @interface ds_Net_IPolicy
  * 
  * ds Network Policy interface.
  * Instantiation of this interface shall create a default policy.
  * That policy can be used as is to produce a default INetwork object via
  * INetworkFactory_CreateNetwork. Alternatively, applications can change
  * one or more members of the Policy object and use it in
  * INetworkFactory_CreateNetwork to request instantiation of a
  * non-default INetwork object.
  */
#define INHERIT_ds_Net_IPolicy(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*GetIfaceName)(iname* _pif, ds_Net_IfaceNameType* value); \
   AEEResult (*SetIfaceName)(iname* _pif, ds_Net_IfaceNameType value); \
   AEEResult (*GetIfaceGroup)(iname* _pif, ds_Net_IfaceGroupType* value); \
   AEEResult (*SetIfaceGroup)(iname* _pif, ds_Net_IfaceGroupType value); \
   AEEResult (*GetPolicyFlag)(iname* _pif, ds_Net_PolicyFlagType* value); \
   AEEResult (*SetPolicyFlag)(iname* _pif, ds_Net_PolicyFlagType value); \
   AEEResult (*GetAddressFamily)(iname* _pif, ds_AddrFamilyType* value); \
   AEEResult (*SetAddressFamily)(iname* _pif, ds_AddrFamilyType value); \
   AEEResult (*GetCDMAProfileNumber)(iname* _pif, int* value); \
   AEEResult (*SetCDMAProfileNumber)(iname* _pif, int value); \
   AEEResult (*GetUMTSProfileNumber)(iname* _pif, int* value); \
   AEEResult (*SetUMTSProfileNumber)(iname* _pif, int value); \
   AEEResult (*GetProfileList)(iname* _pif, ds_Net_PolicyProfileType* value, int valueLen, int* valueLenReq); \
   AEEResult (*SetProfileList)(iname* _pif, const ds_Net_PolicyProfileType* value, int valueLen); \
   AEEResult (*GetRoutable)(iname* _pif, boolean* value); \
   AEEResult (*SetRoutable)(iname* _pif, boolean value); \
   AEEResult (*GetAppType)(iname* _pif, int* value); \
   AEEResult (*SetAppType)(iname* _pif, int value)
AEEINTERFACE_DEFINE(ds_Net_IPolicy);

/** @memberof ds_Net_IPolicy
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IPolicy_AddRef(ds_Net_IPolicy* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicy)->AddRef(_pif);
}

/** @memberof ds_Net_IPolicy
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IPolicy_Release(ds_Net_IPolicy* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicy)->Release(_pif);
}

/** @memberof ds_Net_IPolicy
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IPolicy_QueryInterface(ds_Net_IPolicy* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicy)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IPolicy
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
static __inline AEEResult ds_Net_IPolicy_GetIfaceName(ds_Net_IPolicy* _pif, ds_Net_IfaceNameType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicy)->GetIfaceName(_pif, value);
}

/** @memberof ds_Net_IPolicy
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
static __inline AEEResult ds_Net_IPolicy_SetIfaceName(ds_Net_IPolicy* _pif, ds_Net_IfaceNameType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicy)->SetIfaceName(_pif, value);
}

/** @memberof ds_Net_IPolicy
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
static __inline AEEResult ds_Net_IPolicy_GetIfaceGroup(ds_Net_IPolicy* _pif, ds_Net_IfaceGroupType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicy)->GetIfaceGroup(_pif, value);
}

/** @memberof ds_Net_IPolicy
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
static __inline AEEResult ds_Net_IPolicy_SetIfaceGroup(ds_Net_IPolicy* _pif, ds_Net_IfaceGroupType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicy)->SetIfaceGroup(_pif, value);
}

/** @memberof ds_Net_IPolicy
  * 
  * This attribute indicates the policy flags as part of the network
  * policy.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IPolicy_GetPolicyFlag(ds_Net_IPolicy* _pif, ds_Net_PolicyFlagType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicy)->GetPolicyFlag(_pif, value);
}

/** @memberof ds_Net_IPolicy
  * 
  * This attribute indicates the policy flags as part of the network
  * policy.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IPolicy_SetPolicyFlag(ds_Net_IPolicy* _pif, ds_Net_PolicyFlagType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicy)->SetPolicyFlag(_pif, value);
}

/** @memberof ds_Net_IPolicy
  * 
  * This attribute indicates the address family as part of the network
  * policy.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IPolicy_GetAddressFamily(ds_Net_IPolicy* _pif, ds_AddrFamilyType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicy)->GetAddressFamily(_pif, value);
}

/** @memberof ds_Net_IPolicy
  * 
  * This attribute indicates the address family as part of the network
  * policy.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IPolicy_SetAddressFamily(ds_Net_IPolicy* _pif, ds_AddrFamilyType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicy)->SetAddressFamily(_pif, value);
}

/** @memberof ds_Net_IPolicy
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
static __inline AEEResult ds_Net_IPolicy_GetCDMAProfileNumber(ds_Net_IPolicy* _pif, int* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicy)->GetCDMAProfileNumber(_pif, value);
}

/** @memberof ds_Net_IPolicy
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
static __inline AEEResult ds_Net_IPolicy_SetCDMAProfileNumber(ds_Net_IPolicy* _pif, int value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicy)->SetCDMAProfileNumber(_pif, value);
}

/** @memberof ds_Net_IPolicy
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
static __inline AEEResult ds_Net_IPolicy_GetUMTSProfileNumber(ds_Net_IPolicy* _pif, int* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicy)->GetUMTSProfileNumber(_pif, value);
}

/** @memberof ds_Net_IPolicy
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
static __inline AEEResult ds_Net_IPolicy_SetUMTSProfileNumber(ds_Net_IPolicy* _pif, int value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicy)->SetUMTSProfileNumber(_pif, value);
}

/** @memberof ds_Net_IPolicy
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
static __inline AEEResult ds_Net_IPolicy_GetProfileList(ds_Net_IPolicy* _pif, ds_Net_PolicyProfileType* value, int valueLen, int* valueLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicy)->GetProfileList(_pif, value, valueLen, valueLenReq);
}

/** @memberof ds_Net_IPolicy
  * 
  * This attribute indicates a list of profile numbers that define the
  * policy.
  * In general, profile numbers are carrier specific.
  * In CDMA the profile number may be related to the AppType (OMH)
  * @param _pif Pointer to interface
  * @param value Attribute value
  * @param valueLen Length of sequence
  */
static __inline AEEResult ds_Net_IPolicy_SetProfileList(ds_Net_IPolicy* _pif, const ds_Net_PolicyProfileType* value, int valueLen)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicy)->SetProfileList(_pif, value, valueLen);
}

/** @memberof ds_Net_IPolicy
  * 
  * Set to TRUE to select a routable interface. An interface is routable
  * if it is brought up by a tethered device. In default policy this
  * field is set to FALSE.
  * Only applications that need to modify the behavior of a tethered
  * call should set this flag.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IPolicy_GetRoutable(ds_Net_IPolicy* _pif, boolean* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicy)->GetRoutable(_pif, value);
}

/** @memberof ds_Net_IPolicy
  * 
  * Set to TRUE to select a routable interface. An interface is routable
  * if it is brought up by a tethered device. In default policy this
  * field is set to FALSE.
  * Only applications that need to modify the behavior of a tethered
  * call should set this flag.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IPolicy_SetRoutable(ds_Net_IPolicy* _pif, boolean value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicy)->SetRoutable(_pif, value);
}

/** @memberof ds_Net_IPolicy
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
static __inline AEEResult ds_Net_IPolicy_GetAppType(ds_Net_IPolicy* _pif, int* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicy)->GetAppType(_pif, value);
}

/** @memberof ds_Net_IPolicy
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
static __inline AEEResult ds_Net_IPolicy_SetAppType(ds_Net_IPolicy* _pif, int value)
{
   return AEEGETPVTBL(_pif, ds_Net_IPolicy)->SetAppType(_pif, value);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
namespace ds
{
   namespace Net
   {
      
      /**
        * PolicyFlag: Specifies network interface compatibility to the policy in
        * respect to the interface's Data Connection.
        */
      typedef int PolicyFlagType;
      namespace PolicyFlag
      {
         
         /**
           * ANY: The state of the Data Connection is irrelevant for selecting a
           * Network interface compatible to the policy.
           */
         const ::ds::Net::PolicyFlagType QDS_ANY = 0;
         
         /**
           * UP_ONLY: A Network interface shall be compatible to the policy only
           * if all its characteristics are compatible to the policy AND data
           * connection is already established on the interface.
           * INetwork creation per that policy shall fail if there are interfaces
           * compatible to the policy but none of them in UP state.
           */
         const ::ds::Net::PolicyFlagType QDS_UP_ONLY = 1;
         
         /**
           * UP_PREFERRED: If there are several interfaces compatible with the
           * policy specifications, but some of them in UP state and some not, an
           * interface which is in UP state shall be selected. If Data Connection
           * is not established for all of the compatible interfaces, one of them
           * shall still be selected for the operation where the policy was
           * specified (the operation will not fail).
           */
         const ::ds::Net::PolicyFlagType QDS_UP_PREFERRED = 2;
      };
      
      /**
        * PolicyProfileType: This type is used in the ProfileList member of the
        * Policy interface.
        */
      struct PolicyProfileType {
         int ProfileID;
         int ProfileValue;
      };
      struct _SeqPolicyProfileType__seq_PolicyProfileType_Net_ds {
         PolicyProfileType* data;
         int dataLen;
         int dataLenReq;
      };
      typedef _SeqPolicyProfileType__seq_PolicyProfileType_Net_ds SeqPolicyProfileType;
      const ::AEEIID AEEIID_IPolicy = 0x106c547;
      
      /** @interface IPolicy
        * 
        * ds Network Policy interface.
        * Instantiation of this interface shall create a default policy.
        * That policy can be used as is to produce a default INetwork object via
        * INetworkFactory::CreateNetwork. Alternatively, applications can change
        * one or more members of the Policy object and use it in
        * INetworkFactory::CreateNetwork to request instantiation of a
        * non-default INetwork object.
        */
      struct IPolicy : public ::IQI
      {
         
         /**          
           * This attribute represents the interface name (Network Type) of the
           * policy.
           * Either Network Group *or* Network Type is taken into account when
           * the policy takes effect.
           * If both are set, the last one being set takes effect.          
           * @param value Attribute value
           * @see SetIfaceGroup
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetIfaceName(::ds::Net::IfaceNameType* value) = 0;
         
         /**          
           * This attribute represents the interface name (Network Type) of the
           * policy.
           * Either Network Group *or* Network Type is taken into account when
           * the policy takes effect.
           * If both are set, the last one being set takes effect.          
           * @param value Attribute value
           * @see SetIfaceGroup
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetIfaceName(::ds::Net::IfaceNameType value) = 0;
         
         /**          
           * This attribute represents the interface group (Network Group) of
           * the policy.
           * Either Network Group *or* Network Type is taken into account when
           * the policy takes effect.
           * If both are set, the last one being set takes effect.          
           * @param value Attribute value
           * @see SetIfaceName
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetIfaceGroup(::ds::Net::IfaceGroupType* value) = 0;
         
         /**          
           * This attribute represents the interface group (Network Group) of
           * the policy.
           * Either Network Group *or* Network Type is taken into account when
           * the policy takes effect.
           * If both are set, the last one being set takes effect.          
           * @param value Attribute value
           * @see SetIfaceName
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetIfaceGroup(::ds::Net::IfaceGroupType value) = 0;
         
         /**
           * This attribute indicates the policy flags as part of the network
           * policy.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetPolicyFlag(::ds::Net::PolicyFlagType* value) = 0;
         
         /**
           * This attribute indicates the policy flags as part of the network
           * policy.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetPolicyFlag(::ds::Net::PolicyFlagType value) = 0;
         
         /**
           * This attribute indicates the address family as part of the network
           * policy.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetAddressFamily(::ds::AddrFamilyType* value) = 0;
         
         /**
           * This attribute indicates the address family as part of the network
           * policy.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetAddressFamily(::ds::AddrFamilyType value) = 0;
         
         /**
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
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetCDMAProfileNumber(int* value) = 0;
         
         /**
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
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetCDMAProfileNumber(int value) = 0;
         
         /**
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
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetUMTSProfileNumber(int* value) = 0;
         
         /**
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
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetUMTSProfileNumber(int value) = 0;
         
         /**
           * This attribute indicates a list of profile numbers that define the
           * policy.
           * In general, profile numbers are carrier specific.
           * In CDMA the profile number may be related to the AppType (OMH)
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetProfileList(::ds::Net::PolicyProfileType* value, int valueLen, int* valueLenReq) = 0;
         
         /**
           * This attribute indicates a list of profile numbers that define the
           * policy.
           * In general, profile numbers are carrier specific.
           * In CDMA the profile number may be related to the AppType (OMH)
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetProfileList(const ::ds::Net::PolicyProfileType* value, int valueLen) = 0;
         
         /**
           * Set to TRUE to select a routable interface. An interface is routable
           * if it is brought up by a tethered device. In default policy this
           * field is set to FALSE.
           * Only applications that need to modify the behavior of a tethered
           * call should set this flag.          
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetRoutable(boolean* value) = 0;
         
         /**
           * Set to TRUE to select a routable interface. An interface is routable
           * if it is brought up by a tethered device. In default policy this
           * field is set to FALSE.
           * Only applications that need to modify the behavior of a tethered
           * call should set this flag.          
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetRoutable(boolean value) = 0;
         
         /**
           * This attribute indicates the application type which is used for
           * network usage arbitration.
           * Application Types used on a carrier network shall be based on the
           * standard applicable to that network. One such standard, for example,
           * is C.S0023-D v1.0 R-UIM (See "APPLICATIONS" under EFSIPUPPExt - 
           * SimpleIP User Profile Parameters 1 Extension, in chapter "Multi-Mode
           * R-UIM Dedicated File (DF) and Elementary File (EF) Structure").
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetAppType(int* value) = 0;
         
         /**
           * This attribute indicates the application type which is used for
           * network usage arbitration.
           * Application Types used on a carrier network shall be based on the
           * standard applicable to that network. One such standard, for example,
           * is C.S0023-D v1.0 R-UIM (See "APPLICATIONS" under EFSIPUPPExt - 
           * SimpleIP User Profile Parameters 1 Extension, in chapter "Multi-Mode
           * R-UIM Dedicated File (DF) and Elementary File (EF) Structure").
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetAppType(int value) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IPOLICY_H
