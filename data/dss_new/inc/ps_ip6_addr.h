#ifndef PS_IP6_ADDR_H
#define PS_IP6_ADDR_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        P S _ I P 6 _ A D D R . H

GENERAL DESCRIPTION
 Internet Protocol Version 6 - Interface Layer Functionality

EXTERNALIZED FUNCTIONS

INITIALIZATION AND SEQUENCING REQUIREMENTS

Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/


/*===========================================================================

                           EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_ip6_addr.h#1 $
  $DateTime: 2011/01/10 09:44:56 $

===========================================================================*/

/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "ps_in.h"

#define DEFAULT_IP6_DATA_UNIT 1280 /* Minimum MTU for IPv6                 */

/*---------------------------------------------------------------------------
TYPEDEF PS_IPV6_IID_PARAMS_TYPE

DESCRIPTION
  Parameters for generating specific types of privacy addresses.Other params
  such as prefix to generate the addr with and IPsec options can also be
  added to this structure as needed.
---------------------------------------------------------------------------*/
typedef struct
{
  boolean app_request;            /* Application request                   */
  boolean is_unique;              /* Request a unique address or shareable */
  /* Other params such as prefix to generate the addr with and IPsec options
     Can also be added to this structure. */

} ps_ipv6_iid_params_type;


#ifdef __cplusplus
extern "C" {
#endif
/*= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

                        PUBLIC FUNCTION DECLARATIONS

= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/
/*===========================================================================
MACRO PS_IFACE_IPV6_PRIV_ADDR_INC_REF_CNT()

DESCRIPTION
  This macro increments the ref count of a private IPv6 IID.

PARAMETERS
  ip_addr_ptr:     The IPv6 address

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
);


/*===========================================================================
MACRO PS_IFACE_IPV6_PRIV_ADDR_DEC_REF_CNT()

DESCRIPTION
  This function decrements the ref count of a private IPv6 address. If the
  reference count goes to 0 the privacy address is deleted.

PARAMETERS
  ip_addr_ptr:     The IPv6 address

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
);

/*===========================================================================
MACRO IN6_ARE_PREFIX_EQUAL()
===========================================================================*/
#define IN6_ARE_PREFIX_EQUAL in6_are_prefix_equal
INLINE boolean in6_are_prefix_equal
(
  struct ps_in6_addr *addr1,
  struct ps_in6_addr *addr2,
  uint32              prefix_len
)
{
  uint32 i;
  uint32 mask;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == prefix_len || 128 < prefix_len)
  {
    return FALSE;
  }

  for (i=0; i<(prefix_len/32); i++)
  {
    if (addr2->ps_s6_addr32[i] != addr1->ps_s6_addr32[i])
    {
      return FALSE;
    }
  }

  if (prefix_len%32)
  {
    mask = ~((uint32)((1<<(32-(prefix_len%32)))-1));
    mask = ps_htonl(mask);
    if ((addr1->ps_s6_addr32[i] & mask) != (addr2->ps_s6_addr32[i] & mask))
    {
      return FALSE;
    }
  }

  return TRUE;
} /* in6_are_prefix_equal() */

#ifdef __cplusplus
}
#endif

#endif /* PS_IP6_ADDR_H */
