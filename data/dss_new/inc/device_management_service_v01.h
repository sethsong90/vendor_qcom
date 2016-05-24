#ifndef DMS_SERVICE_H
#define DMS_SERVICE_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        D E V I C E _ M A N A G E M E N T _ S E R V I C E _ V 0 1  . H

GENERAL DESCRIPTION
  This is the public header file which defines the dms service Data structures.

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
#define DMS_V01_IDL_MAJOR_VERS 01
#define DMS_V01_IDL_MINOR_VERS 03
#define DMS_V01_IDL_TOOL_VERS 02

/* Const Definitions */

#define DMS_RADIO_IF_LIST_MAX_V01 255
#define DMS_DEVICE_MANUFACTURER_MAX_V01 256
#define DMS_DEVICE_MODEL_ID_MAX_V01 256
#define DMS_DEVICE_REV_ID_MAX_V01 256
#define DMS_VOICE_NUMBER_MAX_V01 255
#define DMS_MOBILE_ID_NUMBER_MAX_V01 255
#define DMS_ESN_MAX_V01 256
#define DMS_IMEI_MAX_V01 256
#define DMS_MEID_MAX_V01 256
#define DMS_PIN_VALUE_MAX_V01 255
#define DMS_PUK_VALUE_MAX_V01 255
#define DMS_HARDWARE_REV_MAX_V01 256
#define DMS_UIM_ID_MAX_V01 256
#define DMS_FACILITY_CK_MAX_V01 8
#define DMS_FACILITY_UNBLOCK_CK_MAX_V01 255
#define DMS_IMSI_MAX_V01 256

/*
 * dms_reset_req_msg is empty
 * typedef struct {
 * }dms_reset_req_msg_v01;
 */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}dms_reset_resp_msg_v01;	/* Message */

typedef struct {
  uint8_t battery_lvl_lower_limit;	/*  Battery level is reported to the control point if the battery
       level falls below this lower limit (specified as percentage of
       remaining battery power from 0 to 100)
   */

  uint8_t battery_lvl_upper_limit;	/*  Battery level is reported to the control point if the battery
       level rises above the upper limit (specified as percentage of
       remaining battery power from 0 to 100)
    */
}battery_lvl_limits_type_v01;	/* Type */

typedef struct {
  /* Optional */
  /*   Power state reporting */
  uint8_t report_power_state_valid;	/* Must be set to true if report_power_state is being passed */
  uint8_t report_power_state;	/*  Valid values are
       - FALSE - Do not report
       - TRUE - Report on change in Power state
   */

  /* Optional */
  /*  Battery level report limits */
  uint8_t lvl_limits_valid;	/* Must be set to true if lvl_limits is being passed */
  battery_lvl_limits_type_v01 lvl_limits;

  /* Optional */
  /*  PIN state reporting */
  uint8_t report_pin_state_valid;	/* Must be set to true if report_pin_state is being passed */
  uint8_t report_pin_state;	/*  Valid values are:
       - FALSE - Do not report
       - TRUE - Report on change in PIN state
   */

  /* Optional */
  /*  Operating mode reporting */
  uint8_t report_oprt_mode_state_valid;	/* Must be set to true if report_oprt_mode_state is being passed */
  uint8_t report_oprt_mode_state;	/*  Valid values are:
       - FALSE - Do not report operating mode changes
       - TRUE - Report operating mode changes     
   */

  /* Optional */
  /*  UIM state reporting */
  uint8_t report_uim_state_valid;	/* Must be set to true if report_uim_state is being passed */
  uint8_t report_uim_state;	/*  Valid values are:
       - FALSE - Do not report UIM state changes
       - TRUE - Report UIM state changes
   */
}dms_set_event_report_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}dms_set_event_report_resp_msg_v01;	/* Message */

typedef struct {
  uint8_t power_status;	/*  Power status flags are:

       Bit 0 - Power source; valid values are:
       - 0 - Powered by battery
       - 1 - Powered by external source

       Bit 1 - Battery connected; valid values are:
       - 0 - Battery not connected
       - 1 - Battery connected

       Bit 2 - Battery charging; valid values are:
       - 0 - Not charging
       - 1 - Charging

       Bit 3 - Power fault; valid values are:
       - 0 - No power fault
       - 1 - Recognized power fault, calls inhibited
   */

  uint8_t battery_lvl;	/*  Level of the battery; valid values are:
       - 0x00 - Battery is exhausted or the mobile does not have a
                battery connected
       - 1-100 (0x64) - Percentage of battery capacity remaining
   */
}power_state_type_v01;	/* Type */

typedef struct {
  uint8_t status;	/*  Current status of the PIN; valid values are:
       - 0 - PIN not initialized
       - 1 - PIN enabled, not verified
       - 2 - PIN enabled, verified
       - 3 - PIN disabled
       - 4 - PIN blocked
       - 5 - PIN permanently blocked
       - 6 - PIN unblocked
       - 7 - PIN changed
   */

  uint8_t verify_retries_left;	/*  Number of retries left, after which the PIN will be blocked  */

  uint8_t unblock_retries_left;	/*  Number of unblock retries left, after which the PIN will be
       permanently blocked, i.e., UIM is unusable
   */
}pin_status_type_v01;	/* Type */

typedef struct {
  /* Optional */
  /*  Power state */
  uint8_t power_state_valid;	/* Must be set to true if power_state is being passed */
  power_state_type_v01 power_state;

  /* Optional */
  /*  PIN 1 status */
  uint8_t pin1_status_valid;	/* Must be set to true if pin1_status is being passed */
  pin_status_type_v01 pin1_status;

  /* Optional */
  /*  PIN 2 status */
  uint8_t pin2_status_valid;	/* Must be set to true if pin2_status is being passed */
  pin_status_type_v01 pin2_status;

  /* Optional */
  /*  Operating mode */
  uint8_t operating_mode_valid;	/* Must be set to true if operating_mode is being passed */
  uint8_t operating_mode;	/*  Current operating mode
       - 0x00 - Online
       - 0x01 - Low power
       - 0x02 - Factory test mode
       - 0x03 - Offline
       - 0x04 - Resetting
       - 0x05 - Shutting down
   */

  /* Optional */
  /*  UIM state */
  uint8_t uim_state_valid;	/* Must be set to true if uim_state is being passed */
  uint8_t uim_state;	/*  UIM state
       - 0x00 - UIM initialization completed
       - 0x01 - UIM initialization failed
       - 0x02 - UIM not present
   */
}dms_event_report_ind_msg_v01;	/* Message */

/*
 * dms_get_device_cap_req_msg is empty
 * typedef struct {
 * }dms_get_device_cap_req_msg_v01;
 */

typedef struct {
  uint32_t max_tx_channel_rate;	/*  Maximum Tx transmission rate in bits per second (bps) supported
       by the device; the value 0xFFFFFFFF implies a rate >= 0xFFFFFFFF
       (4 Gbps); in multitechnology devices, this value will be the
       greatest rate among all supported technologies
   */

  uint32_t max_rx_channel_rate;	/*  Maximum Rx transmission rate in bits per second (bps) supported
       by the device; the value 0xFFFFFFFF implies rate >= 0xFFFFFFFF
       (4 * Gbps); in multitechnology devices, this value will be the
       greatest rate among all supported technologies
   */

  uint8_t data_service_capability;	/*  Valid values are:
       - 0x00 - No data services supported
       - 0x01 - Only circuit switched (CS) services are supported
       - 0x02 - Only packet switched (PS) services are supported
       - 0x03 - Simultaneous CS and PS
       - 0x04 - Nonsimultaneous CS and PS
   */

  uint8_t sim_capability;	/*  Valid values are:
       - 0x01 - SIM not supported
       - 0x02 - SIM supported
   */

  uint32_t radio_if_list_len;	/* Must be set to # of elements in radio_if_list */
  uint8_t radio_if_list[DMS_RADIO_IF_LIST_MAX_V01];	/*  List of N 1-byte elements describing the radio interfaces
       supported by the device; values are:
       - 0x01 - CDMA2000 1X
       - 0x02 - CDMA2000 HRPD (1xEV-DO)
       - 0x04 - GSM
       - 0x05 - UMTS
   */
}device_capabilities_type_v01;	/* Type */

typedef struct {
  /* Mandatory */
  /*  Device Capabilites */
  device_capabilities_type_v01 device_capabilities;

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}dms_get_device_cap_resp_msg_v01;	/* Message */

/*
 * dms_get_device_mfr_req_msg is empty
 * typedef struct {
 * }dms_get_device_mfr_req_msg_v01;
 */

typedef struct {
  /* Mandatory */
  /*  Device manufacturer */
  char device_manufacturer[DMS_DEVICE_MANUFACTURER_MAX_V01 + 1];	/*  String identifying the device manufacturer  */

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}dms_get_device_mfr_resp_msg_v01;	/* Message */

/*
 * dms_get_device_model_id_req_msg is empty
 * typedef struct {
 * }dms_get_device_model_id_req_msg_v01;
 */

typedef struct {
  /* Mandatory */
  /*  Device model */
  char device_model_id[DMS_DEVICE_MODEL_ID_MAX_V01 + 1];	/*  String identifying the device model  */

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}dms_get_device_model_id_resp_msg_v01;	/* Message */

/*
 * dms_get_device_rev_id_req_msg is empty
 * typedef struct {
 * }dms_get_device_rev_id_req_msg_v01;
 */

typedef struct {
  /* Mandatory */
  /*  Revision ID */
  char device_rev_id[DMS_DEVICE_REV_ID_MAX_V01 + 1];	/*  String containing the device revision ID  */

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}dms_get_device_rev_id_resp_msg_v01;	/* Message */

/*
 * dms_get_msisdn_req_msg is empty
 * typedef struct {
 * }dms_get_msisdn_req_msg_v01;
 */

typedef struct {
  /* Mandatory */
  /*  Voice number */
  char voice_number[DMS_VOICE_NUMBER_MAX_V01 + 1];	/*  String containing the voice number in use by the device  */

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type. */

  /* Optional */
  /*  Mobile ID */
  uint8_t mobile_id_number_valid;	/* Must be set to true if mobile_id_number is being passed */
  char mobile_id_number[DMS_MOBILE_ID_NUMBER_MAX_V01 + 1];	/*  String containing the Mobile ID Number of the device  */
}dms_get_msisdn_resp_msg_v01;	/* Message */

/*
 * dms_get_device_serial_numbers_req_msg is empty
 * typedef struct {
 * }dms_get_device_serial_numbers_req_msg_v01;
 */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */

  /* Optional */
  /*  ESN */
  uint8_t esn_valid;	/* Must be set to true if esn is being passed */
  char esn[DMS_ESN_MAX_V01 + 1];	/*  String containing the Electronic Serial Number (ESN) of the device  */

  /* Optional */
  /*  IMEI */
  uint8_t imei_valid;	/* Must be set to true if imei is being passed */
  char imei[DMS_IMEI_MAX_V01 + 1];	/*  String containing the International Mobile Equipment Identity
      (IMEI) of the device
   */

  /* Optional */
  /*  MEID */
  uint8_t meid_valid;	/* Must be set to true if meid is being passed */
  char meid[DMS_MEID_MAX_V01 + 1];	/*  String containing the Mobile Equipment Identifier (MEID) of the device  */
}dms_get_device_serial_numbers_resp_msg_v01;	/* Message */

/*
 * dms_get_power_state_req_msg is empty
 * typedef struct {
 * }dms_get_power_state_req_msg_v01;
 */

typedef struct {
  /* Mandatory */
  /*  Power state */
  power_state_type_v01 power_state;

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}dms_get_power_state_resp_msg_v01;	/* Message */

typedef struct {
  uint8_t pin_id;	/*  Specifies the ID of the PIN to be enabled or disabled; valid values are:
       - 1 - PIN1 (also called PIN)
       - 2 - PIN2
   */

  uint8_t protection_setting;	/*  Specifies whether the PIN is enabled; valid values are:
     - TRUE - Enable PIN
     - FALSE - Disable PIN
   */

  uint32_t pin_value_len;	/* Must be set to # of elements in pin_value */
  uint8_t pin_value[DMS_PIN_VALUE_MAX_V01];	/*  Specifies the PIN value of the PIN to be enabled/disabled; the
       protection setting will only be changed if this value is
       successfully verified by the SIM
   */
}pin_protection_info_type_v01;	/* Type */

typedef struct {
  /* Mandatory */
  /*  PIN protection information */
  pin_protection_info_type_v01 pin_protection_info;
}dms_uim_set_pin_protection_req_msg_v01;	/* Message */

typedef struct {
  uint8_t verify_retries_left;	/*  Number of retries left, after which the PIN will be blocked  */

  uint8_t unblock_retries_left;	/*  Number of unblock retries left, after which the PIN will be
       permanently blocked, i.e., UIM is unusable
   */
}pin_retries_status_type_v01;	/* Type */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */

  /* Optional */
  /*  Retries left; returned if the enable/disable operation failed */
  uint8_t pin_retries_status_valid;	/* Must be set to true if pin_retries_status is being passed */
  pin_retries_status_type_v01 pin_retries_status;
}dms_uim_set_pin_protection_resp_msg_v01;	/* Message */

typedef struct {
  uint8_t pin_id;	/*  Specifies the ID of the PIN to be enabled or disabled; valid values are:
       - 1 - PIN1 (also called PIN)
       - 2 - PIN2
   */

  uint32_t pin_value_len;	/* Must be set to # of elements in pin_value */
  uint8_t pin_value[DMS_PIN_VALUE_MAX_V01];	/*  Specifies the PIN value of the PIN to be enabled/disabled; the
       protection setting will only be changed if this value is
       successfully verified by the SIM
   */
}pin_info_type_v01;	/* Type */

typedef struct {
  /* Mandatory */
  /*  PIN value */
  pin_info_type_v01 pin_info;
}dms_uim_verify_pin_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */

  /* Optional */
  /*  Retries left; returned if the enable/disable operation failed */
  uint8_t pin_retries_status_valid;	/* Must be set to true if pin_retries_status is being passed */
  pin_retries_status_type_v01 pin_retries_status;
}dms_uim_verify_pin_resp_msg_v01;	/* Message */

typedef struct {
  uint8_t unblock_pin_id;	/*  Specifies the ID of the PIN to be unblocked; valid values are:
       - 1 - PIN1 (also called PIN)
       - 2 - PIN2
   */

  uint32_t puk_value_len;	/* Must be set to # of elements in puk_value */
  uint8_t puk_value[DMS_PUK_VALUE_MAX_V01];	/*  Specifies the puk value (password) of the PIN to be unblocked  */

  uint32_t new_pin_value_len;	/* Must be set to # of elements in new_pin_value */
  uint8_t new_pin_value[DMS_PUK_VALUE_MAX_V01];	/*  Specifies the new PIN value (password) for the PIN to be unblocked  */
}pin_unblock_info_type_v01;	/* Type */

typedef struct {
  /* Mandatory */
  /*  Pin unblock information */
  pin_unblock_info_type_v01 pin_unblock_info;
}dms_uim_unblock_pin_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */

  /* Optional */
  /*  Retries left; returned if the enable/disable operation failed */
  uint8_t pin_retries_status_valid;	/* Must be set to true if pin_retries_status is being passed */
  pin_retries_status_type_v01 pin_retries_status;
}dms_uim_unblock_pin_resp_msg_v01;	/* Message */

typedef struct {
  uint8_t pin_id;	/*  Specifies the ID of the PIN to be changed; valid values are:
     - 1 - PIN1 (also called PIN)
     - 2 - PIN2
   */

  uint32_t old_pin_value_len;	/* Must be set to # of elements in old_pin_value */
  uint8_t old_pin_value[DMS_PIN_VALUE_MAX_V01];	/*  Specifies the old PIN value (old password) of the PIN  */

  uint32_t new_pin_value_len;	/* Must be set to # of elements in new_pin_value */
  uint8_t new_pin_value[DMS_PIN_VALUE_MAX_V01];	/*  Specifies the new PIN value (old password) of the PIN  */
}pin_change_info_type_v01;	/* Type */

typedef struct {
  /* Mandatory */
  /*  Pin change information */
  pin_change_info_type_v01 pin_change_info;
}dms_uim_change_pin_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */

  /* Optional */
  /*  Retries left; returned if the enable/disable operation failed */
  uint8_t pin_retries_status_valid;	/* Must be set to true if pin_retries_status is being passed */
  pin_retries_status_type_v01 pin_retries_status;
}dms_uim_change_pin_resp_msg_v01;	/* Message */

/*
 * dms_uim_get_pin_status_req_msg is empty
 * typedef struct {
 * }dms_uim_get_pin_status_req_msg_v01;
 */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */

  /* Optional */
  /*  PIN 1 status */
  uint8_t pin1_status_valid;	/* Must be set to true if pin1_status is being passed */
  pin_status_type_v01 pin1_status;

  /* Optional */
  /*  PIN 2 status */
  uint8_t pin2_status_valid;	/* Must be set to true if pin2_status is being passed */
  pin_status_type_v01 pin2_status;
}dms_uim_get_pin_status_resp_msg_v01;	/* Message */

/*
 * dms_get_device_hardware_rev_req_msg is empty
 * typedef struct {
 * }dms_get_device_hardware_rev_req_msg_v01;
 */

typedef struct {
  /* Mandatory */
  /*  Hardware revision */
  char hardware_rev[DMS_HARDWARE_REV_MAX_V01 + 1];	/*  String containing the hardware revision of the device  */

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}dms_get_device_hardware_rev_resp_msg_v01;	/* Message */

/*
 * dms_get_operating_mode_req_msg is empty
 * typedef struct {
 * }dms_get_operating_mode_req_msg_v01;
 */

typedef struct {
  /* Mandatory */
  /*  Operating Mode */
  uint8_t operating_mode;	/*  Selected operating mode
       - 0x00 - Online
       - 0x01 - Low Power
       - 0x02 - Factory Test Mode
       - 0x03 - Offline
       - 0x04 - Resetting
       - 0x05 - Shutting Down
   */

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}dms_get_operating_mode_resp_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Operating Mode */
  uint8_t operating_mode;	/*  Selected operating mode
       - 0x00 - Online
       - 0x01 - Low Power
       - 0x02 - Factory Test Mode
       - 0x03 - Offline
       - 0x04 - Reset
       - 0x05 - Shut Down
   */
}dms_set_operating_mode_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}dms_set_operating_mode_resp_msg_v01;	/* Message */

/*
 * dms_get_time_req_msg is empty
 * typedef struct {
 * }dms_get_time_req_msg_v01;
 */

typedef struct {
  uint8_t time_count[6];	/*  Count of 1.25 ms that have elapsed from the start of GPS time
      (Jan 6, 1980)
   */

  uint16_t time_source;	/*  Source of the timestamp
       - 0x00 - 32 kHz device clock
       - 0x01 - CDMA network
       - 0x02 - HDR network
   */
}device_time_type_v01;	/* Type */

typedef struct {
  /* Mandatory */
  /*  Device Time */
  device_time_type_v01 device_time;

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */

  /* Optional */
  /*  System Time in ms */
  uint8_t sys_time_in_ms_valid;	/* Must be set to true if sys_time_in_ms is being passed */
  uint64_t sys_time_in_ms;	/*  System Time in ms - Count of ms that have
       elapsed from the start of GPS Epoch time
      (January 6, 1980)
   */

  /* Optional */
  /*  User Time in ms */
  uint8_t user_time_in_ms_valid;	/* Must be set to true if user_time_in_ms is being passed */
  uint64_t user_time_in_ms;	/*  User Time in ms - Count of ms that have
       elapsed from the start of GPS Epoch time
       (January 6, 1980)
   */
}dms_get_time_resp_msg_v01;	/* Message */

/*
 * dms_get_prl_ver_req_msg is empty
 * typedef struct {
 * }dms_get_prl_ver_req_msg_v01;
 */

typedef struct {
  /* Mandatory */
  /*  PRL version */
  uint16_t PRL_version;	/*  PRL version */

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}dms_get_prl_ver_resp_msg_v01;	/* Message */

/*
 * dms_get_user_lock_state_req_msg is empty
 * typedef struct {
 * }dms_get_user_lock_state_req_msg_v01;
 */

typedef struct {
  /* Mandatory */
  /*  User lock state */
  uint8_t lock_state;	/*  Current state of the lock
       - 0x00 - Disabled
       - 0x01 - Enabled
   */

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}dms_get_user_lock_state_resp_msg_v01;	/* Message */

typedef struct {
  uint8_t lock_state;	/*  Current state of the lock
       - 0x00 - Disabled
       - 0x01 - Enabled
   */

  char lock_code[4];	/*  4-byte code set for the lock in ASCII format (digits 0 to 9 only)  */
}user_lock_state_info_type_v01;	/* Type */

typedef struct {
  /* Mandatory */
  /*  User lock state */
  user_lock_state_info_type_v01 lock_info;
}dms_set_user_lock_state_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}dms_set_user_lock_state_resp_msg_v01;	/* Message */

typedef struct {
  char cur_code[4];	/*  Current 4-byte code to use for the lock in ASCII format (digits 0 to
       9 only)
   */

  char new_code[4];	/*  New 4-byte code to use for the lock in ASCII format (digits 0 to
       9 only)
   */
}user_lock_set_info_type_v01;	/* Type */

typedef struct {
  /* Mandatory */
  user_lock_set_info_type_v01 lock_info;
}dms_set_user_lock_code_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}dms_set_user_lock_code_resp_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Programming code */
  char spc[6];	/*  Service programming code in ASCII format (digits 0 to 9 only)  */
}dms_validate_service_programming_code_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}dms_validate_service_programming_code_resp_msg_v01;	/* Message */

/*
 * dms_uim_get_iccid_req_msg is empty
 * typedef struct {
 * }dms_uim_get_iccid_req_msg_v01;
 */

typedef struct {
  /* Mandatory */
  /*  UIM ICCID */
  char uim_id[DMS_UIM_ID_MAX_V01 + 1];	/*  String containing the UIM ICCID  */

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}dms_uim_get_iccid_resp_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  UIM personalization facility */
  uint8_t facility;	/*  MT or network facility (corresponding AT+CLCK value)
       - 0x00 - Network Personalization (PN)
       - 0x01 - Network sUbset personalization (PU)
       - 0x02 -Service Provider Personalization (PP)
       - 0x03 - Corporate Personalization (PC)
       - 0x04 - UIM Personalization (PF)
   */
}dms_uim_get_ck_status_req_msg_v01;	/* Message */

typedef struct {
  uint8_t facility_state;	/*  UIM facility state
       - 0x00 - Deactivated
       - 0x01 - Activated
       - 0x02 - Blocked
   */

  uint8_t verify_reties_left;	/*  Indicates the number of retries left, after which the CK will
       be blocked  */

  uint8_t unblock_retries_left;	/*  Indicates the number of retries left after the CK will be
       permanently blocked
    */
}facility_state_info_type_v01;	/* Type */

typedef struct {
  /* Mandatory */
  /*  Facility CK status */
  facility_state_info_type_v01 facility_info;

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */

  /* Optional */
  /*  Operation blocking facility */
  uint8_t operation_blocking_valid;	/* Must be set to true if operation_blocking is being passed */
  uint8_t operation_blocking;	/*  Presence of this TLV indicates that this facility is currently
       blocking normal operation of the device. This value can be
       returned only if the facility_state is not 0x00 (Deactivated).

       Note: This value will be set to 1 when the TLV is provided.
   */
}dms_uim_get_ck_status_resp_msg_v01;	/* Message */

typedef struct {
  uint8_t facility;	/*  UIM personalization facility (corresponding AT+CLCK value)
       - 0x00 - Network Personalization (PN)
       - 0x01 - Network sUbset personalization (PU)
       - 0x02 - Service Provider Personalization (PP)
       - 0x03 - Corporate Personalization (PC)
       - 0x04 - UIM Personalization (PF)
   */

  uint8_t facility_state;	/*  UIM facility state
       - 0x00 - Deactivate
   */

  char facility_ck[DMS_FACILITY_CK_MAX_V01 + 1];	/*  Facility de-personalizaton control key string in ASCII text
       (maximum 8 bytes)
    */
}facility_set_ck_info_type_v01;	/* Type */

typedef struct {
  /* Mandatory */
  /*  UIM personalization facility */
  facility_set_ck_info_type_v01 facility_set_ck_info;
}dms_uim_set_ck_protection_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Facility CK retry status */
  uint8_t verify_retries_left;	/*  Indicates the number of retries left, after which the CK will
       be blocked
    */

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}dms_uim_set_ck_protection_resp_msg_v01;	/* Message */

typedef struct {
  uint8_t facility;	/*  UIM personalization facility (corresponding AT+CLCK value)
       - 0x00 - Network Personalization (PN)
       - 0x01 - Network sUbset personalization (PU)
       - 0x02 - Service Provider Personalization (PP)
       - 0x03 - Corporate Personalization (PC)
       - 0x04 - UIM Personalization (PF)
   */

  char facility_unblock_ck[DMS_FACILITY_UNBLOCK_CK_MAX_V01 + 1];	/*  Facility control key string in ASCII text (maximum 8 bytes)  */
}facility_unblock_info_type_v01;	/* Type */

typedef struct {
  /* Mandatory */
  /*  UIM personalization facility */
  facility_unblock_info_type_v01 facility_unblock_info;
}dms_uim_unblock_ck_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */

  /* Optional */
  /*  Facility CK retry status */
  uint8_t unblock_retries_left_valid;	/* Must be set to true if unblock_retries_left is being passed */
  uint8_t unblock_retries_left;	/*  Indicates the number of retries left, after which the CK will
       be permanently blocked
    */
}dms_uim_unblock_ck_resp_msg_v01;	/* Message */

/*
 * dms_uim_get_imsi_req_msg is empty
 * typedef struct {
 * }dms_uim_get_imsi_req_msg_v01;
 */

typedef struct {
  /* Mandatory */
  /*  Int. Mobile Subscriber ID */
  char imsi[DMS_IMSI_MAX_V01 + 1];	/*  String containing the Int. Mobile Subscriber ID  */

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}dms_uim_get_imsi_resp_msg_v01;	/* Message */

/*
 * dms_uim_get_state_req_msg is empty
 * typedef struct {
 * }dms_uim_get_state_req_msg_v01;
 */

typedef struct {
  /* Mandatory */
  /*  UIM state */
  uint8_t uim_state;	/*  UIM state
       - 0x00 - UIM Initialization Completed
       - 0x01 - UIM Locked or Failed
       - 0x02 - UIM Not Present
       - 0x03 - 0xFE - Reserved
       - 0xFF - UIM state currently unavailable
   */

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}dms_uim_get_state_resp_msg_v01;	/* Message */

/*
 * dms_get_band_capability_req_msg is empty
 * typedef struct {
 * }dms_get_band_capability_req_msg_v01;
 */

typedef struct {
  /* Mandatory */
  /*  Band capability */
  uint64_t band_capability;	/*  Bitmask of bands supported by the device
       - Bit 0 - Band class 0, A-system
       - Bit 1 - Band class 0, B-system
       - Bit 2 - Band class 1, all blocks
       - Bit 3 - Band class 2
       - Bit 4 - Band class 3, A-system
       - Bit 5 - Band class 4, all blocks
       - Bit 6 - Band class 5, all blocks
       - Bit 7 - GSM DCS band (1800)
       - Bit 8 - GSM Extended GSM (E-GSM) band (900)
       - Bit 9 - GSM Primary GSM (P-GSM) band (900)
       - Bit 10 - Band class 6
       - Bit 11 - Band class 7
       - Bit 12 - Band class 8
       - Bit 13 - Band class 9
       - Bit 14 - Band class 10
       - Bit 15 - Band class 11
       - Bit 16 - GSM 450 band
       - Bit 17 - GSM 480 band
       - Bit 18 - GSM 750 band
       - Bit 19 - GSM 850 band
       - Bit 20 - GSM railways GSM band (900)
       - Bit 21 - GSM PCS band (1900)
       - Bit 22 - WCDMA (Europe, Japan, and China) 2100 band
       - Bit 23 - WCDMA US PCS 1900 band
       - Bit 24 - WCDMA (Europe and China) DCS 1800 band
       - Bit 25 - WCDMA US 1700 band
       - Bit 26 - WCDMA US 850 band
       - Bit 27 - WCDMA Japan 800 band
       - Bit 28 - Band class 12
       - Bit 29 - Band class 14
       - Bit 30 - Reserved
       - Bit 31 - Band class 15
       - Bits 32 through 47 - Reserved
       - Bit 48 - WCDMA Europe 2600 band
       - Bit 49 - WCDMA Europe and Japan 900 band
       - Bit 50 - WCDMA Japan 1700 band
       - Bits 51 through 55 - Reserved
       - Bit 56 - Band class 16
       - Bit 57 - Band class 17
       - Bit 58 - Band class 18
       - Bit 59 - Band class 19
   */

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}dms_get_band_capability_resp_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Time */
  uint64_t time_in_ms;	/*  Time in ms - Count of ms that have elapsed
         from the start of GPS Epoch time
         (January 6, 1980)
	  */

  /* Optional */
  /*  Time Reference Type */
  uint8_t time_reference_type_valid;	/* Must be set to true if time_reference_type is being passed */
  uint32_t time_reference_type;	/*  Time reference used while setting the time
         0x00000000 - User Time
         0x00000001 to 0xFFFFFFFF - Reserved for
         future extension
	 */
}dms_set_time_req_msg_v01;	/* Message */

typedef struct {
  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;	/*  Standard response type.  */
}dms_set_time_resp_msg_v01;	/* Message */

/*Service Message Definition*/
#define QMI_DMS_RESET_REQ_V01 0x0000
#define QMI_DMS_RESET_RESP_V01 0x0000
#define QMI_DMS_SET_EVENT_REPORT_REQ_V01 0x0001
#define QMI_DMS_SET_EVENT_REPORT_RESP_V01 0x0001
#define QMI_DMS_EVENT_REPORT_IND_V01 0x0001
#define QMI_DMS_GET_DEVICE_CAP_REQ_V01 0x0020
#define QMI_DMS_GET_DEVICE_CAP_RESP_V01 0x0020
#define QMI_DMS_GET_DEVICE_MFR_REQ_V01 0x0021
#define QMI_DMS_GET_DEVICE_MFR_RESP_V01 0x0021
#define QMI_DMS_GET_DEVICE_MODEL_ID_REQ_V01 0x0022
#define QMI_DMS_GET_DEVICE_MODEL_ID_RESP_V01 0x0022
#define QMI_DMS_GET_DEVICE_REV_ID_REQ_V01 0x0023
#define QMI_DMS_GET_DEVICE_REV_ID_RESP_V01 0x0023
#define QMI_DMS_GET_MSISDN_REQ_V01 0x0024
#define QMI_DMS_GET_MSISDN_RESP_V01 0x0024
#define QMI_DMS_GET_DEVICE_SERIAL_NUMBERS_REQ_V01 0x0025
#define QMI_DMS_GET_DEVICE_SERIAL_NUMBERS_RESP_V01 0x0025
#define QMI_DMS_GET_POWER_STATE_REQ_V01 0x0026
#define QMI_DMS_GET_POWER_STATE_RESP_V01 0x0026
#define QMI_DMS_UIM_SET_PIN_PROTECTION_REQ_V01 0x0027
#define QMI_DMS_UIM_SET_PIN_PROTECTION_RESP_V01 0x0027
#define QMI_DMS_UIM_VERIFY_PIN_REQ_V01 0x0028
#define QMI_DMS_UIM_VERIFY_PIN_RESP_V01 0x0028
#define QMI_DMS_UIM_UNBLOCK_PIN_REQ_V01 0x0029
#define QMI_DMS_UIM_UNBLOCK_PIN_RESP_V01 0x0029
#define QMI_DMS_UIM_CHANGE_PIN_REQ_V01 0x002A
#define QMI_DMS_UIM_CHANGE_PIN_RESP_V01 0x002A
#define QMI_DMS_UIM_GET_PIN_STATUS_REQ_V01 0x002B
#define QMI_DMS_UIM_GET_PIN_STATUS_RESP_V01 0x002B
#define QMI_DMS_GET_DEVICE_HARDWARE_REV_REQ_V01 0x002C
#define QMI_DMS_GET_DEVICE_HARDWARE_REV_RESP_V01 0x002C
#define QMI_DMS_GET_OPERATING_MODE_REQ_V01 0x002D
#define QMI_DMS_GET_OPERATING_MODE_RESP_V01 0x002D
#define QMI_DMS_SET_OPERATING_MODE_REQ_V01 0x002E
#define QMI_DMS_SET_OPERATING_MODE_RESP_V01 0x002E
#define QMI_DMS_GET_TIME_REQ_V01 0x002F
#define QMI_DMS_GET_TIME_RESP_V01 0x002F
#define QMI_DMS_GET_PRL_VER_REQ_V01 0x0030
#define QMI_DMS_GET_PRL_VER_RESP_V01 0x0030
#define QMI_DMS_GET_USER_LOCK_STATE_REQ_V01 0x0034
#define QMI_DMS_GET_USER_LOCK_STATE_RESP_V01 0x0034
#define QMI_DMS_SET_USER_LOCK_STATE_REQ_V01 0x0035
#define QMI_DMS_SET_USER_LOCK_STATE_RESP_V01 0x0035
#define QMI_DMS_SET_USER_LOCK_CODE_REQ_V01 0x0036
#define QMI_DMS_SET_USER_LOCK_CODE_RESP_V01 0x0036
#define QMI_DMS_VALIDATE_SERVICE_PROGRAMMING_CODE_REQ_V01 0x003B
#define QMI_DMS_VALIDATE_SERVICE_PROGRAMMING_CODE_RESP_V01 0x003B
#define QMI_DMS_UIM_GET_ICCID_REQ_V01 0x003C
#define QMI_DMS_UIM_GET_ICCID_RESP_V01 0x003C
#define QMI_DMS_UIM_GET_CK_STATUS_REQ_V01 0x0040
#define QMI_DMS_UIM_GET_CK_STATUS_RESP_V01 0x0040
#define QMI_DMS_UIM_SET_CK_PROTECTION_REQ_V01 0x0041
#define QMI_DMS_UIM_SET_CK_PROTECTION_RESP_V01 0x0041
#define QMI_DMS_UIM_UNBLOCK_CK_REQ_V01 0x0042
#define QMI_DMS_UIM_UNBLOCK_CK_RESP_V01 0x0042
#define QMI_DMS_UIM_GET_IMSI_REQ_V01 0x0043
#define QMI_DMS_UIM_GET_IMSI_RESP_V01 0x0043
#define QMI_DMS_UIM_GET_STATE_REQ_V01 0x0044
#define QMI_DMS_UIM_GET_STATE_RESP_V01 0x0044
#define QMI_DMS_GET_BAND_CAPABILITY_REQ_V01 0x0045
#define QMI_DMS_GET_BAND_CAPABILITY_RESP_V01 0x0045
#define QMI_DMS_SET_TIME_REQ_V01 0x004B
#define QMI_DMS_SET_TIME_RESP_V01 0x004B

/* Service Object Accessor */
qmi_idl_service_object_type dms_get_service_object_internal_v01
 ( int32_t idl_maj_version, int32_t idl_min_version, int32_t library_version );
 
#define dms_get_service_object_v01( ) \
          dms_get_service_object_internal_v01( \
            DMS_V01_IDL_MAJOR_VERS, DMS_V01_IDL_MINOR_VERS, \
            DMS_V01_IDL_TOOL_VERS )


#ifdef __cplusplus
}
#endif
#endif

