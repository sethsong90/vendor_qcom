/* Copyright (c) 2007 by Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <errno.h>

#include <adec_svr.h>


/**
 @brief This function processes posted messages

 Once thread is being spawned, this function is run to
 start processing commands posted by client

 @param info pointer to context

 */
void *omx_Qcelp13_msg(void *info)
{
    struct Qcelp13_ipc_info *Qcelp13_info = (struct Qcelp13_ipc_info*)info;
    unsigned char id;
    int n;

    DEBUG_DETAIL("\n%s: message thread start\n", __FUNCTION__);
    while (!Qcelp13_info->dead)
    {
        n = read(Qcelp13_info->pipe_in, &id, 1);
        if (0 == n) break;
        if (1 == n)
        {
          DEBUG_DETAIL("\n%s-->pipe_in=%d pipe_out=%d\n",
                                               Qcelp13_info->thread_name,
                                               Qcelp13_info->pipe_in,
                                               Qcelp13_info->pipe_out);

            Qcelp13_info->process_msg_cb(Qcelp13_info->client_data, id);
        }
        if ((n < 0) && (errno != EINTR)) break;
    }
    DEBUG_DETAIL("%s: message thread stop\n", __FUNCTION__);

    return 0;
}

void *omx_Qcelp13_events(void *info)
{
    struct Qcelp13_ipc_info *Qcelp13_info = (struct Qcelp13_ipc_info*)info;
    unsigned char id;
    int n;
    DEBUG_DETAIL("%s: message thread start\n", Qcelp13_info->thread_name);
    Qcelp13_info->process_msg_cb(Qcelp13_info->client_data, id);
    DEBUG_DETAIL("%s: message thread stop\n", Qcelp13_info->thread_name);
    return 0;
}
/**
 @brief This function starts command server

 @param cb pointer to callback function from the client
 @param client_data reference client wants to get back
  through callback
 @return handle to msging thread
 */
struct Qcelp13_ipc_info *omx_Qcelp13_thread_create(
                                    message_func cb,
                                    void* client_data,
                                    char* th_name)
{
    int r;
    int fds[2];
    struct Qcelp13_ipc_info *Qcelp13_info;

    Qcelp13_info = calloc(1, sizeof(struct Qcelp13_ipc_info));
    if (!Qcelp13_info)
    {
        return 0;
    }

    Qcelp13_info->client_data = client_data;
    Qcelp13_info->process_msg_cb = cb;
    strcpy(Qcelp13_info->thread_name,th_name);

    if (pipe(fds))
    {
        DEBUG_PRINT_ERROR("\n%s: pipe creation failed\n", __FUNCTION__);
        goto fail_pipe;
    }

    Qcelp13_info->pipe_in = fds[0];
    Qcelp13_info->pipe_out = fds[1];

    r = pthread_create(&Qcelp13_info->thr, 0, omx_Qcelp13_msg, Qcelp13_info);
    if (r < 0) goto fail_thread;

    DEBUG_DETAIL("Created thread for %s \n", Qcelp13_info->thread_name);
    return Qcelp13_info;


fail_thread:
    close(Qcelp13_info->pipe_in);
    close(Qcelp13_info->pipe_out);

fail_pipe:
    free(Qcelp13_info);

    return 0;
}
/**
 *  @brief This function starts command server
 *
 *   @param cb pointer to callback function from the client
 *    @param client_data reference client wants to get back
 *      through callback
 *       @return handle to msging thread
 *        */
struct Qcelp13_ipc_info *omx_Qcelp13_event_thread_create(
                                    message_func cb,
                                    void* client_data,
                                    char* th_name)
{
    int r;
    int fds[2];
    struct Qcelp13_ipc_info *Qcelp13_info;
    Qcelp13_info = calloc(1, sizeof(struct Qcelp13_ipc_info));
    if (!Qcelp13_info)
    {
        return 0;
    }
    Qcelp13_info->client_data = client_data;
    Qcelp13_info->process_msg_cb = cb;
    strcpy(Qcelp13_info->thread_name,th_name);
    if (pipe(fds))
    {
        DEBUG_PRINT("\n%s: pipe creation failed\n", __FUNCTION__);
        goto fail_pipe;
    }
    Qcelp13_info->pipe_in = fds[0];
    Qcelp13_info->pipe_out = fds[1];
    r = pthread_create(&Qcelp13_info->thr, 0, omx_Qcelp13_events, Qcelp13_info);
    if (r < 0) goto fail_thread;
    DEBUG_PRINT("Created thread for %s \n", Qcelp13_info->thread_name);
    return Qcelp13_info;
fail_thread:
    close(Qcelp13_info->pipe_in);
    close(Qcelp13_info->pipe_out);
fail_pipe:
    free(Qcelp13_info);
    return 0;
}

void omx_Qcelp13_thread_stop(struct Qcelp13_ipc_info *Qcelp13_info) {
    DEBUG_DETAIL("%s stop server\n", __FUNCTION__);
    close(Qcelp13_info->pipe_in);
    close(Qcelp13_info->pipe_out);
    pthread_join(Qcelp13_info->thr,NULL);
    Qcelp13_info->pipe_out = -1;
    Qcelp13_info->pipe_in = -1;
    DEBUG_DETAIL("%s: message thread close fds%d %d\n", Qcelp13_info->thread_name,
        Qcelp13_info->pipe_in,Qcelp13_info->pipe_out);
    free(Qcelp13_info);
}

void omx_Qcelp13_post_msg(struct Qcelp13_ipc_info *Qcelp13_info, unsigned char id) {
    DEBUG_DETAIL("\n%s id=%d\n", __FUNCTION__,id);
    write(Qcelp13_info->pipe_out, &id, 1);
}
