/*
 * Copyright (c) 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
#include <jni.h>

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jboolean JNICALL Java_com_qualcomm_secureservices_otpgenerator_MainActivity_InitPKCS11
  (JNIEnv *, jclass, jstring );

JNIEXPORT void JNICALL Java_com_qualcomm_secureservices_otpgenerator_MainActivity_closePKCS11
  (JNIEnv *, jclass );

JNIEXPORT jboolean JNICALL Java_com_qualcomm_secureservices_otpgenerator_MainActivity_obtainToken
  (JNIEnv *, jclass, jstring );

JNIEXPORT jlong JNICALL Java_com_qualcomm_secureservices_otpgenerator_MainActivity_createOTPKey
  (JNIEnv *, jclass, jstring, jstring );

JNIEXPORT jlong JNICALL Java_com_qualcomm_secureservices_otpgenerator_MainActivity_retrieveKey
  (JNIEnv *, jclass, jstring );

JNIEXPORT jboolean JNICALL Java_com_qualcomm_secureservices_otpgenerator_MainActivity_deleteOTPKey
  (JNIEnv *, jclass, jstring);

JNIEXPORT jlong JNICALL Java_com_qualcomm_secureservices_otpgenerator_MainActivity_generateOTP
  (JNIEnv *, jclass, jlong);

JNIEXPORT void JNICALL Java_com_qualcomm_secureservices_otpgenerator_MainActivity_setSalt
  (JNIEnv *, jclass, jstring);

JNIEXPORT jstring JNICALL Java_com_qualcomm_secureservices_otpgenerator_MainActivity_getSalt
  (JNIEnv *, jclass );

#ifdef __cplusplus
}
#endif

