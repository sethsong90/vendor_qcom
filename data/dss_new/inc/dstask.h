#ifndef DSTASK_H
#define DSTASK_H
/*===========================================================================

                     D A T A   S E R V I C E S   T A S K

                            H E A D E R   F I L E

DESCRIPTION
  This is the external header file for the Data Services (DS) Task. This file
  contains all the functions, definitions and data types needed for other
  modules to interface to the Data Services Task.


  Copyright (c) 2009 by Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary.
===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $PVCSPath: L:/src/asw/MM_DATA/vcs/dstask.h_v   1.17   28 Feb 2003 18:56:06   rchar  $
  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/dstask.h#1 $ $DateTime: 2011/01/10 09:44:56 $ $Author: maldinge $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
03/09/09   sa     Initial version.

===========================================================================*/


/*===========================================================================

                      INCLUDE FILES

===========================================================================*/


/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/

/*---------------------------------------------------------------------------
  Data statistics types are used by Data Servies to indicate to UI if the
  data statistics UI receives is valid or not. If the data stats is not
  valid, UI should properly handle such case. The data stats types are:
  
  DS_STATS_VALID            - Valid data statistics
  DS_STATS_INVALID_CALL_ID  - Invalid call ID
  DS_STATS_UNAVAILABLE      - Data statistics are unavailable
---------------------------------------------------------------------------*/
typedef enum
{
  DS_STATS_VALID,                                 /* Valid data statistics */
  DS_STATS_INVALID_CALL_ID,                             /* Invalid call ID */
  DS_STATS_UNAVAILABLE                  /* Data statistics are unavailable */
} ds_stats_e_type;


/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================

FUNCTION DS_TASK

DESCRIPTION
  This is the entry point for the Data Services Task. This function contains
  the main processing loop that waits for events (signals or commands) and
  dispatches each event to the appropriate entity for further processing.

DEPENDENCIES
  None

RETURN VALUE
  This function does not return.

SIDE EFFECTS
  None

===========================================================================*/

extern void  ds_task
(
  dword ignored
    /* lint -esym(715,ignored)
    ** Have lint not complain about the ignored parameter 'ignored' which is
    ** specified to make this routine match the template for rex_def_task().
    */
);

#endif /* DSTASK_H */
