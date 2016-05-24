/*======================================================

FILE:  DSS_EventHandler.cpp

GENERAL DESCRIPTION:
   Implementation of DSS_EventHandler functions

=====================================================

Copyright (c) 2008-2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_EventHandler.cpp#3 $
  $DateTime: 2011/07/25 23:24:15 $$Author: c_rpidap $
  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_EventHandler.cpp#3 $
  $DateTime: 2011/07/25 23:24:15 $$Author: c_rpidap $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2011-07-15 rp  QShrink2 changes: MSG_* macros are removed from inline functions.
  2010-04-13 en  History added.

===========================================================================*/

#include "customer.h"

#include "DSS_Globals.h"
#include "DSS_Common.h"
#include "DSS_CritScope.h"
#include "DSS_EventHandler.h"
#include "DSS_MemoryManagement.h"
#include "ds_Utils_Atomic.h"
#include "ds_Errors_Def.h"
#include "ps_system_heap.h"

using namespace ds::Error;

DSSEventHandler::DSSEventHandler() :
   mRegObj(NULL),
   mIsRegistered(FALSE),
   piSignal(NULL),
   mEd(NULL),
   mEt(EVENT_HANDLER_UNDEFINED),
   mEdClone(NULL),
   piCritSect(NULL),
   piSignalCtl(NULL)
{
}

void DSSEventHandler::SignalCB(void* pv)
{
   if (NULL == pv)
   {
      LOG_MSG_ERROR ("Signal dispatch on deleted obj", 0, 0, 0);
      return;
   }

   LOG_MSG_FUNCTION_ENTRY("pv:0x%p", pv, 0, 0);

   // Event Handler may be released from within the CBHandler (EventOccured) function
   // if so , we want to finally delete it just in the very end of SignalCB
   // TODO: we get a strong ref to EventHandler through SignalHandler::Notify(), is
   // this AddRef(), Release is needed?
   //(void) reinterpret_cast<DSSEventHandler*>(pv)->AddRef();
   reinterpret_cast<DSSEventHandler*>(pv)->CBHandler();
   //(void) reinterpret_cast<DSSEventHandler*>(pv)->Release();
}

inline void DSSEventHandler::CBHandler()
{
   // copy all event data to mEdClone
   piCritSect->Enter();
   memcpy(mEdClone->m_Ed, mEd->m_Ed, sizeof(DSSEventHandler::EventData));
   piCritSect->Leave();

   EventOccurred();

   AEEResult res = piSignalCtl->Enable();
   // Assuming no events occurred during the invoking of user app callback

   if (AEE_SUCCESS != res) {
      /*LOG_MSG_ERROR("Signal Enable() failed: %d", res, 0, 0);*/
      return;
   }
}

AEEResult DSSEventHandler::SetEventData
(
   dss_iface_ioctl_event_enum_type event,
   bool bReg,
   dss_iface_ioctl_event_cb userCB,
   void* userData
)
{
   if (NULL == mEd) {
      mEd = (DSSEventHandler::HandlerData *)ps_system_heap_mem_alloc(sizeof(DSSEventHandler::HandlerData));
      if (NULL == mEd) {
         LOG_MSG_ERROR("Can't allocate event data", 0, 0, 0);
         return AEE_ENOMEMORY;
      }

      mEd->m_Ed = NULL;
   }

   if(NULL == mEdClone) {
      mEdClone = (DSSEventHandler::HandlerData *)ps_system_heap_mem_alloc(sizeof(DSSEventHandler::HandlerData));
      if (NULL == mEdClone) {
         LOG_MSG_ERROR("Can't allocate event data", 0, 0, 0);
         return AEE_ENOMEMORY;
      }

      mEdClone->m_Ed= NULL;
   }

   if (NULL == mEd->m_Ed) {
      mEd->m_Ed = (DSSEventHandler::EventData *)ps_system_heap_mem_alloc(sizeof(DSSEventHandler::EventData));
      if (NULL == mEd->m_Ed) {
         LOG_MSG_ERROR("Can't allocate event data", 0, 0, 0);
         return AEE_ENOMEMORY;
      }

      memset (mEd->m_Ed, 0, sizeof (DSSEventHandler::EventData));
   }

   if (NULL == mEdClone->m_Ed) {
      mEdClone->m_Ed = (DSSEventHandler::EventData *)ps_system_heap_mem_alloc(sizeof(DSSEventHandler::EventData));
      if (NULL == mEdClone->m_Ed) {
         LOG_MSG_ERROR("Can't allocate event data", 0, 0, 0);
         return AEE_ENOMEMORY;
      }

      memset (mEdClone->m_Ed, 0, sizeof (DSSEventHandler::EventData));
   }

   IDS_ERR_RET(InitEventData(mEd->m_Ed,bReg,userCB,userData));

   return AEE_SUCCESS;
}

AEEResult DSSEventHandler::InitEventData
(
   EventData* pEd,
   bool bReg,
   dss_iface_ioctl_event_cb userCB,
   void* userData
)
{
   // if we are trying to register and we are already registered
   if (bReg && pEd->bReg) {
      return QDS_EINPROGRESS;
   }

   // if we are trying to deregister and we not registered
   if (!bReg && !pEd->bReg) {
      return QDS_ENOTCONN;
   }

   pEd->bReg = bReg;
   pEd->userCB = userCB;
   pEd->userData = userData;

   return AEE_SUCCESS;

}


AEEResult DSSEventHandler::Register(dss_iface_ioctl_event_enum_type event, dss_iface_ioctl_event_cb userCB, void* userData)
{
   AEEResult res;
   LOG_MSG_FUNCTION_ENTRY("DSSEventHandler::Register(): event %d", event, 0, 0);

   if (NULL == userCB) {
      LOG_MSG_ERROR("Cannot register with NULL callback", 0, 0, 0);
      return QDS_EFAULT;
   }

   {
      DSSCritScope cs(*piCritSect);

      res = SetEventData(event,
                         true,
                         userCB,
                         userData);

      if (AEE_SUCCESS != res) {
         if (QDS_EINPROGRESS == res ||
             QDS_ENOTCONN == res) {
            return res;
         } else {
            return QDS_EFAULT;
         }
      }
   }

   // Before calling this function, bReg flag need to be true.
   res = RegisterIDL();
   if (AEE_SUCCESS != res ) {
     // in case (QDS_EINUSE == res) the Signal is already on corresponding
     // signal bus - so we shall not clean event data
     if(QDS_EINUSE != res){
         DSSCritScope cs(*piCritSect);

         // in case RegisterIDL API fails we are only interested
         // in result of RegisterIDL and not in result of SetEventData below
         // error messages are printed inside SetEventData in case of errors
         (void) SetEventData(event, false, 0, 0);
     }else{
        // registration succeeded from the point of view of DSS
        res = AEE_SUCCESS;
     }
   }
   return res;
}

AEEResult DSSEventHandler::DeRegister(dss_iface_ioctl_event_enum_type event)
{
   DSSCritScope cs(*piCritSect);
   AEEResult res;

   LOG_MSG_FUNCTION_ENTRY("DSSEventHandler::DeRegister(): event %d", event, 0, 0);

   res = SetEventData(event,
                      false,
                      0,
                      0);

   return res;
}

/*lint -e{1551} */
void DSSEventHandler::Destructor() throw()
{
   DSSCommon::ReleaseIf((IQI**)&piSignal);
   DSSCommon::ReleaseIf((IQI**)&piSignalCtl);
   PS_SYSTEM_HEAP_MEM_FREE (mEd->m_Ed);
   PS_SYSTEM_HEAP_MEM_FREE (mEd);
   PS_SYSTEM_HEAP_MEM_FREE (mEdClone->m_Ed);
   PS_SYSTEM_HEAP_MEM_FREE (mEdClone);
   DSSCommon::ReleaseIf((IQI**)&piCritSect);

   SignalHandlerBase::Destructor();
}
/*lint -e{1531} */

/* should have compared argument against sizeof(class) - disabled since the comparison below is done according to the special memory management */
void * DSSEventHandler::operator new
(
 unsigned int numBytes
 ) throw()
{
   // We need to make sure that all the handlers are of the same size
   // and that they are equal to EVENT_HANDLER_SIZE
   if (EVENT_HANDLER_SIZE != numBytes){
      LOG_MSG_FATAL_ERROR ("Can't allocate, the size of the handler should be EVENT_HANDLER_SIZE = %d", EVENT_HANDLER_SIZE, 0, 0);
      ASSERT(0);
      return NULL;
   }
   return ps_mem_get_buf(PS_MEM_DSAL_EVENT_HANDLER);
} /* DSSEventHandler::operator new() */

