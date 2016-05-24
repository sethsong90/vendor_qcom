#ifndef DS_NET_INETWORKCONTROL_H
#define DS_NET_INETWORKCONTROL_H

/*============================================================================
  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "ds_Net_Def.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
typedef int ds_Net_ReleaseReasonType;
#define ds_Net_ReleaseReason_QDS_NONE 1
#define ds_Net_ReleaseReason_QDS_LOW_BATTERY 2
#define ds_Net_NetworkControlEvent_QDS_EV_EXTENDED_IP_CONFIG 0x1074cd5
#define ds_Net_AEEIID_INetworkControl 0x109a89f

/** @interface ds_Net_INetworkControl
  * 
  * ds NetworkControl interface.
  * This is an auxiliary interface to INetwork, supporting some advanced 
  * Network control operations.
  *
  * Events that can be registered on this interface via OnStateChange (as part of IEventManager interface): 
  * - ds_Net_NetworkControlEvent_EXTENDED_IP_CONFIG. Use GetDHCPRefreshResult to fetch the result of the DHCP refresh.
  */
#define INHERIT_ds_Net_INetworkControl(iname) \
   INHERIT_ds_Net_IEventManager(iname); \
   AEEResult (*GoDormant)(iname* _pif, ds_Net_DormantReasonType dormantReason); \
   AEEResult (*GoNull)(iname* _pif, ds_Net_ReleaseReasonType nullReason); \
   AEEResult (*RefreshDHCPConfigInfo)(iname* _pif); \
   AEEResult (*GetDHCPRefreshResult)(iname* _pif, boolean* res)
AEEINTERFACE_DEFINE(ds_Net_INetworkControl);

/** @memberof ds_Net_INetworkControl
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INetworkControl_AddRef(ds_Net_INetworkControl* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkControl)->AddRef(_pif);
}

/** @memberof ds_Net_INetworkControl
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INetworkControl_Release(ds_Net_INetworkControl* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkControl)->Release(_pif);
}

/** @memberof ds_Net_INetworkControl
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_INetworkControl_QueryInterface(ds_Net_INetworkControl* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkControl)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_INetworkControl
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
static __inline AEEResult ds_Net_INetworkControl_OnStateChange(ds_Net_INetworkControl* _pif, ISignal* signal, ds_Net_EventType eventID, IQI** regObj)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkControl)->OnStateChange(_pif, signal, eventID, regObj);
}

/** @memberof ds_Net_INetworkControl
  * 
  * This function is used to explicitly put into dormancy the Default (Primary) Physical 
  * Link associated with this Network *regardless* of other entities that are using it 
  * when this function is called.
  * Dormancy on a link implies the releasing of the traffic channel and other air
  * interface channels to free up appropriate radio resources.
  * The link is automatically brought out of dormancy when applicable operations are
  * applied by users of this Network.
  * The state of the Network object should remain OPEN while the Physical
  * link is dormant.
  * This function requires special privilege. 
  * @param _pif Pointer to interface
  * @param dormantReason Reason why attempting to put the network into dormancy.
  * @retval AEE_SUCCESS Request received successfully.
  * @retval AEE_EWOULDBLOCK Request is processed asynchronous. Application may fetch the
  *                         Physical Link object and register for Physical Link STATE_CHANGED
  *                         events in order to identify when the Physical Link is DOWN.          
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkControl_GoDormant(ds_Net_INetworkControl* _pif, ds_Net_DormantReasonType dormantReason)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkControl)->GoDormant(_pif, dormantReason);
}

/** @memberof ds_Net_INetworkControl
  * 
  * This function is used to explicitly bring down the network connection.
  * Note that the applicable Network interface shall be brought down as well
  * *regardless* of other entities that are using it when this function is called.
  * This function is supported only for MONITORED mode Network objects. 
  * This function requires special privilege. 
  * @param _pif Pointer to interface
  * @param nullReason Reason why attempting to tear down the network.
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkControl_GoNull(ds_Net_INetworkControl* _pif, ds_Net_ReleaseReasonType nullReason)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkControl)->GoNull(_pif, nullReason);
}

/** @memberof ds_Net_INetworkControl
  * 
  * This function used to refresh the DHCP configuration information.
  * @param _pif Pointer to interface
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkControl_RefreshDHCPConfigInfo(ds_Net_INetworkControl* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkControl)->RefreshDHCPConfigInfo(_pif);
}

/** @memberof ds_Net_INetworkControl
  * 
  * This function is used to get the result of a DHCP config refresh.
  * @param _pif Pointer to interface
  * @param res The result of the refresh: true if successful, false otherwise.
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkControl_GetDHCPRefreshResult(ds_Net_INetworkControl* _pif, boolean* res)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkControl)->GetDHCPRefreshResult(_pif, res);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "ds_Net_Def.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
namespace ds
{
   namespace Net
   {
      typedef int ReleaseReasonType;
      namespace ReleaseReason
      {
         const ::ds::Net::ReleaseReasonType QDS_NONE = 1;
         const ::ds::Net::ReleaseReasonType QDS_LOW_BATTERY = 2;
      };
      namespace NetworkControlEvent
      {
         const ::ds::Net::EventType QDS_EV_EXTENDED_IP_CONFIG = 0x1074cd5;
      };
      const ::AEEIID AEEIID_INetworkControl = 0x109a89f;
      
      /** @interface INetworkControl
        * 
        * ds NetworkControl interface.
        * This is an auxiliary interface to INetwork, supporting some advanced 
        * Network control operations.
        *
        * Events that can be registered on this interface via OnStateChange (as part of IEventManager interface): 
        * - ds::Net::NetworkControlEvent::EXTENDED_IP_CONFIG. Use GetDHCPRefreshResult to fetch the result of the DHCP refresh.
        */
      struct INetworkControl : public ::ds::Net::IEventManager
      {
         
         /**
           * This function is used to explicitly put into dormancy the Default (Primary) Physical 
           * Link associated with this Network *regardless* of other entities that are using it 
           * when this function is called.
           * Dormancy on a link implies the releasing of the traffic channel and other air
           * interface channels to free up appropriate radio resources.
           * The link is automatically brought out of dormancy when applicable operations are
           * applied by users of this Network.
           * The state of the Network object should remain OPEN while the Physical
           * link is dormant.
           * This function requires special privilege. 
           * @param dormantReason Reason why attempting to put the network into dormancy.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval AEE_EWOULDBLOCK Request is processed asynchronous. Application may fetch the
           *                         Physical Link object and register for Physical Link STATE_CHANGED
           *                         events in order to identify when the Physical Link is DOWN.          
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GoDormant(::ds::Net::DormantReasonType dormantReason) = 0;
         
         /**
           * This function is used to explicitly bring down the network connection.
           * Note that the applicable Network interface shall be brought down as well
           * *regardless* of other entities that are using it when this function is called.
           * This function is supported only for MONITORED mode Network objects. 
           * This function requires special privilege. 
           * @param nullReason Reason why attempting to tear down the network.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GoNull(::ds::Net::ReleaseReasonType nullReason) = 0;
         
         /**
           * This function used to refresh the DHCP configuration information.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL RefreshDHCPConfigInfo() = 0;
         
         /**
           * This function is used to get the result of a DHCP config refresh.
           * @param res The result of the refresh: true if successful, false otherwise.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetDHCPRefreshResult(boolean* res) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_INETWORKCONTROL_H
