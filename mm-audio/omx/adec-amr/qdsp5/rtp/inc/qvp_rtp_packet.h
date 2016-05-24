#ifndef _H_QVP_RTP_PACKET_H
#define _H_QVP_RTP_PACKET_H

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

$Header: //source/qcom/qct/multimedia/videophone/ps_media/RTP/RTP_core/rel/1.0/inc/qvp_rtp_packet.h#1 $ $DateTime: 2008/04/15 18:56:58 $ $Author: yuanz $

                            EDIT HISTORY FOR FILE


when        who    what, where, why
--------    ---    ----------------------------------------------------------
10/10/06    grk    Removed qvp_rtp_get_tx_payld_type() &
                   qvp_rtp_set_rtp_tx_payld_type().
05/05/06    apr    Including RTP code in feature FEATURE_QVPHONE_RTP
07/05/05    srk    Fixed some prototypes and updated
                   externalized functions.
05/20/05    srk    Added  APIs to get payload type.
03/23/05    srk    Added prototypes for APIs to set
                   RTP payload types.
03/18/05    srk    Initial Creation.
===========================================================================*/
//#include "customer.h"
#include "qvp_rtp_api.h"
#include "qvp_rtp_buf.h"
#ifdef FEATURE_QVPHONE_RTP


/*==========================================================================

  CONSTANTS for this module
===========================================================================*/
#define  QVP_RTP_MAX_PAYLOAD_VAL   0x7f /* 7 bit field for the payload
                                         * type
                                         */
/*--------------------------------------------------------------------------
  TYPEDEF STRUCT QVP_RTP_CTX_TYPE

  The data structure which will be used as a context for each RTP stream.
  Very minimal context.
--------------------------------------------------------------------------*/
typedef struct
{

  uint32            tx_ssrc;
  uint32            sequence_no;
  uint32            use_seq;
  uint8             tx_payload_type;

  /*------------------------------------------------------------------------
    receive side value
  ------------------------------------------------------------------------*/
  uint32            rx_ssrc;
  uint8             rx_payload_type;

} qvp_rtp_ctx_type;

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
qvp_rtp_status_type qvp_rtp_ctx_init
(
  qvp_rtp_ctx_type *ctx
);

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
qvp_rtp_status_type qvp_rtp_pack
(

  qvp_rtp_ctx_type     *ctx,
  qvp_rtp_buf_type     *pkt

);

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

);

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

);

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
);

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

);

#endif /* end of FEATURE_QVPHONE_RTP */
#endif /* end of  _H_QVP_RTP_PACKET_H */
