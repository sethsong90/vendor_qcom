/*===========================================================================
  FILE: ds_Net_ClassIDInstantiator.cpp

  OVERVIEW: This file provides implementation for the methods defined in
            ds_Net_ClassIDInstantiator.h

  DEPENDENCIES: None

            Copyright (c) 2010 Qualcomm Technologies, Inc.
            All Rights Reserved.
            Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_ClassIDInstantiator.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-06-13 vm Created module

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"

#ifdef FEATURE_DATA_PS
#include "AEEStdErr.h"
#include "ds_Utils_CSSupport.h"
#include "ds_Net_ClassIDInstantiator.h"
#include "ds_Net_NetworkFactoryClient.h"
#include "ds_Net_CNetworkFactory.h"
#include "ds_Net_CNetworkFactoryPriv.h"
#include "ds_Net_CNetworkFactoryService.h"
#include "ds_Net_CNetworkFactoryPrivService.h"

using namespace ds::Net;
using namespace ds::Error;


/*===========================================================================

                        PUBLIC MEMBER FUNCTIONS

===========================================================================*/
extern "C"
int dsNetNetworkFactoryCreateInstance
(
  void *    envPtr,
  AEECLSID  clsID,
  void *    privSetPtr,
  void **   newObjPtrPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == newObjPtrPtr)
  {
    LOG_MSG_ERROR( "NULL obj", 0, 0, 0);
    return QDS_EFAULT;
  }

  if (AEECLSID_CNetworkFactory == clsID ||
      AEECLSID_CNetworkFactoryService == clsID ||
      AEECLSID_CNetworkFactoryPriv == clsID ||
      AEECLSID_CNetworkFactoryPrivService == clsID)
  {
    *newObjPtrPtr =
      NetworkFactoryClient::CreateInstance(clsID ,(IPrivSet *)privSetPtr);

    if (0 == *newObjPtrPtr)
    {
      LOG_MSG_ERROR( "Couldn't allocate NetworkFactoryClient", 0, 0, 0);
      return AEE_ENOMEMORY;
    }
  }

  return AEE_SUCCESS;

} /* dsNetNetworkFactoryCreateInstance() */

#endif /* FEATURE_DATA_PS */
