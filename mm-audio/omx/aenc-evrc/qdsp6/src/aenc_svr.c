/* Copyright (c) 2007 by Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <aenc_svr.h>

/**
 @brief This function processes posted messages

 Once thread is being spawned, this function is run to
 start processing commands posted by client

 @param info pointer to context

 */
void *omx_evrc_msg(void *info)
{
    struct evrc_enc_ipc_info *evrc_info = (struct evrc_enc_ipc_info*)info;
    unsigned char id;
    int n;

    DEBUG_DETAIL("\n%s: message thread start\n", __FUNCTION__);
    while (!evrc_info->dead)
    {
        n = read(evrc_info->pipe_in, &id, 1);
        if (0 == n) break;
        if (1 == n)
        {
          DEBUG_DETAIL("\n%s-->pipe_in=%d pipe_out=%d\n",
                                               evrc_info->thread_name,
                                               evrc_info->pipe_in,
                                               evrc_info->pipe_out);

            evrc_info->process_msg_cb(evrc_info->client_data, id);
        }
        if ((n < 0) && (errno != EINTR)) break;
    }
    DEBUG_DETAIL("%s: message thread stop\n", __FUNCTION__);

    return 0;
}

/**
 @brief This function starts command server

 @param cb pointer to callback function from the client
 @param client_data reference client wants to get back
  through callback
 @return handle to msging thread
 */
struct evrc_enc_ipc_info *omx_evrc_thread_create(
                                    message_func cb,
                                    void* client_data,
                                    char* th_name)
{
    int r;
    int fds[2];
    struct evrc_enc_ipc_info *evrc_info;

    evrc_info = calloc(1, sizeof(struct evrc_enc_ipc_info));
    if (!evrc_info)
    {
        return 0;
    }

    evrc_info->client_data = client_data;
    evrc_info->process_msg_cb = cb;
    strcpy(evrc_info->thread_name,th_name);

    if (pipe(fds))
    {
        DEBUG_PRINT_ERROR("\n%s: pipe creation failed\n", __FUNCTION__);
        goto fail_pipe;
    }

    evrc_info->pipe_in = fds[0];
    evrc_info->pipe_out = fds[1];

    r = pthread_create(&evrc_info->thr, 0, omx_evrc_msg, evrc_info);
    if (r < 0) goto fail_thread;

    DEBUG_DETAIL("Created thread for %s \n", evrc_info->thread_name);
    return evrc_info;


fail_thread:
    close(evrc_info->pipe_in);
    close(evrc_info->pipe_out);

fail_pipe:
    free(evrc_info);

    return 0;
}

void omx_evrc_thread_stop(struct evrc_enc_ipc_info *evrc_info) {
    DEBUG_DETAIL("%s stop server\n", __FUNCTION__);
    close(evrc_info->pipe_in);
    close(evrc_info->pipe_out);
    pthread_join(evrc_info->thr,NULL);
    evrc_info->pipe_out = -1;
    evrc_info->pipe_in = -1;
    DEBUG_DETAIL("%s: message thread close fds%d %d\n", evrc_info->thread_name,
        evrc_info->pipe_in,evrc_info->pipe_out);
    free(evrc_info);
}

void omx_evrc_post_msg(struct evrc_enc_ipc_info *evrc_info, unsigned char id) {
    DEBUG_DETAIL("\n%s id=%d\n", __FUNCTION__,id);
    write(evrc_info->pipe_out, &id, 1);
}
