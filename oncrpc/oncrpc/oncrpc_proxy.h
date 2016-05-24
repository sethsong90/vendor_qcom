#ifndef ONCRPC_PROXY_H
#define ONCRPC_PROXY_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                      O N C R P C _ P R O X Y . H

GENERAL DESCRIPTION
  ONC (Open Network Computing) RPC (Remote Procedure Calls) proxy task header
  file.
  This file defines the data structures and interfaces for managing ONCRPC
  proxy tasks.

EXTERNALIZED FUNCTIONS
  oncrpc_proxy_init
    Initializes the data structures used by this package. Must be called
    before any other functions in this package.

  oncrpc_proxy_dispatch
    Forwards RPC calls from the RPC task to proxy tasks that dispatch the
    calls in their own context.

  oncrpc_proxy_forward
    Forwards RPC calls from the RPC task to proxy tasks that then forward the
    calls in their own context onto the shared memory transport and forward
    the reply back to the original caller.

INITIALIZATION AND SEQUENCING REQUIREMENTS

  oncrpc_proxy_init must be called before any other functions in this package.

  The ONCRPC proxy tasks are controlled exclusively by the ONCRPC task. They
  are initially started the first time ONCRPC trys to dispatch a call operation
  to the proxy task. When ONCRPC is stopped, it calls oncrpc_proxy_tasks_stop
  to stop all of the proxy tasks which have been started. When ONCRPC is
  (re)started, it calls oncrpc_proxy_tasks_start to restart all of the proxy
  tasks which were stopped.

 Copyright (c) 2003-2008 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                             EDIT HISTORY FOR MODULE

  $Header: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_proxy.h#4 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
05/26/09    rr     Add support for stopping proxy tasks
02/15/08    hn     Created a dedicated proxy for the client_call commands.
02/15/08    hn     Made all proxies same priority and reduced max to 5.
11/08/07    rr     Added oncrpc_proxy_lock_init to be init from proxy task
                   fixes race condition on init
11/05/07    ih     Added support for dynamic proxy task creation
10/08/07    ptm    Remove prototypes for functions that are now static.
08/22/07    ptm    Unified access to thread local storage.
07/16/07    ptm    Clean up featurization.
07/13/07    hn     Merged fixes from mainline. Have to temporarily relocate
                   handle_cmd_event() function and extern ones it references.
06/01/07    hn     svc_register_with_proxy is obsolete now.
05/15/07    RJS    Split OS specific parts into seperate files. 
                   Remove Int and task locks
05/04/07    hn     Added hooks for handling of msg source information for
                   future support of multiple sources.
12/05/06    ptm    Convert to pool of proxy tasks.
08/23/06    ptm    Remove STOP and START signals.
08/18/06    ptm    Increase the numer of command buffers to 20.
05/08/06    ptm    Change post_sync_sig to take the tcb as an argument.
03/20/06    ptm    Change task state from boolean to enum.
06/07/05    ptm    Added client call support.
04/26/05    hn     Added oncrpc_proxy_forward to support multi-processor STA
04/14/05    ptm    Include task.h for ONCRPC_PROXY_STACK_SIZ.
03/14/05    clp    Add definition for oncrpc_proxy_task_s_type when the proxy
                   tasks are not enabled so that the remainder of ONCRPC code
                   will compile.
01/19/05    ptm    Branched from oncrpc_api_init.h. Reduced content to just
                   oncrpc proxy related structures and prototypes.
01/13/05    hn     Added prototypes for init functions of export joystick
                   api's.
01/10/05    hn     Added the prototypes for the app_init routines of the misc.
                   modem and apps api's.
12/01/04    hn     Created this file.


===========================================================================*/

/*============================================================================

             TYPE DEFINITIONS

============================================================================*/



/*===========================================================================

        Data structure definitions for rpc proxy tasks

===========================================================================*/
#include "oncrpc_os.h"

typedef enum {
  ONCRPC_PROXY_CMD_RPC_CALL,
  ONCRPC_PROXY_CMD_FORWARD_RPC_CALL,
  ONCRPC_PROXY_CMD_CLIENT_CALL
} oncrpc_proxy_cmd_e_type;

typedef struct {
  oncrpc_proxy_cmd_e_type    type;
  void                     (*dispatch) (struct svc_req *, xdr_s_type *);
  struct svc_req             call;
  uint32                     xid;
  oncrpc_addr_type           msg_source;
  dsm_item_type             *in_msg;
  xdr_s_type                *xdr;
} oncrpc_proxy_cmd_rpc_call_type;

typedef struct {
  oncrpc_proxy_cmd_e_type   type;
  struct svc_req            call;
  uint32                    xid;
  dsm_item_type            *in_msg;
  opaque_auth               verf;
  xdr_s_type               *xdr;
} oncrpc_proxy_cmd_forward_rpc_call_type;

#define ONCRPC_PROXY_CLIENT_CALL_DATA_SIZE        (4)

typedef struct oncrpc_proxy_cmd_client_call_type
        oncrpc_proxy_cmd_client_call_type;

typedef void (*oncrpc_proxy_cmd_client_call_f_type)
                 ( oncrpc_proxy_cmd_client_call_type *ptr );

struct oncrpc_proxy_cmd_client_call_type {
  oncrpc_proxy_cmd_client_call_f_type client_call;
  uint32                              data[ONCRPC_PROXY_CLIENT_CALL_DATA_SIZE];
};

typedef struct {
  oncrpc_proxy_cmd_e_type             type;
  oncrpc_proxy_cmd_client_call_type   call_data;
} oncrpc_proxy_cmd_client_calli_type; //FIXME - need better name here

typedef union {
  oncrpc_proxy_cmd_e_type                type;
  oncrpc_proxy_cmd_rpc_call_type         rpc_call;
  oncrpc_proxy_cmd_forward_rpc_call_type forward_rpc_call;
  oncrpc_proxy_cmd_client_calli_type     client_call;
} oncrpc_proxy_cmd_type;

typedef struct oncrpc_proxyi_cmd_type {
  q_link_type link;
  oncrpc_proxy_cmd_type cmd;
} oncrpc_proxyi_cmd_type;

/* This data structure is obsolete. */
typedef void oncrpc_proxy_task_s_type;

/* Number of proxy tasks & buffers */
#define ONCRPC_MIN_NUM_PROXY_TASKS      1
#define ONCRPC_MAX_NUM_PROXY_TASKS      4
#define ONCRPC_PROXY_CMD_BUF_CNT        (3 * ONCRPC_MAX_NUM_PROXY_TASKS)

/* It would probably be OK for the server and client reply sig to be the
   same signal - but since these are local signals and there are plenty of
   of them left over - I decided not to overload the signal. PTM 1/24/05 */

#define ONCRPC_PROXY_CMD_Q_SIG          (0x0001)
#define ONCRPC_PROXY_SERVER_REPLY_SIG   (0x0002)
#define ONCRPC_PROXY_CLIENT_REPLY_SIG   (0x0004)
#define ONCRPC_PROXY_STOP_SIG           (0x0008)
#define ONCRPC_PROXY_WAIT_CNT           (50)      // the TMC_DEFAULT_WAIT_CNT


/*===========================================================================

            FUNCTIONAL INTERFACE

===========================================================================*/
extern void  oncrpc_proxy_task_start(void);
extern void   oncrpc_proxy_task_stop(void);
extern uint32 oncrpc_proxy_task_add( void *key, void *parent );
extern void   oncrpc_task_join(void *thread);
extern void  oncrpc_proxy_task( void * );
extern void  oncrpc_proxy_defer_task( void * );

/*===========================================================================
FUNCTION: oncrpc_proxy_init

DESCRIPTION:
   This function initializes the RPC proxy package defined in this file.

DEPENDENCIES: 
   This function must be called before any other function in this package.

ARGUMENTS: 
   None

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
extern void oncrpc_proxy_init( void );


/*===========================================================================
FUNCTION: oncrpc_proxy_dispatch

DESCRIPTION:
   This function queues an RPC call's service request to a specified RPC proxy
   task that then dispatches the RPC call to the remote service in its context
   instead of the context of the RPC task.

DEPENDENCIES: 
   This function must be called from the RPC task's RPC call handler.

ARGUMENTS: 
   rqstp        Service request structure representing the RPC call.
   srv          XDR transport structure for the server side of the RPC
                connection.
   dispatch     Dispatcher routine for the RPC program providing the remote
                service.
RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
extern void oncrpc_proxy_dispatch
(
  struct svc_req            *rqstp,
  xdr_s_type                *srv,
  void                     (*dispatch)(struct svc_req *, xdr_s_type *)
);

#ifndef FEATURE_ONCRPC_LO
/*===========================================================================
FUNCTION: oncrpc_proxy_forward

DESCRIPTION:
   This function queues an RPC call's service request to a specified RPC proxy
   task that then forwards the RPC call onto the shared memory transport using
   the shared memory protocol. The RPC call is forwarded in the context of
   the proxy task instead of the context of the RPC task.

DEPENDENCIES: 
   This function must be called from the RPC task's RPC call handler.

ARGUMENTS: 
   proxy        pointer to proxy task structure for task that will handle the
                RPC call.
   rqstp        Service request structure representing the RPC call.
   srv          XDR transport structure for the server side of the RPC
                connection.
RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
extern void oncrpc_proxy_forward
(
  struct svc_req           *rqstp,
  xdr_s_type               *srv
);
#endif /* !FEATURE_ONCRPC_LO */

/*===========================================================================
FUNCTION: svc_register_with_proxy (obsolete)

DESCRIPTION:
   This function is obsolete.

   Use svc_register() instead. See also svc_register_with_plugger().

DEPENDENCIES: 
   None

ARGUMENTS: 
  xdr          pointer to xdr structure for the transport
  prog         program number of the service
  vers         version number of the service
  dispatch     dispatch function to call for this service
  protocol     protocol type
  proxy        ***IGNORED***

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
extern bool_t svc_register_with_proxy (SVCXPRT *xdr, rpcprog_t prog,
			    rpcvers_t vers, __dispatch_fn_t dispatch,
			    rpcprot_t protocol,
                            oncrpc_proxy_task_s_type *proxy);

/*===========================================================================
FUNCTION: oncrpc_proxy_client_call

DESCRIPTION:
   Sends a command containing the client call structure to a proxy task.

   The function in the client call structure will be called by the proxy task
   with the client call structure as it's only argument. This function may then
   call a remoted API with the data contained in the client call structure.
   This allows remoted APIs to be called from the proxy task context instead
   of the client's context. This is useful when the client is running at
   interrupt level or has interrupts or tasks locked.

   The passed in client call structure is copied, so the caller need not
   keep the structure after this function returns. Typically, the passed in
   structure is a local variable in the caller's context.


DEPENDENCIES: 

ARGUMENTS: 
  msg          pointer to client call structure

RETURN VALUE: 
   None
  link_ptr->cmd.client_call.cmd = link_ptr;

SIDE EFFECTS: 
   A proxy task will call the function in the client structure
===========================================================================*/
void
oncrpc_proxy_client_call
(
  oncrpc_proxy_cmd_client_call_type *msg
);

/*===========================================================================
FUNCTION: oncrpc_proxy_client_call_get

DESCRIPTION:
   Obsolete - use oncrpc_proxy_client_call instead.

   Allocates a client call structure from the given proxy task.

DEPENDENCIES: 
   None

ARGUMENTS: 
  proxy        pointer to proxy task structure for task that will handle the
               RPC call.

RETURN VALUE: 
   Pointer to client call structure or NULL if none available.

SIDE EFFECTS: 
   None
===========================================================================*/
oncrpc_proxy_cmd_client_call_type *
oncrpc_proxy_client_call_get ( void );

/*===========================================================================
FUNCTION: oncrpc_proxy_client_call_send

DESCRIPTION:
   Obsolete - use oncrpc_proxy_client_call instead.

   Sends a command containing the client call structure to a proxy task.

   The function in the client call structure will be called by the proxy task
   with the client call structure as it's only argument. This function may then
   call a remoted API with the data contained in the client call structure.
   This allows remoted APIs to be called from the proxy task context instead
   of the client's context. This is useful when the client is running at
   interrupt level or has interrupts or tasks locked.

   The caller must not free the client call structure, this function will
   take care of that.

DEPENDENCIES: 
   The client call structure must have been obtained by calling
   oncrpc_proxy_client_call_get and the client call structure must be filled
   out correctly.

ARGUMENTS: 
  msg          pointer to client call structure

RETURN VALUE: 
   None

SIDE EFFECTS: 
   The proxy task, specified in the call to oncrpc_proxy_client_call_get,
   will call the function in the client structure
===========================================================================*/
void
oncrpc_proxy_client_call_send
(
  oncrpc_proxy_cmd_client_call_type *msg
);

/*===========================================================================
FUNCTION: oncrpc_proxy_client_call_free

DESCRIPTION:
   Return a client call structure to the free pool.

DEPENDENCIES: 
   The client call structure must have been obtained by calling
   oncrpc_proxy_client_call_get.

ARGUMENTS: 
  msg          pointer to client call structure

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void
oncrpc_proxy_client_call_free
(
  oncrpc_proxy_cmd_client_call_type *msg
);


/*===========================================================================
FUNCTION: oncrpc_proxy_lock_init

DESCRIPTION:
   Initialize the lock (critical section).

DEPENDENCIES: 
   None

ARGUMENTS: 
   None

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void oncrpc_proxy_lock_init ( void );

/*===========================================================================
FUNCTION: oncrpc_proxy_lock

DESCRIPTION:
   Lock interrupts or critical section for updating global variables in the 
   proxy module. Some functions can be invoked in ISR context in Rex; thus
   the need for INTLOCK instead of a critical section.

DEPENDENCIES: 
   None

ARGUMENTS: 
   None

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void oncrpc_proxy_lock ( void );

/*===========================================================================
FUNCTION: oncrpc_proxy_unlock

DESCRIPTION:
   Unlock interrupts or critical section for updating global variables in the 
   proxy module. Some functions can be invoked in ISR context in Rex; thus
   the need for INTLOCK instead of a critical section.

DEPENDENCIES: 
   None

ARGUMENTS: 
   None

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void oncrpc_proxy_unlock ( void );

#endif /* ! ONCRPC_PROXY_H */
