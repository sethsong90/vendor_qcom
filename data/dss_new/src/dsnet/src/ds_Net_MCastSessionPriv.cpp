/*===========================================================================
  FILE: ds_Net_MCastSessionPriv.cpp

  OVERVIEW: This file provides implementation of the MCastSession class.

  DEPENDENCIES: None

  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_MCastSessionPriv.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-07-29 vm  Created module.

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "ds_Utils_DebugMsg.h"
#include "AEEStdErr.h"
#include "ds_Net_MCastSessionPriv.h"
#include "ds_Net_Platform.h"
#include "ds_Net_EventDefs.h"
#include "ds_Net_EventManager.h"
#include "ds_Net_Conversion.h"
#include "ds_Utils_CreateInstance.h"
#include "AEECSignalBus.h"

using namespace ds::Net;
using namespace ds::Net::Conversion;
using namespace ds::Error;

/*===========================================================================

                     PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
MCastSessionPriv::MCastSessionPriv
(
  int32          ifaceHandle,
  int32          mcastHandle,
  MCastJoinFlagsType regFlag
)
: Handle (ifaceHandle),
  mMCastHandle (mcastHandle),
  mStatusInfoCode(0),
  mJoinFlag(regFlag),
  refCnt (1),
  weakRefCnt (1)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Creating object 0x%p", this, 0, 0);

  memset((void*)&mRegState_1_0, 0, sizeof(MCastStateChangedType));

  memset((void*)&mRegState_2_0, 0, sizeof(MCastStateChangedType));

  (void) DS_Utils_CreateInstance (NULL, AEECLSID_CSignalBus,
                                  NULL, (void **)(&mpSigBusRegStatus1_0));
  (void) DS_Utils_CreateInstance (NULL, AEECLSID_CSignalBus,
                                  NULL, (void **)(&mpSigBusRegStatus2_0));
  (void) DS_Utils_CreateInstance (NULL, AEECLSID_CSignalBus,
                                  NULL, (void **)(&mpSigBusTechStatus));

  Handle::Init(EventManager::mcastObjList);

} /* MCastSessionPriv() */

void MCastSessionPriv::Destructor
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);

  DS_UTILS_RELEASEIF (mpSigBusTechStatus);

  DS_UTILS_RELEASEIF (mpSigBusRegStatus2_0);

  DS_UTILS_RELEASEIF (mpSigBusRegStatus1_0);

  Handle::Destructor();

} /* Destructor() */

MCastSessionPriv::~MCastSessionPriv
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  //NO-OP: only used for freeing memory.

} /* ~MCastSessionPriv() */


/*---------------------------------------------------------------------------
  Inherited functions from IMCastSessionPriv.
---------------------------------------------------------------------------*/
/**
  @brief      This function issues an IOCTL request to leave the multicast
              group it is associated with.

              This function issues an IOCTL request to leave the multicast
              group it is associated with.

  @param      None

  @see        No external dependencies

  @return     AEE_SUCCESS - if the request is issued to leave the MCast group
              successfully. This does not mean that Mcast group is left
              already.
              Error code - on error.
*/
ds::ErrorType MCastSessionPriv::Leave
(
  void
)
{
  NetPlatform::MCastLeaveType       leaveInfo;
  int                               result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
    Create arguments for MCAST_LEAVE ioctl.
  -------------------------------------------------------------------------*/
  memset (&leaveInfo, 0, sizeof (NetPlatform::MCastLeaveType));

  leaveInfo.handle = mMCastHandle;

  /*-----------------------------------------------------------------------
    Call the iface IOCTL to leave the MCast session.
  -----------------------------------------------------------------------*/
  result = NetPlatform::IfaceIoctl (GetHandle(),
                                    NetPlatform::IFACE_IOCTL_MCAST_LEAVE,
                                    static_cast <void *> (&leaveInfo));
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err %d", result, 0, 0);
  return result;

} /* Leave() */

ds::ErrorType MCastSessionPriv::GetRegStateAndInfoCodeBCMCS1_0
(
  MCastStateChangedType* RegStateAndInfoCodeBCMCS1_0
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, ", this, 0, 0);

  if (NULL == RegStateAndInfoCodeBCMCS1_0)
  {
    return QDS_EFAULT;
  }

  RegStateAndInfoCodeBCMCS1_0->infoCode = mRegState_1_0.infoCode;

  RegStateAndInfoCodeBCMCS1_0->regState = mRegState_1_0.regState;

  return AEE_SUCCESS;
}/* GetRegStateAndInfoCodeBCMCS1_0() */

ds::ErrorType MCastSessionPriv::GetRegStateAndInfoCodeBCMCS2_0
(
  MCastStateChangedType* RegStateAndInfoCodeBCMCS2_0
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, ", this, 0, 0);

  if (NULL == RegStateAndInfoCodeBCMCS2_0)
  {
    return QDS_EFAULT;
  }

  RegStateAndInfoCodeBCMCS2_0->infoCode = mRegState_2_0.infoCode;

  RegStateAndInfoCodeBCMCS2_0->regState = mRegState_2_0.regState;

  return AEE_SUCCESS;

}/* GetRegStateAndInfoCodeBCMCS2_0() */

ds::ErrorType MCastSessionPriv::GetTechStatusInfoCode
(
  MCastInfoCodeType* TechStatusInfoCode
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p", this, 0, 0);

  if (NULL == TechStatusInfoCode)
  {
    return QDS_EFAULT;
  }

  *TechStatusInfoCode = mStatusInfoCode;

  return AEE_SUCCESS;
}/* GetTechStatusInfoCode() */

/*---------------------------------------------------------------------------
  Inherited function from ds::Utils::INode interface.
---------------------------------------------------------------------------*/
/**
  @brief      This method implements the INode interface's Process() function.

              This function is used in case of event handling. Whenever an
              event occurs the event manager singleton traverses the
              network factory. On each network object this Process() function
              is called. If it is a MCast event, the network object calls
              Traverse on the MCastManager. MCastManager calls this Process()
              function on each of the MCast nodes. This Process() function is
              handled thus:

              Switch (EventGroup)
                Case EVENT_GROUP_MCAST:
                  If this MCast object registered for the event
                    If this->handle == eventinfo->handle
                    Notify()
                    Return TRUE.
                  End.
                  Return TRUE.

                Case Default:
                  ASSERT (0)
                  Return FALSE
              End

  @param[in]  userDataPtr: Event info for the event.

  @see        No external dependencies

  @return     TRUE - If the Process() succeeds.
              FALSE - If the Process() fails.
*/
boolean MCastSessionPriv::Process
(
  void* userDataPtr
)
{
  EventInfoType*                      pEventInfo;
  ps_iface_mcast_event_info_type *    pPSMCastEventInfo;
  MCastStateChangedType *             regStateToUpdate = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == userDataPtr)
  {
    LOG_MSG_ERROR ("NULL ev info", 0, 0, 0);
    return FALSE;
  }

  pEventInfo = static_cast <EventInfoType *>(userDataPtr);

  LOG_MSG_INFO1 ("Process():event name %d, group %d, handle %d",
    pEventInfo->eventName, pEventInfo->eventGroup, pEventInfo->handle);

  if (mMCastHandle == pEventInfo->userHandle)
  {

    /*-----------------------------------------------------------------------
      Set the MCast Registration status depending upon event
      and BCMCS version, if received info code is related to BCMCS 1.0, DVBH
      or MFLO - only BCMCS 1.0 state is updated,
      if received info code is BCMCS 2.0 related
      - only BCMCS 2.0 state is updated
    -----------------------------------------------------------------------*/
    pPSMCastEventInfo = (ps_iface_mcast_event_info_type *)
                          pEventInfo->psEventInfo;
    if(MCastInfoCodeIsBCMCS1_0(pPSMCastEventInfo->info_code) ||
       MCastInfoCodeIsDVBHOrMFLO(pPSMCastEventInfo->info_code) ||
       (PS_IFACE_MCAST_MBMS_SYSTEM_UNAVAILABLE ==
         pPSMCastEventInfo->info_code) ||
         (PS_IFACE_MCAST_IC_NOT_SPECIFIED == pPSMCastEventInfo->info_code))
    {
      regStateToUpdate = &mRegState_1_0;
      if(MCastSessionEvent::QDS_EV_REGISTRATION_STATE ==
        pEventInfo->eventName)
      {
        pEventInfo->eventName =
          MCastSessionPrivEvent::QDS_EV_REGISTRATION_STATE_PRIV_BCMCS1_0;
      }
    }
    else
    {
      regStateToUpdate = &mRegState_2_0;
      if(MCastSessionEvent::QDS_EV_REGISTRATION_STATE ==
         pEventInfo->eventName)
      {
        pEventInfo->eventName =
          MCastSessionPrivEvent::QDS_EV_REGISTRATION_STATE_PRIV_BCMCS2_0;
      }
    }
    switch (pEventInfo->psEventName)
    {
      case IFACE_MCAST_REGISTER_SUCCESS_EV:
        regStateToUpdate->regState = MCastRegState::MCAST_REGISTER_SUCCESS;
        break;

      case IFACE_MCAST_REGISTER_FAILURE_EV:
        regStateToUpdate->regState = MCastRegState::MCAST_REGISTER_FAILURE;
        break;

      case IFACE_MCAST_DEREGISTERED_EV:
        regStateToUpdate->regState = MCastRegState::MCAST_DEREGISTERED;
        break;
    } /* switch */

    /*-----------------------------------------------------------------------
      Set the MCast technology status depending upon event info.
    -----------------------------------------------------------------------*/
    if(MCastSessionEvent::QDS_EV_TECHNOLOGY_STATUS ==
       pEventInfo->eventName)
    {
      pEventInfo->eventName =
        MCastSessionPrivEvent::QDS_EV_TECHNOLOGY_STATUS_PRIV;
      
      mStatusInfoCode = (MCastInfoCodeType) pPSMCastEventInfo->info_code;
    }
    else
    {
      regStateToUpdate->infoCode = (MCastInfoCodeType) pPSMCastEventInfo->info_code;
    }

    /*-----------------------------------------------------------------------
      Default back to Handle::Process()
    -----------------------------------------------------------------------*/
    return Handle::Process(userDataPtr);
  }

  return TRUE;

} /* Process() */

ds::ErrorType MCastSessionPriv::GetSignalBus
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
  case  MCastSessionPrivEvent::QDS_EV_TECHNOLOGY_STATUS_PRIV:
    // TODO This is a temp fix, why is this being called after dtor
    if (0 == mpSigBusTechStatus)
    {
      *ppISigBus = NULL;
      return QDS_EINVAL;
    }
    *ppISigBus = mpSigBusTechStatus;
    (void)(*ppISigBus)->AddRef();
    return AEE_SUCCESS;

  case  MCastSessionPrivEvent::QDS_EV_REGISTRATION_STATE_PRIV_BCMCS1_0:
    // TODO This is a temp fix, why is this being called after dtor
    if (0 == mpSigBusRegStatus1_0)
    {
      *ppISigBus = NULL;
      return QDS_EINVAL;
    }
    *ppISigBus = mpSigBusRegStatus1_0;
    (void)(*ppISigBus)->AddRef();
    return AEE_SUCCESS;

  case  MCastSessionPrivEvent::QDS_EV_REGISTRATION_STATE_PRIV_BCMCS2_0:
    // TODO This is a temp fix, why is this being called after dtor
    if (0 == mpSigBusRegStatus2_0)
    {
      *ppISigBus = NULL;
      return QDS_EINVAL;
    }
    *ppISigBus = mpSigBusRegStatus2_0;
    (void)(*ppISigBus)->AddRef();
    return AEE_SUCCESS;

  default:
    *ppISigBus = NULL;
    return QDS_EINVAL;
  }

} /* GetSignalBus() */
