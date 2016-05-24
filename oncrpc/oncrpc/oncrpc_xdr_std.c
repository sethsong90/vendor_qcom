/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
                          O N C R P C _ X D R _ S T D . C

GENERAL DESCRIPTION
  This file contains an oncrpc XDR which uses the standard network byte
  ordering. (STD = Standard).

 Copyright (c) 2005-2008,2011-2012 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential. 

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_xdr_std.c#6 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
28/07/11    as     Enable userspace logging
04/20/11    eh/kr  Added reset of xdr on failure
07/21/08    ih     Removed error fatal when XPORT write fails
07/09/08   rr         Fix read handle for multiple callbacks CBSP20
04/21/08    sd     Added oncrpc_get_last_sent_msg_info() and 
                   oncrpc_set_last_sent_msg_info()
03/26/08    ih     Fixed SMEM logging for async RPC
03/24/08    ih     Added workaround to TLS table overflow on thread migration
02/04/08    hn     Dup item in send_dsm to support retries in upper layers.
01/16/07    rr     Handle interrupted wait in send_msg
10/12/07    ptm    Merge OS specific xdr-std files back to this file.
07/10/07    ptm    Remove featurization and reply timer code.
05/04/07    rr     Rename private to xprivate, keyword conflict on c++ compilers.
02/27/07    ptm/hn No longer count commands to prevent flooding the rpc task's
                   command queue. Instead use a pending flag.
01/19/07    hn     No task lock check in WinCE when making an RPC call.
12/04/06    ptm    Minor optimization to xdr_std_read.
10/20/06    hn     Added CONCAT macro to fix WinCE compiler issues.
10/09/06    hn     Put back auto call retry hook in state machine because
                   it's needed after a connection is lost and reinited.
10/03/06    hn     Removed xport_running op from transports.
                   Removed automatic call retry hook from state machine.
                   Added support for locking/unlocking RPC services.
07/31/06    hn     Support handling error checks for clients automatically.
07/10/06    hn     Featurized variables used only for debug with
                   ONCRPC_SMEM_DEBUG
06/27/06    ptm    Add code to log async call event.
05/29/06    ptm    Add code to handle XDR_FLAG_RETRY.
05/11/06    ptm    Don't free memory during SMSM RESET.
04/25/06    ddh    Replaced use of SMEM_LOG_EVENT with ONCRPC_LOG_EVENT
02/22/06    ptm    Change smem event logging to include task name.
02/01/06    ptm    Fix WinCE compiler warnings.
01/26/06    ptm    Make xdr_std_xops const.
01/25/06    ptm    Changed xdr_std_msg_set_reply to xdr_std_msg_set_input.
07/14/05    clp    Fixed uninitialized vers in clone.
07/13/05    clp    Added check for NULL prv in destroy.
05/23/05    hn     Added code to initialize mem field to NULL in xdr.
05/12/05    ptm    Merged XDR field names.
05/09/05    clp    Changed clone to use use_count.
05/09/05    ptm    Add cmd_cnt to keep race condition from filling cmd queue.
04/26/05    ptm    Add retry and reply timer, fix compiler warning, and
                   update msg state machine.
04/22/05    ptm    Change msg_send to check for NO_PROG response.
04/14/05    ptm    Include task.h for oncrpc_tcb.
04/13/05    ptm    Add protocol type to xdr_std_create and xdr_std_xdr_init.
04/12/05    ptm    Include oncrpci.h and remove err.h and smem_log.h.
04/01/05    clp    Include header cleanup changes.
03/22/05    clp    Changes for nonblocking clnt call, including clone.
03/11/05    ptm    Fix compiler warning in xdr_std_control routine.
03/10/05    ptm    Add error check for interrupt level and tasks locked to
                   msg_send routine - can't r e x_wait in these cases.
03/10/05    ptm    First revision w/edit history.
===========================================================================*/


#include "dsm.h"
#include "oncrpc.h"
#include "oncrpci.h"
#include "oncrpc_os.h"
#include "oncrpc_taski.h"
#include "oncrpc_xdr_std.h"
#include "oncrpc_main.h"

#ifdef FEATURE_SMSM
#include "smsm.h"
#endif /* FEATURE_SMSM */

#define XDR_STD_SEND_ERR(xdr, xdr_proc) \
  ERR_FATAL(CONCAT(xdr_proc, ": error in PROG %x, PROC %x, XID %d"),\
            xdr->x_prog, xdr->x_proc, xdr->xid)

#define XDR_STD_RECV_ERR(xdr, xdr_proc) \
  do {\
    xdr_std_private_s_type *prv = (xdr_std_private_s_type *)xdr->xprivate;\
    if ( prv->in_msg_type == RPC_MSG_REPLY )\
    {\
      ERR_FATAL(CONCAT(xdr_proc, ": error in PROG %x, PROC %x, XID %d"),\
                xdr->x_prog, xdr->x_proc, xdr->xid);\
    }\
  } while (0)

/*======================================================================
  in_msg_type state machine:
  When in_msg is valid, in_msg_type indicates the type of message it holds,
  a reply to be decoded by the client, or a call to be decoded by the RPC
  task. 
  When in_msg is not valid, in_msg_type is RPC_MSG_UNDEF.
  ======================================================================*/

typedef struct {
  dsm_item_type      *in_msg;
  rpc_msg_e_type      in_msg_type;
  rpc_msg_e_type      out_msg_type;
  oncrpc_timer_ptr    timer;         /* timer for retry and reply timeout */
} xdr_std_private_s_type;

/*======================================================================
  Define normal entry points
  ======================================================================*/
/* Transport control functions */
static void    xdr_std_destroy  ( xdr_s_type *xdr );
static boolean xdr_std_control  ( xdr_s_type *xdr,
                                  int request,
                                  void *info );
static xdr_s_type *xdr_std_clone( xdr_s_type *xdr );
static void    xdr_std_errchk   ( xdr_s_type *xdr, boolean on );

/* Incoming message control functions */
static boolean xdr_std_read             ( xdr_s_type *xdr );
static boolean xdr_std_msg_set_input    ( xdr_s_type *xdr,
                                          dsm_item_type **item );
static boolean xdr_std_msg_done         ( xdr_s_type *xdr );

/* Outgoing message control functions */
static boolean xdr_std_msg_start           ( xdr_s_type *xdr,
                                             rpc_msg_e_type rpc_msg_type );
static boolean xdr_std_msg_start_w_errchk  ( xdr_s_type *xdr,
                                             rpc_msg_e_type rpc_msg_type );
static boolean xdr_std_msg_abort           ( xdr_s_type *xdr );
static boolean xdr_std_msg_send            ( xdr_s_type *xdr,
                                             rpc_reply_header *reply );
static boolean xdr_std_msg_send_nonblocking( xdr_s_type *xdr,
                                            rpc_reply_cb_type cb,
                                            void *cb_data );

/* Message encode function */
static boolean xdr_std_send_int8  ( xdr_s_type *xdr, const int8 *value );
static boolean xdr_std_send_uint8 ( xdr_s_type *xdr, const uint8 *value );
static boolean xdr_std_send_int16 ( xdr_s_type *xdr, const int16 *value );
static boolean xdr_std_send_uint16( xdr_s_type *xdr, const uint16 *value );
static boolean xdr_std_send_int32 ( xdr_s_type *xdr, const int32 *value );
static boolean xdr_std_send_uint32( xdr_s_type *xdr, const uint32 *value );
static boolean xdr_std_send_bytes ( xdr_s_type *xdr, const void *values,
                                    uint32 len );
static boolean xdr_std_send_dsm  ( xdr_s_type *xdr, dsm_item_type **item );

/* Message encode function (with error checking) */
static boolean xdr_std_send_int8_w_errchk  ( xdr_s_type *xdr,
                                             const int8 *value );
static boolean xdr_std_send_uint8_w_errchk ( xdr_s_type *xdr,
                                             const uint8 *value );
static boolean xdr_std_send_int16_w_errchk ( xdr_s_type *xdr,
                                             const int16 *value );
static boolean xdr_std_send_uint16_w_errchk( xdr_s_type *xdr,
                                             const uint16 *value );
static boolean xdr_std_send_int32_w_errchk ( xdr_s_type *xdr,
                                             const int32 *value );
static boolean xdr_std_send_uint32_w_errchk( xdr_s_type *xdr,
                                             const uint32 *value );
static boolean xdr_std_send_bytes_w_errchk ( xdr_s_type *xdr,
                                             const void *values,
                                             uint32 len );
static boolean xdr_std_send_dsm_w_errchk   ( xdr_s_type *xdr,
                                             dsm_item_type **item );

/* Message decode function */
static boolean xdr_std_recv_int8  ( xdr_s_type *xdr, int8 *value );
static boolean xdr_std_recv_uint8 ( xdr_s_type *xdr, uint8 *value );
static boolean xdr_std_recv_int16 ( xdr_s_type *xdr, int16 *value );
static boolean xdr_std_recv_uint16( xdr_s_type *xdr, uint16 *value );
static boolean xdr_std_recv_int32 ( xdr_s_type *xdr, int32 *value );
static boolean xdr_std_recv_uint32( xdr_s_type *xdr, uint32 *value );
static boolean xdr_std_recv_bytes ( xdr_s_type *xdr, void *values, uint32 len);
static boolean xdr_std_recv_dsm   ( xdr_s_type *xdr, dsm_item_type **item );

/* Message decode function (with error checking) */
static boolean xdr_std_recv_int8_w_errchk  ( xdr_s_type *xdr,
                                             int8 *value );
static boolean xdr_std_recv_uint8_w_errchk ( xdr_s_type *xdr,
                                             uint8 *value );
static boolean xdr_std_recv_int16_w_errchk ( xdr_s_type *xdr,
                                             int16 *value );
static boolean xdr_std_recv_uint16_w_errchk( xdr_s_type *xdr,
                                             uint16 *value );
static boolean xdr_std_recv_int32_w_errchk ( xdr_s_type *xdr,
                                             int32 *value );
static boolean xdr_std_recv_uint32_w_errchk( xdr_s_type *xdr,
                                             uint32 *value );
static boolean xdr_std_recv_bytes_w_errchk ( xdr_s_type *xdr,
                                             void *values,
                                             uint32 len);
static boolean xdr_std_recv_dsm_w_errchk   ( xdr_s_type *xdr,
                                             dsm_item_type **item );

/*======================================================================
  Define normal jump table
  ======================================================================*/

static const xdr_ops_s_type xdr_std_xops = {
  xdr_std_destroy,
  xdr_std_control,
  xdr_std_clone,
  xdr_std_errchk,
  xdr_std_read,
  xdr_std_msg_set_input,
  xdr_std_msg_done,
  xdr_std_msg_start,
  xdr_std_msg_abort,
  xdr_std_msg_send,
  xdr_std_msg_send_nonblocking,
  xdr_std_send_int8,
  xdr_std_send_uint8,
  xdr_std_send_int16,
  xdr_std_send_uint16,
  xdr_std_send_int32,
  xdr_std_send_uint32,
  xdr_std_send_bytes,
  xdr_std_send_dsm,
  xdr_std_recv_int8,
  xdr_std_recv_uint8,
  xdr_std_recv_int16,
  xdr_std_recv_uint16,
  xdr_std_recv_int32,
  xdr_std_recv_uint32,
  xdr_std_recv_bytes,
  xdr_std_recv_dsm
};

static const xdr_ops_s_type xdr_std_xops_w_errchk = {
  xdr_std_destroy,
  xdr_std_control,
  xdr_std_clone,
  xdr_std_errchk,
  xdr_std_read,
  xdr_std_msg_set_input,
  xdr_std_msg_done,
  xdr_std_msg_start_w_errchk,
  xdr_std_msg_abort,
  xdr_std_msg_send,
  xdr_std_msg_send_nonblocking,
  xdr_std_send_int8_w_errchk,
  xdr_std_send_uint8_w_errchk,
  xdr_std_send_int16_w_errchk,
  xdr_std_send_uint16_w_errchk,
  xdr_std_send_int32_w_errchk,
  xdr_std_send_uint32_w_errchk,
  xdr_std_send_bytes_w_errchk,
  xdr_std_send_dsm_w_errchk,
  xdr_std_recv_int8_w_errchk,
  xdr_std_recv_uint8_w_errchk,
  xdr_std_recv_int16_w_errchk,
  xdr_std_recv_uint16_w_errchk,
  xdr_std_recv_int32_w_errchk,
  xdr_std_recv_uint32_w_errchk,
  xdr_std_recv_bytes_w_errchk,
  xdr_std_recv_dsm_w_errchk
};

/*===========================================================================
        Normal (and common) Routines
===========================================================================*/

/*===========================================================================
        External Entry Points
===========================================================================*/
xdr_s_type *
xdr_std_create
(
  xport_s_type   *xport,
  oncrpc_event_t  event,
  rpcprot_t       protocol
)
{
  xdr_s_type *xdr;

  xdr = oncrpc_mem_alloc( sizeof(xdr_s_type) );

  if( xdr != NULL ) {
    if( xdr_std_xdr_init( xdr, xport, event, protocol ) ) {
      xdr->flags |= XDR_FLAG_XDR_ALLOCED;
    }
    else {
      oncrpc_mem_free( xdr );
      xdr = NULL;
    }
  }
  else {
    ERR( "xdr_std_create: unable to malloc xdr", 0, 0, 0 );
  }

  return xdr;
} /* xdr_std_create */
      
boolean xdr_std_xdr_init
(
  xdr_s_type         *xdr,
  xport_s_type       *xport,
  oncrpc_event_t      event,
  rpcprot_t protocol
)
{
  xdr_std_private_s_type *prv;

  if( xdr == NULL ) {
    ERR( "xdr_std_xdr_init: xdr is NULL", 0, 0, 0 );
    return FALSE;
  }

  prv = oncrpc_mem_alloc( sizeof(xdr_std_private_s_type) );
  if( prv == NULL ) {
    return FALSE;
  }

  memset( xdr, 0, sizeof(xdr_s_type) );

  q_link( xdr, &xdr->link );

  xdr->thread_handle   = oncrpc_thread_handle_get();
  xdr->event           = event;
  xdr->protocol        = protocol;
  xdr->cmd_pending     = FALSE;
  xdr->xops            = &xdr_std_xops;
  xdr->mem             = NULL;
  xdr->xprivate        = prv;
  xdr->xport           = xport;
  prv->in_msg          = NULL;
  prv->in_msg_type     = RPC_MSG_UNDEF;
  prv->out_msg_type    = RPC_MSG_UNDEF;

  oncrpc_get_task_name( (char *) xdr->name, sizeof(xdr->name) );

  if (!oncrpc_timer_new(&prv->timer, xdr->event)) {
    ERR_FATAL( "xdr_std_xdr_init: unable to malloc xdr private timer", 0, 0, 0 );
    return FALSE;
  }

  return TRUE;
} /* xdr_std_xdr_init */

/*===========================================================================
        Transport Control Functions
===========================================================================*/

static void xdr_std_destroy( xdr_s_type *xdr )
{
  xdr_std_private_s_type *prv = (xdr_std_private_s_type *) xdr->xprivate;

  // clean up private structure
  if ( 0 == xdr->xport->use_count-- )
  {
    XPORT_DESTROY( xdr->xport );
  }

#ifdef FEATURE_SMSM
  if( (smsm_get_local_state( SMSM_THIS_HOST ) & SMSM_RESET) == 0 )
#endif /* FEATURE_SMSM */
  {
    if ( NULL != prv )
    {
      dsm_free_packet( &prv->in_msg );

      if ( NULL != prv->timer )
      {
        oncrpc_timer_free(prv->timer);
      }

      oncrpc_mem_free( prv );
    }
    xdr->xprivate = NULL;
    oncrpc_queue_reply_remove( xdr );

    // if xdr was alloc'ed, free it
    if( (xdr->flags & XDR_FLAG_XDR_ALLOCED) == XDR_FLAG_XDR_ALLOCED ) {
      oncrpc_mem_free( xdr );
    }
  }
} /* xdr_std_destroy */

static boolean xdr_std_control
(
  xdr_s_type *xdr,
  int request,
  void *info
)
{
  ASSERT(xdr->xport != NULL);
  return XPORT_CONTROL( xdr->xport, request, info );
} /* xdr_std_control */

static xdr_s_type *
xdr_std_clone( xdr_s_type *parent )
{
  xdr_s_type             *xdr;
  xdr_std_private_s_type *prv;

  ASSERT( NULL != parent );
  ASSERT( NULL != parent->xprivate );

  xdr = oncrpc_mem_alloc( sizeof(xdr_s_type) );

  if( xdr != NULL ) {
    prv = oncrpc_mem_alloc( sizeof(xdr_std_private_s_type) );

    if( prv != NULL ) {
      prv->in_msg       = NULL;
      prv->in_msg_type  = RPC_MSG_UNDEF;
      prv->out_msg_type = RPC_MSG_UNDEF;

      memset( xdr, 0, sizeof(xdr_s_type) );
      q_link( xdr, &xdr->link );

      xdr->xport              = parent->xport;
      xdr->xport->use_count++;
      xdr->flags              = XDR_FLAG_XDR_ALLOCED;
      xdr->thread_handle      = parent->thread_handle; 
      xdr->event              = parent->event; 
      xdr->xops               = &xdr_std_xops;

      xdr->xid                = parent->xid;
      xdr->protocol           = parent->protocol;
      xdr->cmd_pending        = FALSE;
      xdr->xprivate           = prv;
      xdr->x_prog             = parent->x_prog;
      xdr->x_vers             = parent->x_vers;

      if (!oncrpc_timer_new(&prv->timer, xdr->event)) {
        ERR_FATAL( "xdr_std_clone: unable to malloc xdr private timer", 0, 0, 0 );

        oncrpc_mem_free(prv);
        oncrpc_mem_free(xdr);
        xdr = NULL;
        }
    }
    else {
      ERR( "xdr_std_clone: unable to malloc xdr private", 0, 0, 0 );
      oncrpc_mem_free( xdr );
      xdr = NULL;
    }
  }
  else {
    ERR( "xdr_std_clone: unable to malloc xdr", 0, 0, 0 );
  }

  return xdr;
} /* xdr_std_clone */

static void xdr_std_errchk( xdr_s_type *xdr, boolean on )
{
#ifdef FEATURE_WINCE
  if(((HANDLE)GetCurrentProcessId() != GetCallerProcess()))
  {
    /* Thread migrated, so the XDR may have been destroyed */
    return; 
  }
#endif
  if ( on ) {
    xdr->xops = &xdr_std_xops_w_errchk;
  } else {
    xdr->xops = &xdr_std_xops;
  }
} /* xdr_std_errchk */

/*===========================================================================
        Incoming Message Control Functions
===========================================================================*/

static boolean xdr_std_read( xdr_s_type *xdr )
{
  xdr_std_private_s_type *prv = (xdr_std_private_s_type *) xdr->xprivate;
  boolean result;
  uint32 size;
  oncrpc_control_get_source_type src_address_lookup;

  if( prv->in_msg_type != RPC_MSG_UNDEF ) {
    ERR_FATAL( "Invalid in_msg_type = %d", prv->in_msg_type, 0, 0 );
  }

  result = XPORT_READ_DSM( xdr->xport, &prv->in_msg );

  if( result ) {
    size = dsm_length_packet( prv->in_msg );
    prv->in_msg_type = RPC_MSG_CALL;

    /* ONCRPC_PROFILE_MSG_SIZE( size ); */

    if( size < 6 * 4 ) {
      RPC_MSG_HIGH( "RPC message too short, size = %d", size, 0, 0 );
      dsm_free_packet( &prv->in_msg );
      result = FALSE;
      prv->in_msg_type = RPC_MSG_UNDEF;
    }
    if( XDR_CONTROL(xdr,ONCRPC_CONTROL_GET_SOURCE_ADDR,(void *)&src_address_lookup) )
    {
      xdr->msg_source = src_address_lookup.addr;
    }
  }

  return result;
} /* xdr_std_read */

static boolean xdr_std_msg_set_input( xdr_s_type *xdr, dsm_item_type **item )
{
  xdr_std_private_s_type *prv = (xdr_std_private_s_type *) xdr->xprivate;

  if( prv->in_msg_type != RPC_MSG_UNDEF ) {
    ERR_FATAL( "Invalid in_msg_type = %d", prv->in_msg_type, 0, 0 );
  }

  // Common case is there should not be any left over data
  if( prv->in_msg != NULL ) {
    ERR( "left over data ignored", 0, 0, 0 );
    dsm_free_packet( &prv->in_msg );
  }

  // set incoming message to reply
  prv->in_msg = *item;
  *item = NULL;

  prv->in_msg_type = RPC_MSG_REPLY;

  return TRUE;
} /* xdr_std_msg_set_input */

static boolean xdr_std_msg_done( xdr_s_type *xdr )
{
  xdr_std_private_s_type *prv = (xdr_std_private_s_type *) xdr->xprivate;
  boolean result = TRUE;

  //if( prv->in_msg_type == RPC_MSG_UNDEF ) {
//    ERR_FATAL( "Invalid in_msg_type = %d", prv->in_msg_type, 0, 0 );
  //}

  
  /* Common case is we've used up all of the message */
  if( prv->in_msg != NULL ) {
    dsm_free_packet( &prv->in_msg );  
    result = FALSE;
  }

  prv->in_msg_type = RPC_MSG_UNDEF;

#ifdef FEATURE_WINCE
  /* This is Windows only feature to free up resources on thread migration */
  {
    oncrpc_tls_type *tls;
    /* Now check to see if we need to free the XDR and TLS */
    if(xdr->thread_handle)
    {
      tls = (oncrpc_tls_type *)xdr->thread_handle;
    }
    else
    {
      /* No TLS associated with XDR, look it up (slow op, print debug msg) */
      RPC_MSG_HIGH("No TLS associated with XDR, look it up",0,0,0);
      tls = oncrpc_tls_get_self();
      xdr->thread_handle = (void *)tls;
    }

    if(!tls->xdr && !xdr->mem 
        && ((HANDLE)GetCurrentProcessId() != GetOwnerProcess()) )
    {
      /* If not proxy thread, no memory was allocated, free TLS & XDR */
      XDR_DESTROY(xdr);
      oncrpc_tls_delete_self();
      RPC_MSG_HIGH("Thread migration into ONCRPC detected, destory TLS & XDR",
                0,0,0);
    }
  }
#endif

  return result;
} /* xdr_std_msg_done */

/*===========================================================================
        Outgoing Message Control Functions
===========================================================================*/

static boolean xdr_std_msg_start( xdr_s_type *xdr, rpc_msg_e_type rpc_msg_type )
{
  xdr_std_private_s_type *prv = (xdr_std_private_s_type *) xdr->xprivate;
  uint32 xid;
  int32 msg;

  if( prv->out_msg_type != RPC_MSG_UNDEF ) {
    ERR( "xdr_std_msg_start: attempt to start message while message pending",
         0, 0, 0 );
    return FALSE;
  }

  if( rpc_msg_type == RPC_MSG_CALL ) {
    xdr->xid = oncrpc_next_xid();
  }
  // For replies, the rpc task as already set the xid

  prv->out_msg_type = rpc_msg_type;

  xid = htonl( xdr->xid );
  msg = htonl( (int32) rpc_msg_type );

  return ( XPORT_WRITE( xdr->xport, &xid, sizeof(uint32) ) == sizeof(uint32) &&
           XPORT_WRITE( xdr->xport, &msg, sizeof(int32) ) == sizeof(int32) );
} /* xdr_std_msg_start */

static boolean
xdr_std_msg_start_w_errchk
(
  xdr_s_type *xdr,
  rpc_msg_e_type rpc_msg_type
)
{
  if ( ! xdr_std_msg_start(xdr, rpc_msg_type) )
  {
    XDR_STD_SEND_ERR(xdr, "xdr_std_msg_start");
    return FALSE;
  }

  return TRUE;
} /* xdr_std_msg_start */

static boolean xdr_std_msg_abort( xdr_s_type *xdr )
{
  xdr_std_private_s_type *prv = (xdr_std_private_s_type *) xdr->xprivate;

  prv->out_msg_type = RPC_MSG_UNDEF;

  return XPORT_FLUSH_OUTPUT( xdr->xport );
} /* xdr_std_msg_abort */



/*===========================================================================
FUNCTION XDR_STD_MSG_SEND_CALL

DESCRIPTION

DEPENDENCIES
  None.

ARGUMENTS

RETURN VALUE
  TRUE  - success
  FALSE - failure

SIDE EFFECTS
  None.
===========================================================================*/
static boolean xdr_std_msg_send_call( xdr_s_type *xdr, rpc_reply_header *reply )
{
  xdr_std_private_s_type *prv = (xdr_std_private_s_type *) xdr->xprivate;
  oncrpc_thread_handle self;
  uint32 sigs;

  if( reply == NULL ) {
    ERR_FATAL( "Reply pointer is NULL", 0, 0, 0 );
  }

  oncrpc_rpc_is_allowed( );

  self = xdr->thread_handle;

  if(!self)
    self = oncrpc_thread_handle_get();

  oncrpc_event_clr( self, xdr->event);

  oncrpc_queue_reply( xdr );

#ifdef ONCRPC_SMEM_DEBUG
  ONCRPC_LOG_EVENT6( ONCRPC_LOG_EVENT_STD_CALL,
                     xdr->xid, xdr->x_prog, xdr->x_proc,
                     xdr->name[0], xdr->name[1], xdr->name[2] );
#endif /* ONCRPC_SMEM_DEBUG */

  
  if( !XPORT_FLUSH_OUTPUT( xdr->xport ) ) {
    MSG_ERROR( "xdr_std_msg_send: unable to flush output for %x",(int) self, 0, 0 );
    oncrpc_queue_reply_remove( xdr );
    prv->out_msg_type = RPC_MSG_UNDEF;
    XDR_MSG_DONE( xdr );
    return FALSE;
  }

  ONCRPC_SYSTEM_LOGI("%s: Sent Xid: %x, Prog: %08x, Ver: %08x, Proc: %08x\n",
         __func__, xdr->xid, xdr->x_prog, xdr->x_vers, xdr->x_proc);

  prv->out_msg_type = RPC_MSG_UNDEF;

  /* initialize reply header before waiting for reply */
  memset( reply, 0, sizeof(rpc_reply_header) );

  sigs = oncrpc_event_wait( self, xdr->event );
  if( (sigs & xdr->event) == 0)
  {
    rpc_fake_reply(xdr);        
  }
  else
  {
  oncrpc_event_clr( self, xdr->event );
  }
  ONCRPC_SYSTEM_LOGI("%s: Received Reply Xid: %x, Prog: %08x, Ver: %08x, Proc: %08x\n",
         __func__, xdr->xid, xdr->x_prog, xdr->x_vers, xdr->x_proc);

  /* The XDR_FLAG_RETRY is set in oncrpc_retry_call() */
  if( xdr->flags & XDR_FLAG_RETRY ) {
    /* retry => return FALSE */
    xdr->flags &= ~XDR_FLAG_RETRY;
    return FALSE;
  }

  if( ! xdr_recv_reply_header( xdr, reply ) ) {
    ERR_FATAL( "unable to receive reply header", 0, 0, 0 );
  }

  if( reply->stat == RPC_MSG_ACCEPTED &&
      reply->u.ar.stat == RPC_PROG_LOCKED ) {
    /* returning FALSE => retry, so flush rest of reply */
    XDR_MSG_DONE( xdr );

    oncrpc_timer_set( prv->timer, ONCRPC_RETRY_TIME );
    oncrpc_event_wait( self, xdr->event );
    oncrpc_event_clr( self, xdr->event );

    return FALSE;
  }

  return TRUE;
} /* xdr_std_msg_send_call */


/*===========================================================================
FUNCTION ONCRPC_MSG_REPLY

DESCRIPTION

DEPENDENCIES
  None.

ARGUMENTS

RETURN VALUE
  None.

SIDE EFFECTS
  None.
===========================================================================*/
static void oncrpc_msg_reply(xdr_s_type *xdr)
{
#ifdef ONCRPC_SMEM_DEBUG
  ONCRPC_LOG_EVENT6( ONCRPC_LOG_EVENT_STD_REPLY, xdr->xid, 0, 0,
                     xdr->name[0], xdr->name[1], xdr->name[2] );
#endif /* ONCRPC_SMEM_DEBUG */
  ONCRPC_SYSTEM_LOGI("%s: Prog: %08x, Ver: %08x, Proc: %08x Xid: %08x\n",
                      __func__, xdr->x_prog, xdr->x_vers, xdr->x_proc, xdr->xid);
  if( !XPORT_FLUSH_OUTPUT( xdr->xport ) ) {
    MSG_ERROR( "oncrpc_msg_reply unable to flush output for %x",
               (int)oncrpc_thread_handle_get(), 0, 0 );
  }
}

static boolean xdr_std_msg_send( xdr_s_type *xdr, rpc_reply_header *reply )
{
  xdr_std_private_s_type *prv = (xdr_std_private_s_type *) xdr->xprivate;
  boolean result = TRUE;

  oncrpc_set_last_sent_msg_info( xdr->x_prog, xdr->x_vers, xdr->x_proc );

  switch( prv->out_msg_type ) {
    case RPC_MSG_CALL:
      /* Setup for reply, send message, then wait for reply  */
      result = xdr_std_msg_send_call( xdr, reply );
      break;

  case RPC_MSG_REPLY:
      oncrpc_msg_reply(xdr);
     
      prv->out_msg_type = RPC_MSG_UNDEF;
      break;

  case RPC_MSG_UNDEF:
      ERR_FATAL( "xdr_std_msg_send: no outgoing message in progress for %x",
				 (int) oncrpc_thread_handle_get(), 0, 0 ); 
      break;

    default:
      ERR_FATAL( "xdr_std_msg_send: out_msg_type invalid %x",
                 prv->out_msg_type, 0, 0 );
      break;
  }

  return result;
} /* xdr_std_msg_send */

static boolean xdr_std_msg_send_nonblocking
(
  xdr_s_type        * xdr,
  rpc_reply_cb_type   cb,
  void              * cb_data
)
{
  xdr_std_private_s_type *prv = (xdr_std_private_s_type *) xdr->xprivate;

#ifdef ONCRPC_SMEM_DEBUG
  ONCRPC_LOG_EVENT6( ONCRPC_LOG_EVENT_STD_CALL_ASYNC, 
                     xdr->xid, xdr->x_prog, xdr->x_proc,
                     xdr->name[0], xdr->name[1], xdr->name[2] );
#endif /* ONCRPC_SMEM_DEBUG */

  // succeed or fail we're done with the current message
  prv->out_msg_type = RPC_MSG_UNDEF;

  xdr->reply_cb = cb;
  xdr->reply_data = cb_data;

  // add reply to queue before flushing message - need to be ready for
  // a response at any time.
  oncrpc_queue_reply( xdr );

  ONCRPC_SYSTEM_LOGI("%s: Sending reply for xid: %x\n", __func__, xdr->xid);
  if( !XPORT_FLUSH_OUTPUT( xdr->xport ) ) {
    // flush failed presumably we'll never get a response
    // (and if we do it will be ignored)
    if( oncrpc_find_reply( xdr->xid ) == xdr ) {
      // we got the reply out of the queue before a response arrived,
      // se we free it
      XDR_DESTROY( xdr );
    }
    return FALSE;
  }

  return TRUE;
} /* xdr_std_msg_send_non_blocking */

/*===========================================================================
        Message Encode/Decode Functions
===========================================================================*/

static boolean xdr_std_send_int8
(
  xdr_s_type *xdr,
  const int8 *value
)
{
  int32 item;

  item = htonl( (int32) *value );
  return( XPORT_WRITE( xdr->xport, &item, sizeof(int32) ) == sizeof(int32) );
} /* xdr_std_send_int8 */

static boolean xdr_std_send_int8_w_errchk
(
  xdr_s_type *xdr,
  const int8 *value
)
{
  if ( ! xdr_std_send_int8(xdr, value) )
  {
    XDR_STD_SEND_ERR(xdr, "xdr_std_send_int8");
    return FALSE;
  }
  return TRUE;
} /* xdr_std_send_int8_w_errchk */

static boolean xdr_std_send_uint8
(
  xdr_s_type *xdr,
  const uint8 *value
)
{
  uint32 item;

  item = htonl( (uint32) *value );
  return( XPORT_WRITE( xdr->xport, &item, sizeof(uint32) ) == sizeof(uint32) );
} /* xdr_std_send_uint8 */

static boolean xdr_std_send_uint8_w_errchk
(
  xdr_s_type *xdr,
  const uint8 *value
)
{
  if ( ! xdr_std_send_uint8(xdr, value) )
  {
    XDR_STD_SEND_ERR(xdr, "xdr_std_send_uint8");
    return FALSE;
  }
  return TRUE;
} /* xdr_std_send_uint8_w_errchk */

static boolean xdr_std_send_int16
(
  xdr_s_type *xdr,
  const int16 *value
)
{
  int32 item;

  item = htonl( (int32) *value );
  return( XPORT_WRITE( xdr->xport, &item, sizeof(int32) ) == sizeof(int32) );
} /* xdr_std_send_int16 */

static boolean xdr_std_send_int16_w_errchk
(
  xdr_s_type *xdr,
  const int16 *value
)
{
  if ( ! xdr_std_send_int16(xdr, value) )
  {
    XDR_STD_SEND_ERR(xdr, "xdr_std_send_int16");
    return FALSE;
  }
  return TRUE;
} /* xdr_std_send_int16_w_errchk */

static boolean xdr_std_send_uint16
(
  xdr_s_type *xdr,
  const uint16 *value
)
{
  uint32 item;

  item = htonl( (uint32) *value );
  return( XPORT_WRITE( xdr->xport, &item, sizeof(uint32) ) == sizeof(uint32) );
} /* xdr_std_send_uint16 */

static boolean xdr_std_send_uint16_w_errchk
(
  xdr_s_type *xdr,
  const uint16 *value
)
{
  if ( ! xdr_std_send_uint16(xdr, value) )
  {
    XDR_STD_SEND_ERR(xdr, "xdr_std_send_uint16");
    return FALSE;
  }
  return TRUE;
} /* xdr_std_send_uint16_w_errchk */

static boolean xdr_std_send_int32
(
  xdr_s_type *xdr,
  const int32 *value
)
{
  int32 item;

  item = htonl( *value );
  return( XPORT_WRITE( xdr->xport, &item, sizeof(int32) ) == sizeof(int32) );
} /* xdr_std_send_int32 */

static boolean xdr_std_send_int32_w_errchk
(
  xdr_s_type *xdr,
  const int32 *value
)
{
  if ( ! xdr_std_send_int32(xdr, value) )
  {
    XDR_STD_SEND_ERR(xdr, "xdr_std_send_int32");
    return FALSE;
  }
  return TRUE;
} /* xdr_std_send_int32_w_errchk */

static boolean xdr_std_send_uint32
(
  xdr_s_type *xdr,
  const uint32 *value
)
{
  uint32 item;

  item = htonl( *value );
  return( XPORT_WRITE( xdr->xport, &item, sizeof(uint32) ) == sizeof(uint32) );
} /* xdr_std_send_uint32 */

static boolean xdr_std_send_uint32_w_errchk
(
  xdr_s_type *xdr,
  const uint32 *value
)
{
  if ( ! xdr_std_send_uint32(xdr, value) )
  {
    XDR_STD_SEND_ERR(xdr, "xdr_std_send_uint32");
    return FALSE;
  }
  return TRUE;
} /* xdr_std_send_uint32_w_errchk */

static boolean xdr_std_send_bytes
(
  xdr_s_type *xdr,
  const void *values,
  uint32 len
)
{
  uint32 pad;

  if( XPORT_WRITE( xdr->xport, values, len ) != len ) {
    return FALSE;
  }

  /* Pad output to multiple of 4 */
  if( len & 0x03 ) {
    len = 4 - (len & 0x03);
    pad = 0;

    if( XPORT_WRITE( xdr->xport, &pad, len) != len ) {
      return FALSE;
    }
  }

  return TRUE;
} /* xdr_std_send_bytes */

static boolean xdr_std_send_bytes_w_errchk
(
  xdr_s_type *xdr,
  const void *values,
  uint32 len
)
{
  if ( ! xdr_std_send_bytes(xdr, values, len) )
  {
    XDR_STD_SEND_ERR(xdr, "xdr_std_send_bytes");
    return FALSE;
  }
  return TRUE;
} /* xdr_std_send_bytes_w_errchk */

static boolean xdr_std_send_dsm
(
  xdr_s_type *xdr,
  dsm_item_type **item
)
{
  dsm_item_type *dup;
  uint16 size;

  size = (uint16) dsm_length_packet(*item);

  if ( dsm_dup_packet(&dup, *item, 0, size) != size )
  {
    ERR_FATAL("xdr_std_send_dsm: dsm_dup failed (item=%d, size=%d).",
              item, size, 0);
  }

  return XPORT_WRITE_DSM( xdr->xport, &dup );
} /* xdr_std_send_dsm */

static boolean xdr_std_send_dsm_w_errchk
(
  xdr_s_type *xdr,
  dsm_item_type **item
)
{
  if ( ! xdr_std_send_dsm(xdr, item) )
  {
    XDR_STD_SEND_ERR(xdr, "xdr_std_send_dsm");
    return FALSE;
  }
  return TRUE;
} /* xdr_std_send_dsm_w_errchk */

static boolean xdr_std_recv_int8
(
  xdr_s_type *xdr,
  int8 *value
)
{
  xdr_std_private_s_type *prv = (xdr_std_private_s_type *) xdr->xprivate;

  int32  item;

  if( dsm_pullup(&prv->in_msg, &item, sizeof(int32)) != sizeof(int32) ) {
    return FALSE;
  }

  *value = (int8) ntohl( item );
  return TRUE;
} /* xdr_std_recv_int8 */

static boolean xdr_std_recv_int8_w_errchk
(
  xdr_s_type *xdr,
  int8 *value
)
{
  if ( ! xdr_std_recv_int8(xdr, value) )
  {
    XDR_STD_RECV_ERR(xdr, "xdr_std_recv_int8");
    return FALSE;
  }
  return TRUE;
} /* xdr_std_recv_int8_w_errchk */

static boolean xdr_std_recv_uint8
(
  xdr_s_type *xdr,
  uint8 *value
)
{
  xdr_std_private_s_type *prv = (xdr_std_private_s_type *) xdr->xprivate;
  uint32 item;

  if( dsm_pullup(&prv->in_msg, &item, sizeof(uint32)) != sizeof(uint32)) {
    return FALSE;
  }

  *value = (uint8) ntohl( item );
  return TRUE;
} /* xdr_std_recv_uint8 */

static boolean xdr_std_recv_uint8_w_errchk
(
  xdr_s_type *xdr,
  uint8 *value
)
{
  if ( ! xdr_std_recv_uint8(xdr, value) )
  {
    XDR_STD_RECV_ERR(xdr, "xdr_std_recv_uint8");
    return FALSE;
  }
  return TRUE;
} /* xdr_std_recv_uint8_w_errchk */

static boolean xdr_std_recv_int16
(
  xdr_s_type *xdr,
  int16 *value
)
{
  xdr_std_private_s_type *prv = (xdr_std_private_s_type *) xdr->xprivate;
  int32 item;

  if( dsm_pullup(&prv->in_msg, &item, sizeof(int32)) != sizeof(int32) ) {
    return FALSE;
  }

  *value = (int16) ntohl( item );
  return TRUE;
} /* xdr_std_recv_uint16 */

static boolean xdr_std_recv_int16_w_errchk
(
  xdr_s_type *xdr,
  int16 *value
)
{
  if ( ! xdr_std_recv_int16(xdr, value) )
  {
    XDR_STD_RECV_ERR(xdr, "xdr_std_recv_int16");
    return FALSE;
  }
  return TRUE;
} /* xdr_std_recv_int16_w_errchk */

static boolean xdr_std_recv_uint16
(
  xdr_s_type *xdr,
  uint16 *value
)
{
  xdr_std_private_s_type *prv = (xdr_std_private_s_type *) xdr->xprivate;
  uint32 item;

  if( dsm_pullup(&prv->in_msg, &item, sizeof(uint32)) != sizeof(uint32)) {
    return FALSE;
  }

  *value = (uint16) ntohl( item );
  return TRUE;
} /* xdr_std_recv_uint16 */

static boolean xdr_std_recv_uint16_w_errchk
(
  xdr_s_type *xdr,
  uint16 *value
)
{
  if ( ! xdr_std_recv_uint16(xdr, value) )
  {
    XDR_STD_RECV_ERR(xdr, "xdr_std_recv_uint16");
    return FALSE;
  }
  return TRUE;
} /* xdr_std_recv_uint16_w_errchk */

static boolean xdr_std_recv_int32
(
  xdr_s_type *xdr,
  int32 *value
)
{
  xdr_std_private_s_type *prv = (xdr_std_private_s_type *) xdr->xprivate;
  int32 item;

  if( dsm_pullup(&prv->in_msg, &item, sizeof(int32)) != sizeof(int32) ) {
    return FALSE;
  }

  *value = ntohl( item );
  return TRUE;
} /* xdr_std_recv_int32 */

static boolean xdr_std_recv_int32_w_errchk
(
  xdr_s_type *xdr,
  int32 *value
)
{
  if ( ! xdr_std_recv_int32(xdr, value) )
  {
    XDR_STD_RECV_ERR(xdr, "xdr_std_recv_int32");
    return FALSE;
  }
  return TRUE;
} /* xdr_std_recv_int32_w_errchk */

static boolean xdr_std_recv_uint32
(
  xdr_s_type *xdr,
  uint32 *value
)
{
  xdr_std_private_s_type *prv = (xdr_std_private_s_type *) xdr->xprivate;
  uint32 item;

  if( dsm_pullup(&prv->in_msg, &item, sizeof(uint32)) != sizeof(uint32)) {
    return FALSE;
  }

  *value = ntohl( item );
  return TRUE;
} /* xdr_std_recv_uint32 */

static boolean xdr_std_recv_uint32_w_errchk
(
  xdr_s_type *xdr,
  uint32 *value
)
{
  if ( ! xdr_std_recv_uint32(xdr, value) )
  {
    XDR_STD_RECV_ERR(xdr, "xdr_std_recv_uint32");
  }
  return TRUE;
} /* xdr_std_recv_uint32_w_errchk */

static boolean xdr_std_recv_bytes
(
  xdr_s_type *xdr,
  void *values,
  uint32 len
)
{
  xdr_std_private_s_type *prv = (xdr_std_private_s_type *) xdr->xprivate;
  uint32 pad;
  uint16 pad_len;
  uint16 pullup_len;

  pad_len = (uint16) (len & 0x03);

  while( len != 0 )
  {
    pullup_len = (uint16) MIN( len, 0xFFFF );
    if( dsm_pullup( &prv->in_msg, values, pullup_len ) != pullup_len ) {
      return FALSE;
    }

    len -= (uint32) pullup_len;
  }

  /* Receive pad to multiple of 4 */
  if( pad_len != 0 ) {
    pad = 0;

    if( dsm_pullup( &prv->in_msg, &pad, (4 - pad_len)) != (4 - pad_len) ) {
      return FALSE;
    }
  }

  return TRUE;
} /* xdr_std_recv_bytes */

static boolean xdr_std_recv_bytes_w_errchk
(
  xdr_s_type *xdr,
  void *values,
  uint32 len
)
{
  if ( ! xdr_std_recv_bytes(xdr, values, len) )
  {
    XDR_STD_RECV_ERR(xdr, "xdr_std_recv_bytes");
    return FALSE;
  }
  return TRUE;
} /* xdr_std_recv_bytes_w_errchk */

static boolean xdr_std_recv_dsm
(
  xdr_s_type *xdr,
  dsm_item_type **item
)
{
  xdr_std_private_s_type *prv = (xdr_std_private_s_type *) xdr->xprivate;
  *item = prv->in_msg;
  prv->in_msg = NULL;

  return TRUE;
} /* xdr_std_recv_dsm */

static boolean xdr_std_recv_dsm_w_errchk
(
  xdr_s_type *xdr,
  dsm_item_type **item
)
{
  if ( ! xdr_std_recv_dsm(xdr, item) )
  {
    XDR_STD_RECV_ERR(xdr, "xdr_std_recv_dsm");
    return FALSE;
  }
  return TRUE;
} /* xdr_std_recv_dsm_w_errchk */
