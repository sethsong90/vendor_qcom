/*===========================================================================
                           OemInterfaceIntegrationTest.java

Copyright (c) 2013-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
==============================================================================*/

package com.qti.snapdragon.digitalpen.test;

import com.qti.snapdragon.digitalpen.DigitalPenGlobalControl;

import android.test.AndroidTestCase;

public class OemInterfaceIntegrationTest extends AndroidTestCase {

    private DigitalPenGlobalControl settings;

    public void testEnable() throws Exception {
        settings = new DigitalPenGlobalControl(getContext());
        if (!settings.isPenFeatureEnabled()) {
            settings.enablePenFeature();
        }
        assertTrue(settings.isPenFeatureEnabled());
        settings.disablePenFeature();
        assertFalse(settings.isPenFeatureEnabled());
        settings.enablePenFeature();
        assertTrue(settings.isPenFeatureEnabled());
    }
}
