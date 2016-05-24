#ifndef DS_SOCK_IICMPERRINFOPRIV_H
#define DS_SOCK_IICMPERRINFOPRIV_H

/*============================================================================
  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Sock_Def.h"
#include "AEEIQI.h"
#include "ds_Sock_IAncDataPriv.h"
#define ds_Sock_AEEIID_IICMPErrInfoPriv 0x106c948

/** @interface ds_Sock_IICMPErrInfoPriv
  * 
  * ds Socket ICMP Error Info interface.
  */
#define INHERIT_ds_Sock_IICMPErrInfoPriv(iname) \
   INHERIT_ds_Sock_IAncDataPriv(iname); \
   AEEResult (*GetExtendedErr)(iname* _pif, ds_Sock_ExtendedErrType* value); \
   AEEResult (*GetAddr)(iname* _pif, ds_SockAddrStorageType value)
AEEINTERFACE_DEFINE(ds_Sock_IICMPErrInfoPriv);

/** @memberof ds_Sock_IICMPErrInfoPriv
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Sock_IICMPErrInfoPriv_AddRef(ds_Sock_IICMPErrInfoPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Sock_IICMPErrInfoPriv)->AddRef(_pif);
}

/** @memberof ds_Sock_IICMPErrInfoPriv
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Sock_IICMPErrInfoPriv_Release(ds_Sock_IICMPErrInfoPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Sock_IICMPErrInfoPriv)->Release(_pif);
}

/** @memberof ds_Sock_IICMPErrInfoPriv
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Sock_IICMPErrInfoPriv_QueryInterface(ds_Sock_IICMPErrInfoPriv* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Sock_IICMPErrInfoPriv)->QueryInterface(_pif, iqi_iid, iqi);
}
static __inline AEEResult ds_Sock_IICMPErrInfoPriv_GetAncID(ds_Sock_IICMPErrInfoPriv* _pif, ds_Sock_AncDataIDType* value)
{
   return AEEGETPVTBL(_pif, ds_Sock_IICMPErrInfoPriv)->GetAncID(_pif, value);
}
static __inline AEEResult ds_Sock_IICMPErrInfoPriv_SetAncID(ds_Sock_IICMPErrInfoPriv* _pif, ds_Sock_AncDataIDType value)
{
   return AEEGETPVTBL(_pif, ds_Sock_IICMPErrInfoPriv)->SetAncID(_pif, value);
}

/** @memberof ds_Sock_IICMPErrInfoPriv
  * 
  * Error information.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Sock_IICMPErrInfoPriv_GetExtendedErr(ds_Sock_IICMPErrInfoPriv* _pif, ds_Sock_ExtendedErrType* value)
{
   return AEEGETPVTBL(_pif, ds_Sock_IICMPErrInfoPriv)->GetExtendedErr(_pif, value);
}

/** @memberof ds_Sock_IICMPErrInfoPriv
  * 
  * Address of the node that sent this ICMP error.
  * @param _pif Pointer to interface
  * @param value Attribute value
  */
static __inline AEEResult ds_Sock_IICMPErrInfoPriv_GetAddr(ds_Sock_IICMPErrInfoPriv* _pif, ds_SockAddrStorageType value)
{
   return AEEGETPVTBL(_pif, ds_Sock_IICMPErrInfoPriv)->GetAddr(_pif, value);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Sock_Def.h"
#include "AEEIQI.h"
#include "ds_Sock_IAncDataPriv.h"
namespace ds
{
   namespace Sock
   {
      const ::AEEIID AEEIID_IICMPErrInfoPriv = 0x106c948;
      
      /** @interface IICMPErrInfoPriv
        * 
        * ds Socket ICMP Error Info interface.
        */
      struct IICMPErrInfoPriv : public ::ds::Sock::IAncDataPriv
      {
         
         /**
           * Error information.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetExtendedErr(::ds::Sock::ExtendedErrType* value) = 0;
         
         /**
           * Address of the node that sent this ICMP error.
           * @param value Attribute value
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetAddr(::ds::SockAddrStorageType value) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_SOCK_IICMPERRINFOPRIV_H
