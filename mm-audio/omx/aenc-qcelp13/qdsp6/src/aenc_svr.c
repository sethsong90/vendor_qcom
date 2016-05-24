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
void *omx_qcelp13_msg(void *info)
{
    struct qcelp13_enc_ipc_info *qcelp13_info = (struct qcelp13_enc_ipc_info*)info;
    unsigned char id;
    int n;

    DEBUG_DETAIL("\n%s: message thread start\n", __FUNCTION__);
    while (!qcelp13_info->dead)
    {
        n = read(qcelp13_info->pipe_in, &id, 1);
        if (0 == n) break;
        if (1 == n)
        {
          DEBUG_DETAIL("\n%s-->pipe_in=%d pipe_out=%d\n",
                                               qcelp13_info->thread_name,
                                               qcelp13_info->pipe_in,
                                               qcelp13_info->pipe_out);

            qcelp13_info->process_msg_cb(qcelp13_info->client_data, id);
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
struct qcelp13_enc_ipc_info *omx_qcelp13_thread_create(
                                    message_func cb,
                                    void* client_data,
                                    char* th_name)
{
    int r;
    int fds[2];
    struct qcelp13_enc_ipc_info *qcelp13_info;

    qcelp13_info = calloc(1, sizeof(struct qcelp13_enc_ipc_info));
    if (!qcelp13_info)
    {
        return 0;
    }

    qcelp13_info->client_data = client_data;
    qcelp13_info->process_msg_cb = cb;
    strcpy(qcelp13_info->thread_name,th_name);

    if (pipe(fds))
    {
        DEBUG_PRINT_ERROR("\n%s: pipe creation failed\n", __FUNCTION__);
        goto fail_pipe;
    }

    qcelp13_info->pipe_in = fds[0];
    qcelp13_info->pipe_out = fds[1];

    r = pthread_create(&qcelp13_info->thr, 0, omx_qcelp13_msg, qcelp13_info);
    if (r < 0) goto fail_thread;

    DEBUG_DETAIL("Created thread for %s \n", qcelp13_info->thread_name);
    return qcelp13_info;


fail_thread:
    close(qcelp13_info->pipe_in);
    close(qcelp13_info->pipe_out);

fail_pipe:
    free(qcelp13_info);

    return 0;
}

void omx_qcelp13_thread_stop(struct qcelp13_enc_ipc_info *qcelp13_info) {
    DEBUG_DETAIL("%s stop server\n", __FUNCTION__);
    close(qcelp13_info->pipe_in);
    close(qcelp13_info->pipe_out);
    pthread_join(qcelp13_info->thr,NULL);
    qcelp13_info->pipe_out = -1;
    qcelp13_info->pipe_in = -1;
    DEBUG_DETAIL("%s: message thread close fds%d %d\n", qcelp13_info->thread_name,
        qcelp13_info->pipe_in,qcelp13_info->pipe_out);
    free(qcelp13_info);
}

void omx_qcelp13_post_msg(struct qcelp13_enc_ipc_info *qcelp13_info, unsigned char id) {
    DEBUG_DETAIL("\n%s id=%d\n", __FUNCTION__,id);
    write(qcelp13_info->pipe_out, &id, 1);
}
