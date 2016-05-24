/******************************************************************************
 * ---------------------------------------------------------------------------
 *  Copyright (C) 2013-2014 Qualcomm Technologies, Inc.
 *  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
 *  ---------------------------------------------------------------------------
 *******************************************************************************/
package com.qti.ultrasound.penpairing;

import java.io.IOException;

import android.app.AlertDialog;
import android.app.ListActivity;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.graphics.Typeface;
import android.os.Bundle;
import android.text.InputType;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.CheckedTextView;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;
import android.text.InputFilter;

public class PairedPensActivity extends ListActivity {
    public static final String NAME_SHARED_PREFERENCE = "PAIRED_PENS_PREF";
    private static final String KEY_ACTIVE_PEN = "active_pen";
    private static final String PEN_SERIES_FILE_NAME = "series_calib";
    private static final String CALIB_FILE_EXTENSION = ".dat";
    private static final String PEN_SELECTED_SERIES_LINK = "/data/usf/epos/"
            + PEN_SERIES_FILE_NAME + CALIB_FILE_EXTENSION;
    private static final String PEN_SERIES_PATH = "/persist/usf/pen_pairing/";

    private static final String SELECT_CURRENT = "Select as current";
    private static final String REMOVE_PAIRING = "Remove pairing";
    private static final String RENAME = "Rename";
    private static final String CANCEL = "Cancel";

    private PairingDbHelper mPairingDbHelper;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mPairingDbHelper = new PairingDbHelper(this);

        TextView tv = new TextView(this);
        tv.setText("List of paired pens");
        tv.setTextSize(30);
        tv.setHeight(40);
        getListView().addHeaderView(tv,
                                    null,
                                    false);
    }

    @Override
    protected void onResume() {
        super.onResume();

        setListAdapter(getNewAdapter());
    }

    private ArrayAdapter<String> getNewAdapter() {
        return new ArrayAdapter<String>(this,
                                        android.R.layout.simple_list_item_checked,
                                        mPairingDbHelper.getPenNames()) {
            @Override
            public View
                    getView(int position, View convertView, ViewGroup parent) {
                CheckedTextView v = (CheckedTextView) super.getView(position,
                                                                    convertView,
                                                                    parent);
                v.setTextSize(30);
                v.setHeight(40);
                String activePen = getSharedPreferences(NAME_SHARED_PREFERENCE,
                                                        0).getString(KEY_ACTIVE_PEN,
                                                                     null);
                Log.d(this.toString(),
                      "activePen: " + activePen);
                if (v.getText()
                     .toString()
                     .equals(activePen)) {
                    v.setChecked(true);
                    v.setTypeface(null,
                                  Typeface.BOLD);
                }
                return v;
            }
        };
    }

    @Override
    public void onListItemClick(ListView l, View v, int position, long id) {
        final String penName = ((TextView) v).getText()
                                             .toString();
        final String[] options = { SELECT_CURRENT, REMOVE_PAIRING, RENAME,
                CANCEL };
        AlertDialog.Builder alert = new AlertDialog.Builder(this);
        alert.setTitle(penName)
             .setItems(options,
                       new DialogInterface.OnClickListener() {
                           @Override
                           public void
                                   onClick(DialogInterface dialog, int which) {
                               String choice = options[which];

                               if (SELECT_CURRENT.equals(choice)) {
                                   changeCurrent(penName,
                                                 mPairingDbHelper,
                                                 getSharedPreferences(NAME_SHARED_PREFERENCE,
                                                                      0).edit());
                                   setListAdapter(getNewAdapter());
                               } else if (REMOVE_PAIRING.equals(choice)) {
                                   removePen(penName);
                                   setListAdapter(getNewAdapter());
                               } else if (RENAME.equals(choice)) {
                                   onRename(penName);
                               } else if (CANCEL.equals(choice)) {
                                   // Do nothing
                               }
                           }
                       });
        alert.show();
    }

    private void removePen(String penName) {
        String activePen = getSharedPreferences(NAME_SHARED_PREFERENCE,
                                                0).getString(KEY_ACTIVE_PEN,
                                                             null);
        mPairingDbHelper.removePen(penName);
        if (penName.equals(activePen)) {
            SharedPreferences.Editor editor = getSharedPreferences(NAME_SHARED_PREFERENCE,
                                                                   0).edit();
            editor.remove(KEY_ACTIVE_PEN);
            editor.commit();
            removeSelectedSeriesLink();
        }
    }

    public static void changeCurrent(String penName,
                                     PairingDbHelper pairingDbHelper,
                                     SharedPreferences.Editor editor) {
        editor.putString(KEY_ACTIVE_PEN,
                         penName);
        editor.commit();
        removeSelectedSeriesLink();
        String seriesFile = PEN_SERIES_PATH + PEN_SERIES_FILE_NAME
                + String.valueOf(pairingDbHelper.getPenId(penName)) + CALIB_FILE_EXTENSION;
        try {
            Runtime.getRuntime()
                   .exec("ln -s " + seriesFile + " " + PEN_SELECTED_SERIES_LINK);
        } catch (IOException e) {
            Log.e(PairedPensActivity.class.getName(),
                  "Failed creating link to series file " + seriesFile);
        }
    }

    private static void removeSelectedSeriesLink() {
        try {
            Runtime.getRuntime()
                   .exec("rm -rf " + PEN_SELECTED_SERIES_LINK);
        } catch (IOException e) {
            Log.e(PairedPensActivity.class.getName(),
                  "Failed removing " + PEN_SELECTED_SERIES_LINK);
        }
    }

    private void onRename(final String penName) {
        AlertDialog.Builder alert = new AlertDialog.Builder(this);
        final EditText input = new EditText(this);
        final SharedPreferences.Editor editor = getSharedPreferences(NAME_SHARED_PREFERENCE,
                                                                     0).edit();
        final String activePen = getSharedPreferences(NAME_SHARED_PREFERENCE,
                                                      0).getString(KEY_ACTIVE_PEN,
                                                                   null);
        input.setFilters(new InputFilter[]{PairingDbHelper.penNameFilter});
        input.setText(penName);
        alert.setTitle("Rename Pen");
        alert.setMessage("Set a new name for " + penName);
        alert.setView(input);
        alert.setPositiveButton("OK",
                                new DialogInterface.OnClickListener() {
                                    public void onClick(DialogInterface dialog,
                                                        int whichButton) {
                                        String to = input.getText()
                                                         .toString();
                                        boolean res = mPairingDbHelper.changePenName(to,
                                                                                     penName);
                                        if (res) {
                                            if (penName.equals(activePen)) {
                                                editor.putString(KEY_ACTIVE_PEN,
                                                                 to);
                                                editor.commit();
                                            }
                                        }
                                        setListAdapter(getNewAdapter());
                                    }
                                });
        alert.setNegativeButton("Cancel",
                                new DialogInterface.OnClickListener() {
                                    public void onClick(DialogInterface dialog,
                                                        int whichButton) {
                                    }
                                });
        alert.setCancelable(false);
        alert.show();
    }
}

