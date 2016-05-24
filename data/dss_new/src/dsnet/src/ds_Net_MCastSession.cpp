/*===========================================================================
  FILE: MCastSession.cpp

  OVERVIEW: This file provides implementation of the MCastSession class.

  DEPENDENCIES: None

  Copyright (c) 2007 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_MCastSession.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-03-25 hm  Created module.

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "ds_Utils_DebugMsg.h"
#include "AEEStdErr.h"
#include "ds_Net_MCastSession.h"
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
/*---------------------------------------------------------------------------
  CONSTRUCTOR/DESTRUCTOR
---------------------------------------------------------------------------*/

/**
  @brief      This is the constructor for the MCastSession class.

              This function is called to create objects of type MCastSession.

  @param[in]  ifaceHandle - Identifies the iface over which this
              mcast session is created.
  @param[in]  mcastHandle - Identifies the mcast session.

  @see        No external dependencies

  @return     Returns the MCastSession object created.
*/
MCastSession::MCastSession
(
  int32          ifaceHandle,
  int32          mcastHandle,
  MCastSessionCreationType creationType,
  MCastJoinFlagsType regFlag
)
: Handle (ifaceHandle),
  refCnt (1),
  weakRefCnt (1),
  mMCastHandle (mcastHandle),
  mStatusInfoCode(0),
  mSessionCreationType(creationType),
  mJoinFlag(regFlag)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  memset((void*)&mRegState, 0, sizeof(MCastStateChangedType));

  (void) DS_Utils_CreateInstance (NULL, AEECLSID_CSignalBus,
                                  NULL, (void **)(&mpSigBusRegStatus));
  (void) DS_Utils_CreateInstance (NULL, AEECLSID_CSignalBus,
                                  NULL, (void **)(&mpSigBusTechStatus));

  Handle::Init(EventManager::mcastObjList);

} /* MCastSession() */

void MCastSession::Destructor
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);

  DS_UTILS_RELEASEIF (mpSigBusRegStatus);
  DS_UTILS_RELEASEIF (mpSigBusTechStatus);

  Handle::Destructor();

} /* Destructor() */

MCastSession::~MCastSession
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  //NO-OP: only used for freeing memory.

} /* ~MCastSession() */


/*---------------------------------------------------------------------------
  Inherited functions from IMCastSession.
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
ds::ErrorType MCastSession::Leave
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


ds::ErrorType MCastSession::GetTechStatusInfoCode
(
  MCastInfoCodeType* pInfoCode
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY (
                           "Obj 0x%p, tech status %d",
                           this,
                           mRegState.infoCode,
                           0
                         );

  if (NULL == pInfoCode)
  {
    return QDS_EFAULT;
  }

  *pInfoCode = mStatusInfoCode;
  return AEE_SUCCESS;

}/* GetTechStatusInfoCode() */


ds::ErrorType MCastSession::GetRegStateAndInfoCode
(
  MCastStateChangedType* RegStateAndInfoCode
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY (
                           "Obj 0x%p, reg status %d",
                           this,
                           mRegState.regState,
                           0
                         );

  if (NULL == RegStateAndInfoCode)
  {
    return QDS_EFAULT;
  }

  *RegStateAndInfoCode = mRegState;

  return AEE_SUCCESS;

}/* GetRegStateAndInfoCode() */


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
boolean MCastSession::Process
(
  void* userDataPtr
)
{
  EventInfoType*                      pEventInfo = 0;
  ps_iface_mcast_event_info_type *    pPSMCastEventInfo = 0;
  ds::ErrorType result = AEE_SUCCESS;
  ps_iface_mcast_info_code_enum_type mappedInfoCode;
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
      Set the MCast Registration status depending upon event.
    -----------------------------------------------------------------------*/
    switch (pEventInfo->psEventName)
    {
      case IFACE_MCAST_REGISTER_SUCCESS_EV:
        mRegState.regState = MCastRegState::MCAST_REGISTER_SUCCESS;
        break;

      case IFACE_MCAST_REGISTER_FAILURE_EV:
        mRegState.regState = MCastRegState::MCAST_REGISTER_FAILURE;
        break;

      case IFACE_MCAST_DEREGISTERED_EV:
        mRegState.regState = MCastRegState::MCAST_DEREGISTERED;
        break;
    } /* switch */

    /*-----------------------------------------------------------------------
      Set the MCast technology status depending upon event info.
    -----------------------------------------------------------------------*/
    pPSMCastEventInfo = (ps_iface_mcast_event_info_type *)
                        pEventInfo->psEventInfo;
    /*-----------------------------------------------------------------------
      If this MCast session was created by JoinBundle call - only BCMCS 2.0,
      DVBH and MFLO events are fired, otherwise (created by Join call) -
      BCMCS 1.0 are translated to BCMCS 2.0 events, DVBH and MFLO events fired
      as is.
    -----------------------------------------------------------------------*/
    if( MCastSessionCreation::MCAST_SESSION_CREATED_BY_JOIN ==
        mSessionCreationType)
    {
      /* BCMCS 1.0 semantics events are translated */
      if(MCastInfoCodeIsBCMCS1_0(pPSMCastEventInfo->info_code) ||
        (PS_IFACE_MCAST_IC_NOT_SPECIFIED == pPSMCastEventInfo->info_code))
      {
        result = MCastInfoCodeBCMCS1_0To2_0 (
                                              pPSMCastEventInfo->info_code,
                                              &mappedInfoCode
                                            );
        if(AEE_SUCCESS == result)
        {
          if(IFACE_MCAST_STATUS_EV == pEventInfo->psEventName)
          {
            mStatusInfoCode = (MCastInfoCodeType)mappedInfoCode;
          }
          else
          {
          mRegState.infoCode = (MCastInfoCodeType)mappedInfoCode;
        }
        }
        else/* no mapping exists - should not get here*/
        {
          ASSERT(0);
          return TRUE;
        }
      }
      else if(MCastInfoCodeIsDVBHOrMFLO(pPSMCastEventInfo->info_code))
      {
        if(IFACE_MCAST_STATUS_EV == pEventInfo->psEventName)
        {
          mStatusInfoCode = (MCastInfoCodeType) pPSMCastEventInfo->info_code;
        }
        else
        {
        mRegState.infoCode = (MCastInfoCodeType) pPSMCastEventInfo->info_code;
      }
      }
      else/* BCMCS 2.0 events are filtered out */
      {
        LOG_MSG_ERROR ("Info code %d is ver 2.0 and is not for Join API ",
                        pPSMCastEventInfo->info_code, 0, 0);
        return TRUE;
      }
    }
    else/* created by call to JoinBundle()*/
    {
      /* BCMCS 1.0 semantics events are filtered out */
      if(MCastInfoCodeIsBCMCS1_0(pPSMCastEventInfo->info_code) ||
         (PS_IFACE_MCAST_IC_NOT_SPECIFIED == pPSMCastEventInfo->info_code))
      {
        LOG_MSG_INFO1 ("Info code %d is ver 1.0 and is not for Bundle API ",
                        pPSMCastEventInfo->info_code, 0, 0);
        return TRUE;
      }
      else if(MCastInfoCodeIsDVBHOrMFLO(pPSMCastEventInfo->info_code) ||
              PS_IFACE_MCAST_MBMS_SYSTEM_UNAVAILABLE ==
                pPSMCastEventInfo->info_code)
      {
        ASSERT(0);
        LOG_MSG_ERROR ("Info code %d is not expected ",
                       pPSMCastEventInfo->info_code, 0, 0);
        return TRUE;
      }
      else/* BCMCS 2.0 */
      {
        if(IFACE_MCAST_STATUS_EV == pEventInfo->psEventName)
        {
          mStatusInfoCode = (MCastInfoCodeType) pPSMCastEventInfo->info_code;
        }
        else
        {
        mRegState.infoCode = (MCastInfoCodeType) pPSMCastEventInfo->info_code;
      }
    }
    }

    /*-----------------------------------------------------------------------
      Default back to Handle::Process()
    -----------------------------------------------------------------------*/
    return Handle::Process(userDataPtr);
  }

  return TRUE;

} /* Process() */

ds::ErrorType MCastSession::GetSignalBus
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
  case  MCastSessionEvent::QDS_EV_REGISTRATION_STATE:
    // TODO This is a temp fix, why is this being called after dtor
    if (0 == mpSigBusRegStatus)
    {
      *ppISigBus = NULL;
      return QDS_EINVAL;
    }
    *ppISigBus = mpSigBusRegStatus;
    (void)(*ppISigBus)->AddRef();
    return AEE_SUCCESS;

  case  MCastSessionEvent::QDS_EV_TECHNOLOGY_STATUS:
    // TODO This is a temp fix, why is this being called after dtor
    if (0 == mpSigBusTechStatus)
    {
      *ppISigBus = NULL;
      return QDS_EINVAL;
    }
    *ppISigBus = mpSigBusTechStatus;
    (void)(*ppISigBus)->AddRef();
    return AEE_SUCCESS;

  default:
    *ppISigBus = NULL;
    return QDS_EINVAL;
  }

} /* GetSignalBus() */
