/*
**
** Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
** Qualcomm Technologies Proprietary and Confidential.
*/

/*****************************************************************************
 typedefs, enums, and data structures for Listen C/C++ code
******************************************************************************/

#ifndef ANDROID_LISTEN_NATIVE_TYPES_H_
#define ANDROID_LISTEN_NATIVE_TYPES_H_

#include <listen_types.h>

namespace android {

//
// forward declaration
//
struct listen_session;

//
// Typedefs
//
typedef uint32_t listen_session_id_t;

/* This structure defines the id type */
typedef uint32_t   listen_id_t;

static const bool     ENABLE  = true;
static const bool     DISABLE = false;

static const uint32_t UNDEFINED = 0xFFFFFFFF;

//
// Enumerators
//

/* This enum is used to return status of Listen API calls */
typedef enum{
  LISTEN_SUCCESS = 0,                   // must be 0 to match ListenSoundModel lib enums
  // Error numbering that match Java API errors in ListenTypes.java
  LISTEN_EFAILURE = 1,                  // Failed for some unspecified reason - generic; must be 1
  LISTEN_EBAD_PARAM = 2,
  LISTEN_ESOUNDMODEL_NOT_REGISTERED = 3,
  LISTEN_ESOUNDMODEL_ALREADY_REGISTERED = 4,
  LISTEN_EFEATURE_NOT_ENABLED = 5,      // either Listen or VoiceWakeup Feature not enabled
  LISTEN_ERESOURCE_NOT_AVAILABLE = 6,
  LISTEN_ECALLBACK_NOT_SET = 7,         // callback must be set for MasterControl or Session object

  // Error not (yet) exposed to Java
  LISTEN_ENO_GLOBAL_CONTROL,            // returned if session request to change global param but does not have permission
  LISTEN_ENOT_INITIALIZED,
  LISTEN_ERECORDINGS_MISMATCH_KEYWORD,  // Recordings dont match the model
  LISTEN_EPOOR_RECORDINGS,              // Recordings dont have enough SNR, SPL - not yet implemented for Phase I
  LISTEN_ESESSION_NOT_ACTIVE,
} listen_status_enum_t;

/* This enum defines the types of event notifications sent to the App
 * from ListenService.  This is a superset of event setn*/
typedef enum{
  // generic event to indicate error
  LISTEN_ERROR = 0,

  // MAD HW Disabled or re-Enabled by AudioHAL or ListenServer
  LISTEN_FEATURE_DISABLED = 1,
  LISTEN_FEATURE_ENABLED = 2,
  VOICE_WAKEUP_FEATURE_DISABLED = 3,
  VOICE_WAKEUP_FEATURE_ENABLED = 4,

  // Keyword detection was successful - minimum keyword and user confidence levels were met
  LISTEN_DETECT_SUCCEED = 5,

  // Keyword detection would have failed but when Special Detect-All mode this event can be return
  LISTEN_DETECT_FAILED = 6,

  // SoundModel de-registered by ListenNativeService because MAD HW Disabled
  SOUNDMODEL_DEREGISTERED = 7,

  // Listen started or stopped.
  // Could be sent due to microphone device concurrency.
  LISTEN_ENGINE_STARTED = 8,
  LISTEN_ENGINE_STOPPED = 9,

  // Catastropic error occurred; Listen Service has been restarted.
  // All previously created Listen objects are stale & must be recreated.
  LISTEN_ENGINE_DIED = 10
}  listen_service_event_enum_t;

/* This enum defines the parameter Types used by Set/GetParam function*/
typedef enum{
  LISTEN_PARAM_LISTEN_FEATURE_ENABLE = 1,      // used to enable or disable Listen Feature
  LISTEN_PARAM_VOICE_WAKEUP_FEATURE_ENABLE  // used to enable or disable VoiceWakeup Feature
}listen_param_enum_t;

/* Reciever types */
typedef enum{
   LISTEN_RECEIVER_UNDEFINED = 0,
	LISTEN_RECEIVER_MASTER_CONTROL = 1,       // has control over global params
	LISTEN_RECEIVER_VOICE_WAKEUP_SESSION = 2,
}listen_receiver_enum_t;

// number should be set to that same int as the last enum in listen_receiver_enum_t
#define NUM_LISTEN_RECEIVER_TYPES 2

/* Session types */
typedef enum{
	LISTEN_VOICE_WAKEUP_SESSION = 1
}listen_session_enum_t;


/* Sound Model types
 * For now, simple sound models that contains data for 1 keyword are all that can be used
 */
typedef enum{
   LISTEN_SOUNDMODEL_UNDEFINED = 0,
	LISTEN_SOUNDMODEL_KEYWORD_ONLY,
	LISTEN_SOUNDMODEL_USER_KEYWORD
}listen_sound_model_enum_t;

}; // namespace android

#endif // ANDROID_LISTEN_NATIVE_TYPES_H_
