#ifndef DS_SOCK_EVENTDEFS_H
#define DS_SOCK_EVENTDEFS_H

/*===========================================================================
  @file DS_Sock_EventDefs.h

  This file provides definitions for different event groups and event infos.

  TODO: Write detailed explanation.

  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_EventDefs.h#1 $
  $DateTime: 2009/09/28 14:45:31 $$Author: dmudgal $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-06-22 hm  Created module.

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "DS_Sock_IDoSAckPriv.h"


/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/
namespace DS
{
  namespace Sock
  {
    namespace Event
    {
      typedef enum
      {
        EVENT_GROUP_MIN             = 0,
        EVENT_GROUP_NETWORK         = EVENT_GROUP_MIN,
        EVENT_GROUP_PS_MEM          = 1,
        EVENT_GROUP_MAX
      } EventGroupType;

      typedef struct EventInfo
      {
        EventGroupType    eventGroup;
        int32             eventMask;
        int32             handle;
      } EventInfoType;

      typedef struct DoSAckEventInfo : public EventInfo
      {
        DS::Sock::DoSAckStatusType  dosAckStatus;
        uint32                      overflow;
      } DoSAckEventInfoType;

    } /* namespace Event */
  } /* namespace Sock */
} /* namespace DS */

#endif /* DS_SOCK_EVENTDEFS_H */
