/******************************************************************************
 * @file    TestAppListActivity.java
 * @brief   Main activity for QoS Test App
 *
 *
 * ---------------------------------------------------------------------------
 *  Copyright (C) 2011 Qualcomm Technologies, Inc.
 *  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
 * ---------------------------------------------------------------------------
 *
 ******************************************************************************/

package com.android.QosTest;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;

import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.app.ListActivity;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.AdapterView.OnItemClickListener;

public class TestAppListActivity extends ListActivity {

    private static final String QOS_TEST = "com.android.QosTest.QOS_TEST_ACTIVITY";
    private static final String LDS_APP = "com.android.QosTest.LDS_TEST_APP";

    private static final String APP_QOS_TEST = "QoS";
    private static final String APP_LDS_TEST = "LDSTestApp";

    public static boolean DBG = true;

    public static final String[] LIST_ENTRIES = new String[] {
        APP_QOS_TEST,
        APP_LDS_TEST,
    };

    private OnItemClickListener mClickListener = new OnItemClickListener() {
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
            if (LIST_ENTRIES[position].equals(APP_QOS_TEST)){
                    Intent mIntent = new Intent(QOS_TEST);
                    startActivity(mIntent);
                } else {
                    Intent mIntent = new Intent(LDS_APP);
                    startActivity(mIntent);
                }
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setListAdapter(new ArrayAdapter<String>(this, R.layout.list_item, LIST_ENTRIES));

        ListView mView = getListView();
        mView.setTextFilterEnabled(true);
        mView.setOnItemClickListener(mClickListener);
        }
}
