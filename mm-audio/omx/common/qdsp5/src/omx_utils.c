/*=========================================================================
                            O p e n M A X   Component
                                Audio Decoder
*//** @file omx_utils.c
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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "omx_utils.h"

/**
@brief This function processes posted messages

Once thread is being spawned, this function is run to
start processing commands posted by client

@param info pointer to context

*/
void *omx_msg(void *info)
{
    struct ipc_info *ipc_info = (struct ipc_info*)info;
    unsigned char id;
    int n;

    DEBUG_DETAIL("\n%s: message thread start\n", __FUNCTION__);
    while (!ipc_info->dead) {
        n = read(ipc_info->pipe_in, &id, 1);
        if (n == 0) break;
        if (n == 1) {
            DEBUG_DETAIL("\n%s-->pipe_in=%d pipe_out=%d\n",
            ipc_info->thread_name,
            ipc_info->pipe_in,
            ipc_info->pipe_out);

            ipc_info->process_msg_cb(ipc_info->client_data, id);
        }
        if ((n < 0) && (errno != EINTR)) break;
    }
    DEBUG_PRINT("%s: message thread stop thread[%s]", __FUNCTION__,
                                             ipc_info->thread_name);

    return 0;
}


/**
@brief This function starts command server

@param cb pointer to callback function from the client
@param client_data reference client wants to get back
through callback
@return handle to msging thread
*/
struct ipc_info *omx_thread_create(
message_func cb,
void* client_data,
char* th_name)
{
    int r;
    int fds[2];
    struct ipc_info *info;

    info = calloc(1, sizeof(struct ipc_info));
    if (!info) return 0;

    info->client_data = client_data;
    info->process_msg_cb = cb;
    strlcpy(info->thread_name,th_name,sizeof(info->thread_name));

    if (pipe(fds)) {
        DEBUG_PRINT_ERROR("\n%s: pipe creation failed\n", __FUNCTION__);
        goto fail_pipe;
    }

    info->pipe_in = fds[0];
    info->pipe_out = fds[1];


    r = pthread_create(&info->thr, 0, omx_msg, info);
    if (r < 0) goto fail_thread;

    DEBUG_DETAIL("Created thread for %s %p\n", info->thread_name,info);
    return info;


    fail_thread:
    close(info->pipe_in);
    close(info->pipe_out);

    fail_pipe:
    free(info);

    return 0;
}

/**
@brief This function starts the event thread

@param cb pointer to callback function from the client
@param client_data reference client wants to get back
through callback
@return handle to msging thread
*/
struct ipc_info *omx_event_thread_create(
message_func cb,
void* client_data,
char* th_name)
{
    int r;
    int fds[2];
    struct ipc_info *info;

    info = calloc(1, sizeof(struct ipc_info));
    if ( !info ) return 0;

    info->client_data = client_data;
    info->process_msg_cb = cb;
    strlcpy(info->thread_name,th_name,sizeof(info->thread_name));

    if ( pipe(fds) )
    {
        DEBUG_PRINT_ERROR("\n%s: pipe creation failed\n", __FUNCTION__);
        goto fail_pipe;
    }

    info->pipe_in = fds[0];
    info->pipe_out = fds[1];

    r = pthread_create(&info->thr, 0, omx_events, info);
    if ( r < 0 ) goto fail_thread;

    DEBUG_DETAIL("Created thread for %s %p \n", info->thread_name,info);
    return info;

    fail_thread:
    close(info->pipe_in);
    close(info->pipe_out);

    fail_pipe:
    free(info);

    return 0;
}

void *omx_events(void *info)
{
    struct ipc_info *ipc_info = (struct ipc_info*)info;
    unsigned char id=0;
    int n;

    DEBUG_DETAIL("%s: message thread start\n", ipc_info->thread_name);
    ipc_info->process_msg_cb(ipc_info->client_data, id);
    DEBUG_DETAIL("%s: message thread stop\n", ipc_info->thread_name);
    return 0;
}

void omx_thread_stop(struct ipc_info *info) {
    DEBUG_DETAIL("%s stop server thread[%s]\n", __FUNCTION__,info->thread_name);
    close(info->pipe_in);
    close(info->pipe_out);
    pthread_join(info->thr,NULL);
    info->pipe_in=-1;
    info->pipe_out=-1;
    free(info);
}

void omx_post_msg(struct ipc_info *info, unsigned char id) {
    DEBUG_DETAIL("\n%s id=%d th_name[%s]\n", __FUNCTION__,id,info->thread_name);
    write(info->pipe_out, &id, 1);
}


