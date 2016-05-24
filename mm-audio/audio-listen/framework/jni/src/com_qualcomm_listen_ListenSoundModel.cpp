/*
 *  Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
 *  Qualcomm Technologies Proprietary and Confidential.
 */
/*
 * Not a Contribution, Apache license notifications and license are retainedparse

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
** See the License for the specific language governing permissions and
** limitations under the License.
*/
/*
 * Listen SoundModel
 *
 * Listen SoundModel is a collection of data that defines the
 *     characteristics of the sound that is to be detected.
 * The detection algorithm uses the registered sound model
 *     to decide if the sound it is given to evaluate matches
 *     these sound model characteristics.
 *
 * No events are generated due to calls to these SoundModel methods.
 * Sound Model methods call Listen SoundModel Library functions directly.
 *
 * includes implimentation of
 *     com_qualcomm_listen_ListenSoundModel JNI class methods
 *
 * In general all methods do the following 3 basic things:
 *    i)   extract fields from jobject input parameters and store into C++ variables
 *    ii)  call ListenSoundModelLib C++  functions
 *    iii) insert output parameters and return values from native function into
 *         jobjects
 *
 * None of these methods call Native ListenReceiver C++ methods
 */
// enable verbose logs
#define LOG_NDEBUG  0
#define LOG_TAG "ListenSM-JNI"
#include <utils/Log.h>

#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>

#include <jni.h>
#include "JNIHelp.h"  // for NELEM sizeof()/sizeof(x[0])
#include "android_runtime/AndroidRuntime.h"

#include <ListenNativeTypes.h>

#include <ListenSoundModelLib.h>

#include <com_qualcomm_listen_ListenSoundModel.h>


/////////////////////////////////////////////////////////////////////////////////

namespace android {

/*
 * Check validity of SoundModel
 *
 * Given a Java ShortBuffer jobject extract the C++ UserRecording data
 *
 * param [in]  env - ptr to JNI environment
 * param [in]  shortBuffObj - java ShortBuffer object
 * param [out] pNumSamples - size of UserRecording contained in shortBuffer
 * param [out] ppData - ptr to short array containing UserRecording data
 *
 * Return - status
 */
listen_status_enum_t checkSoundModel( listen_model_type       * pKeywordModel,
                                      listen_sound_model_info * pModelInfo)
{
   listen_status_enum   status = kSucess;
   listen_status_enum_t eStatus = LISTEN_SUCCESS;
   ALOGV("checkSoundModel entered");
   // Call ListenSoundModel library function to get info about soundmodel
   status = ListenSoundModelLib::querySoundModel(pKeywordModel, pModelInfo);
   if (kSucess != status) {
      ALOGE("checkSoundModel: ERROR querySoundModel() failed");
      eStatus = LISTEN_EFAILURE;
   } else {
      ALOGI("querySoundModel() of Default SM returned type %d: (Keyword=1, User=2)",
         (int)pModelInfo->type);
      ALOGI("                                returned version %d, size %d",
         pModelInfo->version, pModelInfo->size);
      // ListenSoundModelLib code crashes when old SM used - avoid using older versions of SM
      if (pModelInfo->version < 4) {
         ALOGE("checkSoundModel: ERROR Version of soundModel is NOT compatible with Listen SW");
         eStatus = LISTEN_EBAD_PARAM;
      }
   }
   ALOGV("checkSoundModel returns %d", (int)eStatus);
   return eStatus;
}

/*
 * Get SoundModel Data From ByteBuffer
 *
 * Given a Java ByteBuffer jobject extract the C++ soundModel data
 *
 * param [in]  env - ptr to JNI environment
 * param [in]  byteBuffObj - java ByteBuffer object
 * param [out] size - size of SoundModel contained in byteBuffer
 * param [out] data - ptr to byte array containing SoundModel data
 *
 * Return - status
 */
listen_status_enum_t getSMFromByteBuff(JNIEnv *env,
                                       jobject byteBuffObj,
                                       uint32_t *pSize, uint8_t **ppData)
{
   ALOGV("getSMFromByteBuff entered");
   listen_status_enum_t eStatus = LISTEN_SUCCESS;
   // initialize output parameter values
    jclass byteBuffClazz = env->FindClass("java/nio/ByteBuffer");
   if (NULL == byteBuffClazz) {
      ALOGE("FindClass java/nio/ByteBuffer failed");
      return LISTEN_EFAILURE;
   }
   if (NULL == byteBuffObj) {
      ALOGE("byteBuffObj NULL");
      return LISTEN_EBAD_PARAM;
   }
   if (NULL == pSize ||  NULL == ppData ) {
      ALOGE("byteBuffObj NULL");
      return LISTEN_EBAD_PARAM;
   }
   *pSize = 0;
   *ppData = (uint8_t *)NULL;

   // get the ByteBuffer methods we need to call to Get the byte array
   jmethodID hasArrId   = env->GetMethodID(byteBuffClazz, "hasArray", "()Z");
   jmethodID getArrId   = env->GetMethodID(byteBuffClazz, "array", "()[B");
   if ( (NULL == hasArrId) || (NULL == getArrId) ) {
      ALOGE("getSMFromByteBuff: ERROR GetFieldId ByteBuffer class failed");
      return LISTEN_EFAILURE;
   }

   ALOGV("Call ByteBuffer.hasArray()...");
   bool bBuffHasArr = env->CallBooleanMethod(byteBuffObj, hasArrId);
   if (bBuffHasArr) {
       ALOGV("... returns true");
   } else  {
       ALOGE("getSMFromByteBuff: ERROR ... returns false - no array");
       return LISTEN_EBAD_PARAM;
   }
   ALOGV("Call CallObjectMethod -> ByteBuffer.array()...");
   // extract the byte array from Java ByteBuffer into store as C++ ptr to byte[]
   jobject jArrayObj = env->CallObjectMethod(byteBuffObj, getArrId);
   if (NULL == jArrayObj) {
       ALOGE("getSMFromByteBuff: ERROR ... array return NULL");
       return LISTEN_EBAD_PARAM;
   }
   ALOGV("cast jArrayObj = %p to byteArray", jArrayObj);
   jbyteArray byteArray = (jbyteArray)jArrayObj;
   ALOGV("GetArrayLength");
   jsize arrayLen = env->GetArrayLength(byteArray);
   *pSize = (uint32_t)arrayLen;
   ALOGV("Allocate byte array of size %d",*pSize);
   *ppData = (uint8_t *)malloc(*pSize);
   ALOGV("GetByteArrayRegion w/ byteArray=%p, len=%d, into ppData=%p",
      byteArray, arrayLen, *ppData );
   env->GetByteArrayRegion(byteArray, (jsize)0, arrayLen, (jbyte *)*ppData);
   ALOGV("... ByteBuffer.array() returns size, data (%d, %p)", *pSize, *ppData);

   // Call ListenSoundModel library function to get info about soundmodel
   listen_sound_model_info modelInfo;
   listen_model_type    keywordModel;
   keywordModel.size = *pSize;
   keywordModel.data = *ppData;
   eStatus = checkSoundModel(&keywordModel, &modelInfo);
   if (LISTEN_SUCCESS != eStatus) {
       ALOGE("ERROR checkSoundModel() failed");
       return eStatus;
   }
   ALOGV("getSMFromByteBuff returns");
   return LISTEN_SUCCESS;
}


/*
 * Get UserRecording Data From ShortBuffer
 *
 * Given a Java ShortBuffer jobject extract the C++ UserRecording data
 * Also calls ListenSoundModel library function to get info about soundmodel
 *
 * param [in]  env - ptr to JNI environment
 * param [in]  shortBuffObj - java ShortBuffer object
 * param [out] pNumSamples - size of UserRecording contained in shortBuffer
 * param [out] ppData - ptr to short array containing UserRecording data
 *
 * Return - status
 */
listen_status_enum_t getRecFromShortBuff(JNIEnv   *env,
                                         jobject  shortBuffObj,
                                         uint32_t *pNumSamples,
                                         int16_t  **ppData)
{
   ALOGV("getRecFromShortBuff entered");
   listen_status_enum_t eStatus = LISTEN_SUCCESS;
   // initialize output parameter values
   *pNumSamples = 0;
   *ppData = (int16_t *)NULL;
   jclass shortBuffClazz = env->FindClass("java/nio/ShortBuffer");
   if (shortBuffClazz == NULL) {
      ALOGE("FindClass java/nio/ShortBuffer failed");
      return LISTEN_EFAILURE;
   }
   // get the ShortBuffer methods we need to call to Get the short array
   jmethodID hasArrId   = env->GetMethodID(shortBuffClazz, "hasArray", "()Z");
   jmethodID getArrId   = env->GetMethodID(shortBuffClazz, "array", "()[S");
   if ( (NULL == hasArrId) || (NULL == getArrId) ) {
      ALOGE("getRecFromShortBuff: ERROR GetMethodID for ShortBuffer failed");
      return LISTEN_EFAILURE;
   }

   ALOGV("Call ShortBuffer.hasArray()...");
   bool bBuffHasArr = env->CallBooleanMethod(shortBuffObj, hasArrId);
   if (bBuffHasArr) {
       ALOGV("... returns true");
   } else  {
       ALOGE("getRecFromShortBuff: ERROR ... returns false - no array");
       return LISTEN_EBAD_PARAM;
   }
   ALOGV("Call ShortBuffer.array()...");
   // extract the short array from Java ShortBuffer into store as C++ ptr to short[]
   jobject jArrayObj = env->CallObjectMethod(shortBuffObj, getArrId);
   ALOGV("cast jArrayObj = %p to shortArray", jArrayObj);
   jshortArray shortArray = (jshortArray)jArrayObj;
   ALOGV("GetArrayLength");
   jsize arrayLen = env->GetArrayLength(shortArray);
   *pNumSamples = arrayLen;

   ALOGV("Allocate short array of size %d",arrayLen);
   *ppData = (int16_t *)malloc((arrayLen*2));
   ALOGV("GetShortArrayRegion w/ shortArray=%p, len=%d, into ppData=%p",
      shortArray, arrayLen, *ppData );
   env->GetShortArrayRegion(shortArray, (jsize)0, arrayLen, (jshort *)*ppData);
   ALOGV("... ShortBuffer.array() returns size, data (%d, %p)", *pNumSamples, *ppData);

   ALOGV("getRecFromShortBuff returns");
   return LISTEN_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////////


// ----------------------------------------------------------------------------
//    Native Methods
// ----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
/*
 * verifyUserRecording
 *
 * Class:     com_qualcomm_listen_ListenSoundModel
 * Method:    verifyUserRecording
 * Signature: (Ljava/nio/ByteBuffer;Ljava/nio/ShortBuffer;)I
 *
 * Given User-Independent keyword model data this method returns the
 *     detection algorithm's confidence that the given user-specific recordings
 *     matches keyword associated with this given model data
 *
 * Param [in]  env - ptr to JNIEnv
 * Param [in]  clazz - ptr to java class (not an object instance) of this static method
 * Param [in]  userIndeData - contains User-Independent model data
 * Param [in]  recording  - a single recording of user speaking keyword
 *
 * Return - percent confidence level that user recording matches keyword
 *             zero returned if error occured
 */
JNIEXPORT jint JNICALL Java_com_qualcomm_listen_ListenSoundModel_verifyUserRecording
                 (JNIEnv *env, jclass clazz,  // ListenSoundModel
                  jobject userIndeData,       // ByteBuffer
                  jobject rec )               // ShortBuffer
{
   ALOGV("Java_com_qualcomm_listen_ListenSoundModel_verifyUserRecording entered");
   listen_model_type     keywordModel;
   listen_user_recording userRecording;
   int16_t               confidenceLevel = 0;
   jint                  iLevel = 0;
   listen_status_enum    status = kSucess;
   listen_status_enum_t  eStatus = LISTEN_SUCCESS;
   jint                  retStatus = (jint)LISTEN_SUCCESS;

   // Check for NULL jobject parameters
   if ( (NULL == userIndeData) || (NULL == rec) ) {
      ALOGE("_verifyUserRecording: ERROR Null ptr passed to ListenSoundModel.verifyUserRecording");
      return 0;
   }

   userRecording.data = NULL;
   userRecording.n_samples = 0;
   keywordModel.data = NULL;
   keywordModel.size = 0;

   // extract data and size from userIndependentData jobject
   //    and place into keywordModel struct
   eStatus = getSMFromByteBuff(env, userIndeData, &keywordModel.size, &keywordModel.data);
   if ( LISTEN_SUCCESS != eStatus) {
       ALOGE("_verifyUserRecording: ERROR getSMFromByteBuff returned %d", eStatus);
       goto clean_up_keyword_data_ver;
   }
   ALOGV("user independent SM (ptr,size)=(%p,%d)",keywordModel.data,keywordModel.size);

   // extract data and numsamples from recording jobject & put in recording struct
   eStatus = getRecFromShortBuff(env, rec, &userRecording.n_samples, &userRecording.data);
   if ( LISTEN_SUCCESS != eStatus) {
       ALOGE("_verifyUserRecording: ERROR getSMFromShortBuff returned %d", eStatus);
       goto clean_up_keyword_data_ver;
   }
   ALOGV("user recording (ptr,size)=(%p,%d)",userRecording.data,userRecording.n_samples);

   // Call ListenSoundModel library function to determine how closely the recording
   //    matches given keyword contained in user-independent keyword-only SoundModel
   ALOGV("call verifyUserRecording()");
   status = ListenSoundModelLib::verifyUserRecording(&keywordModel, &userRecording, &confidenceLevel);
   if (kSucess != status) {
      ALOGE("_verifyUserRecording: ERROR verifyUserRecording() failed");
      iLevel =  (jint)0;
   } else {
      iLevel = confidenceLevel;
   }
   ALOGI("_ListenSoundModel_verifyUserRecording returned confidence %d", (int)iLevel);

clean_up_keyword_data_ver:
   if (keywordModel.data != NULL)
       free((void *) keywordModel.data);
   if (userRecording.data != NULL)
       free((void *) userRecording.data);

   return (jint) iLevel;
}

/*
 * getSizeWhenExtended
 *
 * Class:     com_qualcomm_listen_ListenSoundModel
 * Method:    getSizeWhenExtended
 * Signature: (Ljava/nio/ByteBuffer;)I
 *
 * Get the total size of bytes required to hold a SoundModel that
 *    containing both User-Independent and User-Dependent model data.
 * Application should call this and create a byte array with this return
 *    size, and use this to hold the 'combined_data' parameter
 *    when extend() method called.
 *
 * Param [in]  env - ptr to JNIEnv
 * Param [in]  clazz - ptr to java class (not an object instance) of this static method
 * Param [in]  userIndependentData - contains User-Independent model data
 *
 * Return - total size of unified User Keyword sound model will be when 'extended'
 */
JNIEXPORT jint JNICALL Java_com_qualcomm_listen_ListenSoundModel_getSizeWhenExtended
                (JNIEnv *env, jclass clazz,  // ListenSoundModel
                 jobject userIndeData)       // ByteBuffer
{
   ALOGV("Java_com_qualcomm_listen_ListenSoundModel_getSizeWhenExtended entered");
   listen_model_type    keywordModel;
	uint32_t             nUserKeywordModelSize = 0;
   jint                 modelSize = 0;
   listen_status_enum   status = kSucess;
   listen_status_enum_t eStatus = LISTEN_SUCCESS;
   jint                 retStatus = (jint)LISTEN_SUCCESS;

   // Check for NULL jobject parameter
   if ( NULL == userIndeData ) {
      ALOGE("_getSizeWhenExtended: ERROR Null ptr passed to ListenSoundModel.getSizeWhenExtended");
      return 0;
   }

   keywordModel.data = NULL;
   keywordModel.size = 0;
   // extract data and size from userIndependentData jobject
   //    and place into keywordModel struct
   eStatus = getSMFromByteBuff(env, userIndeData, &keywordModel.size, &keywordModel.data);
   if ( LISTEN_SUCCESS != eStatus) {
       ALOGE("_getSizeWhenExtended: ERROR getSMFromByteBuff returned %d", eStatus);
       goto clean_up_keyword_data_get_size;
   }
   ALOGV("user independent SM (ptr,size)=(%p,%d)", keywordModel.data, keywordModel.size);

   // Call ListenSoundModel library function to calculate the size of an extend
   //    User-dependent SoundModel appended to given keyword-only SoundModel
   ALOGV("getUserKeywordModelSize() SM (data,size)= (%p,%d)",
      keywordModel.data, keywordModel.size);
   status = ListenSoundModelLib::getUserKeywordModelSize(&keywordModel, &nUserKeywordModelSize);
   if (kSucess != status) {
       ALOGE("_getSizeWhenExtended: ERROR getUserKeywordModelSize() failed");
       modelSize = 0;
   } else {
       modelSize = (jint)nUserKeywordModelSize;
   }
   ALOGI("_ListenSoundModel_getSizeWhenExtended returns %d", (int)modelSize);

clean_up_keyword_data_get_size:
   if (keywordModel.data != NULL)
       free((void *) keywordModel.data);

   return modelSize;
}

/*
 * extend
 *
 * Class:     com_qualcomm_listen_ListenSoundModel
 * Method:    extend
 * Signature: (Ljava/nio/ByteBuffer;I[Ljava/nio/ShortBuffer;Ljava/nio/ByteBuffer;Lcom/qualcomm/listen/ListenTypes/ConfidenceData;)I
 *
 * Extends given User-Independent keyword model data by combining it with
 *    User-Dependent keyword model data created from user recordings
 *    into a single SoundModel
 * The sound model is copied into a memory block pointed to combined_data parameter.
 * Application is responsible for creating a byte array large enough to hold this data
 * Size that this returned SoundModel can be queried using getSizeWhenExtended().
 * At least 5 user recordings should be passed to this method.
 * The more user recordings passed as input, the greater the likelihood
 *     of getting a higher quality SoundModel made.
 *
 * Param [in]  env - ptr to JNIEnv
 * Param [in]  clazz - ptr to java class (not an object instance) of this static method
 * Param [in]  userIndeData - contains user independent Keyword specific model data
 * Param [in]  numRecs - number of recordings of a user speaking the keyword
 * Param [in]  recs  - array of N user recordings
 * Param [out] combinedData  - sound model containing user-independent and user-dependent data
 *             It is assumed that the user has created an ByteByffer for 'combinedData'
 *             whose capacity is large enough to hold byte buffer of copied soundmodel
 * Param [out] qualityData  - structure containing ‘quality’ level of created SoundModel
 *
 * Return - status
 */
JNIEXPORT jint JNICALL Java_com_qualcomm_listen_ListenSoundModel_extend
                (JNIEnv *env, jclass clazz,  // ListenSoundModel
                 jobject userIndeData,       // ByteBuffer
                 jint numRecs,
                 jobjectArray recs,          // array of ShortBuffers
                 jobject combinedData,       // ByteBuffer
                 jobject qualityData)        // ListenTypes$ConfidenceData
{
   ALOGV("Java_com_qualcomm_listen_ListenSoundModel_extend entered");
   listen_model_type           keywordModel;
   listen_user_recording **    userRecsArray = NULL;   // array of ptrs
   listen_model_type           userKeywordModel;
   int16_t		                userMatchingScore = 0 ;
   listen_status_enum          status = kSucess;
   jint                        retStatus = (jint)LISTEN_SUCCESS;
   listen_status_enum_t        eStatus = LISTEN_SUCCESS;
   int32_t                     nRecs = (int32_t)numRecs;
   uint32_t                    smMax = 0;
   jclass                      byteBuffClazz;
   jmethodID                   capacityId;
   int                         capacity = 0;
   jclass                      shortBufferClazz;
   jmethodID                   putId;
   jsize                       smLen = 0;
   jbyteArray                  usmData = NULL;
   jclass                      confidenceDataClazz;
   jfieldID                    userMatchId;
   jint                        iUserMatch = 0;;

   // Check for NULL jobject parameters
   if ( (NULL == userIndeData) || (NULL == recs) ||
        (NULL == combinedData) ||  (NULL == qualityData) ) {
      ALOGE("_extend: ERROR Null ptr passed to ListenSoundModel.extend");
      return LISTEN_EBAD_PARAM;
   }

   keywordModel.data = NULL;
   keywordModel.size = 0;
   userKeywordModel.data = NULL;
   userKeywordModel.size = 0;
   // extract data and size from userIndependentData jobject & put in keywordModel struct
   eStatus = getSMFromByteBuff(env, userIndeData, &keywordModel.size, &keywordModel.data);
   if ( LISTEN_SUCCESS != eStatus) {
       ALOGE("_extend: ERROR getSMFromByteBuff returned %d", eStatus);
       retStatus = LISTEN_EFAILURE;
       goto clean_up_keyword_data_extend;
   }

   ALOGV("_extend: user independent SM (ptr,size)=(%p,%d)", keywordModel.data, keywordModel.size);
   ALOGV("_extend: ref to Java user SM %p", combinedData);

   //
   // should check capacity of combinedData byteBuffer before length before doing Put !!!!!
   //
   status = ListenSoundModelLib::getUserKeywordModelSize(&keywordModel, &smMax);
   // extract data and size from userKeywordModel structure
   //    and place into userKeywordModel ByteBuffer jobject
   //    referenced by combinedData parameter
   byteBuffClazz = env->FindClass("java/nio/ByteBuffer");
   if (byteBuffClazz == NULL) {
       ALOGE("_extend: ERROR FindClass java/nio/ByteBuffer failed");
       retStatus = LISTEN_EFAILURE;
       goto clean_up_keyword_data_extend;
   }
   // get the ByteBuffer methods we need to call to Get the byte array
   capacityId = env->GetMethodID(byteBuffClazz, "capacity", "()I");
   if ( NULL == capacityId ) {
       ALOGE("_extend: ERROR GetMethodId capacity of ByteBuffer class failed");
       retStatus = LISTEN_EFAILURE;
       goto clean_up_keyword_data_extend;
   }
   ALOGV("Call ByteBuffer.capacity()");
   capacity = env->CallIntMethod(combinedData, capacityId);
   ALOGV("ByteBuffer.capacity() returns %d, SMMax size %d", capacity, smMax);
   // test the capacity of ByteBuffer first
   if (capacity < (int)smMax) {
      ALOGE("_extend: ERROR output byteBuffer not large enough to hold %d bytes of SM",smMax);
      retStatus = LISTEN_EBAD_PARAM;
      goto clean_up_keyword_data_extend;
   }

   // extract data and size from each recording in ShortBuffer jobject array
   //    malloc array of listen_user_recording userRecsArray[]
   //    then place into i-th element of userRecordings struct array
   userRecsArray = (listen_user_recording **)malloc( nRecs * (sizeof(listen_user_recording *)) );
   if (NULL == userRecsArray) {
      ALOGE("_extend: ERROR malloc for listen_user_recording[] failed");
      retStatus = LISTEN_EFAILURE;
      goto clean_up_keyword_data_extend;
   }

   // check if recs is an instance of ShortBuffer[]
   ALOGV("FindClass of ShortBuffer[] as [Ljava/nio/ShortBuffer;");
   shortBufferClazz = env->FindClass("[Ljava/nio/ShortBuffer;");
   if (NULL == shortBufferClazz) {
      ALOGE("_extend: ERROR FindClass([java/nio/ShortBuffer) failed");
      retStatus = LISTEN_EFAILURE;
      goto clean_up_keyword_data_extend;
   }
   for (int i=0; i < nRecs; i++) {
       // get the i-th jobject from recs param
       ALOGV("recording[%d] ptr %p", i, userRecsArray[i]);
       userRecsArray[i] = (listen_user_recording *)malloc(sizeof(listen_user_recording));
       if (NULL == userRecsArray[i]) {
          ALOGE("_extend: ERROR malloc for listen_user_recording failed");
          retStatus = LISTEN_EFAILURE;
          goto clean_up_keyword_data_extend;
       }
       ALOGV("call IsInstanceOf(recs, shortBufferClazz)");
       if( !(env->IsInstanceOf(recs, shortBufferClazz))){
          ALOGE("_extend: ERROR recs is not an ShortBuffer[] instance");
          retStatus = LISTEN_EBAD_PARAM;
          goto clean_up_keyword_data_extend;
       }

       ALOGV("call GetObjectArrayElement for element %d of array ptr %p", i, recs );
       jobject userRecObj =  env->GetObjectArrayElement(recs, i); // get i-th jobject recording
       if (NULL == userRecObj) {
          ALOGE("_extend: ERROR GetObjectClass of recs failed");
          retStatus = LISTEN_EBAD_PARAM;
          goto clean_up_keyword_data_extend;
       }

       // copy contents of byteArray into byte array
       // extract data and numsamples from recording jobject & put in recording struct
       eStatus = getRecFromShortBuff(env, userRecObj, &userRecsArray[i]->n_samples, &userRecsArray[i]->data);
       if ( LISTEN_SUCCESS != eStatus) {
           ALOGE("_extend: ERROR getSMFromByteBuff returned %d", eStatus);
           retStatus = LISTEN_EFAILURE;
           goto clean_up_keyword_data_extend;
       }
       ALOGV("user recording[%d] (ptr,size)=(%p,%d)",
          i, userRecsArray[i]->data, userRecsArray[i]->n_samples);
   }

   // Call ListenSoundModel library function create a User-dependent keyword
   //    matches given keyword contained in user-independent keyword-only SoundModel
   ALOGV("createUserKeywordModel() SM (data,size)= (%p,%d),...",
      keywordModel.data, keywordModel.size);
   ALOGV("          ... nRecs = %d,...", nRecs);
   for (int i=0; i < nRecs; i++) {
      ALOGV("          ... rec %d (data,size)= (%p,%d)...", i,
         userRecsArray[i]->data, userRecsArray[i]->n_samples);
   }
   // allocate
   ALOGV("Allocate memory for output parameter userKeywordModel, max size %d", smMax);
   userKeywordModel.size = smMax;
   userKeywordModel.data = (uint8_t *)malloc(smMax);
   ALOGV("ptr to userKeywordModel memory is %p", userKeywordModel.data);

   status = ListenSoundModelLib::createUserKeywordModel(&keywordModel,
               nRecs, userRecsArray,
               &userKeywordModel, &userMatchingScore);
   if (kSucess != status) {
      ALOGE("_extend: ERROR createUserKeywordModel() failed");
      retStatus = LISTEN_EFAILURE;
      goto clean_up_keyword_data_extend;
      // don't return; drop down to clean up memory
   } else {
      ALOGV("createUserKeywordModel output userMatchingScore = %d", (int)userMatchingScore);
      // get the ByteBuffer methods we need to call to Get the byte array
      putId = env->GetMethodID(byteBuffClazz, "put", "([B)Ljava/nio/ByteBuffer;");
      if ( NULL == putId ) {
         ALOGE("_extend: ERROR GetMethodID put for ByteBuffer class failed");
         retStatus = LISTEN_EFAILURE;
         goto clean_up_keyword_data_extend;
      }
      smLen = (jsize)userKeywordModel.size;
      usmData = env->NewByteArray(smLen);
      // byte array Inside ByteBuffer is GC'ed by Java
      ALOGV("createUserKeywordModel SetByteArrayRegion");
      env->SetByteArrayRegion(usmData, (jsize)0, smLen, (jbyte *)userKeywordModel.data );

      ALOGV("createUserKeywordModel call put()");
      combinedData = env->CallObjectMethod(combinedData, putId, usmData);

      // Fill return confidence structure field into ListenTypes$ConfidenceData that is output
      confidenceDataClazz = env->FindClass("com/qualcomm/listen/ListenTypes$ConfidenceData");
      if (NULL == confidenceDataClazz) {
         ALOGE("_extend: ERROR confidenceDataClazz class not found");
         retStatus = LISTEN_EFAILURE;
         goto clean_up_keyword_data_extend;
      }
      // get all Id of all the ListenTypes$VoiceWakeDetectionData fields
      userMatchId = env->GetFieldID(confidenceDataClazz, "userMatch", "I");
      if ( NULL==userMatchId )
      {
         ALOGE("userMatch from DetectionData class not acquired");
         retStatus = LISTEN_EFAILURE;
         goto clean_up_keyword_data_extend;
      }

      // place return detection data into jobject retDetectionData
      iUserMatch = (jint)userMatchingScore;
      ALOGV("createUserKeywordModel set matchingScore %d in to qualityData", (int)iUserMatch);
      env->SetIntField(qualityData, userMatchId, userMatchingScore);

      ALOGV("createUserKeywordModel() returns newSM(data,size)= (%p,%d), score %d",
               userKeywordModel.data, userKeywordModel.size, userMatchingScore);
   }

clean_up_keyword_data_extend:
   // free userRecsArray array
   if (userRecsArray) {
      for (int i=0; i < nRecs; i++) {
         if (userRecsArray[i]) {
             if (userRecsArray[i]->data != NULL)
                free((void *) userRecsArray[i]->data);
             free(userRecsArray[i]);
         }
      }
      free(userRecsArray);
   }
   // clean up keyword model data structures
   if (keywordModel.data != NULL)
       free((void *) keywordModel.data);
   if (userKeywordModel.data != NULL)
      free((void *)userKeywordModel.data);
   if (usmData)
      env->DeleteLocalRef(usmData);

   ALOGV("_ListenSoundModel_extend returns %d", retStatus);

   return retStatus;
}
/*
 * parseVWUDetectionEventData
 *
 * Class:     com_qualcomm_listen_ListenSoundModel
 * Method:    parseVWUDetectionEventData
 * Signature: (Ljava/nio/ByteBuffer;Lcom/qualcomm/listen/ListenTypes/EventData;Lcom/qualcomm/listen/ListenTypes/VoiceWakeupDetectionData;)I
 *
 * Parsers generic payload passed to processEvent() for VoiceWakeup-specific Event Data and
 *    fill fields within voice wakeup detection event data structure.
 * Requires the SoundModel that was used for detection to be given as input.
 *
 *
 * Param [in]   env - ptr to JNIEnv
 * Param [in]   clazz - ptr to java class (not an object instance) of this static method
 * Param [in]   registeredSoundModel - sound model keyword detection was performed with
 * Param [in]   eventPayload - black-box event payload returned by ListenEngine
 * Param [in/out]  outDataStruct - VoiceWakeup detection data structure created in Java
 *              filled by this method
 *
 * Return - status
 */
JNIEXPORT jint JNICALL Java_com_qualcomm_listen_ListenSoundModel_parseVWUDetectionEventData
                (JNIEnv *env, jclass clazz,      // ListenSoundModel
                 jobject registeredSoundModel,   // ByteBuffer
                 jobject eventPayload,           // ListenTypes$EventData
                 jobject outDataStruct)          // ListenTypes$VoiceWakeupDetectionData
{
   ALOGV("Java_com_qualcomm_listen_ListenSoundModel_parseVWUDetectionEventData entered");
   listen_model_type           soundModel;
   listen_event_payload        detectionEventPayload;
   listen_detection_event_type detectionData;
   listen_status_enum          status = kSucess;
   listen_status_enum_t        eStatus = LISTEN_SUCCESS;
   jclass                      eventDataClazz;
   jfieldID                    dataSizeId;
   jfieldID                    dataArrayId;
   jobject                     jArrayObj;
   jbyteArray                  byteArray;
   int                         arrayLen = 0;
   jclass                      vwuDetectionDataClazz  = NULL;
   jfieldID                    statusId;
   jfieldID                    typeId;
   jfieldID                    keywordId;
   jfieldID                    keywordConfidenceLevelId;
   jfieldID                    userConfidenceLevelId;
   jstring                     typeStr = NULL;
   jstring                     keywordStr = NULL;
   jsize                       strLen = 0;
   listen_detection_event_v1 * vwuDetDataV1 = NULL;

   if ( (NULL == registeredSoundModel) || (NULL == eventPayload)  ) {
      ALOGE("_parseVWUDetectionEventData: ERROR Null ptr passed to ListenSoundModel.parseDetectionEventData");
      return (jint)LISTEN_EBAD_PARAM;
   }
   detectionEventPayload.data = NULL;
   detectionEventPayload.size = 0;
   soundModel.data = NULL;
   soundModel.size = 0;
   ALOGV("_parseVWUDetectionEventData: ref to Java registered SM %p", registeredSoundModel);

   // extract data and size from registeredSoundModel jobject & put in soundModel struct
   eStatus = getSMFromByteBuff(env, registeredSoundModel, &soundModel.size, &soundModel.data);
   if ( LISTEN_SUCCESS != eStatus) {
      ALOGE("_parseVWUDetectionEventData: ERROR getSMFromByteBuff returned %d", eStatus);
      eStatus = LISTEN_EBAD_PARAM;
      goto clean_up_detectdata;
   }
   ALOGV("registered SoundModel (ptr,size)=(%p,%d)", soundModel.data, soundModel.size);

   // extract data and size from event_payload jobject
   //    and place into detectionEventPayload struct
   eventDataClazz = env->FindClass("com/qualcomm/listen/ListenTypes$EventData");
   if (NULL == eventDataClazz) {
      ALOGE("_parseVWUDetectionEventData: ERROR EventData class not found");
      eStatus = LISTEN_EFAILURE;
      goto clean_up_detectdata;
   }
   // get size and payload from eventData jObject
   dataSizeId = env->GetFieldID(eventDataClazz, "size", "I");
   dataArrayId  = env->GetFieldID(eventDataClazz, "payload", "[B");
   if ( (NULL==dataSizeId) || (NULL==dataArrayId) ) {
      ALOGE("_parseVWUDetectionEventData: ERROR field from EventData class not acquired");
      eStatus = LISTEN_EFAILURE;
      goto clean_up_detectdata;
   }
   detectionEventPayload.size = env->GetIntField(eventPayload, dataSizeId);

   //
   // extract byte array containing event payload from jobject input parameter
   // store payload and size into detectionEventPayload C structure
   jArrayObj = env->GetObjectField(eventPayload, dataArrayId);
   ALOGV("_parseVWUDetectionEventData: cast jArrayObj = %p to byteArray", jArrayObj);
   byteArray = (jbyteArray)jArrayObj;
   ALOGV("_parseVWUDetectionEventData: GetArrayLength");
   arrayLen = env->GetArrayLength(byteArray);
   ALOGV("_parseVWUDetectionEventData: Allocate byte array of size %d", arrayLen);
   detectionEventPayload.data = (uint8_t *)malloc(arrayLen);
   if (NULL == detectionEventPayload.data) {
      ALOGE("_parseVWUDetectionEventData: ERROR malloc of detection event payload data failed");
      eStatus = LISTEN_EFAILURE;
      goto clean_up_detectdata;
   }
   ALOGV("_parseVWUDetectionEventData: GetByteArrayRegion w/ byteArray=%p, len=%d, into detectionEventPayload.data=%p",
      byteArray, arrayLen, detectionEventPayload.data );
   env->GetByteArrayRegion(byteArray, (jsize)0, (jsize)arrayLen, (jbyte *)detectionEventPayload.data);

   // Call Listen SoundModelLib function to convert black-box event data into
   //    specific structure with specific, well-understood elements
   ALOGV("parseDetectionEventData() SM (data,size)= (%p,%d),...",
      soundModel.data, soundModel.size);
   ALOGV("parseDetectionEventData() payload (data,size)= (%p,%d),...",
      detectionEventPayload.data, detectionEventPayload.size);
   for (int j=0; j<(int)detectionEventPayload.size ;j++) {
      ALOGV("  payload[%d] = 0x%x", j, detectionEventPayload.data[j] );
   }
   // zero out elements of output structure; makes ptrs NULL
   memset(&detectionData, 0, sizeof(listen_detection_event_type) );
   status = ListenSoundModelLib::parseDetectionEventData (
                 &soundModel, &detectionEventPayload, &detectionData);
   if (kSucess != status) {
      ALOGE("_parseVWUDetectionEventData: ERROR parseDetectionEventData() returned error %d",
           status);
      eStatus = LISTEN_EFAILURE;
      goto clean_up_detectdata;
   }
   ALOGI("parseDetectionEventData() returns keyword %s, version %d",
      detectionData.keyword, detectionData.version );
   ALOGI("                                  keywordConfidenceLevel %d",
      detectionData.event.event_v1.keywordConfidenceLevel);
   ALOGI("                                  userConfidenceLevel %d",
      detectionData.event.event_v1.userConfidenceLevel  );

   // NOTE: In the future, this code will need to check the detectionData.version and
   //     map this to the correct data structure
   // For now - since there is only one version of detection data - current version of code
   //     maps this to listen_detection_event_v1 without test
   // Fill return detection structure fields into ListenTypes$VoiceWakeDetectionData that is output
   // create a class object of type VoiceWakeupDetectionData
   vwuDetectionDataClazz = env->FindClass("com/qualcomm/listen/ListenTypes$VoiceWakeupDetectionData");
   if (NULL == vwuDetectionDataClazz) {
       ALOGE("_parseVWUDetectionEventData: ERROR FindClass(com/qualcomm/listen/ListenTypes$VoiceWakeupDetectionData) failed !");
       eStatus = LISTEN_EFAILURE;
       goto clean_up_detectdata;
   }
   // get all Id of all the common ListenTypes$DetectionData fields
   statusId = env->GetFieldID(vwuDetectionDataClazz, "status", "I");
   typeId  = env->GetFieldID(vwuDetectionDataClazz, "type", "Ljava/lang/String;");

   // get all Id of all the ListenTypes$VoiceWakeDetectionData fields
   keywordId  = env->GetFieldID(vwuDetectionDataClazz, "keyword", "Ljava/lang/String;");
   keywordConfidenceLevelId  = env->GetFieldID(vwuDetectionDataClazz, "keywordConfidenceLevel", "S");
   userConfidenceLevelId  = env->GetFieldID(vwuDetectionDataClazz, "userConfidenceLevel", "S");
   if ( (NULL==statusId) || (NULL==typeId) ||(NULL==keywordId) ||
        (NULL==keywordConfidenceLevelId) || (NULL==userConfidenceLevelId) )
   {
      ALOGE("_parseVWUDetectionEventData: ERROR field from VoiceWakeDetectionData class not acquired");
      eStatus = LISTEN_EFAILURE;
      goto clean_up_detectdata;
   }

   // place return detection data into jobject outDataStruct
   ALOGV("_parseVWUDetectionEventData: SetIntField status");
   env->SetIntField(outDataStruct, statusId, status);

   ALOGV("_parseVWUDetectionEventData: NewStringUTF called");
   typeStr = env->NewStringUTF("VoiceWakeup_DetectionData_v0100");

   // All objects within return class 'outDataStruct' ('typeStr' and 'keywordStr') GC'ed by Java
   ALOGV("_parseVWUDetectionEventData: SetObjectField keyword");
   env->SetObjectField(outDataStruct, typeId, typeStr);
   // If there was an error from parse function, just return status and type now
   if (kSucess != status) {
      ALOGE("_parseVWUDetectionEventData: ERROR parseDetectionEventData() failed only status and type set");
      eStatus = LISTEN_EFAILURE;
      goto clean_up_detectdata;
   }

   keywordStr = env->NewStringUTF( detectionData.keyword );
   strLen = env->GetStringUTFLength(keywordStr);
   ALOGV("_parseVWUDetectionEventData: new jstring contains keyword %s, length %d", detectionData.keyword, (int)strLen);
   ALOGV("_parseVWUDetectionEventData: SetObjectField keywordStr");
   env->SetObjectField(outDataStruct, keywordId, keywordStr);

   vwuDetDataV1 = &detectionData.event.event_v1;
   ALOGV("_parseVWUDetectionEventData: SetShortField keywordConfidenceLevel");
   env->SetShortField(outDataStruct, keywordConfidenceLevelId, vwuDetDataV1->keywordConfidenceLevel);
   ALOGV("SetShortField userConfidenceLevel");
   env->SetShortField(outDataStruct, userConfidenceLevelId, vwuDetDataV1->userConfidenceLevel);

clean_up_detectdata:
   if (detectionEventPayload.data) {
      free((void *)detectionEventPayload.data);
   }
   // detectionData.keyword is a static array within detectionData so does not have to be freed
   if (soundModel.data != NULL) {
      free((void *)soundModel.data);
   }
   ALOGV("_ListenSoundModel_parseVWUDetectionEventData returns");
   return (jint)eStatus;
}

#ifdef __cplusplus
}
#endif // __cplusplus

}; // namespace android
