/******************************************************************************
 * @file    DigitalPenSettingsActivity.java
 * @brief   Digital Pen Settings
 *
 * ---------------------------------------------------------------------------
 *  Copyright (C) 2012-2014 Qualcomm Technologies, Inc.
 *  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
 *  ---------------------------------------------------------------------------
 ******************************************************************************/

package qcom.digitalpen;

import com.qti.snapdragon.digitalpen.IDigitalPen;
import com.qti.snapdragon.digitalpen.IEventCallback;
import com.qti.snapdragon.digitalpen.util.DigitalPenConfig;
import com.qti.snapdragon.digitalpen.util.DigitalPenEvent;

import android.app.Activity;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.EditText;
import android.widget.CompoundButton;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;
import android.widget.Toast;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnFocusChangeListener;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.util.Log;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.Context;
import android.content.ComponentName;
import android.os.IBinder;
import android.os.SystemProperties;
import android.os.RemoteException;
import android.os.Bundle;
import android.os.Handler;

public class DigitalPenSettingsActivity extends Activity implements OnCheckedChangeListener {
    private static final String TAG = "DigitalPenSettingsActivity";

    private boolean isDigitalPenEnabled;
    // TODO: wrap IDigitalPen, e.g. with DigitalPenGlobalSettings
    private IDigitalPen serviceHandle;
    private final IEventCallback eventCb = new IEventCallback.Stub() {
        @Override
        public void onDigitalPenPropEvent(DigitalPenEvent event) {
            // Listen for power state change
            int powerState = event
                    .getParameterValue(DigitalPenEvent.PARAM_CURRENT_POWER_STATE);
            Log.d(TAG, "Power state: " + powerState);
            if (needToHandlePowerStateEvent(powerState)) {
                final boolean newEnableValue = powerState == DigitalPenEvent.POWER_STATE_ACTIVE;
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        isDigitalPenEnabled = newEnableValue;
                        updateUi();
                    }
                });

            }

        }

        protected boolean needToHandlePowerStateEvent(int powerState) {
            // other power states, such as idle, are informational
            return powerState == DigitalPenEvent.POWER_STATE_ACTIVE
                    || powerState == DigitalPenEvent.POWER_STATE_OFF;
        }
    };
    private final Handler mHandler = new Handler();
    private final ServiceConnection mConnection = new ServiceConnection() {
        /**
         * Called when the connection with the service is established
         */
        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            // Gets an instance of the IDigitalPen, which we can use to call on
            // the service
            Log.d(TAG, "Connected to the service");
            serviceHandle = IDigitalPen.Stub.asInterface(service);
            try {
                // Register the event callback(s) and update config with what's
                // shown on UI
                serviceHandle.registerEventCallback(eventCb);
            } catch (RemoteException e) {
                Log.e(TAG, "Remote service error: " + e.getMessage());
            }
        }

        /**
         * Called when the connection with the service disconnects unexpectedly
         */
        @Override
        public void onServiceDisconnected(ComponentName className) {
            Log.e(TAG, "Fatal: Service disconnected");
            serviceHandle = null;
        }
    };

    private EditText inRangeDistance;

    private DigitalPenConfig config;

    private EditText onScreenHoverMaxDistance;

    private EditText offScreenHoverMaxDistance;

    private EditText eraseButtonIndex;

    private CompoundButton eraseButtonEnabled;

    private boolean initializingWidgetValues;

    public void updateUi() {
        Log.d(TAG, "updateUi");
        // set enabled/disabled
        disableEnableControls(isDigitalPenEnabled, (ViewGroup) findViewById(R.id.topLayout));
        findViewById(R.id.toggleButtonEnable).setEnabled(true);
    }

    private void disableEnableControls(boolean enable, ViewGroup vg) {
        for (int i = 0; i < vg.getChildCount(); i++) {
            View child = vg.getChildAt(i);
            child.setEnabled(enable);
            if (child instanceof ViewGroup) {
                disableEnableControls(enable, (ViewGroup) child);
            }
        }
    }

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "onCreate");
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        // initialize edit text callbacks
        inRangeDistance = (EditText) findViewById(R.id.editTextInRangeDistance);
        setEditDoneListeners(inRangeDistance);
        onScreenHoverMaxDistance = (EditText) findViewById(R.id.editTextOnScreenHoverMaxDistance);
        setEditDoneListeners(onScreenHoverMaxDistance);
        offScreenHoverMaxDistance = (EditText) findViewById(R.id.editTextOffScreenHoverMaxDistance);
        setEditDoneListeners(offScreenHoverMaxDistance);
        eraseButtonIndex = (EditText) findViewById(R.id.editTextEraserIndex);
        setEditDoneListeners(eraseButtonIndex);
        eraseButtonEnabled = (CompoundButton) findViewById(R.id.switchEraserEnable);

        // initialize switch callbacks; onClick doesn't capture finger slide
        final int[] switchIds = {
                R.id.switchSmarterStand,
                R.id.switchOnScreenHover,
                R.id.switchOffScreenHover,
                R.id.switchOnScreenHoverIcon,
                R.id.switchOffScreenHoverIcon,
                R.id.switchEraserEnable
        };
        for (int switchId : switchIds) {
            ((CompoundButton) findViewById(switchId)).setOnCheckedChangeListener(this);
        }

        // Start the service with no callbacks
        final String serviceName = IDigitalPen.class.getName();
        Log.e(TAG, "serviceName: " + serviceName);
        getApplicationContext().bindService(new Intent(serviceName),
                mConnection,
                Context.BIND_AUTO_CREATE);
        isDigitalPenEnabled = !(SystemProperties.get("init.svc.usf_epos",
                "stopped")).equals("stopped");

        if (config == null) {
            // TODO: read from persistent store
            config = new DigitalPenConfig();
        }
    }

    @Override
    protected void onResume() {
        Log.d(TAG, "onResume");
        super.onResume();
        initializeWidgetValues();
    }

    @Override
    public void onDestroy() {
        if (serviceHandle != null) {
            getApplicationContext().unbindService(mConnection);
        }
        super.onDestroy();
    }

    private void initializeWidgetValues() {
        initializingWidgetValues = true;
        isDigitalPenEnabled = false;
        if (serviceHandle != null) {
            try {
                isDigitalPenEnabled = serviceHandle.isEnabled();
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
        setCheckbox(R.id.toggleButtonEnable, isDigitalPenEnabled);

        // set values read from config object
        if (config.getOffScreenMode() == DigitalPenConfig.DP_OFF_SCREEN_MODE_DISABLED) {
            setCheckbox(R.id.radioOffScreenDisabled, true);
        } else {
            setCheckbox(R.id.radioOffScreenDuplicate, true);
        }
        setCheckbox(R.id.switchSmarterStand, config.isSmarterStandEnabled());
        inRangeDistance.setText(Integer.toString(config.getTouchRange()));

        setCheckbox(R.id.switchOnScreenHover, config.isOnScreenHoverEnabled());
        onScreenHoverMaxDistance.setText(Integer.toString(config.getOnScreenHoverMaxRange()));
        setCheckbox(R.id.switchOnScreenHoverIcon, config.isShowingOnScreenHoverIcon());

        setCheckbox(R.id.switchOffScreenHover, config.isOffScreenHoverEnabled());
        offScreenHoverMaxDistance.setText(Integer.toString(config.getOffScreenHoverMaxRange()));
        setCheckbox(R.id.switchOffScreenHoverIcon, config.isShowingOffScreenHoverIcon());

        if (config.getOffSceenPortraitSide() == DigitalPenConfig.DP_PORTRAIT_SIDE_LEFT) {
            setCheckbox(R.id.radioOffScreenLocationLeft, true);
        } else {
            setCheckbox(R.id.radioOffScreenLocationRight, true);
        }

        int buttonIndex = config.getEraseButtonIndex();
        setCheckbox(R.id.switchEraserEnable, buttonIndex != -1);
        eraseButtonIndex.setText(buttonIndex == -1 ? "0" : Integer
                .toString(buttonIndex));
        if (config.getEraseButtonMode() == DigitalPenConfig.DP_ERASE_MODE_HOLD) {
            setCheckbox(R.id.radioEraserBehaviorHold, true);
        } else {
            setCheckbox(R.id.radioEraserBehaviorToggle, true);
        }

        if (config.getPowerSave() == DigitalPenConfig.DP_POWER_PROFILE_OPTIMIZE_ACCURACY) {
            setCheckbox(R.id.radioPowerModeAccuracy, true);
        } else {
            setCheckbox(R.id.radioPowerModePower, true);
        }
        updateUi();
        initializingWidgetValues = false;
    }

    private void setCheckbox(int id, boolean set) {
        ((CompoundButton) findViewById(id)).setChecked(set);
    }

    private void setEditDoneListeners(EditText editText) {
        editText.setOnEditorActionListener(new OnEditorActionListener() {

            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_DONE) {
                    return editTextDone(v);
                }
                return false;
            }
        });
        editText.setOnFocusChangeListener(new OnFocusChangeListener() {

            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                if (!hasFocus) {
                    editTextDone((TextView) v);
                }
            }
        });
    }

    protected boolean editTextDone(TextView v) {
        // TODO: validate in each case
        final int value = Integer.parseInt(v.getText().toString());
        switch (v.getId()) {
            case R.id.editTextInRangeDistance:
                config.setTouchRange(value);
                break;
            case R.id.editTextOnScreenHoverMaxDistance:
                config.setOnScreenHoverMaxRange(value);
                break;
            case R.id.editTextOffScreenHoverMaxDistance:
                config.setOffScreenHoverMaxRange(value);
                break;
            case R.id.editTextEraserIndex:
                if (eraseButtonEnabled.isChecked()) {
                    config.setEraseButtonIndex(value);
                } else {
                    config.setEraseButtonIndex(-1);
                }
                break;
            default:
                throw new RuntimeException("Unknown text view in editTextDone: " + v);
        }
        loadConfig();
        return true;
    }

    public void onClickEnableButton(View v) {
        Log.d(TAG, "onClickEnableButton: " + ((CompoundButton) v).isChecked());
        if (null == serviceHandle) { // No connection yet
            return; // Nothing to do
        }
        final String serviceName = IDigitalPen.class.getName();
        if (isDigitalPenEnabled) {
            try {
                if (!serviceHandle.disable()) {
                    Log.e(TAG,
                            "Failed to disable daemon");
                    Toast.makeText(this,
                            "Failed to disable Digital Pen daemon",
                            Toast.LENGTH_SHORT).show();
                    // Return button to its previous state
                    updateUi();
                }
            } catch (RemoteException e) {
                Log.e(TAG, "Remote service error: " + e.getMessage());
            }
            // We stop the service after disabling
            getApplicationContext().stopService(
                    new Intent(serviceName));
        } else {
            // Service must stay on when enabled
            getApplicationContext().startService(
                    new Intent(serviceName));
            try {
                if (!serviceHandle.enable()) {
                    Log.e(TAG,
                            "Failed to enable daemon");
                    Toast.makeText(this,
                            "Failed to enable Digital Pen daemon",
                            Toast.LENGTH_SHORT).show();
                    // Return button to its previous state
                    updateUi();
                } else {
                    loadConfig();
                }
            } catch (RemoteException e) {
                Log.e(TAG, "Remote service error: " + e.getMessage());
            }
        }
    }

    private void loadConfig() {
        Log.d(TAG, "loadConfig");
        boolean success = false;
        try {
            success = serviceHandle.loadConfig(config);
        } catch (RemoteException e) {
            Log.e(TAG, "Remote exception trying to load config");
            e.printStackTrace();
        } finally {
            if (!success) {
                Log.e(TAG, "loadConfig failed");
                Toast.makeText(this, "Failed to load config", Toast.LENGTH_SHORT).show();
            } else {
                Log.d(TAG, "loadConfig successful");
            }
        }
    }

    public void onClickRadioOffScreenMode(View v) {
        switch (v.getId()) {
            case R.id.radioOffScreenDisabled:
                config.setOffScreenMode(DigitalPenConfig.DP_OFF_SCREEN_MODE_DISABLED);
                break;
            case R.id.radioOffScreenDuplicate:
                config.setOffScreenMode(DigitalPenConfig.DP_OFF_SCREEN_MODE_DUPLICATE);
                break;
            default:
                throw new RuntimeException("Unknown ID:" + v.getId());
        }
        loadConfig();
        updateUi();
    }

    public void onClickRadioPowerMode(View v) {
        switch (v.getId()) {
            case R.id.radioPowerModeAccuracy:
                config.setPowerSave(DigitalPenConfig.DP_POWER_PROFILE_OPTIMIZE_ACCURACY);
                break;
            case R.id.radioPowerModePower:
                config.setPowerSave(DigitalPenConfig.DP_POWER_PROFILE_OPTIMIZE_POWER);
                break;
            default:
                throw new RuntimeException("Unknown ID:" + v.getId());
        }
        loadConfig();
        updateUi();
    }

    public void onClickRadioOffScreenLocation(View v) {
        switch (v.getId()) {
            case R.id.radioOffScreenLocationLeft:
                config.setOffScreenPortraitSide(DigitalPenConfig.DP_PORTRAIT_SIDE_LEFT);
                break;
            case R.id.radioOffScreenLocationRight:
                config.setOffScreenPortraitSide(DigitalPenConfig.DP_PORTRAIT_SIDE_RIGHT);
                break;
            default:
                throw new RuntimeException("Unknown ID:" + v.getId());
        }
        loadConfig();
        updateUi();
    }

    public void onClickRadioEraserBehavior(View v) {
        switch (v.getId()) {
            case R.id.radioEraserBehaviorHold:
                config.setEraseButtonBehavior(DigitalPenConfig.DP_ERASE_MODE_HOLD);
                break;
            case R.id.radioEraserBehaviorToggle:
                config.setEraseButtonBehavior(DigitalPenConfig.DP_ERASE_MODE_TOGGLE);
                break;
            default:
                throw new RuntimeException("Unknown ID:" + v.getId());
        }
        loadConfig();
        updateUi();
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        if (initializingWidgetValues) {
            return; // button set due to resume or create, not user action
        }
        switch (buttonView.getId()) {

            case R.id.switchEraserEnable:
                // the edit text done handler checks this button state and
                // handles config change.
                editTextDone(eraseButtonIndex);
                return; // all other buttons fall-through to loadConfig
            case R.id.switchSmarterStand:
                config.setSmarterStand(isChecked);
                break;
            case R.id.switchOnScreenHover:
                config.setOnScreenHoverEnable(isChecked);
                break;
            case R.id.switchOffScreenHover:
                config.setOffScreenHoverEnable(isChecked);
                break;
            case R.id.switchOnScreenHoverIcon:
                config.setShowOnScreenHoverIcon(isChecked);
                break;
            case R.id.switchOffScreenHoverIcon:
                config.setShowOffScreenHoverIcon(isChecked);
                break;
            default:
                throw new RuntimeException("Unknown switch, id: " + buttonView.getId());
        }
        loadConfig();
        updateUi();
    }

    public void onClickEraserEnable(View v) {
        // the edit text done handler checks this button state
        editTextDone(eraseButtonIndex);
    }

}
