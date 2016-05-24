/*
 * ----------------------------------------------------------------------------
 *  Copyright (c) 2011-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
 *  Qualcomm Technologies Proprietary and Confidential.
 *  ---------------------------------------------------------------------------
 *
 */

#define LOG_TAG "rmt_storage"

#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <linux/ioprio.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "cutils/log.h"
#include "qmi_idl_lib.h"
#include "qmi_csi.h"
#include "remote_storage_v01.h"
#include "qmi_csi_common.h"
#include <private/android_filesystem_config.h>
#include <cutils/properties.h>
#include "common_log.h"

/* General logging */
#define RMTS_LOG(format, args...) \
		LOGI("%s: " format "\n", __func__, ## args)

/* Track internal client ops */
#define RMTS_LOG_CLNT(format, args...) do { \
	if (client && client->cinfo) \
		LOGI("%s: %s: clnt_h=%p " format "\n", \
			__func__, client->path, client->cinfo->clnt, ## args); \
	else \
		LOGI("***" format, ## args); \
	} while (0)

/* Track QCSI requests */
#define RMTS_LOG_REQ(format, args...) do { \
	if (client && client->cinfo) \
		LOGI("%s: %s: clnt_h=%p: req_h=%p msg_id=%d: "format"\n",\
		     __func__, client->path, client->cinfo->clnt, \
		     req_handle, msg_id, ## args); \
	else \
		LOGI("***" format, ## args); \
	} while (0)

#define RMT_STORAGE_CHECK_WORD   0X12345678
#define RMT_STORAGE_MAX_IOVEC_XFR_CNT 5
#define MAX_NUM_CLIENTS 10
#define MAX_PATH_NAME 32
#define MAX_PART_NAME 128

#define MMC_BLK_SDCC_NUM           1

#define WAKE_LOCK_FILE		"/sys/power/wake_lock"
#define WAKE_UNLOCK_FILE	"/sys/power/wake_unlock"

#ifdef ANDROID
  #define MMC_BLOCK_DEV_NAME       "/dev/block/mmcblk0"
#else
  #define MMC_BLOCK_DEV_NAME       "/dev/mmcblk0"
#endif

#define SECTOR_SIZE 	512
#define BOOT_REC_SIG 	0xAA55

#define MAX_MBR_ENTRIES 4
#define MAX_EBR_ENTRIES 2

#define EXTENDED_PARTITION_TYPE	5

#define PROPERTY_BASEBAND		"ro.baseband"
#define PROPERTY_BASEBAND_SIZE          PROPERTY_VALUE_MAX
#define PROPERTY_CTLSTOP		"ctl.stop"
#define BASEBAND_VALUE_MSM		"msm"
#define BASEBAND_VALUE_APQ		"apq"
#define BASEBAND_VALUE_SGLTE		"sglte"
#define BASEBAND_VALUE_DSDA3		"dsda3"
#define BASEBAND_VALUE_UNDEFINED	"undefined"
#define SERVICE_NAME			LOG_TAG

#define UIO_QCOM_VER	"1.0"
#define UIO_QCOM_NAME	"rmtfs"

#define UIO_DEV		"/dev/uio%d"
#define UIO_CLASS	"/sys/class/uio"
#define UIO_NAME	"/sys/class/uio/uio%d/name"
#define UIO_VER		"/sys/class/uio/uio%d/version"
#define UIO_MAP0	"/sys/class/uio/uio%d/maps/map0"
#define UIO_ADDR	"/sys/class/uio/uio%d/maps/map0/addr"

int uio_num = 0;

extern int ioprio_set(int which, int who, int ioprio);

struct partition_lookup_entry {
	uint8_t partition_type;
	char part_name[MAX_PART_NAME];
	char path[MAX_PATH_NAME];
	char devpath[MAX_PATH_NAME];
	int fd;
};

struct partition_entry {
	uint8_t status;
	uint8_t rsvd0[3];
	uint8_t type;
	uint8_t rsvd1[3];
	uint32_t start_sector;
	uint32_t partition_size;
} __attribute__((__packed__));

struct boot_rec {
	uint8_t rsvd0[446];
	struct partition_entry part_entry[MAX_MBR_ENTRIES];
	uint16_t sig;
} __attribute__((__packed__));

int dev_mem_fd;
static uint32_t rmt_shrd_mem_phy_addr;
void *rmt_shrd_mem_base = NULL; /* virtual address for modem_fs1, modem_fs2 and modem_fsg partitions */

struct rmt_shrd_mem_param {
	uint32_t start;		/* Physical memory address */
	void *base;		/* Virtual user-space memory address */
};

struct rmt_storage_iovec_desc {
	uint32_t sector_addr;
	uint32_t data_phy_addr;
	uint32_t num_sector;
};

struct rmt_storage_cinfo {
	uint32_t is_client_ready;
	qmi_client_handle clnt;
};

struct rmt_storage_client {
	int fd;
	uint32_t check_word;
	pthread_t th_id;
	pthread_mutex_t th_mutex;
	pthread_cond_t cond;
	unsigned close;
	char path[MAX_PATH_NAME];
	struct rmt_storage_iovec_desc xfer_desc[RMT_STORAGE_MAX_IOVEC_XFR_CNT];
	uint32_t xfer_dir;
	uint32_t xfer_cnt;
	uint32_t error_code;
	struct rmt_shrd_mem_param shrd_mem;
	int msg_id;
	qmi_req_handle req_handle;
	struct rmt_storage_cinfo *cinfo;
	char wakelock_name[MAX_PATH_NAME];
	uint32_t wakelock_strlen;
};

struct rmt_storage_svc {
	qmi_csi_service_handle service_handle;
};

static struct rmt_storage_svc svc;
static struct rmt_storage_client clients[MAX_NUM_CLIENTS];

static int wakelock_fd, wakeunlock_fd;
static pthread_mutex_t wakelock_mutex, wakeunlock_mutex;

static int rmt_storage_init_wakelock(void)
{
	/* Open wakelock files */
	wakelock_fd = open(WAKE_LOCK_FILE, O_WRONLY|O_APPEND);
	if(wakelock_fd < 0) {
		LOGE("Unable to open %s file: return val: %d, error no: %d\n",
			WAKE_LOCK_FILE, wakelock_fd, errno);
		if(errno == ENOENT) {
			LOGI("No wakelock support and so not using them.\n");
			wakeunlock_fd = -1;
			return 0;
		}
		return -1;
	} else {
		wakeunlock_fd = open(WAKE_UNLOCK_FILE, O_WRONLY|O_APPEND);
		if(wakeunlock_fd < 0) {
			LOGE("Unable to open %s file: return val: %d, error "
				"no: %d\n", WAKE_UNLOCK_FILE,
				wakelock_fd, errno);
			close(wakelock_fd);
			return -1;
		}
	}

	pthread_mutex_init(&wakelock_mutex, NULL);
	pthread_mutex_init(&wakeunlock_mutex, NULL);
	return 0;
}

static struct partition_lookup_entry part_lookup_table[] = {
	{0x4A, "modemst1", "/boot/modem_fs1", "", -1},
	{0x4B, "modemst2", "/boot/modem_fs2", "", -1},
	{0x58, "fsg",  "/boot/modem_fsg", "", -1},
	{0x59, "", "/q6_fs1_parti_id_0x59", "", -1},
	{0x5A, "", "/q6_fs2_parti_id_0x5A", "", -1},
	{0x5B, "", "/q6_fsg_parti_id_0x5B", "", -1},
	{0x5D, "ssd", "ssd", "", -1},
	{0xFF, "fsc", "/boot/modem_fsc", "", -1},
};

static struct partition_lookup_entry *find_partition_entry_by_type(
					uint8_t part_type)
{
   unsigned int i;
   for (i = 0;
        i < sizeof(part_lookup_table)/sizeof(struct partition_lookup_entry);
        i++)
            if (part_lookup_table[i].partition_type == part_type)
               return &part_lookup_table[i];
   return NULL;
}

static int open_partition(char *path)
{
   unsigned int i;

   if (!path)
      return -1;

   for (i = 0;
        i < sizeof(part_lookup_table)/sizeof(struct partition_lookup_entry);
        i++)
		if (!strncmp(part_lookup_table[i].path, path, MAX_PATH_NAME))
			return part_lookup_table[i].fd;

   return -1;
}

static qmi_csi_cb_error rmt_storage_connect_cb (qmi_client_handle client_handle,
				void *service_cookie, void **connection_handle)
{
	struct rmt_storage_cinfo *cinfo;

	cinfo = malloc(sizeof(struct rmt_storage_cinfo));
	if (!cinfo)
		return QMI_CSI_CB_NO_MEM;
	cinfo->clnt = client_handle;
	cinfo->is_client_ready = 1;
	*connection_handle = cinfo;
	RMTS_LOG("clnt_h=%p conn_h=%p", cinfo->clnt, cinfo);
	return QMI_CSI_CB_NO_ERR;
}

static struct rmt_storage_client *rmt_storage_init_client(void)
{
	int i;

	for (i = 0; i < MAX_NUM_CLIENTS; i++) {
		if (clients[i].check_word != RMT_STORAGE_CHECK_WORD)
			break;
	}

	if (i == MAX_NUM_CLIENTS) {
		RMTS_LOG("Maximum number of clients reached");
		return NULL;
	}

	clients[i].check_word = RMT_STORAGE_CHECK_WORD;
	clients[i].close = 0;
	pthread_mutex_init(&clients[i].th_mutex, NULL);
	pthread_cond_init(&clients[i].cond, NULL);
	return &clients[i];
}

static void rmt_storage_free_client(struct rmt_storage_client *client)
{
	client->check_word = 0;
	pthread_mutex_destroy(&client->th_mutex);
	pthread_cond_destroy(&client->cond);
}

static struct rmt_storage_client *rmt_storage_get_client(int handle)
{
	int i;

	for (i = 0; i < MAX_NUM_CLIENTS; i++) {
		if (clients[i].fd == handle)
			break;
	}
	if (i == MAX_NUM_CLIENTS) {
		RMTS_LOG("No client found for hndl=%d", handle);
		return NULL;
	}
	return &clients[i];
}

static void * rmt_storage_client_thread(void *data)
{
	uint32_t i;
	struct rmt_storage_iovec_desc *xfer;
	ssize_t ret = 0;
	struct rmt_storage_client *client = data;
	char *buf;
	rmtfs_rw_iovec_resp_msg_v01 rw_resp;
	qmi_csi_error resp_err;
	int msg_id;
	qmi_req_handle req_handle;

	RMTS_LOG_CLNT("Worker thread started");

	ret = snprintf(client->wakelock_name, sizeof(client->wakelock_name),
			"rmt_storage_%ld", (long)(client->th_id));
	LOGI("wake lock name: %s, name creation success: %d\n",
		client->wakelock_name, ret);
	client->wakelock_strlen = strlen(client->wakelock_name);

	pthread_mutex_lock(&client->th_mutex);
	while(1) {
		req_handle = client->req_handle;
		msg_id = client->msg_id;

		if (client->close)
			break;

		if(client->xfer_cnt && !client->shrd_mem.base) {
			RMTS_LOG_CLNT("No shared memory found");
			break;
		}

		for(i=0; i<client->xfer_cnt; i++) {
			xfer = &client->xfer_desc[i];
			lseek(client->fd, xfer->sector_addr * (off_t)512, SEEK_SET);
			buf = (char *) ((uint32_t) client->shrd_mem.base +
					(xfer->data_phy_addr - client->shrd_mem.start));

			if(client->xfer_dir == RMTFS_DIRECTION_WRITE_V01)
				ret = write(client->fd, buf, xfer->num_sector * 512);
			else if (client->xfer_dir == RMTFS_DIRECTION_READ_V01)
				ret = read(client->fd, buf, xfer->num_sector * 512);

			if ((int) ret < 0) {
				RMTS_LOG_REQ("%s failed. Err=%d",
				        (client->xfer_dir == RMTFS_DIRECTION_WRITE_V01) ?
				        "Write" : "Read", (int) ret);
				break;
			}
			RMTS_LOG_REQ("Bytes %s = %d",
				      (client->xfer_dir == RMTFS_DIRECTION_WRITE_V01) ?
				      "written" : "read", (int) ret);
		}
		if (client->xfer_cnt) {
			client->error_code = ((int) ret > 0) ? 0 : ret;
			memset(&rw_resp, 0, sizeof(rmtfs_rw_iovec_resp_msg_v01));
			if (client->error_code) {
				rw_resp.resp.result = QMI_RESULT_FAILURE;
				rw_resp.resp.error = QMI_ERR_INTERNAL;
			}
			RMTS_LOG_REQ("Send response: res=%d err=%d",
				     rw_resp.resp.result, rw_resp.resp.error);
			resp_err = qmi_csi_send_resp(client->req_handle, client->msg_id, &rw_resp, sizeof(rw_resp));
			if(resp_err != QMI_CSI_NO_ERR)
				RMTS_LOG_REQ("qmi_csi_send_resp returned error %d",
					     resp_err);
			client->xfer_cnt = 0;

			if(wakeunlock_fd > 0) {
				pthread_mutex_lock(&wakeunlock_mutex);
				ret = write(wakeunlock_fd, client->wakelock_name,
						client->wakelock_strlen);
				pthread_mutex_unlock(&wakeunlock_mutex);
				RMTS_LOG_CLNT("About to block rmt_storage client thread "
					"(th_id: %ld) wakelock released: %d, error no: %d\n",
					client->th_id, (ret == client->wakelock_strlen), errno);
			} else {
				RMTS_LOG_CLNT("About to block rmt_storage client thread "
					"(th_id: %ld)\n", client->th_id);
			}

		}

		/* Wait for subsequent events and process them */
		pthread_cond_wait(&client->cond, &client->th_mutex);
		RMTS_LOG_CLNT("Unblock worker thread (th_id: %ld)",
				client->th_id);
	}
	pthread_mutex_unlock(&client->th_mutex);
	/* free client structure */
	rmt_storage_free_client(client);
	RMTS_LOG_CLNT("Worker thread exiting");

	return NULL;
}

static qmi_csi_cb_error rmt_storage_open_cb (struct rmt_storage_cinfo *clnt_info,
				qmi_req_handle req_handle, int msg_id,
				void *req_c_struct, int req_c_struct_len,
				void *service_cookie)
{
	qmi_csi_cb_error rc = QMI_CSI_CB_INTERNAL_ERR;
	qmi_csi_error resp_err;
	rmtfs_open_resp_msg_v01 open_resp;
	rmtfs_open_req_msg_v01 *data;
	struct rmt_storage_client *client;
	int ret, i;

	memset(&open_resp, 0, sizeof(rmtfs_open_resp_msg_v01));
	data = (rmtfs_open_req_msg_v01*)req_c_struct;

	for (i = 0; i < MAX_NUM_CLIENTS; i++) {
		if (clients[i].check_word == RMT_STORAGE_CHECK_WORD) {
			if (clients[i].fd > 0 && !strcmp(clients[i].path, data->path)) {
				/* This must be subsystem restart case where
				 * open can be called twice without a close
				 * during subsystem restart
				 */
				client = &clients[i];
				client->cinfo = clnt_info;
				goto skip_init;
			}
		}
	}

	client = rmt_storage_init_client();
	if (!client || client->check_word != RMT_STORAGE_CHECK_WORD) {
		RMTS_LOG("Invalid rmt_storage client for %s", data->path);
		goto err;
	}
	client->cinfo = clnt_info;

	client->fd = open_partition(data->path);
	if (client->fd < 0) {
		RMTS_LOG("Unable to open %s", data->path);
		rmt_storage_free_client(client);
		goto err;
	}

	strlcpy(client->path, data->path, MAX_PATH_NAME);
	client->shrd_mem.base = NULL;
	ret = pthread_create(&client->th_id, NULL, rmt_storage_client_thread, (void *)client);
	if (ret) {
		RMTS_LOG_CLNT("Unable to create a pthread");
		close(client->fd);
		rmt_storage_free_client(client);
		goto err;
	}

skip_init:
	RMTS_LOG_REQ("Client found");
	open_resp.caller_id_valid = 1;
	open_resp.caller_id = client->fd;
	goto send_resp;
err:
	open_resp.resp.result = QMI_RESULT_FAILURE;
	open_resp.resp.error = QMI_ERR_INTERNAL;
send_resp:
	RMTS_LOG_REQ("Send response: res=%d err=%d", open_resp.resp.result, open_resp.resp.error);
	resp_err = qmi_csi_send_resp(req_handle, msg_id, &open_resp, sizeof(open_resp));
	if(resp_err != QMI_CSI_NO_ERR) {
		RMTS_LOG_REQ("qmi_csi_send_resp returned error %d", resp_err);
	} else {
		rc = QMI_CSI_CB_NO_ERR;
	}
	return rc;
}

static qmi_csi_cb_error rmt_storage_rw_iovec_cb (struct rmt_storage_cinfo *clnt_info,
				qmi_req_handle req_handle, int msg_id,
				void *req_c_struct, int req_c_struct_len,
				void *service_cookie)
{
	qmi_csi_cb_error rc = QMI_CSI_CB_INTERNAL_ERR;
	qmi_csi_error resp_err;
	rmtfs_rw_iovec_req_msg_v01 *data;
	struct rmt_storage_client *client;
	struct rmt_storage_iovec_desc *xfer;
	rmtfs_iovec_desc_type_v01 *desc_args;
	rmtfs_rw_iovec_resp_msg_v01 rw_resp;
	uint32_t i;
	ssize_t ret = 0;

	memset(&rw_resp, 0, sizeof(rmtfs_rw_iovec_resp_msg_v01));
	data = (rmtfs_rw_iovec_req_msg_v01*)req_c_struct;
	client = rmt_storage_get_client(data->caller_id);
	if (!client || client->check_word != RMT_STORAGE_CHECK_WORD) {
		RMTS_LOG("Invalid rmt_storage client");
		goto err;
	}
	RMTS_LOG_REQ("R/W request received");

	pthread_mutex_lock(&client->th_mutex);
	if (!client->shrd_mem.base) {
		if(!strcmp(client->path,"/boot/modem_fs1") ||
			!strcmp(client->path,"/boot/modem_fs2") ||
			!strcmp(client->path,"/boot/modem_fsg") ||
			!strcmp(client->path,"/boot/modem_fsc")) {
			client->shrd_mem.start = rmt_shrd_mem_phy_addr;
			client->shrd_mem.base = rmt_shrd_mem_base;
			LOGI("Shared mem address set to 0x%08x\n", client->shrd_mem.base);
		}
	}

	client->xfer_dir = data->direction;
	client->xfer_cnt = data->iovec_struct_len;
	for (i = 0; i < data->iovec_struct_len; i++) {
		xfer = &client->xfer_desc[i];
		desc_args = &data->iovec_struct[i];
		xfer->sector_addr = desc_args->sector_addr;
		xfer->data_phy_addr = desc_args->data_phy_addr_offset;
		xfer->num_sector = desc_args->num_sector;
	}

	client->msg_id = msg_id;
	client->req_handle = req_handle;

	if(wakelock_fd > 0) {
		pthread_mutex_lock(&wakelock_mutex);
		ret = write(wakelock_fd, client->wakelock_name,
				client->wakelock_strlen);
		pthread_mutex_unlock(&wakelock_mutex);
		LOGI("wakelock acquired: %d, error no: %d\n",
			(ret == client->wakelock_strlen), errno);
	}

	pthread_cond_signal(&client->cond);
	pthread_mutex_unlock(&client->th_mutex);
	return QMI_CSI_CB_NO_ERR;
err:
	RMTS_LOG_REQ("Send response: res=%d err=%d", rw_resp.resp.result, rw_resp.resp.error);
	rw_resp.resp.result = QMI_RESULT_FAILURE;
	rw_resp.resp.error = QMI_ERR_INTERNAL;

	resp_err = qmi_csi_send_resp(req_handle, msg_id, &rw_resp, sizeof(rw_resp) );
	if(resp_err != QMI_CSI_NO_ERR) {
		RMTS_LOG_REQ("qmi_csi_send_resp returned error %d", resp_err);
		return rc;
	 } else {
		return QMI_CSI_CB_NO_ERR;
	 }
}

static qmi_csi_cb_error rmt_storage_close_cb (struct rmt_storage_cinfo *clnt_info,
				qmi_req_handle req_handle, int msg_id,
				void *req_c_struct, int req_c_struct_len,
				void *service_cookie)
{
	qmi_csi_cb_error rc = QMI_CSI_CB_INTERNAL_ERR;
	qmi_csi_error resp_err;
	rmtfs_close_req_msg_v01 *data;
	rmtfs_close_resp_msg_v01 close_resp;
	struct rmt_storage_client *client;

	memset(&close_resp, 0, sizeof(rmtfs_rw_iovec_resp_msg_v01));
	data = (rmtfs_close_req_msg_v01*)req_c_struct;
	client = rmt_storage_get_client(data->caller_id);
	if (!client || client->check_word != RMT_STORAGE_CHECK_WORD) {
		RMTS_LOG("Invalid rmt_storage client");
		goto err;
	}
	RMTS_LOG_REQ("Close request received");

	pthread_mutex_lock(&client->th_mutex);
	client->close = 1;
	close(client->fd);
	pthread_cond_signal(&client->cond);
	pthread_mutex_unlock(&client->th_mutex);
	goto send_resp;
err:
	close_resp.resp.result = QMI_RESULT_FAILURE;
	close_resp.resp.error = QMI_ERR_INTERNAL;
send_resp:
	RMTS_LOG_REQ("Send response: res=%d err=%d", close_resp.resp.result, close_resp.resp.error);
	resp_err = qmi_csi_send_resp(req_handle, msg_id, &close_resp, sizeof(close_resp) );
	if(resp_err != QMI_CSI_NO_ERR)
		RMTS_LOG_REQ("qmi_csi_send_resp returned error %d", resp_err);
	else
		rc =  QMI_CSI_CB_NO_ERR;

	return rc;
}

static qmi_csi_cb_error rmt_storage_alloc_buff_cb (struct rmt_storage_cinfo *clnt_info,
				qmi_req_handle req_handle, int msg_id,
				void *req_c_struct, int req_c_struct_len,
				void *service_cookie)
{
	qmi_csi_cb_error rc = QMI_CSI_CB_INTERNAL_ERR;
	qmi_csi_error resp_err;
	rmtfs_alloc_buff_req_msg_v01 *data;
	rmtfs_alloc_buff_resp_msg_v01 alloc_buff_resp;
	struct rmt_storage_client *client;

	data = (rmtfs_alloc_buff_req_msg_v01*)req_c_struct;
	client = rmt_storage_get_client(data->caller_id);
	memset(&alloc_buff_resp, 0, sizeof(rmtfs_rw_iovec_resp_msg_v01));

	if (!client || client->check_word != RMT_STORAGE_CHECK_WORD) {
		RMTS_LOG("Invalid rmt_storage client");
		goto err;
	}
	RMTS_LOG_REQ("Alloc request received: Size: %d", data->buff_size);

	if(!strcmp(client->path,"/boot/modem_fs1")) {
		off_t shrd_mem_start = rmt_shrd_mem_phy_addr;
		if (!rmt_shrd_mem_base) {
			rmt_shrd_mem_base = mmap(0, data->buff_size,
				PROT_READ | PROT_WRITE, MAP_SHARED,
				dev_mem_fd, 0);
			if (rmt_shrd_mem_base == MAP_FAILED) {
				RMTS_LOG_REQ("Mmap on 0x%x failed (%d)",
					shrd_mem_start, errno);
				goto err;
			}
		}
		alloc_buff_resp.buff_address_valid = 1;
		alloc_buff_resp.buff_address = shrd_mem_start;
	}
	goto send_resp;
err:
	alloc_buff_resp.resp.result = QMI_RESULT_FAILURE;
	alloc_buff_resp.resp.error = QMI_ERR_INTERNAL;
send_resp:
	RMTS_LOG_REQ("Send response: res=%d err=%d", alloc_buff_resp.resp.result, alloc_buff_resp.resp.error);
	resp_err = qmi_csi_send_resp(req_handle, msg_id, &alloc_buff_resp, sizeof(alloc_buff_resp) );
	if(resp_err != QMI_CSI_NO_ERR)
		RMTS_LOG_REQ("qmi_csi_send_resp returned error %d", resp_err);
	else
		rc =  QMI_CSI_CB_NO_ERR;

	return rc;
}

static void rmt_storage_disconnect_cb(void *connection_handle, void *service_cookie)
{
	struct rmt_storage_cinfo *cinfo = connection_handle;

	RMTS_LOG("clnt_h=0x%p conn_h=0x%p", cinfo->clnt, cinfo);
	if (connection_handle)
		free(connection_handle);
}

static qmi_csi_cb_error (* const req_handle_table[])
(
 struct rmt_storage_cinfo        *clnt_info,
 qmi_req_handle           req_handle,
 int                      msg_id,
 void                     *req_c_struct,
 int                      req_c_struct_len,
 void                     *service_cookie
) =
{
	NULL,
	&rmt_storage_open_cb,
	&rmt_storage_close_cb,
	&rmt_storage_rw_iovec_cb,
	&rmt_storage_alloc_buff_cb,
	//&rmt_storage_get_dev_err_cb,
	//&rmt_storage_force_sync
};

static qmi_csi_cb_error rmt_storage_handle_req_cb (void *connection_handle,
				qmi_req_handle req_handle, int msg_id,
				void *req_c_struct, int req_c_struct_len,
				void *service_cookie)
{
	qmi_csi_cb_error rc = QMI_CSI_CB_INTERNAL_ERR;
	struct rmt_storage_cinfo *cinfo = (struct rmt_storage_cinfo*)connection_handle;
	struct rmt_storage_svc *svc = (struct rmt_storage_svc*) service_cookie;

	if (!cinfo || !cinfo->is_client_ready) {
		RMTS_LOG("Invalid client");
		return rc;
	}

	if((uint32_t) msg_id < (sizeof(req_handle_table) / sizeof(*req_handle_table))) {
		if(req_handle_table[msg_id]) {
			rc = req_handle_table[msg_id] (cinfo,req_handle,
					msg_id,req_c_struct, req_c_struct_len,
					service_cookie);
		} else {
			RMTS_LOG("No function defined to handle request for message ID: %d",
				 msg_id);
		}
	} else {
		RMTS_LOG("Message ID: %d greater than maximum known message ID", msg_id);
	}

	return rc;
}

static int is_valid_boot_rec(const void *boot_rec)
{
   const struct boot_rec *br = boot_rec;

   if (br->sig != BOOT_REC_SIG) {
      LOGI("Invalid boot rec\n");
      return 0;
   }
   return 1;
}

static int read_partition_rec(int fd, void *buf, loff_t sector_offset, size_t size)
{
   loff_t ret, offset;
   ssize_t num_bytes;
   int i;

   if (!buf)
      return -1;
   offset = sector_offset * SECTOR_SIZE;
   ret = lseek64(fd, offset, SEEK_SET);
   if (ret < 0 || ret != offset) {
      LOGI("Error seeking 0x%llx bytes in partition. ret=0x%llx errno=%d\n",
           offset, ret, errno);
      return -1;
   }
   num_bytes = read(fd, buf, size);
   if ((num_bytes < 0) || ((size_t)num_bytes != size)) {
      LOGI("Error reading 0x%x bytes < 0x%x\n",
           (unsigned int)num_bytes, size);
      return -1;
   }
   if (!is_valid_boot_rec(buf))
      return -1;
   return 0;
}

static int parse_gpt_partition(void)
{
	struct partition_lookup_entry *part_entry;
	int fd, part_num;
	int parts_found = 0;
	char part_path[MAX_PART_NAME];
	unsigned int i;

	for (i = 0; i < sizeof(part_lookup_table) /
		sizeof(struct partition_lookup_entry); i++) {

		part_entry = &part_lookup_table[i];
		if (!part_entry->part_name[0])
			continue;
		snprintf(part_path, MAX_PART_NAME, "/dev/block/platform/msm_sdcc.%d/by-name/%s",
			MMC_BLK_SDCC_NUM, part_entry->part_name);

		fd = open(part_path, O_RDWR|O_SYNC);
		if (fd < 0)
			continue;

		part_entry->fd = fd;
		LOGI("Registering %s: 0x%x %s\n",
			part_entry->part_name,
			part_entry->partition_type,
			part_entry->path);
		parts_found++;
	}
	return parts_found;
}

static int parse_mbr_partition(const char *path)
{
   struct boot_rec mbr;
   struct partition_lookup_entry *part_entry;
   int fd, i, part_num, part_type, parse_done, ret, parts_found;
   off_t ebr_offset, local_ebr_offset;

   if (!path)
      return -1;
   if (sizeof(mbr) != SECTOR_SIZE) {
      LOGI("MBR struct is not %d bytes\n", SECTOR_SIZE);
      return -1;
   }
   fd = open(path, O_RDONLY);
   if (fd < 0) {
      LOGI("Unable to open %s\n", path);
      return -1;
   }
   ret = read_partition_rec(fd, &mbr, 0, sizeof(mbr));
   if (ret < 0) {
      close(fd);
      return -1;
   }
   for (i = 0; i < MAX_MBR_ENTRIES; i++) {
      if (mbr.part_entry[i].type == EXTENDED_PARTITION_TYPE) {
         part_num = i + 1;
         ebr_offset = mbr.part_entry[i].start_sector;
         break;
      }
   }
   if (i == MAX_MBR_ENTRIES) {
      LOGI("No EBR found\n");
      close(fd);
      return -1;
   }
   parse_done = 0;
   parts_found = 0;
   local_ebr_offset = 0;
   do {
      ret = read_partition_rec(fd, &mbr, ebr_offset + local_ebr_offset,
                               sizeof(mbr));
      if (ret < 0) {
         close(fd);
         return parts_found;
      }
      for (i = 0; i < MAX_EBR_ENTRIES; i++) {
         part_type = mbr.part_entry[i].type;
         if (!part_type) {
            parse_done = 1;
            break;
         }
         if (part_type == EXTENDED_PARTITION_TYPE) {
            local_ebr_offset = mbr.part_entry[i].start_sector;
            break;
         }
         part_num++;
         part_type = mbr.part_entry[i].type;
         part_entry = find_partition_entry_by_type(part_type);
         if (part_entry) {
            parts_found++;
            snprintf(part_entry->devpath,
                     MAX_PATH_NAME,
                     "%sp%d", MMC_BLOCK_DEV_NAME, part_num);
	    part_entry->fd = open(part_entry->devpath, O_RDWR|O_SYNC);

	    if (part_entry->fd < 0)
		LOGI("Unable to open %s\n", part_entry->devpath);
            LOGV("Registering p%d: 0x%x %s %s\n", part_num,
                 part_entry->partition_type,
                 part_entry->path,
                 part_entry->devpath);
         }
      }
   } while(!parse_done);

   close(fd);
   return parts_found;
}

static uint32_t get_phys_addr(void)
{
	char cwd[50], name[10], version[10], addr[11], tmp[50], addr_dir[50];
	int found_name, found_ver, fgets_ret;
	DIR *uiodir, *xdir, *mapdir;
	FILE *fd;
	struct dirent *uiodirent, *xdirent, *mapdirent;
	uint32_t shrd_mem_phy_addr = 0;

	if (!getcwd(cwd, sizeof(cwd))) {
		LOGE("Unable to get cwd\n");
		goto out;
	}
	/* Need to change dir to read entries */
	if (chdir(UIO_CLASS) < 0) {
		LOGE("Unable to change dir\n");
		goto out;
	}
	uiodir = opendir(UIO_CLASS);
	if(!uiodir) {
		LOGE("Unable to open %s\n", UIO_CLASS);
		goto err;
	}
	/* Parse through every UIO device under /sys/class/uio */
	while(uiodirent = readdir(uiodir)) {
		if (strstr((const char *)uiodirent->d_name, "uio") == NULL)
			continue;
		xdir = opendir(uiodirent->d_name);
		if(!xdir) {
			LOGE("Unable to open %s\n", uiodirent->d_name);
			goto err_xdir;
		}
		while(xdirent = readdir(xdir)) {
			/*
			 * Look for the name and version entries under /sys/class/uio/uio'X'
			 * If they are a match then this is the right UIO device for this client
			 */
			if (!strcmp(xdirent->d_name, "name")) {
				snprintf(tmp, strlen(UIO_NAME), UIO_NAME, uio_num);
				fd = fopen(tmp, "r");
				if (!fd) {
					LOGE("Unable to open file %s\n", tmp);
					goto err_fopen;
				}
				fgets_ret = fgets(name, strlen(UIO_QCOM_NAME) + 1 , fd);
				if (!fgets_ret)
					LOGE("fgets failed with %d for %s\n", ferror(fd), tmp);
				if(!strcasecmp(name, UIO_QCOM_NAME))
					found_name = 1;
				fclose(fd);
			} else if (!strcmp(xdirent->d_name, "version")) {
				snprintf(tmp, strlen(UIO_VER), UIO_VER, uio_num);
				fd = fopen(tmp, "r");
				if (!fd) {
					LOGE("Unable to open file %s\n", tmp);
					goto err_fopen;
				}
				fgets_ret = fgets(version, strlen(UIO_QCOM_VER) + 1, fd);
				if (!fgets_ret)
					LOGE("fgets failed with %d for %s\n", ferror(fd), tmp);
				if(!strcmp(version, UIO_QCOM_VER))
					found_ver = 1;
				fclose(fd);
				break;
			}
		}
		if (found_name && found_ver) {
			/*
			 * Now we have the required device under /sys/class/uio/uio'X'
			 * This device is available at /dev/uio'X'
			 * Acquire the physical address which could be memory mapped for this device node
			 * from /sys/class/uio/uio'X'/map0/map/addr
			 */
			snprintf(addr_dir, strlen(UIO_MAP0), UIO_MAP0, uio_num);
			mapdir = opendir(addr_dir);
			if(!mapdir) {
				LOGE("Unable to open %s\n", addr_dir);
				goto err_fopen;
			}
			while(mapdirent = readdir(mapdir)) {
				if (!strcmp(mapdirent->d_name, "addr")) {
					snprintf(tmp, 50, UIO_ADDR, uio_num);
					fd = fopen(tmp, "r");
					if (!fd) {
						LOGE("Unable to open file %s\n", tmp);
						goto err_mapdir;
					}
					fgets_ret = fgets(addr, 11, fd);
					if (!fgets_ret)
						LOGE("fgets failed with %d for %s\n",ferror(fd), tmp);
					fclose(fd);
					break;
				}
			}
			break;
		} else {
			uio_num++;
			found_name = found_ver = 0;
		}
	}

	if (!(found_name && found_ver)) {
		LOGE("Matching UIO shared mem device not found for %s\n", UIO_QCOM_NAME);
		goto err;
	}
	shrd_mem_phy_addr = strtoul(addr, NULL, 16);
	LOGV("shrd_mem_phy_addr:0x%x\n", shrd_mem_phy_addr);
err_mapdir:
	closedir(mapdir);
err_fopen:
	closedir(xdir);
err_xdir:
	closedir(uiodir);
err:
	chdir(cwd);
out:
	return shrd_mem_phy_addr;
}

int main(int argc, char **argv)
{
	qmi_csi_os_params os_params, os_params_in;
	fd_set fds;
	int ret;
	char args[PROPERTY_BASEBAND_SIZE];
	char def[PROPERTY_BASEBAND_SIZE];
	char dev_path[50];

	(void)strlcpy(def, BASEBAND_VALUE_UNDEFINED, PROPERTY_BASEBAND_SIZE);
	memset(args, 0, sizeof(args));
	ret = property_get(PROPERTY_BASEBAND, args, def);
	if ((ret > 0) && (ret <= PROPERTY_BASEBAND_SIZE)) {
		/* RemoteFS service is not supported on targets that have off chip modem */
		if(strncmp(args, BASEBAND_VALUE_MSM, sizeof(BASEBAND_VALUE_MSM)) &&
		   strncmp(args, BASEBAND_VALUE_APQ, sizeof(BASEBAND_VALUE_APQ)) &&
		   strncmp(args, BASEBAND_VALUE_DSDA3, sizeof(BASEBAND_VALUE_DSDA3)) &&
		   strncmp(args, BASEBAND_VALUE_SGLTE, sizeof(BASEBAND_VALUE_SGLTE))) {
			LOGE("Remote storage service is not supported on %s target\n", args);
			ret = property_set(PROPERTY_CTLSTOP, SERVICE_NAME);
			exit(1);
		}
	}

	ret = ioprio_set(IOPRIO_WHO_PROCESS, 0, IOPRIO_CLASS_RT << IOPRIO_CLASS_SHIFT);
	if (ret < 0)
		LOGE("Error setting io priority to CLASS_RT (%d)\n", errno);

	ret = rmt_storage_init_wakelock();
	if (ret)
		exit(1);

	qmi_idl_service_object_type rmt_storage_service_object = rmtfs_get_service_object_v01();
	qmi_csi_error rc = QMI_CSI_INTERNAL_ERR;
	rmt_shrd_mem_phy_addr = get_phys_addr();
	if(!rmt_shrd_mem_phy_addr) {
		LOGE("Failed in getting the physical address for shared mem\n");
		exit(1);
	}

	/* Open /dev/uioX for mmap later */
	snprintf(dev_path, 50, UIO_DEV, uio_num);
	LOGV("dev_path:%s\n", dev_path);

	dev_mem_fd = open(dev_path, O_RDWR | O_DSYNC);
	if (dev_mem_fd < 0) {
		LOGE("Unable to open /dev/uio%d\n", uio_num);
		exit(1);
	}

	ret = parse_mbr_partition(MMC_BLOCK_DEV_NAME);
	if (ret < 0) {
		LOGI("Error (%d) parsing MBR partitions\n", ret);
		ret = parse_gpt_partition();
		if (ret < 0) {
			LOGI("Error (%d) parsing partitions\n", ret);
			exit(1);
		} else {
			LOGI("%d GPT partitions found\n", ret);
		}
	} else {
		LOGI("%d MBR partitions found\n", ret);
	}

	ret = setgid(AID_NET_RAW);
	if (ret < 0) {
		LOGI("Error changing gid (%d)\n", ret);
		exit(1);
	}

	ret = setuid(AID_NOBODY);
	if (ret < 0) {
		LOGI("Error changing uid (%d)\n", ret);
		exit(1);
	}

	rc = qmi_csi_register(rmt_storage_service_object, rmt_storage_connect_cb,
			rmt_storage_disconnect_cb, rmt_storage_handle_req_cb,
			&svc, &os_params, &svc.service_handle);
	if(rc != QMI_NO_ERR) {
		LOGI("Unable to register service!\n");
		exit(1);
	}

	while(1) {
		fds = os_params.fds;
		select(os_params.max_fd+1, &fds, NULL, NULL, NULL);
		os_params_in.fds = fds;
		qmi_csi_handle_event(svc.service_handle, &os_params_in);
	}
	qmi_csi_unregister(svc.service_handle);
	LOGI("Exiting remote storage service");

	return 0;
}
