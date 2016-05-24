/*
 * Copyright(c) 2013 Qualcomm Technologies, Inc.All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

/*
 * NOTE: This file is for demonstration purposes only.
 * Please change it according to the device specific configuration.
 * */


#include <fcntl.h>
#include <module.h>
#include <android/log.h>
#include <pthread.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define LOG_TAG "sensorsModule"
#define LOG_D(...) __android_log_print(ANDROID_LOG_DEBUG, MY_LOG_TAG, __VA_ARGS__)


/* If enabled, testing will cause the module to generate an asynchronous event
 * after 1 to 5 seconds after the time it has been disabled. */
#ifdef MODULE_TESTING_ENABLED

static pthread_t event_thread;
static int fd;

static void * ping_event(void* arg) {
  int s = rand() % 4 + 1; // sleep between 1 and 5 seconds
  LOG_D("%s in %u secs", __func__, s);
  sleep(s);
  if (fd != -1) {
    uint64_t u = 1;
    LOG_D("pinging");
    write(fd,&u,sizeof(u));
    close(fd);
    fd = -1;
  }
  return NULL;
}

#endif

int sensorsDisable(int * efd) {
#ifndef MODULE_TESTING_ENABLED
  if (efd) *efd=-1;
#else
  LOG_D("%s", __func__);
  if (efd) {
    int rv = 0;
    if (fd != -1) {
      close(fd);
      fd = -1;
      pthread_join(event_thread, NULL);
    }
    fd = eventfd(0,0);
    rv = pthread_create(&event_thread,
                        NULL,
                        ping_event,
                        NULL);
    *efd = fd; // no signaling
  }
#endif
  return 0;
}

int sensorsEnable(int efd) {
#ifdef MODULE_TESTING_ENABLED
  LOG_D("%s", __func__);
  if (efd == fd) {
    if (fd != -1) {
      close(fd);
      fd = -1;
      pthread_join(event_thread, NULL);
    }
  }
#endif
  return 0;
}

struct SideChannelModule sensorsModule = {
  sensorsDisable,
  sensorsEnable,
  "audio"
};
