#ifndef ONCRPC_XDR_TYPES_H
#define ONCRPC_XDR_TYPES_H

/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        O N C R P C _ X D R _ T Y P E S . H

GENERAL DESCRIPTION

  This is a header file for the ONCRPC XDR.  This file defines all of
  the types needed to defined the types needed to... define the xdr
  structure.

  ONCRPC (Open Network Computing Remote Procedure Calls) is a way of
  calling functions from one machine to another.  This is described in
  detail in the following RFCs: 1831, 1832, 1833

INITIALIZATION AND SEQUENCING REQUIREMENTS

 Copyright (c) 2005-2010 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/inc/oncrpc/oncrpc_xdr_types.h#2 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
02/23/09    rr     Add support for close, defined oncrpc_control_close_handle_type
10/17/07    ptm    Move rpc-cb-data-type to oncrpc_cb.h
10/12/07    ptm    Added thread name to xdr-s-type.
10/01/07    rr     Added ONCRPC_CONTROL_GET_CURRENT_DEST_ADDR
05/04/07    hn     Added hooks for handling of msg source information for
                   future support of multiple sources.
05/04/07    rr     Rename private to xprivate, keyword conflict on c++ compilers.
02/27/07    ptm/hn No longer count commands to prevent flooding the rpc task's
                   command queue. Instead use a pending flag.
10/09/06    hn     Put back auto call retry hook in state machine because
                   it's needed after a connection is lost and reinited.
10/03/06    hn     Removed xport_running op from transports.
                   Added support for locking/unlocking RPC services.
08/29/06    hn     Support handling error checks for clients automatically.
05/29/06    ptm    Added XDR_FLAG_RETRY.
01/25/06    ptm    Change msg_set_reply to msg_set_input.
12/22/05    ~SN    Clean up obsolete flag - XDR_FLAG_CLONE
05/27/05    clp    Added get_port functionality.
05/23/05    hn     Added 'mem' field to xdr structure and xdr_mem_s_type.
05/12/05    ptm    Merged XDR field names.
05/09/05    clp    Revamped XDR and XPORT structures.
05/09/05    ptm    Add cmd_cnt to keep race condition from filling cmd queue.
04/22/05    ptm    Add XPRT_RUNNING API and update the XDR_MSG_SEND API.
03/31/05    clp    Split from oncrpcxdr.h and other sources.
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


/*
 * XDR provides a conventional way for converting between C data
 * types and an external bit-string representation.  Library supplied
 * routines provide for the conversion on built-in C data types.  These
 * routines and utility routines defined here are used to help implement
 * a type encode/decode routine for each user-defined type.
 *
 * Each data type provides a single procedure which takes two arguments:
 *
 *      bool_t
 *      xdrproc(xdrs, argresp)
 *              XDR *xdrs;
 *              <type> *argresp;
 *
 * xdrs is an instance of a XDR handle, to which or from which the data
 * type is to be converted.  argresp is a pointer to the structure to be
 * converted.  The XDR handle contains an operation field which indicates
 * which of the operations (ENCODE, DECODE * or FREE) is to be performed.
 *
 * XDR_DECODE may allocate space if the pointer argresp is null.  This
 * data can be freed with the XDR_FREE operation.
 *
 * We write only one procedure per data type to make it easy
 * to keep the encode and decode procedures for a data type consistent.
 * In many cases the same code performs all operations on a user defined type,
 * because all the hard work is done in the component type routines.
 * decode as a series of calls on the nested data types.
 */

/* The version of ONCRPC supported */
#define RPC_MSG_VERSION    ((u_long) 2)

/*
 * Xdr operations.  XDR_ENCODE causes the type to be encoded into the
 * stream.  XDR_DECODE causes the type to be extracted from the stream.
 * XDR_FREE can be used to release the space allocated by an XDR_DECODE
 * request.
 */
enum xdr_op {
  XDR_ENCODE = 0,
  XDR_DECODE = 1,
  XDR_FREE = 2
};

/*
 * This is the number of bytes per unit of external data.
 */
#define BYTES_PER_XDR_UNIT  (4)

/*
 * The XDR handle.
 * Contains operation which is being applied to the stream,
 * an operations vector for the particular implementation (e.g. see xdr_mem.c),
 * and two private fields for the use of the particular implementation.
 */
typedef struct xdr_struct XDR;
/*
 * A xdrproc_t exists for each data type which is to be encoded or decoded.
 *
 * The second argument to the xdrproc_t is a pointer to an opaque pointer.
 * The opaque pointer generally points to a structure of the data type
 * to be decoded.  If this pointer is 0, then the type routines should
 * allocate dynamic storage of the appropriate size and return it.
 * bool_t       (*xdrproc_t)(XDR *, caddr_t *);
 */
typedef bool_t (*xdrproc_t) (XDR *, void *,...);


#define ONCRPC_CONTROL_GET_MTU                ( 1 )
#define ONCRPC_CONTROL_GET_TX_QUOTA           ( 2 )
#define ONCRPC_CONTROL_GET_RX_BUFFER_SIZE     ( 3 )
#define ONCRPC_CONTROL_REGISTER_SERVER        ( 4 )
#define ONCRPC_CONTROL_UNREGISTER_SERVER      ( 5 )
#define ONCRPC_CONTROL_GET_DEST               ( 6 )
#define ONCRPC_CONTROL_OPEN_XPORT             ( 7 )
#define ONCRPC_CONTROL_CLOSE_XPORT            ( 8 )
#define ONCRPC_CONTROL_RESET_XPORT            ( 9 )
#define ONCRPC_CONTROL_SET_DEST               ( 10 )
#define ONCRPC_CONTROL_GET_SOURCE_ADDR        ( 11 )
#define ONCRPC_CONTROL_GET_CURRENT_DEST_ADDR  ( 12 )
#define ONCRPC_CONTROL_OPEN_PROG_VERS         ( 13 )
#define ONCRPC_CONTROL_CLOSE                  ( 14 )




typedef struct oncrpc_prog_ver_struct
{
  rpcprog_t prog;
  rpcvers_t ver;
} oncrpc_prog_ver_type;

typedef uint64  oncrpc_addr_type;

typedef struct {
  oncrpc_addr_type        addr;
  oncrpc_prog_ver_type    prog_ver;
  uint32                  timeout;
} oncrpc_control_get_dest_type;

typedef struct {
  oncrpc_addr_type        addr;
} oncrpc_control_get_current_dest_type;


typedef struct {
  oncrpc_addr_type        addr;
} oncrpc_control_get_source_type;

typedef struct{
  oncrpc_prog_ver_type   prog_ver;
} oncrpc_control_register_server_type;


typedef struct{
  oncrpc_prog_ver_type   prog_ver;
}oncrpc_control_unregister_server_type;


typedef struct{
  oncrpc_addr_type  dest;
}oncrpc_control_set_dest_type;

typedef struct {
  oncrpc_prog_ver_type   prog_ver;
  int64       handle;
}oncrpc_control_open_prog_vers_type;

typedef struct {
  oncrpc_addr_type       handle;
  oncrpc_prog_ver_type   prog_ver;
}oncrpc_control_close_handle_type;

typedef struct{
  unsigned int xp;
  unsigned int port;
}oncrpc_control_open_xport_type;


#define NULL_xdrproc_t ((xdrproc_t)0)

/*
 * Support struct for discriminated unions.
 * You create an array of xdrdiscrim structures, terminated with
 * a entry with a null procedure pointer.  The xdr_union routine gets
 * the discriminant value and then searches the array of structures
 * for a matching value.  If a match is found the associated xdr routine
 * is called to handle that part of the union.  If there is
 * no match, then a default routine may be called.
 * If there is no match and no default routine it is an error.
 */
struct xdr_discrim
{
  int value;
  xdrproc_t proc;
};

/* Message enums */
typedef enum {
  RPC_MSG_CALL=0,
  RPC_MSG_REPLY=1,
  RPC_MSG_UNDEF = 2,
} rpc_msg_e_type;

typedef enum {
  RPC_MSG_ACCEPTED=0,
  RPC_MSG_DENIED=1
} rpc_reply_stat_e_type;

typedef enum {
  RPC_ACCEPT_SUCCESS = 0,
  RPC_PROG_UNAVAIL   = 1,
  RPC_PROG_MISMATCH  = 2,
  RPC_PROC_UNAVAIL   = 3,
  RPC_GARBAGE_ARGS   = 4,
  RPC_SYSTEM_ERR     = 5,
  RPC_PROG_LOCKED    = 6
} rpc_accept_stat_e_type;

typedef enum {
  RPC_MISMATCH=0,
  RPC_AUTH_ERROR=1
} rpc_reject_stat_e_type ;

/* Auth types */

/*
 * Status returned from authentication check
 */
typedef enum {
  ONCRPC_AUTH_OK=0,
  /*
   * failed at remote end
   */
  ONCRPC_AUTH_BADCRED=1,       /* bogus credentials (seal broken) */
  ONCRPC_AUTH_REJECTEDCRED=2,  /* client should begin new session */
  ONCRPC_AUTH_BADVERF=3,       /* bogus verifier (seal broken) */
  ONCRPC_AUTH_REJECTEDVERF=4,  /* verifier expired or was replayed */
  ONCRPC_AUTH_TOOWEAK=5,       /* rejected due to security reasons */
  /*
   * failed locally
   */
  ONCRPC_AUTH_INVALIDRESP=6,   /* bogus response verifier */
  ONCRPC_AUTH_FAILED=7         /* some unknown reason */
} oncrpc_auth_stat;

typedef enum {
  ONCRPC_AUTH_NONE  =0,   /* no authentication */
  ONCRPC_AUTH_NULL  =0,   /* backward compatibility */
  ONCRPC_AUTH_SYS   =1,   /* unix style (uid, gids) */
  ONCRPC_AUTH_UNIX  =1,
  ONCRPC_AUTH_SHORT =2    /* short hand unix style */
} oncrpc_auth_types;
/*
 * Authentication info.  Opaque to client.
 */
typedef struct opaque_auth {
  oncrpc_auth_types oa_flavor;    /* flavor of auth */
  caddr_t           oa_base;      /* address of more auth stuff */
  u_int             oa_length;    /* not to exceed MAX_AUTH_BYTES */
} opaque_auth;

#define MAX_AUTH_BYTES  400
#define MAXNETNAMELEN   255  /* maximum length of network user's name */

/* Error types */
/*
 * Reply header to an rpc request that was accepted by the server.
 * Note: there could be an error even though the request was
 * accepted.
 */
struct rpc_accepted_reply_header
{
  opaque_auth              verf;
  rpc_accept_stat_e_type   stat;
  union
  {
    struct
    {
      uint32 low;
      uint32 high;
    } versions;
  } u;
};

/*
 * Reply to an rpc request that was denied by the server.
 */
struct rpc_denied_reply
{
  rpc_reject_stat_e_type stat;
  union
  {
    struct
    {
      uint32 low;
      uint32 high;
    } versions;
    oncrpc_auth_stat why;  /* why authentication did not work */
  } u;
};

/*
 * RPC reply header structure. The reply header contains error codes in
 * case of errors in the server side or the RPC call being rejected.
 */
typedef struct rpc_reply_header
{
  rpc_reply_stat_e_type stat;
  union
  {
    struct rpc_accepted_reply_header ar;
    struct rpc_denied_reply dr;
  } u;
} rpc_reply_header;

/* XDR memory wrapper structure */
typedef struct oncrpcxdr_mem_struct {
  struct oncrpcxdr_mem_struct *next;

#ifdef IMAGE_APPS_PROC
  /* make structure size 8-bytes so we
     keep 8-byte alignment */
  uint32 padding;
#endif
} oncrpcxdr_mem_s_type;

// TODO - keep XPORT objects on queue to help track down memory leaks

/*===========================================================================
  Defining the XPORT structure
  ===========================================================================*/

#define XPORT_FLAG_XPORT_ALLOCED        0x0001

typedef struct xport_struct xport_s_type;
typedef struct xport_ops_struct xport_ops_s_type;

/* Entry points that must be provided by an xport */
struct xport_ops_struct {
  void    (*destroy)      ( xport_s_type *xport );

  boolean (*read_dsm)     ( xport_s_type *xport, dsm_item_type **item );
  uint32  (*read)         ( xport_s_type *xport, void **buffer,uint32 length );
  uint32  (*write)        ( xport_s_type *xport, const void *buffer,uint32 length );
  boolean (*write_dsm)    ( xport_s_type *xport, dsm_item_type **item );
  boolean (*control)      ( xport_s_type *xport, uint32 cmd, void *arg );
  boolean (*flush_output) ( xport_s_type *xport );
  boolean (*flush_input)  ( xport_s_type *xport );
  uint16  (*get_port)     ( xport_s_type *xport );
};

struct xport_struct {
  uint32                   use_count; /* For clone */
  uint32                   flags;
  const xport_ops_s_type * ops;
  xport_s_type           * xport;
  void                   * xprivate;
};

/*===========================================================================
  Defining the XDR structure
  ===========================================================================*/

/* Must be at least 4 because logging code hard codes references to the
   first three words and we want to have room for a NULL at the end */
#define XDR_THREAD_NAME_SIZE        4

typedef struct xdr_struct xdr_s_type;

/* Call back definition for non-blocking RPC calls */
typedef void (*rpc_reply_cb_type)( xdr_s_type *xdr, void *data );

/* Entry points that must be provided by xdr */
struct xdr_ops_struct {
  /* Transport control functions */
  void        (*xdr_destroy) ( xdr_s_type *xdr );
  boolean     (*xdr_control) ( xdr_s_type *xdr, int request, void *info );
  xdr_s_type *(*xdr_clone)   ( xdr_s_type *xdr );
  void        (*xdr_errchk)  ( xdr_s_type *xdr, boolean on );

  /* Incoming message control functions */
  boolean (*read)           ( xdr_s_type *xdr );
  boolean (*msg_set_input)  ( xdr_s_type *xdr, dsm_item_type **item );
  boolean (*msg_done)       ( xdr_s_type *xdr );

  /* Outgoing message control functions */
  boolean (*msg_start) ( xdr_s_type *xdr, rpc_msg_e_type rpc_msg_type );
  boolean (*msg_abort) ( xdr_s_type *xdr );
  boolean (*msg_send)  ( xdr_s_type *xdr, rpc_reply_header *reply );
  boolean (*msg_send_nonblocking) ( xdr_s_type        *cloned_xdr,
                                    rpc_reply_cb_type  cb,
                                    void              *cb_data );

  /* Message data functions */
  boolean (*send_int8)   ( xdr_s_type *xdr, const int8 *value );
  boolean (*send_uint8)  ( xdr_s_type *xdr, const uint8 *value );
  boolean (*send_int16)  ( xdr_s_type *xdr, const int16 *value );
  boolean (*send_uint16) ( xdr_s_type *xdr, const uint16 *value );
  boolean (*send_int32)  ( xdr_s_type *xdr, const int32 *value );
  boolean (*send_uint32) ( xdr_s_type *xdr, const uint32 *value );
  boolean (*send_bytes)  ( xdr_s_type *xdr, const void *buf, uint32 len );
  boolean (*send_dsm)    ( xdr_s_type *xdr, dsm_item_type **item );

  boolean (*recv_int8)   ( xdr_s_type *xdr, int8 *value );
  boolean (*recv_uint8)  ( xdr_s_type *xdr, uint8 *value );
  boolean (*recv_int16)  ( xdr_s_type *xdr, int16 *value );
  boolean (*recv_uint16) ( xdr_s_type *xdr, uint16 *value );
  boolean (*recv_int32)  ( xdr_s_type *xdr, int32 *value );
  boolean (*recv_uint32) ( xdr_s_type *xdr, uint32 *value );
  boolean (*recv_bytes)  ( xdr_s_type *xdr, void *buf, uint32 len );
  boolean (*recv_dsm)    ( xdr_s_type *xdr, dsm_item_type **item );

  // TODO do we need 32-bit float, 64-bit float, or 128-bit float
};

typedef struct xdr_ops_struct xdr_ops_s_type;

/*===========================================================================
  XDR structure definition - provides a generic interface to each
  supported transport. The xdr structure is used both for clients and
  for servers.
  ===========================================================================*/

struct xdr_struct {
  /* Common elements */
  q_link_type                link;
  uint32                     flags;
  const xdr_ops_s_type      *xops;
  xport_s_type              *xport;
  uint32                     xid;
  rpcprot_t                  protocol;
  boolean                    cmd_pending;
  void                      *xprivate;
  oncrpc_addr_type           msg_source;
  /* Client elements */
  void                      *thread_handle;  /* Primitive type used to reduce
                                                coupling */
  uint32                     event;          /* Primitive type used to reduce
                                                coupling */

  rpc_reply_cb_type          reply_cb;
  void                      *reply_data;
  /* Server elements */
  opaque_auth                verf;           /* verf to send back */
  opaque_auth                cred;           /* cred from client */
  xdr_s_type                *xp_next;        /* used for rpc task registration
                                                list */
  oncrpcxdr_mem_s_type      *mem;            /* memory managed by this xdr */
  /* rpcgen compatibility fields */
  enum xdr_op                x_op;           /* used for ENCODE/DECODE/FREE
                                                control */
  uint32                     x_prog;         /* program number */
  uint32                     x_vers;         /* program version */
  uint32                     x_proc;         /* for debug output */

  uint32                     name[XDR_THREAD_NAME_SIZE];

  char                      reply_buffer[512];
};

// Transport flag definitions
#define XDR_FLAG_XDR_ALLOCED        0x0001
#define XDR_FLAG_DEDICATED          0x0002
#define XDR_FLAG_DESTROYING         0x0004
#define XDR_FLAG_RETRY              0x0008

#endif /* _RPC_XDR_TYPES_H */
