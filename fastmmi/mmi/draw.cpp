/*
 * Copyright (c) 2013, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include <mmi_button.h>
#include <mmi_module_manage.h>
#include <mmi_text.h>
#include <mmi_window.h>

#include <list>
#include <unistd.h>
#include <fcntl.h>
#include "draw.h"
#include "mmi.h"
#include "mmi_utils.h"
#include "input.h"

extern "C" {
#include <minui.h>
}
static const char *TAG = "mmi_draw";
extern mmi_state *state_instance;
extern struct input_params mmi_input;
extern touch_event_t touch_event;

using namespace std;

#define DEFAULT_BOLD 1
void set_brightness(int brightness) {
    char buffer[80];
    int fd = -1;

    fd = open(mmi_input.sys_backlight, O_RDWR);
    if(fd < 0) {
        return;
    }
    snprintf(buffer, sizeof(buffer) - 1, "%d\n", brightness);
    buffer[sizeof(buffer) - 1] = '\0';
    write(fd, buffer, strlen(buffer));
    close(fd);

    return;
}

void clear_screen(void) {
    gr_color(0, 0, 0, 255);
    gr_fill(0, 0, gr_fb_width(), gr_fb_height());
}

void *draw_thread(void *args) {
    set_brightness(100);
    mmi_module *cur_module;
    int ret = 0;
    bool pf_png_exist = true;
    int text_len = 0, text_x_pos = 0;

    gr_surface passSurface, failSurface;

    ret = res_create_surface("mmi/pass", &passSurface);

    if(ret < 0) {
        ALOGE("Cannot load pass image\n");
        pf_png_exist = false;
    }
    ret = res_create_surface("mmi/fail", &failSurface);
    if(ret < 0) {
        ALOGE("Cannot load fail  image\n");
        pf_png_exist = false;
    }

    int passImageWidth = gr_get_width(passSurface);
    int passImageHeight = gr_get_height(passSurface);
    int failImageWidth = gr_get_width(failSurface);
    int failImageHeight = gr_get_width(failSurface);

    int fontWidth, fontHeight;

    gr_font_size(&fontWidth, &fontHeight);

    while(1) {
        clear_screen();

        cur_module = get_current_module();

        list < mmi_button * >*blist = cur_module->get_btn_list();
        list < mmi_button * >::iterator btn_iter;

        list < mmi_text * >*textlist = cur_module->get_text_list();
        list < mmi_text * >::iterator text_iter;

        list < mmi_window * >*windowslist = cur_module->get_window_list();
        list < mmi_window * >::iterator window_iter;

        list < mmi_item * >*itemlist = cur_module->get_item_list();
        list < mmi_item * >::iterator item_iter;

        list < touch_point_t > *trace_list = &touch_event.trace_debug;
        list < touch_point_t >::iterator trace_iter;

        cur_module->win_btn_text_list_lock();
        if(windowslist->begin() != windowslist->end()) {
            mmi_window *tmp = *windowslist->begin();

            tmp->window_lock();
            gr_blit(tmp->get_gr_memory_surface(), 0, 0, gr_fb_width(), gr_fb_height(), 0, 0);
            tmp->window_unlock();
        }

        for(btn_iter = blist->begin(); btn_iter != blist->end(); btn_iter++) {
            mmi_button *tmp = *btn_iter;
            mmi_rect_t *rect = tmp->get_rect();
            mmi_color_t *btn_color = tmp->get_color();

            gr_color(btn_color->r, btn_color->g, btn_color->b, btn_color->a);

            gr_fill(rect->x, rect->y, rect->x + rect->w, rect->y + rect->h);
            gr_color(255, 255, 255, 255);
            text_len = strlen(tmp->get_text()) * fontWidth;
            if(text_len > rect->w)
                text_x_pos = rect->x;
            else
                text_x_pos = rect->x + (rect->w - text_len) / 2;
            gr_text(text_x_pos, rect->y + rect->h / 2, tmp->get_text(), DEFAULT_BOLD);
        }

        for(text_iter = textlist->begin(); text_iter != textlist->end(); text_iter++) {
            mmi_text *tmp = *text_iter;
            mmi_point_t *point = tmp->get_point();

            gr_color(255, 255, 255, 255);
            gr_text(point->x, point->y, tmp->get_text(), DEFAULT_BOLD);
        }
        /*draw debug trace point */
        for(trace_iter = trace_list->begin(); trace_iter != trace_list->end(); trace_iter++) {
            gr_color(255, 0, 0, 255);
            gr_fill(trace_iter->x, trace_iter->y, trace_iter->x + 1, trace_iter->y + 1);
        }

        for(item_iter = itemlist->begin(); item_iter != itemlist->end(); item_iter++) {
            mmi_item *tmp = *item_iter;

            int x = 0;
            int y = 0;
            int w = mmi_item::getItemWidth();
            int h = mmi_item::getItemHeight();
            int index = tmp->get_index();

            y += index * h;

            gr_color(255, 250, 250, 100);   /* delimer */
            gr_fill(x, y, w, y + 1);

            gr_color(255, 130, 71, 100);
            gr_fill(x, y, x + w, y + h);
            gr_color(255, 250, 250, 255);
            gr_text(x + 20, y + h / 2, tmp->get_text(), DEFAULT_BOLD);

            gr_color(255, 250, 250, 100);   /* delimer */
            gr_fill(x, y + h, w, y + h + 1);

            if(pf_png_exist) {

                if(tmp->get_launch_module()->get_result() < 0)
                    gr_blit(failSurface, 0, 0, failImageWidth, failImageHeight,
                            w - failImageWidth, y + h / 2 - failImageHeight / 2);
                else if(tmp->get_launch_module()->get_result() == 0)
                    gr_blit(passSurface, 0, 0, passImageWidth, passImageHeight,
                            w - passImageWidth, y + h / 2 - failImageHeight / 2);
            }
        }

        cur_module->win_btn_text_list_unlock();
        gr_flip();

        usleep(1000 * mmi_input.refresh_interval);
    }

    return NULL;
}
