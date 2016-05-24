/******************************************************************************
  @file:  gsiff_sensor_provider_common.h
  @brief: GNSS / Sensor Interface Framework Support

  DESCRIPTION
    This file defines the interface to common methods shared between GSIFF
    sensor providers

  INITIALIZATION AND SEQUENCING REQUIREMENTS

  -----------------------------------------------------------------------------
  Copyright (c) 2011 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------

******************************************************************************/
/*
  * Copyright (c) 2012 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

/*=====================================================================
                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why
--------   ---      -------------------------------------------------------
05/01/11   jb       Initial version
08/01/11   jb       1. Misc typedef changes

======================================================================*/
#ifndef __GSIFF_SENSOR_PROVIDER_COMMON_H__
#define __GSIFF_SENSOR_PROVIDER_COMMON_H__

#include <stdbool.h>
#include "gsiff_sensor_provider_glue.h"
#include "gsiff_sensor_provider_common.h"
#include "gsiff_msg.h"

/*===========================================================================
FUNCTION    sp_send_sensor_data_batch

DESCRIPTION
   Helper function to look at the given QMI TLV. It will inspect the amount of
   data in the batch and will send to GSIFF data task if the total number of
   samples exceeds the max number of samples or if the interval of samples in
   ms exceeds the batching interval in ms.

   p_msg_q:            Message queue to send message to.
   sensor_str:         String name of sensor to use in logging
   p_gsiff_data_msg:   GSIFF message with sensor data message to send to GSIFF control task if batch exceeded.
   p_sensor_data_tlv:  TLV tested for either max number of samples or exceeding batch interval for these samples (accel or gyro).
   p_sensor_temp_tlv:  TLV tested for either max number of samples or exceeding batch interval for these samples (accel or gyro temperature).
   p_baro_data:        TLV tested for either max number of samples or exceeding batch interval for these samples (baro)
   sample_interval:    Length of a sample in milliseconds. (1000 / sampling rate)
   batching_interval:  Length of a batch of samples in milliseconds (1000 / batching rate)

DEPENDENCIES
   N/A

RETURN VALUE
   N/A

SIDE EFFECTS
   N/A

===========================================================================*/
void sp_send_sensor_data_batch(void* p_msg_q, const char* sensor_str, gsiff_msg* p_gsiff_data_msg,
                               qmiLoc3AxisSensorSampleListStructT_v02* p_sensor_data_tlv,
                               qmiLocSensorTemperatureSampleListStructT_v02* p_sensor_temp_tlv,
                               double sample_interval,
                               double batching_interval);

/*===========================================================================
FUNCTION    sp_process_raw_sensor_data

DESCRIPTION
   Helper function to process the raw sensor samples, log the sample and place
   the sample with timestamp into the proper QMI TLV location.

   Note: Assumes samples are in order based on timestamp for sample and the previous
         samples found in p_sensor_data_tlv. Make sure they are inserted in order
         based on a monotonically increasing timestamp.

   sensor_str:         String name of sensor to use in logging
   data:               Array of 3 data points in sensor sample (0=x, 1=y, 2=z)
   timestamp_ms:       Timestamp of the sensor sample in milliseconds
   p_sensor_data_tlv:  TLV of where to add the sensor sample to the end of.
   p_sensor_temp_tlv:  TLV of where to add the sensor temperature sample to the end of.
   p_baro_data:        TLV tested for either max number of samples or exceeding batch interval for these samples (baro)
DEPENDENCIES
   N/A

RETURN VALUE
   N/A

SIDE EFFECTS
   N/A

===========================================================================*/
void sp_process_raw_sensor_data(const char* sensor_str, float data[3], uint32_t timestamp_ms,
                                qmiLoc3AxisSensorSampleListStructT_v02* p_sensor_data_tlv,
                                qmiLocSensorTemperatureSampleListStructT_v02* p_sensor_temp_tlv);



/*===========================================================================
  FUNCTION    sp_send_baro_data_batch

  DESCRIPTION
  Helper function to look at the given PIP TLV. It will inspect the amount of
  data in the batch and will trigger socket write to PIP if the total number of
  samples exceeds the max number of samples or if the interval of samples in
  ms exceeds the batching interval in ms.

  p_msg_q:            Message queue to send message to.
  sensor_str:         String name of sensor to use in logging
  p_gsiff_data_msg:   GSIFF message with sensor data message to send to GSIFF control task if batch
  exceeded.
  p_baro_data:        TLV tested for either max number of samples or exceeding batch interval for t
  hese samples (baro).
  sample_interval:    Length of a sample in milliseconds. (1000 / sampling rate)
  batching_interval:  Length of a batch of samples in milliseconds (1000 / batching rate)

  DEPENDENCIES
  N/A

  RETURN VALUE
  N/A

  SIDE EFFECTS
  N/A

  ===========================================================================*/
void sp_send_baro_data_batch(void* p_msg_q, const char* sensor_str, gsiff_msg* p_gsiff_data_msg,
                             slimBaroDataPayloadStructT** p_baro_data,
                             double sample_interval,
                             double batching_interval,
                             int reporting_rate);

/*===========================================================================
  FUNCTION    sp_process_raw_baro_data
  142
  DESCRIPTION
  Helper function to process the raw sensor samples, log the sample and place
  the sample with timestamp into the proper PIP TLV location.
  146
  Note: Assumes samples are in order based on timestamp for sample and the previous
  samples found in p_baro_data. Make sure they are inserted in order
  based on a monotonically increasing timestamp.

  sensor_str:         String name of sensor to use in logging
  data:               Single baro value
  timestamp_ms:       Timestamp of the sensor sample in milliseconds
  p_baro_data:        TLV of where to add the sensor sample to the end of.

  DEPENDENCIES
  N/A
  159
  RETURN VALUE
  N/A

  SIDE EFFECTS
  N/A

  ===========================================================================*/
void sp_process_raw_baro_data(const char* sensor_str, float data, uint32_t timestamp_ms,
                              slimBaroDataPayloadStructT** p_baro_data);


/*===========================================================================
FUNCTION    sp_send_msi_mounted_status

DESCRIPTION
   Simple convenience function to add a mounted state message to the
   gsiff_daemon data_task message queue.

   p_msg_q: Message queue to send message to.
   mounted_state: Specify a mounted state qmi message to send.
                  (0=Unmounted, 1=Mounted, 2=Do not send MSI)

DEPENDENCIES
   N/A

RETURN VALUE
   0/FALSE : Failure
   1/TRUE  : Successful

SIDE EFFECTS
   N/A

===========================================================================*/
bool sp_send_msi_mounted_status(void* p_msg_q, uint8_t mounted_state);

/*===========================================================================
FUNCTION    sp_get_sensor_time_offset_ms

DESCRIPTION
   Function to return the current sensor time offset. Needed to implement
   testing to fake time jumps for specified durations.

DEPENDENCIES
   N/A

RETURN VALUE
   Current sensor time offset in milliseconds

SIDE EFFECTS
   N/A

===========================================================================*/
int32_t sp_get_sensor_time_offset_ms();

/*===========================================================================
FUNCTION    sp_apply_sensor_time_offset_ms

DESCRIPTION
   Function to change the sensor time offset for testing. Allows user to modify
   sensor time stamps for a limited duration.

   time_offset_ms: Amount to modify sensor timestamps by in milliseconds.
   duration_ms:    Length of time to apply the sensor time offset in milliseconds.

DEPENDENCIES
   N/A

RETURN VALUE
   N/A

SIDE EFFECTS
   N/A

===========================================================================*/
void sp_apply_sensor_time_offset_ms(int32_t time_offset_ms, uint32_t duration_ms);

/*===========================================================================
FUNCTION    sp_read_sys_time_ms

DESCRIPTION
   Utility function to return the current system time in milliseconds.

DEPENDENCIES
   N/A

RETURN VALUE
   Current system time in milliseconds.

SIDE EFFECTS
   N/A

===========================================================================*/
uint32_t sp_read_sys_time_ms();

/*===========================================================================
FUNCTION    sp_read_sys_time_us

DESCRIPTION
   Utility function to return the current system time in microseconds.

DEPENDENCIES
   N/A

RETURN VALUE
   Current system time in milliseconds.

SIDE EFFECTS
   N/A

===========================================================================*/
uint64_t sp_read_sys_time_us();

/*===========================================================================
FUNCTION    sp_calc_time_diff_ms

DESCRIPTION
   Utility function to return the difference in two timestamps in milliseconds.
   Accounts for any timestamp rollovers due to the limited range of milliseconds
   in uint32. (Rolls over ~ every 50 days)

   ts1_ms:  First timestamp in milliseconds. (Comes first in monotonic time)
   ts2_ms:  Second timestamp in milliseconds. (Comes second in monotonic time)

DEPENDENCIES
   N/A

RETURN VALUE
   Time difference in milliseconds

SIDE EFFECTS
   N/A

===========================================================================*/
uint32_t sp_calc_time_diff_ms(uint32_t ts1_ms, uint32_t ts2_ms);

/*===========================================================================
FUNCTION    sp_calc_time_diff_ms

DESCRIPTION
   Utility function to copy data to a new allocated (heap) location and send
   through the message queue to the main processing data thread in GSIFF.
   (gsiff_data_task)

   msg_q_data: Message queue to send message to.
   msg_obj:    Pointer to buffer to send to msg_q.
   msg_sz:     Size of message to send to msg_q

DEPENDENCIES
   N/A

RETURN VALUE
   0/FALSE : Failure
   1/TRUE  : Successful

SIDE EFFECTS
   N/A

===========================================================================*/
bool sp_msg_q_snd(void* msg_q_data, void* msg_obj, uint32_t msg_sz);

#endif /* __GSIFF_SENSOR_PROVIDER_COMMON_H__ */
