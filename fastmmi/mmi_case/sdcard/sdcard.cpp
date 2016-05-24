/*
 * Copyright (c) 2013-2014, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <mmi_window.h>
#include <mmi_module_manage.h>
#include <mmi_config.h>
#include <semaphore.h>
#include "mmi_utils.h"

#define LOG_TAG   "SDCARD"
#include <cutils/log.h>

#define DEFAULT_SDCARD_DEV_PATH  "/dev/block/mmcblk1"

struct input_params {
    char device[256];
};
static struct input_params input;
static hash_map < string, string > paras;
static mmi_module *g_module;
static sem_t g_sem;
static int module_ret;

int module_main(mmi_module * mod);
void __attribute__ ((constructor)) register_module(void);

void pass(void *) {
    module_ret = 0;
    sem_post(&g_sem);
}

void fail(void *) {
    module_ret = -1;
    sem_post(&g_sem);
}

void register_module(void) {
    g_module = mmi_module::register_module(LOG_TAG, module_main);
}
void get_input() {
    char temp[256] = { 0 };
    memset(&input, 0, sizeof(struct input_params));
    parse_parameter(mmi_config::query_config_value(g_module->get_domain(), "parameter"), paras);
    get_para_value(paras, "device", temp, sizeof(temp), DEFAULT_SDCARD_DEV_PATH);
    strlcpy(input.device, temp, sizeof(input.device));
    ALOGE("sd config: path=%s\n", input.device);
}

bool is_sdcard_exist() {
    bool ret = false;
    int fd = open(input.device, O_RDWR);

    if(fd >= 0) {
        ret = true;
        close(fd);
    }
    return ret;
}
int manual_test() {
    sem_init(&g_sem, 0, 0);
    mmi_window *window = new mmi_window();
    int width = window->get_width();
    int height = window->get_height();

    g_module->add_window(window);
    mmi_rect_t rect = { 10, height / 6, width - 20, height / 4 };
    mmi_button *btn = new mmi_button(rect, "start to detect sd card", NULL);

    if(is_sdcard_exist()) {
        btn->set_color(0, 125, 125, 255);
        btn->set_text("SD card detected");
    } else {
        btn->set_color(125, 125, 0, 255);
        btn->set_text("SD card not detected");
    }

    g_module->add_btn(btn);
    g_module->add_btn_pass(pass);
    g_module->add_btn_fail(fail);

    sem_wait(&g_sem);
    g_module->clean_source();

    return module_ret;
}
int module_main(mmi_module * mod) {
    get_input();
    case_run_mode_t mode = g_module->get_run_mode();

    if(mode == TEST_MODE_PCBA) {
        if (is_sdcard_exist()) {
            g_module->addStringResult("DETECTED", "yes");
            return CASE_SUCCESS;
        } else {
            g_module->addStringResult("DETECTED", "no");
            return PCBA_SD_CARD_READ_FAIL;
        }
    } else if(mode == TEST_MODE_SANITY) {
        return CASE_FAIL;
    } else {
        return manual_test();
    }
}
