/*
 * Copyright (c) 2013, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef __LIBMMI_ITEM__
#define __LIBMMI_ITEM__

#include <list>
#include "mmi_component.h"
extern "C" {
#include <minui.h>
}

class mmi_module;
using namespace std;

class mmi_item;

#define DEFAULT_ITEM_WIDTH  (gr_fb_width()*4/5)
#define DEFAULT_ITEM_NUM_PER_PAGE 10
#define DEFAULT_ITEM_HEIGHT (gr_fb_height()/DEFAULT_ITEM_NUM_PER_PAGE)
typedef void (*mmi_item_cb_t) (mmi_item *);

typedef struct {
    mmi_cb_t pressed;
} mmi_item_cbs_t;

typedef struct {
    int x_min;
    int y_min;
    int x_max;
    int y_max;
} mmi_item_pos_t;

class mmi_item {
  public:

    mmi_item(int index, const char *str, mmi_item_cb_t cb);
      mmi_item(int index, const char *str, mmi_cb_t cb);

    char *get_text();
    void set_text(char *str);
    int get_index();
    void set_index(int index);
    mmi_cb_t get_cb_pressed();
    mmi_module *get_launch_module();
    void set_launch_module(mmi_module * m);
    static int get_item_num_in_page();
    static int getItemWidth();
    static int getItemHeight();
    mmi_item_pos_t *getPosition();
  private:
    int mItemIndex;
    char mText[MAX_DEFAULT_ITEM_TEXT_LEN];
    mmi_module *m_launch_module;
    mmi_item_cbs_t m_cbs;
    static int itemNumInPage;
    mmi_item_pos_t position;
};

#endif
