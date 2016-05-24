/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
  Copyright (c) 2013-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

  Copyright (c) 2012-2014 Qualcomm Atheros, Inc.
  All Rights Reserved.
  Qualcomm Atheros Confidential and Proprietary.
=============================================================================*/

#define LOG_TAG "Wiper_jni"
#define LOG_NDEBUG 0

#define LAT_SCALE_FACTOR 23860929.4222
#define LONG_SCALE_FACTOR 11930464.7111
#define LAT_SCALE_UP(x) ((double)(x * LAT_SCALE_FACTOR))
#define LONG_SCALE_UP(x) ((double)(x * LONG_SCALE_FACTOR))

#define WIPER_FEATURE_ENABLE_MASK 0x1
#define SUPL_WIFI_FEATURE_ENABLE_MASK 0x4

#ifndef IZAT_CONF_FILE
#define IZAT_CONF_FILE "/etc/izat.conf"
#endif

#include "JNIHelp.h"
#include "jni.h"
#include "loc_cfg.h"
#include "LBSAdapter.h"
#include "android_runtime/AndroidRuntime.h"

using namespace android;

static LBSAdapter* nLBSAdapter = NULL;
static jobject jWiper = NULL;

static jmethodID method_wifiRequestEvent;
static jmethodID method_wifiApDataRequestEvent;

static int listenerMode;
static uint8_t wifi_wait_timeout_select = 0;

static loc_param_s_type wiper_parameter_table[] =
{
  {"WIFI_WAIT_TIMEOUT_SELECT",&wifi_wait_timeout_select, NULL,'n'},
};

static void wifiRequestEventCb(int requestType) {
    JNIEnv* env = AndroidRuntime::getJNIEnv();
    ALOGD ("wifiRequestEventCb invoked");
    env->CallVoidMethod(jWiper, method_wifiRequestEvent, requestType);
    if (env->ExceptionCheck()) {
        ALOGE("Exception in %s", __FUNCTION__);
        env->ExceptionClear();
    }
}

static void wifiApDataRequestEventCb() {
    JNIEnv* env = AndroidRuntime::getJNIEnv();
    ALOGD ("wifiDataRequestEventCb invoked");
    env->CallVoidMethod(jWiper, method_wifiApDataRequestEvent);
    if (env->ExceptionCheck()) {
        ALOGE("Exception in %s", __FUNCTION__);
        env->ExceptionClear();
    }
}

void WiperSsrInform::proc() const {
    if (nLBSAdapter){
        ALOGD("calling  nLBSAdapter->handleSSR()\n");
        if(listenerMode & WIPER_FEATURE_ENABLE_MASK == WIPER_FEATURE_ENABLE_MASK)
            nLBSAdapter->wifiStatusInform();
    }
}

void WiperRequest::proc() const {
    wifiRequestEventCb(mType);
}

void WiperApDataRequest::proc() const {
    wifiApDataRequestEventCb();
}

static void classInit(JNIEnv* env, jclass clazz) {
    method_wifiRequestEvent = env->GetMethodID(clazz, "wifiRequestEvent", "(I)V");
    method_wifiApDataRequestEvent = env->GetMethodID(clazz, "wifiApDataRequestEvent", "()V");
}

static void instanceInit(JNIEnv *env, jobject obj, jint listener_mode) {
    if (NULL == jWiper) {
        jWiper = env->NewGlobalRef(obj);
    }
    if (NULL == nLBSAdapter) {
        bool isSuplWifiEnabled = false;
        listenerMode = listener_mode;
        LOC_API_ADAPTER_EVENT_MASK_T mask = LOC_API_ADAPTER_BIT_REQUEST_WIFI;

        /*Check if SUPL WIFI is enabled as well as whether the timeout value is greater than 0*/
        if(listenerMode & SUPL_WIFI_FEATURE_ENABLE_MASK == SUPL_WIFI_FEATURE_ENABLE_MASK) {
            UTIL_READ_CONF(IZAT_CONF_FILE, wiper_parameter_table);
            if(wifi_wait_timeout_select > 0) {
                isSuplWifiEnabled = true;
                ALOGD("Subscribe for supl wifi mask.\n");
                mask = mask | LOC_API_ADAPTER_BIT_REQUEST_WIFI_AP_DATA;
            }
        }

        nLBSAdapter = LBSAdapter::get(mask,
                                      (loc_core::MsgTask::tCreate)
                                      AndroidRuntime::createJavaThread,
                                      NULL);

        if(listenerMode & WIPER_FEATURE_ENABLE_MASK == WIPER_FEATURE_ENABLE_MASK)
        {
            ALOGD("Inform wiper status.\n");
            nLBSAdapter->wifiStatusInform();
        }

        if(isSuplWifiEnabled)
        {
            ALOGD("Inform wifi timout value for supl.\n");
            nLBSAdapter->setWifiWaitTimeoutValue(wifi_wait_timeout_select);
        }

    }
}

static void sendNetworkLocation(JNIEnv* env, jobject obj, jint position_valid,
                                jdouble latitude, jdouble longitude, jfloat accuracy,
                                jint apinfo_valid, jbyteArray mac_array,
                                jintArray rssi_array, jintArray channel_array,
                                jint num_aps_used, jint ap_len)
{
    ALOGD("Send Network Location.\n");

    jint * rssi_arr = 0;
    jint * channel_arr = 0;
    jbyte * mac_arr = 0;
    WifiLocation wifiInfo;

    rssi_arr = env->GetIntArrayElements(rssi_array, 0);
    channel_arr = env->GetIntArrayElements(channel_array, 0);
    mac_arr = env->GetByteArrayElements(mac_array, 0);

    wifiInfo.positionValid = position_valid;
    wifiInfo.latitude = LAT_SCALE_UP(latitude);
    wifiInfo.longitude = LONG_SCALE_UP(longitude);
    wifiInfo.accuracy = accuracy;
    wifiInfo.numApsUsed = num_aps_used;
    wifiInfo.fixError = 0;

    if(apinfo_valid) {
        if(rssi_arr != NULL && channel_arr != NULL && mac_arr != NULL) {
            wifiInfo.apInfoValid = 1;
            wifiInfo.apInfo.apLen = ap_len;

            for(int i=0;i<MAX_REPORTED_APS;i++) {
                wifiInfo.apInfo.rssi[i] = rssi_arr[i];
                wifiInfo.apInfo.channel[i] = channel_arr[i];
            }
            for(int j=0;j<(MAC_ADDRESS_LENGTH * MAX_REPORTED_APS);j++){
                wifiInfo.apInfo.mac_address[j] = mac_arr[j];
            }
        }
    }
    if (nLBSAdapter) {
        nLBSAdapter->injectWifiPosition(wifiInfo);
    }
    if(NULL != rssi_arr) {
        env->ReleaseIntArrayElements(rssi_array, rssi_arr, 0);
    }
    if(NULL != channel_arr) {
        env->ReleaseIntArrayElements(channel_array, channel_arr, 0);
    }
    if(NULL != mac_arr) {
        env->ReleaseByteArrayElements(mac_array, mac_arr, 0);
    }
}

static void sendPassiveLocation(JNIEnv* env, jobject obj, jint position_valid,
                                jdouble latitude, jdouble longitude, jfloat accuracy)
{
    ALOGD("Send Passive Location.\n");

    jint * rssi_arr = 0;
    jint * channel_arr = 0;
    jbyte * mac_arr = 0;
    CoarsePositionInfo cpInfo;
    memset(&cpInfo, 0, sizeof(cpInfo));

    cpInfo.latitudeValid = position_valid;
    cpInfo.latitude = latitude;
    cpInfo.longitudeValid = position_valid;
    cpInfo.longitude = longitude;
    cpInfo.horUncCircularValid = position_valid;
    cpInfo.horUncCircular = accuracy;

    if (nLBSAdapter) {
        nLBSAdapter->injectCoarsePosition(cpInfo);
    }
}

static void sendWifiApInfo(JNIEnv* env, jobject obj,
                           jbyteArray mac_array, jintArray rssi_array,
                           jintArray channel_array, jint ap_len)
{
    ALOGD("Send Wifi Ap info\n");

    jint * rssi_arr = 0;
    jint * channel_arr = 0;
    jbyte * mac_arr = 0;
    WifiApInfo wifiApInfo;

    rssi_arr = env->GetIntArrayElements(rssi_array, 0);
    channel_arr = env->GetIntArrayElements(channel_array, 0);
    mac_arr = env->GetByteArrayElements(mac_array, 0);

    if(rssi_arr != NULL && channel_arr != NULL && mac_arr != NULL) {
        wifiApInfo.apLen = ap_len;
        for(int i=0;i<MAX_REPORTED_APS;i++) {
            wifiApInfo.rssi[i] = rssi_arr[i];
            wifiApInfo.channel[i] = channel_arr[i];
        }
        for(int j=0;j<(MAC_ADDRESS_LENGTH * MAX_REPORTED_APS);j++){
            wifiApInfo.mac_address[j] = mac_arr[j];
        }
    }

    if (nLBSAdapter) {
        nLBSAdapter->injectWifiApInfo(wifiApInfo);
    }
    if(NULL != rssi_arr) {
        env->ReleaseIntArrayElements(rssi_array, rssi_arr, 0);
    }
    if(NULL != channel_arr) {
        env->ReleaseIntArrayElements(channel_array, channel_arr, 0);
    }
    if(NULL != mac_arr) {
        env->ReleaseByteArrayElements(mac_array, mac_arr, 0);
    }
}

static JNINativeMethod sMethods[] = {
    /* name, signature, funcPtr */
    {"native_wiper_class_init", "()V", (void *)classInit},
    {"native_wiper_init", "(I)V", (void *)instanceInit},
    {"native_wiper_send_network_location",  "(IDDFI[B[I[III)V", (void*)sendNetworkLocation},
    {"native_wiper_send_passive_location",  "(IDDF)V", (void*)sendPassiveLocation},
    {"native_wiper_send_wifi_ap_info",  "([B[I[II)V", (void*)sendWifiApInfo},
};

int register_Wiper(JNIEnv* env) {
    return jniRegisterNativeMethods(env, "com/qualcomm/location/Wiper",
                                    sMethods, NELEM(sMethods));
}
