/*
 * Copyright (c) 2013, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include "mmi_item.h"
#include <string.h>

int mmi_item::itemNumInPage = DEFAULT_ITEM_NUM_PER_PAGE;

mmi_item::mmi_item(int i, const char *str, mmi_cb_t cb) {
    strlcpy(mText, str, sizeof(mText));
    m_cbs.pressed = cb;
    m_launch_module = NULL;
    mItemIndex = i;
    position.x_min = 0;
    position.y_min = getItemHeight() * i;
    position.x_max = getItemWidth();
    position.y_max = getItemHeight() * (i + 1);
}

int mmi_item::get_index() {
    return mItemIndex;
}
int mmi_item::get_item_num_in_page() {
    return itemNumInPage;
}
int mmi_item::getItemWidth() {
    return DEFAULT_ITEM_WIDTH;
}

int mmi_item::getItemHeight() {
    return DEFAULT_ITEM_HEIGHT;
}

mmi_item_pos_t *mmi_item::getPosition() {
    position.x_min = 0;
    position.y_min = getItemHeight() * mItemIndex;
    position.x_max = getItemWidth();
    position.y_max = getItemHeight() * (mItemIndex + 1);
    return &position;
}

void mmi_item::set_index(int i) {
    mItemIndex = i;
}

char *mmi_item::get_text() {
    return mText;
}

mmi_cb_t mmi_item::get_cb_pressed() {
    return m_cbs.pressed;
}

mmi_module *mmi_item::get_launch_module() {
    return m_launch_module;
}

void mmi_item::set_launch_module(mmi_module * m) {
    m_launch_module = m;
}

void mmi_item::set_text(char *str) {
    strlcpy(mText, str, sizeof(mText));
}
