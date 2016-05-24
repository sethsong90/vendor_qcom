#ifndef PS_IN_ALIAS_H
#define PS_IN_ALIAS_H
/*===========================================================================

                   P S _ I N _ A L I A S _ H

DESCRIPTION

  This file provides featurized aliases for type names in ps_in.h and
  dssock_defs.h.

  To resolve name conflicts when CDPS stack is built with third-party module
  which also contain data stack (e.g, Linux, Windows), some CDPS data/data
  type names are prefixed with either "ps_/PS_" (in ps_in.h) and "dss_" in
  (dssock_defs.h).

  For example, "struct in_addr" is now "struct ps_in_addr" in ps_in.h.

  Moving forward,the preferred types are the ones such as ps_in_addr, and
  the use of the previous types such as in_addr is deprecated with the
  intention of removing it in the future.

  Note:
    1. Per CMI rule, no feature is allowed in api files
    2. ###--- Special Exception Is Made for This File ---###
        This file is featurized with feature FEATURE_DATA_PS_IN_ALIASES
    3. The reason being:
       a. The aliases in this file are needed for building apps which already
          use types such as "in_addr". For example, 6K targets, BMP targets ...
       b. The approach of using header guards in place of the feature is not
          adopted since it has un-acceptable side effects. The order of
          including CDPS stack and third-party stack matters.

          For example, if we use header guard for "winsock.h": _WINSOCKAPI_ in
          stead of the featuer string here, when build CDPS with Windows
          platform, "windows.h" must be included ahead of "ps_in.h" by the
          user. Otherwise, the type name conflicts remain.

  Usage: For off-target builds,
    a. feature  FEATURE_DATA_PS_IN_ALIASES should be disabled,
    b. the user of the CDPS stack should use names with prefix, e.g.,
       ps_in_addr (see ps_in_alias.h for the affected names).

  On-target CDPS users should use the original version for the affected names.

  Usage: For AMSS target builds,
    a. feature  FEATURE_DATA_PS_IN_ALIASES should be enabled,
    b. the user of the CDPS stack can use names without prefix, e.g., in_addr
       (see ps_in_alias.h for the affected names).

Copyright (c) 2009 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_in_alias.h#1 $
  $Author: maldinge $ $DateTime: 2011/01/10 09:44:56 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
10/13/09    ts     Renamed INET/6_ADDRSTRLEN to PS_IN/6_ADDRSTRLEN
06/24/09    am     Renamed hton/ntoh macros with ps_ prefix.
03/11/09    jy     Changes to avoid ps/ds type name conflicts when build CDPS
                   with third-part software modules containing a data stack
===========================================================================*/


/*===========================================================================

                                INCLUDE FILES

===========================================================================*/
#include "customer.h"       /* Customer Specific Features */
#include "comdef.h"

#ifdef FEATURE_DATA_PS_IN_ALIASES

/*  mapping enums of type ps_ip_protocol_enum_type in ps_in.h */
#define IPV6_BASE_HDR              PS_IPV6_BASE_HDR
#define IPPROTO_HOP_BY_HOP_OPT_HDR PS_IPPROTO_HOP_BY_HOP_OPT_HDR
#define IPPROTO_ICMP               PS_IPPROTO_ICMP
#define IPPROTO_IGMP               PS_IPPROTO_IGMP
#define IPPROTO_IP                 PS_IPPROTO_IP
#define IPPROTO_TCP                PS_IPPROTO_TCP
#define IPPROTO_UDP                PS_IPPROTO_UDP
#define IPPROTO_IPV6               PS_IPPROTO_IPV6
#define IPPROTO_ROUTING_HDR        PS_IPPROTO_ROUTING_HDR
#define IPPROTO_FRAG_HDR           PS_IPPROTO_FRAG_HDR
#define IPPROTO_ESP                PS_IPPROTO_ESP
#define IPPROTO_AH                 PS_IPPROTO_AH
#define IPPROTO_ICMP6              PS_IPPROTO_ICMP6
#define NO_NEXT_HDR                PS_NO_NEXT_HDR
#define IPPROTO_DEST_OPT_HDR       PS_IPPROTO_DEST_OPT_HDR
#define IPPROTO_MOBILITY_HDR       PS_IPPROTO_MOBILITY_HDR
#define ip_protocol_enum_type      ps_ip_protocol_enum_type

/* map struct type and its data memebers in ps_in.h*/
#define in_addr          ps_in_addr            /* struct type */
#define s_addr           ps_s_addr             /* data member */
#define s6_addr          ps_s6_addr
#define s6_addr16        ps_s6_addr16
#define s6_addr32        ps_s6_addr32
#define s6_addr64        ps_s6_addr64

/* map enum in ps_in.h */
#define INADDR_ANY       PS_INADDR_ANY
#define INET_ADDRSTRLEN  PS_IN_ADDRSTRLEN 

/* IN6 mappings: ps_in.h */
#define IN6ADDR_ANY_INIT               PS_IN6ADDR_ANY_INIT
#define IN6ADDR_LOOPBACK_INIT         PS_IN6ADDR_LOOPBACK_INIT
#define IN6_IS_ADDR_UNSPECIFIED       PS_IN6_IS_ADDR_UNSPECIFIED
#define IN6_IS_ADDR_LOOPBACK          PS_IN6_IS_ADDR_LOOPBACK
#define IN6_IS_ADDR_SCOPE_COMPATIBLE PS_IN6_IS_ADDR_SCOPE_COMPATIBLE
#define IN6_IS_ADDR_LINKLOCAL         PS_IN6_IS_ADDR_LINKLOCAL
#define IN6_IS_ADDR_SITELOCAL         PS_IN6_IS_ADDR_SITELOCAL
#define IN6_IS_ADDR_V4MAPPED          PS_IN6_IS_ADDR_V4MAPPED
#define IN6_IS_ADDR_V4COMPAT          PS_IN6_IS_ADDR_V4COMPAT
#define IN6_ARE_ADDR_EQUAL            PS_IN6_ARE_ADDR_EQUAL
#define IN6_GET_V4_FROM_V4_MAPPED_V6 PS_IN6_GET_V4_FROM_V4_MAPPED_V6
#define IN6_GET_V4_MAPPED_V6_FROM_V4 PS_IN6_GET_V4_MAPPED_V6_FROM_V4
#define IN6_IS_ADDR_V6                 PS_IN6_IS_ADDR_V6
#define IN6_IS_ADDR_MULTICAST         PS_IN6_IS_ADDR_MULTICAST
#define IN6_IS_ADDR_MC_LINKLOCAL      PS_IN6_IS_ADDR_MC_LINKLOCAL
#define IN6_IS_ADDR_MC_SITELOCAL      PS_IN6_IS_ADDR_MC_SITELOCAL
#define IN6_IS_ADDR_MC_GLOBAL         PS_IN6_IS_ADDR_MC_GLOBAL
#define IN6_IS_PREFIX_LINKLOCAL       PS_IN6_IS_PREFIX_LINKLOCAL
#define IN6_IS_V4_MAPPED_V6_ADDR_MULTICAST PS_IN6_IS_V4_MAPPED_V6_ADDR_MULTICAST
#define IN_IS_ADDR_MULTICAST          PS_IN_IS_ADDR_MULTICAST
#define IN_IS_ADDR_LIMITED_BROADCAST PS_IN_IS_ADDR_LIMITED_BROADCAST
#define INET6_ADDRSTRLEN              PS_IN6_ADDRSTRLEN 

/* map struct type in ps_in.h */
#define in6_addr         ps_in6_addr

/* map extern global variables in ps_in.h */
#define in6addr_any      ps_in6addr_any
#define in6addr_loopback ps_in6addr_loopback

/* map struct type and its data memebers in ps_in.h */
#define sockaddr_in      ps_sockaddr_in        /* struct type */
#define sin_family       ps_sin_family         /* data member */
#define sin_port         ps_sin_port           /* data member */
#define sin_addr         ps_sin_addr           /* data member */
#define sin_zero         ps_sin_zero           /* data member */

/* map struct type and its data memebers in ps_in.h */
#define sockaddr_in6     ps_sockaddr_in6       /* struct type */
#define sin6_family      ps_sin6_family        /* data member */
#define sin6_port        ps_sin6_port          /* data member */
#define sin6_flowinfo    ps_sin6_flowinfo      /* data member */
#define sin6_addr        ps_sin6_addr          /* data member */
#define sin6_scope_id    ps_sin6_scope_id      /* data member */

/* map struct type and its data memebers in ps_in.h */
#define sockaddr         ps_sockaddr           /* struct type */
#define sa_family        ps_sa_family          /* data member */
#define sa_data          ps_sa_data            /* data member */

/* map struct type and its data memebers in ps_in.h */
#define _SS_MAXSIZE      PS_SS_MAXSIZE
#define _SS_ALIGNSIZE    PS_SS_ALIGNSIZE
#define _SS_PADSIZE      PS_SS_PADSIZE
#define sockaddr_storage ps_sockaddr_storage   /* struct type */
#define _ss_align        ps_ss_align           /* data member */
#define _ss_pad          ps_ss_pad             /* data member */
#define ss_family        ps_ss_family          /* data member */

/* map struct type and its data memebers in ps_in.h */
#define iovec            ps_iovec              /* struct type */
#define iov_base         ps_iov_base           /* data member */
#define iov_len          ps_iov_len            /* data member */

/* map enums in dssock_defs.h */
#define PF_INET          DSS_PF_INET
#define PF_INET6         DSS_PF_INET6

/* map enums in dssock_defs.h */
#define AF_INET          DSS_AF_INET
#define AF_INET6         DSS_AF_INET6
#define AF_UNSPEC        DSS_AF_UNSPEC
#define AF_ANY           DSS_AF_ANY

/* map enums in dssock_defs.h */
#define SOCK_STREAM      DSS_SOCK_STREAM
#define SOCK_DGRAM       DSS_SOCK_DGRAM


/* map hton/ntoh defs in ps_in.h */
#ifndef htonl
#define htonl(x) ps_htonl(x)
#endif /* htonl */

#define htonll(x) ps_htonll(x)

#ifndef htons
#define htons(x) ps_htons(x)
#endif /* htons */

#ifndef ntohl
#define ntohl(x) ps_ntohl(x)
#endif /* htonl */

#define ntohll(x) ps_ntohll(x)

#ifndef ntohs
#define ntohs(x) ps_ntohs(x)
#endif /* htons */

#endif /* FEATURE_DATA_PS_IN_ALIASES */
#endif /* PS_IN_ALIAS_H */
