/*
 *                     Location Service Reciever
 *
 * GENERAL DESCRIPTION
 *   This file is the receiver for the ACTION SHUTDOWN
 *
 * Copyright (c) 2013 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 * Copyright (c) 2012 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

package com.qualcomm.location;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class LocationServiceReceiver extends BroadcastReceiver {
    private static final String TAG = "LocationServiceReceiver";

    static {
        System.loadLibrary("locationservice");
    }

    private native void nativeShutdown();

    @Override
    public void onReceive(Context context, Intent intent) {
        String intentAction = intent.getAction();
        if (intentAction != null){
           if (intentAction.equals(Intent.ACTION_BOOT_COMPLETED)) {
                Intent i = new Intent(context, LBSSystemMonitorService.class);
                context.startService(i);

                Intent intentLocationService = new Intent(context, LocationService.class);
                intentLocationService.setAction("com.qualcomm.location.LocationService");
                context.startService(intentLocationService);
            }
        }
    }
}
