/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
                          O N C R P C _ P A C M A R K

GENERAL DESCRIPTION
  This file contains an oncrpc transport that uses a packet marking protocol
  to multiplex multiple RPC messages on a single RPC connection. For example,
  a UART or single shared memory channel. (Of course, both sides of the RPC
  connection must be using packet marking.)

  This transport is at the same level as the record marking protocol which is
  used for a single RPC message on serial stream such as UART or TCP/IP.

 Copyright (c) 2004-2011, 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_pacmark.c#8 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
06/01/09   rr         Fix return code when read fails, fix buffer size
12/18/08   rr         Fix bug in FH where a callback can alter the fh used
                      in a fwd call, causing write failure.
11/12/08   rr         Fix read_handle determination, use proper IOCTL to get
                      the current handle rather than peek lower layer.
09/11/08    rr        Merge from mainline, cleanup includes
07/09/08   rr         Fix read handle for multiple callbacks CBSP20
03/06/08   ih         Fix memory leak of pacmark_lookup_list_critic_sect_ptr
                      when pacmark_init is invoked once for each XDR.
03/19/07   ih         Added NULL pointer check to pacmark_control, changed 
                      pacmark_read_dsm to only return when a whole packet has
                      been read.
11/30/07   ptm        Remove DSM items from write path.
10/18/07   ptm        Add pacmark-get-status.
10/15/07   ptm        Optimize pacmark-read-dsm.
10/11/07   ptm        Add pacmark-client-died to clean up client resources.
10/01/07   rr         Add  ONCRPC_CONTROL_GET_CURRENT_DEST_ADDR
09/07/07   hn         Do DSM allocation in pacmark_write if msg is NULL not if
                      the out_size is zero. This fixes a memory leak.
07/10/07   ptm        Remove featurization.
05/15/07   RJS        Split OS specific parts into seperate files.
                      Remove Int and task locks
05/04/07   rr         Pacmark 2, supports rpc_router header format
                      Not backwards compatible.
03/20/07   ptm        Rewrite pacmark read/write to use DSM items better.
10/03/06   hn         Removed xport_running op from transports.
05/11/06   ptm        Don't free memory during SMSM RESET.
02/01/06   ptm        Fix WinCE compiler warnings.
01/26/06   ptm        Make pacmark_ops const.
06/07/05   ptm        Add block comments and change pid to mid.
05/27/05   clp        Added get_port functionality.
05/09/05   clp        Added initialization of use count.
04/22/05   ptm        Add XPRT_RUNNING API.
04/12/05   ptm        Include oncrpci.h and remove err.h.
04/01/05   clp        Include header cleanup changes.
03/25/05   ptm        Add code to handle flush with invalid pid and fix error
                      cases.
03/11/05   ptm        Add code to pass flush output and flush input on to the
                      next transport layer.
12/02/04   ptm        Add code to pacmark_write to reduce the number of dsm
                      items that just contain the packet header to reduce
                      overhead in lower layers.
12/02/04   ptm        First revision w/edit history.
===========================================================================*/
#include "oncrpc_dsm.h"
#include "oncrpc.h"
#include "oncrpci.h"
#include "oncrpc_os.h"
#include "oncrpc_pacmark.h"


#ifdef FEATURE_SMSM
#include "smsm.h"
#endif /* FEATURE_SMSM */

/*======================================================================

  Packet header format:

  -------------------------------
  | L | Reserved | MID | Length |
  -------------------------------

  L           1-bit     Last packet flag
  Reserved    7-bits    Unused
  MID         8-bits    Message ID
  Length     16-bits    Length of packet

  ======================================================================*/

#define PACMARK_LAST_PACKET        (1UL << 31)
#define PACMARK_MID_INVALID         0xFFFFFFFF

#define ENCODE_PACKET_HEADER( FLAG, MID, LEN ) \
        ( (FLAG) | (((MID) &0x000000FF) << 16) | ((LEN) & 0x0000FFFF) )

#define DECODE_PACKET_HEADER( FLAG, MID, LEN, HEADER ) \
do { \
  uint32 _header_temp_ = (HEADER); \
  (FLAG) = (_header_temp_ & PACMARK_LAST_PACKET) == PACMARK_LAST_PACKET; \
  (MID)  = (_header_temp_ & 0x00FF0000) >> 16; \
  (LEN)  = (uint16)(_header_temp_ & 0x0000FFFF); \
} while( 0 )

/* Packet Marking 2 Header Structure */
/* Making the assumption that router address type is 64 bits, verified in initialization */
typedef   uint64 pacmark_pkt_address_type;

#define ROUTER_ADDRESS_ENCODE( ADDR1, ADDR2 ) \
    ( ((uint64)(ADDR2) << 32) | (uint64)(ADDR1) )

#define ROUTER_ADDRESS_DECODE1( ADDR ) ((uint32)(ADDR))
#define ROUTER_ADDRESS_DECODE2( ADDR ) ((uint32)((ADDR) >> 32))

typedef struct pacmark_packet_header_struct
{
  uint32 dest;
}pacmark_packet_header_type;


/* Packet Marking Lookup Struct */
typedef struct pacmark_lookup_struct
{
   struct pacmark_lookup_struct   *next;
   dsm_item_type                  *dsm_in_message;
   pacmark_pkt_address_type       source;
   uint32                         mid;
}pacmark_lookup_struct_type;


static oncrpc_crit_sect_ptr   pacmark_lookup_list_critic_sect_ptr;


#define PACMARK_FLAG_INCOMING_MSGS_ALLOC        0x00000001


/*****************************************************
 STRUCT PACMARK_PRIVATE_STRUCT
 Defines private members of pacmark
 ******************************************************/
typedef struct pacmark_private_struct {
  uint32 flags;
  uint32 rec_size;

  dsm_mempool_id_enum_type dsm_pool;

  pacmark_packet_header_type  packet_header;
  pacmark_pkt_address_type    msg_source_addr;

  oncrpc_addr_type dest_handle; 

  /* List that contains all partial messages, by MID and router source*/
  pacmark_lookup_struct_type lookup_incoming_msg_head;
  pacmark_lookup_struct_type *free_item;

  /* output data being assembled - will eventually be sent to
     lower level transport */
  uint32  out_mid;                  /* message id */
  uint32  out_size;
  char   *out_buf;
} pacmark_private_s_type;

/*******************************************************************
 * XPORT Functions
 *******************************************************************/
static boolean pacmark_read_dsm( xport_s_type *xport, dsm_item_type **item);
static uint32  pacmark_write( xport_s_type *xport, const void *buffer,
                              uint32 len);
static uint32  pacmark_read(xport_s_type *xport, void **buffer, uint32 len);
static boolean pacmark_write_dsm( xport_s_type *xport, dsm_item_type **item);
static boolean pacmark_flush_output( xport_s_type *xport );
static boolean pacmark_flush_input( xport_s_type *xport );
static uint16 pacmark_get_port( xport_s_type *xport );
static void pacmark_destroy( xport_s_type *xport );
static boolean pacmark_control( xport_s_type *xport, uint32 cmd, void *info);

static const xport_ops_s_type pacmark_ops = {
  pacmark_destroy,
  pacmark_read_dsm,
  pacmark_read,
  pacmark_write,
  pacmark_write_dsm,
  pacmark_control,
  pacmark_flush_output,
  pacmark_flush_input,
  pacmark_get_port
};

/*! Critical section for protecting mid accesses */
static oncrpc_crit_sect_ptr  oncrpc_pacmark_crit_sect;

static boolean pacmark_mids_in_use[XPORT_PACMARK_NUM_MIDS] = { 0 };
static uint32 pacmark_last_mid = 0;
static void pacmark_free_mid( uint32 mid );
static uint32 pacmark_next_mid( void );

/* TODO Add static left out for unit testing (yes it's being tested!)*/
dsm_item_type ** pacmark_get_dsm_inmessage(
  pacmark_private_s_type *rm,
  uint32 mid,
  pacmark_pkt_address_type  *source);

void pacmark_flush_all_dsm_inmessage( pacmark_private_s_type *rm );

void pacmark_remove_dsm_inmessage(
  pacmark_private_s_type *rm,
  uint32 mid,
  pacmark_pkt_address_type  *source
  );

/*===========================================================================
FUNCTION ONCRPC_PACMARK_MID_INIT

DESCRIPTION
  Initialises packet marking processing.

DEPENDENCIES
  None

ARGUMENTS
  None

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
void oncrpc_pacmark_mid_init(void)
{
  oncrpc_crit_sect_init(&oncrpc_pacmark_crit_sect);
  oncrpc_crit_sect_init(&pacmark_lookup_list_critic_sect_ptr);
}

/*===========================================================================
FUNCTION XPORTSVR_PACMARK_CREATE

DESCRIPTION
  Create a server packet marking xport. This xport can be use both for sending
  and receiving messages.

DEPENDENCIES
  None

ARGUMENTS
  child - pointer to xport below this one
  rec_size - maximum packet record size on outgoing messages
  dsm_pool - which dsm pool to use when allocating dsm items.

RETURN VALUE
  pointer to the packet marking xport, or NULL on error

SIDE EFFECTS
  None.
===========================================================================*/
xport_s_type *xportsvr_pacmark_create
(
  xport_s_type *child,
  uint32 rec_size,
  dsm_mempool_id_enum_type dsm_pool
)
{
  xport_s_type *xport;

  xport = oncrpc_mem_alloc( sizeof(xport_s_type) );

  if( xport_pacmark_init(xport, child,  rec_size, dsm_pool) )
  {
    xport->flags |= XPORT_FLAG_XPORT_ALLOCED;
    ((pacmark_private_s_type *) xport->xprivate)->flags |=
      PACMARK_FLAG_INCOMING_MSGS_ALLOC;
  }
  else
  {
    ERR( "xport_pacmark_create: unable to malloc incoming msgs array",
         0, 0, 0 );
    oncrpc_mem_free( xport );
    xport = NULL;
  }

  return xport;
} /* xportsvr_pacmark_create */

/*===========================================================================
FUNCTION XPORTCLNT_PACMARK_CREATE

DESCRIPTION
  Create a client packet marking xport. This xport can only be used to send
  messages and handle replies.

DEPENDENCIES
  None

ARGUMENTS
  child - pointer to xport below this one
  rec_size - maximum packet record size on outgoing messages
  dsm_pool - which dsm pool to use when allocating dsm items.

RETURN VALUE
  pointer to packet marking xport, or NULL on error

SIDE EFFECTS
  None.
===========================================================================*/
xport_s_type *xportclnt_pacmark_create
(
  xport_s_type *child,
  uint32 rec_size,
  dsm_mempool_id_enum_type dsm_pool
)
{
  xport_s_type *xport;

  xport = oncrpc_mem_alloc( sizeof(xport_s_type) );

  if( xport_pacmark_init( xport, child, rec_size, dsm_pool ) ) {
    xport->flags |= XPORT_FLAG_XPORT_ALLOCED;
  }
  else {
    oncrpc_mem_free( xport );
    xport = NULL;
  }

  return xport;
} /* xportclnt_pacmark_create */

/*===========================================================================
FUNCTION XPORT_PACMARK_INIT

DESCRIPTION
  Common routine for initializing a packet marking xport.

DEPENDENCIES
  None

ARGUMENTS
  xport - pointer to xport to initialize
  child - pointer to xport below this one
  incoming_msgs - pointer to array of dsm items
  rec_size - maximum packet record size on outgoing messages
  dsm_pool - which dsm pool to use when allocating dsm items.

RETURN VALUE
  TRUE if initialization sucessful, FALSE otherwise

SIDE EFFECTS
  None.
===========================================================================*/
boolean xport_pacmark_init
(
  xport_s_type *xport,
  xport_s_type *child,
  uint32 rec_size,
  dsm_mempool_id_enum_type dsm_pool
)
{
  pacmark_private_s_type *rm;

#if defined ONCRPC_64K_RPC
  if( xport == NULL || child == NULL || rec_size > 0x1FFFF ) {
#else
  if( xport == NULL || child == NULL || rec_size > 0xFFFF ) {
#endif
    ERR( "xport_pacmark_init: Invalid argument xport %d child %d rec_size %x",
         xport, child, rec_size );
    return FALSE;
  }

  memset( xport, 0, sizeof(xport_s_type) );

  rm = oncrpc_mem_alloc( sizeof(pacmark_private_s_type) );
  if(rm == NULL)
  {
    ERR("xport_pacmark_init: Memory allocation failed for rm\n", 0, 0, 0);
    return FALSE;
  }
  memset( rm, 0, sizeof(pacmark_private_s_type) );

  /* allocate, verify and initialize output buffer */
  rm->out_buf = (char *) oncrpc_mem_alloc( rec_size );
  if(rm->out_buf == NULL)
  {
    oncrpc_mem_free(rm);
    ERR("xport_pacmark_init: Memory allocation failed for rm->out_buf\n", 0, 0, 0);
    return FALSE;
  }
  memset( rm->out_buf, 0, rec_size );

  xport->ops = &pacmark_ops;
  xport->xprivate = rm;
  xport->xport = child;

  rm->rec_size = rec_size;
  rm->dsm_pool = dsm_pool;

  rm->out_mid = PACMARK_MID_INVALID;
  rm->out_size = sizeof(pacmark_packet_header_type);

  return TRUE;
} /* xport_pacmark_init */

/*===========================================================================
FUNCTION PACMARK_READ_DSM

DESCRIPTION
  Reads data from lower level xport and reassembles messages. It only returns
  complete messages.

  This routine processes the data received from the lower level and returns
  when a complete message is found or when it runs out of data. In order to
  be sure that all of the complete messages have been returned, the client
  (usually an XDR) must keep calling the routine until it returns FALSE.

DEPENDENCIES
  The xport must have been initialized.
  This routine assumes the transport returns either an entire pacmark packet
  or nothing.

ARGUMENTS
  xport - pointer to the xport
  item - pointer to dsm item pointer. If a complete message is found,
         it is appended to this item, otherwise the item is unchanged.

RETURN VALUE
  TRUE if a complete message was returned, FALSE on read error

SIDE EFFECTS
  None
===========================================================================*/
static boolean pacmark_read_dsm( xport_s_type *xport, dsm_item_type **item)
{
  uint32          size;
  boolean         rc=TRUE;
  pacmark_private_s_type *rm = xport->xprivate;
  oncrpc_control_get_current_dest_type xdr_source_addr;

  char *temp_buffer = NULL;
  size = XPORT_READ( xport->xport, (void**)&temp_buffer,PACMARK_MAX_MSG_SIZE);
  if(size > 0 )
  {
     dsm_pushdown(item,temp_buffer,size,ONCRPC_DSM_ITEM_POOL);
  } else
  {
     return FALSE;
  }

  if( !XPORT_CONTROL(xport->xport,ONCRPC_CONTROL_GET_CURRENT_DEST_ADDR,
  (void *)&xdr_source_addr) )
  {
      ERR( "pacmark_read_dsm Failed to get handle",0,0,0);	
      rc=FALSE;
  }
  else
  {
      rm->msg_source_addr = xdr_source_addr.addr;      
  }

  if(temp_buffer)
      free(temp_buffer);
  return rc;
} /* pacmark_read_dsm */


uint32 pacmark_read(xport_s_type *xport, void **buffer, uint32 len)
 {   
   return XPORT_READ( xport->xport, buffer,len);
 }

/*===========================================================================
FUNCTION PACMARK_WRITE

DESCRIPTION
  Packetizes a buffer of data and sends any completed packets to the lower
  level transport.

DEPENDENCIES
  The xport must have been initialized.

ARGUMENTS
  xport - pointer to the xport
  buffer - pointer to the data to send
  buf_len - the number of bytes to send

RETURN VALUE
  The number of bytes taken from the buffer. May not match len.

SIDE EFFECTS
  May buffer up some of the data.
===========================================================================*/
static uint32 pacmark_write
(
  xport_s_type *xport,
  const void *buffer,
  uint32 buf_len
)
{
  pacmark_private_s_type *rm = xport->xprivate;
  uint32 size;
  uint32 len;

  
  /* Limited by size of DSM pool of receiver and size field of packet header */
  if( buf_len > 0xC000 ) {
    buf_len = 0xC000;
  }

  if( rm->out_mid == PACMARK_MID_INVALID ) {
    rm->out_mid = pacmark_next_mid();
  }

  if( rm->out_size + buf_len < rm->rec_size ) {
    /* Even with this data we still won't have a full buffer,
       this is normal case for small items */
    //printf("pacmark copy to rm->out_size %d, len %d \n",(unsigned int) rm->out_size,(unsigned int)buf_len);
    memcpy( &rm->out_buf[rm->out_size], buffer, buf_len );
    rm->out_size += buf_len;
    return buf_len;
  }

  /* We're going to send at least one full buffer now, so set header */
  memcpy( rm->out_buf, &rm->packet_header, sizeof(pacmark_packet_header_type) );
  size = buf_len;

  do {
    /* space remaining in output buffer */
    len = rm->rec_size - rm->out_size;

    if( len <= size ) {
      /* output buffer will be full */
      memcpy( &rm->out_buf[rm->out_size], buffer, len );
      buffer = (char *)buffer + len;
      size -= len;

      XPORT_WRITE( xport->xport, rm->out_buf, rm->rec_size );

      rm->out_size = sizeof(pacmark_packet_header_type);
    }
    else {
      /* output buffer will not be full */
      memcpy( &rm->out_buf[rm->out_size], buffer, size );
      rm->out_size += size;
      size = 0;
    }
  } while( size );

  return buf_len;
} /* pacmark_write */

/*===========================================================================
FUNCTION PACMARK_WRITE_DSM

DESCRIPTION
  Packetizes a dsm chain of data and sends any completed packets to the lower
  level transport.

DEPENDENCIES
  The xport must have been initialized.

ARGUMENTS
  xport - pointer to the xport
  item - dsm chain to send

RETURN VALUE
  TRUE if all of the dsm chain was sent, FALSE otherwise

SIDE EFFECTS
  May buffer up some of the data.
===========================================================================*/
static boolean pacmark_write_dsm( xport_s_type *xport, dsm_item_type **item)
{
  pacmark_private_s_type *rm = xport->xprivate;
  uint32 len;
  uint32 pull_len;
  uint32 size;

  if( rm->out_mid == PACMARK_MID_INVALID ) {
    rm->out_mid = pacmark_next_mid();
  }

  size = dsm_length_packet( *item );

  if( rm->out_size + size < rm->rec_size ) {
    /* Even with this data we still won't have a full buffer */
    size = dsm_pullup_long( item, &rm->out_buf[rm->out_size], size );
    rm->out_size += size;
    return *item == NULL;
  }

  /* We're going to send at least one full buffer now, so set header */
  
  memcpy( rm->out_buf, &rm->packet_header, sizeof(pacmark_packet_header_type) );
  
  do {
    /* space remaining in output buffer */
    len = rm->rec_size - rm->out_size;

    if( len <= size ) {
      /* output buffer should be full */
      pull_len = dsm_pullup_long( item, &rm->out_buf[rm->out_size], len );

      if( pull_len != len ) {
        /* pullup failed, buffer not full after all */
        ERR( "DSM did not pullup the full amount", 0, 0, 0 );
        rm->out_size += pull_len;
        return FALSE;
      }

      size -= len;

      XPORT_WRITE( xport->xport, rm->out_buf, rm->rec_size );

      rm->out_size = sizeof(pacmark_packet_header_type);
    }
    else {
      /* output buffer will not be full */
      pull_len = dsm_pullup_long( item, &rm->out_buf[rm->out_size], size );
      rm->out_size += pull_len;

      if( pull_len != size ) {
        ERR( "DSM did not pullup the full amount", 0, 0, 0 );
        return FALSE;
      }

      /* Normal exit */
      return TRUE;
    }
  } while( size );

  /* Exit here it things work out so that we ended on an exactly full buffer */
  return TRUE;
} /* pacmark_write_dsm */

/*===========================================================================
FUNCTION PACMARK_FLUSH_OUTPUT

DESCRIPTION
  Send any buffered data to the lower level xport and marks the message as
  complete. It may send a partial packet, or even an empty packet to let the
  receiving xport know that the message is complete.

  This routine also calls the flush output routine of the lower level xport.

DEPENDENCIES
  The xport must have been initialized.

ARGUMENTS
  xport - pointer to the xport

RETURN VALUE
  TRUE if the operation was sucessful, FALSE otherwise

SIDE EFFECTS
  None.
===========================================================================*/
static boolean pacmark_flush_output( xport_s_type *xport )
{
  pacmark_private_s_type *rm = xport->xprivate;
  boolean                 result = FALSE;
  
  if( rm->out_mid != PACMARK_MID_INVALID ) {

  memcpy( rm->out_buf, &rm->packet_header,
            sizeof(pacmark_packet_header_type) );
    if(XPORT_WRITE( xport->xport, rm->out_buf, rm->out_size ) == rm->out_size){
      result = XPORT_FLUSH_OUTPUT( xport->xport );      
    }
    else {
      ERR( "unable to write packet", 0, 0, 0 );
      XPORT_FLUSH_OUTPUT( xport->xport );
    }

    pacmark_free_mid( rm->out_mid );
    rm->out_mid = PACMARK_MID_INVALID;
    rm->out_size = sizeof(pacmark_packet_header_type);
  }
  else {
    rm->out_size = sizeof(pacmark_packet_header_type);
     XPORT_FLUSH_OUTPUT( xport->xport );
   }

  return result;
} /* pacmark_flush_output */

/*===========================================================================
FUNCTION PACMARK_FLUSH_INPUT

DESCRIPTION
  Flush all pending incoming messages, partial or complete on the given
  transport.

  This routine also calls the flush input routine of the lower level xport.

  This routine should only be called if the corresponding sending xport has
  been reset.

DEPENDENCIES
  The xport must have been initialized.

ARGUMENTS
  xport - pointer to the xport

RETURN VALUE
  TRUE if the operation was sucessful, FALSE otherwise

SIDE EFFECTS
  None.
===========================================================================*/
static boolean pacmark_flush_input( xport_s_type *xport )
{
  pacmark_private_s_type *rm = xport->xprivate;

  /* The following call removes all items in the list and
     removes all packets from DSM items in list */
  pacmark_flush_all_dsm_inmessage(rm);

  return XPORT_FLUSH_INPUT( xport->xport );
} /* pacmark_flush_input */

/*===========================================================================
FUNCTION PACMARK_GET_PORT

DESCRIPTION
  Returns the result of XPORT_GET_PORT for the lower level xport. This is
  just a pass through routine. There is nothing that needs to be done by
  this xport.

DEPENDENCIES
  The xport must have been initialized.

ARGUMENTS
  xport - pointer to the xport

RETURN VALUE
  The value returned by the lower level xport.

SIDE EFFECTS
  None.
===========================================================================*/
static uint16 pacmark_get_port( xport_s_type *xport )
{
  return XPORT_GET_PORT( xport->xport );
} /* pacmark_get_port */


/*===========================================================================
FUNCTION PACMARK_DESTROY

DESCRIPTION
  Frees all of the resources associated with the xport.

  This routine also calls the xport destroy routine of the lower level xport.

DEPENDENCIES
  The xport must have been initialized.

ARGUMENTS
  xport - pointer to the xport

RETURN VALUE
  None.

SIDE EFFECTS
  The xport can not be used after this routine is called.
===========================================================================*/
static void pacmark_destroy ( xport_s_type *xport )
{
  pacmark_private_s_type *rm = xport->xprivate;

  XPORT_DESTROY( xport->xport );

#ifdef FEATURE_SMSM
  if( (smsm_get_local_state( SMSM_THIS_HOST ) & SMSM_RESET) == 0 )
#endif /* FEATURE_SMSM */
  {
    /* free incoming message buffer resources */
    pacmark_flush_all_dsm_inmessage(rm);

    /* free out going message resources */
    oncrpc_mem_free( rm->out_buf );
    pacmark_free_mid( rm->out_mid );

    oncrpc_mem_free( xport->xprivate );

    if( xport->flags & XPORT_FLAG_XPORT_ALLOCED ) {
      oncrpc_mem_free( xport );
    }
  }
} /* pacmark_destroy */

/*===========================================================================
FUNCTION PACMARK_FREE_MID

DESCRIPTION
  Marks a MID (message ID) as free.

DEPENDENCIES
  The xport must have been initialized.

ARGUMENTS
  mid - the message ID to free

RETURN VALUE
  None.

SIDE EFFECTS
  None
===========================================================================*/
static void pacmark_free_mid( uint32 mid )
{
  if( mid == PACMARK_MID_INVALID ) {
    return;
  }

  if( mid >= XPORT_PACMARK_NUM_MIDS ) {
    ERR_FATAL( "invalid mid %d", mid, 0, 0 );
  }

  pacmark_mids_in_use[mid] = FALSE;
} /* pacmark_free_mid */

/*===========================================================================
FUNCTION PACMARK_NEXT_MID

DESCRIPTION
  Allocates a MID (message ID). The MID identifies which message a packet
  belongs to. This is used by the receiving pacmark xport to reassemble
  messages.

DEPENDENCIES
  None

ARGUMENTS
  None.

RETURN VALUE
  The allocated mid.

SIDE EFFECTS
  None
===========================================================================*/
static uint32 pacmark_next_mid( void )
{
  uint32 result;
  uint32 i;

  oncrpc_crit_sect_enter(oncrpc_pacmark_crit_sect);

  pacmark_last_mid = (pacmark_last_mid + 1 ) % XPORT_PACMARK_NUM_MIDS;

  if( pacmark_mids_in_use[pacmark_last_mid] ) {
    for(i = 1; i < XPORT_PACMARK_NUM_MIDS; i++ ) {
      pacmark_last_mid = (pacmark_last_mid + 1 ) % XPORT_PACMARK_NUM_MIDS;

      if( !pacmark_mids_in_use[pacmark_last_mid] ) {
        break;
      }
    }

    pacmark_mids_in_use[pacmark_last_mid] = TRUE;
    result = pacmark_last_mid;
    oncrpc_crit_sect_leave(oncrpc_pacmark_crit_sect);

    if( i == XPORT_PACMARK_NUM_MIDS ) {
      ERR_FATAL( "ran out of mids", 0, 0, 0 );
    }

    RPC_MSG_HIGH( "mid still in use after all other mids have been used",
                  0, 0, 0 );
  }
  else {
    pacmark_mids_in_use[pacmark_last_mid] = TRUE;
    result = pacmark_last_mid;
    oncrpc_crit_sect_leave(oncrpc_pacmark_crit_sect);
  }

  return result;
} /* pacmark_next_mid */

/*===========================================================================
FUNCTION PACMARK_CONTROL

DESCRIPTION
  Handle XPORT control functions.  Locally processes set router
  destination control, and passes all control functions to lower layer.


DEPENDENCIES
  None

ARGUMENTS
  xport, pointer to xport.
  cmd,   Control command
  info,  Pointer to info structure.  The same structure is used for
  command info and response info.

RETURN VALUE
  TRUE if the command succeess, FALSE otherwise.

SIDE EFFECTS
  None
===========================================================================*/
static boolean pacmark_control( xport_s_type *xport,  uint32 cmd, void *info)
{
  pacmark_private_s_type  *rm;
  pacmark_pkt_address_type addr = 0;
  boolean                  result = FALSE;

  rm=(pacmark_private_s_type *)xport->xprivate;

  if(!info)
    return result;
  addr = *(pacmark_pkt_address_type *)info;

  switch( cmd )
  {
    case ONCRPC_CONTROL_GET_SOURCE_ADDR:
      ((oncrpc_control_get_source_type *)info)->addr =  rm->msg_source_addr;
      result = TRUE;
      break;

     case ONCRPC_CONTROL_SET_DEST:
      rm->dest_handle = ((oncrpc_control_set_dest_type *)info)->dest;
      rm->packet_header.dest = ((oncrpc_control_set_dest_type *)info)->dest;
      //printf("PACMARK SET DEST 0x%08x 0x%08x\n",(unsigned int) rm->packet_header.dest,(unsigned int)rm);
      break;
    default:
      result = XPORT_CONTROL(xport->xport,cmd,info);
      break;
  }

  return result;
} /*  pacmark_control */

/*===========================================================================
FUNCTION      PACMARK_GET_DSM_INMESSAGE

DESCRIPTION   Get the corresponding DSM item list for the MID and pacmark
              source address.
              If the item exists, the current DSM item is returned, if not,
              a new db entry is create containing the DSM item.

              This code is optimized for the assumed typical case of most
              RPC messages take one pacmark buffer. So the list is nearly
              always empty.

ARGUMENTS     list_head: lookup list head
              mid:  Pacmark message ID
              source: Pointer to data containing source, opaque.

RETURN VALUE  DSM Item Ptr.

SIDE EFFECTS  Will create and allocate memory for new item if not found.
===========================================================================*/
dsm_item_type ** pacmark_get_dsm_inmessage
(
  pacmark_private_s_type   *rm,
  uint32                    mid,
  pacmark_pkt_address_type *source
)
{
  pacmark_lookup_struct_type **first;
  pacmark_lookup_struct_type  *item;

  oncrpc_crit_sect_enter(pacmark_lookup_list_critic_sect_ptr);

  /* Search list for item */
  first = &rm->lookup_incoming_msg_head.next;
  item = *first;
  while( item != NULL )
  {
    if( ( mid == item->mid) &&
        (memcmp(&(item->source),source,sizeof(pacmark_pkt_address_type))==0) )
    {
      /* Item found, return it */
      oncrpc_crit_sect_leave(pacmark_lookup_list_critic_sect_ptr);
      return &item->dsm_in_message;
    }
    item = item->next;
  }

  /* Item not found, allocate and initialize item, add to list */
  if( rm->free_item != NULL )
  {
    item = rm->free_item;
    rm->free_item = NULL;
  }
  else
  {
    item = oncrpc_mem_alloc(sizeof(pacmark_lookup_struct_type));
    if(item == NULL)
    {
      oncrpc_crit_sect_leave(pacmark_lookup_list_critic_sect_ptr);
      ERR("pacmark_get_dsm_inmessage: Memory allocation failed for item\n", 0, 0, 0);
      return NULL;
    }
  }

  item->dsm_in_message = NULL;
  item->mid = mid;
  item->source = *source;

  item->next = *first;
  *first = item;

  oncrpc_crit_sect_leave(pacmark_lookup_list_critic_sect_ptr);

  return &item->dsm_in_message;
} /* pacmark_get_dsm_inmessage */

/*===========================================================================
FUNCTION      PACMARK_FLUSH_ALL_DSM_INMESSAGE

DESCRIPTION   Flush (remove) all packets in the list and corresponding
              dsm packets.

ARGUMENTS     rm - pointer to pacmark private data

RETURN VALUE  N/A

SIDE EFFECTS  All pending packets are lost.
===========================================================================*/
void pacmark_flush_all_dsm_inmessage(pacmark_private_s_type *rm )
{
  pacmark_lookup_struct_type *next;
  pacmark_lookup_struct_type *item;

  oncrpc_crit_sect_enter(pacmark_lookup_list_critic_sect_ptr);

  item = rm->lookup_incoming_msg_head.next;
  rm->lookup_incoming_msg_head.next = NULL;

  while( item != NULL )
  {
    next = item->next;

    dsm_free_packet( &item->dsm_in_message );

    if( rm->free_item == NULL )
    {
      rm->free_item = item;
    }
    else
    {
      oncrpc_mem_free(item);
    }

    item = next;
  }

  oncrpc_crit_sect_leave(pacmark_lookup_list_critic_sect_ptr);
} /* pacmark_flush_all_dsm_inmessage */


/*===========================================================================
FUNCTION      PACMARK_REMOVE_DSM_INMESSAGE

DESCRIPTION   Remove a dsm_inmessage item from list.

ARGUMENTS     rm - pointer to pacmark private data
              mid - message Id that partially identifies the item to remove.
              source - router source along with mid which fully identifies
              the item.

RETURN VALUE  N/A

SIDE EFFECTS  If the item is not found, the call does nothing.
===========================================================================*/
void pacmark_remove_dsm_inmessage
(
  pacmark_private_s_type   *rm,
  uint32                    mid,
  pacmark_pkt_address_type *source
)
{
  pacmark_lookup_struct_type *next;
  pacmark_lookup_struct_type *item;

  oncrpc_crit_sect_enter(pacmark_lookup_list_critic_sect_ptr);

  item = &rm->lookup_incoming_msg_head;
  next = item->next;

  while( next != NULL )
  {
    if( ( mid == next->mid) &&
        (memcmp(&(next->source),source,sizeof(pacmark_pkt_address_type))==0) )
    {
      item->next = next->next;

      dsm_free_packet( &next->dsm_in_message );

      if( rm->free_item == NULL )
      {
        rm->free_item = next;
      }
      else
      {
        oncrpc_mem_free(next);
      }

      break;
    }

    item = next;
    next = item->next;
  }

  oncrpc_crit_sect_leave(pacmark_lookup_list_critic_sect_ptr);
} /* pacmark_remove_dsm_inmessage */

/*===========================================================================
FUNCTION      PACMARK_REMOVE_CLIENT_INMESSAGE

DESCRIPTION   Remove all items from the list associated with the given client.

ARGUMENTS     list_head, pointer to list head.
              source, router source along with mid which fully identifies
              the item.

RETURN VALUE  N/A

SIDE EFFECTS  If no items are found, the call does nothing.
===========================================================================*/
static void pacmark_remove_client_inmessage
(
  pacmark_private_s_type   *rm,
  pacmark_pkt_address_type *source
)
{
  pacmark_lookup_struct_type* next;
  pacmark_lookup_struct_type* item;

  oncrpc_crit_sect_enter(pacmark_lookup_list_critic_sect_ptr);

  item = &rm->lookup_incoming_msg_head;
  next = item->next;

  while( next != NULL )
  {
    if( (memcmp(&(next->source),source,sizeof(pacmark_pkt_address_type))==0) )
    {
      item->next = next->next;

      dsm_free_packet( &next->dsm_in_message );

      if( rm->free_item == NULL )
      {
        rm->free_item = next;
      }
      else
      {
        oncrpc_mem_free(next);
      }
    }
    else
    {
      item = next;
    }

    next = item->next;
  }

  oncrpc_crit_sect_leave(pacmark_lookup_list_critic_sect_ptr);
} /* pacmark_remove_dsm_inmessage */

/*===========================================================================
FUNCTION      PACMARK_CLIENT_DIED

DESCRIPTION   Clean up any pacmark structures related to the dead client.
              Currently the only action to take is to remove any partial
              incoming messages from that client.

ARGUMENTS     xport, pointer to xport
              client_addr, Address of dead client.

RETURN VALUE  N/A

SIDE EFFECTS
===========================================================================*/
void pacmark_client_died
(
  xport_s_type *xport,
  oncrpc_addr_type client_addr
)
{
  pacmark_private_s_type *rm = xport->xprivate;

  pacmark_remove_client_inmessage( rm, &client_addr );
} /* pacmark_client_died */

/*===========================================================================
FUNCTION      PACMARK_GET_STATUS

DESCRIPTION   Return the status of the pacmark data structures.

              Currently only returns the number of entries in the partial
              message table.

ARGUMENTS     None

RETURN VALUE  Number of entries in the partial message table

SIDE EFFECTS
===========================================================================*/
int pacmark_get_status( void )
{
  pacmark_lookup_struct_type *item;
  pacmark_private_s_type     *rm;
  int32                       cnt;

  rm = oncrpc_router_read_xdr->xport->xprivate;
  cnt = 0;

  oncrpc_crit_sect_enter(pacmark_lookup_list_critic_sect_ptr);

  item = rm->lookup_incoming_msg_head.next;

  while( item != NULL )
  {
    item = item->next;
    cnt++;
  }

  oncrpc_crit_sect_leave(pacmark_lookup_list_critic_sect_ptr);

  return (int) cnt;
} /* pacmark_get_status */
