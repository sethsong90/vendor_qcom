
#ifndef _QVP_RTP_API_H_
#define _QVP_RTP_API_H_
/*=*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                         QVP_RTP_API . H

GENERAL DESCRIPTION

  This file contains the APIS and call backs which are required for the
  application to access RTP functionality.

EXTERNALIZED FUNCTIONS

    qvp_rtp_init()
        Initializes the memory of the RTP layer. Sets the task running
        and be ready for requests.
        Multiple initilizations do not have any effect. Reinitilization
        attempts will be returned with success.

    qvp_rtp_register()
        Creates a context for an application inside the stack. Call backs
        are connected. App will start getting events starting from
        registered call back.

    qvp_rtp_register2()
        Creates a context for an application inside the stack. Call backs
        are connected. Creates a network layer context for the app
        reserving the requested ports. App will start getting events
        starting from registered call back.

    qvp_rtp_open()
        Opens an RTP stream. Status and stream id returned in the call back.

    qvp_rtp_open2()
        Opens and configures ( if requested ) RTP stream. Status and stream
        id returned in the call back.

    qvp_rtp_reset_session()
        Resets and ends the RTP session associated with already opened
        stream. This logically terminates an RTP session. This call retains
        all the resources pertaining to the opened channel. However the
        RTP/RTCP session context is reset. Also RX and TX path is reset.

    qvp_rtp_send()
        Sends an RTP packet to specified channel.


    qvp_rtp_send_dtmf()
        This function asynchronously sends an RTP packet through the stream.
        This packet contains one or  encoded DTMF digit. No response
        primitive is supported.

    qvp_rtp_close()
        Closes an RTP stream. Status and stream id returned in the call back.

    qvp_rtp_enable_lipsync()
        Enable lipsync for an audio and video stream. Status is returned in
        the call back.

    qvp_rtp_disable_lipsync()
        Disables lip sync b/w an audio and video streams.

    qvp_rtp_validate_config()
        Validates a standalone payload configuration provided by application.
        App will do a RTP configure only if this succeeds

    qvp_rtp_validate_stream_config()
        Validates the payload configuration provided by application
        with reference to the current stream

    qvp_rtp_configure()
        Reconfigures an already existing stream. The stream parameters are
        updated to the new one.


    qvp_rtp_read_default_config()
        Reads the default configuration for a particular payload format.
        The all will return both TX and RX configuration.

    qvp_rtp_read_config()
        Reads the presemt configuration of a stream previously opened by the
        application. This is an  synchronous call.
        CAUTION : Application will get unpredictable results if it initiated
        a configure request and it is waiting for response. This is read is
        unprotected.

    qvp_rtp_pull_pkt()

        Pulls a data packet from the jitter buffer associated with an
        i/b stream. If the stream is a purely o/b stream the implementation
        this will return an error.


    qvp_rtp_read_q_info()

        Reads the buffer occupancy and statistics inside RTP. This is done
        on a per session basis. Application uses this information to do
        source rate control. This is mainly applicable for Video Source rate
        control.



    qvp_rtp_shut_down()
        Shuts down the QVP_RTP layer.


CALL FLOW FOR THIS MODULE

                  1) MODULE INITIALIZATION


                  2) APPLICATION REGISTRATION


                APP                                       RTP
                  |                                         |
                  |   qvp_rtp_regster()                     |
                  |---------------------------------------->|
                  |                                         |
                  |                                         |
                  |   register_cb()                         |
                  |<----------------------------------------|
                  |                                         |
                  |                                         |

                                    OR

                APP                                       RTP
                  |                                         |
                  |   qvp_rtp_regster2()                    |
                  |---------------------------------------->|
                  |                                         |
                  |                                         |
                  |   register2_cb()                         |
                  |<----------------------------------------|
                  |                                         |
                  |                                         |

                  3) CHANNEL OPEN

                APP                                       RTP
                  |                                         |
                  |   qvp_rtp_open()                        |
                  |---------------------------------------->|
                  |                                         |
                  |                                         |
                  |   open_cb()                             |
                  |<----------------------------------------|
                  |                                         |
                  |                                         |


                  3.1) READ DEFAULT CONFIGURATION OF A PARTICULAR PROFILE

                APP                                       RTP
                  |                                         |
                  |   qvp_rtp_read_default_config()         |
                  |---------------------------------------->|
                  |   (synchronous return)                  |
                  |                                         |
                  |                                         |

                  3.2) READ CONFIGURATION OF A PARTICULAR PROFILE

                APP                                       RTP
                  |                                         |
                  |   qvp_rtp_read_config()                 |
                  |---------------------------------------->|
                  |   (synchronous return)                  |
                  |                                         |
                  |                                         |

                  3.3) VALIDATE CONFIGURATION OF A PARTICULAR PROFILE

                APP                                       RTP
                  |                                         |
                  |   qvp_rtp_validate_config()             |
                  |---------------------------------------->|
                  |   (synchronous return)                  |
                  |                                         |
                  |                                         |

                  3.4) SET CONFIGURATION FOR A PARTICULAR CHANNEL

                APP                                       RTP
                  |                                         |
                  |   qvp_rtp_configure()                   |
                  |---------------------------------------->|
                  |                                         |
                  |                                         |
                  |<----------------------------------------|
                  |      config_cb()                        |

                                    OR

                  3) CHANNEL OPEN AND CONFIGURE

                APP                                       RTP
                  |                                         |
                  |   qvp_rtp_open2()                       |
                  |---------------------------------------->|
                  |                                         |
                  |                                         |
                  |   open_cb()                             |
                  |<----------------------------------------|
                  |                                         |
                  |                                         |

                  4) VALIDATE NEW CONFIGURATION FOR A CONFIGURED STREAM

                APP                                       RTP
                  |                                         |
                  |   qvp_rtp_validate_stream_config()      |
                  |---------------------------------------->|
                  |   (synchronous return)                  |
                  |                                         |
                  |                                         |

                  5) SET CONFIGURATION FOR A PARTICULAR CHANNEL

                APP                                       RTP
                  |                                         |
                  |   qvp_rtp_configure()                   |
                  |---------------------------------------->|
                  |                                         |
                  |                                         |
                  |<----------------------------------------|
                  |      config_cb()                        |

                  6) DATA TRAFFIC INWARD

                APP                                       RTP
                  |                                         |
                  |                                         |
                  |   packet_cb()                           |
                  |<----------------------------------------|
                  |                                         |
                  |                                         |

                  7) FLOW CONTRTOL MODEL

                APP                                       RTP
                  |                                         |
                  |   qvp_rtp_send_pkt()                    |
                  |---------------------------------------->|
                  |---------------------------------------->|
                  |---------------------------------------->|
                  | qvp_rtp_send_dtmf()(only for dtmf pyld) |
                  |---------------------------------------->|
                  |---------------------------------------->|
                  |---------------------------------------->|

                  |                                         |
                  |                                         |
                  |   flow_cb()                             |
                  |<----------------------------------------|
                  |                                         |
                  |                                         |


                  8) ENABLE LIPSYNC

                APP                                       RTP
                  |                                         |
                  |   qvp_rtp_lipsync()                     |
                  |---------------------------------------->|
                  |                                         |
                  |                                         |
                  |   lipsync_cb()                          |
                  |<----------------------------------------|
                  |                                         |
                  |                                         |

                  9) DISABLE LIPSYNC

                APP                                       RTP
                  |                                         |
                  |   qvp_rtp_lip_unsync()                  |
                  |---------------------------------------->|
                  |                                         |
                  |                                         |
                  |   lip_unsync_cb()                       |
                  |<----------------------------------------|
                  |                                         |
                  |                                         |

                  10) PULL DATA FOR CONFIGURATION WITH JITTER BUFFER

                APP                                       RTP
                  |                                         |
                  |   qvp_rtp_pull_pkt()                  |
                  |---------------------------------------->|
                  |   (synchronous return)                  |
                  |                                         |
                  |                                         |
                  |                                         |

                  11) READ THE QUEUE OCCUPANCY AND STATISTICS FROM RTP
                      CONTEXT.

                APP                                       RTP
                  |                                         |
                  |       qvp_rtp_read_q_info()             |
                  |---------------------------------------->|
                  |   (synchronous return)                  |
                  |                                         |
                  |                                         |

                  12) RESET THE RTP SESSION CONTEXT TO REUSE SAME
                      CHANNEL ACROSS MULTIPLE CALLS.

                APP                                       RTP
                  |                                         |
                  |  qvp_rtp_reset_session()                |
                  |---------------------------------------->|
                  |                                         |
                  |<----------------------------------------|
                  |      reset_cb()                         |
                  |                                         |


INITIALIZATION AND SEQUENCING REQUIREMENTS

    Any other API will not function if initialization through function
    qvp_rtp_init is not done.


  Copyright (c) 2004 by Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary.  Export of this technology or software is
  regulated by the U.S. Government. Diversion contrary to U.S. law prohibited.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

$Header: //source/qcom/qct/multimedia/videophone/ps_media/RTP/RTP_core/rel/1.0/inc/qvp_rtp_api.h#2 $ $DateTime: 2008/08/12 02:55:08 $ $Author: gurneenk $

                            EDIT HISTORY FOR FILE


when        who    what, where, why
--------    ---    ----------------------------------------------------------
14/07/08    grk    Added codec mode request (CMR) support for AMR.
11/01/08    grk    Fix for CR 134629-fixes bugs in DTMF payload packing.
11/14/07    apr    Adding adaptive_jb flag
01/04/07    apr    Support for time warping
12/31/07    apr    Rate adaptation feature rework
11/07/07    grk    Support for In-call DTMF.
10/30/07    apr    Support for rate adaptation using APTO ARR
10/19/07    grk    Support for in-call DTMF as per RFC 4733.
06/11/07    mc     Changed data type of mac_q in qvp_rtp_data_q_info_type
                   to uint32 from uint16 to be in sycn with DS.
02/28/07    grk    Moved Qos and shared handles to qvp_rtp_pyld_qos_type.
01/08/07    uma    Changed codec_hdr from pointer to a byte array
11/06/06    uma    Added support for AVPF profile - generic NAK , RFC 4585
10/31/06    grk    Addition of qvp_rtp_open2() API.
10/31/06    grk    Addition of QOS and shared handles.
10/31/06    grk    Modification to pass appid in all callbacks.
10/25/06    uma    Fix for socket2 - obtaining iface through rtp open
                   Removing sock interface & having only sock2
10/20/06    uma    Support for socket2 interface FEATURE_QVP_RTP_SOCK2
09/21/06    apr    Addition of app_data in register2 API and register2
                   callback.
08/30/06    grk    Added outbound local port for RTCP in
                   qvp_rtp_stream_params_type
08/30/06    grk    Added support for port reservation.
07/13/06    uma    Added new APIs qvp_rtp_validate_config,
                   qvp_rtp_validate_stream_config for
                   validating payload configurations
07/13/06    uma    Moved qvp_rtp_channel_type to qvp_rtp_payload.h
05/05/06    apr    Included RTP code in feature FEATURE_QVPHONE_RTP
04/24/06    uma    Reduced QVP_RTP_MAX_STREAMS_DFLT for RTP optimization
02/22/06    apr    Removed flow_cb callback function since this is not
                   used in the present code and all local RTP statistics
                   are merged along handle_rtcp_info callback now
02/14/06    apr    Added new callback function to propogation of RTCP
                   reports to application, added the same in the RTP
                   callback list
02/14/06    apr    Added RTCP packet structures
03/20/06    uma    Included qvp_rtp_stubs.h for wrappers
                   qvp_rtp_malloc & qvp_rtp_free
02/28/06    uma    Added end bit to qvp_rtp_dtmf_digit_type
02/27/06    uma    Added rtp_pyld_type to qvp_rtp_send_pkt_type and
                   qvp_rtp_send_dtmf_pkt_type to send pyld in each RTP packet
02/08/06    uma    Modified qvp_rtp_stream_params_type to remove
                   duplicate rtcp transmit paramaeters
02/06/06    apr    Name change of fqi to amr_fqi in qvp_rtp_aud_frm_info_type
01/03/06    srk    Added a new member for holding frame info in the
                   structure qvp_rtp_recv_pkt_type
01/30/06    srk    Added new structures to hold audio/video frame info
                   to be used by the application for rendering
11/18/05    apr    Added a parameter to RTP open callback function
                   for network interface app id.
11/08/05    srk    Added DTMF payload format support. (rfc2833 ).
09/04/05    srk    Added support to reset a stream and  allowing
                   applications to specify the ob local port too.
08/01/05    srk    Added qvp_rtp_read_q_info to read the queue occuppancy
                   and statistics from RTP stack. Added a data type to
                   support this.
07/26/05    srk    Added separate params in open for RTCP config
07/21/05    srk    Adding rtcp configuration and qos configuration in
                   stream parameters.
07/01/05    srk    Adding not supported status code
04/15/05    srk    Added the wall clock synchronization and port not
                   available status code
03/24/05    srk    Added interface handle to the stream param.
02/14/05    srk    Added configure API to configure individual streams.
02/01/05    srk    Added pull model for jitter buffer enabled streams
10/21/04    srk    Review comments from Ramesh, Santosh, Ravi.
10/18/04    srk    Initial Creation.
===========================================================================*/
//#include "customer.h"
#ifdef FEATURE_QVPHONE_RTP

#include "qvp_rtp_payload.h"
//#include "qw.h"             /* for qword data type */
//#include "clk.h"
//#include "qvp_rtp_stubs.h"


/*--------------------------------------------------------------------------
                GENERAL CONSTANTS FOR THE MODULE
--------------------------------------------------------------------------*/
#define QVP_RTP_IP_STR_LEN         16  /* aaa.bbb.ccc.ddd = 4 * 3 + 3 + 1 */
#define QVP_RTCP_CNAME_LEN         256 /* RFC3550 section 6.5.1 CNAME: */
#define QVP_RTP_MAX_DTMF_BURST_SZ  15  /* maximum len of a DTMF burst */
#define QVP_RTP_CODEC_HDR_LEN      32  /* len of codec hdr for mp4,h263 */
/*--------------------------------------------------------------------------
    Default configuration values to be used for the module. In case there
    is no explicit configuration defined by the initializing entity.
--------------------------------------------------------------------------*/
#define QVP_RTP_MAX_USERS_DFLT           2 /* number of applications
                                            * that can be registered and
                                            * connected to this layer.
                                            */
#define QVP_RTP_MAX_STREAMS_DFLT         4 /* number of streams per user
                                            * which can be accessed
                                            */
#define QVP_RTP_MAX_PORTS  ( 2 * QVP_RTP_MAX_STREAMS_DFLT )/*Maximum num of
                                                          *ports supported
                                                          *by RTP streams
                                                          *can be i/b or
                                                          *o/b or duplex.
                                                          *For duplex
                                                          *streams having
                                                          * separate ports
                                                          *for i/b and o/b
                                                          *max ports
                                                          *required will be
                                                          *two ports per
                                                          *stream
                                                          */

#define QVP_RTP_MAX_RESV_PORTS          QVP_RTP_MAX_PORTS/*Maximum number of
                                                          *ports reserved by
                                                          *the application.
                                                          *This value can be
                                                          *reduced if
                                                          *application
                                                          *decides to
                                                          *reserve only less
                                                          *number of ports
                                                          *and allocate the
                                                          *rest dynamically
                                                          *without
                                                          *reservation
                                                          */

/*-------------------------------------------------------------------------
     Max number of ports that DS should be able to support for RTP.
     At RTP, if none of the ports asked for, match with the reserved ports,
     then new ports in addition to reserved ports will be allocated.
-------------------------------------------------------------------------*/
#define QVP_RTP_MAX_DS_USER_PORTS \
            (QVP_RTP_MAX_RESV_PORTS + QVP_RTP_MAX_PORTS)

/*-------------------------------------------------------------------------
  Maximum number of lost seq numbers that can be reported
  in NAK feedback packets within the reporting interval

  Reporting interval is between Min delay to Min delay+avg efb reporting
  interval

  i.e 300 ms to 300+500ms in our case i.e 120 events in 800ms
  For regular mode reporting interval is rtcp report interval 5s
  it will be 120 events in 5s

-------------------------------------------------------------------------*/

#define QVP_RTCP_MAX_NAK_PKT  20

#define QVP_RTCP_APP_PKT_NAME_LEN 4

/*===========================================================================
                    DATA TYPES FOR THE MODULE ACCESS
===========================================================================*/

/*--------------------------------------------------------------------------
    TYPEDEF STRUCT QVP_RTP_CONFIG_TYPE

    Stores the configuration of the RTP module. Application can use a
    pointer to this data type to initilaize the module.
--------------------------------------------------------------------------*/
typedef struct
{
  uint8      max_users;              /* maxumum number of users which
                                      * use this module
                                      */
  uint8      max_streams;            /* maximum number of streams which
                                      * can be opened simultaneously
                                      * should be set  = max_users * max
                                      * channels per user
                                      */
} qvp_rtp_config_type;


/*--------------------------------------------------------------------------
    TYPEDEF ENUM QVP_RTP_STATUS_TYPE

    Enumeration which lists the return values from this module.

--------------------------------------------------------------------------*/
typedef enum
{
  QVP_RTP_SUCCESS,      /* the operation requested was success */
  QVP_RTP_NORESOURCES,  /* the operation failed because there was
                         * no resources
                         */
  QVP_RTP_WRONG_PARAM,  /* the operation failed because there was
                         * at least one unacceptable parameter
                         */
  QVP_RTP_ERR_FATAL,     /*  Unknown but FATAL error */

  QVP_RTP_PORT_NOTAVAILABLE, /* unable to bind to a particular i/b port */
  QVP_RTP_NOT_SUPPORTED   /* the feature or primitive is not supported at
                           * at this time
                           */

} qvp_rtp_status_type;



/*--------------------------------------------------------------------------
  TYPEDEF VOID QVP_RTP_STREAM_ID

  The unique opaque handler to denote an RTP stream opened by the
  application. This is unique within an app context but not guaranteed
  to be unique across apps.
--------------------------------------------------------------------------*/
typedef void *qvp_rtp_stream_id_type;

/*--------------------------------------------------------------------------
  TYPEDEF VOID QVP_RTP_APP_ID

  The unique opaque handler to denote a registered RTP application.
--------------------------------------------------------------------------*/
typedef void *qvp_rtp_app_id_type;

/*--------------------------------------------------------------------------
  TYPEDEF VOID QVP_RTP_APP_DATA_TYPE

  The unique opaque handler to denote a context app supplied as part of
  a request.
--------------------------------------------------------------------------*/
typedef void *qvp_rtp_app_data_type;

/*--------------------------------------------------------------------------
  TYPEDEF VOID *QVP_RTP_IF_HDL_TYPE

  The unique opaque handle to the underlying interface. Most RTP code
  is agnostic to this information except for the Lower layer which
  glues RTP with the platform sockets.
--------------------------------------------------------------------------*/
typedef uint32 qvp_rtp_if_hdl_type;

/*--------------------------------------------------------------------------
  TYPEDEF STRUCT QVP_RTP_AMR_INFO_TYPE

  Structure for AMR specific frame info.
--------------------------------------------------------------------------*/
typedef struct
{
  uint8        amr_mode_request;  /*AMR mode of operation*/
  boolean      amr_fqi;           /* Quality of the frame received. This is
                                  * currently known to apply for AMR only
                                  */
} qvp_rtp_amr_info_type;

/*--------------------------------------------------------------------------
  TYPEDEF STRUCT QVP_RTP_PROFILE_INFO_TYPE

  Union for Codec/profile specific fields.
--------------------------------------------------------------------------*/
typedef union
{

  qvp_rtp_amr_info_type    amr_info;

} qvp_rtp_profile_info_type;

/*--------------------------------------------------------------------------
  TYPEDEF STRUCT QVP_RTP_AUD_FRM_INFO_TYPE

    This structure will contain the information required by the application
    for effectively rendering an audio frame. These information may need
    further expansion as we support more payload formats and the kind of in
    formation may vary from payload format to payload format.
--------------------------------------------------------------------------*/
typedef struct
{
  boolean  frm_present;      /* When TRUE Current frame is not locally
                              * generated NO_DATA or SID but is came from
                              * the network
                              */
  uint16   tw_factor;        /* time warping factor calculated by QDJ */
  uint16   phase_offset;     /* phase offset for phase matching */
  uint16   run_length;       /* run length for phase matching */
  qvp_rtp_profile_info_type  profile_info; /* profile specific info */

} qvp_rtp_aud_frm_info_type;

/*--------------------------------------------------------------------------
  TYPEDEF STRUCT QVP_RTP_VID_FRM_INFO_TYPE

    This structure will contain the information required by the application
    for efectively rendering a video frame.

    Currently this information is not populated.
--------------------------------------------------------------------------*/
typedef struct
{

  uint8   not_used;           /* place holder for future change */

} qvp_rtp_vid_frm_info_type;

/*--------------------------------------------------------------------------
  TYPEDEF STRUCT QVP_RTP_FRM_INFO_TYPE

    This structure will contain the information required by the application
    for effectively rendering a frame. This can be audio or video.

--------------------------------------------------------------------------*/
typedef struct
{
  /*------------------------------------------------------------------------
    Union which will collapse audio and video frm info
  ------------------------------------------------------------------------*/
  union
  {
    qvp_rtp_aud_frm_info_type aud_info;  /* info relating to audio */
    qvp_rtp_vid_frm_info_type vid_info;  /* info relating to video */

  } info;

} qvp_rtp_frm_info_type_type;

/*--------------------------------------------------------------------------
  TYPEDEF STRUCT  QVP_RTP_PKT_TYPE

  Data type which abstracts the RTP data incoming

--------------------------------------------------------------------------*/
typedef struct
{
  uint8             *data;          /* actual RTP payload for the stream */
  uint16            len;            /* the actual length of the buffer  */
  uint32            tstamp;         /* the RTP time stamp received      */
  uint32            seq;            /* the sequence no of the RTP packet */
  boolean           marker_bit;     /* Is this a frame which marks an
                                     * important event ..?
                                     * eg : refresh_frame in video
                                     *    : silence on/off in audio.
                                     */
  boolean           partial_pull;   /* this flag will be true if the pull
                                     * was partial. This will not be set
                                     * and checked for push model.
                                     * Only needed for Pull Pkt APIs
                                     */
  qvp_rtp_frm_info_type_type frm_info; /* frame info useful for rendering */
} qvp_rtp_recv_pkt_type;

/*--------------------------------------------------------------------------
    TYPEDEF ENUM QVP_RTP_FLOW_LEVEL

    The kind of flow control actions the stack advices the applicaion
    to do.
--------------------------------------------------------------------------*/
typedef enum
{
  QVP_RTP_STOP_FLOW,          /* stop the flow level serious local
                               * congestion
                               */
  QVP_RTP_DECREASE_FLOW,      /* decrease the flow level */
  QVP_RTP_INCREASE_FLOW,      /* increase the flow level */

} qvp_rtp_flow_level;

/*--------------------------------------------------------------------------
  TYPEDEF STRUCT  QVP_RTCP_LOCAL_PARAM_TYPE

  Data type which abstracts the RTCP local statistics of a media stream

--------------------------------------------------------------------------*/
typedef struct
{

  uint32 ssrc;               /* SSRC of the media stream       */
  struct qvp_rtcp_sender_stats
  {
    boolean valid;
    uint16  tx_pkt_cnt;      /* Total no of packets transmitted */
    uint16  tx_octet_cnt;    /* Total no of octets transmitted  */

  }sender_stats;

  struct qvp_rtcp_recv_stats
  {
    boolean valid;
    uint16  rx_pkt_cnt;      /* Total no of packets received */
    uint16  rx_octet_cnt;    /* Total no of octets received  */
  }recv_stats;

} qvp_rtcp_local_param_type;



/*--------------------------------------------------------------------------
  TYPEDEF ENUM QVP_RTCP_PKT_TYPE

   The Enumeration which will denote various packet types used by  RTCP.
--------------------------------------------------------------------------*/
typedef enum
{

   QVP_RTCP_SR   = 200,               /* sender report            */
   QVP_RTCP_RR   = 201,               /* receiver report          */
   QVP_RTCP_SDES = 202,               /* source description items */
   QVP_RTCP_BYE  = 203,               /* end of participation     */
   QVP_RTCP_APP  = 204,               /* application specific     */
   QVP_RTCP_RTPFB  = 205,             /* Transport layer feedback message */
   QVP_RTCP_PSFB  = 206               /* Payload specific feedback message */

} qvp_rtcp_pkt_type;

/*--------------------------------------------------------------------------
  TYPEDEF STRUCT QVP_RTCP_SR_TYPE

  Structure which will denote the fields in a SR. This can be passed to
  application.
--------------------------------------------------------------------------*/
typedef struct
{
   boolean   valid;          /* flag to denote this is valid */
   uint32    ssrc;           /* src id from the sender */
   qword     ntp;            /* network timestamp    */
   uint32    tstamp;         /* streams time stamp when the time stamp
                              *  ntp was taken
                              */

   uint32    num_pkts;       /* nuymber of packets transmitted */
   uint32    nBytes;         /* number of bytes transmitted */

} qvp_rtcp_sr_type;

/*--------------------------------------------------------------------------
  TYPEDEF STRUCT QVP_RTCP_RR_TYPE

  Structure which will denote the fields in a SR. This can be passed to
  application.
         |                     SSRC of packet sender                     |
         +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
  report |                 SSRC_1 (SSRC of first source)                 |
  block  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    1    | fraction lost |       cumulative number of packets lost       |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         |           extended highest sequence number received           |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         |                      interarrival jitter                      |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         |                         last SR (LSR)                         |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         |                   delay since last SR (DLSR)                  |
--------------------------------------------------------------------------*/
typedef struct
{

   boolean   valid;          /* flag to denote this is valid */
   uint32    ssrc;           /* src id from the sender */
   uint32    s_ssrc;         /* src id of the source */
   uint8     frac_lost;      /* fraction lost */
   uint32    tot_lost;       /* total lost since last SR */
   uint32    ext_seq;        /* extended highest seq */
   uint32    est_jitter;     /* estimated jitter */
   uint32    lsr;            /* last SR tstamp */
   uint32    dslr;           /* delay since last SR */

} qvp_rtcp_rr_type;

/*--------------------------------------------------------------------------
  TYPEDEF STRUCT QVP_RTCP_LOST_SEQ_LIST

    FCI information for generic Nak consists of pid + blp

    pid: sequence number of a lost packet
    blp :bitmask of  losses of any of the 16 RTP packets
    immediately following the RTP packet indicated by the PID

    BLP's LSbit  is considered as bit 1,
    and MSbit as bit 16, then bit i of the bit mask is set to 1 if the rxr
    has not received RTP packet number (PID+i) (modulo 2^16)

    Eg:
    pid1 = 11 , blp1 = 1001 0000 1000 0100 b = 0x9084
    pid2 = 38, blp2 = 0100 0000 0000 0000 = 0x4000
    lost seq numbers detected are 11, 12, 15, 20, 25, 38, 40

    FCI information for generic Nak consists of pid + blp
    The lost sequence numbers are extracted from pid & blp and sent to app

---------------------------------------------------------------------------*/

typedef struct
{
  uint8 num_seq;
  uint16 lost_seq [ QVP_RTCP_MAX_NAK_PKT ]; /* seq numbers of lost pkts */

}qvp_rtcp_lost_seq_list;

/*--------------------------------------------------------------------------
  TYPEDEF STRUCT QVP_RTCP_FB_TYPE

  Structure which will denote the fields in a feedback.
  This is the common format for all types of feedback message
  FCI- feedback control information has information specific to the pkt type
        & feedback message type


   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                  SSRC of packet sender                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                  SSRC of media source                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   :            Feedback Control Information (FCI)                 :
   :                                                               :
--------------------------------------------------------------------------*/

typedef struct
{

   boolean   valid;        /* flag to denote this is valid */
   uint8     fmt;          /* feedback message type of packet */
   uint32    sender_ssrc;  /* src id from the sender */
   uint32    source_ssrc;  /* src id of the source */
   union
   {
       qvp_rtcp_lost_seq_list gnak_list;
   }fci;

} qvp_rtcp_fb_type;


/*--------------------------------------------------------------------------
    TYPEDEF ENUM  QVP_RTCP_APP_PING_PKT_TYPE

  This is the list of app packets we are planning to support.
--------------------------------------------------------------------------*/
typedef enum
{

  QVP_RTCP_APP_PING_REQ  = 1,
  QVP_RTCP_APP_PING_RES  = 2,

} qvp_rtcp_app_ping_pkt_type;

/*--------------------------------------------------------------------------
    TYPEDEF ENUM  QVP_RTCP_PSVT_APP_PKT_TYPE

  This is the list of app packets we are planning to support for PSVT
  app
--------------------------------------------------------------------------*/
typedef enum
{
  QVP_RTCP_APP_UNK = 0,
  QVP_RTCP_APP_APTO_ARR  = 1,
  QVP_RTCP_APP_PING  = 2

} qvp_rtcp_app_pkt_type;

/*--------------------------------------------------------------------------
    TYPEDEF ENUM  QVP_RTCP_APP_PING_TYPE

    Data which abstracts the ping
--------------------------------------------------------------------------*/
typedef struct
{
  qvp_rtcp_app_ping_pkt_type ptype;

  uint32  ssrc;
  uint32  seq;
  boolean data_match;


} qvp_rtcp_app_ping_type;

/*--------------------------------------------------------------------------
  TYPEDEF STRUCT QVP_RTCP_APP_APTO_ARR_TYPE

  This struct represents an APTO-ARR feedback packet
--------------------------------------------------------------------------*/
typedef struct
{
  uint32                     ssrc;
  int16                      apto;     /* arrival to playout offset */
  uint16                     arr;      /* average receive rate */

} qvp_rtcp_app_apto_arr_type;

/*--------------------------------------------------------------------------
  TYPEDEF STRUCT QVP_RTCP_APP_DATA_TYPE

  Structure which will denote the fields in a SR. This can be passed to
  application.
--------------------------------------------------------------------------*/
typedef struct
{
   boolean               valid;          /* flag to denote this is valid */
   qvp_rtcp_app_pkt_type ptype;     /* app pkt type */
   uint8                 name[QVP_RTCP_APP_PKT_NAME_LEN];

   union
   {
     qvp_rtcp_app_ping_type     ping;         /* the app defined ping */
     qvp_rtcp_app_apto_arr_type apto_arr;     /* APTO-ARR msg */
   } app_data;

} qvp_rtcp_app_data_type;

/*--------------------------------------------------------------------------

  TYPEDEF STRUCT QVP_RTCP_FB_FMT_TYPE

  Format Types of RTCP Transport layer feedback packets
   0:    unassigned
   1:    Generic NACK
   2-30: unassigned
   31:   reserved for future expansion of the identifier number space

--------------------------------------------------------------------------*/
typedef enum
{
  QVP_RTCP_TRANS_FB_UNK = 0,
  QVP_RTCP_TRANS_FB_GNACK = 1, /* only generic ack is supported for now */
  QVP_RTCP_TRANS_FB_RES =2,

} qvp_rtcp_transport_fmt_type;

/*--------------------------------------------------------------------------

  TYPEDEF ENUM QVP_RTCP_PYLD_FB_FMT_TYPE

       0:     unassigned
      1:     Picture Loss Indication (PLI)
      2:     Slice Loss Indication (SLI)
      3:     Reference Picture Selection Indication (RPSI)
      4-14:  unassigned
      15:    Application layer FB (AFB) message
      16-30: unassigned
      31:    reserved for future expansion of the sequence number space

This data type is not used as of now
--------------------------------------------------------------------------*/
typedef enum
{
  QVP_RTCP_PYLD_FB_UNK = 0,
  QVP_RTCP_PYLD_FB_PLI  =1,
  QVP_RTCP_PYLD_FB_SLI = 2,
  QVP_RTCP_PYLD_FB_RPSI = 3,

  /*application layer feedback - special case of payload specific feedback*/
  QVP_RTCP_PYLD_APP_FB = 15,
  QVP_RTCP_PYLD_FB_RESV

} qvp_rtcp_pyld_fmt_type; /* not used as of now */


/*--------------------------------------------------------------------------
  TYPEDEF STRUCT QVP_RTCP_SESSION_INFO_TYPE


  This is collapsed container for all possible RTCP packets and will be
  passed to application depending on the packet content.

  Info passed on each packet will be noted in the inf_type
--------------------------------------------------------------------------*/
typedef struct
{

  qvp_rtcp_pkt_type  inf_type;


  /*------------------------------------------------------------------------
    In RTCP we can get combined information about a session in one RTCP
    packet. The best way to represent it will be to have oner big report
    which unionize the whole thing
  ------------------------------------------------------------------------*/
  union
  {

    qvp_rtcp_sr_type        sr;       /* sender report */
    qvp_rtcp_rr_type        rr;       /* receiver report */
    qvp_rtcp_app_data_type  app_data; /* app_data */
    qvp_rtcp_fb_type        tp_fb;    /* transport layer  feedback message*/
    qvp_rtcp_fb_type        pyd_fb;   /* payload specific feedback message*/
  } info;


} qvp_rtcp_session_info_type;

/*--------------------------------------------------------------------------
  TYPEDEF  VOID ( *QVP_RTP_STREAM_REGISTER_CB_TYPE  )

  This call back is called as an indication primitve to the call
  qvp_rtp_regiuster().


  qvp_rtp_status_type  - status of the register operation
  app_id               - unique id assigned to the app.

  The app id is zero in case of failure.

  CAUTION : APPLICATION SHOULD NOT BLOCK ON THIS CALL BACK. IDEALLY
  IT SHOULD BE POSTING A MESSAGE TO ITS OWN CONTEXT.
--------------------------------------------------------------------------*/
typedef  void ( *qvp_rtp_register_cb_type)
(
  qvp_rtp_status_type      status,
  qvp_rtp_app_id_type      app_id
);

/*--------------------------------------------------------------------------
  TYPEDEF  VOID ( *QVP_RTP_STREAM_REGISTER2_CB_TYPE  )

  This call back is called as an indication primitve to the call
  qvp_rtp_regiuster().


  qvp_rtp_status_type  - status of the register operation
  app_id               - unique id assigned to the app.
  app_data             - app_data supplied as part of resiter API

  The app id is zero in case of failure.

  CAUTION : APPLICATION SHOULD NOT BLOCK ON THIS CALL BACK. IDEALLY
  IT SHOULD BE POSTING A MESSAGE TO ITS OWN CONTEXT.
--------------------------------------------------------------------------*/
typedef  void ( *qvp_rtp_register2_cb_type)
(
  qvp_rtp_status_type      status,
  qvp_rtp_app_id_type      app_id,
  qvp_rtp_app_data_type    app_data
);

/*--------------------------------------------------------------------------
  TYPEDEF  VOID ( *QVP_RTP_STREAM_OPEN_CB_TYPE )

  This call back is called as an indication primitve to the call
  qvp_rtp_open().

  The status gives the status of the requested operation and the stream_id
  contains a valid stream identifier in case of success.

  app_id               - To identify the application associated.
  qvp_rtp_status_type  - status of the open operation
  stream               - stream id of the channel opened.
  app_data             - app_data supplied as part of stream params.

  CAUTION : APPLICATION SHOULD NOT BLOCK ON THIS CALL BACK. IDEALLY
  IT SHOULD BE POSTING A MESSAGE TO ITS OWN CONTEXT.
--------------------------------------------------------------------------*/
typedef  void ( *qvp_rtp_stream_open_cb_type )
(
  qvp_rtp_app_id_type    app_id,      /* RTP Application ID*/
  qvp_rtp_app_data_type  app_data,    /* app handle into the request */
  qvp_rtp_status_type    status,      /* status of requested operation */
  qvp_rtp_stream_id_type stream      /* identifier of the stream being
                                       * opened
                                       */
);

/*--------------------------------------------------------------------------
  TYPEDEF  VOID ( *QVP_RTP_CONFIG_STREAM_CB_TYPE )

  This call back is called as an indication primitve to the call
  qvp_rtp_config().

  The status gives the status of the requested operation and the stream_id
  contains a valid stream identifier in case of success.

  app_id               - To identify the application associated.
  app_data             - app_data supplied as part of stream params.
  qvp_rtp_status_type  - status of the open operation
  stream               - stream id of the channel opened.

  CAUTION : APPLICATION SHOULD NOT BLOCK ON THIS CALL BACK. IDEALLY
  IT SHOULD BE POSTING A MESSAGE TO ITS OWN CONTEXT.
--------------------------------------------------------------------------*/
typedef  void ( *qvp_rtp_config_stream_cb_type )
(
  qvp_rtp_app_id_type    app_id,      /* RTP Application ID*/
  qvp_rtp_app_data_type  app_data,    /* app handle into the request */
  qvp_rtp_status_type    status,      /* status of requested operation */
  qvp_rtp_stream_id_type stream       /* identifier of the stream being
                                       * opened
                                       */
);

/*--------------------------------------------------------------------------
  TYPEDEF  VOID ( *QVP_RTP_RESET_SESSION_CB_TYPE )

  This call back is called as a response primitve to the call
  qvp_rtp_reset_session().

  The status gives the status of the requested operation and the stream_id
  contains a valid stream identifier in case of success.

  app_id               - To identify the application associated.
  app_data             - app_data supplied as part of stream params.
  qvp_rtp_status_type  - status of the open operation
  stream               - stream id of the channel opened.

  CAUTION : APPLICATION SHOULD NOT BLOCK ON THIS CALL BACK. IDEALLY
  IT SHOULD BE POSTING A MESSAGE TO ITS OWN CONTEXT.
--------------------------------------------------------------------------*/
typedef  void ( *qvp_rtp_reset_session_cb_type )
(
  qvp_rtp_app_id_type    app_id,      /* RTP Application ID*/
  qvp_rtp_app_data_type  app_data,    /* app handle into the request */
  qvp_rtp_status_type    status,      /* status of requested operation */
  qvp_rtp_stream_id_type stream       /* identifier of the stream being
                                       * reset
                                       */
);

/*--------------------------------------------------------------------------
  TYPEDEF  VOID ( *QVP_RTP_STREAM_CLOSE_CB_TYPE )

  This call back is called as an indication primitve to the call
  qvp_rtp_close().

  app_id - To identify the application associated.
  status - The status gives the status of the requested operation
  stream - the stream_id contains a valid stream identifier which
           got just closed.
  app_data - app data supplied while opening the stream


  CAUTION : APPLICATION SHOULD NOT BLOCK ON THIS CALL BACK. IDEALLY
  IT SHOULD BE POSTING A MESSAGE TO ITS OWN CONTEXT.
--------------------------------------------------------------------------*/
typedef  void ( *qvp_rtp_stream_close_cb_type )
(
  qvp_rtp_app_id_type      app_id,    /* RTP Application ID*/
  qvp_rtp_app_data_type    app_data,  /* app handle into the request */
  qvp_rtp_status_type      status,    /* satatus of operation        */
  qvp_rtp_stream_id_type   stream     /* stream which got closed */
);

/*--------------------------------------------------------------------------
  TYPEDEF  VOID ( *QVP_RTP_HANDLE_PKT_CB_TYPE )

  This call back is called as an indication primitve to  indicate
  an arrival of a new RTP packet

  app_id  - To identify the application associated.
  stream  - stream to which the packet is being delivered
  pkt     - actual RTP packet. Buffer will be reused on retrun from this
            function
  app_data - app data supplied while opening the stream

  CAUTION : APPLICATION SHOULD NOT BLOCK ON THIS CALL BACK. IDEALLY
  IT SHOULD BE POSTING A MESSAGE TO ITS OWN CONTEXT.
--------------------------------------------------------------------------*/
typedef  void ( *qvp_rtp_handle_pkt_cb_type )
(
  qvp_rtp_app_id_type      app_id,    /* RTP Application ID*/
  qvp_rtp_app_data_type    app_data,  /* app handle into the request */
  qvp_rtp_stream_id_type   stream,    /* stream id through which the
                                       * packet came
                                       */
  qvp_rtp_recv_pkt_type    *pkt       /* packet which is being delivered
                                       */
);

/*--------------------------------------------------------------------------

  TYPEDEF VOID ( *QVP_RTP_HANDLE_RTCP_CB_TYPE )

  This is used to pass the information RTCP collected using the RTCP packets
  Most RTCP packets will translate into one call to this function. Composite
  packet will trigger multiple calls to this function pointer.

--------------------------------------------------------------------------*/

typedef void ( *qvp_rtp_handle_rtcp_cb_type )
(
  qvp_rtp_app_id_type      app_id,    /* RTP Application ID*/
  qvp_rtp_app_data_type    app_data,  /* app handle into the request */
  qvp_rtp_stream_id_type   stream,    /* stream id to which the
                                       * rtcp info belongs
                                       */
  qvp_rtcp_session_info_type* rtcp_info,
                                      /* RTCP info propogated to app */
  qvp_rtcp_local_param_type*   rtcp_stats
                                      /* RTP Local statistics        */

);

/*--------------------------------------------------------------------------
  TYPEDEF  VOID ( *QVP_RTP_LIPSYNC_CB )

  This call back is called as an indication primitve to  indicate
  the status of the qvp_rtp_lipsyn() call.

  app_id       - To identify the application associated.
  status       - status of operation.
  audio_stream - that was lisynced
  video_stream - that was lisynced
  app_data     - app data supplied while opening the auido stream


  CAUTION : APPLICATION SHOULD NOT BLOCK ON THIS CALL BACK. IDEALLY
  IT SHOULD BE POSTING A MESSAGE TO ITS OWN CONTEXT.
--------------------------------------------------------------------------*/
typedef  void ( *qvp_rtp_lipsync_cb_type )
(
  qvp_rtp_app_id_type        app_id,       /* RTP Application ID*/
  qvp_rtp_app_data_type      app_data,     /* app handle into the request */
  qvp_rtp_status_type        status,       /* status of requested operation
                                            */
  qvp_rtp_stream_id_type     audio_stream, /* audion stream id to which the
                                            * lipsync was requested
                                            */
  qvp_rtp_stream_id_type     video_stream  /* video stream id to which the
                                            * lipsync was requested
                                            */
);

/*--------------------------------------------------------------------------
  TYPEDEF  VOID ( *QVP_RTP_LIP_UNSYNC_CB_TYPE )

  This call back is called as an indication primitve to  indicate
  the status of the qvp_rtp_lip_unsync() call.

  app_id       - To identify the application associated.
  app_data     - app data supplied while opening the auido stream
  status       - status of operation.
  audio_stream - that was unlinked
  video_stream - that was unlinked

  CAUTION : APPLICATION SHOULD NOT BLOCK ON THIS CALL BACK. IDEALLY
  IT SHOULD BE POSTING A MESSAGE TO ITS OWN CONTEXT.
--------------------------------------------------------------------------*/
typedef  void ( *qvp_rtp_lip_unsync_cb_type )
(
  qvp_rtp_app_id_type        app_id,      /* RTP Application ID*/
  qvp_rtp_app_data_type      app_data,    /* app handle into the request */

  qvp_rtp_status_type        status,      /* status of requested operation
                                           */
  qvp_rtp_stream_id_type     audio_stream,/* audio stream id to which the
                                           * lipunsync was requested
                                           */
  qvp_rtp_stream_id_type     video_stream /* video stream id to which the
                                           * lipunsync was requested
                                           */
);

/*--------------------------------------------------------------------------
  TYPEDEF STRUCT QVP_RTP_APP_CALL_BACKS_TYPE

  Registration structure which will let applications stuff in their
  call back functions. If an application is not interested in the event
  please stuff in NULL there. Although it is encouraged to listen
  to maximum of number of events.
--------------------------------------------------------------------------*/
typedef struct
{
  qvp_rtp_register_cb_type         register_cb;   /*
                                                   * called to indicate
                                                   * registration is
                                                   * complete
                                                   */
  qvp_rtp_register2_cb_type        register2_cb;  /*
                                                   * called to indicate
                                                   * registration ( with
                                                   * port reservation ) is
                                                   * complete
                                                   */
  qvp_rtp_stream_open_cb_type      open_cb;       /* called when a stream
                                                   * is opend
                                                   */
  qvp_rtp_stream_close_cb_type     close_cb;      /* called to indicate
                                                   * the status of close
                                                   * call to a stream
                                                   */
  qvp_rtp_config_stream_cb_type    config_cb;     /* call back function
                                                   * invoked in response
                                                   * application call to
                                                   * configure a stream
                                                   * parameter
                                                   */

  qvp_rtp_handle_pkt_cb_type       packet_cb;     /* called when a pcket
                                                   * is ready for a
                                                   * particular stream
                                                   */
  qvp_rtp_lipsync_cb_type          lipsync_cb;    /* call back to indicate
                                                   * lipsync status
                                                   */
  qvp_rtp_lip_unsync_cb_type       lip_unsync_cb; /* lypsync disable
                                                   * call back
                                                   */
  qvp_rtp_reset_session_cb_type    reset_cb;      /* response to the call
                                                   * reset_session()
                                                   */
  qvp_rtp_handle_rtcp_cb_type      handle_rtcp_cb;/* call back to notify
                                                   * rtcp reports
                                                   */

} qvp_rtp_app_call_backs_type;


/*--------------------------------------------------------------------------
  TYPEDEF STRUCT QVP_RTP_CHANNEL_PARAMS_TYPE

  describes the properties of the channel parameters reqested. used with
  open api.


--------------------------------------------------------------------------*/
typedef struct
{
  qvp_rtp_if_hdl_type   iface_hdl;    /* interface handle ...
                                       * opaque for most RTP except
                                       * for the  NW Abstraction
                                       * layer.
                                       */
  qvp_rtp_channel_type  channel_type; /* bi/sigle direction open   */
  qvp_rtp_payload_type  payload_type; /* pay load format requested */
  uint16                jitter_size;  /* jitter buffer size requested
                                       * in mllisseconds
                                       */
  uint8                 cname[QVP_RTCP_CNAME_LEN]; /* name to be used for
                                                    * the stream
                                                    */

  uint16                local_port;   /* local port which should be
                                       * opened. Valid only for i/b
                                       * component
                                       */
  uint16                ob_lcl_port;  /* local port to be bound for
                                       * outbound channel. App can
                                       * set this to zero if it does not
                                       * care about the port from which
                                       * data needs to be sent. Typically
                                       * this is an ephemeral port
                                       */
  uint8                 remote_ip[QVP_RTP_IP_STR_LEN]; /* remote ip
                                                        * address
                                                        * valid only if
                                                        * channel is o/b or
                                                        * bi directional
                                                        * DOTTED DECIMAL
                                                        * FORMAT
                                                        */
  uint16                remote_port;  /* remote port . valid only for
                                       * o/b or bi directional channel
                                       */
  uint16                rtcp_lcal_prt;/* local port for getting
                                       * RTCP reports. Set zero if not
                                       * interested
                                       */
  uint16                rtcp_ob_lcal_prt;/* local port for sending
                                          * RTCP reports.
                                          */
  qvp_rtp_rtcp_config_type rtcp_tx_config; /* rtcp transmit configurations
                                            * for this channel. Rx port is
                                            * captured in the top level struc
                                            * (as it is not configurable
                                            * hereafter using RTP config )
                                        */
  qvp_rtp_pyld_qos_type qos;          /* qos settings for this channel */
  /* data for application use */
  qvp_rtp_app_data_type  app_data;    /* application handle which will be
                                       * refered in all the call backs
                                       * related to the stream
                                       */

} qvp_rtp_stream_params_type;

/*--------------------------------------------------------------------------
  TYPEDEF STRUCT QVP_RTP_CHANNEL_PARAMS_TYPE

  describes the properties of the channel parameters reqested. used with
  open api.


--------------------------------------------------------------------------*/
typedef struct
{

  qvp_rtp_stream_params_type     stream_params;/* properties of the
                                                * open
                                                */
  boolean                        config_valid; /* Flags a valid
                                                * configuration
                                                * request
                                                */
  qvp_rtp_payload_config2_type   config;       /* payload configuration
                                                * specific details
                                                */
}qvp_rtp_open_config_param_type;

/*--------------------------------------------------------------------------
  TYPEDEF STRUCT QVP_RTP_SAMPLE_WALL_CLOCK

  Application sticks in the wall clock at the sampling instance one in every
  n samples. This value will get transmitted in the RTCP sender report
  before whenever the stack sees valid flag in this structure.

  CAUTION : This determines the delay estimate in the other end. And quality
  of lipsync in Video telephony. One thing to be noted is that for
  meaningful lipsync application use same wall clock for all related
  streams.
--------------------------------------------------------------------------*/
typedef struct
{

  boolean        valid;    /* set one whenever app wishes to send this
                            * out */
  qword          wl_clock; /* wall clock when the packet was sampled. */

} qvp_rtp_sample_wall_clock_type;


/*--------------------------------------------------------------------------
    TYPEDFE STRUCT QVP_RTP_SEND_PKT_CODEC_HDR_TYPE

   Some payload fromats use intimate codec information in the RTP header.
   This structure abstracts such information and sticks in the bit stream.

   This information will blindly pass down all the way to payload format
   and finds it way out the RTP stack.
--------------------------------------------------------------------------*/
typedef struct
{
  boolean  valid;          /* valid codec data for this frame */
  uint8    codec_hdr[QVP_RTP_CODEC_HDR_LEN];     /* slice_hdr */
  uint16   hdr_len;        /* length of data to be inserted   */
} qvp_rtp_send_pkt_codec_hdr_type;



/*--------------------------------------------------------------------------
    TYPEDFE STRUCT QVP_RTP_SEND_PKT_TYPE

    A packet appliation wishes to send. This only makes sense when the
    application has app_id and stream_id passed in part of RTP.

--------------------------------------------------------------------------*/
typedef struct
{
  uint8             *data;                /* the actual data being
                                           * sent
                                           */
  uint16            len;                  /* length of data in buffer */
  boolean           marker_bit;           /* Is this a frame which marks an
                                           * important event ..?
                                           * eg : refresh_frame in video
                                           *    : silence on/off in audio.
                                           */
  uint8             rtp_pyld_type;        /* tx payload type
                                           * received from the upper layers
                                           * for each RTP packet
                                           */
  uint32            time_stamp;            /* wall clock value at the
                                           * sampling instant
                                           */
  qvp_rtp_sample_wall_clock_type wall_clock; /* this is the wall clock
                                              * instance of sampling
                                              */
  qvp_rtp_send_pkt_codec_hdr_type codec_hdr;  /* codec header */

     /* used internally */
  uint16           head_room;              /* head_room in the buffer */
} qvp_rtp_send_pkt_type;

/*--------------------------------------------------------------------------
    TYPEDFE STRUCT QVP_RTP_DTMF_DIGIT_TYPE

    The following structure will represent a DTMF digit for transmission
    through RTP DTMF payload format.
--------------------------------------------------------------------------*/
typedef struct
{

  uint8             dtmf;                 /* the actual  DTMF digit which
                                           * needs to be transported
                                           */
  boolean           end_bit;
  uint8             vol;                  /* volume of the digit */
  uint16            duration;             /* duration in ms */

} qvp_rtp_dtmf_digit_type;

/*--------------------------------------------------------------------------
    TYPEDFE STRUCT QVP_RTP_SEND_DTMF_PKT_TYPE

    This is used by applications which wishes to send  DTMF digits.
    DTMF digit transmission is very different from other payload formats
    and we define a special simplified container to do it.

--------------------------------------------------------------------------*/
typedef struct
{

  uint8             num_digits;           /* number of digits which is
                                           * populated in the following
                                           * array
                                           */
  /* following array stores a burst of DTMF digits */
  qvp_rtp_dtmf_digit_type dtmf_array[ QVP_RTP_MAX_DTMF_BURST_SZ ];
  boolean           marker_bit;           /* Is this a frame which marks an
                                           * important event ..?
                                           */
  uint32            time_stamp;           /* wall clock value at the
                                           * sampling instant
                                           * should be set to the biginning
                                           * of the burst
                                           */
  uint8             rtp_pyld_type;        /* tx payload type
                                           * received from the upper layers
                                           * for each RTP packet
                                           */
  qvp_rtp_sample_wall_clock_type wall_clock;/* this is the wall clock
                                             * instance of sampling
                                             */

} qvp_rtp_send_dtmf_pkt_type;

/*--------------------------------------------------------------------------
    TYPEDFE STRUCT QVP_RTP_DATA_Q_INFO_TYPE

  Statistics and occupany information for queues in audio and video. This
  information or a modified version of this information can be subscribed if
  the application listens to feed back call back.

--------------------------------------------------------------------------*/
typedef struct
{

  uint16 cur_len;                /* current length of queue in bytes */
  uint16 hwm;                    /* the limit ( hwm) at which RTP will stop
                                  * buffering it and start flooring the
                                  * packets
                                  */
  uint32 mac_q;                  /* RTPs estimated or actual value of MAC
                                  * layer buffer occupancy
                                  */
} qvp_rtp_data_q_info_type;

/*--------------------------------------------------------------------------

  TYPEDEF STRUCT QVP_RTP_REGISTER_PARAM_TYPE

  The structure which will contain the parameters for registration.
--------------------------------------------------------------------------*/
typedef struct
{

  uint8   num_ports;          /* number of ports to be reserved */

  uint16  ports_to_reserve[QVP_RTP_MAX_RESV_PORTS]; /* list of ports to be
                                                     * reserved
                                                     */

}qvp_rtp_register_param_type;

/*===========================================================================
                       EXTERNALIZED APIS
===========================================================================*/

/*===========================================================================

FUNCTION QVP_RTP_INIT

DESCRIPTION

  This function is used to initlialize the RTP layer and associated
  data structures.

  Sets the RTP task running.

  Multiple initilizations do not have any effect. Reinitilization
  attempts will be returned with success.

DEPENDENCIES
  None

ARGUMENTS IN
    config - configuration structure for memory allocation and
             capabilities started.


RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesful
  appropriate error code otherwise


SIDE EFFECTS
  None. System will spawn another thread as part of this function.

===========================================================================*/
qvp_rtp_status_type qvp_rtp_init
(
  qvp_rtp_config_type *config
);

/*===========================================================================

FUNCTION QVP_RTP_REGISTER

DESCRIPTION

  This function is used to  register an application to RTP layer.
  A context is allocated for that application. Configuration should be
  set multiuser mode by explicitly setting num_users to more than 2.

  Or a second call to this function fail.

  Result is conveyed using register_cb set in the structure passed. If this
  call back is left empty the request is not processed.


DEPENDENCIES
  None

ARGUMENTS IN
    call_backs  - set of call backs application is registering with.


RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesfully queued to RTP task.
  appropriate error code otherwise


SIDE EFFECTS
  None.

===========================================================================*/
qvp_rtp_status_type qvp_rtp_register
(
  qvp_rtp_app_call_backs_type *call_backs
);

/*==========================================================================

FUNCTION QVP_RTP_REGISTER2

DESCRIPTION

  This function is used to register an application to RTP layer and reserve
  the ports requested by the registering app.Reservation of ports involves
  opening of sockets and binding each of the socket to one of the ports in
  the list of ports which the app passes.Directionality of these sockets/
  ports is decided only when the app uses them while opening the
  channels.
  A context is allocated for that application. Configuration should be set
  multiuser mode by explicitly setting num_users to more than 1.

  Or a second call to this function fail.

  Result is conveyed using register2_cb set in the structure passed. If this
  call back is left empty the request is not processed.


DEPENDENCIES
  None

ARGUMENTS IN
  call_backs  - set of call backs application is registering with.
  register_param - parameter for registration.
  app_data - identifier to identify the app on calback.

RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesfully queued to RTP task.
  appropriate error code otherwise


SIDE EFFECTS
  None.

==========================================================================*/
qvp_rtp_status_type qvp_rtp_register2
(
  qvp_rtp_app_call_backs_type  *call_backs,

  qvp_rtp_register_param_type  *register_param,

  qvp_rtp_app_data_type        app_data

);


/*===========================================================================

FUNCTION QVP_RTP_OPEN

DESCRIPTION

  This function asynchronously executes opening of an RTP channel. The
  result is conveyed through  open call back registered by the app.

DEPENDENCIES
  None

ARGUMENTS IN
    app_id         - application which is requesting to open the channel
    channel_params - channel paramters which are being requeted



RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesfully queued to RTP task.
  appropriate error code otherwise


SIDE EFFECTS
  None.

===========================================================================*/
qvp_rtp_status_type qvp_rtp_open
(
  qvp_rtp_app_id_type        app_id,         /* app which wishes to open
                                              * a stream
                                              */
  qvp_rtp_stream_params_type *stream_params  /* properties of the open
                                              * channel requested
                                              */
);

/*===========================================================================

FUNCTION QVP_RTP_OPEN2

DESCRIPTION

  This function asynchronously executes opening and configuring ( if
  requested )of an RTP channel. The result is conveyed through open call
  back registered by the app.

DEPENDENCIES
  None

ARGUMENTS IN
    app_id         - application which is requesting to open the channel
    open_config_param - stream open and configuration parameters.

RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesfully queued to RTP task.
  appropriate error code otherwise


SIDE EFFECTS
  None.

===========================================================================*/
qvp_rtp_status_type qvp_rtp_open2
(
  qvp_rtp_app_id_type              app_id,            /* app which wishes to
                                                       *  open a stream
                                                       */
  qvp_rtp_open_config_param_type   *open_config_param /* open and
                                                       * config parameters
                                                       */

);
/*===========================================================================

FUNCTION QVP_RTP_READ_CONFIG

DESCRIPTION

  This function SYNCHRONOUSLY executes payload configuration of an RTP
  channel. The result is conveyed by populating the passed in structure on
  return.

DEPENDENCIES
  None

ARGUMENTS IN
    app_id         - application which is requesting to open the channel
    stream_id      - the stream which needs to be configuared

ARGUMENTS OUT
    payld_config   - pointer to configuration structure. On return this
                     structure will be populated with the default values
                     for the profile.




RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesful.
                    appropriate error code otherwise


SIDE EFFECTS
  payld_config structure will be populated with the currenr values
  for the particular profile.

 CAUTION : Application will get unpredictable results if it initiated
    a configure request and it is waiting for response. This is read is
    unprotected.

===========================================================================*/
qvp_rtp_status_type qvp_rtp_read_config
(

  qvp_rtp_app_id_type       app_id,          /* app which wishes to read
                                              * a stream config
                                              */
  qvp_rtp_stream_id_type    stream_id,       /* stream which needs to be
                                              * read out
                                              */
  qvp_rtp_payload_config_type *payld_config  /* where the configuration
                                              * will be copied
                                              */

);

/*===========================================================================

FUNCTION QVP_RTP_READ_DEFAULT_CONFIG

DESCRIPTION

  This function SYNCHRONOUSLY executes payload configuration of an RTP
  channel. The result is conveyed by populating the passed in structure on
  return.

DEPENDENCIES
  None

ARGUMENTS IN
    app_id         - application which is requesting to open the channel
    payload        - payload type for which the default configuration
                     is being read out.

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
qvp_rtp_status_type qvp_rtp_read_default_config
(

  qvp_rtp_app_id_type         app_id,
  qvp_rtp_payload_type      payload,
  qvp_rtp_payload_config_type *payld_config  /* where the configuration
                                              * will be copied
                                              */

);


/*===========================================================================

FUNCTION QVP_RTP_CONFIGURE

DESCRIPTION

  This function asynchronously executes payload configuration of an RTP
  channel. The result is conveyed through  config call back registered by
  the app.

  THIS FUNCTION ALSO SETS THE SESSION IN ACTIVE STATE - IF NOT ALREADY.
  So if app calls this function after a session reset. This creates another
  RTP session logically. Maintaining same port bindings.

DEPENDENCIES
  None

ARGUMENTS IN
    app_id         - application which is requesting to open the channel
    stream_id      - the stream which needs to be configuared
    payld_config   - pointer to configuration structure




RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesfully queued to RTP task.
  appropriate error code otherwise


SIDE EFFECTS
  CAUTION ::
  If the session was reset before this call sets it back to active state.

===========================================================================*/
qvp_rtp_status_type qvp_rtp_configure
(

  qvp_rtp_app_id_type       app_id,          /* app which wishes to open
                                              * a stream
                                              */
  qvp_rtp_stream_id_type    stream_id,       /* stream which needs to be
                                              * configured
                                              */
  qvp_rtp_payload_config_type *payld_config  /* configuration being
                                              * requested
                                              */

);

/*===========================================================================

FUNCTION QVP_RTP_VALIDATE_CONFIG

DESCRIPTION

  This function SYNCHRONOUSLY validates the payload configuration
  provided for the specified payload.

  This API is generally used when a stream is configured for the first time
  or to validate a standalone configuration

  RTP suggests a valid counter configuration for invalid parameters
  App can choose to use / ignore the counter configuration


DEPENDENCIES
  None

ARGUMENTS IN
  app_id       - application which is requesting for validation
  payld_config - configuration which needs to be validated

ARGUMENTS OUT
  counter_config - If the configuration is valid,
                   counter_config is same as payld_config

                   If the given configuration is not successful,
                   and if an alternate value can be proposed by RTP
                   this contains the counter configuration proposed by RTP

                   If the given configuration is not successful,
                   and if an alternate value cannot be proposed by RTP
                   counter configuration is set to invalid


RETURN VALUE
  QVP_RTP_SUCCESS   - The given Configuration is valid for the payload,
                      RTP configure will succeed for these settings

  QVP_RTP_ERR_FATAL - The given Configuration is not valid for the payload
                      RTP configure will fail for these settings
                      counter_config may conatin proposed valid config

SIDE EFFECTS
  None

===========================================================================*/
qvp_rtp_status_type qvp_rtp_validate_config
(

  qvp_rtp_app_id_type          app_id,         /* app requesting for
                                                * configure validation
                                                */
  qvp_rtp_payload_config_type  *payld_config,  /* configuration proposed
                                                * by app
                                                */
  qvp_rtp_payload_config_type  *counter_config /* counter configuration
                                                * proposed by rtp
                                                */

);


/*===========================================================================

FUNCTION QVP_RTP_VALIDATE_STREAM_CONFIG

DESCRIPTION

  This function SYNCHRONOUSLY validates the payload configuration provided &
  also validates if the current stream can support this configuration

  This API is used to validate configuration of an already configured stream
  where the configuration has to be validated with respect
  to the current stream configuration

  If the input configuration is not successful
  then RTP suggests a valid counter configuration.

  App can choose to use / ignore the counter configuration

DEPENDENCIES

  None

ARGUMENTS IN

    app_id       - application which is requesting for validation
    stream_id    - stream which needs to be configured
    payld_config - configuration which needs to be validated

ARGUMENTS OUT

  counter_config - If the configuration is valid,
                   counter_config is same as payld_config

                   If the given configuration is not successful,
                   and if an alternate value can be proposed by RTP
                   this contains the counter configuration proposed by RTP

                   If the given configuration is not successful,
                   and if an alternate value cannot be proposed by RTP
                   counter configuration is set to invalid


RETURN VALUE
  QVP_RTP_SUCCESS   - The given Configuration is valid for the payload,
                      RTP configure will succeed for these settings

  QVP_RTP_ERR_FATAL - The given Configuration is not valid for the payload
                      RTP configure will fail for these settings.
                      counter_config conatins proposed valid configuration

SIDE EFFECTS
  None

===========================================================================*/
qvp_rtp_status_type qvp_rtp_validate_stream_config
(

  qvp_rtp_app_id_type          app_id,          /* app requesting for
                                                 * configure validation
                                                 */
  qvp_rtp_stream_id_type       stream_id,       /* stream which needs to be
                                                 * configured
                                                 */
  qvp_rtp_payload_config_type  *payld_config,   /* configuration proposed
                                                 * by app
                                                 */
  qvp_rtp_payload_config_type  *counter_config  /* counter configuration
                                                 * proposed by rtp
                                                 */

);



/*===========================================================================

FUNCTION QVP_RTP_RESET_SESSION

DESCRIPTION

  This function asynchronously resets a session. By reset we mean that the
  RTP stack will sieze to remember the previous contents in the de-jitter
  buffer, any data queued up in the outbound socket or RTP application
  command queue and any context that should be wiped as part of session
  setup.

  Resets and ends the RTP session associated with already opened
  stream. This logically terminates an RTP session. This call retains
  all the resources pertaining to the opened channel. However the
  RTP/RTCP session context is reset. Also RX and TX path is reset.

DEPENDENCIES
  None

ARGUMENTS IN
    app_id         - application which is requesting to open the channel
    stream_id      - the stream which needs to be configuared




RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesfully queued to RTP task.
  appropriate error code otherwise


SIDE EFFECTS
  None.

===========================================================================*/
qvp_rtp_status_type qvp_rtp_reset_session
(

  qvp_rtp_app_id_type       app_id,          /* app which wishes to open
                                              * a stream
                                              */
  qvp_rtp_stream_id_type    stream_id        /* stream which needs to be
                                              * configured
                                              */

);

/*===========================================================================

FUNCTION QVP_RTP_CLOSE

DESCRIPTION

  This function asynchronously closing of an RTP stream. Result is conveyed
  through close call back.

DEPENDENCIES
  None

ARGUMENTS IN
    app_id         - application which is requesting to open the channel
    stream_id      - stream being closed



RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesfully queued to RTP task.
  appropriate error code otherwise


SIDE EFFECTS
  None.

===========================================================================*/
qvp_rtp_status_type qvp_rtp_close
(
  qvp_rtp_app_id_type    app_id,      /* application which opened
                                       * the stream
                                       */
  qvp_rtp_stream_id_type stream_id    /* stream which was opened */
);

/*==========================================================================

FUNCTION QVP_RTP_DEREGISTER

DESCRIPTION

  This function deregisters an app closing all the streams and sockets
  associated with the app.

DEPENDENCIES
  None

ARGUMENTS IN
  app_id - application which is requesting deregister

RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesfully queued to RTP task.
  appropriate error code otherwise


SIDE EFFECTS
  None.

==========================================================================*/
qvp_rtp_status_type qvp_rtp_deregister
(
  qvp_rtp_app_id_type    app_id
);


/*===========================================================================

FUNCTION QVP_RTP_SEND()


DESCRIPTION

  This function asynchronously sends an RTP packet through the stream.
  No close call back defined to convey result.

DEPENDENCIES
  None

ARGUMENTS IN
    app_id         - application which is sending packet
    stream_id      - stream through which packet is sent
    rtp_pkt        - packet being sent. App .. free this on return
                     from this call



RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesfully queued to RTP task.
  appropriate error code otherwise


SIDE EFFECTS
  None.

===========================================================================*/
qvp_rtp_status_type qvp_rtp_send
(
  qvp_rtp_app_id_type    app_id,      /* application which opened
                                       * the stream
                                       */
  qvp_rtp_stream_id_type stream_id,   /* stream which was opened */

  qvp_rtp_send_pkt_type  *pkt         /* packet which is being sent
                                       * data is copied over
                                       */
);

/*===========================================================================

FUNCTION QVP_RTP_SEND_DTMF


DESCRIPTION
  This function forms the RTP-dtmf payload ( rfc2833 ) and sends it to the
  stream
  This RTP packet payload contains one or  more encoded DTMF digit.
  Application can invoke this primitive to transmit DTMF packets on audio
  channels.

  This API posts a command at RTP which adds RTP header
  ( without going through the profile for which the specific stream
    was configured. No profile specific header is added ) and then
  sends the packet over the network.

  Since the payload type is sent with each RTP packet
  the receiver will be able to differentiate between the audio & dtmf packet.
  No response  primitive is supported.
  If the receive dtmf on a channel configured for audio then we will
  drop those packets

DEPENDENCIES
  None

ARGUMENTS IN
    app_id         - application which is sending packet
    stream_id      - stream through which packet is sent
    dtmf           - dtmf digit which needs to be sent



RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesfully queued to RTP task.
  appropriate error code otherwise


SIDE EFFECTS
  None.

===========================================================================*/
qvp_rtp_status_type qvp_rtp_send_dtmf
(
  qvp_rtp_app_id_type         app_id,      /* application which opened
                                            * the stream
                                            */
  qvp_rtp_stream_id_type      stream_id,   /* stream which was opened */

  qvp_rtp_send_dtmf_pkt_type *dtmf

);

/*===========================================================================

FUNCTION QVP_RTP_PULL_PKT()


DESCRIPTION
    Pulls a packet from inbound jitter buffer.

DEPENDENCIES
  None

ARGUMENTS IN

    app_id         - application which is sending packet
    stream_id      - stream through which packet is sent
    recv_pkt       - packet structure allocated by the app.
    max_len        - maximum len which can copied into the
                     data pointer within the recv_pkt structure.


RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesful; appropriate error code
                     otherwise


SIDE EFFECTS
   The data pointer in recv_pkt will be populated with the amount of data
   pulled.

===========================================================================*/
qvp_rtp_status_type qvp_rtp_pull_pkt
(
  qvp_rtp_app_id_type    app_id,      /* application which opened
                                       * the stream
                                       */
  qvp_rtp_stream_id_type stream_id,   /* stream which was opened */
  qvp_rtp_recv_pkt_type  *recv_pkt,    /* preallocated recv packet
                                       * structure. This structure
                                       * should contain a preallocated
                                       * data pointer as well.
                                       */

  uint16                 max_len       /* maximum len which can be copied */


);

/*===========================================================================

FUNCTION QVP_RTP_READ_Q_INFO


DESCRIPTION
  Reads the buffer occupancy and statistics inside RTP. This is done on a
  per session basis.

DEPENDENCIES
  This call may not be acurate or adequate if the session is operating on
  REV0 mode.

ARGUMENTS IN

    app_id         - application which is sending packet
    stream_id      - stream through which packet is sent

ARGUMENTS OUT
    ifo            - pointer to the data structure which will store the info
                     be passed back to the app.


RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesful; appropriate error code
                     otherwise


SIDE EFFECTS
   The pointer info will be populated with the relavant information.

===========================================================================*/
qvp_rtp_status_type qvp_rtp_read_q_info
(
  qvp_rtp_app_id_type      app_id,      /* application which opened
                                         * the stream
                                         */
  qvp_rtp_stream_id_type   stream_id,   /* stream which was opened */

  qvp_rtp_data_q_info_type *info        /* information regarding the q
                                         * q occupancy and water marks
                                         */

);

/*===========================================================================

FUNCTION QVP_RTP_ENABLE_LYPSYNC


DESCRIPTION

  This function asynchronously enable lip sync with the specifed streams
  Result is conveyed through lip sync call back.

DEPENDENCIES
  None

ARGUMENTS IN
    app_id         - application which is requesting the operation
    audio_stream   - audio stream which should be lip synced
    video_stream   - video stream which should be lip synced



RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesfully queued to RTP task.
  appropriate error code otherwise


SIDE EFFECTS
  None.

===========================================================================*/
qvp_rtp_status_type qvp_rtp_enable_lipsync
(
  qvp_rtp_app_id_type    app_id,      /* application which opened
                                       * the stream
                                       */
  qvp_rtp_stream_id_type audio_stream,/* audio stream which should be lip
                                       * synced
                                       */
  qvp_rtp_stream_id_type video_stream /* video stream which should be lip
                                       * synced
                                       */
);

/*===========================================================================

FUNCTION QVP_RTP_DISABLE_LYPSYNC


DESCRIPTION

  This function asynchronously disable lip sync with the specifed streams
  Result is conveyed through lip sync disable call back.

DEPENDENCIES
  None

ARGUMENTS IN
    app_id         - application which is requesting the operation
    audio_stream   - audio stream which should be lip synced
    video_stream   - video stream which should be lip synced



RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesfully queued to RTP task.
  appropriate error code otherwise


SIDE EFFECTS
  None.

===========================================================================*/
qvp_rtp_status_type qvp_rtp_disable_lipsync
(
  qvp_rtp_app_id_type    app_id,      /* application which opened
                                       * the stream
                                       */
  qvp_rtp_stream_id_type audio_stream,/* audio stream which should be lip
                                       * synced
                                       */
  qvp_rtp_stream_id_type video_stream /* video stream which should be lip
                                       * synced
                                       */
);

/*===========================================================================

FUNCTION QVP_RTP_SHUTDOWN


DESCRIPTION

  This function asynchronously shuts down the RTP layer. This will not kill
  task started during the initialization.

DEPENDENCIES
  None

ARGUMENTS IN
  None.



RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesfully queued to RTP task.
  appropriate error code otherwise


SIDE EFFECTS
  None.

===========================================================================*/
qvp_rtp_status_type qvp_rtp_shut_down
(
  void
);





#endif /* end of FEATURE_QVPHONE_RTP */
#endif /*  end of function _QVP_RTP_API_H_ */

