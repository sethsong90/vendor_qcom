/*!
  @file
  ds_Utils_SignalHandler.cpp

  @brief
  This file provides implementation for ds::Utils::SignalHandler class.

  @see
  ds_Utils_SignalHandler.h
  AEEISignalHandler.h

  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ds_Utils_SignalHandler.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-06-24  mt Modified to require and use IWeakRef.
  2009-11-16  mt Created the module.

===========================================================================*/

/*---------------------------------------------------------------------------
  Include Files
---------------------------------------------------------------------------*/
#include "comdef.h"

#include "ds_Utils_Atomic.h"
#include "ds_Utils_DebugMsg.h"
#include "AEEStdErr.h"
#include "ds_Errors_Def.h"
#include "ds_Utils_SignalHandler.h"

using namespace ds::Utils;
using namespace ds::Error;

/*===========================================================================

                       CONSTRUCTOR/DESTRUCTOR

===========================================================================*/
SignalHandler::SignalHandler()
throw()
: mpOwner(0), refCnt(1)
{
  LOG_MSG_INFO1 ("Obj 0x%p", this, 0, 0);

} /* SignalHandler() */

SignalHandler::~SignalHandler()
throw()
{
  LOG_MSG_INFO1 ("Obj 0x%p", this, 0, 0);

} /* ~SignalHandler() */

uint32 SignalHandler::Release()
throw()
{
  uint32 tmpRefCnt = ds_utils_atomic_Add(&refCnt, -1);
  LOG_MSG_INFO3("Release(), obj 0x%p, refCnt %d", this, tmpRefCnt, 0);
  if (tmpRefCnt == 0)
  {
    DS_UTILS_RELEASE_WEAKREF_IF(mpOwner); /* Reference taken in Init() */
  }
  return tmpRefCnt;
}

/*===========================================================================

                       PUBLIC FUNCTION DEFINITIONS

===========================================================================*/

int CDECL SignalHandler::Notify(uint32 uArgA, uint32 uArgB)
{
  LOG_MSG_INFO2("obj 0x%p, uArgA 0x%x, uArgB 0x%x", this, uArgA, uArgB);

  if ((mpOwner != NULL) && mpOwner->GetStrongRef()) {
    // Got the strong reference - can call the callback
    ((void (CDECL*)(void*))uArgA)((void*)uArgB);

    (void)mpOwner->Release();
  }

  return AEE_ENOMORE; /* Do not reenable the signal */
}

void SignalHandler::Init(IWeakRef *pOwner)
throw()
{
  mpOwner = pOwner;
  (void)mpOwner->AddRefWeak(); /* released when SignalHandler's refcnt goes
                                  down to 0 */
}
