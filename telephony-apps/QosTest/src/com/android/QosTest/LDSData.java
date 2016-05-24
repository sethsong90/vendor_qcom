/******************************************************************************
 * @file    LDSData.java
 * @brief   Tab for  displaying LinkDatagram Socket data transfer
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
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.util.Log;

public class LDSData extends Activity implements OnClickListener {

    private static final String TAG = "LDS-TEST-APP";
    private Button startDataBtn;
    private Button suspendQoSBtn;
    private Button releaseQoSBtn;

    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.lds_data_transfer);
        startDataBtn = (Button) findViewById(R.string.startDataId);
        startDataBtn.setOnClickListener(this);
        suspendQoSBtn = (Button) findViewById(R.string.suspendQoSId);
        suspendQoSBtn.setOnClickListener(this);
        releaseQoSBtn = (Button) findViewById(R.string.releaseQoSId);
        releaseQoSBtn.setOnClickListener(this);
    }

    public void onClick(View v) {

        if (v.equals(startDataBtn)) {
            if (startDataBtn.getText().equals(getString(R.string.startData))) {
                MyLinkDatagramSocket.getInstance().cmdStartData();
                startDataBtn.setText(getString(R.string.stopData));
            } else if (startDataBtn.getText().equals(getString(R.string.stopData))) {
                MyLinkDatagramSocket.getInstance().cmdStopData();
                startDataBtn.setText("Test Completed");
            } else if (startDataBtn.getText().equals("Test Completed")) {
                finish();
            }
        }

        if (v.equals(suspendQoSBtn)) {
            if (suspendQoSBtn.getText().equals(getString(R.string.suspendQoS))) {
                MyLinkDatagramSocket.getInstance().cmdSuspendQoS();
                suspendQoSBtn.setText(getString(R.string.resumeQoS));
            } else if (suspendQoSBtn.getText().equals(getString(R.string.resumeQoS))) {
                MyLinkDatagramSocket.getInstance().cmdResumeQoS();
                suspendQoSBtn.setText(getString(R.string.suspendQoS));
            }
        }

       if (v.equals(releaseQoSBtn)) {
           Log.v(TAG, "Stopping LDS Data");
           MyLinkDatagramSocket.getInstance().close();
       }
    }
}
