/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                      P S _ I F A C E _ A D D R _ M G M T . C

GENERAL DESCRIPTION
 Interface IP Address Management Layer

EXTERNALIZED FUNCTIONS

INITIALIZATION AND SEQUENCING REQUIREMENTS

Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/


/*===========================================================================

                           EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datamodem/interface/netiface/rel/11.03/src/ps_iface_addr_mgmt.c#1 $
  $Author: zhasan $ $DateTime: 2011/06/17 12:02:33 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
12/14/10    ash    Added cleanup routine for External addresses
08/27/10    ash    Added more F3 messages.
03/09/10    ash    Drill Down to base iface of logical iface chain for ipv6 
                   addrresses.
10/12/09    ssh    Reporting prefix updates to diag
09/23/09    ss     KW warnings fixed.
09/09/09    ss     Critical section released in error cases.
07/13/09    ssh    get_addr_info: Examine handle before invoking timer API.
                   Start unused addr timer for external addrs when made VALID.
06/15/09    ssh    Use Um iface prefix if prefix to be added is linklocal
06/15/09    ssh    Integration fixes
05/21/09    ar     Added IPV6_ADDR_TYPE_INVALID handling on post-DAD update
02/27/09    ar     Added dad_retries to the get/set addr_info
01/09/09    ar     Added iface_ptr to address structure
08/18/08    vp     Featurized addr_mgmt support
06/23/08    mct    Initial Version
===========================================================================*/


/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"       /* Customer Specific Features */
#if defined (FEATURE_DATA_PS) && defined (FEATURE_DATA_PS_IPV6)
#ifdef FEATURE_DATA_PS_ADDR_MGMT
#include "dsm.h"
#include "msg.h"
#include "err.h"
#include "amssassert.h"
#include "ps_mem.h"
#include "ps_ifacei_event.h"
#include "ps_iface.h"
#include "ps_ifacei.h"
#include "ps_handle_mgr.h"
#include "ps_iface_addr_v6.h"
#include "ps_ifacei_addr_v6.h"
#include "ps_iface_addr_mgmt.h"
#include "ps_icmp6_nd.h"
#include "ps_crit_sect.h"
#include "ps_ip6_events.h"
#include "ds_Utils_DebugMsg.h"


/*===========================================================================

                           LOCAL DATA DECLARATIONS

===========================================================================*/


/*===========================================================================

                              INTERNAL FUNCTIONS

===========================================================================*/
ps_ifacei_v6_addr_type * ps_iface_addr_mgmti_get_addr_from_handle
(
  ps_iface_addr_mgmt_handle_type  *  handle_ptr
);

void ps_iface_addr_mgmti_update_ipv6_addr_state
(
  ps_iface_type                      * iface_ptr,
  ps_iface_ipv6_addr_state_enum_type   new_addr_state,
  ps_ifacei_v6_addr_type             * v6_addr_ptr
);

int ps_iface_addr_mgmti_set_addr_info
(
  ps_iface_type                     * iface_ptr,
  ps_iface_addr_mgmt_handle_type    * handle_ptr,
  ps_iface_addr_mgmt_addr_info_type * addr_info_ptr,
  int16                             * ps_errno
);


/*===========================================================================
FUNCTION PS_IFACE_ADDR_MGMTI_GET_ADDR_FROM_HANDLE()

DESCRIPTION
  This function retrieves the IPv6 address structure for the given handle.

PARAMETERS
  handle_ptr:   The handle from which to derive the IP address structure
  v6_addr_ptr:  The address buffer to derive.

RETURN VALUE
  IPv6 address structure if successful
  NULL                   if failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
ps_ifacei_v6_addr_type * ps_iface_addr_mgmti_get_addr_from_handle
(
  ps_iface_addr_mgmt_handle_type  *  handle_ptr
)
{
  ps_ifacei_v6_addr_type        * v6_addr_ptr;
  int16                           index;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if(handle_ptr == NULL)
  {
    LOG_MSG_ERROR("NULL handle_ptr: %p passed!", handle_ptr, 0, 0);
    return NULL;
  }

  if(PS_HANDLE_MGR_INVALID_INDEX == (index =
     ps_handle_mgr_get_index(PS_HANDLE_MGR_CLIENT_IPV6_ADDR, *handle_ptr)))
  {
    LOG_MSG_ERROR("Invalid address handle: %d!", *handle_ptr, 0,0);
    return NULL;
  }

  if(NULL == (v6_addr_ptr = ps_mem_index_to_buf(index, PS_MEM_IPV6_ADDR_TYPE)))
  {
    LOG_MSG_ERROR("Invalid address index: %d!", index, 0, 0);
    return NULL;
  }

  return v6_addr_ptr;
} /* ps_iface_addr_mgmti_get_addr_from_handle() */

/*===========================================================================
FUNCTION PS_IFACE_ADDR_MGMT_GET_HANDLE_FROM_ADDR()

DESCRIPTION
  This function retrieves the handle given the IPv6 address structure.

PARAMETERS
  v6_addr_ptr:  The ptr to the address structure.
  handle_ptr:   The handle from which to derive the IP address structure

RETURN VALUE
  handle if successful
  -1     if failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
ps_iface_addr_mgmt_handle_type ps_iface_addr_mgmt_get_handle_from_addr
(
  ps_ifacei_v6_addr_type        * v6_addr_ptr
)
{
  int16 index;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if(v6_addr_ptr == NULL)
  {
    LOG_MSG_ERROR("NULL v6_addr_ptr: %p passed!", v6_addr_ptr, 0, 0);
    return -1;
  }

  if(-1 == (index = (int16) ps_mem_buf_to_index((void *) v6_addr_ptr)))
  {
    LOG_MSG_ERROR("Invalid address buffer 0x%p!", v6_addr_ptr, 0, 0);
    return -1;
  }

  return (ps_iface_addr_mgmt_handle_type)
         ps_handle_mgr_get_handle(PS_HANDLE_MGR_CLIENT_IPV6_ADDR, index);
} /* ps_iface_addr_mgmti_get_handle_from_addr()*/

/*===========================================================================
FUNCTION PS_IFACE_ADDR_MGMTI_UPDATE_IPV6_ADDR_STATE()

DESCRIPTION
  This internal function is used to set all address information in the
  IP address structure. Anything not allowed to be set externally is filtered
  out in the externalized function.

PARAMETERS
  iface_ptr:      Interface on which the address exists.
  new_addr_state: The new state of the address
  v6_addr_ptr:    The address information structure to update.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_addr_mgmti_update_ipv6_addr_state
(
  ps_iface_type                      * iface_ptr,
  ps_iface_ipv6_addr_state_enum_type   new_addr_state,
  ps_ifacei_v6_addr_type             * v6_addr_ptr
)
{
  ps_iface_event_enum_type     event;
  ps_iface_event_info_u_type   event_info;
  boolean                      invoke_cback = FALSE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if( ( !PS_IFACE_IS_VALID( iface_ptr ) ) || ( v6_addr_ptr == NULL ) )
  {
    LOG_MSG_ERROR("Invalid args in ps_iface_addr_mgmti_update_ipv6_addr_state!",
              0,0,0);
    ASSERT(0);
    return;
  }
  
  memset(&event_info, 0, sizeof(event_info));
  memset(&event, 0, sizeof(event));
  /*-------------------------------------------------------------------------
    Old state and new state are not equal. Update state and generate events
    accordingly.
  -------------------------------------------------------------------------*/
  switch(new_addr_state)
  {
    /*-----------------------------------------------------------------------
      If address changing to TENTATIVE or UNASSIGNED, do nothing.
    -----------------------------------------------------------------------*/
    case IPV6_ADDR_STATE_TENTATIVE:
    case IPV6_ADDR_STATE_UNASSIGNED:
      break;

    /*-----------------------------------------------------------------------
      If address is VALID
    -----------------------------------------------------------------------*/
    case IPV6_ADDR_STATE_VALID:
      switch(v6_addr_ptr->addr_type)
      {
        case IPV6_ADDR_TYPE_PUBLIC:
          invoke_cback = TRUE;
          event        = IFACE_PREFIX_UPDATE_EV;
          event_info.prefix_info.kind = PREFIX_ADDED;
          break;

        case IPV6_ADDR_TYPE_PRIV_SHARED:
        case IPV6_ADDR_TYPE_PRIV_UNIQUE:
          invoke_cback = TRUE;
          event        = IFACE_IPV6_PRIV_ADDR_GENERATED_EV;
          break;

        case IPV6_ADDR_TYPE_EXTERNAL:
          break;

        case IPV6_ADDR_TYPE_INVALID:
          /*-----------------------------------------------------------------
            This case occurs on address buffer preallocation which is not
            triggered by application.  In this case, we set the state to
            UNASSIGNED so that buffer is available for application to use.
          -----------------------------------------------------------------*/
          invoke_cback = TRUE;
          event        = IFACE_IPV6_PRIV_ADDR_GENERATED_EV;
          new_addr_state = IPV6_ADDR_STATE_UNASSIGNED;
          break;

	default:
          ASSERT(0);
          break;
      }
      break;

    /*-----------------------------------------------------------------------
      If address is DEPRECATED
    -----------------------------------------------------------------------*/
    case IPV6_ADDR_STATE_DEPRECATED:
      switch(v6_addr_ptr->addr_type)
      {
        case IPV6_ADDR_TYPE_PUBLIC:
          invoke_cback = TRUE;
          event        = IFACE_PREFIX_UPDATE_EV;
          event_info.prefix_info.kind = PREFIX_DEPRECATED;
          break;

        case IPV6_ADDR_TYPE_PRIV_SHARED:
        case IPV6_ADDR_TYPE_PRIV_UNIQUE:
          invoke_cback = TRUE;
          event        = IFACE_IPV6_PRIV_ADDR_DEPRECATED_EV;
          break;

        case IPV6_ADDR_TYPE_EXTERNAL:
          break;

        case IPV6_ADDR_TYPE_INVALID:
        default:
          ASSERT(0);
          break;
      }
      break;

    /*-----------------------------------------------------------------------
      If address is anything else
    -----------------------------------------------------------------------*/
    case IPV6_ADDR_STATE_INVALID:
    default:
      ASSERT(0);
      break;

  }
  v6_addr_ptr->addr_state = new_addr_state;

  if(invoke_cback && v6_addr_ptr->addr_type != IPV6_ADDR_TYPE_EXTERNAL)
  {
    if(v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_PRIV_SHARED ||
       v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_PRIV_UNIQUE)
    {
      event_info.priv_ipv6_addr.is_unique =
        (v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_PRIV_UNIQUE) ? TRUE : FALSE;
      event_info.priv_ipv6_addr.ip_addr.type              = IPV6_ADDR;
      event_info.priv_ipv6_addr.ip_addr.addr.v6.ps_s6_addr64[0] = 
        v6_addr_ptr->prefix;
      event_info.priv_ipv6_addr.ip_addr.addr.v6.ps_s6_addr64[1] = 
        v6_addr_ptr->iid;
    }
    else
    {
      event_info.prefix_info.prefix.ps_s6_addr64[0] = v6_addr_ptr->prefix;
      event_info.prefix_info.prefix_len          = v6_addr_ptr->prefix_len;
    }

    ps_ifacei_invoke_event_cbacks(iface_ptr,
                                  NULL,
                                  event,
                                  event_info);
    
    if(event == IFACE_PREFIX_UPDATE_EV)
    {
      IP6_REPORT_PFX_UPDATE( iface_ptr, event_info.prefix_info ); 
    }
  }
} /* ps_iface_addr_mgmti_update_ipv6_addr_state() */

/*===========================================================================
FUNCTION PS_IFACE_ADDR_MGMTI_SET_ADDR_INFO()

DESCRIPTION
  This internal function is used to set all address information in the
  IP address structure. Anything not allowed to be set externally is filtered
  out in the externalized function.

PARAMETERS
  iface_ptr:     Interface on which the address exists.
  handle_ptr:    The handle to the IP address structure to update.
  addr_info_ptr: The address information structure from which to populate.
  ps_errno:      The error code to return in case of a problem.

RETURN VALUE
  0 success
 -1 failure

DEPENDENCIES
  None

SIDE EFFECTS
  Copies the IP address iformation from the client's IP address information
  structure to the IP address structure.
===========================================================================*/
int ps_iface_addr_mgmti_set_addr_info
(
  ps_iface_type                     * iface_ptr,
  ps_iface_addr_mgmt_handle_type    * handle_ptr,
  ps_iface_addr_mgmt_addr_info_type * addr_info_ptr,
  int16                             * ps_errno
)
{
  ps_iface_event_info_u_type    event_info;
  ps_ifacei_v6_addr_type      * v6_addr_ptr = NULL;
  boolean                       lifetimes_updated = FALSE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if(handle_ptr == NULL || addr_info_ptr == NULL || ps_errno == NULL)
  {
    LOG_MSG_INFO1("NULL handle_ptr: %p, addr_info_ptr: %p, or ps_errno: %p",
             handle_ptr, addr_info_ptr, ps_errno);
    ASSERT(0);
    return -1;
  }
  if( !PS_IFACE_IS_VALID( iface_ptr ) )
  {
    LOG_MSG_ERROR("Invalid arg ",0,0,0);
    ASSERT(0);
    return -1;
  }

  v6_addr_ptr = ps_iface_addr_mgmti_get_addr_from_handle(handle_ptr);

  if(v6_addr_ptr == NULL)
  {
    LOG_MSG_ERROR("Error retrieving v6 address structure!",0,0,0);
    ASSERT(0);
    return -1;
  }

  memset(&event_info, 0, sizeof(event_info));

  /*---------------------------------------------------------------------------
    Set prefix in address structure. Generate update, added, or deleted events
    as necessary.
  ---------------------------------------------------------------------------*/
  if(addr_info_ptr->addr_mask & PS_IFACE_ADDR_MGMT_MASK_PREFIX)
  {
    if(!(addr_info_ptr->addr_mask & PS_IFACE_ADDR_MGMT_MASK_PREFIX_LEN))
    {
      LOG_MSG_ERROR("Prefix length must always be included with the prefix!",0,0,0);
      ASSERT(0);
      return -1;
    }


    if((v6_addr_ptr->prefix != 0 && addr_info_ptr->prefix != 0) ||
       (v6_addr_ptr->prefix == addr_info_ptr->prefix))
    {
      LOG_MSG_ERROR("Either both prefixes are the same or both are non-zero!",
                0,0,0);
      return -1;
    }

    //todo handoff?

    /*-------------------------------------------------------------------------
      If the new prefix is 0, the old prefix is being deleted.  If the old
      prefix was zero, this is a new prefix.
    -------------------------------------------------------------------------*/
    if(v6_addr_ptr->prefix != 0)
    {
      return ps_iface_addr_mgmt_free_addr(iface_ptr, handle_ptr, NULL, ps_errno);
    }

    v6_addr_ptr->prefix     = addr_info_ptr->prefix;
    v6_addr_ptr->prefix_len = addr_info_ptr->prefix_len;

    if(addr_info_ptr->prefix != 0)
    {
      event_info.prefix_info.kind                = PREFIX_ADDED;
      event_info.prefix_info.prefix.ps_s6_addr64[0] = addr_info_ptr->prefix;
      event_info.prefix_info.prefix_len          = addr_info_ptr->prefix_len;

      ps_ifacei_invoke_event_cbacks(iface_ptr,
                                    NULL,
                                    IFACE_PREFIX_UPDATE_EV,
                                    event_info);
      IP6_REPORT_PFX_UPDATE( iface_ptr, event_info.prefix_info );          
    }
  }

  /*---------------------------------------------------------------------------
    Set address' interface identifier (IID).
  ---------------------------------------------------------------------------*/
  if(addr_info_ptr->addr_mask & PS_IFACE_ADDR_MGMT_MASK_IID)
  {
    if(v6_addr_ptr->iid != 0 && v6_addr_ptr->iid != addr_info_ptr->iid)
    {
      if(v6_addr_ptr->addr_type != IPV6_ADDR_TYPE_PUBLIC)
      {
        LOG_MSG_ERROR("Private or external address IIDs cannot be modified!",0,0,0);
        ASSERT(0);
        return -1;
      }
    }
    v6_addr_ptr->iid = addr_info_ptr->iid;
  }

  /*---------------------------------------------------------------------------
    Set the gateway IID.
  ---------------------------------------------------------------------------*/
  if(addr_info_ptr->addr_mask & PS_IFACE_ADDR_MGMT_MASK_GATEWAY_IID)
  {
    v6_addr_ptr->gateway_iid = addr_info_ptr->gateway_iid;
  }


  /*---------------------------------------------------------------------------
    To be used in future.. Set the preferred and valid lifetimes for the
    address. For now privacy extensions are handled through prefix apply.
  ---------------------------------------------------------------------------*/
  if(addr_info_ptr->addr_mask & PS_IFACE_ADDR_MGMT_MASK_PREF_LIFETIME)
  {
    if(v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_PUBLIC)
    {
      (void) ps_timer_start(v6_addr_ptr->pref_lifetimer_handle,
                            addr_info_ptr->pref_lifetime);
      lifetimes_updated = TRUE;
    }
  }

  if(addr_info_ptr->addr_mask & PS_IFACE_ADDR_MGMT_MASK_VALID_LIFETIME)
  {
    if(v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_PUBLIC)
    {
      (void) ps_timer_start(v6_addr_ptr->valid_lifetimer_handle,
                            addr_info_ptr->valid_lifetime);
      lifetimes_updated = TRUE;
    }
  }

  if((~(addr_info_ptr->addr_mask) & PS_IFACE_ADDR_MGMT_MASK_PREFIX) &&
     lifetimes_updated)
  {
    event_info.prefix_info.kind                = PREFIX_UPDATED;
    event_info.prefix_info.prefix.ps_s6_addr64[0] = v6_addr_ptr->prefix;
    event_info.prefix_info.prefix_len          = v6_addr_ptr->prefix_len;

    ps_ifacei_invoke_event_cbacks(iface_ptr,
                                  NULL,
                                  IFACE_PREFIX_UPDATE_EV,
                                  event_info);
    IP6_REPORT_PFX_UPDATE( iface_ptr, event_info.prefix_info );          
  }

  /*---------------------------------------------------------------------------
    Set the address type before address state. This is important since events,
    etc are generated according to the corresponding address type.
  ---------------------------------------------------------------------------*/
  if(addr_info_ptr->addr_mask & PS_IFACE_ADDR_MGMT_MASK_ADDR_TYPE)
  {
    if(v6_addr_ptr->addr_type != IPV6_ADDR_TYPE_INVALID)
    {
      LOG_MSG_ERROR("Attempting to set an address type that is already set",0,0,0);
      ASSERT(0);
      return -1;
    }

    v6_addr_ptr->addr_type = addr_info_ptr->addr_type;
  }

  /*---------------------------------------------------------------------------
    Set the address state and generate any appropriate events
  ---------------------------------------------------------------------------*/
  if(addr_info_ptr->addr_mask & PS_IFACE_ADDR_MGMT_MASK_ADDR_STATE)
  {
    if(addr_info_ptr->addr_state != v6_addr_ptr->addr_state)
    {
      ps_iface_addr_mgmti_update_ipv6_addr_state(iface_ptr,
                                                 addr_info_ptr->addr_state,
                                                 v6_addr_ptr);
    }
  }

  /*---------------------------------------------------------------------------
    Set the reference count for the address.
  ---------------------------------------------------------------------------*/
  if(addr_info_ptr->addr_mask & PS_IFACE_ADDR_MGMT_MASK_REF_CNT)
  {
    // todo
  }

  /*---------------------------------------------------------------------------
    Set the DAD retries for the address.
  ---------------------------------------------------------------------------*/
  if(addr_info_ptr->addr_mask & PS_IFACE_ADDR_MGMT_MASK_DAD_RETRIES)
  {
    v6_addr_ptr->dad_retries = addr_info_ptr->dad_retries;
  }

  return 0;
} /* ps_iface_addr_mgmti_set_addr_info() */


/*===========================================================================

                             PUBLIC FUNCTIONS

===========================================================================*/

/*===========================================================================
FUNCTION PS_IFACE_ADDR_MGMT_DEFAULT_ADDR_CB_FUNC()

DESCRIPTION
  This is the default address callback function. It does nothing and exists
  simply to indicate that mode handlers should be setting their own functions
  or setting the f_ptr to NULL.

PARAMETERS

RETURN VALUE
 -1 for failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_addr_mgmt_default_addr_cb_func
(
  ps_ip_addr_type                  ip_addr,
  ps_iface_addr_mgmt_event_type    addr_event,
  void                           * user_data
)
{
  LOG_MSG_ERROR("IPV6 addr_cb_f_ptr() has not been properly set!",0,0,0);
  return -1;
} /* ps_iface_addr_mgmt_default_addr_cb_func() */

/*===========================================================================
FUNCTION PS_IFACE_ADDR_MGMT_DEFAULT_DAD_FUNC()

DESCRIPTION
  This is the default DAD callback function. It does nothing and exists
  simply to indicate that mode handlers should be setting their own functions
  or setting the f_ptr to NULL.

PARAMETERS

RETURN VALUE
 -1 for failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_addr_mgmt_default_dad_func
(
  ps_iface_type                  * iface_ptr,
  ps_iface_addr_mgmt_handle_type * handle,
  void                           * user_data
)
{
  LOG_MSG_ERROR("IPV6 dad_f_ptr() has not been properly set!",0,0,0);
  return -1;
} /* ps_iface_addr_mgmt_default_dad_func() */

/*===========================================================================
FUNCTION PS_IFACE_ADDR_MGMT_GET_HANDLE_FROM_IP()

DESCRIPTION
  This function retrieves the handle given the IPv6 address structure.

PARAMETERS
  ip_addr_ptr:  The ptr to the ip address

RETURN VALUE
  handle if successful
 -1      if failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
ps_iface_addr_mgmt_handle_type ps_iface_addr_mgmt_get_handle_from_ip
(
  ps_iface_type      *iface_ptr,
  struct ps_in6_addr *ip_addr_ptr
)
{


  return ps_iface_addr_mgmt_get_handle_from_ip_ex(iface_ptr,
                                                  ip_addr_ptr,
                                                  IPV6_ADDR_TYPE_ALL);
} /* ps_iface_addr_mgmt_get_handle_ip()*/

/*===========================================================================
FUNCTION PS_IFACE_ADDR_MGMT_GET_HANDLE_FROM_IP_EX()

DESCRIPTION
  This function retrieves the handle given the IPv6 address structure.
  While looking for the address it only searches through IP address
  of type specified in addr_type_mask.

PARAMETERS
  ip_addr_ptr:  The ptr to the ip address
  addr_type_mask: IP address type mask.

RETURN VALUE
  handle if successful
 -1      if failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
ps_iface_addr_mgmt_handle_type ps_iface_addr_mgmt_get_handle_from_ip_ex
(
  ps_iface_type                         *iface_ptr,
  struct ps_in6_addr                    *ip_addr_ptr,
  ps_iface_ipv6_addr_type_mask_enum_type addr_type_mask
)
{
  ps_ifacei_v6_addr_type          * v6_addr_ptr = NULL;
  ps_iface_type                   * um_iface_ptr = iface_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if(ip_addr_ptr == NULL)
  {
    LOG_MSG_ERROR("NULL ip_addr_ptr: %p passed!", ip_addr_ptr, 0, 0);
    return -1;
  }

  /*---------------------------------------------------------------------
    Get Um bridged iface if Rm call.
  -------------------------------------------------------------------------*/
  if(iface_ptr->group_mask == RM_GROUP)
  {
    um_iface_ptr = ps_iface_bridge_iface(iface_ptr);

    if(!PS_IFACE_IS_VALID(um_iface_ptr))
    {
      LOG_MSG_ERROR("Invalid bridged Um iface_ptr",0,0,0);
      return -1;
    }
  }

  if(FALSE == ps_iface_find_ipv6_addr_ex(ip_addr_ptr, 
                                         &v6_addr_ptr, 
                                         &um_iface_ptr, 
                                         addr_type_mask) &&
     um_iface_ptr != iface_ptr &&
     FALSE == ps_iface_find_ipv6_addr_ex(ip_addr_ptr, 
                                         &v6_addr_ptr, 
                                         &iface_ptr, 
                                         addr_type_mask))
  {
    LOG_MSG_INFO2("Can't find IP address.",0,0,0);
    return -1;
  }

  return ps_iface_addr_mgmt_get_handle_from_addr(v6_addr_ptr);
} /* ps_iface_addr_mgmt_get_handle_from_ip_ex()*/

/*===========================================================================
FUNCTION PS_IFACE_ADDR_MGMT_EXT_ADDR_PRESENT()

DESCRIPTION
  This function cancels the timer associated with verifying that an
  external address is still present and in use.

PARAMETERS
  handle_ptr:  The handle to the IP address structure.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_addr_mgmt_ext_addr_present
(
  ps_iface_type                  * iface_ptr,
  ps_iface_addr_mgmt_handle_type * handle_ptr
)
{
  ps_ifacei_v6_addr_type            * v6_addr_ptr = NULL;
  ps_iface_type                     * um_iface_ptr = iface_ptr;
  ps_iface_addr_mgmt_addr_info_type   addr_info;
  int16                               ps_errno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if(NULL == (v6_addr_ptr = ps_iface_addr_mgmti_get_addr_from_handle(handle_ptr)))
  {
    LOG_MSG_ERROR("Error retrieving v6 address structure!",0,0,0);
    return;
  }

  /*---------------------------------------------------------------------
    Get Um bridged iface if Rm call.
  -------------------------------------------------------------------------*/
  if(iface_ptr->group_mask == RM_GROUP)
  {
    um_iface_ptr = ps_iface_bridge_iface(iface_ptr);

    if(!PS_IFACE_IS_VALID(um_iface_ptr))
    {
      LOG_MSG_ERROR("Invalid bridged Um iface_ptr",0,0,0);
      return;
    }
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    If um iface has inherit IP info true then get the base iface of the 
    logical chain, else do nothing.
  -------------------------------------------------------------------------*/
  um_iface_ptr = PS_IFACE_GET_BASE_IFACE(um_iface_ptr);

  PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);

  memset(&addr_info, 0, sizeof(addr_info));
  addr_info.addr_mask  = PS_IFACE_ADDR_MGMT_MASK_ADDR_STATE;
  addr_info.addr_state = IPV6_ADDR_STATE_VALID;

  (void) ps_iface_addr_mgmti_set_addr_info(um_iface_ptr,
                                           handle_ptr,
                                           &addr_info,
                                           &ps_errno);

  (void) ps_timer_start(v6_addr_ptr->unused_addr_timer_handle,
                        IPV6_EXT_ADDR_INTERVAL_VERIFY_TIME);

} /* ps_iface_addr_mgmt_ext_addr_present()*/

/*===========================================================================
FUNCTION PS_IFACE_ADDR_MGMT_ALLOC_ADDR()

DESCRIPTION
  This function allocates an address buffer and returns the handle to the
  caller.

PARAMETERS
  iface_ptr:       Interface on which the address exists.
  handle_ptr:      The handle to the newly alloc'd IP address structure.
  alloc_info:      Required information to allocate the address;
  ps_errno:        The error code to return in case of a problem.

RETURN VALUE
  0 success
 -1 failure

DEPENDENCIES
  None

SIDE EFFECTS
  Allocates a ps mem buffer for the address.
===========================================================================*/
int ps_iface_addr_mgmt_alloc_addr
(
  ps_iface_type                  * iface_ptr,
  ps_iface_addr_mgmt_handle_type * handle_ptr,
  ps_iface_addr_mgmt_alloc_type  * create_info,
  int16                          * ps_errno
)
{
  ps_ifacei_v6_addr_type         * v6_addr_ptr  = NULL;
  ps_iface_type                  * um_iface_ptr = iface_ptr;
  uint8                            index;
  struct ps_in6_addr               ipv6_addr;  
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1("ps_iface_addr_mgmt_alloc_addr()",0,0,0);

  if (!PS_IFACE_IS_VALID(iface_ptr))
  {
    LOG_MSG_INFO1("Invalid iface_ptr to ps_iface_addr_mgmt_alloc_addr()",0,0,0);
    ASSERT(0);
    return -1;
  }

  if(handle_ptr == NULL || create_info == NULL || ps_errno == NULL)
  {
    LOG_MSG_INFO1("NULL handle_ptr: %p, addr_info_ptr: %p, or ps_errno: %p",
             handle_ptr, create_info, ps_errno);
    ASSERT(0);
    return -1;
  }

  LOG_MSG_INFO2("alloc_addr on iface %p", iface_ptr, 0, 0);

  /*---------------------------------------------------------------------
    Get Um bridged iface if Rm call.
  -------------------------------------------------------------------------*/
  if(iface_ptr->group_mask == RM_GROUP)
  {
    um_iface_ptr = ps_iface_bridge_iface(iface_ptr);

    if(!PS_IFACE_IS_VALID(um_iface_ptr))
    {
      LOG_MSG_ERROR("Invalid bridged Um iface_ptr",0,0,0);
      return -1;
    }
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    If um iface has inherit IP info true then get the base iface of the 
    logical chain, else do nothing.
  -------------------------------------------------------------------------*/
  um_iface_ptr = PS_IFACE_GET_BASE_IFACE(um_iface_ptr);

  if(create_info->ip_addr.type == IPV6_ADDR)
  {
    if(PS_IFACE_IPV6_IS_ADDR_SPACE_AVAIL(um_iface_ptr, &index))
    {
      um_iface_ptr->iface_private.ipv6_addrs[index] =
        (ps_ifacei_v6_addr_type *) ps_mem_get_buf(PS_MEM_IPV6_ADDR_TYPE);

      v6_addr_ptr = um_iface_ptr->iface_private.ipv6_addrs[index];
      if(NULL == v6_addr_ptr)
      {
        LOG_MSG_INFO2("Out of PS MEM IPv6 address items!",0,0,0);
        *ps_errno = DS_ENOMEM;
        PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
        return -1;
      }

      memset(v6_addr_ptr, 0, sizeof(ps_ifacei_v6_addr_type));

      /*-------------------------------------------------------------------------
        Allocate the timers, if an external address alloc only the unused timer.
      -------------------------------------------------------------------------*/
      if(create_info->addr_type != IPV6_ADDR_TYPE_EXTERNAL)
      {
        v6_addr_ptr->pref_lifetimer_handle =
          ps_timer_alloc(ps_ifacei_ipv6_iid_timer_handler,
                         v6_addr_ptr);
        v6_addr_ptr->valid_lifetimer_handle =
          ps_timer_alloc(ps_ifacei_ipv6_iid_timer_handler,
                         v6_addr_ptr);
        if(v6_addr_ptr->valid_lifetimer_handle == PS_TIMER_INVALID_HANDLE ||
           v6_addr_ptr->pref_lifetimer_handle == PS_TIMER_INVALID_HANDLE)
        {
          LOG_MSG_ERROR("Error can't allocate timers for IPv6  addr!",0,0,0);
          PS_TIMER_FREE_HANDLE(v6_addr_ptr->valid_lifetimer_handle);
          PS_TIMER_FREE_HANDLE(v6_addr_ptr->pref_lifetimer_handle);
          PS_MEM_FREE(um_iface_ptr->iface_private.ipv6_addrs[index]);
          *ps_errno = DS_ENOMEM;
          PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
          return -1;
        }

      }

      if(iface_ptr->group_mask == RM_GROUP)
      {
        v6_addr_ptr->rm_iface_ptr = iface_ptr;
      }

      v6_addr_ptr->unused_addr_timer_handle =
        ps_timer_alloc(ps_ifacei_ipv6_unused_iid_handler,
                       v6_addr_ptr);

      if(v6_addr_ptr->unused_addr_timer_handle == PS_TIMER_INVALID_HANDLE)
      {
        LOG_MSG_ERROR("Error can't allocate timers for IPv6  addr!",0,0,0);
        PS_TIMER_FREE_HANDLE(v6_addr_ptr->unused_addr_timer_handle);
        PS_MEM_FREE(um_iface_ptr->iface_private.ipv6_addrs[index]);
        *ps_errno = DS_ENOMEM;
        PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
        return -1;
      }

      v6_addr_ptr->addr_type = create_info->addr_type;

      /*---------------------------------------------------------------------
        If this is a linklocal prefix, use the prefix of the iface.
      ---------------------------------------------------------------------*/
      if( PS_IN6_IS_PREFIX_LINKLOCAL
          ( create_info->ip_addr.addr.v6.ps_s6_addr64[0] ) )
      {
        v6_addr_ptr->prefix =
          um_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX]->prefix;
      }
      else
      {
        v6_addr_ptr->prefix = create_info->ip_addr.addr.v6.ps_s6_addr64[0];
      }
      v6_addr_ptr->iid = create_info->ip_addr.addr.v6.ps_s6_addr64[1];

      ipv6_addr.in6_u.u6_addr64[0] = v6_addr_ptr->prefix;
      ipv6_addr.in6_u.u6_addr64[1] = v6_addr_ptr->iid;
  
      IPV6_ADDR_MSG(ipv6_addr.ps_s6_addr64);

      if(-1 ==
         (*handle_ptr = ps_iface_addr_mgmt_get_handle_from_addr(v6_addr_ptr)))
      {
        PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
        return -1;
      }
    }
    else
    {
      LOG_MSG_INFO2("MAX IPv6 addresses already allocated for iface %p!",
              um_iface_ptr, 0, 0);
      *ps_errno = DS_ENOMEM;
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      return -1;
    }
  }
  else
  {
    LOG_MSG_ERROR("Unsupported IP alloc type: %d", create_info->ip_addr.type, 0, 0);
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return -1;
  }

  LOG_MSG_INFO1("ps_iface_addr_mgmt_alloc_addr(): success",0,0,0);
  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;
} /* ps_iface_addr_mgmt_alloc_addr() */

/*===========================================================================
FUNCTION PS_IFACE_ADDR_MGMT_FREE_ADDR()

DESCRIPTION
  This function frees the address buffer associated with the passed handle.

PARAMETERS
  iface_ptr:       Interface on which the address exists.
  handle_ptr:      The handle to the newly alloc'd IP address structure.
  free_info_ptr:   Any additional required information needed.
  ps_errno:        The error code to return in case of a problem.

RETURN VALUE
  0 success
 -1 failure

DEPENDENCIES
  None

SIDE EFFECTS
  Allocates a ps mem buffer for the address.
===========================================================================*/
int ps_iface_addr_mgmt_free_addr
(
  ps_iface_type                  * iface_ptr,
  ps_iface_addr_mgmt_handle_type * handle_ptr,
  ps_iface_addr_mgmt_free_type   * free_info,
  int16                          * ps_errno
)
{
  uint8   i;
  boolean                      addr_deleted = FALSE;
  boolean                      invoke_cback = FALSE;
  ps_iface_event_enum_type     event;
  ps_iface_event_info_u_type   event_info;
  ps_ifacei_v6_addr_type     * v6_addr_ptr = NULL;
  ps_iface_type              * um_iface_ptr = iface_ptr;
  ps_ip_addr_type              ip_addr;
  struct ps_in6_addr           ipv6_addr;    
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_INFO1("ps_iface_addr_mgmt_free_addr()",0,0,0);

  memset(&event_info, 0, sizeof(event_info));
  memset(&event, 0, sizeof(event));

  if (!PS_IFACE_IS_VALID(iface_ptr))
  {
    LOG_MSG_INFO1("Invalid iface_ptr to ps_iface_addr_mgmt_free_addr",0,0,0);
    ASSERT(0);
    return -1;
  }

  if(handle_ptr == NULL || ps_errno == NULL)
  {
    LOG_MSG_INFO1("NULL handle_ptr: %p or ps_errno: %p", handle_ptr, ps_errno, 0);
    ASSERT(0);
    return -1;
  }

  if(NULL == (v6_addr_ptr = ps_iface_addr_mgmti_get_addr_from_handle(handle_ptr)))
  {
    LOG_MSG_ERROR("Error retrieving v6 address structure!",0,0,0);
    return -1;
  }

  ipv6_addr.in6_u.u6_addr64[0] = v6_addr_ptr->prefix;
  ipv6_addr.in6_u.u6_addr64[1] = v6_addr_ptr->iid;
  
  IPV6_ADDR_MSG(ipv6_addr.ps_s6_addr64);

  /*-------------------------------------------------------------------------
    Get Um bridged iface if Rm call.
  -------------------------------------------------------------------------*/
  if(iface_ptr->group_mask == RM_GROUP)
  {
    um_iface_ptr = ps_iface_bridge_iface(iface_ptr);

    if(!PS_IFACE_IS_VALID(um_iface_ptr))
    {
      LOG_MSG_ERROR("Invalid bridged Um iface_ptr",0,0,0);
      return -1;
    }
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    If um iface has inherit IP info true then get the base iface of the 
    logical chain, else do nothing.
  -------------------------------------------------------------------------*/
  um_iface_ptr = PS_IFACE_GET_BASE_IFACE(um_iface_ptr);

  /*--------------------------------------------------------------------------
    Free all timers, remove the IPv6 privacy address, generate DELETED_EV and
    remove all event callbacks.
  --------------------------------------------------------------------------*/
  PS_TIMER_FREE_HANDLE(v6_addr_ptr->pref_lifetimer_handle);
  PS_TIMER_FREE_HANDLE(v6_addr_ptr->valid_lifetimer_handle);
  PS_TIMER_FREE_HANDLE(v6_addr_ptr->unused_addr_timer_handle);

  /*---------------------------------------------------------------------------
    Setup callback information for internal addresses.
  ---------------------------------------------------------------------------*/
  ip_addr.type = IPV6_ADDR;
  ip_addr.addr.v6.ps_s6_addr64[0] = v6_addr_ptr->prefix;
  ip_addr.addr.v6.ps_s6_addr64[1] = v6_addr_ptr->iid;

  if(v6_addr_ptr->addr_type != IPV6_ADDR_TYPE_EXTERNAL)
  {
    if(v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_PRIV_SHARED ||
       v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_PRIV_UNIQUE)
    {
      event = IFACE_IPV6_PRIV_ADDR_DELETED_EV;
      event_info.priv_ipv6_addr.is_unique =
        (v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_PRIV_UNIQUE) ? TRUE : FALSE;
      event_info.priv_ipv6_addr.ip_addr.type                 = IPV6_ADDR;
      event_info.priv_ipv6_addr.ip_addr.addr.v6.ps_s6_addr64[0] = v6_addr_ptr->prefix;
      event_info.priv_ipv6_addr.ip_addr.addr.v6.ps_s6_addr64[1] = v6_addr_ptr->iid;
    }
    else
    {
      event                                      = IFACE_PREFIX_UPDATE_EV;
      event_info.prefix_info.kind                = PREFIX_REMOVED;
      event_info.prefix_info.prefix.ps_s6_addr64[0] = v6_addr_ptr->prefix;
      event_info.prefix_info.prefix_len          = v6_addr_ptr->prefix_len;
    }

    invoke_cback = TRUE;
  }

  /*-------------------------------------------------------------------------
    Find and free the IPv6 address memory.
  -------------------------------------------------------------------------*/
  for(i=0; i < MAX_IPV6_ADDRS; i++)
  {
    if(um_iface_ptr->iface_private.ipv6_addrs[i] == NULL)
    {
      break;
    }

    if(um_iface_ptr->iface_private.ipv6_addrs[i] == v6_addr_ptr)
    {
      PS_MEM_FREE(um_iface_ptr->iface_private.ipv6_addrs[i]);
      addr_deleted = TRUE;
      break;
    }
  }

  if(!addr_deleted)
  {
    LOG_MSG_ERROR("Couldn't locate IPv6 address for deletion on iface %p",
              um_iface_ptr, 0, 0);
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return -1;
  }

  if(um_iface_ptr->addr_cb_f_ptr != NULL)
  {
    (void) um_iface_ptr->addr_cb_f_ptr(ip_addr,
                                IFACE_ADDR_MGMT_ADDR_DELETED,
                                um_iface_ptr->client_data_ptr);
  }

  if(invoke_cback)
  {
    ps_ifacei_invoke_event_cbacks(um_iface_ptr,
                                  NULL,
                                  event,
                                  event_info);
  }

  LOG_MSG_INFO1("ps_iface_addr_mgmt_alloc_addr(): "
                "Condensing IPv6 address array.",0,0,0);
  /*--------------------------------------------------------------------------
    Cleanup and condense address array.
  --------------------------------------------------------------------------*/
  while( (i < (MAX_IPV6_ADDRS-1)) &&
         (um_iface_ptr->iface_private.ipv6_addrs[i+1] != NULL))
  {
    um_iface_ptr->iface_private.ipv6_addrs[i] =
      um_iface_ptr->iface_private.ipv6_addrs[i+1];
    i++;
  }
  um_iface_ptr->iface_private.ipv6_addrs[i] = NULL;

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  LOG_MSG_INFO1("ps_iface_addr_mgmt_free_addr(): Success",0,0,0);
  return 0;
} /* ps_iface_addr_mgmt_free_addr() */

/*===========================================================================
FUNCTION PS_IFACE_ADDR_MGMT_GET_ADDR_INFO()

DESCRIPTION
  This function is used to retrieve information about an address to the
  client.

PARAMETERS
  handle_ptr:    The handle to the address structure from which to retrieve
                 the information.
  addr_info_ptr: The address information structure to populate.
  ps_errno:      The error code to return in case of a problem.

RETURN VALUE
  0 success
 -1 failure

DEPENDENCIES
  None

SIDE EFFECTS
  Copies the IP address iformation from the IP address structure to the
  client's address information structure.
===========================================================================*/
int ps_iface_addr_mgmt_get_addr_info
(
  ps_iface_addr_mgmt_handle_type    * handle_ptr,
  ps_iface_addr_mgmt_addr_info_type * addr_info_ptr,
  int16                             * ps_errno
)
{
  ps_ifacei_v6_addr_type            * v6_addr_ptr = NULL;
  int64  lifetime;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if(handle_ptr == NULL || addr_info_ptr == NULL || ps_errno == NULL)
  {
    LOG_MSG_INFO1("NULL handle_ptr: %p, addr_info_ptr: %p, or ps_errno: %p",
             handle_ptr, addr_info_ptr, ps_errno);
    ASSERT(0);
    return -1;
  }

  if(NULL == (v6_addr_ptr = ps_iface_addr_mgmti_get_addr_from_handle(handle_ptr)))
  {
    LOG_MSG_ERROR("Error retrieving v6 address structure!",0,0,0);
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);
  addr_info_ptr->prefix      = v6_addr_ptr->prefix;
  addr_info_ptr->iid         = v6_addr_ptr->iid;
  addr_info_ptr->gateway_iid = v6_addr_ptr->gateway_iid;
  addr_info_ptr->prefix_len  = v6_addr_ptr->prefix_len;
  addr_info_ptr->addr_state  = v6_addr_ptr->addr_state;
  addr_info_ptr->addr_type   = v6_addr_ptr->addr_type;
  addr_info_ptr->dad_retries = v6_addr_ptr->dad_retries;

  /*-------------------------------------------------------------------------
    If the pref lifetime timer is running, set pref_lifetime to the
    remaining time, otherwise set it to infinite if the address is valid
  -------------------------------------------------------------------------*/
  if(v6_addr_ptr->pref_lifetimer_handle != 0)
  {
    lifetime = ps_timer_remaining(v6_addr_ptr->pref_lifetimer_handle)/1000;
    if( lifetime != -1 )
    {
      addr_info_ptr->pref_lifetime = (uint32) lifetime;
    }
  }
  else
  {
    if(v6_addr_ptr->addr_state == IPV6_ADDR_STATE_VALID)
    {
      addr_info_ptr->pref_lifetime = 0xFFFFFFFFUL;
    }
    else
    {
      addr_info_ptr->pref_lifetime = 0;
    }
  }

  /*-------------------------------------------------------------------------
    If the valid lifetime timer is running, set valid_lifetime to the
    remaining time, otherwise set it to infinite if the address is valid
  -------------------------------------------------------------------------*/
  if(v6_addr_ptr->valid_lifetimer_handle != 0)
  {
    lifetime = ps_timer_remaining(v6_addr_ptr->valid_lifetimer_handle)/1000;
    if( lifetime != -1 )
    {
      addr_info_ptr->valid_lifetime = (uint32) lifetime;
    }
  }
  else
  {
    if(v6_addr_ptr->addr_state == IPV6_ADDR_STATE_VALID)
    {
      addr_info_ptr->valid_lifetime = 0xFFFFFFFFUL;
    }
    else
    {
      addr_info_ptr->valid_lifetime = 0;
    }
  }

  addr_info_ptr->addr_mask = PS_IFACE_ADDR_MGMT_MASK_ALL;

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;
} /* ps_iface_addr_mgmt_get_addr_info() */

/*===========================================================================
FUNCTION PS_IFACE_ADDR_MGMT_SET_ADDR_INFO()

DESCRIPTION
  This function is used to set address information in the IP address
  structure.

PARAMETERS
  handle_ptr:    The handle to the IP address structure to update.
  addr_info_ptr: The address information structure from which to populate.
  ps_errno:      The error code to return in case of a problem.

RETURN VALUE
  0 success
 -1 failure

DEPENDENCIES
  None

SIDE EFFECTS
  Copies the IP address iformation from the client's IP address information
  structure to the IP address structure.
===========================================================================*/
int ps_iface_addr_mgmt_set_addr_info
(
  ps_iface_type                     * iface_ptr,
  ps_iface_addr_mgmt_handle_type    * handle_ptr,
  ps_iface_addr_mgmt_addr_info_type * addr_info_ptr,
  int16                             * ps_errno
)
{
  ps_iface_type                     * um_iface_ptr = iface_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (!PS_IFACE_IS_VALID(iface_ptr))
  {
    LOG_MSG_INFO1("Invalid iface_ptr to ps_iface_addr_mgmt_ipv6_do_dad",0,0,0);
    ASSERT(0);
    return -1;
  }

  if(handle_ptr == NULL || addr_info_ptr == NULL || ps_errno == NULL)
  {
    LOG_MSG_INFO1("NULL handle_ptr: %p, addr_info_ptr: %p, or ps_errno: %p",
             handle_ptr, addr_info_ptr, ps_errno);
    ASSERT(0);
    return -1;
  }

  if(addr_info_ptr->addr_mask & PS_IFACE_ADDR_MGMT_MASK_INTERNAL_ONLY)
  {
    LOG_MSG_ERROR("Illegial mask set 0x%x", addr_info_ptr->addr_mask, 0, 0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    Get Um bridged iface if Rm call.
  -------------------------------------------------------------------------*/
  if(iface_ptr->group_mask == RM_GROUP)
  {
    um_iface_ptr = ps_iface_bridge_iface(iface_ptr);

    if(!PS_IFACE_IS_VALID(um_iface_ptr))
    {
      LOG_MSG_ERROR("Invalid bridged Um iface_ptr",0,0,0);
      return -1;
    }
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    If um iface has inherit IP info true then get the base iface of the 
    logical chain, else do nothing.
  -------------------------------------------------------------------------*/
  um_iface_ptr = PS_IFACE_GET_BASE_IFACE(um_iface_ptr);

  if(-1 == ps_iface_addr_mgmti_set_addr_info(um_iface_ptr,
                                             handle_ptr,
                                             addr_info_ptr,
                                             ps_errno))
  {
    LOG_MSG_ERROR("Error setting address info",0,0,0);
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return -1;
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;
} /* ps_iface_addr_mgmt_set_addr_info() */

/*===========================================================================
FUNCTION PS_IFACE_ADDR_MGMT_IPV6_DO_DAD()

DESCRIPTION
  This function is used to initiate duplicate address detection on an address.

PARAMETERS
  iface_ptr:    The interface on which the address verification is to be
                performed.
  ipv6_addr:    The address to perform duplicate address detection on.
  ps_errno:     The error code to return in case of a problem.

RETURN VALUE
  0 success
 -1 failure

DEPENDENCIES
  None

SIDE EFFECTS
===========================================================================*/
int ps_iface_addr_mgmt_ipv6_do_dad
(
  ps_iface_type                  * iface_ptr,
  ps_iface_addr_mgmt_handle_type * handle_ptr,
  ps_iface_addr_mgmt_alloc_type  * alloc_info,
  int16                          * ps_errno
)
{
  ps_iface_addr_mgmt_addr_info_type addr_info;
  ps_ifacei_v6_addr_type          * v6_addr_ptr = NULL;
  ps_iface_type                   * um_iface_ptr = iface_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (!PS_IFACE_IS_VALID(iface_ptr))
  {
    LOG_MSG_INFO1("Invalid iface_ptr to ps_iface_addr_mgmt_ipv6_do_dad",0,0,0);
    ASSERT(0);
    return -1;
  }

  if (handle_ptr == NULL || ps_errno == NULL)
  {
    LOG_MSG_INFO1("NULL handle_ptr: %p or ps_errno: %p", handle_ptr, ps_errno, 0);
    ASSERT(0);
    return -1;
  }

  LOG_MSG_INFO2("do_dad, iface %p", iface_ptr, 0, 0);

  v6_addr_ptr = ps_iface_addr_mgmti_get_addr_from_handle(handle_ptr);

  if(v6_addr_ptr == NULL)
  {
    LOG_MSG_ERROR("Error retrieving v6 address structure!",0,0,0);
    ASSERT(0);
    return -1;
  }

  /*---------------------------------------------------------------------
    If external device is attempting DAD, update routing table, alloc
    addr, etc. *Future*

    Get Um bridged iface if Rm call.
  ---------------------------------------------------------------------*/
  if(iface_ptr->group_mask == RM_GROUP)
  {
    um_iface_ptr = ps_iface_bridge_iface(iface_ptr);

    if(!PS_IFACE_IS_VALID(um_iface_ptr))
    {
      LOG_MSG_ERROR("Invalid bridged Um iface_ptr",0,0,0);
      return -1;
    }
  }
  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    If um iface has inherit IP info true then get the base iface of the 
    logical chain, else do nothing.
  -------------------------------------------------------------------------*/
  um_iface_ptr = PS_IFACE_GET_BASE_IFACE(um_iface_ptr);

  memset(&addr_info, 0, sizeof(addr_info));

  // for external addrs do we want to attempt to check if they've already done
  // DAD on an addr and it's in the Um cache do we want to fail and send NA if
  // it's already there? or just ignore? look at addr type (external) to know.

  if (um_iface_ptr->dad_f_ptr != NULL)
  {
    /*-----------------------------------------------------------------------
      Performing DAD on Um. Mark address as TENTATIVE
    -----------------------------------------------------------------------*/
    addr_info.addr_mask  = PS_IFACE_ADDR_MGMT_MASK_ADDR_STATE;
    addr_info.addr_state = IPV6_ADDR_STATE_TENTATIVE;
    (void)ps_iface_addr_mgmti_set_addr_info( um_iface_ptr,
                                             handle_ptr,
                                             &addr_info,
                                             ps_errno );
    LOG_MSG_INFO2("invoking dad_f_ptr um iface %p", um_iface_ptr, 0, 0);
    if (-1 == um_iface_ptr->dad_f_ptr(um_iface_ptr,
                                      handle_ptr,
                                      um_iface_ptr->client_data_ptr))
    {
      LOG_MSG_ERROR("Failure to perform DAD on address",0,0,0);
      (void)ps_iface_addr_mgmt_free_addr(um_iface_ptr, handle_ptr,
                                         NULL, ps_errno);
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      return -1;
    }
  }
  else
  {
    /*-----------------------------------------------------------------------
      DAD doesn't need to be performed on Um. Mark address as VALID
    -----------------------------------------------------------------------*/
    addr_info.addr_mask  = PS_IFACE_ADDR_MGMT_MASK_ADDR_STATE;
    addr_info.addr_state = IPV6_ADDR_STATE_VALID;
    (void)ps_iface_addr_mgmti_set_addr_info( um_iface_ptr,
                                             handle_ptr,
                                             &addr_info,
                                             ps_errno );

    /*-----------------------------------------------------------------------
      Start the unused addr timer if this is an external address
    -----------------------------------------------------------------------*/
    if( (v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_EXTERNAL) &&
        (v6_addr_ptr->unused_addr_timer_handle != 0) )
    {
      (void)ps_timer_start( v6_addr_ptr->unused_addr_timer_handle,
                            IPV6_EXT_ADDR_INTERVAL_VERIFY_TIME );
    }
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;
} /* ps_iface_addr_mgmt_ipv6_do_dad() */

/*===========================================================================
FUNCTION PS_IFACE_ADDR_MGMT_IPV6_DEFAULT_DAD_F()

DESCRIPTION
  This function is used to initiate the standard method of DAD as per
  RFC 2461. This can be set as the the dad_f_ptr for any interface utilizing
  this RFC.

PARAMETERS
  iface_ptr:    The interface on which the address verification is to be
                performed.
  ipv6_addr:    The address to perform duplicate address detection on.
  ps_errno:     The error code to return in case of a problem.

RETURN VALUE
  0 success
 -1 failure

DEPENDENCIES
  None

SIDE EFFECTS
===========================================================================*/
int ps_iface_addr_mgmt_ipv6_default_dad_f
(
  ps_iface_type         * iface_ptr,
  struct ps_in6_addr    * ip_addr_ptr,
  int16                 * ps_errno
)
{
  ps_ifacei_v6_addr_type  * v6_addr_ptr = NULL;
  ps_iface_type           * um_iface_ptr = iface_ptr;
  ps_iface_type           * ps_iface_ptr = iface_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (!PS_IFACE_IS_VALID(iface_ptr))
  {
    LOG_MSG_INFO1("Invalid iface_ptr to ps_iface_addr_mgmt_ipv6_do_dad",0,0,0);
    ASSERT(0);
    return -1;
  }

  if (ip_addr_ptr == NULL || ps_errno == NULL)
  {
    LOG_MSG_INFO1("NULL ip_addr_ptr: %p or ps_errno: %p",ip_addr_ptr, ps_errno, 0);
    ASSERT(0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    Get Um bridged iface for prefix information.
  -------------------------------------------------------------------------*/
  if(iface_ptr->group_mask == RM_GROUP)
  {
    um_iface_ptr = ps_iface_bridge_iface(iface_ptr);

    if(!PS_IFACE_IS_VALID(um_iface_ptr))
    {
      LOG_MSG_ERROR("Invalid bridged Um iface_ptr",0,0,0);
      return -1;
    }
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    If um iface has inherit IP info true then get the base iface of the 
    logical chain, else do nothing.
  -------------------------------------------------------------------------*/
  um_iface_ptr = PS_IFACE_GET_BASE_IFACE(um_iface_ptr);

  /*-------------------------------------------------------------------------
    Because ps_iface_find_ipv6_addr drills down to base iface, we do not need 
    to call PS_IFACE_GET_BASE_IFACE for ps_iface_ptr
  -------------------------------------------------------------------------*/

  if( FALSE ==
      ps_iface_find_ipv6_addr( ip_addr_ptr, &v6_addr_ptr, &ps_iface_ptr ) )
  {
    LOG_MSG_ERROR("Cannot find IPV6 address",0,0,0);
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return -1;
  }

  /*-------------------------------------------------------------------------
    Allocate unused timer if not done already.
  -------------------------------------------------------------------------*/
  if( PS_TIMER_INVALID_HANDLE == v6_addr_ptr->unused_addr_timer_handle )
  {
    v6_addr_ptr->unused_addr_timer_handle =
      ps_timer_alloc(ps_ifacei_ipv6_unused_iid_handler,
                     v6_addr_ptr);
  }

  /*-------------------------------------------------------------------------
    Send Neighbor Solicitation (NS) messsage.
  -------------------------------------------------------------------------*/
  if(0 == ps_icmp6_nd_send_ns(um_iface_ptr,
                              ip_addr_ptr,
                              ICMP6_ND_DAD))
  {
    (void)ps_timer_start(v6_addr_ptr->unused_addr_timer_handle,
                         ICMP6_ND_DEFAULT_RETRANSMIT_TIME);

    v6_addr_ptr->dad_retries--;
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return 0;
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return -1;
} /* ps_iface_addr_mgmt_ipv6_default_dad_f() */

/*===========================================================================
FUNCTION PS_IFACE_ADDR_MGMT_IPV6_DAD_UPDATE()

DESCRIPTION
  This function is used for clients to indicate to the interface layer
  the status of a nonstandard DAD attempt.

PARAMETERS
  iface_ptr:    The interface on which the address verification is being
                performed.
  handle_ptr:   The handle to the IP address structure to update.
  dad_code:     The error/success code for the dad attempt.
  ps_errno:     The error code to return in case of a problem.

RETURN VALUE
  0 success
 -1 failure

DEPENDENCIES
  None

SIDE EFFECTS
===========================================================================*/
int ps_iface_addr_mgmt_ipv6_dad_update
(
  ps_iface_type                    * iface_ptr,
  ps_iface_addr_mgmt_handle_type   * handle_ptr,
  ps_iface_addr_mgmt_dad_enum_type   dad_code,
  int16                            * ps_errno
)
{
  ps_iface_addr_mgmt_addr_info_type   addr_info;
  ps_iface_type                     * bridge_iface_ptr = NULL;
  ps_iface_type                     * um_iface_ptr = NULL;
  ps_ifacei_v6_addr_type            * v6_addr_ptr = NULL;
  struct ps_in6_addr                  ip_addr;
  uint64                              iid;
  ps_iface_event_info_u_type          event_info;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (!PS_IFACE_IS_VALID(iface_ptr))
  {
    LOG_MSG_INFO1("Invalid iface_ptr to ps_iface_addr_mgmt_ipv6_dad_update",0,0,0);
    ASSERT(0);
    return -1;
  }

  if(handle_ptr == NULL || ps_errno == NULL)
  {
    LOG_MSG_INFO1("NULL handle_ptr: %p, or ps_errno: %p",handle_ptr, ps_errno, 0);
    ASSERT(0);
    return -1;
  }

  if(NULL == (v6_addr_ptr = ps_iface_addr_mgmti_get_addr_from_handle(handle_ptr)))
  {
    LOG_MSG_ERROR("Handle: 0x%p doesn't exist!", handle_ptr, 0, 0);
    return -1;
  }

  memset(&event_info,0,sizeof(event_info));

  /*-------------------------------------------------------------------------
    Get Um/Rm bridged iface for notifying external addresses.
  -------------------------------------------------------------------------*/
  if( PS_IFACE_GET_IS_PROXY_IFACE(iface_ptr) )
  {
    ASSERT( iface_ptr-> group_mask != RM_GROUP );
    um_iface_ptr = iface_ptr;
  }
  else
  {
    bridge_iface_ptr = ps_iface_bridge_iface(iface_ptr);

    if( !PS_IFACE_IS_VALID(bridge_iface_ptr) )
    {
      LOG_MSG_ERROR("Invalid bridged iface_ptr 0x%p", bridge_iface_ptr, 0, 0);
      return -1;
    }

    um_iface_ptr = (iface_ptr->group_mask != RM_GROUP)
                   ? iface_ptr : bridge_iface_ptr;
  }
  
  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);
  
  /*-------------------------------------------------------------------------
    If um iface has inherit IP info true then get the base iface of the 
    logical chain, else do nothing.
  -------------------------------------------------------------------------*/
  um_iface_ptr = PS_IFACE_GET_BASE_IFACE(um_iface_ptr);

  /*---------------------------------------------------------------------------
    If DAD was a success notify the applications (if it's not an external addr)
  ---------------------------------------------------------------------------*/
  if(dad_code == IFACE_ADDR_MGMT_DAD_SUCCESS)
  {
    memset(&addr_info,0,sizeof(addr_info));
    addr_info.addr_mask  = PS_IFACE_ADDR_MGMT_MASK_ADDR_STATE;
    addr_info.addr_state = IPV6_ADDR_STATE_VALID;

    (void)ps_iface_addr_mgmti_set_addr_info(um_iface_ptr, handle_ptr,
                                            &addr_info, ps_errno);

    /*-----------------------------------------------------------------------
      Start the unused addr timer if this is an external address
    -----------------------------------------------------------------------*/
    if( (v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_EXTERNAL) &&
        (v6_addr_ptr->unused_addr_timer_handle != 0) )
    {
      (void)ps_timer_start( v6_addr_ptr->unused_addr_timer_handle,
                            IPV6_EXT_ADDR_INTERVAL_VERIFY_TIME );
    }
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return 0;
  }

  /*---------------------------------------------------------------------------
    DAD failure for an external address. Return an error and free the addr.
  ---------------------------------------------------------------------------*/
  if(v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_EXTERNAL)
  {
   ip_addr.ps_s6_addr64[0] = v6_addr_ptr->prefix;
   ip_addr.ps_s6_addr64[1] = v6_addr_ptr->iid;
   (void) ps_icmp6_nd_send_na(bridge_iface_ptr, &ip_addr, &ip_addr,
                              ICMP6_ND_DAD, TRUE);

    (void) ps_iface_addr_mgmt_free_addr(um_iface_ptr, handle_ptr, NULL, ps_errno);
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return -1;
  }

  /*---------------------------------------------------------------------------
    Address must be internal, regenerate and try again if retries remain
  ---------------------------------------------------------------------------*/
  switch(dad_code)
  {
    case IFACE_ADDR_MGMT_DAD_NO_MEM:
      (void) ps_iface_addr_mgmt_free_addr(um_iface_ptr, handle_ptr, NULL, ps_errno);
      break;

    case IFACE_ADDR_MGMT_DAD_DUP_ADDR:
    case IFACE_ADDR_MGMT_DAD_NETWORK_REJECT:

      if(v6_addr_ptr->dad_retries != 0 &&
         0 == ps_iface_generate_ipv6_iid(um_iface_ptr, &iid, ps_errno))
      {
        (void)ps_timer_cancel(v6_addr_ptr->unused_addr_timer_handle);
        memset(&addr_info,0,sizeof(addr_info));
        addr_info.addr_mask  = PS_IFACE_ADDR_MGMT_MASK_IID;
        addr_info.iid        = iid;

        (void)ps_iface_addr_mgmti_set_addr_info(um_iface_ptr, handle_ptr,
                                                &addr_info, ps_errno);
      }
      /*lint -fallthrough */

    case IFACE_ADDR_MGMT_DAD_TIMEOUT:

      if(v6_addr_ptr->dad_retries != 0)
      {
        if (um_iface_ptr->dad_f_ptr != NULL)
        {
          if (-1 == um_iface_ptr->dad_f_ptr(um_iface_ptr,
                                            handle_ptr,
                                            um_iface_ptr->client_data_ptr))
          {
            LOG_MSG_ERROR("Failure to perform DAD on address",0,0,0);
            (void) ps_iface_addr_mgmt_free_addr(um_iface_ptr, handle_ptr,
                                                NULL, ps_errno);
          }
        }
      }
      else
      {
        /*---------------------------------------------------------------------
          Retries exceeded. If not due to DAD timeout (which is a AEE_SUCCESS)
          return the address as 0. This satisfies privacy extensions waiting on
          an address.
        ---------------------------------------------------------------------*/
        if(dad_code == IFACE_ADDR_MGMT_DAD_TIMEOUT)
        {
          memset(&addr_info,0,sizeof(addr_info));
          addr_info.addr_mask  = PS_IFACE_ADDR_MGMT_MASK_ADDR_STATE;
          addr_info.addr_state = IPV6_ADDR_STATE_VALID;

          (void)ps_iface_addr_mgmti_set_addr_info(um_iface_ptr, handle_ptr,
                                                  &addr_info, ps_errno);

          /*-----------------------------------------------------------------
            Start the unused addr timer if this is an external address
          -----------------------------------------------------------------*/
          if( (v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_EXTERNAL) &&
              (v6_addr_ptr->unused_addr_timer_handle != 0) )
          {
            (void)ps_timer_start( v6_addr_ptr->unused_addr_timer_handle,
                                  IPV6_EXT_ADDR_INTERVAL_VERIFY_TIME );
          }
          PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
          return 0;
        }

        event_info.priv_ipv6_addr.is_unique =
          (v6_addr_ptr->addr_type == IPV6_ADDR_TYPE_PRIV_UNIQUE) ? TRUE : FALSE;
        event_info.priv_ipv6_addr.ip_addr.type              = IPV6_ADDR;
        ps_ifacei_invoke_event_cbacks(um_iface_ptr,
                                      NULL,
                                      IFACE_IPV6_PRIV_ADDR_GENERATED_EV,
                                      event_info);
      }
      break;

    default:
      LOG_MSG_INFO2("ps_iface_addr_mgmt_ipv6_dad_update: "
              "ignoring dad_code %d.", dad_code, 0, 0);
      break;
  }
  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;
} /* ps_iface_addr_mgmt_ipv6_dad_update() */

/*===========================================================================
FUNCTION PS_IFACE_ADDR_MGMT_FREE_EXT_V6_ADDRESSES()

DESCRIPTION
  This function will delete all external IPv6 address from um_iface

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_addr_mgmt_free_ext_v6_addresses
(
  ps_iface_type *this_iface_ptr
)
{
  ps_iface_ipv6_addr_type_enum_type  addr_type;
  ps_ifacei_v6_addr_type            *v6_addr_ptr;
  struct  ps_in6_addr                ip6_addr;
  ps_iface_addr_mgmt_handle_type     handle;
  int16   ps_errno;
  uint8   idx;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2("Start",0,0,0);

  if(!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface ptr 0x%p", this_iface_ptr, 0, 0);
    return;
  }

  if(this_iface_ptr->group_mask == RM_GROUP)
  {
    LOG_MSG_ERROR("Deletion not allowed on Rmnet iface",0,0,0);
    return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    Get um iface from logical  iface chain.
  -------------------------------------------------------------------------*/
  this_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

  /*-------------------------------------------------------------------------
    Find and delete the IPv6 address.
  -------------------------------------------------------------------------*/
  for(idx = MAX_IPV6_PREFIXES; idx < MAX_IPV6_ADDRS; idx++)
  {
    if(this_iface_ptr->iface_private.ipv6_addrs[idx] == NULL)
    {
      break;
    }

    addr_type = this_iface_ptr->iface_private.ipv6_addrs[idx]->addr_type;

    if( IPV6_ADDR_TYPE_EXTERNAL == addr_type )
    {
      v6_addr_ptr = this_iface_ptr->iface_private.ipv6_addrs[idx];

      ip6_addr.ps_s6_addr64[0] = v6_addr_ptr->prefix;
      ip6_addr.ps_s6_addr64[1] = v6_addr_ptr->iid;

      handle = ps_iface_addr_mgmt_get_handle_from_ip(this_iface_ptr, &ip6_addr);
      if (-1 == handle)
      {
        LOG_MSG_ERROR("Could not get handle from IPv6 address", 0, 0, 0);
        continue;
      }

      if( -1 == ps_iface_addr_mgmt_free_addr(this_iface_ptr, 
                                             &handle,
                                             NULL, 
                                             &ps_errno) )
      {
        LOG_MSG_ERROR("Could not delete IPv6 address", 0, 0, 0);
        continue;
      }

      /*---------------------------------------------------------------------
        Because ps_iface_addr_mgmt_free_addr condenses the array, we need 
        to delete the address from same location, until we get NULL address.
      ---------------------------------------------------------------------*/
      idx--;
    }
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  LOG_MSG_INFO2("Finished",0,0,0);

} /* ps_iface_addr_mgmt_free_ext_v6_addresses() */

#endif /* FEATURE_DATA_PS_ADDR_MGMT */
#endif /* FEATURE_DATA_PS && FEATURE_DATA_PS_IPV6 */
