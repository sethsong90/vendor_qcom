/*
 *    Copyright (c) 2012-2013 Qualcomm Technologies, Inc. All Rights Reserved.
 *    Qualcomm Technologies Proprietary and Confidential.
 */

package com.qualcomm.listen;

import java.lang.ref.WeakReference;
import com.qualcomm.listen.ListenTypes.SoundModelParams;
import android.util.Log;

/**
 * ListenVoiceWakeupSession is a Java API to Qualcomm's
 * ListenEngine used for keyword detection. <br>
 *
 * <br>
 * ListenVoiceWakeupSession extends Listen Receiver to allow a
 * SoundModel to be registered.
 * Each ListenVoiceWakeupSession is associated with a SoundModel. <br>
 *
 * <br>
 * Global parameters can be queried but can not be changed via
 * a Session object. <br>
 *
 * <br>
 * Events received by application using this class are:<br>
 *  LISTEN_FEATURE_DISABLE_EVENT <br>
 *  LISTEN_FEATURE_ENABLE_EVENT <br>
 *  VOICEWAKEUP_FEATURE_DISABLE_EVENT <br>
 *  VOICEWAKEUP_FEATURE_ENABLE_EVENT <br>
 *  DETECT_SUCCESS_EVENT <br>
 *  DETECT_FAILED_EVENT <br>
 *  DEREGISTERED_EVENT <br>
 *  LISTEN_RUNNING_EVENT <br>
 *  LISTEN_STOPPED_EVENT <br>
 *  LISTEN_ENGINE_DIED <br>
 */

public class ListenVoiceWakeupSession extends ListenReceiver {
   private final static String TAG = "ListenVoiceWakeupSession";

   private ListenVoiceWakeupSession()
   {
       // no member variables to initialize
   }

   /**
    * Creates an instance of ListenVoiceWakeupSession.
    * <p>
    * Only one session can be created at a time.
    *
    * @return an instance of ListenVoiceWakeupSession or null if a
    *         session is already created.
    */
   public synchronized static ListenVoiceWakeupSession createInstance()
   {
       int status;
       ListenVoiceWakeupSession session = null;
       // assuming since session is return value that is assigned to variable
       //    in calling method, that it will not be garbage collected
       session = new ListenVoiceWakeupSession();
       if (null == session) {
           Log.e(TAG, "new ListenVoiceWakeupSession failed");
           return null;
       }
       // initialize the Listen Native Receiver object
       status = session.init(ListenReceiver.VWUSESSION_RECEIVER_TYPE);
       if (ListenTypes.STATUS_SUCCESS == status) {
           session.bInitialized = true;
       } else {
           Log.e(TAG, "session init returned " + status);
           session.bInitialized = false;
       }
       if (!session.bInitialized) {
           // Session object could be not be acquired from ListenEngine
           Log.e(TAG, "ListenVoiceWakeupSession could not be initialized");
           session = null;
       }
       return session;
   }

   /**
    * Releases an instance of ListenVoiceWakeupSession.
    *
    * @return
    *          STATUS_SUCCESS
    *     <br> STATUS_EFAILURE
    */
   public int releaseInstance()
   {
       int status;
       status = release(); // release instance from ListenEngine
       return status;
   }

   /**
    * Gets a Listen Parameter, used to query the status of Listen
    * and/or VoiceWakeup feature.
    *
    * @param paramType [in] string describing parameter type
    * 		 "ListenFeature" or "VoiceWakeupFeature"
    *
    * @return current value of parameter: "enable" or "disable"
    *         <br> Status is not returned
    */
   public native String getParam(String paramType);

    /**
     * Registers a SoundModel that the application wants
     *     the ListenEngine to use for detection.
     *
     * @param  soundModelParams [in] data structure containing required parameters
     *
     * @return
     *              STATUS_SUCCESS
     *         <br> STATUS_EFAILURE
     *         <br> STATUS_EFEATURE_NOT_ENABLED
     *         <br> STATUS_ESOUNDMODEL_ALREADY_REGISTERED
     */
    public native int registerSoundModel(
            SoundModelParams soundModelParams);
    /**
     * Deregisters a SoundModel.
     *
     * @return
     *              STATUS_SUCCESS
     *        <br> STATUS_EFAILURE
     *        <br> STATUS_ESOUNDMODEL_NOT_REGISTERED
     */
    public native int deregisterSoundModel() ;

}

