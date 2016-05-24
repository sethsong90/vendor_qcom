#ifndef PS_MMGSDI_CLIENT_H
#define PS_MMGSDI_CLIENT_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

  P S  M M G S D I  C L I E N T  H E A D E R  F I L E

GENERAL DESCRIPTION
  Acts as a client to MMGSDI and registers for RUIM card events.

Copyright (c) 2008 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/


/*===========================================================================

                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/protocols/inet/rel/11.03/inc/ps_mmgsdi_client.h#1 $ 
  $Author: zhasan $
  $DateTime: 2011/06/17 12:02:33 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
09/04/08    am     Created.

===========================================================================*/


/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"
#ifdef FEATURE_DATA_PS

/*= = = = = = = =  = = = = = = = = = =  = = =  = = = = = = = = = = = = = = =
                    EXTERNAL FUNCTION DEFINTIONS
= = = = = = = = = = = = = = =  = = = = =  = = = = = = = = = = = = = = = = =*/
/*===========================================================================
FUNCTION PS_MMGSDI_CLIENT_REG_INIT()

DESCRIPTION
  This function registers PS as client to the MMGSDI module.

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.
===========================================================================*/
void ps_mmgsdi_client_reg_init
(
  void
);

#endif /* FEATURE_DATA_PS */
#endif /* PS_MMGSDI_CLIENT_H */

