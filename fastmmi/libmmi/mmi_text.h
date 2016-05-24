/*
 * Copyright (c) 2013, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef __LIBMMI_TEXT__
#define __LIBMMI_TEXT__
#include "mmi_component.h"

class mmi_text {

  public:
    mmi_text(mmi_point_t point, const char *str);
      mmi_text(mmi_point_t point, const char *str, void *extra);

    char *get_text();
    void set_text(const char *str);
    mmi_point_t *get_point();
    void *get_extra();
    static int get_font_size_x();
    static int get_font_size_y();

  private:
      mmi_point_t m_point;
    char m_text[80];
    void *m_extra_1;
};

#endif
