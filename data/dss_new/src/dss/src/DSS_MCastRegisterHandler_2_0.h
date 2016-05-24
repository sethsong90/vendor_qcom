#ifndef __DSS_MCASTREGISTERHANDLER_2_0_H__
#define __DSS_MCASTREGISTERHANDLER_2_0_H__

/*====================================================

FILE:  DSS_MCastRegisterHandler_2_0.h

SERVICES:
Handle MCast Registration BCMCS rev 2.0 events.

=====================================================

Copyright (c) 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_MCastRegisterHandler_2_0.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-07-29 vm  Module created.

===========================================================================*/

#include "DSS_EventHandlerMCast.h"

class DSSMCastRegisterHandler2_0 : public DSSEventHandlerMCast
{
protected:

   virtual void EventOccurred();
   virtual AEEResult RegisterIDL();

public:

   DSSMCastRegisterHandler2_0();
   static DSSMCastRegisterHandler2_0* CreateInstance();

};

#endif // __DSS_MCASTREGISTERHANDLER_2_0_H__
