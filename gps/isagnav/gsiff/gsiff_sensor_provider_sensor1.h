/******************************************************************************
  @file:  gsiff_sensor_provider_sensor1.h
  @brief: GNSS / Sensor Interface Framework Support

  DESCRIPTION
    This file defines the interface to accessing sensor data through Sensor1
    API.

  INITIALIZATION AND SEQUENCING REQUIREMENTS

  -----------------------------------------------------------------------------
  Copyright (c) 2011 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------

******************************************************************************/

/*=====================================================================
                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why
--------   ---      -------------------------------------------------------
05/01/11   jb       Initial version
08/01/11   jb       1. Misc typedef changes

======================================================================*/
#ifndef __GSIFF_SENSOR_PROVIDER_SENSOR1_H__
#define __GSIFF_SENSOR_PROVIDER_SENSOR1_H__

#include <stdbool.h>
#include "gsiff_sensor_provider_glue.h"
#include "sensor1.h"

sensor1_handle_s *p_sensor1_hndl;
sensor1_error_e sensor1_open_result;

/*===========================================================================
FUNCTION    sp_sns1_init

DESCRIPTION
   Initializes internal structures for sensor provider.

   p_msg_q: Message Queue to place sensor/spi/msi data in using gsiff_msg structures.

   Note: Function should not be called from a time sensitive thread as this
         function may block for sensor initialization.

DEPENDENCIES
   N/A

RETURN VALUE
   0/FALSE : Failure
   1/TRUE  : Successful

SIDE EFFECTS
   N/A

===========================================================================*/
bool sp_sns1_init(void* p_msg_q);

/*===========================================================================
FUNCTION    sp_sns1_update_accel_status

DESCRIPTION
  Function updates the current running status of the accelerometer from sensor
  provider. If accel is already reporting sensor data and it is requested to
  start again, this function is a nop. If accel is already stopped and it is
  requested to stop this function is a nop.

  running:       TRUE - start sensor reporting, FALSE - stop sensor reporting
  sampling_rate: Sampling rate in Hz
  batching_rate: Batch samples at this frequency in Hz
  mounted_state: 0 - Send Unmounted State, 1 - Send Mounted State, 2 - Do Not Send

  Note: Function should not be called from a time sensitive thread as this
        function may block for some time to start up sensors.

DEPENDENCIES
   N/A

RETURN VALUE
   0/FALSE : Failure
   1/TRUE  : Successful

SIDE EFFECTS
   N/A

===========================================================================*/
bool sp_sns1_update_accel_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state);

/*===========================================================================
FUNCTION    sp_sns1_update_gyro_status

DESCRIPTION
  Function updates the current running status of the gyroscope from sensor
  provider. If gyro is already reporting sensor data and it is requested to
  start again, this function is a nop. If gyro is already stopped and it is
  requested to stop this function is a nop.

  running:       TRUE - start sensor reporting, FALSE - stop sensor reporting
  sampling_rate: Sampling rate in Hz
  batching_rate: Batch samples at this frequency in Hz
  mounted_state: 0 - Send Unmounted State, 1 - Send Mounted State, 2 - Do Not Send

  Note: Function should not be called from a time sensitive thread as this
        function may block for some time to start up sensors.

DEPENDENCIES
   N/A

RETURN VALUE
   0/FALSE : Failure
   1/TRUE  : Successful

SIDE EFFECTS
   N/A

===========================================================================*/
bool sp_sns1_update_gyro_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state);

/*===========================================================================
FUNCTION    sp_sns1_update_motion_data

DESCRIPTION
  Function updates the current running status of the Motion Data (MD)
  algorithm from sensor provider. If MD is already reporting
  and it is requested to start again, this function is a nop. If MD is already
  stopped and it is requested to stop this function is a nop.

  running:       TRUE - start SPI reporting, FALSE - stop SPI reporting

  Note: Function should not be called from a time sensitive thread as this
        function may block for some time to start up sensors.

DEPENDENCIES
   N/A

RETURN VALUE
   0/FALSE : Failure
   1/TRUE  : Successful

SIDE EFFECTS
   N/A

===========================================================================*/
bool sp_sns1_update_motion_data(bool running);

/*===========================================================================
FUNCTION    sp_sns1_update_pedometer

DESCRIPTION
  Function updates the current running status of pedometer updates from this
  sensor provider.

  running:                TRUE - start pedometer reportig,
                          FALSE - stop pedometer reporting
  reset_step_count:       1 - reset step count,
                          0 - do not reset step count
  step_count_threshold:   0 - keep the previous value or use default value 1
                              if there is no previous value,
                          N - report after every Nth step

  Note: Function should not be called from a time sensitive thread as this
        function may block for some time to start up spi reporting.

DEPENDENCIES
   N/A

RETURN VALUE
   0/FALSE : Failure
   1/TRUE  : Successful

SIDE EFFECTS
   N/A

===========================================================================*/
bool sp_sns1_update_pedometer(bool running, uint8_t reset_step_count, uint32_t step_count_threshold);

/*===========================================================================
FUNCTION    sp_sns1_update_spi_status

DESCRIPTION
  Function updates the current running status of the Stationary Position
  Indicator (SPI) algorithm from sensor provider. If SPI is already reporting
  and it is requested to start again, this function is a nop. If SPI is already
  stopped and it is requested to stop this function is a nop.

  running:       TRUE - start SPI reporting, FALSE - stop SPI reporting

  Note: Function should not be called from a time sensitive thread as this
        function may block for some time to start up sensors.

DEPENDENCIES
   N/A

RETURN VALUE
   0/FALSE : Failure
   1/TRUE  : Successful

SIDE EFFECTS
   N/A

===========================================================================*/
bool sp_sns1_update_spi_status(bool running);

/*===========================================================================
FUNCTION    sp_sns1_update_accel_temperature_status

DESCRIPTION
  Function updates the current running status of the accelerometer temperature
  from sensor provider. If accel is already reporting temperature data and it
  is requested to start again, this function is a nop. If accel temperature is
  already stopped and it is requested to stop this function is a nop.

  running:       TRUE - start accel temperature reporting
                 FALSE - stop accel temperature reporting
  sampling_rate: Sampling rate in Hz
  batching_rate: Batch samples at this frequency in Hz
  mounted_state: 0 - Send Unmounted State, 1 - Send Mounted State, 2 - Do Not Send

  Note: Function should not be called from a time sensitive thread as this
        function may block for some time to start up sensors.

DEPENDENCIES
   N/A

RETURN VALUE
   0/FALSE : Failure
   1/TRUE  : Successful

SIDE EFFECTS
   N/A

===========================================================================*/
bool sp_sns1_update_accel_temperature_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state);

/*===========================================================================
FUNCTION    sp_sns1_update_gyro_temperature_status

DESCRIPTION
  Function updates the current running status of the gyroscope temperature
  from sensor provider. If gyro is already reporting temperature data and it
  is requested to start again, this function is a nop. If gyro temperature is
  already stopped and it is requested to stop this function is a nop.

  running:       TRUE - start gyro temperature reporting
                 FALSE - stop gyro temperature reporting
  sampling_rate: Sampling rate in Hz
  batching_rate: Batch samples at this frequency in Hz
  mounted_state: 0 - Send Unmounted State, 1 - Send Mounted State, 2 - Do Not Send

  Note: Function should not be called from a time sensitive thread as this
        function may block for some time to start up sensors.

DEPENDENCIES
   N/A

RETURN VALUE
   0/FALSE : Failure
   1/TRUE  : Successful

SIDE EFFECTS
   N/A

===========================================================================*/
bool sp_sns1_update_gyro_temperature_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state);


/*===========================================================================
FUNCTION    sp_sns1_update_baro_status

DESCRIPTION
  Function updates the current running status of the gyroscope temperature
  from sensor provider. If gyro is already reporting temperature data and it
  is requested to start again, this function is a nop. If gyro temperature is
  already stopped and it is requested to stop this function is a nop.

  running:       TRUE - start gyro temperature reporting
                 FALSE - stop gyro temperature reporting
  sampling_rate: Sampling rate in Hz
  batching_rate: Batch samples at this frequency in Hz
  mounted_state: 0 - Send Unmounted State, 1 - Send Mounted State, 2 - Do Not Send

  Note: Function should not be called from a time sensitive thread as this
        function may block for some time to start up sensors.

DEPENDENCIES
   N/A

RETURN VALUE
   0/FALSE : Failure
   1/TRUE  : Successful

SIDE EFFECTS
   N/A

===========================================================================*/
bool sp_sns1_update_baro_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state);

/*===========================================================================
FUNCTION    sp_sns1_destroy

DESCRIPTION
  Function cleans up any resources the sensor provider may have been using.
  It will also stop reporting sensor data if it was not stopped previously.

DEPENDENCIES
   N/A

RETURN VALUE
   0/FALSE : Failure
   1/TRUE  : Successful

SIDE EFFECTS
   N/A

===========================================================================*/
bool sp_sns1_destroy();

/*===========================================================================
FUNCTION    sp_sns1_get_sensor_time

DESCRIPTION
  Function returns the current sensor time in milliseconds.

  p_time_ms: Out parameter holding timestamp in milliseconds

  Note: Function should not be called from a time sensitive thread as this
        function may block for some time to query the sensor time.

DEPENDENCIES
   N/A

RETURN VALUE
   1 : Success
   0 : Failure to read sensor time

SIDE EFFECTS
   N/A

===========================================================================*/
bool sp_sns1_get_sensor_time(uint32_t* p_time_ms);


/*===========================================================================
FUNCTION    sensor1_notify_data_callback

DESCRIPTION
  Main callback function for incoming Sensor1 Data.

  Note: Called from an internal Sensor1 thread context. Do NOT perform
        expensive operations in this thread as it can greatly reduce the
        throughput of DSPS.

DEPENDENCIES
   N/A

RETURN VALUE
   N/A

SIDE EFFECTS
   N/A

===========================================================================*/
void sensor1_notify_data_callback(intptr_t cb_data,
                                  sensor1_msg_header_s *msg_hdr,
                                  sensor1_msg_type_e msg_type,
                                  void* msg_ptr);


#endif /* __GSIFF_SENSOR_PROVIDER_SENSOR1_H__ */
