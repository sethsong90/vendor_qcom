#ifndef PS_DNS_LOCAL_CACHE_MGR_H
#define PS_DNS_LOCAL_CACHE_MGR_H
/*===========================================================================

             P S _ D N S _ L O C A L _ C A C H E _ M G R . H

DESCRIPTION

  The Data Services DNS Subsystem Local Cache manager module. Contains
  definitions of functions, variables, macros, structs and enums
  used by the DNS Local Cache manager. This module is external to the 
  DNS subsystem but internal to AMSS.

EXTERNALIZED FUNCTIONS

  ps_dns_local_cache_mgr_init()
    Initializes the module

  ps_dns_local_cache_add()
    Adds an entry to the local cache

  ps_dns_local_cache_search()
    Searches the local cache to find a matching domain name

INTIALIZATIONS AND SEQUENCING REQUIREMENTS
  None

Copyright (c) 2007 by Qualcomm Technologies, Inc.  All Rights Reserved.
===========================================================================*/
/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dssvc/rel/11.03/inc/ps_dns_local_cache_mgr.h#1 $
  $DateTime: 2011/06/17 12:02:33 $ 
  $Author: zhasan $

===========================================================================*/
/*===========================================================================

                            INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"

#ifdef FEATURE_DATA_PS
#ifdef FEATURE_DATA_PS_DNS

#include "queue.h"
#include "ps_iface_ioctl.h"

/*===========================================================================

                         EXTERNAL FUNCTION DECLARATIONS

===========================================================================*/
/*===========================================================================
FUNCTION  PS_DNS_LOCAL_CACHE_MGR_INIT()

DESCRIPTION
  Initializes the module during powerup.

PARAMETERS
  None.

RETURN VALUE
  None.

DEPENDENCIES
  The Queue utils subsystem must be initialzed before this.

SIDE EFFECTS
  None.
===========================================================================*/
void ps_dns_local_cache_mgr_init
(
  void
);

/*===========================================================================
FUNCTION  PS_DNS_LOCAL_CACHE_ADD()

DESCRIPTION
  Adds an entry to the local cache

PARAMETERS
  domain_name_ptr (IN) - Pointer to the domain name string
  domain_name_len (IN) - Length of the string pointed to by domain_name_ptr
  ip_addr_ptr     (IN) - Pointer to the IP address structure
  ps_errno           (OUT) - Pointer to the error number

RETURN VALUE
   0 - Success
  -1 - An error occured. ps_errno should be examined for the specific error.


DEPENDENCIES
  None.

SIDE EFFECTS
  None.
===========================================================================*/
int32 ps_dns_local_cache_add
(
  const char         * domain_name_ptr,
  const uint16         domain_name_len,
  const ip_addr_type * ip_addr_ptr,
  int16              * ps_errno
);

/*===========================================================================
FUNCTION  PS_DNS_LOCAL_CACHE_SEARCH()

DESCRIPTION
  Searches the local cache to find a matching domain name

PARAMETERS
  domain_name_ptr (IN) - Pointer to the domain name string
  domain_name_len (IN) - Length of the string pointed to by domain_name_ptr
  ip_addr_ptr     (OUT) - Pointer to the IP address structure
  ps_errno           (OUT) - Pointer to the error number

RETURN VALUE
  None.

DEPENDENCIES
  None.

SIDE EFFECTS
  None.
===========================================================================*/
int32 ps_dns_local_cache_search
(
  const char   * domain_name_ptr,
  const uint16   domain_name_len,
  ip_addr_type * ip_addr_ptr,
  int16        * ps_errno
);
#endif  /* FEATURE_DATA_PS_DNS */
#endif  /* FEATURE_DATA_PS */
#endif /* PS_DNS_LOCAL_CACHE_MGR_H */
