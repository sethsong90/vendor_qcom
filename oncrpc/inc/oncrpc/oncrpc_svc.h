/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                      O N C R P C _ S V C . H

GENERAL DESCRIPTION
  ONC (Open Network Computing) RPC (Remote Procedure Calls) server header file.

EXTERNALIZED FUNCTIONS
  to be filled in ...
    
INITIALIZATION AND SEQUENCING REQUIREMENTS
  to be filled in ...

 Copyright (c) 2004-2008 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential.
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                             EDIT HISTORY FOR MODULE

  $Header: //linux/pkgs/proprietary/oncrpc/main/source/inc/oncrpc/oncrpc_svc.h#2 $ $DateTime: 2008/09/26 10:58:17 $ $Author: rruigrok $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
04/23/09    rr     Add missing prototype svc_register_auto
09/25/08    rr     Add support for backwards compatibility registration
                   svc_register_auto_apiversions
07/10/07    ptm    Remove featurization.
06/01/07    hn     Added prototype svc_register_with_plugger.
10/03/06    hn     Added support for locking/unlocking RPC services.
10/13/05    clp    Add prototypes for udp and tcp init routines.
04/01/05    clp    Include header cleanup changes.
03/22/05    hn     Moved svcerr_...() routines out of this file and into
                   oncrpcsvc_err.h
01/28/05    ptm    Move svc_register_with_id to oncrpc_proxy.h (and change
                   name to svc_register_with_proxy) to resolve circular header
                   file problem.
01/28/05    ptm    First release with edit history

===========================================================================*/

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
 * svc.h, Server-side remote procedure call interface.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#ifndef _RPC_SVC_H
#define _RPC_SVC_H 1


__BEGIN_DECLS

/*
 * This interface must manage two items concerning remote procedure calling:
 *
 * 1) An arbitrary number of transport connections upon which rpc requests
 * are received.  The two most notable transports are TCP and UDP;  they are
 * created and registered by routines in svc_tcp.c and svc_udp.c, respectively;
 * they in turn call xprt_register and xprt_unregister.
 *
 * 2) An arbitrary number of locally registered services.  Services are
 * described by the following four data: program number, version number,
 * "service dispatch" function, a transport handle, and a boolean that
 * indicates whether or not the exported program should be registered with a
 * local binder service;  if true the program's number and version and the
 * port number from the transport handle are registered with the binder.
 * These data are registered with the rpc svc system via svc_register.
 *
 * A service's dispatch function is called whenever an rpc request comes in
 * on a transport.  The request's program and version numbers must match
 * those of the registered service.  The dispatch function is passed two
 * parameters, struct svc_req * and SVCXPRT *, defined below.
 */

/*
 * Server side transport handle
 */
typedef struct xdr_struct SVCXPRT;

/*
 * Service request
 */
struct svc_req {
  rpcprog_t rq_prog;            /* service program number */
  rpcvers_t rq_vers;            /* service protocol version */
  rpcproc_t rq_proc;            /* the desired procedure */
  SVCXPRT *rq_xprt;             /* associated transport */
};

#ifndef __DISPATCH_FN_T
#define __DISPATCH_FN_T
typedef void (*__dispatch_fn_t) (struct svc_req*, SVCXPRT*);
#endif

/*
 * Service registration (registers only with plugger module)
 *
 * svc_register_with_plugger(xprt, prog, vers, dispatch, protocol)
 *	SVCXPRT *xprt;
 *	rpcprog_t prog;
 *	rpcvers_t vers;
 *	void (*dispatch)(struct svc_req*, SVCXPRT*);
 *	rpcprot_t protocol;  like TCP or UDP, zero means do not register
 */
extern bool_t svc_register_with_plugger (SVCXPRT *__xprt, rpcprog_t __prog,
                                         rpcvers_t __vers,
                                         __dispatch_fn_t __dispatch,
                                         rpcprot_t __protocol) __THROW;

/*
 * Service registration (registers with plugger module and lower layers)
 *
 * svc_register(xprt, prog, vers, dispatch, protocol)
 *	SVCXPRT *xprt;
 *	rpcprog_t prog;
 *	rpcvers_t vers;
 *	void (*dispatch)(struct svc_req*, SVCXPRT*);
 *	rpcprot_t protocol;  like TCP or UDP, zero means do not register
 */
#define svc_register qc_svc_register
extern bool_t qc_svc_register (SVCXPRT *__xprt, rpcprog_t __prog,
			    rpcvers_t __vers, __dispatch_fn_t __dispatch,
			    rpcprot_t __protocol) __THROW;



extern bool_t
svc_register_auto_apiversions
(
  rpcprog_t    prog, 
  rpcvers_t    vers,
  void       (*dispatch) (struct svc_req *, SVCXPRT *),
  uint32     *(*api_versions) (uint32 *size_entries)
);

bool_t
svc_register_auto
(
  rpcprog_t    prog,
  rpcvers_t    vers,
  void       (*dispatch) (struct svc_req *, SVCXPRT *)
);
/*
 * Service un-registration
 *
 * svc_unregister(prog, vers)
 *	rpcprog_t prog;
 *	rpcvers_t vers;
 */
void
svc_unregister 
(
  rpcprog_t prog, 
  rpcvers_t vers,
  rpcprot_t prot
);

/*
 * Service Enable
 *
 * svc_enable( prog, vers )
 *	rpcprog_t prog;
 *	rpcvers_t vers;
 */
#define svc_enable(prog, vers) svc_lock(prog, vers, FALSE)

/*
 * Service Disable
 *
 * svc_disable( prog, vers )
 *	rpcprog_t prog;
 *	rpcvers_t vers;
 */
#define svc_disable(prog, vers) svc_lock(prog, vers, TRUE)

extern void svc_lock(rpcprog_t __prog, rpcvers_t __vers, bool_t __lock) __THROW;

/*
 * When the service routine is called, it must first check to see if it
 * knows about the procedure;  if not, it should call svcerr_noproc
 * and return.  If so, it should deserialize its arguments via
 * SVC_GETARGS (defined above).  If the deserialization does not work,
 * svcerr_decode should be called followed by a return.  Successful
 * decoding of the arguments should be followed the execution of the
 * procedure's code and a call to svc_sendreply.
 *
 * Also, if the service refuses to execute the procedure due to too-
 * weak authentication parameters, svcerr_weakauth should be called.
 * Note: do not confuse access-control failure with weak authentication!
 *
 * NB: In pure implementations of rpc, the caller always waits for a reply
 * msg.  This message is sent when svc_sendreply is called.
 * Therefore pure service implementations should always call
 * svc_sendreply even if the function logically returns void;  use
 * xdr.h - xdr_void for the xdr routine.  HOWEVER, tcp based rpc allows
 * for the abuse of pure rpc via batched calling or pipelining.  In the
 * case of a batched call, svc_sendreply should NOT be called since
 * this would send a return message, which is what batching tries to avoid.
 * It is the service/protocol writer's responsibility to know which calls are
 * batched and which are not.  Warning: responding to batch calls may
 * deadlock the caller and server processes!
 */

extern boolean svc_getargs(SVCXPRT *xdr, xdrproc_t xdr_args, caddr_t args_ptr);
extern boolean svc_freeargs(SVCXPRT *xdr, xdrproc_t xdr_args, caddr_t args_ptr);

extern bool_t	svc_sendreply (SVCXPRT *xprt, xdrproc_t __xdr_results,
			       caddr_t __xdr_location) __THROW;


/*
 * Socket to use on svcxxx_create call to get default socket
 */
#define	RPC_ANYSOCK	-1

/*
 * These are the existing service side transport implementations
 */

/*
 * Memory based rpc for testing and timing.
 */
extern SVCXPRT *svcraw_create (void) __THROW;

/*
 * Udp based rpc.
 */
extern SVCXPRT *svcudp_create (int __sock) __THROW;
extern SVCXPRT *svcudp_bufcreate (int __sock, u_int __sendsz, u_int __recvsz)
     __THROW;
extern void xprtudp_init( void ) __THROW;

/*
 * Tcp based rpc.
 */
extern SVCXPRT *svctcp_create (int __sock, u_int __sendsize, u_int __recvsize)
     __THROW;
extern void xprttcp_init( void ) __THROW;


/*
 * Unix based rpc.
 */
extern SVCXPRT *svcunix_create (int __sock, u_int __sendsize, u_int __recvsize,
				char *__path) __THROW;

/*
 * File base rpc.
 */
extern SVCXPRT *svcfd_create (int __sock, u_int __sendsize, u_int __recvsize)
     __THROW;

/* 
 * SIO based rpc.
 */
extern SVCXPRT *svcsio_create (u_int __sendsize, u_int __recvsize) __THROW;

extern SVCXPRT *svcsm_create (u_int __sendsize, u_int __recvsize) __THROW;

/* 
 * Router based rpc.
 */
extern SVCXPRT *svcrtr_create (u_int __sendsize, u_int __recvsize) __THROW;

/* 
 * Test Loopback based rpc.
 */
extern SVCXPRT *svclo_create (u_int __sendsize, u_int __recvsize) __THROW;
 
/* 
 * DIAG based rpc.
 */

extern SVCXPRT *svcdiag_create (u_int __sendsize, u_int __recvsize) __THROW;
extern void     svcdiag_init( void ) __THROW;

extern SVCXPRT *svcrtr_create (u_int sendsize, u_int recvsize) __THROW;

__END_DECLS


#endif /* _RPC_SVC_H */
