/******************************************************************************
 * 	D E C I S I O N . H

 Copyright (c) 2011,2013 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.
 ******************************************************************************/

#ifndef DECISION_H
#define DECISION_H

#define MAX_CPUS (4) /* Currently supporting up to 4 core systems */

#ifdef USE_ANDROID_LOG
#define LOG_TAG "MP-Decision"
#include "cutils/log.h"
#ifdef LOGE
#define msg(format, ...)   LOGE(format, ## __VA_ARGS__)
#define info(format, ...)   LOGI(format, ## __VA_ARGS__)
#else
#define msg(format, ...)   ALOGE(format, ## __VA_ARGS__)
#define info(format, ...)   ALOGI(format, ## __VA_ARGS__)
#endif /* ifdef LOGE */
#else
#define msg(format, ...)   printf(format, ## __VA_ARGS__)
#define info(format, ...)   printf(format, ## __VA_ARGS__)
#endif

#define dbgmsg(format, ...) \
	if (debug_output) \
		info(format, ## __VA_ARGS__)

enum CORE_STATUS {
	CORE_DOWN = 0,
	CORE_UP = 1,
};

int debug_output;
/* External hook for requesting mpdecision to lock a minimum
   number of cores.
   * min - no. of cores to be locked.
   Return value: 0 if successful, -1 if failed */
int mpdecision_lock_min_cores(int min);

/* External hook for requesting mpdecision to lock the maximum
   number of cores.
   * max - no. of cores to be allowed
   Return value: 0 if successful, -1 if failed */
int mpdecision_lock_max_cores(int max);

/* External hook for enabling and disabling power collapse */
int mpdecision_toggle_power_collapse(int on);

void mpdecision_get_core_mapping(int *aggregated_order);
#endif
