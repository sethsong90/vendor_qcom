/*======================================================

FILE:  DSS_MTPDRequestHandler.h

GENERAL DESCRIPTION:
   Implementation of DSS_MTPDRequestHandler functions

=====================================================

Copyright (c) 2008 - 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_MTPDRequestHandler.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_MTPDRequestHandler.h"
#include "DSS_Common.h"
#include "DSS_IDSNetPolicyScope.h"
#include "DSS_MemoryManagement.h"
#include "ds_Net_CreateInstance.h"

using namespace ds::Net;

DSSMTPDRequestHandler::DSSMTPDRequestHandler()
{
   mEt = EVENT_HANDLER_MTPD_REQUEST;
}

AEEResult DSSMTPDRequestHandler::SetEventData(dss_iface_ioctl_event_enum_type event,
                                  bool bReg,
                                  dss_iface_ioctl_event_cb userCB,
                                  void* userData)
{
   if (NULL == mEd) {
      mEd = (DSSMTPDRequestData *)ps_system_heap_mem_alloc(sizeof(DSSMTPDRequestData));
      if (NULL == mEd) {
         LOG_MSG_ERROR("Can't allocate event data", 0, 0, 0);
         return AEE_ENOMEMORY;
      }

      mEd->m_Ed = NULL;
   }

   if (NULL == mEdClone) {
      mEdClone = (DSSMTPDRequestData *)ps_system_heap_mem_alloc(sizeof(DSSMTPDRequestData));
      if (NULL == mEdClone) {
         LOG_MSG_ERROR("Can't allocate event data", 0, 0, 0);
         return AEE_ENOMEMORY;
      }

      mEdClone->m_Ed = NULL;
   }

   if (NULL == mEd->m_Ed) {
      mEd->m_Ed = (EventData *)ps_system_heap_mem_alloc(sizeof(EventData));
      if (NULL == mEd->m_Ed) {
         LOG_MSG_ERROR("Can't allocate event data", 0, 0, 0);
         return AEE_ENOMEMORY;
      }

      memset (mEd->m_Ed, 0, sizeof (EventData));
   }

   if (NULL == mEdClone->m_Ed) {
      mEdClone->m_Ed = (EventData *)ps_system_heap_mem_alloc(sizeof(EventData));
      if (NULL == mEdClone->m_Ed) {
         LOG_MSG_ERROR("Can't allocate event data", 0, 0, 0);
         return AEE_ENOMEMORY;
      }

      memset (mEdClone->m_Ed, 0, sizeof (EventData));
   }

   ((DSSMTPDRequestData*)mEd)->pMTPDReg = NULL;

   IDS_ERR_RET(InitEventData(mEd->m_Ed,bReg,userCB,userData));

   return AEE_SUCCESS;
}

void DSSMTPDRequestHandler::EventOccurred()
{
   {
      // for DSSMTPDRequestHandler, event data has different size, so it should copy
      // all the data to mEdClone
      DSSCritScope cs(*piCritSect);
      if ((((DSSMTPDRequestData*)mEdClone)->m_Ed)->bReg) {

         memcpy(((DSSMTPDRequestData*)mEdClone)->m_Ed,
            ((DSSMTPDRequestData*)mEd)->m_Ed,
            sizeof(DSSEventHandler::EventData));

         ((DSSMTPDRequestData*)mEdClone)->pMTPDReg = ((DSSMTPDRequestData*)mEd)->pMTPDReg;
      }
      else {
         return;
      }
   } // release lock
   
   dss_iface_ioctl_event_info_union_type eventInfo;
   DSSWeakRefScope WeakRefScope;

   if(!WeakRefScope.Init(parentNetApp)) {
      return;
   }
   
   memset(&eventInfo,0,sizeof(dss_iface_ioctl_event_info_union_type));
   eventInfo.mt_handle = (dss_iface_ioctl_mt_handle_type)this;
   DispatchCB(DSS_IFACE_IOCTL_MT_REQUEST_EV, ((DSSMTPDRequestData*)mEdClone)->m_Ed, &eventInfo);

}

AEEResult DSSMTPDRequestHandler::RegisterIDL()
{
   DSSIDSNetPolicyScope   IDSNetPolicyScope;
   AEEResult              res = AEE_SUCCESS;
   ITechUMTS*             piTechUMTS = 0;
   DSSWeakRefScope WeakRefScope;

   if(!WeakRefScope.Init(parentNetApp)) {
      return AEE_EFAILED;
   }
   
   IDS_ERR_RET(IDSNetPolicyScope.Init(parentNetApp));

   res = parentNetApp->GetTechUMTS(&piTechUMTS);

   if (AEE_SUCCESS != res ) {
      return QDS_EFAULT;
   }

   res = piTechUMTS->RegMTPD(IDSNetPolicyScope.Fetch(), piSignal, &(((DSSMTPDRequestData*)mEd)->pMTPDReg));

   DSSCommon::ReleaseIf((IQI**)&piTechUMTS);

   return res;
}

/*lint -e{1551} */
void DSSMTPDRequestHandler::Destructor() throw()
{
   DSSCommon::ReleaseIf(&(((DSSMTPDRequestData*)mEd)->pMTPDReg));
   DSSEventHandler::Destructor();
}

DSSMTPDRequestHandler* DSSMTPDRequestHandler::CreateInstance()
{
   return new DSSMTPDRequestHandler;
}

