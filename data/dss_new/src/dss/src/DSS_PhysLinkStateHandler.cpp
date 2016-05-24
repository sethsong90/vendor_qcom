/*======================================================

FILE:  DSS_PhysLinkStateHandler.cpp

GENERAL DESCRIPTION:
   Implementation of DSS_PhysLinkStateHandler functions

=====================================================

Copyright (c) 2008 - 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_PhysLinkStateHandler.cpp#2 $
  $DateTime: 2011/08/02 03:56:27 $$Author: eazriel $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_PhysLinkStateHandler.h"
#include "DSS_Common.h"
#include "DSS_IDSNetworkScope.h"
#include "DSS_GenScope.h"
#include "DSS_MemoryManagement.h"

using namespace ds::Net;

DSSPhysLinkStateHandler::DSSPhysLinkStateHandler()
{
   mEt = EVENT_HANDLER_PHYS_LINK_STATE;
}

dss_iface_ioctl_event_enum_type
   DSSPhysLinkStateHandler::PhysLinkStateToEvent(PhysLinkStateType physState)
{
   // TODO: check if we cover all the possible iface events, or some should be added to the idls
   switch (physState) {
      case PhysLinkState::QDS_DOWN:
         return DSS_IFACE_IOCTL_PHYS_LINK_DOWN_EV;
      case PhysLinkState::QDS_UP:
         return DSS_IFACE_IOCTL_PHYS_LINK_UP_EV;
      case PhysLinkState::QDS_RESUMING:
         return DSS_IFACE_IOCTL_PHYS_LINK_COMING_UP_EV;
      case PhysLinkState::QDS_GOING_DOWN:
         return DSS_IFACE_IOCTL_PHYS_LINK_GOING_DOWN_EV;
      default:
         return DSS_IFACE_IOCTL_MIN_EV; // No corresponding event.
   }
}

dss_iface_ioctl_phys_link_event_info_type
   DSSPhysLinkStateHandler::IDS2DSPhysLinkState(PhysLinkStateType physState)
{
   switch (physState) {
      case PhysLinkState::QDS_DOWN:
         return PHYS_LINK_DOWN;
      case PhysLinkState::QDS_UP:
         return PHYS_LINK_UP;
      case PhysLinkState::QDS_RESUMING:
         return PHYS_LINK_COMING_UP;
      case PhysLinkState::QDS_GOING_DOWN:
         return PHYS_LINK_GOING_DOWN;
   }
   return PHYS_LINK_DOWN;
}

AEEResult DSSPhysLinkStateHandler::SetEventData(dss_iface_ioctl_event_enum_type event,
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
      case DSS_IFACE_IOCTL_PHYS_LINK_DOWN_EV:
         pEd =  &(mEd->m_Ed[0]);
         break;
      case DSS_IFACE_IOCTL_PHYS_LINK_UP_EV:
         pEd =  &(mEd->m_Ed[1]);
         break;
      case DSS_IFACE_IOCTL_PHYS_LINK_COMING_UP_EV:
         pEd =  &(mEd->m_Ed[2]);
         break;
      case DSS_IFACE_IOCTL_PHYS_LINK_GOING_DOWN_EV:
         pEd =  &(mEd->m_Ed[3]);
         break;
      default:
         LOG_MSG_ERROR("DSSPhysLinkStateHandler::SetEventData: unknown event %d",event,0,0);
         ASSERT(0); // We shouldn't get here.
         return 0;

   }
   bool bWasRegistered = pEd->bReg;

   IDS_ERR_RET(InitEventData(pEd,bReg,userCB,userData));

   // in case this is the last DeRegister event from 4 possible
   // then DeRegister from DS_Net by Releasing PhysLink object
   if(false == bReg && 
      true == bWasRegistered &&
      false == mEd->m_Ed[0].bReg &&
      false == mEd->m_Ed[1].bReg &&
      false == mEd->m_Ed[2].bReg &&
      false == mEd->m_Ed[3].bReg){
      parentNetApp->ReleasePhysLink();
      mIsRegistered = FALSE;
   }

   return AEE_SUCCESS;
}

void DSSPhysLinkStateHandler::EventOccurred()
{
   if (NULL == mEd->m_Ed || NULL == mEdClone->m_Ed) {
      LOG_MSG_ERROR("EventOccurred: No EventData allocated.", 0, 0, 0);
      return;
   }
   
   {
      // for DSSPhysLinkStateHandler, event data has different size, so it should copy
      // all the data to mEdClone
      DSSCritScope cs(*piCritSect);
      memcpy(mEdClone->m_Ed, mEd->m_Ed, sizeof(EventData)*4);

   } // release lock
   
   IPhysLink* piPhysLink = 0;
   AEEResult res;
   DSSWeakRefScope WeakRefScope;

   if(!WeakRefScope.Init(parentNetApp)) {
      return;
   }

   res = parentNetApp->GetPhysLink(&piPhysLink);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("GetPhysLink() failed: %d", res, 0, 0);
      return;
   }

   DSSGenScope scopePhysLink(piPhysLink,DSSGenScope::IDSIQI_TYPE);

   PhysLinkStateType physState = 0;
   PhysLinkStateType previousPhysState = 0;
   res = piPhysLink->GetState(&physState);
   if (AEE_SUCCESS == res) {
      res = piPhysLink->GetPreviousState(&previousPhysState);
   }
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("GetState() or GetPreviousState() failed: %d", res, 0, 0);
      return;
   }
   dss_iface_ioctl_event_enum_type event = PhysLinkStateToEvent(physState);


   EventData* pEd;

   switch (event) {
      case DSS_IFACE_IOCTL_PHYS_LINK_DOWN_EV:
         pEd =  &(mEdClone->m_Ed[0]);
         break;
      case DSS_IFACE_IOCTL_PHYS_LINK_UP_EV:
         pEd =  &(mEdClone->m_Ed[1]);
         break;
      case DSS_IFACE_IOCTL_PHYS_LINK_COMING_UP_EV:
         pEd =  &(mEdClone->m_Ed[2]);
         break;
      case DSS_IFACE_IOCTL_PHYS_LINK_GOING_DOWN_EV:
         pEd = &(mEdClone->m_Ed[3]);
         break;
      default:
         pEd = NULL;
   }

   if ((0 != pEd) && pEd->bReg) {
      dss_iface_ioctl_event_info_union_type eventInfo;
      eventInfo.phys_link_state_info = IDS2DSPhysLinkState(previousPhysState);
      DispatchCB(event, pEd, &eventInfo);
   }
}

AEEResult DSSPhysLinkStateHandler::RegisterIDL()
{
   IPhysLink* piPhysLink = 0;
   DSSWeakRefScope WeakRefScope;

   if(!WeakRefScope.Init(parentNetApp)) {
      return AEE_EFAILED;
   }

   //do not call lower layer in case already registered
   if(TRUE == mIsRegistered){
      return AEE_SUCCESS;
   }

   AEEResult res = parentNetApp->GetPhysLink(&piPhysLink);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("GetPhysLink() failed: %d", res, 0, 0);
      return res;
   }

   LOG_MSG_INFO1("Registering to QDS_EV_STATE_CHANGED, PhysLink obj 0x%p", piPhysLink, 0, 0);
   res = piPhysLink->OnStateChange(piSignal, PhysLinkEvent::QDS_EV_STATE_CHANGED, &mRegObj);
   if(AEE_SUCCESS == res){
      mIsRegistered = TRUE;
   }

   DSSCommon::ReleaseIf((IQI**)&piPhysLink);

   return res;
}

DSSPhysLinkStateHandler* DSSPhysLinkStateHandler::CreateInstance()
{
   return new DSSPhysLinkStateHandler;
}

