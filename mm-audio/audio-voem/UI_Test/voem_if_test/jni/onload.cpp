/*============================================================================
  @file onload.cpp
  This module registers the JNI of voice post processing test

  Copyright (c) 2011 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
*//*========================================================================*/

#include "JNIHelp.h"
#include "jni.h"
#include "utils/Log.h"
#include "common_log.h"
#include "utils/misc.h"

namespace android {
int register_VoemTest(JNIEnv* env);
};

using namespace android;

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        LOGE("GetEnv failed!");
        return result;
    }
    LOG_ASSERT(env, "Could not retrieve the env!");


    register_VoemTest(env);

    return JNI_VERSION_1_4;
}
