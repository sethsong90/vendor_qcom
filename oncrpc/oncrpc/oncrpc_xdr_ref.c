/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        O N C R P C _ X D R _ R E F . C

GENERAL DESCRIPTION

  This file provides the routines to encode/decode XDR references
  (pointers).

  ONCRPC (Open Network Computing Remote Procedure Calls) is a way of
  calling functions from one machine to another.  This is described in
  detail in the following RFCs: 1831, 1832, 1833

INITIALIZATION AND SEQUENCING REQUIREMENTS

 Copyright (c) 2005, 2007-2008 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential. 

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_xdr_ref.c#4 $ 

when        who    what, where, why
--------    ---    ----------------------------------------------------------
07/10/07    ptm    Remove featurization.
05/23/05    hn     Changed all memory alloc/dealloc's to use oncrpcxdr_mem_alloc
                   and free routines.
04/12/05    ptm    Include oncrpci.h and remove err.h.
03/16/05    clp    Added header to source file.
===========================================================================*/


/* Parts of this file are derived from the original Sun (ONC) RPC
 * code, under the following copyright:
 */
/* @(#)xdr_reference.c	2.1 88/07/29 4.0 RPCSRC */
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
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)xdr_reference.c 1.11 87/08/11 SMI";
#endif

/*
 * xdr_reference.c, Generic XDR routines implementation.
 *
 * Copyright (C) 1987, Sun Microsystems, Inc.
 *
 * These are the "non-trivial" xdr primitives used to serialize and de-serialize
 * "pointers".  See xdr.h for more info on the interface to xdr.
 */

#include "comdef.h"
#include "oncrpc.h"
#include "oncrpci.h"
#include <string.h>

#define LASTUNSIGNED	((u_int)((int)0-1))

/*
 * XDR an indirect pointer
 * xdr_reference is for recursively translating a structure that is
 * referenced by a pointer inside the structure that is currently being
 * translated.  pp references a pointer to storage. If *pp is null
 * the  necessary storage is allocated.
 * size is the size of the referneced structure.
 * proc is the routine to handle the referenced structure.
 */
bool_t
xdr_reference 
(
  XDR *xdrs,
  caddr_t *pp,     /* the pointer to work on */
  u_int size,      /* size of the object pointed to */
  xdrproc_t proc   /* xdr routine to handle the object */
)
{
  caddr_t loc = *pp;
  bool_t stat;

  if (loc == NULL)
  {
    switch (xdrs->x_op)
    {
    case XDR_FREE:
      return TRUE;

    case XDR_DECODE:
      *pp = loc = (caddr_t) oncrpcxdr_mem_alloc (xdrs, size);
      memset(loc, 0, size);
      break;
    default:
      break;
    }
  }

  stat = (*proc) (xdrs, loc, LASTUNSIGNED);

  if (xdrs->x_op == XDR_FREE)
  {
    *pp = NULL;
  }
  return stat;
} /* xdr_reference */


/*
 * xdr_pointer():
 *
 * XDR a pointer to a possibly recursive data structure. This
 * differs with xdr_reference in that it can serialize/deserialize
 * trees correctly.
 *
 *  What's sent is actually a union:
 *
 *  union object_pointer switch (boolean b) {
 *  case TRUE: object_data data;
 *  case FALSE: void nothing;
 *  }
 *
 * > objpp: Pointer to the pointer to the object.
 * > obj_size: size of the object.
 * > xdr_obj: routine to XDR an object.
 *
 */
bool_t
oncrpc_xdr_pointer
(
  XDR *xdrs,
  char **objpp,
  u_int obj_size,
  xdrproc_t xdr_obj
)
{

  bool_t more_data;

  more_data = (*objpp != NULL);
  if (!xdr_bool (xdrs, &more_data))
  {
    return FALSE;
  }
  if (!more_data)
  {
    *objpp = NULL;
    return TRUE;
  }
  return xdr_reference (xdrs, objpp, obj_size, xdr_obj);
} /* xdr_pointer */
