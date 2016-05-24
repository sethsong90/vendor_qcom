/*
* Copyright (c) 2013-2014 Qualcomm Technologies, Inc. All Rights Reserved.
* Qualcomm Technologies Confidential and Proprietary.
*
* Not a Contribution.
* Apache license notifications and license are retained
* for attribution purposes only.
*/

 /*
  * Copyright (C) 2008 The Android Open Source Project
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *      http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */

#include "common.h"
#include <string>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdint.h>
#include <sys/cdefs.h>
#include <utils/Timers.h>
#include "hardware/sensors.h"
#include "mmi_utils.h"
static const char *TAG = "sensor";

struct sensors_poll_device_t *device = NULL;
struct sensors_module_t *module = NULL;
struct sensor_t const *mList = NULL;
mmi_module *mModule = NULL;
bool isPCBA = false;
static const size_t numEvents = 16;
int devCount = 0;
sensors_event_t buffer[numEvents];

char const *getSensorName(int type) {
    switch (type) {
    case SENSOR_TYPE_ACCELEROMETER:
        return "Acc";
    case SENSOR_TYPE_MAGNETIC_FIELD:
        return "Mag";
    case SENSOR_TYPE_ORIENTATION:
        return "Ori";
    case SENSOR_TYPE_GYROSCOPE:
        return "Gyr";
    case SENSOR_TYPE_LIGHT:
        return "Lux";
    case SENSOR_TYPE_PRESSURE:
        return "Bar";
    case SENSOR_TYPE_TEMPERATURE:
        return "Tmp";
    case SENSOR_TYPE_PROXIMITY:
        return "Prx";
    case SENSOR_TYPE_GRAVITY:
        return "Grv";
    case SENSOR_TYPE_LINEAR_ACCELERATION:
        return "Lac";
    case SENSOR_TYPE_ROTATION_VECTOR:
        return "Rot";
    case SENSOR_TYPE_RELATIVE_HUMIDITY:
        return "Hum";
    case SENSOR_TYPE_AMBIENT_TEMPERATURE:
        return "Tam";
    }
    return "ukn";
}

void drawThisText(mmi_module * mModule, char *mText, int startX, int startY, bool clean) {
    mmi_window *window = new mmi_window();
    int width = window->get_width();
    int height = window->get_height();
    mmi_text *text;
    mmi_point_t point = { startX * width / 20, startY * height / 20 };

    text = new mmi_text(point, mText);
    if(clean)
        mModule->clean_text();
    mModule->add_text(text);
}

int initSensor(int senNum, mmi_module * mod) {
    int err;
    static int sensor_test_initialized;

    if (!sensor_test_initialized) {
             mModule = mod;
             err = hw_get_module(SENSORS_HARDWARE_MODULE_ID, (hw_module_t const **) &module);
             if(err != 0) {
                  ALOGE("hw_get_module() failed (%s)\n", strerror(-err));
                  return -1;
             }

             err = sensors_open(&module->common, &device);
             if(err != 0) {
                 ALOGE("sensors_open() failed (%s)\n", strerror(-err));
                 return -1;
             }
             sensor_test_initialized = 1;
    }
    if (!mModule) {
             ALOGE("Sensor test not correctly initialized");
             return -1;
    }
    if (mModule->get_run_mode() == TEST_MODE_PCBA) {
        isPCBA = true;
    }
    devCount = module->get_sensors_list(module, &mList);
    if((senNum > devCount) || (senNum < 0)) {
        goto error;
    }
    for(int i = 0; i < devCount; i++) {
        if(mList[i].type == senNum) {
            ALOGI("Deactivating sensor %d", senNum);
            err = device->activate(device, mList[i].handle, 0);
            if(err != 0) {
                ALOGE("deactivate() for '%s'failed (%s)\n", mList[i].name, strerror(-err));
                goto error;
            }
            ALOGI("Activating sensor %d", senNum);
            err = device->activate(device, mList[i].handle, 1);
            if(err != 0) {
                ALOGE("activate() for '%s'failed (%s)\n", mList[i].name, strerror(-err));
                goto error;
            }
            device->setDelay(device, mList[i].handle, ms2ns(500));
            break;
        }
    }
    return 0;
  error:
    ALOGE("InitSensor failed");
    return -1;
}

int deinitSensor(int senNum) {
    int err = -1;

    if((device == NULL) || (module == NULL) || (mList == NULL)) {
            ALOGE("DeInitSensor NULL check failed\n");
            return -1;
    }

    if(senNum > module->get_sensors_list(module, &mList) || (senNum < 0))
    {
            ALOGE("Invalid sensor number %d passed to deinitSensor",senNum);
            return -1;
    }
    for(int i = 0; i < devCount; i++) {
        if(mList[i].type == senNum) {
            ALOGI("Deactivating sensor %d",senNum);
            err = device->activate(device, mList[senNum].handle, 0);
            if(err != 0) {
                ALOGE("deactivate() for '%s'failed (%s)\n", mList[i].name, strerror(-err));
                break;
            }
        }
    }
    return err;
}

int testThisSensor(int sensor_type) {
    int err = -1;
    char sensorDataToPrint[256];

    if((device == NULL) || (module == NULL) || (mModule == NULL))
        return -1;

    int n = device->poll(device, buffer, numEvents);

    if(n < 0) {
        ALOGE("poll() failed (%s)\n", strerror(-err));
    }

    for(int i = 0; i < n; i++) {
            const sensors_event_t & data = buffer[i];
            if (sensor_type == data.type) {
                switch (data.type) {
                case SENSOR_TYPE_ACCELEROMETER:
                case SENSOR_TYPE_MAGNETIC_FIELD:
                case SENSOR_TYPE_ORIENTATION:
                case SENSOR_TYPE_GYROSCOPE:
                case SENSOR_TYPE_GRAVITY:
                case SENSOR_TYPE_LINEAR_ACCELERATION:
                case SENSOR_TYPE_ROTATION_VECTOR:
                    // Skip drawing when in PCBA mode. Probably display is
                    // missing or nobody will look there
                    if (!isPCBA) {
                        snprintf(sensorDataToPrint,
                                        sizeof(sensorDataToPrint),
                                        "value=<%5.1f,%5.1f,%5.1f>\n",
                                        data.data[0],
                                        data.data[1], data.data[2]);
                        drawThisText(mModule, (char *) getSensorName(data.type),
                                        4, 4, true);
                        drawThisText(mModule, sensorDataToPrint, 4, 5, false);
                    } else {
                        mModule->addFloatResult("X", data.data[0]);
                        mModule->addFloatResult("Y", data.data[1]);
                        mModule->addFloatResult("Z", data.data[2]);

                    }
                    err = 0;
                break;
                case SENSOR_TYPE_LIGHT:
                case SENSOR_TYPE_PRESSURE:
                case SENSOR_TYPE_TEMPERATURE:
                case SENSOR_TYPE_PROXIMITY:
                case SENSOR_TYPE_RELATIVE_HUMIDITY:
                case SENSOR_TYPE_AMBIENT_TEMPERATURE:
                    // Skip drawing when in PCBA mode. Probably display is
                    // missing or nobody will look there
                    if (!isPCBA) {
                        snprintf(sensorDataToPrint, sizeof(sensorDataToPrint),
                                        "value=%f\n", data.data[0]);
                        drawThisText(mModule, (char *) getSensorName(data.type),
                                         4, 4, true);
                        drawThisText(mModule, sensorDataToPrint, 4, 5, false);
                    } else {
                        mModule->addFloatResult("Value", data.data[0]);

                    }
                    err = 0;
                break;
                default:
                    ALOGE("Data received, but sensor is unknown... returning");
                break;
             }
        }
    }
    return err;
}
