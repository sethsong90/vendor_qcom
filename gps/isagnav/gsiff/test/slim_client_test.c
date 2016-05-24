/******************************************************************************
  @file:  slim_client_test.c

  DESCRIPTION
    Unit test Slim client for Barometer data

  INITIALIZATION AND SEQUENCING REQUIREMENTS

  -----------------------------------------------------------------------------
  Copyright (c) 2013 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------

******************************************************************************/

/*=====================================================================
                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why
--------   ---      -------------------------------------------------------
11/06/13   ar       Initial version

======================================================================*/

/*=============================================================================================
 * Function description:
 *   Just basic test client
 *
 * Parameters:
 *         None.
 *
 * Return value:
 *    NA
 =============================================================================================*/
void* test_gsiff_client_thread( void* arg)
{
    int client_socket = -1;
    struct sockaddr_un addr_un;
    slimBaroDataReqStructT baro_req;
    slimTimeSyncReqStructT time_req;
    char buf[MAX_BUFFER_SIZE];

    memset(&addr_un, 0, sizeof(addr_un));
    memset(&baro_req, 0, sizeof(baro_req));
    memset(buf, 0, sizeof(MAX_BUFFER_SIZE));

    const char * const server_addr = SLIM_MSG_SOCKET_PORT_PATH;
    strlcpy(addr_un.sun_path, server_addr, sizeof(addr_un.sun_path));
    addr_un.sun_family = AF_UNIX;
    LOC_LOGV("%s:%d] gsiff test client attempting connect to %s\n", __func__, __LINE__,server_addr);

    client_socket = socket (AF_UNIX, SOCK_STREAM, 0);

    if (client_socket < 0)
    {
        LOC_LOGE("%s:%d] Error in socket() %s", __FUNCTION__, __LINE__, strerror(errno));
        return client_socket;
    }

    if (connect(client_socket, (const struct sockaddr *) &addr_un,
                SOCKADDR_UN_SIZE(&addr_un)) < 0)
    {
        LOC_LOGE("%s:%d] Client socket connect failed: %s\n",__FUNCTION__, __LINE__,
                 strerror(errno));

        if(errno == ENOENT)
        {
            LOC_LOGE("%s:%d] Server socket not yet created %s\n",__FUNCTION__, __LINE__,
                     strerror(errno));
        }
        close(client_socket);
        client_socket = -1;
        return client_socket;
    }

    if(client_socket > 0)
    {
        //send baro request
        //for now populate random values
        baro_req.msgHdr.msgSize = sizeof(baro_req);
        baro_req.msgHdr.msgId = SLIM_MSG_TYPE_BARO_DATA_REQ;
        baro_req.msgHdr.clientId = SLIM_CLIENT_TYPE_PIP;
        baro_req.msgHdr.transactionId = 1;

        //populate with default baro values
        baro_req.msgPayload.enableBaroData = 1;
        baro_req.msgPayload.samplesPerBatch = 5;
        baro_req.msgPayload.batchesPerSecond = 2;

        int len = sizeof(baro_req);
        //memcpy(buf, &baro_req, len);
        LOC_LOGV("%s:%d] sending baro request of size : %d",__FUNCTION__, __LINE__,len);
        LOC_LOGV("%s:%d] sizeof(slimBaroDataReqStructT) is %d",__FUNCTION__, __LINE__,sizeof(slimBaroDataReqStructT));

        if ( write(client_socket, &baro_req, len) < 0)
        {
            LOC_LOGE("%s:%d] Error sending baro request %s\n", __FUNCTION__, __LINE__, strerror(errno));
            close (client_socket);
            client_socket = -1;
            return client_socket;
        }
        else
        {
            LOC_LOGV("%s:%d] Baro request sent successful",__FUNCTION__, __LINE__);
        }

        //trigger time request - random act
        time_req.msgHdr.msgSize = sizeof(time_req);
        time_req.msgHdr.msgId = SLIM_MSG_TYPE_TIME_SYNC_REQ;
        time_req.msgHdr.clientId = 1;
        time_req.msgHdr.transactionId = 1;

        //populate with default time values
        time_req.msgPayload.clientTxTime = 500;

        len = sizeof(time_req);
        memset(buf, 0, sizeof(MAX_BUFFER_SIZE));
        //memcpy(buf, &time_req, len);
        LOC_LOGV("%s:%d] sending time request of size : %d",__FUNCTION__, __LINE__,len);
        LOC_LOGV("%s:%d] sizeof(slimTimeSyncReqStructT) is %d",__FUNCTION__, __LINE__,sizeof(slimTimeSyncReqStructT));
        if ( write(client_socket, &time_req, len) < 0)
        {
            LOC_LOGE("%s:%d] Error sending time request %s\n", __FUNCTION__, __LINE__, strerror(errno));
            close (client_socket);
            client_socket = -1;
            return client_socket;
        }
        else
        {
            LOC_LOGV("%s:%d] Time request sent successful",__FUNCTION__, __LINE__);
        }
    }

    char recv_buf[MAX_BUFFER_SIZE];
    int size = -1;
    int i=0;
    int count = 0;
    int count1 = 0;
    memset(recv_buf, 0, MAX_BUFFER_SIZE);

    while(1)
    {
        /* Only read sizeof(buf)- 1 characters. This will ensure that the buf
           is NULL terminated since we just memset all bytes to 0 */
        size = read(client_socket, recv_buf, sizeof(recv_buf)-1);
        LOC_LOGV(" Read server result %d:\n", size);
        if (size > 0)
        {
            LOC_LOGV("%s:%d] Received data from server size = %d",__func__,__LINE__, size);

            /* first read the message header */
            slimMsgHeaderStructT msg_header;
            int header_len = sizeof(slimMsgHeaderStructT);
            memcpy(&msg_header, recv_buf, header_len);

            LOC_LOGV("%s:%d] msgSize - %d", __func__, __LINE__, msg_header.msgSize );
            LOC_LOGV("%s:%d] msgId - %d", __func__, __LINE__, msg_header.msgId );
            LOC_LOGV("%s:%d] clientId - %d", __func__, __LINE__, msg_header.clientId );
            LOC_LOGV("%s:%d] transactionId- %d", __func__, __LINE__, msg_header.transactionId);

            switch(msg_header.msgId)
            {
            case SLIM_MSG_TYPE_INJECT_BARO_DATA:
                LOC_LOGV("%s:%d] Received baro data, msg_id is %d", __func__, __LINE__, msg_header.msgId);
                /*read the req payload */
                count++;
                slimBaroDataPayloadStructT *payload;
                int payload_len = msg_header.msgSize - header_len;
                payload = (slimBaroDataPayloadStructT *) malloc(payload_len);
                if(payload != NULL)
                {
                    memcpy(payload, recv_buf + header_len, payload_len);

                    LOC_LOGV("%s:%d] payload_len  is %d", __func__, __LINE__, payload_len);
                    LOC_LOGV("%s  timeof first sample is %d", __FUNCTION__,payload->timeOfFirstSample);
                    LOC_LOGV("%s  number of samples is %d", __FUNCTION__,payload->numberOfSample);

                    for(i=0;i<payload->numberOfSample;i++)
                    {
                        LOC_LOGV("%s  baro value[%d] = %f", __FUNCTION__,i,payload->baroData[i].baroValue);
                        LOC_LOGV("%s  timeOffset[%d] = %d", __FUNCTION__,i,payload->baroData[i].timeOffset);
                    }
                }
                else
                {
                    LOC_LOGE("%s:%d] payload malloc failure %d", __func__, __LINE__);
                }

                if(count >= 10)
                {
                    //send stop request
                    baro_req.msgHdr.msgSize = sizeof(baro_req);
                    baro_req.msgHdr.msgId = SLIM_MSG_TYPE_BARO_DATA_REQ;
                    baro_req.msgHdr.clientId = SLIM_CLIENT_TYPE_PIP;
                    baro_req.msgHdr.transactionId = 1;

                    //populate with default baro values
                    baro_req.msgPayload.enableBaroData = 0;
                    baro_req.msgPayload.samplesPerBatch = 0;
                    baro_req.msgPayload.batchesPerSecond = 0;

                    int len = sizeof(baro_req);
                    //memcpy(buf, &baro_req, len);

                    if ( write(client_socket, &baro_req, len) < 0)
                    {
                        LOC_LOGE("%s:%d] Error sending baro request %s\n", __FUNCTION__, __LINE__, strerror(errno));
                        close (client_socket);
                        client_socket = -1;
                        return client_socket;
                    }
                    else
                    {
                        LOC_LOGV("%s:%d] Baro stop request sent successful",__FUNCTION__, __LINE__);
                    }


                    //send start again
                    baro_req.msgHdr.msgSize = sizeof(baro_req);
                    baro_req.msgHdr.msgId = SLIM_MSG_TYPE_BARO_DATA_REQ;
                    baro_req.msgHdr.clientId = SLIM_CLIENT_TYPE_PIP;
                    baro_req.msgHdr.transactionId = 1;

                    //populate with default baro values
                    baro_req.msgPayload.enableBaroData = 1;
                    baro_req.msgPayload.samplesPerBatch = 5;
                    baro_req.msgPayload.batchesPerSecond = 4;

                    len = sizeof(baro_req);
                    if ( write(client_socket, &baro_req, len) < 0)
                    {
                        LOC_LOGE("%s:%d] Error sending baro request %s\n", __FUNCTION__, __LINE__, strerror(errno));
                        close (client_socket);
                        client_socket = -1;
                        return client_socket;
                    }
                    else
                    {
                        LOC_LOGV("%s:%d] Baro start request sent successful",__FUNCTION__, __LINE__);
                    }
                    count = 0;
                }
                break;
            case SLIM_MSG_TYPE_INJECT_TIME_SYNC_DATA:
                LOC_LOGV("%s:%d] Received time sync data, msg_id is %d", __func__, __LINE__, msg_header.msgId);

                slimTimeSyncDataRespPayloadStructT time_sync_data;
                int data_len = sizeof(slimTimeSyncDataRespPayloadStructT);
                memcpy(&time_sync_data, recv_buf+header_len, data_len);

                LOC_LOGV("%s  clientTxTime is %lu", __FUNCTION__,time_sync_data.clientTxTime);
                LOC_LOGV("%s  sensorProcRxTime is %lu", __FUNCTION__,time_sync_data.sensorProcRxTime);
                LOC_LOGV("%s  sensorProcTxTime is %lu", __FUNCTION__,time_sync_data.sensorProcTxTime);
                break;

            case SLIM_MSG_TYPE_BARO_STATUS_RESP:
                LOC_LOGV("%s:%d] Received baro response, msg_id is %d", __func__, __LINE__, msg_header.msgId);

                slimBaroStatusPayloadStructT resp_data;
                int resp_len = sizeof(slimBaroStatusPayloadStructT);
                memcpy(&resp_data, recv_buf + header_len, resp_len);

                LOC_LOGV("%s  baroStatusMask is %d", __FUNCTION__,resp_data.baroStatusMask);
                break;
            default:
                LOC_LOGV("%s:%d] Received invalid request, msg_id is %d", __func__, __LINE__, msg_header.msgId);
                break;
            }
        }
        else
            LOC_LOGE("%s:%d] Error: nothing from server", __func__,__LINE__);
    }
    return client_socket;
}
