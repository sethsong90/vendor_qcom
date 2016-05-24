/**
 * Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

package com.qualcomm.roamingsettings;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Resources;
import android.net.ConnectivityManager;
import android.os.Bundle;
import android.os.SystemProperties;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.provider.Settings;
import android.telephony.MSimTelephonyManager;
import android.telephony.PhoneStateListener;
import android.telephony.ServiceState;
import android.telephony.SignalStrength;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;

import com.android.internal.telephony.MSimConstants;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.TelephonyProperties;
import com.codeaurora.telephony.msim.MSimPhoneFactory;

/**
 * Display the following information # Battery Strength : TODO # Uptime # Awake
 * Time # XMPP/buzz/tickle status : TODO
 */
public class CurrentNetworkStatus extends PreferenceActivity {
    private static final String TAG = "CurrentSimStatus";
    private static final boolean DEBUG = true;
    private static final String BUNDLE_KEY_SUBSCRIPTION =
            MSimConstants.SUBSCRIPTION_KEY;
    private static final String SMSC = "smsc";
    private static final String SUB = "sub";
    private static final String COMMAND_GET_SMSC = "com.android.phone.smsc.smscservice.get";
    private static final String NOTIFY_SMSC_UPDATE =
        "com.android.phone.smsc.smscservice.notify.update";

    private static final String KEY_OPERATOR_NAME = "operator_name";
    private static final String KEY_MCC_MNC = "mcc_mnc";
    private static final String KEY_SID_NID = "sid_nid";
    private static final String KEY_NETWORK_TYPE = "network_type";
    private static final String KEY_SIGNAL_STRENGTH = "signal_strength";
    private static final String KEY_PRL_VERSION = "prl_version";
    private static final String KEY_SMS_SERVICE_CENTER = "sms_service_center";
    private static final String KEY_MIN_NUMBER = "min_number";
    private static final String KEY_ESN_NUMBER = "esn_number";
    private static final String KEY_ICC_ID = "icc_id";
    private static final String KEY_SERVICE_STATE = "service_state";
    private static final String KEY_ROAMING_STATE = "roaming_state";
    private static final String KEY_DATA_STATE = "data_state";

    private static final String[] RELATED_ENTRIES = {
            KEY_SERVICE_STATE,
            KEY_OPERATOR_NAME,
            KEY_ROAMING_STATE,
            KEY_ICC_ID,
            KEY_PRL_VERSION,
            KEY_MIN_NUMBER,
            KEY_ESN_NUMBER,
            KEY_SIGNAL_STRENGTH,
            KEY_MCC_MNC,
            KEY_SID_NID,
            KEY_DATA_STATE,
            KEY_NETWORK_TYPE,
            KEY_SMS_SERVICE_CENTER
    };

    private TelephonyManager mTMgr = null;
    private MSimTelephonyManager mMSimTMgr = null;
    private PhoneStateListener mPhoneStateListener = null;
    private Resources mRes = null;

    private static String mUnknown = null;

    private SignalStrength mSignalStrength = null;
    private ServiceState mServiceState = null;
    private BroadcastReceiver mReceiver = null;
    private Phone mPhone = null;

    private int mSubscription;
    private boolean isCDMA = false;
    private int mDataState = TelephonyManager.DATA_DISCONNECTED;

    private boolean mIsMultiSimEnabled;

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        mSubscription = getIntent().getIntExtra(BUNDLE_KEY_SUBSCRIPTION,
                MSimConstants.INVALID_SUBSCRIPTION);
        if (mSubscription == MSimConstants.INVALID_SUBSCRIPTION) {
            finish();
        }
        if (DEBUG) {
            Log.d(TAG, "mSubscription:" + mSubscription);
        }

        addPreferencesFromResource(R.xml.current_sim_status);

        mIsMultiSimEnabled = MSimTelephonyManager.getDefault().isMultiSimEnabled();
        if (mIsMultiSimEnabled) {
            mMSimTMgr = (MSimTelephonyManager) getSystemService(Context.MSIM_TELEPHONY_SERVICE);
            mPhone = MSimPhoneFactory.getPhone(mSubscription);
        } else {
            mTMgr = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
            mPhone = PhoneFactory.getDefaultPhone();
        }
        if ("CDMA".equals(mPhone.getPhoneName())) {
            isCDMA = true;
        }
        mPhoneStateListener = getPhoneStateListener(mSubscription);

        mRes = getResources();
        if (mUnknown == null) {
            mUnknown = mRes.getString(R.string.device_info_default);
        }

        findPreference(KEY_MCC_MNC).setTitle(
                getString(R.string.status_mcc) + ","
                        + getString(R.string.status_mnc));
        findPreference(KEY_SID_NID).setTitle(
                getString(R.string.status_sid) + ","
                        + getString(R.string.status_nid));

        registerReceiver();
        updateSummery();
    }

    private void updateSummery() {
        String notAvailable = mRes.getString(R.string.device_info_not_available);
        String prlVersionSummery = notAvailable;
        String esnNumberSummery = notAvailable;
        String minNumberSummery = notAvailable;
        String iccIdSummery = notAvailable;
        if (isCDMA) {
            prlVersionSummery = mPhone.getCdmaPrlVersion();
            esnNumberSummery = mPhone.getEsn();
            minNumberSummery = mPhone.getCdmaMin();

            if (mPhone.getLteOnCdmaMode() == PhoneConstants.LTE_ON_CDMA_TRUE) {
                // Show ICC ID and IMEI for LTE device
                iccIdSummery = mPhone.getIccSerialNumber();
            }
        } else {
            boolean airplaneModeOn = Settings.System.getInt(
                    getContentResolver(), Settings.System.AIRPLANE_MODE_ON, 0) != 0;
            updateServiceCenterState(airplaneModeOn);
        }
        setSummery(KEY_PRL_VERSION, prlVersionSummery);
        setSummery(KEY_ESN_NUMBER, esnNumberSummery);
        setSummery(KEY_MIN_NUMBER, minNumberSummery);
        setSummery(KEY_ICC_ID, iccIdSummery);
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (mIsMultiSimEnabled) {
            mMSimTMgr.listen(mPhoneStateListener,
                    PhoneStateListener.LISTEN_SERVICE_STATE
                    | PhoneStateListener.LISTEN_SIGNAL_STRENGTHS
                    | PhoneStateListener.LISTEN_DATA_CONNECTION_STATE);
        } else {
            mTMgr.listen(mPhoneStateListener,
                    PhoneStateListener.LISTEN_SERVICE_STATE
                    | PhoneStateListener.LISTEN_SIGNAL_STRENGTHS
                    | PhoneStateListener.LISTEN_DATA_CONNECTION_STATE);
        }
    }

    @Override
    public void onPause() {
        super.onPause();
        if (mIsMultiSimEnabled) {
            mMSimTMgr.listen(mPhoneStateListener, PhoneStateListener.LISTEN_NONE);
        } else {
            mTMgr.listen(mPhoneStateListener, PhoneStateListener.LISTEN_NONE);
        }
    }

    @Override
    protected void onDestroy() {
        if (mReceiver != null) {
            unregisterReceiver(mReceiver);
        }
        super.onDestroy();
    }

    private PhoneStateListener getPhoneStateListener(final int subscription) {
        PhoneStateListener phoneStateListener = new PhoneStateListener(subscription) {
            @Override
            public void onSignalStrengthsChanged(SignalStrength signalStrength) {
                mSignalStrength = signalStrength;
                if (DEBUG) {
                    Log.d(TAG, "mSignalStrength:" + mSignalStrength);
                }
                updateSignalStrength();
            }

            @Override
            public void onServiceStateChanged(ServiceState state) {
                mServiceState = state;
                if (DEBUG) {
                    Log.d(TAG, "mServiceState:" + mServiceState);
                }
                updateServiceState();
                updateNetworkType(mSubscription);
            }

            @Override
            public void onDataConnectionStateChanged(int state) {
                mDataState = state;
                if (DEBUG) {
                    Log.d(TAG, "mDataState:" + mDataState);
                }
                updateDataState();
            }
        };
        return phoneStateListener;
    }

    private void setSummery(String preference, String text) {
        if (TextUtils.isEmpty(text)) {
            text = mUnknown;
        }
        // some preferences may be missing
        if (findPreference(preference) != null) {
            findPreference(preference).setSummary(text);
        }
    }

    private void updateServiceState() {
        String display = mUnknown;
        if (mServiceState != null) {
            int state = mServiceState.getState();
            switch (state) {
            case ServiceState.STATE_IN_SERVICE:
                display = mRes.getString(R.string.radioInfo_service_in);
                break;
            case ServiceState.STATE_OUT_OF_SERVICE:
            case ServiceState.STATE_EMERGENCY_ONLY:
                display = mRes.getString(R.string.radioInfo_service_out);
                break;
            case ServiceState.STATE_POWER_OFF:
                display = mRes.getString(R.string.radioInfo_service_off);
                break;
            }

            setSummery(KEY_SERVICE_STATE, display);
            String roamingStateSummery = null;
            if (mServiceState.getRoaming()) {
                roamingStateSummery = mRes.getString(R.string.radioInfo_roaming_in);
            } else {
                roamingStateSummery = mRes.getString(R.string.radioInfo_roaming_not);
            }
            setSummery(KEY_ROAMING_STATE, roamingStateSummery);

            String operatorNameSummery;
            if (mIsMultiSimEnabled) {
                operatorNameSummery = mMSimTMgr.getNetworkOperatorName(mSubscription);
            } else {
                operatorNameSummery = mTMgr.getNetworkOperatorName();
            }
            if (operatorNameSummery == null) {
                operatorNameSummery = mUnknown;
            }
            setSummery(KEY_OPERATOR_NAME, operatorNameSummery);

            String mccMncSummery = mUnknown;
            String operatorNumeric = mServiceState.getOperatorNumeric();
            if (!TextUtils.isEmpty(operatorNumeric)) {
                if (DEBUG) {
                    Log.d(TAG, "operatorNumeric:" + operatorNumeric);
                }
                mccMncSummery = operatorNumeric.substring(0, 3) + ","
                        + operatorNumeric.substring(3, operatorNumeric.length());
                new StringBuilder(operatorNumeric).insert(4, ",").toString();
            }
            setSummery(KEY_MCC_MNC, mccMncSummery);

            String sidNidSummery = mRes.getString(R.string.device_info_not_available);
            boolean airplaneModeOn = Settings.System.getInt(
                    getContentResolver(), Settings.System.AIRPLANE_MODE_ON, 0) != 0;
            if (isCDMA && !airplaneModeOn) {
                int sid = mServiceState.getSystemId();
                int nid = mServiceState.getNetworkId();
                if (sid > 0 && nid > 0) {
                    sidNidSummery = sid + "," + nid;
                } else {
                    sidNidSummery = mUnknown;
                }
            }
            setSummery(KEY_SID_NID, sidNidSummery);
        }
    }

    void updateSignalStrength() {
        // not loaded in some versions of the code (e.g., zaku)
        int signalDbm = 0;
        if (mSignalStrength != null) {
            int state = mServiceState.getState();
            Resources r = getResources();
            String mSigStrengthSummery = null;
            if ((ServiceState.STATE_OUT_OF_SERVICE == state)
                    || (ServiceState.STATE_POWER_OFF == state)) {
                mSigStrengthSummery = "0";
            } else {
                if (!mSignalStrength.isGsm()) {
                    signalDbm = mSignalStrength.getCdmaDbm();
                } else {
                    int gsmSignalStrength = mSignalStrength
                            .getGsmSignalStrength();
                    int asu = (gsmSignalStrength == 99 ? -1 : gsmSignalStrength);
                    if (asu != -1) {
                        signalDbm = -113 + 2 * asu;
                    }
                }
                if (-1 == signalDbm) {
                    signalDbm = 0;
                }

                int signalAsu = mSignalStrength.getGsmSignalStrength();
                if (-1 == signalAsu) {
                    signalAsu = 0;
                }

                mSigStrengthSummery = String.valueOf(signalDbm) + " "
                        + r.getString(R.string.radioInfo_display_dbm) + "   "
                        + String.valueOf(signalAsu) + " "
                        + r.getString(R.string.radioInfo_display_asu);
            }
            setSummery(KEY_SIGNAL_STRENGTH, mSigStrengthSummery);
        }
    }

    private String getNetworkTypeFromProperties(int subscription) {

        String prop;
        if (mIsMultiSimEnabled) {
            prop = mMSimTMgr.getTelephonyProperty(
                    TelephonyProperties.PROPERTY_DATA_NETWORK_TYPE, subscription, null);
        } else {
            prop = mTMgr.getTelephonyProperty(
                    TelephonyProperties.PROPERTY_DATA_NETWORK_TYPE, subscription, null);
        }

        if (TextUtils.isEmpty(prop)) {
            prop = mRes.getString(R.string.device_info_default);
        }
        return prop;
    }

    private void updateNetworkType(int subscription) {
        // Whether EDGE, UMTS, etc...
        String networkSummery = getNetworkTypeFromProperties(subscription);
        if (DEBUG) {
            Log.d(TAG, "networkSummery:" + networkSummery);
        }
        setSummery(KEY_NETWORK_TYPE, networkSummery);
    }

    private void updateDataState() {
        String display = mRes.getString(R.string.radioInfo_unknown);
        if (mSubscription == MSimTelephonyManager.getDefault()
                .getPreferredDataSubscription()) {
            switch (mDataState) {
            case TelephonyManager.DATA_CONNECTED:
                display = mRes.getString(R.string.radioInfo_data_connected);
                break;
            case TelephonyManager.DATA_SUSPENDED:
                display = mRes.getString(R.string.radioInfo_data_suspended);
                break;
            case TelephonyManager.DATA_CONNECTING:
                display = mRes.getString(R.string.radioInfo_data_connecting);
                break;
            case TelephonyManager.DATA_DISCONNECTED:
                display = mRes.getString(R.string.radioInfo_data_disconnected);
                break;
            }
        } else {
            display = mRes.getString(R.string.radioInfo_data_disconnected);
        }

        setSummery(KEY_DATA_STATE, display);
    }

    private void registerReceiver() {
        if (mReceiver != null) {
            return;
        }
        mReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();
                if (Intent.ACTION_AIRPLANE_MODE_CHANGED.equals(action)) {
                    // set the default as the airplane mode is off
                    boolean on = intent.getBooleanExtra("state", false);
                    updateServiceCenterState(on);
                } else if (NOTIFY_SMSC_UPDATE.equals(action)) {
                    String summary = intent.getStringExtra(SMSC);
                    setSummery(KEY_SMS_SERVICE_CENTER, summary);
                }
            }
        };

        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_AIRPLANE_MODE_CHANGED);
        filter.addAction(NOTIFY_SMSC_UPDATE);
        registerReceiver(mReceiver, filter);

    }

    private void updateServiceCenterState(boolean airplaneModeOn) {
        if (airplaneModeOn) {
            setSummery(KEY_SMS_SERVICE_CENTER, mUnknown);
            return;
        }
        // We need update the preference summary.
        Intent get = new Intent();
        get.setComponent(new ComponentName("com.android.phone",
                "com.android.phone.smsc.SMSCService"));
        get.setAction(COMMAND_GET_SMSC);
        get.putExtra(SUB, mSubscription);
        startService(get);
    }
}
