#ifndef PS_IFACE_ADDR_MGMT_H
#define PS_IFACE_ADDR_MGMT_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        P S _ I F A C E _ A D D R _ M G M T . H

GENERAL DESCRIPTION
  Interface IP Address Management Layer

EXTERNALIZED FUNCTIONS

INITIALIZATION AND SEQUENCING REQUIREMENTS

Copyright (c)2008-2010 by Qualcomm Technologies, Inc.  All Rights Reserved.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/


/*===========================================================================

                           EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datamodem/interface/netiface/rel/11.03/inc/ps_iface_addr_mgmt.h#1 $
  $DateTime: 2011/06/17 12:02:33 $

===========================================================================*/


/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"       /* Customer Specific Features */
#ifdef FEATURE_DATA_PS
#ifdef FEATURE_DATA_PS_ADDR_MGMT
#include "ps_iface.h"
#include "ps_ifacei_addr_v6.h"

/*===========================================================================

                         EXTERNAL DATA DECLARATIONS

===========================================================================*/

typedef enum
{
  PS_IFACE_ADDR_MGMT_MASK_MIN            = 0x0000,
  PS_IFACE_ADDR_MGMT_MASK_PREFIX         = 0x0001,
  PS_IFACE_ADDR_MGMT_MASK_PREFIX_LEN     = 0x0002,
  PS_IFACE_ADDR_MGMT_MASK_IID            = 0x0004,
  PS_IFACE_ADDR_MGMT_MASK_GATEWAY_IID    = 0x0008,
  PS_IFACE_ADDR_MGMT_MASK_PREF_LIFETIME  = 0x0010,
  PS_IFACE_ADDR_MGMT_MASK_VALID_LIFETIME = 0x0020,
  PS_IFACE_ADDR_MGMT_MASK_ADDR_STATE     = 0x0040,
  PS_IFACE_ADDR_MGMT_MASK_ADDR_TYPE      = 0x0080,
  PS_IFACE_ADDR_MGMT_MASK_REF_CNT        = 0x0100,
  PS_IFACE_ADDR_MGMT_MASK_INTERNAL_ONLY  = 0x01C0,
  PS_IFACE_ADDR_MGMT_MASK_DAD_RETRIES    = 0x0200,
  PS_IFACE_ADDR_MGMT_MASK_ALL            = 0x01FF 
} ps_iface_addr_mgmt_addr_mask_enum_type;


typedef enum 
{
  IFACE_ADDR_MGMT_DAD_SUCCESS        = 0,
  IFACE_ADDR_MGMT_DAD_DUP_ADDR       = 1,
  IFACE_ADDR_MGMT_DAD_NO_MEM         = 2,
  IFACE_ADDR_MGMT_DAD_NETWORK_REJECT = 3,
  IFACE_ADDR_MGMT_DAD_INTERNAL_ERR   = 4,
  IFACE_ADDR_MGMT_DAD_TIMEOUT        = 5
} ps_iface_addr_mgmt_dad_enum_type;

typedef struct
{
  ps_iface_addr_mgmt_addr_mask_enum_type addr_mask;
  uint64  prefix;
  uint64  iid;
  uint64  gateway_iid;
  uint32  pref_lifetime;
  uint32  valid_lifetime;
  uint8   prefix_len;
  uint8   dad_retries;
  
  ps_iface_ipv6_addr_state_enum_type  addr_state;
  ps_iface_ipv6_addr_type_enum_type   addr_type;
} ps_iface_addr_mgmt_addr_info_type;

typedef struct
{
  ps_ip_addr_type                   ip_addr;
  ps_iface_ipv6_addr_type_enum_type addr_type;
} ps_iface_addr_mgmt_alloc_type;

typedef void ps_iface_addr_mgmt_free_type;


/*---------------------------------------------------------------------------
  The time interval in which to verify external addresses to ensure they
  are still being used.
---------------------------------------------------------------------------*/
#define IPV6_EXT_ADDR_INTERVAL_VERIFY_TIME 300000
#define IPV6_EXT_ADDR_WAIT_TIME            60000

#define IPV6_MAX_ADDR_DAD_RETRIES          1


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                         PUBLIC FUNCTION DEFINITIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
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
);

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
);

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
);

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
  ps_iface_type       * iface_ptr,
  struct ps_in6_addr  * ip_addr_ptr
);

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
);

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
);


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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);
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
);


#endif /* FEATURE_DATA_PS_ADDR_MGMT */
#endif /* FEATURE_DATA_PS */
#endif /* PS_IFACE_ADDR_MGMT_H */
