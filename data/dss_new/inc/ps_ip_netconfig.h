#ifndef PS_IP_NETCONFIG_H
#define PS_IP_NETCONFIG_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                            P S _ I P _ N E T C O N F I G . H

GENERAL DESCRIPTION
  This is the header file that defines all of the IP addressing types and
  macros. 

Copyright (c) 2008-2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  $PVCSPath: L:/src/asw/MM_DATA/vcs/ps_ip_addr.h_v   1.5   11 Oct 2002 09:15:46   ubabbar  $
  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_ip_netconfig.h#2 $ $DateTime: 2011/02/03 15:10:41 $ $Author: anupamad $

when        who    what, where, why
-------     ---    ----------------------------------------------------------
02/03/11    cp     Removed the use of first hop mtu variable.
07/30/10    cp     Propagating the first hop mtu value to TE for v6.
03/26/09    pp     CMI De-featurization and Removed unused defs.
12/14/08    pp     Created module as part of Common Modem Interface: 
                   Public/Private API split.
===========================================================================*/



/*===========================================================================

                                INCLUDE FILES

===========================================================================*/
#include "comdef.h"
#include "ps_in.h"
#include "nv_items.h"
#include "ps_ppp_ext.h"

/*---------------------------------------------------------------------------
TYPEDEF IP_V4_CONFIG_TYPE

DESCRIPTION
  All of the IP configuration information
---------------------------------------------------------------------------*/
typedef struct
{
  struct ps_in_addr gateway;
  struct ps_in_addr primary_dns;
  struct ps_in_addr secondary_dns;

  struct ps_in_addr net_mask;

  /* for WINS support */
  struct ps_in_addr primary_nbns; 
  struct ps_in_addr secondary_nbns;
} ip_v4_net_info_type;

/*---------------------------------------------------------------------------
TYPEDEF IP_V6_NET_INFO_TYPE

DESCRIPTION
  All of the IPv6 configuration information
---------------------------------------------------------------------------*/
typedef struct
{
  struct ps_in6_addr primary_dns;
  struct ps_in6_addr secondary_dns;
  uint64             gateway_iid;
  uint8              curr_hop_limit;

} ip_v6_net_info_type;

/*---------------------------------------------------------------------------
  TYPEDEF NETWORK_INFO_TYPE
  All the IP related network information.
---------------------------------------------------------------------------*/
typedef struct
{
  union
  {
    ip_v4_net_info_type v4;
    ip_v6_net_info_type v6;
  } net_ip;
  
  uint16 mtu;

} network_info_type;

/*---------------------------------------------------------------------------
  struct definitions to pass UM config info(ipcp, auth etc) in bring_up_cmd 
  This replaces the previous struct used by umts (dsumtsps_rm_ppp_info)
  Now defined in a common place as will be used by cdma in future
---------------------------------------------------------------------------*/  
typedef struct
{
  uint32  ip_address;                              /* ip address requested */
  uint32  primary_dns;                              /* primary DNS address */
  uint32  secondary_dns;                          /* secondary DNS address */
  uint32  primary_nbns;                             /* primary DNS address */
  uint32  secondary_nbns;                           /* primary DNS address */
} ipcp_info_type;

typedef enum
{
  NET_CFG_PRM_PRI_DNS_MASK         =   0x00000001,
  NET_CFG_PRM_SEC_DNS_MASK         =   0x00000002,
  NET_CFG_PRM_PRI_NBNS_MASK        =   0x00000004,
  NET_CFG_PRM_SEC_NBNS_MASK        =   0x00000008, 
  NET_CFG_PRM_IP_ADDR_MASK         =   0x00000010,
  NET_CFG_PRM_AUTH_PREF_MASK       =   0x00000020,
  NET_CFG_PRM_AUTH_USERNAME_MASK   =   0x00000040,
  NET_CFG_PRM_AUTH_PASSWORD_MASK   =   0x00000080,
  NET_CFG_PRM_CHAP_CHAL_INFO_MASK  =   0x00000100, /* includes chal_info 
                                                      chal_name and auth_id */
  NET_CFG_PRM_DIAL_STR_MASK        =   0x00000200,
  NET_CFG_PRM_MAX_MASK
} net_cfg_params_mask_e_type;

typedef struct
{
  uint32 valid_fields;
  struct
  {
    uint8 auth_type;                   /* auth negotiated=CHAP, PAP or None */
    ppp_auth_info_type params;         /* Pointer to Auth info parameters   */
    uint8 auth_id;                     /* id used for auth packets          */
  }auth_info;
  byte dial_string[NV_PKT_DIAL_STRING_SIZE]; /*dial string passed by ATCOP*/
  ipcp_info_type ipcp_info;
} network_params_info_type;

#endif /* PS_IP_NETCONFIG_H */
