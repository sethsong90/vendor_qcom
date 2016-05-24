/*
 * Copyright (c) 2013-2014, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include <stdio.h>
#include <unistd.h>
#include <mmi_window.h>
#include <mmi_module_manage.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include "mmi_utils.h"
#include "mmi_config.h"

#define LOG_TAG   "Keybacklight"
#include <cutils/log.h>

struct input_params {
    char path[256];
    int timeout;
};
static struct input_params input;

static hash_map < string, string > paras;
static mmi_module *g_module;
static int module_ret;
static pthread_t processThreadPid;
static class mmi_button *btn;

#define DEFAULT_SYS_KEY_BACKLIGHT_PATH "/sys/class/leds/button-backlight/brightness"
#define DEFAULT_TIMETOUT 1000

int module_main(mmi_module * mod);
void pass(void *);
void fail(void *);

void __attribute__ ((constructor)) register_module(void);

void register_module(void) {
    g_module = mmi_module::register_module(LOG_TAG, module_main);
}

void enable(bool enable) {
    if(enable) {
        write_file(input.path, "255");
        btn->set_text("do you see keyback light ON?");
    } else {
        write_file(input.path, "0");
        btn->set_text("do you see keyback light OFF?");
    }
}


void signalHandler(int signal) {
    pthread_exit(NULL);
}

/**Test process or hardware init*/
void *processThread(void *) {
    signal(SIGUSR1, signalHandler);
    while(1) {
        enable(true);
        usleep(input.timeout * 1000);
        enable(false);
        usleep(input.timeout * 1000);
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

    mmi_window *window = new mmi_window();
    int width = window->get_width();
    int height = window->get_height();
    mmi_rect_t rect = { 10, height / 3, width - 10, height / 5 };
    btn = new mmi_button(rect, "do you see the key backlight on ? ", NULL);
    btn->set_color(0, 125, 125, 255);
    g_module->add_btn(btn);

    g_module->add_btn_pass(pass);
    g_module->add_btn_fail(fail);
}

void get_input() {
    char temp[256] = { 0 };
    char int_string[32];

    snprintf(int_string, sizeof(int_string), "%d", DEFAULT_TIMETOUT);
    memset(&input, 0, sizeof(struct input_params));
    parse_parameter(mmi_config::query_config_value(g_module->get_domain(), "parameter"), paras);
    get_para_value(paras, "timeout", temp, sizeof(temp), int_string);
    input.timeout = atoi(temp);
    get_para_value(paras, "path", temp, sizeof(temp), DEFAULT_SYS_KEY_BACKLIGHT_PATH);
    strlcpy(input.path, temp, sizeof(input.path));

    ALOGE("config keypad light ,path=%s;timeout=%d(ms)\n", input.path, input.timeout);
}

/**do some init work for data, UI but not hardware */
void init() {
    module_ret = -1;
    get_input();
    initUI();
}

/**To clean source; diable hardware; free memory*/
void finish() {
    enable(false);
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
