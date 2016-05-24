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

static const char *TAG = "TOUCHPANEL";
static hash_map < string, string > paras;
static mmi_module *g_module;
static int g_retval;
static sem_t g_sem;

#define MAX_KEY_NUM 4

const char *panel_btn_name[MAX_KEY_NUM] = { "menu", "home", "back", "search" };

enum {
    MENU,
    HOME,
    BACK,
    SEARCH
};
static mmi_window *window;
static class mmi_button *btn_back, *btn_home, *btn_menu;

static mmi_key_cb_t backup_back;
static mmi_key_cb_t backup_home;
static mmi_key_cb_t backup_homepage;
static mmi_key_cb_t backup_menu;

int module_main(mmi_module * mod);
void pass(void *);
void fail(void *);

void __attribute__ ((constructor)) register_module(void);

void register_module(void) {
    g_module = mmi_module::register_module(TAG, module_main);
}

static void cb_back(int key) {
    btn_back->set_color(255, 0, 0, 255);
}

static void cb_home(int key) {
    btn_home->set_color(255, 0, 0, 255);
}

static void cb_menu(int key) {
    btn_menu->set_color(255, 0, 0, 255);
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
    case MENU:
        rect.y = height / 20;
        btn_menu = new mmi_button(rect, "MENU ", NULL);
        btn_menu->set_color(0, 125, 125, 255);
        g_module->add_btn(btn_menu);
        break;

    case HOME:
        rect.y = height * 4 / 20;
        btn_home = new mmi_button(rect, "HOME ", NULL);
        btn_home->set_color(0, 125, 125, 255);
        g_module->add_btn(btn_home);
        break;
    case BACK:
        rect.y = height * 7 / 20;
        btn_back = new mmi_button(rect, "BACK ", NULL);
        btn_back->set_color(0, 125, 125, 255);
        g_module->add_btn(btn_back);
        break;

    case SEARCH:
        break;
    default:
        break;

    }
}
void initUI(char *virtual_btn) {
    backup_back = mmi_key::get_key_cb(KEY_BACK);
    backup_home = mmi_key::get_key_cb(KEY_HOME);
    backup_homepage = mmi_key::get_key_cb(KEY_HOMEPAGE);
    backup_menu = mmi_key::get_key_cb(KEY_MENU);

    mmi_key::set_key_cb(KEY_BACK, cb_back);
    mmi_key::set_key_cb(KEY_HOME, cb_home);
    mmi_key::set_key_cb(KEY_HOMEPAGE, cb_home);
    mmi_key::set_key_cb(KEY_MENU, cb_menu);

    window = new mmi_window();

    char *bkup_ptr;
    char *p = strtok_r(virtual_btn, ",", &bkup_ptr);

    while(p != NULL) {
        ALOGE("%s\n", p);
        add_btn(p);
        p = strtok_r(NULL, ",", &bkup_ptr);
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
    mmi_key::set_key_cb(KEY_BACK, backup_back);
    mmi_key::set_key_cb(KEY_HOME, backup_home);
    mmi_key::set_key_cb(KEY_HOMEPAGE, backup_homepage);
    mmi_key::set_key_cb(KEY_MENU, backup_menu);
}

int module_main(mmi_module * mod) {
    // Check if we've entered here by mistake. Normally we shouldn't
    // be here.
    if (mod->get_run_mode() == TEST_MODE_PCBA) {
        return CASE_FAIL;
    }

    init();
    sem_wait(&g_sem);
    finish();
    return g_retval;
}
