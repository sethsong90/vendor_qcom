/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
  Copyright (c) 2014 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
=============================================================================*/

/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
  Copyright (c) 2012 Qualcomm Atheros, Inc.
  All Rights Reserved.
  Qualcomm Atheros Confidential and Proprietary.
=============================================================================*/
package com.qualcomm.location.tel;

import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.content.ContentQueryMap;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.Cursor;
import android.telephony.TelephonyManager;
import android.telephony.MSimTelephonyManager;
import android.telephony.ServiceState;
import android.telephony.SignalStrength;
import android.telephony.CellLocation;
import android.telephony.CellInfo;
import android.telephony.PhoneStateListener;
import android.util.Log;
import android.provider.Settings;
import android.os.SystemProperties;
import com.android.internal.telephony.TelephonyProperties;

import java.util.List;
import java.util.Map;
import java.util.Observer;
import java.util.Observable;

public abstract class TelMgrProxy {
    static private final String TAG = "TelMgr";
    static private final boolean VERBOSE_DBG = Log.isLoggable(TAG, Log.VERBOSE);

    public int mListenerMask;
    public final TelProxyPhoneStateListener mPhoneListener;
    static public final int SingleSim = 0;
    static public final int MultiSim  = 1;
    private final String mHomeOperator;
    private TelephonyManager mTM;
    private static int mSimState;
    public final Context mContext;
    private static MSimTelMgr[] mMsTMs;
    private static SSimTelMgr mSsTm;
    private static TelMgrProxy mCurTm;

    private static final Runnable SIM_STATE_CHANGE_RECEIVER = new Runnable() {
        public void run() {
           checkForSimStateChange();
        }
    };

    static {
        SystemProperties.addChangeCallback(SIM_STATE_CHANGE_RECEIVER);
    }

    abstract boolean isNetworkRoaming();
    abstract String getNetworkOperator();
    abstract String getSubscriberId();
    abstract String getLine1Number();
    abstract int getSimState0();

    public void listen(int mask) {
        mCurTm.listen(mask);
        if (mCurTm instanceof MSimTelMgr) {
            if (mask != 0) {
                mSubSetting.addObserver(SUB_OBSERVER);
            } else {
                mSubSetting.deleteObserver(SUB_OBSERVER);
            }
        }
        mSimState = mCurTm.getSimState();
    }

    static private void logv(String s) {
        if (VERBOSE_DBG) Log.v(TAG, s);
    }

    public boolean getRoaming() {
        boolean roaming = mCurTm.isNetworkRoaming();
        logv("getRoaming() - "+roaming);
        return roaming;
    }

    public String getMncMccCombo() {
        String networkOperator = mCurTm.getNetworkOperator();
        logv("getMncMccCombo() - "+networkOperator +
             "; Sim Operator - "+mHomeOperator);
        return networkOperator;
    }

    public static int getMnc(String networkOperator) {
        int mnc = -1;
        if (networkOperator != null) {
            try {
                int mncmccCombo = Integer.parseInt(networkOperator);

                if (networkOperator.length() == 5) {
                    mnc = mncmccCombo % 100;
                }
                else if (networkOperator.length() == 6) {
                    mnc = mncmccCombo % 1000;
                }
            } catch (NumberFormatException nfe) {
                Log.w(TAG, "getNetworkOperator() - "+networkOperator);
            }
        }

        logv("getMnc() - "+mnc);
        return mnc;
    }

    public static int getMcc(String networkOperator) {
        int mcc = -1;
        if (networkOperator != null) {
            try {
                int mncmccCombo = Integer.parseInt(networkOperator);

                if (networkOperator.length() == 5) {
                    mcc = mncmccCombo / 100;
                }
                else if (networkOperator.length() == 6) {
                    mcc = mncmccCombo / 1000;
                }
            } catch (NumberFormatException nfe) {
                Log.w(TAG, "getNetworkOperator() - "+networkOperator);
            }
        }

        logv("getMcc() - "+mcc);
        return mcc;
    }

    public String getIMSI() {
        String imsi = mCurTm.getSubscriberId();
        logv("getIMSI() - "+imsi);
        return imsi;
    }

    public String getMSISDN() {
        String msisdn = mCurTm.getLine1Number();
        logv("getMSISDN() - "+msisdn);
        return msisdn;
    }

    public int getSimState() {
        int state = mCurTm.getSimState0();
        logv("getSimState() - " + state);
        return state;
    }

    public List<CellInfo> getAllCellInfo() {
        if(mCurTm instanceof MSimTelMgr) {
            MSimTelMgr mstm = (MSimTelMgr)mCurTm;
            return mstm.mTMgr.getAllCellInfo(mstm.mSub);
        }
        else {
            return mTM.getAllCellInfo();
        }
    }

    public String getHomeOperator() {
        return mTM.getSimOperator();
    }

    public int getSimConfig(TelMgrProxy tm) {
        if(tm instanceof MSimTelMgr) {
            return MultiSim;
        }
        else {
            return SingleSim;
        }
    }
    private static void onDdsUpdate(int dds) {
        logv("onDdsUpdate with "+dds);

        if (mCurTm instanceof MSimTelMgr) {
            MSimTelMgr mstm = (MSimTelMgr)mCurTm;

            if (dds < 0 || dds >= mMsTMs.length) {
                dds = mstm.mTMgr.getPreferredDataSubscription();
                logv("invalid dds, get from MSimTMgr as "+dds);
            }
            logv("Current DDS:"+mstm.mSub);
            // check if dds changed, but only if dds is valid
            if (dds >= 0 && dds < mMsTMs.length &&
                dds != mstm.mSub) {
                logv("DDS changed. Updating listener");
                // stop the current listener
                mstm.listen(0);
                // get the new subscription based TM
                mstm = mMsTMs[dds];
                // swap handle to the new subscription based TM
                mCurTm = mstm;
                // call onCellLocationChange here because the DDS change
                // affects the cells we are interested in listening. Here
                // we give the current CellLocation of the new DDS
                mstm.mPhoneListener.onDdsChange();
                mstm.mPhoneListener.onCellLocationChanged(mstm.mLastCellLocation);
                checkForSimStateChange();
                // start listening again but on the new subscription
                mstm.listen(mCurTm.mListenerMask);
            }
        } else {
            logv("but it is not a msim phone ???");
        }
    }

    /**
     * Checks if SIM state has been modidied and calls listener then.
     *
     * The method intercepts SystemProperty changes, as SIM state is
     * stored under TelephonyProperties#PROPERTY_SIM_STATE
     */
    private static void checkForSimStateChange() {
        TelMgrProxy tm = mCurTm;
        int newState = tm != null ? tm.getSimState() : TelephonyManager.SIM_STATE_UNKNOWN;
        if (newState == mSimState) {
            return;
        }
        mSimState = newState;
        if (tm != null) {
            tm.mPhoneListener.onSimStateChange();
        }
    }

    private final static Observer SUB_OBSERVER = new Observer() {
        @Override
        public void update(Observable o, Object arg) {
            logv("subObserver update");
            //int sub = MSimPhoneFactory.getDataSubscription();
            //onDdsUpdate(sub);
            Map<String, ContentValues> kvs = ((ContentQueryMap)o).getRows();
            if (null != kvs) {
                ContentValues subCV =
                    kvs.get(Settings.Global.MULTI_SIM_DATA_CALL_SUBSCRIPTION);
                if (subCV != null) {
                    try {
                        int sub = Integer.parseInt(subCV.toString());
                        onDdsUpdate(sub);
                    } catch (NumberFormatException nfe) {
                    }
                }
            }
        }
    };

    private static ContentQueryMap mSubSetting;

    public static TelMgrProxy get(Context c,
                                  TelProxyPhoneStateListener clientListener) {
        TelephonyManager tm = (TelephonyManager)c
            .getSystemService(Context.TELEPHONY_SERVICE);
        MSimTelephonyManager mstm = (MSimTelephonyManager)c
            .getSystemService(Context.MSIM_TELEPHONY_SERVICE);

        if ((mstm != null) && (mstm.isMultiSimEnabled())) {
            if (mMsTMs == null) {
                int numPhones = mstm.getPhoneCount();
                if (numPhones > 0) {
                    mMsTMs = new MSimTelMgr[numPhones];
                }
            }

            // mMsTMs could only remain null here when
            // mstm.getPhoneCount() returned 0 or less
            // which should account for some error.
            if (mMsTMs != null) {
                // we only need to compare one, since all are
                // created with the same parameters.
                if (mMsTMs[0] == null ||
                    !mMsTMs[0].mPhoneListener.equals(clientListener)) {

                    for (int i = 0; i < mMsTMs.length; i++) {
                        if (mMsTMs[i] != null && mMsTMs[i].mListenerMask != 0) {
                            // if this TM instance is listening something
                            // we stop it.
                            mMsTMs[i].listen(0);
                        }

                        mMsTMs[i] = new MSimTelMgr(c, i,
                                                   clientListener,
                                                   tm, mstm);
                    }
                }

                int dds = mstm.getPreferredDataSubscription();
                mCurTm = mMsTMs[dds];

                logv("TelMgrProxy.get() numPhones - "+mMsTMs.length+"; dds - "+dds);

                if (mSubSetting == null) {
                    // create the observer
                    ContentResolver resolver = c.getContentResolver();
                    Cursor subCursor = resolver.query(Settings.Global.CONTENT_URI,
                           new String[] {Settings.System.NAME, Settings.System.VALUE},
                           "(" + Settings.System.NAME + "=?)",
                           new String[]{Settings.Global.MULTI_SIM_DATA_CALL_SUBSCRIPTION},
                           null);

                    mSubSetting = new ContentQueryMap(subCursor, Settings.System.NAME,
                                                      true, null);
                }
            }
        } else {
            logv("TelMgrProxy.get(), not a msim device");

            if (mSsTm == null ||
                !mSsTm.mPhoneListener.equals(clientListener)) {
                if (mSsTm != null && mSsTm.mListenerMask != 0) {
                    mSsTm.listen(0);
                }
                mSsTm = new SSimTelMgr(c, clientListener, tm);
            }
            mCurTm = mSsTm;
        }

        return mCurTm;
    }

    private TelMgrProxy(Context c,
                        TelProxyPhoneStateListener clientListener,
                        TelephonyManager tm) {
        // keep the mask and listener for later comparisons
        mListenerMask = 0;
        mPhoneListener = clientListener;
        mHomeOperator = tm.getSimOperator();
        mContext = c;
        mTM = tm;
    }

    private static final class MSimTelMgr extends TelMgrProxy {
        private final MSimTelephonyManager mTMgr;
        private final int mSub;
        private CellLocation mLastCellLocation;
        // MSimTelephonyManager does not support querying on CellLocation
        // so we have to cash one in case of a dds switch.
        private final class MsimPhoneStateListener extends PhoneStateListener {
            @Override
            public void onCellLocationChanged(CellLocation location) {
                mLastCellLocation = location;
            }

            @Override
            public void onServiceStateChanged(ServiceState serviceState) {
                if (ServiceState.STATE_OUT_OF_SERVICE == serviceState.getState()) {
                    mLastCellLocation = null;
                }
            }

            @Override
            public void onDataConnectionStateChanged(int state, int networkType) {
                logv("onDataConnectionStateChanged state:"+state+" networkType:"+networkType);
                onDdsUpdate(-1);
            }

            public MsimPhoneStateListener(int sub) {
                super(sub);
                logv("New MsimPhoneStateListener with sub: "+sub);
            }
        };

        private MSimTelMgr(Context c, int index,
                           TelProxyPhoneStateListener clientListener,
                           TelephonyManager tm,
                           MSimTelephonyManager mstm) {
            super(c, clientListener, tm);
            mTMgr = mstm;
            mSub = index;
            mTMgr.listen((new MsimPhoneStateListener(mSub)),
                         PhoneStateListener.LISTEN_CELL_LOCATION |
                         PhoneStateListener.LISTEN_SERVICE_STATE |
                         PhoneStateListener.LISTEN_DATA_CONNECTION_STATE);
            tm.enableLocationUpdates();
        }

        int getSimState0() {
            return mTMgr.getSimState(mSub);
        }

        boolean isNetworkRoaming() {
            return mTMgr.isNetworkRoaming(mSub);
        }

        String getNetworkOperator() {
            return mTMgr.getNetworkOperator(mSub);
        }

        String getSubscriberId() {
            return mTMgr.getSubscriberId(mSub);
        }

        String getLine1Number() {
            return mTMgr.getLine1Number(mSub);
        }

        public void listen(int mask) {
            mListenerMask = mask;
            mPhoneListener.setSubscription(mSub);
            mTMgr.listen(mPhoneListener, mask);
        }

    }

    private static final class SSimTelMgr extends TelMgrProxy {
        private final TelephonyManager mTMgr;

        private SSimTelMgr(Context c,
                           TelProxyPhoneStateListener clientListener,
                           TelephonyManager tm) {
            super(c, clientListener, tm);
            mTMgr = tm;
            tm.enableLocationUpdates();
        }

        int getSimState0() {
            return mTMgr.getSimState();
        }

        boolean isNetworkRoaming() {
            return mTMgr.isNetworkRoaming();
        }

        String getNetworkOperator() {
            return mTMgr.getNetworkOperator();
        }

        String getSubscriberId() {
            return mTMgr.getSubscriberId();
        }

        String getLine1Number() {
            return mTMgr.getLine1Number();
        }

        public void listen(int mask) {
            mListenerMask = mask;
            mPhoneListener.setSubscription(0);
            mTMgr.listen(mPhoneListener, mask);
        }
    }

    public static class TelProxyPhoneStateListener extends PhoneStateListener
    {
        private void setSubscription(int sub) {
            mSubscription = sub;
        }

        public void onDdsChange() {}

        /**
         * Method is called whenever SIM state change detected.
         */
        public void onSimStateChange() {}
    }
}
