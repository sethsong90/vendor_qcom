/*===========================================================================
                           DigitalPenManagerTest.java

Copyright (c) 2013-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
==============================================================================*/

package com.qti.digitalpen.sdk.tester.test;

import android.app.Application;
import android.app.Application.ActivityLifecycleCallbacks;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.test.AndroidTestCase;
import android.test.mock.MockApplication;

import com.qti.digitalpen.sdk.tester.DigitalPenSdkTesterActivity;
import com.qti.snapdragon.digitalpen.IDataCallback;
import com.qti.snapdragon.digitalpen.IDigitalPen;
import com.qti.snapdragon.digitalpen.IEventCallback;
import com.qti.snapdragon.digitalpen.util.AppInterfaceKeys;
import com.qti.snapdragon.digitalpen.util.DigitalPenConfig;
import com.qti.snapdragon.digitalpen.util.DigitalPenData;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager.Area;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager.Feature;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager.InputType;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager.Mapping;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager.OffScreenMode;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager.OnSideChannelDataListener;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager.SideChannelData;

public class DigitalPenManagerTest extends AndroidTestCase {

    // stops method from coming up in Eclipse's auto-complete for "test"
    @Override
    public void testAndroidTestCaseSetupProperly() {
        super.testAndroidTestCaseSetupProperly();
    }

    protected ActivityLifecycleCallbacks lifecycleCallbacks;
    public IDataCallback dataCallback;

    class MockService implements IDigitalPen {

        @Override
        public boolean applyConfig(Bundle appConfig) {
            if (forceApplyConfigException) {
                throw new RuntimeException("Forced failure");
            }
            appliedAppConfig = appConfig;
            return forceApplyConfigFail ? false : true;
        }

        @Override
        public boolean releaseActivity() {
            releaseCalled = true;
            return true;
        }

        @Override
        public boolean setOnScreenDataCallback(IDataCallback cb) {
            // can't access IDataCallback or DigitalPenData due to AIDL mess
            return true;
        }

        // shouldn't be calling these other methods of the interface...
        @Override
        public IBinder asBinder() {
            throw new UnsupportedOperationException();
        }

        @Override
        public boolean enable() throws RemoteException {
            throw new UnsupportedOperationException();
        }

        @Override
        public boolean disable() throws RemoteException {
            throw new UnsupportedOperationException();
        }

        @Override
        public boolean setDefaultConfig(DigitalPenConfig config) throws RemoteException {
            throw new UnsupportedOperationException();
        }

        @Override
        public boolean isEnabled() throws RemoteException {
            throw new UnsupportedOperationException();
        }

        @Override
        public int getDigitalPenState() throws RemoteException {
            throw new UnsupportedOperationException();
        }

        @Override
        public boolean loadConfig(DigitalPenConfig config) throws RemoteException {
            throw new UnsupportedOperationException();
        }

        @Override
        public boolean unloadConfig() throws RemoteException {
            throw new UnsupportedOperationException();
        }

        @Override
        public boolean registerDataCallback(IDataCallback cb) throws RemoteException {
            throw new UnsupportedOperationException();
        }

        @Override
        public boolean registerEventCallback(IEventCallback cb) throws RemoteException {
            throw new UnsupportedOperationException();
        }

        @Override
        public boolean unregisterDataCallback(IDataCallback cb) throws RemoteException {
            throw new UnsupportedOperationException();
        }

        @Override
        public boolean unregisterEventCallback(IEventCallback cb) throws RemoteException {
            throw new UnsupportedOperationException();
        }

    };

    public MockService mockService = new MockService();
    private DigitalPenSdkTesterActivity activity;
    private boolean releaseCalled;
    private DigitalPenManager mgr;
    private Bundle appliedAppConfig;
    protected SideChannelData lastData;
    private boolean forceApplyConfigFail;
    private boolean forceApplyConfigException;

    private class TestableDigitalPenManager extends DigitalPenManager {

        public TestableDigitalPenManager(Application app) {
            super(app);
        }

        @Override
        protected IDigitalPen ensureServiceStarted() throws InterruptedException {
            return mockService;
        }

        public void forceOnScreenData(DigitalPenData data) {
            handleOnScreenDataCallback(data);
        }
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mgr = new TestableDigitalPenManager(new MockApplication() {
            @Override
            public void registerActivityLifecycleCallbacks(ActivityLifecycleCallbacks callback) {
                lifecycleCallbacks = callback;
            }
        });
    }

    public void testPauseCallback() throws Exception {
        lifecycleCallbacks.onActivityPaused(activity);
        assertTrue(releaseCalled);
    }

    public void testResumeAppliesOldConfig() throws Exception {
        mgr.setOffScreenMode(OffScreenMode.DUPLICATE);
        mgr.applyConfig();
        assertEquals(OffScreenMode.DUPLICATE,
                appliedAppConfig.getSerializable(AppInterfaceKeys.OFF_SCREEN_MODE));
        appliedAppConfig = null;
        lifecycleCallbacks.onActivityPaused(activity);
        lifecycleCallbacks.onActivityResumed(activity);
        assertEquals(OffScreenMode.DUPLICATE,
                appliedAppConfig.getSerializable(AppInterfaceKeys.OFF_SCREEN_MODE));
    }

    public void testResumingWithNoAppliedConfigTakesNoAction() throws Exception {
        mgr.setOffScreenMode(OffScreenMode.DISABLED); // set but not applied..
        lifecycleCallbacks.onActivityPaused(activity);
        lifecycleCallbacks.onActivityResumed(activity);
        assertEquals(null, appliedAppConfig);
    }

    // TODO: resume when problem with service
    // TODO: pause when problem with service

    public void testOffScreenModeExtend() throws Exception {
        mgr.setOffScreenMode(OffScreenMode.EXTEND);
        mgr.applyConfig();
        assertEquals(OffScreenMode.EXTEND,
                appliedAppConfig.getSerializable(AppInterfaceKeys.OFF_SCREEN_MODE));
    }

    public void testOffScreenModeDefault() throws Exception {
        mgr.setOffScreenMode(OffScreenMode.DUPLICATE);
        mgr.setOffScreenMode(OffScreenMode.DISABLED);
        mgr.setOffScreenMode(OffScreenMode.SYSTEM_DEFAULT);
        mgr.applyConfig();
        assertTrue(appliedAppConfig.isEmpty());
    }

    public void testGetOffScreenModeDuplicate() throws Exception {
        mgr.setOffScreenMode(OffScreenMode.DUPLICATE);
        assertEquals(OffScreenMode.DUPLICATE, mgr.getOffScreenMode());
    }

    public void testGetOffScreenModeNull() throws Exception {
        assertEquals(null, mgr.getOffScreenMode());
    }

    public void testEnableHovering() throws Exception {
        mgr.setHoverEnabled();
        mgr.applyConfig();
        assertTrue(appliedAppConfig.getBoolean(AppInterfaceKeys.ON_SCREEN_HOVER_ENABLED));
    }

    public void testEnableHoveringWithDistance() throws Exception {
        mgr.setHoverEnabled(200);
        mgr.applyConfig();
        assertTrue(appliedAppConfig.getBoolean(AppInterfaceKeys.ON_SCREEN_HOVER_ENABLED));
        assertEquals(200, appliedAppConfig.getInt(AppInterfaceKeys.ON_SCREEN_HOVER_MAX_DISTANCE));
    }

    public void testEnablingDefaultHoveringResetsMaxDistance() throws Exception {
        mgr.setHoverEnabled(200);
        mgr.setHoverEnabled();
        mgr.applyConfig();
        assertTrue(appliedAppConfig.getBoolean(AppInterfaceKeys.ON_SCREEN_HOVER_ENABLED));
        assertFalse(appliedAppConfig.containsKey(AppInterfaceKeys.ON_SCREEN_HOVER_MAX_DISTANCE));
    }

    public void testDisableHovering() throws Exception {
        mgr.setHoverEnabled(120);
        mgr.applyConfig();
        mgr.setHoverDisabled();
        mgr.applyConfig();
        assertTrue(appliedAppConfig.isEmpty());
    }

    public void testIsHoverEnabled() throws Exception {
        mgr.setHoverEnabled();
        assertTrue(mgr.isHoverEnabled());
        mgr.setHoverDisabled();
        assertFalse(mgr.isHoverEnabled());
    }

    public void testGetHoverDisatance() throws Exception {
        mgr.setHoverEnabled(220);
        assertEquals(220, mgr.getHoverMaxDistance());
        mgr.setHoverDisabled();
        assertEquals("Should return -1 if hover disabled", -1, mgr.getHoverMaxDistance());
    }

    public void testEnableEraserBypass() throws Exception {
        mgr.setEraserBypass();
        mgr.applyConfig();
        assertTrue(appliedAppConfig.getBoolean(AppInterfaceKeys.ERASER_BYPASS));
    }

    public void testDisableEraserBypass() throws Exception {
        mgr.setEraserBypass();
        mgr.applyConfig();
        mgr.setEraserBypassDisabled();
        mgr.applyConfig();
        assertTrue(appliedAppConfig.isEmpty());
    }

    public void testIsEraserBypassed() throws Exception {
        assertFalse(mgr.isEraserBypassed());
        mgr.setEraserBypass();
        assertTrue(mgr.isEraserBypassed());
    }

    public void testSetInputType() throws Exception {
        mgr.setInputType(InputType.STYLUS);
        mgr.setInputType(InputType.MOUSE);
        mgr.setInputType(InputType.JOYSTICK);
        mgr.applyConfig();
        assertEquals(InputType.JOYSTICK,
                appliedAppConfig.getSerializable(AppInterfaceKeys.INPUT_TYPE));
    }

    public void testGetInputType() throws Exception {
        mgr.setInputType(InputType.MOUSE);
        assertEquals(InputType.MOUSE, mgr.getInputType());
    }

    public void testClearConfig() throws Exception {
        mgr.setOffScreenMode(OffScreenMode.EXTEND);
        mgr.setEraserBypass();
        mgr.setHoverEnabled(300);
        mgr.setInputType(InputType.STYLUS);
        mgr.applyConfig();
        mgr.clearConfig();
        mgr.applyConfig();
        assertTrue(appliedAppConfig.isEmpty());

    }

    public void testSetOffScreenCoordinateMapping() throws Exception {
        mgr.setCoordinateMapping(Area.OFF_SCREEN, Mapping.SIDE_CHANNEL);
        mgr.applyConfig();
        assertEquals(Mapping.SIDE_CHANNEL,
                appliedAppConfig.getSerializable(AppInterfaceKeys.OFF_SCREEN_MAPPING));
    }

    public void testSetOnScreenCoordinateMapping() throws Exception {
        mgr.setCoordinateMapping(Area.ON_SCREEN, Mapping.ANDROID);
        mgr.applyConfig();
        assertEquals(Mapping.ANDROID,
                appliedAppConfig.getSerializable(AppInterfaceKeys.ON_SCREEN_MAPPING));
    }

    public void testBothMapping() throws Exception {
        mgr.setCoordinateMapping(Area.OFF_SCREEN, Mapping.ANDROID_AND_SIDE_CHANNEL);
        mgr.applyConfig();
        assertEquals(Mapping.ANDROID_AND_SIDE_CHANNEL,
                appliedAppConfig.getSerializable(AppInterfaceKeys.OFF_SCREEN_MAPPING));
    }

    public void testUnMapping() throws Exception {
        mgr.setCoordinateMapping(Area.OFF_SCREEN, Mapping.ANDROID_AND_SIDE_CHANNEL);
        mgr.applyConfig();
        mgr.setCoordinateMapping(Area.OFF_SCREEN, Mapping.NONE);
        mgr.applyConfig();
        assertTrue("Bundle isn't empty: " + appliedAppConfig, appliedAppConfig.isEmpty());
    }

    public void testGetMappingSideChannel() throws Exception {
        mgr.setCoordinateMapping(Area.ON_SCREEN, Mapping.SIDE_CHANNEL);
        assertEquals(Mapping.SIDE_CHANNEL, mgr.getCoordinateMapping(Area.ON_SCREEN));
    }

    public void testGetMappingNone() throws Exception {
        assertEquals(Mapping.NONE, mgr.getCoordinateMapping(Area.ON_SCREEN));
    }

    public void testBasicServicesFeature() throws Exception {
        assertTrue(DigitalPenManager.isFeatureSupported(Feature.BASIC_DIGITAL_PEN_SERVICES));
    }

    public void testRegisterAndUnregisterCallback() throws Exception {
        mgr.registerOnScreenCallback(new OnSideChannelDataListener() {

            @Override
            public void onDigitalPenData(SideChannelData data) {
                lastData = data;
            }
        });
        TestableDigitalPenManager testHandle = (TestableDigitalPenManager) mgr;
        DigitalPenData data = new DigitalPenData(42, 0, 0, 0, 0, 0, 0, false);
        testHandle.forceOnScreenData(data);
        assertEquals(42, lastData.xPos);
        lastData = null;
        mgr.registerOnScreenCallback(null);
        testHandle.forceOnScreenData(data);
        assertEquals(null, lastData);
    }

    public void testApplyConfigFailure() throws Exception {
        forceApplyConfigFail = true;
        assertFalse(mgr.applyConfig());
    }

    public void testApplyConfigException() throws Exception {
        forceApplyConfigException = true;
        assertFalse(mgr.applyConfig());
    }
}
