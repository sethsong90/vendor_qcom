/*===========================================================================
  FILE: DS_Sock_EventManager.cpp

  OVERVIEW: This file provides implementation EventManager class.

  DEPENDENCIES: None

  Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/


/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_EventManager.cpp#1 $
  $DateTime: 2009/09/28 14:45:31 $$Author: dmudgal $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-05-02 hm  Created module.

===========================================================================*/

/*===========================================================================

                              INCLUDE FILES

===========================================================================*/
#include "comdef.h"
#include "customer.h"
#include "target.h"

#include "DS_Utils_CSSupport.h"
#include "DS_Utils_DebugMsg.h"
#include "DS_Errors.h"
#include "DS_Sock_EventManager.h"
#include "DS_Sock_EventDefs.h"
#include "DS_Sock_SocketFactory.h"
#include "DS_Sock_SocketFactoryPriv.h"
#include "DS_Sock_Socket.h"

#include "ps_iface.h"
#include "ps_flowi_event.h"
#include "ps_phys_link.h"
#include "ps_mem.h"
#include "ps_rt_meta_info.h"
#include "ps_pkt_meta_info.h"

using namespace DS::Sock;
using namespace DS::Utils;
using namespace PS::Sock;


/*===========================================================================

                        PRIVATE DATA DECLARATIONS

===========================================================================*/
static struct
{
  void *  psFlowActivatedEvCbackBufPtr;
  void *  psFlowSuspendedEvCbackBufPtr;
  void *  psFlowTxEnabledEvCbackBufPtr;
  void *  psFlowTxDisabledEvCbackBufPtr;
  void *  psIfaceIPFltrUpdatedEvCbackBufPtr;
  void *  psIfaceAddrChangedEvCbackBufPtr;
  void *  psIfacePrefixUpdatedEvCbackBufPtr;
  void *  psIfaceAddrFamilyChangedEvCbackBufPtr;
  void *  psIfaceUpEvCbackBufPtr;
  void *  psIfaceRouteableEvCbackBufPtr;
  void *  psIfaceDownEvCbackBufPtr;
  void *  psIfaceGoingDownEvCbackBufPtr;
  void *  psIfaceLingeringEvCbackBufPtr;
  void *  psIfaceConfiguringEvCbackBufPtr;
  void *  psIfaceFlowEnabledEvCbackBufPtr;
  void *  psIfaceFlowDisabledEvCbackBufPtr;
  void *  physLinkUpEvCbackBufPtr;
  void *  physLinkDownEvCbackBufPtr;
  void *  physLinkFlowEnabledEvCbackBufPtr;
  void *  physLinkFlowDisabledEvCbackBufPtr;
  void *  physLinkDoSAckEvCbackBufPtr;
} EventCbackInfo;


/*===========================================================================

                        PRIVATE FUNCTION DEFINITIONS

===========================================================================*/
void PSIfaceEventCback
(
  ps_iface_type *             psIfacePtr,
  ps_iface_event_enum_type    psIfaceEvent,
  ps_iface_event_info_u_type  psIfaceEventInfo,
  void *                      userDataPtr
)
{
  SocketFactory *                 sockFactoryPtr;
  SocketFactoryPriv *             sockFactoryPrivPtr;
  DS::Sock::Event::EventInfoType  sockEventInfo;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  (void)userDataPtr;
  LOG_MSG_FUNCTION_ENTRY( "Ev %d Iface 0x%x", psIfaceEvent, psIfacePtr, 0);

  /*-------------------------------------------------------------------------
    Propagate event only if IPv6 prefix is removed. Socket doesn't care about
    other type of prefix updates
  -------------------------------------------------------------------------*/
  if (IFACE_PREFIX_UPDATE_EV == psIfaceEvent &&
      PREFIX_REMOVED != psIfaceEventInfo.prefix_info.kind)
  {
    LOG_MSG_FUNCTION_EXIT( "Ignoring ev %d kind %d",
                           psIfaceEvent, psIfaceEventInfo.prefix_info.kind, 0);
    return;
  }

  sockEventInfo.eventGroup = DS::Sock::Event::EVENT_GROUP_NETWORK;
  sockEventInfo.eventMask  = psIfaceEvent;
  sockEventInfo.handle     = reinterpret_cast <int32> ( psIfacePtr);

  /*-------------------------------------------------------------------------
    Relay events to all the sockets in socket factory
  -------------------------------------------------------------------------*/
  sockFactoryPtr = SocketFactory::CreateInstance();
  ASSERT( 0 != sockFactoryPtr);

  (void) sockFactoryPtr->Traverse( &sockEventInfo);
  (void) sockFactoryPtr->Release();

  /*-------------------------------------------------------------------------
    Relay events to all the sockets in privileged socket factory
  -------------------------------------------------------------------------*/
  sockFactoryPrivPtr = SocketFactoryPriv::CreateInstance();
  ASSERT( 0 != sockFactoryPrivPtr);

  (void) sockFactoryPrivPtr->Traverse( &sockEventInfo);
  (void) sockFactoryPrivPtr->Release();

  LOG_MSG_FUNCTION_EXIT( "Success", 0, 0, 0);
  return;

} /* PSIfaceEventCback() */


void PSFlowEventCback
(
  ps_flow_type *              psFlowPtr,
  ps_iface_event_enum_type    psFlowEvent,
  ps_iface_event_info_u_type  psFlowEventInfo,
  void *                      userDataPtr
)
{
  SocketFactory *                 sockFactoryPtr;
  SocketFactoryPriv *             sockFactoryPrivPtr;
  DS::Sock::Event::EventInfoType  sockEventInfo;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  (void)psFlowEventInfo;
  (void)userDataPtr;
  LOG_MSG_FUNCTION_ENTRY( "Ev %d PSFlow", psFlowEvent, psFlowPtr, 0);

  sockEventInfo.eventGroup = DS::Sock::Event::EVENT_GROUP_NETWORK;
  sockEventInfo.eventMask  = psFlowEvent;
  sockEventInfo.handle     = reinterpret_cast <int32> ( psFlowPtr);

  /*-------------------------------------------------------------------------
    Relay events to all the sockets in socket factory
  -------------------------------------------------------------------------*/
  sockFactoryPtr = SocketFactory::CreateInstance();
  ASSERT( 0 != sockFactoryPtr);

  (void) sockFactoryPtr->Traverse( &sockEventInfo);
  (void) sockFactoryPtr->Release();

  /*-------------------------------------------------------------------------
    Relay events to all the sockets in privileged socket factory
  -------------------------------------------------------------------------*/
  sockFactoryPrivPtr = SocketFactoryPriv::CreateInstance();
  ASSERT( 0 != sockFactoryPrivPtr);

  (void) sockFactoryPrivPtr->Traverse( &sockEventInfo);
  (void) sockFactoryPrivPtr->Release();

  LOG_MSG_FUNCTION_EXIT( "Success", 0, 0, 0);
  return;
} /* PSFlowEventCback() */


void PhysLinkEventCback
(
  ps_phys_link_type *         physLinkPtr,
  ps_iface_event_enum_type    physLinkEvent,
  ps_iface_event_info_u_type  physLinkEventInfo,
  void *                      userDataPtr
)
{
  SocketFactory *                 sockFactoryPtr;
  SocketFactoryPriv *             sockFactoryPrivPtr;
  DS::Sock::Event::EventInfoType  sockEventInfo;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  (void)physLinkEventInfo;
  (void)userDataPtr;
  LOG_MSG_FUNCTION_ENTRY( "Ev %d PhysLink 0x%x",
                          physLinkEvent, physLinkPtr, 0);

  sockEventInfo.eventGroup = DS::Sock::Event::EVENT_GROUP_NETWORK;
  sockEventInfo.eventMask  = physLinkEvent;
  sockEventInfo.handle     = reinterpret_cast <int32> ( physLinkPtr);

  /*-------------------------------------------------------------------------
    Relay events to all the sockets in socket factory
  -------------------------------------------------------------------------*/
  sockFactoryPtr = SocketFactory::CreateInstance();
  ASSERT( 0 != sockFactoryPtr);

  (void) sockFactoryPtr->Traverse( &sockEventInfo);
  (void) sockFactoryPtr->Release();

  /*-------------------------------------------------------------------------
    Relay events to all the sockets in privileged socket factory
  -------------------------------------------------------------------------*/
  sockFactoryPrivPtr = SocketFactoryPriv::CreateInstance();
  ASSERT( 0 != sockFactoryPrivPtr);

  (void) sockFactoryPrivPtr->Traverse( &sockEventInfo);
  (void) sockFactoryPrivPtr->Release();

  LOG_MSG_FUNCTION_EXIT( "Success", 0, 0, 0);
  return;
} /* PhysLinkEventCback() */


void PhysLinkDoSAckEventCback
(
  ps_phys_link_type *         physLinkPtr,
  ps_iface_event_enum_type    physLinkEvent,
  ps_iface_event_info_u_type  physLinkEventInfo,
  void *                      userDataPtr
)
{
  SocketFactory *                       sockFactoryPtr;
  SocketFactoryPriv *                   sockFactoryPrivPtr;
  DS::Sock::Event::DoSAckEventInfoType  dosAckEventInfo;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  (void)userDataPtr;
  LOG_MSG_FUNCTION_ENTRY( "PhysLink 0x%x", physLinkPtr, 0, 0);

  dosAckEventInfo.eventGroup = DS::Sock::Event::EVENT_GROUP_NETWORK;
  dosAckEventInfo.eventMask  = physLinkEvent;
  dosAckEventInfo.handle     = reinterpret_cast <int32> ( physLinkPtr);

  dosAckEventInfo.dosAckStatus =
    (DoSAckStatusType) physLinkEventInfo.dos_ack_info.status_info.status;

  dosAckEventInfo.overflow =
    physLinkEventInfo.dos_ack_info.status_info.overflow;

  /*-------------------------------------------------------------------------
    Relay events to all the sockets in socket factory
  -------------------------------------------------------------------------*/
  sockFactoryPtr = SocketFactory::CreateInstance();
  ASSERT( 0 != sockFactoryPtr);

  (void) sockFactoryPtr->Traverse( &dosAckEventInfo);
  (void) sockFactoryPtr->Release();

  /*-------------------------------------------------------------------------
    Relay events to all the sockets in privileged socket factory
  -------------------------------------------------------------------------*/
  sockFactoryPrivPtr = SocketFactoryPriv::CreateInstance();
  ASSERT( 0 != sockFactoryPrivPtr);

  (void) sockFactoryPrivPtr->Traverse( &dosAckEventInfo);
  (void) sockFactoryPrivPtr->Release();

  LOG_MSG_FUNCTION_EXIT( "Success", 0, 0, 0);
  return;
} /* PhysLinkDoSAckEventCback() */


void PSMemEventCback
(
  int16  wmVal
)
{
  SocketFactory *                 sockFactoryPtr;
  SocketFactoryPriv *             sockFactoryPrivPtr;
  DS::Sock::Event::EventInfoType  sockEventInfo;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Val %d", wmVal, 0, 0);

  sockEventInfo.eventGroup = DS::Sock::Event::EVENT_GROUP_PS_MEM;
  sockEventInfo.eventMask  = PSMemEvent::PS_MEM_BUF_AVAILABLE;
  sockEventInfo.handle     = 0;

  /*-------------------------------------------------------------------------
    Relay events to all the sockets in socket factory
  -------------------------------------------------------------------------*/
  sockFactoryPtr = SocketFactory::CreateInstance();
  ASSERT( 0 != sockFactoryPtr);

  (void) sockFactoryPtr->Traverse( &sockEventInfo);
  (void) sockFactoryPtr->Release();

  /*-------------------------------------------------------------------------
    Relay events to all the sockets in privileged socket factory
  -------------------------------------------------------------------------*/
  sockFactoryPrivPtr = SocketFactoryPriv::CreateInstance();
  ASSERT( 0 != sockFactoryPrivPtr);

  (void) sockFactoryPrivPtr->Traverse( &sockEventInfo);
  (void) sockFactoryPrivPtr->Release();

  LOG_MSG_FUNCTION_EXIT( "Success", 0, 0, 0);
  return;

} /* PSMemEventCback() */


void DS::Sock::EventManager::Init
(
  void
)
{
  int32  retVal;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Register for IFACE_ADDR_CHANGED_EV with netiface module
  -------------------------------------------------------------------------*/
  EventCbackInfo.psIfaceAddrChangedEvCbackBufPtr =
    ps_iface_alloc_event_cback_buf( PSIfaceEventCback, NULL);

  if (NULL == EventCbackInfo.psIfaceAddrChangedEvCbackBufPtr)
  {
    goto bail;
  }

  retVal =
    ps_iface_event_cback_reg( NULL,
                              IFACE_ADDR_CHANGED_EV,
                              EventCbackInfo.psIfaceAddrChangedEvCbackBufPtr);
  if (0 != retVal)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register for IFACE_ADDR_FAMILY_CHANGED_EV with netiface module
  -------------------------------------------------------------------------*/
  EventCbackInfo.psIfaceAddrFamilyChangedEvCbackBufPtr =
    ps_iface_alloc_event_cback_buf( PSIfaceEventCback, NULL);

  if (NULL == EventCbackInfo.psIfaceAddrFamilyChangedEvCbackBufPtr)
  {
    goto bail;
  }

  retVal = ps_iface_event_cback_reg
           (
             NULL,
             IFACE_ADDR_FAMILY_CHANGED_EV,
             EventCbackInfo.psIfaceAddrFamilyChangedEvCbackBufPtr
           );
  if (0 != retVal)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register for IFACE_PREFIX_UPDATE_EV with netiface module
  -------------------------------------------------------------------------*/
  EventCbackInfo.psIfacePrefixUpdatedEvCbackBufPtr =
    ps_iface_alloc_event_cback_buf( PSIfaceEventCback, NULL);

  if (NULL == EventCbackInfo.psIfacePrefixUpdatedEvCbackBufPtr)
  {
    goto bail;
  }

  retVal =
    ps_iface_event_cback_reg( NULL,
                              IFACE_PREFIX_UPDATE_EV,
                              EventCbackInfo.psIfacePrefixUpdatedEvCbackBufPtr);
  if (0 != retVal)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register for IFACE_ROUTEABLE_EV with netiface module
  -------------------------------------------------------------------------*/
  EventCbackInfo.psIfaceRouteableEvCbackBufPtr =
    ps_iface_alloc_event_cback_buf( PSIfaceEventCback, NULL);

  if (NULL == EventCbackInfo.psIfaceRouteableEvCbackBufPtr)
  {
    goto bail;
  }

  retVal =
    ps_iface_event_cback_reg( NULL,
                              IFACE_ROUTEABLE_EV,
                              EventCbackInfo.psIfaceRouteableEvCbackBufPtr);
  if (0 != retVal)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register for IFACE_UP_EV with netiface module
  -------------------------------------------------------------------------*/
  EventCbackInfo.psIfaceUpEvCbackBufPtr =
    ps_iface_alloc_event_cback_buf( PSIfaceEventCback, NULL);

  if (NULL == EventCbackInfo.psIfaceUpEvCbackBufPtr)
  {
    goto bail;
  }

  retVal = ps_iface_event_cback_reg( NULL,
                                     IFACE_UP_EV,
                                     EventCbackInfo.psIfaceUpEvCbackBufPtr);
  if (0 != retVal)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register for IFACE_DOWN_EV with netiface module
  -------------------------------------------------------------------------*/
  EventCbackInfo.psIfaceDownEvCbackBufPtr =
    ps_iface_alloc_event_cback_buf( PSIfaceEventCback, NULL);

  if (NULL == EventCbackInfo.psIfaceDownEvCbackBufPtr)
  {
    goto bail;
  }

  retVal = ps_iface_event_cback_reg( NULL,
                                     IFACE_DOWN_EV,
                                     EventCbackInfo.psIfaceDownEvCbackBufPtr);
  if (0 != retVal)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register for IFACE_CONFIGURING_EV with netiface module
  -------------------------------------------------------------------------*/
  EventCbackInfo.psIfaceConfiguringEvCbackBufPtr =
    ps_iface_alloc_event_cback_buf( PSIfaceEventCback, NULL);

  if (NULL == EventCbackInfo.psIfaceConfiguringEvCbackBufPtr)
  {
    goto bail;
  }

  retVal =
    ps_iface_event_cback_reg( NULL,
                              IFACE_CONFIGURING_EV,
                              EventCbackInfo.psIfaceConfiguringEvCbackBufPtr);
  if (0 != retVal)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register for IFACE_LINGERING_EV with netiface module
  -------------------------------------------------------------------------*/
  EventCbackInfo.psIfaceLingeringEvCbackBufPtr =
    ps_iface_alloc_event_cback_buf( PSIfaceEventCback, NULL);

  if (NULL == EventCbackInfo.psIfaceLingeringEvCbackBufPtr)
  {
    goto bail;
  }

  retVal =
    ps_iface_event_cback_reg( NULL,
                              IFACE_LINGERING_EV,
                              EventCbackInfo.psIfaceLingeringEvCbackBufPtr);
  if (0 != retVal)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register for IFACE_GOING_DOWN_EV with netiface module
  -------------------------------------------------------------------------*/
  EventCbackInfo.psIfaceGoingDownEvCbackBufPtr =
    ps_iface_alloc_event_cback_buf( PSIfaceEventCback, NULL);

  if (NULL == EventCbackInfo.psIfaceGoingDownEvCbackBufPtr)
  {
    goto bail;
  }

  retVal =
    ps_iface_event_cback_reg( NULL,
                              IFACE_GOING_DOWN_EV,
                              EventCbackInfo.psIfaceGoingDownEvCbackBufPtr);
  if (0 != retVal)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register for IFACE_IPFLTR_UPDATED_EV with netiface module
  -------------------------------------------------------------------------*/
  EventCbackInfo.psIfaceIPFltrUpdatedEvCbackBufPtr =
    ps_iface_alloc_event_cback_buf( PSIfaceEventCback, NULL);

  if (NULL == EventCbackInfo.psIfaceIPFltrUpdatedEvCbackBufPtr)
  {
    goto bail;
  }

  retVal =
    ps_iface_event_cback_reg( NULL,
                              IFACE_IPFLTR_UPDATED_EV,
                              EventCbackInfo.psIfaceIPFltrUpdatedEvCbackBufPtr);
  if (0 != retVal)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register for IFACE_FLOW_ENABLED_EV with netiface module
  -------------------------------------------------------------------------*/
  EventCbackInfo.psIfaceFlowEnabledEvCbackBufPtr =
    ps_iface_alloc_event_cback_buf( PSIfaceEventCback, NULL);

  if (NULL == EventCbackInfo.psIfaceFlowEnabledEvCbackBufPtr)
  {
    goto bail;
  }

  retVal =
    ps_iface_event_cback_reg( NULL,
                              IFACE_FLOW_ENABLED_EV,
                              EventCbackInfo.psIfaceFlowEnabledEvCbackBufPtr);
  if (0 != retVal)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register for IFACE_FLOW_DISABLED_EV with netiface module
  -------------------------------------------------------------------------*/
  EventCbackInfo.psIfaceFlowDisabledEvCbackBufPtr =
    ps_iface_alloc_event_cback_buf( PSIfaceEventCback, NULL);

  if (NULL == EventCbackInfo.psIfaceFlowDisabledEvCbackBufPtr)
  {
    goto bail;
  }

  retVal =
    ps_iface_event_cback_reg( NULL,
                              IFACE_FLOW_DISABLED_EV,
                              EventCbackInfo.psIfaceFlowDisabledEvCbackBufPtr);
  if (0 != retVal)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register for FLOW_ACTIVATED_EV with netiface module
  -------------------------------------------------------------------------*/
  EventCbackInfo.psFlowActivatedEvCbackBufPtr =
    ps_flow_alloc_event_cback_buf( PSFlowEventCback, NULL);

  if (NULL == EventCbackInfo.psFlowActivatedEvCbackBufPtr)
  {
    goto bail;
  }

  retVal =
    ps_flow_event_cback_reg( NULL,
                             FLOW_ACTIVATED_EV,
                             EventCbackInfo.psFlowActivatedEvCbackBufPtr);
  if (0 != retVal)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register for FLOW_SUSPENDED_EV with netiface module
  -------------------------------------------------------------------------*/
  EventCbackInfo.psFlowSuspendedEvCbackBufPtr =
    ps_flow_alloc_event_cback_buf( PSFlowEventCback, NULL);

  if (NULL == EventCbackInfo.psFlowSuspendedEvCbackBufPtr)
  {
    goto bail;
  }

  retVal =
    ps_flow_event_cback_reg( NULL,
                             FLOW_SUSPENDED_EV,
                             EventCbackInfo.psFlowSuspendedEvCbackBufPtr);
  if (0 != retVal)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register for FLOW_TX_ENABLED_EV with netiface module
  -------------------------------------------------------------------------*/
  EventCbackInfo.psFlowTxEnabledEvCbackBufPtr =
    ps_flow_alloc_event_cback_buf( PSFlowEventCback, NULL);

  if (NULL == EventCbackInfo.psFlowTxEnabledEvCbackBufPtr)
  {
    goto bail;
  }

  retVal =
    ps_flow_event_cback_reg( NULL,
                             FLOW_TX_ENABLED_EV,
                             EventCbackInfo.psFlowTxEnabledEvCbackBufPtr);
  if (0 != retVal)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register for FLOW_TX_DISABLED_EV with netiface module
  -------------------------------------------------------------------------*/
  EventCbackInfo.psFlowTxDisabledEvCbackBufPtr =
    ps_flow_alloc_event_cback_buf( PSFlowEventCback, NULL);

  if (NULL == EventCbackInfo.psFlowTxDisabledEvCbackBufPtr)
  {
    goto bail;
  }

  retVal =
    ps_flow_event_cback_reg( NULL,
                             FLOW_TX_DISABLED_EV,
                             EventCbackInfo.psFlowTxDisabledEvCbackBufPtr);
  if (0 != retVal)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register for PHYS_LINK_UP_EV with netiface module
  -------------------------------------------------------------------------*/
  EventCbackInfo.physLinkUpEvCbackBufPtr =
    ps_phys_link_alloc_event_cback_buf( PhysLinkEventCback, NULL);

  if (NULL == EventCbackInfo.physLinkUpEvCbackBufPtr)
  {
    goto bail;
  }

  retVal =
    ps_phys_link_event_cback_reg( NULL,
                                  PHYS_LINK_UP_EV,
                                  EventCbackInfo.physLinkUpEvCbackBufPtr);
  if (0 != retVal)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register for PHYS_LINK_DOWN_EV with netiface module
  -------------------------------------------------------------------------*/
  EventCbackInfo.physLinkDownEvCbackBufPtr =
    ps_phys_link_alloc_event_cback_buf( PhysLinkEventCback, NULL);

  if (NULL == EventCbackInfo.physLinkDownEvCbackBufPtr)
  {
    goto bail;
  }

  retVal =
    ps_phys_link_event_cback_reg( NULL,
                                  PHYS_LINK_DOWN_EV,
                                  EventCbackInfo.physLinkDownEvCbackBufPtr);
  if (0 != retVal)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register for PHYS_LINK_FLOW_ENABLED_EV with netiface module
  -------------------------------------------------------------------------*/
  EventCbackInfo.physLinkFlowEnabledEvCbackBufPtr =
    ps_phys_link_alloc_event_cback_buf( PhysLinkEventCback, NULL);

  if (NULL == EventCbackInfo.physLinkFlowEnabledEvCbackBufPtr)
  {
    goto bail;
  }

  retVal = ps_phys_link_event_cback_reg
           (
             NULL,
             PHYS_LINK_FLOW_ENABLED_EV,
             EventCbackInfo.physLinkFlowEnabledEvCbackBufPtr
           );
  if (0 != retVal)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register for PHYS_LINK_FLOW_DISABLED_EV with netiface module
  -------------------------------------------------------------------------*/
  EventCbackInfo.physLinkFlowDisabledEvCbackBufPtr =
    ps_phys_link_alloc_event_cback_buf( PhysLinkEventCback, NULL);

  if (NULL == EventCbackInfo.physLinkFlowDisabledEvCbackBufPtr)
  {
    goto bail;
  }

  retVal = ps_phys_link_event_cback_reg
           (
             NULL,
             PHYS_LINK_FLOW_DISABLED_EV,
             EventCbackInfo.physLinkFlowDisabledEvCbackBufPtr
           );
  if (0 != retVal)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register for PHYS_LINK_707_DOS_ACK_EV with netiface module

    Handle DoS event separately as extra information needs to be passed along
    with the event
  -------------------------------------------------------------------------*/
  EventCbackInfo.physLinkDoSAckEvCbackBufPtr =
    ps_phys_link_alloc_event_cback_buf( PhysLinkDoSAckEventCback, NULL);

  if (NULL == EventCbackInfo.physLinkDoSAckEvCbackBufPtr)
  {
    goto bail;
  }

  retVal =
    ps_phys_link_event_cback_reg( NULL,
                                  PHYS_LINK_707_DOS_ACK_EV,
                                  EventCbackInfo.physLinkDoSAckEvCbackBufPtr);
  if (0 != retVal)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Register for ps_mem callbacks
  -------------------------------------------------------------------------*/
  ps_mem_reg_mem_avail_cb( PS_MEM_RT_META_INFO_TYPE, PSMemEventCback);
  ps_mem_reg_mem_avail_cb( PS_MEM_PKT_META_INFO_TYPE, PSMemEventCback);

  return;

bail:
  LOG_MSG_ERROR( "Event registration failed", 0, 0, 0);
  ASSERT( 0);
  return;

} /* DS::Sock::EventManager::Init() */
