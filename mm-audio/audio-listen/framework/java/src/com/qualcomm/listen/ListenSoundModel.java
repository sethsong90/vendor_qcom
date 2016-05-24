/*
 *	Copyright (c) 2012-2013 Qualcomm Technologies, Inc. All Rights Reserved.
 *	Qualcomm Technologies Proprietary and Confidential.
 */

package com.qualcomm.listen;

import com.qualcomm.listen.ListenTypes.EventData;
import com.qualcomm.listen.ListenTypes.ConfidenceData;
import com.qualcomm.listen.ListenTypes.DetectionData;
import com.qualcomm.listen.ListenTypes.VoiceWakeupDetectionData;
import java.nio.ByteBuffer;
import java.nio.ShortBuffer;
import android.util.Log;

/**
 * ListenSoundModel is a class of helper API to Qualcomm's
 * ListenEngine to be used with SoundModel.
 * <p>
 * No events are generated due to calls to these SoundModel
 * methods so no callback is need for this class.
 */
public class ListenSoundModel {
    private final static String TAG = "ListenSoundModel";

    // load the Listen JNI library
    static {
        Log.d(TAG, "Load liblistenjni");
        System.loadLibrary("listenjni");
    }

    /**
     * Constructor
     */
     public ListenSoundModel() { }


    /**
     * Verifies that User Recording contains the spoken keyword
     * (as defined in the given User-Independent model)
     * and is of good enough quality.
     * <p>
     * A returned confidence level greater than 70 indicates that the
     * phrase in the recording sufficently matches the keyword
     * defined in given model.
     * Such a recording can be can be used for extending a SoundModel.
     *
     * @param userIndependentModel [in] contains User-Independent
     *        model data
     * @param userRecording [in] a single recording of user speaking
     *        keyword
     *
     * @return percent (0-100) confidence level.
     *        Zero returned if error occurs.
     */
     public static native int verifyUserRecording(
            ByteBuffer       userIndependentModel,
            ShortBuffer      userRecording);


    /**
     * Get the total size of bytes required to hold a SoundModel that
     * containing both User-Independent and User-Dependent model
     * data.
     * <p>
     * The size returned by this method must be used to create a
     * ByteBuffer that can hold the extended SoundModel.
     *
     * @param userIndependentModel [in] contains User-Independent
     *        model data.
     *
     * @return total (unsigned) size of extended SoundModel
     *        Zero returned if error occurs.
     */
     public static native int getSizeWhenExtended(
            ByteBuffer userIndependentModel);

    /**
     * Extends a SoundModel by combining UserIndependentModel with
     * UserDependentModel data created from user recordings.
     * <p>
     * Application is responsible for creating a ByteBuffer large
     * enough to hold the SoundModel.
     * <p>
     * At least 5 user recordings should be passed to this method.
     * The more user recordings passed as input, the greater the
     * likelihood of getting a higher quality SoundModel made.
     * <p>
     * Confidence level greater than 70 indicates that user's
     * speech characteristics are sufficently consistent.
     *
     * @param userIndependentModel [in] contains
     *        UserIndependentModel data
     * @param numUserRecordings [in] number of recordings of a
     *        user speaking the keyword
     * @param userRecordings [in] array of N user recordings
     * @param extendedSoundModel [out] extended SoundModel
     * @param confidenceData [out] contains ConfidenceData
     * @return
     *         STATUS_SUCCESS
     *    <br> STATUS_EBAD_PARAM
     */
     public static native int extend(
             ByteBuffer        userIndependentModel,
             int               numUserRecordings,
             ShortBuffer       userRecordings[],
             ByteBuffer        extendedSoundModel,
             ConfidenceData    confidenceData );

     /**
      * Parsers generic Data for detection events from
      * IListenerEventProcessor.processEvent() into more meaningful
      * and returns it.
      * <p>
      * The data structure returned is determined by the type field
      * in DetectionData structure. For example, if vwu_event_0100 is
      * the type then a reference to VoiceWakeupDetectionData is
      * returned.
      *
      * @param registeredSoundModel [in] SoundModel that was used
      *        for registration/detection
      * @param eventPayload [in] event payload returned by
      *        ListenEngine
      *
      * @return reference to DetectionData child object created by
      *        this method
      */
     public static  DetectionData parseDetectionEventData(
              ByteBuffer       registeredSoundModel,
              EventData        eventPayload)
     {
         DetectionData detData;
         int status = ListenTypes.STATUS_SUCCESS;
         Log.d(TAG, "parseDetectionEventData() enter");
         // For this release only VoiceWakeup Detection is supported.
         // Object of type VoiceWakeupDetectionData, which extends
         //    DetectionData class, is created, filled and returned.
         VoiceWakeupDetectionData vwuDetData = new VoiceWakeupDetectionData();
         status = parseVWUDetectionEventData(registeredSoundModel,
                                    eventPayload,
                                    vwuDetData);
         // cast specific data struct to parent abstract class DetectionData
         detData = vwuDetData;
         detData.status = status;
         Log.d(TAG, "parseDetectionEventData() returns detectionData ptr "
               + detData + " status " + status);
         return detData;
    }
     /**
      * Parsers generic Data for detection events from
      * IListenerEventProcessor.processEvent() into more meaningful
      * and returns it.
      * <p>
      * The data structure returned is determined by the type field
      * in DetectionData structure. For example, if vwu_event_0100 is
      * the type then a reference to VoiceWakeupDetectionData is
      * returned.
      *
      * @param registeredSoundModel [in] SoundModel that was used
      *        for registration/detection
      * @param eventPayload [in] event payload returned by
      *        ListenEngine
      * @param vwuDetData [in/out] VoiceWakeupDetectionData
      *        structure filled when eventPayload is parsed
      * @return
      *        <br> STATUS_SUCCESS
      *        <br> STATUS_EBAD_PARAM
      */
     private static native int parseVWUDetectionEventData(
                 ByteBuffer               registeredSoundModel,
                 EventData                eventPayload,
                 VoiceWakeupDetectionData vwuDetData);
}
