/*===========================================================================
                           ConfigChangerTest.java

Copyright (c) 2013-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
==============================================================================*/
package com.qti.snapdragon.digitalpen.test;

import com.qti.snapdragon.digitalpen.SidebandConfigChanger;
import com.qti.snapdragon.digitalpen.util.DigitalPenConfig;

import android.content.Intent;
import android.test.AndroidTestCase;

public class SidebandConfigChangerTest extends AndroidTestCase {

    private static final String PACKAGE_PREFIX = "com.qti.snapdragon.digitalpen.";
    private Intent loadConfigIntent;
    private DigitalPenConfig config;
    private SidebandConfigChanger changer;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        loadConfigIntent = new Intent("com.qti.snapdragon.digitalpen.LOAD_CONFIG");
        config = new DigitalPenConfig();
        changer = new SidebandConfigChanger(config);
    }

    public void testExceptionForBadAction() throws Exception {
        Intent i = new Intent("howdy");
        DigitalPenConfig config = new DigitalPenConfig();
        SidebandConfigChanger changer = new SidebandConfigChanger(config);
        try {
            config = changer.processIntent(i);
            fail("Should have thrown invalid action exception");
        } catch (IllegalArgumentException x) {
            assertEquals("Expected exception", "Unexpected action: howdy", x.getMessage());
        }
    }

    public void testChangeOffScreenMode() throws Exception {
        loadConfigIntent.putExtra(PACKAGE_PREFIX + "OffScreenMode", "2");
        DigitalPenConfig newConfig = changer.processIntent(loadConfigIntent);
        assertEquals(2, newConfig.getOffScreenMode());
    }

    public void testUseSmarterStand() throws Exception {
        loadConfigIntent.putExtra(PACKAGE_PREFIX + "UseSmarterStand", "true");
        DigitalPenConfig newConfig = changer.processIntent(loadConfigIntent);
        assertTrue(newConfig.isSmarterStandEnabled());
    }

    public void testOffScreenPlane() throws Exception {
        loadConfigIntent.putExtra(PACKAGE_PREFIX + "OffScreenPlane",
                new float[] {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f});
        DigitalPenConfig newConfig = changer.processIntent(loadConfigIntent);
        float[] origin = new float[3];
        float[] endX = new float[3];
        float[] endY = new float[3];
        newConfig.getOffScreenPlane(origin, endX, endY);
        assertEquals(1.0f, origin[0]);
        assertEquals(2.0f, origin[1]);
        assertEquals(3.0f, origin[2]);
        assertEquals(4.0f, endX[0]);
        assertEquals(5.0f, endX[1]);
        assertEquals(6.0f, endX[2]);
        assertEquals(7.0f, endY[0]);
        assertEquals(8.0f, endY[1]);
        assertEquals(9.0f, endY[2]);
    }

    public void testOffScreenPlaneChecksSize() throws Exception {
        loadConfigIntent.putExtra(PACKAGE_PREFIX + "OffScreenPlane",
                new float[] {1.0f});
        try {
            changer.processIntent(loadConfigIntent);
            fail("Expected bad array size to throw an error");
        } catch (IllegalArgumentException x) {
            assertEquals(
                    "For key com.qti.snapdragon.digitalpen.OffScreenPlane, expected array of length 9, received length 1",
                    x.getMessage());
        }
    }
}