#ifndef DSQMH_HDLR_H
#define DSQMH_HDLR_H
/*===========================================================================


                       Q M I   M O D E   H A N D L E R

                C O M M A N D  &  E V E N T   H A N D L E R S
		       
GENERAL DESCRIPTION
  This file contains data declarations and function prototypes for the
  QMI Proxy IFACE command and event handlers.

EXTERNALIZED FUNCTIONS

INITIALIZATION AND SEQUENCING REQUIREMENTS

Copyright (c) 2008 - 2009 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

  $Header: //source/qcom/qct/modem/datahlos/interface/qmiifacectls/rel/11.01.01/inc/ds_qmh_hdlr.h#1 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
06/01/09    ar     Added dsqmhhdlr_phys_link_down_cmd prototype.
02/19/09    am     DS Task De-coupling effort and introduction of DCC task.
05/06/08    ar     Created module/initial version.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"

#if defined (FEATURE_DATA_PS) && defined (FEATURE_QMI_CLIENT)
#include "dcc_task_defs.h"

/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/



/*===========================================================================
  
                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/
/*===========================================================================
FUNCTION  DSQMHHDLR_GET_DOS_ACK_HANDLE

DESCRIPTION
  This function gets the DOS ACK handle from IP identifier. 

PARAMETERS
  ip_identifier - IP identifier.

DEPENDENCIES
  The IP identifier/DOS Ack handle pair are removed from map. 

RETURN VALUE
  DOS ack handle if entry is found
  0 if not found if no entry is found.  

SIDE EFFECTS
  Message buffer memory is returned to task heap.
===========================================================================*/
int32 dsqmhhdlr_get_dos_ack_handle
(
  uint16 ip_identifier 
);

/*===========================================================================
FUNCTION  DSQMHHDLR_ASYNC_MSG_DISPATCHER

DESCRIPTION
  This function is the dispatcher for asynchronous messages posted to
  the host task.  Individual messages are passed to internal handlers
  for actual processing.  The passed message buffer memory is freed in
  this function.

PARAMETERS
  cmd_ptr        - Pointer to asynchronous message 

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  Message buffer memory is returned to task heap.
===========================================================================*/
void dsqmhhdlr_async_msg_dispatcher
(
  dcc_cmd_enum_type  cmd,
  void             * user_data_ptr
);


/*===========================================================================
FUNCTION      DSQMHHDLR_PHYS_LINK_DOWN_CMD

DESCRIPTION
  Send asynchronous command to teardown the physical link to Modem processor. 

PARAMETERS
  phys_link_ptr - phys link ptr for the connection
  info_ptr      - not used

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  None.
===========================================================================*/
int dsqmhhdlr_phys_link_down_cmd
(
  ps_phys_link_type *phys_link_ptr,
  void              *info_ptr
);


#ifdef FEATURE_DATA_PS_IPV6
/*===========================================================================
FUNCTION      DSQMHHDLR_IPV6_CONFIGURE_HDLR

DESCRIPTION
  This function performs the operations necessary to configure iface
  public IPV6 address.

PARAMETERS
  iface_inst      - Index for iface table
  is_reconfigure  - Reconfiguration flag
  
DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  None.
===========================================================================*/
int dsqmhhdlr_ipv6_configure_hdlr
(
  uint32     iface_inst,                        /* Index for iface table   */
  boolean    is_reconfigure
);

/*===========================================================================
FUNCTION      DSQMHHDLR_IPV6_CLEANUP_HDLR

DESCRIPTION
  This function performs the operations necessary to cleanup iface
  public IPV6 address.

PARAMETERS
  iface_inst      - Index for iface table
  
DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  None.
===========================================================================*/
int dsqmhhdlr_ipv6_cleanup_hdlr
(
  uint32     iface_inst                         /* Index for iface table   */
);
#endif /* FEATURE_DATA_PS_IPV6 */


/*===========================================================================
FUNCTION      DSQMHHDLR_INIT_IFACE_CMD

DESCRIPTION
  This function performs initialization of the specified Proxy IFACE
  instance.  The PS IFACE and Phys Links are configured for client access.
  This routine is meant to be called from the state machine entry function. 

PARAMETERS
  iface_inst      - Index for iface table

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  None.
===========================================================================*/
int dsqmhhdlr_init_iface_cmd
(
  uint32 iface_inst                             /* Index for iface table   */
);


/*===========================================================================
FUNCTION DSQMHHDLR_INIT

DESCRIPTION
  This function initializes the QMI mode-specific handler handlers. It
  is invoked during power-up.  It registers asynchronous message
  handler and signal handler with the host task.
  
PARAMETERS
  None.
  
DEPENDENCIES
  None.
  
RETURN VALUE
  None.
  
SIDE EFFECTS 
  None.
  
===========================================================================*/
void dsqmhhdlr_init( void );

/*===========================================================================
FUNCTION  DSQMHHDLR_IFACE_IS_LINGER_SUPPORTED

DESCRIPTION
  This function checks if provided iface supports linger feature

PARAMETERS

  iface_ptr         - Pointer to ps_iface

DEPENDENCIES
  iface_ptr shall be valid
  ps_iface_name_enum_type

RETURN VALUE
  TRUE provided iface supports linger
  FALSE provided iface does not support linger

SIDE EFFECTS
  None.
===========================================================================*/
boolean dsqmhhdlr_iface_is_linger_supported
(
  ps_iface_type  *iface_ptr
);

#endif /* FEATURE_DATA_PS && FEATURE_QMI_CLIENT */

#endif    /* DSQMH_HDLR_H */



