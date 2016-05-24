#ifndef DS_SOCK_IRECVIFINFOPRIV_H
#define DS_SOCK_IRECVIFINFOPRIV_H

/*============================================================================
  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Sock_IAncDataPriv.h"
#define ds_Sock_AEEIID_IRecvIFInfoPriv 0x106e70e

/** @interface ds_Sock_IRecvIFInfoPriv
  * 
  * ds Socket Recv IF Info interface.
  * This interface is used to provide application with the network interface handle
  * on which a datagram was received via a call to IDSSock RecvMsg API.
  * This information is provided if the IP_RECVIF socket option is enabled.
  */
#define INHERIT_ds_Sock_IRecvIFInfoPriv(iname) \
   INHERIT_ds_Sock_IAncDataPriv(iname); \
   AEEResult (*GetRecvIFHandle)(iname* _pif, unsigned int* value)
AEEINTERFACE_DEFINE(ds_Sock_IRecvIFInfoPriv);

/** @memberof ds_Sock_IRecvIFInfoPriv
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Sock_IRecvIFInfoPriv_AddRef(ds_Sock_IRecvIFInfoPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Sock_IRecvIFInfoPriv)->AddRef(_pif);
}

/** @memberof ds_Sock_IRecvIFInfoPriv
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Sock_IRecvIFInfoPriv_Release(ds_Sock_IRecvIFInfoPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Sock_IRecvIFInfoPriv)->Release(_pif);
}

/** @memberof ds_Sock_IRecvIFInfoPriv
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Sock_IRecvIFInfoPriv_QueryInterface(ds_Sock_IRecvIFInfoPriv* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Sock_IRecvIFInfoPriv)->QueryInterface(_pif, iqi_iid, iqi);
}
static __inline AEEResult ds_Sock_IRecvIFInfoPriv_GetAncID(ds_Sock_IRecvIFInfoPriv* _pif, ds_Sock_AncDataIDType* value)
{
   return AEEGETPVTBL(_pif, ds_Sock_IRecvIFInfoPriv)->GetAncID(_pif, value);
}
static __inline AEEResult ds_Sock_IRecvIFInfoPriv_SetAncID(ds_Sock_IRecvIFInfoPriv* _pif, ds_Sock_AncDataIDType value)
{
   return AEEGETPVTBL(_pif, ds_Sock_IRecvIFInfoPriv)->SetAncID(_pif, value);
}
static __inline AEEResult ds_Sock_IRecvIFInfoPriv_GetRecvIFHandle(ds_Sock_IRecvIFInfoPriv* _pif, unsigned int* value)
{
   return AEEGETPVTBL(_pif, ds_Sock_IRecvIFInfoPriv)->GetRecvIFHandle(_pif, value);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Sock_IAncDataPriv.h"
namespace ds
{
   namespace Sock
   {
      const ::AEEIID AEEIID_IRecvIFInfoPriv = 0x106e70e;
      
      /** @interface IRecvIFInfoPriv
        * 
        * ds Socket Recv IF Info interface.
        * This interface is used to provide application with the network interface handle
        * on which a datagram was received via a call to IDSSock RecvMsg API.
        * This information is provided if the IP_RECVIF socket option is enabled.
        */
      struct IRecvIFInfoPriv : public ::ds::Sock::IAncDataPriv
      {
         virtual ::AEEResult AEEINTERFACE_CDECL GetRecvIFHandle(unsigned int* value) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_SOCK_IRECVIFINFOPRIV_H
