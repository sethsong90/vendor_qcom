#ifndef SNS_SMGR_SVC_SERVICE_01_H
#define SNS_SMGR_SVC_SERVICE_01_H
/**
  @file sns_smgr_api_v01.h
  
  @brief This is the public header file which defines the SNS_SMGR_SVC service Data structures.

  This header file defines the types and structures that were defined in 
  SNS_SMGR_SVC. It contains the constant values defined, enums, structures,
  messages, and service message IDs (in that order) Structures that were 
  defined in the IDL as messages contain mandatory elements, optional 
  elements, a combination of mandatory and optional elements (mandatory 
  always come before optionals in the structure), or nothing (null message)
   
  An optional element in a message is preceded by a uint8_t value that must be
  set to true if the element is going to be included. When decoding a received
  message, the uint8_t values will be set to true or false by the decode
  routine, and should be checked before accessing the values that they
  correspond to. 
   
  Variable sized arrays are defined as static sized arrays with an unsigned
  integer (32 bit) preceding it that must be set to the number of elements
  in the array that are valid. For Example:
   
  uint32_t test_opaque_len;
  uint8_t test_opaque[16];
   
  If only 4 elements are added to test_opaque[] then test_opaque_len must be
  set to 4 before sending the message.  When decoding, the _len value is set 
  by the decode routine and should be checked so that the correct number of 
  elements in the array will be accessed. 

*/
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
  Copyright (c) 2011,2013 Qualcomm Technologies, Inc.  All Rights Reserved
  Qualcomm Technologies Proprietary and Confidential.


  $Header$
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====* 
 *THIS IS AN AUTO GENERATED FILE. DO NOT ALTER IN ANY WAY 
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/* This file was generated with Tool version 6.2 
   It was generated on: Wed Oct 23 2013 (Spin 0)
   From IDL File: sns_smgr_api_v01.idl */

/** @defgroup SNS_SMGR_SVC_qmi_consts Constant values defined in the IDL */
/** @defgroup SNS_SMGR_SVC_qmi_msg_ids Constant values for QMI message IDs */
/** @defgroup SNS_SMGR_SVC_qmi_enums Enumerated types used in QMI messages */
/** @defgroup SNS_SMGR_SVC_qmi_messages Structures sent as QMI messages */
/** @defgroup SNS_SMGR_SVC_qmi_aggregates Aggregate types used in QMI messages */
/** @defgroup SNS_SMGR_SVC_qmi_accessor Accessor for QMI service object */
/** @defgroup SNS_SMGR_SVC_qmi_version Constant values for versioning information */

#include <stdint.h>
#include "qmi_idl_lib.h"
#include "sns_common_v01.h"


#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup SNS_SMGR_SVC_qmi_version 
    @{ 
  */ 
/** Major Version Number of the IDL used to generate this file */
#define SNS_SMGR_SVC_V01_IDL_MAJOR_VERS 0x01
/** Revision Number of the IDL used to generate this file */
#define SNS_SMGR_SVC_V01_IDL_MINOR_VERS 0x11
/** Major Version Number of the qmi_idl_compiler used to generate this file */
#define SNS_SMGR_SVC_V01_IDL_TOOL_VERS 0x06
/** Maximum Defined Message ID */
#define SNS_SMGR_SVC_V01_MAX_MESSAGE_ID 0x0024;
/** 
    @} 
  */


/** @addtogroup SNS_SMGR_SVC_qmi_consts 
    @{ 
  */

/**  Response message ACK/NAK codes
  

 the enum are 32 bits for reference only, in actual message, we use 8bits only to save space  */
#define SNS_SMGR_RESPONSE_ACK_SUCCESS_V01 0

/**  See reason codes  */
#define SNS_SMGR_RESPONSE_ACK_MODIFIED_V01 1

/**  Lack table space  */
#define SNS_SMGR_RESPONSE_NAK_RESOURCES_V01 2

/**  Can't find report for delete  */
#define SNS_SMGR_RESPONSE_NAK_REPORT_ID_V01 3

/**  None supplied or modified away  */
#define SNS_SMGR_RESPONSE_NAK_NO_ITEMS_V01 4

/**  Invalid Action field  */
#define SNS_SMGR_RESPONSE_NAK_UNK_ACTION_V01 5

/**  Report rate is unsupportable  */
#define SNS_SMGR_RESPONSE_NAK_REPORT_RATE_V01 6

/**  Time period in Query request is unsupportable  */
#define SNS_SMGR_RESPONSE_NAK_TIME_PERIOD_V01 7

/**  Unspecified internal errors  */
#define SNS_SMGR_RESPONSE_NAK_INTERNAL_ERR_V01 8

/**  Query request with given QueryID already received  */
#define SNS_SMGR_RESPONSE_NAK_QUERY_ID_V01 9

/**  Sensor specified in Query request was not in Buffering request  */
#define SNS_SMGR_RESPONSE_NAK_SENSOR_ID_V01 10

/**  Reason codes for substituting a default or deleting an item.  */
#define SNS_SMGR_REASON_NULL_V01 0

/**  Rate set to 20 Hz or maximum supported rate, whichever is lower  */
#define SNS_SMGR_REASON_DEFAULT_RATE_V01 10

/**  Type set to Engineering Units  */
#define SNS_SMGR_REASON_DEFAULT_TYPE_V01 11

/**  Decimation set to Latest Sample  */
#define SNS_SMGR_REASON_DEFAULT_DECIM_V01 12

/**  Sensitivity code set to 0  */
#define SNS_SMGR_REASON_DEFAULT_STIVTY_V01 13
#define SNS_SMGR_REASON_DEFAULT_FINAL_V01 14

/**  Item deleted  */
#define SNS_SMGR_REASON_UNKNOWN_SENSOR_V01 15

/**  Item deleted  */
#define SNS_SMGR_REASON_FAILED_SENSOR_V01 16

/**  Item deleted  */
#define SNS_SMGR_REASON_OTHER_FAILURE_V01 17

/**  Sampling rate is unsupportable. Item deleted  */
#define SNS_SMGR_REASON_SAMPLING_RATE_V01 18

/**  Item modified  */
#define SNS_SMGR_REASON_SAMPLE_QUALITY_NORMAL_V01 19

/**  Status of the report, if OK, or Canceled
   */
#define SNS_SMGR_REPORT_OK_V01 0

/**  Lack table space  */
#define SNS_SMGR_REPORT_CANCEL_RESOURCE_V01 1

/**  All req sensors have failed  */
#define SNS_SMGR_REPORT_CANCEL_FAILURE_V01 2

/**  Server shut down  */
#define SNS_SMGR_REPORT_CANCEL_SHUT_DOWN_V01 3

/**  Sensor Request Activity. Delete or add. Add may act as replace if the same
   Client ID and Report ID are found in the SOL
   */
#define SNS_SMGR_REPORT_ACTION_ADD_V01 1
#define SNS_SMGR_REPORT_ACTION_DELETE_V01 2

/**  The Decimation word specifies how to reduce oversampled data.
   Report most recent sample (Default)
   Average samples since previous report
   Filter at half the reporting rate or next lower available frequency
   sns_smgr_decimation_t
  

 Unfiltered, possibly interpolated and/or calibrated  */
#define SNS_SMGR_DECIMATION_RECENT_SAMPLE_V01 1
#define SNS_SMGR_DECIMATION_AVERAGE_V01 2
#define SNS_SMGR_DECIMATION_FILTER_V01 3

/**  =============== for sensor report message ===============
 Flag bit values in the Flag word associated with each data item in a sensor
   report.  The first 3 values correspond to the first 3 data words which are
   related with XYZ axes of those sensors that have 3 axis measurements.  For
   sensors that are not axis oriented, these flags correspond to the first
   three data words.  If any of the first 3 flags is non-zero, it indicates
   that the corresponding data word was found at the extreme edge of its valid
   range; this indicates that the sensor was railed.
   The 4th flag, when non-zero, indicates that the data item is invalid. The
   Quality word indicates why the item is invalid.
   sns_smgr_item_flags_t
   */
#define SNS_SMGR_ITEM_FLAG_X_RAIL_V01 1
#define SNS_SMGR_ITEM_FLAG_Y_RAIL_V01 2
#define SNS_SMGR_ITEM_FLAG_Z_RAIL_V01 4
#define SNS_SMGR_ITEM_FLAG_INVALID_V01 8
#define SNS_SMGR_ITEM_FLAG_FAC_CAL_V01 16
#define SNS_SMGR_ITEM_FLAG_AUTO_CAL_V01 32

/**  The Quality word is associated with each data item in a sensor report. It
    is a code defining the quality of the measurement.
  

 Unfiltered sample; available when client requests
        SNS_SMGR_CAL_SEL_RAW, and
        SNS_SMGR_DECIMATION_RECENT_SAMPLE, and
        SNS_SMGR_SAMPLE_QUALITY_ACCURATE_TIMESTAMP  */
#define SNS_SMGR_ITEM_QUALITY_CURRENT_SAMPLE_V01 0

/**  Sensor missed sampling schedule  */
#define SNS_SMGR_ITEM_QUALITY_PRIOR_VALUE_LATE_V01 1

/**  Client specified this for no motion  */
#define SNS_SMGR_ITEM_QUALITY_PRIOR_VALUE_SUSPENDED_V01 2

/**  Client specified averaging  */
#define SNS_SMGR_ITEM_QUALITY_AVERAGED_SPECIFIED_V01 3

/**  Average substituted while filter starting  */
#define SNS_SMGR_ITEM_QUALITY_AVERAGED_FILTER_START_V01 4

/**  This value is used if SNS_SMGR_DECIMATION_FILTER option was used in the report request  */
#define SNS_SMGR_ITEM_QUALITY_FILTERED_V01 5

/**  Sample not available due to failed sensor  */
#define SNS_SMGR_ITEM_QUALITY_INVALID_FAILED_SENSOR_V01 10

/**  Sensor starting  */
#define SNS_SMGR_ITEM_QUALITY_INVALID_NOT_READY_V01 11

/**  Not in motion  */
#define SNS_SMGR_ITEM_QUALITY_INVALID_SUSPENDED_V01 12

/**  Sample is the result of interpolation  */
#define SNS_SMGR_ITEM_QUALITY_INTERPOLATED_V01 13

/**  Sample is the result of interpolation and CIC filtering  */
#define SNS_SMGR_ITEM_QUALITY_INTERPOLATED_FILTERED_V01 14

/**  Define sensor identifier.

    The following table illustrates what the data types correspond to. For example, primary
    sensor data type of SNS_SMGR_ID_PROX_LIGHT is proximity sensor and the secondary one is
    ambient light sensor.

    ID        PRIMARY         SECONDARY
    --------  --------------  --------------
    0         Accelerometer   Temperature
    10        Gyro            Temperature
    20        Magnetometer    Temperature
    30        Pressure        Temperature
    40        Proximity       Ambient light
    50        Humidity        Temperature
    60        RGB             Color Temperature/Clear
    70        SAR             Specific Absorption Rate
    220       Step            none
    222       Step Count      none
    224       SMD             none
    226       GameRV          none
    228       IR Gesture      Proximity
    230       Double-tap      Single-tap
    240-249   OEM-defined     OEM-defined


 This is the primary accel sensor ID , namely this is
   the accel ID if there is only one accel sensor or the first
   accel sensor ID if there are multiple accel sensors  */
#define SNS_SMGR_ID_ACCEL_V01 0
#define SNS_SMGR_ID_ACCEL_2_V01 1
#define SNS_SMGR_ID_ACCEL_3_V01 2
#define SNS_SMGR_ID_ACCEL_4_V01 3
#define SNS_SMGR_ID_ACCEL_5_V01 4

/**  This is primary gyro sensor ID  */
#define SNS_SMGR_ID_GYRO_V01 10
#define SNS_SMGR_ID_GYRO_2_V01 11
#define SNS_SMGR_ID_GYRO_3_V01 12
#define SNS_SMGR_ID_GYRO_4_V01 13
#define SNS_SMGR_ID_GYRO_5_V01 14

/** This is primary mag sensor ID  */
#define SNS_SMGR_ID_MAG_V01 20

/**  This is primary pressure sensor ID  */
#define SNS_SMGR_ID_PRESSURE_V01 30

/**  This is primary prox light sensor ID */
#define SNS_SMGR_ID_PROX_LIGHT_V01 40

/**  This is primary humidity sensor ID */
#define SNS_SMGR_ID_HUMIDITY_V01 50

/**  Primary = RGB, Secondary = Color Temperature and clear component of RGB */
#define SNS_SMGR_ID_RGB_V01 60

/**  Primary = SAR, Secondary = none */
#define SNS_SMGR_ID_SAR_V01 70

/**  Embedded sensor: Primary = Step Detection, Secondary = (none)  */
#define SNS_SMGR_ID_STEP_EVENT_V01 220

/**  Embedded sensor: Primary = Step Count, Secondary = (none)  */
#define SNS_SMGR_ID_STEP_COUNT_V01 222

/**  Embedded sensor: Primary = SMD, Secondary = (none)  */
#define SNS_SMGR_ID_SMD_V01 224

/**  Embedded sensor: Primary = Game Rotation Vector, Secondary = (none)  */
#define SNS_SMGR_ID_GAME_ROTATION_VECTOR_V01 226

/**  Embedded sensor: Primary = IR_GESTURE, Secondary = PROXIMITY   */
#define SNS_SMGR_ID_IR_GESTURE_V01 228

/**  Embedded sensor: Primary = Double-Tap, Secondary = Single-Tap  */
#define SNS_SMGR_ID_TAP_V01 230

/**  Sensor IDs for custom sensor types  */
#define SNS_SMGR_ID_OEM_SENSOR_01_V01 240
#define SNS_SMGR_ID_OEM_SENSOR_02_V01 241
#define SNS_SMGR_ID_OEM_SENSOR_03_V01 242
#define SNS_SMGR_ID_OEM_SENSOR_04_V01 243
#define SNS_SMGR_ID_OEM_SENSOR_05_V01 244
#define SNS_SMGR_ID_OEM_SENSOR_06_V01 245
#define SNS_SMGR_ID_OEM_SENSOR_07_V01 246
#define SNS_SMGR_ID_OEM_SENSOR_08_V01 247
#define SNS_SMGR_ID_OEM_SENSOR_09_V01 248
#define SNS_SMGR_ID_OEM_SENSOR_10_V01 249

/**  Select the sensor data type that should be reported from the sensor.
   Primary data type for that sensor.
   Some sensors can also report secondary data Type, this could be expanded.
   */
#define SNS_SMGR_DATA_TYPE_PRIMARY_V01 0

/**  Sensor data types, described in subsequent comments  */
#define SNS_SMGR_DATA_TYPE_SECONDARY_V01 1

/**  Identify the sensor test type.
  
 Status of sensor test, used in sns_smgr_single_sensor_test_resp_msg
  
 Result of sensor test, used in sns_smgr_single_sensor_test_ind_msg
  
 Select the option for how reports will be generated when the unit is
   stationary (not moving). This is a power saving feature. The goal is to
   suspend sampling and/or reporting while the unit is at rest.

   If all items of a report vote for NO_REPORT, that report is suspended
   until motion resumes. However, if another item of this report votes to
   continue reporting, this item is effectively promoted to REPORT_PRIOR.

   REPORT_PRIOR votes to continue generating the report, but suspend sampling
   the sensor named by this item, that is, keep reporting the last available
   sample. However, if some other report votes to keep this sensor sampling,
   then the prior sample continues to be updated.

   REPORT_FULL votes to continue sampling this sensor and generating this
   report.

   REPORT_INTERIM votes to sample and report when hardware indicates motion
   but the motion detection algorithm has not yet confirmed it.
   */
#define SNS_SMGR_REST_OPTION_NO_REPORT_V01 0
#define SNS_SMGR_REST_OPTION_REPORT_PRIOR_V01 1
#define SNS_SMGR_REST_OPTION_REPORT_FULL_V01 2
#define SNS_SMGR_REST_OPTION_REPORT_INTERIM_V01 3

/**  The selection of calibration for sampled data in sensor report request message.
  

 Raw sensor data + factory calibration(if available) + auto calibration(if available)  */
#define SNS_SMGR_CAL_SEL_FULL_CAL_V01 0

/**  Raw sensor data + factory calibration(if available)  */
#define SNS_SMGR_CAL_SEL_FACTORY_CAL_V01 1

/**  Un-calibrated, possibly interpolated, sensor data.  */
#define SNS_SMGR_CAL_SEL_RAW_V01 2

/**  =============== other Constants ===============

 Maximum number of sensor values in one report  */
#define SNS_SMGR_MAX_ITEMS_PER_REPORT_V01 10

/**  Limit number of reason codes in a response message 
 Valid report rate request and default. Rate request may be expressed as Hz
   or msec interval  */
#define SNS_SMGR_MAX_NUM_REASONS_V01 10
#define SNS_SMGR_REPORT_RATE_MIN_HZ_V01 1
#define SNS_SMGR_REPORT_RATE_MAX_HZ_V01 500

/**   Equivalent to 0.5 Hz  */
#define SNS_SMGR_REPORT_RATE_MIN_MSEC_V01 2000

/**   1 minute interval  */
#define SNS_SMGR_REPORT_RATE_MAX_MSEC_V01 60000

/**   Use default if in neither range  */
#define SNS_SMGR_REPORT_RATE_DEFAULT_V01 20

/**   x, y,z axis forprimary datatype
      temperature for secondary datatype
      others are reserved fields  */
#define SNS_SMGR_SENSOR_DIMENSION_V01 3

/**  Size of the compensation matrix (in this case a 3x3 matrix)  */
#define SNS_SMGR_COMPENSATION_MATRIX_SIZE_V01 9

/**   maximum sensor numbers  */
#define SNS_SMGR_MAX_SENSOR_NUM_V01 20

/**   maximum number of bytes to store a sensor name  */
#define SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01 80

/**   maximum number of bytes to store a sensor name  */
#define SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01 20

/**   number of bytes to store a short sensor name  */
#define SNS_SMGR_SHORT_SENSOR_NAME_SIZE_V01 16

/**  maximum data type per sensor, dpending on the sensor  */
#define SNS_SMGR_MAX_DATA_TYPE_PER_SENSOR_V01 3

/**   apply the dynamic calibration data for this report  */
#define SNS_SMGR_CAL_APPLY_V01 0

/**   apply the dynamic calibration data for this report  */
#define SNS_SMGR_CAL_DYNAMIC_V01 SNS_SMGR_CAL_APPLY_V01

/**  This definition is defined for future use, and shall not be used until announce to be used  */
#define SNS_SMGR_CAL_SAVE_V01 1

/**   apply the factory calibration data for this report  */
#define SNS_SMGR_CAL_FACTORY_V01 2

/**  To add report in sns_smgr_sensor_power_status_req_msg  */
#define SNS_SMGR_POWER_STATUS_ADD_V01 0

/**  To delete report in sns_smgr_sensor_power_status_req_msg  */
#define SNS_SMGR_POWER_STATUS_DEL_V01 1

/**   NoChange since last status report sns_smgr_sensor_power_status_s  */
#define SNS_SMGR_POWER_STATE_NO_CHANGE_V01 0

/**  Sensor went active since last report in sns_smgr_sensor_power_status_s  */
#define SNS_SMGR_POWER_STATE_GO_ACTIVE_V01 1

/**  Sensor went low power since last report in sns_smgr_sensor_power_status_s  */
#define SNS_SMGR_POWER_STATE_GO_LOW_POWER_V01 2

/**  Sensor cycled through active and low power since last report in sns_smgr_sensor_power_status_s  */
#define SNS_SMGR_POWER_STATE_CYCLE_ACTIVE_AND_LOW_V01 3

/**   automatic control (default) in sns_smgr_sensor_power_control_req_msg  */
#define SNS_SMGR_POWER_CTRL_AUTO_V01 0

/**   active state - command the max power state  in sns_smgr_sensor_power_control_req_msg  */
#define SNS_SMGR_POWER_CTRL_ACTIVE_V01 1

/**   idle  state - command the low power state  in sns_smgr_sensor_power_control_req_msg  */
#define SNS_SMGR_POWER_CTRL_IDLE_V01 2

/**   off state - not possible in 8660 DSPS  in sns_smgr_sensor_power_control_req_msg  */
#define SNS_SMGR_POWER_CTRL_OFF_V01 3

/**   To add report in sns_smgr_sensor_status_req_msg  */
#define SNS_SMGR_SENSOR_STATUS_ADD_V01 0

/**   To delete report in sns_smgr_sensor_status_req_msg  */
#define SNS_SMGR_SENSOR_STATUS_DEL_V01 1

/**  Define sensor state used in sns_smgr_sensor_status_ind_msg  */
#define SNS_SMGR_SENSOR_STATUS_UNKNOWN_V01 0
#define SNS_SMGR_SENSOR_STATUS_IDLE_V01 1
#define SNS_SMGR_SENSOR_STATUS_ACTIVE_V01 2
#define SNS_SMGR_SENSOR_STATUS_ONE_CLIENT_V01 3

/**  Bit values for SampleQuality field in sensor report requests  */
#define SNS_SMGR_SAMPLE_QUALITY_ACCURATE_TIMESTAMP_V01 1

/**  =============== Buffering Specific Constants ===============
 Defines the actions used in Buffering requests  */
#define SNS_SMGR_BUFFERING_ACTION_ADD_V01 1
#define SNS_SMGR_BUFFERING_ACTION_DELETE_V01 2

/**  Maximum number of sensor/data type pairs in one Buffering request  */
#define SNS_SMGR_BUFFERING_REQUEST_MAX_ITEMS_V01 5

/**  Maximum number of samples in one Buffering report  */
#define SNS_SMGR_BUFFERING_REPORT_MAX_SAMPLES_V01 100

/**  Buffering reports sent only when queried  */
#define SNS_SMGR_BUFFERING_REPORT_RATE_NONE_V01 0

/** ============================================================================
============================================================================
 We allow status updates for sensor status by processor, the following
    constants define the processor mappings  

 Clients on the dedicated sensors processor (DSPS/ADSP)  */
#define SNS_SMGR_DSPS_CLIENTS_V01 0

/**  Clients on the processor running the HLOS  */
#define SNS_SMGR_APPS_CLIENTS_V01 1

/**  Clients on the modem processor  */
#define SNS_SMGR_MODEM_CLIENT_V01 2
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_enums
    @{
  */
typedef enum {
  SNS_SMGR_TEST_TYPE_E_MIN_ENUM_VAL_V01 = -2147483647, /**< To force a 32 bit signed enum.  Do not change or use*/
  SNS_SMGR_TEST_SELF_V01 = 0, 
  SNS_SMGR_TEST_IRQ_V01 = 1, 
  SNS_SMGR_TEST_CONNECTIVITY_V01 = 2, 
  SNS_SMGR_TEST_SELF_HW_V01 = 3, 
  SNS_SMGR_TEST_SELF_SW_V01 = 4, 
  SNS_SMGR_TEST_OEM_V01 = 5, 
  SNS_SMGR_TEST_TYPE_E_MAX_ENUM_VAL_V01 = 2147483647 /**< To force a 32 bit signed enum.  Do not change or use*/
}sns_smgr_test_type_e_v01;
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_enums
    @{
  */
typedef enum {
  SNS_SMGR_TEST_STATUS_E_MIN_ENUM_VAL_V01 = -2147483647, /**< To force a 32 bit signed enum.  Do not change or use*/
  SNS_SMGR_TEST_STATUS_SUCCESS_V01 = 0, 
  SNS_SMGR_TEST_STATUS_PENDING_V01 = 1, 
  SNS_SMGR_TEST_STATUS_DEVICE_BUSY_V01 = 2, /**<  Device is busy streaming  */
  SNS_SMGR_TEST_STATUS_INVALID_TEST_V01 = 3, /**<  Test case is invalid/undefined  */
  SNS_SMGR_TEST_STATUS_INVALID_PARAM_V01 = 4, /**<  Test parameter is invalid  */
  SNS_SMGR_TEST_STATUS_FAIL_V01 = 5, /**<  Unspecified error  */
  SNS_SMGR_TEST_STATUS_BUSY_TESTING_V01 = 6, /**<  Another test is running; try later  */
  SNS_SMGR_TEST_STATUS_E_MAX_ENUM_VAL_V01 = 2147483647 /**< To force a 32 bit signed enum.  Do not change or use*/
}sns_smgr_test_status_e_v01;
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_enums
    @{
  */
typedef enum {
  SNS_SMGR_TEST_RESULT_E_MIN_ENUM_VAL_V01 = -2147483647, /**< To force a 32 bit signed enum.  Do not change or use*/
  SNS_SMGR_TEST_RESULT_PASS_V01 = 0, 
  SNS_SMGR_TEST_RESULT_FAIL_V01 = 1, 
  SNS_SMGR_TEST_RESULT_E_MAX_ENUM_VAL_V01 = 2147483647 /**< To force a 32 bit signed enum.  Do not change or use*/
}sns_smgr_test_result_e_v01;
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_aggregates
    @{
  */
typedef struct {

  uint8_t SensorId;
  /**<   Defines the sensor that this configuration pertains to. The sensor can be one of following:
    - 00 - SNS_SMGR_ID_ACCEL
    - 10 - SNS_SMGR_ID_GYRO
    - 20 - SNS_SMGR_ID_MAG
    - 30 - SNS_SMGR_ID_PRESSURE
    - 40 - SNS_SMGR_ID_PROX_LIGHT
    - 50 - SNS_SMGR_ID_HUMIDITY
    - 60 - SNS_SMGR_ID_RGB
    - 70 - SNS_SMGR_ID_SAR
    - 220 - SNS_SMGR_ID_STEP_EVENT
    - 222 - SNS_SMGR_ID_STEP_COUNT
    - 224 - SNS_SMGR_ID_SMD
    - 226 - SNS_SMGR_ID_GAME_ROTATION_VECTOR
    - 228 - SNS_SMGR_ID_IR_GESTURE
    - 230 - SNS_SMGR_ID_TAP
    - 240-249 - SNS_SMGR_ID_OEM_SENSOR_XX
    - All other values defined as SNS_SMGR_ID_XXXX style are reserved for future use
  */

  uint8_t DataType;
  /**<   Defines sensor data type which classifies if the data type is primary or secondary.
    - 00 - SNS_SMGR_DATA_TYPE_PRIMARY
    - 01 - SNS_SMGR_DATA_TYPE_SECONDARY
    - All other values defined as SNS_SMGR_DATA_TYPE_XXXX style are reserved for future use
      This parameter identifies the sensor data type.
   */

  uint8_t Sensitivity;
  /**<   This parameter is defined for future use and is NOT implemented.
       This value shall be set to 0.
  */

  uint8_t Decimation;
  /**<   Defines decimation option for this item in this report
    - 01 - SNS_SMGR_DECIMATION_RECENT_SAMPLE
    - 03 - SNS_SMGR_DECIMATION_FILTER
    - All other values defined as SNS_SMGR_DECIMATION_XXXX style are reserved for future use
    The SNS_SMGR_DECIMATION_FILTER option can used only for accelerometer and gyro sensor type to reduce data noise.
    When SNS_SMGR_DECIMATION_FILTER option is set, multiple samples could be used for one report.
   */

  uint16_t MinSampleRate;
  /**<   This parameter is defined for future use and is NOT implemented.
       This value shall be set to 0. */

  uint8_t StationaryOption;
  /**<   This parameter is defined for future use and is NOT implemented.
       This value shall be set to 0. */

  uint8_t DoThresholdTest;
  /**<   This parameter is defined for future use and is NOT implemented.
       This value shall be set to 0. */

  uint8_t ThresholdOutsideMinMax;
  /**<   This parameter is defined for future use and is NOT implemented.
       This value shall be set to 0. */

  uint8_t ThresholdDelta;
  /**<   This parameter is defined for future use and is NOT implemented.
       This value shall be set to 0. */

  uint8_t ThresholdAllAxes;
  /**<   This parameter is defined for future use and is NOT implemented.
       This value shall be set to 0. */

  int32_t ThresholdMinMax[2];
  /**<   This parameter is defined for future use and is NOT implemented.
       This value shall be set to 0. */
}sns_smgr_periodic_report_item_s_v01;  /* Type */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Request Message; This command requests sensor periodic report for sensor sampling */
typedef struct {

  /* Mandatory */
  uint8_t ReportId;
  /**<   The report ID assigned by a client to distinguish this report among its reports */

  /* Mandatory */
  uint8_t Action;
  /**<   Defines if this report is to be added or deleted.
    - 01 - SNS_SMGR_REPORT_ACTION_ADD
    - 02 - SNS_SMGR_REPORT_ACTION_DELETE
    - All other values defined as SNS_SMGR_REPORT_ACTION_XXXX style are reserved for future use
    When SNS_SMGR_REPORT_ACTION_ADD is used and the same report ID is already added,
    the old one will be replaced by the new report request.
    */

  /* Mandatory */
  uint16_t ReportRate;
  /**<   Defines reporting rate. This value shall be within the sensor capacity which can be identified by using
   SNS_SMGR_SINGLE_SENSOR_INFO message. When this parameter is 0, 20Hz will be used as the default */

  /* Mandatory */
  uint8_t BufferFactor;
  /**<   This parameter is defined for future use and is NOT implemented.
       This value shall be set to 0 */

  /* Mandatory */
  uint32_t Item_len;  /**< Must be set to # of elements in Item */
  sns_smgr_periodic_report_item_s_v01 Item[SNS_SMGR_MAX_ITEMS_PER_REPORT_V01];

  /* Optional */
  uint8_t cal_sel_valid;  /**< Must be set to true if cal_sel is being passed */
  uint32_t cal_sel_len;  /**< Must be set to # of elements in cal_sel */
  uint8_t cal_sel[SNS_SMGR_MAX_ITEMS_PER_REPORT_V01];
  /**<   Defines the calibration option to be used. The index of the cal sel should match to the index of the Item parameter.
    - 00 - SNS_SMGR_CAL_SEL_FULL_CAL which refers applying factory calibration factors(if available) and auto calibration factors(if available) on to the raw data
    - 01 - SNS_SMGR_CAL_SEL_FACTORY_CAL which refers applying factory calibration factors(if available) on to the raw data
    - 02 - SNS_SMGR_CAL_SEL_RAW
    - All other values defined as SNS_SMGR_CAL_SEL_XXXX style are reserved for future use
    */

  /* Optional */
  uint8_t SrcModule_valid;  /**< Must be set to true if SrcModule is being passed */
  uint8_t SrcModule;
  /**<   For sensor internal use only.
       Defines the source module that is sending this message.
    */

  /* Optional */
  uint8_t SampleQuality_valid;  /**< Must be set to true if SampleQuality is being passed */
  uint32_t SampleQuality_len;  /**< Must be set to # of elements in SampleQuality */
  uint16_t SampleQuality[SNS_SMGR_MAX_ITEMS_PER_REPORT_V01];
  /**<   Specifies the desired quality of sensor data
    - SNS_SMGR_SAMPLE_QUALITY_ACCURATE_TIMESTAMP - High accuracy for sample timestamp.
      Delivery sampling rate may be up to twice the requested sampling rate,
      and may also result in higher report rate.
      Clients are recommended to specify 50, 100, or 200Hz sampling rates to
      minimize the chance of increase in sampling rate.
    */

  /* Optional */
  uint8_t notify_suspend_valid;  /**< Must be set to true if notify_suspend is being passed */
  sns_suspend_notification_s_v01 notify_suspend;
  /**<   Identifies if indications for this request should be sent
       when the processor is in suspend state.

       If this field is not specified, default value will be set to
       proc_type                  = SNS_PROC_APPS
       send_indications_during_suspend  = FALSE
    */
}sns_smgr_periodic_report_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_aggregates
    @{
  */
typedef struct {

  uint8_t ItemNum;
  /**<   The Item parameter number in the request msg */

  uint8_t Reason;
  /**<   Defines reason codes:
    - 00 - SNS_SMGR_REASON_NULL
    - 10 - SNS_SMGR_REASON_DEFAULT_RATE
    - 12 - SNS_SMGR_REASON_DEFAULT_DECIM
    - 15 - SNS_SMGR_REASON_UNKNOWN_SENSOR
    - 16 - SNS_SMGR_REASON_FAILED_SENSOR
    - 17 - SNS_SMGR_REASON_OTHER_FAILURE
    - 18 - SNS_SMGR_REASON_SAMPLING_RATE
    - 19 - SNS_SMGR_REASON_SAMPLE_QUALITY_NORMAL
    - All other values defined as SNS_SMGR_REASON_XXXX style are reserved for future use
  */
}sns_smgr_reason_pair_s_v01;  /* Type */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Response Message; This command requests sensor periodic report for sensor sampling */
typedef struct {

  /* Mandatory */
  sns_common_resp_s_v01 Resp;

  /* Mandatory */
  uint8_t ReportId;
  /**<   The report ID assigned by a client to distinguish this report among its reports */

  /* Mandatory */
  uint8_t AckNak;
  /**<   Defines whether this response is Acknowledgement or Negative Acknowledgement
    - 00 - SNS_SMGR_RESPONSE_ACK_SUCCESS
    - 01 - SNS_SMGR_RESPONSE_ACK_MODIFIED some parameters in the request are modified
    - 02 - SNS_SMGR_RESPONSE_NAK_RESOURCES
    - 03 - SNS_SMGR_RESPONSE_NAK_REPORT_ID Can't find report to delete
    - 04 - SNS_SMGR_RESPONSE_NAK_NO_ITEMS no item is supplied or the item is deleted by SMGR because of wrong parameters
    - 05 - SNS_SMGR_RESPONSE_NAK_UNK_ACTION when the action value is other than add or delete
    - All other values defined as SNS_SMGR_RESPONSE_ACK/NAK_XXXX style are reserved for future use
  */

  /* Mandatory */
  uint32_t ReasonPair_len;  /**< Must be set to # of elements in ReasonPair */
  sns_smgr_reason_pair_s_v01 ReasonPair[SNS_SMGR_MAX_NUM_REASONS_V01];
}sns_smgr_periodic_report_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_aggregates
    @{
  */
typedef struct {

  uint8_t SensorId;
  /**<   Defines the sensor that this configuration pertains to. Refer to the Sensor ID table defined
       under "Define sensor identifier" .
  */

  uint8_t DataType;
  /**<   Defines sensor data type which classifies if the data type is primary or secondary.
    - 00 - SNS_SMGR_DATA_TYPE_PRIMARY
    - 01 - SNS_SMGR_DATA_TYPE_SECONDARY
    - All other values defined as SNS_SMGR_DATA_TYPE_XXXX style are reserved for future use
      This parameter identifies the sensor data type.
   */

  int32_t ItemData[3];
  /**<   For 3-axis items such as accelerometer, gyro, and magnetometer, words [0] to [2] are XYZ. For other items, only the first
     word is used. The units are defined as following:
     - ACCEL       : m/s2
     - GYRO        : rad/s
     - MAG         : Gauss
     - PRESSURE    : hPa
     - PROX        : FAR=0, NEAR=1. Note: still in Q16 format
     - LIGHT       : lx
     - TEMPERATURE : Celsius
     - HUMIDITY    : percentage in Q16
     - RGB         : Raw ADC counts : X = Red, Y = Green, Z = Blue
     - SAR         : FAR=0, NEAR=non negative number indicating the sensor touched
     - CT_C        : X = Color temperature in Q16 (deg Kelvin), Y = Raw ADC counts for Clear data, Z = Reserved
     - STEP        : 1 - a step is detected
     - STEP_COUNT  : number of steps taken
     - SMD         : 1 - SMD was detected
     - GAME_ROTATION_VECTOR : quaternion values (Q16)
     - IR_GESTURE  : (revision 10: Sensor ID defined, but not used)
     - DOUBLE-TAP/PRIMARY :
     - SINGLE-TAP/SECONDARY: Dimension-less (raw) value indicating the source of the tap event, relative
                             to the device. (Consider the device as a point mass located at the origin (0,0,0)
                             of the Cartesian coordinate system.)

                             0 = no tap event,      1 = tap from +X axis, 2 = tap from -X axis,
                             3 = tap from +Y axis,  4 = tap from -Y axis, 5 = tap from +Z axis,
                             6 = tap from -Z axis,  7 = tap along X axis, 8 = tap along Y axis,
                             9 = tap along Z axis, 10 = tap event (unknown axis)

     - OEM_SENSOR  : (OEM-defined)
  */

  uint32_t TimeStamp;
  /**<   The timestamp when the sample is made in ticks. */

  uint8_t ItemFlags;
  /**<   Defines the item flags. This bit flags have meanings following:
    - 00 - Normal
    - 08 - SNS_SMGR_ITEM_FLAG_INVALID
    - 16 - SNS_SMGR_ITEM_FLAG_FAC_CAL : Factory calibration data was applied
    - 32 - SNS_SMGR_ITEM_FLAG_AUTO_CAL: Auto calibration data was applied
    - All other values defined as SNS_SMGR_ITEM_FLAG_XXXX style are reserved for future use
  */

  uint8_t ItemQuality;
  /**<   Defines the item quality which is associated with the ItemFlags.
    - 00 - SNS_SMGR_ITEM_QUALITY_CURRENT_SAMPLE
    - 05 - SNS_SMGR_ITEM_QUALITY_FILTERED
    - 10 - SNS_SMGR_ITEM_QUALITY_INVALID_FAILED_SENSOR
    - 11 - SNS_SMGR_ITEM_QUALITY_INVALID_NOT_READY
    - 13 - SNS_SMGR_ITEM_QUALITY_INTERPOLATED
    - 14 - SNS_SMGR_ITEM_QUALITY_INTERPOLATED_FILTERED
    - All other values defined as SNS_SMGR_ITEM_QUALITY_XXXX style are reserved for future use
  */

  uint8_t ItemSensitivity;
  /**<   This field is defined for future use and is NOT implemented.
       Any value in this field shall not be referenced.
  */
}sns_smgr_data_item_s_v01;  /* Type */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Indication Message; This command requests sensor periodic report for sensor sampling */
typedef struct {

  /* Mandatory */
  uint8_t ReportId;
  /**<   The report ID assigned by a client to distinguish this report among its reports */

  /* Mandatory */
  uint8_t status;
  /**<   Defines the status. Non-zero code notifies that this report is canceled
      - 00 - SNS_SMGR_REPORT_OK
      - 01 - SNS_SMGR_REPORT_CANCEL_RESOURCE
      - 02 - SNS_SMGR_REPORT_CANCEL_FAILURE
      - All other values defined as SNS_SMGR_REPORT_XXXX style are reserved for future use
       */

  /* Mandatory */
  uint16_t CurrentRate;
  /**<   The current reporting rate that is the sampling rate of the first item.*/

  /* Mandatory */
  uint32_t Item_len;  /**< Must be set to # of elements in Item */
  sns_smgr_data_item_s_v01 Item[SNS_SMGR_MAX_ITEMS_PER_REPORT_V01];

  /* Optional */
  uint8_t SamplingRate_valid;  /**< Must be set to true if SamplingRate is being passed */
  uint32_t SamplingRate_len;  /**< Must be set to # of elements in SamplingRate */
  uint32_t SamplingRate[SNS_SMGR_MAX_ITEMS_PER_REPORT_V01];
  /**<   Specifies the frequency at which sensor is actually sampled. This value is expressed
       in Q16 format and in unit of Hz.
     */
}sns_smgr_periodic_report_ind_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Request Message; This command sets dynamic or auto generated calibrate factors of a sensor. */
typedef struct {

  /* Mandatory */
  uint8_t usage;
  /**<   Defines the usage of the calibration data in this request message.
    - 00 - SNS_SMGR_CAL_DYNAMIC
    - 02 - SNS_SMGR_CAL_FACTORY
    - All other values defined as SNS_SMGR_CAL_XXX style are reserved for future use
  */

  /* Mandatory */
  uint8_t SensorId;
  /**<   Defines the sensor that this configuration pertains to. Refer to the Sensor ID table defined
       under "Define sensor identifier" .
  */

  /* Mandatory */
  uint8_t DataType;
  /**<   Defines sensor data type which classifies if the data type is primary or secondary.
    - 00 - SNS_SMGR_DATA_TYPE_PRIMARY
    - 01 - SNS_SMGR_DATA_TYPE_SECONDARY
    - All other values defined as SNS_SMGR_DATA_TYPE_XXXX style are reserved for future use
      This parameter identifies the sensor data type.
   */

  /* Mandatory */
  uint32_t ZeroBias_len;  /**< Must be set to # of elements in ZeroBias */
  int32_t ZeroBias[SNS_SMGR_SENSOR_DIMENSION_V01];
  /**<  
    The value must be Q16 format (16 bits for integer part, 16 bits for decimal part), indicating the zero bias that is to be added (in
    nominal engineering units)
  */

  /* Mandatory */
  uint32_t ScaleFactor_len;  /**< Must be set to # of elements in ScaleFactor */
  uint32_t ScaleFactor[SNS_SMGR_SENSOR_DIMENSION_V01];
  /**<  
    The value must be Q16 format, a multiplier that indicates scale factor need to be multiplied to current data .
    For example, enter 1.01 if the scaling is 1% less aggressive or 0.95 if it is 5% more aggressive.
  */

  /* Optional */
  uint8_t CompensationMatrix_valid;  /**< Must be set to true if CompensationMatrix is being passed */
  uint32_t CompensationMatrix_len;  /**< Must be set to # of elements in CompensationMatrix */
  int32_t CompensationMatrix[SNS_SMGR_COMPENSATION_MATRIX_SIZE_V01];
  /**<  
    The Compensation Matrix, if present to calibrate sensor data for.
    If the Compensation Matrix is supplied, te ScaleFactor above are ignored.
    The calibrated sample (Sc) is computed as
    Sc = (Sr - Bias)*CM
    where :
        Sc = Calibrated sensor sample
        Sr = Read sensor sample
        CM = Compensation Matrix (from this message)
        Bias = Zero Bias (from this message)

    Matrix elements are in Q16 format in row major order ie:
    CM =  CM0  CM1  CM2
          CM3  CM4  CM5
          CM6  CM7  CM8
  */

  /* Optional */
  uint8_t CalibrationAccuracy_valid;  /**< Must be set to true if CalibrationAccuracy is being passed */
  int32_t CalibrationAccuracy;
  /**<   Accuracy of Calibration. The interpretation of this field is
       implementation dependant. A guiding rule though, is that higher
       accuracies are better with 0 meaning complete unreliability.
  */
}sns_smgr_sensor_cal_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Response Message; This command sets dynamic or auto generated calibrate factors of a sensor. */
typedef struct {

  /* Mandatory */
  sns_common_resp_s_v01 Resp;
}sns_smgr_sensor_cal_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_aggregates
    @{
  */
typedef struct {

  uint8_t SensorID;
  /**<   Defines the sensor that this configuration pertains to. Refer to the Sensor ID table defined
       under "Define sensor identifier" .
  */

  uint32_t SensorShortName_len;  /**< Must be set to # of elements in SensorShortName */
  char SensorShortName[SNS_SMGR_SHORT_SENSOR_NAME_SIZE_V01];
  /**<   The value is a short sensor name:
  "ACCEL"
  "GYRO"
  "MAG"
  "PROX_LIGHT"
  "PRESSURE"
  "HUMIDITY"
  "RGB"
  "SAR"
  */
}sns_smgr_sensor_id_info_s_v01;  /* Type */
/**
    @}
  */

typedef struct {
  /* This element is a placeholder to prevent the declaration of 
     an empty struct.  DO NOT USE THIS FIELD UNDER ANY CIRCUMSTANCE */
  char __placeholder;
}sns_smgr_all_sensor_info_req_msg_v01;

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Response Message; This command sends all sensor info request and get all sensor IDs and short
           sensor names. */
typedef struct {

  /* Mandatory */
  sns_common_resp_s_v01 Resp;

  /* Mandatory */
  uint32_t SensorInfo_len;  /**< Must be set to # of elements in SensorInfo */
  sns_smgr_sensor_id_info_s_v01 SensorInfo[SNS_SMGR_MAX_SENSOR_NUM_V01];
}sns_smgr_all_sensor_info_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_aggregates
    @{
  */
typedef struct {

  uint8_t SensorID;
  /**<   Defines the sensor that this configuration pertains to. Refer to the Sensor ID table defined
       under "Define sensor identifier" .
  */

  uint8_t DataType;
  /**<   Defines sensor data type which classifies if the data type is primary or secondary.
    - 00 - SNS_SMGR_DATA_TYPE_PRIMARY
    - 01 - SNS_SMGR_DATA_TYPE_SECONDARY
    - All other values defined as SNS_SMGR_IDSNS_SMGR_DATA_TYPE_XXXX style are reserved for future use
  */

  uint32_t SensorName_len;  /**< Must be set to # of elements in SensorName */
  char SensorName[SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01];
  /**<   The model name of the sensor
  */

  uint32_t VendorName_len;  /**< Must be set to # of elements in VendorName */
  char VendorName[SNS_SMGR_MAX_VENDOR_NAME_SIZE_V01];
  /**<   The vendor name of the sensor */

  uint32_t Version;
  /**<   The version of sensor module */

  uint16_t MaxSampleRate;
  /**<   The maximum freq value that the sensor can stream */

  uint16_t IdlePower;
  /**<   Power consumption in uA when the sensor is in IDLE mode */

  uint16_t MaxPower;
  /**<   Power consumption in uA when the sensor is in operation mode */

  uint32_t MaxRange;
  /**<   The maximum range that the sensor can support in nominal engineering units. This value is represented by Q16 format */

  uint32_t Resolution;
  /**<   The resolution that the sensor uses in nominal engineering units. This value is represented by Q16 format */
}sns_smgr_sensor_datatype_info_s_v01;  /* Type */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_aggregates
    @{
  */
typedef struct {

  uint32_t data_type_info_len;  /**< Must be set to # of elements in data_type_info */
  sns_smgr_sensor_datatype_info_s_v01 data_type_info[SNS_SMGR_MAX_DATA_TYPE_PER_SENSOR_V01];
}sns_smgr_sensor_info_s_v01;  /* Type */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Request Message; This command sends single sensor info request and gets all the detailed information of the sensor */
typedef struct {

  /* Mandatory */
  uint8_t SensorID;
  /**<   see #define SNS_SMGR_ID_XXX_XXX */
}sns_smgr_single_sensor_info_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Response Message; This command sends single sensor info request and gets all the detailed information of the sensor */
typedef struct {

  /* Mandatory */
  sns_common_resp_s_v01 Resp;

  /* Mandatory */
  sns_smgr_sensor_info_s_v01 SensorInfo;

  /* Optional */
  uint8_t num_buffered_reports_valid;  /**< Must be set to true if num_buffered_reports is being passed */
  uint32_t num_buffered_reports_len;  /**< Must be set to # of elements in num_buffered_reports */
  uint32_t num_buffered_reports[SNS_SMGR_MAX_DATA_TYPE_PER_SENSOR_V01];
  /**<   The max number of reports that can be buffered for this data type */
}sns_smgr_single_sensor_info_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_aggregates
    @{
  */
typedef struct {

  uint8_t SensorID;
  /**<   see #define SNS_SMGR_ID_XXX_XXX */

  uint8_t BusCanAccessSensor;

  uint8_t CanCommandSensor;

  uint8_t CanReadSensorStatus;

  uint8_t CanReadSensorData;

  uint8_t DataShowsNoise;

  uint8_t CanReadFactoryCalibrationROM;

  uint8_t ValidSelfTestReport;

  uint8_t CanReceiveInterrupt;
}sns_smgr_sensor_test_result_s_v01;  /* Type */
/**
    @}
  */

typedef struct {
  /* This element is a placeholder to prevent the declaration of 
     an empty struct.  DO NOT USE THIS FIELD UNDER ANY CIRCUMSTANCE */
  char __placeholder;
}sns_smgr_sensor_test_req_msg_v01;

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Response Message; This command requests all-sensor test.
    These message are defined for future use, so the messages WON'T be supported. */
typedef struct {

  /* Mandatory */
  sns_common_resp_s_v01 Resp;

  /* Mandatory */
  uint32_t result_len;  /**< Must be set to # of elements in result */
  sns_smgr_sensor_test_result_s_v01 result[SNS_SMGR_MAX_SENSOR_NUM_V01];
}sns_smgr_sensor_test_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Request Message; This command requests single sensor test. */
typedef struct {

  /* Mandatory */
  uint8_t SensorID;
  /**<   Defines the sensor that this configuration pertains to.
       The sensor ID is defined as SNS_SMGR_ID_XXXX style.
  */

  /* Mandatory */
  uint8_t DataType;
  /**<   Defines sensor data type which classifies if the data type is primary or secondary.
    - 00 - SNS_SMGR_DATA_TYPE_PRIMARY
    - 01 - SNS_SMGR_DATA_TYPE_SECONDARY
  */

  /* Mandatory */
  sns_smgr_test_type_e_v01 TestType;
  /**<   Defines the type of test to be executed.
    - 00 - SNS_SMGR_TEST_SELF
    - All other values defined as SNS_SMGR_TEST_XXXX style are reserved for future use
  */

  /* Optional */
  uint8_t SaveToRegistry_valid;  /**< Must be set to true if SaveToRegistry is being passed */
  uint8_t SaveToRegistry;
  /**<   Specifies whether calibration data generated during the test should be saved to sensors registry.
       This applies only to sensors which generate calibration data as part of factory test.
       Default behavior is TRUE (save calibration data to sensors registry).
  */

  /* Optional */
  uint8_t ApplyCalNow_valid;  /**< Must be set to true if ApplyCalNow is being passed */
  uint8_t ApplyCalNow;
  /**<   Specifies whether calibration data should take affect immediately, rather than after reboot.
       This applies only to sensors which generate calibration data as part of factory test.
       Default behavior is TRUE (apply calibration data immediately).
  */
}sns_smgr_single_sensor_test_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Response Message; This command requests single sensor test. */
typedef struct {

  /* Mandatory */
  sns_common_resp_s_v01 Resp;

  /* Mandatory */
  uint8_t SensorID;
  /**<   Defines the sensor that this configuration pertains to.
       The sensor ID is defined as SNS_SMGR_ID_XXXX style.
  */

  /* Mandatory */
  uint8_t DataType;
  /**<   Defines sensor data type which classifies if the data type is primary or secondary.
    - 00 - SNS_SMGR_DATA_TYPE_PRIMARY
    - 01 - SNS_SMGR_DATA_TYPE_SECONDARY
  */

  /* Mandatory */
  sns_smgr_test_type_e_v01 TestType;
  /**<   Defines the type of test to be executed.
    - 00 - SNS_SMGR_TEST_SELF
    - All other values are reserved for future use
  */

  /* Mandatory */
  sns_smgr_test_status_e_v01 TestStatus;
  /**<   Identifies test status */
}sns_smgr_single_sensor_test_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Indication Message; This command requests single sensor test. */
typedef struct {

  /* Mandatory */
  uint8_t SensorID;
  /**<   Defines the sensor that this configuration pertains to.
       The sensor ID is defined as SNS_SMGR_ID_XXXX style.
  */

  /* Mandatory */
  uint8_t DataType;
  /**<   Defines sensor data type which classifies if the data type is primary or secondary
    - 00 - SNS_SMGR_DATA_TYPE_PRIMARY
    - 01 - SNS_SMGR_DATA_TYPE_SECONDARY
  */

  /* Mandatory */
  sns_smgr_test_type_e_v01 TestType;
  /**<   Defines the type of test to be executed.
    - 00 - SNS_SMGR_TEST_SELF
    - All other values are reserved for future use
  */

  /* Mandatory */
  sns_smgr_test_result_e_v01 TestResult;
  /**<   Indicates test result */

  /* Optional */
  uint8_t ErrorCode_valid;  /**< Must be set to true if ErrorCode is being passed */
  uint8_t ErrorCode;
  /**<   Test-specific error code */
}sns_smgr_single_sensor_test_ind_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Request Message; This command requests sensor power status.
    These message are defined for future use, so the messages WON'T be supported. */
typedef struct {

  /* Mandatory */
  uint8_t ReportId;
  /**<   ID assigned by client to distinguish client's reports */

  /* Mandatory */
  uint8_t Action;
  /**<   SNS_SMGR_POWER_STATUS_ADD =0 ,Add;
       SNS_SMGR_POWER_STATUS_DEL =1, Delete */
}sns_smgr_sensor_power_status_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_aggregates
    @{
  */
typedef struct {

  uint8_t SensorID;
  /**<   see #define SNS_SMGR_ID_XXX_XXX */

  uint8_t PowerAction;
  /**<   see #define SNS_SMGR_POWER_STATE_XXX */

  uint32_t ActiveTimeStamp;
  /**<   Timestamp when state changed to Active */

  uint32_t LowPowerTimeStamp;
  /**<   Timestamp when state changed to Low*/

  uint32_t CycleCount;
  /**<   Number of power state change between on and off since last report*/
}sns_smgr_sensor_power_status_s_v01;  /* Type */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Response Message; This command requests sensor power status.
    These message are defined for future use, so the messages WON'T be supported. */
typedef struct {

  /* Mandatory */
  sns_common_resp_s_v01 Resp;
}sns_smgr_sensor_power_status_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Indication Message; This command requests sensor power status.
    These message are defined for future use, so the messages WON'T be supported. */
typedef struct {

  /* Mandatory */
  uint8_t ReportId;
  /**<   ID assigned by client to distinguish client's reports */

  /* Mandatory */
  uint32_t PowerStatus_len;  /**< Must be set to # of elements in PowerStatus */
  sns_smgr_sensor_power_status_s_v01 PowerStatus[SNS_SMGR_MAX_SENSOR_NUM_V01];
}sns_smgr_sensor_power_status_ind_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Request Message; This command requests sensor power control.
    These message are defined for future use, so the messages WON'T be supported. */
typedef struct {

  /* Mandatory */
  uint8_t SensorID;
  /**<   see #define SNS_SMGR_ID_XXX_XXX; Defines the sensor that this configuration pertains to.
  */

  /* Mandatory */
  uint8_t Action;
  /**<   SNS_SMGR_POWER_CTRL_AUTO =0  automatic control (default)
       SNS_SMGR_POWER_CTRL_ACTIVE =1 active state - command the max power state
       SNS_SMGR_POWER_CTRL_IDLE =2   idle state - command to low power state
       SNS_SMGR_POWER_CTRL_OFF =3 = off - not possible in 8660 DSPS
  */
}sns_smgr_sensor_power_control_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Response Message; This command requests sensor power control.
    These message are defined for future use, so the messages WON'T be supported. */
typedef struct {

  /* Mandatory */
  sns_common_resp_s_v01 Resp;
}sns_smgr_sensor_power_control_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Request Message; This command requests sensor status which tells if the sensor is active, idle, or only one client is left for the
    sensor. Currently SMGR only supports one client at a time. So This feature is limited. */
typedef struct {

  /* Mandatory */
  uint8_t SensorID;
  /**<   Defines the sensor that this configuration pertains to. Refer to the Sensor ID table defined
       under "Define sensor identifier" .
  */

  /* Mandatory */
  uint8_t ReqDataTypeNum;
  /**<   How many data types client monitors and requests sampling data, this is used by
       SMGR to tell if there is only one client left.  When the number of request items drop to this
       number, SMGR will send SNS_SMGR_SENSOR_STATUS_ONE_CLIENT indication for the sensor*/

  /* Mandatory */
  uint8_t Action;
  /**<   SNS_SMGR_SENSOR_STATUS_ADD =0 ,Add;
       SNS_SMGR_SENSOR_STATUS_DEL =1, Delete
       All other values defined as SNS_SMGR_SENSOR_STATUS_XXXX style are reserved for future use
      */
}sns_smgr_sensor_status_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Response Message; This command requests sensor status which tells if the sensor is active, idle, or only one client is left for the
    sensor. Currently SMGR only supports one client at a time. So This feature is limited. */
typedef struct {

  /* Mandatory */
  sns_common_resp_s_v01 Resp;

  /* Mandatory */
  uint8_t SensorID;
  /**<   Defines the sensor that this configuration pertains to. Refer to the Sensor ID table defined
       under "Define sensor identifier" .
  */
}sns_smgr_sensor_status_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Indication Message; This command requests sensor status which tells if the sensor is active, idle, or only one client is left for the
    sensor. Currently SMGR only supports one client at a time. So This feature is limited. */
typedef struct {

  /* Mandatory */
  uint8_t SensorID;
  /**<   Defines the sensor that this configuration pertains to. Refer to the Sensor ID table defined
       under "Define sensor identifier" .
  */

  /* Mandatory */
  uint8_t SensorState;
  /**<   Defines the sensor status for this indication. The status can be one of following:
  - 00 - SNS_SMGR_SENSOR_STATUS_UNKNOWN
  - 01 - SNS_SMGR_SENSOR_STATUS_IDLE
  - 02 - SNS_SMGR_SENSOR_STATUS_ACTIVE
  - 03 - SNS_SMGR_SENSOR_STATUS_ONE_CLIENT
  - All other values defined as SNS_SMGR_SENSOR_STATUS_XXXX style are reserved for future use
  */

  /* Mandatory */
  uint32_t TimeStamp;
  /**<   The timestamp when state is changed */

  /* Optional */
  uint8_t PerProcToalClients_valid;  /**< Must be set to true if PerProcToalClients is being passed */
  uint32_t PerProcToalClients_len;  /**< Must be set to # of elements in PerProcToalClients */
  uint16_t PerProcToalClients[5];
  /**<   Total clients per processor indexed by the constants defined above */

  /* Optional */
  uint8_t MaxFreqPerProc_valid;  /**< Must be set to true if MaxFreqPerProc is being passed */
  uint32_t MaxFreqPerProc_len;  /**< Must be set to # of elements in MaxFreqPerProc */
  int32_t MaxFreqPerProc[5];
  /**<   Max frequency of data requested by clients on each processor
      Units of Hz, Q16 format
  */

  /* Optional */
  uint8_t MaxUpdateRatePerProc_valid;  /**< Must be set to true if MaxUpdateRatePerProc is being passed */
  uint32_t MaxUpdateRatePerProc_len;  /**< Must be set to # of elements in MaxUpdateRatePerProc */
  int32_t MaxUpdateRatePerProc[5];
  /**<   Max update rate of data requested by clients on each processor
      Units of Hz, Q16 format
  */
}sns_smgr_sensor_status_ind_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_aggregates
    @{
  */
typedef struct {

  uint8_t SensorId;
  /**<   Identifies the sensor to be sampled for data.  The valid sensors are:
    - SNS_SMGR_ID_ACCEL
    - SNS_SMGR_ID_GYRO
    - SNS_SMGR_ID_MAG
    - SNS_SMGR_ID_PRESSURE
    - SNS_SMGR_ID_PROX_LIGHT
    - SNS_SMGR_ID_HUMIDITY
    - SNS_SMGR_ID_RGB
    - SNS_SMGR_ID_SAR
    - SNS_SMGR_ID_STEP_EVENT
    - SNS_SMGR_ID_STEP_COUNT
    - SNS_SMGR_ID_SMD
    - SNS_SMGR_ID_GAME_ROTATION_VECTOR
    - SNS_SMGR_ID_IR_GESTURE
    - SNS_SMGR_ID_TAP
    - SNS_SMGR_OEM_SENSOR_XX
    - All other values defined as SNS_SMGR_ID_XXXX style are reserved for future use
  */

  uint8_t DataType;
  /**<   Identifies which data type of the specified sensor is being requested.
    - SNS_SMGR_DATA_TYPE_PRIMARY
    - SNS_SMGR_DATA_TYPE_SECONDARY
    - All other values defined as SNS_SMGR_DATA_TYPE_XXXX style are reserved for future use
      This parameter identifies the sensor data type.
   */

  uint8_t Decimation;
  /**<   Specifies decimation option for samples belonging to this item
    - SNS_SMGR_DECIMATION_RECENT_SAMPLE
    - SNS_SMGR_DECIMATION_FILTER
    - All other values will be rejected.
    The SNS_SMGR_DECIMATION_FILTER option is only applicable for ACCEL, GYRO,
    and MAG sensor types to reduce data noise.  When SNS_SMGR_DECIMATION_FILTER
    option is specified, multiple samples could be used for one report.
   */

  uint8_t Calibration;
  /**<   Specifies how raw data is to be calibrated
    - SNS_SMGR_CAL_SEL_FULL_CAL
    - SNS_SMGR_CAL_SEL_FACTORY_CAL
    - SNS_SMGR_CAL_SEL_RAW
    - All other values will be rejected
    */

  uint16_t SamplingRate;
  /**<   Specifies the frequency at which sensor is sampled.
    This value shall be within the sensor capacity,                                       .
    expressed in integer format and in unit of Hz.
    Values outside of sensor capacity will be rejected.
  */

  uint16_t SampleQuality;
  /**<   Specifies the desired quality of sensor data
    - SNS_SMGR_SAMPLE_QUALITY_ACCURATE_TIMESTAMP - High accuracy for sample timestamp.
      Delivery sampling rate may be up to twice the requested sampling rate,
      and may also result in higher report rate.
      Clients are recommended to specify 50, 100, or 200Hz sampling rates to
      minimize the chance of increase in sampling rate.
    */
}sns_smgr_buffering_req_item_s_v01;  /* Type */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Request Message; 
  This command requests sensor data to be sampled and buffered up to be sent together */
typedef struct {

  /* Mandatory */
  uint8_t ReportId;
  /**<   The report ID assigned by client to be used for identifying corresponding
    response and indication messages
    */

  /* Mandatory */
  uint8_t Action;
  /**<   Specifies the action to be carried out for this report
    - SNS_SMGR_BUFFERING_ACTION_ADD
    - SNS_SMGR_BUFFERING_ACTION_DELETE
    - All other values will be rejected.
    An existing report will be replaced by a new report of the same ID.
    This includes Periodic Report.  It is advisable for clients to use different
    sets of IDs for Buffering reports and Periodic reports.
    */

  /* Mandatory */
  uint32_t ReportRate;
  /**<   Specifies the desired reporting rate expressed in Q16 format and in unit of Hz.
    This is only meaningful when paired with SNS_SMGR_BUFFERING_ACTION_ADD
    To indicate no periodic reports, use SNS_SMGR_BUFFERING_REPORT_RATE_NONE.
  */

  /* Mandatory */
  uint32_t Item_len;  /**< Must be set to # of elements in Item */
  sns_smgr_buffering_req_item_s_v01 Item[SNS_SMGR_BUFFERING_REQUEST_MAX_ITEMS_V01];

  /* Optional */
  uint8_t notify_suspend_valid;  /**< Must be set to true if notify_suspend is being passed */
  sns_suspend_notification_s_v01 notify_suspend;
  /**<   Identifies if indications for this request should be sent
       when the processor is in suspend state.

       If this field is not specified, default value will be set to
       notify_suspend->proc_type                  = SNS_PROC_APPS
       notify_suspend->send_indications_during_suspend  = FALSE
    */

  /* Optional */
  uint8_t SrcModule_valid;  /**< Must be set to true if SrcModule is being passed */
  uint8_t SrcModule;
  /**<   For sensor internal use only.
       Defines the source module that is sending this message.
    */
}sns_smgr_buffering_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Response Message; 
  This command requests sensor data to be sampled and buffered up to be sent together */
typedef struct {

  /* Mandatory */
  sns_common_resp_s_v01 Resp;

  /* Optional */
  uint8_t ReportId_valid;  /**< Must be set to true if ReportId is being passed */
  uint8_t ReportId;
  /**<   The ID corresponding to a Buffering request */

  /* Optional */
  uint8_t AckNak_valid;  /**< Must be set to true if AckNak is being passed */
  uint8_t AckNak;
  /**<   Defines whether this response is Acknowledgement or Negative Acknowledgement
    - SNS_SMGR_RESPONSE_ACK_SUCCESS - the request has been accepted
    - SNS_SMGR_RESPONSE_ACK_MODIFIED - some parameters in the request are modified
    - SNS_SMGR_RESPONSE_NAK_RESOURCES - no resources to service the request
    - SNS_SMGR_RESPONSE_NAK_REPORT_ID - no such report to be deleted
    - SNS_SMGR_RESPONSE_NAK_NO_ITEMS - no valid items were sent in request
    - SNS_SMGR_RESPONSE_NAK_UNK_ACTION - invalid Action field in request
    - SNS_SMGR_RESPONSE_NAK_INTERNAL_ERR - unspecified error
  */

  /* Optional */
  uint8_t ReasonPair_valid;  /**< Must be set to true if ReasonPair is being passed */
  uint32_t ReasonPair_len;  /**< Must be set to # of elements in ReasonPair */
  sns_smgr_reason_pair_s_v01 ReasonPair[SNS_SMGR_MAX_NUM_REASONS_V01];
}sns_smgr_buffering_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_aggregates
    @{
  */
typedef struct {

  int32_t Data[SNS_SMGR_SENSOR_DIMENSION_V01];
  /**<   Each sample can have up to SNS_SMGR_SENSOR_DIMENSION words, each word
    is in Q16 format and in the units specific to the sensor/data type pair.
    For 3-axis samples, Data[0], Data[1], and Data[2] are X, Y, and Z axis,
    respectively.  For others, only Data[0] has valid measurement.
     - ACCEL/PRIMARY      : 3 axes, each in meter/second squared (m/s2)
     - GYRO/PRIMARY       : 3 axes, each in radian/second (rad/s)
     - MAG/PRIMARY        : 3 axes, each in Gauss
     - PRESSURE/PRIMARY   : 1 axis, in hectopascal (hPa)
     - PROX/PRIMARY       : 1 axis, FAR=0, NEAR=1
     - RGB/PRIMARY        : 3 axis, X axis = raw Red counts, Y axis = raw Green counts, Z axis = raw Blue counts
     - SAR/PRIMARY        : 1 axis, FAR=0, NEAR=non negative number indicating the sensor touched
     - ACCEL/SECONDARY    : 1 axis, in Celsius
     - GYRO/SECONDARY     : 1 axis, in Celsius
     - MAG/SECONDARY      : 1 axis, in Celsius
     - PRESSURE/SECONDARY : 1 axis, in Celsius
     - PROX/SECONDARY     : 1 axis, in Lux
     - RGB/SECONDARY      : 3 axis, X axis = Color temperature in Q16 (deg Kelvin), Y axis = raw Clear counts , Z axis = Reserved
     - IR_GESTURE         : (revision 10: Sensor ID defined, but not used)
     - DOUBLE-TAP/PRIMARY :
     - SINGLE-TAP/SECONDARY: Dimension-less (raw) value indicating the source of the tap event, relative
                             to the device. (Consider the device as a point mass located at the origin (0,0,0)
                             of the Cartesian coordinate system.)

                             0 = no tap event,      1 = tap from +X axis, 2 = tap from -X axis,
                             3 = tap from +Y axis,  4 = tap from -Y axis, 5 = tap from +Z axis,
                             6 = tap from -Z axis,  7 = tap along X axis, 8 = tap along Y axis,
                             9 = tap along Z axis, 10 = tap event (unknown axis)

     - OEM_SENSOR  : (OEM-defined)
  */

  uint16_t TimeStampOffset;
  /**<   The offset from timestamps of previous sample in report */

  uint8_t Flags;
  /**<   Status flags of this sample.
    - raw data
    - SNS_SMGR_ITEM_FLAG_INVALID
    - SNS_SMGR_ITEM_FLAG_FAC_CAL : Factory calibration data was applied
    - SNS_SMGR_ITEM_FLAG_AUTO_CAL: Auto calibration data was applied
    - All other values defined as SNS_SMGR_ITEM_FLAG_XXXX style are reserved for future use
  */

  uint8_t Quality;
  /**<   Quality of this sample.
    - SNS_SMGR_ITEM_QUALITY_CURRENT_SAMPLE
    - SNS_SMGR_ITEM_QUALITY_FILTERED
    - SNS_SMGR_ITEM_QUALITY_INTERPOLATED
    - SNS_SMGR_ITEM_QUALITY_INTERPOLATED_FILTERED
    - SNS_SMGR_ITEM_QUALITY_INVALID_FAILED_SENSOR
    - SNS_SMGR_ITEM_QUALITY_INVALID_NOT_READY
    - All other values defined as SNS_SMGR_ITEM_QUALITY_XXXX style are reserved for future use
  */
}sns_smgr_buffering_sample_s_v01;  /* Type */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_aggregates
    @{
  */
typedef struct {

  uint8_t SensorId;
  /**<   Identifies the sensor to which the samples belong.  This shall match one of
    the requested sensors.
  */

  uint8_t DataType;
  /**<   Identifies the data type of the specified sensor to which the samples belong.
   */

  uint8_t FirstSampleIdx;
  /**<   Index into Samples data of the first sample belonging to this
    SensorId/DataType pair.
   */

  uint8_t SampleCount;
  /**<   Number of samples belonging to this SensorId/DataType pair.
   */

  uint32_t FirstSampleTimestamp;
  /**<   Timestamp of first sample belonging to this SensorId/DataType pair.
    */

  uint32_t SamplingRate;
  /**<   Specifies the frequency at which sensor is actually sampled. This value is expressed
       in Q16 format and in unit of Hz.
    */
}sns_smgr_buffering_sample_index_s_v01;  /* Type */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Indication Message; 
  This command requests sensor data to be sampled and buffered up to be sent together */
typedef struct {

  /* Mandatory */
  uint8_t ReportId;
  /**<   The ID corresponding to a Buffering request */

  /* Mandatory */
  uint32_t Indices_len;  /**< Must be set to # of elements in Indices */
  sns_smgr_buffering_sample_index_s_v01 Indices[SNS_SMGR_BUFFERING_REQUEST_MAX_ITEMS_V01];
  /**<   Identifies which items in Samples belong to which SensorId/DataType pair
    specified in Buffering request */

  /* Mandatory */
  uint32_t Samples_len;  /**< Must be set to # of elements in Samples */
  sns_smgr_buffering_sample_s_v01 Samples[SNS_SMGR_BUFFERING_REPORT_MAX_SAMPLES_V01];
  /**<   Samples collected since previous report
    Depending on whether Batching is in effect, this may contain samples for
    only one of the requested items, or it may contain samples for all of them */

  /* Optional */
  uint8_t IndType_valid;  /**< Must be set to true if IndType is being passed */
  uint8_t IndType;
  /**<   Optional batch indication type
       SNS_BATCH_ONLY_IND - Standalone batch indication. Not part of a back to back indication stream
       SNS_BATCH_FIRST_IND - First indication in stream of back to back indications
       SNS_BATCH_INTERMEDIATE_IND - Intermediate indication in stream of back to back indications
       SNS_BATCH_LAST_IND - Last indication in stream of back to back indications
    */
}sns_smgr_buffering_ind_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Request Message; This command allows the client to request sensor samples from
    the SMGR current buffer. Often combined with requesting a buffering
    report with the report rate set to SNS_SMGR_BUFFERING_REPORT_RATE_NONE. */
typedef struct {

  /* Mandatory */
  uint16_t QueryId;
  /**<   The ID corresponding to a Buffering request
    The lower 8-bit value is the ReportId of the Buffering request initiated by same client
    The upper 8-bit value is the transaction ID assigned by client for each query
    Query response and indications for this request shall carry this QueryId
    */

  /* Mandatory */
  uint8_t SensorId;
  /**<   Identifies the sensor from which to collect data. */

  /* Mandatory */
  uint8_t DataType;
  /**<   Identifies the data type of the specified sensor */

  /* Mandatory */
  uint32_t TimePeriod[2];
  /**<   Specify the start and end of the time period within which to collect samples.
    - TimePeriod[0] is timestamp of the start of the time period
    - TimePeriod[1] is timestamp of the end of the time period
    */

  /* Optional */
  uint8_t SrcModule_valid;  /**< Must be set to true if SrcModule is being passed */
  uint8_t SrcModule;
  /**<   For sensor internal use only.
       Defines the source module that is sending this message.
    */
}sns_smgr_buffering_query_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Response Message; This command allows the client to request sensor samples from
    the SMGR current buffer. Often combined with requesting a buffering
    report with the report rate set to SNS_SMGR_BUFFERING_REPORT_RATE_NONE. */
typedef struct {

  /* Mandatory */
  sns_common_resp_s_v01 Resp;

  /* Optional */
  uint8_t QueryId_valid;  /**< Must be set to true if QueryId is being passed */
  uint16_t QueryId;
  /**<   The ID corresponding to a Query request */

  /* Optional */
  uint8_t AckNak_valid;  /**< Must be set to true if AckNak is being passed */
  uint8_t AckNak;
  /**<   Defines whether this response is Acknowledgement or Negative Acknowledgement
    - SNS_SMGR_RESPONSE_ACK_SUCCESS - the request has been accepted
    - SNS_SMGR_RESPONSE_NAK_RESOURCES - no resources to service the request
    - SNS_SMGR_RESPONSE_NAK_REPORT_ID - report not found for given ID
    - SNS_SMGR_RESPONSE_NAK_QUERY_ID - same request already received
    - SNS_SMGR_RESPONSE_NAK_TIME_PERIOD - the start of time period is not greater
        than end of time periodic
    - SNS_SMGR_RESPONSE_NAK_SENSOR_ID - requested sensor ID/data type is not in
        buffering request
    - SNS_SMGR_RESPONSE_NAK_INTERNAL_ERR - unspecified error
  */
}sns_smgr_buffering_query_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SMGR_SVC_qmi_messages
    @{
  */
/** Indication Message; This command allows the client to request sensor samples from
    the SMGR current buffer. Often combined with requesting a buffering
    report with the report rate set to SNS_SMGR_BUFFERING_REPORT_RATE_NONE. */
typedef struct {

  /* Mandatory */
  uint16_t QueryId;
  /**<   The ID corresponding to a Query request */

  /* Mandatory */
  uint32_t FirstSampleTimestamp;
  /**<   Timestamp of first sample belonging to this SensorId/DataType pair.
    */

  /* Mandatory */
  uint32_t SamplingRate;
  /**<   Specifies the actual frequency at which requested sensor is sampled.
       This value is expressed in Q16 format and in unit of Hz. */

  /* Mandatory */
  uint32_t Samples_len;  /**< Must be set to # of elements in Samples */
  sns_smgr_buffering_sample_s_v01 Samples[SNS_SMGR_BUFFERING_REPORT_MAX_SAMPLES_V01];
  /**<   Samples collected within requested time period */
}sns_smgr_buffering_query_ind_msg_v01;  /* Message */
/**
    @}
  */

/*Service Message Definition*/
/** @addtogroup SNS_SMGR_SVC_qmi_msg_ids
    @{
  */
#define SNS_SMGR_CANCEL_REQ_V01 0x0000
#define SNS_SMGR_CANCEL_RESP_V01 0x0000
#define SNS_SMGR_VERSION_REQ_V01 0x0001
#define SNS_SMGR_VERSION_RESP_V01 0x0001
#define SNS_SMGR_REPORT_REQ_V01 0x0002
#define SNS_SMGR_REPORT_RESP_V01 0x0002
#define SNS_SMGR_REPORT_IND_V01 0x0003
#define SNS_SMGR_CAL_REQ_V01 0x0004
#define SNS_SMGR_CAL_RESP_V01 0x0004
#define SNS_SMGR_ALL_SENSOR_INFO_REQ_V01 0x0005
#define SNS_SMGR_ALL_SENSOR_INFO_RESP_V01 0x0005
#define SNS_SMGR_SINGLE_SENSOR_INFO_REQ_V01 0x0006
#define SNS_SMGR_SINGLE_SENSOR_INFO_RESP_V01 0x0006
#define SNS_SMGR_SENSOR_TEST_REQ_V01 0x0007
#define SNS_SMGR_SENSOR_TEST_RESP_V01 0x0007
#define SNS_SMGR_SENSOR_POWER_STATUS_REQ_V01 0x0008
#define SNS_SMGR_SENSOR_POWER_STATUS_RESP_V01 0x0008
#define SNS_SMGR_SENSOR_POWER_STATUS_IND_V01 0x0009
#define SNS_SMGR_SENSOR_POWER_CONTROL_REQ_V01 0x000A
#define SNS_SMGR_SENSOR_POWER_CONTROL_RESP_V01 0x000A
#define SNS_SMGR_SENSOR_STATUS_REQ_V01 0x000B
#define SNS_SMGR_SENSOR_STATUS_RESP_V01 0x000B
#define SNS_SMGR_SENSOR_STATUS_IND_V01 0x000C
#define SNS_SMGR_SINGLE_SENSOR_TEST_REQ_V01 0x000D
#define SNS_SMGR_SINGLE_SENSOR_TEST_RESP_V01 0x000D
#define SNS_SMGR_SINGLE_SENSOR_TEST_IND_V01 0x000D
#define SNS_SMGR_BUFFERING_REQ_V01 0x0021
#define SNS_SMGR_BUFFERING_RESP_V01 0x0021
#define SNS_SMGR_BUFFERING_IND_V01 0x0022
#define SNS_SMGR_BUFFERING_QUERY_REQ_V01 0x0023
#define SNS_SMGR_BUFFERING_QUERY_RESP_V01 0x0023
#define SNS_SMGR_BUFFERING_QUERY_IND_V01 0x0024
/**
    @}
  */

/* Service Object Accessor */
/** @addtogroup wms_qmi_accessor 
    @{
  */
/** This function is used internally by the autogenerated code.  Clients should use the
   macro SNS_SMGR_SVC_get_service_object_v01( ) that takes in no arguments. */
qmi_idl_service_object_type SNS_SMGR_SVC_get_service_object_internal_v01
 ( int32_t idl_maj_version, int32_t idl_min_version, int32_t library_version );
 
/** This macro should be used to get the service object */ 
#define SNS_SMGR_SVC_get_service_object_v01( ) \
          SNS_SMGR_SVC_get_service_object_internal_v01( \
            SNS_SMGR_SVC_V01_IDL_MAJOR_VERS, SNS_SMGR_SVC_V01_IDL_MINOR_VERS, \
            SNS_SMGR_SVC_V01_IDL_TOOL_VERS )
/** 
    @} 
  */


#ifdef __cplusplus
}
#endif
#endif

