/*
 * Copyright (c) 2013-2014, Qualcomm Technologies, Inc. All Rights Reserved.
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

#define LOG_TAG   "FLASHLIGHT"
#include <cutils/log.h>

static hash_map < string, string > paras;
static mmi_module *g_module;
static int module_ret;
static pthread_t processThreadPid;
static class mmi_button *btn;

#ifdef BUILD_FLASH_DEBUGFS
#define FLASHLIGHT0_SYS_PATH  "/sys/kernel/debug/ledflash"
#else
#define FLASHLIGHT0_SYS_PATH "/sys/class/leds/torch-light/brightness"
#define FLASHLIGHT1_SYS_PATH "/sys/class/leds/led:flash_1/brightness"
#endif
#define DEFAULT_TIMETOUT 2000

struct input_params {
    int timeout;
    bool exit_state;
};
static struct input_params input;

int module_main(mmi_module * mod);
void pass(void *);
void fail(void *);

// extern "C", because we use this in dynamic linking in cpp code, and dlsym()
// isn't able to obtain the address of a symbol "register_module".
extern "C" void __attribute__ ((constructor)) register_module(void);

void register_module(void) {
    g_module = mmi_module::register_module(LOG_TAG, module_main);
}

void enable(bool enable) {
    if(enable) {
        write_file(FLASHLIGHT0_SYS_PATH, "255");
        write_file(FLASHLIGHT1_SYS_PATH, "255");
        btn->set_text("Flashlight ON");
    } else {
        write_file(FLASHLIGHT0_SYS_PATH, "0");
        write_file(FLASHLIGHT1_SYS_PATH, "0");
        btn->set_text("Flashlight OFF");
    }
}

int  enableNoUi(bool enable) {
    int ret = CASE_SUCCESS;

    if (enable) {
        ret |= write_file_res(FLASHLIGHT0_SYS_PATH, "255");
#ifndef BUILD_FLASH_DEBUGFS
        ret |= write_file_res(FLASHLIGHT1_SYS_PATH, "255");
#endif
    } else {
        ret |= write_file_res(FLASHLIGHT0_SYS_PATH, "0");
#ifndef BUILD_FLASH_DEBUGFS
        ret |= write_file_res(FLASHLIGHT1_SYS_PATH, "0");
#endif
    }
    return ret;
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
    btn = new mmi_button(rect, "do you see the flash light on ? ", NULL);
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

    // Now get the exit_state parameter...
    get_para_value(paras, "exit_state", temp, sizeof(temp), "off");
    if (!strncmp(temp, "on", 2)) {
        input.exit_state = true;
    } else {
        // If not specified or "off":
        input.exit_state = false;
    }

    ALOGE("config Flashlight ,timeout=%d(ms), exit_state=%d",
        input.timeout, input.exit_state);
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

int auto_test() {
    int ret = CASE_SUCCESS;

    get_input();
    if (!strncmp(g_module->get_domain(),
        "FLASHLIGHT_ON",
        strlen("FLASHLIGHT_ON"))) {
        if (input.exit_state == true) {
            ret |= enableNoUi(true);
        } else {
            return CASE_FAIL;
        }
    } else if (!strncmp(g_module->get_domain(),
                "FLASHLIGHT_OFF",
                strlen("FLASHLIGHT_OFF"))) {
                if (input.exit_state == false) {
                    ret |= enableNoUi(false);
                } else {
                    return CASE_FAIL;
                }
        } else {
            // Probably we've received wrong config: PCBA + FLASHLIGHT...
            return CASE_FAIL;
        }

    return ret;
}
int module_main(mmi_module * mod) {
    if (mod == NULL)
        return -1;
    g_module = mod;

    // PCBA tests first. No init needed here.
    if (mod->get_run_mode() == TEST_MODE_PCBA) {
        return auto_test();
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
