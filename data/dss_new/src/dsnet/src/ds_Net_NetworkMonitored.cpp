/*===========================================================================
  FILE: DS_Net_NetworkMonitored.cpp

  OVERVIEW: This file provides implementation of the NetworkMonitored class.

  DEPENDENCIES: None

  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_NetworkMonitored.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-09-13 ea  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/

#include "ds_Net_NetworkMonitored.h"
#include "ds_Net_Platform.h"
#include "ds_Net_Privileges_Def.h"
#include "ds_Net_Conversion.h"
#include "AEEIPrivSet.h"

using namespace ds::Net;
using namespace ds::Error;
using namespace ds::Net::Conversion;
using namespace NetPlatform;

/*===========================================================================

                     PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
/*---------------------------------------------------------------------------
  CONSTRUCTOR
---------------------------------------------------------------------------*/
NetworkMonitored::NetworkMonitored
(
  Policy *argPolicyPtr,
  IPrivSet *privSetPtr
) : Network(argPolicyPtr,privSetPtr,NetworkMode::QDS_MONITORED)
{
} /* NetworkMonitored::NetworkMonitored() */

/*---------------------------------------------------------------------------
  Functions inherited from Network
---------------------------------------------------------------------------*/
ds::ErrorType NetworkMonitored::Stop
(
  void
)
{
  return QDS_EINVAL;
} /* Stop() */

ds::ErrorType NetworkMonitored::BringUpInterface
(
  void
)
{
  return QDS_EINVAL;
} /* BringUpInterface() */


void NetworkMonitored::ProcessIfaceStateEvent
(
  EventInfoType*          eventInfoPtr
)
{
  IfaceStateEnumType      ifaceState;
  NetworkStateType        netState;
  int32                   ifaceHandle;
  int32                   newIfaceHandle = 0;
  NetDownReasonType       lastNetDownReason;
  int32                   result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Ignore IFACE_ROUTEABLE_EV
  -------------------------------------------------------------------------*/
  if ((int32)IFACE_ROUTEABLE_EV == eventInfoPtr->psEventName)
  {
    return;
  }

  ifaceHandle = GetHandle();

  LOG_MSG_FUNCTION_ENTRY("Obj %p, handle 0x%x, iface state event %d", 
                          this, ifaceHandle, eventInfoPtr->psEventName);


  /*-------------------------------------------------------------------------
    Call network platform IOCTL to get the interface state.
  -------------------------------------------------------------------------*/
  (void) IfaceIoctl (eventInfoPtr->handle,
                     IFACE_IOCTL_GET_STATE,
                     &ifaceState);

  /*-------------------------------------------------------------------------
    Map Iface state to network state.
  -------------------------------------------------------------------------*/
  (void) PS2DSIfaceState(ifaceState, &netState);

  /*-------------------------------------------------------------------------
    if some iface is in OPEN_IN_PROGRESS state, we need to check if it fits
    our policy, and if so, we shall associate the monitored network with that 
    iface.
  -------------------------------------------------------------------------*/
  if(NetworkState::QDS_OPEN_IN_PROGRESS == netState)
  {
    result = IfaceLookUpByPolicy (static_cast <IPolicy *>(mpPolicy),
                                   &newIfaceHandle);
    if (AEE_SUCCESS == result || AEE_EWOULDBLOCK == result)
    {
      if (newIfaceHandle != ifaceHandle)
      {
        SetHandle (newIfaceHandle);
      }
    }
    else
    {
      /* ------------------------------------------------------------------
        if route lookup could not find an iface that fits our policy we 
        should not be bound to an iface.
      --------------------------------------------------------------------*/
      SetHandle(0);
    }
  }

  /*  Process the event only if the event occurred on this handle */
  else if(eventInfoPtr->handle != ifaceHandle)
  {
    return;
  }

  (void) GetIfaceState (&netState);

  /*-------------------------------------------------------------------------
    Store the last netdown reason. Don't reset handle since it's a monitored
    network.
  -------------------------------------------------------------------------*/
  if (NetworkState::QDS_CLOSED == netState)
  {
    (void) GetLastNetworkDownReason 
      (
        ifaceHandle, 
        (NetPlatform::NetworkDownReasonType *)&lastNetDownReason
      );

    mpICritSect->Enter();
    mLastNetDownReason = lastNetDownReason;
    mpICritSect->Leave();
  }

  /*-------------------------------------------------------------------------
    Store the last netdown reason and reset handle for iface down event.
  -------------------------------------------------------------------------*/
  Notify (eventInfoPtr->eventName);

} /* ProcessIfaceStateEvent() */
