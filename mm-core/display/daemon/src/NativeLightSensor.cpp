/*
 * DESCRIPTION
 * This file contains the ambient light sensor functionalities read from
 * Android Native API for sensors
 *
 * Copyright (c) 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 *
 */

#include "NativeLightSensor.h"
#include <utils/Timers.h>

#define LOG_TAG "NativeLightSensor"
#define USEC_SEC 1000000
#define POLL_NO_TIMEOUT -1

int NativeLightSensor::ALSInfoQuery() {
    int minDelay;
    minDelay = ASensor_getMinDelay(mLightSensor);
    if (!minDelay) {
        LOGE("%s: Light Sensor does not have sample rate", __FUNCTION__);
        mMaxSampleRate = DEFAULT_SAMPLE_RATE;
        bSampleRateSet = false;
    } else {
        mMaxSampleRate = SELECT_SAMPLE_RATE(mMaxSampleRate);
        bSampleRateSet = true;
    }
    LOGE_IF(mDebug, "%s: MaxSampleRate = %d", __FUNCTION__, mMaxSampleRate);
    return 0;
}

int NativeLightSensor::ALSRegister() {
    pSensorManager = ASensorManager_getInstance();
    mLightSensor = ASensorManager_getDefaultSensor(pSensorManager,
        ASENSOR_TYPE_LIGHT);

    if(!mLightSensor) {
        LOGE("%s: Light Sensor Was not found on the device", __FUNCTION__);
        return -1;
    }
    LOGE_IF(mDebug, "%s: Light Sensor: %s, Vendor: %s", __FUNCTION__,
        ASensor_getName(mLightSensor), ASensor_getVendor(mLightSensor));

    ALSInfoQuery();
    return 0;
}

int NativeLightSensor::ALSDeRegister() {
    pthread_mutex_lock(&mCtrlLock);
    mSensorThreadStatus = SENSOR_THREAD_SUSPEND;
    pthread_mutex_unlock(&mCtrlLock);
    if (pLooper)
        ALooper_wake(pLooper);
    return 0;
}

int NativeLightSensor::ALSRun(bool isFirstRun) {
    int32_t err;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_mutex_lock(&mCtrlLock);
    mSensorThreadStatus = SENSOR_THREAD_ON;
    pthread_cond_signal(&mCtrlCond);
    pthread_mutex_unlock(&mCtrlLock);

    pthread_mutex_lock(&mALSLock);
    if (!bSensorLoopRunning){
        pthread_mutex_unlock(&mALSLock);
        LOGE_IF(mDebug, "%s: Trying to create a new thread", __FUNCTION__);
        err = pthread_create(&mALSSensorThread, &attr, als_sensor_read, this);
        if (err) {
            LOGE_IF(mDebug, "%s: Failed to start the control thread",
                __FUNCTION__);
            mSensorThreadStatus = SENSOR_THREAD_OFF;
        }
    } else {
        pthread_mutex_unlock(&mALSLock);
    }
    pthread_attr_destroy(&attr);
    return 0;
}

int NativeLightSensor::ALSCleanup() {
    pthread_mutex_lock(&mALSLock);
    if (bSensorLoopRunning ) {
        pthread_mutex_lock(&mCtrlLock);
        mSensorThreadStatus = SENSOR_THREAD_OFF;
        pthread_mutex_unlock(&mCtrlLock);
    }
    pthread_mutex_unlock(&mALSLock);
    if (pLooper)
        ALooper_wake(pLooper);
    pthread_join(mALSSensorThread, NULL);
    pSensorManager = NULL;
    return 0;
}

int NativeLightSensor::ALSReadSensor() {
    int temp_val = 0;
    pthread_mutex_lock(&mALSLock);
    if (bALooperHalt) {
        LOGE("%s: Looper encountered error or exit", __FUNCTION__);
    } else if(!bALSReady) {
        pthread_cond_wait(&mALSCond, &mALSLock);
    }
    bALSReady = false;
    temp_val = mALSValue;
    pthread_mutex_unlock(&mALSLock);
    return temp_val;
}

bool NativeLightSensor::CheckThreadStatus() {
    int status = 0;

    pthread_mutex_lock(&mCtrlLock);
    status = mSensorThreadStatus;
    pthread_mutex_unlock(&mCtrlLock);

    switch (status) {
    case SENSOR_THREAD_ON:
        return true;
    case SENSOR_THREAD_OFF:
        return false;
    case SENSOR_THREAD_SUSPEND:
        SensorControl(OFF);
        pthread_mutex_lock(&mCtrlLock);
        if (mSensorThreadStatus == SENSOR_THREAD_SUSPEND)
            pthread_cond_wait(&mCtrlCond, &mCtrlLock);
        pthread_mutex_unlock(&mCtrlLock);
        return true;
    default:
        LOGE("%s: Invalid thread status!", __FUNCTION__);
    }
    return false;
}

void NativeLightSensor::ProcessNativeLightSensorWork() {
    int status = 0;
    long long t1 = 0, t2 = 0, th = 0;

    pthread_mutex_lock(&mALSLock);
    if (bSensorLoopRunning ) {
        LOGE_IF(mDebug, "%s: Thread already running", __FUNCTION__);
        pthread_mutex_unlock(&mALSLock);
        return;
    }
    bSensorLoopRunning = true;
    pthread_mutex_unlock(&mALSLock);

    pLooper = ALooper_forThread();
    if(!pLooper) {
        LOGE_IF(mDebug, "%s: Preparing new Looper for thread", __FUNCTION__);
        pLooper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    }
    pSensorEventQueue = ASensorManager_createEventQueue(pSensorManager,
        pLooper, ALOOPER_EVENT_INPUT , NULL, NULL);
    SensorControl(ON);

    while(CheckThreadStatus()) {
        SensorControl(ON);
        th = ((long long) (USEC_SEC * 1000)) / mMaxSampleRate;

        mIdent = ALooper_pollOnce( POLL_NO_TIMEOUT, NULL, NULL, NULL);

        if (!CheckThreadStatus())
            break;

        switch (mIdent) {

            case ALOOPER_POLL_WAKE :
                LOGE_IF(mDebug, "%s: Interrupt: Poll awoken before timeout",
                    __FUNCTION__);
                SetLooperHalt(true);
                break;

            case ALOOPER_POLL_CALLBACK :
                LOGE_IF(mDebug, "%s: Callback: Callbacks invoked",
                    __FUNCTION__);
                SetLooperHalt(false);
                break;

            case ALOOPER_POLL_TIMEOUT :
                LOGE_IF(mDebug, "%s: Timeout : No data available",
                    __FUNCTION__);
                SetLooperHalt(false);
                break;

            case ALOOPER_POLL_ERROR :
                LOGE("%s: Error: occured while polling", __FUNCTION__);
                SetLooperHalt(true);
                goto thread_exit;

            default:
                SetLooperHalt(false);
                while (ASensorEventQueue_getEvents(pSensorEventQueue,
                    &mEvent, 1) > 0) {

                    pthread_mutex_lock(&mALSLock);
                    mALSValue = (int) (mEvent.light + 0.5);
                    LOGE_IF(mDebug, "%s: ALS Value (NATIVE) = %d", __FUNCTION__,
                        mALSValue);
                    bALSReady = true;
                    pthread_cond_signal(&mALSCond);
                    pthread_mutex_unlock(&mALSLock);

                    if (!bSampleRateSet) {
                        t2 = systemTime();
                        if ( t2 - t1 < th) {
                            if (!CheckThreadStatus())
                                break;
                            usleep((useconds_t)
                                (USEC_SEC / (2 * mMaxSampleRate)));
                            continue;
                        }
                        t1 = t2;
                    }
                }
                LOGE_IF(mDebug, "%s: No more events present", __FUNCTION__);
       }
    }

thread_exit:
    LOGE_IF(mDebug, "%s: Exiting the thread", __FUNCTION__);
    SensorControl(OFF);
    ASensorManager_destroyEventQueue(pSensorManager, pSensorEventQueue);

    pthread_mutex_lock(&mALSLock);
    bSensorLoopRunning = false;
    pthread_mutex_unlock(&mALSLock);

    pLooper = NULL;

    pthread_exit(NULL);
    return;

}

void NativeLightSensor::SensorControl(bool b) {
    if ((bSensorStatus == OFF) && (b)) {
        ASensorEventQueue_enableSensor(pSensorEventQueue, mLightSensor);
        ASensorEventQueue_setEventRate( pSensorEventQueue,
            mLightSensor, (int32_t)(USEC_SEC/mMaxSampleRate));
        bSensorStatus = ON;
    } else if ((bSensorStatus == ON) && (!b)) {
        ASensorEventQueue_disableSensor(pSensorEventQueue, mLightSensor);
        bSensorStatus = OFF;
    }
}

void NativeLightSensor::SetLooperHalt(bool b) {
    pthread_mutex_lock(&mALSLock);
    bALooperHalt = b;
    if (b)
        pthread_cond_signal(&mALSCond);
    pthread_mutex_unlock(&mALSLock);
}
