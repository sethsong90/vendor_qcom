/*
 *    Copyright (c) 2012-2013 Qualcomm Technologies, Inc. All Rights Reserved.
 *    Qualcomm Technologies Proprietary and Confidential.
 */

package com.qualcomm.listen;

import java.nio.ByteBuffer;

/**
 * Listen Type defines all the data structures that are used by
 * ListenEngineAPIs
 */
public class ListenTypes  {

    /*  Types of event notifications sent to the App */

    /** Event notifying that Listen feature is disabled  */
    public static final int LISTEN_FEATURE_DISABLE_EVENT = 1;
    /** Event notifying that Listen feature is enabled  */
    public static final int LISTEN_FEATURE_ENABLE_EVENT = 2;

    /** Event notifying that VoiceWakeuo feature is disabled  */
    public static final int VOICEWAKEUP_FEATURE_DISABLE_EVENT = 3;
    /** Event notifying that VoiceWakeup feature is enabled  */
    public static final int VOICEWAKEUP_FEATURE_ENABLE_EVENT = 4;

    /** Event notifying VoiceWakeup detection was successful -
     *     minimum keyword and user confidence levels were met */
    public static final int DETECT_SUCCESS_EVENT = 5;
    /** Event notifying VoiceWakeup detection have failed
     *  <br>This event is returned only when Special notify-all
     *     mode is enabled */
    public static final int DETECT_FAILED_EVENT = 6;

    /** SoundModel deregistered implicitly by ListenEngine */
    public static final int DEREGISTERED_EVENT = 7;

    /** Event notifying that Listen detection is now active  */
    public static final int LISTEN_RUNNING_EVENT = 8;
    /** Event notifying that Listen detection is not active
     * <br>Listen is temporarily stopped when microphone is used
     *     during phone call or audio recording */
    public static final int LISTEN_STOPPED_EVENT = 9;

    /** Catastrophic error occurred and Listen Service must
     *  be restarted
     *  <br>All previously created Listen objects are stale
     *  and must be recreated
     */
    public static final int LISTEN_ENGINE_DIED = 10;

    /* The following constants define various status types */

    /** Status Success */
    public static final int STATUS_SUCCESS = 0;
    /** Generic failure-unknown reason */
    public static final int STATUS_EFAILURE = 1;
    /** Failure - Bad parameters */
    public static final int STATUS_EBAD_PARAM = 2;
    /** Failure - SoundModel is not registered */
    public static final int STATUS_ESOUNDMODEL_NOT_REGISTERED = 3;
    /** Previously registered SoundModel should be deregistered
     *  before re-registering */
    public static final int STATUS_ESOUNDMODEL_ALREADY_REGISTERED = 4;
    /** Failure - either Listen or VoiceWakeup Feature not
     *  enabled */
    public static final int STATUS_EFEATURE_NOT_ENABLED = 5;
    /** requested session/masterControl object can not be
     *  instanced */
    public static final int STATUS_ERESOURCE_NOT_AVAILABLE = 6;
    /** callback not set for session/masterControl object */
    public static final int STATUS_ECALLBACK_NOT_SET = 7;

    /* The following constants define the detection modes of VoiceWakeup */

    /** Keyword-only detection mode */
    public static final int KEYWORD_ONLY_DETECTION_MODE = 1;
    /** Keyword and user detection mode */
    public static final int USER_KEYWORD_DETECTION_MODE = 2;

    /* The following constants define the parameters for Set/GetParam function */

    /** Defines the param for Listen feature */
    public static final String LISTEN_FEATURE = "ListenFeature";
    /** Defines the param for VoiceWakup feature */
    public static final String VOICEWAKEUP_FEATURE = "VoiceWakeupFeature";
    /** Defines the param for Enable */
    public static final String ENABLE = "enable";
    /** Defines the param for Disable */
    public static final String DISABLE = "disable";

    /** Defines the version to identify the event data structure
     *  that should be allocated before calling
     *  ListenSoundModel::parseDetectionEventData()
     *  <br>VWU_EVENT_0100 is associated with
     *  VoiceWakeupDetectionEventData structure
     */
    public static final String VWU_EVENT_0100 = "VoiceWakeup_DetectionData_v0100";

    /** This struct is returned to the
     *  IListenEventProcessor::processEvent() by the Listen Engine
     */
    public static class EventData  {
        /** event data returned as payload  */
        public byte  payload[];
        /** size of array */
        public int   size;
    }

    /**
     *  This struct is filled by the application and passed into
     *  ListenVoiceWakeupSession::registerSoundModel()
     */
    public static class SoundModelParams
    {
        /** uffer containing SoundModel */
        public ByteBuffer           soundModelData;
        /** Type of detection to perform:
         *  KEYWORD_ONLY_DETECTION_MODE, USER_KEYWORD_DETECTION_MODE  */
        public int                  detectionMode;
        /** Minimum percent (0-100) confidence level in keyword match
         *  that will trigger DETECT_SUCCESS_EVENT event being sent to the client */
        public short                minKeywordConfidence;
        /** Minimum percent (0-100) confidence level in user match
         *  that will trigger DETECT_SUCCESS_EVENT event being sent to the client  */
        public short                minUserConfidence;
        /** Turns on special notify-all detection mode
         *  <br>Requests that ListenEngine return a DETECT_FAILED_EVENT when minimum
         *  Keyword and User confidence levels are not met */
        public boolean              bFailureNotification;
    }

    /**
     *  This struct is an abstract class inherited by particular types
     *  of Detection event Data
     */
    public static abstract class DetectionData
    {
        /** Status of the parseDetectionEventData() */
        public int             status;
        /**type of Detection Data - includes type number */
        public String          type;
    }
    /**
     *  This struct is filled by
     *  ListenSoundModel::parseDetectionEventData()
     *  when event data is from VoiceWakeup detection event
     */
    public static class VoiceWakeupDetectionData extends DetectionData
    {
        /**Keyword Text as saved in SoundModel */
        public String          keyword;
        /** percent (0-100) confidence that speech matches keyword */
        public short           keywordConfidenceLevel;
        /** percent (0-100) confidence that keyword is spoken by user
         *  who trained sound model */
        public short           userConfidenceLevel;
    }

    /** This struct is returned to as output parameter from
     *  ListenSoundModel::extend()
     */
    public static class ConfidenceData
    {
       /** userMatch  - percent (0-100)
         *        confidence level in user for all recordings is the
         *        same
         */
        public int  userMatch;
    }

}