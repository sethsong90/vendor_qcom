/******************************************************************************
  @file:  gsiff_sensor_provider_common.c
  @brief: GNSS / Sensor Interface Framework Support

  DESCRIPTION
    This file defines the implementation of common methods shared between GSIFF
    sensor providers

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
10/24/12   vr       Adding temperature streaming.
=====================================================================*/

#include "gsiff_sensor_provider_common.h"

#define LOG_TAG "gsiff_sp_com"

#include "log_util.h"

#include "msg_q.h"

#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <pthread.h>

static pthread_mutex_t g_time_offset_mutex       = PTHREAD_MUTEX_INITIALIZER;
static uint32_t        g_time_offset_start_ms    = 0;
static uint32_t        g_time_offset_duration_ms = 0;
static int32_t         g_time_offset_ms          = 0;
static bool            g_time_offset_applied     = false;


extern int gsiff_daemon_inject_baro_data_handler(
     slimBaroDataPayloadStructT** p_baro_data);

/*===========================================================================

  FUNCTION:   sp_send_sensor_data_batch

  ===========================================================================*/
void sp_send_sensor_data_batch(void* p_msg_q, const char* sensor_str, gsiff_msg* p_gsiff_data_msg,
                               qmiLoc3AxisSensorSampleListStructT_v02* p_sensor_data_tlv,
                               qmiLocSensorTemperatureSampleListStructT_v02* p_sensor_temp_tlv,
                               double sample_interval,
                               double batching_interval)
{
    if( p_sensor_data_tlv != NULL )
    {
        /* Send data off to the message queue when the allotted time has passed */
        uint32_t time_elapsed = p_sensor_data_tlv->sensorData[p_sensor_data_tlv->sensorData_len-1].timeOffset;
        if ( p_gsiff_data_msg != NULL &&
             ( (time_elapsed + sample_interval >= batching_interval ) ||  /* Over the interval provided */
               (p_sensor_data_tlv->sensorData_len >= QMI_LOC_SENSOR_DATA_MAX_SAMPLES_V02) ) ) /* Match the max number of samples */
        {
            LOC_LOGI("%s: %s batch sent. Num Samples = %d Start Time = %d Time Range = %d",
                     __FUNCTION__,
                     sensor_str,
                     p_sensor_data_tlv->sensorData_len,
                     p_sensor_data_tlv->timeOfFirstSample,
                     p_sensor_data_tlv->sensorData[p_sensor_data_tlv->sensorData_len-1].timeOffset);

            /* Apply sensor time offset needed for testing sensor time jumps */
            p_sensor_data_tlv->timeOfFirstSample += sp_get_sensor_time_offset_ms();

            bool rv = sp_msg_q_snd(p_msg_q, p_gsiff_data_msg, sizeof(*p_gsiff_data_msg));
            if ( rv == false )
            {
                LOC_LOGE("%s: Invalid Msg Queue send!", __FUNCTION__);
            }

            /* Reset data that was sent off */
            memset(p_sensor_data_tlv, 0, sizeof(*p_sensor_data_tlv));
        }
    }
    else if( p_sensor_temp_tlv != NULL )
    {
        /* Send data off to the message queue when the allotted time has passed */
        uint32_t time_elapsed = p_sensor_temp_tlv->temperatureData[p_sensor_temp_tlv->temperatureData_len-1].timeOffset;
        if ( p_gsiff_data_msg != NULL &&
             ( (time_elapsed + sample_interval >= batching_interval ) ||  /* Over the interval provided */
               (p_sensor_temp_tlv->temperatureData_len >= QMI_LOC_SENSOR_DATA_MAX_SAMPLES_V02) ) ) /* Match the max number of samples */
        {
            LOC_LOGI("%s: %s batch sent. Num Samples = %d Start Time = %d Time Range = %d",
                     __FUNCTION__,
                     sensor_str,
                     p_sensor_temp_tlv->temperatureData_len,
                     p_sensor_temp_tlv->timeOfFirstSample,
                     p_sensor_temp_tlv->temperatureData[p_sensor_temp_tlv->temperatureData_len-1].timeOffset);

            /* Apply sensor time offset needed for testing sensor time jumps */
            p_sensor_temp_tlv->timeOfFirstSample += sp_get_sensor_time_offset_ms();

            bool rv = sp_msg_q_snd(p_msg_q, p_gsiff_data_msg, sizeof(*p_gsiff_data_msg));
            if ( rv == false )
            {
                LOC_LOGE("%s: Invalid Msg Queue send!", __FUNCTION__);
            }

            /* Reset data that was sent off */
            memset(p_sensor_temp_tlv, 0, sizeof(*p_sensor_temp_tlv));
        }
    }

    return;
}


/*===========================================================================

  FUNCTION:   sp_send_baro_data_batch

  ===========================================================================*/
void sp_send_baro_data_batch(void* p_msg_q, const char* sensor_str, gsiff_msg* p_gsiff_data_msg,
                             slimBaroDataPayloadStructT** p_baro_data,
                             double sample_interval,
                             double batching_interval,
                             int reporting_rate)
{
    if( p_baro_data != NULL )
    {
        /* Send data off to the message queue when the allotted time has passed */
        uint32_t time_elapsed = (*p_baro_data)->baroData[(*p_baro_data)->numberOfSample-1].timeOffset;

        if (( (time_elapsed + sample_interval >= batching_interval ) ||  /* Over the interval provided */
               ((*p_baro_data)->numberOfSample >= reporting_rate) ) ) /* Match the max number of samples for which data was allocated*/
        {
            LOC_LOGI("%s: %s batch sent. Num Samples = %d Start Time = %d Time Range = %d",
                     __FUNCTION__,
                     sensor_str,
                     (*p_baro_data)->numberOfSample,
                     (*p_baro_data)->timeOfFirstSample,
                     (*p_baro_data)->baroData[(*p_baro_data)->numberOfSample-1].timeOffset);

            /* Apply sensor time offset needed for testing sensor time jumps */
            (*p_baro_data)->timeOfFirstSample += sp_get_sensor_time_offset_ms();

            gsiff_send_baro_data(*p_baro_data);

            (*p_baro_data)->numberOfSample = 0;
        }
    }
}

/*===========================================================================

  FUNCTION:   sp_process_raw_sensor_data

  ===========================================================================*/
void sp_process_raw_sensor_data(const char* sensor_str, float data[3], uint32_t timestamp_ms,
                                qmiLoc3AxisSensorSampleListStructT_v02* p_sensor_data_tlv,
                                qmiLocSensorTemperatureSampleListStructT_v02* p_sensor_temp_tlv)
{
    uint32_t current_sys_time_ms = sp_read_sys_time_ms();

    /* Common handling for Accel and Gyro Data */
    if ( p_sensor_data_tlv != NULL )
    {
        LOC_LOGI("%s: %s (%.6f,%.6f,%.6f) samp num = %u, flag %u, read @ sns %lu ms, print @ sys %lu ms",
                 __FUNCTION__,
                 sensor_str,
                 data[0],
                 data[1],
                 data[2],
                 p_sensor_data_tlv->sensorData_len + 1,
                 p_sensor_data_tlv->flags,
                 timestamp_ms,
                 current_sys_time_ms);

        if ( p_sensor_data_tlv->sensorData_len == 0 )
        {
            p_sensor_data_tlv->timeOfFirstSample = timestamp_ms;
        }
        /* Check that this is a new sample by comparing new timestamp with prev timestamp */
        else
        {
            uint32_t prev_sample_time_ms = p_sensor_data_tlv->timeOfFirstSample + p_sensor_data_tlv->sensorData[p_sensor_data_tlv->sensorData_len-1].timeOffset;

            /* Is this sample monotonically in order compared to the last sample. */
            if( prev_sample_time_ms >= timestamp_ms )
            {
                LOC_LOGW("%s: Current %s sample is out of order. New time = %lu old time = %lu", __FUNCTION__, sensor_str, timestamp_ms, prev_sample_time_ms);
                return;
            }
        }

        p_sensor_data_tlv->sensorData[p_sensor_data_tlv->sensorData_len].timeOffset = sp_calc_time_diff_ms(p_sensor_data_tlv->timeOfFirstSample, timestamp_ms);
        p_sensor_data_tlv->sensorData[p_sensor_data_tlv->sensorData_len].xAxis = data[0];
        p_sensor_data_tlv->sensorData[p_sensor_data_tlv->sensorData_len].yAxis = data[1];
        p_sensor_data_tlv->sensorData[p_sensor_data_tlv->sensorData_len].zAxis = data[2];
        p_sensor_data_tlv->sensorData_len++;
    }
    else if ( p_sensor_temp_tlv != NULL )
    {
        LOC_LOGI("%s: %s %.6f samp num = %u, read @ sns %lu ms, print @ sys %lu ms",
                 __FUNCTION__,
                 sensor_str,
                 data[0],
                 p_sensor_temp_tlv->temperatureData_len + 1,
                 timestamp_ms,
                 current_sys_time_ms);

        if ( p_sensor_temp_tlv->temperatureData_len == 0 )
        {
            p_sensor_temp_tlv->timeOfFirstSample = timestamp_ms;
        }
        /* Check that this is a new sample by comparing new timestamp with prev timestamp */
        else
        {
            uint32_t prev_sample_time_ms = p_sensor_temp_tlv->timeOfFirstSample + p_sensor_temp_tlv->temperatureData[p_sensor_temp_tlv->temperatureData_len-1].timeOffset;

            /* Is this sample monotonically in order compared to the last sample. */
            if( prev_sample_time_ms >= timestamp_ms )
            {
                LOC_LOGW("%s: Current %s sample is out of order. New time = %lu old time = %lu", __FUNCTION__, sensor_str, timestamp_ms, prev_sample_time_ms);
                return;
            }
        }

        p_sensor_temp_tlv->temperatureData[p_sensor_temp_tlv->temperatureData_len].timeOffset = sp_calc_time_diff_ms(p_sensor_temp_tlv->timeOfFirstSample, timestamp_ms);
        p_sensor_temp_tlv->temperatureData[p_sensor_temp_tlv->temperatureData_len].temperature = data[0];
        p_sensor_temp_tlv->temperatureData_len++;
    }

    return;
}

/*===========================================================================

  FUNCTION:   sp_process_raw_baro_data

  ===========================================================================*/
void sp_process_raw_baro_data(const char* sensor_str, float data, uint32_t timestamp_ms,
                              slimBaroDataPayloadStructT** p_baro_data )
{
    uint32_t current_sys_time_ms = sp_read_sys_time_ms();
    if ( p_baro_data != NULL )
    {

        LOC_LOGI("%s: %s %.6f samp num = %u, read @ sns %lu ms, print @ sys %lu ms",
                 __FUNCTION__,
                 sensor_str,
                 data,
                 (*p_baro_data)->numberOfSample + 1,
                 timestamp_ms,
                 current_sys_time_ms);

        if ( (*p_baro_data)->numberOfSample == 0 )
        {
            (*p_baro_data)->timeOfFirstSample = timestamp_ms;
        }
        /* Check that this is a new sample by comparing new timestamp with prev timestamp */
        else
        {
            uint32_t prev_sample_time_ms = (*p_baro_data)->timeOfFirstSample + (*p_baro_data)->baroData[(*p_baro_data)->numberOfSample - 1].timeOffset;

            /* Is this sample monotonically in order compared to the last sample. */
            if( prev_sample_time_ms >= timestamp_ms )
            {
                LOC_LOGW("%s: Current %s sample is out of order. New time = %lu old time = %lu", __FUNCTION__, sensor_str, timestamp_ms, prev_sample_time_ms);
                return;
            }
        }

        (*p_baro_data)->baroData[(*p_baro_data)->numberOfSample].timeOffset = sp_calc_time_diff_ms((*p_baro_data)->timeOfFirstSample, timestamp_ms);
        (*p_baro_data)->baroData[(*p_baro_data)->numberOfSample].baroValue = data;

        LOC_LOGV("%s: Setting timeOffset to %d and baroValue to %f ",
                 __FUNCTION__,
                 (*p_baro_data)->baroData[(*p_baro_data)->numberOfSample].timeOffset,
                 (*p_baro_data)->baroData[(*p_baro_data)->numberOfSample].baroValue
            );

        (*p_baro_data)->numberOfSample++;
    }
}

/*===========================================================================

  FUNCTION:   sp_msg_q_snd

  ===========================================================================*/
bool sp_msg_q_snd(void* msg_q_data, void* msg_obj, uint32_t msg_sz)
{
   void (*dealloc)(void*) = free;

   void* msg_obj_cpy = malloc(msg_sz);
   if( msg_obj_cpy == NULL )
   {
      LOC_LOGE("%s: Memory allocation failure", __FUNCTION__);
      return false;
   }

   memcpy(msg_obj_cpy, msg_obj, msg_sz);

   msq_q_err_type rv = msg_q_snd(msg_q_data, msg_obj_cpy, dealloc);
   if ( rv != eMSG_Q_SUCCESS )
   {
      LOC_LOGE("%s: Invalid Msg Queue send!", __FUNCTION__);
      dealloc(msg_obj_cpy);
      return false;
   }

   return true;
}

/*===========================================================================

  FUNCTION:   sp_send_msi_mounted_status

  ===========================================================================*/
bool sp_send_msi_mounted_status(void* p_msg_q, uint8_t mounted_state)
{
   if( mounted_state == SP_MSI_DO_NOT_SEND )
   {
      return true;
   }

   if( mounted_state < SP_MSI_UNMOUNTED || mounted_state > SP_MSI_DO_NOT_SEND)
   {
      LOC_LOGE("%s: Cannot send unknown mounted_state = %u", __FUNCTION__, mounted_state);
      return false;
   }

   /* Send state mounted regardless - 0x01 Mounted and 0x00 Unmounted */

   /* Send to gsiff_ctl task through message queue */
   gsiff_msg msi_data_msg;
   msi_data_msg.msg_type = GSIFF_CRADLE_MOUNT_CONFIG_REQ;
   msi_data_msg.msg_data.cradle_mount_config_req.confidenceCradleMountState_valid = false;
   msi_data_msg.msg_data.cradle_mount_config_req.cradleMountState = (qmiLocCradleMountStateEnumT_v02)mounted_state;

   LOC_LOGI("%s: MSI update sent with stationary status = %u", __FUNCTION__, msi_data_msg.msg_data.cradle_mount_config_req.cradleMountState);

   bool rv = sp_msg_q_snd(p_msg_q, &msi_data_msg, sizeof(msi_data_msg));
   if ( rv == false )
   {
      LOC_LOGE("%s: Invalid Msg Queue send!", __FUNCTION__);
      return false;
   }

   return true;
}

/*===========================================================================

  FUNCTION:   sp_get_sensor_time_offset_ms

  ===========================================================================*/
int32_t sp_get_sensor_time_offset_ms()
{
   int32_t  time_offset = 0;
   uint32_t current_time_ms = sp_read_sys_time_ms();

   pthread_mutex_lock(&g_time_offset_mutex);

   // Per Yuhua's request we have an sensor time offset that can be configured.
   // We need to modify the time since the time sync counter hit.
   if ( g_time_offset_ms != 0 )
   {
      /* Take a timestamp when the first offset is applied. This is to compute durations. */
      if (g_time_offset_applied == false)
      {
         g_time_offset_start_ms = sp_read_sys_time_ms();
         g_time_offset_applied = true;
      }

      /* Zero means to apply offset indefinitely */
      if (g_time_offset_duration_ms == 0)
      {
         time_offset = g_time_offset_ms;
      }
      /* Need to apply based on duration */
      else
      {
         uint32_t time_delta = sp_calc_time_diff_ms(g_time_offset_start_ms, current_time_ms);

         /* Within time duration so apply offset */
         if (time_delta <= g_time_offset_duration_ms)
         {
            time_offset = g_time_offset_ms;
         }
         /* Outside of duration */
         else
         {
            /* Stop further offsets from being applied. */
            g_time_offset_start_ms = 0;
            g_time_offset_ms = 0;
         }
      }
   }

   pthread_mutex_unlock(&g_time_offset_mutex);

   if( time_offset != 0 )
   {
      LOC_LOGD("%s: Applying sns time offset = %d", __FUNCTION__, time_offset);
   }

   return time_offset;
}

/*===========================================================================

  FUNCTION:   sp_apply_sensor_time_offset_ms

  ===========================================================================*/
void sp_apply_sensor_time_offset_ms(int32_t time_offset_ms, uint32_t duration_ms)
{
   pthread_mutex_lock(&g_time_offset_mutex);

   /* Invalid time offset provided. */
   if ( time_offset_ms == 0 )
   {
      LOC_LOGE("%s: Applying INVALID sensor time offset = %d duration = %lu", __FUNCTION__, time_offset_ms, duration_ms);
   }
   else
   {
      LOC_LOGI("%s: Applying sensor time offset = %d duration = %lu", __FUNCTION__, time_offset_ms, duration_ms);
      g_time_offset_ms = time_offset_ms;
      g_time_offset_duration_ms = duration_ms;
   }

   g_time_offset_applied = false;
   g_time_offset_start_ms = 0;

   pthread_mutex_unlock(&g_time_offset_mutex);
}

/*===========================================================================

  FUNCTION:   sp_read_sys_time_ms

  ===========================================================================*/
uint32_t sp_read_sys_time_ms()
{
  struct timespec ts;
  uint32_t time_ms = 0;
  clock_gettime(CLOCK_MONOTONIC, &ts);

  time_ms += (ts.tv_sec * 1000LL);     /* Seconds to milliseconds */
  time_ms += ts.tv_nsec / 1000000LL;   /* Nanoseconds to milliseconds */

  return time_ms;
}

/*===========================================================================

  FUNCTION:   sp_read_sys_time_us

  ===========================================================================*/
uint64_t sp_read_sys_time_us()
{
  struct timespec ts;
  uint64_t time_us = 0;
  clock_gettime(CLOCK_MONOTONIC, &ts);

  time_us += (ts.tv_sec * 1000000LL);     /* Seconds to microseconds */
  time_us += ts.tv_nsec / 1000LL;         /* Nanoseconds to microseconds */

  return time_us;
}

/*===========================================================================

  FUNCTION:   sp_calc_time_diff_ms

  ===========================================================================*/
uint32_t sp_calc_time_diff_ms(uint32_t ts1_ms, uint32_t ts2_ms)
{
   if( ts1_ms <= ts2_ms )
   {
      return ts2_ms - ts1_ms;
   }
   else
   {
      return UINT_MAX - ts1_ms + ts2_ms + 1;
   }
}
