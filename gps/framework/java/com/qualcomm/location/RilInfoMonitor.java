/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
  Copyright (c) 2012-2014 Qualcomm Atheros, Inc.
  All Rights Reserved.
  Qualcomm Atheros Confidential and Proprietary.
=============================================================================*/

package com.qualcomm.location;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;
import android.os.Message;
import android.provider.Telephony.Sms.Intents;
import android.telephony.CellInfo;
import android.telephony.CellLocation;
import android.telephony.CellInfoGsm;
import android.telephony.CellIdentityGsm;
import android.telephony.PhoneStateListener;
import android.telephony.ServiceState;
import android.telephony.SmsMessage;
import android.telephony.TelephonyManager;
import android.telephony.gsm.GsmCellLocation;

import android.util.Log;
import java.util.List;
import java.lang.ClassCastException;
import java.lang.ArrayIndexOutOfBoundsException;
import com.qualcomm.location.MonitorInterface.Monitor;
import com.qualcomm.location.tel.TelMgrProxy;
import com.qualcomm.location.tel.TelMgrProxy.TelProxyPhoneStateListener;
import com.qualcomm.location.tel.WeighedRatInfo;

public class RilInfoMonitor extends Monitor {
    private static final String TAG = "RilInfoMonitor";
    private static final boolean VERBOSE_DBG = Log.isLoggable(TAG, Log.VERBOSE);
    private static final int MSG_START = 0;
    private static final int MSG_CID_INJECT = 1;
    private static final int MSG_OOS_INJECT = 2;
    private static final int MSG_SMS_INJECT = 3;
    private static final int MSG_SMS_MULTI_INJECT = 4;
    private static final int MSG_MAX = 5;
    private TelMgrProxy mTelMgr;
    private int mSimConfig = TelMgrProxy.SingleSim;
    private String mHomeOperator;
    private final RilPhoneStateListener mListener;

    private final BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
        @Override public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();

            if (VERBOSE_DBG)
                logv("mBroadcastReceiver - " + action);

            if (action.equals(Intents.DATA_SMS_RECEIVED_ACTION)) {
                SmsMessage[] messages = Intents.getMessagesFromIntent(intent);
                for (int i=0; i <messages.length; i++) {
                    sendMessage(MSG_SMS_INJECT, 0, 0, messages[i].getUserData());
                }
            } else if (action.equals(Intents.WAP_PUSH_RECEIVED_ACTION)) {
                sendMessage(MSG_SMS_INJECT, 0, 0, intent.getByteArrayExtra("data"));
            }
        }
    };

    private final class RilPhoneStateListener extends TelProxyPhoneStateListener {
        @Override
        public void onCellLocationChanged(CellLocation location) {
            logv("onCellLocationChanged () called");
            List<CellInfo> cellInfoValue;

            /*we proactively read the cell info through
            telephony manager.*/
            if (VERBOSE_DBG) {
                logv("cellInfoValue = mTelMgr.getAllCellInfo()");
            }
            cellInfoValue = mTelMgr.getAllCellInfo();

            if (cellInfoValue != null) {
                logv("cellInfoValue != null");
                for (CellInfo ci : cellInfoValue) {
                    if (ci instanceof CellInfoGsm && ci.isRegistered()) {
                        if (VERBOSE_DBG)
                            logv("ci instanceof CellInfoGsm && ci.isRegistered()"
                                  +"Inject will happen.");

                        CellInfoGsm gsmCell = (CellInfoGsm)ci;
                        CellIdentityGsm cellIdentityGsm = gsmCell.getCellIdentity();

                        //only inject GSM info
                        if (cellIdentityGsm != null) {
                            logv("sendMessage(MSG_CID_INJECT, 0, 0, cellIdentityGsm)");
                            sendMessage(MSG_CID_INJECT, 0, 0, cellIdentityGsm);
                        }
                        break;
                    }
                }
            } else {
                logv("cellInfoValue == null");
            }
        }

        @Override
        public void onServiceStateChanged(ServiceState serviceState) {
            if (ServiceState.STATE_OUT_OF_SERVICE == serviceState.getState() &&
                ServiceState.STATE_OUT_OF_SERVICE == serviceState.getDataRegState()) {
                if (VERBOSE_DBG)
                    logv("onServiceStateChanged - OOS");

                sendMessage(MSG_OOS_INJECT, 0, 0, null);
            }
        }

        @Override
        public void onCallStateChanged(int state, String incomingNumber) {
            Log.d(TAG, "onCallStateChanged(): "
                 +"state is "+state
                 +"incomingNumber is "+incomingNumber);
            // listening for emergency call ends
            if (state == TelephonyManager.CALL_STATE_IDLE) {
                Log.d(TAG, "state == TelephonyManager.CALL_STATE_IDLE");
                if (GpsNetInitiatedHandler.obj == null) {
                    Log.e(TAG, "GpsNetInitiatedHandler is NULL!");
                } else {
                    Log.d(TAG, "calling GpsNetInitiatedHandler"
                         +".obj.updateEmergencySUPLStatus(false)");
                    GpsNetInitiatedHandler.obj.updateEmergencySUPLStatus(false);
                }
            }
        }
    }

    public RilInfoMonitor(MonitorInterface service, int msgIdBase, int config) {
        super(service, msgIdBase);
        mListener = new RilPhoneStateListener();
        sendMessage(MSG_START, config, 0, null);
    }

    private void startMonitor(int config) {
        mTelMgr = TelMgrProxy.get(mMoniterService.getContext(), mListener);
	int mask = 0;
        switch (config) {
        case LBSSystemMonitorService.SGLTE_NO_ES_SUPL:
            // SGLTE without ES SUPL
            Log.d(TAG, "SGLTE without ES SUPL");
            mask = PhoneStateListener.LISTEN_CELL_LOCATION |
                           PhoneStateListener.LISTEN_SERVICE_STATE;
            break;
        case LBSSystemMonitorService.SGLTE_WITH_ES_SUPL:
            // SGLTE with ES SUPL
            Log.d(TAG, "SGLTE with ES SUPL");
            mask = PhoneStateListener.LISTEN_CELL_LOCATION |
                           PhoneStateListener.LISTEN_SERVICE_STATE |
                           PhoneStateListener.LISTEN_CALL_STATE;
            break;
        case LBSSystemMonitorService.NON_SGLTE_WITH_ES_SUPL:
            // nonSGLTE device with ES SUPL
            Log.d(TAG, "nonSGLTE device with ES SUPL");
            mask = PhoneStateListener.LISTEN_CALL_STATE;
            break;
        }

        mSimConfig = mTelMgr.getSimConfig(mTelMgr);
        if (mSimConfig == TelMgrProxy.MultiSim) {
            logv("Multi Sim config detected");
        }
        else {
            logv("Single Sim config detected");
        }

        mHomeOperator = mTelMgr.getHomeOperator();
	if (mask != 0) {
            mTelMgr.listen(mask);
            if ((mask & (PhoneStateListener.LISTEN_CELL_LOCATION |
                         PhoneStateListener.LISTEN_SERVICE_STATE)) != 0) {
        	IntentFilter filter = new IntentFilter();
        	filter.addAction(Intents.DATA_SMS_RECEIVED_ACTION);
       		filter.addDataScheme("sms");
        	filter.addDataAuthority("localhost","7275");
        	mMoniterService.getContext().registerReceiver(mBroadcastReceiver, filter);

       		filter = new IntentFilter();
        	filter.addAction(Intents.WAP_PUSH_RECEIVED_ACTION);
        	try {
            		filter.addDataType("application/vnd.omaloc-supl-init");
        	} catch (IntentFilter.MalformedMimeTypeException e) {
            		Log.w(TAG, "Malformed SUPL init mime type");
        	}
        	mMoniterService.getContext().registerReceiver(mBroadcastReceiver, filter);

		}
	}
    }

    @Override
    public void handleMessage(Message msg) {
        int message = msg.what;
        Log.d(TAG, "handleMessage what - " + message);

        try {
            switch (message) {
            case MSG_START:
                startMonitor(msg.arg1);
                native_rm_init();
                break;
            case MSG_CID_INJECT:
                CellIdentityGsm cellIdentityGsm = (CellIdentityGsm) msg.obj;
                int cid = cellIdentityGsm.getCid();
                int lac = cellIdentityGsm.getLac();
                int mnc = cellIdentityGsm.getMnc();
                int mcc = cellIdentityGsm.getMcc();

                if (cid == -1) {
                    logv("GSM is OOS");
                    lac = -1; // overwrite lac when GSM is OOS
                }
                logv("onCellInfoChanged: cid - " + cid +
                      "; lac - " + lac + "; mnc - " + mnc +
                      "; mcc - " + mcc);
                if (mnc >= 0) {
                    native_rm_cinfo_inject(cid, lac, mnc, mcc, getRoaming());
                }
                break;
            case MSG_OOS_INJECT:
                native_rm_oos_inform();
                break;
            case MSG_SMS_INJECT:
                byte[] data = (byte[]) msg.obj;
                native_rm_ni_supl_init(data, data.length);
                break;
            case MSG_SMS_MULTI_INJECT:
                break;
            default:
                break;
            }
        } catch (ClassCastException cce) {
            Log.w(TAG, "ClassCastException on " + message);
        }
    }

    private static native void native_rm_class_init();
    private native void native_rm_init();
    public native void native_rm_cinfo_inject(int cid, int lac, int mnc, int mcc, boolean roaming);
    public native void native_rm_oos_inform();
    public native void native_rm_ni_supl_init(byte[] supl_init, int length);

    @Override
    public int getNumOfMessages() {
        return MSG_MAX;
    }

    private boolean getRoaming() {
        boolean roaming = mTelMgr.getRoaming();
        logv("getRoaming() - "+roaming);
        return roaming;
    }

    static private void logv(String s) {
        if (VERBOSE_DBG) Log.v(TAG, s);
    }
}
