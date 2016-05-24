#ifndef DSQMH_H
#define DSQMH_H
/*===========================================================================

                 Q U A L C O M M   M S M   I N T E R F A C E

                  M O D E - S P E C I F I C   H A N D L E R

                           H E A D E R   F I L E


DESCRIPTION
  This file contains public data declarations and function prototypes
  for the QMI Mode-Specific Handler.


Copyright (c) 2008-2011 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary.  Export of this technology or software is regulated
by the U.S. Government. Diversion contrary to U.S. law prohibited.
===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datahlos/interface/qmiifacectls/rel/11.01.01/inc/ds_qmh.h#2 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
02/19/09    am     DS Task De-coupling effort and introduction of DCC task.
07/21/08    ar     Relocate typedefs to internal header.
05/06/08    ar     Created module/initial version.

===========================================================================*/


/*===========================================================================

                           INCLUDE FILES

===========================================================================*/

#ifdef __cplusplus
/* If compiled into a C++ file, ensure symbols names are not mangled */
extern "C"
{
#endif

#include "comdef.h"
#include "customer.h"

#if defined (FEATURE_DATA_PS) && defined (FEATURE_QMI_CLIENT)
#include "ps_svc.h"
#include "ps_iface.h"
#include "ps_aclrules.h"
#include "ps_route.h"
#include "ps_phys_link_ioctl.h"


/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/
#define DSQMH_SUCCESS          (0)
#define DSQMH_FAILED           (-1)

/*---------------------------------------------------------------------------
  Default linger timeout value in milliseconds.
---------------------------------------------------------------------------*/
#define DSQMH_DEFAULT_LINGER_TIMEOUT 0

/*===========================================================================
                      PUBLIC FUNCTION DECLARATIONS

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
void dsqmh_handler_init( void );

#endif /* FEATURE_DATA_PS && FEATURE_QMI_CLIENT */

#ifdef __cplusplus
} /* extern "C" {...} */
#endif

#endif /* DSQMH_H */
