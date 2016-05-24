#ifndef DS_NET_IQOS1X_H
#define DS_NET_IQOS1X_H

/*============================================================================
  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "AEEIQI.h"
#define ds_Net_AEEIID_IQoS1x 0x106d74b

/**
  * This struct defines the type for RMAC3 information.
  */
struct ds_Net_IQoS1x_RMAC3InfoType {
   int headroomPayloadSize; /**< PA headroom limited payload size in bytes. */
   int bucketLevelPayloadSize; /**< BucketLevel equivalent payload size in bytes. */
   int t2pInflowPayloadSize; /**< T2PInflow equivalent payload size in bytes. */
};
typedef struct ds_Net_IQoS1x_RMAC3InfoType ds_Net_IQoS1x_RMAC3InfoType;

/** @interface ds_Net_IQoS1x
  * 
  * ds Net 1x QoS interface.
  * This interface provides 1x-specific operations on a QoS link.
  * This Object is received from IQoSSecondary/IQoSDefault via
  * GetTechObject.
  */
#define INHERIT_ds_Net_IQoS1x(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*GetRMAC3Info)(iname* _pif, ds_Net_IQoS1x_RMAC3InfoType* value); \
   AEEResult (*GetTXStatus)(iname* _pif, boolean* value); \
   AEEResult (*GetInactivityTimer)(iname* _pif, int* value); \
   AEEResult (*SetInactivityTimer)(iname* _pif, int value)
AEEINTERFACE_DEFINE(ds_Net_IQoS1x);

/** @memberof ds_Net_IQoS1x
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IQoS1x_AddRef(ds_Net_IQoS1x* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoS1x)->AddRef(_pif);
}

/** @memberof ds_Net_IQoS1x
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IQoS1x_Release(ds_Net_IQoS1x* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoS1x)->Release(_pif);
}

/** @memberof ds_Net_IQoS1x
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IQoS1x_QueryInterface(ds_Net_IQoS1x* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoS1x)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IQoS1x
  * 
  * This attribute represents the MAC layer information for the QoS flow.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IQoS1x_GetRMAC3Info(ds_Net_IQoS1x* _pif, ds_Net_IQoS1x_RMAC3InfoType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoS1x)->GetRMAC3Info(_pif, value);
}

/** @memberof ds_Net_IQoS1x
  * 
  * This attribute represents the status of the data transmission for the QoS flow.
  * If there is an error in transmission of data because of the two reasons listed below,
  * then the attribute will be FALSE, otherwise it will be TRUE.
  * Reasons for transmission error:
  *  - Physical Layer Transmission error (M-ARQ)
  *  - Data has become stale (by remaining in transmit watermark for too long) and thus
  *    discarded before a transmission is even attempted.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IQoS1x_GetTXStatus(ds_Net_IQoS1x* _pif, boolean* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoS1x)->GetTXStatus(_pif, value);
}

/** @memberof ds_Net_IQoS1x
  * 
  * This attribute represents the value of the inactivity timer of the associated QoS instance.
  * If data is not transmitted on this QoS instance for the specified inactivity timer value,
  * the traffic channel is released in order to conserve resources.
  * Although the attribute's name implies that there is a timer per every QoS instance, there
  * is only a timer per traffic channel. Since more than one QoS instance can be multiplexed
  * on to the same traffic channel, the largest value of all inactivity timer values is used
  * to start the inactivity timer.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IQoS1x_GetInactivityTimer(ds_Net_IQoS1x* _pif, int* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoS1x)->GetInactivityTimer(_pif, value);
}

/** @memberof ds_Net_IQoS1x
  * 
  * This attribute represents the value of the inactivity timer of the associated QoS instance.
  * If data is not transmitted on this QoS instance for the specified inactivity timer value,
  * the traffic channel is released in order to conserve resources.
  * Although the attribute's name implies that there is a timer per every QoS instance, there
  * is only a timer per traffic channel. Since more than one QoS instance can be multiplexed
  * on to the same traffic channel, the largest value of all inactivity timer values is used
  * to start the inactivity timer.          
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Net_IQoS1x_SetInactivityTimer(ds_Net_IQoS1x* _pif, int value)
{
   return AEEGETPVTBL(_pif, ds_Net_IQoS1x)->SetInactivityTimer(_pif, value);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
namespace ds
{
   namespace Net
   {
      const ::AEEIID AEEIID_IQoS1x = 0x106d74b;
      
      /** @interface IQoS1x
        * 
        * ds Net 1x QoS interface.
        * This interface provides 1x-specific operations on a QoS link.
        * This Object is received from IQoSSecondary/IQoSDefault via
        * GetTechObject.
        */
      struct IQoS1x : public ::IQI
      {
         
         /**
           * This struct defines the type for RMAC3 information.
           */
         struct RMAC3InfoType {
            int headroomPayloadSize; /**< PA headroom limited payload size in bytes. */
            int bucketLevelPayloadSize; /**< BucketLevel equivalent payload size in bytes. */
            int t2pInflowPayloadSize; /**< T2PInflow equivalent payload size in bytes. */
         };
         
         /**
           * This attribute represents the MAC layer information for the QoS flow.          
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetRMAC3Info(RMAC3InfoType* value) = 0;
         
         /**
           * This attribute represents the status of the data transmission for the QoS flow.
           * If there is an error in transmission of data because of the two reasons listed below,
           * then the attribute will be FALSE, otherwise it will be TRUE.
           * Reasons for transmission error:
           *  - Physical Layer Transmission error (M-ARQ)
           *  - Data has become stale (by remaining in transmit watermark for too long) and thus
           *    discarded before a transmission is even attempted.          
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetTXStatus(boolean* value) = 0;
         
         /**
           * This attribute represents the value of the inactivity timer of the associated QoS instance.
           * If data is not transmitted on this QoS instance for the specified inactivity timer value,
           * the traffic channel is released in order to conserve resources.
           * Although the attribute's name implies that there is a timer per every QoS instance, there
           * is only a timer per traffic channel. Since more than one QoS instance can be multiplexed
           * on to the same traffic channel, the largest value of all inactivity timer values is used
           * to start the inactivity timer.          
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetInactivityTimer(int* value) = 0;
         
         /**
           * This attribute represents the value of the inactivity timer of the associated QoS instance.
           * If data is not transmitted on this QoS instance for the specified inactivity timer value,
           * the traffic channel is released in order to conserve resources.
           * Although the attribute's name implies that there is a timer per every QoS instance, there
           * is only a timer per traffic channel. Since more than one QoS instance can be multiplexed
           * on to the same traffic channel, the largest value of all inactivity timer values is used
           * to start the inactivity timer.          
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetInactivityTimer(int value) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IQOS1X_H
