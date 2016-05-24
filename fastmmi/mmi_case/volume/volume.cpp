/*
 * Copyright (c) 2013-2014, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include <stdio.h>
#include <unistd.h>
#include <mmi_window.h>
#include <mmi_module_manage.h>
#include <semaphore.h>
#include <mmi_key.h>

#include "mmi_utils.h"
#include "mmi_config.h"
#include <hash_map>
#include <pthread.h>
#include <errno.h>

// Used if no timeout has been set by user
#define HW_KEYS_TIMEOUT 6

static const char *TAG = "KEYS";
static hash_map < string, string > paras;
static mmi_module *g_module;
static int g_retval;
static sem_t g_sem;
// Used in auto_test()
int keys_pressed;
int timeout;

#define MAX_KEY_NUM 3
const char *panel_btn_name[MAX_KEY_NUM] = { "volumeup", "volumedown", "camsnapshot" };

enum {
    VOLUME_UP,
    VOLUME_DOWN,
    CAMERA_SNAPSHOT,
};
static mmi_window *window;
static class mmi_button *btn_volume_up, *btn_volume_down, *btn_cam;
mmi_key_cb_t backup_volume_up;
mmi_key_cb_t backup_volume_down;
mmi_key_cb_t backup_cam;

int module_main(mmi_module * mod);
void pass(void *);
void fail(void *);

void __attribute__ ((constructor)) register_module(void);

void register_module(void) {
    g_module = mmi_module::register_module(TAG, module_main);
}

static void cb_volume_up(int key) {
    btn_volume_up->set_color(255, 0, 0, 255);
}

static void cb_volume_down(int key) {
    btn_volume_down->set_color(255, 0, 0, 255);
}

static void cb_cam(int key) {
    btn_cam->set_color(255, 0, 0, 255);
}

static void cb_volume_up_auto(int key) {
    keys_pressed &= ~(1<<VOLUME_UP);
    if (keys_pressed == 0) {
        g_retval = 0;
        sem_post(&g_sem);
    }
}

static void cb_volume_down_auto(int key) {
    keys_pressed &= ~(1<<VOLUME_DOWN);
    if (keys_pressed == 0) {
        g_retval = 0;
        sem_post(&g_sem);
    }
}

static void cb_cam_auto(int key) {
    keys_pressed &= ~ (1<<CAMERA_SNAPSHOT);
    if (keys_pressed == 0) {
        g_retval = 0;
        sem_post(&g_sem);
    }
}

void pass(void *) {
    g_retval = 0;
    sem_post(&g_sem);
}

void fail(void *) {
    g_retval = -1;
    sem_post(&g_sem);
}

void add_btn(char *p) {
    int i;
    mmi_point_t text_point;
    mmi_text *text_lebal;
    int width = window->get_width();
    int height = window->get_height();
    mmi_rect_t rect;

    for(i = 0; i < MAX_KEY_NUM - 1; i++) {
        if(!strncmp(panel_btn_name[i], p, strlen(panel_btn_name[i])))
            break;
    }
    ALOGE("set btn %s . \n", panel_btn_name[i]);
    rect.x = width / 6;
    rect.y = height / 20;
    rect.w = width * 2 / 3;
    rect.h = height / 10;

    switch (i) {
    case VOLUME_UP:
        rect.y = height / 20;
        btn_volume_up = new mmi_button(rect, "VOLUME UP ", NULL);
        btn_volume_up->set_color(0, 125, 125, 255);
        g_module->add_btn(btn_volume_up);
        break;

    case VOLUME_DOWN:
        rect.y = height * 4 / 20;
        btn_volume_down = new mmi_button(rect, "VOLUME DOWN ", NULL);
        btn_volume_down->set_color(0, 125, 125, 255);
        g_module->add_btn(btn_volume_down);
        break;
    case CAMERA_SNAPSHOT:
        rect.y = height * 7 / 20;
        btn_cam = new mmi_button(rect, "CAMERA SHAPSHOT ", NULL);
        btn_cam->set_color(0, 125, 125, 255);
        g_module->add_btn(btn_cam);
        break;

    default:
        break;

    }
}

void init_key() {
    backup_volume_up = mmi_key::get_key_cb(KEY_VOLUMEUP);
    backup_volume_down = mmi_key::get_key_cb(KEY_VOLUMEDOWN);
    backup_cam = mmi_key::get_key_cb(KEY_CAMERA_SNAPSHOT);

    mmi_key::set_key_cb(KEY_VOLUMEUP, cb_volume_up_auto);
    mmi_key::set_key_cb(KEY_VOLUMEDOWN, cb_volume_down_auto);
    mmi_key::set_key_cb(KEY_CAMERA_SNAPSHOT, cb_cam_auto);
}

void initUI(char *virtual_btn) {
    backup_volume_up = mmi_key::get_key_cb(KEY_VOLUMEUP);
    backup_volume_down = mmi_key::get_key_cb(KEY_VOLUMEDOWN);
    backup_cam = mmi_key::get_key_cb(KEY_CAMERA_SNAPSHOT);

    mmi_key::set_key_cb(KEY_VOLUMEUP, cb_volume_up);
    mmi_key::set_key_cb(KEY_VOLUMEDOWN, cb_volume_down);
    mmi_key::set_key_cb(KEY_CAMERA_SNAPSHOT, cb_cam);

    window = new mmi_window();

    char *save_ptr;
    char *p = strtok_r(virtual_btn, ",", &save_ptr);

    while(p != NULL) {
        ALOGE("%s\n", p);
        add_btn(p);
        p = strtok_r(NULL, ",", &save_ptr);
    }
    g_module->add_window(window);
    g_module->add_btn_pass(pass);
    g_module->add_btn_fail(fail);
}

void init() {
    g_retval = -1;
    sem_init(&g_sem, 0, 0);
    char virtual_btn[256] = { 0 };
    parse_parameter(mmi_config::query_config_value(g_module->get_domain(), "parameter"), paras);
    get_para_value(paras, "keys", virtual_btn, sizeof(virtual_btn), "home");
    ALOGE("keys: %s \n", virtual_btn);
    initUI(virtual_btn);
}

void finish() {
    g_module->clean_source();
    mmi_key::set_key_cb(KEY_VOLUMEDOWN, backup_volume_down);
    mmi_key::set_key_cb(KEY_VOLUMEUP, backup_volume_up);
    mmi_key::set_key_cb(KEY_CAMERA_SNAPSHOT, backup_cam);
}

void clear_key() {
    mmi_key::set_key_cb(KEY_VOLUMEDOWN, backup_volume_down);
    mmi_key::set_key_cb(KEY_VOLUMEUP, backup_volume_up);
    mmi_key::set_key_cb(KEY_CAMERA_SNAPSHOT, backup_cam);
}

int add_buttons_auto(char *virtual_btn) {
    int i;
    char *save_ptr;
    char *p = strtok_r(virtual_btn, ",", &save_ptr);

    while(p != NULL) {
        ALOGI("%s\n", p);
        for(i = 0; i < MAX_KEY_NUM - 1; i++) {
            if(!strncmp(panel_btn_name[i], p, strlen(panel_btn_name[i])))
                break;
        }
        switch (i) {
            case VOLUME_UP:
                keys_pressed |= (1<<VOLUME_UP);
                break;
            case VOLUME_DOWN:
               keys_pressed |= (1<<VOLUME_DOWN) ;
                break;
            case CAMERA_SNAPSHOT:
                keys_pressed |= (1<<CAMERA_SNAPSHOT);
                break;
            default:
                ALOGE("Unknown key, wrong config...");
                return CASE_FAIL;
        }
        p = strtok_r(NULL, ",", &save_ptr);
    }
    return CASE_SUCCESS;
}

int auto_test(){
    struct timespec ts;
    keys_pressed = 0;
    sem_init(&g_sem, 0, 0);
    char temp[256] = { 0 };

    parse_parameter(mmi_config::query_config_value(g_module->get_domain(), "parameter"), paras);
    get_para_value(paras, "timeout", temp, sizeof(temp), "-1");
    timeout = atoi(temp);
    if (timeout < 0 || 60 < timeout) {
        timeout = HW_KEYS_TIMEOUT;
    }
    get_para_value(paras, "keys", temp, sizeof(temp), "home");
    if (add_buttons_auto(temp)) {
         return CASE_FAIL;
    }
    init_key();

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        ALOGE("Failed to get clock... exiting");
        return CASE_FAIL;
    }
    ts.tv_sec += timeout;
    if (sem_timedwait(&g_sem, &ts)) {
        ALOGE("HW keys timed out");
        if (keys_pressed & (1<<VOLUME_UP))
            return PCBA_VOLUME_UP_KEY_FAIL;
        else if (keys_pressed & (1<<VOLUME_DOWN))
            return PCBA_VOLUME_DOWN_KEY_FAIL;
        else if (keys_pressed & (1<<CAMERA_SNAPSHOT))
            return PCBA_CAMERA_KEY_FAIL;
        return CASE_FAIL;
    }
    clear_key();
    return CASE_SUCCESS;
}

int module_main(mmi_module * mod) {
    if (g_module->get_run_mode() == TEST_MODE_PCBA) {
        return auto_test();
    }

    init();
    sem_wait(&g_sem);
    finish();
    return g_retval;
}
