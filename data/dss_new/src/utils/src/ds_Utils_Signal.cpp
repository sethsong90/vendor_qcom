/*==========================================================================*/
/*!
  @file
  ds_Utils_Signal.cpp

  @brief
  This file provides implementation for ds::Utils::Signal class.

  @see
  ds_Utils_Signal.h
  AEEISignal.h

  Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*=========================================================================*/


/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are signaled in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ds_Utils_Signal.cpp#1 $
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
#include "AEEStdErr.h"
#include "ds_Utils_Signal.h"
#include "ds_Utils_SignalBus.h"
#include "ds_sig_svc.h"

using namespace ds::Utils;
using namespace ds::Error;

/*!
  @brief
  Signal object state masks.

  Signal can be in one or more of the following states.
  SIG_ENABLED    - Ready to receive notifications
  SIG_SET        - Signal has been set.
  SIG_ATTACHED   - This signal is attached to a SignalCtl.
  SIG_ASSOCIATED - This signal is/was associated with a SignalBus.
*/
#define SIG_ENABLED    1
#define SIG_SET        2
#define SIG_ATTACHED   4
#define SIG_ASSOCIATED 8

/*===========================================================================

                       CONSTRUCTOR/DESTRUCTOR

===========================================================================*/
Signal::Signal
(
  ISignalHandler *piSignalHandler,
  uint32          uArgA,
  uint32          uArgB
)
throw()
: mpiSignalHandler (piSignalHandler),
  muArgA (uArgA),
  muArgB (uArgB),
  mMagic (DS_UTILS_SIGNAL_MAGIC_NUMBER),
  mpSignalBus (NULL),
  mSigState (SIG_ENABLED | SIG_ATTACHED),
  refCnt (1),
  weakRefCnt(1)
{
/*-------------------------------------------------------------------------*/

  LOG_MSG_INFO1 ("Obj 0x%p", this, 0, 0);
  (void) mpiSignalHandler->AddRef(); /*lint !e1550 !e1551 */

} /* Signal() */

void Signal::Destructor
(
  void 
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Obj 0x%p", this, 0, 0);

  Detach(); // to release the SignalHandler, if the SignalCtl hasn't done it
            // beforehand.

} /* Signal::Destructor() */

Signal::~Signal
(
  void
)
throw()
{
/*-------------------------------------------------------------------------*/

  LOG_MSG_INFO1 ("Obj 0x%p", this, 0, 0);

} /* ~Signal() */


/*===========================================================================

                       PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
void Signal::CheckAndSchedule
(
  void
)
throw()
{
  uint8 umask = SIG_ENABLED | SIG_SET | SIG_ATTACHED;
  if ((umask & mSigState) == umask)
  {
    mSigState &= ~(SIG_ENABLED | SIG_SET);

    /* Hold on to self and to the SH until the callback completes. */
    (void) AddRef();
    (void) mpiSignalHandler->AddRef(); /*lint !e1550 !e1551 */

    /*-----------------------------------------------------------------------
      CheckAndSchedule() may be called from
      1. APP context (For ISignal_Set() and ISignalCtl_Set())
         In this case, post a command to perform notification later.
      2. DS_SIG context (For ISignalBus_Set())
         In this case, no task-switch is necessary.
    -----------------------------------------------------------------------*/
    if (ds_sig_is_current_task())
    {
      Callback();
      (void) Release();
    }
    else
    {
      ds_sig_send_cmd (DS_SIG_SIGNAL_DISPATCH_CMD, (void *) this);
    }
  }
}

ds::ErrorType CDECL Signal::Set
(
  void
)
throw()
{
  LOG_MSG_INFO1 ("Set() signal 0x%p, state %d", this, mSigState, 0);

  if (!IsValid())
  {
    return QDS_EINVAL;
  }

  mSigState |= SIG_SET;
  CheckAndSchedule();

  return AEE_SUCCESS;

} /* Set() */

ds::ErrorType Signal::SignalCtlSet
(
  void
)
throw()
{
  LOG_MSG_INFO1 ("Set() signal 0x%p, state %d", this, mSigState, 0);

  mSigState |= SIG_SET | SIG_ENABLED;
  CheckAndSchedule();

  return AEE_SUCCESS;

} /* SignalCtlSet() */

void Signal::Enable
(
  void
)
throw()
{
  // If wasn't enabled, and is functional (attached), invoke SignalBus callback
  bool stateChanged = !(mSigState & SIG_ENABLED) && (mSigState & SIG_ATTACHED);
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_INFO2 ("Enable() signal 0x%p, state %d", this, mSigState, 0);

  mSigState |= SIG_ENABLED;
  CheckAndSchedule();

  if ((NULL != mpSignalBus) && (true == stateChanged))
  {
    if(TRUE == mpSignalBus->GetStrongRef())
    {
      mpSignalBus->SignalEnabled(this);
      
      // we don't want to hold a strong reference
      mpSignalBus->Release();
    }
  }

} /* Enable() */

ds::ErrorType Signal::AssociateSignalBus
(
  SignalBus *pSignalBus
)
throw()
{
  LOG_MSG_INFO3 ("Sig %p, SB %p", this, mSigState, pSignalBus);

  // in case signal is detached
  if (0 == (mSigState & SIG_ATTACHED))
  {
    return QDS_EINVAL;
  }
  
  // in case the signal is already part of a bus
  if(SIG_ASSOCIATED == (mSigState & SIG_ASSOCIATED))
  {
    return QDS_EINUSE;
  }
  
  mpSignalBus = pSignalBus;
  
  mpSignalBus->AddRefWeak();

  mSigState |= SIG_ASSOCIATED;

  return AEE_SUCCESS;
}

void Signal::DisconnectSignalBus
(
  void
)
throw()
{
  if(NULL != mpSignalBus)
  {
    mpSignalBus->ReleaseWeak();

    mpSignalBus = NULL;
  }

  // The signal retains the associated state to prevent reuse
}

void Signal::Detach
(
  void
)
throw()
{
  LOG_MSG_INFO2 ("Detach() signal 0x%p, state %d", this, mSigState, 0);

  if (SIG_ATTACHED == (mSigState & SIG_ATTACHED)) // can only detach once
  {
    mSigState &= ~SIG_ATTACHED;

    if (mpSignalBus != NULL)
    {
      if(TRUE == mpSignalBus->GetStrongRef())
      {
        (void) mpSignalBus->Remove(this);
        
        // we don't want to hold a strong reference to SinalBus
        mpSignalBus->Release();

        DisconnectSignalBus();
      }
    }

    (void)mpiSignalHandler->Release(); /*lint !e1550 !e1551 */
  }
} /* Detach() */

boolean Signal::Process
(
  void* argUserDataPtr
)
throw()
{
  SignalProcessEnumType     cmd;
  boolean                   result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == argUserDataPtr)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return FALSE;
  }

  cmd = *((SignalProcessEnumType *)argUserDataPtr);
  LOG_MSG_INFO3 ("Process called for signal ctl 0x%p cmd %d", this, cmd, 0);

  switch (cmd)
  {
    case SIGNAL_PROCESS_ENUM_SET:
      (void) Set();
      result = TRUE;
      break;

    case SIGNAL_PROCESS_ENUM_DISCONNECT_SIGNAL_BUS:
      DisconnectSignalBus();
      result = TRUE;
      break;

    default:
      ASSERT (0);
      result = FALSE;
      break;
  }

  return result;

} /* Process */

void Signal::SignalDispatcher
(
  ds_sig_cmd_enum_type    cmd,
  void                   *pUserData
)
{
  ds::Utils::Signal      *pSignal;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (DS_SIG_SIGNAL_DISPATCH_CMD != cmd)
  {
    LOG_MSG_ERROR ("Inv. cmd %d", cmd, 0, 0);
    ASSERT (0);
    return;
  }

  if (NULL == pUserData)
  {
    LOG_MSG_ERROR ("NULL user data", 0, 0, 0);
    ASSERT (0);
    return;
  }

  pSignal = reinterpret_cast<ds::Utils::Signal *> (pUserData);
  if (!pSignal->IsValid())
  {
    return;
  }

  pSignal->Callback();
  (void) pSignal->Release(); // Release the ref obtained in Schedule()

} /* SignalDispatcher() */

/* Note that this function doesn't check the signal's Attached status. When
the signal is detached, Set() and Enable() should stop scheduling callbacks,
but any callbacks scheduled before that have to be completed. That's why
we take SH references in Schedule(). */
void Signal::Callback
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Callback() signal 0x%p", this, 0, 0);

  if (AEE_SUCCESS == mpiSignalHandler->Notify(muArgA, muArgB)) /*lint !e1550 !e1551 */
  {
     Enable();
  }
  // Release the ref obtained in CheckAndSchedule()
  (void) mpiSignalHandler->Release(); /*lint !e1550 !e1551 */

} /* Callback() */

Signal *Signal::CreateInstance
(
  ISignalHandler *piSignalHandler,
  uint32          uArgA,
  uint32          uArgB
)
throw()
{
   return new Signal(piSignalHandler, uArgA, uArgB);
}

bool Signal::IsEnabled
(
  void
)
throw()
{
   // There is no point to say a signal is enabled if it is already detached,
   // as it cannot be used anyway
   uint8 flags = SIG_ATTACHED | SIG_ENABLED;
   return (flags == (mSigState & flags));
}
