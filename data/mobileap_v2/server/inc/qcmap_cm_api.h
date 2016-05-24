#ifndef _QCMAP_CM_API_H_
#define _QCMAP_CM_API_H_

/******************************************************************************

                           QCMAP_CM_API.H

******************************************************************************/

/******************************************************************************

  @file    qcmap_cm_api.h
  @brief   Mobile AP Connection Manager Lib API

  DESCRIPTION
  Header file for Mobile AP Connection Manager Lib.

  ---------------------------------------------------------------------------
  Copyright (c) 2011-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

when       who        what, where, why
--------   ---        -------------------------------------------------------
07/11/12   gk         9x25
10/26/12   cp         Added support for Dual AP and different types of NAT.
12/19/12   sb         Added support for RNDIS/ECM USB tethering
02/27/13   cp         Added support for deprecating of prefix when switching
					betweenstation mode and WWAN mode.
04/17/13   mp         Added support to get IPv6 WWAN/STA mode configuration.
06/12/13   sg         Added DHCP Reservation Feature
******************************************************************************/

/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/

#include <stdbool.h>
#include "comdef.h"
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include <linux/if_addr.h>
#include "ps_ipfltr_defs.h"
#include "ps_iface.h"
#include "qualcomm_mobile_access_point_msgr_v01.h"
#include "qmi_client.h"
#include "dsi_netctrl.h"

#define QCMAP_CM_QMI_TIMEOUT_VALUE     15000
#define QCMAP_COEX_TIMEOUT_VALUE       1000
#define QCMAP_COEX_MAX_RETRY           5
#define MAX_COMMAND_STR_LEN 200
#define DATA_SIZE 512
#define IPV4_ADDR_LEN 4
#define IPV6_ADDR_LEN 16
#define DSI_PROFILE_3GPP2_OFFSET (1000)
#define DSI_PROFILE_NUM_MAX      (9999)
#define DSI_IP_FAMILY_4   "IP"
#define DSI_IP_FAMILY_6   "IPV6"
#define DSI_IP_FAMILY_4_6 "IPV4V6"
#define QCMAP_DEFAULT_DSS_INIT_TIME    6
#define QCMAP_DSI_UDS_FILE "/etc/qcmap_dsi_uds_file"
#define QCMAP_CMDQ_UDS_FILE "/etc/qcmap_cmdq_uds_file"
#define QCMAP_NAS_UDS_FILE "/etc/qcmap_nas_uds_file"
#define QCMAP_TIMER_UDS_FILE "/etc/qcmap_timer_uds_file"
#define QCMAP_STA_UDS_FILE "/etc/qcmap_sta_uds_file"
#define AUTO_CONNECT_TIMER 1
#define QCMAP_MSGR_INTF_LEN 6
/* WIFI iDriver/Firmware Init Delay Micro seconds */
#define WIFI_DEV_INIT_DELAY  50000
#define WIFI_DEV_INIT_DELAYS_MAX 5000000
#define DHCP_LEASE_TIME "43200" /*iLease time in seconds */
#define MIN_DHCP_LEASE 120 /*Lease time in seconds */
#define MAX_WIFI_CLIENTS 20

#define USB_GW_IP_OFFSET 1
#define USB_IP_OFFSET 2
#define DHCP_IP_OFFSET 4
#define NUM_USB_ADDR 4
#define MIN_DHCP_ADDR_RANGE 7

/* Default Timeout Values. */
#define QCMAP_NAT_ENTRY_DEFAULT_GENERIC_TIMEOUT 200
#define QCMAP_NAT_ENTRY_DEFAULT_ICMP_TIMEOUT 30
#define QCMAP_NAT_ENTRY_DEFAULT_TCP_TIMEOUT 3600
#define QCMAP_NAT_ENTRY_DEFAULT_UDP_TIMEOUT 60
#define QCMAP_NAT_ENTRY_MIN_TIMEOUT 30

#define MAC_NULL_STR "00:00:00:00:00:00" /*MAC Null String*/
#define QCMAP_PROCESS_KILL_WAIT_MS  50000 /* 50 mili seconds*/
#define QCMAP_PROCESS_KILL_RETRY  40
#define QCMAP_BRIDGE_MAX_RETRY 10
#define QCMAP_BRIDGE_MAX_TIMEOUT_MS 500 /* Micro seconds */
#define QCMAP_QTI_MSG_TIMEOUT_S 2
typedef enum
{
  QCMAP_MSGR_INTF_AP_INDEX          = 0x00,
  QCMAP_MSGR_INTF_GUEST_AP_INDEX    = 0x01,
  QCMAP_MSGR_INTF_STATION_INDEX     = 0x02
} qcmap_cm_intf_index_type;

typedef enum
{
  QCMAP_CM_PROFILE_FULL_ACCESS         = 0x01,
  QCMAP_CM_PROFILE_INTERNET_ONLY       = 0x02
} qcmap_cm_access_profile_type;

typedef enum
{
  QCMAP_CM_DEVMODE_AP                  = 0x01,
  QCMAP_CM_DEVMODE_STA                 = 0x02
} qcmap_cm_devmode_type;

typedef enum
{
  QCMAP_MSGR_INPUT_CHAIN,
  QCMAP_MSGR_OUTPUT_CHAIN,
  QCMAP_MSGR_FORWARD_CHAIN,
} qcmap_msgr_iptable_chain;

/*----------------------------------------------------------------------------
  RNDIS and ECM enum types
----------------------------------------------------------------------------*/
typedef enum
{
  QCMAP_QTI_USB_LINK_RNDIS                      = 0x01,
  QCMAP_QTI_USB_LINK_ECM                        = 0x02
}qcmap_qti_usb_link_type;

#define IPTABLE_CHAIN 8

typedef struct {
  dsi_hndl_t handle;
  const char* tech;
  const char* family;
  int profile;
} dsi_call_info_t;



/*===========================================================================

                          PUBLIC DATA DECLARATIONS

===========================================================================*/
typedef struct qcmap_dsi_buffer_s {
  dsi_hndl_t            dsi_nethandle;
  void                  *user_data;
  dsi_net_evt_t         evt;
  dsi_evt_payload_t     *payload_ptr;
} qcmap_dsi_buffer_t;

typedef struct qcmap_nas_buffer_s {
  qmi_client_type user_handle;                    /* QMI user handle       */
  unsigned int    msg_id;                         /* Indicator message ID  */
  void           *ind_buf;                        /* Raw indication data   */
  unsigned int    ind_buf_len;                    /* Raw data length       */
  void           *ind_cb_data;                     /* User call back handle */
} qcmap_nas_buffer_t;

typedef enum
{
  AUTO_CONNECT_V4 = 0x0,
  AUTO_CONNECT_V6 = 0x1
}qcmap_timer_enum_t;

typedef struct qcmap_timer_buffer_s {
qcmap_timer_enum_t msg_id;                         /* Indicator message ID  */
} qcmap_timer_buffer_t;

typedef enum {
  STA_CONNECTED,
  STA_DISCONNECTED
}qcmap_sta_event_t;

typedef struct qcmap_sta_buffer_s {
  uint32 sta_cookie;
  qcmap_sta_event_t event;
}qcmap_sta_buffer_t;

/*---------------------------------------------------------------------------
           Port Forwarding Entry Configuration.
---------------------------------------------------------------------------*/
typedef struct
{
  uint32   port_fwding_private_ip;
  uint16   port_fwding_private_port;
  uint16   port_fwding_global_port;
  uint8    port_fwding_protocol;
} qcmap_cm_port_fwding_entry_conf_t;

/*---------------------------------------------------------------------------
           Extended FireWall Entry Configuration.
---------------------------------------------------------------------------*/
typedef struct
{
  ip_filter_type       filter_spec;
  uint32               firewall_handle;
} qcmap_msgr_firewall_entry_conf_t;

/*---------------------------------------------------------------------------
           Extended FireWall handle list configuration.
---------------------------------------------------------------------------*/
typedef struct
{
  uint32 handle_list[QCMAP_MSGR_MAX_FIREWALL_ENTRIES_V01];
  ip_version_enum_type ip_family;
  int num_of_entries;
} qcmap_msgr_get_firewall_handle_list_conf_t;

/*---------------------------------------------------------------------------
           Extended FireWall handle configuration.
---------------------------------------------------------------------------*/
typedef struct
{
  uint32 handle;
  ip_version_enum_type ip_family;
} qcmap_msgr_firewall_handle_conf_t;

/*---------------------------------------------------------------------------
           Extended FireWall configuration.
---------------------------------------------------------------------------*/
typedef union
{
  qcmap_msgr_firewall_entry_conf_t extd_firewall_entry;
  qcmap_msgr_get_firewall_handle_list_conf_t extd_firewall_handle_list;
  qcmap_msgr_firewall_handle_conf_t extd_firewall_handle;
} qcmap_msgr_firewall_conf_t;
/*---------------------------------------------------------------------------
   Connection type in STA mode.
---------------------------------------------------------------------------*/
typedef enum
{
  QCMAP_STA_CONNECTION_DYNAMIC = 0,
  QCMAP_STA_CONNECTION_STATIC
}qcmap_sta_connection_e;

/*---------------------------------------------------------------------------
   Static IP Configuration in STA mode.
---------------------------------------------------------------------------*/
typedef struct
{
  uint32 ip_addr;
  uint32 gw_ip;
  uint32 netmask;
  uint32 dns_addr;
}qcmap_sta_static_ip_config;

/*---------------------------------------------------------------------------
   User provided connection information in STA mode.
---------------------------------------------------------------------------*/
typedef struct
{
  qcmap_sta_connection_e conn_type;
  qcmap_sta_static_ip_config static_ip_config;
}qcmap_sta_connection_config;

/*---------------------------------------------------------------------------
   IPV6 Address Type.
---------------------------------------------------------------------------*/
typedef enum {
  LINK_LOCAL_IPV6_ADDR = 1,
  GLOBAL_IPV6_ADDR
} qcmap_ipv6_addr_t;

/*---------------------------------------------------------------------------
           NAT Configuration.
---------------------------------------------------------------------------*/

#define QCMAP_CM_MAX_FILE_LEN          120

typedef struct
{
  uint32   nat_entry_generic_timeout;
  uint32   nat_entry_icmp_timeout;
  uint32   nat_entry_tcp_established_timeout;
  uint32   nat_entry_udp_timeout;
  uint32   dmz_ip; /* 0 mean disable DMZ */
  uint8    enable_ipsec_vpn_pass_through;
  uint8    enable_pptp_vpn_pass_through;
  uint8    enable_l2tp_vpn_pass_through;

  uint8    num_port_fwding_entries;
  qcmap_cm_port_fwding_entry_conf_t
           port_fwding_entries[QCMAP_MSGR_MAX_SNAT_ENTRIES_V01];
  char     firewall_config_file[QCMAP_CM_MAX_FILE_LEN];
  uint8_t  del_command[QCMAP_MSGR_MAX_FIREWALL_ENTRIES_V01][MAX_COMMAND_STR_LEN];
  uint8    num_firewall_entries;
  qcmap_msgr_firewall_conf_t
           *extd_firewall_entries[QCMAP_MSGR_MAX_FIREWALL_ENTRIES_V01];

  uint8    firewall_enabled;
  uint8    firewall_pkts_allowed;
  qcmap_msgr_nat_enum_v01
           nat_type;
  qcmap_msgr_nat_enum_v01
           prev_nat_type;
} qcmap_cm_nat_conf_t;

/*---------------------------------------------------------------------------
           LAN Configuration.
---------------------------------------------------------------------------*/
#define QCMAP_LAN_INVALID_QCMAP_HANDLE (-1)
#define QCMAP_LAN_INVALID_IFACE_INDEX  (-1)
#define QCMAP_LAN_MAX_IPV4_ADDR_SIZE   16    /* 3 dots + 4 * 3 #s + 1 null */
/* 3 Interfaces - Possible Modes: AP, AP+AP, STA+AP */
#define QCMAP_MAX_NUM_INTF             3

#define QCMAP_V4_DEFAULT_DEVICE_NAME "rmnet0"
#define QCMAP_V6_DEFAULT_DEVICE_NAME "rmnet1"

/*-------------------------------------------------------------------------
  USB config structure to store IP addresses for RNDIS/ECM tethering
--------------------------------------------------------------------------*/
typedef struct
{
  boolean enable;
  qcmap_qti_usb_link_type link_type;
  uint32 gateway_addr;
  uint32 subnet_mask;
  uint32 usb_ip_address;
  char   dhcp_lease_time[QCMAP_CM_MAX_FILE_LEN];
}qcmap_cm_usb_conf_t;

typedef struct
{
  /*------------ Interface parameters common to STA,AP modes ------------- */
  /* Enable and configure main interface. */
  boolean  enable;
  qcmap_cm_devmode_type devmode;

  /*------------ Interface parameters specific to AP Mode ---------------- */
  /* Path to WLAN AP config which contain SSID/Mode/Encryption info */
  char     path_to_hostapd_conf[QCMAP_CM_MAX_FILE_LEN];
  /* Path to WLAN AP config which contain SSID/Mode/Encryption info in
   * Station Mode. Applicable only to Primary AP. */
  char     path_to_sta_mode_hostapd_conf[QCMAP_CM_MAX_FILE_LEN];

  /* Main interface configuration. All Addresses are in host order */
  uint32   a5_ip_addr;
  uint32   sub_net_mask;

  /* LL interface configuration. All Addresses are in host order */
  uint32   ll_ip_addr;

  /* Type of access main interface has to networks. */
  qcmap_msgr_access_profile_v01 access_profile;

  /* DHCP server config */
  boolean  enable_dhcpd;
  uint32   dhcp_start_address;
  uint32   dhcp_end_address;
  char     dhcp_lease_time[QCMAP_CM_MAX_FILE_LEN];

  /*------------ Interface parameters specific to STA Mode --------------- */
  /* Path to WLAN AP config which contain SSID/Mode/Encryption info */
  char     path_to_supplicant_conf[QCMAP_CM_MAX_FILE_LEN];
  /* Connection type for STA mode. */
  qcmap_msgr_sta_connection_enum_v01 conn_type;
  /* IP configuration for STATIC STA mode. */
  qcmap_msgr_sta_static_ip_config_v01 static_ip_config;
   /*DHCP Reservation*/
  uint32 num_dhcp_reservation_records;
  qcmap_msgr_dhcp_reservation_v01
          dhcp_reservation_records[QCMAP_MSGR_MAX_DHCP_RESERVATION_ENTRIES_V01];
} qcmap_cm_intf_conf_t;

typedef struct
{
  /* Interface information. */
  qcmap_cm_intf_conf_t interface[QCMAP_MAX_NUM_INTF];
  /*USB configuration information*/
  qcmap_cm_usb_conf_t  usb_conf;
  char     module[QCMAP_CM_MAX_FILE_LEN];
  qcmap_msgr_wlan_mode_enum_v01 wlan_mode;
  uint32   ll_subnet_mask;
  uint32   a5_rmnet_ip_addr;
  uint32   q6_ip_addr_facing_a5;
  uint32   usb_rmnet_ip_addr;
  uint32   q6_ip_addr_facing_usb_rmnet;
  uint32   nat_ip_addr;
  /* newly added field for 9x25 */
  uint32   um_iface_ip_addr_facing_a5;
  boolean  enable_ipv6;
  boolean  enable_ipv4;
  char     sta_interface[QCMAP_MSGR_INTF_LEN];
} qcmap_cm_lan_conf_t;

/*---------------------------------------------------------------------------
           WAN Configuration.
---------------------------------------------------------------------------*/
#define QCMAP_WAN_INVALID_QCMAP_HANDLE 0xFFFFFFFF
#define QCMAP_WAN_MAX_ERI_DATA_SIZE    256
#define QCMAP_WAN_TECH_ANY             0
#define QCMAP_WAN_TECH_3GPP            1
#define QCMAP_WAN_TECH_3GPP2           2
#define MAX_WAN_CON_TIMEOUT 120
#define CLOCKID CLOCK_REALTIME
#define SIG SIGUSR1

typedef struct
{
  struct
  {
    int  umts_profile_index;
    int  cdma_profile_index;
  }v4;
  struct
  {
    int  umts_profile_index;
    int  cdma_profile_index;
  }v6;
}qcmap_cm_profile_index;

typedef struct
{
  boolean  auto_connect;
  boolean  auto_connect_timer_running_v4;
  timer_t  timerid_v4;
  int      auto_timer_value_v4;
  boolean  auto_connect_timer_running_v6;
  timer_t  timerid_v6;
  int      auto_timer_value_v6;
  boolean  roaming;
  char     eri_config_file[QCMAP_CM_MAX_FILE_LEN];

  int      tech;
  int      ip_family;
  qcmap_cm_profile_index
           profile_id;
} qcmap_cm_wan_conf_t;

typedef struct
{
  int upnp_config;
  int dlna_config;
  int mdns_config;
} qcmap_cm_srvc_conf_t;

typedef struct
{
  boolean enable_wlan_at_bootup;
  boolean enable_mobileap_at_bootup;
}qcmap_cm_bootup_conf_t;

/*---------------------------------------------------------------------------
           Master Mobile AP Config.
---------------------------------------------------------------------------*/
typedef struct
{
  qcmap_cm_nat_conf_t nat_config;
  qcmap_cm_wan_conf_t wan_config;
  qcmap_cm_lan_conf_t lan_config;
  qcmap_cm_srvc_conf_t srvc_config;
  qcmap_cm_bootup_conf_t bootup_config;
} qcmap_cm_conf_t;

/*---------------------------------------------------------------------------
           Embedded Profile
---------------------------------------------------------------------------*/
typedef struct
{
  int      tech;
  int      umts_profile_index;
  int      cdma_profile_index;
  int      ip_family;

} qcmap_cm_embd_conf;

typedef struct qcmap_cm_nl_prefix_info_s {
  boolean prefix_info_valid;
  unsigned char prefix_len;
  struct sockaddr_storage prefix_addr;
  struct ifa_cacheinfo          cache_info;
} qcmap_cm_nl_prefix_info_t;

/*---------------------------------------------------------------------------
                    Return values indicating error status
---------------------------------------------------------------------------*/
#define QCMAP_CM_SUCCESS               0         /* Successful operation   */
#define QCMAP_CM_ERROR                -1         /* Unsuccessful operation */

/*---------------------------------------------------------------------------
           Error Condition Values
---------------------------------------------------------------------------*/
#define QCMAP_CM_ENOERROR              0        /* No error                */
#define QCMAP_CM_EWOULDBLOCK           1        /* Operation would block   */
#define QCMAP_CM_EINVAL                2        /* Invalid operation       */
#define QCMAP_CM_EOPNOTSUPP            3        /* Operation not supported */
#define QCMAP_CM_EBADAPP               4        /* Invalid application ID  */
#define QCMAP_CM_ENOWWAN               5        /* WWAN not connected      */
#define QCMAP_CM_EALDCONN              6        /* Already connected  */
#define QCMAP_CM_EALDDISCONN           7       /*  Already disconnected  */
#define QCMAP_MSGR_ENTRY_PRESENT       -8
#define QCMAP_MSGR_ENTRY_FULL          -9
#define QCMAP_MSGR_INVALID_PARAM       -10

#define SA_FAMILY(addr)         (addr).sa_family
#define SA_DATA(addr)           (addr).sa_data
#define SASTORAGE_FAMILY(addr)  (addr).ss_family
#define SASTORAGE_DATA(addr)    (addr).__ss_padding

/*---------------------------------------------------------------------------
           Mobile AP Events
---------------------------------------------------------------------------*/
typedef enum
{
  QCMAP_CM_EVENT_ENABLED = 0,
  QCMAP_CM_EVENT_STA_CONNECTED,
  QCMAP_CM_EVENT_STA_DISCONNECTED,
  QCMAP_CM_EVENT_WAN_CONNECTING,
  QCMAP_CM_EVENT_WAN_CONNECTING_FAIL,
  QCMAP_CM_EVENT_WAN_IPv6_CONNECTING_FAIL,
  QCMAP_CM_EVENT_WAN_CONNECTED,
  QCMAP_CM_EVENT_WAN_IPv6_CONNECTED,
  QCMAP_CM_EVENT_WAN_DISCONNECTED,
  QCMAP_CM_EVENT_WAN_IPv6_DISCONNECTED,
  QCMAP_CM_EVENT_DISABLED
} qcmap_cm_event_e;

/*---------------------------------------------------------------------------
   Type representing enumeration of QCMAP CM states
---------------------------------------------------------------------------*/
typedef enum
{
  QCMAP_CM_DISABLE = 0,
  QCMAP_CM_ENABLE,
  QCMAP_CM_WAN_CONNECTING,
  QCMAP_CM_WAN_DISCONNECTING,
  QCMAP_CM_WAN_CONNECTED
} qcmap_cm_state_e;

typedef enum
{
  QCMAP_CM_V6_DISABLE = 0,
  QCMAP_CM_V6_ENABLE,
  QCMAP_CM_V6_WAN_CONNECTING,
  QCMAP_CM_V6_WAN_DISCONNECTING,
  QCMAP_CM_V6_WAN_CONNECTED
} qcmap_cm_v6_state_e;

#define QCMAP_CM_IND_V4_WAN_CONNECTED            1002
#define QCMAP_CM_IND_V4_WAN_DISCONNECTED         1003
#define QCMAP_CM_IND_V6_WAN_CONNECTED            1004
#define QCMAP_CM_IND_V6_WAN_DISCONNECTED         1005

/*---------------------------------------------------------------------------
           Mobile AP CM call back declarations.
---------------------------------------------------------------------------*/
typedef void (*qcmap_cm_cb_fcn)
(
  int                 handle,                  /* Mobile AP Application id */
  qcmap_cm_event_e    event,                   /* Type of Mobile AP Event  */
  void               *qcmap_cm_cb_user_data,    /* Call back User data      */
  dsi_ce_reason_t    *callend_reason
);

/*---------------------------------------------------------------------------
  Mobile AP Statistics
---------------------------------------------------------------------------*/
typedef struct
{
  uint64     bytes_rx;
  uint64     bytes_tx;
  uint32     pkts_rx;
  uint32     pkts_tx;
  uint32     pkts_dropped_rx;
  uint32     pkts_dropped_tx;
}qcmap_cm_statistics_t;

#ifdef __cplusplus
extern "C" {
#endif
/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/
/*===========================================================================
  FUNCTION stop_auto_timer
===========================================================================*/
/*!
@brief
  Funtion to stop auto timer

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
extern void stop_auto_timer(ip_version_enum_type family);

/*===========================================================================
  FUNCTION qcmap_auto_connect_handler
===========================================================================*/
/*!
@brief
   Function to post message to initiate connect backhaul
@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
extern void qcmap_auto_connect_handler(int sig, siginfo_t *si, void *uc );
/*===========================================================================
   FUNCTION  qcmap_cm_process_dsi_net_cb_fcn
===========================================================================*/
/*!
@brief
    QCMAP CM DSS net callback

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
extern void qcmap_cm_process_dsi_net_evt
(
dsi_hndl_t hndl,
void * user_data,
dsi_net_evt_t evt,
dsi_evt_payload_t *payload_ptr
);

extern void
qcmap_cm_process_qmi_nas_ind
(
  qmi_client_type user_handle,                    /* QMI user handle       */
  unsigned int    msg_id,                         /* Indicator message ID  */
  void           *ind_buf,                        /* Raw indication data   */
  unsigned int    ind_buf_len,                    /* Raw data length       */
  void           *ind_cb_data                     /* User call back handle */
);

extern void qcmap_cm_process_qmi_timer_ind ( unsigned int msg_id );

extern void
qcmap_cm_process_sta_ind
(
  uint32 sta_cookie,
  qcmap_sta_event_t event
);
/*===========================================================================

FUNCTION QCMAP_CM_ENABLE()

DESCRIPTION

  Enable Mobile AP CM based on the config.
  It will register Mobile AP event callback.
  It will configure Modem in Mobile AP Mode and bring up RmNet between Q6 and A5.
  It will also bring up LAN if it is config.

DEPENDENCIES
  None.

RETURN VALUE
  Returns Mobile AP CM application ID on success.

  On error, return 0 and place the error condition value in
  *qcmap_cm_errno.

SIDE EFFECTS

===========================================================================*/
extern int qcmap_cm_enable
(
  qcmap_cm_conf_t   *qcmap_cm_cfg,              /* Config for Mobile AP CM */
  qcmap_cm_cb_fcn    qcmap_cm_cb,               /* Callback function       */
  void              *qcmap_cm_cb_user_data,     /* Callback user data      */
  int               *qcmap_cm_errno,            /* Error condition value   */
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================

FUNCTION QCMAP_CM_DISABLE()

DESCRIPTION

  Disable Mobile AP CM.
  It will teardown LAN.
  It will configure Modem in non-Mobile AP mode.

DEPENDENCIES
  None.

RETURN VALUE
  On success, returns QCMAP_CM_SUCCESS.
  On error, return QCMAP_CM_ERROR and places the error condition value in
  *qcmap_cm_errno.

SIDE EFFECTS

===========================================================================*/
extern int qcmap_cm_disable
(
  int    qcmap_cm_handle,                      /* Handle for Mobile AP CM  */
  int   *qcmap_cm_errno,                       /* Error condition value    */
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================

FUNCTION QCMAP_CM_ENABLE_IPV6()

DESCRIPTION
  Enable IPV6 Functionality.

DEPENDENCIES
  None.

RETURN VALUE
  On success, returns QCMAP_CM_SUCCESS.
  On error, return QCMAP_CM_ERROR and places the error condition value in
  *qcmap_cm_errno.

SIDE EFFECTS
  None.

===========================================================================*/
int qcmap_cm_enable_ipv6
(
  int    qcmap_cm_handle,                         /* Handler for QCMAP CM  */
  qmi_error_type_v01 *qmi_err_num                 /* error condition value */
);


/*===========================================================================

FUNCTION QCMAP_CM_DISABLE_IPV6()

DESCRIPTION
  Disable IPV6 Functionality.

DEPENDENCIES
  None.

RETURN VALUE
  On success, returns QCMAP_CM_SUCCESS.
  On error, return QCMAP_CM_ERROR and places the error condition value in
  *qcmap_cm_errno.

SIDE EFFECTS
  None.

===========================================================================*/
int qcmap_cm_disable_ipv6
(
  int    qcmap_cm_handle,                           /* Handle for QCMAP CM  */
  int     *qcmap_cm_errno,
  qmi_error_type_v01 *qmi_err_num                   /* error condition value */
);

/*===========================================================================

FUNCTION QCMAP_CM_ENABLE_IPV4()

DESCRIPTION
  Enable IPV4 backhaul Functionality.

DEPENDENCIES
  None.

RETURN VALUE
  On success, returns QCMAP_CM_SUCCESS.
  On error, return QCMAP_CM_ERROR and places the error condition value in
  *qcmap_cm_errno.

SIDE EFFECTS
  None.

===========================================================================*/
int qcmap_cm_enable_ipv4
(
  int    qcmap_cm_handle,                         /* Handler for QCMAP CM  */
  qmi_error_type_v01 *qmi_err_num                 /* error condition value */
);


/*===========================================================================

FUNCTION QCMAP_CM_DISABLE_IPV4()

DESCRIPTION
  Disable IPV4 Functionality.

DEPENDENCIES
  None.

RETURN VALUE
  On success, returns QCMAP_CM_SUCCESS.
  On error, return QCMAP_CM_ERROR and places the error condition value in
  *qcmap_cm_errno.

SIDE EFFECTS
  None.

===========================================================================*/
int qcmap_cm_disable_ipv4
(
  int    qcmap_cm_handle,                           /* Handle for QCMAP CM  */
  int     *qcmap_cm_errno,
  qmi_error_type_v01 *qmi_err_num                   /* error condition value */
);

/*===========================================================================

FUNCTION QCMAP_CM_CONNECT_BACKHAUL()

DESCRIPTION

  It will bringup WWAN.

DEPENDENCIES
  None.

RETURN VALUE
  If WAN is already connected, returns QCMAP_CM_SUCCESS.
  return QCMAP_CM_ERROR and places the error condition value in
  *qcmap_cm_errno.

  qcmap_cm_errno Values
  ----------------
  QCMAP_CM_EBADAPP           invalid application ID specified
  QCMAP_CM_EWOULDBLOCK       the operation would block
  QCMAP_CM_EOPNOTSUPP        backhaul bringup/teardown in progress


SIDE EFFECTS

===========================================================================*/
extern int qcmap_cm_connect_backhaul
(
  int    qcmap_cm_handle,                      /* Handle for Mobile AP CM  */
  qcmap_msgr_wwan_call_type_v01    call_type,  /* Call type to be brought UP. */
  int    *qcmap_cm_errno,                       /* Error condition value    */
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================

FUNCTION QCMAP_CM_DISCONNECT_BACKHAUL()

DESCRIPTION

  It will teardown WWAN.

DEPENDENCIES
  None.

RETURN VALUE
  If WAN is already disconnected, returns QCMAP_CM_SUCCESS.
  return QCMAP_CM_ERROR and places the error condition value in
  *qcmap_cm_errno.

  qcmap_cm_errno Values
  ----------------
  QCMAP_CM_EBADAPP           invalid application ID specified
  QCMAP_CM_EWOULDBLOCK       the operation would block
  QCMAP_CM_EOPNOTSUPP        backhaul bringup/teardown in progress


SIDE EFFECTS

===========================================================================*/
extern int qcmap_cm_disconnect_backhaul
(
  int    qcmap_cm_handle,                      /* Handle for Mobile AP CM  */
  qcmap_msgr_wwan_call_type_v01    call_type, /* Call type to be brought down. */
  int    *qcmap_cm_errno,                       /* error condition value    */
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================

FUNCTION QCMAP_CM_GET_STATE()

DESCRIPTION

  It will get QCMAP CM current state.

DEPENDENCIES
  None.

SIDE EFFECTS

===========================================================================*/

extern qcmap_cm_state_e qcmap_cm_get_state
(
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================

FUNCTION QCMAP_CM_GET_IPV6_STATE()

DESCRIPTION

  It will get QCMAP CM current IPV6 state.

DEPENDENCIES
  None.

SIDE EFFECTS

===========================================================================*/

extern qcmap_cm_v6_state_e qcmap_cm_get_ipv6_state
(
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
  FUNCTION:  qcmap_cm_write_xml

  DESCRIPTION

  This function write QCMAP CM XML based on QCMAP CM Cfg

DEPENDENCIES
  None.

RETURN VALUE

  QCMAP_CM_ERROR
  QCMAP_CM_SUCCESS

SIDE EFFECTS

===========================================================================*/
int qcmap_cm_write_xml
(
  char *xml_file,                                 /* Filename and path     */
  qcmap_cm_conf_t *config                         /* Mobile AP config data */
);

/*===========================================================================
FUNCTION:  qcmap_cm_write_firewall_xml

DESCRIPTION

  This function write QCMAP CM Firewall XML based on QCMAP CM Cfg

DEPENDENCIES
  None.

RETURN VALUE

  QCMAP_CM_ERROR
  QCMAP_CM_SUCCESS

SIDE EFFECTS

===========================================================================*/
int qcmap_cm_write_firewall_xml
(
  char *xml_file,                                 /* Filename and path     */
  qcmap_cm_conf_t *config                         /* Mobile AP config data */
);

/*===========================================================================
FUNCTION:  qcmap_cm_read_xml

DESCRIPTION

  This function read QCMAP CM XML and populate the QCMAP CM Cfg

DEPENDENCIES
  None.

RETURN VALUE

  QCMAP_CM_ERROR
  QCMAP_CM_SUCCESS

SIDE EFFECTS

===========================================================================*/
int qcmap_cm_read_xml
(
  char *xml_file,                                 /* Filename and path     */
  qcmap_cm_conf_t *config                         /* Mobile AP config data */
);


/*===========================================================================

FUNCTION:  qcmap_cm_read_firewall_xml

DESCRIPTION

  This function reads QCMAP CM Firewall XML and populate the QCMAP CM Cfg

DEPENDENCIES
  None.

RETURN VALUE

  QCMAP_CM_ERROR
  QCMAP_CM_SUCCESS

SIDE EFFECTS

===========================================================================*/
int qcmap_cm_read_firewall_xml
(
  char *xml_file,                                 /* Filename and path     */
  qcmap_cm_conf_t *config                         /* Mobile AP config data */
);


/*===========================================================================

FUNCTION QCMAP_CM_SET_AUTO_CONNECT()

DESCRIPTION

  It will set autoconnect.

DEPENDENCIES
  None.

RETURN VALUE

  qcmap_cm_errno Values
  ----------------
  On success, returns QCMAP_CM_SUCCESS.
  On error, return QCMAP_CM_ERROR and places the error condition value in
  *qcmap_cm_errno.

SIDE EFFECTS

===========================================================================*/
extern int qcmap_cm_set_auto_connect
(
  int    qcmap_cm_handle,                     /* Handle for Mobile AP CM   */
  int    *qcmap_cm_errno,                     /* Error condition value     */
  boolean auto_connect,                        /* Autoconnect Enable or Not */
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================

FUNCTION QCMAP_CM_SET_ROAMING()

DESCRIPTION

  It will set roaming.

DEPENDENCIES
  None.

RETURN VALUE

  qcmap_cm_errno Values
  ----------------
  On success, returns QCMAP_CM_SUCCESS.
  On error, return QCMAP_CM_ERROR and places the error condition value in
  *qcmap_cm_errno.

SIDE EFFECTS

===========================================================================*/
extern int qcmap_cm_set_roaming
(
  int    qcmap_cm_handle,                      /* Handle for Mobile AP CM  */
  qmi_error_type_v01 *qmi_err_num,             /* Error condition value    */
  boolean roaming                              /* Roaming Enable or Not    */
);


/*===========================================================================

FUNCTION QCMAP_CM_GET_IPV4_NET_CONF()

DESCRIPTION
  This Function get the ipv4 WWAN network configuration.

DEPENDENCIES
  None.

RETURN VALUE
  On success, returns QCMAP_CM_SUCCESS.
  On error, return QCMAP_CM_ERROR and places the error condition value in
  *qcmap_cm_errno.

SIDE EFFECTS

===========================================================================*/
extern int qcmap_cm_get_ipv4_net_conf
(
  int     qcmap_cm_handle,                     /* Handle for Mobile AP CM  */
  in_addr_t *public_ip,                           /* IP addr assigned to WWAN */
  uint32 *pri_dns_addr,                        /* Primary DNS IP address   */
  in_addr_t *sec_dns_addr,                        /* Secondary DNS IP address */
  in_addr_t *gw_addr,                             /* UM Iface default gw address */
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================

FUNCTION QCMAP_CM_GET_IPV6_NET_CONF()

DESCRIPTION
  This Function get the IPv6 WWAN network configuration.

DEPENDENCIES
  None.

RETURN VALUE
  On success, returns QCMAP_CM_SUCCESS.
  On error, return QCMAP_CM_ERROR and places the error condition value in
  *qcmap_cm_errno.

SIDE EFFECTS

===========================================================================*/
extern int qcmap_cm_get_ipv6_net_conf
(
  int                qcmap_cm_handle,   /* Handle for Mobile AP CM  */
  uint8_t            public_ip[],       /* IP addr assigned to WWAN */
  uint8_t            pri_dns_addr[],    /* Primary DNS IP address   */
  uint8_t            sec_dns_addr[],    /* Secondary DNS IP address */
  qmi_error_type_v01 *qmi_err_num
);


/*===========================================================================

FUNCTION QCMAP_CM_GET_DEV_NAME()

DESCRIPTION
  This function gets the rmnet device name for a dsi handle.

DEPENDENCIES
  None.

RETURN VALUE
  On success, returns QCMAP_CM_SUCCESS.
  On error, return QCMAP_CM_ERROR and places the error condition value in
  *qcmap_cm_errno.

SIDE EFFECTS

===========================================================================*/
extern int qcmap_cm_get_dev_name
(
  int     qcmap_cm_handle,                     /* Handle for Mobile AP CM  */
  qcmap_msgr_ip_family_enum_v01
          qcmap_dev_type,                      /* Dev type                 */
  char   *qcmap_dev_name,                      /* Device name              */
  int    *qcmap_cm_errno                       /* Error condition value    */
);

/*===========================================================================

FUNCTION QCMAP_CM_ENABLE_STA_MODE()

DESCRIPTION
  Enable QCMobileAP CM based on the config.
  It will register QCMAP event callback.
  It will configure Modem in QCMAP Mode and bring up RmNet between Q6 and A5.
  It will also bring up LAN if it is config.

DEPENDENCIES
  None.

RETURN VALUE
  Returns QCMobileAP CM application ID on success.
  On error, return 0 and places the error condition value in
  *qcmap_cm_errno.

SIDE EFFECTS
  None.

===========================================================================*/
int qcmap_cm_enable_sta_mode
(
  int    qcmap_cm_handle,                         /* Handler for QCMAP CM  */
  int   *qcmap_cm_errno                           /* error condition value */
);


/*===========================================================================

FUNCTION QCMAP_CM_DISABLE_STA_MODE()

DESCRIPTION
  Disable QCMAP CM.
  Send disable_sta_mode msg to modem.

DEPENDENCIES
  None.

RETURN VALUE
  Returns QCMAP CM application ID on success.
  On error, return 0 and places the error condition value in
  *qcmap_cm_errno.

SIDE EFFECTS
  None.

===========================================================================*/
int qcmap_cm_disable_sta_mode
(
  int    qcmap_cm_handle,                           /* Handle for QCMAP CM  */
  int   *qcmap_cm_errno                             /* error condition value */
);

/*===========================================================================


FUNCTION QCMAP_CM_ERI_READ_CONFIG()

DESCRIPTION
  This function reads the passed file name to store the ERI config for
  processing.

DEPENDENCIES
  None.

RETURN VALUE
  On success, returns QCMAP_CM_SUCCESS.
  On error, return QCMAP_CM_ERROR and places the error condition value in
  *qcmap_cm_errno.

SIDE EFFECTS

=============================================================================*/
int
qcmap_cm_eri_read_config
(
  int    qcmap_cm_handle,                           /* Mobile AP CM Handle   */
  char  *file_name,                                 /* ERI config file       */
  int   *qcmap_cm_errno                             /* error condition value */
);

/*===========================================================================

FUNCTION QCMAP_CM_GET_WWAN_STATISTICS()

DESCRIPTION
  This Function gets WWAN statistics

DEPENDENCIES
  None.

RETURN VALUE
  On success, returns QCMAP_CM_SUCCESS.
  On error, return QCMAP_CM_ERROR and places the error condition value in
  *qcmap_cm_errno.

SIDE EFFECTS

===========================================================================*/
extern int qcmap_cm_get_wwan_statistics
(
  int                             qcmap_cm_handle,/*Handle for Mobile AP CM*/
  qcmap_msgr_ip_family_enum_v01   ip_family,     /* V4 or V6 family        */
  qcmap_msgr_wwan_statistics_type_v01 *wwan_stats,    /* WWAN Statistics values */
  qmi_error_type_v01             *qmi_err_num
);

/*===========================================================================

FUNCTION QCMAP_CM_RESET_WWAN_STATISTICS()

DESCRIPTION
  This Function resets WWAN statistics

DEPENDENCIES
  None.

RETURN VALUE
  On success, returns QCMAP_CM_SUCCESS.
  On error, return QCMAP_CM_ERROR and places the error condition value in
  *qcmap_cm_errno.

SIDE EFFECTS

===========================================================================*/
extern int qcmap_cm_reset_wwan_statistics
(
  int                             qcmap_cm_handle,/*Handle for Mobile AP CM*/
  qcmap_msgr_ip_family_enum_v01   ip_family,     /* V4 or V6 family        */
  qmi_error_type_v01             *qmi_err_num
);

/*===========================================================================

FUNCTION COEX_GET_WWAN_STATUS()

DESCRIPTION
  This Function retrieves the LTE frequency used by WiFi-LTE coex feature.

DEPENDENCIES
  None.

RETURN VALUE
  On success, returns TRUE.
  On error, return FALSE.

SIDE EFFECTS

===========================================================================*/
boolean coex_get_wwan_status
(
int                              *lte_frequency
);
/*===========================================================================

FUNCTION QCMAP_CM_SET_WWAN_POLICY()

DESCRIPTION
  This Function gets WWAN profile index
DEPENDENCIES
  None.

RETURN VALUE
  On success, returns QCMAP_CM_SUCCESS.

SIDE EFFECTS

===========================================================================*/

int qcmap_cm_set_wwan_policy
(
  int                             qcmap_cm_handle,/*Handle for Mobile AP CM*/
  qcmap_msgr_net_policy_info_v01 wwan_policy,  /* WWAN config values */
  qmi_error_type_v01             *qmi_err_num
);
/*===========================================================================

FUNCTION QCMAP_CM_GET_DATA_BITRATE()

DESCRIPTION

  Returns the current data bitrates.

DEPENDENCIES
  None.

SIDE EFFECTS

===========================================================================*/

extern int qcmap_cm_get_data_bitrate
(
  int                             qcmap_cm_handle,/*Handle for Mobile AP CM*/
  qcmap_msgr_data_bitrate_v01    *data_rate,    /* WWAN Statistics values */
  qmi_error_type_v01             *qmi_err_num
);

#ifdef __cplusplus
}
#endif
#endif
