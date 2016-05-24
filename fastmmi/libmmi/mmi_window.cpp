/*
 * Copyright (c) 2013, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include "mmi_window.h"
#include <malloc.h>

mmi_window::mmi_window() {
    gglInit(&m_gr_context);
    gr_get_memory_surface(&m_gr_mem_surface);

    GGLContext *gl = m_gr_context;

    gl->colorBuffer(gl, &m_gr_mem_surface);
    gl->activeTexture(gl, 0);
    gl->enable(gl, GGL_BLEND);
    gl->blendFunc(gl, GGL_SRC_ALPHA, GGL_ONE_MINUS_SRC_ALPHA);

    pthread_mutex_init(&m_window_mutex, NULL);
}

mmi_window::~mmi_window() {
    free(m_gr_mem_surface.data);
    pthread_mutex_destroy(&m_window_mutex);
}

void mmi_window::color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    GGLContext *gl = m_gr_context;

    GGLint color[4];

    color[0] = ((r << 8) | r) + 1;
    color[1] = ((g << 8) | g) + 1;
    color[2] = ((b << 8) | b) + 1;
    color[3] = ((a << 8) | a) + 1;
    gl->color4xv(gl, color);
}

void mmi_window::fill(int x, int y, int w, int h) {
    GGLContext *gl = m_gr_context;

    gl->disable(gl, GGL_TEXTURE_2D);
    gl->recti(gl, x, y, w, h);
}

void mmi_window::blit(gr_surface source, int sx, int sy, int w, int h, int dx, int dy) {
    GGLContext *gl = m_gr_context;

    gl->bindTexture(gl, (GGLSurface *) source);
    gl->texEnvi(gl, GGL_TEXTURE_ENV, GGL_TEXTURE_ENV_MODE, GGL_REPLACE);
    gl->texGeni(gl, GGL_S, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->texGeni(gl, GGL_T, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->enable(gl, GGL_TEXTURE_2D);
    gl->texCoord2i(gl, sx - dx, sy - dy);
    gl->recti(gl, dx, dy, dx + w, dy + h);
}

int mmi_window::get_width() {
    return m_gr_mem_surface.width;
}

int mmi_window::get_height() {
    return m_gr_mem_surface.height;
}

GGLSurface *mmi_window::get_gr_memory_surface() {
    return &m_gr_mem_surface;
}

void mmi_window::window_lock() {
    pthread_mutex_lock(&m_window_mutex);
}

void mmi_window::window_unlock() {
    pthread_mutex_unlock(&m_window_mutex);
}
