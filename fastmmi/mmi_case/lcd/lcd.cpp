/*
 * Copyright (c) 2012-2014, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include <stdio.h>
#include <unistd.h>
#include <mmi_window.h>
#include <mmi_module_manage.h>
#include "mmi_utils.h"
#include "mmi_config.h"
#include <hash_map>
#include <pthread.h>
#include <errno.h>
extern "C" {
#include <minui.h>
}
#include <semaphore.h>

#define LOG_TAG   "LCD"
#include <cutils/log.h>
static hash_map < string, string > paras;
static mmi_module *g_module;
static int module_ret;
static pthread_t processThreadPid;
static sem_t g_sem;

int module_main(mmi_module * mod);
void pass(void *);
void fail(void *);

void __attribute__ ((constructor)) register_module(void);

void register_module(void) {
    g_module = mmi_module::register_module(LOG_TAG, module_main);
}

void signalHandler(int signal) {
    pthread_exit(NULL);
}

void btn_cb_next(void *) {
    sem_post(&g_sem);
}

/**Test process or hardware init*/
void *processThread(void *) {
    // register signal handler
    signal(SIGUSR1, signalHandler);

    sem_init(&g_sem, 0, 0);

    int width = gr_fb_width();
    int height = gr_fb_height();

    class mmi_button *btn;
    mmi_rect_t rect_next = { 0, 0, width, height };
    btn = new mmi_button(rect_next, "next", btn_cb_next);
    g_module->add_btn(btn);

    btn->set_color(255, 0, 0, 255);
    sem_wait(&g_sem);

    btn->set_color(0, 255, 0, 255);
    sem_wait(&g_sem);

    btn->set_color(0, 0, 255, 255);
    sem_wait(&g_sem);

    btn->set_color(255, 255, 255, 255);
    sem_wait(&g_sem);

    btn->set_color(0, 0, 0, 255);
    sem_wait(&g_sem);

    sem_close(&g_sem);

    g_module->clean_btn();
    g_module->add_btn_pass(pass);
    g_module->add_btn_fail(fail);

    while(1);
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
