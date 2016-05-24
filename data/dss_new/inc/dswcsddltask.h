#ifndef DSWCSDDLTASK_H
#define DSWCSDDLTASK_H
/*===========================================================================

             W C D M A   C I R C U I T - S W I T C H E D   D A T A

                         D O W N L I N K   T A S K
                         
                          H E A D E R   F I L E

DESCRIPTION
  This file contains function prototypes for WCDMA 
  Circuit-Switched Data Downlink Task used by TMC module. 

EXTERNALIZED FUNCTIONS

INITIALIZATION AND SEQUENCING REQUIREMENTS

   Copyright (c) 2009 by Qualcomm Technologies, Inc.
   All Rights Reserved.
   Qualcomm Technologies Confidential and Proprietary.
===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $PVCSPath:  L:/src/asw/MSM5200/data/vcs/dswcsddltask.h_v   1.2.1.0   07 Nov 2001 17:16:36   sparikh  $
  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/dswcsddltask.h#1 $ $DateTime: 2011/01/10 09:44:56 $ $Author: maldinge $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
03/04/09   sa     Initial version.

===========================================================================*/


/*===========================================================================

                     INCLUDE FILES FOR MODDLE

===========================================================================*/

/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/


/*===========================================================================

FUNCTION DSWCSD_DL_TASK

DESCRIPTION
  This is the entry point for the WCDMA Circuit-Switched Data Downlink Task. 
  This function contains the main processing loop that waits for events 
  (signals or commands) and dispatches each event to the appropriate entity 
  for further processing.

DEPENDENCIES
  None

RETURN VALUE
  This function does not return.

SIDE EFFECTS
  None

===========================================================================*/

extern void  dswcsd_dl_task
(
  uint32 ignored
    /* lint -esym(715,ignored)
    ** Have lint not complain about the ignored parameter 'ignored' which is
    ** specified to make this routine match the template for rex_def_task() 
    */
);
#endif /* DSWCSDDLTASK_H */
