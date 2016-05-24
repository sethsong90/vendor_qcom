/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        D E V I C E _ M A N A G E M E N T _ S E R V I C E _ V 0 1  . C

GENERAL DESCRIPTION
  This is the file which defines the dms service Data structures.

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
#include "device_management_service_v01.h"
#include "common_v01.h"


/*Type Definitions*/
static const uint8_t battery_lvl_limits_type_data_v01[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(battery_lvl_limits_type_v01, battery_lvl_lower_limit),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(battery_lvl_limits_type_v01, battery_lvl_upper_limit),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t power_state_type_data_v01[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(power_state_type_v01, power_status),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(power_state_type_v01, battery_lvl),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t pin_status_type_data_v01[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(pin_status_type_v01, status),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(pin_status_type_v01, verify_retries_left),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(pin_status_type_v01, unblock_retries_left),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t device_capabilities_type_data_v01[] = {
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(device_capabilities_type_v01, max_tx_channel_rate),

  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(device_capabilities_type_v01, max_rx_channel_rate),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(device_capabilities_type_v01, data_service_capability),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(device_capabilities_type_v01, sim_capability),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(device_capabilities_type_v01, radio_if_list),
  DMS_RADIO_IF_LIST_MAX_V01,
  QMI_IDL_OFFSET8(device_capabilities_type_v01, radio_if_list) - QMI_IDL_OFFSET8(device_capabilities_type_v01, radio_if_list_len),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t pin_protection_info_type_data_v01[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(pin_protection_info_type_v01, pin_id),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(pin_protection_info_type_v01, protection_setting),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(pin_protection_info_type_v01, pin_value),
  DMS_PIN_VALUE_MAX_V01,
  QMI_IDL_OFFSET8(pin_protection_info_type_v01, pin_value) - QMI_IDL_OFFSET8(pin_protection_info_type_v01, pin_value_len),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t pin_retries_status_type_data_v01[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(pin_retries_status_type_v01, verify_retries_left),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(pin_retries_status_type_v01, unblock_retries_left),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t pin_info_type_data_v01[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(pin_info_type_v01, pin_id),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(pin_info_type_v01, pin_value),
  DMS_PIN_VALUE_MAX_V01,
  QMI_IDL_OFFSET8(pin_info_type_v01, pin_value) - QMI_IDL_OFFSET8(pin_info_type_v01, pin_value_len),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t pin_unblock_info_type_data_v01[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(pin_unblock_info_type_v01, unblock_pin_id),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(pin_unblock_info_type_v01, puk_value),
  DMS_PUK_VALUE_MAX_V01,
  QMI_IDL_OFFSET8(pin_unblock_info_type_v01, puk_value) - QMI_IDL_OFFSET8(pin_unblock_info_type_v01, puk_value_len),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(pin_unblock_info_type_v01, new_pin_value),
  DMS_PUK_VALUE_MAX_V01,
  QMI_IDL_OFFSET8(pin_unblock_info_type_v01, new_pin_value) - QMI_IDL_OFFSET8(pin_unblock_info_type_v01, new_pin_value_len),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t pin_change_info_type_data_v01[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(pin_change_info_type_v01, pin_id),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(pin_change_info_type_v01, old_pin_value),
  DMS_PIN_VALUE_MAX_V01,
  QMI_IDL_OFFSET8(pin_change_info_type_v01, old_pin_value) - QMI_IDL_OFFSET8(pin_change_info_type_v01, old_pin_value_len),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(pin_change_info_type_v01, new_pin_value),
  DMS_PIN_VALUE_MAX_V01,
  QMI_IDL_OFFSET8(pin_change_info_type_v01, new_pin_value) - QMI_IDL_OFFSET8(pin_change_info_type_v01, new_pin_value_len),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t device_time_type_data_v01[] = {
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(device_time_type_v01, time_count),
  6,

  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(device_time_type_v01, time_source),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t user_lock_state_info_type_data_v01[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(user_lock_state_info_type_v01, lock_state),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(user_lock_state_info_type_v01, lock_code),
  4,

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t user_lock_set_info_type_data_v01[] = {
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(user_lock_set_info_type_v01, cur_code),
  4,

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(user_lock_set_info_type_v01, new_code),
  4,

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t facility_state_info_type_data_v01[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(facility_state_info_type_v01, facility_state),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(facility_state_info_type_v01, verify_reties_left),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(facility_state_info_type_v01, unblock_retries_left),

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t facility_set_ck_info_type_data_v01[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(facility_set_ck_info_type_v01, facility),

  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(facility_set_ck_info_type_v01, facility_state),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_STRING,
  QMI_IDL_OFFSET8(facility_set_ck_info_type_v01, facility_ck),
  DMS_FACILITY_CK_MAX_V01,

  QMI_IDL_FLAG_END_VALUE
};

static const uint8_t facility_unblock_info_type_data_v01[] = {
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(facility_unblock_info_type_v01, facility),

  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_STRING,
  QMI_IDL_OFFSET8(facility_unblock_info_type_v01, facility_unblock_ck),
  DMS_FACILITY_UNBLOCK_CK_MAX_V01,

  QMI_IDL_FLAG_END_VALUE
};

/*Message Definitions*/
/* 
 * dms_reset_req_msg is empty
 * static const uint8_t dms_reset_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t dms_reset_resp_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_reset_resp_msg_v01, resp),
  0, 1
};

static const uint8_t dms_set_event_report_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_set_event_report_req_msg_v01, report_power_state) - QMI_IDL_OFFSET8(dms_set_event_report_req_msg_v01, report_power_state_valid)),
  0x10,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(dms_set_event_report_req_msg_v01, report_power_state),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_set_event_report_req_msg_v01, lvl_limits) - QMI_IDL_OFFSET8(dms_set_event_report_req_msg_v01, lvl_limits_valid)),
  0x11,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_set_event_report_req_msg_v01, lvl_limits),
  0, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_set_event_report_req_msg_v01, report_pin_state) - QMI_IDL_OFFSET8(dms_set_event_report_req_msg_v01, report_pin_state_valid)),
  0x12,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(dms_set_event_report_req_msg_v01, report_pin_state),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_set_event_report_req_msg_v01, report_oprt_mode_state) - QMI_IDL_OFFSET8(dms_set_event_report_req_msg_v01, report_oprt_mode_state_valid)),
  0x14,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(dms_set_event_report_req_msg_v01, report_oprt_mode_state),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_set_event_report_req_msg_v01, report_uim_state) - QMI_IDL_OFFSET8(dms_set_event_report_req_msg_v01, report_uim_state_valid)),
  0x15,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(dms_set_event_report_req_msg_v01, report_uim_state)
};

static const uint8_t dms_set_event_report_resp_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_set_event_report_resp_msg_v01, resp),
  0, 1
};

static const uint8_t dms_event_report_ind_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_event_report_ind_msg_v01, power_state) - QMI_IDL_OFFSET8(dms_event_report_ind_msg_v01, power_state_valid)),
  0x10,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_event_report_ind_msg_v01, power_state),
  1, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_event_report_ind_msg_v01, pin1_status) - QMI_IDL_OFFSET8(dms_event_report_ind_msg_v01, pin1_status_valid)),
  0x11,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_event_report_ind_msg_v01, pin1_status),
  2, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_event_report_ind_msg_v01, pin2_status) - QMI_IDL_OFFSET8(dms_event_report_ind_msg_v01, pin2_status_valid)),
  0x12,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_event_report_ind_msg_v01, pin2_status),
  2, 0,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_event_report_ind_msg_v01, operating_mode) - QMI_IDL_OFFSET8(dms_event_report_ind_msg_v01, operating_mode_valid)),
  0x14,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(dms_event_report_ind_msg_v01, operating_mode),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_event_report_ind_msg_v01, uim_state) - QMI_IDL_OFFSET8(dms_event_report_ind_msg_v01, uim_state_valid)),
  0x15,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(dms_event_report_ind_msg_v01, uim_state)
};

/* 
 * dms_get_device_cap_req_msg is empty
 * static const uint8_t dms_get_device_cap_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t dms_get_device_cap_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_get_device_cap_resp_msg_v01, device_capabilities),
  3, 0,

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_get_device_cap_resp_msg_v01, resp),
  0, 1
};

/* 
 * dms_get_device_mfr_req_msg is empty
 * static const uint8_t dms_get_device_mfr_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t dms_get_device_mfr_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(dms_get_device_mfr_resp_msg_v01, device_manufacturer),
  ((DMS_DEVICE_MANUFACTURER_MAX_V01) & 0xFF), ((DMS_DEVICE_MANUFACTURER_MAX_V01) >> 8),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_get_device_mfr_resp_msg_v01, resp),
  0, 1
};

/* 
 * dms_get_device_model_id_req_msg is empty
 * static const uint8_t dms_get_device_model_id_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t dms_get_device_model_id_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(dms_get_device_model_id_resp_msg_v01, device_model_id),
  ((DMS_DEVICE_MODEL_ID_MAX_V01) & 0xFF), ((DMS_DEVICE_MODEL_ID_MAX_V01) >> 8),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_get_device_model_id_resp_msg_v01, resp),
  0, 1
};

/* 
 * dms_get_device_rev_id_req_msg is empty
 * static const uint8_t dms_get_device_rev_id_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t dms_get_device_rev_id_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(dms_get_device_rev_id_resp_msg_v01, device_rev_id),
  ((DMS_DEVICE_REV_ID_MAX_V01) & 0xFF), ((DMS_DEVICE_REV_ID_MAX_V01) >> 8),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_get_device_rev_id_resp_msg_v01, resp),
  0, 1
};

/* 
 * dms_get_msisdn_req_msg is empty
 * static const uint8_t dms_get_msisdn_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t dms_get_msisdn_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(dms_get_msisdn_resp_msg_v01, voice_number),
  DMS_VOICE_NUMBER_MAX_V01,

  0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_get_msisdn_resp_msg_v01, resp),
  0, 1,

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_get_msisdn_resp_msg_v01, mobile_id_number) - QMI_IDL_OFFSET8(dms_get_msisdn_resp_msg_v01, mobile_id_number_valid)),
  0x10,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(dms_get_msisdn_resp_msg_v01, mobile_id_number),
  DMS_MOBILE_ID_NUMBER_MAX_V01
};

/* 
 * dms_get_device_serial_numbers_req_msg is empty
 * static const uint8_t dms_get_device_serial_numbers_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t dms_get_device_serial_numbers_resp_msg_data_v01[] = {
  0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_get_device_serial_numbers_resp_msg_v01, resp),
  0, 1,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_get_device_serial_numbers_resp_msg_v01, esn) - QMI_IDL_OFFSET8(dms_get_device_serial_numbers_resp_msg_v01, esn_valid)),
  0x10,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(dms_get_device_serial_numbers_resp_msg_v01, esn),
  ((DMS_ESN_MAX_V01) & 0xFF), ((DMS_ESN_MAX_V01) >> 8),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_get_device_serial_numbers_resp_msg_v01, imei) - QMI_IDL_OFFSET8(dms_get_device_serial_numbers_resp_msg_v01, imei_valid)),
  0x11,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(dms_get_device_serial_numbers_resp_msg_v01, imei),
  ((DMS_IMEI_MAX_V01) & 0xFF), ((DMS_IMEI_MAX_V01) >> 8),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_get_device_serial_numbers_resp_msg_v01, meid) - QMI_IDL_OFFSET8(dms_get_device_serial_numbers_resp_msg_v01, meid_valid)),
  0x12,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(dms_get_device_serial_numbers_resp_msg_v01, meid),
  ((DMS_MEID_MAX_V01) & 0xFF), ((DMS_MEID_MAX_V01) >> 8)
};

/* 
 * dms_get_power_state_req_msg is empty
 * static const uint8_t dms_get_power_state_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t dms_get_power_state_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_get_power_state_resp_msg_v01, power_state),
  1, 0,

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_get_power_state_resp_msg_v01, resp),
  0, 1
};

static const uint8_t dms_uim_set_pin_protection_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_uim_set_pin_protection_req_msg_v01, pin_protection_info),
  4, 0
};

static const uint8_t dms_uim_set_pin_protection_resp_msg_data_v01[] = {
  0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_uim_set_pin_protection_resp_msg_v01, resp),
  0, 1,

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_uim_set_pin_protection_resp_msg_v01, pin_retries_status) - QMI_IDL_OFFSET8(dms_uim_set_pin_protection_resp_msg_v01, pin_retries_status_valid)),
  0x10,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_uim_set_pin_protection_resp_msg_v01, pin_retries_status),
  5, 0
};

static const uint8_t dms_uim_verify_pin_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_uim_verify_pin_req_msg_v01, pin_info),
  6, 0
};

static const uint8_t dms_uim_verify_pin_resp_msg_data_v01[] = {
  0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_uim_verify_pin_resp_msg_v01, resp),
  0, 1,

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_uim_verify_pin_resp_msg_v01, pin_retries_status) - QMI_IDL_OFFSET8(dms_uim_verify_pin_resp_msg_v01, pin_retries_status_valid)),
  0x10,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_uim_verify_pin_resp_msg_v01, pin_retries_status),
  5, 0
};

static const uint8_t dms_uim_unblock_pin_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_uim_unblock_pin_req_msg_v01, pin_unblock_info),
  7, 0
};

static const uint8_t dms_uim_unblock_pin_resp_msg_data_v01[] = {
  0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_uim_unblock_pin_resp_msg_v01, resp),
  0, 1,

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_uim_unblock_pin_resp_msg_v01, pin_retries_status) - QMI_IDL_OFFSET8(dms_uim_unblock_pin_resp_msg_v01, pin_retries_status_valid)),
  0x10,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_uim_unblock_pin_resp_msg_v01, pin_retries_status),
  5, 0
};

static const uint8_t dms_uim_change_pin_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_uim_change_pin_req_msg_v01, pin_change_info),
  8, 0
};

static const uint8_t dms_uim_change_pin_resp_msg_data_v01[] = {
  0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_uim_change_pin_resp_msg_v01, resp),
  0, 1,

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_uim_change_pin_resp_msg_v01, pin_retries_status) - QMI_IDL_OFFSET8(dms_uim_change_pin_resp_msg_v01, pin_retries_status_valid)),
  0x10,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_uim_change_pin_resp_msg_v01, pin_retries_status),
  5, 0
};

/* 
 * dms_uim_get_pin_status_req_msg is empty
 * static const uint8_t dms_uim_get_pin_status_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t dms_uim_get_pin_status_resp_msg_data_v01[] = {
  0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_uim_get_pin_status_resp_msg_v01, resp),
  0, 1,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_uim_get_pin_status_resp_msg_v01, pin1_status) - QMI_IDL_OFFSET8(dms_uim_get_pin_status_resp_msg_v01, pin1_status_valid)),
  0x11,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_uim_get_pin_status_resp_msg_v01, pin1_status),
  2, 0,

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_uim_get_pin_status_resp_msg_v01, pin2_status) - QMI_IDL_OFFSET8(dms_uim_get_pin_status_resp_msg_v01, pin2_status_valid)),
  0x12,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_uim_get_pin_status_resp_msg_v01, pin2_status),
  2, 0
};

/* 
 * dms_get_device_hardware_rev_req_msg is empty
 * static const uint8_t dms_get_device_hardware_rev_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t dms_get_device_hardware_rev_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(dms_get_device_hardware_rev_resp_msg_v01, hardware_rev),
  ((DMS_HARDWARE_REV_MAX_V01) & 0xFF), ((DMS_HARDWARE_REV_MAX_V01) >> 8),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_get_device_hardware_rev_resp_msg_v01, resp),
  0, 1
};

/* 
 * dms_get_operating_mode_req_msg is empty
 * static const uint8_t dms_get_operating_mode_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t dms_get_operating_mode_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(dms_get_operating_mode_resp_msg_v01, operating_mode),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_get_operating_mode_resp_msg_v01, resp),
  0, 1
};

static const uint8_t dms_set_operating_mode_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(dms_set_operating_mode_req_msg_v01, operating_mode)
};

static const uint8_t dms_set_operating_mode_resp_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_set_operating_mode_resp_msg_v01, resp),
  0, 1
};

/* 
 * dms_get_time_req_msg is empty
 * static const uint8_t dms_get_time_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t dms_get_time_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_get_time_resp_msg_v01, device_time),
  9, 0,

  0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_get_time_resp_msg_v01, resp),
  0, 1,

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_get_time_resp_msg_v01, sys_time_in_ms) - QMI_IDL_OFFSET8(dms_get_time_resp_msg_v01, sys_time_in_ms_valid)),
  0x10,
  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(dms_get_time_resp_msg_v01, sys_time_in_ms),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_get_time_resp_msg_v01, user_time_in_ms) - QMI_IDL_OFFSET8(dms_get_time_resp_msg_v01, user_time_in_ms_valid)),
  0x11,
  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(dms_get_time_resp_msg_v01, user_time_in_ms)
};

/* 
 * dms_get_prl_ver_req_msg is empty
 * static const uint8_t dms_get_prl_ver_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t dms_get_prl_ver_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(dms_get_prl_ver_resp_msg_v01, PRL_version),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_get_prl_ver_resp_msg_v01, resp),
  0, 1
};

/* 
 * dms_get_user_lock_state_req_msg is empty
 * static const uint8_t dms_get_user_lock_state_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t dms_get_user_lock_state_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(dms_get_user_lock_state_resp_msg_v01, lock_state),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_get_user_lock_state_resp_msg_v01, resp),
  0, 1
};

static const uint8_t dms_set_user_lock_state_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_set_user_lock_state_req_msg_v01, lock_info),
  10, 0
};

static const uint8_t dms_set_user_lock_state_resp_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_set_user_lock_state_resp_msg_v01, resp),
  0, 1
};

static const uint8_t dms_set_user_lock_code_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_set_user_lock_code_req_msg_v01, lock_info),
  11, 0
};

static const uint8_t dms_set_user_lock_code_resp_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_set_user_lock_code_resp_msg_v01, resp),
  0, 1
};

static const uint8_t dms_validate_service_programming_code_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_FLAGS_IS_ARRAY |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(dms_validate_service_programming_code_req_msg_v01, spc),
  6
};

static const uint8_t dms_validate_service_programming_code_resp_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_validate_service_programming_code_resp_msg_v01, resp),
  0, 1
};

/* 
 * dms_uim_get_iccid_req_msg is empty
 * static const uint8_t dms_uim_get_iccid_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t dms_uim_get_iccid_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(dms_uim_get_iccid_resp_msg_v01, uim_id),
  ((DMS_UIM_ID_MAX_V01) & 0xFF), ((DMS_UIM_ID_MAX_V01) >> 8),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_uim_get_iccid_resp_msg_v01, resp),
  0, 1
};

static const uint8_t dms_uim_get_ck_status_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(dms_uim_get_ck_status_req_msg_v01, facility)
};

static const uint8_t dms_uim_get_ck_status_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_uim_get_ck_status_resp_msg_v01, facility_info),
  12, 0,

  0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_uim_get_ck_status_resp_msg_v01, resp),
  0, 1,

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_uim_get_ck_status_resp_msg_v01, operation_blocking) - QMI_IDL_OFFSET8(dms_uim_get_ck_status_resp_msg_v01, operation_blocking_valid)),
  0x10,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(dms_uim_get_ck_status_resp_msg_v01, operation_blocking)
};

static const uint8_t dms_uim_set_ck_protection_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_uim_set_ck_protection_req_msg_v01, facility_set_ck_info),
  13, 0
};

static const uint8_t dms_uim_set_ck_protection_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(dms_uim_set_ck_protection_resp_msg_v01, verify_retries_left),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_uim_set_ck_protection_resp_msg_v01, resp),
  0, 1
};

static const uint8_t dms_uim_unblock_ck_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_uim_unblock_ck_req_msg_v01, facility_unblock_info),
  14, 0
};

static const uint8_t dms_uim_unblock_ck_resp_msg_data_v01[] = {
  0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_uim_unblock_ck_resp_msg_v01, resp),
  0, 1,

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_uim_unblock_ck_resp_msg_v01, unblock_retries_left) - QMI_IDL_OFFSET8(dms_uim_unblock_ck_resp_msg_v01, unblock_retries_left_valid)),
  0x10,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(dms_uim_unblock_ck_resp_msg_v01, unblock_retries_left)
};

/* 
 * dms_uim_get_imsi_req_msg is empty
 * static const uint8_t dms_uim_get_imsi_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t dms_uim_get_imsi_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_STRING,
  QMI_IDL_OFFSET8(dms_uim_get_imsi_resp_msg_v01, imsi),
  ((DMS_IMSI_MAX_V01) & 0xFF), ((DMS_IMSI_MAX_V01) >> 8),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_uim_get_imsi_resp_msg_v01, resp),
  0, 1
};

/* 
 * dms_uim_get_state_req_msg is empty
 * static const uint8_t dms_uim_get_state_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t dms_uim_get_state_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(dms_uim_get_state_resp_msg_v01, uim_state),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_uim_get_state_resp_msg_v01, resp),
  0, 1
};

/* 
 * dms_get_band_capability_req_msg is empty
 * static const uint8_t dms_get_band_capability_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t dms_get_band_capability_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(dms_get_band_capability_resp_msg_v01, band_capability),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_get_band_capability_resp_msg_v01, resp),
  0, 1
};

static const uint8_t dms_set_time_req_msg_data_v01[] = {
  0x01,
  QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(dms_set_time_req_msg_v01, time_in_ms),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(dms_set_time_req_msg_v01, time_reference_type) - QMI_IDL_OFFSET8(dms_set_time_req_msg_v01, time_reference_type_valid)),
  0x10,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(dms_set_time_req_msg_v01, time_reference_type)
};

static const uint8_t dms_set_time_resp_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(dms_set_time_resp_msg_v01, resp),
  0, 1
};

/* Type Table */
static const qmi_idl_type_table_entry  dms_type_table_v01[] = {
  {sizeof(battery_lvl_limits_type_v01), battery_lvl_limits_type_data_v01},
  {sizeof(power_state_type_v01), power_state_type_data_v01},
  {sizeof(pin_status_type_v01), pin_status_type_data_v01},
  {sizeof(device_capabilities_type_v01), device_capabilities_type_data_v01},
  {sizeof(pin_protection_info_type_v01), pin_protection_info_type_data_v01},
  {sizeof(pin_retries_status_type_v01), pin_retries_status_type_data_v01},
  {sizeof(pin_info_type_v01), pin_info_type_data_v01},
  {sizeof(pin_unblock_info_type_v01), pin_unblock_info_type_data_v01},
  {sizeof(pin_change_info_type_v01), pin_change_info_type_data_v01},
  {sizeof(device_time_type_v01), device_time_type_data_v01},
  {sizeof(user_lock_state_info_type_v01), user_lock_state_info_type_data_v01},
  {sizeof(user_lock_set_info_type_v01), user_lock_set_info_type_data_v01},
  {sizeof(facility_state_info_type_v01), facility_state_info_type_data_v01},
  {sizeof(facility_set_ck_info_type_v01), facility_set_ck_info_type_data_v01},
  {sizeof(facility_unblock_info_type_v01), facility_unblock_info_type_data_v01}
};

/* Message Table */
static const qmi_idl_message_table_entry dms_message_table_v01[] = {
  {0, 0},
  {sizeof(dms_reset_resp_msg_v01), dms_reset_resp_msg_data_v01},
  {sizeof(dms_set_event_report_req_msg_v01), dms_set_event_report_req_msg_data_v01},
  {sizeof(dms_set_event_report_resp_msg_v01), dms_set_event_report_resp_msg_data_v01},
  {sizeof(dms_event_report_ind_msg_v01), dms_event_report_ind_msg_data_v01},
  {0, 0},
  {sizeof(dms_get_device_cap_resp_msg_v01), dms_get_device_cap_resp_msg_data_v01},
  {0, 0},
  {sizeof(dms_get_device_mfr_resp_msg_v01), dms_get_device_mfr_resp_msg_data_v01},
  {0, 0},
  {sizeof(dms_get_device_model_id_resp_msg_v01), dms_get_device_model_id_resp_msg_data_v01},
  {0, 0},
  {sizeof(dms_get_device_rev_id_resp_msg_v01), dms_get_device_rev_id_resp_msg_data_v01},
  {0, 0},
  {sizeof(dms_get_msisdn_resp_msg_v01), dms_get_msisdn_resp_msg_data_v01},
  {0, 0},
  {sizeof(dms_get_device_serial_numbers_resp_msg_v01), dms_get_device_serial_numbers_resp_msg_data_v01},
  {0, 0},
  {sizeof(dms_get_power_state_resp_msg_v01), dms_get_power_state_resp_msg_data_v01},
  {sizeof(dms_uim_set_pin_protection_req_msg_v01), dms_uim_set_pin_protection_req_msg_data_v01},
  {sizeof(dms_uim_set_pin_protection_resp_msg_v01), dms_uim_set_pin_protection_resp_msg_data_v01},
  {sizeof(dms_uim_verify_pin_req_msg_v01), dms_uim_verify_pin_req_msg_data_v01},
  {sizeof(dms_uim_verify_pin_resp_msg_v01), dms_uim_verify_pin_resp_msg_data_v01},
  {sizeof(dms_uim_unblock_pin_req_msg_v01), dms_uim_unblock_pin_req_msg_data_v01},
  {sizeof(dms_uim_unblock_pin_resp_msg_v01), dms_uim_unblock_pin_resp_msg_data_v01},
  {sizeof(dms_uim_change_pin_req_msg_v01), dms_uim_change_pin_req_msg_data_v01},
  {sizeof(dms_uim_change_pin_resp_msg_v01), dms_uim_change_pin_resp_msg_data_v01},
  {0, 0},
  {sizeof(dms_uim_get_pin_status_resp_msg_v01), dms_uim_get_pin_status_resp_msg_data_v01},
  {0, 0},
  {sizeof(dms_get_device_hardware_rev_resp_msg_v01), dms_get_device_hardware_rev_resp_msg_data_v01},
  {0, 0},
  {sizeof(dms_get_operating_mode_resp_msg_v01), dms_get_operating_mode_resp_msg_data_v01},
  {sizeof(dms_set_operating_mode_req_msg_v01), dms_set_operating_mode_req_msg_data_v01},
  {sizeof(dms_set_operating_mode_resp_msg_v01), dms_set_operating_mode_resp_msg_data_v01},
  {0, 0},
  {sizeof(dms_get_time_resp_msg_v01), dms_get_time_resp_msg_data_v01},
  {0, 0},
  {sizeof(dms_get_prl_ver_resp_msg_v01), dms_get_prl_ver_resp_msg_data_v01},
  {0, 0},
  {sizeof(dms_get_user_lock_state_resp_msg_v01), dms_get_user_lock_state_resp_msg_data_v01},
  {sizeof(dms_set_user_lock_state_req_msg_v01), dms_set_user_lock_state_req_msg_data_v01},
  {sizeof(dms_set_user_lock_state_resp_msg_v01), dms_set_user_lock_state_resp_msg_data_v01},
  {sizeof(dms_set_user_lock_code_req_msg_v01), dms_set_user_lock_code_req_msg_data_v01},
  {sizeof(dms_set_user_lock_code_resp_msg_v01), dms_set_user_lock_code_resp_msg_data_v01},
  {sizeof(dms_validate_service_programming_code_req_msg_v01), dms_validate_service_programming_code_req_msg_data_v01},
  {sizeof(dms_validate_service_programming_code_resp_msg_v01), dms_validate_service_programming_code_resp_msg_data_v01},
  {0, 0},
  {sizeof(dms_uim_get_iccid_resp_msg_v01), dms_uim_get_iccid_resp_msg_data_v01},
  {sizeof(dms_uim_get_ck_status_req_msg_v01), dms_uim_get_ck_status_req_msg_data_v01},
  {sizeof(dms_uim_get_ck_status_resp_msg_v01), dms_uim_get_ck_status_resp_msg_data_v01},
  {sizeof(dms_uim_set_ck_protection_req_msg_v01), dms_uim_set_ck_protection_req_msg_data_v01},
  {sizeof(dms_uim_set_ck_protection_resp_msg_v01), dms_uim_set_ck_protection_resp_msg_data_v01},
  {sizeof(dms_uim_unblock_ck_req_msg_v01), dms_uim_unblock_ck_req_msg_data_v01},
  {sizeof(dms_uim_unblock_ck_resp_msg_v01), dms_uim_unblock_ck_resp_msg_data_v01},
  {0, 0},
  {sizeof(dms_uim_get_imsi_resp_msg_v01), dms_uim_get_imsi_resp_msg_data_v01},
  {0, 0},
  {sizeof(dms_uim_get_state_resp_msg_v01), dms_uim_get_state_resp_msg_data_v01},
  {0, 0},
  {sizeof(dms_get_band_capability_resp_msg_v01), dms_get_band_capability_resp_msg_data_v01},
  {sizeof(dms_set_time_req_msg_v01), dms_set_time_req_msg_data_v01},
  {sizeof(dms_set_time_resp_msg_v01), dms_set_time_resp_msg_data_v01}
};

/* Predefine the Type Table Object */
static const qmi_idl_type_table_object dms_qmi_idl_type_table_object_v01;

/*Referenced Tables Array*/
static const qmi_idl_type_table_object *dms_qmi_idl_type_table_object_referenced_tables_v01[] =
{&dms_qmi_idl_type_table_object_v01, &common_qmi_idl_type_table_object_v01};

/*Type Table Object*/
static const qmi_idl_type_table_object dms_qmi_idl_type_table_object_v01 = {
  sizeof(dms_type_table_v01)/sizeof(qmi_idl_type_table_entry ),
  sizeof(dms_message_table_v01)/sizeof(qmi_idl_message_table_entry),
  1,
  dms_type_table_v01,
  dms_message_table_v01,
  dms_qmi_idl_type_table_object_referenced_tables_v01
};

/*Arrays of service_message_table_entries for commands, responses and indications*/
static const qmi_idl_service_message_table_entry dms_service_command_messages_v01[] = {
  {QMI_DMS_RESET_REQ_V01, TYPE16(0, 0), 0},
  {QMI_DMS_SET_EVENT_REPORT_REQ_V01, TYPE16(0, 2), 21},
  {QMI_DMS_GET_DEVICE_CAP_REQ_V01, TYPE16(0, 5), 0},
  {QMI_DMS_GET_DEVICE_MFR_REQ_V01, TYPE16(0, 7), 0},
  {QMI_DMS_GET_DEVICE_MODEL_ID_REQ_V01, TYPE16(0, 9), 0},
  {QMI_DMS_GET_DEVICE_REV_ID_REQ_V01, TYPE16(0, 11), 0},
  {QMI_DMS_GET_MSISDN_REQ_V01, TYPE16(0, 13), 0},
  {QMI_DMS_GET_DEVICE_SERIAL_NUMBERS_REQ_V01, TYPE16(0, 15), 0},
  {QMI_DMS_GET_POWER_STATE_REQ_V01, TYPE16(0, 17), 0},
  {QMI_DMS_UIM_SET_PIN_PROTECTION_REQ_V01, TYPE16(0, 19), 261},
  {QMI_DMS_UIM_VERIFY_PIN_REQ_V01, TYPE16(0, 21), 260},
  {QMI_DMS_UIM_UNBLOCK_PIN_REQ_V01, TYPE16(0, 23), 516},
  {QMI_DMS_UIM_CHANGE_PIN_REQ_V01, TYPE16(0, 25), 516},
  {QMI_DMS_UIM_GET_PIN_STATUS_REQ_V01, TYPE16(0, 27), 0},
  {QMI_DMS_GET_DEVICE_HARDWARE_REV_REQ_V01, TYPE16(0, 29), 0},
  {QMI_DMS_GET_OPERATING_MODE_REQ_V01, TYPE16(0, 31), 0},
  {QMI_DMS_SET_OPERATING_MODE_REQ_V01, TYPE16(0, 33), 4},
  {QMI_DMS_GET_TIME_REQ_V01, TYPE16(0, 35), 0},
  {QMI_DMS_GET_PRL_VER_REQ_V01, TYPE16(0, 37), 0},
  {QMI_DMS_GET_USER_LOCK_STATE_REQ_V01, TYPE16(0, 39), 0},
  {QMI_DMS_SET_USER_LOCK_STATE_REQ_V01, TYPE16(0, 41), 8},
  {QMI_DMS_SET_USER_LOCK_CODE_REQ_V01, TYPE16(0, 43), 11},
  {QMI_DMS_VALIDATE_SERVICE_PROGRAMMING_CODE_REQ_V01, TYPE16(0, 45), 9},
  {QMI_DMS_UIM_GET_ICCID_REQ_V01, TYPE16(0, 47), 0},
  {QMI_DMS_UIM_GET_CK_STATUS_REQ_V01, TYPE16(0, 49), 4},
  {QMI_DMS_UIM_SET_CK_PROTECTION_REQ_V01, TYPE16(0, 51), 14},
  {QMI_DMS_UIM_UNBLOCK_CK_REQ_V01, TYPE16(0, 53), 260},
  {QMI_DMS_UIM_GET_IMSI_REQ_V01, TYPE16(0, 55), 0},
  {QMI_DMS_UIM_GET_STATE_REQ_V01, TYPE16(0, 57), 0},
  {QMI_DMS_GET_BAND_CAPABILITY_REQ_V01, TYPE16(0, 59), 0},
  {QMI_DMS_SET_TIME_REQ_V01, TYPE16(0, 61), 18}
};

static const qmi_idl_service_message_table_entry dms_service_response_messages_v01[] = {
  {QMI_DMS_RESET_RESP_V01, TYPE16(0, 1), 7},
  {QMI_DMS_SET_EVENT_REPORT_RESP_V01, TYPE16(0, 3), 7},
  {QMI_DMS_GET_DEVICE_CAP_RESP_V01, TYPE16(0, 6), 276},
  {QMI_DMS_GET_DEVICE_MFR_RESP_V01, TYPE16(0, 8), 266},
  {QMI_DMS_GET_DEVICE_MODEL_ID_RESP_V01, TYPE16(0, 10), 266},
  {QMI_DMS_GET_DEVICE_REV_ID_RESP_V01, TYPE16(0, 12), 266},
  {QMI_DMS_GET_MSISDN_RESP_V01, TYPE16(0, 14), 523},
  {QMI_DMS_GET_DEVICE_SERIAL_NUMBERS_RESP_V01, TYPE16(0, 16), 784},
  {QMI_DMS_GET_POWER_STATE_RESP_V01, TYPE16(0, 18), 12},
  {QMI_DMS_UIM_SET_PIN_PROTECTION_RESP_V01, TYPE16(0, 20), 12},
  {QMI_DMS_UIM_VERIFY_PIN_RESP_V01, TYPE16(0, 22), 12},
  {QMI_DMS_UIM_UNBLOCK_PIN_RESP_V01, TYPE16(0, 24), 12},
  {QMI_DMS_UIM_CHANGE_PIN_RESP_V01, TYPE16(0, 26), 12},
  {QMI_DMS_UIM_GET_PIN_STATUS_RESP_V01, TYPE16(0, 28), 19},
  {QMI_DMS_GET_DEVICE_HARDWARE_REV_RESP_V01, TYPE16(0, 30), 266},
  {QMI_DMS_GET_OPERATING_MODE_RESP_V01, TYPE16(0, 32), 11},
  {QMI_DMS_SET_OPERATING_MODE_RESP_V01, TYPE16(0, 34), 7},
  {QMI_DMS_GET_TIME_RESP_V01, TYPE16(0, 36), 40},
  {QMI_DMS_GET_PRL_VER_RESP_V01, TYPE16(0, 38), 12},
  {QMI_DMS_GET_USER_LOCK_STATE_RESP_V01, TYPE16(0, 40), 11},
  {QMI_DMS_SET_USER_LOCK_STATE_RESP_V01, TYPE16(0, 42), 7},
  {QMI_DMS_SET_USER_LOCK_CODE_RESP_V01, TYPE16(0, 44), 7},
  {QMI_DMS_VALIDATE_SERVICE_PROGRAMMING_CODE_RESP_V01, TYPE16(0, 46), 7},
  {QMI_DMS_UIM_GET_ICCID_RESP_V01, TYPE16(0, 48), 266},
  {QMI_DMS_UIM_GET_CK_STATUS_RESP_V01, TYPE16(0, 50), 17},
  {QMI_DMS_UIM_SET_CK_PROTECTION_RESP_V01, TYPE16(0, 52), 11},
  {QMI_DMS_UIM_UNBLOCK_CK_RESP_V01, TYPE16(0, 54), 11},
  {QMI_DMS_UIM_GET_IMSI_RESP_V01, TYPE16(0, 56), 266},
  {QMI_DMS_UIM_GET_STATE_RESP_V01, TYPE16(0, 58), 11},
  {QMI_DMS_GET_BAND_CAPABILITY_RESP_V01, TYPE16(0, 60), 18},
  {QMI_DMS_SET_TIME_RESP_V01, TYPE16(0, 62), 7}
};

static const qmi_idl_service_message_table_entry dms_service_indication_messages_v01[] = {
  {QMI_DMS_EVENT_REPORT_IND_V01, TYPE16(0, 4), 25}
};

/*Service Object*/
const struct qmi_idl_service_object dms_qmi_idl_service_object_v01 = {
  02,
  01,
  0x02,
  0,
  { sizeof(dms_service_command_messages_v01)/sizeof(qmi_idl_service_message_table_entry),
    sizeof(dms_service_response_messages_v01)/sizeof(qmi_idl_service_message_table_entry),
    sizeof(dms_service_indication_messages_v01)/sizeof(qmi_idl_service_message_table_entry) },
  { dms_service_command_messages_v01, dms_service_response_messages_v01, dms_service_indication_messages_v01},
  &dms_qmi_idl_type_table_object_v01
};

/* Service Object Accessor */
qmi_idl_service_object_type dms_get_service_object_internal_v01
 ( int32_t idl_maj_version, int32_t idl_min_version, int32_t library_version ){
  if ( DMS_V01_IDL_MAJOR_VERS != idl_maj_version || DMS_V01_IDL_MINOR_VERS != idl_min_version 
       || DMS_V01_IDL_TOOL_VERS != library_version) 
  {
    return NULL;
  } 
  return (qmi_idl_service_object_type)&dms_qmi_idl_service_object_v01;
}

