#ifndef SNS_SAM_PED_SVC_SERVICE_01_H
#define SNS_SAM_PED_SVC_SERVICE_01_H
/**
  @file sns_sam_ped_v01.h

  @brief This is the public header file which defines the SNS_SAM_PED_SVC service Data structures.

  This header file defines the types and structures that were defined in
  SNS_SAM_PED_SVC. It contains the constant values defined, enums, structures,
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
  
  Copyright (c) 2010-2013 Qualcomm Technologies, Inc.  All Rights Reserved
  Qualcomm Technologies Proprietary and Confidential.



  $Header$
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====* 
 *THIS IS AN AUTO GENERATED FILE. DO NOT ALTER IN ANY WAY
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/* This file was generated with Tool version 6.6 
   It was generated on: Fri Dec 20 2013 (Spin 0)
   From IDL File: sns_sam_ped_v01.idl */

/** @defgroup SNS_SAM_PED_SVC_qmi_consts Constant values defined in the IDL */
/** @defgroup SNS_SAM_PED_SVC_qmi_msg_ids Constant values for QMI message IDs */
/** @defgroup SNS_SAM_PED_SVC_qmi_enums Enumerated types used in QMI messages */
/** @defgroup SNS_SAM_PED_SVC_qmi_messages Structures sent as QMI messages */
/** @defgroup SNS_SAM_PED_SVC_qmi_aggregates Aggregate types used in QMI messages */
/** @defgroup SNS_SAM_PED_SVC_qmi_accessor Accessor for QMI service object */
/** @defgroup SNS_SAM_PED_SVC_qmi_version Constant values for versioning information */

#include <stdint.h>
#include "qmi_idl_lib.h"
#include "sns_sam_common_v01.h"
#include "sns_common_v01.h"


#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup SNS_SAM_PED_SVC_qmi_version
    @{
  */
/** Major Version Number of the IDL used to generate this file */
#define SNS_SAM_PED_SVC_V01_IDL_MAJOR_VERS 0x01
/** Revision Number of the IDL used to generate this file */
#define SNS_SAM_PED_SVC_V01_IDL_MINOR_VERS 0x06
/** Major Version Number of the qmi_idl_compiler used to generate this file */
#define SNS_SAM_PED_SVC_V01_IDL_TOOL_VERS 0x06
/** Maximum Defined Message ID */
#define SNS_SAM_PED_SVC_V01_MAX_MESSAGE_ID 0x0024
/**
    @}
  */


/** @addtogroup SNS_SAM_PED_SVC_qmi_consts 
    @{ 
  */

/**  Max number of reports in a batch indication -
     set based on max payload size supported by transport framework */
#define SNS_SAM_PED_MAX_ITEMS_IN_BATCH_V01 98
/**
    @}
  */

/** @addtogroup SNS_SAM_PED_SVC_qmi_aggregates
    @{
  */
typedef struct {

  uint8_t step_event;
  /**<  
    Indicates if a step has been detected since the last client initiated
    reset. If this flag is false, all other output fields are to be ignored.
    */

  uint8_t step_confidence;
  /**<  
    Confidence with which the latest step was detected, scaled to percentage -
    0 to 100.
    */

  uint32_t step_count;
  /**<  
    Count of steps detected since the last client initiated reset.
    */

  int32_t step_count_error;
  /**<  
    Error metric associated with reported step count, in steps.
    */

  float step_rate;
  /**<  
    Rate in Hz at which steps are detected since the last client report or
    reset (whichever is latest).
    */
}sns_sam_ped_report_data_s_v01;  /* Type */
/**
    @}
  */

/** @addtogroup SNS_SAM_PED_SVC_qmi_messages
    @{
  */
/** Request Message; This command enables the pedometer algorithm. */
typedef struct {

  /* Mandatory */
  uint32_t report_period;
  /**<  
    Specifies interval for periodic reporting period in seconds, Q16.
    P = 0 for asynchronous reporting.
    P > 0 for periodic reporting. Maximum reporting period is 3600 seconds.
    */

  /* Optional */
  uint8_t sample_rate_valid;  /**< Must be set to true if sample_rate is being passed */
  uint32_t sample_rate;
  /**<  
    Sampling rate in Hz, Q16. Default value of 20 Hz. Requires S >= 20 Hz.
    */

  /* Optional */
  uint8_t step_count_threshold_valid;  /**< Must be set to true if step_count_threshold is being passed */
  uint32_t step_count_threshold;
  /**<  
    The number of steps detected since the previous client report should exceed
    the step count threshold to trigger a new client report. This parameter is
    only used when report period is set to 0. Default value of 0 for
    notification on every step event. A value of 1 will send a notification on
    every other step and so on. Can range from 0 to 2^32-1.
    */

  /* Optional */
  uint8_t duty_cycle_on_percent_valid;  /**< Must be set to true if duty_cycle_on_percent is being passed */
  uint8_t duty_cycle_on_percent;
  /**<  
    Duty cycle ON percentage (0 to 100) for pedometer service when it is duty-cycled.
    Sensor stream to pedometer service is active only during the duty cycle ON period.
    */

  /* Optional */
  uint8_t notify_suspend_valid;  /**< Must be set to true if notify_suspend is being passed */
  sns_suspend_notification_s_v01 notify_suspend;
  /**<   Identifies if indications for this request should be sent
       when the processor is in suspend state.

       If this field is not specified, default value will be set to
       notify_suspend->proc_type                  = SNS_PROC_APPS
       notify_suspend->send_indications_during_suspend  = FALSE

       This field does not have any bearing on error indication
       messages, which will be sent even during suspend.
    */
}sns_sam_ped_enable_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SAM_PED_SVC_qmi_messages
    @{
  */
/** Response Message; This command enables the pedometer algorithm. */
typedef struct {

  /* Mandatory */
  sns_common_resp_s_v01 resp;

  /* Optional */
  uint8_t instance_id_valid;  /**< Must be set to true if instance_id is being passed */
  uint8_t instance_id;
  /**<  
    Algorithm instance ID maintained/assigned by SAM.
    The client shall use this instance ID for future messages associated with
    current algorithm instance.
    */
}sns_sam_ped_enable_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SAM_PED_SVC_qmi_messages
    @{
  */
/** Request Message; This command disables the pedometer algorithm. */
typedef struct {

  /* Mandatory */
  uint8_t instance_id;
  /**<   To identify the algorithm instance to be disabled.  */
}sns_sam_ped_disable_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SAM_PED_SVC_qmi_messages
    @{
  */
/** Response Message; This command disables the pedometer algorithm. */
typedef struct {

  /* Mandatory */
  sns_common_resp_s_v01 resp;

  /* Optional */
  uint8_t instance_id_valid;  /**< Must be set to true if instance_id is being passed */
  uint8_t instance_id;
  /**<   Instance id identifies the algorithm instance.  */
}sns_sam_ped_disable_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SAM_PED_SVC_qmi_messages
    @{
  */
/** Request Message; This command fetches latest report output of pedometer algorithm. */
typedef struct {

  /* Mandatory */
  uint8_t instance_id;
  /**<   Instance id identifies the algorithm instance.  */
}sns_sam_ped_get_report_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SAM_PED_SVC_qmi_messages
    @{
  */
/** Response Message; This command fetches latest report output of pedometer algorithm. */
typedef struct {

  /* Mandatory */
  sns_common_resp_s_v01 resp;

  /* Optional */
  uint8_t instance_id_valid;  /**< Must be set to true if instance_id is being passed */
  uint8_t instance_id;
  /**<   Instance id identifies the algorithm instance.  */

  /* Optional */
  uint8_t timestamp_valid;  /**< Must be set to true if timestamp is being passed */
  uint32_t timestamp;
  /**<   Timestamp of input with which latest step was detected; in SSC ticks  */

  /* Optional */
  uint8_t report_data_valid;  /**< Must be set to true if report_data is being passed */
  sns_sam_ped_report_data_s_v01 report_data;
  /**<   pedometer algorithm output report  */
}sns_sam_ped_get_report_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SAM_PED_SVC_qmi_messages
    @{
  */
/** Indication Message; Output report from the pedometer algorithm. */
typedef struct {

  /* Mandatory */
  uint8_t instance_id;
  /**<   Instance id identifies the algorithm instance.  */

  /* Mandatory */
  uint32_t timestamp;
  /**<   Timestamp of input with which latest step was detected; in SSC ticks */

  /* Mandatory */
  sns_sam_ped_report_data_s_v01 report_data;
  /**<   pedometer algorithm output report  */
}sns_sam_ped_report_ind_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SAM_PED_SVC_qmi_messages
    @{
  */
/** Indication Message; Asynchronous error report from the pedometer algorithm. */
typedef struct {

  /* Mandatory */
  uint8_t error;
  /**<   sensors error code */

  /* Mandatory */
  uint8_t instance_id;
  /**<   Instance id identifies the algorithm instance.  */

  /* Mandatory */
  uint32_t timestamp;
  /**<   Timestamp of when the error was detected; in SSC ticks */
}sns_sam_ped_error_ind_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SAM_PED_SVC_qmi_messages
    @{
  */
/** Request Message; This command resets all stats generated by the pedometer algorithm. */
typedef struct {

  /* Mandatory */
  uint8_t instance_id;
  /**<   Instance id identifies the algorithm instance.  */
}sns_sam_ped_reset_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SAM_PED_SVC_qmi_messages
    @{
  */
/** Response Message; This command resets all stats generated by the pedometer algorithm. */
typedef struct {

  /* Mandatory */
  sns_common_resp_s_v01 resp;

  /* Optional */
  uint8_t instance_id_valid;  /**< Must be set to true if instance_id is being passed */
  uint8_t instance_id;
  /**<   Instance id identifies the algorithm instance.  */
}sns_sam_ped_reset_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SAM_PED_SVC_qmi_messages
    @{
  */
/** Request Message; This command handles batch mode for the pedometer algorithm. */
typedef struct {

  /* Mandatory */
  uint8_t instance_id;
  /**<   Instance id identifies the algorithm instance.  */

  /* Mandatory */
  int32_t batch_period;
  /**<   Specifies interval over which reports are to be batched in seconds, Q16.
       P = 0 to disable batching.
       P > 0 to enable batching.
    */

  /* Optional */
  uint8_t req_type_valid;  /**< Must be set to true if req_type is being passed */
  sns_batch_req_type_e_v01 req_type;
  /**<   Optional request type
       0 – Do not wake client from suspend when buffer is full.
       1 – Wake client from suspend when buffer is full.
       2 – Use to get max buffer depth. Does not enable/disable batching.
           instance_id and batch_period are ignored for this request type.
       Defaults to 0.
    */
}sns_sam_ped_batch_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SAM_PED_SVC_qmi_messages
    @{
  */
/** Response Message; This command handles batch mode for the pedometer algorithm. */
typedef struct {

  /* Mandatory */
  sns_common_resp_s_v01 resp;

  /* Optional */
  uint8_t instance_id_valid;  /**< Must be set to true if instance_id is being passed */
  uint8_t instance_id;
  /**<   Algorithm instance ID maintained/assigned by SAM */

  /* Optional */
  uint8_t max_batch_size_valid;  /**< Must be set to true if max_batch_size is being passed */
  uint32_t max_batch_size;
  /**<   Max supported batch size */

  /* Optional */
  uint8_t timestamp_valid;  /**< Must be set to true if timestamp is being passed */
  uint32_t timestamp;
  /**<   Timestamp when the batch request was processed in SSC ticks */
}sns_sam_ped_batch_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SAM_PED_SVC_qmi_aggregates
    @{
  */
typedef struct {

  sns_sam_ped_report_data_s_v01 report;
  /**<   Pedometer algorithm output report */

  uint32_t timestamp;
  /**<   Timestamp of input with which latest step in report was detected; in SSC ticks */
}sns_sam_ped_batch_item_s_v01;  /* Type */
/**
    @}
  */

/** @addtogroup SNS_SAM_PED_SVC_qmi_messages
    @{
  */
/** Indication Message; Report containing a batch of algorithm outputs */
typedef struct {

  /* Mandatory */
  uint8_t instance_id;
  /**<   Instance id identifies the algorithm instance */

  /* Mandatory */
  uint32_t items_len;  /**< Must be set to # of elements in items */
  sns_sam_ped_batch_item_s_v01 items[SNS_SAM_PED_MAX_ITEMS_IN_BATCH_V01];
  /**<   Pedometer algorithm output reports */

  /* Optional */
  uint8_t ind_type_valid;  /**< Must be set to true if ind_type is being passed */
  uint8_t ind_type;
  /**<   Optional batch indication type
       SNS_BATCH_ONLY_IND - Standalone batch indication. Not part of a back to back indication stream
       SNS_BATCH_FIRST_IND - First indication in stream of back to back indications
       SNS_BATCH_INTERMEDIATE_IND - Intermediate indication in stream of back to back indications
       SNS_BATCH_LAST_IND - Last indication in stream of back to back indications
    */
}sns_sam_ped_batch_ind_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SAM_PED_SVC_qmi_messages
    @{
  */
/** Request Message; This command updates active batch period for a Pedometer algorithm
           when batching is ongoing. */
typedef struct {

  /* Mandatory */
  uint8_t instance_id;
  /**<   Instance ID identifies the algorithm instance.  */

  /* Mandatory */
  int32_t active_batch_period;
  /**<   Specifies new interval (in seconds, Q16) over which reports are to be
       batched when the client processor is awake. Only takes effect if
       batching is ongoing.
       P > 0 to to override active batch period set in batch enable request.
       P = 0 to switch to active batch period set in batch enable request.
    */
}sns_sam_ped_update_batch_period_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup SNS_SAM_PED_SVC_qmi_messages
    @{
  */
/** Response Message; This command updates active batch period for a Pedometer algorithm
           when batching is ongoing. */
typedef struct {

  /* Mandatory */
  sns_common_resp_s_v01 resp;

  /* Optional */
  uint8_t instance_id_valid;  /**< Must be set to true if instance_id is being passed */
  uint8_t instance_id;
  /**<   Algorithm instance ID maintained/assigned by SAM */

  /* Optional */
  uint8_t timestamp_valid;  /**< Must be set to true if timestamp is being passed */
  uint32_t timestamp;
  /**<   Timestamp when the batch request was processed in SSC ticks */
}sns_sam_ped_update_batch_period_resp_msg_v01;  /* Message */
/**
    @}
  */

/*Service Message Definition*/
/** @addtogroup SNS_SAM_PED_SVC_qmi_msg_ids
    @{
  */
#define SNS_SAM_PED_CANCEL_REQ_V01 0x0000
#define SNS_SAM_PED_CANCEL_RESP_V01 0x0000
#define SNS_SAM_PED_VERSION_REQ_V01 0x0001
#define SNS_SAM_PED_VERSION_RESP_V01 0x0001
#define SNS_SAM_PED_ENABLE_REQ_V01 0x0002
#define SNS_SAM_PED_ENABLE_RESP_V01 0x0002
#define SNS_SAM_PED_DISABLE_REQ_V01 0x0003
#define SNS_SAM_PED_DISABLE_RESP_V01 0x0003
#define SNS_SAM_PED_GET_REPORT_REQ_V01 0x0004
#define SNS_SAM_PED_GET_REPORT_RESP_V01 0x0004
#define SNS_SAM_PED_REPORT_IND_V01 0x0005
#define SNS_SAM_PED_ERROR_IND_V01 0x0006
#define SNS_SAM_PED_RESET_REQ_V01 0x0020
#define SNS_SAM_PED_RESET_RESP_V01 0x0020
#define SNS_SAM_PED_BATCH_REQ_V01 0x0021
#define SNS_SAM_PED_BATCH_RESP_V01 0x0021
#define SNS_SAM_PED_BATCH_IND_V01 0x0022
#define SNS_SAM_PED_UPDATE_BATCH_PERIOD_REQ_V01 0x0023
#define SNS_SAM_PED_UPDATE_BATCH_PERIOD_RESP_V01 0x0023
#define SNS_SAM_PED_GET_ATTRIBUTES_REQ_V01 0x0024
#define SNS_SAM_PED_GET_ATTRIBUTES_RESP_V01 0x0024
/**
    @}
  */

/* Service Object Accessor */
/** @addtogroup wms_qmi_accessor 
    @{
  */
/** This function is used internally by the autogenerated code.  Clients should use the
   macro SNS_SAM_PED_SVC_get_service_object_v01( ) that takes in no arguments. */
qmi_idl_service_object_type SNS_SAM_PED_SVC_get_service_object_internal_v01
 ( int32_t idl_maj_version, int32_t idl_min_version, int32_t library_version );
 
/** This macro should be used to get the service object */ 
#define SNS_SAM_PED_SVC_get_service_object_v01( ) \
          SNS_SAM_PED_SVC_get_service_object_internal_v01( \
            SNS_SAM_PED_SVC_V01_IDL_MAJOR_VERS, SNS_SAM_PED_SVC_V01_IDL_MINOR_VERS, \
            SNS_SAM_PED_SVC_V01_IDL_TOOL_VERS )
/** 
    @} 
  */


#ifdef __cplusplus
}
#endif
#endif

