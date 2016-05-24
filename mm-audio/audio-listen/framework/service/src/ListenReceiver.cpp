/*
** Copyright (c) 2013-2014 Qualcomm Technologies, Inc. All Rights Reserved.
** Qualcomm Technologies Confidential and Proprietary.
**
** Not a Contribution, Apache license notifications and
** license are retained for attribution purposes only.
*/
/*
**
** Copyright 2006, The Android Open Source Project
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
 * ListenMasterControl is a particular type of ListenReceiver used to
 *    globablly change the start of Listen features.
 * ListenVoiceWakeupSession is a particular type of ListenReceiver used to
 *    perform voice wakeup detection.
 *
 * includes implimentation of
 *     com_qualcomm_listen_ListenVoiceWakeupSession JNI class methods
 *
 */

#define LOG_TAG "(Native) ListenReceiver"
#define LOG_NDEBUG 0

#include <utils/Log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>

#include <binder/MemoryBase.h>
#include <binder/Parcel.h>

#include <utils/KeyedVector.h>
#include <utils/String8.h>

#include <system/audio.h>
#include <system/window.h>

#include <ListenReceiver.h>
#include <IListenService.h>


namespace android {

ListenReceiver::ListenReceiver()
     :  mReceiverType(LISTEN_RECEIVER_UNDEFINED),
        mListener(NULL),
        mLockThreadId(0)
{
    ALOGV("non-default constructor");
    // mLock & mNotifyLock are initialized when  Mutex::Autolock called
    ALOGV("constructor returns");
}

//
// Lock not acquired in base class destructor but rather in child destructor
//
ListenReceiver::~ListenReceiver()
{
    ALOGV("destructor entered");
    listen_status_enum_t status = LISTEN_SUCCESS;

    clearListener();  // make sure client listener free even if receiver not active

    IPCThreadState::self()->flushCommands();
    ALOGV("deconstructor returns");
}

void ListenReceiver::setListener(ListenReceiverListener * listener)
{
    ALOGV("setListener entered with listener = %p for %p", listener, this );
    mListener = listener;
    ALOGV("setListener returns");
    return;
}

void ListenReceiver::clearListener()
{
    ALOGV("clearListener entered, mListener is %p for %p", mListener, this);
    Mutex::Autolock _l(mNotifyLock);
    if ( mListener ) {
       delete mListener;
       mListener = NULL;
    }
    ALOGV("clearListener returned");
    return;
}

/*
 * Get Listen parameter
 *
 * The form of this function accepts a single parameter type and return value as int32
 * All receivers may query the any parameter
 *
 * Param [in]  eParamType - one of Listen listen_param_enum_t
 * Return - current value of parameter
 */
int32_t  ListenReceiver::getParam(listen_param_enum_t eParamType)
{
    int32_t retVal = UNDEFINED;
    ALOGV("getParameter(%d) entered", (int)eParamType);
    const sp<IListenService>& service(getListenService());
    if (service == 0) {
       ALOGE("getParameter: Could not get ListenService ");
       return LISTEN_ENOT_INITIALIZED;
    }
    retVal = service->getParameter(eParamType);
    ALOGV("getParameter(%d) returns %d", (int)eParamType, retVal);
    return retVal;
}

/*
 * notify
 *
 * Calls client 'listener' notify method with a particular Listen event:
 *    LISTEN_FEATURE_DISABLED:
 *        Listen Feature disabled by application via MasterControl
 *    LISTEN_FEATURE_ENABLED:
 *        Listen Feature enabled by application via MasterControl
 *    VOICE_WAKEUP_FEATURE_DISABLED:
 *        VoiceWakeup Feature disabled by application via MasterControl
 *    VOICE_WAKEUP_FEATURE_ENABLED:
 *        VoiceWakeup Feature enabled by application via MasterControl
 *    LISTEN_ENGINE_STARTED:
 *        sent when MAD HW disabled by HAL or Listen Engine
 *    LISTEN_ENGINE_STOPPED:
 *        sent when MAD HW is re-Enabled by HAL or Listen Engine
 *    LISTEN_DETECT_SUCCEED:
 *        Keyword detection was successful - minimum keyword and user confidence levels were met
 *    LISTEN_DETECT_FAILED:
 *        Keyword detection would have failed but when Special Detect-All mode this event can be return
 *    DEREGISTERED:
 *        SoundModel de-registered by ListenNativeService because
 *            MAD HW Disabled by MasterControl client
 *    ENGINE_DIED:
 *        ListenService died
 *
 * Param [in]  msg - event message type
 * Param [in]  obj - Parcel contain data to associated with event
 * Return - none
 */
void ListenReceiver::notify(int msg, const Parcel *obj)
{
    ALOGV("notify(msg=%d) entered", msg);
    bool locked = false;
    ListenVWUSession *sessionReceiver = NULL;

    // Ensure that thread that calls this function is NOT in the same process as
    // the MediaServer - lock only if different process to avoid deadlock.
    if (mLockThreadId != getThreadId()) {
        mLock.lock();
        locked = true;
    }

    switch ((listen_service_event_enum_t)msg) {
      case LISTEN_FEATURE_DISABLED:
      case LISTEN_FEATURE_ENABLED:
      case VOICE_WAKEUP_FEATURE_DISABLED:
      case VOICE_WAKEUP_FEATURE_ENABLED:
      case LISTEN_ENGINE_STARTED:
      case LISTEN_ENGINE_STOPPED:
      case LISTEN_DETECT_SUCCEED:
      case LISTEN_DETECT_FAILED:
        // no special processing for this event types
        break;
      case SOUNDMODEL_DEREGISTERED:
        // Sessions must be notified when SoundModel is de-registered by Listen Native Service
        //   either when MAD HW implicitly Disabled by HAL
        sessionReceiver = (ListenVWUSession *)this;
        sessionReceiver->mbSoundModelRegistered = false;
        break;
      case LISTEN_ENGINE_DIED:
        // ListenService died
        ALOGE("notify: LISTEN_ERROR: ENGINE DIED !!");
        break;
      default:
        ALOGE("notify: unrecognized message %d", msg);
        break;
    }

    if (locked) mLock.unlock();

    // use mNotifyLock to prevent mListener from being cleaned up while
    //     notify is executing
    ALOGD("notify: Audiolock mNotifyLock");
    Mutex::Autolock _l(mNotifyLock);
    if (mListener) {
        ALOGD("notify: callback application");
        mListener->notify(msg, obj);
        ALOGD("notify: back from callback");
    }
    ALOGV("notify returns");
    return;
}

/*
 * died
 *
 * called by IListenReceiver binder if Listen Native Service dies
 *
 * It is assumed that we will clean up ListenReceiver
 *    and App  will ignore previously created objects, and recreate new ones when it wants to
 *    and can.
 *
 * Param - none
 * Return - none
 */
void ListenReceiver::died()
{
    Parcel empty;
    ALOGE("died entered - because Listen Native Service died");
    // sent Java receiver an event that ListenService died so it can clean up / release
    //    it's ListenReceiver objects
    notify(LISTEN_ENGINE_DIED, &empty);

    // clear member variables so that release of this object does NOT attempt to call
    //    ListenService methods
    if (LISTEN_RECEIVER_MASTER_CONTROL == mReceiverType) {
        ALOGV("died: MasterControl receiver member vars reset");
        ListenMasterControl *mstrCntrlReceiver = NULL;
        mstrCntrlReceiver = (ListenMasterControl *)this;
        mstrCntrlReceiver->mbMasterControlGranted = false;
    }
    else if (LISTEN_RECEIVER_VOICE_WAKEUP_SESSION == mReceiverType) {
        ALOGV("died: VWUSession receiver member vars reset");
        ListenVWUSession *sessionReceiver = NULL;
        sessionReceiver = (ListenVWUSession *)this;
        sessionReceiver->mbSoundModelRegistered = false;
        sessionReceiver->mSessionId = UNDEFINED;
    }
    // do not clearListener, let destructor do this
    ALOGV("died returned");
}

// ----------------------------------------------------------------------------
//
// Listen MasterControl receiver
//
// ----------------------------------------------------------------------------

ListenMasterControl::ListenMasterControl()
     :  mbMasterControlGranted(false)
{
    ALOGV("ListenMasterControl::non-default constructor");
    // mLock & mNotifyLock are initialized when  Mutex::Autolock called
    ALOGV("constructor returns");
}

ListenMasterControl::~ListenMasterControl()
{
    ALOGV("ListenMasterControl::destructor entered");
    listen_status_enum_t status = LISTEN_SUCCESS;
    Mutex::Autolock _l(mNotifyLock);
    if (mbMasterControlGranted) {
       // this receiver acquired masterControl, or openned a session
       // release this resource from ListenService
       release();
    }
}

/* Initialize MasterControl receiver
 *
 * Associates this Listen MasterControl object within the Listen Native Service
 *
 * Return - errors
 *       LISTEN_ERESOURCE_NOT_AVAILABLE  - maximum available sessions already instanced
 */
listen_status_enum_t ListenMasterControl::init()
{
    ALOGV("ListenMasterControl::init entered");
    Mutex::Autolock _l(mLock);
    mReceiverType = LISTEN_RECEIVER_UNDEFINED; // set to MC type only if successful
    const sp<IListenService>& service(getListenService());
    if (service == 0) {
       ALOGE("init: ERROR Could not get ListenService ");
       return LISTEN_ENOT_INITIALIZED;
    }
    mbMasterControlGranted = service->initMasterControl(getpid(), this);
    if (false == mbMasterControlGranted) {
       ALOGE("init: ERROR no free instance of MasterControl available");
       return LISTEN_ERESOURCE_NOT_AVAILABLE;
    }
    mReceiverType = LISTEN_RECEIVER_MASTER_CONTROL;
    return LISTEN_SUCCESS;
}

/* Release Receiver
 *
 * Releases this ListenMasterControl within the Listen native Service
 *
 * Return - errors
 *       LISTEN_ERESOURCE_NOT_AVAILABLE  - maximum available sessions already instanced
 */
listen_status_enum_t ListenMasterControl::release()
{
    listen_status_enum_t status = LISTEN_SUCCESS;
    ALOGV("ListenMasterControl::release entered");
    Mutex::Autolock _l(mLock);
    const sp<IListenService>& service(getListenService());
    if (service == 0) {
       ALOGE("release: ERROR Could not get ListenService ");
       return LISTEN_ENOT_INITIALIZED;
    }
    if (mbMasterControlGranted) {
       status = service->releaseMasterControl();
       mbMasterControlGranted = false;
    }

    clearListener();  // calling JNI function will decrement count before calling release

    ALOGV("release returns status %d", status);
    return status;
}

/*
 * Set Listen parameter via MasterContol methods
 *
 * The function accepts a single parameter type and value
 * Currently the only parameters that can be set are global parameters
 * Only receivers with MasterControl access can set global parameters
 *
 * This puts value into Parcel before calling receiver's
 *    setParameter(int key, const Parcel& request)
 *
 * Param [in]  eParamType - one of Listen listen_param_enum_t
 * Param [in]  value - generic signed int32 containing value
 * Return - errors
 *      LISTEN_ENO_GLOBAL_CONTROL - returned if session does not have permission to set global params
 */
listen_status_enum_t ListenMasterControl::setParam(listen_param_enum_t eParamType,
                                              int32_t             value)
{
    listen_status_enum_t status = LISTEN_SUCCESS;
    ALOGV("ListenMasterControl::setParameter(%d, %d) entered", (int)eParamType, value);
    if (!mbMasterControlGranted) {
       ALOGE("setParameter: ERROR Master control not granted on this receiver ");
       return LISTEN_ENO_GLOBAL_CONTROL;
    }
    const sp<IListenService>& service(getListenService());
    if (service == 0) {
       ALOGE("setParameter: ERROR Could not get ListenService ");
       return LISTEN_ENOT_INITIALIZED;
    }
    status = service->setParameter(eParamType, value);
    ALOGV("setParameter returns status %d", status);
    return status;
}

// ----------------------------------------------------------------------------
//
// Listen VoiceWakeupSession receiver
//
// ----------------------------------------------------------------------------

ListenVWUSession::ListenVWUSession()
     :  mSessionId(UNDEFINED),
        mbSoundModelRegistered(false)
{
    ALOGV("ListenVWUSession::non-default constructor");
    // mLock & mNotifyLock are initialized when  Mutex::Autolock called
    ALOGV("constructor returns");
}

ListenVWUSession::~ListenVWUSession()
{
    ALOGV("ListenVWUSession::destructor entered");
    listen_status_enum_t status = LISTEN_SUCCESS;
    Mutex::Autolock _l(mNotifyLock);

    if (mSessionId != UNDEFINED) {
       // this receiver acquired masterControl, or openned a session
       // release this resource from ListenService
       release();
    }
}

/* Initialize VoiceWakeupSession receiver
 *
 * Associates this Listen VoiceWakeupSession object within the Listen Native Service
 *
 * Return - errors
 *       LISTEN_ERESOURCE_NOT_AVAILABLE  - maximum available sessions already instanced
 */
listen_status_enum_t ListenVWUSession::init()
{
    listen_status_enum_t status = LISTEN_SUCCESS;
    ALOGV("ListenVWUSession::init entered");
    Mutex::Autolock _l(mLock);
    mReceiverType = LISTEN_RECEIVER_UNDEFINED; // set to Session type only if successful
    const sp<IListenService>& service(getListenService());
    if (service == 0) {
       ALOGE("init: ERROR Could not get ListenService ");
       return LISTEN_ENOT_INITIALIZED;
    }
    // check if already session created
    if (mSessionId != UNDEFINED) {
       ALOGE("init: ERROR session already created ");
       return LISTEN_EFAILURE;
    }
    mSessionId = service->initSession(LISTEN_VOICE_WAKEUP_SESSION, getpid(), this);
    ALOGV("init: mSessionId set to %d", mSessionId);
    if (UNDEFINED == mSessionId) {
       ALOGE("init: ERROR createSession returned bad sessionId");
       return LISTEN_ERESOURCE_NOT_AVAILABLE;
    }
    mReceiverType = LISTEN_RECEIVER_VOICE_WAKEUP_SESSION;
    ALOGV("init returns status %d", status);
    return status;
}

/* Release VoiceWakeupSession receiver
 *
 * Releases this VoiceWakeupSession within the Listen native Service
 *
 * Return - errors
 *       LISTEN_ERESOURCE_NOT_AVAILABLE  - maximum available sessions already instanced
 */
listen_status_enum_t ListenVWUSession::release()
{
    listen_status_enum_t status = LISTEN_SUCCESS;
    ALOGV("ListenVWUSession::release entered");
    Mutex::Autolock _l(mLock);
    const sp<IListenService>& service(getListenService());
    if (service == 0) {
       ALOGE("release: ERROR Could not get ListenService ");
       return LISTEN_ENOT_INITIALIZED;
    }
    if (UNDEFINED != mSessionId) {
       if (mbSoundModelRegistered) {
          // first deregister sound model for this receiver
          status = deregisterSoundModel();
          mbSoundModelRegistered = false;
       }
       status = service->destroySession(mSessionId);
       mSessionId = UNDEFINED;
    }

    clearListener();  // calling JNI function will decrement count before calling release

    ALOGV("release returns status %d", status);
    return status;
}

/*
 * registerSoundModel
 *
 * Specifies the sound model that Listen engine uses to detect desired keyword/sound
 * for this session.
 * Only one model may be registed per session.
 * Session must be initialized before this method is called.
 *
 * Param [in]  pSoundModelParams - pointer to struct with all sound model parameters
 * Return - status
 */
listen_status_enum_t ListenVWUSession::registerSoundModel(
                                listen_sound_model_params_t* pSoundModelParams)
{
    ALOGV("ListenVWUSession::registerSoundModel entered");
    if (NULL == pSoundModelParams) {
        ALOGE("registerSoundModel: ERROR pSoundModelParams is NULL");
        return LISTEN_EBAD_PARAM;
    } else {
        ALOGD("registerSoundModel(params=%p)", pSoundModelParams);
        ALOGD("pSoundModelParams: ");
        ALOGD("    pSoundModelData %p ", pSoundModelParams->sound_model_data);
        ALOGD("    detectionMode %d", (int)pSoundModelParams->detection_mode);
        ALOGD("    minKeywordConfidence %d ", (int)pSoundModelParams->min_keyword_confidence);
        ALOGD("    minUserConfidence %d ", (int)pSoundModelParams->min_user_confidence);
        ALOGD("    detectFailure %d ", (int)pSoundModelParams->detect_failure);
    }
    listen_status_enum_t status = LISTEN_SUCCESS;
    if (mbSoundModelRegistered) {
       ALOGE("registerSoundModel: ERROR already registered");
       return LISTEN_ESOUNDMODEL_ALREADY_REGISTERED;
    }
    if (UNDEFINED == mSessionId) {
       ALOGE("registerSoundModel: ERROR Receiver is not initialized yet");
       return LISTEN_ENOT_INITIALIZED;
    }
    const sp<IListenService>& service(getListenService());
    if (service == 0) {
       ALOGE("registerSoundModel: ERROR Could not get ListenService ");
       return LISTEN_ENOT_INITIALIZED;
    }
    ALOGV("call registerSoundModel with sessionId %d, params %p", mSessionId, pSoundModelParams);
    status = service->registerSoundModel(mSessionId, pSoundModelParams);
    if (LISTEN_SUCCESS != status) {
       ALOGE("registerSoundModel: ERROR service->registerSoundModel returns status %d", status);
       return status;
    }
    // save pointer to sound model data
    mbSoundModelRegistered = true;
    ALOGV("registerSoundModel returns status %d", status);
    return status;
}

/*
 * deregisterSoundModel
 *
 * Clears SoundModel registeration for this Listen Session
 *
 * Param - none
 *
 * Return - error
 *      LISTEN_ESOUNDMODEL_NOT_REGISTERED
 */
listen_status_enum_t ListenVWUSession::deregisterSoundModel()
{
    ALOGV("ListenVWUSession::deregisterSoundModel entered");
    listen_status_enum_t status = LISTEN_SUCCESS;
    const sp<IListenService>& service(getListenService());
    if (service == 0) {
       ALOGE("deregisterSoundModel: ERROR Could not get ListenService ");
       return LISTEN_ENOT_INITIALIZED;
    }
    if ( UNDEFINED!=mSessionId ) {
       if (mbSoundModelRegistered) {
          status = service->deregisterSoundModel(mSessionId);
          mbSoundModelRegistered = false;
       } else {
          ALOGE("deregisterSoundModel: Sound model not registered, deregister not called");
          return LISTEN_ESOUNDMODEL_NOT_REGISTERED;
       }
    }
    // otherwise nothing to do

    ALOGV("deregisterSoundModel returned status %d", status);
    return status;
}

}; // namespace android
