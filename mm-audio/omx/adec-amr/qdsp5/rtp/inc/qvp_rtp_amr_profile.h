#ifndef _H_QVP_RTP_AMR_H_
#define _H_QVP_RTP_AMR_H_
/*=*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                         QVP_RTP_AMR_PROFILE . H

GENERAL DESCRIPTION

  This file contains the implementation of AMR profile. AMR profile acts
  as conduite to RTP layer. The RFC which is based on is RFC3267.

EXTERNALIZED FUNCTIONS
  None.


INITIALIZATION AND SEQUENCING REQUIREMENTS
  Need to init and configure the profile before it becomes usable.



  Copyright (c) 2004 by Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary.  Export of this technology or software is
  regulated by the U.S. Government. Diversion contrary to U.S. law prohibited.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*=*/

/*===========================================================================

$Header: //source/qcom/qct/multimedia/videophone/ps_media/RTP/RTP_profile/rel/1.0/src/qvp_rtp_amr_profile.h#2 $ $DateTime: 2008/10/17 03:10:28 $ $Author: apurupa $

                            EDIT HISTORY FOR FILE


when        who    what, where, why
--------    ---    ----------------------------------------------------------
17/10/08    apr    Adding support for max bundle size of 5
12/31/07    grk    Moving maxptime/ptime to media level.
05/05/06    apr    Including RTP code in feature FEATURE_QVPHONE_RTP
10/04/05    apr    Modifications to support silence suppression
                   addition of all_silence_frames variable
                   to the structure qvp_rtp_amr_tx_ctx_type
09/26/05    apr    Modifications for marker bit usage
                   addition of prev_silence_frame
                   varibale to the structure
                   qvp_rtp_amr_tx_ctx_type
07/18/05    srk    Fixed header issue when interleaving
                   is signalled absent
03/20/05    srk    Added more configuration changes
02/10/05    srk    Initial Creation.

===========================================================================*/
#include "rtp_unpack.h"
#ifdef FEATURE_QVPHONE_RTP
#include "qvp_rtp_packet.h"

#define QVP_RTP_AMR_PKT_INTERVAL 160  /* 1/8000 of a second. so 20 ms =
                                       * 20 / ( 1/8 ) = 160 units
                                       */
#define QVP_RTP_DFLT_BUNDLE_SIZE       3 /* Default bundle size */
#define QVP_RTP_MAX_BUNDLE_SIZE       5  /* Max bundle size */
#define QVP_RTP_AMR_DFLT_PTIME       20
#define QVP_RTP_AMR_NOCHG_CMR        15 /* if we don't request CMR
                                           * we will need to stuff this
                                           * in the packet
                                           */
#define QVP_RTP_AMR_OL_FIXED_HDR_SIZE  1 /* fixed header size in Octet
                                          * aligned mode AMR packing
                                          */
#define QVP_RTP_AMR_OL_Q_BIT           0x4 /* Q bit mask in Octet aligned
                                            * mode
                                            */

/*--------------------------------------------------------------------------

  TYPEDEF ENUM QVP_RTP_AMR_TOC_TYPE

  This is TOC type for each frame stuffed.

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
typedef enum
{

  QVP_RTP_AMR_FT_RATE_4_75  = 0, /* 4.75 kbps */
  QVP_RTP_AMR_FT_RATE_5_15  = 1, /* 5.15 kbps */
  QVP_RTP_AMR_FT_RATE_5_9   = 2, /* 5.9 kbps  */
  QVP_RTP_AMR_FT_RATE_6_7   = 3, /* 6.7 kbps  */
  QVP_RTP_AMR_FT_RATE_7_4   = 4, /* 7.4 kbps  */
  QVP_RTP_AMR_FT_RATE_7_95  = 5, /* 7.95 kbps */
  QVP_RTP_AMR_FT_RATE_10_2  = 6, /* 10.2 kbps */
  QVP_RTP_AMR_FT_RATE_12_2  = 7, /* 12.2 kbps */
  QVP_RTP_AMR_FT_RATE_SID   = 8, /* silence ind - 1.95 kbps */
  QVP_RTP_AMR_FT_RATE_NOD   = 15, /* No data zero len packet 0kbps */
  QVP_RTP_AMR_FT_INVALID    = 0xff /* notifies invalid */

} qvp_rtp_amr_toc_type;

/*--------------------------------------------------------------------------
    TYPEDEF STRUCT QVP_RTP_AMR_CONFIG_TYPE

    This strcture stores the configuration for a particular channel
    the AMR channel.

    Not all the configuration options are supported for the time being.
    All the capabilities of the layer is listed here. Any signalling
    entity tries misconfigure will be dealt with error codes/exceptions.
--------------------------------------------------------------------------*/
typedef struct
{

  /* receiver settings */
  boolean rx_octet_aligned;            /* flag whether the receiver is
                                        * octet aligned or bandwisth
                                        * saving mode
                                        */
  uint32   rx_amr_pkt_interval;         /* header less payload */
  boolean rx_interleave_on;            /* flags receiver interleaving */
  boolean rx_rob_sort_on;              /* flag to turn on robust sorting */
  boolean rx_crc_on;                   /* flag to turn on robust sorting */
  boolean rx_UED_UEP_on;               /* flag to turn on unequal Error
                                        * detection/correction
                                        */
  uint8 rx_ptime;
  uint8 rx_max_ptime;

  /* transmitter settings */
  boolean tx_octet_aligned;            /* flag whether the receiver is
                                        * octet aligned or bandwisth
                                        * saving mode
                                        */
  boolean tx_header_on;                /* header less payload */
  boolean tx_crc_on;                   /* flags presence of TX crc */
  uint8   tx_interleave_on;            /* flags transmit interleaving */
  uint8   tx_bundle_size;              /* bundle size in no of 20 ms
                                        * frames
                                        */
  boolean tx_UED_UEP_on;               /* flag to turn on unequal Error
                                        * detection/correction
                                        */
  boolean tx_rob_sort_on;              /* flag to turn on robust sorting */
  boolean tx_repetition_on;            /* used to flag repitition based
                                        * FEC
                                        */
  uint8 tx_ptime;
  uint8 tx_max_ptime;
} qvp_rtp_amr_config_type;

/*--------------------------------------------------------------------------
    TYPEDEF STRUCT QVP_RTP_AMR_RX_CTX

    This strcture stores the receiver context for the AMR channel.
--------------------------------------------------------------------------*/
typedef struct
{
  boolean valid;
  uint16  pkt_interval;

} qvp_rtp_amr_rx_ctx_type;

/*--------------------------------------------------------------------------
    TYPEDEF STRUCT QVP_RTP_AMR_TX_CTX

    This strcture stores the receiver context for the AMR channel.
--------------------------------------------------------------------------*/
typedef struct
{

  boolean valid;
  uint8 frame_cnt;         /* number of frames which got queued up in the
                            * stream when frame_cnt becomes bundle size we
                            * send a packet out
                            */
  uint8 op_packet[ QVP_RTP_MTU_DLFT ]; /* tx headr s preconfigured
                                        * statically * and then stuffed
                                        * in every packet
                                        */
  boolean marker;                      /* marker bit turned on */
  uint8 tx_hdr_len;                    /* actual len of headers in
                                        * bytes depending on the
                                        * bundle size
                                        */
  uint16 data_len;                     /* accumulated data len */

  uint32 first_tstamp;                 /* since the profile sends the time
                                        * stamp of the first packt in a
                                        * bundle we need to store it
                                        */
  boolean prev_silence_frame;          /* Indicates if the previous frame
                                        * sent is a silence frame
                                        */
  boolean all_silence_frames;          /* Indicates if the RTP packet
                                        * contains all silence frames
                                        */
} qvp_rtp_amr_tx_ctx_type;


/*--------------------------------------------------------------------------
    TYPEDEF STRUCT QVP_RTP_AMR_CTX_TYPE

  The total AMR context maintained in each channel
--------------------------------------------------------------------------*/
typedef struct
{

  boolean valid;
  qvp_rtp_amr_config_type stream_config;    /* the way the profile is
                                             * configured
                                             */
  qvp_rtp_amr_rx_ctx_type  rx_ctx;          /* receiver context */
  qvp_rtp_amr_tx_ctx_type  tx_ctx;          /* transmitter context */

} qvp_rtp_amr_ctx_type;

/*--------------------------------------------------------------------------
    TYPEDEF STRUCT  QVP_RTP_AMR_OAL_HDR_PARAM_TYPE

    Used as a container of info while parsing the header in octet aligned
    mode

    Taken from RFC 3267

        0                   1
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
   +-+-+-+-+-+-+-+-+- - - - - - - -
   |  CMR  |R|R|R|R|  ILL  |  ILP  |
   +-+-+-+-+-+-+-+-+- - - - - - - -

    Used internally for parsing purposes. Only the fixed header is
    stored here.
--------------------------------------------------------------------------*/
typedef struct
{

  uint8 cmr;           /* CMR Change mode req */
  uint8 ill;           /* interleaving length */
  uint8 ilp;           /* interleaving index */

} qvp_rtp_amr_oal_hdr_param_type;


//Added
typedef struct
{
  uint8 toc;
  boolean flag;
  uint8 mode;
  uint8 fqi;
  uint8 len;
} struct_toc_info;
//end



extern uint16 qvp_rtp_parse_amr_fixed_hdr_oa
(

  uint8 *data,
  uint16 len,
  qvp_rtp_amr_oal_hdr_param_type *hdr

);

//extern qvp_rtp_amr_profile_recv(void*, void*, qvp_rtp_buf_type*);
extern uint16 qvp_rtp_amr_count_tocs_oa(uint8 *data, uint16 len);
extern qvp_rtp_status_type qvp_rtp_amr_profile(qvp_rtp_buf_type *pkt);
//extern uint8 rtp_api(qvp_rtp_buf_type *aud_buf,
         //qvp_rtp_ctx_type *ctxStruct,
         //qvp_rtp_buf_type *pbufType);

#endif /* end of FEATURE_QVPHONE_RTP */
#endif /* end of  _H_QVP_RTP_AMR_H_ */
