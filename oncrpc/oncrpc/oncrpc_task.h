#ifndef ONCRPC_TASK_H
#define ONCRPC_TASK_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                      O N C R P C _ T A S K . H 

GENERAL DESCRIPTION
  This file exports the public (outside of the ONCRPC stack) APIs
  associated with task creation and manipulation.

  ONC (Open Network Computing) RPC (Remote Procedure Calls) task code.
  This file deals with the dispatch of events to other RPC parts and
  implements various accounting functions that couldn't be done the
  original way due to memory system constrains. (lack of malloc/free).

INITIALIZATION AND SEQUENCING REQUIREMENTS

  oncrpc_task is started from mc in the normal way.

 Copyright (c) 2002-2005, 2007-2008 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential.
  *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $PVCSPath: L:/src/asw/COMMON/vcs/oncrpctask.h_v   1.11   29 Oct 2003 14:27:42   cplatt  $
  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_task.h#3 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
08/31/07    ptm    Change "oncrpc outstanding call" to return count instead of
                   boolean.
07/10/07    ptm    Remove featurization.
04/02/07    ddh    Added API to check for outstanding transactions.
08/08/05    hn     Added definitions for the per task RPC info.
03/30/05    clp    Split file into public (outside oncrpc) and private.
08/19/03    ifk    Added sta_post_signal() and sta_process_signal().
12/17/01    clp    Added header and comments to source file.


===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/

/*= = = = = = = =  = = = = = = = = = =  = = =  = = = = = = = = = = = = = = =
                        DATA TYPE DEFINITIONS
= = = = = = = = = = = = = = =  = = = = =  = = = = = = = = = = = = = = = = =*/
typedef void oncrpc_task_destroy_f_type( void *clnt );

typedef struct {
  void                       *clnt;
  void                       *cb_data;
  oncrpc_task_destroy_f_type *func;
} oncrpc_task_info_type;



/*= = = = = = = =  = = = = = = = = = =  = = =  = = = = = = = = = = = = = = =
                        FUNCTION DEFINTIONS
= = = = = = = = = = = = = = =  = = = = =  = = = = = = = = = = = = = = = = =*/

/*===========================================================================
FUNCTION ONCRPC_OUTSTANDING_CALL

DESCRIPTION
  This function checks if there are any outstanding transactions and
  if so returns TRUE.

PARAMETERS
  None.

RETURN VALUE
  TRUE if there are outstanding transactions, FALSE otherwise.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
extern uint32 oncrpc_outstanding_call ( void );


/*===========================================================================
FUNCTION ONCRPC_TASK

DESCRIPTION
  This function is the task and processes signals/events for the task.

PARAMETERS
  Ignored.

RETURN VALUE
  Never returns.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
#ifdef __linux
void * oncrpc_task(void* not_used);
#else
extern void oncrpc_task(dword unused);
#endif
#endif /* ONCRPC_TASK_H */
