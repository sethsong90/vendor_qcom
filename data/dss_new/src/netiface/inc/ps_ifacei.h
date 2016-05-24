#ifndef PS_IFACEI_H
#define PS_IFACEI_H

/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                             P S _ I F A C E I . H

DESCRIPTION
  Header file defining all of the internal data types

DEPENDENCIES
  None of these functions should EVER be called from Interrupt context!

Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/


/*===========================================================================

                            EDIT HISTORY FOR FILE

  $Header: //source/qcom/qct/modem/datamodem/interface/netiface/rel/11.03/inc/ps_ifacei.h#1 $
  $DateTime: 2011/06/17 12:02:33 $

  when      who    what, where, why
--------    ---    ----------------------------------------------------------
08/16/09    pp     ps_iface_fill_next_hop_addr API introduced.
12/14/08    pp     Created module as part of Common Modem Interface:
                   Public/Private API split.
===========================================================================*/


/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"

#include "ps_crit_sect.h"
#ifdef FEATURE_DATA_RM_NET
#include "ps_tx_meta_info.h"
#endif /* FEATURE_DATA_RM_NET */

#include "ds_Utils_DebugMsg.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================

                         INTERNAL DATA DECLARATIONS

===========================================================================*/

/*---------------------------------------------------------------------------
  The PS iface Authentication credentials type
---------------------------------------------------------------------------*/
#define PS_AUTH_MAX_USER_ID_LEN   (255)
#define PS_AUTH_MAX_PASSWORD_LEN  (255)
typedef struct
{
  uint8  user_id_len;
  char   user_id[PS_AUTH_MAX_USER_ID_LEN];
  uint8  password_len;
  char   password[PS_AUTH_MAX_PASSWORD_LEN];
} ps_iface_auth_credential_type;

typedef struct
{
  ps_iface_auth_alg_pref_bitmask_e_type  alg_pref;
  ps_iface_auth_credential_type          auth_creds;
  void *                                 mode_spec_auth_info_ptr;
} ps_iface_auth_info_type;

extern int softap_rmnet;
/*===========================================================================

                         EXTERNAL MACROS

===========================================================================*/
#define PS_IFACE_GET_INPUT_F_PTR(ps_iface_ptr)                           \
  (PS_IFACE_GET_BASE_IFACE(ps_iface_ptr)->iface_private.ip_input)

/*===========================================================================

                         EXTERNAL FUNCTIONS

===========================================================================*/
/*===========================================================================
FUNCTION PS_IFACE_INIT()

DESCRIPTION
  Initializes ps_iface module

DEPENDENCIES
  None

PARAMETERS
  None

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_init
(
  void
);

/*===========================================================================
FUNCTION PS_IFACE_EVENT_INIT()

DESCRIPTION
  This function initializes the global queues for both ps_iface and
  ps_phys_link.  It also registers the ps_iface phys_link function on the
  global phys_link queue.

  Note: Currently, only the events in ps_iface need initialization. Hence,
  the implementation of this function resides in ps_ifacei_event.c.

PARAMETERS
  None

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_event_init
(
  void
);

/*===========================================================================
FUNCTION PS_IFACE_GET_REF_CNT()

DESCRIPTION
  This macro returns the interface reference count

PARAMETERS
  iface_ptr: pointer to the interface in question.

RETURN VALUE
  the reference count for the passed in interface
===========================================================================*/
#define PS_IFACE_GET_REF_CNT  ps_iface_get_ref_cnt
INLINE uint32 ps_iface_get_ref_cnt
(
  ps_iface_type *iface_ptr
)
{
  uint32  ref_cnt = 0;
  uint8   index   = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (PS_IFACE_IS_VALID(iface_ptr))
  {
    PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

    for (index = 0; index < PS_POLICY_MGR_IFACE_PRIORITY_MAX; index++)
    {
      ref_cnt += iface_ptr->iface_private.ref_cnt[index];
    }

    PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  }

  return ref_cnt;

} /*  ps_iface_get_ref_cnt() */

/*===========================================================================
FUNCTION PS_IFACE_RESET_REF_CNT()

DESCRIPTION
  This macro resets the interface reference count to 0

PARAMETERS
  iface_ptr: pointer to the interface in question.

RETURN VALUE
  None
===========================================================================*/
#define PS_IFACE_RESET_REF_CNT  ps_iface_reset_ref_cnt
INLINE void ps_iface_reset_ref_cnt
(
  ps_iface_type *iface_ptr
)
{
  uint8   index   = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (PS_IFACE_IS_VALID(iface_ptr))
  {
    PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

    for (index = 0; index < PS_POLICY_MGR_IFACE_PRIORITY_MAX; index++)
    {
      iface_ptr->iface_private.ref_cnt[index] = 0;
      iface_ptr->iface_private.priority_mask = 0;
    }

    PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  }
} /*  ps_iface_reset_ref_cnt() */

/*===========================================================================
FUNCTION PS_IFACE_INC_SOCK_REF_CNT()

DESCRIPTION
  This function increments the interface sockets reference count

PARAMETERS
  iface_ptr: pointer to the interface in question.

RETURN VALUE
  None
===========================================================================*/
void ps_ifacei_inc_sock_ref_cnt
(
  ps_iface_type *iface_ptr
);

/*===========================================================================
FUNCTION PS_IFACEI_DEC_SOCK_REF_CNT()

DESCRIPTION
  This function decrements the interface sockets reference count
  In case it reaches zero value IFACE_IDLE_EV event is fired

PARAMETERS
  iface_ptr: pointer to the interface in question.

RETURN VALUE
  None
===========================================================================*/
void ps_ifacei_dec_sock_ref_cnt
(
  ps_iface_type *iface_ptr
);

/*===========================================================================
FUNCTION PS_IFACE_GET_APP_PRIORITY

DESCRIPTION
  Returns the first non-zero app priority field stored in an iface.

PARAMETERS
  iface_ptr - Pointer to ps iface control block.

RETURN VALUE
  app_priority stored in physical ifaces. In case of error, -1 is returned.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int32 ps_iface_get_app_priority
(
  ps_iface_type    *iface_ptr
);

/*===========================================================================
FUNCTION PS_IFACEI_GET_TX_FLTR_HANDLE_EX()

DESCRIPTION
  This function returns a handle which can be used to fetch Tx filters
  associated with a ps_flow independent of the IP version.

  Tx filters can pertain to either QOS_REQUEST/active QOS or pending
  QOS_MODIFY

PARAMETERS
  ps_iface_ptr : iface where filters are stored
  flow_ptr     : pointer to the ps_flow whose filters need to be fetched
  is_modify    : is the client interested in fetching Tx filters associated
                 with pending QOS_MODIFY? If so value must be TRUE

RETURN VALUE
  A handle using which Tx filters can be fetched : on success
  NULL                                           : otherwise

DEPENDENCIES
  All filters must be fetched in one atomic operation. Otherwise filters
  could be deleted (for example because QOS_MODIFY is accepted/rejected)
  while client is fetching them which could lead to incorrect operations

  Sample usage to fetch Tx filters pertaining to QOS_REQUEST is
    TAKLOCK()
      handle = ps_ifacei_get_tx_fltr_handle_ex(iface_ptr, flow_ptr, FALSE)
      while (handle != NULL)
      {
        ps_ifacei_get_tx_fltr_by_handle_ex(iface_ptr, flow_ptr, FALSE,
                                        handle, fltr, prcd, new_handle)
        handle = new_handle
      }
    TASKFREE();

SIDE EFFECTS
  None
===========================================================================*/
void *ps_ifacei_get_tx_fltr_handle_ex
(
  ps_iface_type         * ps_iface_ptr,
  ps_flow_type          * flow_ptr,
  boolean                 is_modify
);

/*===========================================================================
FUNCTION PS_IFACEI_GET_TX_FLTR_BY_HANDLE_EX()

DESCRIPTION
  Given a handle, this function returns a Tx filter, and its precedence
  independent of the IP version..
  Currently 0 is returned as precedence for all Tx filters.

  Tx filters can pertain to either QOS_REQUEST/active QOS or pending
  QOS_MODIFY

PARAMETERS
  ps_iface_ptr   : iface where filters are stored
  flow_ptr       : pointer to the flow
  tx_fltr_handle : handle using which a filter is fetched
  is_modify      : is the client interested in fetching Tx filters associated
                   with pending QOS_MODIFY? If so value must be TRUE
  fltr           : OUT PARAM, pointer to Tx filter
  prcd           : OUT PARAM, precedence of Tx filter
  next_handle    : OUT PARAM, new handle which can be used to fetch next
                   Tx filter

RETURN VALUE
  TRUE  : on success
  FALSE : otherwise

DEPENDENCIES
  All filters must be fetched in one atomic operation. Otherwise filters
  could be deleted (for example because QOS_MODIFY is accepted/rejected)
  while client is fetching them which could lead to incorrect operations.

  Do not try to access filter contents outside of atomic operation. Since
  a filter can be deleted, this may lead to memory corruption or data aborts.

  Sample usage to fetch Tx filters pertaining to QOS_REQUEST is
    TAKLOCK()
      handle = ps_ifacei_get_tx_fltr_handle_ex(iface_ptr, flow_ptr, FALSE)
      while (handle != NULL)
      {
        ps_ifacei_get_tx_fltr_by_handle_ex(iface_ptr, flow_ptr, FALSE,
                                       handle, fltr, prcd, new_handle)
        handle = new_handle
      }
    TASKFREE();

SIDE EFFECTS
  next_handle is set so that if this function is called with that handle,
  next Tx filter is fetched
===========================================================================*/
boolean ps_ifacei_get_tx_fltr_by_handle_ex
(
  ps_iface_type          * ps_iface_ptr,
  ps_flow_type           * flow_ptr,
  void                   * tx_fltr_handle,
  boolean                  is_modify,
  ip_filter_type        ** fltr,
  uint8                  * prcd,
  void                  ** next_handle
);

/*===========================================================================
FUNCTION PS_IFACEI_GET_TX_FLTR_CNT_EX()

DESCRIPTION
  Returns number of Tx filters, independent of the IP version, specified as an
  argument. Tx filters can pertain to either QOS_REQUEST/active QoS or
  pending QOS_MODIFY

PARAMETERS
  ps_iface_ptr : iface where filters are stored
  flow_ptr     : pointer to the ps_flow whose filters need to be fetched
  is_modify    : is the client interested in fetching Tx filters associated
                 with pending QOS_MODIFY? If so value must be TRUE

RETURN VALUE
  Number of Tx filters : on success
  -1                   : otherwise

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int8 ps_ifacei_get_tx_fltr_cnt_ex
(
  ps_iface_type         * ps_iface_ptr,
  ps_flow_type          * flow_ptr,
  boolean                 is_modify
);

/*===========================================================================
MACRO PS_IFACE_GET_BASE_IFACE()

DESCRIPTION
  If Inherit IP info of the iface is set then this macro drills down the
  chain of logical ifaces and return base iface, else it acts like NOOP
  returning the passed iface_ptr.

PARAMETERS
  iface_ptr:   any logical iface in a chain.

RETURN VALUE
  Same iface in case of error
  or base iface ptr of a logical iface chain.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
#define PS_IFACE_GET_BASE_IFACE ps_iface_get_base_iface
INLINE ps_iface_type* ps_iface_get_base_iface
(
  ps_iface_type        *iface_ptr
)
{
  ps_iface_type *base_iface_ptr;

  if (!PS_IFACE_IS_VALID(iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface 0x%p", iface_ptr, 0, 0);
    ASSERT(0);
    return iface_ptr;
  }

 /*--------------------------------------------------------------------------
   If this is a logical iface chain, go to the base iface.
  -------------------------------------------------------------------------*/
  base_iface_ptr = iface_ptr;
  while (PS_IFACEI_IP_INFO_IS_INHERITED(base_iface_ptr))
  {
    base_iface_ptr = PS_IFACEI_GET_ASSOC_IFACE(base_iface_ptr);
    if (!PS_IFACE_IS_VALID(base_iface_ptr))
    {
      LOG_MSG_ERROR("Invalid associated iface 0x%p", base_iface_ptr, 0, 0);
      return iface_ptr;
    }
  }

  return base_iface_ptr;
} /* ps_iface_get_base_iface */

/*===========================================================================
FUNCTION PS_IFACE_ENABLE_FIREWALL

DESCRIPTION
  This function enables the firewall on the interface.

PARAMETERS
  iface_ptr : ptr to interface control block on which to operate on.
  is_allowed: To allow/deny firewall rules

RETURN VALUE
  0 if successful
 -1 if fails

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
boolean ps_iface_enable_firewall
(
  ps_iface_type            *iface_ptr,
  boolean                  is_allowed
);

/*===========================================================================
FUNCTION PS_IFACE_DISABLE_FIREWALL

DESCRIPTION
  This function disables the firewall on the interface.

PARAMETERS
  iface_ptr : ptr to interface control block on which to operate on.

RETURN VALUE
  0 if successful
 -1 if fails

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
boolean ps_iface_disable_firewall
(
  ps_iface_type            *iface_ptr
);

/*===========================================================================
FUNCTION PS_IFACE_ADD_FIREWALL_RULE

DESCRIPTION
  This function adds the firewall rule on the interface.

PARAMETERS
  iface_ptr : ptr to interface control block on which to operate on.
  ip_filter_type : Ptr to the firewall rule

RETURN VALUE
  0 if successful
 -1 if fails

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
/*boolean ps_iface_add_firewall_rule
(
  ps_iface_type            *iface_ptr,
  ip_filter_type           *fltr_spec,
  uint32                   *handle
);
*/
/*===========================================================================
FUNCTION PS_IFACE_DELETE_FIREWALL_RULE

DESCRIPTION
  This function deleted the firewall rule on the interface.

PARAMETERS
  iface_ptr : ptr to interface control block on which to operate on.
  handle : Handle to the firewall rule

RETURN VALUE
  0 if successful
 -1 if fails

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
boolean ps_iface_delete_firewall_rule
(
  ps_iface_type            *iface_ptr,
  uint32                   handle
);

/*===========================================================================
FUNCTION PS_IFACE_GET_FIREWALL_RULE

DESCRIPTION
  This function retrieves the firewall rule on the interface based on 
  the handle provided

PARAMETERS
  iface_ptr : ptr to interface control block on which to operate on.
  fltr_spec : Filter spec
  handle : Handle to the firewall rule

RETURN VALUE
  0 if successful
 -1 if fails

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
boolean ps_iface_get_firewall_rule
(
  ps_iface_type            *iface_ptr,
  ip_filter_type           *fltr_spec,
  uint32                   handle
);

/*===========================================================================
FUNCTION PS_IFACE_GET_FIREWALL_TABLE

DESCRIPTION
  This function retrieves all the firewall rule on the interface

PARAMETERS
  iface_ptr : ptr to interface control block on which to operate on.
  fltr_spec : Filter spec
  num_fltrs: Number of filters to be retrived
  avail_num_fltrs : Actual no of filters installed
  handle : Handles to the firewall rules

RETURN VALUE
  0 if successful
 -1 if fails

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
boolean ps_iface_get_firewall_table
(
  ps_iface_type            *iface_ptr,
  ip_filter_type           *fltr_spec,
  uint8                    num_fltrs,
  uint8                    *avail_num_fltrs,
  uint32                   *handle
);

#ifdef FEATURE_DATA_RM_NET
/*===========================================================================
  FUNCTION PS_IFACE_FILL_NEXT_HOP_ADDR()

DESCRIPTION
  This function fills next hop address from either Pkt info or from iface
  pointer. Mainly used by Broadcast scenarios[Currently RmNet!]

  This function is introduced to avoid duplication in IFACE, IP Frag layers

DEPENDENCIES
  A valid Metainfo must be passed in which contain pkt info.
  A valid iface pointer must be passed in used in non-mcast scenarios.

PARAMETERS
  iface_ptr   - Iface pointer
  mi_ref_ptr  - Reference to meta info block
  errno       - Error value returned by the function

RETURN VALUE
  0 for successful processing of the pkt, -1 in case of error

  Currently this function is used only for RmNet cases - can be opened for
  all - if required!
===========================================================================*/
int ps_iface_fill_next_hop_addr
(
  ps_iface_type                        *iface_ptr,
  ps_tx_meta_info_type                 *meta_info_ptr,
  errno_enum_type                      *ps_errno
);
#endif /* FEATURE_DATA_RM_NET */

#ifdef FEATURE_DATACOMMON_PS_IFACE_IO
int ps_ifacei_default_tx_cmd
(
  ps_iface_type        *this_iface_ptr,
  dsm_item_type       **pkt_ref_ptr,
  ps_tx_meta_info_type *meta_info_ptr,
  void                 *tx_cmd_info
);

int ps_ifacei_logical_default_tx_cmd
(
  ps_iface_type         *this_iface_ptr,
  dsm_item_type        **pkt_ref_ptr,
  ps_tx_meta_info_type  *meta_info_ptr,
  void                  *tx_cmd_info
);
#endif /* FEATURE_DATACOMMON_PS_IFACE_IO */

#ifdef __cplusplus
}
#endif

#endif /* PS_IFACEI_H */
