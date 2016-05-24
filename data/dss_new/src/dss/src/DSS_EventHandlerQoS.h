#ifndef __DSS_EVENTHANDLERQOS_H__
#define __DSS_EVENTHANDLERQOS_H__

/*====================================================

FILE:  DSS_EventHandlerQoS.h

SERVICES:
   Provide a common base class for event handling.

=====================================================

Copyright (c) 2008 - 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_EventHandlerQoS.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_NetQoSSecondary.h"
#include "AEEISignal.h"
#include "AEEISignalCtl.h"
#include "DSS_EventHandler.h"
#include "DSS_WeakRefScope.h"


class DSSEventHandlerQoS : public DSSEventHandler
{
public:
   // This class uses two-phase construction: the constructor is empty and the
   // actual initialization is done through the Init() function, so it can return
   // an error value.
   AEEResult Init(DSSNetQoSSecondary* parentNetAppParam);
   virtual void Destructor() throw();

protected:
   void DispatchCB(dss_iface_ioctl_event_enum_type event, EventData* ped,
                   dss_iface_ioctl_event_info_union_type* eventInfo);

   DSSNetQoSSecondary* parentNetApp;
};

#endif // __DSS_EVENTHANDLERQOS_H__
