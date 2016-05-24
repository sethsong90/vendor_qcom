/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
Copyright (c) 2012-2013 by Qualcomm Technologies, Inc. All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.

              SSR: report subsystem state events to Diag Interface

GENERAL DESCRIPTION
  Detect subsystem status and send events to QXDM

EXTERNALIZED FUNCTIONS
  None

INITIALIZATION AND SEQUENCING REQUIREMENTS

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

#include "event.h"
#include "msg.h"
#include "log.h"

#include "diag_lsm.h"
#include "stdio.h"

#include "diagpkt.h"
#include "diagcmd.h"
#include "diagdiag.h"
#include "diag.h"

#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <linux/netlink.h>

#include <cutils/log.h>

#define UEVENT_BUF_SIZE 1024
#define SSR_EVENT_BUF_SIZE 16
#define PIL_BUF_SIZE 42
#define READ_BUF_SIZE 16

#define PIL_PREFIX "/sys/bus/pil/devices/pil"
#define SUBSYS_PREFIX "/sys/bus/msm_subsys/devices/subsys"
#define DEV_ONLINE "ONLINE"
#define DEV_OFFLINE "OFFLINE"
#define MAX_STR_LEN 36

#define ONLINE 1
#define OFFLINE 0

#define PIL_DEV_NUM 8

#define PIL_MODEM_FW 0
#define PIL_MODEM 1
#define PIL_RIVA 2
#define PIL_DSPS 3
#define PIL_GSS 4
#define PIL_EXT_MODEM 5
#define PIL_ADSP 6
#define PIL_WCNSS 7

typedef struct {
	int fd[PIL_DEV_NUM];
	int dev[PIL_DEV_NUM];
	int state[PIL_DEV_NUM];
} pil_s;
static pil_s subsys;

/* List of all pil devices in addition to MDM */
static char *pil_list[PIL_DEV_NUM] = {
	"modem_fw",
	"modem",
	"riva",
	"dsps",
	"gss",
	"external_modem",
	"adsp",
	"wcnss"
};

/* pil poll struct */
static struct pollfd pfd[PIL_DEV_NUM];

/*==========================================================================*/
/* Local Function declarations */
/*==========================================================================*/

/*==========================================================================*/
/* create this function because of no definition in <string.h> */
char *strnstr(const char *s1, const char *s2, size_t len1)
{
	size_t l2;

	l2 = strnlen(s2, MAX_STR_LEN);
	if (!l2)
		return (char *)s1;

	while (len1 >= l2) {
		if (!memcmp(s1, s2, l2))
			return (char *)s1;
		s1++;
		len1--;
	}

	return NULL;
}

int assemble_name_string(char *out, int len, char *head, char *tail)
{
	/* check buf and len */
	if (out == NULL || head == NULL || tail == NULL || len <= 0)
		return -EINVAL;

	if (out != head)
		strlcpy(out, head, len);
	strlcat(out, tail, len);

	return 0;
}

int do_event_payload(int id, int cur, char *buf, int len)
{
	/* check buf and len */
	if (buf == NULL || len <= 0)
		return -EINVAL;

	switch (id) {
	case PIL_MODEM:
		strlcpy(buf, "MODEM ", len);
		break;

	case PIL_RIVA:
		strlcpy(buf, "RIVA ", len);
		break;

	case PIL_DSPS:
		strlcpy(buf, "DSPS ", len);
		break;

	case PIL_GSS:
		strlcpy(buf, "GSS ", len);
		break;

	case PIL_EXT_MODEM:
		strlcpy(buf, "MDM ", len);
		break;

	case PIL_ADSP:
		strlcpy(buf, "ADSP ", len);
		break;

	case PIL_WCNSS:
		strlcpy(buf, "WCNSS ", len);
		break;

	default:
		strlcpy(buf, "UNKNOWN", len);
		break;
	}

	if (cur == ONLINE)
		strlcat(buf, "power up", len);
	else if (cur == OFFLINE)
		strlcat(buf, "shutdown", len);

	return 0;
}

/*
 search all pil devices and find match subsysm id
 return value
 -1: no more pil devices
  0: no match subsysm in pil_list
  1: match subsystem in pil_list
*/
int open_subsys_fd(int pil_num, pil_s *subsys, char *prefix, int pfd_num)
{
	int i;
	int fd;
	char num_buf[2];
	char rd_buf[READ_BUF_SIZE];
	char dev_buf[PIL_BUF_SIZE];
	char st_buf[PIL_BUF_SIZE];

	/* convert pil num to character */
	num_buf[0] = pil_num + 0x30;
	num_buf[1] = '\0';

	/* check subsystem pil name */
	assemble_name_string(dev_buf, PIL_BUF_SIZE, prefix, num_buf);
	assemble_name_string(dev_buf, PIL_BUF_SIZE, dev_buf, "/name");

	/* Exit if the device doesn't exist */
	if (access(dev_buf, F_OK) < 0)
		return -EINVAL;

	fd = open(dev_buf, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "SSR: open pil%d failed. errno=%d\n",
			pil_num, errno);
		return -EINVAL;
	}

	if (read(fd, (void *)rd_buf, READ_BUF_SIZE) > 0) {
		for (i = 0; i < PIL_DEV_NUM; i++) {
			/* compare subsystem name string */
			if (strnstr(rd_buf, pil_list[i],
				strnlen(pil_list[i], MAX_STR_LEN))) {
				/* skip modem_fw */
				if (i == PIL_MODEM_FW)
					break;

				assemble_name_string(st_buf,
					PIL_BUF_SIZE, prefix, num_buf);
				assemble_name_string(st_buf,
					PIL_BUF_SIZE, st_buf, "/state");

				/* open failed subsystem as no hit */
				subsys->fd[pfd_num] = open(st_buf, O_RDONLY);
				if (subsys->fd[pfd_num] < 0) {
					fprintf(stderr,
						"SSR: open %s failed.\n",
						pil_list[i]);
					break;
				}

				/* store subsystem fd */
				subsys->dev[pfd_num] = i;
				close(fd);
				return 1;
			}
		}
	}
	close(fd);

	return 0;
}

int main()
{
	boolean bInit_Success = FALSE;
	int i;
	int ret;
	int pfd_num = 0;
	int cur_state;
	char rd_buf[READ_BUF_SIZE];
	char payload[SSR_EVENT_BUF_SIZE];

	/* init diag */
	bInit_Success = Diag_LSM_Init(NULL);
	if (!bInit_Success) {
		fprintf(stderr, "SSR: Diag_LSM_Init() failed\n");
		goto err_exit1;
	}

	/* init subsystem struct */
	for (i = 0; i < PIL_DEV_NUM; i++) {
		subsys.fd[i] = -1;
		subsys.state[i] = OFFLINE;
	}

	/* search and open fd in pil_list */
	for (i = 0;; i++) {
		ret = open_subsys_fd(i, &subsys, SUBSYS_PREFIX, pfd_num);
		if (ret > 0) {
			pfd_num++;
			if (pfd_num >= PIL_DEV_NUM)
				break;
		} else if (ret < 0)
			break;
	}

	/* If no fd is found, use PIL prefix to search again */
	if (pfd_num == 0) {
		for (i = 0;; i++) {
			ret = open_subsys_fd(i, &subsys, PIL_PREFIX, pfd_num);
			if (ret > 0) {
				pfd_num++;
				if (pfd_num >= PIL_DEV_NUM)
					break;
			} else if (ret < 0)
				break;
		}
	}

	/* exit if no subsys found */
	if (pfd_num == 0) {
		fprintf(stderr, "SSR: No match subsystem found\n");
		goto err_exit2;
	}

	/* count poll pfd numbers and read subsystem state */
	for (i = 0; i < pfd_num; i++) {
		pfd[i].fd = subsys.fd[i];
		pfd[i].events = POLLPRI;
		pfd[i].revents = 0;
		if (read(subsys.fd[i], (void *)rd_buf, READ_BUF_SIZE)) {
			if (strnstr(rd_buf, DEV_ONLINE,
				strnlen(DEV_ONLINE, MAX_STR_LEN))) {
				subsys.state[i] = ONLINE;
			}
			lseek(subsys.fd[i], 0, SEEK_SET);
		}
	}

	while (1) {
		if (poll(pfd, pfd_num, -1) < 0) {
			fprintf(stderr, "SSR: poll fail errno=%d\n", errno);
			break;
		}

		/* read subsystem state */
		for (i = 0; i < pfd_num; i++) {
			if (!read(pfd[i].fd, (void *)rd_buf, READ_BUF_SIZE))
				continue;

			/* check if state change */
			if (strnstr(rd_buf, DEV_OFFLINE,
				strnlen(DEV_OFFLINE, MAX_STR_LEN))) {
				cur_state = OFFLINE;
				if (subsys.state[i] != cur_state) {
					subsys.state[i] = cur_state;
					do_event_payload(subsys.dev[i],
						cur_state,
						payload,
						SSR_EVENT_BUF_SIZE);
					event_report_payload(
						EVENT_SSR_SUBSYS_PWR_DOWN,
						SSR_EVENT_BUF_SIZE,
						payload);
				}
			} else if (strnstr(rd_buf, DEV_ONLINE,
				strnlen(DEV_ONLINE, MAX_STR_LEN))) {
				cur_state = ONLINE;
				if (subsys.state[i] != cur_state) {
					subsys.state[i] = cur_state;
					do_event_payload(subsys.dev[i],
						cur_state,
						payload,
						SSR_EVENT_BUF_SIZE);
					event_report_payload(
						EVENT_SSR_SUBSYS_PWR_UP,
						SSR_EVENT_BUF_SIZE,
						payload);
				}
			}

			lseek(pfd[i].fd, 0, SEEK_SET);
		}
	}

	for (i = 0; i < pfd_num; i++) {
		close(subsys.fd[i]);
		subsys.fd[i] = -1;
	}

err_exit2:
	Diag_LSM_DeInit();

err_exit1:
	return 0;
}
