/******************************************************************************
 * @file    MyLinkDatagramSocket.java
 * @brief   Special class for accessing and publishing LDS
 *
 *
 * ---------------------------------------------------------------------------
 *  Copyright (C) 2010-2011 Qualcomm Technologies, Inc.
 *  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
 * ---------------------------------------------------------------------------
 *
 ******************************************************************************/

package com.android.QosTest;

import android.util.Log;
import android.net.LinkSocket;
import android.net.LinkDatagramSocket;
import android.net.LinkCapabilities;
import android.net.LinkSocketNotifier;
import java.net.DatagramPacket;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.io.IOException;

public class MyLinkDatagramSocket implements LinkSocketNotifier {
    /**
     * This class is used to maintain the state of LDS internally. It has
     * LinkCapabilites.
     */

    private String TAG = "LDS-TEST-APP";
    private LinkDatagramSocket mSocket;
    private LinkCapabilities mCapabilities;
    private String mRole;
    private String mNotifier;
    private String mDstnAddr;
    private int mDstnPort;
    private DatagramPacket mPkt;
    private Boolean mStopData;
    private static MyLinkDatagramSocket instance = null;

    protected MyLinkDatagramSocket() {
        dumpInfo("In Constructor");
        mSocket = null;
        mCapabilities = null;
        mRole = null;
        mNotifier = null;
        mDstnAddr = null;
        mPkt = null;
        mStopData = false;
    }

    public static MyLinkDatagramSocket getInstance() {
        if (instance == null) {
            instance = new MyLinkDatagramSocket();
        }
        return instance;

    }

    public Boolean cmdCreate(String role, String notifier) {
        dumpInfo("cmdCreate() EX, role " + role);
        if (role.equals(LinkCapabilities.Role.QOS_VIDEO_TELEPHONY)) {
            mRole = LinkCapabilities.Role.QOS_VIDEO_TELEPHONY;
        } else  if (role.equals(LinkCapabilities.Role.QOS_CUSTOM)) {
            mRole = LinkCapabilities.Role.QOS_CUSTOM;
        }
        else {
            dumpError("Unknown Role value to the test application");
        }
        mNotifier = notifier;
        if (mRole == null) {
            return false;
        }
        else {
            try {
                if (mNotifier.equals("enabled")) {
                    mSocket = new LinkDatagramSocket(this);
                    mCapabilities = LinkCapabilities.createNeedsMap(mRole);
                    return true;
                } else if (mNotifier.equals("disabled")) {
                    mSocket = new LinkDatagramSocket();
                    mCapabilities = LinkCapabilities.createNeedsMap(mRole);
                    return true;
                } else {
                    dumpError("Invalid value for notifier");
                    return false;
                }
            } catch (SocketException se) {
                dumpError("LinkDatagramSocket threw SocketException, " + se.toString());
                return false;
            }
        }

    }

    public Boolean cmdPutCapabilities(String key, String value) {
        dumpInfo("cmdPutCapabilities() EX : Key = " + key + " and value = " + value);
        if (mCapabilities == null) {
            dumpError("Capabilities object was not created inside test application");
            return false;
        } else {
            if (key.equals("RW_DESIRED_FWD_BW")) {
                mCapabilities.put(LinkCapabilities.Key.RW_DESIRED_FWD_BW, value);
            }
            else if (key.equals("RW_REQUIRED_FWD_BW")) {
                mCapabilities.put(LinkCapabilities.Key.RW_REQUIRED_FWD_BW, value);
            }
            else if (key.equals("RW_DESIRED_REV_BW")) {
                mCapabilities.put(LinkCapabilities.Key.RW_DESIRED_REV_BW, value);
            }
            else if (key.equals("RW_REQUIRED_REV_BW")) {
                mCapabilities.put(LinkCapabilities.Key.RW_REQUIRED_REV_BW, value);
            }
            else if (key.equals("RW_MAX_ALLOWED_FWD_LATENCY")) {
                mCapabilities.put(LinkCapabilities.Key.RW_MAX_ALLOWED_FWD_LATENCY, value);
            }
            else if (key.equals("RW_MAX_ALLOWED_REV_LATENCY")) {
                mCapabilities.put(LinkCapabilities.Key.RW_MAX_ALLOWED_REV_LATENCY, value);
            }
            else if (key.equals("RW_DISABLE_NOTIFICATIONS")) {
                mCapabilities.put(LinkCapabilities.Key.RW_DISABLE_NOTIFICATIONS, value);
            }
            else if (key.equals("RW_REMOTE_DEST_IP_ADDRESSES")) {
                mCapabilities.put(LinkCapabilities.Key.RW_REMOTE_DEST_IP_ADDRESSES, value);
            }
            else if (key.equals("RW_REMOTE_SRC_IP_ADDRESSES")) {
                mCapabilities.put(LinkCapabilities.Key.RW_REMOTE_SRC_IP_ADDRESSES, value);
            }
            else if (key.equals("RW_REMOTE_SRC_PORTS")) {
                mCapabilities.put(LinkCapabilities.Key.RW_REMOTE_SRC_PORTS, value);
            }
            else if (key.equals("RW_REMOTE_DEST_PORTS")) {
                mCapabilities.put(LinkCapabilities.Key.RW_REMOTE_DEST_PORTS, value);
            }
            else if (key.equals("RW_FILTERSPEC_REV_IP_TOS")) {
                mCapabilities.put(LinkCapabilities.Key.RW_FILTERSPEC_REV_IP_TOS, value);
            }
            else if (key.equals("RW_FILTERSPEC_FWD_IP_TOS")) {
                mCapabilities.put(LinkCapabilities.Key.RW_FILTERSPEC_FWD_IP_TOS, value);
            }
            else if (key.equals("RW_NETWORK_TYPE")) {
                mCapabilities.put(LinkCapabilities.Key.RW_NETWORK_TYPE, value);
            }
            else if (key.equals("RW_REV_TRAFFIC_CLASS")) {
                mCapabilities.put(LinkCapabilities.Key.RW_REV_TRAFFIC_CLASS, value);
            }
            else if (key.equals("RW_FWD_TRAFFIC_CLASS")) {
                mCapabilities.put(LinkCapabilities.Key.RW_FWD_TRAFFIC_CLASS, value);
            }
            else if (key.equals("RW_FWD_3GPP2_PRIORITY")) {
                mCapabilities.put(LinkCapabilities.Key.RW_FWD_3GPP2_PRIORITY, value);
            }
            else if (key.equals("RW_REV_3GPP2_PRIORITY")) {
                mCapabilities.put(LinkCapabilities.Key.RW_REV_3GPP2_PRIORITY, value);
            }
            else if (key.equals("RW_REV_3GPP2_PROFILE_ID")) {
                mCapabilities.put(LinkCapabilities.Key.RW_REV_3GPP2_PROFILE_ID, value);
            }
            else if (key.equals("RW_FWD_3GPP2_PROFILE_ID")) {
                mCapabilities.put(LinkCapabilities.Key.RW_FWD_3GPP2_PROFILE_ID, value);
            }
            else {
                dumpError("Invalid Key, Value pair. Possible mismatch betwen LDS Test application and LDS code or use error in xml");
                return false;
            }
        }
        return true;
    }

    public Boolean cmdConnect(String dstnAddr, int port) {
        dumpInfo("cmdConnect() EX");
        if (mSocket == null) {
            dumpError("Link Datagram socket is null in test application");
            return false;
        } else {
            mSocket.setNeededCapabilities(mCapabilities);
            try {
                mSocket.connect(dstnAddr, port);
            } catch (UnknownHostException ue) {
                dumpError("Received Unknown Host exception on Connect(), " + ue.toString());
                return false;
            } catch (IOException ie) {
                dumpError("Received IO exception on Connect(), " + ie.toString());
                return false;
            }
        }
        return true;
    }

    public Boolean cmdStartData() {
        dumpInfo("cmdStartData() EX");
        if (mSocket == null) {
            dumpError("LinkDatagramSocket is null in test application");
            return false;
        } else {
            mPkt = new DatagramPacket(TAG.getBytes(), (TAG.getBytes()).length);
            new Thread(new Runnable() {
                public void run() {
                    while (!mStopData) {
                        try {
                            mSocket.send(mPkt);
                            Thread.sleep(100);
                        } catch (InterruptedException ie) {
                            dumpError("Recived Interrupted exception on send(), " + ie.toString());
                        } catch (Exception ie) {
                            dumpError("Recieved Exception, " + ie.toString());
                            mStopData = true;
                        }
                    }
                    mSocket.close();
                }
            }).start();
            return true;
        }
    }

    public void cmdStopData() {
        dumpInfo("cmdStopData() EX");
        mStopData = true;
    }

    public void cmdSuspendQoS() {
        dumpInfo("cmdSuspendQoS()");
        mSocket.suspendQoS();
    }

    public void cmdResumeQoS() {
        dumpInfo("cmdResumeQoS()");
        mSocket.resumeQoS();
    }

    public void close() {
        if (mSocket != null) {
            mSocket.close();
        }
    }

    private void dumpInfo(String s) {
        Log.i(TAG, s);
    }

    private void dumpError(String s) {
        Log.e(TAG, s);
    }

    private void dumpVerbose(String s) {
        Log.v(TAG, s);
    }

    public void onBetterLinkAvailable(LinkSocket mOrigSocket) {
        dumpInfo("onBetterLinkAvailable() called on LinkSocket");
    }

    public void onBetterLinkAvailable(LinkDatagramSocket mOrigSocket) {
        dumpInfo("onBetterLinkAvailable() called on LinkDatagramSocket");
    }

    public void onLinkLost(LinkSocket mOrigSocket) {
        dumpInfo("onLinkLost() called on LinkSocket");
    }

    public void onLinkLost(LinkDatagramSocket mOrigSocket) {
        dumpInfo("onLinkLost() called on LinkDatagramSocket");
    }

    public void onCapabilitiesChanged(LinkSocket mOrigSocket, LinkCapabilities changedCapabilities) {
        dumpInfo("onCapabilitiesChanged() called on LinkSocket");
    }

    public void onCapabilitiesChanged(LinkDatagramSocket mOrigSocket,
            LinkCapabilities changedCapabilities) {
        dumpInfo("onCapabilitiesChanged() called on LinkDatagramSocket");
        dumpInfo("QOS_STATE is " + changedCapabilities.get(LinkCapabilities.Key.RO_QOS_STATE));
    }
  
    public void onNewLinkUnavailable(LinkSocket mOrigSocket) {
        dumpInfo("onNewLinkUnavailable() called on LinkSocket");
    }
}
