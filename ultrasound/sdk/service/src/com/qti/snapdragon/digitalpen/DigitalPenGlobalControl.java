/*===========================================================================
                           DigitalPenGlobalControl.java

Copyright (c) 2013-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
==============================================================================*/

package com.qti.snapdragon.digitalpen;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import com.qti.snapdragon.digitalpen.util.DigitalPenConfig;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.os.RemoteException;

public class DigitalPenGlobalControl {

    private static final int SERVICE_ATTACH_TIMEOUT_MSECS = 500;

    public static class DigitalPenGlobalSettings {

        public enum OffScreenMode {
            DISABLED(DigitalPenConfig.DP_OFF_SCREEN_MODE_DISABLED),
            DUPLICATE(DigitalPenConfig.DP_OFF_SCREEN_MODE_DUPLICATE);

            private final byte code;

            private OffScreenMode(byte code) {
                this.code = code;
            }

            private static OffScreenMode fromCode(byte code) {
                for (OffScreenMode mode : OffScreenMode.values()) {
                    if (mode.code == code) {
                        return mode;
                    }
                }
                return null;
            }
        }

        public enum EraseMode {
            TOGGLE(DigitalPenConfig.DP_ERASE_MODE_TOGGLE),
            HOLD(DigitalPenConfig.DP_ERASE_MODE_HOLD);

            private final byte code;

            private EraseMode(byte code) {
                this.code = code;
            }

            private static EraseMode fromCode(byte code) {
                for (EraseMode mode : EraseMode.values()) {
                    if (code == mode.code) {
                        return mode;
                    }
                }
                return null;
            }
        }

        public enum PowerProfile {
            OPTIMIZE_ACCURACY(DigitalPenConfig.DP_POWER_PROFILE_OPTIMIZE_ACCURACY),
            OPTIMIZE_POWER(DigitalPenConfig.DP_POWER_PROFILE_OPTIMIZE_POWER);

            private final byte code;

            private PowerProfile(byte code) {
                this.code = code;
            }

            public static PowerProfile fromCode(byte code) {
                for (PowerProfile profile : PowerProfile.values()) {
                    if (code == profile.code) {
                        return profile;
                    }
                }
                return null;
            }

        }

        public enum Side {
            RIGHT(DigitalPenConfig.DP_PORTRAIT_SIDE_RIGHT),
            LEFT(DigitalPenConfig.DP_PORTRAIT_SIDE_LEFT);

            private byte code;

            private Side(byte code) {
                this.code = code;
            }

            private static Side fromCode(byte code) {
                for (Side side : Side.values()) {
                    if (side.code == code) {
                        return side;
                    }
                }
                return null;
            }

        }

        private final DigitalPenConfig config;

        // only constructable through factory method
        private DigitalPenGlobalSettings(DigitalPenConfig config) {
            this.config = config;
        }

        private DigitalPenConfig getConfig() {
            return config;
        }

        public DigitalPenGlobalSettings setDefaultOffScreenMode(OffScreenMode mode) {
            config.setOffScreenMode(mode.code);
            return this;
        }

        public OffScreenMode getOffScreenMode() {
            return OffScreenMode.fromCode(config.getOffScreenMode());
        }

        public DigitalPenGlobalSettings enableSmarterStand(boolean enable) {
            config.setSmarterStand(enable);
            return this;
        }

        public boolean isSmarterStandEnabled() {
            return config.isSmarterStandEnabled();
        }

        public int getInRangeDistance() {
            return config.getTouchRange();
        }

        public DigitalPenGlobalSettings setInRangeDistance(int distance) {
            config.setTouchRange(distance);
            return this;
        }

        public boolean isHoverEnabled() {
            return config.isOnScreenHoverEnabled();
        }

        public DigitalPenGlobalSettings enableHover(boolean enable) {
            config.setOnScreenHoverEnable(enable);
            return this;
        }

        public int getEraseButtonIndex() {
            return config.getEraseButtonIndex();
        }

        public EraseMode getEraseButtonMode() {
            return EraseMode.fromCode(config.getEraseButtonMode());
        }

        public DigitalPenGlobalSettings enableErase(int index, EraseMode mode) {
            config.setEraseButtonBehavior(mode.code);
            config.setEraseButtonIndex(index);
            return this;
        }

        public DigitalPenGlobalSettings disableErase() {
            config.setEraseButtonIndex(-1);
            return this;
        }

        public DigitalPenGlobalSettings setPowerProfile(PowerProfile profile) {
            config.setPowerSave(profile.code);
            return this;
        }

        public PowerProfile getPowerProfile() {
            return PowerProfile.fromCode(config.getPowerSave());
        }

        public Side getPortraitOffScreenSide() {
            return Side.fromCode(config.getOffSceenPortraitSide());
        }

        public DigitalPenGlobalSettings setPortraitOffScreenSide(Side side) {
            config.setOffScreenPortraitSide(side.code);
            return this;
        }
    }

    private final Context context;
    protected IDigitalPen attachedService;
    private CountDownLatch serviceAttachedLatch;
    private final ServiceConnection connection = new ServiceConnection() {
        @Override
        public void onServiceDisconnected(ComponentName name) {
            attachedService = null;
        }

        @Override
        public void onServiceConnected(ComponentName name, IBinder incomingService) {
            attachedService = IDigitalPen.Stub.asInterface(incomingService);
            serviceAttachedLatch.countDown();
        }
    };

    // TODO: remove local copy of config once we can get it from service
    private final DigitalPenConfig config = new DigitalPenConfig();

    public DigitalPenGlobalSettings getCurrentSettings() {
        DigitalPenGlobalSettings settings = new DigitalPenGlobalSettings(config);
        return settings;
    }

    public void commitSettings(DigitalPenGlobalSettings settings) throws RemoteException {
        IDigitalPen service = attachService();
        if (!service.loadConfig(settings.getConfig())) {
            throw new RuntimeException("Couldn't load config into service");
        }
    }

    public DigitalPenGlobalControl(Context context) {
        this.context = context;
    }

    protected IDigitalPen attachService() {
        if (attachedService == null) {
            serviceAttachedLatch = new CountDownLatch(1);
            context.bindService(new Intent(IDigitalPen.class.getName()),
                    connection,
                    Context.BIND_AUTO_CREATE);
            try {
                boolean serviceAttached = serviceAttachedLatch.await(SERVICE_ATTACH_TIMEOUT_MSECS,
                        TimeUnit.MILLISECONDS);
                if (!serviceAttached) {
                    throw new RuntimeException("Timeout while waiting to attach to service");
                }
            } catch (InterruptedException e) {
                throw new RuntimeException("Interrupted while waiting to attach to service");
            }
        }
        return attachedService;
    }

    public void disablePenFeature() throws RemoteException {
        IDigitalPen service = attachService();
        if (!service.isEnabled()) {
            return;
        }
        boolean success = service.disable();
        if (!success) {
            throw new RuntimeException("Problem disabling pen feature");
        }
    }

    public void enablePenFeature() throws RemoteException {
        IDigitalPen service = attachService();
        if (service.isEnabled()) {
            return;
        }
        boolean success = service.enable();
        if (!success) {
            throw new RuntimeException("Problem enabling pen feature");
        }
    }

    public boolean isPenFeatureEnabled() throws RemoteException {
        IDigitalPen service = attachService();
        return service.isEnabled();
    }

}
