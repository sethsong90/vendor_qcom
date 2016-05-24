/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        O N C R P C . H

GENERAL DESCRIPTION
  This is the public header file included by the rpcgen auto generated
  code, and other external ONCRPC code. 

  This file includes other header file that actually provide the
  external APIs.

  ONCRPC (Open Network Computing Remote Procedure Calls) is a way of
  calling functions from one machine to another.  This is described in
  detail in the following RFCs: 1831, 1832, 1833

 Copyright (c) 2005, 2007 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential.
 
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/inc/oncrpc.h#4$ 

when        who    what, where, why
--------    ---    ----------------------------------------------------------
07/17/08    rr     Add Android support
11/02/07    rr     Add oncrpc_lookup.h
10/16/07    ptm    Remove oncrpcsync.h.
07/10/07    ptm    Moved WINCE include to WINCE header file.
05/29/07    clp    Added OS dependent types.
09/13/05    clp    Featurized the include of oncrpc_cb.h
04/13/05    ptm    Move oncrpc private definitions to oncrpci.h.
04/01/05    clp    Include header cleanup changes.
03/22/05    hn     Included oncrpcsvc_err.h
03/16/05    clp    Added header to source file.
===========================================================================*/

/* Parts of this file are derived from the original Sun (ONC) RPC
 * code, under the following copyright:
 */

/* @(#)rpc.h	2.3 88/08/10 4.0 RPCSRC; from 1.9 88/02/08 SMI */
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
 * rpc.h, Just includes the billions of rpc header files necessary to
 * do remote procedure calling.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#ifdef _RPC_RPC_H
  #error "Can't include system rpc/rpc.h and this header"
#endif

#ifndef _ONCRPC_H
#define _ONCRPC_H 1

/* Set up the types needed for everything else in a OS dependent way */
#include "oncrpc/oncrpc_target.h"

/* external data representation interfaces */
#include "oncrpc/oncrpc_xdr_types.h"     /* Types for oncrpcxdr and more */
#include "oncrpc/oncrpc_xdr.h"		/* generic (de)marshalling */

/* Server side only remote procedure callee */
#include "oncrpc/oncrpc_svc.h"		/* service manager and multiplexer */

#include "oncrpc/oncrpc_svc_err.h"

/* Client side (mostly) remote procedure call */
#include "oncrpc/oncrpc_clnt.h"		/* generic rpc stuff */

/* Memory allocatation */
#include "oncrpc/oncrpc_mem.h"

#include "oncrpc/oncrpc_cb.h"

/* CB Database lookup */
#include "oncrpc/oncrpc_lookup.h"

#include "oncrpc/err.h"

#include "msg.h"

#include "oncrpc/oncrpc_init.h"

#endif /* _ONCRPC_H */
