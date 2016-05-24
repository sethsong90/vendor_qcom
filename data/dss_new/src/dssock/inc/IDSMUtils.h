// Copyright (c) 2006, 2007 Qualcomm Technologies, Inc.

#ifndef DS_IDSMUTIL_H
#define DS_IDSMUTIL_H

/*===========================================================================
@file DSMUtils.h

This file provides declarations for DSM utilities.
It is used only in DSS simulation environment. Target ds code
shall define a parallel header for DSM functions

Copyright (c) 2008 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

#include "AEEStdDef.h"

#include "AEEInterface.h"

//#include "comdef.h"
#include "dsm.h"

#if !defined(AEEINTERFACE_CPLUSPLUS)

#include "AEEIQI.h"

#define AEEIID_IDSMUtils 0x0107dd5f;

#define INHERIT_IDSMUtils(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*ReadDSMChain)(iname* _me,
                             dsm_item_type** item_ptr,        /* ptr to item chain containing read data */
                             int32* pnBytesRead);

   AEEResult (*WriteDSMChain)(iname* _me,
                              dsm_item_type** item_ptr,       /* ptr to item chain containing data to write */
                              int32* pnBytesWritten);

   AEEResult (*SendToDSMChain)(iname* _me,
                               dsm_item_type** item_ptr_ptr,
                               const ::ds::Sock::SockAddrStorageType remoteAddr,
                               unsigned int flags,
                               int32* pnBytesSent);

   AEEResult (*RecvFromDSMChain)(iname* _me,
                                 dsm_item_type** item_ptr_ptr,
                                 const ::ds::Sock::SockAddrStorageType remoteAddr,
                                 unsigned int flags,
                                 int32* pnBytesRecvd);

AEEINTERFACE_DEFINE(IDSMUtils);

static __inline unsigned int IDSMUtils_AddRef(IDSMUtils* _me)
{
   return AEEGETPVTBL(_me,IDSMUtils)->AddRef(_me);
}

static __inline unsigned int IDSMUtils_Release(IDSMUtils* _me)
{
   return AEEGETPVTBL(_me,IDSMUtils)->Release(_me);
}

static __inline int IDSMUtils_QueryInterface(IDSMUtils* _me, AEEIID iid, void** ppo)
{
   return AEEGETPVTBL(_me,IDSMUtils)->QueryInterface(_me, iid, ppo);
}

static __inline AEEResult IDSMUtils_ReadDSMChain(IDSMUtils* _me, dsm_item_type** item_ptr, int32* pnBytesRead)
{
   return AEEGETPVTBL(_me,IDSMUtils)->ReadDSMChain(_me, item_ptr, pnBytesRead);
}

static __inline AEEResult IDSMUtils_WriteDSMChain(IDSMUtils* _me, dsm_item_type** item_ptr, int32* pnBytesWritten)
{
   return AEEGETPVTBL(_me,IDSMUtils)->WriteDSMChain(_me, item_ptr, pnBytesWritten);
}

static __inline AEEResult IDSMUtils_SendToDSMChain(IDSMUtils* _me, dsm_item_type** item_ptr_ptr,
                                                   const ::ds::SockAddrStorageType remoteAddr,
                                                   unsigned int flags,
                                                   int32* pnBytesSent)
{
   return AEEGETPVTBL(_me,IDSMUtils)->SendToDSMChain(_me, item_ptr_ptr, remoteAddr, flags, pnBytesSent);
}

static __inline AEEResult IDSMUtils_RecvFromDSMChain(IDSMUtils* _me, dsm_item_type** item_ptr_ptr,
                                                     const ::ds::SockAddrStorageType remoteAddr,
                                                     unsigned int flags,
                                                     int32* pnBytesRecvd)
{
   return AEEGETPVTBL(_me,IDSMUtils)->RecvFromDSMChain(_me, item_ptr_ptr, remoteAddr, flags, pnBytesRecvd);
}

#else /* C++ */

#include "AEEIQI.h"
/*lint -esym(1510, IDSMUtils) */
namespace ds {
   const ::AEEIID AEEIID_IDSMUtils = 0x0107dd5f;

   class IDSMUtils : public IQI
   {
      public:
         virtual ::AEEResult CDECL ReadDSMChain(dsm_item_type** item_ptr, int32* pnBytesRead) = 0;
         virtual ::AEEResult CDECL WriteDSMChain(dsm_item_type** item_ptr, int32* pnBytesWritten) = 0;
         virtual ::AEEResult CDECL SendToDSMChain(dsm_item_type** item_ptr_ptr, const ::ds::SockAddrStorageType remoteAddr, unsigned int flags, int32* pnBytesSent) = 0;
         virtual ::AEEResult CDECL RecvFromDSMChain(dsm_item_type** item_ptr_ptr, ds::SockAddrStorageType remoteAddr, unsigned int flags, int32* pnBytesRecvd) = 0;
   };
}
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */

#endif /* #ifndef DS_IDSMUTIL_H */

