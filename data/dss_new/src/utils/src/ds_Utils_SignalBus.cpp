/*===========================================================================
  FILE: DSSignalBus.cpp

  OVERVIEW: This file provides implementation for the ds::Utils::SignalBus
  class.

  DEPENDENCIES: None

  Copyright (c) 2007-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/


/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are signaled in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ds_Utils_SignalBus.cpp#1 $
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
#include "ds_Utils_SignalBus.h"
#include "ds_Utils_CSSupport.h"

using namespace ds::Utils;
using namespace ds::Error;


/*===========================================================================

                       PUBLIC FUNCTION DEFINITIONS

===========================================================================*/

SignalBus::SignalBus
(
  void
)
: mpOnEnableSig(NULL), isSet(false), refCnt(1), weakRefCnt(1)
{
  LOG_MSG_INFO1 ("obj 0x%p", this, 0, 0);

} /* SignalBus() */

SignalBus::~SignalBus
(
  void
)
throw()
{
  LOG_MSG_INFO1 ("~SignalBus obj 0x%p", this, 0, 0);
} /* ~SignalBus() */

void SignalBus::Destructor
(
  void
)
throw()
{
  LOG_MSG_INFO1 ("SignalBus::Destructor obj 0x%p", this, 0, 0);

  int userData = (int)SIGNAL_PROCESS_ENUM_DISCONNECT_SIGNAL_BUS;
  (void) Factory::Traverse ((void *) &userData);

  DS_UTILS_RELEASEIF(mpOnEnableSig);

}
/* Implementation notes for SignalBus::Strobe()

Should Strobe change the state of the bus?

Generally, the bus should function in either Set/Clear (persistent) or Strobe
(momentary) mode and there shouldn't be a case where mixing is necessary;
furthermore, calling Strobe when the bus is already Set has no effect (all
signals on the bus were already Set anyway). It seems preferable to have
Clear() being the only function that can clear the bus state. */
ds::ErrorType CDECL SignalBus::Strobe
(
  void
)
throw()
{
  LOG_MSG_INFO2 ("obj 0x%p", this, 0, 0);

  (void) AddRef(); // to prevent SignalBus destruction until asynchronous
                   // traversing of the list of signals completes.

  ds_sig_send_cmd (DS_SIG_SIGNAL_BUS_SET_CMD, (void *) this);
  return AEE_SUCCESS;
}

ds::ErrorType CDECL SignalBus::Set
(
  void
)
throw()
{
  LOG_MSG_INFO1 ("Obj 0x%p", this, 0, 0);

  if (!isSet)
  {
    isSet = true;
    return Strobe();
  }
  else
  {
    return AEE_SUCCESS;
  }
}

ds::ErrorType CDECL SignalBus::Clear
(
  void
)
throw()
{
  LOG_MSG_INFO1 ("obj 0x%p", this, 0, 0);

  isSet = false;

  return AEE_SUCCESS;
}

ds::ErrorType CDECL SignalBus::OnEnable
(
  ISignal* piSignal
)
throw()
{
/*-------------------------------------------------------------------------*/

  LOG_MSG_INFO1 ("OnEnable() on SignalBus 0x%p", this, 0, 0);

  if (NULL == piSignal)
  {
    LOG_MSG_ERROR ("NULL arg", 0, 0, 0);
    return QDS_EFAULT;
  }

  if (NULL != mpOnEnableSig)
  {
    LOG_MSG_ERROR ("ISignalPtr already set for bus 0x%p", this, 0, 0);
    return QDS_EINVAL;
  }

  mpOnEnableSig = reinterpret_cast <Signal *> (piSignal);
  (void) mpOnEnableSig->AddRef();

  return AEE_SUCCESS;
}

// Called by a Signal associated to a SignalBus when the signal enters
// Enabled state.
void CDECL SignalBus::SignalEnabled(Signal *piSignal) throw()
{
  if (NULL != mpOnEnableSig)
  {
    (void) mpOnEnableSig->Set(); /*lint !e1550 !e1551 */
  }

  if (isSet)
  {
    (void) piSignal->Set(); /*lint !e1550 !e1551 */
  }
}

ds::ErrorType CDECL SignalBus::Add
(
  ISignal* piSignal
)
throw()
{
  int32    result;
  Signal * pSig = reinterpret_cast <Signal *> (piSignal);
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2 ("Sig bus 0x%p, signal 0x%p", this, piSignal, 0);

  result = ValidateSignal (pSig);
  if (AEE_SUCCESS != result)
  {
    LOG_MSG_ERROR("Signal failed validation, not added", 0, 0, 0);
    return result;
  }
  
  result = pSig->AssociateSignalBus(this);
  if (AEE_SUCCESS != result)
  {
    LOG_MSG_ERROR ("Cant associate sig 0x%p with bus, error is 0x%x", piSignal, result, 0);
    return result;
  }

  result = AddItem (pSig);
  if (AEE_SUCCESS != result)
  {
    pSig->DisconnectSignalBus();
    LOG_MSG_ERROR ("Cant add sig 0x%p to bus", piSignal, 0, 0);
    return AEE_ENOMEMORY;
  }

  // Can't use SignalEnabled because of the additional condition here
  if ((pSig->IsEnabled()) && (NULL != mpOnEnableSig))
  {
    (void) mpOnEnableSig->Set(); /*lint !e1550 !e1551 */
  }

  if (isSet)
  {
    (void) piSignal->Set(); /*lint !e1550 !e1551 */
  }

  return AEE_SUCCESS;
}

ds::ErrorType SignalBus::ValidateSignal
(
  ISignal *     pISignal
)
throw()
{
  Signal *      pSignal;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Verify signal validity: it should be a DS Signal implementation.
    SignalCtl is not supported.
  -------------------------------------------------------------------------*/
  if (NULL == pISignal)
  {
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Directly calling IsValid() on any passed value can result in executing
    garbage code. Ask the Creator if this signal object is valid.

    The ISignal pointer passed here should be the memory that was originally
    allocated by PS mem. If it were cast to some other base class, thus
    changing the pointer value, this would break. And that is correct
    behavior since we dont want to store invalid pointers.

    Note: If memory allocation scheme is changed from PS MEM to something
    else, change this appropriately.
  -------------------------------------------------------------------------*/
  if (FALSE == ps_mem_is_valid ((void *)pISignal, PS_MEM_DS_UTILS_SIGNAL))
  {
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Call IsValid() on the object now. This is not completely foolproof,
    but for now, this should be enough.
  -------------------------------------------------------------------------*/
  pSignal = reinterpret_cast <Signal *> (pISignal);
  if (FALSE == pSignal->IsValid())
  {
    return QDS_EFAULT;
  }

  return AEE_SUCCESS;

} /* SignalBus::ValidateSignal() */

int32 SignalBus::AddItem
(
  INode* item
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2 ("Fac 0x%p, item 0x%p", this, item, 0);

  return _list.PushFront(item);

} /* AddItem() */

ds::ErrorType CDECL SignalBus::Remove
(
  ISignal* piSignal
)
throw()
{
/*-------------------------------------------------------------------------*/

  LOG_MSG_INFO1 ("Remove sig 0x%p from SignalBus 0x%p", piSignal, this, 0);

  RemoveItem (reinterpret_cast <Signal *> (piSignal));

  return AEE_SUCCESS;
}

/*!
  @brief
  Command handler for signal bus dispatch command.

  @return     None.
*/
void SignalBus::DispatchCmdHandler
(
  ds_sig_cmd_enum_type    cmd,
  void                   *pUserData
)
throw()
{
  SignalBus *pBus = reinterpret_cast <ds::Utils::SignalBus *> (pUserData);
  int userData = (int)SIGNAL_PROCESS_ENUM_SET;

  if (NULL == pUserData)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    ASSERT (0);
    return;
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2 ("obj 0x%p", pBus, 0, 0);

  (void) pBus->Traverse ((void *) &userData);

  (void) pBus->Release(); // if the bus was released during the dispatch,
                          // it can be safely destroyed now.

} /* SignalBus::DispatchCmdHandler() */

void SignalBus::RegisterCmdHandler
(
  void
)
throw()
{
  /*------------------------------------------------------------------------
    Register command handlers.
  -------------------------------------------------------------------------*/
  (void) ds_sig_set_cmd_handler (DS_SIG_SIGNAL_BUS_SET_CMD,
                                 SignalBus::DispatchCmdHandler);
} /* SignalBus::Init() */

int SignalBus::CreateInstance (void* env,
                               AEECLSID clsid,
                               void* privset,
                               void** newObj)
{
  (void) privset;
  (void) env;

  if (AEECLSID_CSignalBus == clsid)
  {
    *newObj = (ISignalBus *)(new SignalBus());
    if (NULL == *newObj)
    {
      return AEE_ENOMEMORY;
    }

    return AEE_SUCCESS;
  }
  return AEE_ECLASSNOTSUPPORT;

} /* SignalBus::CreateInstance() */
