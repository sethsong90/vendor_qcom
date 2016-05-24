/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
                              D S  N E T   L I B

GENERAL DESCRIPTION
  This is the platform specific source file for initializing DSNET Library.

Copyright (c) 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header:$
  $DateTime:$

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  05/03/2010 ar  Created module.

===========================================================================*/

/*===========================================================================

                      INCLUDE FILES FOR MODULE

===========================================================================*/

#include <sys/types.h>
#include <unistd.h>

#include "ds_Net_Init.h"
#include "msg.h"
#include "diag_lsm.h"
#include "ps_utils_init.h"
#include "ps_netiface_init.h"
#include "dcc_task_defs.h"
#include "ds_sig_svc.h"
#include "ds_qmh.h"
#include "dsnet_lib.h"

#ifdef FEATURE_DATA_PS_IPV6  
#include "ps_ifacei_addr_v6.h"
#endif


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                        LOCAL DECLARATIONS FOR MODULE

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

LOCAL boolean dsnet_initialized = FALSE;

LOCAL dsnet_create_instance_cb_t dsnet_create_instance_table = {NULL,NULL};

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                         LOCAL FUNCTION DECLARATIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                         GLOBAL FUNCTION DECLARATIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/*===========================================================================
FUNCTION DSNET_REGISTER_SOCKET_CALLBACKS

DESCRIPTION
  This function initializes the DSNET callback table for DSSocket
  create instance methods.  Callback are used to decouple the libraries
  for DSNet can be used only when control-plane functionality is required.

DEPENDENCIES
  None

PARAMETERS
  cb_tbl_ptr  - Pointer to callback table structure

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
void dsnet_register_socket_callbacks
(
  dsnet_create_instance_cb_t * cb_tbl_ptr
)
{
  ASSERT( cb_tbl_ptr );

  dsnet_create_instance_table = *cb_tbl_ptr;
}


/*===========================================================================
FUNCTION DSNET_INIT

DESCRIPTION
  This function initializes the DSNET API for client usage.  Invokes the
  various initialization routines for modules within DSNET and PS.  Starts
  command thread execution.
  
DEPENDENCIES
  None

PARAMETERS
  None

RETURN VALUE
  None

SIDE EFFECTS
  Command threads are spawnwed
===========================================================================*/
void dsnet_init( void )
{
  /* Note:If client calls dsnet_lib multiple times
   * diag will be initialized multiple times.
  */
#ifdef FEATURE_DATA_LOG_QXDM
  /* Initialize DIAG subsystem logging */
  Diag_LSM_Init(NULL);
#endif

  if( dsnet_initialized ) {
    MSG_ERROR( "DSNET API already initialized, ignoring!",0,0,0 );
    return;
  }

  /*------------------------------------------------------------------------
    Initialize the library task threads. This will start
    a pthread to serialize all async/callback functions
  -------------------------------------------------------------------------*/
  dcc_cmdthrd_init();
  ds_sig_cmdthrd_init();

  /* Initialize PS layer modules */
  ps_utils_powerup();
  ps_netiface_powerup();
#ifdef FEATURE_DATA_PS_IPV6  
  ps_iface_addr_v6_init();
#endif
  
  DSNetPowerup();
  ps_utils_init();
  ps_netiface_init();
  dsqmh_handler_init();
  DSNetInit();

  /* Initialize random number generator for IPV6 IID creation */
  srand(time(NULL) + getpid());
  
  MSG_HIGH( "DSNET initialized!",0,0,0 );
  dsnet_initialized = TRUE;
  return;
}



/*===========================================================================
FUNCTION DSNET_RELEASE

DESCRIPTION
  This function releases DSNET API from client.

DEPENDENCIES
  None

PARAMETERS
  None

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
void dsnet_release( void )
{
  if( !dsnet_initialized ) {
    MSG_ERROR( "DSNET API already released, ignoring!",0,0,0 );
    return;
  }

  /* Release plaform-specific modules */
  ds_sig_cmdthrd_deinit();
  dcc_cmdthrd_deinit();

  MSG_HIGH( "DSNET released!",0,0,0 );
  dsnet_initialized = FALSE;
  return;
}

