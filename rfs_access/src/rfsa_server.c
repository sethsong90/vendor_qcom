/*
 * Copyright (c) 2012-2013 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "rfsa_server.h"
#include "rfsa_vtl_server.h"

/****************************************************************************
* Defines                                                                   *
****************************************************************************/
/**
* Maximum number of clients accepted by the server
*/
#define MAX_CLIENTS			5
/**
* Maximum size of shared memory used by the QDSP client (to be discussed)
*/
#define MAX_SHARED_MEM_DL_DSP		(128 * 1024)
#define MAX_SHARED_MEM_DL_MDM		(128 * 1024)
#define RFSA_SERVER_CMD_QUEUE_SIZE	50

#define CLIENT_ID_ALL			0x0		/* Client ID check not in use */
#define RFSA_DL_DSP_CLIENT_ID		0x011013ec	/* Unique DSP client ID for dynamic loading */
#define RFSA_DL_MDM_CLIENT_ID		0x011013ed	/* Unique MODEM client ID for dynamic loading */

#define UIO_QCOM_VER	"1.0"
#define UIO_QCOM_DSP_NAME	"rfsa_dsp"
#define UIO_QCOM_MDM_NAME	"rfsa_mdm"

#define UIO_DEV		"/dev/uio%d"
#define UIO_CLASS	"/sys/class/uio"
#define UIO_NAME	"/sys/class/uio/uio%d/name"
#define UIO_VER		"/sys/class/uio/uio%d/version"
#define UIO_MAP0	"/sys/class/uio/uio%d/maps/map0"
#define UIO_ADDR	"/sys/class/uio/uio%d/maps/map0/addr"

/****************************************************************************
* Definitions                                                               *
****************************************************************************/

int uio_num = 0;

struct rfsa_pmem_region {
	unsigned long	offset;
	unsigned long	len;
};

typedef struct rfsa_mmap_info
{
	int32_t		pmem_fd;
	int32_t		ion_fd;
	uint8_t		*pVirtualAddr;
	uint64_t	*pPhyAddr;
	uint32_t	map_buf_size;
	uint32_t	filled_len;
} rfsa_mmap_info;

/**
* The client info structure
*/
typedef struct
{
	/* Client ID */
	uint32_t		client_id;
	/* Shared memory buffer */
	struct rfsa_mmap_info	*memory_ptr;
	/* Current shared memory size */
	uint32_t		memory_size;
	/* Max shared memory size */
	uint32_t		max_shared_memory;
	/* Current file handle */
	int			file_h;
	/* Client name*/
	char			client_name[10];
} rfsa_client_info_t;

/****************************************************************************
* Variables                                                                 *
****************************************************************************/

/** name of server working thread */
static const char rfsa_server_my_thread_name[] = "RFSA_SERVER";

/** variables used to syncronize the threads */
static rfsa_lock_t rfsa_server_lock;
static rfsa_event_t rfsa_server_control_event;
static rfsa_event_t rfsa_server_work_event;
static rfsa_event_t rfsa_server_thread_end_event;

/** working thread */
static rfsa_thread_t rfsa_server_thread;

/** variables used for the queue of events */
static rfsa_list_t rfsa_server_free_cmd_q;
static rfsa_list_t rfsa_server_used_cmd_q;
static rfsa_server_work_item_t rfsa_server_cmds[RFSA_SERVER_CMD_QUEUE_SIZE];

static int stopped;
static int init_done;

static uint64_t rfsa_shrd_mem_phy_addr;

/**
* Clients which are supported by the server
*/
rfsa_client_info_t client_info[] = {
	{RFSA_DL_DSP_CLIENT_ID, NULL, 0, MAX_SHARED_MEM_DL_DSP, -1, UIO_QCOM_DSP_NAME},
	{RFSA_DL_MDM_CLIENT_ID, NULL, 0, MAX_SHARED_MEM_DL_MDM, -1, UIO_QCOM_MDM_NAME},
};

/**
* Number of defined clients
*/
int32_t  no_clients = sizeof(client_info)/sizeof(rfsa_client_info_t);

/****************************************************************************
* Path look up table							    *
****************************************************************************/
#define PATH_TABLE_PREFIX_LEN		8
#define PATH_TABLE_PATH_LEN		64
#define PATH_TABLE_MAX_ITEMS		8
typedef struct
{
	char		prefix[PATH_TABLE_PREFIX_LEN];
	char		path[PATH_TABLE_PATH_LEN];
	char		in_use;
	uint32_t	client_id;
} rfsa_path_table_t;

static rfsa_path_table_t sys_path_tbl[PATH_TABLE_MAX_ITEMS] =
{
	{"TEST://", "/data/rfsa/test/", 1, CLIENT_ID_ALL},
	{"ADSP://", "/system/lib/rfsa/adsp/", 1, RFSA_DL_DSP_CLIENT_ID},
	{"MPSS://", "/system/lib/rfsa/mpss/", 1, RFSA_DL_MDM_CLIENT_ID},
	{"", "", 0, 0}
};

/*****************************************************************************
* Forward declarations							     *
*****************************************************************************/
static int32_t rfsa_server_stat(rfsa_packet_t *packet_ptr);
static int32_t rfsa_server_create(rfsa_packet_t *packet_ptr);
static int32_t rfsa_server_read(rfsa_packet_t *packet_ptr);
static int32_t rfsa_server_get_buff_addr(rfsa_packet_t *packet_ptr);
static int32_t rfsa_server_release_buff_addr(rfsa_packet_t *packet_ptr);
static int32_t rfsa_server_iovec_write(rfsa_packet_t *packet_ptr);
static int32_t rfsa_server_iovec_read(rfsa_packet_t *packet_ptr);
static struct rfsa_mmap_info *rfsa_server_mem_alloc(rfsa_client_info_t *client_ptr);
static void rfsa_server_mem_free(struct rfsa_mmap_info **pmem_data);
static int32_t rfsa_path_lookup(char *name_i, int32_t name_i_len, char *name_o,
				int32_t name_o_len, uint32_t client_id);
/*****************************************************************************
* Implementations                                                           *
****************************************************************************/

/**
* Checks if the packet received is valid
*/
int32_t rfsa_server_check_packet(uint_t client_id, rfsa_client_info_t **client_ptr)
{
	int i;

	for (i = 0; i < no_clients; i++) {
		if (client_info[i].client_id == client_id) {
			if (client_ptr != NULL)
				*client_ptr = &(client_info[i]);

			return RFSA_EOK;
		}
	}

	LOGE("Packet error: Client ID is not valid: 0x%x!\n", client_id);

	return RFSA_EFAILED;
}

/**
* Checks and release all buffers in use
*/
int32_t rfsa_server_release_all_buffer()
{
	int i;
	rfsa_client_info_t *client_ptr = NULL;

	for (i = 0; i < no_clients; i++) {
		client_ptr = &(client_info[i]);
		if (client_ptr != NULL && client_ptr->memory_ptr != NULL)
			rfsa_server_mem_free(&client_ptr->memory_ptr);
	}

	return RFSA_EOK;
}

/**
* Look up predefined file path
*/
static int32_t rfsa_path_lookup(char *name_i, int name_i_len, char *name_o,
				int name_o_len, uint32_t client_id)
{
	int32_t ret = RFSA_EOK;
	int i;
	int ent;
	int matched = 0;
	size_t len;

	// Match incoming name with table prefix entry
	for (ent = 0; ent < PATH_TABLE_MAX_ITEMS; ent++) {
		// If we reach the end of the table, end the search
		if (!sys_path_tbl[ent].in_use)
			break;
		// Match the prefix list, one char at a time
		for (i = 0; i < PATH_TABLE_PREFIX_LEN; i++) {
			// Match failed, of this search
			if (i > name_i_len)
				break;
			// End of search found?
			if (sys_path_tbl[ent].prefix[i] == '\0') {
				matched = 1;
				break;
			}
			if (sys_path_tbl[ent].prefix[i] != name_i[i] ||
					      name_i[i] == '\0')
				break;
		}
		if (matched)
			break;
	}
	if (!matched) {
		ret = RFSA_EFAILED;
		goto out;
	}

	// Check for client ID match
	if ((sys_path_tbl[ent].client_id != client_id) &&
	    (sys_path_tbl[ent].client_id != 0) && (client_id != 0)) {
		ret = RFSA_EFAILED;
		goto out;
	}

	// Prepend the prefix path
	len = strlcpy(name_o, sys_path_tbl[ent].path, name_o_len);
	strlcat(name_o, name_i + i, name_o_len - len);

	// Check to see if ".." or "~" string is used
	if ((strstr(name_o, "..") != NULL) ||
		(strstr(name_o, "~") != NULL)) {
		ret = RFSA_EFAILED;
		goto out;
	}

out:
	if (ret != RFSA_EOK)
		LOGE("rfsa_path_lookup failed for %s\n", name_i);

	return ret;
}

/**
* Callback function which processes every packet received
*/
int32_t rfsa_server_callback_impl(rfsa_packet_t *packet_ptr)
{
	int32_t ret = RFSA_EOK;

	if (packet_ptr == NULL) {
		LOGE("rfsa_server_callback_impl, packet_ptr = NULL");
		ret = RFSA_EFAILED;
		goto out;
	}

	switch (packet_ptr->opcode) {
	case QMI_RFSA_FILE_STAT_REQ_MSG_V01:
		ret = rfsa_server_stat(packet_ptr);
		break;
	case QMI_RFSA_FILE_CREATE_REQ_MSG_V01:
		ret = rfsa_server_create(packet_ptr);
		break;
	case QMI_RFSA_FILE_READ_REQ_MSG_V01:
		ret = rfsa_server_read(packet_ptr);
		break;
	case QMI_RFSA_GET_BUFF_ADDR_REQ_MSG_V01:
		ret = rfsa_server_get_buff_addr(packet_ptr);
		break;
	case QMI_RFSA_RELEASE_BUFF_ADDR_REQ_MSG_V01:
		ret = rfsa_server_release_buff_addr(packet_ptr);
		break;
	case QMI_RFSA_IOVEC_FILE_WRITE_REQ_MSG_V01:
		ret = rfsa_server_iovec_write(packet_ptr);
		break;
	case QMI_RFSA_IOVEC_FILE_READ_REQ_MSG_V01:
		ret = rfsa_server_iovec_read(packet_ptr);
		break;
	default:
		LOGE("Unknown opcode, opcode: %d\n", packet_ptr->opcode);
		ret = RFSA_EFAILED;
		break;
	}

out:
	return ret;
}

/**
* File stat implementation
*/
int32_t rfsa_server_stat(rfsa_packet_t *packet_ptr)
{
	int32_t ret = RFSA_EOK;
	int rc = -1;
	struct stat buf;
	char fname[RFSA_MAX_FILE_PATH_V01];
	rfsa_file_stat_req_msg_v01 *file_stat_ptr;
	rfsa_file_stat_resp_msg_v01 *file_stat_resp_ptr;

	file_stat_ptr = &packet_ptr->rfsa_req.file_stat_req;
	file_stat_resp_ptr = &packet_ptr->rfsa_ret.file_stat_ret;

	ret = rfsa_path_lookup(file_stat_ptr->filename, RFSA_MAX_FILE_PATH_V01 + 1,
				fname, sizeof(fname), CLIENT_ID_ALL);
	if (ret != RFSA_EOK)
		goto out;

	file_stat_resp_ptr->flags = 0;

	rc = stat(fname, &buf);
	if (rc == -1) {
		LOGE("File (%s) not found, err: %d\n", fname, errno);
		ret = RFSA_EFAILED;
		goto out;
	}

	LOGV("Stat file (%s), ret: %d, size: %lld\n", fname, ret, buf.st_size);
	file_stat_resp_ptr->size = buf.st_size;
	file_stat_resp_ptr->size_valid = 1;

	// Check for READ operation
	rc = open(fname, O_RDONLY);
	if (rc == -1) {
		LOGE("Failed to open file (%s) as read only, err: %d\n", fname, errno);
		ret = RFSA_EFAILED;
		goto out;
	}

	file_stat_resp_ptr->flags |= RFSA_ACCESS_FLAG_READ_V01;
	close(rc);

out:
	if (ret != RFSA_EOK) {
		file_stat_resp_ptr->resp.result = QMI_RESULT_FAILURE;
		file_stat_resp_ptr->resp.error = QMI_ERR_ACCESS_DENIED_V01;
	} else {
		file_stat_resp_ptr->resp.result = QMI_RESULT_SUCCESS;
		file_stat_resp_ptr->resp.error = QMI_ERR_NONE;
	}
	packet_ptr->data_ptr = &packet_ptr->rfsa_ret.file_stat_ret;
	packet_ptr->data_size = sizeof(rfsa_file_stat_resp_msg_v01);

	return ret;
}

/**
* File create implementation
*/
int32_t rfsa_server_create(rfsa_packet_t *packet_ptr)
{
	int32_t ret = RFSA_EOK;
	int rc = -1;
	struct stat buf;
	char fname[RFSA_MAX_FILE_PATH_V01];
	rfsa_file_create_req_msg_v01 *file_create_ptr;
	rfsa_file_create_resp_msg_v01 *file_create_resp_ptr;
	int open_flags = O_WRONLY;

	file_create_ptr = &packet_ptr->rfsa_req.file_create_req;
	file_create_resp_ptr = &packet_ptr->rfsa_ret.file_create_ret;

	ret = rfsa_path_lookup(file_create_ptr->filename, RFSA_MAX_FILE_PATH_V01 + 1,
				fname, sizeof(fname), CLIENT_ID_ALL);

	if (ret != RFSA_EOK)
		goto out;

	LOGV("Create file (%s)\n", fname);
	// Check to see if the create mode is supported or not
	if (!(file_create_ptr->flags & RFSA_ACCESS_FLAG_CREATE_V01)) {
		LOGE("Invalid calling parameter, flags: %d\n", (int)file_create_ptr->flags);
		ret = RFSA_EFAILED;
		goto out;
	}

	open_flags |= O_CREAT;
	rc = open(fname, open_flags, 0600);
	if (rc == -1) {
		LOGE("File (%s) open failed, flags: %d, err: %d\n", fname, open_flags, errno);
		ret = RFSA_EFAILED;
		goto out;
	}

	// Close the opened handle immediately
	close(rc);

out:
	if (ret != RFSA_EOK) {
		file_create_resp_ptr->resp.result = QMI_RESULT_FAILURE;
		file_create_resp_ptr->resp.error = QMI_ERR_ACCESS_DENIED_V01;
	} else {
		file_create_resp_ptr->resp.result = QMI_RESULT_SUCCESS;
		file_create_resp_ptr->resp.error = QMI_ERR_NONE;
	}
	packet_ptr->data_ptr = &packet_ptr->rfsa_ret.file_create_ret;
	packet_ptr->data_size = sizeof(rfsa_file_create_resp_msg_v01);

	return ret;
}

/**
* File read implementation
*/
int32_t rfsa_server_read(rfsa_packet_t *packet_ptr)
{
	int32_t ret = RFSA_EOK;
	int size_to_read;
	rfsa_file_read_req_msg_v01 *file_read_ptr;
	rfsa_file_read_resp_msg_v01 *file_read_resp_ptr;
	uint32_t count;
	char fname[RFSA_MAX_FILE_PATH_V01];
	rfsa_client_info_t *client_ptr;
	int fp;

	file_read_ptr = &packet_ptr->rfsa_req.file_read_req;
	file_read_resp_ptr = &packet_ptr->rfsa_ret.file_read_ret;

	ret = rfsa_server_check_packet(file_read_ptr->client_id, &client_ptr);
	if (ret != RFSA_EOK)
		goto out;

	ret = rfsa_path_lookup(file_read_ptr->filename, RFSA_MAX_FILE_PATH_V01 + 1,
				fname, sizeof(fname), file_read_ptr->client_id);

	if (ret != RFSA_EOK)
		goto out;

	LOGV("Read file (%s), size: %d\n", fname, file_read_ptr->size);
	fp = open(fname, O_RDONLY);
	if (fp == -1) {
		LOGE("File (%s) open failed, flags: %d, err: %d\n", fname, O_RDONLY, errno);
		ret = RFSA_EFAILED;
		goto out;
	}

	// Check if seek operation is required
	if (file_read_ptr->size != 0) {
		if (lseek(fp, file_read_ptr->offset, SEEK_SET) == -1) {
			LOGE("File (%s) seek failed, offset: %d, err: %d\n",
				fname, file_read_ptr->offset, errno);
			ret = RFSA_EFAILED;
			goto out;
		}
	}

	// Allocate shared memory for data transfer
	if (client_ptr->memory_ptr == NULL) {
		client_ptr->memory_ptr = rfsa_server_mem_alloc(client_ptr);
		if (client_ptr->memory_ptr == NULL) {
			LOGE("Cannot alloc memory, size: %d\n", client_ptr->max_shared_memory);
			ret = RFSA_EFAILED;
			goto out;
		}
	}

	client_ptr->memory_size = client_ptr->memory_ptr->map_buf_size;
	memset(client_ptr->memory_ptr->pVirtualAddr, 0, client_ptr->memory_size);

	if (file_read_ptr->size > client_ptr->memory_size)
		size_to_read = client_ptr->memory_size;
	else
		size_to_read = file_read_ptr->size;

	count = read(fp, client_ptr->memory_ptr->pVirtualAddr, size_to_read);
	if (fp == -1) {
		LOGE("File (%s) read failed, count: %d, err: %d\n", fname, count, errno);
		ret = RFSA_EFAILED;
		goto out;
	}

	// Close the file at the end of the read
	close(fp);
	client_ptr->file_h = -1;

out:
	if (ret != RFSA_EOK) {
		file_read_resp_ptr->data_valid = 0;
		file_read_resp_ptr->resp.result = QMI_RESULT_FAILURE;
		file_read_resp_ptr->resp.error = QMI_ERR_ACCESS_DENIED_V01;
	} else {
		file_read_resp_ptr->data.count = count;
		file_read_resp_ptr->data_valid = 1;
		file_read_resp_ptr->data.buffer = *client_ptr->memory_ptr->pPhyAddr;
		file_read_resp_ptr->resp.result = QMI_RESULT_SUCCESS;
		file_read_resp_ptr->resp.error = QMI_ERR_NONE;
	}
	packet_ptr->data_ptr = &packet_ptr->rfsa_ret.file_read_ret;
	packet_ptr->data_size = sizeof(rfsa_file_read_resp_msg_v01);

	return ret;
}

/**
* Buffer allocation for IOVEC operation
*/
int32_t rfsa_server_get_buff_addr(rfsa_packet_t *packet_ptr)
{
	int32_t ret = RFSA_EOK;
	rfsa_get_buff_addr_req_msg_v01 *get_buff_addr_ptr;
	rfsa_get_buff_addr_resp_msg_v01 *get_buff_addr_resp_ptr;
	rfsa_client_info_t *client_ptr;

	get_buff_addr_ptr = &packet_ptr->rfsa_req.get_buff_addr_req;
	get_buff_addr_resp_ptr = &packet_ptr->rfsa_ret.get_buff_addr_ret;

	ret = rfsa_server_check_packet(get_buff_addr_ptr->client_id, &client_ptr);
	if (ret != RFSA_EOK)
		goto out;

	if (get_buff_addr_ptr->size > client_ptr->max_shared_memory) {
		LOGE("Requested memory size (%d) exceed allowed amount (%d)\n",
			get_buff_addr_ptr->size, client_ptr->max_shared_memory);
		ret = RFSA_EFAILED;
		goto out;
	}

	if (client_ptr->memory_ptr != NULL)
		rfsa_server_mem_free(&client_ptr->memory_ptr);

	client_ptr->memory_ptr = rfsa_server_mem_alloc(client_ptr);

	// Look up the physical memory allocated for the buffer in question
	// and return the allocated address
	get_buff_addr_resp_ptr->address_valid = 1;
	get_buff_addr_resp_ptr->address = rfsa_shrd_mem_phy_addr;

	LOGV("Server alloc buffer, id: %d buf addr: %x\n", get_buff_addr_ptr->client_id,
					   (int32_t)get_buff_addr_resp_ptr->address);

out:
	if (ret != RFSA_EOK) {
		get_buff_addr_resp_ptr->resp.result = QMI_RESULT_FAILURE;
		get_buff_addr_resp_ptr->resp.error = QMI_ERR_ACCESS_DENIED_V01;
	} else {
		get_buff_addr_resp_ptr->resp.result = QMI_RESULT_SUCCESS;
		get_buff_addr_resp_ptr->resp.error = QMI_ERR_NONE;
	}
	packet_ptr->data_ptr = &packet_ptr->rfsa_ret.get_buff_addr_ret;
	packet_ptr->data_size = sizeof(rfsa_get_buff_addr_resp_msg_v01);

	return ret;
}

/**
* Buffer de-allocation for IOVEC operation - dummy function
*/
int32_t rfsa_server_release_buff_addr(rfsa_packet_t *packet_ptr)
{
	int32_t ret = RFSA_EOK;
	rfsa_release_buff_addr_req_msg_v01 *free_buff_addr_ptr;
	rfsa_release_buff_addr_resp_msg_v01 *free_buff_addr_resp_ptr;
	rfsa_client_info_t *client_ptr;

	free_buff_addr_ptr = &packet_ptr->rfsa_req.free_buff_addr_req;
	free_buff_addr_resp_ptr = &packet_ptr->rfsa_ret.free_buff_addr_ret;

	ret = rfsa_server_check_packet(free_buff_addr_ptr->client_id, &client_ptr);
	if (ret != RFSA_EOK) {
		free_buff_addr_resp_ptr->resp.result = QMI_RESULT_FAILURE;
		free_buff_addr_resp_ptr->resp.error = QMI_ERR_ACCESS_DENIED_V01;
	} else {
		free_buff_addr_resp_ptr->resp.result = QMI_RESULT_SUCCESS;
		free_buff_addr_resp_ptr->resp.error = QMI_ERR_NONE;
	}
	packet_ptr->data_ptr = &packet_ptr->rfsa_ret.free_buff_addr_ret;
	packet_ptr->data_size = sizeof(rfsa_release_buff_addr_resp_msg_v01);

	return ret;
}

/**
* IOVEC write operation
*/
int32_t rfsa_server_iovec_write(rfsa_packet_t *packet_ptr)
{
	int32_t ret = RFSA_EOK;
	rfsa_iovec_file_write_req_msg_v01 *iovec_write_ptr;
	rfsa_iovec_file_write_resp_msg_v01 *iovec_write_resp_ptr;
	uint32_t count;
	uint32_t item;
	rfsa_client_info_t *client_ptr;
	int fp;
	uint32_t buf_offset;
	uint32_t file_offset;
	uint32_t bytes;
	char fname[RFSA_MAX_FILE_PATH_V01];
	int open_flags = O_WRONLY;

	iovec_write_ptr = &packet_ptr->rfsa_req.iovec_write_req;
	iovec_write_resp_ptr = &packet_ptr->rfsa_ret.iovec_write_ret;

	ret = rfsa_server_check_packet(iovec_write_ptr->client_id, &client_ptr);
	if (ret != RFSA_EOK)
		goto out;

	ret = rfsa_path_lookup(iovec_write_ptr->filename, RFSA_MAX_FILE_PATH_V01 + 1,
				fname, sizeof(fname), iovec_write_ptr->client_id);

	if (ret != RFSA_EOK)
		goto out;

	LOGV("Server iovec write, file: %s Pieces: %d\n", fname, iovec_write_ptr->iovec_struct_len);
	fp = open(fname, open_flags);
	if (fp == -1) {
		LOGE("File (%s) open failed, flags: %d, err: %d\n", fname, O_RDONLY, errno);
		ret = RFSA_EFAILED;
		goto out;
	}

	// Loop write operation
	for (item = 0; item < iovec_write_ptr->iovec_struct_len; item ++) {
		// Check range of data
		buf_offset = iovec_write_ptr->iovec_struct[item].buff_addr_offset;
		file_offset = iovec_write_ptr->iovec_struct[item].file_offset;
		bytes = iovec_write_ptr->iovec_struct[item].size;

		// Check if the output data buffer is out of range
		if ((buf_offset + bytes) > client_ptr->memory_size) {
			LOGE("Mem buffer out of range %d\n", buf_offset + bytes);
			ret = RFSA_EFAILED;
			goto out;
		}

		// Perform seek
		if (lseek(fp, file_offset, SEEK_SET) == -1) {
			LOGE("File (%s) seek failed, offset: %d, err: %d\n",
				fname, file_offset, errno);
			ret = RFSA_EFAILED;
			goto out;
		}

		// Write the data block
		client_ptr->memory_ptr->pVirtualAddr;
		count = write(fp, client_ptr->memory_ptr->pVirtualAddr + buf_offset, bytes);

		// Check the amount of data written, error if not the same
		if (count != bytes) {
			LOGE("Write size mismatch, expect %d, got %d, err: %d\n",
				bytes, count, errno);
			ret = RFSA_EFAILED;
			goto out;
		}
	}

out:
	if (ret != RFSA_EOK) {
		iovec_write_resp_ptr->resp.result = QMI_RESULT_FAILURE;
		iovec_write_resp_ptr->resp.error = QMI_ERR_ACCESS_DENIED_V01;
	} else {
		iovec_write_resp_ptr->resp.result = QMI_RESULT_SUCCESS;
		iovec_write_resp_ptr->resp.error = QMI_ERR_NONE;
	}
	packet_ptr->data_ptr = &packet_ptr->rfsa_ret.iovec_write_ret;
	packet_ptr->data_size = sizeof(rfsa_iovec_file_write_resp_msg_v01);

	return ret;
}

/**
* IOVEC read operation
*/
int32_t rfsa_server_iovec_read(rfsa_packet_t *packet_ptr)
{
	int32_t ret = RFSA_EOK;
	rfsa_iovec_file_read_req_msg_v01 *iovec_read_ptr;
	rfsa_iovec_file_read_resp_msg_v01 *iovec_read_resp_ptr;
	uint32_t count;
	uint32_t item;
	rfsa_client_info_t *client_ptr;
	int fp;
	uint32_t buf_offset;
	uint32_t file_offset;
	uint32_t bytes;
	char fname[RFSA_MAX_FILE_PATH_V01];

	iovec_read_ptr = &packet_ptr->rfsa_req.iovec_read_req;
	iovec_read_resp_ptr = &packet_ptr->rfsa_ret.iovec_read_ret;

	ret = rfsa_server_check_packet(iovec_read_ptr->client_id, &client_ptr);
	if (ret != RFSA_EOK)
		goto out;

	ret = rfsa_path_lookup(iovec_read_ptr->filename, RFSA_MAX_FILE_PATH_V01 + 1,
				fname, sizeof(fname), iovec_read_ptr->client_id);
	if (ret != RFSA_EOK)
		goto out;

	LOGV("Server iovec read, file: %s Pieces: %d\n", fname, iovec_read_ptr->iovec_struct_len);
	fp = open(fname, O_RDONLY);
	if (fp == -1) {
		LOGE("Failed to open file (%s) as read only, err: %d\n", fname, errno);
		ret = RFSA_EFAILED;
		goto out;
	}

	// Loop read operation
	for (item = 0; item < iovec_read_ptr->iovec_struct_len; item ++) {
		// Check range of data
		buf_offset = iovec_read_ptr->iovec_struct[item].buff_addr_offset;
		file_offset = iovec_read_ptr->iovec_struct[item].file_offset;
		bytes = iovec_read_ptr->iovec_struct[item].size;

		// Check if the output data buffer is out of range
		if ((buf_offset + bytes) > client_ptr->memory_size) {
			LOGE("Mem buffer out of range\n");
			ret = RFSA_EFAILED;
			goto out;
		}

		// Perform seek
		if (lseek(fp, file_offset, SEEK_SET) == -1) {
			LOGE("File (%s) seek failed, offset: %d, err: %d\n",
				fname, file_offset, errno);
			ret = RFSA_EFAILED;
			goto out;
		}

		// Write the data block
		client_ptr->memory_ptr->pVirtualAddr;
		count = read(fp, client_ptr->memory_ptr->pVirtualAddr + buf_offset, bytes);

		// Check the amount of data written, error if not the same
		if (count != bytes) {
			LOGE("Write size mismatch, expect %d, got %d, err: %d\n",
				bytes, count, errno);
			ret = RFSA_EFAILED;
			goto out;
		}
	}

out:
	if (ret != RFSA_EOK) {
		iovec_read_resp_ptr->resp.result = QMI_RESULT_FAILURE;
		iovec_read_resp_ptr->resp.error = QMI_ERR_ACCESS_DENIED_V01;
	} else {
		iovec_read_resp_ptr->resp.result = QMI_RESULT_SUCCESS;
		iovec_read_resp_ptr->resp.error = QMI_ERR_NONE;
	}
	packet_ptr->data_ptr = &packet_ptr->rfsa_ret.iovec_read_ret;
	packet_ptr->data_size = sizeof(rfsa_iovec_file_read_resp_msg_v01);

	return ret;
}

static uint32_t get_phys_addr(char* client_name)
{
	char cwd[50], name[10], version[10], addr[10], tmp[50], addr_dir[50], id[10];
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
				fgets_ret = fgets(name, strlen(client_name) + 1 , fd);
				if (!fgets_ret)
					LOGE("fgets failed with %d for %s\n", ferror(fd), tmp);
				if(!strcasecmp(name, client_name))
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
					fgets_ret = fgets(addr, 10, fd);
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
		LOGE("Matching UIO shared mem device not found for %s\n", client_name);
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

// Adding client ID to indicate which memory space to get the memory from is required
struct rfsa_mmap_info *rfsa_server_mem_alloc(rfsa_client_info_t *client_ptr)
{
	struct rfsa_mmap_info *pmem_data = NULL;
	struct rfsa_pmem_region region;
	int mem_fd;
	char dev_path[50];

	pmem_data = (struct rfsa_mmap_info*) calloc(sizeof(struct rfsa_mmap_info), 1);
	if (!pmem_data)
		return NULL;

	rfsa_shrd_mem_phy_addr = get_phys_addr(client_ptr->client_name);
	if(!rfsa_shrd_mem_phy_addr) {
		LOGE("Failed in getting the physical address for shared mem\n");
		exit(1);
	}
	/* Open /dev/uioX for mmap later */
	snprintf(dev_path, 50, UIO_DEV, uio_num);
	LOGV("dev_path:%s\n", dev_path);

	pmem_data->pmem_fd = open(dev_path, O_RDWR | O_DSYNC);
	if (pmem_data->pmem_fd < 0) {
		LOGE("Unable to open /dev/uio%d\n", uio_num);
		exit(1);
	}

	/* Align the size wrt the page boundary size of 4k */
	pmem_data->map_buf_size = (client_ptr->max_shared_memory + 4095) & (~4095);

	LOGV("Req size: %d memory size: %d\n", client_ptr->max_shared_memory,
		pmem_data->map_buf_size);
	/* Map the PMEM file descriptor into current process address space */
	pmem_data->pVirtualAddr = (uint8_t*) mmap(NULL, pmem_data->map_buf_size,
							PROT_READ | PROT_WRITE,
							MAP_SHARED,
							pmem_data->pmem_fd, 0);

	if (MAP_FAILED == pmem_data->pVirtualAddr) {
		LOGE("\n mmap() failed\n");
		close(pmem_data->pmem_fd);
		free(pmem_data);
		return NULL;
	}

	pmem_data->pPhyAddr = &rfsa_shrd_mem_phy_addr;
	LOGV("memory size: %d, phy addr: %x\n", pmem_data->map_buf_size,
		(unsigned int)*pmem_data->pPhyAddr);

	return pmem_data;
}

/**
* Memory deallocating function for physically contiguous memory
*/
void rfsa_server_mem_free(struct rfsa_mmap_info **pmem_data)
{
	if (pmem_data && (*pmem_data)) {
		if ((*pmem_data)->pVirtualAddr &&
			(EINVAL == munmap((*pmem_data)->pVirtualAddr,
			(*pmem_data)->map_buf_size)))
				LOGE("\n Error in Unmapping the buffer %p\n",
					(*pmem_data)->pVirtualAddr);

		(*pmem_data)->pVirtualAddr = NULL;

		close((*pmem_data)->pmem_fd);
		(*pmem_data)->pmem_fd = -1;

		free(*pmem_data);
		*pmem_data = NULL;
	}
}

static void rfsa_server_isr_lock_fn(void)
{
	(void)rfsa_lock_enter(rfsa_server_lock);
}

static void rfsa_server_isr_unlock_fn(void)
{
	(void)rfsa_lock_leave(rfsa_server_lock);
}

int32_t rfsa_server_get_free_packet(rfsa_server_work_item_t **item)
{
	int32_t ret;

	ret = rfsa_list_remove_head(&rfsa_server_free_cmd_q, ((rfsa_list_node_t**)item));

	return ret;
}

void rfsa_server_add_to_queue(rfsa_server_work_item_t *item)
{
	LOGV("opcode: 0x%x\n", item->rfsa_packet.opcode);
	(void)rfsa_list_add_tail(&rfsa_server_used_cmd_q, (rfsa_list_node_t *)&item->link);
	(void)rfsa_event_signal(rfsa_server_work_event);
}

static int32_t rfsa_server_worker_fn(void *param)
{
	int32_t ret;
	rfsa_server_work_item_t *item;

	(void)rfsa_event_create(&rfsa_server_work_event);
	(void)rfsa_event_signal(rfsa_server_control_event);

	while (1) {
		ret = rfsa_event_wait(rfsa_server_work_event);

		if ((ret != RFSA_EOK) || stopped)
			break;

		while (!stopped) {
			ret = rfsa_list_remove_head(&rfsa_server_used_cmd_q, ((rfsa_list_node_t** )&item));
			if (ret != RFSA_EOK)
				break;

			LOGV("Process command item:0x%x\n", (int)item);
			rfsa_server_callback_impl(&item->rfsa_packet);

			rfsa_vtl_server_response(item);
			(void)rfsa_list_add_tail(&rfsa_server_free_cmd_q, (rfsa_list_node_t* )&item->link);
		}
	}

	(void)rfsa_event_destroy(rfsa_server_work_event);
	(void)rfsa_event_signal(rfsa_server_control_event);
	(void)rfsa_event_signal(rfsa_server_thread_end_event);

	return RFSA_EOK;
}

/**
* Initialize the server
*/
int32_t rfsa_server_init(void)
{
	int index;

	if (init_done) {
		LOGE("rfsa_server_init already called\n");
		return RFSA_EFAILED;
	}

	stopped = 0;

	(void)rfsa_lock_create(&rfsa_server_lock);
	(void)rfsa_event_create(&rfsa_server_control_event);

	(void)rfsa_event_create(&rfsa_server_thread_end_event);

	(void)rfsa_list_init(&rfsa_server_free_cmd_q, rfsa_server_isr_lock_fn,
		rfsa_server_isr_unlock_fn);

	for (index = 0; index < RFSA_SERVER_CMD_QUEUE_SIZE; ++index) {
		(void)rfsa_list_add_tail(&rfsa_server_free_cmd_q,
			(rfsa_list_node_t* )&rfsa_server_cmds[index].link);
	}

	(void)rfsa_list_init(&rfsa_server_used_cmd_q, rfsa_server_isr_lock_fn,
				rfsa_server_isr_unlock_fn);

	(void)rfsa_thread_create(&rfsa_server_thread, rfsa_server_my_thread_name,
				100, NULL, 0, rfsa_server_worker_fn, NULL);
	(void)rfsa_event_wait(rfsa_server_control_event);

	init_done = 1;
	LOGI("Initialized RFS server\n");
	return rfsa_vtl_server_init();
}

/**
* De-Initialize the server
*/
int32_t rfsa_server_deinit(void)
{
	int32_t ret;
	rfsa_server_work_item_t *item;

	if (!init_done) {
		LOGE("rfsa_server_deinit called ahead of rfsa_server_init\n");
		return RFSA_EFAILED;
	}

	/* signal the working thread to finish */
	stopped = 1;
	(void)rfsa_event_signal(rfsa_server_work_event);

	/* wait for working thread to finish */
	ret = rfsa_event_wait(rfsa_server_thread_end_event);
	if (ret != RFSA_EOK)
		LOGE("Error on rfsa_event_wait, ret: %d\n", ret);

	ret = rfsa_vtl_server_deinit();
	if (ret != RFSA_EOK)
		LOGE("Error on rfsa_vtl_server_deinit, ret: %d\n", ret);

	while (1) {
		ret = rfsa_list_remove_head(&rfsa_server_used_cmd_q, ((rfsa_list_node_t** )&item));
		if (ret != RFSA_EOK)
			break;
	}

	while (1) {
		ret = rfsa_list_remove_head(&rfsa_server_free_cmd_q, ((rfsa_list_node_t** )&item));
		if (ret != RFSA_EOK)
			break;
	}

	(void)rfsa_lock_destroy(rfsa_server_lock);
	(void)rfsa_event_destroy(rfsa_server_control_event);

	init_done = 0;
	return ret;
}

int rfsa_server_check_received_data(int msg_id, int req_c_struct_len)
{
	switch (msg_id) {
	case QMI_RFSA_FILE_STAT_REQ_MSG_V01:
		return (req_c_struct_len == sizeof(rfsa_file_stat_req_msg_v01));
	case QMI_RFSA_FILE_CREATE_REQ_MSG_V01:
		return (req_c_struct_len == sizeof(rfsa_file_create_req_msg_v01));
	case QMI_RFSA_FILE_READ_REQ_MSG_V01:
		return (req_c_struct_len == sizeof(rfsa_file_read_req_msg_v01));
	case QMI_RFSA_GET_BUFF_ADDR_REQ_MSG_V01:
		return (req_c_struct_len == sizeof(rfsa_get_buff_addr_req_msg_v01));
	case QMI_RFSA_RELEASE_BUFF_ADDR_REQ_MSG_V01:
		return (req_c_struct_len == sizeof(rfsa_release_buff_addr_req_msg_v01));
	case QMI_RFSA_IOVEC_FILE_WRITE_REQ_MSG_V01:
		return (req_c_struct_len == sizeof(rfsa_iovec_file_write_req_msg_v01));
	case QMI_RFSA_IOVEC_FILE_READ_REQ_MSG_V01:
		return (req_c_struct_len == sizeof(rfsa_iovec_file_read_req_msg_v01));
	default:
		LOGE("Unknown opcode, opcode: %d\n", msg_id);
		break;
	}

	return 0;
}

int main(int argc, char **argv)
{
	int32_t ret = RFSA_EOK;
	pid_t pid;

	ret = rfsa_server_init();
	if (ret != RFSA_EOK) {
		LOGE("rfsa_server_init failed.\n");
		exit(1);
	}

	rfsa_server_release_all_buffer();

	return 0;
}
