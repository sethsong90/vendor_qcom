/*===========================================================================
                           AppInterfaceTest.java

Copyright (c) 2013-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
==============================================================================*/

package com.qti.snapdragon.digitalpen.test;

import com.qti.snapdragon.digitalpen.ConfigManager;
import com.qti.snapdragon.digitalpen.DigitalPenService;
import com.qti.snapdragon.digitalpen.DigitalPenServiceCore;
import com.qti.snapdragon.digitalpen.IDigitalPen;
import com.qti.snapdragon.digitalpen.util.AppInterfaceKeys;
import com.qti.snapdragon.digitalpen.util.DigitalPenConfig;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager.OffScreenMode;

import android.os.Bundle;
import android.test.AndroidTestCase;

public class AppInterfaceTest extends AndroidTestCase {

    public ConfigManager mockConfigManager;
    public DigitalPenConfig lastConfig;

    // so Eclipse stops trying to auto-complete this function
    @Override
    public void testAndroidTestCaseSetupProperly() {
        super.testAndroidTestCaseSetupProperly();
    }

    private class MockService extends DigitalPenService {

    }

    private class TestableDigitalPenServiceCore extends DigitalPenServiceCore {

        public TestableDigitalPenServiceCore(DigitalPenService service) {
            super(service);
        }


        @Override
        protected boolean changeConfig(DigitalPenConfig config) {
            lastConfig = config;
            return true;
        }

    }

    @Override
    protected void setUp() throws Exception {
        mockService = new MockService();
        serviceCore = new TestableDigitalPenServiceCore(mockService);
    }

    private DigitalPenServiceCore serviceCore;
    private MockService mockService;

    public void testApplyConfigCallsConfigManager() throws Exception {
        IDigitalPen appInterface = serviceCore;
        Bundle config = new Bundle();
        config.putSerializable(AppInterfaceKeys.OFF_SCREEN_MODE,
                OffScreenMode.EXTEND);
        assertTrue(appInterface.applyConfig(config));
        assertEquals(DigitalPenConfig.DP_OFF_SCREEN_MODE_EXTEND, lastConfig.getOffScreenMode());
    }

    // TODO: test changing the default config
}
