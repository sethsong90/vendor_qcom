/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        O N C R P C _ C L N T . H

GENERAL DESCRIPTION

  This is the header file that defines the client related structures
  and handling functions.

  ONCRPC (Open Network Computing Remote Procedure Calls) is a way of
  calling functions from one machine to another.  This is described in
  detail in the following RFCs: 1831, 1832, 1833

INITIALIZATION AND SEQUENCING REQUIREMENTS

 Copyright (c) 2005-2008 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential.
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/inc/oncrpc/oncrpc_clnt.h#1 $ 

when        who    what, where, why
--------    ---    ----------------------------------------------------------
07/17/08    rr     Add Android supports
07/10/07    ptm    Remove featurization.
08/22/06    ptm    Added comment.
03/03/06    ptm    Add featurization around TCP references
01/25/06    ptm    Added clntsm_reply_create.
07/13/05    clp    Added clnt_destroy.
06/04/05    ptm    Add block comments.
05/27/05    ptm    Add definition for ONCRPC_RETRY_TIME.
04/01/05    clp    Include header cleanup changes.
03/16/05    clp    Added header to source file.
===========================================================================*/


/* Parts of this file are derived from the original Sun (ONC) RPC
 * code, under the following copyright:
 */
/* @(#)clnt.h 2.1 88/07/29 4.0 RPCSRC; from 1.31 88/02/08 SMI*/
/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * clnt.h - Client side remote procedure call interface.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#ifndef _ONCRPC_CLNT_H
#define _ONCRPC_CLNT_H 1

#ifdef FEATURE_ANDROID
#include "time.h"
#endif
#ifdef FEATURE_ONCRPC_TCP
#include "dssocket.h"
#endif  /* FEATURE_ONCRPC_TCP */

/*
 * Rpc calls return an enum clnt_stat.  This should be looked at more,
 * since each implementation is required to live with this (implementation
 * independent) list of errors.
 */
enum clnt_stat {
  RPC_SUCCESS=0,      /* call succeeded */
  /*
   * local errors
   */
  RPC_CANTENCODEARGS=1,    /* can't encode arguments */
  RPC_CANTDECODERES=2,    /* can't decode results */
  RPC_CANTSEND=3,      /* failure in sending call */
  RPC_CANTRECV=4,      /* failure in receiving result */
  RPC_TIMEDOUT=5,      /* call timed out */
  /*
   * remote errors
   */
  RPC_VERSMISMATCH=6,    /* rpc versions not compatible */
  RPC_AUTHERROR=7,    /* authentication error */
  RPC_PROGUNAVAIL=8,    /* program not available */
  RPC_PROGVERSMISMATCH=9,    /* program version mismatched */
  RPC_PROCUNAVAIL=10,    /* procedure unavailable */
  RPC_CANTDECODEARGS=11,    /* decode arguments error */
  RPC_SYSTEMERROR=12,    /* generic "other problem" */
  RPC_NOBROADCAST = 21,    /* Broadcasting not supported */
  /*
   * callrpc & clnt_create errors
   */
  RPC_UNKNOWNHOST=13,    /* unknown host name */
  RPC_UNKNOWNPROTO=17,    /* unknown protocol */
  RPC_UNKNOWNADDR = 19,    /* Remote address unknown */

  /*
   * rpcbind errors
   */
  RPC_RPCBFAILURE=14,    /* portmapper failed in its call */
#define RPC_PMAPFAILURE RPC_RPCBFAILURE
  RPC_PROGNOTREGISTERED=15,  /* remote program is not registered */
  RPC_N2AXLATEFAILURE = 22,  /* Name to addr translation failed */
  /*
   * unspecified error
   */
  RPC_FAILED=16,
  RPC_INTR=18,
  RPC_TLIERROR=20,
  RPC_UDERROR=23,
  /*
   * asynchronous errors
   */
  RPC_INPROGRESS = 24,
  RPC_STALERACHANDLE = 25
};

typedef struct xdr_struct CLIENT;
/* client call callback. 
 * Callback called when the reply is recieved or there is an error in
 * getting reply.
 */
typedef void (*clnt_call_cb)
( 
  CLIENT * clnt, 
  void * cookie, 
  caddr_t results, 
  rpc_reply_header error
);

typedef void (*clnt_call_non_blocking_cb)
( 
  CLIENT * clnt, 
  void * cookie, 
  caddr_t results, 
  rpc_reply_header error
);

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
extern enum clnt_stat 
clnt_call
( 
  CLIENT *h, 
  u_long proc,
  xdrproc_t xdr_args,
  caddr_t args_ptr,
  xdrproc_t xdr_results, 
  caddr_t rets_ptr,
  struct timeval timeout
);
/*===========================================================================
FUNCTION CLNT_CALL_NON_BLOCKING

DESCRIPTION
  RPCGEN support routine. This routine is called by client routines generated
  by RPCGEN. It generates and sends an RPC message to a server.

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
extern enum clnt_stat 
clnt_call_non_blocking
( 
  CLIENT *h,
  u_long proc,
  xdrproc_t xdr_args,
  caddr_t args_ptr,
  xdrproc_t xdr_results,
  int results_size,
  clnt_call_cb result_cb,
  void * cb_data
);

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

SIDE EFFECTS
  None.
===========================================================================*/
extern bool_t clnt_freeres( CLIENT *xdr, xdrproc_t xdr_res, caddr_t res_ptr );

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
extern void clnt_destroy( CLIENT *xdr );

/* Macros for CLNT functionality */
#define CLNT_DESTROY(xdr) clnt_destroy(xdr)
#define CLNT_CONTROL(xdr, req, info) XDR_CONTROL(xdr, req, info)
#define CLNT_ABORT(xdr) XDR_MSG_ABORT(xdr)
#define CLNT_FREERES(xdr,res,ptr) clnt_freeres(xdr,res,ptr)
/*
 * control operations that apply to all transports
 *
 * Note: options marked XXX are no-ops in this implementation of RPC.
 * The are present in TI-RPC but can't be implemented here since they
 * depend on the presence of STREAMS/TLI, which we don't have.
 */
#define CLSET_TIMEOUT        1    /* set timeout (timeval) */
#define CLGET_TIMEOUT        2    /* get timeout (timeval) */
#define CLGET_SERVER_ADDR    3    /* get server's address (sockaddr) */
#define CLGET_FD             6    /* get connections file descriptor */
#define CLGET_SVC_ADDR       7    /* get server's address (netbuf)      XXX */
#define CLSET_FD_CLOSE       8    /* close fd while clnt_destroy */
#define CLSET_FD_NCLOSE      9    /* Do not close fd while clnt_destroy*/
#define CLGET_XID            10   /* Get xid */
#define CLSET_XID            11   /* Set xid */
#define CLGET_VERS           12   /* Get version number */
#define CLSET_VERS           13   /* Set version number */
#define CLGET_PROG           14   /* Get program number */
#define CLSET_PROG           15   /* Set program number */
#define CLSET_SVC_ADDR       16   /* get server's address (netbuf)      XXX */
#define CLSET_PUSH_TIMOD     17   /* push timod if not already present  XXX */
#define CLSET_POP_TIMOD      18   /* pop timod                          XXX */
/*
 * Connectionless only control operations
 */
#define CLSET_RETRY_TIMEOUT 4 /* set retry timeout (timeval) */
#define CLGET_RETRY_TIMEOUT 5 /* get retry timeout (timeval) */

/*
 * Below are the client handle creation routines for the various
 * implementations of client side rpc.  They can return NULL if a
 * creation failure occurs.
 */

extern CLIENT * clnt_create ( char * host, uint32 prog, uint32 vers,
                              char * proto);

extern CLIENT *clntsio_create ( uint32 event );
extern CLIENT *clntsio_create_dedicated( uint32 prog, uint32 vers,
                                         uint32 event );

extern CLIENT *clntsm_create( uint32 event );
extern CLIENT *clntsm_create_dedicated( uint32 prog, uint32 vers,
                                        uint32 event );

extern CLIENT *clntsm_reply_create( uint32 event );

extern CLIENT *clntrtr_create( uint32 event );
extern CLIENT *clntrtr_create_dedicated ( uint32 prog, uint32 vers, 
                                          uint32 event );

extern CLIENT *clntlo_create( uint32 event );
extern CLIENT *clntlo_create_dedicated ( uint32 prog, uint32 vers, 
                                          uint32 event );

#ifdef FEATURE_ONCRPC_TCP
typedef void (*clnttcp_cb)(void * cb_data, CLIENT * clnt, sint15 dss_errno);

extern CLIENT * clnttcp_create( struct sockaddr  *raddr,
                                uint32            raddr_len,
                                sint15            sock,
                                u_int             sendsize, 
                                u_int             recvsize,
                                clnttcp_cb        cb_func,
                                void             *cb_data,
                                uint32            event);
extern CLIENT * clnttcp_create_dedicated ( struct sockaddr  *raddr,
                                           uint32            raddr_len, 
                                           uint32            prog,
                                           uint32            vers, 
                                           sint15            sock,
                                           u_int             sendsize, 
                                           u_int             recvsize,
                                           clnttcp_cb        cb_func, 
                                           void             *cb_data, 
                                           uint32            event);
#endif /* FEATURE_ONCRPC_TCP */

#define UDPMSGSIZE 8800 /* rpc imposed limit on udp msg size */
#define RPCSMALLMSGSIZE 400 /* a more reasonable packet size */

#define ONCRPC_RETRY_TIME        20 /* Time (in msec) between retries */

#endif /* _ONCRPC_CLNT_H */
