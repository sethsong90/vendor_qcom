#ifndef PS_RM_TASK_H
#define PS_RM_TASK_H
/*===========================================================================

                          P S _ RM _ T A S K . H

DESCRIPTION
  This is the header file for the PS RM Data processing Task. Contained 
  herein are the functions needed to initialize all the modules that execute
  in PS RM task context and the main task processing loop.

EXTERNALIZED FUNCTIONS
  ps_rm_task()
    PS RM Task entry point and main processing loop.

Copyright (c) 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary.
===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_rm_task.h#1 $ 
  $DateTime: 2011/01/10 09:44:56 $
  $Author: maldinge $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
06/10/10    dm     Created module

===========================================================================*/


/*===========================================================================

                                INCLUDE FILES

===========================================================================*/

#include "comdef.h"
#include "dog.h"
#include "rex.h"


/*===========================================================================

                             MACROS & DATA DECLARATIONS

===========================================================================*/

extern dog_report_type   ps_rm_dog_rpt_var;

/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/
/*===========================================================================

FUNCTION PS_RM_TASK()

DESCRIPTION
  This function is the entry point and main processing loop for the RM Data
  processing task.

DEPENDENCIES
  None.

RETURN VALUE
  Does not Return.

SIDE EFFECTS
  None.

===========================================================================*/

void
ps_rm_task
(
  uint32 dummy
);

#endif /* PS_RM_TASK_H */

