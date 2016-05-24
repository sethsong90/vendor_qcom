/*===========================================================================
  FILE: ps_iface_handoff.c

  OVERVIEW: TODO

  DEPENDENCIES: None

  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/netiface/rel/11.03/src/ps_iface_handoff.c#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-11-15 asn corrected termination of old srat
  2010-06-02 jy  added utility function ps_iface_handoff_transfer_sec_ipv6_addr
  2009-10-23 msr Created file

===========================================================================*/

/*===========================================================================

                                INCLUDE FILES

===========================================================================*/
#include "customer.h"
#include "comdef.h"

#ifdef FEATURE_DATA_PS
#include "amssassert.h"

#include "dserrno.h"
#include "ds_Utils_DebugMsg.h"
#include "ds_flow_control.h"

#include "ps_iface.h"
#include "ps_ifacei_event.h"
#include "ps_crit_sect.h"
#include "ps_route.h"
#include "ps_iface_logicali.h"
#include "ps_iface_logical.h"
#include "ps_stat_iface.h"

#ifdef FEATURE_DATA_PS_IPV6
#include "ps_iface_addr_v6.h"
#include "ps_ifacei_addr_v6.h"
#include "ps_mem.h"
#endif /* FEATURE_DATA_PS_IPV6 */

#include "ds_Utils_DebugMsg.h"

/*===========================================================================

                              EXTERNAL FUNCTIONS

===========================================================================*/
int32 ps_iface_handoff_initiate
(
  ps_iface_type         * ps_iface_ptr,
  acl_policy_info_type  * acl_policy_ptr,
  int16                 * ps_errno_ptr
)
{
  ps_iface_type  * assoc_iface_ptr;
  ps_iface_type  * temp_iface_ptr;
  ps_iface_type  * trat_iface_ptr;
  int32            ret_val;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Iface 0x%p", ps_iface_ptr, 0, 0);

  if (NULL == ps_errno_ptr)
  {
    LOG_MSG_INVALID_INPUT( "NULL errno", 0, 0, 0);
    return -1;
  }

  do
  {
    PS_ENTER_CRIT_SECTION( &global_ps_crit_section);

    /*-----------------------------------------------------------------------
      Make sure that iface is valid and is a logical iface
    -----------------------------------------------------------------------*/
    if (!PS_IFACE_IS_VALID( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Invalid iface 0x%p", ps_iface_ptr, 0, 0);
      *ps_errno_ptr = DS_EINVAL;
      break;
    }

    if (!PS_IFACE_IS_LOGICAL( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%d not logical",
                             ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      *ps_errno_ptr = DS_EINVAL;
      break;
    }

    /*-----------------------------------------------------------------------
      Make sure that iface is associated to another iface
    -----------------------------------------------------------------------*/
    assoc_iface_ptr = PS_IFACE_GET_ASSOC_IFACE( ps_iface_ptr);
    if (NULL == assoc_iface_ptr)
    {
      LOG_MSG_ERROR( "Iface 0x%x:%p not associated",
                     ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      *ps_errno_ptr = DS_EINVAL;
      break;
    }

    ps_iface_disable_flow( ps_iface_ptr, DS_FLOW_NO_SERVICE_MASK);

    if (-1 == route_get( NULL, acl_policy_ptr, FALSE, NULL, &temp_iface_ptr))
    {
      *ps_errno_ptr = DS_ENOROUTE;
      break;
    }

    /*-----------------------------------------------------------------------
      Check if routing resulted in a TRAT.

      Note that it is not valid to check if route_get() returned the same iface
      on which handoff is initiated, as route_get() builds a chain of ifaces
      and always returns the iface on top of the chain as the routing result,
      but handoff can be initiated anywhere in the chain.

      If ps_iface_handoff_initiate is called for the second time, when
      handoff is in progress, do nothing.
    -----------------------------------------------------------------------*/
    trat_iface_ptr = PS_IFACE_GET_TRAT_IFACE( ps_iface_ptr);
    if (NULL == trat_iface_ptr)
    {
      *ps_errno_ptr = DS_ENOROUTE;
      break;
    }
    else if (PS_IFACE_GET_ASSOC_IFACE( ps_iface_ptr) == trat_iface_ptr)
    {
      ps_iface_enable_flow( ps_iface_ptr, DS_FLOW_NO_SERVICE_MASK);
      PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
      return 0;
    }

    /*-----------------------------------------------------------------------
      Bring up TRAT
    -----------------------------------------------------------------------*/
    ret_val = ps_iface_bring_up_cmd( trat_iface_ptr, ps_errno_ptr, NULL);
    if (-1 == ret_val && DS_EWOULDBLOCK != *ps_errno_ptr)
    {
      LOG_MSG_INFO3( "ps_iface_bring_up_cmd failed on iface 0x%x:%d err %d",
                     trat_iface_ptr->name,
                     trat_iface_ptr->instance,
                     *ps_errno_ptr);
      break;
    }

    LOG_MSG_FUNCTION_EXIT( "Success, iface 0x%x:%d",
                           ps_iface_ptr->name, ps_iface_ptr->instance, 0);

    PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
    return 0;

  } while (0);

  /*-------------------------------------------------------------------------
    Common error handling
  -------------------------------------------------------------------------*/

  /*-------------------------------------------------------------------------
    Enable data path on the iface so that apps are not blocked for a handoff
    which had already failed
  -------------------------------------------------------------------------*/
  ps_iface_enable_flow( ps_iface_ptr, DS_FLOW_NO_SERVICE_MASK);

  LOG_MSG_FUNCTION_EXIT( "Fail, iface 0x%x:%d",
                         ps_iface_ptr->name, ps_iface_ptr->instance, 0);

  PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
  return -1;

} /* ps_iface_handoff_initiate() */


int32 ps_iface_handoff_swap_rat
(
  ps_iface_type  * ps_iface_ptr,
  int16          * ps_errno_ptr
)
{
  ps_iface_type              * assoc_iface_ptr;
  ps_phys_link_type          * ps_phys_link_ptr;
  ps_ip_addr_type              old_ip_addr;
  ps_ip_addr_type              new_ip_addr;
  ps_iface_event_info_u_type   event_info;
  int32                        assoc_iface_app_priority;
  ps_iface_event_enum_type     phys_link_event_to_post;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Iface 0x%p", ps_iface_ptr, 0, 0);

  do
  {
    PS_ENTER_CRIT_SECTION( &global_ps_crit_section);

    /*-----------------------------------------------------------------------
      Make sure that iface is valid and is a logical iface
    -----------------------------------------------------------------------*/
    if (!PS_IFACE_IS_VALID( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Invalid iface 0x%p", ps_iface_ptr, 0, 0);
      break;
    }

    if (!PS_IFACE_IS_LOGICAL( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%d not logical",
                             ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      break;
    }

    /*-----------------------------------------------------------------------
      Make sure that iface is associated to another iface
    -----------------------------------------------------------------------*/
    assoc_iface_ptr = PS_IFACE_GET_ASSOC_IFACE( ps_iface_ptr);
    if (NULL == assoc_iface_ptr)
    {
      LOG_MSG_ERROR( "Iface 0x%x:%p not associated",
                     ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      break;
    }

    /*-----------------------------------------------------------------------
      Update Rx packet and byte statistics. This is needed if logical iface is
      bound to LTE iface and if A2 briding is enabled. Otherwise, Rx packet
      and byte statistics will remain 0 leading to incorrect statistics after
      the handoff is complete
    -----------------------------------------------------------------------*/
    ps_stat_get_iface( PS_STAT_IFACE_PKTS_RX,
                       ps_iface_ptr,
                       &( ps_iface_ptr->iface_i_stats.pkts_rx),
                       sizeof( ps_iface_ptr->iface_i_stats.pkts_rx));

    ps_stat_get_iface( PS_STAT_IFACE_BYTES_RX,
                       ps_iface_ptr,
                       &( ps_iface_ptr->iface_i_stats.bytes_rx),
                       sizeof( ps_iface_ptr->iface_i_stats.bytes_rx));

    /*-----------------------------------------------------------------------
      Fetch the IP address before the handoff
    -----------------------------------------------------------------------*/
    memset( &old_ip_addr, 0, sizeof( ip_addr_type));
    old_ip_addr.type = IP_ANY_ADDR;
    ps_iface_get_addr( ps_iface_ptr, &old_ip_addr);

    /*-----------------------------------------------------------------------
      Swap RATs
    -----------------------------------------------------------------------*/
    (void) ps_iface_logical_swap_rat( ps_iface_ptr, ps_errno_ptr);

    /*-----------------------------------------------------------------------
      Fetch the IP address after the handoff
    -----------------------------------------------------------------------*/
    memset( &new_ip_addr, 0, sizeof( ip_addr_type));
    new_ip_addr.type = IP_ANY_ADDR;
    ps_iface_get_addr( ps_iface_ptr, &new_ip_addr);

    /*-----------------------------------------------------------------------
      If the IP address changed after handoff, post IFACE_ADDR_CHANGED_EV
    -----------------------------------------------------------------------*/
    if (memcmp( &old_ip_addr, &new_ip_addr, sizeof( ip_addr_type)) != 0)
    {
      event_info.ip_addr = old_ip_addr;
      ps_ifacei_invoke_event_cbacks( ps_iface_ptr,
                                     NULL,
                                     IFACE_ADDR_CHANGED_EV,
                                     event_info);
    }

    /*-----------------------------------------------------------------------
      Post phys link events on new SRAT
    -----------------------------------------------------------------------*/
    ps_phys_link_ptr     = PS_IFACE_GET_PHYS_LINK( ps_iface_ptr);
    event_info.flow_mask = ps_phys_link_ptr->phys_private.tx_flow_mask;

    phys_link_event_to_post = PS_PHYS_LINKI_FLOW_ENABLED( ps_phys_link_ptr)
                                ? PHYS_LINK_FLOW_ENABLED_EV
                                : PHYS_LINK_FLOW_DISABLED_EV;
    ps_ifacei_invoke_event_cbacks( NULL,
                                   ps_phys_link_ptr,
                                   phys_link_event_to_post,
                                   event_info);

    /*-----------------------------------------------------------------------
      Tear down the old SRAT
    -----------------------------------------------------------------------*/
    assoc_iface_app_priority = ps_iface_get_app_priority( assoc_iface_ptr);

    (void) ps_iface_tear_down_cmd_ex( assoc_iface_ptr,
                                      assoc_iface_app_priority,
                                      ps_errno_ptr,
                                      NULL);

    ps_iface_enable_flow( ps_iface_ptr, DS_FLOW_NO_SERVICE_MASK);

    LOG_MSG_FUNCTION_EXIT( "Success, iface 0x%x:%d",
                           ps_iface_ptr->name, ps_iface_ptr->instance, 0);

    PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
    return 0;

  } while (0);

  /*-------------------------------------------------------------------------
    Common error handling
  -------------------------------------------------------------------------*/
  LOG_MSG_FUNCTION_EXIT( "Fail, iface 0x%x:%d",
                         ps_iface_ptr->name, ps_iface_ptr->instance, 0);

  PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
  return -1;

} /* ps_iface_handoff_swap_rat() */


int32 ps_iface_handoff_failure
(
  ps_iface_type  * ps_iface_ptr,
  int16          * ps_errno_ptr
)
{
  ps_iface_type  * trat_iface_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Iface 0x%p", ps_iface_ptr, 0, 0);

  do
  {
    PS_ENTER_CRIT_SECTION( &global_ps_crit_section);

    /*-----------------------------------------------------------------------
      Make sure that iface is valid and is a logical iface
    -----------------------------------------------------------------------*/
    if (!PS_IFACE_IS_VALID( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Invalid iface 0x%p", ps_iface_ptr, 0, 0);
      *ps_errno_ptr = DS_EINVAL;
      break;
    }

    if (!PS_IFACE_IS_LOGICAL( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%d not logical",
                             ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      *ps_errno_ptr = DS_EINVAL;
      break;
    }

    /*-----------------------------------------------------------------------
      Make sure that iface is associated to a TRAT
    -----------------------------------------------------------------------*/
    trat_iface_ptr = PS_IFACE_GET_TRAT_IFACE(ps_iface_ptr);
    if (NULL == trat_iface_ptr)
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%d has no TRAT",
                             ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      *ps_errno_ptr = DS_EINVAL;
      break;
    }

    /*-----------------------------------------------------------------------
      Make sure that TRAT iface is in DOWN/COMING_DOWN state as it is not
      valid to call this function in any other state
    -----------------------------------------------------------------------*/
    if (IFACE_DOWN != PS_IFACEI_GET_STATE( trat_iface_ptr) &&
        IFACE_GOING_DOWN != PS_IFACEI_GET_STATE( trat_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%p in invalid state 0x%p",
                             trat_iface_ptr->name,
                             trat_iface_ptr->instance,
                             PS_IFACEI_GET_STATE( trat_iface_ptr));
      *ps_errno_ptr = DS_EINVAL;
      break;
    }

    /*-----------------------------------------------------------------------
      Set the TRAT to NULL and enable data path on the iface
    -----------------------------------------------------------------------*/
    PS_IFACE_SET_TRAT_IFACE( ps_iface_ptr, NULL);
    ps_iface_enable_flow( ps_iface_ptr, DS_FLOW_NO_SERVICE_MASK);

    LOG_MSG_FUNCTION_EXIT( "Success, iface 0x%x:%d",
                           ps_iface_ptr->name, ps_iface_ptr->instance, 0);

    PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
    return 0;

  } while (0);

  LOG_MSG_FUNCTION_EXIT( "Fail, iface 0x%x:%d",
                         ps_iface_ptr->name, ps_iface_ptr->instance, 0);

  PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
  return -1;

} /* ps_iface_handoff_failure() */

#ifdef FEATURE_DATA_PS_IPV6
/*===========================================================================
FUNCTION PS_IFACE_HANDOFF_TRANSFER_SEC_IPV6_ADDR()

DESCRIPTION
  Transfer the secondary (cached) ip addresses from srat iface to trat iface
  of an EPC iface.

  The functionality is to serve EPC handoff.

PARAMETERS
  ps_iface_ptr : pointer to EPC iface that manages handoff

RETURN VALUE
  -1: failure
   0: sucess

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int32 ps_iface_handoff_transfer_sec_ipv6_addr
(
  ps_iface_type  * ps_iface_ptr,
  int16          * ps_errno_ptr
)
{
  ps_iface_type          * assoc_iface_ptr;
  ps_iface_type          * trat_iface_ptr;
  ps_ifacei_v6_addr_type * v6_src_addr_ptr  = NULL;
  ps_ifacei_v6_addr_type * v6_dst_addr_ptr  = NULL;
  uint8                    i;
  int32                    status = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "IPv6 addr cache transfer Iface 0x%p",
                          ps_iface_ptr, 0, 0);

  do
  {
    PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

    /*-----------------------------------------------------------------------
      Make sure that iface is valid and is a logical iface
    -----------------------------------------------------------------------*/
    if (!PS_IFACE_IS_VALID( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Invalid iface 0x%p", ps_iface_ptr, 0, 0);
      break;
    }

    if (!PS_IFACE_IS_LOGICAL( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%d not logical",
                             ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      break;
    }

    if (NULL == ps_errno_ptr)
    {
      LOG_MSG_INVALID_INPUT( "Invalid ps_errno_ptr", 0, 0, 0);
      break;
    }

    /*-----------------------------------------------------------------------
      Make sure that iface is associated to another iface
    -----------------------------------------------------------------------*/
    assoc_iface_ptr = PS_IFACE_GET_ASSOC_IFACE( ps_iface_ptr);
    if (!PS_IFACE_IS_VALID( assoc_iface_ptr))
    {
      LOG_MSG_ERROR( "Iface 0x%x:%p not associated",
                     ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      break;
    }

    /*-----------------------------------------------------------------------
      srat ip family should be ipv6
    -----------------------------------------------------------------------*/
    if( FALSE == ps_iface_addr_family_is_v6(assoc_iface_ptr) )
    {
      LOG_MSG_ERROR("Unexptected srat iface ip family v4 0x%x:%p",
                assoc_iface_ptr->name, assoc_iface_ptr->instance, 0);
      break;
    }

    /*-----------------------------------------------------------------------
      Make sure that trat already exists
    -----------------------------------------------------------------------*/
    trat_iface_ptr = PS_IFACE_GET_TRAT_IFACE( ps_iface_ptr);
    if (!PS_IFACE_IS_VALID( trat_iface_ptr))
    {
      LOG_MSG_ERROR( "Iface 0x%x:%p does not have trat",
                     ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      break;
    }

    /*-----------------------------------------------------------------------
      trat ip family should be ipv6
    -----------------------------------------------------------------------*/
    if( FALSE == ps_iface_addr_family_is_v6(trat_iface_ptr) )
    {
      LOG_MSG_ERROR("Unexptected srat iface ip family v4 0x%x:%p",
                trat_iface_ptr->name, trat_iface_ptr->instance, 0);
      break;
    }

    /*-----------------------------------------------------------------------
      trat only retains the primary ip, all secondary ip should be from srat.
      remove all secondary ipv6 addr in trat
    -----------------------------------------------------------------------*/
    for(i=0; i<MAX_IPV6_ADDRS; i++)
    {
      /*---------------------------------------------------------------------
       skip primary ipv6 addr - not to be touched
      ---------------------------------------------------------------------*/
      if( DEFAULT_V6_INDEX == i )
      {
        continue;
      }

      v6_dst_addr_ptr = trat_iface_ptr->iface_private.ipv6_addrs[i];
      /*---------------------------------------------------------------------
       no more ipv6 addr in trat iface
      ---------------------------------------------------------------------*/
      if(NULL == v6_dst_addr_ptr)
      {
        break;
      }

      /*---------------------------------------------------------------------
       delete secondary ipv6 ip addr
        - TBD: to revisit for handoff optimization
      ---------------------------------------------------------------------*/
      if( (status = ps_iface_delete_priv_ipv6_addr(trat_iface_ptr,
                                                   v6_dst_addr_ptr)) == -1)
      {
        break;
      }
    } /* for loop */

    /*-------------------------------------------------------------------------
     failure status: go to error handling
    -------------------------------------------------------------------------*/
    if (status == -1)
    {
      LOG_MSG_ERROR("trat iface  0x%x:%p ipv6 seconday addr delete failure",
                trat_iface_ptr->name, trat_iface_ptr->instance, 0);
      break;
    }

    /*-----------------------------------------------------------------------
      transfer secondary ip6 addr from srat to trat
    -----------------------------------------------------------------------*/
    for(i=0; i<MAX_IPV6_ADDRS; i++)
    {
      /*---------------------------------------------------------------------
       skip primary ipv6 addr - not to be touched
      ---------------------------------------------------------------------*/
      if( DEFAULT_V6_INDEX == i )
      {
        continue;
      }

      v6_src_addr_ptr = assoc_iface_ptr->iface_private.ipv6_addrs[i];

      /*---------------------------------------------------------------------
       no more ipv6 addr to be transfered
      ---------------------------------------------------------------------*/
      if( NULL == v6_src_addr_ptr )
      {
        status = 0;
        break;
      }

      /*---------------------------------------------------------------------
       transfer the current secodnary iid from source iface
      ---------------------------------------------------------------------*/
      if( (status = ps_iface_transfer_ipv6_iid(assoc_iface_ptr,
                                               trat_iface_ptr,
                                               v6_src_addr_ptr->iid,
                                               ps_errno_ptr)) == -1)
      {
        break;
      }

    } /* for loop */

    /*-------------------------------------------------------------------------
     failure status: go to error handling
    -------------------------------------------------------------------------*/
    if (status == -1)
    {
      LOG_MSG_ERROR("srat iface  0x%x:%p ipv6 seconday iid 0x%x transfer failure",
                 assoc_iface_ptr->name,
                 assoc_iface_ptr->instance,
                 v6_src_addr_ptr->iid);
      break;
    }

    LOG_MSG_FUNCTION_EXIT( "Success: IPv6 addr cache transfer, iface 0x%x:%d",
                           ps_iface_ptr->name, ps_iface_ptr->instance, 0);
    PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return 0;

  } while (0); /* do */

  /*-------------------------------------------------------------------------
    Common error handling
  -------------------------------------------------------------------------*/
  LOG_MSG_FUNCTION_EXIT( "Fail: IPv6 addr cache transfer, iface 0x%x:%d",
                         ps_iface_ptr->name, ps_iface_ptr->instance, 0);

  PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
  return -1;

} /* ps_iface_handoff_transfer_sec_ipv6_addr */
#endif /* FEATURE_DATA_PS_IPV6 */


#endif  /* FEATURE_DATA_PS */
