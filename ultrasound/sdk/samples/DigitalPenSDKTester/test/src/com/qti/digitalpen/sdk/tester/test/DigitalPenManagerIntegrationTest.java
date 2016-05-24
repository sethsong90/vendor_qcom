/*===========================================================================
                           DigitalPenManagerIntegrationTest.java

Copyright (c) 2013-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
==============================================================================*/

package com.qti.digitalpen.sdk.tester.test;

import java.util.concurrent.SynchronousQueue;
import java.util.concurrent.TimeUnit;

import com.qti.snapdragon.digitalpen.IDigitalPen;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager.OnSideChannelDataListener;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager.SideChannelData;

import android.content.Intent;
import android.content.ServiceConnection;
import android.test.AndroidTestCase;
import android.test.mock.MockApplication;
import android.util.Log;

public class DigitalPenManagerIntegrationTest extends
        AndroidTestCase {

    @Override
    public void testAndroidTestCaseSetupProperly() {
        Log.d(TAG, "test case set up properly!");
        super.testAndroidTestCaseSetupProperly();
    }

    // for when test is expected to not timeout
    private static final int TIMEOUT_MSECS = 2000;

    // for when test is expected to timeout
    private static final long SHORT_TIMEOUT_MSECS = 50;
    protected static final String TAG = "DigitalPenManagerIntegrationTest";
    protected SideChannelData lastData;
    private final SynchronousQueue<SideChannelData> receivedData = new SynchronousQueue<SideChannelData>();
    private DigitalPenManager mgr;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mgr = new DigitalPenManager(new MockApplication() {
            @Override
            public void registerActivityLifecycleCallbacks(ActivityLifecycleCallbacks callback) {
            }

            @Override
            public boolean bindService(Intent service, ServiceConnection conn, int flags) {
                return getContext().bindService(service, conn, flags);
            }
        });
        mgr.registerOnScreenCallback(new OnSideChannelDataListener() {

            @Override
            public void onDigitalPenData(SideChannelData data) {
                Log.d(TAG, "Got callback! Data: " + data.xPos);
                try {
                    assertTrue(receivedData.offer(data, TIMEOUT_MSECS, TimeUnit.MILLISECONDS));
                } catch (InterruptedException e) {
                    throw new RuntimeException(e);
                }
            }
        });
    }

    @Override
    protected void tearDown() throws Exception {
        mgr.releaseApplication();
    }

    public void testOnScreenDataCallback() throws Exception {
        // send intent to service that will cause it to return a data point
        Intent serviceIntent = new Intent(IDigitalPen.class.getName()).putExtra(
                "GenerateOnScreenData", "x=41");
        getContext().startService(serviceIntent);

        // wait for the data point
        SideChannelData data = receivedData.poll(TIMEOUT_MSECS, TimeUnit.MILLISECONDS);
        assertNotNull(data);
        assertEquals(41, data.xPos);
    }

    public void testSecondCall() throws Exception {
        testOnScreenDataCallback();
    }

    public void testUnregisterCallback() throws Exception {
        // disable on-screen callback
        mgr.registerOnScreenCallback(null);

        // send intent to service that will cause it to return a data point
        Intent serviceIntent = new Intent(IDigitalPen.class.getName()).putExtra(
                "GenerateOnScreenData", "x=45");
        getContext().startService(serviceIntent);

        // wait for the data point, should time-out or fail...
        SideChannelData data = receivedData.poll(SHORT_TIMEOUT_MSECS, TimeUnit.MILLISECONDS);
        assertEquals(null, data);
    }

    // TODO: service disconnection handled gracefully
}
