/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
  Copyright (c) 2013-2014 Qualcomm Atheros, Inc.
  All Rights Reserved.
  Qualcomm Atheros Confidential and Proprietary.

  Copyright (C) 2009 Qualcomm Technologies, Inc All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
=============================================================================*/

package com.qualcomm.location;

import android.os.Bundle;
import android.os.Message;
import android.util.Log;

import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.location.LocationProvider;
import android.provider.Settings;

import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.WifiLock;

import android.content.Context;
import android.content.ContentQueryMap;
import android.content.ContentResolver;
import android.content.ContentValues;

import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;

import java.util.List;
import java.io.IOException;

import com.qualcomm.location.MonitorInterface.Monitor;

public class Wiper extends Monitor{
    private static final String TAG = "Wiper";
    private static final boolean VERBOSE_DBG = Log.isLoggable(TAG, Log.VERBOSE);

    // Handler messages
    private static final int WIPER_INIT_MSG = 0;
    private static final int WIPER_REQUEST_NETWORK_LOCATION_MSG = 1;
    private static final int WIPER_UPDATE_NETWORK_LOCATION_MSG = 2;
    private static final int WIPER_TRIGGER_FREE_WIFI_SCAN_INJECTION_MSG = 3;
    private static final int WIPER_REQUEST_AP_INFO_MSG = 4;
    private static final int WIPER_TRIGGER_WIFI_AP_INFO_INJECTION_MSG = 5;
    private static final int WIPER_UPDATE_PASSIVE_LOCATION_MSG = 6;
    private static final int MSG_MAX = 7;

    //wifi request types
    private static final int WIPER_START_PERIODIC_HI_FREQ_FIXES = 0;
    private static final int WIPER_START_PERIODIC_KEEP_WARM = 1;
    private static final int WIPER_STOP_PERIODIC_FIXES_V02 = 2;

    private static final int HIGH_FREQ_PERIOD = 3500; //3.5 seconds
    private static final int LOW_FREQ_PERIOD = 5000; //5 seconds
    private static final int LISTEN_WIFI_SCAN_RESULTS_MASK = 2;

    private final Context mContext;
    private LocationManager mLocMgr;
    private WifiManager mWifiMgr;
    private IntentFilter mScanResultIntentFilter;

    private boolean mIsNetworkLocationInSession;
    private boolean mFreeWifiScanEnabled = false;
    private boolean mIsWifiScanInSession = false;
    private int mListenerFlag = 0;

    private List<ScanResult> mResults;
    private boolean mWifiScanCompleted = false;

    private static final int MAX_APS_INFO_LIMIT = 50;
    private static final int MAC_ADDR_LENGTH = 6;
    private int mRSSI[] = new int[MAX_APS_INFO_LIMIT];
    private int mChannel[] = new int[MAX_APS_INFO_LIMIT];
    private byte mMacAddress[] = new byte[MAX_APS_INFO_LIMIT * MAC_ADDR_LENGTH];
    private int mNumApsUsed;
    private int mApInfoLen;

    public Wiper(MonitorInterface service, int msgIdBase, int listenerFlag) {
        super(service, msgIdBase);

        mContext = mMoniterService.getContext();
        mListenerFlag = listenerFlag;
        if(VERBOSE_DBG)
            Log.v(TAG, "Create Wiper");
        if(VERBOSE_DBG)
            Log.v(TAG, "Listener flag: " + mListenerFlag);

        mLocMgr = (LocationManager) mContext.getSystemService(Context.LOCATION_SERVICE);
        mWifiMgr = (WifiManager)mContext.getSystemService(mContext.WIFI_SERVICE);
        mScanResultIntentFilter = new IntentFilter(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION);

        mIsNetworkLocationInSession = false;
        mNumApsUsed = 0;

        mContext.registerReceiver(mWifiScanReceiver, mScanResultIntentFilter);
        init();

        //add passive provider request
        mLocMgr.requestLocationUpdates(LocationManager.PASSIVE_PROVIDER ,
                                       0,
                                       0, mPassiveLocationListener);
    }

    /*
     * Called from native code to request network location info
     */
    private void wifiRequestEvent(int type)
    {
        if (VERBOSE_DBG)
            Log.v(TAG, "wifiRequest type: "+ type);
        sendMessage(WIPER_REQUEST_NETWORK_LOCATION_MSG, type, 0, null);
    }

    /*
     * Called from native code to request wifi ap info
     */
    private void wifiApDataRequestEvent()
    {
        if (VERBOSE_DBG)
            Log.v(TAG,"wifiApDataRequestEvent");
        sendMessage( WIPER_REQUEST_AP_INFO_MSG, 0, 0, null);
    }

    private void handleNetworkLocationUpdate(Location location) {
        if(VERBOSE_DBG)
            Log.v(TAG, "handleNetworkLocationUpdate lat" +
                  location.getLatitude() + "lon" +
                  location.getLongitude() + "accuracy "
                  + location.getAccuracy());

        Boolean injectLocation = true;
        Bundle locationBundle = location.getExtras();

        if(locationBundle != null) {
            String locationSource = locationBundle.getString("com.qualcomm.location.nlp:source-technology");
            if(locationSource != null) {
                if(VERBOSE_DBG)
                    Log.v(TAG, "locationSource is "+locationSource);
                if(locationSource.equals("ZPP")) {
                    if(VERBOSE_DBG)
                        Log.v(TAG, "Setting injectLocation to false as provider is ZPP");
                    injectLocation = false;
                }
            }
            else
                if(VERBOSE_DBG)
                    Log.e(TAG, "locationSource is null");
        }
        else
            if(VERBOSE_DBG)
                Log.e(TAG, "Bundle is empty");

        if (location.hasAccuracy())
            if( mWifiScanCompleted && injectLocation) {
                native_wiper_send_network_location(1, location.getLatitude(),
                                                   location.getLongitude(),
                                                   location.getAccuracy(),
                                                   1, mMacAddress, mRSSI,
                                                   mChannel, mNumApsUsed, mApInfoLen);
            }
    }

    private void handlePassiveLocationUpdate(Location location) {
        if(VERBOSE_DBG)
            Log.v(TAG, "handlePassiveLocationUpdate lat" +
                  location.getLatitude() + "lon" +
                  location.getLongitude() + "accuracy "
                  + location.getAccuracy());


        Boolean injectLocation = true;
        Bundle locationBundle = location.getExtras();

        if(locationBundle != null) {
            if (locationBundle.containsKey("com.qualcomm.location.nlp:screen")) {
                injectLocation = false;
            }

            String locationSource =
                locationBundle.getString("com.qualcomm.location.nlp:source-technology");
            if(locationSource != null) {
                if(locationSource.equals("ZPP")) {
                    if(VERBOSE_DBG)
                        Log.v(TAG, "Setting passive injectLocation to false as provider is ZPP");
                    injectLocation = false;
                }
            }
        }
        else {
            if(VERBOSE_DBG)
                Log.v(TAG, "Bundle is empty");
        }

        if (location.hasAccuracy() && injectLocation)
            native_wiper_send_passive_location(1, location.getLatitude(),
                                               location.getLongitude(),
                                               location.getAccuracy());

    }

    private void handleFreeWifiScanInjection() {
        if(VERBOSE_DBG)
            Log.v(TAG, "handleFreeWifiScanInjection");
        native_wiper_send_network_location(0, 0, 0, 0, 1, mMacAddress, mRSSI,
                                           mChannel, mNumApsUsed, mApInfoLen);
    }

    private void handleApInfoInjection() {
        if(VERBOSE_DBG)
            Log.v(TAG, "handleApInfoInjection");
        native_wiper_send_wifi_ap_info(mMacAddress, mRSSI,
                                           mChannel, mApInfoLen);
    }

    private void init() {
        sendMessage(WIPER_INIT_MSG, 0, 0, null);
    }

    private void handleNativeNetworkLocationRequest(int type)
    {
        ContentResolver resolver;
        List<String> providers;
        boolean networkLocProvAvailable = false;
        boolean hasNetworkLocationProvider = false;

        resolver = mContext.getContentResolver();
        providers = mLocMgr.getAllProviders();
        networkLocProvAvailable = (providers.contains(LocationManager.NETWORK_PROVIDER) == true);

        if (networkLocProvAvailable &&
            Settings.Secure.isLocationProviderEnabled(resolver, LocationManager.NETWORK_PROVIDER)) {
            hasNetworkLocationProvider = true;
        } else {
            Log.e(TAG, "LocationManager.NETWORK_PROVIDER not enabled");
        }
        switch(type) {
        case WIPER_START_PERIODIC_HI_FREQ_FIXES:
            if(mIsNetworkLocationInSession == false && hasNetworkLocationProvider == true){
                if (VERBOSE_DBG)
                    Log.v(TAG, "request location updates with high frequency option");
                mLocMgr.requestLocationUpdates(LocationManager.NETWORK_PROVIDER ,
                                               HIGH_FREQ_PERIOD,
                                               0, mNetworkLocationListener);
                mIsNetworkLocationInSession = true;
            }
            break;
        case WIPER_START_PERIODIC_KEEP_WARM:
            if(mIsNetworkLocationInSession == false && hasNetworkLocationProvider == true){
                if (VERBOSE_DBG)
                    Log.v(TAG, "request location updates with low frequency option");
                mLocMgr.requestLocationUpdates(LocationManager.NETWORK_PROVIDER ,
                                               LOW_FREQ_PERIOD,
                                               0, mNetworkLocationListener);
                mIsNetworkLocationInSession = true;
            }
            break;
        case WIPER_STOP_PERIODIC_FIXES_V02:
            if(mIsNetworkLocationInSession = true){
                if (VERBOSE_DBG)
                    Log.v(TAG, "remove updates as stop message recieved");
                mLocMgr.removeUpdates(mNetworkLocationListener);
                mIsNetworkLocationInSession = false;
            }
            break;
        default:
            if(VERBOSE_DBG)
                Log.e(TAG, "Incorrect request sent in: "+type);
        }
    }

    private void handleNativeApInfoRequest()
    {
        if (VERBOSE_DBG)
            Log.v(TAG, "start wifi scan on ap info request from modem");

        if(mIsWifiScanInSession == false)
        {
            mWifiMgr.startScan();
            mIsWifiScanInSession = true;
        }

    }

    static private void logv(String s) {
        if (VERBOSE_DBG)
            Log.v(TAG, s);
    }

    public static native void native_wiper_send_network_location(
        int position_valid, double latitude, double longitude, float accuracy, int apinfo_valid,
        byte mac_array[], int rssi_array[],int channel_array[], int num_aps_used, int ap_len);
    public static native void native_wiper_send_passive_location(
        int position_valid, double latitude, double longitude, float accuracy);
    public static native void native_wiper_send_wifi_ap_info(
        byte mac_array[], int rssi_array[], int channel_array[], int ap_len);
    private native void native_wiper_init(int listener_mode);
    private static native void native_wiper_class_init();

    @Override
    public int getNumOfMessages() {
        return MSG_MAX;
    }

    @Override
    public void handleMessage(Message msg) {
        int message = msg.what;
        if (VERBOSE_DBG)
            Log.v(TAG, "handleMessage what - " + message);
        switch (message) {
        case WIPER_REQUEST_NETWORK_LOCATION_MSG:
            handleNativeNetworkLocationRequest(msg.arg1);
            break;
        case WIPER_UPDATE_NETWORK_LOCATION_MSG:
            handleNetworkLocationUpdate((Location)msg.obj);
            break;
        case WIPER_INIT_MSG:
            native_wiper_init(mListenerFlag);
            break;
        case WIPER_TRIGGER_FREE_WIFI_SCAN_INJECTION_MSG:
            handleFreeWifiScanInjection();
            break;
        case  WIPER_REQUEST_AP_INFO_MSG:
            handleNativeApInfoRequest();
            break;
        case WIPER_TRIGGER_WIFI_AP_INFO_INJECTION_MSG:
            handleApInfoInjection();
            break;
        case WIPER_UPDATE_PASSIVE_LOCATION_MSG:
            handlePassiveLocationUpdate((Location)msg.obj);
            break;
        default:
            if (VERBOSE_DBG)
                Log.v(TAG, "unknown message "+message);
        }
    }

    static {
        native_wiper_class_init();
    }

    /*
     * Request for network location info
     */
    private LocationListener mNetworkLocationListener = new LocationListener()
    {
        public void onLocationChanged(Location location) {
            if (VERBOSE_DBG)
                Log.v(TAG, "Active listener onLocationChanged lat" +
                      location.getLatitude()+ "lon" +
                      location.getLongitude() + "accuracy "
                      + location.getAccuracy());

            sendMessage(WIPER_UPDATE_NETWORK_LOCATION_MSG, 0, 0, location);

        }
        public void onStatusChanged(String arg0, int arg1, Bundle arg2) {
            if (VERBOSE_DBG)
                Log.v(TAG, "Status Changed" + arg0);
        }
        public void onProviderEnabled(String arg0) {
            if (VERBOSE_DBG)
                Log.v(TAG, "onProviderEnabled state " + arg0);
        }
        public void onProviderDisabled(String arg0) {
            if (VERBOSE_DBG)
                Log.v(TAG, "onProviderEnabled state " + arg0);
        }
    };


    private LocationListener mPassiveLocationListener = new LocationListener()
    {
        public void onLocationChanged(Location location) {
            if (VERBOSE_DBG)
                Log.v(TAG, "Passive listener onLocationChanged lat" +
                      location.getLatitude()+ "lon" +
                      location.getLongitude() + "accuracy "
                      + location.getAccuracy());
            if (LocationManager.NETWORK_PROVIDER.equals(location.getProvider())) {
                sendMessage(WIPER_UPDATE_PASSIVE_LOCATION_MSG, 0, 0, location);
            }

        }
        public void onStatusChanged(String arg0, int arg1, Bundle arg2) {
            if (VERBOSE_DBG)
                Log.v(TAG, "Status Changed" + arg0);
        }
        public void onProviderEnabled(String arg0) {
            if (VERBOSE_DBG)
                Log.v(TAG, "onProviderEnabled state " + arg0);
        }
        public void onProviderDisabled(String arg0) {
            if (VERBOSE_DBG)
                Log.v(TAG, "onProviderEnabled state " + arg0);
        }
    };

    /*
     * Receive wifi scan results
     */
    private BroadcastReceiver mWifiScanReceiver = new BroadcastReceiver()
    {
        @Override
        public void onReceive(Context context, Intent intent)
        {
            int i = 0;
            int j = 0;
            int k = 0;
            Integer val = 0;

            mResults = mWifiMgr.getScanResults();
            mWifiScanCompleted = (mResults != null);
            if (mWifiScanCompleted) {
                mNumApsUsed = mResults.size();
                if (VERBOSE_DBG)
                    Log.v(TAG,"mNumApsUsed is  "+mNumApsUsed);

                //clear the existing buffer
                for(i=0;i<MAX_APS_INFO_LIMIT;i++){
                    for(j=0;j< MAC_ADDR_LENGTH;j++)
                    {
                        mMacAddress[k] = 0;
                        k++;
                    }
                    mRSSI[i] = 0;
                    mChannel[i] = 0;
                }
                i = 0;
                k = 0;

                for (ScanResult result : mResults)
                {
                    if (VERBOSE_DBG)
                        Log.v(TAG,"WPS Scanner Result BSSID: " + result.BSSID +
                              " SSID: " + result.SSID + " RSSI: "+ result.level +
                              " Channel: "+ result.frequency);

                    if(i<MAX_APS_INFO_LIMIT){
                        String[] mBSSID = result.BSSID.split(":");
                        try {
                            for(j = 0;j<MAC_ADDR_LENGTH; j++,k++){
                                val = Integer.parseInt(mBSSID[j],16);
                                mMacAddress[k] = val.byteValue();
                            }
                            mRSSI[i] = result.level;
                            mChannel[i] = result.frequency;
                            i++;
                        } catch (NumberFormatException e) {
                            if(VERBOSE_DBG)
                                Log.e(TAG, "Unable to parse mac address");
                        }
                    }
                    else
                        break;
                }
                mApInfoLen = i;

                if((mListenerFlag & LISTEN_WIFI_SCAN_RESULTS_MASK) == LISTEN_WIFI_SCAN_RESULTS_MASK){
                    if (VERBOSE_DBG)
                        Log.v(TAG,"Triggering free wifi scan injection");
                    sendMessage(WIPER_TRIGGER_FREE_WIFI_SCAN_INJECTION_MSG, 0, 0, null);
                }

                if(mIsWifiScanInSession == true)
                {
                    if (VERBOSE_DBG)
                        Log.v(TAG,"Triggering wifi ap info injection");
                    sendMessage(WIPER_TRIGGER_WIFI_AP_INFO_INJECTION_MSG, 0, 0, null);
                    mIsWifiScanInSession = false;
                }
            }
        }
    };
}
