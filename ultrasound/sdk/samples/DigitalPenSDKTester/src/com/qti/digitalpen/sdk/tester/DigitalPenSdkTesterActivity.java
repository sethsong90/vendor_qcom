/*===========================================================================
                           DigitalPenSdkTesterActivity.java

Copyright (c) 2013-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
==============================================================================*/

package com.qti.digitalpen.sdk.tester;

import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager.Area;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager.Feature;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager.Mapping;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager.OffScreenMode;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager.OnSideChannelDataListener;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager.SideChannelData;

import android.os.Bundle;
import android.app.Activity;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.widget.CheckBox;
import android.widget.TextView;
import android.widget.Toast;

public class DigitalPenSdkTesterActivity extends Activity {

    private static final String TAG = "DigitalPenSdkTesterActivity";
    private DigitalPenManager mgr;
    private final OnSideChannelDataListener onScreenListener = new OnSideChannelDataListener() {

        @Override
        public void onDigitalPenData(final SideChannelData data) {
            pointCount++;
            runOnUiThread(new Runnable() {

                @Override
                public void run() {
                    setOnScreenDataPoints(data);
                }
            });
        }
    };
    private TextView onScreenDataText;
    private int pointCount;
    private int numListeners = 0; // this compensates for bug in SDK where there
                                  // is
                               // only 1 real listener

    // TODO: route logcat from socket to verify settings (?)
    // TODO: read back settings from service

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        onScreenDataText = (TextView) findViewById(R.id.textSideBandEventCount);
        setOnScreenDataPoints(null);

        if (!DigitalPenManager.isFeatureSupported(Feature.BASIC_DIGITAL_PEN_SERVICES)) {
            Toast.makeText(this, "Basic pen services not supported on this device.",
                    Toast.LENGTH_LONG).show();
        }
        mgr = new DigitalPenManager(getApplication());
    }

    private void setOnScreenDataPoints(SideChannelData data) {
        if (data == null) {
            onScreenDataText.setText("Side-Channel Points: " + pointCount);
        } else {
            onScreenDataText
                    .setText("Side-Channel Points: " + pointCount + ", last point: " + data);
        }

    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);

        return true;
    }

    public void onClickApplyConfiguration(View v) {
        applyConfigWithToastResponse();
    }

    protected void applyConfigWithToastResponse() {
        boolean success = mgr.applyConfig();
        String toastText = success ? "Configuration applied"
                : "Apply Configuration failed! See logcat.";
        int toastLength = success ? Toast.LENGTH_SHORT : Toast.LENGTH_LONG;

        Toast.makeText(this, toastText, toastLength).show();
    }

    public void onClickOffscreenRadio(View v) {
        OffScreenMode mode;
        switch (v.getId()) {
            case R.id.radioButtonOffscreenDuplicate:
                mode = OffScreenMode.DUPLICATE;
                break;
            case R.id.radioButtonOffscreenExtend:
                mode = OffScreenMode.EXTEND;
                break;
            case R.id.radioButtonOffscreenDefault:
                mode = OffScreenMode.SYSTEM_DEFAULT;
                break;
            case R.id.radioButtonOffscreenDisable:
                mode = OffScreenMode.DISABLED;
                break;
            default:
                throw new RuntimeException("Unknown button ID");
        }
        mgr.setOffScreenMode(mode);
    }

    public void onClickCheckBoxOnScreenSideBand(View v) {
        CheckBox checkBox = (CheckBox) v;
        if (checkBox.isChecked()) {
            mgr.setCoordinateMapping(Area.ON_SCREEN, Mapping.ANDROID_AND_SIDE_CHANNEL);
            if (numListeners++ == 0) {
                mgr.registerOnScreenCallback(onScreenListener);
            }
        } else {
            mgr.setCoordinateMapping(Area.ON_SCREEN, Mapping.ANDROID);
            if (--numListeners == 0) {
                mgr.registerOnScreenCallback(null);
            }
        }
        applyConfigWithToastResponse();

    }

    public void onClickCheckBoxOffScreenSideBand(View v) {
        CheckBox checkBox = (CheckBox) v;
        if (checkBox.isChecked()) {
            mgr.setCoordinateMapping(Area.OFF_SCREEN, Mapping.ANDROID_AND_SIDE_CHANNEL);
            if (numListeners++ == 0) {
                mgr.registerOnScreenCallback(onScreenListener);
            }
        } else {
            mgr.setCoordinateMapping(Area.OFF_SCREEN, Mapping.ANDROID);
            if (--numListeners == 0) {
                mgr.registerOnScreenCallback(null);
            }
        }
        applyConfigWithToastResponse();

    }

}
