/*
 *    Copyright (c) 2012-2013 Qualcomm Technologies, Inc. All Rights Reserved.
 *    Qualcomm Technologies Proprietary and Confidential.
 */

package com.qualcomm.listen;

import com.qualcomm.listen.ListenTypes.EventData;
import android.util.Log;

/**
 * ListenReceiver is a Java API to Qualcomm's ListenEngine for
 * setting a callback to receive notification.
 *
 * <p>
 * It is a base class for ListenVoiceWakeupSession and
 * ListenMasterControl Classes. It saves a reference to the
 * applications' implementation of IListenEventProcessor and
 * forwards events from ListenEngine to it.
 */
public class ListenReceiver
{
    protected boolean               bInitialized;
    protected IListenEventProcessor listener;
    private   int                   type;

    // hidden C++ pointer to native client object
    //     this ptr is set when Listen Native Receiver object new'ed
    private int                     nativeClient;

    private final static String TAG = "Java ListenReceiver";

    // load the Listen JNI library
    static {
        Log.d(TAG, "Load liblistenjni");
        System.loadLibrary("listenjni");
    }

    protected static final int UNDEFINED_RECEIVER_TYPE = 0;
    /** Master Control receiver type */
    protected static final int MASTERCONTROL_RECEIVER_TYPE = 1;
    /** VoiceWakeup receiver type */
    protected static final int VWUSESSION_RECEIVER_TYPE = 2;

    protected ListenReceiver()
    {
        listener = null;
        bInitialized = false;
        nativeClient = 0;
        type = UNDEFINED_RECEIVER_TYPE;
    }

    /**
     * Initializes ListenReceiver object.
     *
     * @param receiverType [in] - int type of ListenReceiver
     *        either MASTERCONTROL_RECEIVER_TYPE or
     *        VWUSESSION_RECEIVER_TYPE
     * @return
     *        <br> ERESOURCE_NOT_AVAILABLE - maximum available receivers already instanced
     */
    protected native int init(int receiverType);

  /**
     * Releases ListenReceiver object.
     *
     * @param none
     * @return errors
     */
    protected native int release();


    // ----------------------------------------------------------------------------
    //  Public Methods
    // ----------------------------------------------------------------------------
    /**
     * Sets the callback that implements IListenEventProcessor which will receive
     * events.
     * <p>
     * Stores a reference to the Java application class that implements the
     * IListenEventProcessor processEvent() method that will be used for
     * event callback.
     * Usually this would be same application class that creates a MasterControl
     * or Session object.
     * <p>
     * Setting this callback is mandatory.
     *
     * @param listener [in] - Java App object that implements IListenEventProcessor
     * @return
     *        ERESOURCE_NOT_AVAILABLE - maximum available sessions already instanced
     */
    public int setCallback(IListenEventProcessor listener)
    {
         Log.d(TAG, "Call setCallback");
         if (!bInitialized) {
            Log.e(TAG, "Init was not successfully done");
            return ListenTypes.STATUS_ERESOURCE_NOT_AVAILABLE;
         }
         this.listener = listener;
         return ListenTypes.STATUS_SUCCESS;
    }

     // ----------------------------------------------------------------------------
     //    Callback Mechanism
     // ----------------------------------------------------------------------------
     /**
      * Receives events from Listen Engine and forwards it to
      * IListenEvent.processEvent().
      * <p>
      * This is an internal function that is called only by
      *     Listen Service.
      *
      * @param  eventType [in] - enumerated event type
      * @param  eventData [in] - EventData data structure
      */
     public void receiveEvent( int eventType,
                               EventData eventData )
     {
         Log.d(TAG, "receiveEvent type " + eventType + " calls processEvent");
         listener.processEvent(eventType, eventData);
         return;
     }

}
