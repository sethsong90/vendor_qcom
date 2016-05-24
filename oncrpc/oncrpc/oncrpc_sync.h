#ifndef ONCRPC_SYNC_H
#define ONCRPC_SYNC_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                      O N C R P C _ S Y N C . H 

GENERAL DESCRIPTION
  This file exports the APIs used to sync external events task events
  with the ONCRPC task.  

  An example of a use of this is the starting of SCHED tasks.  This
  allows the task to get started before the task create function
  returns.
  
  ONC (Open Network Computing) RPC (Remote Procedure Calls) task code.
  This file deals with the dispatch of events to other RPC parts and
  implements various accounting functions that couldn't be done the
  original way due to memory system constrains. (lack of malloc/free).

INITIALIZATION AND SEQUENCING REQUIREMENTS

  oncrpc_task is started from mc in the normal way.

 Copyright (c) 2002-2008 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential.
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $PVCSPath: L:/src/asw/COMMON/vcs/oncrpctask.h_v   1.11   29 Oct 2003 14:27:42   cplatt  $
  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_sync.h#3 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
07/10/07    ptm    Remove featurization.
05/08/06    ptm    Change post_sync_sig to take the tcb as an argument.
03/31/05    clp    Split from the oncrpctask.h
08/19/03    ifk    Added sta_post_signal() and sta_process_signal().
12/17/01    clp    Added header and comments to source file.


===========================================================================*/


/*= = = = = = = =  = = = = = = = = = =  = = =  = = = = = = = = = = = = = = =
                        FUNCTION DEFINTIONS
= = = = = = = = = = = = = = =  = = = = =  = = = = = = = = = = = = = = = = =*/

/*===========================================================================

FUNCTION ONCRPC_POST_SYNC_SIG_TCB

DESCRIPTION
  Post the RPC_SYNC_SIG signal to the given task.

PARAMETERS
  The task to signal.

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  RPC_SYNC_SIG is posted to the given task.

===========================================================================*/
void oncrpc_post_sync_sig_tcb
(
  void *tcb
);


/*===========================================================================

FUNCTION ONCRPC_WAIT_SYNC_SIG

DESCRIPTION
  Call oncrpc_event_wait for the RPC_SYNC_SIG signal on the oncrpc_task.  
  Return on getting the signal while reposting any undesired signal back to 
  the task.

DEPENDENCIES
  This function should be called from oncrpc task context only.

RETURN VALUE
  None.

SIDE EFFECTS
  oncrpc_event_wait for RPC_SYNC_SIGNAL.  Repost any unwanted signals back 
  to the task before returning.

===========================================================================*/
void oncrpc_wait_sync_sig
(
  void
);

#endif /* ONCRPC_SYNC_H */
