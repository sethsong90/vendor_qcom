
#ifndef __DSSOCKET_COMMON_H__
#define __DSSOCKET_COMMON_H__

#ifdef FEATURE_DS_SOCKETS
/*===========================================================================

   D A T A   S E R V I C E S   S O C K E T   A P I   H E A D E R   F I L E

DESCRIPTION

 The Data Services Socket Header File. Contains shared variables and enums,
 as well as declarations for functions.

 ----------------------------------------------------------------------------
 Copyright (c) 1998-2002 Qualcomm Technologies, Inc. 
 All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
 ----------------------------------------------------------------------------

===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $PVCSPath: L:/src/asw/MM_DATA/vcs/dssocket.h_v   1.13   27 Feb 2003 12:01:52   ubabbar  $
  $Header: //linux/pkgs/proprietary/data/main/source/dss/inc/dssocket_common.h#6 $ $DateTime: 2009/03/24 09:57:23 $ $Author: smasetty $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
03/24/09    SM     Added Call End Reason Code Support
02/05/07    rt     Removed the DSS_TCP_EIFEL code.

===========================================================================*/

/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/

#if 0
#include "comdef.h"
#include "customer.h"
#endif
#ifdef FEATURE_DATA_PS
#ifdef FEATURE_DS_MOBILE_IP
#include "dsmip_cfg.h"
#endif /* FEATURE_DS_MOBILE_IP */

#include "dserrno.h"
#if 0
#include "dss_iface_ioctl.h"
#include "dssocket_defs.h"
#endif
#include "dss_netpolicy.h"
#if 0
#include "ps_in.h"
#endif

/*===========================================================================

                          PUBLIC DATA DECLARATIONS

===========================================================================*/
/*---------------------------------------------------------------------------
                          DSS VERSION  MajorMinor
The least siginficant digit is the minor version number while the  most
significant digits form the major version number.
---------------------------------------------------------------------------*/
#define DSS_VERSION 1700

/*---------------------------------------------------------------------------
                    Return values indicating error status
---------------------------------------------------------------------------*/
#define DSS_SUCCESS         0                      /* successful operation */
#define DSS_ERROR          -1                    /* unsuccessful operation */


/*---------------------------------------------------------------------------
                      Asynchronous Socket Events
---------------------------------------------------------------------------*/
#define DS_WRITE_EVENT  0x01         /* associated with a writeable socket */
#define DS_READ_EVENT   0x02          /* associated with a readable socket */
#define DS_CLOSE_EVENT  0x04         /* associated with a closeable socket */
#define DS_ACCEPT_EVENT 0x08        /* associated with an accetable socket */

/*---------------------------------------------------------------------------
  Ranges of sockfd and sockfdbase:
  NOTE THAT THE  SOCKFDBASE_MIN_VAL SHOULD NOT EQUAL ZERO.
  Note these should be within range of sockfdbase.
---------------------------------------------------------------------------*/
#define SOCKFDBASE_MIN_VAL  DSS_MAX_SOCKS
#define SOCKFDBASE_MAX_VAL  ((1<<15) - 1 - DSS_MAX_SOCKS)

/*---------------------------------------------------------------------------
  Maximum bytes you can read using a single read call.
---------------------------------------------------------------------------*/
#define DSS_READ_MAX_BYTES ( (1 << 15) - 1)
#define DSS_WRITE_MAX_BYTES ( (1 << 15) - 1)

/*---------------------------------------------------------------------------
  Range values for various socket options
---------------------------------------------------------------------------*/
#define DSS_MIN_RCVBUF (536)
#define DSS_MAX_RCVBUF (192 * 1024)

#define DSS_MIN_SNDBUF (536)
#define DSS_DEF_SNDBUF (10 * 1024)
#ifndef FEATURE_IS2000_REL_A
  #define DSS_MAX_SNDBUF (32 * 1024)
#else
  #define DSS_MAX_SNDBUF (36 * 1024)
#endif /* FEATURE_IS2000_REL_A */

/*---------------------------------------------------------------------------
  Macro to convert from the socket file descriptor to a unique index.
---------------------------------------------------------------------------*/
#define SOCKFD_2_INDEX(fd) (((fd) - SOCKFDBASE_MIN_VAL) % DSS_MAX_SOCKS)

/*===========================================================================
  Define flags passed to dss_sendto() calls here. Any time a new flag is to
  be added, define it here and OR it with the existing flags in
  MSG_VALID_BITS macro definition in dssocki.c so that the reserved bit
  mask can be properly built.

  Note: Refer to the SDB related flags defined in dssocket_defs.h when
        adding a new flag to avoid assigning the same value.
===========================================================================*/
#define DSS_MSG_ERRQUEUE 0x2000

#define DSS_MSG_TRUNC    8

#if 0
/*---------------------------------------------------------------------------
  Socket extended error structure for ICMP(v6) error reporting and related
  constants.
---------------------------------------------------------------------------*/
#define DSS_SO_EE_ORIGIN_LOCAL PS_SO_EE_ORIGIN_LOCAL
#define DSS_SO_EE_ORIGIN_ICMP  PS_SO_EE_ORIGIN_ICMP
#define DSS_SO_EE_ORIGIN_ICMP6 PS_SO_EE_ORIGIN_ICMP6

typedef struct ps_sock_extended_err dss_sock_extended_err;
#endif 
/*===========================================================================
                          Socket Address Structures
===========================================================================*/
/*---------------------------------------------------------------------------
  The socket address structures follow the BSD convention, including data
  types, etc.  These are the BSD generic address structures needed to
  support Internet-family addressing.
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
           Network and socket call back declarations for opennetlib2.
---------------------------------------------------------------------------*/
typedef void (*dss_net_cb_fcn)
(
  sint15            dss_nethandle,                               /* Application id */
  dss_iface_id_type iface_id,                    /* Interfcae id structure */
  sint15            dss_errno, /* type of network error, ENETISCONN, ENETNONET.*/
  void            * net_cb_user_data               /* Call back User data  */
);

typedef void (*dss_sock_cb_fcn)
(
  sint15 dss_nethandle,                                          /* Application id */
  sint15 sockfd,                                      /* socket descriptor */
  uint32 event_mask,                                     /* Event occurred */
  void * sock_cb_user_data       /* User data specfied during registration */
);

/*---------------------------------------------------------------------------
                       Socket Options Data Types
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
  The data type used for changing the socket call back function using the
  socket option DSS_SO_CB_FN.
---------------------------------------------------------------------------*/
typedef struct
{
  dss_sock_cb_fcn  sock_cb_fcn;
  void*            sock_cb_user_data;
} dss_sock_cb_fcn_type;

/*---------------------------------------------------------------------------
  Definition for various options that affect the behaviors of a socket.
  Only the options whose names are defined here are supported. The
  following definition is also used as index to the table of all supported
  options.

  +----------------------+-----------+--------------+-----------+-----------+
  |  Name                |   type    |  default val |  min val  |  max val  |
  +----------------------+-----------+--------------+-----------+-----------+
  |  DSS_IP_TTL          |    int    |     255      |     0     |    255    |
  |  DSS_SO_SYS_SOCK     |   bool    |    False     |    N/A    |    N/A    |
  |  DSS_SO_SILENT_CLOSE |   bool    |    False     |    N/A    |    N/A    |
  |  DSS_SO_RCVBUF **    |    int    |     2144     |   2144    |   17520   |
  |  DSS_SO_LINGER       |  dss_so_  |    {0,0}     |   {0,0}   |  {1, N/A} |
  |                      |linger_type|              |           |           |
  |  DSS_SO_SNDBUF **    |    int    |     2144     |    536    |   18432   |
  |  DSS_TCP_MAXSEG      |    int    |      536     |    536    |   18432   |
  |  DSS_SO_SDB_ACK_CB   |dss_so_sdb_| (NULL, NULL) |    N/A    |    N/A    |
  |                      |ack_cb_type|              |           |           |
  |  DSS_TCP_NODELAY     |   bool    |    FALSE     |    N/A    |    N/A    |
  |  DSS_SO_KEEPALIVE    |   bool    |    FALSE     |    N/A    |    N/A    |
  |  DSS_SO_NETPOLICY    |dss_net_   |  DEF_POLICY  |    N/A    |    N/A    |
  |                      |policy_type|              |           |           |
  |  DSS_SO_TX_IFACE     |dss_iface_ |DSS_INVALID_  |    N/A    |    N/A    |
  |                      |id_type    | IFACE_ID     |           |           |
  |  DSS_TCP_DELAYED_ACK |   bool    |    FALSE     |    N/A    |    N/A    |
  |  DSS_TCP_SACK        |   bool    |    FALSE     |    N/A    |    N/A    |
  |  DSS_TCP_TIME_STAMP  |   bool    |    FALSE     |    N/A    |    N/A    |
  |  DSS_BCMCS_JOIN      |dss_so_ip_ |    0         |    N/A    |    N/A    |
  |                      |addr_type  |              |           |           |
  |  DSS_BCMCS_LEAVE     |dss_so_ip_ |    0         |    N/A    |    N/A    |
  |                      |addr_type  |              |           |           |
  |  DSS_SO_CB_FCN       |dss_sock_cb| (NULL, NULL) |    N/A    |    N/A    |
  |                      |_fcn_type  |              |           |           |
  |  DSS_TCP_EIFEL       |   bool    |    FALSE     |    N/A    |    N/A    |
  |  DSS_SO_QOS_SHARE_HANDLE int     |     0        |    N/A    |    N/A    |
  |  DSS_SO_REUSEADDR    |   bool    |    FALSE     |    N/A    |    N/A    |
  |  DSS_SO_DISABLE_     |   bool    |    FALSE     |    N/A    |    N/A    |
  |          FLOW_FWDING |           |              |           |           |
  +----------------------+-----------+--------------+-----------+-----------+

  ** NOTE: the use of this option REQUIRES that more memory be added to the
           DSM item pool.  For each socket that expands its window the
           appropriate memory needs to be added.
---------------------------------------------------------------------------*/
typedef enum
{
  DSS_SOCKOPT_MIN     = -1,        /* lower bound                          */
  DSS_IP_TTL          =  0,        /* time-to-live                         */
  DSS_SO_SYS_SOCK     =  1,        /* bool: is this a system socket?       */
  DSS_SO_SILENT_CLOSE =  2,        /* bool: close() call causes conn reset */
  DSS_SO_RCVBUF       =  3,        /* set the receive window size          */
  DSS_SO_LINGER       =  4,        /* linger on close                      */
  DSS_SO_SNDBUF       =  5,        /* set/get the sndbuf queue size        */
  DSS_TCP_MAXSEG      =  6,        /* set/get the TCP maximum segement size*/
  DSS_SO_SDB_ACK_CB   =  7,        /* call a cb upon recv'ing SDB data ack */
  DSS_TCP_NODELAY     =  8,        /* Disable Nagle's algorithm            */
  DSS_SO_KEEPALIVE    =  9,        /* Send keepalive probes?               */
  DSS_SO_NETPOLICY    =  10,       /* get socket netpolicy                 */
  DSS_SO_TX_IFACE     =  11,       /* get tx iface id                      */
  DSS_TCP_DELAYED_ACK =  12,       /* Enable delayed ack                   */
  DSS_TCP_SACK        =  13,       /* Enable SACK                          */
  DSS_TCP_TIME_STAMP  =  14,       /* Enable TCP time stamp option         */
  DSS_BCMCS_JOIN      =  15,       /* Join m'cast group                    */
  DSS_BCMCS_LEAVE     =  16,       /* Leave m'cast group                   */
  DSS_IP_RECVIF       =  17,       /* Get incoming packet's interface      */
  DSS_IP_TOS          =  18,       /* Type of Service                      */
  DSS_IPV6_TCLASS     =  19,       /* Traffic class for V6 sockets         */
  DSS_SO_CB_FCN       =  20,       /* set the socket callback function     */
  DSS_SO_ERROR_ENABLE =  21,       /* Enable storage of ICMP err in so_err */
  DSS_SO_ERROR        =  22,       /* Get value of ICMP so_error           */
  DSS_SO_LINGER_RESET =  23,       /* Linger and reset on timeout          */
  DSS_IP_RECVERR      =  24,       /* Enable getting ICMP error pkts       */
  DSS_IPV6_RECVERR    =  25,       /* Enable getting ICMPv6 error pkts     */
  DSS_TCP_EIFEL       =  26,       /* Enable EIFEL Algorithm               */
  DSS_SO_QOS_SHARE_HANDLE = 27,    /* QOS group handle                     */
  DSS_SO_REUSEADDR    =  28,       /* Enable Socket Reuse                  */
  DSS_SO_DISABLE_FLOW_FWDING =  29,/* Disable forwarding data on best effort
                                      flow if QoS flow can't be used       */
  DSS_ICMP_ECHO_ID      = 30,      /* ICMP ECHO_REQ message, identifier    */
  DSS_ICMP_ECHO_SEQ_NUM = 31,      /* ICMP ECHO_REQ message, sequence num  */
  DSS_SOCKOPT_MAX                  /* determine upper bound and array size */
} dss_sockopt_names_type;

/*---------------------------------------------------------------------------
  Socket option level specifies the code in the system to interpret the
  option: the general socket code or some protocol-specific code.  If
  an option is unknown at a given level, an error will be generated.
---------------------------------------------------------------------------*/
typedef enum
{
  DSS_IPPROTO_IP   = 1,               /* IP protocol level                 */
  DSS_SOL_SOCKET   = 2,               /* socket level                      */
  DSS_SOCK         = DSS_SOL_SOCKET,  /* another alias for socket level    */
  DSS_IPPROTO_TCP  = 3,               /* TCP protocol level                */
  DSS_IPPROTO_IPV6 = 4,               /* IPV6 protocol level               */
  DSS_IPPROTO_ICMP  = 5,               /* ICMP protocol level              */
  DSS_IPPROTO_ICMP6 = 6,               /* ICMPv6 protocol level            */
} dss_sockopt_levels_type;

/*---------------------------------------------------------------------------
  The data type used for SO_LINGER socket option. Note that l_linger is in
  ms, not in seconds as on UNIX systems
---------------------------------------------------------------------------*/
typedef struct
{
  int l_onoff;                                     /* linger active or not */
  int l_linger;                   /* how many milli-secondss to linger for */
} dss_so_linger_type;

/*---------------------------------------------------------------------------
  Message header structure for ancillary data.
---------------------------------------------------------------------------*/
struct dss_cmsghdr
{
  uint32   cmsg_len;                  /* data byte count, including header */
  int32    cmsg_level;                             /* originating protocol */
  int32    cmsg_type;                            /* protocol-specific type */
};

/*---------------------------------------------------------------------------
  Message header structure for the common I/o functions.
---------------------------------------------------------------------------*/
struct dss_msghdr
{
  void               * msg_name;            /* protocol address            */
  uint16               msg_namelen;         /* size of protocol address    */
  struct iovec       * msg_iov;             /* scatter/gather array        */
  uint16               msg_iovlen;          /* # elements in msg_iov       */
  void               * msg_control;         /* ancillary data              */
  uint16               msg_controllen;      /* length of ancillary data    */
  int                  msg_flags;           /* flags returned by recvmsg   */
};

/*---------------------------------------------------------------------------
  Macros required to process and control the ancillary data struct cmsghdr.
---------------------------------------------------------------------------*/


#define DSS_CMSG_DATA(cmsg) ((void*)((unsigned char *)(cmsg) + \
                             DSS_CMSG_ALIGN(sizeof(struct dss_cmsghdr))))

#define DSS_CMSG_NXTHDR(msg, cmsg) cmsg_nxthdr (msg, cmsg)

#define DSS_CMSG_FIRSTHDR(msg) \
  ((uint32) (msg)->msg_controllen >= sizeof (struct dss_cmsghdr)          \
   ? (struct dss_cmsghdr *) (msg)->msg_control : NULL)

#define DSS_CMSG_ALIGN(len) ( ((len) + sizeof(uint32) - 1) \
                              & (uint32) ~(sizeof(uint32) - 1) )

#define DSS_CMSG_SPACE(len) ( DSS_CMSG_ALIGN(len) \
                              + DSS_CMSG_ALIGN(sizeof(struct dss_cmsghdr)) )

#define DSS_CMSG_LEN(len) ( DSS_CMSG_ALIGN (sizeof(struct dss_cmsghdr)) + \
                            (len) )

#if 0
INLINE struct dss_cmsghdr * cmsg_nxthdr
(
  struct dss_msghdr  * msg,
  struct dss_cmsghdr * cmsg
)
{
  if ((uint32) cmsg->cmsg_len < sizeof (struct dss_cmsghdr))
    return NULL;

  cmsg = (struct dss_cmsghdr *) ((unsigned char *) cmsg
                   + DSS_CMSG_ALIGN (cmsg->cmsg_len));
  if((unsigned char *) (cmsg + 1) > ((unsigned char *) msg->msg_control
                    + msg->msg_controllen))
    return NULL;
  return cmsg;
}
#endif

/*---------------------------------------------------------------------------
  Structures defined for the IP_RECVIF and IP_RECVSTADDR options.
---------------------------------------------------------------------------*/
typedef struct
{
  struct in_addr ip_addr;                      /* destination IPv4 address */
  uint32         if_index;                     /* received interface index */
} dss_in_pktinfo_type;

#if 0
typedef struct
{
  struct in6_addr ip6_addr;                    /* destination IPv6 address */
  uint32          if_index;                    /* received interface index */
} dss_in6_pktinfo_type;

/*---------------------------------------------------------------------------
  The pointer to function data type used for registering callback for the
  SO_SDB_ACK_CB socket option.
---------------------------------------------------------------------------*/
typedef
void (*dss_so_sdb_ack_cb_fcn)
(
  sint15 sockfd,                                     /* socket descriptor  */
  dss_sdb_ack_status_info_type* sdb_ack_info,    /* info about the sdb ack */
  void*  user_data    /* supplied at the time of setting the socket option */
);

/*---------------------------------------------------------------------------
  The data type used for SO_SDB_ACK_CB socket option. The caller supplies
  user_data when setting the socket option and it is passed to the
  sdb_ack_cb_fcn() whenever it is invoked.
---------------------------------------------------------------------------*/
typedef struct
{
  dss_so_sdb_ack_cb_fcn sdb_ack_cb;
  void*                 user_data;
} dss_so_sdb_ack_cb_type;
#endif /* if 0 */

/*---------------------------------------------------------------------------
  The values for the 'how' argument of the shutdown() call. Specifies what
  action to perform:
---------------------------------------------------------------------------*/
#define  DSS_SHUT_RD   0                      /* disallow further receives */
#define  DSS_SHUT_WR   1                         /* disallow further sends */
#define  DSS_SHUT_RDWR 2     /* disallow further receives as well as sends */

#if 1
/*---------------------------------------------------------------------------
  Structures defined for the API dss_getnetdownreason().
---------------------------------------------------------------------------*/
typedef enum
{
  DSS_NET_DOWN_REASON_NOT_SPECIFIED                     = 0,
  DSS_NET_DOWN_REASON_CLOSE_IN_PROGRESS                 = 1,
  DSS_NET_DOWN_REASON_NW_INITIATED_TERMINATION          = 2,

  DSS_NET_DOWN_REASON_INSUFFICIENT_RESOURCES            = 26,
  DSS_NET_DOWN_REASON_UNKNOWN_APN                       = 27,
  DSS_NET_DOWN_REASON_UNKNOWN_PDP                       = 28,
  DSS_NET_DOWN_REASON_AUTH_FAILED                       = 29,
  DSS_NET_DOWN_REASON_GGSN_REJECT                       = 30,
  DSS_NET_DOWN_REASON_ACTIVATION_REJECT                 = 31,
  DSS_NET_DOWN_REASON_OPTION_NOT_SUPPORTED              = 32,
  DSS_NET_DOWN_REASON_OPTION_UNSUBSCRIBED               = 33,
  DSS_NET_DOWN_REASON_OPTION_TEMP_OOO                   = 34,
  DSS_NET_DOWN_REASON_NSAPI_ALREADY_USED                = 35,
  DSS_NET_DOWN_REASON_REGULAR_DEACTIVATION              = 36,
  DSS_NET_DOWN_REASON_QOS_NOT_ACCEPTED                  = 37,
  DSS_NET_DOWN_REASON_NETWORK_FAILURE                   = 38,
  DSS_NET_DOWN_REASON_UMTS_REATTACH_REQ                 = 39,
  DSS_NET_DOWN_REASON_TFT_SEMANTIC_ERROR                = 41,
  DSS_NET_DOWN_REASON_TFT_SYNTAX_ERROR                  = 42,
  DSS_NET_DOWN_REASON_UNKNOWN_PDP_CONTEXT               = 43,
  DSS_NET_DOWN_REASON_FILTER_SEMANTIC_ERROR             = 44,
  DSS_NET_DOWN_REASON_FILTER_SYNTAX_ERROR               = 45,
  DSS_NET_DOWN_REASON_PDP_WITHOUT_ACTIVE_TFT            = 46,
  DSS_NET_DOWN_REASON_INVALID_TRANSACTION_ID            = 81,
  DSS_NET_DOWN_REASON_MESSAGE_INCORRECT_SEMANTIC        = 95,
  DSS_NET_DOWN_REASON_INVALID_MANDATORY_INFO            = 96,
  DSS_NET_DOWN_REASON_MESSAGE_TYPE_UNSUPPORTED          = 97,
  DSS_NET_DOWN_REASON_MSG_TYPE_NONCOMPATIBLE_STATE      = 98,
  DSS_NET_DOWN_REASON_UNKNOWN_INFO_ELEMENT              = 99,
  DSS_NET_DOWN_REASON_CONDITIONAL_IE_ERROR              = 100,
  DSS_NET_DOWN_REASON_MSG_AND_PROTOCOL_STATE_UNCOMPATIBLE = 101,
  DSS_NET_DOWN_REASON_PROTOCOL_ERROR                    = 111,
  DSS_NET_DOWN_REASON_UNKNOWN_CAUSE_CODE                = 200,

  DSS_NET_DOWN_REASON_INTERNAL_MIN                      = 200,
  DSS_NET_DOWN_REASON_INTERNAL_ERROR                    = 201,
  DSS_NET_DOWN_REASON_INTERNAL_CALL_ENDED               = 202,
  DSS_NET_DOWN_REASON_INTERNAL_UNKNOWN_CAUSE_CODE       = 203,
  DSS_NET_DOWN_REASON_INTERNAL_MAX                      = 204,

  DSS_NET_DOWN_REASON_MAX
} dss_net_down_reason_type;
#endif /* if 0 */


/*===========================================================================

                          PUBLIC MACRO DECLARATIONS

===========================================================================*/
/*===========================================================================

MACRO DSS_CLOSE_NETLIB2()

DESCRIPTION
  This macro makes a dss_close_netlib2() call available corresponding to
  dss_open_netlib2() for the sake of uniformity. Since dss_close_netlib2()
  is no different from dss_close_netlib(), it simply calls the latter with
  the passed parameters.

DEPENDENCIES
  None.

RETURN VALUE
  Same as dss_close_netlib()

SIDE EFFECTS
===========================================================================*/
#define dss_close_netlib2(dss_nethandle, errno) dss_close_netlib(dss_nethandle, errno)

/*===========================================================================

MACRO DSNET_GET_HANDLE()

DESCRIPTION
    Macro to change to the new DSNET naming scheme. This macro maps to
    dss_open_netlib2().

DEPENDENCIES
  None.

RETURN VALUE
  Same as dss_open_netlib2()

SIDE EFFECTS
===========================================================================*/
#define dsnet_get_handle(net_cb, net_cb_user_data, sock_cb, \
                         sock_cb_user_data, policy_info_ptr, errno) \
        dss_open_netlib2(net_cb, net_cb_user_data, sock_cb, \
                         sock_cb_user_data, policy_info_ptr, errno)


/*===========================================================================

MACRO DSNET_RELEASE_HANDLE()

DESCRIPTION
    Macro to change to the new DSNET naming scheme. This macro maps to
    dss_close_netlib().

DEPENDENCIES
  None.

RETURN VALUE
  Same as dss_close_netlib()

SIDE EFFECTS
===========================================================================*/
#define dsnet_release_handle(dss_nethandle, errno) dss_close_netlib(dss_nethandle, errno)

/*===========================================================================

MACRO DSNET_START()

DESCRIPTION
  Macro to change to the new DSNET naming scheme. This macro maps to
  dss_pppopen().

DEPENDENCIES
  None.

RETURN VALUE
  Same as dss_pppopen()

SIDE EFFECTS
===========================================================================*/
#define dsnet_start(dss_nethandle, errno) dss_pppopen(dss_nethandle, errno)

/*===========================================================================

MACRO DSNET_STOP()

DESCRIPTION
  Macro to change to the new DSNET naming scheme. This macro maps to
  dss_pppclose().

DEPENDENCIES
  None.

RETURN VALUE
  Same as dss_pppclose()

SIDE EFFECTS
===========================================================================*/
#define dsnet_stop(dss_nethandle, errno) dss_pppclose(dss_nethandle, errno)

/*===========================================================================

MACRO DSNET_GET_POLICY()

DESCRIPTION
  Macro to change to the new DSNET naming scheme. This macro maps to
  dss_get_app_net_policy().

DEPENDENCIES
  None.

RETURN VALUE
  Same as dss_get_app_net_policy()

SIDE EFFECTS
===========================================================================*/
#define dsnet_get_policy(dss_nethandle, policy_info_ptr, errno) \
        dss_get_app_net_policy(dss_nethandle, policy_info_ptr, errno)

/*===========================================================================

MACRO DSNET_SET_POLICY()

DESCRIPTION
  Macro to change to the new DSNET naming scheme. This macro maps to
  dss_set_app_net_policy().

DEPENDENCIES
  None.

RETURN VALUE
  Same as dss_set_app_net_policy()

SIDE EFFECTS
===========================================================================*/
#define dsnet_set_policy(dss_nethandle, policy_info_ptr, errno) \
        dss_set_app_net_policy(dss_nethandle, policy_info_ptr, errno)

/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/
/*===========================================================================

FUNCTION DSS_SOCKET()

DESCRIPTION
 Create a socket and related data structures, and return a socket descriptor.

 The mapping to actual protocols is as follows:

 ADDRESS FAMILY         Stream          Datagram

 AF_INET                TCP             UDP

  Note this function must be called to obtain a valid socket descriptor, for
  use with all other socket-related functions.  Thus, before any socket
  functions can be used (e.g. I/O, asynchronous notification, etc.), this
  call must have successfully returned a valid socket descriptor.  The
  application must also have made a call to dss_open_netlib() to obtain
  a valid application ID, and to put the Data Services task into "sockets"
  mode.

  Note:  This implementation version has no support for Raw IP sockets, and
         will return an error, if the application attempts to create one.

         Sockets created using this call are bound to the dss_nethandle used in
         creating this socket.

DEPENDENCIES
  The function dss_open_netlib() must be called to open the network library
  and put the DS/PS managers into sockets mode.

RETURN VALUE
  On successful creation of a socket, this function returns socket file
  descriptor which is a sint15 value between 0x1000 and 0x1FFF.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EAFNOSUPPORT     address family not supported
  DS_EBADAPP          invalid application ID
  DS_EPROTOTYPE       specified protocol invalid for socket type
  DS_ESOCKNOSUPPORT   invalid or unsupported socket parameter specified
  DS_EPROTONOSUPPORT  specified protocol not supported
  DS_EMFILE           too many descriptors open.  A socket is already open or
                      has not closed compeletely.

SIDE EFFECTS
  None.

===========================================================================*/
extern sint15 dss_socket
(
  sint15 dss_nethandle,                                         /* application ID */
  byte   family,                          /* Address family - AF_INET only */
  byte   type,                                              /* socket type */
  byte   protocol,                                        /* protocol type */
  sint15 *dss_errno                               /* error condition value */
);

/*===========================================================================

FUNCTION DSS_SOCKET2()

DESCRIPTION
 Create a socket and related data structures, and return a socket descriptor.

 The mapping to actual protocols is as follows:

 ADDRESS FAMILY         Stream          Datagram

 AF_INET                TCP             UDP

  Note this function must be called to obtain a valid socket descriptor, for
  use with all other socket-related functions.  Thus, before any socket
  functions can be used (e.g. I/O, asynchronous notification, etc.), this
  call must have successfully returned a valid socket descriptor.  The
  application must also have made a call to dss_open_netlib() to obtain
  a valid application ID, and to put the Data Services task into "sockets"
  mode.

  Note:  This implementation version has no support for Raw IP sockets, and
         will return an error, if the application attempts to create one.

         Sockets created using socket2 are not bound to any particular dss_nethandle.

DEPENDENCIES
  Netpolicy structure needs to be initialized by calling dss_init_netpolicy.

RETURN VALUE
  On successful creation of a socket, this function returns socket file
  descriptor which is a sint15 value between 0x1000 and 0x1FFF.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EAFNOSUPPORT     address family not supported
  DS_EBADAPP          invalid application ID
  DS_EPROTOTYPE       specified protocol invalid for socket type
  DS_ESOCKNOSUPPORT   invalid or unsupported socket parameter specified
  DS_EPROTONOSUPPORT  specified protocol not supported
  DS_EMFILE           too many descriptors open.  A socket is already open or
                      has not closed compeletely.

SIDE EFFECTS
  None.

===========================================================================*/
extern sint15 dss_socket2
(
  byte   family,                          /* Address family - AF_INET only */
  byte   type,                                              /* socket type */
  byte   protocol,                                        /* protocol type */
  dss_sock_cb_fcn sock_cb,                     /* socket callback function */
  void * sock_cb_user_data,              /* User data for socket call back */
  dss_net_policy_info_type * policy_info_ptr,       /* Network policy info */
  sint15 *dss_errno                               /* error condition value */
);

/*===========================================================================

FUNCTION DSS_CONNECT()

DESCRIPTION
  For TCP, attempts to establish the TCP connection.  Upon
  successful connection, calls the socket callback function asserting that
  the DS_WRITE_EVENT is TRUE.  The implementation does not support connected
  UDP sockets and will return an error.  The function must receive
  (as a parameter) a valid socket descriptor, implying a previous successful
  call to dss_socket().

DEPENDENCIES
  Network subsystem must be established and available.

RETURN VALUE
  Returns DSS_SUCCESS on success.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EWOULDBLOCK      operation would block
  DS_EBADF            invalid socket descriptor is specfied
  DS_ECONNREFUSED     connection attempt refused
  DS_ETIMEDOUT        connection attempt timed out
  DS_EFAULT           addrlen parameter is invalid
  DS_EIPADDRCHANGED   IP address changed due to PPP resync
  DS_EINPROGRESS      connection establishment in progress
  DS_EISCONN          a socket descriptor is specified that is already
                      connected
  DS_ENETDOWN         network subsystem unavailable
  DS_EOPNOTSUPP       invalid server address specified
  DS_EADDRREQ         destination address is required.
  DS_NOMEM            not enough memory to establish connection

SIDE EFFECTS
  For TCP, initiates active open for connection.

===========================================================================*/
extern sint15 dss_connect
(
  sint15 sockfd,                                      /* Socket descriptor */
  struct sockaddr *servaddr,                        /* destination address */
  uint16 addrlen,                                    /* length of servaddr */
  sint15 *dss_errno                               /* error condition value */
);

/*===========================================================================

FUNCTION DSS_LISTEN()

DESCRIPTION

  For TCP, this starts a passive open for connections.  Upon a
  sucessful connection, the socket callback function is invoked
  asserting DS_ACCEPT_EVENT as TRUE.  The application should respond
  with a call to dss_accept(). If a connection is recieved and there
  are no free queue slots the new connection is rejected
  (ECONNREFUSED).  The backlog queue is for ALL unaccepted sockets
  (half-open, or completely established).

  A listening UDP doesn't make sense, and as such isn't supported.
  DS_EOPNOTSUPP is returned.

  The sockfd parameter is a created (dss_socket) and bound (dss_bind)
  socket that will become the new listening socket.  The backlog
  parameter indiates the maximum length for the queue of pending
  sockets.  If backlog is larger than the maximum, it will be
  reduced to the maximum (see DSS_SOMAXCONN).

  The argument dss_error should point to a memory location in which
  error conditions can be recorded.

DEPENDENCIES

  Network subsystem must be established and available.

  The sockfd should get a valid socket descriptor (implying a
  previously successful call to dss_socket) This socket should be
  bound to a specific port number (implying a previously successful
  call to dss_bind) .

RETURN VALUE

  Returns DSS_SUCCESS on success.  If the backlog was truncated
  DS_EFAULT will be set in errno, but the call will still be
  successful.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EWOULDBLOCK      operation would block (PJ: I don't think this CAN happen)
  DS_EBADF            invalid socket descriptor is specfied
  DS_EOPNOTSUPP       The socket is not capable of listening (UDP)
  DS_EFAULT           backlog parameter is invalid
  DS_ENETDOWN         network subsystem unavailable
  DS_NOMEM            not enough memory to establish backlog connections.
  DS_EINVAL           Socket already open, closed, unbound or not one
                      you can listen on.

SIDE EFFECTS
  For TCP, initiates passive open for new connections.

===========================================================================*/
extern sint15 dss_listen
(
  sint15 sockfd,                                      /* Socket descriptor */
  sint15 backlog,                      /* Number of connections to backlog */
  sint15 *dss_errno                               /* error condition value */
);

/*===========================================================================

FUNCTION DSS_ACCEPT()

DESCRIPTION

  The accept function is used on listening sockets to respond when
  DS_ACCEPT_EVENT is asserted.  The first backlog queued connection is
  removed from the queue, and bound to a new connected socket (as if
  you called dss_socket).  The newly created socket is in the
  connected state.  The listening socket is unaffect the queue size is
  maintained (ie. there is not need to call listen again.)

  The argument sockfd is the file descriptor of the listening socket

  The argument remote addr is a pointer to a struct sockaddr.  This
  structure is populated with the address information for the remote
  end of the new connection. addrlen should initially contain the
  length of the struct sockaddr passed in.  The length of the real
  address is placed in this location when the struct is populated.

  The argument dss_error should point to a memory location in which
  error conditions can be recorded.

DEPENDENCIES

  Network subsystem must be established and available.

  The sockfd should get a valid socket descriptor (implying a
  previously successful call to dss_socket) This socket should be
  bound to a specific port number (implying a previously successful
  call to dss_bind).  The socket should be listening (implying a
  previously successful call to dss_listen).

RETURN VALUE
  Returns the socket descriptor of the new socket on success.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EWOULDBLOCK      operation would block
  DS_EBADF            invalid socket descriptor is specfied
  DS_EOPNOTSUPP       The socket is not of type SOCK_STREAM
  DS_EINVAL           Socket is not listening.
  DS_EFAULT           The addr parameter is bogus.
  DS_ENETDOWN         network subsystem unavailable
  DS_NOMEM            not enough memory to establish backlog connections.

SIDE EFFECTS

  The head backlog item from the queue of the listening socket is
  removed from that queue.

===========================================================================*/
extern sint15 dss_accept
(
  sint15 sockfd,                                      /* Socket descriptor */
  struct sockaddr *remoteaddr,                       /* new remote address */
  uint16 *addrlen,                                   /* length of servaddr */
  sint15 *dss_errno                               /* error condition value */
);

/*===========================================================================

FUNCTION DSS_OPEN_NETLIB()

DESCRIPTION

  Opens up the network library.  Assigns application ID and sets the
  application-defined callback functions to be called when library and
  socket calls would make progress.  The callback are called with a pointer
  to a sint15 containing the application ID for the callback.
  NOTE: the memory for the application ID is ephemeral and likely will not be
    available after the callback returns - if it is desired to use this
    information outside the scope of the callback it should be COPIED, a
    pointer MUST not be used.

  Puts data services manager into "socket" mode.

  This function is called from the context of the socket client's task.

DEPENDENCIES
  None.

RETURN VALUE
  Returns application ID on success.

  On error, return DSS_SUCCESS and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EMAPP      no more applications available - max apps exceeded.

SIDE EFFECTS
  Puts data services manager into "socket" mode.

===========================================================================*/
extern sint15 dss_open_netlib
(
  void   (*net_callback_fcn)(void *),         /* network callback function */
  void   (*socket_callback_fcn)(void *),       /* socket callback function */
  sint15 *dss_errno                               /* error condition value */
);

/*===========================================================================

FUNCTION DSS_OPEN_NETLIB2()

DESCRIPTION

  Opens up the network library.  Assigns application ID and sets the
  application-defined callback functions to be called when library and
  socket calls would make progress. Stores the network policy info and
  uses it in further calls.

  Puts data services manager into "socket" mode.

  This function is called from the context of the socket client's task.

DEPENDENCIES
  None.

RETURN VALUE
  Returns application ID on success.

  On error, return DSS_SUCCESS and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EMAPP      no more applications available - max apps exceeded.

SIDE EFFECTS
  Puts data services manager into "socket" mode.

===========================================================================*/
extern sint15 dss_open_netlib2
(
  dss_net_cb_fcn net_cb,                      /* network callback function */
  void *  net_cb_user_data,             /* User data for network call back */
  dss_sock_cb_fcn sock_cb,                     /* socket callback function */
  void * sock_cb_user_data,              /* User data for socket call back */
  dss_net_policy_info_type * policy_info_ptr,       /* Network policy info */
  sint15 *dss_errno                               /* error condition value */
);
/*===========================================================================

FUNCTION DSS_CLOSE_NETLIB()

DESCRIPTION

  Closes the network library for the application.  All sockets must have
  been closed for the application, prior to closing.  If this is the last
  remaining application, the network subsytem (PPP/traffic channel) must
  have been brought down, prior to closing the network library.  If this
  is the last active application using the network library, this function
  takes the data services manager out of "socket" mode.

  This function is called from the context of the socket client's task.

DEPENDENCIES
  None.

RETURN VALUE
  On success, returns DSS_SUCCESS.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADAPP        invalid application ID
  DS_ESOCKEXIST     there are existing sockets
  DS_ENETEXIST      the network subsystem exists

SIDE EFFECTS
  Puts data services manager into "autodetect" mode.

===========================================================================*/
extern sint15 dss_close_netlib
(
  sint15 dss_nethandle,                                         /* application ID */
  sint15 *dss_errno                               /* error condition value */
);


/*===========================================================================

FUNCTION DSS_PPPOPEN()

DESCRIPTION
  Starts up the network subsystem (CDMA data service and PPP) over the Um
  interface for all sockets.

DEPENDENCIES
  dss_pppopen() cannot be called by the application if the network is in the
  process of closing. The network layer cannot queue the open request until
  the close is completely finished.  Therefore, the application should wait
  for the net_callback_fn() to be called (after dss_pppclose() has
  completed), before making a call to dss_pppopen().  Note that a valid
  application ID must be specified as a parameter, obtained by a successful
  return of dss_open_netlib().

RETURN VALUE
  If the network subsytem is already established, return DSS_SUCCESS.

  Return DSS_ERROR and places the error condition value in *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADAPP               invalid application ID specified
  DS_EWOULDBLOCK           the operation would block
  DS_ENETCLOSEINPROGRESS   network close in progress. The application
                           should only call dss_pppopen() after the
                           close/abort has completed.

SIDE EFFECTS
  Initiates call origination and PPP negotiation.

===========================================================================*/
extern sint15 dss_pppopen
(
  sint15 dss_nethandle,                                         /* application id */
  sint15 *dss_errno                               /* error condition value */
);


/*===========================================================================

FUNCTION DSS_BIND()

DESCRIPTION
  For all client sockets, attaches a local address and port value to the
  socket.  If the call is not explicitly issued, the socket will implicitly
  bind in calls to dss_connect() or dss_sendto().  Note that this function
  does not support binding a local IP address, but rather ONLY a local port
  number.  The local IP address is assigned automatically by the sockets
  library.  The function must receive (as a parameter) a valid socket
  descriptor, implying a previous successful call to dss_socket().

DEPENDENCIES
  None.

RETURN VALUE
  Returns DSS_SUCCESS on success.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specfied
  DS_EOPNOTSUPP       operation not supported
  DS_EADDRINUSE       the local address is already in use.
  DS_EINVAL           the socket is already attached to a local name
  DS_EFAULT           invalid address parameter has been specified

SIDE EFFECTS
  None.

===========================================================================*/
extern sint15 dss_bind
(
  sint15 sockfd,                                      /* socket descriptor */
  struct sockaddr *localaddr,                             /* local address */
  uint16 addrlen,                                     /* length of address */
  sint15 *dss_errno                               /* error condition value */
);


/*===========================================================================

FUNCTION DSS_CLOSE()

DESCRIPTION
  Non-blocking close of a socket.  Performs all necessary clean-up of data
  structures and frees the socket for re-use.  For TCP initiates the active
  close for connection termination.  Once TCP has closed, the DS_CLOSE_EVENT
  will become TRUE, and the application can call dss_close() again to free
  the socket for re-use.  UDP sockets also need to call this to
  clean-up the socket and free it for re-use.

DEPENDENCIES
  None.

RETURN VALUE
  Returns DSS_SUCCESS on success.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EWOULDBLOCK      operation would block - TCP close in progress
  DS_EBADF            invalid socket descriptor is specfied

SIDE EFFECTS
  Initiates active close for TCP connections.

===========================================================================*/
extern sint15 dss_close
(
  sint15 sockfd,                                      /* socket descriptor */
  sint15 *dss_errno                               /* error condition value */
);


/*===========================================================================

FUNCTION DSS_PPPCLOSE()

DESCRIPTION
  Initiates termination to bring down PPP and the traffic channel.  Upon
  successful close of the network subsystem, invokes the network callback
  function.

DEPENDENCIES
  None.

RETURN VALUE
  If the network subsytem is already closed, return DSS_SUCCESS.

  Returns DSS_ERROR and places the error condition value in *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADAPP               invalid application ID specified
  DS_EWOULDBLOCK           operation would block
  DS_ENETCLOSEINPROGRESS   network close in progress. A call to
                           dss_pppclose() has already been issued.

SIDE EFFECTS
  Initiates termination of PPP.  Brings down PPP and traffic channel.

===========================================================================*/
extern sint15 dss_pppclose
(
  sint15 dss_nethandle,                                         /* application id */
  sint15 *dss_errno                               /* error condition value */
);


/*===========================================================================

FUNCTION DSS_NETSTATUS()

DESCRIPTION
  Provides status of network subsystem.  Called in response to DS_ENETDOWN
  errors.  Note that origination status is based on the last attempted
  origination.

DEPENDENCIES
  None.

RETURN VALUE

  Returns DSS_ERROR and places the error condition value in *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADAPP               invalid application ID specified
  DS_ENETNONET             network subsystem unavailable for some unknown
                           reason
  DS_ENETISCONN            network subsystem is connected and available
  DS_ENETINPROGRESS        network subsystem establishment currently in
                           progress
  DS_ENETCLOSEINPROGRESS   network subsystem close in progress.

SIDE EFFECTS
  None.

===========================================================================*/
extern sint15 dss_netstatus
(
  sint15 dss_nethandle,                                         /* application ID */
  sint15 *dss_errno                               /* error condition value */
);


/*===========================================================================

FUNCTION DSS_ASYNC_SELECT()

DESCRIPTION
  Enables the events to be notified about through the asynchronous
  notification mechanism.  Application specifies a bitmask of events that it
  is interested in, for which it will receive asynchronous notification via
  its application callback function.  This function also performs a real-time
  check to determine if any of the events have already occurred, and if so
  invokes the application callback.

DEPENDENCIES
  None.

RETURN VALUE

  Returns DSS_SUCCESS on success.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specfied

SIDE EFFECTS
  Sets the relevant event mask in the socket control block.  Will also
  notify the application via the callback function.

===========================================================================*/
extern sint31 dss_async_select
(
  sint15 sockfd,                                      /* socket descriptor */
  sint31 interest_mask,                        /* bitmask of events to set */
  sint15 *dss_errno                               /* error condition value */
);


/*===========================================================================

FUNCTION DSS_ASYNC_DESELECT()

DESCRIPTION
  Clears events of interest in the socket control block interest mask.  The
  application specifies a bitmask of events that it wishes to clear; events
  for which it will no longer receive notification.

DEPENDENCIES
  None.

RETURN VALUE
  Returns DSS_SUCCESS on success.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specfied


SIDE EFFECTS
  Clears specified events from the relevant event mask.

===========================================================================*/
extern sint15 dss_async_deselect
(
  sint15 sockfd,                                      /* socket descriptor */
  sint31 clr_interest_mask,                  /* bitmask of events to clear */
  sint15 *dss_errno                               /* error condition value */
);


/*===========================================================================

FUNCTION DSS_GETNEXTEVENT()

DESCRIPTION
  This function performs a real-time check to determine if any of the events
  of interest specified in the socket control block's event mask have
  occurred.  It also clears any bits in the event mask that have occurred.
  The application must re-enable these events through a subsequent call to
  dss_async_select().  The application may pass in a pointer to a single
  socket descriptor to determine if any events have occurred for that socket.

  Alternatively, the application may set this pointer's value to NULL (0)
  (note, not to be confused with a NULL pointer, but rather a pointer whose
  value is 0) in which case the function will return values for the next
  available socket.  The next available socket's descriptor will be placed
  in the socket descriptor pointer, and the function will return.  If no
  sockets are available (no events have occurred across all sockets for
  that application) the pointer value will remain NULL (originally value
  passed in), and the function will return 0, indicating that no events
  have occurred.

DEPENDENCIES
  None.

RETURN VALUE
  Returns an event mask of the events that were asserted.  A value of 0
  indicates that no events have occurred.

  On passing a pointer whose value is NULL into the function for
  the socket descriptor (not to be confused with a NULL pointer), places
  the next available socket descriptor in *sockfd_ptr and returns the
  event mask for that socket. If no sockets are available (no events have
  occurred across all sockets for that application) the pointer value
  will remain NULL (originally value passed in), and the function will
  return 0, indicating that no events have occurred.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADAPP           invalid app descriptor is specfied
  DS_EBADF             invalid socket descriptor is specfied

SIDE EFFECTS
  Clears the bits in the socket control block event mask, corresponding to
  the events that have occurred.

===========================================================================*/
extern sint31 dss_getnextevent
(
  sint15 dss_nethandle,                                         /* application ID */
  sint15 *sockfd_ptr,                                 /* socket descriptor */
  sint15 *dss_errno                               /* error condition value */
);


/*===========================================================================

FUNCTION DSS_WRITE()

DESCRIPTION
  Sends specified number of bytes in the buffer over the TCP transport.

DEPENDENCIES
  None.

RETURN VALUE
  n - the number of bytes written, which could be less than the number of
      bytes specified.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specfied
  DS_ENOTCONN         socket not connected
  DS_ECONNRESET       TCP connection reset by server
  DS_ECONNABORTED     TCP connection aborted due to timeout or other failure
  DS_EIPADDRCHANGED   IP address changed, causing TCP connection reset
  DS_EPIPE            broken pipe
  DS_EADDRREQ         destination address required - connectionless socket
                      did not call dss_connect()
  DS_ENETDOWN         network subsystem unavailable
  DS_EFAULT           application buffer no valid part of address space
  DS_EWOULDBLOCK      operation would block

SIDE EFFECTS
  None.

===========================================================================*/
extern sint15 dss_write
(
  sint15 sockfd,                                      /* socket descriptor */
  const void *buffer,               /* user buffer from which to copy data */
  uint16 nbytes,                /* number of bytes to be written to socket */
  sint15 *dss_errno                               /* error condition value */
);


/*===========================================================================

FUNCTION DSS_WRITEV()

DESCRIPTION
  Provides the gather write variant of the dss_write() call, which
  allows the application to write from non-contiguous buffers.    Sends
  specified number of bytes in the buffer over the TCP transport.

DEPENDENCIES
  None.

RETURN VALUE
  n - the number of bytes written, which could be less than the number of
      bytes specified.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specfied
  DS_ENOTCONN         socket not connected
  DS_ECONNRESET       TCP connection reset by server
  DS_ECONNABORTED     TCP connection aborted due to timeout or other failure
  DS_EIPADDRCHANGED   IP address changed, causing TCP connection reset
  DS_EPIPE            broken pipe
  DS_EADDRREQ         destination address required - connectionless socket
                      did not call dss_connect()
  DS_ENETDOWN         network subsystem unavailable
  DS_EFAULT           application buffer no valid part of address space
  DS_EWOULDBLOCK      operation would block

SIDE EFFECTS
  None.

===========================================================================*/
extern sint15 dss_writev
(
  sint15 sockfd,                                      /* socket descriptor */
  struct iovec iov[],     /* array of data buffers from which to copy data */
  sint15 iovcount,                                /* number of array items */
  sint15 *dss_errno                               /* error condition value */
);


/*===========================================================================

FUNCTION DSS_READ()

DESCRIPTION
  Reads specified number of bytes into buffer from the TCP transport.

DEPENDENCIES
  None.

RETURN VALUE
  n - the number of bytes read, which could be less than the number of
      bytes specified.  A return of 0 indicates that an End-of-File condition
      has occurred.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specfied
  DS_ENOTCONN         socket not connected
  DS_ECONNRESET       TCP connection reset by server
  DS_ECONNABORTED     TCP connection aborted due to timeout or other failure
  DS_EIPADDRCHANGED   IP address changed, causing TCP connection reset
  DS_EPIPE            broken pipe
  DS_EADDRREQ         destination address required - connectionless socket
                      did not call dss_connect()
  DS_ENETDOWN         network subsystem unavailable
  DS_EFAULT           application buffer no valid part of address space
  DS_EWOULDBLOCK      operation would block

SIDE EFFECTS
  None.

===========================================================================*/
extern sint15 dss_read
(
  sint15 sockfd,                                      /* socket descriptor */
  void   *buffer,                     /* user buffer to which to copy data */
  uint16 nbytes,                 /* number of bytes to be read from socket */
  sint15 *dss_errno                               /* error condition value */
);


/*===========================================================================

FUNCTION DSS_READV()

DESCRIPTION
  Provides the scatter read variant of the dss_read() call, which
  allows the application to read into non-contiguous buffers.    Reads
  specified number of bytes into the buffer from the TCP transport.

DEPENDENCIES
  None.

RETURN VALUE
  n - the number of bytes read, which could be less than the number of
      bytes specified.  A return of 0 indicates that an End-of-File condition
      has occurred.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specfied
  DS_ENOTCONN         socket not connected
  DS_ECONNRESET       TCP connection reset by server
  DS_ECONNABORTED     TCP connection aborted due to timeout or other failure
  DS_EIPADDRCHANGED   IP address changed, causing TCP connection reset
  DS_EPIPE            broken pipe
  DS_EADDRREQ         destination address required - connectionless socket
                      did not call dss_connect()
  DS_ENETDOWN         network subsystem unavailable
  DS_EFAULT           application buffer no valid part of address space
  DS_EWOULDBLOCK      operation would block

SIDE EFFECTS
  None.

===========================================================================*/
extern sint15 dss_readv
(
  sint15 sockfd,                                      /* socket descriptor */
  struct iovec iov[],           /* array of data buffers to copy data into */
  sint15 iovcount,                                /* number of array items */
  sint15 *dss_errno                               /* error condition value */
);


/*===========================================================================

FUNCTION DSS_SENDTO()

DESCRIPTION
  Sends 'nbytes' bytes in the buffer over the UDP transport.

DEPENDENCIES
  None.

RETURN VALUE
  n - the number of bytes to be written, which could be less than the number
      of bytes specified.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specfied
  DS_EAFNOSUPPORT     address family not supported
  DS_EWOULDBLOCK      operation would block
  DS_EADDRREQ         destination address required
  DS_ENETDOWN         network subsystem unavailable
  DS_EFAULT           application buffer no valid part of address space
  DS_EOPNOSUPPORT     option not supported

SIDE EFFECTS
  None.

===========================================================================*/
extern sint15 dss_sendto
(
  sint15 sockfd,                                      /* socket descriptor */
  const void *buffer,           /* user buffer from which to copy the data */
  uint16 nbytes,                          /* number of bytes to be written */
  uint32 flags,                                                  /* unused */
  struct sockaddr *toaddr,                          /* destination address */
  uint16 addrlen,                                        /* address length */
  sint15 *dss_errno                               /* error condition value */
);


/*===========================================================================

FUNCTION DSS_RECVFROM()

DESCRIPTION
  Reads 'nbytes' bytes in the buffer from the UDP transport.  Fills in
  address structure with values from who sent the data.

DEPENDENCIES
  None.

RETURN VALUE
  n - the number of bytes to be written, which could be less than the number
      of bytes specified.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specfied
  DS_EAFNOSUPPORT     address family not supported
  DS_EWOULDBLOCK      operation would block
  DS_ENETDOWN         network subsystem unavailable
  DS_EFAULT           application buffer no valid part of address space
  DS_EOPNOSUPPORT     option not supported

SIDE EFFECTS
  None.

===========================================================================*/
extern sint15 dss_recvfrom
(
  sint15 sockfd,                                      /* socket descriptor */
  void   *buffer,               /* user buffer from which to copy the data */
  uint16 nbytes,                          /* number of bytes to be written */
  uint32 flags,                                                  /* unused */
  struct sockaddr *fromaddr,                        /* destination address */
  uint16 *addrlen,                                       /* address length */
  sint15 *dss_errno                               /* error condition value */
);

/*===========================================================================
FUNCTION DSS_GET_IFACE_STATUS()

DESCRIPTION Retrieve iface_status

===========================================================================*/
#define dss_get_iface_status dssnet_get_iface_status


/*===========================================================================
FUNCTION DSS_ENABLE_DORM_TIMER()

DESCRIPTION
  NOTE - THIS FUNCTION IS NOW DEPRECATED.
  Applications should use the new ioctl interface - dss_iface_ioctl() -
  to enable/disable dormancy timer. Refer to dss_iface_ioctl.h more details.

  Applications can call this function to enable or disable
  dormancy timer for an IS707 (1X) packet data call only.

DEPENDENCIES
  If multiple applications call this function, principle used is last
  caller wins.
  If this function is invoked to disable timer, the disabling of timer
  remains in effect until next data call (or if the function is again
  invoked in between to enable timer).

PARAMETERS
  dss_nethandle           application ID
  flag             true/false - enable/disable dorm timer

RETURN VALUE
  None.

SIDE EFFECTS
  None
===========================================================================*/
extern void dss_enable_dorm_timer
(
  sint15 dss_nethandle,
  boolean flag
);

/*===========================================================================

FUNCTION DSS_SETSOCKOPT

DESCRIPTION
  Set the options associated with a socket. This fuction expects the
  following parameters:

DEPENDENCIES
  None.

PARAMETERS
  int sockfd        -     Socket file descriptor.
  int level         -     Socket option level.
  int optname,      -     Option name.
  void *optval      -     Pointer to the option value.
  uint32 *optlen    -     Pointer to the size of the option value.
  sint15 *errno     -     Error condition value.


RETURN VALUE
  On error, return DSS_ERROR and places the error condition value in *errno.

  Errno Values
  ------------
  DS_EBADF                invalid socket descriptor is specfied
  DS_ENOPROTOOPT          the option is unknown at the level indicated
  DS_EINVAL               invalid option name or invalid option value
  DS_EFAULT               Invalid buffer or argument

SIDE EFFECTS
  None.

===========================================================================*/
extern sint15 dss_setsockopt
(
  int sockfd,                            /* socket descriptor              */
  int level,                             /* socket option level            */
  int optname,                           /* option name                    */
  void *optval,                          /* value of the option            */
  uint32 *optlen,                        /* size of the option value       */
  sint15 *dss_errno                       /* error condition value          */
);


/*===========================================================================
FUNCTION DSS_GETSOCKOPT

DESCRIPTION
  Return an option associated with a socket. This fuction expects the
  following parameters:

DEPENDENCIES
  None.

PARAMETERS
  int sockfd        -     Socket file descriptor.
  int level         -     Socket option level.
  int optname,      -     Option name.
  void *optval      -     Pointer to the option value.
  uint32 *optlen    -     Pointer to the size of the option value.
  sint15 *errno     -     Error condition value.

RETURN VALUE
  optlen is a value-result parameter, initially containing the size of
  the buffer pointed to by optval, and modified on return to indicate the
  actual  size  of the value returned. On error, return DSS_ERROR and places
  the error condition value in *errno.

  Errno Values
  ------------
  DS_EBADF                invalid socket descriptor is specfied
  DS_ENOPROTOOPT          the option is unknown at the level indicated
  DS_EINVAL               invalid option name or invalid option value
  DS_EFAULT               Invalid buffer or argument

SIDE EFFECTS
  None.

===========================================================================*/
extern sint15 dss_getsockopt
(
  int sockfd,                            /* socket descriptor              */
  int level,                             /* socket option level            */
  int optname,                           /* option name                    */
  void *optval,                          /* value of the option            */
  uint32 *optlen,                        /* size of the option value       */
  sint15 *dss_errno                      /* error condition value          */
);

/*===========================================================================
FUNCTION DSS_GETSOCKNAME

DESCRIPTION
  Returns the current local address assigned to the specified socket.

DEPENDENCIES
  None.

PARAMETERS
  sockfd    -  socket file descriptor
  addr      -  local address currently associated with the socket
  addrlen   -  address length. This parameter is initialized to indicate
               the amount of space pointed by addr and on return, it
               contains the actual size of the address returned.
  dss_errno -  error number

RETURN VALUE
  Returns DSS_SUCCESS upon successful completion and places the socket
  address and the address length in addr and addrlen parameters, resp.

  If the address is larger than the supplied buffer then it is silently
  truncated. The value returned in addrlen indicates the size prior to
  truncation, if any.

  On error, returns DSS_ERROR and places the error number in dss_errno.

  Errno Values
  ------------
  DS_EBADF                invalid socket descriptor is specfied
  DS_EFAULT               addr parameter points to an invalid memory
                          location

SIDE EFFECTS
  None.

===========================================================================*/
extern sint15 dss_getsockname
(
  sint15           sockfd,                  /* socket descriptor           */
  struct sockaddr* addr,                    /* address of the socket       */
  uint16*          addrlen,                 /* address length              */
  sint15*          dss_errno                /* error number                */
);

/*===========================================================================
FUNCTION DSS_GETPEERNAME

DESCRIPTION
  Returns the address of the peer connected to the specified socket.

DEPENDENCIES
  None.

PARAMETERS
  sockfd    -  socket file descriptor
  addr      -  address of the peer connected with the socket
  addrlen   -  address length. This parameter is initialized to indicate
               the amount of space pointed by addr and on return, it
               contains the actual size of the address returned.
  dss_errno -  error number

RETURN VALUE
  Returns DSS_SUCCESS upon successful completion and places the peer
  address and the address length in addr and addrlen parameters, resp.

  If the address is larger than the supplied buffer then it is silently
  truncated. The value returned in addrlen indicates the size prior to
  truncation, if any.

  On error, returns DSS_ERROR and places the error number in dss_errno.

  Errno Values
  ------------
  DS_EBADF                invalid socket descriptor is specfied
  DS_EFAULT               addr parameter points to an invalid memory
                          location
  DS_ENOTCONN             the socket is not connected

SIDE EFFECTS
  None.

===========================================================================*/
extern sint15 dss_getpeername
(
  sint15           sockfd,                  /* socket descriptor           */
  struct sockaddr* addr,                    /* address of the socket       */
  uint16*          addrlen,                 /* address length              */
  sint15*          dss_errno                /* error number                */
);

/*===========================================================================
FUNCTION DSS_SHUTDOWN

DESCRIPTION
  Shuts down the connection of the specified socket depending on the
  'how' parameter as follows:

  DSS_SHUT_RD:   Disallow subsequent calls to recv function
  DSS_SHUT_WR:   Disallow subsequent calls to send function
  DSS_SHUT_RDWR: Disallow subseuqnet calls to both recv and send functions

DEPENDENCIES
  None.

PARAMETERS
  sockfd    -  socket file descriptor
  how       -  action to be performed: shutdown read-half, write-half or
               both
  dss_errno -  error number

RETURN VALUE
  In case of successful completion, returns DSS_SUCCESS. Otherwise, returns
  DSS_ERROR and places the error number in dss_errno.

  Errno Values
  ------------
  DS_EBADF                invalid socket descriptor is specfied
  DS_ENOTCONN             the socket is not connected
  DS_EINVAL               invalid operation (e.g., how parameter is invalid)
  DS_ENOMEM               insufficient memory available to complete the
                          operation

SIDE EFFECTS
  None.

===========================================================================*/
extern sint15 dss_shutdown
(
  sint15           sockfd,                  /* socket descriptor           */
  uint16           how,                     /* what action to perform      */
  sint15*          dss_errno                /* error number                */
);

/*===========================================================================
FUNCTION DSS_INIT_NET_POLICY_INFO

DESCRIPTION
  Populates the policy info structure with default values.
DEPENDENCIES
  None.
PARAMETERS
  POLICY_INFO_PTR  pointer to policy info data structure.
RETURN VALUE
  None
SIDE EFFECTS
  Initializes the fields in the policy info data structure.

===========================================================================*/
void dss_init_net_policy_info
(
  dss_net_policy_info_type * policy_info_ptr       /* policy info structure */
);

/*===========================================================================
FUNCTION DSS_GET_APP_NET_POLICY

DESCRIPTION
 Fills in the policy info structure with the current net policy of the
 application.

DEPENDENCIES
  None.

PARAMETERS
  DSS_NETHANDLE            application id
  POLICY_INFO_PTR  pointer to policy info data structure.
  DSS_ERRNO        error number

RETURN VALUE
  In case of successful completion, returns DSS_SUCCESS. Otherwise, returns
  DSS_ERROR and places the error number in dss_errno.
  Errno Values
  ------------
  DS_EBADAPP              Invalid application ID is specfied
  DS_EFAULT               Invalid policy_info_ptr is specified.

SIDE EFFECTS
  Initializes the fields in the policy info data structure.

===========================================================================*/
sint15 dss_get_app_net_policy
(
  sint15 dss_nethandle,                                          /* Application id */
  dss_net_policy_info_type * policy_info_ptr,     /* policy info structure */
  sint15 * dss_errno                                       /* error number */
);
/*===========================================================================
FUNCTION DSS_SET_APP_NET_POLICY

DESCRIPTION
   Sets the appliation netpolicy to the user specified value.

DEPENDENCIES
  None.

PARAMETERS
  DSS_NETHANDLE            application id
  POLICY_INFO_PTR  pointer to policy info data structure.
  DSS_ERRNO        error number

RETURN VALUE
  In case of successful completion, returns DSS_SUCCESS. Otherwise, returns
  DSS_ERROR and places the error number in dss_errno.
  Errno Values
  ------------
  DS_EBADAPP              Invalid application ID is specfied
  DS_EFAULT               Invalid policy_info_ptr is specified.

SIDE EFFECTS
  Sets the appliation netpolicy to the user specified value.

===========================================================================*/
sint15 dss_set_app_net_policy
(
  sint15 dss_nethandle,                                          /* Application id */
  dss_net_policy_info_type * policy_info_ptr,     /* policy info structure */
  sint15 * dss_errno                                       /* error number */
);

#if 0
/*===========================================================================
FUNCTION DSS_GET_IFACE_ID_BY_QOS_HANDLE()

DESCRIPTION
  This function returns the iface_id pointing to the iface and the
  corresponding link providing a specific QOS.  The QOS instance is
  identified by the handle previously returned by
  DSS_IFACE_IOCTL_QOS_REQUEST.  The iface_id thus returned is a handle to
  the secondary link of the iface which is providing this QOS instance and
  can be used to perform an IOCTL on that particular link, for example
  registering a PHYS_LINK event on the secondary link.  The handle to the
  secondary link can also be used for DSS_IFACE_IOCTL_QOS_GET_FLOW_SPEC as
  described below.  Only the application (identified by dss_nethandle) which
  previously requested QOS identified by the handle is allowed to retrieve
  the iface_id for the secondary link.

DEPENDENCIES
  None.

PARAMETERS
  handle: qos handle.

RETURN VALUE
  DSS_IFACE_INVALID_ID: If no valid iface could be obtained based on id_info
  iface_id: Otherwise

SIDE EFFECTS
  None.
===========================================================================*/
dss_iface_id_type dss_get_iface_id_by_qos_handle
(
  dss_qos_handle_type  handle     // Handle to QOS instance
);
#endif /* if 0 */

/*===========================================================================
FUNCTION DSS_RECVMSG()

DESCRIPTION
  This function is a common read function for all the socket input
  functions. The message header contains an array of scattered buffers, a
  socket descriptor and an address field for filling the source address
  of the received packet.The function reads data into the scattered buffers
  over the transport specified by the socket descriptor

DEPENDENCIES
  None.

RETURN VALUE
  n - the number of bytes to be written, which could be less than the number
      of bytes specified.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specfied
  DS_EAFNOSUPPORT     address family not supported
  DS_EWOULDBLOCK      operation would block
  DS_EFAULT           bad memory address
  DS_EOPNOTSUPP       option not supported
  DS_EINVAL           can't recv from a listen socket.

SIDE EFFECTS
  None.
===========================================================================*/
sint15 dss_recvmsg
(
  sint15                  sockfd,   /* socket descriptor                   */
  struct dss_msghdr     * msg,      /* Message header for filling in data  */
  int                     flags,    /* flags from dss_recvfrom             */
  sint15                * dss_errno /* error condition value               */
);

/*===========================================================================
FUNCTION DSS_SENDMSG()

DESCRIPTION
  This function is a common write function for all the socket output
  functions. The message header contains an array of scattered buffers, a
  socket descriptor and destination address for unconnected udp sockets.
  The function writes data from the scattered buffers over the transport
  specified by the socket descriptor.
DEPENDENCIES
  None.

RETURN VALUE
  n - the number of bytes to be written, in case of tcp it could be less
  than the number of bytes specified.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ---------------
  DS_EBADF            invalid socket descriptor is specfied
  DS_EAFNOSUPPORT     address family not supported
  DS_EWOULDBLOCK      operation would block
  DS_EADDRREQ         destination address required
  DS_ENETDOWN         network subsystem unavailable
  DS_EFAULT           bad memory address
  DS_EOPNOTSUPP       option not supported
  DS_EMSGSIZE         the msg is too large to be sent all at once
  DS_EISCONN          if the socket is connected and the destination
                      address is other than it is connected to.

SIDE EFFECTS
  None.
===========================================================================*/
sint15 dss_sendmsg
(
  sint15                  sockfd,  /* socket descriptor                    */
  struct dss_msghdr     * msg,     /* Header containing data and dest addr */
  int                     flags,   /* flags used for SDB (if enabled)      */
  sint15                * dss_errno /* error condition value               */
);

#if 0
/*===========================================================================
FUNCTION DSS_GET_SCOPE_ID_BY_IFACE_ID()

DESCRIPTION
  This function allows to retrieve a route_scope from the iface_id.
  Currently, for applications the notion of scope id is basically same as
  iface id as we do not support sitelocal addresses. However, applications
  need not know that scopeid and ifaceid are same as the interpretation can
  change in future when sitelocal multicast is supported.

DEPENDENCIES
  None.

PARAMETERS
  uint32  - Iface id.
  sint15* - Errno.

RETURN VALUE
  On success - Scope Id
  On failure - 0

  dss_errno Values
  ----------------
  DS_EINVAL      Invalid iface id.

SIDE EFFECTS
  None
===========================================================================*/
dss_scope_id_type dss_get_scope_id_by_iface_id
(
  dss_iface_id_type    iface_id,
  sint15 *dss_errno
);
#endif /* if 0 */

/*===========================================================================
FUNCTION DSS_LAST_NETDOWNREASON()

DESCRIPTION
  This function provides an interface to the applications for retrieving the
  reason for the network interface going down.

DEPENDENCIES
  None.

  PARAMETERS
  dss_nethandle     -  application id calling this function.
  reason    -  network down reason.
  dss_errno -  error number.


RETURN VALUE
  In case of successful completion, returns DSS_SUCCESS and places the
  network down reason in reason. Otherwise, returns DSS_ERROR and places
  the error number in dss_errno.

  dss_errno Values
  ---------------
  DS_EBADAPP               invalid application ID specified

SIDE EFFECTS
  None.
===========================================================================*/

sint15
dss_last_netdownreason
(
  sint15  dss_nethandle,                              /* Application id    */
  dss_net_down_reason_type  *reason,                /* network down reason */
  sint15  *dss_errno                                  /* error number      */
);

#ifdef FEATURE_DATA_PS_QOS
/*===========================================================================
FUNCTION DSS_GET_QOS_SHARE_HANDLE()

DESCRIPTION
  This function returns a qos share handle associated with  the set of QOS
  instances requested using the nethandle. Applications can only use this
  handle to set the QOS_SHARE_HANDLE socket option for sockets created using
  socket2. Setting the share handle will enable sockets to use the QOS
  instances created by the nethandle.

DEPENDENCIES
  None.

PARAMETERS
  net_handle: network handle
  dss_errno -  error number.

RETURN VALUE

 dss_errno Values
  ---------------
  DS_EBADAPP               invalid Network handle specified

SIDE EFFECTS
  None.
===========================================================================*/
int32 dss_get_qos_share_handle
(
  sint15 net_handle,
  sint15 *dss_errno
);

#endif  /* FEATURE_DATA_PS_QOS */
#endif  /* FEATURE_DS_SOCKETS */
#endif /* FEATURE_DATA_PS */
#endif  /* _DSSOCKET_H */
