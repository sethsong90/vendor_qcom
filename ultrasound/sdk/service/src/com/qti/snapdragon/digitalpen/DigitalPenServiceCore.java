/*===========================================================================
                           DigitalPenServiceCore.java

Copyright (c) 2013-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
==============================================================================*/

package com.qti.snapdragon.digitalpen;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import android.content.Context;
import android.content.Intent;
import android.hardware.SensorManager;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.os.SystemService;
import android.util.Log;

import com.qti.snapdragon.digitalpen.SmarterStandSensorListener.AccelerometerChangeCallback;
import com.qti.snapdragon.digitalpen.util.DigitalPenConfig;
import com.qti.snapdragon.digitalpen.util.DigitalPenData;
import com.qti.snapdragon.digitalpen.util.DigitalPenEvent;

public class DigitalPenServiceCore extends IDigitalPen.Stub {
    // TODO: complete refactoring between this & service, e.g. event array
    // TODO: split out threads into full public classes

    private static final int MSG_ID_LOAD_CONFIG = 1;
    private static final int MSG_ID_ACCEL_UPDATE = 2;
    final int SHORT_SLEEP_TIME = 500; // 500 msec sleep time, for pollers
    final int LONG_SLEEP_TIME = 1500; // 1500 msec sleep time

    private static final String TAG = "DigitalPenServiceCore";
    private final DigitalPenService digitalPenService;

    public DigitalPenServiceCore(DigitalPenService digitalPenService) {
        this.digitalPenService = digitalPenService;
        configManager = createConfigManager();
    }

    protected ConfigManager createConfigManager() {
        // TODO: load this default from somewhere -- last session?
        return new ConfigManager(new DigitalPenConfig());
    }

    // Main thread's handler
    private final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.arg1) {
                case DigitalPenService.DAEMON_STOPPED:
                    disable();
                    break;
            }
        }
    };
    // TODO: Get these values from config file
    private final String mControlSocketPath = "/data/usf/epos/control_socket";
    private final String mDataSocketPath = "/data/usf/epos/data_socket";
    private ControlSocketThread mControlSocketThread = null;
    private DataSocketThread mDataSocketThread = null;
    private PollingThread mPollingThread = null;
    private DigitalPenConfig sidebandConfig = new DigitalPenConfig();
    private SmarterStandSensorListener smarterStandSensorListener;
    private final ConfigManager configManager;

    /**
     * SocketThread: The socket thread class, each socket that is shared with
     * the daemon is from (or extends) this class. In the run function, the
     * thread continuously tries to connect to the socket shared with the daemon
     * (since we do not know when the daemon is up and ready, after the
     * connection is set, in case the thread needs to wait for incoming data
     * from the daemon (aka receiver), the thread invokes the "receiveWorker"
     * method.
     */
    abstract class SocketThread extends Thread {
        private static final int MAX_CONNECTION_RETRIES = 50;
        private LocalSocket mSocket;
        private final String mSocketPath;
        private boolean mConnected;
        // Size of int in bytes
        protected static final int INT_SIZE = Integer.SIZE / Byte.SIZE;

        /**
         * C'tor: Argument is the path (file system type) for the socket.
         */
        public SocketThread(String socketPath) {
            mSocketPath = socketPath;
            mConnected = false;
        }

        /** Checks if the socket is connected */
        public boolean isConnected() {
            return mConnected;
        }

        /** Returns the LocalSocket object */
        public LocalSocket getSocket() {
            return mSocket;
        }

        /**
         * Convert the bytes [index, index + 1, ..., index + INT_SIZE] to int
         */
        protected int getIntFromByteArray(byte[] data, int index) {
            int ret = 0;
            for (int j = 0; j < INT_SIZE; ++j)
                ret |= (0xff & data[index * INT_SIZE + j]) << (8 * j);
            return ret;
        }

        @Override
        public void run() {
            // The recover mechanism is for retrying to reconnect to socket
            // in-case
            // socket closes unexpectedly, this could happen because in some
            // cases
            // the daemon has to be restarted. Recover mechanism is there as
            // long
            // as connecting to the socket succeeds. If the 5 seconds retries to
            // connect to socket fails, we disable recover mechanism.
            boolean recover = true;
            try {
                while (recover) {
                    // max retry time: MAX_CONNECTION_RETRIES * SHORT_SLEEP TIME
                    int retriesLeft = MAX_CONNECTION_RETRIES;
                    while (0 < --retriesLeft) {
                        try {
                            // Connects to the socket path given
                            Log.d(TAG, "connecting to : " + mSocketPath + ", retriesLeft: "
                                    + retriesLeft);
                            mSocket = new LocalSocket();
                            mSocket.connect(new LocalSocketAddress(mSocketPath,
                                    LocalSocketAddress.Namespace.FILESYSTEM));
                            mConnected = true;
                            break;
                        } catch (IOException e) {
                            Log.d(TAG, "socket not connecting to: " + mSocketPath);
                            // Do nothing, the loop will make sure we keep
                            // trying
                        }

                        // Sleep Short time msec before trying again
                        Thread.sleep(SHORT_SLEEP_TIME);
                    }
                    // We had no luck trying to connect to socket
                    if (retriesLeft == 0) {
                        Log.e(TAG, "no more retries: " + mSocketPath);
                        // No point in recovering
                        recover = false;
                    }

                    if (!mConnected) { // Thread wasn't able to connect
                        Log.w(TAG, "Connection to " + mSocketPath + " unsuccessful");
                        return;
                    }

                    InputStream socketInput = null;
                    try {
                        // Invoke the receiverWorker which should wait for all
                        // the incoming
                        // data from the daemon
                        socketInput = mSocket.getInputStream();
                        receiveWorker(socketInput);
                    } catch (IOException e) {
                        Log.w(TAG, "IOException" + e.getMessage());
                    }
                } // while (recover)
            } catch (InterruptedException e) {
                // Do nothing, falls through to close
            }
            if (null != mSocket) {
                try {
                    mSocket.close();
                } catch (IOException e) {
                    Log.w(TAG, "IOException while closing socket " + e.getMessage());
                }
            }
            Log.d(TAG, "Thread closing: " + getClass());
        }

        /**
         * If the extending thread is a receiver (waits for incoming data), then
         * he needs to implement the receiverWorker, please note that after
         * invoking this method the thread exists.
         */
        abstract protected void receiveWorker(InputStream input) throws IOException,
                InterruptedException;
    }

    // Control socket:
    // Through the control socket we send configuration parameters update,
    // the daemon keeps listening on this socket to receive those updates.
    // Check {@link DigitalPenConfig} to know more about the format of the
    // configuration parameters updates sent.
    // It also waits for events updates from the daemon, the daemon sends all
    // the events he has using this socket. Check {@link DigitalPenEvent} to
    // know more about the format of the events data received.
    class ControlSocketThread extends SocketThread {
        ControlSocketThread() {
            super(mControlSocketPath);
        }

        /**
         * This method waits for events sent from daemon using the socket, then
         * broadcasts the received event.
         */
        @Override
        protected void receiveWorker(InputStream input) throws IOException, InterruptedException {
            if (null == input) {
                // TODO: Throw an invalid argument exception?
                return;
            }
            // Number of parameters in socket (all from type int)
            final int PACKET_PARAM_NUM = 3;
            final int RAW_DATA_PACKET_SIZE = PACKET_PARAM_NUM * INT_SIZE;
            byte[] data = new byte[RAW_DATA_PACKET_SIZE];
            // Read a packet from the data socket
            while (input.read(data, 0, RAW_DATA_PACKET_SIZE) == RAW_DATA_PACKET_SIZE) {
                // First int is the event type
                int eventType = getIntFromByteArray(data, 0);
                // The rest are the parameters
                int[] params = new int[PACKET_PARAM_NUM - 1];
                // Convert the byte array to long array
                for (int i = 0; i < PACKET_PARAM_NUM - 1; ++i)
                    params[i] = getIntFromByteArray(data, i + 1);

                // Send the event
                digitalPenService.sendEvent(eventType, params);

            }
        }
    }

    // Data socket:
    // From this data socket we receive all pen points sent by the daemon,
    // this data is proprietary and is used by services. Check {@link
    // DigitalPenData} to know more about the format of the proprietary data
    // received.
    class DataSocketThread extends SocketThread {
        DataSocketThread() {
            super(mDataSocketPath);
        }

        /**
         * This method waits for data sent from daemon using the socket, then
         * invokes all callback functions with the data received.
         */
        @Override
        protected void receiveWorker(InputStream input) throws IOException, InterruptedException {
            if (null == input) {
                // TODO: Throw an invalid argument exception?
                return;
            }
            // Number of parameters in socket (all from type int)
            // IMPORTANT: Although the daemon sends long parameters, the
            // parameters sent are 4 bytes long which is consistent
            // with JAVA's int and not JAVA's long (which is 8 bytes).
            final int PACKET_PARAM_NUM = 8;
            final int RAW_DATA_PACKET_SIZE = PACKET_PARAM_NUM * INT_SIZE;
            // Type bits constants
            final int PEN_DOWN = 0x01;
            byte[] data = new byte[RAW_DATA_PACKET_SIZE];
            // Read a packet from the data socket
            while (input.read(data, 0, RAW_DATA_PACKET_SIZE) == RAW_DATA_PACKET_SIZE) {
                int[] params = new int[PACKET_PARAM_NUM];
                // Convert the byte array to long array
                for (int i = 0; i < PACKET_PARAM_NUM; ++i)
                    params[i] = getIntFromByteArray(data, i);
                // Pen *up* or *down*
                boolean penState = ((params[7] & PEN_DOWN) != 0);

                DigitalPenData dataToSend =
                        new DigitalPenData(
                                params[0], // x
                                params[1], // y
                                params[2], // z
                                params[3], // tiltX
                                params[4], // tiltY
                                params[5], // tiltZ
                                params[6], // pressure
                                penState // penState
                        );

                // Invoke callback functions, with the data received from socket
                int i = digitalPenService.mDataCallback.beginBroadcast();
                while (i-- > 0) {
                    try {
                        digitalPenService.mDataCallback.getBroadcastItem(
                                i).onDigitalPenPropData(
                                dataToSend);
                    } catch (RemoteException e) {
                        // The RemoteCallbackList will take care of removing
                        // the dead object for us.
                    }
                }
                digitalPenService.mDataCallback.finishBroadcast();
            }
        }
    }

    class PollingThread extends Thread {
        private final Handler mServiceHandler;

        PollingThread(Handler serviceHandler) {
            mServiceHandler = serviceHandler;
        }

        @Override
        public void run() {
            try {
                Thread.sleep(LONG_SLEEP_TIME);
            } catch (InterruptedException e) {
                return;
            }
            while (true) {
                if ((SystemProperties.get("init.svc.usf_epos", "stopped")).
                        equals("stopped")) { // Daemon has stopped
                    Log.d(TAG, "Digital pen daemon has stopped");
                    // Send event reporting an internal daemon error
                    digitalPenService.sendEvent(
                            DigitalPenEvent.TYPE_INTERNAL_DAEMON_ERROR,
                            new int[] {
                                0
                            }); // No error number supported for now,
                                // sending 0
                    // Inform service about Daemon's stopping
                    Message msg = new Message();
                    msg.arg1 = DigitalPenService.DAEMON_STOPPED;
                    mServiceHandler.sendMessage(msg);
                    break;
                }
                // Sleep for a short amount before trying again
                try {
                    Thread.sleep(SHORT_SLEEP_TIME);
                } catch (InterruptedException e) {
                    return;
                }
            }
        }
    }

    /**
     * Enable the daemon and make a connection with it through the sockets
     */
    @Override
    public synchronized boolean enable() {
        // TODO: PERMISSIONS
        // mContext.enforceCallingOrSelfPermission(DIGITALPEN_ADMIN_PERM,
        // "Need DIGITALPEN_ADMIN permission");
        // TODO: consider WRITE_SECURE_SETTINGS permission instead

        if (!(SystemProperties.get("init.svc.usf_epos", "stopped")).
            equals("running")) {
            // Start the daemon
            SystemService.start(digitalPenService.mDaemonName);
            // Sleep for short amount until the daemon state property
            // is updated
            try {
                Thread.sleep(SHORT_SLEEP_TIME);
            } catch (InterruptedException e) {
                return false;
            }
        }

        if (!(SystemProperties.get("init.svc.usf_epos", "stopped")).
                equals("running")) { // Couldn't start usf_epos because of a
                                     // system
                                     // error
            // Nothing we can do to fix this, we return false in a hope that the
            // user will later try to enable it again.
            Log.e(toString(), "Couldn't start the daemon");
            return false;
        }

        DigitalPenService.mDaemonEnabled = true;

        // Create and start the socket communications threads
        if ((null == mControlSocketThread) || !mControlSocketThread.isAlive()) {
            mControlSocketThread = new ControlSocketThread();
            mControlSocketThread.start();
        }
        if ((null == mDataSocketThread) || !mDataSocketThread.isAlive()) {
            mDataSocketThread = new DataSocketThread();
            mDataSocketThread.start();
        }
        if ((null == mPollingThread) || !mPollingThread.isAlive()) {
            mPollingThread = new PollingThread(mHandler);
            mPollingThread.start();
        }

        digitalPenService.notifyPenEnabled();

        return true;
    }

    /**
     * getPid() function reads the daemon's pid file and returns its pid.
     *
     * @return int the current daemon's pid -1 in case of an error -2 in case
     *         non-integer is read from the pid file
     */
    int getPid() {
        String str = "";
        StringBuffer buf = new StringBuffer();
        int retPid;
        BufferedReader reader = null;
        try {
            // Try to read pid file located at (pidDirectory + mDaemonName +
            // pidFileExtention) path,
            // this file should include one integer, which is the daemon's pid
            FileInputStream fStream = new FileInputStream(DigitalPenService.pidDirectory +
                    digitalPenService.mDaemonName +
                    DigitalPenService.pidFileExtention);
            reader = new BufferedReader(new InputStreamReader(fStream));
            while (null != (str = reader.readLine())) {
                buf.append(str);
            }
        } catch (IOException e) {
            return -1;
        } finally {
            if (null != reader) {
                try {
                    reader.close();
                } catch (IOException e) {
                    return -1;
                }
            }
        }

        try {
            retPid = Integer.parseInt(buf.toString());
        } catch (NumberFormatException e) {
            Log.e(toString(), "Daemon pid file does not contain an integer");
            return -2;
        }
        return retPid;
    }

    /**
     * This function tries to stop the daemon as appropriate by sending it a
     * SIGTERM signal, instead of just calling stop service which in turn sends
     * a SIGKILL. Thus, in exterme cases, where an unexpected error happens,
     * this function calls stop service. <b>This function has a retry mechansim
     * with a timeout</b>
     */
    private void stopDaemon() {
        int pid = -1;
        int numTries = 10;
        while (--numTries > 0) {
            pid = getPid();
            if (-2 == pid) { // Problem getting pid
                // Stop daemon using system service stop call
                SystemService.stop(digitalPenService.mDaemonName);
            } else if (-1 != pid) { // No problems
                try {
                    // Stop daemon with SIGTERM
                    Runtime.getRuntime().exec("kill -15 " + pid);
                    // Stop smarter stand calculation when daemon is stopped.
                    if (configManager.getConfig().isSmarterStandEnabled()
                            && smarterStandSensorListener != null) {
                      smarterStandSensorListener.stop();
                      smarterStandSensorListener = null;
                    }
                    return;
                } catch (IOException e) {
                    Log.e(toString(), e.getMessage());
                }
            }
            // Error occurred, Sleep for short amount before trying again
            try {
                Thread.sleep(SHORT_SLEEP_TIME);
            } catch (InterruptedException e) {
                return;
            }
        }
    }

    /**
     * Disable the framework
     */
    @Override
    public synchronized boolean disable() {
        // TODO: PERMISSIONS
        // mContext.enforceCallingOrSelfPermission(DIGITALPEN_ADMIN_PERM,
        // "Need DIGITALPEN_ADMIN permission");
        if (!DigitalPenService.mDaemonEnabled) { // Already disabled
            return false;
        }
        // Stop the polling thread, to avoid disable() re-entrant
        if (null != mPollingThread) {
            mPollingThread.interrupt();
            try {
                // Wait for polling thread to die, it's dangeours to stop daemon
                // while
                // this thread is running.
                mPollingThread.join();
            } catch (InterruptedException e) {
            }
            mPollingThread = null;
        }
        // Make sure daemon has stopped
        while ((SystemProperties.get("init.svc.usf_epos", "stopped")).
                equals("running")) {
            stopDaemon();
            // Sleep some time to give the daemon chance to close
            try {
                Thread.sleep(LONG_SLEEP_TIME);
            } catch (InterruptedException e) {
            }
        }

        // Stop the threads
        if (null != mControlSocketThread) {
            mControlSocketThread.interrupt();
            try {
                mControlSocketThread.join();
            } catch (InterruptedException e) {
            }
            mControlSocketThread = null;
        }
        if (null != mDataSocketThread) {
            mDataSocketThread.interrupt();
            try {
                mDataSocketThread.join();
            } catch (InterruptedException e) {
            }
            mDataSocketThread = null;
        }

        DigitalPenService.mDaemonEnabled = false;

        // Send an event informing programs that the daemon has stopped
        digitalPenService.sendEvent(DigitalPenEvent.TYPE_POWER_STATE_CHANGED,
                new int[] {
                        DigitalPenEvent.POWER_STATE_OFF,
                        DigitalPenEvent.POWER_STATE_ACTIVE
                });

        // Remove notifications
        digitalPenService.notifyPenDisabled();

        return true;
    }

    /**
     * Dynamically change the configuration of the running daemon.
     */
    protected boolean changeConfig(DigitalPenConfig config)
    {
        checkSmarterStandChange(config);
        return sendControlMessage(MSG_ID_LOAD_CONFIG, config.marshalForDaemon());
    }

    private void checkSmarterStandChange(DigitalPenConfig config) {
        if (config.isSmarterStandEnabled() && smarterStandSensorListener == null) {
            smarterStandSensorListener =
                    createSmarterStandSensorListener(new AccelerometerChangeCallback() {

                        @Override
                        public void onNewPosition(double accelerometerAngleReading) {
                            processAccelerometerData(accelerometerAngleReading);
                        }
                    });
            smarterStandSensorListener.start();
        } else if (!config.isSmarterStandEnabled() && smarterStandSensorListener != null){
            smarterStandSensorListener.stop();
            smarterStandSensorListener = null;
        }
    }

    protected SmarterStandSensorListener createSmarterStandSensorListener(
            AccelerometerChangeCallback callback) {
        return new SmarterStandSensorListener(callback, (SensorManager) digitalPenService
                .getSystemService(Context.SENSOR_SERVICE));
    }

    synchronized protected boolean sendControlMessage(int msgId, byte[] msgBuf) {
        if (!isControlSocketConnected()) {
            return false;
        }

        // form message: id + payloadLen + payload
        byte[] msg = ByteBuffer.allocate(2 * (Integer.SIZE / 8) + msgBuf.length).
                order(ByteOrder.LITTLE_ENDIAN).
                putInt(msgId).
                putInt(msgBuf.length).
                put(msgBuf).
                array();

        // send to daemon
        try {
            OutputStream out = getControlStream();
            out.write(msg, 0, msg.length);
        } catch (IOException e) {
            Log.w(TAG, "IOException while trying to write to daemon" +
                    " through socket: " + e.getMessage());
            return false;
        }
        return true;
    }

    protected OutputStream getControlStream() throws IOException {
        return mControlSocketThread.getSocket().getOutputStream();
    }

    protected boolean isControlSocketConnected() {
        if (null == mControlSocketThread)
            return false;

        // Check for connection, the timeout is 2 second
        int num_tries = 4;
        while (!mControlSocketThread.isConnected()) {
            try {
                Thread.sleep(SHORT_SLEEP_TIME);
            } catch (InterruptedException e) {
            }
            if (0 == --num_tries)
                return false;
        }
        return true;
    }

    /**
     * Restores the default configuration.
     */
    private boolean restoreDefaultConfig() {
        return changeConfig(digitalPenService.mDefaultConfig);
    }

    /**
     * Changes the default configuration for the framework.
     */
    @Override
    public boolean setDefaultConfig(DigitalPenConfig config) {
        digitalPenService.mDefaultConfig = config;
        return restoreDefaultConfig();
    }

    /**
     * Returns true if the framework is enabled
     */
    @Override
    public boolean isEnabled() {
        // TODO: PERMISSIONS
        // mContext.enforceCallingOrSelfPermission(DIGITALPEN_PERM,
        // "Need DIGITALPEN permission");

        // TODO: This should be the implementation of this function
        // (getDigitalPenState() == DigitalPenAdapter.STATE_ON);

        return DigitalPenService.mDaemonEnabled;
    }

    @Override
    public boolean unloadConfig() {
        // TODO: PERMISSIONS
        // mContext.enforceCallingOrSelfPermission(DIGITALPEN_PERM,
        // "Need DIGITALPEN permission");
        return restoreDefaultConfig();
    }

    @Override
    public boolean loadConfig(DigitalPenConfig config) {
        // TODO: PERMISSIONS
        // mContext.enforceCallingOrSelfPermission(DIGITALPEN_PERM,
        // "Need DIGITALPEN permission");
        return changeConfig(config);
    }

    @Override
    public boolean registerDataCallback(IDataCallback cb) {
        if (cb != null) {
            digitalPenService.mDataCallback.register(cb);
            return true;
        }
        return false;
    }

    @Override
    public boolean registerEventCallback(IEventCallback cb) {
        if (cb != null) {
            digitalPenService.mEventCallback.register(cb);
            return true;
        }
        return false;
    }

    @Override
    public boolean unregisterDataCallback(IDataCallback cb) {
        if (cb != null) {
            digitalPenService.mDataCallback.unregister(cb);
            return true;
        }
        return false;
    }

    @Override
    public boolean unregisterEventCallback(IEventCallback cb) {
        if (cb != null) {
            digitalPenService.mEventCallback.unregister(cb);
            return true;
        }
        return false;
    }

    /**
     * Returns The DigitalPen framework state TODO: implement this function!
     * (Needs events)
     */
    @Override
    public int getDigitalPenState() {
        return 0;
    }

    public boolean onSidebandLoadConfig(Intent intent) {
        if (!isEnabled()) {
            enable();
        }
        SidebandConfigChanger changer = createConfigChanger(sidebandConfig);
        sidebandConfig = changer.processIntent(intent);
        return loadConfig(sidebandConfig);
    }

    protected SidebandConfigChanger createConfigChanger(DigitalPenConfig config) {
        return new SidebandConfigChanger(config);
    }

    protected void processAccelerometerData(double d) {
        ByteBuffer buffer = ByteBuffer.allocate(Double.SIZE / 8).
                order(ByteOrder.LITTLE_ENDIAN).
                putDouble(d);
        sendControlMessage(MSG_ID_ACCEL_UPDATE, buffer.array());
    }

    @Override
    public boolean applyConfig(Bundle config) throws RemoteException {
        configManager.setAppConfig(config);
        return changeConfig(configManager.getConfig());
}

    @Override
    public boolean releaseActivity() throws RemoteException {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public boolean setOnScreenDataCallback(IDataCallback cb) throws RemoteException {
        Log.d(TAG, "Adding on-screen callback: " + cb);
        // TODO: unify these callback paths
        digitalPenService.onScreenCallback = cb;
        registerDataCallback(cb);
        return true;
    }
}
