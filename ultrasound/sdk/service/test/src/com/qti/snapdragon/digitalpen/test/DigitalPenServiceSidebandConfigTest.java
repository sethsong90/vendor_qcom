/*===========================================================================
                           DigitalPenBinderSidebandConfigTest.java

Copyright (c) 2013-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
==============================================================================*/
package com.qti.snapdragon.digitalpen.test;

import com.qti.snapdragon.digitalpen.SidebandConfigChanger;
import com.qti.snapdragon.digitalpen.DigitalPenServiceCore;
import com.qti.snapdragon.digitalpen.DigitalPenService;
import com.qti.snapdragon.digitalpen.util.DigitalPenConfig;

import android.content.Intent;
import android.test.AndroidTestCase;

public class DigitalPenServiceSidebandConfigTest extends AndroidTestCase {

    public class MockConfigChanger extends SidebandConfigChanger {

        public MockConfigChanger(DigitalPenConfig config) {
            super(config);
            updatedConfig = config;
        }

        @Override
        public DigitalPenConfig processIntent(Intent intent) {
            updateInputIntent = intent;
            return updatedConfig;
        }

    }

    public DigitalPenConfig loadedConfig;
    public DigitalPenConfig updatedConfig;
    public Intent updateInputIntent;
    private TestableDigitalPenServiceCore service;

    private class TestableDigitalPenServiceCore extends DigitalPenServiceCore {

        public TestableDigitalPenServiceCore() {
            super(new DigitalPenService());
        }

        @Override
        public boolean loadConfig(DigitalPenConfig config) {
            loadedConfig = config;
            return true;
        }

        @Override
        protected SidebandConfigChanger createConfigChanger(DigitalPenConfig config) {
            return new MockConfigChanger(config);
        }

        @Override
        public boolean isEnabled() {
            return true;

        }

        @Override
        public boolean enable() {
            return true;
        }

        @Override
        public boolean disable() {
            return true;
        }
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        service = new TestableDigitalPenServiceCore();
    }

    public void testStartCmdCallsLoadConfig() throws Exception {
        Intent intent = new Intent();
        service.onSidebandLoadConfig(intent);
        assertNotNull(loadedConfig);
        assertEquals(intent, updateInputIntent);
        assertEquals(loadedConfig, updatedConfig);
    }

    public void testUpdatesSameObjectInSubsequentCalls() throws Exception {
        Intent intent = new Intent();
        service.onSidebandLoadConfig(intent);
        assertNotNull(loadedConfig);
        DigitalPenConfig configFromFirstCall = loadedConfig;
        service.onSidebandLoadConfig(intent);
        assertEquals("Expected same object to be re-used to keep values from last call",
                configFromFirstCall, loadedConfig);

    }
}
