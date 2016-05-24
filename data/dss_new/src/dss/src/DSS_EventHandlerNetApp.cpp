/*======================================================

FILE:  DSS_EventHandlerNetApp.cpp

GENERAL DESCRIPTION:
   Implementation of DSS_EventHandlerNetApp functions

=====================================================

Copyright (c) 2008 - 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_EventHandlerNetApp.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_EventHandlerNetApp.h"
#include "DSS_Globals.h"
#include "DSS_Common.h"
#include "DSS_CritScope.h"
#include "DSS_EventHandlerNetApp.h"
#include "ds_Utils_CreateInstance.h"
#include "AEECCritSect.h"

void DSSEventHandlerNetApp::DispatchCB(dss_iface_ioctl_event_enum_type event, EventData* ped,
                                 dss_iface_ioctl_event_info_union_type* eventInfo)
{
   dss_iface_id_type ifaceId = 0;

   LOG_MSG_INFO1("DispachCB of event number %d", event,0,0);

   parentNetApp->GetIfaceId(&ifaceId);

   // in case iface_id was cleared by IFACE_DOWN event
   // fetch the saved one from DSSNetApp
   if(DSS_IFACE_INVALID_ID == ifaceId){
      parentNetApp->GetEventReportIfaceId(&ifaceId);
      LOG_MSG_INFO1("DSSEventHandlerNetApp::DispatchCB - iface_id was cleared, providing saved iface_id : 0x%x",
        ifaceId, 0, 0);
   }

   sint15 netHandle;
   parentNetApp->GetNetHandle(&netHandle);

   LOG_MSG_INFO1("DSS calld App callback, event: %d, ifaceId: %u, netHandle: %d", event, ifaceId, netHandle);
   ped->userCB(event, *eventInfo, ped->userData, netHandle, ifaceId);
}

AEEResult DSSEventHandlerNetApp::Init(DSSNetApp* parentNetAppParam)
{
   parentNetApp = parentNetAppParam;
   (void) parentNetApp->AddRefWeak();

   ISignalFactory *piSignalFactory = 0;
   IDS_ERR_RET(DSSGlobals::Instance()->GetSignalFactory(&piSignalFactory));
   IDS_ERR_RET(piSignalFactory->CreateSignal(&signalHandler,
                                             reinterpret_cast <uint32> (SignalCB),
                                             reinterpret_cast <uint32> (this),
                                             &piSignal, &piSignalCtl));

   DSSCommon::ReleaseIf((IQI**)&(piSignalFactory));

   IDS_ERR_RET(DSSGlobals::Instance()->GetCritSect(&piCritSect));

   return AEE_SUCCESS;
}

void DSSEventHandlerNetApp::Destructor() throw()
{
   DS_UTILS_RELEASE_WEAKREF_IF(parentNetApp);

   DSSEventHandler::Destructor();
}

