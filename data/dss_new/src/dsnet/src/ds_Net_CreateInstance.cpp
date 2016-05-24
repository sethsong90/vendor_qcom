/*=========================================================================*/
/*!
  @file
  ds_Net_CreateInstance.cpp

  @brief
  This file provides implementation of DSNetCreateInstance().

  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*=========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_CreateInstance.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-07-14 hm  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "AEEStdErr.h"
#include "ds_Utils_DebugMsg.h"
#include "ds_Utils_CSSupport.h"
#include "ds_Net_CreateInstance.h"
#include "ds_Net_NetworkFactory.h"
#include "ds_Net_Policy.h"
#include "ds_Net_IPFilterSpec.h"
#include "ds_Net_QoSFlowSpec.h"
#include "ds_Net_MBMSSpec.h"
#include "ds_Net_TechUMTSFactory.h"
#include "ds_Net_ClassIDInstantiator.h"

#include "ds_Net_CNetworkFactory.h"
#include "ds_Net_CNetworkFactoryPriv.h"

#include "ds_Net_CNetworkFactoryService.h"
#include "ds_Net_CNetworkFactoryPrivService.h"


/*===========================================================================

                        PUBLIC FUNCTION DEFINITIONS

===========================================================================*/

int DSNetCreateInstance
(
  IEnv* env,
  AEECLSID clsid,
  IPrivSet* privset,
  void** newObj
)
{
  int ret;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  // Note: in the cases below, the same method is being called
  // both for AEECLSID_CX and for AEECLSID_CXService.
  // AEECLSID_CX is used when the DS requests to create the object.
  // AEECLSID_CXService is used when the CS requests to create the object.
  switch (clsid)
  {
    case ds::Net::AEECLSID_CNetworkFactory:
    case ds::Net::AEECLSID_CNetworkFactoryPriv:
    case ds::Net::AEECLSID_CNetworkFactoryService:
    case ds::Net::AEECLSID_CNetworkFactoryPrivService:
      ret = dsNetNetworkFactoryCreateInstance(env, clsid, privset, newObj);
      break;

    default:
      ret = AEE_ECLASSNOTSUPPORT;
      break;
  
  } /* switch (clsid) */

  LOG_MSG_INFO1 ("Clsid 0x%x, retval 0x%x, obj 0x%p", clsid, ret, newObj);
  
  return ret;

} /* DSNetCreateInstance() */


