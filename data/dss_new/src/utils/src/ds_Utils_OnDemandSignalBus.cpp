
/*===========================================================================
  @file ds_Utils_OnDemandSignalBus.cpp

  Implementation of ds::Utils::OnDemandSignalBus class.

  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are signaled in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ds_Utils_OnDemandSignalBus.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-07-19 mt  Created module.

===========================================================================*/

/*---------------------------------------------------------------------------
  Include Files
---------------------------------------------------------------------------*/
#include "comdef.h"
#include "AEEStdErr.h"
#include "ds_Utils_CritScope.h"
#include "ds_Utils_CSSupport.h"
#include "ds_Utils_CreateInstance.h"
#include "ds_Utils_DebugMsg.h"
#include "ds_Utils_OnDemandSignalBus.h"
#include "AEECCritSect.h"
#include "AEECSignalBus.h"

using namespace ds::Utils;
using namespace ds::Error;

/*---------------------------------------------------------------------------
  QTF Support
---------------------------------------------------------------------------*/
#ifdef TEST_FRAMEWORK
#include <qtf_stub.h>

TF_DECLARE_STUB(int, DS_Utils_CreateInstance, void*, AEECLSID, void*, void**);
#define DS_Utils_CreateInstance TF_STUB(DS_Utils_CreateInstance)

#endif /* TEST_FRAMEWORK */

/*---------------------------------------------------------------------------
  Static variables
---------------------------------------------------------------------------*/
ICritSect *OnDemandSignalBus::critSectPtr = 0;

/*===========================================================================

                       PUBLIC FUNCTION DEFINITIONS

===========================================================================*/

OnDemandSignalBus::OnDemandSignalBus(
  void
):
  busPtr(0),
  isSet(false),
  onEnablePtr(0)
{
}; /* OnDemandSignalBus::OnDemandSignalBus() */

ds::ErrorType OnDemandSignalBus::Init()
{
  int ret = AEE_SUCCESS;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == critSectPtr)
  {
    ret = DS_Utils_CreateInstance (NULL, AEECLSID_CCritSect, NULL,
                                   (void**)&critSectPtr);
  }

  return ret;

}; /* OnDemandSignalBus::Init() */

OnDemandSignalBus::~OnDemandSignalBus(
  void
)
{
  Release();

}; /* OnDemandSignalBus::~OnDemandSignalBus() */

void OnDemandSignalBus::DeInit(
  void
)
{
  DS_UTILS_RELEASEIF(critSectPtr);

}; /* OnDemandSignalBus::DeInit() */

/*---------------------------------------------------------------------------
  SignalBus is reference-counted while this class is not, but the usage of
  Release() is conceptually similar: to signal the class that it will no
  longer be used by the "releaser", allowing it to free resources.

  In a SignalBus, the resources will be cleaned up and the class deallocated
  when the last reference is released. Here, the resources will be cleaned
  up when Release is called, but the class will not be deallocated (as it is
  a member of the user). In both cases, attempting to use an instance after
  Release() was called is a very bad idea.
---------------------------------------------------------------------------*/
void OnDemandSignalBus::Release(
  void
)
throw()
{
  DS_UTILS_RELEASEIF(busPtr);
  DS_UTILS_RELEASEIF(onEnablePtr);

}; /* OnDemandSignalBus::Release() */

ISignalBus* OnDemandSignalBus::GetBus(
  void
)
{
  CritScope cs(critSectPtr);
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (AEE_SUCCESS == CheckAndAllocateRealBus())
  {
    (void) busPtr->AddRef();
    return busPtr; /* Ref goes out, Release() is caller's responsibility */
  }
  return 0;

}; /* OnDemandSignalBus::GetBus() */

ds::ErrorType OnDemandSignalBus::Add
(
  ISignal* piSignal
)
{
  int ret;
  CritScope cs(critSectPtr);
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ret = CheckAndAllocateRealBus();

  if (AEE_SUCCESS == ret)
  {
    ret = busPtr->Add(piSignal);
  }
  return ret;

}; /* OnDemandSignalBus::Add() */

ds::ErrorType OnDemandSignalBus::Remove
(
  ISignal* piSignal
)
{
  CritScope cs(critSectPtr);
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == busPtr)
  {
    return AEE_EFAILED; /* This is not the bus you're looking for */
  }
  else
  {
    return busPtr->Remove(piSignal);
  }

}; /* OnDemandSignalBus::Remove() */

ds::ErrorType OnDemandSignalBus::Set
(
  void
)
{
  CritScope cs(critSectPtr);
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == busPtr)
  {
    isSet = true;
    return AEE_SUCCESS;
  }
  else
  {
    return busPtr->Set();
  }

}; /* OnDemandSignalBus::Set() */

ds::ErrorType OnDemandSignalBus::Clear
(
  void
)
{
  CritScope cs(critSectPtr);
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == busPtr)
  {
    isSet = false;
    return AEE_SUCCESS;
  }
  else
  {
    return busPtr->Clear();
  }

}; /* OnDemandSignalBus::Clear() */

ds::ErrorType OnDemandSignalBus::Strobe
(
  void
)
{
  CritScope cs(critSectPtr);
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == busPtr)
  {
    return AEE_SUCCESS;
  }
  else
  {
    return busPtr->Strobe();
  }

}; /* OnDemandSignalBus::Strobe() */

/*---------------------------------------------------------------------------
  Why do we cache the enable signal, introducing additional complexity,
  instead of forcing signal bus creation at this point?

  If setting/clearing the bus involves non-trivial logic, a client may decide
  to register an OnEnable signal before there are any listeners on the bus,
  and perform state maintenance on the bus only when there are active
  listeners.

  When it is likely there will be no listeners, but the client registers an
  OnEnable signal on its initialization, this optimization allows us
  to defer allocation of a SignalBus until a first listener is added, and
  to avoid using a SignalBus altogether if no listeners register during
  the client lifetime.
---------------------------------------------------------------------------*/
ds::ErrorType OnDemandSignalBus::OnEnable
(
  ISignal* piSignal
)
{
  CritScope cs(critSectPtr);
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == busPtr)
  {
    /*-----------------------------------------------------------------------
      Hold on to the signal without creating the bus, releasing
      the previously cached signal if there was one.
    -----------------------------------------------------------------------*/
    if (0 != onEnablePtr) {
      (void) onEnablePtr->Release();
    }

    onEnablePtr = piSignal;
    (void) onEnablePtr->AddRef();
    return AEE_SUCCESS;
  }
  else
  {
    return busPtr->OnEnable(piSignal);
  }

}; /* OnDemandSignalBus::OnEnable() */

/*===========================================================================

                       PRIVATE FUNCTION DEFINITIONS

===========================================================================*/

ds::ErrorType OnDemandSignalBus::CheckAndAllocateRealBus(
  void
)
{
  int ret = AEE_SUCCESS;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == busPtr)
  {
    ret = DS_Utils_CreateInstance (NULL, AEECLSID_CSignalBus, NULL,
                                   (void**)&busPtr);
    if (AEE_SUCCESS != ret)
    {
      LOG_MSG_INFO3( "Couldn't create SignalBus", 0, 0, 0);
    }
    else /* Real signal bus created */
    {
      /*-----------------------------------------------------------------------
        Register the cached OnEnable signal
      -----------------------------------------------------------------------*/
      if (0 != onEnablePtr)
      {
        (void) busPtr->OnEnable(onEnablePtr);
        (void) onEnablePtr->Release();
        onEnablePtr = 0;
      }

      /*-----------------------------------------------------------------------
        Reconcile bus state with the stored state
      -----------------------------------------------------------------------*/
      if (isSet)
      {
        ret = busPtr->Set();
      }

    } /* AEE_SUCCESS != CreateInstance(bus) */

  } /* 0 != busPtr */

  return ret;

}; /* OnDemandSignalBus::CheckAndAllocateRealBus() */

