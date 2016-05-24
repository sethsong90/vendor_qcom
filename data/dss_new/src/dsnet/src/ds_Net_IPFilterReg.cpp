/*==========================================================================*/
/*!
  @file
  ds_Net_IPFilterReg.cpp

  @brief
  This file provides implementation for the ds::Net::IPFilterReg class.

  @see  ds_Net_IPFilterReg.h

  Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*=========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_IPFilterReg.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-08-10 hm  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "AEEStdErr.h"
#include "ds_Utils_DebugMsg.h"
#include "ds_Net_IPFilterReg.h"
#include "ds_Net_Platform.h"
#include "ds_Net_EventDefs.h"
#include "ds_Net_EventManager.h"
#include "ds_Utils_CreateInstance.h"
#include "ds_Net_Utils.h"
#include "AEECSignalBus.h"

using namespace ds::Net;

IPFilterReg::IPFilterReg
(
  int32 ifaceHandle,
  int32 fltrHandle
)
: Handle (ifaceHandle),
  mFltrHandle (fltrHandle),
  refCnt(1),
  weakRefCnt(1)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Creating obj 0x%p, if handle 0x%x, fltr handle 0x%x",
    this, ifaceHandle, mFltrHandle);

  /* Create signalbuses to hold event registration info */
  (void) DS_Utils_CreateInstance (NULL, AEECLSID_CSignalBus,
                                  NULL, (void **)(&mpSigBusStateChange));

  Handle::Init (EventManager::ipFilterObjList);

} /* IPFilterReg() */

void IPFilterReg::Destructor
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting obj 0x%x, fltr handle 0x%x",
    this, mFltrHandle, 0);

  (void) NetPlatform::PSIfaceIPFilterDelete (GetHandle(),
                                             IP_FLTR_CLIENT_SOCKETS,
                                             mFltrHandle);

  Handle::Destructor();

  DS_UTILS_RELEASEIF (mpSigBusStateChange);

} /* IPFilterReg::Destructor() */

IPFilterReg::~IPFilterReg
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  //NO-OP: only used for freeing memory.

} /* ~IPFilterReg() */

ds::ErrorType IPFilterReg::GetSignalBus
(
  ds::Net::EventType  eventID,
  ISignalBus **       ppISigBus
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == ppISigBus)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    ASSERT (0);
    return AEE_EBADPARM;
  }


  switch (eventID)
  {
    case  IPFilterEvent::QDS_EV_STATE_CHANGED:
      *ppISigBus = mpSigBusStateChange;
      (void)(*ppISigBus)->AddRef();
      return AEE_SUCCESS;

    default:
      *ppISigBus = NULL;
      return AEE_EBADPARM;
  }

} /* GetSignalBus() */



