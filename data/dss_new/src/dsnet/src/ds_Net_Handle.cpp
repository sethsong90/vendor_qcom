/*==========================================================================*/
/*!
  @file
  ds_Net_Handle.cpp

  @brief
  This file provides the implementation of the ds::Net::Handle object.

  @see  ds_Net_Handle.h

  Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*=========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_Handle.cpp#1 $$DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-03-10 hm  Created module.

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "AEEISignal.h"
#include "AEEStdErr.h"
#include "ds_Utils_DebugMsg.h"
#include "ds_Utils_CreateInstance.h"
#include "ds_Net_Handle.h"
#include "ds_Net_EventDefs.h"
#include "ds_Net_EventManager.h"
#include "ds_Net_Utils.h"
#include "AEECCritSect.h"

using namespace ds::Net;
using namespace ds::Utils;
using namespace ds::Error;

/*---------------------------------------------------------------------------
  Constructor/Destructor.
---------------------------------------------------------------------------*/
Handle::Handle
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2 ("Creating 0x%p", this, 0, 0);

  mObjHandle        = 0;
  mStaleIfaceHandle = 0;

  /* Create crit sect for this object. */
  if (AEE_SUCCESS != DS_Utils_CreateInstance (NULL,
                                              AEECLSID_CCritSect,
                                              NULL,
                                              (void **) &mpICritSect))
  {
    LOG_MSG_FATAL_ERROR ("Cannot create crit sect", 0, 0, 0);
    ASSERT (0);
  }

} /* Handle() */

Handle::Handle
(
  int32                  objHandle
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2 ("Creating 0x%p, handle 0x%x", this, objHandle, 0);

  mObjHandle = objHandle;
  mpParentFactory = NULL;

  /* Create crit sect for this object. */
  if (AEE_SUCCESS != DS_Utils_CreateInstance (NULL,
                                              AEECLSID_CCritSect,
                                              NULL,
                                              (void **) &mpICritSect))
  {
    LOG_MSG_FATAL_ERROR ("Cannot create crit sect", 0, 0, 0);
    ASSERT (0);
  }

  mStaleIfaceHandle = 0;

} /* Handle() */

void Handle::Init
(
  ds::Utils::IFactory   *pIFactory
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /* Add self to parent factory */
  ASSERT (NULL != pIFactory);
  mpParentFactory = pIFactory;
  (void) mpParentFactory->AddItem (static_cast <ds::Utils::INode *> (this));

}

void Handle::Destructor
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2 ("Deleting 0x%p, handle 0x%x", this, mObjHandle, 0);

  mObjHandle        = 0;
  mStaleIfaceHandle = 0;

  /* Remove self from parent factory */
  if (NULL != mpParentFactory)
  {
    mpParentFactory->RemoveItem (static_cast <ds::Utils::INode *> (this));
  }

  mObjHandle = 0;

  /*lint -save -e1550, -e1551 */
  DS_UTILS_RELEASEIF(mpICritSect);
  /*lint -restore */

} /* Destructor() */


Handle::~Handle
(
  void
)
throw()
{
/*-------------------------------------------------------------------------*/

  mpParentFactory = NULL;

} /* ~Handle() */


/*---------------------------------------------------------------------------
  Protected functions
---------------------------------------------------------------------------*/
void Handle::SetHandle
(
  int32 objHandle
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2 ("Obj 0x%p, handle=0x%x", this, objHandle, 0);

  mpICritSect->Enter();

  if (0 == objHandle)
  {
    /*-----------------------------------------------------------------------
      Defensive check so that if SetHandle(0) is called back to back,
      mStaleIfaceHandle is not set to 0
    -----------------------------------------------------------------------*/
    if (0 != mObjHandle)
    {
      mStaleIfaceHandle = mObjHandle;
    }
  }
  else
  {
    mStaleIfaceHandle = 0;
  }

  mObjHandle = objHandle;

  LOG_MSG_INFO2 ("Obj 0x%p, handle=0x%x staleHandle=0x%x",
                 this, mObjHandle, mStaleIfaceHandle);

  mpICritSect->Leave();
  return;

} /* SetHandle() */

/*---------------------------------------------------------------------------
  Inherited function definitions.
---------------------------------------------------------------------------*/
int Handle::OnStateChange
(
  ISignal              *pISignal,
  ds::Net::EventType    eventName,
  IQI** regObj
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return RegEvent(pISignal, eventName, regObj);

} /* OnStateChange() */

int32 Handle::RegEvent
(
  ISignal*    pISignal,
  int32       eventName,
  IQI**       regObj
)
{
  int32          result;
  ISignalBus *   pISigBus = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("handle 0x%x, ev 0x%x, sig 0x%p",
                 mObjHandle, eventName, pISignal);

  result = GetSignalBus (eventName, &pISigBus);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Due to a possible security issue where applications may invoke this
    function many times thus causing undesirable extra memory allocation that
    may be entailed in an implementation of socket events registration,
    we should return an object that the application will hold for each
    RegEvent invocation. A Limit that CS imposes on the number of objects the
    application can hold resolves the security issue.

    If the implementation of RegEvent is changed an appropriate
    object must be returned.
  -------------------------------------------------------------------------*/
  *regObj = new RegBound();


  if (NULL == pISignal || NULL == pISigBus)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  result = pISigBus->Add(pISignal);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  result = AEE_SUCCESS;

bail:
  DS_UTILS_RELEASEIF(pISigBus);
  if (AEE_SUCCESS != result)
  {
    LOG_MSG_ERROR ("Err 0x%x registering for event", result, 0, 0);
  }
  return result;

} /* RegEvent() */

void Handle::Notify
(
  int32  eventName
)
{
  ISignalBus *   pISigBus = NULL;
  ds::ErrorType  result = AEE_SUCCESS;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Handle 0x%x, ev 0x%x", mObjHandle, eventName, 0);

  result = GetSignalBus (eventName, &pISigBus);
  if (AEE_SUCCESS != result || NULL == pISigBus)
  {
    return;
  }

  /* Strobe the signal bus to notify */
  (void) pISigBus->Strobe();
  DS_UTILS_RELEASEIF(pISigBus);

} /* Notify() */

boolean Handle::Process
(
  void* pUserData
)
{
  EventInfoType*          pEventInfo;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    User data should never be NULL for event processing.
  -------------------------------------------------------------------------*/
  if (NULL == pUserData)
  {
    ASSERT (0);
    return FALSE;
  }

  pEventInfo = static_cast <EventInfoType *> (pUserData);

  LOG_MSG_INFO1 ("this handle 0x%x, ev 0x%x, ev handle 0x%x",
                 mObjHandle, pEventInfo->eventName, pEventInfo->handle);

  if (mObjHandle == pEventInfo->handle)
  {
    /*-----------------------------------------------------------------------
      Event belongs to this handle. Call Notify()
    -----------------------------------------------------------------------*/
    Notify (pEventInfo->eventName);
  }

  return TRUE;

} /* Process() */


