/*===========================================================================
  FILE: MCastMBMSCtrl.cpp

  OVERVIEW: This file provides implementation of the MCastMBMSCtrl class.

  DEPENDENCIES: None

  Copyright (c) 2007 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_MCastMBMSCtrl.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-04-07 hm  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "ds_Utils_DebugMsg.h"
#include "AEEStdErr.h"
#include "ds_Net_MCastMBMSCtrl.h"
#include "ds_Net_Platform.h"
#include "ds_Net_EventDefs.h"
#include "ds_Net_EventManager.h"
#include "ds_Utils_CreateInstance.h"
#include "ds_Net_Utils.h"
#include "AEECSignalBus.h"

using namespace ds::Net;
using namespace ds::Error;
using namespace NetPlatform;

/*---------------------------------------------------------------------------
  CONSTRUCTOR/DESTRUCTOR
---------------------------------------------------------------------------*/
MCastMBMSCtrl::MCastMBMSCtrl
(
  int32    ifaceHandle,
  int32    mbmsHandle
) : Handle (ifaceHandle),
    mMBMSHandle (mbmsHandle),
    refCnt (1),
    weakRefCnt (1)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Creating object 0x%p", this, 0, 0);

  (void) DS_Utils_CreateInstance (NULL, AEECLSID_CSignalBus,
                                  NULL, (void **)(&mpSigBusStateChange));

  Handle::Init(EventManager::networkMBMSObjList);

} /* MCastMBMSCtrl::MCastMBMSCtrl() */

void MCastMBMSCtrl::Destructor
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);

  DS_UTILS_RELEASEIF (mpSigBusStateChange);

  Handle::Destructor();

} /* MCastMBMSCtrl::Destructor()() */


MCastMBMSCtrl::~MCastMBMSCtrl
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  //NO-OP: only used for freeing memory.

} /* MCastMBMSCtrl::~MCastMBMSCtrl() */

/*---------------------------------------------------------------------------
  Functions inherited from IMCastMBMSCtrlPriv
---------------------------------------------------------------------------*/
int MCastMBMSCtrl::DeActivate
(
  void
)
{
  int32                               result;
  NetPlatform::MBMSContextDeactType   deActInfo;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
    Construct the deactivate IOCTL arg.
  -------------------------------------------------------------------------*/
  memset (&deActInfo, 0, sizeof (deActInfo));

  /*-------------------------------------------------------------------------
    Perform iface ioctl to de-activate MBMS multicast context.
  -------------------------------------------------------------------------*/
  deActInfo.handle = mMBMSHandle;
  result = IfaceIoctl(GetHandle(),
                      IFACE_IOCTL_MBMS_MCAST_CONTEXT_DEACTIVATE,
                      static_cast <void *> (&deActInfo));
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err %d", result, 0, 0);
  return result;

} /* DeActivate() */


int MCastMBMSCtrl::GetState
(
  MBMSStateType* status
)
{
  int32 result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
    Perform iface ioctl to get the MBMS MCast context status.
  -------------------------------------------------------------------------*/
  result = IfaceIoctl (GetHandle(),
                       IFACE_IOCTL_MBMS_MCAST_CONTEXT_GET_STATUS,
                       static_cast <void *> (status));
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err %d", result, 0, 0);
  return result;
} /* GetState() */

int MCastMBMSCtrl::QueryInterface
(
  AEEIID iid,
  void **ppo
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1  ("Obj 0x%p, ref cnt %d, iid 0x%x", this, refCnt, iid);

  if (NULL == ppo)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  *ppo = NULL;

  switch (iid)
  {
  case AEEIID_IMCastMBMSCtrlPriv:
    *ppo = static_cast <IMCastMBMSCtrlPriv *>(this);
    (void) AddRef();
    break;

  case AEEIID_IQI:
    *ppo = reinterpret_cast <IQI *> (this);
    (void) AddRef();
    break;

  default:
    return AEE_ECLASSNOTSUPPORT;
  }

  return AEE_SUCCESS;
} /*QueryInterface() */




ds::ErrorType MCastMBMSCtrl::GetSignalBus
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
    return QDS_EFAULT;
  }


  switch (eventID)
  {
    case  MBMSEvent::QDS_EV_ACTIVATE_STATE:
      *ppISigBus = mpSigBusStateChange;
      (void)(*ppISigBus)->AddRef();
      return AEE_SUCCESS;

    default:
      *ppISigBus = NULL;
      return QDS_EINVAL;
  }

} /* GetSignalBus() */

