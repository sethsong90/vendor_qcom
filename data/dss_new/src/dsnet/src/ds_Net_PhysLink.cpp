/*=========================================================================*/
/*!
  @file
  ds_Net_PhysLink.cpp

  @brief
  This file provides implementation of the PhysLink class.

  Copyright (c) 2008-2011 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*=========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_PhysLink.cpp#2 $
  $DateTime: 2011/07/27 14:52:45 $$Author: hmurari $

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
#include "ds_Net_PhysLink.h"
#include "ds_Net_Platform.h"
#include "ds_Net_EventDefs.h"
#include "ds_Net_EventManager.h"
#include "ds_Net_Conversion.h"
#include "ds_Utils_CreateInstance.h"
#include "AEECSignalBus.h"
#include "ds_Net_Privileges_Def.h"
#include "AEEIPrivSet.h"

using namespace ds::Net;
using namespace ds::Net::Conversion;
using namespace ds::Error;
using namespace NetPlatform;

/*===========================================================================

                     PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
/*---------------------------------------------------------------------------
  CONSTRUCTOR/DESTRUCTOR
---------------------------------------------------------------------------*/
PhysLink::PhysLink
(
  int32 physLinkHandle,
  IPrivSet * pPrivSet,
  NetworkModeType networkMode
)
: Handle(physLinkHandle),
  mpSigBusStateChange(0),
  mpPrivSet(pPrivSet),
  mNetworkMode(networkMode),
  mCachedPreviousState(PhysLinkState::QDS_DOWN),
  refCnt (1),
  weakRefCnt (1)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Object 0x%p, handle 0x%x, mode %d", this, physLinkHandle,
                 mNetworkMode);

  ASSERT(0 != mpPrivSet);

  (void) mpPrivSet->AddRef();

  (void) DS_Utils_CreateInstance(0, AEECLSID_CSignalBus,
                                 0, (void **) &mpSigBusStateChange);

  Handle::Init(EventManager::physLinkObjList);

} /* PhysLink::PhysLink() */

void PhysLink::Destructor
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);

  DS_UTILS_RELEASEIF (mpPrivSet);

  DS_UTILS_RELEASEIF (mpSigBusStateChange);

  //Should be last statement. Call destructor for the base class.
  Handle::Destructor();

} /* PhysLink::Destructor() */

PhysLink::~PhysLink
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  mpPrivSet = NULL;


} /* PhysLink::~PhysLink() */


/*---------------------------------------------------------------------------
  Inherited functions from IPhysLink.
---------------------------------------------------------------------------*/
ds::ErrorType PhysLink::GoActive
(
  void
)
{
  int32                               result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);
  
  if (NetworkMode::QDS_MONITORED == mNetworkMode)
  {
    return AEE_EUNSUPPORTED;
  }

  /*-------------------------------------------------------------------------
    check client provided privileges
  -------------------------------------------------------------------------*/
  if (AEE_SUCCESS != mpPrivSet->CheckPrivileges
                                (
                                  (AEEPRIVID *)&AEEPRIVID_PGoDormant,
                                  sizeof(AEEPRIVID_PGoDormant)
                                ))
  {
    LOG_MSG_ERROR( "No privilege for GoActive operation", 0, 0, 0);
    return AEE_EPRIVLEVEL;
  }

  /*-------------------------------------------------------------------------
    Perform phys link IOCTL to activate phys link
  -------------------------------------------------------------------------*/
  result = PhysLinkIoctl (GetHandle(),
             PHYS_LINK_IOCTL_GO_ACTIVE,
             NULL);
  if (AEE_SUCCESS != result)
  {
    LOG_MSG_ERROR ("Err %d", result, 0, 0);
  }

  return result;

} /* GoActive() */


ds::ErrorType PhysLink::GoDormant
(
  DormantReasonType  dormantReason
)
{
  int32                                result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  /*-------------------------------------------------------------------------
    dormantReason is defined for the future use
  -------------------------------------------------------------------------*/
  (void) dormantReason;

  /*-------------------------------------------------------------------------
    check client provided privileges
  -------------------------------------------------------------------------*/
  if (AEE_SUCCESS != mpPrivSet->CheckPrivileges
                                (
                                  (AEEPRIVID *)&AEEPRIVID_PGoDormant,
                                  sizeof(AEEPRIVID_PGoDormant)
                                ))
  {
    LOG_MSG_ERROR( "No privilege for GoDormant operation", 0, 0, 0);
    return AEE_EPRIVLEVEL;
  }

  /*-------------------------------------------------------------------------
    Perform phys link IOCTL to make phys link go dormant.
  -------------------------------------------------------------------------*/
  result = PhysLinkIoctl (GetHandle(),
             PHYS_LINK_IOCTL_GO_DORMANT,
             NULL);
  if (AEE_SUCCESS != result)
  {
    LOG_MSG_ERROR ("Err %d", result, 0, 0);
  }

  return result;

} /* GoDormant() */


ds::ErrorType PhysLink::GetState
(
 ds::Net::PhysLinkStateType* state
)
{
  int32                 result;
  PSPhysLinkStateType   physLinkState;
  int32                 physLinkHandle;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Validation
  -------------------------------------------------------------------------*/
  if (NULL == state)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  physLinkHandle = GetHandle();
  if (0 == physLinkHandle && 0 != mStaleIfaceHandle)
  {
    physLinkHandle = mStaleIfaceHandle;
    LOG_MSG_INFO2 ("Using stale phys link handle 0x%x", physLinkHandle, 0, 0);
  }

  if (0 == physLinkHandle)
  {
    *state = PhysLinkState::QDS_DOWN;
    return AEE_SUCCESS;
  }

  /*-------------------------------------------------------------------------
    Perform phys link IOCTL to get phys link state.
  -------------------------------------------------------------------------*/
  result = PhysLinkIoctl (physLinkHandle,
                          PHYS_LINK_IOCTL_GET_STATE,
                          &physLinkState);
  if (AEE_SUCCESS != result)
  {
    LOG_MSG_ERROR ("Err %d", result, 0, 0);
    return result;
  }

  *state = PS2DSPhysLinkState (physLinkState);

  LOG_MSG_INFO1  ("Obj 0x%p, handle 0x%x, state %d",
    this, physLinkHandle, *state);
  return result;

} /* GetState() */


ds::ErrorType PhysLink::GetPreviousState
(
  ds::Net::PhysLinkStateType* state
)
{
  int32              result;
  NetPlatform::PSPhysLinkStateType  physLinkState;
  int32              physLinkHandle;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Validation
  -------------------------------------------------------------------------*/
  if (NULL == state)
  {
    result = QDS_EFAULT;
    return QDS_EFAULT;
  }

  physLinkHandle = GetHandle();
  if (0 == physLinkHandle && 0 != mStaleIfaceHandle)
  {
    physLinkHandle = mStaleIfaceHandle;
    LOG_MSG_INFO2 ("Using stale phys link handle 0x%x", physLinkHandle, 0, 0);
  }

  if (0 == physLinkHandle)
  {
    *state = mCachedPreviousState;
    return AEE_SUCCESS;
  }

  /*-------------------------------------------------------------------------
    Perform phys link IOCTL to get phys link state.
  -------------------------------------------------------------------------*/
  result = PhysLinkIoctl (physLinkHandle,
                          PHYS_LINK_IOCTL_GET_PREVIOUS_STATE,
                          &physLinkState);
  if (AEE_SUCCESS != result)
  {
    LOG_MSG_ERROR ("Err %d", result, 0, 0);
    return result;
  }

  *state = PS2DSPhysLinkState (physLinkState);

  LOG_MSG_INFO1  ("Obj 0x%p, handle 0x%x, state %d",
    this, physLinkHandle, *state);
  return AEE_SUCCESS;

} /* GetPreviousState() */

/*---------------------------------------------------------------------------
  Event processing.
---------------------------------------------------------------------------*/
boolean PhysLink::Process
(
  void* pUserData
)
{
  EventInfoType*             pEventInfo;
  PSPhysLinkStateType        physLinkState;
  ps_iface_event_enum_type   expectedPhysLinkEvent;		 
  int32                      result = 0;
  int32                      physLinkHandle;
/*-------------------------------------------------------------------------*/

  /*-------------------------------------------------------------------------
    User data should never be NULL for event processing.
  -------------------------------------------------------------------------*/
  if (NULL == pUserData)
  {
    ASSERT (0);
    return FALSE;
  }

  physLinkHandle = GetHandle();
  if (0 == physLinkHandle && 0 != mStaleIfaceHandle)
  {
    physLinkHandle = mStaleIfaceHandle;
    LOG_MSG_INFO2 ("Using stale phys link handle 0x%x", physLinkHandle, 0, 0);
  }

  pEventInfo = static_cast <EventInfoType *> (pUserData);

  LOG_MSG_INFO1 ("this handle 0x%x, ev 0x%x, ev handle 0x%x", 
    physLinkHandle, pEventInfo->eventName, pEventInfo->handle);  
  
  if (physLinkHandle == pEventInfo->handle)
  {
    /*-----------------------------------------------------------------------
      For PHYS_LINK_GONE_EV event, reset the handle before notifying.
   
      Since Process() is called in a different task context, physlink's 
      state could have chaned by the time, Process() is executed. So, skip 
      processing the event so that STATE_CHANGED_EV is not posted multiple 
      times. Otherwise, the second time the event is posted, current state 
      and previous state point to the same state implying that there is no 
      state change.      
    -----------------------------------------------------------------------*/
    if ((int32)PHYS_LINK_GONE_EV == pEventInfo->psEventName)
    {
      // we need to update the cached previous state, because we won't be able to fetch this info after handle is set to 0
      GetPreviousState(&mCachedPreviousState);
      SetHandle (0);
    }
    else
    {
      result = NetPlatform::PhysLinkIoctl (physLinkHandle,
                                           PHYS_LINK_IOCTL_GET_STATE,
                                           &physLinkState);
      if (AEE_SUCCESS != result)
      {
        LOG_MSG_ERROR ("Err getting phsylink state %d", result, 0, 0);
        return FALSE;
      }
  
      switch (physLinkState)
      {
        case PHYS_LINK_DOWN:
          expectedPhysLinkEvent = PHYS_LINK_DOWN_EV;
          break;
  
        case PHYS_LINK_COMING_UP:
          expectedPhysLinkEvent = PHYS_LINK_COMING_UP_EV;
          break;
  
        case PHYS_LINK_UP:
          expectedPhysLinkEvent = PHYS_LINK_UP_EV;
          break;
  
        case PHYS_LINK_GOING_DOWN:
          expectedPhysLinkEvent = PHYS_LINK_GOING_DOWN_EV;
          break;
  
        case PHYS_LINK_RESUMING:
          expectedPhysLinkEvent = PHYS_LINK_RESUMING_EV;
          break;
  
        case PHYS_LINK_GOING_NULL:
          expectedPhysLinkEvent = PHYS_LINK_GOING_NULL_EV;
          break;
  
        case PHYS_LINK_NULL:
          expectedPhysLinkEvent = PHYS_LINK_GONE_EV;
          break;
  
        default:
          LOG_MSG_ERROR ("Invalid phsylink state %d", result, 0, 0);
          return FALSE;
       }
	  
       /*-----------------------------------------------------------------------
         Do not post the event if the event is different from the current state
       -----------------------------------------------------------------------*/
       if (expectedPhysLinkEvent != pEventInfo->psEventName)
       {
         LOG_MSG_INFO1 ("Expected event %d, got event %d for handle 0x%x", 
                        expectedPhysLinkEvent, pEventInfo->psEventName, 
                        physLinkHandle);

         return TRUE;
       }
     }

     /*-----------------------------------------------------------------------
       Event belongs to this handle. Call Notify() 
     -----------------------------------------------------------------------*/
     Notify (pEventInfo->eventName);    
  }

  return TRUE;
  
} /* Process() */

ds::ErrorType PhysLink::OnStateChange 
(
  ::ISignal*            signalObj,
  ds::Net::EventType    eventID,
  IQI**                 regObj
)
{
  if (NetworkMode::QDS_MONITORED == mNetworkMode)
  {
     if (PhysLinkEvent::QDS_EV_STATE_CHANGED != eventID)
    {
      return AEE_EUNSUPPORTED;
    }
  }
  
  if (PhysLinkEvent::QDS_EV_STATE_CHANGED == eventID)
  {
    /* Set the signal and let APP query for phys link state */
    (void) signalObj->AddRef();
    (void) signalObj->Set();
    (void) signalObj->Release();
  }

  return Handle::OnStateChange(signalObj, eventID, regObj);
} /* OnStateChange() */

ds::ErrorType PhysLink::GetSignalBus
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
    case  PhysLinkEvent::QDS_EV_STATE_CHANGED:
      *ppISigBus = mpSigBusStateChange;
      (void)(*ppISigBus)->AddRef();
      return AEE_SUCCESS;

    default:
      *ppISigBus = NULL;
      return AEE_EBADPARM;
  }

} /* GetSignalBus() */

ds::ErrorType CDECL PhysLink::GetDormancyInfoCode
(
  DormancyInfoCodeType            *pDormancyInfoCode
)
{
  ds::ErrorType                    result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  /*-------------------------------------------------------------------------
    Perform phys link IOCTL to fetch dormancy info-code.
  -------------------------------------------------------------------------*/
  result = PhysLinkIoctl (GetHandle(),
                          PHYS_LINK_IOCTL_GET_DORMANCY_INFO_CODE,
                          (void *) pDormancyInfoCode);
  if (AEE_SUCCESS != result)
  {
    LOG_MSG_ERROR ("Err %d", result, 0, 0);
  }

  return result;

} /* GetDormancyInfoCode() */


