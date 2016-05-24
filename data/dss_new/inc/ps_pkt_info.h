#ifndef PS_PKT_INFO_H
#define PS_PKT_INFO_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        P S _ P K T _ I N F O . H

DESCRIPTION
  Header containing packet information which needs to be passed through
  layers.

Copyright (c) 2004-2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/


/*===========================================================================

                            EDIT HISTORY FOR FILE

  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_pkt_info.h#2 $ $DateTime: 2011/03/14 14:28:07 $ $Author: maldinge $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
03/11/11    am     Added ICMP ID field to pkt_info struct.
02/24/10    pp     secips_ipsec_info_type moved from SEC to PS as 
                   ps_ipsec_info_type.
11/10/09    ss     IP Multicast feature additions.
12/14/08    pp     Common Modem Interface: Public/Private API split.
10/12/05    mct    Fixed support for sending to v4/v4-map-v6 addrs over a
                   v6 socket.
08/16/05    sv     Added SPI filed to ESP and AH pakcet info for fragment
                   filtering.
05/11/05    sv     Added ESP and AH headers to packet info.
04/20/05    sv     Added fraghdl to pktinfo.
10/13/04    vp     Removed inclusion of ps_iphdr.h and included ps_ip4_hdr.h.
06/11/04    vp     Created the file.
===========================================================================*/

#include "comdef.h"
#include "ps_in.h"
#include "ps_ip4_hdr.h"
#include "ps_ip6_hdr.h"

/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*-   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -*/

/*---------------------------------------------------------------------------
  Pseudo-header for TCP and UDP checksumming
---------------------------------------------------------------------------*/

typedef struct
{
  struct ps_in6_addr source;   /* IP source */
  struct ps_in6_addr dest;     /* IP destination */
  uint16 length;               /* Data field length */
  uint8  protocol;             /* Protocol */
} pseudo_header_type;

/*---------------------------------------------------------------------------
  IP packet information needed by ipfilter (IP header + extra info)
---------------------------------------------------------------------------*/
struct ip_pkt_info_s
{
  /* IP header information from ip.h */
  ip_version_enum_type  ip_vsn;
  union
  {
    struct ip    v4;
    ip6_hdr_type v6;
  } ip_hdr;

  /* Pseudo header info - Source Addr, Dest Addr, length and protocol */
  pseudo_header_type    pseudo_hdr;

  /* optional ESP and AH headers */
  struct
  {
    uint32 spi;
    void* esp_handle;
  } esp_hdr;

  struct
  {
    uint32 spi;
    void* ah_handle;
  } ah_hdr;

  /* Info about protocols above IP */
  union
  {
    struct
    {
      uint8       type;              /* ICMP message type                  */
      uint8       code;              /* ICMP message code                  */
      uint16      id;                /* ICMP ID for echo/reply             */
    } icmp;

    struct
    {
      uint16      src_port;          /* Source UDP port number             */
      uint16      dst_port;          /* Destination UDP port number        */
    } udp;

    struct
    {
      uint16      src_port;          /* Source TCP port number             */
      uint16      dst_port;          /* Destination TCP port number        */
    } tcp;

  } ptcl_info;             /* Protocol specific information      */

  boolean     ipsec_required;        /* Packet need sESP processing        */
  boolean     is_secure;             /* Packet is secured by IPSec         */
  boolean     is_brdcst;             /* Packet has a broadcast destination */
  boolean     is_local;              /* Identifies packets for local host  */
  boolean     is_pkt_info_valid;     /* Used to avoid multiple packet info 
                                        generations */
  boolean     is_mcast_loop;         /* Multicast datagram-looping enabled */
  void*       if_ptr;                /* Iface on which IP pkt is rcv'ed    */
  void*       fraghdl;               /* Fragment handler                   */

};

typedef struct ip_pkt_info_s       ip_pkt_info_type;

/*---------------------------------------------------------------------------
  Per Packet IPSec info - part of Meta Info struct
---------------------------------------------------------------------------*/
#define PS_SECIPS_MAX_TUNNELS 2
#define PS_SECIPS_MAX_IFACE_LIST (PS_SECIPS_MAX_TUNNELS +2)

typedef struct
{
  void*           esp_sa_ptr;
  void*           ah_sa_ptr;
  void*           ipsec_handle;
  void*           iface_list[PS_SECIPS_MAX_IFACE_LIST];
  uint8           iface_cnt;
  ps_ip_addr_type next_gw_addr;
  uint32          ipsec_header_size;
  uint32          user_id;
}ps_ipsec_info_type;

#endif /* PS_PKT_INFO_H */
