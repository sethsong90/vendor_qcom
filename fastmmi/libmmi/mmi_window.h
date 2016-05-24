/*
 * Copyright (c) 2013, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef __LIBMMI_WINDOW__
#define __LIBMMI_WINDOW__

#include <pixelflinger/pixelflinger.h>
#include <pthread.h>

extern "C" {
#include <minui.h>
};

class mmi_window {

  public:
    mmi_window();
    ~mmi_window();
    void color(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
    void fill(int x, int y, int w, int h);
    void blit(gr_surface source, int sx, int sy, int w, int h, int dx, int dy);
    int get_width();
    int get_height();
    GGLSurface *get_gr_memory_surface();
    void window_lock();
    void window_unlock();

  private:
      GGLSurface m_gr_mem_surface;
    GGLContext *m_gr_context;
    pthread_mutex_t m_window_mutex;
};

#endif
