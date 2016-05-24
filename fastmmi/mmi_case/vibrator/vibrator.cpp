/*
 * Copyright (c) 2012-2014, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include <stdio.h>
#include <unistd.h>
#include <mmi_window.h>
#include <mmi_module_manage.h>
#include <hardware_legacy/vibrator.h>
#include "mmi_utils.h"
#include "mmi_config.h"
#include <hash_map>
#include <pthread.h>
#include <errno.h>

#define LOG_TAG   "VIBRATOR"
#include <cutils/log.h>

static const char *TAG = "VIBRATOR";
static hash_map < string, string > paras;
static mmi_module *g_module;
static int module_ret;
static pthread_t processThreadPid;

int module_main(mmi_module * mod);
void pass(void *);
void fail(void *);

void __attribute__ ((constructor)) register_module(void);

void register_module(void) {
    g_module = mmi_module::register_module(TAG, module_main);
}

void signalHandler(int signal) {
    pthread_exit(NULL);
}

/**Test process or hardware init*/
void *processThread(void *) {
    // register signal handler
    signal(SIGUSR1, signalHandler);
    while(1) {
        vibrator_on(200);
        usleep(600 * 1000);
    }
    return NULL;
}

void pass(void *) {
    module_ret = 0;
    pthread_kill(processThreadPid, SIGUSR1);
}

void fail(void *) {
    module_ret = -1;
    pthread_kill(processThreadPid, SIGUSR1);
}

void initUI() {
    mmi_point_t point = { gr_fb_width() / 4, gr_fb_height() / 3 };
    mmi_text *text = new mmi_text(point, "Vibrating...");
    g_module->add_text(text);
    g_module->add_btn_pass(pass);
    g_module->add_btn_fail(fail);
}

/**do some init work for data, UI but not hardware */
void init() {
    module_ret = -1;
    parse_parameter(mmi_config::query_config_value(g_module->get_domain(), "parameter"), paras);
    initUI();
}

/**To clean source; diable hardware; free memory*/
void finish() {
    g_module->clean_source();
}

int module_main(mmi_module * mod) {
    // Check if we've entered here by mistake. Normally we shouldn't
    // be here.
    if (mod->get_run_mode() == TEST_MODE_PCBA) {
        return CASE_FAIL;
    }

    init();

    int ret = pthread_create(&processThreadPid, NULL, processThread, NULL);

    if(ret < 0)
        ALOGE("can't create pthread: %s\n", strerror(errno));
    else
        pthread_join(processThreadPid, NULL);

    finish();
    return module_ret;
}
