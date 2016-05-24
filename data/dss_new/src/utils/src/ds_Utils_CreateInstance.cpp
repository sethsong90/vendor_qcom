/*!
  @file
  ds_Utils_CreateInstance.cpp

  @brief
  This file provides implementation CreateInstance methods for objects of
  ds::Utils module when Component Services is not available.

  When CS is not available, internal ds::Utils implementation is used.
*/
/*===========================================================================
  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ds_Utils_CreateInstance.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-01-10 mt  Separated CS functionality into a different file.
  2008-07-14 hm  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"

#ifdef FEATURE_DATA_PS
#include "AEECCritSect.h"
#include "AEECSignalFactory.h"
#include "AEECSignalBus.h"

#include "ds_Utils_CreateInstance.h"
#include "ds_Utils_CritSect.h"
#include "ds_Utils_SignalBus.h"
#include "ds_Utils_SignalFactory.h"
#include "ds_Errors_Def.h"


/*===========================================================================

                        PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
ds::ErrorType DS_Utils_CreateInstance
(
  void *    env,
  AEECLSID  clsid,
  void *    privset,
  void **   newObj
)
{
  ds::ErrorType dsErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  switch (clsid)
  {
    case AEECLSID_CSignalBus:
      dsErrno = ds::Utils::SignalBus::CreateInstance(env, clsid, privset, newObj);
      break;

    case AEECLSID_CCritSect:
      dsErrno = ds::Utils::CritSect::CreateInstance(env, clsid, privset, newObj);
      break;

    case AEECLSID_CSignalFactory:
      dsErrno = ds::Utils::SignalFactory::CreateInstance(env, clsid, privset, newObj);
      break;

    default:
      dsErrno = AEE_ECLASSNOTSUPPORT;

  } /* switch(clsid) */

  LOG_MSG_INFO1("Clsid 0x%x, retval 0x%x, obj 0x%p", clsid, dsErrno, newObj);
  return dsErrno;

} /* DS_Utils_CreateInstance() */

#endif /* FEATURE_DATA_PS */

