/* Copyright (c) 2014 Qualcomm Technologies, Inc.
 * All Rights Reserved. Qualcomm Technologies Confidential and Proprietary.
 *
 * Not a Contribution, Apache license notifications and license are retained
 * for attribution purposes only.
 *
 *
 * Copyright (C) 2012 The Android Open Source Project
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




package org.codeaurora.btmultisim;

import android.app.Service;
import android.content.Intent;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothHeadset;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.os.AsyncResult;
import android.os.Handler;
import android.os.Message;
import android.os.IBinder;
import android.os.SystemProperties;
import android.telephony.PhoneNumberUtils;
import android.util.Log;

import com.android.internal.telephony.Call;
import com.android.internal.telephony.Connection;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.CallManager;
import android.telephony.MSimTelephonyManager;

import java.io.IOException;
import java.util.LinkedList;
import java.util.List;

/**
 * Bluetooth headset manager for the DSDA call state changes.
 * @hide
 */
public class BluetoothDsdaService extends Service {
    private static final String TAG = "BluetoothDsdaService";
    private static final boolean DBG = true;
    private static final boolean VDBG = false;


    private static final String MODIFY_PHONE_STATE = android.Manifest.permission.MODIFY_PHONE_STATE;

    private BluetoothAdapter mAdapter;
    private CallManager mCM;

    private BluetoothHeadset mBluetoothHeadset;
   // private BluetoothMultiPhoneService mPhoneProxy;

   //At any time we can update only one active and one held
    private int mNumActive;  //must be initialized to zero
    private int mNumHeld; //must be initialized to zero
    private int mDsdaCallState;
    private boolean mFakeMultiParty;

    private BluetoothSub[] mSubscriptions;

    //Stores the current SUB on which call state changed happened.
    private int mCurrentSub = SUB1;
    private boolean mCallSwitch = false;
    private int mNumCdmaHeld = 0;
    // CDMA specific flag used in context with BT devices having display capabilities
    // to show which Caller is active. This state might not be always true as in CDMA
    // networks if a caller drops off no update is provided to the Phone.
    // This flag is just used as a toggle to provide a update to the BT device to specify
    // which caller is active.
    private boolean mCdmaIsSecondCallActive = false;
    private boolean mCdmaCallsSwapped = false;

    private long[] mClccTimestamps; // Timestamps associated with each clcc index
    private boolean[] mClccUsed;     // Is this clcc index in use

    private static final int GSM_MAX_CONNECTIONS = 6;  // Max connections allowed by GSM
    private static final int CDMA_MAX_CONNECTIONS = 2;  // Max connections allowed by CDMA

    /* At present the SUBs are valued as 0 and 1 for DSDA*/
    private static final int SUB1 = 0; //SUB1 is by default CDMA in C+G.
    private static final int SUB2 = 1;
    private static final int MAX_SUBS = 2;

    // match up with bthf_call_state_t of bt_hf.h
    final static int CALL_STATE_ACTIVE = 0;
    final static int CALL_STATE_HELD = 1;
    final static int CALL_STATE_DIALING = 2;
    final static int CALL_STATE_ALERTING = 3;
    final static int CALL_STATE_INCOMING = 4;
    final static int CALL_STATE_WAITING = 5;
    final static int CALL_STATE_IDLE = 6;
    final static int CHLD_TYPE_RELEASEHELD = 0;
    final static int CHLD_TYPE_RELEASEACTIVE_ACCEPTHELD = 1;
    final static int CHLD_TYPE_HOLDACTIVE_ACCEPTHELD = 2;
    final static int CHLD_TYPE_ADDHELDTOCONF = 3;

    //CDMA call states
    final static int CDMA_CALL_STATE_IDLE = 0;
    final static int CDMA_CALL_STATE_SINGLE_ACTIVE = 1;
    final static int CDMA_CALL_STATE_THRWAY_ACTIVE = 2;
    final static int CDMA_CALL_STATE_THRWAY_CONF_CALL = 3;

    private int mCdmaThreeWayCallState = CDMA_CALL_STATE_IDLE;
    private int mCurrentCallState = CDMA_CALL_STATE_IDLE;
    private int mPrevCallState = CDMA_CALL_STATE_IDLE;
    private boolean mIsThreewayCallOriginated = false;
    @Override
    public void onCreate() {
        Log.d(TAG, "BluetoothDsdaStateService created");
        super.onCreate();
        mNumActive = 0;
        mNumHeld = 0;
        mFakeMultiParty = false;
        mCM = CallManager.getInstance();
        mAdapter = BluetoothAdapter.getDefaultAdapter();
        if (mAdapter == null) {
            if (VDBG) Log.d(TAG, "mAdapter null");
            return;
        }
        mSubscriptions = new BluetoothSub[MAX_SUBS];
        for (int i = 0; i < MAX_SUBS; i++) {
            mSubscriptions[i] = new BluetoothSub(i);
        }
        //Get the HeadsetService Profile proxy
        mAdapter.getProfileProxy(this, mProfileListener, BluetoothProfile.HEADSET);
        //Initialize CLCC
        mClccTimestamps = new long[GSM_MAX_CONNECTIONS];
        mClccUsed = new boolean[GSM_MAX_CONNECTIONS];
        for (int i = 0; i < GSM_MAX_CONNECTIONS; i++) {
            mClccUsed[i] = false;
        }
    }

    @Override
    public void onStart(Intent intent, int startId) {
        if (mAdapter == null) {
            Log.w(TAG, "Stopping Bluetooth BluetoothPhoneService Service: device does not have BT");
            stopSelf();
        }
        if (VDBG) Log.d(TAG, "BluetoothDsdaState started");
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (DBG) log("Stopping Bluetooth Dsda Service");
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    class CdmaCallStates {
        private int mCurrentCallState = CDMA_CALL_STATE_IDLE;
        private int mPrevCallState = CDMA_CALL_STATE_IDLE;
        private boolean mIsThreewayCallOriginated = false;

        public CdmaCallStates(int curr, int prev, boolean isThreeWay) {
            mCurrentCallState = curr;
            mPrevCallState = prev;
            mIsThreewayCallOriginated = isThreeWay;
        }
    }

    private static final int DSDA_CALL_STATE_CHANGED = 1;
    private static final int DSDA_LIST_CURRENT_CALLS = 2;
    private static final int DSDA_PHONE_SUB_CHANGED = 3;
    private static final int DSDA_SWITCH_SUB = 4;
    private static final int DSDA_UPDATE_CDMA_CALL_STATES = 5;
    private static final int DSDA_SET_CDMA_CALL_STATES = 6;
    private static final int DSDA_SWAP_CDMA_SECOND_CALL_STATE = 7;

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            if (VDBG) Log.d(TAG, "handleMessage: " + msg.what);
            switch(msg.what) {
                case DSDA_CALL_STATE_CHANGED:
                    handleDsdaPreciseCallStateChange();
                    break;
                case DSDA_LIST_CURRENT_CALLS:
                    handleDsdaListCurrentCalls();
                    break;
                case DSDA_PHONE_SUB_CHANGED:
                    handlePhoneSubChanged();
                    break;
                case DSDA_SWITCH_SUB:
                    switchSub();
                    break;
                case DSDA_UPDATE_CDMA_CALL_STATES:
                    if (msg.arg1 == 1)
                        mCdmaIsSecondCallActive = true;
                    else mCdmaIsSecondCallActive = false;
                    if (!mCdmaIsSecondCallActive) {
                        mCdmaCallsSwapped = false;
                    }
                    break;
                case DSDA_SET_CDMA_CALL_STATES:
                    CdmaCallStates cdmaCallStates;
                    cdmaCallStates = (CdmaCallStates)msg.obj;
                    mCurrentCallState = cdmaCallStates.mCurrentCallState;
                    mPrevCallState = cdmaCallStates.mPrevCallState;
                    mIsThreewayCallOriginated = cdmaCallStates.mIsThreewayCallOriginated;
                    break;
                case DSDA_SWAP_CDMA_SECOND_CALL_STATE:
                    mCdmaIsSecondCallActive = !mCdmaIsSecondCallActive;
                    mCdmaCallsSwapped = true;
                    break;
                default:
                    Log.d(TAG, "Unknown event : ");
                    break;
            }
        }
    };

    /* Handles call state changes on each subscription. */
    public void handleDsdaPreciseCallStateChange() {
        Log.d(TAG, "handleDSDAPreciseCallStateChange");
        //Handle call state changes of both subs separately
        int SubId = getCurrentSub();
        Log.d(TAG, "Call change of : " + SubId);
        mSubscriptions[SubId].handleSubscriptionCallStateChange();

    }

    /* Called to notify the Subscription change event from telephony.*/
    private void handlePhoneSubChanged() {
        /*Could be used to notify switch SUB to headsets*/
        int sub = mCM.getActiveSubscription();
        Log.d(TAG, "Phone SUB changed, Active: " + sub);
        if (isSwitchSubAllowed() == true) {
            Log.d(TAG, "Update headset about switch sub");
            if (mBluetoothHeadset != null) {
                mBluetoothHeadset.phoneStateChanged(mNumActive, mNumHeld,
                    CALL_STATE_IDLE, null, 0);
            }
        }
    }

    /* Do the SwithSuB. */
    private void switchSub() {
        log("SwitchSub");
        int activeSub = mCM.getActiveSubscription();
        switchToOtherActiveSub(activeSub);
    }

    private void switchToOtherActiveSub(int sub) {
        int count = MSimTelephonyManager.getDefault().getPhoneCount();
        log("switchToOtherActiveSub");
        for (int i = 0; i < count; i++) {
            if ((i != sub) && (mCM.getState(i) != PhoneConstants.State.IDLE)) {
                setActiveSubscription(i);
                // Since active subscription got changed, call setAudioMode
                // which informs LCH state to RIL and updates audio state of subs.
                // This required to update the call audio states when switch sub
                // triggered from UI.
                mCM.setAudioMode();
                log("Switching to other active sub  = " + i );
                break;
            }
        }
    }

    /**
    * Set the given subscription as current active subscription i.e currently on
    * which voice call is active(with state OFFHOOK/RINGING) and which needs to be
    * visible to user.
    *
    * @param subscription the sub id which needs to be active one.
    */
    private void setActiveSubscription(int subscription) {
        int activeSub = mCM.getActiveSubscription();
        if (activeSub != subscription) {
            mCM.setActiveSubscription(subscription);
        }
    }

    /* Executes AT+CLCC for DSDA scenarios. */
    private void handleDsdaListCurrentCalls() {
        // Check if we are in DSDA mode.
        int activeSub = mCM.getActiveSubscription();
        boolean allowDsda = false;
        Call call = getCallOnOtherSub();
        Log.d(TAG, "handleListCurrentCalls");
        //For CDMA SUB, update CLCC separately if only this SUB is active.
        if (isSingleSubActive()) {
            if (mSubscriptions[activeSub].mPhonetype
                == PhoneConstants.PHONE_TYPE_CDMA) {
                log("Only CDMA call list, on SUB1");
                listCurrentCallsCdma(activeSub);
                if (mBluetoothHeadset != null) {
                    log("last clcc update on SUB1");
                    mBluetoothHeadset.clccResponse(0, 0, 0, 0, false, "", 0);
                } else log("headset null, no clcc update");
                return;
            }
        }
        //Send CLCC for both subs.
        //allowDsda will allow sending CLCC at call setup states too.
        if ((call != null) &&
            (mSubscriptions[activeSub].mCallState != CALL_STATE_IDLE)) {
            allowDsda = true;
        }
        log("allowdsda: " + allowDsda);
        listCurrentCallsOnBothSubs(allowDsda);

        // end the result
        // when index is 0, other parameter does not matter
        if (mBluetoothHeadset != null) {
            log("send last clcc update");
            mBluetoothHeadset.clccResponse(0, 0, 0, 0, false, "", 0);
        } else log("headset null, no clcc update");
    }


    //This will also register for getting the Bluetooth Headset Profile proxy
    private BluetoothProfile.ServiceListener mProfileListener =
            new BluetoothProfile.ServiceListener() {
        public void onServiceConnected(int profile, BluetoothProfile proxy) {
            Log.d(TAG, "Got the headset proxy for DSDA" );
            mBluetoothHeadset = (BluetoothHeadset) proxy;
        }
        public void onServiceDisconnected(int profile) {
            mBluetoothHeadset = null;
            Log.d(TAG, "Released the headset proxy for DSDA");
        }
    };

    public void handleCdmaSetSecCallState(boolean state) {
        if (VDBG) log("cdmaSetSecondCallState: Setting mCdmaIsSecondCallActive to " + state);
        mCdmaIsSecondCallActive = state;
        if (!mCdmaIsSecondCallActive) {
            mCdmaCallsSwapped = false;
        }
    }

    /* get the current SUB*/
    private int getCurrentSub() {
        return mCurrentSub;
    }

    /* SUB switch is allowed only when each sub has max
    of one call, either active or held*/
    public boolean isSwitchSubAllowed() {
        boolean allowed = false;
        if ((((mSubscriptions[SUB1].mActive + mSubscriptions[SUB1].mHeld) == 1)
            && (mSubscriptions[SUB1].mCallState == CALL_STATE_IDLE))
            && (((mSubscriptions[SUB2].mActive +
            mSubscriptions[SUB2].mHeld) == 1)
            && (mSubscriptions[SUB2].mCallState == CALL_STATE_IDLE))) {
            allowed = true;
        }
        log("Is switch SUB allowed: " + allowed);
        return allowed;
    }

    /* Get the active or held call on other Sub. */
    public Call getCallOnOtherSub() {
        log("getCallOnOtherSub");
        int activeSub = mCM.getActiveSubscription();
        int bgSub =  0;  //Telephony to implement one call manager function to
        // get BG SUB
        /*bgSub would be -1 when bg subscription has no calls*/
        if (bgSub == -1)
            return null;

        Call call = null;
        if ((mSubscriptions[bgSub].mActive + mSubscriptions[bgSub].mHeld)
            == 1) {
            if (mCM.hasActiveFgCall(bgSub))
                call = mCM.getActiveFgCall(bgSub);
            else if (mCM.hasActiveBgCall(bgSub))
                call = mCM.getFirstActiveBgCall(bgSub);
        }
        return call;
    }

    /**
    * Check whether any other sub is in active state other than
    * provided subscription, if yes return the other active sub.
    * @return subscription which is active, if no other sub is in
    * active state return -1.
    */
    private int getOtherActiveSub(int subscription) {
        int otherSub = -1;
        int count = MSimTelephonyManager.getDefault().getPhoneCount();
        log("getOtherActiveSub: sub = " + subscription + " count = " + count);
        for (int i = 0; i < count; i++) {
            if ((i != subscription) && (mCM.getState(i)
                != PhoneConstants.State.IDLE)) {
                log("getOtherActiveSub: active sub  = " + i );
                otherSub = i;
                break;
            }
        }
        return otherSub;
    }


    boolean hasCallsOnBothSubs() {
        if (((mSubscriptions[SUB1].mActive + mSubscriptions[SUB1].mHeld) >= 1)
            && ((mSubscriptions[SUB2].mActive + mSubscriptions[SUB2].mHeld)
            >= 1)) {
            log("hasCallsOnBothSubs is true");
            return true;
        }
        return false;
    }

    /* CallState should be for the current active Sub*/
    public int getCallState() {
        int activeSub = mCM.getActiveSubscription();
        Call foregroundCall = mCM.getActiveFgCall(activeSub);
        Call ringingCall = mCM.getFirstActiveRingingCall(activeSub);
        Call.State mForegroundCallState;
        Call.State mRingingCallState;
        mForegroundCallState = foregroundCall.getState();
        mRingingCallState = ringingCall.getState();
        int callState = convertCallState(mRingingCallState, mForegroundCallState);
        return callState;
    }

    /* Check if calls on single subscription only. */
    private boolean isSingleSubActive() {
        if (((((mSubscriptions[SUB1].mActive + mSubscriptions[SUB1].mHeld) >=
            1) || mSubscriptions[SUB1].mCallState != CALL_STATE_IDLE)
            && (((mSubscriptions[SUB2].mActive + mSubscriptions[SUB2].mHeld)
            < 1) && (mSubscriptions[SUB2].mCallState == CALL_STATE_IDLE)))
            || ((((mSubscriptions[SUB2].mActive + mSubscriptions[SUB2].mHeld)
            >= 1) || mSubscriptions[SUB2].mCallState != CALL_STATE_IDLE)
            && ((mSubscriptions[SUB1].mActive + mSubscriptions[SUB1].mHeld) <
            1) && (mSubscriptions[SUB1].mCallState == CALL_STATE_IDLE))) {
            log("calls on single sub");
            return true;
        }
        return false;
    }

    /*List CLCC for CDMA Subscription. */
    private void listCurrentCallsCdma(int sub) {
        // In CDMA at one time a user can have only two live/active connections
        Connection[] clccConnections = new Connection[CDMA_MAX_CONNECTIONS];// indexed by CLCC index
        Call foregroundCall = mCM.getActiveFgCall(sub);
        Call ringingCall = mCM.getFirstActiveRingingCall(sub);

        Call.State ringingCallState = ringingCall.getState();
        // If the Ringing Call state is INCOMING, that means this is the very first call
        // hence there should not be any Foreground Call
        if (ringingCallState == Call.State.INCOMING) {
            if (VDBG) log("Filling clccConnections[0] for INCOMING state");
            clccConnections[0] = ringingCall.getLatestConnection();
        } else if (foregroundCall.getState().isAlive()) {
            // Getting Foreground Call connection based on Call state
            if (ringingCall.isRinging()) {
                if (VDBG) log("Filling clccConnections[0] & [1] for CALL WAITING state");
                clccConnections[0] = foregroundCall.getEarliestConnection();
                clccConnections[1] = ringingCall.getLatestConnection();
            } else {
                if (foregroundCall.getConnections().size() <= 1) {
                    // Single call scenario
                    if (VDBG) {
                        log("Filling clccConnections[0] with ForgroundCall latest connection");
                    }
                    clccConnections[0] = foregroundCall.getLatestConnection();
                } else {
                    // Multiple Call scenario. This would be true for both
                    // CONF_CALL and THRWAY_ACTIVE state
                    if (VDBG) {
                        log("Filling clccConnections[0] & [1] with ForgroundCall connections");
                    }
                    clccConnections[0] = foregroundCall.getEarliestConnection();
                    clccConnections[1] = foregroundCall.getLatestConnection();
                }
            }
        }

        // Update the mCdmaIsSecondCallActive flag based on the Phone call state
        if (mCurrentCallState == CDMA_CALL_STATE_SINGLE_ACTIVE) {
            handleCdmaSetSecCallState(false);
        } else if (mCurrentCallState == CDMA_CALL_STATE_THRWAY_ACTIVE) {
            handleCdmaSetSecCallState(true);
        }
        // send CLCC result
        for (int i = 0; (i < clccConnections.length) && (clccConnections[i] != null); i++) {
            sendClccResponseCdma(i, clccConnections[i]);
        }
    }

    /** Send ClCC results for a Connection object for CDMA phone */
    private void sendClccResponseCdma(int index, Connection connection) {
        int state;

        if ((mPrevCallState == CDMA_CALL_STATE_THRWAY_ACTIVE)
                && (mCurrentCallState == CDMA_CALL_STATE_THRWAY_CONF_CALL)) {
            // If the current state is reached after merging two calls
            // we set the state of all the connections as ACTIVE
            state = CALL_STATE_ACTIVE;
        } else {
            Call.State callState = connection.getState();
            switch (callState) {
            case ACTIVE:
                // For CDMA since both the connections are set as active by FW after accepting
                // a Call waiting or making a 3 way call, we need to set the state specifically
                // to ACTIVE/HOLDING based on the mCdmaIsSecondCallActive flag. This way the
                // CLCC result will allow BT devices to enable the swap or merge options
                if (index == 0) { // For the 1st active connection
                    state = mCdmaIsSecondCallActive ? CALL_STATE_HELD : CALL_STATE_ACTIVE;
                } else { // for the 2nd active connection
                    state = mCdmaIsSecondCallActive ? CALL_STATE_ACTIVE : CALL_STATE_HELD;
                }
                break;
            case HOLDING:
                state = CALL_STATE_HELD;
                break;
            case DIALING:
                state = CALL_STATE_DIALING;
                break;
            case ALERTING:
                state = CALL_STATE_ALERTING;
                break;
            case INCOMING:
                state = CALL_STATE_INCOMING;
                break;
            case WAITING:
                state = CALL_STATE_WAITING;
                break;
            default:
                Log.e(TAG, "bad call state: " + callState);
                return;
            }
        }

        boolean mpty = false;
        if (mCurrentCallState == CDMA_CALL_STATE_THRWAY_CONF_CALL) {
            if (mPrevCallState == CDMA_CALL_STATE_THRWAY_ACTIVE) {
                // If the current state is reached after merging two calls
                // we set the multiparty call true.
                mpty = true;
            } // else
                // CALL_CONF state is not from merging two calls, but from
                // accepting the second call. In this case first will be on
                // hold in most cases but in some cases its already merged.
                // However, we will follow the common case and the test case
                // as per Bluetooth SIG PTS
        }

        int direction = connection.isIncoming() ? 1 : 0;

        String number = connection.getAddress();
        int type = -1;
        if (number != null) {
            type = PhoneNumberUtils.toaFromString(number);
        } else {
            number = "";
        }
        mBluetoothHeadset.clccResponse(index + 1, direction, state, 0, mpty, number, type);
    }

    /* List CLCC on both the subscription. The max call list is driven by
       GSM_MAX_CONNECTIONS, even in DSDA. */
    private void listCurrentCallsOnBothSubs(boolean allowDsda) {
        // Collect all known connections
        // clccConnections isindexed by CLCC index
        log("listCurrentCallsOnBothSubs");
        //In DSDA, call list is limited by GSM_MAX_CONNECTIONS.
        Connection[] clccConnections = new Connection[GSM_MAX_CONNECTIONS];
        LinkedList<Connection> newConnections = new LinkedList<Connection>();
        LinkedList<Connection> connections = new LinkedList<Connection>();
        //Get all calls on subscription one.
        Call foregroundCallSub1 = mCM.getActiveFgCall(SUB1);
        Call backgroundCallSub1 = mCM.getFirstActiveBgCall(SUB1);
        Call ringingCallSub1 = mCM.getFirstActiveRingingCall(SUB1);
        //Get all calls on subscription two.
        Call foregroundCallSub2 = mCM.getActiveFgCall(SUB2);
        Call backgroundCallSub2 = mCM.getFirstActiveBgCall(SUB2);
        Call ringingCallSub2 = mCM.getFirstActiveRingingCall(SUB2);

        Log.d(TAG, " SUB1:" + "foreground: " + foregroundCallSub1 +
            " background: " + backgroundCallSub1 + " ringing: " + ringingCallSub1);
        Log.d(TAG, " SUB2:" + "foreground: " + foregroundCallSub2 + " background: "
            + backgroundCallSub2 + " ringing: " + ringingCallSub2);

        //Get CDMA SUB call connections first.
        if (mSubscriptions[SUB1].mPhonetype ==
            PhoneConstants.PHONE_TYPE_CDMA) {
            Call.State ringingCallState = ringingCallSub1.getState();
            if (ringingCallState == Call.State.INCOMING) {
                log("Filling Connections for INCOMING state");
                connections.add(ringingCallSub1.getLatestConnection());
            } else if (foregroundCallSub1.getState().isAlive()) {
                // Getting Foreground Call connection based on Call state
                if (ringingCallSub1.isRinging()) {
                    log("Filling Connections for CALL WAITING state");
                    connections.add(foregroundCallSub1.getEarliestConnection());
                    connections.add(ringingCallSub1.getLatestConnection());
                } else {
                    if (foregroundCallSub1.getConnections().size() <= 1) {
                        // Single call scenario
                        if (VDBG) {
                            log("Connections with ForgroundCall latest connection");
                        }
                        connections.add(foregroundCallSub1.getLatestConnection());
                    } else {
                        // Multiple Call scenario. This would be true for both
                        // CONF_CALL and THRWAY_ACTIVE state.
                        log("Connections withForgroundCall connections");
                        connections.add(foregroundCallSub1.getEarliestConnection());
                        connections.add(foregroundCallSub1.getLatestConnection());
                    }
                }
            }
            /* Get calls on other Sub. They are non CDMA */
            if (ringingCallSub2.getState().isAlive()) {
                log("Add SUB2 ringing calls");
                connections.addAll(ringingCallSub2.getConnections());
            }
            if (foregroundCallSub2.getState().isAlive()) {
                log("Add SUB2 forground calls");
                connections.addAll(foregroundCallSub2.getConnections());
            }
            if (backgroundCallSub2.getState().isAlive()) {
                log("Add SUB2 background calls");
                connections.addAll(backgroundCallSub2.getConnections());
            }
        } else if (mSubscriptions[SUB2].mPhonetype ==
            PhoneConstants.PHONE_TYPE_CDMA) {
            Call.State ringingCallState = ringingCallSub2.getState();
            if (ringingCallState == Call.State.INCOMING) {
                if (VDBG) log("Filling clccConnections[0] for INCOMING state");
                connections.add(ringingCallSub2.getLatestConnection());
            } else if (foregroundCallSub2.getState().isAlive()) {
                // Getting Foreground Call connection based on Call state
                if (ringingCallSub2.isRinging()) {
                    if (VDBG) log("clccConnections for CALL WAITING state");
                    connections.add(foregroundCallSub2.getEarliestConnection());
                    connections.add(ringingCallSub2.getLatestConnection());
                } else {
                    if (foregroundCallSub2.getConnections().size() <= 1) {
                        // Single call scenario
                        if (VDBG) {
                            log("ClccConnections with ForgroundCall latest connection");
                        }
                        connections.add(foregroundCallSub2.getLatestConnection());
                    } else {
                        // Multiple Call scenario. This would be true for both
                        // CONF_CALL and THRWAY_ACTIVE state
                        log("ClccConnections withForgroundCall connections on sub2");
                        connections.add(foregroundCallSub2.getEarliestConnection());
                        connections.add(foregroundCallSub2.getLatestConnection());
                    }
                }
            }
            /* Get calls on other Sub. They are non CDMA*/
            if (ringingCallSub1.getState().isAlive()) {
                log("Add SUB1 ringing calls");
                connections.addAll(ringingCallSub1.getConnections());
            }
            if (foregroundCallSub1.getState().isAlive()) {
                log("Add SUB1 forground calls");
                connections.addAll(foregroundCallSub1.getConnections());
            }
            if (backgroundCallSub1.getState().isAlive()) {
                log("Add SUB1 background calls");
                connections.addAll(backgroundCallSub1.getConnections());
            }
        }
        // Update the mCdmaIsSecondCallActive flag based on the Phone call state
        if (mCurrentCallState == CDMA_CALL_STATE_SINGLE_ACTIVE) {
            mCdmaIsSecondCallActive = false;
            if (!mCdmaIsSecondCallActive) {
                mCdmaCallsSwapped = false;
            }
        } else if (mCurrentCallState == CDMA_CALL_STATE_THRWAY_ACTIVE) {
            mCdmaIsSecondCallActive = true;
            if (!mCdmaIsSecondCallActive) {
                mCdmaCallsSwapped = false;
            }
        }
        log("calls added for both subscriptions");
        // Mark connections that we already known about
        boolean clccUsed[] = new boolean[GSM_MAX_CONNECTIONS];
        for (int i = 0; i < GSM_MAX_CONNECTIONS; i++) {
            clccUsed[i] = mClccUsed[i];
            log("add clcc about: " + i + " mClcc[]: " + mClccUsed[i]);
            mClccUsed[i] = false;
        }
        for (Connection c : connections) {
            boolean found = false;
            long timestamp = c.getCreateTime();
            for (int i = 0; i < GSM_MAX_CONNECTIONS; i++) {
                if (clccUsed[i] && timestamp == mClccTimestamps[i]) {
                    log("mClccUsed is true for: " + i);
                    mClccUsed[i] = true;
                    found = true;
                    clccConnections[i] = c;
                    break;
                }
            }
            if (!found) {
                log("add new conns");
                newConnections.add(c);
            }
        }
        // Find a CLCC index for new connections
        while (!newConnections.isEmpty()) {
            // Find lowest empty index
            int i = 0;
            while (mClccUsed[i]) i++;
            // Find earliest connection
            long earliestTimestamp = newConnections.get(0).getCreateTime();
            Connection earliestConnection = newConnections.get(0);
            for (int j = 0; j < newConnections.size(); j++) {
                long timestamp = newConnections.get(j).getCreateTime();
                if (timestamp < earliestTimestamp) {
                    earliestTimestamp = timestamp;
                    earliestConnection = newConnections.get(j);
                }
            }
            // update
            mClccUsed[i] = true;
            mClccTimestamps[i] = earliestTimestamp;
            clccConnections[i] = earliestConnection;
            newConnections.remove(earliestConnection);
        }
        // Send CLCC response to Bluetooth headset service
        for (int i = 0; i < clccConnections.length; i++) {
            if (mClccUsed[i]) {
                log("Send CLCC for Connection index: " + i);
                sendClccResponse(i, clccConnections[i], allowDsda);
            }
        }
    }

    /** Convert a Connection object into a single +CLCC result */
    private void sendClccResponse(int index, Connection connection,
                                  boolean allowDsda) {
        int state = convertCallState(connection.getState());
        boolean mpty = false;
        boolean active = false;
        Call call = connection.getCall();
        Phone phone = call.getPhone();
        int sub = phone.getSubscription();

        log("CLCC on this SUB: " + sub + " CallState: " + state);
        int activeSub =  mCM.getActiveSubscription();
        if (mSubscriptions[activeSub].mPhonetype
            == PhoneConstants.PHONE_TYPE_CDMA) {
            log("Active SUB is CDMA");
            Call foregroundCall = mCM.getActiveFgCall(activeSub);
            Connection fg = foregroundCall.getLatestConnection();
            Connection bg = foregroundCall.getEarliestConnection();
            if (mCdmaIsSecondCallActive == true) {
                log("Two calls on CDMA SUB");
                if ((fg != null) && (bg != null)) {
                    if ((fg != null) && (connection == fg)) {
                        active = true;
                        log("getEarliestConnection of CDMA calls");
                    }
                } else log ("Error in cdma connection");
            } else if ((fg != null) && (connection == fg)) {
                log("getEarliestConnection for single cdma call");
                active = true;
            } else if ((bg != null) && (connection == bg)) {
                log("getLatestConnection for single cdma call");
                active = true;
            }
        } else {
            log("CLCC for NON CDMA SUB");
            if (call == mCM.getFirstActiveRingingCall(activeSub)) {
                log("This is FG ringing call");
                active = true;
            } else if (call == mCM.getActiveFgCall(activeSub)) {
                active = true;
                log("This is first FG call");
            } else if (call == mCM.getFirstActiveBgCall(activeSub)) {
                log("BG call on GSM sub");
            }
        }
        //For CDMA subscription, mpty will be true soon after
        //two calls are active.
        if (call != null) {
            mpty = call.isMultiparty();
            log("call.isMultiparty: " + mpty);
            if((mFakeMultiParty == true) && !active) {
                log("A fake mparty scenario");
                if(!mpty)
                mpty = true;
            }
        }
        int direction = connection.isIncoming() ? 1 : 0;
        String number = connection.getAddress();
        int type = -1;
        if (number != null) {
            type = PhoneNumberUtils.toaFromString(number);
        }
        if (mNumActive + mNumHeld >= 1) {
            log("If Incoming call, change to waiting");
            if (state == CALL_STATE_INCOMING)
                state = CALL_STATE_WAITING; //DSDA
        }
        // If calls on both Subs, need to change call states on BG calls
        if (((hasCallsOnBothSubs() == true) || allowDsda) && !active) {
            //Fake call held for all background calls
            log("Check if this call state to b made held");
            Call activeSubForegroundCall = mCM.getActiveFgCall(activeSub);
            Call activeSubRingingCall = mCM.getFirstActiveRingingCall(
                                            activeSub);
            int activeCallState = convertCallState(activeSubRingingCall.getState(),
                                  activeSubForegroundCall.getState());
            if ((state == CALL_STATE_ACTIVE) &&
                (activeCallState != CALL_STATE_INCOMING))
                state = CALL_STATE_HELD;
            else if ((mpty == true)) {
                log("mtpy is true, manage call states on bg SUB");
                if(activeCallState != CALL_STATE_INCOMING)
                    state = CALL_STATE_HELD;
                else if (activeCallState == CALL_STATE_INCOMING)
                    state = CALL_STATE_ACTIVE;
            }
        }
        if (mBluetoothHeadset != null) {
            log("CLCC response to mBluetoothHeadset");
            mBluetoothHeadset.clccResponse(index+1, direction, state, 0, mpty,
            number, type);
        } else log("headset null, no need to send clcc");
    }

    /* Called to notify the Subscription change event from telephony.*/

    private class BluetoothSub {
        private Call.State mForegroundCallState;
        private Call.State mRingingCallState;
        private CallNumber mRingNumber;
        private int mSubscription;
        private int mActive;
        private int mHeld;
        private int mCallState;
        private int mPhonetype;
        private long mBgndEarliestConnectionTime = 0;

        private BluetoothSub(int SubId) {
            Log.d(TAG, "Creating Bluetooth SUb for " + SubId);
            mForegroundCallState = Call.State.IDLE;
            mRingingCallState = Call.State.IDLE;
            mRingNumber = new CallNumber("", 0);;
            mSubscription = SubId;
            mActive = 0;
            mHeld = 0;
            mCallState = 0;
            for (Phone phone : mCM.getAllPhones()) {
               if (phone != null) {
                   if(phone.getSubscription() == SubId)
                       mPhonetype = phone.getPhoneType();
               }
            }
            Log.d(TAG, "Bluetooth SUB: " + SubId + " for PhoneType: " + mPhonetype);
        }
        /* Handles the single subscription call state changes.*/
        private void handleSubscriptionCallStateChange() {
            // get foreground call state
            int oldNumActive = mActive;
            int oldNumHeld = mHeld;
            Call.State oldRingingCallState = mRingingCallState;
            Call.State oldForegroundCallState = mForegroundCallState;
            CallNumber oldRingNumber = mRingNumber;

            Call foregroundCall = mCM.getActiveFgCall(mSubscription);

            Log.d(TAG, " SUB:" + mSubscription + "foreground: " + foregroundCall +
                  " background: " + mCM.getFirstActiveBgCall(mSubscription)
                  + " ringing: " + mCM.getFirstActiveRingingCall(mSubscription));
            Log.d(TAG, "mActive: " + mActive + " mHeld: " + mHeld);

            mForegroundCallState = foregroundCall.getState();
            /* if in transition, do not update */
            if (mForegroundCallState == Call.State.DISCONNECTING) {
                Log.d(TAG, "SUB1. Call disconnecting,wait before update");
                return;
            }
            else
                mActive = (mForegroundCallState == Call.State.ACTIVE) ? 1 : 0;

            Log.d(TAG, "New state of active call:= " + mActive);
            Call ringingCall = mCM.getFirstActiveRingingCall(mSubscription);
            mRingingCallState = ringingCall.getState();
            mRingNumber = getCallNumber(null, ringingCall);

            if (mPhonetype == PhoneConstants.PHONE_TYPE_CDMA) {
                Log.d(TAG, "CDMA. Get number of held calls on this SUB");
                mHeld = getNumHeldCdmaPhone();
            } else {
                Log.d(TAG, "GSM/WCDMA/UMTS. Get number of held calls on this SUB");
                mHeld = getNumHeldUmts();
            }

            mCallState = convertCallState(mRingingCallState, mForegroundCallState);
            boolean callsSwitched = false;
            if (mPhonetype == PhoneConstants.PHONE_TYPE_CDMA &&
                mCdmaThreeWayCallState ==
                CDMA_CALL_STATE_THRWAY_CONF_CALL) {
                callsSwitched = mCdmaCallsSwapped;
                Log.d(TAG, "Call switch value for cdma: " + callsSwitched);
            } else {
                Call backgroundCall = mCM.getFirstActiveBgCall(mSubscription);
                callsSwitched = (mHeld == 1 && ! (backgroundCall.getEarliestConnectTime() ==
                mBgndEarliestConnectionTime));
                mBgndEarliestConnectionTime = backgroundCall.getEarliestConnectTime();
                Log.d(TAG, "Call switch value: " + callsSwitched +
                    " mBgndEarliestConnectionTime " + mBgndEarliestConnectionTime);
            }
            mCallSwitch = callsSwitched;

            if (mActive != oldNumActive || mHeld != oldNumHeld ||
                mRingingCallState != oldRingingCallState ||
                mForegroundCallState != oldForegroundCallState ||
                !mRingNumber.equalTo(oldRingNumber) ||
                callsSwitched) {
                Log.d(TAG, "Update the handleSendcallStates for Sub: " + mSubscription);
                handleSendcallStates(mSubscription, mActive, mHeld,
                    convertCallState(mRingingCallState, mForegroundCallState),
                    mRingNumber.mNumber, mRingNumber.mType);
            }
        }

        private int getNumHeldCdmaPhone() {
            mHeld = mNumCdmaHeld;

            if ((mBluetoothHeadset != null) &&
                (mCdmaThreeWayCallState != mCurrentCallState)) {
                // In CDMA, the network does not provide any feedback
                // to the phone when the 2nd MO call goes through the
                // stages of DIALING > ALERTING -> ACTIVE we fake the
                // sequence
                log("CDMA 3way call state change");
                if ((mCurrentCallState == CDMA_CALL_STATE_THRWAY_ACTIVE )
                    && mIsThreewayCallOriginated) {
                    // Mimic dialing, put the call on hold, alerting
                    handleSendcallStates(mSubscription,0, mHeld,
                        convertCallState(Call.State.IDLE, Call.State.DIALING),
                        mRingNumber.mNumber, mRingNumber.mType);

                    handleSendcallStates(mSubscription,0, mHeld,
                    convertCallState(Call.State.IDLE, Call.State.ALERTING),
                        mRingNumber.mNumber, mRingNumber.mType);
                }
                // In CDMA, the network does not provide any feedback to
                // the phone when a user merges a 3way call or swaps
                // between two calls we need to send a CIEV response
                // indicating that a call state got changed which should
                // trigger a CLCC update request from the BT client.
                if (mCurrentCallState == CDMA_CALL_STATE_THRWAY_CONF_CALL &&
                    mPrevCallState == CDMA_CALL_STATE_THRWAY_ACTIVE) {
                    log("CDMA 3way conf call.");
                    handleSendcallStates(mSubscription, mActive, mHeld,
                    convertCallState(Call.State.IDLE,mForegroundCallState),
                        mRingNumber.mNumber, mRingNumber.mType);
                }
            }
            mCdmaThreeWayCallState = mCurrentCallState;
            return mHeld;
        }

        private int getActive() {
            return mActive;
        }
        private int getHeld() {
            return mHeld;
        }
        private int getNumHeldUmts() {
            int countHeld = 0;
            Call backgroundCall = mCM.getFirstActiveBgCall(mSubscription);
            if (backgroundCall.getState() == Call.State.HOLDING) {
                log("There is a held call on SUB: " + mSubscription);
                countHeld++;
            }
            return countHeld;
        }
        public int getCallState() {
            return mCallState;
        }
    } /* BluetoothSub Class*/


    // DSDA state machine which takes care of sending indicators
    private void handleSendcallStates(int SUB, int numActive, int numHeld, int
                                      callState, String number,int type) {
        //SUB will give info that for which SUB these changes have to be updated
        //Get the states of other SUB..
        int otherSubActive;
        int otherSubHeld;
        int otherSubCallState;
        int otherSub;
        Log.d(TAG, "mNumActive: " + mNumActive + " mNumHeld: " + mNumHeld);
        Log.d(TAG, "numactive: " + numActive + " numHld: " + numHeld + " Callstate: " +
            callState + " Number: " + number + " type: " + type);

        if (SUB == SUB2) {
            otherSub = SUB1;
        } else otherSub = SUB2;

        otherSubActive = mSubscriptions[otherSub].getActive();
        otherSubHeld = mSubscriptions[otherSub].getHeld();
        Log.d(TAG, "Call states of other sub are" + " Active: " +
              otherSubActive + " Held: " + otherSubHeld);

        if ((mNumActive + mNumHeld) == 2) {
            //Meaning, we are already in a state of max calls
            //Check the current call state.Already sent 4,1
            switch(callState){
                case CALL_STATE_INCOMING:
                    //This makes sure that the
                    // current SUB is not running in max calls
                    if ((numActive + numHeld) < 2) {
                        //Fake Indicator first about call join (callheld =0)
                        mNumHeld = 0;
                        mFakeMultiParty = true;
                        if (mBluetoothHeadset != null) {
                            mBluetoothHeadset.phoneStateChanged(1, 0,
                                CALL_STATE_IDLE, null, 0);
                            //Send new incoming call notification
                            mBluetoothHeadset.phoneStateChanged(1, 0,
                                CALL_STATE_INCOMING, number, type);
                        }
                    } else if ((numActive + numHeld) == 2) {
                        //Notify the same .HS may reject this call
                        //If this call is accepted, we fall in a case where
                        // telephony will drop one of the current call
                        if (mBluetoothHeadset != null)
                            mBluetoothHeadset.phoneStateChanged(1, 1,
                                CALL_STATE_INCOMING, number, type);
                    }
                    break;

                case CALL_STATE_IDLE:
                    //Could come when calls are being dropped/hanged OR
                    //This state could be reached when HF tried to receive the
                    // third call and telephony would have dropped held call on
                    // currnt SUB..OR. HS just rejected the call
                    //TODO
                    //This state is also seen in call switch on same sub
                    if ((numActive + numHeld + otherSubActive + otherSubHeld) >= 2) {
                        // greater than 2 means we have atleast one active one held
                        //no need to update the headset on this
                        //Add log to see which call is dropped
                        if (((numActive + numHeld) == 2) &&
                            (mCallSwitch == true)) {
                            log("Call switch happened on this SUB");
                            if (mBluetoothHeadset != null) {
                                log("update hs");
                                mBluetoothHeadset.phoneStateChanged(mNumActive, mNumHeld,
                                    callState,number, type);
                            } else Log.d(TAG, "No need as headset is null");
                        } else if ((otherSubActive + otherSubHeld) >= 1) {
                            log("No update is needed");
                        } else if ((numActive + numHeld) == 1) {
                            log("Call position changed on this sub having single call");
                            //We dont get callSwitch true when call comes frm
                            // held to active
                            if (mBluetoothHeadset != null) {
                                log("update hs for this switch");
                                mBluetoothHeadset.phoneStateChanged(mNumActive, mNumHeld,
                                                          callState,number, type);
                            } else Log.d(TAG, "No need as headset is null");
                        } else if (mDsdaCallState != CALL_STATE_IDLE) {
                            log("New call setup failed");
                            if (mBluetoothHeadset != null) {
                                mBluetoothHeadset.phoneStateChanged(mNumActive, mNumHeld,
                                    callState,number, type);
                            } else Log.d(TAG, "Headset is null");
                        } else log("No need to update this call drop");
                    } else {
                         log("Some call may have dropped in current Sub");
                        //Send proper updates
                        // All calls on this SUB are ended now.
                        if (numActive + numHeld == 0) {
                            if (otherSubActive == 1)
                                mNumActive = 1;
                            else mNumActive = 0;
                            if (otherSubHeld == 1)
                                mNumHeld = 1;
                            else mNumHeld = 0;
                        } else {
                             log("the current SUB has more than one call,update");
                            //The current SUB has more than one call
                            //just update the same.
                            mNumActive = numActive;
                            mNumHeld = numHeld;
                        }
                        if (mBluetoothHeadset != null)
                            mBluetoothHeadset.phoneStateChanged(mNumActive,
                                              mNumHeld,CALL_STATE_IDLE,number, type);
                    }
                    break;

                case CALL_STATE_DIALING:
                    //Check if we can honor the dial from this SUB
                    if ((numActive + numHeld) < 2) {
                        // we would have sent 4,1 already before this
                        // It is slight different again compared to incoming call
                        // scenario. Need to check if even in Single SIM, if
                        //dial is allowed when we already have two calls.
                        //In this case we can send 4,0 as it is valid on this sub
                        //Very less chance to have a headset doing this, but if the
                        //user explicitly tries to dial, we may end up here.
                        //Even is Single SIM , this scenario is not well known and
                        if (mBluetoothHeadset != null) {
                            log("call dial,Call join first");
                            mFakeMultiParty = true;
                            if (mBluetoothHeadset != null) {
                                mBluetoothHeadset.phoneStateChanged(1, 0,
                                                  CALL_STATE_IDLE, null, 0);
                                log("call dial,Send dial with held call");
                                mBluetoothHeadset.phoneStateChanged(0, 1,
                                                  callState, number, type);
                            }
                        }
                        mNumActive = 0;
                        mNumHeld = 1;
                    } else if (numActive + numHeld == 2) {
                        // Tossed up case.
                    }
                    break;

                case CALL_STATE_ALERTING:
                    //numHeld may be 1 here
                    if ((numActive + numHeld) < 2) {
                        //Just update the call state
                        mBluetoothHeadset.phoneStateChanged(0, 1,
                        callState, number, type);
                    }
                    break;

                default:
                    break;
            }
        } else if ((mNumActive == 1) || (mNumHeld == 1)) {
            //We have atleast one call.It could be active or held
            //just notify about the incoming call.
            switch(callState){
                case CALL_STATE_INCOMING:
                    //No change now, just send the new call states
                    Log.d(TAG,"Incoming call while we have active or held already present");
                    Log.d(TAG, " just update the new incoming call state");
                    if (mBluetoothHeadset != null) {
                        mBluetoothHeadset.phoneStateChanged(mNumActive, mNumHeld,
                        callState,number, type);
                    } else Log.d(TAG, "No need as headset is null");
                    break;

                case CALL_STATE_DIALING:
                    //We should see that active call is put on hold
                    //Since, we alread have one call, make sure q
                    //we are getting dial in already having call
                    if ((numActive == 0) && (numHeld == 1)) {
                        Log.d(TAG, "we are getting dial in already having call");
                        mNumActive = numActive;
                        mNumHeld = numHeld;
                    }
                    if(((otherSubActive == 1) || (otherSubHeld == 1)) &&
                        (numActive == 0)) {
                        log("This is new dial on this sub when a call is active on other sub");
                        //Make sure we send held=1 and active = 0
                        //Before dialing, the active call becomes held.
                        //In DSDA we have to fake it
                        mNumActive = 0;
                        mNumHeld = 1;
                    }
                    //Send the update
                    if (mBluetoothHeadset != null) {
                        mBluetoothHeadset.phoneStateChanged(mNumActive, mNumHeld,
                        callState,number, type);
                    } else Log.d(TAG, "No need as headset is null here");

                    break;

                case CALL_STATE_ALERTING:
                    //Just send update
                    Log.d(TAG, "Just send update for ALERT");
                    if (mBluetoothHeadset != null) {
                        mBluetoothHeadset.phoneStateChanged(mNumActive, mNumHeld,
                        callState,number, type);
                    } else Log.d(TAG, "No need as headset is null");

                    break;

                case CALL_STATE_IDLE:
                    //Call could be dropped by now.
                    //Here we have to decide if we need to update held call for
                    // switch SUB etc. Idle could be call accept or reject.Act
                    // properly
                    //Update the active call and held call only when they are zero
                    if (mNumActive == 0) {
                        if ((numActive == 1) || (otherSubActive == 1)) {
                            mNumActive = 1;
                            Log.d(TAG,"New active call on SUB: " + SUB);
                        }
                    } else if (mNumActive == 1) { /* Call dropped, update mNumActive properly*/
                        if(numActive == 0) {
                            Log.d(TAG,"Active Call state changed to 0: " + SUB);
                            if(otherSubActive == 0)
                                mNumActive = numActive;
                        }
                    }
                    if (mNumHeld == 1) {
                        //Update the values properly
                        log("Update the values properly");
                        if ((numActive + numHeld + otherSubActive +
                             otherSubHeld) < 2)
                            mNumHeld = ((numHeld + otherSubHeld) > 0)? 1:0;
                    } else {
                        //There is no held call
                        log("There was no held call previously");
                        if (((otherSubActive == 1) || (otherSubHeld == 1)) &&
                        ((numActive == 1) || (numHeld == 1))) {
                            // Switch SUB happened
                            //This will come for single sub case of 1 active, 1
                            // new call
                            Log.d(TAG,"Switch SUB happened, fake callheld");
                            mNumHeld = 1; // Fake 1 active , 1held, TRICKY
                        } else if (mNumHeld == 0) {
                            Log.d(TAG,"Update Held as value on this sub: " + numHeld);
                            mNumHeld = numHeld;
                        }
                    }
                    //This could be tricky as we may move suddenly from 4,0 t0 4,1
                    // even when  the new call was rejected.
                    if (mBluetoothHeadset != null) {
                        Log.d(TAG, "updating headset");
                        mBluetoothHeadset.phoneStateChanged(mNumActive,mNumHeld,
                                          callState,number, type);
                    } else Log.d(TAG, "No need as headset is null");

                    break;
            }
        } else{
            //This is first of the calls, update properly
            Log.d(TAG, "This is first of the calls, update properly");
            mNumActive = numActive;
            mNumHeld = numHeld;
            if (mBluetoothHeadset != null) {
                Log.d(TAG, "updating headset");
                mBluetoothHeadset.phoneStateChanged(mNumActive, mNumHeld,
                                 callState,number, type);
            } else Log.d(TAG, "No need as headset is null");
        }
        if (mFakeMultiParty == true) {
            if (((mSubscriptions[SUB1].mActive + mSubscriptions[SUB1].mHeld
                + mSubscriptions[SUB2].mActive + mSubscriptions[SUB2].mHeld) <= 2)
                && ((mSubscriptions[SUB1].mCallState == CALL_STATE_IDLE)
                && ((mSubscriptions[SUB2].mCallState == CALL_STATE_IDLE)))) {
                log("Reset mFakeMultiParty");
                mFakeMultiParty = false;
            }
        }
        mDsdaCallState = callState; //Know the call state
    }

    private CallNumber getCallNumber(Connection connection, Call call) {
        String number = null;
        int type = 128;
        // find phone number and type
        if (connection == null) {
            connection = call.getEarliestConnection();
            if (connection == null) {
                Log.e(TAG, "Could not get a handle on Connection object for the call");
            }
        }
        if (connection != null) {
            Log.e(TAG, "get a handle on Connection object for the call");
            number = connection.getAddress();
            if (number != null) {
                type = PhoneNumberUtils.toaFromString(number);
            }
        }
        if (number == null) {
            number = "";
        }
        return new CallNumber(number, type);
    }

    private class CallNumber
    {
        private String mNumber = null;
        private int mType = 0;

        private CallNumber(String number, int type) {
            mNumber = number;
            mType = type;
        }

        private boolean equalTo(CallNumber callNumber)
        {
            if (mType != callNumber.mType) return false;

            if (mNumber != null && mNumber.compareTo(callNumber.mNumber) == 0) {
                return true;
            }
            return false;
        }
    }

    /* Convert telephony phone call state into hf hal call state */
    static int convertCallState(Call.State ringingState, Call.State foregroundState) {
        if ((ringingState == Call.State.INCOMING) ||
            (ringingState == Call.State.WAITING) )
            return CALL_STATE_INCOMING;
        else if (foregroundState == Call.State.DIALING)
            return CALL_STATE_DIALING;
        else if (foregroundState == Call.State.ALERTING)
            return CALL_STATE_ALERTING;
        else
            return CALL_STATE_IDLE;
    }

    static int convertCallState(Call.State callState) {
        switch (callState) {
        case IDLE:
        case DISCONNECTED:
        case DISCONNECTING:
            return CALL_STATE_IDLE;
        case ACTIVE:
            return CALL_STATE_ACTIVE;
        case HOLDING:
            return CALL_STATE_HELD;
        case DIALING:
            return CALL_STATE_DIALING;
        case ALERTING:
            return CALL_STATE_ALERTING;
        case INCOMING:
            return CALL_STATE_INCOMING;
        case WAITING:
            return CALL_STATE_WAITING;
        default:
            Log.e(TAG, "bad call state: " + callState);
            return CALL_STATE_IDLE;
        }
    }

    private static void log(String msg) {
        if (DBG) Log.d(TAG, msg);
    }

    private final IBluetoothDsdaService.Stub mBinder = new IBluetoothDsdaService.Stub() {

        /* Handles call state changes on each subscription. */
        public void handleMultiSimPreciseCallStateChange() {
            enforceCallingOrSelfPermission(MODIFY_PHONE_STATE, null);
            Message msg = Message.obtain(mHandler, DSDA_CALL_STATE_CHANGED);
            mHandler.sendMessage(msg);
        }

        /* Set the current SUB*/
        public void setCurrentSub(int sub) {
            log("Call state changed on SUB: " + sub);
            mCurrentSub = sub;
        }

        public void phoneSubChanged() {
            enforceCallingOrSelfPermission(MODIFY_PHONE_STATE, null);
            Message msg = Message.obtain(mHandler, DSDA_PHONE_SUB_CHANGED);
            mHandler.sendMessage(msg);
        }

        /* Executes AT+CLCC for DSDA scenarios. */
        public void handleListCurrentCalls() {
            enforceCallingOrSelfPermission(MODIFY_PHONE_STATE, null);
            Message msg = Message.obtain(mHandler, DSDA_LIST_CURRENT_CALLS);
            mHandler.sendMessage(msg);
        }

        /* when HeadsetService is created,it queries for current phone
        state. This function provides the current state*/
        public void processQueryPhoneState() {
            log("Call state changed on SUB: ");
        }

        public void handleCdmaSetSecondCallState(boolean state) {
            if (VDBG) log("cdmaSetSecondCallState: Setting mCdmaIsSecondCallActive to " + state);
            Message msg = Message.obtain(mHandler, DSDA_UPDATE_CDMA_CALL_STATES);
            if (state == true)
                msg.arg1 = 1;
            else
                msg.arg1 = 0;
            mHandler.sendMessage(msg);
        }

        public void handleCdmaSwapSecondCallState() {
            log("DSDA: Toggling mCdmaIsSecondCallActive");
            Message msg = Message.obtain(mHandler, DSDA_SWAP_CDMA_SECOND_CALL_STATE);
        }

        public void setCurrentCallState(int currCallState, int prevCallState,
            boolean IsThreeWayCallOrigStateDialing) {
            if (VDBG) log("currCallState: " + currCallState +
                "prevCallState : " + prevCallState);
            Message msg = Message.obtain(mHandler, DSDA_SET_CDMA_CALL_STATES);
            msg.obj = new CdmaCallStates(currCallState, prevCallState,
                                            IsThreeWayCallOrigStateDialing);
            mHandler.sendMessage(msg);
        }

        public int getTotalCallsOnSub(int subId) {
            return ((mSubscriptions[subId].mActive +
                    mSubscriptions[subId].mHeld));
        }

        public boolean isSwitchSubAllowed() {
            boolean allowed = false;
            if ((((mSubscriptions[SUB1].mActive + mSubscriptions[SUB1].mHeld) == 1)
                && (mSubscriptions[SUB1].mCallState == CALL_STATE_IDLE))
                && (((mSubscriptions[SUB2].mActive +
                mSubscriptions[SUB2].mHeld) == 1)
                && (mSubscriptions[SUB2].mCallState == CALL_STATE_IDLE))) {
                allowed = true;
            }
            log("Is switch SUB allowed: " + allowed);
            return allowed;
        }

        public void SwitchSub() {
            enforceCallingOrSelfPermission(MODIFY_PHONE_STATE, null);
            Message msg = Message.obtain(mHandler, DSDA_SWITCH_SUB);
            mHandler.sendMessage(msg);
        }

        /* Check if call swap can be done on active SUB*/
        public boolean canDoCallSwap() {
            int active = mCM.getActiveSubscription();
            if (getTotalCallsOnSub(active) > 1)
                return true;
            return false;
        }

        public boolean answerOnThisSubAllowed() {
            log("answerOnThisSubAllowed.");
            int activeSub = mCM.getActiveSubscription();
            int bgSub =  -1;
            int count = MSimTelephonyManager.getDefault().getPhoneCount();
            for (int i = 0; i < count; i++) {
                if ((i != activeSub) && (mCM.getState(i) !=
                    PhoneConstants.State.IDLE)) {
                    log("getOtherActiveSub: active sub  = " + i );
                    bgSub = i;
                    break;
                }
            }
            if (bgSub == -1) /* No calls on bg sub*/
                return false;

            if( getTotalCallsOnSub(bgSub) >= 1)
                return true;
            return false;
        }

        public void updateCdmaHeldCall(int numheld) {
            mNumCdmaHeld = numheld;
        }

    };
}
