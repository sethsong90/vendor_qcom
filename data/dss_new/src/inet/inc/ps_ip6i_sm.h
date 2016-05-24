#ifndef PS_IP6I_SM
#define PS_IP6I_SM
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                            P S _ I P 6 I _ S M . H

DESCRIPTION
  Internal Header file defining the API for the IPv6 state machine. 

DEPENDENCIES
  The module MUST execute in the PS context.

EXTERNALIZED FUNCTIONS
  ip6_sm_powerup_init
  ip6_sm_init
    
Copyright (c) 2009 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                            EDIT HISTORY FOR FILE

  $Header: //source/qcom/qct/modem/datamodem/protocols/inet/rel/11.03/inc/ps_ip6i_sm.h#1 $
  $Author: zhasan $ $DateTime: 2011/06/17 12:02:33 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
05/07/09    pp     Initial release. Created as part of CMI Phase-4: SU Level 
                   API Effort [Split from ps_ip6_sm.h].
===========================================================================*/

/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#if defined (FEATURE_DATA_PS) && defined (FEATURE_DATA_PS_IPV6)
/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================
FUNCTION IP6_SM_POWERUP_INIT()

DESCRIPTION
  This function associates the SM with the task that initializes it, and
  registers the command handling functions.

PARAMETERS
  None

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ip6_sm_powerup_init
(
  void
);

/*===========================================================================
FUNCTION IP6_SM_INIT()

DESCRIPTION
  This function performs NV related initializations of IP6 state machine.

PARAMETERS
  None

RETURN VALUE
  None

DEPENDENCIES
  This function is called after signalling PS task start.

SIDE EFFECTS
  None
===========================================================================*/
void ip6_sm_init
(
  void
);

#endif /* FEATURE_DATA_PS || FEATURE_DATA_PS_IPV6 */
#endif /* PS_IP6I_SM */
