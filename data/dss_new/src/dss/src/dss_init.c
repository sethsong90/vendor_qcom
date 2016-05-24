/*==========================================================================*/
/*!
  @file 
  dss_init.c

  @brief
  This file provides functions that are used to perform initializations 
  of DSS module.

  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*==========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/dss_init.c#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2009-07-14 hm  Created module.

===========================================================================*/

/*---------------------------------------------------------------------------
  Include Files
---------------------------------------------------------------------------*/
#include "comdef.h"
#include "dss_init.h"
#include "ps_sock_mem_pool.h"
#include "DSS_GlobalInit.h"

#ifdef FEATURE_DATA_PS_PING
#include "dss_ping_comm_mgr.h"
#endif

#ifdef FEATURE_DATA_PS_DNS
#include "dssdns.h"
#include "dss_net_mgr.h"
#endif /* FEATURE_DATA_PS_DNS */

#ifdef FEATURE_DSS_LINUX
#include "msg.h"
#include "diag_lsm.h"
#endif

/*---------------------------------------------------------------------------
  Local declarations.
---------------------------------------------------------------------------*/


void dss_powerup
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
#ifdef FEATURE_DSS_LINUX
  /* Initialize DIAG subsystem logging */
  Diag_LSM_Init(NULL);
#endif
  /*------------------------------------------------------------------------
    Initialize DSS memory pools.
  -------------------------------------------------------------------------*/
  ps_sock_mem_pool_init();

  /*------------------------------------------------------------------------
    If rearch is enabled, initialize DSS module.
  -------------------------------------------------------------------------*/
  DSSGlobalInit();

#ifdef FEATURE_DATA_PS_DNS
  /*-------------------------------------------------------------------------
    Initialize the DSS net manager.
  -------------------------------------------------------------------------*/
  dss_net_mgr_init();

  /*-------------------------------------------------------------------------
    Initialize DNS subsystem during powerup
  -------------------------------------------------------------------------*/
  dss_dns_init();
#endif


} /* dss_powerup() */

void dss_powerdown
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*------------------------------------------------------------------------
    If rearch is enabled, cleanup DSS module.
  -------------------------------------------------------------------------*/
  DSSGlobalDeinit();

} /* dss_powerdown() */

void dss_init
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#ifdef FEATURE_DATA_PS_PING
  /*-------------------------------------------------------------------------
    Initialize ping engine.
  -------------------------------------------------------------------------*/
  dss_ping_comm_mgr_init_ping_engine();
#endif /* FEATURE_DATA_PS_PING */

} /* dss_init() */

void dss_deinit
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

} /* dss_deinit() */




