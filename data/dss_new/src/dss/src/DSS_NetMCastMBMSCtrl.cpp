/*======================================================

FILE:  DSS_NetMCastMBMSCtrl.cpp

SERVICES:

GENERAL DESCRIPTION:
Implementation of DSSNetMCastMBMSCtrl class

=====================================================

Copyright (c) 2008 - 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_NetMCastMBMSCtrl.cpp#1 $
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

#include "DSS_EventHandlerMCastMBMSCtrl.h"
#include "DSS_NetMCastMBMSCtrl.h"
#include "DSS_MemoryManagement.h"

#include "DSS_Globals.h"
#include "DSS_CritScope.h"
#include "DSS_EventHandler.h"
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
//            DSSNetMCastMBMSCtrl Functions Definitions
//===================================================================

DSSNetMCastMBMSCtrl::DSSNetMCastMBMSCtrl(IMCastMBMSCtrlPriv* pMCastMBMSCtrl, uint32 MCastMBMSHandle):
   refCnt(1), weakRefCnt(1), mpNetMCastMBMSCtrl(pMCastMBMSCtrl), mNext(NULL), mMCastMBMSHandle(MCastMBMSHandle),
   mpMCastMBMSHandler(NULL),mparentNetApp(NULL)
{
   if (NULL != pMCastMBMSCtrl) {
      (void)mpNetMCastMBMSCtrl->AddRef();
   }
}

AEEResult DSSNetMCastMBMSCtrl::GetDSSNetApp(DSSNetApp** ppDSSNetApp) {
   if(!mparentNetApp->GetStrongRef())
   {
      *ppDSSNetApp = mparentNetApp;
      return AEE_EFAILED;
   }

   *ppDSSNetApp = mparentNetApp;
   return AEE_SUCCESS;
}

void DSSNetMCastMBMSCtrl::SetDSSNetApp(DSSNetApp* pDSSNetApp) {
   mparentNetApp = pDSSNetApp;
   mparentNetApp->AddRefWeak();
}

//===================================================================
//  FUNCTION:   DSSNetMCastMBMSCtrl::~DSSNetMCastMBMSCtrl
//
//  DESCRIPTION:
//  Destructor of the DSSNetMCastMBMSCtrl class.
//===================================================================

/*lint -e{1551} */
void DSSNetMCastMBMSCtrl::Destructor() throw()
{
   DS_UTILS_RELEASE_WEAKREF_IF(mparentNetApp);
   // release the MCast Session
   //mpNetMCastSession->Close();
   DSSCommon::ReleaseIf((IQI**)&mpNetMCastMBMSCtrl);

   // Pass the 8 LSBits of mFlowID to ReleaseFlowID since they
   // represent the flow ID that is now being released.
   DSSGlobals::ReleaseMCastFlowID((uint8)(mMCastMBMSHandle & 0xFF));

   PS_MEM_RELEASE(mpMCastMBMSHandler);

}
/*lint –restore */

//===================================================================
//  FUNCTION:   DSSNetMCastMBMSCtrl::InsertToSecList
//
//  DESCRIPTION:
//
//===================================================================

void DSSNetMCastMBMSCtrl::InsertToList(DSSNetMCastMBMSCtrl* pDSSMCastMBMS)
{
   mNext = pDSSMCastMBMS;
}

//===================================================================
//  FUNCTION:   DSSMCast::GetMCastHandle
//
//  DESCRIPTION:
//
//===================================================================
void DSSNetMCastMBMSCtrl::GetMCastMBMSHandle(uint32* pMCastMBMSHandle)
{
   *pMCastMBMSHandle = mMCastMBMSHandle;
}

//===================================================================
//  FUNCTION:   DSSMCast::RegEventCB
//
//  DESCRIPTION:
//
//===================================================================
AEEResult DSSNetMCastMBMSCtrl::RegEventCB(dss_iface_ioctl_event_cb cback_fn, void* user_data)
{
   DSSEventHandler* ppEventHandler = 0;

   IDS_ERR_RET(GetEventHandler(DSS_IFACE_IOCTL_MBMS_CONTEXT_ACT_SUCCESS_EV, &ppEventHandler, true));

   // Register to the event.
   IDS_ERR_RET(ppEventHandler->Register(DSS_IFACE_IOCTL_MBMS_CONTEXT_ACT_SUCCESS_EV, cback_fn, user_data));

   return AEE_SUCCESS;
}

//===================================================================
//  FUNCTION:    DSSMCast::GetEventHandler
//
//  DESCRIPTION: Returns the DSSEventHandler for the specified event.
//               If bInit is true, this function will also initialize
//               the handler if it's not initialized.
//===================================================================
AEEResult DSSNetMCastMBMSCtrl::GetEventHandler(dss_iface_ioctl_event_enum_type event, DSSEventHandler** ppEventHandler, bool bInit)
{
   switch (event) {
      case DSS_IFACE_IOCTL_MBMS_CONTEXT_ACT_SUCCESS_EV:
      case DSS_IFACE_IOCTL_MBMS_CONTEXT_ACT_FAILURE_EV:
      case DSS_IFACE_IOCTL_MBMS_CONTEXT_DEACT_SUCCESS_EV:
      case DSS_IFACE_IOCTL_MBMS_CONTEXT_DEACT_FAILURE_EV:
         return FetchHandler(&mpMCastMBMSHandler, ppEventHandler, bInit);

      default:
         return QDS_EFAULT;
   }
}

template<typename HandlerType>
inline AEEResult DSSNetMCastMBMSCtrl::FetchHandler(HandlerType** pHandler, DSSEventHandler** ppEventHandler, bool bInit)
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

void * DSSNetMCastMBMSCtrl::operator new
(
   unsigned int numBytes
)  throw()
{
   return ps_mem_get_buf( PS_MEM_DSAL_NET_MCAST_MBMS_CTRL);
} /* DSSNetMCastMBMSCtrl::operator new() */


void DSSNetMCastMBMSCtrl::operator delete
(
   void *  bufPtr
)
{
   PS_MEM_FREE(bufPtr);
   return;
} /* DSSNetMCastMBMSCtrl::operator delete() */



//===================================================================
