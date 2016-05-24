/*===========================================================================
                           OemInterfaceTest.java

Copyright (c) 2013-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
==============================================================================*/

package com.qti.snapdragon.digitalpen.test;

import com.qti.snapdragon.digitalpen.DigitalPenGlobalControl;
import com.qti.snapdragon.digitalpen.DigitalPenGlobalControl.DigitalPenGlobalSettings;
import com.qti.snapdragon.digitalpen.DigitalPenGlobalControl.DigitalPenGlobalSettings.EraseMode;
import com.qti.snapdragon.digitalpen.DigitalPenGlobalControl.DigitalPenGlobalSettings.OffScreenMode;
import com.qti.snapdragon.digitalpen.DigitalPenGlobalControl.DigitalPenGlobalSettings.PowerProfile;
import com.qti.snapdragon.digitalpen.DigitalPenGlobalControl.DigitalPenGlobalSettings.Side;
import com.qti.snapdragon.digitalpen.IDataCallback;
import com.qti.snapdragon.digitalpen.IDigitalPen;
import com.qti.snapdragon.digitalpen.IEventCallback;
import com.qti.snapdragon.digitalpen.util.DigitalPenConfig;

import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.test.AndroidTestCase;
import android.test.mock.MockContext;

public class OemInterfaceTest extends AndroidTestCase {

    public DigitalPenConfig lastConfig;

    class MockDigitalPen implements IDigitalPen {

        public boolean forceLoadConfigThrowsRemoteException;
        public boolean forceLoadConfigFail;
        public boolean enabled;
        public int enableCallCount;
        public boolean forceErrorOnEnable;
        public int disableCallCount;
        public boolean forceErrorOnDisable;

        @Override
        public IBinder asBinder() {
            return null;
        }

        @Override
        public boolean enable() throws RemoteException {
            enabled = true;
            ++enableCallCount;
            return forceErrorOnEnable ? false : true;
        }

        @Override
        public boolean disable() throws RemoteException {
            enabled = false;
            ++disableCallCount;
            return forceErrorOnDisable ? false : true;
        }

        @Override
        public boolean setDefaultConfig(DigitalPenConfig config) throws RemoteException {
            return false;
        }

        @Override
        public boolean isEnabled() throws RemoteException {
            return enabled;
        }

        @Override
        public int getDigitalPenState() throws RemoteException {
            return 0;
        }

        @Override
        public boolean loadConfig(DigitalPenConfig config) throws RemoteException {
            if (forceLoadConfigThrowsRemoteException) {
                throw new RemoteException("Being forced to fail");
            }
            lastConfig = config;
            return forceLoadConfigFail ? false : true;
        }

        @Override
        public boolean unloadConfig() throws RemoteException {
            return false;
        }

        @Override
        public boolean registerDataCallback(IDataCallback cb) throws RemoteException {
            return false;
        }

        @Override
        public boolean registerEventCallback(IEventCallback cb) throws RemoteException {
            return false;
        }

        @Override
        public boolean unregisterDataCallback(IDataCallback cb) throws RemoteException {
            return false;
        }

        @Override
        public boolean unregisterEventCallback(IEventCallback cb) throws RemoteException {
            return false;
        }

        @Override
        public boolean applyConfig(Bundle config) throws RemoteException {
            return false;
        }

        @Override
        public boolean releaseActivity() throws RemoteException {
            return false;
        }

        @Override
        public boolean setOnScreenDataCallback(IDataCallback cb) throws RemoteException {
            return false;
        }

    }

    private MockDigitalPen mockDigitalPen;
    private TestableDigitalPenGlobalControl control;
    private DigitalPenGlobalSettings settings;

    class TestableDigitalPenGlobalControl extends DigitalPenGlobalControl {

        public TestableDigitalPenGlobalControl() {
            super(new MockContext());
        }

        @Override
        protected IDigitalPen attachService() {
            return mockDigitalPen;
        }

    };

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mockDigitalPen = new MockDigitalPen();
        control = new TestableDigitalPenGlobalControl();
        settings = control.getCurrentSettings();
    }

    public void testEnablePenFeature() throws Exception {
        assertFalse(mockDigitalPen.enabled);
        control.enablePenFeature();
        assertTrue(mockDigitalPen.enabled);
    }

    public void testEnablePenFeatureWhenEnabled() throws Exception {
        mockDigitalPen.enabled = true;
        control.enablePenFeature();
        assertTrue(mockDigitalPen.enabled);
        assertEquals(0, mockDigitalPen.enableCallCount);
    }

    public void testEnablePenFeatureThrowsIfCallFails() throws Exception {
        mockDigitalPen.forceErrorOnEnable = true;
        try {
            control.enablePenFeature();
            fail("Expected thrown error");
        } catch (RuntimeException x) {
            assertEquals("Problem enabling pen feature", x.getMessage());
        }

    }

    public void testDisablePenFeature() throws Exception {
        mockDigitalPen.enabled = true;
        control.disablePenFeature();
        assertFalse(mockDigitalPen.enabled);
    }

    public void testDisablePenFeatureWhenDisabled() throws Exception {
        mockDigitalPen.enabled = false;
        control.disablePenFeature();
        assertFalse(mockDigitalPen.enabled);
        assertEquals(0, mockDigitalPen.disableCallCount);
    }

    public void testDisablePenFeatureThrowsIfCallFails() throws Exception {
        mockDigitalPen.forceErrorOnDisable = true;
        mockDigitalPen.enabled = true;
        try {
            control.disablePenFeature();
            fail("Expected thrown error");
        } catch (RuntimeException x) {
            assertEquals("Problem disabling pen feature", x.getMessage());
        }

    }

    public void testOffScreenModeDisable() throws Exception {
        settings.setDefaultOffScreenMode(OffScreenMode.DISABLED);
        control.commitSettings(settings);
        assertEquals(DigitalPenConfig.DP_OFF_SCREEN_MODE_DISABLED, lastConfig.getOffScreenMode());
    }

    public void testOffScreenModeDuplicate() throws Exception {
        settings.setDefaultOffScreenMode(OffScreenMode.DUPLICATE);
        control.commitSettings(settings);
        assertEquals(DigitalPenConfig.DP_OFF_SCREEN_MODE_DUPLICATE, lastConfig.getOffScreenMode());
    }

    public void testCommitSettingsRemoteException() throws Exception {
        mockDigitalPen.forceLoadConfigThrowsRemoteException = true;
        try {
            control.commitSettings(settings);
            fail("Expected this to throw an exception");
        } catch (RemoteException x) {
            // expected
        }
    }

    public void testLoadConfigFailWithinCommitSettings() throws Exception {
        mockDigitalPen.forceLoadConfigFail = true;
        try {
            control.commitSettings(settings);
            fail("Expected this to throw an exception");
        } catch (RuntimeException x) {
            // expected
        }
    }

    public void testSmarterStandEnable() throws Exception {
        assertTrue(settings.isSmarterStandEnabled());
        settings.enableSmarterStand(false);
        control.commitSettings(settings);
        assertFalse(lastConfig.isSmarterStandEnabled());
    }

    public void testInRangeDistance() throws Exception {
        assertEquals(40, settings.getInRangeDistance());
        settings.setInRangeDistance(42);
        control.commitSettings(settings);
        assertEquals(42, lastConfig.getTouchRange());
    }

    public void testHovering() throws Exception {
        assertTrue(settings.isHoverEnabled());
        settings.enableHover(false);
        control.commitSettings(settings);
        assertFalse(lastConfig.isOnScreenHoverEnabled());
    }

    public void testEraseEnable() throws Exception {
        settings.enableErase(2, EraseMode.TOGGLE);
        control.commitSettings(settings);
        assertEquals(2, lastConfig.getEraseButtonIndex());
        assertEquals(DigitalPenConfig.DP_ERASE_MODE_TOGGLE, lastConfig.getEraseButtonMode());
    }

    public void testEraserModeDisable() throws Exception {
        settings.enableErase(1, EraseMode.TOGGLE);
        control.commitSettings(settings);
        settings.disableErase();
        control.commitSettings(settings);
        assertEquals(-1, lastConfig.getEraseButtonIndex());
    }

    public void testPowerProfile() throws Exception {
        assertEquals(PowerProfile.OPTIMIZE_ACCURACY, settings.getPowerProfile());
        settings.setPowerProfile(PowerProfile.OPTIMIZE_POWER);
        control.commitSettings(settings);
        assertEquals(DigitalPenConfig.DP_POWER_PROFILE_OPTIMIZE_POWER, lastConfig.getPowerSave());
    }

    public void testSetPortraitOffScreenSide() throws Exception {
        assertEquals(Side.RIGHT, settings.getPortraitOffScreenSide());
        settings.setPortraitOffScreenSide(Side.LEFT);
        control.commitSettings(settings);
        assertEquals(DigitalPenConfig.DP_PORTRAIT_SIDE_LEFT, lastConfig.getOffSceenPortraitSide());
    }

    public void testMultiSetting() throws Exception {
        control.enablePenFeature();
        settings
                .enableErase(1, EraseMode.HOLD)
                .enableSmarterStand(true)
                .setPortraitOffScreenSide(Side.LEFT);
        control.commitSettings(settings);
        assertTrue(lastConfig.isSmarterStandEnabled());
        assertEquals(DigitalPenConfig.DP_ERASE_MODE_HOLD, lastConfig.getEraseButtonMode());
        assertEquals(1, lastConfig.getEraseButtonIndex());
        assertEquals(DigitalPenConfig.DP_PORTRAIT_SIDE_LEFT, lastConfig.getOffSceenPortraitSide());
    }
}
