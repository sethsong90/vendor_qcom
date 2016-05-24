#ifndef ONCRPC_CB_H
#define ONCRPC_CB_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        O N C R P C _ C B _ U T I L . C

GENERAL DESCRIPTION

  This file provides the utility routines for a callback mechinism in
  ONCRPC.

  ONCRPC (Open Network Computing Remote Procedure Calls) is a way of
  calling functions from one machine to another.  This is described in
  detail in the following RFCs: 1831, 1832, 1833

INITIALIZATION AND SEQUENCING REQUIREMENTS

 Copyright (c) 2005, 2007-2008 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/inc/oncrpc/oncrpc_cb.h#1 $ 

when        who    what, where, why
--------    ---    ----------------------------------------------------------
01/21/08    hn     Added oncrpc_setup_call routine.
10/17/07    ptm    Add normal header file #ifndef protection and include
                   rpc-cb-data-type.
10/17/07    hn     tls_get and tls_delete now take a thread's key, added
                   tls_get_self and tls_delete_self as the no args version
08/20/07    ptm    Added oncrpc thread exit routine.
05/09/07    hn     Added timeout parameter to rpc_clnt_lookup2.
05/04/07    hn     Added rpc_clnt_lookup2.
03/23/07    hn     Added initialization routine for callback registry.
09/13/05    clp    Split code from oncrpcxdr.h
===========================================================================*/

/*============================================================================
             TYPE DEFINITIONS
============================================================================*/
#ifndef RPC_CLNT_LOOKUP_TIMEOUT
#define RPC_CLNT_LOOKUP_TIMEOUT (uint32)ONCRPC_INFINITY
#endif /* ! RPC_CLNT_LOOKUP_TIMEOUT */

typedef struct rpc_cb_data_type {
  void                     * cb_handler;
  uint32                     cb_id;
  rpcprot_t                  protocol;
  oncrpc_addr_type           cb_source;
} rpc_cb_data_type;

/*===========================================================================
            FUNCTIONAL INTERFACE
===========================================================================*/
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
void oncrpc_thread_exit( void * key );

/*===========================================================================
  FUNCTION: oncrpc_setup_call

  DESCRIPTION:
  Sets up an RPC based on the initialized client XDR and a time out value.

  DEPENDENCIES:
  The "x_prog" and "x_vers" fields of the clnt XDR must have been initialized.

  RESULT:
  TRUE if RPC was setup successfully. FALSE if an error occured.
  ===========================================================================*/
boolean oncrpc_setup_call( xdr_s_type *clnt, uint32 timeout );

/*===========================================================================
  FUNCTION: rpc_clnt_register

  DESCRIPTION:
  Register the client based on the current task.

  RESULT:
  TRUE if client was registered, FALSE if there was an error
  ===========================================================================*/
extern boolean rpc_clnt_register( xdr_s_type *clnt );

/*===========================================================================
  FUNCTION: rpc_clnt_deregister

  DESCRIPTION:
  Deregister the client

  RESULT:
  TRUE if client was deregistered, FALSE if there was an error
  ===========================================================================*/
extern boolean rpc_clnt_deregister( void );

/*===========================================================================
  FUNCTION: rpc_clnt_lookup2

  DESCRIPTION:
  Lookup a client based on the current task and the RPC program it wants to
  talk to.

  RESULT:
  pointer to XPORT structure
  ===========================================================================*/
extern
 xdr_s_type *rpc_clnt_lookup2( rpcprog_t prog, rpcvers_t ver, uint32 timeout );

/*===========================================================================
  FUNCTION: rpc_clnt_lookup (obsolete)

  DESCRIPTION:
  Lookup a client based on the current task.

  RESULT:
  pointer to XPORT structure
  ===========================================================================*/
extern xdr_s_type *rpc_clnt_lookup( void );

/*===========================================================================
  FUNCTION: rpc_init_callback_registry

  DESCRIPTION:
      Initializes data structures associated with the callback registry.

  RESULT:
      None.
  ===========================================================================*/
extern void rpc_init_callback_registry( void );

/*===========================================================================
  FUNCTION: rpc_clnt_callback_register

  DESCRIPTION:
      Register a callback function address and return an callback id which
      can be used to lookup the callback function address.

  RESULT:
      Callback ID
  ===========================================================================*/
extern uint32 rpc_clnt_callback_register( void *cb_func );

/*===========================================================================
  FUNCTION: rpc_clnt_callback_lookup

  DESCRIPTION:
      Lookup a callback function address based on a callback id.

  RESULT:
      Callback function address
  ===========================================================================*/
extern void *rpc_clnt_callback_lookup( uint32 cb_id );

/*===========================================================================
  FUNCTION: rpc_clnt_for_callback

  DESCRIPTION:
      Gets or creates a client for a particular protocol. This is used by
      callback support to get a client to send the callback message to.

  RESULT:
      Pointer to client
  ===========================================================================*/
extern xdr_s_type *rpc_clnt_for_callback( rpc_cb_data_type *cb_data );

/*===========================================================================
  FUNCTION: rpc_svc_callback_register

  DESCRIPTION:
      Register a callback handler function address, callback id and protocol.

  RESULT:
      Function pointer to callback bridge function associated with the
      table entry.
  ===========================================================================*/
extern void *rpc_svc_callback_register( void       *cb_handler,
                                        xdr_s_type *clnt,
                                        uint32      cb_id );

/*===========================================================================
  FUNCTION: rpc_svc_callback_deregister

  DESCRIPTION:
      Remove all registrations associated with the source for given protocol.

  RESULT:
      None
  ===========================================================================*/
void rpc_svc_callback_deregister( oncrpc_addr_type cb_source,
                                  rpcprot_t        protocol );

/*===========================================================================
  FUNCTION: rpc_svc_cb_data_lookup

  DESCRIPTION:
      Using the current tcb, lookup the callback data associated with the
      current callback.

  RESULT:
      Pointer to callback data associated with the current callback.
  ===========================================================================*/
extern rpc_cb_data_type *rpc_svc_cb_data_lookup( void );

/*===========================================================================
  FUNCTION: rpc_svc_callback_lookup

  DESCRIPTION:
      Save a pointer to the callback data associated with the given index
      in the tcb and return a pointer to the cb handler for this callback.

  RESULT:
      Pointer to cb handler associated with the current callback.
  ===========================================================================*/
extern void *rpc_svc_callback_lookup( uint32 idx );

#endif /* ONCRPC_CB_H */
