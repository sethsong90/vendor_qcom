/*===========================================================================

                      P S _ I F A C E _ I P F L T R . C

DESCRIPTION
  This file provides functions to use IP filters for inbound iface specific
  processing of IP packets received on that iface.

EXTERNALIZED FUNCTIONS

  PS_IFACE_IPFLTR_ADD()
    adds filters to the iface

  PS_IFACE_IPFLTR_DELETE()
    deletes filters from the iface

  PS_IFACE_IPFLTR_CONTROL()
    Performs control operation on filters (enable/disable)

  PS_IFACE_IPFLTR_EXECUTE()
    executes filter for the processing inbound IP pkts

  PS_IFACE_IPFLTR_PROCESS_PKT
    Process a raw IP pkt and then performs filtering on the packet

Copyright (c) 2003-2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/netiface/rel/11.03/src/ps_iface_ipfltr.c#1 $
  $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
01/15/11    hs     Made changes to install filter in A2 - port num, IP addr
12/27/10    cp     Fix to process the filter when the next header protocol
                   is set to PS_IPPROTO_TCP_UDP
05/25/10    sy     Featurized inclusion of a2_ipfilter.h
05/13/10    pp     ps_iface_ipfltri_process_default: corrected next_hdr_prot
                   check for IPv6 during filter execution.
06/24/09    am     Renamed hton/ntoh macros with ps_ prefix.
10/01/07    mct    Added support for ICMPv6 filtering.
08/21/2006  rt     Fixed incorrect next protocol field comparison for v6 in
                   ps_iface_ipfltri_process_default().
02/22/06    msr    Using single critical section
02/06/06    msr    Updated for L4 tasklock/crit sections.
12/05/05    msr    Added support for fltr validation
08/16/05    msr    Fixed PS_BRANCH_TASKFREE()
08/15/05    mct    Fixed naming causing some issues w/C++ compilers.
08/03/05    msr    Posting IPFLTR_UPDATED_EV only if fltr status is changed
05/12/05    ks     fixed lint errors.
05/03/05    msr    Using int16 instead of errno_enum_type.
04/20/05    sv     Movied pkt processing code to pkt_info_utils file.
04/19/05    msr    Reverting earlier changes.
04/19/05    msr    Setting fi_result to default flow if packet doesn't match
                   any filter.
04/17/05    msr    Added subset ID and separated filter ID from filter result.
03/25/05    ifk    Added support for IPv6 filtering.
03/09/05    ssh    Enabled fragment filtering, and made minor tweaks to
                   ipfltr_delete() and ipfltr_process_fragment() functions
01/27/05    ssh    Changed include file name dsbyte.h to ps_byte.h
01/26/05    mct    Fixed incorrect q_insert.
01/10/05    sv     Merged IPSEC changes.
01/10/05    ifk    Added support for filtering on fragments.
01/08/05    msr    Added link initialization for Tx IP filters before they
                   are enqueued.
12/06/04    mct    Fixed loop where ipfilter was not being properly executed.
10/13/04    vp     Removal of ps_iface_ipfltri_ntoh() function everything is
                   supposed to be in network order and related changes.
08/02/04    mct    Included ps_ifacei_utils.h for new internal macro use.
06/11/04    vp     Removed inclusion internet.h, psglobal.h and included
                   ps_pkt_info.h. Changes for representation of IP addresses
                   as struct ps_in_addr or struct ps_in6_addr. Replaced use of
                   TCP_PTCL, UDP_PTCL etc with PS_IPPROTO_TCP, PS_IPPROTO_UDP etc.
04/27/04    usb    Added new fns ps_iface_ipfltr_process_pkt(),
                   ps_iface_ipfltr_conrtol(), support for disabling filters
                   without deleting them, API update.
03/23/04    mct    Changed ip_filter_info_type to ip_pkt_info_type.
02/18/04    usb    IP filtering library interface change, client id support,
                   network byte order support for filters, only allow
                   IPV4 pkts and other misc fixes.
10/22/03    mct    Removed dsm.h and included ps_mem.h. Removed old functions
                   PS_IFACE_IPFLTRI_ALLOC_BUF() and
                   PS_IFACE_IPFLTRI_FREE_BUF() and replaced with macros to
                   ps_mem equivalents.
07/22/03    usb    Included rex.h, wrapped q_delete under appropriate
                   feature, incorporated acl fn prototype change.
02/13/03    usb    Not calling ipfltr_delete() while adding filters, instead
                   moved delete code in ipfltr_add() to avoid duplicate event
                   generation.
02/11/03    usb    Modified MSG logs, passing num filters actually
                   added and deleted to ps_iface_ipfltr_updated_ind()
01/28/03    usb    File created.
===========================================================================*/


/*===========================================================================

                       INCLUDE FILES FOR THE MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"

#ifdef FEATURE_DATA_PS

#include "ps_mem.h"
#include "ps_iface.h"
#include "ps_iface_ipfltr.h"
#include "ps_in.h"
#include "ps_ip6_addr.h"
#include "ps_iputil.h"
#include "ps_byte.h"
#include "ps_pkt_info.h"
#include "ps_utils.h"
#include "ps_crit_sect.h"
#include "ps_ifacei_utils.h"
#ifdef FEATURE_DATA_A2_FILTERING
#include "a2_ipfilter.h"
#endif
#include "ds_Utils_DebugMsg.h"


/*===========================================================================

            LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE

This section contains local definitions for constants, macros, types,
variables and other items needed by this module.

===========================================================================*/

/*---------------------------------------------------------------------------
  Defines maximum port that can be opened on a mobile
---------------------------------------------------------------------------*/
#define MAX_PORT  0xFFFF

/*---------------------------------------------------------------------------
  The IP filter information for all ifaces.
---------------------------------------------------------------------------*/
static q_type global_ipfltr_info[IP_FLTR_CLIENT_MAX];

/*---------------------------------------------------------------------------
  Tuning the number of ps iface ip filter buffers needed by this module
---------------------------------------------------------------------------*/
#define PS_IFACE_IPFLTR_BUF_SIZE ((sizeof(ps_iface_ipfilteri_type) + 3) & ~3)

#ifdef FEATURE_DATA_PS_LOW_MEM_CHIPSET
  #define PS_IFACE_IPFLTR_BUF_NUM       20
  #define PS_IFACE_IPFLTR_BUF_HIGH_WM   16
  #define PS_IFACE_IPFLTR_BUF_LOW_WM     5

#else
  #define PS_IFACE_IPFLTR_BUF_NUM       100
  #define PS_IFACE_IPFLTR_BUF_HIGH_WM   80
  #define PS_IFACE_IPFLTR_BUF_LOW_WM    20

#endif /* FEATURE_DATA_PS_LOW_MEM_CHIPSET */

/*----------------------------------------------------------------------------
  Allocate memory to hold ps_iface_ipfltr along with ps_mem header
----------------------------------------------------------------------------*/
static int ps_iface_ipfltr_buf_mem[PS_MEM_GET_TOT_SIZE_OPT
                                   (
                                     PS_IFACE_IPFLTR_BUF_NUM,
                                     PS_IFACE_IPFLTR_BUF_SIZE
                                   )];

#ifdef FEATURE_DATA_PS_MEM_DEBUG
/*----------------------------------------------------------------------------
  Array of pointers used to facilitate easy debugging.  The first one points
  to the ps_mem header and the latter points to ps_iface_ipfltr_buf
----------------------------------------------------------------------------*/
static ps_mem_buf_hdr_type     * ps_iface_ipfltr_buf_hdr[PS_IFACE_IPFLTR_BUF_NUM];
static ps_iface_ipfilteri_type * ps_iface_ipfltr_buf_ptr[PS_IFACE_IPFLTR_BUF_NUM];
#endif /* FEATURE_DATA_PS_MEM_DEBUG */

#ifdef FEATURE_DATA_A2_FILTERING
  #define A2_OFFSET_IPV4_SRC_ADDR		12
  #define A2_OFFSET_IPV4_DST_ADDR		16
  #define A2_OFFSET_IPV4_NEXT_HDR_PROT	9
  #define A2_OFFSET_IPV6_SRC_ADDR		8
  #define A2_OFFSET_IPV6_DST_ADDR		12
  #define A2_OFFSET_TCP_SRC_PORT		0
  #define A2_OFFSET_TCP_DST_PORT		2
  #define A2_OFFSET_UDP_SRC_PORT		0
  #define A2_OFFSET_UDP_DST_PORT		2
  #define A2_OFFSET_ICMP_TYPE		0
  #define A2_OFFSET_ICMP_CODE		1
  #define A2_OFFSET_IPV6_HEADER         40
  #define A2_MASK_ICMP_TYPE             0xFF000000
  #define A2_MASK_ICMP_CODE             0x00FF0000
#endif /*FEATURE_DATA_a2_FILTERING*/

/*===========================================================================

                                    MACROS

===========================================================================*/
/*===========================================================================
MACRO IS_PORT_RANGE_VALID()

DESCRIPTION
  Validates if given range of ports is within maximum permissible port number
  i.e within 65535

  NOTE : port and range needs to be converted to uint32 since otherwise
         check will always be TRUE as values wraparound when it overflows

PARAMETERS
  port  : starting port number in network order
  range : range of ports. port + range gives ending port number

RETURN VALUE
   TRUE  : if port is <= 65535
   FALSE : otherwise
===========================================================================*/
#define IS_PORT_RANGE_VALID(port, range)                                 \
  (((((uint32) ps_ntohs(port)) + ((uint32) (range))) > MAX_PORT) ? FALSE : TRUE)

/*===========================================================================
FUNCTION PS_IFACE_IPFLTRI_ALLOC_BUF()

DESCRIPTION
  This function allocates a memory buffer to contain the IP filter being
  added to the iface.

DEPENDENCIES
  None.

RETURN VALUE
  Ptr to the allocated buffer, NULL if the buffer could not be allocated

SIDE EFFECTS
  None.

===========================================================================*/
#define ps_iface_ipfltri_alloc_buf()                                \
        ps_mem_get_buf(PS_MEM_PS_IFACE_IPFLTER_TYPE)

/*===========================================================================
FUNCTION PS_IFACE_IPFLTRI_FREE_BUF()

DESCRIPTION
  This function frees up the memory buffer coantaining the IP
  filter in the iface.

DEPENDENCIES
  The buffer should have been created using ps_iface_ipfltri_alloc_buf()
  and it should not be on a queue.

RETURN VALUE
  None

SIDE EFFECTS
  None.

===========================================================================*/
#define ps_iface_ipfltri_free_buf( mi_ptr_ptr )                          \
        PS_MEM_FREE(mi_ptr_ptr)



/*===========================================================================

                      INTERNAL FUNCTION DEFINITIONS

===========================================================================*/
/*===========================================================================
FUNCTION PS_IFACE_IPFLTRI_VALIDATE_IPV4_PARAM()

DESCRIPTION
  Validates IPV4 parameters of a filter

PARAMETERS
  client_id     : Filtering client id
  fltr_ptr      : ptr to a filter
  next_hdr_prot : OUT param indicating the higher level protocol

RETURN VALUE
  TRUE  : if parameters are valid
  FALSE : otherwise

DEPENDENCIES
  None

SIDE EFFECTS
  err_mask is updated with appropriate error codes for values and enums
  which failed validation. Because variables are only validated if the enums
  are set in the field masks the only error masks that will be updated will
  be those corresponding to the values set within the field masks.

  If field mask is IPFLTR_MASK_IP4_NONE, errmask is set to
  IPFLTR_MASK_IP4_ALL

  If a bit mask is specified, which is outside the supported range, errmask
  is updated to those bits
===========================================================================*/
static boolean ps_iface_ipfltri_validate_ipv4_param
(
  ps_iface_ipfltr_client_id_enum_type    client_id,
  ip_filter_type                       * fltr_ptr,
  ps_ip_protocol_enum_type             * next_hdr_prot
)
{
  boolean  is_fltr_valid = TRUE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  fltr_ptr->ip_hdr.v4.err_mask = 0;

  if (fltr_ptr->ip_hdr.v4.field_mask == IPFLTR_MASK_IP4_NONE)
  {
    LOG_MSG_ERROR("Invalid IP_V4 field mask: 0x%x",
              fltr_ptr->ip_hdr.v4.field_mask, 0, 0);
    fltr_ptr->ip_hdr.v4.err_mask = IPFLTR_MASK_IP4_ALL;
    return FALSE;
  }

  /*-------------------------------------------------------------------------
    If a bit mask is specified, which is outside the supported range, flag
    error and continue with validation so that other parameters are validated
  -------------------------------------------------------------------------*/
  if (fltr_ptr->ip_hdr.v4.field_mask & ~IPFLTR_MASK_IP4_ALL)
  {
    fltr_ptr->ip_hdr.v4.err_mask =
      (fltr_ptr->ip_hdr.v4.field_mask & ~IPFLTR_MASK_IP4_ALL);
    is_fltr_valid = FALSE;
  }

  /*-----------------------------------------------------------------------
    If Source address is used in a Tx filter or if Destination address is
    used in a Rx filter, make sure that only a single address value is
    specified. Filters are installed per iface and an iface can have only
    one IP address
  -----------------------------------------------------------------------*/
  if (client_id >= IP_FLTR_CLIENT_INPUT_MAX)
  {
    if ((fltr_ptr->ip_hdr.v4.field_mask & IPFLTR_MASK_IP4_SRC_ADDR) &&
        fltr_ptr->ip_hdr.v4.src.subnet_mask.ps_s_addr != 0xFFFFFFFFUL)
    {
      fltr_ptr->ip_hdr.v4.err_mask |= IPFLTR_MASK_IP4_SRC_ADDR;
      is_fltr_valid = FALSE;
    }
  }
  else
  {
    if ((fltr_ptr->ip_hdr.v4.field_mask & IPFLTR_MASK_IP4_DST_ADDR) &&
        fltr_ptr->ip_hdr.v4.dst.subnet_mask.ps_s_addr != 0xFFFFFFFFUL)
    {
      fltr_ptr->ip_hdr.v4.err_mask |= IPFLTR_MASK_IP4_DST_ADDR;
      is_fltr_valid = FALSE;
    }
  }

  /*-------------------------------------------------------------------------
    Make sure that only one of TCP, UDP, or ICMP is specified as higher layer
    protocol in a filter. Also make sure that ICMP filters are not specified
    by QoS clients
  -------------------------------------------------------------------------*/
  if (fltr_ptr->ip_hdr.v4.field_mask & IPFLTR_MASK_IP4_NEXT_HDR_PROT)
  {
    if (fltr_ptr->ip_hdr.v4.next_hdr_prot == PS_IPPROTO_TCP ||
        fltr_ptr->ip_hdr.v4.next_hdr_prot == PS_IPPROTO_UDP ||
        fltr_ptr->ip_hdr.v4.next_hdr_prot == PS_IPPROTO_ICMP ||
        fltr_ptr->ip_hdr.v4.next_hdr_prot == PS_IPPROTO_ICMP6 ||
        fltr_ptr->ip_hdr.v4.next_hdr_prot == PS_IPPROTO_ESP ||
        fltr_ptr->ip_hdr.v4.next_hdr_prot == PS_IPPROTO_TCP_UDP)
    {
      *next_hdr_prot =
        (ps_ip_protocol_enum_type) fltr_ptr->ip_hdr.v4.next_hdr_prot;
    }
    else
    {
      fltr_ptr->ip_hdr.v4.err_mask |= IPFLTR_MASK_IP4_NEXT_HDR_PROT;
      is_fltr_valid = FALSE;
    }
  }

  if ((fltr_ptr->ip_hdr.v4.field_mask & IPFLTR_MASK_IP4_TOS) &&
      fltr_ptr->ip_hdr.v4.tos.mask == 0)
  {
    fltr_ptr->ip_hdr.v4.err_mask |= IPFLTR_MASK_IP4_TOS;
    is_fltr_valid = FALSE;
  }

  if (is_fltr_valid == FALSE)
  {
    LOG_MSG_INFO1( "IP_FLTR V4 field_mask = 0x%x, IP_FLTR V4 err_mask = 0x%x",
             fltr_ptr->ip_hdr.v4.field_mask, fltr_ptr->ip_hdr.v4.err_mask, 0);
  }

  return is_fltr_valid;

} /* ps_iface_ipfltri_validate_ipv4_param() */



/*===========================================================================
FUNCTION PS_IFACE_IPFLTRI_VALIDATE_IPV6_PARAM()

DESCRIPTION
  Validates IPV6 parameters of a filter

PARAMETERS
  client_id     : Filtering client id
  fltr_ptr      : ptr to a filter
  next_hdr_prot : OUT param indicating the higher level protocol

RETURN VALUE
  TRUE  : if parameters are valid
  FALSE : otherwise

DEPENDENCIES
  None

SIDE EFFECTS
  err_mask is updated with appropriate error codes for values and enums
  which failed validation. Because variables are only validated if the enums
  are set in the field masks the only error masks that will be updated will
  be those corresponding to the values set within the field masks.

  If field mask is IPFLTR_MASK_IP6_NONE, errmask is set to
  IPFLTR_MASK_IP6_ALL

  If a bit mask is specified, which is outside the supported range, errmask
  is updated to those bits
===========================================================================*/
static boolean ps_iface_ipfltri_validate_ipv6_param
(
  ps_iface_ipfltr_client_id_enum_type    client_id,
  ip_filter_type                       * fltr_ptr,
  ps_ip_protocol_enum_type             * next_hdr_prot
)
{
  boolean  is_fltr_valid = TRUE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  fltr_ptr->ip_hdr.v6.err_mask = 0;

  if (fltr_ptr->ip_hdr.v6.field_mask == IPFLTR_MASK_IP6_NONE)
  {
    LOG_MSG_ERROR( "Invalid IP_V6 field mask: 0x%x",
              fltr_ptr->ip_hdr.v6.field_mask, 0, 0);
    fltr_ptr->ip_hdr.v6.err_mask = IPFLTR_MASK_IP6_ALL;
    return FALSE;
  }

  /*-------------------------------------------------------------------------
    If a bit mask is specified, which is outside the supported range, flag
    error and continue with validation so that other parameters are validated
  -------------------------------------------------------------------------*/
  if (fltr_ptr->ip_hdr.v6.field_mask & ~IPFLTR_MASK_IP6_ALL)
  {
    fltr_ptr->ip_hdr.v6.err_mask =
      (fltr_ptr->ip_hdr.v6.field_mask & ~IPFLTR_MASK_IP6_ALL);
    is_fltr_valid = FALSE;
  }

  /*-------------------------------------------------------------------------
    Fail if either of source or destination address is V4 mapped V6
  -------------------------------------------------------------------------*/
  if ((fltr_ptr->ip_hdr.v6.field_mask & IPFLTR_MASK_IP6_SRC_ADDR) &&
      PS_IN6_IS_ADDR_V4MAPPED(&fltr_ptr->ip_hdr.v6.src.addr))
  {
    fltr_ptr->ip_hdr.v6.err_mask |= IPFLTR_MASK_IP6_SRC_ADDR;
    is_fltr_valid = FALSE;
  }

  if ((fltr_ptr->ip_hdr.v6.field_mask & IPFLTR_MASK_IP6_DST_ADDR) &&
      PS_IN6_IS_ADDR_V4MAPPED(&fltr_ptr->ip_hdr.v6.dst.addr))
  {
    fltr_ptr->ip_hdr.v6.err_mask |= IPFLTR_MASK_IP6_DST_ADDR;
    is_fltr_valid = FALSE;
  }

  /*-----------------------------------------------------------------------
    If Source address is used in a Tx filter or if Destination address is
    used in a Rx filter, make sure that only a single address value is
    specified. Filters are installed per iface and an iface can have only
    one IP address
  -----------------------------------------------------------------------*/
  if (client_id >= IP_FLTR_CLIENT_INPUT_MAX)
  {
    if ((fltr_ptr->ip_hdr.v6.field_mask & IPFLTR_MASK_IP6_SRC_ADDR) &&
        fltr_ptr->ip_hdr.v6.src.prefix_len != 128)
    {
      fltr_ptr->ip_hdr.v6.err_mask |= IPFLTR_MASK_IP6_SRC_ADDR;
      is_fltr_valid = FALSE;
    }
  }
  else
  {
    if ((fltr_ptr->ip_hdr.v6.field_mask & IPFLTR_MASK_IP6_DST_ADDR) &&
        fltr_ptr->ip_hdr.v6.dst.prefix_len != 128)
    {
      fltr_ptr->ip_hdr.v6.err_mask |= IPFLTR_MASK_IP6_DST_ADDR;
      is_fltr_valid = FALSE;
    }
  }

  /*-------------------------------------------------------------------------
    Make sure that only one of TCP, UDP, or ICMP is specified as higher layer
    protocol in a filter
  -------------------------------------------------------------------------*/
  if (fltr_ptr->ip_hdr.v6.field_mask & IPFLTR_MASK_IP6_NEXT_HDR_PROT)
  {
    if (fltr_ptr->ip_hdr.v6.next_hdr_prot == PS_IPPROTO_TCP ||
        fltr_ptr->ip_hdr.v6.next_hdr_prot == PS_IPPROTO_UDP ||
        fltr_ptr->ip_hdr.v6.next_hdr_prot == PS_IPPROTO_ICMP ||
        fltr_ptr->ip_hdr.v6.next_hdr_prot == PS_IPPROTO_ICMP6 ||
        fltr_ptr->ip_hdr.v6.next_hdr_prot == PS_IPPROTO_ESP ||
        fltr_ptr->ip_hdr.v6.next_hdr_prot == PS_IPPROTO_TCP_UDP)
    {
      *next_hdr_prot =
        (ps_ip_protocol_enum_type) fltr_ptr->ip_hdr.v6.next_hdr_prot;
    }
    else
    {
      fltr_ptr->ip_hdr.v6.err_mask |= IPFLTR_MASK_IP6_NEXT_HDR_PROT;
      is_fltr_valid = FALSE;
    }
  }

  if ((fltr_ptr->ip_hdr.v6.field_mask & IPFLTR_MASK_IP6_TRAFFIC_CLASS) &&
      fltr_ptr->ip_hdr.v6.trf_cls.mask == 0)
  {
    fltr_ptr->ip_hdr.v6.err_mask |= IPFLTR_MASK_IP6_TRAFFIC_CLASS;
    is_fltr_valid = FALSE;
  }

  /*-------------------------------------------------------------------------
    IPV6 flow label already specifies flow ID. No need to further classify
    this packet
  -------------------------------------------------------------------------*/
  if ((fltr_ptr->ip_hdr.v6.field_mask & IPFLTR_MASK_IP6_FLOW_LABEL) &&
      (*next_hdr_prot != PS_NO_NEXT_HDR))
  {
    fltr_ptr->ip_hdr.v6.err_mask |= (IPFLTR_MASK_IP6_NEXT_HDR_PROT |
                                     IPFLTR_MASK_IP6_FLOW_LABEL);
    is_fltr_valid = FALSE;
  }

  if (is_fltr_valid == FALSE)
  {
    LOG_MSG_INFO1( "IP_FLTR V6 field_mask = 0x%x, IP_FLTR V6 err_mask = 0x%x",
             fltr_ptr->ip_hdr.v6.field_mask, fltr_ptr->ip_hdr.v6.err_mask, 0);
  }

  return is_fltr_valid;

} /* ps_iface_ipfltri_validate_ipv6_param() */



/*===========================================================================
FUNCTION PS_IFACE_IPFLTRI_VALIDATE_TCP_PARAM()

DESCRIPTION
  Validates TCP parameters of a filter

PARAMETERS
  client_id : Filtering client id
  fltr_ptr  : ptr to a filter

RETURN VALUE
  TRUE  : if parameters are valid
  FALSE : otherwise

DEPENDENCIES
  None

SIDE EFFECTS
  err_mask is updated with appropriate error codes for values and enums
  which failed validation. Because variables are only validated if the enums
  are set in the field masks the only error masks that will be updated will
  be those corresponding to the values set within the field masks.

  If a bit mask is specified, which is outside the supported range, errmask
  is updated to those bits
===========================================================================*/
static boolean ps_iface_ipfltri_validate_tcp_param
(
  ps_iface_ipfltr_client_id_enum_type    client_id,
  ip_filter_type                       * fltr_ptr
)
{
  boolean  is_fltr_valid = TRUE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  fltr_ptr->next_prot_hdr.tcp.err_mask = 0;

  /*-------------------------------------------------------------------------
    If a bit mask is specified, which is outside the supported range, flag
    error and continue with validation so that other parameters are validated
  -------------------------------------------------------------------------*/
  if (fltr_ptr->next_prot_hdr.tcp.field_mask & ~IPFLTR_MASK_TCP_ALL)
  {
    fltr_ptr->next_prot_hdr.tcp.err_mask =
      (fltr_ptr->next_prot_hdr.tcp.field_mask & ~IPFLTR_MASK_TCP_ALL);
    is_fltr_valid = FALSE;
  }

  /*-------------------------------------------------------------------------
    TCP ports must be within the range (0, 65535]
  -------------------------------------------------------------------------*/
  if ((fltr_ptr->next_prot_hdr.tcp.field_mask & IPFLTR_MASK_TCP_SRC_PORT) &&
      (fltr_ptr->next_prot_hdr.tcp.src.port == 0 ||
       !IS_PORT_RANGE_VALID(fltr_ptr->next_prot_hdr.tcp.src.port,
                            fltr_ptr->next_prot_hdr.tcp.src.range)))
  {
    fltr_ptr->next_prot_hdr.tcp.err_mask |= IPFLTR_MASK_TCP_SRC_PORT;
    is_fltr_valid = FALSE;
  }

  if ((fltr_ptr->next_prot_hdr.tcp.field_mask & IPFLTR_MASK_TCP_DST_PORT) &&
      (fltr_ptr->next_prot_hdr.tcp.dst.port == 0 ||
       !IS_PORT_RANGE_VALID(fltr_ptr->next_prot_hdr.tcp.dst.port,
                            fltr_ptr->next_prot_hdr.tcp.dst.range)))
  {
    fltr_ptr->next_prot_hdr.tcp.err_mask |= IPFLTR_MASK_TCP_DST_PORT;
    is_fltr_valid = FALSE;
  }

  if (is_fltr_valid == FALSE)
  {
    LOG_MSG_INFO1("IP_FLTR TCP field_mask = 0x%x on IP ver: %d, "
             "IP_FLTR TCP err_mask = 0x%x",
             fltr_ptr->next_prot_hdr.tcp.field_mask,
             fltr_ptr->ip_vsn,
             fltr_ptr->next_prot_hdr.tcp.err_mask);
  }

  return is_fltr_valid;

} /* ps_iface_ipfltri_validate_tcp_param() */



/*===========================================================================
FUNCTION PS_IFACE_IPFLTRI_VALIDATE_UDP_PARAM()

DESCRIPTION
  Validates UDP parameters of a filter

PARAMETERS
  client_id : Filtering client id
  fltr_ptr  : ptr to a filter

RETURN VALUE
  TRUE  : if parameters are valid
  FALSE : otherwise

DEPENDENCIES
  None

SIDE EFFECTS
  err_mask is updated with appropriate error codes for values and enums
  which failed validation. Because variables are only validated if the enums
  are set in the field masks the only error masks that will be updated will
  be those corresponding to the values set within the field masks.

  If a bit mask is specified, which is outside the supported range, errmask
  is updated to those bits
===========================================================================*/
static boolean ps_iface_ipfltri_validate_udp_param
(
  ps_iface_ipfltr_client_id_enum_type    client_id,
  ip_filter_type                       * fltr_ptr
)
{
  boolean  is_fltr_valid = TRUE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  fltr_ptr->next_prot_hdr.udp.err_mask = 0;

  /*-------------------------------------------------------------------------
    If a bit mask is specified, which is outside the supported range, flag
    error and continue with validation so that other parameters are validated
  -------------------------------------------------------------------------*/
  if (fltr_ptr->next_prot_hdr.udp.field_mask & ~IPFLTR_MASK_UDP_ALL)
  {
    fltr_ptr->next_prot_hdr.udp.err_mask =
      (fltr_ptr->next_prot_hdr.udp.field_mask & ~IPFLTR_MASK_UDP_ALL);
    is_fltr_valid = FALSE;
  }

  /*-------------------------------------------------------------------------
    UDP ports must be within the range (0, 65535]
  -------------------------------------------------------------------------*/
  if ((fltr_ptr->next_prot_hdr.udp.field_mask & IPFLTR_MASK_UDP_SRC_PORT) &&
      (fltr_ptr->next_prot_hdr.udp.src.port == 0 ||
       !IS_PORT_RANGE_VALID(fltr_ptr->next_prot_hdr.udp.src.port,
                            fltr_ptr->next_prot_hdr.udp.src.range)))
  {
    fltr_ptr->next_prot_hdr.udp.err_mask |= IPFLTR_MASK_UDP_SRC_PORT;
    is_fltr_valid = FALSE;
  }

  if ((fltr_ptr->next_prot_hdr.udp.field_mask & IPFLTR_MASK_UDP_DST_PORT) &&
      (fltr_ptr->next_prot_hdr.udp.dst.port == 0 ||
       !IS_PORT_RANGE_VALID(fltr_ptr->next_prot_hdr.udp.dst.port,
                            fltr_ptr->next_prot_hdr.udp.dst.range)))
  {
    fltr_ptr->next_prot_hdr.udp.err_mask |= IPFLTR_MASK_UDP_DST_PORT;
    is_fltr_valid = FALSE;
  }

  if (is_fltr_valid == FALSE)
  {
    LOG_MSG_INFO1("IP_FLTR UDP field_mask = 0x%x on IP ver: %d, "
             "IP_FLTR UDP err_mask = 0x%x",
             fltr_ptr->next_prot_hdr.udp.field_mask,
             fltr_ptr->ip_vsn,
             fltr_ptr->next_prot_hdr.udp.err_mask);
  }

  return is_fltr_valid;

} /* ps_iface_ipfltri_validate_udp_param() */



/*===========================================================================
FUNCTION PS_IFACE_IPFLTRI_VALIDATE_ICMP_PARAM()

DESCRIPTION
  Validates the ICMP parameters of a filter

PARAMETERS
  client_id : Filtering client id
  fltr_ptr  : ptr to a filter

RETURN VALUE
   TRUE  : if parameters are valid
   FALSE : otherwise

DEPENDENCIES
  None

SIDE EFFECTS
  err_mask is updated with the appropriate error codes for values and enums
  which failed validation.

  If a bit mask, which is outside the supported range, is specified, errmask
  is updated to those bits
===========================================================================*/
static boolean ps_iface_ipfltri_validate_icmp_param
(
  ps_iface_ipfltr_client_id_enum_type    client_id,
  ip_filter_type                       * fltr_ptr
)
{
  boolean  is_fltr_valid = TRUE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  fltr_ptr->next_prot_hdr.icmp.err_mask = 0;

  /*-------------------------------------------------------------------------
    If a bit mask, which is outside the supported range, is specified flag
    error continue with validation so that other parameters are validated
  -------------------------------------------------------------------------*/
  if (fltr_ptr->next_prot_hdr.icmp.field_mask & ~IPFLTR_MASK_ICMP_ALL)
  {
    fltr_ptr->next_prot_hdr.icmp.err_mask =
      (fltr_ptr->next_prot_hdr.icmp.field_mask & ~IPFLTR_MASK_ICMP_ALL);
    is_fltr_valid = FALSE;
  }

  if (is_fltr_valid == FALSE)
  {
    LOG_MSG_INFO1("IP_FLTR ICMP field_mask = 0x%x on IP ver: %d, "
             "IP_FLTR ICMP err_mask = 0x%x",
             fltr_ptr->next_prot_hdr.icmp.field_mask,
             fltr_ptr->ip_vsn,
             fltr_ptr->next_prot_hdr.icmp.err_mask);
  }

  return is_fltr_valid;

} /* ps_iface_ipfltri_validate_icmp_param() */


/*===========================================================================
FUNCTION PS_IFACE_IPFLTRI_VALIDATE_TCP_UDP_PARAM()

DESCRIPTION
  Validates Transport parameters of a filter when the next header is not
  specified.

PARAMETERS
  client_id : Filtering client id
  fltr_ptr  : ptr to a filter

RETURN VALUE
  TRUE  : if parameters are valid
  FALSE : otherwise

DEPENDENCIES
  None

SIDE EFFECTS
  err_mask is updated with appropriate error codes for values and enums
  which failed validation. Because variables are only validated if the enums
  are set in the field masks the only error masks that will be updated will
  be those corresponding to the values set within the field masks.

  If a bit mask is specified, which is outside the supported range, errmask
  is updated to those bits
===========================================================================*/
static boolean ps_iface_ipfltri_validate_tcp_udp_param
(
  ps_iface_ipfltr_client_id_enum_type    client_id,
  ip_filter_type                       * fltr_ptr
)
{
  boolean  is_fltr_valid = TRUE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  fltr_ptr->next_prot_hdr.tcp_udp_port_range.err_mask = 0;

  /*-------------------------------------------------------------------------
    If a bit mask is specified, which is outside the supported range, flag
    error and continue with validation so that other parameters are validated
  -------------------------------------------------------------------------*/
  if (fltr_ptr->next_prot_hdr.tcp_udp_port_range.field_mask &
        ~IPFLTR_MASK_TCP_UDP_ALL)
  {
    fltr_ptr->next_prot_hdr.tcp_udp_port_range.err_mask =
      (fltr_ptr->next_prot_hdr.tcp_udp_port_range.field_mask &
       ~IPFLTR_MASK_TCP_UDP_ALL);
    is_fltr_valid = FALSE;
  }

  /*-------------------------------------------------------------------------
    All ports must be within the range (0, 65535]
  -------------------------------------------------------------------------*/
  if ((fltr_ptr->next_prot_hdr.tcp_udp_port_range.field_mask &
       IPFLTR_MASK_TCP_UDP_SRC_PORT) &&
      (fltr_ptr->next_prot_hdr.tcp_udp_port_range.src.port == 0 ||
       !IS_PORT_RANGE_VALID(
         fltr_ptr->next_prot_hdr.tcp_udp_port_range.src.port,
         fltr_ptr->next_prot_hdr.tcp_udp_port_range.src.range)))
  {
    fltr_ptr->next_prot_hdr.tcp_udp_port_range.err_mask |=
      IPFLTR_MASK_TCP_UDP_SRC_PORT;
    is_fltr_valid = FALSE;
  }

  if ((fltr_ptr->next_prot_hdr.tcp_udp_port_range.field_mask &
       IPFLTR_MASK_TCP_UDP_DST_PORT) &&
      (fltr_ptr->next_prot_hdr.tcp_udp_port_range.dst.port == 0 ||
       !IS_PORT_RANGE_VALID(
         fltr_ptr->next_prot_hdr.tcp_udp_port_range.dst.port,
         fltr_ptr->next_prot_hdr.tcp_udp_port_range.dst.range)))
  {
    fltr_ptr->next_prot_hdr.tcp_udp_port_range.err_mask |=
      IPFLTR_MASK_TCP_UDP_DST_PORT;
    is_fltr_valid = FALSE;
  }

  if (is_fltr_valid == FALSE)
  {
    LOG_MSG_INFO1("IP_FLTR TRANSPORT PROTOCOL field_mask = 0x%x on IP ver: %d, "
             "IP_FLTR TRANSPORT PROTOCOL err_mask = 0x%x",
             fltr_ptr->next_prot_hdr.tcp_udp_port_range.field_mask,
             fltr_ptr->ip_vsn,
             fltr_ptr->next_prot_hdr.tcp_udp_port_range.err_mask);
  }

  return is_fltr_valid;

} /* ps_iface_ipfltri_validate_tcp_udp_param() */


/*===========================================================================
FUNCTION PS_IFACE_IPFLTRI_PROCESS_DEFAULT()

DESCRIPTION
  This function executes a specified filter on a specified IP pkt information
  block using the default set of rules.

DEPENDENCIES
  - Filter and pakcet must have the same IP version.
  - If a field in the next hdr is specified as a filter parameter,
    next_hdr_prot field in the IP hdr must be set. This means that ip hdr
    field mask will always be non null since filter should have at least
    one parameter specified.

RETURN VALUE
  TRUE for a successful match
  FALSE for no match

SIDE EFFECTS
  None.

===========================================================================*/
static boolean ps_iface_ipfltri_process_default
(
  ip_filter_type       *fi_ptr,          /* ptr to filter to process       */
  ip_pkt_info_type     *info_ptr         /* ptr to info to apply filter on */
)
{
  uint32 tmp_mask;
  uint8 next_hdr_prot = 0;
  uint8 tmp_prot;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == fi_ptr || NULL == info_ptr)
  {
    LOG_MSG_ERROR( "Bad args: fi_ptr 0x%p, info_ptr 0x%p", fi_ptr, info_ptr, 0);
    ASSERT( 0 );
    return FALSE;
  }

  if (fi_ptr->ip_vsn != info_ptr->ip_vsn)
  {
    LOG_MSG_ERROR( "Filter address family %d differs from pkt address family %d",
              fi_ptr->ip_vsn, info_ptr->ip_vsn, 0);
    ASSERT( 0 );
    return FALSE;
  }

  /*-------------------------------------------------------------------------
    Algorithm:
    - Based on the IP version process corresponding IP hdr parameters.
    - If parameters for higher level protocol are specified, v4 protocol
    field or v6 next header field must be set in the filter.  Extract next
    header protocol value and amke sure it matches with that of the packet.
    - Next process the next header protocol parameters

    If any parameter specified in the filter fails a match, fail the filter
    execution.  If all specified filter parametrs match the pkt, filter
    is passed.
  -------------------------------------------------------------------------*/

  /*-------------------------------------------------------------------------
    Note the while loop over a tmp_mask for IP hdr processing as an
    optimization.  More likely parameters should be checked first inside
    the while loop.
  -------------------------------------------------------------------------*/

  switch (fi_ptr->ip_vsn)
  {
    case IP_V4:
    {
      tmp_mask = fi_ptr->ip_hdr.v4.field_mask;

      while (tmp_mask)
      {
        if (tmp_mask & IPFLTR_MASK_IP4_SRC_ADDR)
        {
          if ((fi_ptr->ip_hdr.v4.src.addr.ps_s_addr &
              fi_ptr->ip_hdr.v4.src.subnet_mask.ps_s_addr) !=
             (info_ptr->ip_hdr.v4.source.ps_s_addr &
              fi_ptr->ip_hdr.v4.src.subnet_mask.ps_s_addr))
          {
            return FALSE;
          }

          tmp_mask &= ~IPFLTR_MASK_IP4_SRC_ADDR;
          continue;
        }

        if (tmp_mask & IPFLTR_MASK_IP4_DST_ADDR)
        {
          if ((fi_ptr->ip_hdr.v4.dst.addr.ps_s_addr &
              fi_ptr->ip_hdr.v4.dst.subnet_mask.ps_s_addr) !=
             (info_ptr->ip_hdr.v4.dest.ps_s_addr &
              fi_ptr->ip_hdr.v4.dst.subnet_mask.ps_s_addr))
          {
            return FALSE;
          }

          tmp_mask &= ~IPFLTR_MASK_IP4_DST_ADDR;
          continue;
        }

        if (tmp_mask & IPFLTR_MASK_IP4_NEXT_HDR_PROT)
        {
          if (fi_ptr->ip_hdr.v4.next_hdr_prot == PS_IPPROTO_TCP_UDP)
          {
            if (info_ptr->ip_hdr.v4.protocol != PS_IPPROTO_TCP &&
                info_ptr->ip_hdr.v4.protocol != PS_IPPROTO_UDP)
            {
              return FALSE;
            }
          }
          else if (fi_ptr->ip_hdr.v4.next_hdr_prot !=
                     info_ptr->ip_hdr.v4.protocol)
          {
            return FALSE;
          }

          next_hdr_prot = fi_ptr->ip_hdr.v4.next_hdr_prot;

          tmp_mask &= ~IPFLTR_MASK_IP4_NEXT_HDR_PROT;
          continue;
        }

        if (tmp_mask & IPFLTR_MASK_IP4_TOS)
        {
          if ((fi_ptr->ip_hdr.v4.tos.val & fi_ptr->ip_hdr.v4.tos.mask) !=
             (info_ptr->ip_hdr.v4.tos & fi_ptr->ip_hdr.v4.tos.mask))
          {
            return FALSE;
          }

          tmp_mask &= ~IPFLTR_MASK_IP4_TOS;
          continue;
        }

        if (0 != tmp_mask)
        {
          LOG_MSG_ERROR( "Unknown v4 filter param in mask 0x%x", tmp_mask,0,0);
        }
      }
    }
    break;

#ifdef FEATURE_DATA_PS_IPV6
    case IP_V6:
    {
      tmp_mask = fi_ptr->ip_hdr.v6.field_mask;
      while (tmp_mask)
      {
        if (tmp_mask & IPFLTR_MASK_IP6_SRC_ADDR)
        {
          if (FALSE == IN6_ARE_PREFIX_EQUAL
                       (
                         &fi_ptr->ip_hdr.v6.src.addr,
                         &info_ptr->ip_hdr.v6.hdr_body.base_hdr.src_addr,
                         fi_ptr->ip_hdr.v6.src.prefix_len)
                       )
          {
            return FALSE;
          }

          tmp_mask &= ~IPFLTR_MASK_IP6_SRC_ADDR;
          continue;
        }

        if (tmp_mask & IPFLTR_MASK_IP6_DST_ADDR)
        {
          if (FALSE == IN6_ARE_PREFIX_EQUAL
                       (
                         &fi_ptr->ip_hdr.v6.dst.addr,
                         &info_ptr->ip_hdr.v6.hdr_body.base_hdr.dst_addr,
                         fi_ptr->ip_hdr.v6.dst.prefix_len)
                       )
          {
            return FALSE;
          }

          tmp_mask &= ~IPFLTR_MASK_IP6_DST_ADDR;
          continue;
        }

        /*-------------------------------------------------------------------
          Next hdr for IPv6 is transport protocol type
        -------------------------------------------------------------------*/
        if (tmp_mask & IPFLTR_MASK_IP6_NEXT_HDR_PROT)
        {
          if (fi_ptr->ip_hdr.v6.next_hdr_prot == PS_IPPROTO_TCP_UDP)
          {
            if (info_ptr->ip_hdr.v6.hdr_type != PS_IPPROTO_TCP &&
                info_ptr->ip_hdr.v6.hdr_type != PS_IPPROTO_UDP)
            {
              return FALSE;
            }
          }
          else if (fi_ptr->ip_hdr.v6.next_hdr_prot !=
                     info_ptr->ip_hdr.v6.hdr_type)
          {
            return FALSE;
          }

          next_hdr_prot = fi_ptr->ip_hdr.v6.next_hdr_prot;
          tmp_mask &= ~IPFLTR_MASK_IP6_NEXT_HDR_PROT;
          continue;
        }

        if (tmp_mask & IPFLTR_MASK_IP6_TRAFFIC_CLASS)
        {
          if ((fi_ptr->ip_hdr.v6.trf_cls.val &
                 fi_ptr->ip_hdr.v6.trf_cls.mask) !=
              (info_ptr->ip_hdr.v6.hdr_body.base_hdr.trf_cls &
                 fi_ptr->ip_hdr.v6.trf_cls.mask))
          {
            return FALSE;
          }

          tmp_mask &= ~IPFLTR_MASK_IP6_TRAFFIC_CLASS;
          continue;
        }

        if (tmp_mask & IPFLTR_MASK_IP6_FLOW_LABEL)
        {
          if (fi_ptr->ip_hdr.v6.flow_label !=
               info_ptr->ip_hdr.v6.hdr_body.base_hdr.flow_cls)
          {
            return FALSE;
          }

          tmp_mask &= ~IPFLTR_MASK_IP6_FLOW_LABEL;
          continue;
        }

        if (0 != tmp_mask)
        {
          LOG_MSG_ERROR( "Unknown v6 filter param in mask 0x%x", tmp_mask,0,0);
        }
      }
    }
    break;
#endif /* FEATURE_DATA_PS_IPV6 */

    default:
      /* unsupported IP version */
      ASSERT(0);
      return FALSE;
  }

  if (next_hdr_prot == 0)
  {
    return TRUE;
  }

  /*-------------------------------------------------------------------------
    Optimization done for IP hdrs above may also be used for protocol hdr
    processing when more than 2 parameters can be specified for a protocol.
    For 2 or less parameters the scheme is less optimal and hence avoided.
  -------------------------------------------------------------------------*/
  switch (next_hdr_prot)
  {
    case PS_IPPROTO_TCP:
    {
      if (0 == (tmp_mask = fi_ptr->next_prot_hdr.tcp.field_mask))
      {
        break;
      }

      if ((tmp_mask & IPFLTR_MASK_TCP_SRC_PORT) &&

         ((ps_ntohs(fi_ptr->next_prot_hdr.tcp.src.port)) >
            (ps_ntohs(info_ptr->ptcl_info.tcp.src_port)) ||

          (ps_ntohs(fi_ptr->next_prot_hdr.tcp.src.port)) +
            fi_ptr->next_prot_hdr.tcp.src.range <
            (ps_ntohs(info_ptr->ptcl_info.tcp.src_port))))
      {
        return FALSE;
      }

      if ((tmp_mask & IPFLTR_MASK_TCP_DST_PORT) &&

         ((ps_ntohs(fi_ptr->next_prot_hdr.tcp.dst.port)) >
            (ps_ntohs(info_ptr->ptcl_info.tcp.dst_port)) ||

          (ps_ntohs(fi_ptr->next_prot_hdr.tcp.dst.port)) +
            fi_ptr->next_prot_hdr.tcp.dst.range <
            (ps_ntohs(info_ptr->ptcl_info.tcp.dst_port))))
      {
        return FALSE;
      }
    }

    break;

    case PS_IPPROTO_UDP:
    {
      if (0 == (tmp_mask = fi_ptr->next_prot_hdr.udp.field_mask))
      {
        break;
      }

      if ((tmp_mask & IPFLTR_MASK_UDP_SRC_PORT) &&

         ((ps_ntohs(fi_ptr->next_prot_hdr.udp.src.port)) >
            (ps_ntohs(info_ptr->ptcl_info.udp.src_port)) ||

          (ps_ntohs(fi_ptr->next_prot_hdr.udp.src.port)) +
            fi_ptr->next_prot_hdr.udp.src.range <
            (ps_ntohs(info_ptr->ptcl_info.udp.src_port))))
      {
        return FALSE;
      }

      if ((tmp_mask & IPFLTR_MASK_UDP_DST_PORT) &&

         ((ps_ntohs(fi_ptr->next_prot_hdr.udp.dst.port)) >
            (ps_ntohs(info_ptr->ptcl_info.udp.dst_port)) ||

          (ps_ntohs(fi_ptr->next_prot_hdr.udp.dst.port)) +
            fi_ptr->next_prot_hdr.udp.dst.range <
            (ps_ntohs(info_ptr->ptcl_info.udp.dst_port))))
      {
        return FALSE;
      }
    }

    break;

    case PS_IPPROTO_ICMP:
    case PS_IPPROTO_ICMP6:
    {
      if (0 == (tmp_mask = fi_ptr->next_prot_hdr.icmp.field_mask))
      {
        break;
      }

      if (tmp_mask & IPFLTR_MASK_ICMP_MSG_TYPE &&
         fi_ptr->next_prot_hdr.icmp.type != info_ptr->ptcl_info.icmp.type)
      {
        return FALSE;
      }

      if (tmp_mask & IPFLTR_MASK_ICMP_MSG_CODE &&
         fi_ptr->next_prot_hdr.icmp.code != info_ptr->ptcl_info.icmp.code)
      {
        return FALSE;
      }
    }

    break;

    case PS_IPPROTO_ESP:
      /* TODO */
      break;

    case PS_IPPROTO_TCP_UDP:
    {
      if (0 == (tmp_mask = fi_ptr->next_prot_hdr.tcp_udp_port_range.field_mask))
      {
        break;
      }

      if (fi_ptr->ip_vsn == IP_V4)
      {
        tmp_prot = info_ptr->ip_hdr.v4.protocol;
      }
      else
      {
        tmp_prot = info_ptr->ip_hdr.v6.hdr_type;
      }

      switch (tmp_prot)
      {
        case PS_IPPROTO_TCP:
          if ((tmp_mask & IPFLTR_MASK_TCP_UDP_SRC_PORT) &&

             ((ps_ntohs(fi_ptr->next_prot_hdr.tcp_udp_port_range.src.port)) >
                (ps_ntohs(info_ptr->ptcl_info.tcp.src_port)) ||

              (ps_ntohs(fi_ptr->next_prot_hdr.tcp_udp_port_range.src.port)) +
                fi_ptr->next_prot_hdr.tcp_udp_port_range.src.range <
                (ps_ntohs(info_ptr->ptcl_info.tcp.src_port))))
          {
            return FALSE;
          }

          if ((tmp_mask & IPFLTR_MASK_TCP_UDP_DST_PORT) &&

             ((ps_ntohs(fi_ptr->next_prot_hdr.tcp_udp_port_range.dst.port)) >
              (ps_ntohs(info_ptr->ptcl_info.tcp.dst_port)) ||

              (ps_ntohs(fi_ptr->next_prot_hdr.tcp_udp_port_range.dst.port)) +
                fi_ptr->next_prot_hdr.tcp_udp_port_range.dst.range <
                (ps_ntohs(info_ptr->ptcl_info.tcp.dst_port))))
          {
            return FALSE;
          }
        break;

        case PS_IPPROTO_UDP:
          if ((tmp_mask & IPFLTR_MASK_TCP_UDP_SRC_PORT) &&

             ((ps_ntohs(fi_ptr->next_prot_hdr.tcp_udp_port_range.src.port)) >
                (ps_ntohs(info_ptr->ptcl_info.udp.src_port)) ||

              (ps_ntohs(fi_ptr->next_prot_hdr.tcp_udp_port_range.src.port)) +
                fi_ptr->next_prot_hdr.tcp_udp_port_range.src.range <
                (ps_ntohs(info_ptr->ptcl_info.udp.src_port))))
          {
            return FALSE;
          }

          if ((tmp_mask & IPFLTR_MASK_TCP_UDP_DST_PORT) &&

             ((ps_ntohs(fi_ptr->next_prot_hdr.tcp_udp_port_range.dst.port)) >
                (ps_ntohs(info_ptr->ptcl_info.udp.dst_port)) ||

              (ps_ntohs(fi_ptr->next_prot_hdr.tcp_udp_port_range.dst.port)) +
                fi_ptr->next_prot_hdr.tcp_udp_port_range.dst.range <
                (ps_ntohs(info_ptr->ptcl_info.udp.dst_port))))
          {
            return FALSE;
          }
        break;

        default:
          break;
      }
    }
  break;

  default:
    /* unsupported protocol */
    ASSERT(0);
    return FALSE;
  }

  /*-------------------------------------------------------------------------
    If we reach here, all the specified parameters matached
  -------------------------------------------------------------------------*/
  return TRUE;
} /* ps_iface_ipfltri_process_default() */

#ifdef FEATURE_DATA_A2_FILTERING
/*===========================================================================
FUNCTION PS_IFACE_IPFLTRI_ADD_A2_FILTER_V4()

DESCRIPTION

DEPENDENCIES

RETURN VALUE
  TRUE
  FALSE

SIDE EFFECTS
  None.

===========================================================================*/
boolean ps_iface_ipfltri_add_a2_filter_v4
(
  a2_ipfilter_rule_set_handle_t   *a2_ruleset_handle,
  ip_filter_type                  *filter,
  uint8                           *next_hdr_prot
)
{
  uint32                          tmp_mask;
  a2_ipfilter_rule_result_e       a2_ret_val = A2_IPFILTER_RULE_SUCCESS;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  tmp_mask = filter->ip_hdr.v4.field_mask;
  while (tmp_mask)
  {
    if (tmp_mask & IPFLTR_MASK_IP4_SRC_ADDR)
    {
      /*----------------------------------------------------------------------
        The value that A2 expects for comparison has to be in byte order.
      ----------------------------------------------------------------------*/
      a2_ret_val = a2_ipfilter_add_rule_meq_32
                   (
                     a2_ruleset_handle,
                     A2_OFFSET_IPV4_SRC_ADDR,
                     ps_ntohl(filter->ip_hdr.v4.src.subnet_mask.ps_s_addr),
                     ps_ntohl(filter->ip_hdr.v4.src.subnet_mask.ps_s_addr) &
                       ps_ntohl(filter->ip_hdr.v4.src.addr.ps_s_addr)
                   );
      tmp_mask &= ~IPFLTR_MASK_IP4_SRC_ADDR;
    }
    else if (tmp_mask & IPFLTR_MASK_IP4_DST_ADDR)
    {
      a2_ret_val = a2_ipfilter_add_rule_meq_32
                   (
                     a2_ruleset_handle,
                     A2_OFFSET_IPV4_DST_ADDR,
                     ps_ntohl(filter->ip_hdr.v4.dst.subnet_mask.ps_s_addr),
                     ps_ntohl(filter->ip_hdr.v4.dst.subnet_mask.ps_s_addr) &
                       ps_ntohl(filter->ip_hdr.v4.dst.addr.ps_s_addr)
                   );
      tmp_mask &= ~IPFLTR_MASK_IP4_DST_ADDR;
    }
    else if (tmp_mask & IPFLTR_MASK_IP4_NEXT_HDR_PROT)
    {
      a2_ret_val =
        a2_ipfilter_add_rule_protocol_eq(a2_ruleset_handle,
                                         filter->ip_hdr.v4.next_hdr_prot);
      *next_hdr_prot = filter->ip_hdr.v4.next_hdr_prot;
      tmp_mask &= ~IPFLTR_MASK_IP4_NEXT_HDR_PROT;
    }
    else if (tmp_mask & IPFLTR_MASK_IP4_TOS)
    {
      a2_ret_val = a2_ipfilter_add_rule_tos_eq(a2_ruleset_handle,
                                               filter->ip_hdr.v4.tos.val);
      tmp_mask &= ~IPFLTR_MASK_IP4_TOS;
    }

    /*check for errors*/
    if (a2_ret_val != A2_IPFILTER_RULE_SUCCESS)
    {
      LOG_MSG_ERROR("ERROR: ps_iface_ipfltri_add_a2_filter_ipv4: %d",
                a2_ret_val, 0, 0);
      return FALSE;
    }
  } /* while (tmp_mask) */

  return TRUE;
} /* ps_iface_ipfltri_add_a2_filter_ipv4() */

/*===========================================================================
FUNCTION PS_IFACE_IPFLTRI_ADD_A2_FILTER_v6()

DESCRIPTION

DEPENDENCIES

RETURN VALUE
  TRUE
  FALSE

SIDE EFFECTS
  None.

===========================================================================*/
boolean ps_iface_ipfltri_add_a2_filter_v6
(
  a2_ipfilter_rule_set_handle_t   *a2_ruleset_handle,
  ip_filter_type                  *filter,
  uint8                           *next_hdr_prot
)
{
  a2_ipfilter_rule_result_e       ret_val = A2_IPFILTER_RULE_SUCCESS;
  uint32                          tmp_mask = 0;
  uint8                           a2_mask[16] = {0xFF,0xFF,0xFF,0xFF,
                                                 0xFF,0xFF,0xFF,0xFF,
                                                 0xFF,0xFF,0xFF,0xFF,
                                                 0xFF,0xFF,0xFF,0xFF};
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  tmp_mask = filter->ip_hdr.v6.field_mask;
  while (tmp_mask)
  {
    if (tmp_mask & IPFLTR_MASK_IP6_SRC_ADDR)
    {
      ret_val = a2_ipfilter_add_rule_meq_128(
                  a2_ruleset_handle,
                  A2_OFFSET_IPV6_SRC_ADDR,
                  a2_mask,
                  filter->ip_hdr.v6.src.addr.in6_u.u6_addr8);
      tmp_mask &= ~IPFLTR_MASK_IP6_SRC_ADDR;
    }
    else if (tmp_mask & IPFLTR_MASK_IP6_DST_ADDR)
    {
      ret_val = a2_ipfilter_add_rule_meq_128(
                  a2_ruleset_handle,
                  A2_OFFSET_IPV6_DST_ADDR,
                  a2_mask,
                  filter->ip_hdr.v6.dst.addr.in6_u.u6_addr8);
      tmp_mask &= ~IPFLTR_MASK_IP6_DST_ADDR;
    }
    else if (tmp_mask & IPFLTR_MASK_IP6_NEXT_HDR_PROT)
    {
      *next_hdr_prot = filter->ip_hdr.v6.next_hdr_prot;
      ret_val = a2_ipfilter_add_rule_protocol_eq(a2_ruleset_handle,
                                                 *next_hdr_prot);
      tmp_mask &= ~IPFLTR_MASK_IP6_NEXT_HDR_PROT;
    }
    else if (tmp_mask & IPFLTR_MASK_IP6_TRAFFIC_CLASS)
    {
      ret_val = a2_ipfilter_add_rule_tc_eq(a2_ruleset_handle,
                                       filter->ip_hdr.v6.trf_cls.val);
      tmp_mask &= ~IPFLTR_MASK_IP6_TRAFFIC_CLASS;
    }
    else if (tmp_mask & IPFLTR_MASK_IP6_FLOW_LABEL)
    {
      ret_val = a2_ipfilter_add_rule_flow_eq(a2_ruleset_handle,
                                             filter->ip_hdr.v6.flow_label);
      tmp_mask &= ~IPFLTR_MASK_IP6_FLOW_LABEL;
    }

    if (ret_val != A2_IPFILTER_RULE_SUCCESS)
    {
      LOG_MSG_ERROR("ERROR: A2_IPFILTER_ADD_RULE: %d, 0x%x", ret_val, tmp_mask, 0);
      ASSERT(0);
      return FALSE;
    }
  }//end while (tmp_mask)v6

  return TRUE;

}/*ps_iface_ipfltri_add_a2_filter_v6*/

/*===========================================================================
FUNCTION PS_IFACE_IPFLTRI_ADD_A2_FILTER_NEXT_HDR()

DESCRIPTION

DEPENDENCIES
  *assumes the filter and iface have already been verified

RETURN VALUE
  TRUE  for success
  FALSE  otherwise

SIDE EFFECTS
  None.

===========================================================================*/

boolean ps_iface_ipfltri_add_a2_filter_next_hdr
(
  a2_ipfilter_rule_set_handle_t    *a2_ruleset_handle,
  ip_filter_type                   *filter,
  uint8                            next_hdr_prot
  )
{
  a2_ipfilter_rule_result_e        ret_val = A2_IPFILTER_RULE_SUCCESS;
  uint32                           tmp_mask;
  uint32		           ipv6_value;

  switch (next_hdr_prot)
  {
    case PS_IPPROTO_TCP:
      tmp_mask = filter->next_prot_hdr.tcp.field_mask;
      if (tmp_mask & IPFLTR_MASK_TCP_SRC_PORT)
      {
        /*-------------------------------------------------------------------
          The -1 for the SRC port low is needed due to A2 implementation.
          Also the value that A2 expects for comparison has to be in byte
          order.
        -------------------------------------------------------------------*/
        ret_val = a2_ipfilter_add_rule_ihl_offset_range_16
                  (
                    a2_ruleset_handle,
                    A2_OFFSET_TCP_SRC_PORT,
                    ps_ntohs(filter->next_prot_hdr.tcp.src.port) - 1,
                    ps_ntohs(filter->next_prot_hdr.tcp.src.port) +
                      filter->next_prot_hdr.tcp.src.range
                  );
      }

      if (tmp_mask & IPFLTR_MASK_TCP_DST_PORT)
      {
        ret_val = a2_ipfilter_add_rule_ihl_offset_range_16
                  (
                    a2_ruleset_handle,
                    A2_OFFSET_TCP_DST_PORT,
                    ps_ntohs(filter->next_prot_hdr.tcp.dst.port) - 1,
                    ps_ntohs(filter->next_prot_hdr.tcp.dst.port) +
                      filter->next_prot_hdr.tcp.dst.range
                  );
      }

      break;

    case PS_IPPROTO_UDP:
      tmp_mask = filter->next_prot_hdr.udp.field_mask;
      if (tmp_mask & IPFLTR_MASK_UDP_SRC_PORT)
      {
        ret_val = a2_ipfilter_add_rule_ihl_offset_range_16
                  (
                    a2_ruleset_handle,
                    A2_OFFSET_UDP_SRC_PORT,
                    ps_ntohs(filter->next_prot_hdr.udp.src.port) - 1,
                    ps_ntohs(filter->next_prot_hdr.udp.src.port) +
                      filter->next_prot_hdr.udp.src.range
                  );
      }

      if (tmp_mask & IPFLTR_MASK_UDP_DST_PORT)
      {
        ret_val = a2_ipfilter_add_rule_ihl_offset_range_16
                  (
                    a2_ruleset_handle,
                    A2_OFFSET_UDP_DST_PORT,
                    ps_ntohs(filter->next_prot_hdr.udp.dst.port) - 1,
                    ps_ntohs(filter->next_prot_hdr.udp.dst.port) +
                      filter->next_prot_hdr.udp.dst.range
                  );
      }

      break;

    case PS_IPPROTO_ICMP:
      tmp_mask = filter->next_prot_hdr.icmp.field_mask;
      if (tmp_mask & IPFLTR_MASK_ICMP_MSG_TYPE)
      {
        ret_val = a2_ipfilter_add_rule_ihl_offset_eq_16
                  (
                    a2_ruleset_handle,
                    A2_OFFSET_ICMP_TYPE,
                    filter->next_prot_hdr.icmp.type
                  );
      }

      if (tmp_mask & IPFLTR_MASK_ICMP_MSG_CODE)
      {
        ret_val = a2_ipfilter_add_rule_ihl_offset_eq_16
                  (
                    a2_ruleset_handle,
                    A2_OFFSET_ICMP_CODE,
                    filter->next_prot_hdr.icmp.code
                  );
      }

      break;

    case PS_IPPROTO_ICMP6:
      tmp_mask = filter->next_prot_hdr.icmp.field_mask;
      if (tmp_mask & IPFLTR_MASK_ICMP_MSG_TYPE)
      {
        ipv6_value = (uint32)filter->next_prot_hdr.icmp.type;
        ipv6_value = ipv6_value << 24;
        ret_val    = a2_ipfilter_add_rule_meq_32(a2_ruleset_handle,
                                                 A2_OFFSET_IPV6_HEADER,
                                                 A2_MASK_ICMP_TYPE,
                                                 ipv6_value);
      }

      if (tmp_mask & IPFLTR_MASK_ICMP_MSG_CODE)
      {
        ipv6_value = (uint32) filter->next_prot_hdr.icmp.code;
        ipv6_value = ipv6_value << 16;
        ret_val    = a2_ipfilter_add_rule_meq_32(a2_ruleset_handle,
                                                 A2_OFFSET_IPV6_HEADER,
                                                 A2_MASK_ICMP_CODE,
                                                 ipv6_value);
      }

      break;

    default:
      LOG_MSG_ERROR("A2 ipfilter NEXT_HDR_PROT unsupported protocol", 0, 0, 0);
      return FALSE;
  }

  if (ret_val != A2_IPFILTER_RULE_SUCCESS)
  {
    LOG_MSG_ERROR("ERROR: A2_IPFILTER_ADD_RULE: %d, 0x%x",
                  ret_val, tmp_mask, 0);
    return FALSE;
  }

  return TRUE;
} /* ps_iface_ipfltri_add_a2_filter_next_hdr() */

/*===========================================================================
FUNCTION PS_IFACE_IPFLTRI_ADD_A2_FILTER()

DESCRIPTION
  This function adds the input filter to A2

DEPENDENCIES
  *assumes the filter and iface have already been verified

RETURN VALUE
  TRUE  for success
  FALSE  otherwise

SIDE EFFECTS
  None.

===========================================================================*/
boolean ps_iface_ipfltri_add_a2_filter
(
  ps_iface_type                         *iface_ptr,
  ip_filter_type                        *filter,
  a2_ipfilter_rule_set_handle_t         *a2_ruleset_handle,
  ps_iface_ipfltr_client_id_enum_type   client_id
)
{
  uint8                       next_hdr_prot = 0;
  boolean                     ret_val = FALSE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2("adding a2 fltrs to if 0x%p, client %d", iface_ptr, client_id, 0);

  switch (filter->ip_vsn)
  {
    case IP_V4:
      ret_val = ps_iface_ipfltri_add_a2_filter_v4(a2_ruleset_handle,
                                                  filter,
                                                  &next_hdr_prot);
      if (ret_val != TRUE)
      {
        LOG_MSG_ERROR("Failed to add A2 IPv4 Filter to iface: 0x%p",
                      iface_ptr, 0, 0);
        return FALSE;
      }

      break;

    case IP_V6:
      ret_val = ps_iface_ipfltri_add_a2_filter_v6(a2_ruleset_handle,
                                                  filter,
                                                  &next_hdr_prot);
      if (ret_val != TRUE)
      {
        LOG_MSG_ERROR("Failed to add A2 IPv6 Filter to iface: 0x%p",
                      iface_ptr, 0, 0);
        return FALSE;
      }

      break;

    default:
      LOG_MSG_ERROR("ip vsn of A2 filter unsupported if: 0x%p",
                    iface_ptr, 0, 0);
      ASSERT(0);
      return FALSE;
  }

  if (next_hdr_prot != 0)
  {
    ps_iface_ipfltri_add_a2_filter_next_hdr(a2_ruleset_handle,
                                            filter,
                                            next_hdr_prot);
  }

  return TRUE;
} /* ps_iface_ipfltri_add_a2_filter() */
#endif /*FEATURE_DATA_A2_FILTERING*/


/*===========================================================================

                      EXTERNAL FUNCTION DEFINITIONS

===========================================================================*/
/*===========================================================================
FUNCTION PS_IFACE_IPFLTR_INIT()

DESCRIPTION
  Initialize the global IP filter queue.

DEPENDENCIES
  None

PARAMETERS
  None.

RETURN VALUE
  None

SIDE EFFECTS
  None.

===========================================================================*/
void ps_iface_ipfltr_init
(
  void
)
{
  uint8 i;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Initialize Pool
  -------------------------------------------------------------------------*/
  if (PS_MEM_POOL_INIT_OPT(PS_MEM_PS_IFACE_IPFLTER_TYPE,
                           ps_iface_ipfltr_buf_mem,
                           PS_IFACE_IPFLTR_BUF_SIZE,
                           PS_IFACE_IPFLTR_BUF_NUM,
                           PS_IFACE_IPFLTR_BUF_HIGH_WM,
                           PS_IFACE_IPFLTR_BUF_LOW_WM,
                           NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                           (int *) ps_iface_ipfltr_buf_hdr,
                           (int *) ps_iface_ipfltr_buf_ptr
#else
                           NULL,
                           NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                          ) == -1)
  {
    LOG_MSG_FATAL_ERROR("Can't init the module", 0, 0, 0);
  }

  for (i = 0; i < IP_FLTR_CLIENT_MAX; i++)
  {
    (void) q_init(&(global_ipfltr_info[i]));
  }
}
/*===========================================================================
FUNCTION PS_IFACE_IPFLTR_ADD()

DESCRIPTION
  This function adds a set of IP filters to the specified iface. The
  filter set is tied to a filter handle, which uniquely idenifies a set of
  filters added by the given client for the given iface. The filter handle
  is used to manipulate the filter set. A client needs to provide a filter
  result which is returned when a filter successfully match during the filter
  execution.
  If filters are added in disabled state they won't be executed until
  they are enabled.

PARAMETERS
  iface_ptr          : Iface to add filters to
  client_id          : Filtering client id
  fltr_add_param_ptr : ptr to structure containing all the necessary info to
                       add filters to an iface
  ps_errno           : error returned to the caller if operation fails

RETURN VALUE
  A handle to filters            : on success
  PS_IFACE_IPFLTR_INVALID_HANDLE : on failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
ps_iface_ipfltr_handle_type ps_iface_ipfltr_add
(
  ps_iface_type                         * iface_ptr,
  ps_iface_ipfltr_client_id_enum_type     client_id,
  const ps_iface_ipfltr_add_param_type  * fltr_param_ptr,
  int16                                 * ps_errno
)
{
  q_type                         * ipfltr_q_ptr;
  ps_iface_ipfilteri_type        * new_filter_buf_ptr;
  ps_iface_ipfltr_handle_type      fltr_handle;
  ps_iface_ipfilteri_type        * tmp_tx_fltr_buf;
  int                              prev_cnt = 0;
  uint8                            fltr_index;
#ifdef FEATURE_DATA_A2_FILTERING
  boolean                          new_ruleset = FALSE;
#endif /* FEATURE_DATA_A2_FILTERING */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2( "FLTR ADD called by client %d, filter_result %d",
          client_id, fltr_param_ptr->fi_result, 0);

  /*-------------------------------------------------------------------------
    Validate all the parameters
  -------------------------------------------------------------------------*/
  if (ps_errno == NULL)
  {
    LOG_MSG_ERROR("NULL parameter is passed", 0, 0, 0);
    ASSERT(0);
    return PS_IFACE_IPFLTR_INVALID_HANDLE;
  }

  if (client_id >= IP_FLTR_CLIENT_MAX)
  {
    ASSERT(0);
    LOG_MSG_ERROR("Invalid filtering client id %d", client_id, 0, 0);
    *ps_errno = DS_EFAULT;
    return PS_IFACE_IPFLTR_INVALID_HANDLE;
  }

  if (fltr_param_ptr->filter_type >= IPFLTR_MAX_TYPE)
  {
    ASSERT(0);
    LOG_MSG_ERROR("Invalid filter type by client %d", client_id, 0, 0);
    *ps_errno = DS_EFAULT;
    return PS_IFACE_IPFLTR_INVALID_HANDLE;
  }

  if (fltr_param_ptr->fi_ptr_arr == NULL || fltr_param_ptr->num_filters == 0)
  {
    LOG_MSG_INFO1("No filter specified by client %d", client_id, 0, 0);
    *ps_errno = DS_EFAULT;
    return PS_IFACE_IPFLTR_INVALID_HANDLE;
  }

  if (fltr_param_ptr->fi_result == PS_IFACE_IPFLTR_NOMATCH)
  {
    LOG_MSG_ERROR("Invallid filter_result specified by client %d",
              client_id, 0, 0);
    ASSERT(0);
    *ps_errno = DS_EFAULT;
    return PS_IFACE_IPFLTR_INVALID_HANDLE;
  }

  if (fltr_param_ptr->fltr_priority != PS_IFACE_IPFLTR_PRIORITY_FCFS)
  {
    LOG_MSG_ERROR("Invalid fltr add style", fltr_param_ptr->fltr_priority, 0, 0);
    ASSERT(0);
    *ps_errno = DS_EFAULT;
    return PS_IFACE_IPFLTR_INVALID_HANDLE;
  }

  /*-------------------------------------------------------------------------
    Validate filters if they aren't already validated
  -------------------------------------------------------------------------*/
  if (!fltr_param_ptr->is_validated)
  {
    if (IPFLTR_DEFAULT_TYPE == fltr_param_ptr->filter_type)
    {
      if (!ps_iface_ipfltr_validate_fltr_param(client_id,
                                               fltr_param_ptr->fi_ptr_arr,
                                               fltr_param_ptr->num_filters))
      {
        LOG_MSG_INFO1("Invalid fltr spec is specified by client %d",
                 client_id, 0, 0);
        *ps_errno = DS_EINVAL;
        return PS_IFACE_IPFLTR_INVALID_HANDLE;
      }
    }
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);
  if (iface_ptr == NULL)
  {
    LOG_MSG_INFO2("Adding fltrs to global ipfltr q", 0, 0, 0);
    ipfltr_q_ptr = &(global_ipfltr_info[client_id]);
  }
  else if (PS_IFACE_IS_VALID(iface_ptr))
  {
    LOG_MSG_INFO2("Adding fltrs to iface 0x%x:%d",
            iface_ptr->name, iface_ptr->instance, 0);
    ipfltr_q_ptr = &(iface_ptr->iface_private.ipfltr_info[client_id]);
  }
  else
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Invalid iface, 0x%p, is passed by client %d",
              iface_ptr, client_id, 0);
    ASSERT(0);
    *ps_errno = DS_EFAULT;
    return PS_IFACE_IPFLTR_INVALID_HANDLE;
  }

  prev_cnt    = q_cnt(ipfltr_q_ptr);
  fltr_handle = PS_IFACE_IPFLTR_INVALID_HANDLE;

  /*-------------------------------------------------------------------------
    Now add the new filters to this iface
  -------------------------------------------------------------------------*/
  for (fltr_index = 0; fltr_index < fltr_param_ptr->num_filters; fltr_index++)
  {
    if ((new_filter_buf_ptr = ps_iface_ipfltri_alloc_buf()) == NULL)
    {
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      LOG_MSG_ERROR("Failed to create buffer for IP filter", 0, 0, 0);

      (void)
        ps_iface_ipfltr_delete(iface_ptr, client_id, fltr_handle, ps_errno);
      *ps_errno = DS_ENOMEM;
      return PS_IFACE_IPFLTR_INVALID_HANDLE;
    }

    if (fltr_index == 0)
    {
      fltr_handle = (ps_iface_ipfltr_handle_type) new_filter_buf_ptr;
    }

    switch (fltr_param_ptr->filter_type)
    {
      case IPFLTR_DEFAULT_TYPE:
        /* TODO: Apply mask to IP address here */

        new_filter_buf_ptr->filter.fi_default =
          *(((ip_filter_type *) fltr_param_ptr->fi_ptr_arr) + fltr_index);

        break;

      default:
        PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
        LOG_MSG_ERROR("Invalid filter type, %d, by client %d",
                   fltr_param_ptr->filter_type,
                   client_id,
                   0);

        (void)
          ps_iface_ipfltr_delete(iface_ptr, client_id, fltr_handle, ps_errno);
        ps_iface_ipfltri_free_buf(new_filter_buf_ptr);

        ASSERT(0);
        *ps_errno = DS_EFAULT;
        return PS_IFACE_IPFLTR_INVALID_HANDLE;
    }

    (void) q_link(new_filter_buf_ptr, &(new_filter_buf_ptr->link));
    new_filter_buf_ptr->filter_type = fltr_param_ptr->filter_type;
    new_filter_buf_ptr->fi_handle   = fltr_handle;
    new_filter_buf_ptr->fi_result   = fltr_param_ptr->fi_result;
    new_filter_buf_ptr->subset_id   = fltr_param_ptr->subset_id;
    new_filter_buf_ptr->disabled    = !fltr_param_ptr->enable;

#ifdef FEATURE_DATA_A2_FILTERING
    new_filter_buf_ptr->a2_ruleset = NULL;

    if (client_id == IP_FLTR_CLIENT_SOCKETS)
    {
      if (PS_IFACE_GET_CAPABILITY(iface_ptr, PS_IFACE_CAPABILITY_A2_CAPABLE))
      {
        /*----------------------------------------------------------------------
          Use SIO_PORT_NULL as the end point to which A2 has to send the data to
          SIO_PORT_NULL simply tells A2 that we need this packet up the stack
          and it should not be sent to any A2 end point.
        ----------------------------------------------------------------------*/
        if (new_filter_buf_ptr->filter.fi_default.ip_vsn == IP_V4)
        {
          new_filter_buf_ptr->a2_ruleset =
            a2_ipfilter_add_ruleset(A2_IPV4, SIO_PORT_NULL);
          new_ruleset = TRUE;

        }
        else
        {
          new_filter_buf_ptr->a2_ruleset =
            a2_ipfilter_add_ruleset(A2_IPV6, SIO_PORT_NULL);
          new_ruleset = TRUE;
        }

        if (new_filter_buf_ptr->a2_ruleset == NULL)
        {
          PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
          LOG_MSG_ERROR("a2_ipfilter_add_ruleset failed ", 0,0,0);

          (void)
            ps_iface_ipfltr_delete(iface_ptr, client_id, fltr_handle, ps_errno);
          ps_iface_ipfltri_free_buf(new_filter_buf_ptr);

          ASSERT(0);
          *ps_errno = DS_EFAULT;
          return PS_IFACE_IPFLTR_INVALID_HANDLE;
        }

        ps_iface_ipfltri_add_a2_filter(iface_ptr,
                                       &new_filter_buf_ptr->filter.fi_default,
                                       (a2_ipfilter_rule_set_handle_t*) new_filter_buf_ptr->a2_ruleset,
                                       client_id);

        LOG_MSG_INFO1("A2 filter with ruleset 0x%x added for IP vsn %d",
                 new_filter_buf_ptr->a2_ruleset,
                 new_filter_buf_ptr->filter.fi_default.ip_vsn, 0);
      }
    } /* if (client_id == IP_FLTR_CLIENT_SOCKETS)*/
#endif /* FEATURE_DATA_A2_FILTERING */

    /*-------------------------------------------------------------------------
      Add the new filters to the iface sorted by precedence
    -------------------------------------------------------------------------*/
    tmp_tx_fltr_buf = q_check(ipfltr_q_ptr);

    while (tmp_tx_fltr_buf != NULL &&
          new_filter_buf_ptr->filter.fi_default.ipfltr_aux_info.fi_precedence
           >= tmp_tx_fltr_buf->filter.fi_default.ipfltr_aux_info.fi_precedence)
    {
       tmp_tx_fltr_buf = q_next(ipfltr_q_ptr, &(tmp_tx_fltr_buf->link));
    }

    if (NULL == tmp_tx_fltr_buf)
    {
      q_put(ipfltr_q_ptr, &new_filter_buf_ptr->link);
    }
    else
    {
      q_insert(ipfltr_q_ptr, &new_filter_buf_ptr->link, &tmp_tx_fltr_buf->link);
    }
  }

  /*-------------------------------------------------------------------------
    Post indication only if newly added filters are enabled
  -------------------------------------------------------------------------*/
  if (fltr_param_ptr->enable)
  {
    ps_iface_ipfltr_updated_ind(iface_ptr,
                                client_id,
                                prev_cnt,
                                q_cnt(ipfltr_q_ptr));
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
#ifdef FEATURE_DATA_A2_FILTERING
  /*enable all of the A2 filtering if new rulesets have been added*/
  if (new_ruleset)
  {
    a2_ipfilter_commit_rules();
  }
#endif /*FEATURE_DATA_A2_FILTERING*/


  return fltr_handle;

} /* ps_iface_ipfltr_add() */



/*===========================================================================
FUNCTION PS_IFACE_IPFLTR_DELETE()

DESCRIPTION
  This function deletes all the existing IP filter rules for the specified
  filter handle for a given client from the specified iface.

DEPENDENCIES
  None

PARAMETERS
  iface_ptr : Iface to delete filters from
  client_id : Filtering client id
  fi_handle : filter handle associated with the filter set which was added
              on this iface by this client.
  ps_errno  : error returned to the caller if operation fails

RETURN VALUE
   0 : on success
  -1 : on failure

SIDE EFFECTS
  Some packets may not get filtered once these filters are deleted
===========================================================================*/
int ps_iface_ipfltr_delete
(
  ps_iface_type                        * iface_ptr,
  ps_iface_ipfltr_client_id_enum_type    client_id,
  ps_iface_ipfltr_handle_type            fi_handle,
  int16                                * ps_errno
)
{
  ps_iface_ipfilteri_type  * filter_buf_ptr;
  ps_iface_ipfilteri_type  * next_filter_buf_ptr;
  q_type                   * ipfltr_q_ptr;
  int                        prev_cnt;
  boolean                    post_indication = FALSE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2("FLTR DELETE called by client %d, filter_id %d",
          client_id, fi_handle, 0);

  if (ps_errno == NULL)
  {
    LOG_MSG_ERROR("NULL parameter is passed", 0, 0, 0);
    ASSERT(0);
    return -1;
  }

  if (client_id >= IP_FLTR_CLIENT_MAX)
  {
    ASSERT(0);
    LOG_MSG_ERROR("Invalid filtering client id %d", client_id, 0, 0);
    *ps_errno = DS_EFAULT;
    return -1;
  }

  if (fi_handle == PS_IFACE_IPFLTR_INVALID_HANDLE)
  {
    LOG_MSG_INFO1("Invalid filter handle is passed by client, %d",
             client_id, 0, 0);
    *ps_errno = DS_EFAULT;
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);
  if (iface_ptr == NULL)
  {
    LOG_MSG_INFO2("Deleting fltrs from global ipfltr q", 0, 0, 0);
    ipfltr_q_ptr = &(global_ipfltr_info[client_id]);
  }
  else if (PS_IFACE_IS_VALID(iface_ptr))
  {
    LOG_MSG_INFO2("Deleting fltrs from iface 0x%x:%d",
            iface_ptr->name, iface_ptr->instance, 0);
    ipfltr_q_ptr = &(iface_ptr->iface_private.ipfltr_info[client_id]);
  }
  else
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Invalid iface, 0x%p, is passed by client, %d",
              iface_ptr, client_id, 0);
    ASSERT(0);
    *ps_errno = DS_EFAULT;
    return -1;
  }

  prev_cnt = q_cnt(ipfltr_q_ptr);

  filter_buf_ptr = q_check(ipfltr_q_ptr);
  while (filter_buf_ptr != NULL)
  {
    next_filter_buf_ptr = q_next(ipfltr_q_ptr, &(filter_buf_ptr->link));

    if (filter_buf_ptr->fi_handle == fi_handle)
    {
      /*---------------------------------------------------------------------
        Post indication only if any of deleted filters are enabled. Since
        indication is already posted when filters are disabled, another
        indication would be redundant
      ---------------------------------------------------------------------*/
      if (filter_buf_ptr->disabled == FALSE)
      {
        post_indication = TRUE;
      }

      /*---------------------------------------------------------------------
        Delete this filter
      ---------------------------------------------------------------------*/
#ifdef FEATURE_DATA_A2_FILTERING
      /*---------------------------------------------------------------------
        Check if iface is A2 capable
      ---------------------------------------------------------------------*/
      if (PS_IFACE_GET_CAPABILITY(iface_ptr, PS_IFACE_CAPABILITY_A2_CAPABLE))
      {
        if (client_id == IP_FLTR_CLIENT_SOCKETS)
        {
          if (filter_buf_ptr->a2_ruleset != NULL)
          {
          if (filter_buf_ptr->filter.fi_default.ip_vsn == IP_V4)
          {
            a2_ipfilter_remove_ruleset(filter_buf_ptr->a2_ruleset, A2_IPV4);
          }
          else /*as the filter is valid, it must be v4 or v6 only*/
          {
            a2_ipfilter_remove_ruleset(filter_buf_ptr->a2_ruleset, A2_IPV6);
          }
        }
        }

        LOG_MSG_INFO1("Removing A2 filter with ruleset 0x%x for ip vsn %d",
                 filter_buf_ptr->a2_ruleset,
                 filter_buf_ptr->filter.fi_default.ip_vsn, 0);

      } /* if PS_IFACE_CAPABILITY_A2_CAPABLE */
#endif /* FEATURE_DATA_A2_FILTERING */

#ifdef FEATURE_Q_NO_SELF_QPTR
      q_delete(ipfltr_q_ptr, &(filter_buf_ptr->link));
#else
      q_delete(&(filter_buf_ptr->link));
#endif

      ps_iface_ipfltri_free_buf(filter_buf_ptr);
    }

    filter_buf_ptr = next_filter_buf_ptr;
  }

  /*-------------------------------------------------------------------------
    Post indication only if filters are deleted
  -------------------------------------------------------------------------*/
  if (post_indication && prev_cnt != q_cnt(ipfltr_q_ptr))
  {
    ps_iface_ipfltr_updated_ind(iface_ptr,
                                client_id,
                                prev_cnt,
                                q_cnt(ipfltr_q_ptr));
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;

} /* ps_iface_ipfltr_delete() */


int32 ps_iface_ipfltr_is_handle_enabled
(
  ps_iface_type                       * iface_ptr,
  ps_iface_ipfltr_client_id_enum_type   client_id,
  ps_iface_ipfltr_handle_type           fi_handle,
  boolean                             * is_enabled_ptr,
  int16                               * ps_errno
)
{
  q_type                   * q_ptr;
  ps_iface_ipfilteri_type  * filter_buf_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY("iface 0x%p client %d, fltr_id %d",
                         iface_ptr, client_id, fi_handle);

  /*-------------------------------------------------------------------------
    Validate all the parameters
  -------------------------------------------------------------------------*/
  if (ps_errno == NULL)
  {
    LOG_MSG_ERROR("NULL errno, iface 0x%p", iface_ptr, 0, 0);
    return -1;
  }

  if (client_id >= IP_FLTR_CLIENT_MAX)
  {
    LOG_MSG_ERROR("Invalid filtering client id %d, iface 0x%p",
                  client_id, iface_ptr, 0);
    *ps_errno = DS_EFAULT;
    return -1;
  }

  if (fi_handle == PS_IFACE_IPFLTR_INVALID_HANDLE)
  {
    LOG_MSG_ERROR("Invalid fltr handle 0x%x is passed by client %d, "
                  "iface 0x%p", fi_handle, client_id, iface_ptr);
    *ps_errno = DS_EFAULT;
    return -1;
  }

  if (is_enabled_ptr == NULL)
  {
    LOG_MSG_ERROR("NULL arg, iface 0x%p", iface_ptr, 0, 0);
    *ps_errno = DS_EFAULT;
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  if (iface_ptr == NULL)
  {
    q_ptr = &(global_ipfltr_info[client_id]);
  }
  else if (PS_IFACE_IS_VALID(iface_ptr))
  {
    q_ptr = &(iface_ptr->iface_private.ipfltr_info[client_id]);
  }
  else
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Invalid iface 0x%p is passed", iface_ptr, 0, 0);
    *ps_errno = DS_EINVAL;
    return -1;
  }

  filter_buf_ptr = q_check(q_ptr);
  while (filter_buf_ptr != NULL)
  {
    if (filter_buf_ptr->fi_handle == fi_handle)
    {
      *is_enabled_ptr = !( filter_buf_ptr->disabled);
      break;
    }

    filter_buf_ptr = q_next(q_ptr, &(filter_buf_ptr->link));
  }

  if (filter_buf_ptr == NULL)
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Couldn't find a fltr matching fltr handle 0x%x, iface 0x%p",
                  fi_handle, iface_ptr, 0);
    *ps_errno = DS_EINVAL;
    return -1;
  }

  LOG_MSG_FUNCTION_EXIT("is_enabled %d, iface 0x%p",
                        *is_enabled_ptr, iface_ptr, 0);

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;

} /* ps_iface_ipfltr_is_handle_enabled() */



/*===========================================================================
FUNCTION PS_IFACE_IPFLTR_CONTROL()

DESCRIPTION
  This function allows a control operation on all IP filters currently
  associated with the specified filter handle of the client for the iface.
  Only operation supported for now is disabling or enabling the filter
  execution state. Disabled filters are not used during filter execution.

DEPENDENCIES
  None

PARAMETERS
  iface_ptr : Iface on which filters are added
  client_id : Filtering client id
  fi_handle : filter handle which identifies a specific filter set added on
              this iface by this client.
  enable    : Whether to enable or disable already installed filters
  ps_errno  : error returned to the caller if operation fails

RETURN VALUE
   0 : on success
  -1 : on failure

SIDE EFFECTS
  Some packets may not get filtered once a filter set is disabled and they
  may get filtered if a filter set is enabled
===========================================================================*/
int ps_iface_ipfltr_control
(
  ps_iface_type                       *iface_ptr,
  ps_iface_ipfltr_client_id_enum_type client_id,
  ps_iface_ipfltr_handle_type         fi_handle,
  boolean                             enable,
  int16                               *ps_errno
)
{
  q_type                   * q_ptr;
  ps_iface_ipfilteri_type  * filter_buf_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2("FLTR CONTROL called by client %d, filter_id %d, fltrs enabled=%d",
          client_id, fi_handle, enable);

  /*-------------------------------------------------------------------------
    Validate all the parameters
  -------------------------------------------------------------------------*/
  if (ps_errno == NULL)
  {
    LOG_MSG_ERROR("NULL parameter is passed", 0, 0, 0);
    ASSERT(0);
    return -1;
  }

  if (client_id >= IP_FLTR_CLIENT_MAX)
  {
    ASSERT(0);
    LOG_MSG_ERROR("Invalid filtering client id %d", client_id, 0, 0);
    *ps_errno = DS_EFAULT;
    return -1;
  }

  if (fi_handle == PS_IFACE_IPFLTR_INVALID_HANDLE)
  {
    LOG_MSG_ERROR("Invalid filter handle is passed by client, %d",
              client_id, 0, 0);
    *ps_errno = DS_EFAULT;
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);
  if (iface_ptr == NULL)
  {
    LOG_MSG_INFO2("Controlling fltrs on global ipfltr q", 0, 0, 0);
    q_ptr = &(global_ipfltr_info[client_id]);
  }
  else if (PS_IFACE_IS_VALID(iface_ptr))
  {
    LOG_MSG_INFO2("Controlling fltrs on iface 0x%x:%d",
            iface_ptr->name, iface_ptr->instance, 0);
    q_ptr = &(iface_ptr->iface_private.ipfltr_info[client_id]);
  }
  else
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Invalid iface 0x%p is passed", iface_ptr, 0, 0);
    ASSERT(0);
    *ps_errno = DS_EFAULT;
    return -1;
  }

  /*-------------------------------------------------------------------------
    Perform the control operation on all the filters that match the handle
  -------------------------------------------------------------------------*/
  filter_buf_ptr = q_check(q_ptr);
  while (filter_buf_ptr != NULL)
  {
    if (filter_buf_ptr->fi_handle == fi_handle)
    {
      filter_buf_ptr->disabled = !enable;
    }
    filter_buf_ptr = q_next(q_ptr, &(filter_buf_ptr->link));
  }

  ps_iface_ipfltr_updated_ind(iface_ptr, client_id, q_cnt(q_ptr), q_cnt(q_ptr));
  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;

} /* ps_iface_ipfltr_control() */



/*===========================================================================
FUNCTION PS_IFACE_IPFLTR_EXECUTE()

DESCRIPTION
  This function executes all the filters in an iface to see if any of
  those passes criteria specified in the information passed in. Processing
  is done until the first match is found and filter id associated
  with that filter is returned. For DEFAULT filter type, each field set in
  the filter is compared against the input info using a fixed set of rules.
  For ACL type filters, the ACL function is called which can contain more
  complex and variant types of rules.

DEPENDENCIES
  None

PARAMETERS
  iface_ptr          - Iface to pick filters from for execution
  client_id          - Filtering client id
  subset_id          - ID which identified a subset of all filters installed
                       on iface. Only these filters are considered for
                       matching a packet
  ip_filter_info_ptr - Ptr to IP pkt information block to apply filter on

RETURN VALUE
  filter id associated with the filter : on a successful filter match
  PS_IFACE_IPFLTR_NOMATCH              : for no match

SIDE EFFECTS
  None
===========================================================================*/
ps_iface_ipfltr_result_type ps_iface_ipfltr_execute
(
  ps_iface_type                       *iface_ptr,
  ps_iface_ipfltr_client_id_enum_type client_id,
  ps_iface_ipfltr_subset_id_type      subset_id,
  ip_pkt_info_type                    *ip_pkt_info_ptr
)
{
  q_type                       * q_ptr;
  ps_iface_ipfilteri_type      * filter_buf_ptr;
  ps_iface_ipfltr_result_type    result = PS_IFACE_IPFLTR_NOMATCH;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO3("Executing fltrs if 0x%p, client %d", iface_ptr, client_id, 0);

  /*-------------------------------------------------------------------------
    Validate all the parameters
  -------------------------------------------------------------------------*/
  if (iface_ptr != NULL && !PS_IFACE_IS_VALID(iface_ptr))
  {
    ASSERT(0);
    return result;
  }

  if (client_id >= IP_FLTR_CLIENT_MAX)
  {
    ASSERT(0);
    LOG_MSG_ERROR("Invalid filtering client id %d", client_id, 0, 0);
    return result;
  }

  if (ip_pkt_info_ptr == NULL)
  {
    LOG_MSG_ERROR("If 0x%p client %d null pkt info", iface_ptr, client_id, 0);
    return result;
  }

  /*-------------------------------------------------------------------------
    Check if pkt info is same as current IP family of the ps_iface.
  -------------------------------------------------------------------------*/
  if (iface_ptr != NULL &&
      ps_iface_get_addr_family(iface_ptr) !=
        (ps_iface_addr_family_type) ip_pkt_info_ptr->ip_vsn)
  {
    LOG_MSG_ERROR("If 0x%p client %d pkt ip version mismatch",
              iface_ptr, client_id, 0);
    return result;
  }

#ifndef FEATURE_DATA_PS_IPV6
  /*-------------------------------------------------------------------------
    if IPv6 is not defined don't execute the v6 filters.
  -------------------------------------------------------------------------*/
  if (ip_pkt_info_ptr->ip_vsn != IP_V4)
  {
    return result;
  }
#endif /* FEATURE_DATA_PS_IPV6 */

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);
  if (iface_ptr == NULL)
  {
    q_ptr = &(global_ipfltr_info[client_id]);
  }
  else
  {
    q_ptr = &(iface_ptr->iface_private.ipfltr_info[client_id]);
  }

  for (filter_buf_ptr = q_check(q_ptr);
       filter_buf_ptr != NULL;
       filter_buf_ptr = q_next(q_ptr, &(filter_buf_ptr->link)))
  {
    /*-----------------------------------------------------------------------
      Skip the filters which are disabled.
      Skip the filters that doesn't belong to same subset_id. If subset_id is
      PS_IFACE_IPFLTR_SUBSET_ID_DEFAULT, go through all filters
    -----------------------------------------------------------------------*/
    if (!filter_buf_ptr->disabled &&
        (subset_id == filter_buf_ptr->subset_id ||
         subset_id == PS_IFACE_IPFLTR_SUBSET_ID_DEFAULT))
    {
      switch (filter_buf_ptr->filter_type)
      {
      case IPFLTR_DEFAULT_TYPE:
        /*-------------------------------------------------------------------
          For default filter, process the filter info to see if required
          fields match.  Only execute the filter which actually matches the
          IP version of the received packet, skip others.
        -------------------------------------------------------------------*/
        if (filter_buf_ptr->filter.fi_default.ip_vsn !=
            ip_pkt_info_ptr->ip_vsn)
        {
          break;
        }

        if (ps_iface_ipfltri_process_default(
             &(filter_buf_ptr->filter.fi_default),
             ip_pkt_info_ptr) == TRUE)
        {
          result = filter_buf_ptr->fi_result;
          LOG_MSG_INFO2("Fltr passed if 0x%p, client %d, fi_result %d",
                  iface_ptr, client_id, result);
        }
        break;

      default:
        LOG_MSG_ERROR("Invalid filter type %d",
                  filter_buf_ptr->filter_type, 0, 0);
        ASSERT(0);
        break;
      }

      if (result != PS_IFACE_IPFLTR_NOMATCH)
      {
        break;
      }
    }
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return result;

} /* ps_iface_ipfltr_execute() */



/*===========================================================================
FUNCTION PS_IFACE_IPFLTR_VALIDATE_FLTR_PARAM()

DESCRIPTION
  Validates parameters of a filter

PARAMETERS
  client_id : Filtering client id
  fltr_arr  : Array of ptr to filters
  num_fltr  : Number of filters in above array

RETURN VALUE
  TRUE  : if parameters are valid
  FALSE : otherwise

DEPENDENCIES
  None

SIDE EFFECTS
  the appropriate error masks will be set with the appropriate error codes
  for values and enums which failed validation. Because variables are only
  validated if the enums are set in the field masks the only error masks
  that will be updated will be those corresponding to the values set within
  the field masks.
===========================================================================*/
boolean ps_iface_ipfltr_validate_fltr_param
(
  ps_iface_ipfltr_client_id_enum_type  client_id,
  ip_filter_type                       fltr_arr[],
  uint8                                num_fltr
)
{
  ip_filter_type         * fltr_ptr;
  uint8                    fltr_index;
  ps_ip_protocol_enum_type    next_hdr_prot = PS_NO_NEXT_HDR;
  boolean                  is_fltr_valid = TRUE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (client_id >= IP_FLTR_CLIENT_MAX)
  {
    LOG_MSG_ERROR("Invalid client_id, %d, is passed", client_id, 0, 0);
    ASSERT(0);
    return FALSE;
  }

  if (fltr_arr == NULL)
  {
    LOG_MSG_ERROR("NULL parameter is passed", 0, 0, 0);
    ASSERT(0);
    return FALSE;
  }

  LOG_MSG_INFO2("Validating filters of client %d", client_id, 0, 0);

  for (fltr_index = 0; fltr_index < num_fltr; fltr_index++)
  {
    fltr_ptr = &fltr_arr[fltr_index];

    /*----------------------------------------------------------------------
      Validate all fltr enums and variables. Validation fails if any of the
      parameters are invalid
    ----------------------------------------------------------------------*/
    if (fltr_ptr->ip_vsn != IP_V4 && fltr_ptr->ip_vsn != IP_V6)
    {
      LOG_MSG_INFO1("Incorrect IP version specified %d by client %d",
               fltr_ptr->ip_vsn, client_id, 0);
      return FALSE;
    }

    /*-------------------------------------------------------------------------
      Filter spec is invalid if any of the protocol params are invalid and
      hence "is_valid &=" is used below.
    -------------------------------------------------------------------------*/
    if (fltr_ptr->ip_vsn == IP_V4)
    {
      is_fltr_valid &= ps_iface_ipfltri_validate_ipv4_param(client_id,
                                                            fltr_ptr,
                                                            &next_hdr_prot);
    }
    else /* IP version is IPV6 */
    {
      is_fltr_valid &= ps_iface_ipfltri_validate_ipv6_param(client_id,
                                                            fltr_ptr,
                                                            &next_hdr_prot);
    }

    if (next_hdr_prot != PS_NO_NEXT_HDR)
    {
      switch (next_hdr_prot)
      {
        case PS_IPPROTO_TCP:
          is_fltr_valid &=
            ps_iface_ipfltri_validate_tcp_param(client_id, fltr_ptr);
          break;

        case PS_IPPROTO_UDP:
          is_fltr_valid &=
            ps_iface_ipfltri_validate_udp_param(client_id, fltr_ptr);
          break;

        case PS_IPPROTO_ICMP:
        case PS_IPPROTO_ICMP6:
          is_fltr_valid &=
            ps_iface_ipfltri_validate_icmp_param(client_id, fltr_ptr);
          break;

        case PS_IPPROTO_ESP:
          /* Do nothing */
          break;

        case PS_IPPROTO_TCP_UDP:
          is_fltr_valid &=
            ps_iface_ipfltri_validate_tcp_udp_param(client_id, fltr_ptr);
          break;

        default:
          is_fltr_valid = FALSE;
          break;
      }
    } /* end if (next_prot_hdr != PS_NO_NEXT_HDR) */
  }

  return is_fltr_valid;

} /* ps_iface_ipfltr_validate_fltr_param() */

#endif /* FEATURE_DATA_PS */
