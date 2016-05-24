#ifndef DS_SOCK_SOCKETDEF_H
#define DS_SOCK_SOCKETDEF_H
/*===========================================================================
  @file ds_Sock_SocketDef.h

  This file defines the class that implements the ISocket interface.

  Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datamodem/interface/dssock/rel/11.03/inc/ds_Sock_SocketDef.h#1 $
  $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"

#ifdef FEATURE_DATA_PS
/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/
#define  SO_KEEPALIVE_DEF_VAL      FALSE
#define  SO_REUSEADDR_DEF_VAL      FALSE
#define  SO_ERROR_ENABLE_DEF_VAL   FALSE
#define  TCP_NODELAY_DEF_VAL       FALSE
#define  TCP_DELAYED_ACK_DEF_VAL   FALSE
#define  TCP_SACK_DEF_VAL          FALSE
#define  TCP_TIMESTAMP_DEF_VAL     FALSE
#define  IP_RECVIF_DEF_VAL         FALSE
#define  IP_RECVERR_DEF_VAL        FALSE
#define  IPV6_RECVERR_DEF_VAL      FALSE
#define  IP_MCAST_LOOP_DEF_VAL     FALSE
#define  SO_SNDBUF_DEF_VAL         (10 * 1024)
#define  SO_SNDBUF_MIN_VAL         536
#define  SO_SNDBUF_MAX_VAL         (256 * 1024)
#define  IP_TOS_DEF_VAL            0
#define  IP_TTL_DEF_VAL            255
#define  IP_MCAST_TTL_DEF_VAL      1
#define  IPV6_TCLASS_DEF_VAL       0
#define  TCP_MAXSEG_DEF_VAL        1460
#define  TCP_MAXSEG_MIN_VAL        536
#define  TCP_MAXSEG_MAX_VAL        (18 * 1024)

#ifndef FEATURE_HDR
  #define  SO_RCVBUF_DEF_VAL       (16 * 1024)
#else
#ifdef FEATURE_JCDMA_2
  #define  SO_RCVBUF_DEF_VAL       (44 * TCP_MAXSEG_DEF_VAL)
#else
  #define  SO_RCVBUF_DEF_VAL       (45 * 1024)
#endif /* FEATURE_JCDMA_2 */
#endif /* FEATURE_HDR */

#define  SO_RCVBUF_MIN_VAL        536
#define  SO_RCVBUF_MAX_VAL        (256 * 1024)

#define  MAX_SOCKET_FACTORY              1

#define  DSSOCK_SOMAXCONN                3

#ifndef FEATURE_DATA_PS_LOW_MEM_CHIPSET
  #define  MAX_TCP_SOCK                    20

#if defined (FEATURE_BCMCS) || defined (FEATURE_MFLO) || defined (FEATURE_DTV_DVBH)
  #define  MAX_UDP_APP_SOCK                (25 + 20)
#else
  #define  MAX_UDP_APP_SOCK                25
#endif

#else /* FEATURE_DATA_PS_LOW_MEM_CHIPSET */
  #define  MAX_TCP_SOCK                    10
  #define  MAX_UDP_APP_SOCK                10
#endif /* FEATURE_DATA_PS_LOW_MEM_CHIPSET */

#ifdef FEATURE_DS_MOBILE_IP
  #define NUM_MIP_SOCKETS (1)
#else
  #define NUM_MIP_SOCKETS (0)
#endif

#define MAX_PING_SESSIONS 2

#define  MAX_UDP_SOCK   (MAX_UDP_APP_SOCK + NUM_MIP_SOCKETS)
#define  MAX_ICMP_SOCK  (2 + MAX_PING_SESSIONS)
#define  MAX_SOCK       (MAX_TCP_SOCK + MAX_UDP_SOCK + MAX_ICMP_SOCK)

#define  MAX_TCB        (MAX_TCP_SOCK + 10)

#endif /* FEATURE_DATA_PS */
#endif /* DS_SOCK_SOCKETDEF_H */
