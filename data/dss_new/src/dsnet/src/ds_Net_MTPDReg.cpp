/*===========================================================================
  FILE: MTPDReg.cpp

  OVERVIEW: This file provides implementation of the MTPDReg class.

  DEPENDENCIES: None

  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_MTPDReg.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-08-10 hm  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "ds_Utils_DebugMsg.h"
#include "AEEStdErr.h"
#include "ds_Net_TechUMTSFactory.h"
#include "ds_Net_MTPDReg.h"
#include "ds_Net_Platform.h"
#include "ds_Net_EventDefs.h"
#include "ds_Net_EventManager.h"
#include "ds_Utils_CreateInstance.h"
#include "AEECSignalBus.h"

using namespace ds::Net;
using namespace NetPlatform;

MTPDReg::MTPDReg
(
  int32 ifaceHandle,
  int32 mtpdHandle
)
throw()
: Handle (ifaceHandle),
  mMTPDHandle (mtpdHandle),
  mRegObj(NULL),
  refCnt(1),
  weakRefCnt(1)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Obj 0x%p, iface 0x%x, MTPD handle 0x%x",
    this, ifaceHandle, mMTPDHandle);

  /* Create signal bus to hold event registration info */
  (void) DS_Utils_CreateInstance(0, AEECLSID_CSignalBus,
                                 0, (void **) &mpSigBusMTPD);

  Handle::Init(EventManager::mtpdObjList);

} /* MTPDReg() */

void MTPDReg::Destructor
(
  void
)
throw()
{
  MTDeRegCBType   mtDeRegInfo;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);

  memset (&mtDeRegInfo, 0, sizeof(mtDeRegInfo));
  mtDeRegInfo.handle = mMTPDHandle;

  (void) IfaceIoctl (GetHandle(),
                     IFACE_IOCTL_MT_DEREG_CB,
                     &mtDeRegInfo);

  DS_UTILS_RELEASEIF (mpSigBusMTPD);

  Handle::Destructor();

} /* MTPDReg::Destructor() */

MTPDReg::~MTPDReg
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  mRegObj = NULL;

} /* ~MTPDReg() */


boolean MTPDReg::Process
(
  void* userDataPtr
)
{
  EventInfoType*       eventInfoPtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (NULL == userDataPtr)
  {
    LOG_MSG_ERROR ("NULL ev info", 0, 0, 0);
    return FALSE;
  }

  eventInfoPtr = (EventInfoType *)userDataPtr;
  if (eventInfoPtr->userHandle == mMTPDHandle)
  {
    return Handle::Process(userDataPtr);
  }

  return TRUE;

} /* Process() */

ds::ErrorType MTPDReg::GetSignalBus
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
    case  TechUMTSFactory::QDS_EV_MTPD:
      *ppISigBus = mpSigBusMTPD;
      (void)(*ppISigBus)->AddRef();
      return AEE_SUCCESS;

    default:
      *ppISigBus = NULL;
      return AEE_EBADPARM;
  }

} /* GetSignalBus() */

