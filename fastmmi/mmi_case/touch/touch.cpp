/*
 * Copyright (c) 2012-2014, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include <stdio.h>
#include <unistd.h>
#include <mmi_window.h>
#include <mmi_button.h>
#include <mmi_module_manage.h>
#include <mmi_utils.h>
#include <semaphore.h>
#include "mmi_state.h"

static const char *DOMAIN = "TOUCH";

int module_main(mmi_module * mod);

void __attribute__ ((constructor)) register_module(void);

static mmi_module *g_module;

void register_module(void) {
    g_module = mmi_module::register_module(DOMAIN, module_main);
}

int module_ret;

sem_t g_sem;

void btn_cb_pressed(void *component) {
    mmi_button *btn = (mmi_button *) component;

    g_module->delete_btn(btn);
    int i = g_module->get_btn_count();

    if(i == 0) {
        sem_post(&g_sem);
    }
}

void btn_cb_movein(void *component) {
    mmi_button *btn = (mmi_button *) component;

    g_module->delete_btn(btn);
    int i = g_module->get_btn_count();

    if(i == 0) {
        sem_post(&g_sem);
    }
}

void btn_cb_moveout(void *component) {
    mmi_button *btn = (mmi_button *) component;

    g_module->delete_btn(btn);
    int i = g_module->get_btn_count();

    if(i == 0) {
        sem_post(&g_sem);
    }
}

int module_main(mmi_module * mod) {
    // Check if we've entered here by mistake. Normally we shouldn't
    // be here.
    if (mod->get_run_mode() == TEST_MODE_PCBA) {
        return CASE_FAIL;
    }

    sem_init(&g_sem, 0, 0);

    mmi_window *window = new mmi_window();
    int w = window->get_width();
    int h = window->get_height();
    mmi_state *sinstance = mmi_state::get_instance();

    g_module->add_window(window);
    mmi_btn_cbs_t cbs = { btn_cb_pressed, btn_cb_movein, btn_cb_moveout };

    int row = 20;
    int col = 10;

    for(int i = 0; i < row; i += 2) {
        mmi_rect_t rect = { 0, i * h / row, w / col, h / row };
        mmi_button *btn = new mmi_button(rect, "", cbs);

        btn->set_color(0, 125, 125, 255);
        g_module->add_btn(btn);
    }

    for(int i = 0; i < row; i += 2) {
        mmi_rect_t rect = { w - w / col, i * h / row, w / col, h / row };
        mmi_button *btn = new mmi_button(rect, "", cbs);

        btn->set_color(0, 125, 125, 255);
        g_module->add_btn(btn);
    }

    for(int i = 0; i < col; i += 2) {
        mmi_rect_t rect = { i * w / col, 0, w / col, h / row };
        mmi_button *btn = new mmi_button(rect, "", cbs);

        btn->set_color(0, 125, 125, 255);
        g_module->add_btn(btn);
    }

    for(int i = 0; i < col; i += 2) {
        mmi_rect_t rect = { i * w / col, h - h / row, w / col, h / row };
        mmi_button *btn = new mmi_button(rect, "", cbs);

        btn->set_color(0, 125, 125, 255);
        g_module->add_btn(btn);
    }
    sinstance->set_trace_debug_enable(true);

    sem_wait(&g_sem);
    sem_close(&g_sem);
    sinstance->set_trace_debug_enable(false);

    g_module->clean_source();
    return 0;
}
