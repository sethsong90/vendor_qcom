/*===========================================================================
                           DigitalPenBroadcastReceiver.java

Copyright (c) 2013-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
==============================================================================*/
package com.qti.snapdragon.digitalpen;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class DigitalPenBroadcastReceiver extends BroadcastReceiver {

    private static final String TAG = "DigitalPenBroadcastReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.d(TAG, "Got something: " + intent);
        if (intent.getAction() == "com.qti.snapdragon.digitalpen.LOAD_CONFIG") {
            Intent serviceIntent = new Intent("com.qti.snapdragon.digitalpen.IDigitalPen");
            serviceIntent.putExtra("LoadConfigIntent", intent);
            context.startService(serviceIntent);
        }
    }

}
