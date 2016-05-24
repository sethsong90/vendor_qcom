/*
 *
 * This library contains the API to load the audio calibration
 * data from database and push to the DSP
 *
 * Copyright (c) 2012-2014 QUALCOMM Technologies, Inc. All Rights Reserved.
 * QUALCOMM Technologies Proprietary and Confidential.
 *
 */

#include <sys/stat.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <dirent.h>
#include <linux/msm_ion.h>
#include <linux/msm_audio.h>
#include <linux/msm_audio_acdb.h>
#include <linux/mfd/wcd9xxx/wcd9320_registers.h>

#define u32 uint32_t
#define u16 uint16_t
#define u8 uint8_t

#include <linux/mfd/msm-adie-codec.h>
#include <sys/mman.h>

#ifdef _ANDROID_
#include <cutils/properties.h>
/* definitions for Android logging */
#include <utils/Log.h>
#include "common_log.h"
#else /* _ANDROID_ */
#define LOGI(...)      fprintf(stdout,__VA_ARGS__)
#define LOGE(...)      fprintf(stderr,__VA_ARGS__)
#define LOGV(...)      fprintf(stderr,__VA_ARGS__)
#define LOGD(...)      fprintf(stderr,__VA_ARGS__)
#endif /* _ANDROID_ */

#include "acdb.h"
#include "acph.h"
#include "acdb-rtac.h"
#ifdef _ANDROID_
#include "adie-rtac.h"
#endif /* _ANDROID_ */
#include "acdb-loader.h"
#include "acdb-anc-general.h"
#include "acdb-anc-taiko.h"
#include "acdb-id-mapper.h"
#include "acdb-loader-def.h"
#undef LOG_NDDEBUG
#undef LOG_TAG
#define LOG_NDDEBUG 0
#define LOG_TAG "ACDB-LOADER"

#define INVALID_DATA	-1

#define TEMP_CAL_BUFSZ 1024 /* 1K should be plenty */
#define MAX_COL_SIZE		324
#define MAX_ACDB_FILES		20

#define EC_REF_RX_DEVS (sizeof(uint32_t) * 20)

#define round(val) ((val > 0) ? (val + 0.5) : (val - 0.5))

enum {
	RX_CAL,
	TX_CAL,
	MAX_AUDPROC_TYPES
};

#ifdef _ANDROID_
static mode_t PERM644 = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

static int get_files_from_properties(AcdbInitCmdType *acdb_init_cmd)
{
	int i = 0;
	int prop_len;
	char prop_name[21];

	for (i=0; i < MAX_ACDB_FILES; i++) {
		if (snprintf(prop_name, sizeof(prop_name), "persist.audio.calfile%d", i) < 0)
			goto done;

		prop_len = property_get(prop_name, acdb_init_cmd->acdbFiles[i].fileName, NULL);
		if (prop_len <= 0)
			goto done;

		acdb_init_cmd->acdbFiles[i].fileNameLen = strlen(acdb_init_cmd->acdbFiles[i].fileName);
		LOGD("ACDB -> Prop Load file: %s\n", acdb_init_cmd->acdbFiles[i].fileName);
	}
done:
	acdb_init_cmd->nNoOfFiles = i;
	return i;
}
#else
static int get_files_from_properties(AcdbInitCmdType *acdb_init_cmd) {return -ENODEV;}
#endif

static int		acdb_handle;
static int		ion_handle;
static int		alloc_handle;
static int		map_handle;
static uint8_t		*virt_cal_buffer = NULL;
static uint32_t		*mad_buf;
static uint32_t		current_feature_set;
static uint32_t		current_voice_tx_acdb_id;
static uint32_t		current_voice_rx_acdb_id;
static int		global_offset;
static uint32_t		voice_col_data[(MAX_COL_SIZE + sizeof(int))];

static unsigned int	audproc_ioctl[] = {AUDIO_SET_AUDPROC_RX_CAL,
						AUDIO_SET_AUDPROC_TX_CAL};
static unsigned int	audvol_ioctl[] = {AUDIO_SET_AUDPROC_RX_VOL_CAL,
						AUDIO_SET_AUDPROC_TX_VOL_CAL};
static unsigned int	audtop_ioctl[] = {AUDIO_SET_ADM_RX_TOPOLOGY,
						AUDIO_SET_ADM_TX_TOPOLOGY};
static unsigned int	afe_ioctl[] = {AUDIO_SET_AFE_RX_CAL,
						AUDIO_SET_AFE_TX_CAL};
static unsigned int	voc_col_ioctl[] = {AUDIO_SET_VOCPROC_COL_CAL,
						AUDIO_SET_VOCVOL_COL_CAL,
						AUDIO_SET_VOCSTRM_COL_CAL};

pthread_mutex_t loader_mutex = PTHREAD_MUTEX_INITIALIZER;

static int send_asm_topology(void);
static int send_adm_custom_topology(void);
static int send_asm_custom_topology(void);
int acdb_loader_send_wcd9xxx_anc_cal(int acdb_id, int file_descriptor);
void send_mbhc_data(void);


static int get_files_from_device_tree(AcdbInitCmdType *acdb_init_cmd)
{
	int result = 0;
	int result2 = 0;
	int i = 0;
	int prop_len;
	char dir_path[300];
	char board_type[64] = DEFAULT_ACDB_BOARD;
	struct dirent *dentry;
	DIR *dir_fp = NULL;
	FILE *fp = NULL;

	/* Get Board type */
	fp = fopen("/sys/devices/soc0/hw_platform","r");
	if (fp == NULL)
		fp = fopen("/sys/devices/system/soc/soc0/hw_platform","r");
	if (fp == NULL)
		LOGE("ACDB -> Error: Couldn't open hw_platform\n");
	else if (fgets(board_type, sizeof(board_type), fp) == NULL)
		LOGE("ACDB -> Error: Couldn't get board type\n");
	else if (board_type[(strlen(board_type) - 1)] == '\n')
		board_type[(strlen(board_type) - 1)] = '\0';

	/* Set DIR Path */
	result = snprintf(dir_path, sizeof(dir_path), "%s%s", ACDB_BIN_PATH, board_type);
	if (result < 0) {
		LOGE("ACDB -> Error: snprintf failed error: %d\n", result);
		result = snprintf(dir_path, sizeof(dir_path), "%s%s", ACDB_BIN_PATH, DEFAULT_ACDB_BOARD);
		if (result < 0) {
			LOGE("ACDB -> Error: snprintf failed for defualt dir, error: %d\n", result);
			goto done;
		}
	}

	/* Open Directory */
	dir_fp = opendir(dir_path);
	if (dir_fp == NULL) {
		/* Set default DIR Path */
		result = snprintf(dir_path, sizeof(dir_path), "%s%s", ACDB_BIN_PATH, DEFAULT_ACDB_BOARD);
		if (result < 0) {
			LOGE("ACDB -> Error: snprintf failed for defualt dir, error: %d\n", result);
			goto done;
		}
		dir_fp = opendir(dir_path);
		if (dir_fp == NULL) {
			LOGE("ACDB -> Error: directory open failed for %s\n", dir_path);
			result = -ENODEV;
			goto done;
		}
	}

	while ((dentry = readdir(dir_fp)) != NULL) {
		if (strstr(dentry->d_name, ".acdb") != NULL) {
			result2 = snprintf(acdb_init_cmd->acdbFiles[i].fileName,
					sizeof(acdb_init_cmd->acdbFiles[i].fileName),
					"%s/%s", dir_path, dentry->d_name);
			if (result2 < 0) {
				result = result2;
				LOGD("ACDB -> Error: Snprintf load file failed: %s/%s, err %d\n",
					dir_path, acdb_init_cmd->acdbFiles[i].fileName, result);
				continue;
			}

			acdb_init_cmd->acdbFiles[i].fileNameLen =
				strlen(acdb_init_cmd->acdbFiles[i].fileName);
			LOGD("ACDB -> Load file: %s\n", acdb_init_cmd->acdbFiles[i].fileName);
			i++;
			if (i >= MAX_ACDB_FILES) {
				LOGD("ACDB -> Maximum number of ACDB files hit, %d!\n", i);
				break;
			}
		}
	}
done:
	if (fp != NULL)
		fclose(fp);
	if (dir_fp != NULL)
		closedir(dir_fp);
	acdb_init_cmd->nNoOfFiles = i;
	return result;
}

static int acdb_load_files(AcdbInitCmdType *acdb_init_cmd)
{
	int result = 0;

	result = get_files_from_properties(acdb_init_cmd);
	if (result > 0)
		goto done;

	result = get_files_from_device_tree(acdb_init_cmd);

done:
	return result;
}

int acdb_loader_init_ACDB(void)
{
	int				result = 0;
	int				i;
	AcdbInitCmdType			acdb_init_cmd;
	struct ion_fd_data		fd_data;
	struct ion_allocation_data	alloc_data;

	if (acdb_load_files(&acdb_init_cmd) <= 0) {
		LOGE("ACDB -> Could not load .acdb files!\n");
		goto done;
	}

	LOGD("ACDB -> ACDB_CMD_INITIALIZE_V2\n");
	result = acdb_ioctl(ACDB_CMD_INITIALIZE_V2,
		(const uint8_t *)&acdb_init_cmd, sizeof(acdb_init_cmd), NULL, 0);
	if (result) {
		LOGE("Error initializing ACDB returned = %d\n", result);
		goto done;
	}

	LOGD("ACDB -> ACPH INIT\n");
	result = acph_init();
	if (result) {
		LOGE("Error initializing ACPH returned = %d\n", result);
		goto done;
	}

	LOGD("ACDB -> RTAC INIT\n");
	acdb_rtac_init();

#ifdef _ANDROID_
	LOGD("ACDB -> ADIE RTAC INIT\n");
	adie_rtac_init();
#endif /* _ANDROID_ */

	acdb_handle = open("/dev/msm_acdb", O_RDWR);
	if (acdb_handle < 0) {
		LOGE("Cannot open /dev/msm_acdb errno: %d\n", errno);
		goto done;
	}

	if (map_handle) {
		LOGD("ACDB -> MMAP MEM from ACDB driver\n");
		virt_cal_buffer = (uint8_t *)mmap(0, ACDB_BUFFER_SIZE,
			PROT_READ | PROT_WRITE,	MAP_SHARED, map_handle, 0);
	}


	if (virt_cal_buffer == 0) {
		LOGD("No existing ION info in ACDB driver\n");

		ion_handle = open("/dev/ion", O_RDONLY | O_DSYNC);
		if (ion_handle < 0) {
			LOGE("Cannot open /dev/ion errno: %d\n", ion_handle);
			goto err_acdb;
		}

		LOGD("ACDB -> ION_IOC_ALLOC\n");
		alloc_data.len = ACDB_BUFFER_SIZE;
		alloc_data.align = 0x1000;
#ifdef QCOM_AUDIO_USE_SYSTEM_HEAP_ID
		alloc_data.heap_mask = ION_HEAP(ION_SYSTEM_HEAP_ID);
#else
		alloc_data.heap_mask = ION_HEAP(ION_AUDIO_HEAP_ID);
#endif
		alloc_data.flags = 0;
		result = ioctl(ion_handle, ION_IOC_ALLOC, &alloc_data);
		if (result) {
			LOGE("ION_ALLOC errno: %d\n", result);
			goto err_alloc;
		}

		LOGD("ACDB -> ION_IOC_SHARE\n");
		fd_data.handle = alloc_data.handle;
		alloc_handle = (int)alloc_data.handle;
		result = ioctl(ion_handle, ION_IOC_SHARE, &fd_data);
		if (result) {
			LOGE("ION_IOC_SHARE errno: %d\n", result);
			goto err_share;
		}

		LOGD("ACDB -> MMAP ADDR\n");
		map_handle = fd_data.fd;
		virt_cal_buffer = (uint8_t *)mmap(0, ACDB_BUFFER_SIZE,
			PROT_READ | PROT_WRITE, MAP_SHARED, map_handle, 0);
		if (virt_cal_buffer == MAP_FAILED) {
			LOGE("Cannot allocate ION\n");
			goto err_map;
		}

		LOGD("ACDB -> register MEM to ACDB driver: 0x%x\n",
			 (uint32_t)virt_cal_buffer);

		result = ioctl(acdb_handle, AUDIO_REGISTER_PMEM, &map_handle);
		if (result < 0) {
			LOGE("Cannot register PMEM to ACDB driver\n");
			goto err_reg;
		}

		mad_buf = malloc(ACDB_BLOCK_SIZE);
		if (mad_buf == NULL) {
			LOGE("ACDB -> Cannot allocate MAD buffer\n");
			goto err_reg;
		}
	} else {
		LOGD("ACDB -> use MEM from ACDB driver: 0x%x\n", (uint32_t)virt_cal_buffer);
	}

	/* See ../inc/<target>/acdb-loader-def.h for cal block allocation */

	send_asm_topology();
	send_adm_custom_topology();
	send_asm_custom_topology();
	send_mbhc_data();
	send_wcd9xxx_anc_data();
	current_feature_set = ACDB_VOCVOL_FID_DEFAULT;
	LOGD("ACDB -> init done!\n");
	return 0;

err_reg:
	munmap(virt_cal_buffer, ACDB_BUFFER_SIZE);
err_map:
	close(map_handle);
err_share:
	result = ioctl(ion_handle, ION_IOC_FREE, &alloc_handle);
	if (result)
		LOGE("ION_IOC_FREE errno: %d\n", result);
err_alloc:
	close(ion_handle);
err_acdb:
	close(acdb_handle);
done:
	return result;
}

static int get_audcal_path(uint32_t capability)
{
	int path;

	if (capability & MSM_SNDDEV_CAP_RX)
		path = RX_CAL;
	else if (capability & MSM_SNDDEV_CAP_TX)
		path = TX_CAL;
	else
		path = INVALID_DATA;

	return path;
}

static uint32_t get_samplerate(int  acdb_id)
{
	uint32_t sample_rate = 48000;

	if (((uint32_t)acdb_id == DEVICE_BT_SCO_RX_ACDB_ID) ||
		((uint32_t)acdb_id == DEVICE_BT_SCO_TX_ACDB_ID)) {
		sample_rate = 8000;
	} else if (((uint32_t)acdb_id == DEVICE_BT_SCO_RX_WB_ACDB_ID) ||
		((uint32_t)acdb_id == DEVICE_BT_SCO_TX_WB_ACDB_ID)) {
		sample_rate = 16000;
                /*To change to 16000HZ*/
        }

	return sample_rate;
}

static uint32_t get_adm_topology(int acdb_id)
{
	int32_t				result = 0;
	AcdbGetAudProcTopIdCmdType	acdb_get_top;
	AcdbGetTopologyIdRspType	audio_top;

	acdb_get_top.nDeviceId = acdb_id;
	acdb_get_top.nApplicationType = ACDB_APPTYPE_GENERAL_PLAYBACK;

	LOGD("ACDB -> ACDB_CMD_GET_AUDPROC_COMMON_TOPOLOGY_ID\n");

	result = acdb_ioctl(ACDB_CMD_GET_AUDPROC_COMMON_TOPOLOGY_ID,
		(const uint8_t *)&acdb_get_top, sizeof(acdb_get_top),
		(uint8_t *)&audio_top, sizeof(audio_top));
	if (result) {
		LOGE("Error: ACDB get adm topology for acdb id = %d, returned = %d\n",
		     acdb_id, result);
		goto err;
	}
	return audio_top.nTopologyId;
err:
	return 0;
}

static uint32_t get_asm_topology(void)
{
	int32_t					result = 0;
	AcdbGetAudProcStrmTopIdCmdType		acdb_get_top;
	AcdbGetTopologyIdRspType		audio_top;

	acdb_get_top.nApplicationType = ACDB_APPTYPE_GENERAL_PLAYBACK;

	LOGD("ACDB -> ACDB_CMD_GET_AUDPROC_STREAM_TOPOLOGY_ID\n");

	result = acdb_ioctl(ACDB_CMD_GET_AUDPROC_STREAM_TOPOLOGY_ID,
		(const uint8_t *)&acdb_get_top, sizeof(acdb_get_top),
		(uint8_t *)&audio_top, sizeof(audio_top));
	if (result) {
		LOGE("Error: ACDB get asm topology returned = %d\n",
		     result);
		goto err;
	}
	return audio_top.nTopologyId;
err:
	return 0;
}

static int get_adm_custom_topology(struct cal_block *block)
{
	int32_t				result = 0;
	AcdbQueryCmdType		acdb_top;
	AcdbQueryResponseType		response;
	uint32_t			*addr =
		(uint32_t *)(virt_cal_buffer + ACDB_ADM_CUST_TOP_OFFSET
		* ACDB_BLOCK_SIZE);

	acdb_top.nBufferLength = ACDB_BLOCK_SIZE;
	acdb_top.pBufferPointer = (uint8_t *)addr;

	LOGD("ACDB -> ACDB_CMD_GET_AUDIO_COPP_TOPOLOGIES\n");

	result = acdb_ioctl(ACDB_CMD_GET_AUDIO_COPP_TOPOLOGIES,
		(const uint8_t *)&acdb_top, sizeof(acdb_top),
		(uint8_t *)&response, sizeof(response));
	if (result) {
		LOGE("Error: ACDB get adm topologies returned = %d\n",
		     result);
		goto done;
	}

	block->cal_size = response.nBytesUsedInBuffer;
	block->cal_offset = ACDB_ADM_CUST_TOP_OFFSET * ACDB_BLOCK_SIZE;
done:
	return result;;
}

static int get_asm_custom_topology(struct cal_block *block)
{
	int32_t				result = 0;
	AcdbQueryCmdType		acdb_top;
	AcdbQueryResponseType		response;
	uint32_t			*addr =
		(uint32_t *)(virt_cal_buffer + ACDB_ASM_CUST_TOP_OFFSET
		* ACDB_BLOCK_SIZE);

	acdb_top.nBufferLength = ACDB_BLOCK_SIZE;
	acdb_top.pBufferPointer = (uint8_t *)addr;

	LOGD("ACDB -> ACDB_CMD_GET_AUDIO_POPP_TOPOLOGIES\n");

	result = acdb_ioctl(ACDB_CMD_GET_AUDIO_POPP_TOPOLOGIES,
		(const uint8_t *)&acdb_top, sizeof(acdb_top),
		(uint8_t *)&response, sizeof(response));
	if (result) {
		LOGE("Error: ACDB get asm topologies returned = %d\n",
		     result);
		goto done;
	}

	block->cal_size = response.nBytesUsedInBuffer;
	block->cal_offset = ACDB_ASM_CUST_TOP_OFFSET * ACDB_BLOCK_SIZE;
done:
	return result;
}


static int get_audtable(int acdb_id, int path, struct cal_block *block)
{
	int32_t				result = 0;
	AcdbAudProcTableCmdType		audtable;
	AcdbQueryResponseType		response;
	/* Point address to designated PMEM block */
	uint32_t			*addr =
		(uint32_t *)(virt_cal_buffer + (NUM_VOCPROC_BLOCKS + path)
		* ACDB_BLOCK_SIZE);

	audtable.nDeviceId = acdb_id;
	audtable.nDeviceSampleRateId = get_samplerate(acdb_id);
	audtable.nApplicationType = ACDB_APPTYPE_GENERAL_PLAYBACK;
	audtable.nBufferPointer = (uint8_t *)addr;

	/* Set larger buffer for TX due to Quad-mic */
	if (path)
		audtable.nBufferLength = MAX_BLOCK_SIZE;
	else
		audtable.nBufferLength = ACDB_BLOCK_SIZE;

	LOGD("ACDB -> ACDB_CMD_GET_AUDPROC_COMMON_TABLE\n");

	result = acdb_ioctl(ACDB_CMD_GET_AUDPROC_COMMON_TABLE,
		(const uint8_t *)&audtable, sizeof(audtable),
		(uint8_t *)&response, sizeof(response));
	if (result) {
		LOGE("Error: ACDB audproc returned = %d\n", result);
		block->cal_size = 0;
		goto done;
	}

	block->cal_size = response.nBytesUsedInBuffer;
	block->cal_offset = (NUM_VOCPROC_BLOCKS + path) * ACDB_BLOCK_SIZE;
done:
	return result;
}

static int get_audvoltable(int acdb_id, int path, struct cal_block *block)
{
	int32_t					result = 0;
	AcdbAudProcGainDepVolTblStepCmdType	audvoltable;
	AcdbQueryResponseType			response;
	/* Point address to designated PMEM block */
	uint32_t				*addr =
		(uint32_t *)(virt_cal_buffer + (ACDB_AUDVOL_OFFSET + path)
		* ACDB_BLOCK_SIZE);

	audvoltable.nDeviceId = acdb_id;
	audvoltable.nApplicationType = ACDB_APPTYPE_GENERAL_PLAYBACK;
	/* 0 is max volume which is default Q6 COPP volume */
	audvoltable.nVolumeIndex = 0;
	audvoltable.nBufferLength = ACDB_BLOCK_SIZE;
	audvoltable.nBufferPointer = (uint8_t *)addr;

	LOGD("ACDB -> ACDB_CMD_GET_AUDPROC_GAIN_DEP_STEP_TABLE\n");

	result = acdb_ioctl(ACDB_CMD_GET_AUDPROC_GAIN_DEP_STEP_TABLE,
		(const uint8_t *)&audvoltable, sizeof(audvoltable),
		(uint8_t *)&response, sizeof(response));
	if (result) {
		LOGE("Error: ACDB AudProc vol returned = %d\n", result);
		block->cal_size = 0;
		goto done;
	}

	block->cal_size = response.nBytesUsedInBuffer;
	block->cal_offset = (ACDB_AUDVOL_OFFSET + path) * ACDB_BLOCK_SIZE;
done:
	return result;
}

static int get_afetable(int acdb_id, int path, struct cal_block *block)
{
	int32_t					result = 0;
	AcdbAfeCommonTableCmdType		afetable;
	AcdbQueryResponseType			response;
	/* Point address to designated PMEM block */
	uint32_t				*addr =
		(uint32_t *)(virt_cal_buffer + (ACDB_AFE_OFFSET + path)
		* ACDB_BLOCK_SIZE);

	afetable.nDeviceId = acdb_id;
	/* Does not accept ACDB sample rate bit mask */
	afetable.nSampleRateId = get_samplerate(acdb_id);
	afetable.nBufferLength = ACDB_BLOCK_SIZE;
	afetable.nBufferPointer = (uint8_t *)addr;

	LOGD("ACDB -> ACDB_CMD_GET_AFE_COMMON_TABLE\n");

	result = acdb_ioctl(ACDB_CMD_GET_AFE_COMMON_TABLE,
		(const uint8_t *)&afetable, sizeof(afetable),
		(uint8_t *)&response, sizeof(response));
	if (result) {
		LOGE("Error: ACDB AFE returned = %d\n", result);
		block->cal_size = 0;
		goto done;
	}

	block->cal_size = response.nBytesUsedInBuffer;
	block->cal_offset = (ACDB_AFE_OFFSET + path) * ACDB_BLOCK_SIZE;
done:
	return result;
}

static int get_lsm_table(uint32_t acdb_id, uint32_t app_type, struct cal_block *block)
{
	int32_t					result = 0;
	AcdbLsmTableCmdType			lsm_table;
	AcdbQueryResponseType			response;
	/* Point address to designated PMEM block */
	uint32_t				*addr =
		(uint32_t *)(virt_cal_buffer + ACDB_LSM_OFFSET
		* ACDB_BLOCK_SIZE);

	lsm_table.nDeviceId = acdb_id;
	lsm_table.nMadApplicationType = app_type;
	lsm_table.nBufferLength = ACDB_BLOCK_SIZE;
	lsm_table.nBufferPointer = (uint8_t *)addr;

	LOGD("ACDB -> ACDB_CMD_GET_LSM_TABLE\n");

	result = acdb_ioctl(ACDB_CMD_GET_LSM_TABLE,
		(const uint8_t *)&lsm_table, sizeof(lsm_table),
		(uint8_t *)&response, sizeof(response));
	if (result) {
		LOGE("Error: ACDB LSM returned = %d\n", result);
		block->cal_size = 0;
		goto done;
	}

	block->cal_size = response.nBytesUsedInBuffer;
	block->cal_offset = ACDB_LSM_OFFSET * ACDB_BLOCK_SIZE;
done:
	return result;
}

int acdb_loader_get_ecrx_device(int acdb_id)
{
       int32_t result = INVALID_DATA;
       uint32_t *pRxDevs;
       AcdbAudioRecRxListCmdType acdb_cmd;
       AcdbAudioRecRxListRspType acdb_cmd_response;

       acdb_cmd.nTxDeviceId = acdb_id;
       pRxDevs = malloc(EC_REF_RX_DEVS);
       if (pRxDevs == NULL) {
           LOGE("Error: %s Malloc Failed", __func__);
           return result;
       }
       acdb_cmd_response.pRxDevs = pRxDevs;
       result = acdb_ioctl(ACDB_CMD_GET_AUDIO_RECORD_RX_DEVICE_LIST,
                          (const uint8_t *)&acdb_cmd, sizeof(acdb_cmd),
                 (uint8_t *)&acdb_cmd_response, sizeof(acdb_cmd_response));
       if (result) {
               LOGE("Error: ACDB EC_REF_RX returned = %d\n", result);
               goto done;
       }

       if (acdb_cmd_response.nNoOfRxDevs) {
            result = acdb_cmd_response.pRxDevs[0];
       }

done:
       free(pRxDevs);
       return result;
}

static int get_aanctable(int acdb_id, struct cal_block *block)
{
	int32_t					result = 0;
	AcdbAANCConfigTableCmdType		aanctable;
	AcdbQueryResponseType			response;
	/* Point address to designated PMEM block */
	uint32_t				*addr =
		(uint32_t *)(virt_cal_buffer + (ACDB_AANC_OFFSET)
		* ACDB_BLOCK_SIZE);

	aanctable.nTxDeviceId = acdb_id;
	aanctable.nBufferLength = ACDB_BLOCK_SIZE;
	aanctable.nBufferPointer = (uint8_t *)addr;

	LOGD("ACDB -> ACDB_CMD_GET_ADAPTIVE_ANC_CONFIG_TABLE\n");

	result = acdb_ioctl(ACDB_CMD_GET_ADAPTIVE_ANC_CONFIG_TABLE,
		(const uint8_t *)&aanctable, sizeof(aanctable),
		(uint8_t *)&response, sizeof(response));
	LOGD("ACDB_CMD_GET_ADAPTIVE_ANC_CONFIG_TABLE result %d\n",
		result);

	if (result) {
		LOGE("Error: ACDB AANC returned = %d\n", result);
		block->cal_size = 0;
		goto done;
	}

	block->cal_size = response.nBytesUsedInBuffer;
	block->cal_offset = (ACDB_AANC_OFFSET)  * ACDB_BLOCK_SIZE;
done:
	return result;
}

static int send_adm_topology(int acdb_id, int path)
{
	int32_t		result = 0;
	uint32_t	topology = 0;

	LOGD("ACDB -> send_adm_topology\n");

	topology = get_adm_topology(acdb_id);

	if (!topology) {
		result = -ENODEV;
		goto done;
	}

	result = ioctl(acdb_handle, audtop_ioctl[path],
		(unsigned int)&topology);
	if (result) {
		LOGE("Error: Sending ACDB adm topology result = %d\n", result);
		goto done;
	}
done:
	return result;
}

static int send_asm_topology(void)
{
	int32_t		result = 0;
	uint32_t	topology = 0;

	LOGD("ACDB -> send_asm_topology\n");

	topology = get_asm_topology();

	if (!topology) {
		result = -ENODEV;
		goto done;
	}

	result = ioctl(acdb_handle, AUDIO_SET_ASM_TOPOLOGY,
		(unsigned int)&topology);
	if (result) {
		LOGE("Error: Sending ACDB asm topology result = %d\n", result);
		goto done;
	}
done:
	return result;
}

static int send_adm_custom_topology(void)
{
	int32_t				result = 0;
	/* Size buffer to store one cal block (64B) */
	/* and its size as the first element */
	uint32_t			adm_top[3] = {0,0,0};
	LOGD("ACDB -> send_adm_custom_topology\n");

	get_adm_custom_topology((struct cal_block *)&adm_top[1]);

	/* Set size of cal data sent */
	adm_top[0] = sizeof(struct cal_block);

	LOGD("ACDB -> AUDIO_SET_ADM_CUSTOM_TOPOLOGY\n");

	result = ioctl(acdb_handle, AUDIO_SET_ADM_CUSTOM_TOPOLOGY,
		(unsigned int)&adm_top);
	if (result) {
		LOGE("Error: Sending ACDB ADM topology result = %d\n", result);
		goto done;
	}
done:
	return result;
}

static int send_asm_custom_topology(void)
{
	int32_t				result = 0;
	/* Size buffer to store one cal block (64B) */
	/* and its size as the first element */
	uint32_t			asm_top[3] = {0,0,0};
	LOGD("ACDB -> send_asm_custom_topology\n");

	get_asm_custom_topology((struct cal_block *)&asm_top[1]);

	/* Set size of cal data sent */
	asm_top[0] = sizeof(struct cal_block);

	LOGD("ACDB -> AUDIO_SET_ASM_CUSTOM_TOPOLOGY\n");

	result = ioctl(acdb_handle, AUDIO_SET_ASM_CUSTOM_TOPOLOGY,
		(unsigned int)&asm_top);
	if (result) {
		LOGE("Error: Sending ACDB ASM topology result = %d\n", result);
		goto done;
	}
done:
	return result;
}

static int send_audtable(int acdb_id, int path)
{
	int32_t				result = 0;
	/* Size buffer to store one cal block (64B) */
	/* and its size as the first element */
	uint32_t			audproc_cal[3] = {0,0,0};
	LOGD("ACDB -> send_audtable\n");

	get_audtable(acdb_id, path, (struct cal_block *)&audproc_cal[1]);

	/* Set size of cal data sent */
	audproc_cal[0] = sizeof(struct cal_block);

	LOGD("ACDB -> AUDIO_SET_AUDPROC_CAL\n");

	result = ioctl(acdb_handle, audproc_ioctl[path],
		(unsigned int)&audproc_cal);
	if (result) {
		LOGE("Error: Sending ACDB audproc result = %d\n", result);
		goto done;
	}
done:
	return result;
}

static int send_audvoltable(int acdb_id, int path)
{
	int32_t			result = 0;
	/* Size buffer to store one cal block (64B) */
	/* and its size as the first element */
	uint32_t		audvol_cal[3] = {0,0,0};
	LOGD("ACDB -> send_audvoltable\n");

	get_audvoltable(acdb_id, path, (struct cal_block *)&audvol_cal[1]);

	/* Set size of cal data sent */
	audvol_cal[0] = sizeof(struct cal_block);

	LOGD("ACDB -> AUDIO_SET_AUDPROC_VOL_CAL\n");
	result = ioctl(acdb_handle, audvol_ioctl[path],
		(unsigned int)&audvol_cal);
	if (result) {
		LOGE("Error: Sending ACDB audproc vol result = %d\n", result);
		goto done;
	}
done:
	return result;
}

static int32_t valid_aanc_device(int acdb_id)
{
	int32_t result = false;

	if ((acdb_id == DEVICE_HANDSET_TX_AANC_ACDB_ID) ||
		(acdb_id == DEVICE_DUALMIC_HANDSET_TX_ENDFIRE_AANC_ACDB_ID))
		result = true;

	LOGD("Valid AANC Device for device %d is %d\n", acdb_id, result);
	return result;
}

static int send_aanctable(int acdb_id)
{
	int32_t			result = 0;
	/* Size buffer to store one cal block (64B) */
	/* and its size as the first element */
	uint32_t		aanc_table[3] = {0, 0, 0};

	LOGD("ACDB -> send_aanctable\n");
	get_aanctable(acdb_id, (struct cal_block *)&aanc_table[1]);

	/* Set size of cal data sent */
	aanc_table[0] = sizeof(struct cal_block);

	LOGD("ACDB -> AUDIO_SET_AANC_TABLE\n");
	result = ioctl(acdb_handle, AUDIO_SET_AANC_CAL,
		(unsigned int)&aanc_table);
	if (result) {
		LOGE("Error: Sending ACDB AANC result = %d\n", result);
		goto done;
	}
done:
	return result;
}

static int32_t valid_afe_cal(int acdb_id)
{
	int32_t	result = false;

	if ((acdb_id == DEVICE_HEADSET_TX_ACDB_ID) ||
		(acdb_id == DEVICE_SPEAKER_RX_ACDB_ID) ||
		(acdb_id == DEVICE_SPEAKER_MONO_RX_ACDB_ID) ||
		(acdb_id == DEVICE_HANDSET_TX_MONO_LISTEN_LOW_ACDB_ID) ||
		(acdb_id == DEVICE_SPEAKER_MONO_RX_PROT_ACDB_ID) ||
		(acdb_id == DEVICE_SPEAKER_MONO_TX_PROT_ACDB_ID) ||
		(acdb_id == DEVICE_HANDSET_MONO_LISTEN_HIGH_POWER_ACDB_ID))
		result = true;

	return result;
}

static int send_afe_cal(int acdb_id, int path)
{
	int32_t			result = 0;
	/* Size buffer to store one cal block (64B) */
	/* and its size as the first element */
	uint32_t		afe_cal[3] = {0,0,0};

	/* Still send cal to clear out old cal */
	/* since afe cal is not for all devices */
	if (valid_afe_cal(acdb_id)) {
		LOGD("ACDB -> send_afe_cal\n");
		get_afetable(acdb_id, path, (struct cal_block *)&afe_cal[1]);
	}

	/* Set size of cal data sent */
	afe_cal[0] = sizeof(struct cal_block);

	LOGD("ACDB -> AUDIO_SET_AFE_CAL\n");
	result = ioctl(acdb_handle, afe_ioctl[path],
		(unsigned int)&afe_cal);
	if (result) {
		LOGE("Error: Sending ACDB AFE result = %d\n", result);
		goto done;
	}
done:
	return result;
}

static int send_lsm_cal(int acdb_id, int app_id)
{
	int32_t			result = 0;
	/* Size buffer to store one cal block (64B) */
	/* and its size as the first element */
	uint32_t		lsm_cal[3] = {0,0,0};

	LOGD("ACDB -> get_lsm_table\n");
	get_lsm_table(acdb_id, app_id, (struct cal_block *)&lsm_cal[1]);

	/* Set size of cal data sent */
	lsm_cal[0] = sizeof(struct cal_block);

	LOGD("ACDB -> AUDIO_SET_LSM_CAL\n");
	result = ioctl(acdb_handle, AUDIO_SET_LSM_CAL,
		(unsigned int)&lsm_cal);
	if (result) {
		LOGE("Error: Sending ACDB LSM result = %d\n", result);
		goto done;
	}
done:
	return result;
}

int send_codec_cal(int acdb_id)
{
	int				result = 0;
	int				mad_fd;
	AcdbCodecCalDataCmdType		codec_table;
	AcdbQueryResponseType		response;

	LOGD("ACDB -> send_codec_cal\n");
	mad_fd = creat(MAD_BIN_PATH, PERM644);
	if (mad_fd < 0)
	{
		LOGE("Error opening MAD file %d\n", errno);
		result = -ENOENT;
		goto done;
	}

	codec_table.nDeviceID = acdb_id;
	codec_table.nCodecFeatureType = ACDB_WCD9320_MAD;
	codec_table.nBufferLength = ACDB_BLOCK_SIZE;
	codec_table.pBufferPointer = (uint8_t *)mad_buf;

	LOGD("ACDB -> ACDB_CMD_GET_CODEC_CAL_DATA\n");

	result = acdb_ioctl(ACDB_CMD_GET_CODEC_CAL_DATA,
		(const uint8_t *)&codec_table, sizeof(codec_table),
		(uint8_t *)&response, sizeof(response));
	if (result) {
		LOGE("Error: ACDB CODEC CAL returned = %d\n", result);
		goto close_fd;
	}

	result = write(mad_fd, mad_buf, response.nBytesUsedInBuffer);
	if ( result != (int)response.nBytesUsedInBuffer) {
		LOGE("Error writing MAD calibration data returned = %x\n", result);
		goto close_fd;
	}

close_fd:
	fsync(mad_fd);
	close(mad_fd);
done:
	return result;
}

void acdb_loader_send_audio_cal(int acdb_id, int capability)
{
	int32_t		path;

	if (virt_cal_buffer == NULL) {
		LOGE("ACDB -> Not correctly initialized!\n");
		goto done;
	}

	path = get_audcal_path((uint32_t)capability);
	if (path == INVALID_DATA) {
		LOGE("ACDB -> Device is not RX or TX!"
			"acdb_id = %d\n", acdb_id);
		goto done;
	}

	LOGD("ACDB -> send_audio_cal, acdb_id = %d, path =  %d\n",
		acdb_id, path);

	send_adm_topology(acdb_id, path);
	send_asm_topology();
	send_audtable(acdb_id, path);
	send_audvoltable(acdb_id, path);
	send_afe_cal(acdb_id, path);
done:
	return;
}

void acdb_loader_send_listen_cal(int acdb_id, int app_id)
{
	send_afe_cal(acdb_id, TX_CAL);
	send_lsm_cal(acdb_id, app_id);
	send_codec_cal(acdb_id);
}

static int32_t get_anc_table(int acdb_id, struct cal_block *block, int timpani)
{
	int32_t result = -1;
	AcdbCodecANCSettingCmdType acdb_cmd;
	AcdbQueryResponseType acdb_cmd_response;
	uint32_t			*addr =
		(uint32_t *)(virt_cal_buffer + (ACDB_ANC_OFFSET)
		* ACDB_BLOCK_SIZE);

	uint32_t acdb_command_id = ACDB_CMD_GET_CODEC_ANC_SETTING;
	acdb_cmd.nRxDeviceId = acdb_id;
	acdb_cmd.nParamId = ACDB_PID_CODEC_ANC_DATA_WCD9320;
	acdb_cmd.nBufferPointer = (uint8_t *)addr;
	acdb_cmd.nBufferLength = ACDB_BLOCK_SIZE;

	LOGD("ACDB -> ACDB_CMD_GET_ANC_SETTING\n");

	result = acdb_ioctl(acdb_command_id,
		(const uint8_t *)&acdb_cmd, sizeof(acdb_cmd),
		(uint8_t *)&acdb_cmd_response, sizeof(acdb_cmd_response));
	if (result) {
		LOGE("Error: ACDB ANC returned = %d\n", result);
		goto done;
	}
	block->cal_size = acdb_cmd_response.nBytesUsedInBuffer;
	block->cal_offset = (ACDB_ANC_OFFSET) * ACDB_BLOCK_SIZE;
done:
	return result;
}

#define ABS(x) (((x) < 0) ? (-1*(x)) : (x))
int32_t FP_mult(int32_t val1, int32_t val2)
{
	int32_t prod = 0;
	if ((val1 > 0 && val2 > 0) || (val1 < 0 && val2 < 0)) {
		if (ABS(val1) > (int32_t) (MAX_INT/ABS(val2)))
			prod = MAX_INT;
	}
	else if ((val1 > 0 && val2 < 0) || (val1 < 0 && val2 > 0)) {
		if (ABS(val1) > (int32_t) (MAX_INT/ABS(val2)))
			prod = -(int32_t) MAX_INT;
	}
	if (0 == prod)
		prod = val1 * val2;

	return prod;
}
int32_t FP_shift(int32_t val, int32_t shift)
{
	int32_t rnd = 1 << (ABS(shift)-1);
	int32_t val_s = val;
	/* underflow -> rounding errors */
	if (shift < 0) {
		val_s = ABS(val_s) + rnd;
		val_s = val_s >> ABS(shift);
		val_s = (val > 0) ? val_s : -val_s;
	}
	/* overflow -> saturation */
	else if (shift > 0) {
		if (ABS(val) > (int32_t) ((MAX_INT >> ABS(shift)))) {
			if (val < 0)
				val_s = -(int32_t) MAX_INT;
			else
				val_s = (int32_t) MAX_INT;
		} else
			val_s = val << ABS(shift);
	}
	return val_s;
}

uint16_t twosComp(int16_t val, int16_t bits)
{
	uint16_t res = 0;
	uint32_t width = bits + 1;
	if (val >= 0)
		res = (uint16_t) val;
	else
		res = -((-val) - (1 << width));

	return res;
}

int32_t FP_format(int32_t val, int32_t intb, int32_t fracb, int32_t max_val)
{
	val = FP_shift(val, -(ANC_COEFF_FRAC_BITS - fracb));
	/* Check for saturation */
	if (val > max_val)
		val = max_val;
	else if (val < -max_val)
		val = -max_val;
	/* convert to 2s compl */
	val = twosComp((uint16_t) val, (uint16_t) (intb + fracb));
	return val;
}

void send_mbhc_data(void)
{
	int mbhc_fd, result;
	AcdbGblTblCmdType global_cmd;
	AcdbQueryResponseType   response;
	uint8_t *calbuf;

	LOGD("send mbhc data\n");

	mbhc_fd = creat(MBHC_BIN_PATH, PERM644);

	if (mbhc_fd < 0)
	{
		LOGE("Error opening MBHC file %d\n", errno);
		return;
	}

	calbuf = malloc(TEMP_CAL_BUFSZ);
	if (calbuf == NULL) {
		LOGE("Fail to allocate memory for button detection calibration\n");
		goto close_fd;
	}

	global_cmd.nModuleId = ACDB_MID_MBHC;
	global_cmd.nParamId = ACDB_PID_GENERAL_CONFIG;
	global_cmd.nBufferLength = TEMP_CAL_BUFSZ;
	global_cmd.nBufferPointer = calbuf;

	result = acdb_ioctl(ACDB_CMD_GET_GLBTBL_DATA,
		(const uint8_t *) &global_cmd, sizeof(global_cmd),
		(uint8_t *) &response, sizeof(response));

	if (result)
	{
		LOGE("Error reading MBHC general config returned = %x\n", result);
		goto acdb_error;
	}

	result = write(mbhc_fd, calbuf, response.nBytesUsedInBuffer);
	if ( result != (int)response.nBytesUsedInBuffer) {
		LOGE("Error writing MBHC calibration data returned = %x\n", result);
		goto acdb_error;
	}

	global_cmd.nParamId = ACDB_PID_PLUG_REMOVAL_DETECTION;

	result = acdb_ioctl(ACDB_CMD_GET_GLBTBL_DATA,
		(const uint8_t *) &global_cmd, sizeof(global_cmd),
		(uint8_t *) &response, sizeof(response));

	if (result)
	{
		LOGE("Error reading MBHC removal config returned = %x\n", result);
		goto acdb_error;
	}

	result = write(mbhc_fd, calbuf, response.nBytesUsedInBuffer);
	if ( result != (int)response.nBytesUsedInBuffer) {
		LOGE("Error writing MBHC calibration data returned = %x\n", result);
		goto acdb_error;
	}

	global_cmd.nParamId = ACDB_PID_PLUG_TYPE_DETECTION;

	result = acdb_ioctl(ACDB_CMD_GET_GLBTBL_DATA,
		(const uint8_t *) &global_cmd, sizeof(global_cmd),
		(uint8_t *) &response, sizeof(response));

	if (result)
	{
		LOGE("Error reading MBHC plug type config returned = %x\n", result);
		goto acdb_error;
	}

	result = write(mbhc_fd, calbuf, response.nBytesUsedInBuffer);
	if ( result != (int)response.nBytesUsedInBuffer) {
		LOGE("Error writing MBHC calibration data returned = %x\n", result);
		goto acdb_error;
	}
	global_cmd.nParamId = ACDB_PID_BUTTON_PRESS_DETECTION;

	result = acdb_ioctl(ACDB_CMD_GET_GLBTBL_DATA,
		(const uint8_t *) &global_cmd, sizeof(global_cmd),
		(uint8_t *) &response, sizeof(response));

	if (result)
	{
		LOGE("Error reading MBHC button press config returned = %x\n", result);
		goto acdb_error;
	}

	result = write(mbhc_fd, calbuf, response.nBytesUsedInBuffer);
	if ( result != (int)response.nBytesUsedInBuffer) {
		LOGE("Error writing MBHC calibration data returned = %x\n", result);
		goto acdb_error;
	}

	global_cmd.nParamId = ACDB_PID_IMPEDANCE_DETECTION;

	result = acdb_ioctl(ACDB_CMD_GET_GLBTBL_DATA,
		(const uint8_t *) &global_cmd, sizeof(global_cmd),
		(uint8_t *) &response, sizeof(response));

	if (result)
	{
		LOGE("Error reading MBHC impedance config returned = %x\n", result);
		goto acdb_error;
	}

	result = write(mbhc_fd, calbuf, response.nBytesUsedInBuffer);
	if ( result != (int)response.nBytesUsedInBuffer) {
		LOGE("Error writing MBHC calibration data returned = %x\n", result);
		goto acdb_error;
	}

	fsync(mbhc_fd);
	free(calbuf);
	close(mbhc_fd);
	return;

acdb_error:
	free(calbuf);
close_fd:
	close(mbhc_fd);
	unlink(MBHC_BIN_PATH);
}


void send_wcd9xxx_anc_data(void)
{
	uint32_t anc_configurations = 7;
	uint32_t anc_base_configuration = 26;
	uint32_t anc_reserved[3];
	int i;
	int result = creat (WCD9320_ANC_BIN_PATH, PERM644);
	if (result < 0)
	{
		LOGE("Error opening anc file %d\n", errno);
		return;
	}

	write (result, anc_reserved, sizeof(uint32_t) * 3);
	write (result, &anc_configurations, sizeof(uint32_t));

	for (i = 0; i < (int32_t)(anc_configurations - 1); i++) {
		acdb_loader_send_wcd9xxx_anc_cal(i + anc_base_configuration, result);
	}
	/* Add the AANC Rx device */
	acdb_loader_send_wcd9xxx_anc_cal(DEVICE_HANDSET_RX_AANC_ACDB_ID, result);
	close(result);
}

int Setwcd9xxxANC_IIRCoeffs(uint32_t *anc_config, uint32_t *anc_index,
	struct adie_codec_taiko_db_anc_cfg *pANCCfg, uint32_t ancCh)
{
	int res = 0;
	int32_t coeff = 0;
	uint32_t u_coeff = 0;
	uint8_t valMSBs = 0;
	uint8_t valLSBs = 0;
	uint32_t iter = 0;
	uint32_t offset = ancCh * 128;
	uint8_t iir_index=0;
	double cal_gain = 0;
	int32_t temp_int = 0;

	/* Divide by 2^13 */
	cal_gain = ((double)pANCCfg[ancCh].anc_gain)/8192;
	/* Write FF coeffs */
	for (iter = 0; iter < TAIKO_ANC_NUM_IIR_FF_A_COEFFS + TAIKO_ANC_NUM_IIR_FF_B_COEFFS; iter++) {
		coeff = pANCCfg[ancCh].anc_ff_coeff[iter];
		if (iter < TAIKO_ANC_NUM_IIR_FF_A_COEFFS) {
			temp_int = (int32_t)round((double)(coeff)/16);
			u_coeff = (uint32_t)temp_int;
		}
		else {
			temp_int = (int32_t)round((((double)coeff * cal_gain) / 16));
			u_coeff = (uint32_t)temp_int;
		}
		valMSBs = (uint8_t) (0x0F & (u_coeff >> REGISTER_DEPTH));
		valLSBs = (uint8_t) (0xFF & u_coeff);
		anc_config[(*anc_index)++] = TAIKO_CODEC_PACK_ENTRY((TAIKO_A_CDC_ANC1_IIR_B2_CTL + offset), 0xFF, iir_index++);
                anc_config[(*anc_index)++] = TAIKO_CODEC_PACK_ENTRY((TAIKO_A_CDC_ANC1_IIR_B3_CTL + offset), 0xFF, valLSBs);
		anc_config[(*anc_index)++] = TAIKO_CODEC_PACK_ENTRY((TAIKO_A_CDC_ANC1_IIR_B2_CTL + offset), 0xFF, iir_index++);
		anc_config[(*anc_index)++] = TAIKO_CODEC_PACK_ENTRY((TAIKO_A_CDC_ANC1_IIR_B3_CTL + offset), 0xFF, valMSBs);
	}
	/* Write FB coeff */
	for (iter = 0; iter < TAIKO_ANC_NUM_IIR_FB_A_COEFFS + TAIKO_ANC_NUM_IIR_FB_B_COEFFS; iter++) {
		coeff = pANCCfg[ancCh].anc_fb_coeff[iter];
		temp_int = (int32_t)round((double)(coeff)/16);
		u_coeff = (uint32_t)temp_int;
		valMSBs = (uint8_t) (0x0F & (u_coeff >> REGISTER_DEPTH));
		valLSBs = (uint8_t) (0xFF & u_coeff);
		anc_config[(*anc_index)++] = TAIKO_CODEC_PACK_ENTRY((TAIKO_A_CDC_ANC1_IIR_B2_CTL + offset), 0xFF, iir_index++);
		anc_config[(*anc_index)++] = TAIKO_CODEC_PACK_ENTRY((TAIKO_A_CDC_ANC1_IIR_B3_CTL + offset), 0xFF, valLSBs);
		anc_config[(*anc_index)++] = TAIKO_CODEC_PACK_ENTRY((TAIKO_A_CDC_ANC1_IIR_B2_CTL + offset), 0xFF, iir_index++);
		anc_config[(*anc_index)++] = TAIKO_CODEC_PACK_ENTRY((TAIKO_A_CDC_ANC1_IIR_B3_CTL + offset), 0xFF, valMSBs);
	}
	return res;
}


int Setwcd9xxxANC_LPFShift(uint32_t *anc_config, uint32_t *anc_index,
	struct adie_codec_taiko_db_anc_cfg *pANCCfg, uint32_t ancCh)
{
	int res = 0;
	int32_t coeff = 0;
	uint32_t u_coeff = 0;
	uint8_t valMSBs = 0;
	uint8_t valLSBs = 0;
	uint32_t iter = 0;
	uint32_t offset = ancCh * 128;
	uint8_t value = 0;

	/* FF */
	value |= pANCCfg[ancCh].anc_ff_lpf_shift[0];
	value |= pANCCfg[ancCh].anc_ff_lpf_shift[1] << 4;
	anc_config[(*anc_index)++] =  TAIKO_CODEC_PACK_ENTRY((TAIKO_A_CDC_ANC1_LPF_B1_CTL + offset), 0xFF, value);

	/* FB */
	value = 0;
	value |= pANCCfg[ancCh].anc_fb_lpf_shift[0];
        value |= pANCCfg[ancCh].anc_fb_lpf_shift[1] << 4;
	anc_config[(*anc_index)++] =  TAIKO_CODEC_PACK_ENTRY((TAIKO_A_CDC_ANC1_LPF_B2_CTL + offset), 0xFF, value);

	return res;
}

int convert_anc_data_to_wcd9xxx(struct adie_codec_taiko_db_anc_cfg *pANCCfg, int file_descriptor)
{
	uint32_t index;
	uint32_t reg, mask, val;
	uint32_t temp_ctl_reg_val;
	uint32_t anc_index = 0;
	int j;
	uint32_t offset;
	uint32_t ancCh;
	bool ancDMICselect;
	struct storage_adie_codec_anc_data anc_config;

	for(ancCh = 0; ancCh < NUM_ANC_COMPONENTS; ancCh++) {
		if (!pANCCfg[ancCh].input_device) {
			continue;
		}
		offset = ancCh * 128;

		anc_config.writes[anc_index++] = TAIKO_CODEC_PACK_ENTRY(TAIKO_A_CDC_CLK_ANC_RESET_CTL, ancCh ? 0xC : 0x3, ancCh ? 0xC : 0x3);
		temp_ctl_reg_val = 0;
		if (pANCCfg[ancCh].ff_out_enable)
			temp_ctl_reg_val |= 0x1;
		if ((pANCCfg[ancCh].input_device & 0xF) >= ADIE_CODEC_DMIC1)
			temp_ctl_reg_val |= 0x2;
		if (pANCCfg[ancCh].anc_lr_mix_enable)
			temp_ctl_reg_val |= 0x4;
		if (pANCCfg[ancCh].hybrid_enable)
			temp_ctl_reg_val |= 0x8;
		if (pANCCfg[ancCh].ff_in_enable)
			temp_ctl_reg_val |= 0x10;
		if (pANCCfg[ancCh].dcflt_enable)
			temp_ctl_reg_val |= 0x20;
		if (pANCCfg[ancCh].smlpf_enable)
			temp_ctl_reg_val |= 0x40;
		if (pANCCfg[ancCh].adaptive_gain_enable)
			temp_ctl_reg_val |= 0x80;

		anc_config.writes[anc_index++] = TAIKO_CODEC_PACK_ENTRY((TAIKO_A_CDC_ANC1_B1_CTL + offset), 0xFF, temp_ctl_reg_val);
		anc_config.writes[anc_index++] = TAIKO_CODEC_PACK_ENTRY((TAIKO_A_CDC_ANC1_SHIFT + offset), 0xFF, ((pANCCfg[ancCh].anc_ff_shift << 4) | pANCCfg[ancCh].anc_fb_shift));
		/* IIR COEFFS */
		Setwcd9xxxANC_IIRCoeffs((uint32_t *)anc_config.writes, &anc_index, pANCCfg, ancCh);

		anc_config.writes[anc_index++] = TAIKO_CODEC_PACK_ENTRY((TAIKO_A_CDC_ANC1_IIR_B1_CTL + offset), 0x08, pANCCfg[ancCh].adaptive ? 0x08 : 0);

		/* LPF COEFFS */
		Setwcd9xxxANC_LPFShift((uint32_t *)anc_config.writes, &anc_index, pANCCfg, ancCh);

		/* ANC SMLPF CTL */
		anc_config.writes[anc_index++] = TAIKO_CODEC_PACK_ENTRY((TAIKO_A_CDC_ANC1_SMLPF_CTL + offset), 0xFF, pANCCfg[ancCh].smlpf_shift);
		/* ANC DCFLT CTL */
		anc_config.writes[anc_index++] = TAIKO_CODEC_PACK_ENTRY((TAIKO_A_CDC_ANC1_DCFLT_CTL + offset), 0xFF, pANCCfg[ancCh].dcflt_shift);
		/* ANC Adaptive gain */
		anc_config.writes[anc_index++] = TAIKO_CODEC_PACK_ENTRY((TAIKO_A_CDC_ANC1_GAIN_CTL + offset), 0xFF, pANCCfg[ancCh].adaptive_gain);
		anc_config.writes[anc_index++] = TAIKO_CODEC_PACK_ENTRY(TAIKO_A_CDC_CLK_ANC_CLK_EN_CTL, ancCh ? 0xC : 0x3, (1 | (1 << pANCCfg[ancCh].anc_feedback_enable)) << (ancCh*2));
		anc_config.writes[anc_index++] = TAIKO_CODEC_PACK_ENTRY(TAIKO_A_CDC_CLK_ANC_RESET_CTL, ancCh ? 0xC : 0x3, ~((1 | (1 << pANCCfg[ancCh].anc_feedback_enable)) << (ancCh*2)));
	}
	anc_config.size = anc_index;

	write (file_descriptor, &anc_config.size, sizeof(anc_config.size));
	write (file_descriptor, anc_config.writes, anc_config.size * TAIKO_PACKED_REG_SIZE);

	return anc_index;
}

int acdb_loader_send_wcd9xxx_anc_cal(int acdb_id, int file_descriptor)
{
	int32_t		  result;
	int			  index;
	uint32_t			anc_cal[3];
	struct cal_block		*block;
	struct adie_codec_taiko_db_anc_cfg *ancCfg;
	uint32_t			*addr;

	block = (struct cal_block *)&anc_cal[1];

	if (!block) {
		LOGE("Error retrieving calibration block\n");
		return -EPERM;
	}
	result = get_anc_table(acdb_id, block, 0);
	if (result) {
		return result;
	}
	addr = (uint32_t *)(virt_cal_buffer + block->cal_offset);
	ancCfg = (struct adie_codec_taiko_db_anc_cfg *)(addr);

	convert_anc_data_to_wcd9xxx(ancCfg, file_descriptor);

	return 0;
}

static uint32_t get_voice_topology(int acdb_id)
{
	int32_t				result = 0;
	AcdbGetVocProcTopIdCmdType	acdb_get_top;
	AcdbGetTopologyIdRspType	audio_top;

	acdb_get_top.nDeviceId = acdb_id;

	LOGD("ACDB -> ACDB_CMD_GET_VOCPROC_COMMON_TOPOLOGY_ID\n");

	result = acdb_ioctl(ACDB_CMD_GET_VOCPROC_COMMON_TOPOLOGY_ID,
		(const uint8_t *)&acdb_get_top, sizeof(acdb_get_top),
		(uint8_t *)&audio_top, sizeof(audio_top));
	if (result) {
		LOGE("Error: ACDB get voice rx topology for acdb id = %d, returned = %d\n",
		     acdb_id, result);
		goto err;
	}
	return audio_top.nTopologyId;
err:
	return result;
}

static int get_sidetone(int rxacdb_id, int txacdb_id,
			struct sidetone_cal *cal_data)
{
	int32_t				result = 0;
	AcdbAfeDataCmdType		sidetone;
	AcdbQueryResponseType		response;

	sidetone.nTxDeviceId = txacdb_id;
	sidetone.nRxDeviceId = rxacdb_id;
	sidetone.nModuleId = ACDB_MID_SIDETONE;
	sidetone.nParamId = ACDB_PID_SIDETONE;
	sidetone.nBufferLength = sizeof(*cal_data);
	sidetone.nBufferPointer = (uint8_t *)cal_data;

	LOGD("ACDB -> ACDB_CMD_GET_AFE_DATA\n");

	result = acdb_ioctl(ACDB_CMD_GET_AFE_DATA,
		(const uint8_t *)&sidetone, sizeof(sidetone),
		(uint8_t *)&response, sizeof(response));
	if (result) {
		LOGE("Error: ACDB AFE DATA Returned = %d\n", result);
		goto done;
	}
done:
	return result;
}

static int get_voice_columns(uint32_t table_id)
{
	int32_t				result = 0;
	AcdbVocColumnsInfoCmdType	voc_col;
	AcdbQueryResponseType		response;

	voc_col.nTableId = table_id;
	voc_col.nBufferLength = MAX_COL_SIZE;
	voc_col.pBuff = (uint8_t *)&(voice_col_data[1]);

	LOGD("ACDB -> ACDB_CMD_GET_VOC_COLUMNS_INFO\n");

	result = acdb_ioctl(ACDB_CMD_GET_VOC_COLUMNS_INFO,
		(const uint8_t *)&voc_col, sizeof(voc_col),
		(uint8_t *)&response, sizeof(response));
	if (result) {
		LOGE("Error: ACDB VOC COL Returned = %d\n", result);
		goto done;
	}
	voice_col_data[0] = response.nBytesUsedInBuffer;
done:
	return result;
}
static int send_voice_columns(uint32_t table_id)
{
	int32_t		result = 0;

	LOGD("ACDB -> send_voice_columns, table %d\n", table_id);
	get_voice_columns(table_id);

	/* subtract 1 to map acdb.h defines to indexes */
	result = ioctl(acdb_handle, voc_col_ioctl[(table_id-1)],
		(unsigned int)&voice_col_data);
	if (result) {
		LOGE("Error: Sending ACDB voice columns result = %d\n", result);
		goto done;
	}
done:
	return result;
}

static int get_vocproc_dev_cfg(int rxacdb_id, int txacdb_id,
				struct cal_block *block)
{
	int32_t				result = 0;
	AcdbVocProcDevCfgCmdType	vocdevtable;
	AcdbQueryResponseType		response;
	/* Point address to designated PMEM block */
	uint32_t			*addr =
		(uint32_t *)(virt_cal_buffer + VOCPROC_W_DEV_SIZE);

	vocdevtable.nTxDeviceId = txacdb_id;
	vocdevtable.nRxDeviceId = rxacdb_id;
	vocdevtable.nBufferLength = VOCPROC_DEV_CFG_SIZE;
	vocdevtable.pBuff = (uint8_t *)addr;

	LOGD("ACDB -> ACDB_CMD_GET_VOC_PROC_DEVICE_CFG\n");

	result = acdb_ioctl(ACDB_CMD_GET_VOC_PROC_DEVICE_CFG,
		(const uint8_t *)&vocdevtable, sizeof(vocdevtable),
		(uint8_t *)&response, sizeof(response));
	if (result) {
		LOGE("Error: ACDB VocProc Dev Cfg Returned = %d\n",
			result);
		block->cal_size = 0;
		goto done;
	} else if (response.nBytesUsedInBuffer > VOCPROC_DEV_CFG_SIZE) {
		LOGE("Error: ACDB VocProc Dev Cfg used = %u bytes, max of %u\n",
			response.nBytesUsedInBuffer, VOCPROC_DEV_CFG_SIZE);
		block->cal_size = 0;
		goto done;
	}

	block->cal_size = response.nBytesUsedInBuffer;
	block->cal_offset = VOCPROC_W_DEV_SIZE;
done:
	return result;
}
static int send_vocproc_dev_cfg(int rxacdb_id, int txacdb_id)
{
	int result = 0;
	uint32_t vocproc_dev_cal[3];

	get_vocproc_dev_cfg(rxacdb_id, txacdb_id,
			(struct cal_block *)&vocproc_dev_cal[1]);

	vocproc_dev_cal[0] = sizeof(struct cal_block);

	LOGD("ACDB -> AUDIO_SET_VOCPROC_DEV_CFG_CAL\n");
	result = ioctl(acdb_handle, AUDIO_SET_VOCPROC_DEV_CFG_CAL, (unsigned int)&vocproc_dev_cal);
	if (result) {
		LOGE("Error: Sending ACDB VocProc Dev Cfg data result = %d\n", result);
		goto done;
	}
done:
	return result;
}


static int get_voctable(int rxacdb_id, int txacdb_id,
				struct cal_block *block)
{
	int32_t				result = 0;
	AcdbVocProcCmnTblCmdType	voctable;
	AcdbQueryResponseType		response;
	/* Point address to designated PMEM block */
	uint32_t			*addr =
		(uint32_t *)virt_cal_buffer;

	voctable.nTxDeviceId = txacdb_id;
	voctable.nRxDeviceId = rxacdb_id;
	voctable.nTxDeviceSampleRateId = get_samplerate(txacdb_id);
	voctable.nRxDeviceSampleRateId = get_samplerate(rxacdb_id);
	voctable.nBufferLength = VOCPROC_W_DEV_SIZE;
	voctable.nBufferPointer = (uint8_t *)addr;

	LOGD("ACDB -> ACDB_CMD_GET_VOC_PROC_COMMON_TABLE\n");

	result = acdb_ioctl(ACDB_CMD_GET_VOC_PROC_COMMON_TABLE,
		(const uint8_t *)&voctable, sizeof(voctable),
		(uint8_t *)&response, sizeof(response));
	if (result) {
		LOGE("Error: ACDB VocProc Returned = %d\n",
			result);
		block->cal_size = 0;
		goto done;
	} else if (response.nBytesUsedInBuffer > VOCPROC_W_DEV_SIZE) {
		LOGE("Error: ACDB VocProc used = %u bytes, max of %u\n",
			response.nBytesUsedInBuffer, VOCPROC_W_DEV_SIZE);
		block->cal_size = 0;
		goto done;
	}

	block->cal_size = response.nBytesUsedInBuffer;
	block->cal_offset = 0;
done:
	return result;
}

static int get_vocstrmtable(struct cal_block *block)
{
	int32_t				result = 0;
	AcdbQueryCmdType		vocstrmtable;
	AcdbQueryResponseType		response;
	/* Point address to designated PMEM block */
	uint32_t			*addr =
		(uint32_t *)(virt_cal_buffer + ACDB_VOCSTRM_SIZE_OFFSET);

	vocstrmtable.nBufferLength = VOCSTRM_SIZE;
	vocstrmtable.pBufferPointer = (uint8_t *) addr;

	LOGD("ACDB -> ACDB_CMD_GET_VOC_STREAM_COMMON_TABLE\n");

	result = acdb_ioctl(ACDB_CMD_GET_VOC_STREAM_COMMON_TABLE,
		(const uint8_t *)&vocstrmtable,	sizeof(vocstrmtable),
		(uint8_t *)&response, sizeof(response));
	if (result) {
		LOGE("Error: ACDB VocProc Stream Returned = %d\n",
			result);
		block->cal_size = 0;
		goto done;
	} else if (response.nBytesUsedInBuffer > VOCSTRM_SIZE) {
		LOGE("Error: ACDB VocProc Stream used = %u bytes, max of %u\n",
			response.nBytesUsedInBuffer, VOCSTRM_SIZE);
		block->cal_size = 0;
		goto done;
	}

	block->cal_size = response.nBytesUsedInBuffer;
	block->cal_offset = ACDB_VOCSTRM_SIZE_OFFSET;
done:
	return result;
}

static int get_vocvoltable(int rxacdb_id, int txacdb_id, int feature_set,
				struct cal_block *block)
{
	AcdbVocProcGainDepVolTblV2CmdType	vocvoltable;
	int32_t					result = 0;
	AcdbQueryResponseType			response;
	/* Point address to designated PMEM block */
	uint32_t			*addr =
		(uint32_t *)(virt_cal_buffer + ACDB_VOCVOL_SIZE_OFFSET);

	vocvoltable.nTxDeviceId = txacdb_id;
	vocvoltable.nRxDeviceId = rxacdb_id;
	vocvoltable.nFeatureId = feature_set;
	vocvoltable.nBufferLength = VOCVOL_SIZE;
	vocvoltable.nBufferPointer = (uint8_t *) addr;

	LOGD("ACDB -> ACDB_CMD_GET_VOC_PROC_GAIN_DEP_VOLTBL\n");

	result = acdb_ioctl(ACDB_CMD_GET_VOC_PROC_GAIN_DEP_VOLTBL_V2,
		(const uint8_t *)&vocvoltable,	sizeof(vocvoltable),
		(uint8_t *)&response, sizeof(response));
	if (result) {
		LOGE("Error: ACDB VocProc Vol Returned = %d\n",
			result);
		block->cal_size = 0;
		goto done;
	}
	if (response.nBytesUsedInBuffer > VOCVOL_SIZE) {
		LOGE("Error: ACDB VocProc vol used = %u bytes, max of %u\n",
			response.nBytesUsedInBuffer, VOCVOL_SIZE);
		block->cal_size = 0;
		goto done;
	}

	block->cal_size = response.nBytesUsedInBuffer;
	block->cal_offset = ACDB_VOCVOL_SIZE_OFFSET;
done:
	return result;
}

static int send_voice_rx_topology(int acdb_id)
{
	int32_t		result = 0;
	uint32_t	topology = 0;

	LOGD("ACDB -> send_voice_rx_topology\n");

	topology = get_voice_topology(acdb_id);

	if (!topology)
		goto done;

	result = ioctl(acdb_handle, AUDIO_SET_VOICE_RX_TOPOLOGY,
		(unsigned int)&topology);
	if (result) {
		LOGE("Error: Sending ACDB voice rx topology result = %d\n", result);
		goto done;
	}
done:
	return result;
}

static int send_voice_tx_topology(int acdb_id)
{
	int32_t		result = 0;
	uint32_t	topology = 0;

	LOGD("ACDB -> send_voice_tx_topology\n");

	topology = get_voice_topology(acdb_id);

	if (!topology)
		goto done;

	result = ioctl(acdb_handle, AUDIO_SET_VOICE_TX_TOPOLOGY,
		(unsigned int)&topology);
	if (result) {
		LOGE("Error: Sending ACDB voice tx topology result = %d\n", result);
		goto done;
	}
done:
	return result;
}

static int send_sidetone(int rxacdb_id, int txacdb_id)
{
	int		result = 0;
	/* Size to one sidetone_cal block (32B) */
	/* and store size as first element */
	uint32_t	sidetone_cal[2] = {0,0};

	get_sidetone(rxacdb_id, txacdb_id,
		(struct sidetone_cal *)&sidetone_cal[1]);

	/* Sidetone_cal data size */
	sidetone_cal[0] = sizeof(struct sidetone_cal);

	LOGD("ACDB -> AUDIO_SET_SIDETONE_CAL\n");
	result = ioctl(acdb_handle, AUDIO_SET_SIDETONE_CAL,
			(unsigned int)&sidetone_cal);
	if (result) {
		LOGE("Error: Sending ACDB sidetone data result = %d\n", result);
		goto done;
	}
done:
	return result;
}

static int send_voctable(int rxacdb_id, int txacdb_id)
{
	int result = 0;
	/* Size buffer to store one cal block (64B) */
	/* and its size as the first element */
	uint32_t vocproc_cal[3] = {0,0,0};

	get_voctable(rxacdb_id, txacdb_id,
			(struct cal_block *)&vocproc_cal[1]);

	vocproc_cal[0] = sizeof(struct cal_block);

	LOGD("ACDB -> AUDIO_SET_VOCPROC_CAL\n");
	result = ioctl(acdb_handle, AUDIO_SET_VOCPROC_CAL, (unsigned int)&vocproc_cal);
	if (result) {
		LOGE("Error: Sending ACDB VocProc data result = %d\n", result);
		goto done;
	}
done:
	return result;
}

static int send_vocstrmtable(void) {
	int result = 0;
	/* Size buffer to store one cal block (64B) */
	/* and its size as the first element */
	uint32_t vocstrm_cal[3] = {0,0,0};

	get_vocstrmtable((struct cal_block *)&vocstrm_cal[1]);

	vocstrm_cal[0] = sizeof(struct cal_block);

	LOGD("ACDB -> AUDIO_SET_VOCPROC_STREAM_CAL\n");
	result = ioctl(acdb_handle, AUDIO_SET_VOCPROC_STREAM_CAL,
		(unsigned int)&vocstrm_cal);
	if (result < 0) {
		LOGE("Error: Sending ACDB VOCPROC STREAM fail result %d\n",
			result);
		goto done;
	}
done:
	return result;
}


static int send_vocvoltable(int rxacdb_id, int txacdb_id, int feature_set)
{
	int result = 0;
	/* Size buffer to store one cal block (64B) */
	/* and its size as the first element */
	uint32_t vocvol_cal[3] = {0,0,0};

	result = get_vocvoltable(rxacdb_id, txacdb_id, feature_set,
			(struct cal_block *)&vocvol_cal[1]);

	if (result < 0) {
		LOGE("Error: getting VocVol Table = %d\n", result);
		goto done;
	}

	vocvol_cal[0] = sizeof(struct cal_block);

	LOGD("ACDB -> AUDIO_SET_VOCPROC_VOL_CAL\n");
	result = ioctl(acdb_handle, AUDIO_SET_VOCPROC_VOL_CAL,
			(unsigned int)&vocvol_cal);
	if (result) {
		LOGE("Error: Sending ACDB VocProc data result = %d\n", result);
		goto done;
	}
done:
	return result;
}

static int validate_voc_cal_dev_pair(int rxacdb_id, int txacdb_id)
{
	int result = 0;
	AcdbDevicePairType dev_pair;
	AcdbDevicePairingResponseType response;

	dev_pair.nTxDeviceId = txacdb_id;
	dev_pair.nRxDeviceId = rxacdb_id;

        result = acdb_ioctl(ACDB_CMD_IS_DEVICE_PAIRED,
			(const uint8_t *)&dev_pair, sizeof(dev_pair),
			(uint8_t *)&response, sizeof(response));

	if (result < 0) {
		LOGE("Error: failure to vaildate the device pair = %d\n",
			result);
		goto done;
	}

	result = (int)response.ulIsDevicePairValid;
done:
	return result;
}

static void send_voice_cal(int rxacdb_id, int txacdb_id, int feature_set)
{
	LOGD("ACDB -> send_voice_cal, acdb_rx = %d, acdb_tx = %d, feature_set = %d\n",
		rxacdb_id, txacdb_id, feature_set);

	/* check if it is valid RX/TX device pair */
	if (validate_voc_cal_dev_pair(rxacdb_id, txacdb_id) != 1) {
		LOGE("Error invalid device pair");
		goto done;
	}

	if (virt_cal_buffer == NULL) {
		LOGE("ACDB -> Not correctly initialized!\n");
		goto done;
	}

	current_voice_tx_acdb_id = txacdb_id;
	current_voice_rx_acdb_id = rxacdb_id;

	send_voice_rx_topology(rxacdb_id);
	send_voice_tx_topology(txacdb_id);
	send_sidetone(rxacdb_id, txacdb_id);
	send_voice_columns(ACDB_VOC_PROC_TABLE);
	send_voctable(rxacdb_id, txacdb_id);
	send_vocproc_dev_cfg(rxacdb_id, txacdb_id);
	send_voice_columns(ACDB_VOC_PROC_VOL_TABLE);
	if((send_vocvoltable(rxacdb_id, txacdb_id, feature_set) < 0) &&
		(feature_set != ACDB_VOCVOL_FID_DEFAULT)) {
		LOGD("ACDB -> feature set %d failed, using default feature set\n", feature_set);
		if (send_vocvoltable(rxacdb_id, txacdb_id, ACDB_VOCVOL_FID_DEFAULT) < 0)
				LOGE("ACDB -> Resend default vocvol unsuccessful!\n");
	}
	send_voice_columns(ACDB_VOC_STREAM_TABLE);
	send_vocstrmtable();

	/* Send AFE cal for both devices */
	/* if AFE cal supported */
	if (NUM_AFE_BLOCKS) {
		send_afe_cal(txacdb_id, TX_CAL);
		send_afe_cal(rxacdb_id, RX_CAL);
	}
	if (valid_aanc_device(txacdb_id))
		send_aanctable(txacdb_id);

	LOGD("ACDB -> Sent VocProc Cal!\n");
done:
	return;
}

void acdb_loader_send_voice_cal_v2(int rxacdb_id, int txacdb_id, int feature_set)
{
	pthread_mutex_lock(&loader_mutex);
	send_voice_cal(rxacdb_id, txacdb_id, feature_set);
	pthread_mutex_unlock(&loader_mutex);
}

void acdb_loader_send_voice_cal(int rxacdb_id, int txacdb_id)
{
	pthread_mutex_lock(&loader_mutex);
	send_voice_cal(rxacdb_id, txacdb_id, current_feature_set);
	pthread_mutex_unlock(&loader_mutex);
}

int get_vocvoltable_size(int rxacdb_id, int txacdb_id, int feature_set)
{
	int32_t					result = 0;
	AcdbVocProcGainDepVolTblSizeV2CmdType	vocvoltablesize;
	AcdbSizeResponseType			response;

	vocvoltablesize.nTxDeviceId = txacdb_id;
	vocvoltablesize.nRxDeviceId = rxacdb_id;
	vocvoltablesize.nFeatureId = feature_set;

	LOGD("ACDB -> ACDB_CMD_GET_VOC_PROC_GAIN_DEP_VOLTBL_SIZE_V2\n");

	result = acdb_ioctl(ACDB_CMD_GET_VOC_PROC_GAIN_DEP_VOLTBL_SIZE_V2,
		(const uint8_t *)&vocvoltablesize, sizeof(vocvoltablesize),
		(uint8_t *)&response, sizeof(response));
	if (result < 0) {
		LOGE("Error: ACDB VocProc Vol Size Returned = %d\n",
			result);
		goto done;
	}

	result = response.nSize;
	if (response.nSize == 0) {
		LOGE("Error: ACDB VocProc vol size returned %d bytes\n",
			response.nSize);
		goto done;
	}

done:
	return result;
}

int deregister_vocvoltable(void)
{
	int result = 0;

	LOGD("ACDB -> AUDIO_DEREGISTER_VOCPROC_VOL_TABLE\n");
	result = ioctl(acdb_handle, AUDIO_DEREGISTER_VOCPROC_VOL_TABLE, 0);
	if (result < 0) {
		LOGE("Error: Deregister vocproc vol returned = %d\n",
			result);
		goto done;
	}
done:
	return result;
}

int register_vocvoltable(void)
{
	int result = 0;

	LOGD("ACDB -> AUDIO_REGISTER_VOCPROC_VOL_TABLE\n");
	result = ioctl(acdb_handle, AUDIO_REGISTER_VOCPROC_VOL_TABLE, 0);
	if (result < 0) {
		LOGE("Error: Register vocproc vol returned = %d\n",
			result);
		goto done;
	}
done:
	return result;
}

int acdb_loader_reload_vocvoltable(int feature_set)
{
	int result = 0;
	uint32_t txacdb_id;
	uint32_t rxacdb_id;

	pthread_mutex_lock(&loader_mutex);
	txacdb_id = current_voice_tx_acdb_id;
	rxacdb_id = current_voice_rx_acdb_id;
	current_feature_set = feature_set;

	LOGD("ACDB -> acdb_loader_reload_vocvoltable, acdb_rx = %d, acdb_tx = %d, feature_set = %d\n",
		rxacdb_id, txacdb_id, feature_set);

	result = get_vocvoltable_size(rxacdb_id, txacdb_id, feature_set);
	if (result < 0) {
		LOGE("ACDB -> No vocvol table to reload!\n");
		goto done;
	}

	result = deregister_vocvoltable();
	if (result < 0) {
		LOGE("ACDB -> Deregister vocvol table unsuccessful!\n");
		goto done;
	}

	result = send_vocvoltable(rxacdb_id, txacdb_id, feature_set);
	if (result < 0) {
		LOGE("ACDB -> Deregister vocvol table unsuccessful!\n");

		if (feature_set != ACDB_VOCVOL_FID_DEFAULT) {

			LOGE("ACDB -> Resend default vocvol table!\n");
			if (send_vocvoltable(rxacdb_id, txacdb_id, ACDB_VOCVOL_FID_DEFAULT) < 0)
				LOGE("ACDB -> Resend default vocvol unsuccessful!\n");
		}

		/* Even if second attempt to send vol table fails */
		/* Try to re-register. Memory should still contain */
		/* a previous valid volume table */
		LOGE("ACDB -> Reregister default vocvol table!\n");
		if(register_vocvoltable() < 0) {
			LOGE("ACDB -> Reregister default volume unsuccessful!\n");
			goto done;
		}
		LOGE("ACDB -> Resend default volume successful!\n");
		goto done;
	}

	result = register_vocvoltable();
	if (result < 0) {
		LOGE("ACDB -> Register vocvol table unsuccessful!\n");
		goto done;
	}

done:
	pthread_mutex_unlock(&loader_mutex);
	return result;
}

void acdb_loader_deallocate_ACDB(void)
{
	int	result;

#ifdef _ANDROID_
	LOGD("ACDB -> deallocate_ADIE\n");
	adie_rtac_exit();
#endif
	LOGD("ACDB -> deallocate_ACDB\n");
	acdb_rtac_exit();
	acph_deinit();
	munmap(virt_cal_buffer, ACDB_BUFFER_SIZE);
	close(map_handle);
	result = ioctl(ion_handle, ION_IOC_FREE, &alloc_handle);
	if (result)
		LOGE("ION_IOC_FREE errno: %d\n", result);
	close(ion_handle);
	close(acdb_handle);
	free(mad_buf);
	LOGD("ACDB -> deallocate_ACDB done!\n");
}

int acdb_loader_get_remote_acdb_id(unsigned int native_acdb_id)
{
	int				result;
	AcdbGetRmtCompDevIdCmdType	cmd;
	AcdbGetRmtCompDevIdRspType	response;

	LOGD("ACDB -> acdb_loader_get_remote_acdb_id, acdb_id = %d\n",
		native_acdb_id);

	if (virt_cal_buffer == NULL) {
		LOGE("ACDB -> Not correctly initialized!\n");
		result = INVALID_DATA;
		goto done;
	}

	cmd.nNativeDeviceId = native_acdb_id;

	result = acdb_ioctl(ACDB_CMD_GET_COMPATIBLE_REMOTE_DEVICE_ID,
			(const uint8_t *)&cmd, sizeof(cmd),
			(uint8_t *)&response, sizeof(response));
	if (result < 0) {
		LOGE("Error: Remote ACDB ID lookup failed = %d\n",
			result);
		goto done;
	}

	result = response.nRmtDeviceId;
done:
	return result;
}

