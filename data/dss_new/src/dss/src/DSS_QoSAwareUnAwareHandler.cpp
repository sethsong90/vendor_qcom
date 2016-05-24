/*======================================================

FILE:  DSS_QoSAwareUnAwareHandler.cpp

GENERAL DESCRIPTION:
   Implementation of DSS_QoSAwareUnAwareHandler functions

=====================================================

Copyright (c) 2008 - 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_QoSAwareUnAwareHandler.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_QoSAwareUnAwareHandler.h"
#include "DSS_Common.h"
#include "DSS_IDSNetworkPrivScope.h"
#include "DSS_IDSNetworkExtScope.h"
#include "DSS_MemoryManagement.h"

using namespace ds::Net;

DSSQoSAwareUnAwareHandler::DSSQoSAwareUnAwareHandler()
{
   mEt = EVENT_HANDLER_QOS_AW_UNAW;
}

AEEResult DSSQoSAwareUnAwareHandler::SetEventData(dss_iface_ioctl_event_enum_type event,
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
      //there are 2  EventData edAware,edUnAware
      mEd->m_Ed  = (EventData *)ps_system_heap_mem_alloc(sizeof(EventData)*2);
      if (NULL == mEd->m_Ed) {
         LOG_MSG_ERROR("Can't allocate event data", 0, 0, 0);
         return AEE_ENOMEMORY;
      }

      memset (mEd->m_Ed, 0, 2 * sizeof (EventData));
   }

   if (NULL == mEdClone->m_Ed) {
      mEdClone->m_Ed  = (EventData *)ps_system_heap_mem_alloc(sizeof(EventData)*2);
      if (NULL == mEdClone->m_Ed) {
         LOG_MSG_ERROR("Can't allocate event data", 0, 0, 0);
         return AEE_ENOMEMORY;
      }
      
      memset (mEdClone->m_Ed, 0, 2 * sizeof (EventData));
   }

   switch (event) {
      case DSS_IFACE_IOCTL_QOS_AWARE_SYSTEM_EV:
         pEd = &(mEd->m_Ed[0]);
         break;
      case DSS_IFACE_IOCTL_QOS_UNAWARE_SYSTEM_EV:
         pEd = &(mEd->m_Ed[1]);
         break;
      default:
         LOG_MSG_ERROR("DSSQoSAwareUnAwareHandler::SetEventData: unknown event %d",event,0,0);
         ASSERT(0); // We shouldn't get here.
         return 0;
   }

   IDS_ERR_RET(InitEventData(pEd,bReg,userCB,userData));

   return AEE_SUCCESS;
}

void DSSQoSAwareUnAwareHandler::EventOccurred()
{
   if (NULL == mEd->m_Ed || NULL == mEdClone->m_Ed) {
      LOG_MSG_ERROR("EventOccurred: No EventData allocated.", 0, 0, 0);
      return;
   }
   
   {
      // for DSSQoSAwareUnAwareHandler, event data has different size, so it should copy
      // all the data to mEdClone
      DSSCritScope cs(*piCritSect);
      memcpy(mEdClone->m_Ed, mEd->m_Ed, sizeof(EventData)*2);

   }
   
   DSSIDSNetworkExtScope IDSNetworkExtScope;
   DSSIDSNetworkPrivScope IDSNetworkPrivScope;
   AEEResult nRet;
   boolean bQoSAware;
   dss_iface_ioctl_event_enum_type event;
   dss_iface_ioctl_event_info_union_type eventInfo;
   QoSInfoCodeType infoCode;
   DSSWeakRefScope WeakRefScope;

   if(!WeakRefScope.Init(parentNetApp)) {
      return;
   }

   memset(&eventInfo,0,sizeof(dss_iface_ioctl_event_info_union_type));

   if ( AEE_SUCCESS != IDSNetworkExtScope.Init(parentNetApp) ) {
      return;
   }
   if ( AEE_SUCCESS != IDSNetworkPrivScope.Init(parentNetApp) ) {
      return;
   }

   nRet = IDSNetworkExtScope.Fetch()->GetQosAware(&bQoSAware);
   if (AEE_SUCCESS != nRet) {
      // TODO: Need to add error message.
      return;
   }

   if (bQoSAware) {
      event = DSS_IFACE_IOCTL_QOS_AWARE_SYSTEM_EV;
   } else {
      event = DSS_IFACE_IOCTL_QOS_UNAWARE_SYSTEM_EV;
   }

   nRet = IDSNetworkPrivScope.Fetch()->GetQoSAwareInfoCode(&infoCode);
   if (AEE_SUCCESS != nRet) {
      // TODO: Need to add error message.
      return;
   }

   nRet = DSSConversion::IDS2DSQoSAwareInfoCode(infoCode, &(eventInfo.qos_aware_info_code));
   if (AEE_SUCCESS != nRet){
      // TODO: Need to add error message.
      return;
   }

   EventData* pEd;

   switch (event) {
      case DSS_IFACE_IOCTL_QOS_AWARE_SYSTEM_EV:
         pEd = &(mEdClone->m_Ed[0]);
         break;
      case DSS_IFACE_IOCTL_QOS_UNAWARE_SYSTEM_EV:
         pEd = &(mEdClone->m_Ed[1]);
         break;
      default:
         pEd = NULL;
   }

   if ((0 != pEd) && pEd->bReg) {
      DispatchCB(event, pEd, &eventInfo);
   }
}

AEEResult DSSQoSAwareUnAwareHandler::RegisterIDL()
{
   DSSIDSNetworkPrivScope IDSNetworkPrivScope;
   INetworkExt* pINetworkExt = NULL;
   AEEResult res;
   DSSWeakRefScope WeakRefScope;

   if(!WeakRefScope.Init(parentNetApp)) {
      return AEE_EFAILED;
   }

   //do not call lower layer in case already registered
   if(TRUE == mIsRegistered){
      return AEE_SUCCESS;
   }

   IDS_ERR_RET(IDSNetworkPrivScope.Init(parentNetApp));
   IDS_ERR_RET(IDSNetworkPrivScope.Fetch()->QueryInterface(AEEIID_INetworkExt, (void**)&pINetworkExt));

   LOG_MSG_INFO1("Registering to QDS_EV_QOS_AWARENESS, NetworkExt obj 0x%p", pINetworkExt, 0, 0);
   res = pINetworkExt->OnStateChange(piSignal, NetworkExtEvent::QDS_EV_QOS_AWARENESS, &mRegObj);
   if(AEE_SUCCESS == res){
      mIsRegistered = TRUE;
   }

   DSSCommon::ReleaseIf((IQI**)&pINetworkExt);

   return res;
}

DSSQoSAwareUnAwareHandler* DSSQoSAwareUnAwareHandler::CreateInstance()
{
   return new DSSQoSAwareUnAwareHandler;
}

