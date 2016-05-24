/******************************************************************************
 * @file    QosTest.java
 * @brief   Test QoS Services
 *
 *
 * ---------------------------------------------------------------------------
 *  Copyright (C) 2010 Qualcomm Technologies, Inc.
 *  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
 * ---------------------------------------------------------------------------
 *
 ******************************************************************************/

package com.android.QosTest;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Color;
import android.net.ConnectivityManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemProperties;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import android.text.Editable;
import android.util.Log;
import android.util.Xml;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TableRow.LayoutParams;
import android.widget.TextView;
import com.android.internal.telephony.ITelephony;
import com.android.internal.telephony.ITelephonyRegistry;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.QosSpec;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.util.XmlUtils;
import org.xmlpull.v1.XmlPullParser;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Properties;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class QosTest extends Activity {
    /**
     * Activity that displays to users all information collected regarding a
     * chosen Data Service Type
     */

    private ConnectivityManager mManager;
    private LinkedList<String> mQosIds = new LinkedList<String>();
    private String mCustomQosId;
    private ITelephony mTelephonyService;
    private XmlPullParser mConfParser;
    private HashMap<String, Integer> mOperationMap = new HashMap<String, Integer>();
    private HashMap<String, Integer> mQosRowMap = new HashMap<String, Integer>();
    private IntentFilter mIntentFilter;
    private Context mContext;
    private ITelephonyRegistry telephonyRegistry;
    private static int sRowId = 1;
    private static int sTransId = 0;
    public static Handler sHandler;

    public static class QosOperations {
        public static final int QOS_ENABLE = 1;
        public static final int QOS_DISABLE = 2;
        public static final int QOS_GETQOSSTATUS = 3;
        public static final int QOS_RESUME = 4;
        public static final int QOS_SUSPEND = 5;
    }

    public static class QosInterfaceText {
        public static final String QOS_MESSAGE = "Message";
        public static final String QOS_STATUS = "Status";
    }

    public static final int INVALID_TRANSID = -1;
    private static String TAG = "QOS-TEST";
    private static String XML_FILE = "/data/QosTestConfig.xml";
    private static String QOS_ID_TEXT = "QosId = ";

    /**
     * This BroadcastReceiver updates the views on screen to match the
     * information received from the Intent ANY_DATA_STATE This information
     * include IPversion, apn name, state, interface, and type This is for the
     * most part what users expect to see displayed
     */
    BroadcastReceiver mReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String intentStr = intent.getAction();
            int qosIndState;
            String qosError;
            int transId = 0;
            String qosId;
            QosSpec spec = null;
            TextView Message = (TextView) findViewById(R.id.message);

            Log.w(TAG, "Broadcast received: " + intentStr);

            if (intentStr.equals(TelephonyIntents.ACTION_QOS_STATE_IND)) {
                qosIndState = intent.getIntExtra(QosSpec.QosIntentKeys.QOS_INDICATION_STATE, -1);
                qosError = intent.getStringExtra(QosSpec.QosIntentKeys.QOS_ERROR);
                qosId = Integer.toString(intent.getIntExtra(QosSpec.QosIntentKeys.QOS_ID, 0));
                transId = intent.getIntExtra(QosSpec.QosIntentKeys.QOS_USERDATA, INVALID_TRANSID);
                spec = new QosSpec();
                spec = (QosSpec) intent.getExtras().getParcelable(QosSpec.QosIntentKeys.QOS_SPEC);

                if (qosIndState == QosSpec.QosIndStates.ACTIVATED) {
                    if (!mQosIds.contains(qosId)) {
                        showQosFlow(qosId);
                        mQosIds.addLast(qosId);
                    }
                } else if (qosIndState == QosSpec.QosIndStates.RELEASED_NETWORK
                        || qosIndState == QosSpec.QosIndStates.RELEASED
                        || qosIndState == QosSpec.QosIndStates.RELEASING) {
                    removeQosFlow(qosId);
                }
                Log.d(TAG, "qosIndState:" + qosIndState + " id:" + qosId + " transId:" + transId
                        + " error:" + qosError);
                if (spec != null)
                    Log.d(TAG, "qos spec:" + spec.toString());
                Message.setText("QoS Status:" + qosIndState + " id:" + qosId + " error:"
                                + qosError);
            }
        }
    };

    PhoneStateListener phoneListener = new PhoneStateListener() {
        @Override
        public void onDataConnectionStateChanged(int state) {
            Log.w(TAG, "THE NEW STATE IS " + Integer.toString(state));
        }

        @Override
        public void onDataConnectionStateChanged(int state, int networkType) {
            Log.w(TAG, "NEW STATE: " + Integer.toString(state) + " NETWORK TYPE: "
                    + Integer.toString(networkType));
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.qos_test);

        Intent intent = getIntent();
        mTelephonyService = ITelephony.Stub.asInterface(ServiceManager.getService("phone"));
        telephonyRegistry = ITelephonyRegistry.Stub.asInterface(ServiceManager
                .getService("telephony.registry"));

        /**
         * This Handler updates the text on screen to show information about the
         * packets of data sent through each Qos connection
         */
        sHandler = new Handler() {
            public void handleMessage(Message msg) {
                Bundle results = msg.getData();
                String qosId = results.getString(DataTransfer.QosDataTransfer.QOS_ID);
                String line = results.getString(DataTransfer.QosDataTransfer.QOS_PACKET_DATA);
                if (qosId == null) {
                    updateText(qosId, null, "Data command error.");
                    return;
                }
                switch (msg.what) {
                    case DataTransfer.QosSendStatus.SEND_EXIT:
                        updateText(qosId, null, "Send data stopped.");
                        break;
                    case DataTransfer.QosSendStatus.SEND_FINISH:
                        updateText(qosId, null, "Packet Sent.");
                        break;
                    case DataTransfer.QosSendStatus.SEND_UPDATE:
                        Integer rowId = mQosRowMap.get(qosId);
                        rowId = ((rowId + 4) * 10);
                        if (rowId != 0) {
                            TextView intervalData = (TextView) findViewById((int) (rowId + 1));
                            TextView transferData = (TextView) findViewById((int) (rowId + 2));
                            TextView bandwidthData = (TextView) findViewById((int) (rowId + 3));
                            Pattern pat = Pattern
                                    .compile("[0-9]+[. -]*[0-9]*[. -]* *[0-9]*[. -]*[0-9]* *[[A-Z][a-z][/]]+");
                            String[] data = {
                                    null, null, null
                            };
                            if (line == null) {
                                updateText(qosId, null, "Data command error.");
                                return;
                            }
                            if (line.matches(".*?sec.*?Bytes.*bits/sec$")) {
                                Matcher m = pat.matcher(line);
                                int i = 0;
                                while (m.find()) {
                                    data[i] = m.group();
                                    i++;
                                }
                                if (data[0] == null && intervalData != null) {
                                    intervalData.setText("null");
                                    transferData.setText("null");
                                    bandwidthData.setText("null");
                                    if (msg.what == DataTransfer.QosSendStatus.SEND_UPDATE) {
                                        updateText(qosId, null, "Packets Sending...");
                                    }
                                    return;
                                } else if (intervalData != null) {
                                    intervalData.setText(data[0]);
                                    transferData.setText(data[1]);
                                    bandwidthData.setText(data[2]);
                                    if (data[1].matches("^0.00 [[a-z][A-Z]]+")) {
                                        updateText(qosId, null, "Cannot connect to Server.");
                                    }
                                }
                            }
                            if (msg.what == DataTransfer.QosSendStatus.SEND_UPDATE) {
                                updateText(qosId, null, "Packets Sending...");
                            }
                        }
                        break;
                    default:
                        updateText(qosId, null, "Data command error.");
                        break;
                }
            }
        };

        // Function for Start button
        final Button startButton = (Button) findViewById(R.id.startButton);
        startButton.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                Log.d(TAG, "Enable QoS click");
                int result = enableQos();
                if (result != 0) {
                    updateText(null, QosInterfaceText.QOS_MESSAGE,
                    "Error enabling QoS, result: " + result);
                }
            }
        });

        final Button getQosAll = (Button) findViewById(R.id.getQosAll);
        getQosAll.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                Log.d(TAG, "Get QoS all click");
                int result = getQosAll();
                if (result != 0) {
                    updateText(null, QosInterfaceText.QOS_MESSAGE,
                    "Error getting all QoS specs, result: " + result);
                }
            }
        });
    }

    // Function for Send Data button
    private Button.OnClickListener sendClick = new Button.OnClickListener() {
        public void onClick(View v) {
            Log.d(TAG, "Send Data click");
            int rowId = ((View) v.getParent()).getId();
            TextView qosStatus = (TextView) findViewById((rowId - 1) * 10 + 1);
            String qosId = getQoSId(qosStatus.getText().toString());
            boolean sending = DataTransfer.startDataThread(getApplicationContext(), qosId);
            if (sending) {
                updateText(qosId, null, "Sending data...");
            } else {
                updateText(qosId, null, "Already sending data.");
            }
        }
    };

    // Function for Stop Data button
    private Button.OnClickListener stopClick = new Button.OnClickListener() {
        public void onClick(View v) {
            Log.d(TAG, "Stop Data click");
            int rowId = ((View) v.getParent()).getId();
            TextView qosStatus = (TextView) findViewById((rowId - 1) * 10 + 1);
            String qosId = getQoSId(qosStatus.getText().toString());
            DataTransfer.stopDataThread(qosId);
        }
    };

    // Function for Get Qos button
    private Button.OnClickListener getQosClick = new Button.OnClickListener() {
        public void onClick(View v) {
            Log.d(TAG, "Get QoS click");
            int rowId = ((View) v.getParent()).getId();
            TextView qosStatus = (TextView) findViewById((rowId - 1) * 10 + 1);
            String qosId = getQoSId(qosStatus.getText().toString());
            int result = getQos(qosId);
            if (result != 0) {
                updateText(null, QosInterfaceText.QOS_MESSAGE,
                "Error getting QoS spec, result: " + result);
            }
        }
    };

    // Function for Resume button
    private Button.OnClickListener resumeClick = new Button.OnClickListener() {
        public void onClick(View v) {
            Log.d(TAG, "Resume QoS click");
            int rowId = ((View) v.getParent()).getId();
            TextView qosStatus = (TextView) findViewById((rowId - 2) * 10 + 1);
            String qosId = getQoSId(qosStatus.getText().toString());
            int result = resumeQos(qosId);
            if (result != 0) {
                updateText(null, QosInterfaceText.QOS_MESSAGE,
                "Error resuming QoS, result: " + result);
            }
        }
    };

    // Function for Suspend button
    private Button.OnClickListener suspendClick = new Button.OnClickListener() {
        public void onClick(View v) {
            Log.d(TAG, "Suspend Qos click");
            int rowId = ((View) v.getParent()).getId();
            TextView qosStatus = (TextView) findViewById((rowId - 2) * 10 + 1);
            String qosId = getQoSId(qosStatus.getText().toString());
            int result = suspendQos(qosId);
            if (result != 0) {
                updateText(null, QosInterfaceText.QOS_MESSAGE,
                "Error suspending QoS, result: " + result);
            }
        }
    };

    // Function for Disable button
    private Button.OnClickListener disableClick = new Button.OnClickListener() {
        public void onClick(View v) {
            Log.d(TAG, "Disable QoS click");
            int rowId = ((View) v.getParent()).getId();
            TextView qosStatus = (TextView) findViewById((rowId - 2) * 10 + 1);
            String qosId = getQoSId(qosStatus.getText().toString());
            int result = disableQos(qosId);
            if (result != 0) {
                updateText(null, QosInterfaceText.QOS_MESSAGE,
                "Error disabling QoS, result: " + result);
            }
        }
    };

    // Function that parses the QoS id from the status message
    private String getQoSId(String qosStatus) {
        String qosId = qosStatus.replaceAll(QOS_ID_TEXT, "");
        Log.d(TAG, "qos Status: " + qosStatus + " QoS id: " + qosId);
        return qosId;
    }

    // Creates the UI for each Qos connection
    private void showQosFlow(String qosId) {
        TableLayout packetTable = (TableLayout) findViewById(R.id.packetInfo);
        float d = mContext.getResources().getDisplayMetrics().density;

        LayoutParams rowParams = new LayoutParams(LayoutParams.FILL_PARENT,
                LayoutParams.FILL_PARENT);
        rowParams.setMargins(2, 2, 2, 2);
        LayoutParams buttonParams = new LayoutParams(LayoutParams.FILL_PARENT, (int) (40 * d));
        LayoutParams columnParams = new LayoutParams(LayoutParams.FILL_PARENT,
                LayoutParams.FILL_PARENT, (float) 0.3);
        LayoutParams intervalParams = new LayoutParams((int) (93 * d), LayoutParams.FILL_PARENT);
        intervalParams.setMargins(2, 2, 2, 2);
        LayoutParams transferParams = new LayoutParams((int) (103 * d), LayoutParams.FILL_PARENT);
        transferParams.setMargins(2, 2, 2, 2);
        LayoutParams bandwidthParams = new LayoutParams((int) (113 * d), LayoutParams.FILL_PARENT);
        bandwidthParams.setMargins(2, 2, 2, 2);

        // Qos UI row with the status
        TableRow newPacketStatus = new TableRow(QosTest.this);
        newPacketStatus.setId(sRowId);
        mQosRowMap.put(qosId, sRowId);

        TextView qosStatus = new TextView(QosTest.this);
        qosStatus.setText(QOS_ID_TEXT + qosId);
        qosStatus.setId((sRowId * 10) + 1);
        Log.d("TEST", "row = " + (sRowId * 10 + 1));
        qosStatus.setBackgroundColor(Color.BLACK);
        newPacketStatus.addView(qosStatus, rowParams);
        sRowId++;

        // Qos UI row with the data buttons
        TableRow newPacketDataButton = new TableRow(QosTest.this);
        newPacketDataButton.setId(sRowId);

        Button getQos = new Button(QosTest.this);
        getQos.setText(R.string.get_qos);
        getQos.setOnClickListener(getQosClick);
        newPacketDataButton.addView(getQos);

        Button send = new Button(QosTest.this);
        send.setText(R.string.send_data);
        send.setOnClickListener(sendClick);
        newPacketDataButton.addView(send);

        Button stop = new Button(QosTest.this);
        stop.setText(R.string.stop_data);
        stop.setOnClickListener(stopClick);
        newPacketDataButton.addView(stop);
        sRowId++;

        // Qos UI row with the buttons
        TableRow newPacketButton = new TableRow(QosTest.this);
        newPacketButton.setId(sRowId);

        Button resume = new Button(QosTest.this);
        resume.setText(R.string.resume_qos);
        resume.setOnClickListener(resumeClick);
        newPacketButton.addView(resume, buttonParams);

        Button suspend = new Button(QosTest.this);
        suspend.setText(R.string.suspend_qos);
        suspend.setOnClickListener(suspendClick);
        newPacketButton.addView(suspend, buttonParams);

        Button disable = new Button(QosTest.this);
        disable.setText(R.string.disable);
        disable.setOnClickListener(disableClick);
        newPacketButton.addView(disable, buttonParams);
        sRowId++;

        // Qos UI row with the headings for data transfer info
        TableRow newPacketTitle = new TableRow(QosTest.this);
        newPacketTitle.setId(sRowId);
        sRowId++;
        TextView interval = new TextView(QosTest.this);
        interval.setText(R.string.interval);
        newPacketTitle.addView(interval, rowParams);

        TextView transfer = new TextView(QosTest.this);
        transfer.setText(R.string.transfer);
        newPacketTitle.addView(transfer, rowParams);

        TextView bandwidth = new TextView(QosTest.this);
        bandwidth.setText(R.string.bandwidth);
        newPacketTitle.addView(bandwidth, rowParams);

        // Qos UI row with the data for data transfer info
        TableRow newPacketData = new TableRow(QosTest.this);
        newPacketData.setId(sRowId);
        TextView intervalData = new TextView(QosTest.this);
        intervalData.setText(R.string.intervalData);
        intervalData.setBackgroundColor(Color.BLACK);
        intervalData.setId((sRowId * 10) + 1);
        newPacketData.addView(intervalData, intervalParams);

        TextView transferData = new TextView(QosTest.this);
        transferData.setText(R.string.transferData);
        transferData.setBackgroundColor(Color.BLACK);
        transferData.setId((sRowId * 10) + 2);
        newPacketData.addView(transferData, transferParams);

        TextView bandwidthData = new TextView(QosTest.this);
        bandwidthData.setText(R.string.bandwidthData);
        bandwidthData.setBackgroundColor(Color.BLACK);
        bandwidthData.setId((sRowId * 10) + 3);
        newPacketData.addView(bandwidthData, bandwidthParams);
        sRowId++;

        View ruler = new View(QosTest.this);
        ruler.setBackgroundColor(Color.BLACK);
        ruler.setId(sRowId);
        sRowId++;

        // Add rows to TableLayout.
        packetTable.addView(ruler, new ViewGroup.LayoutParams(LayoutParams.FILL_PARENT, 2));
        packetTable.addView(newPacketStatus, columnParams);
        packetTable.addView(newPacketDataButton, columnParams);
        packetTable.addView(newPacketButton, columnParams);
        packetTable.addView(newPacketTitle, columnParams);
        packetTable.addView(newPacketData, columnParams);

    }

    // Removes UI for Qos Service
    private void removeQosFlow(String qosId) {
        Integer ID = mQosRowMap.get(qosId);
        if (ID == null)
            return;
        TableLayout packetTable = (TableLayout) findViewById(R.id.packetInfo);
        packetTable.removeView(findViewById(ID));
        packetTable.removeView(findViewById(ID + 1));
        packetTable.removeView(findViewById(ID + 2));
        packetTable.removeView(findViewById(ID + 3));
        packetTable.removeView(findViewById(ID + 4));
        packetTable.removeView(findViewById(ID + 5));
        mQosRowMap.remove(qosId);
        updateText(null, QosInterfaceText.QOS_MESSAGE, QOS_ID_TEXT + qosId + " disabled.");
    }

    // Confirms text is not null and changes text in UI
    private void updateText(String qosId, String textView, String text) {
        if (qosId == null) {
            TextView view = null;
            if (textView == QosInterfaceText.QOS_STATUS) {
                view = (TextView) findViewById(R.id.status);
            } else if (textView == QosInterfaceText.QOS_MESSAGE) {
                view = (TextView) findViewById(R.id.message);
            } else {
                return;
            }

            if (text != null) {
                view.setText(text);
            }
        } else {
            TextView qosStatus = (TextView) findViewById(mQosRowMap.get(qosId) * 10 + 1);
            qosStatus.setText(QOS_ID_TEXT + qosId + "\n" + text);
        }
    }

    // Initializes the XML file needed to be read for parameters
    private boolean initXmlConfig() {

        mOperationMap.put("enable", QosOperations.QOS_ENABLE);
        mOperationMap.put("disable", QosOperations.QOS_DISABLE);
        mOperationMap.put("getqosstatus", QosOperations.QOS_GETQOSSTATUS);
        mOperationMap.put("resume", QosOperations.QOS_RESUME);
        mOperationMap.put("suspend", QosOperations.QOS_SUSPEND);

        mConfParser = null;

        File confFile = new File(XML_FILE);
        FileReader confreader = null;
        boolean defaultFile = false;
        try {
            confreader = new FileReader(confFile);
            mConfParser = Xml.newPullParser();
            mConfParser.setInput(confreader);
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "Exception while parsing '" + confFile.getAbsolutePath() + "'", e);
            defaultFile = true;
        } catch (IndexOutOfBoundsException e) {
            Log.e(TAG, "Exception while parsing '" + confFile.getAbsolutePath() + "'", e);
            defaultFile = true;
        } catch (FileNotFoundException e) {
            Log.e(TAG, "Exception while parsing '" + confFile.getAbsolutePath() + "'", e);
            defaultFile = true;
        } catch (Exception e) {
            Log.e(TAG, "Exception while parsing '" + confFile.getAbsolutePath() + "'", e);
            defaultFile = true;
        }
        if (defaultFile) {
            try {
                mConfParser = Xml.newPullParser();
                InputStream myFile = this.getAssets().open("QosTestConfig.xml");
                mConfParser.setInput(myFile, null);
            } catch (Exception err) {
                Log.e(TAG, "Exception " + err);
                return false;
            }
        }
        return true;
    }

    // Extracts the parameters from the XML file and starts the Qos Service
    private ArrayList<QosSpec> getDefaultQosSpec() {
        ArrayList<QosSpec> qosSpecs = new ArrayList<QosSpec>();
        QosSpec qosSpec = null;
        boolean success = initXmlConfig();
        if (!success)
            return qosSpecs;
        boolean version2 = false;
        boolean done = false;
        try {
            XmlUtils.beginDocument(mConfParser, "QosTestConfig");
            if (mConfParser.getAttributeCount() != 0) {
                if (mConfParser.getAttributeName(0).equals("version")
                        && mConfParser.getAttributeValue(0).equals("2.0")) {
                    version2 = true;
                }
            }
            while (mConfParser.getEventType() != XmlPullParser.END_DOCUMENT
                    && !mConfParser.getName().equals(null)) {
                // Log.d(TAG, "ky:" + mConfParser.getName());
                if (mConfParser.getName().equals("QosDataTransfer")) {
                    XmlUtils.nextElement(mConfParser);
                    String defaultCmd = null, defaultOptions = null;
                    while (mConfParser.getDepth() == 3) {
                        if (mConfParser.getName().equals("DefaultCmd")) {
                            Log.d(TAG, "DEFAULT CMD = " + mConfParser.getAttributeValue(0));
                            defaultCmd = mConfParser.getAttributeValue(0);
                        } else if (mConfParser.getName().equals("DefaultOptions")) {
                            Log.d(TAG, "DEFAULT OPTIONS = " + mConfParser.getAttributeValue(0));
                            defaultOptions = mConfParser.getAttributeValue(0);
                        }
                        XmlUtils.nextElement(mConfParser); // QosOp
                    }
                    DataTransfer.setCmd(defaultCmd, defaultOptions);
                    if (mConfParser.getEventType() == XmlPullParser.END_DOCUMENT) {
                        break;
                    }
                } else if (mConfParser.getName().equals("QosTest")) {
                    XmlUtils.nextElement(mConfParser); // QosOp
                    if (mConfParser.getName().equals("QosOp")) {
                        if (mConfParser.getAttributeName(0).equals("operation")) {
                            Integer operation = mOperationMap.get(mConfParser.getAttributeValue(0));
                            if (operation == null) {
                                Log.d(TAG, "Operation is not in hash map");
                                continue;
                            }
                            XmlUtils.nextElement(mConfParser); // QosSpec
                            if (mConfParser.getEventType() != XmlPullParser.END_DOCUMENT
                                    && mConfParser.getName().equals("QosSpec")) {
                                while (mConfParser.getName().equals("QosSpec")) {
                                    Log.d(TAG, "ky:" + mConfParser.getName() + " value:"
                                            + mConfParser.getEventType());
                                    qosSpec = new QosSpec();
                                    XmlUtils.nextElement(mConfParser);
                                    while (mConfParser.getName().equals("QosFlowFilter")) {
                                        if (version2) {
                                            Log.d(TAG, "QosFlowFilter");
                                            QosSpec.QosPipe pipe = qosSpec.createPipe();
                                            XmlUtils.nextElement(mConfParser);
                                            while (mConfParser.getDepth() == 5) {
                                                String key = mConfParser.getName();
                                                String value = mConfParser.getAttributeValue(0);
                                                Log.d("PARAMETERS", "ky:" + key + " value:"
                                                                + value);
                                                pipe.put(QosSpec.QosSpecKey.getKey(key), value);
                                                XmlUtils.nextElement(mConfParser);
                                            }
                                        } else {
                                            int nAttr2 = mConfParser.getAttributeCount();
                                            QosSpec.QosPipe pipe = qosSpec.createPipe();
                                            Log.d(TAG, "QosFlowFilter");
                                            for (int k = 0; k < nAttr2; k++) {
                                                String key = mConfParser.getAttributeName(k);
                                                String value = mConfParser.getAttributeValue(k);
                                                Log.d("PARAMETERS", "ky:" + key + " value:"
                                                                + value);
                                                pipe.put(QosSpec.QosSpecKey.getKey(key), value);
                                            }
                                            XmlUtils.nextElement(mConfParser);
                                        }
                                    }
                                    qosSpecs.add(qosSpec);
                                    Log.d(TAG, "QosSpec Added");
                                }
                            } else {
                                done = true;
                            }

                            switch (operation) {
                                case QosOperations.QOS_ENABLE:
                                    Log.d(TAG, "operation = enable");
                                    break;
                                case QosOperations.QOS_DISABLE:
                                    Log.d(TAG, "operation = disable");
                                    break;
                                case QosOperations.QOS_GETQOSSTATUS:
                                    Log.d(TAG, "operation = getQosStatus");
                                    break;
                                case QosOperations.QOS_RESUME:
                                    Log.d(TAG, "operation = resume");
                                    break;
                                case QosOperations.QOS_SUSPEND:
                                    Log.d(TAG, "operation = suspend");
                                    break;
                                default:
                                    break;
                            }
                            if (done) continue;
                        }
                    }
                }
                XmlUtils.nextElement(mConfParser);
            }
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "Exception while parsing: " + e.toString());
            return null;
        } catch (IndexOutOfBoundsException e) {
            Log.e(TAG, "Exception while parsing: " + e.toString());
            return null;
        } catch (Exception e) {
            Log.e(TAG, "Exception while parsing: " + e.toString());
            return null;
        }
        Log.d(TAG, "Done parsing xml file");
        Log.d(TAG, "QosSpec:" + qosSpec.toString());
        return qosSpecs;
    }

    @Override
    public void onResume() {
        super.onResume();
        mContext = getBaseContext();
        mIntentFilter = new IntentFilter();
        mIntentFilter.addAction("android.intent.action.ANY_DATA_STATE");
        mIntentFilter.addAction(TelephonyIntents.ACTION_QOS_STATE_IND);
        mContext.registerReceiver(mReceiver, mIntentFilter);
    }

    @Override
    public void onPause() {
        super.onPause();
        unregisterReceiver(mReceiver);
    }

    /**
     *
     * @return
     */
    private String getApnTypeForQos() {
        String apnType = SystemProperties.get("persist.telephony.qos.apntype", "default");

        // Ensure the APN Type is valid
        if (!(Phone.APN_TYPE_MMS.equals(apnType) || Phone.APN_TYPE_SUPL.equals(apnType)
                || Phone.APN_TYPE_DUN.equals(apnType) || Phone.APN_TYPE_HIPRI.equals(apnType)))
            apnType = Phone.APN_TYPE_DEFAULT;

        return apnType;
    }

    /**
     *
     * @return
     */
    private int enableQos() {
        int result = Phone.QOS_REQUEST_FAILURE;

        ArrayList<QosSpec> qosSpecs = getDefaultQosSpec();
        if ((qosSpecs == null) || (qosSpecs.size() == 0)) {
            updateText(null, QosInterfaceText.QOS_MESSAGE, "Error enabling QoS as spec is null");
            return result;
        }

        Log.d(TAG, "# of QosSpecs:" + qosSpecs.size());
        try {
            for (QosSpec qosSpec : qosSpecs.toArray(new QosSpec[0])) {
                sTransId++;
                qosSpec.setUserData(sTransId);
                String apnType = getApnTypeForQos();

                result = mTelephonyService.enableQos(qosSpec, apnType);
                Log.d(TAG, "enableQos mTransId:" + sTransId + " APN Type:" + apnType
                        + " result:" + result);
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Exception in using Telephony Service: " + e.toString());
        } catch (Exception e) {
            Log.e(TAG, "Exception enabling QoS: " + e.toString());
            e.printStackTrace();
        }

        return result;
    }

    /**
     *
     * @param qosId
     * @return
     */
    private int disableQos(String qosId) {
        int result = Phone.QOS_REQUEST_FAILURE;

        if (mQosIds.size() == 0)
            return result;
        Log.d(TAG, "# of QosIds:" + mQosIds.size());

        try {
            mQosIds.remove(qosId);
            result = mTelephonyService.disableQos(Integer.parseInt(qosId));
            Log.d(TAG, "disableQos():qosId:" + qosId + " result:" + result);
        } catch (NoSuchElementException e) {
            Log.e(TAG, "Exception!" + e);
        } catch (RemoteException e) {
            Log.e(TAG, "Exception in using Telephony Service!" + e);
        }

        return result;
    }

    /**
     *
     * @param qosId
     * @return
     */
    private int getQos(String qosId) {
        int result = Phone.QOS_REQUEST_FAILURE;

        if (mQosIds.size() == 0)
            return result;
        Log.d(TAG, "# of QosIds:" + mQosIds.size());

        try {
            result = mTelephonyService.getQosStatus(Integer.parseInt(qosId));
            Log.d(TAG, "getQos() result:" + result);
        } catch (RemoteException e) {
            Log.e(TAG, "Exception in using Telephony Service!");
        }

        return result;
    }

    /**
     *
     * @return
     */
    private int getQosAll() {
        int result = Phone.QOS_REQUEST_FAILURE;

        if (mQosIds.size() == 0)
            return result;
        Log.d(TAG, "# of QosIds:" + mQosIds.size());

        try {
            for (String qosId : mQosIds.toArray(new String[0])) {
                result = mTelephonyService.getQosStatus(Integer.parseInt(qosId));
                Log.d(TAG, "getQos() result:" + result);
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Exception in using Telephony Service!");
        }

        return result;
    }

    /**
     *
     * @param qosId
     * @return
     */
    private int suspendQos(String qosId) {
        int result = Phone.QOS_REQUEST_FAILURE;

        if (mQosIds.size() == 0)
            return result;
        Log.d(TAG, "# of QosIds:" + mQosIds.size());

        try {
            result = mTelephonyService.suspendQos(Integer.parseInt(qosId));
            Log.d(TAG, "suspendQos() result:" + result);
        } catch (RemoteException e) {
            Log.e(TAG, "Exception in using Telephony Service!");
        }

        return result;
    }

    /**
     *
     * @param qosId
     * @return
     */
    private int resumeQos(String qosId) {
        int result = Phone.QOS_REQUEST_FAILURE;

        if (mQosIds.size() == 0)
            return result;
        Log.d(TAG, "# of QosIds:" + mQosIds.size());

        try {
            result = mTelephonyService.resumeQos(Integer.parseInt(qosId));
            Log.d(TAG, "resumeQos() result:" + result);
        } catch (RemoteException e) {
            Log.e(TAG, "Exception in using Telephony Service!");
        }

        return result;
    }

    /**
     *
     * @param op
     * @return
     */
    private String getDefaultTestQosId(String op) {
        String qosIdStr = new String();
        qosIdStr = null;

        initXmlConfig();

        try {

            // Start from the begining
            XmlUtils.beginDocument(mConfParser, "QosTestConfig");

            boolean found = false;
            // Skip the elements till the one we need
            while (mConfParser.getName() != null && !found) {
                XmlUtils.nextElement(mConfParser);
                if (mConfParser.getName().equals("QosOp"))
                    found = mConfParser.getAttributeValue("", "operation").equals(op);
                Log.e(TAG, "Name:" + mConfParser.getName());
            }

            // If it has been found, extract the value
            if (found) {
                qosIdStr = mConfParser.getAttributeValue("", "qosId");
                Log.d(TAG, "qosId value:" + qosIdStr);
            }
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "Exception while parsing '", e);
        } catch (IndexOutOfBoundsException e) {
            Log.e(TAG, "Exception while parsing '", e);
        } catch (Exception e) {
            Log.e(TAG, "Exception while parsing '", e);
        }

        Log.d(TAG, "QosId from Config:" + qosIdStr);
        return qosIdStr;
    }
}
