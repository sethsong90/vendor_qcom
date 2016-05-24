/******************************************************************************
 * ---------------------------------------------------------------------------
 *  Copyright (C) 2013-2014 Qualcomm Technologies, Inc.
 *  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
 *  ---------------------------------------------------------------------------
 *******************************************************************************/
package com.qti.ultrasound.penpairing;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

public class MainActivity extends Activity implements OnClickListener {

    public static final int PAIRING = 0;
    public static final int PAIRED_PENS = 1;
    public static final int NUM_OF_PAIRING_TYPES = 2;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Button pairedButton = (Button) findViewById(R.id.main_paired_pens_button);
        Button pairingButton = (Button) findViewById(R.id.main_pairing_button);

        pairedButton.setOnClickListener(this);
        pairingButton.setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
        Intent intent = null;
        switch (v.getId()) {
        case R.id.main_paired_pens_button:
            intent = new Intent(this,
                                PairedPensActivity.class);
            break;
        case R.id.main_pairing_button:
            intent = new Intent(this,
                                PairingActivity.class);
            break;
        default:
            Log.wtf(this.toString(),
                    "View id not possible");
            finish();
            break;
        }
        startActivity(intent);
    }
}

