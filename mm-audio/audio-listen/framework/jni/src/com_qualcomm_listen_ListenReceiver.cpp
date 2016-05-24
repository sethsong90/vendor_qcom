/*
 *  Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
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
** See the License for the specific language governing permissions and
** limitations under the License.
*/
/*
 * Listen Receiver
 *
 * Generic class inherited by MasterControl and Session receiver classes.
 *
 * This file includes implementation of com_qualcomm_listen_ListenReceiver JNI class methods
 * These methods call Native ListenReceiver methods to initialize either
 * Listen MasterControl or Session native objects.
 *
 * This file also includes implementation of JNIListenReceiverListener class
 * used to receive and process events from C++ ListenReceiver class.
 * JNIListenReceiverListener deals with sending event notification up to Java
 * ListenReceiver instance associated with this native Listen Receiver.
 * A JNIListenReceiverListener object is created per ListenReceiver and the
 * pointer to this JNIListenReceiverListener object is set in Native ListenReceiver.
 *
 * In general all methods do the following 3 basic things:
 *    i)   extract fields from jobject input parameters and store into C++ variables
 *    ii)  call Native ListenReceiver C++ methods
 *    iii) insert output parameters and return values from native function into
 *         jobjects
 *
 */

// enable verbose logs
#define LOG_NDEBUG  0
#define LOG_TAG "ListenReceiver-JNI"
#include <utils/Log.h>

#include <cutils/properties.h>  // for for getting Listen system property

#include <jni.h>
#include "JNIHelp.h"  // for NELEM sizeof()/sizeof(x[0])

#include "android_runtime/AndroidRuntime.h"
#include <binder/Parcel.h>
#include <utils/Mutex.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <ListenNativeTypes.h>
#include <ListenReceiver.h>

#include <com_qualcomm_listen_ListenReceiver.h>
#include <com_qualcomm_listen_ListenMasterControl.h>
#include <com_qualcomm_listen_ListenVoiceWakeupSession.h>
#include <com_qualcomm_listen_ListenSoundModel.h>


namespace android {

// Java field Id to points to Native ListenReceiver object stored within Java ListenReceiver object.
// This is static since it does not change and can be set up manditory init() call.
static jfieldID    gReceiverPtrId = NULL;     // ptr to ListenReceiver hidden away in Java class

// Java class and method Ids used by notify method
// They are static since they do not change and can be set up during JNI_OnLoad()
static jmethodID   gEventCallbackId = NULL;   // callback method in Java class
static jclass      gEventDataClazz = NULL;

// lock used within ListenReceiver native code only
static Mutex       sLock;

// ----------------------------------------------------------------------------
// ref-counted object for callbacks
class JNIListenReceiverListener: public ListenReceiverListener
{
public:
    JNIListenReceiverListener(JNIEnv* env, jobject thiz);
    ~JNIListenReceiverListener();
    virtual void notify(int msg, const Parcel *obj);
private:
    JNIListenReceiverListener();

    // Strong Ref to ListenReceiver Java object to make method call to for event processing
    jobject     mLRecvrObj;
    Mutex       mListenerLock; // ensure destructor and notify don't interupt each other
};

/* Constructor
 * thiz - instance of ListenMasterControl or ListenVoiceWakeupSession
 */
JNIListenReceiverListener::JNIListenReceiverListener(JNIEnv* env, jobject thiz)
{
    ALOGV("JNIListenReceiverListener constructor thiz = %p entered", thiz);
    // Hold onto the ListerReceiver java class for use in calling the static method
    // that posts events to the application thread.
    jclass clazz = env->GetObjectClass(thiz);
    if (clazz == NULL) {
        ALOGE("JNIListenReceiverListener constructor GetObjectClass(thiz) failed");
        jniThrowException(env, "java/lang/Exception", NULL);
        return;
    }
    // Ensure ListenReceiver object is not garbage collected until we delete the reference
    mLRecvrObj  = env->NewGlobalRef(thiz);
    ALOGV("JNIListenReceiverListener constructor this = %p returns after setting mLRecvrObj = %p", this, mLRecvrObj);
}

/* Destructor
*/
JNIListenReceiverListener::~JNIListenReceiverListener()
{
    ALOGV("JNIListenReceiverListener destructor enters, this = %p, mLRecvrObj = %p", this, mLRecvrObj);
    // remove global references
    JNIEnv *env = AndroidRuntime::getJNIEnv();
    ALOGD("JNIListenReceiverListener destructor DeleteGlobalRef %p", mLRecvrObj);
    env->DeleteGlobalRef(mLRecvrObj);
    mLRecvrObj = NULL;
    ALOGV("JNIListenReceiverListener destructor returns ");
}

// ----------------------------------------------------------------------------

/*
 * set pointer to Native ListenReceiver object into Java ListenReceiver instance
 * Param [in]  env - ptr to JNIEnv
 * Param [in]  thiz - ptr to Java ListenReceiver class object
 * Param [in]  receiver - ptr to Native ListenReceiver class to be associated with
 *              Java ListenReceiver object
 */
sp<ListenReceiver> setPtrInJObject(JNIEnv* env, jobject thiz, const sp<ListenReceiver>& receiver)
{
    ALOGV("setPtrInJObject entered, thiz = %p", thiz);
    // Not locked here - lock in _init()
    sp<ListenReceiver> old = (ListenReceiver*)env->GetIntField(thiz, gReceiverPtrId);
    if (old != 0) {
       ALOGV("setPtrInJObject: incStrong sp<%p>", old.get());
       old->decStrong(thiz);
    }
    if (receiver == 0) {
       ALOGV("setPtrInJObject: set NULL into receiver field");
       env->SetIntField(thiz, gReceiverPtrId, 0);
    } else {
       if (receiver.get()) {
          ALOGV("setPtrInJObject: incStrong sp<%p>", receiver.get());
          receiver->incStrong(thiz);
       }
       ALOGV("setPtrInJObject: set %p into receiver field", receiver.get());
       env->SetIntField(thiz, gReceiverPtrId, (int)receiver.get());
    }
    ALOGV("setPtrInJObject returns");
    return old;
}

sp<ListenReceiver> getPtrFromJObject(JNIEnv* env, jobject thiz)
{
    ALOGV("getPtrFromJObject entered, thiz = %p", thiz);
    if (NULL == gReceiverPtrId) {
       ALOGE("getPtrFromJObject: ERROR gReceiverPtrId is NULL");
       return NULL;
    }
    ListenReceiver* const p = (ListenReceiver*)env->GetIntField(thiz, gReceiverPtrId);
    if (p) {
       ALOGV("getPtrFromJObject returns %p", p);
       return sp<ListenReceiver>(p);
    } else {
       ALOGV("getPtrFromJObject returns NULL");
       return NULL;
    }
}

// ----------------------------------------------------------------------------
//    Native Methods
// ----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
/*
 * initialize
 *
 * Class:     com_qualcomm_listen_ListenReceiver
 * Method:    init
 * Signature: (I)I
 *
 * Initialize a native ListenReceiver
 *
 * Param [in]  env - ptr to JNIEnv
 * Param [in]  thiz - ptr to java class object this method belongs to
 * Param [in]  type - type of receiver: e.g. MasterControl, VWUSession,...
 * Return - errors
 *       ERESOURCE_NOT_AVAILABLE  - maximum available sessions already instanced
 */
JNIEXPORT jint JNICALL Java_com_qualcomm_listen_ListenReceiver_init
  (JNIEnv *env, jobject thiz, jint type)
{
   ALOGV("Java_com_qualcomm_listen_ListenReceiver_init entered");
   jclass inheritorClazz = NULL;
   jclass receiverClazz = NULL;
   listen_status_enum_t eStatus = LISTEN_SUCCESS;
   jint status =  NO_ERROR;
   ListenReceiver *receiver = NULL;
   sp<ListenReceiver> spReceiver = NULL;
   listen_callback_t cb = NULL;
   void * pThiz = NULL;
   bool bSpReceiverInc = false;
   JNIListenReceiverListener *pListener = NULL;

   char listenPropStr[PROPERTY_VALUE_MAX];
   property_get("listen.enable", listenPropStr, "1");
   if ( (strncmp(listenPropStr, "0", PROPERTY_VALUE_MAX))==0  ||
        (strncmp(listenPropStr, "false", PROPERTY_VALUE_MAX))==0 ) {
      // disable Listen object creation if Listen system property is explicitly disabled
      ALOGE("_init: ERROR Listen feature is disabled in system.prop");
      return (jint)LISTEN_EFAILURE;
   }

   listen_receiver_enum_t receiverType = (listen_receiver_enum_t)type;

   ALOGD("_init: lock sLock");
   Mutex::Autolock l(sLock);

   // Get the class for the receiver object that is being initialized
   if (LISTEN_RECEIVER_MASTER_CONTROL == receiverType) {
       inheritorClazz = env->FindClass("com/qualcomm/listen/ListenMasterControl");
       ALOGV("_init: new ListenMasterControl");
       receiver = (ListenReceiver *)new ListenMasterControl();
   } else if (LISTEN_RECEIVER_VOICE_WAKEUP_SESSION == receiverType){
       inheritorClazz = env->FindClass("com/qualcomm/listen/ListenVoiceWakeupSession");
       ALOGV("_init: new ListenVWUSession");
       receiver = (ListenReceiver *)new ListenVWUSession();
   } else {
      ALOGE("_init: ERROR receiver type %d invalid", receiverType);
      return (jint)LISTEN_EBAD_PARAM;
   }
   if (receiver == NULL) {
      ALOGE("_init: ERROR new ListenReceiver failed");
      jniThrowException(env, "java/lang/RuntimeException", "Out of memory");
      return (jint)LISTEN_EFAILURE;
   }
   // assign receiver to strongPtr immediately
   spReceiver = receiver;

   if (NULL == inheritorClazz) {
      eStatus = LISTEN_EFAILURE;
      goto cleanup_after_error;
   }

   // create new listener and pass it to newed ListenReceiver C++ instance
   ALOGV("_init: new JNIListenReceiverListener");
   pListener = new JNIListenReceiverListener(env, thiz);
   if (NULL == pListener) {
      ALOGE("_init: ERROR new JNIListenReceiverListener failed");
      jniThrowException(env, "java/lang/RuntimeException", "Out of memory");
      eStatus = LISTEN_EFAILURE;
      goto cleanup_after_error;
   }
   // save ptr to JNIListenReceiverListener in native ListenReceiver object
   ALOGV("_init: setListener");
   receiver->setListener(pListener);

   // get the fields
   gReceiverPtrId = env->GetFieldID(inheritorClazz, "nativeClient", "I");
   if (gReceiverPtrId == NULL) {
      ALOGE("_init: ERROR nativeClient member var not found in clazz");
      eStatus = LISTEN_EFAILURE;
      goto cleanup_after_error;
   }

   if (LISTEN_RECEIVER_MASTER_CONTROL == receiverType) {
       ListenMasterControl *mstrCtrlReceiver = (ListenMasterControl *)receiver;
       ALOGV("_init: call mstrCtrlReceiver->init");
       eStatus = mstrCtrlReceiver->init();
   } else if (LISTEN_RECEIVER_VOICE_WAKEUP_SESSION == receiverType){
       ListenVWUSession *vwuSessionReceiver = (ListenVWUSession *)receiver;
       ALOGV("_init: call vwuSessionReceiver->init");
       eStatus = vwuSessionReceiver->init();
   }

   ALOGD("_init: XXX->init returns %d", (int)eStatus);
   if (LISTEN_SUCCESS != eStatus) {
      ALOGE("_init: ERROR receiver->init failed with %d", eStatus);
      eStatus = LISTEN_EFAILURE;
      goto cleanup_after_error;
   } else {
      // Stow our new C++ ListenReceiver in an opaque field in the Java ListenReceiver object as a strong ptr
      // instance count is incremented by this method not before
      // get the field for clazz member var mNativeReceiver
      ALOGV("_init: save sp<receiver>");
      setPtrInJObject(env, thiz, spReceiver);
      bSpReceiverInc = true;
   }
   ALOGV("_ListenReceiver_init returns successfully");
   return (jint)LISTEN_SUCCESS;

cleanup_after_error:
   ALOGD("_init cleanup_after_error");
   if (pListener && receiver) {
      ALOGD("_init clearListener");
      receiver->clearListener();
   }
   if ( (spReceiver != NULL) && bSpReceiverInc ) {
      ALOGD("setPtrInJObject: decStrong sp<%p>", spReceiver.get());
      spReceiver->decStrong(thiz);
   }

   ALOGD("_ListenReceiver_init returns after error");
   return (jint)eStatus;
}

/*
 * release
 *
 * Class:     com_qualcomm_listen_ListenReceiver
 * Method:    release
 * Signature: ()I
 *
 * Release ListenReceiver
 *
 * Param [in]  env - ptr to JNIEnv
 * Param [in]  thiz - ptr to java class object this method belongs to
 * Return - errors
 */
JNIEXPORT jint JNICALL Java_com_qualcomm_listen_ListenReceiver_release
  (JNIEnv *env, jobject thiz)
{
   ALOGV("Java_com_qualcomm_listen_ListenReceiver_release entered");
   listen_status_enum_t eStatus = LISTEN_SUCCESS;
   ALOGD("_release: lock sLock");
   Mutex::Autolock l(sLock);
   sp<ListenReceiver> pListenReceiver = getPtrFromJObject(env, thiz);
   if (pListenReceiver == NULL) {
      return LISTEN_SUCCESS; // not able to call ListenReceiver native code
   }
   ALOGV("_release: clear listener set in native Listen Receiver");
   pListenReceiver->clearListener();   // free listener newed in ListenReceiver_init
   ALOGV("_release: call ListenReceiver->release");
   eStatus = pListenReceiver->release();
   if (LISTEN_SUCCESS != eStatus) {
      ALOGE("_release: ERROR receiver->release returned %d", eStatus);
   }
   // decrement reference to native Receiver within java instance
   setPtrInJObject(env, thiz, NULL);

   ALOGV("_ListenReceiver_release returned status %d", eStatus);
   return (jint)eStatus;
}

#ifdef __cplusplus
}
#endif // __cplusplus


// ----------------------------------------------------------------------------

/*
 * Send this event to java object waiting on events
 * If event data in input parcel is not empty, convert to byte array
 *
 * param [in]  eventParcel - parcel containing event data
 *               most events don't have event data
 *               that this event is meant for
 */
void JNIListenReceiverListener::notify(int msg, const Parcel *eventParcel)
{
   ALOGV("JNIListenReceiverListener::notify(msgType %d) entered", msg);
   uint8_t *eventData = NULL;
   jbyteArray dataArray = NULL;
   jmethodID   eventDataConstructorId;
   jobject tmpEventDataObj = NULL;
   jobject eventDataObj = NULL;
   int parcelSize = 0;
   int eventDataSize = 0;
   jfieldID dataArrayId;
   jfieldID dataSizeId;

   JNIEnv *env = AndroidRuntime::getJNIEnv();
   if (NULL == eventParcel) {
      ALOGE("JNIListenReceiverListener::notify eventParcel NULL");
      return;
   }
   ALOGD("JNIListenReceiverListener::notify: lock mListenerLock");
   Mutex::Autolock l(mListenerLock);
   if ( (NULL == gEventCallbackId) || (NULL == mLRecvrObj) || (NULL == gEventDataClazz) ) {
      ALOGE("JNIListenReceiverListener::notify gEventCallbackId is %p, mLRecvrObj is %p, gEventDataClazz is %p",
            gEventCallbackId, mLRecvrObj, gEventDataClazz);
      ALOGE("JNIListenReceiverListener::notify: ERROR bad ptr; notify call SKIPPED!");
      return;
   }
   ALOGV("JNIListenReceiverListener::notify GetMethodID(gEventDataClazz) constructor");
   eventDataConstructorId = env->GetMethodID(gEventDataClazz, "<init>", "()V");
   if ( NULL == eventDataConstructorId ) {
      ALOGE("JNIListenReceiverListener::notify ERROR eventDataConstructorId is NULL");
      return;
   }

   jint eventType = (jint)msg;
   // create a new object of type EventData, clean up after notify is called
   ALOGV("JNIListenReceiverListener::notify NewObject(gEventDataClazz)");
   tmpEventDataObj = env->NewObject(gEventDataClazz, eventDataConstructorId);
   if (tmpEventDataObj == NULL) {
      ALOGE("JNIListenReceiverListener::notify: ERROR NewObject for gEventDataClazz failed");
      return;
   }
   ALOGV("JNIListenReceiverListener::notify NewGlobalRef(tmpEventDataObj)");
   eventDataObj  = env->NewGlobalRef(tmpEventDataObj);
   if (eventDataObj == NULL) {
      ALOGE("JNIListenReceiverListener::notify: ERROR NewGlobalRef gEventDataClazz failed");
      goto cleanup_notify;
   }

   // get size from parcel and store into eventData jObject
   ALOGV("JNIListenReceiverListener::notify GetFieldID(gEventDataClazz,...");
   dataSizeId = env->GetFieldID(gEventDataClazz, "size", "I");
   if (dataSizeId == NULL) {
      ALOGE("JNIListenReceiverListener::notify: ERROR size field from EventData class not acquired");
      goto cleanup_notify;
   } else {
      ALOGD("JNIListenReceiverListener::notify: data size fieldID is %p", dataSizeId);
   }
   if (eventParcel) {
      parcelSize = (int)(eventParcel->dataSize());
      ALOGV("JNIListenReceiverListener::notify  parcel size is %d", parcelSize);
      if (parcelSize > 0) {
         eventParcel->setDataPosition(0);
         ALOGV("JNIListenReceiverListener::notify position %d reset to start", (int)eventParcel->dataPosition());
         // event data contains a size and a array of bytes
         eventDataSize = eventParcel->readInt32();
      }
   }
   ALOGV("JNIListenReceiverListener::notify() set eventDataSize %d", eventDataSize);
   env->SetIntField(eventDataObj, dataSizeId, eventDataSize);

   if (eventDataSize > 0) {
      ALOGV("JNIListenReceiverListener::notify GetFieldID payload from gEventDataClazz");
      dataArrayId  = env->GetFieldID(gEventDataClazz, "payload", "[B"); // byte array
      if (dataArrayId == NULL) {
         ALOGE("JNIListenReceiverListener::notify: ERROR payload field from EventData class not acquired");
         env->DeleteGlobalRef(eventDataObj);  // clean up eventDataObj; won't be needed for method call
         goto cleanup_notify;
      }

      ALOGV("JNIListenReceiverListener::notify Create a byte array jObject and set into eventData jObject");
      eventData = (uint8_t *)malloc(eventDataSize);
      eventParcel->read((void *)eventData, eventDataSize);
      if (NULL == eventData) {
         ALOGE("JNIListenReceiverListener::notify eventData is NULL");
      } else {
         ALOGV("JNIListenReceiverListener::notify eventData ptr %p contains:", eventData);
         for (int j=0; j<eventDataSize; j++) {
            ALOGV("   [%d] = 0x%x", j, eventData[j]);
         }
         dataArray = env->NewByteArray(eventDataSize); // Make a byteArray for event data payload
         // copy eventData into byte array
         ALOGV("JNIListenReceiverListener::notify set eventData byteArrayRegion size %d", eventDataSize);
         env->SetByteArrayRegion(dataArray, (jsize)0, eventDataSize, (jbyte *)eventData);
         ALOGV("JNIListenReceiverListener::notify SetObjectField dataArrayId from eventDataObj");
         env->SetObjectField(eventDataObj, dataArrayId, dataArray);
      }
   } else {
      ALOGV("JNIListenReceiverListener::notify dataArray element left NULL since size is 0");
   }
   // return event data assumed to be copied by receiver if desired
   // call mLRecvrObj's receiveEvent method
   if ( (NULL != mLRecvrObj) && (NULL != gEventCallbackId) ){
      ALOGD("JNIListenReceiverListener::notify() callMethod receiveEvent with mLRecvrObj=%p, gEventCallbackId=%p",
             mLRecvrObj, gEventCallbackId);
      env->CallVoidMethod(mLRecvrObj, gEventCallbackId, eventType, eventDataObj);
      if (env->ExceptionCheck()) {
         // skip exception in this case
         ALOGE("JNIListenReceiverListener::notify: ERROR an exception occurred while notifying Java app of an event!");
         env->ExceptionDescribe();
         env->ExceptionClear();
      }
   } else {
      ALOGE("JNIListenReceiverListener::notify: eventType %d dropped because receive can no longer be reached !", eventType);
   }

cleanup_notify:
   if (eventDataObj) {
   ALOGD("JNIListenReceiverListener::notify DeleteGlobalRef eventDataObj");
   env->DeleteGlobalRef(eventDataObj);
   }
   if (tmpEventDataObj) {
      ALOGD("JNIListenReceiverListener::notify DeleteLocalRef tmpEventDataObj");
      env->DeleteLocalRef(tmpEventDataObj);
   }
   if (dataArray) {
      ALOGD("JNIListenReceiverListener::notify DeleteLocalRef dataArray");
      env->DeleteLocalRef(dataArray);
   }
   if (eventData) {
      ALOGD("JNIListenReceiverListener::notify free eventData");
      free((void *)eventData);
   }
   ALOGV("JNIListenReceiverListener::notify returns");
   return;
}

#ifdef __cplusplus
extern "C" {
#endif
/*
 * JNI_OnLoad
 *
 * This function is called when Java Listen class explicitly loads Listen JNI liblistenjni.so.
 *
 * To ensure that no classes or object references are stale, various JNI variables required by
 * JNIListenReceiverListener::notify() are retrived at load time and stored in static variables.
 */
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    jint version = -1; // undefined

    ALOGV("JNI_OnLoad entered");
    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("JNI_OnLoad: ERROR: GetEnv failed\n");
        return version;
    }
    assert(env != NULL);

    //
    // Get and save classe and method needed by return events to Java ListenReceiver
    // Must be saved from this context
    //
    jclass eventDataClazz = env->FindClass("com/qualcomm/listen/ListenTypes$EventData");
    if (NULL == eventDataClazz) {
       ALOGE("JNI_OnLoad:EventData class not found");
        return version;
    }
    // saved as global reference so it is not stale within notify()
    gEventDataClazz = reinterpret_cast<jclass>(env->NewGlobalRef(eventDataClazz) );
    ALOGV("JNI_OnLoad: gEventDataClazz save %p as globalRef", gEventDataClazz);

    jclass receiverClazz = env->FindClass("com/qualcomm/listen/ListenReceiver");
    if (NULL == receiverClazz) {
       ALOGE("JNI_OnLoad: ERROR FindClass(com/qualcomm/listen/ListenReceiver) failed !" );
    }
    ALOGV("JNI_OnLoad: GetMethodID for receiveEvent in OnLoad");
    gEventCallbackId = env->GetMethodID(receiverClazz, "receiveEvent", "(ILcom/qualcomm/listen/ListenTypes$EventData;)V");
    if (env->ExceptionCheck()) {
      // skip exception in this case
      ALOGE("JNI_OnLoad: ERROR GetMethodID threw an exception");
      env->ExceptionDescribe();
      env->ExceptionClear();
    }
    // jmethodID need not be saved as GlobalRef
    ALOGV("JNI_OnLoad: gEventCallbackId save as %p", gEventCallbackId);

   /* success -- return valid version number */
   version = JNI_VERSION_1_4;
   return version;
}
#ifdef __cplusplus
}
#endif


}; // namespace android
