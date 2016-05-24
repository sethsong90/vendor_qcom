/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        O N C R P C _ S V C _ A U T H . C

GENERAL DESCRIPTION

  This file provides the server side authentiation routines.

  ONCRPC (Open Network Computing Remote Procedure Calls) is a way of
  calling functions from one machine to another.  This is described in
  detail in the following RFCs: 1831, 1832, 1833

INITIALIZATION AND SEQUENCING REQUIREMENTS

 Copyright (c) 2005, 2007-2008 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential.
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_svc_auth.c#4 $ 

when        who    what, where, why
--------    ---    ----------------------------------------------------------
02/05/08    rr     Fix potential dereferencing of null pointer
10/31/07    hn     Used enum xdr macros instead of functions.
07/10/07    ptm    Remove featurization.
04/12/05    ptm    Include oncrpci.h and remove err.h.
04/01/05    clp    Include header cleanup changes.
03/16/05    clp    Added header to source file.
===========================================================================*/


/* Parts of this file are derived from the original Sun (ONC) RPC
 * code, under the following copyright:
 */
/* @(#)svc_auth.c       2.4 88/08/15 4.0 RPCSRC */
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
 * svc_auth.c, Server-side rpc authenticator interface.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#include "comdef.h"
#include "oncrpc.h"
#include "oncrpci.h"
#include "oncrpc_svc_auth.h"

/*
 * svcauthsw is the bdevsw of server side authentication.
 *
 * Server side authenticators are called from authenticate by
 * using the client auth struct flavor field to index into svcauthsw.
 * The server auth flavors must implement a routine that looks
 * like:
 *
 *      enum auth_stat
 *      flavorx_auth(rqst, msg)
 *              register struct svc_req *rqst;
 *              register struct rpc_msg *msg;
 *
 */

/* no authentication */
static boolean
oncrpc_svcauth_null(xdr_s_type*, opaque_auth*);
/* unix style (uid, gids) */
static boolean
oncrpc_svcauth_unix(xdr_s_type*, opaque_auth*);
/* short hand unix style */
static boolean
oncrpc_svcauth_short(xdr_s_type*, opaque_auth*);

static const struct
  {
    boolean (*authenticator) (xdr_s_type*, opaque_auth*);
  }
svcauthsw[] =
{
  { oncrpc_svcauth_null },    /* AUTH_NULL */
  { oncrpc_svcauth_unix },     /* AUTH_UNIX */
  { oncrpc_svcauth_short }     /* AUTH_SHORT */
};
#define  AUTH_MAX  2  /* HIGHEST AUTH NUMBER */

static char * oncrpc_auth_short_base = "NONE";
#define ONCRPC_AUTH_SHORT_LEN 4
#define ONCRPC_AUTH_MAX_LEN MAX_AUTH_BYTES

boolean
oncrpc_xdr_send_auth
(
  xdr_s_type *xdr,
  const opaque_auth *auth
)
{
  return ( XDR_SEND_ENUM( xdr, &auth->oa_flavor ) &&
           XDR_SEND_UINT( xdr, &auth->oa_length ) &&
           ( auth->oa_length == 0 ||
             XDR_SEND_BYTES( xdr, auth->oa_base, auth->oa_length ) ) );
}

boolean xdr_recv_auth
(
  xdr_s_type *xdr,
  opaque_auth *auth
)
{
  ASSERT( auth->oa_base == NULL );

  if( !XDR_RECV_ENUM( xdr, &(auth->oa_flavor) ) ||
      !XDR_RECV_UINT( xdr, &(auth->oa_length) ) ) 
  {
    ERR( "xdr_recv_auth: unable to read auth flavor/length", 0, 0, 0 );
    return FALSE;
  }


  if ( ONCRPC_AUTH_MAX_LEN < auth->oa_length )
  { 
    ERR( "xdr_recv_auth: Got excessive length %d (max %d)",
         auth->oa_length, ONCRPC_AUTH_MAX_LEN, 0 );
    return FALSE;
  }

  if( auth->oa_length != 0 )
  {
    auth->oa_base = oncrpc_mem_alloc( auth->oa_length );
    if ( NULL != auth->oa_base )
    {
      if( !XDR_RECV_BYTES( xdr, auth->oa_base, auth->oa_length ) )
      {
        ERR( "xdr_recv_auth: unable to read auth data", 0, 0, 0 );
        return FALSE;
      }
    }
    else
    {
      ERR( "xdr_recv_auth: unable to alloc memory for base", 0, 0, 0 );
      return FALSE;
    }
  }

  return TRUE;
} /* xdr_recv_auth */

void
xdr_free_auth
(
  opaque_auth * auth
)
{
  if ( auth != NULL && auth->oa_base != NULL )
  {
    oncrpc_mem_free( auth->oa_base );
    auth->oa_base = NULL;
    auth->oa_length = 0;
  }
} /* xdr_free_auth */

/*
 * The call rpc message, msg has been obtained from the wire.  The msg contains
 * the raw form of credentials and verifiers.  authenticate returns AUTH_OK
 * if the msg is successfully authenticated.  If AUTH_OK then the routine also
 * does the following things:
 * set rqst->rq_xprt->verf to the appropriate response verifier;
 * sets rqst->rq_client_cred to the "cooked" form of the credentials.
 *
 * NB: rqst->rq_cxprt->verf must be pre-allocated;
 * its length is set appropriately.
 *
 * The caller still owns and is responsible for msg->u.cmb.cred and
 * msg->u.cmb.verf.  The authentication system retains ownership of
 * rqst->rq_client_cred, the cooked credentials.
 *
 * There is an assumption that any flavour less than AUTH_NULL is
 * invalid.
 */
boolean
xdr_authenticate 
(
  xdr_s_type * xdr
)
{
  int cred_flavor;
  opaque_auth verf;
  boolean ret;

  memset( &verf, 0, sizeof(opaque_auth) );

  /* Grab the credentials from the incoming message */
  if( ! xdr_recv_auth( xdr, &xdr->cred ) || ! xdr_recv_auth( xdr, &verf ) )
  {
    svcerr_auth( xdr, ONCRPC_AUTH_FAILED );
    return FALSE;
  }

  /* Load the XDR verf with default values in case we don't set them */

  xdr->verf.oa_flavor = ONCRPC_AUTH_NONE;
  xdr->verf.oa_length = 0;
  xdr->verf.oa_base = NULL;

  cred_flavor = xdr->cred.oa_flavor;
  if ((cred_flavor <= AUTH_MAX) && (cred_flavor >= ONCRPC_AUTH_NONE))
  {
    ret = (*(svcauthsw[cred_flavor].authenticator)) (xdr, &verf);
    xdr_free_auth( &verf );
    return ret;
  }
  xdr_free_auth( &xdr->cred );
  xdr_free_auth( &verf );
  svcerr_auth(xdr, ONCRPC_AUTH_REJECTEDCRED);
  return FALSE;
} /* xdr_authenticate */

static boolean
oncrpc_svcauth_null
(
  xdr_s_type * xdr,
  opaque_auth * verf
)
{
  return TRUE;
}

/* There is no security here! */
static boolean
oncrpc_svcauth_unix
(
  xdr_s_type * xdr,
  opaque_auth * verf
)
{
  xdr->verf.oa_base = oncrpc_auth_short_base;
  xdr->verf.oa_length = ONCRPC_AUTH_SHORT_LEN;
  xdr->verf.oa_flavor = ONCRPC_AUTH_SHORT;
  return TRUE;
}

/* There is no security here! */
static boolean
oncrpc_svcauth_short
(
  xdr_s_type * xdr,
  opaque_auth * verf
)
{
  return TRUE;
}
