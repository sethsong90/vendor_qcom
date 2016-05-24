#ifndef __DSS_EVENTHANDLERPRIVIPV6ADDR_H__
#define __DSS_EVENTHANDLERPRIVIPV6ADDR_H__

/*====================================================

FILE:  DSS_EventHandlerPrivIpv6Addr.h

SERVICES:
   Provide a common base class for event handling.

=====================================================

Copyright (c) 2009 - 2010 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_EventHandlerPrivIPV6Addr.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_PrivIpv6Addr.h"
#include "DSS_EventHandler.h"
#include "DSS_WeakRefScope.h"

class DSSEventHandlerPrivIpv6Addr : public DSSEventHandler
{
public:
   // This class uses two-phase construction: the constructor is empty and the
   // actual initialization is done through the Init() function, so it can return
   // an error value.
  AEEResult Init(DSSPrivIpv6Addr* parentNetAppParam);
  virtual void Destructor() throw();

protected:
   void DispatchCB(dss_iface_ioctl_event_enum_type event,
                   EventData* ped,
                   dss_iface_ioctl_event_info_union_type* eventInfo);

   DSSPrivIpv6Addr* parentNetApp;

};
  
#endif // __DSS_EVENTHANDLERPRIVIPV6ADDR_H__
