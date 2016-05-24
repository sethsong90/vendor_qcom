/**
 * Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

package com.qualcomm.roamingsettings;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.telephony.MSimTelephonyManager;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.android.internal.telephony.MSimConstants;
import com.android.internal.telephony.OperatorInfo;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneFactory;
import com.codeaurora.telephony.msim.MSimPhoneFactory;
import com.qualcomm.qcrilhook.IQcRilHook;
import com.qualcomm.qcrilhook.QcRilHook;
import com.qualcomm.qcrilhook.QcRilHookCallback;
import com.qualcomm.roamingsettings.RoamingSettingsFragmet.AttentionDialog;
import com.qualcomm.roamingsettings.RoamingSettingsFragmet.MyProgressDialog;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

public class ManualNetworkActivity extends Activity implements OnClickListener {
    private static final String TAG = "RoamingSettingsApp";

    // Events definition
    private static final int EVENT_GET_NETWORK_TYPE = 0;
    private static final int EVENT_SET_NETWORK_TYPE = 1;
    private static final int EVENT_CDMA_ENABLED = 2;
    private static final int EVENT_NETWORK_RETRY_SCAN = 3;
    private static final int EVENT_NETWORK_SCAN_COMPLETED = 4;
    private static final int EVENT_NETWORK_SELECTION_DONE = 5;
    private static final int EVENT_AUTO_SELECT_DONE = 6;

    private static final int EVENT_INVALID = 10;
    private static final int EVENT_DATA_DISABLED = 11;
    private static final int EVENT_AVOID_CURRENT_NETWORK = 12;
    private static final int EVENT_AVOID_CURRENT_NETWORK_COMPLETED = 13;
    private static final int EVENT_CLEAR_AVOIDANCE_LIST = 14;
    private static final int EVENT_CLEAR_AVOIDANCE_LIST_COMPLETED = 15;
    private static final int EVENT_GET_AVOIDANCE_LIST = 16;
    private static final int EVENT_GET_AVOIDANCE_LIST_COMPLETED = 17;
    private static final int EVENT_UNABLE_TO_DISABLE_DATA = 18;

    private static final int SCAN_RETRY_COUNT = 5;
    private static final int MESSAGE_DELAY_MILLIS = 1000;
    private static final int BACK_KEY_CMD = 99;

    private Button mCdmaOpt;
    private Button mGsmOpt;
    private Button mAvoidNetwork;
    private TextView mGsmTitle;
    private TextView mNetworkTip;
    private TextView mEffectTip;
    private TextView mCdmaLoading;
    private TextView mGsmLoading;
    private ListView mGsmListView;
    private LinearLayout mSubCdmaLayout;
    private LinearLayout mSubGsmLayout;
    private MyProgressDialog progress;

    //map of network controls to the network data.
    private HashMap<String, OperatorInfo> mNetworkMap;

    private Phone mPhone = null;

    private int mSubscription;
    private int mNetworkMode;
    private boolean mIsForceStop;
    private boolean mIsMultiSimEnabled;
    private String mNetworkName;

    private CdmaConfig mCdmaConfig = null;

    private static int sNetworkScanCount = 0;

    /**
     * Local handler to receive the network query compete callback from the RIL.
     */
    Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            AsyncResult ar;

            switch (msg.what) {
            case EVENT_GET_NETWORK_TYPE:
                ar = (AsyncResult) msg.obj;
                if (ar.exception == null) {
                    int networkMode = ((int[]) ar.result)[0];
                    if (networkMode >= Phone.NT_MODE_WCDMA_PREF
                            && networkMode <= Phone.NT_MODE_LTE_WCDMA) {
                        mNetworkMode = getDisplayedNetworkMode(networkMode);
                    } else {
                        // The network is not an accept value, we will set the
                        // network to default.
                        Log.w(TAG, "The network(" + networkMode
                            + ") is not an accept value, we will set the network to default.");
                        Message setMsg = mHandler
                                .obtainMessage(EVENT_SET_NETWORK_TYPE);
                        setMsg.arg1 = mSubscription;
                        setMsg.arg2 = Phone.NT_MODE_LTE_CMDA_EVDO_GSM_WCDMA;
                        mPhone.setPreferredNetworkType(Phone.NT_MODE_LTE_CMDA_EVDO_GSM_WCDMA, setMsg);
                    }
                } else {
                    // There is some exception when get the network type.
                    Log.w(TAG, "GET_NETWORK_TYPE Exception: " + ar.exception);

                    // Try to use the system prop to get the network type.
                    try {
                        int networkMode = MSimTelephonyManager.getIntAtIndex(
                                mPhone.getContext().getContentResolver(),
                                Settings.Global.PREFERRED_NETWORK_MODE,
                                mSubscription);
                        mNetworkMode = getDisplayedNetworkMode(networkMode);
                    } catch (SettingNotFoundException e) {
                        Log.e(TAG, "Catch the SettingNotFoundException:" + e);
                    }
                }
                break;

            case EVENT_SET_NETWORK_TYPE:
                if (msg.arg1 == BACK_KEY_CMD) {
                    mNetworkMode = msg.arg2;
                    MSimTelephonyManager.putIntAtIndex(
                            mPhone.getContext().getContentResolver(),
                            Settings.Global.PREFERRED_NETWORK_MODE,
                            mSubscription, mNetworkMode);
                    finish();
                    break;
                }

                ar = (AsyncResult) msg.obj;
                if (ar.exception == null) {
                    mNetworkMode = msg.arg2;
                    MSimTelephonyManager.putIntAtIndex(
                            mPhone.getContext().getContentResolver(),
                            Settings.Global.PREFERRED_NETWORK_MODE,
                            mSubscription, mNetworkMode);

                    selectNetworkMode(mNetworkMode);
                } else {
                    Log.w(TAG, "SET_NETWORK_TYPE Exception: " + ar.exception);

                    // re get the network type.
                    Message get = mHandler.obtainMessage(EVENT_GET_NETWORK_TYPE);
                    get.arg1 = mSubscription;
                    mPhone.getPreferredNetworkType(get);
                }
                break;

            case EVENT_NETWORK_RETRY_SCAN:
                mPhone.getAvailableNetworks(
                        mHandler.obtainMessage(EVENT_NETWORK_SCAN_COMPLETED));
                break;

            case EVENT_NETWORK_SCAN_COMPLETED:
                Log.v(TAG, "start scan network");
                loadNetworkList((AsyncResult) msg.obj);
                break;

            case EVENT_NETWORK_SELECTION_DONE:
                cancelProgressDialog();

                ar = (AsyncResult) msg.obj;
                if (ar.exception != null) {
                    String msgStr = getString(R.string.network_select_failure_info, mNetworkName);
                    showNetworkSelectResultDialog(true, msgStr);
                } else {
                    if (mSubscription > MSimConstants.SUB1) {
                        SharedPreferences userInfo = getSharedPreferences("user_info", 0);
                        userInfo.edit().putBoolean("manual_gsm", true).commit();
                    }
                    String manualNetwork = getNetworkString(
                            MSimTelephonyManager.getDefault().getNetworkOperatorName(mSubscription),
                            R.array.original_carrier_names, R.array.local_carrier_names);
                    String msgStr = getString(R.string.network_select_success_info, manualNetwork);
                    showNetworkSelectResultDialog(false, msgStr);
                }
                break;

            case EVENT_AUTO_SELECT_DONE:
                cancelProgressDialog();
                ar = (AsyncResult) msg.obj;
                if (ar.exception != null) {
                    Log.w(TAG, "automatic network selection Exception: " + ar.exception);
                    String msgStr = getString(R.string.network_auto_failure_info);
                    showNetworkSelectResultDialog(false, msgStr);
                } else {
                    Log.v(TAG, "automatic network selection: succeeded!");
                    if (mSubscription > MSimConstants.SUB1) {
                        SharedPreferences userInfo = getSharedPreferences("user_info", 0);
                        userInfo.edit().putBoolean("manual_gsm", true).commit();
                    }
                    String msgStr = getString(R.string.network_auto_success_info);
                    showNetworkSelectResultDialog(false, msgStr);
                }
                break;

            case EVENT_DATA_DISABLED:
                mCdmaConfig.handleDataDisabled(msg);
                break;

            case EVENT_UNABLE_TO_DISABLE_DATA:
                mCdmaConfig.setNextReqEvent(EVENT_INVALID);
                Toast.makeText(getApplicationContext(), R.string.unable_to_disable_data,
                        Toast.LENGTH_SHORT).show();
                break;

            case EVENT_AVOID_CURRENT_NETWORK:
                mCdmaConfig.handleAvoidCurNwk(msg);
                break;

            case EVENT_AVOID_CURRENT_NETWORK_COMPLETED:
                Log.d(TAG, "Cdma Avoid Current Network Command Completed.");
                break;

            case EVENT_CLEAR_AVOIDANCE_LIST:
                mCdmaConfig.handleClearAvoidanceList(msg);
                break;

            case EVENT_CLEAR_AVOIDANCE_LIST_COMPLETED:
                Log.d(TAG, "Cdma Clear Avoidandce List Command Completed.");
                break;

            case EVENT_CDMA_ENABLED:
                if (mCdmaConfig == null) {
                    mCdmaConfig = new CdmaConfig(getApplicationContext());
                    updateCdmaLayout();
                    cancelProgressDialog();
                }
                break;

            default:
                Log.d(TAG, "Do nothing.");
                break;
            }
        }
    };

    @Override
    public void onCreate(Bundle bundle) {
        super.onCreate(bundle);
        setContentView(R.layout.manual_network);

        mSubscription = getIntent()
                .getIntExtra(MSimConstants.SUBSCRIPTION_KEY, MSimConstants.SUB1);
        mIsMultiSimEnabled = MSimTelephonyManager.getDefault().isMultiSimEnabled();
        if (mIsMultiSimEnabled) {
            mPhone = MSimPhoneFactory.getPhone(mSubscription);
        } else {
            mPhone = PhoneFactory.getDefaultPhone();
        }

        if (mSubscription == MSimConstants.SUB1) {
            // init the network mode
            try {
                if (mPhone != null) {
                    mNetworkMode = MSimTelephonyManager.getIntAtIndex(mPhone
                            .getContext().getContentResolver(),
                            Settings.Global.PREFERRED_NETWORK_MODE, mSubscription);
                }
            } catch (SettingNotFoundException e) {
                Log.e(TAG, "Catch the SettingNotFoundException:" + e);
            }
        }

        initManualNetworkView();
    }

    private void initManualNetworkView() {
        String slotNumber = getResources().getStringArray(
                R.array.slot_number)[mSubscription];
        String titleName = getString(R.string.slot_name, slotNumber);
        titleName += getString(R.string.manual_select_network);
        setTitle(titleName);

        mCdmaOpt = (Button) findViewById(R.id.bt_cdma_opt);
        mCdmaOpt.setOnClickListener(this);
        mGsmOpt = (Button) findViewById(R.id.bt_gsm_opt);
        mGsmOpt.setOnClickListener(this);
        mAvoidNetwork = (Button) findViewById(R.id.bt_next_network);
        mAvoidNetwork.setOnClickListener(this);
        mGsmTitle = (TextView) findViewById(R.id.tv_gsm_title);
        mNetworkTip = (TextView) findViewById(R.id.tv_network_tip);
        mEffectTip = (TextView) findViewById(R.id.tv_current_effect);
        mCdmaLoading = (TextView) findViewById(R.id.tv_cdma_loading);
        mGsmLoading = (TextView) findViewById(R.id.tv_gsm_loading);
        mGsmListView = (ListView) findViewById(R.id.lv_gsm_list);
        mSubCdmaLayout = (LinearLayout) findViewById(R.id.layout_sub_cdma);
        mSubGsmLayout = (LinearLayout) findViewById(R.id.layout_sub_gsm);

        mNetworkMap = new HashMap<String, OperatorInfo>();
        mGsmListView.setOnItemClickListener(new OnItemClickListener() {

            @Override
            public void onItemClick(AdapterView<?> listview, View view, int index,
                    long lIndex) {
                mNetworkName = listview.getItemAtPosition(index).toString();
                String msgStr = getString(R.string.register_on_network, mNetworkName);
                showProgressDailog(msgStr);

                Message msg = mHandler.obtainMessage(EVENT_NETWORK_SELECTION_DONE);
                mPhone.selectNetworkManually(mNetworkMap.get(mNetworkName), msg);
            }
        });

        if (mSubscription > MSimConstants.SUB1) {
            Log.v(TAG, "gsm network query when startup");
            mSubGsmLayout.setVisibility(View.VISIBLE);
            mGsmTitle.setVisibility(View.VISIBLE);
            mGsmLoading.setVisibility(View.VISIBLE);
            mCdmaOpt.setVisibility(View.GONE);
            mGsmOpt.setVisibility(View.GONE);
            mEffectTip.setVisibility(View.GONE);

            mPhone.getAvailableNetworks(
                    mHandler.obtainMessage(EVENT_NETWORK_SCAN_COMPLETED));

            showProgressDailog(getString(R.string.searching_gsm_network_title));
        }
    }

    @Override
    public void onBackPressed() {
        if (mSubscription == MSimConstants.SUB1) {
            Message msg = mHandler.obtainMessage(EVENT_SET_NETWORK_TYPE);
            msg.arg1 = BACK_KEY_CMD;
            msg.arg2 = Phone.NT_MODE_LTE_CMDA_EVDO_GSM_WCDMA;
            mPhone.setPreferredNetworkType(Phone.NT_MODE_LTE_CMDA_EVDO_GSM_WCDMA, msg);
        } else {
            super.onBackPressed();
        }
    }

    @Override
    public void onResume() {
        super.onResume();

        mIsForceStop = false;
    }

    @Override
    public void onPause() {
        MyProgressDialog progress = (MyProgressDialog) getFragmentManager()
                .findFragmentByTag("progress");
        if (progress != null) {
            mIsForceStop = true;
        }

        super.onPause();
    }

    private void selectNetworkAutomatic() {
        showProgressDailog(getString(R.string.register_automatically));

        Message msg = mHandler.obtainMessage(EVENT_AUTO_SELECT_DONE);
        mPhone.setNetworkSelectionModeAutomatic(msg);
    }

    private void updateCdmaLayout() {
        mCdmaLoading.setVisibility(View.GONE);
        mAvoidNetwork.setVisibility(View.VISIBLE);
        mNetworkTip.setVisibility(View.VISIBLE);
    }

    private void selectNetworkMode(int id) {
        if (id == Phone.NT_MODE_CDMA) {
            mSubCdmaLayout.setVisibility(View.VISIBLE);
            mCdmaLoading.setText(R.string.searching_cdma_network_title);
            mCdmaLoading.setVisibility(View.VISIBLE);
            mAvoidNetwork.setVisibility(View.GONE);
            mNetworkTip.setVisibility(View.GONE);

            mHandler.sendMessageDelayed(
                    mHandler.obtainMessage(EVENT_CDMA_ENABLED), MESSAGE_DELAY_MILLIS);
            showProgressDailog(getString(R.string.searching_cdma_network_title));
        }

        if (id == Phone.NT_MODE_GSM_ONLY) {
            mSubGsmLayout.setVisibility(View.VISIBLE);
            mGsmLoading.setText(R.string.searching_gsm_network_title);
            mGsmLoading.setVisibility(View.VISIBLE);
            mGsmListView.setVisibility(View.GONE);
            mPhone.getAvailableNetworks(
                    mHandler.obtainMessage(EVENT_NETWORK_SCAN_COMPLETED));
            showProgressDailog(getString(R.string.searching_gsm_network_title));
        }
    }

    private void loadNetworkList(AsyncResult ar) {
        if (ar == null) {
            Log.v(TAG, "AsyncResult is null.");
            mGsmLoading.setText(R.string.empty_networks_list);
            cancelProgressDialog();
            return;
        }

        if (ar.exception != null) {
            Log.v(TAG, "error while querying available networks");
            if (sNetworkScanCount++ < SCAN_RETRY_COUNT) {
                mHandler.sendMessageDelayed(mHandler.obtainMessage(EVENT_NETWORK_RETRY_SCAN),
                        MESSAGE_DELAY_MILLIS);
            } else {
                mGsmLoading.setText(R.string.empty_networks_list);
                cancelProgressDialog();
            }
        } else {
            cancelProgressDialog();
            ArrayList<OperatorInfo> networkList = (ArrayList<OperatorInfo>) ar.result;
            if (networkList != null) {
                List<String> data = new ArrayList<String>();
                mNetworkMap.clear();

                for (OperatorInfo ni : networkList) {
                    String itemTitle = getNetworkTitle(ni);
                    mNetworkMap.put(itemTitle, ni);
                    data.add(itemTitle);
                }
                ArrayAdapter<String> adapter = new ArrayAdapter<String>(
                        this, android.R.layout.simple_list_item_1, data);
                mGsmListView.setAdapter(adapter);
                mGsmLoading.setVisibility(View.GONE);
                mGsmListView.setVisibility(View.VISIBLE);

                if (mSubscription == MSimConstants.SUB1) {
                    selectNetworkAutomatic();
                }
            } else {
                Log.v(TAG, "empty networks list");
                mGsmLoading.setText(R.string.empty_networks_list);
            }
        }
        sNetworkScanCount = 0;
    }

    private String getNetworkTitle(OperatorInfo ni) {
        if (!TextUtils.isEmpty(ni.getOperatorAlphaLong())) {
            return ni.getOperatorAlphaLong();
        } else if (!TextUtils.isEmpty(ni.getOperatorAlphaShort())) {
            return ni.getOperatorAlphaShort();
        } else {
            return ni.getOperatorNumeric();
        }
    }

    private void showNetworkSelectResultDialog(boolean isAuto, String msgStr) {
        if (mIsForceStop) {
            return;
        }

        final boolean isAutoSelect = isAuto;

        AttentionDialog.OnPositiveClickListener listener =
                new AttentionDialog.OnPositiveClickListener() {
            @Override
            public void onClick(DialogInterface dialog) {
                if (isAutoSelect) {
                    selectNetworkAutomatic();
                }
                dialog.dismiss();
            }
        };

        AttentionDialog alert = AttentionDialog.newInstance(
                mSubscription, 0, msgStr, false, false,
                listener, null);
        alert.show(getFragmentManager(), "alert");
    }

    @Override
    public void onClick(View v) {

        int newNetworkMode = mNetworkMode;

        switch (v.getId()) {
        case R.id.bt_cdma_opt:
            newNetworkMode = Phone.NT_MODE_CDMA;
            if (newNetworkMode == mNetworkMode) {
                selectNetworkMode(mNetworkMode);
            }
            break;

        case R.id.bt_gsm_opt:
            newNetworkMode = Phone.NT_MODE_GSM_ONLY;
            if (newNetworkMode == mNetworkMode) {
                selectNetworkMode(mNetworkMode);
            }
            break;

        case R.id.bt_next_network:
            mCdmaConfig.selectAvoidCurNetwork();
            break;
        }

        if (newNetworkMode != mNetworkMode) {
            Message msg = mHandler.obtainMessage(EVENT_SET_NETWORK_TYPE);
            msg.arg1 = mSubscription;
            msg.arg2 = newNetworkMode;
            mPhone.setPreferredNetworkType(newNetworkMode, msg);
        }
    }

    /**
     * We need to transform the network mode for there only three items to show.
     *
     * @param networkMode the actual network mode
     * @return the network mode will be displayed
     */
    private int getDisplayedNetworkMode(int networkMode) {
        switch (networkMode) {
        case Phone.NT_MODE_GLOBAL:
        case Phone.NT_MODE_LTE_CMDA_EVDO_GSM_WCDMA:
            return Phone.NT_MODE_GLOBAL;
        case Phone.NT_MODE_WCDMA_PREF:
        case Phone.NT_MODE_GSM_ONLY:
        case Phone.NT_MODE_WCDMA_ONLY:
        case Phone.NT_MODE_GSM_UMTS:
        case Phone.NT_MODE_LTE_GSM_WCDMA:
        case Phone.NT_MODE_LTE_WCDMA:
            return Phone.NT_MODE_GSM_ONLY;
        case Phone.NT_MODE_CDMA:
        case Phone.NT_MODE_CDMA_NO_EVDO:
        case Phone.NT_MODE_EVDO_NO_CDMA:
        case Phone.NT_MODE_LTE_CDMA_AND_EVDO:
        case Phone.NT_MODE_LTE_ONLY:
        default:
            return Phone.NT_MODE_CDMA;
        }
    }

    private final String getNetworkString(String originalString,
            int originNamesId, int localNamesId) {
        String[] origNames = getResources().getStringArray(originNamesId);
        String[] localNames = getResources().getStringArray(localNamesId);
        for (int i = 0; i < origNames.length; i++) {
            if (origNames[i].equalsIgnoreCase(originalString)) {
                return getString(getResources().getIdentifier(localNames[i], "string", "android"));
            }
        }
        return originalString;
    }

    private void showProgressDailog(String msgStr) {
        MyProgressDialog progress = MyProgressDialog
                .newInstance(mSubscription, msgStr);
        progress.show(getFragmentManager(), "progress");
    }

    private void cancelProgressDialog() {
        if (mIsForceStop != true) {
            MyProgressDialog progress = (MyProgressDialog) getFragmentManager()
                    .findFragmentByTag("progress");
            if (progress != null) {
                progress.dismiss();
                progress = null;
            }
        }
    }

    private class CdmaConfig {
        private int mNextReqEvent = EVENT_INVALID;
        private boolean mIsQcRilHookReady = false;

        private QcRilHook mQcRilHook;
        private Context mContext;

        public CdmaConfig(Context context) {
            mContext = context;
            mQcRilHook = new QcRilHook(mContext, mQcrilHookCb);
        }

        private QcRilHookCallback mQcrilHookCb = new QcRilHookCallback() {
            public void onQcRilHookReady() {
                mIsQcRilHookReady = true;
            }
        };

        /**
         * Called when the "Avoid the current network" option is selected
         */
        private void selectAvoidCurNetwork() {
            new AlertDialog.Builder(ManualNetworkActivity.this)
                    .setTitle(R.string.pref_cdma_choose_title)
                    .setMessage(R.string.confirm_avoid)
                    .setNegativeButton(R.string.cancel_btn, null)
                    .setPositiveButton(R.string.ok_btn,
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    mHandler.sendMessage(mHandler
                                            .obtainMessage(EVENT_AVOID_CURRENT_NETWORK));
                                }
                            }).show();
        }

        /**
         * Called when the "Clear the avoidance network list" option is selected
         */
        private void clearNetworkAvoidList() {
            new AlertDialog.Builder(ManualNetworkActivity.this)
                    .setTitle(R.string.pref_cdma_choose_title)
                    .setMessage(R.string.confirm_clear)
                    .setNegativeButton(R.string.cancel_btn, null)
                    .setPositiveButton(R.string.ok_btn,
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    mHandler.sendMessage(mHandler
                                            .obtainMessage(EVENT_CLEAR_AVOIDANCE_LIST));
                                }
                            }).show();
        }

        /**
         * Called when the "View the avoidance network list" option is selected
         */
        private void viewNetworkAvoidList() {
            mHandler.sendMessage(mHandler.obtainMessage(EVENT_GET_AVOIDANCE_LIST));
        }

        /**
         * Process a Data Disabled event
         *
         * @param msg : Message
         */
        private void handleDataDisabled(Message msg) {
            if (getNextReqEvent() != EVENT_INVALID) {
                Log.d(TAG, "Data disconnected. Ready to send the next request.");
                mHandler.sendMessage(mHandler.obtainMessage(getNextReqEvent()));

                setNextReqEvent(EVENT_INVALID);
            }
        }

        /**
         * Process a Avoid Current Network Event
         *
         * @param msg : Message
         */
        private void handleAvoidCurNwk(Message msg) {
            Log.d(TAG, "Sending RIL OEM HOOK Cdma Avoid Current Network message.");

            int resStrId = 0;
            boolean result = false;
            if (!mIsQcRilHookReady) {
                // return if the QcRilHook isn't ready
                return;
            }

            result = mQcRilHook.qcRilCdmaAvoidCurNwk();
            if (result == false) {
                Log.e(TAG, "qcRilCdmaAvoidCurNwk command failed.");
                resStrId = R.string.network_select_failure_info;
            } else {
                resStrId = R.string.network_select_success_info;
            }
            String manualNetwork = getNetworkString(
                    MSimTelephonyManager.getDefault().getNetworkOperatorName(mSubscription),
                    R.array.original_carrier_names, R.array.local_carrier_names);
            String msgStr = getString(resStrId, manualNetwork);
            showNetworkSelectResultDialog(false, msgStr);

            mHandler.sendMessage(mHandler
                    .obtainMessage(EVENT_AVOID_CURRENT_NETWORK_COMPLETED));
        }

        /**
         * Process a Clear Avoidance Network List Event
         *
         * @param msg : Message
         */
        private void handleClearAvoidanceList(Message msg) {
            Log.d(TAG, "Sending RIL OEM HOOK Cdma Clear Avoidance List message.");

            boolean result = false;
            if (!mIsQcRilHookReady) {
                // return if the QcRilHook isn't ready
                return;
            }
            result = mQcRilHook.qcRilCdmaClearAvoidanceList();
            if (result == false) {
                Log.e(TAG, "qcRilCdmaClearAvoidanceList command failed.");
                Toast.makeText(mContext, R.string.clear_nwk_list_failed,
                        Toast.LENGTH_SHORT).show();
            } else {
                Toast.makeText(mContext, R.string.clear_nwk_list_succeeded,
                        Toast.LENGTH_SHORT).show();
            }

            mHandler.sendMessage(mHandler
                    .obtainMessage(EVENT_CLEAR_AVOIDANCE_LIST_COMPLETED));
        }

        /**
         * Process a Get Avoidance Network List Event
         *
         * @param msg : Message
         */
        private void handleGetAvoidanceList(Message msg) {
            Log.d(TAG, "Sending RIL OEM HOOK Cdma Get Avoidance List message.");

            byte[] result = null;
            if (!mIsQcRilHookReady) {
                // return if the QcRilHook isn't ready
                return;
            }
            result = mQcRilHook.qcRilCdmaGetAvoidanceList();
            if (result == null) {
                Log.e(TAG, "qcRilCdmaGetAvoidanceList command failed.");
            }

            mHandler.sendMessage(mHandler.obtainMessage(
                    EVENT_GET_AVOIDANCE_LIST_COMPLETED, (Object) result));
        }

        /**
         * Set which request event to be sent after the data is disabled.
         *
         * @param event : int
         */
        synchronized private void setNextReqEvent(int event) {
            mNextReqEvent = event;
        }

        /**
         * Get which request event to be sent after the data is disabled.
         *
         * @return int
         */
        synchronized private int getNextReqEvent() {
            return mNextReqEvent;
        }
    }
}
