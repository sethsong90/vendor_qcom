/*
 *  Copyright (c) 2013-2014 Qualcomm Technologies, Inc. All Rights Reserved.
 *  Qualcomm Technologies Proprietary and Confidential.
 */
/*
 * Not a Contribution, Apache license notifications and license are retained
 *  for attribution purposes only.
 */
/*
**
** Copyright 2007, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing pfermissions and
** limitations under the License.
*/

/*
 * Listen VoiceWakeup Session
 *
 * VoiceWakeup Session is a particular type of ListenReceiver used to
 * perform voice wakeup detection.
 *
 * includes implimentation of
 *      com_qualcomm_listen_ListenVoiceWakeupSession JNI class methods
 *
 * In general all methods do the following 3 basic things:
 *    i)   extract fields from jobject input parameters and store into C++ variables
 *    ii)  call Native ListenReceiver C++ methods
 *    iii) insert output parameters and return values from native function into
 *         jobjects
 *
 * Implementation Notes:
 *
 * 1) NULL ptr checks for StrongPointer objects sp<Xyz> pXyz must be of the form
 *       if (pXyz == NULL) { }
 *    Attempting to use "if (NULL == pXyz)" results in compilation error:
 *       "no match for 'operator==' in '0 == pXyz'"
 * 2) It is expected that Java will Garbage Collect all allocated variables:
 *       a) are returned by JNI methods
 *       b) are contained within other Java objects
 *    Code allocating this objects are marked with a comment "GC'ed"
 */

// enable verbose logs
#define LOG_NDEBUG  0
#define LOG_TAG "ListenVWUSession-JNI"
#include <utils/Log.h>

#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <jni.h>
#include "JNIHelp.h"  // for NELEM sizeof()/sizeof(x[0])

#include "android_runtime/AndroidRuntime.h"

#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>

#include <ListenNativeTypes.h>
#include <ListenReceiver.h>

#include <com_qualcomm_listen_ListenVoiceWakeupSession.h>


namespace android {

// ----------------------------------------------------------------------------
//    External Methods
// ----------------------------------------------------------------------------
// com_qualcomm_listen_ListenReceiver.cpp function
//    to get ptr to ListenReceiver C++ object from ListenReceiver Java object
extern  sp<ListenReceiver> getPtrFromJObject(JNIEnv* env, jobject thiz);

// com_qualcomm_listen_ListenSoundModel.cpp function
//    to get ptr to and size of sound model data from ByteBuffer object
extern listen_status_enum_t getSMFromByteBuff(JNIEnv *env,
                                       jobject byteBuffObj,
                                       uint32_t *pSize, uint8_t **ppData);

// ----------------------------------------------------------------------------
//    Native Methods
// ----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Get Parameter
 *
 * Class:     com_qualcomm_listen_ListenMasterControl
 * Method:    getParam
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 *
 * Query the value of a particular Listen parameter
 * Param [in]  env - ptr to JNIEnv
 * Param [in]  thiz - ptr to java class object this method belongs to
 * Param [in]  sParamType - type specified as string
 * Return - current value for this parameter returned as a string
 *      status is not returned
 */
JNIEXPORT jstring JNICALL Java_com_qualcomm_listen_ListenVoiceWakeupSession_getParam
  (JNIEnv *env, jobject thiz, jstring sParamType)
{
   ALOGV("Java_com_qualcomm_listen_ListenVoiceWakeupSession_getParam entered");
   sp<ListenReceiver> pListenReceiver = getPtrFromJObject(env, thiz);
   if (pListenReceiver == NULL) {
      ALOGE("_getParam: ERROR listener not set for VoiceWakeupSession object");
      return (jstring)NULL;
   }
   listen_status_enum_t eStatus = LISTEN_SUCCESS;
   listen_param_enum_t eParamType;

   // convert param type string parameter to enum
   const char *ntvParamType = env->GetStringUTFChars(sParamType, 0);
   if ( !(strncmp("ListenFeature", ntvParamType, sizeof("ListenFeature")) ) ) {
      eParamType = LISTEN_PARAM_LISTEN_FEATURE_ENABLE;
   } else if (!(strncmp("VoiceWakeupFeature", ntvParamType, sizeof("VoiceWakeupFeature")) ) ) {
      eParamType = LISTEN_PARAM_VOICE_WAKEUP_FEATURE_ENABLE;
   } else {
      ALOGE("setParam: ERROR type %s not recognized", ntvParamType);
      return (jstring)NULL;
   }

   int32_t iValue = pListenReceiver->getParam(eParamType);
   // convert value to string then to jstring
   jstring jstrBuf; // will be GC'ed by Java
   if (iValue == 0) {
      jstrBuf = env->NewStringUTF("disable");
   } else if (iValue == 1) {
      jstrBuf = env->NewStringUTF("enable");
   } else {
      jstrBuf = env->NewStringUTF("invalid");
   }

   // release native strings
   env->ReleaseStringUTFChars(sParamType, ntvParamType);
   ALOGV("getParam(%d) returns %d as %s.",(int)eParamType, (int)iValue, env->GetStringUTFChars(jstrBuf,0));
   return jstrBuf;
}

/*
 * Register SoundModel
 *
 * Class:     com_qualcomm_listen_ListenVoiceWakeupSession
 * Method:    registerSoundModel
 * Signature: (Lcom/qualcomm/listen/ListenTypes/SoundModelParams;)I
 *
 * Specifies the sound model data that Listen engine uses to detect desired keyword/sound
 * for this session.
 * Only one Sound Model may be registered per session.
 * If a model was previous register for this session, it must be explicitly de-registered.
 *
 * Session must be initialized before calling
 *
 * Param [in]  env - ptr to JNIEnv
 * Param [in]  thiz - ptr to java class object this method belongs to
 * Param [in]  pSoundModelParams - ptr to data and settings required to register a SoundModel
 *            ptr to structure of type ListenTypes.SoundModelParams
 *
 * Return - status
 *     EBAD_PARAM
 *     ESESSION_NOT_INITIALIZED
 */
JNIEXPORT jint JNICALL Java_com_qualcomm_listen_ListenVoiceWakeupSession_registerSoundModel
                (JNIEnv *env, jobject thiz,
                 jobject soundModelParams)    //  ListenTypes$SoundModelParams
{
   ALOGV("Java_com_qualcomm_listen_ListenVoiceWakeupSession_registerSoundModel entered");

   if (NULL == soundModelParams) {
      ALOGE("_registerSoundModel: ERROR soundModelParams is NULL");
      return (jint)LISTEN_EBAD_PARAM;
   }

   listen_status_enum_t eStatus = LISTEN_SUCCESS;
   // listener callback must be set before this is called
   sp<ListenReceiver>   pListenReceiver = getPtrFromJObject(env, thiz);
   if (pListenReceiver == NULL) {
      ALOGE("_registerSoundModel: ERROR listener not set for VoiceWakeupSession object");
      return (jint)LISTEN_ECALLBACK_NOT_SET;
   }
   ListenVWUSession *pVWUSessionReceiver = (ListenVWUSession *)pListenReceiver.get();
   if (NULL == pVWUSessionReceiver) {
      ALOGE("_registerSoundModel: ERROR pVWUSessionReceiver NULL");
      return (jint)LISTEN_EFAILURE;
   }

   // extract individual sound model params from jobject into struct listen_sound_model_params_t
   jclass smpClazz = env->FindClass("com/qualcomm/listen/ListenTypes$SoundModelParams");
   if (smpClazz == NULL) {
      ALOGE("_registerSoundModel: ERROR FindClass com/qualcomm/listen/ListenTypes$SoundModelParams failed");
      return (jint)LISTEN_EFAILURE;
   }
   jfieldID smBuffObjId = env->GetFieldID(smpClazz, "soundModelData", "Ljava/nio/ByteBuffer;");  // jobject
   jfieldID detectModeId = env->GetFieldID(smpClazz, "detectionMode", "I");
   jfieldID minKeywordConfidenceId = env->GetFieldID(smpClazz, "minKeywordConfidence", "S");
   jfieldID minUserConfidenceId = env->GetFieldID(smpClazz, "minUserConfidence", "S");
   jfieldID failNotifyId = env->GetFieldID(smpClazz, "bFailureNotification", "Z"); // bool
   if ( (NULL == smBuffObjId) || (NULL == detectModeId) ||
        (NULL == minKeywordConfidenceId) || (NULL == minUserConfidenceId) ||
        (NULL == failNotifyId) )
   {
      ALOGE("_registerSoundModel: GetFieldId ListenSoundModelParams class failed");
      return (jint)LISTEN_EFAILURE;
   }

   listen_sound_model_params_t nativeSoundModelParams;
   listen_sound_model_data_t  soundModelStruct;
   nativeSoundModelParams.sound_model_data = &soundModelStruct;

   // soundmodel data is stored as a ByteBuffer jobject, so extract the byte array from it
   ALOGV("_registerSoundModel: GetObjectField smBuffObjId as object");
   jobject smBuffObj = env->GetObjectField(soundModelParams, smBuffObjId);
   ALOGV("_registerSoundModel: ref to Java SM to be registered = %p", smBuffObj);

   ALOGV("_registerSoundModel: call getSMFromByteBuff w/ smBuffObj = %p", smBuffObj);
   eStatus = getSMFromByteBuff(env, smBuffObj,
      &nativeSoundModelParams.sound_model_data->size,
      &nativeSoundModelParams.sound_model_data->data);
   if ( LISTEN_SUCCESS != eStatus) {
       ALOGE("getSMFromByteBuff returned %d", eStatus);
       return (jint)LISTEN_EBAD_PARAM;
   }
   nativeSoundModelParams.detection_mode = (listen_detection_mode_enum_t)env->GetIntField(soundModelParams, detectModeId);
   nativeSoundModelParams.min_keyword_confidence = (uint16_t)env->GetShortField(soundModelParams, minKeywordConfidenceId);
   nativeSoundModelParams.min_user_confidence = (uint16_t)env->GetShortField(soundModelParams, minUserConfidenceId);
   nativeSoundModelParams.detect_failure = (bool)env->GetBooleanField(soundModelParams, failNotifyId);

   ALOGV("registerSoundModel - SM (ptr,size)=(%p,%d), ...",
      nativeSoundModelParams.sound_model_data->data,
      nativeSoundModelParams.sound_model_data->size);
   ALOGV("         ... mode %d, failureFlag %d, ...",
      nativeSoundModelParams.detection_mode,
      nativeSoundModelParams.detect_failure);
   ALOGV("         ... confidenceLevels=(%d,%d).",
      nativeSoundModelParams.min_keyword_confidence,
      nativeSoundModelParams.min_user_confidence);

   // check parameters for valid ranges of value
   if ( (LISTEN_MODE_KEYWORD_ONLY_DETECTION != nativeSoundModelParams.detection_mode) &&
        (LISTEN_MODE_USER_KEYWORD_DETECTION != nativeSoundModelParams.detection_mode) ) {
       ALOGE("_registerSoundModel: ERROR: invalid detection mode %d", nativeSoundModelParams.detection_mode);
       return (jint)LISTEN_EBAD_PARAM;
   }

   if ( nativeSoundModelParams.min_keyword_confidence > 100 ) {  // minKeywordConfidence is unsigned
       ALOGE("_registerSoundModel: ERROR: min Keyword Confidence level %d should be between 0 & 100", nativeSoundModelParams.min_keyword_confidence);
       return (jint)LISTEN_EBAD_PARAM;
   }

   if ( nativeSoundModelParams.min_user_confidence > 100 ) { // minKeywordConfidence is unsigned
       ALOGE("_registerSoundModel: ERROR: min User Confidence level %d should be between 0 & 100", nativeSoundModelParams.min_user_confidence);
       return (jint)LISTEN_EBAD_PARAM;
   }

   eStatus = pVWUSessionReceiver->registerSoundModel(&nativeSoundModelParams);
   if (LISTEN_SUCCESS != eStatus) {
      ALOGE("ERROR receiver->registerSoundModel failed with %d", (int)eStatus);
   }

   ALOGV("_ListenVoiceWakeupSession_registerSoundModel returns %d", eStatus);
   return (jint)eStatus;
}

/*
 * Deregister SoundModel
 *
 * Class:     com_qualcomm_listen_ListenVoiceWakeupSession
 * Method:    deregisterSoundModel
 * Signature: ()I
 *
 * Clears SoundModel registration this session
 *
 * Param [in]  env - ptr to JNIEnv
 * Param [in]  thiz - ptr to java class object this method belongs to
 * Return - status
 *      ESOUNDMODEL_NOT_REGISTERED
 */
JNIEXPORT jint JNICALL Java_com_qualcomm_listen_ListenVoiceWakeupSession_deregisterSoundModel
  (JNIEnv *env, jobject thiz)
{
   ALOGV("Java_com_qualcomm_listen_ListenVoiceWakeupSession_deregisterSoundModel entered");
   sp<ListenReceiver> pListenReceiver = getPtrFromJObject(env, thiz);
   if (pListenReceiver == NULL) {
      ALOGE("_deregisterSoundModel: ERROR listener not set for VoiceWakeupSession object");
      return (jint)LISTEN_ECALLBACK_NOT_SET;
   }
   ListenVWUSession *pVWUSessionReceiver = (ListenVWUSession *)pListenReceiver.get();
   if (NULL == pVWUSessionReceiver) {
      ALOGE("_registerSoundModel: ERROR pVWUSessionReceiver NULL");
      return (jint)LISTEN_EFAILURE;
   }
   listen_status_enum_t eStatus = LISTEN_SUCCESS;
   eStatus = pVWUSessionReceiver->deregisterSoundModel();
   if (LISTEN_SUCCESS != eStatus) {
      ALOGE("ERROR receiver->deregisterSoundModel failed with %d", (int)eStatus);
   }
   ALOGV("_ListenVoiceWakeupSession_deregisterSoundModel returned");
   return (jint)eStatus;
}
#ifdef __cplusplus
}
#endif // __cplusplus

}; // namespace android
