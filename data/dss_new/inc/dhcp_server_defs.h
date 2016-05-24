#ifndef DHCP_SERVER_DEFS_H
#define DHCP_SERVER_DEFS_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                           D H C P _ S E R V E R _ D E F S . H

GENERAL DESCRIPTION
  AMSS Dynamic Host Configuration Protocol public header file.
  This header file contains the definitions and declarations to interface 
  with the DHCP server.

Copyright (c) 2010-2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

$Header: //source/qcom/qct/modem/api/datamodem/main/latest/dhcp_server_defs.h#1 $ $DateTime: 2011/01/10 09:44:56 $ $Author: maldinge $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
01/05/11    cp     Added support for the reservation ip leases.
10/08/10    cp     Added support for getting hostname and connection time 
                   using dhcp_server_get_conn_info() API. Also added 
                   support to make lease time configurable.                   
07/02/10    cp     Moved the declarations and definitions 
                   from dhcp_server_mgr.h to here.
===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "ps_in.h"

/*===========================================================================

                      DEFINITIONS AND DECLARATIONS

===========================================================================*/

/* -------------------------------------------------------------------
   Enum for the type of message.
  ------------------------------------------------------------------- */
typedef enum {
  DHCP_MIN_MSG = 0,
  DHCP_DHCPDISCOVER = 1,
  DHCP_DHCPOFFER = 2,
  DHCP_DHCPREQUEST = 3,
  DHCP_DHCPDECLINE = 4,
  DHCP_DHCPACK = 5,
  DHCP_DHCPNACK = 6,
  DHCP_DHCPRELEASE = 7,
  DHCP_DHCPINFORM = 8,
  DHCP_MAX_MSG
} dhcp_msg_type;

/* -------------------------------------------------------------------
  Callback invoked on various DHCP messages being rx'ed/tx'ed.

  userdata    - userdata passed in when calling 
                dhcp_server_mgr_start().
  msg_type    - type of the DHCP message.
  ipv4_addr   - IPv4 address assigned.
  ------------------------------------------------------------------- */
typedef void (* dhcp_msg_cback_type)(void * userdata,
                                     dhcp_msg_type msg_type,
                                     uint32 ipv4_addr);

/* -------------------------------------------------------------------
  This structure is used to get the information about the connected 
  devices.

  client_ip            - IP address of the client.
  client_hw            - HW address of the client.
  client_hw_len        - HW address length.
  client_hostname      - Hostname of the client.
  client_hostname_len  - Hostname length.
  client_conn_time     - Time (in seconds) when client connected.
                         Use time_jul_from_secs API (time_jul.h) to convert
                         secs into Julian format.
  is_static            - boolean indicating whether ip address allocation 
                         is static or dynamic (through DHCP).
  ------------------------------------------------------------------- */
#define DHCP_SERVER_MGR_CLIENT_CHADDR_LEN (16)
#define DHCP_SERVER_MGR_MAX_HOST_NAME_LEN (32)
typedef struct dhcp_server_conn_devices_info
{
  struct ps_in_addr client_ip;
  uint8             client_hw[DHCP_SERVER_MGR_CLIENT_CHADDR_LEN];
  uint32            client_hw_len;
  uint8             client_hostname[DHCP_SERVER_MGR_MAX_HOST_NAME_LEN];
  uint32            client_hostname_len;
  uint32            client_conn_time;
  boolean           is_static;
} dhcp_server_conn_devices_info_s;

/* -------------------------------------------------------------------
  This structure contains the DHCP Reservation Addresses information

  client_ip            - IP address of the client.
  client_hw            - HW address of the client.
  client_hw_len        - HW address length.
  client_hostname      - Hostname of the client.
  client_hostname_len  - Hostname length.
  ------------------------------------------------------------------- */
#define MAX_NUM_OF_DHCP_RESERVATIONS 5
typedef struct dhcp_reservation
{
  struct ps_in_addr        client_ip;
  uint8                    client_hw[DHCP_SERVER_MGR_CLIENT_CHADDR_LEN];
  uint32                   client_hw_len;
  uint8                    client_hostname[DHCP_SERVER_MGR_MAX_HOST_NAME_LEN];
  uint32                   client_hostname_len;
}dhcp_reservation_type;

/* -------------------------------------------------------------------
  This structure is used to provide the DHCP Configuration Parameters.

  ipv4_addr_start  - Starting IP address to be allocated.
  ipv4_addr_end    - Last IP address to be allocated.
  gateway_addr     - Gateway/Router IP address to be communicated to 
                     clients. This will be the server ID.
  net_mask         - Subnet mask.
  prim_dns         - Primary DNS address.
  sec_dns          - Secondary DNS address.
  enable_acd       - Enable address conflict detection.
  lease_time       - Lease time in minutes.
  ------------------------------------------------------------------- */
typedef struct dhcp_server_config_params
{
  struct ps_in_addr     ipv4_addr_start;
  struct ps_in_addr     ipv4_addr_end; 
  struct ps_in_addr     gateway_addr;
  struct ps_in_addr     net_mask;
  struct ps_in_addr     prim_dns;
  struct ps_in_addr     sec_dns;
  dhcp_reservation_type dhcp_reservations[MAX_NUM_OF_DHCP_RESERVATIONS];
  uint8                 num_of_reservations;
  uint32                lease_time;
  boolean               enable_acd;
} dhcp_server_config_params_s;

#endif /* DHCP_SERVER_DEFS_H */
