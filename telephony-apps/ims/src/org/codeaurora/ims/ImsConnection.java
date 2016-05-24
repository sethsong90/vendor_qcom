/*
 * Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 *
 * Not a Contribution, Apache license notifications and license are retained
 * for attribution purposes only.
 *
 * Copyright (C) 2006 The Android Open Source Project
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

package org.codeaurora.ims;

import java.util.Map;

import android.content.Context;
import android.os.AsyncResult;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.os.Registrant;
import android.os.SystemClock;
import android.telephony.PhoneNumberUtils;
import android.util.Log;
import android.text.TextUtils;
import android.widget.Toast;


import com.android.internal.telephony.Call;
import com.android.internal.telephony.CallManager;
import com.android.internal.telephony.CallStateException;
import com.android.internal.telephony.Connection;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.UUSInfo;
import com.qualcomm.ims.csvt.CsvtUtils;

import org.codeaurora.ims.conference.ConfInfo;
import com.android.internal.telephony.CallDetails;
import com.android.internal.telephony.CallModify;

/**
 * {@hide}
 */
public class ImsConnection extends Connection {
    static final String LOG_TAG = "IMSCONN";

    // ***** Instance Variables

    public ImsCallTracker owner;
    ImsCall parent;

    public String address; // MAY BE NULL!!!
    String dialString; // outgoing calls only
    String postDialString; // outgoing calls only
    public boolean isIncoming;
    boolean disconnected;
    public int index; // index in ImsCallTracker.connections[], -1 if unassigned
    // The Call index is 1 + this
    int mtCodeOnSsn = -1;
    private static final String MODIFY_CALL_FAILED_MESSAGE =
            "Modify Call request failed";
    private int mPrevVideoCallType = CallDetails.CALL_TYPE_UNKNOWN;
    private boolean mAvpRetryAllowed = true;
    private int mAvpCallType = CallDetails.CALL_TYPE_UNKNOWN;

    private int mMultiTaskRetryCount = 0;
    private static final int MAX_MULTITASK_RETRIES = 1;

    private enum PauseState {
        NONE, PAUSE, RESUME
    };

    private PauseState mPendingVTMultitask = PauseState.NONE;

    //Indicates if pause request sent from local UE succeeded
    private boolean mIsLocallyPaused = false;
    /*
     * These time/timespan values are based on System.currentTimeMillis(), i.e.,
     * "wall clock" time.
     */
    long createTime;
    public long connectTime;
    long disconnectTime;

    /*
     * These time/timespan values are based on SystemClock.elapsedRealTime(),
     * i.e., time since boot. They are appropriate for comparison and
     * calculating deltas.
     */
    long duration;
    long holdingStartTime; // The time when the Connection last transitioned
                           // into HOLDING

    int nextPostDialChar; // index into postDialString

    public DisconnectCause cause = DisconnectCause.NOT_DISCONNECTED;
    PostDialState postDialState = PostDialState.NOT_STARTED;
    int numberPresentation = PhoneConstants.PRESENTATION_ALLOWED;
                                                               // cdma
    UUSInfo uusInfo = null; // only used for GSM so it is null

    Handler h;

    private PowerManager.WakeLock mPartialWakeLock;
    public ConfInfo confDetailsMap = null;
    private Context mContext;

    // ***** Event Constants
    static final int EVENT_DTMF_DONE = 1;
    static final int EVENT_PAUSE_DONE = 2;
    static final int EVENT_NEXT_POST_DIAL = 3;
    static final int EVENT_WAKE_LOCK_TIMEOUT = 4;
    static final int EVENT_AVP_UPGRADE_DONE = 5;
    static final int EVENT_MODIFY_CALL_INITIATE_DONE = 6;
    static final int EVENT_VIDEO_PAUSE_DONE = 7;
    static final int EVENT_MODIFY_CALL_CONFIRM_DONE = 8;
    static final int EVENT_VIDEO_PAUSE_RETRY = 9;

    // ***** Constants
    static final int WAKE_LOCK_TIMEOUT_MILLIS = 60 * 1000;
    static final int PAUSE_DELAY_MILLIS = 3 * 1000;

    // ***** Inner Classes

    class MyHandler extends Handler {
        MyHandler(Looper l) {
            super(l);
        }

        public void handleMessage(Message msg) {
            AsyncResult ar;
            switch (msg.what) {
                case EVENT_NEXT_POST_DIAL:
                case EVENT_DTMF_DONE:
                case EVENT_PAUSE_DONE:
                    processNextPostDialChar();
                    break;
                case EVENT_WAKE_LOCK_TIMEOUT:
                    releaseWakeLock();
                    break;
                case EVENT_MODIFY_CALL_INITIATE_DONE:
                    onModifyCallInitiateDone(msg);
                    break;
                case EVENT_VIDEO_PAUSE_DONE:
                    onVideoPauseDone(msg);
                    break;
                case EVENT_MODIFY_CALL_CONFIRM_DONE:
                    log("EVENT_MODIFY_CALL_CONFIRM_DONE received");
                    clearPendingModify();
                    processPendingVTMultitask();
                    break;
                case EVENT_VIDEO_PAUSE_RETRY:
                    onVideoPauseRetry();
                    break;
                case EVENT_AVP_UPGRADE_DONE:
                    onAvpRetry(msg);
                    break;
            }
        }

        private void onModifyCallInitiateDone(Message msg) {
            log("EVENT_MODIFY_CALL_INITIATE_DONE received");
            AsyncResult ar = (AsyncResult) msg.obj;
            if (ar != null && ar.exception != null) {
                loge("videocall error during modifyCallInitiate");
                Toast.makeText(mContext, MODIFY_CALL_FAILED_MESSAGE, Toast.LENGTH_SHORT)
                        .show();
            }
            Message onComplete = (Message) ar.userObj;
            if (onComplete != null) {
                AsyncResult.forMessage(onComplete, ar.result, ar.exception);
                onComplete.sendToTarget();
            }
            clearPendingModify();
            processPendingVTMultitask();
        }

        private void onVideoPauseDone(Message msg) {
            AsyncResult ar;
            log("EVENT_VIDEO_PAUSE_DONE received");
            ar = (AsyncResult) msg.obj;
            if (ar != null) {
                if (ar.exception == null) {
                    // Success means VT Pause or Resume request succeeded.
                    // Update locally paused flag
                    if (callModifyRequest != null) {
                        mIsLocallyPaused = callModifyRequest.call_details.call_type
                                == CallDetails.CALL_TYPE_VT_NODIR;
                    }
                    clearPendingModify();
                    clearMultiTaskRetryCount();
                    processPendingVTMultitask();

                } else {
                    // Pause request failed, retry MAX_MULTITASK_RETRIES times
                    if (shouldRetryVideoPause()) {
                        loge("Error during video pause so retry");
                        h.sendMessageDelayed(h.obtainMessage(EVENT_VIDEO_PAUSE_RETRY),
                                PAUSE_DELAY_MILLIS);
                        mMultiTaskRetryCount++;
                    } else {
                        log("Video Pause retry limit reached.");
                        clearMultiTaskRetryCount();
                        clearPendingModify();
                        processPendingVTMultitask();
                    }
                }
            } else {
                loge("Error EVENT_VIDEO_PAUSE_DONE ar is null");
            }
        }

        private void onVideoPauseRetry() {
            log("EVENT_VIDEO_PAUSE_RETRY received mMultiTaskRetryCount=" + mMultiTaskRetryCount);
            if (mPendingVTMultitask == PauseState.NONE) {
                createAndSendMultiTaskRequest(callModifyRequest.call_details.call_type);
            } else {
                log("User pressed home/resume during retry so sending out new multitask request");
                clearPendingModify();
                clearMultiTaskRetryCount();
                processPendingVTMultitask();
            }
        }

        private void onAvpRetry(Message msg) {
            log("EVENT_AVP_UPGRADE received");
            AsyncResult ar = (AsyncResult) msg.obj;
            if (ar != null && ar.exception != null) {
                if (ar.userObj instanceof Boolean) {
                    boolean shouldNotifyUser = (Boolean) ar.userObj;
                    if (shouldNotifyUser) {
                        owner.phone
                                .notifyAvpUpgradeFailure(((AsyncResult) msg.obj).exception
                                        .toString());
                        Toast.makeText(mContext, MODIFY_CALL_FAILED_MESSAGE,
                                Toast.LENGTH_SHORT)
                                .show();
                    } else {
                        loge("AVP Retry error when Video call was dialed");
                    }
                }
            }
            clearPendingModify();
            processPendingVTMultitask();
        }

        private int clearMultiTaskRetryCount() {
            log("Clearing MultiTaskRetryCount from " + mMultiTaskRetryCount + " to 0");
            return mMultiTaskRetryCount = 0;
        }
    }

    class ImsDisconnectCause {
        public int mFailCause = -1;
        public String mErrorInfo = null;
    }

    public void setDiscImsErrorInfo(String errorinfostr) {
        if (errorinfostr != null) {
            errorInfo = errorinfostr;
            if (!CsvtUtils.isCsvtConnection(this)) {
                Toast.makeText(mContext, errorInfo, Toast.LENGTH_SHORT).show();
            }
        }
    }

    /*
     * Currently used by CSVT so method is retained.This method can be
     * deprecated since errorinfo is available through base class
     */
    public String getImsDisconnectCauseInfo() {
        String ret;
        if (errorInfo != null) {
            ret = errorInfo;
        } else {
            ret = new String("");
        }
        return ret;
    }
    // ***** Constructors

    /** This is probably an MT call that we first saw in a CLCC response */
    /* package */
    public ImsConnection(Context context, DriverCallIms dc, ImsCallTracker ct,
            int index) {
        mContext = context;
        createWakeLock(context);
        acquireWakeLock();

        owner = ct;
        h = new MyHandler(owner.getLooper());

        address = dc.number;

        isIncoming = dc.isMT;
        createTime = System.currentTimeMillis();
        mCnapName = dc.name; // TBD if present check
        mCnapNamePresentation = dc.namePresentation;
        numberPresentation = dc.numberPresentation;
        mtCodeOnSsn = -1;

        if (dc.callDetails == null) {
            Log.d(LOG_TAG, "Unexpected null for Call Details");
            return;
        }
        errorInfo = null;
        callDetails = dc.callDetails;
        callDetails.localAbility = dc.callDetails.localAbility;
        callDetails.peerAbility = dc.callDetails.peerAbility;
        parent = parentFromDCState(dc.state); // parent = call from ImsPhone
        confDetailsMap = parent.confDetailsMap;
        this.index = index;
        parent.attach(this, dc);
    }

    /** This is an MO call/three way call, created when dialing */
    /* package */
    public ImsConnection(Context context, String dialString, ImsCallTracker ct,
            ImsCall parent, CallDetails moCallDetails) {
        mContext = context;
        createWakeLock(context);
        acquireWakeLock();

        owner = ct;
        h = new MyHandler(owner.getLooper());

        this.dialString = dialString;
        this.address = dialString;
        this.postDialString = PhoneNumberUtils
                .extractPostDialPortion(dialString);
        if (callDetails != null && callDetails.extras != null) {
            String value = callDetails.getValueForKeyFromExtras(callDetails.extras,
                    CallDetails.EXTRAS_IS_CONFERENCE_URI);
            if (value != null && Boolean.valueOf(value)) {
                this.postDialString = "";
            }
        }
        index = -1;

        isIncoming = false;
        mCnapName = null;
        mCnapNamePresentation = PhoneConstants.PRESENTATION_ALLOWED;
        numberPresentation = PhoneConstants.PRESENTATION_ALLOWED;
        mtCodeOnSsn = -1;
        createTime = System.currentTimeMillis();
        callDetails = moCallDetails;
        callDetails.localAbility = moCallDetails.localAbility;
        callDetails.peerAbility = moCallDetails.peerAbility;
        errorInfo = null;
        if (parent != null) {
            this.parent = parent;
            confDetailsMap = parent.confDetailsMap;
            parent.attachFake(this, Call.State.DIALING, false);
        }
    }

    public void dispose() {
    }

    static boolean equalsHandlesNulls(Object a, Object b) {
        return (a == null) ? (b == null) : a.equals(b);
    }

    /* package */public boolean compareTo(DriverCallIms c) {
        // On mobile originated (MO) calls, the phone number may have changed
        // due to a SIM Toolkit call control modification.
        //
        // We assume we know when MO calls are created (since we created them)
        // and therefore don't need to compare the phone number anyway.
        if (!(isIncoming || c.isMT))
            return true;

        // ... but we can compare phone numbers on MT calls, and we have
        // no control over when they begin, so we might as well

        String cAddress = PhoneNumberUtils.stringFromStringAndTOA(c.number,
                c.TOA);
        return isIncoming == c.isMT && equalsHandlesNulls(address, cAddress);
    }

    public String getOrigDialString() {
        return dialString;
    }

    public String getAddress() {
        return address;
    }

    public String getCnapName() {
        return mCnapName;
    }

    public int getCnapNamePresentation() {
        return mCnapNamePresentation;
    }

    public Call getCall() {
        return parent;
    }

    public long getCreateTime() {
        return createTime;
    }

    public long getConnectTime() {
        return connectTime;
    }

    public void setConnectTime(long timeInMillis) {
        connectTime = timeInMillis;
    }

    public long getDisconnectTime() {
        return disconnectTime;
    }

    public long getDurationMillis() {
        if (mConnectTimeReal == 0) {
            return 0;
        } else if (duration == 0) {
            return SystemClock.elapsedRealtime() - mConnectTimeReal;
        } else {
            return duration;
        }
    }

    public long getHoldDurationMillis() {
        if (getState() != Call.State.HOLDING) {
            // If not holding, return 0
            return 0;
        } else {
            return SystemClock.elapsedRealtime() - holdingStartTime;
        }
    }

    public DisconnectCause getDisconnectCause() {
        return cause;
    }

    public boolean isIncoming() {
        return isIncoming;
    }

    public Call.State getState() {
        if (disconnected) {
            return Call.State.DISCONNECTED;
        } else {
            return super.getState();
        }
    }

    public void hangup() throws CallStateException {
        if (!disconnected) {
            if (parent.isMpty) {
                Log.d(LOG_TAG, "Multiparty call - hangupWithReason instead index = " + index +
                        " address =" + address);
                owner.hangupWithReason(getIndex(), address, null, true,
                        ImsQmiIF.CALL_FAIL_NORMAL, null);
            } else {
                owner.hangup(this);
            }
        } else {
            throw new CallStateException("disconnected");
        }
    }

    public void separate() throws CallStateException {
        if (!disconnected) {
            owner.separate(this);
        } else {
            throw new CallStateException("disconnected");
        }
    }

    public PostDialState getPostDialState() {
        return postDialState;
    }

    public void proceedAfterWaitChar() {
        if (postDialState != PostDialState.WAIT) {
            Log.w(LOG_TAG, "ConnectionBase.proceedAfterWaitChar(): Expected "
                    + "getPostDialState() to be WAIT but was " + postDialState);
            return;
        }

        setPostDialState(PostDialState.STARTED);

        processNextPostDialChar();
    }

    public void proceedAfterWildChar(String str) {
        if (postDialState != PostDialState.WILD) {
            Log.w(LOG_TAG, "CdmaConnection.proceedAfterWaitChar(): Expected "
                    + "getPostDialState() to be WILD but was " + postDialState);
            return;
        }

        setPostDialState(PostDialState.STARTED);

        // make a new postDialString, with the wild char replacement string
        // at the beginning, followed by the remaining postDialString.

        StringBuilder buf = new StringBuilder(str);
        buf.append(postDialString.substring(nextPostDialChar));
        postDialString = buf.toString();
        nextPostDialChar = 0;
        if (Phone.DEBUG_PHONE) {
            log("proceedAfterWildChar: new postDialString is " + postDialString);
        }

        processNextPostDialChar();

    }

    public void cancelPostDial() {
        setPostDialState(PostDialState.CANCELLED);
    }

    /**
     * Called when this Connection is being hung up locally (eg, user pressed
     * "end") Note that at this point, the hangup request has been dispatched to
     * the radio but no response has yet been received so update() has not yet
     * been called
     */
    public void onHangupLocal() {
        cause = DisconnectCause.LOCAL;
    }

    public void onRemoteDisconnect(int causeCode) {
        onDisconnect(owner.phone.disconnectCauseFromCode(causeCode));
    }

    /** Called when the radio indicates the connection has been disconnected */
    /*package*/ boolean
    onDisconnect(DisconnectCause cause) {
        boolean changed = false;
        this.cause = cause;
        if (!disconnected) {
            doDisconnect();
            Log.d(LOG_TAG, "onDisconnect: cause=" + cause);
            owner.phone.notifyDisconnect(this);

            if (parent != null) {
                changed = parent.connectionDisconnected(this);
            }
        }
        releaseWakeLock();
        return changed;
    }

    /** Called when the call waiting connection has been hung up */
    public void onLocalDisconnect() {
        if (!disconnected) {
            doDisconnect();
            Log.d(LOG_TAG, "onLoalDisconnect");

            if (parent != null) {
                parent.detach(this);
            }
        }
        releaseWakeLock();
    }

    private void clearPendingModify() {
        log("clearPendingModify imsconn=" + this);
        callModifyRequest = null;
    }

    public boolean isAvpRetryAllowed() {
        return mAvpRetryAllowed;
    }

    private void retryAvpUpgrade(boolean shouldNotifyUser) {
        log("retryAvpUpgrade: AVPF failed so retrying using AVP. mAvpCallType=" + mAvpCallType
                + " shouldNotifyUser=" + shouldNotifyUser);
        try {
                Message msg = h.obtainMessage(EVENT_AVP_UPGRADE_DONE, shouldNotifyUser);
                modifyCallInitiate(msg, mAvpCallType, null);
        } catch (CallStateException e) {
            Log.e(LOG_TAG + " videocall", "retryAvpUpgrade CallStateException " + e);
        }
    }

    private void updatePreviousVTCallType() {
        int callType = callDetails.call_type;
        if (callType == CallDetails.CALL_TYPE_VT
                || callType == CallDetails.CALL_TYPE_VT_TX
                || callType == CallDetails.CALL_TYPE_VT_RX) {
            mPrevVideoCallType = callDetails.call_type;
            log("Updating mPrevVideoCallType to " + mPrevVideoCallType);
        }
    }

    // Returns true if state has changed, false if nothing changed
    public boolean update(DriverCallIms dc) {
        ImsCall newParent;
        boolean changed = false;
        boolean wasConnectingInOrOut = isConnectingInOrOut();
        boolean wasHolding = (getState() == Call.State.HOLDING);
        int videoPauseState = CallDetails.VIDEO_PAUSE_STATE_RESUMED;

        newParent = parentFromDCState(dc.state); // parent = CdmaPhone

        if(ImsCallUtils.isVideoCallTypeWithDir(dc.callDetails.call_type)) {
            //Entered video call with peer so do not allow AVP retry for future
            mAvpRetryAllowed = false;
        }
        if (!ImsCallUtils.isVideoCall(dc.callDetails.call_type)) {
            log("videocall: update: Not a videocall CASE");
            clearPendingVTMultiTask();
            mMultiTaskRetryCount = 0;
            h.removeMessages(EVENT_VIDEO_PAUSE_RETRY);
        }
        boolean isAvpRetryDialingCase = ImsCallUtils.isAvpRetryDialing(this, dc);
        if (isAvpRetryDialingCase) {
            // Update AVP call type with current call type for dialed video call
            mAvpCallType = callDetails.call_type;
        }
        boolean isAvpRetryUpgradeCase = ImsCallUtils.isAvpRetryUpgrade(this, dc);
        if (isAvpRetryDialingCase || isAvpRetryUpgradeCase) {
            log("videocall AVP RETRY CASE dc= " + dc + " conn= " + this);
            retryAvpUpgrade(getState() == Call.State.ACTIVE);
        } else if (ImsCallUtils.isVideoPaused(this, dc)) {
            // Display toast Video Paused
            log("videocall Video Paused CASE");
            videoPauseState = CallDetails.VIDEO_PAUSE_STATE_PAUSED;
        } else if (ImsCallUtils.isVideoResumed(this, dc)) {
            // Display toast Video Resumed
            log("videocall Video Resumed CASE");
            videoPauseState = CallDetails.VIDEO_PAUSE_STATE_RESUMED;
        }

        updatePreviousVTCallType();

        CallDetails newDetails = new CallDetails(dc.callDetails);

        if (!callDetails.equals(dc.callDetails)) {
            Log.d(LOG_TAG, "[Connection " + index
                    + "] Updating call Details to " + newDetails);
            callDetails = newDetails;
            // Update call type to notify multitasking state to UI
            callDetails.setVideoPauseState(videoPauseState);
            //If video is paused then update UI call type with previous calltype
            if (videoPauseState == CallDetails.VIDEO_PAUSE_STATE_PAUSED) {
                log("Updating UI CallType from " + callDetails.call_type
                        + " to mPrevVideoCallType=" + mPrevVideoCallType);
                callDetails.call_type = mPrevVideoCallType;
            }

            changed = true;
        }

        // Reset the mIsLocallyPaused flag if call is not active videocall
        if (!ImsCallUtils.canVideoPause(this)) {
            mIsLocallyPaused = false;
        }

        if (!equalsHandlesNulls(address, dc.number)) {
            if (Phone.DEBUG_PHONE)
                log("update: phone # changed!");
            address = dc.number;
            changed = true;
        }

        // A null cnapName should be the same as ""
        if (TextUtils.isEmpty(dc.name)) {
            if (!TextUtils.isEmpty(mCnapName)) {
                changed = true;
                mCnapName = "";
            }
        } else if (!dc.name.equals(mCnapName)) {
            changed = true;
            mCnapName = dc.name;
        }

        if (Phone.DEBUG_PHONE) log("--dssds----"+mCnapName);
        mCnapNamePresentation = dc.namePresentation;
        numberPresentation = dc.numberPresentation;

        if (newParent != parent) {
            if (parent != null) {
                parent.detach(this);
            }
            newParent.attach(this, dc);
            parent = newParent;
            changed = true;
        } else {
            boolean parentStateChange;
            parentStateChange = parent.update(this, dc);
            changed = changed || parentStateChange;
        }

        /** Some state-transition events */

        if (Phone.DEBUG_PHONE)
            log("Update, wasConnectingInOrOut=" + wasConnectingInOrOut
                    + ", wasHolding=" + wasHolding + ", isConnectingInOrOut="
                    + isConnectingInOrOut() + ", changed=" + changed);

        if (wasConnectingInOrOut && !isConnectingInOrOut()) {
            onConnectedInOrOut();
        }

        if (changed && !wasHolding && (getState() == Call.State.HOLDING)) {
            // We've transitioned into HOLDING
            onStartedHolding();
        }

        return changed;
    }

    /**
     * Called when this Connection is in the foregroundCall when a dial is
     * initiated. We know we're ACTIVE, and we know we're going to end up
     * HOLDING in the backgroundCall
     */
    public void fakeHoldBeforeDial() {
        boolean multiparty = false;

        if (parent != null) {
            multiparty = parent.isMpty;
            log("FakeHold case cached mpty: " + multiparty);
            parent.detach(this);
        }

        parent = owner.backgroundCall;
        parent.attachFake(this, Call.State.HOLDING, multiparty);

        onStartedHolding();
    }

    public int getIndex() throws CallStateException {
        if (index >= 0) {
            return index + 1;
        } else {
            throw new CallStateException("CDMA connection index not assigned");
        }
    }

    /**
     * An incoming or outgoing call has connected
     */
    public void onConnectedInOrOut() {
        connectTime = System.currentTimeMillis();
        mConnectTimeReal = SystemClock.elapsedRealtime();
        duration = 0;

        // bug #678474: incoming call interpreted as missed call, even though
        // it sounds like the user has picked up the call.
        if (Phone.DEBUG_PHONE) {
            log("onConnectedInOrOut: connectTime=" + connectTime);
        }

        if (!isIncoming) {
            // outgoing calls only
            processNextPostDialChar();
        } else {
            // Only release wake lock for incoming calls, for outgoing calls the
            // wake lock
            // will be released after any pause-dial is completed
            releaseWakeLock();
        }
    }

    private void doDisconnect() {
        index = -1;
        disconnectTime = System.currentTimeMillis();
        duration = SystemClock.elapsedRealtime() - mConnectTimeReal;
        disconnected = true;
        mtCodeOnSsn = -1;
        confDetailsMap.dispose();
    }

    private void onStartedHolding() {
        holdingStartTime = SystemClock.elapsedRealtime();
    }

    /**
     * Performs the appropriate action for a post-dial char, but does not notify
     * application. returns false if the character is invalid and should be
     * ignored
     */
    private boolean processPostDialChar(char c) {
        if (PhoneNumberUtils.is12Key(c)) {
            owner.cm.sendDtmf(c, h.obtainMessage(EVENT_DTMF_DONE));
        } else if (c == PhoneNumberUtils.PAUSE) {
            setPostDialState(PostDialState.PAUSE);// TBD check why it is not in
                                                  // Gsm
            /*
             * From TS 22.101: It continues... Upon the called party answering
             * the UE shall send the DTMF digits automatically to the network
             * after a delay of 3 seconds(plus/minus 20 %). The digits shall be
             * sent according to the procedures and timing specified in 3GPP TS
             * 24.008 [13]. The first occurrence of the
             * "DTMF Control Digits Separator" shall be used by the ME to
             * distinguish between the addressing digits (i.e. the phone number)
             * and the DTMF digits. Upon subsequent occurrences of the
             * separator, the UE shall pause again for 3 seconds before sending
             * any further DTMF digits.
             */

            // TBD 3s for gsm n 2s for cdma check specs
            h.sendMessageDelayed(h.obtainMessage(EVENT_PAUSE_DONE),
                    PAUSE_DELAY_MILLIS);
        } else if (c == PhoneNumberUtils.WAIT) {
            setPostDialState(PostDialState.WAIT);
        } else if (c == PhoneNumberUtils.WILD) {
            setPostDialState(PostDialState.WILD);
        } else {
            return false;
        }

        return true;
    }

    public String getRemainingPostDialString() {
        if (postDialState == PostDialState.CANCELLED
                || postDialState == PostDialState.COMPLETE
                || postDialString == null
                || postDialString.length() <= nextPostDialChar) {
            return "";
        }

        String subStr = postDialString.substring(nextPostDialChar);

        if (subStr != null) {
            int wIndex = subStr.indexOf(PhoneNumberUtils.WAIT);
            int pIndex = subStr.indexOf(PhoneNumberUtils.PAUSE);

            if (wIndex > 0 && (wIndex < pIndex || pIndex <= 0)) {
                subStr = subStr.substring(0, wIndex);
            } else if (pIndex > 0) {
                subStr = subStr.substring(0, pIndex);
            }
        }
        return subStr;
    }

    public void updateParent(ImsCall oldParent, ImsCall newParent) {
        boolean multiparty = false;

        if (newParent != oldParent) {
            if (oldParent != null) {
                multiparty = oldParent.isMpty;
                log("updateParent cached mpty: " + multiparty);
                oldParent.detach(this);
            }
            newParent.attachFake(this, Call.State.ACTIVE, multiparty);
            parent = newParent;
        }
    }

    @Override
    protected void finalize() {
        /**
         * It is understood that This finializer is not guaranteed to be called
         * and the release lock call is here just in case there is some path
         * that doesn't call onDisconnect and or onConnectedInOrOut.
         */
        if (mPartialWakeLock.isHeld()) {
            Log.e(LOG_TAG,
                    "UNEXPECTED; mPartialWakeLock is held when finalizing.");
        }
        releaseWakeLock();
    }

    void processNextPostDialChar() {
        char c = 0;
        Registrant postDialHandler;

        if (postDialState == PostDialState.CANCELLED) {
            releaseWakeLock();
            // Log.v("CDMA",
            // "##### processNextPostDialChar: postDialState == CANCELLED, bail");
            return;
        }

        if (postDialString == null
                || postDialString.length() <= nextPostDialChar) {
            setPostDialState(PostDialState.COMPLETE);

            // We were holding a wake lock until pause-dial was complete, so
            // give it up now
            releaseWakeLock();

            // notifyMessage.arg1 is 0 on complete
            c = 0;
        } else {
            boolean isValid;

            setPostDialState(PostDialState.STARTED);

            c = postDialString.charAt(nextPostDialChar++);

            isValid = processPostDialChar(c);

            if (!isValid) {
                // Will call processNextPostDialChar
                h.obtainMessage(EVENT_NEXT_POST_DIAL).sendToTarget();
                // Don't notify application
                Log.e("CDMA", "processNextPostDialChar: c=" + c
                        + " isn't valid!");
                return;
            }
        }

        postDialHandler = owner.phone.mPostDialHandler;

        Message notifyMessage;

        if (postDialHandler != null
                && (notifyMessage = postDialHandler.messageForRegistrant()) != null) {
            // The AsyncResult.result is the Connection object
            PostDialState state = postDialState;
            AsyncResult ar = AsyncResult.forMessage(notifyMessage);
            ar.result = // cdma specific - check what is done here this;
            ar.userObj = state;

            // arg1 is the character that was/is being processed
            notifyMessage.arg1 = c;

            notifyMessage.sendToTarget();
        }
    }

    /**
     * "connecting" means "has never been ACTIVE" for both incoming and outgoing
     * calls
     */
    private boolean isConnectingInOrOut() {
        return parent == null || parent == owner.ringingCall
                || parent.mState == Call.State.DIALING
                || parent.mState == Call.State.ALERTING;
    }

    private ImsCall parentFromDCState(DriverCallIms.State state) {
        switch (state) {
            case ACTIVE:
            case DIALING:
            case ALERTING:
                return owner.foregroundCall;
            case HOLDING:
                return owner.backgroundCall;
            case INCOMING:
            case WAITING:
                return owner.ringingCall;
            default:
                throw new RuntimeException("illegal call state: " + state);
        }
    }

    /**
     * Set post dial state and acquire wake lock while switching to "started" or
     * "wait" state, the wake lock will be released if state switches out of
     * "started" or "wait" state or after WAKE_LOCK_TIMEOUT_MILLIS.
     *
     * @param s new PostDialState
     */
    private void setPostDialState(PostDialState s) {
        if (s == PostDialState.STARTED || s == PostDialState.PAUSE) {
            synchronized (mPartialWakeLock) {
                if (mPartialWakeLock.isHeld()) {
                    h.removeMessages(EVENT_WAKE_LOCK_TIMEOUT);
                } else {
                    acquireWakeLock();
                }
                Message msg = h.obtainMessage(EVENT_WAKE_LOCK_TIMEOUT);
                h.sendMessageDelayed(msg, WAKE_LOCK_TIMEOUT_MILLIS);
            }
        } else {
            h.removeMessages(EVENT_WAKE_LOCK_TIMEOUT);
            releaseWakeLock();
        }
        postDialState = s;
    }

    private void createWakeLock(Context context) {
        PowerManager pm = (PowerManager) context
                .getSystemService(Context.POWER_SERVICE);
        mPartialWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,
                LOG_TAG);
    }

    private void acquireWakeLock() {
        log("acquireWakeLock");
        mPartialWakeLock.acquire();
    }

    private void releaseWakeLock() {
        synchronized (mPartialWakeLock) {
            if (mPartialWakeLock.isHeld()) {
                log("releaseWakeLock");
                mPartialWakeLock.release();
            }
        }
    }

    private void log(String msg) {
        Log.d(LOG_TAG, "" + msg);
    }

    private void loge(String msg) {
        Log.e(LOG_TAG, "" + msg);
    }

    public String toString() {
        return "conn: index=" + index + " callModifyRequest= " + callModifyRequest +
                " callDetails= " + callDetails + " mPendingVTMultitask=" + mPendingVTMultitask +
                " mIsLocallyPaused=" + mIsLocallyPaused + " mAvpCallType=" + mAvpCallType;
    }

    @Override
    public int getNumberPresentation() {
        return numberPresentation;
    }

    @Override
    public UUSInfo getUUSInfo() {
        return uusInfo;
    }

    public CallDetails getCallDetails() {
        return this.callDetails;
    }

    private boolean isServiceAllowed(int srvType, CallDetails.ServiceStatus[] ability) {
        boolean allowed = true;
        if(ability != null) {
            for (CallDetails.ServiceStatus srv : ability) {
                if (srv != null && srv.type == srvType) {
                    if (srv.status == ImsQmiIF.STATUS_DISABLED ||
                            srv.status == ImsQmiIF.STATUS_NOT_SUPPORTED) {
                        allowed = false;
                    }
                    break;
                }
            }
        }
        return allowed;
    }

    public boolean isVTAllowed() {
        boolean allowed = true;
        allowed = isServiceAllowed(CallDetails.CALL_TYPE_VT, this.callDetails.localAbility);
        if (allowed) {
            allowed = isServiceAllowed(CallDetails.CALL_TYPE_VT, this.callDetails.peerAbility);
        }
        return allowed;
    }

    public boolean isVoiceAllowed() {
        boolean allowed = true;
        allowed = isServiceAllowed(CallDetails.CALL_TYPE_VOICE, this.callDetails.localAbility);
        if (allowed) {
            allowed = isServiceAllowed(CallDetails.CALL_TYPE_VOICE, this.callDetails.peerAbility);
        }
        return allowed;
    }

    /**
     * getConnectionType()
     *
     * @return The current connection type or -1 if callDetails is null
     */
    public int getConnectionType() {
        if (callDetails != null) {
            return callDetails.call_type;
        } else {
            return -1;
        }
    }

    // This function validates if the call can be modified to the given call type
    private boolean validateCanModifyConnectionType(Message msg, int newCallType) {


        boolean ret = false;
        Call call = getCall();
        boolean isMP = call != null && (call.isMultiparty()); // Is it a Multiparty call?

        final int videoPauseState = callDetails.getVideoPauseState();
        ret = ((!isMP) && index >= 0 &&
                (newCallType != callDetails.call_type
                || videoPauseState == CallDetails.VIDEO_PAUSE_STATE_PAUSED));

        log("isMP=" + isMP + " index=" + index + " newCallType=" + newCallType
                + " Current callType=" + callDetails.call_type + " VideoPauseState="
                + videoPauseState);
        if (!ret && (msg != null)) {
            AsyncResult ar;

            String s = "";
            if (isMP) {
                s += "Call is Multiparty. ";
            }

            if (index < 0) {
                s += "Index is not yet assigned. ";
            }

            if (newCallType != getCallDetails().call_type) {
                // Call type is not same as requested. Reply with success.
                ar = AsyncResult.forMessage(msg, null, null);
            } else {
                ar = AsyncResult.forMessage(msg, null, new Exception("Unable to change: " + s));
            }

            msg.obj = ar;
            msg.sendToTarget();
        }
        return ret;
    }

    private boolean isOldAndNewPauseRequestSame() {
        boolean ret = false;
        if (callModifyRequest != null) {
            loge("isOldAndNewPauseRequestSame Unexpectedly callModifyRequest:" + callModifyRequest);
            return ret;
        }
        ret = (mIsLocallyPaused && mPendingVTMultitask == PauseState.PAUSE)
                || (!mIsLocallyPaused && mPendingVTMultitask == PauseState.RESUME);
        log("isOldAndNewPauseRequestSame " + ret);
        return ret;
    }

    private void clearPendingVTMultiTask() {
        log("clearPendingVTMultiTask imsconn=" + this);
        mPendingVTMultitask = PauseState.NONE;
    }

    private boolean shouldRetryVideoPause(){
        return (mMultiTaskRetryCount <= MAX_MULTITASK_RETRIES);
    }

    // MODIFY_CALL_INITIATE
    public void changeConnectionType(Message msg,
            int newCallType, Map<String, String> newExtras) throws CallStateException {

        log("changeConnectionType index= " + index + " newCallType=" + newCallType + " newExtras= "
                + newExtras);
        if (isVTMultitaskRequest(newCallType)) {
            //Video pause/resume request
            triggerOrQueueVTMultitask(newCallType);
        } else {
            //Regular upgrade/downgrade request
            if (isAvpRetryAllowed() && ImsCallUtils.isVideoCallTypeWithDir(newCallType)) {
                mAvpCallType = newCallType;
            }

            Message newMsg = h.obtainMessage(EVENT_MODIFY_CALL_INITIATE_DONE, msg);
            if (callModifyRequest == null) {
                if (validateCanModifyConnectionType(newMsg, newCallType)) {
                    modifyCallInitiate(newMsg, newCallType, newExtras);
                }
            } else {
                Log.e(LOG_TAG,
                        "videocall changeConnectionType: not invoking modifyCallInitiate "
                                + "as there is pending callModifyRequest="
                                + callModifyRequest);
                String errorStr = "Pending callModifyRequest so not sending modify request down";
                RuntimeException ex = new RuntimeException(errorStr);
                if (msg != null) {
                    AsyncResult.forMessage(msg, null, ex);
                    msg.sendToTarget();
                }
            }
        }
    }

    private void modifyCallInitiate(Message newMsg, int newCallType, Map<String, String> newExtras)
            throws CallStateException {
        if(!ImsCallUtils.isValidRilModifyCallType(newCallType)) {
            loge("modifyCallInitiate not a Valid RilCallType" + newCallType);
            return;
        }
        CallDetails callDetails = new CallDetails(newCallType,
                getCallDetails().call_domain,
                CallDetails.getExtrasFromMap(newExtras));
        int rilCallIndex = index + 1;
        CallModify callModify = new CallModify(callDetails, rilCallIndex);
        // Store the outgoing modify call request in the connection
        if (callModifyRequest != null) {
            log("Overwriting callModifyRequest: " + callModifyRequest + " with callModify:"
                    + callModify);
        }
        callModifyRequest = callModify;
        owner.modifyCallInitiate(newMsg, callModify);
    }

    private boolean isVTMultitaskRequest(int callType) {
        return callType == CallDetails.CALL_TYPE_VT_PAUSE
                || callType == CallDetails.CALL_TYPE_VT_RESUME;
    }

    private void triggerOrQueueVTMultitask(int callType) throws CallStateException {
        log("triggerOrQueueVTMultitask callType= " + callType + " conn= " + this);
        boolean isPauseRequested = callType == CallDetails.CALL_TYPE_VT_PAUSE;
        // Update the pending pause flag
        mPendingVTMultitask = isPauseRequested ? PauseState.PAUSE : PauseState.RESUME;
        if (callModifyRequest == null) {
            processPendingVTMultitask();
        }
    }

    private int toRilCallType(int callType) {
        int rilCallType = CallDetails.CALL_TYPE_UNKNOWN;
        if (callType == CallDetails.CALL_TYPE_VT_PAUSE) {
            rilCallType = CallDetails.CALL_TYPE_VT_NODIR;
        } else if (callType == CallDetails.CALL_TYPE_VT_RESUME) {
            rilCallType = mPrevVideoCallType;
        } else {
            loge("toRilCallType unexpected calltype for VT multitask="
                    + callType);
        }
        return rilCallType;
    }

    private void createAndSendMultiTaskRequest(int rilCallType) {
        try {
            h.removeMessages(EVENT_VIDEO_PAUSE_RETRY);
            Message message = h.obtainMessage(EVENT_VIDEO_PAUSE_DONE);
            modifyCallInitiate(message, rilCallType, null);
        } catch (CallStateException e) {
            loge("createAndSendMultiTaskRequest CallStateException");
            e.printStackTrace();
        }
    }

    private int pendingPauseStatetoRilCallType() {
        int rilCallType = CallDetails.CALL_TYPE_UNKNOWN;
        if (mPendingVTMultitask == PauseState.PAUSE) {
            rilCallType = CallDetails.CALL_TYPE_VT_NODIR;
        }
        else if (mPendingVTMultitask == PauseState.RESUME) {
            rilCallType = mPrevVideoCallType;
        }
        return rilCallType;
    }

    private void processPendingVTMultitask() {
        log("processPendingVTMultitask mPendingVTMultitask=" + mPendingVTMultitask);
        if (isOldAndNewPauseRequestSame()) {
            log("Old and new Pause Request is Same so clearing Pending VT multitask");
            clearPendingVTMultiTask();
        } else if (mPendingVTMultitask != PauseState.NONE) {
            if (callModifyRequest == null) {
                createAndSendMultiTaskRequest(pendingPauseStatetoRilCallType());
                clearPendingVTMultiTask();
            } else {
                loge("processPendingVTMultitask callModifyRequest not null");
            }

        }
    }

    // UNSOL_MODIFY_CALL
    public boolean onReceivedModifyCall(CallModify callModify) {
        Log.d(LOG_TAG, "onReceivedCallModify(" + callModify + ")");

        Message msg = null;
        boolean ret = false;
        // Is this a new modify call request or an error notification for
        // the previous modify call request?
        if ( callModify.error() ) {
            if (this.callModifyRequest != null
                    && this.callModifyRequest.call_index == callModify.call_index
                    && this.callModifyRequest.call_details.call_type
                        == callModify.call_details.call_type
                    && this.callModifyRequest.call_details.call_domain
                        == callModify.call_details.call_domain
                    && !this.callModifyRequest.error()) {
                // Update the previous request.
                this.callModifyRequest.error = callModify.error;
                ret = true;
            } else {
                Log.e(LOG_TAG, "onReceivedModifyCall: Call Modify request not found."
                        + "Dropping the Modify Call Request Failed. Cached Request: "
                        + this.callModifyRequest + ", Received Request:" + callModify);
                ret = false;
            }
        } else { // This a new modify call request.
            ret = validateCanModifyConnectionType(msg, callModify.call_details.call_type);
            if (this.callModifyRequest == null) {
                this.callModifyRequest = callModify;
            } else {
                Log.e(LOG_TAG, "videocall onReceivedModifyCall: not notifying user about"
                        + " incoming modify call request as there is pending callModifyRequest="
                        + callModifyRequest);
                ret = false;
            }

            if (! ret) {
                try {
                    rejectConnectionTypeChange();
                } catch (CallStateException ex) {
                    Log.e(LOG_TAG, "Exception while rejecting ConnectionTypeChange", ex);
                }
            }
        }
        return ret;
    }

    /**
     * When a remote user requests to change the type of the connection
     * (e.g. to upgrade from voice to video), it will be possible to
     * query the proposed type with this method. After receiving an indication of a request
     * (see {@link CallManager#registerForConnectionTypeChangeRequest(Handler, int, Object)}).
     *
     * If no request has been received, this function returns the current type.
     * The proposed type is cleared after calling {@link #acceptConnectionTypeChange(Map)} or
     * {@link #rejectConnectionTypeChange()}.
     *
     * @return The proposed connection type or the current connectionType if no request exists.
     */
    public int getProposedConnectionType() {
        int ret = getConnectionType();

        if (callModifyRequest != null) {
            if (callModifyRequest.call_details != null) {
                ret = callModifyRequest.call_details.call_type;
            } else {
                Log.d(LOG_TAG, "Received callModifyRequest without call details");
            }
        }

        return ret;
    }

    /**
     * The system notifies about the failure (e.g. timeout) of the previous
     * request to change the type of the connection by re-sending the modify
     * connection type request with the status set to fail.After receiving an
     * indication of call modify request it will be possible to query for the
     * status of the request.(see
     * {@link CallManager#registerForConnectionTypeChangeRequest(Handler, int, Object)}
     * ) If no request has been received, this function returns false, no error.
     *
     * @return true if the proposed connection type request failed (e.g.
     *         timeout).
     */
    public boolean getProposedConnectionFailed() {

        if (callModifyRequest != null) {
            return callModifyRequest.error();
        } else {
            Log.d(LOG_TAG, "getProposedConnectionFailed: callModifyRequest is null");
        }

        return false;
    }

   /**
     * Approve a request to change the call type. Optionally, provide new extra values.
     *
     * @param newExtras
     * @throws CallStateException
     */
    public void acceptConnectionTypeChange(Map<String, String> newExtras)
            throws CallStateException {
        Log.d(LOG_TAG, "Confirming call type change request: " + callModifyRequest);

        if (callModifyRequest != null) {
            if (newExtras != null) {
                callModifyRequest.call_details.setExtrasFromMap(newExtras);
            }
            Message newMsg = h.obtainMessage(EVENT_MODIFY_CALL_CONFIRM_DONE);
            owner.modifyCallConfirm(newMsg, callModifyRequest);
        }
    }

    /**
     * Reject a previously received request to change the call type.
     *
     * @throws CallStateException
     */
    public void rejectConnectionTypeChange() throws CallStateException {
        if (callModifyRequest == null) {
            Log.e(LOG_TAG, "rejectConnectionTypeChange callModifyRequest is null");
            return;
        } else if ( callModifyRequest.error() ) {
            Log.d(LOG_TAG, "rejectConnectionTypeChange callModifyRequest timed out.");
            clearPendingModify();
            return; // Don't notify lower layers. They are aware of this.
        }
        CallModify callModify = new CallModify();
        callModify.call_index = index + 1;
        callModify.call_details = new CallDetails(callDetails);

        Log.d(LOG_TAG, "Rejecting Change request: " + callModifyRequest + " keep as " + callModify);

        Message newMsg = h.obtainMessage(EVENT_MODIFY_CALL_CONFIRM_DONE);
        owner.modifyCallConfirm(newMsg, callModify);
    }

}
