/*===========================================================================
  FILE: QoSSecondariesOutput.cpp

  OVERVIEW: This file provides implementation of the QoSSecondariesOutput class.

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
#include "ds_Net_QoSSecondariesOutput.h"
#include "ds_Net_QoSSecondary.h"
#include "ds_Utils_CreateInstance.h"
#include "ds_Net_Privileges_Def.h"
#include "AEEIPrivSet.h"

using namespace ds::Error;
using namespace ds::Net;

QoSSecondariesOutput::QoSSecondariesOutput()
: refCnt (1),
  weakRefCnt (1)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Creating object 0x%p", this, 0, 0);  
}

void QoSSecondariesOutput::Destructor
(
  void 
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);
  
} /* QoSSecondariesOutput::Destructor() */

QoSSecondariesOutput::~QoSSecondariesOutput()
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  QoSSecondary* pQoSSecondary;

  // Release the reference to all the QoSSecondaries objects.
  for(int i=0 ; i<mQoSSecondariesList.Count(); i++)
  {
     pQoSSecondary = static_cast <QoSSecondary *> (mQoSSecondariesList.Get(i));
     DS_UTILS_RELEASEIF(pQoSSecondary); 
  }
} /* QoSSecondariesOutput::~QoSSecondariesOutput() */


/*---------------------------------------------------------------------------
  Inherited functions from IQoSSecondariesOutput.
---------------------------------------------------------------------------*/
ds::ErrorType QoSSecondariesOutput::GetNth 
(
 int index,  ::ds::Net::IQoSSecondary** qosSecondary
)
{
  QoSSecondary*      pQoSSecondary;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
    Validate arguments.
  -------------------------------------------------------------------------*/
  if (index < 0)
  {
    LOG_MSG_ERROR ("index < 0 ", 0, 0, 0);
    return AEE_EFAILED;
  }

  /*-------------------------------------------------------------------------
  Get the Nth QoSSecondary object from the list
  -------------------------------------------------------------------------*/
  pQoSSecondary = static_cast <QoSSecondary *> (mQoSSecondariesList.Get(index));
  if (NULL == pQoSSecondary)
  {
    LOG_MSG_ERROR ("NULL item at index %d", index, 0, 0);
    return AEE_EFAILED;
  }

  *qosSecondary = pQoSSecondary;
  (*qosSecondary)->AddRef();

  return AEE_SUCCESS;

} /* GetNth() */


ds::ErrorType QoSSecondariesOutput::GetnumElements 
(
 int* numElements
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
  Validate arguments.
  -------------------------------------------------------------------------*/
  if (NULL == numElements)
  {
    LOG_MSG_ERROR ("numElements == NULL", 0, 0, 0);
    return AEE_EFAILED;
  }

  /*-------------------------------------------------------------------------
  Get the number of elements from the list
  -------------------------------------------------------------------------*/
  *numElements = mQoSSecondariesList.Count();

  return AEE_SUCCESS;

} /* GetnumElements() */

ds::ErrorType QoSSecondariesOutput::AddQoSSecondary
(
  IQoSSecondary* qosSecondary
)
{
  QoSSecondary*      pQoSSecondary;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
    Validate arguments.
  -------------------------------------------------------------------------*/
  if (NULL == qosSecondary)
  {
    LOG_MSG_ERROR ("qosSecondary == NULL", 0, 0, 0);
    return AEE_EFAILED;
  }

  /*-------------------------------------------------------------------------
  Push the QoSSecondary object to the beginning of the list
  -------------------------------------------------------------------------*/
  pQoSSecondary = static_cast <QoSSecondary *>(qosSecondary);
  (void)mQoSSecondariesList.PushFront(pQoSSecondary);

  // Add a reference to this object. This is the reference of the 
  // QoSSecondariesOutput object to the session. This reference is
  // released in the QoSSecondariesOutput destructor.
  // The AddRef() in GetNth() is the reference that the user holds
  // to the QoSSessions.
  pQoSSecondary->AddRef();

  return AEE_SUCCESS;

}/* AddQoSSecondary() */

