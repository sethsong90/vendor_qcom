#ifndef PS_MIP_TASK_H
#define PS_MIP_TASK_H
/*===========================================================================
                          P S _ M I P _ T A S K . H

DESCRIPTION
  Header file containing the MIP task specific information.  This is intended
  to isolate all of the task specific information to a single module.

Copyright (c) 2002-2009 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                            EDIT HISTORY FOR FILE

$PVCSPath: L:/src/asw/MM_DATA/vcs/ps_mip_task.h_v   1.1   16 Sep 2002 14:52:18   jayanthm  $
$Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_mip_task.h#1 $ $DateTime: 2011/01/10 09:44:56 $ $Author: maldinge $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
03/26/09    pp     CMI De-featurization.
12/19/08    pp     Common Modem Interface: Public/Private split.
01/09/07    as     Support for PS task self start dmu pre-encryption
12/04/06  as/msr   MIP dereg support
09/19/06    as     Added mip_task_rand_data_ready() API for secutil to
                   indicate when random data is available.
08/13/04    kvd    Added new cmd MIP_PHYS_IFACE_UP_CMD.
02/02/04    jd     Replace definitions for MIP signal handling with command.
                   Added MIP_RAND_DATA_READY() macro for secutil to call when
                   DMU can poll for random data without blocking.
01/27/02    jd     MIP uses commands now instead of signals
09/10/02    jd/jay created file
===========================================================================*/

#include "ps_svc.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                                   MACROS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
/*---------------------------------------------------------------------------
  Define the task functions in terms of the equivalent functions for the
  hosting task: in this case PS
---------------------------------------------------------------------------*/
#define MIP_SEND_CMD( cmd, data_ptr )           \
  ps_send_cmd((cmd), (void *) (data_ptr))

/*---------------------------------------------------------------------------
  define all of the MIP related commands that can be sent in terms of the
  commands for the hosting task: in this case PS
---------------------------------------------------------------------------*/
#define MIP_META_SM_MIN_CMD         PS_MIP_META_SM_MIN_CMD
#define MIP_PHYS_IFACE_UP_CMD       PS_MIP_PHYS_IFACE_UP_CMD
#define MIP_CFG_IFACE_CMD           PS_MIP_CFG_IFACE_CMD
#define MIP_EXIT_CMD                PS_MIP_EXIT_CMD
#define MIP_REG_FAILURE_CMD         PS_MIP_REG_FAILURE_CMD
#define MIP_BRING_DOWN_CMD          PS_MIP_BRING_DOWN_CMD
#define MIP_META_SM_MAX_CMD         PS_MIP_META_SM_MAX_CMD

#define MIP_SOCKET_EVENT_CMD        PS_MIP_SOCKET_EVENT_CMD
#define DMU_PREENCRYPT_CMD          PS_DMU_PREENCRYPT_CMD

/*---------------------------------------------------------------------------
  Macro for SEC to call to indicate deadlock will not occur if DMU calls
  secutil_get_random() for random data.
---------------------------------------------------------------------------*/
#define MIP_RAND_DATA_READY()  MIP_SEND_CMD( DMU_PREENCRYPT_CMD, NULL )
#define MIP_GEN_DMU_KEYS()     MIP_SEND_CMD( DMU_PREENCRYPT_CMD, NULL )

/*===========================================================================
FUNCTION    MIP_TASK_RAND_DATA_READY

DESCRIPTION
  This function is called by secutil when random data is available via
  secutil_get_rand().

PARAMETERS
  none

DEPENDENCIES
  none

RETURN VALUE
  none

SIDE EFFECTS
  None
===========================================================================*/
void mip_task_rand_data_ready( void );

#endif /* PS_MIP_TASK_H */
