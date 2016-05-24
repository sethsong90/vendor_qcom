/******************************************************************************
  @file:  gsiff_sensor_provider_glue.h
  @brief: GNSS / Sensor Interface Framework Support

  DESCRIPTION
    This file defines a generic interface to be used when communicating with
    either Sensor1 API or Native Android API for sensor samples or sensor
    timing information.

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
#ifndef __GSIFF_SENSOR_PROVIDER_GLUE_H__
#define __GSIFF_SENSOR_PROVIDER_GLUE_H__

#include <stdbool.h>
#include <stdint.h>
#include "msg_q.h"

/**
 * Specifies the different inputs for mounted state indicators.
 */
#define SP_MSI_UNMOUNTED      ((uint8_t)0)
#define SP_MSI_MOUNTED        ((uint8_t)1)
#define SP_MSI_UNKNOWN        ((uint8_t)2)
#define SP_MSI_DO_NOT_SEND    ((uint8_t)3)

/**
 * Different types of supported sensors
 */
typedef enum e_sensor_type {
   ACCELEROMETER_SENSOR_TYPE,
   GYROSCOPE_SENSOR_TYPE,
   ACCELEROMETER_TEMPERATURE_SENSOR_TYPE,
   GYROSCOPE_TEMPERATURE_SENSOR_TYPE,
   BAROMETER_SENSOR_TYPE
} e_sensor_type;

/**
 * Different data flow paths supported for
 * retrieving sensor data.
 */
typedef enum e_sensor_provider_type {
   MIN_SENSOR_PROVIDER = 0,

   SENSOR1_SENSOR_PROVIDER,
   ANDROID_NDK_SENSOR_PROVIDER,
   ANDROID_HAL_SENSOR_PROVIDER,

   MAX_SENSOR_PROVIDER,
} e_sensor_provider_type;

/*===========================================================================
FUNCTION    sp_init

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
bool sp_init(void* p_msg_q, e_sensor_provider_type provider_type);

/*===========================================================================
FUNCTION    sp_update_accel_status

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
bool sp_update_accel_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state);

/*===========================================================================
FUNCTION    sp_update_gyro_status

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
bool sp_update_gyro_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state);

/*===========================================================================
FUNCTION    sp_update_accel_temperature_status

DESCRIPTION
  Function updates the current running status of the accelerometer temperature from
  sensor provider. If accel is already reporting sensor temperature and it is requested to
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
bool sp_update_accel_temperature_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state);

/*===========================================================================
FUNCTION    sp_update_gyro_temperature_status

DESCRIPTION
  Function updates the current running status of the gyroscope temperature from sensor
  provider. If gyro is already reporting sensor temperature and it is requested to
  start again, this function is a nop. If gyro temperature is already stopped and it is
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
bool sp_update_gyro_temperature_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state);

/*===========================================================================
FUNCTION    sp_update_motion_data

DESCRIPTION
  Function updates the current running status of the Motion Data algorithm
  (MD) from sensor provider. If MD is already reporting
  and it is requested to start again, this function is a nop. If MD is already
  stopped and it is requested to stop this function is a nop.

  running:       TRUE - start MD reporting, FALSE - stop MD reporting

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
bool sp_update_motion_data(bool running);

/*===========================================================================
FUNCTION    sp_update_pedometer

DESCRIPTION
  Function updates the current running status of the Pedometer algorithm
  from sensor provider. If Pedometer is already running and it is requested to
  start again, the step count may be reset or the step count threshold changed.
  If Pedometer is already stopped and it is requested to stop, this function is a nop.

  running:                TRUE - start pedometer reporting,
                          FALSE - stop pedometer reporting
  reset_step_count:       1 - reset step count,
                          0 - do not reset step count
  step_count_threshold:   0 - keep the previous value or use default value 1
                              if there is no previous value,
                          N - report after every Nth step

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
bool sp_update_pedometer(bool running, uint8_t reset_step_count, uint32_t step_count_threshold);

/*===========================================================================
FUNCTION    sp_update_spi_status

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
bool sp_update_spi_status(bool running);

/*===========================================================================
FUNCTION    sp_destroy

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
bool sp_destroy();

/*===========================================================================
FUNCTION    sp_get_sensor_time

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
bool sp_get_sensor_time(uint32_t* p_time_ms);

#endif /* __GSIFF_SENSOR_PROVIDER_GLUE_H__ */
