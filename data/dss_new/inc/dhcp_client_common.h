#ifndef DHCP_CLIENT_COMMON_H
#define DHCP_CLIENT_COMMON_H

/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                  D H C P _ C L I E N T _ C O M M O N. H

GENERAL DESCRIPTION
  DMSS Dynamic Host Configuration Protocol client common header file.
  This header file contains the common enums and typedefs used in the  
  DHCP client public and private interfaces.

Copyright (c) 2007-2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

$Id: //source/qcom/qct/modem/api/datamodem/main/latest/dhcp_client_common.h#1 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
01/25/10    ss     Added DHCP_CLIENT_IP_DECLINED status.
08/07/09    kk     Added support for client identifier option (opt type=61).
12/14/08    pp     Common Modem Interface: Public/Private API split.
08/25/08    cp     Updated changes in draft-ietf-mip6-hiopt-17.txt from
                   draft-ietf-mip6-hiopt-09.txt
05/07/07     es    Initial creation.
===========================================================================*/

/* Max DUID len is 128 + 2(typecode), see RFC 3315, Section 9.1 */
#define DHCP6_CLIENT_MAX_DUID_LENGTH         (130)

/* Max Size of MIP6 HNInfo Value */
#define DHCP6_CLIENT_MIP6_HNINFO_DATA_MAX_SIZE (72)

/* MIPv6 Home Network Identification Option values */
#define DHCP6_CLIENT_HNINFO_OPTION_IDTYPE_LEN      (1) 
/* MIPv6 Home Network Information Option values */
#define DHCP6_CLIENT_HNINFO_SUBOPT_CODE_SIZE       (2)
#define DHCP6_CLIENT_HNINFO_SUBOPT_LEN_SIZE        (2)
#define DHCP6_CLIENT_SUBOPT_CODE_HOMENETWORKID     (1)

/* MIP6 Home Network Information sub-option structure
 * See draft-ietf-mip6-hiopt-17.txt */
struct dhcp6_mip6_hninfo_sub_option
{
  uint16 sub_opt_code;
  uint16 sub_opt_len;
  uint8 hninfo[DHCP6_CLIENT_MIP6_HNINFO_DATA_MAX_SIZE];
};

/* MIP6 Home Network Information option structure
 * See draft-ietf-mip6-hiopt-17.txt */
typedef struct
{
  uint8 id_type;
  struct dhcp6_mip6_hninfo_sub_option hninfo_sub_opt_list;
  uint32 hninfo_len;
} dhcp6_mip6_hnid_option_vals;

/* Hardware type values from "ADDRESS RESOLUTION PROTOCOL PARAMETERS"
 *   - http://www.iana.org/assignments/arp-parameters (updated 12/19/05)
 *  Number Hardware Type (hrd)                              References
 *  ------ -----------------------------------              ----------
 *       1 Ethernet (10Mb)                                       [JBP]
 *       2 Experimental Ethernet (3Mb)                           [JBP]
 *       3 Amateur Radio AX.25                                   [PXK]
 *       4 Proteon ProNET Token Ring                             [JBP]
 *       5 Chaos                                                 [GXP]
 *       6 IEEE 802 Networks                                     [JBP]
 *       7 ARCNET                                                [JBP]
 *       8 Hyperchannel                                          [JBP]
 *       9 Lanstar                                                [TU]
 *      10 Autonet Short Address                                [MXB1]
 *      11 LocalTalk                                            [JKR1]
 *      12 LocalNet (IBM PCNet or SYTEK LocalNET)                [JXM]
 *      13 Ultra link                                           [RXD2]
 *      14 SMDS                                                 [GXC1]
 *      15 Frame Relay                                           [AGM]
 *      16 Asynchronous Transmission Mode (ATM)                 [JXB2]
 *      17 HDLC                                                  [JBP]
 *      18 Fibre Channel                               [Yakov Rekhter]
 *      19 Asynchronous Transmission Mode (ATM)         [Mark Laubach]
 *      20 Serial Line                                           [JBP]
 *      21 Asynchronous Transmission Mode (ATM)                 [MXB1]
 *      22 MIL-STD-188-220                                    [Jensen]
 *      23 Metricom                                            [Stone]
 *      24 IEEE 1394.1995                                     [Hattig]
 *      25 MAPOS                                            [Maruyama]
 *      26 Twinaxial                                           [Pitts]
 *      27 EUI-64                                           [Fujisawa]
 *      28 HIPARP                                                [JMP]
 *      29 IP and ARP over ISO 7816-3                        [Guthery]
 *      30 ARPSec                                            [Etienne]
 *      31 IPsec tunnel                                      [RFC3456]
 *      32 InfiniBand (TM)  [RFC-ietf-ipoib-ip-over-infiniband-09.txt]
 *      33 TIA-102 Project 25 Common Air Interface (CAI)    [Anderson]
 */
typedef enum 
{
  DHCP_CLIENT_HWTYPE_LOOPBACK = 0, /* Not part of RFC */
  DHCP_CLIENT_HWTYPE_ETHERNET = 1,
  DHCP_CLIENT_HWTYPE_EXP_ETHERNET = 2,
  DHCP_CLIENT_HWTYPE_AX_25 = 3,
  DHCP_CLIENT_HWTYPE_TOKEN_RING = 4,
  DHCP_CLIENT_HWTYPE_CHAOS = 5,
  DHCP_CLIENT_HWTYPE_IEEE_802 = 6,
  DHCP_CLIENT_HWTYPE_ARCNET = 7,
  DHCP_CLIENT_HWTYPE_HYPERCHANNEL = 8,
  DHCP_CLIENT_HWTYPE_LANSTAR = 9,
  DHCP_CLIENT_HWTYPE_AUTONET = 10,
  DHCP_CLIENT_HWTYPE_LOCALTALK = 11,
  DHCP_CLIENT_HWTYPE_LOCALNET = 12,
  DHCP_CLIENT_HWTYPE_ULTRA_LINK = 13,
  DHCP_CLIENT_HWTYPE_SMDS = 14,
  DHCP_CLIENT_HWTYPE_FRAME_RELAY = 15,
  DHCP_CLIENT_HWTYPE_ATM_1 = 16,
  DHCP_CLIENT_HWTYPE_HDLC = 17,
  DHCP_CLIENT_HWTYPE_FIBER_CHANNEL = 18,
  DHCP_CLIENT_HWTYPE_ATM_2 = 19,
  DHCP_CLIENT_HWTYPE_SERIAL_LINE = 20,
  DHCP_CLIENT_HWTYPE_ATM_3 = 21,
  DHCP_CLIENT_HWTYPE_MIL_STD_188_220 = 22,
  DHCP_CLIENT_HWTYPE_METRICOM = 23,
  DHCP_CLIENT_HWTYPE_IEEE1394 = 24,
  DHCP_CLIENT_HWTYPE_MAPOS = 25,
  DHCP_CLIENT_HWTYPE_TWINAXIAL = 26,
  DHCP_CLIENT_HWTYPE_EUI_64 = 27,
  DHCP_CLIENT_HWTYPE_HIPARP = 28,
  DHCP_CLIENT_HWTYPE_ISO7816_3 = 29,
  DHCP_CLIENT_HWTYPE_ARPSEC = 30,
  DHCP_CLIENT_HWTYPE_IPSEC_TUNNEL = 31,
  DHCP_CLIENT_HWTYPE_INFINIBAND = 32,
  DHCP_CLIENT_HWTYPE_CAI = 33
} dhcp_client_hw_type_enum;

/* Minimum prescribed length for the client_id option */
#define DHCP_CLIENT_CLIENT_ID_MIN_LEN           (2)
/* Maximum allowed length for the client_id option */
#define DHCP_CLIENT_CLIENT_ID_MAX_LEN           (254)

/* This enumeration type allows the clients to explicitly specify a
 * DHCP client identifier.
 * 0 = DHCP_CLIENT_ID_OPTTYPE_NONE (Default)
 *   The client identifier option is not used.
 * 1 = DHCP_CLIENT_ID_OPTTYPE_HWADDR
 *   The hardware address of the underlying interface (if set) will be
 *   used as the DHCP client identifier.
 * 2 = DHCP_CLIENT_ID_OPTTYPE_CUSTOM
 *   The client should provide the client identifier (and its length in
 *   octets) explicitly. */
typedef enum 
{
  DHCP_CLIENT_ID_OPTTYPE_NONE        = 0,
  DHCP_CLIENT_ID_OPTTYPE_HWADDR      = 1,
  DHCP_CLIENT_ID_OPTTYPE_CUSTOM      = 2,
  DHCP_CLIENT_ID_OPTTYPE_FORCE_32BIT = 0x7FFFFFFF
} dhcp_client_id_option_type_enum;

/* Status callback enums for the DHCP client */
typedef enum 
{
  DHCP_CLIENT_IP_ASSIGNED    = 0,   /* an IP address has been obtained */
  DHCP_CLIENT_IP_RELEASED    = 1,   /* an IP address has been released */
  DHCP_CLIENT_IP_EXPIRED     = 2,   /* an IP address has been lost */
  DHCP_CLIENT_IP_FAIL        = 3,   /* Failed to obtain an IP address */
  DHCP_CLIENT_INIT_FAIL      = 4,   /* Failed to initialize properly */
  DHCP_CLIENT_PROVISIONED    = 5,   /* Received a REPLY to our INFORM 
                                        and provisioned successfully */
  DHCP_CLIENT_PROVISION_FAIL = 6,   /* INFORM sequence failed */
  DHCP_CLIENT_IP_DECLINED    = 7    /* client declined allocated IP
                                        address */
} dhcp_client_status_enum;

/* callback function pointer type */
typedef void (*dhcp_client_status_cb_type)
(
  void * userdata, 
  dhcp_client_status_enum status 
);

#endif /* DHCP_CLIENT_COMMON_H */

