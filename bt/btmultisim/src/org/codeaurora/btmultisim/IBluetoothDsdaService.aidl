/* Copyright (c) 2014 Qualcomm Technologies, Inc.
 * All Rights Reserved. Qualcomm Technologies Confidential and Proprietary.
 */

package org.codeaurora.btmultisim;

/**
 * Interface used to interact with BluetoothPhoneService to handle DSDA.
 *
 * {@hide}
 */
interface IBluetoothDsdaService {
    void setCurrentSub(int sub);
    void phoneSubChanged();
    void handleMultiSimPreciseCallStateChange();
    void handleListCurrentCalls();
    void processQueryPhoneState();
    void handleCdmaSwapSecondCallState();
    void handleCdmaSetSecondCallState(boolean state);
    void setCurrentCallState(int currCallState, int prevCallState,
    boolean IsThreeWayCallOrigStateDialing);
    int getTotalCallsOnSub(int subId);
    boolean isSwitchSubAllowed();
    void SwitchSub();
    boolean canDoCallSwap();
    boolean answerOnThisSubAllowed();
    void updateCdmaHeldCall(int numheld);
}

