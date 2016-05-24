/*
 * Copyright (c) 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef _NATIVELIGHTSENSOR_H
#define _NATIVELIGHTSENSOR_H

#include <jni.h>
#include <android/sensor.h>

#include "als.h"

#define ON true
#define OFF false

enum {
    SENSOR_THREAD_OFF = 0,
    SENSOR_THREAD_ON,
    SENSOR_THREAD_SUSPEND,
};

class NativeLightSensor: public ALS {
    ALooper* pLooper;
    ASensorEvent mEvent;
    ASensorEventQueue* pSensorEventQueue;
    ASensorManager* pSensorManager;
    ASensorRef mLightSensor;
    bool bALooperHalt;
    bool bALSReady;
    bool bSampleRateSet;
    bool bSensorLoopRunning;
    bool bSensorStatus;
    bool CheckThreadStatus();
    int mALSValue;
    int mIdent;
    int mMaxSampleRate;
    int mSensorThreadStatus;
    pthread_cond_t mALSCond;
    pthread_cond_t mCtrlCond;
    pthread_mutex_t mALSLock;
    pthread_mutex_t mCtrlLock;
    pthread_t mALSSensorThread;
    static void *als_sensor_read(void *obj) {
        reinterpret_cast<NativeLightSensor *>(obj)->
            ProcessNativeLightSensorWork();
        return NULL;
    }
    void ProcessNativeLightSensorWork();
    void SensorControl(bool b);
    void SetLooperHalt(bool b);

public:

    NativeLightSensor() : pLooper(NULL), pSensorEventQueue(NULL),
        pSensorManager(NULL), mLightSensor(NULL), bALooperHalt(false),
        bALSReady(false), bSampleRateSet(false), bSensorLoopRunning(false),
        bSensorStatus(OFF), mALSValue(0), mIdent(0), mMaxSampleRate(0),
        mSensorThreadStatus(SENSOR_THREAD_OFF) {

        pthread_cond_init(&mALSCond, NULL);
        pthread_cond_init(&mCtrlCond, NULL);
        pthread_mutex_init(&mALSLock, NULL);
        pthread_mutex_init(&mCtrlLock, NULL);

    }

    ~NativeLightSensor() {
        pthread_cond_destroy(&mALSCond);
        pthread_cond_destroy(&mCtrlCond);
        pthread_mutex_destroy(&mALSLock);
        pthread_mutex_destroy(&mCtrlLock);
    }

    int ALSInfoQuery();
    int ALSRegister();
    int ALSDeRegister();
    int ALSRun(bool isFirstRun);
    int ALSCleanup();
    int ALSReadSensor();
};

#endif
