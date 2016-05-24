/******************************************************************************
  @file:  gsiff_sensor_provider_and_hal.h
  @brief: GNSS / Sensor Interface Framework Support

  DESCRIPTION
    This file defines the interface to accessing sensor data through Native Android
    HAL API.

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
#ifndef __GSIFF_SENSOR_PROVIDER_ANDROID_HAL_H__
#define __GSIFF_SENSOR_PROVIDER_ANDROID_HAL_H__

#include <stdbool.h>
#include "gsiff_sensor_provider_glue.h"

/*===========================================================================
FUNCTION    sp_and_hal_init

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
bool sp_and_hal_init(void* p_msg_q);

/*===========================================================================
FUNCTION    sp_and_hal_update_accel_status

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
bool sp_and_hal_update_accel_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state);

/*===========================================================================
FUNCTION    sp_and_hal_update_gyro_status

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
bool sp_and_hal_update_gyro_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state);

/*===========================================================================
FUNCTION    sp_and_hal_destroy

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
bool sp_and_hal_destroy();

/*===========================================================================
FUNCTION    sp_and_hal_get_sensor_time

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
bool sp_and_hal_get_sensor_time(uint32_t* p_time_ms);

#endif /* __GSIFF_SENSOR_PROVIDER_ANDROID_HAL_H__ */
