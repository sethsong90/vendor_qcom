/******************************************************************************
 * @file    DataTransfer.java
 * @brief   Handles data transfer for Qos Connections
 *
 *
 * ---------------------------------------------------------------------------
 *  Copyright (C) 2012 Qualcomm Technologies, Inc.
 *  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
 * ---------------------------------------------------------------------------
 *
 ******************************************************************************/

package com.android.QosTest;

import android.content.Context;
import android.os.Bundle;
import android.os.Message;
import android.os.SystemProperties;
import android.util.Log;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

public class DataTransfer {
    /**
     * This class contains the functions needed to send data packets through
     * each Qos connection and also determines what text to place in the UI
     */
    static final String TAG = "DATA-TRANSFER";
    private static String sDefaultCmd, sDefaultOptions;
    private static Map<String, Process> sQosDataMap = new HashMap<String, Process>();
    private static Context sContext;
    private static String sQosId = "";

    public static class QosSendStatus {
        public static final int SEND_FINISH = 0;
        public static final int SEND_UPDATE = 2;
        public static final int SEND_EXIT = 9;
    }

    public static class QosDataTransfer {
        public static final String QOS_ID = "QosId";
        public static final String QOS_PACKET_DATA = "Data";
    }

    // Runs the command to send packets of data
    public static void sendData(Context context, String qosId) {
        int exit = QosSendStatus.SEND_UPDATE;
        String cmd = SystemProperties.get("mpdn.cmd.qosId." + qosId, "");
        try {
            Process p;
            if (cmd != "") {
                Log.d(TAG, "Using setprop data command");
                Log.d(TAG, "QosId = " + qosId + " cmd = " + cmd);
                p = Runtime.getRuntime().exec(cmd);
            }
            else {
                Log.d(TAG, "Using default xml data command cmd = "
                        + sDefaultCmd + " " + sDefaultOptions);
                p = Runtime.getRuntime().exec(sDefaultCmd + " " + sDefaultOptions);
                Log.d(TAG, "QosId = " + qosId + " cmd = "
                        + sDefaultCmd + " " + sDefaultOptions);
                Log.d(TAG, "mpdn.cmd.qosId." + qosId + " " + cmd);
            }
            sQosDataMap.put(qosId, p);
            BufferedReader in = new BufferedReader(
                    new InputStreamReader(p.getInputStream()));
            String line = null;
            while ((line = in.readLine()) != null) {
                Log.d(TAG, line);
                Message msg = Message.obtain(QosTest.sHandler, QosSendStatus.SEND_UPDATE);
                Bundle results = new Bundle();
                results.putString(QosDataTransfer.QOS_ID, qosId);
                results.putString(QosDataTransfer.QOS_PACKET_DATA, line);
                msg.setData(results);
                QosTest.sHandler.sendMessage(msg);
            }
            try {
                exit = p.waitFor();
                sQosDataMap.remove(qosId);
                if (exit == 0)
                    Log.d(TAG, "SENT " + exit);
                else {
                    Log.e(TAG, "FAILED SENDING for QosId = "
                            + qosId + " error = " + exit);
                }
                Bundle results2 = new Bundle();
                results2.putInt("EXIT", exit);
                results2.putString(QosDataTransfer.QOS_ID, qosId);
                Message msg =
                        Message.obtain(QosTest.sHandler, exit);
                msg.setData(results2);
                QosTest.sHandler.sendMessage(msg);

            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        } catch (IOException e) {
            Log.e(TAG, "Got ERROR = " + e);
        }
        return;
    }

    public static String getCmd() {
        return sDefaultCmd + " " + sDefaultOptions;
    }

    public static void setCmd(String defaultCmd, String defaultOptions) {
        sDefaultCmd = defaultCmd;
        sDefaultOptions = defaultOptions;
    }

    public static boolean startDataThread(Context context, String qosId) {
        Process operation = sQosDataMap.get(qosId);
        if (operation == null) {
            sContext = context;
            sQosId = qosId;
            Thread thread = new Thread(runCmd);
            thread.start();
            return true;
        }
        else
            return false;
    }

    public static void stopDataThread(String qosId) {
        Process operation = sQosDataMap.get(qosId);
        if (operation != null) {
            operation.destroy();
        }
    }

    final static Runnable runCmd = new Runnable() {
        public void run() {
            sendData(sContext, sQosId);
        }
    };
}
