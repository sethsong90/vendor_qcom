/*======================================================

FILE:  DSS_NetworkStateHandler.cpp

GENERAL DESCRIPTION:
   Implementation of DSS_NetworkStateHandler functions

=====================================================

Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_NetworkStateHandler.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_NetworkStateHandler.h"
#include "DSS_Common.h"
#include "DSS_IDSNetworkPrivScope.h"
#include "DSS_IDSNetworkScope.h"
#include "DSS_MemoryManagement.h"

using namespace ds::Net;

DSSNetworkStateHandler::DSSNetworkStateHandler()
{
   mEt = EVENT_HANDLER_NETWORK_STATE;
}

dss_iface_ioctl_event_enum_type
   DSSNetworkStateHandler::NetStateToEvent(NetworkStateType netState)
{
   // TODO: check if we cover all the possible iface events, or some should be added to the idls
   switch (netState) {
      case NetworkState::QDS_OPEN_IN_PROGRESS:
         return DSS_IFACE_IOCTL_COMING_UP_EV;
      case NetworkState::QDS_OPEN:
         return DSS_IFACE_IOCTL_UP_EV;
      case NetworkState::QDS_CLOSE_IN_PROGRESS:
         return DSS_IFACE_IOCTL_GOING_DOWN_EV;
      case NetworkState::QDS_CLOSED:
      case NetworkState::QDS_LINGERING:
         return DSS_IFACE_IOCTL_DOWN_EV;
      default:
         return DSS_IFACE_IOCTL_MIN_EV; // No corresponding event.
   }
}

dss_iface_ioctl_event_info_type
   DSSNetworkStateHandler::NetStateToIfaceState(NetworkStateType netState)
{
   switch (netState) {
      case NetworkState::QDS_OPEN_IN_PROGRESS:
         return IFACE_COMING_UP;
      case NetworkState::QDS_OPEN:
         return IFACE_UP;
      case NetworkState::QDS_CLOSE_IN_PROGRESS:
         return IFACE_GOING_DOWN;
      case NetworkState::QDS_CLOSED:
         return IFACE_DOWN;
      default:
         LOG_MSG_ERROR("NetStateToIfaceState: unknown netState %d",netState,0,0);
         ASSERT(0); // We shouldn't get here.
   }
   return IFACE_DISABLED;
}

AEEResult DSSNetworkStateHandler::SetEventData(dss_iface_ioctl_event_enum_type event,
                                         bool bReg,
                                         dss_iface_ioctl_event_cb userCB,
                                         void* userData)
{
   EventData* pEd;

   if (NULL == mEd) {
      mEd = (DSSEventHandler::HandlerData *)ps_system_heap_mem_alloc(sizeof(DSSEventHandler::HandlerData));
      if (NULL == mEd) {
         LOG_MSG_ERROR("Can't allocate event data", 0, 0, 0);
         return AEE_ENOMEMORY;
      }

      mEd->m_Ed = NULL;
   }

   if (NULL == mEdClone) {
      mEdClone = (DSSEventHandler::HandlerData *)ps_system_heap_mem_alloc(sizeof(DSSEventHandler::HandlerData));
      if (NULL == mEdClone) {
         LOG_MSG_ERROR("Can't allocate event data", 0, 0, 0);
         return AEE_ENOMEMORY;
      }
      
      mEdClone->m_Ed = NULL;
   }

   // if it is the first call we need to allocate the EventData array
   if (NULL == mEd->m_Ed){
      //there are 4  EventData edDown,edUp, edComingUp, edGoingDown;
      mEd->m_Ed  = (EventData *)ps_system_heap_mem_alloc(sizeof(EventData)*4);
      if (NULL == mEd->m_Ed) {
         LOG_MSG_ERROR("Can't allocate event data", 0, 0, 0);
         return AEE_ENOMEMORY;
      }

      memset (mEd->m_Ed, 0, 4 * sizeof (EventData));
   }

   if (NULL == mEdClone->m_Ed) {
      mEdClone->m_Ed  = (EventData *)ps_system_heap_mem_alloc(sizeof(EventData)*4);
      if (NULL == mEdClone->m_Ed) {
         LOG_MSG_ERROR("Can't allocate event data", 0, 0, 0);
         return AEE_ENOMEMORY;
      }
      
      memset (mEdClone->m_Ed, 0, 4 * sizeof (EventData));
   }

   switch (event) {
      case DSS_IFACE_IOCTL_DOWN_EV:
         pEd = &(mEd->m_Ed[0]);
         break;
      case DSS_IFACE_IOCTL_UP_EV:
         pEd = &(mEd->m_Ed[1]);
         break;
      case DSS_IFACE_IOCTL_COMING_UP_EV:
         pEd = &(mEd->m_Ed[2]);
         break;
      case DSS_IFACE_IOCTL_GOING_DOWN_EV:
         pEd = &(mEd->m_Ed[3]);
         break;
      default:
         LOG_MSG_ERROR("DSSNetworkStateHandler:SetEventData: unknown event %d",event,0,0);
         ASSERT(0); // We shouldn't get here.
         return 0;
   }

   IDS_ERR_RET(InitEventData(pEd,bReg,userCB,userData));

   return AEE_SUCCESS;
}

void DSSNetworkStateHandler::EventOccurred()
{
   if (NULL == mEd->m_Ed || NULL == mEdClone->m_Ed) {
      LOG_MSG_ERROR("EventOccurred: No EventData allocated.", 0, 0, 0);
      return;
   }
   
   {
      // for DSSNetworkStateHandler, event data has different size, so it should copy
      // all the data to mEdClone
      DSSCritScope cs(*piCritSect);
      memcpy(mEdClone->m_Ed, mEd->m_Ed, sizeof(EventData)*4);

   } // release lock

   DSSIDSNetworkScope IDSNetworkScope;
   DSSIDSNetworkPrivScope IDSNetworkPrivScope;
   DSSWeakRefScope WeakRefScope;

   if(!WeakRefScope.Init(parentNetApp)) {
      return;
   }

   if ( AEE_SUCCESS != IDSNetworkScope.Init(parentNetApp) ) {
      return;
   }

   if ( AEE_SUCCESS != IDSNetworkPrivScope.Init(parentNetApp) ) {
      return;
   }

   NetworkStateType netState = 0;
   NetworkStateType previousNetState = 0;
   AEEResult res = IDSNetworkScope.Fetch()->GetState(&netState);
   if (AEE_SUCCESS == res) {
      res = IDSNetworkPrivScope.Fetch()->GetPreviousState(&previousNetState);
   }
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("GetState() or GetPreviousState() failed: %d", res, 0, 0);
      return;
   }
   if (NetworkState::QDS_CLOSED == netState ||
       NetworkState::QDS_LINGERING == netState) {
      parentNetApp->SetNetworkIsUp(FALSE);
   }
   dss_iface_ioctl_event_enum_type event = NetStateToEvent(netState);

   EventData* pEd;

   switch (event) {
      case DSS_IFACE_IOCTL_DOWN_EV:
         pEd = &(mEdClone->m_Ed[0]);
         break;
      case DSS_IFACE_IOCTL_UP_EV:
         pEd = &(mEdClone->m_Ed[1]);
         break;
      case DSS_IFACE_IOCTL_COMING_UP_EV:
         pEd = &(mEdClone->m_Ed[2]);
         break;
      case DSS_IFACE_IOCTL_GOING_DOWN_EV:
         pEd = &(mEdClone->m_Ed[3]);
         break;
      default:
         pEd = NULL;
   }

   if ((0 != pEd) && pEd->bReg) {
      dss_iface_ioctl_event_info_union_type eventInfo;
      eventInfo.iface_state_info = NetStateToIfaceState(previousNetState);
      DispatchCB(event, pEd, &eventInfo);
   }
}

AEEResult DSSNetworkStateHandler::RegisterIDL()
{
   DSSIDSNetworkPrivScope IDSNetworkPrivScope;
   DSSWeakRefScope WeakRefScope;

   if(!WeakRefScope.Init(parentNetApp)) {
      return AEE_EFAILED;
   }

   //do not call lower layer in case already registered
   if(TRUE == mIsRegistered){
      return AEE_SUCCESS;
   }

   IDS_ERR_RET(IDSNetworkPrivScope.Init(parentNetApp));
   
   LOG_MSG_INFO1("Registering to QDS_EV_STATE_CHANGED, NetworkPriv obj 0x%p", IDSNetworkPrivScope.Fetch(), 0, 0);
   AEEResult res = IDSNetworkPrivScope.Fetch()->OnStateChange(piSignal, NetworkEvent::QDS_EV_STATE_CHANGED, &mRegObj);
   if(AEE_SUCCESS == res){
      mIsRegistered = TRUE;
   }

   return res;
}

DSSNetworkStateHandler* DSSNetworkStateHandler::CreateInstance()
{
   return new DSSNetworkStateHandler;
}


