#ifndef __DSS_WEAK_REF_SCOPE_H__
#define __DSS_WEAK_REF_SCOPE_H__

/*===================================================

FILE:  DSS_WeakRefScope.h

SERVICES:
   A utility class to automatically GetSrongRef and 
   Release an entire scope.

=====================================================

Copyright (c) 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_WeakRefScope.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-12-09 aa  History added.

===========================================================================*/

#include "ds_Utils_IWeakRef.h"
#include "DSS_Common.h"

using ds::Utils::IWeakRef;

// This class provides an abstraction over WeakRef classes.
// Defining an instance of this class in the top of the scope will try to
// get strong ref. The strong ref is released automatically when the scope
// ends, due to the stack semantics that call the object's destructor.
class DSSWeakRefScope {
public:
   DSSWeakRefScope() : mStrongRef(NULL) {}
   boolean Init(IWeakRef* pWeakRef);
   ~DSSWeakRefScope();

private:
   IWeakRef* mStrongRef;
};

inline boolean DSSWeakRefScope::Init(IWeakRef* pWeakRef)
{
   mStrongRef = pWeakRef;
   boolean res = pWeakRef->GetStrongRef();
   if(!res) {
      LOG_MSG_ERROR("GetStrongRef() failed on obj 0x%p" ,mStrongRef, 0, 0);
      mStrongRef = NULL;
   }

   return res;
}

/*lint -e{1551} */
inline DSSWeakRefScope::~DSSWeakRefScope()
{
   DSSCommon::ReleaseIf((IQI**)&mStrongRef);
}
/*lint –restore */

#endif // __DSS_WEAK_REF_SCOPE_H__

