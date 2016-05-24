/*
 * Copyright(c) 2013 Qualcomm Technologies, Inc.All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include <android/log.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <module.h>
#include <poll.h>
#include <StSideChannels.h>
#include <vector>
#include <sys/eventfd.h>

#define MY_LOG_TAG "StSideChannels"

#define LOG_D(...) __android_log_print(ANDROID_LOG_DEBUG, MY_LOG_TAG, __VA_ARGS__)
#define LOG_I(...) __android_log_print(ANDROID_LOG_INFO, MY_LOG_TAG, __VA_ARGS__)
#define LOG_W(...) __android_log_print(ANDROID_LOG_WARN, MY_LOG_TAG, __VA_ARGS__)
#define LOG_E(...) __android_log_print(ANDROID_LOG_ERROR, MY_LOG_TAG, __VA_ARGS__)

/* Array with all registered subsystems */
extern struct SideChannelModule modules[];
extern size_t modulesLen;

/* EventFD created by modules, we will wait on them */
static std::vector<int> efds;

/* EventFD created by us, to signal the waiting thread */
static int g_efd = -1;

/* This mutex protects access to efds */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* Library constructor */
__attribute__((constructor)) static void stInit() {
  g_efd = eventfd(0,0);
}

/* Library destructor */
__attribute__((destructor)) static void stDeinit() {
  if (g_efd >= 0) close(g_efd);
}

int stDisableChannels(void)
{
  int rv = 0;
  size_t i = 0;
  int efd = -1;

  //LOG_D("%s", __func__);

  if (g_efd == -1) {
    LOG_E("Initialization failed");
    return -ENODEV;
  }
  if (0 != pthread_mutex_lock(&mutex)) {
    LOG_E("Failed to lock mutex: %s", strerror(errno));
    return -EIO;
  }
  do {
    if (!efds.empty()) {
      rv = -EBUSY;
      break;
    }

    // go through all modules and disable them
    for (i = 0; i < modulesLen; i++) {
      if (modules[i].disableChannel == NULL) {
        LOG_E("Disable channel not provided for module %s", modules[i].name);
        rv = -EINVAL;
        break;
      }
      efd = -1;
      rv = modules[i].disableChannel(&efd);
      if (rv != 0) {
        LOG_E("Failed to disable module %s", modules[i].name);
        break;
      }
      efds.push_back(efd);
    }
  } while (0);
  if ((rv != 0) && (rv != -EBUSY)) {
    // cleanup
    for (i = 0; i < efds.size(); i++) {
      if (modules[i].enableChannel) {
        modules[i].enableChannel(efds[i]);
      }
    }
    efds.clear();
  }
  pthread_mutex_unlock(&mutex);
  return rv;
}

int stEnableChannels(void)
{
  int rv = 0;
  size_t s = 0;
  size_t i = 0;
  uint64_t u = 1;

  //LOG_D("%s", __func__);

  if (g_efd == -1) {
    LOG_E("Initialization failed");
    return -ENODEV;
  }

  // wake up the waiting thread, if it is waiting, so it can release the mutex
  s = write(g_efd,&u,sizeof(u));
  if (s != sizeof(u)) {
    LOG_E("Failed to wake up the waiting thread: %s", strerror(errno));
    return -EIO;
  }

  if (0 != pthread_mutex_lock(&mutex)) {
    LOG_E("Failed to lock mutex: %s", strerror(errno));
    return -EIO;
  }
  // we can enable back all the subsystems
  do {
    if (efds.empty()) {
      rv = 0; // all already enabled
      break;
    }
    // enable all the remaining
    for (i = 0; i < efds.size(); i++) {
      if (modules[i].enableChannel) {
        modules[i].enableChannel(efds[i]);
      }
    }
    efds.clear();
  } while (0);
  pthread_mutex_unlock(&mutex);
  return rv;
}

int stWaitForChannelEvent(void)
{
  int rv = 0;
  size_t i = 0;

  //LOG_D("%s", __func__);

  if (g_efd == -1) {
    LOG_E("Initialization failed");
    return -ENODEV;
  }
  if (efds.empty()) {
    LOG_E("No channels blocked?!?");
    return -EBADF; // already enabled?
  }
  if (0 != pthread_mutex_lock(&mutex)) {
    LOG_E("Failed to lock mutex: %s", strerror(errno));
    return -EIO;
  }
  do {
    struct pollfd fds[efds.size()+1];
    for (i = 0; i < efds.size(); i++) {
      fds[i].fd = efds[i];
      fds[i].events = POLLIN;
      fds[i].revents = 0;
    }
    // our internal event as last
    fds[efds.size()].fd = g_efd;
    fds[efds.size()].events = POLLIN;
    fds[efds.size()].revents = 0;
    rv = poll(fds, efds.size()+1, -1);
    // add log to print which event caused us to wake up
    if (rv > 0) {
      rv = 0;
      for (i = 0; i < efds.size(); i++) {
        if (fds[i].revents & POLLIN) {
          LOG_I("Wake up event from module %s", modules[i].name);
        }
      }
      // read and clear our own event
      if (fds[efds.size()].revents & POLLIN) {
        LOG_W("Woken up by the upper layer");
        uint64_t u;
        size_t s = 0;
        s = read(g_efd,&u,sizeof(u));
        if (s != sizeof(u)) {
          LOG_W("Failed to read from internal eventfd, further calls might fail");
        }
      }
    }
  } while (0);
  pthread_mutex_unlock(&mutex);
  return rv;
}
