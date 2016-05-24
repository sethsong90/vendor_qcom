#ifndef PS_EAP_SIM_H
#define PS_EAP_SIM_H


/*===========================================================================

        E A P  S U B S C R I B E R  I D E N T I T Y  M O D U L E
                            
                            F O R  W L A N
                
                   
DESCRIPTION
  This file contains EAP-SIM processing functions.
     
    
Copyright (c) 2006-2009 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE


  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.


  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_eap_sim.h#1 $ 
  $DateTime: 2011/01/10 09:44:56 $ 
  $Author: maldinge $


when        who    what, where, why
--------    ---    ----------------------------------------------------------
12/19/08    pp      Common Modem Interface: Public/Private API split.
05/02/08    ifk     Moved EAP-AKA/SIM code to CDPS.
05/13/07    lti     Added header for sim/aka internals
02/26/09    lti     Moved internal function declarations to source file
11/14/06    lti     Code review changes and restructuring
06/08/06    lti     Created module.

===========================================================================*/



/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/

/*===========================================================================

               EAP SIM DATA DECLARATIONS AND DEFINITIONS

===========================================================================*/

/*===========================================================================

                     EXTERNAL FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================
FUNCTION EAP_SIM_INIT

DESCRIPTION
  Initialize the EAP-SIM software entity.

DEPENDENCIES
  None

PARAMETERS
  None.
RETURN VALUE
  None

SIDE EFFECTS
  Register for PEAP packets over EAP transport.
  
===========================================================================*/
void 
eap_sim_init
(
  void
);

#endif /* PS_EAP_SIM_H */
