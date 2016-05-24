/*
 * Copyright (c) 2012-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
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

import java.util.ArrayList;
import java.util.List;

import android.os.AsyncResult;
import android.os.Handler;
import android.os.Message;
import android.os.Registrant;
import android.os.RegistrantList;
import android.os.SystemProperties;
import android.telephony.PhoneNumberUtils;
import android.telephony.ServiceState;
import android.text.TextUtils;
import android.util.Log;
import android.widget.Toast;
import android.os.Build;
import com.android.internal.telephony.Call;
import com.android.internal.telephony.CallManager;
import com.android.internal.telephony.CallStateException;
import com.android.internal.telephony.CallTracker;
import com.android.internal.telephony.Connection;
import com.android.internal.telephony.DriverCall;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.TelephonyProperties;

import com.android.internal.telephony.CallDetails;
import com.android.internal.telephony.CallModify;
import org.codeaurora.ims.CallFailCause;

/**
 * {@hide}
 */
public final class ImsCallTracker extends CallTracker {
    static final String LOG_TAG = "IMSCallTracker";

    private static final boolean DBG_POLL = true;

    // ***** Constants

    public static final int MAX_CONNECTIONS = 7;
    public static final int MAX_CONNECTIONS_PER_CALL = 5;

    private static final boolean VDBG = true;

    protected static final int EVENT_IMS_STATE_CHANGED = 21;
    protected static final int EVENT_MODIFY_CALL = 22;
    protected static final int EVENT_TTY_STATE_CHANGED = 23;
    protected static final int EVENT_RADIO_STATE_CHANGED = 24;
    protected static final int EVENT_HANDOVER_STATE_CHANGED = 25;
    protected static final int EVENT_SET_SERVICE_STATUS = 26;

    public ImsCall ringingCall;
    // A call that is ringing or (call) waiting
    public ImsCall foregroundCall;
    public ImsCall backgroundCall;

    public PhoneConstants.State state = PhoneConstants.State.IDLE;
    public boolean mIsInEmergencyCall = false;
    public boolean callSwitchPending = false;
    private boolean mPendingHandover = false;

    // ***** Instance Variables
    ImsConnection connections[] = new ImsConnection[MAX_CONNECTIONS];

    // connections dropped during last poll
    ArrayList<ImsConnection> droppedDuringPoll = new ArrayList<ImsConnection>(
            MAX_CONNECTIONS);

    ImsConnection pendingMO;
    boolean hangupPendingMO;
    boolean pendingCallInEcm = false;

    boolean desiredMute = false; // false = mute off

    int pendingCallClirMode;

    private boolean mIsEcmTimerCanceled = false;

    public ImsPhone phone;
    public ImsSenderRxr cm;

    public RegistrantList imsCallEndedRegistrants = new RegistrantList();
    public RegistrantList imsCallStartedRegistrants = new RegistrantList();
    public RegistrantList callWaitingRegistrants = new RegistrantList();

    // ***** Events

    // ***** Constructors
    public ImsCallTracker(ImsPhone imsPhone) {
        this.phone = imsPhone;
        Log.i(LOG_TAG, " phone object in constructor " + phone);

        cm = (ImsSenderRxr)phone.mCi;
        super.mCi = cm;
        cm.registerForCallStateChanged(this, EVENT_CALL_STATE_CHANGE, null);
        cm.registerForImsNetworkStateChanged(this, EVENT_IMS_STATE_CHANGED,
                null);
        cm.registerForModifyCall(this, EVENT_MODIFY_CALL, null);
        cm.registerForTtyStatusChanged(this, EVENT_TTY_STATE_CHANGED, null);
        cm.registerForRadioStateChanged(this, EVENT_RADIO_STATE_CHANGED, null);
        cm.registerForHandoverStatusChanged(this, EVENT_HANDOVER_STATE_CHANGED,
                null);
        ringingCall = new ImsCall(this);
        // A call that is ringing or (call) waiting -
        // These are calls of CdmaPhone

        foregroundCall = new ImsCall(this);
        backgroundCall = new ImsCall(this);
        foregroundCall.setGeneric(false);
    }

    public void dispose() {
        cm.unregisterForCallStateChanged(this);
        cm.unregisterForCallWaitingInfo(this);
        cm.unregisterForModifyCall(this);
        cm.unregisterForTtyStatusChanged(this);
        cm.unregisterForHandoverStatusChanged(this);
        for (ImsConnection c : connections) {
            try {
                if (c != null)
                    hangup(c);
            } catch (CallStateException ex) {
                Log.e(LOG_TAG, "unexpected error on hangup during dispose");
            }
        }

        try {
            if (pendingMO != null) {
                hangup(pendingMO);
                Log.d(LOG_TAG,
                        "Posting disconnect to pendingMO due to LOST_SIGNAL");
                pendingMO.onDisconnect(Connection.DisconnectCause.LOST_SIGNAL);
            }
        } catch (CallStateException ex) {
            Log.e(LOG_TAG, "unexpected error on hangup during dispose");
        }

        clearDisconnected();
    }

    protected void finalize() {
        Log.d(LOG_TAG, "ImsCallTracker finalized");
    }

    public static class HandoverInfo {
        public int mType = 0;
        public int mSrcTech = 0;
        public int mTargetTech = 0;
        public int mExtraType = 0;
        public byte[] mExtraInfo = null;
    }

    // ***** Instance Methods

    // ***** Public Methods

    public void registerForVoiceCallStarted(Handler h, int what, Object obj) {
        logUnexpectedMethodCall("registerForVoiceCallStarted");
    }

    public void unregisterForVoiceCallStarted(Handler h) {
        logUnexpectedMethodCall("unregisterForVoiceCallStarted");
    }

    public void registerForVoiceCallEnded(Handler h, int what, Object obj) {
        logUnexpectedMethodCall("registerForVoiceCallEnded");
    }

    public void unregisterForVoiceCallEnded(Handler h) {
        logUnexpectedMethodCall("unregisterForVoiceCallEnded");
    }

    public void registerForImsCallStarted(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        imsCallStartedRegistrants.add(r);
        // Notify if in call when registering
        if (phone.getState() != PhoneConstants.State.IDLE) {
            r.notifyRegistrant(new AsyncResult(null, null, null));
        }
    }

    public void unregisterForImsCallStarted(Handler h) {
        imsCallStartedRegistrants.remove(h);
    }

    public void registerForImsCallEnded(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        imsCallEndedRegistrants.add(r);
    }

    public void unregisterForImsCallEnded(Handler h) {
        imsCallEndedRegistrants.remove(h);
    }

    private void fakeHoldForegroundBeforeDial() {
        // this method is for IMS calls only
        List<Connection> connCopy;

        // We need to make a copy here, since fakeHoldBeforeDial()
        // modifies the lists, and we don't want to reverse the order
        connCopy = (List<Connection>) foregroundCall.connections.clone();

        for (int i = 0, s = connCopy.size(); i < s; i++) {
            ImsConnection conn = (ImsConnection) connCopy.get(i);
            conn.fakeHoldBeforeDial();
        }
    }

    public boolean isAddParticipantAllowed() {
        // Just check the fg call state here as the check for
        // adding call is done in InCallControlState using canAddCall
        return (foregroundCall.getState() == Call.State.ACTIVE);
    }

    public void addParticipant(String dialString, int clirMode,
            CallDetails callDetails) {
        Log.d (LOG_TAG, "addParticipant string = " + dialString + "clirMode =" + clirMode);
        cm.addParticipant(dialString, clirMode, callDetails, obtainCompleteMessage());
    }

    public Connection dial(String dialString, int clirMode,
            CallDetails callDetails) throws CallStateException {
        // flag used to determine if cm.dial needs to be sent now or later
        boolean isDialRequestPending = false;
        boolean isEmergencyNumber = phone.isExactOrPotentialLocalEmergencyNumber(dialString);
        int serviceState = phone.getServiceState().getState();
        boolean isPhoneInEcmMode = SystemProperties.getBoolean(
                TelephonyProperties.PROPERTY_INECM_MODE, false);

        Log.d(LOG_TAG, "dialphone is " + phone + "call details" + callDetails);

        if (!canDial() ||
                (serviceState != ServiceState.STATE_IN_SERVICE && !isEmergencyNumber)) {
            // Only emergency call can be dialed when there's no IMS service
            throw new CallStateException("cannot dial in current state");
        }

        // note that this triggers call state changed notify
        clearDisconnected();

        // Cancel Ecm timer if a second emergency call is originating in Ecm mode
        if (isPhoneInEcmMode && isEmergencyNumber) {
            handleEcmTimer(phone.CANCEL_ECM_TIMER);
        }

        // We are initiating a call therefore even if we previously
        // didn't know the state (i.e. Generic was true) we now know
        // and therefore can set Generic to false.
        foregroundCall.setGeneric(false);

        // The new call must be assigned to the foreground call.
        // That call must be idle, so place anything that's
        // there on hold
        if (foregroundCall.getState() == Call.State.ACTIVE) {
            Log.d(LOG_TAG, "2nd Ims call , start holding 1st Ims call");
            switchWaitingOrHoldingAndActive();
            fakeHoldForegroundBeforeDial();
            isDialRequestPending = true;
        }

        pendingMO = new ImsConnection(phone.getContext(), dialString, this,
                foregroundCall, callDetails);
        hangupPendingMO = false;

        if (pendingMO.address == null || pendingMO.address.length() == 0
                || pendingMO.address.indexOf(PhoneNumberUtils.WILD) >= 0) {
            // Phone number is invalid
            pendingMO.cause = Connection.DisconnectCause.INVALID_NUMBER;

            // handlePollCalls() will notice this call not present
            // and will mark it as dropped.
            pollCallsWhenSafe();
        } else {
            // if isDialRequestPending is true, we would postpone the dial
            // request for the second call till we get the hold confirmation
            // for the first call.
            if (isDialRequestPending == false) {
                if((!isPhoneInEcmMode) || (isPhoneInEcmMode && isEmergencyNumber)) {
                    // Always unmute when initiating a new call
                    setMute(false);
                    cm.dial(pendingMO.address, clirMode, callDetails, obtainCompleteMessage());
                } else {
                    phone.exitEmergencyCallbackMode();
                    phone.setOnEcbModeExitResponse(this,EVENT_EXIT_ECM_RESPONSE_CDMA, null);
                    pendingCallClirMode=clirMode;
                    pendingCallInEcm=true;
                }
            }
        }

        updatePhoneState();
        phone.notifyPreciseCallStateChanged();

        return pendingMO;
    }

    public void acceptCall() throws CallStateException {
        logUnexpectedMethodCall("acceptCall()");
    }

    public void acceptCall(int callType) throws CallStateException {

        int actualCallType = callType;
        if (callType == CallDetails.CALL_TYPE_UNKNOWN) {
            actualCallType = getCallType(ringingCall);

            //If the calltype is still unknown, then that means
            //connection is null, so ignore further processing
            if (actualCallType == CallDetails.CALL_TYPE_UNKNOWN) {
                return;
            }
        }

        if (ringingCall.getState() == Call.State.INCOMING) {
            Log.i("phone", "acceptCall: incoming with calltype...");
            // Always unmute when answering a new call
            setMute(false);
            log("ACCEPT with obtainCompleteMessage");
            cm.acceptCall(obtainCompleteMessage(), actualCallType);
        } else if (ringingCall.getState() == Call.State.WAITING) {
            ImsConnection cwConn = (ImsConnection) (ringingCall
                    .getLatestConnection());

            // Since there is no network response for supplimentary
            // service for CDMA, we assume call waiting is answered.
            // ringing Call state change to idle is in CallBase.detach
            // triggered by updateParent.
            cwConn.updateParent(ringingCall, foregroundCall);
            cwConn.onConnectedInOrOut();

            updatePhoneState();
            switchWaitingOrHoldingAndActive(actualCallType);

        } else {
            throw new CallStateException("phone not ringing");
        }

    }

    public void deflectCall(int index, String number, Message response) {
        cm.deflectCall(index, number, response);
    }

    public int getCallType(Call call) {
        ImsConnection conn = (ImsConnection) call.getEarliestConnection();
        if (conn == null) {
            return CallDetails.CALL_TYPE_UNKNOWN;
        }
        CallDetails callDetails = conn.getCallDetails();

        return callDetails.call_type;
    }

    public int getCallDomain(Call call) {
        ImsConnection conn = (ImsConnection) call.getEarliestConnection();
        if (conn == null) {
            return CallDetails.CALL_DOMAIN_UNKNOWN;
        }
        CallDetails callDetails = conn.getCallDetails();

        return callDetails.call_domain;
    }

    public void rejectCall() throws CallStateException {
        // AT+CHLD=0 means "release held or UDUB"
        // so if the phone isn't ringing, this could hang up held
        if (ringingCall.getState().isRinging()) {
            vlog("REJECT with obtainCompleteMessage");

            cm.rejectCall(obtainCompleteMessage());
        } else {
            throw new CallStateException("phone not ringing");
        }
    }

    private boolean isValidFailCause(int failCause) {
        switch (failCause)
        {
            case ImsQmiIF.CALL_FAIL_UNOBTAINABLE_NUMBER:
            case ImsQmiIF.CALL_FAIL_NORMAL:
            case ImsQmiIF.CALL_FAIL_BUSY:
            case ImsQmiIF.CALL_FAIL_CONGESTION:
            case ImsQmiIF.CALL_FAIL_INCOMPATIBILITY_DESTINATION:
            case ImsQmiIF.CALL_FAIL_CALL_BARRED:
            case ImsQmiIF.CALL_FAIL_NETWORK_UNAVAILABLE:
            case ImsQmiIF.CALL_FAIL_FEATURE_UNAVAILABLE:
                return true;
        }
        return false;
    }

    public void rejectCall(int failCause) throws CallStateException {
        // AT+CHLD=0 means "release held or UDUB"
        // so if the phone isn't ringing, this could hang up held
        if (ringingCall.getState().isRinging() ) {
            if (!isValidFailCause(failCause) ) {
                throw new CallStateException("Invalid fail cause: " + failCause);
            }
            Connection conn = ringingCall.getLatestConnection();
            if (conn != null) {
                vlog("REJECT with obtainCompleteMessage");
                cm.hangupWithReason(conn.getIndex(), conn.getAddress(), null, false, failCause,
                        null, obtainCompleteMessage());
            }
        } else {
            throw new CallStateException("phone not ringing");
        }
    }

    public void switchWaitingOrHoldingAndActive() throws CallStateException {
        if (callSwitchPending == false) {
            vlog("SWITCH with obtainCompleteMessage");
            cm.switchWaitingOrHoldingAndActive(obtainCompleteMessage(EVENT_SWITCH_RESULT));
            callSwitchPending = true;
        } else {
            Log.w(LOG_TAG,
                    "Call Switch request ignored due to pending response");
        }
    }

    public void switchWaitingOrHoldingAndActive(int callType) {
        if (callSwitchPending == false) {
            vlog("SWITCH with obtainCompleteMessage callType=" + callType);
            cm.switchWaitingOrHoldingAndActive(obtainCompleteMessage(EVENT_SWITCH_RESULT),
                    callType);
            callSwitchPending = true;
        } else {
            Log.w(LOG_TAG,
                    "Call Switch request ignored due to pending response");
        }
    }

    public void conference() throws CallStateException {
        Log.d(LOG_TAG, "CONFERENCE with obtainCompleteMessage");
        cm.conference(obtainCompleteMessage(EVENT_CONFERENCE_RESULT));
    }

    public void explicitCallTransfer() throws CallStateException {
        Log.d(LOG_TAG, "xFER with obtainCompleteMessage");
        cm.explicitCallTransfer(obtainCompleteMessage(EVENT_ECT_RESULT));
    }

    public void clearDisconnected() {
        internalClearDisconnected();
        updatePhoneState();
        phone.notifyPreciseCallStateChanged();
    }

    public boolean canConference() {
        // Limitation - IMS conference cannot have more than 5 users added
        // from this UE. More users can be added through
        // other particpants in the conference which is ok because we dont have
        // to maintain the call context for all those remotely added participants
        // Even this limitation has to be removed because for ims conference there
        // is only one call instance irrespective of the number of users added
        //
        return foregroundCall.getState() == Call.State.ACTIVE
                && backgroundCall.getState() == Call.State.HOLDING
                && !(foregroundCall.connections.size() == MAX_CONNECTIONS_PER_CALL)
                && !(backgroundCall.connections.size() == MAX_CONNECTIONS_PER_CALL);

    }

    public boolean canDial() {
           String disableCall;
        if(("A862").equals(Build.PROJECT) && Build.PWV_CUSTOM_CUSTOM.equals("EVIANT")){
            disableCall = SystemProperties.get(
                TelephonyProperties.PROPERTY_DISABLE_CALL, "true");
        }else{
            disableCall = SystemProperties.get(
                TelephonyProperties.PROPERTY_DISABLE_CALL, "false");
        }

        Log.v(LOG_TAG, "canDial(): "
                + "\nserviceState = " + phone.getServiceState().getState()
                + "\npendingMO == null::=" + String.valueOf(pendingMO == null)
                + "\nringingCall: " + ringingCall.getState()
                + "\ndisableCall = " + disableCall
                + "\nforegndCall: " + foregroundCall.getState()
                + "\nbackgndCall: " + backgroundCall.getState());

        /*
         * Call Manager does the can dial check based on all calls from CDMA/GSM/IMS phones.
         * If CS has both foreground & back ground calls, foreground CS call
         * is hang up before PS call is dialed.  If CS has just foreground it is placed
         * in background before PS call is dialed.
         *
         * 911 can be dialed when IMS is not registered hence postpone check for serviceState
         * until dial function where 911 check can be done.
         */
        boolean ret = pendingMO == null
                && !ringingCall.isRinging()
                && !disableCall.equals("true")
                && (!foregroundCall.getState().isAlive()
                    || !backgroundCall.getState().isAlive());

        return ret;
    }

    /*
     * This functionality is not supported for IMS.
     * Hence return false
     */
    public boolean canTransfer() {
        return false;
    }

    // ***** Private Instance Methods

    private void internalClearDisconnected() {
        // phone.getR
        ringingCall.clearDisconnected();
        foregroundCall.clearDisconnected();
        backgroundCall.clearDisconnected();
    }

    /**
     * Obtain a message to use for signalling "invoke getCurrentCalls() when
     * this operation and all other pending operations are complete
     */
    private Message obtainCompleteMessage() {
        return obtainCompleteMessage(EVENT_OPERATION_COMPLETE);
    }

    /**
     * Obtain a message to use for signalling "invoke getCurrentCalls() when
     * this operation and all other pending operations are complete
     */
    private Message obtainCompleteMessage(int what) {
        mPendingOperations++;
        mLastRelevantPoll = null;
        mNeedsPoll = true;

        if (DBG_POLL)
            log("obtainCompleteMessage: pendingOperations=" + mPendingOperations
                    + ", needsPoll=" + mNeedsPoll);
        return obtainMessage(what, phone);
    }

    private void operationComplete() {
        mPendingOperations--;

        if (DBG_POLL)
            log("operationComplete: pendingOperations=" + mPendingOperations
                    + ", needsPoll=" + mNeedsPoll);

        if (mPendingOperations == 0 && mNeedsPoll) {
            mLastRelevantPoll = obtainMessage(EVENT_POLL_CALLS_RESULT);
            cm.getCurrentCalls(mLastRelevantPoll);
        } else if (mPendingOperations < 0) {
            // this should never happen
            Log.e(LOG_TAG, "ImsCallTracker.pendingOperations < 0");
            mPendingOperations = 0;
        }
    }

    private void updatePhoneState() {

        if (phone == null) {
            log("null phone object in updatePhoneState");
            return;
        }
        PhoneConstants.State oldState = phone.getState();
        PhoneConstants.State curState = oldState;

        if (ringingCall.isRinging()) {
            curState = PhoneConstants.State.RINGING;
        } else if (pendingMO != null
                || !(foregroundCall.isIdle() && backgroundCall.isIdle())) {
            curState = PhoneConstants.State.OFFHOOK;
        } else {
            mPendingHandover = false;
            curState = PhoneConstants.State.IDLE;
        }

        if (curState != oldState) {
            phone.setState(curState);
            phone.notifyPhoneStateChanged();
        }

        phone.setState(curState);

        if (curState == PhoneConstants.State.IDLE && oldState != curState) {
            imsCallEndedRegistrants.notifyRegistrants(new AsyncResult(null,
                    null, null));
        } else if (oldState == PhoneConstants.State.IDLE && oldState != curState) {
            imsCallStartedRegistrants.notifyRegistrants(new AsyncResult(null,
                    null, null));
        }
        log("update ims phone state, old=" + oldState + " new=" + curState);

    }

    private void dumpConnection(ImsConnection con) {
        if (con != null) {
            Log.d(LOG_TAG, "[conn] number: " + con.address + " index: "
                    + con.index + " incoming: " + con.isIncoming + " alive: "
                    + con.isAlive() + " ringing: " + con.isRinging());
        }
    }

    private void dumpDC(DriverCallIms dc) {
        if (dc != null) {
            Log.d(LOG_TAG, "[ dc ] number:" + dc.number + " index: " + dc.index
                    + " incoming: " + dc.isMT + " state: " + dc.state
                    + "callDetails" + dc.callDetails);
        }
    }

    private void dumpState(List dcalls) {
        Log.d(LOG_TAG, "Connections:");
        for (int i = 0; i < connections.length; i++) {
            if (connections[i] == null) {
                Log.d(LOG_TAG, "Connection " + i + ": NULL");
            } else {
                Log.d(LOG_TAG, "Connection " + i + ": ");
                dumpConnection(connections[i]);
            }
        }
        if (dcalls != null) {
            Log.d(LOG_TAG, "Driver Calls:");
            for (Object dcall : dcalls) {
                DriverCallIms dc = (DriverCallIms) dcall;
                dumpDC(dc);
            }
        }
    }

    protected boolean isImsExceptionRadioNotAvailable(Throwable e) {
        return e != null
                && e instanceof RuntimeException
                && ((RuntimeException) e).getMessage().equals(
                        ImsSenderRxr.errorIdToString(ImsQmiIF.E_RADIO_NOT_AVAILABLE));
    }

    public void setServiceStatus(int srvType, int network, int enabled, int restrictCause) {
        cm.setServiceStatus(obtainCompleteMessage(EVENT_SET_SERVICE_STATUS), srvType, network,
                enabled, restrictCause);
    }

    private void handleImsStateChanged() {
        // handlePollCalls will clear out its
        // call list when it gets the CommandException
        // error result from this
        pollCallsWhenSafe();
    }

    private void handleModifyCallRequest(CallModify cm) {
        Log.d(LOG_TAG, "handleCallModifyRequest(" + cm + ")");

        if (cm != null) {
            ImsConnection c = getConnectionByIndex(cm.call_index);
            if (c != null) {
                if (c.onReceivedModifyCall(cm)) {
                    phone.notifyModifyCallRequest(c);
                } else {
                    Log.w(LOG_TAG, "Not notifing registrants about" + cm);
                }
            } else {
                Log.e(LOG_TAG, "Null Call Modify request ");
            }
        }
    }

    public void modifyCallInitiate(Message msg, CallModify callModify)
            throws CallStateException {
        cm.modifyCallInitiate(msg, callModify);
    }

    public void modifyCallConfirm(Message msg, CallModify callModify)
            throws CallStateException {
        cm.modifyCallConfirm(msg, callModify);
    }

    private boolean checkDomainSwitch(ImsConnection conn, DriverCallIms dc) {
        boolean ret = false;
        if (conn != null && dc != null && conn.compareTo(dc) && isCsVoice(dc)) {
            if (conn.mConnectTimeReal != 0 || conn.getConnectTime() != 0) {
                // Transfer call connect time to CS call
                Connection newConn = CallManager.getInstance().getFgCallLatestConnection();
                Phone newPhone = newConn != null ? (newConn.getCall() != null ? newConn.getCall()
                        .getPhone() : null) : null;

                // Retrieve call with same address from CS
                if (newPhone != null && newPhone != phone
                        && conn.equalsHandlesNulls(conn.address, newConn.getAddress())) {
                    Log.d(LOG_TAG, "IMS call started @ realTime" + conn.mConnectTimeReal
                            + " connectTime" + conn.getConnectTime()
                            + " tranferred successfully to CS ");
                    newConn.mConnectTimeReal = conn.mConnectTimeReal;
                    newConn.setConnectTime(conn.getConnectTime());
                } else {
                    /*
                     * Domain switch is a make(cs path) before break (ims path).
                     * We should never hit this log. If we hit this it means
                     * something caused a huge backlog at RIL which by itself is
                     * a problem.
                     */
                    Log.e(LOG_TAG, "IMS domain switched CS call not found, lost connect time ");
                }
            }

            conn.onDisconnect(Connection.DisconnectCause.SRVCC_CALL_DROP);

            if (conn == pendingMO) {
                Log.d(LOG_TAG, "Pending MO switched to CS domain, ignoring call");
                pendingMO = null;
                hangupPendingMO = false;
                if (pendingCallInEcm) {
                    pendingCallInEcm = false;
                }
            } else {
                Log.d(LOG_TAG, "Active call switched to CS domain, ignoring call");
            }

            conn = null;
            updatePhoneState();
            ret = true;
        }

        if (processDc(dc)) {
            String oldIds = dc.callDetails.getValueForKeyFromExtras(dc.callDetails.extras,
                    CallDetails.EXTRAS_PARENT_CALL_ID);
            if (oldIds != null) {
                for (String temp : oldIds.split(",", MAX_CONNECTIONS)) {
                    int currId = -1;
                    try {
                        currId = Integer.parseInt(temp);
                    } catch (NumberFormatException e) {
                        Log.e(LOG_TAG, "Invalid parentCallId extra");
                    }
                    if (currId > 0 && currId <= MAX_CONNECTIONS) {
                        Log.d("LOG_TAG", "Local disconnection of old Id " + currId);
                        if (connections[currId - 1] != null) {
                            connections[currId - 1].onLocalDisconnect();
                        }
                    }
                }
            }
            ret = true;
        }
        return ret;
    }

    protected boolean isImsExceptionImsDeregistered(Throwable e) {
        return e != null
                && e instanceof RuntimeException
                && ((RuntimeException) e).getMessage().equals(
                        ImsSenderRxr.errorIdToString(ImsQmiIF.E_IMS_DEREGISTERED));
    }

    private boolean processDc(DriverCallIms dc){
        return ((dc != null && dc.isMpty
                && dc.callDetails.call_domain == CallDetails.CALL_DOMAIN_CS
                && dc.callDetails.extras != null
                && dc.callDetails.getValueForKeyFromExtras(dc.callDetails.extras,
                CallDetails.EXTRAS_PARENT_CALL_ID) != null)) ? true : false;
    }

    // ***** Overwritten from CallTracker

    protected void handlePollCalls(AsyncResult ar) {
        List polledCalls;
        Log.d(LOG_TAG, ">handlePollCalls");

        if (ar.exception == null) {
            polledCalls = (List) ar.result;
        } else if (isImsExceptionRadioNotAvailable(ar.exception)) {
            // just a dummy empty ArrayList to cause the loop
            // to hang up all the calls
            polledCalls = new ArrayList();
        } else {
            // Radio probably wasn't ready--try again in a bit
            // But don't keep polling if the channel is closed
            pollCallsAfterDelay();
            return;
        }

        Connection newRinging = null; // or waiting

        boolean hasNonHangupStateChanged = false;// Any change besides a dropped
        // connection
        boolean hasAnyCallDisconnected = false;
        boolean needsPollDelay = false;
        boolean unknownConnectionAppeared = false;

        if (DBG_POLL) dumpState(polledCalls);

        for (int i = 0, curDC = 0, dcSize = polledCalls.size(); i < connections.length; i++) {
            ImsConnection conn = connections[i];
            DriverCallIms dc = null;

            // polledCall list is sparse
            if (curDC < dcSize) {
                dc = (DriverCallIms) polledCalls.get(curDC);

                if (dc.index == i + 1) {
                    curDC++;
                } else if (!processDc(dc)) {
                    Log.d(LOG_TAG, "Setting dc to null");
                    dc = null;
                } else {
                    // Currently for the scenario where SRVCC conference call is
                    // in progress,even if index of dc does not match with the
                    // connection index process it
                    curDC++;
                    Log.d(LOG_TAG, "Not resetting dc as SRVCC Conference notification");
                }
            }

            if (DBG_POLL)
                log("poll: conn[i=" + i + "]=" + conn + ", dc=" + dc);

            if ((checkDomainSwitch(conn, dc)) || (checkDomainSwitch(pendingMO, dc))
                    || (dc != null && isCsVoice(dc)))
            {
                Log.d(LOG_TAG, "continue after checkDomainSwitch");
                continue;
            }

            if (conn != null && dc != null && !TextUtils.isEmpty(conn.address)
                    && !conn.compareTo(dc)) {
                // This means we received a different call than we expected
                // in
                // the call list.
                // Drop the call, and set conn to null, so that the dc can
                // be
                // processed as a new
                // call by the logic below.
                // This may happen if for some reason the modem drops the
                // call,
                // and replaces it
                // with another one, but still using the same index (for
                // instance, if BS drops our
                // MO and replaces with an MT due to priority rules)
                Log.d(LOG_TAG, "New call with same index. Dropping old call");
                droppedDuringPoll.add(conn);
                conn = null;
            }

            if (conn == null && dc != null) {
                Log.d(LOG_TAG, "conn(" + conn + ")");

                // Connection appeared in CLCC response that we don't know
                // about
                if (pendingMO != null && pendingMO.compareTo(dc)) {

                    if (DBG_POLL)
                        log("poll: pendingMO=" + pendingMO);

                    // It's our pending mobile originating call
                    connections[i] = pendingMO;
                    pendingMO.index = i;
                    pendingMO.update(dc);
                    pendingMO = null;

                    // Someone has already asked to hangup this call
                    if (hangupPendingMO) {
                        hangupPendingMO = false;
                        // Re-start Ecm timer when an uncompleted emergency call ends
                        if (mIsEcmTimerCanceled) {
                            handleEcmTimer(phone.RESTART_ECM_TIMER);
                        }

                        try {
                            if (Phone.DEBUG_PHONE) {
                                log("poll: hangupPendingMO, hangup conn " + i);
                            }
                            hangup(connections[i]);
                        } catch (CallStateException ex) {
                            Log.e(LOG_TAG, "unexpected error on hangup");
                        }

                        // Do not continue processing this poll
                        // Wait for hangup and repoll
                        return;
                    }
                } else {
                    if (Phone.DEBUG_PHONE) {
                        log("pendingMo=" + pendingMO + ", dc=" + dc);
                    }
                    // find if the MT call is a new ring or unknown
                    // connection
                    newRinging = checkMtFindNewRinging(dc, i);

                    if (newRinging == null) {
                        unknownConnectionAppeared = true;
                    }
                }
                hasNonHangupStateChanged = true;
            } else if (conn != null && dc == null) {
                if (dcSize != 0) {
                    // This happens if the call we are looking at (index i)
                    // got dropped but the call list is not yet empty.
                    Log.d(LOG_TAG,
                            "conn != null, dc == null. Still have connections in the call list");
                    droppedDuringPoll.add(conn);
                } else {
                    // This case means the RIL has no more active call
                    // anymore
                    // and
                    // we need to clean up the foregroundCall and
                    // ringingCall.
                    cleanupCalls();
                }

                // Re-start Ecm timer when the connected emergency call ends
                if (mIsEcmTimerCanceled) {
                    handleEcmTimer(phone.RESTART_ECM_TIMER);
                }
                // Dropped connections are removed from the CallTracker
                // list but kept in the Call list
                connections[i] = null;
            } else if (conn != null && dc != null) {
                if (conn.isIncoming != dc.isMT) {
                    // Call collision case
                    if (dc.isMT == true) {
                        // Mt call takes precedence than Mo,drops Mo
                        droppedDuringPoll.add(conn);
                        // find if the MT call is a new ring or unknown
                        // connection
                        newRinging = checkMtFindNewRinging(dc, i);
                        if (newRinging == null) {
                            unknownConnectionAppeared = true;
                        }

                    } else {
                        // Call info stored in conn is not consistent with
                        // the
                        // call info from dc.
                        // We should follow the rule of MT calls taking
                        // precedence over MO calls
                        // when there is conflict, so here we drop the call
                        // info
                        // from dc and continue to use the call info from
                        // conn,
                        // and only take a log.
                        Log.e(LOG_TAG, "Error in RIL, Phantom call appeared "
                                + dc);
                    }
                } else {
                    boolean changed;
                    changed = conn.update(dc);
                    hasNonHangupStateChanged = hasNonHangupStateChanged
                            || changed;
                }
            }
        }

        // This is the first poll after an ATD.
        // We expect the pending call to appear in the list
        // If it does not, we land here
        if (pendingMO != null) {
            Log.d(LOG_TAG, "Pending MO dropped before poll fg state:"
                    + foregroundCall.getState());

            droppedDuringPoll.add(pendingMO);
            pendingMO = null;
            hangupPendingMO = false;
            if (pendingCallInEcm) {
                pendingCallInEcm = false;
            }
        }

        if (newRinging != null) {
            phone.notifyNewRingingConnection(newRinging);
        }

        // clear the "local hangup" and "missed/rejected call"
        // cases from the "dropped during poll" list
        // These cases need no "last call fail" reason
        for (int i = droppedDuringPoll.size() - 1; i >= 0; i--) {
            ImsConnection conn = droppedDuringPoll.get(i);

            if (conn.isIncoming() && conn.getConnectTime() == 0) {
                // Missed or rejected or answered else where call
                if (conn.cause == Connection.DisconnectCause.LOCAL) {
                    Connection.DisconnectCause cause = Connection.DisconnectCause.INCOMING_REJECTED;
                    droppedDuringPoll.remove(i);
                    hasAnyCallDisconnected |= conn.onDisconnect(cause);
                    if (Phone.DEBUG_PHONE) {
                        log("rejected call, conn.cause=" + conn.cause);
                        log("setting cause to " + cause);
                    }
                } else {
                    Log.d(LOG_TAG, "incoming missed or answered elsewhere call");
                }

            } else if (conn.cause == Connection.DisconnectCause.LOCAL
                    || conn.cause == Connection.DisconnectCause.INVALID_NUMBER) {
                // Local hangup
                droppedDuringPoll.remove(i);
                hasAnyCallDisconnected |= conn.onDisconnect(conn.cause);
            }
        }

        // Any non-local disconnects: determine cause
        if (droppedDuringPoll.size() > 0) {
            cm.getLastCallFailCause(obtainNoPollCompleteMessage(EVENT_GET_LAST_CALL_FAIL_CAUSE));
        }

        // Cases when we can no longer keep disconnected Connection's
        // with their previous calls
        // 1) the phone has started to ring
        // 2) A Call/Connection object has changed state...
        // we may have switched or held or answered (but not hung up)
        if ((newRinging != null) || (hasNonHangupStateChanged) || hasAnyCallDisconnected) {
            internalClearDisconnected();
            updatePhoneState();
            phone.notifyPreciseCallStateChanged();
        }

        updatePhoneState();

        if (unknownConnectionAppeared) {// unknown connection notified only to
            // imsphone
            phone.notifyUnknownConnection();
        }

        Log.d(LOG_TAG, "<handlePollCalls");
    }

    private boolean isCsVoice(DriverCallIms dc) {
        return dc.callDetails.call_domain == CallDetails.CALL_DOMAIN_CS
        && dc.callDetails.call_type != CallDetails.CALL_TYPE_VT;
    }

    private void cleanupCalls() {
        // Loop through foreground call connections as
        // it contains the known logical connections.

        // FIXME use connections list
        int count = foregroundCall.connections.size();
        log("fgcall phone " + phone + " call count" + count);
        for (int n = 0; n < count; n++) {
            if (Phone.DEBUG_PHONE)
                log("adding fgCall cn " + n + " to droppedDuringPoll");
            ImsConnection cn = (ImsConnection) (foregroundCall.connections
                    .get(n));
            droppedDuringPoll.add(cn);
        }

        count = backgroundCall.connections.size();
        log("bgcall phone " + phone + " call count" + count);
        for (int n = 0; n < count; n++) {
            if (Phone.DEBUG_PHONE)
                log("adding BgCall cn " + n + " to droppedDuringPoll");
            ImsConnection cn = (ImsConnection) (backgroundCall.connections
                    .get(n));
            droppedDuringPoll.add(cn);
        }

        count = ringingCall.connections.size();
        // Loop through ringing call connections as
        // it may contain the known logical connections.
        log("rgcall phone " + phone + " call count" + count);
        for (int n = 0; n < count; n++) {
            if (Phone.DEBUG_PHONE)
                log("adding rgCall cn " + n + " to droppedDuringPoll");
            ImsConnection cn = (ImsConnection) (ringingCall.connections.get(n));
            droppedDuringPoll.add(cn);
        }

        foregroundCall.setGeneric(false);
        ringingCall.setGeneric(false);
        backgroundCall.setGeneric(false);
    }

    // ***** Called from ConnectionBase
    public void hangup(ImsConnection conn) throws CallStateException {
        if (conn.owner != this) {
            throw new CallStateException("ConnectionBase " + conn
                    + "does not belong to ImsCallTracker " + this);
        }
        Call call = conn.getCall();
        if (conn == pendingMO) {
            // We're hanging up an outgoing call that doesn't have it's
            // GSM index assigned yet

            if (Phone.DEBUG_PHONE)
                log("hangup: set hangupPendingMO to true");
            hangupPendingMO = true;
        } else if ((call == ringingCall)
                && (ringingCall.getState() == Call.State.WAITING)) {
            // Handle call waiting hang up case.
            //
            // The ringingCall state will change to IDLE in Call.detach
            // if the ringing call connection size is 0. We don't specifically
            // set the ringing call state to IDLE here to avoid a race condition
            // where a new call waiting could get a hang up from an old call
            // waiting ringingCall.
            //
            // PhoneApp does the call log itself since only PhoneApp knows
            // the hangup reason is user ignoring or timing out. So
            // conn.onDisconnect()
            // is not called here. Instead, conn.onLocalDisconnect() is called.
            conn.onLocalDisconnect();
            updatePhoneState();
            phone.notifyPreciseCallStateChanged();
            return;
        } else {
            try {
                cm.hangupConnection(conn.getIndex(), obtainCompleteMessage());
            } catch (CallStateException ex) {
                // Ignore "connection not found"
                // Call may have hung up already
                Log.w(LOG_TAG,
                        "ImsCallTracker WARN: hangup() on absent connection "
                                + conn);
            }
        }

        conn.onHangupLocal();
    }

    public void separate(ImsConnection conn) throws CallStateException {
        logUnexpectedMethodCall("separate");
    }

    // ***** Called from CDMAPhone

    /* package */public void setMute(boolean mute) {
        desiredMute = mute;
    }

    /* package */public boolean getMute() {
        return desiredMute;
    }

    public void hangupWaitingOrBackground() {
        if (Phone.DEBUG_PHONE)
            log("hangupWaitingOrBackground");
        cm.hangupWaitingOrBackground(obtainCompleteMessage());
    }

    public void hangupWithReason(int connectionId, String userUri, String confUri, boolean mpty,
            int failCause, String errorInfo) {
        if (Phone.DEBUG_PHONE)
            log("hangupWithReason");
        cm.hangupWithReason(connectionId, userUri, confUri, mpty, failCause, errorInfo,
                obtainCompleteMessage());
    }

    /* package */public void hangup(ImsCall call) throws CallStateException {
        if (call.getConnections().size() == 0) {
            throw new CallStateException("no connections in call");
        }

        if (call == ringingCall) {
            if (Phone.DEBUG_PHONE)
                log("(ringing) hangup waiting or background");
            if(ringingCall.getState() == Call.State.INCOMING) {
                ImsConnection conn = (ImsConnection)ringingCall.connections.get(0);
                //Refer "errorinfo" field of "CallFailCauseResponse" for possible errorinfo values
                hangupWithReason(conn.getIndex(), conn.getAddress(),
                        null, false, ImsQmiIF.CALL_FAIL_MISC, String.valueOf(1));
            } else {
                cm.hangupWaitingOrBackground(obtainCompleteMessage());
            }
        } else if (call == foregroundCall) {
            if (call.isDialingOrAlerting()) {
                if (Phone.DEBUG_PHONE) {
                    log("(foregnd) hangup dialing or alerting...");
                }
                hangup((ImsConnection) (call.getConnections().get(0)));
            } else {
                hangupForegroundResumeBackground();
            }
        } else if (call == backgroundCall) {
            if (ringingCall.isRinging()) {
                if (Phone.DEBUG_PHONE) {
                    log("hangup all conns in background call");
                }
                hangupAllConnections(call);
            } else {
                hangupWaitingOrBackground();
            }
        } else {
            throw new RuntimeException("Call " + call
                    + "does not belong to ImsCallTracker " + this);
        }

        call.onHangupLocal();
        phone.notifyPreciseCallStateChanged();
    }

    /* package */
    void hangupForegroundResumeBackground() {
        if (Phone.DEBUG_PHONE)
            log("hangupForegroundResumeBackground");
        cm.hangupForegroundResumeBackground(obtainCompleteMessage());
    }

    public void hangupConnectionByIndex(ImsCall call, int index)
            throws CallStateException {
        int count = call.connections.size();
        for (int i = 0; i < count; i++) {
            ImsConnection cn = (ImsConnection) call.connections.get(i);
            if (cn.getIndex() == index) {
                cm.hangupConnection(index, obtainCompleteMessage());
                return;
            }
        }

        throw new CallStateException("no gsm index found");
    }

    void hangupAllConnections(ImsCall call) throws CallStateException {
        try {
            int count = call.connections.size();
            for (int i = 0; i < count; i++) {
                ImsConnection cn = (ImsConnection) call.connections.get(i);
                cm.hangupConnection(cn.getIndex(), obtainCompleteMessage());
            }
        } catch (CallStateException ex) {
            Log.e(LOG_TAG, "hangupConnectionByIndex caught " + ex);
        }
    }

    /**
     * Get Connection from complete list of connections of CdmaImsCallTracker
     *
     * @param index
     * @return
     * @throws CallStateException
     */
    ImsConnection getConnectionByIndex(int index) {
        for (ImsConnection c : connections) {
            try {
                if (c != null && c.getIndex() == index) {
                    return c;
                }
            } catch (CallStateException ex) {
                // Ignore "connection not found"
                // Call may have hung up already
                Log.w(LOG_TAG, " absent connection for index " + index);
            }
        }
        return null;
    }

    private Phone.SuppService getFailedService(int what) {
        switch (what) {
            case EVENT_SWITCH_RESULT:
                return Phone.SuppService.SWITCH;
            case EVENT_CONFERENCE_RESULT:
                return Phone.SuppService.CONFERENCE;
            case EVENT_SEPARATE_RESULT:
                return Phone.SuppService.SEPARATE;
            case EVENT_ECT_RESULT:
                return Phone.SuppService.TRANSFER;
            default:
                return Phone.SuppService.UNKNOWN;
        }
    }

    private static boolean isSrvccHandover (int srcTech, int targetTech) {
        return !(srcTech == ImsQmiIF.RADIO_TECH_LTE && targetTech == ImsQmiIF.RADIO_TECH_LTE);
    }

    private void handleHandover(HandoverInfo msg) {
        Log.d(LOG_TAG, " Rxd handover Trigger " + msg + "PendingHandover" + mPendingHandover
                + " Phone State = " + phone.getState());
        if (isSrvccHandover (msg.mSrcTech, msg.mTargetTech)) {
            switch (msg.mType) {
                case ImsQmiIF.START: // START Handover triggered
                    if (phone.getState() == PhoneConstants.State.OFFHOOK ||
                            (phone.getState() == PhoneConstants.State.RINGING &&
                            (foregroundCall.getState() == Call.State.ACTIVE ||
                            backgroundCall.getState() == Call.State.HOLDING))) {
                        mPendingHandover = true;
                    }
                    break;
                case ImsQmiIF.COMPLETE_SUCCESS: // COMPLETE_SUCCESS Handover completed successfully
                    if (mPendingOperations > 0) {
                        Log.d(LOG_TAG, " Rxd handover with pending operation , cleaning up "
                                + mPendingOperations);
                        mLastRelevantPoll = null;
                        mNeedsPoll = true;
                        sendEmptyMessage(EVENT_OPERATION_COMPLETE);
                    }
                    mPendingHandover = false;
                    break;
                case ImsQmiIF.COMPLETE_FAIL: // COMPLETE_FAIL Handover failed
                case ImsQmiIF.CANCEL: // CANCEL = 3 Handover cancelled
                    mPendingHandover = false;
                    break;
            }
        } else { // This is a Handover Failure for LTE to IWLAN scenario
            if (msg.mType == ImsQmiIF.COMPLETE_FAIL) {
                Log.d(LOG_TAG, "HO Failure for WWAN->IWLAN " + msg.mExtraType + msg.mExtraInfo);
                if (msg.mExtraType == ImsQmiIF.LTE_TO_IWLAN_HO_FAIL) {
                    for (ImsConnection c : connections) {
                        String[] tempExtras;
                        if (c != null) {
                            String hoExtra = CallDetails.EXTRAS_HANDOVER_INFORMATION + "=" +
                                    CallDetails.EXTRA_TYPE_LTE_TO_IWLAN_HO_FAIL;
                            if (c.callDetails.extras != null) {
                                tempExtras = new String[c.callDetails.extras.length + 1];
                                System.arraycopy(c.callDetails.extras, 0, tempExtras, 0,
                                        c.callDetails.extras.length);
                                tempExtras[c.callDetails.extras.length] = hoExtra;
                            } else {
                                tempExtras = new String[1];
                                tempExtras[0] = hoExtra;
                            }
                            c.callDetails.setExtras(tempExtras);
                        }
                    }
                    phone.notifyPhoneStateChanged();
                }
            } else {
                Log.e(LOG_TAG, "Unhandled message for WWAN to IWLAN IRAT " + msg.mType);
            }
        }
    }

    // ****** Overridden from Handler

    public void handleMessage(Message msg) {
        AsyncResult ar;

        if (!phone.mIsTheCurrentActivePhone) {
            Log.w(LOG_TAG, "Ignoring events received on inactive CdmaPhone");
            return;
        }
        switch (msg.what) {
            case EVENT_POLL_CALLS_RESULT: {
                if (DBG_POLL)log("handle EVENT_POLL_CALL_RESULT received");
                ar = (AsyncResult) msg.obj;

                if (msg == mLastRelevantPoll) {
                    mNeedsPoll = false;
                    mLastRelevantPoll = null;
                    handlePollCalls((AsyncResult) msg.obj);
                }
            }
                break;

            case EVENT_OPERATION_COMPLETE:
                operationComplete();
                break;

            case EVENT_SWITCH_RESULT:
                // operationComplete() here gets the
                // current call list.
                // This event will also be called when the call is placed
                // on hold while there is another dialed call. If Hold succeeds,
                // dialPendingCall would be invoked.Else getCurrentCalls is
                // anyways
                // invoked through operationComplete,which will get the new
                // call states depending on which UI would be updated.
                callSwitchPending = false;
                ar = (AsyncResult) msg.obj;
                if (ar.exception != null) {
                    Log.i(LOG_TAG, "Exception during call switching");
                    if (isImsExceptionImsDeregistered(ar.exception)) {
                        Log.i(LOG_TAG, "Suppressing error due to Ims De-registration");
                    } else {
                        phone.notifySuppServiceFailed(getFailedService(msg.what));
                    }
                    // pending emergency call still needs to be dialed
                    if (pendingMO != null) {
                        boolean isEmergencyNumber = phone.isExactOrPotentialLocalEmergencyNumber(
                                pendingMO.getAddress());

                        if (isEmergencyNumber) {
                            dialPendingCall();
                        }
                    }
                } else {
                    if (pendingMO != null) {
                        dialPendingCall();
                    }
                }
                operationComplete();
                break;

            case EVENT_GET_LAST_CALL_FAIL_CAUSE: {
                int causeCode = CallFailCause.NORMAL_CLEARING;
                ImsQmiIF.CallFailCauseResponse callfail = null;
                ar = (AsyncResult) msg.obj;
                operationComplete();
                if (ar != null && ar.exception == null && ar.result != null) {
                    callfail = (ImsQmiIF.CallFailCauseResponse)ar.result;
                    causeCode = callfail.getFailcause();
                    Log.d(LOG_TAG, "causeCode : " + causeCode);
                } else {
                    // An exception occurred...just treat the disconnect
                    // cause as "normal"
                    Log.d(LOG_TAG, "IMS GET_LAST_CALL_FAIL_CAUSE failed");
                }

                for (int i = 0, s = droppedDuringPoll.size(); i < s; i++) {
                    ImsConnection conn = droppedDuringPoll.get(i);
                    if (callfail != null && callfail.hasErrorinfo()) {
                        if (callfail.getErrorinfo() != null) {
                            byte errorInfo[] = new byte[callfail.getErrorinfo().size()];
                            callfail.getErrorinfo().copyTo(errorInfo, 0);
                            String errorInfoStr = new String(errorInfo);
                            Log.d(LOG_TAG, "Error Info : " + errorInfoStr);
                            conn.setDiscImsErrorInfo(errorInfoStr);
                        } else {
                            Log.d(LOG_TAG, "Error Info is null");
                        }
                    } else {
                        Log.d(LOG_TAG, "It has no Error info");
                    }

                    if (conn.isIncoming() &&
                        conn.getConnectTime() == 0 &&
                        causeCode == ImsQmiIF.CALL_FAIL_NORMAL) {
                        conn.onDisconnect(Connection.DisconnectCause.INCOMING_MISSED);
                    } else {
                        conn.onRemoteDisconnect(causeCode);
                    }

                    updatePhoneState();
                    phone.notifyPreciseCallStateChanged();
                }

                droppedDuringPoll.clear();
            }
                break;

            case EVENT_REPOLL_AFTER_DELAY:
            case EVENT_CALL_STATE_CHANGE:
                pollCallsWhenSafe();
                break;

            case EVENT_IMS_STATE_CHANGED:
                handleImsStateChanged();
                break;

            case EVENT_EXIT_ECM_RESPONSE_CDMA:
                // no matter the result, we still do the same here
                if (pendingCallInEcm) {
                    cm.dial(pendingMO.address, pendingCallClirMode,
                            pendingMO.callDetails, obtainCompleteMessage());
                    pendingCallInEcm = false;
                }
                phone.unsetOnEcbModeExitResponse(this);
                break;

            case EVENT_MODIFY_CALL:
                ar = (AsyncResult) msg.obj;
                if (ar != null && ar.result != null && ar.exception == null) {
                    handleModifyCallRequest((CallModify) ar.result);
                } else {
                    Log.e(LOG_TAG, "Error EVENT_MODIFY_CALL AsyncResult ar= " + ar);
                }
                break;

            case EVENT_TTY_STATE_CHANGED:
                ar = (AsyncResult) msg.obj;
                if (ar != null && ar.result != null && ar.exception == null) {
                    int[] mode = (int[])ar.result;
                    String toastMsg = null;
                    switch (mode[0]) {
                        case ImsQmiIF.TTY_MODE_FULL:
                            toastMsg = "Peer requested TTY Mode FULL";
                            break;
                        case ImsQmiIF.TTY_MODE_HCO:
                            toastMsg = "Peer requested TTY Mode HCO";
                            break;
                        case ImsQmiIF.TTY_MODE_VCO:
                            toastMsg = "Peer requested TTY Mode VCO";
                            break;
                        case ImsQmiIF.TTY_MODE_OFF:
                            toastMsg = "Peer requested TTY Mode OFF";
                            break;
                        default:
                            toastMsg = "Peer request Unsupported TTY mode " + mode[0];
                            Log.e(LOG_TAG, "TTY Mode not supported");
                            break;
                    }
                    if (foregroundCall != null && foregroundCall.mState == Call.State.ACTIVE) {
                        Toast toast = Toast.makeText(phone.getContext(),
                                toastMsg, Toast.LENGTH_LONG);
                        toast.show();
                    } else {
                        Log.e(LOG_TAG, "TTY Toast blocked incorrect Foreground Call state");
                    }
                } else {
                    Log.e(LOG_TAG, "Error EVENT_TTY_STATE_CHANGED AsyncResult ar= " + ar);
                }
                break;

            case EVENT_RADIO_STATE_CHANGED:
                phone.handleRadioStateChange(cm.getRadioState());
                handleImsStateChanged();
                break;

            case EVENT_CONFERENCE_RESULT:
            case EVENT_SEPARATE_RESULT:
            case EVENT_ECT_RESULT:
                ar = (AsyncResult) msg.obj;
                if (ar.exception != null) {
                    Log.i(LOG_TAG, "Exception during supplementary services");
                    if (ar.userObj != null) {
                        phone.notifySuppServiceFailed(getFailedService(msg.what));
                    }
                }
                operationComplete();
                break;
            case EVENT_HANDOVER_STATE_CHANGED:
                AsyncResult arResult = (AsyncResult) msg.obj;
                if (arResult.exception == null && arResult.result != null) {
                    HandoverInfo response = (HandoverInfo) arResult.result;
                    Log.d(LOG_TAG, "Handover state is: " + response.mType + " Src Tech " +
                            response.mSrcTech + " Target Tech " + response.mTargetTech
                            + " Extra " + response.mExtraType + response.mExtraInfo);
                    handleHandover(response);
                } else {
                    Log.e(LOG_TAG, "IMS Handover State Change failed!");
                }
                break;
            case EVENT_SET_SERVICE_STATUS:
                /* TODO: Notify UI using callback for adjusting setting */
                break;
            default: {
                throw new RuntimeException("unexpected event not handled");
            }
        }
    }

    void dialPendingCall() {
        Log.d(LOG_TAG, "dialPendingCall: enter");
        if (pendingMO.address == null || pendingMO.address.length() == 0
                || pendingMO.address.indexOf(PhoneNumberUtils.WILD) >= 0) {
            // Phone number is invalid
            pendingMO.cause = Connection.DisconnectCause.INVALID_NUMBER;

            // handlePollCalls() will notice this call not present
            // and will mark it as dropped.
            pollCallsWhenSafe();
        } else {
            Log.d(LOG_TAG, "dialPendingCall: dialing...");
            // Always unmute when initiating a new call
            setMute(false);
            // uusinfo is null as it is not applicable to cdma
            // TODO but they may be applicable to IMS - need to check
            cm.dial(pendingMO.address, pendingCallClirMode, pendingMO.callDetails,
                    obtainCompleteMessage());
        }

        updatePhoneState();
        phone.notifyPreciseCallStateChanged();
    }

    /**
     * Check the MT call to see if it's a new ring or a unknown connection.
     */
    private Connection checkMtFindNewRinging(DriverCallIms dc, int i) {

        Connection newRinging = null;

        connections[i] = new ImsConnection(phone.getContext(), dc, this, i);

        // it's a ringing call
        if ((ringingCall != null) && (connections[i].getCall() == ringingCall)) {
            newRinging = connections[i];
            if (Phone.DEBUG_PHONE)
                log("Notify new ring " + dc);
        } else {
            // Something strange happened: a call which is neither
            // a ringing call nor the one we created. It could be the
            // call collision result from RIL
            Log.e(LOG_TAG, "Phantom call appeared " + dc);
            // If it's a connected call, set the connect time so that
            // it's non-zero. It may not be accurate, but at least
            // it won't appear as a Missed Call.
            if (dc.state != DriverCall.State.ALERTING
                    && dc.state != DriverCall.State.DIALING) {
                connections[i].connectTime = System.currentTimeMillis();
            }
        }
        return newRinging;
    }

    /**
     * Check if current call is in emergency call
     *
     * @return true if it is in emergency call false if it is not in emergency
     *         call
     */
    public boolean isInEmergencyCall() {
        return mIsInEmergencyCall;
    }

    /*
     * @Override protected void hangupAllCallsP(PhoneBase phone) throws
     * CallStateException { // TODO implEMENT THIS }
     */

    protected void log(String msg) {
        Log.d(LOG_TAG, "[ImsCallTracker] " + msg);
    }

    protected void vlog(String msg) {
        if (VDBG) {
            Log.d(LOG_TAG, "[ImsCallTracker] " + msg);
        }
    }

    public void registerForCallWaiting(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        callWaitingRegistrants.add(r);
    }

    public void unregisterForCallWaiting(Handler h) {
        callWaitingRegistrants.remove(h);
    }

    /* package */
    ImsConnection getConnectionByIndex(ImsCall call, int index)
            throws CallStateException {
        int count = call.connections.size();
        for (int i = 0; i < count; i++) {
            ImsConnection cn = (ImsConnection) call.connections.get(i);
            if (cn.getIndex() == index) {
                return cn;
            }
        }

        return null;
    }

    protected void pollCallsWhenSafe() {
        mNeedsPoll = true;

        if (checkNoOperationsPending()) {
            mLastRelevantPoll = obtainMessage(EVENT_POLL_CALLS_RESULT);
            cm.getCurrentCalls(mLastRelevantPoll);
        }
    }

    /**
     * @return true if we're idle or there's a call to getCurrentCalls() pending
     *         but nothing else
     */
    private boolean checkNoOperationsPending() {
        if (DBG_POLL)
            log("checkNoOperationsPending: pendingOperations="
                    + mPendingOperations);
        return mPendingOperations == 0;
    }

    /*
     * public DisconnectCause disconnectCauseFromCode(int causeCode) { /** See
     * 22.001 Annex F.4 for mapping of cause codes to local tones int
     * serviceState = phone.getServiceState().getState(); if (serviceState ==
     * ServiceState.STATE_POWER_OFF) { return DisconnectCause.POWER_OFF; } else
     * if (serviceState == ServiceState.STATE_OUT_OF_SERVICE || serviceState ==
     * ServiceState.STATE_EMERGENCY_ONLY) { return
     * DisconnectCause.OUT_OF_SERVICE; }else if (causeCode ==
     * CallFailCause.NORMAL_CLEARING) { return DisconnectCause.NORMAL; } else {
     * return DisconnectCause.ERROR_UNSPECIFIED; } }
     */
    /**
     * Common error logger method for unexpected calls to ImsCallTracker
     * methods.
     */
    private void logUnexpectedMethodCall(String name) {
        Log.e(LOG_TAG, "Error! " + name
                + "() is not supported by ImsCallTracker");
    }

    private ImsCall getCallFromConfCallState(int state){
        ImsCall call = null;
        switch (state) {
            case ImsQmiIF.RINGING: //ImsQmiIF.ConfCallState.RINGING
                call = ringingCall;
                break;
            case ImsQmiIF.FOREGROUND: //ImsQmiIF.ConfCallState.FOREGROUND
                call = foregroundCall;
                break;
            case ImsQmiIF.BACKGROUND: //ImsQmiIF.ConfCallState.BACKGROUND
                call = backgroundCall;
                break;
            default:
                log("no call found with given call state");
                break;
        }
        return call;
    }

    public void handleRefreshConfInfo(ImsQmiIF.ConfInfo confRefreshInfo) {
        log("handleRefreshConfInfo" + foregroundCall);
        byte[] confInfo = null;
        int state = -1;
        ImsCall call = null;
        if (confRefreshInfo != null) {
            final com.google.protobuf.micro.ByteStringMicro refreshConfInfoUri = confRefreshInfo
                    .getConfInfoUri();
            if (refreshConfInfoUri != null
                    && refreshConfInfoUri.size() >= 1) {
                confInfo = new byte[refreshConfInfoUri.size()];
                refreshConfInfoUri.copyTo(confInfo, 0);
                if (confRefreshInfo.hasConfCallState()) {
                    state = confRefreshInfo.getConfCallState();
                    call = getCallFromConfCallState(state);
                } else {
                    /*
                     * defaulting to foreground call for backward compatibility
                     * before call state was added to the interface
                     */
                    call = foregroundCall;
                }
            }
        }
        log("handleRefreshConfInfo confRefreshInfo \n" + "callstate: " + state + "\n" +
                "call : " + call + "\n" +
                " confRefreshInfo : " + confRefreshInfo);
        /*
         * UE subscribes for conference xml as soon as it establishes session
         * with conference server.Multiparty bit will be updated only through
         * Get_current_calls after all the participants are merged to the call.
         * So refresh info can be received during the interval in which the
         * conference request is sent and before the conference call reflects in
         * the Get_current-calls
         */
        if (confInfo != null && call != null) {
            log("Update UI for Conference");
            call.setConfInfoIfRequired(confInfo);
            /* TODO: Enhancement - If on the Main Window of the Conference Call display
             * a toast here saying a new caller has been added or existing caller has
             * been removed. If on the window for member list for conference, it has to
             * be updated from here by calling notifyPhoneStateChanged possibly.
             */
        }
    }

    public String[] getCallDetailsExtras(int callId) {
        String[] extras = null;
        if (callId >= 0 && callId < MAX_CONNECTIONS) {
            ImsConnection conn = connections[callId];
            if (conn != null && conn.callDetails != null && conn.callDetails.extras != null) {
                extras = conn.callDetails.extras;
            }
        }
        Log.d(LOG_TAG, "Call Details extras = " + extras);
        return extras;
    }

    public String getImsDisconnectCauseInfo(int callId) {
        String cause = null;
        if (callId >= 0 && callId < MAX_CONNECTIONS) {
            ImsConnection conn = connections[callId];
            if (conn != null && conn.disconnected) {
                cause = conn.getImsDisconnectCauseInfo();
            }
        }
        Log.d(LOG_TAG, "Disconnect Cause = " + cause);
        return cause;
    }

    public String[] getUriListinConf() {
        ImsConnection conn = (ImsConnection)foregroundCall.connections.get(0);
        return conn.getCall().getConfUriList();
    }

    public boolean getIsServiceAllowed(int service) {
        boolean allowed = true;
        ImsConnection conn = (ImsConnection)foregroundCall.getLatestConnection();
        if (conn != null) {
            switch (service) {
                case CallDetails.CALL_TYPE_VT:
                    allowed = conn.isVTAllowed();
                    break;
                case CallDetails.CALL_TYPE_VOICE:
                    allowed = conn.isVoiceAllowed();
                    break;
                default:
                    Log.e(LOG_TAG, "Always allowed Service " + service);
                    break;
            }
        }
        return allowed;
    }

    /**
     * Handle Ecm timer to be canceled or re-started
     */
    private void handleEcmTimer(int action) {
        phone.handleTimerInEmergencyCallbackMode(action);
        switch(action) {
            case ImsPhone.CANCEL_ECM_TIMER: mIsEcmTimerCanceled = true; break;
            case ImsPhone.RESTART_ECM_TIMER: mIsEcmTimerCanceled = false; break;
            default:
                Log.e(LOG_TAG, "handleEcmTimer, unsupported action " + action);
        }
    }

}
