/*
 * Copyright (c) 2013, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef __LIBMMI_BUTTON__
#define __LIBMMI_BUTTON__

#include <list>
#include "mmi_component.h"

class mmi_module;
using namespace std;

class mmi_button;

typedef struct {
    int x;
    int y;
    int w;
    int h;
} mmi_rect_t;

typedef struct {
    mmi_cb_t pressed;
    mmi_cb_t movein;
    mmi_cb_t moveout;

} mmi_btn_cbs_t;

class mmi_button {
  public:
    mmi_button(mmi_rect_t rect, const char *str, mmi_cb_t cb);
      mmi_button(mmi_rect_t rect, const char *str, mmi_btn_cbs_t cbs);
    char *get_text();
    void set_text(const char *str);
    mmi_rect_t *get_rect();
    void set_rect(mmi_rect_t * rect);
    void set_rect(int x, int y, int w, int h);
    mmi_cb_t get_cb_pressed();
    mmi_cb_t get_cb_movein();
    mmi_cb_t get_cb_moveout();

    mmi_module *get_launch_module();
    void set_launch_module(mmi_module * m);

    void set_color(char r, char g, char b, char a);
    void set_color(mmi_color_t * color);

    void color_set_default();
    mmi_color_t *get_color();
  private:
      mmi_rect_t m_rect;
    char m_text[MAX_DEFAULT_BUTTON_TEXT_LEN];
    mmi_module *m_launch_module;
    mmi_color_t m_btn_color;
    mmi_btn_cbs_t m_cbs;

};
#endif
