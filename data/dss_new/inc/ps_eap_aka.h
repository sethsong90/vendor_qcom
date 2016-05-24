#ifndef PS_EAP_AKA_H
#define PS_EAP_AKA_H


/*===========================================================================

        E A P  A U T H E N T I C A T I O N   K E Y   A G R E E M E N T
                            
                            F O R  W L A N
                
                   
DESCRIPTION
  This file contains EAP-AKA processing functions.
     
    
Copyright (c) 2006-2009 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE


  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.


  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_eap_aka.h#1 $ 
  $DateTime: 2011/01/10 09:44:56 $ 
  $Author: maldinge $


when        who    what, where, why
--------    ---    ----------------------------------------------------------
12/19/08    pp      Common Modem Interface: Public/Private API split.
05/02/08    ifk     Moved EAP-AKA/SIM code to CDPS.
11/14/06    lti     Code review changes and restructuring
06/08/06    lti     Created module.

===========================================================================*/



/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/

#include "comdef.h"
#include "ps_eap.h"

/*===========================================================================

                     EXTERNAL FUNCTION DECLARATIONS

===========================================================================*/
/*===========================================================================
FUNCTION EAP_AKA_POWERUP_INIT

DESCRIPTION
  Power-up initialize the EAP-AKA software entity.

DEPENDENCIES
  None

PARAMETERS
  None.
  
RETURN VALUE
  None

SIDE EFFECTS
  Register for EAP-AKA packets over EAP transport.
  
===========================================================================*/
void 
eap_aka_powerup_init
(
  void
);


/*===========================================================================
FUNCTION EAP_AKA_INIT

DESCRIPTION
  Initialize the EAP-AKA software entity.

DEPENDENCIES
  None

PARAMETERS
  None.
  
RETURN VALUE
  None

SIDE EFFECTS
  Register for EAP-AKA packets over EAP transport.
  
===========================================================================*/
void 
eap_aka_init
(
  void
);

#endif /* PS_EAP_AKA_H */
