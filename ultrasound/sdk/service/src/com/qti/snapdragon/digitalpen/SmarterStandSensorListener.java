/*===========================================================================
                           SmarterStandSensorListener.java

Copyright (c) 2013-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
==============================================================================*/

package com.qti.snapdragon.digitalpen;

import android.hardware.SensorManager;
import android.hardware.SensorEventListener;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import java.util.List;
import android.util.Log;
import java.lang.Math;

public class SmarterStandSensorListener {
    private static final String TAG = "SmarterStandSensorListener";

    public interface AccelerometerChangeCallback {
        void onNewPosition(double accelerometerAngleReading);
    }

    private final AccelerometerChangeCallback callback;
    private final SensorManager sensorManager;
    private double prevReportedAngle = 0;
    /**
     * Report angles that differ by this threshold
     */
    private static final double REPORT_ANGLE_THRESHOLD = 1.5;
    /**
     * Create a sensor event listener to listen to accelerometer changes and
     * store angle.
     */
    private final SensorEventListener sensorEventListener = new SensorEventListener() {
        @Override
        public void onSensorChanged(SensorEvent event) {
            double rotationAngle = getRotationAngle(event.values[0], event.values[1],
                    event.values[2]);
            if (-1 != rotationAngle &&
                    REPORT_ANGLE_THRESHOLD < diff(rotationAngle, prevReportedAngle)) {
                callback.onNewPosition(rotationAngle);
                Log.d(TAG,
                        "Rotation angle: " + rotationAngle);
                prevReportedAngle = rotationAngle;
                return;
            }
        }

        @Override
        public void onAccuracyChanged(Sensor sensor, int accuracy) {
            // TODO Auto-generated method stub
        }

        double getRotationAngle(float x, float y, float z) {
            if (0 == z) {
                return -1;
            }

            double rotationAngle = Math.toDegrees(Math.atan(-y / z));
            if (0 > rotationAngle)
            {
                return 180 + rotationAngle;
            }
            else
            {
                return rotationAngle;
            }
        }
    };

    public SmarterStandSensorListener(AccelerometerChangeCallback callback,
            SensorManager sensorManager) {
        this.callback = callback;
        this.sensorManager = sensorManager;
    }

    public void start() {
        // Register to accelerometer sensor events
        Sensor sensor;
        List<Sensor> sensors = sensorManager.getSensorList(Sensor.TYPE_ACCELEROMETER);
        if (0 < sensors.size()) {
            sensor = sensors.get(0);
            sensorManager.registerListener(sensorEventListener, sensor,
                    SensorManager.SENSOR_DELAY_NORMAL);
        } else {
            // Notify the user that there's no accelerometer sensor
            Log.w(TAG,
                    "There's no accelerometer sensor, cannot track angle changes");
        }
    }

    public void stop() {
        if (null != sensorManager)
        {
            sensorManager.unregisterListener(sensorEventListener);
        }
    }

    private double diff(double d1, double d2)
    {
        return Math.abs(d1 - d2);
    }
}
