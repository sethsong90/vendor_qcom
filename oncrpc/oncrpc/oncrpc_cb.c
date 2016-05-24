/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        O N C R P C _ C B . C

GENERAL DESCRIPTION

  This file provides the support routines for a callback mechinism in
  ONCRPC.

  ONCRPC (Open Network Computing Remote Procedure Calls) is a way of
  calling functions from one machine to another.  This is described in
  detail in the following RFCs: 1831, 1832, 1833

INITIALIZATION AND SEQUENCING REQUIREMENTS

 Copyright (c) 2005-2008, 2011, 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_cb.c#6 $ 

when        who    what, where, why
--------    ---    ----------------------------------------------------------
28/07/11    as     Enable userspace logging
12/01/08    zap    There is a small chance that rpc_svc_callback_register
                   will match the wrong entry if cb_handler isn't also
                   checked.
06/13/08    rr     Add default cb handler instead of null, for callbacks
                   received after de-registration. SR 1084246
03/25/08    ih     Changed thread_exit to use oncrpc_tls_find 
03/24/08    ih     Moved common client creation code into rpc_xdr_get
02/05/08    hn     Moved assignment out of if() condition.
01/21/08    hn     Added oncrpc_setup_call routine.
10/30/07    rr     Fix timeout parameter in lookup2 function
10/17/07    hn     tls_get and tls_delete now take a thread's key, added
                   tls_get_self and tls_delete_self as the no args version
08/22/07    ptm    Unified access to thread local storage.
08/20/07    ih     Added callback message source check that was previously 
                   commented out.
07/10/07    ptm    Cleaned up featurization.
05/15/07    RJS    Split OS specific parts into seperate files. 
                   Remove Int and task locks
05/09/07    hn     Added timeout parameter to rpc_clnt_lookup2.
05/04/07    hn     Added hooks for handling of msg source information for
                   future support of multiple sources.
05/04/07    hn     Added rpc_clnt_lookup2
03/23/07    hn     Added initialization routine for callback registry.
03/06/07    hn     Added critical sections to callback registry.
12/08/06    ptm    Increase the callback table size from 100 to 200.
03/03/06    ptm    Add featurization around TCP and UDP references
09/13/05    clp    Split code from oncrpcxdr.c
===========================================================================*/

#include "oncrpc.h"
#include "oncrpci.h"
#include "oncrpc_os.h"
#include "oncrpc_main.h"
#include "oncrpc_lookup.h"

/*===========================================================================
 *        Callback Registry Definitions
 *===========================================================================*/

#define RPC_MAX_NUM_CALLBACKS    200
#define RPC_CALLBACK_IS_NULL     0xffffffff

#define RPC_CALLBACK_ENTRY_UNASSIGNED   (0x00000000)
static void                  *rpc_clnt_cb_registry[RPC_MAX_NUM_CALLBACKS];
static oncrpc_crit_sect_ptr  rpc_clnt_cb_registry_crit_section;
static uint32                rpc_num_clnt_callbacks = 0;

static rpc_cb_data_type      rpc_svc_cb_registry[RPC_MAX_NUM_CALLBACKS];
static oncrpc_crit_sect_ptr  rpc_svc_cb_registry_crit_section;

/*===========================================================================
  FUNCTION: oncrpc_thread_exit

  DESCRIPTION:
  Called when a thread exits to release any resources associated with the
  thread.

  Should clean up the following information:

  1. Client XDR.
  2. Server XDR.
  3. Any thread local memory associated with the thread.

  RESULT:
  NONE
  ===========================================================================*/
void oncrpc_thread_exit( void * key )
{
  oncrpc_tls_type *tls = oncrpc_tls_find(key);

  if( tls == NULL )
    return;

  if( tls->clnt != NULL )
  {
    XDR_DESTROY( tls->clnt );
  }

  if( tls->xdr != NULL )
  {
    XDR_DESTROY( tls->xdr );
  }

  oncrpc_tls_delete(key);
} /* oncrpc_thread_exit */

/*===========================================================================
  FUNCTION: rpc_xdr_get

  DESCRIPTION:
  Get XDR from TLS. Create it and register with TLS if one has not been created

  RESULT:
  Pointer to XDR on success, else NULL
  ===========================================================================*/
xdr_s_type *rpc_xdr_get( oncrpc_tls_type *tls )
{
  xdr_s_type *clnt = tls->clnt;

  if( clnt == NULL )
  {
#ifdef FEATURE_ONCRPC_LO
    clnt = clntlo_create(ONCRPC_REPLY_SIG);
#else
    clnt = clntrtr_create(ONCRPC_REPLY_SIG);
#endif 
    tls->clnt = clnt;
  }

  return clnt;
}

/*===========================================================================
  FUNCTION: rpc_clnt_register

  DESCRIPTION:
  Register the client based on the current task.

  RESULT:
  TRUE if client was registered, FALSE if there was an error
  ===========================================================================*/
boolean rpc_clnt_register( xdr_s_type *clnt )
{
  oncrpc_tls_type *tls = oncrpc_tls_get_self();

  if( clnt == NULL ) {
    ERR_FATAL("rpc_clnt_register: task %x tried to register a NULL client",
      (uint32) tls, 0, 0);
  }

  if( tls->clnt == NULL ) {
    tls->clnt = clnt;
  }
  else if( tls->clnt != clnt ) {
    ERR_FATAL("task %x attempt to register a 2nd client", (uint32) tls, 0, 0 );
  }

  return TRUE;
} /* rpc_clnt_register */

/*===========================================================================
  FUNCTION: rpc_clnt_deregister

  DESCRIPTION:
  Deregister the client

  RESULT:
  TRUE if client was deregistered, FALSE if there was an error
  ===========================================================================*/
boolean rpc_clnt_deregister( void )
{
  oncrpc_tls_type *tls = oncrpc_tls_get_self();

  if( tls->clnt == NULL ) {
    ERR_FATAL("rpc_clnt_deregister: task %x tried to deregister a NULL client",
      (uint32) tls, 0, 0);
  }

  tls->clnt = NULL;

  return TRUE;
} /* rpc_clnt_deregister */

/*===========================================================================
  FUNCTION: oncrpc_setup_call

  DESCRIPTION:
  Sets up an RPC based on the initialized client XDR and a time out value.

  DEPENDENCIES:
  The "x_prog" and "x_vers" fields of the clnt XDR must have been initialized.

  RESULT:
  TRUE if RPC was setup successfully. FALSE if an error occured.
  ===========================================================================*/
boolean oncrpc_setup_call( xdr_s_type *clnt, uint32 timeout )
{
  boolean ret_val = TRUE;
#ifndef FEATURE_ONCRPC_LO
  uint32 dest_handle;
  oncrpc_control_open_prog_vers_type open_args;
  oncrpc_control_set_dest_type  set_dest_args;

  ASSERT( clnt != NULL );
  open_args.prog_ver.prog = clnt->x_prog;
  open_args.prog_ver.ver = clnt->x_vers;

  if ( clnt->protocol == ONCRPC_RTR_PROTOCOL )
  {
  /* Lookup the destination in the local database */
    dest_handle = (uint32) oncrpc_lookup_get_dest(clnt->x_prog,clnt->x_vers);
    if( dest_handle  )
    {
    /* Set the destination in lower layer */
      set_dest_args.dest = dest_handle;
      XDR_CONTROL(clnt,ONCRPC_CONTROL_SET_DEST,(void *)dest_handle);
    }
    else
    {      
       XDR_CONTROL(clnt,ONCRPC_CONTROL_OPEN_PROG_VERS,(void *)&open_args);
       if( open_args.handle == -1 )
       {
         ERR("Open handle failed, unable to open program %x ver %x NOT FOUND \n",(int)open_args.prog_ver.prog,
          (int)open_args.prog_ver.ver,0);
         ret_val= FALSE;
        }
        else
        {
           oncrpc_lookup_add_dest(clnt->x_prog,clnt->x_vers,open_args.handle);      
           set_dest_args.dest = open_args.handle;
           XDR_CONTROL(clnt,ONCRPC_CONTROL_SET_DEST,(void *)&set_dest_args);
         }
      }
   }
#endif /* !FEATURE_ONCRPC_LO */
return ret_val;
} /* oncrpc_setup_call */



/*===========================================================================
  FUNCTION: rpc_clnt_lookup2

  DESCRIPTION:
  Lookup a client based on the current task and the RPC program it wants to
  talk to.

  RESULT:
  pointer to XPORT structure
  ===========================================================================*/
xdr_s_type *rpc_clnt_lookup2( rpcprog_t prog, rpcvers_t ver, uint32 timeout )
{
  oncrpc_tls_type  *tls = oncrpc_tls_get_self();
  xdr_s_type       *clnt;

  clnt = rpc_xdr_get(tls);

  if( clnt == NULL )
  {
    ERR_FATAL( "Unable to create shared memory client for task %x",
      (uint32) tls, 0, 0 );
  }

  clnt->x_prog = prog;
  clnt->x_vers = ver;

  if ( ! oncrpc_setup_call(clnt, timeout) )
  {
    ERR( "Couldn't setup RPC call",0,0,0 );
    return NULL;
  }
  ONCRPC_SYSTEM_LOGI("Setup RPC Call for task %x\n", (unsigned)tls);

  return clnt;
} /* rpc_clnt_lookup2 */

/*===========================================================================
  FUNCTION: rpc_clnt_lookup (obsolete)

  DESCRIPTION:
  Lookup a client based on the current task.

  RESULT:
  pointer to XPORT structure
  ===========================================================================*/
xdr_s_type *rpc_clnt_lookup( void )
{
  /* FIXME, XXX, TODO: Hard code remote router ID as the version -- defined in 
   * the builds file */
  return rpc_clnt_lookup2( 0x3000fffe,
    RPC_ROUTER_REMOTE_DEFAULT_PROCESSOR_ID,
    (uint32)ONCRPC_INFINITY );
} /* rpc_clnt_lookup */

/*===========================================================================
  FUNCTION: rpc_init_callback_registry

  DESCRIPTION:
      Initializes data structures associated with the callback registry.

  RESULT:
      None.
  ===========================================================================*/
void rpc_init_callback_registry( void )
{
  oncrpc_crit_sect_init(&rpc_clnt_cb_registry_crit_section);
  oncrpc_crit_sect_init(&rpc_svc_cb_registry_crit_section);
} /* rpc_init_callback_registry */

/*===========================================================================
  FUNCTION: rpc_clnt_callback_register

  DESCRIPTION:
      Register a callback function address and return an callback id which
      can be used to lookup the callback function address.

  RESULT:
      Callback ID
  ===========================================================================*/
uint32 rpc_clnt_callback_register( void *cb_func )
{
  uint32 cb_id;

  if( cb_func == NULL ) {
    return RPC_CALLBACK_IS_NULL;
  }

  oncrpc_crit_sect_enter(rpc_clnt_cb_registry_crit_section);

  for( cb_id = 0;
    ( cb_id < rpc_num_clnt_callbacks &&
    rpc_clnt_cb_registry[cb_id] != cb_func );
    cb_id++ ) {
  }

  if( cb_id == rpc_num_clnt_callbacks ) {
    /* callback id not found */
    if( rpc_num_clnt_callbacks == RPC_MAX_NUM_CALLBACKS ) {
      ERR_FATAL( "RPC clnt callback registry overflow", 0, 0, 0 );
    }

    rpc_clnt_cb_registry[rpc_num_clnt_callbacks++] = cb_func;
  }

  oncrpc_crit_sect_leave(rpc_clnt_cb_registry_crit_section);

  return cb_id;
} /* rpc_clnt_callback_register */

/*===========================================================================
  FUNCTION: rpc_clnt_callback_lookup

  DESCRIPTION:
      Lookup a callback function address based on a callback id.

  RESULT:
      Callback function address
  ===========================================================================*/
void *rpc_clnt_callback_lookup( uint32 cb_id )
{
  if( cb_id < rpc_num_clnt_callbacks ) {
    return rpc_clnt_cb_registry[cb_id];
  }

  return NULL;
} /* rpc_clnt_callback_lookup */

/*===========================================================================
  FUNCTION: rpc_svc_default_cb_handler

  DESCRIPTION:
      When a callback is de-registered, this function pointer is placed
	  in the callback table instead of NULL, it is treated as an available
	  callback to be "recycled" and reused if a new callback allocaiton is needed.
	  The callbacks are allocated in round-robin.

      This function should normally not be called, but in some servers there
	  is a delay between the de-registration call and the actual disabling of
	  the callback, so the callback could be called after the de-registration.
	  In that case, this function will be called.


  RESULT:
      Pointer to client
===========================================================================*/
void rpc_svc_default_cb_handler(void)
{
   rpc_cb_data_type *rpc_cb_data;
   
   rpc_cb_data = rpc_svc_cb_data_lookup();

  if ( rpc_cb_data == NULL )
  {
    RPC_SVC_CB_DATA_LOOKUP_ERR();
    return;
  }
  ERR( "Default cb handler called, calling callback after de-registration? handler:0x%08x cbid:%d\n ",rpc_cb_data->cb_handler,rpc_cb_data->cb_id,0);
}

/*===========================================================================
  FUNCTION: rpc_clnt_for_callback

  DESCRIPTION:
      Gets or creates a client for a particular protocol. This is used by
      callback support to get a client to send the callback message to.

  RESULT:
      Pointer to client
  ===========================================================================*/
xdr_s_type *rpc_clnt_for_callback( rpc_cb_data_type *cb_data )
{
  oncrpc_tls_type *tls;
  xdr_s_type      *clnt;

  switch( cb_data->protocol ) {
    case ONCRPC_RTR_PROTOCOL:
      {
        tls = oncrpc_tls_get_self();
        clnt = rpc_xdr_get(tls);

      if( clnt == NULL )
      {
          ERR_FATAL( "Unable to create shared memory client for task %x",
            (uint32) tls, 0, 0 );
        }

        /* Set the destination in lower layer */
        XDR_CONTROL(clnt,ONCRPC_CONTROL_SET_DEST,(void *)&cb_data->cb_source);
        break;
      }

    case ONCRPC_SIO_PROTOCOL:
    case ONCRPC_DIAG_PROTOCOL:
      /*
        TODO - put this case back after we stop defining ONCRPC_SM_PROTOCOL to
        be ONCRPC_RTR_PROTOCOL?
      case ONCRPC_SM_PROTOCOL:
      */
#ifdef FEATURE_ONCRPC_TCP
    case IPPROTO_TCP:
#endif /* FEATURE_ONCRPC_TCP */
#ifdef FEATURE_ONCRPC_UDP
    case IPPROTO_UDP:
#endif /* FEATURE_ONCRPC_UDP */
    default:
      ERR_FATAL( "Callback on protocol %d unimplemented",
        cb_data->protocol, 0, 0 );
      clnt = NULL;
      break;
  }

  return clnt;
} /* rpc_clnt_for_callback */

extern void *oncrpc_cb_bridge_table[]; /* defined in oncrpc_cb.s */
/*===========================================================================
  FUNCTION: rpc_svc_callback_register

  DESCRIPTION:
      Register a callback handler function address, callback id and protocol.

      Use ...

  RESULT:
      Function pointer to callback bridge function associated with the
      table entry.
  ===========================================================================*/
void *rpc_svc_callback_register
  (
  void *cb_handler,
  xdr_s_type *clnt,
  uint32 cb_id
)
{
  static uint32 oncrpc_cb_search_idx = 0;
  uint32 idx;
  uint32 cnt;
  rpc_cb_data_type *cb_data;

  if( cb_id == (uint32)RPC_CALLBACK_IS_NULL ) {
    return NULL;
  }

  oncrpc_crit_sect_enter(rpc_svc_cb_registry_crit_section);

  /* Look through table for entry with the same 
   * cb_handler/cb_id/msg source/protocol 
   * combination 
   */
  for( idx = 0; idx < RPC_MAX_NUM_CALLBACKS; idx++ ) {
    cb_data = &rpc_svc_cb_registry[idx];
    if( cb_data->cb_handler == cb_handler &&
        cb_data->cb_id == cb_id &&
        cb_data->cb_source == clnt->msg_source &&
        cb_data->protocol == clnt->protocol ) 
    {
      if( clnt->protocol == ONCRPC_SM_PROTOCOL ) {
        break;
      }
      else if( clnt->protocol == ONCRPC_RTR_PROTOCOL ) {
        break;
      }
      // TODO - add code for other protocols
      else {
        ERR_FATAL( "Callback on protocol %d unimplemented",
          clnt->protocol, 0, 0 );
      }
    }
  }

  if( idx == RPC_MAX_NUM_CALLBACKS ) {
    /* cb_id/protocol combination not found */
    idx = oncrpc_cb_search_idx;
    for( cnt = 0; cnt < RPC_MAX_NUM_CALLBACKS; cnt++ )
    {
      if( ( rpc_svc_cb_registry[idx].cb_handler == RPC_CALLBACK_ENTRY_UNASSIGNED )  ||
          ( rpc_svc_cb_registry[idx].cb_handler == rpc_svc_default_cb_handler ) )         
      {
        break;
      }

      if( ++idx == RPC_MAX_NUM_CALLBACKS ) {
        idx = 0;
      }
    }

    if( cnt == RPC_MAX_NUM_CALLBACKS ) {
      ERR_FATAL( "RPC svc callback registry overflow", 0, 0, 0 );
    }

    /* Fill in new entry fields */
    oncrpc_cb_search_idx = ((idx + 1) == RPC_MAX_NUM_CALLBACKS) ? 0 : idx + 1;

    cb_data = &rpc_svc_cb_registry[idx];
    cb_data->cb_handler = cb_handler;
    cb_data->cb_id = cb_id;
    cb_data->protocol = clnt->protocol;
    cb_data->cb_source = clnt->msg_source;
    // TODO - set additional data fields depending on the protocol
  }

  oncrpc_crit_sect_leave(rpc_svc_cb_registry_crit_section);

  /* Return cb bridge function associated with this table entry */
  return oncrpc_cb_bridge_table[idx];
} /* rpc_svc_callback_register */

/*===========================================================================
  FUNCTION: rpc_svc_callback_deregister

  DESCRIPTION:
      Remove all registrations associated with the source for given protocol.

  RESULT:
      None
  ===========================================================================*/
void rpc_svc_callback_deregister
  (
  oncrpc_addr_type cb_source,
  rpcprot_t protocol
)
{
  uint32 idx;
  rpc_cb_data_type *cb_data;

  oncrpc_crit_sect_enter(rpc_svc_cb_registry_crit_section);

  for( idx = 0; idx < RPC_MAX_NUM_CALLBACKS; idx++ ) {
    cb_data = &rpc_svc_cb_registry[idx];
    if( cb_data->cb_source == cb_source && cb_data->protocol == protocol ) 
    {
      memset(cb_data, 0, sizeof(rpc_cb_data_type ));
      cb_data->cb_handler = (void *)rpc_svc_default_cb_handler;
    }
  }

  oncrpc_crit_sect_leave(rpc_svc_cb_registry_crit_section);
} /* rpc_svc_callback_deregister */

/*===========================================================================
  FUNCTION: rpc_svc_cb_data_lookup

  DESCRIPTION:
      Using the current tcb, lookup the callback data associated with the
      current callback.

  RESULT:
      Pointer to callback data associated with the current callback.
  ===========================================================================*/
rpc_cb_data_type *rpc_svc_cb_data_lookup( void )
{
  oncrpc_tls_type  *tls = oncrpc_tls_get_self();
  rpc_cb_data_type *cb_data;

  cb_data = tls->cb_data;
  tls->cb_data = NULL;

  if( cb_data == NULL ) {
    ERR_FATAL( "Invalid cb_data for thread %x", (uint32) tls, 0, 0 );
  }

  return cb_data;
} /* rpc_svc_cb_data_lookup */

/*===========================================================================
  FUNCTION: rpc_svc_callback_lookup

  DESCRIPTION:
      Save a pointer to the callback data associated with the given index
      in the tls and return a pointer to the cb handler for this callback.

  RESULT:
      Pointer to cb handler associated with the current callback.
  ===========================================================================*/
void *rpc_svc_callback_lookup( uint32 idx )
{
  oncrpc_tls_type *tls = oncrpc_tls_get_self();

  ASSERT( idx < RPC_MAX_NUM_CALLBACKS );

  if( tls->cb_data != NULL ) {
    ERR_FATAL( "cb_data %x already in use for this thread %x",
      (uint32) tls->cb_data, (uint32) tls, 0 );
  }

  tls->cb_data = &rpc_svc_cb_registry[idx];

  /* Lint does not understand the above assert which protects for out-of-bounds*/
  /*lint -e661 possible out-of-bounds access */
  if( rpc_svc_cb_registry[idx].cb_handler == NULL )
  {
    ERR( "cb registry entry cleared entry %x task %x",idx, (uint32) tls, 0 );

    return (void *)rpc_svc_default_cb_handler;
  }

  return rpc_svc_cb_registry[idx].cb_handler;
  /*lint +e661*/
} /* rpc_svc_callback_lookup */

