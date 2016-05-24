/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        O N C R P C _ D S M . H

GENERAL DESCRIPTION
  This is the public header file which defines the ONCRPC DSM item pool.

 Copyright (c) 2006 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_dsm.h#3 $ 

when        who    what, where, why
--------    ---    ----------------------------------------------------------
08/23/06    ptm    Initial revision.
===========================================================================*/
#ifndef ONCRPC_DSM_H
#define ONCRPC_DSM_H

#include "dsm.h"
#define ONCRPC_DSM_ITEM_POOL ((dsm_mempool_id_type)(&oncrpc_dsm_item_pool))
extern dsm_pool_mgmt_table_type oncrpc_dsm_item_pool;  /* Defined in oncrpc_main_linux */

#endif /* ONCRPC_DSM_H */
