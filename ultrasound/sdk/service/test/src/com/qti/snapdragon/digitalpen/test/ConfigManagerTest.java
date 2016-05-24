/*===========================================================================
                           ConfigManagerTest.java

Copyright (c) 2013-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
==============================================================================*/

package com.qti.snapdragon.digitalpen.test;

import java.util.Arrays;

import android.os.Bundle;

import com.qti.snapdragon.digitalpen.ConfigManager;
import com.qti.snapdragon.digitalpen.util.AppInterfaceKeys;
import com.qti.snapdragon.digitalpen.util.DigitalPenConfig;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager.Mapping;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager.OffScreenMode;

import junit.framework.TestCase;

public class ConfigManagerTest extends TestCase {
    private DigitalPenConfig defaultConfig;
    private ConfigManager configManager;
    private Bundle appConfig;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        defaultConfig = new DigitalPenConfig();
        configManager = new ConfigManager(defaultConfig);
        appConfig = new Bundle();
    }

    private void assertConfigEquals(DigitalPenConfig expected, DigitalPenConfig config) {
        assertTrue(Arrays.equals(expected.marshalForDaemon(), config.marshalForDaemon()));
    }

    public void testDefault() throws Exception {
        assertConfigEquals(defaultConfig, configManager.getConfig());
    }

    public void testOffScreenMode() throws Exception {
        assertEquals(DigitalPenConfig.DP_OFF_SCREEN_MODE_DISABLED, defaultConfig.getOffScreenMode());
        appConfig.putSerializable(AppInterfaceKeys.OFF_SCREEN_MODE,
                OffScreenMode.EXTEND);
        configManager.setAppConfig(appConfig);
        assertEquals(DigitalPenConfig.DP_OFF_SCREEN_MODE_EXTEND, configManager.getConfig()
                .getOffScreenMode());
    }

    public void testHoverOemDisableAppEnable() throws Exception {
        defaultConfig.setOnScreenHoverEnable(false);
        defaultConfig.setOnScreenHoverMaxRange(42);
        configManager.setDefaultConfig(defaultConfig);

        appConfig.putBoolean(AppInterfaceKeys.ON_SCREEN_HOVER_ENABLED, true);

        configManager.setAppConfig(appConfig);
        DigitalPenConfig config = configManager.getConfig();
        assertTrue(config.isOnScreenHoverEnabled());
        assertEquals(42, config.getOnScreenHoverMaxRange());
    }

    public void DISABLED_testHoverOemEnableAppDisable() throws Exception {
        // TODO: need clarification on requirements here
    }

    public void testAppEraserBypassEnabled() throws Exception {
        defaultConfig.setEraseButtonIndex(0);
        configManager.setDefaultConfig(defaultConfig);

        appConfig.putBoolean(AppInterfaceKeys.ERASER_BYPASS, true);
        configManager.setAppConfig(appConfig);

        assertEquals(-1, configManager.getConfig().getEraseButtonIndex());
    }

    public void testAppStopsEraserBypass() throws Exception {
        defaultConfig.setEraseButtonIndex(0);
        configManager.setDefaultConfig(defaultConfig);

        // TODO: might be better to talk to a wrapper class rather than bundle;
        // this is silly
        appConfig.putBoolean(AppInterfaceKeys.ERASER_BYPASS, true);
        appConfig.remove(AppInterfaceKeys.ERASER_BYPASS);
        configManager.setAppConfig(appConfig);

        assertEquals(0, configManager.getConfig().getEraseButtonIndex());
    }

    public void testAppSetsOnScreenMapping() throws Exception {
        // TODO: mapped/unmapped isn't correct, see HLD:
        /*
         * "iii) isMapped Relevant to side-channel mode only. If true,
         * coordinates are given as pixel location on relevant display. If
         * false, coordinates are given in raw millimeters, regardless of the
         * display's size or location."
         */
        defaultConfig.setOnScreenCoordReporting(DigitalPenConfig.DP_COORD_DESTINATION_MOTION_EVENT,
                true);
        configManager.setDefaultConfig(defaultConfig);

        appConfig.putSerializable(AppInterfaceKeys.ON_SCREEN_MAPPING,
                Mapping.ANDROID_AND_SIDE_CHANNEL);
        configManager.setAppConfig(appConfig);

        assertEquals(DigitalPenConfig.DP_COORD_DESTINATION_BOTH, configManager.getConfig()
                .getOnScreenCoordReportDestination());
        assertFalse(configManager.getConfig().getOnScreenCoordReportIsMapped());

    }

    public void testAppSetsOnScreenMappingAndroidOnly() throws Exception {
        // TODO: remove copy/paste
        defaultConfig.setOnScreenCoordReporting(DigitalPenConfig.DP_COORD_DESTINATION_BOTH,
                true);
        configManager.setDefaultConfig(defaultConfig);

        appConfig.putSerializable(AppInterfaceKeys.ON_SCREEN_MAPPING,
                Mapping.ANDROID);
        configManager.setAppConfig(appConfig);

        assertEquals(DigitalPenConfig.DP_COORD_DESTINATION_MOTION_EVENT, configManager.getConfig()
                .getOnScreenCoordReportDestination());
    }

    public void testAppSetsOnScreenSocketOnly() throws Exception {
        // TODO: remove copy/paste
        defaultConfig.setOnScreenCoordReporting(DigitalPenConfig.DP_COORD_DESTINATION_BOTH,
                true);
        configManager.setDefaultConfig(defaultConfig);

        appConfig.putSerializable(AppInterfaceKeys.ON_SCREEN_MAPPING,
                Mapping.SIDE_CHANNEL);
        configManager.setAppConfig(appConfig);

        assertEquals(DigitalPenConfig.DP_COORD_DESTINATION_SOCKET, configManager.getConfig()
                .getOnScreenCoordReportDestination());
        assertFalse(configManager.getConfig().getOnScreenCoordReportIsMapped());
    }

    public void testAppSetsOffScreenSocketOnly() throws Exception {
        // TODO: remove copy/paste
        defaultConfig.setOffScreenCoordReporting(DigitalPenConfig.DP_COORD_DESTINATION_BOTH,
                true);
        configManager.setDefaultConfig(defaultConfig);

        appConfig.putSerializable(AppInterfaceKeys.OFF_SCREEN_MAPPING,
                Mapping.SIDE_CHANNEL);
        configManager.setAppConfig(appConfig);

        assertEquals(DigitalPenConfig.DP_COORD_DESTINATION_SOCKET, configManager.getConfig()
                .getOffScreenCoordReportDestination());
        assertFalse(configManager.getConfig().getOnScreenCoordReportIsMapped());
    }

}
