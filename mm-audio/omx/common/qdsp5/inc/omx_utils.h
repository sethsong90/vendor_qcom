#ifndef _OMX_UTILS_H_
#define _OMX_UTILS_H_

/*=========================================================================
                            O p e n M A X   Component
                                Audio Decoder
*//** @file omx_utils.h
This module contains the class definition for openMAX decoder component.

Copyright (c) 2006-2008 Qualcomm Technologies, Inc.
All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
*//*=====================================================================*/
/*=========================================================================
                            Edit History

$Header:
when       who     what, where, why
--------   ---     -------------------------------------------------------
=========================================================================*/
#ifdef __cplusplus
extern "C" {
#endif

#include "qc_omx_msg.h"

#include <pthread.h>
#include <sched.h>

    typedef void (*message_func)(void* client_data, unsigned char id);

    struct ipc_info
    {
        pthread_t    thr;
        int          pipe_in;
        int          pipe_out;
        int          dead;
        message_func process_msg_cb;
        void         *client_data;
        char         thread_name[128];
    };

    struct ipc_info *omx_thread_create(message_func cb,
    void* client_data,
    char *th_name);

    struct ipc_info *omx_event_thread_create(message_func cb,
    void*        client_data,
    char*        th_name);
    void *omx_msg(void *info);

    void *omx_events(void *info);

    void omx_thread_stop(struct ipc_info *ipc);

    void omx_post_msg(struct ipc_info *ipc, unsigned char id);

#ifdef __cplusplus
}
#endif

#endif // _OMX_UTILS_H_
