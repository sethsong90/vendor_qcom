#ifndef ADEC_SVR_H
#define ADEC_SVR_H

/* Copyright (c) 2008 by Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
#ifdef __cplusplus
extern "C" {
#endif
#include <pthread.h>
#include <sched.h>

#ifdef _ANDROID_
#define LOG_TAG "QC_QCELPDEC"
#endif
#include "qc_omx_msg.h"

typedef void (*message_func)(void* client_data, unsigned char id);

/**
 @brief audio decoder ipc info structure

 */
struct Qcelp13_ipc_info
{
    pthread_t    thr;
    int          pipe_in;
    int          pipe_out;
    int          dead;
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
struct Qcelp13_ipc_info *omx_Qcelp13_thread_create(message_func cb,
    void* client_data,
    char *th_name);

struct Qcelp13_ipc_info *omx_Qcelp13_event_thread_create(message_func cb,
    void* client_data,
    char *th_name);
/**
 @brief This function stop command server

 @param svr handle to command server
 @return none
 */
void omx_Qcelp13_thread_stop(struct Qcelp13_ipc_info *Qcelp13_ipc);


/**
 @brief This function post message in the command server

 @param svr handle to command server
 @return none
 */
void omx_Qcelp13_post_msg(struct Qcelp13_ipc_info *Qcelp13_ipc,
                          unsigned char id);

void* omx_Qcelp13_comp_timer_handler(void *);

#ifdef __cplusplus
}
#endif

#endif /* ADEC_SVR */
