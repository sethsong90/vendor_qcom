/*!
  @file
  ds_Utils_CreateInstance_CS.cpp

  @brief
  This file provides implementation of CreateInstance for objects of
  ds::Utils module.

  When Component Services is present, the objects implemented by CS are
  created through it, and the internal ds::Utils implementation is not used.

*/
/*===========================================================================
  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ds_Utils_CreateInstance_cs.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-02-28 mt  Separated out CS-only functionality.
  2008-07-14 hm  Created module.

===========================================================================*/

/*===========================================================================

                        INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"

#ifdef FEATURE_DATA_PS
#include "ds_Errors_Def.h"
#include "ds_Utils_CreateInstance.h"
#include "ds_Utils_DebugMsg.h"

#include "AEECCritSect.h"
#include "AEECSignalFactory.h"
#include "AEECSignalBus.h"
#include "AEEIEnv.h"
#include "AEEenv.h"
#include "AEECEnv.h"

/*===========================================================================

                      STATIC VARIABLES FOR MODULE

===========================================================================*/

/*===========================================================================

                        PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
int DS_Utils_CreateInstance
(
  void *    env,
  AEECLSID  clsid,
  void *    privset,
  void **   newObj
)
{
  IEnv* piEnv = (IEnv*)env;
  int  nErr = AEE_SUCCESS;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == piEnv) {
    nErr = env_GetCurrent(&piEnv);
    if (AEE_SUCCESS != nErr) {
      LOG_MSG_ERROR("Cannot recover a CS environment pointer, CS ret=%d",
                     nErr, 0, 0);
      return nErr;
    }
  }

  switch (clsid)
  {
    case AEECLSID_CSignalBus:
    case AEECLSID_CCritSect:
    case AEECLSID_CSignalFactory:
      nErr = piEnv->CreateInstance(clsid, newObj);
      if (AEE_SUCCESS != nErr)
      {
        LOG_MSG_ERROR("CS object creation failed, CS ret=%d", nErr, 0, 0);
      }
      break;

    default:
      nErr = AEE_ECLASSNOTSUPPORT;

  } /* switch(clsid) */

  LOG_MSG_INFO1("Clsid 0x%x, retval 0x%x, obj 0x%p", clsid, nErr, newObj);
  return nErr;

} /* DS_Utils_CreateInstance() */

#endif /* FEATURE_DATA_PS */

