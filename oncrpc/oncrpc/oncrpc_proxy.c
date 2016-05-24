/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                      O N C R P C _ P R O X Y . C 

GENERAL DESCRIPTION
  ONC (Open Network Computing) RPC (Remote Procedure Calls) proxy task code.
  This file contains the code for managing ONCRPC proxy tasks and the code
  for the proxy tasks.

EXTERNALIZED FUNCTIONS
  oncrpc_proxy_init
    Initializes the data structures used by this package. Must be called
    before any other functions in this package.

  oncrpc_proxy_dispatch
    Forwards RPC calls from the RPC task to proxy tasks that dispatch the
    calls in their own context.

  oncrpc_proxy_forward
    Forwards RPC calls from the RPC task to proxy tasks that then forward the
    call in their own context onto the shared memory transport and forward
    the reply back to the original caller.

INITIALIZATION AND SEQUENCING REQUIREMENTS

  oncrpc_proxy_init must be called before any other functions in this package.

  The ONCRPC proxy tasks are controlled exclusively by the ONCRPC task. They
  are initially started the first time ONCRPC or a client trys to dispatch a
  command to the proxy task. In the case of a client, if the proxy task has not
  been started, a command is send to ONCRPC to start the proxy.

  When ONCRPC is stopped, it calls oncrpc_proxy_tasks_stop to stop all of the
  proxy tasks which have been started. When ONCRPC is (re)started, it calls
  oncrpc_proxy_tasks_start to restart all of the proxy tasks which were stopped.

 Copyright (c) 2003-2008, 2011-2012 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                             EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_proxy.c#3 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
28/07/11    as     Enable userspace logging
05/26/09    rr     Add support to stop proxy tasks
02/15/08    hn     No longer need the running queue and the critical section.
02/15/08    hn     Created a dedicated proxy for the client_call commands.
02/15/08    hn     Made all proxies same priority.
02/04/08    hn     Fixed STA forwarding with router.
12/14/07    ih     Fixed proxy task names of dynamically created tasks
11/21/07    ih     Fixed deadlock when handle command blocks before proxy_task_
                   started gets incremented, so no new proxy tasks are created
11/08/07    rr     Added oncrpc_proxy_lock_init to be init from proxy task
                   fixes race condition on init
11/05/07    ih     Added support for dynamic proxy task creation
10/31/07    ptm    Add code to set the proxy task name.
10/22/07    ptm    Add oncrpc-proxy-get-status.
10/08/07    ptm    Bring in functions that are no longer OS specific.
08/22/07    ptm    Unified access to thread local storage.
07/16/07    ptm    Clean up featurization.
07/13/07    hn     Merged fixes from mainline. Have to temporarily relocate
                   handle_cmd_event() function and extern ones it references.
05/15/07    RJS    Split OS specific parts into seperate files. 
                   Remove Int and task locks
05/04/07    hn     Added hooks for handling of msg source information for
                   future support of multiple sources.
04/30/07    hn     Sort priorities of proxy tasks based on who ran first so
                   that commands are handled in the order they're received.
01/09/07    ptm    Fix typo in ERR_FATAL.
12/05/06    ptm    Convert to pool of proxy tasks.
11/28/06   hn/ptm  Fixed deadlock issue between proxy tasks and the ONCRPC task
                   during proxy task init code and a race condition where a
                   proxy's xdr was being used before it was ready.
08/23/06    ptm    Remove STOP and START signals.
05/08/06    ptm    Change post_sync_sig to take the tcb as an argument.
03/22/06    ptm    Queue proxy command before calling
                   oncrpc_proxy_task_start_cmd.
03/20/06    ptm    Change task state from boolean to enum.
03/08/06    ptm    Move cmd queue init to struct init and fix task start in
                   oncrpc_proxy_client_call_send.
02/23/06    ptm    Change task to only look at signals it's waiting on.
01/25/06    ptm    Change to SMD client XDR that uses reply port.
08/17/05    ptm    Start unstarted proxy tasks in oncrpc_proxy_client_call_send
06/07/05    ptm    Added client call support.
04/26/05    hn     Added oncrpc_proxy_forward to support multi-processor STA
04/12/05    ptm    Include oncrpci.h and remove err.h.
04/01/05    clp    Include header cleanup changes.
03/14/05    clp    Fix debug message and remove unneeded cast.
03/03/05    hn     Added FEATURE ONCRPC SM protection around references to xdr
                   member in proxy task structure.
01/19/05    ptm    Branched from oncrpc_api_init.c. Reduced content to just
                   oncrpc proxy related structures and functions.
01/13/05    hn     Included timetest.h and fixed name of 'leds' field in
                   oncrpc_proxy_tasks_start() code.
                   Moved the proxy task's shared memory XDR client to the
                   structure of the proxy tasks and used it to also handle
                   the server side of RPC calls made to the proxy task.
                   Added init code for exported joystick api's.
01/10/05    hn     Defined ONCRPC_PROXY_REPLY_SIG to support RPC local reply
                   signals. And added init code for the misc. modem and apps
                   api's.
12/01/04    hn     Created this file.


===========================================================================*/


/*===========================================================================

                            INCLUDE FILES FOR MODULE

===========================================================================*/
#include "customer.h"
#include "target.h"

#include "queue.h"
#include "oncrpc.h"
#include "oncrpci.h"
#include "oncrpc_os.h"
#include "oncrpc_taski.h"
#include "oncrpc_proxy.h"

/*===========================================================================
                     LOCAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

static q_type                   proxy_task_ready_q;
static oncrpc_tls_type         *proxy_defer_task;

static oncrpc_proxyi_cmd_type  *proxy_cmd_buf;

static uint32                   proxy_task_count = 0;
static uint32                   proxy_task_started = 0;

static void *                   proxy_thread_keys[ONCRPC_MAX_NUM_PROXY_TASKS];
static oncrpc_crit_sect_ptr     proxy_task_start_stop_crit_sect;
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                      O N C R P C   P R O X Y   T A S K 

DESCRIPTION
  ONC (Open Network Computing) RPC (Remote Procedure Calls) Proxy Task.
  This is a proxy task used to run RPC servers of certain API's the services
  of which require the use of the RPC task (thus avoiding deadlocks) or take
  a long time to execute (thus avoiding compromising performance of RPC task)

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
#define MAX_FORWARD_SENDS 2
#define CLIENT_CALL_OFFSET \
    ((uint32)  &(((oncrpc_proxyi_cmd_type *)NULL)->cmd.client_call.call_data))

/* Proxy cmd queues*/
static q_type                   proxy_cmd_q;
static q_type                   proxy_cmd_free_q;
static q_type                   proxy_defer_cmd_q;


#if defined ONCRPC_DEBUG_ENABLE
#define DEBUG_PRINT(p...) do { \
	printf(p); \
} while (0)
#else
#define DEBUG_PRINT(p...) do { } while (0)
#endif

/*===========================================================================

          LOCAL FUNCTION DEFINITIONS

===========================================================================*/

/*===========================================================================
FUNCTION: oncrpc_proxy_task_init

DESCRIPTION: 
   This function initializes the data structures used by RPC proxy tasks.

DEPENDENCIES: 
   This function must be called before the proxy tasks get going and start
   receiving messages.

ARGUMENTS: 
   proxy        pointer to proxy task structure for this task

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void oncrpc_proxy_task_init( oncrpc_tls_type *tls )
{
  xdr_s_type *xdr;

  //RPC_MSG_MED( "RPC Proxy Task 0x%x powerup/init", (uint32) tls->thread, 0, 0 );
  
#if defined(FEATURE_ONCRPC_ROUTER) 
      /* Create router XDR for this proxy */
      tls->xdr = clntrtr_create(ONCRPC_PROXY_SERVER_REPLY_SIG);
      if( tls->xdr == NULL ) {
        ERR_FATAL( "Unable to create router memory client", 0, 0, 0 );
      }

      xdr = clntrtr_create(ONCRPC_PROXY_CLIENT_REPLY_SIG);
      if( xdr == NULL ) {
        ERR_FATAL( "Unable to create router memory client", 0, 0, 0 );
      }
      
      if( !rpc_clnt_register( xdr ) ) {
        ERR_FATAL( "Unable to register client", 0, 0, 0 );
      }
#endif /* defined(FEATURE_ONCRPC_ROUTER) */   

#if defined(FEATURE_ONCRPC_LO) 
      /* Create router XDR for this proxy */
      tls->xdr = clntlo_create(ONCRPC_PROXY_SERVER_REPLY_SIG);
      if( tls->xdr == NULL ) {
        ERR_FATAL( "Unable to create lo memory client", 0, 0, 0 );
      }

      xdr = clntlo_create(ONCRPC_PROXY_CLIENT_REPLY_SIG);
      if( xdr == NULL ) {
        ERR_FATAL( "Unable to create lo memory client", 0, 0, 0 );
      }
      
      if( !rpc_clnt_register( xdr ) ) {
        ERR_FATAL( "Unable to register client", 0, 0, 0 );
      }
#endif /* defined(FEATURE_ONCRPC_LO) */   

} /* oncrpc_proxy_task_init */


/*===========================================================================
FUNCTION: oncrpc_proxy_cmd_sig

DESCRIPTION:
   This function signals the first proxy task in the proxy task queue that
   there is a command available on the command queue. If there is no proxy
   task available, the next one that completes its current command will
   pick up the next command.

DEPENDENCIES:
   None

ARGUMENTS:
   None

RETURN VALUE:
   None

SIDE EFFECTS:
   None
===========================================================================*/
static void oncrpc_proxy_cmd_sig( void )
{
  oncrpc_tls_type *tls;

  /* ONCRPC_PROFILE_FREE_TASK( q_cnt(&proxy_task_q) ); */

  tls = (oncrpc_tls_type *) q_get(&proxy_task_ready_q);
  if( tls != NULL )
  {
    oncrpc_event_set(tls->thread, ONCRPC_PROXY_CMD_Q_SIG);
  }
  else
  {
     DEBUG_PRINT("No proxy tasks available, count:%d, started:%d, max:%d\n",
                 (unsigned int)proxy_task_count,
                 (unsigned int)proxy_task_started,
                 (unsigned int)ONCRPC_MAX_NUM_PROXY_TASKS);
    /* no proxy task available, dynamically create a task as necessary */
    oncrpc_proxy_lock();

    /* make sure all tasks we created have started before adding new ones. It
     * is possible that the newly created proxy task has not been scheduled
     * before we try to add another one
     */
    if( proxy_task_count == proxy_task_started &&
        proxy_task_count < ONCRPC_MAX_NUM_PROXY_TASKS )
    {
      uint32 count = proxy_task_count++;
      oncrpc_proxy_unlock();
      DEBUG_PRINT("Adding proxy task %d \n",(unsigned int)count);
      oncrpc_proxy_task_add(&proxy_thread_keys[count],0);
    }
    else
    {
      oncrpc_proxy_unlock();
    }
  }

} /* oncrpc_proxy_cmd_sig */


/*===========================================================================
FUNCTION: oncrpc_proxy_handle_cmd_rpc_call

DESCRIPTION: Handler for ONCRPC_PROXY_CMD_RPC_CALL commands.

DEPENDENCIES: Must be called from the context of the oncrpc proxy task that
              received the command passed in to this function.

ARGUMENTS: 
   Command that needs handling;

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
static void oncrpc_proxy_handle_cmd_rpc_call
(
  oncrpc_tls_type                *tls,
  oncrpc_proxy_cmd_rpc_call_type *rpc_call
)
{

  /* dispatch the call */
  if( rpc_call->xdr == NULL )
  {
    rpc_call->xdr          = tls->xdr;
    rpc_call->call.rq_xprt = tls->xdr;
  }

  XDR_MSG_SET_INPUT(rpc_call->xdr, &(rpc_call->in_msg));
  rpc_call->xdr->xid = rpc_call->xid;
  rpc_call->xdr->msg_source = rpc_call->msg_source;

  /* set x_op field for old encode/decode routines */
  rpc_call->xdr->x_op = XDR_DECODE;

  /* set protocol field for client died code */
  tls->protocol = rpc_call->xdr->protocol;

  ONCRPC_SYSTEM_LOGI("%s: Dispatching xid: %x\n", __func__, rpc_call->xid);
  rpc_call->dispatch( &rpc_call->call, rpc_call->xdr );
  ONCRPC_SYSTEM_LOGI("%s: Dispatch returned for xid: %x\n", __func__, rpc_call->xid);

} /* oncrpc_proxy_handle_cmd_rpc_call */


/*===========================================================================
FUNCTION: oncrpc_proxy_handle_cmd_forward_rpc_call

DESCRIPTION: Handler for ONCRPC_PROXY_CMD_FORWARD_RPC_CALL commands.

DEPENDENCIES: Must be called from the context of the oncrpc proxy task that
              received the command passed in to this function.

ARGUMENTS: 
   Command that needs handling;

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
static void oncrpc_proxy_handle_cmd_forward_rpc_call
(
  oncrpc_proxy_cmd_forward_rpc_call_type *forward_rpc_call
)
{
  opaque_auth       empty_auth   = { ONCRPC_AUTH_NONE, NULL, 0 };
  xdr_s_type       *clnt         = NULL;
  dsm_item_type    *item         = NULL;
  rpc_reply_header  reply_header;
  int               call_cnt     = 0;
  boolean           send_success = FALSE;

  /* Forward the RPC call on shared memory */
  /* Does retries a MAX_FORWARD_SENDS number of times */
  do
  {
    clnt = rpc_clnt_lookup2( forward_rpc_call->call.rq_prog,
                             forward_rpc_call->call.rq_vers,
                             0 );

    /* If the lookup failed, return noprog reply and exit */
    if ( clnt == NULL )
    {
      svcerr_noprog(forward_rpc_call->xdr);
      return;
    }
  
    /* Rebuild the RPC message header using the fields (prog, vers & proc) that
     * were already received, and use empty authentication since this call is
     * going to be between the two processors.
     */
    if ( ! xdr_call_msg_start( clnt,
                               forward_rpc_call->call.rq_prog,
                               forward_rpc_call->call.rq_vers,
                               forward_rpc_call->call.rq_proc,
                               &empty_auth,
                               &empty_auth ) )
    {
      ERR_FATAL( "oncrpc_proxy_handle_cmd_forward_rpc_call: msg_start failed",
                 0, 0, 0 );
    }

    /* Send the remaining body of the message */
    if ( ! XDR_SEND_DSM( clnt, &forward_rpc_call->in_msg ) )
    {
      ERR_FATAL( "oncrpc_proxy_handle_cmd_forward_rpc_call: SEND_DSM failed",
                 0, 0, 0 );
    }

    call_cnt++;
    send_success = XDR_MSG_SEND( clnt, &reply_header );
  } while ( ! send_success && call_cnt < MAX_FORWARD_SENDS );

  /* Grab the remainder of the reply message */
  XDR_RECV_DSM( clnt, &item );
  XDR_MSG_DONE( clnt );

  if ( reply_header.stat == RPC_MSG_ACCEPTED )
  {
    /* Insert the result of authenticating the original call back into the
     * reply
     */
    reply_header.u.ar.verf = forward_rpc_call->verf;
  }

  /* Send the reply message back to the original caller */
  if ( ! ( XDR_MSG_START( forward_rpc_call->xdr, RPC_MSG_REPLY ) &&
           xdr_send_reply_header( forward_rpc_call->xdr, &reply_header ) &&
           XDR_SEND_DSM( forward_rpc_call->xdr, &item ) &&
           XDR_MSG_SEND( forward_rpc_call->xdr, NULL ) ) )
  {
    ERR_FATAL( "oncrpc_proxy_handle_cmd_forward_rpc_call: send reply failed",
               0, 0, 0 );
  }
} /* oncrpc_proxy_handle_cmd_forward_rpc_call */


/*===========================================================================
FUNCTION: oncrpc_proxy_handle_cmd_client_call

DESCRIPTION: Handler for ONCRPC_PROXY_CMD_CLIENT_CALL commands.

DEPENDENCIES: Must be called from the context of the oncrpc proxy task that
              received the command passed in to this function.

ARGUMENTS: 
   Command that needs handling;

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
static void oncrpc_proxy_handle_cmd_client_call
(
  oncrpc_tls_type *tls
)
{
  oncrpc_proxyi_cmd_type             *cmd;
  oncrpc_proxy_cmd_client_calli_type *client_call;

  /* grab the current command off the queue */
  while ( NULL != ( cmd = q_get( &proxy_defer_cmd_q ) ) )
  {
    client_call = &(cmd->cmd.client_call);

    if( client_call->call_data.client_call == NULL )
    {
      ERR( "Client call cmd with NULL function", 0, 0, 0 );
    }
    else
    {
      client_call->call_data.client_call( &client_call->call_data );
    }

    q_put( &proxy_cmd_free_q, &cmd->link );
  }
} /* oncrpc_proxy_handle_cmd_client_call */


/*===========================================================================
FUNCTION: oncrpc_proxy_handle_cmd_event

DESCRIPTION:
   This function handles command events and dispatches them to the proper
   subsystem.

ARGUMENTS:
   proxy        pointer to proxy task structure for this task

RETURN VALUE:
   None

SIDE EFFECTS:
   None
===========================================================================*/
static void oncrpc_proxy_handle_cmd_event ( oncrpc_tls_type *tls )
{
  oncrpc_proxyi_cmd_type         *cmd;

  /* grab the current command off the queue */
  while ( NULL != ( cmd = q_get( &proxy_cmd_q ) ) )
  {
    switch( cmd->cmd.type )
    {
      case ONCRPC_PROXY_CMD_RPC_CALL:
        oncrpc_proxy_handle_cmd_rpc_call( tls, &cmd->cmd.rpc_call );
        break;

      case ONCRPC_PROXY_CMD_FORWARD_RPC_CALL:
        oncrpc_proxy_handle_cmd_forward_rpc_call( &cmd->cmd.forward_rpc_call );
        break;

      default:
        ERR_FATAL( "Invalid command type %d", cmd->cmd.type, 0, 0 );
    }

    /* Return command to the cmd_free_q */
    q_put( &proxy_cmd_free_q, &cmd->link );
  }


} /* oncrpc_proxy_handle_cmd_event */


/*===========================================================================

          EXTERNAL FUNCTION DEFINITIONS

===========================================================================*/

/*===========================================================================
FUNCTION: oncrpc_proxy_dispatch

DESCRIPTION:
   This function queues an RPC call's service request to a specified RPC proxy
   task that then dispatches the RPC call to the remote service in its context
   instead of the context of the RPC task.

DEPENDENCIES: 
   This function must be called from the RPC task's RPC call handler.

ARGUMENTS: 
   proxy        pointer to proxy task structure for task that will handle the
                RPC call.
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
void oncrpc_proxy_dispatch
(
  struct svc_req            *rqstp,
  xdr_s_type                *srv,
  void                     (*dispatch)(struct svc_req *, xdr_s_type *)
)
{
  oncrpc_proxyi_cmd_type *link_ptr;
  dsm_item_type *item = NULL;

  /* Get link to free command buffer */
  if( (link_ptr = (oncrpc_proxyi_cmd_type *) q_get(&proxy_cmd_free_q)) == NULL )
  {
    ERR_FATAL( "RPC Proxy Task Command link not available", 0, 0, 0);
  }
  else // Assign to the link, put link onto queue and signal the proxy task.
  {
    link_ptr->cmd.rpc_call.type     = ONCRPC_PROXY_CMD_RPC_CALL;
    link_ptr->cmd.rpc_call.dispatch = dispatch;
    link_ptr->cmd.rpc_call.call     = *rqstp;

    /* The RPC message itself is copied into the command structure instead of
     * keeping it in an XDR structure. This prevents RPC calls from stepping
     * over each others' messages once more than one call is pending on the
     * proxy's queue.
     */
    XDR_RECV_DSM(srv, &item);
    XDR_MSG_DONE(srv);
    link_ptr->cmd.rpc_call.in_msg = item;
    link_ptr->cmd.rpc_call.xid    = srv->xid;
    link_ptr->cmd.rpc_call.msg_source = srv->msg_source;

    if ( srv->protocol == ONCRPC_SM_PROTOCOL ||
         srv->protocol == ONCRPC_RTR_PROTOCOL ||
         srv->protocol == ONCRPC_LO_PROTOCOL )
    {
      /* If the RPC call came on a multiplexed transport, then the proxy task's
       * own svc xdr is used to decode the message. This allows the RPC task to
       * continue receiving other RPC messages from the multiplexed transport
       * using its own svc xdr. So we set xdr to NULL to tell the proxy task to
       * use its own svc xdr.
       */
      link_ptr->cmd.rpc_call.xdr = NULL;
    }
    else
    {
      /* For other transports, we use the RPC task's xdr structure
       * since the RPC task cannot receive any RPC calls until this RPC
       * call returns its reply. The RPC task's xdr is specified in the cmd.
       */
      link_ptr->cmd.rpc_call.xdr = srv;
    }

    q_put( &proxy_cmd_q, &link_ptr->link);
    oncrpc_proxy_cmd_sig( );
  }
} /* oncrpc_proxy_dispatch */


#ifdef FEATURE_ONCRPC_ROUTER
/*===========================================================================
FUNCTION: oncrpc_proxy_forward

DESCRIPTION:
   This function queues an RPC call's service request to a specified RPC proxy
   task that then forwards the RPC call the shared memory transport using the
   shared memory protocol. The RPC call is forwarded in the context of
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
void oncrpc_proxy_forward
(
  struct svc_req           *rqstp,
  xdr_s_type               *srv
)
{
  oncrpc_proxyi_cmd_type *link_ptr = NULL;
  dsm_item_type          *item     = NULL;
  
  /* Get link to free command buffer */
  if( (link_ptr = (oncrpc_proxyi_cmd_type *) q_get(&proxy_cmd_free_q)) == NULL )
  {
    ERR_FATAL( "RPC Proxy Task Command link not available", 0, 0, 0);
  }
  else // Assign to the link, put link onto queue and signal the proxy task.
  {
    link_ptr->cmd.forward_rpc_call.type = ONCRPC_PROXY_CMD_FORWARD_RPC_CALL;

    XDR_RECV_DSM(srv, &item);
    XDR_MSG_DONE(srv);

    link_ptr->cmd.forward_rpc_call.in_msg = item;
    link_ptr->cmd.forward_rpc_call.xid    = srv->xid;
    link_ptr->cmd.forward_rpc_call.call   = *rqstp;
    link_ptr->cmd.forward_rpc_call.xdr    = srv;
    
    /* For now, we will settle for doing a shallow copy of the verf structure,
     * but in the future as we add more auth flavours we might have to do a
     * deep copy and then we will need to worry about alloc/dealloc issues.
     */
    link_ptr->cmd.forward_rpc_call.verf           = srv->verf;

    q_put( &proxy_cmd_q, &link_ptr->link);
    oncrpc_proxy_cmd_sig( );
  }
} /* oncrpc_proxy_forward */
#endif /* FEATURE_ONCRPC_ROUTER */


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
  cmd          pointer to client call structure

RETURN VALUE: 
   None

SIDE EFFECTS: 
   A proxy task will call the function in the client structure
===========================================================================*/
void
oncrpc_proxy_client_call
(
  oncrpc_proxy_cmd_client_call_type *cmd
)
{
  oncrpc_proxyi_cmd_type *link_ptr;

  if( cmd == NULL || cmd->client_call == NULL )
  {
    ERR_FATAL( "Invalid cmd pointer or client function pointer", 0, 0, 0 );
  }

  link_ptr = (oncrpc_proxyi_cmd_type *) q_get(&proxy_cmd_free_q);

  if( link_ptr == NULL )
  {
    ERR_FATAL( "Out of proxy cmd buffers", 0, 0, 0 );
  }

  link_ptr->cmd.client_call.type = ONCRPC_PROXY_CMD_CLIENT_CALL;
  link_ptr->cmd.client_call.call_data = *cmd;

  q_put( &proxy_defer_cmd_q, &link_ptr->link );

  ASSERT( proxy_defer_task != NULL );
  oncrpc_event_set(proxy_defer_task->thread, ONCRPC_PROXY_CMD_Q_SIG);
} /* oncrpc_proxy_client_call */


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
oncrpc_proxy_client_call_get( void )
{
  oncrpc_proxyi_cmd_type *link_ptr;

  link_ptr = (oncrpc_proxyi_cmd_type *) q_get(&proxy_cmd_free_q);

  if( link_ptr == NULL )
  {
    return NULL;
  }

  link_ptr->cmd.client_call.type = ONCRPC_PROXY_CMD_CLIENT_CALL;
  link_ptr->cmd.client_call.call_data.client_call = NULL;

  return &link_ptr->cmd.client_call.call_data;
} /* oncrpc_proxy_client_call_get */


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
)
{
  oncrpc_proxyi_cmd_type *link_ptr;

  if( msg == NULL )
  {
    ERR_FATAL( "Invalid msg pointer", 0, 0, 0 ); 
  }

  if( msg->client_call == NULL )
  {
    ERR_FATAL( "Invalid client function pointer", 0, 0, 0 );
  }

  /* Pointer arithmetic to get back to cmd */
  /*lint -e413 Likely use of null pointer 'unknown-name 
    CLIENT_CALL_OFFSET MACRO, computes a constant from null*/
  link_ptr = (oncrpc_proxyi_cmd_type *) ((char *)msg - CLIENT_CALL_OFFSET);
  /*lint +e413*/

  q_put( &proxy_defer_cmd_q, &link_ptr->link );

  ASSERT( proxy_defer_task != NULL );
  oncrpc_event_set(proxy_defer_task->thread, ONCRPC_PROXY_CMD_Q_SIG);
} /* oncrpc_proxy_client_call_send */


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
)
{
  oncrpc_proxyi_cmd_type *link_ptr;

  if( msg == NULL )
  {
    ERR_FATAL( "Invalid msg pointer", 0, 0, 0 ); 
  }

  /* Pointer arithmetic to get back to cmd */
  /*lint -e413 Likely use of null pointer 'unknown-name 
    CLIENT_CALL_OFFSET MACRO, computes a constant from null*/
  link_ptr = (oncrpc_proxyi_cmd_type *) ((char *)msg - CLIENT_CALL_OFFSET);
  /*lint +e413 */

  q_put( &proxy_cmd_free_q, &link_ptr->link );
} /* oncrpc_proxy_client_call_free */


/*===========================================================================
FUNCTION: oncrpc_proxy_init

DESCRIPTION:
   This function initializes the RPC proxy package defined in this file.

DEPENDENCIES: 
   This function must be called before any other function in this package,
   and that the ONCRPC task priority comes immediately after the ONCRPC
   proxy tasks priorities in the task priority enum defined in task.h

ARGUMENTS: 
   None

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void oncrpc_proxy_init( void )
{
  int32 i;

  /* Initialise proxy cmd queue */
  (void) q_init ( &proxy_cmd_q );
  (void) q_init ( &proxy_cmd_free_q );
  (void) q_init ( &proxy_defer_cmd_q );

  /* Build the command queue */
  proxy_cmd_buf = (oncrpc_proxyi_cmd_type *)
    oncrpc_mem_alloc(sizeof(oncrpc_proxyi_cmd_type) * ONCRPC_PROXY_CMD_BUF_CNT);

  for ( i = 0; i < ONCRPC_PROXY_CMD_BUF_CNT; i++ )
  {
    q_put( &proxy_cmd_free_q,
           q_link( &proxy_cmd_buf[i], &proxy_cmd_buf[i].link ) );
  }

  /* initialize proxy lock (happens only once) */
  oncrpc_proxy_lock_init();

  if( proxy_task_start_stop_crit_sect == 0 )
  {
      oncrpc_crit_sect_init( &proxy_task_start_stop_crit_sect );
  }


  q_init ( &proxy_task_ready_q );

  DEBUG_PRINT("Proxy init done, %d tasks running\n",(unsigned int)proxy_task_count);
} /* oncrpc_proxy_init */


/*===========================================================================
FUNCTION: oncrpc_proxy_task

DESCRIPTION:
   The ONCRPC proxy task main function, this function processes signals and
   events for the task.

DEPENDENCIES:
   None.

ARGUMENTS:
   None.

RETURN VALUE:
   None

SIDE EFFECTS:
   None
===========================================================================*/
void oncrpc_proxy_task( void *parent_thread_handle )
{
  oncrpc_tls_type *tls;
  oncrpc_thread_handle thread;
  oncrpc_event_t event;
  char name[ONCRPC_THREAD_NAME_SIZE];

  /* Get the tls and thread information */
  tls = oncrpc_tls_get_self();
  thread = tls->thread;
  tls->state = TASK_STATE_RUNNING;
//  RPC_MSG_HIGH( "Proxy task %p running", tls, 0, 0 );

#if ONCRPC_THREAD_NAME_SIZE < 10
#error "ONCRPC_THREAD_NAME_SIZE too small"
#endif

  if( tls->name[0] == '\0' )
  {
    memcpy( name, "PROXY ", MIN(ONCRPC_THREAD_NAME_SIZE, 6) );
    name[6] = '0' + (char)((uint32) tls/100 % 10);
    name[7] = '0' + (char)((uint32) tls/10 % 10);
    name[8] = '0' + (char)((uint32) tls % 10);
    name[9] = '\0';

    oncrpc_set_task_name( name );
  }

  /* init data structures */
  oncrpc_proxy_task_init( tls );

  /* Increment proxy_task_started before handle commands in case it blocks */
  oncrpc_proxy_lock();
  proxy_task_started++;
  oncrpc_proxy_unlock();

  /* Handle commands that were queued before any proxy tasks were put in
     the task queue */
  oncrpc_proxy_handle_cmd_event( tls );
  /* Add self to the ready task queue*/

  q_put( &proxy_task_ready_q, &tls->link );
  DEBUG_PRINT("Num Proxy's in Queue: %d \n",(unsigned int)q_cnt(&proxy_task_ready_q));

  if(parent_thread_handle)
  {
     oncrpc_event_set((oncrpc_thread_handle)parent_thread_handle,RPC_SYNC_SIG);
  }
  for( ;; )
  {
    /* Wait on event */
    event = oncrpc_event_wait( thread, ONCRPC_PROXY_CMD_Q_SIG | ONCRPC_PROXY_STOP_SIG );

    oncrpc_event_clr( thread, event );

    /* Process the event */
    if(event & ONCRPC_PROXY_CMD_Q_SIG)
    {
       /* Handle this event even if stop is requested */
    oncrpc_proxy_handle_cmd_event( tls );
  }

    if(event & ONCRPC_PROXY_STOP_SIG)
    {
       break;
    }
    else
    {
        /* Add self to the ready task queue*/
       q_put( &proxy_task_ready_q, &tls->link );
    }
  }
  DEBUG_PRINT("Proxy task exiting tls 0x%08x thread 0x%08x\n",(unsigned int)tls,(unsigned int)tls->thread);

   /* Remove item from ready queue */
   q_delete(&proxy_task_ready_q, &tls->link);

   tls->state = TASK_STATE_STOPPED;
   oncrpc_proxy_lock();
   proxy_task_count--;
   oncrpc_proxy_unlock();
   oncrpc_tls_delete_self();
} /* oncrpc_proxy_task */

/*===========================================================================
FUNCTION: oncrpc_proxy_task_start

DESCRIPTION:
   Start proxy tasks up to ONCRPC_MIN_NUM_PROXY_TASKS, using the current
   number of started tasks.

DEPENDENCIES:
   None.

ARGUMENTS:
   None.

RETURN VALUE:
   None

SIDE EFFECTS:
   uses proxy interlock
===========================================================================*/
void oncrpc_proxy_task_start(void)
{
   uint32 i;
   oncrpc_thread_handle this_thread_handle;
   this_thread_handle = oncrpc_thread_handle_get();

   oncrpc_event_clr(this_thread_handle,RPC_SYNC_SIG);
   /* Initialise and start all proxy tasks*/
   oncrpc_crit_sect_enter( proxy_task_start_stop_crit_sect );
   for(i = proxy_task_count; i < ONCRPC_MIN_NUM_PROXY_TASKS; i++)
   {
      ONCRPC_SYSTEM_LOGI("\nStarting proxy task %d %p\n",(unsigned int)i, oncrpc_tls_get_self());
      if(! oncrpc_proxy_task_add(&proxy_thread_keys[i],this_thread_handle) )
      {
         ONCRPC_ERR("Error starting proxy task:%d \n",i,0,0);
      }
      else
      {
         DEBUG_PRINT("Started proxy waiting .... \n");
         oncrpc_event_wait(this_thread_handle,RPC_SYNC_SIG);
         oncrpc_event_clr(this_thread_handle,RPC_SYNC_SIG);
         DEBUG_PRINT("Started proxy got Signal .... \n");
         oncrpc_proxy_lock();
         proxy_task_count++;
         oncrpc_proxy_unlock();
      }
   }
  oncrpc_crit_sect_leave( proxy_task_start_stop_crit_sect );
}

/*===========================================================================
FUNCTION: oncrpc_proxy_task_stop

DESCRIPTION:
   Stop all currenly running proxy tasks.

DEPENDENCIES:
   None.

ARGUMENTS:
   None.

RETURN VALUE:
   None

SIDE EFFECTS:
   None
===========================================================================*/
void oncrpc_proxy_task_stop(void)
{
   int32 i;
   uint32 num_proxy_tasks;
   oncrpc_tls_type *tls;

   oncrpc_crit_sect_enter( proxy_task_start_stop_crit_sect );
   num_proxy_tasks = proxy_task_started;
   for(i=0; i < num_proxy_tasks; i++)
   {
      DEBUG_PRINT("Stopping proxy task %d, key 0x%08x \n",(unsigned int)i,(unsigned int)proxy_thread_keys[i]);
      tls = oncrpc_tls_find(proxy_thread_keys[i]);
      if(! tls)
      {
         ONCRPC_ERR("TLS not found in oncrpc_proxy_task_stop, cannot stop proxy task %d \n",(unsigned int)i,0,0);
      }
      else
      {
         DEBUG_PRINT("Stopping proxy task %d \n",(unsigned int)i);
         oncrpc_event_set(tls,ONCRPC_PROXY_STOP_SIG);
         oncrpc_task_join(proxy_thread_keys[i]);
         oncrpc_proxy_lock();
         proxy_task_started--;
         oncrpc_proxy_unlock();
         DEBUG_PRINT("Stopping proxy task count:%d started:%d \n",(unsigned int)proxy_task_count,(unsigned int)proxy_task_started);
         oncrpc_thread_exit(proxy_thread_keys[i]);
      }
   }
   oncrpc_crit_sect_leave( proxy_task_start_stop_crit_sect );
}

/*===========================================================================
FUNCTION: oncrpc_proxy_defer_task

DESCRIPTION:
   The ONCRPC proxy defer task main function, this function processes signals
   and events for the task.

DEPENDENCIES:
   None.

ARGUMENTS:
   None.

RETURN VALUE:
   None

SIDE EFFECTS:
   None
===========================================================================*/
void oncrpc_proxy_defer_task( void * ignored )
{
  oncrpc_tls_type *tls;
  oncrpc_thread_handle thread;
  char name[ONCRPC_THREAD_NAME_SIZE];

  /* Get the tls and thread information */
  tls = oncrpc_tls_get_self();
  thread = tls->thread;
  tls->state = TASK_STATE_RUNNING;
  //RPC_MSG_HIGH( "Proxy task %p running", tls, 0, 0 );

#if ONCRPC_THREAD_NAME_SIZE < 10
#error "ONCRPC_THREAD_NAME_SIZE too small"
#endif

  if( tls->name[0] == '\0' )
  {
    memcpy( name, "PROXY DFR", MIN(ONCRPC_THREAD_NAME_SIZE, 9) );
    name[9] = '\0';

    oncrpc_set_task_name( name );
  }

  /* init data structures */
  oncrpc_proxy_task_init( tls );

  /* Destroy the server xdr for this proxy since it's not used */
  XDR_DESTROY( tls->xdr );
  tls->xdr = NULL;

  proxy_defer_task = tls;

  /* Handle commands that were queued before any proxy tasks were put in
     the task queue */
  oncrpc_proxy_handle_cmd_client_call( tls );

  for( ;; )
  {
    /* Wait on event */
    oncrpc_event_wait( thread, ONCRPC_PROXY_CMD_Q_SIG );
    oncrpc_event_clr( thread, ONCRPC_PROXY_CMD_Q_SIG );

    /* Process the event */
    oncrpc_proxy_handle_cmd_client_call( tls );
  }
} /* oncrpc_proxy_defer_task */
