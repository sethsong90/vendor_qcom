#ifndef  _QVP_RTP_H_
#define _QVP_RTP_H_
/*=*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                         QVP_RTP.H

GENERAL DESCRIPTION

  This function defines the data types which will define the state of the
  whole RTP stack. The internal APIS which goes to the heart of RTP
  implementation will also go here.

EXTERNALIZED FUNCTIONS

  qvp_rtp_process_app_cmd

    Process application messages by dispatching it to sub handlers. A
    call back is invoked for every message.


INITIALIZATION AND SEQUENCING REQUIREMENTS

  None of the externalized functions will function before the task is
  spawned.


  Copyright (c) 2004 by Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary.  Export of this technology or software is
  regulated by the U.S. Government. Diversion contrary to U.S. law prohibited.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*=*/

/*===========================================================================

$Header: //source/qcom/qct/multimedia/videophone/ps_media/RTP/RTP_core/rel/1.0/inc/qvp_rtp.h#2 $ $DateTime: 2008/05/01 23:32:42 $ $Author: apurupa $

                            EDIT HISTORY FOR FILE


when        who    what, where, why
--------    ---    ----------------------------------------------------------
05/02/08   apr    Removing adaptive JB related code
10/26/07   rk     QDJ & VJB integration. Cleanup
11/14/07   apr    Added adaptive_jb flag
07/13/07   apr    Added macro for TS wrap around check
08/26/07   srk    Add default vid packet interval macro
11/03/06   apr    Support for Adaptive De-Jitter Buffer
12/14/06   apr    Support for clock-skew, added code for receiver
                  reference audio clock
10/10/06   grk    Addition of appid in user context.
08/30/06   grk    Added support for port reservation.
05/05/06   apr    Including RTP code in feature FEATURE_QVPHONE_RTP
09/09/05   sek    Added session reset state.
03/23/05   srk    Fixing a typo in the rtp context
03/18/05   srk    Changed Video Clock to 90
                  Removed RV dependancey
10/30/04   srk    Initial Creation.
===========================================================================*/
#include "customer.h"
#ifdef FEATURE_QVPHONE_RTP

#include "qvp_rtp_msg.h"
#include "qvp_rtcp_api.h"
#include "qvp_rtp_packet.h"
#include "qvp_rtp_profile.h"
#include "qvp_rtp_api.h"


/*===========================================================================
                     MACROS FOR THE MODULE
===========================================================================*/
#define DEAD_BEEF                   0xdeadbeef   /* to denote an invalid
                                                  * handle or pointer
                                                  */

#define QVP_RTP_JITTER_AUD_CLK_DFLT       8   /* 8khz for AMR and EVRC */
#define QVP_RTP_JITTER_AUD_PKT_INTVL_DFLT 20  /* 20 ms for AMR and EVRC */

#define QVP_RTP_JITTER_VID_CLK_DFLT       90 /*  default in 3016
                                               * and 3640
                                               */
#define QVP_RTP_JITTER_VID_PKT_INTVL_DFLT 67 /* translates to 15 fps
                                              * roughly
                                              */

#define QVP_RTP_INITIAL_DEJITTER_OFFSET 100  /* Initial offset used on
                                              * stream reset
                                              */

#define QVP_TS_MAX_WRAP_AROUND      0x7fffffff /* some large no to detect
                                                * timestamp wrap arounds.
                                                */

/*--------------------------------------------------------------------------

  TYPEDEF ENUM QVP_RTP_AV_SYNC_TYPE

  Different modes of Audio Video Sync are listed by this enumeration
--------------------------------------------------------------------------*/
typedef enum
{

  QVP_RTP_AVSYNC_SEPARATE = 0,           /* AVSync mode 0: minimizing the
                                          * Delay-Jitter-AVSync cost funct
                                          * separately for audio & video
                                          */
  QVP_RTP_AVSYNC_MIN_COST_THRESHOLD = 1, /* Get separate dejitter offset by
                                          * minimizing the Delay-Jitter cost
                                          * function and then force the diff
                                          * between the two dejitter offset
                                          * below a threshold
                                          */
  QVP_RTP_AVSYNC_MIN_COST_MAX_TOL  = 2 /* Get separate dejitter offset by
                                          * minimizing the Delay-Jitter cost
                                          * function and then set a common
                                          * dejitter offset as max of
                                          * av_sync tolerance
                                          */

}qvp_rtp_av_sync_type;

/*--------------------------------------------------------------------------

  TYPEDEF VOID *QVP_RTP_JITTER_HDL_TYPE

  The unique handle into the jitter layer to denote an inbound channel.
--------------------------------------------------------------------------*/
typedef void *qvp_rtp_jitter_hdl_type;     /* handle into inbound jitter
                                            * buffer module
                                            */

/*--------------------------------------------------------------------------

  TYPEDEF VOID QVP_RTP_SESS_STATE_TYPE

    Enumeration to notify a session state. This demarks a reset or inactive
    session to an active session.

--------------------------------------------------------------------------*/
typedef enum
{

  QVP_RTP_SESS_ACTIVE,
  QVP_RTP_SESS_INACTIVE

} qvp_rtp_sess_state_type;

/*--------------------------------------------------------------------------

  TYPEDEF STRUCT QVP_RTP_OB_STREAM_TYPE

  The structure which will abstract all the details regarding an outbound
  channel.
--------------------------------------------------------------------------*/
typedef struct
{

  boolean           valid;                   /* set for a valid stream  */
  uint32            remote_ip;               /* remote_addr */
  uint16            remote_port;             /* remote port */
  uint8             cname[QVP_RTCP_CNAME_LEN];/* canonical name passed */
  uint16            local_port;              /* remote port */
  qvp_rtp_trasport_hdl_type sock_id;         /* socket identifier */

  /* stastics */
  uint32            pkt_cnt;

} qvp_rtp_ob_stream_type;


/*--------------------------------------------------------------------------

  TYPEDEF STRUCT QVP_RTP_IB_STREAM_TYPE

  The structure which will abstract all the details regarding an inbound
  channel.
--------------------------------------------------------------------------*/
typedef struct
{

  boolean               valid;              /* set if the stream is valid */
  qvp_rtp_jitter_hdl_type jitter_queue;     /* handle into jitter/lipsync
                                             * module
                                             */
  uint16                jitter_size;        /* jitter size in
                                             * milliseconds
                                             */
  uint32                dejitter_delay;     /* Time for which the arrived
                                             * packets have to wait in the
                                             * jitter buffer
                                             */
  int32                 dejitter_offset;    /* Offset calculated for each
                                             * packet based on first packet
                                             * arrival time
                                             */
  qvp_rtp_payload_type  payload_type;       /* pay load format for stream*/
  uint16                local_port;         /* remote port */
  qvp_rtp_trasport_hdl_type     sock_id;    /* socket identifier */

  /* stastics */
  uint32            pkt_cnt;

} qvp_rtp_ib_stream_type;

/*--------------------------------------------------------------------------
      Each stream handle internally has an o/b stream handle and an inboud
      stream handle. this is done for easy processing of bidirectional
      opens
--------------------------------------------------------------------------*/
typedef struct
{


  /* stream related */
  boolean                valid;       /* set if the stream is valid */
  qvp_rtp_sess_state_type state;      /* state of the stream. Session
                                       * is set to inactive on the call
                                       * to reset and active back on
                                       * a successful configure
                                       */
  qvp_rtp_payload_type   payload_type;/* pay load format for stream*/
  qvp_rtp_channel_type   stream_type; /* inbound/outbound or
                                       * bi-directional
                                       */
  qvp_rtp_ob_stream_type ob_stream;   /* outbound stream */
  qvp_rtp_ib_stream_type ib_stream;   /* inbound stream */

  /* RTCP control related */
  qvp_rtcp_app_handle_type rtcp_hdl;  /* rtcp handle for the session */
  qvp_rtp_ctx_type         rtp;       /* context into RTP
                                       * packetizer/ * depacketizer
                                       */

  /* profile related */
  qvp_rtp_profile_type     *profile;
  qvp_rtp_profile_hdl_type prof_handle; /* profile handle */


  /* data for application use */
  qvp_rtp_app_data_type  app_data;    /* application handle which will be
                                       * refered in all the call backs
                                       * related to the stream
                                      */
  /* data for internal use */
  void                   *usr_ctx;    /* populated while initialization */
  qvp_rtp_stream_id_type *str_id;     /* populated while initialization.
                                       * easy access to top level variables.
                                       * for incoming packet propagation
                                       */

} qvp_rtp_stream_type;


/*--------------------------------------------------------------------------
  TYPEDEF STRUCT QVP_RTP_CTX

  Structure which will abstract all the details within the RTP layer.

  This might mean handles to sub modules but each module context can be
  traced from this top level container.
--------------------------------------------------------------------------*/
typedef struct
{

  boolean                     valid;               /* falgs occupancy */
  uint8                       num_streams;         /* falgs occupancy */
  qvp_rtp_app_call_backs_type call_backs;          /* user call backs */
  qvp_rtp_stream_type         stream_array[ QVP_RTP_MAX_STREAMS_DFLT ];
  qvp_rtp_network_hdl_type    nw_hdl;              /* handle to the list
                                                    * of ports associated
                                                    * with this application
                                                    */
  qvp_rtp_app_id_type         app_id;               /* Application identifier
                                                    * To track user
                                                    * association to the
                                                    * streams and callbacks.
                                                    * This will be passed in
                                                    * pkt callback on pkt
                                                    * reception.
                                                    */

} qvp_rtp_usr_ctx;

typedef struct
{
  uint8           num_users;    /* number of user in the RTP context */
  qvp_rtp_usr_ctx *user_array;  /* array of user contexts            */
  uint8           hk_timer;     /* housekeeping timer for the whole layer */

} qvp_rtp_ctx;

#endif /* end of FEATURE_QVPHONE_RTP */
#endif /* end of _QVP_RTP_H_ */
