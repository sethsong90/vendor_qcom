#ifndef _H_QVP_RTP_PROFILE_H_
 #define _H_QVP_RTP_PROFILE_H_
/*=*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                         QVP_RTP_PROFILE . H

GENERAL DESCRIPTION

  This file contains the interface to profile. Ans the private data
  associted with  all profiles

EXTERNALIZED FUNCTIONS

  qvp_rtp_init_profiles

    Goes ahead and initializes all the profiles registered to this entity.
    The parameters are passed and a tx rx call back is stuck in as well.

  qvp_rtp_get_payld_profile

    Returns a profile pointer for accessing a specifcic profiles regstered
    within this module. If no profile is registered for the requested
    profile  a NULL is returned.

  qvp_rtp_find_profile_property
    This function is utility which gives the properties of the profile like
    clock rate, aud/vid, packetization interval etc.


INITIALIZATION AND SEQUENCING REQUIREMENTS

  None of the externalized functions will function before the task is
  spawned.


  Copyright (c) 2004 by Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary.  Export of this technology or software is
  regulated by the U.S. Government. Diversion contrary to U.S. law prohibited.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*=*/

/*===========================================================================

$Header: //source/qcom/qct/multimedia/videophone/ps_media/RTP/RTP_profile/rel/1.0/inc/qvp_rtp_profile.h#1 $ $DateTime: 2008/04/15 18:56:58 $ $Author: yuanz $

                            EDIT HISTORY FOR FILE


when        who    what, where, why
--------    ---    ----------------------------------------------------------
10/31/07   apr    Support for APTO ARR in RTCP
07/09/07   apr    Added error concealment related macros
02/02/07   apr    Added API to get output mode of a stream
07/13/06   uma    Added new functions to qvp_rtp_profile_type
                  for validating payload configuration
05/05/06   apr    Including RTP code in feature FEATURE_QVPHONE_RTP
09/09/05   srk    Removing the send_pkt struct.
05/20/05   srk    Added configure macro
03/18/05   srk    Added MTU macro
10/30/04   srk    Initial Creation.
===========================================================================*/
//#include "customer.h"
#ifdef FEATURE_QVPHONE_RTP

/*===========================================================================
                     INCLUDE FILES FOR MODULE
===========================================================================*/
#include "qvp_rtp_buf.h"
//#include "qvp_rtp_jb_api.h"  /* for jitter buffer mode */
//#include "qvp_rtcp_api.h"

/*===========================================================================
                     CONSTANTS FOR MODULE
===========================================================================*/
#define QVP_RTP_MTU_DLFT            330       /* for testing mtu size */

/*--------------------------------------------------------------------------
  Error concealment related definitions, these are commonly used for all
  video profiles ( H263, MP4 )
--------------------------------------------------------------------------*/
#define QVP_RTP_ERR_CONCL_PATTERN_LENGTH 4 /* Length of bit pattern */

#define QVP_RTP_ERR_CONCEAL_BYTE_1 0x00  /* Error conealment bit */
#define QVP_RTP_ERR_CONCEAL_BYTE_2 0x00  /* pattern - 4 bytes    */
#define QVP_RTP_ERR_CONCEAL_BYTE_3 0x01
#define QVP_RTP_ERR_CONCEAL_BYTE_4 0xA1

/*--------------------------------------------------------------------------

  TYPEDEF VOID *QVP_RTP_PROFILE_HDL_TYPE


  Opaqueue handle into any profile. Profiles use this to identify a channel
  opened.
--------------------------------------------------------------------------*/
typedef void *qvp_rtp_profile_hdl_type;      /* handle into a profile
                                              * stream
                                              */
/*--------------------------------------------------------------------------

  TYPEDEF VOID *QVP_RTP_PROFILE_USR_HDL_TYPE


   User supplied context handle to any channel used when the profile
   calls the RTP stack for help in dispaching packets formed
--------------------------------------------------------------------------*/
typedef void *qvp_rtp_profile_usr_hdl_type;  /* user supplied handle for
                                              * call backs
                                              */
/*--------------------------------------------------------------------------
    TYPEDEF QVP_RTP_PROFILE_PROPERTY_TYPE

      The property of particular payload type. This include clock rate
      packet interval jitter buffer mode etc.
--------------------------------------------------------------------------*/
typedef struct
{

  uint8                   clk_rate;  /* the rate at which RTP clock ticks */
  uint16                  pkt_interval;/* packetization interval */
  //qvp_rtp_jitter_buf_mode mode;        /* jitter buffer mode audio/video */

} qvp_rtp_profile_property_type;

/*--------------------------------------------------------------------------
    TYPEDEF ENUM QVP_RTP_PROFILE_OUTPUT_MODE

    Output mode used by profiles, to determine if the fragments are to be reassembled
    at the profile level or at jitter buffer level
--------------------------------------------------------------------------*/
typedef enum
{

  QVP_RTP_UNK_OUTPUT_MODE,   /* Unknown */
  QVP_RTP_OUTPUT_FRAME_MODE, /* Profiles reassemble frames */
  QVP_RTP_OUTPUT_FRAG_MODE   /* Profiles pass the fragments
                              * recvd( RTP Packets ) to next level
                              * reassembling will be at JB level
                              */

} qvp_rtp_profile_output_mode;


/*--------------------------------------------------------------------------
  TYPEDEF QVP_RTP_STATUS_TYPE ( *QVP_PROFILE_TXMIT_CB )


  This is the call back RTP stack will register in each profile this call
  back is called when the profile is already done with the formation of a
  packet and ready to send it to network.
--------------------------------------------------------------------------*/

typedef qvp_rtp_status_type ( *qvp_profile_txmit_cb )
(
  qvp_rtp_buf_type            *pkt,         /* packet profile mapped and
                                              * send
                                              */
  qvp_rtp_profile_usr_hdl_type usr_data      /* user data supplied while
                                              * calling send
                                              */
);

/*--------------------------------------------------------------------------

    TYPEDEF QVP_RTP_STATUS_TYPE ( *QVP_PROFILE_RECV_CB )

   This is the callback when the profile has already parsed the packet
   and is ready to be dispatched to jitter buffer.
--------------------------------------------------------------------------*/
typedef qvp_rtp_status_type ( *qvp_profile_recv_cb )
(
 qvp_rtp_buf_type   *rcv_pkt,  /* actual data recieved from
                                              * the
                                              */
  qvp_rtp_profile_usr_hdl_type   usr_data
);

/*--------------------------------------------------------------------------

    TYPEDEF  STRUCT QVP_RTP_PROFILE_CONFIG_TYPE

    The configuration structure which will be used by the RTP stack to
    initialize the profile. Profile specific configuration will be
    propagated using a separate API.

--------------------------------------------------------------------------*/
typedef  struct
{

  uint16 num_streams;               /* max number of streams */
  uint16 rtp_mtu;                   /* mtu for underlying trasport */
  qvp_profile_txmit_cb tx_cb;       /* call back from profile to RTP stack
                                     * request to send data
                                     */
  qvp_profile_recv_cb  rx_cb;       /* call back from profile to RTP stack
                                     * to receive data
                                     */


}  qvp_rtp_profile_config_type;


/*--------------------------------------------------------------------------

    TYPEDEF QVP_RTP_STATUS_TYPE ( *QVP_RTP_PROFILE_INIT )

    This API/Accessor is used to initialize the profile. The configuration
    is used as the part of initialization.

--------------------------------------------------------------------------*/
typedef qvp_rtp_status_type ( *qvp_rtp_profile_init )
(
  qvp_rtp_profile_config_type *config
);

/*--------------------------------------------------------------------------

    TYPEDEF QVP_RTP_STATUS_TYPE ( *QVP_RTP_PROFILE_READ_DFLT_CONFIG )

    This API will read out the default configuration for the given
    profile

--------------------------------------------------------------------------*/
typedef qvp_rtp_status_type ( *qvp_rtp_profile_read_dflt_config )
(
  qvp_rtp_payload_type        payld,             /* used by some paylds
                                                  * which supports multiple
                                                  */
  qvp_rtp_payload_config_type *payld_config
);

/*--------------------------------------------------------------------------

    TYPEDEF QVP_RTP_STATUS_TYPE ( *QVP_RTP_PROFILE_VALIDATE_CONFIG )

    This API will validate the given configuration for the given profile

--------------------------------------------------------------------------*/

typedef qvp_rtp_status_type ( *qvp_rtp_profile_validate_config )
(
  qvp_rtp_payload_config_type  *payld_config,
  qvp_rtp_payload_config_type  *counter_config
);


/*--------------------------------------------------------------------------

    TYPEDEF QVP_RTP_STATUS_TYPE ( *QVP_RTP_PROFILE_READ_CONFIG )

    This API will read out the current configuration for the given
    profile

--------------------------------------------------------------------------*/

typedef qvp_rtp_status_type ( *qvp_rtp_profile_read_config )
(
  qvp_rtp_profile_hdl_type     hdl,          /* handle created while open */
  qvp_rtp_payload_config_type *payld_config
);

/*--------------------------------------------------------------------------

    TYPEDEF QVP_RTP_STATUS_TYPE ( *QVP_RTP_PROFILE_CONFIGURE )

    This API will configure a profile with the given parameters.
--------------------------------------------------------------------------*/
typedef qvp_rtp_status_type ( *qvp_rtp_profile_configure )
(
  qvp_rtp_profile_hdl_type     hdl,          /* handle created while open */
  qvp_rtp_payload_config_type *payld_config
);

/*--------------------------------------------------------------------------

  TYPEDEF QVP_RTP_STATUS_TYPE ( *QVP_RTP_PROFILE_OPEN )

  This function is used to open a bi-directional channel into the profile.
  The handle to the channel is copied back in the double pointer.
--------------------------------------------------------------------------*/
typedef qvp_rtp_status_type ( *qvp_rtp_profile_open )
(
  qvp_rtp_profile_hdl_type     *hdl,         /* handle which will get
                                              * populated with a new profile
                                              * stream handle
                                              */
  qvp_rtp_profile_usr_hdl_type  usr_hdl     /* user handle for any cb */
);

/*--------------------------------------------------------------------------

  TYPEDEF QVP_RTP_STATUS_TYPE ( *QVP_RTP_PROFILE_SEND )

    This is the function to send something through the profile. This may
    trigger a transmit call back.

--------------------------------------------------------------------------*/
typedef qvp_rtp_status_type ( *qvp_rtp_profile_send )
(
  qvp_rtp_profile_hdl_type     hdl,          /* handle created while open */
  qvp_rtp_profile_usr_hdl_type usr_hdl,      /* user handle for tx cb */
  qvp_rtp_buf_type             *pkt          /* packet to be formatted
                                              * through the profile
                                              */
);

/*--------------------------------------------------------------------------

    TYPEDEF QVP_RTP_STATUS_TYPE ( *QVP_RTP_PROFILE_RECV )

  This is the function which will take a nw packet in and parse it and send
  it to the app. This may trigger an RX callback.

--------------------------------------------------------------------------*/
typedef qvp_rtp_status_type ( *qvp_rtp_profile_recv )
(

  qvp_rtp_profile_hdl_type     hdl,          /* handle created while open */
  qvp_rtp_profile_usr_hdl_type usr_hdl,      /* user handle for rx cb */

  qvp_rtp_buf_type  *rcv_pkt  /* actual data recieved from
                                              * N/w one N/w pool associated
                                              */

);

/*--------------------------------------------------------------------------

  typedef qvp_rtp_status_type ( *qvp_rtp_profile_close )

    This function is used to close a bi directional channel within the
    profile.
--------------------------------------------------------------------------*/
typedef qvp_rtp_status_type ( *qvp_rtp_profile_close )
(

  qvp_rtp_profile_hdl_type     hdl           /* handle the profile */

);

/*--------------------------------------------------------------------------

  TYPEDEF QVP_RTP_STATUS_TYPE ( *QVP_RTP_PROFILE_SHUTDOWN )

    This function shuts down the profile

--------------------------------------------------------------------------*/
typedef void ( * qvp_rtp_profile_shutdown ) ( void );


/*--------------------------------------------------------------------------
    Each profile has to implement a table of function pointers.
    that table need to be populated in the table of profile tables inside
    RTP stack that hooks up the profile to the stack
--------------------------------------------------------------------------*/
typedef struct
{

  qvp_rtp_profile_init             init_profile;  /* initialise */
  qvp_rtp_profile_open             open_profile;  /* open abi-dir channel*/
  qvp_rtp_profile_send             send_profile;  /* send a packet */
  qvp_rtp_profile_recv             recv_profile;  /* recv a packet */
  qvp_rtp_profile_close            close_profile; /* close already open ch */
  qvp_rtp_profile_read_dflt_config read_dflt;     /* read default config */
  qvp_rtp_profile_read_config      read_config;   /* read current config */
  qvp_rtp_profile_validate_config  validate_config_rx;
                                                  /* validate rx plane
                                                   * configuration
                                                   */
  qvp_rtp_profile_validate_config  validate_config_tx;
                                                  /* validate tx plane
                                                   * configuration
                                                   */
  qvp_rtp_profile_configure        configure;     /* configure the stream
                                                   * with the current
                                                   * configuration
                                                   */
  qvp_rtp_profile_shutdown         shut_profile;  /* shut down the profile */

} qvp_rtp_profile_type;

/*===========================================================================

FUNCTION QVP_RTP_INIT_PROFILES


DESCRIPTION
  This function initializes all the profiles which are registered within
  the RTP profile module.

DEPENDENCIES
  None

ARGUMENTS IN
  num_streams - number of simultaneous streams each profile is expected
                to have.

  rtp_mtu    -  MTU of the underlying transport. Profile MUST not
                try to create a payload Larger than this number.


RETURN VALUE
  QVP_RTP_SUCCESS  - All registered profiles are initialized. Or
                    at least one of the prfiles did not initialize.


SIDE EFFECTS
  None.

===========================================================================*/
qvp_rtp_status_type qvp_rtp_init_profiles
(
  uint16 num_streams,               /* max number of streams */
  uint16 rtp_mtu                    /* mtu for underlying trasport */
);

/*===========================================================================

FUNCTION QVP_RTP_GET_PAYLD_PROFILE


DESCRIPTION
  This fucntion will return a pointer to the approproate profile enum
  passed. If no profile is registered (no table entry ) a NULL is returned.

DEPENDENCIES
  None

ARGUMENTS IN
  payload - enumeration denoting the profiile.

RETURN VALUE
  QVP_RTP_SUCCESS  - All registered profiles are initialized. Or
                    at least one of the prfiles did not initialize.


SIDE EFFECTS
  None.

===========================================================================*/
qvp_rtp_profile_type *qvp_rtp_get_payld_profile
(
  qvp_rtp_payload_type profile
);

/*===========================================================================

FUNCTION QVP_RTP_PROFILE_TXMIT


DESCRIPTION
  This is the function all the profiles will use to push a packet into
  the N/W. Interfaces with RTP layer to get the packet formed.

  Interfaces with the N/W layer to send the packet out.

DEPENDENCIES
  None

ARGUMENTS IN
  pkt - packet being sent out.
  user_data - the stream pointer to which the packet is associated. Passed
              in as void pointer.

RETURN VALUE
  QVP_RTP_SUCCESS  - on rsuccess or failure code.



SIDE EFFECTS
  None.

===========================================================================*/
#if 0
LOCAL qvp_rtp_status_type qvp_rtp_profile_txmit
(
  qvp_rtp_buf_type            *pkt,         /* packet profile mapped and
                                              * send
                                              */
  qvp_rtp_profile_usr_hdl_type usr_data      /* user data supplied while
                                              * calling send
                                              */
);
#endif
/*===========================================================================

FUNCTION QVP_RTP_PROFILE_RX


DESCRIPTION
  This is the function which is registered to all profiles. This function
   will push the packet to Application or associated jitter buffer (if any).

DEPENDENCIES
  None

ARGUMENTS IN
  pkt - packet being received .
  user_data - the stream pointer to which the packet is associated. Passed
              in as void pointer.

RETURN VALUE
  QVP_RTP_SUCCESS  - on rsuccess or failure code.



SIDE EFFECTS
  None.

===========================================================================*/
#if 0
LOCAL qvp_rtp_status_type qvp_rtp_profile_rx
(
  qvp_rtp_buf_type *pkt,      /* packet profile decoded */
  qvp_rtp_profile_usr_hdl_type   usr_data
);
#endif
/*===========================================================================

FUNCTION QVP_RTP_FIND_PROFILE_PROPERTY


DESCRIPTION
  This function is utility which gives the properties of the profile like
  clock rate, aud/vid, packetization interval etc.

DEPENDENCIES
  None

ARGUMENTS IN
  paylod     - paylod for which info is requested
  property   - passed in struct where property is returned

RETURN VALUE
  QVP_RTP_SUCCESS  - on rsuccess or failure code.



SIDE EFFECTS
  None.

===========================================================================*/
qvp_rtp_status_type qvp_rtp_find_profile_property
(
  qvp_rtp_payload_type          payload,
  qvp_rtp_profile_property_type *property
);

/*===========================================================================

FUNCTION QVP_RTP_GET_PROFILE_OUTPUT_MODE


DESCRIPTION
  This function is utility which gives the output mode to be used by a
  profile. Output mode is decided based on jitter size now. RTP Fragments
  are not reassembled at profile level if JB is present.

DEPENDENCIES
  None

ARGUMENTS IN
  usr_data   - the stream pointer Passed in as void pointer.

RETURN VALUE
  qvp_rtp_profile_output_mode  - output mode



SIDE EFFECTS
  None.

===========================================================================*/
qvp_rtp_profile_output_mode qvp_rtp_get_profile_output_mode
(
  uint32  jitter_size
);

/*===========================================================================

FUNCTION QVP_RTP_PROFILE_SHUTDOWN


DESCRIPTION
  Calls the shutdown API for all the profiles we have in record.

DEPENDENCIES
  None

ARGUMENTS IN
  None

RETURN VALUE
  None



SIDE EFFECTS
  None.

===========================================================================*/
void qvp_rtp_profiles_shutdown( void );

#endif /* end of FEATURE_QVPHONE_RTP */
#endif
