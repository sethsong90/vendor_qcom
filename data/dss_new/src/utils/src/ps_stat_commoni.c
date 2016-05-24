/*===========================================================================

                    P S _ S T A T _ C O M M O N I . C

GENERAL DESCRIPTION
  This program provides the statistic access functions that can be used by
  all layers

EXTERNALIZED FUNCTIONS
  ps_stat_init
    This function initializes the statistics module appropriately

INITIALIZATION AND SEQUENCING REQUIREMENTS
  NONE

Copyright (c) 2007-2011 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ps_stat_commoni.c#1 $ $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $


when       who     what, where, why
--------   ---     ----------------------------------------------------------
12/08/10   rp      RAM optimization changes.
05/07/09   sp      Added support for updating the HC packed files.
08/19/08   pp      Included ps_mem.h to avoid compile warnings.
04/04/07   scb     Added featurization for IPHC, ROHC and RSVP
02/28/07   scb     Initial version

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "customer.h"
#include "comdef.h"
#ifndef FEATURE_DATACOMMON_THIRD_PARTY_APPS
#ifdef FEATURE_DATA_PS
#include "IxErrno.h"
#include "ps_stat_commoni.h"
#include "ps_mem.h"

#ifdef FEATURE_DATA_PS_LLC
#include "ps_stat_arp.h"
#include "ps_stat_llc.h"
#endif/* FEATURE_DATA_PS_LLC */

#ifdef FEATURE_DATA_PS_IPV6
#include "ps_stat_icmpv6.h"
#include "ps_stat_ipv6.h"
#endif/* FEATURE_DATA_PS_IPV6 */

#include "ps_stat_flow.h"
#include "ps_stat_icmpv4.h"
#include "ps_stat_iface.h"
#include "ps_stat_ipv4.h"
#include "ps_stat_mem.h"
#include "ps_stat_phys_link.h"

#ifdef FEATURE_DATA_PS_PPP
#include "ps_stat_ppp.h"
#endif

#ifdef FEATURE_DATA_PS_RSVP
#include "ps_stat_rsvp.h"
#endif /* FEATURE_DATA_PS_RSVP */
#include "ps_stat_tcp.h"
#include "ps_stat_udp.h"

#include "err.h"
#include <string.h>

/*===========================================================================
FUNCTION PS_STAT_INIT()

DESCRIPTION

DEPENDENCIES
  None

PARAMETERS

RETURN VALUE

SIDE EFFECTS
  None
===========================================================================*/
void ps_stat_init
(
  void
)
{
  
  /*-------------------------------------------------------------------------
    For each of the statistics modules, initialize the offset structures
  -------------------------------------------------------------------------*/
#ifdef FEATURE_DATA_PS_LLC
  ps_stat_init_arp();
  ps_stat_init_llc();
#endif/* FEATURE_DATA_PS_LLC */

#ifdef FEATURE_DATA_PS_IPV6
  ps_stat_init_ipv6();
  ps_stat_init_icmpv6();
#endif/* FEATURE_DATA_PS_IPV6 */

  ps_stat_init_flow();
  ps_stat_init_icmpv4();
  ps_stat_init_iface();
  ps_stat_init_ipv4();
  ps_stat_init_mem();
  ps_stat_init_phys_link();

#ifdef FEATURE_DATA_PS_PPP  
  ps_stat_init_ppp();
#endif

#ifdef FEATURE_DATA_PS_RSVP
  ps_stat_init_rsvp();
#endif /* FEATURE_DATA_PS_RSVP */
  ps_stat_init_tcp();
  ps_stat_init_udp();
}

#endif /* FEATURE_DATA_PS */
#endif /* ifndef FEATURE_DATACOMMON_THIRD_PARTY_APPS */
