/*==========================================================================*/
/*!
  @file
  ps_utils_init.c

  @brief
  This file provides functions that are used to perform initializations
  of utils module.

  Copyright (c) 2009-2011 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*==========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ps_utils_init.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  03/01/11   jee Fix to handle MMGSDI reg issue during startup
  11/08/10   mct Added AKAv2 DSS support
  10/26/10   jee Added AKAv2 feature
  2009-07-14 hm  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "ps_utils_init.h"
#include "ps_mem.h"
#include "ps_system_heap.h"
#include "ps_crit_sect.h"
#include "pstimer_config.h"
#include "ds_Utils_Signal.h"
#include "ds_Utils_MemManager.h"
#include "ds_Utils_SignalBus.h"
#include "ds_Utils_Atomic.h"
#include "ps_utils_aka.h"
#include "ds_auth_platform.h"

/*---------------------------------------------------------------------------
  Local declarations.
---------------------------------------------------------------------------*/

/*!
 * Defines the global PS critical section used by all modules.
 */
ps_crit_sect_type global_ps_crit_section;
void ps_utils_powerup
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*------------------------------------------------------------------------
    Initialize the PS system heap module.
  -------------------------------------------------------------------------*/
  ps_system_heap_init();

  /*------------------------------------------------------------------------
    Initialize the global PS critical section
  -------------------------------------------------------------------------*/
  PS_INIT_CRIT_SECTION(&global_ps_crit_section);

  /*------------------------------------------------------------------------
    Initialize the PS timer module.
  -------------------------------------------------------------------------*/
  ps_timer_init();

  /*------------------------------------------------------------------------
    Initialize the PS Mem module.
  -------------------------------------------------------------------------*/
  ps_mem_init();

  /*------------------------------------------------------------------------
    Initialize the utils memory pools.
  -------------------------------------------------------------------------*/
  ds::Utils::MemoryManager::MemPoolInit();

  /*------------------------------------------------------------------------
    Initialize the atomic operations module
  -------------------------------------------------------------------------*/
  ds_utils_atomic_Init();

} /* ps_utils_powerup() */

void ps_utils_powerdown
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*------------------------------------------------------------------------
    DeInitialize the PS timer module.
  -------------------------------------------------------------------------*/
  ps_timer_deinit();

  /*------------------------------------------------------------------------
    DeInitialize the PS Mem module.
  -------------------------------------------------------------------------*/
  ps_mem_deinit();

  /*------------------------------------------------------------------------
    DeInitialize the PS system heap module.
  -------------------------------------------------------------------------*/
  ps_system_heap_deinit();

  
  /*------------------------------------------------------------------------
    DeInitialize ds_Utils_Atomic module.
  -------------------------------------------------------------------------*/
  ds_utils_atomic_DeInit();

  /*------------------------------------------------------------------------
    Destroy global PS critical section
  -------------------------------------------------------------------------*/
  PS_DESTROY_CRIT_SECTION(&global_ps_crit_section);


} /* ps_utils_powerdown() */

void ps_utils_init
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*------------------------------------------------------------------------
    Register a Signal dispatcher with DS_SIG task (when CS is not available)
  -------------------------------------------------------------------------*/
#if (!(defined(FEATURE_DATACOMMON_PACKAGE_BMP) || defined(FEATURE_DATACOMMON_PACKAGE_SINGLE_PROC))) \
    || defined(TEST_FRAMEWORK) 
  (void) ds_sig_set_cmd_handler (DS_SIG_SIGNAL_DISPATCH_CMD,
                                 ds::Utils::Signal::SignalDispatcher);
  ds::Utils::SignalBus::RegisterCmdHandler();
#endif

  /*------------------------------------------------------------------------
    Register with MMGSDI
  -------------------------------------------------------------------------*/
#ifdef FEATURE_MMGSDI_SESSION_LIB
  ps_utils_aka_reg_mmgsdi_client_id();
#endif /* FEATURE_MMGSDI_SESSION_LIB */

#ifndef FEATURE_DSS_LINUX
  ps_utils_aka_create_efs_item_file_path();

  ds_auth_init();
#endif
} /* ps_utils_init() */

void ps_utils_deinit
(
  void
)
{
}


