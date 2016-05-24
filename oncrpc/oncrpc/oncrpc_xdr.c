/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        O N C R P C _ X D R . C

GENERAL DESCRIPTION

  This file provides the XDR routines for ONCRPC.

  ONCRPC (Open Network Computing Remote Procedure Calls) is a way of
  calling functions from one machine to another.  This is described in
  detail in the following RFCs: 1831, 1832, 1833

INITIALIZATION AND SEQUENCING REQUIREMENTS

 Copyright (c) 2005-2008, 2011-2012 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_xdr.c#5 $ 

when        who    what, where, why
--------    ---    ----------------------------------------------------------
28/07/11    as     Enable userspace logging
09/11/08    rr     Merge from mainline, cleanup includes
04/21/08    sd     Added oncrpc_get_last_sent_msg_info() and 
                   oncrpc_set_last_sent_msg_info()
07/09/08   rr         Fix read handle for multiple callbacks CBSP20
03/13/08    hn     Fixed the xdr_enum routine to take a value parameter and use
                   it when encoding enums.
02/20/08    hn     Added xdr boolean routines.
02/20/08    hn     Fixed the xdr_send_packed_enum routine to pass by value.
01/21/08    hn     "x_prog" field of XDR no longer set in msg_start
10/31/07    hn     Fixed the xdr_send_enum routine to pass by value as int32.
10/17/07    hn     Added xdr_unknown_discriminator_msg message.
08/22/07    hn     Modified error messages.
07/10/07    ptm    Remove featurization.
06/13/07    hn     Added global declarations for error message strings.
10/03/06    hn     Added support for locking/unlocking RPC services.
03/03/06    ptm    Fix WinCE compiler warnings.
10/12/05    clp    Move call back related routines to oncrpc_cb.c.
08/08/05    hn     Added oncrpc_clnt_destroy helper function to be called by r e x.
06/09/05    ptm    Add block comments.
05/23/05    hn     Added oncrpcxdr_mem_alloc and oncrpcxdr_mem_free routines
05/17/05    hn     Added missing variants of 64 bit integer xdr routines
05/16/05    hn     Add 64-bit XDR support in ONCRPC.
05/12/05    ptm    Merged XDR field names.
04/14/05    ptm    Include task.h for ONCRPC_REPLY_SIG.
04/13/05    ptm    Move reply queue code to oncrpctask.c.
04/12/05    ptm    Include oncrpci.h and remove err.h.
04/01/05    clp    Include header cleanup changes and add reply xdr routines.
03/28/05    ptm    Change FEATURE ONCRPC CB SUPPORT to FEATURE MULTIPROCESSOR.
03/22/05    hn     Added "const" qualifier to parameter in xdr_send routines
                   for enums, packed enums and auth.        
                   Added xdr_free_auth() routine.
                   Fixed bug in xdr_send_packed_uint16() routine.
03/16/05    clp    Added header to source file.
===========================================================================*/


/* Parts of this file are derived from the original Sun (ONC) RPC
 * code, under the following copyright:
 */
/* @(#)xdr.c	2.1 88/07/29 4.0 RPCSRC */
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

#include "comdef.h"
#include "oncrpc.h"
#include "oncrpci.h"

/*============================================================================
 *  G L O B A L   D E C L A R A T I O N S   F O R   E R R   M E S S A G E S
 *===========================================================================*/
const char xdr_clnt_lookup_err_msg[]=
"Failed to lookup RPC client. prog = %d, vers = %d, tout = %d";
const char xdr_msg_send_err_msg[]=
"Failed to send RPC message. xdr = %d, reply = %d";
const char xdr_call_rejected_err_msg[]=
"RPC call rejected, clnt = %d, reject status = %d";
const char xdr_err_on_server_err_msg[]=
"Error on server side, clnt = %d, error status = %d";
const char xdr_op_err_msg[]=
"XDR operation failed. xdr = %d, XDR OP NUMBER = %d";
const char xdr_svc_create_err_msg[]=
"Failed to create RPC app";
const char xdr_svc_register_err_msg[]=
"Failed to register RPC app";
const char rpc_svc_cb_data_lookup_err_msg[]=
"Failed to lookup cb data on the server";
const char xdr_unknown_discriminator_msg[]=
"Got unknown discriminator value: %d";
const char xdr_malloc_err_msg[]=
"Failed to allocate %d bytes from ONCRPC";

/*============================================================================
 *                 X D R    H E L P E R    F U N C T I O N S
 *===========================================================================*/

/*===========================================================================
  FUNCTION: xdr_send_int64

  DESCRIPTION:
      Send an int64 by sending two int32s.

  RESULT:
      TRUE is send was successful, FALSE otherwise
  ===========================================================================*/
boolean xdr_send_int64( xdr_s_type *xdr, const int64 *value )
{
  long t1;
  unsigned long t2;

  t1 = (long) ((*value) >> 32);
  t2 = (long) (*value);

  return ( XDR_SEND_INT32(xdr, (int32 *)&t1) &&
           XDR_SEND_INT32(xdr, (int32 *)&t2) );
} /* xdr_send_int64 */

/*===========================================================================
  FUNCTION: xdr_recv_int64

  DESCRIPTION:
      Receive an int64 by receiving two int32s.

  RESULT:
      TRUE is receive was successful, FALSE otherwise
  ===========================================================================*/
boolean xdr_recv_int64( xdr_s_type *xdr, int64 *value )
{
  long t1;
  unsigned long t2;

  if ( !XDR_RECV_INT32(xdr, (int32 *)&t1) ||
       !XDR_RECV_INT32(xdr, (int32 *)&t2) )
  {
    return FALSE;
  }

  *value = ((int64) t1) << 32;
  *value |= t2;

  return TRUE;
} /* xdr_recv_int64 */

/*===========================================================================
  FUNCTION: xdr_send_uint64

  DESCRIPTION:
      Send an uint64 by sending two int32s.

  RESULT:
      TRUE is send was successful, FALSE otherwise
  ===========================================================================*/
boolean xdr_send_uint64( xdr_s_type *xdr, const uint64 *value )
{
  unsigned long t1;
  unsigned long t2;

  t1 = (unsigned long) ((*value) >> 32);
  t2 = (unsigned long) (*value);

  return ( XDR_SEND_INT32(xdr, (int32 *)&t1) &&
           XDR_SEND_INT32(xdr, (int32 *)&t2) );
} /* xdr_send_uint64 */

/*===========================================================================
  FUNCTION: xdr_recv_uint64

  DESCRIPTION:
      Receive an uint64 by receiving two int32s.

  RESULT:
      TRUE is receive was successful, FALSE otherwise
  ===========================================================================*/
boolean xdr_recv_uint64( xdr_s_type *xdr, uint64 *value )
{
  unsigned long t1;
  unsigned long t2;

  if ( !XDR_RECV_INT32(xdr, (int32 *)&t1) ||
       !XDR_RECV_INT32(xdr, (int32 *)&t2) )
  {
    return FALSE;
  }

  *value = ((uint64) t1) << 32;
  *value |= t2;

  return TRUE;
} /* xdr_recv_uint64 */

/*===========================================================================
  FUNCTION: oncrpc_xdr_send_enum

  DESCRIPTION:
      Send an enum by switching on the size and sending the appropriately
      size int.

  RESULT:
      TRUE is send was successful, FALSE otherwise
  ===========================================================================*/
boolean oncrpc_xdr_send_enum( xdr_s_type *xdr, int32 value )
{
  return XDR_SEND_INT32( xdr, &value );
} /* xdr_send_enum */

/*===========================================================================
  FUNCTION: oncrpc_xdr_recv_enum

  DESCRIPTION:
      Receive an enum by switching on the size and receiving the appropriately
      size int.

  RESULT:
      TRUE is receive was successful, FALSE otherwise
  ===========================================================================*/
boolean oncrpc_xdr_recv_enum( xdr_s_type *xdr, void *value, uint32 size )
{
  if( size == sizeof(int32) ) {
    return XDR_RECV_INT32( xdr, (int32 *) value );
  }
  else if( size == sizeof(int16) ) {
    return XDR_RECV_INT16( xdr, (int16 *) value );
  }
  else if( size == sizeof(char) ) {
    return XDR_RECV_INT8( xdr, (int8 *) value );
  }

  return FALSE;
} /* xdr_recv_enum */

/*
 * Packed version of the enum send/recv helper functions
 */

boolean xdr_send_packed_enum
(
  xdr_s_type *xdr,
  int32 value
)
{
  return xdr_send_enum(xdr, value);
} /* xdr_send_packed_enum */

boolean xdr_recv_packed_enum
(
  xdr_s_type *xdr,
  PACKED void *value PACKED_POST,
  uint32 size
)
{
  if( size == sizeof(int32) ) {
    return XDR_RECV_PACKED_INT32( xdr, (PACKED int32 *) value );
  }
  else if( size == sizeof(int16) ) {
    return XDR_RECV_PACKED_INT16( xdr, (PACKED int16 *) value );
  }
  else if( size == sizeof(char) ) {
    return XDR_RECV_PACKED_INT8( xdr, (PACKED int8 *) value  );
  }

  return FALSE;
} /* xdr_recv_packed_enum */

/*===========================================================================
  FUNCTION: xdr_send_boolean

  DESCRIPTION:
      Send a boolean.

  RESULT:
      TRUE is send was successful, FALSE otherwise
  ===========================================================================*/
boolean xdr_send_boolean
(
  xdr_s_type *xdr,
  const boolean *value
)
{
  uint32 val = (uint32) *value;

  return XDR_SEND_UINT32( xdr, &val );
} /* xdr_send_boolean */

/*===========================================================================
  FUNCTION: xdr_recv_boolean

  DESCRIPTION:
      Receive a boolean.

  RESULT:
      TRUE is send was successful, FALSE otherwise
  ===========================================================================*/
boolean xdr_recv_boolean
(
  xdr_s_type *xdr,
  boolean *value
)
{
  uint32 val;
  boolean res;

  res = XDR_RECV_UINT32( xdr, &val );

  if ( res )
  {
    *value = (boolean) val;
  }

  return res;
} /* xdr_recv_boolean */

/*
 * Packed version of the boolean send/recv helper functions
 */
boolean xdr_send_packed_boolean
(
  xdr_s_type *xdr,
  const PACKED boolean *value PACKED_POST
)
{
  uint32 val = (uint32) *value;

  return XDR_SEND_UINT32( xdr, &val );
} /* xdr_send_packed_boolean */

boolean xdr_recv_packed_boolean
(
  xdr_s_type *xdr,
  PACKED boolean *value PACKED_POST
)
{
  uint32 val;
  boolean res;

  res = XDR_RECV_UINT32( xdr, &val );

  if ( res )
  {
    *value = (boolean) val;
  }

  return res;
} /* xdr_recv_packed_boolean */

/* 
 * The following helper routines are used to send packed primitives.
 */

boolean xdr_send_packed_int8
(
  xdr_s_type *xdr,
  const PACKED int8 *value PACKED_POST
)
{
  int8 copy = *value;
  return XDR_SEND_INT8( xdr, &copy );
} /* xdr_send_packed_int8 */

boolean xdr_send_packed_uint8
(
  xdr_s_type *xdr,
  const PACKED uint8 *value PACKED_POST
)
{
  uint8 copy = *value;
  return XDR_SEND_UINT8( xdr, &copy );
} /* xdr_send_packed_uint8 */

boolean xdr_send_packed_int16
(
  xdr_s_type *xdr,
  const PACKED int16 *value PACKED_POST
)
{
  int16 copy = *value;
  return XDR_SEND_INT16( xdr, &copy );
} /* xdr_send_packed_int16 */

boolean xdr_send_packed_uint16
(
  xdr_s_type *xdr,
  const PACKED uint16 *value PACKED_POST
)
{
  uint16 copy = *value;
  return XDR_SEND_UINT16( xdr, &copy );
} /* xdr_send_packed_uint16 */

boolean xdr_send_packed_int32
(
  xdr_s_type *xdr,
  const PACKED int32 *value PACKED_POST
)
{
  int32 copy = *value;
  return XDR_SEND_INT32( xdr, &copy );
} /* xdr_send_packed_int32 */

boolean xdr_send_packed_uint32
(
  xdr_s_type *xdr,
  const PACKED uint32 *value PACKED_POST
)
{
  uint32 copy = *value;
  return XDR_SEND_UINT32( xdr, &copy );
} /* xdr_send_packed_uint32 */

boolean xdr_send_packed_int64
(
  xdr_s_type *xdr,
  const PACKED int64 *value PACKED_POST
)
{
  int64 copy = *value;
  return XDR_SEND_INT64( xdr, &copy );
} /* xdr_send_packed_int64 */

boolean xdr_send_packed_uint64
(
  xdr_s_type *xdr,
  const PACKED uint64 *value PACKED_POST
)
{
  uint64 copy = *value;
  return XDR_SEND_UINT64( xdr, &copy );
} /* xdr_send_packed_uint64 */

/* 
 * The following helper routines are used to receive packed primitives.
 */

boolean xdr_recv_packed_int8
(
  xdr_s_type *xdr,
  PACKED int8 *value PACKED_POST
)
{
  int8 copy;
  if ( ! XDR_RECV_INT8( xdr, &copy ) ) {
      return FALSE;
  }
  *value = copy;
  return TRUE;
} /* xdr_recv_packed_int8 */

boolean xdr_recv_packed_uint8
(
  xdr_s_type *xdr,
  PACKED uint8 *value PACKED_POST
)
{
  uint8 copy;
  if ( ! XDR_RECV_UINT8( xdr, &copy ) ) {
    return FALSE;
  }
  *value = copy;
  return TRUE;
} /* xdr_recv_packed_uint8 */

boolean xdr_recv_packed_int16
(
  xdr_s_type *xdr,
  PACKED int16 *value PACKED_POST
)
{
  int16 copy;
  if ( ! XDR_RECV_INT16( xdr, &copy ) ) {
    return FALSE;
  }
  *value = copy;
  return TRUE;
} /* xdr_recv_packed_uint16 */

boolean xdr_recv_packed_uint16
(
  xdr_s_type *xdr,
  PACKED uint16 *value PACKED_POST
)
{
  uint16 copy;
  if ( ! XDR_RECV_UINT16( xdr, &copy ) ) {
    return FALSE;
  }
  *value = copy;
  return TRUE;
} /* xdr_recv_packed_uint16 */

boolean xdr_recv_packed_int32
(
  xdr_s_type *xdr,
  PACKED int32 *value PACKED_POST
)
{
  int32 copy;
  if ( ! XDR_RECV_INT32( xdr, &copy ) ) {
    return FALSE;
  }
  *value = copy;
  return TRUE;
} /* xdr_recv_packed_int32 */

boolean xdr_recv_packed_uint32
(
  xdr_s_type *xdr,
  PACKED uint32 *value PACKED_POST
)
{
  uint32 copy;
  if ( ! XDR_RECV_UINT32( xdr, &copy ) ) {
    return FALSE;
  }
  *value = copy;
  return TRUE;
} /* xdr_recv_packed_uint32 */

boolean xdr_recv_packed_int64
(
  xdr_s_type *xdr,
  PACKED int64 *value PACKED_POST
)
{
  int64 copy;
  if ( ! XDR_RECV_INT64( xdr, &copy ) ) {
    return FALSE;
  }
  *value = copy;
  return TRUE;
} /* xdr_recv_packed_int64 */

boolean xdr_recv_packed_uint64
(
  xdr_s_type *xdr,
  PACKED uint64 *value PACKED_POST
)
{
  uint64 copy;
  if ( ! XDR_RECV_UINT64( xdr, &copy ) ) {
    return FALSE;
  }
  *value = copy;
  return TRUE;
} /* xdr_recv_packed_uint64 */



/*===========================================================================*/
boolean oncrpc_xdr_call_msg_start
(
  xdr_s_type *xdr,
  uint32 prog,
  uint32 ver,
  uint32 proc,
  opaque_auth *cred,
  opaque_auth *verf
)
{
  uint32         vers = RPC_MSG_VERSION;

  xdr->x_prog = prog;
  xdr->x_vers = ver;
  xdr->x_proc = proc;

  ONCRPC_SYSTEM_LOGI("%s: Prog: %08x, Ver: %08x, Proc: %08x\n",
                     __func__, prog, ver, proc);

  return ( XDR_MSG_START( xdr, RPC_MSG_CALL ) &&
           XDR_SEND_UINT32( xdr, &vers ) &&
           XDR_SEND_UINT32( xdr, &prog ) &&
           XDR_SEND_UINT32( xdr, &ver ) &&
           XDR_SEND_UINT32( xdr, &proc ) &&
           xdr_send_auth( xdr, cred ) &&
           xdr_send_auth( xdr, verf ) );
} // xdr_call_msg_start


boolean oncrpc_xdr_reply_msg_start
(
  xdr_s_type *xdr,
  opaque_auth *verf
)
{
  int32 stat   = (int32) RPC_MSG_ACCEPTED;
  int32 accept = (int32) RPC_ACCEPT_SUCCESS;

  ONCRPC_SYSTEM_LOGI("%s: Prog: %08x, Ver: %08x, Proc: %08x Xid: %08x\n",
                      __func__, xdr->x_prog, xdr->x_vers, xdr->x_proc, xdr->xid);
  XDR_CONTROL(xdr,ONCRPC_CONTROL_SET_DEST,&(xdr->msg_source));

  return( XDR_MSG_START( xdr, RPC_MSG_REPLY ) &&
          XDR_SEND_INT32( xdr, &stat ) &&
          xdr_send_auth( xdr, verf ) &&
          XDR_SEND_INT32( xdr, &accept ) );
} // xdr_reply_msg_start

/*===========================================================================
  FUNCTION: oncrpcxdr_mem_alloc

  DESCRIPTION:
      Allocates buffer from oncrpc's pool of memory and associates it with
      an XDR.

  RESULT:
      Pointer to allocated buffer or NULL if allocation failed
  ===========================================================================*/
void *oncrpcxdr_mem_alloc( xdr_s_type *xdr, uint32 size )
{
  oncrpcxdr_mem_s_type *wrapper;

  /* We can use a very restrictive case here because size is a uint32 */
  if(0 == size) {
    ERR_FATAL( "oncrpcxdr_mem_alloc: passed size of 0",
               0, 0, 0 );
  }

  size += sizeof(oncrpcxdr_mem_s_type);

  wrapper = oncrpc_mem_alloc( size );

  if ( wrapper == NULL ) {
    ERR_FATAL( "oncrpcxdr_mem_alloc: Failed to allocate oncrpc buffer (%d)",
               size, 0, 0 );
  }

  wrapper->next = xdr->mem;
  xdr->mem = wrapper;

  return (void *) &wrapper[1];

} /* oncrpcxdr_mem_alloc */

/*===========================================================================
  FUNCTION: oncrpcxdr_mem_free

  DESCRIPTION:
      Frees all the buffers that were allocated from oncrpc's pool of memory
      and were associated with an XDR.

  RESULT:
      None
  ===========================================================================*/
void oncrpcxdr_mem_free( xdr_s_type *xdr )
{
  oncrpcxdr_mem_s_type *head = NULL;
  while ( xdr->mem != NULL ) {
    head = xdr->mem;
    xdr->mem = head->next;
    oncrpc_mem_free(head);
  }
} /* oncrpcxdr_mem_free */

/***********************************************************************
        Support for rpcgen
 ***********************************************************************/
/*
 * constants specific to the xdr "protocol"
 */
#define XDR_FALSE	((long) 0)
#define XDR_TRUE	((long) 1)
#define LASTUNSIGNED	((u_int)((int)0-1))

bool_t oncrpc_xdr_void( void )
{
  return TRUE;
} /* xdr_void */

/*
 * XDR hyper integers
 * same as xdr_u_hyper - open coded to save a proc call!
 */
bool_t
xdr_hyper (XDR *xdrs, quad_t *llp)
{
  long t1;
  unsigned long int t2;

  if (xdrs->x_op == XDR_ENCODE)
    {
      t1 = (long) ((*llp) >> 32);
      t2 = (long) (*llp);
      return (XDR_SEND_INT32(xdrs, (int32 *)&t1) &&
              XDR_SEND_INT32(xdrs, (int32 *)&t2));
    }

  if (xdrs->x_op == XDR_DECODE)
    {
      if (!XDR_RECV_INT32(xdrs, (int32 *)&t1) ||
          !XDR_RECV_INT32(xdrs, (int32 *)&t2))
	return FALSE;
      *llp = ((quad_t) t1) << 32;
      *llp |= t2;
      return TRUE;
    }

  if (xdrs->x_op == XDR_FREE)
    return TRUE;

  return FALSE;
}

bool_t
xdr_int64_t (XDR *xdrs, int64_t *llp)
{
  return xdr_hyper( xdrs, llp );
}

bool_t
xdr_longlong_t (XDR *xdrs, quad_t *llp)
{
  return xdr_hyper( xdrs, llp );
}

bool_t
oncrpc_xdr_quad_t (XDR *xdrs, quad_t *llp)
{
  return xdr_hyper( xdrs, llp );
}

/*
 * XDR hyper integers
 * same as xdr_hyper - open coded to save a proc call!
 */
bool_t
xdr_u_hyper (XDR *xdrs, u_quad_t *ullp)
{
  unsigned long t1;
  unsigned long t2;

  if (xdrs->x_op == XDR_ENCODE)
    {
      t1 = (unsigned long) ((*ullp) >> 32);
      t2 = (unsigned long) (*ullp);
      return (XDR_SEND_INT32(xdrs, (int32 *)&t1) &&
              XDR_SEND_INT32(xdrs, (int32 *)&t2));
    }

  if (xdrs->x_op == XDR_DECODE)
    {
      if (!XDR_RECV_INT32(xdrs, (int32 *)&t1) ||
          !XDR_RECV_INT32(xdrs, (int32 *)&t2))
	return FALSE;
      *ullp = ((u_quad_t) t1) << 32;
      *ullp |= t2;
      return TRUE;
    }

  if (xdrs->x_op == XDR_FREE)
    return TRUE;

  return FALSE;
}

bool_t
xdr_uint64_t (XDR *xdrs, uint64_t *ullp)
{
  return xdr_u_hyper( xdrs, ullp );
}

bool_t
xdr_u_longlong_t (XDR *xdrs, u_quad_t *ullp)
{
  return xdr_u_hyper( xdrs, ullp );
}

bool_t
oncrpc_xdr_u_quad_t (XDR *xdrs, u_quad_t *ullp)
{
  return xdr_u_hyper( xdrs, ullp );
}

bool_t oncrpc_xdr_int(
  XDR *xdr,
  int *ip
)
{
  switch( xdr->x_op ) {
    case XDR_ENCODE:
      return XDR_SEND_INT32( xdr, (int32 *) ip );

    case XDR_DECODE:
      return XDR_RECV_INT32( xdr, (int32 *) ip );

    case XDR_FREE:
      return TRUE;

    default:
      ERR_FATAL( "invalid x_op %x", xdr->x_op, 0, 0 );
      break;
  }

  return FALSE;
} /* xdr_int */

bool_t oncrpc_xdr_u_int(
  XDR *xdr,
  u_int *uip
)
{
  switch( xdr->x_op ) {
    case XDR_ENCODE:
      return XDR_SEND_UINT32( xdr, (uint32 *) uip );

    case XDR_DECODE:
      return XDR_RECV_UINT32( xdr, (uint32 *) uip );

    case XDR_FREE:
      return TRUE;

    default:
      ERR_FATAL( "invalid x_op %x", xdr->x_op, 0, 0 );
      break;
  }

  return FALSE;
} /* xdr_u_int */

bool_t oncrpc_xdr_long
(
  XDR *xdr,
  long *lp
)
{
  switch( xdr->x_op ) {
    case XDR_ENCODE:
      return XDR_SEND_INT32( xdr, lp );

    case XDR_DECODE:
      return XDR_RECV_INT32( xdr, lp );

    case XDR_FREE:
      return TRUE;

    default:
      ERR_FATAL( "invalid x_op %x", xdr->x_op, 0, 0 );
      break;
  }

  return FALSE;
} /* xdr_long */

bool_t oncrpc_xdr_u_long
(
  XDR *xdr,
  u_long *ulp
)
{
  switch( xdr->x_op ) {
    case XDR_ENCODE:
      return XDR_SEND_UINT32( xdr, ulp );

    case XDR_DECODE:
      return XDR_RECV_UINT32( xdr, ulp );

    case XDR_FREE:
      return TRUE;

    default:
      ERR_FATAL( "invalid x_op %x", xdr->x_op, 0, 0 );
      break;
  }

  return FALSE;
} /* xdr_u_long */

bool_t oncrpc_xdr_short
(
  XDR *xdr,
  short *sp
)
{
  long l;

  switch (xdr->x_op) {
    case XDR_ENCODE:
      l = (long) *sp;
      return XDR_SEND_INT32( xdr, &l );

    case XDR_DECODE:
      if( !XDR_RECV_INT32( xdr, &l ) )
      {
        return FALSE;
      }
      *sp = (short) l;
      return TRUE;

    case XDR_FREE:
      return TRUE;

    default:
      ERR_FATAL( "invalid x_op %x", xdr->x_op, 0, 0 );
      break;
  }

  return FALSE;
} /* xdr_short */

bool_t oncrpc_xdr_u_short
(
  XDR *xdr,
  u_short *usp
)
{
  u_long l;

  switch (xdr->x_op) {
    case XDR_ENCODE:
      l = (u_long) * usp;
      return XDR_SEND_UINT32( xdr, &l );

    case XDR_DECODE:
      if( !XDR_RECV_UINT32( xdr, &l ) )
      {
        return FALSE;
      }
      *usp = (u_short) l;
      return TRUE;

    case XDR_FREE:
      return TRUE;

    default:
      ERR_FATAL( "invalid x_op %x", xdr->x_op, 0, 0 );
      break;
  }

  return FALSE;
} /* xdr_u_short */

bool_t oncrpc_xdr_char(
  XDR *xdr,
  char *cp
)
{
  int i;

  i = (*cp);
  if (!xdr_int (xdr, &i))
  {
    return FALSE;
  }
  *cp = i;

  return TRUE;
} /* xdr_char */

bool_t oncrpc_xdr_u_char
(
  XDR *xdr,
  u_char *cp
)
{
  u_int u;

  u = (*cp);
  if (!xdr_u_int (xdr, &u))
    {
      return FALSE;
    }
  *cp = (u_char) u;

  return TRUE;
} /* xdr_u_char */

bool_t oncrpc_xdr_bytes
( 
  XDR    *xdr,
  char  **cpp,
  u_int  *sizep,
  u_int   maxsize
)
{
  switch( xdr->x_op ) {
    case XDR_DECODE:
      if( !XDR_RECV_UINT( xdr,  sizep ) || *sizep > maxsize ) {
        return FALSE;
      }

      if( *sizep == 0 ) {
        return TRUE;
      }

      if( *cpp == NULL ) {
        *cpp = (char *) oncrpcxdr_mem_alloc( xdr, *sizep );
      }

      return XDR_RECV_BYTES( xdr, (uint8 *) *cpp, *sizep );

    case XDR_ENCODE:
      return ( XDR_SEND_UINT( xdr, sizep ) &&
               *sizep <= maxsize &&
               XDR_SEND_BYTES( xdr, (uint8 *) *cpp, *sizep ) );

    case XDR_FREE:
      *cpp = NULL;
      return TRUE;

    default:
      ERR_FATAL( "xdr_bytes: invalid x_op %x", xdr->x_op, 0, 0 );
      break;
  }

  return FALSE;
} /* xdr_bytes */

bool_t oncrpc_xdr_bool
(
  XDR *xdr,
  bool_t *bp
)
{
  uint32 lb;

  switch( xdr->x_op ) {
    case XDR_ENCODE:
      lb = *bp ? XDR_TRUE : XDR_FALSE;
      return XDR_SEND_UINT32( xdr, &lb );

    case XDR_DECODE:
      if( !XDR_RECV_UINT32( xdr, &lb ) ) {
        return FALSE;
      }
      *bp = (lb == XDR_FALSE) ? FALSE : TRUE;
      return TRUE;

    case XDR_FREE:
      return TRUE;

    default:
      ERR_FATAL( "invalid x_op %x", xdr->x_op, 0, 0 );
      break;
  }

  return FALSE;
} /* xdr_bool */

/* See oncrpcxdr.h for definition of xdr_enum */
bool_t oncrpc_xdr_enum_ext(
  XDR *xdr,
  enum_t *ep,
  int32 ev,
  uint16 size
)
{
  switch( xdr->x_op ) {
    case XDR_ENCODE:
      return xdr_send_enum( xdr, ev );

    case XDR_DECODE:
      return xdr_recv_enum( xdr, ep, size );

    case XDR_FREE:
      return TRUE;

    default:
      ERR_FATAL( "invalid x_op %x", xdr->x_op, 0, 0 );
      break;
  }

  return FALSE;
} /* xdr_enum_ext */

bool_t oncrpc_xdr_opaque(
  XDR *xdr,
  caddr_t cp,
  u_int cnt
)
{
  /* if no data we are done */
  if( cnt == 0 ) {
    return TRUE;
  }

  switch( xdr->x_op ) {
    case XDR_ENCODE:
      if( !XDR_SEND_BYTES( xdr, (uint8 *) cp, cnt ) ) {
        return FALSE;
      }
      return TRUE;

    case XDR_DECODE:
      if( !XDR_RECV_BYTES( xdr, (uint8 *) cp, cnt ) ) {
        return FALSE;
      }
      return TRUE;

    case XDR_FREE:
      return TRUE;

    default:
      ERR_FATAL( "invalid x_op %x", xdr->x_op, 0, 0 );
      break;
  }

  return FALSE;
} /* xdr_opaque */

/*
 * XDR a discriminated union
 * Support routine for discriminated unions.
 * You create an array of xdrdiscrim structures, terminated with
 * an entry with a null procedure pointer.  The routine gets
 * the discriminant value and then searches the array of xdrdiscrims
 * looking for that value.  It calls the procedure given in the xdrdiscrim
 * to handle the discriminant.  If there is no specific routine a default
 * routine may be called.
 * If there is no specific or default routine an error is returned.
 */
bool_t
xdr_union
(
  XDR *xdr,
  enum_t *dscmp,                      /* enum to decide which arm to work on */
  uint16 size,                        /* sizeof enum */
  char *unp,                          /* the union itself */
  const struct xdr_discrim *choices,  /* [value, xdr proc] for each arm */
  xdrproc_t dfault                    /* default xdr routine */
)
{
  enum_t dscm;

  /*
   * we deal with the discriminator;  it's an enum
   */
  if (!xdr_enum_ext (xdr, dscmp, *dscmp, size ))
  {
    return FALSE;
  }
  switch ( size )
  {
  case 4:
    dscm = *dscmp;
    break;
  case 2:
    dscm = * (uint16*) dscmp;
    break;
  case 1:
    dscm = * (uint8*) dscmp;
    break;
  default:
    ERR_FATAL( "Unknown size %d", size, 0, 0);
    /* Shut compile/lint warning up */
    dscm = 0;
    break;
  }

  /*
   * search choices for a value that matches the discriminator.
   * if we find one, execute the xdr routine for that value.
   */
  for (; choices->proc != NULL_xdrproc_t; choices++)
  {
    if (choices->value == dscm)
      return (*(choices->proc)) (xdr, unp, LASTUNSIGNED);
  }

  /*
   * no match - execute the default xdr routine if there is one
   */
  return ((dfault == NULL_xdrproc_t) ? FALSE :
          (*dfault) (xdr, unp, LASTUNSIGNED));
} /* xdr_union */

/*
 * Non-portable xdr primitives.
 * Care should be taken when moving these routines to new architectures.
 */

/*
 * XDR null terminated ASCII strings
 * xdr_string deals with "C strings" - arrays of bytes that are
 * terminated by a NULL character.  The parameter cpp references a
 * pointer to storage; If the pointer is null, then the necessary
 * storage is allocated.  The last parameter is the max allowed length
 * of the string as specified by a protocol.
 */
bool_t oncrpc_xdr_string
(
  XDR *xdr,
  char **cpp,
  u_int maxsize
)
{
  char *sp = *cpp;	/* sp is the actual string pointer */
  u_int size;
  u_int nodesize;

  /*
   * first deal with the length since xdr strings are counted-strings
   */
  switch ( xdr->x_op ) {
    case XDR_FREE:
      if ( sp == NULL )
      {
        return TRUE;		/* already free */
      }
      /* fall through... */
    case XDR_ENCODE:
      if ( sp == NULL )
	return FALSE;
      size = strlen ( sp );
      break;
    case XDR_DECODE:
      break;

    default:
      ERR_FATAL( "invalid x_op %x", xdr->x_op, 0, 0 );
      break;
  }
  
  if ( !xdr_u_int ( xdr, &size ) )
  {
    return FALSE;
  }
  if (size > maxsize)
  {
    return FALSE;
  }
  nodesize = size + 1;

  /*
   * now deal with the actual bytes
   */
  switch (xdr->x_op) {
    case XDR_DECODE:
      if (nodesize == 0)
      {
        return TRUE;
      }
      if (sp == NULL)
      {
        *cpp = sp = (char *) oncrpcxdr_mem_alloc (xdr, nodesize);

	if (sp == NULL) {
	  ERR_FATAL( "xdr_string: oncrpcxdr_mem_alloc returned NULL!", 0, 0, 0);
	  break;
	}
      }

      sp[size] = 0;
      /* fall into ... */

    case XDR_ENCODE:
      return xdr_opaque (xdr, sp, size);

    case XDR_FREE:
      sp = NULL;
      *cpp = NULL;
      return TRUE;

    default:
      ERR_FATAL( "invalid x_op %x", xdr->x_op, 0, 0 );
      break;
  }
  return FALSE;
} /* xdr_string */

/*
 * Wrapper for xdr_string that can be called directly from
 * routines like clnt_call
 */
bool_t xdr_wrapstring 
(
  XDR *xdr,
  char **cpp
)
{
  if ( xdr_string ( xdr, cpp, LASTUNSIGNED ) )
  {
    return TRUE;
  }

  return FALSE;
} /* xdr_wrapstring */
 
/*===========================================================================
FUNCTION: xdr_send_accepted_reply_header

DESCRIPTION:  XDR routine for encoding the part of the RPC reply message that
              describes an accepted RPC calls.
              NOTE: In case of RPC call success, the RPC call's results are
              left for the server to encode separately after this routine is
              done.

ARGUMENTS: 
   xdr        The XDR transport that the RPC server is using to decode/encode
              the RPC messages.
   accreply   Pointer to the structure describing the reply to an accepted RPC
              call.

RETURN VALUE: 
   TRUE if the structure was encoded successfully into the RPC message. FALSE
   otherwise.

SIDE EFFECTS: 
   None
===========================================================================*/
static boolean xdr_send_accepted_reply_header
(
  xdr_s_type                              *xdr,
  struct rpc_accepted_reply_header const  *accreply
)
{
  if ( ! xdr_send_auth( xdr, &accreply->verf ) ) {
    return (FALSE);
  }

  if ( ! XDR_SEND_ENUM( xdr, &accreply->stat ) ) {
    return (FALSE);
  }

  switch ( (*accreply).stat ){
    case RPC_PROG_MISMATCH:

      if ( ! XDR_SEND_UINT32( xdr, &accreply->u.versions.low ) ) {
        return (FALSE);
      }

      if ( ! XDR_SEND_UINT32( xdr, &accreply->u.versions.high ) ) {
        return (FALSE);
      }

      break;

    case RPC_ACCEPT_SUCCESS:
    case RPC_PROG_UNAVAIL:
    case RPC_PROC_UNAVAIL:
    case RPC_GARBAGE_ARGS:
    case RPC_SYSTEM_ERR:
    case RPC_PROG_LOCKED:
      // case ignored
      break;

    default:
      return (FALSE);
  }

  return (TRUE);
} /* xdr_send_accepted_reply_header */

/*===========================================================================
FUNCTION: xdr_recv_accepted_reply_header

DESCRIPTION:  XDR routine for decoding the part of the RPC reply message that
              describes an accepted RPC call.
              NOTE: In case of RPC call success, the function results are
              left for the client to receive separately after this routine is
              done.

DEPENDENCIES: The "verf" field of the reply header structure must be
              initialized with the correct base address of where the auth data
              should be stored.

ARGUMENTS: 
   xdr        The XDR transport that the RPC server is using to decode/encode
              the RPC messages.
   accreply   Pointer to the structure where the part of the RPC reply message
              that describes the reply to an accepted RPC call must be stored.

RETURN VALUE: 
   TRUE if the reply msg was successfully decoded, FALSE otherwise.

SIDE EFFECTS: 
   None
===========================================================================*/
static boolean
xdr_recv_accepted_reply_header
(
  xdr_s_type                        *xdr,
  struct rpc_accepted_reply_header  *accreply
)
{
  if ( ! xdr_recv_auth( xdr, &accreply->verf ) ) {
    return (FALSE);
  }

  if ( ! XDR_RECV_ENUM( xdr, &accreply->stat ) ) {
    return (FALSE);
  }

  switch ( (*accreply).stat ){
    case RPC_PROG_MISMATCH:

      if ( ! XDR_RECV_UINT32( xdr, &accreply->u.versions.low ) ) {
        return (FALSE);
      }

      if ( ! XDR_RECV_UINT32( xdr, &accreply->u.versions.high ) ) {
        return (FALSE);
      }

      break;
    case RPC_ACCEPT_SUCCESS:
    case RPC_PROG_UNAVAIL:
    case RPC_PROC_UNAVAIL:
    case RPC_GARBAGE_ARGS:
    case RPC_SYSTEM_ERR:
    case RPC_PROG_LOCKED:
      // case ignored
      break;

    default:
      return (FALSE);
  }

  return (TRUE);
} /* xdr_recv_accepted_reply_header */

/*===========================================================================
FUNCTION: xdr_free_accepted_reply_header

DESCRIPTION:  Frees data that was allocated when receiving an RPC accepted
              reply using xdr_recv_accepted_reply_header.

DEPENDENCIES: The contents of accreply must be the result of a call to
              xdr_recv_accepted_reply_header.

ARGUMENTS:    Pointer to the structure where the part of the RPC reply message
              that describes the reply to an accepted RPC call is stored.

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
static void xdr_free_accepted_reply_header
(
  struct rpc_accepted_reply_header  *accreply
)
{
  xdr_free_auth( &accreply->verf );
} /* xdr_free_accepted_reply_header */

/*===========================================================================
FUNCTION: xdr_send_denied_reply 

DESCRIPTION:  XDR routine for encoding the part of the RPC reply message that
              describes a denied RPC calls.

ARGUMENTS: 
   xdr        The XDR transport that the RPC server is using to decode/encode
              the RPC messages.
   rejreply   Pointer to the structure describing the reply to a denied RPC
              call.

RETURN VALUE: 
   TRUE if the structure was encoded successfully into the RPC message. FALSE
   otherwise.

SIDE EFFECTS: 
   None
===========================================================================*/

static boolean xdr_send_denied_reply
(
  xdr_s_type                     *xdr,
  struct rpc_denied_reply const  *rejreply
)
{
  if ( ! XDR_SEND_ENUM( xdr, &rejreply->stat ) ) {
    return (FALSE);
  }

  switch ( (*rejreply).stat ){
    case RPC_MISMATCH:

      if ( ! XDR_SEND_UINT32( xdr, &rejreply->u.versions.low ) ) {
        return (FALSE);
      }

      if ( ! XDR_SEND_UINT32( xdr, &rejreply->u.versions.high ) ) {
        return (FALSE);
      }

      break;
    case RPC_AUTH_ERROR:

      if ( ! XDR_SEND_ENUM( xdr, &rejreply->u.why ) ) {
        return (FALSE);
      }

      break;
    default:
      return (FALSE);
  }

  return (TRUE);
} /* xdr_send_denied_reply */

/*===========================================================================
FUNCTION: xdr_recv_denied_reply

DESCRIPTION:  XDR routine for decoding the part of the RPC reply message that
              describes a denied RPC call.

ARGUMENTS: 
   xdr        The XDR transport that the RPC server is using to decode/encode
              the RPC messages.
   accreply   Pointer to the structure where the part of the RPC reply message
              that describes the reply to a denied RPC call must be stored.

RETURN VALUE: 
   TRUE if the reply msg was successfully decoded, FALSE otherwise.

SIDE EFFECTS: 
   None
===========================================================================*/
static boolean xdr_recv_denied_reply
(
  xdr_s_type               *xdr,
  struct rpc_denied_reply  *rejreply
)
{
  if ( ! XDR_RECV_ENUM( xdr, &rejreply->stat ) ) {
    return (FALSE);
  }

  switch ( (*rejreply).stat ){
    case RPC_MISMATCH:

      if ( ! XDR_RECV_UINT32( xdr, &rejreply->u.versions.low ) ) {
        return (FALSE);
      }

      if ( ! XDR_RECV_UINT32( xdr, &rejreply->u.versions.high ) ) {
        return (FALSE);
      }

      break;
    case RPC_AUTH_ERROR:

      if ( ! XDR_RECV_ENUM( xdr, &rejreply->u.why ) ) {
        return (FALSE);
      }

      break;
    default:

      return (FALSE);
  }

  return (TRUE);
} /* xdr_recv_denied_reply */

/*===========================================================================
FUNCTION: xdr_send_reply_header

DESCRIPTION: This function sends the msg header for a successful RPC call's
             reply. In case the server encounters an error and an error reply
             is sent, the entire body of the reply message is sent by this
             function.

DEPENDENCIES: Must be called after XDR_MSG_START has been called with with
              RPC_MSG_REPLY as the message type.

ARGUMENTS: 
   xdr       XDR transport that this message should be sent through.
   reply     Pointer to the rpc reply header structure.

RETURN VALUE: 
   TRUE if reply header structure was properly formatted and was successfully
   sent. FALSE otherwise.

SIDE EFFECTS: 
   None
===========================================================================*/
boolean xdr_send_reply_header
(
  xdr_s_type              *xdr,
  rpc_reply_header const  *reply
)
{
  if ( ! XDR_SEND_ENUM( xdr, &reply->stat ) ) {
    return (FALSE);
  }

  switch ( (*reply).stat ){
    case RPC_MSG_ACCEPTED:

      if ( ! xdr_send_accepted_reply_header( xdr, &reply->u.ar ) ) {
        return (FALSE);
      }

      break;
    case RPC_MSG_DENIED:

      if ( ! xdr_send_denied_reply( xdr, &reply->u.dr ) ) {
        return (FALSE);
      }

      break;
    default:
      return (FALSE);
  }

  return (TRUE);
} /* xdr_send_reply_header */

/*===========================================================================
FUNCTION: xdr_recv_reply_header

DESCRIPTION:  This function decodes the msg header for a successful RPC call's
              reply. In case the server encountered an error and sent back an
              error reply, the entire body of the reply message will be decoded
              by this function.

              The decoded data will be the full body of a service error reply
              if:
                  ( reply->stat      != RPC_MSG_ACCEPTED ||
                    reply->u.ar.stat != RPC_ACCEPT_SUCCESS )

DEPENDENCIES: The "verf" field of the reply header structure must be
              initialized with the correct base address of where the auth data
              should be stored.

              Must be called from client task after sending the RPC message
              and receiving the signal from the RPC task indicating the reply
              msg has arrived and is ready to be decoded.

ARGUMENTS:
   xdr        The XDR transport that the RPC client is using to decode/encode
              the RPC messages.
   reply      Pointer to the rpc reply header structure where the decoded data
              must be stored.

RETURN VALUE: 
   TRUE if data has been received successfully.
   FALSE otherwise.

SIDE EFFECTS: 
   None
===========================================================================*/
boolean xdr_recv_reply_header
(
  xdr_s_type        *xdr,
  rpc_reply_header  *reply
)
{
  if ( ! XDR_RECV_ENUM( xdr, &reply->stat ) ) {
    return (FALSE);
  }

  switch ( (*reply).stat ){
    case RPC_MSG_ACCEPTED:

      if ( ! xdr_recv_accepted_reply_header( xdr, &reply->u.ar ) ) {
        return (FALSE);
      }

      break;
    case RPC_MSG_DENIED:

      if ( ! xdr_recv_denied_reply( xdr, &reply->u.dr ) ) {
        return (FALSE);
      }

      break;
    default:

      return (FALSE);
  }

  return (TRUE);
} /* xdr_recv_reply_header */

/*===========================================================================
FUNCTION: xdr_free_reply_header

DESCRIPTION:  This function frees the memory that was allocated for the auth.
              field of RPC replies.

DEPENDENCIES: Must be called by client right after receiving and processing
              the reply message.

ARGUMENTS: 
   reply      Pointer to the rpc reply header structure that has fields that
              need to be freed.

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void xdr_free_reply_header
(
  rpc_reply_header  *reply
)
{
  switch ( (*reply).stat ){
    case RPC_MSG_ACCEPTED:

      xdr_free_accepted_reply_header( &reply->u.ar );

      break;
    case RPC_MSG_DENIED:

      // Field u.dr doesn't need freeing of memory
      break;
    default:

      ERR_FATAL("xdr_free_reply_header: badly formatted reply", 0, 0, 0);
  }
} /* xdr_free_reply_header */
