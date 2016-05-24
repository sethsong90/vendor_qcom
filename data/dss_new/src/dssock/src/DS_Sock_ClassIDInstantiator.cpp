/*===========================================================================
  FILE: DS_Sock_ClassIDInstantiator.cpp

  OVERVIEW: This file provides implementation for the methods defined in
            DS_Sock_ClassIDInstantiator.h

  DEPENDENCIES: None

  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_ClassIDInstantiator.cpp#1 $
  $DateTime: 2009/09/28 14:45:31 $$Author: dmudgal $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-05-14 msr Created module

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"

#ifdef FEATURE_DATA_PS
#include "DS_Errors.h"
#include "DS_Utils_CSSupport.h"
#include "DS_Sock_ClassIDInstantiator.h"
#include "DS_Sock_SocketFactory.h"
#include "DS_Sock_SocketFactoryPriv.h"

using namespace DS::Sock;
using namespace DS::Error;


/*===========================================================================

                        PUBLIC MEMBER FUNCTIONS

===========================================================================*/
extern "C"
int DSSockSocketFactoryCreateInstance
(
  void *    envPtr,
  AEECLSID  clsID,
  void *    privSetPtr,
  void **   newObjPtrPtr
)
{
   (void)envPtr;
   (void)clsID;
   (void)privSetPtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == newObjPtrPtr)
  {
    LOG_MSG_ERROR( "NULL obj", 0, 0, 0);
    return DSS_EFAULT;
  }

  *newObjPtrPtr =
    reinterpret_cast <ISocketFactory *> ( SocketFactory::CreateInstance());

  if (0 == *newObjPtrPtr)
  {
    LOG_MSG_ERROR( "Couldn't allocate SocketFactory", 0, 0, 0);
    return DSS_ENOMEM;
  }

  return SUCCESS;
} /* DSSockSocketFactoryCreateInstance() */


extern "C"
int DSSockSocketFactoryPrivCreateInstance
(
  void *    envPtr,
  AEECLSID  clsID,
  void *    privSetPtr,
  void **   newObjPtrPtr
)
{
   (void)envPtr;
   (void)clsID;
   (void)privSetPtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == newObjPtrPtr)
  {
    LOG_MSG_ERROR( "NULL obj", 0, 0, 0);
    return DSS_EFAULT;
  }

  *newObjPtrPtr =
    reinterpret_cast <ISocketFactory *> ( SocketFactoryPriv::CreateInstance());

  if (0 == *newObjPtrPtr)
  {
    LOG_MSG_ERROR( "Couldn't create SocketFactoryPriv", 0, 0, 0);
    return DSS_ENOMEM;
  }

  return SUCCESS;
} /* DSSockSocketFactoryCreateInstance() */

#endif /* FEATURE_DATA_PS */
