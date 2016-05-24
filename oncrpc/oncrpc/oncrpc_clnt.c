/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        O N C R P C _ C L N T . C

GENERAL DESCRIPTION

  This file provides the client handling functions for ONCRPC.

  ONCRPC (Open Network Computing Remote Procedure Calls) is a way of
  calling functions from one machine to another.  This is described in
  detail in the following RFCs: 1831, 1832, 1833

INITIALIZATION AND SEQUENCING REQUIREMENTS

 Copyright (c) 2005,2007-2008, 2011 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_clnt.c#4 $ 

when        who    what, where, why
--------    ---    ----------------------------------------------------------
09/11/08    rr     Merge from mainline, cleanup includes
02/04/08    hn     Specify infinite timeout for automatic lookup.
01/21/08    hn     Added calls to oncrpc_setup_call() in clnt_call routines.
08/29/07    ptm    Fix compiler warnings.
07/10/07    ptm    Cleanup featurization.
05/23/07    clp    Added clnt_call_non_blocking and made clnt_call blocking
07/13/05    clp    Added clnt_destroy.
07/13/05    clp    Added XDR_CLONE in client_call.
07/13/05    clp    added memset of reply header so auth init'd.
06/04/05    ptm    Add block comments.
05/23/05    hn     Changed clnt_freeres() to call oncrpcxdr_mem_free()
04/12/05    ptm    Include oncrpci.h.
04/01/05    clp    Include header cleanup changes.
03/16/05    clp    Added header to source file.
===========================================================================*/

#include "comdef.h"
#include "oncrpc.h"
#include "oncrpci.h"
#include "oncrpc_taski.h"
#include "oncrpc_lookup.h"

struct clnt_call_cb_struct {
  xdr_s_type * xdr;
  xdrproc_t    xdr_results;
  int          results_size;
  clnt_call_cb result_cb;
  void *       cb_data;
  uint32       xid;
};

typedef struct clnt_call_cb_struct clnt_call_cb_s_type;

/*===========================================================================
FUNCTION CLNT_CREATE

DESCRIPTION
  RPCGEN support routine. This routine calls the approprate transport
  specific client create call. 
  
DEPENDENCIES
  None.

ARGUMENTS
  host - who to connect to
  prog - which prog to connect to 
  vers - which version to connect to 
  proto - how to connect

RETURN VALUE
  xdr - pointer to XDR structure

SIDE EFFECTS
  None.
===========================================================================*/
CLIENT *
clnt_create
(
  char * host,
  uint32 prog,
  uint32 vers,
  char * proto
)
{  
#ifndef FEATURE_ONCRPC_LO
{

    uint32 dest_handle;

    CLIENT *clnt = NULL;
    oncrpc_control_open_prog_vers_type open_args;
    clnt = clntrtr_create_dedicated( prog, vers, ONCRPC_REPLY_SIG );
    if (!clnt)
      return NULL;

    open_args.prog_ver.prog = prog;
    open_args.prog_ver.ver = vers;

/* Lookup the destination in the local database */
   
  dest_handle = (uint32)oncrpc_lookup_get_dest(prog,vers);
  if( dest_handle )
  {
    /* Set the destination in lower layer */
    XDR_CONTROL(clnt,ONCRPC_CONTROL_SET_DEST,(void *)dest_handle);
  }
  else
  {
    oncrpc_control_open_prog_vers_type open_args;
    open_args.prog_ver.prog = prog;
    open_args.prog_ver.ver = vers;
    XDR_CONTROL(clnt,ONCRPC_CONTROL_OPEN_PROG_VERS,(void *)&open_args);
    {
      oncrpc_lookup_add_dest(prog,vers,open_args.handle);
      XDR_CONTROL(clnt,ONCRPC_CONTROL_SET_DEST,(void *)&open_args.handle);
    }
  }

  return clnt;
}

#else
  return clntlo_create_dedicated( prog, vers, ONCRPC_REPLY_SIG );
#endif
}

/*===========================================================================
FUNCTION CLNT_CALL_CALLBACK

DESCRIPTION
  RPCGEN support routine. This routine is called when a response is received
  for an RPC call that was made from RPCGEN code. It decodes the response and
  calls the client's callback routine with the result structure.
  
DEPENDENCIES
  None.

ARGUMENTS
  xdr - pointer to XDR structure
  cookie - call back info

RETURN VALUE
  None.

SIDE EFFECTS
  None.
===========================================================================*/
static void 
clnt_call_callback
(
  xdr_s_type *xdr,
  void *cookie
)
{
  clnt_call_cb_s_type * data = (clnt_call_cb_s_type *) cookie;
  rpc_reply_header      reply_hdr;
  void * resp = NULL;

  ASSERT( NULL != data );

  memset( &reply_hdr, 0, sizeof( rpc_reply_header ) );
  xdr->x_op = XDR_DECODE;

  // error status next
  xdr_recv_reply_header( xdr, &reply_hdr );

  if( reply_hdr.stat == RPC_MSG_ACCEPTED )
  {
    if ( reply_hdr.u.ar.stat == RPC_ACCEPT_SUCCESS )
    {
      resp = oncrpc_mem_alloc( data->results_size );
      if( resp == NULL )
      {
        reply_hdr.u.ar.stat = RPC_SYSTEM_ERR;
      }
      else if( !data->xdr_results( xdr, resp ) )
      {
        reply_hdr.u.ar.stat = RPC_GARBAGE_ARGS;
      }
    }
  }

  if( data->result_cb != NULL ) {
    data->result_cb( data->xdr, data->cb_data, resp, reply_hdr );
  }

  XDR_MSG_DONE( xdr );
  XDR_DESTROY( xdr );
  oncrpc_mem_free( data );
} /* clnt_call_callback */

/*===========================================================================
FUNCTION CLNT_CALL

DESCRIPTION
  RPCGEN support routine. This routine is called by client routines generated
  by RPCGEN. It generates and sends an RPC message to a server.

  This is a blocking call.   

DEPENDENCIES
  None.

ARGUMENTS
  xdr - the XDR to use to send the RPC message
  proc - the server procedure to call
  xdr_args - function pointer for encoding the RPC message args
  args_ptr - pointer to args data structure
  xdr_results - function pointer for decoding the RPC response
  rets_ptr - pointer to results data structure
  timeout - return after timeout (ignored)

RETURN VALUE
  RPC_SUCCESS - if successful
  error code otherwise

SIDE EFFECTS
  None.
===========================================================================*/
enum clnt_stat 
clnt_call
( 
  CLIENT       * xdr, 
  u_long         proc,
  xdrproc_t      xdr_args,
  caddr_t        args_ptr,
  xdrproc_t      xdr_results, 
  caddr_t        rets_ptr,
  struct timeval timeout
)
{
  opaque_auth cred;
  opaque_auth verf;
  rpc_reply_header reply_header;


  cred.oa_flavor = ONCRPC_AUTH_NONE;
  cred.oa_length = 0;
  verf.oa_flavor = ONCRPC_AUTH_NONE;
  verf.oa_length = 0;

  if( (xdr->flags & XDR_FLAG_DEDICATED) != XDR_FLAG_DEDICATED ) {
    ERR_FATAL( "clnt_call requires dedicated XDR", 0, 0, 0 );
  }

  /* Do any special setup stuff for the RPC here */
  if ( ! oncrpc_setup_call( xdr, (timeout.tv_usec*1000) + (timeout.tv_sec/1000) ) )
  {
    return RPC_CANTSEND;
  }

  xdr->x_op = XDR_ENCODE;
  /* Send message header and arguments */
  if( !xdr_call_msg_start( xdr, xdr->x_prog, xdr->x_vers, 
                           proc, &cred, &verf) ||
      !xdr_args(xdr, args_ptr) ) {
    XDR_MSG_ABORT( xdr );
    return RPC_CANTENCODEARGS;
  }

  /* Finish message - blocking */
  if( !XDR_MSG_SEND( xdr, &reply_header ) ) {
    return RPC_CANTSEND;
  }

  /* Check that other side accepted and responded */
  if ( reply_header.stat != RPC_MSG_ACCEPTED ) {
    ERR("RPC call rejected, reject status %d", reply_header.u.dr.stat, 0, 0);
    /* Offset to map returned error into clnt_stat */
    return (enum clnt_stat)( (uint32)reply_header.u.dr.stat +
                             (uint32)RPC_VERSMISMATCH );

  } else if ( reply_header.u.ar.stat != RPC_ACCEPT_SUCCESS ) {

    ERR("Server side error, error status %d", reply_header.u.ar.stat, 0, 0);
    /* Offset to map returned error into clnt_stat */
    return (enum clnt_stat)( (uint32)reply_header.u.ar.stat +
                             (uint32)RPC_AUTHERROR);
  }

  xdr->x_op = XDR_DECODE;
  /* Decode results */
  if ( !xdr_results(xdr, rets_ptr) || ! XDR_MSG_DONE(xdr) ) {
    return RPC_CANTDECODEARGS;
  }

  return RPC_SUCCESS;
} /* clnt_call */

/*===========================================================================
FUNCTION CLNT_CALL_NON_BLOCKING

DESCRIPTION
  RPCGEN support routine. This routine is called by client routines generated
  by RPCGEN. It generates and sends an RPC message to a server.
oncrpcclnt.c:199:
  This is a non-blocking call. It registers clnt_call_callback to be called
  when the RPC response is received.
  
DEPENDENCIES
  None.

ARGUMENTS
  xdr - the XDR to use to send the RPC message
  proc - the server procedure to call
  xdr_args - function pointer for encoding the RPC message args
  args_ptr - pointer to args data structure
  xdr_results - function pointer for decoding the RPC response
  results_size - size of the results data structure
  result_cb - function pointer to be called with the results
  cb_data - cookie for results call back function

RETURN VALUE
  RPC_SUCCESS - if successful
  error code otherwise

SIDE EFFECTS
  None.
===========================================================================*/
enum clnt_stat clnt_call_non_blocking
(
  CLIENT *xdr,
  u_long proc,
  xdrproc_t xdr_args,
  caddr_t args_ptr,
  xdrproc_t xdr_results,
  int results_size,
  clnt_call_cb result_cb,
  void * cb_data
)
{
  opaque_auth cred;
  opaque_auth verf;
  clnt_call_cb_s_type *data;
  xdr_s_type * clone;

  cred.oa_flavor = ONCRPC_AUTH_NONE;
  cred.oa_length = 0;
  verf.oa_flavor = ONCRPC_AUTH_NONE;
  verf.oa_length = 0;

  if( (xdr->flags & XDR_FLAG_DEDICATED) != XDR_FLAG_DEDICATED ) {
    ERR_FATAL( "clnt_call requires dedicated XDR", 0, 0, 0 );
  }

  clone = XDR_CLONE( xdr );

  /* Do any special setup stuff for the RPC here. */
  /* Since this is clnt_call_nonblocking, use zero time out. */
  if ( ! oncrpc_setup_call( clone, 0 ) )
  {
    return RPC_CANTSEND;
  }

  clone->x_op = XDR_ENCODE;
  // Send message header and arguments
  if( !xdr_call_msg_start( clone, clone->x_prog, clone->x_vers, 
                           proc, &cred, &verf) ||
      !(*xdr_args) (clone, args_ptr) ) {
    XDR_MSG_ABORT( clone );
    return( RPC_CANTENCODEARGS );
  }

  data = oncrpc_mem_alloc( sizeof(clnt_call_cb_s_type) );
  if( data == NULL ) {
    XDR_MSG_ABORT( clone );
    return RPC_SYSTEMERROR;
  }

  data->xdr          = xdr;
  data->xdr_results  = xdr_results;
  data->results_size = results_size;
  data->result_cb    = result_cb;
  data->cb_data      = cb_data;
  data->xid          = clone->xid;


  // Finish message - non blocking
  if( !XDR_MSG_SEND_NONBLOCKING( clone, clnt_call_callback, data ) ) {
    return RPC_CANTSEND;
  }

  return RPC_SUCCESS;
} /* clnt_call_non_blocking */

/*===========================================================================
FUNCTION CLNT_FREERES

DESCRIPTION
  RPCGEN support routine. This routine is called by server routines to free
  any resources that were allocated when decoding the RPC call and by client
  routines to free any resources that were allocated when decoding the RPC
  response.
  
DEPENDENCIES
  None.

ARGUMENTS
  xdr - the XDR containing resources that need to be freed
  xdr_res - not used
  res_ptr - not used

RETURN VALUE
  Always TRUE.
sizeof(struct sockaddr)
SIDE EFFECTS
  None.
===========================================================================*/
bool_t clnt_freeres
(
  CLIENT *xdr,
  xdrproc_t xdr_res,
  caddr_t res_ptr
)
{
  oncrpcxdr_mem_free( xdr );

  return TRUE;
} /* clnt_freeres */

/*===========================================================================
FUNCTION CLNT_DESTORY

DESCRIPTION
  Used by clients to destroy an XDR when they are done with it. The destroy
  operation must be serialized through the ONCRPC task.
  
DEPENDENCIES
  None.

ARGUMENTS
  xdr - the XDR to be destroyed

RETURN VALUE
  None.

SIDE EFFECTS
  None.
===========================================================================*/
void
clnt_destroy
(
  CLIENT *xdr
)
{
  rpc_cmd_type cmd;

  if ( NULL != xdr && 
       XDR_FLAG_DESTROYING != (xdr->flags & XDR_FLAG_DESTROYING) )
  {
    xdr->flags |= XDR_FLAG_DESTROYING;
    cmd.cmd = RPC_DESTROY;
    cmd.xdr = xdr;
    rpc_cmd( &cmd );
  }
} /* clnt_destroy */
