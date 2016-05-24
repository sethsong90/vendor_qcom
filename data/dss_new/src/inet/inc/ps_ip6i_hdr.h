#ifndef PS_IP6I_HDR_H
#define PS_IP6I_HDR_H
/*===========================================================================

                         P S _ I  P 6 I _ H D R. H

DESCRIPTION
  Internet Protocol (IP) version 6 Internal header file (RFC 2460)

EXTERNALIZED FUNCTIONS
  ps_ip6_hdr_parse()  This function is used to process the next IP6 header in
                      the incoming IP6 packet and provides the parsed header 
                      as an output parameter.
                        
  
Copyright (c) 2008 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/


/*===========================================================================

                            EDIT HISTORY FOR FILE

$Header: //source/qcom/qct/modem/datamodem/protocols/inet/rel/11.03/inc/ps_ip6i_hdr.h#1 $
$Author: zhasan $ $DateTime: 2011/06/17 12:02:33 $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
12/14/08    pp     Created module as part of Common Modem Interface: 
                   Public/Private API split.
===========================================================================*/


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                                INCLUDE FILES

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
#include "comdef.h"
#include "customer.h"
#ifdef FEATURE_DATA_PS


#include "ps_ip6_hdr.h"

/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================

FUNCTION     PS_IP6_BASE_HDR_PARSE()

DESCRIPTION  This function is used to process the next IP6 header in the 
             incoming IP6 packet and provides the parsed header as an output 
             parameter.

DEPENDENCIES None

RETURN VALUE boolean - TRUE on success
                       FALSE on failure

SIDE EFFECTS None
===========================================================================*/
boolean ps_ip6_base_hdr_parse
(
  dsm_item_type     *data,
  uint16            *offset,
  ip6_hdr_type      *hdr
);

/*===========================================================================
FUNCTION     PS_IP6_FRAG_HDR_PARSE()

DESCRIPTION  This function is used to parse the IPv6 fragment header in the 
             incoming IP6 packet and returns it as an output parameter.

DEPENDENCIES None

RETURN VALUE boolean - TRUE on success
                       FALSE on failure

SIDE EFFECTS None
===========================================================================*/
boolean ps_ip6_frag_hdr_parse
(
  dsm_item_type *pkt_ptr,
  uint16        *offset,
  ip6_hdr_type  *hdr
);

#ifdef FEATURE_DATA_PS_MIPV6
/*===========================================================================

FUNCTION     PS_IP6_DEST_OPT_HDR_CREATE()

DESCRIPTION  This function is used to add a destination option header
             to the packet.
             
DEPENDENCIES None

RETURN VALUE TRUE on success
             FALSE on failure
                         
SIDE EFFECTS None
===========================================================================*/
boolean ps_ip6_dest_opt_hdr_create
(
  dsm_item_type         **dsm_ptr_ptr,
  ip6_dest_opt_hdr_type  *dest_opt_hdr
);
#endif /* FEATURE_DATA_PS_MIPV6 */


#endif /* FEATURE_DATA_PS */
#endif /* PS_IP6I_HDR_H */
