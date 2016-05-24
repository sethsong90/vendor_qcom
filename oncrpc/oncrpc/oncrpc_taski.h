#ifndef ONCRPC_TASKI_H
#define ONCRPC_TASKI_H

/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                      O N C R P C _ T A S K I . H

GENERAL DESCRIPTION
  This file exports the internal (Inside of the ONCRPC stack) APIs
  associated with task creation and manipulation.

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
  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_taski.h#3 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
07/10/07    ptm    Remove reply task APIs.
10/09/06    hn     Put back auto call retry hook in state machine because
                   it's needed after a connection is lost and reinited.
10/03/06    hn     Removed automatic call retry hook from state machine.
05/29/06    ptm    Added oncrpc_retry_call
01/25/06    ptm    Added rpc_reply_cmd.
07/13/05    clp    Added oncrpc_queue_reply_remove
05/09/05    clp    Added code review comments.
04/13/05    ptm    Added prototypes for reply queue APIs.
03/31/05    clp    Split into public (outside oncrpc) and private.
08/19/03    ifk    Added sta_post_signal() and sta_process_signal().
12/17/01    clp    Added header and comments to source file.


===========================================================================*/

/* Defines */

/* Signals */

#define RPC_CMD_Q_SIG                 0x0001
#define RPC_SVC_DIAG_TIMEOUT_SIG      0x0002
/* Signal to indicate a synchronization ack.  This signal is handled in
 * the corresponding code running in ONCRPC task context but outside the main
 * ONCRPC signal handling loop.
 */
#define RPC_SYNC_SIG                  0x0004
#define RPC_WAIT_SIG                  0x0008

/* types, etc */

// FIXME Make the rpc_cmd_struct generic handler, no cmd, no xdr. just
// handler and data.
typedef enum {
  RPC_READ,
  // FIXME change RPC_XDR -> RPC_CMD_HANDLER
  RPC_XDR,
  RPC_DESTROY
} rpc_cmd_command_type;

#define ONCRPC_CMD_MAX_DATA 4
typedef struct rpc_cmd_struct {
  rpc_cmd_command_type  cmd;
  xdr_s_type           *xdr;
  void                (*handler)( struct rpc_cmd_struct *cmd );
  uint32                data[ONCRPC_CMD_MAX_DATA];
} rpc_cmd_type;

/*= = = = = = = =  = = = = = = = = = =  = = =  = = = = = = = = = = = = = = =
                        FUNCTION DEFINTIONS
= = = = = = = = = = = = = = =  = = = = =  = = = = = = = = = = = = = = = = =*/

/*===========================================================================
FUNCTION RPC_CMD

DESCRIPTION
  This function allocates a cmd buffer for the ONCRPC_TASK, copies the
  data into it, and queues it for the task.  Also sets the cmd signal
  for the task to wake up.

PARAMETERS
  The cmd to queue.

RETURN VALUE
  None.

DEPENDENCIES
  None.

SIDE EFFECTS
  None.
===========================================================================*/
// FIXME this takes a handler, xdr and data.
extern void
rpc_cmd
(
  rpc_cmd_type * cmd_ptr             /* pointer to the command data */
);

/*===========================================================================
FUNCTION ONCRPC_APP_INIT

DESCRIPTION
  This function initializes the RPC applications.

PARAMETERS
  None.

RETURN VALUE
  None.

DEPENDENCIES
  ONCRPC should be up and running.

SIDE EFFECTS
  None
===========================================================================*/
void
oncrpc_app_init( void );

/*===========================================================================

FUNCTION ONCRPC_NEXT_XID

DESCRIPTION
  Returns the next RPC message ID (XID) - by using a single source of XIDs,
  the oncrpc task can match replies to calls.

DEPENDENCIES
  None.

RETURN VALUE
  XID

SIDE EFFECTS
  None.

===========================================================================*/
extern uint32
oncrpc_next_xid( void );

/*===========================================================================

FUNCTION oncrpc_queue_reply

DESCRIPTION
  Queues an xdr transport on which a message has been sent. When the ONCRPC
  task receives a reply the xdr transport is dequeued and given the reply.

DEPENDENCIES
  oncrpc_init_reply_queue must have been called

RETURN VALUE
  None

SIDE EFFECTS
  None.

===========================================================================*/
extern void
oncrpc_queue_reply
(
  xdr_s_type *xdr               /* pointer to xdr to enqueue */
);

/*===========================================================================

FUNCTION oncrpc_queue_reply_remove

DESCRIPTION
  Removes the given XDR from the reply queue when the XDR is
  destroyed.

DEPENDENCIES
  oncrpc_init_reply_queue must have been called

RETURN VALUE
  None

SIDE EFFECTS
  None.

===========================================================================*/
void
oncrpc_queue_reply_remove
(
  xdr_s_type *xdr               /* pointer to xdr to enqueue */
);

/*===========================================================================

FUNCTION ONCRPC_FIND_REPLY

DESCRIPTION
  Search the reply queue for and XDR that matches the given XID. 

DEPENDENCIES
  None

RETURN VALUE
  XDR if one is found, NULL otherwise

SIDE EFFECTS
  The XDR structure is removed from the reply queue.

===========================================================================*/
xdr_s_type *
oncrpc_find_reply
(
  uint32 xid                    /* the XID to match */
);

/*===========================================================================

FUNCTION ONCRPC_RETRY_CALL

DESCRIPTION
  Scans the reply queue for XDRs waiting on the given transport signals
  those XDRs to retry the RPC call.

  This is used for the Shared Memory (SM) transport for cases when the
  server at the other end of the transport has reset.

DEPENDENCIES
  oncrpc_init_reply_queue must have been called

RETURN VALUE
  None

SIDE EFFECTS
  Selected XDRs are removed from the reply queue.

===========================================================================*/
void
oncrpc_retry_call
(
  rpcprot_t protocol
);
#endif /* ONCRPC_TASKI_H */
