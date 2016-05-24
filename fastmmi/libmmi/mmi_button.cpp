/*
 * Copyright (c) 2013, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include "mmi_button.h"
#include <string.h>
mmi_button::mmi_button(mmi_rect_t rect, const char *str, mmi_cb_t cb) {
    m_rect = rect;
    strlcpy(m_text, str, sizeof(m_text));
    m_cbs.pressed = cb;
    m_cbs.movein = NULL;
    m_cbs.moveout = NULL;

    m_launch_module = NULL;
    set_color(0xcd, 0x66, 0, 100);

}

mmi_button::mmi_button(mmi_rect_t rect, const char *str, mmi_btn_cbs_t cbs) {
    m_rect = rect;
    strlcpy(m_text, str, sizeof(m_text));
    m_cbs = cbs;
    m_launch_module = NULL;
    set_color(0, 0, 0, 0);
}

char *mmi_button::get_text() {
    return m_text;
}

mmi_rect_t *mmi_button::get_rect() {
    return &m_rect;
}

mmi_cb_t mmi_button::get_cb_pressed() {
    return m_cbs.pressed;
}

mmi_module *mmi_button::get_launch_module() {
    return m_launch_module;
}

void mmi_button::set_color(char r, char g, char b, char a) {
    m_btn_color.r = r;
    m_btn_color.g = g;
    m_btn_color.b = b;
    m_btn_color.a = a;
}

void mmi_button::set_color(mmi_color_t * color) {
    m_btn_color.r = color->a;
    m_btn_color.g = color->g;
    m_btn_color.b = color->b;
    m_btn_color.a = color->a;
}

mmi_color_t *mmi_button::get_color() {
    return &m_btn_color;
}

void mmi_button::color_set_default() {
    memset(&m_btn_color, 0, sizeof(m_btn_color));
}

void mmi_button::set_rect(mmi_rect_t * rect) {
    m_rect = *rect;
}
void mmi_button::set_rect(int x, int y, int w, int h) {
    m_rect.x = x;
    m_rect.y = y;
    m_rect.w = w;
    m_rect.h = h;
}
void mmi_button::set_launch_module(mmi_module * m) {
    m_launch_module = m;
}

mmi_cb_t mmi_button::get_cb_movein() {
    return m_cbs.movein;
}

mmi_cb_t mmi_button::get_cb_moveout() {
    return m_cbs.moveout;
}

void mmi_button::set_text(const char *str) {
    strlcpy(m_text, str, sizeof(m_text));
}
