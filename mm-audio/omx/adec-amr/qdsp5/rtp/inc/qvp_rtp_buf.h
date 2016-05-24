#ifndef _QVP_RTP_BUF_H_
#define _QVP_RTP_BUF_H_
/*=*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                         QVP_RTP_BUF . H

GENERAL DESCRIPTION

  This file contains the APIS and call backs which are required for the
  application to access RTP functionality.

EXTERNALIZED FUNCTIONS

  qvp_rtp_buf_init()
    Initialize the buffer pool library

  qvp_rtp_alloc_buf()
    Returns a buffer from an appropriate pool. The pool is determined
    by type requested.

 qvp_rtp_free_buf()
    Returns a buffer back to the free pool. The pool to be returned is
    gathered from the buffer pointer itself.


INITIALIZATION AND SEQUENCING REQUIREMENTS

    Any other API will not function if initialization through function
    qvp_rtp_buf_init is not done.


  Copyright (c) 2004 by Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary.  Export of this technology or software is
  regulated by the U.S. Government. Diversion contrary to U.S. law prohibited.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*=*/

/*===========================================================================

$Header: //source/qcom/qct/multimedia/videophone/ps_media/RTP/RTP_core/rel/1.0/inc/qvp_rtp_buf.h#1 $ $DateTime: 2008/04/15 18:56:58 $ $Author: yuanz $

                            EDIT HISTORY FOR FILE


when        who    what, where, why
--------    ---    ----------------------------------------------------------
10/18/07   rk     QDJ integration. Flag for marking silence frames
07/28/06   uma    Increased RTP buffer sizes due to reported field issues
06/14/06   apr    Including codec header info in RTP buffer structure
05/05/06   apr    Including RTP code in feature FEATURE_QVPHONE_RTP
04/28/06   uma    Reduced RTP buffer sizes for optimization
04/26/06   apr    Added a macro for maximum number of video fragments that
                  can be received
01/30/06   srk    Added new member for holding frame info in the
                  structure qvp_rtp_buf_type
06/14/06   srk    Adding len based allocation api.
03/18/05   srk    Added payload type in buffer.
11/01/04   srk    Initial Creation.
===========================================================================*/
//#include "customer.h"
#ifdef FEATURE_QVPHONE_RTP

//#include <queue.h>
#include "qvp_rtp_api.h"

#define QVP_RTP_PKT_NW_SIZE     1472   /* size to gather a n/w packet */
#define QVP_RTP_PKT_AUDIO_SIZE  256    /* size to hold a audio packet */
#define QVP_RTP_PKT_VIDEO_SIZE  8000  /* size to video packet */

/*------------------------------------------------------------------------
  The buffer sizes are for max 1 audio and 1 video stream.
  If multiple streams are involved
  the buffer sizes(in bytes) have to be increased that many times
------------------------------------------------------------------------*/
#define QVP_RTP_POOL_NW_SIZE     100

#define QVP_RTP_POOL_AUDIO_SIZE  90

#define QVP_RTP_POOL_VIDEO_SIZE  20


#define QVP_RTP_MAX_NO_OF_VIDEO_FRAGS 64 /* Maximum number of video
                                          * fragments(RTP pkts) that can
                                          * be rx while reassembling a single
                                          * video frame
                                          * QVP_RTP_PKT_VIDEO_SIZE % RTP_MTU
                                          */

/*--------------------------------------------------------------------------
   enumeration to denote the buffer size being requested.
--------------------------------------------------------------------------*/
typedef enum
{

  QVP_RTP_POOL_NW,         /* buffer pool th gather n/w data size 1470 */
  QVP_RTP_POOL_AUDIO,      /* 512 bytes  this is to gether complete AU*/
  QVP_RTP_POOL_VIDEO,      /* 6000 bytes  a complete FRAME */

} qvp_rtp_buf_pool_type;


/*--------------------------------------------------------------------------
  TYPEDEF QVP_RTP_BUF_TYPE

      Data Buffer to form the pool of packets this module will use.
--------------------------------------------------------------------------*/
typedef struct
{
  //q_link_type   link;               /*  Link to link and unlink into
                                     //*  buffer pool
                                     //*/
  uint8         *data;              /* allocated data pointer for the size
                                     * of the pool
                                     */
  uint16        len;                /* len of actual data content */

  boolean       need_tofree;        /* if app allocated this no need
                                     * to free this
                                     */
  uint16        head_room;          /* room for adding headers */

  /* RTP Specific params may not be valid for all packets */
  uint32            tstamp;         /* the RTP time stamp received      */
  uint32            seq;            /* the seq number of the RTP packet*/
  boolean           marker_bit;     /* Is this a frame which marks an
                                     * important event ..?
                                     * eg : refresh_frame in video
                                     */
  boolean           silence;        /* For audio only: indicates if the pkt
                                     * is a silence pkt
                                     */
  uint8         rtp_pyld_type;      /* RTP payload type as received from
                                     * lower layer
                                     */
  qvp_rtp_frm_info_type_type  frm_info; /* frame information. Valid only
                                        * when the frame is completely
                                        * parsed and ready for codec.
                                        */

  qvp_rtp_send_pkt_codec_hdr_type codec_hdr; /* codec hdr used for
                                              * video codecs Mp4(3016 )
                                              * and H263(2190)
                                              */


  /* for private use of the buffer management library */
//  q_type         *buf_queue;       /* queue from which this got allocated */

  uint16 parse_offset;

} qvp_rtp_buf_type;


/*===========================================================================

FUNCTION QVP_RTP_BUF_INIT

DESCRIPTION

  This function will initialize the common memory pool for reassembly
  buffers video and audio buffers. This pool is shared by profiles,
  jitter buffer and other possible entities within the RTP SW subsystem.

DEPENDENCIES
  None

ARGUMENTS IN
  None


RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesful
  appropriate error code otherwise


SIDE EFFECTS
  None.

===========================================================================*/
qvp_rtp_status_type qvp_rtp_buf_init ( void );

/*===========================================================================

FUNCTION QVP_RTP_ALLOC_BUF

DESCRIPTION
  This function will allocate a data buffer from the pool requested.

DEPENDENCIES
  None

ARGUMENTS IN
  buf_type - enum for the buf type requested n/w audio or video.


RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesful
  appropriate error code otherwise


SIDE EFFECTS
  None. System will spawn another thread as part of this function.

===========================================================================*/
qvp_rtp_buf_type *qvp_rtp_alloc_buf
(
  qvp_rtp_buf_pool_type buf_type
);

/*===========================================================================

FUNCTION QVP_RTP_ALLOC_BUF_BY_LEN

DESCRIPTION
  This function will allocate a data buffer from a pool which fits the
  length requested.

DEPENDENCIES
  None

ARGUMENTS IN
  buf_type - enum for the buf type requested n/w audio or video.


RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesful
  appropriate error code otherwise


SIDE EFFECTS
  None.

===========================================================================*/
qvp_rtp_buf_type *qvp_rtp_alloc_buf_by_len
(
  uint16 len
);

/*===========================================================================

FUNCTION QVP_RTP_FREE_BUF

DESCRIPTION
  This function will return a buffer allocated using the function
  qvp_rtp_alloc_buf  back to the free pool.

DEPENDENCIES
  None

ARGUMENTS IN
  buf_to_free  - buffer to be free


RETURN VALUE
  QVP_RTP_SUCCESS  - operation was succesful
  appropriate error code otherwise


SIDE EFFECTS
  None. System will spawn another thread as part of this function.

===========================================================================*/
void qvp_rtp_free_buf
(
  qvp_rtp_buf_type *buf_to_free
);

/*===========================================================================

FUNCTION QVP_RTP_SHUTDOWN_BUF

DESCRIPTION

  This function shuts down this module and resets the init
  flag to FALSE

DEPENDENCIES
  None

ARGUMENTS IN
None


RETURN VALUE
  None


SIDE EFFECTS
  None.

===========================================================================*/
void qvp_rtp_shutdown_buf( void );




#endif /* end of FEATURE_QVPHONE_RTP */
#endif /* end of _QVP_RTP_BUF_H_ */
