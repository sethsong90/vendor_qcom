#ifndef PS_PHYS_LINKI_H
#define PS_PHYS_LINKI_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                             P S _ P H Y S _ L I N K I . H

DESCRIPTION
  Header file defining all of the internal data types for the ps physical 
  link architecture.

EXTERNAL FUNCTIONS


INITIALIZATION AND SEQUENCING REQUIREMENTS
  None for the module.  Each phys_link is created by calling
  ps_phys_link_create().

Copyright (c) 2008 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/


/*===========================================================================

                            EDIT HISTORY FOR FILE

  $Header: //source/qcom/qct/modem/datamodem/interface/netiface/rel/11.03/inc/ps_phys_linki.h#1 $ $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
12/14/08    pp     Created module as part of Common Modem Interface: 
                   Public/Private API split.
===========================================================================*/


/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"

#ifdef FEATURE_DATA_PS


#include "queue.h"
#include "ps_phys_link.h"

/*===========================================================================
FUNCTION PS_PHYS_LINK_INST_FILL_DESC()

DESCRIPTION
  This function fills the description of the specified instance handle into
  a log packet.  If the handle matches the handle for all descriptions, it
  copies all of the active instance descriptions into the log packet.  If the
  handle does not specify all instances but specifies a valid active instance,
  the description associated with that instance is copied into the log packet.
  If the handle is invalid, nothing is done.

PARAMETERS
  handle          : Specifies which instance to retrieve the description from

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_phys_link_inst_fill_desc
(
  int32          handle
);


/*===========================================================================
FUNCTION PS_PHYS_LINK_TX_CMD()

DESCRIPTION

PARAMETERS
  this_phys_link_ptr:ptr to physlink control block on which to operate on.
  pkt_ref_ptr:       ref to dsm item ptr received for tx
  meta_info_ptr:     ptr to meta info (per pkt information)

RETURN VALUE
  0: if the packet was transmitted
 -1: if not

DEPENDENCIES
  None

SIDE EFFECTS
  The memory that was passed in will be freed by one of the tx functions.
===========================================================================*/
int ps_phys_link_tx_cmd
(
  ps_phys_link_type                *this_phys_link_ptr,
  dsm_item_type                    **pkt_ref_ptr,
  ps_phys_link_higher_layer_proto_enum_type  protocol,
  ps_tx_meta_info_type              *meta_info_ptr
);


#endif /* FEATURE_DATA_PS */
#endif /* PS_PHYS_LINK_H */
