/******************************************************************************
 * @file    EmbmsService.java
 *
 * Copyright (c) 2012-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 *
 *******************************************************************************/

package com.qualcomm.embms;

import java.nio.ByteBuffer;
import java.util.Arrays;

import android.app.Service;
import android.content.Intent;
import android.os.AsyncResult;
import android.os.IBinder;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.util.Log;

import com.qualcomm.embms.IEmbmsServiceCallback;
import com.qualcomm.qcrilhook.EmbmsOemHook;
import com.qualcomm.qcrilhook.EmbmsOemHook.*;
import com.qualcomm.embms.EmbmsSntpClient.*;

public class EmbmsService extends Service {
    private static final String LOG_TAG = "eMBMS  Service";
    private static final String VERSION = "00.04.22";
    private static final int SUCCESS = 0;
    private static final int ERROR_UNKNOWN = 1;
    private static final int ERROR_REGISTRATION_MISSING = 2;
    private static final int ERROR_INVALID_PARAMETER = 3;
    private static final int ERROR_SERVICE_NOT_READY = 4;
    private static final int INT_LENGTH = 4;

    private static final int EVENT_UNSOL = 1;
    private static final int EVENT_ENABLE_RESPONSE = 2;
    private static final int EVENT_ACTIVATE_RESPONSE = 3;
    private static final int EVENT_DEACTIVATE_RESPONSE = 4;
    private static final int EVENT_DISABLE_RESPONSE = 5;
    private static final int EVENT_ACTDEACTIVATE_RESPONSE = 6;
    private static final int EVENT_SIG_STRENGTH_RESPONSE = 7;
    private static final int EVENT_GET_TIME_RESPONSE = 8;
    private static final int EVENT_SIB16_COVERAGE_REQUEST = 9;
    private static final int EVENT_SIB16_COVERAGE_RESPONSE = 10;
    private static final int EVENT_EMBMS_SNTP_CLIENT_RESPONSE = 11;
    private static final int EVENT_SET_TIME_RESPONSE = 12;
    private static final int EVENT_ACTIVELOGPACKETIDS_RESPONSE = 13;

    final RemoteCallbackList<IEmbmsServiceCallback> mCallbacks
            = new RemoteCallbackList<IEmbmsServiceCallback>();

    private EmbmsOemHook mEmbmsOemHook;
    private Handler mHandler;
    private byte mCallId;
    private boolean mIsCallbackRegistered = false; // flag to indicate we have
                                                   // at least one callback registration
    private EmbmsSntpClient mSntpClient;
    private boolean mSetUtcTimeProperty;
    private boolean mIsEmbmsOemHookReady = false;

    //Delay in millisecond
    private static final int DELAY_SIB16_COVERAGE_REQUEST_MSEC = 100;
    //number of retries if  EmbmsService is not ready.
    private static final int NUM_OF_RETRIES_SIB16_COVERAGE_REQUEST = 3;
    private int mNumOfRetriesLeft;

    @Override
    public void onCreate() {
        super.onCreate();
        Log.i(LOG_TAG, "Service created. Version = " + VERSION);

        this.mCallId = 0;
        mSetUtcTimeProperty = SystemProperties.getBoolean("persist.radio.sib16_support", false);
        mNumOfRetriesLeft = NUM_OF_RETRIES_SIB16_COVERAGE_REQUEST;

        mEmbmsOemHook = EmbmsOemHook.getInstance(this);
        if (mEmbmsOemHook == null) {
            Log.e(LOG_TAG, "mEmbmsOemHook is null");
            return;
        } else {
            Log.d(LOG_TAG, "mEmbmsOemHook created successfully");
        }

        mSntpClient = EmbmsSntpClient.getInstance(this);
        if (mSntpClient == null) {
            Log.e(LOG_TAG, "mSntpClient is null");
        } else {
            Log.d(LOG_TAG, "mSntpClient created successfully");
        }

        mHandler = new Handler() {
            public void handleMessage(Message msg) {
                Log.i(LOG_TAG, "Message received: what = " + msg.what);
                switch (msg.what) {
                    case EVENT_UNSOL: {
                        Log.i(LOG_TAG, "Message  EVENT_UNSOL received");
                        UnsolObject obj = (UnsolObject) (((AsyncResult) msg.obj).result);
                        handleUnsol(obj);
                        break;
                    }
                    default:
                        handleSolicitedResponse(msg);
                        break;
                }
            }
        };

        mEmbmsOemHook.registerForNotifications(mHandler, EVENT_UNSOL, null);

    /**
     * If "persist.radio.sib16_support" property is set,
     * send a message to itself (EmbmsService) to getSib16CoverageStatus from RIL, when the service
     * is created for the first time.
     */
        if(mSetUtcTimeProperty) {
            Message msg = mHandler.obtainMessage(EVENT_SIB16_COVERAGE_REQUEST);
            msg.sendToTarget();
        }
    }

    private boolean isServiceReady() {
        synchronized (this) {
            if(!mIsEmbmsOemHookReady) {
                Log.i(LOG_TAG, "EmbmsService is not ready");
                return false;
            }
            return true;
        }
    }

    private int getSib16CoverageStatus() {
        int ret = SUCCESS;
        Log.i(LOG_TAG, "getSib16CoverageStatus called");

        Message msg = mHandler.obtainMessage(EVENT_SIB16_COVERAGE_RESPONSE);
        ret = mEmbmsOemHook.getSib16CoverageStatus(msg);

        Log.i(LOG_TAG, "getSib16CoverageStatus returned " + ret);
        return ret;
    }

    /**
     * This method will be called when the EmbmsSevice boots up. This method
     * will trigger the SNTP time retrieval from the EmbmsSntpClinet and then
     * syncing it periodically.
     */
    private void getSNTPTime() {
        Log.v(LOG_TAG, "getSNTPTime() called");
        Handler handler = mHandler;
        int what = EVENT_EMBMS_SNTP_CLIENT_RESPONSE;
        mSntpClient.startTimeReporting(handler, what);
    }

    /**
     * sends the following information to ril sntpSuccess true - SNTP time was
     * recieved successfully from the server false - SNTP time retrieval failed
     * timeMSeconds sntp time in milliseconds Long.MAX_VALUE if sntpSuccess is
     * false timeStamp System time at which sntp time retrieval was attempted
     *
     * @return SUCCESS 0 Action taken by eMBMS Service is successful
     */
    private int setTime(int success, long time, long sntpTimeStamp) {
        int ret = SUCCESS;
        boolean sntpSuccess;
        long timeMSeconds;
        long timeStamp;
        Log.i(LOG_TAG, "setTime called ");

        if (success == SUCCESS) {
            sntpSuccess = true;
            timeMSeconds = time;
        } else {
            sntpSuccess = false;
            timeMSeconds = Long.MAX_VALUE;
        }

        timeStamp = sntpTimeStamp;

        Message msg = mHandler.obtainMessage(EVENT_SET_TIME_RESPONSE);
        Log.i(LOG_TAG, "setTime : sntpSuccess = " + sntpSuccess + "timeMseconds = " + timeMSeconds
                + "timeStamp = " + timeStamp + "msg = " + msg);
        ret = mEmbmsOemHook.setTime(sntpSuccess, timeMSeconds, timeStamp, msg);
        Log.i(LOG_TAG, "setTime returned" + ret);

        return ret;
    }

    private boolean handleInternalResponse(Message msg) {
        switch(msg.what) {
            case EVENT_SIB16_COVERAGE_REQUEST: {
                Log.i(LOG_TAG, "Message  EVENT_SIB16_COVERAGE_REQUEST");
                if(!isServiceReady()) {
                    if(mNumOfRetriesLeft > 0) {
                        Message m = Message.obtain(msg);
                        mHandler.sendMessageDelayed(m, DELAY_SIB16_COVERAGE_REQUEST_MSEC);
                        mNumOfRetriesLeft--;
                    } else {
                        Log.e(LOG_TAG, "Failed to send sib16 coverage status request as" +
                                "EmbmsService is not ready ");
                    }
                } else {
                    mNumOfRetriesLeft = NUM_OF_RETRIES_SIB16_COVERAGE_REQUEST;
                    int ret = getSib16CoverageStatus();
                    Log.i(LOG_TAG, "getSib16CoverageStatus() ret = " + ret);
                }
                break;
            }
            case EVENT_SIB16_COVERAGE_RESPONSE: {
                Sib16Coverage response = (Sib16Coverage) msg.obj;
                Log.i(LOG_TAG, "Message EVENT_SIB16_COVERAGE_RESPONSE received:in Sib16 Coverage ="
                                    + response.inCoverage);
                if (!response.inCoverage) {
                    getSNTPTime();
                }
                break;
            }
            case EVENT_EMBMS_SNTP_CLIENT_RESPONSE: {
                SntpResponse response = (SntpResponse) msg.obj;
                Log.i(LOG_TAG, "Message  EVENT_EMBMS_SNTP_CLIENT_RESPONSE received: success= "
                                     + response.resp_sntpSuccess
                                     + "sntp time ms = " + response.resp_sntpTime
                                     + "sntp time stamp ms = " + response.resp_timeStamp);
                int ret = setTime(response.resp_sntpSuccess, response.resp_sntpTime,
                                     response.resp_timeStamp);
                Log.i(LOG_TAG, "setTime returned " + ret);
                break;
            }
            case EVENT_SET_TIME_RESPONSE: {
                int response = msg.arg1;
                Log.i(LOG_TAG, "Message  EVENT_SET_TIME_RESPONSE received: status= " + response);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    private void handleSolicitedResponse(Message msg) {
        //handles the internal EmbmsRequests; does not require a broadcast to the client
        if(handleInternalResponse(msg)) {
            return;
        }

        int i = mCallbacks.beginBroadcast();
        while (i > 0) {
            i--;
            try {
                //handles the requests that require a broadcast to the client
                switch (msg.what) {
                    case EVENT_ENABLE_RESPONSE: {
                        EnableResponse response = (EnableResponse) msg.obj;

                        Log.i(LOG_TAG, "Message  EVENT_ENABLE_RESPONSE received: status= "
                                + response.status);
                        if (response.status == SUCCESS) {
                            mCallId = response.callId;
                        }

                        mCallbacks.getBroadcastItem(i).enableResponse(response.traceId,
                                                                      response.code,
                                                                      response.interfaceName,
                                                                      response.ifIndex);
                        break;
                    }
                    case EVENT_DISABLE_RESPONSE: {
                        DisableResponse response = (DisableResponse) msg.obj;
                        Log.i(LOG_TAG, "Message  EVENT_DISABLE_RESPONSE received: status= "
                                + response.status);
                        if (response.status == SUCCESS) {
                            mCallId = 0;
                        }
                        mCallbacks.getBroadcastItem(i).disableResponse(response.traceId,
                                                                       response.code);
                        break;
                    }
                    case EVENT_ACTIVATE_RESPONSE: {
                        TmgiResponse response = (TmgiResponse) msg.obj;
                        Log.i(LOG_TAG, "Message  EVENT_ACTIVATE_RESPONSE received: status= "
                                + response.status);
                        mCallbacks.getBroadcastItem(i).activateTMGIResponse(response.traceId,
                                                                            response.code,
                                                                            response.tmgi);
                        break;
                    }
                    case EVENT_ACTDEACTIVATE_RESPONSE: {
                        ActDeactResponse response = (ActDeactResponse) msg.obj;
                        Log.i(LOG_TAG, "Message  EVENT_ACTDEACTIVATE_RESPONSE received: status= "
                                + response.status);
                        mCallbacks.getBroadcastItem(i).actDeactTMGIResponse(response.traceId,
                                                                            response.actCode,
                                                                            response.actTmgi,
                                                                            response.deactCode,
                                                                            response.deactTmgi);
                        break;
                    }
                    case EVENT_SIG_STRENGTH_RESPONSE: {
                        SigStrengthResponse response = (SigStrengthResponse) msg.obj;
                        Log.i(LOG_TAG, "Message  EVENT_SIG_STRENGTH_RESPONSE received: status= "
                                + response.status);
                        try {
                            mCallbacks.getBroadcastItem(i).signalStrengthResponse(response.traceId,
                                                                              response.code,
                                                                              response.mbsfnAreaId,
                                                                              response.snr,
                                                                              response.esnr,
                                                                              response.tmgiPerMbsfn,
                                                                              response.tmgilist);
                        } catch (RemoteException e) {
                            Log.e(LOG_TAG, "Exception in EVENT_SIG_STRENGTH_RESPONSE ");
                        }
                        break;
                    }
                    case EVENT_DEACTIVATE_RESPONSE: {
                        TmgiResponse response = (TmgiResponse) msg.obj;
                        Log.i(LOG_TAG, "Message  EVENT_DEACTIVATE_RESPONSE received: status= "
                                + response.status);
                        mCallbacks.getBroadcastItem(i).deactivateTMGIResponse(response.traceId,
                                                                              response.code,
                                                                              response.tmgi);
                        break;
                    }
                    case EVENT_GET_TIME_RESPONSE: {
                        TimeResponse response = (TimeResponse) msg.obj;
                        Log.i(LOG_TAG, "Message  EVENT_TIME_RESPONSE received: status= "
                                + response.status);
                        mCallbacks.getBroadcastItem(i).timeResponse(response.traceId,
                                                                    response.code,
                                                                    response.timeMseconds,
                                                                    response.additionalInfo,
                                                                    response.dayLightSaving,
                                                                    response.leapSeconds,
                                                                    response.localTimeOffset);
                        break;
                    }
                    case EVENT_ACTIVELOGPACKETIDS_RESPONSE: {
                        ActiveLogPacketIDsResponse response = (ActiveLogPacketIDsResponse) msg.obj;
                        Log.i(LOG_TAG, "Message  EVENT_ACTIVELOGPACKETIDS_RESPONSE received:" +
                                " status= "+ response.status);
                        mCallbacks.getBroadcastItem(i).activeLogPacketIDsResponse(response.traceId,
                                response.activePacketIdList);
                        break;
                    }
                    default:
                        Log.i(LOG_TAG, "Invalid Message received " + msg.what);
                        break;
                }
            } catch (RemoteException e) {
                Log.e(LOG_TAG, "Remote Exception occurred during solicited broadcast");
            }
        }
        mCallbacks.finishBroadcast();
    }

    private boolean handleInternalUnsol(UnsolObject obj) {
        boolean isHandled = false;
        if (obj.unsolId == EmbmsOemHook.UNSOL_TYPE_SIB16_COVERAGE) {
            isHandled = true;
            Log.i(LOG_TAG, "Sib16 Coverage unsol received");
            Sib16Coverage sib16 = (Sib16Coverage) obj.obj;
            Log.i(LOG_TAG, "Sib16 Coverage unsol = " + sib16.inCoverage);
            if (!sib16.inCoverage) {
                getSNTPTime();
            } else {
                Log.i(LOG_TAG, "Sib16 Coverage unsol : " + sib16.inCoverage);
                mSntpClient.stopTimeReporting();
            }
        }
        return isHandled;
    }

    private void handleUnsol(UnsolObject obj) {

        if(obj.unsolId == EmbmsOemHook.UNSOL_TYPE_EMBMSOEMHOOK_READY_CALLBACK) {
            Log.i(LOG_TAG, "QcRilHook Ready Callback received");
            synchronized(this) {
                mIsEmbmsOemHookReady = (Boolean) obj.obj;
                Log.i(LOG_TAG, "mIsEmbmsOemHookReady = " + mIsEmbmsOemHookReady);
                return;
            }
        }

        //handles internal EmbmsUnsols; does not require broadcast to clients.
        if(handleInternalUnsol(obj)) {
            return;
        }

        int i = mCallbacks.beginBroadcast();
        while (i > 0) {
            i--;
            try {
                //handles unsols that require broadcast to client
                switch (obj.unsolId) {
                    case EmbmsOemHook.UNSOL_TYPE_STATE_CHANGE:
                        Log.i(LOG_TAG, "State change unsol received");
                        StateChangeInfo info = (StateChangeInfo) obj.obj;
                        break;
                    case EmbmsOemHook.UNSOL_TYPE_ACTIVE_TMGI_LIST:
                        Log.i(LOG_TAG, "Active TMGI list unsol received");
                        TmgiListIndication list = (TmgiListIndication) obj.obj;
                        mCallbacks.getBroadcastItem(i).activeTMGIListNotification(list.traceId,
                                                                                  list.list);
                        break;
                    case EmbmsOemHook.UNSOL_TYPE_BROADCAST_COVERAGE:
                        Log.i(LOG_TAG, "Broadcast coverage unsol received");
                        CoverageState cs = (CoverageState) obj.obj;
                        mCallbacks.getBroadcastItem(i).broadcastCoverageNotification(cs.traceId,
                                                                                     cs.state);
                        break;
                    case EmbmsOemHook.UNSOL_TYPE_AVAILABLE_TMGI_LIST:
                        Log.i(LOG_TAG, "Available TMGI list unsol received");
                        TmgiListIndication avList = (TmgiListIndication) obj.obj;
                        mCallbacks.getBroadcastItem(i).availableTMGIListNotification(avList.traceId,
                                                                                     avList.list);
                        break;
                    case EmbmsOemHook.UNSOL_TYPE_OOS_STATE:
                        Log.i(LOG_TAG, "OOS state unsol received");
                        OosState os = (OosState) obj.obj;
                        mCallbacks.getBroadcastItem(i).oosNotification(os.traceId, os.state,
                                                                       os.list);
                        break;
                    case EmbmsOemHook.UNSOL_TYPE_CELL_ID:
                        Log.i(LOG_TAG, "Cell Id unsol received");
                        CellIdIndication ciInd = (CellIdIndication) obj.obj;
                        mCallbacks.getBroadcastItem(i).cellGlobalIdNotification(ciInd.traceId,
                                                                                ciInd.mcc,
                                                                                ciInd.mnc,
                                                                                ciInd.id);
                        break;
                    case EmbmsOemHook.UNSOL_TYPE_SAI_LIST:
                        Log.i(LOG_TAG, "Sai List unsol received");
                        SaiIndication saiInd = (SaiIndication)obj.obj;
                        mCallbacks.getBroadcastItem(i).saiNotification(saiInd.traceId,
                                saiInd.campedSaiList, saiInd.numSaiPerGroupList,
                                saiInd.availableSaiList);
                        break;
                    case EmbmsOemHook.UNSOL_TYPE_RADIO_STATE:
                        Log.i(LOG_TAG, "Radio state unsol received");
                        RadioStateIndication rs = (RadioStateIndication) obj.obj;
                        mCallbacks.getBroadcastItem(i).radioStateNotification(rs.traceId, rs.state);
                        break;

                    default:
                        Log.e(LOG_TAG, "Invalid Unsol ID received " + obj.unsolId);
                        break;
                }
            } catch (RemoteException e) {
                Log.e(LOG_TAG, "Remote Exception occurred during unsolicited broadcast");
            }
        }
        mCallbacks.finishBroadcast();
    }

    @Override
    public void onDestroy() {
        mCallbacks.kill();
        mEmbmsOemHook.unregisterForNotifications(mHandler);
        mEmbmsOemHook.dispose();
    }

    public IBinder onBind(Intent intent) {
        Log.i(LOG_TAG, "Service bound");
        return mBinder;
    }

    /*
     * Implement the methods of the IEmbmsService interface in this stub. Please
     * see IEmbmsService.aidl and IEmbmsServiceCallbacks.aidl for documentation.
     */
    private final IEmbmsService.Stub mBinder = new IEmbmsService.Stub() {

        private boolean isRegistered() {
            synchronized (this) {
                if (!mIsCallbackRegistered) {
                    Log.i(LOG_TAG, "Error! No callback registered");
                    return false;
                }
                return true;
            }
        }

        public String getVersion(int debugTraceId) {
            return VERSION;
        }

        public int enable(int debugTraceId) {
            int ret = SUCCESS;
            Log.i(LOG_TAG, "enable called ");

            if (!isRegistered()) {
                return ERROR_REGISTRATION_MISSING;
            }

            if(!isServiceReady()) {
                return ERROR_SERVICE_NOT_READY;
            }

            Message msg = mHandler.obtainMessage(EVENT_ENABLE_RESPONSE);
            ret = mEmbmsOemHook.enable(debugTraceId, msg);
            Log.i(LOG_TAG, "mEmbmsOemHook.enable returned " + ret);

            return ret;
        }

        public int disable(int debugTraceId) {
            int ret = SUCCESS;
            Log.i(LOG_TAG, "disable called ");

            if (!isRegistered()) {
                return ERROR_REGISTRATION_MISSING;
            }

            if(!isServiceReady()) {
                return ERROR_SERVICE_NOT_READY;
            }

            Message msg = mHandler.obtainMessage(EVENT_DISABLE_RESPONSE);
            ret = mEmbmsOemHook.disable(debugTraceId, mCallId, msg);
            Log.i(LOG_TAG, "mEmbmsOemHook.disable returned " + ret);

            return ret;
        }

        public int registerCallback(int debugTraceId, IEmbmsServiceCallback cb) {
            boolean ret = false;

            if (cb == null) {
                Log.e(LOG_TAG, "Null object passed to register");
                return ERROR_UNKNOWN;
            }

            synchronized (this) {
                ret = mCallbacks.register(cb);

                if (ret == true) {
                    mIsCallbackRegistered = true;
                    return SUCCESS;
                } else {
                    Log.e(LOG_TAG, "register failed");
                    return ERROR_UNKNOWN;
                }
            }
        }

        public int deregisterCallback(int debugTraceId, IEmbmsServiceCallback cb) {
            boolean ret = false;

            if (cb == null) {
                Log.e(LOG_TAG, "Null object passed to de-register");
                return ERROR_UNKNOWN;
            }

            synchronized (this) {
                ret = mCallbacks.unregister(cb);

                if (ret == true) {
                    mIsCallbackRegistered = false;
                    return SUCCESS;
                } else {
                    Log.e(LOG_TAG, "unregister failed");
                    return ERROR_UNKNOWN;
                }
            }
        }

        public int activateTMGI(int traceId, byte[] tmgi, int priority, int[] saiList,
                int[] earFcnList) {
            int ret = SUCCESS;
            Log.i(LOG_TAG, "activateTMGI received: traceId = " + traceId
                        + "tmgi = " + Arrays.toString(tmgi)
                        + "priority = " + priority
                        + "saiList = " + Arrays.toString(saiList)
                        + "earFcnList = " + Arrays.toString(earFcnList));

            if (tmgi == null || tmgi.length == 0) {
                Log.e(LOG_TAG, "Error! invalid parameter" + " " + tmgi);
                return ERROR_INVALID_PARAMETER;
            }

            if (!isRegistered()) {
                return ERROR_REGISTRATION_MISSING;
            }

            if(!isServiceReady()) {
                return ERROR_SERVICE_NOT_READY;
            }

            Message msg = mHandler.obtainMessage(EVENT_ACTIVATE_RESPONSE);
            ret = mEmbmsOemHook.activateTmgi(traceId, mCallId, tmgi, priority, saiList, earFcnList,
                    msg);
            Log.i(LOG_TAG, "activateTMGI returned " + ret);

            return ret;
        }

        public int deactivateTMGI(int traceId, byte[] tmgi) {
            int ret = SUCCESS;
            Log.i(LOG_TAG, "deactivateTmgi called ");

            if (tmgi == null || tmgi.length == 0) {
                Log.e(LOG_TAG, "Error! invalid parameter" + " " + tmgi);
                return ERROR_INVALID_PARAMETER;
            }

            if (!isRegistered()) {
                return ERROR_REGISTRATION_MISSING;
            }

            if(!isServiceReady()) {
                return ERROR_SERVICE_NOT_READY;
            }

            Message msg = mHandler.obtainMessage(EVENT_DEACTIVATE_RESPONSE);
            ret = mEmbmsOemHook.deactivateTmgi(traceId, mCallId, tmgi, msg);
            Log.i(LOG_TAG, "deactivateTmgi returned " + ret);

            return ret;
        }

        public int actDeactTMGI(int traceId, byte[] actTMGI, int priority, int[] saiList,
                int[] earfcnList, byte[] deactTMGI) {
            int ret = SUCCESS;
            Log.i(LOG_TAG, "actDeact TMGI received: traceId = " + traceId
                        + "act tmgi = " + Arrays.toString(actTMGI)
                        + "priority = " + priority
                        + "saiList = " + Arrays.toString(saiList)
                        + "earFcnList = " + Arrays.toString(earfcnList)
                        + "deact tmgi = " + Arrays.toString(deactTMGI));

            if (actTMGI == null || actTMGI.length == 0 || deactTMGI == null
                    || deactTMGI.length == 0) {
                Log.e(LOG_TAG, "Error! invalid parameter" + " " + actTMGI + " " + " " + deactTMGI);
                return ERROR_INVALID_PARAMETER;
            }

            if (!isRegistered()) {
                return ERROR_REGISTRATION_MISSING;
            }

            if(!isServiceReady()) {
                return ERROR_SERVICE_NOT_READY;
            }

            Message msg = mHandler.obtainMessage(EVENT_ACTDEACTIVATE_RESPONSE);
            ret = mEmbmsOemHook.actDeactTmgi(traceId, mCallId, actTMGI, deactTMGI, priority,
                    saiList, earfcnList, msg);
            Log.i(LOG_TAG, "actDeactTMGI returned " + ret);

            return SUCCESS;
        }

        public int getAvailableTMGIList(int debugTraceId) {
            int ret = SUCCESS;
            Log.i(LOG_TAG, "getAvailableTMGIList called");

            if (!isRegistered()) {
                return ERROR_REGISTRATION_MISSING;
            }

            if(!isServiceReady()) {
                return ERROR_SERVICE_NOT_READY;
            }

            ret = mEmbmsOemHook.getAvailableTMGIList(debugTraceId, mCallId);
            Log.i(LOG_TAG, "getAvailableTMGIList returned " + ret);

            return SUCCESS;
        }

        public int getActiveTMGIList(int debugTraceId) {
            int ret = SUCCESS;
            Log.i(LOG_TAG, "getActiveTMGIList called");

            if (!isRegistered()) {
                return ERROR_REGISTRATION_MISSING;
            }

            if(!isServiceReady()) {
                return ERROR_SERVICE_NOT_READY;
            }

            ret = mEmbmsOemHook.getActiveTMGIList(debugTraceId, mCallId);
            Log.i(LOG_TAG, "getActiveTMGIList returned " + ret);

            return SUCCESS;
        }

        public int getCoverageState(int debugTraceId) {
            int ret = SUCCESS;
            Log.i(LOG_TAG, "getCoverageState called");

            if (!isRegistered()) {
                return ERROR_REGISTRATION_MISSING;
            }

            if(!isServiceReady()) {
                return ERROR_SERVICE_NOT_READY;
            }

            ret = mEmbmsOemHook.getCoverageState(debugTraceId);
            Log.i(LOG_TAG, "getCoverageState returned " + ret);

            return SUCCESS;
        }

        public int getSignalStrength(int debugTraceId) {
            int ret = SUCCESS;
            Log.i(LOG_TAG, "getSignalStrength called");

            if (!isRegistered()) {
                return ERROR_REGISTRATION_MISSING;
            }

            if(!isServiceReady()) {
                return ERROR_SERVICE_NOT_READY;
            }

            Message msg = mHandler.obtainMessage(EVENT_SIG_STRENGTH_RESPONSE);
            ret = mEmbmsOemHook.getSignalStrength(debugTraceId, msg);
            Log.i(LOG_TAG, "getSignalStrength returned " + ret);

            return ret;
        }

        public int getActiveLogPacketIDs(int debugTraceId, int[] supportedLogPacketIdList) {
            int ret = SUCCESS;
            Log.i(LOG_TAG, "getActiveLogPacketIDs supportedLogPacketIdList received: = " +
                    Arrays.toString(supportedLogPacketIdList));
            if (supportedLogPacketIdList == null || supportedLogPacketIdList.length == 0) {
                // If supportedLogPacketIdList is null then send defult log packet ID list.
                int[] defaultsupportedLogPacketIdList = {0,1,2};
                supportedLogPacketIdList = defaultsupportedLogPacketIdList;
            }

            if (!isRegistered()) {
                return ERROR_REGISTRATION_MISSING;
            }

            if(!isServiceReady()) {
                return ERROR_SERVICE_NOT_READY;
            }

            Message msg = mHandler.obtainMessage(EVENT_ACTIVELOGPACKETIDS_RESPONSE);
            ret = mEmbmsOemHook.getActiveLogPacketIDs(debugTraceId, supportedLogPacketIdList, msg);

            Log.i(LOG_TAG, "getActiveLogPacketIDs returned " + ret);

            return ret;
        }

        public int deliverLogPacket(int debugTraceId, int logPacketId, byte[] logPacket) {
            int ret = SUCCESS;
            Log.i(LOG_TAG, "deliverLogPacket received: traceId = " + debugTraceId +
                    "logPacketId = " + logPacketId + "logPacket = " + Arrays.toString(logPacket));

            if (!isRegistered()) {
                return ERROR_REGISTRATION_MISSING;
            }

            if(!isServiceReady()) {
                return ERROR_SERVICE_NOT_READY;
            }

            ret = mEmbmsOemHook.deliverLogPacket(debugTraceId, logPacketId, logPacket);
            Log.i(LOG_TAG, "deliverLogPacket returned " + ret);

            return ret;
        }

        public int getTime(int debugTraceId) {
            int ret = SUCCESS;
            Log.i(LOG_TAG, "getTime called");

            if (!isRegistered()) {
                return ERROR_REGISTRATION_MISSING;
                }

            if(!isServiceReady()) {
                return ERROR_SERVICE_NOT_READY;
            }

            Message msg = mHandler.obtainMessage(EVENT_GET_TIME_RESPONSE);
            ret = mEmbmsOemHook.getTime(debugTraceId, msg);
            Log.i(LOG_TAG, "getTime returned " + ret);

            return ret;
        }
    };
}
