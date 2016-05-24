#ifndef __DSS_PRIVIPV6ADDRHANDLER_H__
#define __DSS_PRIVIPV6ADDRHANDLER_H__

/*====================================================

FILE:  DSS_PrivIpv6AddrHandler.h

SERVICES:
   Handle private Ipv6 Adress events

=====================================================

Copyright (c) 2008 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_PrivIpv6AddrHandler.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_EventHandlerPrivIPV6Addr.h"

class DSSPrivIpv6AddrHandler : public DSSEventHandlerPrivIpv6Addr
{
protected:
   virtual void EventOccurred();
   virtual AEEResult RegisterIDL();

public:
   static DSSPrivIpv6AddrHandler* CreateInstance();
   DSSPrivIpv6AddrHandler();
};

#endif // __DSS_PRIVIPV6ADDRHANDLER_H__
