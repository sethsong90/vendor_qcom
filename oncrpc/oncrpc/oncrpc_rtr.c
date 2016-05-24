/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                      O N C R P C _ R T R . C

GENERAL DESCRIPTION

  This is the ONCRPC transport for the RPC router

  ONCRPC (Open Network Computing Remote Procedure Calls) is a way of
  calling functions from one machine to another.  This is described in
  detail in the following RFCs: 1831, 1832, 1833

INITIALIZATION AND SEQUENCING REQUIREMENTS

 Copyright (c) 2007-2010, 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_rtr.c#8 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
02/23/09   rr      Fix race condition in open file handle.
                   Add support for close
12/18/08   rr      Changes to support fix in pacmark, where FH is passed
                   as the first word in the data stream, and is extraced
                   at this layer, to fix bug in FH where a callback can
                   alter the fh used in a fwd call, causing write failure.
11/11/08    rr     Fix return code, add IOCTL to get handle
07/17/08    rr     Add Android support
03/19/08     ih    Added close xport IOCTL
03/05/08     ih    Added reset xport IOCTL
02/20/08     hn    Changed ERR_FATAL to ERR.
02/12/08     ih    Add null pointer check to the return value of dsm_new_buffer
02/04/07    hn     Cleanup use of arg->timeout in xprtrtr_control.
10/17/07    ptm    Minor cleanups to xprtrtr-control routine.
10/12/07    ptm    Add oncrpc-os.h. It was previously included by another header
10/30/07     rr    Handle timeout parameter in lookup
10/11/07     rr    Update for compatibility with rpc_router address type.
07/10/07    ptm    Remove featurization and change d word to uint 32.
05/02/07     ih    Initial version

===========================================================================*/

/* Parts of this file are derived from the original Sun (ONC) RPC
 * code, under the following copyright:
 */
/*
 * sun RPC is a product of Sun Microsystems, Inc. and is provided for
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


  #include "dsm.h"
  #include "oncrpc.h"
  #include "oncrpci.h"
  #include "oncrpc_os.h"
  #include "oncrpc_taski.h"
  #include "oncrpc_pacmark.h"
  #include "oncrpc_xdr_std.h"
  #include "oncrpc_rtr.h"
  #include "oncrpc_rtr_os.h"
  #include "oncrpc_dsm.h"
  #include "rpc_router_types.h"
  #include "oncrpc_main.h"
  #include <stdio.h>
/* Structures defs and such */
typedef struct
{  /* Kept in xprt->private */
  xport_s_type         *xport_parent; /* The "record" or "pac" mark handle */
  XPORT_HANDLE_TYPE     handle;       /* file handle for opened router */
} xprtrtr_conn_s_type;

/* XPRTRTR_PACMARK_SIZE must be smaller than the DSM item size */
/* by the amount needed for the pacmark header */
  #define XPRTRTR_PACMARK_SIZE  ( PACMARK_MAX_MSG_SIZE )

  #if(ONCRPC_DSM_ITEM_SIZ != RPC_ROUTER_MAX_MSG_SIZE)
    #error DSM ITEM must be equal to RPC_ROUTER_MAX_MSG_SIZE
  #endif /* ONCRPC_DSM_ITEM_SIZ != RPC_ROUTER_MAX_MSG_SIZE */

  #if defined WAIT_FOR_SERVER
    #define TOTAL_POLL_DURATION 15
  #else
    #define TOTAL_POLL_DURATION 0
  #endif

#define POLL_INTERVAL_MS 500

/* SM call port specific global variables */
static xdr_s_type            xprtrtr_xdr;
static xport_s_type          xprtrtr_pacmark;
static xport_s_type          xprtrtr_rtr;
static xprtrtr_conn_s_type   xprtrtr_conn;
static dsm_item_type       * xprtrtr_incoming_msgs[XPORT_PACMARK_NUM_MIDS];
static const char            *xprtrtr_device_location;
/*======================================================================
  XPORT structure definitions
  ======================================================================*/

static boolean xprtrtr_read_dsm( xport_s_type *xport, dsm_item_type **item);
static uint32  xprtrtr_write( xport_s_type *xport, const void *buffer,uint32 len);
static uint32  xprtrtr_read( xport_s_type *xport, void **buffer,uint32 len);
static boolean xprtrtr_write_dsm( xport_s_type *xport, dsm_item_type **item);
static boolean xprtrtr_flush_output( xport_s_type *xport );
static boolean xprtrtr_flush_input( xport_s_type *xport );
static uint16  xprtrtr_get_port( xport_s_type *xport );
static void    xprtrtr_destroy( xport_s_type *xport );
static boolean xprtrtr_control( xport_s_type *xport, uint32 cmd, void *data);

static const xport_ops_s_type xprtrtr_ops = {
  xprtrtr_destroy,
  xprtrtr_read_dsm,
  xprtrtr_read,
  xprtrtr_write,
  xprtrtr_write_dsm,
  xprtrtr_control,
  xprtrtr_flush_output,
  xprtrtr_flush_input,
  xprtrtr_get_port,
};


  #define ROUTER_ADDRESS_ENCODE( ADDR1, ADDR2 ) \
      ( ((uint64)(ADDR2) << 32) | (uint64)(ADDR1) )

  #define ROUTER_ADDRESS_DECODE1( ADDR ) ((uint32)(ADDR))
  #define ROUTER_ADDRESS_DECODE2( ADDR ) ((uint32)((ADDR) >> 32))

/*===========================================================================
  XPRTRTR INIT ROUTINES
===========================================================================*/

/*===========================================================================
FUNCTION XPRTRTR_INIT_XDR

DESCRIPTION
  Initialize a router transport XDR.

DEPENDENCIES
  None.

ARGUMENTS
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.
===========================================================================*/
void
  xprtrtr_init_xdr
  (
  xport_s_type         * rtr_xprt,
  xprtrtr_conn_s_type   * conn,
  xdr_s_type           * xdr,
  xport_s_type         * pacmark_xprt,
  dsm_item_type       ** incoming_msgs
  )
{
  /* Intialize the conn structure */
  conn->xport_parent = pacmark_xprt;

  /* Set up the sm xport */
  rtr_xprt->flags = 0;
  rtr_xprt->ops = &xprtrtr_ops;
  rtr_xprt->xprivate = conn;
  rtr_xprt->xport = NULL;
  rtr_xprt->use_count = 0;

  /* Set up the SERVER */
  if( !xport_pacmark_init( pacmark_xprt, rtr_xprt,
    XPRTRTR_PACMARK_SIZE,
    ONCRPC_DSM_ITEM_POOL ) )
  {
    ERR_FATAL( "xprtrtr_init_xdr: could not init pacmark", 0, 0, 0 );
  }

  if( !xdr_std_xdr_init( xdr, pacmark_xprt, 0, ONCRPC_RTR_PROTOCOL ) )
  {
    ERR_FATAL( "xprtrtr_init_xdr: could not init xdr_std", 0, 0, 0 );
  }
} /* xprtrtr_init_xdr */

/*===========================================================================
FUNCTION XPRTRTR_INIT

DESCRIPTION
  Initialize the router transport.

DEPENDENCIES
  None.

ARGUMENTS
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.
===========================================================================*/
void
  xprtrtr_init( void )
{
  /* Intialize the SM call port XDR structure */
  xprtrtr_init_xdr( &xprtrtr_rtr,
    &xprtrtr_conn,
    &xprtrtr_xdr,
    &xprtrtr_pacmark,
    xprtrtr_incoming_msgs );

  /* Open XPORT here */
  xprtrtr_os_init();

  xprtrtr_conn.handle = 0;

  if(xprtrtr_os_access("/dev/oncrpc") < 0)
  {
	xprtrtr_device_location = "/dev";
  }
  else
  {
	xprtrtr_device_location = "/dev/oncrpc";
  }



} /* xprtrtr_init */

/*===========================================================================
FUNCTION XPRTRTR_DEINIT

DESCRIPTION
  Deinitialize the router transport.

DEPENDENCIES
  None.

ARGUMENTS
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.
===========================================================================*/
void
  xprtrtr_deinit( void )
{
  xprtrtr_os_deinit();
} /*xprtrtr_deinit */

/*===========================================================================
  XPRTRTR OPS
===========================================================================*/

/*===========================================================================
FUNCTION XPRTRTR_READ_DSM

DESCRIPTION
  Read all of the data pending in the transport.

DEPENDENCIES
  The transport must have been initialized.

ARGUMENTS
  A pointer to the transport
  A pointer to the DSM item pointer.

RETURN VALUE
  TRUE if any data was added to the item list,
  FALSE otherwise

SIDE EFFECTS
  None.
===========================================================================*/
static boolean
  xprtrtr_read_dsm
  (
  xport_s_type *xport,
  dsm_item_type **item
  )
{
  int ret;

  xprtrtr_conn_s_type * conn = (xprtrtr_conn_s_type*)xport->xprivate;

  if( item == NULL ) {
    ERR( "xprtrtr_read_dsm: item pointer is NULL", 0, 0, 0 );
    return FALSE;
  }

  *item = dsm_new_buffer(ONCRPC_DSM_ITEM_POOL);

  if(*item == NULL) {
    ERR( "xprtrtr_read_dsm: Cannot allocate DSM item from pool", 0, 0, 0 );
    return FALSE;
  }

  ret = xprtrtr_os_read( &(conn->handle), (char **)(&(*item)->data_ptr),
          RPC_ROUTER_MAX_MSG_SIZE);

  if(ret < 0) {
    dsm_free_packet(item);
    *item = NULL;
    return FALSE;
  }

  (*item)->used = (uint16) ret;

  return TRUE;
} /* xprtrtr_read_dsm */



/*===========================================================================
FUNCTION XPRTRTR_READ

DESCRIPTION
  Read all of the data pending in the transport.

DEPENDENCIES
  The transport must have been initialized.

ARGUMENTS
  A pointer to the transport
  A pointer to the DSM item pointer.

RETURN VALUE
  TRUE if any data was added to the item list,
  FALSE otherwise

SIDE EFFECTS
  None.
===========================================================================*/
static uint32 xprtrtr_read
  (
  xport_s_type *xport,
  void **buffer,
  uint32 size
  )
{
  int32 rc;
  xprtrtr_conn_s_type * conn = (xprtrtr_conn_s_type*)xport->xprivate;
  rc = xprtrtr_os_read( &(conn->handle), (char **)buffer,size);
  if( rc < 0 )
    rc = 0;

  return rc;
} /* xprtrtr_read */



/*===========================================================================
FUNCTION XPRTRTR_WRITE

DESCRIPTION
  Write a data buffer to the router transport.

DEPENDENCIES
  The transport must have been initialized.

ARGUMENTS
  A pointer to the transport
  A pointer to the buffer to write
  The amount of data in the buffer

RETURN VALUE
  The amount of data written

SIDE EFFECTS
  None.
===========================================================================*/
static uint32
  xprtrtr_write
  (
  xport_s_type *xport,
  const void *buffer,
  uint32 len
  )
{
  int ret;
  uint32 handle = *((uint32 *)buffer);
  const char *buffer_data = ((const char *)buffer+4);

  ret = xprtrtr_os_write(handle, buffer_data, len-4);

  if( ret < 0 )
  {
    ret = 0;
  }
  else
  {
     ret = ret+4;
  }
  return ret;
} /* xprtrtr_write */

/*===========================================================================
FUNCTION XPRTRTR_WRITE_DSM

DESCRIPTION
  Puts the given DSM pointer on the send watermak queue of the shared memory
  driver.

DEPENDENCIES
  The transport must have been initialized.

ARGUMENTS
  A pointer to the transport
  A pointer to the DSM item pointer.

RETURN VALUE
  Returns FALSE if the sm port is not open,
  otherwise returns TRUE

SIDE EFFECTS
  None.
===========================================================================*/
static boolean
  xprtrtr_write_dsm
  (
  xport_s_type *xport,
  dsm_item_type **item
  )
{
  uint32 written, len;
  boolean retval = TRUE;

  if( item == NULL || *item == NULL )
  {
    ERR( "xprtrtr_write_dsm: empty item", 0, 0, 0 );
    return TRUE;
  }

  /* Send only the first buffer and ERR_FATAL if there are multiple buffers */
  /* Cache (*item)->used because it can get freed after the write */
  len = (*item)->used;
  if( (written = xprtrtr_write(xport, (*item)->data_ptr, (*item)->used)) != len )
  {
    retval = FALSE;
    ERR( "xprtrtr_write() failed! Wrote %d bytes.", written,0,0);
  }

  /* item could be freed at this point! */
  if( *item && (*item)->pkt_ptr )
  {
    retval = FALSE;
    ERR_FATAL( "Dropping rest of the packet. Wrote %d of %d bytes.", written,
      dsm_length_packet(*item), 0);
  }

  /* Free packet */
  dsm_free_packet(item);

  *item = NULL;
  return retval;
} /* xprtrtr_write_dsm */

/*===========================================================================
FUNCTION XPRTRTR_CONTROL

DESCRIPTION
  Send IOCTL to the router transport.

DEPENDENCIES
  The transport must have been initialized.

ARGUMENTS
  A pointer to the transport
  A command code
  A pointer to the command buffer

RETURN VALUE
  TRUE on success, else FALSE

SIDE EFFECTS
  None.
===========================================================================*/
static boolean
  xprtrtr_control
  (
  xport_s_type *xport,
  uint32 cmd,
  void *arg
  )
{
  xprtrtr_conn_s_type *conn = (xprtrtr_conn_s_type *) xport->xprivate;
  uint32 server_cmd = RPC_ROUTER_IOCTL_UNREGISTER_SERVER;
   boolean rc=TRUE;


  switch( cmd )
  {
    case ONCRPC_CONTROL_GET_DEST:
      break;

    case ONCRPC_CONTROL_OPEN_PROG_VERS:
      {
        char name[256];
        int no_of_polls;
        struct timespec poll_time;
        poll_time.tv_sec = 0;
        poll_time.tv_nsec = (POLL_INTERVAL_MS * 1000000);
        oncrpc_control_open_prog_vers_type *open_args;
        open_args = (oncrpc_control_open_prog_vers_type *) arg;
        /* All programs are in %08x:%08x except 00000000:0 which is a special device for registering servers */
        if( (open_args->prog_ver.prog == 0) && (open_args->prog_ver.ver == 0) )
        {
               snprintf(name, sizeof(name), "%s/%08x:%01x",xprtrtr_device_location,(uint32_t)open_args->prog_ver.prog, (uint32_t)open_args->prog_ver.ver);
        }else
        {
               snprintf(name, sizeof(name), "%s/%08x:%08x",xprtrtr_device_location,(uint32_t)open_args->prog_ver.prog, (uint32_t)open_args->prog_ver.ver);
        }

        if( ( (open_args->prog_ver.ver & 0x80000000) == 0) && ( xprtrtr_os_access(name) < 0 ) )
        {
               uint32 bwc_vers =  (open_args->prog_ver.ver & 0xffff0000);
               snprintf(name, sizeof(name), "%s/%08x:%08x",xprtrtr_device_location,(uint32_t)open_args->prog_ver.prog, (uint32_t)bwc_vers);
        }

        no_of_polls = TOTAL_POLL_DURATION * (1000/POLL_INTERVAL_MS);
        while(1)
        {
               if((xprtrtr_os_access(name) < 0) && (no_of_polls))
               {
                      nanosleep(&poll_time, NULL);
                      if (no_of_polls > 0)
                             no_of_polls--;
               }
               else
               {
                      open_args->handle = xprtrtr_os_open(name);
                      break;
               }
        }
        if(open_args->handle < 0)
        {
               printf("ONCRPC Failed to open device: %s\n",name);
               rc = FALSE;
        }
        else
               printf("ONCRPC Opened device: %s\n",name);
      }
      break;

    case ONCRPC_CONTROL_CLOSE:
         xprtrtr_os_close( ((oncrpc_control_close_handle_type *)arg)->handle);
      break;

    case ONCRPC_CONTROL_REGISTER_SERVER:
      server_cmd = RPC_ROUTER_IOCTL_REGISTER_SERVER;
      /* fall through */
    case ONCRPC_CONTROL_UNREGISTER_SERVER:
         /* Unsupported */
         rc = FALSE;
         break;

       case ONCRPC_CONTROL_GET_CURRENT_DEST_ADDR:
          ((oncrpc_control_get_current_dest_type *)arg)->addr = conn->handle;
         break;


    case ONCRPC_CONTROL_CLOSE_XPORT:
         xprtrtr_os_close( ((oncrpc_control_close_handle_type *)arg)->handle);
         break;

    default:
         rc=FALSE;
         break;
  }
   return rc;
} /* xprtrtr_write */

/*===========================================================================
FUNCTION XPRTRTR_FLUSH_OUTPUT

DESCRIPTION
  Flush any output data pending in the transport. There's no data queued in
  this transport, so there's nothing to do locally. However, we pass on the
  flush to the shared memory driver incase there's something that needs to be
  done there.

DEPENDENCIES
  None.

RETURN VALUE
  Always TRUE.

SIDE EFFECTS
  None.
===========================================================================*/
static boolean
  xprtrtr_flush_output
  (
  xport_s_type *xport
  )
{
  return TRUE;
} /* xprtrtr_flush_output */

/*===========================================================================
FUNCTION XPRTRTR_FLUSH_INPUT

DESCRIPTION
  Flush any pending input from the transport.

DEPENDENCIES
  The transport must have been initialized.

ARGUMENTS
  A pointer to the transport.

RETURN VALUE
  Always TRUE.

SIDE EFFECTS
  None.
===========================================================================*/
static boolean
  xprtrtr_flush_input
  (
  xport_s_type *xport
  )
{
  return TRUE;
} /* xprtrtr_flush_input */

/*===========================================================================
FUNCTION XPRTRTR_GET_PORT

DESCRIPTION
  This function returns the port associated with a xport, if any. This
  function only returns non-zero for TCP/UDP transports.

DEPENDENCIES
  None.

ARGUMENTS
  A pointer to the transport

RETURN VALUE
  Always 0.

SIDE EFFECTS
  None.
===========================================================================*/
static uint16
  xprtrtr_get_port
  (
  xport_s_type *xport
  )
{
  return 0;
} /* xprtrtr_get_port */

/*===========================================================================
FUNCTION XPRTRTR_DESTROY

DESCRIPTION
  Free the passed in transport because it is no longer needed

DEPENDENCIES
  The transport must have been initialized.

ARGUMENTS
  A pointer to the transport to destroy.

RETURN VALUE
  None.

SIDE EFFECTS
  None.
===========================================================================*/
static void
  xprtrtr_destroy
  (
  xport_s_type *xport
  )
{
  /* Does nothing at the moment since there are no dynamic structures to be
   * freed inside the smd transport and the smd transport is defined statically
   */
  ;
} /* xprtrtr_destroy */

/*===========================================================================
  SM XDR CREATION ROUTINES
===========================================================================*/

/*===========================================================================
FUNCTION SVCSM_CREATE

DESCRIPTION
  Returns a pointer to a shared memory server XDR

DEPENDENCIES
  None.

ARGUMENTS
  Both args are historical and not used.

  sendsize - The size of the send buffer to use, 0 => use the system default
  recvsize - The size of the receive buffer to use, 0 => use the system default

RETURN VALUE
  Pointer to a shared memory server XDR

SIDE EFFECTS
  None.
===========================================================================*/
SVCXPRT *
  svcrtr_create
  (
  u_int sendsize,
  u_int recvsize
  )
{
  return XDR_CLONE( &xprtrtr_xdr );
} /* svcrtr_create */

/*===========================================================================
FUNCTION CLNTRTR_CREATE

DESCRIPTION
  Create a general purpose shared memory client XDR. It can NOT be used by a
  client created using RPCGEN (see CLNTSM_CREATE_DEDICATED below).

DEPENDENCIES
  None.

ARGUMENTS
  The signal ONCRPC can use to wait/set for the client. The signal
  may be 0 if the XDR is only used for non-blocking calls.

RETURN VALUE
  Pointer to the new client or NULL on error.

SIDE EFFECTS
  None.
===========================================================================*/
CLIENT *
  clntrtr_create
  (
  uint32 event
  )
{
  xdr_s_type   *xdr;
  xport_s_type *pacmark;

  if (oncrpc_is_rpc_thread()) {
    ERR_FATAL( "Attempt to make an RPC call from within the ONCRPC TASK",
      0, 0, 0 );
  }

  pacmark = xportclnt_pacmark_create( &xprtrtr_rtr,
              XPRTRTR_PACMARK_SIZE,
              ONCRPC_DSM_ITEM_POOL );
  if( pacmark == NULL ) {
    return NULL;
  }

  xdr = xdr_std_create( pacmark, event, ONCRPC_RTR_PROTOCOL );

  if( xdr == NULL ) {
    XPORT_DESTROY( pacmark );
    return NULL;
  }

  return xdr;
} /* clntrtr_create */

/*===========================================================================
FUNCTION CLNTSM_CREATE_DEDICATED

DESCRIPTION
  Create a shared memory client XDR that is dedicated to a single program and
  version number.

  The XDR stack created by this routine is to support clients based on RPCGEN.
  The RPCGEN support code depends on the program number and version being
  stored in the XDR

DEPENDENCIES
  None.

ARGUMENTS
  prog - dedicated program number
  vers - dedicated version number
  sig - the signal ONCRPC can use to wait/set for the client.

RETURN VALUE
  Pointer to the new client or NULL on error.

SIDE EFFECTS
  None.
===========================================================================*/
CLIENT *
  clntrtr_create_dedicated
  (
  uint32 prog,
  uint32 vers,
  uint32 event
  )
{
  xdr_s_type *xdr;

  xdr = clntrtr_create( event );
  if( xdr != NULL ) {
    xdr->flags  |= XDR_FLAG_DEDICATED;
    xdr->x_prog  = prog;
    xdr->x_vers  = vers;
  }

  return xdr;
} /* clntrtr_create_dedicated */
