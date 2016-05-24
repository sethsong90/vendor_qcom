/*===========================================================================
                           DigitalPenServiceCoreLoadConfigTest.java

Copyright (c) 2013-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
==============================================================================*/

package com.qti.snapdragon.digitalpen.test;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;
import java.util.TreeSet;

import android.os.Parcel;
import android.test.AndroidTestCase;
import com.qti.snapdragon.digitalpen.DigitalPenServiceCore;
import com.qti.snapdragon.digitalpen.DigitalPenService;
import com.qti.snapdragon.digitalpen.SmarterStandSensorListener;
import com.qti.snapdragon.digitalpen.SmarterStandSensorListener.AccelerometerChangeCallback;
import com.qti.snapdragon.digitalpen.util.DigitalPenConfig;

public class DigitalPenServiceCoreLoadConfigTest extends AndroidTestCase {

    public class MockSmarterStandListener extends SmarterStandSensorListener {
        public MockSmarterStandListener() {
            super(null, null);
        }

        @Override
        public void start() {
        }

        @Override
        public void stop() {
        }
    }

    public ByteArrayOutputStream mockControlStream = new ByteArrayOutputStream();
    private DigitalPenServiceCore service;
    private DigitalPenConfig config;
    private TreeSet<Integer> unmodifiedIndices;

    // offsets into structure below
    private static final int MSG_OFFSET_OFF_SCREEN_MODE = 0;
    private static final int MSG_OFFSET_TOUCH_RANGE = MSG_OFFSET_OFF_SCREEN_MODE + 1;
    private static final int MSG_OFFSET_POWER_SAVE = MSG_OFFSET_TOUCH_RANGE + 4;
    private static final int MSG_OFFSET_ON_SCREEN_HOVER_MAX_RANGE = MSG_OFFSET_POWER_SAVE + 1;
    private static final int MSG_OFFSET_ON_SCREEN_HOVER_ENABLE = MSG_OFFSET_ON_SCREEN_HOVER_MAX_RANGE + 4;
    private static final int MSG_OFFSET_ON_SCREEN_SHOW_HOVER_ICON = MSG_OFFSET_ON_SCREEN_HOVER_ENABLE + 1;
    private static final int MSG_OFFSET_OFF_SCREEN_HOVER_MAX_RANGE = MSG_OFFSET_ON_SCREEN_SHOW_HOVER_ICON + 1;
    private static final int MSG_OFFSET_OFF_SCREEN_HOVER_ENABLE = MSG_OFFSET_OFF_SCREEN_HOVER_MAX_RANGE + 4;
    private static final int MSG_OFFSET_OFF_SCREEN_SHOW_HOVER_ICON = MSG_OFFSET_OFF_SCREEN_HOVER_ENABLE + 1;
    private static final int MSG_OFFSET_SMARTER_STAND_ENABLE = MSG_OFFSET_OFF_SCREEN_SHOW_HOVER_ICON + 1;
    private static final int MSG_OFFSET_OFFSCREEN_AREA = MSG_OFFSET_SMARTER_STAND_ENABLE + 1;
    private static final int MSG_OFFSET_ONSCREEN_COORD_DEST = MSG_OFFSET_OFFSCREEN_AREA + 36;
    private static final int MSG_OFFSET_ONSCREEN_COORD_MAPPED = MSG_OFFSET_ONSCREEN_COORD_DEST + 1;
    private static final int MSG_OFFSET_OFFSCREEN_COORD_DEST = MSG_OFFSET_ONSCREEN_COORD_MAPPED + 1;
    private static final int MSG_OFFSET_OFFSCREEN_COORD_MAPPED = MSG_OFFSET_OFFSCREEN_COORD_DEST + 1;
    private static final int MSG_OFFSET_ERASE_INDEX = MSG_OFFSET_OFFSCREEN_COORD_MAPPED + 1;
    private static final int MSG_OFFSET_ERASE_MODE = MSG_OFFSET_ERASE_INDEX + 4;

    private static final byte[] MARSHALLED_DEFAULT_CONFIG_MESSAGE;
    private static final int MSG_ID_LOAD_CONFIG = 1;
    static {
        byte[] msg = new DigitalPenConfig().marshalForDaemon();
        MARSHALLED_DEFAULT_CONFIG_MESSAGE =
                ByteBuffer.allocate(2 * (Integer.SIZE / 8) + msg.length).
                order(ByteOrder.LITTLE_ENDIAN).
                putInt(MSG_ID_LOAD_CONFIG).
                putInt(msg.length).
                put(msg).
                array();
    }
    private byte[] marshalledMessage;

    private class TestableDigitalPenServiceCore extends DigitalPenServiceCore {

        public TestableDigitalPenServiceCore() {
            super(new DigitalPenService());
        }

        @Override
        protected boolean isControlSocketConnected() {
            return true;
        }

        @Override
        protected OutputStream getControlStream() throws IOException {
            return mockControlStream;
        }

        @Override
        protected SmarterStandSensorListener createSmarterStandSensorListener(
                AccelerometerChangeCallback callback) {
            return new MockSmarterStandListener();
        }
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        service = new TestableDigitalPenServiceCore();
        config = new DigitalPenConfig();

        unmodifiedIndices = new TreeSet<Integer>();
        for (int i = 0; i < MARSHALLED_DEFAULT_CONFIG_MESSAGE.length; ++i) {
            unmodifiedIndices.add(i);
        }
    }

    private void validateRestOfMessage() {
        for (int i : unmodifiedIndices) {
            assertEquals("Default value expected in message at byte " + i,
                    MARSHALLED_DEFAULT_CONFIG_MESSAGE[i],
                    marshalledMessage[i]);
        }
    }

    private int getMessageOffset(int payloadOffset) {
        final int lengthSize = Integer.SIZE / 8;
        final int msgIdSize = Integer.SIZE / 8;
        final int offset = payloadOffset + lengthSize + msgIdSize;
        return offset;
    }

    private void validateByte(int expectedValue, int payloadOffset) {
        final int offset = getMessageOffset(payloadOffset);
        assertEquals(expectedValue, marshalledMessage[offset]);
        unmodifiedIndices.remove(offset);
    }

    private void validateInt(int expectedValue, int payloadOffset) {
        final int offset = getMessageOffset(payloadOffset);
        ByteBuffer buffer = ByteBuffer.allocate(Integer.SIZE / 8);
        buffer.order(ByteOrder.LITTLE_ENDIAN);
        buffer.putInt(expectedValue);
        byte[] expectedArray = buffer.array();
        compareFourByteArray(expectedArray, offset);
    }

    private void validateFloat(float expectedValue, int payloadOffset) {
        final int offset = getMessageOffset(payloadOffset);
        ByteBuffer buffer = ByteBuffer.allocate(Float.SIZE / 8);
        buffer.order(ByteOrder.LITTLE_ENDIAN);
        buffer.putFloat(expectedValue);
        byte[] expectedArray = buffer.array();
        compareFourByteArray(expectedArray, offset);
    }

    private void compareFourByteArray(byte[] expectedArray, final int offset) {
        final String assertionFailedMessage = "expected: " + byteArrayToString(expectedArray) +
                ", but was: "
                + byteArrayToString(Arrays.copyOfRange(marshalledMessage, offset, offset + 4));
        for (int i = 0; i < 4; ++i) {
            assertEquals(assertionFailedMessage,
                    expectedArray[i],
                    marshalledMessage[offset + i]);
            unmodifiedIndices.remove(offset + i);
        }
    }

    private String byteArrayToString(byte[] expectedArray) {
        String s = "<" + expectedArray[0] + ", " +
                expectedArray[1] + ", " +
                expectedArray[2] + ", " +
                expectedArray[3] + ">";
        return s;
    }

    private void doLoadConfig() {
        simulateInterProcessorMarshalling();
        assertTrue(service.loadConfig(config));
        marshalledMessage = mockControlStream.toByteArray();
        assertEquals("Wrong message length.", MARSHALLED_DEFAULT_CONFIG_MESSAGE.length,
                marshalledMessage.length);
    }

    private void simulateInterProcessorMarshalling() {
        Parcel p = Parcel.obtain();
        config.writeToParcel(p, 0);
        p.setDataPosition(0);
        config = DigitalPenConfig.CREATOR.createFromParcel(p);
        p.recycle();
    }

    public void testDefaultConfig() throws Exception {
        doLoadConfig();
        validateRestOfMessage();
    }

    public void testTouchRange() throws Exception {
        config.setTouchRange(10);
        doLoadConfig();
        validateInt(10, MSG_OFFSET_TOUCH_RANGE);
    }

    public void testPowerSave() throws Exception {
        config.setPowerSave((byte) 14);
        doLoadConfig();
        validateByte(14, MSG_OFFSET_POWER_SAVE);
    }

    public void testOnScreenHoverMaxRange() throws Exception {
        config.setOnScreenHoverMaxRange(50);
        doLoadConfig();
        validateInt(50, MSG_OFFSET_ON_SCREEN_HOVER_MAX_RANGE);
    }

    public void testOnScreenHoverEnable() throws Exception {
        config.setOnScreenHoverEnable(true);
        doLoadConfig();
        validateByte(1, MSG_OFFSET_ON_SCREEN_HOVER_ENABLE);
    }

    public void testShowOnScreenHoverIcon() throws Exception {
        config.setShowOnScreenHoverIcon(true);
        doLoadConfig();
        validateByte(1, MSG_OFFSET_ON_SCREEN_SHOW_HOVER_ICON);
    }

    public void testOffScreenHoverMaxRange() throws Exception {
        config.setOffScreenHoverMaxRange(60);
        doLoadConfig();
        validateInt(60, MSG_OFFSET_OFF_SCREEN_HOVER_MAX_RANGE);
    }

    public void testOffScreenHoverEnable() throws Exception {
        config.setOffScreenHoverEnable(true);
        doLoadConfig();
        validateByte(1, MSG_OFFSET_OFF_SCREEN_HOVER_ENABLE);
    }

    public void testShowOffScreenHoverIcon() throws Exception {
        config.setShowOffScreenHoverIcon(true);
        doLoadConfig();
        validateByte(1, MSG_OFFSET_OFF_SCREEN_SHOW_HOVER_ICON);
    }

    public void testSmarterStand() throws Exception {
        config.setSmarterStand(true);
        doLoadConfig();
        validateByte(1, MSG_OFFSET_SMARTER_STAND_ENABLE);
    }

    public void testOffScreenPlane() throws Exception {
        float[] origin = {
                1.1f, 2.2f, 3.3f
        };
        float[] endX = {
                83.3f, 167.7f, -294.56f
        };
        float[] endY = {
                -42.0f, -43.0f, -44.0f
        };
        config.setOffScreenPlane(origin, endX, endY);
        doLoadConfig();
        validateOffScreenPlane(origin, endX, endY);
    }

    public void testDefaultPlane() throws Exception {
        float[] origin = new float[] {
                -12.4f, 332.76f, -10.0f
        };
        float[] endX = new float[] {
                243.92f, 332.76f, -10.0f
        };
        float[] endY = new float[] {
                -12.4f, 188.58f, -10.0f
        };
        doLoadConfig();
        validateOffScreenPlane(origin, endX, endY);

    }

    private void validateOffScreenPlane(float[] origin, float[] endX, float[] endY) {
        validateFloat(origin[0], MSG_OFFSET_OFFSCREEN_AREA + 0);
        validateFloat(origin[1], MSG_OFFSET_OFFSCREEN_AREA + 4);
        validateFloat(origin[2], MSG_OFFSET_OFFSCREEN_AREA + 8);

        validateFloat(endX[0], MSG_OFFSET_OFFSCREEN_AREA + 12);
        validateFloat(endX[1], MSG_OFFSET_OFFSCREEN_AREA + 16);
        validateFloat(endX[2], MSG_OFFSET_OFFSCREEN_AREA + 20);

        validateFloat(endY[0], MSG_OFFSET_OFFSCREEN_AREA + 24);
        validateFloat(endY[1], MSG_OFFSET_OFFSCREEN_AREA + 28);
        validateFloat(endY[2], MSG_OFFSET_OFFSCREEN_AREA + 32);
    }

    public void testCoordinateReporting() throws Exception {
        config.setOnScreenCoordReporting(DigitalPenConfig.DP_COORD_DESTINATION_SOCKET, true);
        config.setOffScreenCoordReporting(DigitalPenConfig.DP_COORD_DESTINATION_BOTH, false);
        doLoadConfig();
        validateByte(DigitalPenConfig.DP_COORD_DESTINATION_SOCKET, MSG_OFFSET_ONSCREEN_COORD_DEST);
        validateByte(1, MSG_OFFSET_ONSCREEN_COORD_MAPPED);
        validateByte(DigitalPenConfig.DP_COORD_DESTINATION_BOTH, MSG_OFFSET_OFFSCREEN_COORD_DEST);
        validateByte(0, MSG_OFFSET_OFFSCREEN_COORD_MAPPED);
    }

    public void testEraseButtonIndex() throws Exception {
        config.setEraseButtonIndex(1);
        doLoadConfig();
        validateInt(1, MSG_OFFSET_ERASE_INDEX);
    }

    public void testEraseButtonBehavior() throws Exception {
        config.setEraseButtonBehavior(DigitalPenConfig.DP_ERASE_MODE_TOGGLE);
        doLoadConfig();
        validateByte(DigitalPenConfig.DP_ERASE_MODE_TOGGLE, MSG_OFFSET_ERASE_MODE);
    }

}
