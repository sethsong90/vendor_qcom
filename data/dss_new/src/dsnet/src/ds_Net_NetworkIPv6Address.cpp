/*===========================================================================
  FILE: NetworkIPv6Address.cpp

  OVERVIEW: This file provides implementation of the NetworkIPv6Address class

  DEPENDENCIES: None

  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_NetworkIPv6Address.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2009-02-27 hm  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "ds_Utils_DebugMsg.h"
#include "AEEStdErr.h"
#include "ds_Net_NetworkIPv6Address.h"
#include "ds_Net_Conversion.h"
#include "ds_Net_MemManager.h"
#include "ds_Net_EventDefs.h"
#include "ds_Net_EventManager.h"
#include "ds_Net_Platform.h"
#include "ds_Utils_CreateInstance.h"
#include "AEECSignalBus.h"

using namespace ds::Net::Conversion;
using namespace ds::Net;
using namespace ds::Error;
using namespace NetPlatform;


/*===========================================================================

                     PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
/*---------------------------------------------------------------------------
  CONSTRUCTOR/DESTRUCTOR
---------------------------------------------------------------------------*/

NetworkIPv6Address::NetworkIPv6Address
(
  const ds::INAddr6Type      ipv6Addr,
  IPv6AddrStateType          ipv6AddrState,
  int32                      ifaceHandle
)
: Handle (),
  mState (ipv6AddrState),
  mpSigBusStateChange (0),
  refCnt (1),
  weakRefCnt (1)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Object 0x%p, state %d, if handle 0x%x",
    this, mState, ifaceHandle);

  if (0 == ifaceHandle)
  {
     ASSERT (0);
    return;
  }

  (void) memcpy (mIPv6Addr, ipv6Addr, sizeof (ds::INAddr6Type));
  SetHandle (ifaceHandle);

} /* NetworkIPv6Address() */

ds::ErrorType NetworkIPv6Address::Init
(
  void
)
{
  int32  result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Init object 0x%p", this, 0, 0);

  result = DS_Utils_CreateInstance(0, AEECLSID_CSignalBus,
                                   0, (void **) &mpSigBusStateChange);

  Handle::Init(EventManager::networkIPv6ObjList);

  return result;

} /* NetworkIPv6Address::Init() */


void NetworkIPv6Address::Destructor
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);

  DS_UTILS_RELEASEIF (mpSigBusStateChange);

  //Should be last statement. Call destructor for the base class.
  Handle::Destructor();

} /* NetworkIPv6Address::Destructor() */


NetworkIPv6Address::~NetworkIPv6Address
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  //NO-OP: only used for freeing memory.

} /* NetworkIPv6Address::~NetworkIPv6Address() */


/*---------------------------------------------------------------------------
  Handle class forwarders.
---------------------------------------------------------------------------*/
ds::ErrorType NetworkIPv6Address::OnStateChange
(
  ISignal*                                signalObj,
  ds::Net::EventType                      eventID,
  IQI**                                   regObj
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1  ("Obj 0x%p, ev %d, sig 0x%p",
    this, eventID, signalObj);

  if (IPv6AddrEvent::QDS_EV_STATE_CHANGED == eventID)
  {
    return Handle::OnStateChange (signalObj, eventID, regObj);
  }

  return QDS_EOPNOTSUPP;

} /* OnStateChange() */


/*---------------------------------------------------------------------------
  INode interface Methods
---------------------------------------------------------------------------*/
boolean NetworkIPv6Address::Process
(
  void*                                   userDataPtr
)
{
  EventInfoType*                          eventInfoPtr;
  IPv6PrivAddrEventInfoType*              pIPv6PrivAddrEventInfo = NULL;
  int32                                   ifaceHandle;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ifaceHandle = GetHandle();

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p, if handle 0x%x", this, ifaceHandle, 0);

  if (NULL == userDataPtr)
  {
    LOG_MSG_ERROR ("NULL ev info", 0, 0, 0);
    return FALSE;
  }

  if (0 == ifaceHandle)
  {
    LOG_MSG_ERROR ("NULL iface handle", 0, 0, 0);
    ASSERT (0);
    return FALSE;
  }

  eventInfoPtr = static_cast <EventInfoType *>(userDataPtr);

  LOG_MSG_INFO1 ("ev name %d, ev handle 0x%x, this if handle 0x%x",
    eventInfoPtr->eventName, eventInfoPtr->handle, ifaceHandle);

  /*-------------------------------------------------------------------------
    Processing for IPv6AddrEvent::STATE_CHANGED.
  -------------------------------------------------------------------------*/
  if (IPv6AddrEvent::QDS_EV_STATE_CHANGED == eventInfoPtr->eventName)
  {
    pIPv6PrivAddrEventInfo =
      (IPv6PrivAddrEventInfoType *) eventInfoPtr->psEventInfo;
    if (NULL == pIPv6PrivAddrEventInfo)
    {
      return FALSE;
    }

    /*-----------------------------------------------------------------------
      Process this event:
        1. For tentative objects (object's state is PRIV_ADDR_WAITING):
           a. This object's address is IN6_ANY_ADDR
           b. This object's handle matches the event handle.
           c. The event is IFACE_IPV6_PRIV_ADDR_GENERATED_EV.
        2. For other objects
           a. It belongs to current IPv6 address object
      Processing of this event is same for shared and unique IPv6 addresses.
    -----------------------------------------------------------------------*/
    if (IPv6AddrState::PRIV_ADDR_WAITING == mState)
    {
      if((uint32)IFACE_IPV6_PRIV_ADDR_GENERATED_EV ==
          eventInfoPtr->psEventName                &&
          eventInfoPtr->handle == ifaceHandle      &&
          PS_IN6_ARE_ADDR_EQUAL (mIPv6Addr, &ps_in6addr_any))
      {

        /*-------------------------------------------------------------------
          Update the state of object before notifying.
        -------------------------------------------------------------------*/
        mState = IPv6AddrState::PRIV_ADDR_AVAILABLE;
        (void) memcpy(mIPv6Addr,
                      &pIPv6PrivAddrEventInfo->ip_addr.addr.v6,
                      sizeof (ds::INAddr6Type));
        Notify (eventInfoPtr->eventName);
      }

      /*---------------------------------------------------------------------
        Update the state of object before notifying.
      ---------------------------------------------------------------------*/
      /* This address object has consumed the event, stop traversal */
      return FALSE;
    } /* if (IPv6AddrState::PRIV_ADDR_WAITING == mState) */

    /*-----------------------------------------------------------------------
      The event and handle should match to continue processing
    -----------------------------------------------------------------------*/
    if (eventInfoPtr->handle != ifaceHandle ||
        !PS_IN6_ARE_ADDR_EQUAL (&pIPv6PrivAddrEventInfo->ip_addr.addr.v6,
                              mIPv6Addr))
    {
      return TRUE;
    }

    switch (eventInfoPtr->psEventName)
    {
      case IFACE_IPV6_PRIV_ADDR_GENERATED_EV:
        /* Do not notify, as this event is valid for only TENTATIVE objs */
        return TRUE;

      case IFACE_IPV6_PRIV_ADDR_DEPRECATED_EV:
        mState = IPv6AddrState::PRIV_ADDR_DEPRECATED;
        break;

      case IFACE_IPV6_PRIV_ADDR_EXPIRED_EV:
        mState = IPv6AddrState::PRIV_ADDR_WAITING;
        break;

      case IFACE_IPV6_PRIV_ADDR_DELETED_EV:
        mState = IPv6AddrState::PRIV_ADDR_DELETED;
        break;

      default:
        ASSERT (0);
        return FALSE;
    } /* switch */

    /*-------------------------------------------------------------------------
      Notify the app.
    -------------------------------------------------------------------------*/
    (void) Notify (eventInfoPtr->eventName);

  } /* STATE_CHANGED event */


  return FALSE;
} /* Process() */

ds::ErrorType NetworkIPv6Address::GetAddress
(
  ds::INAddr6Type ipv6Addr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  memcpy (ipv6Addr, mIPv6Addr, sizeof (ds::INAddr6Type));
  return AEE_SUCCESS;

} /* GetAddress() */

ds::ErrorType NetworkIPv6Address::GetState
(
  IPv6AddrStateType* pState
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (NULL == pState)
  {
    return QDS_EFAULT;
  }

  *pState = mState;
  return AEE_SUCCESS;

} /* GetState() */


ds::ErrorType NetworkIPv6Address::GetSignalBus
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
    case  IPv6AddrEvent::QDS_EV_STATE_CHANGED:
      *ppISigBus = mpSigBusStateChange;
      (void)(*ppISigBus)->AddRef();
      return AEE_SUCCESS;

    default:
      *ppISigBus = NULL;
      return QDS_EINVAL;
  }

} /* GetSignalBus() */

