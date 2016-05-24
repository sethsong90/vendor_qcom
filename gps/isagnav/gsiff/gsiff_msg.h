/******************************************************************************
  @file:  gsiff_msg.h
  @brief: GNSS / Sensor Interface Framework Support

  DESCRIPTION
    This file defines the gsiff control message to be used in the messaging queue
    of the gsiff_daemon_manager. The control task processes these messages.

  INITIALIZATION AND SEQUENCING REQUIREMENTS

  -----------------------------------------------------------------------------
  Copyright (c) 2011, 2014 Qualcomm Technologies, Inc.
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
08/01/11   jb       Remove custom Sensor Streaming structure as the QMI
                    version has been enhanced with necessary info.
10/24/12   vr       1. Adding temperature streaming.
                    2. Adding motion data streming.
                    3. Adding pedometer streaming.
======================================================================*/

#ifndef __GSIFF_MSG_H__
#define __GSIFF_MSG_H__

#include <stddef.h>
#include <pthread.h>
#include "location_service_v02.h"
#include "gsiff_api.h" //baro api

#ifdef DEBUG_X86
#define GSIFF_CTRL_Q_PATH "/tmp/gsiff_ctrl_q"
#else
#define GSIFF_CTRL_Q_PATH "/data/misc/location/gsiff/gsiff_ctrl_q"
#endif

typedef enum
{
    GSIFF_INVALID = 0,
    GSIFF_SENSOR_STREAMING_IND,
    GSIFF_TIME_SYNC_IND,
    GSIFF_INJECT_SENSOR_DATA_ACCEL_REQ,
    GSIFF_INJECT_SENSOR_DATA_GYRO_REQ,
    GSIFF_INJECT_SENSOR_TEMPERATURE_ACCEL_REQ,
    GSIFF_INJECT_SENSOR_TEMPERATURE_GYRO_REQ,
    GSIFF_SPI_STATUS_REQ,
    GSIFF_CRADLE_MOUNT_CONFIG_REQ,
    GSIFF_SENSOR_TIME_OFFSET_CONFIG,
    GSIFF_SPI_STREAMING_IND,
    GSIFF_MOTION_DATA_IND,
    GSIFF_MOTION_DATA_REQ,
    GSIFF_PEDOMETER_IND,
    GSIFF_PEDOMETER_REQ,
    GSIFF_BAROMETER_IND,//REQUEST
    GSIFF_BAROMETER_REQ,//INJECTION TBD: remove if unused?
    GSIFF_TIME_SYNC_PIP_IND //Time sync request from PIP engine
} e_gsiff_msg_type;

typedef struct gsiff_sensor_time_offset_config {
    uint32_t time_offset_ms;
    uint32_t time_offset_duration_ms;
} gsiff_sensor_time_offset_config;


typedef struct gsiff_msg {
    size_t msg_size;                                                             /* Size of message in bytes */
    e_gsiff_msg_type msg_type;                                                   /* Type of message */
    union {
        qmiLocEventTimeSyncReqIndMsgT_v02 time_sync_ind;                          /* Time Sync Indication */
        qmiLocEventSensorStreamingReadyStatusIndMsgT_v02 sensor_streaming_ind;    /* Sensor Injection Indication */
        qmiLoc3AxisSensorSampleListStructT_v02 sensor_data_samples;               /* Sensor Data Injection */
        qmiLocSensorTemperatureSampleListStructT_v02 sensor_temp_samples;         /* Sensor Temperature Injection */
        qmiLocInjectMotionDataReqMsgT_v02 motion_data_req;                        /* Motion Data Injection*/
        qmiLocEventMotionDataControlIndMsgT_v02 motion_data_ind;                  /* Motion Data Injection Indication */
        qmiLocPedometerReportReqMsgT_v02 pedometer_req;                           /* Pedometer Injection*/
        qmiLocEventPedometerControlIndMsgT_v02 pedometer_ind;                     /* Pedometer Injection Indication */
        qmiLocSetSpiStatusReqMsgT_v02 spi_status_req;                             /* XSPI - Stationary State*/
        qmiLocSetCradleMountConfigReqMsgT_v02 cradle_mount_config_req;            /* XMSI - Mounted State */
        gsiff_sensor_time_offset_config sensor_time_offset_config;                /* Sensor Time Offset Configuration (Testing) */
        qmiLocEventSetSpiStreamingReportIndMsgT_v02 spi_streaming_ind;            /* SPI Streaming Indication */
        slimBaroDataReqPayloadStructT bara_data_req;                              /* PIP baro data request */
        slimTimeSyncReqStructT pip_time_sync_req;                                 /* PIP time sync request */
    } msg_data;
} gsiff_msg;

/*mutex/cv to synchronize sensor1 api calls*/
typedef struct access_control_t {
    pthread_mutex_t         cb_mutex;                 /* mutex lock for sensor1 callback */
    pthread_cond_t          cb_arrived_cond;          /* cond variable to signal callback has arrived */
} access_control_t;

#endif /* __GSIFF_MSG_H__ */
