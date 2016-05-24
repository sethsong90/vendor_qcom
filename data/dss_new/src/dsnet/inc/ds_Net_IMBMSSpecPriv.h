#ifndef DS_NET_IMBMSSPECPRIV_H
#define DS_NET_IMBMSSPECPRIV_H

/*============================================================================
  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "AEEIQI.h"
typedef int ds_Net_MBMSServiceType;
#define ds_Net_MBMSService_QDS_STREAMING 1
#define ds_Net_MBMSService_QDS_DOWNLOAD 2
typedef int ds_Net_MBMSServiceMethodType;
#define ds_Net_MBMSServiceMethod_QDS_BROADCAST 1
#define ds_Net_MBMSServiceMethod_QDS_MULTICAST 2
#define ds_Net_AEEIID_IMBMSSpecPriv 0x106cf97

/** @interface ds_Net_IMBMSSpecPriv
  * 
  * MBMS Spec Info interface.
  */
#define INHERIT_ds_Net_IMBMSSpecPriv(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*GetTMGI)(iname* _pif, uint64* value); \
   AEEResult (*SetTMGI)(iname* _pif, AEEINTERFACE_ATPCS_PADARGS(value) uint64 value); \
   AEEResult (*GetSessionStartTime)(iname* _pif, uint64* value); \
   AEEResult (*SetSessionStartTime)(iname* _pif, AEEINTERFACE_ATPCS_PADARGS(value) uint64 value); \
   AEEResult (*GetSessionEndTime)(iname* _pif, uint64* value); \
   AEEResult (*SetSessionEndTime)(iname* _pif, AEEINTERFACE_ATPCS_PADARGS(value) uint64 value); \
   AEEResult (*GetPriority)(iname* _pif, unsigned short* value); \
   AEEResult (*SetPriority)(iname* _pif, unsigned short value); \
   AEEResult (*GetService)(iname* _pif, ds_Net_MBMSServiceType* value); \
   AEEResult (*SetService)(iname* _pif, ds_Net_MBMSServiceType value); \
   AEEResult (*GetServiceMethod)(iname* _pif, ds_Net_MBMSServiceMethodType* value); \
   AEEResult (*SetServiceMethod)(iname* _pif, ds_Net_MBMSServiceMethodType value); \
   AEEResult (*GetSelectedService)(iname* _pif, boolean* value); \
   AEEResult (*SetSelectedService)(iname* _pif, boolean value); \
   AEEResult (*GetServiceSecurity)(iname* _pif, boolean* value); \
   AEEResult (*SetServiceSecurity)(iname* _pif, boolean value)
AEEINTERFACE_DEFINE(ds_Net_IMBMSSpecPriv);

/** @memberof ds_Net_IMBMSSpecPriv
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IMBMSSpecPriv_AddRef(ds_Net_IMBMSSpecPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMBMSSpecPriv)->AddRef(_pif);
}

/** @memberof ds_Net_IMBMSSpecPriv
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IMBMSSpecPriv_Release(ds_Net_IMBMSSpecPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMBMSSpecPriv)->Release(_pif);
}

/** @memberof ds_Net_IMBMSSpecPriv
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IMBMSSpecPriv_QueryInterface(ds_Net_IMBMSSpecPriv* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IMBMSSpecPriv)->QueryInterface(_pif, iqi_iid, iqi);
}
static __inline AEEResult ds_Net_IMBMSSpecPriv_GetTMGI(ds_Net_IMBMSSpecPriv* _pif, uint64* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IMBMSSpecPriv)->GetTMGI(_pif, value);
}
static __inline AEEResult ds_Net_IMBMSSpecPriv_SetTMGI(ds_Net_IMBMSSpecPriv* _pif, uint64 value)
{
   return AEEGETPVTBL(_pif, ds_Net_IMBMSSpecPriv)->SetTMGI(_pif, AEEINTERFACE_ATPCS_PADPARAM value);
}
static __inline AEEResult ds_Net_IMBMSSpecPriv_GetSessionStartTime(ds_Net_IMBMSSpecPriv* _pif, uint64* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IMBMSSpecPriv)->GetSessionStartTime(_pif, value);
}
static __inline AEEResult ds_Net_IMBMSSpecPriv_SetSessionStartTime(ds_Net_IMBMSSpecPriv* _pif, uint64 value)
{
   return AEEGETPVTBL(_pif, ds_Net_IMBMSSpecPriv)->SetSessionStartTime(_pif, AEEINTERFACE_ATPCS_PADPARAM value);
}
static __inline AEEResult ds_Net_IMBMSSpecPriv_GetSessionEndTime(ds_Net_IMBMSSpecPriv* _pif, uint64* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IMBMSSpecPriv)->GetSessionEndTime(_pif, value);
}
static __inline AEEResult ds_Net_IMBMSSpecPriv_SetSessionEndTime(ds_Net_IMBMSSpecPriv* _pif, uint64 value)
{
   return AEEGETPVTBL(_pif, ds_Net_IMBMSSpecPriv)->SetSessionEndTime(_pif, AEEINTERFACE_ATPCS_PADPARAM value);
}
static __inline AEEResult ds_Net_IMBMSSpecPriv_GetPriority(ds_Net_IMBMSSpecPriv* _pif, unsigned short* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IMBMSSpecPriv)->GetPriority(_pif, value);
}
static __inline AEEResult ds_Net_IMBMSSpecPriv_SetPriority(ds_Net_IMBMSSpecPriv* _pif, unsigned short value)
{
   return AEEGETPVTBL(_pif, ds_Net_IMBMSSpecPriv)->SetPriority(_pif, value);
}
static __inline AEEResult ds_Net_IMBMSSpecPriv_GetService(ds_Net_IMBMSSpecPriv* _pif, ds_Net_MBMSServiceType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IMBMSSpecPriv)->GetService(_pif, value);
}
static __inline AEEResult ds_Net_IMBMSSpecPriv_SetService(ds_Net_IMBMSSpecPriv* _pif, ds_Net_MBMSServiceType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IMBMSSpecPriv)->SetService(_pif, value);
}
static __inline AEEResult ds_Net_IMBMSSpecPriv_GetServiceMethod(ds_Net_IMBMSSpecPriv* _pif, ds_Net_MBMSServiceMethodType* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IMBMSSpecPriv)->GetServiceMethod(_pif, value);
}
static __inline AEEResult ds_Net_IMBMSSpecPriv_SetServiceMethod(ds_Net_IMBMSSpecPriv* _pif, ds_Net_MBMSServiceMethodType value)
{
   return AEEGETPVTBL(_pif, ds_Net_IMBMSSpecPriv)->SetServiceMethod(_pif, value);
}
static __inline AEEResult ds_Net_IMBMSSpecPriv_GetSelectedService(ds_Net_IMBMSSpecPriv* _pif, boolean* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IMBMSSpecPriv)->GetSelectedService(_pif, value);
}
static __inline AEEResult ds_Net_IMBMSSpecPriv_SetSelectedService(ds_Net_IMBMSSpecPriv* _pif, boolean value)
{
   return AEEGETPVTBL(_pif, ds_Net_IMBMSSpecPriv)->SetSelectedService(_pif, value);
}
static __inline AEEResult ds_Net_IMBMSSpecPriv_GetServiceSecurity(ds_Net_IMBMSSpecPriv* _pif, boolean* value)
{
   return AEEGETPVTBL(_pif, ds_Net_IMBMSSpecPriv)->GetServiceSecurity(_pif, value);
}
static __inline AEEResult ds_Net_IMBMSSpecPriv_SetServiceSecurity(ds_Net_IMBMSSpecPriv* _pif, boolean value)
{
   return AEEGETPVTBL(_pif, ds_Net_IMBMSSpecPriv)->SetServiceSecurity(_pif, value);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
namespace ds
{
   namespace Net
   {
      typedef int MBMSServiceType;
      namespace MBMSService
      {
         const ::ds::Net::MBMSServiceType QDS_STREAMING = 1;
         const ::ds::Net::MBMSServiceType QDS_DOWNLOAD = 2;
      };
      typedef int MBMSServiceMethodType;
      namespace MBMSServiceMethod
      {
         const ::ds::Net::MBMSServiceMethodType QDS_BROADCAST = 1;
         const ::ds::Net::MBMSServiceMethodType QDS_MULTICAST = 2;
      };
      const ::AEEIID AEEIID_IMBMSSpecPriv = 0x106cf97;
      
      /** @interface IMBMSSpecPriv
        * 
        * MBMS Spec Info interface.
        */
      struct IMBMSSpecPriv : public ::IQI
      {
         virtual ::AEEResult AEEINTERFACE_CDECL GetTMGI(::uint64* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetTMGI(::uint64 value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetSessionStartTime(::uint64* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetSessionStartTime(::uint64 value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetSessionEndTime(::uint64* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetSessionEndTime(::uint64 value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetPriority(unsigned short* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetPriority(unsigned short value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetService(::ds::Net::MBMSServiceType* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetService(::ds::Net::MBMSServiceType value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetServiceMethod(::ds::Net::MBMSServiceMethodType* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetServiceMethod(::ds::Net::MBMSServiceMethodType value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetSelectedService(boolean* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetSelectedService(boolean value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL GetServiceSecurity(boolean* value) = 0;
         virtual ::AEEResult AEEINTERFACE_CDECL SetServiceSecurity(boolean value) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IMBMSSPECPRIV_H
