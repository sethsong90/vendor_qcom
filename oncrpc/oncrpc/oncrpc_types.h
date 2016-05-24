#ifndef _RPC_TYPES_H
#define _RPC_TYPES_H 1
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        O N C R P C _ T Y P E S . H

GENERAL DESCRIPTION

  This header file provides mappings between the original ONCRPC
  types, and the primitaves types. 

  ONCRPC (Open Network Computing Remote Procedure Calls) is a way of
  calling functions from one machine to another.  This is described in
  detail in the following RFCs: 1831, 1832, 1833

INITIALIZATION AND SEQUENCING REQUIREMENTS

 Copyright (c) 2005,2007-2008 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential. 
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_types.h#4 $ 

when        who    what, where, why
--------    ---    ----------------------------------------------------------
07/10/07    ptm    Remove and cleanup featurization.
07/05/07    ptm    Added support for QUARTZ 1.3.5.
05/21/07    hn     Moved definitions of protocols to oncrpctypes.h.
05/09/07    hn     Defined ONCRPC_INFINITY constant to -1.
03/31/05    clp    cleaned out unused stuff.
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
/* fixincludes should not add extern "C" to this file */
/*
 * Rpc additions to <sys/types.h>
 */

#include "comdef.h"

typedef  int32 bool_t;
typedef  int32 enum_t;
typedef uint32 rpcprog_t;
typedef uint32 rpcvers_t;
typedef uint32 rpcproc_t;
typedef  int32 rpcprot_t;
typedef uint32 rpcport_t;

#define __BEGIN_DECLS 
#define __END_DECLS
#define __THROW

typedef uint8 u_char;
typedef uint16 u_short;
typedef uint32 u_int;
typedef uint32 u_long;
typedef int64 quad_t;
typedef uint64 u_quad_t;
typedef char * caddr_t;
typedef int8  int8_t;
typedef int16 int16_t;
typedef int64 int64_t;
typedef uint8  uint8_t;
typedef uint16 uint16_t;
typedef uint64 uint64_t;
typedef uint16 u_int16_t;
typedef uint32 u_int32_t;
typedef uint64 u_int64_t;

#ifndef FEATURE_QUARTZ_135
typedef int32 int32_t;
typedef uint32 uint32_t;
#endif

#define ONCRPC_INFINITY (-1)

/* Define the non-standard protocols -
   the standard ones are IPPROTO_TCP and IPPROTO_UDP */
#define ONCRPC_SIO_PROTOCOL            ((rpcprot_t)(-1))
#define ONCRPC_DIAG_PROTOCOL           ((rpcprot_t)(-2))
#define ONCRPC_SM_PROTOCOL             ((rpcprot_t)(-3))
#define ONCRPC_RTR_PROTOCOL            ((rpcprot_t)(-4))
#define ONCRPC_LO_PROTOCOL             ((rpcprot_t)(-5))

#if defined(FEATURE_ONCRPC_SM_IS_ROUTER)
#undef  ONCRPC_SM_PROTOCOL
#define ONCRPC_SM_PROTOCOL ONCRPC_RTR_PROTOCOL
#define svcsm_create(a,b) svcrtr_create(a,b)
#endif /* FEATURE_ONCRPC_SM_IS_ROUTER */

#endif /* _RPC_TYPES_H */
