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
#include "ds_Net_QoSSecondariesInput.h"
#include "ds_Net_QoSSecondary.h"
#include "ds_Utils_CreateInstance.h"
#include "ds_Net_Privileges_Def.h"
#include "AEEIPrivSet.h"

using namespace ds::Error;
using namespace ds::Net;

QoSSecondariesInput::QoSSecondariesInput()
: refCnt (1),
  weakRefCnt (1)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Creating object 0x%p", this, 0, 0);  

  mpQoSSecondaryArray[0] = NULL;
}

void QoSSecondariesInput::Destructor
(
  void 
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);
 
  //DS_UTILS_RELEASEIF (mpSigBusProfilesChanged);

} /* QoSSecondariesInput::Destructor() */

QoSSecondariesInput::~QoSSecondariesInput()
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  //NO-OP: only used for freeing memory.

} /* QoSSecondariesInput::~QoSSecondariesInput() */


/*---------------------------------------------------------------------------
  Inherited functions from IQoSSecondariesInput.
---------------------------------------------------------------------------*/
ds::ErrorType QoSSecondariesInput::Associate 
(
 ::ds::Net::IQoSSecondary* qosSecondary
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
   
   return AEE_SUCCESS;

} /* Associate() */


ds::ErrorType QoSSecondariesInput::Disassociate 
(
 ::ds::Net::IQoSSecondary* qosSecondary
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
   (void)mQoSSecondariesList.RemoveItem(pQoSSecondary);

   return AEE_SUCCESS;

} /* Disassociate() */

ds::ErrorType QoSSecondariesInput::GetNth 
(
  int index,  
  IQoSSecondary** qosSecondary
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


ds::ErrorType QoSSecondariesInput::GetNumElements 
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
