/*
 * Copyright (c) 2013-2014, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef __SYSTEM_CORE_MMI__
#define __SYSTEM_CORE_MMI__

#include <pthread.h>
#include <linux/input.h>
#include "mmi_state.h"

#define LOG_TAG   "MMI"
#include <cutils/log.h>

#define MMI_DOMAIN "MMI"
#define DEFAULT_MISC_DEV_PATH "/dev/block/platform/msm_sdcc.1/by-name/misc"
#define DEFAULT_SYS_BACKLIGHT "/sys/class/leds/lcd-backlight/brightness"
#define DEFAULT_REFLRESH_INTERVAL_MS "50"
struct input_params {
    char misc_dev[256];
    char sys_backlight[256];
    int refresh_interval;
    case_run_mode_t test_mode;
};

#define ARRAY_SIZE(x)           (sizeof(x)/sizeof(x[0]))

#define BASE_DIR "/data/FTM_AP/"
#define DEFAULT_CFG "/etc/mmi.cfg"
#define CURRENT_CFG_SEQ "/data/FTM_AP/cfg.seq"
#define DEFAULT_PCBA_CFG_INSTALATION "/system/etc/MMI_PCBA.cfg"
#define DEFAULT_PCBA_CFG "/data/FTM_AP/MMI_PCBA.cfg"
#define DEBUG_LOG "/data/FTM_AP/mmi.log"
#define DEFUAL_RES_FILE "/data/FTM_AP/result.res"
#define EXEC_DELAY_NS  (1000 * 10)

mmi_module *get_current_module();
mmi_module *get_main_module();
void set_boot_mode(boot_mode_type bootmode);

int get_mmi_state(void);
int get_mmi_fail_count(void);
int reconfig_mmi(char *cfg_filename, int *numOfTestCaseLoaded);
int excute_single_test(const char *domain);
int excute_all_test();
int test_list_to_file(char *filepath);
int clear_cur_test_result(void);
void clean_up(void);
void init_config(void);

void write_result(const char *domain, const char *result);
void write_result(const char *domain, const int result);

#endif
