/**
 * Copyright (c) 2012, Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 * Developed by QRD Engineering team.
 */
package com.qualcomm.agent;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

public class PhoneProcessAgent extends Service {

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        logd(startId);
        super.onStartCommand(intent, flags, startId);
        if (intent == null) {
            stopSelf(startId);
            return START_STICKY;
        }
        if (Values.ACTION_PHONEPROCESS_AGENT.equals(intent.getAction())) {
            String para = intent.getStringExtra("para");
            doCommand(para);
        }
        stopSelf(startId);
        return START_STICKY;
    }
    
    private void doCommand(final String para) {
        logd(para);
        if (para == null)
            return;
        String[] paras = para.split(",");
        int len = paras.length;
        if (Values.GET_DEVICE_ID.equals(paras[0])) {
            for (int i = 0; i < len; i++)
                logd(i + ":" + paras[i]);
            String deviceId = AgentUtils.getDeviceId();
            Intent intent = new Intent(Values.AGENT_RESPONSE_ACTION);
            // getDeviceId,id
            intent.putExtra("response", Values.GET_DEVICE_ID + "," + deviceId);
            sendBroadcast(intent);
        }
    }

    @Override
    public IBinder onBind(Intent arg0) {
        // TODO Auto-generated method stub
        return null;
    }
    
    private static void logd(Object s) {
        
        Thread mThread = Thread.currentThread();
        StackTraceElement[] mStackTrace = mThread.getStackTrace();
        String mMethodName = mStackTrace[3].getMethodName();
        
        s = "[" + mMethodName + "] " + s;
        Log.d("SystemAgent", s + "");
    }

}
