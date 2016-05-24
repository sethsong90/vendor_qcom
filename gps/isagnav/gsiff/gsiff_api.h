/******************************************************************************
  @file:  gsiff_api.h
  @brief:

  DESCRIPTION
    This file contains GSIFF API for external clients to connect and
    request sensor data.

 -----------------------------------------------------------------------------
  Copyright (c) 2013-2014 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------

******************************************************************************/

#ifndef GSIFF_API_H
#define GSIFF_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#define SLIM_MSG_SOCKET_PORT_NUM   28602
#ifdef DEBUG_X86
#define SLIM_MSG_SOCKET_PORT_PATH  "/tmp/gsiff_quipc_unix_socket"
#else
#define SLIM_MSG_SOCKET_PORT_PATH  "/data/misc/location/quipc/gsiff_socket"
#endif

/* Starts with 1000 */
#define SLIM_MSG_TYPE_BARO_DATA_REQ         1000
#define SLIM_MSG_TYPE_BARO_STATUS_RESP      1001
#define SLIM_MSG_TYPE_INJECT_BARO_DATA      1002
#define SLIM_MSG_TYPE_TIME_SYNC_REQ         1003
#define SLIM_MSG_TYPE_INJECT_TIME_SYNC_DATA 1004

/* Starts with 1 */
#define SLIM_CLIENT_TYPE_PIP              1

#define MAX_SENSOR_DATA_SAMPLES 50

/**
 * The message header. Used in both incoming and outgoing messages
 */
typedef struct
{
  uint32_t msgSize;
  /**<   Total size for this message (in bytes) */
  uint32_t msgId;
  /**<   Unique message ID for each message type */
  uint32_t clientId;
  /**<   Unique client ID for each client */
  uint32_t transactionId;
  /**<   Unique transaction ID for each transaction for a specific client */
} slimMsgHeaderStructT;

/**
 * Data-structure for Barometer Sample
 */
typedef struct
{
  uint16_t timeOffset;
  /**<   Sample time offset. This time offset must be relative to the
         sensor time of the first sample.
       - Type: Unsigned integer
       - Units: Milliseconds  */

  float baroValue;
  /**<   Barometer sample value.
       - Type: Floating point \n
       - Units: hectopascal (hPa) */

} slimBaroSampleStructT;

/**
 * Data-structure for Barometer Sensor-Data message
 */
typedef struct
{
    uint64_t timeOfFirstSample;
    /**<   Denotes a full 64-bit time tag of the first (oldest) sample in
       this message.
       - Type: Unsigned integer
       - Units: Milliseconds  */

    uint8_t numberOfSample;
    /**< Must be set to # of elements in baroData */

     slimBaroSampleStructT baroData[1];
    /**<   Variable length array to specify sensor samples. */

} slimBaroDataPayloadStructT;

/**
 * Data-structure for Barometer Sensor-Data message
 */
typedef struct
{
  slimMsgHeaderStructT msgHdr;
  /**<   Message header */
  slimBaroDataPayloadStructT msgPayload;
  /**<   Variable length array to specify sensor samples. */

} slimBaroDataStructT;

/**
 * Defines the enums for baroStatusResponse message
 */
typedef enum
{
  SLIM_BARO_STATUS_RESP_OK = 0,
  SLIM_BARO_STATUS_RESP_GENERIC_ERROR = 1,
  SLIM_BARO_STATUS_RESP_MAX = 0xFFFF
  /**<   Forced the enum type to be 16-bit. */
} slimBaroStatusEnumT;

/**
 * Data-structure for Barometer Status Response message
 */
typedef struct
{
  slimBaroStatusEnumT baroStatusMask;
  /**<   Barometer status response bitmask */

} slimBaroStatusPayloadStructT;

/**
 * Data-structure for Barometer Status Response message
 */
typedef struct
{
  slimMsgHeaderStructT msgHdr;
  /**<   Message header */

  slimBaroStatusPayloadStructT msgPayload;
  /**<   Barometer status response bitmask */

} slimBaroStatusStructT;


/**
 * Data-structure for Barometer Data Request message
 */
typedef struct
{
  uint8_t enableBaroData;
  /**<   Whether the Location Engine is ready to accept barometer data
       Valid values:
       - 0x01 (TRUE) : Location Engine is ready to accept baro data
       - 0x00 (FALSE): Location Engine is not ready to accept baro data
   */

  uint8_t samplesPerBatch;
  /**<   Specifies the number of samples per batch the Location Engine is to
       receive. The sensor sampling frequency can be computed as follows:
       samplingFrequency = samplesPerBatch * batchesPerSecond
       samplesPerBatch must be a non-zero positive value.
       Note: This samplesPerBatch is irrelevant when enableBaroData=FALSE
   */

  uint8_t batchesPerSecond;
  /**<   Number of sensor-data batches the Location Engine is to receive
       per second. The rate is specified in integral number of batches per
       second (Hz).
       batchesPerSecond must be a non-zero positive value.
       Note: This batchesPerSecond is irrelevant when enableBaroData=FALSE
   */
} slimBaroDataReqPayloadStructT;

/**
 * Data-structure for Barometer Data Request message
 */
typedef struct
{
  slimMsgHeaderStructT msgHdr;
  /**<   Message header */
  slimBaroDataReqPayloadStructT msgPayload;
  /**<   Number of sensor-data batches the Location Engine is to receive
       per second. The rate is specified in integral number of batches per
  */
} slimBaroDataReqStructT;

/**
 * Data-structure for Time-Sync Request message
 */
typedef struct
{
  uint64_t clientTxTime;
  /**<   Timestamp in client clock of the time-sync request message
       transmit time.
   */
} slimTimeSyncReqPayloadStructT;

/**
 * Data-structure for Barometer Data Request message
 */
typedef struct
{
  slimMsgHeaderStructT msgHdr;
  /**<   Message header */
  slimTimeSyncReqPayloadStructT msgPayload;
  /**<   Data for time-sync request message
  */
} slimTimeSyncReqStructT;


/**
 * Data-structure for Time-Sync Data message
 */
typedef struct
{
  uint64_t clientTxTime;
  /**<   Transmit timestamp in client clock of the time-sync
       request message.
   */

  uint64_t sensorProcRxTime;
  /**<   Receive timestamp in Sensor-Processor clock of the
       time-sync request message.
   */

  uint64_t sensorProcTxTime;
  /**<   Transmit timestamp in Sensor-Processor clock of the
       time-sync data response message.
   */
} slimTimeSyncDataRespPayloadStructT;

/**
 * Data-structure for Barometer Data Request message
 */
typedef struct
{
  slimMsgHeaderStructT msgHdr;
  /**<   Message header */
  slimTimeSyncDataRespPayloadStructT msgPayload;
  /**<   Time-sync data response message payload
  */
} slimTimeSyncDataRespStructT;


#ifdef __cplusplus
}
#endif


#endif /* GSIFF_API_H */
