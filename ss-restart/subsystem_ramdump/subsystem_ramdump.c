/*
 * Copyright (c) 2011-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 *
 * Polls subsystem ramdumps and outputs them to SD card or eMMC
 *
 * To add additional subsystem ramdump logs you must add the entry to
 * RAMDUMP_LIST and NODES, and increase DUMP_NUM.
 *
 * Support MDM, QSC ramdump and rpm log
 */

#include <stdio.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <linux/netlink.h>

#include <cutils/log.h>

#define BUFFER_SIZE	0x00008000
#define SUCCESS	0

#define LOG_NUM 1
#define DUMP_NUM 16
#define MDMDUMP_NUM 10
#define QSCDUMP_NUM 7

#define FILENAME_SIZE 60
#define TIMEBUF_SIZE 21
#define UEVENT_BUF_SIZE 1024
#define RPMLOG_BUF_SIZE 0x00500000
#define MAX_STR_LEN 36
#define CHK_ELF_LEN 5

#define UEVENT_MDM_SHUTDOWN "remove@/devices/platform/diag_bridge"
#define UEVENT_MDM_POWERUP "add@/devices/platform/diag_bridge"
#define DUMP_SDCARD_DIR "/sdcard/ramdump"
#define DUMP_EMMC_DIR "/data/ramdump"
#define EXT_MDM_DIR "/tombstones/mdm"
#define EXT_MDM2_DIR "/tombstones/mdm2"
#define DUMP_SMEM_STR "ramdump_smem"
#define DUMP_MODEM_SW_STR "ramdump_modem_sw"
#define DUMP_HEAD_STR "/dev/"
#define DUMP_TAIL_BIN ".bin"
#define DUMP_TAIL_ELF ".elf"
#define STR_ELF "ELF"
#define STR_MDM "/mdm"
#define STR_QSC "/qsc"

/* SSR events */
#define SSR_LOG_START 0x00000001
#define SSR_OCCURRED 0x00000002
#define SSR_MSM_AND_QSC 0x00000004
#define SSR_DUMP_SDCARD 0x00000008

typedef struct {
	int fd[DUMP_NUM];
	int dev[DUMP_NUM];
	char **dev_list;
	char *dir;
} ramdump_s;

/* List of all ramdump dev, add new nodes to the bottom */
char *RAMDUMP_LIST[DUMP_NUM] = {
	"ramdump_adsp",
	"ramdump_dsps",
	"ramdump_gss",
	"ramdump_lpass",
	"ramdump_modem",
	"ramdump_modem_fw",
	"ramdump_modem_sw",
	"ramdump_venus",
	"ramdump_riva",
	"ramdump_smem-dsps",
	"ramdump_smem-gss",
	"ramdump_smem-modem",
	"ramdump_pronto",
	"ramdump_smem",
	"ramdump_smem-smd",
	"ramdump_audio-ocmem"
};

/* List of log dev */
char *LOG_LIST[LOG_NUM] = {
	"/sys/kernel/debug/rpm_log"
};

/* Log output files */
char *LOG_NODES[LOG_NUM] = {
	"/rpm_log"
};

/* MDM dump files */
char *mdm_list[MDMDUMP_NUM] = {
	"/CODERAM.BIN",
	"/CPU_REG.BIN",
	"/EBI1.BIN",
	"/load.cmm",
	"/LPASS.BIN",
	"/Q6_TCM.BIN",
	"/RPM_MSG.BIN",
	"/RPM_REG.BIN",
	"/RST_STAT.BIN",
	"/SYS_IMEM.BIN"
};

/* QSC dump files */
char *qsc_list[QSCDUMP_NUM] = {
	"/load.cmm",
	"/mdsp_rama.lst",
	"/mdsp_ramb.lst",
	"/mdsp_ramc.lst",
	"/mdsp_regs.lst",
	"/nor_dump_0x0.bin",
	"/psram_dump_0x1.bin"
};

static ramdump_s ramdump;

/* Poll struct */
static struct pollfd pfd[DUMP_NUM];
static struct pollfd plogfd[LOG_NUM];

sem_t ramdump_sem;
int ssr_flag = -1;

/*==========================================================================*/
/* Local Function declarations */
/*==========================================================================*/
int generateRamdump(int index, ramdump_s *dump, char *tm);
static int parse_args(int argc, char **argv);

/*===========================================================================*/
int check_folder(char *f_name)
{
	int ret = SUCCESS;
	struct stat st;

	if ((ret = stat(f_name, &st)) != SUCCESS)
		fprintf(stderr, "SSR: %s doesn't exist\n", f_name);

	return ret;
}

int create_folder(char *f_name)
{
	int ret = SUCCESS;

	if ((ret = mkdir(f_name, S_IRWXU | S_IRWXG | S_IRWXO)) < 0)
		fprintf(stderr, "Ramdump: Unable to create %s\n", f_name);

	return ret;
}

char *get_current_timestamp(char *buf, int len)
{
	time_t local_time;
	struct tm *tm;

	if (buf == NULL || len < TIMEBUF_SIZE) {
		fprintf(stderr, "SSR: timestamp buf error\n");
		goto err_exit;
	}

	/* Get current time */
	local_time = time(NULL);
	if (!local_time) {
		fprintf(stderr, "SSR: unable to get timestamp\n");
		goto err_exit;
	}
	tm = localtime(&local_time);

	if (!tm) {
		fprintf(stderr, "SSR: unable to get localtime\n");
		goto err_exit;
	}

	snprintf(buf, TIMEBUF_SIZE,
		"_%04d-%02d-%02d_%02d-%02d-%02d", tm->tm_year+1900,
		tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min,
		tm->tm_sec);

	return buf;
err_exit:
	return NULL;
}

void ssr_ramdump_signal_handler(int sig)
{
	switch (sig) {
	case SIGUSR1:
	case SIGUSR2:
	case SIGTERM:
		/* call thread exits */
		pthread_exit(NULL);
		break;

	default:
		break;
	}
}

int ssr_stop_thread(pthread_t thread_id, int sig)
{
	int ret = SUCCESS;

	/* Signal the thread to exit */
	ret = pthread_kill(thread_id, sig);
	if (ret != SUCCESS)
		fprintf(stderr, "Ramdump: pthread_kill failed\n");
	else {
		ret = pthread_join(thread_id, NULL);
		if (ret != SUCCESS)
			fprintf(stderr, "Ramdump: pthread_join failed\n");
	}

	return ret;
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

int copy_file(char *out_f, char *in_f)
{
	int ret = SUCCESS;
	int in, out;
	int rbytes;
	char buf[BUFFER_SIZE];

	if (out_f == NULL || in_f == NULL) {
		ret = -EINVAL;
		goto err_exit_1;
	}

	/* check if source file exist */
	if (access(in_f, F_OK) != SUCCESS) {
		ret = -EINVAL;
		goto err_exit_1;
	}

	/* make sure output file doesn't exist before creation */
	remove(out_f);

	in = open(in_f, O_RDONLY);
	if (in < 0) {
		fprintf(stderr, "SSR: open %s fail\n", in_f);
		ret = -EIO;
		goto err_exit_1;
	}

	out = open(out_f, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	if (out < 0) {
		fprintf(stderr, "SSR: open %s fail\n", out_f);
		ret = -EIO;
		goto err_exit_2;
	}

	while ((rbytes = read(in, buf, BUFFER_SIZE)) > 0) {
		ret = write(out, buf, rbytes);
		if (ret < 0) {
			fprintf(stderr, "SSR: write error. fd=%d\n", out);
			break;
		}
	}
	close(out);

err_exit_2:
	close(in);

err_exit_1:
	return ret;
}

static int ssr_copy_dump(char *out, char *in, char **list, int num, char *tm)
{
	int ret = SUCCESS;
	int i;
	int err_cnt = 0;
	char dir[FILENAME_SIZE];
	char out_file[FILENAME_SIZE];
	char in_file[FILENAME_SIZE];

	assemble_name_string(dir, FILENAME_SIZE, ramdump.dir, out);
	assemble_name_string(dir, FILENAME_SIZE, dir, tm);
	if (create_folder(dir) < 0) {
		fprintf(stderr, "SSR: create %s fail\n", dir);
		ret = -EINVAL;
		goto err_exit;
	}

	for (i = 0; i < num; i++) {
		assemble_name_string(out_file, FILENAME_SIZE, dir, list[i]);
		assemble_name_string(in_file, FILENAME_SIZE, in, list[i]);
		if (copy_file(out_file, in_file) != SUCCESS)
			err_cnt++;
	}

	if (err_cnt != 0) {
		ret = -EINVAL;
		if ((err_cnt == QSCDUMP_NUM) || (err_cnt == MDMDUMP_NUM)) {
			remove(dir);
			/* DUMP_NUM is greater than QSC or MDM num. Use it
			 * as the notification for empty folder.
			 */
			ret = DUMP_NUM;
		}
	}

err_exit:
	return ret;
}

static void *mdm_ramdump_mon(void* param)
{
	int ret;
	char timestamp[TIMEBUF_SIZE];
	struct stat st;

	while (1) {
		/* wait mdm ramdump is ready */
		sem_wait(&ramdump_sem);

		/*
		* mdm ramdump should be collected after a completed restart
		* We don't want to collect legacy ramdumps
		*/
		if (!(ssr_flag & SSR_OCCURRED))
			continue;
		ssr_flag &= ~SSR_OCCURRED;

		/* check if ramdump exist */
		if (stat(EXT_MDM_DIR, &st) != SUCCESS)
			continue;

		/* get current time */
		if (get_current_timestamp(timestamp, TIMEBUF_SIZE) == NULL) {
			fprintf(stderr, "SSR: get timestamp fail\n");
			break;
		}

		if (ssr_flag & SSR_MSM_AND_QSC) {
			ret = ssr_copy_dump(STR_QSC, EXT_MDM_DIR, qsc_list,
				QSCDUMP_NUM, timestamp);
			if (ret != SUCCESS)
				fprintf(stderr, "SSR: QSC ramdump fail\n");
			else
				fprintf(stderr, "SSR: QSC ramdump complete\n");
			ssr_flag &= ~SSR_MSM_AND_QSC;
		} else {
			if (stat(EXT_MDM2_DIR, &st) == SUCCESS) {
				ret = ssr_copy_dump(STR_QSC, EXT_MDM2_DIR,
					qsc_list, QSCDUMP_NUM, timestamp);
				if (ret == SUCCESS) {
					fprintf(stderr,
						"SSR: QSC ramdump complete\n");
				} else if (ret != DUMP_NUM) {
					fprintf(stderr,
						"SSR: QSC ramdump fail\n");
				}
			}

			ret = ssr_copy_dump(STR_MDM, EXT_MDM_DIR, mdm_list,
				MDMDUMP_NUM, timestamp);
			if (ret != SUCCESS)
				fprintf(stderr, "SSR: MDM ramdump fail\n");
			else
				fprintf(stderr, "SSR: MDM ramdump complete\n");
		}
	}

	return NULL;
}

int open_uevent()
{
	struct sockaddr_nl addr;
	int sz = UEVENT_BUF_SIZE;
	int s;

	memset(&addr, 0, sizeof(addr));
	addr.nl_family = AF_NETLINK;
	addr.nl_groups = 0xffffffff;

	/*
	*	netlink(7) on nl_pid:
	*	If the application sets it to 0, the kernel takes care of
	*	assigning it.
	*	The kernel assigns the process ID to the first netlink socket
	*	the process opens and assigns a unique nl_pid to every netlink
	*	socket that the process subsequently creates.
	*/
	addr.nl_pid = getpid();

	s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
	if(s < 0) {
		fprintf(stderr, "Ramdump: %s socket failed\n", __func__);
		return -1;
	}

	setsockopt(s, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz));

	if(bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		fprintf(stderr, "Ramdump: %s bind failed\n", __func__);
		close(s);
		return -1;
	}

	return s;
}

int uevent_next_event(int fd, char* buffer, int buffer_length)
{
	struct pollfd fds;
	int nr;
	int count;

	while (1) {
		fds.fd = fd;
		fds.events = POLLIN;
		fds.revents = 0;
		nr = poll(&fds, 1, -1);

		if (nr > 0 && fds.revents == POLLIN) {
			count = recv(fd, buffer, buffer_length, 0);
			if (count > 0) {
				return count;
			}
			fprintf(stderr, "Ramdump: %s rcv failed\n", __func__);
		}
	}

	return 0;
}

static void *uevent_mon(void* param)
{
	int i;
	int ufd;
	int count;
	char uevent_buf[UEVENT_BUF_SIZE];

	/* open uevent fd */
	ufd = open_uevent();
	if (ufd < 0)
	{
		fprintf(stderr, "Ramdump: open uevent fd failed\n");
		return NULL;
	}

	while (1)
	{
		/* Listen for user space event */
		count = uevent_next_event(ufd, uevent_buf, UEVENT_BUF_SIZE);
		if (!count)
			break;

		/*
		* Look for a completed MDM restart
		* MDM power down event: set restart flag
		* MDM power up event: post to semaphore
		*/
		for (i = 0; i < MAX_STR_LEN; i++) {
			if (*(uevent_buf + i) == '\0')
				break;
		}
		if (i == MAX_STR_LEN)
			*(uevent_buf + i) = '\0';

		if (strstr(uevent_buf, UEVENT_MDM_SHUTDOWN))
			ssr_flag |= (SSR_OCCURRED | SSR_LOG_START);
		else if (strstr(uevent_buf, UEVENT_MDM_POWERUP))
			sem_post(&ramdump_sem);
	}

	close(ufd);

	return NULL;
}

int generate_log(char *buf, int len)
{
	int ret = SUCCESS;
	int fd;
	char name_buf[FILENAME_SIZE];
	char timestamp[TIMEBUF_SIZE];

	/* get current time */
	if (get_current_timestamp(timestamp, TIMEBUF_SIZE) == NULL) {
		fprintf(stderr, "Ramdump: get timestamp error\n");
		ret = -EINVAL;
		goto err_exit_1;
	}

	/* Assemble output file */
	assemble_name_string(name_buf, FILENAME_SIZE, ramdump.dir,
		LOG_NODES[0]);
	assemble_name_string(name_buf, FILENAME_SIZE, name_buf,
		timestamp);
	assemble_name_string(name_buf, FILENAME_SIZE, name_buf,
		DUMP_TAIL_BIN);

	/* Open the output file */
	fd = open(name_buf, O_WRONLY | O_CREAT, S_IRUSR);
	if (fd < 0) {
		fprintf(stderr, "SSR: %s: open file err\n", __func__);
		ret = -EIO;
		goto err_exit_1;
	}

	/* write to file */
	ret = write(fd, buf, len);
	if (ret < 0) {
		fprintf(stderr, "SSR: write error. fd=%d\n", fd);
		goto err_exit_2;
	}

	/* Make sure things are written */
	fsync(fd);

err_exit_2:
	close(fd);

err_exit_1:
	return ret;
}

static void *rpm_log_thread(void* param)
{
	int ret;
	int count = 0;
	char *read_buf;
	char *rpm_log_buf;
	int rpm_log_len = 0;

	rpm_log_buf = (char *)malloc(RPMLOG_BUF_SIZE);
	if (rpm_log_buf == NULL) {
		fprintf(stderr, "SSR: %s: output buf err\n", __func__);
		return NULL;
	}

	while (1) {
		/* Poll RPM log */
		if ((ret = poll(plogfd, LOG_NUM, -1)) < 0) {
			fprintf(stderr,
				"SSR: %s: poll err: %d\n", __func__, ret);
			break;
		}

		read_buf = malloc(BUFFER_SIZE);
		if (read_buf == NULL) {
			fprintf(stderr, "SSR: %s: read buf err\n", __func__);
			break;
		}

		/* Collect RPM log */
		count = read(plogfd[0].fd, read_buf, BUFFER_SIZE);

		if ((rpm_log_len + count) >= RPMLOG_BUF_SIZE)
			count = RPMLOG_BUF_SIZE - rpm_log_len;
		if (count) {
			memcpy(rpm_log_buf + rpm_log_len, read_buf, count);
			rpm_log_len += count;
		}

		/* Store rpm log while subystem crash */
		if (ssr_flag & SSR_LOG_START) {
			ssr_flag &= ~SSR_LOG_START;
			if (generate_log(rpm_log_buf, rpm_log_len) < 0) {
				fprintf(stderr, "SSR: get rpm log err\n");
				free(read_buf);
				break;
			}
			rpm_log_len = 0;
		}

		if (rpm_log_len >= RPMLOG_BUF_SIZE)
			rpm_log_len = 0;

		free(read_buf);
	}

	free(rpm_log_buf);

	return NULL;
}

int log_init(void)
{
	/* check if log device exist */
	if (access(LOG_LIST[0], F_OK) != SUCCESS)
		return -EIO;

	plogfd[0].fd = open(LOG_LIST[0], O_RDONLY);
	if (plogfd[0].fd < 0) {
		fprintf(stderr, "SSR: open %s err\n", LOG_LIST[0]);
		return -EIO;
	}
	plogfd[0].events = POLLIN;
	plogfd[0].revents = 0;

	return SUCCESS;
}

char *ssr_ramdump_filename(int index, ramdump_s *dump, char *name, char *tm, int type)
{
	strlcpy(name, dump->dir, FILENAME_SIZE);
	strlcat(name, "/", FILENAME_SIZE);
	strlcat(name, dump->dev_list[dump->dev[index]], FILENAME_SIZE);
	strlcat(name, tm, FILENAME_SIZE);
	if (type)
		strlcat(name, DUMP_TAIL_ELF, FILENAME_SIZE);
	else
		strlcat(name, DUMP_TAIL_BIN, FILENAME_SIZE);

	return name;
}

int open_ramdump_fd(ramdump_s *dump)
{
	int i;
	int fd;
	int count = 0;
	char dev_buf[FILENAME_SIZE];

	for (i = 0; i < DUMP_NUM; i++) {
		assemble_name_string(dev_buf, FILENAME_SIZE, DUMP_HEAD_STR,
			dump->dev_list[i]);

		/* check if ramdump devices exist */
		if (access(dev_buf, F_OK) != SUCCESS)
			continue;

		/* open ramdump devices */
		if ((fd = open(dev_buf, O_RDONLY)) < 0) {
			fprintf(stderr, "SSR: open %s err\n", dev_buf);
			continue;
		}

		/* store open fd */
		dump->fd[i] = fd;
		dump->dev[count] = i;
		count++;
	}

	return count;
}

static void ssr_tool_helper(void)
{
	fprintf(stdout, "***********************************************\n");
	fprintf(stdout, "\n Qualcomm Technologies, Inc.\n");
	fprintf(stdout, " Copyright (c) 2011-2013  All Rights Reserved.\n\n");
	fprintf(stdout, " Subsystem Ramdump Apps\n");
	fprintf(stdout, " usage:\n");
	fprintf(stdout, " ./system/bin/subsystem_ramdump [arg1] [arg2]\n");
	fprintf(stdout, " [arg1]: (1/2) Ramdump location\n");
	fprintf(stdout, " [arg2]: (1/0) Enable/disable RPM log\n\n");
	fprintf(stdout, "***********************************************\n");
}

static int parse_args(int argc, char **argv)
{
	int ret = SUCCESS;
	int loc, rpmlog;

	ssr_tool_helper();
	if (argc == 1) {
		/* Storage location */
		fprintf(stdout, "Select ramdump location:\n");
		fprintf(stdout, "1: eMMC: /data/ramdump\n");
		fprintf(stdout, "2: SD card: /sdcard/ramdump\n");
		scanf("%d", &loc);

		/* RPM logging */
		fprintf(stdout, "Enable/disable RPM log:\n");
		fprintf(stdout, "1: Enable RPM log\n");
		fprintf(stdout, "0: Disable RPM log\n");
		scanf("%d", &rpmlog);
	} else if (argc == 2) {
		/* Disable RPM logging by default */
		rpmlog = 0;
		sscanf(argv[1], "%d", &loc);
	} else if (argc == 3) {
		sscanf(argv[1], "%d", &loc);
		sscanf(argv[2], "%d", &rpmlog);
	} else {
		fprintf(stderr, "SSR: too many arguments\n");
		ret = -EINVAL;
		goto err_exit;
	}

	if (loc == 1) {
		ret &= ~SSR_DUMP_SDCARD;
	} else if (loc == 2) {
		ret |= SSR_DUMP_SDCARD;
	} else {
		fprintf(stderr, "SSR: invalid ramdump location\n");
		ret = -EINVAL;
		goto err_exit;
	}

	if (rpmlog == 1) {
		ret |= SSR_LOG_START;
	} else if (rpmlog == 0) {
		ret &= ~SSR_LOG_START;
	} else {
		fprintf(stderr, "SSR: invalid RPM log option\n");
		ret = -EINVAL;
	}

err_exit:
	return ret;
}

int main(int argc, char *argv[])
{
	int ret = SUCCESS;
	int i;
	int rpmlog = 0;
	int pfd_num = 0;
	char timestamp[TIMEBUF_SIZE];
	pthread_t uevent_thread_hdl;
	pthread_t mdm_thread_hdl;
	pthread_t rpm_log_thread_hdl;
	struct sigaction action;

	ret = parse_args(argc, argv);
	if (ret < 0) {
		fprintf(stderr, "SSR: arguments parsing error!\n");
		goto err_exit_1;
	}

	if (ret & SSR_DUMP_SDCARD)
		ramdump.dir = DUMP_SDCARD_DIR;
	else
		ramdump.dir = DUMP_EMMC_DIR;

	if (ret & SSR_LOG_START)
		rpmlog = 1;

	/* Check if you can create dir, otherwise die */
	if (check_folder(ramdump.dir) != SUCCESS) {
		fprintf(stderr, "SSR: create %s\n", ramdump.dir);
		if ((ret = create_folder(ramdump.dir)) != SUCCESS) {
			fprintf(stderr, "SSR: create %s err\n", ramdump.dir);
			goto err_exit_2;
		}
	}

	/* Register signal handlers */
	memset(&action, 0, sizeof(action));
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	action.sa_handler = ssr_ramdump_signal_handler;

	/* Initialize semaphore */
	ret = sem_init(&ramdump_sem, 0, 0);
	if (ret != SUCCESS)
		goto err_exit_2;

	/* init parameters */
	ssr_flag = 0;
	ramdump.dev_list = RAMDUMP_LIST;
	for (i = 0; i < DUMP_NUM; i++) {
		ramdump.fd[i] = -1;
		ramdump.dev[i] = -1;
	}

	/* open fd in ramdump list */
	if ((ret = open_ramdump_fd(&ramdump)) == 0) {
		fprintf(stderr, "Ramdump: No ramdump availble\n");
		goto err_exit_3;
	}

	/* Count poll ramdump fd numbers */
	for (i = 0; i < DUMP_NUM; i++)
	{
		if (ramdump.fd[i] >= 0)
		{
			pfd[pfd_num].fd = ramdump.fd[i];
			pfd[pfd_num].events = POLLIN;
			pfd[pfd_num].revents = 0;
			pfd_num++;
		}
	}

	/* create thread to listen uevent */
	ret = pthread_create(&uevent_thread_hdl, NULL, uevent_mon, NULL);
	if (ret != SUCCESS) {
		fprintf(stderr, "SSR: uevent thread: init error\n");
		goto err_exit_4;
	}

	/* create thread to copy mdm ramdump */
	ret = pthread_create(&mdm_thread_hdl, NULL, mdm_ramdump_mon, NULL);
	if (ret != SUCCESS) {
		fprintf(stderr, "SSR: mdm thread: init error\n");
		goto err_exit_5;
	}

	/* create RPM log thread if required */
	if (rpmlog) {
		ret = log_init();
		if (ret != SUCCESS) {
			goto err_exit_6;
		} else {
			ret = pthread_create(&rpm_log_thread_hdl, NULL,
				rpm_log_thread, NULL);
			if (ret != SUCCESS) {
				fprintf(stderr,
					"SSR: rpm log thread: init error\n");
				goto err_exit_6;
			}
		}
	}

	/* Loop forever and poll */
	while (1)
	{
		/* Poll all ramdump files */
		if ((ret = poll(pfd, pfd_num, -1)) < 0)
		{
			fprintf(stderr, "Ramdump: Polling ramdump failed: %d\n", ret);
			break;
		}

		/* Get current time */
		if (get_current_timestamp(timestamp, TIMEBUF_SIZE) == NULL) {
			fprintf(stderr, "Ramdump: get timestamp error\n");
			break;
		}

		/* Collect ramdumps */
		for (i = 0; i < pfd_num; i++)
		{
			ret = generateRamdump(i, &ramdump, timestamp);
			if (ret < 0)
			{
				fprintf(stderr, "Ramdump: Generate %s failed\n",
					ramdump.dev_list[ramdump.dev[i]]);
				/* If generate failed on a ramdump that is open, just die
				Don't want to keep polling if it keeps erroring */
				goto err_exit_7;
			}
		}
	}

err_exit_7:
	if (rpmlog) {
		ret = ssr_stop_thread(rpm_log_thread_hdl, SIGUSR2);
		close(plogfd[0].fd);
	}

err_exit_6:
	ret = ssr_stop_thread(mdm_thread_hdl, SIGUSR2);

err_exit_5:
	ret = ssr_stop_thread(uevent_thread_hdl, SIGUSR2);

err_exit_4:
	for (i = 0; i < pfd_num; i++)
		close(pfd[i].fd);

err_exit_3:
	sem_destroy(&ramdump_sem);
	ramdump.dev_list = NULL;

err_exit_2:
	ramdump.dir = NULL;

err_exit_1:
	return ret;
}

int generateRamdump(int index, ramdump_s *dump, char *tm)
{
	int ret = SUCCESS;
	int numBytes = 0;
	int totalBytes = 0;
	int fd = -1;
	char *rd_buf;
	char f_name[FILENAME_SIZE];

	if (index < 0 || index >= DUMP_NUM) {
		ret = -EINVAL;
		goto err_exit_1;
	}

	/* Check to see if we have anything to do */
	if ((pfd[index].revents & POLLIN) == 0)
		goto err_exit_1;

	/* Notify rpm thread to write rpm logs once per each SSR */
	switch (dump->dev[index]) {
	case 0: /* adsp */
	case 1: /* dsps */
	case 2: /* gss */
	case 4: /* modem: 8660, 8974 */
	case 5: /* modem: 8960 */
	case 7: /* venus */
	case 8: /* riva */
	case 12: /* pronto */
		ssr_flag |= SSR_LOG_START;
		break;

	case 11: /* smem-modem */
	case 13:
		/* QSC restart has dependency on modem restart */
		ssr_flag |= (SSR_OCCURRED | SSR_MSM_AND_QSC);
		sem_post(&ramdump_sem);
		break;

	default:
		break;
	}

	/* Allocate a buffer for us to use */
	rd_buf = malloc(BUFFER_SIZE);
	if (rd_buf == NULL) {
		ret = -ENOMEM;
		goto err_exit_1;
	}

	/* Read first section of ramdump to determine type */
	while ((numBytes = read(pfd[index].fd, rd_buf, CHK_ELF_LEN)) > 0) {
		*(rd_buf + numBytes) = '\0';
		if (strstr(rd_buf, STR_ELF))
			ssr_ramdump_filename(index, dump, f_name, tm, 1);
		else
			ssr_ramdump_filename(index, dump, f_name, tm, 0);

		/* Open the output file */
		fd = open(f_name, O_WRONLY | O_CREAT, S_IRUSR);
		if (fd < 0) {
			fprintf(stderr, "SSR: open %s error\n", f_name);
			ret = -EIO;
			goto err_exit_2;
		}

		/* Write first section ramdump into file and exit loop */
		ret = write(fd, rd_buf, numBytes);
		if (ret < 0) {
			fprintf(stderr, "SSR: write error. fd=%d\n", fd);
			goto err_exit_3;
		}
		totalBytes += numBytes;
		break;
	}

	/* Read data from the ramdump, and write it into the output file */
	while ((numBytes = read(pfd[index].fd, rd_buf, BUFFER_SIZE)) > 0) {
		ret = write(fd, rd_buf, numBytes);
		if (ret < 0) {
			fprintf(stderr, "SSR: write error. fd=%d\n", fd);
			goto err_exit_3;
		}
		totalBytes += numBytes;
	}

	/* Make sure things are written */
	fsync(fd);

	/* Return the number of bytes written */
	ret = totalBytes;

err_exit_3:
	close(fd);

err_exit_2:
	free(rd_buf);

err_exit_1:
	return ret;
}
