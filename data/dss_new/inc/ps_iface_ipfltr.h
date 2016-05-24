#ifndef PS_IFACE_IPFLTR_H
#define PS_IFACE_IPFLTR_H

/*===========================================================================

                          P S _ I F A C E _ I P F L T R . H

DESCRIPTION
  Header file for declaration of functions to use IP filters for inbound
  iface specific processing of IP packets received on that iface.

EXTERNALIZED FUNCTIONS

  PS_IFACE_IPFLTR_ADD()
    adds filters to the iface

  PS_IFACE_IPFLTR_DELETE()
    deletes filters from the iface

  PS_IFACE_IPFLTR_CONTROL()
    Performs control operation on filters (enable/disable)

  PS_IFACE_IPFLTR_EXECUTE()
    executes filter for the processing inbound IP pkts

  PS_IFACE_IPFLTR_PROCESS_PKT
    Process a raw IP pkt and then performs filtering on the packet

  PS_IFACE_IPFLTR_GET_QUEUED_FRAGMENTS
    Passed a fraghdl returned by ps_iface_ipfltr_process_pkt(), returns
    Fragments queued in the passed fraghdl datastructure

Copyright (c) 2003-2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_iface_ipfltr.h#1 $
  $DateTime: 2006/10/02 16:26:28
 
  when        who    what, where, why
  --------    ---    -------------------------------------------------------- 
  05/20/10    ss     Added pointer to a2 filter handler to ipfiltri_type
  05/14/10    guru   Added new function ps_iface_ipfltr_is_handle_enabled  
===========================================================================*/

#include "comdef.h"
#include "queue.h"
#include "ps_iface_defs.h"
#include "ps_ipfltr_defs.h"
#include "ps_pkt_info.h"
#ifdef FEATURE_DSS_LINUX
#include "ds_sl_list.h"
#else
#include "list.h"
#endif


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                          REGIONAL DATA DECLARATIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_IPFLTR_HANDLE_TYPE
  Client specified 4 byte IP filter handle.
---------------------------------------------------------------------------*/
typedef uint32 ps_iface_ipfltr_handle_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_IPFLTR_RESULT_TYPE
  Client specified 4 byte IP filter result. Returned when a fltr matches
  given pkt.
---------------------------------------------------------------------------*/
typedef uint32 ps_iface_ipfltr_result_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_IPFLTR_SUBSET_ID_TYPE
  Client specified 4 byte IP filter subset id.
---------------------------------------------------------------------------*/
typedef uint32 ps_iface_ipfltr_subset_id_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_RX_QOS_FLTR_HANDLE_TYPE
---------------------------------------------------------------------------*/
typedef uint32 ps_iface_rx_qos_fltr_handle_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_A2_IPFLTR_RULE_SET_HANDLE_T
---------------------------------------------------------------------------*/
typedef void* ps_a2_ipfltr_rule_set_handle_t;

/*---------------------------------------------------------------------------
PS_IFACE_IPFLTR_INVALID_HANDLE - handle which does not identify any filter
---------------------------------------------------------------------------*/
#define PS_IFACE_IPFLTR_INVALID_HANDLE  0x0

/*---------------------------------------------------------------------------
PS_IFACE_IPFLTR_NOMATCH - Result returned when pkt doesn't match any filter
---------------------------------------------------------------------------*/
#define PS_IFACE_IPFLTR_NOMATCH  0x0

/*---------------------------------------------------------------------------
PS_IFACE_IPFLTR_SUBSET_ID_DEFAULT
  Subset ID indicating that all filters in a queue must be executed to match
  a packet
---------------------------------------------------------------------------*/
#define PS_IFACE_IPFLTR_SUBSET_ID_DEFAULT  0x0

/*---------------------------------------------------------------------------
TYPEDEF FLTR_PRIORITY_ENUM_TYPE

DESCRIPTION
  Defines various priorities in which a filter can be added to existing
  filters
---------------------------------------------------------------------------*/
typedef enum
{
  PS_IFACE_IPFLTR_PRIORITY_MIN     = 0,
  PS_IFACE_IPFLTR_PRIORITY_FCFS    = 0,
  PS_IFACE_IPFLTR_PRIORITY_CUSTOM  = 1,
  PS_IFACE_IPFLTR_PRIORITY_DEFAULT = PS_IFACE_IPFLTR_PRIORITY_FCFS,
  PS_IFACE_IPFLTR_PRIORITY_MAX
} fltr_priority_enum_type;

/*---------------------------------------------------------------------------
TYPEDEF FLTR_COMPARISON_F_PTR_TYPE

DESCRIPTION
  Function prototype to compare two filter sets to compute precedence.

PARAMETERS
  fi_one_ptr: Ptr to first filter set to compare
  fi_two_ptr: Ptr to second filter set to compare

RETURN VALUE
  TRUE  - Filter1 has higher precedence than filter2
  FALSE - Filter2 has higher precedence than filter1
---------------------------------------------------------------------------*/
typedef boolean (*ipfltr_comparison_f_ptr_type)
(
  ip_filter_type                 *fi_one_ptr,
  ip_filter_type                 *fi_two_ptr
);

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_IPFLTR_ADD_PARAM_TYPE

DESCRIPTION
  Structure containing all the necessary information to add filters to an
  iface
---------------------------------------------------------------------------*/
typedef struct
{
  void                            * fi_ptr_arr;
  ps_iface_ipfltr_result_type       fi_result;
  ps_iface_ipfltr_subset_id_type    subset_id;
  ipfltr_comparison_f_ptr_type      fltr_compare_f_ptr;
  fltr_priority_enum_type           fltr_priority;
  ipfltr_type_enum_type             filter_type;
  uint8                             num_filters;
  boolean                           enable;
  boolean                           is_validated;
} ps_iface_ipfltr_add_param_type;

/*---------------------------------------------------------------------------
  Definition of data buffer used to store filters in iface control block
---------------------------------------------------------------------------*/
typedef struct
{
  q_link_type                     link;         /* Link to enqueue filter  */
  ipfltr_type_enum_type           filter_type;  /* Type of this filter     */
  ps_a2_ipfltr_rule_set_handle_t  a2_ruleset;   /* pointer to A2 ruleset   */

  union
  {
    ip_filter_type                fi_default;   /* Default filter          */
  } filter;

  ps_iface_ipfltr_handle_type     fi_handle;  /* Client specific filter id */
  ps_iface_ipfltr_result_type     fi_result;  /* Result sent when fltr match*/
  ps_iface_ipfltr_subset_id_type  subset_id;  /* Subset ID of this filter  */
  boolean                         disabled;   /* Is disabled for execution */
} ps_iface_ipfilteri_type;


#ifdef __cplusplus
extern "C" {
#endif
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                           EXTERNAL FUNCTION DECLARATIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
/*===========================================================================
FUNCTION PS_IFACE_IPFLTR_INIT()

DESCRIPTION
  Initialize the global IP filter queue.

DEPENDENCIES
  None

PARAMETERS
  None.

RETURN VALUE
  None

SIDE EFFECTS
  None.

===========================================================================*/
void ps_iface_ipfltr_init
(
 void
);

/*===========================================================================
FUNCTION PS_IFACE_IPFLTR_ADD()

DESCRIPTION
  This function adds a set of IP filters to the specified iface. The
  filter set is tied to a filter handle, which uniquely idenifies a set of
  filters added by the given client for the given iface. The filter handle
  is used to manipulate the filter set. A client needs to provide a filter
  result which is returned when a filter successfully match during the filter
  execution.
  If filters are added in disabled state they won't be executed until
  they are enabled.

PARAMETERS
  iface_ptr          : Iface to add filters to
  client_id          : Filtering client id
  fltr_add_param_ptr : ptr to structure containing all the necessary info to
                       add filters to an iface
  ps_errno           : error returned to the caller if operation fails

RETURN VALUE
  A handle to filters            : on success
  PS_IFACE_IPFLTR_INVALID_HANDLE : on failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
ps_iface_ipfltr_handle_type ps_iface_ipfltr_add
(
  ps_iface_type                         * iface_ptr,
  ps_iface_ipfltr_client_id_enum_type     client_id,
  const ps_iface_ipfltr_add_param_type  * fltr_add_param_ptr,
  int16                                 * ps_errno
);

/*===========================================================================
FUNCTION PS_IFACE_IPFLTR_DELETE()

DESCRIPTION
  This function deletes all the existing IP filter rules for the specified
  filter handle for a given client from the specified iface.

DEPENDENCIES
  None

PARAMETERS
  iface_ptr : Iface to delete filters from
  client_id : Filtering client id
  fi_handle : filter handle associated with the filter set which was added
              on this iface by this client.
  ps_errno  : error returned to the caller if operation fails

RETURN VALUE
   0 : on success
  -1 : on failure

SIDE EFFECTS
  Some packets may not get filtered once these filters are deleted
===========================================================================*/
int ps_iface_ipfltr_delete
(
  ps_iface_type                        * iface_ptr,
  ps_iface_ipfltr_client_id_enum_type    client_id,
  ps_iface_ipfltr_handle_type            fi_handle,
  int16                                * ps_errno
);

/*===========================================================================
FUNCTION PS_IFACE_IPFLTR_IS_HANDLE_ENABLED()

DESCRIPTION
  This function returnns if the filters associated with a particular
  fltr_handle are enabled.

DEPENDENCIES
  None

PARAMETERS
  iface_ptr      : Iface on which filters are added
  client_id      : Filtering client id
  fi_handle      : filter handle which identifies a specific filter set 
                   added on this iface by this client.
  is_enabled_ptr : Whether installed filters are enabled or disabled
  ps_errno       : error returned to the caller if operation fails

RETURN VALUE
   0 : on success
  -1 : on failure

SIDE EFFECTS
  None
===========================================================================*/
int32 ps_iface_ipfltr_is_handle_enabled
(
  ps_iface_type                       * iface_ptr,
  ps_iface_ipfltr_client_id_enum_type   client_id,
  ps_iface_ipfltr_handle_type           fi_handle,
  boolean                             * is_enabled_ptr,
  int16                               * ps_errno
);

/*===========================================================================
FUNCTION PS_IFACE_IPFLTR_CONTROL()

DESCRIPTION
  This function allows a control operation on all IP filters currently
  associated with the specified filter handle of the client for the iface.
  Only operation supported for now is disabling or enabling the filter
  execution state. Disabled filters are not used during filter execution.

DEPENDENCIES
  None

PARAMETERS
  iface_ptr : Iface on which filters are added
  client_id : Filtering client id
  fi_handle : filter handle which identifies a specific filter set added on
              this iface by this client.
  enable    : Whether to enable or disable already installed filters
  ps_errno  : error returned to the caller if operation fails

RETURN VALUE
   0 : on success
  -1 : on failure

SIDE EFFECTS
  Some packets may not get filtered once a filter set is disabled and they
  may get filtered if a filter set is enabled
===========================================================================*/
int ps_iface_ipfltr_control
(
  ps_iface_type                       *iface_ptr,
  ps_iface_ipfltr_client_id_enum_type client_id,
  ps_iface_ipfltr_handle_type         fi_handle,
  boolean                             enable,
  int16                               *ps_errno
);

/*===========================================================================
FUNCTION PS_IFACE_IPFLTR_EXECUTE()

DESCRIPTION
  This function executes all the filters in an iface to see if any of
  those passes criteria specified in the information passed in. Processing
  is done until the first match is found and filter id associated
  with that filter is returned. For DEFAULT filter type, each field set in
  the filter is compared against the input info using a fixed set of rules.
  For ACL type filters, the ACL function is called which can contain more
  complex and variant types of rules.

DEPENDENCIES
  None

PARAMETERS
  iface_ptr          - Iface to pick filters from for execution
  client_id          - Filtering client id
  subset_id          - ID which identified a subset of all filters installed
                       on iface. Only these filters are considered for
                       matching a packet
  ip_filter_info_ptr - Ptr to IP pkt information block to apply filter on

RETURN VALUE
  filter id associated with the filter : on a successful filter match
  PS_IFACE_IPFLTR_NOMATCH              : for no match

SIDE EFFECTS
  None
===========================================================================*/
ps_iface_ipfltr_result_type ps_iface_ipfltr_execute
(
  ps_iface_type                       *iface_ptr,
  ps_iface_ipfltr_client_id_enum_type client_id,
  ps_iface_ipfltr_subset_id_type      subset_id,
  ip_pkt_info_type                    *ip_filter_info_ptr
);

/*===========================================================================
FUNCTION PS_IFACE_IPFLTR_VALIDATE_FLTR_PARAM()

DESCRIPTION
  Validates parameters of a filter

PARAMETERS
  client_id : Filtering client id
  fltr_arr  : Array of ptr to filters
  num_fltr  : Number of filters in above array

RETURN VALUE
  TRUE  : if parameters are valid
  FALSE : otherwise

DEPENDENCIES
  None

SIDE EFFECTS
  the appropriate error masks will be set with the appropriate error codes
  for values and enums which failed validation. Because variables are only
  validated if the enums are set in the field masks the only error masks
  that will be updated will be those corresponding to the values set within
  the field masks.
===========================================================================*/
boolean ps_iface_ipfltr_validate_fltr_param
(
  ps_iface_ipfltr_client_id_enum_type  client_id,
  ip_filter_type                       fltr_arr[],
  uint8                                num_fltr
);

#ifdef __cplusplus
}
#endif

#endif /* PS_IFACE_IPFLTR_H */
