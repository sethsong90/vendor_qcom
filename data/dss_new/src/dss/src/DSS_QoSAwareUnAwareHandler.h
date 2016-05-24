#ifndef __DSS_QOSAWAREUNAWAREHANDLER_H__
#define __DSS_QOSAWAREUNAWAREHANDLER_H__

/*====================================================

FILE:  DSS_QoSAwareUnAwareHandler.h

SERVICES:
   Handle QoS Aware/UnAware events.

=====================================================

Copyright (c) 2008 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_QoSAwareUnAwareHandler.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_EventHandlerNetApp.h"

// This handler have 2 EventData  edAware,edUnAware that are stored
// in an array in the mEdClone in EventHandler.
class DSSQoSAwareUnAwareHandler : public DSSEventHandlerNetApp
{
protected:
   virtual void EventOccurred();
   virtual AEEResult RegisterIDL();
   virtual AEEResult SetEventData(dss_iface_ioctl_event_enum_type event,
                            bool bReg,
                            dss_iface_ioctl_event_cb userCB,
                            void* userData);
public:
   static DSSQoSAwareUnAwareHandler* CreateInstance();
   DSSQoSAwareUnAwareHandler();
};

#endif // __DSS_QOSAWAREUNAWAREHANDLER_H__
