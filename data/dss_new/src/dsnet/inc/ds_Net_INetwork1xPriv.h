#ifndef DS_NET_INETWORK1XPRIV_H
#define DS_NET_INETWORK1XPRIV_H

/*============================================================================
  Copyright (c) 2008-2011 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_Def.h"
#include "ds_Net_IEventManager.h"
#include "ds_Addr_Def.h"
#include "ds_Net_DownReasons_Def.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_INetwork.h"
#include "ds_Net_INetwork1x.h"
#define ds_Net_Network1xPrivEvent_SLOTTED_MODE_RESULT 0x106e11a
#define ds_Net_Network1xPrivEvent_SLOTTED_MODE_CHANGED 0x106e11b
#define ds_Net_Network1xPrivEvent_HDR_REV0_RATE_INERTIA_RESULT 0x106e11c
struct ds_Net_Network1xPrivHDRSlottedModeArg {
   boolean enable;
   AEEINTERFACE_PADMEMBERS(slottedModeOption, 3)
   int slottedModeOption;
   int getSlottedMode;
};
typedef struct ds_Net_Network1xPrivHDRSlottedModeArg ds_Net_Network1xPrivHDRSlottedModeArg;
typedef int ds_Net_Network1xPrivResultCodeType;
#define ds_Net_Network1xPrivSlottedMode_REQUEST_SUCCEES 0
#define ds_Net_Network1xPrivSlottedMode_REQUEST_REJECTED 1
#define ds_Net_Network1xPrivSlottedMode_REQUEST_FAILED_TX 2
#define ds_Net_Network1xPrivSlottedMode_REQUEST_NO_NET 3
#define ds_Net_Network1xPrivSlottedMode_REQUEST_UNSUPPORTED 4
typedef int ds_Net_Network1xPrivSessionTimerSelectType;
#define ds_Net_Network1xPrivSessionTimerSelect_S_INVALID 0
#define ds_Net_Network1xPrivSessionTimerSelect_S_DO 1
#define ds_Net_Network1xPrivSessionTimerSelect_S_1X 2
#define ds_Net_Network1xPrivSessionTimerSelect_S_1X_AND_DO 3
struct ds_Net_Network1xPrivSessionTimerType {
   ds_Net_Network1xPrivSessionTimerSelectType select;
   short value;
   AEEINTERFACE_PADMEMBERS(__pad, 2)
};
typedef struct ds_Net_Network1xPrivSessionTimerType ds_Net_Network1xPrivSessionTimerType;
typedef int ds_Net_Network1xPrivHDRRev0RateInertiaFailureCodeType;
#define ds_Net_Network1xPrivHDRRev0RateInertiaFailureCode_REQUEST_REJECTED 0
#define ds_Net_Network1xPrivHDRRev0RateInertiaFailureCode_REQUEST_FAILED_TX 1
#define ds_Net_Network1xPrivHDRRev0RateInertiaFailureCode_NOT_SUPPORTED 2
#define ds_Net_Network1xPrivHDRRev0RateInertiaFailureCode_NO_NET 3
#define ds_Net_AEEIID_INetwork1xPriv 0x10741e0

/** @interface ds_Net_INetwork1xPriv
  * 
  * ds Net Network 1x Priv interface.
  * Events that can be registered on this interface via OnStateChange (as part of IEventManager interface): 
  * - ds_Net_Network1xPrivEvent_SLOTTED_MODE_RESULT. Use GetHDRSlottedModeResult to fetch the slotted mode result information.
  * - ds_Net_Network1xPrivEvent_SLOTTED_MODE_CHANGED. Use GetHDRSlottedModeCycleIndex to fetch the new slotted mode cycle index. TODO: Verify that this API provides the information relevant to this event.
  * - ds_Net_Network1xPrivEvent_HDR_REV0_RATE_INERTIA_RESULT. Use GetHDRRev0RateInteriaResult to fetch the rate information.
  */
#define INHERIT_ds_Net_INetwork1xPriv(iname) \
   INHERIT_ds_Net_IEventManager(iname); \
   AEEResult (*EnableHoldDown)(iname* _pif, boolean enable); \
   AEEResult (*EnableHDRRev0RateInertia)(iname* _pif, boolean enable); \
   AEEResult (*GetHDRRev0RateInertiaResult)(iname* _pif, boolean* result); \
   AEEResult (*EnableHDRSlottedMode)(iname* _pif, const ds_Net_Network1xPrivHDRSlottedModeArg* arg); \
   AEEResult (*GetHDRSlottedModeResult)(iname* _pif, ds_Net_Network1xPrivResultCodeType* resultCode); \
   AEEResult (*GetHDRSlottedModeCycleIndex)(iname* _pif, uint32 * sci); \
   AEEResult (*EnableHDRHPTMode)(iname* _pif, boolean enable); \
   int (*GetDormancyTimer)(iname* _pif, int* value); \
   int (*SetDormancyTimer)(iname* _pif, int value); \
   int (*GetSessionTimer)(iname* _pif, ds_Net_Network1xPrivSessionTimerType* value); \
   int (*SetSessionTimer)(iname* _pif, const ds_Net_Network1xPrivSessionTimerType* value); \
   int (*GetHDR1xHandDownOption)(iname* _pif, boolean* value); \
   int (*SetHDR1xHandDownOption)(iname* _pif, boolean value); \
   AEEResult (*GetHDRRev0RateInertiaResultInfoCode)(iname* _pif, ds_Net_Network1xPrivHDRRev0RateInertiaFailureCodeType* failureCode)
AEEINTERFACE_DEFINE(ds_Net_INetwork1xPriv);

/** @memberof ds_Net_INetwork1xPriv
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INetwork1xPriv_AddRef(ds_Net_INetwork1xPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1xPriv)->AddRef(_pif);
}

/** @memberof ds_Net_INetwork1xPriv
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INetwork1xPriv_Release(ds_Net_INetwork1xPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1xPriv)->Release(_pif);
}

/** @memberof ds_Net_INetwork1xPriv
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_INetwork1xPriv_QueryInterface(ds_Net_INetwork1xPriv* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1xPriv)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_INetwork1xPriv
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
static __inline AEEResult ds_Net_INetwork1xPriv_OnStateChange(ds_Net_INetwork1xPriv* _pif, ISignal* signal, ds_Net_EventType eventID, IQI** regObj)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1xPriv)->OnStateChange(_pif, signal, eventID, regObj);
}

/** @memberof ds_Net_INetwork1xPriv
  * 
  * This function is used to set the 1x holddown timer.
  * @param _pif Pointer to interface
  * @param enable Used to enable/disable the functionality.
  * @retval AEE_SUCCESS Registration succeeded.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetwork1xPriv_EnableHoldDown(ds_Net_INetwork1xPriv* _pif, boolean enable)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1xPriv)->EnableHoldDown(_pif, enable);
}

/** @memberof ds_Net_INetwork1xPriv
  * 
  * This function is used to set the mode for operating VT on 
  * a Rev 0 HDR system.
  * @param _pif Pointer to interface
  * @param enable Used to enable/disable the functionality.
  * @retval AEE_SUCCESS Registration succeeded.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetwork1xPriv_EnableHDRRev0RateInertia(ds_Net_INetwork1xPriv* _pif, boolean enable)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1xPriv)->EnableHDRRev0RateInertia(_pif, enable);
}

/** @memberof ds_Net_INetwork1xPriv
  * 
  * This function is used to get the result of attempting to 
  * set the mode for operating VT on a Rev 0 HDR system.
  * @param _pif Pointer to interface
  * @param result Holds the result.
  * @retval AEE_SUCCESS Registration succeeded.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetwork1xPriv_GetHDRRev0RateInertiaResult(ds_Net_INetwork1xPriv* _pif, boolean* result)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1xPriv)->GetHDRRev0RateInertiaResult(_pif, result);
}

/** @memberof ds_Net_INetwork1xPriv
  * 
  * This function is used to set the HDR slotted mode.
  * @param _pif Pointer to interface
  * @param arg Slotted mode argument.
  * @see ds_Net_Network1xPriv_HDRSlottedModeArg.
  * @retval AEE_SUCCESS Registration succeeded.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetwork1xPriv_EnableHDRSlottedMode(ds_Net_INetwork1xPriv* _pif, const ds_Net_Network1xPrivHDRSlottedModeArg* arg)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1xPriv)->EnableHDRSlottedMode(_pif, arg);
}

/** @memberof ds_Net_INetwork1xPriv
  * 
  * This function is used to get the result of attempting to 
  * set HDR slotted mode.
  * @param _pif Pointer to interface
  * @param resultCode Holds the result.
  * @see ds_Net_Network1x_ResultCodeType.
  * @retval AEE_SUCCESS Registration succeeded.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetwork1xPriv_GetHDRSlottedModeResult(ds_Net_INetwork1xPriv* _pif, ds_Net_Network1xPrivResultCodeType* resultCode)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1xPriv)->GetHDRSlottedModeResult(_pif, resultCode);
}

/** @memberof ds_Net_INetwork1xPriv
  * 
  * This function is used to get HDR slotted mode cycle index.
  * @param _pif Pointer to interface
  * @param sci Holds the HDR Slotted mode cycle index.
  * @retval AEE_SUCCESS Registration succeeded.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetwork1xPriv_GetHDRSlottedModeCycleIndex(ds_Net_INetwork1xPriv* _pif, uint32* sci)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1xPriv)->GetHDRSlottedModeCycleIndex(_pif, sci);
}

/** @memberof ds_Net_INetwork1xPriv
  * 
  * This function is used to set the HDR HPT mode.
  * @param _pif Pointer to interface
  * @param enable Used to enable/disable the functionality.
  * @retval AEE_SUCCESS Registration succeeded.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetwork1xPriv_EnableHDRHPTMode(ds_Net_INetwork1xPriv* _pif, boolean enable)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1xPriv)->EnableHDRHPTMode(_pif, enable);
}

/** @memberof ds_Net_INetwork1xPriv
  * 
  * This attribute indicates the dormacy timer.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline int ds_Net_INetwork1xPriv_GetDormancyTimer(ds_Net_INetwork1xPriv* _pif, int* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1xPriv)->GetDormancyTimer(_pif, value);
}

/** @memberof ds_Net_INetwork1xPriv
  * 
  * This attribute indicates the dormacy timer.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline int ds_Net_INetwork1xPriv_SetDormancyTimer(ds_Net_INetwork1xPriv* _pif, int value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1xPriv)->SetDormancyTimer(_pif, value);
}

/** @memberof ds_Net_INetwork1xPriv
  * 
  * This attribute indicates the session timer.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline int ds_Net_INetwork1xPriv_GetSessionTimer(ds_Net_INetwork1xPriv* _pif, ds_Net_Network1xPrivSessionTimerType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1xPriv)->GetSessionTimer(_pif, value);
}

/** @memberof ds_Net_INetwork1xPriv
  * 
  * This attribute indicates the session timer.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline int ds_Net_INetwork1xPriv_SetSessionTimer(ds_Net_INetwork1xPriv* _pif, const ds_Net_Network1xPrivSessionTimerType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1xPriv)->SetSessionTimer(_pif, value);
}

/** @memberof ds_Net_INetwork1xPriv
  * 
  * This attribute indicates the 1x HandDown Option information.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline int ds_Net_INetwork1xPriv_GetHDR1xHandDownOption(ds_Net_INetwork1xPriv* _pif, boolean* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1xPriv)->GetHDR1xHandDownOption(_pif, value);
}

/** @memberof ds_Net_INetwork1xPriv
  * 
  * This attribute indicates the 1x HandDown Option information.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline int ds_Net_INetwork1xPriv_SetHDR1xHandDownOption(ds_Net_INetwork1xPriv* _pif, boolean value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1xPriv)->SetHDR1xHandDownOption(_pif, value);
}

/** @memberof ds_Net_INetwork1xPriv
  * 
  * This function gets the result info code from HDR Rev0 
  * Rate inertia event.
  * @param _pif Pointer to interface
  * @param infoCode Info code returned
  * @retval AEE_SUCCESS Registration succeeded.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetwork1xPriv_GetHDRRev0RateInertiaResultInfoCode(ds_Net_INetwork1xPriv* _pif, ds_Net_Network1xPrivHDRRev0RateInertiaFailureCodeType* failureCode)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1xPriv)->GetHDRRev0RateInertiaResultInfoCode(_pif, failureCode);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_Def.h"
#include "ds_Net_IEventManager.h"
#include "ds_Addr_Def.h"
#include "ds_Net_DownReasons_Def.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_INetwork.h"
#include "ds_Net_INetwork1x.h"
namespace ds
{
   namespace Net
   {
      namespace Network1xPrivEvent
      {
         const int SLOTTED_MODE_RESULT = 0x106e11a;
         const int SLOTTED_MODE_CHANGED = 0x106e11b;
         const int HDR_REV0_RATE_INERTIA_RESULT = 0x106e11c;
      };
      struct Network1xPrivHDRSlottedModeArg {
         boolean enable;
         AEEINTERFACE_PADMEMBERS(slottedModeOption, 3)
         int slottedModeOption;
         int getSlottedMode;
      };
      typedef int Network1xPrivResultCodeType;
      namespace Network1xPrivSlottedMode
      {
         const ::ds::Net::Network1xPrivResultCodeType REQUEST_SUCCEES = 0;
         const ::ds::Net::Network1xPrivResultCodeType REQUEST_REJECTED = 1;
         const ::ds::Net::Network1xPrivResultCodeType REQUEST_FAILED_TX = 2;
         const ::ds::Net::Network1xPrivResultCodeType REQUEST_NO_NET = 3;
         const ::ds::Net::Network1xPrivResultCodeType REQUEST_UNSUPPORTED = 4;
      };
      typedef int Network1xPrivSessionTimerSelectType;
      namespace Network1xPrivSessionTimerSelect
      {
         const ::ds::Net::Network1xPrivSessionTimerSelectType S_INVALID = 0;
         const ::ds::Net::Network1xPrivSessionTimerSelectType S_DO = 1;
         const ::ds::Net::Network1xPrivSessionTimerSelectType S_1X = 2;
         const ::ds::Net::Network1xPrivSessionTimerSelectType S_1X_AND_DO = 3;
      };
      struct Network1xPrivSessionTimerType {
         Network1xPrivSessionTimerSelectType select;
         short value;
         AEEINTERFACE_PADMEMBERS(__pad, 2)
      };
      typedef int Network1xPrivHDRRev0RateInertiaFailureCodeType;
      namespace Network1xPrivHDRRev0RateInertiaFailureCode
      {
         const ::ds::Net::Network1xPrivHDRRev0RateInertiaFailureCodeType REQUEST_REJECTED = 0;
         const ::ds::Net::Network1xPrivHDRRev0RateInertiaFailureCodeType REQUEST_FAILED_TX = 1;
         const ::ds::Net::Network1xPrivHDRRev0RateInertiaFailureCodeType NOT_SUPPORTED = 2;
         const ::ds::Net::Network1xPrivHDRRev0RateInertiaFailureCodeType NO_NET = 3;
      };
      const ::AEEIID AEEIID_INetwork1xPriv = 0x10741e0;
      
      /** @interface INetwork1xPriv
        * 
        * ds Net Network 1x Priv interface.
        * Events that can be registered on this interface via OnStateChange (as part of IEventManager interface): 
        * - ds::Net::Network1xPrivEvent::SLOTTED_MODE_RESULT. Use GetHDRSlottedModeResult to fetch the slotted mode result information.
        * - ds::Net::Network1xPrivEvent::SLOTTED_MODE_CHANGED. Use GetHDRSlottedModeCycleIndex to fetch the new slotted mode cycle index. TODO: Verify that this API provides the information relevant to this event.
        * - ds::Net::Network1xPrivEvent::HDR_REV0_RATE_INERTIA_RESULT. Use GetHDRRev0RateInteriaResult to fetch the rate information.
        */
      struct INetwork1xPriv : public ::ds::Net::IEventManager
      {
         
         /**
           * This function is used to set the 1x holddown timer.
           * @param enable Used to enable/disable the functionality.
           * @retval AEE_SUCCESS Registration succeeded.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL EnableHoldDown(boolean enable) = 0;
         
         /**
           * This function is used to set the mode for operating VT on 
           * a Rev 0 HDR system.
           * @param enable Used to enable/disable the functionality.
           * @retval AEE_SUCCESS Registration succeeded.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL EnableHDRRev0RateInertia(boolean enable) = 0;
         
         /**
           * This function is used to get the result of attempting to 
           * set the mode for operating VT on a Rev 0 HDR system.
           * @param result Holds the result.
           * @retval AEE_SUCCESS Registration succeeded.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetHDRRev0RateInertiaResult(boolean* result) = 0;
         
         /**
           * This function is used to set the HDR slotted mode.
           * @param arg Slotted mode argument.
           * @see ds::Net::Network1xPriv::HDRSlottedModeArg.
           * @retval AEE_SUCCESS Registration succeeded.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL EnableHDRSlottedMode(const ::ds::Net::Network1xPrivHDRSlottedModeArg* arg) = 0;
         
         /**
           * This function is used to get the result of attempting to 
           * set HDR slotted mode.
           * @param resultCode Holds the result.
           * @see ds::Net::Network1x::ResultCodeType.
           * @retval AEE_SUCCESS Registration succeeded.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetHDRSlottedModeResult(::ds::Net::Network1xPrivResultCodeType* resultCode) = 0;
         
         /**
           * This function is used to get HDR slotted mode cycle index.
           * @param sci Holds the HDR Slotted mode cycle index.
           * @retval AEE_SUCCESS Registration succeeded.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetHDRSlottedModeCycleIndex(::uint32* sci) = 0;
         
         /**
           * This function is used to set the HDR HPT mode.
           * @param enable Used to enable/disable the functionality.
           * @retval AEE_SUCCESS Registration succeeded.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL EnableHDRHPTMode(boolean enable) = 0;
         
         /**
           * This attribute indicates the dormacy timer.
           * @param value Attribute value
           */
         virtual int AEEINTERFACE_CDECL GetDormancyTimer(int* value) = 0;
         
         /**
           * This attribute indicates the dormacy timer.
           * @param value Attribute value
           */
         virtual int AEEINTERFACE_CDECL SetDormancyTimer(int value) = 0;
         
         /**
           * This attribute indicates the session timer.
           * @param value Attribute value
           */
         virtual int AEEINTERFACE_CDECL GetSessionTimer(::ds::Net::Network1xPrivSessionTimerType* value) = 0;
         
         /**
           * This attribute indicates the session timer.
           * @param value Attribute value
           */
         virtual int AEEINTERFACE_CDECL SetSessionTimer(const ::ds::Net::Network1xPrivSessionTimerType* value) = 0;
         
         /**
           * This attribute indicates the 1x HandDown Option information.
           * @param value Attribute value
           */
         virtual int AEEINTERFACE_CDECL GetHDR1xHandDownOption(boolean* value) = 0;
         
         /**
           * This attribute indicates the 1x HandDown Option information.
           * @param value Attribute value
           */
         virtual int AEEINTERFACE_CDECL SetHDR1xHandDownOption(boolean value) = 0;
         
         /**
           * This function gets the result info code from HDR Rev0 
           * Rate inertia event.
           * @param infoCode Info code returned
           * @retval AEE_SUCCESS Registration succeeded.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetHDRRev0RateInertiaResultInfoCode(::ds::Net::Network1xPrivHDRRev0RateInertiaFailureCodeType* failureCode) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_INETWORK1XPRIV_H
