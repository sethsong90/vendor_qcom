#ifndef _QVP_RTP_PAYLOAD_H_
#define _QVP_RTP_PAYLOAD_H_
/*=*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                         QVP_RTP_PAYLOAD . H

GENERAL DESCRIPTION

  This file defines the data types required to identify and configure
  different payload formats supported by the RTP Stack.

  The configuration of these payloads are designed to be RFC complete except
  for specifics like port numbers ip address etc. Those are dealt with at a
  higher abstract level by the companion file qvp_rtp_api.h

  For an app programmer this file should be read with the qvp_rtp_api.h
  file. Actually these data types will be used in those apis as a container
  to configure the payload formats for each channel opened.

  These data types will be used to read out the default configuration of
   a payload format module as well.

  All payloads supported will have a data type defined for its
  configuration.

EXTERNALIZED FUNCTIONS
  None


INITIALIZATION AND SEQUENCING REQUIREMENTS
  None

  Copyright (c) 2005 by Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary.  Export of this technology or software is
  regulated by the U.S. Government. Diversion contrary to U.S. law prohibited.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*=*/

/*===========================================================================

$Header: //source/qcom/qct/multimedia/videophone/ps_media/RTP/RTP_core/rel/1.0/inc/qvp_rtp_payload.h#1 $ $DateTime: 2008/04/15 18:56:58 $ $Author: yuanz $

                            EDIT HISTORY FOR FILE


when        who    what, where, why
--------    ---    ----------------------------------------------------------
01/14/08    apr    Adding sender bitrate & rtp bw params in RTCP config
12/31/07    grk    Moving maxptime/ptime to media level.
11/21/07    grk    Support for EVRCWB as per draft-ietf-avt-rtp-evrc-wb-09.
10/30/07    apr    Support for APTO ARR
04/24/07    apr    Fix for CR117260, include valid flag for silencesupp
03/14/07    grk    Fixed lint warnings.
03/22/07    apr    Removing rate parameter in dtmf & mp4 from fmtp params,
                   since it is encoded in rtpmap
02/28/07    grk    Added QOS params to qvp_rtp_rtcp_config_type
02/28/07    grk    Moved Qos and shared handles to qvp_rtp_pyld_qos_type.
02/28/07    grk    Added support for channel upgrade/downgrade.
11/06/06    uma    Added support for AVPF profile - generic NAK , RFC 4585
10/10/06    grk    Moved payload specific union outside
                   qvp_rtp_payload_config_type.
10/10/06    grk    Changed payload_type in qvp_rtp_pyld_rtp_rx_param_type
                   from uint16 to uint8.
10/10/06    grk    Removed payload_type from qvp_rtp_pyld_rtp_tx_param_type.
04/09/06    grk    Added inbound local port parameter to
                   qvp_rtp_pyld_rtp_rx_param_type
07/31/06    uma    Added Ch_dir to qvp_rtp_payload_config_type
                   Moved qvp_rtp_channel_type from qvp_rtp_api.h
07/24/06    grk    Added jitter parameters to qvp_rtp_pyld_rtp_rx_param_type
05/23/06    apr    Added support for rfc 2429, new payload config structure
                   for mime type H263-2000
05/31/06    uma    Support for EVRC0, EVRCB0, EVRCB, EVRC1, EVRCB1 & DTX
05/05/06    apr    Included RTP code in feature FEATURE_QVPHONE_RTP
03/04/06    apr    Modified DTMF events size to 16
02/08/06    uma    Name change of rtcp_config to rtcp_tx_config
                   in qvp_rtp_payload_config_type
01/18/06    apr    DTMF support. Added QVP_SDP_MAX_DTMF_EVENTS,
                   qvp_rtp_dtmf_tel_event_range_type,
                   qvp_rtp_dtmf_tel_event_mode_type,
                   qvp_rtp_dtmf_tel_event_sdp_config_type
                   Added dtmf configuration to qvp_rtp_payload_config_type
12/26/05    uma    1) Added rtcp_valid flag to qvp_rtp_rtcp_config_type
                   2) Added payload_valid flag to qvp_rtp_payload_config_type
                   3) Added qos valid flag to qvp_rtp_pyld_qos_type
                   4) Added qos to qvp_rtp_payload_config_type
12/09/05    apr    Renamed AMR mode min and max macros
12/09/05    apr    Changed interleaving AMR variable to uint16
                   from boolean
11/09/05    srk    1) Added Telephone Event MIME type. 2) Added
                   configuration of IP DSCP.
08/11/05    srk    Changing the data type for payload type no.
08/03/05    srk    Adding the connection ip at the top level
                   config and removing it from elsewhere.
07/28/05    srk    Adding RTCP tx port
07/21/05    srk    Added RTCP configuration parameters.
07/18/05    srk    Added QOS parameters for the RTP layer.
06/06/05    srk    1) Splitting RAW profiile in AUDIO and
                   Video 2)adding H.263 profile.
03/23/05    srk    Adding RTP related payload configuration.
02/16/05    srk    Initial creation.
===========================================================================*/
//#include "customer.h"
#if defined FEATURE_QVPHONE_RTP || defined FEATURE_QVPHONE_SDP

#define QVP_RTP_MAX_MP4_CONFIG_LEN       120
#ifndef QVP_RTP_IP_STR_LEN
#define QVP_RTP_IP_STR_LEN 16
#endif

#define QVP_RTP_MAX_FLOAT_STR_LEN        64
#define QVP_RTP_H263_SAMPLG_SUBMODES_NUM 4

/*--------------------------------------------------------------------------
  TYPEDEF uint32 QVP_RTP_QOS_HDL_TYPE

  The unique opaque QOS handle.
--------------------------------------------------------------------------*/
typedef uint32 qvp_rtp_qos_hdl_type;

/*--------------------------------------------------------------------------
  TYPEDEF uint32 QVP_RTP_SHARED_HDL_TYPE

  The unique opaque shared handle.
--------------------------------------------------------------------------*/
typedef uint32 qvp_rtp_shared_hdl_type;

/*--------------------------------------------------------------------------
  TYPEDEF STRUCT QVP_RTP_PYLD_RTP_TX_PARAM_TYPE

  The RTP specific parameters which can be set for any receive path
--------------------------------------------------------------------------*/
typedef struct
{
  uint8    payload_type;
  boolean  jitter_valid;
  uint16   jitter_size;
  uint16   ib_local_port;

  boolean ptime_valid;
  uint16  ptime;  /* time in ms in each packet( default 20 ms ) */

  boolean maxptime_valid;
  uint16  maxptime; /* maximum time in ms for each packet ( def :200 ) */
} qvp_rtp_pyld_rtp_rx_param_type;

/*--------------------------------------------------------------------------
    TYPEDEF STRUCT QVP_RTP_PYLD_APPQOSHDLS_TYPE

     Used to specify QOS handle settings by the application.
--------------------------------------------------------------------------*/
typedef struct
{

  boolean                  qos_hdl_valid;   /* set if qos_hdl is valid*/
  qvp_rtp_qos_hdl_type     qos_hdl;         /* unique handle to QOS flow
                                             * associated with stream
                                             * used by encoder to read
                                             * lower layer queues
                                             * for rate control
                                             */

  boolean                  shared_hdl_valid;/* set if shared_hdl is valid*/
  qvp_rtp_shared_hdl_type  shared_hdl;      /* shared handle associated
                                             * with set of QOS instances
                                             * created using a network hdl
                                             * can be used only with sock2
                                             */

} qvp_rtp_pyld_appqoshdls_type;

/*--------------------------------------------------------------------------
    TYPEDEF STRUCT QVP_RTP_PYLD_QOS_TYPE

     Used to specify QOS settings by the application.  Using this
     application can enable/disable packet shaping( video ). Also
     application can optionally set the IP TOS used in conjunction with
     QOS.
--------------------------------------------------------------------------*/
typedef struct
{

  boolean use_this_qos;    /* set if application wants specify the qos */

  boolean shaping_enabled; /* true when shaping is enabled -- only used
                            * for video profiles
                            */
  boolean use_this_tos;    /* set if application wants specify the IP
                            * header TOS
                            */
  uint16  tos;             /* IP header TOS value desired */

  qvp_rtp_pyld_appqoshdls_type  qos_handles;/* Qos and shared handle */

} qvp_rtp_pyld_qos_type;

/*--------------------------------------------------------------------------
  TYPEDEF STRUCT QVP_RTP_RTCP_CONFIG_TYPE

  This data type allows applications to control RTCP transmission
  and reception. This is required becuase RTCP sessions are tied to receptio
  -n of final answer as given in RFC3264.
--------------------------------------------------------------------------*/
typedef struct
{
  boolean rtcp_valid;         /* Set to valid if rtcp is to be configured */
  boolean rtcp_tx_enabled;    /* Enables Transmission of RTCP packets like SR
                               * RR SDES etc.
                               */
  uint16  tx_rtcp_port;       /* remote port to which report must be sent */

  boolean rtcp_tx_fb_enabled; /* Enables Transmission of RTCP feedback pkts
                               * for AVPF profile - RFC 4585
                               */

  boolean rtcp_apto_arr_enabled;
                              /* Enables transmission of RTCP APTO_ARR
                               * packets, application can set this if
                               * if rate adaptation is supported
                               */

  qvp_rtp_pyld_qos_type rtcp_qos;/* qos settings for control channel */

  uint32  rtp_bandwidth;      /* RTP stream bandwidth
                               * as negotiated in SDP b=
                               */

  uint32  sender_bitrate;      /* Maximum sender bitrate
                                * can be same as the rtp bw for now
                                * it can be negotiated in SDP also
                                */

} qvp_rtp_rtcp_config_type;

/*--------------------------------------------------------------------------
  TYPEDEF STRUCT QVP_RTP_PYLD_RTP_TX_PARAM_TYPE

  The RTP specific parameters which can be set for any transmit path
--------------------------------------------------------------------------*/
typedef struct
{

  uint16 remote_port;   /* negotiated remote port for the stream */
  uint16 ob_local_port; /* Port from which we transmit */
  boolean ptime_valid;
  uint16  ptime;  /* time in ms in each packet( default 20 ms ) */

  boolean maxptime_valid;
  uint16  maxptime; /* maximum time in ms for each packet ( def :200 ) */
} qvp_rtp_pyld_rtp_tx_param_type;

/*--------------------------------------------------------------------------
    TYPEDEF ENUM QVP_RTP_PAYLOAD_TYPE

    Enumeration which lists the payload format supported by the module.
    Application can open channels to any of these pay load types.

    Payload formatting is taken care by this module.


--------------------------------------------------------------------------*/
typedef enum
{

  QVP_RTP_PYLD_RAW_AUD,      /*  This profile sends the packets as is
                              *  use this profile if application
                              *  wants to implement its own payload
                              *  format or testing. This is reserved for
                              *  audio
                              */
  QVP_RTP_PYLD_RAW_VID,      /*  This profile sends the packets as is
                              *  use this profile if application
                              *  wants to implement its own payload
                              *  format or testing. This is reserved for
                              *  video
                              */
  QVP_RTP_PYLD_PROPREITARY,  /* propreitary payload format by QC (QTPD) */

  QVP_RTP_PYLD_EVRC,         /* EVRC payload format */
  QVP_RTP_PYLD_AMR,          /* AMR payload format */
  QVP_RTP_PYLD_MP4_ES,       /* mpeg4 payload format RFC3016 */
  QVP_RTP_PYLD_MP4_GENERIC,  /* mpeg4 payload format RFC3640 */
  QVP_RTP_PYLD_H263,         /* H263 profile as in RFC2190 */
  QVP_RTP_PYLD_H263_1998,    /* H263 profile as in RFC2429 */
  QVP_RTP_PYLD_H263_2000,    /* H263 profile as in RFC2429 with fmtp */
  QVP_RTP_PYLD_G723,         /* G723 Audio codec          */
  QVP_RTP_PYLD_TEL_EVENT,    /* DTMF events as given in rfc2833 */
  QVP_RTP_PYLD_EVRC0,        /* header free, single frame per RTP pkt */
  QVP_RTP_PYLD_EVRCB0,       /* header free, single frame per RTP pkt */
  QVP_RTP_PYLD_EVRCB,        /* interleaved/bundled header format */
  QVP_RTP_PYLD_EVRC1,        /* Compact bundled header free format */
  QVP_RTP_PYLD_EVRCB1,       /* Compact bundled header free  format */
  QVP_RTP_PYLD_EVRCWB,        /* interleaved/bundled header format */
  QVP_RTP_PYLD_EVRCWB0,       /* header free, single frame per RTP pkt */
  QVP_RTP_PYLD_EVRCWB1,       /* Compact bundled header free  format */
  /* all supported payload formats to be added beofre here */
  QVP_RTP_PYLD_NIL           /* INVALID payload format */

} qvp_rtp_payload_type;


/*--------------------------------------------------------------------------
  TYPEDEF STRUCT QVP_RTP_DTX_SDP_ATTR_TYPE

  This data type collapses all the discontinuous transmission support
  parameters as in draft-xie-avt-compact-bundle-evrc-01

  DTX is supported by mime subtype EVRC0, EVRC, EVRC1, EVRCB0, EVRCB, EVRCB1
--------------------------------------------------------------------------*/
typedef struct
{

  /*------------------------------------------------------------------------
    As per RFC 4788 & EVRC Drafts -
    silencesupp_valid - FALSE : Default -
                                silncesupp is supported
                                DTX values are used from fmtp params or
                                default values are used from spec
                                DTX of Speech in cdma 2000(3GPP2 C.S0076-0)
    silencesupp_valid - TRUE
       silence_supp 0         : Silencesupp not supported
                                ( dtxmax & hangover should be false )
       silence supp 1         : Supported & DTX values are taken from
                                fmtp params dtxmax & dtxmin if specified,
                                else codec uses default DTX values
  ------------------------------------------------------------------------*/

  boolean                   silencesupp_valid;

  boolean                   silencesupp;

  boolean                   dtxmax_valid;

  uint8                     dtxmax;/* Max sil frames that can be suppressed*/

  boolean                   dtxmin_valid;

  uint8                     dtxmin;/* Min sil frames that shld be supp */

  boolean                   hangover_valid;

  uint8                     hangover;/* Sil frames txd before supp*/

}qvp_rtp_dtx_sdp_config_type;


/*--------------------------------------------------------------------------
  TYPEDEF STUCT QVP_RTP_EVRC_INTL_BUND_SDP_CONFIG_TYPE

  This structure will store all the information that can be signalled for an
  EVRC codec based payload implementation which is based on RFC3558.
  and draft-ietf-avt-compact-bundled-evrc-04.txt

  The signalling is done through SIP offer/answer model as in RFC3264


  Since fields are optional and application is assumed agnostic to the
  meaning of default values of these fields
  we flag presence of each field with a boolean.

  This structure is for the interleaved or bundled packet support
  The mime using these attributes are EVRC, EVRCB
--------------------------------------------------------------------------*/
typedef struct
{

  boolean valid;     /* set this flag if the remainder of the fields are
                       * valid
                       */

  /* REQUIRED PARAMS */
  /* NONE */

/*--------------------------------------------------------------------------
  optional params common to evrc based codecs - EVRC, EVRCB
--------------------------------------------------------------------------*/

  /* fmtp params */
  boolean maxinterleave_valid;
  uint16  maxinterleave;  /* maximum interleaving len LLL in the header */

  qvp_rtp_dtx_sdp_config_type evrc_dtx; /* dtx attributes */

  /* This applies only to EVRCWB*/
  boolean           mode_set_recv_valid;
  uint8             mode_map; /* decoder's prferred mode of operation */

  /* This applies only to EVRCWB and EVRCB*/
  boolean           sendmode_valid;
  uint8             sendmode; /* encoder's preferred mode of operation */

  /* This applies only to EVRCB*/
  boolean           recvmode_valid;
  uint8             recvmode; /* decoder's preferred mode of operation */

} qvp_rtp_evrc_intl_bund_sdp_config_type;


/*--------------------------------------------------------------------------
  TYPEDEF ENUM QVP_RTP_EVRC_RATE_TYPE
  This enum specifies the codec rates supported for EVRCB1 and EVRC1
--------------------------------------------------------------------------*/
typedef enum
{
  QVP_RTP_EVRC1_RATE_BLANK,    /* blank rate, frame len 0 */
  QVP_RTP_EVRC1_RATE_1_2,      /* 1/2rate */
  QVP_RTP_EVRC1_RATE_1,        /* full rate */
  QVP_RTP_EVRC1_ERASURE,       /* used locally for missing frames*/
  QVP_RTP_EVRC1_INVALID        /* used to flag error */

} qvp_rtp_evrcb_rate_type;

/*--------------------------------------------------------------------------

  Please refer to table 2.5.1.2-1 of 3GPP2 C.S0014-C and
  Table 2-6 of 3GPP2 C.S0014-B.
  The rates mentioned in the comments below are channel encoding rates.
--------------------------------------------------------------------------*/
#define QVP_SDP_ENABLE_EVRC_MODE_0  0x1  /* EVRCB-9.3kbps, EVRCWB-8.5kbps */
#define QVP_SDP_ENABLE_EVRC_MODE_1  0x2  /* EVRCB-8.5 kbps */
#define QVP_SDP_ENABLE_EVRC_MODE_2  0x4  /* EVRCB-7.5 kbps  */
#define QVP_SDP_ENABLE_EVRC_MODE_3  0x8  /* EVRCB-7.0 kbps */
#define QVP_SDP_ENABLE_EVRC_MODE_4  0x10 /* EVRCB-6.6 kbps,EVRCWB-9.3kbps*/
#define QVP_SDP_ENABLE_EVRC_MODE_5  0x20 /* EVRCB-6.2 kbps */
#define QVP_SDP_ENABLE_EVRC_MODE_6  0x40 /* EVRCB-5.8 kbps */
#define QVP_SDP_ENABLE_EVRC_MODE_7  0x80 /* EVRCB-4.8 kbps,EVRCWB-4.8kbps*/
#define QVP_SDP_EVRC_MODE_MIN       0    /* max rate */
#define QVP_SDP_EVRC_MODE_MAX       7    /* min rate */

/*--------------------------------------------------------------------------
  TYPEDEF STUCT QVP_RTP_EVRC_COMP_BUND_SDP_CONFIG_TYPE

  This structure is for the Compact Bundled packet
  (Header free, fixed rate) support
  The mime using these attributes are EVRC1, EVRCB1
--------------------------------------------------------------------------*/
typedef struct
{

  boolean valid;                          /* set this flag if the remaining
                                          * fields are valid
                                          */

  /* REQUIRED PARAMS */
  /* NONE */

  /*--------------------------------------------------------------------------
    OPTIONAL PARAMS common to evrc based codecs   EVRC1, EVRCB1
  --------------------------------------------------------------------------*/

  /* fmtp  attributes */
  boolean                      fixedrate_valid;
  qvp_rtp_evrcb_rate_type      fixedrate; /*Fixed rate of codec frames */
  qvp_rtp_dtx_sdp_config_type  evrc_dtx;  /* dtx info not used by RTP*/

  /* This applies only to EVRCWB1*/
  boolean           mode_set_recv_valid;
  uint8             mode_map; /* decoder's prferred mode of operation */

  /* This applies only to EVRCWB1*/
  boolean           sendmode_valid;
  uint8             sendmode; /* encoder's preferred mode of operation */

}qvp_rtp_evrc_comp_bund_sdp_config_type;

/*--------------------------------------------------------------------------
  TYPEDEF STUCT QVP_RTP_EVRC_HDR_FREE_SDP_CONFIG_TYPE

  This structure is for the Header free packet support
  The mime using these attributes are EVRC0, EVRCB0
--------------------------------------------------------------------------*/
typedef struct
{
  boolean valid;
  qvp_rtp_dtx_sdp_config_type evrc_dtx; /* dtx info not used by RTP*/


  /* This applies only to EVRCWB0*/
  boolean           mode_set_recv_valid;
  uint8             mode_map; /* decoder's prferred mode of operation */

  /* This applies only to EVRCWB0 and EVRCB0*/
  boolean           sendmode_valid;
  uint8             sendmode; /* encoder's preferred mode of operation */

  /* This applies only to EVRCB0*/
  boolean           recvmode_valid;
  uint8             recvmode; /* decoder's preferred mode of operation */

}qvp_rtp_evrc_hdr_free_sdp_config_type;




/*--------------------------------------------------------------------------
TYPEDEF STUCT QVP_RTP_G723_SDP_CONFIG_TYPE

This structure will store all the information that can be signalled for an
G723 payload implementation which is based on RFC3551/RFC3555.
The signalling is done through SIP offer/answer model as in RFC3264


Since fields are optional and application is assumed agnostic to the
meaning of default values of these fields we flag presence of each field
with a boolean.
--------------------------------------------------------------------------*/
typedef struct
{

  boolean valid;     /* set this flag if the remainder of the fields are
                       * valid
                       */

  /* REQUIRED PARAMS */
  /* NONE */

  /* OPTIONAL PARAMS */

  boolean bitrate_valid;
  uint8   bitrate;  /* bitrate prefered in khz */

  boolean annexa_valid;
  boolean annexa;   /* flags Annex A voice activity detection */


} qvp_rtp_g723_sdp_config_type;

/*--------------------------------------------------------------------------

  Please refer to table 1a of TS 26.101
--------------------------------------------------------------------------*/
#define SDP_ENABLE_AMR_MODE_0  0x1    /* 4.75 kbps */
#define SDP_ENABLE_AMR_MODE_1  0x2    /* 5.15 kbps */
#define SDP_ENABLE_AMR_MODE_2  0x4    /* 5.9 kbps  */
#define SDP_ENABLE_AMR_MODE_3  0x8    /* 6.17 kbps */
#define SDP_ENABLE_AMR_MODE_4  0x10   /* 7.4 kbps  */
#define SDP_ENABLE_AMR_MODE_5  0x20   /* 7.95 kbps */
#define SDP_ENABLE_AMR_MODE_6  0x40   /* 10.2 kbps */
#define SDP_ENABLE_AMR_MODE_7  0x80   /* 12.2 kbps */
#define SDP_AMR_MODE_MIN       0      /* min rate */
#define SDP_AMR_MODE_MAX       7      /* min rate */


/*--------------------------------------------------------------------------
TYPEDEF STUCT QVP_RTP_AMR_SDP_CONFIG_TYPE

This structure will store all the information that can be signalled for an
AMR payload implementation which is based on RFC3267. The signalling
is done through SIP offer/answer model as in RFC3264


Since fields are optional and application is assumed agnostic to the meaning
of default values of these fields we flag presence of each field with a
boolean.
--------------------------------------------------------------------------*/
typedef struct
{
  boolean valid;     /* set this flag if the remainder of the fields are
                      * valid
                      */

  /* REQUIRED PARAMS */
  /* NONE */

  /* OPTIONAL PARAMS */
   boolean           octet_align_valid; /* was signalled or not */
   boolean           octet_align;  /* default value zero which maps
                                    * bandwidth efficient mode
                                    */

   boolean           mode_set_valid;
   uint32            mode_map; /* codec modes supported */

   boolean           mode_change_period_valid; /* valid mode change
                                                * period
                                                */
   uint32            mode_change_period;       /* number of mode consecutive
                                                * frames before successive
                                                * mode changes
                                                */
   boolean           mode_change_neighbor_valid;
   boolean           mode_change_neighbor;      /* if valid you can only
                                                 * change to adjacent permi
                                                 * ssible mode
                                                 */

   boolean           crc_valid;                 /* valid if the SDP
                                                 * contained CRC tag
                                                 */
   boolean           crc;                       /* crc is flagged or not */

   boolean           robust_sorting_valid;
   boolean           robust_sorting;            /* robust sorting employed
                                                 * or not
                                                 */


   boolean           interleaving_valid;
   uint16            interleaving;             /* flags interleaving */

   boolean           channels_valid;
   uint16            channels;     /* number of channesls we will have 1 */

} qvp_rtp_amr_sdp_config_type;

/*--------------------------------------------------------------------------
   TYPEDEF STRUCT QVP_RTP_MP4_ES_SDP_CONFIG_TYPE

   This data type collapses all the MIME parameters pertaining to the
   payload format implementation as in RFC3016.

   The MIME tag is MP4V-ES as subtype
--------------------------------------------------------------------------*/
typedef struct
{
  boolean valid;

  /* REQUIRED PARAMETERS */
  /* none */

  /* OPTIONAL PARAMETERS */

  boolean profile_level_id_valid; /* flags the field */
  uint16  profile_level_id;       /* flags the field */

  boolean config_valid;
  uint16  config_len;
  uint8   config[ QVP_RTP_MAX_MP4_CONFIG_LEN ];


} qvp_rtp_mp4_es_sdp_config_type;

/*--------------------------------------------------------------------------
   TYPEDEF STRUCT QVP_H263_K_SLICE_ORDER

   Slice order type for H263+
--------------------------------------------------------------------------*/
typedef enum
{

  QVP_H263_K_SLICE_ORDER_NONE      = 0,
  QVP_H263_K_IN_ORDER_NON_RECT     = 1,
  QVP_H263_K_IN_ORDER_RECT         = 2,
  QVP_H263_K_NOT_IN_ORDER_RECT     = 3,
  QVP_H263_K_NOT_IN_ORDER_NON_RECT = 4

}qvp_h263_k_slice_order;

/*--------------------------------------------------------------------------
   TYPEDEF STRUCT QVP_H263_N_REF_PIC_MODE

    Reference Picture Selection mode - Four numeric choices
--------------------------------------------------------------------------*/
typedef enum
{

  QVP_H263_N_MODE_NONE   = 0,  /* none selected */
  QVP_H263_N_NEITHER     = 1,  /* no back chnl data from decoder */
  QVP_H263_N_ACK         = 2,  /* only ack msgs return by decoder */
  QVP_H263_N_NACK        = 3, /* only nack msgs return by decoder */
  QVP_H263_N_ACK_NACK    = 4   /* decoder returns both ack and nack */

}qvp_h263_n_ref_pic_mode;

/*--------------------------------------------------------------------------
   TYPEDEF STRUCT QVP_H263_P_SAMPLG_SUBMODE

    Reference Picture Selection mode - Four numeric choices
--------------------------------------------------------------------------*/
typedef enum
{

  QVP_H263_P_SUBMODE_NONE = 0,  /* none selected */
  QVP_H263_P_RESIZE_BY_4  = 1,  /* Dynamic pic resizing by 4 */
  QVP_H263_P_RESIZE_BY_16 = 2,  /* Dynamic pic resizing by 16 */
  QVP_H263_P_WARP_HALF    = 3,  /* Dynamic warping half */
  QVP_H263_P_WARP_SIXTEEN = 4   /* Dynamic warping sixteen */

}qvp_h263_p_samplg_submode;

/*--------------------------------------------------------------------------
   TYPEDEF STRUCT QVP_H263_CUSTOM_MPI_PARAM

     Specifies the MPI for a custom defined resolution.
--------------------------------------------------------------------------*/
typedef struct
{

  uint16 Xmax;
  uint16 Ymax;
  uint16 mpi;

}qvp_h263_custom_mpi_param;

/*--------------------------------------------------------------------------
   TYPEDEF STRUCT QVP_RTP_H263+_common_params

   This data type collapses all the MIME parameters common to H263_1998
   and H263_2000 as listed in draft for RFC 2429
--------------------------------------------------------------------------*/
typedef struct
{
  /* PARAMETERS */
  boolean sqcif_mpi_valid;   /* flags presence of sqcif mpi param */
  uint16   sqcif_mpi;        /* value of mpi for SCQIF format     */

  boolean qcif_mpi_valid;    /* flags presence of qcif mpi param  */
  uint16   qcif_mpi;         /* value of mpi for QCIF format      */

  boolean cif_mpi_valid;     /* flags presence of cif mpi param   */
  uint16   cif_mpi;          /* value of mpi for CIF format       */

  boolean cif4_mpi_valid;    /* flags presence of cif4 mpi param  */
  uint16   cif4_mpi;         /* value of mpi for CIF4 format      */

  boolean cif16_mpi_valid;   /* flags presence of cif16 mpi param */
  uint16   cif16_mpi;         /* value of mpi for CIF16 format     */

  qvp_h263_custom_mpi_param custom_mpi;
  boolean custom_mpi_valid;  /* Specifies the MPI for a custom    */
                             /* defined resolution.               */

  boolean f_annex_valid;     /* flags presence of F annex */
  boolean i_annex_valid;     /* flags presence of I annex */
  boolean j_annex_valid;     /* flags presence of J annex */
  boolean t_annex_valid;     /* flags presence of T annex */

  boolean k_annex_valid;     /* flags presence of K annex */
  qvp_h263_k_slice_order   k_slice_oder;
                             /* it gives the slice order  */

  boolean n_annex_valid;     /* flags presence of N annex */
  qvp_h263_n_ref_pic_mode   n_ref_pic_mode;
                             /* it gives reference picture
                              * selection mode
                              */

  uint16  p_num_of_modes;     /* flags presence of P annex */
  qvp_h263_p_samplg_submode   \
          p_samplg_modes[QVP_RTP_H263_SAMPLG_SUBMODES_NUM];
                             /* it gives reference picture
                              * sampling submode
                              */

  boolean par_valid;         /* flags presence of PAR     */
  uint16   par_value;        /* Arbitrary Pixel Aspect Ratio
                              */

  boolean cpcf_valid;        /* flags presence of CPCF     */
  uint8   cpcf_value[QVP_RTP_MAX_FLOAT_STR_LEN];
                             /* Arbitrary (Custom) Picture
                              * Clock Frequency
                              */

  boolean bpp_valid;         /* flags presence of BPP     */
  uint16  bpp_value;         /* BitsPerPictureMaxKb: Maximum
                              * number of bits in units of 1024
                              * bits allowed to represent a
                              * single picture.
                              */

  boolean hrd_valid;         /* Hypothetical Reference Decoder */

}qvp_rtp_h263_new_common_params;

/*--------------------------------------------------------------------------
   TYPEDEF STRUCT QVP_RTP_H263_2000_SDP_CONFIG_TYPE

   This data type collapses all the MIME parameters pertaining to the
   payload format implementation as in RFC2429.

   The MIME tag is H263-2000 as subtype
--------------------------------------------------------------------------*/
typedef struct
{
  boolean valid;
  /* REQUIRED PARAMETERS */
  /* none */

  /* OPTIONAL PARAMETERS */
  boolean profile_valid;     /* flags presence of profile  */
  uint16  profile;           /* Specifies H263 supported annexes */

  boolean level_valid;       /* flags presence of level field */
  uint16  level;             /* Level of bitstream operation
                              * specifies level of comutation
                              * complexity of decoding process
                              */

  boolean interlace_valid;   /* flags presence of level field */
  uint16  interlace;         /* indicates the support for interlace
                              * display mode as specified in H.263
                              * annex W.6.3.11.
                              */

 qvp_rtp_h263_new_common_params common_params;
                              /* new optional params set as per
                               * draft on rfc 2429
                               */

} qvp_rtp_h263_2000_sdp_config_type;

/*--------------------------------------------------------------------------
   TYPEDEF STRUCT QVP_RTP_H263_1998_SDP_CONFIG_TYPE

   This data type collapses all the MIME parameters pertaining to the
   payload format implementation as in RFC2429.

   The MIME tag is H263-1998 as subtype
--------------------------------------------------------------------------*/
typedef struct
{
  boolean valid;
  /*OPTIONAL PARAMETERS */
  qvp_rtp_h263_new_common_params params;

}qvp_rtp_h263_1998_sdp_config_type;

/*--------------------------------------------------------------------------
   TYPEDEF STRUCT QVP_RTP_MP4_GENERIC_SDP_CONFIG_TYPE

   This data type collapses all the MIME parameters pertaining to the
   payload format implementation as in RFC3640.

   The MIME tag is MP4V-generic as subtype
--------------------------------------------------------------------------*/
typedef struct
{
  boolean valid;

  /* REQUIRED PARAMETERS */
  boolean streamType_valid;
  uint16  streamType;

  boolean  profile_level_id_valid;
  uint16  profile_level_id;


  boolean config_valid;
  uint8   config[ QVP_RTP_MAX_MP4_CONFIG_LEN ];

  boolean mode_valid;
  uint16  mode;

  /* OPTIONAL PARAMETERS */
  boolean objectType_valid;
  uint16  objectType;

  boolean constantSize_valid;
  uint16  constantSize;

  boolean constantDuration_valid;
  uint16  constantDuration;

  boolean maxDisplacement_valid;
  uint16  maxDisplacement;

  boolean de_interleaveBufferSize_valid;
  uint16  de_interleaveBufferSize;

  boolean sizeLength_valid;
  uint16  sizeLength;  /* zero means we are flagging the 3016 mode of
                        * operation
                        */

  boolean indexLength_valid;
  uint16  indexLength; /* used by encoder and decoder inside the profile */

  boolean indexDeltaLength_valid;
  uint16  indexDeltaLength; /* used by profile encoder and decoder */

  boolean CTSDeltaLength_valid;
  uint16  CTSDeltaLength;   /* Coding time stamp delta */

  boolean DTSDeltaLength_valid;
  uint16  DTSDeltaLength;   /* decode timestamp delta */

  boolean randomAccessIndication_valid;
  boolean randomAccessIndication; /* default false. Configure flagging
                                   * this in AU
                                   */

  boolean streamStateIndication_valid;
  uint16  streamStateIndication;   /* used by some streams */

  boolean auxiliaryDataSizeLength_valid;
  uint16  auxiliaryDataSizeLength; /* used to parse or skip aux info inside
                                    * the profile
                                    */

} qvp_rtp_mp4_generic_sdp_config_type;



/*--------------------------------------------------------------------------
   Maximum number of dtmf elements supported. RFC 2833 list a total of
   173 events, of which we support 0-15
--------------------------------------------------------------------------*/

#define QVP_SDP_MAX_DTMF_EVENTS 16


/*--------------------------------------------------------------------------
   TYPEDEF STRUCT QVP_RTP_DTMF_TEL_EVENT_RANGE_TYPE
   This data type is to store the range of telephone events for dtmf
--------------------------------------------------------------------------*/
typedef struct
 {
    uint16 start_event; /* rfc 2833 lists 173 events, so 8 bit should be ok*/
    uint16 end_event;
  }qvp_rtp_dtmf_tel_event_range_type;

/*--------------------------------------------------------------------------
   TYPEDEF ENUM QVP_RTP_DTMF_TEL_EVENT_MODE_TYPE
   This enum specifies whether it is a single event or an event range
--------------------------------------------------------------------------*/
typedef enum
 {
    QVP_RTP_DTMF_TEL_NO_EVENT ,
    QVP_RTP_DTMF_TEL_SINGLE_EVENT,
    QVP_RTP_DTMF_TEL_EVENT_RANGE
  }qvp_rtp_dtmf_tel_event_mode_type;




/*--------------------------------------------------------------------------
   TYPEDEF STRUCT QVP_RTP_DTMF_TEL_EVENT_SDP_CONFIG_TYPE

   This data type collapses all the DTMF parameters pertaining to the
   payload format implementation as in RFC2833.

   The MIME tag is telephone-event as subtype
--------------------------------------------------------------------------*/
typedef struct
{
  boolean valid;

  /* REQUIRED PARAMETERS */
  /* none */

  /* OPTIONAL PARAMETERS */

  boolean events_valid;   /* Indicates if events param is present */
    /*------------------------------------------------------------------------
     event list can be a combination of single integers or range of events
     as in events=16-25,30,32
  ------------------------------------------------------------------------*/
  struct
  {

    qvp_rtp_dtmf_tel_event_mode_type event_mode;
    union
    {
      uint32 single_event;
      qvp_rtp_dtmf_tel_event_range_type event_range;
    }event;

  } events_list[ QVP_SDP_MAX_DTMF_EVENTS ];

}qvp_rtp_dtmf_tel_event_sdp_config_type;

/*--------------------------------------------------------------------------
     TYPEDEF ENUM QVP_RTP_CHANNEL_TYPE

     Enumeraion which describes the open properties. Is this is uni
     drectional outbound / uinidirectional inbound / bi directional open

--------------------------------------------------------------------------*/
typedef enum
{
  QVP_RTP_CHANNEL_OUTBOUND,     /* stream being opened is outbound only */
  QVP_RTP_CHANNEL_INBOUND,      /* stream being opened is inbound only */
  QVP_RTP_CHANNEL_FULL_DUPLEX,  /* stream being opened is inbound
                                 * and outbound only
                                 */

} qvp_rtp_channel_type;

/*--------------------------------------------------------------------------
  TYPEDEF STRCUT QVP_RTP_CONFIG_RX_PAYLD_PARAMS_TYPE

  This structure collapses all the possible SDP based configuration of
  rx payload formats.
--------------------------------------------------------------------------*/
typedef union
{

  /* EVRC RX */
  qvp_rtp_evrc_intl_bund_sdp_config_type  evrc_rx_config;

  /* AMR RX */
  qvp_rtp_amr_sdp_config_type             amr_rx_config;

  /* G723 codec */
  qvp_rtp_g723_sdp_config_type            g723_rx_config;

  /* MP4 ES ( rfc3016 ) RX */
  qvp_rtp_mp4_es_sdp_config_type          mp4_es_rx_config;

  /* MP4 GENERIC ( rfc3640 ) RX */
  qvp_rtp_mp4_generic_sdp_config_type     mp4_gen_rx_config;

  /* DTMF telephone event ( rfc2833 ) RX */
  qvp_rtp_dtmf_tel_event_sdp_config_type  tel_event_rx_config;

  /* EVRC0 - Header free packet format - not used */
  qvp_rtp_evrc_hdr_free_sdp_config_type   evrc0_rx_config;

  /* EVRCB0 Header free packet format - not used */
  qvp_rtp_evrc_hdr_free_sdp_config_type   evrcb0_rx_config;

  /* EVRCB Interleaved/Bundled variable rate with header */
  qvp_rtp_evrc_intl_bund_sdp_config_type  evrcb_rx_config;

  /* EVRC1 Compact bundled fixed rate header free format */
  qvp_rtp_evrc_comp_bund_sdp_config_type  evrc1_rx_config;

  /* EVRCB1 Compact bundled fixed rate header free format */
  qvp_rtp_evrc_comp_bund_sdp_config_type  evrcb1_rx_config;


  /* EVRCWB0 Header free packet format  */
  qvp_rtp_evrc_hdr_free_sdp_config_type   evrcwb0_rx_config;

  /* EVRCWB Interleaved/Bundled variable rate with header */
  qvp_rtp_evrc_intl_bund_sdp_config_type  evrcwb_rx_config;

  /* EVRCWB1 Compact bundled fixed rate header free format */
  qvp_rtp_evrc_comp_bund_sdp_config_type  evrcwb1_rx_config;

  /* H263-2000 ( rfc2429 ) TX */
  qvp_rtp_h263_2000_sdp_config_type   h263_2000_rx_config;

  /* H263-1998 ( rfc2429 ) RX */
  qvp_rtp_h263_1998_sdp_config_type   h263_1998_rx_config;


} qvp_rtp_config_rx_payld_params_type;
/*--------------------------------------------------------------------------
  TYPEDEF STRCUT QVP_RTP_CONFIG_TX_PAYLD_PARAMS_TYPE

  This structure collapses all the possible SDP based configuration of
  tx payload formats.
--------------------------------------------------------------------------*/
typedef union
{

  /* EVRC TX */
  qvp_rtp_evrc_intl_bund_sdp_config_type  evrc_tx_config;

  /* AMR TX */
  qvp_rtp_amr_sdp_config_type             amr_tx_config;

  /* G723 codec */
  qvp_rtp_g723_sdp_config_type            g723_tx_config;

  /* MP4 ES ( rfc3016 ) TX */
  qvp_rtp_mp4_es_sdp_config_type          mp4_es_tx_config;

  /* MP4 GENERIC ( rfc3640 ) TX */
  qvp_rtp_mp4_generic_sdp_config_type     mp4_gen_tx_config;

  /* DTMF telephone event ( rfc2833 ) TX */
  qvp_rtp_dtmf_tel_event_sdp_config_type  tel_event_tx_config;

  /* EVRC0 - Header free packet format - not used  */
  qvp_rtp_evrc_hdr_free_sdp_config_type   evrc0_tx_config;

  /* EVRCB0 Header free packet format - not used */
  qvp_rtp_evrc_hdr_free_sdp_config_type   evrcb0_tx_config;

  /* EVRCB Interleaved/Bundled variable rate with header */
  qvp_rtp_evrc_intl_bund_sdp_config_type  evrcb_tx_config;

  /* EVRC1 Compact bundled fixed rate header free format */
  qvp_rtp_evrc_comp_bund_sdp_config_type  evrc1_tx_config;

  /* EVRCB1 Compact bundled fixed rate header free format */
  qvp_rtp_evrc_comp_bund_sdp_config_type  evrcb1_tx_config;

  /* EVRCWB0 Header free packet format  */
  qvp_rtp_evrc_hdr_free_sdp_config_type   evrcwb0_tx_config;

  /* EVRCWB Interleaved/Bundled variable rate with header */
  qvp_rtp_evrc_intl_bund_sdp_config_type  evrcwb_tx_config;

  /* EVRCWB1 Compact bundled fixed rate header free format */
  qvp_rtp_evrc_comp_bund_sdp_config_type  evrcwb1_tx_config;

  /* H263-2000 ( rfc2429 ) TX */
  qvp_rtp_h263_2000_sdp_config_type   h263_2000_tx_config;

  /* H263-1998 ( rfc2429 ) TX */
  qvp_rtp_h263_1998_sdp_config_type   h263_1998_tx_config;


} qvp_rtp_config_tx_payld_params_type;
/*--------------------------------------------------------------------------
  TYPEDEF STRCUT QVP_RTP_PAYLOAD_CONFIG2_TYPE

  This structure collapses SDP based configuration of payload formats for
  tx and rx side.
--------------------------------------------------------------------------*/
typedef struct
{
  boolean                              config_tx_valid;/* Flags the
                                                        * configuration
                                                        * of tx side
                                                        */
  qvp_rtp_config_tx_payld_params_type  config_tx;      /* tx payload
                                                        * configuration
                                                        * parameter
                                                        */
  boolean                              config_rx_valid;/* Flags the
                                                        * configuration
                                                        * of rx side
                                                        */
  uint8                                rx_payload_type;/* rx payload type */

  qvp_rtp_config_rx_payld_params_type  config_rx;      /* rx payload
                                                        * configuration
                                                        * parameter
                                                        */
} qvp_rtp_payload_config2_type;

/*--------------------------------------------------------------------------
  TYPEDEF STRCUT QVP_RTP_PAYLOAD_CONFIG_TYPE

  This structure collapses all the possible SDP based congiguration of
  payload formats.

  There is 2 similar allocation for each type.

  One for TX and one for RX.
--------------------------------------------------------------------------*/

typedef struct
{

  boolean                   valid; /* set this flag if the remainder
                                    * of the fields are
                                    * valid
                                    */
  /*------------------------------------------------------------------------
    Payload types. Uniquely identifies all the payloads understood and
    supported by this module.
  ------------------------------------------------------------------------*/
  boolean                   payload_valid; /* set if the pyld has valid value.
                                            * can be false if the config does
                                            * not involve change in pyld enum
                                            * has to be true for a standalone
                                            * config validate request */

  qvp_rtp_payload_type      payload;       /* set to pyld enum if
                                            * valid flag is set */


  /*------------------------------------------------------------------------
    Used if a channel upgrade/downgrade is required
  ------------------------------------------------------------------------*/
  boolean                   chdir_valid; /* set this if the ch_dir
                                          * has a valid value.*/

  qvp_rtp_channel_type      ch_dir;  /* channel direction of this config*/

  /*------------------------------------------------------------------------
    To configure RTP bandwidth
  ------------------------------------------------------------------------*/
  boolean                rtp_bw_valid;
  uint32                  rtp_stream_bw;

  /*------------------------------------------------------------------------
    Following is the configuration of RTCP.
  ------------------------------------------------------------------------*/
  qvp_rtp_rtcp_config_type rtcp_tx_config;

  /*------------------------------------------------------------------------
       connection ip address as specified in C line or chached by app
  ------------------------------------------------------------------------*/
  uint8                    connection_ip[QVP_RTP_IP_STR_LEN];

  /*------------------------------------------------------------------------
    Collapses all the payload parameters on this structure

    Below is RX plane
    Of course this depends on type of channel we are talking
    ( recvonly or sendonly or sendrecv )
  ------------------------------------------------------------------------*/
  struct
  {

    boolean                              valid;       /* remaining has
                                                       * meaning only if
                                                       * this flag is set
                                                       */

    qvp_rtp_pyld_rtp_rx_param_type       rx_rtp_param;/*
                                                       * rtp specific
                                                       * parameters
                                                       */
    qvp_rtp_config_rx_payld_params_type  config_rx_payld_params;/* payload
                                                                 * specific
                                                                 * parameters
                                                                 * on rx side
                                                                 */

  } config_rx_params;


  /*------------------------------------------------------------------------
    Collapses all the payload parameters on this structure

    Below is TX plane
    Of course this depends on type of channel we are talking
    ( recvonly or sendonly or sendrecv )
  ------------------------------------------------------------------------*/
  /*------------------------------------------------------------------------
   qos setting for the outbound stream.
  ------------------------------------------------------------------------*/
  qvp_rtp_pyld_qos_type qos_config;

  struct
  {

    boolean                              valid;       /* remaining has
                                                       * meaning only if
                                                       * this flag is set
                                                       */

    qvp_rtp_pyld_rtp_tx_param_type       tx_rtp_param; /*
                                                        * rtp specific
                                                        * parameters.
                                                        * Payload type
                                                        * is not needed
                                                        * as a part of
                                                        * this struct as
                                                        * tx payload type
                                                        * is assigned on
                                                        * per packet
                                                        * basis while
                                                        * transmitting
                                                        * packets.
                                                        */

    qvp_rtp_config_tx_payld_params_type  config_tx_payld_params;/* payload
                                                                 * specific
                                                                 * parameters
                                                                 * on tx side
                                                                 */

  } config_tx_params;

} qvp_rtp_payload_config_type;

#endif /* end of FEATURE_QVPHONE_RTP */
#endif /*  end of ifndef _QVP_RTP_PAYLOAD_H_ */



