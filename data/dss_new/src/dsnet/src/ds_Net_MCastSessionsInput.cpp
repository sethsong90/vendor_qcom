/*===========================================================================
  FILE: QoSSecondariesInput.cpp

  OVERVIEW: This file provides implementation of the QoSSecondariesInput class.

  DEPENDENCIES: None

  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/

#include "comdef.h"
#include "ds_Utils_DebugMsg.h"
#include "AEEStdErr.h"
#include "ds_Net_MCastSessionsInput.h"
#include "ds_Net_MCastSession.h"
#include "ds_Utils_CreateInstance.h"
#include "ds_Net_Privileges_Def.h"
#include "AEEIPrivSet.h"

using namespace ds::Error;
using namespace ds::Net;

MCastSessionsInput::MCastSessionsInput()
: refCnt (1),
  weakRefCnt (1)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Creating object 0x%p", this, 0, 0);

}/* MCastSessionsInput::MCastSessionsInput() */

void MCastSessionsInput::Destructor
(
  void 
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);

} /* MCastSessionsInput::Destructor() */

MCastSessionsInput::~MCastSessionsInput()
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  //NO-OP: only used for freeing memory.

} /* MCastSessionsInput::~MCastSessionsInput() */


/*---------------------------------------------------------------------------
  Inherited functions from IMCastSessionsInput.
---------------------------------------------------------------------------*/
ds::ErrorType MCastSessionsInput::Associate 
(
  IMCastSession *  mcastSession
)
{
  MCastSession*      pMCastSession = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
    Validate arguments.
  -------------------------------------------------------------------------*/
  if (0 == mcastSession)
  {
    LOG_MSG_ERROR ("mcastSession == NULL", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Push the MCastSession object to the beginning of the list
  -------------------------------------------------------------------------*/
  pMCastSession = static_cast <MCastSession *>(mcastSession);

  return mMCastSessionsList.PushFront(pMCastSession);

} /* Associate() */


ds::ErrorType MCastSessionsInput::Disassociate 
(
  IMCastSession* mcastSession
)
{
  MCastSession*      pMCastSession = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
    Validate arguments.
  -------------------------------------------------------------------------*/
  if (0 == mcastSession)
  {
    LOG_MSG_ERROR ("mcastSession == NULL", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Remove the MCastSession object from the beginning of the list
  -------------------------------------------------------------------------*/
  pMCastSession = static_cast <MCastSession *>(mcastSession);

  mMCastSessionsList.RemoveItem(pMCastSession);

  return AEE_SUCCESS;

} /* Disassociate() */

ds::ErrorType MCastSessionsInput::GetNth 
(
  int index,
  IMCastSession ** mcastSession
)
{
  MCastSession*      pMCastSession = 0;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
    Validate arguments.
  -------------------------------------------------------------------------*/
  if (index < 0 || 0 == mcastSession)
  {
    LOG_MSG_ERROR ("null args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Get the Nth IMCastSession object from the list
  -------------------------------------------------------------------------*/
  pMCastSession = static_cast <MCastSession *> (mMCastSessionsList.Get(index));
  if(0 == pMCastSession)
  {
    LOG_MSG_ERROR ("can't found MCastSession with index %d", index, 0, 0);
    return AEE_EFAILED;
  }

  *mcastSession = pMCastSession;

  (void) (*mcastSession)->AddRef();

  return AEE_SUCCESS;

} /* GetNth() */

ds::ErrorType MCastSessionsInput::GetNumOfSessions
(
  int32 * sessionsNum
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if(0 == sessionsNum)
  {
    LOG_MSG_ERROR ("pSession is NULL", 0, 0, 0);
    return QDS_EFAULT;
  }

  *sessionsNum = mMCastSessionsList.Count();

  return AEE_SUCCESS;
}/* GetNumOfSessions() */
