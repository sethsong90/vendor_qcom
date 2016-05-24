/* Copyright (c) 2009,2011 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef AENC_SVR_H
#define AENC_SVR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <sched.h>

#ifdef _ANDROID_
#define LOG_TAG "QC_QCELPENC"
#endif
#include "qc_omx_msg.h"

typedef void (*message_func)(void* client_data, unsigned char id);

/**
 @brief audio encoder ipc info structure

 */
struct qcelp13_enc_ipc_info
{
    pthread_t thr;
    int pipe_in;
    int pipe_out;
    int dead;
    message_func process_msg_cb;
    void         *client_data;
    char         thread_name[128];
};

/**
 @brief This function starts command server

 @param cb pointer to callback function from the client
 @param client_data reference client wants to get back
  through callback
 @return handle to command server
 */
struct qcelp13_enc_ipc_info *omx_qcelp13_thread_create(message_func cb,
    void* client_data,
    char *th_name);


/**
 @brief This function stop command server

 @param svr handle to command server
 @return none
 */
void omx_qcelp13_thread_stop(struct qcelp13_enc_ipc_info *qcelp13_ipc);


/**
 @brief This function post message in the command server

 @param svr handle to command server
 @return none
 */
void omx_qcelp13_post_msg(struct qcelp13_enc_ipc_info *qcelp13_ipc,
                          unsigned char id);

#ifdef __cplusplus
}
#endif

#endif /* AENC_SVR */
