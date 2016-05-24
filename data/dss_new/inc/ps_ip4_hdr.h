#ifndef PS_IP4_HDR_H
#define PS_IP4_HDR_H
/*===========================================================================

                         P S _ I  P 4 _ H D R. H

DESCRIPTION
  Internet Protocol version 4 header file. (RFC 791).

EXTERNALIZED FUNCTIONS

  ps_ip4_hdr_parse()  This function is used to parse the IP4 header in the 
                      incoming packet and provides the parsed header as an 
                      output parameter.

  ps_ip4_hdr_create() This function is used to create outgoing IP4 headers for 
                      outgoing IP4 packets. This function creates the 
                      associated IP4 header using the parameters specified by 
                      the caller. 

Copyright (c) 2004-2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================

                            EDIT HISTORY FOR FILE

$Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_ip4_hdr.h#2 $ $DateTime: 2011/07/21 11:01:42 $ $Author: anupamad $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
07/20/11    gs     Removed inclusion of <target.h>
04/17/09    pp     Modified IPv4 header to use UINT32_T(unsigned int) to fix
                   compile warning.
12/28/08    pp     Common Modem Interface: Public/Private API split.
10/22/08    dm     Modified TCP and IP4 headers to fix compiler warnings
07/28/07    rs     Added router_alert field to struct ip.
10/13/04    vp     Created the module.
===========================================================================*/


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                                INCLUDE FILES

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
#include "comdef.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "ps_in.h"
#include "dsm.h"

/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/

#define IP_CS_OLD  1   /* use saved checksum */
#define IP_CS_NEW  0   /* calculate checksum */

/* IP header, INTERNAL representation */

#define IPLEN      20  /* Length of standard IP header */
#define IP_MAXOPT  40  /* Largest option field, bytes */

#define     UDP_ENCAPSULATION_LEN          8

/* Fields in option type byte */

#define OPT_COPIED  0x80  /* Copied-on-fragmentation flag */
#define OPT_CLASS   0x60  /* Option class */
#define OPT_NUMBER  0x1f  /* Option number */

/* IP option numbers */

#define IP_EOL    0      /* End of options list */
#define IP_NOOP   1      /* No Operation */
#define IP_SECURITY  2   /* Security parameters */
#define IP_LSROUTE  3    /* Loose Source Routing */
#define IP_TIMESTAMP  4  /* Internet Timestamp */
#define IP_RROUTE  7     /* Record Route */
#define IP_STREAMID  8   /* Stream ID */
#define IP_SSROUTE  9    /* Strict Source Routing */

/* Timestamp option flags */

#define TS_ONLY   0      /* Time stamps only */
#define TS_ADDRESS  1    /* Addresses + Time stamps */
#define TS_PRESPEC  3    /* Prespecified addresses only */

#define IP_DEF_TTL 255   /* Default TTL value */

/*---------------------------------------------------------------------------
   
                               IP header format

       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |Version|  IHL  |Type of Service|          Total Length         |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |         Identification        |Flags|      Fragment Offset    |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |  Time to Live |    Protocol   |         Header Checksum       |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                       Source Address                          |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                    Destination Address                        |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                    Options                    |    Padding    |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

       IHL - Header Length 
       Flags - Consist of 3 bits
               First bit is kept 0
         Second bit is Dont Fragment bit.
         Third bit is More Fragments bit.

---------------------------------------------------------------------------*/
struct ip
{
  /*-------------------------------------------------------------------------
   ANSI C: Bit wise fields should be defined with "int" type ONLY. 
   AMSS "uint32" uses unsigned long and cannot be used to define Bit fields.
   Current "unit32" definition in comdef.h violates ANSI C rule, hence 
   created new MACRO to define unsigned int
  -------------------------------------------------------------------------*/
#define UINT32_T unsigned int
  UINT32_T         ihl:4;              /*                 IP header length */
  UINT32_T         version:4;          /*                       IP version */
  UINT32_T         tos:8;              /*                  Type of service */
  UINT32_T         length:16;          /*                     Total length */
  UINT32_T         id:16;              /*                   Identification */
  UINT32_T         congest:1;          /* Congestion experienced bit (exp) */
  UINT32_T         df:1;               /*              Don't fragment flag */
  UINT32_T         mf:1;               /*              More Fragments flag */
  UINT32_T         off:13;             /*  Fragment offset in 8 byte units */
  uint8             ttl;               /*                     Time to live */
  uint8             protocol;          /*                         Protocol */
  uint16            checksum;          /*                  Header checksum */
  struct ps_in_addr source;            /*                   Source address */
  struct ps_in_addr dest;              /*              Destination address */
  uint8             options[IP_MAXOPT];/*                    Options field */
  /* Following are not part of IPv4 hdr,included here for processing info. */
  uint16            optlen;            /*   Length of options field, bytes */
  uint16            offset;            /*         Fragment offset in bytes */
  boolean           router_alert;      /* Routers should examine this packet
                                          more closely                     */

};


/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/
/*===========================================================================

FUNCTION     PS_IP4_HDR_PARSE()

DESCRIPTION  This function is used to parse the IP4 header in the 
             incoming packet and provides the parsed header as an output 
             parameter.

DEPENDENCIES None

PARAMETERS   dsm_item_type* - Dsm item containing the new packet.
             uint16*        - Offset which points to the start of packet.
             struct ip*     - Return parameter which gets filled with the 
                        contents of ip header.

RETURN VALUE boolean - TRUE on success
                       FALSE on failure

SIDE EFFECTS The offset gets updated to end of ip header. 
             The header is not actually removed from dsm item.
===========================================================================*/   
boolean ps_ip4_hdr_parse
(
  dsm_item_type     * data,
  uint16            * offset,
  struct ip         * hdr
);

/*===========================================================================

FUNCTION     PS_IP4_HDR_CREATE()

DESCRIPTION  This function is used to create outgoing IP4 headers for 
             outgoing IP4 packets. This function creates the associated IP4 
             header using the parameters specified by the caller.
             
DEPENDENCIES The caller should set offset and optlen fields of struct ip 
             and should leave the off and ihl fields as it is.

PARAMETERS   dsm_item_type** - Dsm item for outgoing packet.
             struct ip*      - IP header which needs to be creted on outgoing
                         packet.
             int             - Flag which tells whether to copy the existing
                               checksum or calculate a new one.

RETURN VALUE boolean TRUE on success
                     FALSE on failure
                         
SIDE EFFECTS None
===========================================================================*/
boolean ps_ip4_hdr_create
(
  dsm_item_type   **data_payload,
  struct ip       *hdr,
  int             cflag
);

#ifdef __cplusplus
}
#endif

#endif /* PS_IP4_HDR_H */
