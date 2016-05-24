/*
 * Copyright (c) 2012-2013, Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 *
 */

package com.qualcomm.simcontacts;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.telephony.MSimTelephonyManager;
import android.util.Log;

import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.MSimConstants;
import com.android.internal.telephony.TelephonyIntents;

public class SimStateReceiver extends BroadcastReceiver {
    private static boolean DBG = true;
    private static String TAG = "SimStateReceiver";

    private Context mContext;

    @Override
    public void onReceive(Context context, Intent intent) {
        final String action = intent.getAction();
        mContext = context;
        if (DBG)
            log("received broadcast " + action);
        if (TelephonyIntents.ACTION_SIM_STATE_CHANGED.equals(action)) {
            final int subscription = intent.getIntExtra(MSimConstants.SUBSCRIPTION_KEY,
                    MSimConstants.DEFAULT_SUBSCRIPTION);
            final String stateExtra = intent.getStringExtra(IccCardConstants.INTENT_KEY_ICC_STATE);
            final int simState;
            if (DBG)
                log("ACTION_SIM_STATE_CHANGED intent received on sub = " + subscription
                        + "SIM STATE IS " + stateExtra);

            if (IccCardConstants.INTENT_VALUE_ICC_LOADED.equals(stateExtra)) {
                simState = SimContactsConstants.SIM_STATE_LOAD;
            } else if (IccCardConstants.INTENT_VALUE_ICC_IMSI.equals(stateExtra)
                    || IccCardConstants.INTENT_VALUE_ICC_READY.equals(stateExtra)) {
                simState = SimContactsConstants.SIM_STATE_READY;
            }
            else if (IccCardConstants.INTENT_VALUE_ICC_ABSENT.equals(stateExtra)
                    || IccCardConstants.INTENT_VALUE_ICC_UNKNOWN.equals(stateExtra)
                    || IccCardConstants.INTENT_VALUE_ICC_CARD_IO_ERROR.equals(stateExtra)) {
                simState = SimContactsConstants.SIM_STATE_ERROR;
            } else {
                simState = SimContactsConstants.SIM_STATE_NOT_READY;
            }
            sendSimState(subscription, simState);
        } else if (Intent.ACTION_BOOT_COMPLETED.equals(action)) {
            sendPhoneBoot();
        } else if ("org.codeaurora.intent.action.ACTION_SIM_REFRESH_UPDATE".equals(action)) {
            final int subscription = intent.getIntExtra(MSimConstants.SUBSCRIPTION_KEY,
                    MSimConstants.DEFAULT_SUBSCRIPTION);
            if (DBG)
                log("ACTION_SIM_REFRESH_UPDATE intent received on sub = " + subscription);
            sendSimRefreshUpdate(subscription);
        }
    }

    private void sendPhoneBoot() {
        Bundle args = new Bundle();
        args.putInt(SimContactsService.OPERATION, SimContactsService.MSG_BOOT_COMPLETE);
        mContext.startService(new Intent(mContext, SimContactsService.class)
                .putExtras(args));
    }

    private void sendSimState(int subscription, int state) {
        Bundle args = new Bundle();
        args.putInt(MSimConstants.SUBSCRIPTION_KEY, subscription);
        args.putInt(SimContactsService.OPERATION, SimContactsService.MSG_SIM_STATE_CHANGED);
        args.putInt(SimContactsService.SIM_STATE, state);
        mContext.startService(new Intent(mContext, SimContactsService.class)
                .putExtras(args));
    }

    private void sendSimRefreshUpdate(int subscription) {
        Bundle args = new Bundle();
        args.putInt(SimContactsService.OPERATION, SimContactsService.MSG_SIM_REFRESH);
        args.putInt(MSimConstants.SUBSCRIPTION_KEY, subscription);
        mContext.startService(new Intent(mContext, SimContactsService.class)
                .putExtras(args));
    }

    protected void log(String msg) {
        Log.d(TAG, msg);
    }
}
