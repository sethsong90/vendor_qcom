/******************************************************************************
 * @file    LDSTestApp.java
 * @brief   Test LinkDatagram Socket API Services
 *
 *
 * ---------------------------------------------------------------------------
 *  Copyright (C) 2010-2011 Qualcomm Technologies, Inc.
 *  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
 * ---------------------------------------------------------------------------
 *
 ******************************************************************************/

package com.android.QosTest;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.util.Log;
import android.net.Uri;
import android.util.Xml;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.EditText;
import android.os.RemoteException;
import java.io.File;
import java.io.FileReader;
import java.io.FileNotFoundException;
import java.io.IOException;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

public class LDSTestApp extends Activity implements OnClickListener {
    /**
     * Activity displays "Start LDS application" to users Main Entry point On
     * click() Reads the XML in /data/lds_test.xml and sets corresponding
     * capability on link Takes user to LDSDataActivity
     */

    private static final String TAG = "LDS-TEST-APP";

    // These are the views as set in the XML files //
    private Button mStartLDS;
    private Button mStopLDS;
    private Button mSettings;
    private IntentFilter intentFilter;
    private boolean listenerNotSet = true;
    private ProgressDialog mProgress;
    private String mUpdateMsg;
    private Handler mHandler;
    private MyLinkDatagramSocket mSocket = null;
    private XmlPullParser mConfigParser = null;

    private Runnable mUpdateProgress = new Runnable() {
        public void run() {
            if (mUpdateMsg.equals("DISMISS")) {
                mProgress.cancel();
            } else {
                mProgress.cancel();
                mProgress = ProgressDialog.show(LDSTestApp.this, "", mUpdateMsg);
            }
        }
    };

    private synchronized void setMsg(String msg) {
        mUpdateMsg = msg;
    }

    @Override
    public void onCreate(Bundle savedInstance) {
        super.onCreate(savedInstance);
        setContentView(R.layout.lds_main);
        Intent intent = getIntent();
        setupViews();
    }

    @Override
    public void onResume() {
        super.onResume();

    }

    @Override
    public void onPause() {
        super.onPause();
    }

    private void setupViews() {
        if (listenerNotSet) {
            mHandler = new Handler();
            mStartLDS = (Button) findViewById(R.string.start_lds_btn);
            mStopLDS = (Button) findViewById(R.string.stop_lds_btn);
            mSettings = (Button) findViewById(R.string.settings_lds_btn);
            mStartLDS.setOnClickListener(this);
            mStopLDS.setOnClickListener(this);
            mSettings.setOnClickListener(this);
            listenerNotSet = false;
        }
    }

    public void onClick(View v) {
        if (v.equals(mStartLDS)) {
            // Start Thread to Parse XML and handoff to LDSData Activity
            // Update the status thru. handler
            mProgress = ProgressDialog.show(this, "", "Parsing XML file()", true);
            parseXMLThread();
        } else if (v.equals(mStopLDS)) {
            finish();
        } else if (v.equals(mSettings)) {
            // TODO: Implement settings with configurable file name & link
            // capabilities
            Toast.makeText(this, "Settings not Implemented yet..", Toast.LENGTH_SHORT).show();
        } else {
            Toast.makeText(this, "Not Implemented yet..", Toast.LENGTH_SHORT).show();
        }
    }

    public void parseXMLThread() {
        new Thread(new Runnable() {
            public void run() {
                try {
                    Thread.sleep(1000);
                    setMsg("Starting LDS Test..");
                    mHandler.post(mUpdateProgress);
                    Log.v(TAG, "Starting LDS Test");
                    try {
                        parseXMLConfig("/data/LDSTestApp.xml");
                    } catch (XmlPullParserException xe) {
                        Log.e(TAG, xe.toString());
                        setMsg("Error parsing XML config and creating LinkDatagram socket");
                        Thread.sleep(5000);
                        setMsg("DISMISS");
                        mHandler.post(mUpdateProgress);
                        return;
                    } catch (IOException ie) {
                        Log.e(TAG, ie.toString());
                        setMsg("Error parsing XML config and creating LinkDatagram socket");
                        Thread.sleep(5000);
                        setMsg("DISMISS");
                        mHandler.post(mUpdateProgress);
                        return;
                    }
                    /*
                     * setMsg("Starting LDS Data..");
                     * mHandler.post(mUpdateProgress);
                     * Log.v(TAG,"Starting LDS Data"); mSocket.cmdStartData();
                     * Thread.sleep(10000); setMsg("Stoping LDS Data..");
                     * mHandler.post(mUpdateProgress);
                     * Log.v(TAG,"Stoping LDS Data"); mSocket.cmdStopData();
                     */
                    setMsg("DISMISS");
                    mHandler.post(mUpdateProgress);
                    startActivity(new Intent(LDSTestApp.this, LDSTabUI.class));

                } catch (InterruptedException ie) {
                    Log.e(TAG, "Caught Exception in LDS Test application");
                    ie.printStackTrace();
                } catch (Exception e) {
                    Log.e(TAG, "Caught Exception in LDS Test application");
                    e.printStackTrace();
                }
            }
        }).start();
    }

    private void parseXMLConfig(String configFile) throws XmlPullParserException, IOException {
        try {
            initXMLConfig(configFile);
            String notifier = null;
            String dstnHost = null;
            mSocket = MyLinkDatagramSocket.getInstance();
            mConfigParser.next();
            int eventType = mConfigParser.getEventType();
            while (eventType != XmlPullParser.END_DOCUMENT) {
                if (eventType == XmlPullParser.START_DOCUMENT) {
                    // mSocket = new MyLinkDatagramSocket();
                } else if (eventType == XmlPullParser.START_TAG) {
                    String startTag = mConfigParser.getName();
                    if (startTag.equals("Role")) {
                        eventType = mConfigParser.next();
                        if (eventType == XmlPullParser.TEXT) {
                            if (!mSocket.cmdCreate(mConfigParser.getText(), notifier)) {
                                Log.e(TAG, "Error Creating LinkDatagramSocket");
                                throw new IOException("cmdCreate returned error");
                            }
                        } else {
                            Log.e(TAG, "Error parsing Role element in XML");
                            throw new IOException("Error parsing role element");
                        }
                    } else if (startTag.equals("Notifier")) {
                        eventType = mConfigParser.next();
                        if (eventType == XmlPullParser.TEXT) {
                            notifier = mConfigParser.getText();
                        } else {
                            Log.e(TAG, "Error parsing Notifier element in XML");
                            throw new IOException("Error parsing notifier element in xml");
                        }
                    } else if (startTag.equals("Capabilities")) {
                        int attrCount = mConfigParser.getAttributeCount();
                        Log.v(TAG, "Capability count :" + attrCount);
                        while (attrCount > 0) {
                            Log.v(TAG, "Attr Name : "
                                    + mConfigParser.getAttributeName(attrCount - 1)
                                    + " Attr value : "
                                    + mConfigParser.getAttributeValue(attrCount - 1));
                            mSocket.cmdPutCapabilities(mConfigParser
                                    .getAttributeName(attrCount - 1), mConfigParser
                                    .getAttributeValue(attrCount - 1));
                            --attrCount;
                        }
                    } else if (startTag.equals("DstnHost")) {
                        eventType = mConfigParser.next();
                        if (eventType == XmlPullParser.TEXT) {
                            dstnHost = mConfigParser.getText();
                        } else {
                            Log.e(TAG, "Error parsing dstnHost element in XML");
                            throw new IOException("Error parsing dstnHost element in XML");

                        }
                    } else if (startTag.equals("DstnPort")) {
                        eventType = mConfigParser.next();
                        if (eventType == XmlPullParser.TEXT) {
                            mSocket.cmdConnect(dstnHost, Integer.parseInt(mConfigParser.getText()));
                            Log.v(TAG, "cmdConnect returned");
                        } else {
                            Log.e(TAG, "Error parsing Notifier element in XML");
                            throw new IOException("error parsing notifier element in XML");
                        }
                    }
                } else if (eventType == XmlPullParser.END_TAG) {
                } else if (eventType == XmlPullParser.TEXT) {
                }
                eventType = mConfigParser.next();

            }
        } catch (Exception e) {
            Log.e(TAG, "Caught Exception in LDS Test application");
            e.printStackTrace();
        }
    }

    private void initXMLConfig(String configFile) {
        String xmlFile = configFile;
        File confFile = new File(xmlFile);
        FileReader confreader = null;

        try {
            confreader = new FileReader(confFile);
            mConfigParser = Xml.newPullParser();
            mConfigParser.setInput(confreader);
            Log.i(TAG, configFile + " opened successfully");
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "Exception while parsing '" + confFile.getAbsolutePath() + "'", e);
        } catch (IndexOutOfBoundsException e) {
            Log.e(TAG, "Exception while parsing '" + confFile.getAbsolutePath() + "'", e);
        } catch (FileNotFoundException e) {
            Log.e(TAG, "Exception while parsing '" + confFile.getAbsolutePath() + "'", e);
        } catch (Exception e) {
            Log.e(TAG, "Exception while parsing '" + confFile.getAbsolutePath() + "'", e);
        }
    }
}
