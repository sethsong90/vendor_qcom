/*
 * Copyright (c) 2014, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef __LIBMMI_GRAPHICS_H__
#define __LIBMMI_GRAPHICS_H__

#define DEFAULT_FONT_PATH "/system/etc/mmi/fonts.ttf"
#define DEFAULT_UNICODE_STR_LEN 256
void mmi_gr_deinit(void);
int mmi_gr_init(void);
void mmi_gr_text(int x, int y, const char *s, int bold);
void mmi_set_font_size(int w, int h);
#endif
