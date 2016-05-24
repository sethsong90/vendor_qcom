/*
 *qcom-system-daemon: A module to handle commands(diag) that can be processed
 * in native code.
 *
 *Copyright (C) 2013-2014 Qualcomm Technologies, Inc. All rights reserved.
 *        Qualcomm Technologies Proprietary and Confidential.
 *
 *qcom_system_daemon.c : Main implementation of qcom-system-daemon
 */
#include "qcom_system_daemon.h"
#include <poll.h>

PACK(void *) ffbm_diag_reset_handler(PACK(void *)req_pkt, uint16 pkt_len);
PACK(void *) ffbm_diag_mode_handler(PACK(void *)req_pkt, uint16 pkt_len);
PACK(void *) reboot_to_edl(PACK(void *)req_pkt, uint16 pkt_len);
PACK(void *) diagpkt_version_handler(PACK(void*) req_ptr, uint16 pkt_len);
/*Table for commands handled only in ffbm mode*/
static const diagpkt_user_table_entry_type ftm_ffbm_mode_table[] =
{
	{DIAG_CONTROL_F, DIAG_CONTROL_F, ffbm_diag_reset_handler}
};

/*Table for commands handled in ffbm and normal mode*/
static const diagpkt_user_table_entry_type ftm_table[] =
{
	{FTM_FFBM_CMD_CODE, FTM_FFBM_CMD_CODE, ffbm_diag_mode_handler}
};

static const diagpkt_user_table_entry_type system_operations_table[] =
{
	{EDL_RESET_CMD_CODE, EDL_RESET_CMD_CODE, reboot_to_edl}
};

/* Table for image version command */
static const diagpkt_user_table_entry_type diagpkt_tbl[] =
{
   { VERSION_DIAGPKT_PREFIX, VERSION_DIAGPKT_PREFIX, diagpkt_version_handler }
};

typedef PACK(struct)
{
	uint8 command_code;
	uint8 subsys_id;
	uint16 subsys_cmd_code;
	uint32 status;
	uint16 delayed_rsp_id;
	uint16 rsp_cnt;         /* 0, means one response and 1, means two responses */
} diagpkt_version_subsys_hdr_v2_type;

typedef PACK(struct)
{
	diagpkt_version_subsys_hdr_v2_type hdr;
	uint16 version_data_len;
	unsigned char version_data[VERSION_TABLE_S];
} diagpkt_version_delayed_rsp;

static int fd_select_image = -1;
static int fd_image_version = -1;
static int fd_image_variant = -1;
static int fd_image_crm_version = -1;

int qcom_system_daemon_pipe[2];

static boot_selection_info bootselect_info;

PACK(void *) ffbm_diag_reset_handler(PACK(void *)req_pkt, uint16 pkt_len)
{
	void *rsp_pkt;
	rsp_pkt = diagpkt_alloc(DIAG_CONTROL_F, pkt_len);

	if (rsp_pkt)
		memcpy(rsp_pkt, req_pkt, pkt_len);
	else {
		ALOGE("diagpkt_alloc failed");
		return rsp_pkt;
	}
	if (write(qcom_system_daemon_pipe[1],
				REBOOT_CMD,
				sizeof(REBOOT_CMD)) < 0) {
		ALOGE("Failed to write to pipe: %s", strerror(errno));
		goto error;
	}
	close(qcom_system_daemon_pipe[1]);
error:
	return rsp_pkt;
}

PACK(void *) ffbm_diag_mode_handler(PACK(void *)req_pkt, uint16 pkt_len)
{
	PACK(void*)rsp = NULL;
	ALOGI("In ffbm_diag_mode_handler");
	rsp = ffbm_dispatch(req_pkt);
	return rsp;
}

void * ffbm_dispatch(ffbm_pkt_type *ffbm_pkt)
{
	ffbm_set_mode_rsq_type *rsp;
	if (ffbm_pkt->ffbm_cmd_code == FTM_FFBM_SET_MODE)
		rsp = ffbm_set_mode((ffbm_set_mode_req_type*)ffbm_pkt);
	else if (ffbm_pkt->ffbm_cmd_code == FTM_FFBM_GET_MODE)
		rsp = ffbm_get_mode();
	else
		rsp = NULL;
	return (void*)rsp;
}

static int set_mode(const char* mode)
{
	int num_bytes = 0;
	unsigned int write_count = 0;
	int fd = -1;
	struct stat misc_partition_stat;
	if (stat(MISC_PARTITION_LOCATION, &misc_partition_stat)) {
		ALOGE("Misc partition not found.Returning error");
		goto error;
	}
	fd = open(MISC_PARTITION_LOCATION, O_RDWR);
	if (fd < 0) {
		ALOGE("Failed to open misc partition");
		goto error;
	}
	do {
		num_bytes = write(fd, mode + write_count/sizeof(char),
				strlen(mode) - write_count);
		if (num_bytes < 0) {
			ALOGE("Failed to write to partition");
			goto error;
		}
		write_count+=num_bytes;
	} while(write_count < strlen(mode));
	close(fd);
	return RET_SUCCESS;
error:
	if (fd >= 0)
		close(fd);
	return RET_FAILED;

}

static int get_mode( ffbm_get_mode_rsq_type *pkt)
{
	int bytes_read = 0;
	char bootmode[PROPERTY_VALUE_MAX];
	int fd = -1;
	int offset = 0;
	char buffer[FFBM_COMMAND_BUFFER_SIZE];
	struct stat misc_partition_stat;
	if (!pkt) {
		ALOGE("Invalid argument to get_mode");
		goto error;
	}
	property_get("ro.bootmode", bootmode, NULL);
	if (!strncmp(bootmode, "ffbm", 4)) {
		ALOGI("Current mode : ffbm");
		pkt->iCurrentBootMode = BOOT_MODE_FFBM;
	} else {
		ALOGI("Current mode : hlos");
		pkt->iCurrentBootMode = BOOT_MODE_HLOS;
	}
	memset(buffer,'\0', sizeof(buffer));
	if (stat(MISC_PARTITION_LOCATION, &misc_partition_stat)) {
		ALOGE("Misc partition not found. Returning error");
		goto error;
	}
	fd = open(MISC_PARTITION_LOCATION, O_RDWR);
	if (fd < 0) {
		ALOGE("Failed to open misc partition");
		goto error;
	}
	do {
		bytes_read = read(fd, buffer + offset,
				(FFBM_COMMAND_BUFFER_SIZE-1) - offset);
		if (bytes_read < 0) {
			ALOGE("Failed to read from misc partition");
			goto error;
		}
		offset += bytes_read;
	} while (bytes_read > 0 && offset < (FFBM_COMMAND_BUFFER_SIZE - 1));

	if (!strncmp(buffer, "ffbm", 4)) {
		ALOGI("Next mode: ffbm");
		pkt->iNextBootMode = BOOT_MODE_FFBM;
	} else {
		ALOGI("Next mode: hlos");
		pkt->iNextBootMode = BOOT_MODE_HLOS;
	}
	return 0;
error:
	if (fd >= 0)
		close(fd);
	return -1;
}

static int open_image_files(void)
{
	fd_select_image = open(SELECT_IMAGE_FILE, O_WRONLY);
	if (fd_select_image < 0) {
		ALOGE("version_diagpkt: could not open select image file: %s", strerror(errno));
		return -1;
	}
	fd_image_version = open(IMAGE_VERSION_FILE, O_RDONLY);
	if (fd_image_version < 0) {
		ALOGE("version_diagpkt: could not open image version file: %s", strerror(errno));
		close(fd_select_image);
		return -1;
	}

	fd_image_variant = open(IMAGE_VARIANT_FILE, O_RDONLY);
	if (fd_image_variant < 0) {
		ALOGE("version_diagpkt: could not open image variant file: %s", strerror(errno));
		close(fd_select_image);
		close(fd_image_version);
		return -1;
	}

	fd_image_crm_version = open(IMAGE_CRM_VERSION_FILE, O_RDONLY);
	if (fd_image_crm_version < 0) {
		ALOGE("version_diagpkt: could not open image crm version file: %s", strerror(errno));
		close(fd_select_image);
		close(fd_image_version);
		close(fd_image_variant);
		return -1;
	}
	return 0;
}

void close_image_files(void)
{
	if (fd_select_image >= 0)
		close(fd_select_image);
	if (fd_image_version >= 0)
		close(fd_image_version);
	if (fd_image_variant >= 0)
		close(fd_image_variant);
	if (fd_image_crm_version >= 0)
		close(fd_image_crm_version);
}

static int read_image_files(unsigned char *temp_version_table_p)
{
	int ret = 0;
	unsigned char *temp;

	if (!temp_version_table_p) {
		ALOGE("Bad Address for image version: %s", strerror(errno));
		return -1;
	}

	temp = temp_version_table_p;
	if (fd_image_version >= 0) {
		ret = read(fd_image_version, temp, IMAGE_VERSION_NAME_SIZE);
		if (ret < 0) {
			ALOGE("version_diagpkt: Unable to read image version file: %s", strerror(errno));
			return -1;
		}
	} else {
		ALOGE("version_diagpkt: Fail to read, invalid fd_image_version");
		return -1;
	}

	temp += (IMAGE_VERSION_NAME_SIZE - 1);
	*temp++ = '\0';
	if (fd_image_variant >= 0) {
		ret = read(fd_image_variant, temp, IMAGE_VERSION_VARIANT_SIZE);
		if (ret < 0) {
			ALOGE("version_diagpkt: Unable to read image variant file: %s", strerror(errno));
			return -1;
		}
	} else {
		ALOGE("version_diagpkt: Fail to read, invalid fd_image_variant");
		return -1;
	}

	temp += (IMAGE_VERSION_VARIANT_SIZE - 1);
	*temp++ = '\0';
	if (fd_image_crm_version >= 0) {
		ret = read(fd_image_crm_version, temp, IMAGE_VERSION_OEM_SIZE);
		if (ret < 0) {
			ALOGE("version_diagpkt: Unable to read image crm version file: %s", strerror(errno));
			return -1;
		}
	} else {
		ALOGE("version_diagpkt: Fail to read, invalid fd_image_crm_version");
		return -1;
	}
	return 0;
}

static int get_version_info(diagpkt_version_delayed_rsp *version_table_p)
{
	int err = 0, ret, i, count = 0;
	unsigned char *temp_version_table_p;
	char image_index[3];

	temp_version_table_p = version_table_p->version_data;

	for (i = 0; i < (VERSION_TABLE_S/IMAGE_VERSION_SINGLE_BLOCK_SIZE); i++) {
		err = open_image_files();
		if (err < 0) {
			ALOGE("version_diagpkt: could not open image files %d", i);
			return -1;
		}
		count = (i < 10) ? 1 : 2;
		snprintf(image_index, sizeof(image_index), "%d", i);
		ret = write(fd_select_image, image_index, count);
		if (ret < 0) {
			ALOGE("version_diagpkt: Unable to write %d in select image file: %s", i, strerror(errno));
			close_image_files();
			return -1;
		}
		ret = read_image_files(temp_version_table_p);
		if (ret < 0) {
			ALOGE("version_diagpkt: Unable to read image file %d", i);
			close_image_files();
			return -1;
		}
		temp_version_table_p += (IMAGE_VERSION_SINGLE_BLOCK_SIZE - 1);
		*temp_version_table_p++ = '\0';

		close_image_files();
	}
	return 0;
}

PACK(void*) diagpkt_version_handler(PACK(void*) req_ptr, uint16 pkt_len)
{
	PACK(void *)rsp = NULL;
	int err = 0;
	diagpkt_version_delayed_rsp *version_table_p = NULL;
	diagpkt_subsys_delayed_rsp_id_type delay_rsp_id = 0;

	/* Allocate the length for immediate response */
	rsp = diagpkt_subsys_alloc_v2(VERSION_DIAGPKT_SUBSYS,
		VERSION_DIAGPKT_PREFIX, sizeof(diagpkt_version_subsys_hdr_v2_type));
	if (rsp == NULL) {
		ALOGE("version_diagpkt: could not allocate memory for response");
		return NULL;
	}
	/* Get the delayed_rsp_id that was allocated by diag to
	* use for the delayed response we're going to send next.
	* This id is unique in the system.
	*/
	delay_rsp_id = diagpkt_subsys_get_delayed_rsp_id(rsp);
	diagpkt_commit(rsp);

	/* Response length can't be more than VERSION_TABLE_S - 2
	* i.e. 4094 bytes because of limitation at diag side
	*/
	rsp = diagpkt_subsys_alloc_v2_delay(VERSION_DIAGPKT_SUBSYS,
		VERSION_DIAGPKT_PREFIX, delay_rsp_id, VERSION_TABLE_S - 2);

	if ((NULL != rsp) && (pkt_len == 4)) {
		diagpkt_subsys_set_rsp_cnt(rsp,1);
		version_table_p = (diagpkt_version_delayed_rsp*) rsp;
	} else {
		ALOGE("version_diagpkt: diagpkt allocation failed:%p or wrong pkt_len:%d",
				rsp, pkt_len);
		return NULL;
	}
	err = get_version_info(version_table_p);
	if (err < 0) {
		ALOGE("version_diagpkt: could not get image version info");
		return NULL;
	}
	version_table_p->version_data_len =
		VERSION_TABLE_S - 2 - sizeof(diagpkt_version_subsys_hdr_v2_type);
	/* copy the version data length */
	memcpy((char *)rsp+sizeof(diagpkt_version_subsys_hdr_v2_type),
		&(version_table_p->version_data_len),
		sizeof(uint16));
	/* copy the image version info */
	memcpy((char *)rsp+sizeof(diagpkt_version_subsys_hdr_v2_type)+sizeof(uint16),
		version_table_p->version_data,
		VERSION_TABLE_S - 2 - sizeof(diagpkt_version_subsys_hdr_v2_type));
	diagpkt_delay_commit(rsp);

	return NULL;
}

void *ffbm_set_mode(ffbm_set_mode_req_type *ffbm_pkt)
{
	ffbm_set_mode_rsq_type *rsp;
	rsp = (ffbm_set_mode_rsq_type*)diagpkt_subsys_alloc( DIAG_SUBSYS_FTM,
		FTM_FFBM_CMD_CODE,
		sizeof(ffbm_set_mode_rsq_type));
	if (!rsp) {
		ALOGE("Failed to allocate response packet");
		return rsp;
	}
	if (ffbm_pkt->iNextBootMode == BOOT_MODE_FFBM) {
		ALOGI("Setting bootmode to FFBM");
		if (set_mode(MODE_FFBM) != RET_SUCCESS) {
			ALOGE("Failed to set bootmode");
			goto error;
		}
	} else if (ffbm_pkt->iNextBootMode == BOOT_MODE_HLOS) {
		ALOGI("Setting bootmode to Normal mode");
		if (set_mode(MODE_NORMAL) != RET_SUCCESS) {
			ALOGE("Failed to set bootmode");
			goto error;
		}
	} else {
		ALOGI("Unknown boot mode recieved");
		goto error;
	}
	rsp->iFTM_Error_Code = FTM_FFBM_SUCCESS;
	return (void*)rsp;
error:
	rsp->iFTM_Error_Code = FTM_FFBM_FAIL;
	return (void*)rsp;
}

void *ffbm_get_mode()
{
	   ffbm_get_mode_rsq_type *rsp;
	   int rcode = -1;
	   ALOGI("In ffbm_get_mode");
	   rsp = (ffbm_get_mode_rsq_type*)diagpkt_subsys_alloc( DIAG_SUBSYS_FTM,
			   FTM_FFBM_CMD_CODE,
			   sizeof(ffbm_get_mode_rsq_type));
	   if (!rsp) {
		   ALOGE("Failed to allocate response packet");
	   } else {
		   rcode = get_mode(rsp);
		   if (rcode) {
			   ALOGE("Failed to get boot mode info");
			   rsp->iFTM_Error_Code = FTM_FFBM_FAIL;
		   } else
			   rsp->iFTM_Error_Code = FTM_FFBM_SUCCESS;
		   /*Boot sub modes are 0 for LA*/
		   rsp->iCurrentBootSubMode = 0;
		   rsp->iNextBootSubMode = 0;
	   }
	   return(void *) rsp;
}

void *reboot_to_edl(PACK(void *)req_pkt, uint16 pkt_len)
{
	void *rsp_pkt = NULL;
	ALOGI("reboot to edl command recieved");
	rsp_pkt = diagpkt_alloc(EDL_RESET_CMD_CODE, pkt_len);
	if (rsp_pkt)
		memcpy(rsp_pkt, req_pkt, pkt_len);
	else {
		ALOGE("diagpkt_alloc failed");
		goto error;
	}
	if (write(qcom_system_daemon_pipe[1],
				EDL_REBOOT_CMD,
				sizeof(EDL_REBOOT_CMD)) < 0) {
		ALOGE("Failed to write command to pipe: %s",
				strerror(errno));
		goto error;
	}
	close(qcom_system_daemon_pipe[1]);
	ALOGI("returning response packet");
error:
	return rsp_pkt;
}

static int get_bootselect_info(boot_selection_info *out) {
	FILE* f = fopen(BLOCK_DEVICE_NODE, "rb");
	if (f == NULL) {
		ALOGE("Can't open %s: (%s)", BLOCK_DEVICE_NODE, strerror(errno));
		return -1;
	}
	int count = fread(out, sizeof(*out), 1, f);
	fclose(f);
	if (count != 1) {
		ALOGE("Failed reading %s: (%s)", BLOCK_DEVICE_NODE, strerror(errno));
		return -1;
	}
	return 0;
}


static int reset_os_selector_bit(boot_selection_info *in) {
	FILE* f = fopen(BLOCK_DEVICE_NODE, "wb");
	if (f == NULL) {
		ALOGE("Can't open %s: (%s)", BLOCK_DEVICE_NODE, strerror(errno));
		return -1;
	}
	(*in).boot_partition_selection = 0;
	int count = fwrite(in, sizeof(*in), 1, f);
	fclose(f);
	if (count != 1) {
		ALOGE("Failed writing %s: (%s)", BLOCK_DEVICE_NODE, strerror(errno));
		return -1;
	}
	return 0;
}

int main()
{
	int binit_Success = 0;
	int bytes_read = 0;
	int offset = 0;
	char bootmode[PROPERTY_VALUE_MAX];
	char diag_reset_handler[PROPERTY_VALUE_MAX];
	char buffer[FFBM_COMMAND_BUFFER_SIZE];
	struct pollfd fds[2];
	char buf[10];
	int ret, nfds = 1;
	int is_factory = 0;

	// Initialize the structure
	if (!get_bootselect_info(&bootselect_info)) {
		// Create Node only if the factory bit is set
		if ((bootselect_info.state_info & BOOTSELECT_FACTORY)) {
			if ((bootselect_info.signature == BOOTSELECT_SIGNATURE) &&
				(bootselect_info.version == BOOTSELECT_VERSION)) {
				mode_t mode = 0600 | S_IFIFO;
				if (mknod(SOS_FIFO, mode, 0) < 0) {
					if (errno != EEXIST) {
						ALOGE("Failed to create node: %s",
						strerror(errno));
						return -1;
					}
				}
				is_factory = 1;
			} else
				ALOGE("bootselect signature or version mis-matched!");
		}
	}

	ALOGI("Starting qcom system daemon");
	memset(buffer, '\0', sizeof(buffer));
	if (pipe(qcom_system_daemon_pipe) < 0) {
		ALOGE("Failed to create pipe: %s",
				strerror(errno));
		return -1;
	}
	binit_Success = Diag_LSM_Init(NULL);

	if (!binit_Success) {
		ALOGE(" Diag_LSM_Init failed : %d\n",binit_Success);
		close(qcom_system_daemon_pipe[0]);
		close(qcom_system_daemon_pipe[1]);
		return -1;
	}

	DIAGPKT_DISPATCH_TABLE_REGISTER_V2_DELAY(VERSION_DIAGPKT_PROCID,
			VERSION_DIAGPKT_SUBSYS, diagpkt_tbl);
	DIAGPKT_DISPATCH_TABLE_REGISTER(DIAG_SUBSYS_FTM ,
			ftm_table);
	DIAGPKT_DISPATCH_TABLE_REGISTER(DIAG_SUBSYS_SYSTEM_OPERATIONS,
			system_operations_table);
	property_get("ro.bootmode", bootmode, NULL);
	property_get("diag.reset_handler", diag_reset_handler, NULL);

	if (!strncmp(bootmode, "ffbm", 4) || !strncmp(diag_reset_handler, "true", 4)) {
		ALOGI("Registering mode reset handler");
		DIAGPKT_DISPATCH_TABLE_REGISTER(DIAGPKT_NO_SUBSYS_ID,
				ftm_ffbm_mode_table);
	}
	do {
		fds[0].fd = qcom_system_daemon_pipe[0];
		fds[0].events = POLLIN;
		fds[0].revents = 0;
		if (is_factory) {
			int fifo;
			if ((fifo = open(SOS_FIFO, O_RDONLY | O_NONBLOCK)) > 0) {
				fds[1].fd = fifo;
				fds[1].events = POLLIN;
				fds[1].revents = 0;
				nfds = 2;
			}
		}
		// start polling on fds
		ret = poll(fds, nfds, -1);
		if (ret < 0) {
			ALOGE("Failed to poll: %s", strerror(errno));
			goto error;
		}
		if ((fds[0].revents & POLLIN)) {
			do {
				bytes_read = read(qcom_system_daemon_pipe[0],
						buffer + offset,
						(FFBM_COMMAND_BUFFER_SIZE-1) - offset);
				if (bytes_read < 0) {
					ALOGE("Failed to read command from pipe : %s",
						strerror(errno));
				goto error;
				}
			offset += bytes_read;
			} while (bytes_read > 0 && offset <
				(FFBM_COMMAND_BUFFER_SIZE - 1));
			if (!strncmp(buffer, REBOOT_CMD, sizeof(REBOOT_CMD))) {
				ALOGW("Got request to reboot.Shutting down modem");
				subsystem_control_shutdown(PROC_MSM);
				ALOGW("modem shutdown complete");
				sleep(2);
				android_reboot(ANDROID_RB_RESTART, 0, 0);
			} else if (!strncmp(buffer, EDL_REBOOT_CMD,
				sizeof(EDL_REBOOT_CMD))) {
				ALOGW("Got request for EDL reboot.Shutting down modem");
				subsystem_control_shutdown(PROC_MSM);
				ALOGW("modem shutdown complete");
				sleep(2);
				android_reboot(ANDROID_RB_RESTART2, 0, "edl");
			}
		}
		if ((fds[1].revents & POLLIN)) {
			bytes_read = read(fds[1].fd, buf, sizeof(buf)-1);
			if (bytes_read <= 0) {
				ALOGE("Failed to read command from pipe : %s",
				strerror(errno));
			} else {
				buf[bytes_read-1] = '\0';
				if (!strncmp(buf, SWITCH_OS_CMD,
						sizeof(SWITCH_OS_CMD))) {
					ALOGI("Got request to switch OS");
					if (reset_os_selector_bit(&bootselect_info)) {
						ALOGE("Error updating bootselect partition");
						continue;
					} else {
						android_reboot(ANDROID_RB_RESTART, 0, 0);
					}
				}
			}
		}
error:
		ALOGE("Unrecognised command");
		bytes_read = 0;
		offset = 0;
	} while(1);
	return 0;
}
