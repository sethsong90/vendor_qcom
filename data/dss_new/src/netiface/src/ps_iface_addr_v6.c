/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                      P S _ I F A C E _ A D D R _ V 6 . C

GENERAL DESCRIPTION
 Internet Protocol Version 6 - Neighbor Discovery (IPv6 ND)

EXTERNALIZED FUNCTIONS

INITIALIZATION AND SEQUENCING REQUIREMENTS

Copyright (c) 2007 - 2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/


/*===========================================================================

                           EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datamodem/interface/netiface/rel/11.03/src/ps_iface_addr_v6.c#1 $
  $Author: zhasan $ $DateTime: 2011/06/17 12:02:33 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
05/24/11    ssh    Check iface validity before operation in unused_iid hdlr
03/21/11    op     Data SU API cleanup
11/05/10    ssh    Cancel unused iid timer if address is not found
08/27/10    ash    Added more F3 messages.
05/27/10    ssh    memset ipv6 addr memory after allocation
03/09/10    ash    Drill Down to base iface of logical iface chain for ipv6
                   addrresses.
10/12/09    ssh    Reporting prefix updates to diag
10/12/09    ssh    apply_prefix: cache iid directly instead of calling macro
10/01/09    ssh    Skip external addresses when updating priv addr lifetimes.
                   Use internal set_addr api to update addr_state.
09/23/09    ss     KW warnings fixed.
09/09/09    ss     Critical section released in error cases.
07/21/09    ssh    Don't apply prefix if iid is 0
07/21/09    vp     Updating the gateway_iid when prefix did not change
06/24/09    am     Renamed hton/ntoh macros with ps_ prefix.
06/22/09    ssh    Examine ROUTEABLE ifaces too in unused_iid_handler
01/09/09    ar     Added dad_retries initialization.
08/18/08    vp     Featurized addr_mgmt support
07/01/08    mct    Updated for MIPv6 laptop support.
03/14/08    ssh    Disallow IPv6 privacy extensions for MIPv6 iface
01/18/07    mct    Fix to compare privacy address lifetimes in ms.
10/01/07    mct    Added support for IPv6 RmNet.
01/23/07    mct    Initial Version
===========================================================================*/


/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"       /* Customer Specific Features */
#ifndef FEATURE_DATACOMMON_THIRD_PARTY_APPS
#if defined (FEATURE_DATA_PS) && defined (FEATURE_DATA_PS_IPV6)
#include "dsm.h"
#include "ps_iface.h"
#include "ps_ifacei.h"
#include "ps_ip6.h"
#include "ps_icmp6_msg.h"
#include "msg.h"
#include "err.h"
#include "amssassert.h"
#include "ps_in.h"
#include "ps_ip6_addr.h"
#include "ps_icmp6.h"
#include "ps_stat_icmpv6.h"
#include "ps_iface_addr_v6.h"
#include "ps_ifacei_addr_v6.h"
#include "ps_ifacei_event.h"
#include "ps_ifacei_utils.h"
#include "ps_ip6.h"
#include "ps_ip6_sm.h"
#include "ps_ip6_events.h"
#include "ps_route.h"
#include "ps_routei.h"
#include "ps_routei_lo.h"
#include "ps_mem.h"
#include "ps_utils.h"

#ifdef FEATURE_DATA_PS_ADDR_MGMT
#include "ps_iface_addr_mgmt.h"
#endif /* FEATURE_DATA_PS_ADDR_MGMT */

#include "ps_handle_mgr.h"
#include "ps_icmp6_nd.h"
#include "ds_Utils_DebugMsg.h"

/*===========================================================================

                           LOCAL DATA DECLARATIONS

===========================================================================*/
/*---------------------------------------------------------------------------
  Tuning the number of ps iface internal v6 address buffers needed by this
  module
---------------------------------------------------------------------------*/
#define PS_IFACEI_V6_ADDR_BUF_SIZE  ((sizeof(ps_ifacei_v6_addr_type) +3) & ~3)

#define PS_IFACEI_V6_ADDR_BUF_NUM  (MAX_IPV6_ADDRS * (MAX_IPV6_LO_IFACES - 1))
#define PS_IFACEI_V6_ADDR_BUF_HIGH_WM   (PS_IFACEI_V6_ADDR_BUF_NUM - 5)
#define PS_IFACEI_V6_ADDR_BUF_LOW_WM    1

/*----------------------------------------------------------------------------
  Allocate memory to hold ps_ifacei_v6_addr along with ps_mem header
----------------------------------------------------------------------------*/
static int ps_ifacei_v6_addr_buf_mem[PS_MEM_GET_TOT_SIZE_OPT
                                     (
                                       PS_IFACEI_V6_ADDR_BUF_NUM,
                                       PS_IFACEI_V6_ADDR_BUF_SIZE
                                     )];

#ifdef FEATURE_DATA_PS_MEM_DEBUG
/*----------------------------------------------------------------------------
  Array of pointers used to facilitate easy debugging.  The first one points
  to the ps_mem header and the latter ponts to ps_ifacei_v6_addr_buf
----------------------------------------------------------------------------*/
static ps_mem_buf_hdr_type     * ps_ifacei_v6_addr_buf_hdr[PS_IFACEI_V6_ADDR_BUF_NUM];
static ps_ifacei_v6_addr_type  * ps_ifacei_v6_addr_buf_ptr[PS_IFACEI_V6_ADDR_BUF_NUM];
#endif /* FEATURE_DATA_PS_MEM_DEBUG */


#ifdef FEATURE_DSS_LINUX
/* Adding this here from ps_ip6_sm, which is not used on Linux. */
boolean ipv6_priv_ext_enabled = TRUE;
//#define IP6_REPORT_PFX_UPDATE(a,b)

#include <stdlib.h>
#define ps_utils_generate_rand_64bit_num(a) (*a = (uint64)((uint64)rand()<<32 | (uint64)rand()))
#endif /* FEATURE_DSS_LINUX */


/*===========================================================================

                     INTERNAL FUNCTIONS - TIMER HANDLERS

===========================================================================*/
/*===========================================================================
FUNCTION PS_IFACEI_V6_PREFIX_DEPRECATED_HANDLER()

DESCRIPTION
  This function is a callback for the prefix pref_lifetime timer -> it is
  called when the IPv6 pref_lifetimer timer expires.  It indicates that the
  prefix has gone from Preferred to Deprecated.

  NOTE: this function will need rework when multiple prefixes are supported.
  NOTE: it assumes that both IT and the apply_v6_prefix() function execute in
    PS context, and so there is no issue with changing the state of the
    prefix to deprecated

PARAMETERS
  user_data_ptr: when the timer is created it has the v6 address with which this
  callback is assiciated with stored.

RETURN VALUE
  None

DEPENDENCIES
  Must be modified to support multiple prefixes.

SIDE EFFECTS
  Changes the state of the prefix
===========================================================================*/
#ifdef FEATURE_DSS_LINUX
void ps_ifacei_v6_prefix_deprecated_handler
#else
static void ps_ifacei_v6_prefix_deprecated_handler
#endif
(
  void *user_data_ptr
)
{
  ps_ifacei_v6_addr_type *v6_addr_ptr =
    (ps_ifacei_v6_addr_type *)user_data_ptr;
  ps_iface_event_info_u_type event_info;
  ps_iface_type          *iface_ptr = NULL;
  ps_ifacei_v6_addr_type *tmp_v6_addr_ptr;
  struct ps_in6_addr  ip_addr;
  uint8            index = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if(!v6_addr_ptr)
  {
    LOG_MSG_ERROR("Invalid v6 addr ptr in ps_ifacei_v6_prefix_deprecated_handler!",
              0,0,0);
    ASSERT(0);
  }

  memset(&event_info, 0, sizeof(event_info));

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    Retrieve the interface which owns this IPv6 address.
  -------------------------------------------------------------------------*/
  ip_addr.ps_s6_addr64[0] = v6_addr_ptr->prefix;
  ip_addr.ps_s6_addr64[1] = v6_addr_ptr->iid;

/*---------------------------------------------------------------------------
  Because ps_iface_find_ipv6 drills down to the base iface of logical iface
  chain, the call to PS_IFACE_GET_BASE_IFACE is not neccessarry here.
---------------------------------------------------------------------------*/

  (void)ps_iface_find_ipv6_addr(&ip_addr, NULL, &iface_ptr);

  if(!PS_IFACE_IS_VALID(iface_ptr))
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Invalid iface in ps_ifacei_v6_prefix_deprecated_handler!",
              0,0,0);
    return;
  }

  ASSERT(iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX] != NULL);

  IPV6_ADDR_MSG(ip_addr.ps_s6_addr64);
  LOG_MSG_INFO1("Prefix is now in DEPRECATED state: %x",
            v6_addr_ptr->addr_state, 0, 0);

  /*-------------------------------------------------------------------------
    Cleanup all unused Privacy addresses (ref_cnt of 0). Prefix is deprecated,
    so unused addresses can no longer be returned. Also remove from the index
    at which privacy extensions are started (MAX_IPV6_PREFIXES index)
    since the array is condensed as part of address deletion.
  -------------------------------------------------------------------------*/
  for(index = MAX_IPV6_PREFIXES; index < MAX_IPV6_ADDRS; index++)
  {
    tmp_v6_addr_ptr = iface_ptr->iface_private.ipv6_addrs[MAX_IPV6_PREFIXES];

    if(tmp_v6_addr_ptr == NULL)
    {
      break;
    }

    if( (v6_addr_ptr->prefix == tmp_v6_addr_ptr->prefix) &&
        (tmp_v6_addr_ptr->ref_cnt == 0))
    {
      (void) ps_iface_delete_priv_ipv6_addr(iface_ptr, tmp_v6_addr_ptr);
    }
  }

  v6_addr_ptr->addr_state = IPV6_ADDR_STATE_DEPRECATED;
  event_info.prefix_info.prefix.ps_s6_addr64[0] = v6_addr_ptr->prefix;
  event_info.prefix_info.kind                = PREFIX_DEPRECATED;
  event_info.prefix_info.prefix_len          = v6_addr_ptr->prefix_len;

  ps_ifacei_invoke_event_cbacks(iface_ptr,
                                NULL,
                                IFACE_PREFIX_UPDATE_EV,
                                event_info);
  IP6_REPORT_PFX_UPDATE( iface_ptr, event_info.prefix_info );

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

} /* ps_ifacei_v6_prefix_deprecated_handler() */



/*===========================================================================
FUNCTION PS_IFACEI_V6_PREFIX_EXPIRED_HANDLER()

DESCRIPTION
  This function is a callback for the prefix valid lifetime timer -> it
  is called when the IPv6 valid_lifetimer timer expires.  It will remove a
  prefix as it's lifetime has expired.

  NOTE: this function will need rework when multiple prefixes are supported.

PARAMETERS
  user_data_ptr: when the timer is created it has the v6 address with which this
  callback is assiciated with stored.

RETURN VALUE
  None

DEPENDENCIES
  Must be modified to support multiple prefixes.

SIDE EFFECTS
  None
===========================================================================*/
#ifdef FEATURE_DSS_LINUX
void ps_ifacei_v6_prefix_expired_handler
#else
static void ps_ifacei_v6_prefix_expired_handler
#endif
(
  void *user_data_ptr
)
{
  ps_ifacei_v6_addr_type *v6_addr_ptr =
    (ps_ifacei_v6_addr_type *)user_data_ptr;
  ps_iface_type   *iface_ptr = NULL;
  struct ps_in6_addr  ip_addr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if(!v6_addr_ptr)
  {
    LOG_MSG_ERROR("Invalid v6 addr ptr in ps_ifacei_v6_prefix_expired_handler!",
              0,0,0);
    ASSERT(0);
  }

/*-------------------------------------------------------------------------
    Retrieve the interface which owns this IPv6 address.
  -------------------------------------------------------------------------*/
  ip_addr.ps_s6_addr64[0] = v6_addr_ptr->prefix;
  ip_addr.ps_s6_addr64[1] = v6_addr_ptr->iid;

/*---------------------------------------------------------------------------
  Because ps_iface_find_ipv6 drills down to the base iface of logical iface
  chain, the call to PS_IFACE_GET_BASE_IFACE is not neccessarry here.
---------------------------------------------------------------------------*/

  (void)ps_iface_find_ipv6_addr(&ip_addr, NULL, &iface_ptr);

  if (!PS_IFACE_IS_VALID(iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface_ptr in ps_ifacei_v6_prefix_expired_handler",
              0,0,0);
    return;
  }

  IPV6_ADDR_MSG(ip_addr.ps_s6_addr64);
  LOG_MSG_INFO1("Prefix is now in EXPIRED state: %x",
            0, v6_addr_ptr->addr_state, 0);

  (void)ps_iface_remove_v6_prefix(iface_ptr, v6_addr_ptr->prefix);

} /* ps_ifacei_v6_prefix_expired_handler() */

/*===========================================================================
FUNCTION PS_IFACEI_IPV6_IID_TIMER_HANDLER()

DESCRIPTION
  This function is a callback for the privacy extension lifetime timers. It
  is called when an IPv6 private IID timer expires.  It will take the
  appropriate action depedning upon if the timer that expired was the
  preferred or valid lifetime.

PARAMETERS
  user_data_ptr: the callback information which will be the IPv6 address
                 information structure

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_ifacei_ipv6_iid_timer_handler
(
  void *user_data_ptr
)
{
  (void)user_data_ptr;

#ifndef FEATURE_DSS_LINUX
  ps_ifacei_v6_addr_type *v6_addr_ptr =
    (ps_ifacei_v6_addr_type *)user_data_ptr;
  ps_iface_type   *iface_ptr = NULL;
  struct ps_in6_addr  ip_addr;
  ps_iface_event_info_u_type event_info;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if(!v6_addr_ptr)
  {
    LOG_MSG_ERROR("Invalid v6 addr ptr in ps_ifacei_ipv6_iid_timer_handler!",
              0,0,0);
    ASSERT(0);
  }

  memset(&event_info, 0, sizeof(event_info));

  /*-------------------------------------------------------------------------
    Retrieve the interface which owns this IPv6 address.
  -------------------------------------------------------------------------*/
  ip_addr.ps_s6_addr64[0] = v6_addr_ptr->prefix;
  ip_addr.ps_s6_addr64[1] = v6_addr_ptr->iid;

/*---------------------------------------------------------------------------
  Because ps_iface_find_ipv6 drills down to the base iface of logical iface
  chain, the call to PS_IFACE_GET_BASE_IFACE is not neccessarry here.
---------------------------------------------------------------------------*/

  (void)ps_iface_find_ipv6_addr(&ip_addr, NULL, &iface_ptr);

  if (!PS_IFACE_IS_VALID(iface_ptr))
  {
    LOG_MSG_INFO1("Invalid iface_ptr in ipv6_private_iid_timer_handler",
             0,0,0);
    ASSERT(0);
    return;
  }

  if(v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_PRIV_UNIQUE)
  {
    event_info.priv_ipv6_addr.is_unique = TRUE;
  }
  else if(v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_PRIV_SHARED)
  {
    event_info.priv_ipv6_addr.is_unique = FALSE;
  }
  else
  {
    LOG_MSG_INFO1("Invalid address type %d in ipv6_private_iid_timer_handler",
             v6_addr_ptr->addr_type,0,0);
    ASSERT(0);
    return;
  }

  /*-------------------------------------------------------------------------
    If the address is not in deprecated state then the preferred lifetime
    expired.
  -------------------------------------------------------------------------*/
  if(v6_addr_ptr->addr_state == IPV6_ADDR_STATE_VALID)
  {
    v6_addr_ptr->addr_state = IPV6_ADDR_STATE_DEPRECATED;
    IPV6_ADDR_MSG(ip_addr.ps_s6_addr64);
    LOG_MSG_INFO1("type: %x is now in DEPRECATED state: %x",
             v6_addr_ptr->addr_type, v6_addr_ptr->addr_state, 0);

    event_info.priv_ipv6_addr.ip_addr.type = IPV6_ADDR;
    event_info.priv_ipv6_addr.ip_addr.addr.v6.ps_s6_addr64[0] =
                                                         v6_addr_ptr->prefix;
    event_info.priv_ipv6_addr.ip_addr.addr.v6.ps_s6_addr64[1] =
                                                            v6_addr_ptr->iid;
    ps_ifacei_invoke_event_cbacks(iface_ptr,
                                  NULL,
                                  IFACE_IPV6_PRIV_ADDR_DEPRECATED_EV,
                                  event_info);

    /*-----------------------------------------------------------------------
      Only delete private shared addresses if both ref_cnt is 0 and state is
      deprecated, as multiple apps could have requested the addr, but not
      binded yet.
    -----------------------------------------------------------------------*/
    if(v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_PRIV_SHARED &&
       v6_addr_ptr->ref_cnt == 0)
    {
      if(-1 == ps_iface_delete_priv_ipv6_addr(iface_ptr, v6_addr_ptr))
      {
        LOG_MSG_ERROR("Error deleting private shared address on iface %p!",
                  iface_ptr, 0, 0);
        return;
      }
    }
  }
  else if(v6_addr_ptr->addr_state == IPV6_ADDR_STATE_DEPRECATED)
  {
    /*-----------------------------------------------------------------------
      This was a valid lifetime expiration. Delete the address.
    -----------------------------------------------------------------------*/
    event_info.priv_ipv6_addr.ip_addr.type = IPV6_ADDR;
    event_info.priv_ipv6_addr.ip_addr.addr.v6.ps_s6_addr64[0] =
                                                         v6_addr_ptr->prefix;
    event_info.priv_ipv6_addr.ip_addr.addr.v6.ps_s6_addr64[1] =
                                                            v6_addr_ptr->iid;

    ps_ifacei_invoke_event_cbacks(iface_ptr,
                                  NULL,
                                  IFACE_IPV6_PRIV_ADDR_EXPIRED_EV,
                                  event_info);
  }
  else
  {
    LOG_MSG_ERROR("Invalid IPv6 address state %d", v6_addr_ptr->addr_state, 0, 0);
    return;
  }
#endif /* FEATURE_DSS_LINUX */
}

/*===========================================================================
FUNCTION PS_IFACEI_IPV6_UNUSED_IID_HANDLER()

DESCRIPTION
  This function serves as a callback for three purposes:
  1)The retry timer for DAD verification expires. In this case (only for
    internal addresses) the timer is restarted (if tries remain) and the
    dad function called.
  2)The mobile needs to check that an external address is still being used.
  3)The privacy extension unsed timer to indicate when an IPv6 private address
    has not been bound by a socket in the required time. The function will
    notify the application(s) and free the address.

PARAMETERS
  user_data_ptr: the callback information which will be the IPv6 address
                 information structure

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_ifacei_ipv6_unused_iid_handler
(
  void *user_data_ptr
)
{
  ps_ifacei_v6_addr_type *v6_addr_ptr =
    (ps_ifacei_v6_addr_type *)user_data_ptr;
  ps_iface_type   *iface_ptr = NULL;
  struct ps_in6_addr  ip_addr;
#ifdef FEATURE_DATA_PS_ADDR_MGMT
  ps_iface_type   *rm_iface_ptr;
  int16            ps_errno;
  ps_iface_addr_mgmt_handle_type    handle;
  ps_iface_addr_mgmt_addr_info_type addr_info;
  uint8   iface_index;
  uint8   addr_index;
  boolean addr_found = FALSE;
#endif /* FEATURE_DATA_PS_ADDR_MGMT */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1("ps_ifacei_ipv6_unused_iid_handler()",0,0,0);
  if(!v6_addr_ptr)
  {
    LOG_MSG_ERROR("Invalid v6 addr ptr in ps_ifacei_ipv6_unused_iid_handler!",
              0,0,0);
    ASSERT(0);
  }

  /*-------------------------------------------------------------------------
    Retrieve the interface which owns this IPv6 address.
  -------------------------------------------------------------------------*/
  ip_addr.ps_s6_addr64[0] = v6_addr_ptr->prefix;
  ip_addr.ps_s6_addr64[1] = v6_addr_ptr->iid;

  IPV6_ADDR_MSG(ip_addr.ps_s6_addr64);
#ifdef FEATURE_DATA_PS_ADDR_MGMT
  /*-------------------------------------------------------------------------
    Search the global iface array for that with the specified address.
    During DAD processing, iface will be in CONFIGURE state.
    Otherwise, it will be in UP, GOING_DOWN or ROUTEABLE state.
  -------------------------------------------------------------------------*/
  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);
  for(iface_index = 0; !addr_found && (iface_index < MAX_SYSTEM_IFACES); iface_index++)
  {
    iface_ptr = global_iface_ptr_array[iface_index];
    if(PS_IFACE_IS_VALID(iface_ptr))
    {
      /*---------------------------------------------------------------------
        If iface has inherit IP info true then get the base iface of the
        logical chain, else do nothing.
      ---------------------------------------------------------------------*/
      iface_ptr = PS_IFACE_GET_BASE_IFACE(iface_ptr);
      
      /*---------------------------------------------------------------------
        Skip iface unless it is configuring, going down, routeable or up
      ---------------------------------------------------------------------*/
      if( !(PS_IFACEI_GET_STATE(iface_ptr) & 
            (IFACE_CONFIGURING | IFACE_GOING_DOWN | IFACE_ROUTEABLE | IFACE_UP)))
      {
        continue;
      }

      /*---------------------------------------------------------------------
        Find the IPv6 address. If it matches the full or link local
        address use the address structure.
      ---------------------------------------------------------------------*/
      for(addr_index = 0; addr_index < MAX_IPV6_ADDRS; addr_index++)
      {
        if( NULL == iface_ptr->iface_private.ipv6_addrs[addr_index] )
        {
          continue;
        }

        if( ( v6_addr_ptr == iface_ptr->iface_private.ipv6_addrs[addr_index] ) &&
            ( ((ip_addr.ps_s6_addr64[0] ==
              iface_ptr->iface_private.ipv6_addrs[addr_index]->prefix) &&
             (ip_addr.ps_s6_addr64[1] ==
              iface_ptr->iface_private.ipv6_addrs[addr_index]->iid)) ||
            (PS_IN6_IS_ADDR_LINKLOCAL(&ip_addr) &&
             (ip_addr.ps_s6_addr64[1] ==
              iface_ptr->iface_private.ipv6_addrs[addr_index]->iid)) ) )
        {
          /*-----------------------------------------------------------------
            Address match found.
          -----------------------------------------------------------------*/
          addr_found = TRUE;
          break;
        }
      }/* end for addr_index*/
    } /* end for iface valid */
  }/* end for iface_index*/
  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

  if( !addr_found )
  {
    /*-----------------------------------------------------------------------
      Cancel the unused IID timer and return
    -----------------------------------------------------------------------*/
    LOG_MSG_ERROR("Cannot find iface from address in ipv6_unused_iid_handler", 0, 0, 0);
    PS_TIMER_FREE_HANDLE(v6_addr_ptr->unused_addr_timer_handle);
    return;
  }
#else

  /*-------------------------------------------------------------------------
    Because ps_iface_find_ipv6 drills down to the base iface of logical
    iface chain, the call to PS_IFACE_GET_BASE_IFACE is not neccessarry here.
  -------------------------------------------------------------------------*/

  (void)ps_iface_find_ipv6_addr(&ip_addr, NULL, &iface_ptr);

#endif

  /*-------------------------------------------------------------------------
    If the iface_ptr is invalid it implies the address doesn't exist or
    that the interface has already gone down, but for some reason the timers
    are still running.
  -------------------------------------------------------------------------*/
  if (!PS_IFACE_IS_VALID(iface_ptr))
  {
    LOG_MSG_INFO1("Invalid iface_ptr in ipv6_unused_iid_handler", 0, 0, 0);
    ASSERT(0);
    return;
  }

#ifdef FEATURE_DATA_PS_ADDR_MGMT
  handle = ps_iface_addr_mgmt_get_handle_from_ip(iface_ptr, &ip_addr);

  /*-------------------------------------------------------------------------
    If the address is external it indicates we are trying to verify its
    presence
  -------------------------------------------------------------------------*/
  if(v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_EXTERNAL)
  {
    rm_iface_ptr = v6_addr_ptr->rm_iface_ptr;

    if (rm_iface_ptr == NULL)
    {
      if (!PS_IFACE_GET_IS_ACTIVE(iface_ptr))
      {
        LOG_MSG_INFO1("NULL rm_iface_ptr of iface_ptr in ipv6_unused_iid_handler",
                 0, 0, 0);
        ASSERT(0);
      }
      else
      {
        LOG_MSG_INFO1("ps_ifacei_ipv6_unused_iid_handler(): "
                      "RM iface is NULL for external address.", 0, 0, 0);
        /*-------------------------------------------------------------------------
          If rm_iface_ptr is NULL for Always on IFACE then free up the iid,
          because we can't do NS without proper rm_iface_ptr.
        -------------------------------------------------------------------------*/
        (void) ps_iface_addr_mgmt_free_addr(iface_ptr, &handle,
                                            NULL, &ps_errno);
      }
      return;
    }

    if( -1 == ps_iface_addr_mgmt_get_addr_info(&handle, &addr_info, &ps_errno))
    {
      LOG_MSG_ERROR("ps_ifacei_ipv6_unused_iid_handler(): "
                    "ps_iface_addr_mgmt_get_addr_info failed", 0, 0, 0);
      return;
    }

    /*-----------------------------------------------------------------------
      If the address is VALID then send the NS to validate its existence and
      DEPRECATE the address. If the address is DEPRECATED there was no
      response and the address should be freed.
    -----------------------------------------------------------------------*/
    if(addr_info.addr_state == IPV6_ADDR_STATE_VALID)
    {
      LOG_MSG_INFO1("ps_ifacei_ipv6_unused_iid_handler(): "
                    "Deprecating the address", 0, 0, 0);
      memset(&addr_info, 0, sizeof(addr_info));
      addr_info.addr_mask  = PS_IFACE_ADDR_MGMT_MASK_ADDR_STATE;
      addr_info.addr_state = IPV6_ADDR_STATE_DEPRECATED;

      (void) ps_iface_addr_mgmti_set_addr_info(iface_ptr,
                                               &handle,
                                               &addr_info,
                                               &ps_errno);

      if(-1 == ps_icmp6_nd_send_ns(rm_iface_ptr, &ip_addr,
                                   ICMP6_ND_ADDR_RESOLUTION))
      {
        LOG_MSG_ERROR("ps_ifacei_ipv6_unused_iid_handler(): "
                      "ps_icmp6_nd_send_ns() failed.", 0, 0, 0);
        (void) ps_iface_addr_mgmt_free_addr(iface_ptr, &handle,
                                            NULL, &ps_errno);
      }
      (void) ps_timer_start(v6_addr_ptr->unused_addr_timer_handle,
                            IPV6_EXT_ADDR_WAIT_TIME);
    }
    else if(addr_info.addr_state == IPV6_ADDR_STATE_DEPRECATED)
    {
      LOG_MSG_INFO1("ps_ifacei_ipv6_unused_iid_handler(): "
                    "Freeing the address.", 0, 0, 0);
      (void) ps_iface_addr_mgmt_free_addr(iface_ptr, &handle,
                                          NULL, &ps_errno);
    }
    return;
  }

  /*-------------------------------------------------------------------------
    For addresses which require DAD, if the address has not been verified yet
    (TENTATIVE) and this timer expires, it indicates the address must be VALID
    since no message has come back indicating a conflict.
  -------------------------------------------------------------------------*/
  if(v6_addr_ptr->addr_state == IPV6_ADDR_STATE_TENTATIVE)
  {
    (void) ps_iface_addr_mgmt_ipv6_dad_update(iface_ptr,
                                              &handle,
                                              IFACE_ADDR_MGMT_DAD_TIMEOUT,
                                              &ps_errno);
    return;
  }
#endif /* FEATURE_DATA_PS_ADDR_MGMT */

  /*-------------------------------------------------------------------------
    This case is for internal privacy extension addresses that have been
    allocated, but not used. Since this timer has expired free the address.
  -------------------------------------------------------------------------*/
  IPV6_ADDR_MSG(ip_addr.ps_s6_addr64);
  LOG_MSG_INFO1("was never bound to a socket. Deleting the address.",0,0,0);

  if(-1 == ps_iface_delete_priv_ipv6_addr(iface_ptr, v6_addr_ptr))
  {
    LOG_MSG_ERROR("Error deleting private shared address on iface %p!",
              iface_ptr, 0, 0);
  }

  LOG_MSG_INFO1("ps_ifacei_ipv6_unused_iid_handler(): Success", 0, 0, 0);
}

/*===========================================================================
FUNCTION PS_IFACEI_UPDATE_IPV6_PRIV_ADDR_LIFETIMES()

DESCRIPTION
  This function updates the lifetimes of privacy addresses.

PARAMETERS
  iface_ptr:       The pointer to the interface on which to operate.
  def_v6_addr_ptr: The default address with the new prefix.
  pref_lifetime:   The new prefix's preferred lifetime
  valid_lifetime:  The new prefix's valid lifetime

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/

void ps_ifacei_update_ipv6_priv_addr_lifetimes
(
  ps_iface_type          *iface_ptr,
  ps_ifacei_v6_addr_type *def_v6_addr_ptr,
  int64                   pref_lifetime,
  int64                   valid_lifetime
)
{
  ps_ifacei_v6_addr_type *v6_addr_ptr;
  struct ps_in6_addr addr1;
  struct ps_in6_addr addr2;
  int64 temp_time = 0;
  uint8 i         = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (!PS_IFACE_IS_VALID(iface_ptr))
  {
    ASSERT(0);
    return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    If iface has inherit IP info true then get the base iface of the logical
    chain, else do nothing.
  -------------------------------------------------------------------------*/
  iface_ptr = PS_IFACE_GET_BASE_IFACE(iface_ptr);

  /*-------------------------------------------------------------------------
    Setup the default address
    *Update when address with prefixes > 64bits are supported.
  -------------------------------------------------------------------------*/
  addr1.ps_s6_addr64[0] = def_v6_addr_ptr->prefix;
  addr1.ps_s6_addr64[1] = 0;

  for(i = MAX_IPV6_PREFIXES; i < MAX_IPV6_ADDRS; i++)
  {
    if(NULL == (v6_addr_ptr = iface_ptr->iface_private.ipv6_addrs[i]))
    {
      break;
    }

    addr2.ps_s6_addr64[0] = v6_addr_ptr->prefix;
    addr2.ps_s6_addr64[1] = 0;

      /*---------------------------------------------------------------------
      If the prefixes don't match go to the next address.
      ---------------------------------------------------------------------*/
    if(!IN6_ARE_PREFIX_EQUAL(&addr1, &addr2, def_v6_addr_ptr->prefix_len))
    {
      continue;
    }

      /*---------------------------------------------------------------------
      Skip EXTERNAL addresses. (Note that we don't maintain pref and valid
      lifetime timers for them. See ps_iface_addr_mgmt_alloc_addr())
      ---------------------------------------------------------------------*/
    if(v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_EXTERNAL)
    {
      continue;
    }

    if(v6_addr_ptr->pref_lifetimer_handle != 0)
    {
      if(-1 != (temp_time = ps_timer_remaining(v6_addr_ptr->pref_lifetimer_handle)))
      {
        if(temp_time > pref_lifetime)
        {
          (void) ps_timer_cancel(v6_addr_ptr->pref_lifetimer_handle);

          (void) ps_timer_start(v6_addr_ptr->pref_lifetimer_handle, pref_lifetime);
        }
      }
    }

    if(v6_addr_ptr->valid_lifetimer_handle != 0)
    {
      if(-1 != (temp_time = ps_timer_remaining(v6_addr_ptr->valid_lifetimer_handle)))
      {
        if(temp_time > valid_lifetime)
        {
          (void) ps_timer_cancel(v6_addr_ptr->valid_lifetimer_handle);

          (void) ps_timer_start(v6_addr_ptr->valid_lifetimer_handle, valid_lifetime);
        }
      }
    }
  } /* end for */

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

} /* ps_ifacei_update_ipv6_priv_addr_lifetimes() */

/*===========================================================================
FUNCTION PS_IFACEI_GET_LINKLOCAL_ADDR()

DESCRIPTION
  This function is used to get the link local address of the V6 interface.
  The function sets the addr type to invalid if the call fails.

  Since the IP address can either be stored in this IFACE or an associated
  IFACE, find the IP address, based on the inherit_ip_info boolean.

  This implies recursion (though enforced through a static var to depth = 1).
  Recursion is enforced via ASSERT.  In the first call to this function,
  we drill down to the iface which actually contains the linklocal addr (i.e.
  the base iface).  Then we call this function again (recursion), passing
  the base iface.  Since we are at the base iface, there is no farther
  to drill down, and we get the linklocal addr.

  Note that to return a valid IP addr, both the passed in IFACE and the
  associated PS_IFACE must be in a valid IFACE state.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  ip_addr_ptr:    value return - the address will be will be stored here

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  Copies the ipv6 addr from ps_iface_ptr to ip_addr_ptr.
===========================================================================*/
static void ps_ifacei_get_linklocal_addr
(
  ps_iface_type    * this_iface_ptr,
  ps_ip_addr_type  * ip_addr_ptr,
  uint8              recurse_level
)
{
  ps_iface_type    *base_iface_ptr;  /* iface with ip addr stored locally  */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (ip_addr_ptr == NULL || recurse_level > 1)
  {
    ASSERT(0);
    return;
  }

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return;
  }

  ip_addr_ptr->type =  IPV6_ADDR;

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    The address is only valid if the state of the interface is UP - in all
    other cases return invalid address.
  -------------------------------------------------------------------------*/
  switch(this_iface_ptr->iface_private.state)
  {
    case IFACE_DISABLED:
    case IFACE_DOWN:
    case IFACE_GOING_DOWN:
      /*---------------------------------------------------------------------
        Return the invalid address, the address is invalid in this state
      ---------------------------------------------------------------------*/
      ip_addr_ptr->type = IP_ADDR_INVALID;
      break;

    case IFACE_CONFIGURING:
      /*---------------------------------------------------------------------
        Note that the address could be 0 in this state but its still valid
      ---------------------------------------------------------------------*/
    case IFACE_COMING_UP:
    case IFACE_ROUTEABLE:
    case IFACE_UP:
      /*---------------------------------------------------------------------
        If iface has inherit IP info true then get the base iface of the
        logical chain, else do nothing.
      ---------------------------------------------------------------------*/
      base_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

      /*---------------------------------------------------------------------
        if PS_IFACE_GET_BASE_IFACE returned iface_ptr with inhertied flag
        set, then it indicates error.
      ---------------------------------------------------------------------*/
      if (PS_IFACEI_IP_INFO_IS_INHERITED(base_iface_ptr))
      {
        PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
        LOG_MSG_ERROR("Invalid associated iface 0x%p", base_iface_ptr, 0, 0);
        ASSERT(0);
        return;
      }

      if (base_iface_ptr != this_iface_ptr)
      {
        /*-------------------------------------------------------------------
          RECURSION!!!  Only if we actually followed the assoc iface chain
                        to the end.
        -------------------------------------------------------------------*/
        ps_ifacei_get_linklocal_addr(base_iface_ptr,
                                     ip_addr_ptr,
                                     recurse_level + 1);
      }
      else
      {
        /*-------------------------------------------------------------------
          Get the address.  Could have come here directly or via the
          recursive call.
        -------------------------------------------------------------------*/
        if(!ps_iface_addr_family_is_v6(this_iface_ptr))
        {
          ip_addr_ptr->type = IP_ADDR_INVALID;
        }
        else
        {
          ASSERT(this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX]
                   != NULL);
          ip_addr_ptr->type = IPV6_ADDR;
          ip_addr_ptr->addr.v6.ps_s6_addr32[0] = ps_htonl(0xFE800000UL);
          ip_addr_ptr->addr.v6.ps_s6_addr32[1] = 0;
          ip_addr_ptr->addr.v6.ps_s6_addr64[1] =
            this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX]->iid;
        }
      }
      break;

    default:
      ASSERT(0);
  } /* switch(iface state) */

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

} /* ps_ifacei_get_linklocal_addr() */



/*===========================================================================

                             PUBLIC FUNCTIONS

===========================================================================*/
/*===========================================================================
FUNCTION PS_IFACE_ADDR_V6_INIT()

DESCRIPTION
  Initializes ps_iface_addr_v6 module

DEPENDENCIES
  None

PARAMETERS
  None

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_addr_v6_init
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  /*-------------------------------------------------------------------------
    Initialize Pools
  -------------------------------------------------------------------------*/
  if (PS_MEM_POOL_INIT_OPT(PS_MEM_IPV6_ADDR_TYPE,
                           ps_ifacei_v6_addr_buf_mem,
                           PS_IFACEI_V6_ADDR_BUF_SIZE,
                           PS_IFACEI_V6_ADDR_BUF_NUM,
                           PS_IFACEI_V6_ADDR_BUF_HIGH_WM,
                           PS_IFACEI_V6_ADDR_BUF_LOW_WM,
                           NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                           (int *) ps_ifacei_v6_addr_buf_hdr,
                           (int *) ps_ifacei_v6_addr_buf_ptr
#else
                           NULL,
                           NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                          ) == -1)
  {
    LOG_MSG_FATAL_ERROR("Can't init the module", 0, 0, 0);
  }

#ifdef FEATURE_DATA_PS_ADDR_MGMT
  ps_handle_mgr_init_client(PS_HANDLE_MGR_CLIENT_IPV6_ADDR,
                            PS_IFACEI_V6_ADDR_BUF_NUM,
                            0,
                            0);
#endif /* FEATURE_DATA_PS_ADDR_MGMT */

} /* ps_iface_addr_v6_init() */

/*===========================================================================
FUNCTION PS_IFACE_GET_ALL_V6_PREFIXES()

DESCRIPTION
  This function will retrieve all of the prefixes on an interface along
  with the state and length of each prefix.

PARAMETERS
  this_iface_ptr: The pointer to the interface on which to cleanup the
                  neighbor discovery caches.
  prefix_info:    The prefix and its state and length.
  num_prefixes:   The space alloc'd for prefixes and the number passed back

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_get_all_v6_prefixes
(
  ps_iface_type             *this_iface_ptr,
  ps_iface_prefix_info_type *prefix_info,
  uint8                     *num_prefixes
)
{
  uint8 index = 0;
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  if(!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return;
  }

  if(prefix_info == NULL || num_prefixes == NULL)
  {
    LOG_MSG_ERROR("Null value passed to get_all_v6_prefixes()",0,0,0);
    ASSERT(0);
    return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    If iface has inherit IP info true then get the base iface of the logical
    chain, else do nothing.
  --------------------------------------------------------------------------*/
  this_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

  for(index = 0; (index < *num_prefixes && index < MAX_IPV6_PREFIXES); index++)
  {
    if (NULL == this_iface_ptr->iface_private.ipv6_addrs[index] ||
        IPV6_ADDR_TYPE_PUBLIC !=
        this_iface_ptr->iface_private.ipv6_addrs[index]->addr_type)
    {
      break;
    }

    memset(&prefix_info[index].prefix, 0, sizeof(prefix_info[index].prefix));

    memcpy(&prefix_info[index].prefix,
           &this_iface_ptr->iface_private.ipv6_addrs[index]->prefix,
           sizeof(this_iface_ptr->iface_private.ipv6_addrs[index]->prefix));

    prefix_info[index].prefix_state =
      this_iface_ptr->iface_private.ipv6_addrs[index]->addr_state;

    prefix_info[index].prefix_len =
      this_iface_ptr->iface_private.ipv6_addrs[index]->prefix_len;
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

  *num_prefixes = index;

  return;
}

/*===========================================================================
FUNCTION PS_IFACE_GET_LINKLOCAL_ADDR()

DESCRIPTION
  This function is used to get the link local address of the V6 interface.
  The function sets the addr type to invalid if the call fails.

  Since the IP address can either be stored in this IFACE or an associated
  IFACE, find the IP address, based on the inherit_ip_info boolean.

  This implies recursion (though enforced through a static var to depth = 1).
  Recursion is enforced via ASSERT.  In the first call to this function,
  we drill down to the iface which actually contains the linklocal addr (i.e.
  the base iface).  Then we call this function again (recursion), passing
  the base iface.  Since we are at the base iface, there is no farther
  to drill down, and we get the linklocal addr.

  Note that to return a valid IP addr, both the passed in IFACE and the
  associated PS_IFACE must be in a valid IFACE state.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  ip_addr_ptr:    value return - the address will be will be stored here

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  Copies the ipv6 addr from ps_iface_ptr to ip_addr_ptr.
===========================================================================*/
void ps_iface_get_linklocal_addr
(
  ps_iface_type *this_iface_ptr,
  ps_ip_addr_type  *ip_addr_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  ps_ifacei_get_linklocal_addr(this_iface_ptr, ip_addr_ptr, 0);

} /* ps_iface_get_linklocal_addr() */


/*===========================================================================
FUNCTION PS_IFACE_FIND_IPV6_ADDR()

DESCRIPTION
  This function locates an IPv6 address on any UP interface.

PARAMETERS
  ip_addr_ptr:       Pointer to the ip address
  v6_addr_ptr_ptr:   Pointer to the v6_addr_ptr to fill if the addr is found
  ps_iface_ptr_ptr:  Pointer to the ps_iface_ptr to return if the address
                     matches one owned by that interface.

RETURN VALUE
  TRUE  if the address is located
  FALSE if the address cannot be found

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
boolean ps_iface_find_ipv6_addr
(
  struct ps_in6_addr       *ip_addr_ptr,
  ps_ifacei_v6_addr_type  **v6_addr_ptr_ptr,
  ps_iface_type           **ps_iface_ptr_ptr
)
{
  return ps_iface_find_ipv6_addr_ex(ip_addr_ptr,
                                     v6_addr_ptr_ptr,
                                     ps_iface_ptr_ptr,
                                     IPV6_ADDR_TYPE_ALL);
}

/*===========================================================================
FUNCTION PS_IFACE_FIND_IPV6_ADDR_EX()

DESCRIPTION
  This function locates an IPv6 address on any UP interface. While searching
  it includes the IPv6 of address type specified in addr_type_mask mask.

PARAMETERS
  ip_addr_ptr:       Pointer to the ip address
  v6_addr_ptr_ptr:   Pointer to the v6_addr_ptr to fill if the addr is found
  ps_iface_ptr_ptr:  Pointer to the ps_iface_ptr to return if the address
                     matches one owned by that interface.
  addr_type_mask:    IPv6 address type mask that indicates what type of
                     addresses should be included in the search.

RETURN VALUE
  TRUE  if the address is located
  FALSE if the address cannot be found

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
boolean ps_iface_find_ipv6_addr_ex
(
  struct ps_in6_addr                      *ip_addr_ptr,
  ps_ifacei_v6_addr_type                 **v6_addr_ptr_ptr,
  ps_iface_type                          **ps_iface_ptr_ptr,
  ps_iface_ipv6_addr_type_mask_enum_type   addr_type_mask
)
{
  ps_iface_ipv6_addr_type_enum_type  addr_type;
  ps_iface_type                     *iface_ptr = NULL;

  uint8   iface_index;
  uint8   addr_index;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if(ip_addr_ptr == NULL)
  {
    LOG_MSG_ERROR("Invalid ip_addr_ptr %p", ip_addr_ptr, 0, 0);
    return FALSE;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  if (NULL != ps_iface_ptr_ptr && NULL != *ps_iface_ptr_ptr)
  {
    iface_ptr = *ps_iface_ptr_ptr;

    if(PS_IFACE_IS_VALID(iface_ptr))
    {
      /*---------------------------------------------------------------------
        If iface has inherit IP info true then get the base iface of the
        logical chain, else do nothing.
      ---------------------------------------------------------------------*/
      iface_ptr = PS_IFACE_GET_BASE_IFACE(iface_ptr);

      /*----------------------------------------------------------------------
        Find the IPv6 address. If it matches the full or link local address
        return the address structure.
      ----------------------------------------------------------------------*/
      for(addr_index = 0; addr_index < MAX_IPV6_ADDRS; addr_index++)
      {
        if( iface_ptr->iface_private.ipv6_addrs[addr_index] == NULL )
        {
          break;
        }

        addr_type = iface_ptr->iface_private.ipv6_addrs[addr_index]->addr_type;

        /*----------------------------------------------------------------------
          Search through IP addresses of type specified in addr_type_mask
        ----------------------------------------------------------------------*/
        if( ((addr_type_mask & (1 << addr_type)) == (unsigned int)(1 << addr_type)) &&
            (((ip_addr_ptr->ps_s6_addr64[0] ==
              iface_ptr->iface_private.ipv6_addrs[addr_index]->prefix) &&
             (ip_addr_ptr->ps_s6_addr64[1] ==
              iface_ptr->iface_private.ipv6_addrs[addr_index]->iid)) ||
            (PS_IN6_IS_ADDR_LINKLOCAL(ip_addr_ptr) &&
             (ip_addr_ptr->ps_s6_addr64[1] ==
               iface_ptr->iface_private.ipv6_addrs[addr_index]->iid))) )
        {
          /*-----------------------------------------------------------------
            Address match found. Return the appropriate interface and address
            ptr if requested.
          -----------------------------------------------------------------*/
          if(v6_addr_ptr_ptr != NULL)
          {
            *v6_addr_ptr_ptr = iface_ptr->iface_private.ipv6_addrs[addr_index];
          }
          PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
          return TRUE;
        }
      }/* end for */
    }
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return FALSE;
  }
  /*--------------------------------------------------------------------------
    Look through all IPv6 ifaces that are UP. Loopback table maintains this
    list of UP IPv6 interfaces.
  --------------------------------------------------------------------------*/
  for(iface_index = 0; iface_index < MAX_IPV6_LO_IFACES; iface_index++)
  {
    iface_ptr = global_v6_lo_route_array[iface_index].iface_ptr;

    if(PS_IFACE_IS_VALID(iface_ptr))
    {
      /*---------------------------------------------------------------------
        If iface has inherit IP info true then get the base iface of the
        logical chain, else do nothing.
      ---------------------------------------------------------------------*/
      iface_ptr = PS_IFACE_GET_BASE_IFACE(iface_ptr);

      /*----------------------------------------------------------------------
        Find the IPv6 address. If it matches the full or link local address
        return the address structure.
      ----------------------------------------------------------------------*/
      for(addr_index = 0; addr_index < MAX_IPV6_ADDRS; addr_index++)
      {
        if( iface_ptr->iface_private.ipv6_addrs[addr_index] == NULL )
        {
          break;
        }

        addr_type = iface_ptr->iface_private.ipv6_addrs[addr_index]->addr_type;

        /*----------------------------------------------------------------------
          Search through IP addresses of type specified in addr_type_mask
        ----------------------------------------------------------------------*/
        if( ((addr_type_mask & (1 << addr_type)) == (unsigned int)(1 << addr_type)) &&
            (((ip_addr_ptr->ps_s6_addr64[0] ==
              iface_ptr->iface_private.ipv6_addrs[addr_index]->prefix) &&
             (ip_addr_ptr->ps_s6_addr64[1] ==
              iface_ptr->iface_private.ipv6_addrs[addr_index]->iid)) ||
            (PS_IN6_IS_ADDR_LINKLOCAL(ip_addr_ptr) &&
             (ip_addr_ptr->ps_s6_addr64[1] ==
               iface_ptr->iface_private.ipv6_addrs[addr_index]->iid))) )
        {
          /*-----------------------------------------------------------------
            Address match found. Return the appropriate interface and address
            ptr if requested.
          -----------------------------------------------------------------*/
          if(v6_addr_ptr_ptr != NULL)
          {
            *v6_addr_ptr_ptr = iface_ptr->iface_private.ipv6_addrs[addr_index];
          }
          if(ps_iface_ptr_ptr != NULL)
          {
            *ps_iface_ptr_ptr = iface_ptr;
          }
          PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
          return TRUE;
        }
      }/* end for */
    }
    else
    {
      /*---------------------------------------------------------------------
        Hit the end of the UP interfaces, the address either doesn't exist or
        the interface already went down in which case it has been cleaned up
        already.
      ---------------------------------------------------------------------*/
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      return FALSE;
    }
  } /* end for */

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return FALSE;
} /* ps_iface_find_ipv6_addr_ex */

/*===========================================================================
FUNCTION PS_IFACE_GET_V6_IID

DESCRIPTION
  This function returns the IPV6 IID of an iface.  If the iface is
  NULL or IPV4, then it returns NULL.

PARAMETERS
  this_iface_ptr: Target iface ptr

RETURN VALUE
  IPv6 interface identifier (last 64 bits of the address), 0 if interface
  pointer is NULL or iface is IPV4 family.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
uint64 ps_iface_get_v6_iid
(
  ps_iface_type       *this_iface_ptr
)
{
  ps_ip_addr_type            ip_addr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Set the ip_addr struct to all zero's.  This includes the field which
    holds the IP address.
  -------------------------------------------------------------------------*/
  if(PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ip_addr.type = IPV6_ADDR;

    PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

    /*-----------------------------------------------------------------------
      If iface has inherit IP info true then get the base iface of the
      logical chain, else do nothing.
    -----------------------------------------------------------------------*/
    this_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);

    ps_iface_get_addr(this_iface_ptr, &ip_addr);

    if (ip_addr.type == IP_ADDR_INVALID)
    {
      return(0);
    }
    else
    {
      return(ip_addr.addr.v6.ps_s6_addr64[1]);
    }
  }
  return(0);
} /* ps_iface_get_v6_iid() */

/*===========================================================================
FUNCTION PS_IFACE_GET_V6_PREFIX

DESCRIPTION
  This function returns the IPV6 PREFIX of an iface.  If the iface is
  NULL or IPV4, then it returns NULL.

PARAMETERS
  this_iface_ptr: Target iface ptr

RETURN VALUE
  IPv6 prefix (first 64 bits of the address), 0 if interface
  pointer is NULL or iface is IPV4 family.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
uint64 ps_iface_get_v6_prefix
(
  ps_iface_type       *this_iface_ptr
)
{
  ps_ip_addr_type            ip_addr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if(PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ip_addr.type = IPV6_ADDR;

    PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

    /*-----------------------------------------------------------------------
      If iface has inherit IP info true then get the base iface of the
      logical chain, else do nothing.
    -----------------------------------------------------------------------*/
    this_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);

    ps_iface_get_addr(this_iface_ptr, &ip_addr);

    if (ip_addr.type == IP_ADDR_INVALID)
    {
      return(0);
    }
    else
    {
      return(ip_addr.addr.v6.ps_s6_addr64[0]);
    }
  }
  return(0);
} /* ps_iface_get_v6_prefix() */

/*===========================================================================
FUNCTION PS_IFACE_V6_ADDR_MATCH

DESCRIPTION
  This function matches the passed IPv6 address with the possible IPv6
  addresses of the passed interface.

PARAMETERS
  struct ps_in6_addr * - Ptr to IPv6 address to match.
  ps_iface_type *   - Interface pointer.

RETURN VALUE
  TRUE  - If the passed address matches any of the IPv6 addr's of the iface.
  FALSE - Otherwise.

DEPENDENCIES
  None.

SIDE EFFECTS
  None
===========================================================================*/
boolean ps_iface_v6_addr_match
(
  struct ps_in6_addr *v6_addr_ptr,
  ps_iface_type   *if_ptr
)
{
  ps_iface_type *base_iface_ptr;
  int            i;
  boolean        ret_val = FALSE;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Sanity checks.
  -------------------------------------------------------------------------*/
  if(v6_addr_ptr == NULL || !PS_IFACE_IS_VALID(if_ptr))
  {
    LOG_MSG_INFO1("Invalid parameter passed to ps_iface_v6_addr_match()",0,0,0);
    return FALSE;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  if( if_ptr->iface_private.state == IFACE_DISABLED ||
      if_ptr->iface_private.state == IFACE_DOWN ||
      if_ptr->iface_private.state == IFACE_COMING_UP)
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_INFO1("Invalid iface ptr passed to ps_iface_v6_addr_match()",0,0,0);
    return FALSE;
  }

  if(PS_IN6_IS_ADDR_MULTICAST(v6_addr_ptr->ps_s6_addr))
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_INFO1("Multicast addr wont match with iface v6 addr",0,0,0);
    return FALSE;
  }

  /*-------------------------------------------------------------------------
    If iface has inherit IP info true then get the base iface of the logical
    chain, else do nothing.
  --------------------------------------------------------------------------*/
  base_iface_ptr = PS_IFACE_GET_BASE_IFACE(if_ptr);

  /*-------------------------------------------------------------------------
    if PS_IFACE_GET_BASE_IFACE returned iface_ptr with inhertied flag set,
    then it indicates error.
  -------------------------------------------------------------------------*/
  if (PS_IFACEI_IP_INFO_IS_INHERITED(base_iface_ptr))
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Invalid associated iface 0x%p", base_iface_ptr, 0, 0);
    ASSERT(0);
    return FALSE;
  }
  /*-------------------------------------------------------------------------
    Now Compare prefixes and iids. This will need to be modified for
    supporting multiple prefixes.
  -------------------------------------------------------------------------*/
  for(i=0;i<MAX_IPV6_ADDRS;i++)
  {
    if(base_iface_ptr->iface_private.ipv6_addrs[i] == NULL)
    {
      break;
    }

    if(v6_addr_ptr->ps_s6_addr64[1] !=
         base_iface_ptr->iface_private.ipv6_addrs[i]->iid)
    {
      continue;
    }
    else if(!(PS_IN6_IS_ADDR_LINKLOCAL(v6_addr_ptr->ps_s6_addr) ||
              PS_IN6_IS_ADDR_SITELOCAL(v6_addr_ptr->ps_s6_addr)))
    {
      /*---------------------------------------------------------------------
        Match prefix only if prefix is global.
      ---------------------------------------------------------------------*/
      if(v6_addr_ptr->ps_s6_addr64[0] ==
           base_iface_ptr->iface_private.ipv6_addrs[i]->prefix)
      {
        ret_val = TRUE;
        /*---------------------------------------------------------------------
          Address must be in a valid state to send/receive packets.
        ---------------------------------------------------------------------*/
        if( (base_iface_ptr->iface_private.ipv6_addrs[i]->addr_state ==
             IPV6_ADDR_STATE_VALID) ||
            (base_iface_ptr->iface_private.ipv6_addrs[i]->addr_state ==
             IPV6_ADDR_STATE_DEPRECATED))
        {
          break;
        }
        continue;
      }
    }
    else
    {
      ret_val = TRUE;
      break;
    }
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return ret_val;

}/* ps_iface_v6_addr_match() */


/*===========================================================================
FUNCTION PS_IFACE_SET_V6_IID

DESCRIPTION
  This function sets the IPV6 IID of an iface.  If the iface is
  NULL or IPV4 or if the IID conflicts with any other associated IIDs,
  then it returns FALSE.

PARAMETERS
  this_iface_ptr: pointer to the interface in question.
  iid:       64-bit IPv6 interface identifier (the v6 address suffix)

RETURN VALUE
  FALSE if interface pointer is NULL or IPV4 or Logical, TRUE otherwise

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
boolean ps_iface_set_v6_iid
(
  ps_iface_type       *this_iface_ptr,
  uint64               iid
)
{
  ps_iface_event_info_u_type              event_info;
  uint64                                  old_iid;
  boolean                                 ret_val = FALSE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Error checks: 1) is iface valid?
                  2) is the ip info inherited?
                  3) is the IP addr family correct?
                  4) is IID conflicts with other IIDs stored on IFACE?
  -------------------------------------------------------------------------*/

  if ( !PS_IFACE_IS_VALID(this_iface_ptr) )
  {
    return(FALSE);
  }

  do
  {
    PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

    if ( PS_IFACEI_IP_INFO_IS_INHERITED(this_iface_ptr) ||
         ps_iface_addr_family_is_v4(this_iface_ptr) == TRUE )
    {
      ret_val = FALSE;
      break;
    }

    ASSERT(this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX] != NULL);

    /*-----------------------------------------------------------------------
      Exclude Public IID of iface while checking for conflict. This is
      done because the conflict test will fail in PPP Resync case
      when the new IID is same as earlier IID.
    -----------------------------------------------------------------------*/
    if ( ps_iface_check_ipv6_iid_conflict(this_iface_ptr, iid, TRUE) )
    {
      LOG_MSG_ERROR("Primary IPv6 IID %lu conflicts with existing IIDs",
                    iid, 0, 0);
      ret_val = FALSE;
      break;
    }

    /*-----------------------------------------------------------------------
      Everything is okay.  Set the IID.
    -----------------------------------------------------------------------*/
    old_iid = this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX]->iid;
    this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX]->iid = iid;

    /*-------------------------------------------------------------------------
      Iface has an IP address in UP and ROUTEABLE state. Post
      IFACE_ADDR_CHANGED_EV if address is changed.

      Iface will also have an IP address in CONFIGURING state if PPP resync
      happened or if IP reconfiguration is triggered. Iface will not have
      an IP address during the initial configuration though. So post
      IFACE_ADDR_CHANGED_EV only if old address is not 0 and if address is
      changed.
    -------------------------------------------------------------------------*/
    if ((PS_IFACEI_GET_STATE(this_iface_ptr) == IFACE_UP ||
         PS_IFACEI_GET_STATE(this_iface_ptr) == IFACE_ROUTEABLE ||
         (PS_IFACEI_GET_STATE(this_iface_ptr) == IFACE_CONFIGURING &&
          old_iid != 0)) &&
        old_iid != iid)
    {
      event_info.ip_addr.type = IPV6_ADDR;
      event_info.ip_addr.addr.v6.ps_s6_addr64[0] =
        this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX]->prefix;
      event_info.ip_addr.addr.v6.ps_s6_addr64[1] = old_iid;
      ps_ifacei_invoke_event_cbacks(this_iface_ptr,
                                    NULL,
                                    IFACE_ADDR_CHANGED_EV,
                                    event_info);
    }

    ret_val = TRUE;

  } while (0); /* loop ends here */

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return ret_val;

} /* ps_iface_set_v6_iid() */


/*===========================================================================
FUNCTION PS_IFACE_GENERATE_PRIV_IPV6_ADDR()

DESCRIPTION
  This function will generate a new private IPv6 as per RFC 3041. Based on
  the input to the function it will create a private shared (interface
  based) or private unique (owned and useable only by a particular socket).

  The algorithm is as follows:

  1)If an address doesn't exist, create one. If not app initiated, done.
  2)If an address exists and an application requested it assign the addr
    to the application. Create one extra.
  3)If an extra address exists and it is initiated by receiving an RA, done.
  4)Create one additional address if necessary.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  ip_addr_ptr:    return value - the address will be stored here
  iid_param_ptr:  The type of address to create as well as whether it was
                  an application that requested the address or not.
  ps_errno:       the returned error code

RETURN VALUE
  0 for success
 -1 for failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_generate_priv_ipv6_addr
(
  ps_iface_type           *this_iface_ptr,
  ps_ip_addr_type         *ip_addr_ptr,
  ps_ipv6_iid_params_type *iid_param_ptr,
  int16                   *ps_errno
)
{
  ps_ifacei_v6_addr_type *v6_addr_ptr;
  int                     ret_val = 0;
  int                     ret     = 0;
  int16                   error_num;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#ifndef FEATURE_DSS_LINUX
  if(!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return -1;
  }

  if( (ip_addr_ptr == NULL) ||
      (iid_param_ptr == NULL) ||
      (ps_errno == NULL))
  {
    LOG_MSG_ERROR("ip_addr_ptr %p, iid_param_ptr %p, or ps_errno %p is NULL",
              ip_addr_ptr, iid_param_ptr, ps_errno);
    ASSERT(0);
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    If iface has inherit IP info true then get the base iface of the logical
    chain, else do nothing.
  -------------------------------------------------------------------------*/
  this_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

  if(!ps_iface_addr_family_is_v6(this_iface_ptr))
  {
    LOG_MSG_ERROR("Iface %p is not an IPv6 interface!", this_iface_ptr, 0, 0);
    ASSERT(0);
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return -1;
  }

  if(!ip6_sm_is_priv_ext_enabled())
  {
    LOG_MSG_INFO2("IPv6 Privacy Extension Support not enabled!",0,0,0);
    *ps_errno = DS_EOPNOTSUPP;
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return -1;
  }

#ifdef FEATURE_DATA_PS_MIPV6
  if(this_iface_ptr->name == MIP6_IFACE)
  {
    LOG_MSG_INFO2("Operation not supported for MIPv6 Iface!",0,0,0);
    *ps_errno = DS_EOPNOTSUPP;
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return -1;
  }
#endif /* FEATURE_DATA_PS_MIPV6 */

  /*-------------------------------------------------------------------------
    Privacy addresses can only be allocated on VALID prefixes.
  -------------------------------------------------------------------------*/
  if(this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX]->addr_state !=
     IPV6_ADDR_STATE_VALID)
  {
    LOG_MSG_ERROR("No valid prefix available!",0,0,0);
    *ps_errno = DS_ENOSRCADDR;
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return -1;
  }

  /*-------------------------------------------------------------------------
    If an application is requesting a private shared address verify that a
    valid one doesn't already exist. If it does return it. If it is a
    broadcast interface verify that one is not currently undergoing DAD.
  -------------------------------------------------------------------------*/
  if( (iid_param_ptr->app_request == TRUE) &&
      (iid_param_ptr->is_unique == FALSE))
  {
    if(PS_IFACEI_FIND_IPV6_ADDR_TYPE(this_iface_ptr,
                                     IPV6_ADDR_STATE_VALID,
                                     IPV6_ADDR_TYPE_PRIV_SHARED,
                                     &v6_addr_ptr) )
    {

      ip_addr_ptr->type = IPV6_ADDR;
      ip_addr_ptr->addr.v6.ps_s6_addr64[0] = v6_addr_ptr->prefix;
      ip_addr_ptr->addr.v6.ps_s6_addr64[1] = v6_addr_ptr->iid;

      /*---------------------------------------------------------------------
        If preferred lifetime timer is already started it means an app has
        already bound to this address at some point. We no longer start the
        unused timer in this case.
      ---------------------------------------------------------------------*/
      if(!ps_timer_is_running(v6_addr_ptr->pref_lifetimer_handle))
      {
        /*-------------------------------------------------------------------
          Start the unused activity timer. If an application doesn't bind
          before this expires delete the address.
        -------------------------------------------------------------------*/
        (void)ps_timer_start(v6_addr_ptr->unused_addr_timer_handle,
                             (int64)DEFAULT_UNUSED_IPV6_PRIV_ADDR_TIMEOUT);
      }
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      return 0;
    }
    else if(PS_IFACE_GET_IS_BCAST_IFACE(this_iface_ptr) &&
            PS_IFACEI_FIND_IPV6_ADDR_TYPE(this_iface_ptr,
                                          IPV6_ADDR_STATE_TENTATIVE,
                                          IPV6_ADDR_TYPE_PRIV_SHARED,
                                          &v6_addr_ptr) )
    {
      *ps_errno = DS_EWOULDBLOCK;
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      return -1;
    }
  }

  /*-------------------------------------------------------------------------
    1) Generate an extra address if one doesn't already exist. There must be
       no unassigned addresses, or if the iface is a broadcast interface,
       tentative addresses.
  -------------------------------------------------------------------------*/
  if(!PS_IFACE_IPV6_EXTRA_ADDR_IS_AVAIL(this_iface_ptr, &v6_addr_ptr))
  {
    /*-----------------------------------------------------------------------
      Allocate memory and generate the IID for the address. Set appropriate
      state and address type information if necessary.
    -----------------------------------------------------------------------*/
    ret = ps_iface_alloc_priv_ipv6_addr(this_iface_ptr, &v6_addr_ptr,
                                        iid_param_ptr, FALSE, &error_num);

    if(ret == 0)
    {
      if(iid_param_ptr->app_request)
      {
        /*-------------------------------------------------------------------
          Populate information back to the application.
        -------------------------------------------------------------------*/
        ip_addr_ptr->type = IPV6_ADDR;
        ip_addr_ptr->addr.v6.ps_s6_addr64[0] = v6_addr_ptr->prefix;
        ip_addr_ptr->addr.v6.ps_s6_addr64[1] = v6_addr_ptr->iid;
        /*-------------------------------------------------------------------
          Start the unused activity timer. If an application doesn't bind
          before this expires delete the address.
        -------------------------------------------------------------------*/
        (void)ps_timer_start(v6_addr_ptr->unused_addr_timer_handle,
                             (int64)DEFAULT_UNUSED_IPV6_PRIV_ADDR_TIMEOUT);
      }
      ret_val = 0;
    }
    else if(ret == -1 && error_num == DS_EWOULDBLOCK)
    {
      *ps_errno = error_num;
      ret_val = -1;
    }
    else
    {
      *ps_errno = error_num;
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      return -1;
    }
  }
  /*-------------------------------------------------------------------------
    2) Application Request, extra addr already exists, use this for the app
  -------------------------------------------------------------------------*/
  else if(iid_param_ptr->app_request)
  {
    if(iid_param_ptr->is_unique)
    {
      v6_addr_ptr->addr_type = IPV6_ADDR_TYPE_PRIV_UNIQUE;
    }
    else
    {
      v6_addr_ptr->addr_type = IPV6_ADDR_TYPE_PRIV_SHARED;
    }

    if(v6_addr_ptr->addr_state != IPV6_ADDR_STATE_TENTATIVE)
    {
      v6_addr_ptr->addr_state = IPV6_ADDR_STATE_VALID;
      ip_addr_ptr->type = IPV6_ADDR;
      ip_addr_ptr->addr.v6.ps_s6_addr64[0] = v6_addr_ptr->prefix;
      ip_addr_ptr->addr.v6.ps_s6_addr64[1] = v6_addr_ptr->iid;
      /*-------------------------------------------------------------------
        Start the unused activity timer. If an application doesn't bind
        before this expires delete the address.
      -------------------------------------------------------------------*/
      (void)ps_timer_start(v6_addr_ptr->unused_addr_timer_handle,
                           (int64)DEFAULT_UNUSED_IPV6_PRIV_ADDR_TIMEOUT);
      ret_val = 0;
    }
    else
    {
      *ps_errno = DS_EWOULDBLOCK;
      ret_val   = -1;
    }
  }
  /*-------------------------------------------------------------------------
    3) Extra address already exists, initiated by receiving valid RAs.
       Do Nothing.
  -------------------------------------------------------------------------*/
  else
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return 0;
  }


  /*-------------------------------------------------------------------------
    4) Generate the extra address. Ensure that an extra doesn't already
       exist (the case where an RA generates one, but it hasn't been used
       yet). No additional errors need be generated back if this fails
       as this is the extra address.
  -------------------------------------------------------------------------*/
  if(!PS_IFACE_IPV6_EXTRA_ADDR_IS_AVAIL(this_iface_ptr, &v6_addr_ptr))
  {
    ret = ps_iface_alloc_priv_ipv6_addr(this_iface_ptr, &v6_addr_ptr,
                                        iid_param_ptr, TRUE, &error_num);
  }
  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
#endif
  return ret_val;
} /* ps_iface_generate_priv_ipv6_addr() */

/*===========================================================================
FUNCTION PS_IFACE_ALLOC_PRIV_IPV6_ADDR()

DESCRIPTION
  This function will verify address space is available, allocate a ps mem
  item for a new private IPv6 address, allocate timers for the address,
  and populate the address with the necessary state information. In addition
  this function will start the DAD process for broadcast or proxy interfaces.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  ip_addr_ptr:    return value - the address will be stored here
  iid_param_ptr:  The type of address to create as well as whether it was
                  an application that requested the address or not.
  ps_errno:       the returned error code

RETURN VALUE
  0 for success
 -1 for failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_alloc_priv_ipv6_addr
(
  ps_iface_type           *this_iface_ptr,
  ps_ifacei_v6_addr_type **v6_addr_ptr_ptr,
  ps_ipv6_iid_params_type *iid_param_ptr,
  boolean                  extra_addr,
  int16                   *ps_errno
)
{
  uint8                   index;
  ps_ifacei_v6_addr_type *v6_addr_ptr;
  uint64                  priv_iid;
  struct ps_in6_addr      ipv6_addr;
#ifdef FEATURE_DATA_PS_ADDR_MGMT
  ps_iface_addr_mgmt_handle_type handle;
#endif
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_INFO1("ps_iface_alloc_priv_ipv6_addr()",0,0,0);
  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    If iface has inherit IP info true then get the base iface of the logical
    chain, else do nothing.
  -------------------------------------------------------------------------*/
  this_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

  ASSERT(this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX] != NULL);


  /*-----------------------------------------------------------------------
    Check if there is space to allocate a new address.
  -----------------------------------------------------------------------*/
  if(PS_IFACE_IPV6_IS_ADDR_SPACE_AVAIL(this_iface_ptr, &index))
  {
    this_iface_ptr->iface_private.ipv6_addrs[index] =
      (ps_ifacei_v6_addr_type *) ps_mem_get_buf(PS_MEM_IPV6_ADDR_TYPE);
    memset( this_iface_ptr->iface_private.ipv6_addrs[index],
            0,
            sizeof(ps_ifacei_v6_addr_type) );

    *v6_addr_ptr_ptr = this_iface_ptr->iface_private.ipv6_addrs[index];

    if(NULL == *v6_addr_ptr_ptr)
    {
      LOG_MSG_INFO2("Out of PS MEM IPv6 address items!",0,0,0);
      *ps_errno = DS_ENOMEM;
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      return -1;
    }

    v6_addr_ptr = *v6_addr_ptr_ptr;

  }
  else
  {
    LOG_MSG_INFO2("MAX IPv6 addresses already allocated for iface %p!",
            this_iface_ptr, 0, 0);
    *ps_errno = DS_ENOMEM;
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return -1;
  }

  /*-------------------------------------------------------------------------
    Generate a new random unique IID and populate all prefix, gateway, etc.
    information in the address structure.
  -------------------------------------------------------------------------*/
  (void)ps_iface_generate_ipv6_iid(this_iface_ptr,
                                   &priv_iid,
                                   ps_errno);

  memset(v6_addr_ptr, 0, sizeof(ps_ifacei_v6_addr_type));

  v6_addr_ptr->gateway_iid =
    this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX]->gateway_iid;

  v6_addr_ptr->prefix =
    this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX]->prefix;

  v6_addr_ptr->iid = priv_iid;

  ipv6_addr.in6_u.u6_addr64[0] = v6_addr_ptr->prefix;
  ipv6_addr.in6_u.u6_addr64[1] = v6_addr_ptr->iid;

  IPV6_ADDR_MSG(ipv6_addr.ps_s6_addr64);


#ifdef FEATURE_DATA_PS_ADDR_MGMT
  v6_addr_ptr->dad_retries  = IPV6_MAX_ADDR_DAD_RETRIES;
#endif

  /*-------------------------------------------------------------------------
    Allocate the timers for the privacy address.
  -------------------------------------------------------------------------*/
  v6_addr_ptr->pref_lifetimer_handle =
    ps_timer_alloc(ps_ifacei_ipv6_iid_timer_handler,
                   v6_addr_ptr);
  v6_addr_ptr->valid_lifetimer_handle =
    ps_timer_alloc(ps_ifacei_ipv6_iid_timer_handler,
                   v6_addr_ptr);
  v6_addr_ptr->unused_addr_timer_handle =
    ps_timer_alloc(ps_ifacei_ipv6_unused_iid_handler,
                   v6_addr_ptr);

  if(v6_addr_ptr->valid_lifetimer_handle == PS_TIMER_INVALID_HANDLE ||
     v6_addr_ptr->pref_lifetimer_handle == PS_TIMER_INVALID_HANDLE  ||
     v6_addr_ptr->unused_addr_timer_handle == PS_TIMER_INVALID_HANDLE)
  {
    LOG_MSG_ERROR("Error can't allocate timers for IPv6 Privacy addr!",0,0,0);
    PS_TIMER_FREE_HANDLE(v6_addr_ptr->valid_lifetimer_handle);
    PS_TIMER_FREE_HANDLE(v6_addr_ptr->pref_lifetimer_handle);
    PS_TIMER_FREE_HANDLE(v6_addr_ptr->unused_addr_timer_handle);
    PS_MEM_FREE(this_iface_ptr->iface_private.ipv6_addrs[index]);
    *ps_errno = DS_ENOMEM;
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return -1;
  }

  /*-------------------------------------------------------------------------
    If this address is for an application set the appropriate address type.
    Otherwise set the type to INVALID. Set the address state as well.
  -------------------------------------------------------------------------*/
  if(iid_param_ptr->app_request == TRUE && !extra_addr)
  {
    if(iid_param_ptr->is_unique)
    {
      v6_addr_ptr->addr_type = IPV6_ADDR_TYPE_PRIV_UNIQUE;
    }
    else
    {
      v6_addr_ptr->addr_type = IPV6_ADDR_TYPE_PRIV_SHARED;
    }

    v6_addr_ptr->addr_state = IPV6_ADDR_STATE_VALID;
  }
  else
  {
    v6_addr_ptr->addr_type  = IPV6_ADDR_TYPE_INVALID;
    v6_addr_ptr->addr_state = IPV6_ADDR_STATE_UNASSIGNED;
  }

  /*-------------------------------------------------------------------------
    For broadcast or proxy interfaces the state should be tentative because
    DAD must first be performed on the address.
  -------------------------------------------------------------------------*/
  if( PS_IFACE_GET_IS_BCAST_IFACE(this_iface_ptr) ||
      PS_IFACE_GET_IS_PROXY_IFACE(this_iface_ptr) )
  {
    v6_addr_ptr->addr_state = IPV6_ADDR_STATE_TENTATIVE;

#ifdef FEATURE_DATA_PS_ADDR_MGMT
    if( -1 !=
        (handle = ps_iface_addr_mgmt_get_handle_from_addr( v6_addr_ptr )) )
    {
      /*---------------------------------------------------------------------
        Kickoff DAD procesing.  If Iface does not have DAD callback
        registered, this is a no-op.
      ---------------------------------------------------------------------*/
      if( -1 == ps_iface_addr_mgmt_ipv6_do_dad( this_iface_ptr,
                                                &handle,
                                                NULL,
                                                ps_errno ) )
      {
        PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
        LOG_MSG_ERROR("Failure to initiate IPV6 DAD processing", 0, 0, 0);
        *ps_errno = DS_EFAULT;
        return -1;
      }
    }
    else
    {
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      LOG_MSG_ERROR("Failure to find IPV6 addr handle", 0, 0, 0);
      *ps_errno = DS_EFAULT;
      return -1;
    }
#endif /* FEATURE_DATA_PS_ADDR_MGMT */
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  LOG_MSG_INFO1("ps_iface_alloc_priv_ipv6_addr: success",0,0,0);
  return 0;
} /* ps_iface_alloc_priv_ipv6_addr() */

/*===========================================================================
FUNCTION PS_IFACE_DELETE_PRIV_IPV6_ADDR()

DESCRIPTION
  This function will delete an old private IPv6 address. Once the ref_cnt
  has gone to zero this function will take care of deleting and cleaning up
  the interface's ipv6 addresses.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  v6_addr_ptr:    the ipv6 address location to be removed from the interface

RETURN VALUE
  0 for success
 -1 for failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_delete_priv_ipv6_addr
(
  ps_iface_type           *this_iface_ptr,
  ps_ifacei_v6_addr_type  *v6_addr_ptr
)
{
  uint8   i;
  boolean addr_deleted = FALSE;
  boolean invoke_cback = TRUE;
  ps_iface_event_info_u_type event_info;
  struct ps_in6_addr      ipv6_addr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1("ps_iface_delete_priv_ipv6_addr()",0,0,0);
  memset(&event_info, 0, sizeof(event_info));

  if(!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return -1;
  }

  if(v6_addr_ptr == NULL)
  {
    LOG_MSG_ERROR("NULL IPv6 address ptr %p", v6_addr_ptr, 0, 0);
    ASSERT(0);
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    If iface has inherit IP info true then get the base iface of the logical
    chain, else do nothing.
  -------------------------------------------------------------------------*/
  this_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

  /*--------------------------------------------------------------------------
    Free all timers, remove the IPv6 privacy address, generate DELETED_EV and
    remove all event callbacks.
  --------------------------------------------------------------------------*/
  PS_TIMER_FREE_HANDLE(v6_addr_ptr->pref_lifetimer_handle);
  PS_TIMER_FREE_HANDLE(v6_addr_ptr->valid_lifetimer_handle);
  PS_TIMER_FREE_HANDLE(v6_addr_ptr->unused_addr_timer_handle);

  if(v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_PRIV_UNIQUE)
  {
    event_info.priv_ipv6_addr.is_unique = TRUE;
  }
  else if(v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_PRIV_SHARED)
  {
    event_info.priv_ipv6_addr.is_unique = FALSE;
  }
  else
  {
    invoke_cback = FALSE;
  }

  ipv6_addr.in6_u.u6_addr64[0] = v6_addr_ptr->prefix;
  ipv6_addr.in6_u.u6_addr64[1] = v6_addr_ptr->iid;

  IPV6_ADDR_MSG(ipv6_addr.ps_s6_addr64);

  event_info.priv_ipv6_addr.ip_addr.type = IPV6_ADDR;
  event_info.priv_ipv6_addr.ip_addr.addr.v6.ps_s6_addr64[0] =
                                                       v6_addr_ptr->prefix;
  event_info.priv_ipv6_addr.ip_addr.addr.v6.ps_s6_addr64[1] =
                                                          v6_addr_ptr->iid;

  /*-------------------------------------------------------------------------
    Find and free the IPv6 address memory.
  -------------------------------------------------------------------------*/
  for(i=0; i < MAX_IPV6_ADDRS; i++)
  {
    if(this_iface_ptr->iface_private.ipv6_addrs[i] == NULL)
    {
      break;
    }

    if(this_iface_ptr->iface_private.ipv6_addrs[i] == v6_addr_ptr)
    {
      PS_MEM_FREE(this_iface_ptr->iface_private.ipv6_addrs[i]);
      addr_deleted = TRUE;
      break;
    }
  }

  if(!addr_deleted)
  {
    LOG_MSG_ERROR("Couldn't locate IPv6 address for deletion on iface %p",
              this_iface_ptr, 0, 0);
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return -1;
  }

  if(invoke_cback)
  {
    ps_ifacei_invoke_event_cbacks(this_iface_ptr,
                                  NULL,
                                  IFACE_IPV6_PRIV_ADDR_DELETED_EV,
                                  event_info);
  }

  LOG_MSG_INFO1("ps_iface_delete_priv_ipv6_addr(): "
                "Condensing IPv6 address array",0,0,0);
  /*--------------------------------------------------------------------------
    Cleanup and condense address array.
  --------------------------------------------------------------------------*/
  while( (i < (MAX_IPV6_ADDRS-1)) &&
         (this_iface_ptr->iface_private.ipv6_addrs[i+1] != NULL))
  {
    this_iface_ptr->iface_private.ipv6_addrs[i] =
      this_iface_ptr->iface_private.ipv6_addrs[i+1];
    i++;
  }
  this_iface_ptr->iface_private.ipv6_addrs[i] = NULL;

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  LOG_MSG_INFO1("ps_iface_delete_priv_ipv6_addr: Success",0,0,0);
  return 0;

} /* ps_iface_delete_priv_ipv6_addr() */


/*===========================================================================
FUNCTION PS_IFACE_APPLY_V6_PREFIX()

DESCRIPTION
  This function will apply a prefix to a particular interface.  In it's
  initial incarnation it will only store a single prefix - the only way to
  write a new prefix is to .  In future a more
  sophisticated method will be used to store prefixes.

PARAMETERS
  this_iface_ptr:  ptr to interface control block on which to operate on.
  gateway_iid: interface identifier of the router
  prefix: prefix being added.
  valid_lifetime: lifetime of prefix (seconds); see rfc 2461 (Section 4.6.2)
  pref_lifetime: preferred lifetime for prefix; see also rfc 2462 (Section 2)
  prefix_length

RETURN VALUE
  0 on successly applying prefix
 -1 on failure or prefix not applied

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_apply_v6_prefix
(
  ps_iface_type *this_iface_ptr,
  uint64         gateway_iid,
  uint64         prefix,
  uint32         valid_lifetime,
  uint32         pref_lifetime,
  uint8          prefix_length
)
{
/*lint -save -e715 */
  ps_iface_event_info_u_type event_info;
  ps_ifacei_v6_addr_type *v6_addr_ptr;
  int64  lifetime              = 0;
  int64  stored_valid_lifetime = 0;
  int64  tmp_pref_lifetime     = 0;
  int64  tmp_valid_lifetime    = 0;
  uint64 iface_ipv6_iid        = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  if ( PS_IFACEI_IP_INFO_IS_INHERITED(this_iface_ptr) )
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return(-1);
  }

  v6_addr_ptr    = this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX];

  /*-------------------------------------------------------------------------
    If the prefix is currently configured on the mobile, but has a lifetime
    of zero, remove the prefix.
  -------------------------------------------------------------------------*/
  if(v6_addr_ptr != NULL)
  {
    iface_ipv6_iid = v6_addr_ptr->iid;

    /*-----------------------------------------------------------------------
      If the router changed, remove all prefixes and set the gateway_iid in
      the PS iface to the new router. (Handoff)
    -----------------------------------------------------------------------*/
    if(gateway_iid != v6_addr_ptr->gateway_iid &&
       prefix      != v6_addr_ptr->prefix)
    {
      (void)ps_iface_delete_all_v6_prefixes( this_iface_ptr );
      v6_addr_ptr->iid         = iface_ipv6_iid;
      v6_addr_ptr->gateway_iid = gateway_iid;
    }
    /*-----------------------------------------------------------------------
      If the router changed but the new router assigned the same prefix as
      before, update the gateway iid only.
    -----------------------------------------------------------------------*/
    else if(gateway_iid != v6_addr_ptr->gateway_iid &&
            prefix      == v6_addr_ptr->prefix)
    {
      v6_addr_ptr->gateway_iid = gateway_iid;
    }
    /*-----------------------------------------------------------------------
      The mobile only supports one prefix. If the prefix received is
      different than the interface's current prefix, drop it.
    -----------------------------------------------------------------------*/
    else if(v6_addr_ptr->prefix != 0     &&
            v6_addr_ptr->prefix != prefix)
    {
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      return -1;
    }
  }
  else
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Default IPv6 index should never be NULL!",0,0,0);
    ASSERT(0);
    return -1;
  }

#ifndef FEATURE_DSS_LINUX /* Using kernel IPV6 state machine */
  /*-------------------------------------------------------------------------
    Allocated timers for the prefix if they don't already exist.
  -------------------------------------------------------------------------*/
  if(v6_addr_ptr->valid_lifetimer_handle == 0 &&
     v6_addr_ptr->pref_lifetimer_handle == 0)
  {
    v6_addr_ptr->valid_lifetimer_handle =
      ps_timer_alloc(ps_ifacei_v6_prefix_expired_handler, v6_addr_ptr);

    v6_addr_ptr->pref_lifetimer_handle =
      ps_timer_alloc(ps_ifacei_v6_prefix_deprecated_handler, v6_addr_ptr);

    if(v6_addr_ptr->pref_lifetimer_handle == PS_TIMER_INVALID_HANDLE ||
       v6_addr_ptr->valid_lifetimer_handle == PS_TIMER_INVALID_HANDLE)
    {
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      PS_TIMER_FREE_HANDLE(v6_addr_ptr->pref_lifetimer_handle);
      PS_TIMER_FREE_HANDLE(v6_addr_ptr->valid_lifetimer_handle);
      PS_TIMER_FREE_HANDLE(v6_addr_ptr->unused_addr_timer_handle);
      return -1;
    }
  }

  /*--------------------------------------------------------------------------
    If the either of the lifetimes is infinite don't start them - only start
    the ones that are not infinite. Cancel old timers if the new ones are
    infinite.
  --------------------------------------------------------------------------*/
  if(valid_lifetime != 0xFFFFFFFFUL)
  {
    /*-----------------------------------------------------------------------
      Adjust the lifetimes according to RFC 2462 5.5.3.
    -----------------------------------------------------------------------*/
    stored_valid_lifetime =
      (ps_timer_remaining(v6_addr_ptr->valid_lifetimer_handle) / 1000);

    if(valid_lifetime > MIN_IPV6_VALID_LIFETIME ||
       valid_lifetime > stored_valid_lifetime)
    {
      lifetime = ((int64) valid_lifetime);
    }
#ifdef PS_UNSUPPORTED
    else if(stored_valid_lifetime <= MIN_IPV6_VALID_LIFETIME &&
            (valid_lifetime) <= stored_valid_lifetime &&
              RA_authenticated == TRUE)
    {
      lifetime = ((int64)valid_lifetime);
    }
#endif
    else
    {
      lifetime = ((int64) MIN_IPV6_VALID_LIFETIME);
    }

    LOG_MSG_INFO2("Starting valid lifetimer %d for prefix 0x%llx and %ds",
            v6_addr_ptr->valid_lifetimer_handle,
            ps_ntohll(prefix),
            lifetime);

    if(ps_timer_start(v6_addr_ptr->valid_lifetimer_handle,
                      (lifetime*1000)) == PS_TIMER_FAILURE)
    {
      LOG_MSG_ERROR("Can't start the IPv6 prefix valid lifetimer!",0,0,0);
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      return -1;
    }

    tmp_valid_lifetime = lifetime*1000;
  }
  else
  {
    (void) ps_timer_cancel(v6_addr_ptr->valid_lifetimer_handle);
  }

  if(pref_lifetime != 0xFFFFFFFFUL)
  {
    LOG_MSG_INFO2("Starting preferred lifetimer %d for prefix 0x%llx and %ds",
            v6_addr_ptr->pref_lifetimer_handle,
            ps_ntohll(prefix),
            pref_lifetime);
    lifetime = ((int64)pref_lifetime);
    if(ps_timer_start(v6_addr_ptr->pref_lifetimer_handle,
                      lifetime*1000) == PS_TIMER_FAILURE)
    {
      LOG_MSG_ERROR("Can't start the IPv6 prefix preferred lifetimer!",0,0,0);
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      return -1;
    }
    tmp_pref_lifetime = lifetime*1000;
  }
  else
  {
    (void) ps_timer_cancel(v6_addr_ptr->pref_lifetimer_handle);
  }

  /*-------------------------------------------------------------------------
    Only update privacy addresses if lifetimes are non-zero.
  -------------------------------------------------------------------------*/
  if(tmp_pref_lifetime > 0 && tmp_valid_lifetime > 0)
  {
    (void) ps_ifacei_update_ipv6_priv_addr_lifetimes(this_iface_ptr,
                                                     v6_addr_ptr,
                                                     tmp_pref_lifetime,
                                                     tmp_valid_lifetime);
  }
#endif /* FEATURE_DSS_LINUX */

  /*-------------------------------------------------------------------------
    Only generate the event if a prefix is really applied.  This means either
    there was a new prefix added, OR that a deprecated prefix has been made
    preferred again OR the gateway changed.
  -------------------------------------------------------------------------*/
/*
#ifdef MSG_8
  MSG_8(MSG_SSID_DS,
        MSG_LEGACY_MED,
        "Applying prefix 0x%x%x to iface 0x%x:%d with IID 0x%llx \
pref/valid LT are %ds/%ds",
        ntohl((uint32)(prefix >> 32)),
        ntohl((uint32)prefix),
        this_iface_ptr->name,
        this_iface_ptr->instance,
        ntohl((uint32)(iface_ipv6_iid >> 32)),
        ntohl((uint32)iface_ipv6_iid),
        pref_lifetime,
        valid_lifetime);
#else
  LOG_MSG_INFO1("Applying prefix 0x%llx to iface 0x%x:%d",
           prefix,
           this_iface_ptr->name,
           this_iface_ptr->instance);
  LOG_MSG_INFO1("IID for iface 0x%x:%d is 0x%llx",
           this_iface_ptr->name,
           this_iface_ptr->instance,
           iface_ipv6_iid);
#endif
*/

  event_info.prefix_info.prefix.ps_s6_addr64[0]     = prefix;
  event_info.prefix_info.prefix_len              = prefix_length;

  if(prefix == v6_addr_ptr->prefix)
  {
    event_info.prefix_info.kind = PREFIX_UPDATED;
  }
  else
  {
    event_info.prefix_info.kind = PREFIX_ADDED;
  }
  v6_addr_ptr->addr_state                        = IPV6_ADDR_STATE_VALID;
  v6_addr_ptr->addr_type                         = IPV6_ADDR_TYPE_PUBLIC;
  v6_addr_ptr->prefix                            = prefix;
  v6_addr_ptr->prefix_len                        = prefix_length;
#ifdef FEATURE_DATA_PS_ADDR_MGMT
  v6_addr_ptr->dad_retries                       = IPV6_MAX_ADDR_DAD_RETRIES;
#endif

  ps_ifacei_invoke_event_cbacks(this_iface_ptr,
                                NULL,
                                IFACE_PREFIX_UPDATE_EV,
                                event_info);
  IP6_REPORT_PFX_UPDATE( this_iface_ptr, event_info.prefix_info );

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;
/*lint -restore */
} /* ps_iface_apply_v6_prefix() */


/*===========================================================================
FUNCTION PS_IFACE_REMOVE_V6_PREFIX()

DESCRIPTION
  This function will remove a prefix from the interface.  It will only fail if
  the prefix doesn't exist on this interface.

PARAMETERS
  this_iface_ptr:  ptr to interface control block on which to operate on.
  prefix: prefix being removed

RETURN VALUE
  0 on success
 -1 on failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_remove_v6_prefix
(
  ps_iface_type *this_iface_ptr,
  uint64         prefix
)
{
  ps_iface_event_info_u_type event_info;
  ps_ifacei_v6_addr_type     *v6_addr_ptr;
  uint8                       iface_index;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_INFO1("ps_iface_remove_v6_prefix()",0,0,0);

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    If iface has inherit IP info true then get the base iface of the logical
    chain, else do nothing.
  -------------------------------------------------------------------------*/
  this_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

  memset(&event_info, 0, sizeof(event_info));

  /*-------------------------------------------------------------------------
    Clear the public address, but don't free it. Only free the timers.
  -------------------------------------------------------------------------*/
  v6_addr_ptr = this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX];
  if(v6_addr_ptr != NULL && v6_addr_ptr->prefix == prefix)
  {
    event_info.prefix_info.prefix.ps_s6_addr64[0] = prefix;
    event_info.prefix_info.kind                = PREFIX_REMOVED;
    event_info.prefix_info.prefix_len          = v6_addr_ptr->prefix_len;

    PS_TIMER_FREE_HANDLE(v6_addr_ptr->pref_lifetimer_handle);
    PS_TIMER_FREE_HANDLE(v6_addr_ptr->valid_lifetimer_handle);

    memset(v6_addr_ptr,
           0,
           sizeof(ps_ifacei_v6_addr_type));
  }
  /*-------------------------------------------------------------------------
    Make sure that the prefix being removed is the same as the one that is
    already stored. Free any additional privacy addresses if they exist.
    Always delete private addrs at index 1. Since the array is condensed as
    part of address deletion if there are other addresses they will be moved
    to that index.
  -------------------------------------------------------------------------*/
  for(iface_index = MAX_IPV6_PREFIXES; iface_index < MAX_IPV6_ADDRS; iface_index++)
  {
    v6_addr_ptr = this_iface_ptr->iface_private.ipv6_addrs[MAX_IPV6_PREFIXES];

    if(v6_addr_ptr == NULL)
    {
      break;
    }

    if(v6_addr_ptr->prefix == prefix)
    {
      LOG_MSG_INFO1("ps_iface_remove_v6_prefix(): "
                    "Deleting Address",0,0,0);
      (void) ps_iface_delete_priv_ipv6_addr(this_iface_ptr, v6_addr_ptr);
    }
  }

  ps_ifacei_invoke_event_cbacks(this_iface_ptr,
                                NULL,
                                IFACE_PREFIX_UPDATE_EV,
                                event_info);
  IP6_REPORT_PFX_UPDATE( this_iface_ptr, event_info.prefix_info );

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  LOG_MSG_INFO1("ps_iface_remove_v6_prefix(): Success",0,0,0);
  return 0;

} /* ps_iface_remove_v6_prefix() */


/*===========================================================================
FUNCTION PS_IFACE_DELETE_ALL_V6_PREFIXES()

DESCRIPTION
  This function will remove all prefixes associated with the interface.

PARAMETERS
  this_iface_ptr:  ptr to interface control block on which to operate on.

RETURN VALUE
  0 on success
 -1 on failure

DEPENDENCIES
  Currently only ONE prefix is supported on an interface.

SIDE EFFECTS
  Deletes all V6 prefixes.
===========================================================================*/
int ps_iface_delete_all_v6_prefixes
(
  ps_iface_type *this_iface_ptr
)
{
  ps_iface_event_info_u_type event_info;
  ps_ifacei_v6_addr_type     *v6_addr_ptr;
  uint8                       addr_index;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_INFO1("ps_iface_delete_all_v6_prefixes()",0,0,0);
  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    LOG_MSG_INFO1("Invalid iface_ptr to ps_iface_delete_all_v6_prefixes",0,0,0);
    ASSERT(0);
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    If iface has inherit IP info true then get the base iface of the logical
    chain, else do nothing.
  -------------------------------------------------------------------------*/
  this_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

  if(this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX] != NULL &&
     this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX]->prefix != 0)
  {
    event_info.prefix_info.prefix.ps_s6_addr64[0] =
      this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX]->prefix;
    event_info.prefix_info.kind       = PREFIX_REMOVED;
    event_info.prefix_info.prefix_len =
      this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX]->prefix_len;

    /*-----------------------------------------------------------------------
      Clear the public address, but don't free it. Only free the timers.
    -----------------------------------------------------------------------*/
    v6_addr_ptr = this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX];

    if( v6_addr_ptr != NULL )
    {
      PS_TIMER_FREE_HANDLE(v6_addr_ptr->pref_lifetimer_handle);
      PS_TIMER_FREE_HANDLE(v6_addr_ptr->valid_lifetimer_handle);

      memset(v6_addr_ptr,
             0,
             sizeof(ps_ifacei_v6_addr_type));

      /*---------------------------------------------------------------------
        Free any additional privacy addresses if they exist. Always delete 
        priv addrs at index 1. Since the array is condensed as part of 
        address deletion if there are other addresses they will be moved to 
        that index.
      ---------------------------------------------------------------------*/
      for( addr_index = MAX_IPV6_PREFIXES; 
           addr_index < MAX_IPV6_ADDRS; 
           addr_index++ )
      {
        v6_addr_ptr = this_iface_ptr->iface_private.ipv6_addrs[MAX_IPV6_PREFIXES];

        if(v6_addr_ptr == NULL)
        {
          break;
        }

        LOG_MSG_INFO1("ps_iface_delete_all_v6_prefixes(): "
                      "Deleting address.",0,0,0);
        (void) ps_iface_delete_priv_ipv6_addr(this_iface_ptr, v6_addr_ptr);
      }
    }

    ps_ifacei_invoke_event_cbacks(this_iface_ptr,
                                  NULL,
                                  IFACE_PREFIX_UPDATE_EV,
                                  event_info);
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  LOG_MSG_INFO1("ps_iface_delete_all_v6_prefixes(): Success",0,0,0);
  return 0;

} /* ps_iface_delete_all_v6_prefixes() */


/*===========================================================================
FUNCTION PS_IFACE_SET_V6_DNS_ADDRS

DESCRIPTION
  This function sets the primary and secondary DNS addr's on the
  IPV6 iface.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  prim_dns_ptr:  input v6 primary dns address
  sec_dns_ptr:  input v6 secondary dns address

RETURN VALUE
  0 if successful
 -1 if fails

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_set_v6_dns_addrs
(
  ps_iface_type         *this_iface_ptr,
  ps_ip_addr_type       *prim_dns_ptr,
  ps_ip_addr_type       *sec_dns_ptr
)
{
  ps_iface_type    *base_iface_ptr;  /* iface with ip addr stored locally  */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (prim_dns_ptr == NULL)
  {
    LOG_MSG_ERROR("NULL parameter is passed, prim_dns_ptr, 0x%p ",
              prim_dns_ptr, 0, 0);
    ASSERT(0);
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  if (ps_iface_get_addr_family(this_iface_ptr) != IPV6_ADDR)
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_INFO1("Cannot set v6 DNS address on a non v6 interface",0,0,0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    If iface has inherit IP info true then get the base iface of the logical
    chain, else do nothing.
  -------------------------------------------------------------------------*/
  base_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

  /*-------------------------------------------------------------------------
    if PS_IFACE_GET_BASE_IFACE returned iface_ptr with inhertied flag set,
    then it indicates error.
  -------------------------------------------------------------------------*/
  if (PS_IFACEI_IP_INFO_IS_INHERITED(base_iface_ptr))
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Invalid associated iface 0x%p", base_iface_ptr, 0, 0);
    ASSERT(0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    Ensure that the addr family is consistent through the logica/assoc
    iface chain.
  -------------------------------------------------------------------------*/
  ASSERT(ps_iface_get_addr_family(base_iface_ptr) == IPV6_ADDR);

  memcpy(&base_iface_ptr->v6_net_info.primary_dns,
         &prim_dns_ptr->addr.v6, sizeof(struct ps_in6_addr));
  if(sec_dns_ptr != NULL)
  {
    memcpy(&base_iface_ptr->v6_net_info.secondary_dns,
           &sec_dns_ptr->addr.v6, sizeof(struct ps_in6_addr));
  }
  else
  {
    memset(&base_iface_ptr->v6_net_info.secondary_dns,
           0, sizeof(struct ps_in6_addr));
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;

} /* ps_iface_set_v6_dns_addrs() */


/*===========================================================================
FUNCTION PS_IFACE_GET_V6_DNS_ADDRS

DESCRIPTION
  This function returns the primary and secondary DNS addr's on the
  IPV6 iface.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  prim_dns_ptr:  storage for primary dns address
  sec_dns_ptr:  storage for secondary dns address

RETURN VALUE
  None.  However, if the addr family is not IPV6, then the input
  parameters are stored with zero.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_get_v6_dns_addrs
(
  ps_iface_type         *this_iface_ptr,
  ip_addr_type          *prim_dns_ptr,
  ip_addr_type          *sec_dns_ptr
)
{
  ps_iface_type    *base_iface_ptr;  /* iface with ip addr stored locally  */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);
//MSR add validation
  if (ps_iface_get_addr_family(this_iface_ptr) != IPV6_ADDR)
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);

    prim_dns_ptr->addr.v6[0] = 0;
    prim_dns_ptr->addr.v6[1] = 0;

    sec_dns_ptr->addr.v6[0]  = 0;
    sec_dns_ptr->addr.v6[1]  = 0;
    prim_dns_ptr->type       = IP_ADDR_INVALID;
    sec_dns_ptr->type        = IP_ADDR_INVALID;
    return;
  }

  /*-------------------------------------------------------------------------
    If iface has inherit IP info true then get the base iface of the logical
    chain, else do nothing.
  -------------------------------------------------------------------------*/
  base_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

  /*-------------------------------------------------------------------------
    if PS_IFACE_GET_BASE_IFACE returned iface_ptr with inhertied flag set,
    then it indicates error.
  -------------------------------------------------------------------------*/
  if (PS_IFACEI_IP_INFO_IS_INHERITED(base_iface_ptr))
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Invalid associated iface 0x%p", base_iface_ptr, 0, 0);
    ASSERT(0);
    return;
  }

  /*-------------------------------------------------------------------------
    Ensure that the addr family is consistent through the logica/assoc
    iface chain.
  -------------------------------------------------------------------------*/
  ASSERT(ps_iface_get_addr_family(base_iface_ptr) == IPV6_ADDR);

  prim_dns_ptr->addr.v6[0] =
                        base_iface_ptr->v6_net_info.primary_dns.ps_s6_addr64[0];
  prim_dns_ptr->addr.v6[1] =
                        base_iface_ptr->v6_net_info.primary_dns.ps_s6_addr64[1];

  sec_dns_ptr->addr.v6[0] =
                      base_iface_ptr->v6_net_info.secondary_dns.ps_s6_addr64[0];
  sec_dns_ptr->addr.v6[1] =
                      base_iface_ptr->v6_net_info.secondary_dns.ps_s6_addr64[1];

  /*-------------------------------------------------------------------------
    Set appropriate ip address types.
  -------------------------------------------------------------------------*/
  if( (prim_dns_ptr->addr.v6[0] == 0) &&
      (prim_dns_ptr->addr.v6[1] == 0))
  {
    prim_dns_ptr->type = IP_ADDR_INVALID;
  }
  else
  {
    prim_dns_ptr->type = IPV6_ADDR;
  }

  if( (sec_dns_ptr->addr.v6[0] == 0) &&
      (sec_dns_ptr->addr.v6[1] == 0))
  {
    sec_dns_ptr->type  = IP_ADDR_INVALID;
  }
  else
  {
    sec_dns_ptr->type  = IPV6_ADDR;
  }
  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

} /* ps_iface_get_v6_dns_addrs() */


/*===========================================================================
FUNCTION PS_IFACE_GENERATE_IPV6_IID()

DESCRIPTION
  This function generates a random IPv6 IID, ensures that the IID generated
  is unique on the interface, and begins DAD (if necessary).

PARAMETERS
  *this_iface_ptr - Pointer to the interface to operate on.
  *iid            - Pointer to the IID to be returned by this function.
  *ps_errno       - Pointer to the error number to be returned.

RETURN VALUE
  0 success
 -1 failure

DEPENDENCIES

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_generate_ipv6_iid
(
  ps_iface_type *this_iface_ptr, /* Pointer to the interface to operate on */
  uint64        *iid,            /* Pointer to interface ID to be returned */
  int16         *ps_errno
)
{
  uint64  rand_num;
  boolean is_iid_unique = FALSE;
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  if( !PS_IFACE_IS_VALID(this_iface_ptr) )
  {
    LOG_MSG_ERROR("Invalid parameters are passed, iface 0x%p",
              this_iface_ptr, 0, 0);
    ASSERT(0);
    return -1;
  }

  if( (iid == NULL) ||
      (ps_errno == NULL) )
  {
    LOG_MSG_ERROR("IID %p or errno %p are NULL", iid, ps_errno, 0);
    ASSERT(0);
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    If iface has inherit IP info true then get the base iface of the logical
    chain, else do nothing.
  -------------------------------------------------------------------------*/
  this_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

  /*-------------------------------------------------------------------------
    Generate a new private address and verify that the new IID is unique.
  -------------------------------------------------------------------------*/
  while(is_iid_unique == FALSE)
  {
    rand_num = 0;
    ps_utils_generate_rand_64bit_num(&rand_num);

  /*-------------------------------------------------------------------------
    Reset universal/local bit (7th most significant bit) for random iid.
  -------------------------------------------------------------------------*/
    rand_num &= ps_htonll(0xfdffffffffffffffULL);

    if(PS_IFACE_IPV6_IS_PRIV_IID_UNIQUE(this_iface_ptr, &rand_num))
    {
      is_iid_unique = TRUE;
    }
  }/* end while */

  *iid = rand_num;

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;

} /* ps_iface_generate_ipv6_iid */

/*===========================================================================
FUNCTION PS_IFACE_CHECK_IPV6_IID_CONFLICT()

DESCRIPTION
  This function checks for a conflict of passed IID with any of the
  associated iid's of an iface.

PARAMETERS
  *this_iface_ptr - Pointer to the interface to operate on.
   iid            - IID to check for conflict.
   exclude_primary_iid - if set dont include primary IID
                         while checking for conflict

RETURN VALUE
  -1 in case of any error.
   1 in case of conflict
   0 otherwise.

DEPENDENCIES

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_check_ipv6_iid_conflict
(
  ps_iface_type *this_iface_ptr, /* Pointer to the interface to operate on */
  uint64         iid,            /* IID to check for conflict */
  boolean        exclude_primary_iid
)
{
  int i;
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  if( !PS_IFACE_IS_VALID(this_iface_ptr) )
  {
    LOG_MSG_ERROR("Invalid parameters are passed in "
              "ps_iface_check_ipv6_iid_conflict, iface 0x%p",
              this_iface_ptr, 0, 0);
    ASSERT(0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    Conflict check is only allowed on iface that has IP information
    stored not inherited.
  -------------------------------------------------------------------------*/
  if ( PS_IFACEI_IP_INFO_IS_INHERITED(this_iface_ptr) )
  {
    LOG_MSG_ERROR("ps_iface_check_ipv6_iid_conflict: "
              "Unexptected iface type 0x%x:%p",
              this_iface_ptr->name, this_iface_ptr->instance, 0);
    ASSERT(0);
    return -1;
  }

  for( i = 0; i < MAX_IPV6_ADDRS; i++ )
  {
    if( TRUE == exclude_primary_iid && DEFAULT_V6_INDEX == i)
    {
      continue;
    }

    if(this_iface_ptr->iface_private.ipv6_addrs[i] == NULL)
    {
      break;
    }

    if( iid == this_iface_ptr->iface_private.ipv6_addrs[i]->iid )
    {
      return 1;
    }

  }

  return 0;
} /* ps_iface_check_ipv6_iid_conflict */


/*===========================================================================
FUNCTION PS_IFACE_TRANSFER_IPV6_IID()

DESCRIPTION
  This function transfers the IPv6 IID from source iface to destination iface.
  It also takes care to checking if the IID is in conflict with any
  other IID on destination iface. Also it inherits the prefix &
  default gateway information from destination IFACE's primary IID.

PARAMETERS
  *src_iface_ptr - Pointer to the interface from which IID will be copied.
  *dst_iface_ptr - Pointer to the interface to which IID will be copied.
   iid           - IID that need to be transfered.
  *ps_errno      - Error Reason when failed.


RETURN VALUE
  -1 in case of any error.
   0 in case of sucess

DEPENDENCIES
  This function should only be called when the primary IID is populated
  on destination iface.

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_transfer_ipv6_iid
(
  ps_iface_type *src_iface_ptr, /* Pointer to the interface from which
                                   ipv6 iid will be copied. */
  ps_iface_type *dst_iface_ptr, /* Pointer to the interface to where
                                   ipv6 iid will get copied */
  uint64         iid,            /* The IID that will be transfered */
  int16         *ps_errno
)
{
  ps_ifacei_v6_addr_type * v6_src_addr_ptr  = NULL;
  ps_ifacei_v6_addr_type * v6_dst_addr_ptr  = NULL;
  int32                    status = 0;
  int64                    timer_remaining_time;
  boolean                  addr_found = FALSE;
  uint8                    i, index;
  boolean                  alloc_pref_lifetimer = FALSE;
  boolean                  alloc_valid_lifetimer = FALSE;
  boolean                  alloc_unused_addr_timer = FALSE;
  struct ps_in6_addr       ipv6_addr;
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  if ( ps_errno == NULL )
  {
    LOG_MSG_ERROR("errno %p is NULL", ps_errno, 0, 0);
    ASSERT(0);
    return -1;
  }


  if ( !PS_IFACE_IS_VALID(src_iface_ptr) || !PS_IFACE_IS_VALID(dst_iface_ptr) )
  {
    LOG_MSG_ERROR("Invalid parameters are passed in "
              "ps_iface_transfer_ipv6_iid, src_iface 0x%p, "
              "dst_iface 0x%p", src_iface_ptr, dst_iface_ptr, 0);
    ASSERT(0);
    *ps_errno = DS_EFAULT;
    return -1;
  }

  /*-----------------------------------------------------------------------
    Source iface should be physical iface & ip family should be ipv6
  -----------------------------------------------------------------------*/
  if ( PS_IFACEI_IP_INFO_IS_INHERITED(src_iface_ptr) ||
      FALSE == ps_iface_addr_family_is_v6(src_iface_ptr) )
  {
    LOG_MSG_ERROR("Unexptected source iface type or ip family 0x%x:%p",
              src_iface_ptr->name, src_iface_ptr->instance, 0);
    *ps_errno = DS_EAFNOSUPPORT;
    return -1;
  }

  /*-----------------------------------------------------------------------
    Destination iface should be physical iface & ip family should be ipv6
  -----------------------------------------------------------------------*/
  if ( PS_IFACEI_IP_INFO_IS_INHERITED(dst_iface_ptr) ||
      FALSE == ps_iface_addr_family_is_v6(dst_iface_ptr) )
  {
    LOG_MSG_ERROR("Unexptected destination iface type or ip family 0x%x:%p",
              dst_iface_ptr->name, dst_iface_ptr->instance, 0);
    *ps_errno = DS_EAFNOSUPPORT;
    return -1;
  }

  /*-------------------------------------------------------------------------
     Destination IFACE should have primary IPv6 Address with prefix &
     gateway IID.
  -------------------------------------------------------------------------*/
  if ( dst_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX] == NULL )
  {
    LOG_MSG_ERROR("No primary IPv6 address found on destination iface.",
              0, 0, 0);
    *ps_errno = DS_EAFNOSUPPORT;
    return -1;
  }


  do
  {
    PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

    /*-------------------------------------------------------------------------
      Find the IID on source IFACE.
    -------------------------------------------------------------------------*/
    for( i = 0; i < MAX_IPV6_ADDRS; i++ )
    {
      if ( src_iface_ptr->iface_private.ipv6_addrs[i] == NULL )
      {
        break;
      }

      if ( iid == src_iface_ptr->iface_private.ipv6_addrs[i]->iid )
      {
        /*---------------------------------------------------------------------
           Primary address cannot be transfered.
        ---------------------------------------------------------------------*/
        if ( DEFAULT_V6_INDEX == i )
        {
          LOG_MSG_ERROR( "Transfer of primary IPv6 address is not allowed "
                     "IID %lu, source iface 0x%p destination iface 0x %p",
                     iid, dst_iface_ptr->name, dst_iface_ptr->instance );
          *ps_errno = DS_EOPNOTSUPP;
          status = -1;
          break;
        }

        v6_src_addr_ptr = src_iface_ptr->iface_private.ipv6_addrs[i];

        ipv6_addr.in6_u.u6_addr64[0] = v6_src_addr_ptr->prefix;
        ipv6_addr.in6_u.u6_addr64[1] = v6_src_addr_ptr->iid;

        LOG_MSG_INFO1("ps_iface_transfer_ipv6_iid(): "
                      "Transfering following address from source "
                      "iface to target iface.",0,0,0);
        IPV6_ADDR_MSG(ipv6_addr.ps_s6_addr64);

        addr_found = TRUE;
        break;
      }
    } /* for loop */

    /*-------------------------------------------------------------------------
     failure status: go to error handling
    -------------------------------------------------------------------------*/
    if ( status == -1 )
    {
      break;
    }

    /*-------------------------------------------------------------------------
       Address not found on source IFACE.
    -------------------------------------------------------------------------*/
    if ( addr_found == FALSE )
    {
      *ps_errno = DS_ENOSRCADDR;
      break;
    }

    /*-------------------------------------------------------------------------
      Check for conflict of IID on destination iface.
      The probability of this happening is very low, in most cases
      the conflict should not occur.
    -------------------------------------------------------------------------*/
    if ( ps_iface_check_ipv6_iid_conflict(dst_iface_ptr,
                                          v6_src_addr_ptr->iid,
                                          FALSE) )
    {
      LOG_MSG_ERROR("IID %ul conflicts with any of the associated IIDs of an iface.",
                 v6_src_addr_ptr->iid, 0, 0);
      *ps_errno = DS_EADDRINUSE;
      break;
    }

    /*---------------------------------------------------------------------
     alloc mem for new v6 addr
    ---------------------------------------------------------------------*/
    if ( PS_IFACE_IPV6_IS_ADDR_SPACE_AVAIL(dst_iface_ptr, &index) )
    {
      dst_iface_ptr->iface_private.ipv6_addrs[index] =
        (ps_ifacei_v6_addr_type *) ps_mem_get_buf(PS_MEM_IPV6_ADDR_TYPE);

      v6_dst_addr_ptr = dst_iface_ptr->iface_private.ipv6_addrs[index];
      if(NULL == v6_dst_addr_ptr)
      {
        LOG_MSG_INFO2("Out of PS MEM IPv6 address items!",0,0,0);
        *ps_errno = DS_ENOMEM;
        break;
      }

      /*---------------------------------------------------------------------
        Copy the all fields over, inherti prefix & gateway information
        from target iface, reset timers to default.
      ---------------------------------------------------------------------*/
      *v6_dst_addr_ptr = *v6_src_addr_ptr;

      v6_dst_addr_ptr->gateway_iid =
        dst_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX]->gateway_iid;

      v6_dst_addr_ptr->prefix =
        dst_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX]->prefix;

      v6_dst_addr_ptr->prefix_len =
        dst_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX]->prefix_len;

      v6_dst_addr_ptr->pref_lifetimer_handle    = PS_TIMER_INVALID_HANDLE;
      v6_dst_addr_ptr->valid_lifetimer_handle   = PS_TIMER_INVALID_HANDLE;
      v6_dst_addr_ptr->unused_addr_timer_handle = PS_TIMER_INVALID_HANDLE;

      /*---------------------------------------------------------------------
       Transfer timers from source address to destination address, and start
       the timer using the remaining time of the source address.
      ---------------------------------------------------------------------*/

      if (PS_TIMER_INVALID_HANDLE !=
                             v6_src_addr_ptr->pref_lifetimer_handle)
      {
        alloc_pref_lifetimer = TRUE;
        /*---------------------------------------------------------------------
         Allocate pref life timer on destination address.
        ---------------------------------------------------------------------*/
         v6_dst_addr_ptr->pref_lifetimer_handle =
                             ps_timer_alloc( ps_ifacei_ipv6_iid_timer_handler,
                                             v6_dst_addr_ptr);
      }

      if (PS_TIMER_INVALID_HANDLE !=
                             v6_src_addr_ptr->valid_lifetimer_handle)
      {
        alloc_valid_lifetimer = TRUE;
        /*---------------------------------------------------------------------
         Allocate valid life timer on destination address.
        ---------------------------------------------------------------------*/
         v6_dst_addr_ptr->valid_lifetimer_handle =
                             ps_timer_alloc( ps_ifacei_ipv6_iid_timer_handler,
                                             v6_dst_addr_ptr);
      }

      if (PS_TIMER_INVALID_HANDLE !=
                             v6_src_addr_ptr->unused_addr_timer_handle)
      {
        alloc_unused_addr_timer = TRUE;
        /*---------------------------------------------------------------------
         Allocate unused iid timer on destination address.
        ---------------------------------------------------------------------*/
         v6_dst_addr_ptr->unused_addr_timer_handle =
                             ps_timer_alloc( ps_ifacei_ipv6_unused_iid_handler,
                                             v6_dst_addr_ptr);
      }

      /*---------------------------------------------------------------------
       Check if the timers allocated successfully on destination address.
      ---------------------------------------------------------------------*/
      if(
         (TRUE == alloc_pref_lifetimer && PS_TIMER_INVALID_HANDLE ==
                                  v6_dst_addr_ptr->pref_lifetimer_handle)  ||
         (TRUE == alloc_valid_lifetimer && PS_TIMER_INVALID_HANDLE ==
                                  v6_dst_addr_ptr->valid_lifetimer_handle) ||
         (TRUE == alloc_unused_addr_timer && PS_TIMER_INVALID_HANDLE ==
                                  v6_dst_addr_ptr->unused_addr_timer_handle)
        )
      {
        LOG_MSG_ERROR("Error can't allocate timers for IPv6  addr!",0,0,0);
        PS_TIMER_FREE_HANDLE(v6_dst_addr_ptr->pref_lifetimer_handle);
        PS_TIMER_FREE_HANDLE(v6_dst_addr_ptr->valid_lifetimer_handle);
        PS_TIMER_FREE_HANDLE(v6_dst_addr_ptr->unused_addr_timer_handle);
        PS_MEM_FREE(dst_iface_ptr->iface_private.ipv6_addrs[index]);
        *ps_errno = DS_ENOMEM;
        break;
      }

      if (TRUE == alloc_pref_lifetimer)
      {
        /*---------------------------------------------------------------------
         start pref life timer on destination address
        ---------------------------------------------------------------------*/
        timer_remaining_time = ps_timer_remaining (
                               v6_src_addr_ptr->pref_lifetimer_handle);
        if(timer_remaining_time > 0)
        {
          (void) ps_timer_start(v6_dst_addr_ptr->pref_lifetimer_handle,
                                timer_remaining_time);
        }
      }

      if (TRUE == alloc_valid_lifetimer)
      {
        /*---------------------------------------------------------------------
         start valid life timer on destination address
        ---------------------------------------------------------------------*/
        timer_remaining_time = ps_timer_remaining (
                               v6_src_addr_ptr->valid_lifetimer_handle);
        if(timer_remaining_time > 0)
        {
          (void) ps_timer_start(v6_dst_addr_ptr->valid_lifetimer_handle,
                                timer_remaining_time);
        }
      }

      if (TRUE == alloc_unused_addr_timer)
      {
        /*---------------------------------------------------------------------
         start unused iid timer on destination address
        ---------------------------------------------------------------------*/
        timer_remaining_time = ps_timer_remaining (
                             v6_src_addr_ptr->unused_addr_timer_handle);
        if(timer_remaining_time > 0)
        {
          (void) ps_timer_start(v6_dst_addr_ptr->unused_addr_timer_handle,
                                timer_remaining_time);
        }
      }
    }
    else
    {
      LOG_MSG_INFO2("MAX IPv6 addresses already allocated for iface %p!",
               dst_iface_ptr, 0, 0);
      *ps_errno = DS_ENOMEM;
      break;
    }

    PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return 0;

  } while(0); /* loop ends */

  LOG_MSG_ERROR("ps_iface_transfer_ipv6_iid: Error returning -1", 0, 0, 0);
  /*-------------------------------------------------------------------------
   Common Error handling.
  -------------------------------------------------------------------------*/
  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return -1;

} /* ps_iface_transfer_ipv6_iid */

#ifdef FEATURE_DATA_PS_MIPV6
/*===========================================================================
FUNCTION PS_IFACE_SET_MIP6_BOOTSTRAP_CFG_INFO

DESCRIPTION
  This function sets the MIP6 bootstrap config info on an iface. This function
  overwrites the previously stored info, if any.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  mip6_cfg_info : ptr to mip6 bootstrap config info
  ps_errno      : return error code.

RETURN VALUE
  0 if successful
 -1 if fails

DEPENDENCIES
  None

SIDE EFFECTS
  ps_errno is set if function fails
===========================================================================*/
int ps_iface_set_mip6_bootstrap_cfg_info
(
  ps_iface_type                         *this_iface_ptr,
  ps_iface_mip6_bootstrap_cfg_info_type *mip6_cfg_info,
  errno_enum_type                       *ps_errno
)
{
  ps_iface_type                 *base_iface_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if ( ps_errno== NULL )
  {
    LOG_MSG_ERROR("NULL parameters are passed, ps_errno, 0x%p",
              ps_errno, 0, 0);
    ASSERT(0);
    return -1;
  }

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface, 0x%p, is passed", this_iface_ptr, 0, 0);
    *ps_errno = E_BAD_ADDRESS;
    ASSERT(0);
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    If iface has inherit IP info true then get the base iface of the logical
    chain, else do nothing.
  -------------------------------------------------------------------------*/
  base_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

  /*-------------------------------------------------------------------------
    if PS_IFACE_GET_BASE_IFACE returned iface_ptr with inhertied flag set,
    then it indicates error.
  -------------------------------------------------------------------------*/
  if (PS_IFACEI_IP_INFO_IS_INHERITED(base_iface_ptr))
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Invalid associated iface 0x%p", base_iface_ptr, 0, 0);
    ASSERT(0);
    *ps_errno = E_BAD_ADDRESS;
    return -1;
  }

  if(NULL == mip6_cfg_info)
  { /* This is a reset */
    if(base_iface_ptr->mip6_bootstrap_info)
    {
      PS_MEM_FREE(base_iface_ptr->mip6_bootstrap_info);
    }
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return 0;
  }

  if(NULL == base_iface_ptr->mip6_bootstrap_info)
  {
    base_iface_ptr->mip6_bootstrap_info =
      (ps_iface_mip6_bootstrap_cfg_info_type *)
        ps_mem_get_buf(PS_MEM_PS_MIP6_BOOTSTRAP_CFG_INFO_TYPE);

    if(NULL == base_iface_ptr->mip6_bootstrap_info)
    {
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      LOG_MSG_ERROR("Couldn't create a buffer for MIP6 bootstrap info", 0, 0, 0);
      *ps_errno = E_NO_MEMORY;
      return -1;
    }
  }

  /*-------------------------------------------------------------------------
    Copy the contents of MIP6 info in the passed ptr to the base_iface_ptr
  -------------------------------------------------------------------------*/
  memcpy(base_iface_ptr->mip6_bootstrap_info, mip6_cfg_info,
         sizeof(ps_iface_mip6_bootstrap_cfg_info_type));

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;
}/* ps_iface_set_mip6_bootstrap_cfg_info() */

/*===========================================================================
FUNCTION PS_IFACE_GET_MIP6_BOOSTRAP_CFG_INFO

DESCRIPTION
  This function gets the MIP6 bootstrap config info stored in he iface.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  mip6_cfg_info : ptr to mip6 bootstrap config info
  ps_errno      : return error code.

RETURN VALUE
  0 if successful
 -1 if fails

DEPENDENCIES
  None

SIDE EFFECTS
  ps_errno is set if function fails
===========================================================================*/
int ps_iface_get_mip6_bootstrap_cfg_info
(
  ps_iface_type                         *this_iface_ptr,
  ps_iface_mip6_bootstrap_cfg_info_type *mip6_cfg_info,
  errno_enum_type                       *ps_errno
)
{
  ps_iface_type                 *base_iface_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if ( ps_errno== NULL || mip6_cfg_info == NULL )
  {
    LOG_MSG_ERROR("NULL parameters are passed, ps_errno, 0x%p, "
              "mip6_cfg_info, 0x%p",
              ps_errno, mip6_cfg_info, 0);
    ASSERT(0);
    return -1;
  }

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface, 0x%p, is passed", this_iface_ptr, 0, 0);
    *ps_errno = E_BAD_ADDRESS;
    ASSERT(0);
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    If iface has inherit IP info true then get the base iface of the logical
    chain, else do nothing.
  -------------------------------------------------------------------------*/
  base_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

  /*-------------------------------------------------------------------------
    if PS_IFACE_GET_BASE_IFACE returned iface_ptr with inhertied flag set,
    then it indicates error.
  -------------------------------------------------------------------------*/
  if (PS_IFACEI_IP_INFO_IS_INHERITED(base_iface_ptr))
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Invalid associated iface 0x%p", base_iface_ptr, 0, 0);
    *ps_errno = E_BAD_ADDRESS;
    ASSERT(0);
    return -1;
  }

  if(base_iface_ptr->mip6_bootstrap_info == NULL)
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("MIP6 bootstrap info not set 0x%p",
              base_iface_ptr->mip6_bootstrap_info, 0, 0);
    *ps_errno = E_NO_DATA;
    return -1;
  }

  /*-------------------------------------------------------------------------
    Copy the contents of MIP6 info in the base_iface_ptr to the passed ptr
  -------------------------------------------------------------------------*/
  memcpy(mip6_cfg_info, base_iface_ptr->mip6_bootstrap_info,
         sizeof(ps_iface_mip6_bootstrap_cfg_info_type));

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;
}/* ps_iface_get_mip6_bootstrap_cfg_info() */
#endif /* FEATURE_DATA_PS_MIPV6 */

#endif /* FEATURE_DATA_PS && FEATURE_DATA_PS_IPV6 */
#endif /* ifndef FEATURE_DATACOMMON_THIRD_PARTY_APPS */
