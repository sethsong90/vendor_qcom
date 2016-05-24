/*============================================================================
@file CommandReceiver.java

@brief
Receives and processes broadcast commands.

Copyright (c) 2012-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
============================================================================*/
package com.qualcomm.sensors.qsensortest;

import java.util.List;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.Sensor;

import com.qti.sensors.stream.SensorView;
import com.qti.sensors.stream.StreamingActivity;

public class CommandReceiver extends BroadcastReceiver {
  public CommandReceiver() {}

  @Override
  public void onReceive(Context context, Intent intent) {
    if(intent.getAction().contentEquals("com.qualcomm.sensors.qsensortest.intent.STREAM")) {
      CharSequence sensorName = intent.getCharSequenceExtra("sensorname");
      int sensorType = intent.getIntExtra("sensortype", 0);
      int rate = intent.getIntExtra("rate", 60);
      int batchRate = intent.getIntExtra("batch", -1);

      List<SensorView> sensorViews = StreamingActivity.sensorListView.sensorList();
      for(SensorView sensorView : sensorViews) {
        Sensor sensor = sensorView.sensorAdapter().sensor();
        if(sensor.getName().contentEquals(sensorName) &&
           sensor.getType() == sensorType &&
           SettingsDatabase.getSettings().getSensorSetting(sensor).getEnableStreaming()) {
          sensorView.sensorAdapter().streamRateIs(rate, batchRate, true);
          break;
        }
      }
    }
  }

  public IntentFilter getIntentFilter() {
    IntentFilter intentFilter = new IntentFilter();
    intentFilter.addAction("com.qualcomm.sensors.qsensortest.intent.STREAM");
    intentFilter.addCategory("com.qualcomm.sensors.qsensortest.intent.category.DEFAULT");

    return intentFilter;
  }
}