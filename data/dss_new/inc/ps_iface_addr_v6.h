#ifndef PS_IFACE_ADDR_V6_H
#define PS_IFACE_ADDR_V6_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        P S _ I F A C E _ A D D R _ V 6 . H

GENERAL DESCRIPTION
 Internet Protocol Version 6 - Interface Layer Functionality

EXTERNALIZED FUNCTIONS

INITIALIZATION AND SEQUENCING REQUIREMENTS

Copyright (c) 2007-2009 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/


/*===========================================================================

                           EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_iface_addr_v6.h#1 $
  $DateTime: 2011/01/10 09:44:56 $

===========================================================================*/


/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "pstimer.h"
#include "ps_iface_defs.h"

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACEI_V6_ADDR_TYPE

DESCRIPTION
  the internal PS Iface data structure that is used for storing the PS Iface
  Addresses.
---------------------------------------------------------------------------*/
typedef struct
{
  uint64  prefix;
  uint64  iid;
  uint64  gateway_iid;             /* Gateway from which prefix acquired   */
  ps_timer_handle_type  pref_lifetimer_handle;       /* Preferred lifetime */
  ps_timer_handle_type  valid_lifetimer_handle;         /*  Valid lifetime */
  ps_timer_handle_type  unused_addr_timer_handle; /* Priv addr unused timer.
                                                     If no app binds before
                                                     expir, addr is freed  */
  ps_iface_ipv6_addr_state_enum_type  addr_state;
  ps_iface_ipv6_addr_type_enum_type   addr_type;
  uint8   ref_cnt;                   /* Privacy Extensions reference count */
  uint8   prefix_len;                              /* Length of the prefix */
  uint8   dad_retries;                 /* Number of DAD attempts remaining */

  ps_iface_type *rm_iface_ptr;          /* ptr to rm iface */
} ps_ifacei_v6_addr_type;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                         PUBLIC MACRO DEFINITIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/*===========================================================================
MACRO IPV6_ADDR_MSG()

DESCRIPTION
  This macro prints an IPV6 address to F3.

PARAMETERS
  ip_addr: The IPV6 address in network byte order.

RETURN VALUE
  none
===========================================================================*/
#define IPV6_ADDR_MSG(ip_addr) MSG_8(MSG_SSID_DS, \
                        MSG_LEGACY_HIGH, \
                        "IPV6 Address %x:%x:%x:%x:%x:%x:%x:%x", \
                        (uint16)(ps_ntohs(ip_addr[0])), \
                        (uint16)(ps_ntohs(ip_addr[0] >> 16)), \
                        (uint16)(ps_ntohs(ip_addr[0] >> 32)) , \
                        (uint16)(ps_ntohs(ip_addr[0] >> 48)), \
                        (uint16)(ps_ntohs(ip_addr[1])), \
                        (uint16)(ps_ntohs(ip_addr[1] >> 16)), \
                        (uint16)(ps_ntohs(ip_addr[1] >> 32)) , \
                        (uint16)(ps_ntohs(ip_addr[1] >> 48)))

/*===========================================================================
MACRO IPV6_IID_MSG

DESCRIPTION
  This macro prints an IPV6 IID.

PARAMETERS
  iid: The IPV6 iid in network byte order.

RETURN VALUE
  none
===========================================================================*/
#define IPV6_IID_MSG(iid) MSG_4( MSG_SSID_DS, \
                                 MSG_LEGACY_HIGH, \
                                 "IID is %x:%x:%x:%x", \
                                 (uint16)(ps_ntohs(iid)), \
                                 (uint16)(ps_ntohs(iid >> 16)), \
                                 (uint16)(ps_ntohs(iid >> 32)) , \
                                 (uint16)(ps_ntohs(iid >> 48)))


/*===========================================================================
MACRO PS_IFACE_GET_IP_V6_IID()

DESCRIPTION
  This macro returns the IPv6 interface identifier (the v6 address suffix)
assigned to the interface (this is NOT the same as ps_iface).

PARAMETERS
  iface_ptr: pointer to the ps_iface interface in question.

RETURN VALUE
  IPv6 interface identifier (last 64 bits of the address), 0 if interface
pointer is NULL
===========================================================================*/
#define PS_IFACE_GET_IP_V6_IID( iface_ptr )                     \
ps_iface_get_v6_iid(iface_ptr)

/*===========================================================================
MACRO PS_IFACE_GET_IP_V6_PREFIX()

DESCRIPTION
  This macro returns the IPv6 prefix (the v6 address prefix)
assigned to the interface (this is NOT the same as ps_iface).

PARAMETERS
  iface_ptr: pointer to the ps_iface interface in question.

RETURN VALUE
  IPv6 address prefix (first 64 bits of the address), 0 if interface
pointer is NULL
===========================================================================*/
#define PS_IFACE_GET_IP_V6_PREFIX( iface_ptr )                     \
ps_iface_get_v6_prefix(iface_ptr)

/*===========================================================================
MACRO PS_IFACE_V6_ADDR_MATCH

DESCRIPTION
  This macro matches the passed IPv6 address with the possible IPv6
  addresses of the passed interface.

PARAMETERS
  v6_addr_ptr - Ptr to IPv6 address to match.
  if_ptr      - Interface pointer.

RETURN VALUE
  TRUE  - If the passed address matches any of the IPv6 addr's of the iface.
  FALSE - Otherwise.

DEPENDENCIES
  None.

SIDE EFFECTS
  None
===========================================================================*/
#define PS_IFACE_V6_ADDR_MATCH( v6_addr_ptr, if_ptr )              \
ps_iface_v6_addr_match( v6_addr_ptr, if_ptr)

/*===========================================================================
FUNCTION PS_IFACE_SET_IP_V6_IID()

DESCRIPTION
  This macro sets the IPv6 interface identifier (the v6 address suffix) in
the interface.

PARAMETERS
  iface_ptr: pointer to the interface in question.
  iid:       64-bit IPv6 interface identifier (the v6 address suffix)

RETURN VALUE
  FALSE if interface pointer is NULL, TRUE otherwise
===========================================================================*/
#define PS_IFACE_SET_IP_V6_IID( iface_ptr, iid )                        \
 ps_iface_set_v6_iid(iface_ptr,iid)


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                         PUBLIC FUNCTION DEFINITIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

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
);

/*===========================================================================
FUNCTION PS_IFACE_APPLY_V6_PREFIX()

DESCRIPTION
  This function will apply a prefix to a particular interface.  In it's
  initial incarnation it will only store a single prefix - the only way to
  write a new prefix is to .  In future a more
  sophisticated method will be used to store prefixes.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  gateway_iid:  iid of the router
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
);

/*===========================================================================
FUNCTION PS_IFACE_REMOVE_V6_PREFIX()

DESCRIPTION
  This function will remove a prefix from the interface.  It will only fail if
  the prefix doesn't exist on this interface.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
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
);

/*===========================================================================
FUNCTION PS_IFACE_DELETE_ALL_V6_PREFIXES()


DESCRIPTION
  This function will remove all prefix associated with the interface.


PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.


RETURN VALUE
  0 on success
 -1 on failure


DEPENDENCIES
  None


SIDE EFFECTS
  Deletes all V6 prefixes.
===========================================================================*/
int ps_iface_delete_all_v6_prefixes
(
  ps_iface_type *this_iface_ptr
);

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
  None

DEPENDENCIES

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_generate_ipv6_iid
(
  ps_iface_type *this_iface_ptr, /* Pointer to the interface to operate on */
  uint64        *iid,            /* Pointer to interface ID to be returned */
  int16         *ps_errno
);

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
);

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
);

/*===========================================================================
FUNCTION PS_IFACE_GET_LINKLOCAL_ADDR()

DESCRIPTION
  This function is used to get the link local address of the V6 interface.
  The function sets the addr type to invalid if the call fails.

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
  ps_iface_type    *this_iface_ptr,
  ps_ip_addr_type  *ip_addr_ptr
);

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
);
/*===========================================================================
FUNCTION PS_IFACE_SET_V6_IID

DESCRIPTION
  This function sets the IPV6 IID of an iface.  If the iface is
  NULL or IPV4, then it returns FALSE.

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
);

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
);

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
);

#endif /* PS_IFACE_ADDR_V6_H */
