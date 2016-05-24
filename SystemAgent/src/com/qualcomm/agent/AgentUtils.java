/**
 * Copyright (c) 2012-2013, Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 * Developed by QRD Engineering team.
 */
package com.qualcomm.agent;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.OutputStream;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.media.MediaActionSound;
import android.net.Uri;
import android.os.PowerManager;
import android.os.SystemProperties;
import android.provider.MediaStore;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.SurfaceControl;
import android.view.WindowManager;

import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneFactory;

public class AgentUtils {
    private final static String TAG = "SystemAgent";
    
    public static boolean mkdirAgent(String path) {
        File dir = new File(path);
        return dir.mkdirs();
    }
    
    public static boolean writeFileAgent(String filePath, String content) {
        logd("");
        boolean res = true;
        File file = new File(filePath);
        File dir = new File(file.getParent());
        if (!dir.exists())
            dir.mkdirs();
        try {
            FileWriter mFileWriter = new FileWriter(file);
            mFileWriter.write(content);
            mFileWriter.close();
        } catch (IOException e) {
            logd(e);
            res = false;
        }
        return res;
    }
    
    public static boolean setSystemProperties(String key, String val) {
        logd("key=" + key + " value=" + val);
        if (val == null || key == null)
            return false;
        SystemProperties.set(key, val);
        if (key.equals(SystemProperties.get(key)))
            return true;
        else
            return false;
    }
    
    public static String getSystemProperties(String key, String defaultValue) {
        logd("key=" + key + " value=" + defaultValue);
        if (key == null)
            return null;

        String property = SystemProperties.get(key, defaultValue);
        logd(property);
        return property;
    }

    public static void takeScreenshot(Context context, String path) {
        logd("Path=" + path);

        WindowManager windowManager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        Display display = windowManager.getDefaultDisplay();
        DisplayMetrics displayMetrics = new DisplayMetrics();
        display.getRealMetrics(displayMetrics);
        
        float[] dimension = { displayMetrics.widthPixels, displayMetrics.heightPixels };
        Bitmap bitmap = SurfaceControl.screenshot((int) dimension[0], (int) dimension[1]);
        //Save image to the MediaStore
        ContentValues contentValues = new ContentValues();
        ContentResolver contentResolver = context.getContentResolver();
        // will create the path automatically
        contentValues.put(MediaStore.Images.ImageColumns.DATA, path);
        contentValues.put(MediaStore.Images.ImageColumns.MIME_TYPE, "image/png");
        
        OutputStream outputStream;
        try {
            //delete the column of old screenshot for android 4.2 does not allow to insert the same file
            String columns[] = new String[]{MediaStore.Images.Media._ID,MediaStore.Images.Media.DATA};
            Cursor cur = contentResolver.query(MediaStore.Images.Media.EXTERNAL_CONTENT_URI,columns,
                MediaStore.Images.Media.DATA+"=\""+path+"\"",null, MediaStore.Images.Media._ID);
            if (cur != null) {
                contentResolver.delete(MediaStore.Images.Media.EXTERNAL_CONTENT_URI,MediaStore.Images.Media.DATA+"=\""+path+"\"",null);
            }
            Uri uri = contentResolver.insert(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, contentValues);
            outputStream = contentResolver.openOutputStream(uri);
            bitmap.compress(Bitmap.CompressFormat.PNG, 100, outputStream);
            outputStream.flush();
            outputStream.close();
        } catch (Exception e) {
            logd(e);
            e.printStackTrace();
            return;
        }
        String title = context.getString(R.string.screenshot_notification_title);
        String text = path + " " + context.getString(R.string.screenshot_notification_text);
        MediaActionSound cameraSound = new MediaActionSound();
        cameraSound.play(MediaActionSound.STOP_VIDEO_RECORDING);

        sendNotification(context, Values.SCREENSHOT_NOTIFICATION_ID, R.drawable.qcom_small, title, text);
        try {
            
            Intent intent = new Intent("action.logkit.upload");
            intent.putExtra("reason", "screenshot");
            intent.putExtra("path", path);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(intent);
        } catch (Exception e) {
            loge(e);
        }
        // // update file size in the database
        // contentValues.clear();
        // contentValues.put(MediaStore.Images.ImageColumns.SIZE, new
        // File(path).length());
        // contentResolver.update(uri, contentValues, null, null);
    }
    
    public static void sendNotification(Context context, int id, int resId, String title, String text) {
        Notification notification;
        NotificationManager mNotificationManager = (NotificationManager) context
                .getSystemService(Context.NOTIFICATION_SERVICE);
        Intent intent = new Intent();
        intent.setClassName("com.qualcomm.stats", "com.qualcomm.stats.log.upload.UploadMainUI");
        PendingIntent pendingIntent = PendingIntent.getActivity(context, 0, intent, 0);
        notification = new Notification(resId, title, System.currentTimeMillis());
        // mNotification.flags = Notification.FLAG_NO_CLEAR;
        notification.flags = Notification.FLAG_AUTO_CANCEL;
        notification.setLatestEventInfo(context, title, text, pendingIntent);
        mNotificationManager.notify(id, notification);
    }

    public static String getDeviceId() {
        String deviceId = "unknown";
        Phone phone = PhoneFactory.getDefaultPhone();
        deviceId = phone.getDeviceId();
        logd("DeviceId=" + deviceId);
        // (TelephonyManager)mApplicationContext.getSystemService("phone")).getDeviceId();
        return deviceId;
        // for (int i = 0; i < 2; i++) {
        // Phone phone = MSimPhoneFactory.getPhone(i);
        // logd("name " + phone.getPhoneName());
        // logd("imei " + phone.getImei());
        // logd("meid " + phone.getMeid());
        // logd("deviceid " + phone.getDeviceId());
        // }
    }
    public static void reboot(Context context, String reason) {
        PowerManager pm = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        // pm.reboot("recovery");
        pm.reboot(reason);
    }

    public static String readFileAgent(String filePath) {
        try {
            File file = new File(filePath);
            FileInputStream fis = new FileInputStream(file);
            int length = fis.available();
            byte [] buffer = new byte[length];
            fis.read(buffer);
            fis.close();
            String content = new String(buffer, "UTF-8");;
            return content;
        } catch (Exception e) {
            loge(e);
        }
        return null;
    }

    private static void logd(Object s) {
        
        Thread mThread = Thread.currentThread();
        StackTraceElement[] mStackTrace = mThread.getStackTrace();
        String mMethodName = mStackTrace[3].getMethodName();
        
        s = "[" + mMethodName + "] " + s;
        Log.d(TAG, s + "");
    }
    
    private static void loge(Object s) {
        
        Thread mThread = Thread.currentThread();
        StackTraceElement[] mStackTrace = mThread.getStackTrace();
        String mMethodName = mStackTrace[3].getMethodName();
        
        s = "[" + mMethodName + "] " + s;
        Log.e(TAG, s + "");
    }
}
