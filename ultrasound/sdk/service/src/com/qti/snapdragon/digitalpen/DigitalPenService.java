/*===========================================================================
                           DigitalPenService.java

Copyright (c) 2012-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
=============================================================================*/

package com.qti.snapdragon.digitalpen;

import java.util.Scanner;

import android.os.RemoteException;

import com.qti.snapdragon.digitalpen.util.DigitalPenConfig;
import com.qti.snapdragon.digitalpen.util.DigitalPenData;
import com.qti.snapdragon.digitalpen.util.DigitalPenEvent;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;
import android.os.RemoteCallbackList;
import android.os.SystemProperties;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;

/**
 * This service gets all the requests for enabling/changing the DigitalPen
 * framework.
 */
public class DigitalPenService extends Service {
    // TODO: PERMISSIONS
    // private static final String DIGITALPEN_ADMIN_PERM =
    // android.Manifest.permission.DIGITALPEN_ADMIN;
    // private static final String DIGITALPEN_PERM =
    // android.Manifest.permission.DIGITALPEN;
    final String TAG = "DigitalPenService";
    // This flag is used by the messaging mechanism to report that the dameon
    // has stopped
    public static final int DAEMON_STOPPED = 0;
    // Stop daemon with Sigterm decelration
    static final String pidDirectory = "/data/usf/";
    static final String pidFileExtention = ".pid";

    final String mDaemonName = "usf_epos";
    // The C'tor returns the default configuration
    DigitalPenConfig mDefaultConfig = new DigitalPenConfig();

    // Notification variables
    private enum PenNotification {
        ACTIVE(0x10, "Digital Pen is active", R.drawable.pen),
        BLOCKED_2_MICS(0x20, "2 Digital Pen microphones are blocked", R.drawable.blocked_mic_yellow),
        BLOCKED_3_OR_MORE_MICS(0x20, "3 or more Digital Pen microphones are blocked",
                R.drawable.blocked_mic_red),
        LOW_BATTERY(0x30, "Digital Pen battery is low", R.drawable.low_battery);

        final int id; // arbitrary number; blocked mics share ID
        final String text;
        final int icon;
        private PenNotification(int id, String text, int icon) {
            this.id = id;
            this.text = text;
            this.icon = icon;
        }
    }

    private NotificationManager mNM;

    /**
     * Those are a lists of callbacks that have been registered with the
     * service. Note that this is package scoped (instead of private) so that it
     * can be accessed more efficiently from inner classes.
     */
    final RemoteCallbackList<IDataCallback> mDataCallback =
            new RemoteCallbackList<IDataCallback>();
    final RemoteCallbackList<IEventCallback> mEventCallback =
            new RemoteCallbackList<IEventCallback>();

    static boolean mDaemonEnabled;

    // The state of the daemon, check "DigitalPenEvent" for available states

    /** Send an event to all registered users */
    protected void sendEvent(int eventType,
            int[] params) {
        DigitalPenEvent eventToSend =
                new DigitalPenEvent(
                        eventType,
                        params
                );
        Log.d(TAG, "Sending Event: " + eventType);
        // Invoke callback functions, with the event received from socket
        int i = mEventCallback.beginBroadcast();
        while (i-- > 0) {
            try {
                mEventCallback.getBroadcastItem(i).onDigitalPenPropEvent(
                        eventToSend);
            } catch (RemoteException e) {
                // The RemoteCallbackList will take care of removing
                // the dead object for us.
            }
        }
        mEventCallback.finishBroadcast();

        // Show or remove blocked microphone notifications
        if (DigitalPenEvent.TYPE_MIC_BLOCKED == eventType) {
            final int numMicsBlocked = params[0];
            switch (numMicsBlocked) {
            case 0:
            case 1: // 1 mic always seems to be blocked
                // Since BLOCKED_2_MICS and BLOCKED_3_OR_MORE_MICS have the
                // same notification id, removing the BLOCKED_2_MICS notification
                // also removes the BLOCKED_3_OR_MORE_MICS notification
                removeNotification(PenNotification.BLOCKED_2_MICS);

                break;
            case 2:
                showNotification(PenNotification.BLOCKED_2_MICS);
                break;
            default:
                showNotification(PenNotification.BLOCKED_3_OR_MORE_MICS);
                break;
            }
        }

        // Show or remove low pen battery notification
        if (DigitalPenEvent.TYPE_PEN_BATTERY_STATE == eventType) {
            switch (params[0]) {
            case DigitalPenEvent.BATTERY_OK:
                removeNotification(PenNotification.LOW_BATTERY);
                break;
            case DigitalPenEvent.BATTERY_LOW:
                showNotification(PenNotification.LOW_BATTERY);
                break;
            }
        }
    }

    /**
     * The implementation for the IDigitalPen interface which is used to
     * communicate with this service
     */
    private final DigitalPenServiceCore serviceCore = new DigitalPenServiceCore(this);

    /**
     * Show a notification while this service is running.
     */
    void showNotification(PenNotification penNotification) {
        // The PendingIntent to launch our activity if the user selects this
        // notification

        // TODO: this doesn't do anything; service isn't an activity
        PendingIntent contentIntent = PendingIntent.getActivity(this, 0,
                new Intent(this, DigitalPenService.class), 0);

        Notification notification = new Notification.Builder(this)
                .setSmallIcon(penNotification.icon)
                .setContentTitle(penNotification.text)
                .setContentIntent(contentIntent)
                .build();

        mNM.notify(penNotification.id, notification);
    }

    /**
     * Called from core when pen is enabled to notify user
     */
    void notifyPenEnabled() {
        showNotification(PenNotification.ACTIVE);
    }

    /**
     * Called from core when pen is disabled to notify user
     */
    void notifyPenDisabled()
    {
        mNM.cancelAll();
    }

    /**
     * Remove a notification.
     */
    private void removeNotification(PenNotification notification)
    {
        mNM.cancel(notification.id);
    }

    // TODO: make list, move to service core
    IDataCallback onScreenCallback;

    @Override
    public IBinder onBind(Intent intent) {
        return serviceCore;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mNM = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        // Update enabled according to the system property
        mDaemonEnabled = !(SystemProperties.get("init.svc.usf_epos", "stopped")).
                equals("stopped");
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (intent != null) {
            // TODO: this is fine for testing, but a security issue
            if (intent.hasExtra("LoadConfigIntent")) {
                Intent loadConfigIntent = intent.getParcelableExtra("LoadConfigIntent");
                serviceCore.onSidebandLoadConfig(loadConfigIntent);
            }
            // TODO: move to AIDL interface, or broadcast intent, check
            // permissions there; security issue
            if (intent.hasExtra("GenerateOnScreenData")) {
                Log.d(TAG,
                        "Got 'GenerateOnScreenData': "
                                + intent.getStringExtra("GenerateOnScreenData"));
                Scanner scanner = new Scanner(intent.getStringExtra("GenerateOnScreenData"));
                scanner.skip("x=");
                DigitalPenData data = new DigitalPenData(scanner.nextInt(), 0, 0, 0, 0, 0, 0, false);
                scanner.close();
                try {
                    onScreenCallback.onDigitalPenPropData(data);
                } catch (RemoteException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }
        }
        return super.onStartCommand(intent, flags, startId);
    }


}
