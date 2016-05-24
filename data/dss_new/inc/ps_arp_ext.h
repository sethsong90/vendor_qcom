#ifndef PS_ARP_EXT_H
#define PS_ARP_EXT_H
/*===========================================================================
                               P S _ A R P _ E X T . H

DESCRIPTION
  Header file for the PS ARP protocol suite External Interfacing functions.

Copyright (c) 2010 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                            EDIT HISTORY FOR FILE

           $Author: maldinge $ $DateTime: 2011/01/10 09:44:56 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
07/02/10    cp     Defined separate interfaces to update and clear the 
                   entries from the ARP cache.
06/08/10    cp     Made arp update ipv4 cache API CMI compliant. 
                   (Moved from ps_arp.h to API) 

===========================================================================*/

/*===========================================================================

                                INCLUDE FILES

===========================================================================*/
#include "comdef.h"
#include "ps_iface.h"

/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/
/*===========================================================================
FUNCTION ARP_EXT_IPV4_CACHE_UPDATE()

DESCRIPTION
  This function is called by DHCP clients to update ARP entries related
  to a particular LLE. If there is no LLE associated with
  this PS_IFACE, no action is taken.

PARAMETERS
    ifaceptr: PS_IFACE for which an entry is been updated.
    ip_addr: IP address entry to update
    hw_addr: Hardware address with which entry should be updated.

RETURN VALUE
 None.

DEPENDENCIES
  arp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void arp_ext_ipv4_cache_update
(
  ps_iface_type*  iface_ptr,
  uint32          ip_addr,
  byte*           hw_addr
);

/*===========================================================================
FUNCTION ARP_EXT_IPV4_CACHE_CLEAR()

DESCRIPTION
  This function is called by DHCP clients to clear ARP entries related
  to a particular LLE. If there is no LLE associated with
  this PS_IFACE, no action is taken.

PARAMETERS
    ifaceptr: PS_IFACE for which an entry is been cleared.
    ip_addr:  IP address of the entry to clear.
    hw_addr:  Hardware address of the entry to clear.

RETURN VALUE
 None.

DEPENDENCIES
  arp_init should have been called.

SIDE EFFECTS
  None
===========================================================================*/
void arp_ext_ipv4_cache_clear
(
  ps_iface_type*  iface_ptr,
  uint32          ip_addr,
  byte*           hw_addr
);

#endif /* PS_ARP_EXT_H */

