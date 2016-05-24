#ifndef PS_STAT_COMMONI_H
#define PS_STAT_COMMONI_H
/*===========================================================================

                     P S _ S T A T _ C O M M O N I . H

DESCRIPTION
  This is the header file for the Data protocol Task. Contained herein are
  the functions needed to initialize all the statistics modules for PS 

EXTERNALIZED FUNCTIONS
  ps_stat_init

 Copyright (c) 2007 by Qualcomm Technologies, Inc.  All Rights Reserved.
 
===========================================================================*/


/*=========================================================================*/

#include "comdef.h"
#include "customer.h"       /* Customer Specific Features */
#include "rex.h"

#ifdef FEATURE_DATA_PS
#include "IxErrno.h"


/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/
/*===========================================================================
FUNCTION PS_STAT_INIT()

DESCRIPTION

DEPENDENCIES
  None

PARAMETERS

RETURN VALUE

SIDE EFFECTS
  None
===========================================================================*/
void ps_stat_init
(
  void
);


#endif /* FEATURE_DATA_PS */

#endif /* PS_STAT_COMMONI_H */
