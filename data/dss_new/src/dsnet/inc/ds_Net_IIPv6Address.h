#ifndef DS_NET_IIPV6ADDRESS_H
#define DS_NET_IIPV6ADDRESS_H

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
#define ds_Net_IPv6AddrEvent_QDS_EV_STATE_CHANGED 0x106e60a
typedef int ds_Net_IPv6AddrStateType;
#define ds_Net_IPv6AddrState_PRIV_ADDR_DELETED 0
#define ds_Net_IPv6AddrState_PRIV_ADDR_WAITING 1
#define ds_Net_IPv6AddrState_PRIV_ADDR_AVAILABLE 3
#define ds_Net_IPv6AddrState_PRIV_ADDR_DEPRECATED 4
#define ds_Net_AEEIID_IIPv6Address 0x106df4c

/** @interface ds_Net_IIPv6Address
  * 
  * IPv6 address interface. 
  * For public IPv6 address get an instance of this interface via
  * INetworkIPv6_GetIPv6Address (after fetching the address itself via
  * INetwork_IPAddr).
  * For private IPv6 address get an instance of this interface via
  * INetworkIPv6_GeneratePrivAddr.
  * 
  * Events that can be registered on this interface via OnStateChange (as part of IEventManager interface): 
  * - ds_Net_IPv6AddrEvent_STATE_CHANGED. Use GetState API to fetch information regarding the new state.
  */
#define INHERIT_ds_Net_IIPv6Address(iname) \
   INHERIT_ds_Net_IEventManager(iname); \
   AEEResult (*GetAddress)(iname* _pif, ds_INAddr6Type value); \
   AEEResult (*GetState)(iname* _pif, ds_Net_IPv6AddrStateType* state)
AEEINTERFACE_DEFINE(ds_Net_IIPv6Address);

/** @memberof ds_Net_IIPv6Address
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IIPv6Address_AddRef(ds_Net_IIPv6Address* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPv6Address)->AddRef(_pif);
}

/** @memberof ds_Net_IIPv6Address
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IIPv6Address_Release(ds_Net_IIPv6Address* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPv6Address)->Release(_pif);
}

/** @memberof ds_Net_IIPv6Address
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IIPv6Address_QueryInterface(ds_Net_IIPv6Address* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPv6Address)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IIPv6Address
  * 
  * This function registers a signal to be set when event eventID occurs.
  * Cannot use the signal for more than one eventID. Can use more than one
  * signal for the same event.
  * @param _pif Pointer to interface
  * @param signal The signal to Set() when the state associated with the eventID changes.
  *               To cancel the registration the application should release this signal. 
  * @param eventID The event for which the signal shall be fired.
  * @param regObj Output The application must hold this output registration object to ensure its
  *                      Signal stays registered. The application can release this object once
  *                      it has released the signal object.
  * @retval AEE_SUCCESS Signal set completed successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IIPv6Address_OnStateChange(ds_Net_IIPv6Address* _pif, ISignal* signal, ds_Net_EventType eventID, IQI** regObj)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPv6Address)->OnStateChange(_pif, signal, eventID, regObj);
}

/** @memberof ds_Net_IIPv6Address
  * 
  * This attribute contains the IPv6 address encapsulated by this interface.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IIPv6Address_GetAddress(ds_Net_IIPv6Address* _pif, ds_INAddr6Type value)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPv6Address)->GetAddress(_pif, value);
}

/** @memberof ds_Net_IIPv6Address
  * 
  * This function is used to get the state of the IPv6 address.
  * @param _pif Pointer to interface
  * @param state Holds the state of the address.
  * @see ds_StateType.
  * @retval AEE_SUCCESS Registration succeeded.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IIPv6Address_GetState(ds_Net_IIPv6Address* _pif, ds_Net_IPv6AddrStateType* state)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPv6Address)->GetState(_pif, state);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "ds_Addr_Def.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_Def.h"
#include "ds_Net_IEventManager.h"
namespace ds
{
   namespace Net
   {
      namespace IPv6AddrEvent
      {
         const ::ds::Net::EventType QDS_EV_STATE_CHANGED = 0x106e60a;
      };
      typedef int IPv6AddrStateType;
      namespace IPv6AddrState
      {
         const ::ds::Net::IPv6AddrStateType PRIV_ADDR_DELETED = 0;
         const ::ds::Net::IPv6AddrStateType PRIV_ADDR_WAITING = 1;
         const ::ds::Net::IPv6AddrStateType PRIV_ADDR_AVAILABLE = 3;
         const ::ds::Net::IPv6AddrStateType PRIV_ADDR_DEPRECATED = 4;
      };
      const ::AEEIID AEEIID_IIPv6Address = 0x106df4c;
      
      /** @interface IIPv6Address
        * 
        * IPv6 address interface. 
        * For public IPv6 address get an instance of this interface via
        * INetworkIPv6::GetIPv6Address (after fetching the address itself via
        * INetwork::IPAddr).
        * For private IPv6 address get an instance of this interface via
        * INetworkIPv6::GeneratePrivAddr.
        * 
        * Events that can be registered on this interface via OnStateChange (as part of IEventManager interface): 
        * - ds::Net::IPv6AddrEvent::STATE_CHANGED. Use GetState API to fetch information regarding the new state.
        */
      struct IIPv6Address : public ::ds::Net::IEventManager
      {
         
         /**
           * This attribute contains the IPv6 address encapsulated by this interface.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetAddress(::ds::INAddr6Type value) = 0;
         
         /**
           * This function is used to get the state of the IPv6 address.
           * @param state Holds the state of the address.
           * @see ds::StateType.
           * @retval AEE_SUCCESS Registration succeeded.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetState(::ds::Net::IPv6AddrStateType* state) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IIPV6ADDRESS_H
