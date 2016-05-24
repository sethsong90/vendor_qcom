/******************************************************************************
  @file:  gsiff_sensor_provider_glue.c
  @brief: GNSS / Sensor Interface Framework Support

  DESCRIPTION

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
                    2. Fixed bug with starting sensors if batching/sampling
                       rate were 0 but we were turning sensors off.
10/24/12   vr       1. Adding temperature streaming.
                    2. Adding motion data streming.
                    3. Adding pedometer streaming.
======================================================================*/

#include "gsiff_sensor_provider_glue.h"
#include "gsiff_sensor_provider_and_hal.h"
#include "gsiff_sensor_provider_and_ndk.h"
#ifdef FEATURE_GSIFF_DSPS
#include "gsiff_sensor_provider_sensor1.h"
#endif
#define LOG_TAG "gsiff_sp_glue"
#include "log_util.h"

static e_sensor_provider_type g_provider_type = MIN_SENSOR_PROVIDER;

typedef bool (*sensor_init_func)(void* p_msg_q);
typedef bool (*sensor_destroy_func)();
typedef bool (*sensor_status_func)(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state);
typedef bool (*spi_status_func)(bool running);
typedef bool (*motion_data_func)(bool running);
typedef bool (*pedometer_func)(bool running, uint8_t reset_step_count, uint32_t step_count_threshold);
typedef bool (*sensor_get_time_func)(uint32_t* p_time_ms);

typedef struct s_sp_callbacks {
    sensor_init_func init;
    sensor_status_func update_accel;
    sensor_status_func update_gyro;
    sensor_status_func update_accel_temperature;
    sensor_status_func update_gyro_temperature;
    spi_status_func update_spi;
    motion_data_func update_motion_data;
    pedometer_func update_pedometer;
    sensor_destroy_func destroy;
    sensor_get_time_func get_sensor_time;
    sensor_status_func update_baro;
}s_sp_callbacks;

/**
 * Lookup table to call appropriate functions based on the sensor provider
 * Note: This has to match the numbers defined in
 * e_sensor_provider_type
 */
static s_sp_callbacks g_sp_callbacks_arr[] =
{
   /* Min Sensor Provider should not call anything. Shouldn't happen */
   {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,NULL},

#ifdef FEATURE_GSIFF_DSPS
   /* Sensor 1 API */
   {sp_sns1_init, sp_sns1_update_accel_status, sp_sns1_update_gyro_status, sp_sns1_update_accel_temperature_status, sp_sns1_update_gyro_temperature_status, sp_sns1_update_spi_status, sp_sns1_update_motion_data, sp_sns1_update_pedometer, sp_sns1_destroy, sp_sns1_get_sensor_time, sp_sns1_update_baro_status},
#else
   {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
#endif

#ifdef FEATURE_GSIFF_ANDROID_NDK
   /* Native Android NDK */
   {sp_and_ndk_init, sp_and_ndk_update_accel_status, sp_and_ndk_update_gyro_status, NULL, NULL, NULL, NULL, NULL, sp_and_ndk_destroy, sp_and_ndk_get_sensor_time, sp_and_ndk_update_baro_status},
#else
   {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
#endif

#ifdef FEATURE_GSIFF_ANDROID_HAL
   /* Native Android HAL */
   {sp_and_hal_init, sp_and_hal_update_accel_status, sp_and_hal_update_gyro_status, NULL, NULL, NULL, NULL, NULL, sp_and_hal_destroy, sp_and_hal_get_sensor_time,NULL},
#else
   {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
#endif
};

/*===========================================================================

  FUNCTION:   sp_init

  ===========================================================================*/
bool sp_init(void* p_msg_q, e_sensor_provider_type provider_type)
{
   /* Check that we are not already using a sensor provider */
   if( g_provider_type > MIN_SENSOR_PROVIDER && g_provider_type < MAX_SENSOR_PROVIDER )
   {
      LOC_LOGE("%s: Initializing sensor provider after initialized", __FUNCTION__);
      return false;
   }

   if( provider_type <= MIN_SENSOR_PROVIDER || provider_type >= MAX_SENSOR_PROVIDER )
   {
      LOC_LOGE("%s: Unknown sensor provider type = %d", __FUNCTION__, provider_type);
      return false;
   }
   else
   {
      bool rv = false;
      sensor_init_func init = g_sp_callbacks_arr[provider_type].init;
      if( init != NULL )
      {
         rv = init(p_msg_q);

         if( true == rv )
         {
            g_provider_type = provider_type;
         }
      }
      else
      {
         LOC_LOGE("%s: Initializing sensor provider %d not supported!", __FUNCTION__, provider_type);
      }

      return rv;
   }
}

/*===========================================================================

  FUNCTION:   sp_update_accel_temperature_status

  ===========================================================================*/
bool sp_update_accel_temperature_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state)
{
   if( running && ( sampling_rate == 0 || batching_rate == 0 ) )
   {
      LOC_LOGE("%s: Sampling/batching rate must be non-zero", __FUNCTION__);
      return false;
   }

   if( g_provider_type <= MIN_SENSOR_PROVIDER || g_provider_type >= MAX_SENSOR_PROVIDER )
   {
      LOC_LOGE("%s: Sensor provider not initialized. Sensor provider = %d", __FUNCTION__, g_provider_type);
      return false;
   }
   else
   {
      sensor_status_func update_accel_temperature = g_sp_callbacks_arr[g_provider_type].update_accel_temperature;
      if( update_accel_temperature != NULL )
      {
         return update_accel_temperature(running, sampling_rate, batching_rate, mounted_state);
      }
      else
      {
         LOC_LOGW("%s: Updating accel status not supported for sensor provider %d!", __FUNCTION__, g_provider_type);
      }
   }

   return false;
}

/*===========================================================================

  FUNCTION:   sp_update_gyro_temperature_status

  ===========================================================================*/
bool sp_update_gyro_temperature_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state)
{
   if( running && ( sampling_rate == 0 || batching_rate == 0 ) )
   {
      LOC_LOGE("%s: Sampling/batching rate must be non-zero", __FUNCTION__);
      return false;
   }

   if( g_provider_type <= MIN_SENSOR_PROVIDER || g_provider_type >= MAX_SENSOR_PROVIDER )
   {
      LOC_LOGE("%s: Sensor provider not initialized. Sensor provider = %d", __FUNCTION__, g_provider_type);
      return false;
   }
   else
   {
      sensor_status_func update_gyro_temperature = g_sp_callbacks_arr[g_provider_type].update_gyro_temperature;
      if( update_gyro_temperature != NULL )
      {
         return update_gyro_temperature(running, sampling_rate, batching_rate, mounted_state);
      }
      else
      {
         LOC_LOGW("%s: Updating gyro status not supported for sensor provider %d!", __FUNCTION__, g_provider_type);
      }
   }

   return false;
}
/*===========================================================================

  FUNCTION:   sp_update_accel_status

  ===========================================================================*/
bool sp_update_accel_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state)
{
   if( running && ( sampling_rate == 0 || batching_rate == 0 ) )
   {
      LOC_LOGE("%s: Sampling/batching rate must be non-zero", __FUNCTION__);
      return false;
   }

   if( g_provider_type <= MIN_SENSOR_PROVIDER || g_provider_type >= MAX_SENSOR_PROVIDER )
   {
      LOC_LOGE("%s: Sensor provider not initialized. Sensor provider = %d", __FUNCTION__, g_provider_type);
      return false;
   }
   else
   {
      sensor_status_func update_accel = g_sp_callbacks_arr[g_provider_type].update_accel;
      if( update_accel != NULL )
      {
         return update_accel(running, sampling_rate, batching_rate, mounted_state);
      }
      else
      {
         LOC_LOGW("%s: Updating accel status not supported for sensor provider %d!", __FUNCTION__, g_provider_type);
      }
   }

   return false;
}

/*===========================================================================

  FUNCTION:   sp_update_gyro_status

  ===========================================================================*/
bool sp_update_gyro_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state)
{
   if( running && ( sampling_rate == 0 || batching_rate == 0 ) )
   {
      LOC_LOGE("%s: Sampling/batching rate must be non-zero", __FUNCTION__);
      return false;
   }

   if( g_provider_type <= MIN_SENSOR_PROVIDER || g_provider_type >= MAX_SENSOR_PROVIDER )
   {
      LOC_LOGE("%s: Sensor provider not initialized. Sensor provider = %d", __FUNCTION__, g_provider_type);
      return false;
   }
   else
   {
      sensor_status_func update_gyro = g_sp_callbacks_arr[g_provider_type].update_gyro;
      if( update_gyro != NULL )
      {
         return update_gyro(running, sampling_rate, batching_rate, mounted_state);
      }
      else
      {
         LOC_LOGW("%s: Updating gyro status not supported for sensor provider %d!", __FUNCTION__, g_provider_type);
      }
   }

   return false;
}

/*===========================================================================

  FUNCTION:   sp_update_motion_data

  ===========================================================================*/
bool sp_update_motion_data(bool running)
{
   if( g_provider_type <= MIN_SENSOR_PROVIDER || g_provider_type >= MAX_SENSOR_PROVIDER )
   {
      LOC_LOGE("%s: Sensor provider not initialized. Sensor provider = %d", __FUNCTION__, g_provider_type);
      return false;
   }
   else
   {
      motion_data_func update_motion_data = g_sp_callbacks_arr[g_provider_type].update_motion_data;
      if( update_motion_data != NULL )
      {
         return update_motion_data(running);
      }
      else
      {
         LOC_LOGW("%s: Updating motion data not supported for sensor provider %d!", __FUNCTION__, g_provider_type);
      }
   }

   return false;
}

/*===========================================================================

  FUNCTION:   sp_update_pedometer

  ===========================================================================*/
bool sp_update_pedometer(bool running, uint8_t reset_step_count, uint32_t step_count_threshold)
{
   if( g_provider_type <= MIN_SENSOR_PROVIDER || g_provider_type >= MAX_SENSOR_PROVIDER )
   {
      LOC_LOGE("%s: Sensor provider not initialized. Sensor provider = %d", __FUNCTION__, g_provider_type);
      return false;
   }
   else
   {
      pedometer_func update_pedometer = g_sp_callbacks_arr[g_provider_type].update_pedometer;
      if( update_pedometer != NULL )
      {
         return update_pedometer(running, reset_step_count, step_count_threshold);
      }
      else
      {
         LOC_LOGW("%s: Updating pedometer not supported for sensor provider %d!", __FUNCTION__, g_provider_type);
      }
   }

   return false;
}

/*===========================================================================

  FUNCTION:   sp_update_spi_status

  ===========================================================================*/
bool sp_update_spi_status(bool running)
{
   if( g_provider_type <= MIN_SENSOR_PROVIDER || g_provider_type >= MAX_SENSOR_PROVIDER )
   {
      LOC_LOGE("%s: Sensor provider not initialized. Sensor provider = %d", __FUNCTION__, g_provider_type);
      return false;
   }
   else
   {
      spi_status_func update_spi = g_sp_callbacks_arr[g_provider_type].update_spi;
      if( update_spi != NULL )
      {
         return update_spi(running);
      }
      else
      {
         LOC_LOGW("%s: Updating spi status not supported for sensor provider %d!", __FUNCTION__, g_provider_type);
      }
   }

   return false;
}

/*===========================================================================

  FUNCTION:   sp_destroy

  ===========================================================================*/
bool sp_destroy()
{
   if( g_provider_type <= MIN_SENSOR_PROVIDER || g_provider_type >= MAX_SENSOR_PROVIDER )
   {
      LOC_LOGE("%s: Sensor provider not initialized. Sensor provider = %d", __FUNCTION__, g_provider_type);
      return false;
   }
   else
   {
      bool rv = false;
      sensor_destroy_func destroy = g_sp_callbacks_arr[g_provider_type].destroy;
      if( destroy != NULL )
      {
         rv = destroy();

         if( true == rv )
         {
            g_provider_type = MIN_SENSOR_PROVIDER;
         }
      }
      else
      {
         LOC_LOGE("%s: Destroying sensor provider %d not supported!", __FUNCTION__, g_provider_type);
      }

      return rv;
   }
}

/*===========================================================================

  FUNCTION:   sp_get_sensor_time

  ===========================================================================*/
bool sp_get_sensor_time(uint32_t* p_time_ms)
{
   if( g_provider_type <= MIN_SENSOR_PROVIDER || g_provider_type >= MAX_SENSOR_PROVIDER )
   {
      LOC_LOGE("%s: Sensor provider not initialized. Sensor provider = %d", __FUNCTION__, g_provider_type);
      return false;
   }
   else
   {
      bool rv = false;
      sensor_get_time_func get_time = g_sp_callbacks_arr[g_provider_type].get_sensor_time;
      if( get_time != NULL )
      {
         rv = get_time(p_time_ms);
      }
      else
      {
         LOC_LOGE("%s: Getting sensor time not supported for sensor provider %d!", __FUNCTION__, g_provider_type);
      }

      return rv;
   }
}

/*===========================================================================

  FUNCTION:   sp_update_baro_status

  ===========================================================================*/
bool sp_update_baro_status(bool running, uint32_t sampling_rate, uint32_t batching_rate, uint8_t mounted_state)
{
   if( running && ( sampling_rate == 0 || batching_rate == 0 ) )
   {
      LOC_LOGE("%s: Sampling/batching rate must be non-zero", __FUNCTION__);
      return false;
   }

   if( g_provider_type <= MIN_SENSOR_PROVIDER || g_provider_type >= MAX_SENSOR_PROVIDER )
   {
      LOC_LOGE("%s: Sensor provider not initialized. Sensor provider = %d", __FUNCTION__, g_provider_type);
      return false;
   }
   else
   {
      sensor_status_func update_baro = g_sp_callbacks_arr[g_provider_type].update_baro;
      if( update_baro != NULL )
      {
         return update_baro(running, sampling_rate, batching_rate, mounted_state);
      }
      else
      {
         LOC_LOGW("%s: Updating baro status not supported for sensor provider %d!", __FUNCTION__, g_provider_type);
      }
   }

   return false;
}
