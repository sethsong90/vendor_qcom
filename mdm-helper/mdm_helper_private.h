/*
 *mdm_helper: A module to monitor modem activities on fusion devices
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
 *mdm_helper_private.h : Header file needed by mdm_helper
 */
#include "qsc_helper.h"
#include "9x15_helper.h"
#include "mdm_helper.h"

#define MDM_POLL_DELAY              (500)
#define NUM_RETRIES                 (100)
#define DELAY_BETWEEN_RETRIES_MS    (500)
#define BASEBAND_NAME_LENGTH 30

typedef enum MdmHelperStates {
	MDM_HELPER_STATE_POWERUP = 0,
	MDM_HELPER_STATE_POST_POWERUP,
	MDM_HELPER_STATE_RAMDUMP,
	MDM_HELPER_STATE_REBOOT,
	MDM_HELPER_STATE_FAIL
} MdmHelperStates;

struct mdm_device sglte_device[] = {
	{
		.mdm_name = "qsc",
		.mdm_port = "/dev/mdm",
		.device_descriptor = 0,
		.required_action = MDM_REQUIRED_ACTION_NONE,
		.ram_dump_path = "/tombstones/mdm",
		.images_path = NULL,
		.ops = {
			.power_up = power_up_qsc,
			.post_power_up = monitor_qsc,
			.reboot = reboot_qsc,
			.prep_for_ramdumps = prepare_qsc_for_ramdumps,
			.collect_ramdumps = collect_ramdumps_from_qsc,
			.post_ramdump_collect = qsc_helper_post_ramdump_cleanup,
			.failure_cleanup = qsc_helper_failure_cleanup,
			.standalone_mode = standalone_task,
		},
		.private_data = NULL,
	},
};

struct mdm_device dsda_device[] = {
	{
		.mdm_name = "qsc",
		.mdm_port = "/dev/mdm1",
		.device_descriptor = 0,
		.required_action = MDM_REQUIRED_ACTION_NONE,
		.ram_dump_path = "/tombstones/mdm",
		.images_path = NULL,
		.ops = {
			.power_up = power_up_qsc,
			.post_power_up = monitor_qsc,
			.reboot = reboot_qsc,
			.prep_for_ramdumps = prepare_qsc_for_ramdumps,
			.collect_ramdumps = collect_ramdumps_from_qsc,
			.post_ramdump_collect = qsc_helper_post_ramdump_cleanup,
			.failure_cleanup = qsc_helper_failure_cleanup,
			.standalone_mode = standalone_task,
		},
		.private_data = NULL,
	},
	{
		.mdm_name = "mdm9x15",
		.mdm_port = "/dev/mdm",
		.device_descriptor = 0,
		.required_action = MDM_REQUIRED_ACTION_NONE,
		.ram_dump_path = NULL,
		.images_path = NULL,
		.ops = {
			.power_up = power_up_9x15,
			.post_power_up = monitor_9x15,
			.reboot = reboot_9x15,
			.prep_for_ramdumps = prepare_9x15_for_ramdumps,
			.collect_ramdumps = collect_ramdumps_from_9x15,
			.post_ramdump_collect = mdm_9x15_post_ramdump_cleanup,
			.failure_cleanup = NULL,
			.standalone_mode = NULL,
		},
		.private_data = NULL,
	},
};

struct mdm_device fusion3_device[] = {
	{
		.mdm_name = "mdm9x15",
		.mdm_port = "/dev/mdm",
		.device_descriptor = 0,
		.required_action = MDM_REQUIRED_ACTION_NONE,
		.ram_dump_path = "/tombstones/mdm/",
		.images_path = "/firmware/image/",
		.ops = {
			.power_up = power_up_9x15,
			.post_power_up = monitor_9x15,
			.reboot = reboot_9x15,
			.prep_for_ramdumps = prepare_9x15_for_ramdumps,
			.collect_ramdumps = collect_ramdumps_from_9x15,
			.post_ramdump_collect = mdm_9x15_post_ramdump_cleanup,
			.failure_cleanup = NULL,
			.standalone_mode = NULL,
		},
		.private_data = &private_data_9x15_hsic,
	},
};

struct mdm_device dsda2_device[] = {
	{
		.mdm_name = "mdm9x15_hsic",
		.mdm_port = "/dev/mdm",
		.device_descriptor = 0,
		.required_action = MDM_REQUIRED_ACTION_NONE,
		.ram_dump_path = "/tombstones/mdm/",
		.images_path = "/firmware/image/",
		.ops = {
			.power_up = power_up_9x15,
			.post_power_up = monitor_9x15,
			.reboot = reboot_9x15,
			.prep_for_ramdumps = prepare_9x15_for_ramdumps,
			.collect_ramdumps = collect_ramdumps_from_9x15,
			.post_ramdump_collect = mdm_9x15_post_ramdump_cleanup,
			.failure_cleanup = NULL,
			.standalone_mode = NULL,
		},
		.private_data = &private_data_9x15_hsic_dsda,
	},
	{
		.mdm_name = "mdm9x15_hsusb",
		.mdm_port = "/dev/mdm1",
		.device_descriptor = 0,
		.required_action = MDM_REQUIRED_ACTION_NONE,
		.ram_dump_path = "/tombstones/mdm2/",
		.images_path = "/firmware/image/",
		.ops = {
			.power_up = power_up_9x15,
			.post_power_up = monitor_9x15,
			.reboot = reboot_9x15,
			.prep_for_ramdumps = prepare_9x15_for_ramdumps,
			.collect_ramdumps = collect_ramdumps_from_9x15,
			.post_ramdump_collect = mdm_9x15_post_ramdump_cleanup,
			.failure_cleanup = NULL,
			.standalone_mode = NULL,
		},
		.private_data = &private_data_9x15_usb_dsda,
	},
};

struct mdm_device sglte2_device[] = {
	{
		.mdm_name = "qsc",
		.mdm_port = "/dev/mdm1",
		.device_descriptor = 0,
		.required_action = MDM_REQUIRED_ACTION_NONE,
		.ram_dump_path = "/tombstones/mdm",
		.images_path = NULL,
		.ops = {
			.power_up = power_up_qsc,
			.post_power_up = monitor_qsc,
			.reboot = reboot_qsc,
			.prep_for_ramdumps = prepare_qsc_for_ramdumps,
			.collect_ramdumps = collect_ramdumps_from_qsc,
			.post_ramdump_collect = qsc_helper_post_ramdump_cleanup,
			.failure_cleanup = qsc_helper_failure_cleanup,
			.standalone_mode = standalone_task,
		},
		.private_data = NULL,
	},
	{
		.mdm_name = "mdm9x15",
		.mdm_port = "/dev/mdm",
		.device_descriptor = 0,
		.required_action = MDM_REQUIRED_ACTION_NONE,
		.ram_dump_path = NULL,
		.images_path = NULL,
		.ops = {
			.power_up = power_up_9x15,
			.post_power_up = monitor_9x15,
			.reboot = reboot_9x15,
			.prep_for_ramdumps = prepare_9x15_for_ramdumps,
			.collect_ramdumps = collect_ramdumps_from_9x15,
			.post_ramdump_collect = mdm_9x15_post_ramdump_cleanup,
			.failure_cleanup = NULL,
			.standalone_mode = NULL,
		},
		.private_data = &private_data_9x15_hsic,
	},
};

struct mdm_helper_drv supported_devices[] = {
	{
		.baseband_name = "sglte",
		.num_modems = 1,
		.dev = sglte_device,
	},
	{
		.baseband_name = "sglte2",
		.num_modems = 2,
		.dev = sglte2_device,
	},
	{
		.baseband_name = "dsda",
		.num_modems = 2,
		.dev = dsda_device,
	},
	{
		.baseband_name = "mdm",
		.num_modems = 1,
		.dev = fusion3_device,
	},
	{
		.baseband_name = "dsda2",
		.num_modems = 2,
		.dev = dsda2_device,
	},
};
