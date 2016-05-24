/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                  O N C R P C _ X D R _ A R R A Y . C

GENERAL DESCRIPTION

  This file provides the XDR routines in encode/decode arrays.

  ONCRPC (Open Network Computing Remote Procedure Calls) is a way of
  calling functions from one machine to another.  This is described in
  detail in the following RFCs: 1831, 1832, 1833

INITIALIZATION AND SEQUENCING REQUIREMENTS

 Copyright (c) 2005, 2007-2008 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential. 

 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_xdr_array.c#4 $ 

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
/* @(#)xdr_array.c	2.1 88/07/29 4.0 RPCSRC */
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
static char sccsid[] = "@(#)xdr_array.c 1.10 87/08/11 Copyr 1984 Sun Micro";
#endif

/*
 * xdr_array.c, Generic XDR routines implementation.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * These are the "non-trivial" xdr primitives used to serialize and de-serialize
 * arrays.  See xdr.h for more info on the interface to xdr.
 */

#include "comdef.h"
#include "oncrpc.h"
#include "oncrpci.h"

#define LASTUNSIGNED	((u_int)((int)0-1))


/*
 * XDR an array of arbitrary elements
 * *addrp is a pointer to the array, *sizep is the number of elements.
 * If addrp is NULL (*sizep * elsize) bytes are allocated.
 * elsize is the size (in bytes) of each element, and elproc is the
 * xdr procedure to call to handle each element of the array.
 */
bool_t
oncrpc_xdr_array
( 
     XDR *xdrs,
     caddr_t *addrp,		/* array pointer */
     u_int *sizep,	  	/* number of elements */
     u_int maxsize,	  	/* max numberof elements */
     u_int elsize,		  /* size in bytes of each element */
     xdrproc_t elproc		/* xdr routine to handle each element */
)
{
  u_int i;
  caddr_t target = *addrp;
  u_int c;		/* the actual element count */
  bool_t stat = TRUE;
  u_int nodesize;

  /* like strings, arrays are really counted arrays */
  if (!xdr_u_int (xdrs, sizep))
    {
      return FALSE;
    }
  c = *sizep;
  if ((c > maxsize) && (xdrs->x_op != XDR_FREE))
    {
      return FALSE;
    }
  nodesize = c * elsize;

  /*
   * if we are deserializing, we may need to allocate an array.
   * We also save time by checking for a null array if we are freeing.
   */
  if (target == NULL)
    switch (xdrs->x_op)
      {
      case XDR_DECODE:
	if (c == 0)
	  return TRUE;

	*addrp = target = oncrpcxdr_mem_alloc (xdrs, nodesize);
	memset (target, 0, nodesize);
	break;
        
      case XDR_FREE:
	return TRUE;
      default:
	break;
      }

  /*
   * now we xdr each element of array
   */
  for (i = 0; (i < c) && stat; i++)
    {
      stat = (*elproc) (xdrs, target, LASTUNSIGNED);
      target += elsize;
    }

  /*
   * the array may need freeing
   */
  if (xdrs->x_op == XDR_FREE)
    {
      *addrp = NULL;
    }
  return stat;
}

/*
 * xdr_vector():
 *
 * XDR a fixed length array. Unlike variable-length arrays,
 * the storage of fixed length arrays is static and unfreeable.
 * > basep: base of the array
 * > size: size of the array
 * > elemsize: size of each element
 * > xdr_elem: routine to XDR each element
 */
bool_t
oncrpc_xdr_vector
(
  XDR *xdrs,
  char *basep,
  u_int nelem,
  u_int elemsize,
  xdrproc_t xdr_elem
)
{
  u_int i;
  char *elptr;

  elptr = basep;
  for (i = 0; i < nelem; i++)
    {
      if (!(*xdr_elem) (xdrs, elptr, LASTUNSIGNED))
	{
	  return FALSE;
	}
      elptr += elemsize;
    }
  return TRUE;
}
