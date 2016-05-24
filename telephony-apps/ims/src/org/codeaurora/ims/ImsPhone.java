/*
 * Copyright (c) 2012-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
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
 *
 * Copyright (C) 2010 The Android Open Source Project
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

import static com.android.internal.telephony.CommandsInterface.CF_ACTION_DISABLE;
import static com.android.internal.telephony.CommandsInterface.CF_ACTION_ENABLE;
import static com.android.internal.telephony.CommandsInterface.CF_ACTION_ERASURE;
import static com.android.internal.telephony.CommandsInterface.CF_ACTION_REGISTRATION;
import static com.android.internal.telephony.CommandsInterface.CF_REASON_ALL;
import static com.android.internal.telephony.CommandsInterface.CF_REASON_ALL_CONDITIONAL;
import static com.android.internal.telephony.CommandsInterface.CF_REASON_BUSY;
import static com.android.internal.telephony.CommandsInterface.CF_REASON_NOT_REACHABLE;
import static com.android.internal.telephony.CommandsInterface.CF_REASON_NO_REPLY;
import static com.android.internal.telephony.CommandsInterface.CF_REASON_UNCONDITIONAL;
import static com.android.internal.telephony.CommandsInterface.SERVICE_CLASS_VOICE;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;

import android.app.ActivityManagerNative;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.media.AudioManager;
import android.os.AsyncResult;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.os.Registrant;
import android.os.RegistrantList;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.preference.PreferenceManager;
import android.telephony.CellLocation;
import android.telephony.PhoneNumberUtils;
import android.telephony.ServiceState;
import android.telephony.SignalStrength;
import android.util.Log;
import android.location.CountryDetector;
import android.location.Country;
import android.widget.Toast;

import com.android.internal.telephony.Call;
import com.android.internal.telephony.CallForwardInfo;
import com.android.internal.telephony.CallStateException;
import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.Connection;
import com.android.internal.telephony.Connection.DisconnectCause;
import com.android.internal.telephony.CallManager;
import com.android.internal.telephony.IccPhoneBookInterfaceManager;
import com.android.internal.telephony.MmiCode;
import com.android.internal.telephony.OperatorInfo;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneBase;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.PhoneNotifier;
import com.android.internal.telephony.PhoneSubInfo;
import com.android.internal.telephony.RILConstants;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.TelephonyProperties;
import com.android.internal.telephony.UUSInfo;
import com.android.internal.telephony.gsm.SuppServiceNotification;
import com.android.internal.telephony.uicc.IccRecords;
import com.android.internal.telephony.CallDetails;

import java.util.Locale;
import org.codeaurora.ims.conference.ConfInfo;

import com.qualcomm.ims.csvt.CsvtUtils;

/**
 * {@hide}
 */
public class ImsPhone extends PhoneBase {
    private static final String LOG_TAG = "ImsPhone";
    private static final boolean DEBUG = true;
    static final int MAX_CONNECTIONS = 7;
    static final int MAX_CONNECTIONS_PER_CALL = 5;
    protected static final int EVENT_IMS_STATE_CHANGED = 36;
    protected static final int EVENT_IMS_STATE_DONE = 37;
    protected static final int EVENT_REFRESH_CONF_INFO = 39;
    protected static final int EVENT_SRV_STATUS_UPDATE = 40;
    protected static final int EVENT_GET_SRV_STATUS = 41;
    protected static final int EVENT_SET_SRV_STATUS = 42;

    // Will be removed once ImsService is ready.
    protected final RegistrantList mCsvtPreciseCallStateRegistrants =
            new RegistrantList();
    protected final RegistrantList mCsvtNewRingingConnectionRegistrants =
            new RegistrantList();
    protected final RegistrantList mCsvtDisconnectRegistrants =
            new RegistrantList();

    private PhoneConstants.State state = PhoneConstants.State.IDLE; // phone state for IMS phone
    private ServiceState mServiceState;

    // if set to true, the actual service state will be ignored, the
    // getServiceState function will return ServiceState.STATE_IN_SERVICE
    // regardless of the actual service state. This is a temporary solution for
    // CSVT, which doesn't use IMS's service state.
    private boolean mIgnoreServiceState = false;

    // if set to true, the getServiceState function will return
    // ServiceState.STATE_IN_SERVICE regardless the actual service state.
    // This is a temporary solution for CSVT, which doesn't use IMS's service state.
    public void setIgnoreSerivceStateFlag(boolean ignore) {
        mIgnoreServiceState = ignore;
    }

    public boolean getIgnoreSerivceStateFlag() {
        return mIgnoreServiceState;
    }

    // mEcmExitRespRegistrant is informed after the phone has been exited
    //the emergency callback mode
    //keep track of if phone is in emergency callback mode
    private boolean mIsPhoneInEcmState;
    private Registrant mEcmExitRespRegistrant;
    WakeLock mWakeLock;
    public ImsCallTracker mCT;
    public ImsSenderRxr cm;
    ArrayList <ImsMmiCode> mPendingMMIs = new ArrayList<ImsMmiCode>();

    public Registrant mPostDialHandler;

    /** List of Registrants to receive Supplementary Service Notifications. */
    RegistrantList mSsnRegistrants = new RegistrantList();

    public static final int RESTART_ECM_TIMER = 0; // restart Ecm timer
    public static final int CANCEL_ECM_TIMER = 1; // cancel Ecm timer

    // Create Cfu (Call forward unconditional) so that dialling number &
    // onCfComplete (Message object passed by client) can be packed &
    // given as a single Cfu object as user data to RIL.
    private static class Cfu {
        final String setCfNumber;
        final Message onCfComplete;

        Cfu(String cfNumber, Message onComplete) {
            setCfNumber = cfNumber;
            onCfComplete = onComplete;
        }
    }
    /** Key used to read/write the SIM IMSI used for storing the voice mail
     *  NOTE: This will update the Voice Mail for CS calls also as GSM and IMS
     *  share the same preferences for this key. This is the expected behavior.
     */
    public static final String VM_SIM_IMSI = "vm_sim_imsi_key";
    // Key used to read/write if Call Forwarding is enabled
    public static final String CF_ENABLED = "cf_enabled_key";
    /**
     * Specify if IMS calls should be originated with PS domain
     */
    private static final String IMS_PS_DOMAIN = "persist.radio.domain.ps";

    private static final int SERVICE_TYPE_MAX = 4; /*
                                                    * VOIP, VT_TX, VT_RX, VT
                                                    */
    private CountryDetector mDetector;

    public static class ServiceStatus{
        public boolean isValid;
        public int type; /* Type is the Key and index for mServiceStatus */
        public int status; /*
                            * Overall Status best case for this type across all
                            * access techs
                            */
        public byte[] userdata;
        public StatusForAccessTech[] accessTechStatus;

        public static class StatusForAccessTech {
            public int networkMode;
            public int status;
            public int restrictCause;
            public int registered;
        }

        public ServiceStatus() {
            this.isValid = false;
        }
    }


    ServiceStatus mServiceStatus[] = null;
    // Default Emergency Callback Mode exit timer for 5 minutes.
    private static final int DEFAULT_ECM_EXIT_TIMER_VALUE = 300000;

    // mEcmTimerResetRegistrants are informed after Ecm timer is canceled or re-started
    private final RegistrantList mEcmTimerResetRegistrants = new RegistrantList();

    // mSrvStatusChangedRegistrants are informed after Service Status changes
    private final RegistrantList mSrvStatusChangedRegistrants = new RegistrantList();

    // A runnable which is used to automatically exit from Ecm after a period of time.
    private Runnable mExitEcmRunnable = new Runnable() {
        @Override
        public void run() {
            exitEmergencyCallbackMode();
        }
    };

    public ImsPhone(Context context, PhoneNotifier notifier, ImsSenderRxr mCi) {
        super("IMS", notifier, context, mCi);
        setCallTracker();
        cm = mCi;
        mDetector = (CountryDetector) mContext.getSystemService(
                                               mContext.COUNTRY_DETECTOR);
        cm.registerForImsNetworkStateChanged(this, EVENT_IMS_STATE_CHANGED,
                null);
        // Query for registration state in case we have missed the UNSOL
        cm.getImsRegistrationState(this.obtainMessage(EVENT_IMS_STATE_DONE));
        cm.registerForRefreshConfInfo(this, EVENT_REFRESH_CONF_INFO, null);
        cm.registerForSrvStatusUpdate(this, EVENT_SRV_STATUS_UPDATE, null);
        cm.setOnSuppServiceNotification(this, EVENT_SSN, null);
        mServiceState = new ServiceState();
        mServiceState.setStateOutOfService();
        initServiceStatus();
        queryServiceStatus(null);
        cm.setEmergencyCallbackMode(this, EVENT_EMERGENCY_CALLBACK_MODE_ENTER, null);
        cm.registerForExitEmergencyCallbackMode(this, EVENT_EXIT_EMERGENCY_CALLBACK_RESPONSE,
                null);

        // This is needed to handle phone process crashes
        // Same property is used for both CDMA & IMS phone.
        mIsPhoneInEcmState = SystemProperties.getBoolean(
                TelephonyProperties.PROPERTY_INECM_MODE, false);
        if (mIsPhoneInEcmState) {
            // Send a message which will invoke handleExitEmergencyCallbackMode
            cm.exitEmergencyCallbackMode(obtainMessage(EVENT_EXIT_EMERGENCY_CALLBACK_RESPONSE));
        }

        PowerManager pm
            = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,LOG_TAG);
        mWakeLock.setReferenceCounted(false);
    }

    /* Method to initialize the Service related objects */
    private void initServiceStatus() {
        mServiceStatus = new ServiceStatus[SERVICE_TYPE_MAX];
        for (int i = 0; i < SERVICE_TYPE_MAX; i++) {
            mServiceStatus[i] = new ServiceStatus();
            /*
             * By default, the assumption is the service is enabled on LTE -
             * when RIL and modem changes to update the availability of service
             * on power up this will be removed
             */
            mServiceStatus[i].accessTechStatus = new ServiceStatus.StatusForAccessTech[1];
            mServiceStatus[i].accessTechStatus[0] = new ServiceStatus.StatusForAccessTech();
            mServiceStatus[i].accessTechStatus[0].networkMode = ImsQmiIF.RADIO_TECH_LTE;
            mServiceStatus[i].accessTechStatus[0].status = ImsQmiIF.STATUS_NOT_SUPPORTED;
            mServiceStatus[i].accessTechStatus[0].registered = ImsQmiIF.Registration.NOT_REGISTERED;
            mServiceStatus[i].status = ImsQmiIF.STATUS_NOT_SUPPORTED;
        }
        mServiceState.setRilDataRadioTechnology(ImsQmiIF.RADIO_TECH_LTE);
    }

    public String getCountryIso() {
        String countryIso = null;
        Country country;
        if ((mDetector != null) && ((country = mDetector.detectCountry()) != null)) {
                countryIso = country.getCountryIso();
        } else {
            Locale locale = mContext.getResources().getConfiguration().locale;
            if(locale != null) {
                countryIso = locale.getCountry();
                Log.w(LOG_TAG, "No CountryDetector; falling back to countryIso based on locale: "
                                   + countryIso);
            } else {
                countryIso = "US"; //default value is "US"
            }
        }
        return countryIso;
    }

    protected void setCallTracker() {
        mCT = new ImsCallTracker(this);
    }

    boolean isExactOrPotentialLocalEmergencyNumber(String number) {
        if (number == null) {
            return false;
        }
        boolean isExactEmergencyNumber =
                PhoneNumberUtils.isEmergencyNumber(number, getCountryIso());
        boolean isPotentialEmergencyNumber =
                PhoneNumberUtils.isPotentialEmergencyNumber(number, getCountryIso());

        return (isExactEmergencyNumber || isPotentialEmergencyNumber);
    }

    public void queryServiceStatus(Message msg) {
        cm.queryServiceStatus(obtainMessage(EVENT_GET_SRV_STATUS, msg));
    }

    private static boolean isSrvTypeValid(int type) {
        return ((type < ImsQmiIF.CALL_TYPE_VOICE) || (type > ImsQmiIF.CALL_TYPE_VT)) ? false
                : true;
    }

    private void handleSrvStatusUpdate(ArrayList<ServiceStatus> updateList) {
        for (ServiceStatus update : updateList) {
            Log.d(LOG_TAG, "type = " + update.type + " status = " + update.status
                    + " isValid = " + update.isValid);
            if (update.isValid && isSrvTypeValid(update.type)) {
                ServiceStatus srvSt = mServiceStatus[update.type];
                srvSt.isValid = update.isValid;
                srvSt.type = update.type;
                if (update.userdata != null) {
                    srvSt.userdata = new byte[update.userdata.length];
                    srvSt.userdata = Arrays.copyOf(update.userdata, update.userdata.length);
                }
                if (update.accessTechStatus != null && update.accessTechStatus.length > 0) {
                    srvSt.accessTechStatus = new ServiceStatus.StatusForAccessTech[update.
                            accessTechStatus.length];
                    Log.d(LOG_TAG, "Call Type " + srvSt.type + "has num updates = " +
                            update.accessTechStatus.length);
                    ServiceStatus.StatusForAccessTech[] actSt = srvSt.accessTechStatus;
                    for (int i = 0; i < update.accessTechStatus.length; i++) {
                        ServiceStatus.StatusForAccessTech actUpdate = update.accessTechStatus[i];
                        actSt[i] = new ServiceStatus.StatusForAccessTech();
                        actSt[i].networkMode = actUpdate.networkMode;
                        actSt[i].registered = actUpdate.registered;
                        if (actUpdate.status == ImsQmiIF.STATUS_ENABLED &&
                                actUpdate.restrictCause != CallDetails.CALL_RESTRICT_CAUSE_NONE) {
                            actSt[i].status = ImsQmiIF.STATUS_PARTIALLY_ENABLED;
                        } else {
                            actSt[i].status = actUpdate.status;
                        }
                        /*
                         * Currently only one RAT information is passed for a
                         * service so the overall status is updated right away,
                         * in future when modem supports per RAT information,
                         * more logic to be added to update the overall status
                         * for service
                         */
                        srvSt.status = actSt[i].status;
                        actSt[i].restrictCause = actUpdate.restrictCause;
                    }
                }
            }
        }
        setRadioTechChange();
    }

    private void setRadioTechChange() {
        int currTech = mServiceState.getRilDataRadioTechnology();
        for (ServiceStatus srv : mServiceStatus) {
            if (srv.isValid) {
                for (int i = 0; i < srv.accessTechStatus.length; i++) {
                    ServiceStatus.StatusForAccessTech actStatus = srv.accessTechStatus[i];
                    if ((actStatus.status == ImsQmiIF.STATUS_ENABLED ||
                            actStatus.status == ImsQmiIF.STATUS_PARTIALLY_ENABLED) &&
                            (currTech != actStatus.networkMode &&
                            (actStatus.networkMode == ImsQmiIF.RADIO_TECH_LTE ||
                            actStatus.networkMode == ImsQmiIF.RADIO_TECH_WIFI))) {
                        Log.d(LOG_TAG, "Update Radio Tech " + actStatus.networkMode);
                        mServiceState.setRilDataRadioTechnology(actStatus.networkMode);
                        break;
                    }
                }
            }
        }
        // if modem does not send radio tech, default it to LTE
        if (currTech == 0) {
            mServiceState.setRilDataRadioTechnology(ImsQmiIF.RADIO_TECH_LTE);
            Log.d(LOG_TAG, "Default radio to Lte");
        }
    }

    public void setServiceStatus(Message msg, int service, int networkType, int enabled,
            int restrictCause) {
        cm.setServiceStatus(obtainMessage(EVENT_SET_SRV_STATUS, msg),
                service, networkType, enabled, restrictCause);
    }

    void handleRefreshConfInfo(ImsQmiIF.ConfInfo confInfo) {
        mCT.handleRefreshConfInfo(confInfo);
    }

    public String[] getUriListinConf() {
        String[] uriListInConf = null;
        uriListInConf = mCT.getUriListinConf();
        return uriListInConf;
    }

    public String[] getCallDetailsExtras(int callId){
        return mCT.getCallDetailsExtras(callId);
    }

    public String getImsDisconnectCauseInfo(int callId){
        return mCT.getImsDisconnectCauseInfo(callId);
    }

    public boolean getIsServiceAllowed(int callType) {
        boolean allowed = true;
        /*
         * The highest priority is if the overall Service Capability is supported as a feature
         * across calls. The next lower priority is the per call service capability.
         */
        if (isSrvTypeValid(callType) && mServiceStatus[callType].isValid
                && (mServiceStatus[callType].status == ImsQmiIF.STATUS_DISABLED) ||
                (mServiceStatus[callType].status == ImsQmiIF.STATUS_NOT_SUPPORTED)) {
            allowed = false;
        } else {
            allowed = mCT.getIsServiceAllowed(callType);
        }
        return allowed;
    }

    protected void setVmSimImsi(String imsi) {
        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(getContext());
        SharedPreferences.Editor editor = sp.edit();
        editor.putString(VM_SIM_IMSI, imsi);
        editor.apply();
    }

    /**
     * This method stores the CF_ENABLED flag in preferences
     * @param enabled
     */
    protected void setCallForwardingPreference(boolean enabled) {
        Log.d(LOG_TAG, "Set callforwarding info to preferences");
        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(mContext);
        SharedPreferences.Editor edit = sp.edit();
        edit.putBoolean(CF_ENABLED, enabled);
        edit.commit();

        // Using the same method as VoiceMail to be able to track when the sim card is changed.
        setVmSimImsi(getSubscriberId());
    }

    private void handleCfuQueryResult(CallForwardInfo[] infos) {
        Log.d(LOG_TAG, "In handleCfuQueryResult" + infos);
        IccRecords r = mIccRecords.get();
        if (r != null) {
            if (infos == null || infos.length == 0) {
                // Assume the default is not active
                // Set unconditional CFF in SIM to false
                r.setVoiceCallForwardingFlag(1, false, null);
            } else {
                for (int i = 0, s = infos.length; i < s; i++) {
                    if ((infos[i].serviceClass & SERVICE_CLASS_VOICE) != 0) {
                        setCallForwardingPreference(infos[i].status == 1);
                        r.setVoiceCallForwardingFlag(1, (infos[i].status == 1), infos[i].number);
                        // should only have the one
                        break;
                    }
                }
            }
        }
    }

    /**
     * Make sure the network knows our preferred setting.
     */
    protected  void syncClirSetting() {
        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(getContext());
        int clirSetting = sp.getInt(CLIR_KEY, -1);
        Log.d(LOG_TAG, "Syncing CLIR setting " + clirSetting + " with network");
        if (clirSetting >= 0) {
            mCi.setCLIR(clirSetting, null);
        }
    }

    public void handleRadioStateChange(CommandsInterface.RadioState state) {
        Log.v(LOG_TAG, "handleRadioStateChange for state " + state);
        switch (state) {
            case RADIO_OFF: /* intentional fall through */
            case RADIO_UNAVAILABLE:
                mServiceState.setStateOff();
                break;
            case RADIO_ON:
                mCi.getImsRegistrationState(this
                        .obtainMessage(EVENT_IMS_STATE_DONE));
                break;
            default:
                Log.e(LOG_TAG, "Invalid Radio State " + state);
                break;
        }
    }

    public void handleMessage(Message msg) {
        Log.d(LOG_TAG, "Received event:" + msg.what);
        Message onComplete;

        switch (msg.what) {
            case EVENT_IMS_STATE_CHANGED:
                mCi.getImsRegistrationState(this
                        .obtainMessage(EVENT_IMS_STATE_DONE));
                break;
            case EVENT_IMS_STATE_DONE:
                AsyncResult ar = (AsyncResult) msg.obj;
                if (ar.exception == null && ar.result != null
                        && ((int[]) ar.result).length >= 1) {
                    int[] responseArray = (int[]) ar.result;
                    Log.d(LOG_TAG, "IMS registration state is: " + responseArray[0]);
                    if (responseArray[0] == ImsQmiIF.Registration.REGISTERED) {
                        mServiceState.setState(ServiceState.STATE_IN_SERVICE);
                        /* Sync CLIR setting made via UI with the network */
                        syncClirSetting();
                    } else if (mCi.getRadioState() != CommandsInterface.RadioState.RADIO_OFF) {
                        mServiceState.setState(ServiceState.STATE_OUT_OF_SERVICE);
                    }
                } else {
                    Log.e(LOG_TAG, "IMS State query failed!");
                }
                break;
            case EVENT_REFRESH_CONF_INFO:
                AsyncResult arResultConf = (AsyncResult) msg.obj;
                if (arResultConf != null && arResultConf.exception == null
                        && arResultConf.result != null) {
                    ImsQmiIF.ConfInfo responseArray = (ImsQmiIF.ConfInfo) arResultConf.result;
                    handleRefreshConfInfo(responseArray);
                } else {
                    Log.e(LOG_TAG, "IMS Refresh Conference Information failed!");
                }
                break;

           case EVENT_SET_SRV_STATUS:
                Log.d(LOG_TAG, "Received event: EVENT_SET_SRV_STATUS");
                AsyncResult arResultSrvSt = (AsyncResult) msg.obj;
                if (arResultSrvSt != null && arResultSrvSt.userObj != null) {
                    onComplete = (Message) arResultSrvSt.userObj;
                    AsyncResult.forMessage(onComplete, null,
                            arResultSrvSt.exception);
                    onComplete.sendToTarget();
                }
                break;

            case EVENT_GET_SRV_STATUS:
                Log.d(LOG_TAG, "Received event: EVENT_GET/SRV_STATUS_UPDATE");
                AsyncResult arResultSrv = (AsyncResult) msg.obj;
                if (arResultSrv.exception == null && arResultSrv.result != null) {
                    ArrayList<ServiceStatus> responseArray =
                            (ArrayList<ServiceStatus>) arResultSrv.result;
                    handleSrvStatusUpdate(responseArray);
                } else {
                    Log.e(LOG_TAG, "IMS Service Status Update failed!");
                    initServiceStatus();
                }
                if (mSrvStatusChangedRegistrants != null) {
                    mSrvStatusChangedRegistrants.notifyRegistrants();
                }
                if (arResultSrv != null && arResultSrv.userObj != null) {
                    onComplete = (Message) arResultSrv.userObj;
                    AsyncResult.forMessage(onComplete, arResultSrv.result,
                            arResultSrv.exception);
                    onComplete.sendToTarget();
                }
                break;

            case EVENT_SRV_STATUS_UPDATE:
                Log.d(LOG_TAG, "Received event: EVENT_GET/SRV_STATUS_UPDATE");
                AsyncResult arResult = (AsyncResult) msg.obj;
                if (arResult.exception == null && arResult.result != null) {
                    ArrayList<ServiceStatus> responseArray =
                            (ArrayList<ServiceStatus>) arResult.result;
                    handleSrvStatusUpdate(responseArray);
                } else {
                    Log.e(LOG_TAG, "IMS Service Status Update failed!");
                    initServiceStatus();
                }
                if (mSrvStatusChangedRegistrants != null) {
                    mSrvStatusChangedRegistrants.notifyRegistrants();
                }
                break;

            case EVENT_GET_CALL_FORWARD_DONE:
                AsyncResult arGetCallForward = (AsyncResult)msg.obj;
                if (arGetCallForward.exception == null) {
                    handleCfuQueryResult((CallForwardInfo[])arGetCallForward.result);
                    Log.d(LOG_TAG, "handleCfuQueryResult!");
                }
                onComplete = (Message) arGetCallForward.userObj;
                if (onComplete != null) {
                    AsyncResult.forMessage(onComplete, arGetCallForward.result,
                            arGetCallForward.exception);
                    onComplete.sendToTarget();
                }
                break;
            case EVENT_SET_CALL_FORWARD_DONE:
                ar = (AsyncResult)msg.obj;
                IccRecords r = mIccRecords.get();
                Cfu cfu = (Cfu) ar.userObj;
                if (ar.exception == null && r != null) {
                    r.setVoiceCallForwardingFlag(1, msg.arg1 == 1, cfu.setCfNumber);
                }
                if (cfu.onCfComplete != null) {
                    AsyncResult.forMessage(cfu.onCfComplete, ar.result, ar.exception);
                    cfu.onCfComplete.sendToTarget();
                }
                break;

            case EVENT_EMERGENCY_CALLBACK_MODE_ENTER:
                handleEnterEmergencyCallbackMode(msg);
                break;
            case  EVENT_EXIT_EMERGENCY_CALLBACK_RESPONSE:
                handleExitEmergencyCallbackMode(msg);
                break;
            case EVENT_SSN:
                handleSuppSvcNotification(msg);
                break;
            default:
                super.handleMessage(msg);
        }
    }

    private void handleSuppSvcNotification(Message msg) {
        AsyncResult arSuppService = (AsyncResult) msg.obj;
        if ((arSuppService != null) && (arSuppService.exception == null)) {
            if (arSuppService.result != null) {
                ImsQmiIF.SuppSvcNotification ssn =
                        (ImsQmiIF.SuppSvcNotification) arSuppService.result;
                SuppServiceNotification response = new SuppServiceNotification();
                response.notificationType = ssn.getNotificationType();
                response.code = ssn.getCode();
                response.index = ssn.getIndex();
                response.type = ssn.getType();
                response.number = ssn.getNumber();
                mSsnRegistrants.notifyRegistrants(new AsyncResult(null, response, null));
                if (ssn.hasConnId()) {
                    int index = ssn.getConnId();
                    Log.d(LOG_TAG, "handleSuppSvcNotification.conn id: " + index);
                    ImsConnection conn = mCT.getConnectionByIndex(index);
                    if (conn != null) {
                        Log.d(LOG_TAG, "suppSvcNotification.code: " + response.code);
                        conn.mtCodeOnSsn = response.code;
                    }
                } else {
                    Log.d(LOG_TAG, "handleSuppSvcNotification: SSN has no conn id");
                }
            } else {
                Log.d(LOG_TAG, "suppSvcNotification result is null");
            }
        }
    }

    @Override
    public boolean equals(Object o) {
        if (o == this)
            return true;
        return false;
    }

    public String getPhoneName() {
        return "IMS";
    }

    public boolean equals(Phone phone) {
        return phone == this;
    }

    public void acceptCall() throws CallStateException {
        mCT.acceptCall();
    }

    @Override
    public void acceptCall(int callType) throws CallStateException {
        mCT.acceptCall(callType);
    }

    @Override
    public int getCallType(Call call) {
        return mCT.getCallType(call);
    }

    @Override
    public int getCallDomain(Call call) {
        return mCT.getCallDomain(call);
    }

    public void rejectCall() throws CallStateException {
        mCT.rejectCall();
    }

    public void rejectCall(int failCause) throws CallStateException {
        mCT.rejectCall(failCause);
    }


    public boolean canDial() {
        return mCT.canDial();
    }

    boolean isInCall() {
        return (getForegroundCall().getState().isAlive() ||
                getBackgroundCall().getState().isAlive() ||
                getRingingCall().getState().isAlive());
    }

    /**
     * Saves CLIR setting so that we can re-apply it as necessary
     * (in case the RIL resets it across reboots).
     */
    public void saveClirSetting(int commandInterfaceCLIRMode) {
        // open the shared preferences editor, and write the value.
        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(getContext());
        SharedPreferences.Editor editor = sp.edit();
        editor.putInt(CLIR_KEY, commandInterfaceCLIRMode);

        // commit and log the result.
        if (! editor.commit()) {
            Log.e(LOG_TAG, "failed to commit CLIR preference");
        }
    }

    public boolean isAddParticipantAllowed() {
        return (mCT.isAddParticipantAllowed());
    }

    public void addParticipant(String dialString, int clir, int callType, String[] extras) {
        int domain = getCallDomain(getForegroundCall());

        if (callType == CallDetails.CALL_TYPE_UNKNOWN) {
            callType = getCallType(getForegroundCall());
        }

        CallDetails details = new CallDetails(callType, domain, extras);

        mCT.addParticipant(dialString, clir, details);
    }

    public Connection dial(String dialString) throws CallStateException {
        throw new CallStateException(
                "Dial with dialstring not supported for ImsPhone");
    }

    public Connection dial(String dialString, UUSInfo uusInfo)
            throws CallStateException {
        throw new CallStateException(
                "Dial with uusinfo not supported for ImsPhone");
    }

    private boolean canDial(int callType) {
         boolean allowed = false;
         boolean ttyenabled = false;
         if (isSrvTypeValid(callType)) {
             try {
                 if (mServiceStatus[callType].status == ImsQmiIF.STATUS_PARTIALLY_ENABLED ||
                         mServiceStatus[callType].status == ImsQmiIF.STATUS_ENABLED) {
                     allowed = true;
                     if(ImsCallUtils.isVideoCall(callType)) {
                         ttyenabled = android.provider.Settings.Secure.getInt(
                                 getContext().getContentResolver(),
                                 android.provider.Settings.Secure.PREFERRED_TTY_MODE,
                                 Phone.TTY_MODE_OFF) != Phone.TTY_MODE_OFF;
                         allowed = !ttyenabled;
                     }
                 }
             } catch (NullPointerException e) {
                 Log.e(LOG_TAG, "Service Status null for Type " + callType);
             }
         }
         if (!allowed) {
             String toastMsg = "Cannot dial";
             if (callType == CALL_TYPE_VOICE || callType == CALL_TYPE_VT ) {
                 if (ttyenabled) {
                     toastMsg += " VT call when TTY is enabled";
                 } else {
                     toastMsg += " as CallType is not supported. ";
                     toastMsg += "Try Ims Account->Enable/Disable Ims Service Capability->Select ";
                     toastMsg += (callType == CALL_TYPE_VOICE) ? "Voice":"Video";
                 }
             }
             Toast toast = Toast.makeText(getContext(), toastMsg, Toast.LENGTH_LONG);
             toast.show();
         }
         return allowed;
    }

    public Connection dial(String dialString, int callType
            , int callDomain, String[] extras)
            throws CallStateException {

        int clir = CommandsInterface.CLIR_DEFAULT;
        String newDialString = dialString;
        boolean isConferenceUri = false;
        CallDetails details = new CallDetails(callType,
                callDomain, extras);
        if (details != null && details.extras != null) {
            String value = details.getValueForKeyFromExtras(details.extras,
                    CallDetails.EXTRAS_IS_CONFERENCE_URI);
            if (value != null && Boolean.valueOf(value)) {
                isConferenceUri = true;
            }
        }

        if (!isConferenceUri) {
            // Need to make sure dialString gets parsed properly
            newDialString = PhoneNumberUtils.stripSeparators(dialString);
        }

        String networkPortion = newDialString;
        // Consider complete dialString for ICB (Specific DN)
        if (!ImsMmiCode.isScMatchesIcb(newDialString)) {
            // Only look at the Network portion for mmi
            networkPortion = PhoneNumberUtils.extractNetworkPortionAlt(newDialString);
        }
        Log.d(LOG_TAG, "networkPortion: " + networkPortion);
        ImsMmiCode mmi = ImsMmiCode.newFromDialString(networkPortion, this, mIccRecords.get());

        Log.d(LOG_TAG, "dialing w/ mmi '" + mmi + "'...");

        if (mmi != null) {
            if (mmi.isTemporaryModeCLIR()) {
                clir = mmi.getCLIRMode();
                if (mmi.dialingNumber != null) {
                    dialString = mmi.dialingNumber;
                }
            } else {
                mPendingMMIs.add(mmi);
                mMmiRegistrants.notifyRegistrants(new AsyncResult(null, mmi, null));
                mmi.processCode();
                // FIXME should this return null or something else?
                return null;
            }
        }
        return mCT.dial(dialString, clir, details);
    }

    public Connection dial(String dialString, int callType, String[] extras)
            throws CallStateException {

        int callDomain = CallDetails.CALL_DOMAIN_AUTOMATIC;
        Connection conn = null;

        // For emergency calls use auto domain always
        boolean isEmergencyNumber =
                isExactOrPotentialLocalEmergencyNumber(dialString);
        boolean shouldDial = true;
        if (!isEmergencyNumber && isPSDomain()) {
            callDomain = CallDetails.CALL_DOMAIN_PS;
            shouldDial = canDial(callType);
        }
        if (shouldDial) {
            conn = dial(dialString, callType, callDomain, extras);
        }

        return conn;
    }

    public void switchHoldingAndActive() throws CallStateException {
        if (DEBUG)
            Log.d(LOG_TAG, " ~~~~~~  switch fg and bg");
        synchronized (ImsPhone.class) {
            mCT.switchWaitingOrHoldingAndActive();
        }
    }

    public void conference() throws CallStateException {
        mCT.conference();
    }

    public boolean canConference() {
        return mCT.canConference();
    }

    public boolean canTransfer() {
        return mCT.canTransfer();
    }

    public void explicitCallTransfer() throws CallStateException {
        mCT.explicitCallTransfer();
    }

    /**
     * Notify any interested party of a Phone state change {@link Phone.State}
     */
    public void notifyPhoneStateChanged() {
        // Ignore (don't notify) Csvt caused state changes.
        switch (getState()) {
            case OFFHOOK: {
                if (!CsvtUtils.hasActiveCsvtConnection(this)) {
                    mNotifier.notifyPhoneState(this);
                }
                break;
            }
            case IDLE: {
                mNotifier.notifyPhoneState(this);
                break;
            }
            case RINGING: {
                if (!CsvtUtils.hasActiveRingingCsvtConnection(this)) {
                    mNotifier.notifyPhoneState(this);
                }
                break;
            }
        }
    }

    public void notifySuppServiceFailed(SuppService code) {
        mSuppServiceFailedRegistrants.notifyResult(code);
    }

    void updatePhoneState() {
        PhoneConstants.State oldState = state;

        if (getRingingCall().isRinging()) {
            state = PhoneConstants.State.RINGING;
        } else if (getForegroundCall().isIdle() && getBackgroundCall().isIdle()) {
            state = PhoneConstants.State.IDLE;
        } else {
            state = PhoneConstants.State.OFFHOOK;
        }

        if (state != oldState) {
            Log.d(LOG_TAG, " ^^^ new phone state: " + state);
            notifyPhoneStateChanged();
        }
    }

    public void registerForCsvtPreciseCallStateChanged(Handler h, int what, Object obj){
        mCsvtPreciseCallStateRegistrants.addUnique(h, what, obj);
    }

    public void unregisterForCsvtPreciseCallStateChanged(Handler h){
        mCsvtPreciseCallStateRegistrants.remove(h);
    }

    public void registerForCsvtNewRingingConnection(Handler h, int what, Object obj){
        mCsvtNewRingingConnectionRegistrants.addUnique(h, what, obj);
    }

    public void unregisterForCsvtNewRingingConnection(Handler h){
        mCsvtNewRingingConnectionRegistrants.remove(h);
    }

    public void registerForCsvtDisconnect(Handler h, int what, Object obj){
        mCsvtDisconnectRegistrants.addUnique(h, what, obj);
    }

    public void unregisterForCsvtDisconnect(Handler h){
        mCsvtDisconnectRegistrants.remove(h);
    }

    /**
     * Notify registrants of a change in the call state. This notifies changes
     * in {@link Call.State} Use this when changes in the precise call state are
     * needed, else use notifyPhoneStateChanged.
     */
    public void notifyPreciseCallStateChanged() {
        if (DEBUG)
            Log.d(LOG_TAG, "Filter: notifyPreciseCallStateChanged");

        PhoneConstants.State ps = getState();
        if (DEBUG)
            Log.v(LOG_TAG, "Phone state:" + ps);

        switch (ps) {
            case OFFHOOK: {
                if (CsvtUtils.hasActiveCsvtConnection(this)) {
                    mCsvtPreciseCallStateRegistrants.notifyRegistrants();
                } else {
                    super.notifyPreciseCallStateChangedP();
                }
                break;
            }
            case IDLE: {
                mCsvtPreciseCallStateRegistrants.notifyRegistrants();
                super.notifyPreciseCallStateChangedP();
                break;
            }
            case RINGING: {
                if (CsvtUtils.hasActiveRingingCsvtConnection(this)) {
                    mCsvtPreciseCallStateRegistrants.notifyRegistrants();
                } else {
                    super.notifyPreciseCallStateChangedP();
                }
                break;
            }
        }
    }

    public void clearDisconnected() {
        mCT.clearDisconnected();
    }

    public void hangupWithReason(int connectionId, String userUri,
            boolean mpty, int failCause, String errorInfo) {
        mCT.hangupWithReason(connectionId, userUri, null, mpty, failCause, errorInfo);
    }

    public void sendDtmf(char c) {
        if (!PhoneNumberUtils.is12Key(c)) {
            Log.e(LOG_TAG, "sendDtmf called with invalid character '" + c + "'");
        } else {
            if (mCT.state == PhoneConstants.State.OFFHOOK) {
                mCi.sendDtmf(c, null);
            }
        }
    }

    public void startDtmf(char c) {
        if (!PhoneNumberUtils.is12Key(c)) {
            Log.e(LOG_TAG, "startDtmf called with invalid character '" + c
                    + "'");
        } else {
            mCi.startDtmf(c, null);
        }
    }

    public void stopDtmf() {
        mCi.stopDtmf(null);
    }

    public void sendBurstDtmf(String dtmfString, int on, int off,
            Message onComplete) {
        boolean check = true;
        for (int itr = 0; itr < dtmfString.length(); itr++) {
            if (!PhoneNumberUtils.is12Key(dtmfString.charAt(itr))) {
                Log.e(LOG_TAG,
                        "BurstDtmf invalid character '"
                                + dtmfString.charAt(itr) + "'");
                check = false;
                break;
            }
        }
        if ((mCT.state == PhoneConstants.State.OFFHOOK) && (check)) {
            mCi.sendBurstDtmf(dtmfString, on, off, onComplete);
        }
    }

    public void getOutgoingCallerIdDisplay(Message onComplete) {
        mCi.getCLIR(onComplete);
    }

    public void setOutgoingCallerIdDisplay(int commandInterfaceCLIRMode,
            Message onComplete) {
        mCi.setCLIR(commandInterfaceCLIRMode, onComplete);
    }

    public void getCallWaiting(Message onComplete) {
        //As per 3GPP TS 24.083, section 1.6 UE doesn't need to send service
        //class parameter in call waiting interrogation  to network
        mCi.queryCallWaiting(CommandsInterface.SERVICE_CLASS_NONE, onComplete);
    }

    public void setCallWaiting(boolean enable, Message onComplete) {
        mCi.setCallWaiting(enable, CommandsInterface.SERVICE_CLASS_VOICE, onComplete);
    }

    public void setCallWaitingForData(boolean enable, Message onComplete) {
        mCi.setCallWaiting(enable, CommandsInterface.SERVICE_CLASS_DATA_SYNC, onComplete);
    }

    public void getCallWaitingForData(Message onComplete) {
        mCi.queryCallWaiting(CommandsInterface.SERVICE_CLASS_DATA_SYNC, onComplete);
    }


    public void setCallForwardingOptionForData(int commandInterfaceCFReason,
            int commandInterfaceCFAction, String dialingNumber,
            int timerSeconds, Message onComplete) {
        if (isValidCommandInterfaceCFAction(commandInterfaceCFAction)
                && isValidCommandInterfaceCFReason(commandInterfaceCFReason)) {
            Log.d(LOG_TAG, "set call forwarding for data.");
            mCi.setCallForward(commandInterfaceCFAction, commandInterfaceCFReason,
                    CommandsInterface.SERVICE_CLASS_DATA_SYNC, dialingNumber,
                    timerSeconds, onComplete);
        }
    }

    public void getCallForwardingOptionForData(int commandInterfaceCFReason,
            Message onComplete) {
        if (isValidCommandInterfaceCFReason(commandInterfaceCFReason)) {
            Log.d(LOG_TAG, "requesting call forwarding query for data.");
            mCi.queryCallForwardStatus(commandInterfaceCFReason,
                    CommandsInterface.SERVICE_CLASS_DATA_SYNC, null, onComplete);
        }
    }

    public void setMute(boolean muted) {
        synchronized (ImsPhone.class) {
            mCT.setMute(muted);
        }
    }

    public boolean getMute() {
        return mCT.getMute();
    }

    public Call getForegroundCall() {
        return mCT.foregroundCall;
    }

    public Call getBackgroundCall() {
        return mCT.backgroundCall;
    }

    public Call getRingingCall() {
        return mCT.ringingCall;
    }

    public void changeConnectionType(Message msg, Connection conn,
            int newCallType, Map<String, String> newExtras) throws CallStateException {
        Log.d("ImsPhone", "changeConnectionType");
        ImsConnection imsConn = (ImsConnection) conn;
        imsConn.changeConnectionType(msg, newCallType, newExtras);
    }

    public void acceptConnectionTypeChange(Connection conn, Map<String, String> newExtras)
            throws CallStateException {
        ImsConnection imsConn = (ImsConnection) conn;
        imsConn.acceptConnectionTypeChange(newExtras);
    }

    public void rejectConnectionTypeChange(Connection conn) throws CallStateException {
        ImsConnection imsConn = (ImsConnection) conn;
        imsConn.rejectConnectionTypeChange();
    }

    public void deflectCall(int connId, String number, Message response) {
        mCT.deflectCall(connId, number, response);
    }

    /*
     * To check VT call capability
     */
    public boolean isVTModifyAllowed() {
        boolean isVideoCall = false, modifyAllowed = false;

        int callType = getCallType(getForegroundCall());
        if (callType == CallDetails.CALL_TYPE_UNKNOWN) {
            callType = getCallType(getBackgroundCall());
        }

        isVideoCall = ImsCallUtils.isVideoCall(callType);
        if (isVideoCall) {
            modifyAllowed = getIsServiceAllowed(CallDetails.CALL_TYPE_VOICE);
        } else if (callType == CallDetails.CALL_TYPE_VOICE) {
            modifyAllowed = getIsServiceAllowed(CallDetails.CALL_TYPE_VT);
            if (modifyAllowed) {
                modifyAllowed = android.provider.Settings.Secure.getInt(
                        getContext().getContentResolver(),
                        android.provider.Settings.Secure.PREFERRED_TTY_MODE,
                        Phone.TTY_MODE_OFF) == Phone.TTY_MODE_OFF;
            }
        } else {
            Log.e(LOG_TAG, "Modify Allowed cannot be found for the CallType " + callType);
        }

        Log.d(LOG_TAG, "Modify Allowed = " + modifyAllowed);
        return modifyAllowed;
    }

    public int getProposedConnectionType(Connection conn) throws CallStateException {
        ImsConnection imsConn = (ImsConnection) conn;
        return imsConn.getProposedConnectionType();
    }

    /**
     * When a remote user requests to change the type of the connection
     * (e.g. to upgrade from voice to video), it will be possible to
     * query the proposed type with this method. After receiving an indication of a request
     * (see {@link CallManager#registerForConnectionTypeChangeRequest(Handler, int, Object)}).
     *
     * If no request has been received, this function returns the current type.
     * The proposed type is cleared after calling {@link #acceptConnectionTypeChange(Map)} or
     * {@link #rejectConnectionTypeChange()}.
     *
     * @return The proposed connection type or the current connectionType if no request exists.
     */
    public boolean getProposedConnectionFailed(int connIndex) {
        ImsConnection imsConn = mCT.getConnectionByIndex(connIndex);
        if ( imsConn != null )
            return imsConn.getProposedConnectionFailed();
        return false;
    }

    private boolean isPSDomain() {
        return (SystemProperties.getBoolean(IMS_PS_DOMAIN, true));
    }

    public ServiceState getServiceState() {
        if (getIgnoreSerivceStateFlag()) {
            Log.d(LOG_TAG, "Faking IMS service state. The actual state is: "
                       + mServiceState);
            ServiceState state = new ServiceState();
            state.setState(ServiceState.STATE_IN_SERVICE);
            return state;
        } else {
            return mServiceState;
        }
    }

    public PhoneConstants.State getState() {
        return state;
    }

    public void setState(PhoneConstants.State newState) {
        state = newState;
    }

    private boolean isValidCommandInterfaceCFReason (int commandInterfaceCFReason) {
        switch (commandInterfaceCFReason) {
            case CF_REASON_UNCONDITIONAL:
            case CF_REASON_BUSY:
            case CF_REASON_NO_REPLY:
            case CF_REASON_NOT_REACHABLE:
            case CF_REASON_ALL:
            case CF_REASON_ALL_CONDITIONAL:
                return true;
            default:
                return false;
        }
    }

    private boolean isValidCommandInterfaceCFAction (int commandInterfaceCFAction) {
        switch (commandInterfaceCFAction) {
            case CF_ACTION_DISABLE:
            case CF_ACTION_ENABLE:
            case CF_ACTION_REGISTRATION:
            case CF_ACTION_ERASURE:
                return true;
            default:
                return false;
        }
    }

    protected  boolean isCfEnable(int action) {
        return (action == CF_ACTION_ENABLE) || (action == CF_ACTION_REGISTRATION);
    }

    @Override
    public int getPhoneType() {
        return RILConstants.IMS_PHONE;
    }

    public void notifyDisconnect(Connection cn) {
        if (DEBUG) Log.d(LOG_TAG, "Filter: notifyDisconnect");

        if (CsvtUtils.isCsvtConnection(cn)) {
            if (DEBUG) Log.d(LOG_TAG, "Csvt notification...");
            mCsvtDisconnectRegistrants.notifyResult(cn);
        } else {
            mDisconnectRegistrants.notifyResult(cn);
        }
    }

    public void notifyUnknownConnection() {
        if (DEBUG) Log.d(LOG_TAG, "Filter: notifyUnknownConnection");

        mUnknownConnectionRegistrants.notifyResult(this);
    }

    /* package */void notifyNewRingingConnection(Connection c) {
        if (DEBUG) Log.d(LOG_TAG, "Filter: notifyNewRingingConnection");

        if ( CsvtUtils.isCsvtConnection(c) ) {
            if ( DEBUG ) Log.d(LOG_TAG, "Csvt notification...");
            mCsvtNewRingingConnectionRegistrants.notifyRegistrants(
                    new AsyncResult(null, c, null) );
        } else {
            super.notifyNewRingingConnectionP(c);
        }
    }

    public DisconnectCause disconnectCauseFromCode(int causeCode) {

        /**
         * See 22.001 Annex F.4 for mapping of cause codes to local tones
         */
        switch (causeCode) {
            case CallFailCause.NO_CIRCUIT_AVAIL:
            case CallFailCause.TEMPORARY_FAILURE:
            case CallFailCause.SWITCHING_CONGESTION:
            case CallFailCause.CHANNEL_NOT_AVAIL:
            case CallFailCause.QOS_NOT_AVAIL:
            case CallFailCause.BEARER_NOT_AVAIL:
                return DisconnectCause.CONGESTION;
            case CallFailCause.ACM_LIMIT_EXCEEDED:
                return DisconnectCause.LIMIT_EXCEEDED;
            case CallFailCause.CALL_BARRED:
                return DisconnectCause.CALL_BARRED;
            case CallFailCause.FDN_BLOCKED:
                return DisconnectCause.FDN_BLOCKED;
            case CallFailCause.UNOBTAINABLE_NUMBER:
                return DisconnectCause.UNOBTAINABLE_NUMBER;
            case CallFailCause.USER_BUSY:
                return DisconnectCause.BUSY;
            case CallFailCause.ANSWERED_ELSEWHERE:
                return DisconnectCause.ANSWERED_ELSEWHERE;
            case CallFailCause.ERROR_UNSPECIFIED:
            case CallFailCause.NORMAL_CLEARING:
            default:
                int serviceState = getServiceState().getState();
                if (serviceState == ServiceState.STATE_POWER_OFF) {
                    return DisconnectCause.POWER_OFF;
                } else if (serviceState == ServiceState.STATE_OUT_OF_SERVICE
                        || serviceState == ServiceState.STATE_EMERGENCY_ONLY) {
                    return DisconnectCause.OUT_OF_SERVICE;
                } else if (causeCode == CallFailCause.NORMAL_CLEARING) {
                    return DisconnectCause.NORMAL;
                } else {
                    return DisconnectCause.ERROR_UNSPECIFIED;
                }
        }
    }

    public CellLocation getCellLocation() {
        logUnexpectedMethodCall("getCellLocation");
        return null;
    }

    public PhoneConstants.DataState getDataConnectionState(String apnType) {
        logUnexpectedMethodCall("getDataConnectionState");
        return null;
    }

    public DataActivityState getDataActivityState() {
        logUnexpectedMethodCall("getDataActivityState");
        return null;
    }

    public SignalStrength getSignalStrength() {
        logUnexpectedMethodCall("getSignalStrength");
        return null;
    }

    public List<? extends MmiCode> getPendingMmiCodes() {
        logUnexpectedMethodCall("getPendingMmiCodes");
        return null;
    }

    @Override
    public void registerForModifyCallRequest(Handler h, int what, Object obj)
            throws CallStateException {
        mCallModifyRegistrants.add(h, what, obj);
    }

    @Override
    public void unregisterForModifyCallRequest(Handler h) throws CallStateException {
        mCallModifyRegistrants.remove(h);
    }

    public void notifyModifyCallRequest(Connection c){
        AsyncResult ar = new AsyncResult(null, c, null);
        mCallModifyRegistrants.notifyRegistrants(ar);
    }

    @Override
    public void registerForAvpUpgradeFailure(Handler h, int what, Object obj)
            throws CallStateException {
        mAvpUpgradeFailureRegistrants.add(h, what, obj);
    }

    @Override
    public void unregisterForAvpUpgradeFailure(Handler h) throws CallStateException {
        mAvpUpgradeFailureRegistrants.remove(h);
    }

    public void notifyAvpUpgradeFailure(String errorStr){
        AsyncResult ar = new AsyncResult(null, errorStr, null);
        mAvpUpgradeFailureRegistrants.notifyRegistrants(ar);
    }

    @Override
    public void sendUssdResponse(String ussdMessge) {
        logUnexpectedMethodCall("sendUssdResponse");
    }

    @Override
    public void registerForSuppServiceNotification(Handler h, int what,
            Object obj) {
        mSsnRegistrants.addUnique(h, what, obj);
        if (mSsnRegistrants.size() == 1) mCi.setSuppServiceNotifications(true, null);
    }

    @Override
    public void unregisterForSuppServiceNotification(Handler h) {
        mSsnRegistrants.remove(h);
        if (mSsnRegistrants.size() == 0) mCi.setSuppServiceNotifications(false, null);
    }

    @Override
    public boolean handlePinMmi(String dialString) {
        logUnexpectedMethodCall("handlePinMmi");
        return false;
    }

    @Override
    public boolean handleInCallMmiCommands(String command)
            throws CallStateException {
        logUnexpectedMethodCall("handleInCallMmiCommands");
        return false;
    }

    @Override
    public void setRadioPower(boolean power) {
        logUnexpectedMethodCall("setRadioPower");
    }

    @Override
    public String getLine1Number() {
        logUnexpectedMethodCall("getLine1Number");
        return null;
    }

    @Override
    public String getLine1AlphaTag() {
        logUnexpectedMethodCall("getLine1AlphaTag");
        return null;
    }

    @Override
    public void setLine1Number(String alphaTag, String number,
            Message onComplete) {
        logUnexpectedMethodCall("setLine1Number");
    }

    @Override
    public String getVoiceMailNumber() {
        logUnexpectedMethodCall("getVoiceMailNumber");
        return null;
    }

    @Override
    public String getVoiceMailAlphaTag() {
        logUnexpectedMethodCall("getVoiceMailAlphaTag");
        return null;
    }

    @Override
    public void setVoiceMailNumber(String alphaTag, String voiceMailNumber,
            Message onComplete) {
        logUnexpectedMethodCall("setVoiceMailNumber");
    }

    @Override
    public void getCallForwardingOption(int commandInterfaceCFReason,
            Message onComplete) {
        if (isValidCommandInterfaceCFReason(commandInterfaceCFReason)) {
            Log.d(LOG_TAG, "requesting call forwarding query.");
            Message resp;
            if (commandInterfaceCFReason == CF_REASON_UNCONDITIONAL) {
                resp = obtainMessage(EVENT_GET_CALL_FORWARD_DONE, onComplete);
            } else {
                resp = onComplete;
            }
            mCi.queryCallForwardStatus(commandInterfaceCFReason,0,null,resp);
        }
    }

    @Override
    public void setCallForwardingOption(int commandInterfaceCFAction,
            int commandInterfaceCFReason,
            String dialingNumber,
            int timerSeconds,
            Message onComplete) {
        if ((isValidCommandInterfaceCFAction(commandInterfaceCFAction)) &&
                (isValidCommandInterfaceCFReason(commandInterfaceCFReason))) {

            Message resp;
            if (commandInterfaceCFReason == CF_REASON_UNCONDITIONAL) {
                Cfu cfu = new Cfu(dialingNumber, onComplete);
                resp = obtainMessage(EVENT_SET_CALL_FORWARD_DONE,
                        isCfEnable(commandInterfaceCFAction) ? 1 : 0, 0, cfu);
            } else {
                resp = onComplete;
            }
            mCi.setCallForward(commandInterfaceCFAction,
                    commandInterfaceCFReason,
                    CommandsInterface.SERVICE_CLASS_VOICE,
                    dialingNumber,
                    timerSeconds,
                    resp);
        }
    }

    @Override
    public void getAvailableNetworks(Message response) {
        logUnexpectedMethodCall("getAvailableNetworks");
    }

    @Override
    public void setNetworkSelectionModeAutomatic(Message response) {
        logUnexpectedMethodCall("setNetworkSelectionModeAutomatic");
    }

    @Override
    public void selectNetworkManually(OperatorInfo network, Message response) {
        logUnexpectedMethodCall("selectNetworkManually");
    }

    @Override
    public void getNeighboringCids(Message response) {
        logUnexpectedMethodCall("getNeighboringCids");
    }

    @Override
    public void setOnPostDialCharacter(Handler h, int what, Object obj) {
        logUnexpectedMethodCall("setOnPostDialCharacter");
    }

    @Override
    public void getDataCallList(Message response) {
        logUnexpectedMethodCall("getDataCallList");
    }

    @Override
    public void updateServiceLocation() {
        logUnexpectedMethodCall("updateServiceLocation");
    }

    public void enableLocationUpdates() {
        logUnexpectedMethodCall("enableLocationUpdates");
    }

    @Override
    public void disableLocationUpdates() {
        logUnexpectedMethodCall("disableLocationUpdates");
    }

    @Override
    public boolean getDataRoamingEnabled() {
        logUnexpectedMethodCall("getDataRoamingEnabled");
        return false;
    }

    @Override
    public void setDataRoamingEnabled(boolean enable) {
        logUnexpectedMethodCall("setDataRoamingEnabled");
    }

    @Override
    public String getDeviceId() {
        logUnexpectedMethodCall("getDeviceId");
        return null;
    }

    @Override
    public String getDeviceSvn() {
        logUnexpectedMethodCall("getDeviceSvn");
        return null;
    }

    @Override
    public String getSubscriberId() {
        IccRecords r = mIccRecords.get();
        return (r != null) ? r.getIMSI() : null;
    }

    @Override
    public String getEsn() {
        logUnexpectedMethodCall("getEsn");
        return null;
    }

    @Override
    public String getMeid() {
        logUnexpectedMethodCall("getMeid");
        return null;
    }

    @Override
    public String getImei() {
        logUnexpectedMethodCall("getImei");
        return null;
    }

    @Override
    public PhoneSubInfo getPhoneSubInfo() {
        logUnexpectedMethodCall("getPhoneSubInfo");
        return null;
    }

    @Override
    public IccPhoneBookInterfaceManager getIccPhoneBookInterfaceManager() {
        logUnexpectedMethodCall("getIccPhoneBookInterfaceManager");
        return null;
    }

    @Override
    public void activateCellBroadcastSms(int activate, Message response) {
        logUnexpectedMethodCall("activateCellBroadcastSms");
    }

    @Override
    public void getCellBroadcastSmsConfig(Message response) {
        logUnexpectedMethodCall("getCellBroadcastSmsConfig");
    }

    @Override
    public void setCellBroadcastSmsConfig(int[] configValuesArray,
            Message response) {
        logUnexpectedMethodCall("setCellBroadcastSmsConfig");
    }

    @Override
    protected void onUpdateIccAvailability() {
        logUnexpectedMethodCall("updateIccAvailability");
    }

    public Registrant getPostDialHandler() {
        logUnexpectedMethodCall("getPostDialHandler");
        return null;
    }

    /**
     * Common error logger method for unexpected calls to ImsPhone methods.
     */
    private void logUnexpectedMethodCall(String name) {
        Log.e(LOG_TAG, "Error! " + name + "() is not supported by "
                + getPhoneName());
    }

    /**
     * Removes the given MMI from the pending list and notifies
     * registrants that it is complete.
     * @param mmi MMI that is done
     */
    /*package*/ void
    onMMIDone(ImsMmiCode mmi) {
        /* Only notify complete if it's on the pending list.
         * Otherwise, it's already been handled (eg, previously canceled).
         * The exception is cancellation of an incoming USSD-REQUEST, which is
         * not on the list.
         */
        if (mPendingMMIs.remove(mmi) || mmi.isUssdRequest() || mmi.isSsInfo()) {
            mMmiCompleteRegistrants.notifyRegistrants(
                new AsyncResult(null, mmi, null));
        }
    }

    public boolean isInEmergencyCall() {
        return mCT.isInEmergencyCall();
    }

    public boolean isInEcm() {
        return mIsPhoneInEcmState;
    }

    void sendEmergencyCallbackModeChange(){
        //Send an Intent
        Intent intent = new Intent(TelephonyIntents.ACTION_EMERGENCY_CALLBACK_MODE_CHANGED);
        intent.putExtra(PhoneConstants.PHONE_IN_ECM_STATE, mIsPhoneInEcmState);
        intent.putExtra("ims_phone", true);
        ActivityManagerNative.broadcastStickyIntent(intent,null,UserHandle.USER_ALL);
        if (DEBUG) Log.d(LOG_TAG, "sendEmergencyCallbackModeChange");
    }

    @Override
    public void exitEmergencyCallbackMode() {
        if (mWakeLock.isHeld()) {
            mWakeLock.release();
        }
        // Send a message which will invoke handleExitEmergencyCallbackMode
        mCi.exitEmergencyCallbackMode(obtainMessage(EVENT_EXIT_EMERGENCY_CALLBACK_RESPONSE));
    }


    private void handleEnterEmergencyCallbackMode(Message msg) {
        if (DEBUG) {
            Log.d(LOG_TAG, "handleEnterEmergencyCallbackMode,mIsPhoneInEcmState= "
                    + mIsPhoneInEcmState);
        }
        // if phone is not in Ecm mode, and it's changed to Ecm mode
        if (mIsPhoneInEcmState == false) {
            mIsPhoneInEcmState = true;
            // notify change
            sendEmergencyCallbackModeChange();
            setSystemProperty(TelephonyProperties.PROPERTY_INECM_MODE, "true");

            // Post this runnable so we will automatically exit
            // if no one invokes exitEmergencyCallbackMode() directly.
            long delayInMillis = SystemProperties.getLong(
                    TelephonyProperties.PROPERTY_ECM_EXIT_TIMER, DEFAULT_ECM_EXIT_TIMER_VALUE);
            postDelayed(mExitEcmRunnable, delayInMillis);
            // We don't want to go to sleep while in Ecm
            mWakeLock.acquire();
        }
    }


    private void handleExitEmergencyCallbackMode(Message msg) {
        AsyncResult ar = (AsyncResult)msg.obj;
        if (DEBUG) {
            Log.d(LOG_TAG, "handleExitEmergencyCallbackMode,ar.exception , mIsPhoneInEcmState "
                    + ar.exception + mIsPhoneInEcmState);
        }
        // Remove pending exit Ecm runnable, if any
        removeCallbacks(mExitEcmRunnable);

        if (mEcmExitRespRegistrant != null) {
            mEcmExitRespRegistrant.notifyRegistrant(ar);
        }
        // if exiting ecm success
        if ((ar != null) && (ar.exception == null)) {
            if (mIsPhoneInEcmState) {
                mIsPhoneInEcmState = false;
                setSystemProperty(TelephonyProperties.PROPERTY_INECM_MODE, "false");
            }
            // send an Intent
            sendEmergencyCallbackModeChange();
        }
    }

    /**
     * Handle to cancel or restart Ecm timer in emergency call back mode
     * if action is CANCEL_ECM_TIMER, cancel Ecm timer and notify apps the timer is canceled;
     * otherwise, restart Ecm timer and notify apps the timer is restarted.
     */
    void handleTimerInEmergencyCallbackMode(int action) {
        switch(action) {
            case CANCEL_ECM_TIMER:
                removeCallbacks(mExitEcmRunnable);
                mEcmTimerResetRegistrants.notifyResult(Boolean.TRUE);
                break;
            case RESTART_ECM_TIMER:
                long delayInMillis = SystemProperties.getLong(
                        TelephonyProperties.PROPERTY_ECM_EXIT_TIMER, DEFAULT_ECM_EXIT_TIMER_VALUE);
                postDelayed(mExitEcmRunnable, delayInMillis);
                mEcmTimerResetRegistrants.notifyResult(Boolean.FALSE);
                break;
            default:
                Log.e(LOG_TAG, "handleTimerInEmergencyCallbackMode, unsupported action " + action);
        }
    }

    /**
     * Registration point for Ecm timer reset
     * @param h handler to notify
     * @param what User-defined message code
     * @param obj placed in Message.obj
     */
    public void registerForEcmTimerReset(Handler h, int what, Object obj) {
        mEcmTimerResetRegistrants.addUnique(h, what, obj);
    }

    public void unregisterForEcmTimerReset(Handler h) {
        mEcmTimerResetRegistrants.remove(h);
    }

    public void registerForServiceStatusChanged(Handler h, int what, Object obj) {
        mSrvStatusChangedRegistrants.add(h, what, obj);
    }

    public void unregisterForServiceStatusChanged(Handler h) {
        mSrvStatusChangedRegistrants.remove(h);
    }

    public void setOnEcbModeExitResponse(Handler h, int what, Object obj) {
        mEcmExitRespRegistrant = new Registrant (h, what, obj);
    }

    public void unsetOnEcbModeExitResponse(Handler h) {
        mEcmExitRespRegistrant.clear();
    }

    @Override
    public String getGroupIdLevel1() {
        // TODO Auto-generated method stub
        return null;
    }

    // Query for current video call quality.
    public void queryVideoQuality(Message msg) {
        Log.d(LOG_TAG, "queryVideoQuality");
        cm.queryVideoQuality(msg);
    }


     // Set for current video call quality.
    public void setVideoQuality(int quality, Message msg) {
        Log.d(LOG_TAG, "setVideoQuality quality=" + quality);
        cm.setVideoQuality(quality, msg);
    }

}
