#ifndef DS_SOCK_EVENTMANAGER_H
#define DS_SOCK_EVENTMANAGER_H

/*===========================================================================
  @file EventManager.h

  This file provides the EventManager class.

  TODO: Write detailed explanation.

  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_EventManager.h#1 $
  $DateTime: 2009/09/28 14:45:31 $$Author: dmudgal $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-05-02 hm  Created module.

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"


/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/
namespace DS
{
  namespace Sock
  {
    namespace EventManager
    {
      void Init
      (
        void
      );

    } /* namespace EventManager */
  } /* namespace Sock */
}/* namespace DS */

#endif /* DS_SOCK_EVENTMANAGER_H */
