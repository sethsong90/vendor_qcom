/******************************************************************************
  @file:  gsiff_daemon_manager.c
  @brief: GNSS / Sensor Interface Framework Daemon

  DESCRIPTION
    This file creates a daemon that will register with LocApi and an Android
    Sensor Provider (DSPS or Android API). It will provide sensor data and
    sensor timing information to the Modem through LocApi.

  INITIALIZATION AND SEQUENCING REQUIREMENTS

  -----------------------------------------------------------------------------
  Copyright (c) 2012-2013 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
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
08/01/11   jb       1. Adding QMI bootup messages.
                    2. Read configuration from gps.conf file for bootup msgs.
                    3. Change to new sensor streaming structure and remove
                       default sampling/batching rate as it is specified.
10/24/12   vr       1. Adding temperature streaming.
                    2. Adding motion data streming.
                    3. Adding pedometer streaming.
======================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#define LOG_TAG "gsiff_dmn"

#include <stdbool.h>
#include <pthread.h>
#include "log_util.h"

#include "location_service_v02.h"

#include "gpsone_thread_helper.h"
#include "gpsone_glue_msg.h"
#include "msg_q.h"
#include "loc_cfg.h"

#include "gsiff_loc_api_glue.h"
#ifdef FEATURE_GSIFF_DSPS
#include "gsiff_sensor_provider_sensor1.h"
#endif
#include "gsiff_sensor_provider_glue.h"
#include "gsiff_sensor_provider_common.h"
#include "gsiff_msg.h"
#include "gsiff_slim_client_glue.h"

#define DEFAULT_MOUNTED_STATE   SP_MSI_DO_NOT_SEND

#ifdef FEATURE_GSIFF_DSPS
#define DEFAULT_SENSOR_PROVIDER SENSOR1_SENSOR_PROVIDER
#elif FEATURE_GSIFF_ANDROID_HAL
#define DEFAULT_SENSOR_PROVIDER ANDROID_HAL_SENSOR_PROVIDER
#else
#define DEFAULT_SENSOR_PROVIDER ANDROID_NDK_SENSOR_PROVIDER
#endif

#define DEFAULT_EVT_REG_MASK    (QMI_LOC_EVENT_MASK_TIME_SYNC_REQ_V02 | \
                                 QMI_LOC_EVENT_MASK_SENSOR_STREAMING_READY_STATUS_V02 | \
                                 QMI_LOC_EVENT_MASK_SET_SPI_STREAMING_REPORT_V02 | \
                                 QMI_LOC_EVENT_MASK_MOTION_DATA_CONTROL_V02 | \
                                 QMI_LOC_EVENT_MASK_PEDOMETER_CONTROL_V02)

#define SAP_CONF_FILE "/etc/sap.conf"

#ifndef GPS_CONF_FILE
#define GPS_CONF_FILE "/etc/gps.conf"
#endif

typedef struct gsiff_control_t {
   int32_t               msg_q_id;              /* Ctl Message Queue identifier (pipe-based) */
   void*                 data_msg_q;            /* Data Message Queue (memory-based) */
   struct gpsone_thelper ctl_task_helper;       /* Control Task thread state */
   struct gpsone_thelper data_task_helper;      /* Data Task thread state */
   uint8_t               mounted_state;         /* Mounted State 1 - MOUNTED, 0 - UNMOUNTED, 2 Do Not Send*/
} gsiff_control_t;

static gsiff_control_t* g_gsiff_control = NULL;
static uint32_t g_sensor_provider = (uint32_t)DEFAULT_SENSOR_PROVIDER;
static uint32_t g_sensor_usage = (uint32_t)0;

static loc_param_s_type conf_parameter_table[] =
{
  {"SENSOR_PROVIDER",                   &g_sensor_provider, NULL,                  'n'},
  {"SENSOR_USAGE",                   &g_sensor_usage, NULL,                  'n'},
};
pthread_mutex_t lock;

pthread_t  server_thread;
extern slimBaroDataPayloadStructT* g_baro_data;

access_control_t* g_access_control = NULL;

/*===========================================================================
FUNCTION    gsiff_daemon_sensor_streaming_handler

DESCRIPTION
   Handler for Sensor streaming status event indications received from LocApi.

   Note: Function called from the ctl_task thread context as starting/stopping
         sensor provider may take a while complete.

DEPENDENCIES
   N/A

RETURN VALUE
   < 0  : Failure
   0 >= : Success

SIDE EFFECTS
   Updates the sensor status for accel/gyro and/or accel/gyro temperature.

===========================================================================*/
static int gsiff_daemon_sensor_streaming_handler(qmiLocEventSensorStreamingReadyStatusIndMsgT_v02* p_streaming_status_ind)
{
   LOC_LOGI("%s: Recv Sensor Streaming Accel(%u,%u,%u,%u) "
            " Gyro(%u,%u,%u,%u) (valid,ready,#samp,batching)",
                 __FUNCTION__,
                 p_streaming_status_ind->accelReady_valid,
                 p_streaming_status_ind->accelReady.injectEnable,
                 p_streaming_status_ind->accelReady.dataFrequency.samplesPerBatch,
                 p_streaming_status_ind->accelReady.dataFrequency.batchesPerSecond,
                 p_streaming_status_ind->gyroReady_valid,
                 p_streaming_status_ind->gyroReady.injectEnable,
                 p_streaming_status_ind->gyroReady.dataFrequency.samplesPerBatch,
                 p_streaming_status_ind->gyroReady.dataFrequency.batchesPerSecond);
   LOC_LOGI("%s: Recv Sensor Streaming AccelTemperature(%u,%u,%u,%u) "
            "GyroTemperature(%u,%u,%u,%u)(valid,ready,#samp,batching)",
                 __FUNCTION__,
                 p_streaming_status_ind->accelTemperatureReady_valid,
                 p_streaming_status_ind->accelTemperatureReady.injectEnable,
                 p_streaming_status_ind->accelTemperatureReady.dataFrequency.samplesPerBatch,
                 p_streaming_status_ind->accelTemperatureReady.dataFrequency.batchesPerSecond,
                 p_streaming_status_ind->gyroTemperatureReady_valid,
                 p_streaming_status_ind->gyroTemperatureReady.injectEnable,
                 p_streaming_status_ind->gyroTemperatureReady.dataFrequency.samplesPerBatch,
                 p_streaming_status_ind->gyroTemperatureReady.dataFrequency.batchesPerSecond);

   /* Read sampling / batching rate from the struture if they are non-zero */
   uint32_t accel_batching_rate = p_streaming_status_ind->accelReady.dataFrequency.batchesPerSecond;
   uint32_t gyro_batching_rate = p_streaming_status_ind->gyroReady.dataFrequency.batchesPerSecond;
   uint32_t accel_sampling_rate = p_streaming_status_ind->accelReady.dataFrequency.samplesPerBatch * accel_batching_rate;
   uint32_t gyro_sampling_rate = p_streaming_status_ind->gyroReady.dataFrequency.samplesPerBatch * gyro_batching_rate;
   uint32_t accel_temperature_batching_rate = p_streaming_status_ind->accelTemperatureReady.dataFrequency.batchesPerSecond;
   uint32_t gyro_temperature_batching_rate = p_streaming_status_ind->gyroTemperatureReady.dataFrequency.batchesPerSecond;
   uint32_t accel_temperature_sampling_rate = p_streaming_status_ind->accelTemperatureReady.dataFrequency.samplesPerBatch * accel_temperature_batching_rate;
   uint32_t gyro_temperature_sampling_rate = p_streaming_status_ind->gyroTemperatureReady.dataFrequency.samplesPerBatch * gyro_temperature_batching_rate;

   /* Update Accel running status if the TLV is valid */
   if(p_streaming_status_ind->accelReady_valid)
   {
       sp_update_accel_status(p_streaming_status_ind->accelReady.injectEnable, accel_sampling_rate, accel_batching_rate, g_gsiff_control->mounted_state);
   }
   /* Update Gyro running status if the TLV is valid */
   if(p_streaming_status_ind->gyroReady_valid)
   {
      sp_update_gyro_status(p_streaming_status_ind->gyroReady.injectEnable, gyro_sampling_rate, gyro_batching_rate, g_gsiff_control->mounted_state);
   }

   /* Update Accel Temperature running status if the TLV is valid */
   if(p_streaming_status_ind->accelTemperatureReady_valid)
   {
      sp_update_accel_temperature_status(p_streaming_status_ind->accelTemperatureReady.injectEnable, accel_temperature_sampling_rate, accel_temperature_batching_rate, g_gsiff_control->mounted_state);
   }
   /* Update Gyro Temperature running status if the TLV is valid */
   if(p_streaming_status_ind->gyroTemperatureReady_valid)
   {
      sp_update_gyro_temperature_status(p_streaming_status_ind->gyroTemperatureReady.injectEnable, gyro_temperature_sampling_rate, gyro_temperature_batching_rate, g_gsiff_control->mounted_state);
   }

   return 0;
}

/*===========================================================================
FUNCTION    gsiff_daemon_time_sync_handler

DESCRIPTION
   Handler for Time Sync event indications received from LocApi.

   Note: Function called from the ctl_task thread context as reading the sensor
         provider clock may take a while to complete.

DEPENDENCIES
   N/A

RETURN VALUE
   < 0  : Failure
   0 >= : Success

SIDE EFFECTS
   N/A

===========================================================================*/
static int gsiff_daemon_time_sync_handler(qmiLocEventTimeSyncReqIndMsgT_v02* p_time_sync_ind)
{
   locClientReqUnionType req_union;
   qmiLocInjectTimeSyncDataReqMsgT_v02 time_sync_data_req;

   req_union.pInjectTimeSyncReq = &time_sync_data_req;
   time_sync_data_req.refCounter = p_time_sync_ind->refCounter;

   /* Read sensor provider clock */
   uint32_t timestamp_ms;
   bool rv = sp_get_sensor_time(&timestamp_ms);
   if( rv == false )
   {
      LOC_LOGE("%s: Unable to retrieve sensor time for time sync!", __FUNCTION__);
      return 0;
   }

   /* Apply sensor time jumps to time sync. Modify all sensor times to be consistent */
   time_sync_data_req.sensorProcRxTime = timestamp_ms + sp_get_sensor_time_offset_ms();
   time_sync_data_req.sensorProcTxTime = time_sync_data_req.sensorProcRxTime;

   LOC_LOGI("%s: Sending TimeSyncData to LocApi. Timestamp Read = %lu Timestamp sent = %lu ref_counter = %lu",
                 __FUNCTION__,
                 timestamp_ms,
                 time_sync_data_req.sensorProcRxTime,
                 time_sync_data_req.refCounter);

   /* Send Sensor time data to Modem */
   gsiff_loc_api_send(QMI_LOC_INJECT_TIME_SYNC_DATA_REQ_V02,
                      &req_union);

   return 0;
}

/*===========================================================================
FUNCTION    gsiff_daemon_time_sync_handler_for_slim_client

DESCRIPTION
   Handler for Time Sync request from pip engine.

   Note: Function called from the ctl_task thread context as reading the sensor
         provider clock may take a while to complete.

DEPENDENCIES
   N/A

RETURN VALUE
   < 0  : Failure
   0 >= : Success

SIDE EFFECTS
   N/A

===========================================================================*/
static int gsiff_daemon_time_sync_handler_for_slim_client(slimTimeSyncReqStructT* p_time_sync_req)
{
   slimTimeSyncDataRespStructT time_sync_resp;

   time_sync_resp.msgHdr.msgSize = sizeof(slimTimeSyncDataRespStructT);
   time_sync_resp.msgHdr.msgId = SLIM_MSG_TYPE_INJECT_TIME_SYNC_DATA;
   time_sync_resp.msgHdr.clientId = p_time_sync_req->msgHdr.clientId;
   time_sync_resp.msgHdr.transactionId = p_time_sync_req->msgHdr.transactionId;

   time_sync_resp.msgPayload.clientTxTime = p_time_sync_req->msgPayload.clientTxTime;

   /* Read sensor provider clock */
   uint32_t timestamp_ms;
   bool rv = sp_get_sensor_time(&timestamp_ms);
   if( rv == false )
   {
      LOC_LOGE("%s: Unable to retrieve sensor time for time sync!", __FUNCTION__);
      return 0;
   }

   /* Apply sensor time jumps to time sync. Modify all sensor times to be consistent */
   time_sync_resp.msgPayload.sensorProcRxTime = timestamp_ms + sp_get_sensor_time_offset_ms();
   time_sync_resp.msgPayload.sensorProcTxTime = time_sync_resp.msgPayload.sensorProcRxTime;

   LOC_LOGI("%s: Sending TimeSyncData to PIP Engine. Timestamp Read = %lu Timestamp sent = %lu  clientTxTime = %lu",
                 __FUNCTION__,
                 timestamp_ms,
                 time_sync_resp.msgPayload.sensorProcRxTime,
                 time_sync_resp.msgPayload.clientTxTime);

   /* Send Sensor time data to PIP Engine TBD: Route through data task*/
    LOC_LOGV("%s: Sending the time sync data to pip engine ", __FUNCTION__);
    gsiff_send_time_sync_data(time_sync_resp);

   return 0;
}


/*===========================================================================
FUNCTION    gsiff_daemon_inject_sensor_data_handler

DESCRIPTION
   Handler for Inject Sensor Data Requests received from sensor provider.

   Note: Function called from the data_task thread context as sending sensor
         data through LocApi may take a while.

DEPENDENCIES
   N/A

RETURN VALUE
   < 0  : Failure
   0 >= : Success

SIDE EFFECTS
   N/A

===========================================================================*/
static int gsiff_daemon_inject_sensor_data_handler(
             qmiLoc3AxisSensorSampleListStructT_v02* p_inject_sensor_samples,
             qmiLocSensorTemperatureSampleListStructT_v02* p_inject_sensor_temp_samples,
             e_gsiff_msg_type msg_type)
{
   static uint32_t opaque_id = 0;
   locClientReqUnionType req_union;
   qmiLocInjectSensorDataReqMsgT_v02 sensor_data_msg;
   memset(&sensor_data_msg, 0, sizeof(sensor_data_msg));

   if( msg_type == GSIFF_INJECT_SENSOR_DATA_ACCEL_REQ )
   {
      sensor_data_msg.threeAxisAccelData_valid = true;
      sensor_data_msg.threeAxisAccelData = *p_inject_sensor_samples;
   }
   else if( msg_type == GSIFF_INJECT_SENSOR_DATA_GYRO_REQ )
   {
      sensor_data_msg.threeAxisGyroData_valid = true;
      sensor_data_msg.threeAxisGyroData = *p_inject_sensor_samples;
   }
   else if( msg_type == GSIFF_INJECT_SENSOR_TEMPERATURE_ACCEL_REQ )
   {
      sensor_data_msg.accelTemperatureData_valid = true;
      sensor_data_msg.accelTemperatureData = *p_inject_sensor_temp_samples;
   }
   else if( msg_type == GSIFF_INJECT_SENSOR_TEMPERATURE_GYRO_REQ )
   {
      sensor_data_msg.gyroTemperatureData_valid = true;
      sensor_data_msg.gyroTemperatureData = *p_inject_sensor_temp_samples;
   }
   else
   {
      return 0;
   }

   sensor_data_msg.opaqueIdentifier_valid = true;
   sensor_data_msg.opaqueIdentifier = opaque_id;
   opaque_id++;

   req_union.pInjectSensorDataReq = &sensor_data_msg;

   LOC_LOGI("%s: Sending Sensor Data to LocApi. opaque_id = %u", __FUNCTION__, opaque_id);

   /* Send Sensor data to Modem */
   gsiff_loc_api_send(QMI_LOC_INJECT_SENSOR_DATA_REQ_V02,
                      &req_union);

   return 0;
}

/*===========================================================================
FUNCTION    gsiff_daemon_baro_request_handler

DESCRIPTION
   Handler for Sensor baro data requests received from PIP Engine.

   Note: Function called from the ctl_task thread context as starting/stopping
         sensor provider may take a while complete.

DEPENDENCIES
   N/A

RETURN VALUE
   < 0  : Failure
   0 >= : Success

SIDE EFFECTS
   Updates the sensor status for baro.

===========================================================================*/
static int gsiff_daemon_baro_request_handler(slimBaroDataReqPayloadStructT* p_baro_data_req)
{
    LOC_LOGI("%s: Recv baro data request "
             " Baro(%u,%u,%u) (ready,#samp,batching)",
             __FUNCTION__,
             p_baro_data_req->enableBaroData,
             p_baro_data_req->samplesPerBatch,
             p_baro_data_req->batchesPerSecond);

    /* Read sampling / batching rate from the struture if they are non-zero */
    uint32_t baro_batching_rate = p_baro_data_req->batchesPerSecond;
    uint32_t baro_sampling_rate = p_baro_data_req->samplesPerBatch * baro_batching_rate;

    /* Update baro running status if the pip engine is ready to receive */
    sp_update_baro_status(p_baro_data_req->enableBaroData, baro_sampling_rate, baro_batching_rate, g_gsiff_control->mounted_state);

    return 0;
}

/*===========================================================================
FUNCTION    gsiff_daemon_motion_data_handler

DESCRIPTION
   Handler for Motion Data Requests received from sensor provider.

   Note: Function called from the data_task thread context as sending motion
   data through LocApi may take a while.

DEPENDENCIES
   N/A

RETURN VALUE
   < 0  : Failure
   0 >= : Success

SIDE EFFECTS
   N/A

===========================================================================*/
static int gsiff_daemon_motion_data_handler(qmiLocInjectMotionDataReqMsgT_v02* p_motion_data_req)
{
  locClientReqUnionType req_union;
  req_union.pInjectMotionDataReq = p_motion_data_req;

  LOC_LOGI("%s: Sending Motion Data to LocApi.", __FUNCTION__);
  gsiff_loc_api_send(QMI_LOC_INJECT_MOTION_DATA_REQ_V02,&req_union);

  return 0;
}

/*===========================================================================
FUNCTION    gsiff_daemon_pedometer_handler

DESCRIPTION
   Handler for Pedometer requests received from sensor provider.

   Note: Function called from the data_task thread context as sending pedometer
   data through LocApi may take a while.

DEPENDENCIES
   N/A

RETURN VALUE
   < 0  : Failure
   0 >= : Success

SIDE EFFECTS
   N/A

===========================================================================*/
static int gsiff_daemon_pedometer_handler(qmiLocPedometerReportReqMsgT_v02* p_pedometer_req)
{
  locClientReqUnionType req_union;
  req_union.pPedometerReportReq = p_pedometer_req;

  LOC_LOGI("%s: Sending Pedometer data to LocApi.", __FUNCTION__);
  gsiff_loc_api_send(QMI_LOC_PEDOMETER_REPORT_REQ_V02,&req_union);

  return 0;
}

/*===========================================================================
FUNCTION    gsiff_daemon_spi_status_handler

DESCRIPTION
   Handler for SPI Status Requests received from sensor provider.

   Note: Function called from the data_task thread context as sending spi status
   through LocApi may take a while.

DEPENDENCIES
   N/A

RETURN VALUE
   < 0  : Failure
   0 >= : Success

SIDE EFFECTS
   N/A

===========================================================================*/
static int gsiff_daemon_spi_status_handler(qmiLocSetSpiStatusReqMsgT_v02* p_set_spi_req)
{
   locClientReqUnionType req_union;
   req_union.pSetSpiStatusReq = p_set_spi_req;

   LOC_LOGI("%s: Sending XSPI to LocApi.", __FUNCTION__);

   gsiff_loc_api_send(QMI_LOC_SET_SPI_STATUS_REQ_V02,
                      &req_union);
   return 0;
}

/*===========================================================================
FUNCTION    gsiff_daemon_cradle_mount_config_handler

DESCRIPTION
   Handler for Cradle Mount Config Requests received from sensor provider.

   Note: Function called from the data_task thread context as sending cradle
   mount config through LocApi may take a while.

DEPENDENCIES
   N/A

RETURN VALUE
   < 0  : Failure
   0 >= : Success

SIDE EFFECTS
   N/A

===========================================================================*/
static int gsiff_daemon_cradle_mount_config_handler(qmiLocSetCradleMountConfigReqMsgT_v02* p_set_cradle_mount_config_req)
{
   locClientReqUnionType req_union;
   req_union.pSetCradleMountConfigReq = p_set_cradle_mount_config_req;

   LOC_LOGI("%s: Sending XMSI to LocApi mounted_state = %d.",
            __FUNCTION__, p_set_cradle_mount_config_req->cradleMountState);

   gsiff_loc_api_send(QMI_LOC_SET_CRADLE_MOUNT_CONFIG_REQ_V02,
                      &req_union);

   return 0;
}

/*===========================================================================
FUNCTION    gsiff_daemon_sensor_time_offset_config_handler

DESCRIPTION
   Handler for Sensor Time Offset Config requests received from test applications.

DEPENDENCIES
   N/A

RETURN VALUE
   < 0  : Failure
   0 >= : Success

SIDE EFFECTS
   N/A

===========================================================================*/
static int gsiff_daemon_sensor_time_offset_config_handler(gsiff_sensor_time_offset_config* p_sensor_time_offset_config)
{
   sp_apply_sensor_time_offset_ms(p_sensor_time_offset_config->time_offset_ms,
                                  p_sensor_time_offset_config->time_offset_duration_ms);

   return 0;
}

/*===========================================================================
FUNCTION    gsiff_daemon_motion_data_streaming_handler

DESCRIPTION
   Handler for Inject Motion Data Indication requests received from Loc API 2.0.

DEPENDENCIES
   N/A

RETURN VALUE
   < 0  : Failure
   0 >= : Success

SIDE EFFECTS
   N/A

===========================================================================*/
static int gsiff_daemon_motion_data_streaming_handler(qmiLocEventMotionDataControlIndMsgT_v02* p_motion_data_ind)
{
   LOC_LOGI("%s: Recv Inject Motion Data Ind requestMotionData = %u",
                 __FUNCTION__,
                 p_motion_data_ind->requestMotionData);

   sp_update_motion_data(p_motion_data_ind->requestMotionData);

   return 0;
}

/*===========================================================================
FUNCTION    gsiff_daemon_pedometer_control_handler

DESCRIPTION
   Handler for Pedometer Indication requests received from Loc API 2.0.

DEPENDENCIES
   N/A

RETURN VALUE
   < 0  : Failure
   0 >= : Success

SIDE EFFECTS
   N/A

===========================================================================*/
static int gsiff_daemon_pedometer_control_handler(qmiLocEventPedometerControlIndMsgT_v02* p_pedometer_ind)
{
   bool running =  p_pedometer_ind->requestPedometerData;
   uint8_t reset_step_count = 0;
   uint32_t step_count_threshold = 0;

   LOC_LOGI("%s: Recv Pedometer Ind requestPedometer = %u, resetStepCount_valid = %u,"
            " resetStepCount = %u, stepCountThreshold_valid = %u, stepCountThreshold = %u",
                 __FUNCTION__,
                 p_pedometer_ind->requestPedometerData,
                 p_pedometer_ind->resetStepCount_valid,
                 p_pedometer_ind->resetStepCount,
                 p_pedometer_ind->stepCountThreshold_valid,
                 p_pedometer_ind->stepCountThreshold);

   if ( p_pedometer_ind->resetStepCount_valid)
   {
     reset_step_count = p_pedometer_ind->resetStepCount;
   }
   if ( p_pedometer_ind->stepCountThreshold_valid)
   {
     step_count_threshold = p_pedometer_ind->stepCountThreshold;
   }

   sp_update_pedometer(running,reset_step_count,step_count_threshold);

   return 0;
}

/*===========================================================================
FUNCTION    gsiff_daemon_spi_streaming_handler

DESCRIPTION
   Handler for SPI Streaming Indication requests received from Loc API 2.0.

DEPENDENCIES
   N/A

RETURN VALUE
   < 0  : Failure
   0 >= : Success

SIDE EFFECTS
   N/A

===========================================================================*/
static int gsiff_daemon_spi_streaming_handler(
           qmiLocEventSetSpiStreamingReportIndMsgT_v02* p_spi_streaming_ind)
{
   LOC_LOGI("%s: Recv SPI Streaming Ind enabled = %u",
                 __FUNCTION__,
                 p_spi_streaming_ind->enable);

   sp_update_spi_status(p_spi_streaming_ind->enable);

   return 0;
}

/*===========================================================================
FUNCTION    gsiff_data_task

DESCRIPTION
   Data Task Thread used by daemon process. Reads from a data message queue
   (memory-based)and calls appropriate message handlers based on received
   message. Thread runs forever unless killed by user. Used for low latency
   operations such as injecting sensor data as pipe implementation is too
   slow.

DEPENDENCIES
   N/A

RETURN VALUE
   < 0  : Failure
   0 >= : Success

SIDE EFFECTS
   N/A

===========================================================================*/
static int gsiff_data_task(void* context)
{
   void* p_data_msg_q = (void*)context;
   int rv = 0;

   gsiff_msg* msg_received = NULL;

   msq_q_err_type msg_q_rv = msg_q_rcv(p_data_msg_q, (void**)&msg_received);
   if ( msg_q_rv == eMSG_Q_SUCCESS && msg_received != NULL )
   {
      LOC_LOGI("%s: Handling message type = %u", __FUNCTION__, msg_received->msg_type);

      switch ( msg_received->msg_type )
      {
      case GSIFF_INJECT_SENSOR_DATA_ACCEL_REQ:
      case GSIFF_INJECT_SENSOR_DATA_GYRO_REQ:
         rv = gsiff_daemon_inject_sensor_data_handler(&msg_received->msg_data.sensor_data_samples,
              NULL,msg_received->msg_type);
         break;
      case GSIFF_INJECT_SENSOR_TEMPERATURE_ACCEL_REQ:
      case GSIFF_INJECT_SENSOR_TEMPERATURE_GYRO_REQ:
         rv = gsiff_daemon_inject_sensor_data_handler(NULL,&msg_received->msg_data.sensor_temp_samples,
              msg_received->msg_type);
         break;
      case GSIFF_MOTION_DATA_REQ:
         rv = gsiff_daemon_motion_data_handler(&msg_received->msg_data.motion_data_req);
         break;
      case GSIFF_PEDOMETER_REQ:
         rv = gsiff_daemon_pedometer_handler(&msg_received->msg_data.pedometer_req);
         break;
      case GSIFF_SPI_STATUS_REQ:
         rv = gsiff_daemon_spi_status_handler(&msg_received->msg_data.spi_status_req);
         break;
      case GSIFF_CRADLE_MOUNT_CONFIG_REQ:
         rv = gsiff_daemon_cradle_mount_config_handler(&msg_received->msg_data.cradle_mount_config_req);
         break;
      default:
         LOC_LOGE("%s: Unexpected Message Type = %u in gsiff_data_task msg queue",
                  __FUNCTION__, msg_received->msg_type);
         break;
      }
   }
   else
   {
      LOC_LOGE("%s: Bad read from data message queue", __FUNCTION__);
   }

   if( msg_received != NULL )
   {
      free(msg_received);
   }

   return rv;
}

/*===========================================================================
FUNCTION    gsiff_ctl_task

DESCRIPTION
   Control Task Thread used by daemon process. Reads from a pipe message queue and
   calls appropriate message handlers based on received message. Thread runs
   forever unless killed by user. This is used for app control. Only accepts messages
   related to loc_api_glue and testing parameters. Accessible by message queue pipe.

DEPENDENCIES
   N/A

RETURN VALUE
   < 0  : Failure
   0 >= : Success

SIDE EFFECTS
   N/A

===========================================================================*/
static int gsiff_ctl_task(void* context)
{
   int* p_msg_q_id = (int*)context;
   int rv = 0;

   gsiff_msg msg_received;
   memset(&msg_received, 0, sizeof(msg_received));

   int bytes_received = gpsone_glue_msgrcv(*p_msg_q_id, &msg_received, sizeof(msg_received));
   if ( bytes_received == sizeof(msg_received) )
   {
       LOC_LOGI("%s: Handling message type = %u", __FUNCTION__, msg_received.msg_type);

       switch ( msg_received.msg_type )
       {
       case GSIFF_TIME_SYNC_IND:
           rv = gsiff_daemon_time_sync_handler(&msg_received.msg_data.time_sync_ind);
           break;
       case GSIFF_SENSOR_STREAMING_IND:
           rv = gsiff_daemon_sensor_streaming_handler(&msg_received.msg_data.sensor_streaming_ind);
           break;
       case GSIFF_SENSOR_TIME_OFFSET_CONFIG:
           rv = gsiff_daemon_sensor_time_offset_config_handler(&msg_received.msg_data.sensor_time_offset_config);
           break;
       case GSIFF_MOTION_DATA_IND:
           rv = gsiff_daemon_motion_data_streaming_handler(&msg_received.msg_data.motion_data_ind);
           break;
       case GSIFF_PEDOMETER_IND:
           rv = gsiff_daemon_pedometer_control_handler(&msg_received.msg_data.pedometer_ind);
           break;
       case GSIFF_SPI_STREAMING_IND:
           rv = gsiff_daemon_spi_streaming_handler(&msg_received.msg_data.spi_streaming_ind);
           break;
       case GSIFF_BAROMETER_IND:
           rv = gsiff_daemon_baro_request_handler(&msg_received.msg_data.bara_data_req);
           break;
       case GSIFF_TIME_SYNC_PIP_IND:
           rv = gsiff_daemon_time_sync_handler_for_slim_client(&msg_received.msg_data.pip_time_sync_req);
           break;
       default:
           LOC_LOGE("%s: Unexpected Message Type = %u in ctl_task msg queue", __FUNCTION__, msg_received.msg_type);
           break;
       }
   }
   else
   {
      LOC_LOGE("%s: Bad read from message queue. (%d out of %lu bytes)",
                __FUNCTION__, bytes_received, sizeof(msg_received));
   }

   return rv;
}

/*===========================================================================
FUNCTION    gsiff_dmn_destroy

DESCRIPTION
   Destroys LocApi, Sensor Provider, and variables used by the GSIFF daemon
   process. If any of these fail an error message is printed. However, all
   memory will be deallocated.

DEPENDENCIES
   N/A

RETURN VALUE
   < 0  : Failure
   0 >= : Success

SIDE EFFECTS
   N/A

===========================================================================*/
static int gsiff_dmn_destroy(void)
{
    /* Remove LocApi structures */
    if( gsiff_loc_api_close() == false )
    {
        LOC_LOGE("%s: Could not destroy loc api!", __FUNCTION__);
    }

    /* Remove Sensor Provider structures. */
    bool sp_rc = sp_destroy();
    if( sp_rc == false )
    {
        LOC_LOGE("%s: Could not destroy sensor provider rc = %d!", __FUNCTION__, sp_rc);
    }

    if( g_gsiff_control != NULL )
    {
        /* Remove ctl message queue */
        int msg_rc = gpsone_glue_msgremove(GSIFF_CTRL_Q_PATH, g_gsiff_control->msg_q_id);
        if( msg_rc != 0 )
        {
            LOC_LOGE("%s: Could not destroy msg queue rc = %d errno = %d!", __FUNCTION__, msg_rc, errno);
        }

        /* Remove data message queue */
        msg_rc = msg_q_destroy(&g_gsiff_control->data_msg_q);
        if( msg_rc != eMSG_Q_SUCCESS )
        {
            LOC_LOGE("%s: Could not destroy data msg queue rc = %d errno = %d!", __FUNCTION__, msg_rc, errno);
        }

        /* Free memory resources */
        free(g_gsiff_control);
        g_gsiff_control = NULL;
    }

    /* Remove sensor1 mutex/CV */
    int mutex_rc = pthread_mutex_destroy(&g_access_control->cb_mutex);
    if ( mutex_rc != 0 )
    {
        LOC_LOGE("%s: Could not destroy sensor1 cb mutex rc = %d errno = %d!",
                 __FUNCTION__, mutex_rc, errno);
    }

    int cv_rc = pthread_cond_destroy(&g_access_control->cb_arrived_cond);
    if ( cv_rc != 0 )
    {
        LOC_LOGE("%s: Could not destroy sensor1 cb CV rc = %d errno = %d!",
                 __FUNCTION__, cv_rc, errno);
    }

    free(g_access_control);
    g_access_control = NULL;

    gsiff_slim_client_connection_close();

    return 0;
}

/*===========================================================================
FUNCTION    gsiff_dmn_init

DESCRIPTION
   Initializes LocApi, Sensor Provider, and variables used by the GSIFF daemon
   process. If any of these fail the process exits with an error code.

   event_reg_mask: Event Registration mask to use for LocApi.

DEPENDENCIES
   N/A

RETURN VALUE
   < 0  : Failure
   0 >= : Success

SIDE EFFECTS
   N/A

===========================================================================*/
static int gsiff_dmn_init(uint64_t event_reg_mask, e_sensor_provider_type sensor_provider)
{
    const uint8_t MAX_ATTEMPTS = 10;
    const uint8_t ATTEMPT_SLEEP_INTERVAL_SEC = 3;
    uint8_t attempts = 0;

    if( g_gsiff_control != NULL )
    {
        gsiff_dmn_destroy();
    }

    /* Fill with default values */
    g_gsiff_control = (gsiff_control_t*)calloc(1, sizeof(*g_gsiff_control));
    if( NULL == g_gsiff_control )
    {
        LOC_LOGE("%s: Error: calloc error", __FUNCTION__);
        return -1;
    }

    g_gsiff_control->msg_q_id = -1;
    g_gsiff_control->mounted_state = DEFAULT_MOUNTED_STATE;

    /* Truly initialize data properly */

    /* Create Message Queue */
    g_gsiff_control->msg_q_id = gpsone_glue_msgget(GSIFF_CTRL_Q_PATH, O_RDWR);
    if( g_gsiff_control->msg_q_id < 0 )
    {
        LOC_LOGE("%s: Unable to initialize Message Queue!", __FUNCTION__);
        gsiff_dmn_destroy();
        return -1;
    }

    if( msg_q_init(&g_gsiff_control->data_msg_q) != eMSG_Q_SUCCESS )
    {
        LOC_LOGE("%s: Unable to initialize Data Message Queue!", __FUNCTION__);
        gsiff_dmn_destroy();
        return -1;
    }

    /* Initialize Sensor Provider */
    for( attempts = 0; attempts < MAX_ATTEMPTS; attempts++ )
    {
        if( sp_init(g_gsiff_control->data_msg_q, sensor_provider) == true )
        {
            break;
        }

        /* Sleep before trying again */
        sleep(ATTEMPT_SLEEP_INTERVAL_SEC);
    }

    /* Try 10 times in case the sensor provider is not properly booted up */
    if( attempts >= MAX_ATTEMPTS )
    {
        LOC_LOGE("%s: Unable to initialize Sensor Provider!", __FUNCTION__);
        gsiff_dmn_destroy();
        return -1;
    }

    LOC_LOGI("%s: Sensor Provider initialized on attempt %u!", __FUNCTION__, attempts);

    /* Initialize LocApi */
    for( attempts = 0; attempts < MAX_ATTEMPTS; attempts++ )
    {
        if( gsiff_loc_api_open(g_gsiff_control->msg_q_id, event_reg_mask) == true )
        {
            break;
        }

        /* Sleep before trying again */
        sleep(ATTEMPT_SLEEP_INTERVAL_SEC);
    }

    /* Try 10 times in case the modem is not properly booted up */
    if( attempts >= MAX_ATTEMPTS )
    {
        LOC_LOGE("%s: Unable to initialize Loc Api!", __FUNCTION__);
        gsiff_dmn_destroy();
        return -1;
    }

    LOC_LOGI("%s: Loc Api initialized on attempt %u!", __FUNCTION__, attempts);

    /* Create server socket thread for establishing communication to PIP Engine */
    if (pthread_create(&(server_thread), NULL, gsiff_slim_client_socket_server_thread, (void *)(&g_gsiff_control->msg_q_id)) != 0)
    {
        LOC_LOGE("%s: Error cannot create server thread", __FUNCTION__);
        return -1;
    }

    return 0;
}

/*===========================================================================
FUNCTION    main

DESCRIPTION
   Startup of daemon process. Simply initializes daemon and starts up control
   thread to process message queue requests. Then waits until control thread
   exits (which should be never).

DEPENDENCIES
   N/A

RETURN VALUE
   < 0  : Failure
   0 >= : Success

SIDE EFFECTS
   N/A

===========================================================================*/
int main(int argc, char *argv[])
{
   uint8_t  requested_mounted_state = DEFAULT_MOUNTED_STATE;
   uint64_t requested_event_reg_mask = 0;
   char* endptr;
   int c;
   static int dsps_fd = 0;

   /* Read logging configuration and sensor provider */
   UTIL_READ_CONF(SAP_CONF_FILE, conf_parameter_table);
   UTIL_READ_CONF_DEFAULT(GPS_CONF_FILE);

   opterr = 0;

   while ((c = getopt(argc, argv, "b:s:m:agtpdhn")) != -1)
   {
      switch (c)
      {

      /* Mounted State - 1 to send mounted. 0 to send unmounted. */
      case 'm':
         LOC_LOGV("%s: Processing mounted state param = %s", __FUNCTION__, optarg);
         errno = 0;
         requested_mounted_state = strtoul(optarg, &endptr, 10);

         /* Bad Mounted State provided */
         if( errno != 0 || *endptr != '\0' || requested_mounted_state > SP_MSI_DO_NOT_SEND )
         {
            LOC_LOGE("%s: Invalid mounted state = %s", __FUNCTION__, optarg);
            requested_mounted_state = DEFAULT_MOUNTED_STATE;
         }
         break;

      /* Enable SPI Reporting */
      case 'p':
         LOC_LOGV("%s: XSPI Reporting enabled", __FUNCTION__);
         requested_event_reg_mask |= QMI_LOC_EVENT_MASK_SET_SPI_STREAMING_REPORT_V02;
         break;

      /* Enable Sensor Reporting */
      case 's':
         LOC_LOGV("%s: Sensor Reporting enabled", __FUNCTION__);
         requested_event_reg_mask |= QMI_LOC_EVENT_MASK_SENSOR_STREAMING_READY_STATUS_V02;
         break;

      /* Enable Time Sync Reporting */
      case 't':
         LOC_LOGV("%s: Time Sync Reporting enabled", __FUNCTION__);
         requested_event_reg_mask |= QMI_LOC_EVENT_MASK_TIME_SYNC_REQ_V02;
         break;

      /* Unknown Option */
      case '?':
         if (optopt == 's' || optopt == 'b' || optopt == 'm')
         {
            LOC_LOGE("%s: Option -%c requires an argument.", __FUNCTION__, optopt);
         }
         else if (isprint(optopt))
         {
            LOC_LOGE("%s: Unknown option `-%c'.", __FUNCTION__, optopt);
         }
         else
         {
            LOC_LOGE("%s: Unknown option character `\\x%x'.", __FUNCTION__, optopt);
         }

         /* Intentional code fall-through */
      default:
         LOC_LOGE("USAGE:\n"
                  "%s: [-m mounted_state]\n"
                  "    m - Mounted State to Send (0=Unmounted, 1=Mounted, 2=Unknown, 3=Do not Send)\n"
                  "    s - Register for Sensor Indications (Default)\n"
                  "    t - Register for Time Sync Indications (Default)\n"
                  "    p - Register for XSPI Indications (Default)\n"
                  "Example (w/ Defaults selected): %s -s -m 1 -t -p -d\n\n",
                  argv[0], argv[0]);
         return -1;
      }
   }

   /* If sensor usage is disabled by gps.conf then Wait on the thread forever. */
   if(g_sensor_usage != 0)
   {
      LOC_LOGI("%s: Sensor usage disabled!", __FUNCTION__);
      pthread_mutex_init(&lock, NULL);
      pthread_mutex_lock(&lock);
      pthread_mutex_lock(&lock);
      /* Sleep for one hour */
      sleep(3600);
      return -1;
   }

   /* If a bad sensor provider is given then revert to the default. */
   if( g_sensor_provider <= MIN_SENSOR_PROVIDER || g_sensor_provider >= MAX_SENSOR_PROVIDER )
   {
      g_sensor_provider = (uint32_t)DEFAULT_SENSOR_PROVIDER;
   }

   /* If no event registration mask given then use the default */
   if( requested_event_reg_mask == 0 )
   {
      requested_event_reg_mask = DEFAULT_EVT_REG_MASK;
   }

   if( g_access_control != NULL )
    {
        free(g_access_control);
        g_access_control = NULL;
    }

   /* Fill with default values */
    g_access_control = (access_control_t*)calloc(1, sizeof(*g_access_control));

    if( NULL == g_access_control )
    {
        LOC_LOGE("%s: Error: calloc error", __FUNCTION__);
        return -1;
    }

   /*initialize mutex/cv to synchronize sensor1 api calls */
   pthread_mutex_init(&(g_access_control->cb_mutex), NULL);
   pthread_cond_init(&(g_access_control->cb_arrived_cond), NULL);

#ifdef FEATURE_GSIFF_DSPS
   if( (e_sensor_provider_type)g_sensor_provider == SENSOR1_SENSOR_PROVIDER )
   {
      sensor1_open_result = sensor1_init();

      LOC_LOGV("%s: Sensor1 init: %d", __FUNCTION__, sensor1_open_result);
      /* Check that sensor1 was successfully initialized */
      if(SENSOR1_SUCCESS != sensor1_open_result)
      {
         LOC_LOGE("Fatal: Could not initialize Sensor1 sensor1_init = %d!", sensor1_open_result);
      }
      else
      {
         sensor1_open_result = sensor1_open(&p_sensor1_hndl,sensor1_notify_data_callback,(intptr_t)NULL);
         LOC_LOGV("%s: Sensor1 open: %d", __FUNCTION__, sensor1_open_result);
      }

      switch (sensor1_open_result)
      {
        case SENSOR1_SUCCESS:
           LOC_LOGI("%s: Sensor1 connection opened successfully!", __FUNCTION__);
           break;

        case SENSOR1_EWOULDBLOCK:
           LOC_LOGI("%s: Pending Sensor1 connection opening!", __FUNCTION__);
           break;

        default:
           LOC_LOGI("%s: Fall back to Android NDK as Sensor Core is not available!", __FUNCTION__);
           g_sensor_provider = (uint32_t)ANDROID_NDK_SENSOR_PROVIDER;
           break;
      }
   }
#else
    LOC_LOGI("%s: Fall back to Android NDK as Sensor Core is not on this target!", __FUNCTION__);
    g_sensor_provider = (uint32_t)ANDROID_NDK_SENSOR_PROVIDER;
#endif

   /* Initialize module structures */
   if( gsiff_dmn_init(requested_event_reg_mask, (e_sensor_provider_type)g_sensor_provider) < 0 )
   {
      LOC_LOGE("%s: Unable to initialize GSIFF!", __FUNCTION__);
      return -1;
   }

   LOC_LOGI("%s: Mounted state default = %u and selection = %u", __FUNCTION__, g_gsiff_control->mounted_state, requested_mounted_state);
   g_gsiff_control->mounted_state = requested_mounted_state;

   /* Create task to listen for message queue updates */
   gpsone_launch_thelper(&g_gsiff_control->ctl_task_helper,
                         NULL,           /* Initialize func */
                         NULL,           /* Pre-Process func */
                         gsiff_ctl_task, /* Process func */
                         NULL,           /* Post-Process func */
                         &g_gsiff_control->msg_q_id);

   /* Create task to listen for message queue updates */
   gpsone_launch_thelper(&g_gsiff_control->data_task_helper,
                         NULL,            /* Initialize func */
                         NULL,            /* Pre-Process func */
                         gsiff_data_task, /* Process func */
                         NULL,            /* Post-Process func */
                         g_gsiff_control->data_msg_q);

   /* Wait until the Control/Data Task exits. This should be never */
   gpsone_join_thelper(&g_gsiff_control->ctl_task_helper);
   gpsone_join_thelper(&g_gsiff_control->data_task_helper);

   /* Clean up all resources */
   gsiff_dmn_destroy();

   return 0;
}
