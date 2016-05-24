#ifndef DS_SOCK_CREATE_INSTANCE_H
#define DS_SOCK_CREATE_INSTANCE_H

/*=========================================================================*/
/*!
  @file
  ds_Sock_CreateInstance.h

  @brief
  This file provides declarations for creating instances of various DS
  objects.

  @detail
  DSSockCreateInstance() method is used to create various DSSOCK objects.
  This function should be used on non-CS platforms. ON CS platform
  env_CreateInstance() should be used.

  To create an instance of an interface, an appropriate class ID is required.
  The class IDs are contained in a special header file that comes along with
  this distribution.

  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*=========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dssock/rel/11.03/inc/ds_Sock_CreateInstance.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-07-14 hm  Created module.

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "AEEStdErr.h"
#include "AEEIPrivSet.h"
#include "AEEIEnv.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
  @brief
  Creates an instance of a speficied implementation object.

  @detail
  This function is used to create objects of the DSSOCK library. This
  function is provided for non-CS platforms only (Brew, WM, Linux). For
  CS-platforms please use the env_CreateInstance() CS function.

  @param[in] env
  Unused parameter. Set to NULL. It is included here to be consistent with
  CS env_CreateInstance() signature.

  @param[in] clsid
  Class ID of the class that needs to be created. The class ID is stored in
  special header file (such as ds_Sock_CSocketFactory.h). These files define a
  specific implementation of an interface. 

  @param[in] privSet
  Unused parameter. Set to NULL. It is used here to be consistent with
  CS env_CreateInstance() signature.

  @param[out] newObj
  This is for returning the out parameter for the newly created object.

  @see        CS env_CreateInstance() method for reference.
  @see        Set of header files that come with this release, and define class
              IDs. These objects can be created using DSSockCreateInstance().
  @return     ds::Error::QDS_EFAULT - On invalid arguments
  @return     AEE_ECLASSNOSUPPORT - Unsupported class ID.
  @return     AEE_SUCCESS - On successful creation of the object.
*/

int DSSockCreateInstance
(
   IEnv* env,
   AEECLSID clsid,
   IPrivSet* privset,
   void** newObj
);

#ifdef __cplusplus
}
#endif


#endif /* DS_SOCK_CREATE_INSTANCE_H */

