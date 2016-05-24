/*
 * Copyright (c) 2014 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

package org.codeaurora.ims;

import com.android.internal.telephony.Call;
import com.android.internal.telephony.CallDetails;

public class ImsCallUtils {
    /** Checks if a call type is any valid video call type with or without direction
     */
    public static boolean isVideoCall(int callType) {
        return callType == CallDetails.CALL_TYPE_VT
                || callType == CallDetails.CALL_TYPE_VT_TX
                || callType == CallDetails.CALL_TYPE_VT_RX
                || callType == CallDetails.CALL_TYPE_VT_PAUSE
                || callType == CallDetails.CALL_TYPE_VT_RESUME
                || callType == CallDetails.CALL_TYPE_VT_NODIR;
    }

    /** Check if call type is valid for lower layers
     */
    public static boolean isValidRilModifyCallType(int callType){
        return callType == CallDetails.CALL_TYPE_VT
                || callType == CallDetails.CALL_TYPE_VT_TX
                || callType == CallDetails.CALL_TYPE_VT_RX
                || callType == CallDetails.CALL_TYPE_VOICE
                || callType == CallDetails.CALL_TYPE_VT_NODIR;
    }

    /** Checks if videocall state transitioned to Video Paused state
     * @param conn - Current connection object
     * @param dc - Latest DriverCallIms instance
     */
    public static boolean isVideoPaused(ImsConnection conn, DriverCallIms dc) {
        int currCallType = conn.callDetails.call_type;
        Call.State currCallState = conn.getState();
        int nextCallType = dc.callDetails.call_type;
        DriverCallIms.State nextCallState = dc.state;

        return (isVideoCall(currCallType)
                && (currCallState == Call.State.ACTIVE)
                && isVideoCallTypeWithoutDir(nextCallType)
                && (nextCallState == DriverCallIms.State.ACTIVE));
    }

    /** Detects active video call
     */
    public static boolean canVideoPause(ImsConnection conn) {
        return isVideoCall(conn.callDetails.call_type) && conn.getState() == Call.State.ACTIVE;
    }

    /** Checks if videocall state transitioned to Video Resumed state
     * @param conn - Current connection object
     * @param dc - Latest DriverCallIms instance
     */
    public static boolean isVideoResumed(ImsConnection conn, DriverCallIms dc) {
        int currCallType = conn.callDetails.call_type;
        Call.State currCallState = conn.getState();
        int nextCallType = dc.callDetails.call_type;
        DriverCallIms.State nextCallState = dc.state;

        return (isVideoCallTypeWithoutDir(currCallType)
                && (currCallState == Call.State.ACTIVE)
                && isVideoCall(nextCallType)
                && (nextCallState == DriverCallIms.State.ACTIVE));
    }

    /** Checks if AVP Retry needs to be triggered during dialing
     * @param conn - Current connection object
     * @param dc - Latest DriverCallIms instance
     */
    public static boolean isAvpRetryDialing(ImsConnection conn, DriverCallIms dc) {
        int currCallType = conn.callDetails.call_type;
        Call.State currCallState = conn.getState();
        int nextCallType = dc.callDetails.call_type;
        DriverCallIms.State nextCallState = dc.state;

        boolean dialingAvpRetry = (isVideoCall(currCallType)
                    && (currCallState == Call.State.DIALING || currCallState == Call.State.ALERTING)
                    && isVideoCallTypeWithoutDir(nextCallType)
                    && nextCallState == DriverCallIms.State.ACTIVE);
        return (conn.isAvpRetryAllowed() && dialingAvpRetry);
    }

    /** Checks if AVP Retry needs to be triggered during upgrade
     * @param conn - Current connection object
     * @param dc - Latest DriverCallIms instance
     */
    public static boolean isAvpRetryUpgrade(ImsConnection conn, DriverCallIms dc) {
        int currCallType = conn.callDetails.call_type;
        Call.State currCallState = conn.getState();
        int nextCallType = dc.callDetails.call_type;
        DriverCallIms.State nextCallState = dc.state;

        boolean upgradeAvpRetry = (currCallType == CallDetails.CALL_TYPE_VOICE
                    && currCallState == Call.State.ACTIVE
                    && isVideoCallTypeWithoutDir(nextCallType)
                    && nextCallState == DriverCallIms.State.ACTIVE);
        return (conn.isAvpRetryAllowed() && upgradeAvpRetry);
    }

    /** Checks if a call type is video call type with direction
     * @param callType
     */
    public static boolean isVideoCallTypeWithDir(int callType) {
        return callType == CallDetails.CALL_TYPE_VT
                || callType == CallDetails.CALL_TYPE_VT_RX
                || callType == CallDetails.CALL_TYPE_VT_TX;
    }

    /** Checks if a call type is video call type without direction
     * @param callType
     */
    public static boolean isVideoCallTypeWithoutDir(int callType) {
        return callType == CallDetails.CALL_TYPE_VT_NODIR;
    }
}
