/**
 * Copyright (c) 2013-2014 Qualcomm Technologies, Inc.All Rights Reserved.
 * Qualcomm Technologies Confidential and Proprietary.
 */

package com.qti.snapdragon.sdk.digitalpen;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import com.qti.snapdragon.digitalpen.IDataCallback;
import com.qti.snapdragon.digitalpen.IDigitalPen;
import com.qti.snapdragon.digitalpen.util.AppInterfaceKeys;
import com.qti.snapdragon.digitalpen.util.DigitalPenData;

import android.app.Activity;
import android.app.Application;
import android.app.Application.ActivityLifecycleCallbacks;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.util.Log;
import android.view.MotionEvent;

/**
 * The DigitalPenManager communicates configuration to the digital pen service
 * on the device and has methods for registering listeners for events and
 * side-channel data from the digital pen.
 * <p>
 * <ul>
 * <li>The minimum Android API level is <b>15</b>.
 * <li>Not all devices will have Digital Pen functionality. Query
 * <code>{@link #isBasicServiceSupported()}</code> to verify.
 * </ul>
 * <p>
 * Configurations can be changed at run-time by calling
 * <code>{@link #applyConfig()}</code>
 * <p>
 * Multiple applications may use the Digital Pen SDK. Configurations are reset
 * to system default when the application is paused, and restored when the
 * application resumes.
 * <p>
 * <p>
 * <b>Development Flow</b>
 * <p>
 * <ul>
 * <li>Query for capability of device. Call
 * <code>{@link #isBasicServiceSupported()}</code> to check if the device has
 * the digital pen feature enabled.
 * <li>Create a new DigitalPenManager object.
 * <li>Apply configuration changes, e.g.,
 * <code>{@link #setOffScreenMode(OffScreenMode)}.{@link #applyConfig()}</code>.
 * <li>Register Event Listeners if desired.
 * </ul>
 * <p>
 * <b>Usage</b>
 *
 * <pre class="language-java">
 * // in your class's activity...
 * protected void onCreate(Bundle savedInstanceState) {
 *     // boilerplate ...
 *     super.onCreate(savedInstanceState);
 *     setContentView(R.layout.activity_main);
 *
 *     // To use the digital pen service...
 *     DigitalPenManager mgr = new DigitalPenManager(getApplication());
 *     mgr.setOffScreenMode(OffScreenMode.EXTEND)
 *             .setHoveringEnabled()
 *             .setCoordinateMapping(Area.OFF_SCREEN, Mapping.ANDROID_AND_SIDE_CHANNEL)
 *             .applyConfig();
 *     mgr.registerOffScreenCallback(new OnSideChannelDataListener() {
 *
 *         public void onDigitalPenData(SideChannelData data) {
 *             // do what you will with &quot;data&quot;
 *         }
 *     });
 * }
 * </pre>
 */
public class DigitalPenManager {

    public enum Feature {
        BASIC_DIGITAL_PEN_SERVICES
    }

    /**
     * This data is returned through listeners upon receiving pen events. This
     * side-channel data differs from the Android {@link MotionEvent} in that it
     * contains more information (e.g. tilt) and bypasses the android event
     * framework.
     *
     * @see OnSideChannelDataListener
     * @see DigitalPenManager#registerOnScreenCallback(OnSideChannelDataListener)
     * @see DigitalPenManager#registerOffScreenCallback(OnSideChannelDataListener)
     */
    public static class SideChannelData {
        /** X axis for the pen position in 0.1mm */
        public int xPos;

        /** Y axis for the pen position in 0.1mm */
        public int yPos;

        /** Z axis for the pen position in 0.1mm */
        public int zPos;

        /** Tilt of the pen between X axis and Z axis in 1.4*degrees */
        public int xTilt;

        /** Tilt of the pen between Y axis and Z axis in 1.4*degrees */
        public int yTilt;

        /**
         * Describes the orientation of the pen relative to the device:
         * <ul>
         * <li>+1: pen is pointed toward the device, perpendicular with it.
         * <li>-1: pen is pointed away from the device, perpendicular with it.
         * <li>0: pen is parallel with the device, i.e. resting on it.
         * </ul>
         */
        public int zTilt;

        /** Pen pressure from 0 (none) to 255 (max) */
        public int pressure;

        /** True if pen is down, false if pen is up (hovering) */
        public boolean isDown;

        @Override
        public String toString() {
            return "SideChannelData ("
                    + "x=" + xPos
                    + ",y=" + yPos
                    + ",z=" + zPos
                    + ",xTilt=" + xTilt
                    + ",yTilt=" + yTilt
                    + ",zTilt=" + zTilt
                    + ",isDown=" + isDown
                    + ",pressure=" + pressure
                    + ")";
        }

    }

    /**
     * Used when registering listeners for side-channel data, i.e., data points
     * that bypass the Android event framework.
     *
     * @see DigitalPenManager#registerOnScreenCallback(OnSideChannelDataListener)
     * @see DigitalPenManager#registerOffScreenCallback(OnSideChannelDataListener)
     * @see SideChannelData
     */
    public interface OnSideChannelDataListener {
        /**
         * Called each time the digital pen service generates a data point.
         *
         * @param data the side-channel data point generated by the pen
         */
        void onDigitalPenData(SideChannelData data);
    }

    // TODO: get answer for how these input types affect pen behavior
    /**
     * The type of input event the pen should generate.
     */
    public enum InputType {
        STYLUS, MOUSE, JOYSTICK
    }

    /** How the off-screen mode will operate */
    public enum OffScreenMode {
        // TODO: link helper class or remove this comment
        /**
         * Off-screen Android events will be generated on the virtual
         * <code>{@link android.app.Presentation Presentation}</code> to the
         * side of the screen. See the <code>{@link tbdHelperClass}</code>
         * documentation for an example of how to get this
         * <code>{@link android.app.Presentation Presentation}</code> handle
         */
        EXTEND,

        /**
         * Off-screen Android events will be mapped to the device's display, as
         * if the device screen was duplicated.
         */
        DUPLICATE,

        /** Android events will not be generated from off-screen pen activity. */
        DISABLED,

        /** The off-screen setting reverts to current system default. */
        SYSTEM_DEFAULT
    }

    /** Selects between off-screen and on-screen areas. */
    public enum Area {
        /** Refers to the off-screen area to the side of the device */
        OFF_SCREEN(AppInterfaceKeys.OFF_SCREEN_MAPPING),

        /** Refers to the device's screen */
        ON_SCREEN(AppInterfaceKeys.ON_SCREEN_MAPPING);

        private final String key;

        private Area(String key) {
            this.key = key;
        }

        String getKey() {
            return key;
        }
    }

    /**
     * How events generated by the digital pen service will be sent to the
     * application
     */
    public enum Mapping {
        /** No events will be generated. */
        NONE,

        // TODO: figure out how to link the javadoc below
        /**
         * Only side-channel events will be generated. These will come through
         * {@link DigitalPenManager#registerOnScreenCallback(OnSideChannelDataListener)
         * registerOnScreenCallback(OnSideChannelDataListener)} or
         * {@link DigitalPenManager#registerOffScreenCallback(OnSideChannelDataListener)
         * registerOffScreenCallback(OnSideChannelDataListener)}.
         * <p>
         * <b><i>I can't figure out how to link these to the containing class;
         * looks fine in Eclipse's javadoc view.</i></b>
         */
        SIDE_CHANNEL,

        /**
         * Only Android events ({@link android.view.MotionEvent MotionEvent})
         * will be generated
         */
        ANDROID,

        /**
         * Both Android events and side-channel events (as in
         * {@link #SIDE_CHANNEL} and {@link #ANDROID}) will be generated. The
         * side-channel events come through the appropriate
         * {@link OnSideChannelDataListener} registered with either
         * {@link DigitalPenManager#registerOnScreenCallback} or
         * {@link DigitalPenManager#registerOffScreenCallback}
         */
        ANDROID_AND_SIDE_CHANNEL
    }

    private static final String TAG = "DigitalPenManager";
    private static final String DIGITAL_PEN_SYSTEM_PROP = "ro.qc.sdk.us.digitalpen";
    private static final OnSideChannelDataListener NULL_DATA_LISTENER = new OnSideChannelDataListener() {

        @Override
        public void onDigitalPenData(SideChannelData data) {
        }
    };
    private static final long SERVICE_CONNECTION_TIMEOUT_SECONDS = 5;

    /**
     * Checks whether DigitalPen framework is supported on this device.
     *
     * @return true if DigitalPen is supported on this device, false otherwise.
     */
    static private boolean isBasicServiceSupported() {
        String value = SystemProperties.get(DIGITAL_PEN_SYSTEM_PROP);
        return value.equals("1");
    }

    private ActivityLifecycleHandler activityLifecycleHandler;

    private IDigitalPen service;

    private final ServiceConnection serviceConnection = new ServiceConnection() {

        @Override
        public void onServiceDisconnected(ComponentName name) {
            // TODO: test service disconnection

        }

        @Override
        public void onServiceConnected(ComponentName name, IBinder incomingService) {
            service = IDigitalPen.Stub.asInterface(incomingService);
            try {
                service.setOnScreenDataCallback(new IDataCallback.Stub() {

                    @Override
                    public void onDigitalPenPropData(DigitalPenData data) throws RemoteException {
                        Log.d(TAG, "Got callback from service!");
                        handleOnScreenDataCallback(data);
                    }

                });
            } catch (RemoteException e) {
                // TODO: test what happens when this API call fails
                e.printStackTrace();
            }
            connectionFinished.countDown();
        }
    };

    synchronized protected void handleOnScreenDataCallback(DigitalPenData data) {
        // TODO: populate with all data
        Log.d(TAG, "On-screen callback, x=" + data.getX());
        SideChannelData returnedData = new SideChannelData();
        returnedData.xPos = data.getX();
        returnedData.yPos = data.getY();
        returnedData.zPos = data.getZ();
        returnedData.xTilt = data.getXTilt();
        returnedData.yTilt = data.getYTilt();
        returnedData.zTilt = data.getZTilt();
        returnedData.isDown = data.getPenState();
        returnedData.pressure = data.getPressure();
        onScreenCallback.onDigitalPenData(returnedData);
    }

    private CountDownLatch connectionFinished;

    private final Bundle appConfig = new Bundle();

    private boolean isConfigApplied;
    private Application application;

    /**
     * Creates a new DigitalPenManager instance.
     *
     * @param application the application handle
     */
    public DigitalPenManager(Application application) {
        init(application);
        new Thread(new Runnable() {

            @Override
            public void run() {
                Log.d(TAG, "Background thread starting service...");
                try {
                    ensureServiceStarted();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                Log.d(TAG, "Background thread finished");
            }
        }).start();
    }

    /**
     * Checks if is feature supported. Returns true if the feature is available
     * and false if it is not.
     *
     * @param feature the {@link Feature} being checked.
     * @return true if feature is supported
     */
    public static boolean isFeatureSupported(Feature feature) {
        switch (feature) {
            case BASIC_DIGITAL_PEN_SERVICES:
                return isBasicServiceSupported();
            default:
                return false;
        }
    }

    private class ActivityLifecycleHandler implements ActivityLifecycleCallbacks {

        @Override
        public void onActivityCreated(Activity activity, Bundle savedInstanceState) {
        }

        @Override
        public void onActivityStarted(Activity activity) {
        }

        @Override
        public void onActivityResumed(Activity activity) {
            if (!isConfigApplied) {
                return;
            }
            try {
                applyConfig();
            } catch (Exception e) {
                Log.e(TAG, "applyConfig failed after resuming activity!");
                e.printStackTrace();
            }
        }

        @Override
        public void onActivityPaused(Activity activity) {
            try {
                releaseApplication();
            } catch (InterruptedException e) {
                // TODO: test this path
                e.printStackTrace();
            }
        }

        @Override
        public void onActivityStopped(Activity activity) {
        }

        @Override
        public void onActivitySaveInstanceState(Activity activity, Bundle outState) {
        }

        @Override
        public void onActivityDestroyed(Activity activity) {
        }
    };

    /**
     * Initializes the GestureDeviceManager.
     *
     * @param application the activity context
     */
    private void init(Application application) {
        // Currently the settings are app-wide; different activities within an
        // application with different configuration needs will have to manage
        // their own configurations

        this.application = application;
        activityLifecycleHandler = new ActivityLifecycleHandler();
        application.registerActivityLifecycleCallbacks(activityLifecycleHandler);

        // TODO: Initialize the feature list

    }

    /**
     * Release the connection with the digital pen service. The digital pen
     * configuration will revert to the system default.
     */
    public void releaseApplication() throws InterruptedException {
        // TODO: return false on fail
        // This should be called automatically anyway when application is
        // paused. But this lets the user release it as well.
        service = ensureServiceStarted();
        try {
            service.releaseActivity();
        } catch (RemoteException e) {
            // TODO: return false
            e.printStackTrace();
        }
    }

    /**
     * Apply all configuration changes with the digital pen service. This should
     * be called after completing configuration changes, including:
     * <ul>
     * <li> {@link #setOffScreenMode(OffScreenMode)}
     * <li> {@link #setHoverEnabled()}
     * <li> {@link #setEraserBypass()}
     * <li> {@link #setInputType(InputType)}
     * <li> {@link #setCoordinateMapping(Area, Mapping)}
     * <li> {@link #clearConfig()}
     * </ul>
     */
    public boolean applyConfig() {
        try {
            service = ensureServiceStarted();
            if (!service.applyConfig(appConfig)) {
                return false;
            }
            isConfigApplied = true;
            return true;
        } catch (Exception e) {
            Log.e(TAG, "applyConfig failed!");
            e.printStackTrace();
            return false;
        }
    }

    protected IDigitalPen ensureServiceStarted() throws InterruptedException {
        if (service != null) {
            return service;
        }
        Log.d(TAG, "Binding to service w/ secret key..");
        Intent serviceIntent = new Intent(IDigitalPen.class.getName()).putExtra("SecretKey",
                "this is a test");
        connectionFinished = new CountDownLatch(1);
        application.bindService(serviceIntent, serviceConnection, Context.BIND_AUTO_CREATE);
        if (!connectionFinished.await(SERVICE_CONNECTION_TIMEOUT_SECONDS, TimeUnit.SECONDS)) {
            throw new RuntimeException("Couldn't connect with service");
        }
        Log.d(TAG, "Service bound: " + service);
        return service; // TODO: setting self to self, helps test but looks odd
    }

    /**
     * Set how the off-screen area will be handled.
     * <p>
     * Takes effect after calling {@link #applyConfig()}.
     *
     * @param mode new off-screen mode.
     */
    public void setOffScreenMode(OffScreenMode mode) {
        if (mode == OffScreenMode.SYSTEM_DEFAULT) {
            appConfig.remove(AppInterfaceKeys.OFF_SCREEN_MODE);
        } else {
            appConfig.putSerializable(AppInterfaceKeys.OFF_SCREEN_MODE, mode);
        }
    }

    // TODO: find default for hover distance
    /**
     * The calling app will receive input events when the pen is hovering above
     * the writing area within a default 4cm in the Z axis, even if the pen is
     * not in-range.
     * <p>
     * Takes effect after calling {@link #applyConfig()}.
     */
    public void setHoverEnabled() {
        appConfig.putBoolean(AppInterfaceKeys.ON_SCREEN_HOVER_ENABLED, true);
        appConfig.remove(AppInterfaceKeys.ON_SCREEN_HOVER_MAX_DISTANCE); // use default
    }

    /**
     * The calling app will receive input events when the pen is hovering above
     * the writing area within the maxDistance limit in the Z axis, even if the
     * pen is not in-range.
     * <p>
     * Takes effect after calling {@link #applyConfig()}.
     *
     * @param maxDistance the distance limit in 0.1mm
     */
    public void setHoverEnabled(int maxDistance) {
        appConfig.putInt(AppInterfaceKeys.ON_SCREEN_HOVER_MAX_DISTANCE, maxDistance);
        appConfig.putBoolean(AppInterfaceKeys.ON_SCREEN_HOVER_ENABLED, true);
    }

    /**
     * The calling app will not receive pen hover events when the pen is not
     * in-range. OEM settings determine whether the app receive hovering events
     * when the pen is in-range.
     * <p>
     * Takes effect after calling {@link #applyConfig()}.
     */
    public void setHoverDisabled() {
        appConfig.remove(AppInterfaceKeys.ON_SCREEN_HOVER_ENABLED);
        appConfig.remove(AppInterfaceKeys.ON_SCREEN_HOVER_MAX_DISTANCE);
    }

    /**
     * Allows the app to ignore the current global eraser button setting, and
     * receive all pen button presses as regular button events. The eraser tool
     * is not used.
     * <p>
     * Takes effect after calling {@link #applyConfig()}.
     */
    public void setEraserBypass() {
        appConfig.putBoolean(AppInterfaceKeys.ERASER_BYPASS, true);
    }

    /**
     * Stops bypassing the global eraser button setting. The behavior of the pen
     * buttons, in respect to enabling the eraser tool type, is determined by
     * the global setting of the eraser.
     * <p>
     * Takes effect after calling {@link #applyConfig()}.
     */
    public void setEraserBypassDisabled() {
        appConfig.remove(AppInterfaceKeys.ERASER_BYPASS);
    }

    /**
     * Allows selecting the type of input events to be generated.
     * <p>
     * Takes effect after calling {@link #applyConfig()}.
     *
     * @param type the type of event that is generated by the digital pen
     *            service.
     */
    public void setInputType(InputType type) {
        appConfig.putSerializable(AppInterfaceKeys.INPUT_TYPE, type);
    }

    /**
     * Returns the current configuration's off-screen mode.
     * <p>
     * This value is for the local configuration; it will only be the service's
     * configuration if {@link #applyConfig()} was called.
     *
     * @return the current configuration's off-screen mode.
     */
    public OffScreenMode getOffScreenMode() {
        return (OffScreenMode) appConfig.getSerializable(AppInterfaceKeys.OFF_SCREEN_MODE);
    }

    /**
     * Returns whether hover is enabled in the current configuration.
     * <p>
     * This value is for the local configuration; it will only be the service's
     * configuration if {@link #applyConfig()} was called.
     *
     * @return the current configuration's hover setting.
     */
    public boolean isHoverEnabled() {
        return appConfig.containsKey(AppInterfaceKeys.ON_SCREEN_HOVER_ENABLED);
    }

    /**
     * Returns the maximum distance for hover events in the current
     * configuration.
     * <p>
     * This value is for the local configuration; it will only be the service's
     * configuration if {@link #applyConfig()} was called.
     *
     * @return the current configuration's hovering maximum distance.
     */
    public int getHoverMaxDistance() {
        return appConfig.getInt(AppInterfaceKeys.ON_SCREEN_HOVER_MAX_DISTANCE, -1);
    }

    /**
     * Returns whether the system's eraser setting is bypassed in the current
     * configuration.
     * <p>
     * This value is for the local configuration; it will only be the service's
     * configuration if {@link #applyConfig()} was called.
     *
     * @return true if the eraser is bypassed, false if it is being left to the
     *         system default.
     */
    public boolean isEraserBypassed() {
        return appConfig.containsKey(AppInterfaceKeys.ERASER_BYPASS);
    }

    /**
     * Returns the type of input event that will be generated in the currecnt
     * configuration.
     * <p>
     * This value is for the local configuration; it will only be the service's
     * configuration if {@link #applyConfig()} was called.
     *
     * @return the type of input event that will be generated.
     */
    public InputType getInputType() {
        return (InputType) appConfig.getSerializable(AppInterfaceKeys.INPUT_TYPE);
    }

    /**
     * Clears the current configuration.
     * <p>
     * Takes effect after calling {@link #applyConfig()}.
     */
    public void clearConfig() {
        // TODO: return false on error
        appConfig.clear();
    }

    /**
     * Sets the coordinate mapping for either on- or off-screen.
     * <p>
     * Takes effect after calling {@link #applyConfig()}.
     *
     * @param area {@link Area#ON_SCREEN ON_SCREEN} or {@link Area#OFF_SCREEN
     *            OFF_SCREEN}
     * @param mapping which {@link Mapping} to apply
     */
    public void setCoordinateMapping(Area area, Mapping mapping) {
        if (mapping == Mapping.NONE) {
            appConfig.remove(area.getKey());
        } else {
            appConfig.putSerializable(area.getKey(), mapping);
        }
    }

    private OnSideChannelDataListener onScreenCallback = NULL_DATA_LISTENER;

    /**
     * Register a listener to be called back with side-channel data when it
     * arrives from on-screen.
     * <p>
     * The {@link Area#OFF_SCREEN ON_SCREEN} Area should be configured with
     * {@link Mapping#SIDE_CHANNEL} or {@link Mapping#ANDROID_AND_SIDE_CHANNEL}
     * via {@link #setCoordinateMapping(Area, Mapping)}
     *
     * @param callback the listener that will be called back when there are
     *            on-screen data points. Pass null to unregister.
     */
    public synchronized void registerOnScreenCallback(OnSideChannelDataListener callback) {
        if (callback == null) {
            onScreenCallback = NULL_DATA_LISTENER;
        } else {
            onScreenCallback = callback;
        }
        // TODO: look more carefully at failure modes here
        try {
            service = ensureServiceStarted();
        } catch (InterruptedException e) {
            // TODO: return false
            e.printStackTrace();
        }
    }

    /**
     * Register a listener to be called back with side-channel data when it
     * arrives from off-screen.
     * <p>
     * The {@link Area#OFF_SCREEN OFF_SCREEN} Area should be configured with
     * {@link Mapping#SIDE_CHANNEL} or {@link Mapping#ANDROID_AND_SIDE_CHANNEL}
     * via {@link #setCoordinateMapping(Area, Mapping)}
     *
     * @param callback the listener that will be called back when there are
     *            off-screen data points. Pass null to unregister.
     */
    public synchronized void registerOffScreenCallback(OnSideChannelDataListener callback) {
        // NOTE: internally there is only one callback
        // TODO: redesign interface to account for only one side-channel
        // callback
        registerOnScreenCallback(callback);
    }

    /**
     * Returns the current mapping from the given area in this configuration.
     * <p>
     * This value is for the local configuration; it will only be the service's
     * configuration if {@link #applyConfig()} was called.
     *
     * @param area {@link Area#ON_SCREEN ON_SCREEN} or {@link Area#OFF_SCREEN
     *            OFF_SCREEN}
     * @return the current mapping.
     */
    public Mapping getCoordinateMapping(Area area) {
        if (appConfig.containsKey(area.getKey())) {
            return (Mapping) appConfig.getSerializable(area.getKey());
        } else {
            return Mapping.NONE;
        }
    }

}
