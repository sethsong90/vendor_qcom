#ifndef PS_NAT_H
#define PS_NAT_H

/*===========================================================================

                   NAT    H E A D E R    F I L E

DESCRIPTION
  Network Address Translation (NAT) Implementation based on RFC3022

Copyright (c) 1995-2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE

when       who     what, where, why
--------   ---     ----------------------------------------------------------
04/20/2010 bq      Initial Version.

===========================================================================*/

/*===========================================================================

                                INCLUDE FILES

===========================================================================*/
#include "customer.h"       /* Customer Specific Features */
#include "comdef.h"
#ifdef FEATURE_DATA_PS
#ifdef FEATURE_DATA_PS_NAT_IFACE

#ifdef __cplusplus
extern "C"
{
#endif

#include "dsm.h"
#include "ps_in.h"
#include "ps_ip4_hdr.h"
#include "ps_icmp.h"
#include "ps_pkt_info.h"
#include "ps_pkt_info_utils.h"
#include "ps_iputil.h"
#include "ps_udp.h"
#include "ps_tcp_hdr.h"

/*===========================================================================

                      EXTERNAL MACRO DECLARATIONS

===========================================================================*/

/*---------------------------------------------------------------------------
  NAT HASH table size.
---------------------------------------------------------------------------*/
#define NAT_HASH_SIZE 200

/*---------------------------------------------------------------------------
  Max NAT Entry
---------------------------------------------------------------------------*/
#define NAT_MAX_ENTRY 200

/*---------------------------------------------------------------------------
  NAT Period Timeout Interval to check for entry timeout in sec
---------------------------------------------------------------------------*/
#define NAT_TIMEOUT_INTERVAL               10

/*---------------------------------------------------------------------------
  NAT Timeout Interval after receive TCP FIN/RST in sec
---------------------------------------------------------------------------*/
#define NAT_TCP_CLOSE_TIMEOUT_INTERVAL     20

/*---------------------------------------------------------------------------
  Default NAT Entry Timeout in sec
---------------------------------------------------------------------------*/
#define NAT_ENTRY_TIMEOUT                  120 
#define NAT_ENTRY_TCP_IDLE_TIMEOUT         (60*60)

#define NAT_FTP_CONTROL_PORT               21
#define NAT_FTP_DATA_PORT                  20

/*---------------------------------------------------------------------------
  NAT ICMP Query ID 
---------------------------------------------------------------------------*/
#define NAT_START_ICMP_ID                  1024
#define NAT_ICMP_ID_RANGE                  64500

/*---------------------------------------------------------------------------
  NAT Reserved Port 
---------------------------------------------------------------------------*/
#define NAT_START_RESERVED_PORT            1024 
#define NAT_RESERVED_PORT_RANGE            64500 

/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/

/*---------------------------------------------------------------------------
  NAT Entry State
---------------------------------------------------------------------------*/
typedef enum
{
  NAT_STATE_IDLE = 0,
  NAT_STATE_ACTIVE
} nat_state_e;

/*---------------------------------------------------------------------------
  NAT Entry Type
---------------------------------------------------------------------------*/
typedef enum
{
  NAT_STATIC_ENTRY,
  NAT_DYNAMIC_ENTRY
} nat_translation_entry_type_e;


/*---------------------------------------------------------------------------
  NAT Pkt Process Result
---------------------------------------------------------------------------*/
typedef enum
{
  NAT_PKT_SUCCESS = 0, /* NAT operation sucess */  
  NAT_PKT_FAIL, /* NAT operation fail */
  NAT_PKT_CONSUME /* NAT consume the packet */
} nat_pkt_process_result_e;

/*---------------------------------------------------------------------------
  NAT Packet Direction
---------------------------------------------------------------------------*/
typedef enum
{
  NAT_PKT_INBOUND = 0, /* INBOUND NAT Packet */
  NAT_PKT_OUTBOUND = 1,/* OUTBOUND NAT Packet */
  NAT_PKT_DIR_DONT_CARE = 2 /* Direction ignored */
} nat_pkt_direction_e;

/*---------------------------------------------------------------------------
  NAT Configuration Structure
---------------------------------------------------------------------------*/
typedef struct nat_config
{
  /* Reserved port range used for NAT external port */
  uint16                     reserved_port;
  uint16                     reserved_port_range;

  /* Reserved ICMP Query ID */
  uint16                     reserved_icmp_id;
  uint16                     reserved_icmp_id_range;                   

  uint16                     entry_timeout;
  uint16                     entry_tcp_timeout;

  /* Whether IPSEC VPN passthrough is enabled or not */
  boolean                    ipsec_vpn_pass_through;

  /* Whether L2TP VPN passthrough is enabled or not */
  boolean                    l2tp_vpn_pass_through;

  /* Whether PPTP VPN passthrough is enabled or not */
  boolean                    pptp_vpn_pass_through;

  /* Whether DMZ is enables or not */
  boolean                    is_dmz_enabled;
  /* DMZ Private IP address in network order */ 
  ip_addr_type               dmz_ip_addr; 
} nat_config_s;


/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/


/*===========================================================================

FUNCTION ps_nat_init()

DESCRIPTION
  This function initialize the NAT control block, NAT Translation Table.

DEPENDENCIES

RETURN VALUE
  TRUE - Success
  FALSE - Fail

SIDE EFFECTS
  None
===========================================================================*/
extern boolean ps_nat_init(
  void *nat_table_ptr
);

/*===========================================================================

FUNCTION ps_nat_cleanup()

DESCRIPTION
  This function cleanup the NAT control block, NAT Translation Table.

DEPENDENCIES

RETURN VALUE

SIDE EFFECTS
  None
===========================================================================*/
extern void ps_nat_cleanup(void);

/*===========================================================================

FUNCTION ps_nat_checksumadjust()

DESCRIPTION
  This function perform checksum adjustment.

DEPENDENCIES
  - chksum points to the chksum in the packet
  - optr points to the old data in the packet
  - nptr points to the new data in the packet

RETURN VALUE
  TRUE - Success
  FALSE - Fail

SIDE EFFECTS
  None
===========================================================================*/
extern void ps_nat_checksumadjust
(
  unsigned char *chksum, \
  const unsigned char *optr,
  int olen, 
  const unsigned char *nptr, 
  int nlen
);


/*===========================================================================

FUNCTION nat_config_get()

DESCRIPTION
  This function get the NAT configuration.

DEPENDENCIES

RETURN VALUE

SIDE EFFECTS
  None
===========================================================================*/
void ps_nat_config_get(nat_config_s *nat_config);

/*===========================================================================
FUNCTION PS_IFACE_ADD_STATIC_NAT_ENTRY

DESCRIPTION
  This function add a static nat entry

PARAMETERS
  entry_ptr: ptr to the NAT entry to be added

RETURN VALUE
   0 if successful
   -1 if fails

DEPENDENCIES

SIDE EFFECTS
  
===========================================================================*/
extern int ps_iface_add_static_nat_entry
(
  ps_iface_ioctl_add_static_nat_entry_type * entry_ptr
);

/*===========================================================================
FUNCTION PS_IFACE_DELETE_STATIC_NAT_ENTRY

DESCRIPTION
  This function delete a static nat entry

PARAMETERS
  entry_ptr: ptr to the NAT entry to be deleted

RETURN VALUE
   0 if successful
   -1 if fails

DEPENDENCIES

SIDE EFFECTS
  
===========================================================================*/
extern int ps_iface_delete_static_nat_entry
(
  ps_iface_ioctl_delete_static_nat_entry_type * entry_ptr
);

/*===========================================================================
FUNCTION PS_IFACE_GET_STATIC_NAT_ENTRY

DESCRIPTION
  This function get static nat entries

PARAMETERS
  entry_ptr: ptr to the NAT entries to be returned

RETURN VALUE
   0 if successful
   -1 if fails

DEPENDENCIES

SIDE EFFECTS
  
===========================================================================*/
extern int ps_iface_get_static_nat_entry
(
  ps_iface_ioctl_get_static_nat_entry_type * entry_ptr
);

/*===========================================================================
FUNCTION PS_IFACE_GET_DYNAMIC_NAT_ENTRY_TIMEOUT

DESCRIPTION
  This function get dynamic NAT entry timeout

PARAMETERS
  timeout_ptr: ptr to the timeout

RETURN VALUE
   0 if successful
   -1 if fails

DEPENDENCIES

SIDE EFFECTS
  
===========================================================================*/
extern int ps_iface_get_dynamic_nat_entry_timeout
(
  ps_iface_ioctl_get_dynamic_nat_entry_timeout_type * timeout_ptr
);

/*===========================================================================
FUNCTION PS_IFACE_SET_DYNAMIC_NAT_ENTRY_TIMEOUT

DESCRIPTION
  This function set dynamic NAT entry timeout

PARAMETERS
  timeout_ptr: ptr to the timeout

RETURN VALUE
   0 if successful
   -1 if fails

DEPENDENCIES

SIDE EFFECTS
  
===========================================================================*/
extern int ps_iface_set_dynamic_nat_entry_timeout
(
  ps_iface_ioctl_set_dynamic_nat_entry_timeout_type * timeout_ptr
);


/*===========================================================================
FUNCTION PS_IFACE_GET_NAT_IPSEC_VPN_PASS_THROUGH

DESCRIPTION
  This function get NAT IPSEC VPN PassThrough

PARAMETERS
  vpn_passthrough_ptr: ptr to the IPSEC VPN PassThrough

RETURN VALUE
   0 if successful
   -1 if fails

DEPENDENCIES

SIDE EFFECTS
  
===========================================================================*/
extern int ps_iface_get_nat_ipsec_vpn_pass_through
(
  ps_iface_ioctl_nat_ipsec_vpn_pass_through_type * vpn_passthrough_ptr
);

/*===========================================================================
FUNCTION PS_IFACE_SET_NAT_IPSEC_VPN_PASS_THROUGH

DESCRIPTION
  This function set NAT IPSEC VPN PassThrough

PARAMETERS
  vpn_passthrough_ptr: ptr to the IPSEC VPN PassThrough

RETURN VALUE
   0 if successful
   -1 if fails

DEPENDENCIES

SIDE EFFECTS
  
===========================================================================*/
extern int ps_iface_set_nat_ipsec_vpn_pass_through
(
  ps_iface_ioctl_nat_ipsec_vpn_pass_through_type * vpn_passthrough_ptr
);

/*===========================================================================
FUNCTION ps_iface_set_nat_l2tp_vpn_pass_through

DESCRIPTION
  This function enable/disable NAT L2TP VPN passthrough

PARAMETERS
  vpn_pass_through_ptr: Ptr to VPN Passthrough

RETURN VALUE

DEPENDENCIES

SIDE EFFECTS
  
===========================================================================*/
extern int ps_iface_set_nat_l2tp_vpn_pass_through
(
  ps_iface_ioctl_nat_l2tp_vpn_pass_through_type *vpn_pass_through_ptr
);

/*===========================================================================
FUNCTION ps_iface_get_nat_l2tp_vpn_pass_through

DESCRIPTION
  This function return whether L2TP VPN Passthrough in NAT is enabled or not

PARAMETERS
  vpn_pass_through_ptr: Ptr to VPN Passthrough

RETURN VALUE

DEPENDENCIES

SIDE EFFECTS
  
===========================================================================*/
extern int ps_iface_get_nat_l2tp_vpn_pass_through
(
  ps_iface_ioctl_nat_l2tp_vpn_pass_through_type *vpn_pass_through_ptr
);

/*===========================================================================
FUNCTION ps_iface_set_nat_pptp_vpn_pass_through

DESCRIPTION
  This function enable/disable NAT PPTP VPN passthrough

PARAMETERS
  vpn_pass_through_ptr: Ptr to VPN Passthrough

RETURN VALUE

DEPENDENCIES

SIDE EFFECTS
  
===========================================================================*/
extern int ps_iface_set_nat_pptp_vpn_pass_through
(
  ps_iface_ioctl_nat_pptp_vpn_pass_through_type *vpn_pass_through_ptr
);

/*===========================================================================
FUNCTION ps_iface_get_nat_pptp_vpn_pass_through

DESCRIPTION
  This function return whether PPTP VPN Passthrough in NAT is enabled or not

PARAMETERS
  vpn_pass_through_ptr: Ptr to VPN Passthrough

RETURN VALUE

DEPENDENCIES

SIDE EFFECTS
  
===========================================================================*/
extern int ps_iface_get_nat_pptp_vpn_pass_through
(
  ps_iface_ioctl_nat_pptp_vpn_pass_through_type *vpn_pass_through_ptr
);

/*===========================================================================
FUNCTION PS_NAT_ADD_DMZ

DESCRIPTION
  This function enables and creates a DMZ entry

PARAMETERS
  dmz_entry: ptr to the DMZ IP addr

RETURN VALUE
   0 if successful
   -1 if fails

DEPENDENCIES

SIDE EFFECTS
  
===========================================================================*/
extern int ps_nat_add_dmz
(
  ps_iface_ioctl_dmz_type * dmz_entry
);

/*===========================================================================
FUNCTION PS_NAT_GET_DMZ

DESCRIPTION
  This function gets a DMZ entry

PARAMETERS
  dmz_entry: ptr to the DMZ IP addr

RETURN VALUE
   0 if successful
   -1 if fails

DEPENDENCIES

SIDE EFFECTS
  
===========================================================================*/
extern int ps_nat_get_dmz
(
  ps_iface_ioctl_dmz_type * dmz_entry
);

/*===========================================================================
FUNCTION PS_NAT_DELETE_DMZ

DESCRIPTION
  This function deletes DMZ

PARAMETERS
  None

RETURN VALUE
   0 if successful
   -1 if fails

DEPENDENCIES

SIDE EFFECTS
  
===========================================================================*/
extern int ps_nat_delete_dmz
(
  void
);

/*===========================================================================
FUNCTION PS_NAT_GET_CONFIG_ENTRY_TIMEOUT

DESCRIPTION
  This function gets the default entry timeout

PARAMETERS
  proto - transport protocol type

RETURN VALUE
  timeout value

DEPENDENCIES

SIDE EFFECTS
  
===========================================================================*/
uint16 ps_nat_get_config_entry_timeout
( 
  uint8 proto
);

/*===========================================================================
FUNCTION PS_NAT_GET_CONFIG_VPN_PASS_THROUGH

DESCRIPTION
  This function gets whether VPN pass through is enabled

PARAMETERS
  None

RETURN VALUE
   TRUE if VPN pass through is enabled
   FALSE otherwise

DEPENDENCIES

SIDE EFFECTS
  
===========================================================================*/
boolean ps_nat_get_config_vpn_pass_through
( 
  void 
);

/*===========================================================================
FUNCTION PS_NAT_GET_CONFIG_PPTP_VPN_PASS_THROUGH

DESCRIPTION
  This function gets whether PPTP VPN pass through (GRE proto)  is enabled

PARAMETERS
  None

RETURN VALUE
   TRUE if VPN pass through is enabled
   FALSE otherwise

DEPENDENCIES

SIDE EFFECTS
  
===========================================================================*/
boolean ps_nat_get_config_pptp_vpn_pass_through
(
  void 
);

/*===========================================================================
FUNCTION PS_NAT_GET_CONFIG_DMZ_ENABLED

DESCRIPTION
  This function gets whether DMZ is enabled

PARAMETERS
  None

RETURN VALUE
   TRUE if DMZ is enabled
   FALSE otherwise

DEPENDENCIES

SIDE EFFECTS
  
===========================================================================*/
boolean ps_nat_get_config_dmz_enabled
( 
  void 
);

/*===========================================================================
FUNCTION PS_NAT_GET_CONFIG_DMZ_IP

DESCRIPTION
  This function gets configured DMZ IP address

PARAMETERS
  None

RETURN VALUE
  IP address

DEPENDENCIES

SIDE EFFECTS
  
===========================================================================*/
uint32 ps_nat_get_config_dmz_ip
( 
  void 
);

/*===========================================================================
FUNCTION PS_NAT_GET_AVAILABLE_PORT

DESCRIPTION
  This function gets the next available port.
  If ICMP protocol is specified, the next available ICMP
    id is returned.

PARAMETERS
  proto - transport protocol

RETURN VALUE
  port/id value

DEPENDENCIES

SIDE EFFECTS
  
===========================================================================*/
uint16 ps_nat_get_available_port
( 
  uint8 proto 
);

/*===========================================================================
FUNCTION PS_NAT_GET_CONFIG_RESV_PORT_RANGE

DESCRIPTION
  This function gets the configured TCP/UDP port range.
  If ICMP protocol is specified, then the id range is returned.

PARAMETERS
  proto - transport protocol

RETURN VALUE
  port/id range

DEPENDENCIES

SIDE EFFECTS
  
===========================================================================*/
uint16 ps_nat_get_config_resv_port_range
( 
  uint8 proto 
);

#ifdef __cplusplus
}
#endif

#endif /* FEATURE_DATA_PS_NAT_IFACE  */
#endif /* FEATURE_DATA_PS */
#endif  /* PS_NAT_H */
