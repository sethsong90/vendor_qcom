/*
 * Copyright (c) 2013, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include "mmi_window.h"
#include <malloc.h>
extern "C" {
#include <minui.h>
}
#include <semaphore.h>
#include <cutils/log.h>

static sem_t g_win_sem;
mmi_window::mmi_window() {

}

mmi_window::~mmi_window() {
}

void mmi_window::color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {

}

void mmi_window::fill(int x, int y, int w, int h) {

}

void mmi_window::blit(gr_surface source, int sx, int sy, int w, int h, int dx, int dy) {

}

int mmi_window::get_width() {
    return gr_fb_width();
}

int mmi_window::get_height() {
    return gr_fb_height();
}

void mmi_window::window_lock() {

}

void mmi_window::window_unlock() {

}

int win_sem_init() {
    int ret = sem_init(&g_win_sem, 0, 0);

    if(ret != 0) {
        ALOGE("sem_init \n");
    }
    return ret;
}

int win_sem_wait() {
    return sem_wait(&g_win_sem);
}

int invalidate() {
    return sem_post(&g_win_sem);
}
