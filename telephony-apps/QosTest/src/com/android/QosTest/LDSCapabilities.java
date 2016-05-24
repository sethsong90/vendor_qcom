/******************************************************************************
 * @file    LDSCapabilities.java
 * @brief   Tab for  displaying LinkDatagram Socket data transfer
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
import android.content.Intent;
import android.os.Bundle;
import android.widget.Toast;
import android.widget.Button;
import android.view.View;
import android.view.View.OnClickListener;

public class LDSCapabilities extends Activity implements OnClickListener {
    private static final String TAG = "LDS-TEST-APP";
    private Button getCapabilitiesBtn;
    private Button getTrackedCapabilitiesBtn;
    private Button getNeededCapabilitiesBtn;
    private Button getLinkPropertiesBtn;

    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.lds_capabilities);
        getCapabilitiesBtn = (Button) findViewById(R.string.capabilitiesId);
        getTrackedCapabilitiesBtn = (Button) findViewById(R.string.trackedCapabilitiesId);
        getNeededCapabilitiesBtn = (Button) findViewById(R.string.neededCapabilitiesId);
        getLinkPropertiesBtn = (Button) findViewById(R.string.linkPropertiesId);

        getCapabilitiesBtn.setOnClickListener(this);
        getTrackedCapabilitiesBtn.setOnClickListener(this);
        getNeededCapabilitiesBtn.setOnClickListener(this);
        getLinkPropertiesBtn.setOnClickListener(this);
    }

    public void onClick(View v) {
        if(v.equals(getCapabilitiesBtn)) {
        Toast.makeText(this, "Not implemented yet..will be in shortly..", Toast.LENGTH_SHORT).show();
        } else if (v.equals(getTrackedCapabilitiesBtn)){
        Toast.makeText(this, "Not implemented yet..will be in shortly..", Toast.LENGTH_SHORT).show();
        } else if (v.equals(getNeededCapabilitiesBtn)) {
        Toast.makeText(this, "Not implemented yet..will be in shortly..", Toast.LENGTH_SHORT).show();
        } else if (v.equals(getLinkPropertiesBtn)) {
        Toast.makeText(this, "Not implemented yet..will be in shortly..", Toast.LENGTH_SHORT).show();
        }
    }
}
