/*
 * Copyright (c) 2012 Qualcomm Technologies, Inc.
 * All Rights Reserved. Qualcomm Technologies Confidential and Proprietary.
 *
 * Not a Contribution, Apache license notifications and license are retained
 * for attribution purposes only.
 *
 * Copyright (C) 2006 The Android Open Source Project
 * Copyright (c) 2012 Code Aurora Forum. All rights reserved.
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

import com.android.internal.telephony.Call;
import com.android.internal.telephony.CallStateException;
import com.android.internal.telephony.Connection;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneBase;
import com.android.internal.telephony.Call.State;
import com.android.internal.telephony.gsm.GsmCallTracker;
import com.android.internal.telephony.DriverCall;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.codeaurora.ims.conference.ConfInfo;

import android.util.Log;

/**
 * {@hide}
 */
public class ImsCall extends Call {
    /*************************** Instance Variables **************************/

    /* package */ImsCallTracker owner;
    protected final String LOG_TAG = "IMSCall";
    public ArrayList<Connection> connections = new ArrayList<Connection>();
    public ConfInfo confDetailsMap = null;

    /****************************** Constructors *****************************/
    public ImsCall(ImsCallTracker owner) {
        this.owner = owner;
        confDetailsMap = new ConfInfo();
    }

    public void dispose() {
    }

    /************************** Overridden from Call *************************/
    public List<Connection> getConnections() {
        return Collections.unmodifiableList(connections);
    }

    public static State stateFromDCState(DriverCall.State dcState) {
        switch (dcState) {
            case ACTIVE:
                return State.ACTIVE;
            case HOLDING:
                return State.HOLDING;
            case DIALING:
                return State.DIALING;
            case ALERTING:
                return State.ALERTING;
            case INCOMING:
                return State.INCOMING;
            case WAITING:
                return State.WAITING;
            default:
                throw new RuntimeException("illegal call state:" + dcState);
        }
    }

    public ImsPhone getPhone() {
        return owner.phone;
    }

    public boolean isMultiparty() {
        return isMpty;
    }

    /**
     * Please note: if this is the foreground call and a background call exists,
     * the background call will be resumed because an AT+CHLD=1 will be sent
     */
    public void hangup() throws CallStateException {
        owner.hangup(this);

    }

    public String toString() {
        return mState.toString();
    }

    // ***** Called from ImsConnection

    public void attach(Connection conn, DriverCallIms dc) {
        connections.add(conn);
        if (isMpty != dc.isMpty) {
            isMpty = dc.isMpty;
        }
        if (connections.size() == 1) {
            assignConfDetailsFromConnectionToCall(conn);
        }
        mState = stateFromDCState(dc.state);
    }

    public void attachFake(Connection conn, State state, boolean multiparty) {
        connections.add(conn);
        isMpty = multiparty;
        this.mState = state;
        if (connections.size() == 1) {
            assignConfDetailsFromConnectionToCall(conn);
        }
    }

    private void assignConfDetailsFromConnectionToCall(Connection conn) {
        ImsConnection imsCon = (ImsConnection) conn;
        confDetailsMap = imsCon.confDetailsMap;
        setConfUriList(confDetailsMap.getUserUriList());
    }

    public void setConfInfoIfRequired(byte[] confInfo) {
        confDetailsMap.isUpdateRequired(confInfo);
        setConfUriList(confDetailsMap.getUserUriList());
    }

    /**
     * Called by Connection when it has disconnected
     */
    /*package*/ boolean
    connectionDisconnected(Connection conn) {
        if (mState != State.DISCONNECTED) {
            /* If only disconnected connections remain, we are disconnected */

            boolean hasOnlyDisconnectedConnections = true;

            for (int i = 0, s = connections.size(); i < s; i++) {
                if (connections.get(i).getState() != State.DISCONNECTED) {
                    hasOnlyDisconnectedConnections = false;
                    break;
                }
            }

            if (hasOnlyDisconnectedConnections) {
                mState = State.DISCONNECTED;
                return true;
            }
        }

        return false;
    }

    public void detach(Connection conn) {
        connections.remove(conn);
        isMpty = false;
        if (connections.isEmpty()) {
            mState = State.IDLE;
            confDetailsMap.clearAndSetDefault();
        }
    }

    public boolean update(Connection conn, DriverCallIms dc) {
        State newState;
        boolean changed = false;

        newState = stateFromDCState(dc.state);

        if (newState != mState) {
            mState = newState;
            changed = true;
        }

        if (isMpty != dc.isMpty) {
            isMpty = dc.isMpty;
            changed = true;
        }
        Log.d(LOG_TAG, "update in Ims Call changed" + changed);
        return changed;
    }

    /**
     * @return true if there's no space in this call for additional connections
     *         to be added via "conference"
     */
    public boolean isFull() {
        return connections.size() == ImsCallTracker.MAX_CONNECTIONS_PER_CALL;
    }

    /**
     * Called when this Call is being hung up locally (eg, user pressed "end")
     * Note that at this point, the hangup request has been dispatched to the
     * radio but no response has yet been received so update() has not yet been
     * called
     */
    public void onHangupLocal() {
        for (int i = 0, s = connections.size(); i < s; i++) {
            ImsConnection cn = (ImsConnection) connections.get(i);

            cn.onHangupLocal();
        }
        mState = State.DISCONNECTING;
    }

    /**
     * Called when it's time to clean up disconnected Connection objects
     */
    public void clearDisconnected() {
        for (int i = connections.size() - 1; i >= 0; i--) {
            Connection cn = (Connection) connections.get(i);

            if (cn.getState() == State.DISCONNECTED) {
                connections.remove(i);
            }
        }

        if (connections.size() == 0) {
            mState = State.IDLE;
        }
    }
}
