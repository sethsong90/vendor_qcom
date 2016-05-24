#ifndef PS_IN_H
#define PS_IN_H
/*===========================================================================

                   M O D E L    H E A D E R    F I L E

DESCRIPTION


Copyright (c) 2003-2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_in.h#1 $
  $Author: maldinge $ $DateTime: 2011/01/10 09:44:56 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
12/28/10    pp     PS_IN_IS_ADDR_LIMITED_BROADCAST MACRO fix.
12/16/10    dm     Added GRE protocol used for PPTP VPN Passthrough.
07/08/10    pp     Removed unused metacomments
10/13/09    ts     Added definition of PS_IN_ADDRSTRLEN and
                   PS_IN6_ADDRSTRLEN. Previously were defined in dssdns.h.
06/24/09    am     Renamed hton/ntoh macros with ps_ prefix.
05/21/09    pp     IN6 defs renamed to ps_in6 etc.,
03/11/09    jy     Changes to avoid ps/ds type name conflicts when build CDPS
                   with third-part software modules containing a data stack.
11/09/07    ssh    Added support for MIPv6
11/22/06    sv     Added support for IPSEC support for IPv6
11/21/06    rt     Added IGMP/MLD support for MBMS.
11/06/06    mct    Added new macro for identifying valid IPv6 addresses.
04/19/06    rt     Added new IPv6 header extensions.
02/21/06    mct    Compiler fix.
04/26/05    vp     Put parenthesis around IN6_GET_V4_FROM_V4_MAPPED_V6.
04/19/05    jd     Fixed limited broadcast macro to exclude experimental IP
04/18/05    vp     Macros for IPv6 sitelocal and global multicast addrs.
04/13/05    ifk    Added in6_are_prefix_equal
01/10/05    sv     Merged IPSEC changes.
12/27/04    lyr    Added macro to determine if an address is a bcast address
06/11/04    vp     Packed constants for IP protocol types into
                   ip_protocol_enum_type. Definition of byte ordering macros.
05/26/04    mct    Added IN6_IS_PREFIX_LINKLOCAL to properly decode the
                   linklocal prefix.
04/28/04    aku    Added macros to support BCMCS feature.
03/03/04    sv     Added IS_ADDR_MC_LINK_LOCAL. Fixed IS_ADDR_LINKLOCAL
                   macro to use the right mask.
12/07/03    aku/rc Added dss_htonll() and dss_ntohll() macros
11/11/03    aku/rc  Fixed macro IN6_IS_ADDR_LINKLOCAL to use the right address.
08/21/03    ss     Correctly use the last quad while returning the v4 address
                   from v4 mapped v6 address
08/14/03    sv     Created module
===========================================================================*/


/*===========================================================================

                                INCLUDE FILES

===========================================================================*/
#include "comdef.h"
#include "ps_in_alias.h"    /* Macros to map certain data/data types with
                               prefix "ps_" or "dss_" prefixed to data/data
                               type without prefix */

/*---------------------------------------------------------------------------
      IP protocol numbers - use in dss_socket() to identify protocols.
      Also contains the extension header types for IPv6.
---------------------------------------------------------------------------*/
typedef enum
{
  PS_IPV6_BASE_HDR        = 4,                  /* IPv6 Base Header           */
  PS_IPPROTO_HOP_BY_HOP_OPT_HDR = 0,            /* Hop-by-hop Option Header   */
  PS_IPPROTO_ICMP         = 1,                               /* ICMP protocol */
  PS_IPPROTO_IGMP         = 2,                               /* IGMP protocol */
  PS_IPPROTO_IP           = PS_IPV6_BASE_HDR,                /* IPv4          */
  PS_IPPROTO_TCP          = 6,                                /* TCP Protocol */
  PS_IPPROTO_UDP          = 17,                               /* UDP Protocol */
  PS_IPPROTO_IPV6         = 41,                 /* IPv6                       */
  PS_IPPROTO_ROUTING_HDR  = 43,                 /* Routing Header             */
  PS_IPPROTO_FRAG_HDR     = 44,                 /* Fragmentation Header       */
  PS_IPPROTO_GRE          = 47,                               /* GRE Protocol */
  PS_IPPROTO_ESP          = 50,                               /* ESP Protocol */
  PS_IPPROTO_AH           = 51,                 /* Authentication Header      */
  PS_IPPROTO_ICMP6        = 58,                 /* ICMPv6                     */
  PS_NO_NEXT_HDR          = 59,                 /* No Next Header for IPv6    */
  PS_IPPROTO_DEST_OPT_HDR = 60,                 /* Destination Options Header */
  PS_IPPROTO_MOBILITY_HDR = 135,                /* Mobility Header            */
  PS_IPPROTO_TCP_UDP      = 253                 /* Unspecified protocol*/
} ps_ip_protocol_enum_type;

/*---------------------------------------------------------------------------
               Internet-family specific host internet address
---------------------------------------------------------------------------*/
struct ps_in_addr                   /* structure defined for historic reason */
{
  uint32 ps_s_addr;                                        /* socket address */
};

/* Local IP wildcard address */
enum
{
  PS_INADDR_ANY  = 0
};

/* IPv6 address structure */
struct ps_in6_addr
{
  union
  {
    uint8   u6_addr8[16];
    uint16  u6_addr16[8];
    uint32  u6_addr32[4];
    uint64  u6_addr64[2];
  } in6_u;

#define ps_s6_addr    in6_u.u6_addr8
#define ps_s6_addr16  in6_u.u6_addr16
#define ps_s6_addr32  in6_u.u6_addr32
#define ps_s6_addr64  in6_u.u6_addr64
};

/* Common link layer address structure */
typedef struct
{
  byte   ll_addr[6];
  uint8  ll_addr_len;
} ps_link_layer_addr_type;

#ifdef __cplusplus
extern "C" {
#endif

extern const struct ps_in6_addr ps_in6addr_any;                       /* :: */
extern const struct ps_in6_addr ps_in6addr_loopback;                  /* ::1 */
extern const struct ps_in6_addr in6addr_all_hosts;

#ifdef __cplusplus
}
#endif

/*---------------------------------------------------------------------------
                  Internet-family Socket Address Structures
---------------------------------------------------------------------------*/
struct ps_sockaddr_in                     /* Internet style socket address */
{
  uint16 ps_sin_family;                          /* internet socket family */
  uint16 ps_sin_port;                              /* internet socket port */
  struct ps_in_addr ps_sin_addr;                /* internet socket address */
  char   ps_sin_zero[8];         /* zero'ed out data for this address type */
};

struct ps_sockaddr_in6
{
  uint16             ps_sin6_family;                        /*DSS_AF_INET6 */
  uint16             ps_sin6_port;               /* transport layer port # */
  uint32             ps_sin6_flowinfo;            /* IPv6 flow information */
  struct ps_in6_addr ps_sin6_addr;                         /* IPv6 address */
  uint32             ps_sin6_scope_id;     /* set of interface for a scope */
};


/*===========================================================================

                          Socket Address Structures

===========================================================================*/
/*---------------------------------------------------------------------------
  The socket address structures follow the BSD convention, including data
  types, etc.  These are the BSD generic address structures needed to
  support Internet-family addressing.
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
                     BSD Generic Socket Address structure
---------------------------------------------------------------------------*/
struct ps_sockaddr                     /* generic socket address structure */
{
  uint16         ps_sa_family;                    /* socket address family */
  unsigned char  ps_sa_data[14];                          /* address data */
};

/*---------------------------------------------------------------------------
                    Sockaddr storage structure
---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
 Definitions used for ps_sockaddr_storage structure padding
---------------------------------------------------------------------------*/
#define PS_SS_MAXSIZE    32
#define PS_SS_ALIGNSIZE  (sizeof(int64))
#define PS_SS_PADSIZE    ((PS_SS_MAXSIZE) - (2 * (PS_SS_ALIGNSIZE)))

struct ps_sockaddr_storage
{
 uint16       ps_ss_family;             /* address family                  */
 int64        ps_ss_align;              /* field to force alignment      */
 char         ps_ss_pad[PS_SS_PADSIZE];   /* Padding                         */
};

/*---------------------------------------------------------------------------
                       Non-Contiguous Buffer Structure
---------------------------------------------------------------------------*/
struct ps_iovec
{
  byte  *ps_iov_base;                        /* starting address of buffer */
  uint16 ps_iov_len;                        /* size of the buffer in bytes */
};

/*---------------------------------------------------------------------------
TYPEDEF IP_ADDR_ENUM_TYPE

DESCRIPTION
  An enum that defines all of the address types supported - used to
  discriminate the union below.

  NOTE: The values are chosen to easy debugging.
---------------------------------------------------------------------------*/
typedef enum
{
  IP_ANY_ADDR     = 0,
  IPV4_ADDR       = 4,
  IPV6_ADDR       = 6,
  IP_ADDR_INVALID           = 255,
  IFACE_ANY_ADDR_FAMILY     = IP_ANY_ADDR,
  IFACE_IPV4_ADDR_FAMILY    = IPV4_ADDR,
  IFACE_IPV6_ADDR_FAMILY    = IPV6_ADDR,
  IFACE_UNSPEC_ADDR_FAMILY  = 8,
  IFACE_INVALID_ADDR_FAMILY = IP_ADDR_INVALID
} ip_addr_enum_type;

/*---------------------------------------------------------------------------
TYPEDEF IP_ADDR_TYPE

DESCRIPTION
  structure which is a discriminated union that defines the IP addresses that
  we support.
---------------------------------------------------------------------------*/
typedef struct ip_address
{
  ip_addr_enum_type type;

  union
  {
    uint32 v4;
    uint64 v6[2];
  } addr;

} ip_addr_type;

/*---------------------------------------------------------------------------
MACRO IP_ADDR_TYPE_IS_VALID()

DESCRIPTION
  This macro returns a boolean indicating whether the address type passed in
  is valid.

PARAMETERS
  addr_type: the address type being checked

RETURN VALUE
  TRUE: if addr_type is valid
  FALSE: otherwise.
===========================================================================*/
#define IP_ADDR_TYPE_IS_VALID( addr_type )                               \
  ((addr_type) == IP_ANY_ADDR ||                                         \
   (addr_type) == IPV4_ADDR   ||                                         \
   (addr_type) == IPV6_ADDR)

/*---------------------------------------------------------------------------
TYPEDEF PS_IP_ADDR_TYPE

DESCRIPTION
  structure which is a discriminated union that defines the IP addresses that
  we support.
---------------------------------------------------------------------------*/
typedef struct
{
  ip_addr_enum_type type;

  union
  {
    struct ps_in_addr  v4;
    struct ps_in6_addr v6;
  } addr;

} ps_ip_addr_type;

typedef enum
{
  IP_V4 = 4,
  IP_V6 = 6
} ip_version_enum_type;

/*===========================================================================

MACRO PS_HTONL()

DESCRIPTION
  Converts host-to-network long integer.  Handles potential byte order
  differences between different computer architectures and different network
  protocols.

PARAMETERS
  x     unsigned long integer value to be converted.

DEPENDENCIES
  None.

RETURN VALUE
  The network byte-ordered value.

SIDE EFFECTS
  None.

===========================================================================*/
/*lint -emacro(572, ps_htonl) The bit shifting is correct*/
#define ps_htonl(x)                                                      \
  (((((uint32)(x) & 0x000000FFU) << 24) |                                \
  (((uint32)(x) & 0x0000FF00U) <<  8) |                                  \
  (((uint32)(x) & 0x00FF0000U) >>  8) |                                  \
  (((uint32)(x) & 0xFF000000U) >> 24)))

/*===========================================================================

MACRO PS_HTONLL()

DESCRIPTION
  Converts host-to-network long integer.  Handles potential byte order
  differences between different computer architectures and different network
  protocols.

PARAMETERS
  x     unsigned long integer value to be converted.

DEPENDENCIES
  None.

RETURN VALUE
  The host byte-ordered value.

SIDE EFFECTS
  None.

===========================================================================*/
/*lint -emacro(572, ps_htonll) The bit shifting is correct*/
#define ps_htonll(x)                                                     \
  (((((uint64)(x) & 0x00000000000000FFULL) << 56)   |                    \
  (((uint64)(x) & 0x000000000000FF00ULL) << 40)   |                      \
  (((uint64)(x) & 0x0000000000FF0000ULL) << 24)   |                      \
  (((uint64)(x) & 0x00000000FF000000ULL) << 8)    |                      \
  (((uint64)(x) & 0x000000FF00000000ULL) >> 8)    |                      \
  (((uint64)(x) & 0x0000FF0000000000ULL) >> 24)   |                      \
  (((uint64)(x) & 0x00FF000000000000ULL) >> 40)   |                      \
  (((uint64)(x) & 0xFF00000000000000ULL) >> 56)))

/*===========================================================================

MACRO PS_HTONS()

DESCRIPTION
 Converts host-to-network short integer.  Handles potential byte order
 differences between different computer architectures and different network
 protocols.

PARAMETERS
  x     unsigned short integer value to be converted.

DEPENDENCIES
  None.

RETURN VALUE
  The network byte-ordered value.

SIDE EFFECTS
  None.

===========================================================================*/
/*lint -emacro(572, ps_htons) The bit shifting is correct*/
#define ps_htons(x)                                                      \
  (((((uint16)(x) & 0x00FF) << 8) | (((uint16)(x) & 0xFF00) >> 8)))

/*===========================================================================

MACRO PS_NTOHL()

DESCRIPTION
  Converts network-to-host long integer.  Handles potential byte order
  differences between different computer architectures and different network
  protocols.

PARAMETERS
  x     unsigned long integer value to be converted.

DEPENDENCIES
  None.

RETURN VALUE
  The host byte-ordered value.

SIDE EFFECTS
  None.

===========================================================================*/
/*lint -emacro(572, ps_ntohl) The bit shifting is correct*/
#define ps_ntohl(x)                                                      \
  (((((uint32)(x) & 0x000000FFU) << 24) |                                \
  (((uint32)(x) & 0x0000FF00U) <<  8) |                                  \
  (((uint32)(x) & 0x00FF0000U) >>  8) |                                  \
  (((uint32)(x) & 0xFF000000U) >> 24)))

/*===========================================================================

MACRO PS_NTOHLL()

DESCRIPTION
  Converts network-to-host long integer.  Handles potential byte order
  differences between different computer architectures and different network
  protocols.

PARAMETERS
  x     unsigned long integer value to be converted.

DEPENDENCIES
  None.

RETURN VALUE
  The host byte-ordered value.

SIDE EFFECTS
  None.

===========================================================================*/
/*lint -emacro(572, ps_ntohll) The bit shifting is correct*/
#define ps_ntohll(x)                                                     \
  (((((uint64)(x) & 0x00000000000000FFULL) << 56)   |                    \
  (((uint64)(x) & 0x000000000000FF00ULL) << 40)   |                      \
  (((uint64)(x) & 0x0000000000FF0000ULL) << 24)   |                      \
  (((uint64)(x) & 0x00000000FF000000ULL) << 8)    |                      \
  (((uint64)(x) & 0x000000FF00000000ULL) >> 8)    |                      \
  (((uint64)(x) & 0x0000FF0000000000ULL) >> 24)   |                      \
  (((uint64)(x) & 0x00FF000000000000ULL) >> 40)   |                      \
  (((uint64)(x) & 0xFF00000000000000ULL) >> 56)))

/*===========================================================================

MACRO PS_NTOHS()

DESCRIPTION
 Converts network-to-host short integer.  Handles potential byte order
  differences between different computer architectures and different network
  protocols.

PARAMETERS
  x     unsigned short integer value to be converted.

DEPENDENCIES
  None.

RETURN VALUE
  The host byte-ordered value.

SIDE EFFECTS
  None.

===========================================================================*/
/*lint -emacro(572, ps_ntohs) The bit shifting is correct*/
#define ps_ntohs(x)                                                      \
  (((((uint16)(x) & 0x00FF) << 8) | (((uint16)(x) & 0xFF00) >> 8)))

/*---------------------------------------------------------------------------
 IN6 Macros
---------------------------------------------------------------------------*/
#define PS_IN6ADDR_ANY_INIT { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } } }
#define PS_IN6ADDR_LOOPBACK_INIT { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } } }
/*---------------------------------------------------------------------------
 All Hosts Multicast Group FF02::1 General query
---------------------------------------------------------------------------*/
#define PS_MCAST_ALL_HOSTS_IPV6 { { { 0xF,0xF,0,2,0,0,0,0,0,0,0,0,0,0,0,1 } } }
/*---------------------------------------------------------------------------
 All Hosts Multicast Group FF02::2 Done
---------------------------------------------------------------------------*/
#define PS_MCAST_ALL_ROUTERS_IPV6 { { { 0xF,0xF,0,2,0,0,0,0,0,0,0,0,0,0,0,2 } } }


#define PS_IN6_IS_ADDR_UNSPECIFIED(v6)                                      \
  (((const unsigned int *) (v6))[0] == 0 &&                                 \
   ((const unsigned int *) (v6))[1] == 0 &&                                 \
   ((const unsigned int *) (v6))[2] == 0 &&                                 \
   ((const unsigned int *) (v6))[3] == 0)

#define PS_IN6_IS_ADDR_LOOPBACK(v6)                                         \
  (((const unsigned int *) (v6))[0] == 0 &&                                 \
   ((const unsigned int *) (v6))[1] == 0 &&                                 \
   ((const unsigned int *) (v6))[2] == 0 &&                                 \
   ((const unsigned int *) (v6))[3] == ps_htonl(1))


//TODO: Get the correct macro for this.
#define PS_IN6_IS_ADDR_SCOPE_COMPATIBLE(v6_addr1, v6_addr2)    (TRUE)

#define PS_IN6_IS_ADDR_LINKLOCAL(v6)                                        \
  ((((const unsigned int *) (v6))[0] & ps_htonl(0xffc00000)) ==             \
     ps_htonl(0xfe800000))

#define PS_IN6_IS_ADDR_SITELOCAL(v6)                                        \
  ((((const unsigned int *) (v6))[0] & ps_htonl(0xffc00000)) ==             \
     ps_htonl(0xfec00000))

#define PS_IN6_IS_ADDR_V4MAPPED(v6)                                         \
  (((const unsigned int *) (v6))[0] == 0 &&                                 \
   ((const unsigned int *) (v6))[1] == 0 &&                                 \
   ((const unsigned int *) (v6))[2] == ps_htonl(0xffff))

#define PS_IN6_IS_ADDR_V4COMPAT(v6)                                         \
  (((const unsigned int *) (v6))[0] == 0 &&                                 \
   ((const unsigned int *) (v6))[1] == 0 &&                                 \
   ((const unsigned int *) (v6))[2] == 0 &&                                 \
   ps_ntohl(((const unsigned int *) (v6))[3]) > 1)

#define PS_IN6_ARE_ADDR_EQUAL(a,b)                                          \
        ((((const unsigned int *) (a))[0] ==                                \
             ((const unsigned int *) (b))[0])                               \
         && (((const unsigned int *) (a))[1] ==                             \
             ((const unsigned int *) (b))[1])                               \
         && (((const unsigned int *) (a))[2] ==                             \
             ((const unsigned int *) (b))[2])                               \
         && (((const unsigned int *) (a))[3] ==                             \
             ((const unsigned int *) (b))[3]))

#define PS_IN6_GET_V4_FROM_V4_MAPPED_V6(v6)                                 \
  (PS_IN6_IS_ADDR_V4MAPPED(v6)                                              \
     ? ((const unsigned int *)(v6))[3]                                      \
     : 0 )

#define PS_IN6_GET_V4_MAPPED_V6_FROM_V4(v6, v4)                             \
{                                                                           \
  ((unsigned int *)(v6))[0] = 0;                                            \
  ((unsigned int *)(v6))[1] = 0;                                            \
  ((unsigned int *)(v6))[2] = ( v4 != 0) ? ps_htonl(0xFFFF) : 0;            \
  ((unsigned int *)(v6))[3] = v4;                                           \
}

#define PS_IN6_IS_ADDR_V6(v6)                                               \
  (!PS_IN6_IS_ADDR_V4MAPPED(v6) && !PS_IN6_IS_ADDR_UNSPECIFIED(v6))

#define PS_IN6_IS_ADDR_MULTICAST(v6) (((uint8 *) (v6))[0] == 0xFF)

#define PS_IN6_IS_ADDR_MC_LINKLOCAL(v6)                                     \
	(PS_IN6_IS_ADDR_MULTICAST(v6) && ((((uint8 *) (v6))[1] & 0x0F) == 0x2))

#define PS_IN6_IS_ADDR_MC_SITELOCAL(v6)                                     \
	(PS_IN6_IS_ADDR_MULTICAST(v6) && ((((uint8 *) (v6))[1] & 0x0F) == 0x5))

#define PS_IN6_IS_ADDR_MC_GLOBAL(v6)                                        \
	(PS_IN6_IS_ADDR_MULTICAST(v6) && ((((uint8 *) (v6))[1] & 0x0F) == 0xe))

#define PS_IN6_IS_PREFIX_LINKLOCAL(v6)                                      \
  ((ps_htonl(v6) & 0xffff0000) == 0xfe800000)

#define PS_IN6_IS_V4_MAPPED_V6_ADDR_MULTICAST(v6)                           \
  ( PS_IN6_IS_ADDR_V4MAPPED(v6) && PS_IN_IS_ADDR_MULTICAST(((int *)(v6))[3]) )

/*---------------------------------------------------------------------------
  Destination is a multicast address if in the range of
  224.0.0.0 to 239.255.255.255

  i.e. if address starts with 0xE
---------------------------------------------------------------------------*/
#define PS_IN_IS_ADDR_MULTICAST(v4)                                         \
  ((((unsigned long )(v4)) & ps_htonl(0xF0000000)) == ps_htonl(0xE0000000))

/*---------------------------------------------------------------------------
  Destination is a broadcast address if

  a.  Destination address ends with 0xFF (limited broadcast)
  b.  Destination address is the subnet broadcast address
      (which is basically the subnet number with all host bits set to 1)

  THIS MACRO ONLY CHECKS FOR (A).  CALLER MUST CHECK FOR (B).
---------------------------------------------------------------------------*/
#define PS_IN_IS_ADDR_LIMITED_BROADCAST(v4)                                 \
  ((((unsigned long )(v4)) & ps_htonl(0x000000FF)) == ps_htonl(0x000000FF))

/*--------------------------------------------------------------------------
  Upper bound on memory needed for presentation (printable) formats of IPv4
  and IPv6 addresses.  Equal to (sizeof "255.255.255.255") and
  (sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255")
--------------------------------------------------------------------------*/
#define PS_IN_ADDRSTRLEN  16
#define PS_IN6_ADDRSTRLEN 46

#endif /* PS_IN_H */

