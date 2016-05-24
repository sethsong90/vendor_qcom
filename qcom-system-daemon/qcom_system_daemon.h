/*
 *Copyright (C) 2013-2014 Qualcomm Technologies, Inc. All rights reserved.
 *        Qualcomm Technologies Proprietary and Confidential.
 *
 *qcom_system_daemon.h : Header file for qcom-system-daemon
 */
#include "msg.h"
#include "diag_lsm.h"
#include "diagpkt.h"
#include "diagcmd.h"
#include "subsystem_control.h"
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "QCOMSysDaemon"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <cutils/android_reboot.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/reboot.h>

/* FTM MODE ID 75 11 53*/
#define FTM_FFBM_CMD_CODE	53
#define EDL_RESET_CMD_CODE	1

#define MISC_PARTITION_LOCATION "/dev/block/platform/msm_sdcc.1/by-name/misc"
#define BLOCK_DEVICE_NODE "/dev/block/platform/msm_sdcc.1/by-name/bootselect"
#define MODE_FFBM "ffbm-01"
#define MODE_NORMAL "normal"

#define RET_SUCCESS 0
#define RET_FAILED 1

#define REBOOT_CMD "reboot"
#define EDL_REBOOT_CMD "edl-reboot"
#define SWITCH_OS_CMD "switchos"
#define FFBM_COMMAND_BUFFER_SIZE 20

/* Subsystem command codes for image version */

#define VERSION_DIAGPKT_PROCID            0x80              // VERSION_PROCID 128
#define VERSION_DIAGPKT_SUBSYS            0x63              // VERSION_SUBSYS 99
#define VERSION_DIAGPKT_PREFIX            0x00              // VERSION_PREFIX 0

#define SELECT_IMAGE_FILE		"/sys/devices/soc0/select_image"
#define IMAGE_VERSION_FILE		"/sys/devices/soc0/image_version"
#define IMAGE_VARIANT_FILE		"/sys/devices/soc0/image_variant"
#define IMAGE_CRM_VERSION_FILE		"/sys/devices/soc0/image_crm_version"
#define SOS_FIFO			"/data/app/qsysdaemon"
#define BOOTSELECT_FACTORY		(1 << 30)
#define BOOTSELECT_SIGNATURE		('B' | ('S' << 8) | ('e' << 16) | ('l' << 24))
#define BOOTSELECT_VERSION		0x00010001

/* Size of version table stored in smem */
#define VERSION_TABLE_S 4096
#define IMAGE_VERSION_SINGLE_BLOCK_SIZE 128
#define IMAGE_VERSION_NAME_SIZE 75
#define IMAGE_VERSION_VARIANT_SIZE 20
#define IMAGE_VERSION_OEM_SIZE 32

typedef enum
{
	FTM_FFBM_SET_MODE   =     0,
	FTM_FFBM_GET_MODE   =     1
}FFBM_CMD_CODE;

typedef enum
{
	FTM_FFBM_SUCCESS = 0,
	FTM_FFBM_FAIL = 1
}FTM_ERROR_CODE;

typedef enum
{
	BOOT_MODE_HLOS = 0,
	BOOT_MODE_FFBM = 1
}BOOT_MODE;

typedef PACKED struct
{
	diagpkt_cmd_code_type              cmd_code;
	diagpkt_subsys_id_type             subsys_id;
	diagpkt_subsys_cmd_code_type       subsys_cmd_code;
	uint16                             ffbm_cmd_code;
	uint16                             reserved;
	uint16                             reserved1;
}__attribute__((packed))ffbm_pkt_type;

/* FFBM Set Mode command request packet */
typedef PACKED struct
{
	ffbm_pkt_type        ftm_header;
	uint8                iNextBootMode;
	uint8                iNextBootSubMode;
}__attribute__((packed))ffbm_set_mode_req_type;

/* FFBM Set Mode command respond packet */
typedef PACKED struct
{
	ffbm_pkt_type        ftm_header;
	uint16               iFTM_Error_Code;
}__attribute__((packed))ffbm_set_mode_rsq_type;

/* FFBM Get Mode command request packet */
typedef PACKED struct
{
	ffbm_pkt_type        ftm_header;
	uint16               iFTM_Error_Code;
	uint8                iCurrentBootMode;
	uint8                iCurrentBootSubMode;
	uint8                iNextBootMode;
	uint8                iNextBootSubMode;
}__attribute__((packed))ffbm_get_mode_rsq_type;

/* bootselect partition format structure */
typedef struct {
	uint32_t signature;                // Contains value BOOTSELECT_SIGNATURE defined above
	uint32_t version;
	uint32_t boot_partition_selection; // Decodes which partitions to boot: 0-Windows,1-Android
	uint32_t state_info;               // Contains factory and format bit as definded above
} boot_selection_info;

void * ffbm_dispatch(ffbm_pkt_type *ffbm_pkt);
void * ffbm_get_mode();
void * ffbm_set_mode(ffbm_set_mode_req_type *ffbm_pkt);

