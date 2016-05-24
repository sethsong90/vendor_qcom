#ifndef PS_IPFLTR_DEFS_H
#define PS_IPFLTR_DEFS_H

/*===========================================================================

                          P S _ I P F L T R _ D E F S . H

DESCRIPTION

  Data structure definition for inbound IP filters

Copyright (c) 2004-2009 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE
                      
  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $PVCSPath: L:/src/asw/MM_DATA/vcs/ps_ipfltr_defs.h_v   1.0   07 Feb 2003 20:14:44   ubabbar  $
  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_ipfltr_defs.h#1 $ $DateTime: 2011/01/10 09:44:56 $ $Author: maldinge $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
11/27/07    hm     Added ESP field for next header protocol
11/10/04    mct    Added more enums to check ranges for QOS.
06/11/04    vp     Included ps_in.h, Changed src_addr and dst_addr in ip_hdr 
                   struct ps_in_addr.
02/18/04    usb    Renamed enum, added next_prot field to the default filter
10/09/03    jd     fixed typo in IP filter type ICMP protocol header field
01/28/03    usb    created file
===========================================================================*/

#include "comdef.h"
#include "ps_in.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                      GLOBAL DATA DECLARATIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/*---------------------------------------------------------------------------
  Type of IP filters, for ACL_T
---------------------------------------------------------------------------*/

typedef enum
{
  IPFLTR_DEFAULT_TYPE = 0,   /* Default filter params and execution rules  */
  IPFLTR_MAX_TYPE
} ipfltr_type_enum_type;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
                       
                       QOS FILTER VALIDATION RULES
                       
=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
/*---------------------------------------------------------------------------
  These are the rules for specifiying a QOS filter spec. If these critera 
  are not met the validation of the filter spec will fail, and QOS
  registration will not succeed.

  This section describes the validation rules and semantics for the IP filter 
  spec parameters.
  1. A bit in field_mask shall be set for each parameter specified, if a bit 
     is not set the corresponding parameter is ignored.
  2. If one or more parameters are incorrrectly specified, the bits in 
     err_mask indicate those parameters on return.  Hence in case of errors, 
     err_mask should be checked to see if a parameter validation error has 
     occurred which will be cleared otherwise.
  3. If a parameter consisting of multiple sub fields is specified, each 
     subfield shall also be specified.  Such parameters are,
       a.IP v4/v6 src and dst addr
       b.IP v4 TOS
       c.IP v6 traffic class
       d.TCP/UDP src and dst port
  4. Each filter will have an IP version associated with it which is either 
     v4 or v6 and is shall be specified by ip_vsn.  It is invalid to specify 
     v6 parameters for a v4 type filter and visa versa. 
  5. Not all combinations of filter parameters are allowed in each filter.  
     The following table describes the valid combinations.  Only those 
     attributes marked with an "X" may be specified for a single packet 
     filter. All marked attributes may be specified, but at least one shall 
     be specified.
             Table 1: Valid Packet Filter Attribute Combinations
                 
           Table 1                       Valid combination types
    Packet filter attribute	                  I   II  III  IV
    Source Address and Subnet Mask	          X   X    X   X
    Destination Address and Subnet Mask	      X   X    X   X
    Protocol Number (IPv4)/Next Header (IPv6)	X	  X    X
    Destination Port Range                    X
    Source Port Range                         X
    ICMP Msg Type                                 X
    ICMP Msg Code                                 X
    IPSec SPI                                          X
    TOS (IPv4)/Traffic Class (IPv6) and Mask	X   X    X   X
    Flow Label (IPv6)                                      X
    
  6. If a parameter from next header protocol (TCP, UDP etc.) is specified in
     a filter, the Protocol Number for v4 filters or Next Header for v6 
     filters shall be specified.
  7. In most cases IP source address in Tx filters and IP destination address
     in Rx filters is not required to be specified.  In case these values are,
     specified following requirements shall be met.       
       a. IP source address in Tx filters and IP destination address in Rx 
          filters can only have a single address value.  Hence subnet_mask 
          (for v4 filters) or prefix_len (for v6 filters) is ignored for 
          these parameters.
       b. These address values shall be same as the current IP address 
          assigned to the MS on the IP interface on which the QOS is being 
          requested.
       c. If the IP address on the interface changes (e.g. during a network 
          handoff), the client which requested QOS  is responsible for 
          updating the filters with the new address or else the flow may not
          be able to receive the desired treatment.
  8. IP destination address in Tx filters and IP source address in Rx filters
     can be specified as address ranges using the subnet_mask (for v4 filters)
     or prefix_len (for v6 filters) parameters.  A subnet_mask may be set to 
     all 1's or prefix_len may be set to 128 to specify a single address 
     value.
  9.A nonzero value for range may be set to specify a range of port numbers 
     starting from the value specified in port [port to port+range], 
     otherwise range shall be set to zero.
  10.A nonzero value for range may be set to specify a range of port numbers 
     starting from the value specified in port [port to port+range], otherwise
     range shall be set to zero.
  11.Certain fields like address, port numbers etc. shall be specified in 
     network byte order, everything else shall be in host byte order.  
     Following fields shall be specified in netwok byte order:
       a.IPv4 addresses
       b.IPv4 subnet masks
       c.IPv6 addresses (prefix length shall be in host order)
       d.TCP and UDP port numbers (port ranges shall be in host order)
       e.IPv6 foe label
---------------------------------------------------------------------------*/


/* IPV4 hdr fields */
typedef enum
{
  IPFLTR_MASK_IP4_NONE          = 0x00,
  IPFLTR_MASK_IP4_SRC_ADDR      = 0x01,
  IPFLTR_MASK_IP4_DST_ADDR      = 0x02,
  IPFLTR_MASK_IP4_NEXT_HDR_PROT = 0x04,
  IPFLTR_MASK_IP4_TOS           = 0x08,
  IPFLTR_MASK_IP4_ALL           = 0x0f
} ipfltr_ip4_hdr_field_mask_enum_type;

/* IPV6 hdr fields */
typedef enum
{
  IPFLTR_MASK_IP6_NONE          = 0x00,
  IPFLTR_MASK_IP6_SRC_ADDR      = 0x01,
  IPFLTR_MASK_IP6_DST_ADDR      = 0x02,
  IPFLTR_MASK_IP6_NEXT_HDR_PROT = 0x04,
  IPFLTR_MASK_IP6_TRAFFIC_CLASS = 0x08,
  IPFLTR_MASK_IP6_FLOW_LABEL    = 0x10,
  IPFLTR_MASK_IP6_ALL           = 0x1f
} ipfltr_ip6_hdr_field_mask_enum_type;
		
/* Higher level protocol hdr parameters */
 
/* TCP hdr fields */
typedef enum
{	
  IPFLTR_MASK_TCP_NONE          = 0x00,
  IPFLTR_MASK_TCP_SRC_PORT      = 0x01,
  IPFLTR_MASK_TCP_DST_PORT      = 0x02,
  IPFLTR_MASK_TCP_ALL           = 0x03
} ipfltr_tcp_hdr_field_mask_enum_type;
  
/* UDP hdr fields */
typedef enum
{	
  IPFLTR_MASK_UDP_NONE          = 0x00,
  IPFLTR_MASK_UDP_SRC_PORT      = 0x01,
  IPFLTR_MASK_UDP_DST_PORT      = 0x02,
  IPFLTR_MASK_UDP_ALL           = 0x03
} ipfltr_udp_hdr_field_mask_enum_type;
    
/* ICMP hdr fields */
typedef enum
{
  IPFLTR_MASK_ICMP_NONE         = 0x00,
  IPFLTR_MASK_ICMP_MSG_TYPE     = 0x01,
  IPFLTR_MASK_ICMP_MSG_CODE     = 0x02,
  IPFLTR_MASK_ICMP_ALL          = 0x03
} ipfltr_icmp_hdr_field_mask_enum_type;

/* ESP hdr fields */
typedef enum
{
  IPFLTR_MASK_ESP_NONE          = 0x00,
  IPFLTR_MASK_ESP_SPI           = 0x01,
  IPFLTR_MASK_ESP_ALL           = 0x01
} ipfltr_esp_hdr_field_mask_enum_type;

/* TCP UDP hdr fields */
typedef enum
{   
  IPFLTR_MASK_TCP_UDP_NONE          = 0x00,
  IPFLTR_MASK_TCP_UDP_SRC_PORT      = 0x01,
  IPFLTR_MASK_TCP_UDP_DST_PORT      = 0x02,
  IPFLTR_MASK_TCP_UDP_ALL           = 0x03
} ipfltr_tcp_udp_hdr_field_mask_enum_type;

typedef uint8 ipfltr_ip4_hdr_field_mask_type;
typedef uint8 ipfltr_ip6_hdr_field_mask_type;
typedef uint8 ipfltr_tcp_hdr_field_mask_type;
typedef uint8 ipfltr_udp_hdr_field_mask_type;
typedef uint8 ipfltr_icmp_hdr_field_mask_type;
typedef uint8 ipfltr_esp_hdr_field_mask_type;
typedef uint8 ipfltr_tcp_udp_hdr_field_mask_type;

/*---------------------------------------------------------------------------
TYPEDEF IP_FILTER_TYPE

DESCRIPTION
  This data structure defines the IP filter parameters for a Default filter
  type. A default filter contains all the common parameters required for most 
  of the filtering needs and are processed by a default set of rules 
  (ie pattern matching on parameters).
  
  All the address/port number fields must be specified in network byte 
  order, everything else in host byte order.
  
  Rules:    

---------------------------------------------------------------------------*/
typedef struct
{
  /* Mandatory Parameter - IP version of the filter (v4 or v6)	*/
  ip_version_enum_type  ip_vsn;

  /* Filter parameter values,  the ones set in field masks are only valid */
  /* Correspopnding err mask is set if a parameter value is invalid */
  union
  {
    struct
    {
      ipfltr_ip4_hdr_field_mask_type      field_mask;  /* In mask   */
      ipfltr_ip4_hdr_field_mask_type      err_mask;    /* Out mask  */

      struct
      {
        struct ps_in_addr  addr;
        struct ps_in_addr  subnet_mask;
      } src;

      struct
      {
        struct ps_in_addr  addr;
        struct ps_in_addr  subnet_mask;
      } dst;
      
      struct
      {
        uint8 val;
        uint8 mask;
      } tos;      
      
      uint8 next_hdr_prot;
    } v4;

    struct
    {
      ipfltr_ip6_hdr_field_mask_type      field_mask;  /* In mask   */
      ipfltr_ip6_hdr_field_mask_type      err_mask;    /* Out mask  */

      struct
      {
        struct ps_in6_addr  addr;
        uint8               prefix_len;
      } src;
      
      struct
      {
        struct ps_in6_addr addr;
        uint8              prefix_len;
      } dst;
      
      struct
      {
        uint8   val;
        uint8   mask;
      } trf_cls;
      
      uint32   flow_label;
      uint8    next_hdr_prot;   /* This is transport level protocol header */
    } v6; 
  } ip_hdr;

  /* next_hdr_prot field in v4 or v6 hdr must be set to specify a    */
  /* parameter from the next_prot_hdr                                */
  union
  {
    struct
    {
      ipfltr_tcp_hdr_field_mask_type      field_mask;  /* In mask   */
      ipfltr_tcp_hdr_field_mask_type      err_mask;    /* Out mask  */

      struct
      {
        uint16    port;
        uint16    range;
      } src;
    
      struct
      {
        uint16    port;
        uint16    range;
      } dst;    
    } tcp;
    
    struct
    {
      ipfltr_udp_hdr_field_mask_type      field_mask;  /* In mask   */
      ipfltr_udp_hdr_field_mask_type      err_mask;    /* Out mask  */

      struct
      {
        uint16    port;
        uint16	  range;
      } src;
      
      struct
      {
        uint16    port;
        uint16    range;
      } dst;
    } udp;
    
    struct
    {    
      ipfltr_icmp_hdr_field_mask_type     field_mask; /* In mask   */
      ipfltr_icmp_hdr_field_mask_type     err_mask;   /* Out mask  */

      uint8   type;
      uint8   code;
    } icmp;
  
    struct
    {
      ipfltr_esp_hdr_field_mask_type     field_mask; /* In mask   */
      ipfltr_esp_hdr_field_mask_type     err_mask;   /* Out mask  */

      uint32  spi;
    } esp;

    struct
    {
      ipfltr_tcp_udp_hdr_field_mask_type      field_mask;  /* In mask   */
      ipfltr_tcp_udp_hdr_field_mask_type      err_mask;    /* Out mask  */
  
      struct
      {
        uint16    port;
        uint16    range;
      } src;
  
      struct
      {
        uint16    port;
        uint16    range;
      } dst; 

    } tcp_udp_port_range;
  } next_prot_hdr;

  struct
  { 
    uint16 fi_id;          /* Filter ID */
    uint16 fi_precedence;  /* Filter precedence */
  } ipfltr_aux_info;

} ip_filter_type;

typedef struct
{
  uint8             num_filters;      /* Num filters in the list     */
  ip_filter_type    *list_ptr;        /* List of filters             */
} ip_filter_spec_type;


#endif /* PS_IPFLTR_DEFS_H */

