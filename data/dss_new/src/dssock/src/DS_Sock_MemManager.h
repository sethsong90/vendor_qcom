#ifndef DS_SOCK_MEMMANAGER_H
#define DS_SOCK_MEMMANAGER_H

/*===========================================================================
  @file SocketMemManager.h

  This file defines functions for translation of Class id to pool id macros.
  It also provides default initializations of PS mem pool objects for all
  the different DSNet objects.

  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_MemManager.h#1 $
  $DateTime: 2009/09/28 14:45:31 $$Author: dmudgal $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-06-30 hm  Created module.

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#ifdef FEATURE_DATA_PS


/*===========================================================================

                     PUBLIC DATA DEFINITIONS

===========================================================================*/
namespace DS
{
  namespace Sock
  {
    namespace MemManager
    {
      void Init
      (
        void
      );
    }
  }
}

#endif /* FEATURE_DATA_PS */
#endif /* DS_SOCK_MEMMANAGER_H */
