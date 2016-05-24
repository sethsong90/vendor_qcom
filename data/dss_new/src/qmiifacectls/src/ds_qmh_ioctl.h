#ifndef DSQMH_IOCTL_H
#define DSQMH_IOCTL_H
/*===========================================================================


                       Q M I   M O D E   H A N D L E R

           M O D E  - S P E C I F I C   I O C T L   H A N D L E R S
                       
GENERAL DESCRIPTION
  This file contains data declarations and function prototypes for the
  QMI Proxy IFACE mode-specific IOCTL handlers.

EXTERNALIZED FUNCTIONS

INITIALIZATION AND SEQUENCING REQUIREMENTS
  dsqmh_ioctl_init() should be called at startup. 

Copyright (c) 2008 by Qualcomm Technologies, Inc.  All Rights Reserved.
===========================================================================*/

/*===========================================================================

  $Header: //source/qcom/qct/modem/datahlos/interface/qmiifacectls/rel/11.01.01/src/ds_qmh_ioctl.h#1 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
10/03/08    ar     Created module/initial version.

===========================================================================*/

		       
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"

#if defined (FEATURE_DATA_PS) && defined (FEATURE_QMI_CLIENT)

/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/



/*===========================================================================
                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================
FUNCTION  DSQMHIOCTL_IFACE_DISPATCHER

DESCRIPTION
  This function is the dispatcher for IFACE IOCTLs sent by clients. Any
  IOCTL not supported will be treated as an error.

PARAMETERS
  iface_ptr         - Pointer to ps_iface

  ioctl_name        - The operation name

  argval_ptr        - Pointer to operation specific structure

  ps_errno          - Error code returned in case of failure (Error
                      values are those defined in dserrno.h)

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  Message buffer memory is returned to task heap.
===========================================================================*/
int dsqmhioctl_iface_dispatcher
(
  ps_iface_type           *iface_ptr,
  ps_iface_ioctl_type      ioctl_name,
  void                    *argval_ptr,
  sint15                  *ps_errno
);


/*===========================================================================
FUNCTION  DSQMHIOCTL_FLOW_DISPATCHER

DESCRIPTION
  This function is the dispatcher for QOS FLOW IOCTLs sent by clients. Any
  IOCTL not supported will be treated as an error.

PARAMETERS
  flow_ptr          - Pointer to ps_flow

  ioctl_name        - The operation name

  argval_ptr        - Pointer to operation specific structure

  ps_errno          - Error code returned in case of failure (Error
                      values are those defined in dserrno.h)

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  Message buffer memory is returned to task heap.
===========================================================================*/
int dsqmhioctl_flow_dispatcher
(
  ps_flow_type            *flow_ptr,
  ps_flow_ioctl_type       ioctl_name,
  void                    *argval_ptr,
  sint15                  *ps_errno
);


/*===========================================================================
FUNCTION DSQMHIOCTL_INIT

DESCRIPTION
  This function initializes the QMI Modem Hander mode-specific IOCTL
  handlers. It is invoked during power-up.
  
PARAMETERS
  None.
  
DEPENDENCIES
  None.
  
RETURN VALUE
  None.
  
SIDE EFFECTS 
  None.
  
===========================================================================*/
void dsqmhioctl_init( void );

#endif /* FEATURE_DATA_PS && FEATURE_QMI_CLIENT */

#endif    /* DSQMH_IOCTL_H */



