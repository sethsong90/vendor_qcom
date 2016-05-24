/*==========================================================================*/
/*!
  @file 
  ps_netiface_init.c

  @brief
  This file provides functions that are used to perform initializations 
  of netiface module.

  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*==========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/netiface/rel/11.03/src/ps_netiface_init.c#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2009-07-14 hm  Created module.

===========================================================================*/

/*---------------------------------------------------------------------------
  Include Files
---------------------------------------------------------------------------*/
#include "comdef.h"
#include "ps_netiface_init.h"
#include "ps_iface.h"
#include "ps_iface_flow.h"
#include "ps_ifacei.h"
#include "ps_phys_linki_event.h"
#include "ps_flowi_event.h"
#include "ps_policyi_mgr.h"


/*---------------------------------------------------------------------------
  Local declarations.
---------------------------------------------------------------------------*/

void ps_netiface_powerup
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*------------------------------------------------------------------------
    Initialize the global PS critical section
  -------------------------------------------------------------------------*/
  ps_iface_init();
#ifdef FEATURE_DATA_PS_QOS
  ps_iface_flow_init();
#endif /* FEATURE_DATA_PS_QOS */

  ps_iface_ipfltr_init();

  /*-------------------------------------------------------------------------
    Initialize Phys Link, PS flow, and PS iface events module.
    Must be done in this order.
  -------------------------------------------------------------------------*/
  ps_phys_link_event_init();
  ps_flow_event_init();
  ps_iface_event_init();

  /*-------------------------------------------------------------------------
    Initialize the policy manager.
  -------------------------------------------------------------------------*/
  ps_policy_mgr_init();

} /* ps_netiface_powerup() */

void ps_netiface_powerdown
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  // TODO what cleanup should take place here?

} /* ps_netiface_powerdown() */

void ps_netiface_init
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Currently a no-op
  -------------------------------------------------------------------------*/

} /* ps_netiface_init() */

void ps_netiface_deinit
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Currently a no-op
  -------------------------------------------------------------------------*/

} /* ps_netiface_deinit() */



