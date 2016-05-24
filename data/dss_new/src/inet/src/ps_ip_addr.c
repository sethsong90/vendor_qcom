/*===========================================================================

                           P S _ I P _ A D D R . C

DESCRIPTION
This file contains IP functions that are common to both IPv4 and IPv6.

Copyright (c) 1995-2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  $Header: //source/qcom/qct/modem/datamodem/protocols/inet/rel/11.03/src/ps_ip_addr.c#1 $ $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
03/21/11    op     Data SU API cleanup
03/26/09    pp     CMI De-featurization.
05/07/07    mct    IPv6 Neighbor Discovery
12/06/06    mct    Updated conditions under which to delete priv shared addrs
                   Also priv unique addrs are now application unique.
09/08/06    mct    Created file.

===========================================================================*/


/*===========================================================================

                                INCLUDE FILES

===========================================================================*/
#include "customer.h"       /* Customer Specific Features */
#include "comdef.h"
#ifdef FEATURE_DATA_PS

#include "ps_iface.h"
#include "ps_in.h"
#include "ps_route.h"
#include "ps_utils.h"
#include "ps_crit_sect.h"
#include "ds_Utils_DebugMsg.h"

#ifdef FEATURE_DATA_PS_IPV6
#include "ps_ip6_sm.h"
#include "ps_iface_addr_v6.h"
#include "ps_ifacei_addr_v6.h"
#endif /* FEATURE_DATA_PS_IPV6 */


/*===========================================================================

                        PUBLIC DATA DECLARATIONS

===========================================================================*/

/*===========================================================================

                           INTERNAL FUNCTIONS

===========================================================================*/

/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/

#ifdef FEATURE_DATA_PS_IPV6


/*===========================================================================
FUNCTION PS_IP_ADDR_IPV6_PRIV_ADDR_INC_REF_CNT()

DESCRIPTION
  This function increments the ref cnt of the IPv6 private address. For
  private unique addresses this ensures that the reference count is currently
  0. If it is greater than 0 it implies another socket is currently bound to
  the address and will fail the subsequent bind. This starts the deprecated
  lifetime timer if the ref_cnt goes from 0 to 1.

PARAMETERS
  ip_addr_ptr:   Pointer to the ip address

RETURN VALUE
  0 success
 -1 failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_ip_addr_ipv6_priv_addr_inc_ref_cnt
(
  struct ps_in6_addr *ip_addr_ptr
)
{
  ps_ifacei_v6_addr_type  *v6_addr_ptr = NULL;
  ps_iface_type           *ps_iface_ptr = NULL;
  int64                   pref_lifetime = 0;
  int64                   lifetime = 0;
  priv_ext_lifetimes_type *priv_ext_lifetimes_ptr = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if(ip_addr_ptr == NULL)
  {
    LOG_MSG_ERROR("NULL IPv6 address ptr %p", ip_addr_ptr, 0, 0);
    ASSERT(0);
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  if(ps_iface_find_ipv6_addr(ip_addr_ptr, &v6_addr_ptr, &ps_iface_ptr))
  {
    if (NULL == ps_iface_ptr)
    {
      LOG_MSG_ERROR ("Cant find iface for IP addr", 0, 0, 0);
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      return -1;
    }

    if(v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_PUBLIC &&
       v6_addr_ptr->addr_state == IPV6_ADDR_STATE_VALID)
    {
      /*---------------------------------------------------------------------
        Nothing to be done for public addresses.
      ---------------------------------------------------------------------*/
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      return 0;
    }
    else if(v6_addr_ptr->addr_state != IPV6_ADDR_STATE_VALID)
    {
      /*---------------------------------------------------------------------
         Address must be in valid state, don't allow binds to deprecated 
         addresses.
      ---------------------------------------------------------------------*/
      LOG_MSG_INFO1("Can't bind to an IPv6 address in state %d!",
               v6_addr_ptr->addr_state,0,0);
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      return -1;
    }
    /*-----------------------------------------------------------------------
      Address has been located. Only allow reference counts on privacy 
      addresses.
    -----------------------------------------------------------------------*/ 
    if( (v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_PRIV_UNIQUE) ||
        (v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_PRIV_SHARED))
    {
      /*-------------------------------------------------------------------
        Increment the reference count of the address and start the
        preferred lifetime timer. If it's already been started do nothing.
      -------------------------------------------------------------------*/
      if(ps_timer_is_running(v6_addr_ptr->unused_addr_timer_handle))
      {
        /*-----------------------------------------------------------------
          Cancel the unused activity timer if it's running.
        -----------------------------------------------------------------*/
        (void)ps_timer_cancel(v6_addr_ptr->unused_addr_timer_handle);
      }
 
      v6_addr_ptr->ref_cnt++;
      if(!ps_timer_is_running(v6_addr_ptr->pref_lifetimer_handle))
      {
        pref_lifetime = ps_timer_remaining(
          ps_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX]->
            pref_lifetimer_handle);
        priv_ext_lifetimes_ptr = ip6_sm_get_priv_ext_lifetimes();
        lifetime = MIN(pref_lifetime,
            (((int64) priv_ext_lifetimes_ptr->pref_lifetime_timer) * 1000) - 5);

        (void)ps_timer_start(v6_addr_ptr->pref_lifetimer_handle, lifetime);
      }
    }
    else
    {
      LOG_MSG_ERROR("BIND FAIL: Incorrect address type %x or state %x",
                v6_addr_ptr->addr_type, v6_addr_ptr->addr_state, 0);
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      return -1;

    }
  }
  else
  {  
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return -1;
  }
  
  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;
}


/*===========================================================================
FUNCTION PS_IP_ADDR_IPV6_PRIV_ADDR_DEC_REF_CNT()

DESCRIPTION
  This function decrements the ref count of a private IPv6 address. If the 
  reference count goes to 0 the privacy address is deleted.

PARAMETERS
  ip_addr_ptr:   Pointer to the ip address

RETURN VALUE
  0 success
 -1 failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_ip_addr_ipv6_priv_addr_dec_ref_cnt
(
  struct ps_in6_addr *ip_addr_ptr
)
{
  ps_ifacei_v6_addr_type  *v6_addr_ptr = NULL;
  ps_iface_type           *ps_iface_ptr = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if(ip_addr_ptr == NULL)
  {
    LOG_MSG_ERROR("NULL IPv6 address ptr %p", ip_addr_ptr, 0, 0);
    ASSERT(0);
    return -1;
  }


  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  if(ps_iface_find_ipv6_addr(ip_addr_ptr, &v6_addr_ptr, &ps_iface_ptr))
  {
    /*-----------------------------------------------------------------------
      Address has been located. Only allow reference counts on privacy
      addresses. If it is a bind to a public IPv6 address, do nothing.
    -----------------------------------------------------------------------*/
    if(v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_PUBLIC)
    {
      /*---------------------------------------------------------------------
        Nothing to be done for public addresses.
      ---------------------------------------------------------------------*/
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      return 0;
    }
    
    if( ((v6_addr_ptr->addr_state == IPV6_ADDR_STATE_VALID) ||
         (v6_addr_ptr->addr_state == IPV6_ADDR_STATE_DEPRECATED)) && 
        ((v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_PRIV_UNIQUE) ||
        (v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_PRIV_SHARED)) )
    {
      if(v6_addr_ptr->ref_cnt > 0)
      {
        v6_addr_ptr->ref_cnt--;
      }
      else
      {
        LOG_MSG_ERROR("Trying to unbind to an addr that should already be freed.",
                  0,0,0);
        PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
        return -1;
      }
    
      if(v6_addr_ptr->ref_cnt == 0)
      {
        /*---------------------------------------------------------------------
          Delete the private IPv6 address. Always delete unique addresses if
          ref_cnt is zero. Only delete shared addresses if both ref_cnt is 0
          and state is deprecated, as multiple apps could have requested the
          addr, but not binded yet.
        ---------------------------------------------------------------------*/
        if( (v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_PRIV_SHARED &&
             v6_addr_ptr->addr_state == IPV6_ADDR_STATE_DEPRECATED) ||
            (v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_PRIV_UNIQUE))
        {
          (void)ps_iface_delete_priv_ipv6_addr(ps_iface_ptr, v6_addr_ptr);
        }
      }
    }
  }  
  else
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return -1;
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;
}
#endif /* FEATURE_DATA_PS_IPV6 */
#endif /* FEATURE_DATA_PS */
