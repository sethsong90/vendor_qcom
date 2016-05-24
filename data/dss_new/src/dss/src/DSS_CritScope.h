#ifndef __DSS_CRITSCOPE_H__
#define __DSS_CRITSCOPE_H__

/*===================================================

FILE:  DSS_CritScope.h

SERVICES:
   A utility class to automatically lock and unlock
   an entire scope.

=====================================================

Copyright (c) 2008 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_CritScope.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-13 en  History added.

===========================================================================*/

#include "AEEICritSect.h"

// This class provides an abstraction over DSS_CritSect.
// Defining an instance of this class in the top of the scope will enter the
// critical section. The critical section is exited automatically when the scope
// ends, due to the stack semantics that call the object's destructor.
class DSSCritScope {
public:
   DSSCritScope(ICritSect& cs);
   ~DSSCritScope();

private:
   ICritSect* pCritSect;
};

inline DSSCritScope::DSSCritScope(ICritSect& cs)
{
   pCritSect = &cs;
   pCritSect->Enter();
}

/*lint -e{1551} */
inline DSSCritScope::~DSSCritScope()
{
   pCritSect->Leave();
}
/*lint –restore */

#endif // __DSS_CRITSCOPE_H__
