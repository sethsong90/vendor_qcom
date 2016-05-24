#ifndef PS_IP6_HDR_H
#define PS_IP6_HDR_H
/*===========================================================================

                         P S _ I  P 6 _ H D R. H

DESCRIPTION
  Internet Protocol (IP) version 6 header file (RFC 2460)

EXTERNALIZED FUNCTIONS
  ps_ip6_hdr_create() This function is used to create outgoing IP6 headers 
                      for outgoing IP6 packets. This function creates the 
                      associated IP6 header using the parameters specified 
                      by the caller.

Copyright (c) 2003-2009 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/


/*===========================================================================

                            EDIT HISTORY FOR FILE

$Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_ip6_hdr.h#1 $
$Author: maldinge $ $DateTime: 2011/01/10 09:44:56 $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
12/14/08   pp      Common Modem Interface: Public/Private API split.
02/07/08   ssh     Added ps_ip6_dest_opt_hdr_create()
11/09/07   ssh     Added support for MIPv6
10/12/05   mct     Fixed support for sending to v4/v4-map-v6 addrs over a
                   v6 socket.
03/02/05   vp      Addition of offset constant defines and IP6_MAXPLEN.
10/15/04   ifk     Added support for fragment header
07/30/04   vp      Declarations for routing header and renaming of 
                   ps_ip6_hdr_parse() to ps_ip6_base_hdr_parse().
06/11/04   vp      Replaced ICMP6_PTCL with PS_IPPROTO_ICMP6
04/20/04   mct     Misc fixes for V6.
04/05/04   sv      Featurized the header file.
08/20/03   aku     Initial version
===========================================================================*/


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                                INCLUDE FILES

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
#include "comdef.h"
#include "ps_in.h"
#include "dsm.h"

/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/
#define IP6LEN      40  /* Length of base IP6 header */
#define IP6_MAXPLEN 65535 /* Max payload length allowed for IPv6 packets  */ 

/*---------------------------------------------------------------------------
  Typedef for different IP6 header types.  
---------------------------------------------------------------------------*/
typedef ps_ip_protocol_enum_type ip6_hdr_enum_type;

/*---------------------------------------------------------------------------
  Typedef for IP6 base header.  
---------------------------------------------------------------------------*/
typedef struct
{
  unsigned int        version:4;             /* 4-bit version field        */
  unsigned int        trf_cls:8;             /* 8-bit traffic class field  */
  unsigned int        flow_cls:20;           /* 20-bit flow class field    */
  uint16              payload_len;           /* 16-bit payload length      */
  byte                next_hdr;              /* 8-bit next header field    */
  byte                hop_limit;             /* 8-bit hop limit field      */
  struct ps_in6_addr  src_addr;              /* 128-bit source address     */
  struct ps_in6_addr  dst_addr;              /* 128-bit destination address*/

} ip6_base_hdr_type;

#define IP6_BASE_HDR_NEXT_HDR_OFFSET 6
#define IP6_BASE_HDR_PAYLOAD_LEN_OFFSET 4

/*---------------------------------------------------------------------------
  Routing Header type.
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |  Next Header  |  Hdr Ext Len  |  Routing Type | Segments Left |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                                                               |
    :                      type-specific data                       :
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

---------------------------------------------------------------------------*/
typedef struct
{
  byte   next_hdr;                 /* Next header after the routing header */
  byte   hdrlen;/* Routing hdr len in 8 octet units excluding 1st 8 octets */
  byte   type;                                   /* Type of routing header */
  byte   segments_left;              /* Number of route segments remaining */
  uint32 reserved;/* Reserved field. Initialized to 0.Ignored on reception */
  /*      Type specific data comes here. Format determined by routing type */
} ip6_routing_hdr_type;

#define IP6_ROUTING_HDR_TYPE_0 0


/*---------------------------------------------------------------------------
  Hop By Hop Header type for router-alert option.
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |  Next Header  |  Hdr Ext Len  |                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+    
    |                                                               |
    :                      options                                  :
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


  Router Alert Option :   
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |0 0 0|0 0 1 0 1|0 0 0 0 0 0 1 0| Value (2 octets) |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                          length = 2

---------------------------------------------------------------------------*/
typedef struct
{
  byte     next_hdr;                /* Next header after hop by hop header */
  byte     hdrlen;               
  byte     opt_type_0;                /* Option Type of the options header */
  byte     opt_type_1;                /* Option Type of the options header */  
  uint16   opt_value;            /* Value field for the router alert option*/                            
  uint16   pad_rtr_alert;        /* 2 byte padding for the router alert option*/ 
} ip6_hopbyhop_hdr_type;  

# define IP6_HOPBYHOP_HDR_TYPE_0 5  
# define IP6_HOPBYHOP_RTR_ALERT_TYPE_1 2

#define IP6_ROUTING_HDR_TYPE_2               2 
#define IP6_ROUTING_HDR_TYPE_2_HDR_EXT_LEN   2 
#define IP6_ROUTING_HDR_TYPE_2_SEG_LEFT      1 

/*---------------------------------------------------------------------------
  Fragment Header type.
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |  Next Header  |  Reserved     |  Fragment Offset        |Res|M|
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                      Identification                           |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

---------------------------------------------------------------------------*/
typedef PACKED struct PACKED_POST ip6_frag_hdr_type
{
  uint8  next_hdr;           /* Header after the fragment header           */
  uint8  reserved;           /* Reserved field. 0 on TX, ignored on RX     */
  uint16 offset_flags;       /* Fragment offset and flags field            */
  uint32 id;                 /* Fragment identification value              */
} ip6_frag_hdr_type;

#define IP6_FRAG_HDR_OFFSET_FIELD_OFFSET 2

/*---------------------------------------------------------------------------
  Destination Options Header Type
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |  Next Header  |  Hdr Ext Len  |                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               +
    |                                                               |
    .                                                               .
    .                            Options                            .
    .                                                               .
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
---------------------------------------------------------------------------*/
typedef struct
{
  byte   next_hdr;                        /* Next header after this header */
  byte   hdrlen;        /* hdr len in 8 octet units excluding 1st 8 octets */
  /* Options come here: in TLV format                                      */
} ip6_dest_opt_hdr_type;

/*---------------------------------------------------------------------------
  Typedef for ip_hdr_enum_type struct
---------------------------------------------------------------------------*/
typedef struct
{
  ip6_hdr_enum_type     hdr_type;
  struct
  {
    ip6_base_hdr_type   base_hdr;
    ip6_hopbyhop_hdr_type     hop_hdr; 
    ip6_routing_hdr_type rt_hdr;
    ip6_frag_hdr_type    frag_hdr;
    ip6_dest_opt_hdr_type  dest_opt_hdr;
  }hdr_body;

} ip6_hdr_type;

/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================

FUNCTION     PS_IP6_HDR_CREATE()

DESCRIPTION  This function is used to create outgoing IP6 headers for 
             outgoing IP6 packets. This function creates the associated IP6 
             header using the parameters specified by the caller.
             
DEPENDENCIES None

RETURN VALUE TRUE on success
             FALSE on failure
            
SIDE EFFECTS None
===========================================================================*/
boolean ps_ip6_hdr_create
(
  dsm_item_type   **data_payload,
  ip6_hdr_type    *hdr
);

#endif /* PS_IP6_HDR_H */
