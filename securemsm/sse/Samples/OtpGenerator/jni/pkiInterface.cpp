/*
 * Copyright (c) 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include "pkiInterface.h"
#include "pki.h"

extern "C" jboolean
Java_com_qualcomm_secureservices_otpgenerator_MainActivity_InitPKCS11(JNIEnv*  env, jclass cls, jstring inJNIStr)
{
  const char *inCStr = env->GetStringUTFChars(inJNIStr, NULL);
  if (NULL == inCStr)
    return false;

  LOGD("MainActivity_InitPKCS11+");
  LOGD("The received string in JNI interface is %s",inCStr);

  Pki * pki = Pki::initPkiInstance(inCStr);
  if (NULL_PTR == pki) {
    env->ReleaseStringUTFChars(inJNIStr, inCStr);
    LOGD("MainActivity_closePKCS11-");
    LOGE("Pki::initPkiInstance returned NULL_PTR");
    return false;
  }

  LOGD("MainActivity_InitPKCS11-");
  env->ReleaseStringUTFChars(inJNIStr, inCStr);
  return true;
}

extern "C" void
Java_com_qualcomm_secureservices_otpgenerator_MainActivity_closePKCS11(JNIEnv*  env, jclass cls)
{
  LOGD("MainActivity_closePKCS11+");
  Pki * pki = Pki::getPkiInstance();
  if (NULL_PTR != pki) {
    pki->closePki();
  }
  LOGD("MainActivity_closePKCS11-");
}

extern "C" jboolean
Java_com_qualcomm_secureservices_otpgenerator_MainActivity_obtainToken(JNIEnv*  env, jclass cls, jstring inJNIStr)
{
  const char *inCStr = env->GetStringUTFChars(inJNIStr, NULL);
  if (NULL == inCStr)
    return false;

  LOGD("MainActivity_obtainToken+");
  LOGD("The received string in JNI interface is %s",inCStr);

  Pki * pki =  Pki::getPkiInstance();
  if (NULL_PTR == pki) {
    env->ReleaseStringUTFChars(inJNIStr, inCStr);
    LOGE("Pki::getPkiInstance() returned NULL_PTR");
    return false;
  }

  if (!pki->obtainToken(inCStr)){
    env->ReleaseStringUTFChars(inJNIStr, inCStr);
    LOGE("pki->obtainToken(inCStr) returned false ");
    return false;
  }

  LOGD("MainActivity_obtainToken-");
  env->ReleaseStringUTFChars(inJNIStr, inCStr);
  return true;
}

extern "C" jlong
Java_com_qualcomm_secureservices_otpgenerator_MainActivity_retrieveKey(JNIEnv*  env, jclass cls, jstring inJNIStr)
{
  const char *inCStr = env->GetStringUTFChars(inJNIStr, NULL);
  if (NULL == inCStr)
    return 0;

  LOGD("MainActivity_retrieveKey+");
  LOGD("The received string in JNI interface is %s",inCStr);

  Pki * pki = Pki::getPkiInstance();;
  if (NULL_PTR == pki) {
    LOGD("MainActivity_retrieveKey-");
    LOGE("Pki::getPkiInstance() returned NULL_PTR");
    env->ReleaseStringUTFChars(inJNIStr, inCStr);
    return 0;
  }

  long val  = pki->retrieveKey(inCStr);
  if (0 == val){
    LOGD("MainActivity_retrieveKey-");
    LOGE("pki->retrieveKey(inCStr) returned 0 ");
    env->ReleaseStringUTFChars(inJNIStr, inCStr);
    return 0;
  }

  LOGD("MainActivity_retrieveKey- 0x%x",val);
  env->ReleaseStringUTFChars(inJNIStr, inCStr);
  return val;
}

extern "C" void
Java_com_qualcomm_secureservices_otpgenerator_MainActivity_SetTimeBased(JNIEnv*  env, jclass cls, jboolean val)
{
  LOGD("MainActivity_SetTimeBased+");
  Pki * pki = Pki::getPkiInstance();
  if (NULL_PTR != pki)
    pki->setTimeBasedOTP(val);
  LOGD("MainActivity_SetTimeBased-");
}

extern "C" jlong
Java_com_qualcomm_secureservices_otpgenerator_MainActivity_createOTPKey(JNIEnv*  env, jclass cls, jstring inJNIStr1, jstring inJNIStr2)
{

  const char *inCStr1 = env->GetStringUTFChars(inJNIStr1, NULL);
  if (NULL == inCStr1)
    return false;

  const char *inCStr2 = env->GetStringUTFChars(inJNIStr2, NULL);
  if (NULL == inCStr2)
    return false;

  LOGD("MainActivity_createOTPKey+");
  LOGD("The received string1 in JNI interface is %s",inCStr1);
  LOGD("The received string2 in JNI interface is %s",inCStr2);

  Pki * pki = Pki::getPkiInstance();
  if (NULL_PTR == pki) {
    LOGD("MainActivity_createOTPKey-");
    LOGE("Pki::getPkiInstance() returned NULL_PTR");
    env->ReleaseStringUTFChars(inJNIStr1, inCStr1);
    env->ReleaseStringUTFChars(inJNIStr2, inCStr2);
    return 0;
  }

  long val = pki->createOTPKey(inCStr1,inCStr2);
  if (!val){
    LOGD("MainActivity_createOTPKey-");
    LOGE("pki->retrieveKey(inCStr) returned false ");
    env->ReleaseStringUTFChars(inJNIStr1, inCStr1);
    env->ReleaseStringUTFChars(inJNIStr2, inCStr2);
    return 0;
  }

  LOGD("MainActivity_createOTPKey-");
  env->ReleaseStringUTFChars(inJNIStr1, inCStr1);
  env->ReleaseStringUTFChars(inJNIStr2, inCStr2);
  return val;

}

extern "C" jlong
Java_com_qualcomm_secureservices_otpgenerator_MainActivity_generateOTP(JNIEnv*  env, jclass cls, jlong handle)
{
  LOGD("MainActivity_generateOTP+");

  Pki * pki = Pki::getPkiInstance();
  if (NULL_PTR == pki) {
    LOGD("MainActivity_generateOTP-");
    LOGE("Pki::getPkiInstance() returned NULL_PTR");
    return 0;
  }

  long val = pki->generateOTP(handle);;
  if (!val){
    LOGD("MainActivity_generateOTP-");
    LOGE("pki->generateOTP(handle) returned false ");
    return 0;
  }

  LOGD("MainActivity_generateOTP- %d", val);
  return val;
}

extern "C" jboolean
Java_com_qualcomm_secureservices_otpgenerator_MainActivity_deleteOTPKey(JNIEnv*  env, jclass cls, jstring inJNIStr)
{
  const char *inCStr = env->GetStringUTFChars(inJNIStr, NULL);
  if (NULL == inCStr)
    return false;

  LOGD("MainActivity_deleteOTPKey+");

  Pki * pki = Pki::getPkiInstance();
  if (NULL_PTR == pki) {
    LOGD("MainActivity_deleteOTPKey-");
    LOGE("Pki::getPkiInstance() returned NULL_PTR");
    env->ReleaseStringUTFChars(inJNIStr, inCStr);
    return false;
  }

  if (!pki->deleteOTPKey(inCStr)){
    LOGD("MainActivity_deleteOTPKey-");
    LOGE("pki->retrieveKey(inCStr) returned false ");
    env->ReleaseStringUTFChars(inJNIStr, inCStr);
    return false;
  }

   env->ReleaseStringUTFChars(inJNIStr, inCStr);
  LOGD("MainActivity_deleteOTPKey-");
  return true;
}

extern "C" void
Java_com_qualcomm_secureservices_otpgenerator_MainActivity_setSalt(JNIEnv*  env, jclass cls, jstring inJNIStr)
{
  const char *inCStr = env->GetStringUTFChars(inJNIStr, NULL);
  if (NULL == inCStr)
    return;

  LOGD("MainActivity_setSalt+");
  LOGD("The received string1 in JNI interface is %s",inCStr);

  Pki * pki = Pki::getPkiInstance();
  if (NULL_PTR != pki)
    pki->setSalt(inCStr);

  env->ReleaseStringUTFChars(inJNIStr, inCStr);
  LOGD("MainActivity_setSalt-");

}

extern "C" jstring
Java_com_qualcomm_secureservices_otpgenerator_MainActivity_getSalt(JNIEnv*  env, jclass cls)
{
  LOGD("MainActivity_getSalt+");
  Pki * pki = Pki::getPkiInstance();
  if (NULL_PTR != pki) {
      LOGD("MainActivity_getSalt-");
      return (env->NewStringUTF(pki->getSalt()));
  } else{
      LOGD("MainActivity_getSalt-Error");
      return NULL;
  }
}

