/*===========================================================================
  FILE: ds_Net_EventManager.cpp

  OVERVIEW: This file provides implementation EventManager class.

  DEPENDENCIES: None

  Copyright (c) 2007-2011 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/


/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_EventManager.cpp#2 $
  $DateTime: 2011/06/30 17:39:13 $$Author: brijeshd $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-05-02 hm  Created module.

===========================================================================*/

/*---------------------------------------------------------------------------
  Include Files
---------------------------------------------------------------------------*/
#include "comdef.h"

extern "C"
{
#include "ps_svc.h"
}

#include "AEEStdErr.h"
#include "ps_system_heap.h"
#include "ds_Utils_CSSupport.h"
#include "ds_Utils_DebugMsg.h"

#include "ds_Net_EventManager.h"
#include "ds_Net_EventDefs.h"
#include "ds_Net_Platform.h"
#include "ds_Net_TechUMTSFactory.h"
#include "ds_Net_NetworkFactory.h"
#include "ds_Net_Conversion.h"

using namespace ds::Net;
using namespace ds::Error;
using namespace ds::Utils;
using namespace NetPlatform;

/*===========================================================================

                        PRIVATE DATA DEFINITIONS

===========================================================================*/
/*---------------------------------------------------------------------------
  Definition of static members of EventManager class.
---------------------------------------------------------------------------*/
EventManager*  EventManager::instance             = NULL;
Factory*       EventManager::networkObjList       = NULL;
Factory*       EventManager::qosObjList           = NULL;
Factory*       EventManager::physLinkObjList      = NULL;
Factory*       EventManager::mcastObjList         = NULL;
Factory*       EventManager::networkMBMSObjList   = NULL;
Factory*       EventManager::networkIPv6ObjList   = NULL;
Factory*       EventManager::network1XObjList     = NULL;
Factory*       EventManager::ipFilterObjList      = NULL;
Factory*       EventManager::mtpdObjList          = NULL;

/*---------------------------------------------------------------------------
  DSNET Process Event Cmd data type.
---------------------------------------------------------------------------*/
typedef struct
{
  int32                     handle;
  int32                     eventName;
  IfaceEventInfoUnionType   eventInfo;
} ProcessEventCmdDataType;

/*===========================================================================

                        PRIVATE FUNCTION DEFINITIONS

===========================================================================*/
EventManager::EventManager
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Creating EventManager obj 0x%p", this, 0, 0);

  networkObjList       = new Factory();
  qosObjList           = new Factory();
  physLinkObjList      = new Factory();
  mcastObjList         = new Factory();
  networkMBMSObjList   = new Factory();
  networkIPv6ObjList   = new Factory();
  network1XObjList     = new Factory();
  ipFilterObjList      = new Factory();
  mtpdObjList          = new Factory();

} /* EventManager() */

EventManager::~EventManager
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting Event Manager 0x%p", this, 0, 0);

  delete networkObjList;
  delete qosObjList;
  delete physLinkObjList;
  delete mcastObjList;
  delete networkMBMSObjList;
  delete networkIPv6ObjList;
  delete network1XObjList;
  delete ipFilterObjList;
  delete mtpdObjList;

} /* ~EventManager() */

static void ProcessEventCmdHandler
(
  ps_cmd_enum_type    cmd,
  void *              pUserData
)
{
  EventInfoType                              eventInfo;
  EventGroupType                             dsEventGroup = EVENT_GROUP_INVALID;
  int32                                      dsEventName = -1;
  int32                                      userHandle = 0;
  void *                                     pPSEventInfo = NULL;
  static IPv6PrivAddrEventInfoType           ipv6PrivAddrEventInfo;
  static IPv6PrefixUpdatedEventInfoType      ipv6PrefixUpdatedEventInfo;
  static ps_iface_mcast_event_info_type      mcastEventInfo;
  IfaceEventInfoUnionType *                  pEventInfo;
  int32                                      result;
  ProcessEventCmdDataType *                  pProcessEventCmdData;
  int32                                      psEventName;
  int32                                      handle;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/


  if (NULL == pUserData || cmd != PS_DSNET_PROCESS_GENERIC_EVENT_CMD)
  {
    LOG_MSG_ERROR( "Invalid args: cmd %d, event info %p",
                    cmd, pUserData, 0);
    ASSERT (0);
    return;
  }

  pProcessEventCmdData = (ProcessEventCmdDataType *) pUserData;

  psEventName = pProcessEventCmdData->eventName;
  handle      = pProcessEventCmdData->handle;
  pEventInfo  = &(pProcessEventCmdData->eventInfo);

  /*-------------------------------------------------------------------------
    Convert ps_iface_event_enum_type to appropriate ds event name and group.
  -------------------------------------------------------------------------*/
  result = ds::Net::Conversion::PS2DSEventInfo (psEventName,
                                                pEventInfo,
                                                &dsEventName,
                                                &userHandle,
                                                &dsEventGroup);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Special cases for IPv6/MCAST. TODO: FIXME
    Remove this special case handling.
  -------------------------------------------------------------------------*/
  switch (psEventName)
  {
    case IFACE_PREFIX_UPDATE_EV:
      memcpy (&ipv6PrefixUpdatedEventInfo,
              &pEventInfo->prefix_info,
              sizeof (ipv6PrefixUpdatedEventInfo));
      pPSEventInfo = (void *) &ipv6PrefixUpdatedEventInfo;
      break;

    case IFACE_IPV6_PRIV_ADDR_GENERATED_EV:
    case IFACE_IPV6_PRIV_ADDR_DEPRECATED_EV:
    case IFACE_IPV6_PRIV_ADDR_EXPIRED_EV:
    case IFACE_IPV6_PRIV_ADDR_DELETED_EV:
      memcpy (&ipv6PrivAddrEventInfo,
              &pEventInfo->priv_ipv6_addr,
              sizeof (ipv6PrivAddrEventInfo));

      pPSEventInfo = (void *) &ipv6PrivAddrEventInfo;
      break;


    case IFACE_MCAST_REGISTER_SUCCESS_EV:
    case IFACE_MCAST_REGISTER_FAILURE_EV:
    case IFACE_MCAST_DEREGISTERED_EV:
    case IFACE_MCAST_STATUS_EV:
      memcpy (&mcastEventInfo,
              &pEventInfo->mcast_info,
              sizeof (mcastEventInfo));
      pPSEventInfo = (void *) &mcastEventInfo;
      break;
	  
    case IFACE_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_FAILURE_EV:
      pPSEventInfo = 
        (void *)pEventInfo->slotted_mode_info.hdr_slotted_mode_failure_code;
      break;

    case IFACE_ENABLE_HDR_REV0_RATE_INERTIA_FAILURE_EV:
      pPSEventInfo = (void *) pEventInfo->hdr_rev0_rate_inertia_failure_code;
      break;
	
    default:
       break;

  } /* special handling for some IPV6 events */


  /*-------------------------------------------------------------------------
    Construct the event info data structure.
  -------------------------------------------------------------------------*/
  eventInfo.eventName   = dsEventName;
  eventInfo.userHandle  = userHandle;
  eventInfo.eventGroup  = dsEventGroup;
  eventInfo.handle      = handle;
  //TODO: Remove psEventName from here.
  eventInfo.psEventName = psEventName;
  eventInfo.psEventInfo = pPSEventInfo;

  /*-------------------------------------------------------------------------
    Traverse the list of DSNet objects depending upon the event group.
  -------------------------------------------------------------------------*/
  switch (dsEventGroup)
  {
    case EVENT_GROUP_NETWORK:
      (void) EventManager::networkObjList->Traverse ((void *) &eventInfo);
      break;
    case EVENT_GROUP_QOS:
      (void) EventManager::qosObjList->Traverse ((void *) &eventInfo);
      break;
    case EVENT_GROUP_PHYS_LINK:
      (void) EventManager::physLinkObjList->Traverse ((void *) &eventInfo);
      break;
    case EVENT_GROUP_MCAST:
      (void) EventManager::mcastObjList->Traverse ((void *) &eventInfo);
      break;
    case EVENT_GROUP_NETWORK_MBMS:
      (void) EventManager::networkMBMSObjList->Traverse ((void *) &eventInfo);
      break;
    case EVENT_GROUP_NETWORK_IPV6:
      (void) EventManager::networkIPv6ObjList->Traverse ((void *) &eventInfo);
      break;
    case EVENT_GROUP_NETWORK_1X:
      (void) EventManager::network1XObjList->Traverse ((void *) &eventInfo);
      break;
    case EVENT_GROUP_IP_FLTR:
      (void) EventManager::ipFilterObjList->Traverse ((void *) &eventInfo);
      break;
    case EVENT_GROUP_MTPD:
      (void) EventManager::mtpdObjList->Traverse ((void *) &eventInfo);
      break;

    default:
       break;
  } /* switch (dsEventGroup) */

  LOG_MSG_FUNCTION_EXIT ("SUCCESS", 0, 0, 0);
  /* Fall-through */

bail:
  PS_SYSTEM_HEAP_MEM_FREE (pUserData);
  return;

} /* ProcessEventCmdHandler() */

void EventManager::ProcessEvent
(
  int32                       handle,
  int32                       psEventName,
  IfaceEventInfoUnionType     argEventInfo,
  void *                      pUserData
)
{

  ProcessEventCmdDataType *   pProcessEventCmdData;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

//#if 0
  LOG_MSG_FUNCTION_ENTRY ("ProcessEvent:Received event: %d, handle: 0x%x",
                          psEventName, handle, 0);
//#endif

  pProcessEventCmdData =
    (ProcessEventCmdDataType *)
      ps_system_heap_mem_alloc (sizeof (ProcessEventCmdDataType));
  if (NULL == pProcessEventCmdData)
  {
    LOG_MSG_ERROR ("Out of memory, cannot alloc buf for processing ev %d",
                   psEventName, 0, 0);
    return;
  }

  pProcessEventCmdData->handle    = handle;
  pProcessEventCmdData->eventName = psEventName;
  memcpy (&pProcessEventCmdData->eventInfo,
          &argEventInfo,
          sizeof (IfaceEventInfoUnionType));

  /*-------------------------------------------------------------------------
    Send the command to process in PS task.
  -------------------------------------------------------------------------*/
  ps_send_cmd (PS_DSNET_PROCESS_GENERIC_EVENT_CMD,
               (void*) pProcessEventCmdData);

//#if 0
  LOG_MSG_FUNCTION_EXIT ("SUCCESS", 0, 0, 0);
//#endif

  return;

} /* ProcessEvent() */

/*===========================================================================

                        PUBLIC FUNCTION DEFINITIONS

===========================================================================*/

EventManager* EventManager::Instance
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return instance;

} /* Instance() */

void EventManager::Init
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Create an instance of EventManager singleton.
  -------------------------------------------------------------------------*/
  instance = new EventManager();


  /*-------------------------------------------------------------------------
    Register command handler for processing all events.
  -------------------------------------------------------------------------*/
  (void) ps_set_cmd_handler (PS_DSNET_PROCESS_GENERIC_EVENT_CMD,
                             ProcessEventCmdHandler);

  /*-------------------------------------------------------------------------
    Register callback functions for events.
  -------------------------------------------------------------------------*/
  (void) RegIfaceCbackFcn(EventManager::ProcessEvent, NULL);
  (void) RegFlowCbackFcn(EventManager::ProcessEvent, NULL);
  (void) RegPhysLinkCbackFcn(EventManager::ProcessEvent, NULL);


} /* Init() */


void EventManager::Deinit
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Delete the EventManager singleton.
  -------------------------------------------------------------------------*/
  delete instance;
  instance = NULL;

  /*-------------------------------------------------------------------------
    Deregister callback functions for events.
  -------------------------------------------------------------------------*/
  // TODO not allowed at this point, should be
  //(void) RegIfaceCbackFcn(NULL, NULL);
  //(void) RegFlowCbackFcn(NULL, NULL);
  //(void) RegPhysLinkCbackFcn(NULL, NULL);

} /* Denit() */


