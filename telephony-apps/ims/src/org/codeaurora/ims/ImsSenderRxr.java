/*
 * Copyright (c) 2012-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 *
 * Not a Contribution, Apache license notifications and license are retained
 * for attribution purposes only.
 *
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

package org.codeaurora.ims;

import java.io.FileDescriptor;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;
import java.net.Socket;

import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.AsyncResult;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.os.Registrant;
import android.os.RegistrantList;
import android.os.PowerManager.WakeLock;
import android.os.Registrant;
import android.os.RegistrantList;
import android.os.SystemProperties;
import android.telephony.PhoneNumberUtils;
import android.util.Log;

import com.android.internal.telephony.BaseCommands;
import com.android.internal.telephony.CallForwardInfo;
import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.DriverCall;
import com.android.internal.telephony.gsm.SuppServiceNotification;
import com.android.internal.telephony.TelephonyProperties;
import com.android.internal.telephony.UUSInfo;
import com.android.internal.telephony.cdma.CdmaSmsBroadcastConfigInfo;
import com.android.internal.telephony.gsm.SmsBroadcastConfigInfo;
import com.android.internal.telephony.CallDetails;
import com.android.internal.telephony.CallModify;
import com.google.protobuf.micro.ByteStringMicro;

/**
 * {@hide}
 */
class IFRequest {
    static final String LOG_TAG = "ImsSenderRxr";

    // ***** Class Variables
    static int sNextSerial = 0;
    static Object sSerialMonitor = new Object();
    private static Object sPoolSync = new Object();
    private static IFRequest sPool = null;
    private static int sPoolSize = 0;
    private static final int MAX_POOL_SIZE = 4;

    // ***** Instance Variables
    int mSerial;
    int mRequest;
    Message mResult;
    // FIXME delete parcel
    // Parcel mp;
    IFRequest mNext;
    byte[] mData;

    /**
     * Retrieves a new IFRequest instance from the pool.
     *
     * @param request ImsQmiIF.MsgId.REQUEST_*
     * @param result sent when operation completes
     * @return a IFRequest instance from the pool.
     */
    static IFRequest obtain(int request, Message result) {
        IFRequest rr = null;

        synchronized (sPoolSync) {
            if (sPool != null) {
                rr = sPool;
                sPool = rr.mNext;
                rr.mNext = null;
                sPoolSize--;
            }
        }

        if (rr == null) {
            rr = new IFRequest();
        }

        synchronized (sSerialMonitor) {
            rr.mSerial = sNextSerial++;
        }
        rr.mRequest = request;
        rr.mResult = result;

        if (result != null && result.getTarget() == null) {
            throw new NullPointerException("Message target must not be null");
        }

        return rr;
    }

    /**
     * Returns a IFRequest instance to the pool. Note: This should only be
     * called once per use.
     */
    void release() {
        synchronized (sPoolSync) {
            if (sPoolSize < MAX_POOL_SIZE) {
                this.mNext = sPool;
                sPool = this;
                sPoolSize++;
                mResult = null;
            }
        }
    }

    private IFRequest() {
    }

    static void resetSerial() {
        synchronized (sSerialMonitor) {
            sNextSerial = 0;
        }
    }

    String serialString() {
        // Cheesy way to do %04d
        StringBuilder sb = new StringBuilder(8);
        String sn;

        sn = Integer.toString(mSerial);

        // sb.append("J[");
        sb.append('[');
        for (int i = 0, s = sn.length(); i < 4 - s; i++) {
            sb.append('0');
        }

        sb.append(sn);
        sb.append(']');
        return sb.toString();
    }

    void onError(int error, Object ret) {
        RuntimeException ex;
        String errorMsg;

        if (error == ImsQmiIF.E_SUCCESS) {
            ex = null;
        } else {
            errorMsg = ImsSenderRxr.errorIdToString(error);
            ex = new RuntimeException(errorMsg);
        }

        if (ImsSenderRxr.IF_LOGD)
            Log.d(LOG_TAG, serialString() + "< "
                    + ImsSenderRxr.msgIdToString(mRequest)
                    + " error: " + error);

        if (mResult != null) {
            AsyncResult.forMessage(mResult, ret, ex);
            mResult.sendToTarget();
        }
    }
}

/**
 * IMS implementation of the CommandsInterface. {@hide}
 */
public final class ImsSenderRxr extends BaseCommands implements CommandsInterface {
    static final String LOG_TAG = "ImsSenderRxr";
    static final boolean IF_LOGD = true;
    static final boolean IF_LOGV = true; // STOP SHIP if true

    /**
     * Wake lock timeout should be longer than the longest timeout in the vendor
     */
    private static final int DEFAULT_WAKE_LOCK_TIMEOUT = 60000;
    private static final int PDU_LENGTH_OFFSET = 4;
    private static final int MSG_TAG_LENGTH = 1;
    // ***** Instance Variables

    LocalSocket mSocket;
    HandlerThread mSenderThread;
    IFMsg_Sender mSender;
    Thread mReceiverThread;
    IFMsg_Rxr mReceiver;
    WakeLock mWakeLock;
    int mWakeLockTimeout;
    // The number of requests pending to be sent out, it increases before
    // calling
    // EVENT_SEND and decreases while handling EVENT_SEND. It gets cleared while
    // WAKE_LOCK_TIMEOUT occurs.
    int mRequestMessagesPending;
    // The number of requests sent out but waiting for response. It increases
    // while
    // sending request and decreases while handling response. It should match
    // mRequestList.size() unless there are requests no replied while
    // WAKE_LOCK_TIMEOUT occurs.
    int mRequestMessagesWaiting;

    // I'd rather this be LinkedList or something
    ArrayList<IFRequest> mRequestsList = new ArrayList<IFRequest>();

    Object mLastNITZTimeInfo;

    protected RegistrantList mModifyCallRegistrants = new RegistrantList();
    protected Registrant mSsnRegistrant;

    // When we are testing emergency calls
    AtomicBoolean mTestingEmergencyCall = new AtomicBoolean(false);

    // ***** Events

    static final int EVENT_SEND = 1;
    static final int EVENT_WAKE_LOCK_TIMEOUT = 2;

    // ***** Constants

    static final int MAX_COMMAND_BYTES = (8 * 1024);
    static final int RESPONSE_SOLICITED = 0;
    static final int RESPONSE_UNSOLICITED = 1;

    static final String SOCKET_NAME_IF = "qmux_radio/rild_ims";
    static final int SOCKET_OPEN_RETRY_MILLIS = 4 * 1000;

    private RegistrantList mHandoverStatusRegistrants = new RegistrantList();
    private RegistrantList mRefreshConfInfoRegistrations = new RegistrantList();
    private RegistrantList mSrvStatusRegistrations = new RegistrantList();
    private RegistrantList mTtyStatusRegistrants = new RegistrantList();
    private RegistrantList mRadioStateRegistrations = new RegistrantList();

    public void registerForHandoverStatusChanged(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mHandoverStatusRegistrants.add(r);
    }

    public void unregisterForHandoverStatusChanged(Handler h) {
        mHandoverStatusRegistrants.remove(h);
    }

    public void registerForRefreshConfInfo(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mRefreshConfInfoRegistrations.add(r);
    }

    public void unregisterForRefreshConfInfo(Handler h) {
        mRefreshConfInfoRegistrations.remove(h);
    }

    public void registerForSrvStatusUpdate(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mSrvStatusRegistrations.add(r);
    }

    public void unregisterForSrvStatusUpdate(Handler h) {
        mSrvStatusRegistrations.remove(h);
    }

    public void registerForTtyStatusChanged(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mTtyStatusRegistrants.add(r);
    }

    public void unregisterForTtyStatusChanged(Handler h) {
        mTtyStatusRegistrants.remove(h);
    }

    public void setOnSuppServiceNotification(Handler h, int what, Object obj) {
        mSsnRegistrant = new Registrant (h, what, obj);
    }

    public void unSetOnSuppServiceNotification(Handler h) {
        mSsnRegistrant.clear();
    }

    class IFMsg_Sender extends Handler implements Runnable {
        public IFMsg_Sender(Looper looper) {
            super(looper);
        }

        // ***** Runnable implementation
        public void
                run() {
            // setup if needed
        }

        // ***** Handler implementation
        @Override
        public void handleMessage(Message msg) {
            IFRequest rr = (IFRequest) (msg.obj);
            IFRequest req = null;

            switch (msg.what) {
                case EVENT_SEND:
                    /**
                     * mRequestMessagePending++ already happened for every
                     * EVENT_SEND, thus we must make sure
                     * mRequestMessagePending-- happens once and only once
                     */
                    boolean alreadySubtracted = false;
                    try {
                        LocalSocket s;
                        s = mSocket;

                        if (s == null) {
                            rr.onError(ImsQmiIF.E_RADIO_NOT_AVAILABLE, null);
                            rr.release();
                            if (mRequestMessagesPending > 0)
                                mRequestMessagesPending--;
                            alreadySubtracted = true;
                            return;
                        }

                        synchronized (mRequestsList) {
                            mRequestsList.add(rr);
                            mRequestMessagesWaiting++;
                        }

                        if (mRequestMessagesPending > 0)
                            mRequestMessagesPending--;
                        alreadySubtracted = true;

                        if (rr.mData.length > MAX_COMMAND_BYTES) {
                            throw new RuntimeException(
                                    "Message is larger than max bytes allowed! "
                                            + rr.mData.length);
                        }

                        s.getOutputStream().write(rr.mData);

                    } catch (IOException ex) {
                        Log.e(LOG_TAG, "IOException", ex);
                        req = findAndRemoveRequestFromList(rr.mSerial);
                        // make sure this request has not already been handled,
                        // eg, if IFMsg_Rxr cleared the list.
                        if (req != null || !alreadySubtracted) {
                            rr.onError(ImsQmiIF.E_RADIO_NOT_AVAILABLE, null);
                            rr.release();
                        }
                    } catch (RuntimeException exc) {
                        Log.e(LOG_TAG, "Uncaught exception ", exc);
                        req = findAndRemoveRequestFromList(rr.mSerial);
                        // make sure this request has not already been handled,
                        // eg, if IFMsg_Rxr cleared the list.
                        if (req != null || !alreadySubtracted) {
                            rr.onError(ImsQmiIF.E_GENERIC_FAILURE, null);
                            rr.release();
                        }
                    } finally {
                        // Note: We are "Done" only if there are no outstanding
                        // requests or replies. Thus this code path will only
                        // release
                        // the wake lock on errors.
                        releaseWakeLockIfDone();
                    }

                    if (!alreadySubtracted && mRequestMessagesPending > 0) {
                        mRequestMessagesPending--;
                    }

                    break;

                case EVENT_WAKE_LOCK_TIMEOUT:
                    // Haven't heard back from the last request. Assume we're
                    // not getting a response and release the wake lock.
                    synchronized (mWakeLock) {
                        if (mWakeLock.isHeld()) {
                            // The timer of WAKE_LOCK_TIMEOUT is reset with each
                            // new send request. So when WAKE_LOCK_TIMEOUT
                            // occurs
                            // all requests in mRequestList already waited at
                            // least DEFAULT_WAKE_LOCK_TIMEOUT but no response.
                            // Reset mRequestMessagesWaiting to enable
                            // releaseWakeLockIfDone().
                            //
                            // Note: Keep mRequestList so that delayed response
                            // can still be handled when response finally comes.
                            if (mRequestMessagesWaiting != 0) {
                                Log.d(LOG_TAG, "NOTE: mReqWaiting is NOT 0 but"
                                        + mRequestMessagesWaiting + " at TIMEOUT, reset!"
                                        + " There still msg waitng for response");

                                mRequestMessagesWaiting = 0;

                                if (IF_LOGD) {
                                    synchronized (mRequestsList) {
                                        int count = mRequestsList.size();
                                        Log.d(LOG_TAG, "WAKE_LOCK_TIMEOUT " +
                                                " mRequestList=" + count);

                                        for (int i = 0; i < count; i++) {
                                            rr = mRequestsList.get(i);
                                            Log.d(LOG_TAG, i + ": [" + rr.mSerial + "] "
                                                    + msgIdToString(rr.mRequest));
                                        }
                                    }
                                }
                            }
                            // mRequestMessagesPending shows how many
                            // requests are waiting to be sent (and before
                            // to be added in request list) since star the
                            // WAKE_LOCK_TIMEOUT timer. Since WAKE_LOCK_TIMEOUT
                            // is the expected time to get response, all
                            // requests
                            // should already sent out (i.e.
                            // mRequestMessagesPending is 0 )while TIMEOUT
                            // occurs.
                            if (mRequestMessagesPending != 0) {
                                Log.e(LOG_TAG, "ERROR: mReqPending is NOT 0 but"
                                        + mRequestMessagesPending + " at TIMEOUT, reset!");
                                mRequestMessagesPending = 0;

                            }
                            mWakeLock.release();
                        }
                    }
                    break;
            }
        }
    }

    /**
     * Reads in a single message off the wire. A message consists of a 4-byte
     * little-endian length and a subsequent series of bytes. The final message
     * (length header omitted) is read into <code>buffer</code> and the length
     * of the final message (less header) is returned. A return value of -1
     * indicates end-of-stream.
     *
     * @param is non-null; Stream to read from
     * @param buffer Buffer to fill in. Must be as large as maximum message
     *            size, or an ArrayOutOfBounds exception will be thrown.
     * @return Length of message less header, or -1 on end of stream.
     * @throws IOException
     */
    private static int readMessage(InputStream is, byte[] buffer)
            throws IOException {
        int countRead;
        int offset;
        int remaining;
        int messageLength;

        // First, read in the length of the message
        offset = 0;
        remaining = 4;
        do {
            countRead = is.read(buffer, offset, remaining);

            if (countRead < 0) {
                Log.e(LOG_TAG, "Hit EOS reading message length");
                return -1;
            }

            offset += countRead;
            remaining -= countRead;
        } while (remaining > 0);

        messageLength = ((buffer[0] & 0xff) << 24)
                | ((buffer[1] & 0xff) << 16)
                | ((buffer[2] & 0xff) << 8)
                | (buffer[3] & 0xff);

        // Then, re-use the buffer and read in the message itself
        offset = 0;
        remaining = messageLength;
        do {
            countRead = is.read(buffer, offset, remaining);

            if (countRead < 0) {
                Log.e(LOG_TAG, "Hit EOS reading message.  messageLength=" + messageLength
                        + " remaining=" + remaining);
                return -1;
            }

            offset += countRead;
            remaining -= countRead;
        } while (remaining > 0);

        return messageLength;
    }

    class IFMsg_Rxr implements Runnable {

        byte[] buffer;
        IFMsg_Rxr() {
            buffer = new byte[MAX_COMMAND_BYTES];
        }

        public void run() {
            int retryCount = 0;
            boolean killMe = false;

            String ifSocket = SOCKET_NAME_IF;

            try {
                if (!killMe) {
                    for (;;) {
                        LocalSocket s = null;
                        LocalSocketAddress l;

                        try {
                             s = new LocalSocket();
                             l = new LocalSocketAddress(ifSocket,
                             LocalSocketAddress.Namespace.RESERVED);
                             s.connect(l);
                             Log.d(LOG_TAG, "Connecting to socket " + s);

                        } catch (IOException ex) {
                            Log.e(LOG_TAG,
                                    "Exception in socket create'");
                            // don't print an error message after the the first time
                            // or after the 8th time

                            if (retryCount == 8) {
                                Log.e(LOG_TAG,
                                        "Couldn't find socket after " + retryCount
                                                + " times, continuing to retry silently");
                                disableSrvStatus(); // Disable all Ims Services
                                try {
                                    s.close();
                                    killMe = true;
                                    return;
                                } catch (IOException e) {
                                    Log.e(LOG_TAG, "IOException", ex);
                                    return;
                                }
                            } else if (retryCount > 0 && retryCount < 8) {
                                Log.i(LOG_TAG,
                                        "Couldn't find socket; retrying after timeout");
                            }

                            try {
                                Thread.sleep(SOCKET_OPEN_RETRY_MILLIS);
                            } catch (InterruptedException er) {
                            }

                            retryCount++;
                            continue;
                        }

                        retryCount = 0;

                        mSocket = s;

                        Log.i(LOG_TAG, "Connected to '" + mSocket + "' socket");

                        int length = 0;
                        try {
                            InputStream is = mSocket.getInputStream();

                            for (;;) {

                                length = readMessage(is, buffer);

                                if (length < 0) {
                                    // End-of-stream reached
                                    break;
                                }

                                Log.v(LOG_TAG, "Read packet: " + length + " bytes");

                                if (length > 0) processResponse(buffer, length);

                            }
                        } catch (java.io.IOException ex) {
                            Log.i(LOG_TAG, "socket closed",
                                    ex);
                        } catch (Throwable tr) {
                            Log.e(LOG_TAG, "Uncaught exception read length=" + length +
                                    "Exception:" + tr.toString());
                        }

                        Log.i(LOG_TAG, "Disconnected from socket");

                        try {
                            mSocket.close();
                        } catch (IOException ex) {
                        }

                        mSocket = null;
                        IFRequest.resetSerial();

                        // Clear request list on close

                        clearRequestsList(ImsQmiIF.E_RADIO_NOT_AVAILABLE, false);
                    }
                }
            } catch (Throwable tr) {
                Log.e(LOG_TAG, "Uncaught exception", tr);
            }

            /* We're disconnected so we don't know the version */
            // notifyRegistrantsIFConnectionChanged(-1);
        }
    }

    // ***** Constructors

    public ImsSenderRxr(Context context)
    {
        super(context);

        mPhoneType = 0; // NO_PHONE;

        PowerManager pm = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, LOG_TAG);
        mWakeLock.setReferenceCounted(false);
        mWakeLockTimeout = SystemProperties.getInt(TelephonyProperties.PROPERTY_WAKE_LOCK_TIMEOUT,
                DEFAULT_WAKE_LOCK_TIMEOUT);
        mRequestMessagesPending = 0;
        mRequestMessagesWaiting = 0;

        mSenderThread = new HandlerThread("IFMsg_Sender");
        mSenderThread.start();

        Looper looper = mSenderThread.getLooper();
        mSender = new IFMsg_Sender(looper);

        log("Starting IFMsg_Rxr");
        mReceiver = new IFMsg_Rxr();
        mReceiverThread = new Thread(mReceiver, "IFMsg_Rxr");
        mReceiverThread.start();

        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_SCREEN_ON);
        filter.addAction(Intent.ACTION_SCREEN_OFF);
        // context.registerReceiver(mIntentReceiver, filter);
    }

    /**
     * Holds a PARTIAL_WAKE_LOCK whenever a) There is outstanding request sent
     * to the interface and no replied b) There is a request pending to be sent
     * out. There is a WAKE_LOCK_TIMEOUT to release the lock, though it
     * shouldn't happen often.
     */

    private void acquireWakeLock() {
        synchronized (mWakeLock) {
            mWakeLock.acquire();
            mRequestMessagesPending++;

            mSender.removeMessages(EVENT_WAKE_LOCK_TIMEOUT);
            Message msg = mSender.obtainMessage(EVENT_WAKE_LOCK_TIMEOUT);
            mSender.sendMessageDelayed(msg, mWakeLockTimeout);
        }
    }

    private void releaseWakeLockIfDone() {
        synchronized (mWakeLock) {
            if (mWakeLock.isHeld() &&
                    (mRequestMessagesPending == 0) &&
                    (mRequestMessagesWaiting == 0)) {
                mSender.removeMessages(EVENT_WAKE_LOCK_TIMEOUT);
                mWakeLock.release();
            }
        }
    }

    public void send(IFRequest rr) {
        Message msg;

        if (mSocket == null) {
            rr.onError(ImsQmiIF.E_RADIO_NOT_AVAILABLE, null);
            rr.release();
            return;
        }

        msg = mSender.obtainMessage(EVENT_SEND, rr);

        acquireWakeLock();

        msg.sendToTarget();
    }

    public void processResponse(byte[] data, int length) {

        // Parse tag length from first one byte
        int msglen = 1;
        int startIndex = 0;
        int endIndex = startIndex + msglen;
        byte[] msg = null;

        logv("processResponse");
        if (ImsSenderRxr.IF_LOGV) {
            for (int i = 0; i < length; i++) {
                log(" byte " + i + ":" + data[i]);
            }
        }

        if (endIndex <= length) {
            msglen = data[startIndex];

            // Parse tag
            ImsQmiIF.MsgTag tag = null;
            startIndex = endIndex;
            endIndex = startIndex + msglen;

            if ((endIndex <= length) && (msglen > 0)) {

                msg = new byte[msglen];

                try {
                    System.arraycopy(data, startIndex, msg, 0, msglen);
                    // Convert tag in bytes to local data structure
                    tag = ImsQmiIF.MsgTag.parseFrom(msg);
                } catch (IndexOutOfBoundsException ex) {
                    log(" IndexOutOfBoundsException while parsing msg tag");
                    releaseWakeLockIfDone();
                    return;
                } catch (ArrayStoreException ex) {
                    log(" ArrayStoreException while parsing msg tag");
                    releaseWakeLockIfDone();
                    return;
                } catch (NullPointerException ex) {
                    log(" NullPointerException while parsing msg tag");
                    releaseWakeLockIfDone();
                    return;
                } catch (com.google.protobuf.micro.InvalidProtocolBufferMicroException ex) {
                    log("InvalidProtocolBufferException while parsing msg tag");
                    releaseWakeLockIfDone();
                    return;
                }
                log(" Tag " + tag.getToken() + " " + tag.getType() + " "
                        + tag.getId() + " " + tag.getError());

                // Parse message content if present
                startIndex = endIndex;
                msglen = length - startIndex;
                endIndex = startIndex + msglen;

                msg = null;
                if ((endIndex <= length) && (msglen > 0)) {
                    msg = new byte[msglen];
                    try {
                        System.arraycopy(data, startIndex, msg, 0, msglen);
                    } catch (IndexOutOfBoundsException ex) {
                        log(" IndexOutOfBoundsException while parsing msg tag");
                    } catch (ArrayStoreException ex) {
                        log(" ArrayStoreException while parsing msg tag");
                    } catch (NullPointerException ex) {
                        log(" NullPointerException while parsing msg tag");
                    }

                }

                // Call message handler based on message type present in tag
                switch (tag.getType()) {
                    case ImsQmiIF.UNSOL_RESPONSE:
                        processUnsolicited(tag, msg);
                        break;
                    case ImsQmiIF.RESPONSE:
                        processSolicited(tag, msg);
                        break;
                    default:
                        log(" Unknown Message Type  ");
                        break;
                }
            }
            else {
                log("Error in parsing msg tag");
            }
        } else {
            log("Error in parsing msg tag length");
        }
        releaseWakeLockIfDone();
    }

    /**
     * Release each request in mReqeustsList then clear the list
     *
     * @param error is the ImsQmiIF.Error sent back
     * @param loggable true means to print all requests in mRequestslist
     */
    private void clearRequestsList(int error, boolean loggable) {
        IFRequest rr;
        synchronized (mRequestsList) {
            int count = mRequestsList.size();
            if (IF_LOGD && loggable) {
                Log.d(LOG_TAG, "WAKE_LOCK_TIMEOUT " +
                        " mReqPending=" + mRequestMessagesPending +
                        " mRequestList=" + count);
            }

            for (int i = 0; i < count; i++) {
                rr = mRequestsList.get(i);
                if (IF_LOGD && loggable) {
                    Log.d(LOG_TAG, i + ": [" + rr.mSerial + "] " +
                            msgIdToString(rr.mRequest));
                }
                rr.onError(error, null);
                rr.release();
            }
            mRequestsList.clear();
            mRequestMessagesWaiting = 0;
        }
    }

    private IFRequest findAndRemoveRequestFromList(int serial) {
        synchronized (mRequestsList) {
            for (int i = 0, s = mRequestsList.size(); i < s; i++) {
                IFRequest rr = mRequestsList.get(i);

                if (rr.mSerial == serial) {
                    mRequestsList.remove(i);
                    if (mRequestMessagesWaiting > 0)
                        mRequestMessagesWaiting--;
                    return rr;
                }
            }
        }

        return null;
    }

    protected void processSolicited(ImsQmiIF.MsgTag tag, byte[] message) {
        boolean found = false;

        int serial = tag.getToken();
        int error = tag.getError();
        int id = tag.getId();

        IFRequest rr;

        rr = findAndRemoveRequestFromList(serial);

        if (rr == null) {
            Log.w(LOG_TAG, "Unexpected solicited response! sn: "
                    + serial + " error: " + error);
            return;
        }

        Object ret = null;

        if (error == ImsQmiIF.E_SUCCESS || ((message != null) && (message.length >= 1))) {
            // either command succeeds or command fails but with data payload
            try {
                switch (id) {
                    case ImsQmiIF.REQUEST_IMS_REGISTRATION_STATE:
                        ret = responseImsRegistration(message);
                        break;
                    case ImsQmiIF.REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND: {
                        if (mTestingEmergencyCall.getAndSet(false)) {
                            if (mEmergencyCallbackModeRegistrant != null) {
                                log("testing emergency call, notify ECM Registrants");
                                mEmergencyCallbackModeRegistrant.notifyRegistrant();
                            }
                        }
                        break;
                    }
                    case ImsQmiIF.REQUEST_GET_CURRENT_CALLS:
                        ret = responseCallList(message);
                        break;
                    case ImsQmiIF.REQUEST_LAST_CALL_FAIL_CAUSE:
                        ret = responseCallFailCause(message);
                        break;
                    case ImsQmiIF.REQUEST_GET_CLIR:
                        ret = responseGetClir(message);
                        break;
                    case ImsQmiIF.REQUEST_QUERY_CALL_FORWARD_STATUS:
                        ret = responseQueryCallForward(message);
                        break;
                    case ImsQmiIF.REQUEST_QUERY_CALL_WAITING:
                        ret = responseQueryCallWaiting(message);
                        break;
                    case ImsQmiIF.REQUEST_QUERY_CLIP:
                        ret = responseQueryClip(message);
                        break;
                    case ImsQmiIF.REQUEST_QUERY_SERVICE_STATUS:
                        ret = handleSrvStatus(message);
                        break;
                    case ImsQmiIF.REQUEST_SET_CALL_FORWARD_STATUS:
                    case ImsQmiIF.REQUEST_SET_CALL_WAITING:
                    case ImsQmiIF.REQUEST_SUPP_SVC_STATUS:
                        ret = responseSuppSvcStatus(message);
                        break;
                    case ImsQmiIF.REQUEST_QUERY_VT_CALL_QUALITY:
                        ret = responseQueryVideoCallQuality(message);
                        break;
                    case ImsQmiIF.REQUEST_SET_SERVICE_STATUS:
                    case ImsQmiIF.REQUEST_DIAL:
                    case ImsQmiIF.REQUEST_ANSWER:
                    case ImsQmiIF.REQUEST_HANGUP:
                    case ImsQmiIF.REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE:
                    case ImsQmiIF.REQUEST_CONFERENCE:
                    case ImsQmiIF.REQUEST_EXIT_EMERGENCY_CALLBACK_MODE:
                    case ImsQmiIF.REQUEST_DTMF:
                    case ImsQmiIF.REQUEST_DTMF_START:
                    case ImsQmiIF.REQUEST_DTMF_STOP:
                    case ImsQmiIF.REQUEST_HANGUP_WAITING_OR_BACKGROUND:
                    case ImsQmiIF.REQUEST_MODIFY_CALL_INITIATE:
                    case ImsQmiIF.REQUEST_MODIFY_CALL_CONFIRM:
                    case ImsQmiIF.REQUEST_SET_CLIR:
                    case ImsQmiIF.REQUEST_IMS_REG_STATE_CHANGE:
                    case ImsQmiIF.REQUEST_SET_SUPP_SVC_NOTIFICATION:
                    case ImsQmiIF.REQUEST_SET_VT_CALL_QUALITY:
                        // no data
                        break;
                    default:
                        throw new RuntimeException("Unrecognized solicited response: "
                                + rr.mRequest);
                }
            } catch (Throwable tr) {
                // Exceptions here usually mean invalid responses

                Log.w(LOG_TAG, rr.serialString() + "< "
                        + msgIdToString(rr.mRequest)
                        + " exception, possible invalid response", tr);

                if (rr.mResult != null) {
                    AsyncResult.forMessage(rr.mResult, null, tr);
                    rr.mResult.sendToTarget();
                }
                rr.release();
                return;
            }
        }
        if (error != ImsQmiIF.E_SUCCESS) {
            rr.onError(error, ret);
            rr.release();
            return;
        }

        if (IF_LOGD)
            log(rr.serialString() + "< " + msgIdToString(rr.mRequest)
                    + " " + retToString(rr.mRequest, ret));

        if (rr.mResult != null) {
            AsyncResult.forMessage(rr.mResult, ret, null);
            rr.mResult.sendToTarget();
        }

        rr.release();
    }

    private String retToString(int req, Object ret) {

        if (ret == null)
            return "";

        StringBuilder sb;
        String s;
        int length;
        if (ret instanceof int[]) {
            int[] intArray = (int[]) ret;
            length = intArray.length;
            sb = new StringBuilder("{");
            if (length > 0) {
                int i = 0;
                sb.append(intArray[i++]);
                while (i < length) {
                    sb.append(", ").append(intArray[i++]);
                }
            }
            sb.append("}");
            s = sb.toString();
        } else if (ret instanceof String[]) {
            String[] strings = (String[]) ret;
            length = strings.length;
            sb = new StringBuilder("{");
            if (length > 0) {
                int i = 0;
                sb.append(strings[i++]);
                while (i < length) {
                    sb.append(", ").append(strings[i++]);
                }
            }
            sb.append("}");
            s = sb.toString();
        } else if (req == ImsQmiIF.REQUEST_GET_CURRENT_CALLS) {
            ArrayList<DriverCallIms> calls = (ArrayList<DriverCallIms>) ret;
            sb = new StringBuilder(" ");
            for (DriverCall dc : calls) {
                sb.append("[").append(dc).append("] ");
            }
            s = sb.toString();
        } else {
            s = ret.toString();
        }
        return s;
    }

    public void registerForModifyCall(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mModifyCallRegistrants.add(r);
    }

    public void unregisterForModifyCall(Handler h) {
        mModifyCallRegistrants.remove(h);
    }

    protected void processUnsolicited(ImsQmiIF.MsgTag tag, byte[] message) {
        int response = tag.getId();
        Object ret = null;

        try {
            switch (response) {
                case ImsQmiIF.UNSOL_RINGBACK_TONE:
                    if (message != null) ret = responseCallRingBack(message);
                    break;
                case ImsQmiIF.UNSOL_CALL_RING:
                case ImsQmiIF.UNSOL_RESPONSE_CALL_STATE_CHANGED:
                case ImsQmiIF.UNSOL_RESPONSE_IMS_NETWORK_STATE_CHANGED:
                case ImsQmiIF.UNSOL_ENTER_EMERGENCY_CALLBACK_MODE:
                case ImsQmiIF.UNSOL_EXIT_EMERGENCY_CALLBACK_MODE:
                    ret = null;
                    break;
                case ImsQmiIF.UNSOL_MODIFY_CALL:
                    if (message != null) ret = responseModifyCall(message);
                    if (IF_LOGD) unsljLogRet(response, ret);
                    mModifyCallRegistrants
                            .notifyRegistrants(new AsyncResult(null, ret, null));
                    break;
                case ImsQmiIF.UNSOL_RESPONSE_HANDOVER:
                    if (message != null) ret = responseHandover(message);
                    break;
                case ImsQmiIF.UNSOL_REFRESH_CONF_INFO:
                    if (message != null) ret = handleRefreshInfo(message);
                    break;
                case ImsQmiIF.UNSOL_SRV_STATUS_UPDATE:
                    if (message != null) ret = handleSrvStatus(message);
                    break;
                case ImsQmiIF.UNSOL_TTY_NOTIFICATION:
                    if (message != null) ret = handleTtyNotify(message);
                    break;
                case ImsQmiIF.UNSOL_SUPP_SVC_NOTIFICATION:
                    if (message != null) ret = responseSuppServiceNotification(message);
                    break;
                case ImsQmiIF.UNSOL_RADIO_STATE_CHANGED:
                    if (message != null) ret = handleRadioStateChange(message);
                    break;
                default:
                    throw new RuntimeException("Unrecognized unsol response: " + response);
            }
        } catch (Throwable tr) {
            Log.e(LOG_TAG, "Exception processing unsol response: " + response +
                    "Exception:" + tr.toString());
            return;
        }

        switch (response) {
            case ImsQmiIF.UNSOL_RESPONSE_IMS_NETWORK_STATE_CHANGED:
                if (IF_LOGD)
                    unsljLog(response);

                mImsNetworkStateChangedRegistrants
                        .notifyRegistrants(new AsyncResult(null, null, null));
                break;
            case ImsQmiIF.UNSOL_RESPONSE_CALL_STATE_CHANGED:
                if (IF_LOGD)
                    unsljLog(response);
                mCallStateRegistrants
                        .notifyRegistrants(new AsyncResult(null, null, null));
                break;
            case ImsQmiIF.UNSOL_CALL_RING:
                if (IF_LOGD)
                    unsljLogRet(response, ret);

                if (mRingRegistrant != null) {
                    mRingRegistrant.notifyRegistrant(
                            new AsyncResult(null, ret, null));
                }
                break;
            case ImsQmiIF.UNSOL_ENTER_EMERGENCY_CALLBACK_MODE:
                if (IF_LOGD)
                    unsljLog(response);

                if (mEmergencyCallbackModeRegistrant != null) {
                    mEmergencyCallbackModeRegistrant.notifyRegistrant();
                }
                break;

            case ImsQmiIF.UNSOL_RINGBACK_TONE:
                boolean playtone = false;
                if (IF_LOGD)
                    unsljLogvRet(response, ret);
                if (ret != null) playtone = (((int[]) ret)[0] == 1);
                if (mRingbackToneRegistrants != null) {
                    mRingbackToneRegistrants.notifyRegistrants(
                            new AsyncResult(null, playtone, null));
                }
                break;
            case ImsQmiIF.UNSOL_EXIT_EMERGENCY_CALLBACK_MODE:
                if (IF_LOGD)
                    unsljLogRet(response, ret);

                if (mExitEmergencyCallbackModeRegistrants != null) {
                    mExitEmergencyCallbackModeRegistrants.notifyRegistrants(
                            new AsyncResult(null, null, null));
                }
                break;
            case ImsQmiIF.UNSOL_RESPONSE_HANDOVER:
                if (IF_LOGD)
                    unsljLogRet(response, ret);
                if (ret != null) {
                    mHandoverStatusRegistrants
                    .notifyRegistrants(new AsyncResult(null, ret, null));
                }
                break;
            case ImsQmiIF.UNSOL_REFRESH_CONF_INFO:
                if (IF_LOGD)
                    unsljLogRet(response, ret);
                if (ret != null) {
                    mRefreshConfInfoRegistrations
                    .notifyRegistrants(new AsyncResult(null, ret, null));
                }
                break;
            case ImsQmiIF.UNSOL_SRV_STATUS_UPDATE:
                if (IF_LOGD)
                    unsljLogRet(response, ret);
                if (ret != null) {
                    mSrvStatusRegistrations
                    .notifyRegistrants(new AsyncResult(null, ret, null));
                }
                break;
            case ImsQmiIF.UNSOL_TTY_NOTIFICATION:
                if (IF_LOGD)
                    unsljLogRet(response, ret);
                if (ret != null) {
                    mTtyStatusRegistrants
                    .notifyRegistrants(new AsyncResult(null, ret, null));
                }
                break;
            case ImsQmiIF.UNSOL_SUPP_SVC_NOTIFICATION:
                if (IF_LOGD)
                    unsljLogRet(response, ret);
                if (mSsnRegistrant != null) {
                    mSsnRegistrant.notifyRegistrant(
                                        new AsyncResult (null, ret, null));
                }
                break;
        }
    }

    private Object responseModifyCall(byte[] message) {
        CallModify callModify = new CallModify();
        try {
            ImsQmiIF.CallModify callModifyIF = ImsQmiIF.CallModify.parseFrom(message);
            callModify.call_details.call_type = callModifyIF.getCallDetails().getCallType();
            callModify.call_details.call_domain = callModifyIF.getCallDetails().getCallDomain();
            List<String> extrasList = callModifyIF.getCallDetails().getExtrasList();
            callModify.call_details.extras = extrasList.toArray(new String[extrasList.size()]);
            callModify.call_index = callModifyIF.getCallIndex();
            callModify.error = callModifyIF.hasError() ? callModifyIF.getError()
                    : ImsQmiIF.E_SUCCESS;
            log("responseModifyCall " + callModify);
        } catch (com.google.protobuf.micro.InvalidProtocolBufferMicroException ex) {
            log(" Message parsing. InvalidProtocolBufferException ");
        }
        return callModify;
    }

    private Object responseQueryVideoCallQuality(byte[] message) {
        if (message == null) {
            log("responseQueryVideoCallQuality failed, message is null");
        } else {
            try {
                ImsQmiIF.VideoCallQuality vQuality = ImsQmiIF.VideoCallQuality.parseFrom(message);
                if (vQuality.hasQuality()) {
                    int[] quality = new int[1];
                    quality[0] = vQuality.getQuality();
                    log("responseQueryVideoCallQuality, quality=" + quality[0]);
                } else {
                    log("responseQueryVideoCallQuality failed. Quality is not set.");
                }
            } catch (com.google.protobuf.micro.InvalidProtocolBufferMicroException ex) {
                log(" Message parsing. InvalidProtocolBufferException ");
            }
        }
        return null;
    }

    static String errorIdToString(int request) {
        String errorMsg;
        switch (request) {
            case ImsQmiIF.E_SUCCESS:
                errorMsg = "SUCCESS";
                break;
            case ImsQmiIF.E_RADIO_NOT_AVAILABLE:
                errorMsg = "E_RADIO_NOT_AVAILABLE";
                break;
            case ImsQmiIF.E_GENERIC_FAILURE:
                errorMsg = "E_GENERIC_FAILURE";
                break;
            case ImsQmiIF.E_REQUEST_NOT_SUPPORTED:
                errorMsg = "E_REQUEST_NOT_SUPPORTED";
                break;
            case ImsQmiIF.E_CANCELLED:
                errorMsg = "E_CANCELLED";
                break;
            case ImsQmiIF.E_UNUSED:
                errorMsg = "E_UNUSED";
                break;
            case ImsQmiIF.E_INVALID_PARAMETER:
                errorMsg = "E_INVALID_PARAMETER";
                break;
            case ImsQmiIF.E_REJECTED_BY_REMOTE:
                errorMsg = "E_REJECTED_BY_REMOTE";
                break;
            default:
                errorMsg = "E_UNKNOWN";
                break;
        }
        return errorMsg;
    }

    static String msgIdToString(int request) {
        // TODO - check all supported messages are covered
        switch (request) {
            case ImsQmiIF.REQUEST_GET_CURRENT_CALLS:
                return "GET_CURRENT_CALLS";
            case ImsQmiIF.REQUEST_DIAL:
                return "DIAL";
            case ImsQmiIF.REQUEST_ANSWER:
                return "REQUEST_ANSWER";
            case ImsQmiIF.REQUEST_HANGUP:
                return "HANGUP";
            case ImsQmiIF.REQUEST_HANGUP_WAITING_OR_BACKGROUND:
                return "HANGUP_WAITING_OR_BACKGROUND";
            case ImsQmiIF.REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND:
                return "HANGUP_FOREGROUND_RESUME_BACKGROUND";
            case ImsQmiIF.REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE:
                return "ImsQmiIF.REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE";
            case ImsQmiIF.REQUEST_CONFERENCE:
                return "CONFERENCE";
            case ImsQmiIF.REQUEST_UDUB:
                return "UDUB";
            case ImsQmiIF.REQUEST_MODIFY_CALL_INITIATE:
                return "MODIFY_CALL_INITIATE";
            case ImsQmiIF.REQUEST_MODIFY_CALL_CONFIRM:
                return "MODIFY_CALL_CONFIRM";
            case ImsQmiIF.UNSOL_MODIFY_CALL:
                return "UNSOL_MODIFY_CALL";
            case ImsQmiIF.REQUEST_LAST_CALL_FAIL_CAUSE:
                return "LAST_CALL_FAIL_CAUSE";
            case ImsQmiIF.REQUEST_DTMF:
                return "DTMF";
            case ImsQmiIF.REQUEST_DTMF_START:
                return "DTMF_START";
            case ImsQmiIF.REQUEST_DTMF_STOP:
                return "DTMF_STOP";
            case ImsQmiIF.REQUEST_EXPLICIT_CALL_TRANSFER:
                return "ImsQmiIF.REQUEST_EXPLICIT_CALL_TRANSFER";
            case ImsQmiIF.REQUEST_EXIT_EMERGENCY_CALLBACK_MODE:
                return "ImsQmiIF.REQUEST_EXIT_EMERGENCY_CALLBACK_MODE";
            case ImsQmiIF.REQUEST_IMS_REGISTRATION_STATE:
                return "REQUEST_IMS_REGISTRATION_STATE";
            case ImsQmiIF.REQUEST_QUERY_CLIP:
                return "REQUEST_QUERY_CLIP";
            case ImsQmiIF.REQUEST_QUERY_SERVICE_STATUS:
                return "REQUEST_QUERY_SERVICE_STATUS";
            case ImsQmiIF.REQUEST_SET_SERVICE_STATUS:
                return "REQUEST_SET_SERVICE_STATUS";
            case ImsQmiIF.REQUEST_GET_CLIR:
                return "REQUEST_GET_CLIR";
            case ImsQmiIF.REQUEST_SET_CLIR:
                return "REQUEST_SET_CLIR";
            case ImsQmiIF.REQUEST_QUERY_CALL_FORWARD_STATUS:
                return "REQUEST_QUERY_CALL_FORWARD_STATUS";
            case ImsQmiIF.REQUEST_SET_CALL_FORWARD_STATUS:
                return "REQUEST_SET_CALL_FORWARD_STATUS";
            case ImsQmiIF.REQUEST_QUERY_CALL_WAITING:
                return "REQUEST_QUERY_CALL_WAITING";
            case ImsQmiIF.REQUEST_SET_CALL_WAITING:
                return "REQUEST_SET_CALL_WAITING";
            case ImsQmiIF.REQUEST_SET_SUPP_SVC_NOTIFICATION:
                return "REQUEST_SET_SUPP_SVC_NOTIFICATION";
            case ImsQmiIF.REQUEST_SUPP_SVC_STATUS:
                return "REQUEST_SUPP_SVC_STATUS";
            case ImsQmiIF.REQUEST_QUERY_VT_CALL_QUALITY:
                return "REQUEST_QUERY_VT_CALL_QUALITY";
            case ImsQmiIF.REQUEST_SET_VT_CALL_QUALITY:
                return "REQUEST_SET_VT_CALL_QUALITY";
            case ImsQmiIF.UNSOL_RESPONSE_IMS_NETWORK_STATE_CHANGED:
                return "UNSOL_RESPONSE_IMS_NETWORK_STATE_CHANGED";
            case ImsQmiIF.UNSOL_RESPONSE_CALL_STATE_CHANGED:
                return "UNSOL_RESPONSE_CALL_STATE_CHANGED";
            case ImsQmiIF.UNSOL_CALL_RING:
                return "UNSOL_CALL_RING";
            case ImsQmiIF.UNSOL_ENTER_EMERGENCY_CALLBACK_MODE:
                return "UNSOL_ENTER_EMERGENCY_CALLBACK_MODE";
            case ImsQmiIF.UNSOL_RINGBACK_TONE:
                return "UNSOL_RINGBACK_TONE";
            case ImsQmiIF.UNSOL_EXIT_EMERGENCY_CALLBACK_MODE:
                return "UNSOL_EXIT_EMERGENCY_CALLBACK_MODE";
            case ImsQmiIF.REQUEST_IMS_REG_STATE_CHANGE:
                return "REQUEST_IMS_REG_STATE_CHANGE";
            case ImsQmiIF.UNSOL_RESPONSE_HANDOVER:
                return "UNSOL_RESPONSE_HANDOVER";
            case ImsQmiIF.UNSOL_REFRESH_CONF_INFO:
                return "UNSOL_REFRESH_CONF_INFO";
            case ImsQmiIF.UNSOL_SRV_STATUS_UPDATE:
                return "UNSOL_SRV_STATUS_UPDATE";
            case ImsQmiIF.UNSOL_SUPP_SVC_NOTIFICATION:
                return "UNSOL_SUPP_SVC_NOTIFICATION";
            case ImsQmiIF.UNSOL_TTY_NOTIFICATION:
                return "UNSOL_TTY_NOTIFICATION";
            case ImsQmiIF.UNSOL_RADIO_STATE_CHANGED:
                return "UNSOL_RADIO_STATE_CHANGED";
            default:
                return "<unknown message>";
        }
    }

    public void log(String msg) {
        Log.d(LOG_TAG, msg + "");
    }

    public void logv(String msg) {
        if (ImsSenderRxr.IF_LOGV) {
            Log.v(LOG_TAG, msg + "");
        }
    }

    /**
     * Use this only for unimplemented methods. Prints stack trace if the
     * unimplemented method is ever called
     */
    public void logUnimplemented() {
        try {
            Exception e = new Exception();
            throw e;
        } catch (Exception e) {
            Log.d(LOG_TAG, "Unimplemented method. Stack trace: ");
            e.printStackTrace();
        }
    }

    public void unsljLog(int response) {
        log("[UNSL]< " + msgIdToString(response));
    }

    public void unsljLogMore(int response, String more) {
        log("[UNSL]< " + msgIdToString(response) + " " + more);
    }

    public void unsljLogRet(int response, Object ret) {
        log("[UNSL]< " + msgIdToString(response) + " " + retToString(response, ret));
    }

    public void unsljLogvRet(int response, Object ret) {
        logv("[UNSL]< " + msgIdToString(response) + " " + retToString(response, ret));
    }

    @Override
    public void setPhoneType(int phoneType) { // Called by Phone constructor
        if (IF_LOGD)
            log("setPhoneType=" + phoneType + " old value=" + mPhoneType);
        mPhoneType = phoneType;
    }

    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("IMS INTERFACE:");
        pw.println(" mSocket=" + mSocket);
        pw.println(" mSenderThread=" + mSenderThread);
        pw.println(" mSender=" + mSender);
        pw.println(" mReceiverThread=" + mReceiverThread);
        pw.println(" mReceiver=" + mReceiver);
        pw.println(" mWakeLock=" + mWakeLock);
        pw.println(" mWakeLockTimeout=" + mWakeLockTimeout);
        synchronized (mRequestsList) {
            pw.println(" mRequestMessagesPending=" + mRequestMessagesPending);
            pw.println(" mRequestMessagesWaiting=" + mRequestMessagesWaiting);
            int count = mRequestsList.size();
            pw.println(" mRequestList count=" + count);
            for (int i = 0; i < count; i++) {
                IFRequest rr = mRequestsList.get(i);
                pw.println("  [" + rr.mSerial + "] " + msgIdToString(rr.mRequest));
            }
        }
        pw.println(" mLastNITZTimeInfo=" + mLastNITZTimeInfo);
        pw.println(" mTestingEmergencyCall=" + mTestingEmergencyCall.get());
    }

    /** Message tag encoding */
    public void encodeMsg(int id, Message result, byte[] msg) {

        int msgLen = 0;
        int index = 0;
        int totalPacketLen = 0;
        IFRequest rr = IFRequest.obtain(id, result);

        ImsQmiIF.MsgTag msgtag;
        msgtag = new ImsQmiIF.MsgTag();
        msgtag.setToken(rr.mSerial);
        msgtag.setType(ImsQmiIF.REQUEST);
        msgtag.setId(rr.mRequest);
        msgtag.setError(ImsQmiIF.E_SUCCESS);
        byte[] tagb = msgtag.toByteArray();

        if (msg != null) {
            msgLen = msg.length;
        }

        // data is the byte stream that will be sent across the socket
        // byte 0..3 = total msg length (i.e. tag length + msglen)
        // byte 4 = msgtag length
        // byte 5..tagb.length = msgtag in bytes
        // byte 5+tagb.lenght..msglen = msg in bytes
        totalPacketLen = PDU_LENGTH_OFFSET + tagb.length + msgLen + MSG_TAG_LENGTH;
        rr.mData = new byte[totalPacketLen];

        // length in big endian
        rr.mData[index++] = rr.mData[index++] = 0;
        rr.mData[index++] = (byte)(((totalPacketLen - PDU_LENGTH_OFFSET) >> 8) & 0xff);
        rr.mData[index++] = (byte)(((totalPacketLen - PDU_LENGTH_OFFSET)) & 0xff);

        rr.mData[index++] = (byte) tagb.length;

        try {
            System.arraycopy(tagb, 0, rr.mData, index, tagb.length);
        } catch (IndexOutOfBoundsException ex) {
            log(" IndexOutOfBoundsException while encoding msg tag");
        } catch (ArrayStoreException ex) {
            log(" ArrayStoreException while parsing msg tag");
        } catch (NullPointerException ex) {
            log(" NullPointerException while parsing msg tag");
        }

        if (msgLen > 0) {
            try {
                System.arraycopy(msg, 0, rr.mData, (index + tagb.length), msgLen);
            } catch (IndexOutOfBoundsException ex) {
                log(" IndexOutOfBoundsException while encoding msg");
            } catch (ArrayStoreException ex) {
                log(" ArrayStoreException while parsing msg");
            } catch (NullPointerException ex) {
                log(" NullPointerException while parsing msg");
            }
        }

        if (IF_LOGD) {
            log(rr.serialString() + "> " + msgIdToString(rr.mRequest));
            for (int count = 0; count < rr.mData.length; count++) {
                log(" " + rr.mData[count]);
            }
        }

        send(rr);
        return;
    }

    private byte[] processDial(String address, int clirMode, CallDetails callDetails) {
        boolean isConferenceUri = false;
        if (callDetails != null && callDetails.extras != null) {
            String value = callDetails.getValueForKeyFromExtras(callDetails.extras,
                    CallDetails.EXTRAS_IS_CONFERENCE_URI);
            if (value != null && Boolean.valueOf(value)) {
                isConferenceUri = true;
            }
        }

        logv("process dial isConfererenceUri = " + isConferenceUri);
        ImsQmiIF.CallDetails callDetailsIF = new ImsQmiIF.CallDetails();
        callDetailsIF.setCallDomain(callDetails.call_domain);
        callDetailsIF.setCallType(callDetails.call_type);

        ImsQmiIF.Dial dialIF = new ImsQmiIF.Dial();
        dialIF.setAddress(address);
        dialIF.setCallDetails(callDetailsIF);
        dialIF.setClir(clirMode);
        switch(clirMode){
            case CommandsInterface.CLIR_SUPPRESSION:
                dialIF.setPresentation(ImsQmiIF.IP_PRESENTATION_NUM_RESTRICTED);
                break;
            case CommandsInterface.CLIR_INVOCATION:
            case CommandsInterface.CLIR_DEFAULT:
            default:
                dialIF.setPresentation(ImsQmiIF.IP_PRESENTATION_NUM_ALLOWED);
                break;
        }

        if (isConferenceUri) {
            dialIF.setIsConferenceUri(isConferenceUri);
        }

        byte[] dialb = dialIF.toByteArray();
        return dialb;
    }

    public void addParticipant(String address, int clirMode, CallDetails callDetails,
            Message result) {
        logv("addParticipant address= " + address + "clirMode= " + clirMode
                + " callDetails= " + callDetails);
        byte[] dialb = processDial(address, clirMode, callDetails);

        encodeMsg(ImsQmiIF.REQUEST_ADD_PARTICIPANT, result, dialb);
    }

    public void
    dial(String address, int clirMode, CallDetails callDetails,
            Message result) {
        logv("dial address= " + address + "clirMode= " + clirMode
                + " callDetails= " + callDetails);
        byte[] dialb = processDial(address, clirMode, callDetails);

        encodeMsg(ImsQmiIF.REQUEST_DIAL, result, dialb);
    }

    public void
    acceptCall(Message result, int callType) {
        logv("acceptCall callType=" + callType);
        int callTypeIF = callType;
        ImsQmiIF.Answer answerIF = new ImsQmiIF.Answer();
        answerIF.setCallType(callTypeIF);
        byte[] ansb = answerIF.toByteArray();

        encodeMsg(ImsQmiIF.REQUEST_ANSWER, result, ansb);
    }

    public void deflectCall(int index, String number, Message result) {
        logv("deflect call to: " + number + "connid:" + index);
        ImsQmiIF.DeflectCall deflectCall = new ImsQmiIF.DeflectCall();
        deflectCall.setConnIndex(index);
        deflectCall.setNumber(number);
        byte[] deflectCallb = deflectCall.toByteArray();

        encodeMsg(ImsQmiIF.REQUEST_DEFLECT_CALL, result, deflectCallb);
    }

    /* Not used yet - TODO figure out how to get this presentation value before calling */
    public void
    acceptCall(Message result, int callType, int presentation) {
        logv("acceptCall callType= " + callType + " presentation= " + presentation);
        int callTypeIF = callType;
        ImsQmiIF.Answer answerIF = new ImsQmiIF.Answer();
        answerIF.setCallType(callTypeIF);
        answerIF.setPresentation(presentation);
        byte[] ansb = answerIF.toByteArray();

        encodeMsg(ImsQmiIF.REQUEST_ANSWER, result, ansb);
    }

    public void
    hangupConnection(int index, Message result) {
        logv("hangupConnection index= " + index);
        ImsQmiIF.Hangup hangupIF = new ImsQmiIF.Hangup();
        hangupIF.setConnIndex(index);
        hangupIF.setMultiParty(false);
        byte[] hangupb = hangupIF.toByteArray();
        encodeMsg(ImsQmiIF.REQUEST_HANGUP, result, hangupb);
    }

    public void
    hangupWithReason(int connectionId, String userUri, String confUri,
            boolean mpty, int failCause, String errorInfo, Message result) {
        logv("hangupWithReason connId= " + connectionId + " userUri= "+ userUri + " confUri= "+
                confUri + "mpty = " + mpty + "failCause = " + failCause);
        ImsQmiIF.Hangup hangupIF = new ImsQmiIF.Hangup();
        /* If Calltracker has a matching local connection the connection id will be used.
         * if there is no matching connection object and if it is a remotely added participant
         * then connection id will not be present hence -1
         */
        if(connectionId != -1) {
            hangupIF.setConnIndex(connectionId);
        }
        hangupIF.setMultiParty(mpty);
        if(userUri != null)
            hangupIF.setConnUri(userUri);
        ImsQmiIF.CallFailCauseResponse callfail = new ImsQmiIF.CallFailCauseResponse();
        if (errorInfo != null && !errorInfo.isEmpty()) {
            logv("hangupWithReason errorInfo = " + errorInfo);
            ByteStringMicro errorInfoStringMicro = ByteStringMicro.copyFrom(errorInfo.getBytes());
            callfail.setErrorinfo(errorInfoStringMicro);
        }
        callfail.setFailcause(failCause);
        hangupIF.setFailCauseResponse(callfail);

        /* TODO: Change proto file for conf_id to confUri and then enable the line below
        This okay for now as there is not more than one conference simultaneously */
        //hangupIF.setConfUri(confUri);
        byte[] hangupb = hangupIF.toByteArray();
        encodeMsg(ImsQmiIF.REQUEST_HANGUP, result, hangupb);
    }

    public void
    getLastCallFailCause(Message result) {
        encodeMsg(ImsQmiIF.REQUEST_LAST_CALL_FAIL_CAUSE, result, null);
    }

    public void queryServiceStatus(Message result) {
        encodeMsg(ImsQmiIF.REQUEST_QUERY_SERVICE_STATUS, result, null);
    }

    public void setServiceStatus(Message result, int srvType, int network, int enabled,
            int restrictCause) {
        ImsQmiIF.StatusForAccessTech srvSetting = new ImsQmiIF.StatusForAccessTech();
        srvSetting.setNetworkMode(network);
        srvSetting.setStatus(enabled); /*
                                        * TODO: Switch when values of enabled
                                        * and restrictCause are defined in
                                        * Phone.java
                                        */
        srvSetting.setRestrictionCause(restrictCause);
        ImsQmiIF.Info srvInfo = new ImsQmiIF.Info();
        srvInfo.setIsValid(true);
        srvInfo.setCallType(srvType);
        srvInfo.addAccTechStatus(srvSetting);
        encodeMsg(ImsQmiIF.REQUEST_SET_SERVICE_STATUS, result, srvInfo.toByteArray());
    }

    public void
    getCurrentCalls(Message result) {
        encodeMsg(ImsQmiIF.REQUEST_GET_CURRENT_CALLS, result, null);
    }

    public void
            getImsRegistrationState(Message result) {
        encodeMsg(ImsQmiIF.REQUEST_IMS_REGISTRATION_STATE, result, null);
    }

    public void
            sendImsRegistrationState(int imsRegState, Message result) {
        logv("sendImsRegistration " + "imsRegState= " + imsRegState);

        ImsQmiIF.Registration registerImsIF = new ImsQmiIF.Registration();
        registerImsIF.setState(imsRegState);

        byte[] registerImsb = registerImsIF.toByteArray();

        encodeMsg(ImsQmiIF.REQUEST_IMS_REG_STATE_CHANGE, result, registerImsb);

   }

    private byte[] setCallModify(CallModify callModify) {
        logv("setCallModify callModify= " + callModify);
        ImsQmiIF.CallDetails callDetailsIF = new ImsQmiIF.CallDetails();
        callDetailsIF.setCallType(callModify.call_details.call_type);
        callDetailsIF.setCallDomain(callModify.call_details.call_domain);

        ImsQmiIF.CallModify callModifyIF = new ImsQmiIF.CallModify();
        callModifyIF.setCallDetails(callDetailsIF);
        callModifyIF.setCallIndex(callModify.call_index);

        // This field is not used for outgoing requests.
        // callModifyIF.setError(callModify.error);

        byte[] callModifyb = callModifyIF.toByteArray();
        return callModifyb;
    }

    public void modifyCallInitiate(Message result, CallModify callModify) {
        logv("modifyCallInitiate callModify= " + callModify);
        byte[] callModifyb = setCallModify(callModify);
        encodeMsg(ImsQmiIF.REQUEST_MODIFY_CALL_INITIATE, result, callModifyb);
    }

    public void modifyCallConfirm(Message result, CallModify callModify) {
        logv("modifyCallConfirm callModify= " + callModify);
        byte[] callModifyb = setCallModify(callModify);
        encodeMsg(ImsQmiIF.REQUEST_MODIFY_CALL_CONFIRM, result, callModifyb);
    }

    public void switchWaitingOrHoldingAndActive(Message result, int callType) {
        logv("switchWaitingOrHoldingAndActive callType=" + callType);
        ImsQmiIF.SwitchWaitingOrHoldingAndActive switchIF
            = new ImsQmiIF.SwitchWaitingOrHoldingAndActive();
        switchIF.setCallType(callType);
        byte[] switchb = switchIF.toByteArray();
        encodeMsg(ImsQmiIF.REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE, result, switchb);
    }

    public void switchWaitingOrHoldingAndActive(Message result) {
        ImsQmiIF.SwitchWaitingOrHoldingAndActive switchIF
            = new ImsQmiIF.SwitchWaitingOrHoldingAndActive();
        byte[] switchb = switchIF.toByteArray();
        encodeMsg(ImsQmiIF.REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE, result, switchb);
    }

    public void hangupForegroundResumeBackground(Message result) {
        encodeMsg(ImsQmiIF.REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND, result, null);
    }

    public void hangupWaitingOrBackground(Message result) {
        encodeMsg(ImsQmiIF.REQUEST_HANGUP_WAITING_OR_BACKGROUND, result, null);
    }

    public void conference(Message result) {
        encodeMsg(ImsQmiIF.REQUEST_CONFERENCE, result, null);
    }

    public void explicitCallTransfer(Message result) {
        encodeMsg(ImsQmiIF.REQUEST_EXPLICIT_CALL_TRANSFER, result, null);
    }

    public void rejectCall(Message result) {
        encodeMsg(ImsQmiIF.REQUEST_UDUB, result, null);
    }

    public void sendDtmf(char c, Message result) {
        ImsQmiIF.Dtmf dtmfIF = new ImsQmiIF.Dtmf();
        dtmfIF.setDtmf(Character.toString(c));
        byte[] dtmfb = dtmfIF.toByteArray();

        encodeMsg(ImsQmiIF.REQUEST_DTMF, result, dtmfb);
    }

    public void startDtmf(char c, Message result) {
        ImsQmiIF.Dtmf dtmfIF = new ImsQmiIF.Dtmf();
        dtmfIF.setDtmf(Character.toString(c));

        byte[] dtmfb = dtmfIF.toByteArray();

        encodeMsg(ImsQmiIF.REQUEST_DTMF_START, result, dtmfb);
    }

    public void stopDtmf(Message result) {
        encodeMsg(ImsQmiIF.REQUEST_DTMF_STOP, result, null);
    }

    // RESPONSE PROCESSING
    private Object responseCallFailCause(byte[] callFailB) {
        log(" responseCallFailCause ");
        ImsQmiIF.CallFailCauseResponse callfail = null;
        try {

            callfail = ImsQmiIF.CallFailCauseResponse
                    .parseFrom(callFailB);
            log("callfail cause response" + callfail);
        } catch (com.google.protobuf.micro.InvalidProtocolBufferMicroException ex) {
            log(" Message parsing ");
            log("InvalidProtocolBufferException ");
        }
        return callfail;

    }

    private Object responseCallRingBack(byte[] ringBackB) {
        int[] response = new int[1];
        log(" responseCallRingBack ");

        try {
            ImsQmiIF.RingBackTone ringbackTone = ImsQmiIF.RingBackTone
                    .parseFrom(ringBackB);

            response[0] = ringbackTone.getFlag();
            log("responseCallRingBack " + response[0]);
        } catch (com.google.protobuf.micro.InvalidProtocolBufferMicroException ex) {
            log(" Message parsing ");
            log("InvalidProtocolBufferException ");
        }
        return response;
    }

    protected Object responseImsRegistration(byte[] imsRegB) {
        int[] response = null;
        if (imsRegB != null && imsRegB.length >= 1) {
            try {
                ImsQmiIF.Registration registration = ImsQmiIF.Registration.parseFrom(imsRegB);
                response = new int[1];
                response[0] = registration.getState();
            } catch (com.google.protobuf.micro.InvalidProtocolBufferMicroException ex) {
                log("InvalidProtocolBufferException in message tag parsing");
            }
        }
        return response;
    }

    protected Object responseQueryCallForward(byte[] callInfoList) {

        CallForwardInfo infos[] = null;
        int numInfos = 0;

        if (callInfoList != null) {
            try {
                ImsQmiIF.CallForwardInfoList infoList = ImsQmiIF.CallForwardInfoList
                        .parseFrom(callInfoList);
                numInfos = infoList.getInfoCount();
                infos = new CallForwardInfo[numInfos];
                for (int i = 0; i < numInfos; i++) {
                    ImsQmiIF.CallForwardInfoList.CallForwardInfo callInfo = infoList
                            .getInfo(i);

                    infos[i] = new CallForwardInfo();
                    infos[i].status = callInfo.getStatus();
                    infos[i].reason = callInfo.getReason();
                    infos[i].serviceClass = callInfo.getServiceClass();
                    infos[i].toa = callInfo.getToa();
                    infos[i].number = callInfo.getNumber();
                    infos[i].timeSeconds = callInfo.getTimeSeconds();
                }

            } catch (com.google.protobuf.micro.InvalidProtocolBufferMicroException ex) {
                log("InvalidProtocolBufferException in message tag parsing");
            }
        } else {
            infos = new CallForwardInfo[0];
        }
        return infos;
    }

    protected Object responseQueryCallWaiting(byte[] callWaitingInfo) {
        int[] response = null;

        if (callWaitingInfo != null) {
            try {
                ImsQmiIF.CallWaitingInfo waitingInfo = ImsQmiIF.CallWaitingInfo
                        .parseFrom(callWaitingInfo);
                ImsQmiIF.ServiceClass srvClass = waitingInfo.getServiceClass();

                if (waitingInfo.getServiceStatus() == ImsQmiIF.DISABLED) {
                    response = new int[1];
                    response[0] = ImsQmiIF.DISABLED;
                } else {
                    response = new int[2];
                    response[0] = ImsQmiIF.ENABLED;
                    response[1] = srvClass.getServiceClass();
                }
            } catch (com.google.protobuf.micro.InvalidProtocolBufferMicroException ex) {
                log("InvalidProtocolBufferException in message tag parsing");
            }
        } else {
            response = new int[0];
        }
        return response;
    }

    protected Object responseQueryClip(byte[] clipInfo) {
        int[] response = null;

        if (clipInfo != null) {
            try {
                ImsQmiIF.ClipProvisionStatus clipStatus = ImsQmiIF.ClipProvisionStatus
                        .parseFrom(clipInfo);
                response = new int[1];
                response[0] = clipStatus.getClipStatus();
            } catch (com.google.protobuf.micro.InvalidProtocolBufferMicroException ex) {
                log("InvalidProtocolBufferException in message tag parsing");
            }
        }
        else {
            response = new int[0];
        }
        return response;
    }

    protected Object responseGetClir(byte[] clirInfo) {
        int[] response = null;

        if (clirInfo != null) {
            try {
                ImsQmiIF.Clir info = ImsQmiIF.Clir.parseFrom(clirInfo);

                response = new int[2];

                response[0] = info.getParamN();
                response[1] = info.getParamM();
            } catch (com.google.protobuf.micro.InvalidProtocolBufferMicroException ex) {
                log("InvalidProtocolBufferException in message tag parsing");
            }
        }
        else
        {
            response = new int[0];
        }
        return response;
    }

    protected Object responseHandover(byte[] handoverB) {
        ImsCallTracker.HandoverInfo response = null;
        if (handoverB != null && handoverB.length >= 1) {
            try {
                ImsQmiIF.Handover handover = ImsQmiIF.Handover.parseFrom(handoverB);
                response = new ImsCallTracker.HandoverInfo();
                response.mType = handover.getType();
                if (handover.hasSrcTech()) {
                    response.mSrcTech = handover.getSrcTech();
                }
                if (handover.hasTargetTech()) {
                    response.mTargetTech = handover.getTargetTech();
                }
                if (handover.hasHoExtra() && handover.getHoExtra() != null) {
                    ImsQmiIF.Extra extra = handover.getHoExtra();
                    if (extra.hasType()) {
                        response.mExtraType = extra.getType();
                    }
                    if (extra.hasExtraInfo()) {
                        response.mExtraInfo = new byte[extra.getExtraInfo().size()];
                        extra.getExtraInfo().copyTo(response.mExtraInfo, 0);
                    }
                }
            } catch (com.google.protobuf.micro.InvalidProtocolBufferMicroException ex) {
                log("InvalidProtocolBufferException in responseHandover parsing");
            }
        }
        return response;
    }

    protected Object handleRadioStateChange(byte[] radioStateChange) {
        int[] response = null;
        if (radioStateChange != null && radioStateChange.length >= 1) {
            try {
                ImsQmiIF.RadioStateChanged state = ImsQmiIF.RadioStateChanged
                        .parseFrom(radioStateChange);
                response = new int[1];
                response[0] = state.getState();
                switch (state.getState()) {
                    case ImsQmiIF.RADIO_STATE_OFF:
                        setRadioState(RadioState.RADIO_OFF);
                        break;
                    case ImsQmiIF.RADIO_STATE_UNAVAILABLE:
                        setRadioState(RadioState.RADIO_UNAVAILABLE);
                        break;
                    case ImsQmiIF.RADIO_STATE_ON:
                        setRadioState(RadioState.RADIO_ON);
                        break;
                    default:
                        Log.e(LOG_TAG, "Invalid state in Radio State Change");
                        break;

                }
            } catch (com.google.protobuf.micro.InvalidProtocolBufferMicroException ex) {
                log("InvalidProtocolBufferException in handleRadioStateChange parsing");
            }
        }
        return response;
    }

    protected Object
    responseSuppServiceNotification(byte[] suppSrvNotification) {
        ImsQmiIF.SuppSvcNotification notification = null;
        if (suppSrvNotification != null && suppSrvNotification.length >= 1 ) {
            try {
                notification =
                        ImsQmiIF.SuppSvcNotification.parseFrom(suppSrvNotification);
            } catch (com.google.protobuf.micro.InvalidProtocolBufferMicroException ex) {
                log("InvalidProtocolBufferException in responseSuppServiceNotification parsing");
            }
        }
        return notification;
    }

    protected Object handleRefreshInfo(byte[] confInfo) {
        ImsQmiIF.ConfInfo info = null;
        if (confInfo != null && confInfo.length >= 1) {
            try {
                info = ImsQmiIF.ConfInfo.parseFrom(confInfo);
            } catch (com.google.protobuf.micro.InvalidProtocolBufferMicroException ex) {
                log("InvalidProtocolBufferException in responseHandover parsing");
            }
        }
        return info;
    }

    private void disableSrvStatus() {
        Log.v(LOG_TAG, "disableSrvStatus");
        if (mSrvStatusRegistrations != null) {
            mSrvStatusRegistrations
                    .notifyRegistrants(new AsyncResult(null, null, new IOException()));
        }
    }

    protected Object handleSrvStatus(byte[] updateList) {
        ArrayList<ImsPhone.ServiceStatus> response = null;
        int num = 0, numAccessTechUpdate = 0;

        if (updateList != null) {
            try {
                ImsQmiIF.SrvStatusList statusList = ImsQmiIF.SrvStatusList.parseFrom(updateList);
                num = statusList.getSrvStatusInfoCount();
                response = new ArrayList<ImsPhone.ServiceStatus>(num);
                ImsPhone.ServiceStatus srvSt;

                for (int i = 0; i < num; i++) {
                    ImsQmiIF.Info info = statusList.getSrvStatusInfo(i);
                    srvSt = new ImsPhone.ServiceStatus();
                    srvSt.isValid = info.getIsValid();
                    srvSt.type = info.getCallType();
                    srvSt.status = info.getStatus();
                    if (info.getUserdata().size() > 0) {
                        srvSt.userdata = new byte[info.getUserdata().size()];
                        info.getUserdata().copyTo(srvSt.userdata, 0);
                    }
                    numAccessTechUpdate = info.getAccTechStatusCount();
                    log("isValid = " + srvSt.isValid + " type = " + srvSt.type + " status = " +
                            srvSt.status + " userdata = " + srvSt.userdata);
                    srvSt.accessTechStatus = new ImsPhone.ServiceStatus.
                            StatusForAccessTech[numAccessTechUpdate];
                    for (int j = 0; j < numAccessTechUpdate && numAccessTechUpdate > 0; j++) {
                        ImsQmiIF.StatusForAccessTech update = info.getAccTechStatus(j);
                        srvSt.accessTechStatus[j] = new ImsPhone.ServiceStatus.
                                StatusForAccessTech();
                        srvSt.accessTechStatus[j].networkMode = update.getNetworkMode();
                        srvSt.accessTechStatus[j].status = update.getStatus();
                        srvSt.accessTechStatus[j].restrictCause = update.getRestrictionCause();
                        if (update.getRegistered() != null) { // Registered is
                                                              // optional field
                            srvSt.accessTechStatus[j].registered = update.getRegistered()
                                    .getState();
                        } else {
                            srvSt.accessTechStatus[j].registered = ImsQmiIF.Registration.
                                    NOT_REGISTERED;
                            Log.e(LOG_TAG, "Registered not sent");
                        }
                        log(" networkMode = " + srvSt.accessTechStatus[j].networkMode +
                                " status = " + srvSt.accessTechStatus[j].status +
                                " restrictCause = " + srvSt.accessTechStatus[j].restrictCause +
                                " registered = " + srvSt.accessTechStatus[j].registered);
                    }
                    response.add(srvSt);
                }
            } catch (com.google.protobuf.micro.InvalidProtocolBufferMicroException ex) {
                log("InvalidProtocolBufferException in handleSrvStatus parsing");
            }
        }
        else {
            response = new ArrayList<ImsPhone.ServiceStatus>(num);
        }
        return response;
    }

    protected Object handleTtyNotify(byte[] notification) {
        int[] mode = null;
        if (notification != null) {
            try {
                ImsQmiIF.TtyNotify notify = ImsQmiIF.TtyNotify.parseFrom(notification);
                mode = new int[1];
                mode[0] = notify.getMode();
                Log.d(LOG_TAG, "handleTtyNotify mode = " + mode[0]);
            } catch (com.google.protobuf.micro.InvalidProtocolBufferMicroException ex) {
                log("InvalidProtocolBufferException in Message Tag parsing ");
            }
        }
        return mode;
    }

    private CallDetails.ServiceStatus[] copySrvStatusList(CallDetails.ServiceStatus[] toList,
            ImsQmiIF.SrvStatusList fromList) {
        if (fromList != null) {
            toList = new CallDetails.ServiceStatus[fromList.getSrvStatusInfoCount()];
            Log.v(LOG_TAG, "Num of SrvUpdates = " + fromList.getSrvStatusInfoCount());
            for (int i = 0; i < fromList.getSrvStatusInfoCount(); i++) {
                ImsQmiIF.Info info = fromList.getSrvStatusInfo(i);
                if (info != null && toList != null) {
                    toList[i] = new CallDetails.ServiceStatus();
                    toList[i].isValid = info.getIsValid();
                    toList[i].type = info.getCallType();
                    toList[i].restrictCause = info.getRestrictCause();
                    if (info.getStatus() == ImsQmiIF.STATUS_ENABLED &&
                            toList[i].restrictCause != CallDetails.CALL_RESTRICT_CAUSE_NONE) {
                        Log.v(LOG_TAG, "Partially Enabled Status due to Restrict Cause");
                        toList[i].status = ImsQmiIF.STATUS_PARTIALLY_ENABLED;
                    } else {
                        toList[i].status = info.getStatus();
                    }
                    if (info.getUserdata().size() > 0) {
                        toList[i].userdata = new byte[info.getUserdata().size()];
                        info.getUserdata().copyTo(toList[i].userdata, 0);
                    }
                } else {
                    Log.e(LOG_TAG, "Null service status in list");
                }
            }
        }
        return toList;
    }

    protected Object responseCallList(byte[] callListB) {
        ArrayList<DriverCallIms> response = null;
        int num = 0;

        if (callListB != null) {
            try {
                ImsQmiIF.CallList callList = ImsQmiIF.CallList.parseFrom(callListB);
                num = callList.getCallAttributesCount();

                int voiceSettings;
                DriverCallIms dc;

                response = new ArrayList<DriverCallIms>(num);

                for (int i = 0; i < num; i++) {
                    ImsQmiIF.CallList.Call call = callList.getCallAttributes(i);
                    dc = new DriverCallIms();

                    dc.state = DriverCallIms.stateFromCLCC(call.getState());
                    dc.index = call.getIndex();
                    dc.TOA = call.getToa();
                    dc.isMpty = call.getIsMpty();
                    dc.isMT = call.getIsMT();
                    dc.als = call.getAls();
                    dc.isVoice = call.getIsVoice();
                    dc.isVoicePrivacy = call.getIsVoicePrivacy();
                    dc.number = call.getNumber();
                    int np = call.getNumberPresentation();
                    dc.numberPresentation = DriverCallIms.presentationFromCLIP(np);
                    dc.name = call.getName();
                    dc.namePresentation = call.getNamePresentation();

                    dc.callDetails = new CallDetails();
                    dc.callDetails.call_type = call.getCallDetails().getCallType();
                    dc.callDetails.call_domain = call.getCallDetails().getCallDomain();
                    List<String> extrasList = call.getCallDetails().getExtrasList();
                    dc.callDetails.extras = extrasList.toArray(new String[extrasList.size()]);
                    dc.callDetails.localAbility = copySrvStatusList(dc.callDetails.localAbility,
                            call.getCallDetails().getLocalAbility());
                    dc.callDetails.peerAbility = copySrvStatusList(dc.callDetails.peerAbility,
                            call.getCallDetails().getPeerAbility());
                    Log.v(LOG_TAG, "Call Details = " + dc.callDetails);
                    // Make sure there's a leading + on addresses with a TOA of
                    // 145
                    dc.number = PhoneNumberUtils.stringFromStringAndTOA(dc.number, dc.TOA);

                    response.add(dc);

                    if (dc.isVoicePrivacy) {
                        mVoicePrivacyOnRegistrants.notifyRegistrants();
                        log("InCall VoicePrivacy is enabled");
                    } else {
                        mVoicePrivacyOffRegistrants.notifyRegistrants();
                        log("InCall VoicePrivacy is disabled");
                    }
                }

                Collections.sort(response);
            } catch (com.google.protobuf.micro.InvalidProtocolBufferMicroException ex) {
                log("InvalidProtocolBufferException in Message Tag parsing ");
            }
        }
        else
            response = new ArrayList<DriverCallIms>(num); //empty array

        return response;
    }

    protected Object responseSuppSvcStatus(byte[] suppSvcStatusInfo) {
        ImsQmiIF.SuppSvcResponse response = null;

        if (suppSvcStatusInfo != null) {
            try {
                response = ImsQmiIF.SuppSvcResponse.parseFrom(suppSvcStatusInfo);
            } catch (com.google.protobuf.micro.InvalidProtocolBufferMicroException ex) {
                log("InvalidProtocolBufferException in message tag parsing");
            }
        } else {
            log("responseSuppSvcStatus suppSvcStatusInfo null");
        }
        return response;
    }

    // UNIMPLEMENTED
    public void getIccCardStatus(Message result) {
        logUnimplemented();
    }

    public void supplyIccPin(String pin, Message result) {
        logUnimplemented();
    }

    public void supplyIccPuk(String puk, String newPin, Message result) {
        logUnimplemented();
    }

    public void supplyIccPin2(String pin, Message result) {
        logUnimplemented();
    }

    public void supplyIccPuk2(String puk, String newPin2, Message result) {
        logUnimplemented();
    }

    public void changeIccPin(String oldPin, String newPin, Message result) {
        logUnimplemented();
    }

    public void changeIccPin2(String oldPin2, String newPin2, Message result) {
        logUnimplemented();
    }

    public void supplyDepersonalization(String netpin, String type, Message result){
        logUnimplemented();
    }

    public void changeBarringPassword(String facility, String oldPwd,
            String newPwd, Message result) {
        logUnimplemented();
    }

    public void supplyDepersonalization(String netpin, int type, Message result) {
        logUnimplemented();
    }

    public void getDataCallList(Message result) {
        logUnimplemented();
    }

    public void dial(String address, int clirMode, Message result) {
        logUnimplemented();
    }

    public void dial(String address, int clirMode, UUSInfo uusInfo,
            Message result) {
        logUnimplemented();
    }

    public void getIMSI(Message result) {
        logUnimplemented();
    }

    public void getIMSIForApp(String aid, Message result) {
        logUnimplemented();
    }

    public void getIMEI(Message result) {
        logUnimplemented();
    }

    public void getIMEISV(Message result) {
        logUnimplemented();
    }

    public void setPreferredVoicePrivacy(boolean enable, Message result) {
        logUnimplemented();
    }

    public void getPreferredVoicePrivacy(Message result) {
        logUnimplemented();
    }

    public void separateConnection(int gsmIndex, Message result) {
        logUnimplemented();
    }

    public void acceptCall(Message result) {
        logUnimplemented();
    }

    public void getLastDataCallFailCause(Message result) {
        logUnimplemented();
    }

    public void setMute(boolean enableMute, Message response) {
        logUnimplemented();
    }

    public void getMute(Message response) {
        logUnimplemented();
    }

    public void getSignalStrength(Message result) {
        logUnimplemented();
    }

    public void getVoiceRegistrationState(Message result) {
        logUnimplemented();
    }

    public void getDataRegistrationState(Message result) {
        logUnimplemented();
    }

    public void getOperator(Message result) {
        logUnimplemented();
    }

    public void sendBurstDtmf(String dtmfString, int on, int off,
            Message result) {
        logUnimplemented();
    }

    public void sendSMS(String smscPDU, String pdu, Message result) {
        logUnimplemented();
    }

    public void sendCdmaSms(byte[] pdu, Message result) {
        logUnimplemented();
    }

    public void sendImsGsmSms(String smscPDU, String pdu,
            int retry, int messageRef, Message response) {
        logUnimplemented();
    }

    public void sendImsCdmaSms(byte[] pdu, int retry, int messageRef,
            Message response) {
        logUnimplemented();
    }

    public void deleteSmsOnSim(int index, Message response) {
        logUnimplemented();
    }

    public void deleteSmsOnRuim(int index, Message response) {
        logUnimplemented();
    }

    public void writeSmsToSim(int status, String smsc, String pdu, Message response) {
        logUnimplemented();
    }

    public void writeSmsToRuim(int status, String pdu, Message response) {
        logUnimplemented();
    }

    public void setupDataCall(String radioTechnology, String profile,
            String apn, String user, String password, String authType,
            String protocol, Message result) {
        logUnimplemented();
    }

    public void setupQosReq(int callId, ArrayList<String> qosFlows, Message result) {
        logUnimplemented();
    }

    public void releaseQos(int qosId, Message result) {
        logUnimplemented();
    }

    public void modifyQos(int qosId, ArrayList<String> qosFlows, Message result) {
        logUnimplemented();
    }

    public void suspendQos(int qosId, Message result) {
        logUnimplemented();
    }

    public void resumeQos(int qosId, Message result) {
        logUnimplemented();
    }

    public void getQosStatus(int qosId, Message result) {
        logUnimplemented();
    }

    public void deactivateDataCall(int cid, int reason, Message result) {
        logUnimplemented();
    }

    public void setRadioPower(boolean on, Message result) {
        logUnimplemented();
    }

    public void setSuppServiceNotifications(boolean enable, Message result) {
        logv("setSuppServiceNotifications enable = " + enable);
        ImsQmiIF.SuppSvcStatus svcStatus = new ImsQmiIF.SuppSvcStatus();
        svcStatus.setStatus(enable ? ImsQmiIF.ENABLED
                : ImsQmiIF.DISABLED);
        byte[] suppServiceNotif = svcStatus.toByteArray();
        encodeMsg(ImsQmiIF.REQUEST_SET_SUPP_SVC_NOTIFICATION, result, suppServiceNotif);
    }

    public void acknowledgeLastIncomingGsmSms(boolean success, int cause,
            Message result) {
        logUnimplemented();
    }

    public void acknowledgeLastIncomingCdmaSms(boolean success, int cause,
            Message result) {
        logUnimplemented();
    }

    public void acknowledgeIncomingGsmSmsWithPdu(boolean success, String ackPdu,
            Message result) {
        logUnimplemented();
    }

    public void iccIO(int command, int fileid, String path, int p1, int p2,
            int p3, String data, String pin2, Message result) {
        logUnimplemented();
    }

    public void iccIOForApp(int command, int fileid, String path, int p1, int p2,
            int p3, String data, String pin2, String aid, Message result) {
        logUnimplemented();
    }

    public void getCLIR(Message result) {
        logv("getCLIR");
        encodeMsg(ImsQmiIF.REQUEST_GET_CLIR, result, null);
    }

    public void setCLIR(int clirMode, Message result) {
        logv("setCLIR clirmode = " + clirMode);
        ImsQmiIF.Clir clirValue = new ImsQmiIF.Clir();
        // clirValue.param_n = clirMode;
        clirValue.setParamN(clirMode);
        byte[] setCLIRInfo = clirValue.toByteArray();
        encodeMsg(ImsQmiIF.REQUEST_SET_CLIR, result, setCLIRInfo);
    }

    public void queryCallWaiting(int serviceClass, Message response) {
        logv("queryCallWaiting serviceClass = " + serviceClass);
        ImsQmiIF.ServiceClass callWaitingQuery = new ImsQmiIF.ServiceClass();
        callWaitingQuery.setServiceClass(serviceClass);
        byte[] callWaitingQueryInfo = callWaitingQuery.toByteArray();
        encodeMsg(ImsQmiIF.REQUEST_QUERY_CALL_WAITING, response,
                callWaitingQueryInfo);
    }

    public void setCallWaiting(boolean enable, int serviceClass,
            Message response) {
        logv("setCallWaiting enable = " + enable + "serviceClass = "
                + serviceClass);
        ImsQmiIF.CallWaitingInfo setCallWaiting = new ImsQmiIF.CallWaitingInfo();
        ImsQmiIF.ServiceClass sc = new ImsQmiIF.ServiceClass();
        sc.setServiceClass(serviceClass);
        setCallWaiting.setServiceStatus(enable ? ImsQmiIF.ENABLED
                : ImsQmiIF.DISABLED);
        setCallWaiting.setServiceClass(sc);
        byte[] callWaitingSetInfo = setCallWaiting.toByteArray();
        encodeMsg(ImsQmiIF.REQUEST_SET_CALL_WAITING, response,
                callWaitingSetInfo);
    }

    public void setNetworkSelectionModeAutomatic(Message response) {
        logUnimplemented();
    }

    public void setNetworkSelectionModeManual(
            String operatorNumeric, Message response) {
        logUnimplemented();
    }

    public void getNetworkSelectionMode(Message response) {
        logUnimplemented();
    }

    public void getAvailableNetworks(Message response) {
        logUnimplemented();
    }

    public void queryIncomingCallBarring(String facility, int serviceClass, Message response) {
        suppSvcStatus(ImsQmiIF.QUERY, facilityStringToInt(facility), null, serviceClass, response);
    }

    public void setIncomingCallBarring(int operation, String facility, String[] icbNum,
            int serviceClass, Message response) {
        suppSvcStatus(operation, facilityStringToInt(facility), icbNum, serviceClass, response);
    }

    public void setCallForward(int action, int cfReason, int serviceClass,
            String number, int timeSeconds, Message response) {
        logv("setCallForward cfReason= " + cfReason + " serviceClass = "
                + serviceClass + "number = " + number + "timeSeconds = "
                + timeSeconds);
        ImsQmiIF.CallForwardInfoList callForwardIF = new ImsQmiIF.CallForwardInfoList();
        ImsQmiIF.CallForwardInfoList.CallForwardInfo callInfo = new ImsQmiIF.CallForwardInfoList.CallForwardInfo();
        callInfo.setStatus(action);
        callInfo.setReason(cfReason);
        callInfo.setServiceClass(serviceClass);
        callInfo.setToa(PhoneNumberUtils.toaFromString(number));
        if (number != null)
            callInfo.setNumber(number);
        callInfo.setTimeSeconds(timeSeconds);
        callForwardIF.addInfo(callInfo);
        byte[] setCallForwardInfo = callForwardIF.toByteArray();
        encodeMsg(ImsQmiIF.REQUEST_SET_CALL_FORWARD_STATUS, response,
                setCallForwardInfo);
    }

    public void queryCallForwardStatus(int cfReason, int serviceClass,
            String number, Message response) {
        logv("queryCallForwardStatus cfReason= " + cfReason
                + " serviceClass = " + serviceClass + "number = " + number);
        ImsQmiIF.CallForwardInfoList callForwardIF = new ImsQmiIF.CallForwardInfoList();
        ImsQmiIF.CallForwardInfoList.CallForwardInfo callInfo = new ImsQmiIF.CallForwardInfoList.CallForwardInfo();
        callInfo.setStatus(2);
        callInfo.setReason(cfReason);
        callInfo.setServiceClass(serviceClass);
        callInfo.setToa(PhoneNumberUtils.toaFromString(number));
        if (number != null)
            callInfo.setNumber(number);
        callInfo.setTimeSeconds(0);
        callForwardIF.addInfo(callInfo);
        byte[] callForwardQuery = callForwardIF.toByteArray();
        encodeMsg(ImsQmiIF.REQUEST_QUERY_CALL_FORWARD_STATUS, response,
                callForwardQuery);
    }

    public void queryCLIP(Message response) {
        logv("queryClip");
        encodeMsg(ImsQmiIF.REQUEST_QUERY_CLIP, response, null);
    }

    public void getBasebandVersion(Message response) {
        logUnimplemented();
    }

    public void sendUSSD(String ussdString, Message response) {
        logUnimplemented();
    }

    public void cancelPendingUssd(Message response) {
        logUnimplemented();
    }

    public void resetRadio(Message result) {
        logUnimplemented();
    }

    public void invokeOemRilRequestRaw(byte[] data, Message response) {
        logUnimplemented();
    }

    public void invokeOemRilRequestStrings(String[] strings, Message response) {
        logUnimplemented();
    }

    public void setBandMode(int bandMode, Message response) {
        logUnimplemented();
    }

    public void queryAvailableBandMode(Message response) {
        logUnimplemented();
    }

    public void sendTerminalResponse(String contents, Message response) {
        logUnimplemented();
    }

    public void sendEnvelope(String contents, Message response) {
        logUnimplemented();
    }

    public void sendEnvelopeWithStatus(String contents, Message response) {
        logUnimplemented();
    }

    public void handleCallSetupRequestFromSim(
            boolean accept, Message response) {
        logUnimplemented();
    }

    public void setPreferredNetworkType(int networkType, Message response) {
        logUnimplemented();
    }

    public void getPreferredNetworkType(Message response) {
        logUnimplemented();
    }

    public void getNeighboringCids(Message response) {
        logUnimplemented();
    }

    public void setLocationUpdates(boolean enable, Message response) {
        logUnimplemented();
    }

    public void getSmscAddress(Message result) {
        logUnimplemented();
    }

    public void setSmscAddress(String address, Message result) {
        logUnimplemented();
    }

    public void reportSmsMemoryStatus(boolean available, Message result) {
        logUnimplemented();
    }

    public void reportStkServiceIsRunning(Message result) {
        logUnimplemented();
    }

    @Override
    public void getCdmaSubscriptionSource(Message response) {
        logUnimplemented();
    }

    public void getGsmBroadcastConfig(Message response) {
        logUnimplemented();
    }

    public void setGsmBroadcastConfig(SmsBroadcastConfigInfo[] config, Message response) {
        logUnimplemented();
    }

    public void setGsmBroadcastActivation(boolean activate, Message response) {
        logUnimplemented();
    }

    // ***** Methods for CDMA support
    public void getDeviceIdentity(Message response) {
        logUnimplemented();
    }

    public void getCDMASubscription(Message response) {
        logUnimplemented();
    }

    public void queryCdmaRoamingPreference(Message response) {
        logUnimplemented();
    }

    public void setCdmaRoamingPreference(int cdmaRoamingType, Message response) {
        logUnimplemented();
    }

    public void setCdmaSubscriptionSource(int cdmaSubscription, Message response) {
        logUnimplemented();
    }

    public void queryTTYMode(Message response) {
        logUnimplemented();
    }

    public void setTTYMode(int ttyMode, Message response) {
        logUnimplemented();
    }

    public void sendCDMAFeatureCode(String FeatureCode, Message response) {
        logUnimplemented();
    }

    public void getCdmaBroadcastConfig(Message response) {
        logUnimplemented();
    }

    public void setCdmaBroadcastConfig(int[] configValuesArray, Message response) {
        logUnimplemented();
    }

    public void setCdmaBroadcastConfig(CdmaSmsBroadcastConfigInfo[] configs, Message response) {
        logUnimplemented();
    }

    public void setCdmaBroadcastActivation(boolean activate, Message response) {
        logUnimplemented();
    }

    public void exitEmergencyCallbackMode(Message response) {
        logv("exitEmergencyCallbackMode");
        encodeMsg(ImsQmiIF.REQUEST_EXIT_EMERGENCY_CALLBACK_MODE, response, null);
    }

    @Override
    public void supplyIccPinForApp(String pin, String aid, Message response) {
        logUnimplemented();
    }

    @Override
    public void supplyIccPukForApp(String puk, String newPin, String aid, Message response) {
        logUnimplemented();
    }

    @Override
    public void supplyIccPin2ForApp(String pin2, String aid, Message response) {
        logUnimplemented();
    }

    @Override
    public void supplyIccPuk2ForApp(String puk2, String newPin2, String aid, Message response) {
        logUnimplemented();
    }

    @Override
    public void changeIccPinForApp(String oldPin, String newPin, String aidPtr, Message response) {
        logUnimplemented();
    }

    @Override
    public void changeIccPin2ForApp(String oldPin2, String newPin2, String aidPtr,
            Message response) {
        logUnimplemented();
    }

    public void requestIsimAuthentication(String nonce, Message response) {
        logUnimplemented();
    }

    public void getVoiceRadioTechnology(Message result) {
        logUnimplemented();
    }

    public void getDataCallProfile(int appType, Message result) {
        logUnimplemented();
    }

    public void setSubscriptionMode(int subscriptionMode, Message response) {
        logUnimplemented();
    }

    public void setUiccSubscription(int slotId, int appIndex, int subId, int subStatus,
            Message response) {
        logUnimplemented();
    }

    @Override
    public void queryFacilityLock(String facility, String password,
            int serviceClass, Message response) {
        suppSvcStatus(ImsQmiIF.QUERY, facilityStringToInt(facility), response);
    }

    @Override
    public void queryFacilityLockForApp(String facility, String password,
            int serviceClass, String appId, Message response) {
        logUnimplemented();
    }

    @Override
    public void setFacilityLock(String facility, boolean lockState,
            String password, int serviceClass, Message response) {
        int operation = lockState ? ImsQmiIF.ACTIVATE : ImsQmiIF.DEACTIVATE;
        suppSvcStatus(operation, facilityStringToInt(facility), response);
    }

    @Override
    public void setFacilityLockForApp(String facility, boolean lockState,
            String password, int serviceClass, String appId, Message response) {
        logUnimplemented();
    }

    public void getSuppSvc(String facility, Message response) {
        suppSvcStatus(ImsQmiIF.QUERY, facilityStringToInt(facility), response);
    }

    public void setSuppSvc(String facility, boolean lockState, Message response) {
        int operation = lockState ? ImsQmiIF.ACTIVATE : ImsQmiIF.DEACTIVATE;
        suppSvcStatus(operation, facilityStringToInt(facility), response);
    }

    public void suppSvcStatus(int operationType, int facility, String[] icbNum,
            int serviceClassValue, Message response) {
        logv("suppSvcStatus operationType = " + operationType + " facility = "
                + facility + "serviceClassValue = " + serviceClassValue);

        ImsQmiIF.SuppSvcRequest supsServiceStatus = new ImsQmiIF.SuppSvcRequest();
        supsServiceStatus.setOperationType(operationType);
        supsServiceStatus.setFacilityType(facility);

        ImsQmiIF.ServiceClass serviceClass = new ImsQmiIF.ServiceClass();
        serviceClass.setServiceClass(serviceClassValue); /* holds service class value
                                                          * i.e 1 for Voice class etc
                                                          */

        ImsQmiIF.CbNumListType cbNumListType = new ImsQmiIF.CbNumListType();
        cbNumListType.setServiceClass(serviceClass);

        if (icbNum != null) {
            for (int i = 0; i <  icbNum.length; i++) {
                logv("icbnum: " + icbNum[i] + "at index: " + i);
                ImsQmiIF.CbNumList cbNumList = new ImsQmiIF.CbNumList();
                cbNumList.setNumber(icbNum[i]);
                cbNumListType.addCbNumList(cbNumList);
            }
        }

        supsServiceStatus.setCbNumListType(cbNumListType);

        byte[] supsService = supsServiceStatus.toByteArray();
        encodeMsg(ImsQmiIF.REQUEST_SUPP_SVC_STATUS, response,
                supsService);
    }

    public void suppSvcStatus(int operationType, int facility, Message response) {
        logv("suppSvcStatus operationType = " + operationType + " facility = "
                + facility);
        ImsQmiIF.SuppSvcRequest supsServiceStatus = new ImsQmiIF.SuppSvcRequest();
        supsServiceStatus.setOperationType(operationType);
        supsServiceStatus.setFacilityType(facility);
        byte[] supsService = supsServiceStatus.toByteArray();
        encodeMsg(ImsQmiIF.REQUEST_SUPP_SVC_STATUS, response,
                supsService);
    }

    public void setDataSubscription(Message response) {
        logUnimplemented();
    }

    static int facilityStringToInt(String sc) {
        if (sc == null) {
            throw new RuntimeException ("invalid supplementary service");
        }

        if (sc.equals(ImsMmiCode.SC_BAOC)) {
            return ImsQmiIF.FACILITY_BAOC;
        } else if (sc.equals(ImsMmiCode.SC_BAOIC)) {
            return ImsQmiIF.FACILITY_BAOIC;
        } else if (sc.equals(ImsMmiCode.SC_BAOICxH)) {
            return ImsQmiIF.FACILITY_BAOICxH;
        } else if (sc.equals(ImsMmiCode.SC_BAIC)) {
            return ImsQmiIF.FACILITY_BAIC;
        } else if (sc.equals(ImsMmiCode.SC_BAICr)) {
            return ImsQmiIF.FACILITY_BAICr;
        } else if (sc.equals(ImsMmiCode.SC_BA_ALL)) {
            return ImsQmiIF.FACILITY_BA_ALL;
        } else if (sc.equals(ImsMmiCode.SC_BA_MO)) {
            return ImsQmiIF.FACILITY_BA_MO;
        } else if (sc.equals(ImsMmiCode.SC_BA_MT)) {
            return ImsQmiIF.FACILITY_BA_MT;
        } else if (sc.equals(ImsMmiCode.SC_BS_MT)) {
            return ImsQmiIF.FACILITY_BS_MT;
        } else if (sc.equals(ImsMmiCode.SC_BAICa)) {
            return ImsQmiIF.FACILITY_BAICa;
        } else if (sc.equals(ImsMmiCode.SC_CLIP)) {
            return ImsQmiIF.FACILITY_CLIP;
        } else if (sc.equals(ImsMmiCode.SC_COLP)) {
            return ImsQmiIF.FACILITY_COLP;
        } else {
            throw new RuntimeException ("invalid supplementary service");
        }
    }

    /**
     * @deprecated
     */
    public void
            getLastPdpFailCause(Message result) {
    }

    @Deprecated
    public void
            getPDPContextList(Message result) {
    }

    @Override
    public void getCellInfoList(Message result) {
        // TODO Auto-generated method stub

    }

    @Override
    public void setCellInfoListRate(int rateInMillis, Message response) {
        // TODO Auto-generated method stub

    }

    @Override
    public void setInitialAttachApn(String apn, String protocol, int authType, String username,
        String password, Message result) {
    }

    // Query for current video call quality.
    public void queryVideoQuality(Message response) {
        Log.d(LOG_TAG, "queryVideoQuality");
        encodeMsg(ImsQmiIF.REQUEST_QUERY_VT_CALL_QUALITY, response, null);
    }

     // Set for current video call quality.
    public void setVideoQuality(int quality,  Message response) {
        Log.d(LOG_TAG, "setVideoQuality quality=" + quality);
        ImsQmiIF.VideoCallQuality msgQuality = new ImsQmiIF.VideoCallQuality();
        msgQuality.setQuality(quality);
        encodeMsg(ImsQmiIF.REQUEST_SET_VT_CALL_QUALITY, response, msgQuality.toByteArray());
    }


}
