/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
# Copyright (c) 2012-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
# Qualcomm Technologies Proprietary and Confidential.

              Diag Consumer Interface (DCI)

GENERAL DESCRIPTION

Implementation of functions specific to DCI.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

$Header:

when       who    what, where, why
--------   ---    ----------------------------------------------------------
10/08/12   RA     Interface Implementation for DCI I/O
03/20/12   SJ     Created
===========================================================================*/

#include <stdlib.h>
#include "comdef.h"
#include "stdio.h"
#include "diag_lsmi.h"
#include "./../include/diag_lsm.h"
#include "diagsvc_malloc.h"
#include "diag_lsm_event_i.h"
#include "diag_lsm_log_i.h"
#include "diag_lsm_msg_i.h"
#include "diag.h" /* For definition of diag_cmd_rsp */
#include "diag_lsm_pkt_i.h"
#include "diag_lsm_dci.h"
#include "diag_shared_i.h" /* For different constants */
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include "errno.h"
#include <pthread.h>
#include <stdint.h>
#include <eventi.h>
#include <msgi.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>

int dci_transaction_id;
unsigned char *dci_req_buf;

int diag_lsm_dci_init(void)
{
	int i;
	dci_transaction_id = 0;
	dci_req_buf = (unsigned char *)malloc(DCI_MAX_REQ_BUF_SIZE);
	if (!dci_req_buf)
		return DIAG_DCI_NO_MEM;

	diag_pkt_rsp_tracking_tbl *head = &dci_client_tbl->req_tbl_head;
	head->next = head;
	head->prev = head;
	head->info = NULL;
	pthread_mutex_init(&dci_client_tbl->req_tbl_mutex, NULL);
	return DIAG_DCI_NO_ERROR;
}

static void dci_delete_request_entry(diag_pkt_rsp_tracking_tbl *entry)
{
	if (!entry)
		return;
	entry->prev->next = entry->next;
	entry->next->prev = entry->prev;
	free(entry->info);
	free(entry);
}

void lookup_pkt_rsp_transaction(unsigned char *ptr)
{
	int len, uid, found = 0;
	uint8 delete_flag = 0;
	unsigned char *temp = ptr;
	diag_pkt_rsp_tracking_tbl *head = NULL, *walk_ptr = NULL;
	diag_pkt_tracking_info info;

	if (!ptr) {
		DIAG_LOGE("  Invalid pointer in %s\n", __func__);
		return;
	}

	len = *(int *)temp;
	temp += sizeof(int);
	delete_flag = *(uint8 *)temp;
	temp += sizeof(uint8);
	uid = *(int *)temp;
	temp += sizeof(int);
	len = len - sizeof(int); /* actual length of response */
	memset(&info, 0, sizeof(diag_pkt_tracking_info));

	pthread_mutex_lock(&dci_client_tbl->req_tbl_mutex);
	head = &dci_client_tbl->req_tbl_head;
	for (walk_ptr = head->next; walk_ptr && walk_ptr != head; walk_ptr = walk_ptr->next) {
		if (!walk_ptr->info || walk_ptr->info->uid != uid)
			continue;
		/*
		 * Found a match. Copy the response to the buffer and call
		 * the corresponding response handler
		 */
		if (len > 0 && len <= walk_ptr->info->rsp_len) {
			memcpy(&info, walk_ptr->info, sizeof(diag_pkt_tracking_info));
			memcpy(info.rsp_ptr, temp, len);
		} else {
			DIAG_LOGE(" Invalid response in %s, len:%d rsp_len: %d\n", __func__, len, walk_ptr->info->rsp_len);
		}
		/*
		 * Delete Flag will be set if it is safe to delete the entry.
		 * This means that the response is either a regular response or
		 * the last response in a sequence of delayed responses.
		 */
		if (delete_flag)
			dci_delete_request_entry(walk_ptr);
		found = 1;
		break;
	}
	pthread_mutex_unlock(&dci_client_tbl->req_tbl_mutex);

	if (found) {
		if( info.func_ptr != NULL ) {
			(*info.func_ptr) (info.rsp_ptr, len, info.data_ptr);
		}
	} else {
		DIAG_LOGE("  In %s, incorrect transaction %d\n", __func__, uid);
	}
}

int diag_register_dci_client (int *client_id, diag_dci_peripherals *list, int channel, void *os_params)
{
	int ret = DIAG_DCI_NO_REG;

	if (!client_id)
		return ret;

	dci_client_tbl = (struct diag_dci_client_tbl *)malloc(sizeof(struct diag_dci_client_tbl));
	if (!dci_client_tbl)
		return DIAG_DCI_NO_MEM;

	dci_client_tbl->list = *list;
	dci_client_tbl->signal_type = *(int *)os_params;
	dci_client_tbl->data_signal_flag = DISABLE;
	dci_client_tbl->data_signal_type = DIAG_INVALID_SIGNAL;

	*client_id = ioctl(fd, DIAG_IOCTL_DCI_REG, dci_client_tbl, 0, NULL, 0, NULL, NULL);

	if (*client_id == DIAG_DCI_NO_REG) {
		DIAG_LOGE(" could not register client, error: %d\n", errno);
	} else {
		dci_client_tbl->client_id = (unsigned int)*client_id;
		ret = diag_lsm_dci_init();
	}

	return ret;
}

int diag_register_dci_stream (void (*func_ptr_logs)(unsigned char *ptr, int len),
							  void (*func_ptr_events)(unsigned char *ptr, int len))
{
	if (!dci_client_tbl)
		return DIAG_DCI_NO_MEM;

	dci_client_tbl->func_ptr_logs = func_ptr_logs;
	dci_client_tbl->func_ptr_events = func_ptr_events;
	return DIAG_DCI_NO_ERROR;
}

int diag_release_dci_client(int *client_id)
{
	int result = 0;
	diag_pkt_rsp_tracking_tbl *head = NULL, *walk_ptr = NULL;

	if (!client_id)
		return DIAG_DCI_NO_REG;

	if (*client_id <= 0) {
		return DIAG_DCI_NOT_SUPPORTED;
	}

	result = ioctl(fd, DIAG_IOCTL_DCI_DEINIT, *client_id, 0, NULL, 0, NULL, NULL);

	if (result == -1) {
		DIAG_LOGE(" could not remove entries, error: %d\n", errno);
		return DIAG_DCI_ERR_DEREG;
	} else {
		*client_id = 0;
		dci_client_tbl->client_id = 0;

		/* Delete the client requests */
		pthread_mutex_lock(&dci_client_tbl->req_tbl_mutex);
		head = &dci_client_tbl->req_tbl_head;
		for (walk_ptr = head->next; walk_ptr && walk_ptr != head; walk_ptr = head->next)
			dci_delete_request_entry(walk_ptr);
		pthread_mutex_unlock(&dci_client_tbl->req_tbl_mutex);
		free(dci_client_tbl);
		free(dci_req_buf);
		return DIAG_DCI_NO_ERROR;
	}
}

static diag_pkt_rsp_tracking_tbl *diag_register_dci_pkt(void (*func_ptr)(unsigned char *ptr, int len, void *data_ptr),
							int uid, unsigned char *rsp_ptr, int rsp_len, void *data_ptr)
{
	diag_pkt_tracking_info *req_info = NULL;
	diag_pkt_rsp_tracking_tbl *temp = NULL;
	diag_pkt_rsp_tracking_tbl *new_req = NULL;
	diag_pkt_rsp_tracking_tbl *head = &dci_client_tbl->req_tbl_head;

	req_info = (diag_pkt_tracking_info *)malloc(sizeof(diag_pkt_tracking_info));
	if (!req_info)
		return NULL;
	new_req = (diag_pkt_rsp_tracking_tbl *)malloc(sizeof(diag_pkt_rsp_tracking_tbl));
	if (!new_req) {
		free(req_info);
		return NULL;
	}

	req_info->uid = uid;
	req_info->func_ptr = func_ptr;
	req_info->rsp_ptr = rsp_ptr;
	req_info->rsp_len = rsp_len;
	req_info->data_ptr = data_ptr;
	new_req->info = req_info;
	new_req->next = new_req->prev = NULL;

	pthread_mutex_lock(&dci_client_tbl->req_tbl_mutex);
	temp = head->prev;
	head->prev = new_req;
	new_req->next = head;
	new_req->prev = temp;
	temp->next = new_req;
	pthread_mutex_unlock(&dci_client_tbl->req_tbl_mutex);
	return new_req;
}

int diag_send_dci_async_req(int client_id, unsigned char buf[], int bytes, unsigned char *rsp_ptr, int rsp_len,
					   void (*func_ptr)(unsigned char *ptr, int len, void *data_ptr), void *data_ptr)
{
	int err = -1;
	diag_pkt_rsp_tracking_tbl *new_req = NULL;

	if (client_id <= 0)
		return DIAG_DCI_NOT_SUPPORTED;
	if (bytes > DCI_MAX_REQ_BUF_SIZE || bytes < 1)
		return DIAG_DCI_HUGE_PACKET;

	if (!buf) {
		DIAG_LOGE("diag: Request Bufffer is not set\n");
		return DIAG_DCI_NO_MEM;
	}
	if (!dci_req_buf) {
		DIAG_LOGE("diag: Request Buffer not initialized\n");
		return DIAG_DCI_NO_MEM;
	}
	if (!rsp_ptr) {
		DIAG_LOGE("diag: Response Buffer not initialized\n");
		return DIAG_DCI_NO_MEM;
	}
	dci_transaction_id++;
	*(int *)dci_req_buf = DCI_DATA_TYPE; /* start header */
	*(int *)(dci_req_buf + 4) = dci_transaction_id; /* transaction ID */
	new_req = diag_register_dci_pkt(func_ptr, dci_transaction_id, rsp_ptr, rsp_len, data_ptr);
	if (!new_req)
		return DIAG_DCI_NO_MEM;
	memcpy(dci_req_buf + 8, buf, bytes);
	err = diag_send_data(dci_req_buf, 8 + bytes);

	/* Registration failed. Delete entry from registration table */
	if (err != DIAG_DCI_NO_ERROR) {
	pthread_mutex_lock(&dci_client_tbl->req_tbl_mutex);
		dci_delete_request_entry(new_req);
		pthread_mutex_unlock(&dci_client_tbl->req_tbl_mutex);
		err = DIAG_DCI_SEND_DATA_FAIL;
	}

	return err;
}

int diag_get_dci_support_list(diag_dci_peripherals *list)
{
	if (!list)
		return DIAG_DCI_NO_MEM;

	return(ioctl(fd, DIAG_IOCTL_DCI_SUPPORT, list, 0, NULL, 0, NULL, NULL));
}

int diag_log_stream_config(int client_id, int set_mask, uint16 log_codes_array[], int num_codes)
{
	int err = -1;

	if (client_id <= 0)
		return DIAG_DCI_NOT_SUPPORTED;
	if (num_codes < 1)
		return DIAG_DCI_PARAM_FAIL;
	if (!dci_req_buf)
		return DIAG_DCI_NO_MEM;

	*(int *)dci_req_buf = DCI_DATA_TYPE; /* start header */
	*(int *)(dci_req_buf + 4) = DCI_LOG_TYPE; /* ID for log stream */
	*(int *)(dci_req_buf + 8) = set_mask; /* set/disable masks */
	*(int *)(dci_req_buf + 12) = num_codes; /* number of codes */
	memcpy(dci_req_buf + 16, log_codes_array, num_codes * sizeof(uint16));
	err = diag_send_data(dci_req_buf, 16 + num_codes * sizeof(uint16));
	if (err != DIAG_DCI_NO_ERROR)
		return DIAG_DCI_SEND_DATA_FAIL;
	else
		return DIAG_DCI_NO_ERROR;
}

int diag_event_stream_config(int client_id, int set_mask, int event_id_array[], int num_id)
{
	int err = -1;

	if (client_id <= 0)
		return DIAG_DCI_NOT_SUPPORTED;
	if (num_id < 1)
		return DIAG_DCI_PARAM_FAIL;
	if (!dci_req_buf)
		return DIAG_DCI_NO_MEM;

	*(int *)dci_req_buf = DCI_DATA_TYPE; /* start header */
	*(int *)(dci_req_buf + 4) = DCI_EVENT_TYPE; /* ID for event stream */
	*(int *)(dci_req_buf + 8) = set_mask; /* set/disable masks */
	*(int *)(dci_req_buf + 12) = num_id; /* number of codes */
	memcpy(dci_req_buf + 16, event_id_array, num_id * sizeof(int));
	err = diag_send_data(dci_req_buf, 16 + num_id * sizeof(int));
	if (err != DIAG_DCI_NO_ERROR) {
		DIAG_LOGE(" diag: error sending log stream config\n");
		return DIAG_DCI_SEND_DATA_FAIL;
	} else
		return DIAG_DCI_NO_ERROR;
}

int diag_get_health_stats(struct diag_dci_health_stats *dci_health)
{
	if (!dci_health)
		return DIAG_DCI_NO_MEM;

	dci_health->client_id = dci_client_tbl->client_id;
	return ioctl(fd, DIAG_IOCTL_DCI_HEALTH_STATS, dci_health, 0, NULL, 0, NULL, NULL);
}

int diag_get_log_status(int client_id, uint16 log_code, boolean *value)
{
	int err = DIAG_DCI_NO_ERROR;
	struct diag_log_event_stats stats;

	if (client_id <= 0)
		return DIAG_DCI_NOT_SUPPORTED;
	if (!value)
		return DIAG_DCI_NO_MEM;

	stats.code = log_code;
	stats.is_set = 0;
	err = ioctl(fd, DIAG_IOCTL_DCI_LOG_STATUS, &stats, 0, NULL, 0, NULL, NULL);
	if (err != DIAG_DCI_NO_ERROR) {
		return DIAG_DCI_SEND_DATA_FAIL;
	} else {
		*value = (stats.is_set == 1) ? TRUE : FALSE;
		return DIAG_DCI_NO_ERROR;
	}
}

int diag_get_event_status(int client_id, uint16 event_id, boolean *value)
{
	int err = DIAG_DCI_NO_ERROR;
	struct diag_log_event_stats stats;

	if (client_id <= 0)
		return DIAG_DCI_NOT_SUPPORTED;
	if (!value)
		return DIAG_DCI_NO_MEM;

	stats.code = event_id;
	stats.is_set = 0;
	err = ioctl(fd, DIAG_IOCTL_DCI_EVENT_STATUS, &stats, 0, NULL, 0, NULL, NULL);
	if (err != DIAG_DCI_NO_ERROR) {
		return DIAG_DCI_SEND_DATA_FAIL;
	} else {
		*value = (stats.is_set == 1) ? TRUE : FALSE;
		return DIAG_DCI_NO_ERROR;
	}
}

int diag_disable_all_logs(int client_id)
{
	int ret = DIAG_DCI_NO_ERROR;
	if (client_id <= 0)
		return DIAG_DCI_NOT_SUPPORTED;
	ret = ioctl(fd, DIAG_IOCTL_DCI_CLEAR_LOGS, &client_id, 0, NULL, 0, NULL, NULL);
	if (ret != DIAG_DCI_NO_ERROR) {
		DIAG_LOGE(" diag: error clearing all log masks, ret: %d, error: %d\n", ret, errno);
		return DIAG_DCI_SEND_DATA_FAIL;
	}
	return ret;
}

int diag_disable_all_events(int client_id)
{
	int ret = DIAG_DCI_NO_ERROR;
	if (client_id <= 0)
		return DIAG_DCI_NOT_SUPPORTED;
	ret = ioctl(fd, DIAG_IOCTL_DCI_CLEAR_EVENTS, &client_id, 0, NULL, 0, NULL, NULL);
	if (ret != DIAG_DCI_NO_ERROR) {
		DIAG_LOGE(" diag: error clearing all event masks, ret: %d, error: %d\n", ret, errno);
		return DIAG_DCI_SEND_DATA_FAIL;
	}
	return ret;
}

int diag_dci_vote_real_time(int client_id, int real_time)
{
	int err = DIAG_DCI_NO_ERROR;
	struct real_time_vote_t vote;
	if (client_id <= 0) {
		DIAG_LOGE(" Requesting client not supported for DCI\n");
		return DIAG_DCI_NOT_SUPPORTED;
	}
	if (!(real_time == MODE_REALTIME || real_time == MODE_NONREALTIME)) {
		DIAG_LOGE("diag: invalid mode change request\n");
		return DIAG_DCI_PARAM_FAIL;
	}
	vote.proc = DIAG_PROC_DCI;
	vote.real_time_vote = real_time;
	err = ioctl(fd, DIAG_IOCTL_VOTE_REAL_TIME, &vote, 0, NULL, 0, NULL, NULL);
	if (err == -1) {
		DIAG_LOGE(" diag: error voting for real time switch, ret: %d, error: %d\n", err, errno);
		err = DIAG_DCI_SEND_DATA_FAIL;
	}
	return DIAG_DCI_NO_ERROR;
}

int diag_dci_get_real_time_status(int *real_time)
{
	int err = DIAG_DCI_NO_ERROR;
	if (!real_time) {
		DIAG_LOGE("diag: invalid pointer in %s\n", __func__);
		return DIAG_DCI_PARAM_FAIL;
	}
	err = ioctl(fd, DIAG_IOCTL_GET_REAL_TIME, real_time, 0, NULL, 0, NULL, NULL);
	if (err != 0) {
		DIAG_LOGE(" diag: error voting for real time switch, err: %d, error: %d\n", err, errno);
		err = DIAG_DCI_SEND_DATA_FAIL;
	}
	return DIAG_DCI_NO_ERROR;
}

int diag_register_dci_signal_data(int client_id, int signal_type)
{
	if (client_id <= 0)
		return DIAG_DCI_NOT_SUPPORTED;
	if (!dci_client_tbl)
		return DIAG_DCI_NO_MEM;
	if (signal_type <= DIAG_INVALID_SIGNAL)
		return DIAG_DCI_PARAM_FAIL;

	dci_client_tbl->data_signal_flag = ENABLE;
	dci_client_tbl->data_signal_type = signal_type;
	return DIAG_DCI_NO_ERROR;
}

int diag_deregister_dci_signal_data(int client_id)
{
	if (client_id <= 0)
		return DIAG_DCI_NOT_SUPPORTED;
	if (!dci_client_tbl)
		return DIAG_DCI_NO_MEM;
	if (dci_client_tbl->data_signal_type == DIAG_INVALID_SIGNAL)
		return DIAG_DCI_NO_REG;

	dci_client_tbl->data_signal_flag = DISABLE;
	dci_client_tbl->data_signal_type = DIAG_INVALID_SIGNAL;
	return DIAG_DCI_NO_ERROR;
}

void diag_send_to_output(FILE *op_file, const char *str, ...)
{
	char buffer[6144];
	va_list arglist;
	va_start(arglist, str);
	if (!op_file)
		return;
	vsnprintf(buffer, 6144, str, arglist);
	fprintf(op_file, "%s", buffer);
	va_end(arglist);
}

int to_integer(char *str)
{
	int hex_int = 0;
	int i = 0, is_hex = 0;
	char *p;
	if (str) {
		while(str[i] != '\0') {
			if(str[i] == 'x' || str[i] == 'X') {
				sscanf(str, "%i", &hex_int);
				is_hex = 1;
				break;
			}
			i++;
		}
		if(is_hex == 0)
			hex_int = strtol(str, &p, 10);
	}
	return hex_int;
}
