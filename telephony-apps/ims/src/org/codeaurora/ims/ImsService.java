/* Copyright (c) 2012 Qualcomm Technologies, Inc.
 * All Rights Reserved. Qualcomm Technologies Confidential and Proprietary.
 *
 * Not a Contribution, Apache license notifications and license are retained
 * for attribution purposes only.
 *
 *
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may21 obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


package org.codeaurora.ims;

import com.android.internal.telephony.CallManager;
import com.android.internal.telephony.CallStateException;
import com.android.internal.telephony.CallTracker;
import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.Connection;
import com.android.internal.telephony.DefaultPhoneNotifier;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneNotifier;
import com.android.internal.telephony.PhoneProxy;
import org.codeaurora.ims.IImsService;
import org.codeaurora.ims.IImsServiceListener;
import com.android.internal.telephony.CallDetails;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.os.Handler;
import android.os.AsyncResult;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.telephony.ServiceState;
import android.util.Log;

import org.codeaurora.ims.IImsServiceListener;

public class ImsService extends Service {
    private static final String LOG_TAG = "IMS Service";
    private static final int SUCCESS = 0;
    private static final int ERROR_UNKNOWN = 1;
    private static final int ERROR_REGISTRATION_MISSING = 2;
    private static final int EVENT_IMS_REG_STATE_CHANGE = 1;
    private static final int EVENT_IMS_REG_STATE_CHANGED = 2;
    private static final int EVENT_IMS_STATE_CHANGE_DONE = 3;
    private static final int EVENT_IMS_SERVICE_STATE_CHANGED = 4;
    private static final int EVENT_IMS_SET_SERVICE_STATUS = 5;
    private static final int EVENT_IMS_GET_SERVICE_STATUS = 6;

    private static final int EVENT_SET_VT_CALL_QUALITY = 7;
    private static final int EVENT_QUERY_VT_CALL_QUALITY = 8;

    protected CallManager mCallManager;

    static public ImsPhone mImsPhone;
    static protected ImsPhoneNotifier sPhoneNotifier;
    static protected ImsSenderRxr sCommandsInterface = null;

    public static final RemoteCallbackList<IImsServiceListener> mListener =
            new RemoteCallbackList<IImsServiceListener>();

    private boolean mIsListenerRegistered = false; // flag to indicate we have
                                                   // at least one callback
                                                   // registration
    private Handler mHandler;

    @Override
    public void onCreate() {
        super.onCreate();
        sPhoneNotifier = new ImsPhoneNotifier();
        makeImsPhone();
        Log.i(LOG_TAG, "IMS Service created");

        mHandler = new Handler() {
            public void handleMessage(Message msg) {
                Log.i(LOG_TAG, "Message received: what = " + msg.what);
                switch (msg.what) {
                    case EVENT_IMS_REG_STATE_CHANGE:
                    case EVENT_IMS_REG_STATE_CHANGED:
                    case EVENT_IMS_STATE_CHANGE_DONE:
                        handleImsRegMsg(msg);
                        break;

                    case EVENT_IMS_SET_SERVICE_STATUS:
                    case EVENT_IMS_GET_SERVICE_STATUS:
                    case EVENT_IMS_SERVICE_STATE_CHANGED:
                        handleServiceStateMsg(msg);
                        break;
                    case EVENT_SET_VT_CALL_QUALITY:
                    case EVENT_QUERY_VT_CALL_QUALITY:
                        forwardMessage(msg);
                        break;


                    default:
                        Log.i(LOG_TAG, "Unknown message = " + msg.what);
                }
            }
        };
        sCommandsInterface.registerForImsNetworkStateChanged(mHandler, EVENT_IMS_REG_STATE_CHANGED,
                null);
        // Query for registration state in case we have missed the UNSOL
        sCommandsInterface.getImsRegistrationState(mHandler
                .obtainMessage(EVENT_IMS_STATE_CHANGE_DONE));
        mImsPhone.registerForServiceStatusChanged(mHandler, EVENT_IMS_SERVICE_STATE_CHANGED, null);
    }

    private void updateBroadcastForServiceState() {
        int i = mListener.beginBroadcast();
        while (i > 0) {
            i--;
            try {
                mListener.getBroadcastItem(i).imsUpdateServiceStatus(
                        ImsQmiIF.CALL_TYPE_VOICE,
                        mImsPhone.mServiceStatus[ImsQmiIF.CALL_TYPE_VOICE].status);
            } catch(RemoteException e) {
                Log.e(LOG_TAG, "Exception for update service status voice = " + e);
            }
            try {
                mListener.getBroadcastItem(i).imsUpdateServiceStatus(
                        ImsQmiIF.CALL_TYPE_VT,
                        mImsPhone.mServiceStatus[ImsQmiIF.CALL_TYPE_VT].status);
            } catch (RemoteException e){
                Log.e(LOG_TAG, "Exception for update service status VT = " + e);
            }
        }
        mListener.finishBroadcast();
    }

    private void handleServiceStateMsg(Message msg) {
        switch (msg.what) {
            case EVENT_IMS_SET_SERVICE_STATUS:
                Log.d(LOG_TAG, "EVENT_IMS_SET_SERVICE_STATUS");
                sendMessage(msg);
                break;

            case EVENT_IMS_GET_SERVICE_STATUS:
                Log.d(LOG_TAG, "EVENT_IMS_GET_SERVICE_STATUS");
                sendMessage(msg);
                updateBroadcastForServiceState();
                break;

            case EVENT_IMS_SERVICE_STATE_CHANGED:
                Log.d(LOG_TAG, "EVENT_IMS_SERVICE_STATE_CHANGED");
                updateBroadcastForServiceState();
                break;
        }
    }

    private void sendMessage(Message msg) {
        AsyncResult arResultSrvSt = (AsyncResult) msg.obj;
        if (arResultSrvSt != null && arResultSrvSt.userObj != null) {
            Message onComplete = (Message) arResultSrvSt.userObj;
            if (msg.replyTo != null) {
                try {
                    Messenger msgr = (Messenger) msg.replyTo;
                    msgr.send(msg);
                } catch (RemoteException e) {
                    Log.e(LOG_TAG, "Exception for IMS Service State Change - " + e);
                }
            } else {
                Log.d(LOG_TAG, "sendMessage replyTo null");
            }
        }
    }

    private void handleImsRegMsg(Message msg) {
        int i = mListener.beginBroadcast();
        while (i > 0) {
            i--;
            Log.i(LOG_TAG, "Message received: what = " + msg.what);
            switch (msg.what) {
                case EVENT_IMS_REG_STATE_CHANGE:
                    AsyncResult aRes = (AsyncResult) msg.obj;
                    if (aRes.exception == null) {
                        Log.d(LOG_TAG, "IMS Registration state change--- SUCCESS");
                    } else {
                        Log.d(LOG_TAG,
                                "IMS Registration state change request--- FAILED");
                        try {
                            mListener.getBroadcastItem(i).imsRegStateChangeReqFailed();
                            Log.e(LOG_TAG, "Hit Callback imsRegStateReqFailed");
                        } catch (Exception e) {
                            Log.e(LOG_TAG, "Exception - " + e);
                        }
                    }
                    break;
                case EVENT_IMS_REG_STATE_CHANGED:
                    sCommandsInterface.getImsRegistrationState(mHandler
                            .obtainMessage(EVENT_IMS_STATE_CHANGE_DONE));
                    break;
                case EVENT_IMS_STATE_CHANGE_DONE:
                    AsyncResult ar = (AsyncResult) msg.obj;
                    if (ar.exception == null && ar.result != null
                            && ((int[]) ar.result).length >= 1) {
                        int[] responseArray = (int[]) ar.result;
                        Log.d(LOG_TAG, "IMS registration state is: " + responseArray[0]);
                        try {
                            if (responseArray[0] == ImsQmiIF.Registration.REGISTERED) {
                                mListener.getBroadcastItem(i).imsRegStateChanged(1);
                            }
                            else {
                                mListener.getBroadcastItem(i).imsRegStateChanged(2);
                            }
                            Log.e(LOG_TAG, "Hit Callback imsRegStateChanged");
                        } catch (Exception e) {
                            Log.e(LOG_TAG, "Registration Failure. Exception - " + e);
                        }
                    } else {
                        Log.e(LOG_TAG, "IMS State query failed!");
                    }
                    break;
            }
        }
        mListener.finishBroadcast();
    }

    @Override
    public void onDestroy() {
        mImsPhone.unregisterForServiceStatusChanged(mHandler);
        mListener.kill();
        return;
    }

    public IBinder onBind(Intent intent) {
        Log.i(LOG_TAG, "Ims Service bound");
        return mBinder;
    }

    private void makeImsPhone() {
        sCommandsInterface = new ImsSenderRxr(this);
        mImsPhone = new ImsPhone(this, sPhoneNotifier, sCommandsInterface);
        mCallManager = CallManager.getInstance();
        boolean ret = mCallManager.registerPhone(mImsPhone);
        Log.i(LOG_TAG, "Ims Phone registered with CallManager ? " + ret);
        return;
    }

    /*
     * Implement the methods of the IImsService interface in this stub
     */
    private final IImsService.Stub mBinder = new IImsService.Stub() {
        public Phone getImsPhone() {
            return mImsPhone;
        }
        public void dial(String number) {
            return ;
        }

        /*
         * Register Callback
         */
        public int registerCallback(IImsServiceListener imsServListener) {
            boolean ret = false;

            if (imsServListener == null) {
                Log.e(LOG_TAG, "Null object passed to register");
                return ERROR_UNKNOWN;
            }

            synchronized (this) {
                ret = mListener.register(imsServListener);

                if (ret == true) {
                    mIsListenerRegistered = true;
                    return SUCCESS;
                } else {
                    Log.e(LOG_TAG, "register failed");
                    return ERROR_UNKNOWN;
                }
            }
        }

        /*
         * Deregister Callback
         */
        public int deregisterCallback(IImsServiceListener imsServListener) {
            boolean ret = false;

            if (imsServListener == null) {
                Log.e(LOG_TAG, "Null object passed to de-register");
                return ERROR_UNKNOWN;
            }

            synchronized (this) {
                ret = mListener.unregister(imsServListener);

                if (ret == true) {
                    mIsListenerRegistered = false;
                    return SUCCESS;
                } else {
                    Log.e(LOG_TAG, "unregister failed");
                    return ERROR_UNKNOWN;
                }
            }
        }

        /*
         * Set IMS Registration state
         */
        public void setRegistrationState(int imsRegState) {
            synchronized (this) {
                if (!mIsListenerRegistered) {
                    Log.i(LOG_TAG, "Error! No callback registered");
                    return;
                }
            }

            Log.i(LOG_TAG, "sendImsRegistrationState with imsRegState = " + imsRegState);
            Message msg = mHandler.obtainMessage(EVENT_IMS_REG_STATE_CHANGE);
            sCommandsInterface.sendImsRegistrationState(imsRegState, msg);
        }

        /*
         * Get IMS Registration state
         */
        public int getRegistrationState()
        {
            int isImsRegistered;
            Log.i(LOG_TAG, "getRegistrationState");
            if (mImsPhone.getServiceState().getState() == ServiceState.STATE_IN_SERVICE)
            {
                isImsRegistered = ImsQmiIF.Registration.REGISTERED;
                Log.i(LOG_TAG, "isImsRegistered=====Registered");
            }
            else
            {
                isImsRegistered = ImsQmiIF.Registration.NOT_REGISTERED;
                Log.i(LOG_TAG, "isImsRegistered=====Deregistered");
            }
            return isImsRegistered;
        }


        /*
         * Set the status of a service for a network type
         */
        public void setServiceStatus(int service, int networkType, int enabled, int restrictCause,
                int event, Messenger msgr) {
            Log.i(LOG_TAG, "setServiceStatus Service =" + service + " NetworkType =" + networkType
                    + "enabled = " + enabled + " restrictCause = " + restrictCause);
            Message msg = mHandler.obtainMessage(EVENT_IMS_SET_SERVICE_STATUS);
            msg.replyTo = msgr;
            mImsPhone.setServiceStatus(msg, service, networkType, enabled, restrictCause);
        }

        /*
         * Query for Status of IMS Services
         */
        public void queryImsServiceStatus(int event, Messenger msgr) {
            Message msg = mHandler.obtainMessage(EVENT_IMS_GET_SERVICE_STATUS);
            msg.replyTo = msgr;
            mImsPhone.queryServiceStatus(msg);
        }

        /**
         * Query for current video call quality.
         */
        @Override
        public void queryVtQuality(Message msg) {
            Log.d(LOG_TAG, "queryVtQuality");
            mImsPhone.queryVideoQuality(mHandler.obtainMessage(EVENT_QUERY_VT_CALL_QUALITY, msg));
        }

        /**
         * Set for current video call quality.
         */
        @Override
        public void setVtQuality(int quality, Message msg) {
            Log.d(LOG_TAG, "setVtQuality qualiy=" + quality);
            mImsPhone.setVideoQuality(quality,
                    mHandler.obtainMessage(EVENT_SET_VT_CALL_QUALITY, msg));
        }
    };

    private void logException(Throwable e) {
        if (e!=null) Log.e(LOG_TAG, "Exception: ", e);
    }

    private void forwardMessage(Message msg) {
        AsyncResult ar = (AsyncResult) msg.obj;
        if (ar==null ) {
            Log.e(LOG_TAG, "forwardMessage, AsyncResult is null. Messsage="+ msg);
        } else if (ar.userObj==null ) {
            Log.e(LOG_TAG, "forwardMessage, AsyncResult.userObj Messsage="+ msg);
        } else {
            Message onComplete = (Message) ar.userObj;
            if (onComplete.replyTo != null) {
                onComplete.arg1 = ar.exception==null ? 0 : 1;
                logException(ar.exception);
                onComplete.obj = ar.result;
                try {
                    onComplete.replyTo.send(onComplete);
                } catch (RemoteException e) {
                    Log.d(LOG_TAG, "forwardMessage, Exception=" + e);
                }
            }
        }
    }
}
