#ifndef DS_NET_INETWORKIPV6_H
#define DS_NET_INETWORKIPV6_H

/*============================================================================
  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "ds_Addr_Def.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_Def.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IIPv6Address.h"
#define ds_Net_AEEIID_INetworkIPv6 0x106dcc5

/** @interface ds_Net_INetworkIPv6
  * 
  * ds Network IPv6 interface.
  */
#define INHERIT_ds_Net_INetworkIPv6(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*GeneratePrivAddr)(iname* _pif, boolean unique, ds_Net_IIPv6Address** addr); \
   AEEResult (*GetIPv6Address)(iname* _pif, const ds_INAddr6Type inAddr6, ds_Net_IIPv6Address** addr); \
   AEEResult (*GetScopeID)(iname* _pif, unsigned int* scopeID)
AEEINTERFACE_DEFINE(ds_Net_INetworkIPv6);

/** @memberof ds_Net_INetworkIPv6
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INetworkIPv6_AddRef(ds_Net_INetworkIPv6* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkIPv6)->AddRef(_pif);
}

/** @memberof ds_Net_INetworkIPv6
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INetworkIPv6_Release(ds_Net_INetworkIPv6* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkIPv6)->Release(_pif);
}

/** @memberof ds_Net_INetworkIPv6
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_INetworkIPv6_QueryInterface(ds_Net_INetworkIPv6* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkIPv6)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_INetworkIPv6
  * 
  * This function requests to generate a private IPv6 address.
  * This API is part of functionality based on RFC 3041 - 
  * "Privacy Extensions for Stateless Address Autoconfiguration in IPv6".
  * @param _pif Pointer to interface
  * @param unique TRUE if the user wishes to generate a unique address,
  *               FALSE if the user wishes to generate a shared address.
  * @param addr Object that will hold the IPv6 address.
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkIPv6_GeneratePrivAddr(ds_Net_INetworkIPv6* _pif, boolean unique, ds_Net_IIPv6Address** addr)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkIPv6)->GeneratePrivAddr(_pif, unique, addr);
}

/** @memberof ds_Net_INetworkIPv6
  * 
  * This function return IIPv6Address object.
  * Use this API to obtain an IIPv6Address object once the public IPv6
  * address is fetched via INetwork_IPAddr. That object can be used to
  * register for notification on state change of the address.
  * @param _pif Pointer to interface
  * @param inAddr6 Binary address to be used to build the IIPv6Address object.
  * @param addr Object that will hold the IPv6 address.
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  * @See INetwork_IPAddr
  * @See IPv6AddrEvent_QDS_EV_STATE_CHANGED
  */
static __inline AEEResult ds_Net_INetworkIPv6_GetIPv6Address(ds_Net_INetworkIPv6* _pif, const ds_INAddr6Type inAddr6, ds_Net_IIPv6Address** addr)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkIPv6)->GetIPv6Address(_pif, inAddr6, addr);
}

/** @memberof ds_Net_INetworkIPv6
  * 
  * This function returns the scope id 
  * @param _pif Pointer to interface
  * @param scopeID Object that will hold the scope id.
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkIPv6_GetScopeID(ds_Net_INetworkIPv6* _pif, unsigned int* scopeID)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkIPv6)->GetScopeID(_pif, scopeID);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "ds_Addr_Def.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_Def.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IIPv6Address.h"
namespace ds
{
   namespace Net
   {
      const ::AEEIID AEEIID_INetworkIPv6 = 0x106dcc5;
      
      /** @interface INetworkIPv6
        * 
        * ds Network IPv6 interface.
        */
      struct INetworkIPv6 : public ::IQI
      {
         
         /**
           * This function requests to generate a private IPv6 address.
           * This API is part of functionality based on RFC 3041 - 
           * "Privacy Extensions for Stateless Address Autoconfiguration in IPv6".
           * @param unique TRUE if the user wishes to generate a unique address,
           *               FALSE if the user wishes to generate a shared address.
           * @param addr Object that will hold the IPv6 address.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GeneratePrivAddr(boolean unique, ::ds::Net::IIPv6Address** addr) = 0;
         
         /**
           * This function return IIPv6Address object.
           * Use this API to obtain an IIPv6Address object once the public IPv6
           * address is fetched via INetwork::IPAddr. That object can be used to
           * register for notification on state change of the address.
           * @param inAddr6 Binary address to be used to build the IIPv6Address object.
           * @param addr Object that will hold the IPv6 address.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           * @See INetwork::IPAddr
           * @See IPv6AddrEvent::QDS_EV_STATE_CHANGED
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetIPv6Address(const ::ds::INAddr6Type inAddr6, ::ds::Net::IIPv6Address** addr) = 0;
         
         /**
           * This function returns the scope id 
           * @param scopeID Object that will hold the scope id.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetScopeID(unsigned int* scopeID) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_INETWORKIPV6_H
