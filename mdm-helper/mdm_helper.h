/*
 * mdm_helper: A module to monitor modem activities on fusion devices
 *
 * Copyright (C) 2012 Qualcomm Technologies, Inc. All rights reserved.
 *                    Qualcomm Technologies Proprietary/GTDR
 *
 * All data and information contained in or disclosed by this document is
 * confidential and proprietary information of Qualcomm Technologies, Inc. and all
 * rights therein are expressly reserved.  By accepting this material the
 * recipient agrees that this material and the information contained therein
 * is held in confidence and in trust and will not be used, copied, reproduced
 * in whole or in part, nor its contents revealed in any manner to others
 * without the express written permission of Qualcomm Technologies, Inc.
 *
 *  mdm_helper.h : Header file needed by mdm_helper.c
 */

#ifndef MDM_HELPER_PRIVATE_H
#define MDM_HELPER_PRIVATE_H

#define RET_SUCCESS 0
#define RET_FAILED 1
#define LOG_TAG "mdm_helper"
#define LOG_NIDEBUG 0
#define MDM_NAME_LEN 32
#include <cutils/log.h>
#include "common_log.h"

typedef enum MdmRequiredAction {
	MDM_REQUIRED_ACTION_NONE = 0,
	MDM_REQUIRED_ACTION_NORMAL_BOOT,
	MDM_REQUIRED_ACTION_RAMDUMPS
} MdmRequiredAction;

struct mdm_helper_drv;
struct mdm_device;

struct mdm_helper_ops{
	int (*power_up)(struct mdm_device *dev);
	int (*post_power_up)(struct mdm_device *dev);
	int (*reboot)(struct mdm_device *dev);
	int (*prep_for_ramdumps)(struct mdm_device *dev);
	int (*collect_ramdumps)(struct mdm_device *dev);
	int (*post_ramdump_collect)(struct mdm_device *dev);
	int (*failure_cleanup)(struct mdm_device *dev);
	int (*standalone_mode)(struct mdm_device *dev, char *argv[]);
};

struct mdm_device {
	char mdm_name[MDM_NAME_LEN];
	char mdm_port[MDM_NAME_LEN];
	int device_descriptor;
	MdmRequiredAction required_action;
    char ram_dump_path[MDM_NAME_LEN];
    char images_path[MDM_NAME_LEN];
	struct mdm_helper_ops ops;
    void* private_data;
};

struct mdm_helper_drv {
	char *baseband_name;
	int num_modems;
	struct mdm_device *dev;
};
#endif
