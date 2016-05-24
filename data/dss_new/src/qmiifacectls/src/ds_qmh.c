/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                 Q U A L C O M M   M S M   I N T E R F A C E

                  M O D E - S P E C I F I C   H A N D L E R


GENERAL DESCRIPTION
  This file contains data and function definitions for the QUALCOMM
  MSM Interface (QMI) Mode-Specific Handler. This module implements
  the PS IFACE interface on the Application processor using the QMI
  framework. Each is a proxy for a matching RmNET IFACE on the Modem
  processor.

EXTERNALIZED FUNCTIONS

  dsqmh_handler_init()
    Initializes the QMI mode-specific handler at powerup.


INITIALIZATION AND SEQUENCING REQUIREMENTS
  dsqmh_handler_init() should be called at startup.

Copyright (c) 2008-2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/


/*===========================================================================

                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datahlos/interface/qmiifacectls/rel/11.01.01/src/ds_qmh.c#2 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
08/13/10    hm     Increased MSG_BUF to 100.
08/24/09    ar     Adjust DSQMH_MSG_BUF macros
02/19/09    am     DS Task De-coupling effort and introduction of DCC task.
07/21/08    ar     Use PS memory buffers for asynch messages.
05/06/08    ar     Created module/initial version.

===========================================================================*/


/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"


#if defined (FEATURE_DATA_PS) && defined (FEATURE_QMI_CLIENT)

#include "msg.h"
#include "err.h"
#include "amssassert.h"
#include "dsm.h"
#include "rex.h"

#include "dserrno.h"
#include "ps_svc.h"
#include "ps_in.h"
#include "ps_mem.h"
#include "ps_flow_ioctl.h"
#include "ps_iface.h"
#include "ps_iface_defs.h"
#include "ps_iface_ioctl.h"
#include "ps_iface_ipfltr.h"
#include "ps_phys_link.h"
#include "ps_phys_link_ioctl.h"
#include "ps_utils.h"
#include "ds_qmh_sm_ext.h"
#include "ds_qmhi.h"
#include "ds_qmh_acl.h"
#include "ds_qmh_hdlr.h"
#include "ds_qmh_llif.h"
#include "dcc_task_defs.h"
#ifndef FEATURE_DSS_LINUX
#include "dcc_task_svc.h"
#endif

#ifdef FEATURE_DATA_PS_IPV6
#include "ps_iface_addr_v6.h"
#endif /* FEATURE_DATA_PS_IPV6 */

/*===========================================================================

            LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE

This section contains local definitions for constants, macros, types,
variables and other items needed by this module.

===========================================================================*/

/*---------------------------------------------------------------------------
  Declare module state information.
---------------------------------------------------------------------------*/
dsqmh_module_info_type  dsqmh_state_info;

/*----------------------------------------------------------------------------
  Allocate memory to hold asynchronous message buffers
----------------------------------------------------------------------------*/
#define DSQMH_MSG_BUF_SIZE  ((sizeof(dsqmh_msg_buf_type) + 3) & ~3)

/*  # of asynchronous message buffers. */
#define DSQMH_MSG_BUF_NUM      100        /* support many QOS notifications. */
#define DSQMH_MSG_BUF_HIGH_WM   80 
#define DSQMH_MSG_BUF_LOW_WM    20

static int ds_qmh_msg_buf_mem[PS_MEM_GET_TOT_SIZE_OPT( DSQMH_MSG_BUF_NUM,
                                                       DSQMH_MSG_BUF_SIZE )];

/*===========================================================================

                    INTERNAL FUNCTION DEFINITIONS

===========================================================================*/
/*===========================================================================
FUNCTION DSQMH_MEM_INIT

DESCRIPTION
  This function initializes the QMI mode-specific handler memory pools. It is
  invoked during host task power-up.

PARAMETERS
  None.

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/
LOCAL void dsqmh_mem_init( void )
{
  int ret_val = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Initialize module memory pools.
  -------------------------------------------------------------------------*/
  ret_val = PS_MEM_POOL_INIT_OPT( PS_MEM_QMH_MSG_BUF_TYPE,
                                  ds_qmh_msg_buf_mem,
                                  DSQMH_MSG_BUF_SIZE,
                                  DSQMH_MSG_BUF_NUM,
                                  DSQMH_MSG_BUF_HIGH_WM,
                                  DSQMH_MSG_BUF_LOW_WM,
                                  NULL,
                                  NULL,
                                  NULL );
  DSQMH_ASSERT( (-1 != ret_val), "QMH HANDLER cannot init the module");

} /* dsqmh_mem_init() */



/*===========================================================================

                    EXTERNAL FUNCTION DEFINITIONS

===========================================================================*/

/*===========================================================================
FUNCTION DSQMH_HANDLER_INIT

DESCRIPTION
  This function initializes the QMI mode-specific handler. It is
  invoked during host task power-up.  It creates a proxy PS interface
  for each QMI instance and initializes parameters.

PARAMETERS
  None.

DEPENDENCIES
  None.

RETURN VALUE
  Signal mask containing the REX signals that the handlers want to wait on.

SIDE EFFECTS
  None.

===========================================================================*/
void dsqmh_handler_init( void )
{
  uint32        sm_inst;                  /* Index for state machine table */
  
  int                      ret_val;
  uint32                   iface_inst;
  /*DELETE*/
  ps_iface_type             *iface_ptr = NULL;
  ps_iface_state_enum_type   iface_state;
  dsqmh_msg_buf_type      *msg_ptr = NULL;    /* Pointer to message buffer */
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_HIGH( "QMH HANDLER INIT executing", 0, 0, 0 );

  /*-------------------------------------------------------------------------
    Initialize module state info block
  -------------------------------------------------------------------------*/
  memset( (void*)&dsqmh_state_info, 0x0, sizeof(dsqmh_state_info) );

  /*-------------------------------------------------------------------------
    Initialize module memory pools
  -------------------------------------------------------------------------*/
  dsqmh_mem_init();

  /*-------------------------------------------------------------------------
    Initialize proc ID to default value.
  -------------------------------------------------------------------------*/
  dsqmh_config_set_supported_call_types( DS_QMH_DEFAULT_SUPPORTED_CALL_TYPE );

  /*-------------------------------------------------------------------------
    Set command handler with DCC for DCC_QMH_PROXY_IFACE_MSG_CMD.
  -------------------------------------------------------------------------*/
  (void) dcc_set_cmd_handler( DCC_QMH_PROXY_IFACE_MSG_CMD, 
                              dsqmhhdlr_async_msg_dispatcher);


  /*-------------------------------------------------------------------------
    Initialize the module lower-layer interface.
  -------------------------------------------------------------------------*/
  dsqmhllif_init();

  /*-------------------------------------------------------------------------
    Initialize the state machine instances.  There is one per Proxy IFACE.
    Note the Proxy IFACE is created within the state machine entry handler.
  -------------------------------------------------------------------------*/
  for( sm_inst=0; sm_inst < DSQMH_MAX_PS_IFACES; sm_inst++ )
  {
    if( STM_SUCCESS != stm_instance_activate( DSQMH_SM,
		       	                      sm_inst,
			                      (void*)sm_inst ) )
    {
      DSQMH_MSG_FATAL( "Failed to initialize state machine: %d",sm_inst,0,0);
    }
  } /* for (DSUMTSPS_MAX_PS_IFACES */

  /*Temporary solution put in place until the issues with 
    QMI message libraries sys event indication is resolved.*/
  sleep(1);
  for (iface_inst = 0; iface_inst < DSQMH_MAX_PS_IFACES; iface_inst++)
  {
    DSQMH_MSG_LOW( "QMH LLIF: before posting PROXY_IFACE_MODEM_INIT_IND ",0,0,0);

    /*---------------------------------------------------------------------
      Post message to host task to complete initialization of the QMI
      mode handler.
    ---------------------------------------------------------------------*/
    DSQMH_GET_MSG_BUF( msg_ptr );
    if( NULL != msg_ptr )
    {
      memset((void*)msg_ptr, 0x0, sizeof(dsqmh_msg_buf_type));
      msg_ptr->msg_id     = PROXY_IFACE_MODEM_INIT_IND;
      msg_ptr->iface_inst = iface_inst;

      DSQMH_MSG_MED( "Posting DS cmd: PROXY_IFACE_MODEM_INIT_IND",0,0,0);
      DSQMH_PUT_MSG_BUF( msg_ptr );
    }
    else
    {
      DSQMH_MSG_FATAL( "Failed to allocate msg buffer, dropping msg",0,0,0 );
    }
  }

  /*-------------------------------------------------------------------------
    Initialize the module command, event & signal handlers.
    Must be done AFTER iface & phys links initialized.
    Note: The module state info is not considered fully initialized until
    after QMI connections have been established.
  -------------------------------------------------------------------------*/
  dsqmhhdlr_init();

  return;

} /* dsqmh_handler_init() */

#endif /* FEATURE_DATA_PS && FEATURE_QMI_CLIENT */

