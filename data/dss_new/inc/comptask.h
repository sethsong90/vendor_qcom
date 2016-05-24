#ifndef COMPTASK_H
#define COMPTASK_H
/*===========================================================================

                              C O M P  T A S K

                            H E A D E R   F I L E

DESCRIPTION
  This is the external header file for the COMP Task. This file
  contains all the functions, definitions and data types needed for other
  tasks to interface to the COMP Task.

  Copyright (c) 2009 by Qualcomm Technologies, Inc.  All Rights Reserved.  
===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/comptask.h#1 $ $DateTime: 2011/01/10 09:44:56 $ $Author: maldinge $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
04/03/09   sa     Initial creation.

===========================================================================*/


/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================

FUNCTION COMP_TASK

DESCRIPTION
  This is the entry point for the Compression Task. This function contains
  the main processing loop that waits for events (signals or commands) and
  dispatches each event to the appropriate entity for further processing.

DEPENDENCIES
  None

RETURN VALUE
  This function does not return.

SIDE EFFECTS
  None

===========================================================================*/

extern void  comp_task
(
  dword ignored
    /* lint -esym(715,ignored) */
);

#endif /* COMPTASK_H */
