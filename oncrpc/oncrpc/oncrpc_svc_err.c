/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                      O N C R P C _ S V C _ E R R . C

GENERAL DESCRIPTION
  ONC (Open Network Computing) RPC (Remote Procedure Calls) server-side
  error handling implementation.

EXTERNALIZED FUNCTIONS
  svcerr_noproc
  svcerr_decode
  svcerr_systemerr
  svcerr_auth
  svcerr_weakauth
  svcerr_noprog
  svcerr_progvers
  svcerr_rpcvers

INITIALIZATION AND SEQUENCING REQUIREMENTS
  none.

 Copyright (c) 2005-2008, 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                             EDIT HISTORY FOR MODULE

  $Header: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_svc_err.c#6 $ $DateTime: 2008/09/26 10:58:17 $ $Author: rruigrok $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
09/10/07    hn     Added ASSERT(0) calls to ease debugging.
07/10/07    ptm    Remove featurization.
10/03/06    hn     Added support for locking/unlocking RPC services.
04/22/05    ptm    Update the XDR_MSG_SEND API.
04/12/05    ptm    Include oncrpci.h and remove err.h.
04/01/05    clp    Include header cleanup changes and moved xdr routines to
                   oncrpcxdr.c.
03/022/05   hn     First revision of this file.


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

/*===========================================================================

                            INCLUDE FILES FOR MODULE

===========================================================================*/


#include "oncrpc.h"
#include "oncrpci.h"

#define SVCERR_XDR_SEND(xdr, reply) \
        ( XDR_MSG_START(xdr, RPC_MSG_REPLY) && \
          xdr_send_reply_header(xdr, &reply) && \
          XDR_MSG_SEND(xdr, NULL) )


/*===========================================================================

          Local Function Definitions

============================================================================*/


/*===========================================================================

          Externalized Function Definitions

============================================================================*/


/*===========================================================================
FUNCTION: svcerr_default_err

DESCRIPTION: Handling of default error case when procedure number 
             is not found.

ARGUMENTS: 
   The XDR transport structure for the server side of the RPC connection.
   Service request handle.
   Version Table.

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void svcerr_default_err
(
  xdr_s_type *svr,
  struct svc_req *rqstp,
  uint32   *(*ver_table_func) (uint32 *size_entries)
)
{
  uint32 *version_table;
  uint32 v_table_size;
  uint32 i;
  uint32 version_found=0;
  version_table = ver_table_func(& v_table_size);
  for(i=0;i<v_table_size;i++)
  {
    if(rqstp->rq_vers == version_table[i])
    {
      version_found=1;
      svcerr_noproc( svr );
      break;
    }
  }
  if(version_found==0)
  {
    svcerr_progvers(svr,(u_long)version_table[0],(u_long)version_table[v_table_size]);
  }
}
  




/*===========================================================================
FUNCTION: svcerr_noproc

DESCRIPTION: Sends an error reply indicating that the RPC program does not
             support the requested procedure.

ARGUMENTS: 
   The XDR transport structure for the server side of the RPC connection.

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void svcerr_noproc
(
  xdr_s_type  *svc
)
{
  rpc_reply_header reply;

  reply.stat = RPC_MSG_ACCEPTED;
  reply.u.ar.verf = svc->verf;
  reply.u.ar.stat = RPC_PROC_UNAVAIL;

  if ( ! SVCERR_XDR_SEND(svc, reply) )
  {
    /* Couldn't send the reply - just give up */
    XDR_MSG_ABORT(svc);
    ERR( "svcerr_noproc: unable to send message", 0, 0, 0 );
  }
} /* svcerr_noproc */

/*===========================================================================
FUNCTION: svcerr_decode

DESCRIPTION: Sends an error reply indicating that the server can't decode the
             arguments.

ARGUMENTS: 
   The XDR transport structure for the server side of the RPC connection.

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void svcerr_decode
(
  xdr_s_type  *svc
)
{
  rpc_reply_header reply;

  /* Makes debugging easier */
  ASSERT(0);

  reply.stat = RPC_MSG_ACCEPTED;
  reply.u.ar.verf = svc->verf;
  reply.u.ar.stat = RPC_GARBAGE_ARGS;

  if ( ! SVCERR_XDR_SEND(svc, reply) )
  {
    /* Couldn't send the reply - just give up */
    XDR_MSG_ABORT(svc);
    ERR( "svcerr_decode: unable to send message", 0, 0, 0 );
  }

} /* svcerr_decode */

/*===========================================================================
FUNCTION: svcerr_systemerr

DESCRIPTION: Sends an error reply indicating that the RPC call failed due to
             a system error on the server side. (e.g. memory allocation
             failures etc.)

ARGUMENTS: 
   The XDR transport structure for the server side of the RPC connection.

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void svcerr_systemerr
(
  xdr_s_type  *svc
)
{
  rpc_reply_header reply;

  ONCRPC_SYSTEM_LOGE("%s: Prog: %08x, Ver: %08x, Proc: %08x Xid: %08x\n",
                     __func__, svc->x_prog, svc->x_vers, svc->x_proc, svc->xid);
  /* Makes debugging easier */
  ASSERT(0);

  reply.stat = RPC_MSG_ACCEPTED;
  reply.u.ar.verf = svc->verf;
  reply.u.ar.stat = RPC_SYSTEM_ERR;

  if ( ! SVCERR_XDR_SEND(svc, reply) )
  {
    /* Couldn't send the reply - just give up */
    XDR_MSG_ABORT(svc);
    ERR( "svcerr_systemerr: unable to send message", 0, 0, 0 );
  }

} /* svcerr_systemerr */

/*===========================================================================
FUNCTION: svcerr_auth

DESCRIPTION: Sends an error reply indicating that the RPC call failed
             authentication on the server side. The reply contains a field
             that specifies why the authentication failed.

ARGUMENTS: 
   svc       The XDR transport structure for the server side of the RPC
             connection.
   why       Reason for failure of authentication.


RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void svcerr_auth
(
  xdr_s_type	  *svc,
  oncrpc_auth_stat   why
)
{
  rpc_reply_header reply;

  reply.stat = RPC_MSG_DENIED;
  reply.u.dr.stat = RPC_AUTH_ERROR;
  reply.u.dr.u.why = why;

  if ( ! SVCERR_XDR_SEND(svc, reply) )
  {
    /* Couldn't send the reply - just give up */
    XDR_MSG_ABORT(svc);
    ERR( "svcerr_auth: unable to send message", 0, 0, 0 );
  }
 
} /* svcerr_auth */

/*===========================================================================
FUNCTION: svcerr_weakauth

DESCRIPTION: Sends an error reply indicating that the RPC call failed because
             the call's authentication was too weak.

ARGUMENTS: 
   The XDR transport structure for the server side of the RPC connection.

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void svcerr_weakauth
(
  xdr_s_type  *svc
)
{
  svcerr_auth (svc, ONCRPC_AUTH_TOOWEAK);
} /* svcerr_weakauth */

/*===========================================================================
FUNCTION: svcerr_noprog

DESCRIPTION: Sends an error reply indicating that the requeseted RPC Program
             is not exported by the server side.

ARGUMENTS: 
   The XDR transport structure for the server side of the RPC connection.

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void svcerr_noprog
(
  xdr_s_type  *svc
)
{
  rpc_reply_header reply;

  reply.stat = RPC_MSG_ACCEPTED;
  reply.u.ar.verf = svc->verf;
  reply.u.ar.stat = RPC_PROG_UNAVAIL;

  if ( ! SVCERR_XDR_SEND(svc, reply) )
  {
    /* Couldn't send the reply - just give up */
    XDR_MSG_ABORT(svc);
    ERR( "svcerr_noprog: unable to send message", 0, 0, 0 );
  }

} /* svcerr_noprog */

/*===========================================================================
FUNCTION: svcerr_proglocked

DESCRIPTION: Sends an error reply indicating that the requeseted RPC Program
             is locked on the server side.

ARGUMENTS: 
   The XDR transport structure for the server side of the RPC connection.

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void svcerr_proglocked
(
  xdr_s_type  *svc
)
{
  rpc_reply_header reply;

  reply.stat = RPC_MSG_ACCEPTED;
  reply.u.ar.verf = svc->verf;
  reply.u.ar.stat = RPC_PROG_LOCKED;

  if ( ! SVCERR_XDR_SEND(svc, reply) )
  {
    /* Couldn't send the reply - just give up */
    XDR_MSG_ABORT(svc);
    ERR( "svcerr_proglocked: unable to send message", 0, 0, 0 );
  }

} /* svcerr_proglocked */

/*===========================================================================
FUNCTION: svcerr_progvers

DESCRIPTION: Sends an error reply indicating that the requested version of
             the RPC Program is not supported. Part of the reply will
             specify the lowest and highest versions supported of the
             program.

ARGUMENTS: 
   svc       The XDR transport structure for the server side of the RPC
             connection.
   low_vers  Lowest version number of the program that is supported by the
             server.
   high_vers Highest version number of the program that is supported by the
             server.

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void svcerr_progvers
(
  xdr_s_type  *svc,
  rpcvers_t    low_vers,
  rpcvers_t    high_vers
)
{
  rpc_reply_header reply;

  reply.stat = RPC_MSG_ACCEPTED;
  reply.u.ar.verf = svc->verf;
  reply.u.ar.stat = RPC_PROG_MISMATCH;
  reply.u.ar.u.versions.low = low_vers;
  reply.u.ar.u.versions.high = high_vers;

  if ( ! SVCERR_XDR_SEND(svc, reply) )
  {
    /* Couldn't send the reply - just give up */
    XDR_MSG_ABORT(svc);
    ERR( "svcerr_progvers: unable to send message", 0, 0, 0 );
  }

} /* svcerr_progvers */

/*===========================================================================
FUNCTION: svcerr_rpcvers

DESCRIPTION: Sends an error reply indicating that the version of RPC used to
             make the call on the client side is not supported by the server
             side. Part of the reply will specify the lowest and highest
             versions of RPC that are supported.

ARGUMENTS: 
   svc       The XDR transport structure for the server side of the RPC
             connection.
   low       Lowest version of RPC that is supported by the server side.
   high      Highest version of RPC that is supported by the server side.

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void svcerr_rpcvers
(
  xdr_s_type  *svc,
  uint32       low,
  uint32       high
)
{
  rpc_reply_header reply;

  reply.stat = RPC_MSG_DENIED;
  reply.u.dr.stat = RPC_MISMATCH;
  reply.u.dr.u.versions.low = low;
  reply.u.dr.u.versions.high = high;
  
  if ( ! SVCERR_XDR_SEND(svc, reply) )
  {
    /* Couldn't send the reply - just give up */
    XDR_MSG_ABORT(svc);
    ERR( "svcerr_rpcvers: unable to send message", 0, 0, 0 );
  }

} /* svcerr_rpcvers */

