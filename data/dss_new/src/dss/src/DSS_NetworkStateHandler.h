#ifndef __DSS_NETWORKSTATEHANDLER_H__
#define __DSS_NETWORKSTATEHANDLER_H__

/*====================================================

FILE:  DSS_NetworkStateHandler.h

SERVICES:
   Handle network state change events.

=====================================================

Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_NetworkStateHandler.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_EventHandlerNetApp.h"

// This handler have 4 EventData  edDown,edUp, edComingUp, edGoingDown that are stored
// in an array in the mEdClone in EventHandler.
class DSSNetworkStateHandler : public DSSEventHandlerNetApp
{
protected:
   virtual void EventOccurred();
   virtual AEEResult RegisterIDL();
   virtual AEEResult SetEventData(dss_iface_ioctl_event_enum_type event,
                            bool bReg,
                            dss_iface_ioctl_event_cb userCB,
                            void* userData);
private:
   // TODO: move the following 2 to DSSConversion
   static dss_iface_ioctl_event_enum_type NetStateToEvent(ds::Net::NetworkStateType netState);
   static dss_iface_ioctl_event_info_type NetStateToIfaceState(ds::Net::NetworkStateType netState);

public:
   static DSSNetworkStateHandler* CreateInstance();
   DSSNetworkStateHandler();

};

#endif // __DSS_NETWORKSTATEHANDLER_H__
