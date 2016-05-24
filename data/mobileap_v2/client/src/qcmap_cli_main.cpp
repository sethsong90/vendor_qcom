
/*!
  @file
  qcmap_cli_main.cpp

  @brief
  basic QCMAP Command Line Module Client Main

  Copyright (c) 2011-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
*/
/*=========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

$Header: $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
07/11/12   gk      Created module.
10/26/12   cp      Added support for Dual AP and different types of NAT.
02/27/13   cp      Added support to get IPV6 WAN status.
04/17/13   mp      Added support to get IPv6 WWAN/STA mode configuration.
06/12/13   sg      Added support for DHCP reservation.
===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include <string>
#include "comdef.h"
#include "QCMAP_Client.h"
#include "limits.h"
#include "ds_util.h"
/* The original delay for dss_init was 6 seconds. Allowing this value to be
   set at runtime to determine if delay still necessary. */
#define QCMAP_DEFAULT_DSS_INIT_TIME    6
#define MAX_PORT_VALUE           65535
#define MIN_DHCP_LEASE 120 /*Lease time in seconds */
#define MIN_NOTIFY_INTERVAL 30
#define MAX_NOTIFY_INTERVAL 60000
#define MAC_HEX_STRING "0123456789abcdefABCDEF" /*MAC hex check*/
#define MAC_NULL_STRING "00:00:00:00:00:00" /*MAC Null String*/

uint8 mac_addr_int[QCMAP_MSGR_MAC_ADDR_LEN_V01]; /*byte array of mac address*/
char mac_addr_str[QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01]; /*char array of mac address*/

#define QCMAP_CLI_LOG(...)                         \
  fprintf( stderr, "\n%s %d:", __FILE__, __LINE__); \
  fprintf( stderr, __VA_ARGS__ )

boolean read_firewall_conf(qcmap_msgr_firewall_conf_t *extd_firewall_add);
void Dump_firewall_conf( qcmap_msgr_firewall_entry_conf_t *firewall_entry);

QCMAP_Client *QcMapClient = NULL;

/* Arry for CLI options, Should always have and even number of elements */
std::string options_list[]=
{
" 1. Display Current Config         ",
" 2. Delete SNAT Entry              ",
" 3. Add SNAT Entry                 ",
" 4. Get SNAT Config                ",
" 5. Set Roaming                    ",
" 6. Get Roaming                    ",
" 7. Delete DMZ IP                  ",
" 8. Add DMZ IP                     ",
" 9. Get DMZ IP                     ",
"10. Set IPSEC VPN Passthrough      ",
"11. Get IPSEC VPN Passthrough      ",
"12. Set PPTP VPN Passthrough       ",
"13. Get PPTP VPN Passthrough       ",
"14. Set L2TP VPN Passthrough       ",
"15. Get L2TP VPN Passthrough       ",
"16. Set Autoconnect Config         ",
"17. Get Autoconnect Config         ",
"18. Get WAN status                 ",
"19. Add Firewall Entry             ",
"20. Enable/Disable M-DNS           ",
"21. Enable/Disable UPnP            ",
"22. Enable/Disable DLNA            ",
"23. Display Firewalls              ",
"24. Delete Firewall Entry          ",
"25. Get WWAN Statistics            ",
"26. Reset WWAN Statistics          ",
"27. Get Network Configuration      ",
"28. Get NAT Type                   ",
"29. Set NAT Type                   ",
"30. Enable/Disable Mobile AP       ",
"31. Enable/Disable WLAN            ",
"32. Connect/Disconnect Backhaul    ",
"33. Get Mobile AP status           ",
"34. Set NAT Timeout                ",
"35. Get NAT Timeout                ",
"36. Set WLAN Config                ",
"37. Get WLAN Config                ",
"38. Activate WLAN                  ",
"39. Set  LAN Config                ",
"40. Get  LAN Config                ",
"41. Activate  LAN                   ",
"42. Get WLAN Status                ",
"43. Enable/Disable IPV6            ",
"44. Set Firewall Config            ",
"45. Get Firewall Config            ",
"46. Get IPv6 State                 ",
"47. Get WWAN Profile               ",
"48. Set WWAN Profile               ",
"49. Get UPnP Status                ",
"50. Get DLNA Status                ",
"51. Get MDNS Status                ",
"52. Get Station Mode Status        ",
"53. Set DLNA Media Directory       ",
"54. Get DLNA Media Directory       ",
"55. Set MobileAP/WLAN Bootup Config",
"56. Get MobileAP/WLAN Bootup Config",
"57. Enable/Disable IPV4            ",
"58. Get IPv4 State                 ",
"59. Get Data Bitrate               ",
"60. Set UPnP Notify Interval       ",
"61. Get UPnP Notify Interval       ",
"62. Set DLNA Notify Interval       ",
"63. Get DLNA Notify Interval       ",
"64. Add DHCP Reservation Record    ",
"65. Get DHCP Reservation Records   ",
"66. Edit DHCP Reservation Record   ",
"67. Delete DHCP Reservation Record ",
"68. Activate Hostapd Config        ",
"69. Activate Supplicant Config     ",
"70. Teardown/Disable and Exit      "
};
/* Teardown/Disable and Exit should always be the last option.
  Always keep an even number of elements in array.  If array size is odd,
  add an emtpy placeholder to the end of the array.  When adding new options
  always replace empty options first.*/

/*===========================================================================
  FUNCTION  qcmap_msgr_qmi_qcmap_ind
  ===========================================================================*/
/*!
  @brief
  Processes an incoming QMI QCMAP Indication.

  @return
  void

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
void qcmap_msgr_qmi_qcmap_ind
(
 qmi_client_type user_handle,                    /* QMI user handle       */
 unsigned int    msg_id,                         /* Indicator message ID  */
 void           *ind_buf,                        /* Raw indication data   */
 unsigned int    ind_buf_len,                    /* Raw data length       */
 void           *ind_cb_data                     /* User call back handle */
 )
{
  qmi_client_error_type qmi_error;

  QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: user_handle %X msg_id %d ind_buf_len %d.",
          user_handle, msg_id, ind_buf_len);

  switch (msg_id)
  {
    case QMI_QCMAP_MSGR_BRING_UP_WWAN_IND_V01:
    {
      qcmap_msgr_bring_up_wwan_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(qcmap_msgr_bring_up_wwan_ind_msg_v01));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d",qmi_error);
        break;
     }

      /* Process packet service status indication for WWAN for QCMAP*/
      if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_CONNECTED_V01)
      {
        if (ind_data.mobile_ap_handle == QcMapClient->mobile_ap_handle)
        {
          QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: IPV4 WWAN Connected");
          return;
        }
      }
      else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_CONNECTING_FAIL_V01)
      {
        if (ind_data.mobile_ap_handle == QcMapClient->mobile_ap_handle)
        {
          QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: IPV4 WWAN Connecting Failed...WAN CallendType=%d CallendCode=%d", ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
          return;
        }
      }
      else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_IPV6_CONNECTED_V01)
      {
        if (ind_data.mobile_ap_handle == QcMapClient->mobile_ap_handle)
        {
          QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: IPV6 WWAN Connected");
          return;
        }
      }
      else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_IPV6_CONNECTING_FAIL_V01)
      {
        if (ind_data.mobile_ap_handle == QcMapClient->mobile_ap_handle)
        {
          QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: IPV6 WWAN Connecting Failed...WAN CallendType=%d CallendCode=%d", ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);

          return;
        }
      }

      break;
    }
    case QMI_QCMAP_MSGR_TEAR_DOWN_WWAN_IND_V01:
    {
      qcmap_msgr_tear_down_wwan_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(qcmap_msgr_tear_down_wwan_ind_msg_v01));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d",qmi_error);
        break;
      }

      if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_DISCONNECTED_V01)
      {
        if (ind_data.mobile_ap_handle == QcMapClient->mobile_ap_handle)
        {
          QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: IPV4 WWAN Disconnected...WAN CallendType=%d CallendCode=%d", ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);

          return;
        }
      }
      else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_DISCONNECTING_FAIL_V01)
      {
        if (ind_data.mobile_ap_handle == QcMapClient->mobile_ap_handle)
        {
          QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: IPV4 WWAN Disconnecting Failed...WAN CallendType=%d CallendCode=%d", ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
          return;
        }
      }
      else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_IPV6_DISCONNECTED_V01)
      {
        if (ind_data.mobile_ap_handle == QcMapClient->mobile_ap_handle)
        {
          QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: IPV6 WWAN Disconnected...WAN CallendType=%d CallendCode=%d", ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);

          return;
        }
      }
      else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_IPV6_DISCONNECTING_FAIL_V01)
      {
        if (ind_data.mobile_ap_handle == QcMapClient->mobile_ap_handle)
        {
          QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: IPV6 WWAN Disconnecting Failed...WAN CallendType=%d CallendCode=%d", ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
          return;
        }
      }

      break;
    }
    case QMI_QCMAP_MSGR_WWAN_STATUS_IND_V01:
    {
      qcmap_msgr_wwan_status_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(qcmap_msgr_wwan_status_ind_msg_v01));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d",qmi_error);
        break;
      }

      if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_DISCONNECTED_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: IPV4 WWAN Disconnected...WAN CallendType=%d CallendCode=%d", ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
        return;
      }
      else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_DISCONNECTING_FAIL_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: IPV4 WWAN Disconnecting Failed...WAN CallendType=%d CallendCode=%d", ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
        return;
      }
      else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_CONNECTED_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: IPv4 WWAN Connected...");
        return;
      }
      else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_CONNECTING_FAIL_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: IPV4 WWAN Connecting Failed...WAN CallendType=%d CallendCode=%d", ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
        return;
      }
      else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_IPV6_DISCONNECTED_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: IPV6 WWAN Disconnected...WAN CallendType=%d CallendCode=%d", ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
        return;
      }
      else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_IPV6_DISCONNECTING_FAIL_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: IPV6 WWAN Disconnecting Failed...WAN CallendType=%d CallendCode=%d", ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
        return;
      }
      else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_IPV6_CONNECTED_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: IPV6 WWAN Connected...");
        return;
      }
      else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_IPV6_CONNECTING_FAIL_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: IPV6 WWAN Connecting Failed...WAN CallendType=%d CallendCode=%d", ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
        return;
      }

      break;
    }
  case QMI_QCMAP_MSGR_MOBILE_AP_STATUS_IND_V01:
    {
      qcmap_msgr_mobile_ap_status_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(qcmap_msgr_mobile_ap_status_ind_msg_v01));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d",qmi_error);
        break;
      }

      if (ind_data.mobile_ap_status == QCMAP_MSGR_MOBILE_AP_STATUS_CONNECTED_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: Mobile AP Connected...");
        return;
      }
      else if (ind_data.mobile_ap_status == QCMAP_MSGR_MOBILE_AP_STATUS_DISCONNECTED_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: Mobile AP Disconnected...");
        return;
      }
      break;
    }

  case QMI_QCMAP_MSGR_STATION_MODE_STATUS_IND_V01:
    {
      qcmap_msgr_station_mode_status_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(qcmap_msgr_station_mode_status_ind_msg_v01));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d",qmi_error);
        break;
      }

      if (ind_data.station_mode_status == QCMAP_MSGR_STATION_MODE_CONNECTED_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: Mobile AP Station mode Connected...");
        return;
      }
      else if (ind_data.station_mode_status == QCMAP_MSGR_STATION_MODE_DISCONNECTED_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: Mobile AP Station mode Disconnected...");
        return;
      }
      break;
    }


  default:
    break;
}

  return;
}


/*===========================================================================
  FUNCTION sighandler
  ===========================================================================
  @brief
  Signal Handler
  @input
  signal- signal number
  @return
  void
  @dependencies
  Under lying os to generate the signal
  @sideefects
  None
  =========================================================================*/
void sighandler(int signal)
{
  qmi_error_type_v01 qmi_err_num;
  switch (signal)
  {
    case SIGTERM:
      if (QcMapClient)
        QcMapClient->DisableMobileAP(&qmi_err_num);

      exit(0);
      break;

    default:
      printf("Received unexpected signal %s\n", signal);
      break;
  }
}
/*===========================================================================
  FUNCTION  check_port
===========================================================================
 @brief
   Port value is validated against the range 1 - MAX_PORT_VALUE
 @input
   sport - port value
 @return
   0  - success
   -1 - failure
 @dependencies
   None
 @sideefects
   None
=========================================================================*/
int16
check_port (uint32 sport)
{
  if((sport > MAX_PORT_VALUE) || (sport < 1) )
  {
    printf(" port value should be between 1 - %d\n",MAX_PORT_VALUE);
    return -1;
  }
  else
    return 0;
}

/*===========================================================================
  FUNCTION  check_proto
===========================================================================
 @brief
   protocol value is validated against the range 1 - MAX_PROTO_VALUE
 @input
   sport - protocol value
 @return
   0  - success
   -1 - failure
 @dependencies
   None
 @sideefects
   None
=========================================================================*/
int16
check_proto (uint8 sport)
{
  if( sport > MAX_PROTO_VALUE )
  {
    printf(" port value should be between 1 - %d\n",MAX_PROTO_VALUE);
    return -1;
  }
  else
    return 0;
}

/*===========================================================================
  FUNCTION  check_tos
===========================================================================
 @brief
   Tos value is validated against the range 1 - MAX_TOS_VALUE
 @input
   tos - port value
 @return
   0  - success
   -1 - failure
 @dependencies
   None
 @sideefects
   None
=========================================================================*/
int16
check_tos (uint8 tos)
{
  if( tos > MAX_TOS_VALUE )
  {
    printf(" Tos value should be between 0 - %d\n",MAX_TOS_VALUE);
    return -1;
  }
  else
    return 0;
}

/*===========================================================================
  FUNCTION read_addr
  ===========================================================================
  @brief
  Read address funtion will read the address from the user
  @input
  domain - identifies ipv4 or ipv6 domain
  addr   - contains the numeric address, it's an output value.
  @return
  0  - success
  -1 - failure
  @dependencies
  It depends on inet_pton()
  @sideefects
  None
  =========================================================================*/
int read_addr(int domain,uint8 *addr)
{
  unsigned char buf[50]={0};
  char *ptr=NULL;

  bzero(buf,sizeof(buf));

  read_again:
  if ( domain == AF_INET )
  {
    printf("\n Please Enter V4 Address Value =>");
  }
  else
  if(domain == AF_INET6)
  {
     printf("\n Please Enter V6 Address Value =>");
  }
  else
  printf("\n Please Enter Address Mask Value =>");

  if ( fgets(buf, sizeof(buf), stdin) != NULL )
  {
    ptr = strchr((char *)&buf,'\n');
    *ptr = NULL;
    if ( inet_pton(domain,buf,addr) <=0 ) {
      printf("\n Address not in presentation format\n");
      goto read_again;
    }
  }
  return 0;
}

/*===========================================================================
  FUNCTION read_uint8
  ===========================================================================
  @brief
  Read uint8 value from user.
  @input
  void
  @return
  int value read from user.
  @dependencies
  usr to provide input
  @sideefects
  None
  =========================================================================*/

uint8 read_uint8()
{
  char  scan_string[50];
  uint8 result;
  uint32 src_addr[4]={0};
  bzero(scan_string,sizeof(scan_string));
  fgets(scan_string, sizeof(scan_string), stdin);
  result = atoi(scan_string);
  return result;
}

/*===========================================================================
  FUNCTION read_uint32
  ===========================================================================
  @brief
  Read integer value from user.
  @input
  void
  @return
  int value read from user.
  @dependencies
  usr to provide input
  @sideefects
  None
  =========================================================================*/
int read_uint32()
{
  char  scan_string[50];
  int result;
  bzero(scan_string,sizeof(scan_string));
  uint32 src_addr[4]={0};
  fgets(scan_string, sizeof(scan_string), stdin);
  result = atoi(scan_string);
  return result;
}

/*===========================================================================
  FUNCTION mac_addr_pton
  ===========================================================================
  @brief
  Read a char array and returns the byte array.
  @input:  char array
  void
  @return: bool
  @dependencies
  usr to provide input
  @sideefects
  None
  =========================================================================*/
bool mac_addr_pton(const char *mac_addr_str)
{
	int i;
  memset(mac_addr_int,0,QCMAP_MSGR_MAC_ADDR_LEN_V01);
  /* XX:XX:XX:XX:XX:XX */
  if ( strncmp(mac_addr_str,MAC_NULL_STRING,QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01) == 0 )
     return false;
  if (strlen(mac_addr_str) < QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01 - 1)
     return false;
  /* Don't dirty result unless string is valid MAC. */
  for (i = 0; i < QCMAP_MSGR_MAC_ADDR_LEN_V01; i++)
  {
	if (!strchr(MAC_HEX_STRING, mac_addr_str[i * 3]))
		return false;
	if (!strchr(MAC_HEX_STRING, mac_addr_str[i * 3 + 1]))
		return false;
	if ( ( i != QCMAP_MSGR_MAC_ADDR_LEN_V01 - 1 ) &&
       (( mac_addr_str[i * 3 + 2] != ':') && (mac_addr_str[i * 3 + 2] != '-' )) )
		return false;
  }
  for (i = 0; i < QCMAP_MSGR_MAC_ADDR_LEN_V01; i++)
  {
		mac_addr_int[i] = (ds_hex_to_dec(mac_addr_str[i * 3]) << 4) |
                       ds_hex_to_dec(mac_addr_str[i * 3 + 1]);
  }
  return true;
}

/*===========================================================================
  FUNCTION mac_addr_ntop
  ===========================================================================
  @brief
  Read a byte array and returns the char array.
  @input:  byte array
  @return: void
  @dependencies
  usr to provide input
  @sideefects
  None
  =========================================================================*/

void mac_addr_ntop(const uint8 *mac)
{
  memset(mac_addr_str,0,QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01);
  snprintf(mac_addr_str,QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01,"%02x:%02x:%02x:%02x:%02x:%02x",
	          mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  return;
}

#define GET_MAC_ADDR(scan_string) {\
    memset(scan_string,0,QCMAP_MSGR_MAX_FILE_PATH_LEN);\
    while(TRUE)\
    {\
          printf("Please input the MAC address  :");\
          if ( fgets(scan_string,sizeof(scan_string),stdin) != NULL )\
          {\
              if ( mac_addr_pton(scan_string))\
                break; \
          }\
          printf("Invalid Mac Address entered %s\n", scan_string);\
        }\
 }

#define GET_IP_ADDR(scan_string, ip_addr) {\
    memset(scan_string,0,QCMAP_MSGR_MAX_FILE_PATH_LEN);\
    while(TRUE)\
    {\
         printf("Please input the client reserved IP(xxx.xxx.xxx.xxx)  :");\
         if ( fgets(scan_string,sizeof(scan_string),stdin) != NULL )\
         {\
              if ( (inet_aton(scan_string, &ip_addr)) && \
                   ( ip_addr.s_addr != 0 ) &&\
                   ( ip_addr.s_addr != 0xffffffff ))\
               break; \
          }\
          printf("Invalid IPv4 Address entered: %s\n", scan_string);\
     }\
 }


/*===========================================================================
  FUNCTION read_firewall_conf
  ===========================================================================
  @brief
  Read firewall configuration value from user and calls add fireall entry.
  @input
  void
  @return
  true  - success
  false - failure
  @dependencies
  usr to provide input
  @sideefects
  None
  =========================================================================*/
boolean read_firewall_conf(qcmap_msgr_firewall_conf_t *extd_firewall_add)
{
  unsigned char buf[50];
  int val;
  memset(extd_firewall_add, 0, sizeof(qcmap_msgr_firewall_conf_t));

  while(1)
  {
    printf("\nPlease input IP family [4-IPV4/6-IPV6]:");
    val = read_uint32();
    if( val == IP_V4 || val== IP_V6 )
    break;
  }
  extd_firewall_add->extd_firewall_entry.filter_spec.ip_vsn = val;

  if(extd_firewall_add->extd_firewall_entry.filter_spec.ip_vsn == IP_V4)
  {
    printf("\nDo you want to enter IPV4 source address\n");
    printf("  and subnet mask: [1-YES 0-NO]:");

    if(read_uint32() == 1)

    {
      printf("\nPlease enter Src Addr [xxx.xxx.xxx.xxx]:");
      read_addr(AF_INET,(char *)&extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v4.src.addr.ps_s_addr);
      extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v4.field_mask |= IPFLTR_MASK_IP4_SRC_ADDR;
      printf("\nPlease enter IPV4 src subnet mask [xxx.xxx.xxx.xxx]:");
      read_addr(AF_INET,(uint8* )&extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v4.src.subnet_mask.ps_s_addr);
    }

    printf("\nDo you want to enter IPV4 TOS value\n");
    printf("and TOS mask: [1-YES 0-NO]:");
    if(read_uint32() == 1)
    {
      while(1)
      {
        printf("\nPlease enter the TOS value [0 to 255]:");
        val = read_uint32();
        if( check_tos(val)!= -1)
          break;
      }
      extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v4.tos.val =val;
      while(1)
      {
        printf("\nPlease enter the TOS mask [0 to 255]:");
        val = read_uint32();
        if( check_tos( val )!= -1)
          break;
      }
      extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v4.tos.mask = val;
      extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v4.field_mask |= IPFLTR_MASK_IP4_TOS;
    }
  }else
  {
    printf("\nDo you want to enter IPV6 source address\n"
           "and subnet mask: [1-YES 0-NO]:");
    if(read_uint32() == 1)
    {
      printf("\nPlease enter Src Addr [xxxx:xxxx::xxxx:xxxx:xxxx]:");
      read_addr(AF_INET6,(uint8 *)&extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v6.src.addr.in6_u.u6_addr8);
      extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v6.field_mask |= IPFLTR_MASK_IP6_SRC_ADDR;
      printf("\nPlease enter IPV6 src prefixlen:");
      extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v6.src.prefix_len = read_uint32();
    }

    printf("\nDo you want to enter IPV6 Traffic Class value\n"
           "and subnet mask: [1-YES 0-NO]:");
    if(read_uint32() == 1)
    {
      printf("\nPlease enter IPV6 Class value:");
      extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v6.trf_cls.val=read_uint32();
      extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v6.field_mask |= IPFLTR_MASK_IP6_TRAFFIC_CLASS;
      printf("\nPlease enter IPV6 class mask value:");
      extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v6.trf_cls.mask = read_uint32();
    }
  }

  printf("\nDo you want to enter Protocol [1-YES 0-NO]:");

  if(read_uint32() == 1)
  {
    read_proto_again:
    printf("\nPlease input IPV4 next header protocol\n");
    printf("TCP=%d, UDP=%d,ICMP=%d,ICMP6=%d,TCP_UDP=%d,ESP=%d",PS_IPPROTO_TCP, PS_IPPROTO_UDP, \
            PS_IPPROTO_ICMP, PS_IPPROTO_ICMP6, PS_IPPROTO_TCP_UDP, PS_IPPROTO_ESP);
    val = read_uint32();
    if( val == PS_IPPROTO_TCP   || val == PS_IPPROTO_UDP || val == PS_IPPROTO_ICMP || \
        val == PS_IPPROTO_ICMP6 || val == PS_IPPROTO_ESP || val == PS_IPPROTO_TCP_UDP )
    {
      if( extd_firewall_add->extd_firewall_entry.filter_spec.ip_vsn == IP_V4 )
      {
        extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v4.next_hdr_prot = val;
        extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v4.field_mask |= IPFLTR_MASK_IP4_NEXT_HDR_PROT;
      }
      else if(extd_firewall_add->extd_firewall_entry.filter_spec.ip_vsn == IP_V6 )
      {
        extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v6.next_hdr_prot = val;
        extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v6.field_mask |= IPFLTR_MASK_IP6_NEXT_HDR_PROT;
      }
    }else
      goto read_proto_again;
  }

  if(val == PS_IPPROTO_TCP)
  {
    printf("\nDo you want to enter Source Port and Range [1-YES 0-NO]:");
    if(read_uint32() == 1)
    {
      read_tcp_src_single_port:
      printf("\nPlease input TCP source port:");
      val = read_uint32();
      if( check_port (val) == -1)
        goto read_tcp_src_single_port;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp.src.port = val;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp.field_mask |= IPFLTR_MASK_TCP_SRC_PORT;

      read_tcp_start_port:
      printf("\nPlease enter TCP source port range:");
      val = read_uint32();
      if( check_port ((extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp.src.port+val)) == -1 )
        goto read_tcp_start_port;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp.src.range = val;
    }

    printf("\nDo you want to enter Destination Port and Range [1-YES 0-NO]:");
    val = read_uint32();
    if(val ==  1)
    {
      read_tcp_dst_port:
      printf("\nPlease input TCP destination port:");
      val = read_uint32();
      if( check_port (val) == -1)
        goto read_tcp_dst_port;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp.dst.port = val;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp.field_mask |= IPFLTR_MASK_TCP_DST_PORT;

      read_tcp_dst_port_range:
      printf("\nPlease enter TCP destination port range:");
      val = read_uint32();
      if( check_port ((extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp.dst.port + val)) == -1 )
        goto read_tcp_dst_port_range;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp.dst.range = val;
    }
  }else if( val == PS_IPPROTO_UDP)
  {
    printf("\nDo you want to enter Source Port and Range [1-YES 0-NO]:");
    val = read_uint32();
    if(val ==  1)
    {
      read_udp_src_single_port:
      printf("\nPlease input udp source port:");
      val = read_uint32();
      if( check_port (val) == -1)
        goto read_udp_src_single_port;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.udp.src.port = val;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.udp.field_mask |= IPFLTR_MASK_UDP_SRC_PORT;

      read_udp_start_port:
      printf("\nPlease enter udp source port range:");
      val = read_uint32();
      if( check_port ((extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.udp.src.port+val)) == -1 )
        goto read_udp_start_port;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.udp.src.range = val;
    }
    printf("\nDo you want to enter Destination Port and Range [1-YES 0-NO]:");
    val = read_uint32();
    if( val == 1)
    {
      read_udp_dst_port:
      printf("\nPlease input udp destination port:");
      val = read_uint32();
      if( check_port (val) == -1)
        goto read_udp_dst_port;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.udp.dst.port = val;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.udp.field_mask |= IPFLTR_MASK_UDP_DST_PORT;

      read_udp_dst_end_port:
      printf("\nPlease enter udp destination port range:");
      val = read_uint32();
      if( check_port ((extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.udp.dst.port+val)) == -1 )
        goto read_udp_dst_end_port;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.udp.dst.range = val;
    }
  }
  else if (val == PS_IPPROTO_TCP_UDP)
  {
    printf("\nDo you want to enter Source Port and Range [1-YES 0-NO]:");
    if(read_uint32() == 1)
    {
     read_tcp_udp_port_range_src_single_port:
     printf("\nPlease input tcp_udp source port:");
     val = read_uint32();
     if( check_port (val) == -1)
     goto read_tcp_udp_port_range_src_single_port;
     extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp_udp_port_range.src.port = val;
     extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp_udp_port_range.field_mask |= IPFLTR_MASK_TCP_UDP_SRC_PORT;

    read_tcp_udp_port_range_start_port:
     printf("\nPlease enter tcp_udp source port range:");
     val = read_uint32();
     if( check_port ((extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp_udp_port_range.src.port+val)) == -1 )
     goto read_tcp_udp_port_range_start_port;
     extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp_udp_port_range.src.range = val;
    }
    printf("\nDo you want to enter Source Port and Range [1-YES 0-NO]:");
    if(read_uint32() == 1)
    {
      read_tcp_udp_port_range_dst_port:
      printf("\nPlease input tcp_udp destination port:");
      val = read_uint32();
      if( check_port (val) == -1)
        goto read_tcp_udp_port_range_dst_port;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp_udp_port_range.dst.port = val;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp_udp_port_range.field_mask |= IPFLTR_MASK_TCP_UDP_DST_PORT;
      read_tcp_udp_port_range_dst_end_port:
      printf("\nPlease enter tcp_udp destination port range:");
      val = read_uint32();
      if( check_port ((extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp_udp_port_range.dst.port + val)) == -1 )
        goto read_tcp_udp_port_range_dst_end_port;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp_udp_port_range.dst.range = val;
    }
  }else if ( val == PS_IPPROTO_ICMP || val == PS_IPPROTO_ICMP6)
  {
    printf("\nDo you want to enter ICMP Type [1-YES 0-NO]:");
    if(read_uint32() == 1)
    {
      printf("\nPlease enter the ICMP Type value:");
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.icmp.type = read_uint32();
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.icmp.field_mask |= IPFLTR_MASK_ICMP_MSG_TYPE;
    }
    printf("\nDo you want to enter ICMP code [1-YES 0-NO]:");
    if(read_uint32() == 1)
    {
      printf("\nPlease enter ICMP code value:");
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.icmp.code = read_uint32();
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.icmp.field_mask |= IPFLTR_MASK_ICMP_MSG_CODE;
    }
  }else if( val == PS_IPPROTO_ESP)
  {
    printf("\nDo you want to enter ESP SPI value [1-YES 0-NO]:");
    if(read_uint32() == 1)
    {
      printf("\nPlease enter the ESP SPI value:");
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.esp.spi = read_uint32();
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.esp.field_mask |= IPFLTR_MASK_ESP_SPI;
    }
  }
  return true;
}

int readable_addr(int domain,uint32_t *addr, char *str)
{
  if (inet_ntop(domain, addr, str, INET6_ADDRSTRLEN) == NULL)
  {
  printf("\n Not in presentation format \n");
  return -1;
  }
}

void DisplayIPv6State()
{
  boolean ipv6_state;
  qmi_error_type_v01  qmi_err_num;

  memset((void *)&ipv6_state, 0, sizeof(uint8_t));

  if (QcMapClient->GetIPv6State( &ipv6_state, &qmi_err_num))
  {
    printf("\nIPV6 is: %s.\n",(ipv6_state)?"Enabled":"Disabled");
  }
  else
  {
    printf("\nGetIPV6State returns Error: 0x%x", qmi_err_num);
  }
}

void DisplayWWANPolicy()
{
  qcmap_msgr_net_policy_info_v01 wwan_policy;
  memset(&wwan_policy,0,sizeof(qcmap_msgr_net_policy_info_v01));
  qmi_error_type_v01  qmi_err_num;

  if (QcMapClient->GetWWANPolicy(&wwan_policy, &qmi_err_num))
  {
    printf("\nTech preference is :");
    switch (wwan_policy.tech_pref)
    {
      case 0:
        printf("ANY \n");
        break;

      case 1:
        printf("UMTS \n");
        break;

      case 2:
        printf("CDMA \n");
        break;

      default:
        printf("Error: invalid tech preference:- %d \n",wwan_policy.tech_pref);
        break;
    }
    printf("\nV4 3gpp profile id is: %d.\n",
            wwan_policy.v4_profile_id_3gpp);
    printf("\nV4 3gpp2 profile id is: %d.\n",
            wwan_policy.v4_profile_id_3gpp2);
    printf("\nV6 3gpp pofile id is: %d.\n",
            wwan_policy.v6_profile_id_3gpp);
    printf("\nV6 3gpp2 pofile id is: %d.\n",
            wwan_policy.v6_profile_id_3gpp2);
  }
  else
  {
    printf("\nGet WWAN policy failed Error 0x%x.\n", qmi_err_num);
  }
}

boolean DisplayFirewall(int handle_list_len,
                        qcmap_msgr_firewall_conf_t extd_firewall_handle_list)
{
  char str[INET6_ADDRSTRLEN];
  qcmap_msgr_firewall_conf_t extd_firewall_get;
  int   index;
  int   result;
  int next_hdr_prot = 0;
  qcmap_msgr_firewall_entry_conf_t *firewall_entry;
  qmi_error_type_v01  qmi_err_num;

  if(handle_list_len > 0)
  {
    for(index =0; index < handle_list_len; index++)
    {
      memset(&extd_firewall_get, 0, sizeof(qcmap_msgr_firewall_conf_t));
      extd_firewall_get.extd_firewall_entry.filter_spec.ip_vsn =
        extd_firewall_handle_list.extd_firewall_handle_list.ip_family;
      extd_firewall_get.extd_firewall_entry.firewall_handle =
        extd_firewall_handle_list.extd_firewall_handle_list.handle_list[index];

      if(QcMapClient->GetFireWallEntry(&extd_firewall_get.extd_firewall_entry, &qmi_err_num))
      {
        firewall_entry=&extd_firewall_get.extd_firewall_entry;
        printf("\n### Start Displaying firewall configuration of handle =%d ###",
	             extd_firewall_get.extd_firewall_entry.firewall_handle);
        if(firewall_entry!=NULL)
        {
          if( firewall_entry->filter_spec.ip_vsn == IP_V4 )
          {
            printf("\nIp version : IPv4");
            if( firewall_entry->filter_spec.ip_hdr.v4.field_mask & IPFLTR_MASK_IP4_SRC_ADDR)
            {
              readable_addr(AF_INET,&firewall_entry->filter_spec.ip_hdr.v4.src.addr.ps_s_addr,(char *)&str);
              printf("\nSRC Addr : %s",str);
              readable_addr(AF_INET,&firewall_entry->filter_spec.ip_hdr.v4.src.subnet_mask.ps_s_addr,(char *)&str);
              printf("\nSRC Addr Mask : %s",str);
            }
            else
              printf("\nSRC Addr : Any");

            if(  firewall_entry->filter_spec.ip_hdr.v4.field_mask & IPFLTR_MASK_IP4_TOS)
            {
              printf("\nTos value : %x ",firewall_entry->filter_spec.ip_hdr.v4.tos.val);
              printf("\nTos Mask : %x ",firewall_entry->filter_spec.ip_hdr.v4.tos.val);
            }
            else
              printf("\nTos value : Any");

            if( firewall_entry->filter_spec.ip_hdr.v4.field_mask & IPFLTR_MASK_IP4_NEXT_HDR_PROT )
            {
              next_hdr_prot = firewall_entry->filter_spec.ip_hdr.v4.next_hdr_prot;
            }
          }
          else
          {
            printf("\nIp version : Ipv6");
            if( firewall_entry->filter_spec.ip_hdr.v6.field_mask & IPFLTR_MASK_IP6_SRC_ADDR)
            {
              readable_addr(AF_INET6,(uint32 *)&firewall_entry->filter_spec.ip_hdr.v6.src.addr.in6_u.u6_addr32,(char *)&str);
              printf("\nSrc Addr : %s ",str);
              printf("\nSrc Prefixlen : %d ",firewall_entry->filter_spec.ip_hdr.v6.src.prefix_len);
            }
            else
              printf("\nSRC Addr : Any");

            if( extd_firewall_get.extd_firewall_entry.filter_spec.ip_hdr.v6.field_mask &
                IPFLTR_MASK_IP6_TRAFFIC_CLASS )
            {
              printf("\n IPV6 Traffic class value: %d",
                  extd_firewall_get.extd_firewall_entry.filter_spec.ip_hdr.v6.trf_cls.val);
              printf("\n IPV6 Traffic class mask: %d",
                  extd_firewall_get.extd_firewall_entry.filter_spec.ip_hdr.v6.trf_cls.mask);
            }

            if( firewall_entry->filter_spec.ip_hdr.v6.field_mask & IPFLTR_MASK_IP6_NEXT_HDR_PROT )
            {
              next_hdr_prot = firewall_entry->filter_spec.ip_hdr.v6.next_hdr_prot;
            }
          }
          switch(next_hdr_prot)
          {
            case PS_IPPROTO_TCP:
              printf("\nProtocol : TCP");
              if( firewall_entry->filter_spec.next_prot_hdr.tcp.field_mask & IPFLTR_MASK_TCP_SRC_PORT )
              {
                printf("\nSrc port : %d",firewall_entry->filter_spec.next_prot_hdr.tcp.src.port);
                printf("\nSrc portrange : %d",firewall_entry->filter_spec.next_prot_hdr.tcp.src.range);
              }
              if( firewall_entry->filter_spec.next_prot_hdr.tcp.field_mask & IPFLTR_MASK_TCP_DST_PORT )
              {
                printf("\nDst port : %d",firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port);
                printf("\nDst portrange : %d",firewall_entry->filter_spec.next_prot_hdr.tcp.dst.range);
              }
              break;
            case PS_IPPROTO_UDP:
              printf("\nProtocol: UDP");
              if( firewall_entry->filter_spec.next_prot_hdr.udp.field_mask & IPFLTR_MASK_UDP_SRC_PORT )
              {
                printf("\nSrc port : %d",firewall_entry->filter_spec.next_prot_hdr.udp.src.port);
                printf("\nDst portrange : %d",firewall_entry->filter_spec.next_prot_hdr.udp.src.range);
              }
              if( firewall_entry->filter_spec.next_prot_hdr.udp.field_mask & IPFLTR_MASK_UDP_DST_PORT )
              {
                printf("\nDst port : %d",firewall_entry->filter_spec.next_prot_hdr.udp.dst.port);
                printf("\nDst portrange : %d",firewall_entry->filter_spec.next_prot_hdr.udp.dst.range);
              }
              break;
            case PS_IPPROTO_TCP_UDP:
              printf("\nProtocol: TCP_UDP");
              if( firewall_entry->filter_spec.next_prot_hdr.udp.field_mask & IPFLTR_MASK_TCP_UDP_SRC_PORT )
              {
                printf("\nSrc port : %d",firewall_entry->filter_spec.next_prot_hdr.tcp_udp_port_range.src.port);
                printf("\nSrc portrange : %d",firewall_entry->filter_spec.next_prot_hdr.tcp_udp_port_range.src.range);

              }
              if( firewall_entry->filter_spec.next_prot_hdr.udp.field_mask & IPFLTR_MASK_TCP_UDP_DST_PORT )
              {
                printf("\nDst port : %d",firewall_entry->filter_spec.next_prot_hdr.tcp_udp_port_range.dst.port);
                printf("\nDst portrange : %d",firewall_entry->filter_spec.next_prot_hdr.tcp_udp_port_range.dst.range);
              }
              break;
            case PS_IPPROTO_ICMP:
              printf("\nProtocol : ICMP");
              if( firewall_entry->filter_spec.next_prot_hdr.icmp.field_mask & IPFLTR_MASK_ICMP_MSG_CODE )
              {
                printf("\nIcmp Type: %d ",firewall_entry->filter_spec.next_prot_hdr.icmp.type);
                printf("\nIcmp Type: %d ",firewall_entry->filter_spec.next_prot_hdr.icmp.code);
              }
              break;
            case PS_IPPROTO_ICMP6:
              printf("\nProtocol : ICMP6");
              if( firewall_entry->filter_spec.next_prot_hdr.icmp.field_mask & IPFLTR_MASK_ICMP_MSG_CODE )
              {
                printf("\nICMPv6 type: %d ",firewall_entry->filter_spec.next_prot_hdr.icmp.type);
              }
              break;
            case PS_IPPROTO_ESP:
              printf("\nProtocol : ESP");
              if( firewall_entry->filter_spec.next_prot_hdr.esp.field_mask & IPFLTR_MASK_ESP_SPI )
              {
                printf("\nESP spi : %d",firewall_entry->filter_spec.next_prot_hdr.esp.spi);

              }
              break;
            default:
              printf("\nProtocol : Any");
              break;
          }
          printf("\n### End of Firewall configuration of handle =%d ###",
		           extd_firewall_get.extd_firewall_entry.firewall_handle);
        }
      }
      else
      {
        printf("\nFirewall entry get failed, Error: 0x%x", qmi_err_num);
        break;
      }
     }
   }
   else
   {
      printf("\nNo Firewall Rules to Display \n" );
   }
}




void DisplayConfig()
{
  qcmap_msgr_firewall_conf_t extd_firewall_get[50];
  qcmap_msgr_firewall_conf_t extd_firewall_handle_list;
  qcmap_msgr_firewall_entry_conf_t *firewall_entry;
  qcmap_msgr_nat_enum_v01 nat_type;
  qcmap_msgr_wlan_mode_enum_v01 wlan_mode;
  qcmap_msgr_lan_config_v01 lan_config;
  qcmap_msgr_station_mode_config_v01 station_config;
  qcmap_msgr_bootup_flag_v01 mobileap_enable, wlan_enable;
  int   handle_list_len;
  int   index;
  int   result;
  in_addr tmpIP;
  int status;
  boolean roaming,pptp,ltp,autocon;
  uint32 dmz_ip=0;
  boolean vpn;
  int p_error=0;
  int i=0, num_entries=0;
  qcmap_nw_params_t qcmap_nw_params;
  char ipv6_addr_buf[INET6_ADDRSTRLEN];

  qcmap_msgr_snat_entry_config_v01 snat_config[QCMAP_MSGR_MAX_SNAT_ENTRIES_V01];
  char str[INET6_ADDRSTRLEN];
  in_addr addr;
  in_addr start,gateway,subnet_mask;
  uint32 leasetime=0;
  qmi_error_type_v01  qmi_err_num;
  qcmap_msgr_access_profile_v01 guest_ap_profile;
  uint32_t num_dhcp_entries=0;
  qcmap_msgr_dhcp_reservation_v01 dhcp_reserv_record[QCMAP_MSGR_MAX_DHCP_RESERVATION_ENTRIES_V01];

  memset(snat_config, 0, QCMAP_MSGR_MAX_SNAT_ENTRIES_V01*sizeof(qcmap_msgr_snat_entry_config_v01));
  QcMapClient->GetStaticNatConfig(snat_config, &num_entries, &qmi_err_num);
  QcMapClient->GetRoaming(&roaming, &qmi_err_num);
  QcMapClient->GetDMZ(&dmz_ip, &qmi_err_num);
  QcMapClient->GetIPSECVpnPassthrough(&vpn, &qmi_err_num);
  QcMapClient->GetPPTPVpnPassthrough(&pptp, &qmi_err_num);
  QcMapClient->GetL2TPVpnPassthrough(&ltp, &qmi_err_num);
  QcMapClient->GetAutoconnect(&autocon, &qmi_err_num);
 // QcMapClient->GetUSBConfig(&gateway.s_addr,&start.s_addr,&subnet_mask.s_addr,&leasetime);
  memset(&extd_firewall_handle_list, 0, sizeof(qcmap_msgr_firewall_conf_t));
  handle_list_len=0;
  extd_firewall_handle_list.extd_firewall_handle_list.ip_family = IP_V4;
  if(QcMapClient->GetFireWallHandlesList(&extd_firewall_handle_list.extd_firewall_handle_list,&qmi_err_num))
  {
    handle_list_len = extd_firewall_handle_list.extd_firewall_handle_list.num_of_entries;
  }
  if(handle_list_len > 0)
  {
    for(index =0; index < handle_list_len; index++)
    {
      memset(&extd_firewall_get[index], 0, sizeof(qcmap_msgr_firewall_conf_t));
      extd_firewall_get[index].extd_firewall_entry.filter_spec.ip_vsn =
        extd_firewall_handle_list.extd_firewall_handle_list.ip_family;
      extd_firewall_get[index].extd_firewall_entry.firewall_handle =
        extd_firewall_handle_list.extd_firewall_handle_list.handle_list[index];
      QcMapClient->GetFireWallEntry(&extd_firewall_get[index].extd_firewall_entry, &qmi_err_num);
    }
  }

  memset(&qcmap_nw_params,0,sizeof(qcmap_nw_params_t));

  QcMapClient->GetNatType(&nat_type, &qmi_err_num);
  uint32 timeout_value[5];
  uint32 timeout_type;
  p_error=0;
  timeout_type = QCMAP_MSGR_NAT_TIMEOUT_GENERIC_V01;
  while(timeout_type <= QCMAP_MSGR_NAT_TIMEOUT_UDP_V01)
  {
    QcMapClient->GetNatTimeout(timeout_type,(uint32 *)&timeout_value[timeout_type], &qmi_err_num);
    timeout_type++;
  }
  memset(&lan_config,0,sizeof(qcmap_msgr_lan_config_v01));
  memset(&station_config,0,sizeof(qcmap_msgr_station_mode_config_v01));
  QcMapClient->GetLANConfig(&lan_config,&qmi_err_num);
  QcMapClient->GetWLANConfig(&wlan_mode,&guest_ap_profile,&station_config,&qmi_err_num);
  printf("\nSNAT configuration:\n");
  for (i=0; i<num_entries; i++)
  {
    printf("Entry %d:\n",i);
    tmpIP.s_addr = ntohl(snat_config[i].private_ip_addr);
    printf("private ip: %s\n", inet_ntoa(tmpIP));
    printf("private port: %d\n", snat_config[i].private_port);
    printf("global port: %d\n", snat_config[i].global_port);
    printf("protocol: %d\n", snat_config[i].protocol);
  }
  printf("\nRoaming Mode: %s.\n",(roaming)?"Enabled":"Disabled");
  printf("\nDMZ IP:");
  if ( dmz_ip == 0 )
    printf(" No DMZ Configured!");
  else
  {
    tmpIP.s_addr = ntohl(dmz_ip);
    printf(" %s",inet_ntoa(tmpIP));
  }
  printf("\nIPSEC Passthrough Enable Flag : %d",vpn);
  printf("\nPPTP Passthrough Enable Flag : %d",pptp);
  printf("\nL2TP Passthrough Enable Flag : %d", ltp);
  printf("\nAuto Connect Mode: %s.\n",(autocon)?"Enabled":"Disabled");
  printf("\nFirewall Entries for Ipv4:");
  DisplayFirewall( handle_list_len, extd_firewall_handle_list );
  memset(&extd_firewall_handle_list, 0, sizeof(qcmap_msgr_firewall_conf_t));
  handle_list_len=0;
  extd_firewall_handle_list.extd_firewall_handle_list.ip_family = IP_V6;
  if(QcMapClient->GetFireWallHandlesList(&extd_firewall_handle_list.extd_firewall_handle_list,&qmi_err_num))
  {
    handle_list_len = extd_firewall_handle_list.extd_firewall_handle_list.num_of_entries;
  }
  if(handle_list_len > 0)
  {
    for(index =0; index < handle_list_len; index++)
    {
      memset(&extd_firewall_get[index], 0, sizeof(qcmap_msgr_firewall_conf_t));
      extd_firewall_get[index].extd_firewall_entry.filter_spec.ip_vsn =
        extd_firewall_handle_list.extd_firewall_handle_list.ip_family;
      extd_firewall_get[index].extd_firewall_entry.firewall_handle =
        extd_firewall_handle_list.extd_firewall_handle_list.handle_list[index];
      QcMapClient->GetFireWallEntry(&extd_firewall_get[index].extd_firewall_entry, &qmi_err_num);
    }
  }
  printf("\nFirewall Entries for Ipv6:");
  DisplayFirewall( handle_list_len, extd_firewall_handle_list );

  DisplayWWANPolicy();
  DisplayIPv6State();

  if (QcMapClient->GetNetworkConfiguration(
                                           QCMAP_MSGR_IP_FAMILY_V4_V01,
                                           &qcmap_nw_params,
                                           &qmi_err_num))
  {
    if(qmi_err_num == QMI_ERR_NONE_V01)
    {
      printf("\nIPv4 WWAN NetworkConfiguration:\n");
      qcmap_nw_params.v4_conf.public_ip.s_addr =
                        htonl(qcmap_nw_params.v4_conf.public_ip.s_addr);
      qcmap_nw_params.v4_conf.primary_dns.s_addr =
                        htonl(qcmap_nw_params.v4_conf.primary_dns.s_addr);
      qcmap_nw_params.v4_conf.secondary_dns.s_addr =
                        htonl(qcmap_nw_params.v4_conf.secondary_dns.s_addr);

      printf("Public IP for WWAN: %s \n",
             inet_ntoa(qcmap_nw_params.v4_conf.public_ip));
      printf("Primary DNS IP address: %s \n",
             inet_ntoa(qcmap_nw_params.v4_conf.primary_dns));
      printf("Secondary DNS IP address: %s \n",
             inet_ntoa(qcmap_nw_params.v4_conf.secondary_dns));
    }
    else
      printf("\nError in IPv4 config - 0x%x\n\n", qmi_err_num);
  }
  else
    printf("\n GetNetworkConfiguration fails\n");

  if (QcMapClient->GetNetworkConfiguration(
                                           QCMAP_MSGR_IP_FAMILY_V6_V01,
                                           &qcmap_nw_params,
                                           &qmi_err_num ))
  {
    if(qmi_err_num == QMI_ERR_NONE_V01)
    {
      /* Print this if user specified IPv6 or IPv4v6 */
      printf("\nIPv6 WWAN NetworkConfiguration:\n");
      printf("Public IP for WWAN: %s \n",
             inet_ntop(AF_INET6,&qcmap_nw_params.v6_conf.public_ip_v6,
             ipv6_addr_buf,sizeof(ipv6_addr_buf)));
      printf("Primary DNS IP address: %s \n",
             inet_ntop(AF_INET6,&qcmap_nw_params.v6_conf.primary_dns_v6,
             ipv6_addr_buf,sizeof(ipv6_addr_buf)));
      printf("Secondary DNS IP address: %s \n\n",
             inet_ntop(AF_INET6,&qcmap_nw_params.v6_conf.secondary_dns_v6,
             ipv6_addr_buf,sizeof(ipv6_addr_buf)));
    }
    else
      printf("\nError in IPv6 config - 0x%x\n\n", qmi_err_num);
  }
  else
    printf("\n GetNetworkConfiguration fails \n\n");

  printf("Nat type configured: \n");
  if(nat_type == QCMAP_MSGR_NAT_SYMMETRIC_NAT_V01)
  {
    printf("Symmetric NAT");
  }
  else if(nat_type == QCMAP_MSGR_NAT_PORT_RESTRICTED_CONE_NAT_V01)
  {
    printf("Port Restricted Cone NAT");
  }
  else if (nat_type == QCMAP_MSGR_NAT_FULL_CONE_NAT_V01)
  {
    printf("Full Cone NAT");
  }
  else if(nat_type == QCMAP_MSGR_NAT_ADDRESS_RESTRICTED_NAT_V01)
  {
    printf("Address Restricted Cone NAT");
  }
  else
  {
    printf("Default NAT configured");
  }
  printf("\nNAT Timeout Values Configured:\n");
  timeout_type = QCMAP_MSGR_NAT_TIMEOUT_GENERIC_V01;
  while(timeout_type <= QCMAP_MSGR_NAT_TIMEOUT_UDP_V01)
  {
    if(timeout_type == QCMAP_MSGR_NAT_TIMEOUT_GENERIC_V01)
    {
      printf("GENRIC NAT Timeout: %d\n", timeout_value[timeout_type]);
    }
    else if(timeout_type == QCMAP_MSGR_NAT_TIMEOUT_ICMP_V01)
    {
      printf("ICMP NAT Timeout: %d\n", timeout_value[timeout_type]);
    }
    else if(timeout_type == QCMAP_MSGR_NAT_TIMEOUT_TCP_ESTABLISHED_V01)
    {
      printf("TCP ESTABLISHED NAT Timeout: %d\n", timeout_value[timeout_type]);
    }
    else if(timeout_type == QCMAP_MSGR_NAT_TIMEOUT_UDP_V01)
    {
      printf("UDP NAT Timeout: %d\n", timeout_value[timeout_type]);
    }
    timeout_type++;
  }
  printf("\nWlan Configuration :");
  if(wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_V01)
  {
    printf(" WLAN Mode is AP.\n");
  }
  else if(wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_V01)
  {
    printf(" WLAN Mode is AP-AP.\n");
  }
  else if(wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_STA_V01)
  {
    printf(" WLAN Mode is AP-STA.\n");
  }
  else if(wlan_mode == QCMAP_MSGR_WLAN_MODE_ENUM_MIN_ENUM_VAL_V01)
  {
    printf(" WLAN is not configured.\n");
  }

  // Guest AP Access profile
  printf("\n Guest AP is setup to be in '%s' mode \n", (guest_ap_profile == QCMAP_MSGR_PROFILE_FULL_ACCESS_V01)?"Full Access" : "Internet Only Access");

  /* Print LAN Configuration. */
  printf("\n LAN Configuration.\n");
  addr.s_addr = htonl(lan_config.gw_ip);
  printf("\nGateway IP: %s\n", inet_ntoa(addr));
  addr.s_addr = htonl(lan_config.netmask);
  printf("\nNetmask : %s\n", inet_ntoa(addr));
  printf("\nDHCP Enabled: %d\n", lan_config.enable_dhcp);
  if ( lan_config.enable_dhcp == TRUE )
  {
    addr.s_addr = htonl(lan_config.dhcp_config.dhcp_start_ip);
    printf("\nDHCP Start IP: %s\n", inet_ntoa(addr));
    addr.s_addr = htonl(lan_config.dhcp_config.dhcp_end_ip);
    printf("\nDHCP End IP : %s\n", inet_ntoa(addr));
    printf("\nDHCP Lease Time (seconds) : %d\n", lan_config.dhcp_config.lease_time);
  }

  /* Print Staton Configuration. */
  printf("\nStation Configuration.\n");
  if ( station_config.conn_type == QCMAP_MSGR_STA_CONNECTION_DYNAMIC_V01)
    printf("\nConnection Type : DYNAMIC (DHCP)\n");
  else
  {
    printf("\nConnection Type : STATIC\n");
    addr.s_addr = htonl(station_config.static_ip_config.ip_addr);
    printf("\nIP Address: %s\n", inet_ntoa(addr));
    addr.s_addr = htonl(station_config.static_ip_config.gw_ip);
    printf("\nGateway IP : %s\n", inet_ntoa(addr));
    addr.s_addr = htonl(station_config.static_ip_config.netmask);
    printf("\nNetmask: %s\n", inet_ntoa(addr));
    addr.s_addr = htonl(station_config.static_ip_config.dns_addr);
    printf("\nDNS Address : %s\n", inet_ntoa(addr));
  }

   if (QcMapClient->GetQCMAPBootupCfg(&mobileap_enable, &wlan_enable, &qmi_err_num))
  {
     printf("\n Mobile AP will be %s on bootup \n",((mobileap_enable == QCMAP_MSGR_ENABLE_ON_BOOT_V01) ?"ENABLED":"DISABLED"));
     printf("\n WLAN will be %s on bootup \n",((wlan_enable == QCMAP_MSGR_ENABLE_ON_BOOT_V01)?"ENABLED":"DISABLED"));
  }
   else
  {
     printf("\n  Get QCMAP Bootup Cfg Fails , Error: 0x%x \n", qmi_err_num);
  }
  memset(dhcp_reserv_record,0,QCMAP_MSGR_MAX_DHCP_RESERVATION_ENTRIES_V01*sizeof(qcmap_msgr_dhcp_reservation_v01));
  if( QcMapClient->GetDHCPReservRecords(dhcp_reserv_record, &num_entries,&qmi_err_num) )
  {
       //display each DHCP reservation records
       if ( !num_entries)
       {
           printf("\nNo DHCP Reservation Records");
       }
       else
       {
           for ( i = 0;i < num_entries; i++)
           {
             printf("\nEntry  %d:",i);
             mac_addr_ntop(dhcp_reserv_record[i].client_mac_addr);
             if ( strncmp(mac_addr_str,MAC_NULL_STRING,QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01) != 0 )
             {
                 printf("\nMAC address of the client[%i]: %s",i,mac_addr_str);
             }
             tmpIP.s_addr =ntohl(dhcp_reserv_record[i].client_reserved_ip);
             printf("IP address of the client[%i]: %s",i,inet_ntoa(tmpIP));
             printf("Device Name of the client[%i]: %s",i,dhcp_reserv_record[i].client_device_name);
             printf("DHCP Reservation enabled for the client[%i]: %d",i,dhcp_reserv_record[i].enable_reservation);
           }
       }
   }
   else
   {
        printf("\nFailed to Dsiplay DHCP Reservation record. Error 0x%x.\n ", qmi_err_num);
   }
}


int main(int argc, char **argv)
{

  int opt = 0, p_error;
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];
  //char scan_string[100];
  in_addr addr;
  uint32 tmp_input=0;
  qcmap_msgr_snat_entry_config_v01 snat_entry;
  qcmap_msgr_dhcp_reservation_v01 dhcp_reserv_record;
  qmi_error_type_v01  qmi_err_num;
  int array_size = sizeof(options_list)/sizeof(options_list[0]);

  /* Register the sighandlers, so the app may be shutdown with a
     kill command.*/
  signal(SIGTERM, sighandler);

  QcMapClient = new QCMAP_Client( qcmap_msgr_qmi_qcmap_ind );

  if (QcMapClient->qmi_qcmap_msgr_handle == 0)
  {
    printf("\nCouldn't setup QcMapClient..exiting");
    sighandler(SIGTERM);
    exit(1);
  }


  while (TRUE)
  {

    /* Display menu of options. */
    printf("\nPlease select an option to test from the items listed below.\n\n");

    for (int i=0; i<(array_size/2); i++)
    {
      printf("%s%s\n",options_list[i].c_str(),options_list[i+(array_size/2)].c_str());
    }
    printf("Option > ");

    /* Initialize QMI Error Number. */
    qmi_err_num = QMI_ERR_NONE_V01;

    /* Read the option from the standard input. */
    if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
      continue;

    /* Convert the option to an integer, and switch on the option entered. */
    opt = atoi(scan_string);
    switch (opt)
    {
      /* Display the current configuration of the modem. */
      case 1:
        DisplayConfig();
        break;

      /* Delete a static NAT entry and save XML if successful. */
      case 2:
        {
          memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
          memset(&snat_entry, 0, sizeof(qcmap_msgr_snat_entry_config_v01));
          in_addr tmp1, tmp2;
          inet_aton("0.0.0.0",&tmp1);
          inet_aton("255.255.255.255",&tmp2);

          while(TRUE)
          {
            printf("   Please input port_fwding_protocol     : ");
            fgets(scan_string, sizeof(scan_string), stdin);
            tmp_input = atoi(scan_string);
            if ( check_proto(tmp_input) == 0 )
            break;
          }
          snat_entry.protocol = (uint8)tmp_input;
          while (TRUE)
          {
            printf("   Please input port_fwding_private_ip(xxx.xxx.xxx.xxx)   : ");
            if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
            {
              if ( !( inet_aton(scan_string, &addr) <= 0) &&
                    ( addr.s_addr != tmp1.s_addr ) &&
                    ( addr.s_addr != tmp2.s_addr ))
                break;
            }
            printf("   Invalid IPv4 address %s",scan_string);
          }
          snat_entry.private_ip_addr = ntohl(addr.s_addr);
          while (TRUE)
          {
            printf("   Please input port_fwding_private_port : ");
            fgets(scan_string, sizeof(scan_string), stdin);
            tmp_input = atoi(scan_string);
            /*No port validation check for ICMP*/
            if(snat_entry.protocol == 1)
              break;
            if(check_port (tmp_input) == 0 )
              break;
          }
          snat_entry.private_port = (uint16)tmp_input;

          while (TRUE)
          {
            printf("   Please input port_fwding_global_port  : ");
            fgets(scan_string, sizeof(scan_string), stdin);
            tmp_input = atoi(scan_string);

            /*No port validation check for ICMP*/
            if(snat_entry.protocol == 1)
              break;
            if(check_port (tmp_input) == 0 )
              break;
          }
          snat_entry.global_port = (uint16)tmp_input;
          if (QcMapClient->DeleteStaticNatEntry(&snat_entry, &qmi_err_num))
          {
            printf("\nSNAT Entry deleted successfully");
          }
          else
          {
            if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01)
              printf("\nBackhaul down, SNAT Entry deleted from xml file.");
            else
              printf("\nSNAT Entry delete failed, Error: 0x%x", qmi_err_num);
          }
        }
        break;

      /* Add a static NAT entry and save XML if successful. */
      case 3:
        {
          memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
          in_addr tmp1, tmp2;

          inet_aton("0.0.0.0",&tmp1);
          inet_aton("255.255.255.255",&tmp2);

          memset(&snat_entry, 0, sizeof(qcmap_msgr_snat_entry_config_v01));

          while(TRUE)
          {
            printf("   Please input port_fwding_protocol     : ");
            fgets(scan_string, sizeof(scan_string), stdin);
            tmp_input = atoi(scan_string);
            if ( check_proto(tmp_input) == 0 )
            break;
          }
          snat_entry.protocol = (uint8)tmp_input;
          while (TRUE)
          {
            printf("   Please input port_fwding_private_ip(xxx.xxx.xxx.xxx)   : ");
            if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
            {
               if ( !( inet_aton(scan_string, &addr) <= 0) &&
                    ( addr.s_addr != tmp1.s_addr ) &&
                    ( addr.s_addr != tmp2.s_addr ))
                break;
            }
            printf("   Invalid IPv4 address %s",scan_string);
          }
          snat_entry.private_ip_addr = ntohl(addr.s_addr);
          while (TRUE)
          {
            printf("   Please input port_fwding_private_port : ");
            fgets(scan_string, sizeof(scan_string), stdin);
            tmp_input = atoi(scan_string);

            /*No port validation check for ICMP*/
            if(snat_entry.protocol == 1)
              break;
            if(check_port (tmp_input) == 0 )
              break;
          }
          snat_entry.private_port = (uint16)tmp_input;

          while (TRUE)
          {
            printf("   Please input port_fwding_global_port  : ");
            fgets(scan_string, sizeof(scan_string), stdin);
            tmp_input = atoi(scan_string);

            /*No port validation check for ICMP*/
            if(snat_entry.protocol == 1)
              break;
            if(check_port (tmp_input) == 0 )
              break;
          }
          snat_entry.global_port = (uint16)tmp_input;

        if (QcMapClient->AddStaticNatEntry(&snat_entry, &qmi_err_num))
        {
          printf("\nSNAT Entry added successfully");
        }
        else
        {
          if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01)
            printf("\nBackhaul down, SNAT Entry added to xml file");
          else
            printf("\nSNAT Entry add failed, Error: 0x%x", qmi_err_num);
        }
      }
       break;
      case 4:
      {
        in_addr tmpIP;
        int i=0, num_entries=0;
        qcmap_msgr_snat_entry_config_v01 snat_config[QCMAP_MSGR_MAX_SNAT_ENTRIES_V01];

        memset(snat_config, 0, QCMAP_MSGR_MAX_SNAT_ENTRIES_V01*sizeof(qcmap_msgr_snat_entry_config_v01));

        if (QcMapClient->GetStaticNatConfig(snat_config, &num_entries, &qmi_err_num))
        {
            for (i=0; i<num_entries; i++)
            {
              printf("\n\nEntry %d:",i);
              tmpIP.s_addr = ntohl(snat_config[i].private_ip_addr);
              printf("\nprivate ip: %s", inet_ntoa(tmpIP));
              printf("\nprivate port: %d", snat_config[i].private_port);
              printf("\nglobal port: %d", snat_config[i].global_port);
              printf("\nprotocol: %d", snat_config[i].protocol);
            }
        }
	else
        {
          printf("\nNo SNAT Entries Configured Error: 0x%x", qmi_err_num);
        }
      }
        break;

      case 5:
       {
          boolean enable;
          printf("   Please input Roaming Mode Flag (1-Enable/0-Disable) : ");
          fgets(scan_string, sizeof(scan_string), stdin);
          enable = (atoi(scan_string)) ? true : false;
          if (QcMapClient->SetRoaming(enable, &qmi_err_num))
          {
            printf("\nRoaming set config succeeds.");
          }
          else
            printf("\nRoaming set config fails, Error: 0x%x", qmi_err_num);
        }
          break;
      case 6:
       {
          boolean enable;
          int p_error=0;
          if (QcMapClient->GetRoaming(&enable, &qmi_err_num))
          {
            printf("\nRoaming Mode: %s.",(enable)?"Enabled":"Disabled");
          }
          else
            printf("\nRoaming  get fails. Error: 0x%x", qmi_err_num);
       }
          break;

      /* Delete the current DMZ IP and save XML if successful. */
      case 7:
      {
        if (QcMapClient->DeleteDMZ(&qmi_err_num))
        {
          printf("\nDMZ deleted successfully");
        }
        else
        {
          if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01)
            printf("\nBackhaul down. DMZ deleted from xml file.");
          else
            printf("\nDMZ delete fails. Error: 0x%x", qmi_err_num);
        }
      }
        break;

      /* Add a DMZ IP and save XML if successful. */
      case 8:
      {
        uint32 dmz_ip=0;

        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
        while (TRUE)
        {
          printf("   Please input DMZ IP to add(xxx.xxx.xxx.xxx) : ");
          if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
          {
            if ( !(inet_aton(scan_string, &addr) <=0 ))
              break;
          }
          printf("   Invalid IPv4 address %d\n", scan_string);
        }
        dmz_ip = ntohl(addr.s_addr);

        if (QcMapClient->AddDMZ(dmz_ip, &qmi_err_num))
        {
          printf("\nDMZ IP added successfully");
        }
        else if ( qmi_err_num == QMI_ERR_NO_EFFECT_V01 )
        {
          printf(" DMZ is already configured. Delete the current configuration, if DMZ reconfiguration is needed \n");
        }
        else if ( qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01 ) {
          printf("\nBackhaul down. DMZ Entry deleted from xml file.");
        }
        else
          printf("\nDMZ add fails. Error: 0x%x", qmi_err_num);
      }
        break;

      /* Get DMZ IP */
      case 9:
      {
        uint32_t dmz_ip=0;
        in_addr tmpIP;
        if (QcMapClient->GetDMZ(&dmz_ip, &qmi_err_num))
        {
          if ( dmz_ip == 0 )
            printf("\nNo DMZ Configured!");
          else
          {
            tmpIP.s_addr = ntohl(dmz_ip);
            printf("\ndmz ip %s",inet_ntoa(tmpIP));
          }
        }
        else
          printf("\nDMZ get fails. Error: 0x%x", qmi_err_num);
      }
        break;

      /* Enable/disable the IPSEC VPN pass through and save XML if successful. */
      case 10:
      {
        boolean enable;
        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);

        printf("   Please input IPSEC VPN Pass Through(1-Enable/0-Disable) : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        enable = (atoi(scan_string)) ? true : false;

        if (QcMapClient->SetIPSECVpnPassthrough(enable, &qmi_err_num))
        {
          printf("\nIPSEC VPN passthrough set successfully");
        }
        else{
          if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01)
            printf("\nBackhaul down. IPSEC VPN passthrough enabled in xml file.");
          else
            printf("\nIPSEC VPN passthrough set fails. Error: 0x%x", qmi_err_num);
        }
      }
        break;

      case 11:
      {
        boolean flag;
        int p_error=0;

        if (QcMapClient->GetIPSECVpnPassthrough(&flag, &qmi_err_num))
        {
          printf("\nIPSEC Passthrough Enable Flag : %d", flag);
        }
        else
          printf("\nIPSEC VPN passthrough get fails. Error: 0x%x", qmi_err_num);
      }
        break;

      /* Enable/disable the PPTP VPN pass through and save XML if successful. */
      case 12:
      {
        boolean enable;
        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);

        printf("   Please input PPTP VPN Pass Through(1-Enable/0-Disable) : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        enable = (atoi(scan_string)) ? true : false;

        if (QcMapClient->SetPPTPVpnPassthrough(enable, &qmi_err_num))
        {
          printf("\nPPTP Passthrough set successfully");
        }
        else
        {
          if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01)
            printf("\nBackhaul down. PPTP VPN passthrough enabled in xml file.");
          else
            printf("\nPPTP VPN passthrough set fails. Error: 0x%x", qmi_err_num);
        }
      }
        break;

      case 13:
      {
        boolean flag;
        int p_error=0;

        if (QcMapClient->GetPPTPVpnPassthrough(&flag, &qmi_err_num))
        {
          printf("\nPPTP Passthrough Enable Flag : %d", flag);
        }
        else
          printf("\nPPTP VPN passthrough get fails. Error: 0x%x", qmi_err_num);
      }
        break;

      /* Enable/disable the L2TP VPN pass through and save XML if successful. */
      case 14:
      {
         boolean enable;
         memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);

         printf("   Please input L2TP VPN Pass Through(1-Enable/0-Disable) : ");
         fgets(scan_string, sizeof(scan_string), stdin);
         enable = (atoi(scan_string))? true : false;

         if (QcMapClient->SetL2TPVpnPassthrough(enable, &qmi_err_num))
         {
           printf("\nL2TP VPN Passthrough set successfully");
         }
         else
         {
           if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01)
             printf("\nBackhaul down. L2TP VPN passthrough enabled in xml file.");
           else
             printf("\nL2TP VPN passthrough set fails. Error: 0x%x", qmi_err_num);
         }
      }
         break;
      case 15:
      {
        boolean flag;
        int p_error=0;

        if (QcMapClient->GetL2TPVpnPassthrough(&flag, &qmi_err_num))
        {
          printf("\nL2TP Passthrough Enable Flag : %d", flag);
        }
        else
          printf("\nL2TP VPN passthrough get fails. Error: 0x%x", qmi_err_num);
      }
        break;

      case 16:
      {
        boolean enable;
        printf("   Please input Autoconnect Mode Flag (1-Enable/0-Disable) : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        enable = (atoi(scan_string)) ? true : false;
        if (QcMapClient->SetAutoconnect(enable, &qmi_err_num))
        {
          printf("\nAuto Connect config set succeeds.");
        }
        else
          printf("\nAuto Connect config set fails, Error: 0x%x", qmi_err_num);
      }
        break;
      case 17:
      {
        boolean enable;
        int p_error=0;
        if (QcMapClient->GetAutoconnect(&enable, &qmi_err_num))
        {
          printf("\nAuto Connect Mode: %s.",(enable)?"Enabled":"Disabled");
        }
        else
          printf("\nAuto Connect config get fails, Error: 0x%x", qmi_err_num);
      }
        break;
      case 18:
      {
        qcmap_msgr_wwan_status_enum_v01 v4_status, v6_status;
       if(QcMapClient->GetWWANStatus(&v4_status, &v6_status, &qmi_err_num))
       {
         if(v4_status == QCMAP_MSGR_WWAN_STATUS_CONNECTING_V01)
         {
           printf(" IPV4 WWAN is Connecting \n");
         }
         else if(v4_status == QCMAP_MSGR_WWAN_STATUS_CONNECTED_V01)
         {
           printf(" IPV4 WWAN is connected \n");
         }
         else if(v4_status == QCMAP_MSGR_WWAN_STATUS_DISCONNECTING_V01)
         {
           printf(" IPV4 WWAN is Disconnecting \n");
         }
         else if(v4_status == QCMAP_MSGR_WWAN_STATUS_DISCONNECTED_V01)
         {
           printf(" IPV4 WWAN is Disconnected \n");
         }

         if(v6_status == QCMAP_MSGR_WWAN_STATUS_IPV6_CONNECTING_V01)
         {
           printf(" IPV6 WWAN is Connecting \n");
         }
         else if(v6_status == QCMAP_MSGR_WWAN_STATUS_IPV6_CONNECTED_V01)
         {
           printf(" IPV6 WWAN is connected \n");
         }
         else if(v6_status == QCMAP_MSGR_WWAN_STATUS_IPV6_DISCONNECTING_V01)
         {
           printf(" IPV6 WWAN is Disconnecting \n");
         }
         else if(v6_status == QCMAP_MSGR_WWAN_STATUS_IPV6_DISCONNECTED_V01)
         {
           printf(" IPV6 WWAN is Disconnected \n");
         }

       }
       else
         printf("\nWWAN status get fails, Error: 0x%x", qmi_err_num);
      }
        break;

      case 19:
       {
         qcmap_msgr_firewall_conf_t     extd_firewall_add;
         /*Default rule is to accept the packets */
         printf("Please Enter Firewall entry:\n");
         if ( read_firewall_conf( &extd_firewall_add ) )
         {
           if ( QcMapClient->AddFireWallEntry(&extd_firewall_add,&qmi_err_num )== true)
           {
             printf("\n Add Firewall Entry success.");
           }
           else if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01) {
             printf("\nBackhaul down. Saved firwall entry in configuration file.");
           }
           else
             printf("\nAdd Firewall Entry failed, Error: 0x%x", qmi_err_num);
         }
       }
       break;
      case 20:
      {
          printf("   Please input M-DNS State (1-Enable/0-Disable) : ");
          fgets(scan_string, sizeof(scan_string), stdin);
          if (atoi(scan_string))
          {
             if (QcMapClient->EnableMDNS(&qmi_err_num)) {
               printf("\n M-DNS Enable succeeds.");
             } else {
                if (qmi_err_num == QMI_ERR_NO_EFFECT_V01) {
                   printf("\n M-DNS Already Enabled \n");
                } else {
                   printf("\n M-DNS Enable fails, Error: 0x%x \n", qmi_err_num);
                }
             }
          } else {
            if (QcMapClient->DisableMDNS(&qmi_err_num)) {
              printf("\n M-DNS Disable in progress.");
          } else {
                if (qmi_err_num == QMI_ERR_NO_EFFECT_V01) {
                   printf("\n M-DNS Already Disabled \n");
                } else {
                   printf("\n M-DNS Disable fails, Error: 0x%x \n", qmi_err_num);
                }
            }
          }
      }
        break;
      case 21:
		 printf("   Please input UPnP State (1-Enable/0-Disable) : ");
		 fgets(scan_string, sizeof(scan_string), stdin);
		 if (atoi(scan_string))
		 {
		   if (QcMapClient->EnableUPNP(&qmi_err_num))
			 printf("\nUPNP Enable succeeds.");
		   else
			 printf("\nUPNP Enable fails, Error: 0x%x", qmi_err_num);
		 }
		 else
		 {
		   if (QcMapClient->DisableUPNP(&qmi_err_num))
			 printf("\nUPNP Disabled.");
		   else
			 printf("\nUPNP Disable request fails, Error: 0x%x", qmi_err_num);
		   }
        break;
      case 22:
        printf("   Please input DLNA State (1-Enable/0-Disable) : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        if (atoi(scan_string))
        {
          if (QcMapClient->EnableDLNA(&qmi_err_num))
            printf("\nDLNA Enable succeeds.");
          else
            printf("\nDLNA Enable fails, Error: 0x%x", qmi_err_num);
        }
        else
        {
          if (QcMapClient->DisableDLNA(&qmi_err_num))
            printf("\nDLNA Disabled.");
          else
            printf("\nDLNA Disable request fails, Error: 0x%x", qmi_err_num);
        }
        break;
      case 23:
      {
        qcmap_msgr_firewall_conf_t extd_firewall_handle_list;
        int   handle_list_len;
        int   result;

        memset(&extd_firewall_handle_list, 0, sizeof(qcmap_msgr_firewall_conf_t));
        handle_list_len=0;
        while(1)
        {
          printf("\n Please input IP family type [4-IPv4 6-IPv6]:");
          fgets(scan_string, sizeof(scan_string), stdin);
          result = atoi(scan_string);
          if(result == IP_V4 || result == IP_V6 )
          break;
        }

        if(result == IP_V4)
        {
            extd_firewall_handle_list.extd_firewall_handle_list.ip_family = IP_V4;
        }
         else if(result == IP_V6)
        {
         extd_firewall_handle_list.extd_firewall_handle_list.ip_family = IP_V6;
        }
        if(QcMapClient->GetFireWallHandlesList(&extd_firewall_handle_list.extd_firewall_handle_list, &qmi_err_num))
        {
          handle_list_len = extd_firewall_handle_list.extd_firewall_handle_list.num_of_entries;
        }
	else
	{
          printf("\nFirewall Handle list get failed, Error: 0x%x", qmi_err_num);
          break;
	}
        if( DisplayFirewall( handle_list_len, extd_firewall_handle_list ) )
        {
          printf("\n Firewall Display success \n");
        }
      }
        break;
      case 24:
      {
        int handle=0;
        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
        printf(" Please Enter the Firewall Handle : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        handle= atoi(scan_string);
        if(handle <0)
          printf("\n Entered Handle is invalid \n");

        if (QcMapClient->DeleteFireWallEntry(handle, &qmi_err_num)< 0)
        {
          if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01)
          {
            printf("\nBackhaul down. firewall entry deleted from configuration file.");
          }
          else
            printf("\nDelete firewall Fails, Error: 0x%x", qmi_err_num);
        }
        else
          printf("\nDelete firewall Successfully");
      }
      break;
      /* Get WWAN Statistics */
      case 25:
      {
        qcmap_msgr_ip_family_enum_v01 ip_family;
        qcmap_msgr_wwan_statistics_type_v01 wwan_stats;

        printf("Please input IP Family IPV4-4 IPV6-6 : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        ip_family = (qcmap_msgr_ip_family_enum_v01)atoi(scan_string);

        memset((void *)&wwan_stats, 0, sizeof(qcmap_msgr_wwan_statistics_type_v01));
        if (QcMapClient->GetWWANStatistics(ip_family, &wwan_stats, &qmi_err_num))
        {
          printf("\nWWAN Stats Fetched.\n");
          printf("\nbytes_rx: %llu",wwan_stats.bytes_rx);
          printf("\nbytes_tx: %llu",wwan_stats.bytes_tx);
          printf("\npkts_rx: %lu",wwan_stats.pkts_rx);
          printf("\npkts_tx: %lu",wwan_stats.pkts_tx);
          printf("\npkts_dropped_rx: %lu",wwan_stats.pkts_dropped_rx);
          printf("\npkts_dropped_tx: %lu",wwan_stats.pkts_dropped_tx);
        }
        else
          printf("\nGet WWAN Stats Fails, Error: 0x%x", qmi_err_num);
      }
        break;

      /* RESET WWAN Statistics */
      case 26:
      {
        qcmap_msgr_ip_family_enum_v01 ip_family;

        printf("Please input IP Family IPV4-4 IPV6-6 : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        ip_family = (qcmap_msgr_ip_family_enum_v01)atoi(scan_string);

        if (QcMapClient->ResetWWANStatistics(ip_family, &qmi_err_num))
        {
          printf("WWAN Stats Reset for IP Family: %d \n", ip_family);
        }
        else
          printf("\nReset WWAN Stats failed, Error: 0x%x", qmi_err_num);
      }
        break;
      case 27:
      {
        qcmap_msgr_ip_family_enum_v01 ip_family;
        qcmap_nw_params_t qcmap_nw_params;
        char ip6_addr_buf[INET6_ADDRSTRLEN];

        memset(&qcmap_nw_params,0,sizeof(qcmap_nw_params_t));

        printf("\nPlease input IP Family 4:IPv4 6:IPv6 :- ");
        fgets(scan_string, sizeof(scan_string), stdin);
        ip_family = (qcmap_msgr_ip_family_enum_v01)atoi(scan_string);

        if (QcMapClient->GetNetworkConfiguration(
                                     ip_family,
                                     &qcmap_nw_params,
                                     &qmi_err_num ))
        {
          if (ip_family == QCMAP_MSGR_IP_FAMILY_V4_V01)
          {
            if (qmi_err_num == QMI_ERR_NONE_V01)
            {
              printf("\n IPv4 configuration \n");
              qcmap_nw_params.v4_conf.public_ip.s_addr =
                          htonl(qcmap_nw_params.v4_conf.public_ip.s_addr);
              qcmap_nw_params.v4_conf.primary_dns.s_addr =
                          htonl(qcmap_nw_params.v4_conf.primary_dns.s_addr);
              qcmap_nw_params.v4_conf.secondary_dns.s_addr =
                          htonl(qcmap_nw_params.v4_conf.secondary_dns.s_addr);

              printf("\nPublic IP for WWAN: %s \n",
                     inet_ntoa(qcmap_nw_params.v4_conf.public_ip));
              printf("Primary DNS IP address: %s \n",
                     inet_ntoa(qcmap_nw_params.v4_conf.primary_dns));
              printf("Secondary DNS IP address: %s \n",
                     inet_ntoa(qcmap_nw_params.v4_conf.secondary_dns));
            }
            else
              printf("\nError in IPv4 config - 0x%x\n\n", qmi_err_num);
          }

          if (ip_family == QCMAP_MSGR_IP_FAMILY_V6_V01)
          {
            if (qmi_err_num == QMI_ERR_NONE_V01)
            {
              printf("\n IPv6 configuration \n");
              printf("\nPublic IP for WWAN: %s \n",
                      inet_ntop(AF_INET6,
                                &qcmap_nw_params.v6_conf.public_ip_v6,
                                ip6_addr_buf,sizeof(ip6_addr_buf)));
              printf("Primary DNS IP address: %s \n",
                      inet_ntop(AF_INET6,
                                &qcmap_nw_params.v6_conf.primary_dns_v6,
                                ip6_addr_buf,sizeof(ip6_addr_buf)));
              printf("Secondary DNS IP address: %s \n",
                      inet_ntop(AF_INET6,
                                &qcmap_nw_params.v6_conf.secondary_dns_v6,
                                ip6_addr_buf,sizeof(ip6_addr_buf)));
            }
            else
              printf("\nError in IPv6 config - 0x%x\n\n", qmi_err_num);
          }
        }
        else
          printf("\nGet WWAN Network Config failed,0x%x", qmi_err_num);
        break;
      }
      case 28:
      {
        qcmap_msgr_nat_enum_v01 nat_type;
        if (QcMapClient->GetNatType(&nat_type, &qmi_err_num))
        {
          switch (nat_type)
          {
            case QCMAP_MSGR_NAT_SYMMETRIC_NAT_V01:
              printf("\n Symmetric NAT \n");
              break;
            case QCMAP_MSGR_NAT_PORT_RESTRICTED_CONE_NAT_V01:
              printf("Port Restricted Cone NAT\n");
              break;
            case QCMAP_MSGR_NAT_FULL_CONE_NAT_V01:
              printf("\nFull Cone NAT\n");
              break;
            case QCMAP_MSGR_NAT_ADDRESS_RESTRICTED_NAT_V01:
              printf("Address Restricted Cone NAT\n");
              break;
            default:
              printf("Invalid NAT Type Returned: 0x%d", nat_type);
              break;
          }
        }
        else
          printf("\nGet NAT type failed, Error: 0x%x", qmi_err_num);

        break;
      }
      case 29:
      {
        int nat_type;
        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
        printf("Select the Type of NAT : \n"
                "0:SYMMETRIC NAT\n1: PORT RESTRICTED CONE NAT\n2: FULL CONE NAT\n"
                "3: ADDRESS RESTRICTED CONE NAT\t");
        fgets(scan_string, sizeof(scan_string), stdin);
        nat_type = atoi(scan_string);
        if ( nat_type < QCMAP_MSGR_NAT_SYMMETRIC_NAT_V01 ||
              nat_type > QCMAP_MSGR_NAT_ADDRESS_RESTRICTED_NAT_V01 )
        {
          printf("\nInvalid NAT Type : %d", nat_type);
          break;
        }
        if (QcMapClient->SetNatType((qcmap_msgr_nat_enum_v01)nat_type, &qmi_err_num))
        {
          printf("\nNAT Type set successfully");
        }
        else
          printf("\nNAT Type set fail, Error: 0x%x", qmi_err_num);
        break;
      }
      case 30:
      {
        printf("   Please input MobileAP State(1-Enable/0-Disable) : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        if (atoi(scan_string))
        {
          if (QcMapClient->EnableMobileAP(&qmi_err_num))
            printf("\nMobileAP Enable succeeds.");
          else
            printf("\nMobileAP Enable fails, Error: 0x%x", qmi_err_num);
        }
        else
        {
          if (QcMapClient->DisableMobileAP(&qmi_err_num))
            printf("\nMobileAP Disable in progress.");
          else
            printf("\nMobileAP Disable request fails, Error: 0x%x", qmi_err_num);
        }
        break;
      }
      case 31:

      {
        printf("   Please input WLAN State(1-Enable/0-Disable) : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        if (atoi(scan_string))
        {
          if(QcMapClient->EnableWLAN(&qmi_err_num))
          {
            printf("\nEnabled WLAN");
          }
          else
            printf("\nEnable WLAN fails, Error: 0x%x", qmi_err_num);
        }
        else
        {
          if(QcMapClient->DisableWLAN(&qmi_err_num))
          {
            printf("\nDisabled WLAN");
          }
          else
            printf("\nDisable WLAN fails, Error: 0x%x", qmi_err_num);
        }
        break;
      }
      case 32:
      {
        qcmap_msgr_wwan_call_type_v01 call_type;
        printf("   Please input Backhaul State(1-Connect/0-Disconnect) : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        if (atoi(scan_string))
        {
          memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
          printf("\n   Please input Call Type (1-IPV4; 2-IPV6) : ");
          fgets(scan_string, sizeof(scan_string), stdin);
		  call_type = (qcmap_msgr_wwan_call_type_v01)atoi(scan_string);

          if ( call_type < QCMAP_MSGR_WWAN_CALL_TYPE_V4_V01 ||
               call_type > QCMAP_MSGR_WWAN_CALL_TYPE_V6_V01 )
          {
            printf("\nInvalid Call Type : %d", call_type);
            break;
          }
          if (QcMapClient->ConnectBackHaul(call_type, &qmi_err_num))
            printf("ConnectBackHaul succeeds.");
          else
            printf("\nConnectBackHaul fails, Error: 0x%x", qmi_err_num);
        }
        else
        {
          memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
          printf("\n   Please input Call Type (1-IPV4; 2-IPV6) : ");
          fgets(scan_string, sizeof(scan_string), stdin);
		  call_type = (qcmap_msgr_wwan_call_type_v01)atoi(scan_string);
          if ( call_type < QCMAP_MSGR_WWAN_CALL_TYPE_V4_V01 ||
               call_type > QCMAP_MSGR_WWAN_CALL_TYPE_V6_V01 )
          {
            printf("\nInvalid Call Type : %d", call_type);
            break;
          }
          if (QcMapClient->DisconnectBackHaul(call_type, &qmi_err_num))
            printf("\nDisconnect BackHaul succeeds.");
          else
            printf("\nDisconnect BackHaul fails, Error: 0x%x", qmi_err_num);
        }
        break;
      }
      case 33:
      {
        qcmap_msgr_mobile_ap_status_enum_v01 status;
        if(QcMapClient->GetMobileAPStatus(&status, &qmi_err_num))
        {
         if(status ==QCMAP_MSGR_MOBILE_AP_STATUS_CONNECTED_V01)
         {
           printf("\nMobile AP is Connected");
         }
          else if(status == QCMAP_MSGR_MOBILE_AP_STATUS_DISCONNECTED_V01)
           printf("\nMobile AP is Disconnected");
        }
      }
        break;
      /* Set NAT Timeout. */
      case 34:
      {
         qcmap_msgr_nat_timeout_enum_v01 timeout_type;
         uint32 timeout_value = 0;
         int p_error=0;
         memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
         printf("Select the Type of Timeout : \n"
                "1: GENRIC TIMEOUT\t2: ICMP TIMEOUT\n"
                "3: TCP TIMEOUT ESTABLISHED\t4: UDP TIMEOUT\t:::");
         fgets(scan_string, sizeof(scan_string), stdin);
         timeout_type = (qcmap_msgr_nat_timeout_enum_v01)atoi(scan_string);
         if ( timeout_type < QCMAP_MSGR_NAT_TIMEOUT_GENERIC_V01 ||
              timeout_type > QCMAP_MSGR_NAT_TIMEOUT_UDP_V01 )
         {
           printf("\nInvalid NAT Timeout Type : %d", timeout_type);
           break;
         }
         printf("\nEnter the Timeout Value (should be >= 30):::");
         fgets(scan_string, sizeof(scan_string), stdin);
         timeout_value = atoi(scan_string);
         if (QcMapClient->SetNatTimeout(timeout_type, timeout_value, &qmi_err_num ))
         {
           printf("\nNAT Timeout Set Successfully\n");
         }
         else
           printf("\nNAT timeout set fails, Error: 0x%x", qmi_err_num);
         break;
       }
       case 35:
       {
         qcmap_msgr_nat_timeout_enum_v01 timeout_type;
         uint32 timeout_value = 0;
         int p_error=0;
         memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
         printf("Select the Type of Timeout : \n"
                "1: GENRIC TIMEOUT\t2: ICMP TIMEOUT\n"
                "3: TCP TIMEOUT ESTABLISHED\t4: UDP TIMEOUT\t:::");
         fgets(scan_string, sizeof(scan_string), stdin);
         timeout_type = (qcmap_msgr_nat_timeout_enum_v01)atoi(scan_string);
         if ( timeout_type < QCMAP_MSGR_NAT_TIMEOUT_GENERIC_V01 ||
              timeout_type > QCMAP_MSGR_NAT_TIMEOUT_UDP_V01 )
         {
           printf("\n\nInvalid NAT Timeout Type : %d\n", timeout_type);
           break;
         }
         if (QcMapClient->GetNatTimeout(timeout_type, &timeout_value, &qmi_err_num))
         {
           printf("\n\nNAT Timeout for Type %d : %d\n", timeout_type, timeout_value);
         }
         else
           printf("\nNAT timeout get fails, Error: 0x%x", qmi_err_num);
         break;
       }
      case 36:
      {
        qcmap_msgr_wlan_mode_enum_v01 wlan_mode;
        qcmap_msgr_access_profile_v01 guest_ap_access_profile = QCMAP_MSGR_ACCESS_PROFILE_MIN_ENUM_VAL_V01;
        int p_error=0;
        qcmap_msgr_station_mode_config_v01 station_config;
        in_addr addr;
        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
        memset(&station_config,0,sizeof(qcmap_msgr_station_mode_config_v01));

        printf("Select the Type of WLAN Mode : \n"
                    "1: AP\t2: AP-AP\n"
                    "3: AP-STA\t:::");
       while (TRUE ) {
            fgets(scan_string, sizeof(scan_string), stdin);
            wlan_mode = (qcmap_msgr_wlan_mode_enum_v01)atoi(scan_string);
            if ( wlan_mode < QCMAP_MSGR_WLAN_MODE_AP_V01 ||
                 wlan_mode > QCMAP_MSGR_WLAN_MODE_AP_STA_V01 ){
            printf("\nPlease enter a valid WLAN Mode\t:::");
            continue;
           }
           break;
       }

        printf(" Do you want to change Guest AP Access Profile 1-Yes/0-No\n");
        fgets(scan_string, sizeof(scan_string), stdin);
        if (atoi(scan_string)) {
           printf(" Select guest AP access profile 1-Full Access / 0- Internet Only \n");
           fgets(scan_string, sizeof(scan_string), stdin);
           if (atoi(scan_string) == 1) {
              guest_ap_access_profile = QCMAP_MSGR_PROFILE_FULL_ACCESS_V01;
           } else {
              guest_ap_access_profile = QCMAP_MSGR_PROFILE_INTERNET_ONLY_V01;
           }
        }

        printf("   Do you want to set Station Configuration(1-Yes/0-No) : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        if ( atoi(scan_string) == 1)
        {
          printf("   Please input Connection Type, 1 for DYNAMIC/2 for STATIC:");
          fgets(scan_string, sizeof(scan_string), stdin);
          if (atoi(scan_string) == 2)
          {
            station_config.conn_type = QCMAP_MSGR_STA_CONNECTION_STATIC_V01;
            while (TRUE)
            {
              printf("   Please input a valid Static IP address:");
              if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
              {
                memset(&addr,0,sizeof(in_addr));
                if (inet_aton(scan_string, &addr))
                {
                  station_config.static_ip_config.ip_addr = ntohl(addr.s_addr);
                  break;
                }
              }
              printf("      Invalid IPv4 address %s", scan_string);
            }
            while (TRUE)
            {
              printf("   Please input a valid Gateway address:");
              if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
              {
                memset(&addr,0,sizeof(in_addr));
                if (inet_aton(scan_string, &addr))
                {
                  station_config.static_ip_config.gw_ip = ntohl(addr.s_addr);
                  break;
                }
              }
              printf("      Invalid IPv4 address %s", scan_string);
            }
            while (TRUE)
            {
              printf("   Please input a valid Netmask:");
              if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
              {
                memset(&addr,0,sizeof(in_addr));
                if (inet_aton(scan_string, &addr))
                {
                  station_config.static_ip_config.netmask = ntohl(addr.s_addr);
                  break;
                }
              }
              printf("      Invalid IPv4 address %s", scan_string);
            }
            while (TRUE)
            {
              printf("   Please input a valid DNS Address:");
              if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
              {
                memset(&addr,0,sizeof(in_addr));
                if (inet_aton(scan_string, &addr))
                {
                  station_config.static_ip_config.dns_addr = ntohl(addr.s_addr);
                  break;
                }
              }
              printf("      Invalid IPv4 address %s", scan_string);
            }
          }
          else
          {
            station_config.conn_type = QCMAP_MSGR_STA_CONNECTION_DYNAMIC_V01;
          }
        }

        if (QcMapClient->SetWLANConfig(wlan_mode, guest_ap_access_profile, station_config, &qmi_err_num))
        {
          if (qmi_err_num ==  QMI_ERR_NONE_V01) {
            printf("\n LAN Config Set Successfully\n");
          } else {
              if (qmi_err_num == QMI_ERR_INVALID_ARG_V01) {
                printf("SSID1 (AP Mode) DHCP Address Range provided is invalid, Minimum range is 7 \n");
                printf("Setting AP DHCP address to default values which are derived from AP Gateway Addr \n");
              } else {
                printf("\nWLAN Config set fails, Error: 0x%x", qmi_err_num);
              }
          }
        }
        else
        {
          printf("\nWLAN Config set fails, Error: 0x%x", qmi_err_num);
        }
        break;
      }
      case 37:
      {
        qcmap_msgr_wlan_mode_enum_v01 wlan_mode;
        qcmap_msgr_access_profile_v01 guest_ap_access_profile;
        qcmap_msgr_station_mode_config_v01 station_config;
        in_addr addr;
        int p_error=0;
        memset(&station_config,0,sizeof(qcmap_msgr_station_mode_config_v01));
        memset(&addr, 0 ,sizeof(in_addr));
        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
        if (QcMapClient->GetWLANConfig(&wlan_mode, &guest_ap_access_profile, &station_config,&qmi_err_num))
        {
          switch (wlan_mode)
          {
            case QCMAP_MSGR_WLAN_MODE_AP_V01:
              printf("\nConfigured LAN Mode is AP.\n");
              break;
            case QCMAP_MSGR_WLAN_MODE_AP_AP_V01:
              printf("\nConfigured LAN Mode is AP-AP.\n");
              break;
            case QCMAP_MSGR_WLAN_MODE_AP_STA_V01:
              printf("\nConfigured LAN Mode is AP-STA.\n");
              break;
            case QCMAP_MSGR_WLAN_MODE_ENUM_MIN_ENUM_VAL_V01:
              printf("\n LAN is not configured.\n");
              break;
            default:
              printf("\nUnsupported LAN Mode:- %d.\n", wlan_mode);
              break;
          }

          // Guest AP Access profile
          printf("\n Guest AP is setup to be in '%s' mode \n", (guest_ap_access_profile == QCMAP_MSGR_PROFILE_FULL_ACCESS_V01)?"Full Access" : "Internet Only Access");

          /* Print Staton Configuration. */
          printf("\nStation Configuration.\n");
          if ( station_config.conn_type == QCMAP_MSGR_STA_CONNECTION_DYNAMIC_V01)
            printf("\nConnection Type : DYNAMIC (DHCP)\n");
          else
          {
            printf("\nConnection Type : STATIC\n");
            addr.s_addr = htonl(station_config.static_ip_config.ip_addr);
            printf("\nIP Address: %s\n", inet_ntoa(addr));
            addr.s_addr = htonl(station_config.static_ip_config.gw_ip);
            printf("\nGateway IP : %s\n", inet_ntoa(addr));
            addr.s_addr = htonl(station_config.static_ip_config.netmask);
            printf("\nNetmask: %s\n", inet_ntoa(addr));
            addr.s_addr = htonl(station_config.static_ip_config.dns_addr);
            printf("\nDNS Address : %s\n", inet_ntoa(addr));
          }
        }
        else
        {
          printf("\nGet LAN Config failed, Error:0x%x", qmi_err_num);
        }
        break;
      }
      case 38:
      {
        if(QcMapClient->ActivateWLAN(&qmi_err_num))
        {
          printf("\nActivated WLAN\n");
        }
        else
          printf("\nFailed to Activate WLAN, Error: 0x%x", qmi_err_num);
        break;
      }
      case 39:
      {
        int p_error=0;
        qcmap_msgr_lan_config_v01 lan_config;
        in_addr addr;
        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
        memset(&lan_config,0,sizeof(qcmap_msgr_lan_config_v01));

        printf("   Do you want to set LAN Configuration(1-Yes/0-No) : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        if (atoi(scan_string) == 1)
        {
          while (TRUE)
          {
            printf("   Please input Gateway IP address : ");
            if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
            {
              memset(&addr,0,sizeof(in_addr));
              if ( !(inet_aton(scan_string, &addr) <= 0) )
              {
                lan_config.gw_ip = ntohl(addr.s_addr);
                break;
              }
            }
            printf("      Invalid IPv4 address %d\n", scan_string);
          }
          while (TRUE)
          {
            printf("   Please input AP subnet  : ");
            if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
            {
              memset(&addr,0,sizeof(in_addr));
              if ( !(inet_aton(scan_string, &addr) <= 0) )
              {
                lan_config.netmask = ntohl(addr.s_addr);
                break;
              }
            }
            printf("      Invalid IPv4 address %d\n", scan_string);
          }
          printf("   Please input Enable/Disable DHCP(1-Enable/0-Disable):");
          fgets(scan_string, sizeof(scan_string), stdin);
          if ( atoi(scan_string) == 1)
          {
            lan_config.enable_dhcp = 1;
            printf("   Please input DHCP Configuration\n");
            while (TRUE)
            {
              printf("   Please input starting DHCPD address : ");
              if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
              {
                memset(&addr,0,sizeof(in_addr));
                if ( !(inet_aton(scan_string, &addr) <= 0) )
                {
                  lan_config.dhcp_config.dhcp_start_ip = ntohl(addr.s_addr);
                  break;
                }
              }
              printf("      Invalid IPv4 address %d\n", scan_string);
            }
            while (TRUE)
            {
              printf("   Please input ending DHCPD address : ");
              if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
              {
                memset(&addr,0,sizeof(in_addr));
                if ( !(inet_aton(scan_string, &addr) <= 0) )
                {
                  lan_config.dhcp_config.dhcp_end_ip = ntohl(addr.s_addr);
                  // Range check
                  if (lan_config.dhcp_config.dhcp_end_ip > lan_config.dhcp_config.dhcp_start_ip) {
                     break;
                  } else {
                     LOG_MSG_ERROR("SSID1 (AP Mode) DHCP Address Range provided is too short",0,0,0);
                     printf(" Please enter a different DHCP End Address. \n");
                  }
                }
              }
              printf("      Invalid IPv4 address %d\n", scan_string);
            }
            while(TRUE)
            {
              printf("   Please input DHCP lease time in Seconds:");
              printf("   Minimum DHCP lease time in 120 Seconds:");
              fgets(scan_string, sizeof(scan_string), stdin);
              lan_config.dhcp_config.lease_time = (uint32)atoi(scan_string);
              if(lan_config.dhcp_config.lease_time >= MIN_DHCP_LEASE)
              break;
            }
          }
          else
          {
            lan_config.enable_dhcp = 0;
          }
        }

        if (QcMapClient->SetLANConfig(lan_config, &qmi_err_num))
        {
          if (qmi_err_num ==  QMI_ERR_NONE_V01) {
            printf("\n LAN Config Set Successfully\n");
          } else {
              if (qmi_err_num == QMI_ERR_INVALID_ARG_V01) {
                printf("SSID1 (AP Mode) DHCP Address Range provided is invalid, Minimum range is 7 \n");
                printf("Setting AP DHCP address to default values which are derived from AP Gateway Addr \n");
              } else {
                printf("\n LAN Config set fails, Error: 0x%x", qmi_err_num);
              }
          }
        }
        else
        {
          printf("\n LAN Config set fails, Error: 0x%x", qmi_err_num);
        }
        break;
      }
      case 40:
      {
        qcmap_msgr_lan_config_v01 lan_config;
        in_addr addr;
        int p_error=0;
        memset(&lan_config,0,sizeof(qcmap_msgr_lan_config_v01));
        memset(&addr, 0 ,sizeof(in_addr));
        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
        if (QcMapClient->GetLANConfig(&lan_config,&qmi_err_num))
        {
          /* Print AP Configuration. */
          printf("\nAP Configuration.\n");
          addr.s_addr = htonl(lan_config.gw_ip);
          printf("\nGateway IP: %s\n", inet_ntoa(addr));
          addr.s_addr = htonl(lan_config.netmask);
          printf("\nNetmask : %s\n", inet_ntoa(addr));
          printf("\nDHCP Enabled: %d\n", lan_config.enable_dhcp);
          if ( lan_config.enable_dhcp == TRUE )
          {
            addr.s_addr = htonl(lan_config.dhcp_config.dhcp_start_ip);
            printf("\nDHCP Start IP: %s\n", inet_ntoa(addr));
            addr.s_addr = htonl(lan_config.dhcp_config.dhcp_end_ip);
            printf("\nDHCP End IP : %s\n", inet_ntoa(addr));
            printf("\nDHCP Lease Time (seconds) : %d\n", lan_config.dhcp_config.lease_time);
          }
        }
        else
        {
          printf("\nGet LAN Config failed, Error:0x%x", qmi_err_num);
        }
        break;
      }
      case 41:
      {
        if(QcMapClient->ActivateLAN(&qmi_err_num))
        {
          printf("\nActivated LAN\n");
        }
        else
          printf("\nFailed to Activate LAN, Error: 0x%x", qmi_err_num);
        break;
      }
      case 42:
      {
        qcmap_msgr_wlan_mode_enum_v01 wlan_mode;
        int p_error=0;
        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
        if (QcMapClient->GetWLANStatus(&wlan_mode, &qmi_err_num))
        {
          switch (wlan_mode)
          {
            case QCMAP_MSGR_WLAN_MODE_AP_V01:
              printf("\nCurrent WLAN Mode is AP.\n");
              break;
            case QCMAP_MSGR_WLAN_MODE_AP_AP_V01:
              printf("\nCurrent WLAN Mode is AP-AP.\n");
              break;
            case QCMAP_MSGR_WLAN_MODE_AP_STA_V01:
              printf("\nCurrent WLAN Mode is AP-STA.\n");
              break;
            case QCMAP_MSGR_WLAN_MODE_ENUM_MIN_ENUM_VAL_V01:
              printf("\nWLAN is not UP.\n");
              break;
            default:
              printf("\nUnsupported WLAN Mode:- %d.\n", wlan_mode);
              break;
          }
        }
        else
          printf("\nGet WLAN Status failed, Error: 0x%x", qmi_err_num);
        break;
      }
      /* Enable/Disable IPV6. */
      case 43:
      {
        printf("   Please input IPV6 State (1-Enable/0-Disable) : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        if (atoi(scan_string))
        {
          if (QcMapClient->EnableIPV6(&qmi_err_num))
            printf("\nIPV6 Enable succeeds.");
          else
            printf("\nIPV6 Enable fails, Error: 0x%x", qmi_err_num);
        }
        else
        {
          if (QcMapClient->DisableIPV6(&qmi_err_num))
            printf("\nIPV6 Disable in progress.");
          else
            printf("\nIPV6 Disable request fails, Error: 0x%x", qmi_err_num);
        }
        break;
      }
      case 44:
      {
        boolean enable_firewall, pkts_allowed = false;
        printf("   Please input Firewall State (1-Enable/0-Disable) : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        enable_firewall = atoi(scan_string);
        if (enable_firewall)
        {
          printf("   Please input Packets Allowed Setting(1-ACCEPT/0-Drop) : ");
          fgets(scan_string, sizeof(scan_string), stdin);
          pkts_allowed = atoi(scan_string);
        }
        if (QcMapClient->SetFirewall(enable_firewall, pkts_allowed, &qmi_err_num))
        {
          printf("Set Firewall config\n");
        }
        else
        {
          printf("Set Firewall config Error: 0x%x", qmi_err_num);
        }
        break;
      }
      case 45:
       {
        boolean enable_firewall, pkts_allowed;

        if (QcMapClient->GetFirewall(&enable_firewall, &pkts_allowed, &qmi_err_num))
        {
          if(enable_firewall)
            printf("Firewall is Enabled\n");
          else
            printf("Firewall is Disabled \n");

           if(pkts_allowed)
            printf("Firewall is configured to ACCEPT packets\n");
          else
            printf("Firewall is configured to DROP packets\n");
        }
        else
         printf("Get Firewall configuration failed,Error 0x%x", qmi_err_num);
        break;
      }
      /* Get ipv6 state .     */
      case 46:
      {
        DisplayIPv6State();
        break;
      }
      /* get WWAN policy    */
      case 47:
      {
        DisplayWWANPolicy();
        break;
      }
      /* set WWAN config */
      case 48:
      {
        qcmap_msgr_ip_family_enum_v01 ip_family ;
        qcmap_msgr_net_policy_info_v01 net_policy;
        memset(&net_policy,0,sizeof(qcmap_msgr_net_policy_info_v01));

        printf("Please select Technology (0-ANY, 1-UMTS, 2-CDMA) : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        net_policy.tech_pref = atoi(scan_string);
        if ( net_policy.tech_pref != 0 && net_policy.tech_pref != 1 &&
             net_policy.tech_pref != 2)
        {
           printf ("\n Invalid tech preference\n");
           break;
        }
        printf("Please input IP Family IPV4-4 IPV6-6 IPV4V6-10 : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        ip_family = (qcmap_msgr_ip_family_enum_v01)atoi(scan_string);
        if ( ip_family == QCMAP_MSGR_IP_FAMILY_V4_V01 )
        {
          printf("   Please enter UMTS Profile Number : ");
          fgets(scan_string, sizeof(scan_string), stdin);
          net_policy.v4_profile_id_3gpp = atoi(scan_string);
          printf("   Please enter CDMA Profile Number : ");
          fgets(scan_string, sizeof(scan_string), stdin);
          net_policy.v4_profile_id_3gpp2 = atoi(scan_string);
        }
        else if ( ip_family == QCMAP_MSGR_IP_FAMILY_V6_V01 )
        {
           printf("   Please enter UMTS Profile Number : ");
           fgets(scan_string, sizeof(scan_string), stdin);
           net_policy.v6_profile_id_3gpp = atoi(scan_string);
           printf("   Please enter CDMA Profile Number : ");
           fgets(scan_string, sizeof(scan_string), stdin);
           net_policy.v6_profile_id_3gpp2 = atoi(scan_string);
        }
        else if ( ip_family == QCMAP_MSGR_IP_FAMILY_V4V6_V01 )
        {
           printf("Please enter V4 UMTS Profile Number : ");
           fgets(scan_string, sizeof(scan_string), stdin);
           net_policy.v4_profile_id_3gpp = atoi(scan_string);
           printf("   Please enter V4 CDMA Profile Number : ");
           fgets(scan_string, sizeof(scan_string), stdin);
           net_policy.v4_profile_id_3gpp2 = atoi(scan_string);
           printf("Please enter V6 UMTS Profile Number : ");
           fgets(scan_string, sizeof(scan_string), stdin);
           net_policy.v6_profile_id_3gpp = atoi(scan_string);
           printf("   Please enter V6 CDMA Profile Number : ");
           fgets(scan_string, sizeof(scan_string), stdin);
           net_policy.v6_profile_id_3gpp2 = atoi(scan_string);
        }
        else
        {
           printf("\nUnsupported ip mode:- %d.\n", ip_family);
           break;
        }
        net_policy.ip_family = ip_family;
        if (QcMapClient->SetWWANPolicy(net_policy, &qmi_err_num))
        printf("  Set Wwan policy succeeds.\n. ");
        else
        printf("  Failed to Set WWAN policy .Error 0x%x.\n ", qmi_err_num);
        break;
      }
      /* get UPnP status    */
      case 49:
      {
        qcmap_msgr_upnp_mode_enum_v01 upnp_state;
        if(QcMapClient->GetUPNPStatus( &upnp_state, &qmi_err_num))
        {
          if (upnp_state == QCMAP_MSGR_UPNP_MODE_UP_V01)
          {
            printf("\nUPnP is enabled");
          }
          else
          {
            printf("\nUPnP is disabled");
          }
        }
        else
        {
          printf("\nGetUPNPStatus returns Error: 0x%x", qmi_err_num);
        }
        break;
      }
      /* get DLNA status */
      case 50:
      {
        qcmap_msgr_dlna_mode_enum_v01 dlna_state;
        if(QcMapClient->GetDLNAStatus( &dlna_state, &qmi_err_num))
        {
          if (dlna_state == QCMAP_MSGR_DLNA_MODE_UP_V01)
          {
            printf("\nDLNA is enabled");
          }
          else
          {
            printf("\nDLNA is disabled");
          }
        }
        else
        {
          printf("\nGetDLNAStatus returns Error: 0x%x", qmi_err_num);
        }
        break;
      }
      /* Get MDNS status. */
      case 51:
      {
        qcmap_msgr_mdns_mode_enum_v01 mdns_state;
        if(QcMapClient->GetMDNSStatus( &mdns_state, &qmi_err_num))
        {
          if (mdns_state == QCMAP_MSGR_MDNS_MODE_UP_V01)
          {
            printf("\nMDNS is enabled");
          }
          else
          {
            printf("\nMDNS is disabled");
          }
        }
        else
        {
          printf("\nGetMDNSStatus returns Error: 0x%x", qmi_err_num);
        }
        break;
      }
      /* Get station mode status. */
      case 52:
      {
        qcmap_msgr_station_mode_status_enum_v01 status;
        if (QcMapClient->GetStationModeStatus(&status, &qmi_err_num))
        {
          if (status == QCMAP_MSGR_STATION_MODE_CONNECTED_V01)
            printf("\nMobile AP Station Mode is Connected");
          else if(status == QCMAP_MSGR_STATION_MODE_DISCONNECTED_V01)
            printf("\nMobile AP Station Mode is Disconnected");
          else
            printf("\nIncorrect state returned: 0x%x", status);
        }
        else
          printf("  Failed to Get Station Mode Status .Error 0x%x.\n ", qmi_err_num);
      }
      break;
      /* set DLNA media directory */
      case 53:
      {
        char media_dir_get[QCMAP_MSGR_MAX_DLNA_DIR_LEN_V01] = "";
        char media_dir_set[QCMAP_MSGR_MAX_DLNA_DIR_LEN_V01] = "";
        char *ptr;

        if(QcMapClient->GetDLNAMediaDir( media_dir_get, &qmi_err_num))
        {
          printf("\nCurrent DLNA Media Dir('s):");
          printf("\n%s\n", media_dir_get);
          printf("   Do you wish to keep these Directories?(1-YES/0-NO) : ");

          fgets(scan_string, sizeof(scan_string), stdin);
          int enable = (atoi(scan_string)) ? true : false;

          if (enable)
          {
            strlcpy(media_dir_set, media_dir_get, sizeof(media_dir_set));

            //replace all newlines with ','
            ptr = strchr(media_dir_set, '\n');
            while (ptr != NULL)
            {
              media_dir_set[ptr-media_dir_set] = ',';
              ptr = strchr(ptr+1, '\n');
            }
            strlcat(media_dir_set, ",", sizeof(media_dir_set));
          }
        }
        printf("   Please input a valid Media Directory (\",\" to seperate multiple):");
        if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
        {
          strlcat(media_dir_set, scan_string, sizeof(media_dir_set));
          if(QcMapClient->SetDLNAMediaDir( media_dir_set, &qmi_err_num))
          {
            printf("\nDLNA Media Dir added!\n");
          }
          else
          {
            if (qmi_err_num != QMI_ERR_NO_EFFECT_V01)
            {
              printf("\nSetDLNAMediaDir returns Error: 0x%x", qmi_err_num);
            }
            else
            {
              printf("\nSetDLNAMediaDir succeeds but restart failed");
            }
          }
        }
        else
          printf("      Invalid Media Directory: %s", scan_string);
        break;
      }
      /* Get DLNA media directory */
      case 54:
      {
        char media_dir[QCMAP_MSGR_MAX_DLNA_DIR_LEN_V01] = "";
        if(QcMapClient->GetDLNAMediaDir( media_dir, &qmi_err_num))
        {
          printf("\nCurrent DLNA Media Dir('s):");
          printf("\n%s\n", media_dir);
        }
        else
        {
          printf("\nGetDLNAMediaDir returns Error: 0x%x", qmi_err_num);
        }
        break;
      }

      case 55:
      {
        qcmap_msgr_bootup_flag_v01 mobileap_enable, wlan_enable;
        printf(" Do you want to change MobileAP bootup configuration (1- Change /0- Do not Change) : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        if (atoi(scan_string)) {
           printf(" Do you want to enable MobileAP on bootup (1-Enable/0-Disable) : ");
           fgets(scan_string, sizeof(scan_string), stdin);
           if (atoi(scan_string)) {
              mobileap_enable = QCMAP_MSGR_ENABLE_ON_BOOT_V01;
           } else {
              mobileap_enable = QCMAP_MSGR_DISABLE_ON_BOOT_V01;
           }
        } else {
           mobileap_enable = QCMAP_MSGR_BOOTUP_FLAG_MIN_ENUM_VAL_V01;
        }
        printf(" Do you want to change WLAN bootup configuration (1- Change /0- Do not Change) : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        if (atoi(scan_string)) {
           printf(" Do you want to enable WAN on bootup (1-Enable/0-Disable) : ");
           fgets(scan_string, sizeof(scan_string), stdin);
           if (atoi(scan_string)) {
              wlan_enable = QCMAP_MSGR_ENABLE_ON_BOOT_V01;
           } else {
              wlan_enable = QCMAP_MSGR_DISABLE_ON_BOOT_V01;
           }
        } else {
           wlan_enable = QCMAP_MSGR_BOOTUP_FLAG_MIN_ENUM_VAL_V01;
        }
        if (QcMapClient->SetQCMAPBootupCfg(mobileap_enable, wlan_enable, &qmi_err_num)){
            printf("\n QCMAP Bootup configuration set successfully set \n");
        } else {
            printf("\n  Set QCMAP Bootup Cfg Fails , Error: 0x%x \n", qmi_err_num);
        }
        break;
      }
      case 56:
      {
        qcmap_msgr_bootup_flag_v01 mobileap_enable, wlan_enable;
        if (QcMapClient->GetQCMAPBootupCfg(&mobileap_enable, &wlan_enable, &qmi_err_num)){
            printf("\n Mobile AP will be %s on bootup \n",((mobileap_enable == QCMAP_MSGR_ENABLE_ON_BOOT_V01) ?"ENABLED":"DISABLED"));
            printf("\n WLAN will be %s on bootup \n",((wlan_enable == QCMAP_MSGR_ENABLE_ON_BOOT_V01)?"ENABLED":"DISABLED"));
        } else {
            printf("\n  Get QCMAP Bootup Cfg Fails , Error: 0x%x \n", qmi_err_num);
        }
        break;
      }
      /* Enable/Disable IPV4. */
      case 57:
      {
        printf("   Please input IPV4 State (1-Enable/0-Disable) : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        if (atoi(scan_string))
        {
          if (QcMapClient->EnableIPV4(&qmi_err_num))
            printf("\nIPV4 Enable succeeds.");
          else
            printf("\nIPV4 Enable fails, Error: 0x%x", qmi_err_num);
        }
        else
        {
          if (QcMapClient->DisableIPV4(&qmi_err_num))
            printf("\nIPV4 Disable in progress.");
          else
            printf("\nIPV4 Disable request fails, Error: 0x%x", qmi_err_num);
        }
        break;
      }
      /* Get ipv4 state .     */
      case 58:
      {
        boolean ipv4_state;
        memset((void *)&ipv4_state, 0, sizeof(uint8_t));

        if (QcMapClient->GetIPv4State( &ipv4_state, &qmi_err_num))
        {
           printf("\nIPV4 is: %s.\n",(ipv4_state)?"Enabled":"Disabled");
        }
        else
        {
          printf("\nGetIPV4State returns Error: 0x%x", qmi_err_num);
        }
        break;
      }
      /* Get data bitrates */
      case 59:
      {
        qcmap_msgr_data_bitrate_v01 data_rate;
        memset(&data_rate, 0, sizeof(qcmap_msgr_data_bitrate_v01));
        if (QcMapClient->GetDataRate(&data_rate, &qmi_err_num)){
            printf("\n Data Rates:");
            printf("\n Current tx: %llu", data_rate.tx_rate);
            printf("\n Current rx: %llu", data_rate.rx_rate);
            printf("\n Max tx: %llu", data_rate.max_tx_rate);
            printf("\n Max rx: %llu\n", data_rate.max_rx_rate);
        } else {
            printf("\n  Get Data Bitrate Fails , Error: 0x%x \n", qmi_err_num);
        }
        break;
      }
      /* Set UPnP notify interval */
      case 60:
      {
        int upnp_notify_int;
        qcmap_msgr_upnp_mode_enum_v01 upnp_state;

        printf("   Please input UPnP notify interval in seconds (%d-%d):",
               MIN_NOTIFY_INTERVAL, MAX_NOTIFY_INTERVAL);
        fgets(scan_string, sizeof(scan_string), stdin);
        upnp_notify_int = atoi(scan_string);
        if (upnp_notify_int >= MIN_NOTIFY_INTERVAL && upnp_notify_int <= MAX_NOTIFY_INTERVAL)
        {
          printf("\nUPnP notify interval set!\n");
          if(QcMapClient->SetUPNPNotifyInterval(upnp_notify_int, &qmi_err_num))
          {
            if (QcMapClient->GetUPNPStatus( &upnp_state, &qmi_err_num))
            {
              if (upnp_state == QCMAP_MSGR_UPNP_MODE_UP_V01)
              {
                printf("\nThis change will not take effect until restart.\n");
                printf("   Do you want to restart now? (1-yes/0-no) : ");
                fgets(scan_string, sizeof(scan_string), stdin);
                if (atoi(scan_string) == 1)
                {
                  if (QcMapClient->DisableUPNP(&qmi_err_num))
                  {
                    printf("\nUPNP has been stopped.");
                    if (QcMapClient->EnableUPNP(&qmi_err_num))
                      printf("\nUPNP Restart succeeds.");
                    else
                      printf("\nUPNP Restart fails, Error: 0x%x", qmi_err_num);
                  }
                  else
                    printf("\nUPNP Restart fails, Error: 0x%x", qmi_err_num);
                }
              }
              else
                printf("\nGetUPNPStatus returns Error: 0x%x", qmi_err_num);
            }
          }
          else
          {
            printf("\nUPnP notify interval returns Error: 0x%x", qmi_err_num);
          }
        }
        else
          printf("      Invalid UPnP notify interval, must be in range %d-%d: %s",
                 MIN_NOTIFY_INTERVAL, MAX_NOTIFY_INTERVAL, scan_string);
        break;
      }
      /* Get UPnP notify interval */
      case 61:
      {
        int upnp_notify_int = 0;

        if(QcMapClient->GetUPNPNotifyInterval(&upnp_notify_int, &qmi_err_num))
        {
          printf("\nCurrent UPnP notify interval: %d\n", upnp_notify_int);
        }
        else
        {
          printf("\nUPnP notify interval returns Error: 0x%x", qmi_err_num);
        }
        break;
      }
      /* Set DLNA notify interval */
      case 62:
      {
        int dlna_notify_int;
        qcmap_msgr_dlna_mode_enum_v01 dlna_state;

        printf("   Please input DLNA notify interval in seconds (%d-%d):",
               MIN_NOTIFY_INTERVAL, MAX_NOTIFY_INTERVAL);
        fgets(scan_string, sizeof(scan_string), stdin);
        dlna_notify_int = atoi(scan_string);
        if (dlna_notify_int >= MIN_NOTIFY_INTERVAL && dlna_notify_int <= MAX_NOTIFY_INTERVAL)
        {
          if(QcMapClient->SetDLNANotifyInterval(dlna_notify_int, &qmi_err_num))
          {
            printf("\nDLNA notify interval set!\n");
            if(QcMapClient->GetDLNAStatus( &dlna_state, &qmi_err_num))
            {
              if (dlna_state == QCMAP_MSGR_DLNA_MODE_UP_V01)
              {
                printf("\nThis change will not take effect until restart.\n");
                printf("   Do you want to restart now? (1-yes/0-no) : ");
                fgets(scan_string, sizeof(scan_string), stdin);
                if (atoi(scan_string) == 1)
                {
                  if (QcMapClient->DisableDLNA(&qmi_err_num))
                  {
                    printf("\nDLNA has been stopped.");
                    if (QcMapClient->EnableDLNA(&qmi_err_num))
                      printf("\nDLNA Restart succeeds.");
                    else
                      printf("\nDLNA Restart fails, Error: 0x%x", qmi_err_num);
                  }
                  else
                    printf("\nDLNA Restart fails, Error: 0x%x", qmi_err_num);
                }
              }
            }
            else
              printf("\nGetDLNAStatus returns Error: 0x%x", qmi_err_num);
          }
          else
          {
            printf("\nDLNA notify interval returns Error: 0x%x", qmi_err_num);
          }
        }
        else
          printf("      Invalid DLNA notify interval, must be in range %d-%d: %s",
                 MIN_NOTIFY_INTERVAL, MAX_NOTIFY_INTERVAL, scan_string);
        break;
      }
      /* Get DLNA notify interval */
      case 63:
      {
        int dlna_notify_int = 0;

        if(QcMapClient->GetDLNANotifyInterval(&dlna_notify_int, &qmi_err_num))
        {
          printf("\nCurrent DLNA notify interval: %d\n", dlna_notify_int);
        }
        else
        {
          printf("\nDLNA notify interval returns Error: 0x%x", qmi_err_num);
        }
        break;
      }
      /*Add DHCP Reservation record*/
      case 64:
      {
          memset(&dhcp_reserv_record,0,sizeof(qcmap_msgr_dhcp_reservation_v01));
          int i = 0;
          boolean enable = TRUE, device_type = FALSE;
          dhcp_reserv_record.enable_reservation = enable;

          printf("\nEnter the device type(0-USB/1-AP):   ");
          fgets(scan_string,sizeof(scan_string),stdin);
          device_type = (atoi(scan_string)) ? true:false;
          if ( device_type )
          {
             GET_MAC_ADDR(scan_string);
             memcpy(dhcp_reserv_record.client_mac_addr, mac_addr_int,\
                    sizeof(dhcp_reserv_record.client_mac_addr));
          }
          GET_IP_ADDR(scan_string,addr);
          dhcp_reserv_record.client_reserved_ip = ntohl(addr.s_addr);
          while(TRUE)
          {
            printf("Enter device name.If client is AP client,press \"ENTER\" key to skip  :");
            if ( fgets(scan_string,sizeof(scan_string),stdin) != NULL )
            {
              if (*scan_string == '\n')
              {
                if (device_type)
                {
                  break;
                }
                else
                {
                  printf("Client name is mandatory for tethered client\n");
                  continue;
                }
              }
              else
              {
                for ( i=0;i < strlen(scan_string)-1;i++)
                {
                  dhcp_reserv_record.client_device_name[i] = scan_string[i];
                }
                dhcp_reserv_record.client_device_name[i] ='\0';
                break;
              }
            }
          }
          printf("Enable/disable reservation for this client(1-Enable/0-Disable/Enter-skipped  :");
          fgets(scan_string,sizeof(scan_string),stdin);

          if ( *scan_string != '\n')
          {
              enable = (atoi(scan_string)) ? true:false;
              dhcp_reserv_record.enable_reservation = enable;
          }
          if( QcMapClient->AddDHCPReservRecord(&dhcp_reserv_record, &qmi_err_num) )
          {
              printf("\nDHCP  Reservation Record added successfully");
          }
          else
          {
             printf("\nFailed to add DHCP Reservation record. Error 0x%x.\n ", qmi_err_num);
          }
          break;
      }
      /*Dsiplay DHCP Reservation record*/
      case 65:
      {
          uint32_t num_entrees =0, i=0;;
          in_addr tmpIP;
          qcmap_msgr_dhcp_reservation_v01 dhcp_reserv_record[QCMAP_MSGR_MAX_DHCP_RESERVATION_ENTRIES_V01];
          memset(dhcp_reserv_record,0,\
                 QCMAP_MSGR_MAX_DHCP_RESERVATION_ENTRIES_V01*sizeof(qcmap_msgr_dhcp_reservation_v01));

          if( QcMapClient->GetDHCPReservRecords(dhcp_reserv_record, &num_entrees,&qmi_err_num) )
          {
             //display each DHCP reservation records
            if ( num_entrees == 0 )
            {
              printf("\nNo DHCP Reservation Records");
            }
            else
            {
               for ( i = 0;i < num_entrees; i++)
               {
                 printf("\nEntry  %d:",i);
                 mac_addr_ntop(dhcp_reserv_record[i].client_mac_addr);
                 if ( strncmp(mac_addr_str,MAC_NULL_STRING,QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01) != 0 )
                 {
                     printf("\nMAC address of the client[%i]: %s",i,mac_addr_str);
                 }
                 tmpIP.s_addr =ntohl(dhcp_reserv_record[i].client_reserved_ip);
                 printf("\nIP address of the client[%i]: %s",i,inet_ntoa(tmpIP));
                 if ( dhcp_reserv_record[i].client_device_name[0] != '\0')
                 {
                   printf("\nDevice Name of the client[%i]: %s",i,\
                          dhcp_reserv_record[i].client_device_name);
                 }
                 printf("\nDHCP Reservation enabled for the client[%i]: %d",i,\
                        dhcp_reserv_record[i].enable_reservation);
               }
             }
          }
          else
          {
             printf("\nFailed to Dsiplay DHCP Reservation record. Error 0x%x.\n ", qmi_err_num);
          }
          break;
      }

      /*Edit DHCP Reservation record*/
      case 66:
      {
          memset(&dhcp_reserv_record,0,sizeof(qcmap_msgr_dhcp_reservation_v01));
          uint32_t addr_to_edit =0;
          addr.s_addr =0;
          uint8 options=0;
          boolean enable;

          GET_IP_ADDR(scan_string,addr);
          addr_to_edit = ntohl(addr.s_addr);
          dhcp_reserv_record.enable_reservation = true;
          while (TRUE )
          {
             printf("Please enter the field to edit:  ");
             printf("\n\t1. MAC Address\n\t2. IP Addr\n\t3. Device Name\n\t4. Enable/Disable\n\t:");
             fgets(scan_string,sizeof(scan_string),stdin);
             options = atoi(scan_string);
             switch (options)
             {
                case 1:
                    GET_MAC_ADDR(scan_string);
                    memcpy(dhcp_reserv_record.client_mac_addr, mac_addr_int,\
                           sizeof(dhcp_reserv_record.client_mac_addr));
                    break;

                 case 2:
                   addr.s_addr =0;
                   GET_IP_ADDR(scan_string,addr);
                   dhcp_reserv_record.client_reserved_ip = ntohl(addr.s_addr);
                   break;

                 case 3:
                    while(TRUE)
                    {
                        printf("Please input the device name  :");
                        if ( fgets(scan_string,sizeof(scan_string),stdin) != NULL )
                        {
                             if ( *scan_string != '\n')
                             {
                                for ( i=0;i < strlen(scan_string)-1;i++)
                                {
                                    dhcp_reserv_record.client_device_name[i] = scan_string[i];
                                }
                                dhcp_reserv_record.client_device_name[i] ='\0';
                               break;
                             }
                        }
                        printf("\nInvalid Device name entered %s", scan_string);
                    }
                    break;
                 case 4:
                    printf("Enable/disable reservation for this client(1-Enable/0-Disable   :");
                    fgets(scan_string,sizeof(scan_string),stdin);
                    enable = (atoi(scan_string)) ? true:false;
                    dhcp_reserv_record.enable_reservation = enable;
                    break;
                default:
                  printf("Invalid response %d\n",options);
                  break;
             }
             printf("Do you wish to Edit more fields(Enter-skipped/other character to continue):");
             fgets(scan_string,sizeof(scan_string),stdin);
             if ( *scan_string == '\n')
             {
                break;
             }
             else
             {
                continue;
             }
          }
          if( QcMapClient->EditDHCPReservRecord(&addr_to_edit,&dhcp_reserv_record,&qmi_err_num) )
          {
            printf("\nDHCP  Reservation Record edited successfully");
          }
          else
         {
            printf("\nFailed to edit DHCP Reservation record. Error 0x%x.\n ", qmi_err_num);
         }
         break;
      }
      /*Delete DHCP Reservation record*/
      case 67:
      {
          uint32_t addr_to_edit =0;
          addr.s_addr =0;

          GET_IP_ADDR(scan_string,addr);

          addr_to_edit = ntohl(addr.s_addr);
          if( QcMapClient->DeleteDHCPReservRecord(&addr_to_edit,&qmi_err_num) )
          {
              printf("\nDHCP  Reservation Record deleted successfully");
          }
          else
          {
             printf("\nFailed to delete DHCP Reservation record. Error 0x%x.\n ", qmi_err_num);
          }
          break;
      }
      /* Activate Hostapd with the new config. */
      case 68:
      {
        qcmap_msgr_activate_hostapd_ap_enum_v01 ap_type;
        qcmap_msgr_activate_hostapd_action_enum_v01 action_type;
        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
        printf("Select the AP Type for which settings need to be activated : \n"
               "1: Primary AP\t2: Guest AP\n"
               "3: Both\t:::");
        fgets(scan_string, sizeof(scan_string), stdin);
        ap_type = (qcmap_msgr_activate_hostapd_ap_enum_v01)atoi(scan_string);
        if ( ap_type < QCMAP_MSGR_PRIMARY_AP_V01 ||
             ap_type > QCMAP_MSGR_PRIMARY_AND_GUEST_AP_V01 )
        {
          printf("\n\nInvalid AP Type : %d\n", ap_type);
          break;
        }
        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
        printf("Select the Action Type for hostapd which need to be activated : \n"
               "1: Start\t2: Stop\n"
               "3: Restart\t:::");
        fgets(scan_string, sizeof(scan_string), stdin);
        action_type = (qcmap_msgr_activate_hostapd_action_enum_v01)atoi(scan_string);
        if ( action_type < QCMAP_MSGR_HOSTAPD_START_V01 ||
             action_type > QCMAP_MSGR_HOSTAPD_RESTART_V01 )
        {
          printf("\n\nInvalid Action Type : %d\n", action_type);
          break;
        }
        if(QcMapClient->ActivateHostapdConfig(ap_type, action_type, &qmi_err_num))
        {
          printf("\nActivated Hostapd with the new config\n");
        }
        else
          printf("\nFailed to Activate Hostapd, Error: 0x%x", qmi_err_num);
        break;
      }
      /* Activate Supplicant with the new config. */
      case 69:
      {
        if(QcMapClient->ActivateSupplicantConfig(&qmi_err_num))
        {
          printf("\nActivated Supplicant with the new config\n");
        }
        else
          printf("\nFailed to Activate Supplicant, Error: 0x%x", qmi_err_num);
        break;
      }
      /* Disconnect BackHaul, disable LAN and exit application. */
      case 70:
      {
        sighandler(SIGTERM);
        exit(1);
      }
        break;

      /* Invalid integer entered. */
      default :
      {
        printf("Invalid response %d\n", opt);
      }
        break;
    }
  }
  return 0;
}
