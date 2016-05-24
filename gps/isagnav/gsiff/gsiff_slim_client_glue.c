/******************************************************************************
  @file:  gsiff_sdp_glue.c
  @brief: GNSS / Sensor Interface Framework Support

  DESCRIPTION
    This file the interface between slim client wrapper on PIP engine
    and GSIFF core

  -----------------------------------------------------------------------------
  Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <grp.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "log_util.h"

#include "gsiff_slim_client_glue.h"
#include "gsiff_api.h"
#include "gpsone_glue_msg.h"
#include "gsiff_msg.h"

#define LOG_TAG "gsiff_slim_client"
#define MAX_BUFFER_SIZE 1024

/* Evaluates the actual length of `sockaddr_un' structure. */
#define SOCKADDR_UN_SIZE(p) ((size_t)(((struct sockaddr_un *) NULL)->sun_path) \
           + strlen ((p)->sun_path))

static int g_client_fd;
static int g_server_fd;

/* msg_q id */
static int g_msg_q_id = -1;

gsiff_baro_request_info g_baro_request; // has current request information
slimBaroDataPayloadStructT* p_baro_data = NULL;


/*=============================================================================================
 * Function description:
 *     This thread starts during GSIFF daemon init. It listens to
 * slim client wrapper on PIP Engine's requests.
 * When slim client wrapper sends baro data request to the server,
 * it forwards the request to sensor providers(Sensor1 or NDK).
 *
 * Parameters:
 *     MSG Q ID
 *
 * Return value:
 *      If successful ( > 0)
 *                0 or less : error

 =============================================================================================*/
 void* gsiff_slim_client_socket_server_thread(void* arg)
 {
     struct sockaddr_un addr_un;
     int socket_fd;
     int result = 0;
     const char * const server_addr = SLIM_MSG_SOCKET_PORT_PATH;

     LOC_LOGV("%s:%d] path %s\n", __func__,__LINE__, server_addr);
     socket_fd = socket (AF_UNIX, SOCK_STREAM, 0);
     g_server_fd = socket_fd;
     if (socket_fd < 0) {
         LOC_LOGE("%s:%d] socket failed\n", __func__,__LINE__);
         return -1;
     }

     unlink(server_addr);
     memset(&addr_un, 0, sizeof(addr_un));
     addr_un.sun_family = AF_UNIX;
     strlcpy(addr_un.sun_path, server_addr, sizeof(addr_un.sun_path));

     result = bind (socket_fd, (struct sockaddr *) &addr_un,
                    SOCKADDR_UN_SIZE(&addr_un));
     if (result != 0) {
         LOC_LOGE("%s:%d] bind failed! result %d: %s\n", __func__,__LINE__, result, strerror(errno));
         close(socket_fd);
         return -1;
     }

     struct group * gps_group = getgrnam("gps");
     if (gps_group != NULL)
     {
         result = chown (server_addr, -1, gps_group->gr_gid);
         result = (result == 0) ? chmod (server_addr, 0770) : result;
         if (result != 0)
         {
             ALOGE("%s: chown/chmod for socket result, gid = %d, result = %d, error = %d:%s\n",
                   __func__, gps_group->gr_gid, result, errno, strerror(errno));
         }
     }
     else
     {
         ALOGE("%s: getgrnam for gps failed, error code = %d\n", __func__,  errno);
     }

     //listen for client request
     result = listen (socket_fd, 5);
     if (result != 0) {
         LOC_LOGE("%s:%d] listen failed result %d: %s\n", __func__,__LINE__, result, strerror(errno));
         close(socket_fd);
         return -1;
     }
     LOC_LOGV("%s:%d] succeeded. Return Val %d\n", __func__, __LINE__, socket_fd);

     //note the msg id sent during init
     int * val = (int*)arg;
     g_msg_q_id  = *val;

     char buf[MAX_BUFFER_SIZE];
     int len = -1;
     memset(buf, 0, sizeof(buf));
     int bytes_read = 0;
     int bytes_remaining = 0;

     while(1)
     {
         g_client_fd = accept(socket_fd, NULL, 0);
         if (g_client_fd == -1)
         {
             LOC_LOGE(" Error accepting client connection\n");
             // return?
             continue;
         }

         while(1)
         {
             slimMsgHeaderStructT msg_header;
             int header_len = sizeof(msg_header);

             LOC_LOGV(" header_len %d\n", header_len);
             len = read(g_client_fd, &msg_header, header_len);
             LOC_LOGV(" Read client result %d ", len);
             if (len > 0)
             {
                 LOC_LOGV("%s:%d] Received data from client len=%d,  msgSize=%d, msgId=%d",
                          __func__,__LINE__, len, msg_header.msgSize, msg_header.msgId);

                 /*read the req payload */
                 int req_payload_len =  msg_header.msgSize - sizeof(slimMsgHeaderStructT);
                 bytes_remaining = req_payload_len;
                 bytes_read = 0;
                 void* p_req_payload = (void*) malloc(bytes_remaining);
                 if(p_req_payload == NULL)
                 {
                     LOC_LOGE("%s: p_req_payload malloc %d failed", __FUNCTION__, bytes_remaining);
                     break;
                 }

                 while(bytes_remaining > 0)
                 {
                     len = read(g_client_fd, (p_req_payload+bytes_read),bytes_remaining);
                     if(len < 0)
                     {
                         LOC_LOGE("Socket read error");
                     }
                     else
                     {
                         bytes_read += len;
                         bytes_remaining -= len;
                     }
                 }

                 if(bytes_read == req_payload_len)
                 {
                     switch(msg_header.msgId)
                     {
                     case SLIM_MSG_TYPE_BARO_DATA_REQ:

                         LOC_LOGD("%s:%d] Received baro data request, msg_id is %d", __func__, __LINE__, msg_header.msgId);

                         slimBaroDataReqPayloadStructT* req_payload =
                             (slimBaroDataReqPayloadStructT*)p_req_payload;
                         slimBaroDataReqStructT baro_req;
                         baro_req.msgHdr =  msg_header;
                         baro_req.msgPayload.enableBaroData = req_payload->enableBaroData;
                         baro_req.msgPayload.samplesPerBatch = req_payload->samplesPerBatch;
                         baro_req.msgPayload.batchesPerSecond = req_payload->batchesPerSecond;

                         //send baro data request to ctrl task
                         gsiff_process_baro_data_request(baro_req);
                         break;
                     case SLIM_MSG_TYPE_TIME_SYNC_REQ:
                         LOC_LOGD("%s:%d] Received time sync request, msg_id is %d", __func__, __LINE__, msg_header.msgId);

                         slimTimeSyncReqPayloadStructT* p_time_sync_payload =
                             (slimTimeSyncReqPayloadStructT*)p_req_payload;
                         //Construct the complete struct with header and req info
                         slimTimeSyncReqStructT time_sync_req;
                         time_sync_req.msgHdr = msg_header;
                         time_sync_req.msgPayload.clientTxTime = p_time_sync_payload->clientTxTime;
                         LOC_LOGV("%s:%d] clientTxTime - %lu", __func__, __LINE__, time_sync_req.msgPayload.clientTxTime );

                         //send time sync request to ctrl task
                         gsiff_send_time_sync_request(time_sync_req);
                         break;
                     default:
                         LOC_LOGV("%s:%d] Received invalid request, msg_id is %d", __func__, __LINE__, msg_header.msgId);
                         break;
                     }
                 }
                 else
                 {
                     LOC_LOGE("Invalid payload");
                 }
                 free(p_req_payload);
             }
             else if(len == 0)
             {
                 LOC_LOGI("%s:%d] Closing client-connection", __func__,__LINE__);
                 close(g_client_fd);
                 g_client_fd = -1;
                 break;
             }
             else
             {
                 LOC_LOGE("%s:%d] Error: empty request client", __func__,__LINE__);
             }
         } /* while(1) loop for reading client-data from client-socket */
     } /* while(1) loop for waiting for new clients, and accepting connections */

     return socket_fd;
 }

/*=============================================================================================
 * Function description:
 *   This function closes the socket connection.
 *
 * Parameters:
 *         None.
 *
 * Return value:
 *    error code: 0: success
 *                non-zero: error
 =============================================================================================*/
 int gsiff_slim_client_connection_close()
{
    LOC_LOGD("%s:%d]\n", __func__, __LINE__);
    close(g_server_fd);
    if(p_baro_data != NULL)
    {
        free(p_baro_data);
        p_baro_data = NULL;
    }
    return 0;
}

/*=============================================================================================
 * Function description:
 *   This function sends baro data to slim client wrapper on PIP Engine.
 *
 * Parameters:
 *         None.
 *
 * Return value:
 *    If successful ( > 0)
 *                0 or less : error
 =============================================================================================*/
int gsiff_send_baro_data(slimBaroDataPayloadStructT* p_baro_data)
{
    LOC_LOGD("%s:%d]\n", __func__, __LINE__);
    slimBaroDataStructT baro_data;
    slimMsgHeaderStructT msgHdr;
    slimBaroDataPayloadStructT *p_baro_payload = NULL;
    int i;

    char buf[MAX_BUFFER_SIZE];
    memset(buf, 0, MAX_BUFFER_SIZE);

    int size = sizeof (slimBaroDataPayloadStructT) - sizeof (slimBaroSampleStructT);
    int total_payload_size = size +  p_baro_data->numberOfSample *  sizeof(slimBaroSampleStructT);
    p_baro_payload = (slimBaroDataPayloadStructT* ) malloc(total_payload_size);

    if(p_baro_payload == NULL)
    {
        LOC_LOGE("%s: p_baro_data malloc failed", __FUNCTION__);
        //send error response to slim client wrapper on PIP
        int status = SLIM_BARO_STATUS_RESP_GENERIC_ERROR;
        gsiff_send_baro_resp(status);
        return -1;
    }

    p_baro_payload->timeOfFirstSample = p_baro_data->timeOfFirstSample;
    p_baro_payload->numberOfSample = p_baro_data->numberOfSample;
    memcpy(p_baro_payload->baroData, p_baro_data->baroData , (p_baro_data->numberOfSample *  sizeof(slimBaroSampleStructT)));

    LOC_LOGV("%s: Baro data before writing - timeof first sample is %d,  number of samples is %d",
             __FUNCTION__, p_baro_payload->timeOfFirstSample, p_baro_payload->numberOfSample);

    for(i=0;i<p_baro_payload->numberOfSample;i++)
    {
        LOC_LOGV("%s  baro value[%d] = %f, timeOffset[%d] = %d",
                 __FUNCTION__, i, p_baro_payload->baroData[i].baroValue,
                 i, p_baro_payload->baroData[i].timeOffset);
    }

    msgHdr.msgSize = total_payload_size + sizeof(slimMsgHeaderStructT);
    LOC_LOGV("%s:%d]\n msgSize is %d", __func__, __LINE__,msgHdr.msgSize);

    msgHdr.msgId = SLIM_MSG_TYPE_INJECT_BARO_DATA;
    msgHdr.clientId = SLIM_CLIENT_TYPE_PIP;
    msgHdr.transactionId = g_baro_request.baro_req.msgHdr.transactionId;

    int hdr_size = sizeof(msgHdr);

    if((hdr_size + total_payload_size) <  MAX_BUFFER_SIZE)
    {
        memcpy(buf, &msgHdr, hdr_size);
        memcpy(buf + hdr_size, p_baro_payload, total_payload_size);
    }
    else
    {
        LOC_LOGV("%s:%d] Buffer size exceeded the max limit", __func__, __LINE__);
        return -1;
    }

    // send data to client socket
    if(g_client_fd > 0)
    {
        LOC_LOGV("%s:%d] Writing baro data to client", __func__, __LINE__);
        write(g_client_fd, buf, hdr_size + total_payload_size);
    }
    else
    {
        LOC_LOGE("%s:%d] Could not write baro data as invalid socket", __func__, __LINE__);
        return -1;
    }

    free(p_baro_payload);
    p_baro_payload = NULL;
    return g_client_fd;
}


/*=============================================================================================
 * Function description:
 *   This function sends time sync data to slim client wrapper on PIP Engine.
 *
 * Parameters:
 *         None.
 *
 * Return value:
 *    If successful ( > 0)
 *                0 or less : error
 =============================================================================================*/
int gsiff_send_time_sync_data(slimTimeSyncDataRespStructT time_sync_resp)
 {
     LOC_LOGD("%s:%d]\n", __func__, __LINE__);

     int size;
     char buf[MAX_BUFFER_SIZE];
     memset(buf, 0, MAX_BUFFER_SIZE);

     size = sizeof(time_sync_resp);
     if(size < MAX_BUFFER_SIZE)
     {
         memcpy(buf, &time_sync_resp, size);
     }
     else
     {
         LOC_LOGV("%s:%d] Buffer size exceeded the max limit", __func__, __LINE__);
         return -1;
     }
     /* send time sync data to client socket */
     if(g_client_fd > 0)
     {
         LOC_LOGV("%s:%d] Writing time sync data to client", __func__, __LINE__);
         write(g_client_fd, buf, size);
     }
     else
     {
         LOC_LOGE("%s:%d] Could not write time sync data as invalid socket", __func__, __LINE__);
         return -1;
     }
     return g_client_fd;
 }

/*=============================================================================================
 * Function description:
 *   This function sends baro response to slim client wrapper on PIP Engine.
 *
 * Parameters:
 *         None.
 *
 * Return value:
 *    If successful ( > 0)
 *                0 or less : error
 =============================================================================================*/
int gsiff_send_baro_resp(int status)
{
    LOC_LOGD("%s:%d]\n", __func__, __LINE__);
    slimBaroStatusStructT baro_resp;
    int size;
    char buf[MAX_BUFFER_SIZE];
    memset(buf, 0, MAX_BUFFER_SIZE);

    baro_resp.msgHdr.msgSize = sizeof(slimBaroStatusStructT);
    baro_resp.msgHdr.msgId = SLIM_MSG_TYPE_BARO_STATUS_RESP;
    baro_resp.msgHdr.clientId = SLIM_CLIENT_TYPE_PIP;
    baro_resp.msgHdr.transactionId = g_baro_request.baro_req.msgHdr.transactionId;

    baro_resp.msgPayload.baroStatusMask = status;

    size = sizeof(baro_resp);
    if(size < MAX_BUFFER_SIZE)
    {
        memcpy(buf, &baro_resp, size);
    }
    else
    {
        LOC_LOGV("%s:%d] Buffer size exceeded the max limit", __func__, __LINE__);
        return -1;
    }

    /* send baro response to client socket */
    if(g_client_fd > 0)
    {
        LOC_LOGV("%s:%d] baro response to client", __func__, __LINE__);
        write(g_client_fd, buf, size);
    }
    else
    {
        LOC_LOGE("%s:%d] Could not write baro response as invalid socket", __func__, __LINE__);
        return -1;
    }
    return g_client_fd;
}

/*=============================================================================================
 * Function description:
 *   This function sends baro request from slim client wrapper on PIP Engine to ctrl task
 *
 * Parameters:
 *         None.
 *
 * Return value:
 *    If successful ( > 0)
 *                0 or less : error
 =============================================================================================*/
int gsiff_send_baro_request(slimBaroDataReqPayloadStructT baro_req)
{
    /* Place message in ctl_task message queue to process */
    gsiff_msg baro_streaming_msg;
    memset(&baro_streaming_msg, 0, sizeof(baro_streaming_msg));
    baro_streaming_msg.msg_type =  GSIFF_BAROMETER_IND;
    baro_streaming_msg.msg_data.bara_data_req = baro_req;

    LOC_LOGI("%s:%d] Send baro request to control task\n", __func__, __LINE__);

    gpsone_glue_msgsnd(g_msg_q_id, &baro_streaming_msg, sizeof(baro_streaming_msg));

    return 1;
}

/*=============================================================================================
 * Function description:
 *   This function sends time sync request from slim client wrapper on PIP Engine to ctrl task.
 *
 * Parameters:
 *         None.
 *
 * Return value:
 *    If successful ( > 0)
 *                0 or less : error
 =============================================================================================*/
int gsiff_send_time_sync_request(slimTimeSyncReqStructT time_sync_req)
{
    /* Place message in ctl_task message queue to process */
    gsiff_msg baro_streaming_msg;
    memset(&baro_streaming_msg, 0, sizeof(baro_streaming_msg));
    baro_streaming_msg.msg_type =  GSIFF_TIME_SYNC_PIP_IND;
    baro_streaming_msg.msg_data.pip_time_sync_req = time_sync_req;

    LOC_LOGV("%s:%d] Send pip time sync request to control task\n", __func__, __LINE__);

    gpsone_glue_msgsnd(g_msg_q_id, &baro_streaming_msg, sizeof(baro_streaming_msg));

    return 1;
}

/*=============================================================================================
 * Function description:
 *   This function processes baro request from slim client wrapper on PIP Engine
 *
 * Parameters:
 *         baro request TLV
 *
 * Return value:
 *    If successful ( > 0)
 *                0 or less : error
 =============================================================================================*/
int gsiff_process_baro_data_request(slimBaroDataReqStructT baro_req)
{
    LOC_LOGI("%s:%d] Request with enable baro data - %d, samples per batch - %d, batches per second ",
             __func__, __LINE__, baro_req.msgPayload.enableBaroData,
             baro_req.msgPayload.samplesPerBatch, baro_req.msgPayload.batchesPerSecond);

    //check whether reporting rate is more than MAX_SENSOR_DATA_SAMPLES allowed
    if( (baro_req.msgPayload.samplesPerBatch * baro_req.msgPayload.batchesPerSecond) > MAX_SENSOR_DATA_SAMPLES)
    {
        LOC_LOGE("%s: Reporting rate exceeds max value allower of 50", __FUNCTION__);
        //send error response to slim client wrapper
        int status = SLIM_BARO_STATUS_RESP_GENERIC_ERROR;
        gsiff_send_baro_resp(status);
        return -1;
    }

    if(g_baro_request.session_status == 0 && baro_req.msgPayload.enableBaroData == 1) // check whether it is a fresh session
    {
        g_baro_request.session_status = 1;
        g_baro_request.baro_req.msgHdr = baro_req.msgHdr;
        g_baro_request.baro_req.msgPayload = baro_req.msgPayload;
        g_baro_request.reporting_rate = baro_req.msgPayload.samplesPerBatch * baro_req.msgPayload.batchesPerSecond;

        if(p_baro_data != NULL)
        {
            free(p_baro_data); // free existing data
            p_baro_data = NULL;
        }
        int size = sizeof (slimBaroDataPayloadStructT) - sizeof (slimBaroSampleStructT);
        p_baro_data = (slimBaroDataPayloadStructT* ) malloc(size +
                                                            (g_baro_request.reporting_rate * sizeof(slimBaroSampleStructT)));

        int total_payload_size = size + (g_baro_request.reporting_rate * sizeof(slimBaroSampleStructT));
        LOC_LOGV("%s: Create space for baro data with size total size %d", __FUNCTION__, total_payload_size);
        if(p_baro_data == NULL)
        {
            LOC_LOGE("%s: p_baro_data malloc failed", __FUNCTION__);
            //send error response to slim client wrapper
            int status = SLIM_BARO_STATUS_RESP_GENERIC_ERROR;
            gsiff_send_baro_resp(status);
            return -1;
        }
        else
        {
            memset(p_baro_data, 0, total_payload_size);
        }

    }
    else if (g_baro_request.session_status == 1 && baro_req.msgPayload.enableBaroData == 0)//stop request
    {
        LOC_LOGI("%s: Stopping the baro data request", __FUNCTION__);
        g_baro_request.session_status = 0;
    }

    gsiff_send_baro_request(baro_req.msgPayload);
    return 1;
}
