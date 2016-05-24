/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        O N C R P C _ S V C . C

GENERAL DESCRIPTION

  This file provides the server functionality for ONCRPC.

  ONCRPC (Open Network Computing Remote Procedure Calls) is a way of
  calling functions from one machine to another.  This is described in
  detail in the following RFCs: 1831, 1832, 1833

INITIALIZATION AND SEQUENCING REQUIREMENTS

 Copyright (c) 2005-2008 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential.
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_svc.c#4 $ 

when        who    what, where, why
--------    ---    ----------------------------------------------------------
07/10/07    ptm    Remove featurization.
02/01/06    ptm    Fix WinCE compiler warnings.
05/23/05    hn     Chagned svc_freeargs() to use oncrpcxdr_mem_free().
04/26/05    hn     Added missing XDR_MSG_DONE() call to svc_getargs().
04/22/05    ptm    Update the XDR_MSG_SEND API.
04/12/05    ptm    Include oncrpci.h.
04/01/05    clp    Include header cleanup changes.
03/22/05    hn     Moved svcerr_...() routines out of this file and into
                   oncrpcsvc_err.c
03/16/05    clp    Added header to source file.
===========================================================================*/


/* Parts of this file are derived from the original Sun (ONC) RPC
 * code, under the following copyright:
 */
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
 * svc.c, Server-side remote procedure call interface.
 *
 * There are two sets of procedures here.  The xprt routines are
 * for handling transport handles.  The svc routines handle the
 * list of service routines.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#include "comdef.h"
#include "oncrpc.h"
#include "oncrpci.h"

/* ******************* REPLY GENERATION ROUTINES  ************ */

boolean svc_getargs( SVCXPRT * xdr, xdrproc_t xdr_args, caddr_t args_ptr )
{
  boolean result;
  /* call xdr argument routine */
  result = (boolean) (*xdr_args)((XDR *) xdr, args_ptr);

  XDR_MSG_DONE( (XDR *) xdr );

  return result;
} /* svc_getargs */

boolean svc_freeargs (SVCXPRT * xprt, xdrproc_t xdr_args, caddr_t args_ptr)
{
  oncrpcxdr_mem_free( xprt );

  return TRUE;
}

/* Send a reply to an rpc request */
bool_t
svc_sendreply (register SVCXPRT *xdr, xdrproc_t xdr_results,
	       caddr_t xdr_location)
{
  opaque_auth verf;
  
  verf.oa_flavor = ONCRPC_AUTH_NONE;
  verf.oa_length = 0;

  xdr->x_op = XDR_ENCODE;
  
  if( !xdr_reply_msg_start( xdr, &verf ) ||
      !xdr_results( xdr, xdr_location ) ||
      !XDR_MSG_SEND( xdr, NULL ) ) {
    ERR_FATAL( "Unable to send reply", 0, 0, 0 );
    return FALSE;
  }

  return TRUE;
}
