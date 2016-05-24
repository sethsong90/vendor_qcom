/*===========================================================================
                           ConfigManager.java

Copyright (c) 2013-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
==============================================================================*/

package com.qti.snapdragon.digitalpen;

import com.qti.snapdragon.digitalpen.util.AppInterfaceKeys;
import com.qti.snapdragon.digitalpen.util.DigitalPenConfig;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager.Mapping;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager.OffScreenMode;

import android.os.Bundle;
import android.os.Parcel;
import android.util.Log;

public class ConfigManager {

    private static final String TAG = "ConfigManager";
    private DigitalPenConfig defaultConfig;
    private Bundle appConfig = new Bundle();

    public ConfigManager(DigitalPenConfig defaultConfig) {
        this.defaultConfig = defaultConfig;
    }

    public void setAppConfig(Bundle config) {
        appConfig = config;
    }

    public void setDefaultConfig(DigitalPenConfig defaultConfig) {
        this.defaultConfig = defaultConfig;
    }

    public DigitalPenConfig getConfig() {
        DigitalPenConfig currentConfig = copyOfDefaultConfig();

        // TODO: dispatch table or reflection
        if (appConfig.containsKey(AppInterfaceKeys.OFF_SCREEN_MODE)) {
            DigitalPenManager.OffScreenMode mode = (OffScreenMode) appConfig
                    .getSerializable(AppInterfaceKeys.OFF_SCREEN_MODE);
            byte configMode;
            switch (mode) {
                case DISABLED:
                    configMode = DigitalPenConfig.DP_OFF_SCREEN_MODE_DISABLED;
                    break;
                case DUPLICATE:
                    configMode = DigitalPenConfig.DP_OFF_SCREEN_MODE_DUPLICATE;
                    break;
                case EXTEND:
                    configMode = DigitalPenConfig.DP_OFF_SCREEN_MODE_EXTEND;
                    break;
                case SYSTEM_DEFAULT:
                    configMode = currentConfig.getOffScreenMode();
                    break;
                default:
                    throw new RuntimeException("Unknown mode: " + mode);
            }
            currentConfig.setOffScreenMode(configMode);
        }

        if (appConfig.containsKey(AppInterfaceKeys.ON_SCREEN_HOVER_ENABLED)) {
            currentConfig.setOnScreenHoverEnable(appConfig.getBoolean(AppInterfaceKeys.ON_SCREEN_HOVER_ENABLED));
        }

        if (appConfig.containsKey(AppInterfaceKeys.ERASER_BYPASS)) {
            currentConfig.setEraseButtonIndex(-1);
        }

        if (appConfig.containsKey(AppInterfaceKeys.INPUT_TYPE)) {
            // TODO: implement INPUT_TYPE
            Log.w(TAG, "Implementation of Input Type for Digital Pen is incomplete");
        }

        // TODO: figure out way to share code w/ OFF_SCREEN_MAPPING
        if (appConfig.containsKey(AppInterfaceKeys.ON_SCREEN_MAPPING)) {
            Mapping mapping = (Mapping) appConfig
                    .getSerializable(AppInterfaceKeys.ON_SCREEN_MAPPING);
            byte destination = convertMappingToDestination(mapping);
            // TODO: isMapped is always false; need to vary through interface
            currentConfig.setOnScreenCoordReporting(destination, false);
        }

        if (appConfig.containsKey(AppInterfaceKeys.OFF_SCREEN_MAPPING)) {
            Mapping mapping = (Mapping) appConfig
                    .getSerializable(AppInterfaceKeys.OFF_SCREEN_MAPPING);
            byte destination = convertMappingToDestination(mapping);
            // TODO: isMapped is always false; need to vary through interface
            currentConfig.setOffScreenCoordReporting(destination, false);
        }

        return currentConfig;
    }

    protected byte convertMappingToDestination(Mapping mapping) {
        byte destination = 0;
        switch (mapping) {
            case ANDROID:
                destination = DigitalPenConfig.DP_COORD_DESTINATION_MOTION_EVENT;
                break;
            case ANDROID_AND_SIDE_CHANNEL:
                destination = DigitalPenConfig.DP_COORD_DESTINATION_BOTH;
                break;
            case NONE:
                throw new UnsupportedOperationException();
            case SIDE_CHANNEL:
                destination = DigitalPenConfig.DP_COORD_DESTINATION_SOCKET;
                break;
            default:
                throw new UnsupportedOperationException();
        }
        return destination;
    }

    // parcels & unparcels object in lieu of writing clone or copy ctor
    protected DigitalPenConfig copyOfDefaultConfig() {
        Parcel parcel = Parcel.obtain();
        defaultConfig.writeToParcel(parcel, 0);
        parcel.setDataPosition(0);
        DigitalPenConfig currentConfig = DigitalPenConfig.CREATOR.createFromParcel(parcel);
        parcel.recycle();
        return currentConfig;
    }

}
