/******************************************************************************
 * ---------------------------------------------------------------------------
 *  Copyright (C) 2013-2014 Qualcomm Technologies, Inc.
 *  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
 *  ---------------------------------------------------------------------------
 *******************************************************************************/
package com.qti.ultrasound.penpairing;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.graphics.Color;
import android.graphics.drawable.GradientDrawable;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.widget.TextView;
import android.widget.ProgressBar;

public class SemiAutomaticActivity extends Activity {

    private static final String EXE_PATH = "/system/bin/usf_pairing";
    private static final String SOCKET_PATH = "/data/usf/pairing/pairing_com_soc";

    // These MUST be the same constants as in the pen pairing
    // daemon, and with the same values.
    private static final int STATUS_IN_PROGRESS = 1;
    private static final int STATUS_REPOSITION = 2;
    private static final int STATUS_BLOCKED = 3;
    private static final int STATUS_SUCCESS = 4;
    private static final int STATUS_FAIL = 5;
    private static final int STATUS_TIMEOUT = 6;

    private static final int MSG_IND_STATUS = 0;
    private static final int MSG_IND_PEN_ID = 1;

    private static final String TIMEOUT_MESSAGE = "Pen Pairing daemon failed to find a pen. Please try again";

    private static boolean pairingDaemonRunning;
    private Process pairingDaemonProcess = null;

    // Background threads use this Handler to post messages to
    // the main application thread
    private final Handler handler = new Handler();

    private SocketListener socketListener;
    private ControlThread controlThread = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_semiautomatic);
    }

    @Override
    protected void onStart() {
        super.onStart();

        startPairingDaemon();

        socketListener = new SocketListener();

        Log.d(this.toString(),
              "Starting socket thread...");

        socketListener.start();

        controlThread = new ControlThread(socketListener);
        controlThread.start();
    }

    @Override
    protected void onStop() {
        stopPairingDaemon();
        super.onStop();
    }

    /**
     * This class defines the thread which waits for the socket
     * thread to finish in no more than a timeout. If timeout has
     * passed and the socket thread still runs - it is interrupted.
    */
    public class ControlThread extends Thread {
        private static final int SLEEP_MSEC = 500;
        private Thread mControlledThread;
        private Handler mHandler;

        ControlThread(Thread thread) {
            mControlledThread = thread;
            this.mHandler = handler;
        }

        @Override
        public void run() {
            try {
                mControlledThread.join();
            } catch (InterruptedException e) {
                Log.e(this.toString(),
                      "Control thread was interrupted while waiting for socket thread");
            }

            pairingDaemonRunning = false;
            try {
                // Wait a little for the SocketThread to realize the pairing
                // daemon isn't running anymore
                Thread.sleep(SLEEP_MSEC);
            } catch (InterruptedException e) {
            }

            if (mControlledThread.isAlive()) {
                mControlledThread.interrupt();
                stopPairingDaemon();
                handler.post(new Runnable() { // Run from main thread
                        @Override
                        public void run() {
                            alertFail(TIMEOUT_MESSAGE);
                        }
                    });
            }
        }
    }

    public class SocketListener extends Thread {
        private static final int NUM_TRIES_SOCKET = 10;
        private static final int DELAY_SOCKET_RECONNECT_MSEC = 500;
        private static final int MESSAGE_LEN_BYTES = 8; // Two ints

        private Handler mHandler;
        private LocalSocket receiver;

        SocketListener() {
            this.mHandler = handler;
            receiver = new LocalSocket();
        }

        @Override
        public void run() {
            Log.d(this.toString(),
                  "Socket thread running");
            int numTries = 0;
            boolean isConnected = false;
            InputStream input = null;
            while (NUM_TRIES_SOCKET >= ++numTries &&
                   !isConnected &&
                   pairingDaemonRunning) {
                try {
                    Log.d(this.toString(),
                          "Attempt " + numTries + " to connect to socket...");
                    receiver.connect(new LocalSocketAddress(SOCKET_PATH,
                                                            LocalSocketAddress.Namespace.FILESYSTEM));
                    Log.d(this.toString(),
                          "Connected to socket!");
                    input = receiver.getInputStream();
                    isConnected = true;
                } catch (IOException e) {
                    Log.e(this.toString(),
                          e.getMessage());
                }
                try {
                    Thread.sleep(DELAY_SOCKET_RECONNECT_MSEC);
                } catch (InterruptedException e) {
                    Log.e(this.toString(),
                          "App socket Thread Interruped while sleeping");
                }
            }

            if (!isConnected) {
                // Show error msg only in case of connection failure
                if (pairingDaemonRunning) {
                    mHandler.post(new Runnable() { // Run from main thread
                        @Override
                        public void run() {
                            alertFail("Connection to daemon socket failed. Please try again");
                        }
                    });
                }
                return;
            }

            while (pairingDaemonRunning) {
                try {
                    byte[] buffer = new byte[MESSAGE_LEN_BYTES];
                    int bytesRead = input.read(buffer);
                    Log.d(this.toString(),
                          "Bytes read: " + bytesRead);
                    if (-1 == bytesRead) {
                        break;
                    }
                    IntBuffer intBuf = ByteBuffer.wrap(buffer)
                                                 .order(ByteOrder.LITTLE_ENDIAN)
                                                 .asIntBuffer();
                    final int[] res = new int[intBuf.remaining()];
                    intBuf.get(res);
                    mHandler.post(new Runnable() { // Run from main thread
                        @Override
                        public void run() {
                            onSocketEvent(res);
                        }
                    });
                } catch (IOException e) {
                    Log.e(this.toString(),
                          e.getMessage());
                    pairingDaemonRunning = false;
                }
            }
        }
    }

    private void onSocketEvent(int[] res) {
        int status = res[MSG_IND_STATUS];

        if (status != STATUS_IN_PROGRESS) {
            pairingDaemonRunning = false;
        }

        switch (status) {
        case STATUS_IN_PROGRESS:
            setUI("Series aquisition in progress",
                  Color.YELLOW);
            break;
        case STATUS_REPOSITION:
            alertFail("Pen is not positioned correctly. Please try again");
            break;
        case STATUS_BLOCKED:
            alertFail("One of the microphones is blocked. Please try again");
            break;
        case STATUS_FAIL:
            alertFail("Pen Pairing has failed.");
            break;
        case STATUS_TIMEOUT:
            alertFail(TIMEOUT_MESSAGE);
            break;
        case STATUS_SUCCESS:
            int penId = res[MSG_IND_PEN_ID];
            setUI("Pen detected (series ID " + String.valueOf(penId) + ")",
                  Color.GREEN);
            PairingDbHelper pairingDbHelper = new PairingDbHelper(this);
            pairingDbHelper.addPen(penId);
            Log.d(this.toString(),
                  "Added new pen with pen id " + penId);
            break;
        }
    }

    private void startPairingDaemon() {
        try {
            pairingDaemonProcess = Runtime.getRuntime()
                   .exec(EXE_PATH);
        } catch (IOException e) {
            Log.e(toString(),
                  "Error trying to execute " + EXE_PATH,
                  e);
            alertFail("An error has occurred while starting the pairing daemon. click 'OK' to exit");
            return;
        }
        pairingDaemonRunning = true;
    }

    private void stopPairingDaemon() {
        if (null != pairingDaemonProcess) {
            pairingDaemonProcess.destroy();
        }
        pairingDaemonProcess = null;
        pairingDaemonRunning = false;
    }

    private void setUI(final String message, final int circleColor) {
        View cv = findViewById(R.id.semiautomatic_circle_view);
        GradientDrawable gd = (GradientDrawable) cv.getBackground()
                                                   .mutate();
        gd.setColor(circleColor);
        gd.invalidateSelf();

        TextView tv = (TextView) findViewById(R.id.semiautomatic_text_view);
        tv.setText(message);

        ProgressBar progress = (ProgressBar) findViewById(R.id.semiautomatic_progress_bar);
        if (progress.getVisibility() == View.VISIBLE) {
            progress.setVisibility(View.INVISIBLE);
        } else {
            progress.setVisibility(View.VISIBLE);
        }
    }

    private void alertFail(String message) {
        AlertDialog.Builder alert = new AlertDialog.Builder(this);
        alert.setTitle("Pen Pairing Failed")
             .setMessage(message)
             .setPositiveButton("Ok",
                                new OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog,
                                                        int which) {
                                        finish();
                                    }
                                });
        alert.setCancelable(false);
        alert.show();
    }
}

