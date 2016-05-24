#ifndef _RPC_XDR_H
#define _RPC_XDR_H 1
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        O N C R P C _ X D R . H

GENERAL DESCRIPTION

  This is the header file for the ONCRPC XDR.

  ONCRPC (Open Network Computing Remote Procedure Calls) is a way of
  calling functions from one machine to another.  This is described in
  detail in the following RFCs: 1831, 1832, 1833

INITIALIZATION AND SEQUENCING REQUIREMENTS

 Copyright (c) 2005-2008 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential. 
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/inc/oncrpc/oncrpc_xdr.h#1 $ 

when        who    what, where, why
--------    ---    ----------------------------------------------------------
08/18/09    rr     Redefine comon procedures with librpc to avoid conflicts
07/21/09    rr     Redefine xdr_call_msg_start and xdr_reply_msg_start 
                   to oncrpc_... to avoid conflicts with librpc
03/13/08    hn     Changed xdr_enum to a macro that passes the enum pointer
                   in addition to a dereference of 'objp' to xdr_enum_ext
02/20/08    hn     Added xdr boolean routines.
02/20/08    hn     Fixed the xdr_send_packed_enum routine to pass by value.
10/31/07    hn     Fixed the xdr_send_enum routine to pass by value as int32.
10/31/07    taw    Make XDR_UNKNOWN_DISCRIMINATOR_MSG an empty macro for WinCE
                   for now.
10/17/07    hn     Got rid of redundant NULL checks after xdr_mem_alloc.
10/17/07    hn     Added XDR_UNKNOWN_DISCRIMINATOR_MSG macro.
09/24/07    hn     Suppressed 'macro defined with arguments' warning in Lint.
08/22/07    hn     Removed references to oncrpc_thread_handle_get.
07/10/07    ptm    Remove featurization.
07/07/07    hn     Reverted change to union macros because it changed the api,
                   created new macros instead.
07/05/07    hn     Do malloc/memset on a temp variable first since final
                   pointer could be a pointer to const.
07/02/07    hn     Several required changes to fix lint errors in glue code.
06/13/07    hn     Added macros to print error messages using common strings.
05/07/07    hn     Added new versions of the union pointer XDR macros.
01/18/07    hn     Added XDR macros for float and double.
10/03/06    hn     Removed xport_running op from transports.
08/29/06    hn     Support handling error checks for clients automatically.
03/03/06    ptm    Fix WinCE compiler warnings.
01/25/06    ptm    Changed xdr_msg_set_reply to xdr_msg_set_input.
10/12/05    clp    Move call back related prototypes to oncrpc_cb.h.
05/27/05    clp    Added get_port functionality.
05/23/05    hn     Added oncrpcxdr_mem_alloc and oncrpcxdr_mem_free routines
05/17/05    hn     Added missing variants of 64 bit integer xdr routines
05/16/05    hn     Add 64-bit XDR support in ONCRPC.
04/22/05    ptm    Add XPRT_RUNNING API and update the XDR_MSG_SEND API.
04/13/05    ptm    Remove prototypes for reply queue APIs.
04/01/05    clp    Include header cleanup changes and add reply xdr routines.
03/22/05    clp    Changes for nonblocking clnt call, including clone.
03/22/05    hn     Added "const" qualifier to parameter in xdr_send routines
                   for enums, packed enums and auth.        
                   Added xdr_free_auth() routine.
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
 * xdr.h, External Data Representation Serialization Routines.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

/* Defines to resolve conficts between librpc and oncrpc */

#define xdr_call_msg_start       oncrpc_xdr_call_msg_start
#define xdr_reply_msg_start      oncrpc_xdr_reply_msg_start
#define xdr_send_reply_header    oncrpc_xdr_send_reply_header
#define xdr_send_auth            oncrpc_xdr_send_auth
#define xdr_send_enum            oncrpc_xdr_send_enum
#define xdr_recv_enum            oncrpc_xdr_recv_enum
#define xdr_bytes                oncrpc_xdr_bytes
#define xdr_enum_ext             oncrpc_xdr_enum_ext
#define xdr_pointer              oncrpc_xdr_pointer
#define xdr_int                  oncrpc_xdr_int
#define xdr_u_int                oncrpc_xdr_u_int
#define xdr_char                 oncrpc_xdr_char
#define xdr_u_char               oncrpc_xdr_u_char
#define xdr_long                 oncrpc_xdr_long
#define xdr_u_long               oncrpc_xdr_u_long
#define xdr_quad_t               oncrpc_xdr_quad_t
#define xdr_u_quad_t             oncrpc_xdr_u_quad_t
#define xdr_short                oncrpc_xdr_short
#define xdr_u_short              oncrpc_xdr_u_short
#define xdr_vector               oncrpc_xdr_vector
#define xdr_void                 oncrpc_xdr_void
#define xdr_opaque               oncrpc_xdr_opaque
#define xdr_string               oncrpc_xdr_string
#define xdr_array                oncrpc_xdr_array
#define xdr_bool                 oncrpc_xdr_bool

#ifdef FEATURE_WINCE
#define ONCRPC_ERR(fmt, a, b, c)        printf(fmt, a, b, c)
#else
#define ONCRPC_ERR(fmt, a, b, c)        ERR(fmt, a, b, c)
#endif
#define	XDR_INLINE(xdrs, len) NULL
#define	xdr_inline(xdrs, len) NULL
/*
 * Inline routines for fast encode/decode of primitive data types.
 * Caveat emptor: these use single memory cycles to get the
 * data from the underlying buffer, and will fail to operate
 * properly if the data is not aligned.  The standard way to use these
 * is to say:
 *      if ((buf = XDR_INLINE(xdrs, count)) == NULL)
 *              return (FALSE);
 *      <<< macro calls >>>
 * where ``count'' is the number of bytes of data occupied
 * by the primitive data types.
 *
 * N.B. and frozen for all time: each data type here uses 4 bytes
 * of external representation.
 */

#define IXDR_GET_INT32(buf)        ((buf)++,(int32_t)ntohl((uint32_t)*(buf-1)))
#define IXDR_PUT_INT32(buf, v)        (*(buf)++ = (int32_t)htonl((uint32_t)(v)))
#define IXDR_GET_U_INT32(buf)         ((uint32_t)IXDR_GET_INT32(buf))
#define IXDR_PUT_U_INT32(buf, v)      IXDR_PUT_INT32(buf, (int32_t)(v))

/* WARNING: The IXDR_*_LONG defines are removed by Sun for new platforms
 * and shouldn't be used any longer. Code which use this defines or longs
 * in the RPC code will not work on 64bit Solaris platforms !
 */
#define IXDR_GET_LONG(buf)            ((long)IXDR_GET_INT32(buf))
#define IXDR_PUT_LONG(buf, v) \
	(*((u_int32_t*)((buf)++)) = (long)htonl((u_long)(v)))

#define IXDR_GET_U_LONG(buf)	      ((u_long)IXDR_GET_LONG(buf))
#define IXDR_PUT_U_LONG(buf, v)	      IXDR_PUT_LONG(buf, (long)(v))


#define IXDR_GET_BOOL(buf)            ((bool_t)IXDR_GET_LONG(buf))
#define IXDR_GET_ENUM(buf, t)         ((t)IXDR_GET_LONG(buf))
#define IXDR_GET_SHORT(buf)           ((short)IXDR_GET_LONG(buf))
#define IXDR_GET_U_SHORT(buf)         ((u_short)IXDR_GET_LONG(buf))

#define IXDR_PUT_BOOL(buf, v)         IXDR_PUT_LONG(buf, (long)(v))
#define IXDR_PUT_ENUM(buf, v)         IXDR_PUT_LONG(buf, (long)(v))
#define IXDR_PUT_SHORT(buf, v)        IXDR_PUT_LONG(buf, (long)(v))
#define IXDR_PUT_U_SHORT(buf, v)      IXDR_PUT_LONG(buf, (long)(v))

/*===========================================================================
  Defining the XPORT access macros
  ===========================================================================*/
#define XPORT_READ_DSM( XPORT, ITEM )  (XPORT)->ops->read_dsm( XPORT, ITEM )
#define XPORT_READ( XPORT, BUF, LEN )  (XPORT)->ops->read( XPORT, BUF, LEN )
#define XPORT_WRITE( XPORT, BUF, LEN ) (XPORT)->ops->write( XPORT, BUF, LEN )
#define XPORT_WRITE_DSM( XPORT, ITEM ) (XPORT)->ops->write_dsm( XPORT, ITEM )
#define XPORT_FLUSH_OUTPUT( XPORT )    (XPORT)->ops->flush_output( XPORT )
#define XPORT_FLUSH_INPUT( XPORT )     (XPORT)->ops->flush_input( XPORT )
#define XPORT_GET_PORT( XPORT )        (XPORT)->ops->get_port( XPORT )
#define XPORT_DESTROY( XPORT )         (XPORT)->ops->destroy( XPORT )
#define XPORT_CONTROL(XPORT,REQ,INFO)  (XPORT)->ops->control( XPORT, REQ, INFO)

/*===========================================================================
  Macros for calling primitive xdr routines
  ===========================================================================*/
#define XDR_DESTROY( XDR )            (XDR)->xops->xdr_destroy( XDR )
#define XDR_CONTROL( XDR, REQ, INFO ) (XDR)->xops->xdr_control( XDR, REQ, INFO )
#define XDR_CLONE( XDR )              (XDR)->xops->xdr_clone( XDR )
#define XDR_ERRCHK( XDR, ON )         (XDR)->xops->xdr_errchk( XDR, ON )

#define XDR_READ( XDR )                 (XDR)->xops->read( XDR )
#define XDR_MSG_SET_INPUT( XDR, ITEM )  (XDR)->xops->msg_set_input( XDR, ITEM )
#define XDR_MSG_DONE( XDR )             (XDR)->xops->msg_done( XDR )


#define XDR_MSG_START( XDR, TYPE )      (XDR)->xops->msg_start( XDR, TYPE )
#define XDR_MSG_ABORT( XDR )            (XDR)->xops->msg_abort( XDR )
#define XDR_MSG_SEND( XDR, REPLY )      (XDR)->xops->msg_send( XDR, REPLY )
#define XDR_MSG_SEND_NONBLOCKING( XDR, CB, DATA ) \
        (XDR)->xops->msg_send_nonblocking( XDR, CB, DATA )

#define XDR_SEND_CHAR( XDR, VALUE ) \
        (XDR)->xops->send_uint8( XDR, (uint8 *)(VALUE) )
#define XDR_SEND_STRING( XDR, BUF, LEN ) \
        (XDR)->xops->send_bytes( XDR, (uint8 *)(BUF), LEN )
#define XDR_SEND_INT8( XDR, VALUE )     (XDR)->xops->send_int8( XDR, VALUE )
#define XDR_SEND_UINT8( XDR, VALUE )    (XDR)->xops->send_uint8( XDR, VALUE )
#define XDR_SEND_INT16( XDR, VALUE )    (XDR)->xops->send_int16( XDR, VALUE )
#define XDR_SEND_UINT16( XDR, VALUE )   (XDR)->xops->send_uint16( XDR, VALUE )
#define XDR_SEND_INT32( XDR, VALUE )    (XDR)->xops->send_int32( XDR, VALUE )
#define XDR_SEND_UINT32( XDR, VALUE )   (XDR)->xops->send_uint32( XDR, VALUE )
#define XDR_SEND_INT64( XDR, VALUE )    xdr_send_int64( XDR, VALUE )
#define XDR_SEND_UINT64( XDR, VALUE )   xdr_send_uint64( XDR, VALUE )
#define XDR_SEND_BYTES( XDR, BUF, LEN ) (XDR)->xops->send_bytes( XDR, BUF, LEN )
#define XDR_SEND_DSM( XDR, VALUE )      (XDR)->xops->send_dsm( XDR, VALUE )

#define XDR_RECV_CHAR( XDR, VALUE ) \
        (XDR)->xops->recv_uint8( XDR, (uint8 *)(VALUE) )
#define XDR_RECV_STRING( XDR, BUF, LEN ) \
        (XDR)->xops->recv_bytes( XDR, (uint8 *)(BUF), LEN )
#define XDR_RECV_INT8( XDR, VALUE )     (XDR)->xops->recv_int8( XDR, VALUE )
#define XDR_RECV_UINT8( XDR, VALUE )    (XDR)->xops->recv_uint8( XDR, VALUE )
#define XDR_RECV_INT16( XDR, VALUE )    (XDR)->xops->recv_int16( XDR, VALUE )
#define XDR_RECV_UINT16( XDR, VALUE )   (XDR)->xops->recv_uint16( XDR, VALUE )
#define XDR_RECV_INT32( XDR, VALUE )    (XDR)->xops->recv_int32( XDR, VALUE )
#define XDR_RECV_UINT32( XDR, VALUE )   (XDR)->xops->recv_uint32( XDR, VALUE )
#define XDR_RECV_INT64( XDR, VALUE )    xdr_recv_int64( XDR, VALUE )
#define XDR_RECV_UINT64( XDR, VALUE )   xdr_recv_uint64( XDR, VALUE )
#define XDR_RECV_BYTES( XDR, BUF, LEN ) (XDR)->xops->recv_bytes( XDR, BUF, LEN )
#define XDR_RECV_DSM( XDR, VALUE )      (XDR)->xops->recv_dsm( XDR, VALUE )

#define XDR_SEND_INT( XDR, VALUE ) \
        (XDR)->xops->send_int32( XDR, (int32 *)(VALUE) )
#define XDR_RECV_INT( XDR, VALUE ) \
        (XDR)->xops->recv_int32( XDR, (int32 *)(VALUE) )

#define XDR_SEND_UINT( XDR, VALUE ) \
        (XDR)->xops->send_uint32( XDR, (uint32 *)(VALUE) )
#define XDR_RECV_UINT( XDR, VALUE ) \
        (XDR)->xops->recv_uint32( XDR, (uint32 *)(VALUE) )

#define XDR_SEND_HANDLE( XDR, VALUE ) \
        (XDR)->xops->send_uint32( XDR, (uint32 *)(VALUE) )
#define XDR_RECV_HANDLE( XDR, VALUE ) \
        (XDR)->xops->recv_uint32( XDR, (uint32 *)(VALUE) )

#define XDR_SEND_ENUM( XDR, VALUE ) \
        xdr_send_enum( XDR, (int32)(*(VALUE)))
#define XDR_RECV_ENUM( XDR, VALUE ) \
        xdr_recv_enum( XDR, (void *) (VALUE), sizeof(*(VALUE)))

#define XDR_SEND_BOOLEAN( XDR, VALUE ) \
        xdr_send_boolean( XDR, VALUE )
#define XDR_RECV_BOOLEAN( XDR, VALUE ) \
        xdr_recv_boolean( XDR, VALUE )

/*
 * PACKED versions of the macro's are used when the data being sent is at an
 * unaligned address. These macros call helper routines.
 */
#define XDR_SEND_PACKED_CHAR( XDR, VALUE ) \
        xdr_send_packed_uint8( XDR, (PACKED uint8 *)(VALUE) PACKED_POST )
#define XDR_SEND_PACKED_STRING XDR_SEND_STRING
#define XDR_SEND_PACKED_INT8( XDR, VALUE ) \
        xdr_send_packed_int8( XDR, VALUE )
#define XDR_SEND_PACKED_UINT8( XDR, VALUE ) \
        xdr_send_packed_uint8( XDR, VALUE )
#define XDR_SEND_PACKED_INT16( XDR, VALUE ) \
        xdr_send_packed_int16( XDR, VALUE )
#define XDR_SEND_PACKED_UINT16( XDR, VALUE ) \
        xdr_send_packed_uint16( XDR, VALUE )
#define XDR_SEND_PACKED_INT32( XDR, VALUE ) \
        xdr_send_packed_int32( XDR, VALUE )
#define XDR_SEND_PACKED_UINT32( XDR, VALUE ) \
        xdr_send_packed_uint32( XDR, VALUE )
#define XDR_SEND_PACKED_INT64( XDR, VALUE ) \
        xdr_send_packed_int64( XDR, VALUE )
#define XDR_SEND_PACKED_UINT64( XDR, VALUE ) \
        xdr_send_packed_uint64( XDR, VALUE )
#define XDR_SEND_PACKED_BYTES( XDR, BUF, LEN ) \
        (XDR)->xops->send_bytes( XDR, (uint8 *)(BUF), LEN )

#define XDR_SEND_PACKED_INT( XDR, VALUE ) \
        xdr_send_packed_int32( XDR, (PACKED int32 *)(VALUE) PACKED_POST )

#define XDR_SEND_PACKED_UINT( XDR, VALUE ) \
        xdr_send_packed_uint32( XDR, (PACKED uint32 *)(VALUE) PACKED_POST )

#define XDR_SEND_PACKED_HANDLE( XDR, VALUE ) \
        xdr_send_packed_uint32( XDR, (PACKED uint32 *)(VALUE) PACKED_POST )

#define XDR_SEND_PACKED_ENUM( XDR, VALUE ) \
        xdr_send_packed_enum( XDR, (int32) (*(VALUE)) ) 

#define XDR_SEND_PACKED_BOOLEAN( XDR, VALUE ) \
        xdr_send_packed_boolean( XDR, VALUE  ) 

/*
 * PACKED versions of the macro's are used when the destination for the data
 * being received is an unalligned address. These macros call helper
 * routines
 */
#define XDR_RECV_PACKED_CHAR( XDR, VALUE ) \
        xdr_recv_packed_uint8( XDR, (PACKED uint8 *)(VALUE) PACKED_POST )
#define XDR_RECV_PACKED_STRING XDR_RECV_STRING
#define XDR_RECV_PACKED_INT8( XDR, VALUE ) \
        xdr_recv_packed_int8( XDR, VALUE )
#define XDR_RECV_PACKED_UINT8( XDR, VALUE ) \
        xdr_recv_packed_uint8( XDR, VALUE )
#define XDR_RECV_PACKED_INT16( XDR, VALUE ) \
        xdr_recv_packed_int16( XDR, VALUE )
#define XDR_RECV_PACKED_UINT16( XDR, VALUE ) \
        xdr_recv_packed_uint16( XDR, VALUE )
#define XDR_RECV_PACKED_INT32( XDR, VALUE ) \
        xdr_recv_packed_int32( XDR, VALUE )
#define XDR_RECV_PACKED_UINT32( XDR, VALUE ) \
        xdr_recv_packed_uint32( XDR, VALUE )
#define XDR_RECV_PACKED_INT64( XDR, VALUE ) \
        xdr_recv_packed_int64( XDR, VALUE )
#define XDR_RECV_PACKED_UINT64( XDR, VALUE ) \
        xdr_recv_packed_uint64( XDR, VALUE )
#define XDR_RECV_PACKED_BYTES( XDR, BUF, LEN ) \
        (XDR)->xops->recv_bytes( XDR, (uint8 *)(BUF), LEN )

#define XDR_RECV_PACKED_INT( XDR, VALUE ) \
        xdr_recv_int32( XDR, (PACKED int32 *)(VALUE) PACKED_POST )

#define XDR_RECV_PACKED_UINT( XDR, VALUE ) \
        xdr_recv_uint32( XDR, (PACKED uint32 *)(VALUE) PACKED_POST )

#define XDR_RECV_PACKED_HANDLE( XDR, VALUE ) \
        xdr_recv_uint32( XDR, (PACKED uint32 *)(VALUE) PACKED_POST )

#define XDR_RECV_PACKED_ENUM( XDR, VALUE ) \
        xdr_recv_packed_enum( XDR, \
                              (PACKED void *) (VALUE) PACKED_POST, \
                              sizeof(*(VALUE)) )

#define XDR_RECV_PACKED_BOOLEAN( XDR, VALUE ) \
        xdr_recv_packed_boolean( XDR, VALUE )

/*===========================================================================
  Macros for sending/receiving pointers
  ===========================================================================*/

#define XDR_SEND_POINTER( XDR, OBJPP, XDR_SEND_OBJ, RESULT) \
do { \
  /*lint --e{123} suppress macro defined with arguments warning */ \
  uint32 _more_data_ = ( *OBJPP != NULL ); \
  RESULT = TRUE;\
  if ( ! XDR_SEND_UINT32( XDR, &_more_data_ ) ) { \
    RESULT = FALSE; \
  } \
  else if ( _more_data_ ) { \
    if ( ! XDR_SEND_OBJ( XDR, *OBJPP) ) { \
      RESULT = FALSE; \
    } \
  } \
} while ( 0 )

#define XDR_SEND_POINTER_NO_ERRCHK( XDR, OBJPP, XDR_SEND_OBJ) \
do { \
  /*lint --e{123} suppress macro defined with arguments warning */ \
  uint32 _more_data_ = ( *OBJPP != NULL ); \
  if ( XDR_SEND_UINT32( XDR, &_more_data_ ) && _more_data_ ) { \
    (void) XDR_SEND_OBJ( XDR, *OBJPP); \
  } \
} while ( 0 )

#define XDR_RECV_POINTER( XDR, OBJPP, XDR_RECV_OBJ, RESULT ) \
do { \
  /*lint --e{123} suppress macro defined with arguments warning */ \
  uint32  _more_data_; \
  void   *_temp_; \
  RESULT = TRUE; \
  if ( ! XDR_RECV_UINT32( XDR, &_more_data_ ) ) { \
    RESULT = FALSE; \
  } \
  else if ( ! _more_data_ ) { \
    *OBJPP = NULL; \
  } \
  else if ( *OBJPP == NULL ) { \
    _temp_ = oncrpcxdr_mem_alloc( XDR, sizeof(**OBJPP) ); \
    memset( _temp_, 0, sizeof( **OBJPP ) ); \
    *OBJPP = _temp_; \
    if ( ! XDR_RECV_OBJ( XDR, *OBJPP ) ) { \
      RESULT = FALSE; \
    } \
  } \
  else { \
    if( !XDR_RECV_OBJ( XDR, *OBJPP ) ) { \
      RESULT = FALSE;  \
    } \
  } \
} while ( 0 ) 

#define XDR_RECV_POINTER_NO_ERRCHK( XDR, OBJPP, XDR_RECV_OBJ ) \
do { \
  /*lint --e{123} suppress macro defined with arguments warning */ \
  uint32  _more_data_; \
  void   *_temp_; \
  (void) XDR_RECV_UINT32( XDR, &_more_data_ ); \
  if ( ! _more_data_ ) { \
    *OBJPP = NULL; \
  } \
  else if ( *OBJPP == NULL ) { \
    _temp_ = oncrpcxdr_mem_alloc( XDR, sizeof(**OBJPP) ); \
    memset( _temp_, 0, sizeof( **OBJPP ) ); \
    *OBJPP = _temp_; \
    (void) XDR_RECV_OBJ( XDR, *OBJPP ); \
  } \
  else { \
    (void) XDR_RECV_OBJ( XDR, *OBJPP ); \
  } \
} while ( 0 ) 

#define XDR_SEND_S_FIELD_POINTER( XDR, OBJPP, SPTR, XDR_SEND_OBJ, RESULT) \
do { \
  /*lint --e{123} suppress macro defined with arguments warning */ \
  uint32 _more_data_ = ( *OBJPP != NULL ); \
  RESULT = TRUE; \
  if ( ! XDR_SEND_UINT32( XDR, &_more_data_ ) ) { \
    RESULT = FALSE; \
  } \
  else if ( _more_data_ ) { \
    if ( ! XDR_SEND_OBJ( XDR, *OBJPP, SPTR) ) { \
      RESULT = FALSE; \
    } \
  } \
} while ( 0 )

#define XDR_SEND_S_FIELD_POINTER_NO_ERRCHK( XDR, OBJPP, SPTR, XDR_SEND_OBJ) \
do { \
  /*lint --e{123} suppress macro defined with arguments warning */ \
  uint32 _more_data_ = ( *OBJPP != NULL ); \
  if ( XDR_SEND_UINT32( XDR, &_more_data_ ) && _more_data_ ) { \
    (void) XDR_SEND_OBJ( XDR, *OBJPP, SPTR); \
  } \
} while ( 0 )

#define XDR_RECV_S_FIELD_POINTER( XDR, OBJPP, SPTR, XDR_RECV_OBJ, RESULT ) \
do { \
  /*lint --e{123} suppress macro defined with arguments warning */ \
  uint32  _more_data_; \
  void   *_temp_; \
  RESULT = TRUE; \
  if ( ! XDR_RECV_UINT32( XDR, &_more_data_ ) ) { \
    RESULT = FALSE; \
  } \
  else if ( ! _more_data_ ) { \
    *OBJPP = NULL; \
  } \
  else if ( *OBJPP == NULL ) { \
    _temp_ = oncrpcxdr_mem_alloc( XDR, sizeof(**OBJPP) ); \
    memset( _temp_, 0, sizeof( **OBJPP ) ); \
    *OBJPP = _temp_; \
    if ( ! XDR_RECV_OBJ( XDR, *OBJPP, SPTR ) ) { \
      RESULT = FALSE; \
    } \
  } \
  else { \
    if( ! XDR_RECV_OBJ( XDR, *OBJPP, SPTR ) ) { \
      RESULT = FALSE;  \
    } \
  } \
} while ( 0 ) 

#define XDR_RECV_S_FIELD_POINTER_NO_ERRCHK( XDR, OBJPP, SPTR, XDR_RECV_OBJ ) \
do { \
  /*lint --e{123} suppress macro defined with arguments warning */ \
  uint32  _more_data_; \
  void   *_temp_; \
  (void) XDR_RECV_UINT32( XDR, &_more_data_ ); \
  if ( ! _more_data_ ) { \
    *OBJPP = NULL; \
  } \
  else if ( *OBJPP == NULL ) { \
    _temp_ = oncrpcxdr_mem_alloc( XDR, sizeof(**OBJPP) ); \
    memset( _temp_, 0, sizeof( **OBJPP ) ); \
    *OBJPP = _temp_; \
    (void) XDR_RECV_OBJ( XDR, *OBJPP, SPTR ); \
  } \
  else { \
    (void) XDR_RECV_OBJ( XDR, *OBJPP, SPTR ); \
  } \
} while ( 0 ) 

#define XDR_SEND_U_POINTER2( XDR, OBJPP, DISC, XDR_SEND_OBJ, XDR_SEND_DISC, RESULT) \
do { \
  /*lint --e{123} suppress macro defined with arguments warning */ \
  uint32 _more_data_ = ( *OBJPP != NULL ); \
  RESULT = TRUE; \
  if ( ! XDR_SEND_UINT32( XDR, &_more_data_ ) ) { \
    RESULT = FALSE; \
  } \
  else if ( _more_data_ ) { \
    if ( ! XDR_SEND_DISC( XDR, DISC ) ) { \
      RESULT = FALSE; \
    } else if ( ! XDR_SEND_OBJ( XDR, *OBJPP, *DISC) ) { \
      RESULT = FALSE; \
    } \
  } \
} while ( 0 )

#define XDR_SEND_U_POINTER_NO_ERRCHK2( XDR, OBJPP, DISC, XDR_SEND_OBJ, XDR_SEND_DISC) \
do { \
  /*lint --e{123} suppress macro defined with arguments warning */ \
  uint32 _more_data_ = ( *OBJPP != NULL ); \
  if ( XDR_SEND_UINT32( XDR, &_more_data_ ) && _more_data_ ) { \
    if ( XDR_SEND_DISC( XDR, DISC ) ) { \
      (void) XDR_SEND_OBJ( XDR, *OBJPP, *DISC); \
    } \
  } \
} while ( 0 )

#define XDR_RECV_U_POINTER2( XDR, OBJPP, DISC, XDR_RECV_OBJ, XDR_RECV_DISC, RESULT ) \
do { \
  /*lint --e{123} suppress macro defined with arguments warning */ \
  uint32  _more_data_; \
  void   *_temp_; \
  RESULT = TRUE; \
  if ( ! XDR_RECV_UINT32( XDR, &_more_data_ ) ) { \
    RESULT = FALSE; \
  } \
  else if ( ! _more_data_ ) { \
    *OBJPP = NULL; \
  } \
  else if ( *OBJPP == NULL ) { \
    _temp_ = oncrpcxdr_mem_alloc( XDR, sizeof(**OBJPP) ); \
    memset( _temp_, 0, sizeof( **OBJPP ) ); \
    *OBJPP = _temp_; \
    if ( ! XDR_RECV_DISC( XDR, DISC ) ) { \
      RESULT = FALSE; \
    } else if ( ! XDR_RECV_OBJ( XDR, *OBJPP, *DISC ) ) { \
      RESULT = FALSE; \
    } \
  } \
  else { \
    if ( ! XDR_RECV_DISC( XDR, DISC ) ) { \
      RESULT = FALSE; \
    } else if( ! XDR_RECV_OBJ( XDR, *OBJPP, *DISC ) ) { \
      RESULT = FALSE;  \
    } \
  } \
} while ( 0 ) 

#define XDR_RECV_U_POINTER_NO_ERRCHK2( XDR, OBJPP, DISC, XDR_RECV_OBJ, XDR_RECV_DISC ) \
do { \
  /*lint --e{123} suppress macro defined with arguments warning */ \
  uint32  _more_data_; \
  void   *_temp_; \
  (void) XDR_RECV_UINT32( XDR, &_more_data_ ); \
  if ( ! _more_data_ ) { \
    *OBJPP = NULL; \
  } \
  else if ( *OBJPP == NULL ) { \
    _temp_ = oncrpcxdr_mem_alloc( XDR, sizeof(**OBJPP) ); \
    memset( _temp_, 0, sizeof( **OBJPP ) ); \
    *OBJPP = _temp_; \
    (void) XDR_RECV_DISC( XDR, DISC ); \
    (void) XDR_RECV_OBJ( XDR, *OBJPP, *DISC ); \
  } \
  else { \
    (void) XDR_RECV_DISC( XDR, DISC ); \
    (void) XDR_RECV_OBJ( XDR, *OBJPP, *DISC ); \
  } \
} while ( 0 ) 

/*===========================================================================
  Macros for sending/receiving floating-point values
  ===========================================================================*/
/*
 * This implementation of the floating-point XDR macros assumes the compiler
 * uses the IEEE 754 standard. In IEEE 754, the XDR representation of floating-
 * point numbers will match the XDR representation for int32 for float and int64
 * for double. In order to support formats other than IEEE 754, new separate
 * routines must be written to encode and decode the floating-point values and
 * relying on the existing int32 and int64 routines will no longer be sufficient
 */
#define XDR_SEND_FLOAT( XDR, VALUE )        XDR_SEND_INT32(XDR, (int32 *)(VALUE))
#define XDR_SEND_DOUBLE( XDR, VALUE )       XDR_SEND_INT64(XDR, (int64 *)(VALUE))
#define XDR_RECV_FLOAT( XDR, VALUE )        XDR_RECV_INT32(XDR, (int32 *)(VALUE))
#define XDR_RECV_DOUBLE( XDR, VALUE )       XDR_RECV_INT64(XDR, (int64 *)(VALUE))
#define XDR_SEND_PACKED_FLOAT( XDR, VALUE ) XDR_SEND_PACKED_INT32(XDR,\
                                                         (PACKED int32 *)(VALUE))
#define XDR_SEND_PACKED_DOUBLE( XDR,VALUE ) XDR_SEND_PACKED_INT64(XDR,\
                                                         (PACKED int64 *)(VALUE))
#define XDR_RECV_PACKED_FLOAT( XDR, VALUE ) XDR_RECV_PACKED_INT32(XDR,\
                                                         (PACKED int32 *)(VALUE))
#define XDR_RECV_PACKED_DOUBLE( XDR,VALUE ) XDR_RECV_PACKED_INT64(XDR,\
                                                         (PACKED int64 *)(VALUE))

/*===========================================================================
  Macros for producing error messages
  ===========================================================================*/
extern  const char xdr_clnt_lookup_err_msg[];
#define XDR_CLNT_LOOKUP2_ERR_FATAL( PROG, VER, TIMEOUT )\
        ERR_FATAL( xdr_clnt_lookup_err_msg, PROG, VER, TIMEOUT )
#define XDR_CLNT_LOOKUP2_ERR( PROG, VER, TIMEOUT )\
        ONCRPC_ERR( xdr_clnt_lookup_err_msg, PROG, VER, TIMEOUT )

extern  const char xdr_msg_send_err_msg[];
#define XDR_MSG_SEND_ERR_FATAL( XDR, REPLY_HEADER )\
        ERR_FATAL( xdr_msg_send_err_msg, XDR, REPLY_HEADER, 0 )
#define XDR_MSG_SEND_ERR( XDR, REPLY_HEADER )\
        ONCRPC_ERR( xdr_msg_send_err_msg, XDR, REPLY_HEADER, 0 )

extern  const char xdr_call_rejected_err_msg[];
#define XDR_CALL_REJECTED_ERR_FATAL( XDR, REPLY_HEADER )\
        ERR_FATAL(xdr_call_rejected_err_msg, XDR, (REPLY_HEADER)->u.dr.stat, 0 )
#define XDR_CALL_REJECTED_ERR( XDR, REPLY_HEADER )\
        ONCRPC_ERR(xdr_call_rejected_err_msg, XDR, (REPLY_HEADER)->u.dr.stat, 0 )

extern  const char xdr_err_on_server_err_msg[];
#define XDR_ERR_ON_SERVER_ERR_FATAL( XDR, REPLY_HEADER )\
        ERR_FATAL(xdr_err_on_server_err_msg, XDR, (REPLY_HEADER)->u.ar.stat, 0 )
#define XDR_ERR_ON_SERVER_ERR( XDR, REPLY_HEADER )\
        ONCRPC_ERR(xdr_err_on_server_err_msg, XDR, (REPLY_HEADER)->u.ar.stat, 0 )

extern  const char xdr_op_err_msg[];
#define XDR_OP_ERR_FATAL( XDR, XDR_OP_NUMBER )\
        ERR_FATAL( xdr_op_err_msg, XDR, XDR_OP_NUMBER, 0 )
#define XDR_OP_ERR( XDR, XDR_OP_NUMBER )\
        ONCRPC_ERR( xdr_op_err_msg, XDR, XDR_OP_NUMBER, 0 )

extern  const char xdr_svc_create_err_msg[];
#define XDR_SVC_CREATE_ERR_FATAL( SEND_SIZE, RECV_SIZE )\
        ERR_FATAL( xdr_svc_create_err_msg, 0, 0, 0 )

extern  const char xdr_svc_register_err_msg[];
#define XDR_SVC_REGISTER_ERR_FATAL( XPRT, PROG, VERS, DISPATCH, PROT )\
        ERR_FATAL( xdr_svc_register_err_msg, 0, 0, 0 )

extern  const char rpc_svc_cb_data_lookup_err_msg[];
#define RPC_SVC_CB_DATA_LOOKUP_ERR_FATAL()\
        ERR_FATAL( rpc_svc_cb_data_lookup_err_msg, 0, 0, 0 )
#define RPC_SVC_CB_DATA_LOOKUP_ERR()\
        ONCRPC_ERR( rpc_svc_cb_data_lookup_err_msg, 0, 0, 0 )

extern const char xdr_unknown_discriminator_msg[];
#ifdef FEATURE_WINCE
  /* FIXME: remove this when MSG_HIGH or ERR fixed in WinMobile */
  #define XDR_UNKNOWN_DISCRIMINATOR_MSG( VALUE )  /* */
#else /* !FEATURE_WINCE */
  #define XDR_UNKNOWN_DISCRIMINATOR_MSG( VALUE )\
    MSG_HIGH( xdr_unknown_discriminator_msg, VALUE, 0, 0 )
#endif /* !FEATURE_WINCE */

/*===========================================================================
  ENTRY POINTS
  ===========================================================================*/

extern const char xdr_clnt_lookup_err_msg[];
extern const char xdr_call_rejected_err_msg[];
extern const char xdr_err_on_server_err_msg[];
extern const char xdr_svc_create_err_msg[];
extern const char xdr_svc_register_err_msg[];
extern const char xdr_msg_send_err_msg[];
extern const char xdr_malloc_err_msg[];
extern const char xdr_unknown_discriminator_msg[];

/*===========================================================================
  FUNCTION: oncrpcxdr_mem_alloc

  DESCRIPTION:
      Allocates buffer from oncrpc's pool of memory and associates it with
      an XDR.

  RESULT:
      Pointer to allocated buffer.
  ===========================================================================*/
extern void *oncrpcxdr_mem_alloc( xdr_s_type *xdr, uint32 size );

/*===========================================================================
  FUNCTION: oncrpcxdr_mem_free

  DESCRIPTION:
      Frees all the buffers that were allocated from oncrpc's pool of memory
      and were associated with an XDR.

  RESULT:
      None
  ===========================================================================*/
extern void oncrpcxdr_mem_free( xdr_s_type *xdr );

extern boolean xdr_send_int64( xdr_s_type *xdr, const int64 *value );
extern boolean xdr_recv_int64( xdr_s_type *xdr, int64 *value );
extern boolean xdr_send_uint64( xdr_s_type *xdr, const uint64 *value );
extern boolean xdr_recv_uint64( xdr_s_type *xdr, uint64 *value );
extern boolean oncrpc_xdr_send_enum( xdr_s_type *xdr, int32 value );
extern boolean oncrpc_xdr_recv_enum( xdr_s_type *xdr, void *value, uint32 size );
extern boolean xdr_send_boolean( xdr_s_type *xdr, const boolean *value );
extern boolean xdr_recv_boolean( xdr_s_type *xdr, boolean *value );

extern boolean xdr_send_packed_enum( xdr_s_type *xdr, int32 value );
extern boolean xdr_recv_packed_enum( xdr_s_type *xdr,
                                     PACKED void *value PACKED_POST,
                                     uint32 size );
extern boolean xdr_send_packed_boolean(xdr_s_type *xdr,
                                       const PACKED boolean *value PACKED_POST);
extern boolean xdr_recv_packed_boolean( xdr_s_type *xdr,
                                        PACKED boolean *value PACKED_POST );

extern boolean xdr_send_packed_int8( xdr_s_type *xdr,
                                     const PACKED int8 *value PACKED_POST );
extern boolean xdr_send_packed_uint8( xdr_s_type *xdr,
                                      const PACKED uint8 *value PACKED_POST );
extern boolean xdr_send_packed_int16( xdr_s_type *xdr,
                                      const PACKED int16 *value PACKED_POST );
extern boolean xdr_send_packed_uint16( xdr_s_type *xdr,
                                       const PACKED uint16 *value PACKED_POST );
extern boolean xdr_send_packed_int32( xdr_s_type *xdr,
                                      const PACKED int32 *value PACKED_POST );
extern boolean xdr_send_packed_uint32( xdr_s_type *xdr,
                                       const PACKED uint32 *value PACKED_POST );
extern boolean xdr_send_packed_int64( xdr_s_type *xdr,
                                      const PACKED int64 *value PACKED_POST );
extern boolean xdr_send_packed_uint64( xdr_s_type *xdr,
                                       const PACKED uint64 *value PACKED_POST );
extern boolean xdr_recv_packed_int8( xdr_s_type *xdr,
                                     PACKED int8 *value PACKED_POST );
extern boolean xdr_recv_packed_uint8( xdr_s_type *xdr,
                                      PACKED uint8 *value PACKED_POST );
extern boolean xdr_recv_packed_int16( xdr_s_type *xdr,
                                      PACKED int16 *value PACKED_POST );
extern boolean xdr_recv_packed_uint16( xdr_s_type *xdr,
                                       PACKED uint16 *value PACKED_POST );
extern boolean xdr_recv_packed_int32( xdr_s_type *xdr,
                                      PACKED int32 *value PACKED_POST );
extern boolean xdr_recv_packed_uint32( xdr_s_type *xdr,
                                       PACKED uint32 *value PACKED_POST );
extern boolean xdr_recv_packed_int64( xdr_s_type *xdr,
                                      PACKED int64 *value PACKED_POST );
extern boolean xdr_recv_packed_uint64( xdr_s_type *xdr,
                                       PACKED uint64 *value PACKED_POST );

extern boolean oncrpc_xdr_send_auth( xdr_s_type *xdr, const opaque_auth *auth );
extern boolean xdr_recv_auth( xdr_s_type *xdr,
                              opaque_auth *auth );
extern void    xdr_free_auth( opaque_auth *auth );

extern boolean oncrpc_xdr_call_msg_start( xdr_s_type *clnt,
                                   uint32 prog,
                                   uint32 ver,
                                   uint32 proc,
                                   opaque_auth *cred,
                                   opaque_auth *verf );

extern boolean oncrpc_xdr_reply_msg_start( xdr_s_type *srv, opaque_auth *verf );

/*===========================================================================
FUNCTION: oncrpc_xdr_send_reply_header

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
boolean oncrpc_xdr_send_reply_header
(
  xdr_s_type              *xdr,
  rpc_reply_header const  *reply
);

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
);

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
);

/***********************************************************************
        Support for rpcgen
 ***********************************************************************/
extern bool_t oncrpc_xdr_void( void );
extern bool_t oncrpc_xdr_short (XDR *xdr, short *sp);
extern bool_t oncrpc_xdr_u_short (XDR *xdr, u_short *usp);
extern bool_t oncrpc_xdr_int( XDR *xdr, int *ip );
extern bool_t oncrpc_xdr_u_int( XDR *xdr, u_int *uip );
extern bool_t oncrpc_xdr_long (XDR *xdr, long *lp);
extern bool_t oncrpc_xdr_u_long( XDR *xdr, u_long *ulp );
extern bool_t oncrpc_xdr_bool( XDR *xdr, bool_t *bp );
/* NOTE: The RVCT2.2 compiler sometimes treats an enum as an unsigned quantity
 *       even if some enum values have the most significant bit set. This means
 *       that we cannot assume that enums are all signed integers inside the
 *       xdr_enum routine. A 0x80 value of an enum that is treated as an
 *       unsigned char would be sign extended, and if the receiver side treats
 *       all enums as signed int, it will decode an enum value of 0xffffff80.
 *       Therefore, we define xdr_enum as a macro that does an explicit cast
 *       to int32 on the enum value which is assumed to always be defined as
 *       '*objp' in any code generated by rpcgen. The compiler takes care of
 *       sign extension when needed and therefore the value is always encoded
 *       correctly.
 */
#define xdr_enum( xdr, ep, size ) oncrpc_xdr_enum_ext(xdr, ep, (int32)*objp, size)
extern bool_t oncrpc_xdr_enum_ext( XDR *xdr, enum_t *ep, int32 ev, uint16 size );
extern bool_t xdr_array (XDR * xdrs, caddr_t *addrp, u_int *sizep,
			 u_int maxsize, u_int elsize, xdrproc_t elproc);
extern bool_t oncrpc_xdr_bytes( XDR *xdr, char **cpp, u_int *sizep, u_int maxsize );
extern bool_t xdr_opaque( XDR *xdr, caddr_t cp, u_int cnt );
extern bool_t xdr_string (XDR *xdr, char **cpp, u_int maxsize);
extern bool_t xdr_union (XDR *xdr, enum_t *dscmp, uint16 size, 
                         char *unp, const struct xdr_discrim *choices,
			 xdrproc_t dfault);
extern bool_t xdr_char (XDR *xdr, char *cp);
extern bool_t xdr_u_char (XDR *xdr, u_char *cp);
extern bool_t xdr_vector (XDR *xdrs, char *basep, u_int nelem,
			  u_int elemsize, xdrproc_t xdr_elem);
extern bool_t xdr_reference (XDR *xdrs, caddr_t *xpp, u_int size,
			     xdrproc_t proc);
extern bool_t oncrpc_xdr_pointer (XDR *xdrs, char **_objpp,
			   u_int obj_size, xdrproc_t xdr_obj);
extern bool_t xdr_wrapstring (XDR *xdr, char **cpp);
extern bool_t xdr_hyper (XDR *__xdrs, quad_t *__llp);
extern bool_t xdr_u_hyper (XDR *__xdrs, u_quad_t *__ullp);
extern bool_t xdr_int64_t (XDR *__xdrs, int64_t *__llp);
extern bool_t xdr_uint64_t (XDR *__xdrs, uint64_t *__ullp);
extern bool_t xdr_longlong_t (XDR *__xdrs, quad_t *__llp);
extern bool_t xdr_u_longlong_t (XDR *__xdrs, u_quad_t *__ullp);
extern bool_t oncrpc_xdr_quad_t (XDR *__xdrs, quad_t *__llp);
extern bool_t xdr_u_quad_t (XDR *__xdrs, u_quad_t *__ullp);

/***********************************************************************
      Backwards Compatibility Support for Older Versions of HTORPC
 ***********************************************************************/
#define XDR_CLNT_LOOKUP2_ERROR( PROG, VER, TIMEOUT )\
        XDR_CLNT_LOOKUP2_ERR_FATAL( PROG, VER, TIMEOUT )

#define XDR_MSG_SEND_ERROR( XDR, REPLY_HEADER )\
        XDR_MSG_SEND_ERR_FATAL( XDR, NULL )

#define XDR_CALL_REJECTED_ERROR( REPLY_HEADER )\
        XDR_CALL_REJECTED_ERR_FATAL( NULL, &(REPLY_HEADER) )

#define XDR_SVC_CREATE_ERROR( SEND_SIZE, RECV_SIZE )\
        XDR_SVC_CREATE_ERR_FATAL( SEND_SIZE, RECV_SIZE )

#define XDR_ERR_ON_SERVER_ERROR( REPLY_HEADER )\
        XDR_ERR_ON_SERVER_ERR_FATAL( NULL, &(REPLY_HEADER) )

#define XDR_SVC_REGISTER_ERROR( XPRT, PROG, VERS, DISPATCH, PROT )\
        XDR_SVC_REGISTER_ERR_FATAL( XPRT, PROG, VERS, DISPATCH, PROT )

extern const char xdr_malloc_err_msg[];
#define XDR_MALLOC_ERROR( XDR, SIZE )\
        ERR_FATAL( xdr_malloc_err_msg, (SIZE), 0, 0 )

#define XDR_SEND_UNION_POINTER( XDR, OBJPP, DISC, XDR_SEND_OBJ, RESULT) \
do { \
  uint32 _more_data_ = ( *OBJPP != NULL ); \
  RESULT = TRUE; \
  if ( ! XDR_SEND_UINT32( XDR, &_more_data_ ) ) { \
    RESULT = FALSE; \
  } \
  else if ( _more_data_ ) { \
    if ( ! XDR_SEND_OBJ( XDR, *OBJPP, DISC) ) { \
      RESULT = FALSE; \
    } \
  } \
} while ( 0 )

#define XDR_SEND_UNION_POINTER_NO_ERRCHK( XDR, OBJPP, DISC, XDR_SEND_OBJ) \
do { \
  uint32 _more_data_ = ( *OBJPP != NULL ); \
  if ( XDR_SEND_UINT32( XDR, &_more_data_ ) && _more_data_ ) { \
    (void) XDR_SEND_OBJ( XDR, *OBJPP, DISC); \
  } \
} while ( 0 )

#define XDR_SEND_U_POINTER( XDR, OBJPP, DISC, XDR_SEND_OBJ, XDR_SEND_DISC, RESULT) \
do { \
  /*lint --e{123} suppress macro defined with arguments warning */ \
  uint32 _more_data_ = ( *OBJPP != NULL ); \
  RESULT = TRUE; \
  if ( ! XDR_SEND_UINT32( XDR, &_more_data_ ) ) { \
    RESULT = FALSE; \
  } \
  else if ( _more_data_ ) { \
    if ( ! XDR_SEND_DISC( XDR, &DISC ) ) { \
      RESULT = FALSE; \
    } else if ( ! XDR_SEND_OBJ( XDR, *OBJPP, DISC) ) { \
      RESULT = FALSE; \
    } \
  } \
} while ( 0 )

#define XDR_SEND_U_POINTER_NO_ERRCHK( XDR, OBJPP, DISC, XDR_SEND_OBJ, XDR_SEND_DISC) \
do { \
  /*lint --e{123} suppress macro defined with arguments warning */ \
  uint32 _more_data_ = ( *OBJPP != NULL ); \
  if ( XDR_SEND_UINT32( XDR, &_more_data_ ) && _more_data_ ) { \
    if ( XDR_SEND_DISC( XDR, &DISC ) ) { \
      (void) XDR_SEND_OBJ( XDR, *OBJPP, DISC); \
    } \
  } \
} while ( 0 )

#define XDR_RECV_UNION_POINTER( XDR, OBJPP, DISC, XDR_RECV_OBJ, RESULT ) \
do { \
  uint32  _more_data_; \
  void   *_temp_; \
  RESULT = TRUE; \
  if ( ! XDR_RECV_UINT32( XDR, &_more_data_ ) ) { \
    RESULT = FALSE; \
  } \
  else if ( ! _more_data_ ) { \
    *OBJPP = NULL; \
  } \
  else if ( *OBJPP == NULL ) { \
    _temp_ = oncrpcxdr_mem_alloc( XDR, sizeof(**OBJPP) ); \
    memset( _temp_, 0, sizeof( **OBJPP ) ); \
    *OBJPP = _temp_; \
    if ( ! XDR_RECV_OBJ( XDR, *OBJPP, DISC ) ) { \
      RESULT = FALSE; \
    } \
  } \
  else { \
    if( ! XDR_RECV_OBJ( XDR, *OBJPP, DISC ) ) { \
      RESULT = FALSE;  \
    } \
  } \
} while ( 0 ) 

#define XDR_RECV_UNION_POINTER_NO_ERRCHK( XDR, OBJPP, DISC, XDR_RECV_OBJ ) \
do { \
  uint32  _more_data_; \
  void   *_temp_; \
  (void) XDR_RECV_UINT32( XDR, &_more_data_ ); \
  if ( ! _more_data_ ) { \
    *OBJPP = NULL; \
  } \
  else if ( *OBJPP == NULL ) { \
    _temp_ = oncrpcxdr_mem_alloc( XDR, sizeof(**OBJPP) ); \
    memset( _temp_, 0, sizeof( **OBJPP ) ); \
    *OBJPP = _temp_; \
    (void) XDR_RECV_OBJ( XDR, *OBJPP, DISC ); \
  } \
  else { \
    (void) XDR_RECV_OBJ( XDR, *OBJPP, DISC ); \
  } \
} while ( 0 ) 

#define XDR_RECV_U_POINTER( XDR, OBJPP, DISC, XDR_RECV_OBJ, XDR_RECV_DISC, RESULT ) \
do { \
  /*lint --e{123} suppress macro defined with arguments warning */ \
  uint32  _more_data_; \
  void   *_temp_; \
  RESULT = TRUE; \
  if ( ! XDR_RECV_UINT32( XDR, &_more_data_ ) ) { \
    RESULT = FALSE; \
  } \
  else if ( ! _more_data_ ) { \
    *OBJPP = NULL; \
  } \
  else if ( *OBJPP == NULL ) { \
    _temp_ = oncrpcxdr_mem_alloc( XDR, sizeof(**OBJPP) ); \
    memset( _temp_, 0, sizeof( **OBJPP ) ); \
    *OBJPP = _temp_; \
    if ( ! XDR_RECV_DISC( XDR, &DISC ) ) { \
      RESULT = FALSE; \
    } else if ( ! XDR_RECV_OBJ( XDR, *OBJPP, DISC ) ) { \
      RESULT = FALSE; \
    } \
  } \
  else { \
    if ( ! XDR_RECV_DISC( XDR, &DISC ) ) { \
      RESULT = FALSE; \
    } else if( ! XDR_RECV_OBJ( XDR, *OBJPP, DISC ) ) { \
      RESULT = FALSE;  \
    } \
  } \
} while ( 0 ) 

#define XDR_RECV_U_POINTER_NO_ERRCHK( XDR, OBJPP, DISC, XDR_RECV_OBJ, XDR_RECV_DISC ) \
do { \
  /*lint --e{123} suppress macro defined with arguments warning */ \
  uint32  _more_data_; \
  void   *_temp_; \
  (void) XDR_RECV_UINT32( XDR, &_more_data_ ); \
  if ( ! _more_data_ ) { \
    *OBJPP = NULL; \
  } \
  else if ( *OBJPP == NULL ) { \
    _temp_ = oncrpcxdr_mem_alloc( XDR, sizeof(**OBJPP) ); \
    memset( _temp_, 0, sizeof( **OBJPP ) ); \
    *OBJPP = _temp_; \
    (void) XDR_RECV_DISC( XDR, &DISC ); \
    (void) XDR_RECV_OBJ( XDR, *OBJPP, DISC ); \
  } \
  else { \
    (void) XDR_RECV_DISC( XDR, &DISC ); \
    (void) XDR_RECV_OBJ( XDR, *OBJPP, DISC ); \
  } \
} while ( 0 ) 

#endif /* _RPC_XDR_H */
