/*======================================================

FILE:  DSS_PhysLinkStateHandler.cpp

GENERAL DESCRIPTION:
   Implementation of DSS_PhysLinkStateHandler functions

=====================================================

Copyright (c) 2008 - 2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_IPv6PrefixChangedStateHandler.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_IPv6PrefixChangedStateHandler.h"
#include "DSS_Common.h"
#include "DSS_IDSNetworkScope.h"
#include "DSS_GenScope.h"
#include "ps_in.h"
#include "ps_ip6_addr.h"
#include "ps_system_heap.h"

using namespace ds::Net;

DSSIPv6PrefixChangedStateHandler::DSSIPv6PrefixChangedStateHandler()
{
   mEt = EVENT_HANDLER_IPV6_PREFIX;
}

void DSSIPv6PrefixChangedStateHandler::Destructor() throw()
{
   PS_SYSTEM_HEAP_MEM_FREE (((DSSIPv6PrefixHndlData*)mEd)->mPrevPrefixes);
   PS_SYSTEM_HEAP_MEM_FREE (((DSSIPv6PrefixHndlData*)mEdClone)->mPrevPrefixes);
   DSSEventHandlerNetApp::Destructor();
}

AEEResult DSSIPv6PrefixChangedStateHandler::SetEventData(dss_iface_ioctl_event_enum_type event,
                                  bool bReg,
                                  dss_iface_ioctl_event_cb userCB,
                                  void* userData)
{
   if (NULL == mEd) {
      mEd = (DSSIPv6PrefixHndlData *)ps_system_heap_mem_alloc(sizeof(DSSIPv6PrefixHndlData));
      if (NULL == mEd) {
         LOG_MSG_ERROR("Can't allocate event data", 0, 0, 0);
         return AEE_ENOMEMORY;
      }
      
      mEd->m_Ed = NULL;
      ((DSSIPv6PrefixHndlData *)mEd)->mPrevPrefixes = NULL;
      ((DSSIPv6PrefixHndlData *)mEd)->mNumPrevPrefixes = 0;
   }

   if (NULL == mEdClone) {
      mEdClone = (DSSIPv6PrefixHndlData *)ps_system_heap_mem_alloc(sizeof(DSSIPv6PrefixHndlData));
      if (NULL == mEdClone) {
         LOG_MSG_ERROR("Can't allocate event data", 0, 0, 0);
         return AEE_ENOMEMORY;
      }
      
      mEdClone->m_Ed = NULL;
      ((DSSIPv6PrefixHndlData *)mEdClone)->mPrevPrefixes = NULL;
      ((DSSIPv6PrefixHndlData *)mEdClone)->mNumPrevPrefixes = 0;
   }

   
   PS_SYSTEM_HEAP_MEM_FREE(((DSSIPv6PrefixHndlData*)mEd)->mPrevPrefixes);
   ((DSSIPv6PrefixHndlData*)mEd)->mNumPrevPrefixes = 0;
   PS_SYSTEM_HEAP_MEM_FREE(((DSSIPv6PrefixHndlData*)mEdClone)->mPrevPrefixes);
   ((DSSIPv6PrefixHndlData*)mEdClone)->mNumPrevPrefixes = 0;

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

   IDS_ERR_RET(InitEventData(mEd->m_Ed,bReg,userCB,userData));

   return AEE_SUCCESS;
}

AEEResult DSSIPv6PrefixChangedStateHandler::GetNetworkIPv6Priv(INetworkIPv6Priv** ppiNetworkIPv6Priv)
{
   DSSIDSNetworkScope IDSNetworkScope;
   IDS_ERR_RET(IDSNetworkScope.Init(parentNetApp));
   IDS_ERR_RET(IDSNetworkScope.Fetch()->GetTechObject(AEEIID_INetworkIPv6Priv, (void**)ppiNetworkIPv6Priv));
   return AEE_SUCCESS;
}


void DSSIPv6PrefixChangedStateHandler::EventOccurred()
{
   INetworkIPv6Priv* piNetworkIPv6Priv;

   AEEResult res;

   {
      // for DSSIPv6PrefixChangedStateHandler, event data has different size, so it should copy
      // all the data to mEdClone
      DSSCritScope cs(*piCritSect);
      if ((((DSSIPv6PrefixHndlData*)mEd)->m_Ed)->bReg) {
         
         memcpy(((DSSIPv6PrefixHndlData*)mEdClone)->m_Ed,
                ((DSSIPv6PrefixHndlData*)mEd)->m_Ed,
                sizeof(DSSEventHandler::EventData));
         
         if (0 != ((DSSIPv6PrefixHndlData*)mEd)->mNumPrevPrefixes) {
            PS_SYSTEM_HEAP_MEM_FREE (((DSSIPv6PrefixHndlData*)mEdClone)->mPrevPrefixes);

            ((DSSIPv6PrefixHndlData*)mEdClone)->mPrevPrefixes = 
               ((DSSIPv6PrefixHndlData*)mEd)->mPrevPrefixes;
         
         ((DSSIPv6PrefixHndlData*)mEdClone)->mNumPrevPrefixes = ((DSSIPv6PrefixHndlData*)mEd)->mNumPrevPrefixes;
            ((DSSIPv6PrefixHndlData*)mEd)->mPrevPrefixes = NULL;
            ((DSSIPv6PrefixHndlData*)mEd)->mNumPrevPrefixes = 0;
         }
         else {
           ((DSSIPv6PrefixHndlData*)mEdClone)->mPrevPrefixes = NULL;
           ((DSSIPv6PrefixHndlData*)mEdClone)->mNumPrevPrefixes = 0;
         }

      }
      else {
         return;
      }
   } // release lock

   DSSWeakRefScope WeakRefScope;

   if(!WeakRefScope.Init(parentNetApp)) {
      return;
   }

   res = GetNetworkIPv6Priv(&piNetworkIPv6Priv);
   DSSGenScope scopeNetworkIPv6Priv(piNetworkIPv6Priv,DSSGenScope::IDSIQI_TYPE);

   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("GetNetworkIPv6Priv() failed: %d", res, 0, 0);
      return;
   }

   IPv6PrivPrefixInfoType* prevPrefixes = ((DSSIPv6PrefixHndlData*)mEdClone)->mPrevPrefixes;
   int numCurrPrefixes = 0;
   IPv6PrivPrefixInfoType* currPrefixes = NULL;
   res = GetAllPrefixes(piNetworkIPv6Priv,&currPrefixes,&numCurrPrefixes);

   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("GetAllPrefixes failed ", 0, 0, 0);
      return;
   }

   // now we need to compare current and previous prefixes arrays
   // stage 1 , pass the currPrefix array and locate all prefixes appearing also inside prevPrefix array but with a different state
   int currInd = 0;  int prevInd = 0;
   bool nextCurr = false;

   int numPrevPrefixes = (((DSSIPv6PrefixHndlData*)mEdClone)->mNumPrevPrefixes);
   IPv6PrivPrefixInfoType prev,curr;

   while (currInd<numCurrPrefixes) {
      curr = currPrefixes[currInd];
      nextCurr = false;
      while ((prevInd< numPrevPrefixes) && (!nextCurr)){
         prev = prevPrefixes[prevInd];
         // we've already found this one , skip it
         if (!prev.prefixLen) {
            prevInd++ ;
            continue ;
         }

         IPV6PrefixCompareResult resPrefix = ComparePrefixes(&prev,&curr);
         switch (resPrefix) {
            case PREFIXES_EQUAL:
               // we have a match , now mark the prefix in prevPrefix array , so that we skip it in the future
               prev.prefixLen = 0;
               // continue to the next currInd
               nextCurr = true;
               continue;
            case PREFIXES_DIFFERENT:
               // nothing to do here , continue
               prevInd++;
               continue;
            case PREFIXES_STATE_CHANGED:
               // we have a match , now mark the prefix in prevPrefix array , so that we skip it in the future
               prev.prefixLen = 0;
               // fire callback and continue to the next currInd
               if ( (IPv6AddrState::PRIV_ADDR_AVAILABLE == prev.prefixType) && (IPv6AddrState::PRIV_ADDR_WAITING == curr.prefixType)) {
                  LOG_MSG_ERROR("Wrong prefix state sequence:%d %d", prev.prefixType, curr.prefixType, 0);
                  goto bail;
               }
               if ( (IPv6AddrState::PRIV_ADDR_AVAILABLE == prev.prefixType) && (IPv6AddrState::PRIV_ADDR_DEPRECATED == curr.prefixType)) {
                  ProcessCB(&curr,DSS_IFACE_IOCTL_PREFIX_DEPRECATED);
               }
               if ( (IPv6AddrState::PRIV_ADDR_WAITING == prev.prefixType) && (IPv6AddrState::PRIV_ADDR_AVAILABLE == curr.prefixType)) {
                  ProcessCB(&curr,DSS_IFACE_IOCTL_PREFIX_ADDED);
               }
               if ( (IPv6AddrState::PRIV_ADDR_WAITING == prev.prefixType) && (IPv6AddrState::PRIV_ADDR_DEPRECATED == curr.prefixType)) {
                  ProcessCB(&curr,DSS_IFACE_IOCTL_PREFIX_ADDED);
                  ProcessCB(&curr,DSS_IFACE_IOCTL_PREFIX_DEPRECATED);
               }
               if ( (IPv6AddrState::PRIV_ADDR_DEPRECATED == prev.prefixType) && (IPv6AddrState::PRIV_ADDR_AVAILABLE == curr.prefixType)) {
                  ProcessCB(&curr,DSS_IFACE_IOCTL_PREFIX_UPDATED);
               }
               if ( (IPv6AddrState::PRIV_ADDR_DEPRECATED == prev.prefixType) && (IPv6AddrState::PRIV_ADDR_WAITING == curr.prefixType)) {
                  LOG_MSG_ERROR("Wrong prefix state sequence:%d %d", prev.prefixType, curr.prefixType, 0);
                  goto bail;
               }
               nextCurr = true;
               continue;
            default:
               continue;
         } // switch
      } // while (prevInd< numPrevPrefixes)
      currInd++; prevInd=0;
      if (nextCurr) {
         continue;
      }

      // if we got here , it means that we could not find current prefix inside previous prefixes array
      // it's a new one , fire callback and advance to next currInd
      if (IPv6AddrState::PRIV_ADDR_AVAILABLE == curr.prefixType) {
         ProcessCB(&curr,DSS_IFACE_IOCTL_PREFIX_ADDED);
      }
      if (IPv6AddrState::PRIV_ADDR_DEPRECATED == curr.prefixType) {
         ProcessCB(&curr,DSS_IFACE_IOCTL_PREFIX_ADDED);
         ProcessCB(&curr,DSS_IFACE_IOCTL_PREFIX_DEPRECATED);
      }
   } //  while (currInd<numCurrPrefixes)

   // stage 2
   // pass the prevPrefix array and dispatch callback with DSS_IFACE_IOCTL_PREFIX_REMOVED event for all unmarked prefixes
   for (int i=0; i < (((DSSIPv6PrefixHndlData*)mEdClone)->mNumPrevPrefixes); i++) {
      if (!prevPrefixes[i].prefixLen) {
         continue;
      }
      if ((IPv6AddrState::PRIV_ADDR_AVAILABLE == prevPrefixes[i].prefixType) || (IPv6AddrState::PRIV_ADDR_DEPRECATED == prevPrefixes[i].prefixType)) {
         ProcessCB(&prevPrefixes[i],DSS_IFACE_IOCTL_PREFIX_REMOVED);
      }
   }
   
bail:
   PS_SYSTEM_HEAP_MEM_FREE (((DSSIPv6PrefixHndlData*)mEdClone)->mPrevPrefixes);
   ((DSSIPv6PrefixHndlData*)mEdClone)->mNumPrevPrefixes = 0;
   
   // copy all back to mEd.   
   DSSCritScope cs(*piCritSect);
   if ((((DSSIPv6PrefixHndlData*)mEd)->m_Ed)->bReg) {
      PS_SYSTEM_HEAP_MEM_FREE (((DSSIPv6PrefixHndlData*)mEd)->mPrevPrefixes);
      ((DSSIPv6PrefixHndlData*)mEd)->mPrevPrefixes = currPrefixes;
      ((DSSIPv6PrefixHndlData*)mEd)->mNumPrevPrefixes = numCurrPrefixes;
   }
}


AEEResult DSSIPv6PrefixChangedStateHandler::RegisterIDL()
{
   INetworkIPv6Priv* piNetworkIPv6Priv = NULL;
   
   DSSWeakRefScope WeakRefScope;

   if(!WeakRefScope.Init(parentNetApp)) {
      return AEE_EFAILED;
   }

   IDS_ERR_RET(GetNetworkIPv6Priv(&piNetworkIPv6Priv));
   DSSGenScope scopeNetworkIPv6Priv(piNetworkIPv6Priv,DSSGenScope::IDSIQI_TYPE);

   PS_SYSTEM_HEAP_MEM_FREE (((DSSIPv6PrefixHndlData*)mEd)->mPrevPrefixes);
   AEEResult nRet = GetAllPrefixes(piNetworkIPv6Priv,&(((DSSIPv6PrefixHndlData*)mEd)->mPrevPrefixes),&(((DSSIPv6PrefixHndlData*)mEd)->mNumPrevPrefixes));

   if (AEE_SUCCESS != nRet) {
      LOG_MSG_ERROR("Could not get all prefixes while registering: %d", nRet, 0, 0);
      return nRet;
   }

   LOG_MSG_INFO1("Registering to QDS_EV_PREFIX_CHANGED, NetworkIPv6Priv obj 0x%p", piNetworkIPv6Priv, 0, 0);
   nRet = piNetworkIPv6Priv->OnStateChange(piSignal, IPv6PrivEvent::QDS_EV_PREFIX_CHANGED, &mRegObj);
   if (AEE_SUCCESS != nRet) {
      LOG_MSG_ERROR("Could not register for PREFIX_CHANGED event: %d", nRet, 0, 0);
      return nRet;
   }

   return nRet;
}


AEEResult DSSIPv6PrefixChangedStateHandler::GetAllPrefixes(INetworkIPv6Priv* piNetworkIPv6Priv ,IPv6PrivPrefixInfoType** prefixes,int* num)
{
   int temp;
   AEEResult nRet = AEE_SUCCESS;
   IDS_ERR_RET(piNetworkIPv6Priv->GetAllIPv6Prefixes(NULL,0,num));
   // In case there are no prefixes, just return success
   if (0 == *num) {
      return AEE_SUCCESS;
   }

   *prefixes = (IPv6PrivPrefixInfoType *)ps_system_heap_mem_alloc(sizeof(IPv6PrivPrefixInfoType)*(*num));

   if (NULL == *prefixes) {
      LOG_MSG_ERROR("Can't allocate prefixes ", 0, 0, 0);
      return AEE_ENOMEMORY;
   }
   BAIL_ERR(piNetworkIPv6Priv->GetAllIPv6Prefixes(*prefixes,*num,&temp));

   return nRet;

bail:
   PS_SYSTEM_HEAP_MEM_FREE (*prefixes);

   return nRet;
}


DSSIPv6PrefixChangedStateHandler::IPV6PrefixCompareResult
DSSIPv6PrefixChangedStateHandler::ComparePrefixes(IPv6PrivPrefixInfoType* source, IPv6PrivPrefixInfoType* dest)
{
   if (source->prefixLen != dest->prefixLen) {
      return PREFIXES_DIFFERENT;
   }

   if (!IN6_ARE_PREFIX_EQUAL((ps_in6_addr*)(source->prefix),(ps_in6_addr*)(dest->prefix),source->prefixLen)) {
      return PREFIXES_DIFFERENT;
   }

   if (source->prefixType != dest->prefixType) {
      return PREFIXES_STATE_CHANGED;
   } else {
      return PREFIXES_EQUAL;
   }
}


void DSSIPv6PrefixChangedStateHandler::ProcessCB(IPv6PrivPrefixInfoType* prefixStruct,dss_iface_ioctl_prefix_update_enum_type event)
{
   dss_iface_ioctl_event_info_union_type eventInfo;
   dss_iface_ioctl_event_enum_type eventStatus = DSS_IFACE_IOCTL_PREFIX_UPDATE_EV;
   eventInfo.prefix_info.kind = event;
   eventInfo.prefix_info.prefix_len = (uint8) prefixStruct->prefixLen;
   memcpy(&eventInfo.prefix_info.prefix,prefixStruct->prefix,sizeof(ps_in6_addr));

   DSSEventHandlerNetApp::DispatchCB(eventStatus, ((DSSIPv6PrefixHndlData*)mEd)->m_Ed, &eventInfo);

}

DSSIPv6PrefixChangedStateHandler* DSSIPv6PrefixChangedStateHandler::CreateInstance()
{
   return new DSSIPv6PrefixChangedStateHandler;
}
