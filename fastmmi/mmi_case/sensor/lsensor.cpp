/*
 * Copyright (c) 2012-2014, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include <stdio.h>
#include <unistd.h>
#include <mmi_window.h>
#include <mmi_module_manage.h>
#include <semaphore.h>
#include "mmi_utils.h"
#include "mmi_config.h"
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include "common.h"
#include "hardware/sensors.h"

#define SENSOR_TIMEOUT 5
#define SENSOR_RETRY_COUNT 20

static const char *TAG = "LSensor";
static const int SENSOR = SENSOR_TYPE_LIGHT;
static mmi_module *g_module;
static int module_ret;
static pthread_t processThreadPid;
static sem_t g_sem;

int module_main(mmi_module * mod);
void pass(void *);
void fail(void *);

// extern "C", because we use this in dynamic linking in cpp code, and dlsym()
// isn't able to obtain the address of a symbol "register_module".
extern "C" void __attribute__ ((constructor)) register_module(void);

void register_module(void) {
    g_module = mmi_module::register_module(TAG, module_main);
}

void signalHandler(int signal) {
    pthread_exit(NULL);
}

void *processThread(void *) {
    int ret;

    signal(SIGUSR1, signalHandler);

    mmi_point_t point = { gr_fb_width() / 4, gr_fb_height() / 3 };
    mmi_text *text = new mmi_text(point, "Initializing...");

    g_module->add_text(text);

    ret = initSensor(SENSOR, g_module);
    if(ret)
        fail(NULL);
    /*Tell user we are waiting for valid data*/
    text->set_text("Waiting for sensor...");

    while(1) {
        testThisSensor(SENSOR);
    }

    return NULL;
}

// 1. Deactivating sensors is not functional ( bug in sensors hal probably ).
// 2. There are events from different sensors in the queue which we get when
// polling. This is caused by [1], when two or more sensors are enabled.
// Frequently testThisSensor() returns an error ( poll returned events, but
// those events are not produced by selected sensor ). That's why we are
// looping here for several times, and wait for our event.
void *processThreadAuto(void *) {
    signal(SIGUSR1, signalHandler);
    int err = -1;
    for (int i=0; i<SENSOR_RETRY_COUNT && err!=0; i++) {
         err = testThisSensor(SENSOR);
    }
    module_ret = err;
    sem_post(&g_sem);
    return NULL;
}

void pass(void *) {
    module_ret = 0;
    pthread_kill(processThreadPid, SIGUSR1);
}

void fail(void *) {
    module_ret = -1;
    pthread_kill(processThreadPid, SIGUSR1);
}

void initUI() {
    g_module->add_btn_pass(pass);
    g_module->add_btn_fail(fail);
}

void finish() {
    deinitSensor(SENSOR);
    g_module->clean_source();
}

int auto_test() {
    int res = 0;
    struct timespec ts;

    res = initSensor(SENSOR, g_module);
    if (res) {
        return PCBA_SENSOR_READ_FAIL;
    }
    sem_init(&g_sem, 0, 0);
    res = pthread_create(&processThreadPid, NULL, processThreadAuto, NULL);
    if(res < 0) {
        ALOGE("can't create pthread: %s\n", strerror(errno));
        return PCBA_SENSOR_READ_FAIL;
    }
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        goto failed;
    }
    ts.tv_sec += SENSOR_TIMEOUT;
    if (sem_timedwait(&g_sem, &ts)) {
        goto failed;
    }
    res = deinitSensor(SENSOR);
    if (res) {
        return PCBA_SENSOR_READ_FAIL;
    }
    // Return success only if we've received a callback from the sensor
    return CASE_SUCCESS;

failed:
    pthread_kill(processThreadPid, SIGUSR1);
    deinitSensor(SENSOR);
    return PCBA_SENSOR_READ_FAIL;
}

int module_main(mmi_module * mod) {
    int res = CASE_SUCCESS;

    g_module = mod;
    module_ret = -1;
    // PCBA tests first. No init needed here.
    if (g_module->get_run_mode() == TEST_MODE_PCBA) {
        return auto_test();
    }
    initUI();
    res = pthread_create(&processThreadPid, NULL, processThread, NULL);
    if(res < 0)
        ALOGE("can't create pthread: %s\n", strerror(errno));
    else
        pthread_join(processThreadPid, NULL);
    finish();

    return module_ret;
}
