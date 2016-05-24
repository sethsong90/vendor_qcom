/*
 * Copyright (c) 2014 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

package com.qualcomm.qti.services.secureui;

import com.qualcomm.wfd.service.ISessionManagerService;
import com.qualcomm.wfd.service.IWfdActionListener;
import com.qualcomm.wfd.WfdEnums;

import com.android.internal.telephony.cat.AppInterface;
import android.app.PendingIntent;
import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.os.Message;
import android.os.RemoteException;
import android.os.UserHandle;
import android.util.Log;

/** Secure UI Service
 * Gathers events from the Android framework and forwards them to the native
 * SecureUI entities.
 * Blocks WFD when a secure UI session is starting, and resumes it when it's done.
 *
 * This service is started on BOOT_COMPLETED
 * */
public class SecureUIService extends Service implements Runnable{
  public static final String TAG = "SecUISvc";

  public static final String ACTION_SECURE_DISPLAY_ACTIVE = "org.codeaurora.intent.action.SECURE_DISPLAY_ACTIVE";
  public static final String EXTRA_SECURE_DISPLAY_STATE = "state";

  private native int init();
  private native void terminate();
  private native byte[] waitForMessage();
  private native byte[] getSource();
  private native int sendResponse(int id, int payload, byte[] remote);
  public static native int sendNotification(int id, int payload, byte[] remote);
  public static byte[] TOUCH_LIB_ADDR = {0, 's', 'u', 'i', 't', 'c', 'h'};
  private boolean running = false;
  private boolean secureDisplayActive = false;
  private WfdReceiver wfdReceiver;

  /** DO NOT CHANGE THE FOLLOWING. Refer to readme.txt for an explanation */
  /** Bytes in a message */
  private static final int BYTES_IN_MEX           = 4;
  /** Message type */
  private static final int SUI_MSG_CMD_MSK        = 0x08;
  private static final int SUI_MSG_RSP_MSK        = 0x04;
  private static final int SUI_MSG_NOT_MSK        = 0x02;
  /** Message ID */
  public static final int SUI_MSG_ID_CALL_STATUS = 0x01;
  public static final int SUI_MSG_ID_ABORT       = 0x02;
  public static final int SUI_MSG_ID_SCREEN_STATUS = 0x03;
  public static final int SUI_MSG_ID_SD_STARTING = 0x10;
  public static final int SUI_MSG_ID_SD_STOPPED  = 0x11;
  /** Message payload */
  public static final int SUI_MSG_RET_OK         = 0x00;
  public static final int SUI_MSG_RET_ABORT      = 0x01;
  public static final int SUI_MSG_RET_NOP        = 0x0F;

  static {
    System.loadLibrary("secureuisvc_jni");
  }

  public static Context context;

  @Override
  public void run() {
    int rv = 0;
    Log.d(TAG, "Thread created.");
    rv = init();
    if (rv != 0) {
      Log.e(TAG, "Error initializing local socket: " + rv);
      return;
    }
    running = true;
    ScreenReceiver mScreenReceiver = new ScreenReceiver();
    IntentFilter filter = new IntentFilter(Intent.ACTION_SCREEN_OFF);
    filter.addAction(Intent.ACTION_SCREEN_ON);
    filter.addAction(Intent.ACTION_USER_PRESENT);
    filter.addAction(AppInterface.CAT_IDLE_SCREEN_ACTION);
    registerReceiver(mScreenReceiver, filter);

    wfdReceiver = new WfdReceiver(true);
    filter = new IntentFilter(WfdEnums.ACTION_WIFI_DISPLAY_ENABLED);
    filter.addAction(WfdEnums.ACTION_WIFI_DISPLAY_DISABLED);
    registerReceiver(wfdReceiver, filter);

    // send the initial notification with the current call status
    if (CallReceiver.callActive() == false) {
      sendNotification(SUI_MSG_ID_CALL_STATUS, SUI_MSG_RET_OK, null);
    } else {
      sendNotification(SUI_MSG_ID_CALL_STATUS, SUI_MSG_RET_ABORT, null);
    }

    // send the initial notification with the current screen status
    if (ScreenReceiver.screenOn()){
      sendNotification(SUI_MSG_ID_SCREEN_STATUS, SUI_MSG_RET_OK, null);
    } else {
      sendNotification(SUI_MSG_ID_SCREEN_STATUS, SUI_MSG_RET_ABORT, null);
    }

    do {
      byte[] mex = waitForMessage();
      if (mex == null) {
        Log.w(TAG, "Error waiting for message");
        break;
      }
      if (mex.length != BYTES_IN_MEX) {
        Log.w(TAG, "Unexpected length message: " + mex.length + " vs " + BYTES_IN_MEX);
        Log.w(TAG, "Mex: " + String.format("%02X %02X %02X %02X", mex[0], mex[1], mex[2], mex[3]));
        continue;
      }
      if (mex[0] == SUI_MSG_NOT_MSK) {
        rv = ProcessNotification(mex[1], mex[2]);
        if (rv != 0) {
          Log.w(TAG, "Failed to process notification " + String.format("%02X %02X: %d", mex[1], mex[2], rv));
          continue;
        }
      } else {
        byte[] source = getSource();
        rv = ProcessCommand(mex[1], mex[2], source);
        if (rv != 0) {
          Log.w(TAG, "Failed to process command " + String.format("%02X %02X: %d", mex[1], mex[2], rv));
          continue;
        }
      }
    } while (true);
  }

  void setSecureDisplayActive(boolean active) {
    if (secureDisplayActive != active) {
      secureDisplayActive = active;
      Intent intent = new Intent(ACTION_SECURE_DISPLAY_ACTIVE);
      intent.putExtra(EXTRA_SECURE_DISPLAY_STATE, active);
      sendStickyBroadcastAsUser(intent, UserHandle.ALL);
    }
  }

  private int ProcessCommand(int id, int payload, byte[] source) {
    int rv = 0;
    Log.d(TAG, "ProcessCommand: " + String.format("%02X %02X", id, payload));
    if (id == SUI_MSG_ID_SD_STARTING) {
      if (wfdReceiver.pause()) {
        Log.d(TAG, "paused or not needing to pause");
        sendResponse(id, SUI_MSG_RET_OK, source);
        // broadcasting intent
        setSecureDisplayActive(true);
      } else {
        Log.d(TAG, "Cannot pause, vetting");
        sendResponse(id, SUI_MSG_RET_ABORT, source);
      }
    } else if (id == SUI_MSG_ID_CALL_STATUS) {
      if (CallReceiver.callActive()) {
        sendResponse(id, SUI_MSG_RET_ABORT, source);
      } else {
        sendResponse(id, SUI_MSG_RET_OK, source);
      }
    } else if (id == SUI_MSG_ID_SCREEN_STATUS) {
      if (ScreenReceiver.screenOn()) {
        sendResponse(id, SUI_MSG_RET_OK, source);
      } else {
        sendResponse(id, SUI_MSG_RET_ABORT, source);
      }
    }
    return rv;
  }

  private int ProcessNotification(int id, int payload) {
    int rv = 0;
    Log.d(TAG, "ProcessNotification: " + String.format("%02X %02X", id, payload));
    if (id == SUI_MSG_ID_SD_STOPPED) {
      Log.d(TAG, "Resuming");
      wfdReceiver.resume();
      // broadcasting intent
        setSecureDisplayActive(false);
    }
    return rv;
  }

  @Override
  public void onCreate() {
    super.onCreate();
    SecureUIService.context = this;
    if (!running) {
      new Thread(this).start();
    }
  }

  @Override
  public int onStartCommand(Intent intent, int flags, int startId)
  {
    Log.d(TAG, "Service started flags " + flags + " startId " + startId);
    // We want this service to continue running until it is explicitly
    // stopped, so return sticky.
    return START_STICKY;
  }

  @Override
  public void onDestroy() {
    super.onDestroy();
    sendNotification(SUI_MSG_ID_ABORT, SUI_MSG_RET_NOP, TOUCH_LIB_ADDR);
    sendNotification(SUI_MSG_ID_ABORT, SUI_MSG_RET_NOP, null);
    terminate();
    Log.d(TAG, "Service destroyed.");
  }

  @Override
  public IBinder onBind(Intent intent) {
    Log.d(TAG, "Service bind.");
    return null;
  }

}
