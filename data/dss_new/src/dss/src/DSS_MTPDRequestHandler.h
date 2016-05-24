#ifndef __DSS_MTPDREQUESTHANDLER_H__
#define __DSS_MTPDREQUESTHANDLER_H__

/*====================================================

FILE:  DSS_MTPDRequestHandler.h

SERVICES:
   Handle MTPD events.

=====================================================

Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_MTPDRequestHandler.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_EventHandlerNetApp.h"
#include "ds_Net_ITechUMTS.h"

class DSSMTPDRequestHandler : public DSSEventHandlerNetApp
{
protected:
   virtual void EventOccurred();
   virtual AEEResult RegisterIDL();
   virtual AEEResult SetEventData(dss_iface_ioctl_event_enum_type event,
                            bool bReg,
                            dss_iface_ioctl_event_cb userCB,
                            void* userData);

private:
   struct DSSMTPDRequestData : public HandlerData{
      IQI* pMTPDReg;
   };

public:
   static DSSMTPDRequestHandler* CreateInstance();
   DSSMTPDRequestHandler();
   virtual void Destructor() throw();

};

#endif // __DSS_MTPDREQUESTHANDLER_H__
