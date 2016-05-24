/*=*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                         QVP_RTP_AMR_PROFILE . C

GENERAL DESCRIPTION

  This file contains the implementation of amr profile. amr profile acts
  as conduite inside RTP layer. The RFC which is based on is RFC3267.

EXTERNALIZED FUNCTIONS
  None.


INITIALIZATION AND SEQUENCING REQUIREMENTS

  Need to init and configure the profile before it becomes usable.


  Copyright (c) 2004 by Qualcomm Technologies, Inc.  All Rights Reserved.
  Copyright (c) 2009 by Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary.  Export of this technology or software is
  regulated by the U.S. Government. Diversion contrary to U.S. law prohibited.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*=*/

/*===========================================================================

$Header: //source/qcom/qct/multimedia/videophone/ps_media/RTP/RTP_profile/rel/1.0/src/qvp_rtp_amr_profile.c#4 $ $DateTime: 2008/10/17 03:10:28 $ $Author: apurupa $

                            EDIT HISTORY FOR FILE


when        who    what, where, why
--------    ---    ----------------------------------------------------------
17/10/08    apr    Adding support for max bundle size of 5
16/10/08    apr    Fix in configuring maxptime
14/07/08    grk    Added codec mode request (CMR) support for AMR.
12/31/07    grk    Moving maxptime/ptime to media level.
10/18/07    rk     QDJ integration. Marking silence frames
07/13/06    uma    Added new functions for validating payload configuration
28/07/06    Uma    Eliminated KDDI specific compiler flags.
                   AMR channels set to FALSE.
06/09/06    apr    RTP payload type number is copied into the network
                   buffer as part of fix for CR 95499
05/05/06    apr    Including RTP code in feature FEATURE_QVPHONE_RTP
03/20/06    uma    Replaced all malloc and free with wrappers
                   qvp_rtp_malloc & qvp_rtp_free
03/03/06    srk    Added some comments. Beautifucation. Removed all the
                   Tabs.
03/03/06    uma    Modified qvp_rtp_amr_profile_copy_config to set channels
                   invalid for KDDI. Fixes CR89187
01/30/06    apr    Added code to fill audio frame info parameters fqi,
                   and frm_present in the func qvp_rtp_amr_profile_recv
10/04/05    apr    Modifications to support silence suppression
                   A RTP packet is sent for transmission only
                   if it has atleast one speech frame
09/26/05    apr    Modifications for marker bit usage
                   using prev_silence_frame flag marker bit
                   is set.
09/09/05    srk    Started returning max_ptime valid with our default values
                   Started adjusting our bundle size with the maxptime in
                   config. Error check on maxptime config.
07/18/05    srk    Fixed KDDI interop issue with ILL/ILP
                   field in the header. This is used only
                   when interleaving is signalled.
06/26/05    srk    Added code to forgive mismatching length
                   to a loer transmittable length.
06/06/05    srk    Fixing the channles config
05/20/05    srk    Added configuration APIs
03/18/05    srk    No more RV dependancy. Fixing a
                   warning
02/11/05    srk    Initial Creation.
===========================================================================*/
//#include "customer.h"
#include "rtp_unpack.h"
#ifdef FEATURE_QVPHONE_RTP

/*===========================================================================
                     INCLUDE FILES FOR MODULE
===========================================================================*/
//#include <comdef.h>               /* common target/c-sim defines */
#include <stdio.h>
#include <string.h>               /* memory routines */
#include <stdlib.h>               /* for malloc */
#include "qvp_rtp_api.h"          /* return type propagation */
#include "qvp_rtp_profile.h"      /* profile template data type */
//#include "qvp_rtp_msg.h"
//#include "qvp_rtp_log.h"
#include "qvp_rtp_amr_profile.h" /* profile template data type */
#include "bit.h"                  /* for bit parsing/manipulation */


#define QVP_RTP_ERR printf
#define QVP_RTP_MSG_HIGH printf

/*===========================================================================
                     LOCAL STATIC FUNCTIONS
===========================================================================*/
LOCAL qvp_rtp_status_type qvp_rtp_amr_profile_init
(
  qvp_rtp_profile_config_type *config
);

LOCAL qvp_rtp_status_type qvp_rtp_amr_profile_open
(
  qvp_rtp_profile_hdl_type     *hdl,         /* handle which will get
                                              * populated with a new profile
                                              * stream handle
                                              */
  qvp_rtp_profile_usr_hdl_type  usr_hdl     /* user handle for any cb */
);


LOCAL qvp_rtp_status_type qvp_rtp_amr_profile_read_deflt_config
(

  qvp_rtp_payload_type        payld, /* not used */
  qvp_rtp_payload_config_type *payld_config
);

LOCAL qvp_rtp_status_type qvp_rtp_amr_profile_validate_config_rx
(
  qvp_rtp_payload_config_type  *payld_config,
  qvp_rtp_payload_config_type  *counter_config
);

LOCAL qvp_rtp_status_type qvp_rtp_amr_profile_validate_config_tx
(
  qvp_rtp_payload_config_type  *payld_config,
  qvp_rtp_payload_config_type  *counter_config
);

LOCAL qvp_rtp_status_type qvp_rtp_amr_profile_configure
(
  qvp_rtp_profile_hdl_type      hdl,
  qvp_rtp_payload_config_type   *payld_config

);


//LOCAL qvp_rtp_status_type qvp_rtp_amr_profile_send
//(
//  qvp_rtp_profile_hdl_type     hdl,          /* handle created while open */
//  qvp_rtp_profile_usr_hdl_type usr_hdl,      /* user handle for tx cb */
//  qvp_rtp_buf_type             *pkt          /* packet to be formatted
//                                              * through the profile
//                                              */
//);

#if 0
qvp_rtp_status_type qvp_rtp_amr_profile_recv
(

  qvp_rtp_profile_hdl_type     hdl,          /* handle created while open */
  qvp_rtp_profile_usr_hdl_type usr_hdl,      /* user handle for rx cb */
  qvp_rtp_buf_type             *pkt          /* packet parsed and removed
                                              * header
                                              */

);
#endif

LOCAL qvp_rtp_status_type qvp_rtp_amr_profile_close
(

  qvp_rtp_profile_hdl_type     hdl           /* handle the profile */

);


LOCAL void qvp_rtp_amr_profile_shutdown( void );

/*--------------------------------------------------------------------------
      FUNCTIONS APART FROM PROFILE TEMPLATE
--------------------------------------------------------------------------*/
LOCAL void qvp_rtp_amr_reset_tx_ctx
(
  qvp_rtp_amr_ctx_type *stream
);

LOCAL void qvp_rtp_amr_reset_stream
(
  qvp_rtp_amr_ctx_type *stream
);

LOCAL uint8 qvp_rtp_amr_form_header_oa
(
  qvp_rtp_amr_ctx_type *stream
);

//LOCAL uint16 qvp_rtp_parse_amr_fixed_hdr_oa
//(
//
//  uint8 *data,
//  uint16 len,
//  qvp_rtp_amr_oal_hdr_param_type *hdr
//
//);

LOCAL qvp_rtp_status_type qvp_rtp_amr_profile_validate_fmtp
(
  qvp_rtp_channel_type         ch_dir,
  qvp_rtp_amr_sdp_config_type  amr_config,
  qvp_rtp_amr_sdp_config_type  *counter_amr_config
);


LOCAL qvp_rtp_status_type qvp_rtp_amr_profile_copy_config
(
  qvp_rtp_payload_config_type *payld_config,
  qvp_rtp_amr_config_type     *amr_config
);

LOCAL qvp_rtp_status_type qvp_rtp_amr_profile_config_tx_param
(
  qvp_rtp_amr_ctx_type         *stream,
  qvp_rtp_payload_config_type  *payld_config
);

LOCAL qvp_rtp_status_type qvp_rtp_amr_profile_config_rx_param
(
  qvp_rtp_amr_ctx_type         *stream,
  qvp_rtp_payload_config_type  *payld_config
);

/*LOCAL qvp_rtp_amr_toc_type qvp_rtp_amr_find_FT
(
  uint16 len
);*/

uint16 qvp_rtp_amr_count_tocs_oa
(
  uint8 *data,
  uint16 len
);

/*--------------------------------------------------------------------------
    Table of accessors for the profile. This need to be populated in
    the table entry at qvp_rtp_profole.c. All these functions will
    automatically link to the RTP stack by doing so.
--------------------------------------------------------------------------*/
qvp_rtp_profile_type qvp_rtp_amr_profile_table =
{
  qvp_rtp_amr_profile_init,              /* LOACAL initialization routine */
  qvp_rtp_amr_profile_open,              /* LOCAL function to open channel*/
  NULL, //  qvp_rtp_amr_profile_send,              /* LOCAL function to send a pkt  */
  NULL,  //qvp_rtp_amr_profile_recv,              /* LOCAL function to rx a nw pkt */
  qvp_rtp_amr_profile_close,             /* LOCAL function to close chn   */
  qvp_rtp_amr_profile_read_deflt_config, /* func to read dflt module conf */
  NULL,
  qvp_rtp_amr_profile_validate_config_rx,/* LOCAL func to validate rx conf*/
  qvp_rtp_amr_profile_validate_config_tx,/* LOCAL func to validate rx conf */
  qvp_rtp_amr_profile_configure,         /* LOCAL func to config profile   */
  qvp_rtp_amr_profile_shutdown           /* LOCAL func to shutdown module  */
};

/*===========================================================================
                     LOCAL STATIC DATA
===========================================================================*/

LOCAL boolean amr_initialized = FALSE;

/*--------------------------------------------------------------------------
     Stores the configuration requested by the app.
--------------------------------------------------------------------------*/
LOCAL  qvp_rtp_profile_config_type amr_profile_config;

/*--------------------------------------------------------------------------
        Stream context array.  Responsible for each bidirectional
        connections
--------------------------------------------------------------------------*/
LOCAL qvp_rtp_amr_ctx_type *qvp_rtp_amr_array = NULL;

/*--------------------------------------------------------------------------
     Configuration pertaining payload formatting for MP4
--------------------------------------------------------------------------*/
LOCAL qvp_rtp_amr_config_type amr_stream_config;

/*--------------------------------------------------------------------------
  LOCAL table to parse the length

  TAKEN FROM RFC3267.

                   Class A   total speech
   Index   Mode     bits     bits
   ----------------------------------------
   0     AMR 4.75   42         95
   1     AMR 5.15   49        103
   2     AMR 5.9    55        118
   3     AMR 6.7    58        134
   4     AMR 7.4    61        148
   5     AMR 7.95   75        159
   6     AMR 10.2   65        204
   7     AMR 12.2   81        244
   8     AMR SID    39         39
--------------------------------------------------------------------------*/
int16 qvp_rtp__amr_ft_len_table[ QVP_RTP_AMR_FT_RATE_SID + 1 ] =
{

  95,  /* 4.75 kbps */
  103, /* 5.15 kbps */
  118, /* 118 */
  134, /* 6.7 kbps */
  148, /* 7.4 kbps */
  159, /* 7.95 kbps */
  204, /* 10.2 kbps */
  244, /* 12.2 kbps */
  39   /* SID - 1.95 kbps */

};


/*===========================================================================

FUNCTION QVP_RTP_AMR_PROFILE_INIT


DESCRIPTION
  Initializes data structures and resources used by the amr profile.

DEPENDENCIES
  None

ARGUMENTS IN
  config - pointer to profile configuration requested.

RETURN VALUE
  QVP_RTP_SUCCESS  - if we could initialize the profile. or an error code


SIDE EFFECTS
  None.

===========================================================================*/
LOCAL qvp_rtp_status_type qvp_rtp_amr_profile_init
(
  qvp_rtp_profile_config_type *config
)
{
/*------------------------------------------------------------------------*/


  if( amr_initialized )
  {
    return( QVP_RTP_SUCCESS );
  }

  /*------------------------------------------------------------------------
    Init if we get a valid config  and appropriate call backs
    for the basic operation of the profile.
  ------------------------------------------------------------------------*/
  if( config && config->rx_cb && config->tx_cb )
  {
    memcpy( &amr_profile_config, config, sizeof( amr_profile_config ) );
  }
  else
  {
    return( QVP_RTP_WRONG_PARAM );
  }

  /*------------------------------------------------------------------------
    Allocate the number of streams first
  ------------------------------------------------------------------------*/
  /*qvp_rtp_amr_array  = qvp_rtp_malloc( config->num_streams *
                                sizeof( qvp_rtp_amr_ctx_type ) );*/

  qvp_rtp_amr_array  = (qvp_rtp_amr_ctx_type *)malloc(sizeof(qvp_rtp_amr_ctx_type) * config->num_streams);

  /*------------------------------------------------------------------------
      See if we could get the memory
  ------------------------------------------------------------------------*/
  if( !qvp_rtp_amr_array )
  {
    return( QVP_RTP_ERR_FATAL );
  }
  else
  {
    memset( qvp_rtp_amr_array, 0, config->num_streams *
                    sizeof( qvp_rtp_amr_ctx_type ) );

    /*----------------------------------------------------------------------
        Set up the default configuration
    ----------------------------------------------------------------------*/

    amr_stream_config.rx_octet_aligned = TRUE;
    amr_stream_config.rx_crc_on = FALSE;
    amr_stream_config.rx_rob_sort_on  = FALSE;
    amr_stream_config.rx_UED_UEP_on   = FALSE;
    amr_stream_config.rx_interleave_on = FALSE;
    amr_stream_config.rx_amr_pkt_interval = QVP_RTP_AMR_PKT_INTERVAL;
    amr_stream_config.rx_ptime = QVP_RTP_AMR_DFLT_PTIME;
    amr_stream_config.rx_max_ptime = QVP_RTP_DFLT_BUNDLE_SIZE *
                                      QVP_RTP_AMR_DFLT_PTIME;

    amr_stream_config.tx_header_on   = TRUE;
    amr_stream_config.tx_interleave_on = FALSE;
    amr_stream_config.tx_bundle_size = QVP_RTP_DFLT_BUNDLE_SIZE;
    amr_stream_config.tx_octet_aligned = TRUE;
    amr_stream_config.tx_UED_UEP_on   = FALSE;
    amr_stream_config.tx_repetition_on = FALSE;
    amr_stream_config.tx_rob_sort_on  = FALSE;
    amr_stream_config.tx_crc_on = FALSE;
    amr_stream_config.tx_ptime = QVP_RTP_AMR_DFLT_PTIME;
    amr_stream_config.tx_max_ptime = QVP_RTP_DFLT_BUNDLE_SIZE *
                                      QVP_RTP_AMR_DFLT_PTIME;

  }

  /*------------------------------------------------------------------------
    Flag initialization
  ------------------------------------------------------------------------*/
  amr_initialized  = TRUE;

  return( QVP_RTP_SUCCESS );

}/* end of function qvp_rtp_amr_profile_init */

/*===========================================================================

FUNCTION  QVP_RTP_AMR_PROFILE_SEND


DESCRIPTION
  request to send specified buffer through this profile.

DEPENDENCIES
  None

ARGUMENTS IN
  hdl     - handle into the profile.
  usr_hdl - handle to use when we call send function using the registered
            tx callback
  pkt     - packet of data to be sent.

RETURN VALUE
  QVP_RTP_SUCCESS  - if we could send data. or an error code


SIDE EFFECTS
  None.

===========================================================================*/
//LOCAL qvp_rtp_status_type qvp_rtp_amr_profile_send
//(
//  qvp_rtp_profile_hdl_type     hdl,          /* handle created while open */
//  qvp_rtp_profile_usr_hdl_type usr_hdl,      /* user handle for tx cb */
//  qvp_rtp_buf_type             *pkt          /* packet to be formatted
//                                              * through the profile
//                                              */
//)
//{
//  qvp_rtp_amr_ctx_type *stream = ( qvp_rtp_amr_ctx_type *) hdl;
//  uint8                toc = 0;
//  uint8                FT;
//  qvp_rtp_buf_type *nw_buf = NULL;
//  uint16            pkt_len;
//  uint8             rtp_pyld_type;
///*------------------------------------------------------------------------*/
//
//  /*------------------------------------------------------------------------
//                    Are we sane here ..?
//  ------------------------------------------------------------------------*/
//  if( !amr_initialized || !stream || !stream->valid ||
//      !amr_profile_config.tx_cb  || !pkt )
//  {
//    if( pkt )
//    {
//      qvp_rtp_free_buf( pkt );
//    }
//    return( QVP_RTP_ERR_FATAL );
//  }
//  /*------------------------------------------------------------------------
//    Store the payload type number
//  ------------------------------------------------------------------------*/
//  rtp_pyld_type = pkt->rtp_pyld_type;
//
//  /*------------------------------------------------------------------------
//
//    if there is more frames in this bundle set F Bit to 1
//  ------------------------------------------------------------------------*/
//  if( (stream->tx_ctx.frame_cnt + 1 ) /* this pkt*/ !=
//        stream->stream_config.tx_bundle_size  )
//  {
//
//    toc = 0x80;
//  }
//
//  /*------------------------------------------------------------------------
//     Set Q bit to TRUE When we send out
//  ------------------------------------------------------------------------*/
//  toc |= QVP_RTP_AMR_OL_Q_BIT;
//
//  /*------------------------------------------------------------------------
//    Length adjustment for SID and zero len
//
//    THIS IS REALLY A HACK SHOULD BE REMOVED LATER
//  ------------------------------------------------------------------------*/
//  if( ( pkt->len == 1 ) || ( pkt->len == 6 ) )
//  {
//    pkt->len--;
//  }
//
//
//  /*------------------------------------------------------------------------
//     If this is the first sample of the bundle cache the time stamp
//  ------------------------------------------------------------------------*/
//  if( stream->tx_ctx.frame_cnt == 0 )
//  {
//    stream->tx_ctx.first_tstamp = pkt->tstamp;
//  }
//
//  /*------------------------------------------------------------------------
//               If packet len is zero this is a silent packet
//  ------------------------------------------------------------------------*/
//  if( !pkt->len )
//  {
//
//    /*----------------------------------------------------------------------
//        Flag it if it is a silent frame
//    ----------------------------------------------------------------------*/
//    stream->tx_ctx.prev_silence_frame = TRUE;
//
//    /*----------------------------------------------------------------------
//        RFC3267 Octet aligned mode
//
//       0 1 2 3 4 5 6 7
//       +-+-+-+-+-+-+-+-+
//       |F|  FT   |Q|P|P|
//       +-+-+-+-+-+-+-+-+
//    ----------------------------------------------------------------------*/
//    toc |= ( QVP_RTP_AMR_FT_RATE_NOD << 3 );
//
//    /*----------------------------------------------------------------------
//      Stick in the no data toc here
//    ----------------------------------------------------------------------*/
//    *(stream->tx_ctx.op_packet + QVP_RTP_AMR_OL_FIXED_HDR_SIZE +
//       stream->tx_ctx.frame_cnt ) = toc;
//
//
//  }
//  else
//  {
//
//
//    /*----------------------------------------------------------------------
//       See if the length matches any of the TOCs
//    ----------------------------------------------------------------------*/
//    FT =  qvp_rtp_amr_find_FT( pkt->len );
//
//
//    /*----------------------------------------------------------------------
//      check for valid toc
//    ----------------------------------------------------------------------*/
//    if( FT ==  QVP_RTP_AMR_FT_INVALID )
//    {
//      QVP_RTP_ERR( " wrong pkt len in amr send \r\n", 0, 0, 0);
//      qvp_rtp_free_buf( pkt  );
//      return( QVP_RTP_ERR_FATAL );
//    }
//    else
//    {
//
//      /*--------------------------------------------------------------------
//           check if it is a silence frame
//      --------------------------------------------------------------------*/
//      if( ( FT == QVP_RTP_AMR_FT_RATE_NOD ) ||
//          ( FT == QVP_RTP_AMR_FT_RATE_SID ) )
//      {
//        stream->tx_ctx.prev_silence_frame = TRUE;
//      }
//      else
//      {
//
//        /*------------------------------------------------------------------
//          for a speech frame check if it is the first frame in the RTP
//          packet
//        ------------------------------------------------------------------*/
//        if( stream->tx_ctx.frame_cnt == 0 &&
//            stream->tx_ctx.prev_silence_frame )
//        {
//          stream->tx_ctx.marker = TRUE;
//        }
//
//        /*------------------------------------------------------------------
//          Set the prev_silence_frame flag to false since its a speech frame
//        ------------------------------------------------------------------*/
//        stream->tx_ctx.prev_silence_frame = FALSE;
//      }
//
//      /*--------------------------------------------------------------------
//        check if it is a speech frame other than NODATA
//      --------------------------------------------------------------------*/
//      if( FT != QVP_RTP_AMR_FT_RATE_NOD )
//      {
//        stream->tx_ctx.all_silence_frames = FALSE;
//      }
//
//      /*--------------------------------------------------------------------
//                      Len is non zero
//                      RFC3267 TOC entry in octet aligned mode
//
//                       0 1 2 3 4 5 6 7
//                       +-+-+-+-+-+-+-+-+
//                       |F|  FT   |Q|P|P|
//                       +-+-+-+-+-+-+-+-+
//      --------------------------------------------------------------------*/
//      toc |= ( FT << 3 );
//
//
//      *(stream->tx_ctx.op_packet + QVP_RTP_AMR_OL_FIXED_HDR_SIZE +
//          stream->tx_ctx.frame_cnt ) = toc;
//
//
//      /*--------------------------------------------------------------------
//              concatenate the frame into our n/w buffer
//      --------------------------------------------------------------------*/
//      memcpy( stream->tx_ctx.op_packet +
//             QVP_RTP_AMR_OL_FIXED_HDR_SIZE +
//              stream->stream_config.tx_bundle_size
//                + stream->tx_ctx.data_len,
//                pkt->data  + pkt->head_room, pkt->len );
//
//      /*--------------------------------------------------------------------
//            Add to the data len for the newly copied frame
//      --------------------------------------------------------------------*/
//      stream->tx_ctx.data_len += pkt->len;
//
//    }
//  }
//
//  /*------------------------------------------------------------------------
//    then free the buffer
//  ------------------------------------------------------------------------*/
//  qvp_rtp_free_buf( pkt );
//
//
//  /*------------------------------------------------------------------------
//      Frame count should be incrementd on each frame
//
//      We will ship the bundle when frame count equals bundle size
//  ------------------------------------------------------------------------*/
//  stream->tx_ctx.frame_cnt++;
//
//
//  /*------------------------------------------------------------------------
//              If we reached the bundle size lets ship it out
//  ------------------------------------------------------------------------*/
//  if( stream->tx_ctx.frame_cnt == stream->stream_config.tx_bundle_size )
//  {
//    QVP_RTP_MSG_HIGH( " frame cnt %d \r\n", stream->tx_ctx.frame_cnt, 0, 0);
//    /*----------------------------------------------------------------------
//      Send RTP packet only if it has speech frames
//    ----------------------------------------------------------------------*/
//    if( !stream->tx_ctx.all_silence_frames )
//    {
//
//      /*----------------------------------------------------------------------
//        Precalculate the packet length
//      ----------------------------------------------------------------------*/
//      pkt_len  = QVP_RTP_AMR_OL_FIXED_HDR_SIZE +  /* amr fixed header len */
//                        stream->stream_config.tx_bundle_size /* toc len */
//                        + stream->tx_ctx.data_len;  /* data already copied */
//
//
//      /*----------------------------------------------------------------------
//       try and allocate a buffer
//      ----------------------------------------------------------------------*/
//      nw_buf  = qvp_rtp_alloc_buf_by_len( pkt_len + QVP_RTP_HEAD_ROOM );
//
//      if( !nw_buf )
//      {
//        QVP_RTP_ERR(" Failed allocate a buffer arrggggggg....", 0, 0, 0 );
//        qvp_rtp_amr_reset_tx_ctx( stream );
//        return( QVP_RTP_ERR_FATAL );
//      }
//
//
//      /*----------------------------------------------------------------------
//        Set up room for RTP headers
//      ----------------------------------------------------------------------*/
//      nw_buf->head_room = QVP_RTP_HEAD_ROOM;
//
//
//      /*----------------------------------------------------------------------
//        Copy the payload data
//      ----------------------------------------------------------------------*/
//      memcpy( nw_buf->data + nw_buf->head_room, stream->tx_ctx.op_packet,
//              pkt_len );
//
//
//      /*----------------------------------------------------------------------
//        set up other RTP fields
//      ----------------------------------------------------------------------*/
//      nw_buf->len = pkt_len;
//      nw_buf->marker_bit = stream->tx_ctx.marker;
//      nw_buf->tstamp  = stream->tx_ctx.first_tstamp;
//      nw_buf->rtp_pyld_type = rtp_pyld_type;
//
//      QVP_RTP_MSG_HIGH( " Sending AMR bundle len = %d, timestamp = %d, 
//                          marker = %d", nw_buf->len, nw_buf->tstamp,
//                          nw_buf->marker_bit );
//
//      /*----------------------------------------------------------------------
//                    Ship the packet out
//      ----------------------------------------------------------------------*/
//      amr_profile_config.tx_cb( nw_buf, usr_hdl );
//    }
//
//    /*----------------------------------------------------------------------
//                We have sent the packet so lets reset it for next bundle
//    ----------------------------------------------------------------------*/
//    qvp_rtp_amr_reset_tx_ctx( stream );
//
//  }
//
//  return( QVP_RTP_SUCCESS );
//
//} /* end of function qvp_rtp_amr_profile_send */
/*===========================================================================

FUNCTION  QVP_RTP_AMR_PROFILE_OPEN


DESCRIPTION
    Requests to open a bi directional channel within the profile.

DEPENDENCIES
  None

ARGUMENTS IN
  usr_hdl       - handle to use when we call send function using the
                  registered tx callback

ARGUMENTS OUT
  hdl           - on success we write the handle to the profiile into
                  the  passed double pointer

RETURN VALUE
  QVP_RTP_SUCCESS  - if we could send data. or an error code


SIDE EFFECTS
  None.

===========================================================================*/
LOCAL qvp_rtp_status_type qvp_rtp_amr_profile_open
(
  qvp_rtp_profile_hdl_type     *hdl,         /* handle which will get
                                              * populated with a new profile
                                              * stream handle
                                              */
  qvp_rtp_profile_usr_hdl_type  usr_hdl     /* user handle for any cb */
)
{
  uint32 i;   /* index variable */
/*------------------------------------------------------------------------*/

  (void)usr_hdl;
  /*------------------------------------------------------------------------
    Bail out if we are not initialized yet
  ------------------------------------------------------------------------*/
  if( !amr_initialized )
  {
    return( QVP_RTP_ERR_FATAL );
  }

  /*------------------------------------------------------------------------
              look at an idle stream
  ------------------------------------------------------------------------*/
  for( i = 0;  i < amr_profile_config.num_streams; i++ )
  {


    /*----------------------------------------------------------------------
              Fish for free entries
    ----------------------------------------------------------------------*/
    if( !qvp_rtp_amr_array[ i ].valid )
    {
      qvp_rtp_amr_array[ i ].valid = TRUE;
      *hdl = &qvp_rtp_amr_array[ i ];

      /*--------------------------------------------------------------------
            copy the amr configuration to the  stream_ctx
      --------------------------------------------------------------------*/
      memcpy( &qvp_rtp_amr_array[i].stream_config, &amr_stream_config,
               sizeof( qvp_rtp_amr_array[i].stream_config ));

      /*--------------------------------------------------------------------
          Reset the stream context - if any stale values in there.
      --------------------------------------------------------------------*/
      qvp_rtp_amr_reset_stream( &qvp_rtp_amr_array[i] );


      /*--------------------------------------------------------------------
        Form a  predetermined header. There is nothing which will change
        from bundle to bundle
      --------------------------------------------------------------------*/
      qvp_rtp_amr_array[i].tx_ctx.tx_hdr_len =
                      qvp_rtp_amr_form_header_oa( &qvp_rtp_amr_array[i] );

      return( QVP_RTP_SUCCESS );

    }

  } /* maximum no of streams */

  return( QVP_RTP_NORESOURCES );


} /* end of function qvp_rtp_amr_profile_open */


/*===========================================================================

FUNCTION QVP_RTP_AMR_PROFILE_CONFIGURE

DESCRIPTION

  This function payload configuration of a previously opened EVRC
  channel.

DEPENDENCIES
  None

ARGUMENTS IN
    hdl            - handle to the opend channel
    payld_config   - pointer to configuration structure




RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesfull.
  else - Attempted a configuration  which is not currently supported by
         the implementation.


SIDE EFFECTS
  None.

===========================================================================*/
LOCAL qvp_rtp_status_type qvp_rtp_amr_profile_configure
(
  qvp_rtp_profile_hdl_type      hdl,
  qvp_rtp_payload_config_type   *payld_config

)
{
  qvp_rtp_amr_ctx_type    *stream = ( qvp_rtp_amr_ctx_type *) hdl;
  qvp_rtp_amr_config_type prev_config;
  qvp_rtp_status_type     status = QVP_RTP_SUCCESS;
/*------------------------------------------------------------------------*/

  /*------------------------------------------------------------------------
    Are we sane here ..?
  ------------------------------------------------------------------------*/
  if( !amr_initialized || !stream || !stream->valid ||
      !payld_config )
  {
    return( QVP_RTP_ERR_FATAL );
  }



  /*------------------------------------------------------------------------
    Backup the current configuration before we proceed from here.
  ------------------------------------------------------------------------*/
  prev_config = stream->stream_config;

  /*------------------------------------------------------------------------
      Configuration steps.

      1) Try and configure RX.
         If RX fails.
          Reset it to previous state and return error.
          This way application can easily implement OFFER/ANSWER model as
          given in RFC3264.
      2) Try and configure TX
         If TX  configure fails.
          Reset it to previous state and return error.
          This way application can easily implement OFFER/ANSWER model
          as given in RFC3264.
  ------------------------------------------------------------------------*/

  if( payld_config->config_rx_params.valid )
  {
    status = qvp_rtp_amr_profile_config_rx_param( stream, payld_config );
  }


  /*------------------------------------------------------------------------
      Try and configure Tx part now....
  ------------------------------------------------------------------------*/
  if( ( status == QVP_RTP_SUCCESS )&&
      payld_config->config_tx_params.valid )
  {
    status = qvp_rtp_amr_profile_config_tx_param(stream, payld_config );

  }

  /*------------------------------------------------------------------------
    If we did not succeed to a struct to reset to preivous value and
    return failure
  ------------------------------------------------------------------------*/
  if( status != QVP_RTP_SUCCESS )
  {
    stream->stream_config = prev_config ;
  }

  return( status );

} /* end of function qvp_rtp_amr_profile_configure */

/*===========================================================================

FUNCTION QVP_RTP_EVRC_PROFILE_READ_DEFLT_CONFIG


DESCRIPTION

  This function reads out the default configuration of EVRC
  payload format. The result is conveyed by populating the passed in
  structure on return.

DEPENDENCIES
  None

ARGUMENTS IN
    payload        - payload type for which the default configuration
                     is being read out. This is not used by EVRC. Some
                     other profiles uses it as they support multiple
                     payload formats.

ARGUMENTS OUT
    payld_config   - pointer to configuration structure. On return this
                     structure will be populated with the default values
                     for the profile.




RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesful.
                    appropriate error code otherwise


SIDE EFFECTS
  payld_config structure will be populated with the default values
  for the particular profile.

===========================================================================*/
LOCAL qvp_rtp_status_type qvp_rtp_amr_profile_read_deflt_config
(

  qvp_rtp_payload_type        payld, /* not used */
  qvp_rtp_payload_config_type *payld_config
)
{
  (void)payld;
  /*------------------------------------------------------------------------
    If profile config is NULL return error
  ------------------------------------------------------------------------*/
  if( !payld_config )
  {

    return( QVP_RTP_ERR_FATAL );
  }


  return( qvp_rtp_amr_profile_copy_config( payld_config,
                                            &amr_stream_config  ) );



} /* end of function qvp_rtp_amr_profile_read_deflt_config */


/*===========================================================================

FUNCTION QVP_RTP_AMR_PROFILE_VALIDATE_CONFIG


DESCRIPTION

  This function checks the configuration of AMR specified in the input
  against the valid fmtp values for AMR

DEPENDENCIES
  None

ARGUMENTS IN
  payld_config - configuration which needs to be validated

ARGUMENTS OUT
  counter_config - If the attribute  is valid, counter_config is not modified
                   If the attribute  is invalid,
                   counter_config is updated with proposed value

RETURN VALUE
  QVP_RTP_SUCCESS  - The given Configuration is  valid for the given payload,
                     RTP configure will succeed for these settings

  QVP_RTP_ERR_FATAL - The given Configuration is not valid for the given
                        payload. RTP configure will fail for these settings

SIDE EFFECTS
  None

===========================================================================*/
LOCAL qvp_rtp_status_type qvp_rtp_amr_profile_validate_config_rx
(
  qvp_rtp_payload_config_type  *payld_config,
  qvp_rtp_payload_config_type  *counter_config
)
{
  qvp_rtp_status_type status = QVP_RTP_SUCCESS;
  /*----------------------------------------------------------------------*/

  /*------------------------------------------------------------------------
    If profile config is NULL return error
  ------------------------------------------------------------------------*/
  if ( ( !payld_config ) || ( !counter_config ) )
  {
    return( QVP_RTP_ERR_FATAL );
  }

  /*------------------------------------------------------------------------
    Validating rx configuration
  ------------------------------------------------------------------------*/
  if ( ( payld_config->config_rx_params.valid ) &&
       ( payld_config->config_rx_params.
             config_rx_payld_params.amr_rx_config.valid ) )
  {
    if ( ( payld_config->ch_dir == QVP_RTP_CHANNEL_INBOUND ) ||
         ( payld_config->ch_dir == QVP_RTP_CHANNEL_FULL_DUPLEX ) )
    {
      /*--------------------------------------------------------------------
        Validate ptime. Throw an error if we cant receive packets with
        the ptime proposed.
      --------------------------------------------------------------------*/
      if ( payld_config->config_rx_params.rx_rtp_param.ptime_valid &&
           ( payld_config->config_rx_params.rx_rtp_param.ptime !=
                     QVP_RTP_AMR_DFLT_PTIME ) )
      {
        counter_config->config_rx_params.rx_rtp_param.ptime =
                    QVP_RTP_AMR_DFLT_PTIME ;
        status = QVP_RTP_ERR_FATAL;
      }
      /*--------------------------------------------------------------------
        Validate maxptime. Throw an error if we cant receive packets with
        the maxptime proposed.
      --------------------------------------------------------------------*/
      if ( ( payld_config->config_rx_params.rx_rtp_param.maxptime_valid )
          && ( ( payld_config->config_rx_params.rx_rtp_param.maxptime <
                        QVP_RTP_AMR_DFLT_PTIME ) ||
              ( payld_config->config_rx_params.rx_rtp_param.maxptime >
                 QVP_RTP_MAX_BUNDLE_SIZE *
                                           QVP_RTP_AMR_DFLT_PTIME ) ) )
      {
        counter_config->config_tx_params.tx_rtp_param.maxptime =
                   QVP_RTP_DFLT_BUNDLE_SIZE * QVP_RTP_AMR_DFLT_PTIME ;
        status = QVP_RTP_ERR_FATAL;

      }
      /*--------------------------------------------------------------------
        Attempting to configure rx for an inbound/ duplex channel
      --------------------------------------------------------------------*/
      status = qvp_rtp_amr_profile_validate_fmtp (
                payld_config->ch_dir,
                (payld_config->config_rx_params.config_rx_payld_params.
                                                           amr_rx_config) ,
                & (counter_config->config_rx_params.config_rx_payld_params.
                                                           amr_rx_config) );
    }/* end of if ( ( payld_config->ch_dir */
    else
    {
      QVP_RTP_ERR ( " Attempting to config rx for an o/b only ch %d %d %d\n", 0, 0, 0);
      status = QVP_RTP_ERR_FATAL ;
    }
  } /* end of   if ( ( payld_config->config_rx_params.valid )*/

  return ( status );
}/* end of qvp_rtp_amr_profile_validate_config_rx */


/*===========================================================================

FUNCTION QVP_RTP_AMR_PROFILE_VALIDATE_CONFIG_TX


DESCRIPTION

  This function checks the configuration of AMR specified in the input
  against the valid fmtp values for AMR

DEPENDENCIES
  None

ARGUMENTS IN
  payld_config - configuration which needs to be validated

ARGUMENTS OUT
  counter_config - If the attribute  is valid, counter_config is not modified
                   If the attribute  is invalid,
                   counter_config is updated with proposed value


RETURN VALUE
  QVP_RTP_SUCCESS  - The given Configuration is  valid for the given payload,
                     RTP configure will succeed for these settings
  QVP_RTP_ERR_FATAL   - The given Configuration is not valid for the given
                        payload. RTP configure will fail for these settings

SIDE EFFECTS
  None
===========================================================================*/
LOCAL qvp_rtp_status_type qvp_rtp_amr_profile_validate_config_tx
(
  qvp_rtp_payload_config_type*  payld_config,
  qvp_rtp_payload_config_type*  counter_config
)
{
  qvp_rtp_status_type status = QVP_RTP_SUCCESS;
/*------------------------------------------------------------------------*/

  /*------------------------------------------------------------------------
    If profile config is NULL return error
  ------------------------------------------------------------------------*/
  if( !payld_config )
  {
    return( QVP_RTP_ERR_FATAL );
  }

  /*------------------------------------------------------------------------
    Validating tx configuration
  ------------------------------------------------------------------------*/
  if ( ( payld_config->config_tx_params.valid ) &&
       ( payld_config->config_tx_params.
             config_tx_payld_params.amr_tx_config.valid ) )
  {
    if ( ( payld_config->ch_dir == QVP_RTP_CHANNEL_OUTBOUND ) ||
         ( payld_config->ch_dir == QVP_RTP_CHANNEL_FULL_DUPLEX ) )
    {
      /*--------------------------------------------------------------------
        Validate ptime. Throw an error if peer wants us to send
        packets with ptime lesser than our minimum supported ptime.
      --------------------------------------------------------------------*/
      if ( payld_config->config_tx_params.tx_rtp_param.ptime_valid &&
           ( payld_config->config_tx_params.tx_rtp_param.ptime <
                     QVP_RTP_AMR_DFLT_PTIME ) )
      {
        counter_config->config_tx_params.tx_rtp_param.ptime =
                        QVP_RTP_AMR_DFLT_PTIME ;
        status = QVP_RTP_ERR_FATAL;
      }
      /*--------------------------------------------------------------------
        Validate maxptime. Throw an error if peer wants us to send
        packets with maxptime lesser than our minimum supported maxptime.
      --------------------------------------------------------------------*/
      if ( ( payld_config->config_tx_params.tx_rtp_param.maxptime_valid )
           && ( payld_config->config_tx_params.tx_rtp_param.maxptime <
                   QVP_RTP_AMR_DFLT_PTIME ) )
      {
        counter_config->config_tx_params.tx_rtp_param.maxptime =
          QVP_RTP_DFLT_BUNDLE_SIZE * QVP_RTP_AMR_DFLT_PTIME ;
        status = QVP_RTP_ERR_FATAL;

      }
      /*--------------------------------------------------------------------
      Attempting to configure tx for an outbound/ duplex channel
      --------------------------------------------------------------------*/
      status = qvp_rtp_amr_profile_validate_fmtp (
                payld_config->ch_dir,
                (payld_config->config_tx_params.config_tx_payld_params.
                                                            amr_tx_config),
                & (counter_config->config_tx_params.config_tx_payld_params.
                                                            amr_tx_config) );
    }/* end of if ( ( payld_config->ch_dir */
    else
    {
      QVP_RTP_ERR ( " Attempting to config tx for an i/b only ch  %d %d %d\n", 0, 0, 0);
      status = QVP_RTP_ERR_FATAL ;
    }
  }/* end of if ( ( payld_config->config_tx_params.valid )*/

  return ( status );

}/* end of qvp_rtp_amr_profile_validate_config_tx */


/*===========================================================================

FUNCTION QVP_RTP_AMR_PROFILE_VALIDATE_FMTP


DESCRIPTION

  This function checks the configuration of AMR specified in the input
  against the valid fmtp values for AMR

  This is the generic func used for both tx & rx params since most of the
  parameters have the same valid range in both the directions.

  In case of any assymetry,the differences have to be accomodated
  using channel direction
  or this can be split into separate functions on tx&rx plane


DEPENDENCIES
  None

ARGUMENTS IN
  payld_config - configuration which needs to be validated
  ch_dir       - This gives the information whether the Tx or Rx plane
                 has to be configured.
                 Currently not used since the Tx & Rx plane are symmetrical

ARGUMENTS OUT
  counter_config - If the attribute is valid, counter_config is not modified
                   If the attribute  is invalid,
                   counter_config is updated with proposed value

                   counter_config has the basic capabilities.
                   In future additional support can be added to come up with
                   an enhanced configuration


RETURN VALUE
  QVP_RTP_SUCCESS   - The given Config is  valid for the given payload,
                      RTP configure will succeed for these settings
  QVP_RTP_ERR_FATAL - The given Configuration is not valid for the given
                      payload. RTP configure will fail for these settings

SIDE EFFECTS
  None

===========================================================================*/
LOCAL qvp_rtp_status_type qvp_rtp_amr_profile_validate_fmtp
(
  qvp_rtp_channel_type ch_dir, /* not used for now*/
  qvp_rtp_amr_sdp_config_type      amr_config,
  qvp_rtp_amr_sdp_config_type*     counter_amr_config
)
{
  qvp_rtp_status_type status = QVP_RTP_SUCCESS;
/*------------------------------------------------------------------------*/
  (void)ch_dir;
  if ( !counter_amr_config )
  {
    return( QVP_RTP_ERR_FATAL );
  }

  if ( !amr_config.valid )
  {
    return (status );
  }

  /*------------------------------------------------------------------------
    Do not give up on the first invalid value
    Validate all values so that app knows
    all the unsupported values and can use counter config
  ------------------------------------------------------------------------*/

  if ( amr_config.octet_align_valid  &&  ( !amr_config.octet_align ) )
   /* bandwidth efficient mode not supported */
  {
    counter_amr_config->octet_align = 1 ;
    status = QVP_RTP_ERR_FATAL;
  }

  if ( amr_config.crc_valid && amr_config.crc )  /* crc not supported */
  {
    counter_amr_config->crc = 0 ;
    status = QVP_RTP_ERR_FATAL;
  }

  if ( amr_config.robust_sorting_valid && amr_config.robust_sorting )
  /* robust_sorting not supported */
  {
    counter_amr_config->robust_sorting = 0 ;
    status = QVP_RTP_ERR_FATAL;
  }

  if ( amr_config.interleaving_valid && amr_config.interleaving )
  /* interleaving not supported */
  {
    counter_amr_config->interleaving = 0 ;
    status = QVP_RTP_ERR_FATAL;
  }

  if ( amr_config.channels_valid && ( amr_config.channels != 1 ) )
  {
    counter_amr_config->channels = 1 ;
    status = QVP_RTP_ERR_FATAL;
  }

  return ( status );

} /* end of function qvp_rtp_amr_profile_validate_fmtp */


/*===========================================================================

FUNCTION  QVP_RTP_AMR_PROFILE_RECV


DESCRIPTION
   The function which the RTP framework will call upon arrival of data from
   NW. amr header is stripped but parameters are passed.

DEPENDENCIES
  None

ARGUMENTS IN
  hdl           - hdl into the profile.
  usr_hdl       - handle to use when we call send function using the
                  registered rx callback
  pkt           - actual data packet to be parsed.


RETURN VALUE
  QVP_RTP_SUCCESS  - if we could send data. or an error code


SIDE EFFECTS
  None.

===========================================================================*/
#if 0
qvp_rtp_status_type qvp_rtp_amr_profile_recv
(

  qvp_rtp_profile_hdl_type     hdl,          /* handle created while open */
  qvp_rtp_profile_usr_hdl_type usr_hdl,      /* user handle for rx cb */
  qvp_rtp_buf_type             *pkt          /* packet parsed and removed*/

)
{
  qvp_rtp_amr_oal_hdr_param_type hdr;     /* parsed header storage */
  qvp_rtp_amr_ctx_type *stream = ( qvp_rtp_amr_ctx_type *) hdl;
  uint16                  toc;        /* to parse toc field */
  qvp_rtp_buf_type       *aud_buf;
  uint16                  offset = 0; /* for parsing tocs */
  uint16                 len;
  uint8                  *aud_data;
  uint16                 aud_len;
  uint16                 frame_cnt;
  uint16                 frame_index;
  uint8                  FT;
  uint8                  fqi = 0;
/*------------------------------------------------------------------------*/

  /*------------------------------------------------------------------------
                Are we sane here ..?
  ------------------------------------------------------------------------*/
  if( !amr_initialized || !stream || !stream->valid ||
      !amr_profile_config.rx_cb  )
  {
    return( QVP_RTP_ERR_FATAL );
  }

  if( !stream->stream_config.rx_octet_aligned ||
    ( pkt->len < QVP_RTP_AMR_OL_FIXED_HDR_SIZE ) )
  {

    /*----------------------------------------------------------------------
                free and exit
    ----------------------------------------------------------------------*/
    if( pkt->need_tofree )
    {
      //qvp_rtp_free_buf( pkt );
        free(pkt);
      pkt = NULL;
    }
    return( QVP_RTP_ERR_FATAL );

  }

  /*------------------------------------------------------------------------
    Rest of the code deals with the octet aligned and more flexible
    flavour of RFC3267
  ------------------------------------------------------------------------*/


  /*------------------------------------------------------------------------
              Parse the header
  ------------------------------------------------------------------------*/
  if( ( qvp_rtp_parse_amr_fixed_hdr_oa( pkt->data + pkt->head_room,
                              pkt->len, &hdr )  == 0 /* parse error */) ||
     (  ( hdr.ill != 0 /* we are getting interleaved frames */ )
        && !stream->stream_config.rx_interleave_on ) )
  {

    /*----------------------------------------------------------------------
                free and exit
    ----------------------------------------------------------------------*/
    if( pkt->need_tofree )
    {
      //qvp_rtp_free_buf( pkt );
        free(pkt);
      pkt = NULL;
    }
    return( QVP_RTP_ERR_FATAL );

  }

  //QVP_RTP_MSG_MED( " RX AMR with CMR = %d", hdr.cmr, 0, 0 );
  /*------------------------------------------------------------------------
    Do an initial walk through tocs for determining sanity and
    frame count

    Call the find toc in octet aligned mode
  ------------------------------------------------------------------------*/
  frame_cnt = qvp_rtp_amr_count_tocs_oa( pkt->data + pkt->head_room
                                     + QVP_RTP_AMR_OL_FIXED_HDR_SIZE,
                                  pkt->len - QVP_RTP_AMR_OL_FIXED_HDR_SIZE );
  /*------------------------------------------------------------------------
      If frame_cnt is zero sanity check failed
  ------------------------------------------------------------------------*/
  if( !frame_cnt )
  {

    /*----------------------------------------------------------------------
      could not walk throgh tocs   free and exit
    ----------------------------------------------------------------------*/
    if( pkt->need_tofree )
    {
      //qvp_rtp_free_buf( pkt );
        free(pkt);
      pkt = NULL;
    }
    return( QVP_RTP_ERR_FATAL );

  }



  /*------------------------------------------------------------------------
      If we have a smaller buffer free and exit
  ------------------------------------------------------------------------*/
  if( pkt->len < ( QVP_RTP_AMR_OL_FIXED_HDR_SIZE /* fixed header */ +
            frame_cnt ) )
  {
    /*----------------------------------------------------------------------
                free and exit
    ----------------------------------------------------------------------*/
    if( pkt->need_tofree )
    {
      //qvp_rtp_free_buf( pkt );
        free(pkt);
      pkt = NULL;
    }
    return( QVP_RTP_ERR_FATAL );
  }


  /*------------------------------------------------------------------------
      Advance to audio data inside the packet
  ------------------------------------------------------------------------*/
  aud_data = pkt->data + pkt->head_room + QVP_RTP_AMR_OL_FIXED_HDR_SIZE
                         /* fixed header length */ +
                        frame_cnt /* each toc one byte */ ;

  /*------------------------------------------------------------------------
      Preacalculate the audio data length inside the packet
  ------------------------------------------------------------------------*/
  aud_len = pkt->len - QVP_RTP_AMR_OL_FIXED_HDR_SIZE  /* fixed hdr */
            - frame_cnt /* dynamic header */ ;

  /*------------------------------------------------------------------------
      iterate through the packet until we have parsed all the frames...

      Or we get erraneous PDU.
  ------------------------------------------------------------------------*/
  frame_index = frame_cnt;
  while( frame_index )
  {

    /*----------------------------------------------------------------------
                    read the toc
    ----------------------------------------------------------------------*/
    toc = *( pkt->data + pkt->head_room + QVP_RTP_AMR_OL_FIXED_HDR_SIZE +
            offset);
    offset++;


    /*----------------------------------------------------------------------
                validate the toc first
       RFC3267

       Octet aligned TOC

       0 1 2 3 4 5 6 7
       +-+-+-+-+-+-+-+-+
       |F|  FT   |Q|P|P|
       +-+-+-+-+-+-+-+-+


    ----------------------------------------------------------------------*/


    /*----------------------------------------------------------------------
      Extract the FT value from TOC
    ----------------------------------------------------------------------*/
    FT = ( toc & 0x78 ) >> 3; /* bi maxk 0111 (7) 1000 (8) */

    /*----------------------------------------------------------------------
      Extract Frame quality Q bit
    ----------------------------------------------------------------------*/
    fqi = ( toc & 0x04 ) >> 2; /* bi maxk 0x0100 (8)  */

    /*----------------------------------------------------------------------
      From  RFC3267
      "
      If receiving a ToC entry with a FT value in the range 9-14 for AMR or
     10-13 for AMR-WB the whole packet SHOULD be discarded.
      "
    ----------------------------------------------------------------------*/
    if( ( ( FT > QVP_RTP_AMR_FT_RATE_SID ) &&
          ( FT < QVP_RTP_AMR_FT_RATE_NOD ) )
          || ( FT >  QVP_RTP_AMR_FT_RATE_NOD ) )
    {

      /*--------------------------------------------------------------------
                  Free the buffer if needed
      --------------------------------------------------------------------*/
      if( pkt->need_tofree )
      {
        //qvp_rtp_free_buf( pkt );
          free(pkt);
      pkt = NULL;
      }

      QVP_RTP_ERR( " got unknown toc  support this \r\n", 0, 0, 0 );
      return( QVP_RTP_ERR_FATAL );

    }

    if( FT == QVP_RTP_AMR_FT_RATE_NOD )
    {
      len = 0;
    }
    else
    {
      /*--------------------------------------------------------------------
                find the frame  len from frame type
      --------------------------------------------------------------------*/
      len = ( qvp_rtp__amr_ft_len_table[ FT ] + 7 ) / 8;

    }



    /*----------------------------------------------------------------------
       Double check for the len remaining
    ----------------------------------------------------------------------*/
    if( aud_len < len )
    {


      /*--------------------------------------------------------------------
        Malformed or corrupt packet
      --------------------------------------------------------------------*/
      QVP_RTP_ERR( " Insufficient data len inside the amr \r\n", \
                    0, 0, 0 );

      /*--------------------------------------------------------------------
                  Free the buffer if needed
      --------------------------------------------------------------------*/
      if( pkt->need_tofree )
      {
        //qvp_rtp_free_buf( pkt );
          free(pkt);
      pkt = NULL;
      }
      return( QVP_RTP_ERR_FATAL );
    }


    /*--------------------------------------------------------------------
              try and alloc an audio buffer
    --------------------------------------------------------------------*/
    //aud_buf = qvp_rtp_alloc_buf( QVP_RTP_POOL_AUDIO );
    aud_buf = (qvp_rtp_buf_type *)malloc(sizeof(uint8) * 512);


    /*--------------------------------------------------------------------
              See if we got a buffer
    --------------------------------------------------------------------*/
    if ( !aud_buf )
    {

      QVP_RTP_ERR( " Could not get an audio buffer \r\n", 0, 0, 0 );

      /*------------------------------------------------------------------
              Free if needed
      ------------------------------------------------------------------*/
      if ( pkt->need_tofree )
      {
        //qvp_rtp_free_buf( pkt );
          free(pkt);
          pkt = NULL;

      }
      return( QVP_RTP_ERR_FATAL );

    }
    else
    {
      aud_buf->head_room = 0;
      aud_buf->len = len;

      /*--------------------------------------------------------------------
        Taken from RFC3558

        "
        The RTP timestamp is in 1/8000 of a second units
        for amr and SMV.  For any other vocoders that use this packet
        format, the timestamp unit needs to be defined explicitly.

        "
        Taken from RFC3267

        "

        The RTP timestamp corresponds to the sampling instant of the first
        sample encoded for the first frame-block in the packet.

        "
        Remaining time stamps are calculated using packetization
        interval ( 20 ms fixed for amr ) in this implementation

      --------------------------------------------------------------------*/

      aud_buf->tstamp = pkt->tstamp + (
                       ( frame_cnt - frame_index ) *
                        stream->stream_config.rx_amr_pkt_interval );
      aud_buf->need_tofree = TRUE;
      aud_buf->seq = pkt->seq;
      /*--------------------------------------------------------------------
        Mark silence correctly
      --------------------------------------------------------------------*/
      if( FT == QVP_RTP_AMR_FT_RATE_NOD || FT == QVP_RTP_AMR_FT_RATE_SID )
      {
        aud_buf->silence = TRUE;
      }
      else
      {
        aud_buf->silence = FALSE;
      }
      /*--------------------------------------------------------------------
        Mark fqi bit
      --------------------------------------------------------------------*/
      if( fqi )
      {
        aud_buf->frm_info.info.aud_info.profile_info.amr_info.amr_fqi =
                                                              TRUE;
      }
      else
      {
        aud_buf->frm_info.info.aud_info.profile_info.amr_info.amr_fqi =
                                                              FALSE;
      }

      aud_buf->frm_info.info.aud_info.profile_info.amr_info.\
                                      amr_mode_request = hdr.cmr;

      /*--------------------------------------------------------------------
        Mark frm_present bit
      --------------------------------------------------------------------*/
      aud_buf->frm_info.info.aud_info.frm_present = TRUE;

      memcpy( aud_buf->data, aud_data, len );

      QVP_RTP_MSG_HIGH( " RX AMR Len = %d, timestamp = %d, \
                       marker = %d", aud_buf->len, aud_buf->tstamp,
                       aud_buf->marker_bit );

      /*------------------------------------------------------------------
                Ship the packet up

                If we could not ship it break;
      ------------------------------------------------------------------*/
      if( amr_profile_config.rx_cb(aud_buf, usr_hdl ) != QVP_RTP_SUCCESS )
      {
        break;
      }
    }


    /*----------------------------------------------------------------------
              Decrement the frame count next no
    ----------------------------------------------------------------------*/
    frame_index--;


    /*----------------------------------------------------------------------
            decrement the remaining audion len accordingly
    ----------------------------------------------------------------------*/
    aud_len -= len;

    /*----------------------------------------------------------------------
          If there is still life in this loop advance data pointer
    ----------------------------------------------------------------------*/
    if( frame_index )
    {

      /*--------------------------------------------------------------------
            Advance the audio buffer
      --------------------------------------------------------------------*/
      aud_data += len;

     }


  } /* end of while frame_index */


  /*----------------------------------------------------------------------
            Free the buffer if needed
  ----------------------------------------------------------------------*/
  if( pkt->need_tofree )
  {
    //qvp_rtp_free_buf( pkt );
      free(pkt);
      pkt = NULL;
  }

  /*------------------------------------------------------------------------
     If at this moment frame_index is not zero there is something wrong
  ------------------------------------------------------------------------*/
  if( frame_index )
  {
    return( QVP_RTP_ERR_FATAL );
  }
  else
  {
    return( QVP_RTP_SUCCESS );
  }

} /* end of function qvp_rtp_amr_profile_recv */
#endif

/*===========================================================================

FUNCTION  QVP_RTP_AMR_PROFILE_CLOSE


DESCRIPTION
  Closes an already open bi directional channel inside the profile.

DEPENDENCIES
  None

ARGUMENTS IN
  hdl - handle of channel to close.

RETURN VALUE
  QVP_RTP_SUCCESS  - if we could send data. or an error code


SIDE EFFECTS
  None.

===========================================================================*/
LOCAL qvp_rtp_status_type qvp_rtp_amr_profile_close
(

  qvp_rtp_profile_hdl_type     hdl           /* handle the profile */

)
{
  qvp_rtp_amr_ctx_type *stream = ( qvp_rtp_amr_ctx_type *) hdl;
/*------------------------------------------------------------------------*/

  /*------------------------------------------------------------------------
              If we get  a valid handle invalidate the index

              Also check are we initialized yet ..?
  ------------------------------------------------------------------------*/
  if( hdl && amr_initialized )
  {

    stream->valid = FALSE;

    /*----------------------------------------------------------------------
        Reset the stream context - if any stale values in there.
    ----------------------------------------------------------------------*/
    qvp_rtp_amr_reset_stream( stream );

    return( QVP_RTP_SUCCESS );
  }
  else
  {
    return( QVP_RTP_ERR_FATAL );
  }


} /* end of function qvp_rtp_amr_profile_close */

/*===========================================================================

FUNCTION  QVP_RTP_AMR_PROFILE_COPY_CONFIG


DESCRIPTION
  Closes an already open bi directional channel inside the profile.

DEPENDENCIES
  None

ARGUMENTS IN
  payld_config    - config struct which needs to be populated
  amr_config      - amr configuration which needs to be populated

RETURN VALUE
  QVP_RTP_SUCCESS  - if we could send data. or an error code


SIDE EFFECTS
  None.

===========================================================================*/
LOCAL qvp_rtp_status_type qvp_rtp_amr_profile_copy_config
(
  qvp_rtp_payload_config_type *payld_config,
  qvp_rtp_amr_config_type    *amr_config
)
{
 qvp_rtp_amr_sdp_config_type *amr_rx_config =
   &payld_config->config_rx_params.config_rx_payld_params.amr_rx_config;
 qvp_rtp_amr_sdp_config_type *amr_tx_config =
   &payld_config->config_tx_params.config_tx_payld_params.amr_tx_config;
/*------------------------------------------------------------------------*/


  /*------------------------------------------------------------------------
    Memset the whole configuration before we proceed
  ------------------------------------------------------------------------*/
  memset( amr_rx_config, 0,
          sizeof( qvp_rtp_amr_sdp_config_type ) );
  memset( amr_tx_config, 0,
          sizeof( qvp_rtp_amr_sdp_config_type ) );


  /*------------------------------------------------------------------------
    Setup RX path first
  ------------------------------------------------------------------------*/
  amr_rx_config->octet_align_valid = TRUE;
  amr_rx_config->octet_align = amr_config->rx_octet_aligned;

  amr_rx_config->mode_set_valid = FALSE;
  amr_rx_config->mode_change_neighbor_valid = FALSE;
  amr_rx_config->mode_change_period_valid = FALSE;

  amr_rx_config->crc_valid = TRUE;
  amr_rx_config->crc = amr_config->rx_crc_on;

  amr_rx_config->robust_sorting_valid = TRUE;
  amr_rx_config->robust_sorting = amr_config->rx_rob_sort_on;

  amr_rx_config->interleaving_valid = TRUE;
  amr_rx_config->interleaving = amr_config->rx_interleave_on;

  amr_rx_config->channels_valid = FALSE;

  amr_rx_config->valid = TRUE;


  /*------------------------------------------------------------------------
    Setup TX path now
  ------------------------------------------------------------------------*/
  amr_tx_config->octet_align_valid = TRUE;
  amr_tx_config->octet_align = amr_config->rx_octet_aligned;

  amr_tx_config->mode_set_valid = FALSE;
  amr_tx_config->mode_change_neighbor_valid = FALSE;
  amr_tx_config->mode_change_period_valid = FALSE;

  amr_tx_config->crc_valid = TRUE;
  amr_tx_config->crc = amr_config->tx_crc_on;

  amr_tx_config->robust_sorting_valid = TRUE;
  amr_tx_config->robust_sorting = amr_config->rx_rob_sort_on;

  amr_tx_config->interleaving_valid = TRUE;
  amr_tx_config->interleaving = amr_config->rx_interleave_on;

  amr_tx_config->channels_valid = FALSE;

  amr_tx_config->valid = TRUE;

  payld_config->config_rx_params.rx_rtp_param.maxptime_valid =
  payld_config->config_tx_params.tx_rtp_param.maxptime_valid = TRUE;

  payld_config->config_rx_params.rx_rtp_param.maxptime =
  payld_config->config_tx_params.tx_rtp_param.maxptime =
         QVP_RTP_AMR_DFLT_PTIME * QVP_RTP_DFLT_BUNDLE_SIZE;

  payld_config->config_rx_params.rx_rtp_param.ptime_valid =
  payld_config->config_tx_params.tx_rtp_param.ptime_valid = TRUE;

  payld_config->config_rx_params.rx_rtp_param.ptime =
  payld_config->config_tx_params.tx_rtp_param.ptime =
         QVP_RTP_AMR_DFLT_PTIME;

  payld_config->config_rx_params.valid = TRUE;
  payld_config->config_tx_params.valid = TRUE;

  payld_config->config_rx_params.config_rx_payld_params.amr_rx_config.valid
                                = TRUE;
  payld_config->config_tx_params.config_tx_payld_params.amr_tx_config.valid
                                 = TRUE;
  payld_config->payload  = QVP_RTP_PYLD_AMR;
  payld_config->valid = TRUE;

  return( QVP_RTP_SUCCESS );

} /* end of function qvp_rtp_amr_profile_copy_config */

/*===========================================================================

FUNCTION QVP_RTP_AMR_PROFILE_CONFIG_RX_PARAM

DESCRIPTION

  This function payload will configuration of Rx part of a previously
  opened stream.

DEPENDENCIES
  None

ARGUMENTS IN
    stream         - AMR stream which needs to be configured.
    payld_config   - pointer to configuration structure




RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesfull.
  else - Attempted a configuration  which is not currently supported by
         the implementation.


SIDE EFFECTS
  None.

===========================================================================*/
LOCAL qvp_rtp_status_type qvp_rtp_amr_profile_config_rx_param
(
  qvp_rtp_amr_ctx_type       *stream,
  qvp_rtp_payload_config_type *payld_config
)
{
  qvp_rtp_amr_sdp_config_type *amr_rx_config = NULL;
 /*------------------------------------------------------------------------*/
  if( !payld_config || !stream )
  {
    QVP_RTP_ERR ( " Invalid params  %d %d %d\n", 0, 0, 0);
    return( QVP_RTP_ERR_FATAL );
  }

  /*------------------------------------------------------------------------
    If ptime is other than 20 ms we are not setup for that.
  ------------------------------------------------------------------------*/
  if( payld_config->config_rx_params.rx_rtp_param.ptime_valid &&
      ( payld_config->config_rx_params.rx_rtp_param.ptime !=
                    QVP_RTP_AMR_DFLT_PTIME ) )
  {
    QVP_RTP_MSG_HIGH( "AMR Invalid Rx ptime %d %d %d\n",
        payld_config->config_rx_params.rx_rtp_param.ptime, 0, 0 );
    return( QVP_RTP_ERR_FATAL );
  }

  /*------------------------------------------------------------------------
    if max ptime is valid and maxptime is less than the packetization
    interval or greater thans maximum maxptime we support just  bail out.
  ------------------------------------------------------------------------*/
  if( ((payld_config->config_rx_params.rx_rtp_param.maxptime_valid) &&
      ( payld_config->config_rx_params.rx_rtp_param.maxptime <
                   QVP_RTP_AMR_DFLT_PTIME )) ||
      ( payld_config->config_rx_params.rx_rtp_param.maxptime >
                   QVP_RTP_AMR_DFLT_PTIME*QVP_RTP_MAX_BUNDLE_SIZE ) )
  {
    QVP_RTP_MSG_HIGH( "AMR Invalid Rx maxptime %d %d %d\n",
        payld_config->config_rx_params.rx_rtp_param.maxptime, 0, 0 );
    return( QVP_RTP_ERR_FATAL );
  }

  /*------------------------------------------------------------------------
    Store this value.
  ------------------------------------------------------------------------*/
  stream->stream_config.rx_max_ptime =
               payld_config->config_rx_params.rx_rtp_param.maxptime;

  QVP_RTP_MSG_HIGH( "AMR Rx maxptime %d Rx ptime %d %d\n",
        payld_config->config_rx_params.rx_rtp_param.maxptime,
          payld_config->config_rx_params.rx_rtp_param.ptime, 0 );

  amr_rx_config = &payld_config->config_rx_params.config_rx_payld_params.amr_rx_config;
  /*------------------------------------------------------------------------
    If there is no RX componet we should just bail out
    saying success
  ------------------------------------------------------------------------*/
  if( !amr_rx_config || !amr_rx_config->valid )
  {
    return( QVP_RTP_SUCCESS );
  }

  /*------------------------------------------------------------------------
    If it is multichannel out ... we dont support it
  ------------------------------------------------------------------------*/
  if( amr_rx_config->channels_valid && ( amr_rx_config->channels != 1 ) )
  {
    return( QVP_RTP_ERR_FATAL );
  }




  /*------------------------------------------------------------------------
    We dont do CRC now if someone asking for it we will just bail out
  ------------------------------------------------------------------------*/
  if( amr_rx_config->crc_valid &&
       amr_rx_config->crc )
  {
    return( QVP_RTP_ERR_FATAL );
  }


  /*------------------------------------------------------------------------
    We dont support interleaving we will just bail out
  ------------------------------------------------------------------------*/
  if( amr_rx_config->interleaving_valid &&
      amr_rx_config->interleaving )
  {
    return( QVP_RTP_ERR_FATAL );
  }

  /*------------------------------------------------------------------------
    If some one is asking BW efficient mode we will just say no
    for now
  ------------------------------------------------------------------------*/
  if( amr_rx_config->octet_align_valid &&
      !amr_rx_config->octet_align )
  {
    return( QVP_RTP_ERR_FATAL );
  }


  /*------------------------------------------------------------------------
    No Robust sorting
  ------------------------------------------------------------------------*/
  if( amr_rx_config->robust_sorting_valid &&
      ( amr_rx_config->robust_sorting ) )
  {

    return( QVP_RTP_ERR_FATAL );
  }

  /*------------------------------------------------------------------------
    If we got here we have a deal
  ------------------------------------------------------------------------*/
  return( QVP_RTP_SUCCESS );


} /* end of function qvp_rtp_amr_profile_config_rx_param */

/*===========================================================================

FUNCTION QVP_RTP_AMR_PROFILE_CONFIG_TX_PARAM

DESCRIPTION

  This function payload will configuration of Tx part of a previously
  opened stream.

DEPENDENCIES
  None

ARGUMENTS IN
    stream         - AMR stream which needs to be configured.
    payld_config   - pointer to configuration structure




RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesfull.
  else - Attempted a configuration  which is not currently supported by
         the implementation.


SIDE EFFECTS
  None.

===========================================================================*/
LOCAL qvp_rtp_status_type qvp_rtp_amr_profile_config_tx_param
(
  qvp_rtp_amr_ctx_type       *stream,
  qvp_rtp_payload_config_type *payld_config
)
{
  qvp_rtp_amr_sdp_config_type *amr_tx_config = NULL;
 /*------------------------------------------------------------------------*/
  if( !payld_config || !stream )
  {
    QVP_RTP_ERR ( " Invalid params  %d %d %d\n", 0, 0, 0);
    return( QVP_RTP_ERR_FATAL );
  }

  if( payld_config->config_tx_params.tx_rtp_param.ptime_valid )
  {
    /*----------------------------------------------------------------------
      This means peer can receive pkts with ptime lesser
      than our minimum ptime pkts we can send. Throw an error.
    ----------------------------------------------------------------------*/
    if ( payld_config->config_tx_params.tx_rtp_param.ptime <
       QVP_RTP_AMR_DFLT_PTIME )
    {
      QVP_RTP_MSG_HIGH( "AMR Invalid Tx ptime %d %d %d\n",
        payld_config->config_tx_params.tx_rtp_param.ptime, 0, 0 );
      return( QVP_RTP_ERR_FATAL );
    }
    /*----------------------------------------------------------------------
      This means peer can receive pkts with ptime greater
      than our maximum ptime. Just set the transmit ptime to maximum
      ptime we can send.
    ----------------------------------------------------------------------*/
    else if ( payld_config->config_tx_params.tx_rtp_param.ptime >
       QVP_RTP_AMR_DFLT_PTIME )
    {
      stream->stream_config.tx_ptime = QVP_RTP_AMR_DFLT_PTIME;
    }
    /*----------------------------------------------------------------------
      Proposed ptime is within the limits, we can send packets with the
      ptime proposed.
    ----------------------------------------------------------------------*/
    else
    {
      stream->stream_config.tx_ptime =
           payld_config->config_tx_params.tx_rtp_param.ptime;
    }
  }

  /*------------------------------------------------------------------------
    if we have max ptime make sure we do agree to it and set it
  ------------------------------------------------------------------------*/
  if( payld_config->config_tx_params.tx_rtp_param.maxptime_valid )
  {
    /*----------------------------------------------------------------------
      If maxptime is less than ptime just return error
    ----------------------------------------------------------------------*/
    if( payld_config->config_tx_params.tx_rtp_param.maxptime <
                QVP_RTP_AMR_DFLT_PTIME )
    {
      QVP_RTP_MSG_HIGH( "AMR Invalid Tx maxptime %d %d %d\n",
        payld_config->config_tx_params.tx_rtp_param.maxptime, 0, 0 );
      return( QVP_RTP_ERR_FATAL );
    }
    /*----------------------------------------------------------------------
      This means peer can receive pkts with maxptime greater
      than our maximum maxptime. Just set the transmit maxptime to
      maximum maxptime we can send.
    ----------------------------------------------------------------------*/
    else if ( payld_config->config_tx_params.tx_rtp_param.maxptime >
                 QVP_RTP_AMR_DFLT_PTIME*QVP_RTP_MAX_BUNDLE_SIZE )
    {
      stream->stream_config.tx_max_ptime =
          QVP_RTP_AMR_DFLT_PTIME*QVP_RTP_DFLT_BUNDLE_SIZE;
    }
    /*----------------------------------------------------------------------
      Proposed maxptime is within the limits, we can send packets with the
      maxptime proposed.
    ----------------------------------------------------------------------*/
    else
    {
      stream->stream_config.tx_max_ptime =
                     payld_config->config_tx_params.tx_rtp_param.maxptime;
    }

  }


  stream->stream_config.tx_bundle_size =
     stream->stream_config.tx_max_ptime/stream->stream_config.tx_ptime;

  QVP_RTP_MSG_HIGH( "AMR Tx maxptime %d Tx ptime %d bundle size %d",
        stream->stream_config.tx_max_ptime,
    stream->stream_config.tx_ptime, stream->stream_config.tx_bundle_size );


  amr_tx_config =
    &payld_config->config_tx_params.config_tx_payld_params.amr_tx_config;

  /*------------------------------------------------------------------------
    If there is no TX componet we should just bail out
    saying success
  ------------------------------------------------------------------------*/

  if( !amr_tx_config || !amr_tx_config->valid )
  {
    return( QVP_RTP_SUCCESS );
  }

  /*------------------------------------------------------------------------
    If it is multichannel out ... we dont support it
  ------------------------------------------------------------------------*/
  if( amr_tx_config->channels_valid &&
      ( amr_tx_config->channels != 1 ) )
  {
    return( QVP_RTP_ERR_FATAL );
  }


  /*------------------------------------------------------------------------
    We dont do CRC now if someone asking for it we will just bail out
  ------------------------------------------------------------------------*/
  if( amr_tx_config->crc_valid &&
       amr_tx_config->crc )
  {
    return( QVP_RTP_ERR_FATAL );
  }


  /*------------------------------------------------------------------------
    If some one is asking BW efficient mode we will just say no
    for now
  ------------------------------------------------------------------------*/
  if( amr_tx_config->octet_align_valid &&
      !amr_tx_config->octet_align )
  {
    return( QVP_RTP_ERR_FATAL );
  }


  /*------------------------------------------------------------------------
    No Robust sorting
  ------------------------------------------------------------------------*/
  if( amr_tx_config->robust_sorting_valid &&
      ( amr_tx_config->robust_sorting ) )
  {

    return( QVP_RTP_ERR_FATAL );
  }


  /*------------------------------------------------------------------------
    If we got here we have a deal
  ------------------------------------------------------------------------*/
  return( QVP_RTP_SUCCESS );

} /* end of function qvp_rtp_amr_profile_config_tx_param */
/*===========================================================================

FUNCTION  QVP_RTP_AMR_RESET_TX_CTX


DESCRIPTION
  Reset the trasmitter context inside the stream

DEPENDENCIES
  None

ARGUMENTS IN
  stream to be reset

RETURN VALUE
  None


SIDE EFFECTS
  None.

===========================================================================*/
LOCAL void qvp_rtp_amr_reset_tx_ctx
(
  qvp_rtp_amr_ctx_type *stream
)
{
/*------------------------------------------------------------------------*/

  /*------------------------------------------------------------------------
      Reset the packet to packet to packet context
  ------------------------------------------------------------------------*/
  stream->tx_ctx.data_len = 0;
  stream->tx_ctx.frame_cnt = 0;
  stream->tx_ctx.marker = 0;
  stream->tx_ctx.all_silence_frames = TRUE;
} /* end of function qvp_rtp_amr_reset_tx_ctx */

/*===========================================================================

FUNCTION  QVP_RTP_AMR_RESET_STREAM


DESCRIPTION
  Reset the context inside the stream

DEPENDENCIES
  None

ARGUMENTS IN
  stream to be reset

RETURN VALUE
  QVP_RTP_SUCCESS  - if we could send data. or an error code


SIDE EFFECTS
  None.

===========================================================================*/
LOCAL void qvp_rtp_amr_reset_stream
(
  qvp_rtp_amr_ctx_type *stream
)
{

/*------------------------------------------------------------------------*/

  /*------------------------------------------------------------------------
          Reset the transmit side
  ------------------------------------------------------------------------*/
  qvp_rtp_amr_reset_tx_ctx( stream );


  /*------------------------------------------------------------------------
    prev_silence_frame is reset only when the whole stream is reset
    SO IT DOES NOT STRICTLY BELONG TO PREVIOUS FUNCTION.
  ------------------------------------------------------------------------*/
  stream->tx_ctx.prev_silence_frame = FALSE;


  /*------------------------------------------------------------------------
      add code to reset rx_ctx when needed if and when we do
      interleaving
  ------------------------------------------------------------------------*/

} /* end of function qvp_rtp_amr_reset_stream */

/*===========================================================================

FUNCTION  QVP_RTP_PARSE_AMR_FIXED_HDR _OA


DESCRIPTION
  Parse the fixed part of amr header inside trasmit buffer. This is
  typically done for each bundled ( pkt received ).

DEPENDENCIES
  None

ARGUMENTS IN
  data - data to  be parsed
  len  - len of data contained

ARGUMENTS IN
  hdr - container for parsed values.

RETURN VALUE
  length of bytes chewed up. Zero on parse errors.


SIDE EFFECTS
  None.

===========================================================================*/

uint16 qvp_rtp_parse_amr_fixed_hdr_oa
(

  uint8 *data,
  uint16 len,
  qvp_rtp_amr_oal_hdr_param_type *hdr

)
{
  uint16 parse_offset = 0;
/*------------------------------------------------------------------------*/

  /*------------------------------------------------------------------------
      If length is than the fixed header return 0
  ------------------------------------------------------------------------*/
  if( len < QVP_RTP_AMR_OL_FIXED_HDR_SIZE )
  {
    return( 0 );
  }

  /*------------------------------------------------------------------------

      0                   1
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
   +-+-+-+-+-+-+-+-+- - - - - - - -
   |  CMR  |R|R|R|R|  ILL  |  ILP  |
   +-+-+-+-+-+-+-+-+- - - - - - - -

    From RFC3267
  ------------------------------------------------------------------------*/

  /*------------------------------------------------------------------------
    Parse the CMR
  ------------------------------------------------------------------------*/
  hdr->cmr = b_unpackb( data, parse_offset, 4 );
  /*------------------------------------------------------------------------
    Advance the CMR len Skip over the reserved bits
  ------------------------------------------------------------------------*/
  parse_offset += 4 /* CMR */ + 4 /* R bits */;


  /*------------------------------------------------------------------------
    Since we do not support interleaving these values should be flipped
    FALSE
  ------------------------------------------------------------------------*/
  hdr->ill = 0;
  hdr->ilp = 0;

  /*------------------------------------------------------------------------
        return the fixed header len
  ------------------------------------------------------------------------*/
  return( QVP_RTP_AMR_OL_FIXED_HDR_SIZE );

} /* end of  function qvp_rtp_parse_amr_fixed_hdr */

/*===========================================================================

FUNCTION  QVP_RTP_AMR_COUNT_TOCS_OA


DESCRIPTION
  Walks through a list of AMR tocs and calculates the number of tocs
  contained in the packet. This will do initial sanity check of the packet
  to some extend.

DEPENDENCIES
  None

ARGUMENTS IN
  data - data to  be parsed
  len  - len of data contained

ARGUMENTS OUT
  None

RETURN VALUE
  length of tocs contained in the stream. Zero on error.


SIDE EFFECTS
  None.

===========================================================================*/
uint16 qvp_rtp_amr_count_tocs_oa
(
  uint8 *data,
  uint16 len
)
{
  uint16 fr_cnt = 0;
  boolean more_to_come =  TRUE; /* init with true we should
                                   * have one remaining
                                   */
/*------------------------------------------------------------------------*/

  /*------------------------------------------------------------------------
    If there is zero bytes there is something wrong
  ------------------------------------------------------------------------*/
  if( !len )
  {
    return( 0 );
  }

  /*------------------------------------------------------------------------
    iterate through the packet either till there is no data or
    there is no more to come
  ------------------------------------------------------------------------*/
  while( len && more_to_come )
  {


    /*----------------------------------------------------------------------

      Taken from RFC 3267

      +---------------------+
      | list of ToC entries |
      +---------------------+
      | list of frame CRCs  | (optional)
      - - - - - - - - - - -

      Taken from RFC 3267
     A ToC entry takes the following format in octet-aligned mode:

      0 1 2 3 4 5 6 7
     +-+-+-+-+-+-+-+-+
     |F|  FT   |Q|P|P|
     +-+-+-+-+-+-+-+-+

    ----------------------------------------------------------------------*/
    more_to_come = ( (*data) & 0x80 ); /* extract the F bit for more_to_come
                                      * flag
                                      */
    /*----------------------------------------------------------------------
      point Data to next TOC
    ----------------------------------------------------------------------*/
    data++;

    /*----------------------------------------------------------------------
      Decrement len as we walk this TOC ( a byte len );
    ----------------------------------------------------------------------*/
    len--;


    /*----------------------------------------------------------------------
      Increment fr_cnt by one as we walk this TOC
    ----------------------------------------------------------------------*/
    fr_cnt++;

  } /* end of while len && more_to_come */

  /*------------------------------------------------------------------------
    If there is not enough data than is determined by the TOC walk
    through then we have a problem
  ------------------------------------------------------------------------*/
  if( !len && more_to_come )
  {
    return( 0 );
  }
  else
  {
    return( fr_cnt );
  }

} /* end of function qvp_rtp_amr_count_tocs_oa */

/*===========================================================================

FUNCTION  QVP_RTP_AMR_FORM_HEADER


DESCRIPTION
  Preloads the trasmit buffer with the fixed amr paylod header for AMR
  octet aligned mode

DEPENDENCIES
  None

ARGUMENTS IN
  stream to be stuffed with fixed header

RETURN VALUE
   length of bytes loaded


SIDE EFFECTS
  None.

===========================================================================*/
LOCAL uint8 qvp_rtp_amr_form_header_oa
(
  qvp_rtp_amr_ctx_type *stream
)
{
  uint16 offset = 0;
/*------------------------------------------------------------------------*/

  /*------------------------------------------------------------------------
    Reset the maximum len of a amr pyld header
  ------------------------------------------------------------------------*/
  memset( stream->tx_ctx.op_packet, 0,
          QVP_RTP_AMR_OL_FIXED_HDR_SIZE );

  /*------------------------------------------------------------------------



      0                   1
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
   +-+-+-+-+-+-+-+-+- - - - - - - -
   |  CMR  |R|R|R|R|  ILL  |  ILP  |
   +-+-+-+-+-+-+-+-+- - - - - - - -

  From RFC3267

  ------------------------------------------------------------------------*/

  /*------------------------------------------------------------------------
    We will not request CMR for now
  ------------------------------------------------------------------------*/
  b_packb(  QVP_RTP_AMR_NOCHG_CMR, stream->tx_ctx.op_packet, offset, 4 );
  offset += 4;


  /*------------------------------------------------------------------------
      Advance offset for RRRR bits
  ------------------------------------------------------------------------*/
  offset += 4;

  /*------------------------------------------------------------------------
      If we do not have interleave this header is static
  ------------------------------------------------------------------------*/
  if( stream->stream_config.tx_interleave_on )
  {
    /*----------------------------------------------------------------------
      Add interleave len
      ill
    ----------------------------------------------------------------------*/
    b_packb( 0, stream->tx_ctx.op_packet, offset, 4 );
    offset += 4;

    /*----------------------------------------------------------------------
      Add interleave position
     ilp
    ----------------------------------------------------------------------*/
    b_packb( 0, stream->tx_ctx.op_packet, offset, 4 );
    offset += 4;

    /*----------------------------------------------------------------------
        If interleave is asked cry and bail out
    ----------------------------------------------------------------------*/
    /*QVP_RTP_MSG_MED( " interleaving asked but not supported\r\n", \
                     0, 0, 0 );*/
    return( QVP_RTP_AMR_OL_FIXED_HDR_SIZE + 1 );
  }

  return( QVP_RTP_AMR_OL_FIXED_HDR_SIZE /* total header size */ );


} /* end of function qvp_rtp_amr_form_header_oa */

/*===========================================================================

FUNCTION  QVP_RTP_AMR_FIND_FT


DESCRIPTION
   Finds the matching Frame Type for a particular frame length.

DEPENDENCIES
  None

ARGUMENTS IN
  len - length of the parload

RETURN VALUE
  toc type


SIDE EFFECTS
  None.

===========================================================================*/
#if 0
LOCAL qvp_rtp_amr_toc_type qvp_rtp_amr_find_FT
(
  uint16 len
)
{
  int8 i; /* index variable */
/*------------------------------------------------------------------------*/

  /*------------------------------------------------------------------------
      Lookup the len table for a matching TOC
  ------------------------------------------------------------------------*/
  for( i = 0; i <= QVP_RTP_AMR_FT_RATE_SID ; i ++ )
  {
    /* return the index of the table when we get a match */
    if( ( ( qvp_rtp__amr_ft_len_table[i] + 7 /* for rounding */ ) /
          8 /* devide by 8 to get the no of bytes */ ) == len  )
    {
      return( ( qvp_rtp_amr_toc_type ) i );
    }

  } /* end of for i = 0 */

  return( QVP_RTP_AMR_FT_INVALID );

} /* end of function qvp_rtp_amr_find_FT */
#endif
/*===========================================================================

FUNCTION  QVP_RTP_AMR_PROFILE_SHUTDOWN


DESCRIPTION
  Shuts this module down and flag intialization as false

DEPENDENCIES
  None

ARGUMENTS IN
  len - length of the parload

RETURN VALUE
  toc type


SIDE EFFECTS
  None.

===========================================================================*/
LOCAL void qvp_rtp_amr_profile_shutdown( void )
{
  uint32 i;
/*------------------------------------------------------------------------*/


  /*------------------------------------------------------------------------
    If aleready shutdown bail out
  ------------------------------------------------------------------------*/
  if( !amr_initialized )
  {
    return;
  }

  /*------------------------------------------------------------------------
          Walk through the stream array and close all channels
  ------------------------------------------------------------------------*/
  for( i = 0; i < amr_profile_config.num_streams; i ++ )
  {


    /*----------------------------------------------------------------------
            If this stream is open close it now
    ----------------------------------------------------------------------*/
    if ( qvp_rtp_amr_array[ i ].valid )
    {

      qvp_rtp_amr_profile_close(  ( qvp_rtp_profile_hdl_type ) i );

    }

  } /* end of for i = 0  */

  /*------------------------------------------------------------------------
              free the array of streams
  ------------------------------------------------------------------------*/
  //qvp_rtp_free( qvp_rtp_amr_array   );
  free(qvp_rtp_amr_array);
  qvp_rtp_amr_array = NULL;

  /*------------------------------------------------------------------------
            Flag array as NULL and init as FALSE
  ------------------------------------------------------------------------*/
  qvp_rtp_amr_array = NULL;
  amr_initialized  = FALSE;

} /* end of function qvp_rtp_amr_profile_shutdown  */



/************************************************************************************
API to parse the profile header and seperate the audio data
************************************************************************************/

#if 0
qvp_rtp_status_type qvp_rtp_amr_profile
(

  //qvp_rtp_profile_hdl_type     hdl,          /* handle created while open */
  //qvp_rtp_profile_usr_hdl_type usr_hdl,      /* user handle for rx cb */
  qvp_rtp_buf_type             *pkt          /* packet parsed and removed*/

)
{
  qvp_rtp_amr_oal_hdr_param_type hdr;     /* parsed header storage */
  //qvp_rtp_amr_ctx_type *stream = ( qvp_rtp_amr_ctx_type *) hdl;
  uint16                  toc;        /* to parse toc field */
  qvp_rtp_buf_type       *aud_buf;
  uint16                  offset = 0; /* for parsing tocs */
  uint16                 len;
  uint8                  *aud_data;
  uint16                 aud_len;
  uint16                 frame_cnt;
  uint16                 frame_index;
  uint8                  FT;
  uint8                  fqi = 0;
/*------------------------------------------------------------------------*/

  /*------------------------------------------------------------------------
                Are we sane here ..?
  ------------------------------------------------------------------------*/
  /*if( !amr_initialized || !stream || !stream->valid ||
      !amr_profile_config.rx_cb  )
  {
    return( QVP_RTP_ERR_FATAL );
  }*/

  /*if( !stream->stream_config.rx_octet_aligned ||
    ( pkt->len < QVP_RTP_AMR_OL_FIXED_HDR_SIZE ) )
  {

    //----------------------------------------------------------------------
               // free and exit
    //----------------------------------------------------------------------
    if( pkt->need_tofree )
    {
      //qvp_rtp_free_buf( pkt );
        free(pkt);
      pkt = NULL;
    }
    return( QVP_RTP_ERR_FATAL );

  }*/

  /*------------------------------------------------------------------------
    Rest of the code deals with the octet aligned and more flexible
    flavour of RFC3267
  ------------------------------------------------------------------------*/


  /*------------------------------------------------------------------------
              Parse the header
  ------------------------------------------------------------------------*/
  if( ( qvp_rtp_parse_amr_fixed_hdr_oa( pkt->data + pkt->head_room,
                              pkt->len, &hdr )  == 0 /* parse error */) ||
     (  ( hdr.ill != 0 /* we are getting interleaved frames */ )
        /*&& !stream->stream_config.rx_interleave_on */) )
  {

    /*----------------------------------------------------------------------
                free and exit
    ----------------------------------------------------------------------*/
    if( pkt->need_tofree )
    {
      //qvp_rtp_free_buf( pkt );
        free(pkt);
      pkt = NULL;
    }
    return( QVP_RTP_ERR_FATAL );

  }

  //QVP_RTP_MSG_MED( " RX AMR with CMR = %d", hdr.cmr, 0, 0 );
  /*------------------------------------------------------------------------
    Do an initial walk through tocs for determining sanity and
    frame count

    Call the find toc in octet aligned mode
  ------------------------------------------------------------------------*/
  frame_cnt = qvp_rtp_amr_count_tocs_oa( pkt->data + pkt->head_room
                                     + QVP_RTP_AMR_OL_FIXED_HDR_SIZE,
                                  pkt->len - QVP_RTP_AMR_OL_FIXED_HDR_SIZE );
  /*------------------------------------------------------------------------
      If frame_cnt is zero sanity check failed
  ------------------------------------------------------------------------*/
  if( !frame_cnt )
  {

    /*----------------------------------------------------------------------
      could not walk throgh tocs   free and exit
    ----------------------------------------------------------------------*/
    if( pkt->need_tofree )
    {
      //qvp_rtp_free_buf( pkt );
        free(pkt);
      pkt = NULL;
    }
    return( QVP_RTP_ERR_FATAL );

  }



  /*------------------------------------------------------------------------
      If we have a smaller buffer free and exit
  ------------------------------------------------------------------------*/
  if( pkt->len < ( QVP_RTP_AMR_OL_FIXED_HDR_SIZE /* fixed header */ +
            frame_cnt ) )
  {
    /*----------------------------------------------------------------------
                free and exit
    ----------------------------------------------------------------------*/
    if( pkt->need_tofree )
    {
      //qvp_rtp_free_buf( pkt );
        free(pkt);
      pkt = NULL;
    }
    return( QVP_RTP_ERR_FATAL );
  }


  /*------------------------------------------------------------------------
      Advance to audio data inside the packet
  ------------------------------------------------------------------------*/
  aud_data = pkt->data + pkt->head_room + QVP_RTP_AMR_OL_FIXED_HDR_SIZE
                         /* fixed header length */ +
                        frame_cnt /* each toc one byte */ ;

  /*------------------------------------------------------------------------
      Preacalculate the audio data length inside the packet
  ------------------------------------------------------------------------*/
  aud_len = pkt->len - QVP_RTP_AMR_OL_FIXED_HDR_SIZE  /* fixed hdr */
            - frame_cnt /* dynamic header */ ;

  /*------------------------------------------------------------------------
      iterate through the packet until we have parsed all the frames...

      Or we get erraneous PDU.
  ------------------------------------------------------------------------*/
  frame_index = frame_cnt;
  while( frame_index )
  {

    /*----------------------------------------------------------------------
                    read the toc
    ----------------------------------------------------------------------*/
    toc = *( pkt->data + pkt->head_room + QVP_RTP_AMR_OL_FIXED_HDR_SIZE +
            offset);
    offset++;


    /*----------------------------------------------------------------------
                validate the toc first
       RFC3267

       Octet aligned TOC

       0 1 2 3 4 5 6 7
       +-+-+-+-+-+-+-+-+
       |F|  FT   |Q|P|P|
       +-+-+-+-+-+-+-+-+


    ----------------------------------------------------------------------*/


    /*----------------------------------------------------------------------
      Extract the FT value from TOC
    ----------------------------------------------------------------------*/
    FT = ( toc & 0x78 ) >> 3; /* bi maxk 0111 (7) 1000 (8) */

    /*----------------------------------------------------------------------
      Extract Frame quality Q bit
    ----------------------------------------------------------------------*/
    fqi = ( toc & 0x04 ) >> 2; /* bi maxk 0x0100 (8)  */

    /*----------------------------------------------------------------------
      From  RFC3267
      "
      If receiving a ToC entry with a FT value in the range 9-14 for AMR or
     10-13 for AMR-WB the whole packet SHOULD be discarded.
      "
    ----------------------------------------------------------------------*/
    if( ( ( FT > QVP_RTP_AMR_FT_RATE_SID ) &&
          ( FT < QVP_RTP_AMR_FT_RATE_NOD ) )
          || ( FT >  QVP_RTP_AMR_FT_RATE_NOD ) )
    {

      /*--------------------------------------------------------------------
                  Free the buffer if needed
      --------------------------------------------------------------------*/
      if( pkt->need_tofree )
      {
        //qvp_rtp_free_buf( pkt );
          free(pkt);
      pkt = NULL;
      }

      QVP_RTP_ERR( " got unknown toc  support this \r\n", 0, 0, 0 );
      return( QVP_RTP_ERR_FATAL );

    }

    if( FT == QVP_RTP_AMR_FT_RATE_NOD )
    {
      len = 0;
    }
    else
    {
      /*--------------------------------------------------------------------
                find the frame  len from frame type
      --------------------------------------------------------------------*/
      len = ( qvp_rtp__amr_ft_len_table[ FT ] + 7 ) / 8;

    }



    /*----------------------------------------------------------------------
       Double check for the len remaining
    ----------------------------------------------------------------------*/
    if( aud_len < len )
    {


      /*--------------------------------------------------------------------
        Malformed or corrupt packet
      --------------------------------------------------------------------*/
      QVP_RTP_ERR( " Insufficient data len inside the amr \r\n", \
                    0, 0, 0 );

      /*--------------------------------------------------------------------
                  Free the buffer if needed
      --------------------------------------------------------------------*/
      if( pkt->need_tofree )
      {
        //qvp_rtp_free_buf( pkt );
          free(pkt);
      pkt = NULL;
      }
      return( QVP_RTP_ERR_FATAL );
    }


    /*--------------------------------------------------------------------
              try and alloc an audio buffer
    --------------------------------------------------------------------*/
    //aud_buf = qvp_rtp_alloc_buf( QVP_RTP_POOL_AUDIO );
    aud_buf = (qvp_rtp_buf_type *)malloc(sizeof(uint8) * 512);


    /*--------------------------------------------------------------------
              See if we got a buffer
    --------------------------------------------------------------------*/
    if ( !aud_buf )
    {

      QVP_RTP_ERR( " Could not get an audio buffer \r\n", 0, 0, 0 );

      /*------------------------------------------------------------------
              Free if needed
      ------------------------------------------------------------------*/
      if ( pkt->need_tofree )
      {
        //qvp_rtp_free_buf( pkt );
          free(pkt);
          pkt = NULL;

      }
      return( QVP_RTP_ERR_FATAL );

    }
    else
    {
      aud_buf->head_room = 0;
      aud_buf->len = len;

      /*--------------------------------------------------------------------
        Taken from RFC3558

        "
        The RTP timestamp is in 1/8000 of a second units
        for amr and SMV.  For any other vocoders that use this packet
        format, the timestamp unit needs to be defined explicitly.

        "
        Taken from RFC3267

        "

        The RTP timestamp corresponds to the sampling instant of the first
        sample encoded for the first frame-block in the packet.

        "
        Remaining time stamps are calculated using packetization
        interval ( 20 ms fixed for amr ) in this implementation

      --------------------------------------------------------------------*/

      //aud_buf->tstamp = pkt->tstamp + (
                       //( frame_cnt - frame_index ) *
                        //stream->stream_config.rx_amr_pkt_interval );
      aud_buf->need_tofree = TRUE;
      aud_buf->seq = pkt->seq;
      /*--------------------------------------------------------------------
        Mark silence correctly
      --------------------------------------------------------------------*/
      if( FT == QVP_RTP_AMR_FT_RATE_NOD || FT == QVP_RTP_AMR_FT_RATE_SID )
      {
        aud_buf->silence = TRUE;
      }
      else
      {
        aud_buf->silence = FALSE;
      }
      /*--------------------------------------------------------------------
        Mark fqi bit
      --------------------------------------------------------------------*/
      if( fqi )
      {
        aud_buf->frm_info.info.aud_info.profile_info.amr_info.amr_fqi =
                                                              TRUE;
      }
      else
      {
        aud_buf->frm_info.info.aud_info.profile_info.amr_info.amr_fqi =
                                                              FALSE;
      }

      aud_buf->frm_info.info.aud_info.profile_info.amr_info.\
                                      amr_mode_request = hdr.cmr;

      /*--------------------------------------------------------------------
        Mark frm_present bit
      --------------------------------------------------------------------*/
      aud_buf->frm_info.info.aud_info.frm_present = TRUE;

      memcpy( aud_buf->data, aud_data, len );

      QVP_RTP_MSG_HIGH( " RX AMR Len = %d, timestamp = %d, \
                       marker = %d", aud_buf->len, aud_buf->tstamp,
                       aud_buf->marker_bit );

      /*------------------------------------------------------------------
                Ship the packet up

                If we could not ship it break;
      ------------------------------------------------------------------*/
      /*if( amr_profile_config.rx_cb(aud_buf, usr_hdl ) != QVP_RTP_SUCCESS )
      {
        break;
      }*/
    }


    /*----------------------------------------------------------------------
              Decrement the frame count next no
    ----------------------------------------------------------------------*/
    frame_index--;


    /*----------------------------------------------------------------------
            decrement the remaining audion len accordingly
    ----------------------------------------------------------------------*/
    aud_len -= len;

    /*----------------------------------------------------------------------
          If there is still life in this loop advance data pointer
    ----------------------------------------------------------------------*/
    if( frame_index )
    {

      /*--------------------------------------------------------------------
            Advance the audio buffer
      --------------------------------------------------------------------*/
      aud_data += len;

     }


  } /* end of while frame_index */


  /*----------------------------------------------------------------------
            Free the buffer if needed
  ----------------------------------------------------------------------*/
  if( pkt->need_tofree )
  {
    //qvp_rtp_free_buf( pkt );
      free(pkt);
      pkt = NULL;
  }

  /*------------------------------------------------------------------------
     If at this moment frame_index is not zero there is something wrong
  ------------------------------------------------------------------------*/
  if( frame_index )
  {
    return( QVP_RTP_ERR_FATAL );
  }
  else
  {
    return( QVP_RTP_SUCCESS );
  }

} /* end of function qvp_rtp_amr_profile_recv */
#endif

#endif /* end of FEATURE_QVPHONE_RTP */
