/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                      O N C R P C M A I N . C 

GENERAL DESCRIPTION
  ONC (Open Network Computing) RPC (Remote Procedure Calls) task code.
  This file deals with the dispatch of events to other RPC parts and
  implements various accounting functions that couldn't be done the
  original way due to memory system constrains. (lack of malloc/free).

INITIALIZATION AND SEQUENCING REQUIREMENTS

  oncrpc_task is started from mc in the normal way.

 Copyright (c) 2002-2011 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_main.c#9 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
28/07/11    as     Enable userspace logging
04/06/09    rr     Add oncrpc_task_stop to deinit
02/26/09    zp     Added SMEM log init and deinit
02/23/09    rr     Remove diag deinit since it hangs
05/12/08    ih     Added oncrpc_deinit
05/08/08    ih     Removed oncrpc_remote_apis_inited
09/11/08    rr     Merge from mainline, cleanup includes
04/21/08    sd     Added oncrpc_get_last_sent_msg_info() and 
                   oncrpc_set_last_sent_msg_info()
03/25/08    ih     Add tls_find_common
01/30/08    rr     Add protection agains re-initialization oncrpc_init.
01/16/08    rr     Add rpc_fake_reply as a function, it's needed externally.
12/18/07    al     Fixed the reply message sent to tasks in the reply queue.
11/30/07    ptm    Add oncrpc_strncpy.
11/09/07    rr     Add error handling if tls is not initialize or out of space.
10/31/07    ptm    Add task name APIs.
10/16/07    ptm    Merge tls-header-type with tls-type, remove tls-common-type
                   and remove sync sig routines (not used anywhere).
10/08/07    ptm    Clean up oncrpc thread handle references.
10/01/07    rr     Added support for cleaning reply queue when server restarts.
08/20/07    ptm    Added common tls APIs.
07/10/07    ptm    Remove featurization, remove reply task code and move DSM
                   item related code to OS specific files.
04/02/07    ddh    Added API to check for outstanding transactions.
03/23/07    hn     Added initialization routine for callback registry.
03/20/07    ptm    Increase the size of the DSM pool to handle large EFS writes.
02/27/07    ptm/hn No longer count commands to prevent flooding the rpc task's
                   command queue. Instead use a pending flag.
01/11/07    mjb    Fixed wrong RPC-version causing wrong RPC error message.
12/04/06    ptm    Remove forward proxy task.
11/01/06    hwu    Changed to alloc oncrpc_dsm_item_array out of shared
                   slots for WinMob.
10/09/06    hn     Put back auto call retry hook in state machine because
                   it's needed after a connection is lost and reinited.
10/03/06    hn     Removed automatic call retry hook from state machine.
                   Added support for locking/unlocking RPC services.
08/23/06    ptm    Moved oncrpc dsm item pool to this file.
08/22/06    ptm    Merge WinMobile changes to main line.
08/22/06    ptm    Ignore STOP sig.
06/19/06    ptm    Change post_sync_sig to take the tcb as an argument.
05/29/06    ptm    Added oncrpc_retry_call fucntion.
04/21/06    ptm    Set x_op field of reply XDR to support STA.
03/20/06    ptm    Change reply task to always handle the START sig and delete
                   unneeded call to start proxy tasks.
02/22/06    ptm    Move reply task start to be before oncrpc_init.
01/25/06    ptm    Add support for SM reply port including the reply task.
12/22/05    ~SN    Removed redundant XDR_DESTROY for non blocking rpc calls.
09/13/05    clp    Featurized the include of oncrpc_proxy.h
07/13/05    clp    Added cmd RPC_DESTROY handling.
06/09/05    ptm    Add block comments.
05/16/05    hn     Wrapped call to xprtsm_apis_inited with FEATURE_SMSM
05/09/05    clp    Added plugger.
05/09/05    ptm    Add cmd_cnt to keep race condition from filling cmd queue.
04/26/05    hn     Added multi-processor support for STA.
04/22/05    ptm    Reorganize multiprocessor initialization.
04/13/05    ptm    Added reply queue code.
04/12/05    ptm    Include oncrpci.h and remove err.h.
03/25/05    ptm    Remove include of oncrpc_sm.h and a fix featurization of
                   include of oncrpc_sio.h.
03/22/05    clp    Changed call to reply cb for non-blocking clnt calls.
03/22/05    hn     Modified the rpc_handle_rpc_call() routine to send back
                   correct error replies for failure cases. Removed code for
                   receiving the reply header from rpc_handle_rpc_reply(), now
                   is handled by clients.
03/03/05    clp    Featurized more of the proxy stuff.
01/28/05    ptm    Change proxy tasks to use proxy handle instead of id and
                   remove remnants of sta remote task code. Change task init
                   so that a re-start does not register the RPC services.
01/18/04    ~SN    Removed xprt_register/unregister/find & moved the
                   functionality into oncrpcsvc_tcp.c. These functions
                   are now called socket_register/unregister/find 
                   in oncrpcsvc_tcp.c.
11/30/04    ptm    Increase the number of command buffers from 5 to 32.
08/19/03    ifk    Added sta_post_signal() and sta_process_signal().
02/13/03    mjb    Changed FEATURE DSPE RPC to FEATURE ONCRPC NATIVE for LTK
                   Made other changes to better integrate LTK.
10/10/02    igt    Added back FEATURE TMC TASK for backward compatibility
10/09/02    igt    Obsoleted FEATURE TMC TASK with FEATURE MULTIMODE ARCH
12/17/01    clp    Added header and comments to source file.
02/13/02    clp    Move initialization stuff to oncrpcinit.c


===========================================================================*/


/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "string.h"


#include "comdef.h"
#include "oncrpc_dsm.h"
#include "oncrpc.h"
#include "oncrpci.h"
#include "oncrpc_os.h"
#include "oncrpc_main.h"
#include "oncrpc_pacmark.h"
#include "oncrpc_task.h"
#include "oncrpc_taski.h"
#include "oncrpc_svc_auth.h"

#include "oncrpc_lookup.h"

#include "oncrpc_rtr.h"
#ifdef FEATURE_ONCRPC_DIAG
  #include "oncrpc_diag.h"
#endif /* FEATURE_ONCRPC_DIAG */

#ifdef FEATURE_ONCRPC_LO
  #include "oncrpc_lo.h"
#endif /* FEATURE_ONCRPC_LO */

#include "oncrpc_plugger_init.h"
#include "oncrpc_proxy.h"
#include "diag_lsm.h"
/*===========================================================================

            LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE

===========================================================================*/

/* For now we have a small number of programs registered, and a small
 * number of xprt's, so we will do linear searches for now
 */
#define RPC_MAX_PROGS 128

typedef struct 
{
  q_link_type link;
  rpc_cmd_type cmd;
} rpci_cmd_type;

/* For locking */
static oncrpc_crit_sect_ptr  oncrpc_task_crit_sect;

/* Command buffers and queues */
#define RPC_CMD_BUF_CNT (32)
static rpci_cmd_type rpc_cmd_buf[ RPC_CMD_BUF_CNT ];

static q_type    oncrpc_cmd_q;
static q_type    oncrpc_cmd_free_q;

static uint32    oncrpc_xid;

static q_type    oncrpc_reply_queue;
static oncrpc_crit_sect_ptr  oncrpc_reply_queue_crit_sect;

static pthread_mutex_t oncrpc_init_mutex = PTHREAD_MUTEX_INITIALIZER;
static boolean oncrpc_inited = FALSE;

static uint32  last_prog;
static uint32  last_vers;
static uint32  last_proc;
/*= = = = = = = =  = = = = = = = = = =  = = =  = = = = = = = = = = = = = = =
                        FUNCTION DEFINTIONS
= = = = = = = = = = = = = = =  = = = = =  = = = = = = = = = = = = = = = = =*/

/* --------------------------------------------------------------------- *
 * Functions that need replacing from svc.c for the task model
 * --------------------------------------------------------------------- */

/* --------------------------------------------------------------------- *
 * xprt's (or more specifically SVCXPRT's) are service transport
 * structures.  They describe the transport for dispatching events. 
 * 
 * The xprt functions start/stop us using this transport. These are
 * called from the transport code.
 *
 * Normally, these would be referenced by fd in an indexed array, but
 * we neither want to spend the memory on it, nor do we have a unique
 * fd space, as SIO and socket fd's may overlap.
 *
 * So, instead we extend the SVCXPRT with a singly linked list and
 * just hunt the list since it is expected to be "small".  We also add
 * the protocol... 
 * --------------------------------------------------------------------- */

/*===========================================================================

FUNCTION ONCRPC_NEXT_XID

DESCRIPTION
  Returns the next RPC message ID, known as the XID. Using the routine
  guarantees that each message has a unique XID. This is important because
  the XID is used to match replies with calls.

DEPENDENCIES
  oncrpc_init should be called before this routine is used.

RETURN VALUE
  The XID value

SIDE EFFECTS
  Updates state of XID counter.

===========================================================================*/
uint32
oncrpc_next_xid( void )
{
  uint32 xid;

  oncrpc_crit_sect_enter(oncrpc_task_crit_sect);
  xid = oncrpc_xid++;
  oncrpc_crit_sect_leave(oncrpc_task_crit_sect);

  return xid;
} /* oncrpc_next_xid */

/*===========================================================================

FUNCTION ONCRPC_INIT_REPLY_QUEUE

DESCRIPTION
  Initialize the reply queue. The reply queue is a list of XDRs that are
  waiting for replies. The incoming replies are matched with waiting XDRs by
  matching XIDs

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
static void
oncrpc_init_reply_queue( void )
{
  oncrpc_crit_sect_enter(oncrpc_reply_queue_crit_sect);
  (void) q_init( &oncrpc_reply_queue );
  oncrpc_crit_sect_leave(oncrpc_reply_queue_crit_sect);
} /* oncrpc_init_reply_queue */

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
void
oncrpc_queue_reply
(
  xdr_s_type *xdr               /* pointer to xdr to enqueue */
)
{
  oncrpc_crit_sect_enter(oncrpc_reply_queue_crit_sect);
  q_put( &oncrpc_reply_queue, &xdr->link );
  oncrpc_crit_sect_leave(oncrpc_reply_queue_crit_sect);
} /* oncrpc_queue_reply */

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
)
{
  if ( Q_ALREADY_QUEUED( &xdr->link ) )
  {
    oncrpc_crit_sect_enter(oncrpc_reply_queue_crit_sect);
    q_delete( &oncrpc_reply_queue, &xdr->link );
    oncrpc_crit_sect_leave(oncrpc_reply_queue_crit_sect);
  }
} /* oncrpc_queue_reply_remove */

/*===========================================================================

FUNCTION ONCRPC_XID_CMP

DESCRIPTION
  Compare function used by ONCRPC_FIND_REPLY to compare an XID to the XID
  stored in the XDR.

DEPENDENCIES
  None

RETURN VALUE
  TRUE if the XID in the XDR matches the given XID.

SIDE EFFECTS
  None

===========================================================================*/
static int
oncrpc_xid_cmp
(
  void *item_ptr,               /* XDR pointer */
  void *compare_val             /* XID value */
)
{
  xdr_s_type *xdr = (xdr_s_type *) item_ptr;
  uint32      xid = (uint32) compare_val;

  return xdr->xid == xid;
} /* oncrpc_xid_cmp */

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
)
{
  xdr_s_type *reply;

  oncrpc_crit_sect_enter(oncrpc_reply_queue_crit_sect);
  reply = q_linear_search( &oncrpc_reply_queue, oncrpc_xid_cmp, (void *) xid );

  if( reply != NULL ) {
    q_delete( &oncrpc_reply_queue, &reply->link );
  }
  oncrpc_crit_sect_leave(oncrpc_reply_queue_crit_sect);

  return reply;
} /* oncrpc_find_reply */

/*===========================================================================

FUNCTION ONCRPC_PROTO_CMP

DESCRIPTION
  Compare function used by ONCRPC_RETRY_CALL to compare a protocol to the 
  protocol stored in the XDR.

DEPENDENCIES
  None

RETURN VALUE
  TRUE if the protocol in the XDR matches the given protocol.

SIDE EFFECTS
  None

===========================================================================*/
static int
oncrpc_proto_cmp
(
  void *item_ptr,               /* XDR pointer */
  void *compare_val             /* protocol value */
)
{
  xdr_s_type *xdr = (xdr_s_type *) item_ptr;
  rpcprot_t   protocol = (uint32) compare_val;

  return xdr->protocol == protocol;
} /* oncrpc_proto_cmp */

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
)
{
  xdr_s_type *reply;

  oncrpc_crit_sect_enter(oncrpc_reply_queue_crit_sect);

  while( (reply = q_linear_search( &oncrpc_reply_queue,
                                   oncrpc_proto_cmp,
                                   (void *) protocol )) != NULL )
  {
    q_delete( &oncrpc_reply_queue, &reply->link );

    reply->flags |= XDR_FLAG_RETRY;

    if( reply->reply_cb != NULL ) {
      /* Nonblocking client - use call back */
      reply->reply_cb( reply, reply->reply_data );
    }
    else {
      /* Blocking client - use signal */
	  oncrpc_event_set(reply->thread_handle, reply->event);
    }
  }
  oncrpc_crit_sect_leave(oncrpc_reply_queue_crit_sect);
} /* oncrpc_retry_call */

/* --------------------------------------------------------------------- * 
 * task stuff - The following functions are the main task for rpc
 * --------------------------------------------------------------------- */

/*===========================================================================
FUNCTION RPC_HANDLE_RPC_CALL

DESCRIPTION
  This function is called when an RPC call message has been received. It decodes
  the rest of the header and dispatches the RPC call.

  If the shared memory transport is enabled:
  If the program specified in the message is not available on this processor
  and the message was not received on the shared memory transport, the message
  is forwarded on the shared memory transport (via a proxy task). This feature
  is implemented so that STA calls can be received on the modem processor and
  be forwarded to the apps processor.

PARAMETERS
  xdr - pointer to the XDR containing the call message

RETURN VALUE
  None.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
static void
rpc_handle_rpc_call
(
  xdr_s_type *xdr
)
{
  struct svc_req                 call;
  uint32                         rpcvers;
  rpcvers_t                      min_vers;
  rpcvers_t                      max_vers;
  oncrpc_plugger_find_status     find_status;

  /* Decode RPC version */
  if ( ! XDR_RECV_UINT32( xdr, &rpcvers ) ) {
    ERR( "rpc_handle_rpc_call: Failed to decode RPC version number", 0, 0, 0 );
    XDR_MSG_DONE( xdr );
    svcerr_decode( xdr );
    return;    
  }

  if ( rpcvers != RPC_MSG_VERSION ) {
    XDR_MSG_DONE( xdr );
    svcerr_rpcvers( xdr, RPC_MSG_VERSION, RPC_MSG_VERSION );
    return;
  }

  /* Decode RPC program number */
  if ( ! XDR_RECV_UINT32( xdr, &call.rq_prog ) ) {
    ERR( "rpc_handle_rpc_call: Failed to decode RPC program number", 0, 0, 0 );
    XDR_MSG_DONE( xdr );
    svcerr_decode( xdr );
    return;
  }

  /* Decode RPC program version number */
  if ( ! XDR_RECV_UINT32( xdr, &call.rq_vers ) ) {
    ERR( "rpc_handle_rpc_call: Failed to decode RPC program version number", 0,
         0, 0 );
    XDR_MSG_DONE( xdr );
    svcerr_decode( xdr );
    return;
  }

  /* Decode procedure number */
  if ( ! XDR_RECV_UINT32( xdr, &call.rq_proc ) ) {
    ERR( "rpc_handle_rpc_call: Failed to decode RPC procedure number", 0, 0,
         0 );
    XDR_MSG_DONE( xdr );
    svcerr_decode( xdr );
    return;
  }

  /* Decode and process auth */
  if( ! xdr_authenticate( xdr ) ) {
    XDR_MSG_DONE( xdr );
    return;
  }

  call.rq_xprt = xdr;

  ONCRPC_SYSTEM_LOGI("%s: for Xid: %x, Prog: %08x, Vers: %08x, Proc: %08x\n",
	__func__, xdr->xid, call.rq_prog, call.rq_vers, call.rq_proc);

/* Look for prog/protocol match */
  find_status = svc_find_dispatch ( xdr, &call, &max_vers, &min_vers );

  ONCRPC_SYSTEM_LOGI("%s: Find Status: %d Xid: %x\n", __func__, (int)find_status, xdr->xid);

  switch ( find_status ) {
    case ONCRPC_PLUGGER_FIND_FOUND: /* Didn't find right version of program */
      /* Done with call */
      XDR_MSG_DONE( xdr );
        /* Requested version of program unavailable */
        svcerr_progvers ( xdr, min_vers, max_vers );
      break;            

     case ONCRPC_PLUGGER_FIND_NOT_FOUND: /* Didn't find program */
      printf("ERROR ONCRPC Cannot find the callback function received prog:0x%08x, proc:0x%08x vers:0x%08x \n",
               (unsigned int)call.rq_prog,(unsigned int)call.rq_proc,(unsigned int)call.rq_vers);
      printf("Possible cause, <api>cb_app_init not called \n");
      if ( xdr->protocol != ONCRPC_RTR_PROTOCOL )
      {
        /* Use the forward proxy task to forward the RPC call to the other
         * processors in case the RPC program resides there.
         */

        oncrpc_proxy_forward( &call, xdr );
      }
      else
      {
        /* Done with call */
        XDR_MSG_DONE( xdr );
          /* Program unavailable */
          svcerr_noprog ( xdr );
        }
      break;            

    case ONCRPC_PLUGGER_FIND_LOCKED: /* Found program, but server is locked */
      XDR_MSG_DONE( xdr );
      /* return failure */
      svcerr_proglocked ( xdr );
      break;

    case ONCRPC_PLUGGER_FIND_DISPATCHED:
      /* Found right version of program, got dispatched, need do nothing */
    default:
      ;
  }
} /* rpc_handle_rpc_call */

/*===========================================================================
FUNCTION RPC_HANDLE_RPC_REPLY

DESCRIPTION
  This function is called when an RPC reply message has been received. It
  decodes the rest of the header and either calls the call back registered
  for the reply or signals the task waiting for the reply.

  It determines which reply XDR goes with the reply message by matching the
  XID in the reply message with the XID in the reply XDR.

PARAMETERS
  xdr - pointer to the XDR containing the reply message

RETURN VALUE
  None.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
static void
rpc_handle_rpc_reply
(
  xdr_s_type *xdr,
  uint32      xid
)
{
  xdr_s_type *reply;
  dsm_item_type *item;

  /* find reply associated with xid */
  reply = oncrpc_find_reply( xid );
  if( reply == NULL ) {
    /* No client waiting for this response - just delete it */
    XDR_MSG_DONE( xdr );
  }
  else {

    /* Move reply results from server to client */
    XDR_RECV_DSM( xdr, &item );
    XDR_MSG_DONE( xdr );
    XDR_MSG_SET_INPUT( reply, &item );

    if( reply->reply_cb != NULL ) {
      /* Nonblocking client - use call back */
      reply->reply_cb( reply, reply->reply_data );
    }
    else {
      /* Blocking client - use signal */
	  oncrpc_event_set(reply->thread_handle, reply->event);
    }
  }
} /* rpc_handle_rpc_reply */

/*===========================================================================
FUNCTION RPC_FAKE_REPLY

DESCRIPTION
  This function generates a fake reply, removes the xdr from the reply 
  queue, and sets the remply message in the xdr.

  This is needed for cases when the wait gets interrupted and there is 
  no reply.  The upper-layers need to have some valid reply.

PARAMETERS
  xdr - pointer to the XDR containing the reply message

RETURN VALUE
  None.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void rpc_fake_reply(xdr_s_type *client_xdr_ptr)
{
    dsm_item_type *item = dsm_new_buffer(ONCRPC_DSM_ITEM_POOL);
    int32          msg[4];
    q_type     *q_ptr = &oncrpc_reply_queue;

    msg[0] = htonl((int32) RPC_MSG_ACCEPTED);
    msg[1] = htonl((int32) ONCRPC_AUTH_NONE);
    msg[2] = 0;
    msg[3] = htonl((int32) RPC_ACCEPT_SUCCESS);

    dsm_pushdown_tail(&item, msg, sizeof(msg), ONCRPC_DSM_ITEM_POOL);

    XDR_MSG_SET_INPUT( client_xdr_ptr , &item ); 

    q_delete( q_ptr, &client_xdr_ptr->link );
}


/*===========================================================================
FUNCTION ONCRPC_CLEAN_REPLY_QUEUE_BY_CLIENT

DESCRIPTION
  This function is called when an RPC server has died, and there are clients
  waiting for reply from the terminated server.

  The clients that need to be cleaned are identified as follows:
    - They are in waiting for reply state (are in the reply queue)
    - The last message they sent (current destination) is the server which
      is identified in the call to this function, which is terminated.

  For these clients, the reply is pulled from the queue and a pre-defined
  reply is composed and the thread is unblocked, such that the uppler layer 
  will process the reply and handle it for re-transmission.
  
PARAMETERS
  Address of the client or server that has terminated.

RETURN VALUE
  None.

DEPENDENCIES
  None

SIDE EFFECTS
  - Will unblock tasks waiting for replies
  - Will consider message sent undelivered and trigger re-transmission.
===========================================================================*/
void oncrpc_clean_reply_queue_by_client(  oncrpc_addr_type client_addr )
{
  q_type     *q_ptr = &oncrpc_reply_queue;
  xdr_s_type *client_xdr_ptr;
  xdr_s_type *client_xdr_next_ptr;
  oncrpc_control_get_current_dest_type xdr_client_addr;
  
  oncrpc_crit_sect_enter(oncrpc_reply_queue_crit_sect);
  client_xdr_ptr = (xdr_s_type*)q_check( q_ptr );

  while( client_xdr_ptr != NULL )
  {
    client_xdr_next_ptr = (xdr_s_type * )q_next( q_ptr,
                            (q_link_type *)&client_xdr_ptr->link );

    if( !XDR_CONTROL(client_xdr_ptr,ONCRPC_CONTROL_GET_CURRENT_DEST_ADDR,
      (void *)&xdr_client_addr) )
    {
      ERR( "oncrpc_clean_reply_queue_by_client: Failed for client:0x%8x", 
        client_xdr_ptr, 0, 0 );
    }
    else
    {
      if( xdr_client_addr.addr == client_addr )
      {
        rpc_fake_reply(client_xdr_ptr);

        /* Wakeup Client */
        oncrpc_event_set(client_xdr_ptr->thread_handle, client_xdr_ptr->event);
      } /* xdr_client_addr.addr == client_addr */
    } /* XDR_CONTROL succeeded */
    client_xdr_ptr = client_xdr_next_ptr;
  } /* client_xdr_ptr != NULL */
  oncrpc_crit_sect_leave(oncrpc_reply_queue_crit_sect);
} /* END while traversing the queue */



/*===========================================================================
FUNCTION RPC_HANDLE_RPC_MSG

DESCRIPTION
  This function is called when an RPC message has been received. It decodes
  enough of the header to determine if it is a call or reply message and
  then calls the appropriate handler.

PARAMETERS
  xdr - pointer to the XDR containing the message

RETURN VALUE
  None.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void
rpc_handle_rpc_msg
(
  xdr_s_type *xdr
)
{
  uint32 xid;
  int32  msg;

  /* required for rpcgen code */
  xdr->x_op = XDR_DECODE;

  if( !XDR_RECV_UINT32( xdr, &xid ) ||
      !XDR_RECV_INT32( xdr, &msg ) ) {
    ERR( "rpc_handle_cmd_msg: invalid message", 0, 0, 0 );
    XDR_MSG_DONE( xdr );
    return;
  }

  switch( (rpc_msg_e_type) msg ) {
    case RPC_MSG_CALL:
      xdr->xid = xid;
      rpc_handle_rpc_call( xdr );
      break;

    case RPC_MSG_REPLY:
      rpc_handle_rpc_reply( xdr, xid );
      break;

    default:
      ERR( "rpc_handl_cmd_event: invalid message type %x, xid = %x",
           msg, xid, 0 );
      XDR_MSG_DONE( xdr );
      break;
  }
} /* rpc_handle_rpc_msg */

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
void oncrpc_init( void )
{
  uint32 i;

  pthread_mutex_lock(&oncrpc_init_mutex);
  if(oncrpc_inited)
  {
    pthread_mutex_unlock(&oncrpc_init_mutex);
    RPC_MSG_MED( "ONCRPC Already initialized", 0, 0, 0 );
    return;
  }
  oncrpc_inited = TRUE;
  pthread_mutex_unlock(&oncrpc_init_mutex);

#ifdef ONCRPC_SMEM_DEBUG
  smem_log_init();
#endif

#ifndef FEATURE_ONCRPC_EXCLUDE_DIAG_LSM
  Diag_LSM_Init(NULL);
#endif
 
  oncrpc_mem_init_all_pools();
  
  (void) q_init ( &oncrpc_cmd_q );
  (void) q_init ( &oncrpc_cmd_free_q );

  for ( i = 0; i < RPC_CMD_BUF_CNT; i++ ) 
  {
    q_put( &oncrpc_cmd_free_q, q_link( &rpc_cmd_buf[i], &rpc_cmd_buf[i].link ));
  }

  oncrpc_pacmark_mid_init();
  oncrpc_crit_sect_init(&oncrpc_task_crit_sect);
  oncrpc_crit_sect_init(&oncrpc_reply_queue_crit_sect);
  oncrpc_init_reply_queue();
  rpc_init_callback_registry();

  /* Init OS resources */
  oncrpc_os_init();
  oncrpc_main_os_init();

  oncrpc_lookup_init();

  /* Call transport init's if they need them */
  


#ifdef FEATURE_ONCRPC_LO
  xprtlo_init();
#else
  xprtrtr_init();
#endif /* FEATURE_ONCRPC_LO */

#ifdef FEATURE_ONCRPC_DIAG
  svcdiag_init();
#endif /* FEATURE_ONCRPC_DIAG */

  oncrpc_proxy_init();
  oncrpc_app_init();


} /* oncrpc_init */

/*===========================================================================
FUNCTION ONCRPC_DEINIT

DESCRIPTION
  This function deinitializes ONCRPC in the case of a process unloading the
  library without terminating.  It will call oncrpc_task_stop to stop all
  threas of execution and de-allocate memory.

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
void oncrpc_deinit( void )
{
  pthread_mutex_lock(&oncrpc_init_mutex);
  if(oncrpc_inited)
  {
    oncrpc_inited = FALSE;
    /* TODO: Need to free memory allocated by all the init functions */
    oncrpc_main_os_deinit();
#ifndef FEATURE_ONCRPC_EXCLUDE_DIAG_LSM
    Diag_LSM_DeInit();
#endif

#ifdef ONCRPC_SMEM_DEBUG
    smem_log_exit();
#endif

#ifndef FEATURE_ONCRPC_LO
  xprtrtr_deinit();
#endif /* !FEATURE_ONCRPC_LO */
  }
  pthread_mutex_unlock(&oncrpc_init_mutex);
}

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
void
rpc_cmd
(
  rpc_cmd_type * cmd_ptr             /* pointer to the command data */
)
{
  rpci_cmd_type        *link_ptr;     /* for queueing commands */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*----------------------------------------------------------------------
    Get a link from the free queue, call error routine if no links 
    available, else copy buffer to internal link.
  -----------------------------------------------------------------------*/
  if(( link_ptr = (rpci_cmd_type *)q_get( &oncrpc_cmd_free_q)) == NULL)
  {
    ERR_FATAL( "RPC Task Command link not available", 0, 0, 0);
  }
  else /* Assign to the link, put link onto queue and signal task.  */
  {
    link_ptr->cmd = *cmd_ptr;
    q_put( &oncrpc_cmd_q, &link_ptr->link);
    oncrpc_signal_rpc_thread(RPC_CMD_Q_SIG);
  }
} /* rpc_cmd */


/*===========================================================================

FUNCTION ONCRPC_MAIN

DESCRIPTION
  This function handles cmd events and dispatches them to the proper
  handler.

DEPENDENCIES
  This function should be called from oncrpc task context only.

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/
void oncrpc_main(void)
{
  rpci_cmd_type *cmd;
  xdr_s_type    *xdr;

  /* First grab the current command off the queue */
  while ( NULL != ( cmd = q_get( &oncrpc_cmd_q ) ) ) {
    switch( cmd->cmd.cmd ) {
      case RPC_READ:
        xdr = (xdr_s_type *) cmd->cmd.xdr;

        /* Clear flag before the loop to avoid critical race condition with
         * the code that checks the flag and queues the command only if it's
         * set.
         */
        xdr->cmd_pending = FALSE;

        while( XDR_READ( xdr ) ) {
          rpc_handle_rpc_msg( xdr );
        }
        break;

      case RPC_XDR:
        if( cmd->cmd.handler == NULL ) {
          ERR_FATAL( "rpc_handle_cmd_event: RPC_XDR cmd with null function",
                     0, 0, 0 );
        }
        cmd->cmd.handler( &cmd->cmd );
        break;

      case RPC_DESTROY:
        xdr = (xdr_s_type *) cmd->cmd.xdr;
        if ( NULL == xdr ) {
          ERR_FATAL( "rpc_handle_cmd_event: RPC_DESTROY with NULL XDR",
                     0, 0, 0 );
        }
        xdr->xops->xdr_destroy( xdr );
        break;

      default:
        ERR( "rpc_handle_cmd_event: unknown cmd %d", cmd->cmd.cmd, 0, 0 );
        break;
    }

    q_put( &oncrpc_cmd_free_q, &cmd->link);
  }
} /* oncrpc_main */


/*===========================================================================
FUNCTION ONCRPC_SET_LAST_SENT_MSG_INFO

DESCRIPTION
  This function saves information regarding the last RPC message (call or
  reply). 

PARAMETERS
  prog - The RPC program number.
  vers - The RPC program's version number.
  proc - The RPC procedure number.

RETURN VALUE
  None.

DEPENDENCIES
  None.

SIDE EFFECTS
  None
===========================================================================*/
void oncrpc_set_last_sent_msg_info
(
  uint32 prog,
  uint32 vers,
  uint32 proc
)
{
  oncrpc_crit_sect_enter(oncrpc_task_crit_sect);
  last_proc = proc;
  last_vers = vers;
  last_prog = prog;
  oncrpc_crit_sect_leave(oncrpc_task_crit_sect);
} /* oncrpc_set_last_sent_msg_info */

  
/*===========================================================================
FUNCTION ONCRPC_GET_LAST_SENT_MSG_INFO

DESCRIPTION
  This function returns information regarding the last RPC message (call or
  reply). 

PARAMETERS
  prog - The RPC program number.
  vers - The RPC program's version number.
  proc - The RPC procedure number.

RETURN VALUE
  TRUE if information exists.
  FALST if no RPC messages have been sent yet.

DEPENDENCIES
  None.

SIDE EFFECTS
  None
===========================================================================*/
boolean oncrpc_get_last_sent_msg_info
(
  uint32 *prog,
  uint32 *vers,
  uint32 *proc
)
{
  boolean ret = FALSE;

  if ( 0 != last_prog )
  {
    ret = TRUE;
    if ( NULL != prog )
    {
      *prog = last_prog;
    }
    if ( NULL != vers )
    {
      *vers = last_vers;
    }
    if ( NULL != proc )
    {
      *proc = last_proc;
  }
  }

  return ret;
} /* oncrpc_get_last_sent_msg_info */


/*===========================================================================
FUNCTION ONCRPC_OUTSTANDING_CALL

DESCRIPTION
  This function returns the number of outstanding transactions.

PARAMETERS
  None.

RETURN VALUE
  The number of outstanding transactions

DEPENDENCIES
  oncrpc_init_reply_queue must have been called

SIDE EFFECTS
  None
===========================================================================*/
uint32 oncrpc_outstanding_call 
(
  void
)
{
  return (uint32)q_cnt( &oncrpc_reply_queue );
} /* oncrpc_outstanding_call */

/*===========================================================================
FUNCTION ONCRPC_TLS_GET_COMMON

DESCRIPTION
  Searchs a TLS table for an entry with the given key. If an entry is not
  found, then a entry is added to the table.

PARAMETERS
  tbl - the TLS table structure
  key - the key to use to search the table

RETURN VALUE
  pointer to table entry

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
oncrpc_tls_type *oncrpc_tls_get_common
(
  oncrpc_tls_table_type *tbl,
  void                  *key
)
{
  oncrpc_tls_type *ptr = tbl->table;
  uint32 i;

  /*--------------------------------------------------
   * Implementation assumes the search usually succeeds. So, the search
   * doesn't try to keep track of the first available slot to use if the
   * search fails.
   *--------------------------------------------------*/

  /* See if the key is already in the resource table */
  oncrpc_crit_sect_enter( tbl->crit_sec );
  for( i = 0; i < tbl->num_entries; i++ )
  {
    if( ptr->valid && ptr->key == key )
    {
      /* found entry */
      oncrpc_crit_sect_leave( tbl->crit_sec );
      return ptr;
    }

    ptr = (oncrpc_tls_type *)((byte *)ptr + tbl->entry_size);
  }
 
  /* Entry wasn't found, add it to the table and initialise the resources */
  ptr = tbl->table;
  for( i = 0; i < tbl->num_entries; i++ )
  {
    if( !ptr->valid )
    {
      memset( ptr, 0, tbl->entry_size );
      ptr->valid  = TRUE;
      ptr->key    = key;
      ptr->thread = (oncrpc_thread_handle) ptr;

      oncrpc_crit_sect_leave( tbl->crit_sec );

      /* Initialize the OS specific fields of the tls */
      oncrpc_tls_init( ptr );

      return ptr;
    }

    ptr = (oncrpc_tls_type *)((byte *)ptr + tbl->entry_size);
  }

  oncrpc_crit_sect_leave( tbl->crit_sec );
  if(tbl->num_entries == 0)
  {
    ERR_FATAL("Attempt to use task local storage while not initialized",0,0,0);
  }
  else
  {
    ERR_FATAL("Task local storage out of resources",0,0,0);
  }  
  return NULL;
} /* oncrpc_tls_get_common */

/*===========================================================================
FUNCTION ONCRPC_TLS_DELETE_COMMON

DESCRIPTION
  Deletes the entry assoicated with the given key from a TLS table. If no
  entry is found, no action is taken.

PARAMETERS
  tbl - the TLS table structure
  key - the key to use to search the table

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void oncrpc_tls_delete_common( oncrpc_tls_table_type *tbl, void *key )
{
  oncrpc_tls_type *ptr = tbl->table;
  uint32 i;

  /* See if the key is already in the resource table */
  oncrpc_crit_sect_enter( tbl->crit_sec );
  for( i = 0; i < tbl->num_entries; i++ )
  {
    if( ptr->valid && ptr->key == key )
    {
      /* found entry */
      ptr->valid = FALSE;
      break;
    }

    ptr = (oncrpc_tls_type *)((byte *)ptr + tbl->entry_size);
  }

  oncrpc_crit_sect_leave( tbl->crit_sec );
} /* oncrpc_tls_delete_common */

/*===========================================================================
FUNCTION ONCRPC_TLS_FIND_COMMON

DESCRIPTION
  Find the entry assoicated with the given key from a TLS table. If no
  entry is found, NULL is returned

PARAMETERS
  tbl - the TLS table structure
  key - the key to use to search the table

RETURN VALUE
  pointer to TLS entry if found, NULL otherwise

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
oncrpc_tls_type * oncrpc_tls_find_common( oncrpc_tls_table_type *tbl, void *key )
{
  oncrpc_tls_type *ptr = tbl->table;
  uint32 i;

  /* See if the key is already in the resource table */
  for( i = 0; i < tbl->num_entries; i++ )
  {
    if( ptr->valid && ptr->key == key )
    {
      /* found entry */
      return ptr;
    }

    ptr = (oncrpc_tls_type *)((byte *)ptr + tbl->entry_size);
  }

  return NULL;
} /* oncrpc_tls_delete_common */


/*===========================================================================
FUNCTION ONCRPC_SET_TASK_NAME

DESCRIPTION
  Fills the TLS name buffer with as much of the passed in string as will fit.

PARAMETERS
  name - pointer to buffer

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void oncrpc_set_task_name( char *name )
{
  oncrpc_tls_type *tls;
  uint32 src_len;

  tls = oncrpc_tls_get_self();
  src_len = strlen( name ) + 1;

  memcpy( tls->name, name, MIN(src_len, ONCRPC_THREAD_NAME_SIZE) );

  tls->name[ONCRPC_THREAD_NAME_SIZE-1] = '\0';
} /* oncrpc_get_task_name */

/*===========================================================================
FUNCTION ONCRPC_GET_TASK_NAME

DESCRIPTION
  Fills the passed in buffer with as much as the task name as will fit.

PARAMETERS
  name - pointer to buffer
  len  - length of buffer (in bytes)

RETURN VALUE
  task name is returned in name argument

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void oncrpc_get_task_name( char *name, uint32 len )
{
  oncrpc_tls_type *tls;
  uint32 src_len;

  tls = oncrpc_tls_get_self();
  src_len = strlen( tls->name ) + 1;

  memcpy( name, tls->name, MIN(src_len, len) );

  name[len-1] = '\0';
} /* oncrpc_get_task_name */

/*===========================================================================
FUNCTION      ONCRPC_STRNCPY

DESCRIPTION   Copies the source string into the destination buffer until 
              size is reached, or until a '\0' is encountered.  If valid,
              the destination string will always be NULL deliminated.
              
PARAMETERS    dst - the destination string

              src - the source string

              len - the maximum copy len

RETURN VALUE  dst
===========================================================================*/
char *oncrpc_strncpy(char *dst, char *src, uint32 len)
{
  int src_len;

  if( !len || !dst || !src )
  {
    return NULL;
  }

  src_len = strlen(src) + 1;

  memcpy( dst, src, MIN(src_len, (int)len) );

  dst[len-1] = '\0';

  return dst;
}

/*===========================================================================
 * End of module
 *=========================================================================*/
