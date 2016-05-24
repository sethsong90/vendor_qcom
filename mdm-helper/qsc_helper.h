/*
 *qsc_helper: QSC related functions used by mdm_helper
 *
 *Copyright (C) 2012 Qualcomm Technologies, Inc. All rights reserved.
 *                   Qualcomm Technologies Proprietary/GTDR
 *
 *All data and information contained in or disclosed by this document is
 *confidential and proprietary information of Qualcomm Technologies, Inc. and all
 *rights therein are expressly reserved.  By accepting this material the
 *recipient agrees that this material and the information contained therein
 *is held in confidence and in trust and will not be used, copied, reproduced
 *in whole or in part, nor its contents revealed in any manner to others
 *without the express written permission of Qualcomm Technologies, Inc.
 *
 *qsc_helper.h : Functions used by mdm_helper to  monitor and interact with QSC
 *module
 */
#include "mdm_helper.h"

#define NUM_DLOAD_DETECT_RETRIES 60
#define MDM_POLL_DELAY   (500)
#define NUM_RETRIES      (100)
#define DELAY_BETWEEN_RETRIES_MS       (500)
#define UART_MODE_TTY 1
#define UART_MODE_SMUX 2
#define QSC_UART_NODE "/dev/ttyHS1"
#define IMAGE_UPGRADE_STATUS_FILE "/data/misc/mdmhelperdata/upgrade_in_progress"
#define QSC_HELPER_COMMAND_UPGRADE_IMAGE "upgrade_amss"
#define QSC_HELPER_COMMAND_SWITCH_USB_CONTROL "switch_usb"
#define QSC_HELPER_COMMAND_UPGRADE_START "--start_upgrade"
#define QSC_HELPER_COMMAND_UPGRADE_DONE "--finish_upgrade"
#define QSC_HELPER_COMMAND_LENGTH 20
#define QSC_HELPER_SOCKET "/data/misc/mdmhelperdata/qschelpersocket"
#define QSC_HELPER_CMD_BUF_SIZE 512
#define N_SMUX 25
#define N_TTY 0
#define BAUD_RATE_3200 B200

typedef enum DloadQueryType {
	DLOAD_QUERY_TILL_RETRY_LIMIT = 0,
	DLOAD_QUERY_SINGLE
}DloadQueryType;
int power_up_qsc(struct mdm_device *);
int reboot_qsc(struct mdm_device *);
int monitor_qsc(struct mdm_device *);
int prepare_qsc_for_ramdumps(struct mdm_device *);
int collect_ramdumps_from_qsc(struct mdm_device *);
int qsc_helper_post_ramdump_cleanup(struct mdm_device *);
int qsc_helper_failure_cleanup(struct mdm_device *);
int standalone_task(struct mdm_device *, char *argv[]);

