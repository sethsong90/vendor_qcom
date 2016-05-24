/*===========================================================================
                           ExtendedOffscreenFragment.java

Copyright (c) 2014 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
==============================================================================*/

package com.qti.extendedoffscreen;

import com.qti.extendedoffscreen.R;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager.Area;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager.Mapping;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenManager.OffScreenMode;

import android.app.Fragment;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnDismissListener;
import android.graphics.Bitmap;
import android.graphics.drawable.ColorDrawable;
import android.graphics.Color;
import android.hardware.display.DisplayManager;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.Display;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.ToggleButton;

public class ExtendedOffscreenFragment extends Fragment implements OnClickListener {

    static final String TAG = "ExtendedOffscreenFragment";

    protected static final long PREVIEW_UPDATE_PERIOD_MS = 100;

    private ImageView preview;

    private OffscreenPresentation offscreenPresentation;

    private boolean isPreviewEnabled;

    protected Bitmap offscreenCache;

    private final Runnable previewTask = new Runnable() {

        @Override
        public void run() {
            if (isPreviewEnabled && preview != null) {
                View topView = offscreenPresentation
                        .findViewById(android.R.id.content);
                if (!topView.isDrawingCacheEnabled()) {
                    topView.setDrawingCacheEnabled(true);
                }
                topView.buildDrawingCache();
                offscreenCache = topView.getDrawingCache();
                if (offscreenCache != null) {
                    preview.setImageBitmap(offscreenCache.copy(offscreenCache.getConfig(), false));
                    topView.destroyDrawingCache();
                    // topView.setDrawingCacheEnabled(false);
                }
            }
            handler.postDelayed(this, PREVIEW_UPDATE_PERIOD_MS);
        }

    };

    private Handler handler;

    private DigitalPenManager digitalPenManager;

    public ExtendedOffscreenFragment() {
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View topView = inflater.inflate(R.layout.fragment_extended_offscreen, container, false);
        topView.findViewById(R.id.toggleButtonOffscreenEnable).setOnClickListener(this);
        preview = (ImageView) topView.findViewById(R.id.imageViewOffscreenPreview);
        clearPreview();
        digitalPenManager = new DigitalPenManager(getActivity().getApplication());

        return topView;
    }

    private void onClickOffscreenEnable(View v) {
        ToggleButton btn = (ToggleButton) v;
        if (btn.isChecked()) {
            createOffscreenPresentation();
            digitalPenManager.setOffScreenMode(OffScreenMode.EXTEND);
            digitalPenManager.setCoordinateMapping(Area.OFF_SCREEN, Mapping.ANDROID);
            isPreviewEnabled = true;
        } else {
            digitalPenManager.setOffScreenMode(OffScreenMode.DISABLED);
            isPreviewEnabled = false;
            clearPreview();
        }
    }

    @Override
    public void onResume() {
        super.onResume();

        // restart presentation if enabled
        onClickOffscreenEnable(getActivity().findViewById(R.id.toggleButtonOffscreenEnable));

        // restart preview task
        handler = new Handler();
        previewTask.run();
    }

    @Override
    public void onPause() {
        handler.removeCallbacks(previewTask);
        super.onPause();
    }

    private void clearPreview() {
        preview.setImageDrawable(new ColorDrawable(Color.GRAY));
    }

    private void createOffscreenPresentation() {
        Context ctxt = getActivity();

        Display offscreenDisplay = findOffscreenDPenDisplay(ctxt);

        offscreenPresentation = new OffscreenPresentation(getActivity(), offscreenDisplay);
        offscreenPresentation.setOnDismissListener(new OnDismissListener() {

            @Override
            public void onDismiss(DialogInterface dialog) {
                isPreviewEnabled = false;
            }
        });
        offscreenPresentation.show();
    }

    // TODO: move into SDK
    private Display findOffscreenDPenDisplay(Context ctxt) {
        DisplayManager displayMgr = (DisplayManager) ctxt.getSystemService(Context.DISPLAY_SERVICE);
        Display[] displays = displayMgr.getDisplays();
        Display offscreenDisplay = null;
        for (Display display : displays) {
            String displayName = display.getName();
            Log.d(TAG, "Display " + display.getDisplayId() + ": " + displayName);
            if (displayName.contains("DPen")) {
                // TODO: string currently "DPen off-screen display"; more robust
                // method?
                offscreenDisplay = display;
                break;
            }
        }

        if (offscreenDisplay == null) {
            // TODO: reconsider this failure handling
            throw new RuntimeException("No DPen display found!");
        }
        return offscreenDisplay;
    }

    @Override
    public void onClick(View arg0) {
        switch (arg0.getId()) {
            case R.id.toggleButtonOffscreenEnable:
                onClickOffscreenEnable(arg0);
                break;
        }
    }

}
