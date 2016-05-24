/*===========================================================================
  FILE: RexUtils.cpp

  OVERVIEW: This file is a helper containing functions to start ps_task,
  dcc_task

  DEPENDENCIES: None

  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datacommon/dssock/main/latest/     
      test/RexUtils.cpp $
  $DateTime: 2009/09/28 14:45:31 $ $Author: dmudgal $

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "RexUtils.h"
using namespace PS::QTF;

/*===========================================================================

                     PUBLIC DECLARATIONS

===========================================================================*/

unsigned char buffer_ps_stack [ PS_STACK_SIZ ];
unsigned char buffer_dcc_stack [ 4096 ]; //DCC_STACK_SIZ < 4096


void RexUtils::SpawnPSTask
(
  char * name
)
{
  TF_MSG( "Spawning PS task for %s", name );

  qtf_tmc_spawn_rex_task(name,
                         &ps_tcb, 
                         buffer_ps_stack,
                         PS_STACK_SIZ,
                         PS_PRI,
                         ps_task,
                         0L);
  TF_MSG( "Starting PS task ");

  qtf_tmc_start_rex_task( &ps_tcb );
}

void RexUtils::SpawnDCCTask
(
  char *name
)
{
  TF_MSG( "Spawning DCC task for %s", name);

  qtf_tmc_spawn_rex_task(name,
                        &dcc_tcb, 
                        buffer_dcc_stack,
                        4096,
                        PS_PRI + 1,
                        dcc_task,
                        0L);

  TF_MSG( "Starting DCC task ");

  qtf_tmc_start_rex_task( &dcc_tcb );
}
