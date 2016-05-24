/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        W I R E L E S S _ D A T A _ S E R V I C E _ V 0 1  . C

GENERAL DESCRIPTION
  This is the file which defines the wds service Data structures.

 Copyright (c) 2010 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential.

 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====* 
 *THIS IS AN AUTO GENERATED FILE. DO NOT ALTER IN ANY WAY 
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/* This file was generated with Tool version 02.01 
   It was generated on: Fri Oct 15 2010
   From IDL File: */

#include "stdint.h"
#include "qmi_idl_lib_internal.h"
#include "wireless_data_service_v01.h"
#include "common_v01.h"


/*Type Definitions*/
static const uint8_t wds_statistics_indicator_type_data_v01[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_statistics_indicator_type_v01, stats_period),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_statistics_indicator_type_v01, stats_mask),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t wds_channel_rate_type_data_v01[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_channel_rate_type_v01, current_channel_tx_rate),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_channel_rate_type_v01, current_channel_rx_rate),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t wds_current_bearer_tech_type_data_v01[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_current_bearer_tech_type_v01, current_nw),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_current_bearer_tech_type_v01, rat_mask),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_current_bearer_tech_type_v01, so_mask),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t wds_verbose_call_end_reason_type_data_v01[] = {
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(wds_verbose_call_end_reason_type_v01, call_end_reason_type),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(wds_verbose_call_end_reason_type_v01, call_end_reason),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t wds_packet_service_status_type_data_v01[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_packet_service_status_type_v01, connection_status),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_packet_service_status_type_v01, reconfiguration_required),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t wds_current_channel_rate_type_data_v01[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_current_channel_rate_type_v01, current_channel_tx_rate),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_current_channel_rate_type_v01, current_channel_rx_rate),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_current_channel_rate_type_v01, max_channel_tx_rate),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_current_channel_rate_type_v01, max_channel_rx_rate),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t wds_umts_qos_type_data_v01[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_umts_qos_type_v01, traffic_class),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_umts_qos_type_v01, max_uplink_bitrate),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_umts_qos_type_v01, max_downlink_bitrate),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_umts_qos_type_v01, guaranteed_uplink_bitrate),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_umts_qos_type_v01, guaranteed_downlink_bitrate),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_umts_qos_type_v01, qos_delivery_order),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_umts_qos_type_v01, max_sdu_size),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_umts_qos_type_v01, sdu_error_ratio),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_umts_qos_type_v01, residual_ber_ratio),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_umts_qos_type_v01, delivery_erroneous_SDUs),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_umts_qos_type_v01, transfer_delay),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_umts_qos_type_v01, traffic_handling_priority),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t wds_umts_qos_with_sig_ind_type_data_v01[] = {
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_umts_qos_with_sig_ind_type_v01, umts_qos),
 6, 0,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_umts_qos_with_sig_ind_type_v01, sig_ind),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t wds_gprs_qos_type_data_v01[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_gprs_qos_type_v01, precedence_class),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_gprs_qos_type_v01, delay_class),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_gprs_qos_type_v01, reliability_class),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_gprs_qos_type_v01, peak_throughput_class),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_gprs_qos_type_v01, mean_throughput_class),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t wds_tft_id_param_type_data_v01[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_tft_id_param_type_v01, filter_id),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_tft_id_param_type_v01, eval_id),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_tft_id_param_type_v01, ip_version),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_tft_id_param_type_v01, source_ip),
  16,

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_tft_id_param_type_v01, source_ip_mask),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_tft_id_param_type_v01, next_header),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(wds_tft_id_param_type_v01, dest_port_range_start),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(wds_tft_id_param_type_v01, dest_port_range_end),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(wds_tft_id_param_type_v01, src_port_range_start),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(wds_tft_id_param_type_v01, src_port_range_end),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_tft_id_param_type_v01, ipsec_spi),

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(wds_tft_id_param_type_v01, tos_mask),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_tft_id_param_type_v01, flow_label),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t wds_3gpp_lte_qos_params_data_v01[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_3gpp_lte_qos_params_v01, qci),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_3gpp_lte_qos_params_v01, g_dl_bit_rate),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_3gpp_lte_qos_params_v01, max_dl_bit_rate),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_3gpp_lte_qos_params_v01, g_ul_bit_rate),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_3gpp_lte_qos_params_v01, max_ul_bit_rate),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t wds_profile_identifier_type_data_v01[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_profile_identifier_type_v01, profile_type),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_profile_identifier_type_v01, profile_index),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t wds_profile_info_type_data_v01[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_profile_info_type_v01, profile_type),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_profile_info_type_v01, profile_index),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_STRING,
  QMI_IDL_OFFSET8(wds_profile_info_type_v01, profile_name),
  WDS_PROFILE_NAME_MAX_V01,

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t wds_fqdn_type_data_v01[] = {
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET8(wds_fqdn_type_v01, fqdn),
  ((511) & 0xFF), ((511) >> 8),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t wds_domain_name_list_type_data_v01[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_domain_name_list_type_v01, num_instances),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 | QMI_IDL_STRING,
  QMI_IDL_OFFSET8(wds_domain_name_list_type_v01, domain_name),
  ((511) & 0xFF), ((511) >> 8),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t wds_profile_id_family_type_data_v01[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_profile_id_family_type_v01, profile_type),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_profile_id_family_type_v01, profile_family),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t wds_profile_identifier_with_family_type_data_v01[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_profile_identifier_with_family_type_v01, profile_type),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_profile_identifier_with_family_type_v01, profile_family),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_profile_identifier_with_family_type_v01, profile_index),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t wds_profile_param_type_data_v01[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_profile_param_type_v01, profile_type),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_profile_param_type_v01, profile_index),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_profile_param_type_v01, profile_param_id),

  QMI_IDL_FLAG_END_VALUE
};

/*Message Definitions*/
/* 
 * wds_reset_req_msg is empty
 * static const uint8_t wds_reset_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t wds_reset_resp_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_reset_resp_msg_v01, resp),
  0, 1
};

static const uint8_t wds_set_event_report_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_set_event_report_req_msg_v01, report_channel_rate) - QMI_IDL_OFFSET8(wds_set_event_report_req_msg_v01, report_channel_rate_valid)),
  0x10,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_set_event_report_req_msg_v01, report_channel_rate),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_set_event_report_req_msg_v01, report_stats) - QMI_IDL_OFFSET8(wds_set_event_report_req_msg_v01, report_stats_valid)),
  0x11,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_set_event_report_req_msg_v01, report_stats),
  0, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_set_event_report_req_msg_v01, report_data_bearer_tech) - QMI_IDL_OFFSET8(wds_set_event_report_req_msg_v01, report_data_bearer_tech_valid)),
  0x12,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_set_event_report_req_msg_v01, report_data_bearer_tech),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_set_event_report_req_msg_v01, report_dormancy_status) - QMI_IDL_OFFSET8(wds_set_event_report_req_msg_v01, report_dormancy_status_valid)),
  0x13,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_set_event_report_req_msg_v01, report_dormancy_status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_set_event_report_req_msg_v01, report_current_data_bearer_tech) - QMI_IDL_OFFSET8(wds_set_event_report_req_msg_v01, report_current_data_bearer_tech_valid)),
  0x15,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_set_event_report_req_msg_v01, report_current_data_bearer_tech)
};

static const uint8_t wds_set_event_report_resp_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_set_event_report_resp_msg_v01, resp),
  0, 1
};

static const uint8_t wds_event_report_ind_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, tx_ok_count) - QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, tx_ok_count_valid)),
  0x10,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, tx_ok_count),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, rx_ok_count) - QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, rx_ok_count_valid)),
  0x11,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, rx_ok_count),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, tx_err_count) - QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, tx_err_count_valid)),
  0x12,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, tx_err_count),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, rx_err_count) - QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, rx_err_count_valid)),
  0x13,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, rx_err_count),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, tx_ofl_count) - QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, tx_ofl_count_valid)),
  0x14,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, tx_ofl_count),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, rx_ofl_count) - QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, rx_ofl_count_valid)),
  0x15,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, rx_ofl_count),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, channel_rate) - QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, channel_rate_valid)),
  0x16,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, channel_rate),
  1, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, data_bearer_tech) - QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, data_bearer_tech_valid)),
  0x17,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, data_bearer_tech),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, dormancy_status) - QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, dormancy_status_valid)),
  0x18,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, dormancy_status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, current_bearer_tech) - QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, current_bearer_tech_valid)),
  0x1D,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_event_report_ind_msg_v01, current_bearer_tech),
  2, 0
};

static const uint8_t wds_abort_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(wds_abort_req_msg_v01, tx_id)
};

static const uint8_t wds_abort_resp_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_abort_resp_msg_v01, resp),
  0, 1
};

static const uint8_t wds_start_network_interface_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, primary_DNS_IPv4_address_preference) - QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, primary_DNS_IPv4_address_preference_valid)),
  0x10,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, primary_DNS_IPv4_address_preference),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, secondary_DNS_IPv4_address_preference) - QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, secondary_DNS_IPv4_address_preference_valid)),
  0x11,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, secondary_DNS_IPv4_address_preference),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, primary_nbns_address_pref) - QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, primary_nbns_address_pref_valid)),
  0x12,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, primary_nbns_address_pref),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, secondary_nbns_address_pref) - QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, secondary_nbns_address_pref_valid)),
  0x13,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, secondary_nbns_address_pref),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, apn_name) - QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, apn_name_valid)),
  0x14,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, apn_name),
  WDS_APN_NAME_MAX_V01,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, ipv4_address_pref) - QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, ipv4_address_pref_valid)),
  0x15,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, ipv4_address_pref),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, authentication_preference) - QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, authentication_preference_valid)),
  0x16,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, authentication_preference),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, username) - QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, username_valid)),
  0x17,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, username),
  WDS_USER_NAME_MAX_V01,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, password) - QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, password_valid)),
  0x18,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, password),
  WDS_PASSWORD_MAX_V01,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, ip_family_preference) - QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, ip_family_preference_valid)),
  0x19,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, ip_family_preference),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, technology_preference) - QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, technology_preference_valid)),
  0x30,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, technology_preference),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, profile_index) - QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, profile_index_valid)),
  0x31,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, profile_index),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, Threegpp2_profile_index) - QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, Threegpp2_profile_index_valid)),
  0x32,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, Threegpp2_profile_index),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, ext_technology_preference) - QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, ext_technology_preference_valid)),
  0x34,
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, ext_technology_preference),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, call_type) - QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, call_type_valid)),
  0x35,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_start_network_interface_req_msg_v01, call_type)
};

static const uint8_t wds_start_network_interface_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_start_network_interface_resp_msg_v01, pkt_data_handle),

  0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_start_network_interface_resp_msg_v01, resp),
  0, 1,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_start_network_interface_resp_msg_v01, call_end_reason) - QMI_IDL_OFFSET8(wds_start_network_interface_resp_msg_v01, call_end_reason_valid)),
  0x10,
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(wds_start_network_interface_resp_msg_v01, call_end_reason),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_start_network_interface_resp_msg_v01, verbose_call_end_reason) - QMI_IDL_OFFSET8(wds_start_network_interface_resp_msg_v01, verbose_call_end_reason_valid)),
  0x11,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_start_network_interface_resp_msg_v01, verbose_call_end_reason),
  3, 0
};

static const uint8_t wds_stop_network_interface_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_stop_network_interface_req_msg_v01, pkt_data_handle)
};

static const uint8_t wds_stop_network_interface_resp_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_stop_network_interface_resp_msg_v01, resp),
  0, 1
};

/* 
 * wds_get_pkt_srvc_status_req_msg is empty
 * static const uint8_t wds_get_pkt_srvc_status_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t wds_get_pkt_srvc_status_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_get_pkt_srvc_status_resp_msg_v01, connection_status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_pkt_srvc_status_resp_msg_v01, resp),
  0, 1
};

static const uint8_t wds_pkt_srvc_status_ind_msg_data_v01[] = {
  0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_pkt_srvc_status_ind_msg_v01, status),
  4, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_pkt_srvc_status_ind_msg_v01, call_end_reason) - QMI_IDL_OFFSET8(wds_pkt_srvc_status_ind_msg_v01, call_end_reason_valid)),
  0x10,
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(wds_pkt_srvc_status_ind_msg_v01, call_end_reason),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_pkt_srvc_status_ind_msg_v01, verbose_call_end_reason) - QMI_IDL_OFFSET8(wds_pkt_srvc_status_ind_msg_v01, verbose_call_end_reason_valid)),
  0x11,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_pkt_srvc_status_ind_msg_v01, verbose_call_end_reason),
  3, 0
};

/* 
 * wds_get_current_channel_rate_req_msg is empty
 * static const uint8_t wds_get_current_channel_rate_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t wds_get_current_channel_rate_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_current_channel_rate_resp_msg_v01, rates),
  5, 0,

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_current_channel_rate_resp_msg_v01, resp),
  0, 1
};

static const uint8_t wds_get_pkt_statistics_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_get_pkt_statistics_req_msg_v01, stat_mask)
};

static const uint8_t wds_get_pkt_statistics_resp_msg_data_v01[] = {
  0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_pkt_statistics_resp_msg_v01, resp),
  0, 1,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_pkt_statistics_resp_msg_v01, tx_ok_count) - QMI_IDL_OFFSET8(wds_get_pkt_statistics_resp_msg_v01, tx_ok_count_valid)),
  0x10,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_get_pkt_statistics_resp_msg_v01, tx_ok_count),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_pkt_statistics_resp_msg_v01, rx_ok_count) - QMI_IDL_OFFSET8(wds_get_pkt_statistics_resp_msg_v01, rx_ok_count_valid)),
  0x11,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_get_pkt_statistics_resp_msg_v01, rx_ok_count),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_pkt_statistics_resp_msg_v01, tx_err_count) - QMI_IDL_OFFSET8(wds_get_pkt_statistics_resp_msg_v01, tx_err_count_valid)),
  0x12,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_get_pkt_statistics_resp_msg_v01, tx_err_count),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_pkt_statistics_resp_msg_v01, rx_err_count) - QMI_IDL_OFFSET8(wds_get_pkt_statistics_resp_msg_v01, rx_err_count_valid)),
  0x13,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_get_pkt_statistics_resp_msg_v01, rx_err_count),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_pkt_statistics_resp_msg_v01, tx_ofl_count) - QMI_IDL_OFFSET8(wds_get_pkt_statistics_resp_msg_v01, tx_ofl_count_valid)),
  0x14,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_get_pkt_statistics_resp_msg_v01, tx_ofl_count),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_pkt_statistics_resp_msg_v01, rx_ofl_count) - QMI_IDL_OFFSET8(wds_get_pkt_statistics_resp_msg_v01, rx_ofl_count_valid)),
  0x15,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_get_pkt_statistics_resp_msg_v01, rx_ofl_count)
};

/* 
 * wds_go_dormant_req_msg is empty
 * static const uint8_t wds_go_dormant_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t wds_go_dormant_resp_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_go_dormant_resp_msg_v01, resp),
  0, 1
};

/* 
 * wds_go_active_req_msg is empty
 * static const uint8_t wds_go_active_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t wds_go_active_resp_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_go_active_resp_msg_v01, resp),
  0, 1
};

static const uint8_t wds_create_profile_req_msg_data_v01[] = {
  0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, profile_type),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, profile_name) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, profile_name_valid)),
  0x10,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, profile_name),
  WDS_PROFILE_NAME_MAX_V01,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, pdp_type) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, pdp_type_valid)),
  0x11,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, pdp_type),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, pdp_hdr_compression_type) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, pdp_hdr_compression_type_valid)),
  0x12,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, pdp_hdr_compression_type),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, pdp_data_compression_type) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, pdp_data_compression_type_valid)),
  0x13,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, pdp_data_compression_type),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, apn_name) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, apn_name_valid)),
  0x14,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, apn_name),
  WDS_APN_NAME_MAX_V01,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, primary_DNS_IPv4_address_preference) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, primary_DNS_IPv4_address_preference_valid)),
  0x15,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, primary_DNS_IPv4_address_preference),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, secondary_DNS_IPv4_address_preference) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, secondary_DNS_IPv4_address_preference_valid)),
  0x16,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, secondary_DNS_IPv4_address_preference),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, umts_requested_qos) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, umts_requested_qos_valid)),
  0x17,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, umts_requested_qos),
  6, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, umts_minimum_qos) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, umts_minimum_qos_valid)),
  0x18,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, umts_minimum_qos),
  6, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, gprs_requested_qos) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, gprs_requested_qos_valid)),
  0x19,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, gprs_requested_qos),
  8, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, gprs_minimum_qos) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, gprs_minimum_qos_valid)),
  0x1A,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, gprs_minimum_qos),
  8, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, username) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, username_valid)),
  0x1B,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, username),
  WDS_USER_NAME_MAX_V01,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, password) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, password_valid)),
  0x1C,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, password),
  WDS_PASSWORD_MAX_V01,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, authentication_preference) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, authentication_preference_valid)),
  0x1D,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, authentication_preference),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, ipv4_address_preference) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, ipv4_address_preference_valid)),
  0x1E,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, ipv4_address_preference),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, pcscf_addr_using_pco) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, pcscf_addr_using_pco_valid)),
  0x1F,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, pcscf_addr_using_pco),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, pdp_access_control_flag) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, pdp_access_control_flag_valid)),
  0x20,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, pdp_access_control_flag),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, pcsf_addr_using_dhcp) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, pcsf_addr_using_dhcp_valid)),
  0x21,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, pcsf_addr_using_dhcp),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, im_cn_flag) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, im_cn_flag_valid)),
  0x22,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, im_cn_flag),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, tft_id1_params) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, tft_id1_params_valid)),
  0x23,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, tft_id1_params),
  9, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, tft_id2_params) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, tft_id2_params_valid)),
  0x24,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, tft_id2_params),
  9, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, pdp_context) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, pdp_context_valid)),
  0x25,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, pdp_context),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, secondary_flag) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, secondary_flag_valid)),
  0x26,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, secondary_flag),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, primary_id) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, primary_id_valid)),
  0x27,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, primary_id),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, ipv6_address_preference) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, ipv6_address_preference_valid)),
  0x28,
  QMI_IDL_FLAGS_IS_ARRAY |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, ipv6_address_preference),
  16,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, umts_requested_qos_with_sig_ind) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, umts_requested_qos_with_sig_ind_valid)),
  0x29,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, umts_requested_qos_with_sig_ind),
  7, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, umts_minimum_qos_with_sig_ind) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, umts_minimum_qos_with_sig_ind_valid)),
  0x2A,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, umts_minimum_qos_with_sig_ind),
  7, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, primary_dns_ipv6_address_preference) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, primary_dns_ipv6_address_preference_valid)),
  0x2B,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, primary_dns_ipv6_address_preference),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, secodnary_dns_ipv6_address_preference) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, secodnary_dns_ipv6_address_preference_valid)),
  0x2C,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, secodnary_dns_ipv6_address_preference),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, addr_allocation_preference) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, addr_allocation_preference_valid)),
  0x2D,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, addr_allocation_preference),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, threegpp_lte_qos_params) - QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, threegpp_lte_qos_params_valid)),
  0x2E,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_create_profile_req_msg_v01, threegpp_lte_qos_params),
  10, 0
};

static const uint8_t wds_create_profile_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_create_profile_resp_msg_v01, profile),
  11, 0,

  0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_create_profile_resp_msg_v01, resp),
  0, 1,

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_create_profile_resp_msg_v01, extended_error_code) - QMI_IDL_OFFSET8(wds_create_profile_resp_msg_v01, extended_error_code_valid)),
  0xE0,
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(wds_create_profile_resp_msg_v01, extended_error_code)
};

static const uint8_t wds_modify_profile_settings_req_msg_data_v01[] = {
  0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, profile),
  11, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, profile_name) - QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, profile_name_valid)),
  0x10,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, profile_name),
  WDS_PROFILE_NAME_MAX_V01,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, pdp_type) - QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, pdp_type_valid)),
  0x11,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, pdp_type),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, apn_name) - QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, apn_name_valid)),
  0x14,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, apn_name),
  WDS_APN_NAME_MAX_V01,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, primary_DNS_IPv4_address_preference) - QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, primary_DNS_IPv4_address_preference_valid)),
  0x15,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, primary_DNS_IPv4_address_preference),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, secondary_DNS_IPv4_address_preference) - QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, secondary_DNS_IPv4_address_preference_valid)),
  0x16,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, secondary_DNS_IPv4_address_preference),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, umts_requested_qos) - QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, umts_requested_qos_valid)),
  0x17,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, umts_requested_qos),
  6, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, umts_minimum_qos) - QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, umts_minimum_qos_valid)),
  0x18,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, umts_minimum_qos),
  6, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, gprs_requested_qos) - QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, gprs_requested_qos_valid)),
  0x19,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, gprs_requested_qos),
  8, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, gprs_minimum_qos) - QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, gprs_minimum_qos_valid)),
  0x1A,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, gprs_minimum_qos),
  8, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, username) - QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, username_valid)),
  0x1B,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, username),
  WDS_USER_NAME_MAX_V01,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, password) - QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, password_valid)),
  0x1C,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, password),
  WDS_PASSWORD_MAX_V01,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, authentication_preference) - QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, authentication_preference_valid)),
  0x1D,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, authentication_preference),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, ipv4_address_preference) - QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, ipv4_address_preference_valid)),
  0x1E,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, ipv4_address_preference),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, pcscf_addr_using_pco) - QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, pcscf_addr_using_pco_valid)),
  0x1F,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_modify_profile_settings_req_msg_v01, pcscf_addr_using_pco)
};

static const uint8_t wds_modify_profile_settings_resp_msg_data_v01[] = {
  0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_modify_profile_settings_resp_msg_v01, resp),
  0, 1,

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_modify_profile_settings_resp_msg_v01, extended_error_code) - QMI_IDL_OFFSET8(wds_modify_profile_settings_resp_msg_v01, extended_error_code_valid)),
  0xE0,
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(wds_modify_profile_settings_resp_msg_v01, extended_error_code)
};

static const uint8_t wds_delete_profile_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_delete_profile_req_msg_v01, profile),
  11, 0
};

static const uint8_t wds_delete_profile_resp_msg_data_v01[] = {
  0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_delete_profile_resp_msg_v01, resp),
  0, 1,

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_delete_profile_resp_msg_v01, extended_error_code) - QMI_IDL_OFFSET8(wds_delete_profile_resp_msg_v01, extended_error_code_valid)),
  0xE0,
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(wds_delete_profile_resp_msg_v01, extended_error_code)
};

static const uint8_t wds_get_profile_list_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_profile_list_req_msg_v01, profile_type) - QMI_IDL_OFFSET8(wds_get_profile_list_req_msg_v01, profile_type_valid)),
  0x10,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_get_profile_list_req_msg_v01, profile_type)
};

static const uint8_t wds_get_profile_list_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_profile_list_resp_msg_v01, profiles),
  10,
  QMI_IDL_OFFSET8(wds_get_profile_list_resp_msg_v01, profiles) - QMI_IDL_OFFSET8(wds_get_profile_list_resp_msg_v01, profiles_len),
  12, 0,

  0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_profile_list_resp_msg_v01, resp),
  0, 1,

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_profile_list_resp_msg_v01, extended_error_code) - QMI_IDL_OFFSET8(wds_get_profile_list_resp_msg_v01, extended_error_code_valid)),
  0xE0,
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(wds_get_profile_list_resp_msg_v01, extended_error_code)
};

static const uint8_t wds_get_profile_settings_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_profile_settings_req_msg_v01, profile),
  11, 0
};

static const uint8_t wds_get_profile_settings_resp_msg_data_v01[] = {
  0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, resp),
  0, 1,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, profile_name) - QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, profile_name_valid)),
  0x10,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, profile_name),
  WDS_PROFILE_NAME_MAX_V01,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, pdp_type) - QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, pdp_type_valid)),
  0x11,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, pdp_type),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, apn_name) - QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, apn_name_valid)),
  0x14,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, apn_name),
  WDS_APN_NAME_MAX_V01,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, primary_DNS_IPv4_address_preference) - QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, primary_DNS_IPv4_address_preference_valid)),
  0x15,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, primary_DNS_IPv4_address_preference),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, secondary_DNS_IPv4_address_preference) - QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, secondary_DNS_IPv4_address_preference_valid)),
  0x16,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, secondary_DNS_IPv4_address_preference),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, umts_requested_qos) - QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, umts_requested_qos_valid)),
  0x17,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, umts_requested_qos),
  6, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, umts_minimum_qos) - QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, umts_minimum_qos_valid)),
  0x18,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, umts_minimum_qos),
  6, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, gprs_requested_qos) - QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, gprs_requested_qos_valid)),
  0x19,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, gprs_requested_qos),
  8, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, gprs_minimum_qos) - QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, gprs_minimum_qos_valid)),
  0x1A,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, gprs_minimum_qos),
  8, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, username) - QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, username_valid)),
  0x1B,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, username),
  WDS_USER_NAME_MAX_V01,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, authentication_preference) - QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, authentication_preference_valid)),
  0x1D,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, authentication_preference),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, ipv4_address_preference) - QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, ipv4_address_preference_valid)),
  0x1E,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, ipv4_address_preference),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, pcscf_addr_using_pco) - QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, pcscf_addr_using_pco_valid)),
  0x1F,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_get_profile_settings_resp_msg_v01, pcscf_addr_using_pco)
};

static const uint8_t wds_get_default_settings_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_get_default_settings_req_msg_v01, profile_type)
};

static const uint8_t wds_get_default_settings_resp_msg_data_v01[] = {
  0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, resp),
  0, 1,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, profile_name) - QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, profile_name_valid)),
  0x10,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, profile_name),
  WDS_PROFILE_NAME_MAX_V01,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, pdp_type) - QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, pdp_type_valid)),
  0x11,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, pdp_type),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, apn_name) - QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, apn_name_valid)),
  0x14,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, apn_name),
  WDS_APN_NAME_MAX_V01,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, primary_DNS_IPv4_address_preference) - QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, primary_DNS_IPv4_address_preference_valid)),
  0x15,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, primary_DNS_IPv4_address_preference),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, secondary_DNS_IPv4_address_preference) - QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, secondary_DNS_IPv4_address_preference_valid)),
  0x16,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, secondary_DNS_IPv4_address_preference),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, umts_requested_qos) - QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, umts_requested_qos_valid)),
  0x17,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, umts_requested_qos),
  6, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, umts_minimum_qos) - QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, umts_minimum_qos_valid)),
  0x18,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, umts_minimum_qos),
  6, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, gprs_requested_qos) - QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, gprs_requested_qos_valid)),
  0x19,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, gprs_requested_qos),
  8, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, gprs_minimum_qos) - QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, gprs_minimum_qos_valid)),
  0x1A,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, gprs_minimum_qos),
  8, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, username) - QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, username_valid)),
  0x1B,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, username),
  WDS_USER_NAME_MAX_V01,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, authentication_preference) - QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, authentication_preference_valid)),
  0x1D,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, authentication_preference),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, ipv4_address_preference) - QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, ipv4_address_preference_valid)),
  0x1E,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, ipv4_address_preference),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, pcscf_addr_using_pco) - QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, pcscf_addr_using_pco_valid)),
  0x1F,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_get_default_settings_resp_msg_v01, pcscf_addr_using_pco)
};

static const uint8_t wds_get_runtime_settings_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_runtime_settings_req_msg_v01, requested_settings) - QMI_IDL_OFFSET8(wds_get_runtime_settings_req_msg_v01, requested_settings_valid)),
  0x10,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_get_runtime_settings_req_msg_v01, requested_settings)
};

static const uint8_t wds_get_runtime_settings_resp_msg_data_v01[] = {
  0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, resp),
  0, 1,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, profile) - QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, profile_valid)),
  0x1F,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, profile),
  11, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, ipv4_gateway_addr) - QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, ipv4_gateway_addr_valid)),
  0x20,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, ipv4_gateway_addr),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, ipv4_subnet_mask) - QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, ipv4_subnet_mask_valid)),
  0x21,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, ipv4_subnet_mask),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, pcscf_addr_using_pco) - QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, pcscf_addr_using_pco_valid)),
  0x22,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, pcscf_addr_using_pco),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, pcscf_ipv4_address) - QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, pcscf_ipv4_address_valid)),
  0x23,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, pcscf_ipv4_address),
  255,
  QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, pcscf_ipv4_address) - QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, pcscf_ipv4_address_len),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, fqdn) - QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, fqdn_valid)),
  0x24,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, fqdn),
  10,
  QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, fqdn) - QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, fqdn_len),
  13, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, primary_dns_IPv6_address) - QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, primary_dns_IPv6_address_valid)),
  0x27,
  QMI_IDL_FLAGS_IS_ARRAY |   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, primary_dns_IPv6_address),
  8,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, secondary_dns_IPv6_address) - QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, secondary_dns_IPv6_address_valid)),
  0x28,
  QMI_IDL_FLAGS_IS_ARRAY |   QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, secondary_dns_IPv6_address),
  8,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, mtu) - QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, mtu_valid)),
  0x29,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, mtu),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, domain_name_list) - QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, domain_name_list_valid)),
  0x2A,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, domain_name_list),
  14, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, ip_family) - QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, ip_family_valid)),
  0x2B,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, ip_family),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, im_cn_flag) - QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, im_cn_flag_valid)),
  0x2C,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, im_cn_flag),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, technology_name) - QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, technology_name_valid)),
  0x2D,
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(wds_get_runtime_settings_resp_msg_v01, technology_name)
};

static const uint8_t wds_set_mip_mode_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_set_mip_mode_req_msg_v01, mip_mode)
};

static const uint8_t wds_set_mip_mode_resp_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_set_mip_mode_resp_msg_v01, resp),
  0, 1
};

/* 
 * wds_get_mip_mode_req_msg is empty
 * static const uint8_t wds_get_mip_mode_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t wds_get_mip_mode_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_get_mip_mode_resp_msg_v01, mip_mode),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_mip_mode_resp_msg_v01, resp),
  0, 1
};

/* 
 * wds_get_dormancy_status_req_msg is empty
 * static const uint8_t wds_get_dormancy_status_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t wds_get_dormancy_status_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_get_dormancy_status_resp_msg_v01, dormancy_status),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_dormancy_status_resp_msg_v01, resp),
  0, 1
};

/* 
 * wds_get_call_duration_req_msg is empty
 * static const uint8_t wds_get_call_duration_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t wds_get_call_duration_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(wds_get_call_duration_resp_msg_v01, call_duration),

  0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_call_duration_resp_msg_v01, resp),
  0, 1,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_call_duration_resp_msg_v01, last_call_duration) - QMI_IDL_OFFSET8(wds_get_call_duration_resp_msg_v01, last_call_duration_valid)),
  0x10,
  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(wds_get_call_duration_resp_msg_v01, last_call_duration),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_call_duration_resp_msg_v01, call_active_duration) - QMI_IDL_OFFSET8(wds_get_call_duration_resp_msg_v01, call_active_duration_valid)),
  0x11,
  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(wds_get_call_duration_resp_msg_v01, call_active_duration),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_call_duration_resp_msg_v01, last_call_active_duration) - QMI_IDL_OFFSET8(wds_get_call_duration_resp_msg_v01, last_call_active_duration_valid)),
  0x12,
  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(wds_get_call_duration_resp_msg_v01, last_call_active_duration)
};

/* 
 * wds_get_current_data_bearer_technology_req_msg is empty
 * static const uint8_t wds_get_current_data_bearer_technology_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t wds_get_current_data_bearer_technology_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_current_data_bearer_technology_resp_msg_v01, current_bearer_tech),
  2, 0,

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_current_data_bearer_technology_resp_msg_v01, resp),
  0, 1
};

static const uint8_t wds_get_default_profile_num_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_get_default_profile_num_req_msg_v01, profile),
  15, 0
};

static const uint8_t wds_get_default_profile_num_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_get_default_profile_num_resp_msg_v01, profile_index),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_get_default_profile_num_resp_msg_v01, extended_error_code) - QMI_IDL_OFFSET8(wds_get_default_profile_num_resp_msg_v01, extended_error_code_valid)),
  0xE0,
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(wds_get_default_profile_num_resp_msg_v01, extended_error_code)
};

static const uint8_t wds_set_default_profile_num_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_set_default_profile_num_req_msg_v01, profile_identifier),
  16, 0
};

static const uint8_t wds_set_default_profile_num_resp_msg_data_v01[] = {
  0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_set_default_profile_num_resp_msg_v01, resp),
  0, 1,

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_set_default_profile_num_resp_msg_v01, extended_error_code) - QMI_IDL_OFFSET8(wds_set_default_profile_num_resp_msg_v01, extended_error_code_valid)),
  0xE0,
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(wds_set_default_profile_num_resp_msg_v01, extended_error_code)
};

static const uint8_t wds_reset_profile_to_default_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_reset_profile_to_default_req_msg_v01, profile_identifier),
  11, 0
};

static const uint8_t wds_reset_profile_to_default_resp_msg_data_v01[] = {
  0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_reset_profile_to_default_resp_msg_v01, resp),
  0, 1,

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_reset_profile_to_default_resp_msg_v01, extended_error_code) - QMI_IDL_OFFSET8(wds_reset_profile_to_default_resp_msg_v01, extended_error_code_valid)),
  0xE0,
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(wds_reset_profile_to_default_resp_msg_v01, extended_error_code)
};

static const uint8_t wds_reset_profile_param_to_invalid_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_reset_profile_param_to_invalid_req_msg_v01, profile_param),
  17, 0
};

static const uint8_t wds_reset_profile_param_to_invalid_resp_msg_data_v01[] = {
  0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_reset_profile_param_to_invalid_resp_msg_v01, resp),
  0, 1,

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_reset_profile_param_to_invalid_resp_msg_v01, extended_error_code) - QMI_IDL_OFFSET8(wds_reset_profile_param_to_invalid_resp_msg_v01, extended_error_code_valid)),
  0xE0,
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(wds_reset_profile_param_to_invalid_resp_msg_v01, extended_error_code)
};

static const uint8_t wds_set_client_ip_family_pref_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(wds_set_client_ip_family_pref_req_msg_v01, ip_preference) - QMI_IDL_OFFSET8(wds_set_client_ip_family_pref_req_msg_v01, ip_preference_valid)),
  0x10,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(wds_set_client_ip_family_pref_req_msg_v01, ip_preference)
};

static const uint8_t wds_set_client_ip_family_pref_resp_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(wds_set_client_ip_family_pref_resp_msg_v01, resp),
  0, 1
};

/* Type Table */
static const qmi_idl_type_table_entry  wds_type_table_v01[] = {
  {sizeof(wds_statistics_indicator_type_v01), wds_statistics_indicator_type_data_v01},
  {sizeof(wds_channel_rate_type_v01), wds_channel_rate_type_data_v01},
  {sizeof(wds_current_bearer_tech_type_v01), wds_current_bearer_tech_type_data_v01},
  {sizeof(wds_verbose_call_end_reason_type_v01), wds_verbose_call_end_reason_type_data_v01},
  {sizeof(wds_packet_service_status_type_v01), wds_packet_service_status_type_data_v01},
  {sizeof(wds_current_channel_rate_type_v01), wds_current_channel_rate_type_data_v01},
  {sizeof(wds_umts_qos_type_v01), wds_umts_qos_type_data_v01},
  {sizeof(wds_umts_qos_with_sig_ind_type_v01), wds_umts_qos_with_sig_ind_type_data_v01},
  {sizeof(wds_gprs_qos_type_v01), wds_gprs_qos_type_data_v01},
  {sizeof(wds_tft_id_param_type_v01), wds_tft_id_param_type_data_v01},
  {sizeof(wds_3gpp_lte_qos_params_v01), wds_3gpp_lte_qos_params_data_v01},
  {sizeof(wds_profile_identifier_type_v01), wds_profile_identifier_type_data_v01},
  {sizeof(wds_profile_info_type_v01), wds_profile_info_type_data_v01},
  {sizeof(wds_fqdn_type_v01), wds_fqdn_type_data_v01},
  {sizeof(wds_domain_name_list_type_v01), wds_domain_name_list_type_data_v01},
  {sizeof(wds_profile_id_family_type_v01), wds_profile_id_family_type_data_v01},
  {sizeof(wds_profile_identifier_with_family_type_v01), wds_profile_identifier_with_family_type_data_v01},
  {sizeof(wds_profile_param_type_v01), wds_profile_param_type_data_v01}
};

/* Message Table */
static const qmi_idl_message_table_entry wds_message_table_v01[] = {
  {0, 0},
  {sizeof(wds_reset_resp_msg_v01), wds_reset_resp_msg_data_v01},
  {sizeof(wds_set_event_report_req_msg_v01), wds_set_event_report_req_msg_data_v01},
  {sizeof(wds_set_event_report_resp_msg_v01), wds_set_event_report_resp_msg_data_v01},
  {sizeof(wds_event_report_ind_msg_v01), wds_event_report_ind_msg_data_v01},
  {sizeof(wds_abort_req_msg_v01), wds_abort_req_msg_data_v01},
  {sizeof(wds_abort_resp_msg_v01), wds_abort_resp_msg_data_v01},
  {sizeof(wds_start_network_interface_req_msg_v01), wds_start_network_interface_req_msg_data_v01},
  {sizeof(wds_start_network_interface_resp_msg_v01), wds_start_network_interface_resp_msg_data_v01},
  {sizeof(wds_stop_network_interface_req_msg_v01), wds_stop_network_interface_req_msg_data_v01},
  {sizeof(wds_stop_network_interface_resp_msg_v01), wds_stop_network_interface_resp_msg_data_v01},
  {0, 0},
  {sizeof(wds_get_pkt_srvc_status_resp_msg_v01), wds_get_pkt_srvc_status_resp_msg_data_v01},
  {sizeof(wds_pkt_srvc_status_ind_msg_v01), wds_pkt_srvc_status_ind_msg_data_v01},
  {0, 0},
  {sizeof(wds_get_current_channel_rate_resp_msg_v01), wds_get_current_channel_rate_resp_msg_data_v01},
  {sizeof(wds_get_pkt_statistics_req_msg_v01), wds_get_pkt_statistics_req_msg_data_v01},
  {sizeof(wds_get_pkt_statistics_resp_msg_v01), wds_get_pkt_statistics_resp_msg_data_v01},
  {0, 0},
  {sizeof(wds_go_dormant_resp_msg_v01), wds_go_dormant_resp_msg_data_v01},
  {0, 0},
  {sizeof(wds_go_active_resp_msg_v01), wds_go_active_resp_msg_data_v01},
  {sizeof(wds_create_profile_req_msg_v01), wds_create_profile_req_msg_data_v01},
  {sizeof(wds_create_profile_resp_msg_v01), wds_create_profile_resp_msg_data_v01},
  {sizeof(wds_modify_profile_settings_req_msg_v01), wds_modify_profile_settings_req_msg_data_v01},
  {sizeof(wds_modify_profile_settings_resp_msg_v01), wds_modify_profile_settings_resp_msg_data_v01},
  {sizeof(wds_delete_profile_req_msg_v01), wds_delete_profile_req_msg_data_v01},
  {sizeof(wds_delete_profile_resp_msg_v01), wds_delete_profile_resp_msg_data_v01},
  {sizeof(wds_get_profile_list_req_msg_v01), wds_get_profile_list_req_msg_data_v01},
  {sizeof(wds_get_profile_list_resp_msg_v01), wds_get_profile_list_resp_msg_data_v01},
  {sizeof(wds_get_profile_settings_req_msg_v01), wds_get_profile_settings_req_msg_data_v01},
  {sizeof(wds_get_profile_settings_resp_msg_v01), wds_get_profile_settings_resp_msg_data_v01},
  {sizeof(wds_get_default_settings_req_msg_v01), wds_get_default_settings_req_msg_data_v01},
  {sizeof(wds_get_default_settings_resp_msg_v01), wds_get_default_settings_resp_msg_data_v01},
  {sizeof(wds_get_runtime_settings_req_msg_v01), wds_get_runtime_settings_req_msg_data_v01},
  {sizeof(wds_get_runtime_settings_resp_msg_v01), wds_get_runtime_settings_resp_msg_data_v01},
  {sizeof(wds_set_mip_mode_req_msg_v01), wds_set_mip_mode_req_msg_data_v01},
  {sizeof(wds_set_mip_mode_resp_msg_v01), wds_set_mip_mode_resp_msg_data_v01},
  {0, 0},
  {sizeof(wds_get_mip_mode_resp_msg_v01), wds_get_mip_mode_resp_msg_data_v01},
  {0, 0},
  {sizeof(wds_get_dormancy_status_resp_msg_v01), wds_get_dormancy_status_resp_msg_data_v01},
  {0, 0},
  {sizeof(wds_get_call_duration_resp_msg_v01), wds_get_call_duration_resp_msg_data_v01},
  {0, 0},
  {sizeof(wds_get_current_data_bearer_technology_resp_msg_v01), wds_get_current_data_bearer_technology_resp_msg_data_v01},
  {sizeof(wds_get_default_profile_num_req_msg_v01), wds_get_default_profile_num_req_msg_data_v01},
  {sizeof(wds_get_default_profile_num_resp_msg_v01), wds_get_default_profile_num_resp_msg_data_v01},
  {sizeof(wds_set_default_profile_num_req_msg_v01), wds_set_default_profile_num_req_msg_data_v01},
  {sizeof(wds_set_default_profile_num_resp_msg_v01), wds_set_default_profile_num_resp_msg_data_v01},
  {sizeof(wds_reset_profile_to_default_req_msg_v01), wds_reset_profile_to_default_req_msg_data_v01},
  {sizeof(wds_reset_profile_to_default_resp_msg_v01), wds_reset_profile_to_default_resp_msg_data_v01},
  {sizeof(wds_reset_profile_param_to_invalid_req_msg_v01), wds_reset_profile_param_to_invalid_req_msg_data_v01},
  {sizeof(wds_reset_profile_param_to_invalid_resp_msg_v01), wds_reset_profile_param_to_invalid_resp_msg_data_v01},
  {sizeof(wds_set_client_ip_family_pref_req_msg_v01), wds_set_client_ip_family_pref_req_msg_data_v01},
  {sizeof(wds_set_client_ip_family_pref_resp_msg_v01), wds_set_client_ip_family_pref_resp_msg_data_v01}
};

/* Predefine the Type Table Object */
static const qmi_idl_type_table_object wds_qmi_idl_type_table_object_v01;

/*Referenced Tables Array*/
static const qmi_idl_type_table_object *wds_qmi_idl_type_table_object_referenced_tables_v01[] =
{&wds_qmi_idl_type_table_object_v01, &common_qmi_idl_type_table_object_v01};

/*Type Table Object*/
static const qmi_idl_type_table_object wds_qmi_idl_type_table_object_v01 = {
  sizeof(wds_type_table_v01)/sizeof(qmi_idl_type_table_entry ),
  sizeof(wds_message_table_v01)/sizeof(qmi_idl_message_table_entry),
  1,
  wds_type_table_v01,
  wds_message_table_v01,
  wds_qmi_idl_type_table_object_referenced_tables_v01
};

/*Arrays of service_message_table_entries for commands, responses and indications*/
static const qmi_idl_service_message_table_entry wds_service_command_messages_v01[] = {
  {QMI_WDS_RESET_REQ_V01, TYPE16(0, 0), 0},
  {QMI_WDS_SET_EVENT_REPORT_REQ_V01, TYPE16(0, 2), 24},
  {QMI_WDS_ABORT_REQ_V01, TYPE16(0, 5), 5},
  {QMI_WDS_START_NETWORK_INTERFACE_REQ_V01, TYPE16(0, 7), 838},
  {QMI_WDS_STOP_NETWORK_INTERFACE_REQ_V01, TYPE16(0, 9), 7},
  {QMI_WDS_GET_PKT_SRVC_STATUS_REQ_V01, TYPE16(0, 11), 0},
  {QMI_WDS_GET_CURRENT_CHANNEL_RATE_REQ_V01, TYPE16(0, 14), 0},
  {QMI_WDS_GET_PKT_STATISTICS_REQ_V01, TYPE16(0, 16), 7},
  {QMI_WDS_GO_DORMANT_REQ_V01, TYPE16(0, 18), 0},
  {QMI_WDS_GO_ACTIVE_REQ_V01, TYPE16(0, 20), 0},
  {QMI_WDS_CREATE_PROFILE_REQ_V01, TYPE16(0, 22), 1434},
  {QMI_WDS_MODIFY_PROFILE_SETTINGS_REQ_V01, TYPE16(0, 24), 1188},
  {QMI_WDS_DELETE_PROFILE_REQ_V01, TYPE16(0, 26), 5},
  {QMI_WDS_GET_PROFILE_LIST_REQ_V01, TYPE16(0, 28), 4},
  {QMI_WDS_GET_PROFILE_SETTINGS_REQ_V01, TYPE16(0, 30), 5},
  {QMI_WDS_GET_DEFAULT_SETTINGS_REQ_V01, TYPE16(0, 32), 4},
  {QMI_WDS_GET_RUNTIME_SETTINGS_REQ_V01, TYPE16(0, 34), 7},
  {QMI_WDS_SET_MIP_MODE_REQ_V01, TYPE16(0, 36), 4},
  {QMI_WDS_GET_MIP_MODE_REQ_V01, TYPE16(0, 38), 0},
  {QMI_WDS_GET_DORMANCY_STATUS_REQ_V01, TYPE16(0, 40), 0},
  {QMI_WDS_GET_CALL_DURATION_REQ_V01, TYPE16(0, 42), 0},
  {QMI_WDS_GET_CURRENT_DATA_BEARER_TECHNOLOGY_REQ_V01, TYPE16(0, 44), 0},
  {QMI_WDS_GET_DEFAULT_PROFILE_NUM_REQ_V01, TYPE16(0, 46), 5},
  {QMI_WDS_SET_DEFAULT_PROFILE_NUM_REQ_V01, TYPE16(0, 48), 6},
  {QMI_WDS_RESET_PROFILE_TO_DEFAULT_REQ_V01, TYPE16(0, 52), 9},
  {QMI_WDS_RESET_PROFILE_TO_DEFAULT_REQ_V01, TYPE16(0, 52), 9},
  {QMI_WDS_SET_CLIENT_IP_FAMILY_PREF_REQ_V01, TYPE16(0, 54), 4}
};

static const qmi_idl_service_message_table_entry wds_service_response_messages_v01[] = {
  {QMI_WDS_RESET_RESP_V01, TYPE16(0, 1), 7},
  {QMI_WDS_SET_EVENT_REPORT_RESP_V01, TYPE16(0, 3), 7},
  {QMI_WDS_ABORT_RESP_V01, TYPE16(0, 6), 7},
  {QMI_WDS_START_NETWORK_INTERFACE_RESP_V01, TYPE16(0, 8), 26},
  {QMI_WDS_STOP_NETWORK_INTERFACE_RESP_V01, TYPE16(0, 10), 7},
  {QMI_WDS_GET_PKT_SRVC_STATUS_RESP_V01, TYPE16(0, 12), 11},
  {QMI_WDS_GET_CURRENT_CHANNEL_RATE_RESP_V01, TYPE16(0, 15), 26},
  {QMI_WDS_GET_PKT_STATISTICS_RESP_V01, TYPE16(0, 17), 49},
  {QMI_WDS_GO_DORMANT_RESP_V01, TYPE16(0, 19), 7},
  {QMI_WDS_GO_ACTIVE_RESP_V01, TYPE16(0, 21), 7},
  {QMI_WDS_CREATE_PROFILE_RESP_V01, TYPE16(0, 23), 17},
  {QMI_WDS_MODIFY_PROFILE_SETTINGS_RESP_V01, TYPE16(0, 25), 12},
  {QMI_WDS_DELETE_PROFILE_RESP_V01, TYPE16(0, 27), 12},
  {QMI_WDS_GET_PROFILE_LIST_RESP_V01, TYPE16(0, 29), 2596},
  {QMI_WDS_GET_PROFILE_SETTINGS_RESP_V01, TYPE16(0, 31), 932},
  {QMI_WDS_GET_DEFAULT_SETTINGS_RESP_V01, TYPE16(0, 33), 932},
  {QMI_WDS_GET_RUNTIME_SETTINGS_RESP_V01, TYPE16(0, 35), 6763},
  {QMI_WDS_SET_MIP_MODE_RESP_V01, TYPE16(0, 37), 7},
  {QMI_WDS_GET_MIP_MODE_RESP_V01, TYPE16(0, 39), 11},
  {QMI_WDS_GET_DORMANCY_STATUS_RESP_V01, TYPE16(0, 41), 11},
  {QMI_WDS_GET_CALL_DURATION_RESP_V01, TYPE16(0, 43), 51},
  {QMI_WDS_GET_CURRENT_DATA_BEARER_TECHNOLOGY_RESP_V01, TYPE16(0, 45), 19},
  {QMI_WDS_GET_DEFAULT_PROFILE_NUM_RESP_V01, TYPE16(0, 47), 9},
  {QMI_WDS_SET_DEFAULT_PROFILE_NUM_RESP_V01, TYPE16(0, 49), 12},
  {QMI_WDS_RESET_PROFILE_TO_DEFAULT_RESP_V01, TYPE16(0, 53), 12},
  {QMI_WDS_RESET_PROFILE_TO_DEFAULT_RESP_V01, TYPE16(0, 53), 12},
  {QMI_WDS_SET_CLIENT_IP_FAMILY_PREF_RESP_V01, TYPE16(0, 55), 7}
};

static const qmi_idl_service_message_table_entry wds_service_indication_messages_v01[] = {
  {QMI_WDS_EVENT_REPORT_IND_V01, TYPE16(0, 4), 73},
  {QMI_WDS_PKT_SRVC_STATUS_IND_V01, TYPE16(0, 13), 17}
};

/*Service Object*/
const struct qmi_idl_service_object wds_qmi_idl_service_object_v01 = {
  02,
  01,
  0x01,
  0,
  { sizeof(wds_service_command_messages_v01)/sizeof(qmi_idl_service_message_table_entry),
    sizeof(wds_service_response_messages_v01)/sizeof(qmi_idl_service_message_table_entry),
    sizeof(wds_service_indication_messages_v01)/sizeof(qmi_idl_service_message_table_entry) },
  { wds_service_command_messages_v01, wds_service_response_messages_v01, wds_service_indication_messages_v01},
  &wds_qmi_idl_type_table_object_v01
};

/* Service Object Accessor */
qmi_idl_service_object_type wds_get_service_object_internal_v01
 ( int32_t idl_maj_version, int32_t idl_min_version, int32_t library_version ){
  if ( WDS_V01_IDL_MAJOR_VERS != idl_maj_version || WDS_V01_IDL_MINOR_VERS != idl_min_version 
       || WDS_V01_IDL_TOOL_VERS != library_version) 
  {
    return NULL;
  } 
  return (qmi_idl_service_object_type)&wds_qmi_idl_service_object_v01;
}

