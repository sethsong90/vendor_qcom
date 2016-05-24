/*
 * Copyright (c) 2013, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef __SYSTEM_CORE_MMI_INPUT__
#define __SYSTEM_CORE_MMI_INPUT__

#define TIME_KEY_LONG_PRESS       (MSEC_PER_SEC / 2)

#define MSEC_PER_SEC            (1000LL)
#define NSEC_PER_MSEC           (1000000LL)
#define USEC_PER_MSEC           (1000LL)

#define PRESS_SHAKE_X 10
#define PRESS_SHAKE_Y 10

struct key_state {
    bool pending;
    bool down;
    int64_t timestamp;
};

typedef struct {
    int x;
    int y;
} touch_point_t;

typedef struct {
    list < touch_point_t > trace;
    list < touch_point_t > trace_debug;
    int is_sync_lastevent;
} touch_event_t;

typedef struct {
    int x_max;
    int x_min;
    int y_max;
    int y_min;
} touch_xy_min_max_t;

void *input_thread(void *);
#endif
