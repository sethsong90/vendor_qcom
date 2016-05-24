/*===========================================================================
  FILE: ds_Net_MCastSessionsOutput.cpp

  OVERVIEW: This file provides implementation of the MCastSessionsOutput class.

  DEPENDENCIES: None

  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/

#include "comdef.h"
#include "ds_Utils_DebugMsg.h"
#include "AEEStdErr.h"
#include "ds_Net_MCastSessionsOutput.h"
#include "ds_Net_MCastSession.h"
#include "ds_Utils_CreateInstance.h"

#include "ds_Net_Privileges_Def.h"
#include "AEEIPrivSet.h"

using namespace ds::Error;
using namespace ds::Net;

MCastSessionsOutput::MCastSessionsOutput()
: refCnt (1),
  weakRefCnt (1)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Creating object 0x%p", this, 0, 0);  
}/* MCastSessionsOutput::MCastSessionsOutput() */

void MCastSessionsOutput::Destructor
(
  void 
)
throw()
{
  ds::Utils::INode *pItem = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);
  
  while (NULL != (pItem = mMCastSessionsList.PopFront()))
  {
    (void) pItem->Release();
  }
  
} /* MCastSessionsOutput::Destructor() */

MCastSessionsOutput::~MCastSessionsOutput()
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  //NO-OP: only used for freeing memory.

} /* MCastSessionsOutput::~MCastSessionsOutput() */


/*---------------------------------------------------------------------------
  Inherited functions from IMCastSessionsOutput.
---------------------------------------------------------------------------*/
ds::ErrorType MCastSessionsOutput::GetNth 
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


ds::ErrorType MCastSessionsOutput::GetnumElements 
(
  int* numElements
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
    Validate arguments.
  -------------------------------------------------------------------------*/
  if (0 == numElements)
  {
    LOG_MSG_ERROR ("numElements is NULL", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
  Get the number of elements from the list
  -------------------------------------------------------------------------*/
  *numElements = mMCastSessionsList.Count();

  return AEE_SUCCESS;

} /* GetnumElements() */

ds::ErrorType MCastSessionsOutput::AddMCastSession
(
  IMCastSession * pSession
)
{
  MCastSession*      pMCastSession = 0;
  ds::ErrorType      result = AEE_SUCCESS;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if(0 == pSession)
  {
    LOG_MSG_ERROR ("pSession is NULL", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Push the MCastSession object to the beginning of the list.
    Take a reference to MCastSession object, caller is responsible 
    to release it 
  -------------------------------------------------------------------------*/
  pMCastSession = static_cast <MCastSession *>(pSession);

  result = mMCastSessionsList.PushFront(pMCastSession);
  if(AEE_SUCCESS == result)
  {
    (void) pMCastSession->AddRef();
  }

  return result;
}/* AddMCastSession() */

