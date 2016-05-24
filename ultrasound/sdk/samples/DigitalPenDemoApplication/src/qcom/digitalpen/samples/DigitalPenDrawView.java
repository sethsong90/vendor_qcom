/******************************************************************************
 * @file    DigitalPenDrawView.java
 * @brief   The view used by the DigitalPenDemoApplicationActivity.
 *
 * ---------------------------------------------------------------------------
 *  Copyright (C) 2012 Qualcomm Technologies, Inc.
 *  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
 *  ---------------------------------------------------------------------------
 ******************************************************************************/
package qcom.digitalpen.samples;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Point;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

/**
 * The draw view for the DigitalPen demo application. It draws a circle when
 * it receives touch events from the stylus. The DigitalPen main activity also
 * updates the color of the screen to be blue when on-screen and red when
 * off-screen.
 */
public class DigitalPenDrawView extends View {

  private DigitalPenDemoApplicationActivity mActivity   = null;
  private boolean                           mDrawCircle = false;
  private Point                             mCircle     = new Point();
  private Paint                             mPaint      = new Paint();

  public DigitalPenDrawView(Context context) {
    super(context);
  }
  public DigitalPenDrawView(Context context, AttributeSet attrs) {
    super(context, attrs);
  }
  public DigitalPenDrawView(Context context, AttributeSet attrs, int defStyle) {
    super(context, attrs, defStyle);
  }

  public void setActivity(DigitalPenDemoApplicationActivity activity) {
    mActivity = activity;
  }

  @Override
  public void onDraw(Canvas canvas) {
    if (mDrawCircle) {
      // Drawing a Circle that is enabled
      canvas.drawCircle(mCircle.x, mCircle.y, 20f, mPaint);
    }
  }

  /**
  * Notify the view when pen changes draw screen.
  * @param onScreen: true in case draw screen changed to the on screen
  *                  false if it changed to the off screen.
  */
  public void notifyDrawScreenChanged(boolean onScreen) {
    // Draw a blue circle when on screen, or a red circle else
    if (onScreen) {
      mPaint.setColor(Color.BLUE);
    } else {
      mPaint.setColor(Color.RED);
    }
  }

  @Override
  public boolean onTouchEvent(MotionEvent event) {
    // The logic is only available for stylus
    if (event.getToolType(0) == MotionEvent.TOOL_TYPE_STYLUS) {
      // No need to get historical events since it's only a sample app
      if (event.getAction() == MotionEvent.ACTION_DOWN) {
        mDrawCircle = true;
      } else if (event.getAction() == MotionEvent.ACTION_UP) {
        mDrawCircle = false;
      }
      // Update circle
      mCircle.x = (int)event.getX();
      mCircle.y = (int)event.getY();
      mActivity.switchToTouch();
      invalidate();
    }
    return true;
  }

  /**
  * We get hovering events when pen is not pressed, and is floating on top
  * of a defined area (in/out screen).
  */
  @Override
  public boolean onHoverEvent(MotionEvent event) {
    // Distance coordinates are 0.01 MM, so 2000 equals 2 CM.
    final float POINTER_SWITCH_DISTANCE = 2000f;
    // The logic is only available for stylus
    if (event.getToolType(0) == MotionEvent.TOOL_TYPE_STYLUS) {
      if (event.getAxisValue(MotionEvent.AXIS_DISTANCE) >
                                              POINTER_SWITCH_DISTANCE) {
        mActivity.switchToMouse();
      }
    }
    return true;
  }
}
