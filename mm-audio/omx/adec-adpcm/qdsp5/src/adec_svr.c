/* Copyright (c) 2009-2010 Qualcomm Technologies, Inc.  All Rights Reserved.
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
void *omx_adpcm_msg(void *info)
{
    struct adpcm_ipc_info *adpcm_info = (struct adpcm_ipc_info*)info;
    unsigned char id;
    int n;

    DEBUG_PRINT("\n%s: message thread start\n", __FUNCTION__);
    while (!adpcm_info->dead)
    {
        n = read(adpcm_info->pipe_in, &id, 1);
        if (0 == n) break;
        if (1 == n)
        {
            DEBUG_DETAIL("\n%s-->pipe_in=%d pipe_out=%d\n",
                adpcm_info->thread_name,
                adpcm_info->pipe_in,
                adpcm_info->pipe_out);

            adpcm_info->process_msg_cb(adpcm_info->client_data, id);
        }
        if ((n < 0) && (errno != EINTR)) break;
    }
    DEBUG_PRINT("%s: message thread stop\n", __FUNCTION__);

    return 0;
}

void *omx_adpcm_events(void *info)
{
    struct adpcm_ipc_info *adpcm_info = (struct adpcm_ipc_info*)info;
    unsigned char id = 0;
    DEBUG_DETAIL("%s: message thread start\n", adpcm_info->thread_name);
    adpcm_info->process_msg_cb(adpcm_info->client_data, id);
    DEBUG_DETAIL("%s: message thread stop\n", adpcm_info->thread_name);
    return 0;
}
/**
 @brief This function starts command server

 @param cb pointer to callback function from the client
 @param client_data reference client wants to get back
  through callback
 @return handle to msging thread
 */
struct adpcm_ipc_info *omx_adpcm_thread_create(
                                    message_func cb,
                                    void* client_data,
                                    char* th_name)
{
    int r;
    int fds[2];
    struct adpcm_ipc_info *adpcm_info;

    adpcm_info = calloc(1, sizeof(struct adpcm_ipc_info));
    if (!adpcm_info)
    {
        return 0;
    }

    adpcm_info->client_data = client_data;
    adpcm_info->process_msg_cb = cb;
    strlcpy(adpcm_info->thread_name,th_name,sizeof(adpcm_info->thread_name));

    if (pipe(fds))
    {
        DEBUG_PRINT("\n%s: pipe creation failed\n", __FUNCTION__);
        goto fail_pipe;
    }

    adpcm_info->pipe_in = fds[0];
    adpcm_info->pipe_out = fds[1];

    r = pthread_create(&adpcm_info->thr, 0, omx_adpcm_msg, adpcm_info);
    if (r < 0) goto fail_thread;

    DEBUG_PRINT("Created thread for %s \n", adpcm_info->thread_name);
    return adpcm_info;


fail_thread:
    close(adpcm_info->pipe_in);
    close(adpcm_info->pipe_out);

fail_pipe:
    free(adpcm_info);

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
struct adpcm_ipc_info *omx_adpcm_event_thread_create(
                                    message_func cb,
                                    void* client_data,
                                    char* th_name)
{
    int r;
    int fds[2];
    struct adpcm_ipc_info *adpcm_info;
    adpcm_info = calloc(1, sizeof(struct adpcm_ipc_info));
    if (!adpcm_info)
    {
        return 0;
    }
    adpcm_info->client_data = client_data;
    adpcm_info->process_msg_cb = cb;
    strlcpy(adpcm_info->thread_name,th_name,sizeof(adpcm_info->thread_name));
    if (pipe(fds))
    {
        DEBUG_PRINT("\n%s: pipe creation failed\n", __FUNCTION__);
        goto fail_pipe;
    }
    adpcm_info->pipe_in = fds[0];
    adpcm_info->pipe_out = fds[1];
    r = pthread_create(&adpcm_info->thr, 0, omx_adpcm_events, adpcm_info);
    if (r < 0) goto fail_thread;
    DEBUG_PRINT("Created thread for %s \n", adpcm_info->thread_name);
    return adpcm_info;
fail_thread:
    close(adpcm_info->pipe_in);
    close(adpcm_info->pipe_out);
fail_pipe:
    free(adpcm_info);
    return 0;
}

void omx_adpcm_thread_stop(struct adpcm_ipc_info *adpcm_info) {
    DEBUG_PRINT("%s stop server\n", __FUNCTION__);
    close(adpcm_info->pipe_in);
    close(adpcm_info->pipe_out);
    pthread_join(adpcm_info->thr,NULL);
    adpcm_info->pipe_out = -1;
    adpcm_info->pipe_in = -1;
    DEBUG_PRINT("%s: message thread close fds%d %d\n", adpcm_info->thread_name,
        adpcm_info->pipe_in,adpcm_info->pipe_out);
    free(adpcm_info);
}

void omx_adpcm_post_msg(struct adpcm_ipc_info *adpcm_info, unsigned char id) {
    DEBUG_DETAIL("\n%s id=%d\n", __FUNCTION__,id);
    write(adpcm_info->pipe_out, &id, 1);
}
