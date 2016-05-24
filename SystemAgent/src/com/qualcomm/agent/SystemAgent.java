/**
 * Copyright (c) 2012-2013, Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 * Developed by QRD Engineering team.
 */
package com.qualcomm.agent;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;
import android.os.storage.StorageManager;
import android.content.Context;
import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.os.Environment;
import android.util.Log;

public class SystemAgent extends Service {
    private static String filePath = Environment.getExternalStorageDirectory() + "/screenshot.png";
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        logd(startId);
        super.onStartCommand(intent, flags, startId);
        if (intent == null) {
            stopSelf(startId);
            return START_STICKY;
        }
        if (Values.ACTION_AGENT.equals(intent.getAction())) {
            String para = intent.getStringExtra("para");
            doSystemActions(para);
        } else if (Values.ACTION_FULL_AGENT.equals(intent.getAction())) {
            String para = intent.getStringExtra("para");
            exec(para);
        }
        // stopSelf(startId);
        return START_STICKY;
    }

    @Override
    public void onCreate() {
        String meta = AgentUtils.readFileAgent(Values.META_INFO_FILE);
        int retry = 20;
        if (meta != null) {
            do {
                try {
                    Thread.sleep(50);
                } catch (InterruptedException e) {
                    logd(e);
                }
            } while (!AgentUtils.setSystemProperties(Values.META_ID_PROPERTY,meta) && retry-- > 0);
        }
        super.onCreate();
        logd("RUN");
    }

    /**
     * @param para
     *            is a string which consists of commands divided by ","
     */
    private void doSystemActions(final String para) {
        logd(para);
        if (para == null)
            return;
        String[] paras = para.split(",");
        int len = paras.length;
        if (Values.SET_SYSTEM_PROPERTIES.equals(paras[0])) {
            for (int i = 0; i < len; i++)
                logd(i + ":" + paras[i]);
            String key = paras[1];
            String val = paras[2];
            AgentUtils.setSystemProperties(key, val);
            
        } else if (Values.GET_SYSTEM_PROPERTIES.equals(paras[0])) {
            for (int i = 0; i < len; i++)
                logd(i + ":" + paras[i]);
            String key = paras[1];
            String defaultValue = paras[2];
            String property = AgentUtils.getSystemProperties(key, defaultValue);
            Intent intent = new Intent(Values.AGENT_RESPONSE_ACTION);
            // getDeviceId,id
            intent.putExtra("response", Values.GET_SYSTEM_PROPERTIES + "," + property);
            sendBroadcast(intent);

        } else if (Values.WRITE_SYSTEM_FILES.equals(paras[0])) {
            for (int i = 0; i < len; i++)
                logd(i + ":" + paras[i]);
            // adb shell am startservice -a "android.system.agent" --es "para"
            // writeSystemFiles,/cache/recovery/command,wipe_data\\n--update_package=/sdcard/update.zip
            // Use adb command \n=>\\n; Use code \n not change
            String filePath = paras[1];
            String content = paras[2];
            AgentUtils.writeFileAgent(filePath, content);
        } else if (Values.TAKE_SCREENSHOT.equals(paras[0])) {
            for (int i = 0; i < len; i++)
                logd(i + ":" + paras[i]);
            if (paras.length > 1)
                filePath = paras[1];
            AgentUtils.takeScreenshot(getApplicationContext(), filePath);
        } else if (Values.REBOOT.equals(paras[0])) {
            for (int i = 0; i < len; i++)
                logd(i + ":" + paras[i]);
            String reason = null;
            if (len <= 1)
                reason = null;
            else
                reason = paras[1];
            AgentUtils.reboot(getApplicationContext(), reason);
        }
    }

    void exec(final String para) {
        // adb shell am startservice -a "android.system.fullagent" --es "para"
        // setprop,persist.debug.test,5
        new Thread() {
            
            public void run() {
                try {
                    logd(para);

                    Process mProcess;
                    String paras[] = para.split(",");
                    for (int i = 0; i < paras.length; i++)
                        logd(i + ":" + paras[i]);
                    mProcess = Runtime.getRuntime().exec(paras);
                    mProcess.waitFor();
                    
                    InputStream inStream = mProcess.getInputStream();
                    InputStreamReader inReader = new InputStreamReader(inStream);
                    BufferedReader inBuffer = new BufferedReader(inReader);
                    String s;
                    String data = "";
                    while ((s = inBuffer.readLine()) != null) {
                        data += s + "\n";
                    }
                    logd(data);
                    
                    int result = mProcess.exitValue();
                    
                    logd("ExitValue=" + result);
                    String resultProp = paras[0] + ",";
                    if (result >= 0 && result != 255) {
                        if (data.length() > 35)
                            resultProp += data.substring(0, 35);
                        else
                            resultProp += data;
                    }
                    AgentUtils.setSystemProperties(Values.AGENT_RESULT_PROP, resultProp);
                    
                } catch (Exception e) {
                    logd(e);
                }

            }
        }.start();
        
    }

    private static void logd(Object s) {

        Thread mThread = Thread.currentThread();
        StackTraceElement[] mStackTrace = mThread.getStackTrace();
        String mMethodName = mStackTrace[3].getMethodName();

        s = "[" + mMethodName + "] " + s;
        Log.d("SystemAgent", s + "");
    }

    @Override
    public IBinder onBind(Intent arg0) {
        return null;
    }

}