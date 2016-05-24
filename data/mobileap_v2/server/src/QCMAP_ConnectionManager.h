#ifndef _QCMAP_CONNECTION_MANAGER_H_
#define _QCMAP_CONNECTION_MANAGER_H_

/*====================================================

FILE:  QCMAP_ConnectionManager.h

SERVICES:
   QCMAP Connection Manager Class

=====================================================

  Copyright (c) 2011-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  when       who        what, where, why
  --------   ---        -------------------------------------------------------
  07/11/12   gk         9x25
  10/26/12   cp         Added support for Dual AP and different types of NAT.
  02/27/13   cp         Added support for deprecating of prefix when switching
                        between station mode and WWAN mode.
  04/16/13   mp         Added support to get IPv6 WWAN/STA mode configuration.
  06/12/13   sg         Added DHCP Reservation feature
===========================================================================*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/select.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include "stringl.h"
#include "ds_util.h"
#include "comdef.h"
#include "amssassert.h"
#include "ps_iface_defs.h"
#include "ds_Utils_DebugMsg.h"

#include "qcmap_cm_api.h"

//===================================================================
//              Class Definitions
//===================================================================

#define QCMAP_LAN_CONNECTED_TIMEOUT 60 // 60 seconds for LAN connected
#define QCMAP_WAN_CONNECTED_TIMEOUT 60 // 60 seconds for WAN connected
#define USB_SUB_NET_MASK                "255.255.255.252"

// Backhaul Interface
typedef enum{
  TYPE_IPV4  =1,
  TYPE_IPV6
}qcmap_backhaul_interface_type;


class QCMAP_ConnectionManager
{
public:

   QCMAP_ConnectionManager(char *xml_path = NULL);
   virtual ~QCMAP_ConnectionManager(void);
   /* ---------------------------MobileAP Execution---------------------------*/

   /* Enable MobileAP */
   boolean Enable(int *handle, void *cb_user_data, qmi_error_type_v01 *qmi_err_num);

   /* Disable MobileAP */
   boolean Disable(int *err_num, qmi_error_type_v01 *qmi_err_num);

   /* Intialize Bridge Interface. */
   void InitBridge();
   int CreateBridgeSocket(void);
   int UpdatePrefix(qcmap_cm_nl_prefix_info_t *prefix_info, boolean deprecate);
   void DelBridge();

  /* Disable MobileAP handle. */
  void DisableHandle(void);

   /* ConnectBackHaul */
  boolean ConnectBackHaul(qcmap_msgr_wwan_call_type_v01 call_type, int *err_num, qmi_error_type_v01 *qmi_err_num);

  /* Enable DNS */
   boolean EnableDNS(void);

   /* Enable/Disable NAT on A5 (In Station mode) */
   boolean EnableNATonA5(void);
   boolean DisableNATonA5(void);
   /*Misc NAT tasks to be done after NAT types is set */
   boolean EnableMiscNATTasks(void);

   /* DisconnectBackHaul */
   boolean DisconnectBackHaul(qcmap_msgr_wwan_call_type_v01 call_type, int *err_num, qmi_error_type_v01 *qmi_err_num);

   /* -----------------------------Modem Config-------------------------------*/

   /* Display the current modem configuration. */
   boolean DisplayModemConfig(void);

   /* Add a Static Nat Entry to the configuration and update XML file. */
   boolean AddStaticNatEntry(qcmap_cm_port_fwding_entry_conf_t* nat_entry, qmi_error_type_v01 *qmi_err_num);

   /* Delete a Static Nat Entry from the configuration and update XML file. */
   boolean DeleteStaticNatEntry(qcmap_cm_port_fwding_entry_conf_t* nat_entry, qmi_error_type_v01 *qmi_err_num);

   /* Get a Static Nat Entries from the configuration. */
   boolean GetStaticNatEntries(qcmap_msgr_snat_entry_config_v01 *snat_config, unsigned int* num_entries, qmi_error_type_v01 *qmi_err_num);

   /* Add a DMZ IP address to the configuration and update XML file. */
   boolean AddDMZ(uint32 dmz_ip, qmi_error_type_v01 *qmi_err_num);

   /* Get the DMZ IP address from the configuration . */
   boolean GetDMZ(uint32_t *dmz_ip, qmi_error_type_v01 *qmi_err_num);

   /* Delete a DMZ IP address from the configuration and update XML file. */
   boolean DeleteDMZ(qmi_error_type_v01 *qmi_err_num);

   /* Set NAT Timeout in the configuration and update XML file. */
   boolean SetNATEntryTimeout(uint16 timeout);

   /* Get NAT Timeout from the configuration and update XML file. */
   uint16 GetNATEntryTimeout(void);

   boolean SetAutoconnect(boolean enable, qmi_error_type_v01 *qmi_err_num);
   boolean GetAutoconnect(void);
   boolean SetRoaming(boolean enable, qmi_error_type_v01 *qmi_err_num);
   /* Get WWAN statistics */
   boolean GetWWANStatistics( qcmap_msgr_ip_family_enum_v01 ip_family,
                              qcmap_msgr_wwan_statistics_type_v01 *wwan_stats,
                              qmi_error_type_v01 *qmi_err_num);

   /* Reset WWAN statistics */
   boolean ResetWWANStatistics(qcmap_msgr_ip_family_enum_v01 ip_family,
                               qmi_error_type_v01 *qmi_err_num);

   boolean GetNetworkConfig(in_addr_t *public_ip,
                                uint32 *primary_dns,
                                in_addr_t *secondary_dns,
                                qmi_error_type_v01 *qmi_err_num);

   boolean GetIPv6NetworkConfig(uint8_t public_ip[],
                                    uint8_t primary_dns[],
                                    uint8_t secondary_dns[],
                                    qmi_error_type_v01 *qmi_err_num);

   /* -----------------------------Linux Config-------------------------------*/

   /* Bring up Linux LAN. */
   boolean EnableWLAN(qmi_error_type_v01 *qmi_err_num);
   /* Bring up Linux LAN. */
   void EnableWLANModule(char *);

   /*Misc WLAN tasks to be done after rmnet is enabled */
   boolean EnableMiscRoutingTasks(void);

   /* Enable IPV4 */
   boolean EnableIPV4(qmi_error_type_v01 *qmi_err_num);
   /* Disable IPV4 */
   boolean DisableIPV4(qmi_error_type_v01 *qmi_err_num);

   /* Enable IPV6 */
   boolean EnableIPV6(qmi_error_type_v01 *qmi_err_num);
   /* Disable IPV6 */
   boolean DisableIPV6(qmi_error_type_v01 *qmi_err_num);

   /* Enable IPV6 Forwarding */
   boolean EnableIPV6Forwarding(void);
   /* Disable IPV6 Forwarding */
   boolean DisableIPV6Forwarding(void);

   /* Get Backhaul IPV6 Prefix Info. */
   void GetIPV6PrefixInfo(char *devname,
                          qcmap_cm_nl_prefix_info_t   *ipv6_prefix_info);

   /* Bring down Linux LAN. */
   boolean DisableWLAN(qmi_error_type_v01 *qmi_err_num);
   void DisableWLANModule(void);

  /* Set LAN Mode. */
  boolean SetLANConfig(qcmap_msgr_lan_config_v01 *lan_config,
                       qmi_error_type_v01 *qmi_err_num);

  /* Get Configured LAN Mode */
  boolean GetLANConfig(qcmap_msgr_lan_config_v01 *lan_config,
                       qmi_error_type_v01 *qmi_err_num);

  /* Actiavte LAN Request. */
  boolean ActivateLAN(qmi_error_type_v01 *qmi_err_num);

  /* Set WLAN Mode. */
  boolean SetWLANConfig(qcmap_msgr_wlan_mode_enum_v01 wlan_mode,
                       qcmap_msgr_access_profile_v01 guest_access_profile,
                       qcmap_msgr_station_mode_config_v01 *station_config,
                       qmi_error_type_v01 *qmi_err_num);

  /* Get Configured LAN Mode */
  boolean GetWLANConfig(qcmap_msgr_wlan_mode_enum_v01 *wlan_mode,
                       qcmap_msgr_access_profile_v01 *guest_access_profile,
                       qcmap_msgr_station_mode_config_v01 *station_config,
                       qmi_error_type_v01 *qmi_err_num);

  /* Actiavte WLAN Request. */
  boolean ActivateWLAN(qmi_error_type_v01 *qmi_err_num);


  /* Get the Current LAN Status. */
  boolean GetWLANStatus(qcmap_msgr_wlan_mode_enum_v01 *wlan_mode, qmi_error_type_v01 *qmi_err_num);

  /* Activate HostapdConfig Request. */
  boolean ActivateHostapdConfig(qcmap_msgr_activate_hostapd_ap_enum_v01 ap_type,
                                qcmap_msgr_activate_hostapd_action_enum_v01 action_type,
                                qmi_error_type_v01 *qmi_err_num);

  boolean ActivateHostapdActionStart(qcmap_msgr_activate_hostapd_ap_enum_v01 ap_type,
                                     int ap_pid,
                                     int guest_ap_pid,
                                     qmi_error_type_v01 *qmi_err_num);

  boolean ActivateHostapdActionStop(qcmap_msgr_activate_hostapd_ap_enum_v01 ap_type,
                                    int ap_pid,
                                    int guest_ap_pid,
                                    qmi_error_type_v01 *qmi_err_num);

  boolean ActivateHostapdActionRestart(qcmap_msgr_activate_hostapd_ap_enum_v01 ap_type,
                                       int ap_pid,
                                       int guest_ap_pid,
                                       qmi_error_type_v01 *qmi_err_num);

  /* Actiavte Supplicant Request. */
  boolean ActivateSupplicantConfig(qmi_error_type_v01 *qmi_err_num);

   /* Enable STA mode (Concurrent AP+STA) */
   boolean EnableStaMode(void);

   /* Disable STA mode */
   boolean DisableStaMode(void);

   /* Boolean to indicate whether STA mode is connected or not. */
   boolean InStaMode(void);

   /* Handle STA Associated Event. */
   void ProcessStaAssoc(void);

   /* Handle STA Disassociation event. */
   void ProcessStaDisAssoc(void);

   /* Configure, start and stop the Linux HostAPD server. */
   boolean SetHostAPDConfig(int intf, char * cfg_path);
   boolean StopHostAPD(int intf);
   boolean StartHostAPD(int intf);

   /* Configure, start and stop the Linux DHCPD server. */
   boolean SetDHCPDConfig(int intf, uint32 start, uint32 end, char * leasetime);
   boolean StartDHCPD(void);
   boolean StopDHCPD(void);

  /* Start, stop and restart the linux MCAST routing daemon. */
   void StartMcastDaemon(void);
   void StopMcastDaemon(void);

  /* Start and Stop RADISH. */
   void StartRadish(void);
   void StopRadish(void);

   /* Used in case of Concurrent STA+AP mode */
   boolean AddSNATEntryOnA5(qcmap_cm_port_fwding_entry_conf_t* nat_entry, qmi_error_type_v01 *qmi_err_num);
   boolean DeleteSNATEntryOnA5(qcmap_cm_port_fwding_entry_conf_t* nat_entry, qmi_error_type_v01 *qmi_err_num);

      /*Add an Extended Firewall rule to the configuration*/
   int AddFireWallEntry(qcmap_msgr_firewall_conf_t* firewall_entry, qmi_error_type_v01 *qmi_err_num);

   int  SetFirewall(qcmap_msgr_firewall_conf_t* firewall_entry,boolean add_rule, qmi_error_type_v01 *qmi_err_num);
   int  SetFirewallV4(qcmap_msgr_firewall_conf_t* firewall_entry,boolean add_rule, qmi_error_type_v01 *qmi_err_num);
   int  SetFirewallV6(qcmap_msgr_firewall_conf_t* firewall_entry,boolean add_rule, qmi_error_type_v01 *qmi_err_num);

   /*Get an Extended Firewall rule from the configuration*/
   boolean GetFireWallEntry(qcmap_msgr_get_firewall_entry_resp_msg_v01* resp,uint32_t handle, qmi_error_type_v01 *qmi_err_num);

   /*Delete extended firewall rule from the configuration*/
   boolean DeleteFireWallEntry(qcmap_msgr_firewall_conf_t* firewall_entry, qmi_error_type_v01  *qmi_err_num);

   /*Get Firewall rule handles from the configuration*/
   boolean GetFireWallHandleList(qcmap_msgr_firewall_conf_t* firewall_entry, qmi_error_type_v01 *qmi_err_num);

   void Dump_firewall_conf( qcmap_msgr_firewall_entry_conf_t *firewall_entry);

   boolean AddDMZOnA5(uint32 dmz_ip, qmi_error_type_v01 *qmi_err_num);
   boolean DeleteDMZOnA5(uint32 dmz_ip, qmi_error_type_v01 *qmi_err_num);

   boolean SetIPSECVpnPassThrough(boolean enable, qmi_error_type_v01 *qmi_err_num);
   boolean SetIPSECVpnPassThroughOnA5(boolean enable, qmi_error_type_v01 *qmi_err_num);
   boolean GetIpsecVpnPassthroughFlag(uint8 *flag, qmi_error_type_v01 *qmi_err_num);

   boolean SetPPTPVpnPassThrough(boolean enable, qmi_error_type_v01 *qmi_err_num);
   boolean SetPPTPVpnPassThroughOnA5(boolean enable, qmi_error_type_v01 *qmi_err_num);
   boolean GetPptpVpnPassthroughFlag(uint8 *flag, qmi_error_type_v01 *qmi_err_num);

   boolean SetL2TPVpnPassThrough(boolean enable, qmi_error_type_v01 *qmi_err_num);
   boolean SetL2TPVpnPassThroughOnA5(boolean enable, qmi_error_type_v01 *qmi_err_num);
   boolean GetL2tpVpnPassthroughFlag(uint8 *flag, qmi_error_type_v01 *qmi_err_num);

   /* USB tethering for RNDIS/ECM */
   boolean SetupUSBLink(qcmap_qti_usb_link_type usb_link,int *err_num);
   boolean BringDownUSBLink(qcmap_qti_usb_link_type usb_link,int *err_num);

   /* NAT Type */
   boolean SetNatType(qcmap_msgr_nat_enum_v01 nat_type, qmi_error_type_v01 *qmi_err_num);
   boolean GetNatType(qcmap_msgr_nat_enum_v01 * cur_nat_type, qmi_error_type_v01 *qmi_err_num);

   /* NAT Timeout */
   boolean SetNatTimeout(qcmap_msgr_nat_timeout_enum_v01 timeout_type, uint32 timeout_value, qmi_error_type_v01 *qmi_err_num);
   boolean GetNatTimeout(qcmap_msgr_nat_timeout_enum_v01 timeout_type, uint32 *timeout_value, qmi_error_type_v01 *qmi_err_num);
   boolean SetNatTimeoutOnA5(qcmap_msgr_nat_timeout_enum_v01 timeout_type, uint32 timeout_value, qmi_error_type_v01 *qmi_err_num);
   boolean GetNatTimeoutOnA5(qcmap_msgr_nat_timeout_enum_v01 timeout_type, uint32 *timeout_value, qmi_error_type_v01 *qmi_err_num);

   /* Use for Signal the LAN/WAN connected */
   pthread_mutex_t             cm_mutex;
   pthread_cond_t              cm_cond;
   boolean                     enable_dns;
   /* Which mode is wifi brought up in */
   qcmap_msgr_wlan_mode_enum_v01     wifi_mode;
   /* Is STA connected */
   boolean                     sta_connected;
   /* Autoconnect state prior to STA mode.*/
   boolean                     auto_connect;
   /* Index of STA iface in interfaces array in lan_config */
   int                         sta_iface_index;
   /* eth device number for first AP iface */
   int                         ap_dev_num;
   /*To track qcmap tear down*/
   boolean                     qcmap_tear_down_in_progress;
   /* To check whether bridge interface is initialized or not. */
   boolean                     bridge_inited;
/* Socket used for sending RA's to deprecate prefix. */
   int                         bridge_sock;
   qcmap_cm_nl_prefix_info_t  ipv6_prefix_info;

   int readable_addr(int domain,const uint32_t *addr, char *str);
   boolean GetWWANStatus(qcmap_msgr_wwan_call_type_v01 call_type, uint8_t *status, qmi_error_type_v01 *qmi_err_num);
   int GetMobileAPhandle(qmi_error_type_v01 *qmi_err_num);
   boolean GetRoaming();

   boolean CheckforAddrConflict( qcmap_msgr_lan_config_v01 *lan_config,
                                 qcmap_msgr_station_mode_config_v01 *station_config );

  boolean SetFirewallConfig( boolean enable_firewall,
                             boolean pkts_allowed,
                             qmi_error_type_v01 *qmi_err_num);
  boolean GetFirewallConfig( boolean *enable_firewall,
                             boolean *pkts_allowed,
                             qmi_error_type_v01 *qmi_err_num);
  boolean GetIPv4State(uint8_t *status, qmi_error_type_v01 *qmi_err_num);
  boolean GetIPv6State(uint8_t *status, qmi_error_type_v01 *qmi_err_num);
  boolean GetWWANPolicy(qcmap_msgr_net_policy_info_v01 *wwan_policy, qmi_error_type_v01 *qmi_err_num );
  boolean SetWWANPolicy(qcmap_msgr_net_policy_info_v01 wwan_policy, qmi_error_type_v01 *qmi_err_num );

  /* UPnP Restart*/
  qcmap_msgr_upnp_mode_enum_v01                 upnp_state;
  /* Enable UPnP daemon */
  boolean EnableUPNP(qmi_error_type_v01 *qmi_err_num);
  /* Disable UPnP daemon */
  boolean DisableUPNP(qmi_error_type_v01 *qmi_err_num);
  /* Interim Disable UPnP daemon */
  boolean InterimDisableUPNP(qmi_error_type_v01 *qmi_err_num);
  /* Restart UPnP daemon */
  boolean RestartUPNP(qmi_error_type_v01 *qmi_err_num);
  /* Return status of the UPnP daemon */
  boolean GetUPNPStatus(qcmap_msgr_upnp_mode_enum_v01 *status, qmi_error_type_v01 *qmi_err_num);

  /* DLNA Restart*/
  qcmap_msgr_dlna_mode_enum_v01                 dlna_state;
  /* Enable DLNA daemon */
  boolean EnableDLNA(qmi_error_type_v01 *qmi_err_num);
  /* Disable DLNA daemon */
  boolean DisableDLNA(qmi_error_type_v01 *qmi_err_num);
  /* Interim Disable DLNA daemon */
  boolean InterimDisableDLNA(qmi_error_type_v01 *qmi_err_num);
  /* Restart DLNA daemon */
  boolean RestartDLNA(qmi_error_type_v01 *qmi_err_num);
  /* Return status of the DLNA daemon */
  boolean GetDLNAStatus(qcmap_msgr_dlna_mode_enum_v01 *status, qmi_error_type_v01 *qmi_err_num);
  /* Set the DLNA Media Dir */
  boolean SetDLNAMediaDir(char media_dir[], qmi_error_type_v01 *qmi_err_num);
  /* Get the DLNA Media Dir */
  boolean GetDLNAMediaDir(char media_dir[], qmi_error_type_v01 *qmi_err_num);

  // Internal M-DNS state flag
  qcmap_msgr_mdns_mode_enum_v01                 mdns_state;
  /* Enable M-DNS daemon */
  boolean EnableMDNS(qmi_error_type_v01 *qmi_err_num);
  /* Disable M-DNS daemon */
  boolean DisableMDNS(qmi_error_type_v01 *qmi_err_num);

  /* Return status of the UPnP daemon */
  boolean GetMDNSStatus(qcmap_msgr_mdns_mode_enum_v01 *status, qmi_error_type_v01 *qmi_err_num);

  boolean EnableIPV6Firewall(void);
  void FlushIPV6Firewall();
  void FlushIPV4Firewall();

  // Utility functions to check for update in lan/wlan configs
  boolean IsLanCfgUpdated(void);
  inline boolean IsWlanModeUpdated(){return (this->wifi_mode != this->cfg.lan_config.wlan_mode); }
  inline boolean IsGuestProfileUpdated(){ return  (this->prev_guest_profile != this->cfg.lan_config.interface[QCMAP_MSGR_INTF_GUEST_AP_INDEX].access_profile); }
  boolean IsStaCfgUpdated(void);

  inline set_activate_lan_in_progress(boolean val) { activate_lan_in_progress = val; }
  inline boolean is_activate_lan_in_progress() { return activate_lan_in_progress; }

  void UpdateAccessProfileRules(void);
  void InstallGuestAPAccessRules(void);
  void DeleteGuestAPAccessRules(void);

  /* Set/get QCMAP bootup fucntions*/
  boolean SetQCMAPBootupConfig(qcmap_msgr_bootup_flag_v01 mobileap_enable, qcmap_msgr_bootup_flag_v01 wlan_enable, qmi_error_type_v01 *qmi_err_num);
  boolean GetQCMAPBootupConfig(qcmap_msgr_bootup_flag_v01 *mobileap_enable, qcmap_msgr_bootup_flag_v01 *wlan_enable, qmi_error_type_v01 *qmi_err_num);
  // Get MobileAPEnable on bootup Flag
  inline boolean get_mobileap_enable_cfg_flag(void) { return cfg.bootup_config.enable_mobileap_at_bootup; }
  // Get WLAN Enable on bootup flag
  inline boolean get_wlan_enable_cfg_flag(void) { return cfg.bootup_config.enable_wlan_at_bootup; }

  /* Holds IPv4 public address of a connected interface*/
  in_addr_t ipv4_public_ip;

  /* Holds IPv4 gateway address of a connected interface*/
  in_addr_t ipv4_default_gw_addr;

  /* Get the data bitrates */
  boolean GetDataBitrate(qcmap_msgr_data_bitrate_v01 *data_rate, qmi_error_type_v01 *qmi_err_num);

  /* Set/Get the UPnP notify interval */
  boolean SetUPNPNotifyInterval(int notify_int, qmi_error_type_v01 *qmi_err_num);
  boolean GetUPNPNotifyInterval(int *notify_int, qmi_error_type_v01 *qmi_err_num);

  /* Set/Get the UPnP notify interval */
  boolean SetDLNANotifyInterval(int notify_int, qmi_error_type_v01 *qmi_err_num);
  boolean GetDLNANotifyInterval(int *notify_int, qmi_error_type_v01 *qmi_err_num);

  boolean IsDLNAKilled();

 /*DHCP Reservation*/
 /* Add a DHCP Reservation Record to the configuration and update XML file. */
 boolean AddDHCPReservRecord(qcmap_msgr_dhcp_reservation_v01* dhcp_reservation_record,
                             qmi_error_type_v01 *qmi_err_num);
 /* Edit a DHCP Reservation Record from the configuration and update XML file. */
 boolean EditDHCPReservRecord(uint32_t *client_ip,qcmap_msgr_dhcp_reservation_v01 *dhcp_reservation_record,
                               qmi_error_type_v01 *qmi_err_num);
 /* Delete a DHCP Reservation Record from the configuration and update XML file. */
 boolean DeleteDHCPReservRecord(uint32_t *client_reserved_ip,qmi_error_type_v01 *qmi_err_num);
 /* Get DHCP Reservation Records from the XML configuration. */
 boolean GetDHCPReservRecords( qcmap_msgr_dhcp_reservation_v01* dhcp_reservation_record,
                               unsigned int* num_entries, qmi_error_type_v01 *qmi_err_num);
 boolean IsHostapdkilled(int omit_pid);
 boolean IsWpaClikilled();
 boolean IsWpaSupplicantkilled();
 boolean activate_wlan_in_progress;
 void sync_dhcp_hosts(void);
private:

   /* Read QCMAP config from XML file */
   boolean ReadConfigFromXML(void);

   /* Write QCMAP config to XML file */
   boolean WriteConfigToXML(void);

   /*Read QCMAP extended firewall config from XML */
   boolean ReadConfigFromFirewallXML(void);

   /*Write QCMAP extended firewall config to XML */
   boolean WriteConfigToFirewallXML(void);

   boolean DisconnectSTA(void);

   boolean get_backhaul_name_for_firewall(qcmap_backhaul_interface_type interface, char* devname);
   boolean SetDefaultFirewallRule(qcmap_backhaul_interface_type interface);
   void SetDefaultFirewall(void);
   boolean EnableIPV6DefaultFirewall(void);

   /* Find IP address assigned to the STA interface and its netmask */
   boolean GetStaIP(uint32 *staIP, uint32 *netMask);

   /* Find Gateway IP address assigned to the STA interface */
   boolean GetGatewayIP(uint32 *gwIP);

   /* XML file name */
   char                        xml_path[QCMAP_CM_MAX_FILE_LEN];

   /*Firewall XML file name*/
   char                        firewall_xml_path[QCMAP_CM_MAX_FILE_LEN];

   /* QCMAP CM Config */
   boolean                     qcmap_enable;
   qcmap_cm_conf_t             cfg;

   // Data structures used to store backup.
   qcmap_msgr_station_mode_config_v01 prev_station_mode_config;
   qcmap_msgr_lan_config_v01                prev_lan_config;
   qcmap_msgr_access_profile_v01           prev_guest_profile;

   boolean activate_lan_in_progress;
   boolean ipv6_prefix_based_rules_added;

   int                         qcmap_cm_handle;
};

#endif
