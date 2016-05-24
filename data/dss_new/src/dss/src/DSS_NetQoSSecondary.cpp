/*======================================================

FILE:  DSS_NetQoSSecondary.cpp

SERVICES:

GENERAL DESCRIPTION:
Implementation of DSSNetQoSSecondary class

=====================================================

Copyright (c) 2008 - 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_NetQoSSecondary.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

//===================================================================
//   Includes and Public Data Declarations
//===================================================================

//-------------------------------------------------------------------
// Include Files
//-------------------------------------------------------------------
#include "DSS_Common.h"
#include "dserrno.h"

#include "DSS_NetQoSSecondary.h"
#include "DSS_Globals.h"
#include "DSS_CritScope.h"
#include "DSS_EventHandler.h"
#include "DSS_QoSHandler.h"
#include "DSS_QoSModifyHandler.h"
#include "DSS_QoSInfoCodeUpdatedHandler.h"
#include "DSS_MemoryManagement.h"
#include "ds_Errors_Def.h"

using namespace ds::Net;
using namespace ds::Error;
//-------------------------------------------------------------------
// Constant / Define Declarations
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// Type Declarations (typedef, struct, enum, etc.)
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// Global Constant Data Declarations
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// Global Data Declarations
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// Forward Declarations
//-------------------------------------------------------------------

//===================================================================
//              Macro Definitions
//===================================================================

//===================================================================
//            DSSNetQoSSecondary Functions Definitions
//===================================================================

// TODO: documentation
DSSNetQoSSecondary::DSSNetQoSSecondary(IQoSSecondary* pNetQoSSecondary, uint32 flowID):
   refCnt(1), weakRefCnt(1), mpNetQoSSecondary(pNetQoSSecondary), mNext(NULL), mFlowID(flowID),
   mpNetQoSHandler(NULL),mpNetQoSModifyHandler(NULL),mpQoSInfoCodeUpdatedHandler(NULL), mparentNetApp(NULL)
{
   if (NULL != pNetQoSSecondary) {
      (void)mpNetQoSSecondary->AddRef();
   }
}

AEEResult DSSNetQoSSecondary::GetDSSNetApp(DSSNetApp** ppDSSNetApp) {
   if(!mparentNetApp->GetStrongRef())
   {
     *ppDSSNetApp = mparentNetApp;
     return AEE_EFAILED;
   }

   *ppDSSNetApp = mparentNetApp;
   return AEE_SUCCESS;
}

void DSSNetQoSSecondary::SetDSSNetApp(DSSNetApp* pDSSNetApp) {
   mparentNetApp = pDSSNetApp;
   mparentNetApp->AddRefWeak();
}

//===================================================================
//  FUNCTION:   DSSNetQoSSecondary::~DSSNetQoSSecondary
//
//  DESCRIPTION:
//  Destructor of the DSSNetQoSSecondary class.
//===================================================================

/*lint -e{1551} */
 void DSSNetQoSSecondary::Destructor() throw()
{

   DS_UTILS_RELEASE_WEAKREF_IF(mparentNetApp);
   DSSCommon::ReleaseIf((IQI**)&mpNetQoSSecondary);

   // Pass the 8 LSBits of mFlowID to ReleaseFlowID since they
   // represent the flow ID that is now being released.
   DSSGlobals::ReleaseQoSFlowID((uint8)(mFlowID & 0xFF));

   PS_MEM_RELEASE(mpNetQoSHandler);
   PS_MEM_RELEASE(mpNetQoSModifyHandler);
   PS_MEM_RELEASE(mpQoSInfoCodeUpdatedHandler);
}
/*lint –restore */

//===================================================================
//  FUNCTION:   DSSNetQoSSecondary::InsertToSecList
//
//  DESCRIPTION:
//
//===================================================================

void DSSNetQoSSecondary::InsertToList(DSSNetQoSSecondary* pDSSQoS)
{
   mNext = pDSSQoS;
}

//===================================================================
//  FUNCTION:   DSSNetQoSSecondary::GetFlowID
//
//  DESCRIPTION:
//
//===================================================================
void DSSNetQoSSecondary::GetFlowID(uint32* pFlowID)
{
   *pFlowID = mFlowID;
}
//===================================================================



AEEResult DSSNetQoSSecondary::GetNetQoSSecondary(IQoSSecondary** ppNetQoSSecondary)
{
   if (NULL == mpNetQoSSecondary) {
      return QDS_EINVAL;
   }
   *ppNetQoSSecondary = mpNetQoSSecondary;
   (void)mpNetQoSSecondary->AddRef();
   return AEE_SUCCESS;
}



//===================================================================
//  FUNCTION:    DSSNetQoSSecondary::RegEventCB
//
//  DESCRIPTION: Serves DSS_IFACE_IOCTL_REG_EVENT_CB
//===================================================================
AEEResult DSSNetQoSSecondary::RegEventCB(dss_iface_ioctl_ev_cb_type* pEvArg)
{
   //TODO: implement
   // outline: ???
   // (1)(a) create a network, if necessary.
   // (1)(b) find the right DSSEventHandler instance

   // Done in DSSEventHandler:
   // (2) fetch the right interface for the event.
   // (3) register for the event using a single ISignal (per event "group").
   // (4) save the user's callback, it's just a CB/signal pair because only one CB is allowed per app.
   // (5) when the event comes, the handler dispatches the callback with the needed data.


   DSSEventHandler* ppEventHandler = 0;
   IDS_ERR_RET(GetEventHandler(pEvArg->event, &ppEventHandler, true));

   // Register to the event.
   IDS_ERR_RET(ppEventHandler->Register(pEvArg->event, pEvArg->event_cb, pEvArg->user_data_ptr));

   return AEE_SUCCESS;
}

//===================================================================
//  FUNCTION:    DSSNetQoSSecondary::DeregEventCB
//
//  DESCRIPTION: Serves DSS_IFACE_IOCTL_DEREG_EVENT_CB
//===================================================================
AEEResult DSSNetQoSSecondary::DeregEventCB(dss_iface_ioctl_ev_cb_type* pEvArg)
{
   DSSEventHandler* ppEventHandler = 0;
   IDS_ERR_RET(GetEventHandler(pEvArg->event, &ppEventHandler, false));

   // Deregister to the event.
   if (NULL != ppEventHandler) {
      IDS_ERR_RET(ppEventHandler->DeRegister(pEvArg->event));
   }

   return AEE_SUCCESS;
}
//===================================================================
//  FUNCTION:    DSSNetQoSSecondary::GetEventHandler
//
//  DESCRIPTION: Returns the DSSEventHandler for the specified event.
//               If bInit is true, this function will also initialize
//               the handler if it's not initialized.
//===================================================================
AEEResult DSSNetQoSSecondary::GetEventHandler(dss_iface_ioctl_event_enum_type event, DSSEventHandler** ppEventHandler, bool bInit)
{
   switch (event) {
      case DSS_IFACE_IOCTL_QOS_AVAILABLE_MODIFIED_EV:
      case DSS_IFACE_IOCTL_QOS_AVAILABLE_DEACTIVATED_EV:
      case DSS_IFACE_IOCTL_QOS_UNAVAILABLE_EV:
         return FetchHandler(&mpNetQoSHandler, ppEventHandler, bInit);
      case DSS_IFACE_IOCTL_QOS_MODIFY_ACCEPTED_EV:
      case DSS_IFACE_IOCTL_QOS_MODIFY_REJECTED_EV:
         return FetchHandler(&mpNetQoSModifyHandler, ppEventHandler, bInit);
      case DSS_IFACE_IOCTL_QOS_INFO_CODE_UPDATED_EV:
         return FetchHandler(&mpQoSInfoCodeUpdatedHandler, ppEventHandler, bInit);

      default:
         return QDS_EFAULT;
   }
}

template<typename HandlerType>
inline AEEResult DSSNetQoSSecondary::FetchHandler(HandlerType** pHandler, DSSEventHandler** ppEventHandler, bool bInit)
{
   if (bInit) {
      if (NULL == *pHandler) {
         *pHandler = HandlerType::CreateInstance();
         if (NULL == *pHandler) {
            LOG_MSG_ERROR( "Couldn't allocate HandlerType", 0, 0, 0);
            return AEE_ENOMEMORY;
         }
         IDS_ERR_RET((*pHandler)->Init(this));
      }
   } else {
      // bInit is false , we expect pHandler to have a value here
      if (0 == *pHandler) {
         return QDS_EFAULT;
      }
   }
   *ppEventHandler = *pHandler;
   return AEE_SUCCESS;
}


void * DSSNetQoSSecondary::operator new
(
   unsigned int numBytes
)  throw()
{
   return ps_mem_get_buf( PS_MEM_DSAL_NET_QOS_SECONDARY_TYPE);
} /* DSSNetQoSSecondary::operator new() */


void DSSNetQoSSecondary::operator delete
(
   void *  bufPtr
)
{
   PS_MEM_FREE(bufPtr);
   return;
} /* DSSNetQoSSecondary::operator delete() */

