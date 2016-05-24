#ifndef DS_NET_INETWORK1X_H
#define DS_NET_INETWORK1X_H

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
typedef int ds_Net_Network1xMDRModeType;
#define ds_Net_Network1xMDRMode_MDR_ONLY 0
#define ds_Net_Network1xMDRMode_IF_AVAIL 1
#define ds_Net_Network1xMDRMode_NO_MDR 2
#define ds_Net_Network1xMDRMode_SO33_PREF 3
typedef int ds_Net_Network1xPrivHysteresisTimerType;
#define ds_Net_Network1xRLP_MAX_NAK_ROUNDS 3
struct ds_Net_Network1xRLPOptionType {
   unsigned char rscbIndex;
   unsigned char nakRoundsFwd;
   unsigned char naksPerRoundFwd[3];
   unsigned char nakRoundsRev;
   unsigned char naksPerRoundRev[3];
};
typedef struct ds_Net_Network1xRLPOptionType ds_Net_Network1xRLPOptionType;
typedef int ds_Net_Network1xDoSFlagsType;
#define ds_Net_Network1xDoSFlags_QDS_MSG_EXPEDITE 0x1
#define ds_Net_Network1xDoSFlags_QDS_MSG_FAST_EXPEDITE 0x2
#define ds_Net_AEEIID_INetwork1x 0x106ce1c

/** @interface ds_Net_INetwork1x
  * 
  * ds Net Network 1x interface.
  */
#define INHERIT_ds_Net_INetwork1x(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*QueryDoSSupport)(iname* _pif, ds_Net_Network1xDoSFlagsType flags, boolean* dosSupported); \
   AEEResult (*GetMDR)(iname* _pif, int* value); \
   AEEResult (*SetMDR)(iname* _pif, int value); \
   AEEResult (*GetQoSNAPriority)(iname* _pif, int* value); \
   AEEResult (*SetQoSNAPriority)(iname* _pif, int value); \
   AEEResult (*GetRLPAllCurrentNAK)(iname* _pif, ds_Net_Network1xRLPOptionType* value); \
   AEEResult (*SetRLPAllCurrentNAK)(iname* _pif, const ds_Net_Network1xRLPOptionType* value); \
   AEEResult (*GetRLPDefCurrentNAK)(iname* _pif, ds_Net_Network1xRLPOptionType* value); \
   AEEResult (*SetRLPDefCurrentNAK)(iname* _pif, const ds_Net_Network1xRLPOptionType* value); \
   AEEResult (*GetRLPNegCurrentNAK)(iname* _pif, ds_Net_Network1xRLPOptionType* value); \
   AEEResult (*GetHysteresisTimer)(iname* _pif, int* value); \
   AEEResult (*SetHysteresisTimer)(iname* _pif, int value)
AEEINTERFACE_DEFINE(ds_Net_INetwork1x);

/** @memberof ds_Net_INetwork1x
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INetwork1x_AddRef(ds_Net_INetwork1x* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1x)->AddRef(_pif);
}

/** @memberof ds_Net_INetwork1x
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INetwork1x_Release(ds_Net_INetwork1x* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1x)->Release(_pif);
}

/** @memberof ds_Net_INetwork1x
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_INetwork1x_QueryInterface(ds_Net_INetwork1x* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1x)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_INetwork1x
  * 
  * This function is used to query if DoS (Data Over Signaling) is
  * supported. The flags parameter is used to query support for
  * specific DoS features (same features used in Socket SendTo
  * operations).
  * DoS and SDB (Short Data Burst) are parallel terms.
  * DoS is the term used in 1xEVDO
  * SDB is the term used in CDMA 1x.
  * @param _pif Pointer to interface
  * @param flags DoS flags to be used.
  * @param dosSupported TRUE if DoS is supported.
  *                     FALSE if DoS is not supported.
  * @retval AEE_SUCCESS Registration succeeded.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetwork1x_QueryDoSSupport(ds_Net_INetwork1x* _pif, ds_Net_Network1xDoSFlagsType flags, boolean* dosSupported)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1x)->QueryDoSSupport(_pif, flags, dosSupported);
}

/** @memberof ds_Net_INetwork1x
  * 
  * This attribute indicates the medium data rate value.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_INetwork1x_GetMDR(ds_Net_INetwork1x* _pif, int* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1x)->GetMDR(_pif, value);
}

/** @memberof ds_Net_INetwork1x
  * 
  * This attribute indicates the medium data rate value.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_INetwork1x_SetMDR(ds_Net_INetwork1x* _pif, int value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1x)->SetMDR(_pif, value);
}

/** @memberof ds_Net_INetwork1x
  * 
  * This attribute indicates the QoS NA priority.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_INetwork1x_GetQoSNAPriority(ds_Net_INetwork1x* _pif, int* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1x)->GetQoSNAPriority(_pif, value);
}

/** @memberof ds_Net_INetwork1x
  * 
  * This attribute indicates the QoS NA priority.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_INetwork1x_SetQoSNAPriority(ds_Net_INetwork1x* _pif, int value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1x)->SetQoSNAPriority(_pif, value);
}

/** @memberof ds_Net_INetwork1x
  * 
  * This attribute indicates the 1x RLP current NAK policy information.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_INetwork1x_GetRLPAllCurrentNAK(ds_Net_INetwork1x* _pif, ds_Net_Network1xRLPOptionType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1x)->GetRLPAllCurrentNAK(_pif, value);
}

/** @memberof ds_Net_INetwork1x
  * 
  * This attribute indicates the 1x RLP current NAK policy information.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_INetwork1x_SetRLPAllCurrentNAK(ds_Net_INetwork1x* _pif, const ds_Net_Network1xRLPOptionType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1x)->SetRLPAllCurrentNAK(_pif, value);
}

/** @memberof ds_Net_INetwork1x
  * 
  * This attribute indicates the 1x RLP default NAK policy information.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_INetwork1x_GetRLPDefCurrentNAK(ds_Net_INetwork1x* _pif, ds_Net_Network1xRLPOptionType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1x)->GetRLPDefCurrentNAK(_pif, value);
}

/** @memberof ds_Net_INetwork1x
  * 
  * This attribute indicates the 1x RLP default NAK policy information.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_INetwork1x_SetRLPDefCurrentNAK(ds_Net_INetwork1x* _pif, const ds_Net_Network1xRLPOptionType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1x)->SetRLPDefCurrentNAK(_pif, value);
}

/** @memberof ds_Net_INetwork1x
  * 
  * This attribute indicates the 1x RLP negotiated NAK policy information.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_INetwork1x_GetRLPNegCurrentNAK(ds_Net_INetwork1x* _pif, ds_Net_Network1xRLPOptionType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1x)->GetRLPNegCurrentNAK(_pif, value);
}

/** @memberof ds_Net_INetwork1x
  * 
  * This attribute indicates the hysteresis act timer.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_INetwork1x_GetHysteresisTimer(ds_Net_INetwork1x* _pif, int* value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1x)->GetHysteresisTimer(_pif, value);
}

/** @memberof ds_Net_INetwork1x
  * 
  * This attribute indicates the hysteresis act timer.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_INetwork1x_SetHysteresisTimer(ds_Net_INetwork1x* _pif, int value)
{
   return AEEGETPVTBL(_pif, ds_Net_INetwork1x)->SetHysteresisTimer(_pif, value);
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
namespace ds
{
   namespace Net
   {
      typedef int Network1xMDRModeType;
      namespace Network1xMDRMode
      {
         const ::ds::Net::Network1xMDRModeType MDR_ONLY = 0;
         const ::ds::Net::Network1xMDRModeType IF_AVAIL = 1;
         const ::ds::Net::Network1xMDRModeType NO_MDR = 2;
         const ::ds::Net::Network1xMDRModeType SO33_PREF = 3;
      };
      typedef int Network1xPrivHysteresisTimerType;
      namespace Network1xRLP
      {
         const int MAX_NAK_ROUNDS = 3;
      };
      struct Network1xRLPOptionType {
         unsigned char rscbIndex;
         unsigned char nakRoundsFwd;
         unsigned char naksPerRoundFwd[3];
         unsigned char nakRoundsRev;
         unsigned char naksPerRoundRev[3];
      };
      typedef int Network1xDoSFlagsType;
      namespace Network1xDoSFlags
      {
         const ::ds::Net::Network1xDoSFlagsType QDS_MSG_EXPEDITE = 0x1;
         const ::ds::Net::Network1xDoSFlagsType QDS_MSG_FAST_EXPEDITE = 0x2;
      };
      const ::AEEIID AEEIID_INetwork1x = 0x106ce1c;
      
      /** @interface INetwork1x
        * 
        * ds Net Network 1x interface.
        */
      struct INetwork1x : public ::IQI
      {
         
         /**
           * This function is used to query if DoS (Data Over Signaling) is
           * supported. The flags parameter is used to query support for
           * specific DoS features (same features used in Socket SendTo
           * operations).
           * DoS and SDB (Short Data Burst) are parallel terms.
           * DoS is the term used in 1xEVDO
           * SDB is the term used in CDMA 1x.
           * @param flags DoS flags to be used.
           * @param dosSupported TRUE if DoS is supported.
           *                     FALSE if DoS is not supported.
           * @retval AEE_SUCCESS Registration succeeded.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL QueryDoSSupport(::ds::Net::Network1xDoSFlagsType flags, boolean* dosSupported) = 0;
         
         /**
           * This attribute indicates the medium data rate value.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetMDR(int* value) = 0;
         
         /**
           * This attribute indicates the medium data rate value.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetMDR(int value) = 0;
         
         /**
           * This attribute indicates the QoS NA priority.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetQoSNAPriority(int* value) = 0;
         
         /**
           * This attribute indicates the QoS NA priority.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetQoSNAPriority(int value) = 0;
         
         /**
           * This attribute indicates the 1x RLP current NAK policy information.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetRLPAllCurrentNAK(::ds::Net::Network1xRLPOptionType* value) = 0;
         
         /**
           * This attribute indicates the 1x RLP current NAK policy information.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetRLPAllCurrentNAK(const ::ds::Net::Network1xRLPOptionType* value) = 0;
         
         /**
           * This attribute indicates the 1x RLP default NAK policy information.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetRLPDefCurrentNAK(::ds::Net::Network1xRLPOptionType* value) = 0;
         
         /**
           * This attribute indicates the 1x RLP default NAK policy information.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetRLPDefCurrentNAK(const ::ds::Net::Network1xRLPOptionType* value) = 0;
         
         /**
           * This attribute indicates the 1x RLP negotiated NAK policy information.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetRLPNegCurrentNAK(::ds::Net::Network1xRLPOptionType* value) = 0;
         
         /**
           * This attribute indicates the hysteresis act timer.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetHysteresisTimer(int* value) = 0;
         
         /**
           * This attribute indicates the hysteresis act timer.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetHysteresisTimer(int value) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_INETWORK1X_H
