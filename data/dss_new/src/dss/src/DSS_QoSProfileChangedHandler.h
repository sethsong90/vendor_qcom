#ifndef __DSS_QOSPROFILECHANGEDHANDLER_H__
#define __DSS_QOSPROFILECHANGEDHANDLER_H__

/*====================================================

FILE:  DSS_QoSProfileChangedHandler.h

SERVICES:
   Handle network extended IP config events.

=====================================================

Copyright (c) 2008 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_QoSProfileChangedHandler.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_EventHandlerNetApp.h"

class DSSQoSProfileChangedHandler : public DSSEventHandlerNetApp
{
protected:
   virtual void EventOccurred();
   virtual AEEResult RegisterIDL();

public:
   static DSSQoSProfileChangedHandler* CreateInstance();
   DSSQoSProfileChangedHandler();
};

#endif // __DSS_QOSPROFILECHANGEDHANDLER_H__
