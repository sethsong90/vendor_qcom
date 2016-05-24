/*
 * Copyright (c) 2013, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include "mmi_text.h"
#include <string.h>

extern "C" {
#include <minui.h>
};
mmi_text::mmi_text(mmi_point_t point, const char *str) {
    m_point = point;
    strlcpy(m_text, str, sizeof(m_text));
    m_extra_1 = NULL;
}

mmi_text::mmi_text(mmi_point_t point, const char *str, void *extra) {
    m_point = point;
    strlcpy(m_text, str, sizeof(m_text));
    m_extra_1 = extra;
}

mmi_point_t *mmi_text::get_point() {
    return &m_point;
}

char *mmi_text::get_text() {
    return m_text;
}

void *mmi_text::get_extra() {
    return m_extra_1;
}

void mmi_text::set_text(const char *str) {
    strlcpy(m_text, str, sizeof(m_text));
}

int mmi_text::get_font_size_x() {
    int x, y;

    gr_font_size(&x, &y);
    return x;
}

int mmi_text::get_font_size_y() {
    int x, y;

    gr_font_size(&x, &y);
    return y;
}
