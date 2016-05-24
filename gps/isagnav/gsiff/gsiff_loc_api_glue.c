/******************************************************************************
  @file:  gsiff_loc_api_glue.c
  @brief: GNSS / Sensor Interface Framework Support

  DESCRIPTION
    This file defines the module where gsiff interacts with Loc Api.

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
08/01/11   jb       1. Add a way to send messages synchornously over LocAPI
                       - Needed for the bootup messages.
10/24/12   vr       1. Adding temperature streaming.
                    2. Adding motion data streming.
                    3. Adding pedometer streaming.
======================================================================*/

#include "gsiff_loc_api_glue.h"
#include "gsiff_msg.h"

#include "location_service_v02.h"

#include "gpsone_glue_msg.h"

#define LOG_TAG "gsiff_dmn"

#include "log_util.h"
#include "loc_api_sync_req.h"
#include <string.h>
#include <pthread.h>

/* Opaque LocApi handle */
static locClientHandleType g_loc_api_handle = LOC_CLIENT_INVALID_HANDLE_VALUE;
/* msg_q id */
static int g_msg_q_id = -1;
/* lock loc_api sends */
static pthread_mutex_t g_loc_send_mutex     = PTHREAD_MUTEX_INITIALIZER;

/*===========================================================================
FUNCTION    loc_api_event_ind_callback

DESCRIPTION
  Callback function for LocApi Indication events.

DEPENDENCIES
   N/A

RETURN VALUE
   N/A

SIDE EFFECTS
   N/A

===========================================================================*/
void loc_api_event_ind_callback( locClientHandleType handle,
                                 uint32_t event_ind_id,
                                 const locClientEventIndUnionType event_ind_payload,
                                 void* pClientCookie)
{
   LOC_LOGI("%s: Received LocAPI Event ind = %u", __FUNCTION__, event_ind_id);
   switch( event_ind_id )
   {
   case QMI_LOC_EVENT_TIME_SYNC_REQ_IND_V02:
      if( NULL != event_ind_payload.pTimeSyncReqEvent )
      {
         const qmiLocEventTimeSyncReqIndMsgT_v02* pTimeSyncReqEvent =
            event_ind_payload.pTimeSyncReqEvent;

         /* Place message in ctl_task message queue to process */
         gsiff_msg time_sync_msg;
         memset(&time_sync_msg, 0, sizeof(time_sync_msg));
         time_sync_msg.msg_type = GSIFF_TIME_SYNC_IND;
         time_sync_msg.msg_data.time_sync_ind = *pTimeSyncReqEvent;

         gpsone_glue_msgsnd(g_msg_q_id, &time_sync_msg, sizeof(time_sync_msg));
      }
      else
      {
         LOC_LOGE("%s: TimeSyncIndMsg is NULL when it shouldn't be", __FUNCTION__);
      }
      break;

   case QMI_LOC_EVENT_SENSOR_STREAMING_READY_STATUS_IND_V02:
      if( NULL != event_ind_payload.pSensorStreamingReadyStatusEvent )
      {
         const qmiLocEventSensorStreamingReadyStatusIndMsgT_v02* pSnsRdyStatusEvent =
            event_ind_payload.pSensorStreamingReadyStatusEvent;

         /* Place message in ctl_task message queue to process */
         gsiff_msg sensor_streaming_msg;
         memset(&sensor_streaming_msg, 0, sizeof(sensor_streaming_msg));
         sensor_streaming_msg.msg_type = GSIFF_SENSOR_STREAMING_IND;
         sensor_streaming_msg.msg_data.sensor_streaming_ind = *pSnsRdyStatusEvent;

         gpsone_glue_msgsnd(g_msg_q_id, &sensor_streaming_msg, sizeof(sensor_streaming_msg));
      }
      else
      {
         LOC_LOGE("%s: SensorStreamingReadyStatusEvent is NULL when it shouldn't be", __FUNCTION__);
      }
      break;

   case QMI_LOC_EVENT_SET_SPI_STREAMING_REPORT_IND_V02:
      if( NULL != event_ind_payload.pSetSpiStreamingReportEvent )
      {
         const qmiLocEventSetSpiStreamingReportIndMsgT_v02* pSetSpiStreamingReportEvent =
            event_ind_payload.pSetSpiStreamingReportEvent;

         /* Place message in ctl_task message queue to process */
         gsiff_msg spi_streaming_msg;
         memset(&spi_streaming_msg, 0, sizeof(spi_streaming_msg));
         spi_streaming_msg.msg_type = GSIFF_SPI_STREAMING_IND;
         spi_streaming_msg.msg_data.spi_streaming_ind = *pSetSpiStreamingReportEvent;

         gpsone_glue_msgsnd(g_msg_q_id, &spi_streaming_msg, sizeof(spi_streaming_msg));
      }
      else
      {
         LOC_LOGE("%s: SetSpiStreamingReportEvent is NULL when it shouldn't be", __FUNCTION__);
      }
      break;

   case QMI_LOC_EVENT_MOTION_DATA_CONTROL_IND_V02:
      if( NULL != event_ind_payload.pMotionDataControlEvent )
      {
         const qmiLocEventMotionDataControlIndMsgT_v02* pMotionDataControlEvent =
            event_ind_payload.pMotionDataControlEvent;

         /* Place message in ctl_task message queue to process */
         gsiff_msg motion_data_msg;
         memset(&motion_data_msg, 0, sizeof(motion_data_msg));
         motion_data_msg.msg_type = GSIFF_MOTION_DATA_IND;
         motion_data_msg.msg_data.motion_data_ind = *pMotionDataControlEvent;

         gpsone_glue_msgsnd(g_msg_q_id, &motion_data_msg, sizeof(motion_data_msg));
      }
      else
      {
         LOC_LOGE("%s: InjectMotionDataEvent is NULL when it shouldn't be", __FUNCTION__);
      }
      break;

   case QMI_LOC_EVENT_PEDOMETER_CONTROL_IND_V02:
      if( NULL != event_ind_payload.pPedometerControlEvent )
      {
         const qmiLocEventPedometerControlIndMsgT_v02* pPedometerControlEvent =
            event_ind_payload.pPedometerControlEvent;

         /* Place message in ctl_task message queue to process */
         gsiff_msg pedometer_msg;
         memset(&pedometer_msg, 0, sizeof(pedometer_msg));
         pedometer_msg.msg_type = GSIFF_PEDOMETER_IND;
         pedometer_msg.msg_data.pedometer_ind = *pPedometerControlEvent;

         gpsone_glue_msgsnd(g_msg_q_id, &pedometer_msg, sizeof(pedometer_msg));
      }
      else
      {
         LOC_LOGE("%s: PedometerControlEvent is NULL when it shouldn't be", __FUNCTION__);
      }
      break;

   default:
      LOC_LOGE("%s: Error - invalid msg type in cb: %u", __FUNCTION__, event_ind_id);
      break;
   }
}

/*===========================================================================
FUNCTION    loc_api_resp_ind_callback

DESCRIPTION
  Callback function for LocApi Response Indications (i.e indications from client
  requests).

DEPENDENCIES
   N/A

RETURN VALUE
   N/A

SIDE EFFECTS
   N/A

===========================================================================*/
void loc_api_resp_ind_callback( locClientHandleType handle,
                                uint32_t resp_ind_id,
                                const locClientRespIndUnionType resp_ind_payload,
                                void* pClientCookie)
{
   LOC_LOGI("%s: Received LocAPI Resp ind = %u", __FUNCTION__, resp_ind_id);

   /* Process Loc API indications to wake up blocked user threads if send was synchronous */
   /* Use pDeleteAssistDataInd as a dummy pointer */
   loc_sync_process_ind(
                       g_loc_api_handle,
                       resp_ind_id,
                       (void *)resp_ind_payload.pDeleteAssistDataInd
                       );

   /* All responses have a status field. If not NULL then check status field and log failures */
   if( resp_ind_payload.pSetSpiStatusInd != NULL )
   {
      if( resp_ind_payload.pSetSpiStatusInd->status != eQMI_LOC_SUCCESS_V02 )
      {
         LOC_LOGE("%s: Error - Bad Status in Resp Ind: %u", __FUNCTION__, resp_ind_payload.pSetSpiStatusInd->status);
         return;
      }
   }

   switch( resp_ind_id )
   {
   /* Expected indications listed below. */
   case QMI_LOC_INJECT_SENSOR_DATA_IND_V02:
   case QMI_LOC_INJECT_TIME_SYNC_DATA_IND_V02:
   case QMI_LOC_SET_CRADLE_MOUNT_CONFIG_IND_V02:
   case QMI_LOC_SET_SPI_STATUS_IND_V02:
   case QMI_LOC_SET_SENSOR_CONTROL_CONFIG_IND_V02:
   case QMI_LOC_GET_SENSOR_CONTROL_CONFIG_IND_V02:
   case QMI_LOC_SET_SENSOR_PROPERTIES_IND_V02:
   case QMI_LOC_GET_SENSOR_PROPERTIES_IND_V02:
   case QMI_LOC_SET_SENSOR_PERFORMANCE_CONTROL_CONFIGURATION_IND_V02:
   case QMI_LOC_GET_SENSOR_PERFORMANCE_CONTROL_CONFIGURATION_IND_V02:
   case QMI_LOC_INJECT_MOTION_DATA_IND_V02:
   case QMI_LOC_PEDOMETER_REPORT_IND_V02:
      break;

   default:
      LOC_LOGE("%s: Error - invalid msg type in cb: %u", __FUNCTION__, resp_ind_id);
      break;
   }
}

/* ----------------------- END INTERNAL FUNCTIONS ---------------------------------------- */

static locClientCallbacksType loc_api_callbacks =
{
   sizeof(locClientCallbacksType),
   loc_api_event_ind_callback,
   loc_api_resp_ind_callback,
   NULL
} ;

/*===========================================================================

  FUNCTION:   gsiff_loc_api_open

  ===========================================================================*/
bool gsiff_loc_api_open(int msg_q_id, uint64_t event_reg_mask)
{
   /* Opening when not closed properly */
   if( g_loc_api_handle != LOC_CLIENT_INVALID_HANDLE_VALUE )
   {
      LOC_LOGE("%s: Opening Loc API when it was already initialized!", __FUNCTION__);
      return false;
   }

   g_msg_q_id = msg_q_id;

   loc_sync_req_init();

   locClientStatusEnumType loc_api_rc = locClientOpen(
                                          event_reg_mask,
                                          &loc_api_callbacks,
                                          &g_loc_api_handle,
                                          NULL
                                        );

   LOC_LOGI("%s: locClientOpen %d %u", __FUNCTION__, loc_api_rc, (int)g_loc_api_handle );
   /* Check that locapi client was successfully created */
   if(eLOC_CLIENT_SUCCESS != loc_api_rc)
   {
      g_msg_q_id = -1;
      g_loc_api_handle = LOC_CLIENT_INVALID_HANDLE_VALUE;
      LOC_LOGE("%s: Could not initialize Loc Api Client!", __FUNCTION__);
      return false;
   }

   return true;
}

/*===========================================================================

  FUNCTION:   gsiff_loc_api_close

  ===========================================================================*/
bool gsiff_loc_api_close()
{
   /* Remove LocApi structures */
   if( g_loc_api_handle != LOC_CLIENT_INVALID_HANDLE_VALUE )
   {
      locClientStatusEnumType loc_api_rc = locClientClose(& g_loc_api_handle );
      if( loc_api_rc != eLOC_CLIENT_SUCCESS )
      {
         LOC_LOGE("%s: Could not close LocApi %d!", __FUNCTION__, loc_api_rc);
         return false;
      }

      g_loc_api_handle = LOC_CLIENT_INVALID_HANDLE_VALUE;
      g_msg_q_id = -1;
      return true;
   }
   else
   {
      LOC_LOGE("%s: Improperly closing Loc API Client!", __FUNCTION__);
      return false;
   }
}

/*===========================================================================

  FUNCTION:   gsiff_loc_api_send

  ===========================================================================*/
locClientStatusEnumType gsiff_loc_api_send(uint32_t reqId, locClientReqUnionType* reqPayload)
{
   if( g_loc_api_handle != LOC_CLIENT_INVALID_HANDLE_VALUE )
   {
      pthread_mutex_lock(&g_loc_send_mutex);

      locClientStatusEnumType rv = locClientSendReq(g_loc_api_handle,
                       reqId,
                       *reqPayload);

      pthread_mutex_unlock(&g_loc_send_mutex);

      /* Pipe to modem is down if it is not successful */
      if( rv != eLOC_CLIENT_SUCCESS )
      {
         /* Possibly abort here to achieve the modem restart requirement. */
         LOC_LOGE("%s: Failure to send LocAPI req id = %lu rv = %d!", __FUNCTION__, reqId, rv);
      }

      return rv;
   }
   /* Not properly initialized */
   else
   {
      LOC_LOGE("%s: Uninitialized and sending Loc API Message!", __FUNCTION__);
      return eLOC_CLIENT_FAILURE_NOT_INITIALIZED;
   }
}

/*===========================================================================

  FUNCTION:   gsiff_loc_api_send_sync

  ===========================================================================*/
locClientStatusEnumType gsiff_loc_api_send_sync(
   uint32_t                  reqId,
   locClientReqUnionType*    reqPayload,
   uint32_t                  timeout_msec,
   uint32_t                  ind_id,          /* ind ID to block for, usually the same as req_id */
   void                      *ind_payload_ptr /* can be NULL*/
   )
{
   if( g_loc_api_handle != LOC_CLIENT_INVALID_HANDLE_VALUE )
   {
      pthread_mutex_lock(&g_loc_send_mutex);

      locClientStatusEnumType rv = loc_sync_send_req (
                                                     g_loc_api_handle,
                                                     reqId,
                                                     *reqPayload,
                                                     timeout_msec,
                                                     ind_id,
                                                     ind_payload_ptr
                                                     );

      pthread_mutex_unlock(&g_loc_send_mutex);

      /* Pipe to modem is down if it is not successful */
      if( rv != eLOC_CLIENT_SUCCESS )
      {
         /* Possibly abort here to achieve the modem restart requirement. */
         LOC_LOGE("%s: Failure to send LocAPI req id = %lu rv = %d!", __FUNCTION__, reqId, rv);
      }

      return rv;
   }
   /* Not properly initialized */
   else
   {
      LOC_LOGE("%s: Uninitialized and sending Loc API Message!", __FUNCTION__);
      return eLOC_CLIENT_FAILURE_NOT_INITIALIZED;
   }
}

/*===========================================================================

  FUNCTION:   gsiff_loc_api_get_handle

  ===========================================================================*/
locClientHandleType gsiff_loc_api_get_handle()
{
   return g_loc_api_handle;
}
