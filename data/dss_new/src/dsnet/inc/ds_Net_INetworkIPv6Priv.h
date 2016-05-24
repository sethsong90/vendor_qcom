#ifndef DS_NET_INETWORKIPV6PRIV_H
#define DS_NET_INETWORKIPV6PRIV_H

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
#define ds_Net_IPv6PrivEvent_QDS_EV_PREFIX_CHANGED 0x1089294
struct ds_Net_IPv6PrivPrefixInfoType {
   ds_INAddr6Type prefix;
   ds_Net_IPv6AddrStateType prefixType;
   unsigned short prefixLen;
   AEEINTERFACE_PADMEMBERS(__pad, 2)
};
typedef struct ds_Net_IPv6PrivPrefixInfoType ds_Net_IPv6PrivPrefixInfoType;
struct ds_Net__SeqIPv6PrivPrefixInfoType__seq_IPv6PrivPrefixInfoType_Net_ds {
   ds_Net_IPv6PrivPrefixInfoType* data;
   int dataLen;
   int dataLenReq;
};
typedef struct ds_Net__SeqIPv6PrivPrefixInfoType__seq_IPv6PrivPrefixInfoType_Net_ds ds_Net__SeqIPv6PrivPrefixInfoType__seq_IPv6PrivPrefixInfoType_Net_ds;
typedef ds_Net__SeqIPv6PrivPrefixInfoType__seq_IPv6PrivPrefixInfoType_Net_ds ds_Net_SeqIPv6PrivPrefixInfoType;
#define ds_Net_AEEIID_INetworkIPv6Priv 0x1089273

/** @interface ds_Net_INetworkIPv6Priv
  * 
  * ds Net Network Privileged interface.
  * This interface is intended for internal usage for backward compatibility purposes.
  * INetworkIPv6Priv address interface.
  * Events that can be registered on this interface via OnStateChange (as part of IEventManager interface): 
  * - ds_Net_IPv6PrivEvent_QDS_EV_PREFIX_CHANGED. 
  */
#define INHERIT_ds_Net_INetworkIPv6Priv(iname) \
   INHERIT_ds_Net_IEventManager(iname); \
   AEEResult (*GetAllIPv6Prefixes)(iname* _pif, ds_Net_IPv6PrivPrefixInfoType* allPrefixes, int allPrefixesLen, int* allPrefixesLenReq)
AEEINTERFACE_DEFINE(ds_Net_INetworkIPv6Priv);

/** @memberof ds_Net_INetworkIPv6Priv
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INetworkIPv6Priv_AddRef(ds_Net_INetworkIPv6Priv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkIPv6Priv)->AddRef(_pif);
}

/** @memberof ds_Net_INetworkIPv6Priv
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INetworkIPv6Priv_Release(ds_Net_INetworkIPv6Priv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkIPv6Priv)->Release(_pif);
}

/** @memberof ds_Net_INetworkIPv6Priv
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_INetworkIPv6Priv_QueryInterface(ds_Net_INetworkIPv6Priv* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkIPv6Priv)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_INetworkIPv6Priv
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
static __inline AEEResult ds_Net_INetworkIPv6Priv_OnStateChange(ds_Net_INetworkIPv6Priv* _pif, ISignal* signal, ds_Net_EventType eventID, IQI** regObj)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkIPv6Priv)->OnStateChange(_pif, signal, eventID, regObj);
}

/** @memberof ds_Net_INetworkIPv6Priv
  * 
  * This function used to get all available prefixes
  * @param _pif Pointer to interface
  * @param allPrefixes List , prefixes with DELETED state should not be included
  * @param allPrefixesLen Length of sequence
  * @param allPrefixesLenReq Required length of sequence
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkIPv6Priv_GetAllIPv6Prefixes(ds_Net_INetworkIPv6Priv* _pif, ds_Net_IPv6PrivPrefixInfoType* allPrefixes, int allPrefixesLen, int* allPrefixesLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkIPv6Priv)->GetAllIPv6Prefixes(_pif, allPrefixes, allPrefixesLen, allPrefixesLenReq);
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
      namespace IPv6PrivEvent
      {
         const ::ds::Net::EventType QDS_EV_PREFIX_CHANGED = 0x1089294;
      };
      struct IPv6PrivPrefixInfoType {
         ::ds::INAddr6Type prefix;
         IPv6AddrStateType prefixType;
         unsigned short prefixLen;
         AEEINTERFACE_PADMEMBERS(__pad, 2)
      };
      struct _SeqIPv6PrivPrefixInfoType__seq_IPv6PrivPrefixInfoType_Net_ds {
         IPv6PrivPrefixInfoType* data;
         int dataLen;
         int dataLenReq;
      };
      typedef _SeqIPv6PrivPrefixInfoType__seq_IPv6PrivPrefixInfoType_Net_ds SeqIPv6PrivPrefixInfoType;
      const ::AEEIID AEEIID_INetworkIPv6Priv = 0x1089273;
      
      /** @interface INetworkIPv6Priv
        * 
        * ds Net Network Privileged interface.
        * This interface is intended for internal usage for backward compatibility purposes.
        * INetworkIPv6Priv address interface.
        * Events that can be registered on this interface via OnStateChange (as part of IEventManager interface): 
        * - ds::Net::IPv6PrivEvent::QDS_EV_PREFIX_CHANGED. 
        */
      struct INetworkIPv6Priv : public ::ds::Net::IEventManager
      {
         
         /**
           * This function used to get all available prefixes
           * @param allPrefixes List , prefixes with DELETED state should not be included
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetAllIPv6Prefixes(::ds::Net::IPv6PrivPrefixInfoType* allPrefixes, int allPrefixesLen, int* allPrefixesLenReq) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_INETWORKIPV6PRIV_H
