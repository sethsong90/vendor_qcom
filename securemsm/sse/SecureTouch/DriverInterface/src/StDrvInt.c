/*
 * Copyright(c) 2013 Qualcomm Technologies, Inc.All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
#define MAIN_C

#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>

#define SYSFS_CONTROL_FILE  "/sys/devices/f9924000.i2c/i2c-2/2-004a/secure_touch_enable"
#define SYSFS_IRQ_FILE      "/sys/devices/f9924000.i2c/i2c-2/2-004a/secure_touch"

static int fd_control = -1;
static int fd_irq = -1;

int32_t stStartSession(void)
{
  int32_t rv = 0;
  ssize_t writtenBytes = 0;
  do {
    if (fd_control != -1) {
      rv = EBUSY;
      break;
    }
    fd_control = open(SYSFS_CONTROL_FILE, O_WRONLY);
    if (fd_control == -1) {
      rv = errno;
      break;
    }
    writtenBytes = pwrite(fd_control,"1",1,0);
    if (writtenBytes == 0) {
      rv = errno;
      close(fd_control);
      fd_control = -1;
      break;
    }
  } while (0);
  return rv;
}

int32_t stTerminateSession(uint32_t force)
{
  int32_t rv = 0;
  ssize_t writtenBytes = 0;
  do {
    if (force && (fd_control == -1)) {
      fd_control = open(SYSFS_CONTROL_FILE, O_WRONLY);
    }
    if (fd_control == -1) {
      rv = ENODEV;
      break;
    }
    writtenBytes = pwrite(fd_control,"0",1,0);
    if (writtenBytes == 0) {
      rv = errno;
      break;
    }
    close(fd_control);
    fd_control = -1;
    if (fd_irq != -1) {
      close(fd_irq);
      fd_irq = -1;
    }
  } while (0);
  return rv;
}

int32_t stWaitForEvent(int32_t abortFd, int32_t timeout)
{
  int32_t rv = 0;
  ssize_t readBytes = 0;
  char c;
  struct pollfd *fds = NULL; /* Used for poll() */
  size_t events = 1; /* Number of FD to poll */

  do {
    if (fd_irq == -1) {
      fd_irq = open(SYSFS_IRQ_FILE,O_RDONLY);
      if (fd_irq == -1) {
        rv = errno;
        break;
      }
    }

    // read and verify if an interrupt is already pending
    readBytes = pread(fd_irq,&c,1,0);
    if (readBytes <= 0) {
      rv = errno;
      break;
    }

    if (c == '1') {
      // interrupt
      rv = 0;
      break;
    }

    if (abortFd != -1)
      events = 2;

    fds = (struct pollfd *)calloc(events, sizeof(struct pollfd));
    if (fds == NULL) {
      rv = -ENOMEM;
      break;
    }
    /* IRQ FD, always available */
    fds[0].fd = fd_irq;
    fds[0].events = POLLERR|POLLPRI;
    /* FD for abort requests */
    if (events == 2) {
      fds[1].fd = abortFd;
      fds[1].events = POLLIN;
    }

    rv = poll(fds, events, timeout);
    if (rv < 0) {
      /* Error, return error condition */
      rv = errno;
      break;
    }
    if (rv == 0) {
      /* timeout */
      rv = -ETIMEDOUT;
      break;
    }
    /* Check for external abort */
    if ((events == 2) && (fds[1].revents)) {
      rv = -ECONNABORTED;
      break;
    }
    /* Consume data, or error, and return */
    if (fds[0].revents) {
      readBytes = pread(fd_irq,&c,1,0);
      if (readBytes <= 0) {
        rv = errno;
        break;
      }
      if (c == '1') {
        // interrupt
        rv = 0;
        break;
      }
      rv = 0;
    }
  } while (0);
  if (fds) {
    free (fds);
  }
  return rv;
}
