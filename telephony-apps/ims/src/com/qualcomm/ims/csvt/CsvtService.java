/*
 * Copyright (c) 2014 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Qualcomm Technologies Confidential and Proprietary.
 *
 * Not a Contribution.
 * Apache license notifications and license are retained
 * for attribution purposes only.
 *
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.qualcomm.ims.csvt;


import android.app.Service;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.Registrant;
import android.os.ServiceManager;
import android.os.SystemProperties;
import android.telephony.MSimTelephonyManager;
import android.telephony.PhoneStateListener;
import android.telephony.ServiceState;
import android.telephony.PhoneNumberUtils;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Config;
import android.util.Log;
import android.provider.Settings;
import android.provider.SyncStateContract.Constants;
import android.content.Context;
import android.os.RemoteCallbackList;
import android.os.RemoteException;

import com.android.internal.telephony.Call;
import com.android.internal.telephony.CallDetails;
import com.android.internal.telephony.CallForwardInfo;
import com.android.internal.telephony.CallManager;
import com.android.internal.telephony.CallStateException;
import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.Connection;
import com.android.internal.telephony.TelephonyProperties;
import com.android.internal.telephony.Connection.DisconnectCause;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.uicc.UiccController;

import org.codeaurora.ims.CallFailCause;
import org.codeaurora.ims.IImsServiceListener;
import org.codeaurora.ims.IImsService;
import org.codeaurora.ims.ImsConnection;
import org.codeaurora.ims.ImsPhone;
import org.codeaurora.ims.ImsService;

import com.qualcomm.ims.csvt.CsvtConstants;
import com.qualcomm.ims.csvt.CsvtIntents;
import com.qualcomm.ims.csvt.CsvtUtils;

import org.codeaurora.ims.csvt.ICsvtServiceListener;
import org.codeaurora.ims.csvt.ICsvtService;
import org.codeaurora.ims.csvt.CallForwardInfoP;

import java.util.ArrayList;
import java.util.List;


public class CsvtService extends Service {

    private static final boolean DBG = true; //Config.DEBUG;
    private static final String TAG = "CsvtService";

    final private RemoteCallbackList<ICsvtServiceListener> mListeners =
            new RemoteCallbackList<ICsvtServiceListener>();

    private static ImsPhone mPhone;
    private static IImsService mImsService;
    private static IImsServiceListener mImsListener;
    private static CallManager mCm;
    private static Call.State mLastCallState = Call.State.IDLE;

    private Connection mConnection;

    private Handler mHandler;

    // Note: mTelephonyManager or mMsimTelephonyManager will be null.
    TelephonyManager mTelephonyManager;
    MSimTelephonyManager mMsimTelephonyManager;
    final private List<PhoneStateListener> mTelephonyListeners =
            new ArrayList<PhoneStateListener>();
    final private List<Integer> mPhonesStates =
            new ArrayList<Integer>();

    private static final int EVENT_IMS_REG_STATE_CHANGED = 1;
    private static final int EVENT_PRECISE_CALL_STATE_CHANGED = 2;
    private static final int EVENT_RINGBACK_TONE = 3;
    private static final int EVENT_NEW_RINGING_CONNECTION= 4;
    private static final int EVENT_DISCONNECTED = 5;
    private static final int EVENT_SET_CALL_WAITING = 6;
    private static final int EVENT_SET_CALL_FORWARDING = 7;
    private static final int EVENT_GET_CALL_WAITING = 8;
    private static final int EVENT_GET_CALL_FORWARDING = 9;

    @Override
    public void onCreate() {
        bindImsService();

        mCm = CallManager.getInstance();
        createHandler();
        acquireTelephonyManager();
        registerForTelephonyEvents();
        Log.d(TAG, "onCreate: Initialization is completed");
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.i(TAG, "Csvt Service bound");
        return mBinder;
    }

    private void acquireTelephonyManager() {
        Log.d(TAG, "acquireTelephonyManager");

        mMsimTelephonyManager = (MSimTelephonyManager)
                getSystemService(MSIM_TELEPHONY_SERVICE);

        if (mMsimTelephonyManager == null ||
                (!mMsimTelephonyManager.isMultiSimEnabled())) {
            mMsimTelephonyManager = null;
            mTelephonyManager = (TelephonyManager)
                    getSystemService(TELEPHONY_SERVICE);
            if (mTelephonyManager == null) {
                Log.e(TAG, "Failed to acquire TelephonyManager");
            }
        }
    }

    private PhoneStateListener createPhoneStateListener(int subscription) {
        return new PhoneStateListener(subscription) {
            @Override
            public void onCallStateChanged(int s, String in) {
                if ( DBG ) Log.d(TAG, "PhoneStateListener: CallState: " + s
                        + " Incomming Number: " + in );
                if ( (mSubscription >=0) &&
                        (mSubscription < mPhonesStates.size())) {
                    mPhonesStates.set(mSubscription, s );
                } else {
                    Log.e(TAG, "PhoneStateListener: Error: subscription: "
                + mSubscription + " PhonesStates: " + mPhonesStates);
                    }
                }
        };
    }

    private void registerForTelephonyEvents() {

        if (mMsimTelephonyManager != null) {
            for (int i = 0; i < mMsimTelephonyManager.getPhoneCount(); i++) {
                mPhonesStates.add(TelephonyManager.CALL_STATE_IDLE);
                PhoneStateListener l = createPhoneStateListener(i);
                mTelephonyListeners.add(l);
                mMsimTelephonyManager.listen(l,
                        PhoneStateListener.LISTEN_CALL_STATE);
            }
        } else if (mTelephonyManager != null) {
            mPhonesStates.add(TelephonyManager.CALL_STATE_IDLE);
            PhoneStateListener l = createPhoneStateListener(0);
            mTelephonyListeners.add(l);
            mTelephonyManager.listen(l, PhoneStateListener.LISTEN_CALL_STATE);
        } else {
            Log.e(TAG, "Failed to obtain TelephonyManager");
        }
    }

    private void unregisterFromTelephonyEvents() {

        if (mMsimTelephonyManager != null) {
            for (PhoneStateListener l : mTelephonyListeners) {
                mMsimTelephonyManager.listen(l, PhoneStateListener.LISTEN_NONE);
            }
        } else if (mTelephonyManager != null) {
            for (PhoneStateListener l : mTelephonyListeners) {
                mTelephonyManager.listen(l, PhoneStateListener.LISTEN_NONE);
            }
        }
        mTelephonyListeners.clear();
        mPhonesStates.clear();
    }

    private void createHandler() {
        mHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                CsvtService.this.handleMessage(msg);
            }
        };
    }

    private void registerForPhoneEvents() {
        if (mPhone == null) {
            Log.e(TAG,"Failed to register for events.");
            return;
        }
        mPhone.registerForCsvtPreciseCallStateChanged(mHandler,
                EVENT_PRECISE_CALL_STATE_CHANGED, null);
        mPhone.registerForCsvtNewRingingConnection(mHandler, EVENT_NEW_RINGING_CONNECTION
                , null);
        mPhone.registerForCsvtDisconnect(mHandler, EVENT_DISCONNECTED, null);
    }

    private void bindImsService() {
        try {
            // send intent to start ims service n get phone from ims service
            boolean bound = bindService(new Intent("org.codeaurora.ims.IImsService"),
                    ImsServiceConnection, Context.BIND_AUTO_CREATE);
            Log.d(TAG, "IMSService bound request : " + bound);
        } catch (NoClassDefFoundError e) {
            Log.w(TAG, "Ignoring IMS class not found exception " + e);
        }
    }

    private void registerForImsEvents() {
        if (mImsListener != null) {
            Log.e(TAG, "Already registered for Ims Events");
            unregisterFromImsEvents();
        }
        if (mImsService == null) {
            Log.e(TAG, "Not bound to Ims Service. Listener registration failed.");
            return;
        }
        mImsListener = new IImsServiceListener.Stub() {
            public void imsRegStateChanged(int state) {
                Log.d(TAG, "IMS state changed:" + state);
            }

            public void imsRegStateChangeReqFailed() {
                Log.w(TAG, "IMS state changed request failed.");
            }

            public void imsUpdateServiceStatus(int service, int status) {
                Log.d(TAG, "IMS update service status: " + status + "service: " + service);
            }
        };
        try {
            if (mImsService.registerCallback(mImsListener) != 0) {
                Log.e(TAG, "Listener registration failed.");
                mImsListener = null;
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Listener registration failed." + e);
            mImsListener = null;
        }
    }

    private void unregisterFromImsEvents() {
        if (mImsService == null) {
            Log.e(TAG, "Not bound to Ims Service. Listener registration failed.");
            return;
        }
        if (mImsListener != null) {
            try {
                mImsService.deregisterCallback(mImsListener);
            } catch (RemoteException e) {
                Log.e(TAG, "Listener unregistration failed." + e);
            }
            mImsListener = null;
        }
    }

    private static ImsPhone getImsPhone() {
        for (Phone phone : mCm.getAllPhones()) {
            if (phone.getPhoneType() == PhoneConstants.PHONE_TYPE_IMS) {
                return (ImsPhone) phone;
            }
        }
        return null;
    }

    private ServiceConnection ImsServiceConnection = new ServiceConnection() {

        public void onServiceConnected(ComponentName name, IBinder service) {
            mImsService = IImsService.Stub.asInterface(service);
            Log.d(TAG,"Ims Service Connected: " + mImsService);
            mPhone = getImsPhone();
            if ( mPhone == null ) {
                Log.e(TAG,"IMS Phone not found.");
                return;
            }

            // Force IMS Service to ServiceState.STATE_IN_SERVICE.
            mPhone.setIgnoreSerivceStateFlag(true);

            registerForImsEvents();
            registerForPhoneEvents();
        }

        public void onServiceDisconnected(ComponentName arg0) {
            Log.w(TAG,"Ims Service onServiceDisconnected");
            mPhone = null;
            mImsService = null;
        }
    };

    void handleMessage(Message msg) {
        switch ( msg.what ) {
            case EVENT_PRECISE_CALL_STATE_CHANGED:
                onPreciseCallStateChanged(msg);
                break;
            case EVENT_NEW_RINGING_CONNECTION:
                onNewRingingConnection(msg);
                break;
            case EVENT_DISCONNECTED:
                onDisconnect(msg);
                break;
            case EVENT_SET_CALL_WAITING:
                onSetCallWaiting(msg);
                break;
            case EVENT_SET_CALL_FORWARDING:
                onSetCallForwarding(msg);
                break;
            case EVENT_GET_CALL_WAITING:
                onGetCallWaiting(msg);
                break;
            case EVENT_GET_CALL_FORWARDING:
                onGetCallForwarding(msg);
                break;
        }

    }

    private void onPreciseCallStateChanged(Message msg) {
        if (DBG) Log.d(TAG, "onPreciseCallStateChanged");

        int state = CsvtConstants.CALL_STATE_OFFHOOK;
        Call call = mPhone.getForegroundCall();

        PhoneConstants.State ps = mPhone.getState();
        if (DBG)  Log.v(TAG, "Phone state:" + ps);

        if (ps == PhoneConstants.State.IDLE) {
            mConnection = null;
            state = CsvtConstants.CALL_STATE_IDLE;
        } else if (ps == PhoneConstants.State.RINGING ) {
            state = CsvtConstants.CALL_STATE_RINGING;
        } else {
            state = CsvtConstants.CALL_STATE_OFFHOOK;

            Connection c = call.getEarliestConnection();
            if (DBG) Log.v(TAG, "ForegroundCall State = " + call.getState() );
            if ( CsvtUtils.isCsvtConnection(c) && call.getState() == Call.State.ACTIVE) {
                notifyCallStatus(CsvtConstants.CALL_STATUS_CONNECTED);
            }
        }

        // Temporary fix to enable ring-back tone
        Call.State callState = call.getState();
        boolean isAlerting = (callState == Call.State.ALERTING);
        if (isAlerting || mLastCallState == Call.State.ALERTING) {
            onRingbackTone(isAlerting);
        }
        mLastCallState = callState;

        notifyPhoneStateChanged(state);
    }

    private void onNewRingingConnection(Message msg) {
        AsyncResult ar = (AsyncResult) msg.obj;
        mConnection = (Connection) ar.result;

        Intent i = new Intent(CsvtIntents.ACTION_NEW_CSVT_RINGING_CONNECTION);
        i.putExtra(CsvtConstants.CONNECTION_ADDRESS_KEY, mConnection.getAddress());
        mPhone.getContext().sendBroadcast(i);
    }

    private void onRingbackTone(final boolean playTone) {
        if (DBG) Log.d(TAG, "onRingbackTone");

        notifyListeners(new INotifyEvent() {
            @Override
            public void onNotify(ICsvtServiceListener l) throws RemoteException {
                l.onRingbackTone(playTone);
            }
        });
    }

    private void onSetCallWaiting(Message msg) {
        Log.d(TAG, "OnSetCallWaiting:");

        AsyncResult ar = (AsyncResult) msg.obj;
        if (ar.exception != null) {
            Log.d(TAG, "SetCallWaiting: Exception: " + ar.exception);
        }

        sendRequestStatus(ar);
    }

    private void onGetCallWaiting(Message msg) {
        Log.d(TAG, "OnGetCallWaiting:");

        AsyncResult ar = (AsyncResult) msg.obj;

        sendRequestStatus(ar);

        if (ar.exception != null) {
            Log.d(TAG, "GetCallWaiting: Exception: " + ar.exception);
        } else {
            int[] r = (int[]) ar.result;
            final boolean enabled = r[0]==1 &&
                    (r[1] & CommandsInterface.SERVICE_CLASS_DATA_SYNC) != 0;
            if (DBG) Log.d(TAG, "GetCallWaiting: Enabled: " + enabled);

            notifyListeners( new INotifyEvent() {
                @Override
                public void onNotify(ICsvtServiceListener l)throws RemoteException {
                    l.onCallWaiting(enabled);
                }
            });
        }
    }

    private void onSetCallForwarding(Message msg) {
        Log.d(TAG, "onSetCallForwarding:");

        AsyncResult ar = (AsyncResult) msg.obj;
        if (ar.exception != null) {
            Log.d(TAG, "onSetCallForwarding: Exception: " + ar.exception);
        }

        sendRequestStatus(ar);

    }

    private void sendRequestStatus(AsyncResult ar) {
        Message uo = (Message) ar.userObj;
        if (uo != null && (uo.replyTo instanceof Messenger) ) {
            uo.arg1 = (ar.exception == null) ? CsvtConstants.ERROR_SUCCESS :
                CsvtConstants.ERROR_FAILED;
            try {
                uo.replyTo.send(uo);
            } catch (RemoteException e) {
                Log.e(TAG, "Reply failed, " + e);
            }
        }
    }

    private void onGetCallForwarding(Message msg) {
        Log.d(TAG, "onGetCallForwarding:");

        AsyncResult ar = (AsyncResult) msg.obj;

        sendRequestStatus(ar);

        if (ar.exception != null) {
            Log.d(TAG, "SetCallForwarding: Exception: " + ar.exception);
        } else {
            CallForwardInfo cfInfoArray[] = (CallForwardInfo[]) ar.result;
            if (cfInfoArray.length == 0) {
                Log.d(TAG, "handleGetCFResponse: cfInfoArray.length==0");
                notifyCallForwardingOptions( new ArrayList<CallForwardInfoP>() );
            } else {
                ArrayList<CallForwardInfoP> arr = new ArrayList<CallForwardInfoP>();
                for (int i = 0, length = cfInfoArray.length; i < length; i++) {
                    Log.d(TAG, "handleGetCFResponse, cfInfoArray[" + i + "]="
                            + cfInfoArray[i]);
                    CallForwardInfoP cf = new CallForwardInfoP();
                    cf.status = cfInfoArray[i].status;
                    cf.reason = cfInfoArray[i].reason;
                    cf.toa = cfInfoArray[i].toa;
                    cf.number = cfInfoArray[i].number;
                    cf.timeSeconds = cfInfoArray[i].timeSeconds;
                    cf.serviceClass = cfInfoArray[i].serviceClass;
                    arr.add(cf);

                }
                notifyCallForwardingOptions(arr);
            }
        }
    }

    private int fromCallFailCause( int cause) {
        switch(cause) {
            case CallFailCause.LOCAL_PHONE_OUT_OF_3G_Service:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_LCOAL_PHONE_OUT_OF_3G_SERVICE;
            case CallFailCause.USER_ALERTING_NO_ANSWER:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_NO_ANSWER;
            case CallFailCause.INCOMPATIBILITY_DESTINATION:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_INCOMPATIBILITY_DESTINATION;
            case CallFailCause.RESOURCES_UNAVAILABLE:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_RESOURCES_UNAVAILABLE;
            case CallFailCause.BEARER_NOT_AUTHORIZATION:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_BEARER_NOT_AUTHORIZATION;
            case CallFailCause.BEARER_NOT_AVAIL:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_BEARER_NOT_AVAIL;
            case CallFailCause.NUMBER_CHANGED:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_NUMBER_CHANGED;
            case CallFailCause.NORMAL_UNSPECIFIED:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_NORMAL_UNSPECIFIED;
            case CallFailCause.PROTOCOL_ERROR_UNSPECIFIED:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_PROTOCOL_ERROR_UNSPECIFIED;
            case CallFailCause.BEARER_SERVICE_NOT_IMPLEMENTED:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_BEARER_SERVICE_NOT_IMPLEMENTED;
            case CallFailCause.SERVICE_OR_OPTION_NOT_IMPLEMENTED:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_SERVICE_OR_OPTION_NOT_IMPLEMENTED;
            case CallFailCause.NO_USER_RESPONDING:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_NO_USER_RESPONDING;
            case CallFailCause.NORMAL_CLEARING:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_NORMAL;
            case CallFailCause.USER_BUSY:
            case CallFailCause.CALL_REJECTED:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_BUSY;
            case CallFailCause.INVALID_NUMBER:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_INVALID_NUMBER;
            case CallFailCause.NO_CIRCUIT_AVAIL:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_NETWORK_CONGESTION;
            case CallFailCause.UNOBTAINABLE_NUMBER:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_UNASSIGNED_NUMBER;

        }
        return CsvtConstants.CALL_STATUS_DISCONNECTED_ERROR_UNSPECIFIED;
    }

    private int fromDisconnectCause(Connection.DisconnectCause cause) {
        switch (cause) {
            case INCOMING_MISSED:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_INCOMING_MISSED;
            case NORMAL:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_NORMAL;
            case BUSY:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_BUSY;
            case INVALID_NUMBER:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_INVALID_NUMBER;
            case INCOMING_REJECTED:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_INCOMING_REJECTED;
            case POWER_OFF:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_POWER_OFF;
            case OUT_OF_SERVICE:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_OUT_OF_SERVICE;
            case CONGESTION:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_NETWORK_CONGESTION;
            case LOST_SIGNAL:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_LOST_SIGNAL;
            case UNOBTAINABLE_NUMBER:
                return CsvtConstants.CALL_STATUS_DISCONNECTED_UNASSIGNED_NUMBER;
        }
        return CsvtConstants.CALL_STATE_DISCONNECTED;
    }

    private int getDisconnectCause(ImsConnection c) {
        int dc = CsvtConstants.CALL_STATUS_DISCONNECTED_ERROR_UNSPECIFIED;

        if ( c == null ) {
            Log.e(TAG, "Disconnect Cause: null connection.");
            return dc;
        }

        // If disconnectInfo is available try to convert it to Csvt Call Fail Cause.
        String di = c.getImsDisconnectCauseInfo();
        if ( di != null && ! di.isEmpty() ) {
            try {
                dc = fromCallFailCause( Integer.parseInt(di) );
            } catch (NumberFormatException e) {
                Log.w(TAG, "Fail to parse disconnect info: " + di);
            }
        } else {
            dc = fromDisconnectCause( c.getDisconnectCause() );
        }
        return dc;

    }

    private void onDisconnect(Message msg)
    {
        AsyncResult ar = (AsyncResult)msg.obj;
        ImsConnection c = (ImsConnection) ar.result;

        if ( c == null ) {
            if (DBG) Log.d(TAG, "onDisconnect: Connection object is null.");

            if (mConnection != null) {
                notifyCallStatus(CsvtConstants.CALL_STATE_DISCONNECTED);
                mConnection = null;
            }
            return;
        }

        Connection.DisconnectCause cause = c.getDisconnectCause();
        String disconnectInfo = c.getImsDisconnectCauseInfo();

        if (DBG) Log.d(TAG, "DisconnectCause is " + cause + " DisconnectInfo is"
                        + disconnectInfo);

        notifyCallStatus( getDisconnectCause(c) );
        mConnection = null;
    }

    private void notifyError(int code) {
        notifyCallStatus(code);
    }

    void dial(String number) {
        if(DBG) Log.d(TAG,"dial: " + number);

        final boolean nonCsvtIdle = isNonCsvtIdle();
        final boolean csvtIdle = isIdle();
        if ( ! (nonCsvtIdle || csvtIdle) ) {
            Log.e(TAG, "Cannot dial: "
                    + "nonCsvtIdle: " + nonCsvtIdle
                    + "csvtIdle: " + csvtIdle );
            notifyError(CsvtConstants.DIAL_FAILED);
            return;
        }

        try {
            mConnection = mPhone.dial(number, CallDetails.CALL_TYPE_VT,
                    CallDetails.CALL_DOMAIN_CS, null);
        } catch (Exception ex) {
            notifyError(CsvtConstants.DIAL_FAILED);
            Log.e(TAG, "Dial failed: " + ex);
        }

        if(mConnection == null) {
            if(DBG) Log.e(TAG,"Connection is null after dial");
            notifyError(CsvtConstants.DIAL_FAILED);
        }
    }

    void hangup() {
        if (DBG) Log.v(TAG, "hangup connection: " + mConnection);

        if (mConnection == null) {
            Log.w(TAG, "hangup failed. connection is null");
            return;
        }

        try {
            mConnection.hangup();
        } catch (Exception e) {
            notifyError(CsvtConstants.HANGUP_FAILED);
            Log.e(TAG, "hangup failed. " + e);
        }
    }

    void rejectCall() {
        if(DBG) Log.v(TAG,"Reject call");

        if ( ! CsvtUtils.hasActiveRingingCsvtConnection(mPhone) ) {
            Log.w(TAG, "Reject call failed: Phone not ringing.");
            return;
        }

        try {
            mPhone.rejectCall(CallFailCause.USER_BUSY);
        } catch (Exception e) {
            notifyError(CsvtConstants.REJECT_CALL_FAILED);
            Log.e(TAG, "Reject call failed. " + e);
        }
    }

    void acceptCall() {
        if(DBG) Log.d(TAG,"acceptCall");

        try {
            if ( CsvtUtils.hasActiveFgCsvtConnection( mPhone) ) {
                if(DBG) Log.d(TAG, "Disconnecting active call,"
                        + " before accepting the ringing call");
                hangup();
            }
            // TODO Pass the domain information as well.
            mPhone.acceptCall(CallDetails.CALL_TYPE_VT);
        } catch(Exception e) {
            Log.e(TAG, "acceptCall failed. " + e);
            notifyError(CsvtConstants.ACCEPT_CALL_FAILED);
        }
    }

    public void registerListener(ICsvtServiceListener l){
        if(DBG) Log.v(TAG, "Registering listener l = " + l);
        if ( l == null ) {
            Log.e(TAG, "Listener registration failed. Listener is null");
            return;
        }

        synchronized (this) {
            mListeners.register(l);
        }
    }

    public void fallback() {
        if ( CsvtUtils.hasActiveRingingCsvtConnection(mPhone) ) {
            try {
                mPhone.rejectCall(CallFailCause.INCOMPATIBILITY_DESTINATION);
            } catch (CallStateException e) {
                Log.e(TAG, "Fallback failed: " + e);
                notifyError(CsvtConstants.FALLBACK_FAILED);
            }
        } else {
            Log.w(TAG, "Fallback failed: phone not ringing");
            notifyError(CsvtConstants.FALLBACK_FAILED);
        }
    }

    public  void unregisterListener(ICsvtServiceListener l){
        if(DBG) Log.v(TAG, "Unregistering listener l = " + l);
        if ( l == null ) {
            Log.e(TAG, "Listener unregistration failed. Listener is null");
            return;
        }

        synchronized (this) {
            mListeners.unregister(l);
        }
    }

    private void notifyCallForwardingOptions(final List<CallForwardInfoP> cfl ) {
        notifyListeners(new INotifyEvent() {
            @Override
            public void onNotify(ICsvtServiceListener l) throws RemoteException {
                l.onCallForwardingOptions(cfl);
            }
        });
    }

    private void notifyPhoneStateChanged(final int state) {
        notifyListeners(new INotifyEvent() {
            @Override
            public void onNotify(ICsvtServiceListener l) throws RemoteException {
                l.onPhoneStateChanged(state);
            }
        });
    }

    void notifyCallStatus(final int status) {
        notifyListeners(new INotifyEvent() {
            @Override
            public void onNotify(ICsvtServiceListener l) throws RemoteException {
                l.onCallStatus(status);
            }
        });
    }

    private interface INotifyEvent {
        void onNotify(ICsvtServiceListener l) throws RemoteException;
    }

    void notifyListeners(INotifyEvent n) {
        synchronized (this) {
            mListeners.beginBroadcast();
            final int size = mListeners.getRegisteredCallbackCount();
            for (int i = 0; i < size; ++i) {
                try {
                    n.onNotify( mListeners.getBroadcastItem(i) );
                } catch (RemoteException e) {
                    Log.e(TAG, "Broadcast failed. idx: " + i + " Ex: " + e);
                }
            }
            mListeners.finishBroadcast();
        }
    }

    public boolean isIdle() {
        return ! ( CsvtUtils.hasActiveFgCsvtConnection(mPhone) ||
                   CsvtUtils.hasActiveRingingCsvtConnection(mPhone) );
    }

    public boolean isActive() {
        return CsvtUtils.hasActiveFgCsvtConnection(mPhone);
    }

    public boolean isNonCsvtIdle() {
        boolean isIdle = true;
        for (int callState: mPhonesStates) {
            if (callState != TelephonyManager.CALL_STATE_IDLE) {
                isIdle = false;
                break;
            }
        }
        return isIdle;
    }


    public void setCallForwardingOption(int commandInterfaceCFReason,
            int commandInterfaceCFAction,
            String dialingNumber,
            int timerSeconds,
            Message onComplete) {
        mPhone.setCallForwardingOptionForData(commandInterfaceCFReason,
                commandInterfaceCFAction,
                dialingNumber,
                timerSeconds,
                mHandler.obtainMessage(EVENT_SET_CALL_FORWARDING, onComplete));
    }

    public void getCallForwardingOption(int commandInterfaceCFReason,
                Message onComplete) {
        mPhone.getCallForwardingOptionForData( commandInterfaceCFReason,
                mHandler.obtainMessage(EVENT_GET_CALL_FORWARDING, onComplete));
    }


    public void setCallWaiting(boolean enable, Message onComplete) {
        mPhone.setCallWaitingForData(enable,
                mHandler.obtainMessage(EVENT_SET_CALL_WAITING, onComplete));
    }

    public void getCallWaiting(Message onComplete) {
        mPhone.getCallWaitingForData(
                mHandler.obtainMessage(EVENT_GET_CALL_WAITING, onComplete) );
    }


    final private ICsvtService.Stub mBinder = new ICsvtService.Stub() {

        /**
         * Initiate a new Csvt connection. This happens asynchronously, so you
         * cannot assume the audio path is connected (or a call index has been
         * assigned) until PhoneStateChanged notification has occurred.
         */
        public void dial(String number) {
            CsvtService.this.dial(number);
        }

        /**
         * Hang up the foreground call. Reject occurs asynchronously,
         * and final notification occurs via PhoneStateChanged callback.
         */
        public void hangup() {
            CsvtService.this.hangup();
        }

        /**
         * Answers a ringing.
         * Answering occurs asynchronously, and final notification occurs via
         * PhoneStateChanged callback.
         */
        public void acceptCall() {
            CsvtService.this.acceptCall();
        }

        /**
         * Reject (ignore) a ringing call. In GSM, this means UDUB
         * (User Determined User Busy). Reject occurs asynchronously,
         * and final notification occurs via  PhoneStateChanged callback.
         */
        public void rejectCall() {
            CsvtService.this.rejectCall();
        }

        /**
         * Reject (ignore) a ringing call and sends Incompatible Destination
         * fail cause to the remote party. Reject occurs asynchronously,
         * and final notification occurs via  PhoneStateChanged callback.
         */
        public void fallBack() {
            CsvtService.this.fallback();
        }

        /**
         * Checks if there is an active or ringing Csvt call.
         * @return false if there is an active or ringing Csvt call.
         */
        public boolean isIdle() {
            return CsvtService.this.isIdle();
        }

        /**
         * Checks if there is an active Csvt call.
         * @return true if there is an active Csvt call.
         */
        public boolean isActive() {
            return CsvtService.this.isActive();
        }


        /**
         * Checks if all non-Csvt calls are idle.
         * @return true if all non-Csvt calls are idle.
         */
        public boolean isNonCsvtIdle() {
            return CsvtService.this.isNonCsvtIdle();
        }

        /**
         * getCallForwardingOptions
         * Call Forwarding options are returned via
         * ICsvtServiceListener.onCallForwardingOptions callback.
         *
         * @param commandInterfaceCFReason is one of the valid call forwarding
         *        CF_REASONS, as defined in
         *        <code>com.android.internal.telephony.CommandsInterface.</code>
         * @param onComplete a callback message when the action is completed.
         *        onComplete.arg1 is set to zero (0) if the action is completed
         *        successfully.
         * @see   ICsvtServiceListener.onCallForwardingOptions
         */
        public void getCallForwardingOption(int commandInterfaceCFReason,
                                     Message onComplete) {
            CsvtService.this.getCallForwardingOption(commandInterfaceCFReason
                    ,onComplete);
        }

        /**
         * setCallForwardingOptions
         * sets a call forwarding option.
         *
         * @param commandInterfaceCFReason is one of the valid call forwarding
         *        CF_REASONS, as defined in
         *        <code>com.android.internal.telephony.CommandsInterface.</code>
         * @param commandInterfaceCFAction is one of the valid call forwarding
         *        CF_ACTIONS, as defined in
         *        <code>com.android.internal.telephony.CommandsInterface.</code>
         * @param dialingNumber is the target phone number to forward calls to
         * @param timerSeconds is used by CFNRy to indicate the timeout before
         *        forwarding is attempted.
         * @param onComplete a callback message when the action is completed.
         *        onComplete.arg1 is set to zero (0) if the action is completed
         *        successfully.
         */
        public void setCallForwardingOption(int commandInterfaceCFReason,
                                     int commandInterfaceCFAction,
                                     String dialingNumber,
                                     int timerSeconds,
                                     Message onComplete) {
            CsvtService.this.setCallForwardingOption(commandInterfaceCFReason,
                    commandInterfaceCFAction, dialingNumber, timerSeconds,
                    onComplete);
        }

        /**
         * getCallWaiting
         * gets call waiting activation state. The call waiting activation state
         * is returned via ICsvtServiceListener.onCallWaiting callback.
         *
         * @param onComplete a callback message when the action is completed.
         *        onComplete.arg1 is set to zero (0) if the action completed
         *        successfully.
         * @see   ICsvtServiceListener.onCallWaiting
         */
        public void getCallWaiting(Message onComplete) {
            CsvtService.this.getCallWaiting(onComplete);
        }

        /**
         * setCallWaiting
         * sets a call forwarding option.
         *
         * @param enable is a boolean representing the state that you are
         *        requesting, true for enabled, false for disabled.
         * @param onComplete a callback message when the action is completed.
         *        onComplete.arg1 is set to zero (0) if the action is completed
         *        successfully.
         */
        public void setCallWaiting(boolean enable, Message onComplete) {
            CsvtService.this.setCallWaiting(enable, onComplete);
        }

        public void registerListener(ICsvtServiceListener l){
            CsvtService.this.registerListener(l);
        }

        public  void unregisterListener(ICsvtServiceListener l){
            CsvtService.this.unregisterListener(l);
        }

    };
}

