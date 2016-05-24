/*
 *
 * Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
package com.qualcomm.universaldownload;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.telephony.MSimTelephonyManager;
import android.telephony.TelephonyManager;
import android.util.Log;

public class NetManager {
    private static final String TAG = "Download.NetManager";
    private static final boolean DEBUG = true;
    private static boolean mOrigData = true;
    private static int mOrigSub = 0;
    private static boolean mConnectSettingChanged = false;
    private static String CHINA_TELECOM_CODE = "46003, 46005";

    public static boolean isCTCardInsert(Context context) {
        MSimTelephonyManager mSimTelephonyManager = MSimTelephonyManager.getDefault();
        for(int i = 0; i < mSimTelephonyManager.getPhoneCount(); i++) {
            if( CTCardAvailable(context, i)) {
                return true;
            }
        }
        return false;
    }

    public static boolean haveNetConnective(Context context) {
        boolean dataEnable;
        ConnectivityManager cm = (ConnectivityManager)
                context.getSystemService(Context.CONNECTIVITY_SERVICE);
        MSimTelephonyManager mSimTelephonyManager = MSimTelephonyManager.getDefault();
        //mobile 3G Data Network
        NetworkInfo.State mobile = cm.getNetworkInfo(ConnectivityManager.TYPE_MOBILE).getState();
        //wifi
        NetworkInfo.State wifi = cm.getNetworkInfo(ConnectivityManager.TYPE_WIFI).getState();

        logd("haveNetConnective, wifi state is " + wifi + ", mobile state is " + mobile);
        //only when have wifi connection or sim1 is CT card and data active to update
        if(wifi == NetworkInfo.State.CONNECTED) {
            return true;
        }

        if(mSimTelephonyManager.isMultiSimEnabled()) {
            dataEnable = (mobile == NetworkInfo.State.CONNECTED && CTCardAvailable(context, 0)
                    && mSimTelephonyManager.getPreferredDataSubscription() == 0);
        } else {
            dataEnable = (mobile == NetworkInfo.State.CONNECTED);
        }
        return dataEnable;
    }

    public static boolean CTCardAvailable(Context context, int sub) {
        MSimTelephonyManager mSimTelephonyManager = MSimTelephonyManager.getDefault();
        if(mSimTelephonyManager.isMultiSimEnabled()) {
            logd("sim" + sub + "'s state is " + mSimTelephonyManager.getSimState(sub));
            logd("operator is " + mSimTelephonyManager.getSimOperator(sub));
            return  mSimTelephonyManager.getSimState(sub) == TelephonyManager.SIM_STATE_READY &&
                    CHINA_TELECOM_CODE.contains(mSimTelephonyManager.getSimOperator(sub));
        } else {
            TelephonyManager telephonyManager = (TelephonyManager)
                    context.getSystemService(Context.TELEPHONY_SERVICE);
            logd("sim's state is " + telephonyManager.getSimState());
            logd("operator is " + telephonyManager.getSimOperator());
            return telephonyManager.getSimState() == TelephonyManager.SIM_STATE_READY &&
                    CHINA_TELECOM_CODE.contains(telephonyManager.getSimOperator());
        }
    }

    public static void setDataEnableTo(Context context, int subscribe) {
        MSimTelephonyManager mSimTelephonyManager = MSimTelephonyManager.getDefault();
        ConnectivityManager cm = (ConnectivityManager)
                context.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo.State state = cm.getNetworkInfo(ConnectivityManager.TYPE_MOBILE).getState();
        if(state != NetworkInfo.State.CONNECTED) {
            mOrigData = false;
            cm.setMobileDataEnabled(true);
            mConnectSettingChanged = true;
        }
        if(mSimTelephonyManager.isMultiSimEnabled()) {
            if(mSimTelephonyManager.getPreferredDataSubscription() != subscribe) {
                mOrigSub = mSimTelephonyManager.getPreferredDataSubscription();
                mSimTelephonyManager.setPreferredDataSubscription(subscribe);
                mConnectSettingChanged = true;
            }
        }
    }

    public static void resetDataConnect(Context context) {
        ConnectivityManager cm = (ConnectivityManager)
                context.getSystemService(Context.CONNECTIVITY_SERVICE);
        MSimTelephonyManager mSimTelephonyManager = MSimTelephonyManager.getDefault();
        if(mSimTelephonyManager.isMultiSimEnabled()) {
            mSimTelephonyManager.setPreferredDataSubscription(mOrigSub);
        }
        cm.setMobileDataEnabled(mOrigData);
        mConnectSettingChanged = false;
    }

    public static boolean isConnectSettingChanged() {
        return mConnectSettingChanged;
    }

    public static String getMEID(Context context) {
        MSimTelephonyManager mSimTelephonyManager = MSimTelephonyManager.getDefault();
        boolean isMultiSimEnabled = mSimTelephonyManager.isMultiSimEnabled();
        if (isMultiSimEnabled) {
            return mSimTelephonyManager.getDeviceId(0);
        } else {
            TelephonyManager telephonyManager =
                    (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
            return telephonyManager.getDeviceId();
        }
    }

    public static String getIMSI(Context context) {
        MSimTelephonyManager mSimTelephonyManager = MSimTelephonyManager.getDefault();
        boolean isMultiSimEnabled = mSimTelephonyManager.isMultiSimEnabled();
        if (isMultiSimEnabled) {
            return mSimTelephonyManager.getSubscriberId(0);
        } else {
            TelephonyManager telephonyManager =
                    (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
            return telephonyManager.getSubscriberId();
        }
    }

    private static void logd(String msg) {
        if(DEBUG) {
            Log.d(TAG, msg);
        }
    }
}
