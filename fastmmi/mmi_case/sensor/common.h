/*
 * Copyright (c) 2012, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
#ifndef _COMMON_H
#define _COMMON_H

#include <string>
#include <mmi_module_manage.h>

#define LOG_TAG   "MMI_SENSOR"
#include <cutils/log.h>

int readFile(const char *path, char *content, size_t count);
int writeFile(const char *path, const char *content);

struct input_event {
    struct timeval time;
    __u16 type;
    __u16 code;
    __s32 value;
};

int getInputName(const char *searchPath, const char *targetSensor, char *inputName);
int initSensor(int senNum, mmi_module * mod);
int deinitSensor(int senNum);
int testThisSensor(int sensor_type);
#endif
