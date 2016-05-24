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

 @param _svr pointer to command server context

 */
void *aenc_message_thread(void *_svr)
{
    struct aenc_cmd_svr *svr = (struct aenc_cmd_svr*)_svr;
    unsigned char id;
    int n;

    DEBUG_DETAIL("\n%s: message thread start\n", __FUNCTION__);
    while (!svr->dead) {
        n = read(svr->pipe_in, &id, 1);
        if (n == 0) break;
        if (n == 1) {
          DEBUG_DETAIL("\n%s: process next event\n", __FUNCTION__);
          svr->process_msg_cb(svr->client_data, id);
        }
        if ((n < 0) && (errno != EINTR)) break;
    }

    DEBUG_DETAIL("%s: message thread stop\n", __FUNCTION__);

    return 0;
}

void *aenc_message_output_thread(void *_cln)
{
    struct aenc_cmd_cln *cln = (struct aenc_cmd_cln*)_cln;
    unsigned char id;
    int n;

    DEBUG_DETAIL("\n%s: message thread start\n", __FUNCTION__);
    while (!cln->dead) {
        n = read(cln->pipe_in, &id, 1);
        if (n == 0) break;
        if (n == 1) {
          DEBUG_DETAIL("\n%s: process next event\n", __FUNCTION__);
          cln->process_msg_output_cb(cln->client_data, id);
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
 @return handle to command server
 */
struct aenc_cmd_svr *aenc_svr_start(process_message_func cb,
                                    void* client_data)
{
  int r;
  int fds[2];
  struct aenc_cmd_svr *svr;

  DEBUG_DETAIL("%s: start server\n", __FUNCTION__);
  svr = calloc(1, sizeof(struct aenc_cmd_svr));
  if (!svr) return 0;

  svr->client_data = client_data;
  svr->process_msg_cb = cb;

  if (pipe(fds)) {
    DEBUG_PRINT_ERROR("\n%s: pipe creation failed\n", __FUNCTION__);
    goto fail_pipe;
  }

  svr->pipe_in = fds[0];
  svr->pipe_out = fds[1];


  r = pthread_create(&svr->thr, 0, aenc_message_thread, svr);
  if (r < 0) goto fail_thread;

  return svr;



fail_thread:
  close(svr->pipe_in);
  close(svr->pipe_out);

fail_pipe:
  free(svr);

  return 0;
}

/**
 @brief This function stop command server

 @param svr handle to command server
 @return none
 */
void aenc_svr_stop(struct aenc_cmd_svr *svr) {
  DEBUG_DETAIL("%s stop server\n", __FUNCTION__);
  close(svr->pipe_in);
  close(svr->pipe_out);
  pthread_join(svr->thr, NULL);
  svr->pipe_in = -1;
  svr->pipe_out = -1;
  free(svr);
}

/**
 @brief This function post message in the command server

 @param svr handle to command server
 @return none
 */
void aenc_svr_post_msg(struct aenc_cmd_svr *svr, unsigned char id) {
  DEBUG_DETAIL("\n%s id=%d\n", __FUNCTION__,id);
  write(svr->pipe_out, &id, 1);
}

void aenc_output_post_msg(struct aenc_cmd_cln *cln, unsigned char id) {
  DEBUG_DETAIL("\n%s id=%d\n", __FUNCTION__,id);
  write(cln->pipe_out, &id, 1);
}

//TODO ..........remove this function and write single function for all the 3 threads
void aenc_input_post_msg(struct aenc_cmd_cln *input_cln, unsigned char id) {
  DEBUG_DETAIL("\n%s id=%d\n", __FUNCTION__,id);
  write(input_cln->pipe_out, &id, 1);
}

/**
 @brief This function starts command server

 @param cb pointer to callback function from the client
 @param client_data reference client wants to get back
  through callback
 @return handle to command server
 */
struct aenc_cmd_cln *aenc_cln_start(process_message_func cb,
                                    void* client_data)
{
  int r;
  int fds[2];
  struct aenc_cmd_cln *cln;

  DEBUG_DETAIL("%s: start server\n", __FUNCTION__);
  cln = calloc(1, sizeof(struct aenc_cmd_cln));
  if (!cln) return 0;

  cln->client_data = client_data;
  cln->process_msg_output_cb = cb;

  if (pipe(fds)) {
    DEBUG_DETAIL("\n%s: pipe creation failed\n", __FUNCTION__);
    goto fail_pipe;
  }

  cln->pipe_in = fds[0];
  cln->pipe_out = fds[1];


  r = pthread_create(&cln->thr, 0, aenc_message_output_thread, cln);
  if (r < 0) goto fail_thread;

  return cln;



fail_thread:
  close(cln->pipe_in);
  close(cln->pipe_out);

fail_pipe:
  free(cln);

  return 0;
}

void aenc_cln_stop(struct aenc_cmd_cln *cln) {
  DEBUG_DETAIL("%s stop client\n", __FUNCTION__);
  close(cln->pipe_in);
  close(cln->pipe_out);
  pthread_join(cln->thr, NULL);
  cln->pipe_in = -1;
  cln->pipe_out = -1;
  free(cln);
}

