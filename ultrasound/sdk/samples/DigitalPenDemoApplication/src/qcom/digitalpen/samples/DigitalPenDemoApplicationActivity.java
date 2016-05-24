/******************************************************************************
 * @file    DigitalPenDemoApplicationActivity.java
 * @brief   This activity is the main activity of the demo application that
 *          shows how to use the Digital Pen framework. It uses various features
 *          of the framework, such as on/off screen notification, power states
 *          notification, and different configurations.
 *
 * ---------------------------------------------------------------------------
 *  Copyright (c) 2012-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
 *  Qualcomm Technologies Proprietary and Confidential.
 *  ---------------------------------------------------------------------------
 ******************************************************************************/
package qcom.digitalpen.samples;

import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.widget.Button;
import android.widget.Toast;

import com.qti.snapdragon.sdk.digitalpen.DigitalPenActivity;
import com.qti.snapdragon.sdk.digitalpen.DigitalPenAdapter;
import com.qti.snapdragon.digitalpen.util.DigitalPenConfig;
import com.qti.snapdragon.digitalpen.util.DigitalPenEvent;

/**
 * This activity has two modes of operations, one that shows a mouse pointer,
 * and one that doesn't. The activity switches between the two modes based on
 * the distance of the pen from the display. When the pen touches the display
 * the mouse pointer disappears and the pen is used as a finger on a touch
 * screen. When the pen is held 2cm above the device the mouse pointer appears
 * and the pen is used as a mouse. It also has two buttons, one that
 * enables/disables on screen, and one that enables/disables off screen.
 */
public class DigitalPenDemoApplicationActivity extends DigitalPenActivity {

  private DigitalPenConfig mTouchConfig;
  private DigitalPenConfig mPointerConfig;
  // The mIsPointer flag indicates whether we are currently in a touch or mouse
  // state.
  private boolean mIsPointer;

  private DigitalPenDrawView mView = null;

  // Flags to indicate whether on/off screen is enabled/disabled
  private boolean mIsOnScreenEn;
  private boolean mIsOffScreenEn;
  // Buttons variables
  private Button mOnScreenBtn;
  private Button mOffScreenBtn;
  // Define a handler for this thread
  private Handler mHandler = new Handler() {
    public void handleMessage(Message msg) {
      switch (msg.arg1) {
      case DigitalPenEvent.TYPE_POWER_STATE_CHANGED:
        // arg2 value is the current power state in this case
        Toast.makeText(getApplicationContext(),
        "Digital pen state is now " + powerStateText(msg.arg2),
        Toast.LENGTH_SHORT).show();

        if (msg.arg2 == DigitalPenEvent.POWER_STATE_OFF) {
        // We don't want to continue when Digital Pen framework is off.
        finish();
        }
        break;
      }
    }
  };

  /**
  * Returns the text that we want to display for the given power state value.
  */
  public String powerStateText(int value) {
    switch (value) {
    case DigitalPenEvent.POWER_STATE_ACTIVE:
      return "Active";
    case DigitalPenEvent.POWER_STATE_STANDBY:
      return "Standby";
    case DigitalPenEvent.POWER_STATE_IDLE:
      return "Idle";
    case DigitalPenEvent.POWER_STATE_OFF:
      return "Off - Exiting now!";
    }
    return "Undefined";
  }
  /**
   * Inits the configurations so that on/off screen and 3D are enabled, it also
   * sets the touch configuration to not show a pointer and the pointer
   * configuration to show a pointer.
   */
  private void initConfigs() {
    // Init the touch configuration (doesn't not show a mouse pointer)
    mTouchConfig = new DigitalPenConfig();
    mTouchConfig.enableOnScreen();
    mTouchConfig.enableOffScreen();
    mTouchConfig.enable3D();
    mTouchConfig.setToolType(DigitalPenConfig.DP_TOOL_TYPE_STYLUS);

    // Init the mouse configuration (shows a mouse pointer)
    mPointerConfig = new DigitalPenConfig();
    mPointerConfig.enableOnScreen();
    mPointerConfig.enableOffScreen();
    mPointerConfig.enable3D();
    mPointerConfig.setToolType(DigitalPenConfig.DP_TOOL_TYPE_STYLUS_POINTER);
  }

  /**
  * Changes the configuration of the Digital Pen framework to hide mouse pointer.
  * This way, when touch events are received, no pointer will be shown.
  */
  public void switchToTouch() {
    if (mIsPointer) {
      setConfig(mTouchConfig);
      mIsPointer = false;
    }
  }
  /**
  * Changes the configuration of the Digital Pen framework to show mouse pointer.
  */
  public void switchToMouse() {
    if (!mIsPointer) {
      setConfig(mPointerConfig);
      mIsPointer = true;
    }
  }

  /** Called when the activity is first created. */
  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    initConfigs();
    // Init on/off screen enabled flags to true
    mIsOnScreenEn  = true;
    mIsOffScreenEn = true;

    setContentView(R.layout.main);
    mView = (DigitalPenDrawView) findViewById(R.id.drawView);
    mView.setActivity(this);
    mOnScreenBtn  = (Button) findViewById(R.id.onScreenButton);
    mOffScreenBtn = (Button) findViewById(R.id.offScreenButton);

    mDigitalPenAdapter.setOnDrawScreenChangedListener(new DigitalPenAdapter.OnDrawScreenChangedListener() {
      @Override
      public void onDrawScreenChanged(DigitalPenEvent event) {
        switch(event.getParameterValue(DigitalPenEvent.PARAM_CURRENT_PEN_DRAW_SCREEN)) {
        case DigitalPenEvent.SCREEN_ON:
          if (mView != null) {
            mView.notifyDrawScreenChanged(true);
          }
          break;
        case DigitalPenEvent.SCREEN_OFF:
          if (mView != null) {
            mView.notifyDrawScreenChanged(false);
          }
          break;
        }
      }
    });

    mDigitalPenAdapter.setOnPowerStateChangedListener(new DigitalPenAdapter.OnPowerStateChangedListener() {
      @Override
      public void onPowerStateChanged(DigitalPenEvent event) {
        // Let the main thread know about this
        Message msg = new Message();
        msg.arg1 = DigitalPenEvent.TYPE_POWER_STATE_CHANGED;
        msg.arg2 = event.getParameterValue(DigitalPenEvent.PARAM_CURRENT_POWER_STATE);
        mHandler.sendMessage(msg);
      }
    });

    mOnScreenBtn.setOnTouchListener(new OnTouchListener() {
      public boolean onTouch(View v, MotionEvent event) {
        // We filter out stylus events, so that the buttons can only
        // be pressed by finger.
        if (event.getToolType(0) != MotionEvent.TOOL_TYPE_STYLUS &&
            event.getAction() == MotionEvent.ACTION_UP) {
          // Switch enabled/disabled state of the on screen
          if (mIsOnScreenEn) {
            mIsOnScreenEn = false;
            // Disable on screen for both configurations
            mTouchConfig.disableOnScreen();
            mPointerConfig.disableOnScreen();
            // Re-set the appropriate configuration
            if (mIsPointer) {
              setConfig(mPointerConfig);
            } else {
              setConfig(mTouchConfig);
            }
            mOnScreenBtn.setBackgroundColor(Color.RED);
            mOnScreenBtn.setText(R.string.onScreenDisText);
          } else {
            mIsOnScreenEn = true;
            // Enable on screen for both configurations
            mTouchConfig.enableOnScreen();
            mPointerConfig.enableOnScreen();
            // Re-set the appropriate configuration
            if (mIsPointer) {
              setConfig(mPointerConfig);
            } else {
              setConfig(mTouchConfig);
            }
            mOnScreenBtn.setBackgroundColor(Color.GREEN);
            mOnScreenBtn.setText(R.string.onScreenEnText);
          }
        }
        return true;
      }
    });

    mOffScreenBtn.setOnTouchListener(new OnTouchListener() {
      public boolean onTouch(View v, MotionEvent event) {
        // We filter out stylus events, so that the buttons can only
        // be pressed by finger.
        if (event.getToolType(0) != MotionEvent.TOOL_TYPE_STYLUS &&
          event.getAction() == MotionEvent.ACTION_UP) {
          // Switch enabled/disabled state of the off screen
          if (mIsOffScreenEn) {
            mIsOffScreenEn = false;
            // Disable off screen for both configurations
            mTouchConfig.disableOffScreen();
            mPointerConfig.disableOffScreen();
            // Re-set the appropriate configuration
            if (mIsPointer) {
              setConfig(mPointerConfig);
            } else {
              setConfig(mTouchConfig);
            }
            mOffScreenBtn.setBackgroundColor(Color.RED);
            mOffScreenBtn.setText(R.string.offScreenDisText);
          } else {
            mIsOffScreenEn = true;
            // Enable off screen for both configurations
            mTouchConfig.enableOffScreen();
            mPointerConfig.enableOffScreen();
            // Re-set the appropriate configuration
            if (mIsPointer) {
              setConfig(mPointerConfig);
            } else {
              setConfig(mTouchConfig);
            }
            mOffScreenBtn.setBackgroundColor(Color.GREEN);
            mOffScreenBtn.setText(R.string.offScreenEnText);
          }
        }
        return true;
      }
    });
  }
}
