/*
 *mdm_helper: A module to monitor modem activities on fusion devices
 *
 *Copyright (C) 2012 Qualcomm Technologies, Inc. All rights reserved.
 *                    Qualcomm Technologies Proprietary/GTDR
 *
 *All data and information contained in or disclosed by this document is
 *confidential and proprietary information of Qualcomm Technologies, Inc. and all
 *rights therein are expressly reserved.  By accepting this material the
 *recipient agrees that this material and the information contained therein
 *is held in confidence and in trust and will not be used, copied, reproduced
 *in whole or in part, nor its contents revealed in any manner to others
 *without the express written permission of Qualcomm Technologies, Inc.
 *
 *mdm_helper.c : Main implementation of mdm_helper
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <linux/msm_charm.h>
#include <linux/ioctl.h>
#include <cutils/properties.h>
#include <mdm_helper_private.h>

static struct mdm_helper_drv mdm_helper;

int mdm_helper_init()
{
	int i;
	int target_supported = 0;
	int num_supported_devices =
		sizeof(supported_devices)/sizeof(supported_devices[0]);
	char baseband_name[PROPERTY_VALUE_MAX];
	property_get("ro.baseband", baseband_name, '\0');
	if (!*baseband_name) {
		LOGE("Could not read ro.baseband\n");
		return RET_FAILED;
	}
	for(i = 0; i < num_supported_devices; i++) {
		if (!strncmp(baseband_name, supported_devices[i].baseband_name,
					BASEBAND_NAME_LENGTH)) {
			mdm_helper = supported_devices[i];
			target_supported = 1;
			break;
		}
	}
	if (!target_supported) {
		LOGE("Target %s not supported", baseband_name);
		return RET_FAILED;
	}
	LOGI("Target is %s\nNum modems %d", baseband_name,
			mdm_helper.num_modems);
	for (i = 0; i < mdm_helper.num_modems; i++) {
		if (!mdm_helper.dev[i].ops.power_up ||
				!mdm_helper.dev[i].ops.post_power_up) {
			LOGE("power_up/post_power_up undefined for %s\n",
					mdm_helper.dev[i].mdm_name);
			return RET_FAILED;
		}
	}
	return RET_SUCCESS;
}


static void* modem_state_machine(void *arg)
{
	struct mdm_device *dev = (struct mdm_device *) arg;
	MdmHelperStates state = MDM_HELPER_STATE_POWERUP;
	LOGI("Starting %s",dev->mdm_name);
	do {
		switch (state) {
		case MDM_HELPER_STATE_POWERUP:
			LOGI("%s : switching state to POWERUP", dev->mdm_name);
			if (dev->ops.power_up(dev)
					!= RET_SUCCESS) {
				LOGE("%s : Powerup failed", dev->mdm_name);
				state = MDM_HELPER_STATE_FAIL;
				break;
			}
			if (dev->required_action ==
					MDM_REQUIRED_ACTION_RAMDUMPS) {
				state = MDM_HELPER_STATE_RAMDUMP;
				dev->required_action =
					MDM_REQUIRED_ACTION_NONE;
			} else
				state = MDM_HELPER_STATE_POST_POWERUP;
			break;
		case MDM_HELPER_STATE_POST_POWERUP:
			LOGI("%s : switching state to POST POWERUP",
					dev->mdm_name);
			if (dev->ops.post_power_up(dev) !=
					RET_SUCCESS) {
				LOGE("%s : Post power_up failed",
						dev->mdm_name);
				state = MDM_HELPER_STATE_FAIL;
				break;
			}
			if (dev->required_action ==
					MDM_REQUIRED_ACTION_RAMDUMPS) {
				state = MDM_HELPER_STATE_RAMDUMP;
				dev->required_action =
					MDM_REQUIRED_ACTION_NONE;
				break;
			} else if (dev->required_action ==
					MDM_REQUIRED_ACTION_NORMAL_BOOT) {
				state = MDM_HELPER_STATE_POWERUP;
				dev->required_action =
					MDM_REQUIRED_ACTION_NONE;
				break;
			}
			LOGE("%s : post pwrup returned unsupported action:%d",
					dev->mdm_name,
					dev->required_action);
			state = MDM_HELPER_STATE_FAIL;
			break;
		case MDM_HELPER_STATE_RAMDUMP:
			LOGI("%s : Switching state to RAMDUMP",
					dev->mdm_name);
			if (dev->ops.prep_for_ramdumps) {
				if (dev->ops.\
					prep_for_ramdumps(dev)
						!= RET_SUCCESS) {
					LOGE("%s :prep_for_ramdump failed",
							dev->mdm_name);
					state = MDM_HELPER_STATE_FAIL;
					break;
				}
			}
			if (dev->ops.collect_ramdumps) {
				if (dev->ops.\
					collect_ramdumps(dev) !=
						RET_SUCCESS){
					LOGE("%s :ramdump collect failed",
							dev->mdm_name);
					state = MDM_HELPER_STATE_FAIL;
					break;
				}
			} else {
				LOGE("%s :No collect_ramdump function defined",
						dev->mdm_name);
				state = MDM_HELPER_STATE_FAIL;
				break;
			}
			if (dev->ops.post_ramdump_collect) {
				if (dev->ops.\
					post_ramdump_collect(dev)
						!= RET_SUCCESS) {
					LOGE("%s :post ramdump collect failed",
							dev->mdm_name);
					state = MDM_HELPER_STATE_FAIL;
					break;
				}
			}
			state = MDM_HELPER_STATE_POWERUP;
			break;
		case MDM_HELPER_STATE_REBOOT:
			LOGI("%s : Normal reboot request", dev->mdm_name);
			if (!dev->ops.reboot) {
				LOGE("%s : Reboot function not defined",
						dev->mdm_name);
				state = MDM_HELPER_STATE_FAIL;
				break;
			}
			if (dev->ops.reboot(dev) != RET_SUCCESS) {
				state = MDM_HELPER_STATE_FAIL;
				break;
			}
			state = MDM_HELPER_STATE_POST_POWERUP;
			break;
		case MDM_HELPER_STATE_FAIL:
			LOGE("%s : Reached failed state. exiting",
					dev->mdm_name);
			if (dev->ops.failure_cleanup) {
				LOGI("%s : Calling cleanup function",
						dev->mdm_name);
				dev->ops.failure_cleanup(dev);
			} else
				LOGI("%s : No cleanup function defined",
						dev->mdm_name);
			return 0;
		default:
			LOGE("%s : Reached unknown state %d.exiting",
					dev->mdm_name,
					state);
			state = MDM_HELPER_STATE_FAIL;
			break;
		}
	} while(1);
}

int main(int argc, char *argv[])
{
	int i;
	int rcode = 0;
	int valid_argument = 0;
	pthread_t *tid = NULL;
	if (mdm_helper_init()) {
		LOGE("Initializaion failed");
		return 0;
	}
	if (argc > 1) {
		LOGI("Standalone mode");
		for (i = 0; i < mdm_helper.num_modems; i++)
		{
			if (!strncmp(argv[1],mdm_helper.dev[i].mdm_name,
						MDM_NAME_LEN)) {
				valid_argument = 1;
				break;
			}
		}
		if (!valid_argument) {
			LOGE("Unrecognised modem :%s",argv[1]);
			return RET_FAILED;
		}
		if (mdm_helper.dev[i].ops.standalone_mode)
			return mdm_helper.dev[i].ops.\
				standalone_mode(&mdm_helper.dev[i],
					&argv[1]);
		else
			LOGE("No standalone function defined");
		return RET_FAILED;
	}
	LOGI("Starting MDM helper");
	tid = (pthread_t*) malloc(mdm_helper.num_modems * sizeof(pthread_t));
	if (!tid) {
		LOGE("Failed to create modem threads : %s",strerror(errno));
		return RET_FAILED;
	}
	for (i = 0; i < mdm_helper.num_modems; i++) {
		LOGI("Creating thread for %s",mdm_helper.dev[i].mdm_name);
		rcode = pthread_create(&tid[i], NULL,
				&modem_state_machine,
				(void*)(&(mdm_helper.dev[i])));
		if (rcode) {
			LOGE("Failed to create thread for %s\n",
					mdm_helper.dev[i].mdm_name);
			return RET_FAILED;
		}
	}
	do {
		sleep(250000);
	} while(1);
	return 0;
}
