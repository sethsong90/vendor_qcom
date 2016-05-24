/*===========================================================================
  FILE: DS_Net_NetworkActive.cpp

  OVERVIEW: This file provides implementation of the NetworkActive class.

  DEPENDENCIES: None

  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_NetworkActive.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-09-13 ea  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/

#include "ds_Net_NetworkActive.h"
#include "ds_Net_Platform.h"
#include "ds_Net_Privileges_Def.h"
#include "AEEIPrivSet.h"
#include "ds_Net_INetworkFactory.h"
#include "ps_policy_mgr.h"


using namespace ds::Net;
using namespace ds::Error;
using namespace NetPlatform;

/*===========================================================================

                     PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
/*---------------------------------------------------------------------------
  CONSTRUCTOR
---------------------------------------------------------------------------*/
NetworkActive::NetworkActive
(
  Policy *argPolicyPtr,
  IPrivSet *privSetPtr
) : Network(argPolicyPtr,privSetPtr,NetworkMode::QDS_ACTIVE)
{
} /* NetworkActive::NetworkActive() */

/*---------------------------------------------------------------------------
  Functions inherited from Network
---------------------------------------------------------------------------*/
ds::ErrorType NetworkActive::Stop
(
  void
)
{
  int32                     ifaceHandle;
  int32                     result;
  boolean                   isBringupPerformed;
  boolean                   isTeardownPerformed;
  NetworkStateType          prevState;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ifaceHandle = GetHandle();
  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, ifaceHandle, 0);

  /*-------------------------------------------------------------------------
    check client provided privileges
  -------------------------------------------------------------------------*/
  if (AEE_SUCCESS != mpPrivSet->CheckPrivileges((AEEPRIVID *)&AEEPRIVID_PNet,
                                                 sizeof(AEEPRIVID_PNet)))
  {
    LOG_MSG_ERROR( "No privilege for Stop operation", 0, 0, 0);
    return AEE_EPRIVLEVEL;
  }

  mpICritSect->Enter();
  isBringupPerformed = mBringupFirst;
  isTeardownPerformed = mTeardown;
  mTeardown = TRUE;
  mpICritSect->Leave();

  if (FALSE == isBringupPerformed)
  {
    LOG_MSG_INFO1 ("Iface 0x%x not brought by obj %p", ifaceHandle, this, 0);
    return AEE_SUCCESS;
  }

  /*-------------------------------------------------------------------------
    If net handle associated with this object is NULL, or if teardown
    has been called already, return SUCCESS
  -------------------------------------------------------------------------*/
  if (0 == ifaceHandle || TRUE == isTeardownPerformed)
  {
    LOG_MSG_INFO1 ("NULL handle or teardown already called", 0, 0, 0);
    return AEE_SUCCESS;
  }

  /*-------------------------------------------------------------------------
    Call tear_down_cmd on the network interface.
  -------------------------------------------------------------------------*/
  result = IfaceTearDownCmdEx (ifaceHandle,
                               mAppPriority,
                               NULL);
  if (AEE_SUCCESS == result)
  {
    /*-----------------------------------------------------------------------
      Cache previous state, reset handle and notify application. 
    -----------------------------------------------------------------------*/
    GetPreviousState (&prevState);

    mpICritSect->Enter();
    mCachedPreviousState = prevState;
    mpICritSect->Leave();

    SetHandle (0);
    Notify (NetworkEvent::QDS_EV_STATE_CHANGED);
  }

  /*-------------------------------------------------------------------------
    Return SUCCESS always.
  -------------------------------------------------------------------------*/
  return AEE_SUCCESS;

} /* Stop() */

ds::ErrorType NetworkActive::BringUpInterface
(
  void
)
{
  int32             result;
  int32             ifaceHandle;
  NetworkStateType  netState;
  boolean           isBringupPerformed;
  boolean           bringupAgain;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ifaceHandle = GetHandle();
  LOG_MSG_INFO1 ("Obj 0x%p, if handle 0x%x", this, ifaceHandle, 0);

  /*-------------------------------------------------------------------------
    Bringup can be performed only once. For repeat calls, return error code
    depending upon current iface state. Retrieve the object variable using 
    object's cirtical section.
  -------------------------------------------------------------------------*/
  mpICritSect->Enter();
  isBringupPerformed = mBringupFirst;
  bringupAgain       = mBringupAgain;
  mpICritSect->Leave();

  if (TRUE == isBringupPerformed)
  {
    (void) GetIfaceState (&netState);

    switch (netState)
    {
      case NetworkState::QDS_CLOSE_IN_PROGRESS:
        if (TRUE == bringupAgain)
        {
          result = AEE_EWOULDBLOCK;
        }
        else
        {
          result = QDS_ENETCLOSEINPROGRESS;
        }
        break;

      case NetworkState::QDS_CLOSED:
        result = QDS_ENETDOWN;
        break;

      case NetworkState::QDS_OPEN:
      case NetworkState::QDS_LINGERING:
        result = AEE_SUCCESS;
        break;

      case NetworkState::QDS_OPEN_IN_PROGRESS:
        result = AEE_EWOULDBLOCK;
        break;

      default:
        result = QDS_EINVAL;
        break;
    } /* switch */

    LOG_MSG_FUNCTION_EXIT ("Obj 0x%p, handle 0x%x, bringup already performed," 
                           "return %d", this, ifaceHandle, result);
    return result;

  } /* if (TRUE == isBringupPerformed) */


  /*-------------------------------------------------------------------------
    Set the first-time-bring-up flag to true. Reset the mTeardown flag, so
    network can be brought down again
  -------------------------------------------------------------------------*/
  mpICritSect->Enter();
  mBringupFirst = TRUE;
  mTeardown = FALSE;
  mpICritSect->Leave();

  /*-------------------------------------------------------------------------
    Perform Bringup.
  -------------------------------------------------------------------------*/
  result = IfaceBringUpByPolicy (static_cast <IPolicy *>(mpPolicy), 
                                 &ifaceHandle,
                                 &mAppPriority);

  if (QDS_ENETCLOSEINPROGRESS == result ||
      AEE_EWOULDBLOCK == result ||
      AEE_SUCCESS == result)
  {
    /*-----------------------------------------------------------------------
      Success case
    -----------------------------------------------------------------------*/
    LOG_MSG_INFO1("Bring up, object 0x%p, obtained handle 0x%x",
                  this, ifaceHandle, 0);

    mpICritSect->Enter();
  
    SetHandle(ifaceHandle);
    
    /*-----------------------------------------------------------------------
      QDS_ENETCLOSEINPROGRESS needs to be translated to AEE_EWOULDBLOCK.
      We need to bring up the interface after iface down.
    -----------------------------------------------------------------------*/
    if (QDS_ENETCLOSEINPROGRESS == result)
    {
      result = AEE_EWOULDBLOCK;
      mBringupAgain = TRUE;
    }

    mpICritSect->Leave();

    /*-------------------------------------------------------------------------
      Notify if iface is already UP (should be outside crit sect)
    -------------------------------------------------------------------------*/
    (void) GetIfaceState (&netState);
    if (NetworkState::QDS_OPEN == netState || 
        NetworkState::QDS_CLOSED == netState)
    {
      Notify (NetworkEvent::QDS_EV_STATE_CHANGED);
    }

  }
  else
  {
    /*-----------------------------------------------------------------------
      Failure case
    -----------------------------------------------------------------------*/
    LOG_MSG_INFO1("Failed to bring up iface, obj 0x%p", this, 0, 0);

    mpICritSect->Enter();
    mBringupFirst = FALSE;
    mpICritSect->Leave();

  } 
  

  return result;

} /* BringUpInterface() */

void NetworkActive::ProcessIfaceStateEvent
(
  EventInfoType*          eventInfoPtr
)
{
  NetworkStateType        netState;
  NetworkStateType        prevState;
  boolean                 bringupAgain;
  int32                   ifaceHandle;
  int32                   newIfaceHandle = 0;
  NetDownReasonType       lastNetDownReason;
  int32                   result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ifaceHandle = GetHandle();

  /*-------------------------------------------------------------------------
    Process the event only if the event occurred on this handle 
  -------------------------------------------------------------------------*/
  if (ifaceHandle != eventInfoPtr->handle)
  {
    return;
  }

  LOG_MSG_FUNCTION_ENTRY("Obj %p, handle 0x%x, iface state event %d", 
                          this, ifaceHandle, eventInfoPtr->psEventName);

  /*-------------------------------------------------------------------------
    Cache the previous state for any iface state change notifications.
  -------------------------------------------------------------------------*/
  (void) GetIfaceState (&netState);
  (void) GetPreviousState (&prevState);

  mpICritSect->Enter();
  mCachedPreviousState = prevState;
  bringupAgain         = mBringupAgain;
  mpICritSect->Leave();

  /*-------------------------------------------------------------------------
    Ignore IFACE_ROUTEABLE_EV always.
  -------------------------------------------------------------------------*/
  if ((int32)IFACE_ROUTEABLE_EV == eventInfoPtr->psEventName)
  {
    return;
  }

  /*-------------------------------------------------------------------------
    Ignore CLOSE_IN_PROGRESS event when bringupAgain flag is set.
  -------------------------------------------------------------------------*/
  if (TRUE == bringupAgain &&
      NetworkState::QDS_CLOSE_IN_PROGRESS == netState)
  {
    return;
  }

  /*-------------------------------------------------------------------------
    BringupFlag processing.
    Reset bringup flag on CLOSE event.
    Ignore (Dont post event to app) for QDS_CLOSED and CLOSE_IN_PROG events.
  -------------------------------------------------------------------------*/
  if (TRUE == bringupAgain &&
      NetworkState::QDS_CLOSED == netState)
  {

    mpICritSect->Enter();
    mBringupAgain = FALSE;
    mTeardown = FALSE;
    mpICritSect->Leave();

    /*-----------------------------------------------------------------------
      Bringup network again based on policy. If same network cannot be
      brought up, the tear it down and set handle to 0. Reset the mTeardown
      flag, so network can be brought down again

                             IfaceBringUpByPolicy()
                            /                       \
                        SUCESS                     FAILURE
                  /               \                       \
          ifaceHandle ==       ifaceHandle !=         1.SetHandle(0);
          newIfaceHandle       newIfaceHandle         2.Notify(STATE);

          1.NO-OP, return;    1.SetHandle(0);
                              2.Notify(STATE);
                              3.IfaceTearDownCmd(newHandle)

    -----------------------------------------------------------------------*/
    result = IfaceBringUpByPolicy (static_cast <IPolicy *>(mpPolicy),
                                   &newIfaceHandle,
                                   &mAppPriority);

    if (AEE_SUCCESS == result || AEE_EWOULDBLOCK == result)
    {
      if (newIfaceHandle != ifaceHandle)
      {
        SetHandle (0);
        Notify (NetworkEvent::QDS_EV_STATE_CHANGED);
        (void) IfaceTearDownCmdEx (newIfaceHandle, mAppPriority, NULL);
      }
    }
    else
    {
      SetHandle(0);
      Notify (NetworkEvent::QDS_EV_STATE_CHANGED);
    }

    return;
  }


  /*-------------------------------------------------------------------------
    Store the last netdown reason and reset handle for iface down event.
  -------------------------------------------------------------------------*/
  if (NetworkState::QDS_CLOSED == netState)
  {
    (void) GetLastNetworkDownReason 
      (
        ifaceHandle, 
        (NetPlatform::NetworkDownReasonType *) &lastNetDownReason
      );

    mpICritSect->Enter();
    mLastNetDownReason = lastNetDownReason;
    mpICritSect->Leave();

    SetHandle (0);
  }

  /*-------------------------------------------------------------------------
    Notify event to application.
  -------------------------------------------------------------------------*/
  Notify (eventInfoPtr->eventName);

} /* ProcessIfaceStateEvent() */

