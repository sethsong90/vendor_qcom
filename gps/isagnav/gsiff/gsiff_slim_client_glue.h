/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

  This file the interface between Sensor Data Processor(SDP) on PIP engine
  and GSIFF core

 -----------------------------------------------------------------------------
  Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------

=============================================================================*/


#include "gsiff_msg.h"

typedef struct gsiff_baro_request_info {
    int session_status;  // 1: running 0: not running
    slimBaroDataReqStructT baro_req;
    int reporting_rate;
} gsiff_baro_request_info;

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
void* gsiff_slim_client_socket_server_thread(void* arg);

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
int gsiff_slim_client_connection_close();

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
int gsiff_send_baro_data(slimBaroDataPayloadStructT* p_baro_data);

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
int gsiff_send_time_sync_data(slimTimeSyncDataRespStructT time_sync_resp);

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
int gsiff_send_baro_resp(int status);

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
int gsiff_send_baro_request(slimBaroDataReqPayloadStructT baro_req);

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
int gsiff_send_time_sync_request(slimTimeSyncReqStructT time_sync_req);

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
int gsiff_process_baro_data_request(slimBaroDataReqStructT baro_req);

void* test_gsiff_client_thread( void* arg);

