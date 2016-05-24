/* afd_thread.c
 *
 * Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
#include <pthread.h>
#include "mct_queue.h"
#include "afd_thread.h"
#include "afd_port.h"
/** afd_thread_init
 *
 *  Initialize afd thread
 **/
afd_thread_data_t* afd_thread_init(void)
{
  afd_thread_data_t *thread_data;

  thread_data = malloc(sizeof(afd_thread_data_t));
  if (thread_data == NULL)
    return NULL;

  thread_data->msg_q = (mct_queue_t *)mct_queue_new;
  if (!thread_data->msg_q) {
    free(thread_data);
    return NULL;
  }

  pthread_mutex_init(&thread_data->msg_q_lock, NULL);
  mct_queue_init(thread_data->msg_q);
  pthread_cond_init(&(thread_data->thread_cond), NULL);
  pthread_mutex_init(&(thread_data->thread_mutex), NULL);
  sem_init(&thread_data->sem_launch, 0, 0);
  return thread_data;
}

/** afd_thread_deinit
 *    @p: mct_port_t object
 *
 *  deinitialize AFD thread related resources
 *
 *  Return: No
 **/
void afd_thread_deinit(void *p)
{
  afd_thread_data_t   *thread_data;
  afd_port_private_t  *private;
  mct_port_t *port    = (mct_port_t *)p;

  private = (afd_port_private_t *)port->port_private;
  if (!private)
    return;

  thread_data = private->thread_data;

  pthread_mutex_destroy(&thread_data->thread_mutex);
  pthread_cond_destroy(&thread_data->thread_cond);
  mct_queue_free(thread_data->msg_q);
  pthread_mutex_destroy(&thread_data->msg_q_lock);
  sem_destroy(&thread_data->sem_launch);
  free(thread_data);
}

/**
 *
 **/
boolean afd_thread_en_q_msg(void *afd_data,
  afd_thread_msg_t  *msg)
{
  afd_thread_data_t *thread_data = (afd_thread_data_t *)afd_data;
  boolean rc = FALSE;

  if (thread_data->active) {

    pthread_mutex_lock(&thread_data->msg_q_lock);
    mct_queue_push_tail(thread_data->msg_q, msg);
    if (msg->type == MSG_STOP_AFD_THREAD) {
      thread_data->active = 0;
    }
    pthread_mutex_unlock(&thread_data->msg_q_lock);

    pthread_mutex_lock(&thread_data->thread_mutex);
    pthread_cond_signal(&thread_data->thread_cond);
    pthread_mutex_unlock(&thread_data->thread_mutex);
    rc = TRUE;
  } else {
    rc = FALSE;
    free(msg);
  }

  return rc;
}

/** afd_thread_handler:
 *    @port_info: pointer to afd port.
 **/
static void* afd_thread_handler(void *port_info)
{
  afd_port_private_t  *private;
  afd_thread_msg_t    *msg = NULL;
  afd_thread_data_t   *thread_data;
  afd_module_object_t *afd_obj;
  mct_port_t *port = (mct_port_t *)port_info;
  int exit_flag = 0;
  private = (afd_port_private_t *)port->port_private;
  if (!private)
    return NULL;

  sem_post(&private->thread_data->sem_launch);

  thread_data = private->thread_data;
  afd_obj     = &(private->afd_object);

  do {
    pthread_mutex_lock(&thread_data->thread_mutex);
    while (thread_data->msg_q->length == 0) {
        pthread_cond_wait(&thread_data->thread_cond,
          &thread_data->thread_mutex);
    }
    pthread_mutex_unlock(&thread_data->thread_mutex);

    /* Get the message */
    pthread_mutex_lock(&thread_data->msg_q_lock);
    msg = (afd_thread_msg_t *)
      mct_queue_pop_head(thread_data->msg_q);
    pthread_mutex_unlock(&thread_data->msg_q_lock);

    if (!msg) {
      continue;
    }

    if(private->thread_data->active == 0) {
      if(msg->type != MSG_STOP_AFD_THREAD) {
          free(msg);
          msg = NULL;
          continue;
      }
    }

    CDBG("%s got event msgtype %d",__func__, msg->type);
    /* Process message accordingly */
    switch (msg->type) {
    case MSG_AFD_SET:
      CDBG("%s got set evt fn  %p",__func__, afd_obj->set_parameters);
      if (afd_obj->set_parameters) {
        int rc;
        rc = afd_obj->set_parameters(&msg->u.afd_set_parm, afd_obj->afd,
          &(afd_obj->output));
        if (rc > 1) {
          afd_obj->afd_cb(&(afd_obj->output), port);
        }
      }
      break;

    case MSG_STOP_AFD_THREAD:
      exit_flag = 1;
      break;
    case MSG_AFD_STATS: {
      boolean rc;
      rc = afd_obj->process(msg->u.stats, afd_obj->afd,
        &(afd_obj->output));
      if (rc == TRUE)
        afd_obj->afd_cb(&(afd_obj->output), port);
      }
      break;
    default:
      break;
    }

    if (msg) {
      free(msg);
      msg = NULL;
    }
  } while (!exit_flag);

  return NULL;
}

/** afd_thread_start
 *
 **/
boolean afd_thread_start(void *p)
{
  pthread_t id;
  afd_port_private_t  *private;
  afd_thread_data_t   *thread_data;
  mct_port_t *port    = (mct_port_t *)p;
  pthread_create(&id, NULL, afd_thread_handler, (void *)port);
  private = (afd_port_private_t *)port->port_private;
  if (!private)
    return FALSE;
  thread_data = private->thread_data;
  sem_wait(&thread_data->sem_launch);
  thread_data->thread_id = id;
  thread_data->active = 1;
  return TRUE;
}

/**
 *
 **/
boolean afd_thread_stop(afd_thread_data_t   *thread_data)
{
  boolean rc ;
  afd_thread_msg_t *msg = malloc(sizeof(afd_thread_msg_t));
  if (msg) {
    CDBG("%s:%d MSG_STOP_AFD_THREAD", __func__, __LINE__);
    msg->type = MSG_STOP_AFD_THREAD;
    rc = afd_thread_en_q_msg(thread_data,msg);
    if(rc)
      pthread_join(thread_data->thread_id, NULL);
     CDBG("%s:%d pthread_join", __func__, __LINE__);
  } else {
    rc = FALSE;
  }
  return rc;
}

