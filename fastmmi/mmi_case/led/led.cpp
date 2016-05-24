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
#include <semaphore.h>

enum led_color {
    LED_NONE = 0,
    LED_RED = 1,
    LED_GREEN = 2,
    LED_BLUE = 3,
    LED_MAX
};

struct input_params {
    char color_name[16];
    char path[256];
    enum led_color color;
    int timeout;
    bool exit_state;
};
#define LOG_TAG   "LED"
#include <cutils/log.h>

static hash_map < string, string > paras;
static mmi_module *g_module;
static int module_ret;
static pthread_t processThreadPid;

#define DEFUALT_SYS_PATH_RED  "/sys/class/leds/red/brightness"
#define DEFUALT_SYS_PATH_GREEN  "/sys/class/leds/green/brightness"
#define DEFUALT_SYS_PATH_BLUE  "/sys/class/leds/blue/brightness"
#define DEFAULT_TIMETOUT 1000

static mmi_window *window;
static class mmi_button *btn_show;
static struct input_params input;

int module_main(mmi_module * mod);
void pass(void *);
void fail(void *);
void enable(bool on);

extern "C" void __attribute__ ((constructor)) register_module(void);

void register_module(void) {
    g_module = mmi_module::register_module(LOG_TAG, module_main);
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

void enable(bool on) {
    char temp[128] = { 0 };

    switch (input.color) {
    case LED_RED:
        strlcat(temp, "do you see LED-RED ", sizeof(temp));
        strlcat(temp, on ? "ON" : "OFF", sizeof(temp));
        break;
    case LED_BLUE:
        strlcat(temp, "do you see LED-BLUE ", sizeof(temp));
        strlcat(temp, on ? "ON" : "OFF", sizeof(temp));
        break;
    case LED_GREEN:
        strlcat(temp, "do you see LED-GREEN ", sizeof(temp));
        strlcat(temp, on ? "ON" : "OFF", sizeof(temp));
        break;
    default:
        break;
    }
    strlcat(temp, "?", sizeof(temp));
    btn_show->set_text(temp);

    if(on) {
        write_file(input.path, "255");
    } else {
        write_file(input.path, "0");
    }
}

void set_string_color(enum led_color color) {
    char temp[32] = { 0 };
    switch (color) {
    case LED_RED:
        btn_show->set_text("do you see the RED color light on?");
        btn_show->set_color(255, 0, 0, 255);
        break;
    case LED_BLUE:
        btn_show->set_text("do you see the BLUE color light on?");
        btn_show->set_color(0, 0, 255, 255);
        break;
    case LED_GREEN:
        btn_show->set_text("do you see the GREEN color light on?");
        btn_show->set_color(0, 255, 0, 255);
        break;
    default:
        break;
    }
}

void initUI() {
    window = new mmi_window();
    int width = window->get_width();
    int height = window->get_height();
    mmi_rect_t rect = { 10, height / 3, width - 10, height / 5 };
    btn_show = new mmi_button(rect, "LED", NULL);
    g_module->add_btn(btn_show);
    set_string_color(input.color);

    g_module->add_window(window);
    g_module->add_btn_pass(pass);
    g_module->add_btn_fail(fail);
}



void get_input() {
    char temp[256] = { 0 };
    char temp_path[256] = { 0 };
    char int_string[32];

    snprintf(int_string, sizeof(int_string), "%d", DEFAULT_TIMETOUT);
    parse_parameter(mmi_config::query_config_value(g_module->get_domain(), "parameter"), paras);
    get_para_value(paras, "color", temp, sizeof(temp), NULL);
    memset(&input, 0, sizeof(struct input_params));
    strlcpy(input.color_name, temp, sizeof(input.color_name));
    if(!strncmp(input.color_name, "red", 3)) {
        input.color = LED_RED;
        get_para_value(paras, "path", temp_path, sizeof(temp_path), DEFUALT_SYS_PATH_RED);
    } else if(!strncmp(input.color_name, "green", 5)) {
        input.color = LED_GREEN;
        get_para_value(paras, "path", temp_path, sizeof(temp_path), DEFUALT_SYS_PATH_GREEN);
    } else if(!strncmp(input.color_name, "blue", 4)) {
        input.color = LED_BLUE;
        get_para_value(paras, "path", temp_path, sizeof(temp_path), DEFUALT_SYS_PATH_BLUE);
    } else
        input.color = LED_NONE;

    strlcpy(input.path, temp_path, sizeof(input.path));
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

    ALOGE("config LED , %s:%d. path=%s; timeout=%d(ms), exit_state:%d",
                 input.color_name, input.color, input.path, input.timeout,
                 input.exit_state);

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
    get_input();

    if (!strncmp(g_module->get_domain(), "LED_RED_ON", strlen("LED_RED_ON")) ||
        !strncmp(g_module->get_domain(), "LED_GREEN_ON", strlen("LED_GREEN_ON"))) {
        if (input.exit_state == true) {
            if (write_file_res(input.path, "255")) {
                return CASE_FAIL;
            }
        } else {
            return CASE_FAIL;
        }
    } else if (
        !strncmp(g_module->get_domain(),
                        "LED_RED_OFF",
                        strlen("LED_RED_OFF")) ||
        !strncmp(g_module->get_domain(),
                        "LED_GREEN_OFF",
                        strlen("LED_GREEN_OFF"))) {
        if (input.exit_state == false) {
            if (write_file_res(input.path, "0")) {
                return CASE_FAIL;
            }
        } else {
            return CASE_FAIL;
        }
    } else {
        return CASE_FAIL;
    }
    return CASE_SUCCESS;
}

int module_main(mmi_module * mod) {
    if(mod == NULL)
        return -1;
    g_module = mod;

    // PCBA tests first. No init needed here.
    if (g_module->get_run_mode() == TEST_MODE_PCBA) {
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
