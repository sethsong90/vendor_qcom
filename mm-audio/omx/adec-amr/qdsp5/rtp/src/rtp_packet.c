
/*=*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                         QVP_RTP_PACKET.H

GENERAL DESCRIPTION

  This files contains the routines which will do RTP packetization and
  depacketization of RTP headers into a Packet. This file is not clean layer
  in the following respect. Very much mingled with the rest of the QVP_RTP
  architecture.

EXTERNALIZED FUNCTIONS

    qvp_rtp_ctx_init()

      Initializes the context data structure used by each RTP stream.

    qvp_rtp_pack()

      Forms an RTP header in the headroom area of the buf pointer passed
      and then readjusts the head room and len.

    qvp_rtp_unpack()

      Parses an RTP header in the buf passed  and then readjusts the head
      room and len contained.

    qvp_rtp_set_rtp_rx_payld_type()


      Sets the payload type number for the RX side of this RTP session.

    qvp_rtp_get_rx_payld_type()

      Returns the payload type number for the RX side of this RTP session.

    qvp_rtp_get_rtp_tx_ssrc

      Returns transmit SSRC for this RTP context.

INITIALIZATION AND SEQUENCING REQUIREMENTS

  None of the externalized functions will function before the task is
  spawned.


  Copyright (c) 2004 by Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary.  Export of this technology or software is
  regulated by the U.S. Government. Diversion contrary to U.S. law prohibited.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*=*/

/*===========================================================================

$Header: //source/qcom/qct/multimedia/videophone/ps_media/RTP/RTP_core/rel/1.0/src/qvp_rtp_packet.c#4 $ $DateTime: 2008/12/04 05:02:09 $ $Author: c_madhav $

                            EDIT HISTORY FOR FILE


when        who    what, where, why
--------    ---    ----------------------------------------------------------
12/04/08    mc    Fixing RVCT compile warnings for 7K
08/27/08    grk   Added log messages for test framework.
06/05/08    mc    Enabled RTP Send and Recv events for HSPA
09/24/07    apr   Change in format of printing TS values
10/10/06    grk   Removed qvp_rtp_get_tx_payld_type() &
                  qvp_rtp_set_rtp_tx_payld_type().
05/25/06    xz    Added a NEWLINE char at the EOF to remove a 7k warning
05/05/06   apr    Including RTP code in feature FEATURE_QVPHONE_RTP
02/27/06   uma    Modified qvp_rtp_unpack to drop multiple pyld sent
                  on same channel.Modified qvp_rtp_pack to set pyld based on
                  the incoming RTP packet
02/02/06   mkh    Removed the event EVENT_QVP_RTP_TX_PACKET. Data already
                  being logged using UL_RTP log packet.
08/01/05   srk    Added event reporting in RTP
07/05/05   srk    Updated externalized functions.
05/20/05   srk    Started using the payload type no
03/23/05   srk    Added API to set the payload type
                  for the streams.
03/18/05   srk    Initial Creation.
===========================================================================*/
//#include "customer.h"
#include "rtp_unpack.h"
#ifdef FEATURE_QVPHONE_RTP

/*===========================================================================
                     INCLUDE FILES FOR MODULE
===========================================================================*/
//#include <comdef.h>
//#include <qw.h>
//#include <clk.h>
#include <stdio.h>
#include "qvp_rtp_api.h"
#include "qvp_rtp_buf.h"
//#include "qvp_rtp_jb_api.h"
#include "qvp_rtp_profile.h"
#include "qvp_rtp_packet.h"
//#include "qvp_rtp_log.h"
#include "bit.h"
#include <stdlib.h>


#define QVP_RTP_ERR printf
#define QVP_RTP_MSG_HIGH printf

/*===========================================================================
                  LOCAL FORWARD DECLATIONS
===========================================================================*/
LOCAL void qvp_rtp_packet_skip_padding
(
  qvp_rtp_buf_type     *buf
);

LOCAL void qvp_rtp_packet_skip_xtn
(
  qvp_rtp_buf_type     *buf
);

/*===========================================================================

FUNCTION QVP_RTP_CTX_INIT

DESCRIPTION

  This function initializes the context with SRC identifiers and any other
  data which need initialization.


DEPENDENCIES
  None

ARGUMENTS IN
  ctx  - context pointer which needs init




RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succes
                     else error code.


SIDE EFFECTS
  None.

===========================================================================*/
//qvp_rtp_status_type qvp_rtp_ctx_init
//(
//  qvp_rtp_ctx_type *ctx
//)
//{
//  qword    cur_time;
//  uint32   lo_time;
///*------------------------------------------------------------------------*/
//
//
//  /*------------------------------------------------------------------------
//    Do a Quick Sanity
//  ------------------------------------------------------------------------*/
//  if( !ctx )
//  {
//    return( QVP_RTP_ERR_FATAL );
//  }
//
//
//  /*------------------------------------------------------------------------
//    Read the clock and then get the low value
//  ------------------------------------------------------------------------*/
//  clk_read_ms( cur_time );
//  lo_time = qw_lo( cur_time );
//
//  /*------------------------------------------------------------------------
//    Apply the seed and go then read a random no....
//  ------------------------------------------------------------------------*/
//  srand( (unsigned int ) lo_time  );
//  ctx->tx_ssrc = rand(  );
//
//  /*------------------------------------------------------------------------
//     Start the sequence number from zero
//  ------------------------------------------------------------------------*/
//  ctx->sequence_no = 0;
//
//  return( QVP_RTP_SUCCESS );
//
//} /* end of function qvp_rtp_ctx_init */

/*===========================================================================

FUNCTION QVP_RTP_PACK

DESCRIPTION

   Forms an RTP header in the headroom area of the buf pointer passed
   and then readjusts the head room and len.


DEPENDENCIES
  None

ARGUMENTS IN
  ctx  - context pointer RTP stream.
  pkt  - packet in which the header should be packed




RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succes
                     else error code.


SIDE EFFECTS
  None.

===========================================================================*/
//qvp_rtp_status_type qvp_rtp_pack
//(
//
//  qvp_rtp_ctx_type              *ctx,
//  qvp_rtp_buf_type              *pkt
//
//)
//{
//  uint8 *header; /* used as a pointer to stuff the header */
//  uint16 offset = 0;
//  qvp_rtp_packet_send_recv_event_info_type event_info;
//
///*------------------------------------------------------------------------*/
//
//
//  /*------------------------------------------------------------------------
//    Are we sane here... Or we bail out
//  ------------------------------------------------------------------------*/
//  if( !ctx || !pkt )
//  {
//    return( QVP_RTP_ERR_FATAL );
//  }
//
//  /*------------------------------------------------------------------------
//     See if we can stick in an RTP header
//  ------------------------------------------------------------------------*/
//  if( pkt->head_room < 12 )
//  {
//
//    return( QVP_RTP_ERR_FATAL );
//
//  }
//  else
//  {
//
//    pkt->head_room -= 12;
//    pkt->len += 12;
//
//  }
//
//  /*------------------------------------------------------------------------
//    Initialize the header to where the data starts
//  ------------------------------------------------------------------------*/
//  header =(uint8 *)( pkt->data + pkt->head_room );
//
//  /*----------------------------------------------------------------------
//    reset first byte to zero ... Set all the bit fields to zero
//  ----------------------------------------------------------------------*/
//  memset( header, 0, 12 );
//
//
//  /*----------------------------------------------------------------------
//    Stick in version 2
//  -----------------------------------------------------------------------*/
//  b_packd( 2, header, offset, 2 );
//  offset += 2;
//
//  /*----------------------------------------------------------------------
//    no padding , cc, X bits. move offset. Sice we set everything to zero
//    just move the offset
//  ----------------------------------------------------------------------*/
//  offset += 6;
//
//  /*------------------------------------------------------------------------
//     Set the marker bit as dictated by packet
//  ------------------------------------------------------------------------*/
//  b_packb( pkt->marker_bit, header, offset, 1 );
//  offset += 1;
//
//  /*------------------------------------------------------------------------
//    Set the payload type as dictated by the incoming RTP packet
//  ------------------------------------------------------------------------*/
//  b_packb( pkt->rtp_pyld_type, header, offset, 7 );
//  offset += 7;
//
//  /*------------------------------------------------------------------------
//      Now we need to stick in Seq
//  ------------------------------------------------------------------------*/
//  b_packw( ( uint16 ) ctx->sequence_no , header, offset, 16 );
//  offset += 16;
//
//  /*------------------------------------------------------------------------
//     Very important we need to stick in the time stamp
//  ------------------------------------------------------------------------*/
//  b_packd( pkt->tstamp, header, offset, 32 );
//  offset += 32;
//
//  /*------------------------------------------------------------------------
//     We need to identify the source
//  ------------------------------------------------------------------------*/
//  b_packd( ctx->tx_ssrc, header, offset, 32 );
//  offset += 32;
//
//
//  event_info.timestamp  = pkt->tstamp;
//  event_info.seq_num    = ctx->sequence_no;
//  event_info.marker_bit = pkt->marker_bit;
//
//  /*event_report_payload(
//          (event_id_enum_type) EVENT_QVP_SEND_RTP_PACKET,
//          sizeof(qvp_rtp_packet_send_recv_event_info_type),
//          (void *)&event_info);*/
//
//  /*------------------------------------------------------------------------
//    logs a complete descriprtion of the packet we just built.
//  ------------------------------------------------------------------------*/
//  QVP_RTP_MSG_HIGH(" Sending RTP tstamp= %lu, marker = %d, seq = %d",
//                    pkt->tstamp, pkt->marker_bit, ctx->sequence_no );
//  QVP_RTP_MSG_HIGH( "packet type = %d", pkt->rtp_pyld_type,0,0 );
//
//  /* increment the internal sequence number for this session */
//  ctx->sequence_no++;
//
//  return (QVP_RTP_SUCCESS );
//
//} /* end of function qvp_rtp_pack */

/*===========================================================================

FUNCTION QVP_RTP_UNPACK

DESCRIPTION

   Parses an RTP header in the buf passed  and then readjusts the head
   room and len contained. The parsed values are populated into appropriate
   packet fields.


DEPENDENCIES
  None

ARGUMENTS IN
  ctx  - context pointer RTP stream.
  pkt  - packet in which the header should be parsed




RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succes
                     else error code.


SIDE EFFECTS
  None.

===========================================================================*/
qvp_rtp_status_type qvp_rtp_unpack
(

  qvp_rtp_ctx_type     *ctx,
  qvp_rtp_buf_type     *buf

)
{
  uint8 *header;
  uint16 parse_offset = 0;
  boolean padding;
  boolean xtn;   /* extension bit */
  uint8   num_csrc;
  //qvp_rtp_packet_send_recv_event_info_type event_info;
  uint8 version = 0;
#ifdef QVP_RTP_TESTFRAMEWORK
  char logmsg[1000] ; //stores errmsgs
#endif
/*------------------------------------------------------------------------*/


  /*------------------------------------------------------------------------
    Are we sane here... Or we bail out
  ------------------------------------------------------------------------*/
  if( !ctx || !buf )
  {
    return( QVP_RTP_ERR_FATAL );
  }

#ifdef QVP_RTP_TESTFRAMEWORK
  sprintf(logmsg,"Received a packet\n");
  qvp_rtp_testcases_log(logmsg);
#endif
  /*------------------------------------------------------------------------
     See if there is enough bytes to begin the packing
  ------------------------------------------------------------------------*/
  if( buf->len < 12 )
  {
    return( QVP_RTP_ERR_FATAL );
  }

#ifdef QVP_RTP_TESTFRAMEWORK
  sprintf(logmsg,"\n\nPKT_LEN \t\t %d \r\n", buf->len );
  qvp_rtp_testcases_log(logmsg);
#endif
  /*------------------------------------------------------------------------
    Initialize header with the right pointer
  ------------------------------------------------------------------------*/
  header = buf->data + buf->head_room;
  /*printf("*buf->data - %x\n", *buf->data);
  printf("Inside, qvp_rtp_unpack: contents of header- %x\n", *header);*/

  /*------------------------------------------------------------------------

      RTP header Taken from RFC3550.

      0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           timestamp                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           synchronization source (SSRC) identifier            |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |            contributing source (CSRC) identifiers             |
   |                             ....                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  ------------------------------------------------------------------------*/

  /*------------------------------------------------------------------------
    Parse the version no
  ------------------------------------------------------------------------*/
  if( ( version = b_unpackw( header, parse_offset, 2) ) != 2)
  {
    QVP_RTP_ERR(" Wrong Version : %d %d %d\n", version, 0, 0 );
    return( QVP_RTP_ERR_FATAL );
  }
  parse_offset += 2;
#ifdef QVP_RTP_TESTFRAMEWORK
  sprintf(logmsg,"version \t\t %d \r\n", version );
  qvp_rtp_testcases_log(logmsg);
#endif
  /*------------------------------------------------------------------------
    Parse the padding
  ------------------------------------------------------------------------*/
  padding = b_unpackw( header, parse_offset, 1 );
  parse_offset += 1;
#ifdef QVP_RTP_TESTFRAMEWORK
  sprintf(logmsg,"Padding \t\t %d \r\n", padding );
  qvp_rtp_testcases_log(logmsg);
#endif
  /*------------------------------------------------------------------------
    Parse the extension
  ------------------------------------------------------------------------*/
  xtn = b_unpackw( header, parse_offset, 1 );
  parse_offset += 1;
#ifdef QVP_RTP_TESTFRAMEWORK
  sprintf(logmsg,"extension \t\t %d \r\n", xtn );
  qvp_rtp_testcases_log(logmsg);
#endif
  /*------------------------------------------------------------------------
    parse the csrc count
  ------------------------------------------------------------------------*/
  num_csrc = b_unpackw( header, parse_offset, 4 );
  parse_offset += 4;

  /*------------------------------------------------------------------------
    Stick marker bit in the pkt container
  ------------------------------------------------------------------------*/
  buf->marker_bit = b_unpackw( header, parse_offset, 1 );
  parse_offset += 1;
#ifdef QVP_RTP_TESTFRAMEWORK
  sprintf(logmsg,"marker bit \t\t %d \r\n", buf->marker_bit );
  qvp_rtp_testcases_log(logmsg);
#endif
  /*------------------------------------------------------------------------
    Pyld type contains in the buf
  ------------------------------------------------------------------------*/
  buf->rtp_pyld_type = b_unpackw( header, parse_offset, 7 );
#ifdef QVP_RTP_TESTFRAMEWORK
  sprintf(logmsg,"payload type \t\t %d \r\n", buf->rtp_pyld_type );
  qvp_rtp_testcases_log(logmsg);
#endif
  /*----------------------------------------------------------------------
    check if the received payload belongs to the stream we have configured
    Currently each RTP stream can be configured only for one payload
    With support for One of N payload each stream can be configured
    for multiple pylds and this check has to be replaced with packet
    switcher to check for supported payloads on the channel
  ----------------------------------------------------------------------*/
  if ( buf->rtp_pyld_type != ctx->rx_payload_type)
  {
    QVP_RTP_ERR(" Rxed payload type %d  different from that of configured\
       stream %d %d\n", buf->rtp_pyld_type, ctx->rx_payload_type, 0 );
    return( QVP_RTP_ERR_FATAL );
  }

  parse_offset += 7;

  /*------------------------------------------------------------------------
     Get the seq no out of the packet
  ------------------------------------------------------------------------*/
  buf->seq  = b_unpackw( header, parse_offset, 16 );
  parse_offset += 16;
#ifdef QVP_RTP_TESTFRAMEWORK
  sprintf(logmsg,"Seq \t\t\t %d \r\n",  buf->seq  );
  qvp_rtp_testcases_log(logmsg);
#endif
  /*------------------------------------------------------------------------
     Followed by 32 bit timestamp
  ------------------------------------------------------------------------*/
  buf->tstamp = b_unpackd( header, parse_offset, 32 );
  parse_offset += 32;
#ifdef QVP_RTP_TESTFRAMEWORK
  sprintf(logmsg,"timestamp \t\t %d \r\n", buf->tstamp );
  qvp_rtp_testcases_log(logmsg);
#endif

  ctx->rx_ssrc = b_unpackd( header, parse_offset, 32 );
  parse_offset += 32;
#ifdef QVP_RTP_TESTFRAMEWORK
  sprintf(logmsg,"rx_ssrc \t\t %d \r\n", ctx->rx_ssrc );
  qvp_rtp_testcases_log(logmsg);
#endif

  /*------------------------------------------------------------------------
    Offset for c_src identifiers
  ------------------------------------------------------------------------*/
  if( num_csrc )
  {
    buf->head_room += 12 + ( num_csrc * 4 );
    buf->len -= 12 + (num_csrc * 4 );

  }
  else
  {
    buf->head_room += 12;
    buf->len -= 12;
  }
#ifdef QVP_RTP_TESTFRAMEWORK
  sprintf(logmsg,"Head_room \t\t %d, buf_len \t\t %d \r\n", buf->head_room,
               buf->len );
  qvp_rtp_testcases_log(logmsg);
#endif
  /*------------------------------------------------------------------------
    If Xtn is set just follow skip it
  ------------------------------------------------------------------------*/
  if( xtn )
  {
    qvp_rtp_packet_skip_xtn( buf );

  }


  /*------------------------------------------------------------------------
     If there is padding we need to ingnore that
  ------------------------------------------------------------------------*/
  if( padding )
  {
    qvp_rtp_packet_skip_padding( buf );
  }

  //event_info.timestamp  = buf->tstamp;
  //event_info.seq_num    = buf->seq;
  //event_info.marker_bit = buf->marker_bit;

 /* event_report_payload(
          (event_id_enum_type) EVENT_QVP_RECV_RTP_PACKET,
          sizeof(qvp_rtp_packet_send_recv_event_info_type),
          (void *)&event_info);*/

  QVP_RTP_MSG_HIGH(" Received RTP tstamp= %lu, marker = %d, seq = %lu",
                    buf->tstamp, buf->marker_bit, buf->seq );

  QVP_RTP_MSG_HIGH( "packet type = %d %d %d\n", buf->rtp_pyld_type,0 , 0 );

  buf->parse_offset = parse_offset;


  return( QVP_RTP_SUCCESS );

} /* end of function qvp_rtp_unpack */


/*===========================================================================

FUNCTION QVP_RTP_PACKET_SKIP_PADDING

DESCRIPTION

  This function will skip the paddin in the end of an RTP packet.



DEPENDENCIES
  None

ARGUMENTS IN
  buf - the buffer pointer which needs a skip padding




RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succes
                     else error code.


SIDE EFFECTS
  Length of the buf is adjusted

===========================================================================*/
LOCAL void qvp_rtp_packet_skip_padding
(
  qvp_rtp_buf_type     *buf
)
{
  uint16 pad_len;
#ifdef QVP_RTP_TESTFRAMEWORK
  char logmsg[1000] ; //stores errmsgs
#endif
/*------------------------------------------------------------------------*/


  /*------------------------------------------------------------------------
      Taken from rfc3550

       The last octet of the padding contains a count of how
      many padding octets should be ignored, including itself.  Padding
      may be needed by some encryption algorithms with fixed block sizes
      or for carrying several RTP packets in a lower-layer protocol data
      unit.
  ------------------------------------------------------------------------*/
  pad_len = *(buf->data + buf->head_room + buf->len);
  buf->len -= pad_len;
#ifdef QVP_RTP_TESTFRAMEWORK
  sprintf(logmsg,"Pad_len \t\t %d, buf_len \t\t %d \r\n\n", pad_len,
                     buf->len );
  qvp_rtp_testcases_log(logmsg);
#endif
} /* end of function qvp_rtp_packet_skip_padding */

/*===========================================================================

FUNCTION QVP_RTP_PACKET_SKIP_XTN

DESCRIPTION

  This function will skip the extension.



DEPENDENCIES
  None

ARGUMENTS IN
  buf - the buffer pointer which needs a skip padding




RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succes
                     else error code.


SIDE EFFECTS
  Length of the buf is adjusted

===========================================================================*/
LOCAL void qvp_rtp_packet_skip_xtn
(
  qvp_rtp_buf_type     *buf
)
{
  uint8 *xtn_hdr;
  uint16 xtn_len;
#ifdef QVP_RTP_TESTFRAMEWORK
  char logmsg[1000] ; //stores errmsgs
#endif
/*------------------------------------------------------------------------*/


  /*------------------------------------------------------------------------

   Taken from rfc3550


    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |      defined by profile       |           length              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        header extension                       |
   |                             ....                              |

  "
   If the X bit in the RTP header is one, a variable-length header
   extension MUST be appended to the RTP header, following the CSRC list
   if present.  The header extension contains a 16-bit length field that
   counts the number of 32-bit words in the extension, excluding the
   four-octet extension header (therefore zero is a valid length).  Only
   a single extension can be appended to the RTP data header.  To allow
   multiple interoperating implementations to each experiment
   independently with different header extensions, or to allow a

   "

  ------------------------------------------------------------------------*/
  xtn_hdr = buf->data + buf->head_room;
  xtn_len = b_unpackw( xtn_hdr, 16, 16 );

  /*------------------------------------------------------------------------
        Just adjust the headeroom and len
  ------------------------------------------------------------------------*/
  buf->head_room += xtn_len + 2;
  buf->len -=  xtn_len + 2;

#ifdef QVP_RTP_TESTFRAMEWORK
  sprintf(logmsg,"Xtn_header \t\t %d, Xtn_len \t\t %d \r\n", xtn_hdr,
                     xtn_len );
  qvp_rtp_testcases_log(logmsg);
  sprintf(logmsg,"Head_room \t\t %d, buf_len \t\t %d \r\n", buf->head_room,
                     buf->len );
  qvp_rtp_testcases_log(logmsg);
#endif
} /* end of function qvp_rtp_packet_skip_padding */

/*===========================================================================

FUNCTION QVP_RTP_SET_RTP_RX_PAYLD_TYPE

DESCRIPTION

  This function establishes the payload type expected on the receive
  path.



DEPENDENCIES
  None

ARGUMENTS IN

  ctx     - The context to which the payload type is set.
  uint8   - Actual payload type used




RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succes
                     else error code.


SIDE EFFECTS
  None

===========================================================================*/
qvp_rtp_status_type qvp_rtp_set_rtp_rx_payld_type
(
  qvp_rtp_ctx_type     *ctx,
  uint8                payload
)
{
  /*------------------------------------------------------------------------
    If we do not have a good payload val or ctx in NULL bail out
  ------------------------------------------------------------------------*/
  if( !ctx || ( payload >  QVP_RTP_MAX_PAYLOAD_VAL ) )
  {

    return( QVP_RTP_ERR_FATAL );

  }

  ctx->rx_payload_type = payload;

  return( QVP_RTP_SUCCESS );

} /* end of function qvp_rtp_set_rtp_rx_payld_type */

/*===========================================================================

FUNCTION QVP_RTP_GET_RTP_RX_PAYLD_TYPE

DESCRIPTION

  This function retrieves the payload type expected on the receive
  path.



DEPENDENCIES
  None

ARGUMENTS IN

  ctx     - The context to which the payload type is set.
  payload - this will be carrying the new payload on return




RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succes
                     else error code.


SIDE EFFECTS
  None

===========================================================================*/
qvp_rtp_status_type qvp_rtp_get_rx_payld_type
(
  qvp_rtp_ctx_type     *ctx,
  uint8                *payload
)
{
  /*------------------------------------------------------------------------
    If we do not have a good payload val or ctx in NULL bail out
  ------------------------------------------------------------------------*/
  if( !ctx )
  {

    return( QVP_RTP_ERR_FATAL );

  }

  *payload = ctx->rx_payload_type;

  return( QVP_RTP_SUCCESS );

} /* end of function qvp_rtp_set_rtp_rx_payld_type */

/*===========================================================================

FUNCTION QVP_RTP_GET_RTP_TX_SSRC

DESCRIPTION

  This function returns the SSRC id of a particular stream this is only
  if there is an O/B component.



DEPENDENCIES
  None

ARGUMENTS IN

  ctx     - The context to which the payload type is get.
  ssrc    - Actual ssrc




RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succes
                     else error code.


SIDE EFFECTS
  None

===========================================================================*/
qvp_rtp_status_type qvp_rtp_get_rtp_tx_ssrc
(
  qvp_rtp_ctx_type     *ctx,
  uint32               *ssrc

)
{
  /*------------------------------------------------------------------------
    If we do not have a good payload val or ctx in NULL bail out
  ------------------------------------------------------------------------*/
  if( !ctx )
  {

    return( QVP_RTP_ERR_FATAL );

  }

  *ssrc = ctx->tx_ssrc;

  return( QVP_RTP_SUCCESS );

} /* end of function qvp_rtp_get_rtp_tx_ssrc */

#endif /* end of FEATURE_QVPHONE_RTP*/
