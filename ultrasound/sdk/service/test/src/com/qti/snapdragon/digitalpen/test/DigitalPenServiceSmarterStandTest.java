/*===========================================================================
                           DigitalPenServiceSmarterStandTest.java

Copyright (c) 2013-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
==============================================================================*/

package com.qti.snapdragon.digitalpen.test;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import com.qti.snapdragon.digitalpen.DigitalPenServiceCore;
import com.qti.snapdragon.digitalpen.DigitalPenService;
import com.qti.snapdragon.digitalpen.SmarterStandSensorListener;
import com.qti.snapdragon.digitalpen.SmarterStandSensorListener.AccelerometerChangeCallback;
import com.qti.snapdragon.digitalpen.util.DigitalPenConfig;

import android.test.AndroidTestCase;

public class DigitalPenServiceSmarterStandTest extends AndroidTestCase {
    public int lastMsgId;
    public byte[] lastMsgBuf;

    class MockSmarterStandSensorListener extends SmarterStandSensorListener {
        public AccelerometerChangeCallback callback;
        public boolean isRunning;

        public MockSmarterStandSensorListener(AccelerometerChangeCallback callback) {
            super(null, null);
            this.callback = callback;
        }

        @Override
        public void start() {
            isRunning = true;
        }

        @Override
        public void stop() {
            isRunning = false;
        }

    };

    private MockSmarterStandSensorListener mockSmarterStandSensorListener;
    private TestableDigitalPenService service;
    private int createCalledCount;

    private class TestableDigitalPenService extends DigitalPenServiceCore {

        public TestableDigitalPenService() {
            super(new DigitalPenService());
        }

        @Override
        protected boolean sendControlMessage(int msgId, byte[] msgBuf) {
            lastMsgId = msgId;
            lastMsgBuf = msgBuf;
            return true;
        }

        @Override
        protected SmarterStandSensorListener createSmarterStandSensorListener(
                AccelerometerChangeCallback callback) {
            createCalledCount++;
            mockSmarterStandSensorListener = new MockSmarterStandSensorListener(callback);
            return mockSmarterStandSensorListener;
        }

    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        DigitalPenConfig config = new DigitalPenConfig();
        config.setSmarterStand(true);
        service = new TestableDigitalPenService();
        service.loadConfig(config);
    }

    public void testActivateSmarterStand() throws Exception {
        assertNotNull(mockSmarterStandSensorListener);
        assertTrue(mockSmarterStandSensorListener.isRunning);
    }

    public void testListenerNotCreatedWhenSmarterStandActive() throws Exception {
        assertEquals(1, createCalledCount);
        service.loadConfig(new DigitalPenConfig().setSmarterStand(true));
        assertEquals("Smarter stand should still be active, no need to re-create",
                1, createCalledCount);
    }

    public void testListenerReleasedWhenSmarterStandDeactivated() throws Exception {
        service.loadConfig(new DigitalPenConfig().setSmarterStand(false));
        assertFalse(mockSmarterStandSensorListener.isRunning);
        service.loadConfig(new DigitalPenConfig().setSmarterStand(false));
        // expect a null exception if the above call re-releases
    }

    public void testAccelerometer() throws Exception {
        final double accelerometerReading = 12.345;
        mockSmarterStandSensorListener.callback.onNewPosition(accelerometerReading);
        assertEquals(accelerometerReading, getAccelerometerMessage());
    }

    private double getAccelerometerMessage() {
        assertEquals(2, lastMsgId);
        assertEquals(Double.SIZE / 8, lastMsgBuf.length);
        ByteBuffer buffer = ByteBuffer.wrap(lastMsgBuf).order(ByteOrder.LITTLE_ENDIAN);
        return buffer.getDouble();
    }

}
