#ifndef __DSS_COMMON_H__
#define __DSS_COMMON_H__

/*===================================================

FILE:  DSS_Common.h

SERVICES:
   Common functions and macros for DSS_.

=====================================================

Copyright (c) 2008 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_Common.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-13 en  History added.

===========================================================================*/

#include "comdef.h"
#include "ds_Utils_DebugMsg.h"
#include "DSS_Conversion.h"

// Call the function f, which returns AEEResult.
// If it fails, convert the result to a ds error code, assign it to *dss_errno
// and return DSS_ERROR.
// dss_errno should be defined in the scope, and be of type sint15.
#define IDS_ERR_RET_ERRNO(f)                                \
   do {                                                     \
      AEEResult nRes = f;                                   \
      if (AEE_SUCCESS != nRes) {                            \
         *dss_errno = DSSConversion::IDS2DSErrorCode(nRes); \
         return DSS_ERROR;                                  \
      }                                                     \
   } while (0)

#define IDS_ERR_RET(f)                                      \
   do {                                                     \
      AEEResult nRes = f;                                   \
      if (AEE_SUCCESS != nRes) {                            \
         return nRes;                                       \
      }                                                     \
   } while (0)

// Call the function f, which returns AEEResult.
// If it fails, convert the result to a ds error code, assign it to *dss_errno,
// and goto bail to free any specific resources.
// dss_errno should be defined in the scope, and be of type sint15.
// nRet should be defined in the scope
// bail label is required in the function
#define BAIL_ERRNO(f)                                       \
   do {                                                     \
      AEEResult res = f;                                    \
      if (AEE_SUCCESS != res) {                      \
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);  \
         nRet = DSS_ERROR;                                  \
         goto bail;                                         \
      }                                                     \
   } while (0)


// Call the function f, which returns AEEResult.
// If it fails, goto bail to free any specific resources.
// nRet should be defined in the scope
// bail label is required in the function
#define BAIL_ERR(f)                                         \
   do {                                                     \
      nRet = f;                                             \
      if (AEE_SUCCESS != nRet) {                     \
         goto bail;                                         \
      }                                                     \
   } while (0)


namespace DSSCommon
{
   inline void ReleaseIf(IQI** iqi) throw()
   {
      if (0 != *iqi) {
         // The cast to void is done because we do not
         // need to check the return value which is 
         // the decremented reference count for the object.
         // The cast is done to avoid lint error.
         (void)(*iqi)->Release();
         *iqi = NULL;
      }
   }
}

#endif // __DSS_COMMON_H__
