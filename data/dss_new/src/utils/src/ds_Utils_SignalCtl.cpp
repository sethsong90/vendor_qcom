/*==========================================================================*/
/*!
  @file
  ds_Utils_SignalCtl.cpp

  @brief
  This file provides implementation for ds::Utils::SignalCtl class. 
  To simplify things all logic is in the Signal code, SignalCtl merely
  forwards the calls.

  @see
  ds_Utils_SignalCtl.h
  AEEISignalCtl.h

  Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*=========================================================================*/


/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are signaled in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ds_Utils_SignalCtl.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-05-02 hm  Created module.

===========================================================================*/

/*---------------------------------------------------------------------------
  Include Files
---------------------------------------------------------------------------*/
#include "comdef.h"
#include "ds_Utils_DebugMsg.h"
#include "ds_Utils_SignalCtl.h"
#include "AEEStdErr.h"

using namespace ds::Utils;
using namespace ds::Error;


/*===========================================================================

                       CONSTRUCTOR/DESTRUCTOR

===========================================================================*/
SignalCtl* SignalCtl::CreateInstance
(
  Signal *pSignal
)
throw()
{
  return new SignalCtl(pSignal);
} /* CreateInstance() */

SignalCtl::SignalCtl
(
  Signal* argSig
)
throw()
: sigPtr(argSig),
  refCnt(1)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("SigCtl obj 0x%p for Sig 0x%p", this, argSig, 0);

  (void) sigPtr->AddRef();

} /* SignalCtl() */


SignalCtl::~SignalCtl
(
  void 
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("obj 0x%p", this, 0, 0);

  if (NULL != sigPtr)
  {
    sigPtr->Detach();
    (void) sigPtr->Release();
    sigPtr = NULL;
  }

} /* ~SignalCtl() */


/*===========================================================================

                       PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
ds::ErrorType CDECL SignalCtl::Detach
(
  void
)
throw()
{
/*-------------------------------------------------------------------------*/

  LOG_MSG_INFO3 ("Detach signal ctl 0x%p", this, 0, 0);

  sigPtr->Detach();
  return AEE_SUCCESS;

} /* Detach() */

ds::ErrorType CDECL SignalCtl::Enable
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO3 ("Enable signal 0x%p", this, 0, 0);

  sigPtr->Enable();
  return AEE_SUCCESS;

} /* Enable() */

ds::ErrorType CDECL SignalCtl::Set
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO3 ("Set() signalCtl 0x%p", this, 0, 0);

  return sigPtr->SignalCtlSet();

} /* Set() */

