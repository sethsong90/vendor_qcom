/******************************************************************************
  @file:  loc_api_v02_client.c
  @brief: Implements the GPS LOC API 2.0 interface on top of QCCI
  framework

  DESCRIPTION
  Implements the GPS LOC API 2.0 interface on top of QCCI
  framework


  INITIALIZATION AND SEQUENCING REQUIREMENTS

  Copyright (c) 2011 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

******************************************************************************/

/*=====================================================================
                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why
--------   ---      ---------------------------------------------------
03/20/11   nks      Initial version
08/01/11   jb       1. Misc typedef changes

======================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include "loc_api_v02_client.h"
#include "log_util.h"
#include "gpsone_thread_helper.h"

/* right now client handle is meaning less since locclient library
   runs in the context of calling process all clients will get the
   same handle */
#define LOC_CLIENT_VALID_HANDLE (1234)
#define DEFAULT_BATCHING_RATE 4
#define DEFAULT_SAMPLING_RATE 60

/** @struct locClientInternalState
 */

typedef struct
{
  bool isInitialized;
  locClientHandleType clientHandle;/*  handle returned to loc API client */
  uint8_t fixSessionState; /* on going fix? */
  uint8_t gpsOnState; /*  gps on ? */
  locClientEventIndCbType eventCallback;
  locClientRespIndCbType respCallback;
  locClientEventMaskType eventRegMask;
  struct gpsone_thelper  time_sync_task_helper;
  struct gpsone_thelper  accel_data_task_helper;
  struct gpsone_thelper  gyro_data_task_helper;
  bool                   gyro_streaming_status;
  bool                   accel_streaming_status;
  uint32_t               ref_counter;
}locClientInternalState;

/* internal state of the Loc Client */
locClientInternalState gLocClientState;

#ifdef __cplusplus
extern "C"
{
#endif

/*===========================================================================
 *
 *                          FUNCTION DECLARATION
 *
 *==========================================================================*/

/** convertResponseToStatus
 @brief converts a qmiLocGenRespMsgT to locClientStatusEnumType*
 @param [in] pResponse; pointer to the response received from
        QMI_LOC service.
 @return locClientStatusEnumType corresponding to the
         response.
*/

static locClientStatusEnumType convertResponseToStatus(
  qmiLocGenRespMsgT_v02 *pResponse)
{

  LOC_LOGD("convertResponseToStatus: resp.result = %d, resp.error = %d"
                 ,pResponse->resp.result,  pResponse->resp.error);
  if(pResponse->resp.result == 0)
  {
    return(eLOC_CLIENT_SUCCESS);
  }
  else
  {
    return(eLOC_CLIENT_FAILURE_INTERNAL);
  }
}

/**  validateRequest
  @brief validates the input request
  @param [in] reqId       request ID
  @param [in] reqPayload  Union of pointers to message payload
  @param [out] ppOutData  Pointer to void *data if successful
  @param [out] pOutLen    Pointer to length of data if succesful.
  @return false on failure, true on Success
*/

static bool validateRequest(
  uint32_t                    reqId,
  const locClientReqUnionType reqPayload,
  void                        **ppOutData,
  uint32_t                    *pOutLen )

{
  bool noPayloadFlag = false;

  LOC_LOGD("%s:%d]: reqId = %d", __func__, __LINE__, reqId);
  switch(reqId)
  {
    case QMI_LOC_INFORM_CLIENT_REVISION_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInformClientRevisionReqMsgT_v02);
      break;
    }

    case QMI_LOC_REG_EVENTS_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocRegEventsReqMsgT_v02);
       break;
    }

    case QMI_LOC_START_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocStartReqMsgT_v02);
       break;
    }

    case QMI_LOC_STOP_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocStopReqMsgT_v02);
       break;
    }

    case QMI_LOC_NI_USER_RESPONSE_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocNiUserRespReqMsgT_v02);
       break;
    }

    case QMI_LOC_INJECT_PREDICTED_ORBITS_DATA_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInjectPredictedOrbitsDataReqMsgT_v02);
      break;
    }

    case QMI_LOC_INJECT_UTC_TIME_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInjectUtcTimeReqMsgT_v02);
      break;
    }

    case QMI_LOC_INJECT_POSITION_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInjectPositionReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_ENGINE_LOCK_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSetEngineLockReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_SBAS_CONFIG_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSetSbasConfigReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_NMEA_TYPES_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSetNmeaTypesReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_LOW_POWER_MODE_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSetLowPowerModeReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_SERVER_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSetServerReqMsgT_v02);
      break;
    }

    case QMI_LOC_DELETE_ASSIST_DATA_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocDeleteAssistDataReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_XTRA_T_SESSION_CONTROL_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSetXtraTSessionControlReqMsgT_v02);
      break;
    }

    case QMI_LOC_INJECT_WIFI_POSITION_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInjectWifiPositionReqMsgT_v02);
      break;
    }

    case QMI_LOC_NOTIFY_WIFI_STATUS_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocNotifyWifiStatusReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_OPERATION_MODE_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSetOperationModeReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_SPI_STATUS_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSetSpiStatusReqMsgT_v02);
      break;
    }

    case QMI_LOC_INJECT_SENSOR_DATA_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInjectSensorDataReqMsgT_v02);
      break;
    }

    case QMI_LOC_INJECT_TIME_SYNC_DATA_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInjectTimeSyncDataReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_CRADLE_MOUNT_CONFIG_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSetCradleMountConfigReqMsgT_v02);
      break;
    }

    case QMI_LOC_SET_EXTERNAL_POWER_CONFIG_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocSetExternalPowerConfigReqMsgT_v02);
      break;
    }

    case QMI_LOC_INFORM_LOCATION_SERVER_CONN_STATUS_REQ_V02:
    {
      *pOutLen = sizeof(qmiLocInformLocationServerConnStatusReqMsgT_v02);
      break;
    }

    // ALL requests with no payload
    case QMI_LOC_GET_SERVICE_REVISION_REQ_V02:
    case QMI_LOC_GET_FIX_CRITERIA_REQ_V02:
    case QMI_LOC_GET_PREDICTED_ORBITS_DATA_SOURCE_REQ_V02:
    case QMI_LOC_GET_PREDICTED_ORBITS_DATA_VALIDITY_REQ_V02:
    case QMI_LOC_GET_ENGINE_LOCK_REQ_V02:
    case QMI_LOC_GET_SBAS_CONFIG_REQ_V02:
    case QMI_LOC_GET_NMEA_TYPES_REQ_V02:
    case QMI_LOC_GET_LOW_POWER_MODE_REQ_V02:
    case QMI_LOC_GET_SERVER_REQ_V02:
    case QMI_LOC_GET_XTRA_T_SESSION_CONTROL_REQ_V02:
    case QMI_LOC_GET_REGISTERED_EVENTS_REQ_V02:
    case QMI_LOC_GET_OPERATION_MODE_REQ_V02:
    case QMI_LOC_GET_CRADLE_MOUNT_CONFIG_REQ_V02:
    case QMI_LOC_GET_EXTERNAL_POWER_CONFIG_REQ_V02:
    {
      noPayloadFlag = true;
      break;
    }

    default:
      LOC_LOGW("%s:%d]: Error unknown reqId=%d", __func__, __LINE__,
                    reqId);
      return false;
  }
  if(true == noPayloadFlag)
  {
    *ppOutData = NULL;
    *pOutLen = 0;
  }
  else
  {
    //set dummy pointer for request union
    *ppOutData = (void*) reqPayload.pInformClientRevisionReq;
  }
  LOC_LOGV("%s:%d]: reqId=%d, len = %d", __func__, __LINE__,
                reqId, *pOutLen);
  return true;
}

/** locClientInit
 @brief Initialize the internal state of the client.
 @param [in]              clnt; the qmi_client_Type is copied
                          for future reference
 @param [in] eventRegMask Mask of asynchronous events the
                          client is interested in receiving
 @param [in] eventIndCb   Function to be invoked to handle
                          an event.
 @param [in] respIndCb    Function to be invoked to handle a
                          response indication.
*/

static bool locClientInit( locClientEventMaskType eventRegMask,
  locClientEventIndCbType eventIndCb,
  locClientRespIndCbType  respIndCb)
{
  LOC_LOGD("locClientInit: eventMask = %ld", eventRegMask);
  gLocClientState.isInitialized = true;
  gLocClientState.clientHandle = LOC_CLIENT_VALID_HANDLE;
  gLocClientState.eventCallback = eventIndCb;
  gLocClientState.respCallback  = respIndCb;
  gLocClientState.eventRegMask  = eventRegMask;
  return true;
}

/** locClientDeInit
 @brief De-Initialize the internal state of the client*
*/

static bool locClientDeInit()
{
  LOC_LOGD("locClientDeInit: ");
  gLocClientState.isInitialized = false;
  gLocClientState.clientHandle =  LOC_CLIENT_INVALID_HANDLE_VALUE;
  gLocClientState.eventCallback = NULL;
  gLocClientState.respCallback  = NULL;
  gLocClientState.eventRegMask  = 0;
  gLocClientState.gyro_streaming_status  = false;
  gLocClientState.accel_streaming_status  = false;

  gLocClientState.ref_counter = 0;
  memset(&gLocClientState.time_sync_task_helper, 0, sizeof(gLocClientState.time_sync_task_helper));
  memset(&gLocClientState.accel_data_task_helper, 0, sizeof(gLocClientState.accel_data_task_helper));
  memset(&gLocClientState.gyro_data_task_helper, 0, sizeof(gLocClientState.gyro_data_task_helper));

  return true;
}

static int
time_sync_task(void* context)
{
   /* Check that the client hasn't been closed and we are registered */
   if ( gLocClientState.isInitialized && gLocClientState.eventRegMask & QMI_LOC_EVENT_MASK_TIME_SYNC_REQ_V02)
   {
      gLocClientState.ref_counter++;

      LOC_LOGV("%s: Sending Time Sync Req ref_counter = %lu", __FUNCTION__, gLocClientState.ref_counter);

      if ( gLocClientState.eventCallback != NULL)
      {
         qmiLocEventTimeSyncReqIndMsgT_v02 timeSyncInd;
         timeSyncInd.refCounter = gLocClientState.ref_counter;

         locClientEventIndUnionType eventIndUnion;
         eventIndUnion.pTimeSyncReqEvent = &timeSyncInd;

         gLocClientState.eventCallback(gLocClientState.clientHandle,
                                       QMI_LOC_EVENT_TIME_SYNC_REQ_IND_V02,
                                       eventIndUnion);
      }
   }

   /* Send a time sync req every 2 minutes. */
   usleep(1000 * 1000 * 120);

   return 0;
}

static int
accel_status_task(void* context)
{
   /* Check that the client hasn't been closed and we are registered */
   if ( gLocClientState.isInitialized && gLocClientState.eventRegMask & QMI_LOC_EVENT_MASK_SENSOR_STREAMING_READY_STATUS_V02)
   {
      if ( gLocClientState.eventCallback != NULL)
      {
         /* gLocClientState.accel_streaming_status = !gLocClientState.accel_streaming_status; */
         gLocClientState.accel_streaming_status = true;

         qmiLocEventSensorStreamingReadyStatusIndMsgT_v02 sensorStreamInd;
         sensorStreamInd.accelReady_valid = true;
         sensorStreamInd.accelReady.injectEnable = gLocClientState.accel_streaming_status;
         sensorStreamInd.accelReady.dataFrequency.batchesPerSecond = DEFAULT_BATCHING_RATE;
         sensorStreamInd.accelReady.dataFrequency.samplesPerBatch = DEFAULT_SAMPLING_RATE / DEFAULT_BATCHING_RATE;
         sensorStreamInd.gyroReady_valid = false;

         locClientEventIndUnionType eventIndUnion;
         eventIndUnion.pSensorStreamingReadyStatusEvent = &sensorStreamInd;

         gLocClientState.eventCallback(gLocClientState.clientHandle,
                                       QMI_LOC_EVENT_SENSOR_STREAMING_READY_STATUS_IND_V02,
                                       eventIndUnion);
      }
   }

   /* Send a time sync req after sleeping */
   uint32_t seconds = (gLocClientState.accel_streaming_status) ? 150 : 30;
   usleep(1000 * 1000 * seconds);

   return 0;
}

static int
gyro_status_task(void* context)
{
   /* Check that the client hasn't been closed and we are registered */
   if ( gLocClientState.isInitialized && gLocClientState.eventRegMask & QMI_LOC_EVENT_MASK_SENSOR_STREAMING_READY_STATUS_V02)
   {
      if ( gLocClientState.eventCallback != NULL)
      {
         /* gLocClientState.gyro_streaming_status = !gLocClientState.gyro_streaming_status; */
         gLocClientState.gyro_streaming_status = true;

         qmiLocEventSensorStreamingReadyStatusIndMsgT_v02 sensorStreamInd;
         sensorStreamInd.accelReady_valid = false;
         sensorStreamInd.gyroReady_valid = true;
         sensorStreamInd.gyroReady.injectEnable = gLocClientState.gyro_streaming_status;
         sensorStreamInd.gyroReady.dataFrequency.batchesPerSecond = DEFAULT_BATCHING_RATE;
         sensorStreamInd.gyroReady.dataFrequency.samplesPerBatch = DEFAULT_SAMPLING_RATE / DEFAULT_BATCHING_RATE;

         locClientEventIndUnionType eventIndUnion;
         eventIndUnion.pSensorStreamingReadyStatusEvent = &sensorStreamInd;

         gLocClientState.eventCallback(gLocClientState.clientHandle,
                                       QMI_LOC_EVENT_SENSOR_STREAMING_READY_STATUS_IND_V02,
                                       eventIndUnion);
      }
   }

   /* Send a time sync req after sleeping */
   uint32_t seconds = (gLocClientState.gyro_streaming_status) ? 150 : 30;
   usleep(1000 * 1000 * seconds);

   return 0;
}

/* ----------------------- END INTERNAL FUNCTIONS ---------------------------------------- */

/** locClientOpen
  @brief Connects a location client to the location engine. If the connection
         is successful, returns a handle that the location client uses for
         future location operations.

  @param [in] eventRegMask     Mask of asynchronous events the client is
                               interested in receiving
  @param [in] eventIndCb       Function to be invoked to handle an event.
  @param [in] respIndCb        Function to be invoked to handle a response
                               indication.
  @param [out] locClientHandle Handle to be used by the client
                               for any subsequent requests.

  @return
  One of the following error codes:
  - eLOC_CLIENT_SUCCESS  -– If the connection is opened.
  - non-zero error code(see locClientStatusEnumType) -– On failure.
*/

locClientStatusEnumType locClientOpen (
  locClientEventMaskType       eventRegMask,
  locClientEventIndCbType      eventIndCb,
  locClientRespIndCbType       respIndCb,
  locClientHandleType*   pLocClientHandle)
{
  locClientStatusEnumType status = eLOC_CLIENT_SUCCESS;

  LOC_LOGD("locClientOpen: ");

  /*  DeInitialize the client */
  locClientDeInit();

  if( NULL == respIndCb || NULL == pLocClientHandle)
  {
    status = eLOC_CLIENT_FAILURE_INVALID_PARAMETER;
    LOC_LOGD("locClientOpen: invalid parameters ");
    return (status);
  }

  /* Initialize the client */
  locClientInit(eventRegMask, eventIndCb, respIndCb);

  *pLocClientHandle = gLocClientState.clientHandle;

  /* Push back some initial indications to start up the sensor board*/
  if( gLocClientState.eventRegMask & QMI_LOC_EVENT_MASK_SENSOR_STREAMING_READY_STATUS_V02 )
  {
      /* Create task to send periodic sensor streaming requests */
      gpsone_launch_thelper(&gLocClientState.accel_data_task_helper,
                            NULL,           /* Initialize func */
                            NULL,           /* Pre-Process func */
                            accel_status_task, /* Process func */
                            NULL,           /* Post-Process func */
                            NULL);
  }

  if( gLocClientState.eventRegMask & QMI_LOC_EVENT_MASK_SENSOR_STREAMING_READY_STATUS_V02 )
  {
      /* Create task to send periodic sensor streaming requests */
      gpsone_launch_thelper(&gLocClientState.gyro_data_task_helper,
                            NULL,           /* Initialize func */
                            NULL,           /* Pre-Process func */
                            gyro_status_task, /* Process func */
                            NULL,           /* Post-Process func */
                            NULL);
   }

   if ( gLocClientState.eventRegMask & QMI_LOC_EVENT_MASK_TIME_SYNC_REQ_V02 )
   {
      /* Create task to send periodic time sync requests */
      gpsone_launch_thelper(&gLocClientState.time_sync_task_helper,
                            NULL,           /* Initialize func */
                            NULL,           /* Pre-Process func */
                            time_sync_task, /* Process func */
                            NULL,           /* Post-Process func */
                            NULL);
   }

   return(status);
}

void loc_sync_req_init()
{

}

/* Process Loc API indications to wake up blocked user threads */
void loc_sync_process_ind(
      locClientHandleType     client_handle,     /* handle of the client */
      uint32_t                ind_id ,      /* respInd id */
      void                    *ind_payload_ptr /* payload              */
)
{

}

/* Thread safe synchronous request,  using Loc API status return code */
locClientStatusEnumType loc_sync_send_req
(
      locClientHandleType       client_handle,
      uint32_t                  req_id,        /* req id */
      locClientReqUnionType     req_payload,
      uint32_t                  timeout_msec,
      uint32_t                  ind_id,  //ind ID to block for, usually the same as req_id */
      void                      *ind_payload_ptr /* can be NULL*/
)
{
  return eLOC_CLIENT_SUCCESS;
}


/** locClientClose
  @brief Disconnects a client from the location engine.
  @param [in] handle  Handle returned by the locClientOpen()
          function.
  @return
  One of the following error codes:
  - 0 (eLOC_CLIENT_SUCCESS) -– On success.
  - non-zero error code(see locClientStatusEnumType) -– On failure.
*/

locClientStatusEnumType locClientClose(&
  locClientHandleType handle)
{
  LOC_LOGD("locClientClose:" );
  if(handle != gLocClientState.clientHandle )
  {
    return(eLOC_CLIENT_FAILURE_INVALID_HANDLE);
  }
  locClientDeInit();
  return(eLOC_CLIENT_SUCCESS);
}


/** locClientSendMsg
  @brief Sends a message to the location engine. If the locClientSendMsg()
         function is successful, the client should expect an indication
         (except start, stop, event reg and sensor injection messages),
         through the registered callback in the locOpen() function. The
         indication will contain the status of the request and if status is a
         success, indication also contains the payload
         associated with response.
  @param [in] handle Handle returned by the locClientOpen()
              function.
  @param [in] reqId         message ID of the request
  @param [in] reqPayload   Payload of the request, can be NULL
                            if request has no payload


  @return
  One of the following error codes:
  - 0 (eLOC_CLIENT_SUCCESS ) -– On success.
  - non-zero error code (see locClientStatusEnumType) -– On failure.
*/

locClientStatusEnumType locClientSendReq(
  locClientHandleType      handle,
  uint32_t                 reqId,
  locClientReqUnionType    reqPayload )
{
  locClientStatusEnumType status = eLOC_CLIENT_SUCCESS;
  qmiLocGenRespMsgT_v02 resp;
  uint32_t reqLen = 0;
  void *pReqData = NULL;

  /*  check if initialized */
  if(false == gLocClientState.isInitialized)
  {
    return(eLOC_CLIENT_FAILURE_NOT_INITIALIZED);
  }

  /*  check if handle is the same as is stored internally */
  if(handle != gLocClientState.clientHandle)
  {
    return(eLOC_CLIENT_FAILURE_INVALID_PARAMETER);
  }

  /*  validate that the request is correct */
  if (validateRequest(reqId, reqPayload, &pReqData, &reqLen) == false)
  {
    return(eLOC_CLIENT_FAILURE_INVALID_PARAMETER);
  }

  return(status);
}
/*=============================================================================*/

#ifdef __cplusplus
}
#endif
