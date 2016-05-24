/* Copyright (c) 2007 by Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <adec_svr.h>
#include <string.h>
#include <errno.h>

/**
 @brief This function starts command server

 @param cb pointer to callback function from the client
 @param client_data reference client wants to get back
  through callback
 @return handle to msging thread
 */
struct mp3_ipc_info *omx_mp3_thread_create(
                                    message_func cb,
                                    void* client_data,
                                    char* th_name)
{
  int r;
  int fds[2];
  struct mp3_ipc_info *mp3_info;

  mp3_info = calloc(1, sizeof(struct mp3_ipc_info));
  if (!mp3_info) return 0;

  mp3_info->client_data = client_data;
  mp3_info->process_msg_cb = cb;
  strlcpy(mp3_info->thread_name,th_name,sizeof(mp3_info->thread_name));

  if (pipe(fds)) {
    DEBUG_PRINT_ERROR("\n%s: pipe creation failed\n", __FUNCTION__);
    goto fail_pipe;
  }

  mp3_info->pipe_in = fds[0];
  mp3_info->pipe_out = fds[1];


  r = pthread_create(&mp3_info->thr, 0, omx_mp3_msg, mp3_info);
  if (r < 0) goto fail_thread;

  DEBUG_DETAIL("Created thread for %s \n", mp3_info->thread_name);
  return mp3_info;


fail_thread:
  close(mp3_info->pipe_in);
  close(mp3_info->pipe_out);

fail_pipe:
  free(mp3_info);

  return 0;
}

/**
 @brief This function starts the event thread

 @param cb pointer to callback function from the client
 @param client_data reference client wants to get back
  through callback
 @return handle to msging thread
 */
struct mp3_ipc_info *omx_mp3_event_thread_create(
                                    message_func cb,
                                    void* client_data,
                                    char* th_name)
{
  int r;
  int fds[2];
  struct mp3_ipc_info *mp3_info;

  mp3_info = calloc(1, sizeof(struct mp3_ipc_info));
  if (!mp3_info) return 0;

  mp3_info->client_data = client_data;
  mp3_info->process_msg_cb = cb;
  strlcpy(mp3_info->thread_name,th_name,sizeof(mp3_info->thread_name));

  if (pipe(fds)) {
    DEBUG_PRINT_ERROR("\n%s: pipe creation failed\n", __FUNCTION__);
    goto fail_pipe;
  }

  mp3_info->pipe_in = fds[0];
  mp3_info->pipe_out = fds[1];


  r = pthread_create(&mp3_info->thr, 0, omx_mp3_events, mp3_info);
  if (r < 0) goto fail_thread;

  DEBUG_DETAIL("Created thread for %s \n", mp3_info->thread_name);
  return mp3_info;


fail_thread:
  close(mp3_info->pipe_in);
  close(mp3_info->pipe_out);

fail_pipe:
  free(mp3_info);

  return 0;
}

/**
 @brief This function processes posted messages

 Once thread is being spawned, this function is run to
 start processing commands posted by client

 @param info pointer to context

 */
void *omx_mp3_msg(void *info)
{
    struct mp3_ipc_info *mp3_info = (struct mp3_ipc_info*)info;
    unsigned char id;
    int n;

    DEBUG_DETAIL("\n%s: message thread start\n", __FUNCTION__);
    while (!mp3_info->dead) {
        n = read(mp3_info->pipe_in, &id, 1);
        if (n == 0) break;
        if (n == 1) {
          DEBUG_DETAIL("\n%s-->pipe_in=%d pipe_out=%d\n",
                                               mp3_info->thread_name,
                                               mp3_info->pipe_in,
                                               mp3_info->pipe_out);

         mp3_info->process_msg_cb(mp3_info->client_data, id);

        }
        if ((n < 0) && (errno != EINTR)) break;
    }
    DEBUG_DETAIL("%s: message thread stop\n", mp3_info->thread_name);

    return 0;
}

void *omx_mp3_events(void *info)
{
    struct mp3_ipc_info *mp3_info = (struct mp3_ipc_info*)info;
    unsigned char id = 0;
    DEBUG_DETAIL("%s: message thread start\n", mp3_info->thread_name);
    mp3_info->process_msg_cb(mp3_info->client_data, id);
    DEBUG_PRINT("%s: message thread stop\n", mp3_info->thread_name);
    return 0; 	
}


void omx_mp3_thread_stop(struct mp3_ipc_info *mp3_info)
{
  int rc = 0;
  DEBUG_DETAIL("%s: message thread close fds%d %d\n", mp3_info->thread_name,
                                             mp3_info->pipe_in,mp3_info->pipe_out);

  close(mp3_info->pipe_in);
  close(mp3_info->pipe_out);
  rc = pthread_join(mp3_info->thr,NULL);
  mp3_info->pipe_out = -1;
  mp3_info->pipe_in = -1;
  DEBUG_DETAIL("%s: message thread close fds%d %d rc= %d\n", mp3_info->thread_name,
                                             mp3_info->pipe_in,mp3_info->pipe_out, rc);
  free(mp3_info);
}

void omx_mp3_post_msg(struct mp3_ipc_info *mp3_info, unsigned char id) {
  DEBUG_DETAIL("\n%s id=%d\n", __FUNCTION__,id);
  write(mp3_info->pipe_out, &id, 1);
}
