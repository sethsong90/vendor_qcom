/******************************************************************************
 * @file    LDSTabUI.java
 * @brief   Tab UI for  LinkDatagram Socket API Services
 *
 *
 * ---------------------------------------------------------------------------
 *  Copyright (C) 2010 Qualcomm Technologies, Inc.
 *  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
 * ---------------------------------------------------------------------------
 *
 ******************************************************************************/

package com.android.QosTest;

import android.app.TabActivity;
import android.content.Intent;
import android.content.res.Resources;
import android.os.Bundle;
import android.widget.TabHost;

public class LDSTabUI extends TabActivity {
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.lds_data_tab);
        TabHost tabHost = getTabHost();
        TabHost.TabSpec spec;
        Intent intent;
        // Create an Intent to launch an Activity for the tab (to be reused)
        intent = new Intent().setClass(this, LDSData.class);
       // Initialize a TabSpec for each tab and add it to the TabHost
        spec = tabHost.newTabSpec("LDS Data transfer").setIndicator("LDS Data transfer").setContent(intent);
        tabHost.addTab(spec);
        // Do the same for the other tabs
        intent = new Intent().setClass(this, LDSCapabilities.class);
        spec = tabHost.newTabSpec("LDS Capabilities").setIndicator("LDS Capabilities").setContent(intent);
        tabHost.addTab(spec);
        tabHost.setCurrentTab(2);}
}
