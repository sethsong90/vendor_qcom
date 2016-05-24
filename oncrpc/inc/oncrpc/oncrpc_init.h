#ifndef ONCRPC_INIT_H
#define ONCRPC_INIT_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                      O N C R P C _ I N I T . H

GENERAL DESCRIPTION
  ONC (Open Network Computing) RPC (Remote Procedure Calls) initialization
  header file.

EXTERNALIZED FUNCTIONS
  oncrpc_init()
  oncrpc_task_start()
  oncrpc_task_stop()
  oncrpc_deinit()

INITIALIZATION AND SEQUENCING REQUIREMENTS
  oncrpc_init  must be called once before oncrpc_task_start
  oncrpc_task_start enter active state where RPC calls can be received
                    and processed
  oncrpc_task_stop  leave active state, when tasks are stoppes no RPC
                    calls are processed.  Calls received and currently
                    in process will be completed and replies sent.

 Copyright (c) 2009 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                             EDIT HISTORY FOR MODULE

  $Header: //linux/pkgs/proprietary/oncrpc/main/source/inc/oncrpc/oncrpc_svc.h#2 $ $DateTime: 2008/09/26 10:58:17 $ $Author: rruigrok $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
09/25/08    rr     Initial version
===========================================================================*/

/*===========================================================================
FUNCTION ONCRPC_INIT

DESCRIPTION
  This function initializes the various data structures used by oncrpc.
  It must be called once and before oncrpc_task_start.

PARAMETERS
  None.

RETURN VALUE
  None.

DEPENDENCIES
  None

SIDE EFFECTS
  Initialization of data structures, memory allocation.
===========================================================================*/
void oncrpc_init( void );


/*===========================================================================
FUNCTION ONCRPC_DEINIT

DESCRIPTION
  This function deinitializes ONCRPC. It will call oncrpc_task_stop to stop all
  threads of execution and deallocate memory.

PARAMETERS
  None.

RETURN VALUE
  None.

DEPENDENCIES
  This should only be called once when a program exits and oncrpc stack is
  no longer needed.

SIDE EFFECTS
  Stop all RPC calls, de-allocate memory.
===========================================================================*/
void oncrpc_deinit( void );

/*===========================================================================
FUNCTION ONCRPC_TASK_START

DESCRIPTION
  Starts oncrpc tasks/threads. This function puts the oncrpc stack in active
  state where outgoing rpc calls can be made, and rpc callbacks and other
  rpc servers can receive calls.

PARAMETERS
  None.

RETURN VALUE
  None.

DEPENDENCIES
  oncrpc_init() must be called prior to calling this function.

SIDE EFFECTS
  Will start pthreads and put oncrpc stack into active state.
===========================================================================*/
void oncrpc_task_start(void);

/*===========================================================================
FUNCTION ONCRPC_TASK_STOP

DESCRIPTION
  Stop oncrpc tasks/threads. This function puts the oncrpc stack in inactive
  state, where no inbound rpc calls are serviced.  Calls currently in
  execution will complete and outgoing replies will be sent.
  This effectively stops the main oncrpc thread, modem restart thread and
  the router reader thread.

PARAMETERS
  None.

RETURN VALUE
  None.

DEPENDENCIES
  None

SIDE EFFECTS
  Will stop main, read and modem_restart pthreads and put oncrpc stack into
  inactive state.
  If the modem restarts, oncrpc stack will not process the required cleanup
  of clients following a restart while in the inactive state.
===========================================================================*/
void oncrpc_task_stop(void);
#endif /*ONCRPC_INIT_H*/
