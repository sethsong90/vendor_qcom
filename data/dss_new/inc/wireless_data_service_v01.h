#ifndef WDS_SERVICE_H
#define WDS_SERVICE_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        W I R E L E S S _ D A T A _ S E R V I C E _ V 0 1  . H

GENERAL DESCRIPTION
  This is the public header file which defines the wds service Data structures.

 Copyright (c) 2010 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential.

 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====* 
 *THIS IS AN AUTO GENERATED FILE. DO NOT ALTER IN ANY WAY 
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*
 * This header file defines the types and structures that were defined in 
 * cat. It contains the constant values defined, enums, structures,
 * messages, and service message IDs (in that order) Structures that were 
 * defined in the IDL as messages contain mandatory elements, optional 
 * elements, a combination of mandatory and optional elements (mandatory 
 * always come before optionals in the structure), or nothing (null message)
 *  
 * An optional element in a message is preceded by a uint8_t value that must be
 * set to true if the element is going to be included. When decoding a received
 * message, the uint8_t values will be set to true or false by the decode
 * routine, and should be checked before accessing the values that they
 * correspond to. 
 *  
 * Variable sized arrays are defined as static sized arrays with an unsigned
 * integer (32 bit) preceding it that must be set to the number of elements
 * in the array that are valid. For Example:
 *  
 * uint32_t test_opaque_len;
 * uint8_t test_opaque[16];
 *  
 * If only 4 elements are added to test_opaque[] then test_opaque_len must be
 * set to 4 before sending the message.  When decoding, the _len value is set 
 * by the decode routine and should be checked so that the correct number of 
 * elements in the array will be accessed. 
 */

/* This file was generated with Tool version 02.01 
   It was generated on: Fri Oct 15 2010
   From IDL File: */

#include <stdint.h>
#include "qmi_idl_lib.h"
#include "common_v01.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Version Number of the IDL used to generate this file */
#define WDS_V01_IDL_MAJOR_VERS 01
#define WDS_V01_IDL_MINOR_VERS 10
#define WDS_V01_IDL_TOOL_VERS 02

/* Const Definitions */

#define WDS_APN_NAME_MAX_V01 255
#define WDS_USER_NAME_MAX_V01 255
#define WDS_PASSWORD_MAX_V01 255
#define WDS_PROFILE_NAME_MAX_V01 255

/*
 * wds_reset_req_msg is empty
 * typedef struct {
 * }wds_reset_req_msg_v01;
 */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type. */
}wds_reset_resp_msg_v01;	/* Message */

typedef struct {
  uint8_t stats_period;	/*  Period between transfer statistics reports:
       - 0 - Do not report
       - Other - Period between reports (seconds)
   */

  uint32_t stats_mask;	/*  Requested statistic bit mask:
       - 0x00000001 - Tx packets ok
       - 0x00000002 - Rx packets ok
       - 0x00000004 - Tx packet errors
       - 0x00000008 - Rx packet errors
       - 0x00000010 - Tx overflows
       - 0x00000020 - Rx overflows

       Each bit set will cause the corresponding optional TLV to be
       sent in the QMI_WDS_EVENT_REPORT_IND.

       All unlisted bits are reserved for future use and must be set
       to zero.
   */
}wds_statistics_indicator_type_v01;	/* Type */

typedef struct {
  /* Optional */
  /*  Current channel rate indicator */
  uint8_t report_channel_rate_valid;	/* Must be set to true if report_channel_rate is being passed */
  uint8_t report_channel_rate;	/*  - 0 - Do not report
       - 1 - Report channel rate when it changes
   */

  /* Optional */
  /*  Transfer statistics indicator */
  uint8_t report_stats_valid;	/* Must be set to true if report_stats is being passed */
  wds_statistics_indicator_type_v01 report_stats;

  /* Optional */
  /*  Current data bearer technology indicator
 (This TLV is deprecated from QMI WDS version 1.4) */
  uint8_t report_data_bearer_tech_valid;	/* Must be set to true if report_data_bearer_tech is being passed */
  uint8_t report_data_bearer_tech;	/*  - 0 - Do not report
       - 1 - Report Report radio interface used for data transfer when it
         changes
   */

  /* Optional */
  /*  Dormancy Status indicator */
  uint8_t report_dormancy_status_valid;	/* Must be set to true if report_dormancy_status is being passed */
  uint8_t report_dormancy_status;	/*  - 0 - Do not report
       - 1 - Report traffic channel state of interface used for data
         connection
   */

  /* Optional */
  /*  Current data bearer technology indicator */
  uint8_t report_current_data_bearer_tech_valid;	/* Must be set to true if report_current_data_bearer_tech is being passed */
  uint8_t report_current_data_bearer_tech;	/*  - 0 - Do not report
       - 1 - Report current data bearer technology when it changes
   */
}wds_set_event_report_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type. */
}wds_set_event_report_resp_msg_v01;	/* Message */

typedef struct {
  uint32_t current_channel_tx_rate;	/*  Max channel Tx rate in bits per second */

  uint32_t current_channel_rx_rate;	/*  Max channel Rx rate in bits per second */
}wds_channel_rate_type_v01;	/* Type */

typedef struct {
  uint8_t current_nw;	/*  Current network type of data bearer:
       - 0x00 - UNKNOWN
       - 0x01 - CDMA
       - 0x02 - UMTS
   */

  uint32_t rat_mask;	/*  Radio access technology mask to indicate the type of technology

       RAT mask value of zero indicates that this field is ignored.
       - 0x00 - DON'T_CARE

       CDMA RAT mask:
       - 0x01 - CDMA_1X
       - 0x02 - EVDO_REV0
       - 0x04 - EVDO_REVA

       UMTS RAT mask:
       - 0x01 - WCDMA
       - 0x02 - GPRS
       - 0x04 - HSDPA
       - 0x08 - HSUPA
       - 0x10 - EDGE
   */

  uint32_t so_mask;	/*  Service option make to indicate the service option or type of
       application

       SO mask value of zero indicates that this field is ignored:
       - 0x00 - DON'T_CARE

       CDMA 1x SO mask:
       - 0x01 - CDMA_1X_IS95
       - 0x02 - CDMA_1X_IS2000
       - 0x04 - CDMA_1X_IS2000_REL_A

       CDMA EV-DO Rev A SO mask:
       - 0x01 - EVDO_REVA_DPA
       - 0x02 - EVDO_REVA_MFPA
       - 0x04 - EVDO_REVA_EMPA
   */
}wds_current_bearer_tech_type_v01;	/* Type */

typedef struct {
  /* Optional */
  /*  Tx packets ok */
  uint8_t tx_ok_count_valid;	/* Must be set to true if tx_ok_count is being passed */
  uint32_t tx_ok_count;	/*  Number of packets transmitted without error */

  /* Optional */
  /*  Rx packets ok */
  uint8_t rx_ok_count_valid;	/* Must be set to true if rx_ok_count is being passed */
  uint32_t rx_ok_count;	/*  Number of packets received without error */

  /* Optional */
  /*  Tx packet errors */
  uint8_t tx_err_count_valid;	/* Must be set to true if tx_err_count is being passed */
  uint32_t tx_err_count;	/*  Number of outgoing packets with framing 
 errors */

  /* Optional */
  /*  Rx packet errors */
  uint8_t rx_err_count_valid;	/* Must be set to true if rx_err_count is being passed */
  uint32_t rx_err_count;	/*  Number of incoming packets with framing
 errors */

  /* Optional */
  /*  Tx overflows */
  uint8_t tx_ofl_count_valid;	/* Must be set to true if tx_ofl_count is being passed */
  uint32_t tx_ofl_count;	/*  Number of packets dropped because Tx
 buffer overflowed (out of memory) */

  /* Optional */
  /*  Rx overflows */
  uint8_t rx_ofl_count_valid;	/* Must be set to true if rx_ofl_count is being passed */
  uint32_t rx_ofl_count;	/*  Number of packets dropped because Rx
 buffer overflowed (out of memory) */

  /* Optional */
  /*  Channel rate */
  uint8_t channel_rate_valid;	/* Must be set to true if channel_rate is being passed */
  wds_channel_rate_type_v01 channel_rate;

  /* Optional */
  /*  Data bearer technology
 This TLV is deprecated from QMI WDS version 1.4 */
  uint8_t data_bearer_tech_valid;	/* Must be set to true if data_bearer_tech is being passed */
  uint8_t data_bearer_tech;	/*  - 0x01 - cdma2000(R) 1X
       - 0x02 - cdma2000 HRPD (1xEV-DO)
       - 0x03 - GSM
       - 0x04 - UMTS
   */

  /* Optional */
  /*  Dormancy status */
  uint8_t dormancy_status_valid;	/* Must be set to true if dormancy_status is being passed */
  uint8_t dormancy_status;	/*  - 0x01 - Traffic channel dormant
       - 0x02 - Traffic channel active
   */

  /* Optional */
  /*  Current data bearer technology */
  uint8_t current_bearer_tech_valid;	/* Must be set to true if current_bearer_tech is being passed */
  wds_current_bearer_tech_type_v01 current_bearer_tech;
}wds_event_report_ind_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  TX_ID */
  uint16_t tx_id;	/*  Transaction ID of the request that is to be aborted */
}wds_abort_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type. */
}wds_abort_resp_msg_v01;	/* Message */

typedef struct {
  /* Optional */
  /*  Primary DNS address preference */
  uint8_t primary_DNS_IPv4_address_preference_valid;	/* Must be set to true if primary_DNS_IPv4_address_preference is being passed */
  uint32_t primary_DNS_IPv4_address_preference;	/*  Value may be used as a preference during negotiation with network;
       if not specified, wireless device will attempt to obtain DNS
       address automatically from network; negotiated value is provided
       to host via DHCP.
   */

  /* Optional */
  /*  Secondary DNS address preference */
  uint8_t secondary_DNS_IPv4_address_preference_valid;	/* Must be set to true if secondary_DNS_IPv4_address_preference is being passed */
  uint32_t secondary_DNS_IPv4_address_preference;	/*  Value may be used as a preference during negotiation with network;
       if not specified, wireless device will attempt to obtain DNS
       address automatically from network; negotiated value is provided
       to the host via DHCP.
   */

  /* Optional */
  /*  Primary NetBIOS Name Server (NBNS) address preference */
  uint8_t primary_nbns_address_pref_valid;	/* Must be set to true if primary_nbns_address_pref is being passed */
  uint32_t primary_nbns_address_pref;	/*  Primary NBNS address - Specified IPv4 address is requested as the
       primary NBNS server during data session establishment; if not
       provided, primary NBNS server address is obtained automatically
       from the network; result of negotiation (the assigned address) is
       provided to the host via DHCP
   */

  /* Optional */
  /*  Secondary NBNS address preference */
  uint8_t secondary_nbns_address_pref_valid;	/* Must be set to true if secondary_nbns_address_pref is being passed */
  uint32_t secondary_nbns_address_pref;	/*  Secondary NetBIOS name server address - Specified IPv4 address is
       requested as the secondary NBNS server during data session
       establishment; if not provided, the secondary NBNS server address
       is obtained automatically from the network; result of negotiation
       (the assigned address) is provided to the host via DHCP.
   */

  /* Optional */
  /*  Context Access Point Node (APN) name */
  uint8_t apn_name_valid;	/* Must be set to true if apn_name is being passed */
  char apn_name[WDS_APN_NAME_MAX_V01 + 1];	/*  Access point name - A string parameter that is a logical name used to
       select GGSN and external packet data network.

       If value is NULL or omitted, then the subscription default value
       will be requested.

       QMI_ERR_ARG_TOO_LONG will be returned if APN name is too long.

       This TLV is ignored if the 3GPP configured profile TLV is
       present, i.e., APN name cannot be overridden
   */

  /* Optional */
  /*  IP address preference */
  uint8_t ipv4_address_pref_valid;	/* Must be set to true if ipv4_address_pref is being passed */
  uint32_t ipv4_address_pref;	/*  Preferred IPv4 address to be assigned to the TE - Actual assigned
       address is negotiated with the network and may differ from this value;
       if not specified, the IPv4 Address is obtained automatically from the
       network; assigned value is provided to the host via DHCP.
   */

  /* Optional */
  /*  Authentication preference */
  uint8_t authentication_preference_valid;	/* Must be set to true if authentication_preference is being passed */
  uint8_t authentication_preference;	/*  A bit map that indicates the authentication algorithm preference

       Bit 0 - PAP preference
       - 0 - PAP is never performed
       - 1 - PAP may be performed

       Bit 1 - CHAP preference
       - 0 - CHAP is never performed
       - 1 - CHAP may be performed

       All other bits are reserved and will be ignored even if they are set
       in the request.

       If more than one bit is set, then the device decides which
       authentication procedure is performed while setting up the data
       session. For example, the device may have a policy to select
       the most secure authentication mechanism.
   */

  /* Optional */
  /*  Username */
  uint8_t username_valid;	/* Must be set to true if username is being passed */
  char username[WDS_USER_NAME_MAX_V01 + 1];	/*  Username to be used during data network authentication - 
       QMI_ERR_ARG_TOO_LONG will be returned if the storage on the wireless
       device is insufficient in size to hold the value.
   */

  /* Optional */
  /*  Password */
  uint8_t password_valid;	/* Must be set to true if password is being passed */
  char password[WDS_PASSWORD_MAX_V01 + 1];	/*  Password to be used during data network authentication -
       QMI_ERR_ARG_TOO_LONG will be returned if the storage on the wireless
       device is insufficient in size to hold the value.
   */

  /* Optional */
  /*  IP family preference */
  uint8_t ip_family_preference_valid;	/* Must be set to true if ip_family_preference is being passed */
  uint8_t ip_family_preference;	/*  IP family preference

       - 0x04 - IPv4
       - 0x06 - IPv6
       - 0x08 - UNSPEC

       If this TLV is absent, then the device will try to bring up a call
       on default IP preference (currently IPv4, so as to maintain current
       behavioral backward compatability)
   */

  /* Optional */
  /*  Technology preference */
  uint8_t technology_preference_valid;	/* Must be set to true if technology_preference is being passed */
  uint8_t technology_preference;	/*  Bitmap that indicates the technology preference; single connection
       is attempted using specified technology preferences:

         - Bit 0 - 3GPP
         - Bit 1 - 3GPP2

       All other bits are reserved and will be ignored even if they are set
       in the request; if a single value of the technology preference bit
       mask is set, then the device attempts to use that technology; if two
       or more bits in the technology preference bit mask are set, then the
       device determines which technology to use from those specified; if
       this TLV is absent, then the device assumes all supported technologies
       are acceptable.
    */

  /* Optional */
  /*  3GPP configured profile identifier */
  uint8_t profile_index_valid;	/* Must be set to true if profile_index is being passed */
  uint8_t profile_index;	/*  Index of the configured profile on which data call parameters
       are based (other TLVs present will override the profile settings);
       if this TLV is not present, then data call parameters are based on
       device default settings for each parameter.
   */

  /* Optional */
  /*  3GPP2 configured profile identifier */
  uint8_t Threegpp2_profile_index_valid;	/* Must be set to true if Threegpp2_profile_index is being passed */
  uint8_t Threegpp2_profile_index;	/*  Index of the configured profile on which data call parameters
       are based (other TLVs present will override the profile settings);
       if this TLV is not present, then data call parameters are based on
       device default settings for each parameter.
   */

  /* Optional */
  /*  Extended technology preference */
  uint8_t ext_technology_preference_valid;	/* Must be set to true if ext_technology_preference is being passed */
  uint16_t ext_technology_preference;	/*  Technology preference to be used while attempting a packet data
       connection.

       - CDMA - 0x8001
       - UMTS - 0x8004
   */

  /* Optional */
  /*  Call type identifier */
  uint8_t call_type_valid;	/* Must be set to true if call_type is being passed */
  uint8_t call_type;	/*  Type of call to be originated

       - 0x00 - LAPTOP CALL
       - 0x01 - EMBEDDED CALL

       If this TLV is not present, then by default the call is considered
       to be laptop call.
   */
}wds_start_network_interface_req_msg_v01;	/* Message */

typedef struct {
  uint16_t call_end_reason_type;	/*  Call end reason type; see Appendix A for the definition of these
       values.
   */

  uint16_t call_end_reason;	/*  Reason the call ended (verbose); see Appendix A for the definition
       of these values.
   */
}wds_verbose_call_end_reason_type_v01;	/* Type */

typedef struct {
  /* Mandatory */
  /*  Packet data handle */
  uint32_t pkt_data_handle;	/*  Handle identifying the call instance providing packet service.

       The packet data handle must be retained by control point and
       specified in  STOP_NETWORK_INTERFACE message issued when control
       point is finished with the packet data session.
   */

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type. */

  /* Optional */
  /*  Call end reason */
  uint8_t call_end_reason_valid;	/* Must be set to true if call_end_reason is being passed */
  uint16_t call_end_reason;	/*  Reason the call ended; see Appendix A for the definition of
       these values
   */

  /* Optional */
  /*  Verbose call end reason */
  uint8_t verbose_call_end_reason_valid;	/* Must be set to true if verbose_call_end_reason is being passed */
  wds_verbose_call_end_reason_type_v01 verbose_call_end_reason;
}wds_start_network_interface_resp_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Packet data handle */
  uint32_t pkt_data_handle;	/*  Handle identifying the call instance to unbind the control point from

       This value must be the handle previously returned by
       QMI_WDS_START_NETWORK_INTERFACE_REQ.
    */
}wds_stop_network_interface_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type. */
}wds_stop_network_interface_resp_msg_v01;	/* Message */

/*
 * wds_get_pkt_srvc_status_req_msg is empty
 * typedef struct {
 * }wds_get_pkt_srvc_status_req_msg_v01;
 */

typedef struct {
  /* Mandatory */
  /*  Connection status */
  uint8_t connection_status;	/*  Current link status:
        - 0x01 - QMI_WDS_PKT_DATA_DISCONNECTED
        - 0x02 - QMI_WDS_PKT_DATA_CONNECTED
        - 0x03 - QMI_WDS_PKT_DATA_SUSPENDED
        - 0x04 - QMI_WDS_PKT_DATA_AUTHENTICATING
    */

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type. */
}wds_get_pkt_srvc_status_resp_msg_v01;	/* Message */

typedef struct {
  /*  Connection status */
  uint8_t connection_status;	/*  Current link status:
       - 0x01 - QMI_WDS_PKT_DATA_DISCONNECTED
       - 0x02 - QMI_WDS_PKT_DATA_CONNECTED
       - 0x03 - QMI_WDS_PKT_DATA_SUSPENDED
       - 0x04 - QMI_WDS_PKT_DATA_AUTHENTICATING
    */

  /*  Reconfiguration required */
  uint8_t reconfiguration_required;	/*  Indicates if the network interface on the host needs to be
       reconfigured:
       - 0x00 - No need to reconfigure
       - 0x01 - Reconfiguration required
    */
}wds_packet_service_status_type_v01;	/* Type */

typedef struct {
  /* Mandatory */
  /*  Packet service status */
  wds_packet_service_status_type_v01 status;

  /* Optional */
  /*  Call end reason */
  uint8_t call_end_reason_valid;	/* Must be set to true if call_end_reason is being passed */
  uint16_t call_end_reason;	/*  Reason the call ended; see Appendix A for the definition of these
       values
   */

  /* Optional */
  /*  Verbose call end reason */
  uint8_t verbose_call_end_reason_valid;	/* Must be set to true if verbose_call_end_reason is being passed */
  wds_verbose_call_end_reason_type_v01 verbose_call_end_reason;
}wds_pkt_srvc_status_ind_msg_v01;	/* Message */

/*
 * wds_get_current_channel_rate_req_msg is empty
 * typedef struct {
 * }wds_get_current_channel_rate_req_msg_v01;
 */

typedef struct {
  uint32_t current_channel_tx_rate;	/*  Instantaneous channel Tx rate in bits per second */

  uint32_t current_channel_rx_rate;	/*  Instantaneous channel Rx rate in bits per second */

  uint32_t max_channel_tx_rate;	/*  Max Tx rate that may be assigned to the device by the serving
       system in bits per second
   */

  uint32_t max_channel_rx_rate;	/*  Max Rx rate that may be assigned to the device by the serving
     system in bits per second
   */
}wds_current_channel_rate_type_v01;	/* Type */

typedef struct {
  /* Mandatory */
  /*  Channel rate */
  wds_current_channel_rate_type_v01 rates;

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type. */
}wds_get_current_channel_rate_resp_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Packet stats mask */
  uint32_t stat_mask;	/*  Requested statistic bit mask:
       - 0x00000001 - Tx packets OK
       - 0x00000002 - Rx packets OK
       - 0x00000004 - Tx packet errors
       - 0x00000008 - Rx packet errors
       - 0x00000010 - Tx overflows
       - 0x00000020 - Rx overflows

       All unlisted bits are reserved for future use and must be set
       to zero unless recognized by issuer.
   */
}wds_get_pkt_statistics_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type. */

  /* Optional */
  /*  Tx packets ok */
  uint8_t tx_ok_count_valid;	/* Must be set to true if tx_ok_count is being passed */
  uint32_t tx_ok_count;	/*  Number of packets transmitted without error */

  /* Optional */
  /*  Rx packets ok */
  uint8_t rx_ok_count_valid;	/* Must be set to true if rx_ok_count is being passed */
  uint32_t rx_ok_count;	/*  Number of packets received without error */

  /* Optional */
  /*  Tx packets errors */
  uint8_t tx_err_count_valid;	/* Must be set to true if tx_err_count is being passed */
  uint32_t tx_err_count;	/*  Number of outgoing packets with framing errors */

  /* Optional */
  /*  Rx packet errors */
  uint8_t rx_err_count_valid;	/* Must be set to true if rx_err_count is being passed */
  uint32_t rx_err_count;	/*  Number of incoming packets with framing errors */

  /* Optional */
  /*  Tx overflows */
  uint8_t tx_ofl_count_valid;	/* Must be set to true if tx_ofl_count is being passed */
  uint32_t tx_ofl_count;	/*  Number of packets dropped because Tx buffer overflowed (out of memory) */

  /* Optional */
  /*  Rx overflows */
  uint8_t rx_ofl_count_valid;	/* Must be set to true if rx_ofl_count is being passed */
  uint32_t rx_ofl_count;	/*  Number of packets dropped because Rx buffer overflowed (out of memory) */
}wds_get_pkt_statistics_resp_msg_v01;	/* Message */

/*
 * wds_go_dormant_req_msg is empty
 * typedef struct {
 * }wds_go_dormant_req_msg_v01;
 */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type. */
}wds_go_dormant_resp_msg_v01;	/* Message */

/*
 * wds_go_active_req_msg is empty
 * typedef struct {
 * }wds_go_active_req_msg_v01;
 */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type. */
}wds_go_active_resp_msg_v01;	/* Message */

typedef struct {
  uint8_t traffic_class;	/*  Traffic class:
       - 0x00 - Subscribed
       - 0x01 - Conversational
       - 0x02 - Streaming
       - 0x03 - Interactive
       - 0x04 - Background
    */

  uint32_t max_uplink_bitrate;	/*  Maximum uplink bit rate in bits/second */

  uint32_t max_downlink_bitrate;	/*  Maximum downlink bit rate in bits/second */

  uint32_t guaranteed_uplink_bitrate;	/*  Guaranteed uplink bit rate in bits/second */

  uint32_t guaranteed_downlink_bitrate;	/*  Guaranteed downlink bit rate in bits/second */

  uint8_t qos_delivery_order;	/*  QoS delivery order:
       - 0x00 - Subscribe
       - 0x01 - Delivery order on
       - 0x02 - Delivery order off
    */

  uint32_t max_sdu_size;	/*  Maximum SDU size */

  uint8_t sdu_error_ratio;	/*  SDU error ratio

       Target value for the fraction of SDUs lost or detected as erroneous:
       - 0x00 - Subscribe
       - 0x01 - 1x10-2
       - 0x02 - 7x10-3
       - 0x03 - 1x10-3
       - 0x04 - 1x10-4
       - 0x05 - 1x10-5
       - 0x06 - 1x10-6
       - 0x07 - 1x10-1
    */

  uint8_t residual_ber_ratio;	/*  Residual bit error ratio

       Target value for the undetected bit error ratio in the delivered SDUs:
        - 0x00 - Subscribe
        - 0x01 - 5x10-2
        - 0x02 - 1x10-2
        - 0x03 - 5x10-3
        - 0x04 - 4x10-3
        - 0x05 - 1x10-3
        - 0x06 - 1x10-4
        - 0x07 - 1x10-5
        - 0x08 - 1x10-6
        - 0x09 - 6x10-8
    */

  uint8_t delivery_erroneous_SDUs;	/*  Delivery of erroneous SDUs

       Indicates whether SDUs detected as erroneous shall be delivered or not:
       - 0x00 - Subscribe
       - 0x01 - No detection
       - 0x02 - Erroneous SDU is delivered
       -0x03 - Erroneous SDU is not delivered
    */

  uint32_t transfer_delay;	/*  Transfer delay (ms)

       Indicates the targeted time between a request to transfer a SDU
       at one SAP to its delivery at the other SAP, in
       milliseconds. If the parameter is set to 0, the subscribed
       value will be requested.
    */

  uint32_t traffic_handling_priority;	/*  Traffic handling priority

       Specifies the relative importance for handling of all SDUs
       belonging to the UMTS bearer compared to the SDUs of other
       bearers. If the parameter is set to 0, the subscribed value
       will be requested.
    */
}wds_umts_qos_type_v01;	/* Type */

typedef struct {
  wds_umts_qos_type_v01 umts_qos;

  uint8_t sig_ind;	/*  Signaling Indication flag:
         - TRUE - Signaling indication ON
         - FALSE - SIgnaling indication OFF
   */
}wds_umts_qos_with_sig_ind_type_v01;	/* Type */

typedef struct {
  uint32_t precedence_class;	/*  Precedence class [Q3] */

  uint32_t delay_class;	/*  Delay class [Q3] */

  uint32_t reliability_class;	/*  Reliability class [Q3] */

  uint32_t peak_throughput_class;	/*  Peak throughput class [Q3] */

  uint32_t mean_throughput_class;	/*  Mean throughput class [Q3] */
}wds_gprs_qos_type_v01;	/* Type */

typedef struct {
  uint8_t filter_id;	/*  Filter identifier */

  uint8_t eval_id;	/*  Evaluation precedence index */

  uint8_t ip_version;	/*  IP version number:
         - 4 - IPv4
         - 5 - IPv6
   */

  uint8_t source_ip[16];	/*  - IPv4 - Fill the first 4 bytes
       - IPv6 - Fill all the 16 bytes
   */

  uint8_t source_ip_mask;	/*  Mask value for the source address */

  uint8_t next_header;	/*  Next header/protocol value */

  uint16_t dest_port_range_start;	/*  Start vaue fo the destination port range */

  uint16_t dest_port_range_end;	/*  End vaue fo the destination port range */

  uint16_t src_port_range_start;	/*  Start vaue fo the source port range */

  uint16_t src_port_range_end;	/*  End vaue fo the source port range */

  uint32_t ipsec_spi;	/*  IPSEC security parameter index */

  uint16_t tos_mask;	/*  TOS mask (Traffic class for IPv6) */

  uint32_t flow_label;	/*  Flow label */
}wds_tft_id_param_type_v01;	/* Type */

typedef struct {
  uint8_t qci;	/*  For LTE, requested QOS must be specified using the QOS Class
       Identifier (QOS) values:
         - QCI value 0 - Requests the network to assign the appropriate QCI
           value
         - QCI values 1-4 - Associated with guranteed bit rates
         - QCI values 5-9 - Associated with nonguranteed bit rates, the values
           specified as guranteed and amximum bit rates are ignored
   */

  uint32_t g_dl_bit_rate;	/*  Guranteed DL bit rate */

  uint32_t max_dl_bit_rate;	/*  Maximum DL bit rate */

  uint32_t g_ul_bit_rate;	/*  Guranteed UL bit rate */

  uint32_t max_ul_bit_rate;	/*  Maximum UL bit rate */
}wds_3gpp_lte_qos_params_v01;	/* Type */

typedef struct {
  /* Mandatory */
  /*  Profile type */
  uint8_t profile_type;	/*  Identifies the technology type of the profile:
       - 0x00 - 3GPP
       Note: Creation of 3GPP2 profiles is not supported
    */

  /* Optional */
  /*  Profile name */
  uint8_t profile_name_valid;	/* Must be set to true if profile_name is being passed */
  char profile_name[WDS_PROFILE_NAME_MAX_V01 + 1];	/*  One or more bytes describing the profile. The description may
       be a user-defined name for the profile.

       QMI_ERR_ARG_TOO_LONG will be returned if the profile_name is too long
    */

  /* Optional */
  /*  PDP type */
  uint8_t pdp_type_valid;	/* Must be set to true if pdp_type is being passed */
  uint8_t pdp_type;	/*  Packet Data Protocol (PDP) type specifies the type of data payload
       exchanged over the air link when the packet data session is
       established with this profile:
       - 0x00 - PDP-IP (IPv4)
    */

  /* Optional */
  /*  PDP header compression type */
  uint8_t pdp_hdr_compression_type_valid;	/* Must be set to true if pdp_hdr_compression_type is being passed */
  uint8_t pdp_hdr_compression_type;	/*  PDP header compression type to use:
       0 - PDP header compression is OFF
       1 - Manufacturer preferred compression
       2 - PDP header compression based on RFC 1144
       3 - PDP header compression based on RFC 2507
       4 - PDP header compression based on RFC 3095
   */

  /* Optional */
  /*  PDP data compression type to use */
  uint8_t pdp_data_compression_type_valid;	/* Must be set to true if pdp_data_compression_type is being passed */
  uint8_t pdp_data_compression_type;	/*  PDP data compression type to use:
       0 - PDP data compression is OFF
       1 - Manufacturer preferred compression
       2 - V.42BIS data compresion
       3 - V.44 data compresion
   */

  /* Optional */
  /*  Context Access Point Node (APN) name */
  uint8_t apn_name_valid;	/* Must be set to true if apn_name is being passed */
  char apn_name[WDS_APN_NAME_MAX_V01 + 1];	/*  Access point name - A string parameter that is a logical name
       used to select the GGSN and the external packet data network.

       If the value is NULL or omitted, then the subscription default
       value will be requested.

       QMI_ERR_ARG_TOO_LONG will be returned if the APN name is too long.
   */

  /* Optional */
  /*  Primary DNS address preference */
  uint8_t primary_DNS_IPv4_address_preference_valid;	/* Must be set to true if primary_DNS_IPv4_address_preference is being passed */
  uint32_t primary_DNS_IPv4_address_preference;	/*  The value may be used as a preference during negotiation with the
       network. If not specified, the wireless device will attempt to obtain
       the DNS address automatically from the network. The negotiated value
       is provided to the host via DHCP.
   */

  /* Optional */
  /*  Secondary DNS address preference */
  uint8_t secondary_DNS_IPv4_address_preference_valid;	/* Must be set to true if secondary_DNS_IPv4_address_preference is being passed */
  uint32_t secondary_DNS_IPv4_address_preference;	/*  The value may be used as a preference during negotiation with the
       network. If not specified, the wireless device will attempt to obtain
       the DNS address automatically from the network. The negotiated value
       is provided to the host via DHCP.
   */

  /* Optional */
  /*  UMTS requested QoS */
  uint8_t umts_requested_qos_valid;	/* Must be set to true if umts_requested_qos is being passed */
  wds_umts_qos_type_v01 umts_requested_qos;

  /* Optional */
  /*  UMTS minimum QoS */
  uint8_t umts_minimum_qos_valid;	/* Must be set to true if umts_minimum_qos is being passed */
  wds_umts_qos_type_v01 umts_minimum_qos;

  /* Optional */
  /*  GPRS requested QoS */
  uint8_t gprs_requested_qos_valid;	/* Must be set to true if gprs_requested_qos is being passed */
  wds_gprs_qos_type_v01 gprs_requested_qos;

  /* Optional */
  /*  GRPS minimum Qos */
  uint8_t gprs_minimum_qos_valid;	/* Must be set to true if gprs_minimum_qos is being passed */
  wds_gprs_qos_type_v01 gprs_minimum_qos;

  /* Optional */
  /*  Username */
  uint8_t username_valid;	/* Must be set to true if username is being passed */
  char username[WDS_USER_NAME_MAX_V01 + 1];	/*  Username to be used during data network authentication

       QMI_ERR_ARG_TOO_LONG will be returned if the storage on the wireless
       device is insufficient in size to hold the value.
   */

  /* Optional */
  /*  Password */
  uint8_t password_valid;	/* Must be set to true if password is being passed */
  char password[WDS_PASSWORD_MAX_V01 + 1];	/*  Password to be used during data network authentication
       QMI_ERR_ARG_TOO_LONG will be returned if the storage on the wireless
       device is insufficient in size to hold the value.
   */

  /* Optional */
  /*  Authentication preference */
  uint8_t authentication_preference_valid;	/* Must be set to true if authentication_preference is being passed */
  uint8_t authentication_preference;	/*  A bit map that indicates the authentication algorithm preference

       Bit 0 - PAP preference
       - 0 - PAP is never performed
       - 1 - PAP may be performed

       Bit 1 - CHAP preference
       - 0 - CHAP is never performed
       - 1 - CHAP may be performed

       All other bits are reserved and must be set to 0.

       If more than one bit is set, then the device decides which
       authentication procedure is performed while setting up the data
       session. For example, the device may have a policy to select
       the most secure authentication mechanism.
   */

  /* Optional */
  /*  IPv4 address preference */
  uint8_t ipv4_address_preference_valid;	/* Must be set to true if ipv4_address_preference is being passed */
  uint32_t ipv4_address_preference;	/*  Preferred IPv4 address to be assigned to the TE

       The actual assigned address is negotiated with the network and
       may differ from this value. If not specified, the IPv4 Address
       is obtained automatically from the network. The assigned value
       is provided to the host via DHCP.
   */

  /* Optional */
  /*  P-CSCF address using PCO Flag */
  uint8_t pcscf_addr_using_pco_valid;	/* Must be set to true if pcscf_addr_using_pco is being passed */
  uint8_t pcscf_addr_using_pco;	/* 
     - 1 (TRUE) implies Request PCSCF address using PCO
     - 0 (FALSE) implies do not request.

     By default this value is 0.
   */

  /* Optional */
  /*  PDP access control flag */
  uint8_t pdp_access_control_flag_valid;	/* Must be set to true if pdp_access_control_flag is being passed */
  uint8_t pdp_access_control_flag;	/*  PDP access control flag:
       0 - PDP access control none
       1 - PDP access control reject
       2 - PDP access control permission
   */

  /* Optional */
  /*  P-CSCF address using DHCP */
  uint8_t pcsf_addr_using_dhcp_valid;	/* Must be set to true if pcsf_addr_using_dhcp is being passed */
  uint8_t pcsf_addr_using_dhcp;	/*  1 (TRUE) implies request PCSCF address using DHCP
       0 (FALSE) implies do not request
       By default, value is 0
   */

  /* Optional */
  /*  IM CN flag */
  uint8_t im_cn_flag_valid;	/* Must be set to true if im_cn_flag is being passed */
  uint8_t im_cn_flag;	/*  - 1 (TRUE) implies request IM CN flag for this profile
       - 0 (FALSE) implies do not request IM CN flag for this profile
   */

  /* Optional */
  /*  Traffic Flow Template (TFT) ID1 parameters */
  uint8_t tft_id1_params_valid;	/* Must be set to true if tft_id1_params is being passed */
  wds_tft_id_param_type_v01 tft_id1_params;

  /* Optional */
  /*  TFT ID2 parameters */
  uint8_t tft_id2_params_valid;	/* Must be set to true if tft_id2_params is being passed */
  wds_tft_id_param_type_v01 tft_id2_params;

  /* Optional */
  /*  PDP context number */
  uint8_t pdp_context_valid;	/* Must be set to true if pdp_context is being passed */
  uint8_t pdp_context;	/*  PDP context number */

  /* Optional */
  /*  PDP context secondary flag */
  uint8_t secondary_flag_valid;	/* Must be set to true if secondary_flag is being passed */
  uint8_t secondary_flag;	/*  - 1 (TRUE) implies this is secondary profile
       - 0 (FALSE) implies this is not secondary profile
   */

  /* Optional */
  /*  PDP context primary ID */
  uint8_t primary_id_valid;	/* Must be set to true if primary_id is being passed */
  uint8_t primary_id;	/*  PDP context number primary ID */

  /* Optional */
  /*  IPv6 address preference */
  uint8_t ipv6_address_preference_valid;	/* Must be set to true if ipv6_address_preference is being passed */
  uint8_t ipv6_address_preference[16];	/*  Preferred IPv6 address to be assigned to the TE; actual assigned
       address is negotiated with the network and may differ from this value;
       if not specified, the IPv6 address is obtaiend automatically from the
       network
   */

  /* Optional */
  /*  UMTS requested QoS with Signaling Indication Flag */
  uint8_t umts_requested_qos_with_sig_ind_valid;	/* Must be set to true if umts_requested_qos_with_sig_ind is being passed */
  wds_umts_qos_with_sig_ind_type_v01 umts_requested_qos_with_sig_ind;

  /* Optional */
  /*  UMTS minimum QoS with signalling indication */
  uint8_t umts_minimum_qos_with_sig_ind_valid;	/* Must be set to true if umts_minimum_qos_with_sig_ind is being passed */
  wds_umts_qos_with_sig_ind_type_v01 umts_minimum_qos_with_sig_ind;

  /* Optional */
  /*  Primary DNS IPv6 address preference */
  uint8_t primary_dns_ipv6_address_preference_valid;	/* Must be set to true if primary_dns_ipv6_address_preference is being passed */
  uint32_t primary_dns_ipv6_address_preference;	/*  The value may be used as a preference during negotiation with the
       network; if not specified, the wireless device will attempt to obtain
       the DNS address automatically from the network; the negotiated value
       is provided to the host via DHCP
   */

  /* Optional */
  /*  Secondary DNS IPv6 address preference */
  uint8_t secodnary_dns_ipv6_address_preference_valid;	/* Must be set to true if secodnary_dns_ipv6_address_preference is being passed */
  uint32_t secodnary_dns_ipv6_address_preference;	/*  The value may be used as a preference during negotiation with the
       network; if not specified, the wireless device will attempt to obtain
       the DNS address automatically from the network; the negotiated value
       is provided to the host via DHCP
   */

  /* Optional */
  /*  DHCP/NAS preference */
  uint8_t addr_allocation_preference_valid;	/* Must be set to true if addr_allocation_preference is being passed */
  uint8_t addr_allocation_preference;	/*  This enumerated value may be used to indicate the address allocation
       preference:
         - 0 - NAS signaling is used for address allocation
         - 1 - DHCP is used for address allocation
   */

  /* Optional */
  /*  3GPP LTE QoS parameters */
  uint8_t threegpp_lte_qos_params_valid;	/* Must be set to true if threegpp_lte_qos_params is being passed */
  wds_3gpp_lte_qos_params_v01 threegpp_lte_qos_params;
}wds_create_profile_req_msg_v01;	/* Message */

typedef struct {
  uint8_t profile_type;	/*  Identifies the type of profile that was created:
       - 0x00 - 3GPP
       - 0x01 - 3GPP2
    */

  uint8_t profile_index;	/*  Index identifying the profile that was created */
}wds_profile_identifier_type_v01;	/* Type */

typedef struct {
  /* Mandatory */
  /*  Profile identifier */
  wds_profile_identifier_type_v01 profile;

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type. */

  /* Optional */
  /*  Extended error code */
  uint8_t extended_error_code_valid;	/* Must be set to true if extended_error_code is being passed */
  uint16_t extended_error_code;	/*  Error code from the DS profile */
}wds_create_profile_resp_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Profile identifier */
  wds_profile_identifier_type_v01 profile;

  /* Optional */
  /*  Profile name */
  uint8_t profile_name_valid;	/* Must be set to true if profile_name is being passed */
  char profile_name[WDS_PROFILE_NAME_MAX_V01 + 1];	/*  One or more bytes describing the profile. The description may
       be a user-defined name for the profile.

       QMI_ERR_ARG_TOO_LONG will be returned if the profile_name is too long
    */

  /* Optional */
  /*  PDP type */
  uint8_t pdp_type_valid;	/* Must be set to true if pdp_type is being passed */
  uint8_t pdp_type;	/*  Packet Data Protocol (PDP) type specifies the type of data payload
       exchanged over the air link when the packet data session is
       established with this profile:
       - 0x00 - PDP-IP (IPv4)
    */

  /* Optional */
  /*  Context Access Point Node (APN) name */
  uint8_t apn_name_valid;	/* Must be set to true if apn_name is being passed */
  char apn_name[WDS_APN_NAME_MAX_V01 + 1];	/*  Access point name - A string parameter that is a logical name
       used to select the GGSN and the external packet data network.

       If the value is NULL or omitted, then the subscription default
       value will be requested.

       QMI_ERR_ARG_TOO_LONG will be returned if the APN name is too long.
   */

  /* Optional */
  /*  Primary DNS address preference */
  uint8_t primary_DNS_IPv4_address_preference_valid;	/* Must be set to true if primary_DNS_IPv4_address_preference is being passed */
  uint32_t primary_DNS_IPv4_address_preference;	/*  The value may be used as a preference during negotiation with the
       network. If not specified, the wireless device will attempt to obtain
       the DNS address automatically from the network. The negotiated value
       is provided to the host via DHCP.
   */

  /* Optional */
  /*  Secondary DNS address preference */
  uint8_t secondary_DNS_IPv4_address_preference_valid;	/* Must be set to true if secondary_DNS_IPv4_address_preference is being passed */
  uint32_t secondary_DNS_IPv4_address_preference;	/*  The value may be used as a preference during negotiation with the
       network. If not specified, the wireless device will attempt to obtain
       the DNS address automatically from the network. The negotiated value
       is provided to the host via DHCP.
   */

  /* Optional */
  /*  UMTS requested QoS */
  uint8_t umts_requested_qos_valid;	/* Must be set to true if umts_requested_qos is being passed */
  wds_umts_qos_type_v01 umts_requested_qos;

  /* Optional */
  /*  UMTS minimum QoS */
  uint8_t umts_minimum_qos_valid;	/* Must be set to true if umts_minimum_qos is being passed */
  wds_umts_qos_type_v01 umts_minimum_qos;

  /* Optional */
  /*  GPRS requested QoS */
  uint8_t gprs_requested_qos_valid;	/* Must be set to true if gprs_requested_qos is being passed */
  wds_gprs_qos_type_v01 gprs_requested_qos;

  /* Optional */
  /*  GRPS minimum Qos */
  uint8_t gprs_minimum_qos_valid;	/* Must be set to true if gprs_minimum_qos is being passed */
  wds_gprs_qos_type_v01 gprs_minimum_qos;

  /* Optional */
  /*  Username */
  uint8_t username_valid;	/* Must be set to true if username is being passed */
  char username[WDS_USER_NAME_MAX_V01 + 1];	/*  Username to be used during data network authentication

       QMI_ERR_ARG_TOO_LONG will be returned if the storage on the wireless
       device is insufficient in size to hold the value.
   */

  /* Optional */
  /*  Password */
  uint8_t password_valid;	/* Must be set to true if password is being passed */
  char password[WDS_PASSWORD_MAX_V01 + 1];	/*  Password to be used during data network authentication
       QMI_ERR_ARG_TOO_LONG will be returned if the storage on the wireless
       device is insufficient in size to hold the value.
   */

  /* Optional */
  /*  Authentication preference */
  uint8_t authentication_preference_valid;	/* Must be set to true if authentication_preference is being passed */
  uint8_t authentication_preference;	/*  A bit map that indicates the authentication algorithm preference

       Bit 0 - PAP preference
       - 0 - PAP is never performed
       - 1 - PAP may be performed

       Bit 1 - CHAP preference
       - 0 - CHAP is never performed
       - 1 - CHAP may be performed

       All other bits are reserved and must be set to 0.

       If more than one bit is set, then the device decides which
       authentication procedure is performed while setting up the data
       session. For example, the device may have a policy to select
       the most secure authentication mechanism.
   */

  /* Optional */
  /*  IPv4 address preference */
  uint8_t ipv4_address_preference_valid;	/* Must be set to true if ipv4_address_preference is being passed */
  uint32_t ipv4_address_preference;	/*  Preferred IPv4 address to be assigned to the TE

       The actual assigned address is negotiated with the network and
       may differ from this value. If not specified, the IPv4 Address
       is obtained automatically from the network. The assigned value
       is provided to the host via DHCP.
   */

  /* Optional */
  /*  P-CSCF address using PCO Flag */
  uint8_t pcscf_addr_using_pco_valid;	/* Must be set to true if pcscf_addr_using_pco is being passed */
  uint8_t pcscf_addr_using_pco;	/* 
     - 1 (TRUE) implies Request PCSCF address using PCO
     - 0 (FALSE) implies do not request.

     By default this value is 0.
   */
}wds_modify_profile_settings_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type. */

  /* Optional */
  /*  Extended error code */
  uint8_t extended_error_code_valid;	/* Must be set to true if extended_error_code is being passed */
  uint16_t extended_error_code;	/*  Error code from the DS profile */
}wds_modify_profile_settings_resp_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Profile identifier */
  wds_profile_identifier_type_v01 profile;	/*  Note: Deletion of 3GPP2 profiles is not supported. */
}wds_delete_profile_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type. */

  /* Optional */
  /*  Extended error code */
  uint8_t extended_error_code_valid;	/* Must be set to true if extended_error_code is being passed */
  uint16_t extended_error_code;	/*  Error code from the DS profile */
}wds_delete_profile_resp_msg_v01;	/* Message */

typedef struct {
  /* Optional */
  /*  Profile Type */
  uint8_t profile_type_valid;	/* Must be set to true if profile_type is being passed */
  uint8_t profile_type;	/*  Identifies the type of profile:
         - 0x00 - 3GPP
         - 0x01 - 3GPP2
   */
}wds_get_profile_list_req_msg_v01;	/* Message */

typedef struct {
  uint8_t profile_type;	/*  Identifies the type of profile:
       - 0x00 - 3GPP
       - 0x01 - 3GPP2
   */

  uint8_t profile_index;	/*  Profile number identifying the profile */

  char profile_name[WDS_PROFILE_NAME_MAX_V01 + 1];	/*  One or more bytes describing the profile. The description may
       be a user-defined name for the profile.
   */
}wds_profile_info_type_v01;	/* Type */

typedef struct {
  /* Mandatory */
  /*  Profile list */
  uint32_t profiles_len;	/* Must be set to # of elements in profiles */
  wds_profile_info_type_v01 profiles[10];

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type. */

  /* Optional */
  /*  Extended error code */
  uint8_t extended_error_code_valid;	/* Must be set to true if extended_error_code is being passed */
  uint16_t extended_error_code;	/*  Error code from the DS profile */
}wds_get_profile_list_resp_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Profile identifier */
  wds_profile_identifier_type_v01 profile;
}wds_get_profile_settings_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type. */

  /* Optional */
  /*  Profile name */
  uint8_t profile_name_valid;	/* Must be set to true if profile_name is being passed */
  char profile_name[WDS_PROFILE_NAME_MAX_V01 + 1];	/*  One or more bytes describing the profile. The description may
       be a user-defined name for the profile.

       QMI_ERR_ARG_TOO_LONG will be returned if the profile_name is too long
    */

  /* Optional */
  /*  PDP type */
  uint8_t pdp_type_valid;	/* Must be set to true if pdp_type is being passed */
  uint8_t pdp_type;	/*  Packet Data Protocol (PDP) type specifies the type of data payload
       exchanged over the air link when the packet data session is
       established with this profile:
       - 0x00 - PDP-IP (IPv4)
    */

  /* Optional */
  /*  Context Access Point Node (APN) name */
  uint8_t apn_name_valid;	/* Must be set to true if apn_name is being passed */
  char apn_name[WDS_APN_NAME_MAX_V01 + 1];	/*  Access point name - A string parameter that is a logical name
       used to select the GGSN and the external packet data network.

       If the value is NULL or omitted, then the subscription default
       value will be requested.

       QMI_ERR_ARG_TOO_LONG will be returned if the APN name is too long.
   */

  /* Optional */
  /*  Primary DNS address preference */
  uint8_t primary_DNS_IPv4_address_preference_valid;	/* Must be set to true if primary_DNS_IPv4_address_preference is being passed */
  uint32_t primary_DNS_IPv4_address_preference;	/*  The value may be used as a preference during negotiation with the
       network. If not specified, the wireless device will attempt to obtain
       the DNS address automatically from the network. The negotiated value
       is provided to the host via DHCP.
   */

  /* Optional */
  /*  Secondary DNS address preference */
  uint8_t secondary_DNS_IPv4_address_preference_valid;	/* Must be set to true if secondary_DNS_IPv4_address_preference is being passed */
  uint32_t secondary_DNS_IPv4_address_preference;	/*  The value may be used as a preference during negotiation with the
       network. If not specified, the wireless device will attempt to obtain
       the DNS address automatically from the network. The negotiated value
       is provided to the host via DHCP.
   */

  /* Optional */
  /*  UMTS requested QoS */
  uint8_t umts_requested_qos_valid;	/* Must be set to true if umts_requested_qos is being passed */
  wds_umts_qos_type_v01 umts_requested_qos;

  /* Optional */
  /*  UMTS minimum QoS */
  uint8_t umts_minimum_qos_valid;	/* Must be set to true if umts_minimum_qos is being passed */
  wds_umts_qos_type_v01 umts_minimum_qos;

  /* Optional */
  /*  GPRS requested QoS */
  uint8_t gprs_requested_qos_valid;	/* Must be set to true if gprs_requested_qos is being passed */
  wds_gprs_qos_type_v01 gprs_requested_qos;

  /* Optional */
  /*  GRPS minimum Qos */
  uint8_t gprs_minimum_qos_valid;	/* Must be set to true if gprs_minimum_qos is being passed */
  wds_gprs_qos_type_v01 gprs_minimum_qos;

  /* Optional */
  /*  Username */
  uint8_t username_valid;	/* Must be set to true if username is being passed */
  char username[WDS_USER_NAME_MAX_V01 + 1];	/*  Username to be used during data network authentication

       QMI_ERR_ARG_TOO_LONG will be returned if the storage on the wireless
       device is insufficient in size to hold the value.
   */

  /* Optional */
  /*  Authentication preference */
  uint8_t authentication_preference_valid;	/* Must be set to true if authentication_preference is being passed */
  uint8_t authentication_preference;	/*  A bit map that indicates the authentication algorithm preference

       Bit 0 - PAP preference
       - 0 - PAP is never performed
       - 1 - PAP may be performed

       Bit 1 - CHAP preference
       - 0 - CHAP is never performed
       - 1 - CHAP may be performed

       All other bits are reserved and must be set to 0.

       If more than one bit is set, then the device decides which
       authentication procedure is performed while setting up the data
       session. For example, the device may have a policy to select
       the most secure authentication mechanism.
   */

  /* Optional */
  /*  IPv4 address preference */
  uint8_t ipv4_address_preference_valid;	/* Must be set to true if ipv4_address_preference is being passed */
  uint32_t ipv4_address_preference;	/*  Preferred IPv4 address to be assigned to the TE

       The actual assigned address is negotiated with the network and
       may differ from this value. If not specified, the IPv4 Address
       is obtained automatically from the network. The assigned value
       is provided to the host via DHCP.
   */

  /* Optional */
  /*  P-CSCF address using PCO Flag */
  uint8_t pcscf_addr_using_pco_valid;	/* Must be set to true if pcscf_addr_using_pco is being passed */
  uint8_t pcscf_addr_using_pco;	/* 
     - 1 (TRUE) implies Request PCSCF address using PCO
     - 0 (FALSE) implies do not request.

     By default this value is 0.
   */
}wds_get_profile_settings_resp_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Profile type */
  uint8_t profile_type;	/*  Identifies the technology type of the profile:
       - 0x00 - 3GPP
    */
}wds_get_default_settings_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type. */

  /* Optional */
  /*  Profile name */
  uint8_t profile_name_valid;	/* Must be set to true if profile_name is being passed */
  char profile_name[WDS_PROFILE_NAME_MAX_V01 + 1];	/*  One or more bytes describing the profile. The description may
       be a user-defined name for the profile.

       QMI_ERR_ARG_TOO_LONG will be returned if the profile_name is too long
    */

  /* Optional */
  /*  PDP type */
  uint8_t pdp_type_valid;	/* Must be set to true if pdp_type is being passed */
  uint8_t pdp_type;	/*  Packet Data Protocol (PDP) type specifies the type of data payload
       exchanged over the air link when the packet data session is
       established with this profile:
       - 0x00 - PDP-IP (IPv4)
    */

  /* Optional */
  /*  Context Access Point Node (APN) name */
  uint8_t apn_name_valid;	/* Must be set to true if apn_name is being passed */
  char apn_name[WDS_APN_NAME_MAX_V01 + 1];	/*  Access point name - A string parameter that is a logical name
       used to select the GGSN and the external packet data network.

       If the value is NULL or omitted, then the subscription default
       value will be requested.

       QMI_ERR_ARG_TOO_LONG will be returned if the APN name is too long.
   */

  /* Optional */
  /*  Primary DNS address preference */
  uint8_t primary_DNS_IPv4_address_preference_valid;	/* Must be set to true if primary_DNS_IPv4_address_preference is being passed */
  uint32_t primary_DNS_IPv4_address_preference;	/*  The value may be used as a preference during negotiation with the
       network. If not specified, the wireless device will attempt to obtain
       the DNS address automatically from the network. The negotiated value
       is provided to the host via DHCP.
   */

  /* Optional */
  /*  Secondary DNS address preference */
  uint8_t secondary_DNS_IPv4_address_preference_valid;	/* Must be set to true if secondary_DNS_IPv4_address_preference is being passed */
  uint32_t secondary_DNS_IPv4_address_preference;	/*  The value may be used as a preference during negotiation with the
       network. If not specified, the wireless device will attempt to obtain
       the DNS address automatically from the network. The negotiated value
       is provided to the host via DHCP.
   */

  /* Optional */
  /*  UMTS requested QoS */
  uint8_t umts_requested_qos_valid;	/* Must be set to true if umts_requested_qos is being passed */
  wds_umts_qos_type_v01 umts_requested_qos;

  /* Optional */
  /*  UMTS minimum QoS */
  uint8_t umts_minimum_qos_valid;	/* Must be set to true if umts_minimum_qos is being passed */
  wds_umts_qos_type_v01 umts_minimum_qos;

  /* Optional */
  /*  GPRS requested QoS */
  uint8_t gprs_requested_qos_valid;	/* Must be set to true if gprs_requested_qos is being passed */
  wds_gprs_qos_type_v01 gprs_requested_qos;

  /* Optional */
  /*  GRPS minimum Qos */
  uint8_t gprs_minimum_qos_valid;	/* Must be set to true if gprs_minimum_qos is being passed */
  wds_gprs_qos_type_v01 gprs_minimum_qos;

  /* Optional */
  /*  Username */
  uint8_t username_valid;	/* Must be set to true if username is being passed */
  char username[WDS_USER_NAME_MAX_V01 + 1];	/*  Username to be used during data network authentication

       QMI_ERR_ARG_TOO_LONG will be returned if the storage on the wireless
       device is insufficient in size to hold the value.
   */

  /* Optional */
  /*  Authentication preference */
  uint8_t authentication_preference_valid;	/* Must be set to true if authentication_preference is being passed */
  uint8_t authentication_preference;	/*  A bit map that indicates the authentication algorithm preference

       Bit 0 - PAP preference
       - 0 - PAP is never performed
       - 1 - PAP may be performed

       Bit 1 - CHAP preference
       - 0 - CHAP is never performed
       - 1 - CHAP may be performed

       All other bits are reserved and must be set to 0.

       If more than one bit is set, then the device decides which
       authentication procedure is performed while setting up the data
       session. For example, the device may have a policy to select
       the most secure authentication mechanism.
   */

  /* Optional */
  /*  IPv4 address preference */
  uint8_t ipv4_address_preference_valid;	/* Must be set to true if ipv4_address_preference is being passed */
  uint32_t ipv4_address_preference;	/*  Preferred IPv4 address to be assigned to the TE

       The actual assigned address is negotiated with the network and
       may differ from this value. If not specified, the IPv4 Address
       is obtained automatically from the network. The assigned value
       is provided to the host via DHCP.
   */

  /* Optional */
  /*  P-CSCF address using PCO Flag */
  uint8_t pcscf_addr_using_pco_valid;	/* Must be set to true if pcscf_addr_using_pco is being passed */
  uint8_t pcscf_addr_using_pco;	/* 
     - 1 (TRUE) implies Request PCSCF address using PCO
     - 0 (FALSE) implies do not request.

     By default this value is 0.
   */
}wds_get_default_settings_resp_msg_v01;	/* Message */

typedef struct {
  /* Optional */
  /*  Requested settings */
  uint8_t requested_settings_valid;	/* Must be set to true if requested_settings is being passed */
  uint32_t requested_settings;	/* 
      - Bit 0 - Profile identifier
      - Bit 1 - Profile name
      - Bit 2 - PDP type
      - Bit 3 - Apn name
      - Bit 4 - DNS address
      - Bit 5 - UMTS/GPRS granted QoS
      - Bit 6 - Username
      - Bit 7 - Authentication protocol
      - Bit 8 - IP address
      - Bit 9 - Gateway info (address and subnet mask)
      - Bit 10 - P-CSCF address using PCO flag
      - Bit 11 - PCSCF server address list
      - Bit 12 - PCSCF domain name list

      Set the bits corresponding to the information requested to
      1. All other bits must be set to 0.

      If any values are not available, then the corresponding TLVs
      will not be returned in the response.

      Absence of this mask TLV will result in the device returning all
      of the available information.
    */
}wds_get_runtime_settings_req_msg_v01;	/* Message */

typedef struct {
  char fqdn[511 + 1];	/*  FQDN string */
}wds_fqdn_type_v01;	/* Type */

typedef struct {
  uint8_t num_instances;

  char domain_name[511 + 1];
}wds_domain_name_list_type_v01;	/* Type */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type. */

  /* Optional */
  /*  Profile identifier */
  uint8_t profile_valid;	/* Must be set to true if profile is being passed */
  wds_profile_identifier_type_v01 profile;

  /* Optional */
  /*  IPv4 gateway address */
  uint8_t ipv4_gateway_addr_valid;	/* Must be set to true if ipv4_gateway_addr is being passed */
  uint32_t ipv4_gateway_addr;	/*  Gateway address */

  /* Optional */
  /*  IPv4 subnet mask */
  uint8_t ipv4_subnet_mask_valid;	/* Must be set to true if ipv4_subnet_mask is being passed */
  uint32_t ipv4_subnet_mask;	/*  Subnet mask */

  /* Optional */
  /*  P-CSCF address using PCO Flag */
  uint8_t pcscf_addr_using_pco_valid;	/* Must be set to true if pcscf_addr_using_pco is being passed */
  uint8_t pcscf_addr_using_pco;	/*  - 1 (TRUE) implies PCSCF address is requested using PCO
       - 0 (FALSE) implies it is not requested
   */

  /* Optional */
  /*  P-CSCF IPv4 server address list */
  uint8_t pcscf_ipv4_address_valid;	/* Must be set to true if pcscf_ipv4_address is being passed */
  uint32_t pcscf_ipv4_address_len;	/* Must be set to # of elements in pcscf_ipv4_address */
  uint32_t pcscf_ipv4_address[255];	/*  P-CSCF IPv4 server address */

  /* Optional */
  /*  P-CSCF FQDN list */
  uint8_t fqdn_valid;	/* Must be set to true if fqdn is being passed */
  uint32_t fqdn_len;	/* Must be set to # of elements in fqdn */
  wds_fqdn_type_v01 fqdn[10];

  /* Optional */
  /*  Primary IPv6 DNS address */
  uint8_t primary_dns_IPv6_address_valid;	/* Must be set to true if primary_dns_IPv6_address is being passed */
  uint16_t primary_dns_IPv6_address[8];	/*  IPv6 address (in network byte order) format (in hex)

       XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX

       This is a 16-byte array (in Big-endian format).
   */

  /* Optional */
  /*  Secondary IPv6 DNS address */
  uint8_t secondary_dns_IPv6_address_valid;	/* Must be set to true if secondary_dns_IPv6_address is being passed */
  uint16_t secondary_dns_IPv6_address[8];	/*  Fromat (see IPv6 address above) */

  /* Optional */
  /*  MTU */
  uint8_t mtu_valid;	/* Must be set to true if mtu is being passed */
  uint32_t mtu;	/*  MTU */

  /* Optional */
  /*  Domain name list */
  uint8_t domain_name_list_valid;	/* Must be set to true if domain_name_list is being passed */
  wds_domain_name_list_type_v01 domain_name_list;

  /* Optional */
  /*  IP family */
  uint8_t ip_family_valid;	/* Must be set to true if ip_family is being passed */
  uint8_t ip_family;	/*  0x04 - IPV4_ADDR
       0x06 - IPV6_ADDR
   */

  /* Optional */
  uint8_t im_cn_flag_valid;	/* Must be set to true if im_cn_flag is being passed */
  uint8_t im_cn_flag;	/*  0x00 - FALSE
       0x01 - TRUE
   */

  /* Optional */
  /*  Technology name */
  uint8_t technology_name_valid;	/* Must be set to true if technology_name is being passed */
  uint16_t technology_name;	/*  Technology on which current packet data session is in progress
       - CDMA - 0x8001
       - UMTS - 0x8004
   */
}wds_get_runtime_settings_resp_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Mobile IP mode */
  uint8_t mip_mode;	/*  Mobile IP setting:
     - 0x00 - MIP off (simple IP only)
     - 0x01 - MIP preferred
     - 0x02 - MIP only
    */
}wds_set_mip_mode_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}wds_set_mip_mode_resp_msg_v01;	/* Message */

/*
 * wds_get_mip_mode_req_msg is empty
 * typedef struct {
 * }wds_get_mip_mode_req_msg_v01;
 */

typedef struct {
  /* Mandatory */
  /*  Mobile IP mode */
  uint8_t mip_mode;	/*  Mobile IP setting:
     - 0x00 - MIP off (simple IP only)
     - 0x01 - MIP preferred
     - 0x02 - MIP only
    */

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}wds_get_mip_mode_resp_msg_v01;	/* Message */

/*
 * wds_get_dormancy_status_req_msg is empty
 * typedef struct {
 * }wds_get_dormancy_status_req_msg_v01;
 */

typedef struct {
  /* Mandatory */
  /*  Dormancy status */
  uint8_t dormancy_status;	/*  Current traffic channel status:
     - 0x01 - QMI_WDS_TRAFFIC_CH_DORMANT
     - 0x02 - QMI_WDS_TRAFFIC_CH_ACTIVE
    */

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}wds_get_dormancy_status_resp_msg_v01;	/* Message */

/*
 * wds_get_call_duration_req_msg is empty
 * typedef struct {
 * }wds_get_call_duration_req_msg_v01;
 */

typedef struct {
  /* Mandatory */
  /*  Call duration */
  uint64_t call_duration;	/*  Call duration in milliseconds */

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */

  /* Optional */
  /*  Last call duration */
  uint8_t last_call_duration_valid;	/* Must be set to true if last_call_duration is being passed */
  uint64_t last_call_duration;	/*  Call duration in milliseconds of the last data call since device was
       powered up (zero if no call was made); returned only if not in a call.
   */

  /* Optional */
  /*  Call active duration */
  uint8_t call_active_duration_valid;	/* Must be set to true if call_active_duration is being passed */
  uint64_t call_active_duration;	/*  Duration that the call was active, in milliseconds, of the current
       call; returned only if in a call.
   */

  /* Optional */
  /*  Last call active duration */
  uint8_t last_call_active_duration_valid;	/* Must be set to true if last_call_active_duration is being passed */
  uint64_t last_call_active_duration;	/*  Duration that the call was active, in milliseconds, of the last data
       call since the device was powered up (zero if no call has been made);
       returned only if not in a call.
   */
}wds_get_call_duration_resp_msg_v01;	/* Message */

/*
 * wds_get_current_data_bearer_technology_req_msg is empty
 * typedef struct {
 * }wds_get_current_data_bearer_technology_req_msg_v01;
 */

typedef struct {
  /* Mandatory */
  /*  Current data bearer technology */
  wds_current_bearer_tech_type_v01 current_bearer_tech;

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}wds_get_current_data_bearer_technology_resp_msg_v01;	/* Message */

typedef struct {
  uint8_t profile_type;	/*  Identifies the type of profile:
         - 0x00 - 3GPP
         - 0x01 - 3GPP2
   */

  uint8_t profile_family;	/*  Identifies the family of profile:
         - 0x01 - Sockets Family
   */
}wds_profile_id_family_type_v01;	/* Type */

typedef struct {
  /* Mandatory */
  /*  Profile Type */
  wds_profile_id_family_type_v01 profile;
}wds_get_default_profile_num_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  uint8_t profile_index;	/*  Profile number identifying the default profile */

  /* Optional */
  /*  Extended error code */
  uint8_t extended_error_code_valid;	/* Must be set to true if extended_error_code is being passed */
  uint16_t extended_error_code;	/*  Error code from the DS profile */
}wds_get_default_profile_num_resp_msg_v01;	/* Message */

typedef struct {
  uint8_t profile_type;	/*  Identifies the type of profile:
         - 0x00 - 3GPP
         - 0x01 - 3GPP2
   */

  uint8_t profile_family;	/*  Identifies the family of profile:
         - 0x01 - Sockets family
   */

  uint8_t profile_index;	/*  Profile number ot be set as default profile */
}wds_profile_identifier_with_family_type_v01;	/* Type */

typedef struct {
  /* Mandatory */
  /*  Profile Identifier */
  wds_profile_identifier_with_family_type_v01 profile_identifier;
}wds_set_default_profile_num_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type. */

  /* Optional */
  /*  Extended error code */
  uint8_t extended_error_code_valid;	/* Must be set to true if extended_error_code is being passed */
  uint16_t extended_error_code;	/*  Error code from the DS profile */
}wds_set_default_profile_num_resp_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Profile Identifier */
  wds_profile_identifier_type_v01 profile_identifier;
}wds_reset_profile_to_default_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type. */

  /* Optional */
  /*  Extended error code */
  uint8_t extended_error_code_valid;	/* Must be set to true if extended_error_code is being passed */
  uint16_t extended_error_code;	/*  Error code from the DS profile */
}wds_reset_profile_to_default_resp_msg_v01;	/* Message */

typedef struct {
  uint8_t profile_type;	/*  Identifies the type of profile that was created:
       - 0x00 - 3GPP
       - 0x01 - 3GPP2
    */

  uint8_t profile_index;	/*  Profile number whose profile_param_id needs to be set to invaild */

  uint32_t profile_param_id;	/*  Profile parameter that should be marked as invalid; only the following
       values are allowed:
         - 0x17 - UMTS Requested QoS
         - 0x18 - UMTS Minimum QoS
         - 0x19 - GPRS Requested QoS
         - 0x1A - GPRS Minimum QoS
         - 0x23 - TFT Filter ID 1
         - 0x24 - TFT Filter ID 2
   */
}wds_profile_param_type_v01;	/* Type */

typedef struct {
  /* Mandatory */
  /*  Profile Param */
  wds_profile_param_type_v01 profile_param;
}wds_reset_profile_param_to_invalid_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type. */

  /* Optional */
  /*  Extended error code */
  uint8_t extended_error_code_valid;	/* Must be set to true if extended_error_code is being passed */
  uint16_t extended_error_code;	/*  Error code from the DS profile */
}wds_reset_profile_param_to_invalid_resp_msg_v01;	/* Message */

typedef struct {
  /* Optional */
  /*  IP family preference */
  uint8_t ip_preference_valid;	/* Must be set to true if ip_preference is being passed */
  uint8_t ip_preference;	/*  IP family preference:
         - 0x04 - IPV4
         - 0x06 - IPV6
   */
}wds_set_client_ip_family_pref_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type. */
}wds_set_client_ip_family_pref_resp_msg_v01;	/* Message */

/*Service Message Definition*/
#define QMI_WDS_RESET_REQ_V01 0x0000
#define QMI_WDS_RESET_RESP_V01 0x0000
#define QMI_WDS_SET_EVENT_REPORT_REQ_V01 0x0001
#define QMI_WDS_SET_EVENT_REPORT_RESP_V01 0x0001
#define QMI_WDS_EVENT_REPORT_IND_V01 0x0001
#define QMI_WDS_ABORT_REQ_V01 0x0002
#define QMI_WDS_ABORT_RESP_V01 0x0002
#define QMI_WDS_START_NETWORK_INTERFACE_REQ_V01 0x0020
#define QMI_WDS_START_NETWORK_INTERFACE_RESP_V01 0x0020
#define QMI_WDS_STOP_NETWORK_INTERFACE_REQ_V01 0x0021
#define QMI_WDS_STOP_NETWORK_INTERFACE_RESP_V01 0x0021
#define QMI_WDS_GET_PKT_SRVC_STATUS_REQ_V01 0x0022
#define QMI_WDS_GET_PKT_SRVC_STATUS_RESP_V01 0x0022
#define QMI_WDS_PKT_SRVC_STATUS_IND_V01 0x0022
#define QMI_WDS_GET_CURRENT_CHANNEL_RATE_REQ_V01 0x0023
#define QMI_WDS_GET_CURRENT_CHANNEL_RATE_RESP_V01 0x0023
#define QMI_WDS_GET_PKT_STATISTICS_REQ_V01 0x0024
#define QMI_WDS_GET_PKT_STATISTICS_RESP_V01 0x0024
#define QMI_WDS_GO_DORMANT_REQ_V01 0x0025
#define QMI_WDS_GO_DORMANT_RESP_V01 0x0025
#define QMI_WDS_GO_ACTIVE_REQ_V01 0x0026
#define QMI_WDS_GO_ACTIVE_RESP_V01 0x0026
#define QMI_WDS_CREATE_PROFILE_REQ_V01 0x0027
#define QMI_WDS_CREATE_PROFILE_RESP_V01 0x0027
#define QMI_WDS_MODIFY_PROFILE_SETTINGS_REQ_V01 0x0028
#define QMI_WDS_MODIFY_PROFILE_SETTINGS_RESP_V01 0x0028
#define QMI_WDS_DELETE_PROFILE_REQ_V01 0x0029
#define QMI_WDS_DELETE_PROFILE_RESP_V01 0x0029
#define QMI_WDS_GET_PROFILE_LIST_REQ_V01 0x002A
#define QMI_WDS_GET_PROFILE_LIST_RESP_V01 0x002A
#define QMI_WDS_GET_PROFILE_SETTINGS_REQ_V01 0x002B
#define QMI_WDS_GET_PROFILE_SETTINGS_RESP_V01 0x002B
#define QMI_WDS_GET_DEFAULT_SETTINGS_REQ_V01 0x002C
#define QMI_WDS_GET_DEFAULT_SETTINGS_RESP_V01 0x002C
#define QMI_WDS_GET_RUNTIME_SETTINGS_REQ_V01 0x002D
#define QMI_WDS_GET_RUNTIME_SETTINGS_RESP_V01 0x002D
#define QMI_WDS_SET_MIP_MODE_REQ_V01 0x002E
#define QMI_WDS_SET_MIP_MODE_RESP_V01 0x002E
#define QMI_WDS_GET_MIP_MODE_REQ_V01 0x002F
#define QMI_WDS_GET_MIP_MODE_RESP_V01 0x002F
#define QMI_WDS_GET_DORMANCY_STATUS_REQ_V01 0x0030
#define QMI_WDS_GET_DORMANCY_STATUS_RESP_V01 0x0030
#define QMI_WDS_GET_CALL_DURATION_REQ_V01 0x0035
#define QMI_WDS_GET_CALL_DURATION_RESP_V01 0x0035
#define QMI_WDS_GET_CURRENT_DATA_BEARER_TECHNOLOGY_REQ_V01 0x0044
#define QMI_WDS_GET_CURRENT_DATA_BEARER_TECHNOLOGY_RESP_V01 0x0044
#define QMI_WDS_GET_DEFAULT_PROFILE_NUM_REQ_V01 0x0049
#define QMI_WDS_GET_DEFAULT_PROFILE_NUM_RESP_V01 0x0049
#define QMI_WDS_SET_DEFAULT_PROFILE_NUM_REQ_V01 0x004A
#define QMI_WDS_SET_DEFAULT_PROFILE_NUM_RESP_V01 0x004A
#define QMI_WDS_RESET_PROFILE_TO_DEFAULT_REQ_V01 0x004C
#define QMI_WDS_RESET_PROFILE_TO_DEFAULT_RESP_V01 0x004C
#define QMI_WDS_RESET_PROFILE_TO_DEFAULT_REQ_V01 0x004C
#define QMI_WDS_RESET_PROFILE_TO_DEFAULT_RESP_V01 0x004C
#define QMI_WDS_SET_CLIENT_IP_FAMILY_PREF_REQ_V01 0x004D
#define QMI_WDS_SET_CLIENT_IP_FAMILY_PREF_RESP_V01 0x004D

/* Service Object Accessor */
qmi_idl_service_object_type wds_get_service_object_internal_v01
 ( int32_t idl_maj_version, int32_t idl_min_version, int32_t library_version );
 
#define wds_get_service_object_v01( ) \
          wds_get_service_object_internal_v01( \
            WDS_V01_IDL_MAJOR_VERS, WDS_V01_IDL_MINOR_VERS, \
            WDS_V01_IDL_TOOL_VERS )


#ifdef __cplusplus
}
#endif
#endif

