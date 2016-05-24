/**
 * Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

package com.qualcomm.roamingsettings;

import android.app.ActionBar.LayoutParams;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.app.Fragment;
import android.app.FragmentTransaction;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.database.ContentObserver;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.SystemProperties;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.telephony.MSimTelephonyManager;
import android.telephony.ServiceState;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.webkit.WebView.FindListener;
import android.widget.Button;
import android.widget.TextView;

import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.MSimConstants;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.PhoneProxy;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.cdma.CDMAPhone;
import com.codeaurora.telephony.msim.MSimPhoneFactory;
import com.codeaurora.telephony.msim.MSimProxyManager;
import com.codeaurora.telephony.msim.MSimUiccController;
import com.codeaurora.telephony.msim.Subscription.SubscriptionStatus;
import com.codeaurora.telephony.msim.SubscriptionManager;
import com.codeaurora.telephony.msim.SubscriptionData;
import com.codeaurora.telephony.msim.CardSubscriptionManager;
import com.codeaurora.telephony.msim.Subscription;

public class RoamingSettingsFragmet extends Fragment implements OnClickListener {
    private static final String TAG = "RoamingSettingsApp";
    private static final boolean DEBUG = true;

    private static final int EVENT_SET_SUBSCRIPTION_DONE = 0;
    private static final int EVENT_AUTO_SELECT_DONE = 1;

    // The keys for arguments of dialog.
    private static final String SUB = MSimConstants.SUBSCRIPTION_KEY;
    private final int MAX_SUBSCRIPTIONS = SubscriptionManager.NUM_SUBSCRIPTIONS;
    private static final String TITLE_RES_ID = "title_res_id";
    private static final String MSG_RES_ID = "msg_res_id";

    // The invalid sub index for we will use it if deactivate the card.
    private static final int SUBSCRIPTION_INDEX_INVALID = 99999;

    // The operator numeric of China and Macao.
    private static final String CHINA_MCC = "460"; // China
    private static final String MACAO_MCC = "455"; // Macao

    private static final String CHINA_TELECOM = "46003";
    private static final String UNKNOWN_DATA = "00000";

    private View mViewRoot = null;
    private Button mActiveCard;
    private Button mDataRoaming;
    private Button mNetworkInfo;
    private Button mManualSetup;
    private Button mRoamingHotline;
    private TextView mIntroInfo;

    private SharedPreferences mUserInfo;

    private Drawable mOffChecked;
    private Drawable mOnChecked;

    private boolean mIsActiveCardEnable = false;
    private boolean mIsDataRoamingEnable = false;
    private boolean mIsManualSetupEnable = false;

    private Phone mPhone = null;
    private SubscriptionManager mSubMgr = null;
    private CardSubscriptionManager mCardSubMgr;
    private SubscriptionData[] mCardSubscrInfo;
    private SubscriptionData mCurrentSelSub;
    private SubscriptionData mUserSelSub;

    private int mSubscription;
    private int mNetworkMode;
    private int mPhoneCount;

    private boolean mIsActiveStore[] = new boolean[MAX_SUBSCRIPTIONS];
    private boolean mIsForceStop;
    private boolean mIsMultiSimEnabled;

    private String mNumericPLMN = "null";
    private String mNumericSPN = "null";

    public class SimStateChangedReceiver extends BroadcastReceiver {
        private static final String TAG = "RoamingSettingsApp";

        private String mCurrentNumber = "";

        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            int subscription = 0;

            if (TelephonyIntents.ACTION_SIM_STATE_CHANGED.equals(action)
                    || TelephonyIntents.ACTION_SERVICE_STATE_CHANGED.equals(action)) {
                subscription = intent.getIntExtra(MSimConstants.SUBSCRIPTION_KEY, 0);
                mCurrentNumber = MSimTelephonyManager.getDefault().getNetworkOperator(subscription);

                if (subscription != mSubscription) {
                    return;
                }
                if (!mCurrentNumber.equals(mNumericPLMN) && !mCurrentNumber.equals(UNKNOWN_DATA)) {
                    updateRoamingSetttingsView();
                }
            }
        }

    }
    private SimStateChangedReceiver mReceiver = null;

    private class RoamingDataObserver extends ContentObserver {
        public RoamingDataObserver(Handler handler) {
            super(handler);
        }

        @Override
        public void onChange(boolean selfChange) {
            boolean enable = multiSimGetDataRoaming(mSubscription);
            setDataRoamingChecked(enable);
        }

        public void startObserving() {
            final ContentResolver cr = getActivity().getContentResolver();
            cr.unregisterContentObserver(this);
            cr.registerContentObserver(
                    Settings.Global.CONTENT_URI, true, this);
        }
    }

    // We used this handle to deal with the msg of set/get network type and
    // enable/disable icc card.
    public Handler mHandler = new Handler() {

        @Override
        public void handleMessage(Message msg) {
            if (DEBUG)
                Log.i(TAG, "handle the message, what=" + msg.what);

            AsyncResult ar = (AsyncResult) msg.obj;
            switch (msg.what) {
            case EVENT_AUTO_SELECT_DONE:
                if (mIsForceStop != true) {
                    MyProgressDialog progress = (MyProgressDialog) getFragmentManager()
                            .findFragmentByTag("progress");
                    if (progress != null) {
                        progress.dismiss();
                    }
                }

                int msgId;
                if (ar.exception != null) {
                    Log.v(TAG, "automatic network selection: failed!");
                    setManualSetupChecked(true);
                    msgId = R.string.network_auto_failure_info;
                } else {
                    Log.v(TAG, "automatic network selection: succeeded!");
                    setManualSetupChecked(false);
                    mUserInfo.edit().putBoolean("manual_gsm", false).commit();
                    msgId = R.string.network_auto_success_info;
                }

                showConnectResultDialog(getString(msgId));
                break;

            case EVENT_SET_SUBSCRIPTION_DONE:
                Log.d(TAG, "EVENT_SET_SUBSCRIPTION_DONE");
                mSubMgr.unRegisterForSetSubscriptionCompleted(mHandler);
                for (int i = 0; i < mPhoneCount; i++) {
                    mIsActiveStore[i] = mSubMgr.isSubActive(i);
                }
                updateRoamingSetttingsView();

                if (mIsForceStop != true) {
                    MyProgressDialog progress = (MyProgressDialog) getFragmentManager()
                            .findFragmentByTag("progress");
                    if (progress != null) {
                        progress.dismiss();
                    }
                }
                showConnectResultDialog(getString(R.string.operation_success));
                break;
            }
        }
    };

    @Override
    public void onCreate(Bundle bundle) {
        super.onCreate(bundle);

        mOffChecked = getResources().getDrawable(R.drawable.btn_check_off_roaming);
        mOnChecked = getResources().getDrawable(R.drawable.btn_check_on_roaming);

        mSubscription = MSimConstants.DEFAULT_SUBSCRIPTION;
        mIsMultiSimEnabled = MSimTelephonyManager.getDefault().isMultiSimEnabled();
        if (mIsMultiSimEnabled) {
            mPhone = MSimPhoneFactory.getPhone(mSubscription);
            mSubMgr = SubscriptionManager.getInstance();
            mCardSubMgr = CardSubscriptionManager.getInstance();

            mCardSubscrInfo = new SubscriptionData[MAX_SUBSCRIPTIONS];
            mUserSelSub = new SubscriptionData(MAX_SUBSCRIPTIONS);
            for (int i = 0; i < MAX_SUBSCRIPTIONS; i++) {
                mCardSubscrInfo[i] = mCardSubMgr.getCardSubscriptions(i);
                mIsActiveStore[i] = mSubMgr.isSubActive(i);
            }
        } else {
            mPhone = PhoneFactory.getDefaultPhone();
        }

        mUserInfo = getActivity().getSharedPreferences("user_info", 0);
        mIsManualSetupEnable = mUserInfo.getBoolean("manual_gsm", false);

        RoamingDataObserver roamingObserver = new RoamingDataObserver(new Handler());
        roamingObserver.startObserving();

        // register the receiver.
        mReceiver = new SimStateChangedReceiver();
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        intentFilter.addAction(TelephonyIntents.ACTION_SERVICE_STATE_CHANGED);
        getActivity().registerReceiver(mReceiver, intentFilter);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        mViewRoot = inflater.inflate(R.layout.roaming_settings, null);
        mActiveCard = (Button) mViewRoot.findViewById(R.id.bt_active_card);
        mActiveCard.setOnClickListener(this);
        mDataRoaming = (Button) mViewRoot.findViewById(R.id.bt_data_roaming);
        mDataRoaming.setOnClickListener(this);
        mNetworkInfo = (Button) mViewRoot.findViewById(R.id.bt_network_info);
        mNetworkInfo.setOnClickListener(this);
        mManualSetup = (Button) mViewRoot.findViewById(R.id.bt_manual_network_select);
        mManualSetup.setOnClickListener(this);
        mRoamingHotline = (Button) mViewRoot.findViewById(R.id.bt_hotline);
        mRoamingHotline.setOnClickListener(this);
        mIntroInfo = (TextView) mViewRoot.findViewById(R.id.tv_intro_info);

        // Update view
        updateRoamingSetttingsView();

        // Hide hot line and info view when single card
        if (!mIsMultiSimEnabled) {
            mActiveCard.setVisibility(View.GONE);
            mIntroInfo.setVisibility(View.GONE);
        }

        return mViewRoot;
    }

    @Override
    public void onDestroyView() {
        if (mReceiver != null) {
            getActivity().unregisterReceiver(mReceiver);
        }
        super.onDestroyView();
    }

    @Override
    public void onResume() {
        super.onResume();

        mIsForceStop = false;
        if (mSubscription > MSimConstants.SUB1) {
            boolean enable = mUserInfo.getBoolean("manual_gsm", false);
            setManualSetupChecked(enable);
        }
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

    public void onSubscriptionChanged(int sub) {
        mSubscription = sub;
        if (mViewRoot == null) {
            Log.v(TAG, "the first startup");
        } else {
            if (mSubscription == MSimConstants.SUB1) {
                mDataRoaming.setVisibility(View.VISIBLE);
                mManualSetup.setCompoundDrawablesWithIntrinsicBounds(
                        null, null, null, null);
                mRoamingHotline.setVisibility(View.VISIBLE);
                mIntroInfo.setVisibility(View.VISIBLE);
            } else {
                setManualSetupChecked(mIsManualSetupEnable);
                mDataRoaming.setVisibility(View.GONE);
                mRoamingHotline.setVisibility(View.GONE);
                mIntroInfo.setVisibility(View.GONE);
            }

            // if the phone count > 1, we need get the phone by subscription.
            if (mIsMultiSimEnabled) {
                mPhone = MSimPhoneFactory.getPhone(mSubscription);
            } else {
                mPhone = PhoneFactory.getDefaultPhone();
            }

            updateRoamingSetttingsView();
        }
    }

    private void updateRoamingSetttingsView() {
        boolean isActive = false;
        boolean hasIccCard = false;

        if (mIsMultiSimEnabled) {
            mCurrentSelSub = new SubscriptionData(MAX_SUBSCRIPTIONS);
            for (int i = 0; i < mPhoneCount; i++) {
                Subscription sub = mSubMgr.getCurrentSubscription(i);
                mCurrentSelSub.subscription[i].copyFrom(sub);
            }
            if (mCurrentSelSub != null) {
                mUserSelSub.copyFrom(mCurrentSelSub);
            }
        }

        hasIccCard = mPhone.getIccCard().hasIccCard();
        if (hasIccCard) {
            if (!mIsMultiSimEnabled) {
                isActive = true;
            } else if (mSubMgr != null) {
                isActive = mSubMgr.isSubActive(mSubscription);
            }
        }

        if (mSubscription == MSimConstants.SUB1) {
            boolean enable = false;
            boolean isRoaming = false;
            boolean isChinaTelecom = false;
            boolean isDomestic = false;

            if (hasIccCard) {
                enable = hasIccCard;
                // Get China Telecom code and CN MO
                mNumericPLMN = MSimTelephonyManager.getDefault().getNetworkOperator(mSubscription);
                mNumericSPN = MSimTelephonyManager.getDefault().getSimOperator(mSubscription);
                if (mNumericPLMN != null && mNumericSPN != null) {
                    isChinaTelecom = mNumericSPN.startsWith(CHINA_TELECOM);
                    if (mNumericPLMN.startsWith(CHINA_MCC) || mNumericPLMN.startsWith(MACAO_MCC)) {
                        isDomestic = true;
                    }
                }

                if (isActive) {
                    enable = isChinaTelecom;
                    // Get roaming state
                    isRoaming = multiSimGetDataRoaming(mSubscription);
                } else {
                    enable = isActive;
                }
            }

            mActiveCard.setEnabled(hasIccCard);
            mDataRoaming.setEnabled(enable);
            mNetworkInfo.setEnabled(enable);
            mManualSetup.setEnabled(enable && !isDomestic);
            mIntroInfo.setEnabled(enable);
            mRoamingHotline.setEnabled(enable);
            setDataRoamingChecked(enable && isRoaming);
        } else {
            mActiveCard.setEnabled(hasIccCard);
            mNetworkInfo.setEnabled(isActive);
            mManualSetup.setEnabled(isActive);
        }

        if (mIsMultiSimEnabled && mSubMgr != null) {
            setActiveCardChecked(mSubMgr.isSubActive(mSubscription)
                    && hasIccCard);
        }

    }

    private boolean onActiveCardCheck() {
        mIsActiveCardEnable = mIsActiveCardEnable ? false : true;
        Drawable drawable = mIsActiveCardEnable ? mOnChecked : mOffChecked;
        mActiveCard.setCompoundDrawablesWithIntrinsicBounds(
                null, null, drawable, null);
        return mIsActiveCardEnable;
    }

    private boolean onDataRoamingCheck() {
        mIsDataRoamingEnable = mIsDataRoamingEnable ? false : true;
        Drawable drawable = mIsDataRoamingEnable ? mOnChecked : mOffChecked;
        mDataRoaming.setCompoundDrawablesWithIntrinsicBounds(
                null, null, drawable, null);
        return mIsDataRoamingEnable;
    }

    private boolean onManualSetupCheck() {
        mIsManualSetupEnable = mIsManualSetupEnable ? false : true;
        Drawable drawable = mIsManualSetupEnable ? mOnChecked : mOffChecked;
        mManualSetup.setCompoundDrawablesWithIntrinsicBounds(
                null, null, drawable, null);
        return mIsManualSetupEnable;
    }

    private void setActiveCardChecked(boolean checked) {
        mIsActiveCardEnable = checked;
        Drawable drawable = mIsActiveCardEnable ? mOnChecked : mOffChecked;
        mActiveCard.setCompoundDrawablesWithIntrinsicBounds(
                null, null, drawable, null);
    }

    private void setDataRoamingChecked(boolean checked) {
        mIsDataRoamingEnable = checked;
        Drawable drawable = mIsDataRoamingEnable ? mOnChecked : mOffChecked;
        mDataRoaming.setCompoundDrawablesWithIntrinsicBounds(
                null, null, drawable, null);
    }

    private void setManualSetupChecked(boolean checked) {
        mIsManualSetupEnable = checked;
        Drawable drawable = mIsManualSetupEnable ? mOnChecked : mOffChecked;
        mManualSetup.setCompoundDrawablesWithIntrinsicBounds(
                null, null, drawable, null);
    }

    @Override
    public void onClick(View v) {
        Intent intent;
        switch (v.getId()) {
        case R.id.bt_active_card:
            final boolean activeCardChecked = onActiveCardCheck();
            if (mIsMultiSimEnabled) {
                // We will always prompt the dialog to alert the user which
                // action will be done.
                AttentionDialog.OnPositiveClickListener plistener =
                        new AttentionDialog.OnPositiveClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog) {
                        // Set the subcription's state
                        setSubState(activeCardChecked);

                        // Show the progress dialog
                        int msgResId = activeCardChecked ? R.string.progress_msg_enable_card
                                : R.string.progress_msg_disable_card;
                        MyProgressDialog progress = MyProgressDialog
                                .newInstance(mSubscription, getString(msgResId));
                        progress.show(getFragmentManager(), "progress");

                        dialog.dismiss();
                    }
                };
                AttentionDialog.OnNegativeClickListener nlistener =
                        new AttentionDialog.OnNegativeClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog) {
                        dialog.dismiss();
                        // reset the value.
                        setActiveCardChecked(!activeCardChecked);
                    }
                };
                int msgResId = activeCardChecked ? R.string.alert_msg_enable_card
                        : R.string.alert_msg_disable_card;
                AttentionDialog dialog = AttentionDialog.newInstance(
                        mSubscription, R.string.slot_name, getString(msgResId), true, true,
                        plistener, nlistener);
                dialog.show(getFragmentManager(), "alert");
            }
            break;

        case R.id.bt_data_roaming:
            final boolean dataRoamingChecked = onDataRoamingCheck();
            if (dataRoamingChecked) {
                // If the user want to enable data roaming, we need prompt the
                // dialog to alert the user.
                AttentionDialog.OnPositiveClickListener plistener =
                        new AttentionDialog.OnPositiveClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog) {
                        multiSimSetDataRoaming(dataRoamingChecked, mSubscription);
                        dialog.dismiss();
                    }
                };
                AttentionDialog.OnNegativeClickListener nlistener =
                        new AttentionDialog.OnNegativeClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog) {
                        // If user cancel the action, we need reset the
                        // preference value.
                        setDataRoamingChecked(!dataRoamingChecked);
                        dialog.dismiss();
                    }
                };
                AttentionDialog dialog = AttentionDialog.newInstance(
                        mSubscription, R.string.alert_data_roaming_title,
                        getString(R.string.alert_data_roaming_msg), true, true,
                        plistener, nlistener);
                dialog.show(getFragmentManager(), "data_roaming");
            } else {
                multiSimSetDataRoaming(dataRoamingChecked, mSubscription);
            }
            break;

        case R.id.bt_network_info:
            intent = new Intent(getActivity(), CurrentNetworkStatus.class);
            intent.putExtra(MSimConstants.SUBSCRIPTION_KEY, mSubscription);
            startActivity(intent);
            break;

        case R.id.bt_manual_network_select:
            if (mSubscription == MSimConstants.SUB1) {
                intent = new Intent(getActivity(), ManualNetworkActivity.class);
                intent.putExtra(MSimConstants.SUBSCRIPTION_KEY, mSubscription);
                startActivity(intent);
            } else {
                final boolean manualSetupChecked = onManualSetupCheck();
                if (manualSetupChecked) {
                    intent = new Intent(getActivity(), ManualNetworkActivity.class);
                    intent.putExtra(MSimConstants.SUBSCRIPTION_KEY, mSubscription);
                    startActivity(intent);
                } else {
                    selectNetworkAutomatic();
                }
            }
            break;

        case R.id.bt_hotline:
            intent = new Intent();
            intent.setAction(Intent.ACTION_CALL);
            intent.setData(Uri.parse("tel:" + "+8618918910000"));
            startActivity(intent);
            break;
        }
    }

    private boolean isPhoneInCall() {
        boolean phoneInCall = false;
        if (mIsMultiSimEnabled) {
            for (int i = 0; i < mPhoneCount; i++) {
                if (MSimTelephonyManager.getDefault().getCallState(i)
                        != TelephonyManager.CALL_STATE_IDLE) {
                    phoneInCall = true;
                    break;
                }
            }
        } else {
            if (TelephonyManager.getDefault().getCallState()
                    != TelephonyManager.CALL_STATE_IDLE) {
                phoneInCall = true;
            }
        }
        return phoneInCall;
    }

    /**
     * Set the subscription's state.
     *
     * @param enabled
     *            if enable the sub, set it as true.
     */
    private void setSubState(boolean enabled) {
        if (mSubMgr == null)
            return;
        if (DEBUG)
            Log.i(TAG, "setSubState, new state:" + enabled);

        mIsActiveStore[mSubscription] = enabled;
        if (isPhoneInCall()) {
            // User is not allowed to activate or deactivate the subscriptions
            // while in a voice call.
            Log.v(TAG, "on Telephone Call");
        } else {
            for (int i = 0; i < MAX_SUBSCRIPTIONS; i++) {
                if (mIsActiveStore[i] == false) {
                    if (mCurrentSelSub.subscription[i].subStatus ==
                            Subscription.SubscriptionStatus.SUB_ACTIVATED) {
                        Log.d(TAG, "setSubscription: Sub " + i + " not selected. Setting 99999");
                        mUserSelSub.subscription[i].slotId = SUBSCRIPTION_INDEX_INVALID;
                        mUserSelSub.subscription[i].m3gppIndex = SUBSCRIPTION_INDEX_INVALID;
                        mUserSelSub.subscription[i].m3gpp2Index = SUBSCRIPTION_INDEX_INVALID;
                        mUserSelSub.subscription[i].subId = i;
                        mUserSelSub.subscription[i].subStatus = Subscription.
                                SubscriptionStatus.SUB_DEACTIVATE;
                    }
                } else {
                    // Key is the string :  "slot<SlotId> index<IndexId>"
                    // Split the string into two and get the SlotId and IndexId.
                    if (mCardSubscrInfo[i] == null) {
                        Log.d(TAG, "setSubscription: mCardSubscrInfo is not in sync "
                                + "with SubscriptionManager");
                        mUserSelSub.subscription[i].slotId = SUBSCRIPTION_INDEX_INVALID;
                        mUserSelSub.subscription[i].m3gppIndex = SUBSCRIPTION_INDEX_INVALID;
                        mUserSelSub.subscription[i].m3gpp2Index = SUBSCRIPTION_INDEX_INVALID;
                        mUserSelSub.subscription[i].subId = i;
                        mUserSelSub.subscription[i].subStatus = Subscription.
                                SubscriptionStatus.SUB_DEACTIVATE;

                        if (mCurrentSelSub.subscription[i].subStatus ==
                                Subscription.SubscriptionStatus.SUB_ACTIVATED) {
                        }
                        continue;
                    }

                    // Compate the user selected subscriptio with the current subscriptions.
                    // If they are not matching, mark it to activate.
                    int subIndex = mCardSubscrInfo[i].getLength() - 1;
                    mUserSelSub.subscription[i].copyFrom(mCardSubscrInfo[i].
                            subscription[subIndex]);
                    mUserSelSub.subscription[i].subId = i;
                    if (mCurrentSelSub != null) {
                        // subStatus used to store the activation status as the mCardSubscrInfo
                        // is not keeping track of the activation status.
                        Subscription.SubscriptionStatus subStatus =
                                mCurrentSelSub.subscription[i].subStatus;
                        mUserSelSub.subscription[i].subStatus = subStatus;
                        if ((subStatus != Subscription.SubscriptionStatus.SUB_ACTIVATED) ||
                            (!mUserSelSub.subscription[i].equals(mCurrentSelSub.subscription[i]))) {
                            // User selected a new subscription.  Need to activate this.
                            mUserSelSub.subscription[i].subStatus = Subscription.
                            SubscriptionStatus.SUB_ACTIVATE;
                        }

                        if (mCurrentSelSub.subscription[i].subStatus == Subscription.
                                 SubscriptionStatus.SUB_ACTIVATED
                                 && mUserSelSub.subscription[i].subStatus == Subscription.
                                 SubscriptionStatus.SUB_ACTIVATE) {
                        }
                    } else {
                        mUserSelSub.subscription[i].subStatus = Subscription.
                                SubscriptionStatus.SUB_ACTIVATE;
                    }
                }
            }
        }

        boolean result = mSubMgr.setSubscription(mUserSelSub);
        if (result) {
            mSubMgr.registerForSetSubscriptionCompleted(mHandler,
                    EVENT_SET_SUBSCRIPTION_DONE, null);
        } else {
            showConnectResultDialog(getString(R.string.operation_failure));
        }
    }

    // Get Data roaming flag, from DB, as per SUB.
    private boolean multiSimGetDataRoaming(int sub) {
        boolean enabled;
        if (mIsMultiSimEnabled) {
            enabled = Settings.Global.getInt(mPhone.getContext().getContentResolver(),
                    Settings.Global.DATA_ROAMING + sub, 0) != 0;
            Log.v(TAG, "Get Data Roaming for SUB-" + sub + " is " + enabled);
        } else {
            enabled = Settings.Global.getInt(mPhone.getContext().getContentResolver(),
                    Settings.Global.DATA_ROAMING, 0) != 0;
        }
        return enabled;
    }

    // Set Data roaming flag, in DB, as per SUB.
    private void multiSimSetDataRoaming(boolean enabled, int sub) {
        // as per SUB, set the individual flag
        if (mIsMultiSimEnabled) {
            Settings.Global.putInt(mPhone.getContext().getContentResolver(),
                    Settings.Global.DATA_ROAMING + sub, enabled ? 1 : 0);
            Log.v(TAG, "Set Data Roaming for SUB-" + sub + " is " + enabled
                    + " DDS: " + android.telephony.MSimTelephonyManager.
                    getDefault().getPreferredDataSubscription());

            // If current DDS is this SUB, update the Global flag also
            if (sub == MSimTelephonyManager.getDefault().getPreferredDataSubscription()) {
                mPhone.setDataRoamingEnabled(enabled);
                Log.v(TAG, "Set Data Roaming for DDS-" + sub + " is " + enabled);
            }
        } else {
            Settings.Global.putInt(mPhone.getContext().getContentResolver(),
                    Settings.Global.DATA_ROAMING, enabled ? 1 : 0);
        }
    }

    private void selectNetworkAutomatic() {
        MyProgressDialog progress = MyProgressDialog
                .newInstance(mSubscription, getString(R.string.register_automatically));
        progress.show(getFragmentManager(), "progress");

        Message msg = mHandler.obtainMessage(EVENT_AUTO_SELECT_DONE);
        mPhone.setNetworkSelectionModeAutomatic(msg);
    }

    private void showConnectResultDialog(String msgStr) {
        if (mIsForceStop != true) {
            AttentionDialog.OnPositiveClickListener listener =
                    new AttentionDialog.OnPositiveClickListener() {
                @Override
                public void onClick(DialogInterface dialog) {
                    dialog.dismiss();
                }
            };

            AttentionDialog alert = AttentionDialog.newInstance(
                    mSubscription, R.string.slot_name,
                    msgStr, true, false, listener, null);
            alert.show(getFragmentManager(), "alert");
        }
    }

    /**
     * Request user confirmation before settings
     */
    public static class AttentionDialog extends DialogFragment {
        private OnPositiveClickListener mPositiveListener;
        private OnNegativeClickListener mNegativeListener;
        private boolean mShowTitle;
        private boolean mShowCancelButton;

        public static AttentionDialog newInstance(int sub, int titleResId,
                String msgStr, boolean showTitle, boolean showCancelButton,
                OnPositiveClickListener pLis, OnNegativeClickListener nLis) {
            AttentionDialog dialog = new AttentionDialog();
            dialog.mPositiveListener = pLis;
            dialog.mNegativeListener = nLis;
            dialog.mShowTitle = showTitle;
            dialog.mShowCancelButton = showCancelButton;

            final Bundle args = new Bundle();
            args.putInt(SUB, sub);
            args.putInt(TITLE_RES_ID, titleResId);
            args.putString(MSG_RES_ID, msgStr);
            dialog.setArguments(args);

            return dialog;
        }

        @Override
        public Dialog onCreateDialog(Bundle bundle) {
            final int titleResId = getArguments().getInt(TITLE_RES_ID);
            final String msgRes = getArguments().getString(MSG_RES_ID);
            final int sub = getArguments().getInt(SUB);

            String slotNumber = getResources().getStringArray(
                    R.array.slot_number)[sub];
            AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
            builder.setMessage(msgRes)
                    .setPositiveButton(android.R.string.ok,
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog,
                                        int which) {
                                    if (mPositiveListener != null) {
                                        mPositiveListener.onClick(dialog);
                                    }
                                }
                            }).setCancelable(true);
            if (mShowTitle) {
                builder.setIcon(android.R.drawable.ic_dialog_alert)
                .setTitle(getString(titleResId, slotNumber));
            }
            if (mShowCancelButton) {
                builder.setNegativeButton(android.R.string.cancel,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                if (mNegativeListener != null) {
                                    mNegativeListener.onClick(dialog);
                                }
                            }
                        });
            }

            return builder.create();
        }

        @Override
        public void onCancel(DialogInterface dialog) {
            super.onCancel(dialog);
            if (mNegativeListener != null) {
                mNegativeListener.onClick(dialog);
            }
        }

        static abstract class OnPositiveClickListener {
            public abstract void onClick(DialogInterface dialog);
        }

        static abstract class OnNegativeClickListener {
            public abstract void onClick(DialogInterface dialog);
        }
    }

    /**
     * This progress dialog will be shown if user want to active or deactivate
     * the icc card.
     */
    public static class MyProgressDialog extends DialogFragment {

        public static MyProgressDialog newInstance(int sub, String msgStr) {
            MyProgressDialog dialog = new MyProgressDialog();

            final Bundle args = new Bundle();
            args.putInt(SUB, sub);
            args.putString(MSG_RES_ID, msgStr);
            dialog.setArguments(args);

            return dialog;
        }

        @Override
        public Dialog onCreateDialog(Bundle bundle) {
            final String msgStr = getArguments().getString(MSG_RES_ID);
            final int sub = getArguments().getInt(SUB);

            final ProgressDialog progress = new ProgressDialog(getActivity(),
                    ProgressDialog.STYLE_SPINNER);
            String slotNumber = getResources().getStringArray(
                    R.array.slot_number)[sub];
            progress.setTitle(getString(R.string.slot_name, slotNumber));
            progress.setMessage(msgStr);
            progress.setCancelable(false);
            progress.setCanceledOnTouchOutside(false);
            return progress;
        }

    }

}
