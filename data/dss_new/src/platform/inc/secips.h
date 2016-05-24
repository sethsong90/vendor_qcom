#ifndef SECIPS_H
#define SECIPS_H
/*===========================================================================

               A S W   S E C U R I T Y   S E R V I C E S

                        I P S E C   L A Y E R

GENERAL DESCRIPTION
  This file contains IPSec initialization and shutdown functions.

EXTERNALIZED FUNCTIONS
  secips_init  
  secips_shutdown
  secips_proc_msg_sig
  secips_proc_phase1_timer_sig
  secips_generate_ipsec_info

INITIALIZATION AND SEQUENCING REQUIREMENTS
  None

Copyright (c) 2003-2009 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  $PVCSPath: L:/src/asw/COMMON/vcs/secips.h_v   1.5   30 Aug 2003 20:03:54   omichael  $
  $Header: //depot/asic/sandbox/users/hmurari/qcmapi_porting/stubs/inc/secips.h#1 $ $DateTime: 2009/06/27 13:50:47 $ $Author: hmurari $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
02/26/09   sp      IPsec CMI Decoupling.
04/20/05   ssm     Added Phase2 renegotiation timers and code cleanup
01/25/05   sv/ssm  Re-architectured IPSEC to support multiple SA's and added
                   support for AH.
03/31/03   et      Created module, sunny cal, bored

===========================================================================*/



/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"
#include "ps_pkt_info.h"


/*===========================================================================

                     DEFINITIONS AND CONSTANTS

===========================================================================*/



/*===========================================================================

                 DEFINITIONS AND TYPE DECLARATIONS

===========================================================================*/

#define SECIPS_ESP_MAX_HDR_SIZE   64
#define SECIPS_MAX_TUNNELS 2
#define SECIPS_MAX_IFACE_LIST (SECIPS_MAX_TUNNELS +2)


typedef struct
{
  void*           esp_sa_ptr;
  void*           ah_sa_ptr;
  void*           ipsec_handle;
  void*           iface_list[SECIPS_MAX_IFACE_LIST];
  uint8           iface_cnt;
  ps_ip_addr_type next_gw_addr;
  uint32          ipsec_header_size;
  uint32          user_id;
}secips_ipsec_info_type;

/* <EJECT> */ 
#ifdef FEATURE_SEC_IPSEC
#include "dsm.h"

#include "secerrno.h"
#include "secipstypes.h"
/*===========================================================================

                      FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================
FUNCTION SECIPS_INIT

DESCRIPTION
  

DEPENDENCIES
  None
  
PARAMETERS
  None

RETURN VALUE
  None
  
SIDE EFFECTS
  None
===========================================================================*/
secerrno_enum_type secips_init
(
  void
);




/*===========================================================================
FUNCTION SECIPS_SHUTDOWN

DESCRIPTION
  

DEPENDENCIES
  None
  
PARAMETERS
  None

RETURN VALUE
  None
  
SIDE EFFECTS
  None
===========================================================================*/
secerrno_enum_type secips_shutdown
(
  void
);


/*===========================================================================
FUNCTION SECIPS_PROC_MSG_SIG

DESCRIPTION
  

DEPENDENCIES
  None
  
PARAMETERS
  None

RETURN VALUE
  None
  
SIDE EFFECTS
  None
===========================================================================*/
void secips_proc_msg_sig
(
  void
);

/*===========================================================================
FUNCTION SECIPS_PROC_PHASE1_TIMER_SIG

DESCRIPTION
  

DEPENDENCIES
  None
  
PARAMETERS
  None

RETURN VALUE
  None
  
SIDE EFFECTS
  None
===========================================================================*/
void secips_proc_phase1_timer_sig
(
  rex_sigs_type sig
);


/*===========================================================================
FUNCTION SECIPS_GET_IPSEC_INFO

DESCRIPTION
  

DEPENDENCIES
  None
  
PARAMETERS
  None

RETURN VALUE
  None
  
SIDE EFFECTS
  None
===========================================================================*/
secerrno_enum_type secips_generate_ipsec_info
(
  void *                  ipsec_handle,
  ip_pkt_info_type        *pkt_info_ptr,
  secips_ipsec_info_type  *ipsec_info,
  uint32                  user_id
);

#endif /* FEATURE_SEC_IPSEC */

#endif /* SECIPS_H */
