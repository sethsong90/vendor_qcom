/*===========================================================================
                           DigitalPenAdapter.java

Copyright (c) 2012-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
=============================================================================*/
package com.qti.snapdragon.sdk.digitalpen;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.util.Log;

import com.qti.snapdragon.digitalpen.*;
import com.qti.snapdragon.digitalpen.util.*;

/**
 * DigitalPenAdapter class:
 *   This class abstracts all communication needed with the service
 *   for both activities and services.
 * <p>Usage:-
 * <pre>
 *      public class DemoActivity extends ListActivity {
 *              private DigitalPenAdapter mDigitalPenAdapter;
 *              private DigitalPenConfig  mDigitalPenConfig;
 *
 *              &#64;Override
 *              public void onCreate(Bundle savedInstanceState) {
 *                      super.onCreate(savedInstanceState);
 *                      mDigitalPenAdapter = new DigitalPenAdapter(getApplicationContext());
 *                      if (!mDigitalPenAdapter.isSupported()) {
 *                              // Handle the case where the digital pen feature is not supported
 *                      }
 *                      if (!mDigitalPenAdapter.isEnabled()) {
 *                              // Handle the case where the digital pen feature is not enabled
 *                      }
 *                      // Init the configuration we want to use
 *                      mDigitalPenConfig = new DigitalPenConfig();
 *                      mDigitalPenConfig.enable3D();
 *                      ...
 *                      // Set listeners
 *                      mDigitalPenAdapter.setOnPowerStateChangedListener(new OnPowerStateChangedListener() {
 *                        @Override
 *                        void onPowerStateChanged(DigitalPenEvent event) {
 *                          // Power state changed listener implementation
 *                        }
 *                      });
 *              }
 *
 *              &#64;Override
 *              public void onResume() {
 *                      super.onResume();
 *                      mDigitalPenAdapter.loadConfig(mDigitalPenConfig);
 *              }
 *
 *              &#64;Override
 *              public void onPause() {
 *                      super.onPause();
 *                      mDigitalPenAdapter.unloadConfig();
 *              }
 *
 *              &#64;Override
 *              public void onDestroy() {
 *                      super.onDestroy();
 *                      // unbind the adapter before exiting
 *                      mDigitalPenAdapter.unbind();
 *              }
 *      }
 * </pre>
 */
public class DigitalPenAdapter {

  private static final String TAG                     = "DigitalPenAdapter";
  private static final String DIGITAL_PEN_SYSTEM_PROP = "ro.qc.sdk.us.digitalpen";

  private IDigitalPen mService;
  private Context mContext;
  // Listeners definitions
  private DigitalPenConfig mConfig;
  private OnPowerStateChangedListener    mPowerStateChangedListener = null;
  private OnInternalDaemonErrorListener  mInternalDaemonErrorListener = null;
  private OnMicBlockStateChangedListener mMicBlockStateChangedListener = null;
  private OnDigitalPenDataListener       mDigitalPenDataListener = null;
  private IEventCallback mEventCb = new IEventCallback.Stub() {
    public void onDigitalPenPropEvent(DigitalPenEvent event) {
      switch (event.getEventType()) {
      case DigitalPenEvent.TYPE_POWER_STATE_CHANGED:
        if (mPowerStateChangedListener != null) {
          mPowerStateChangedListener.onPowerStateChanged(event);
        }
        break;
      case DigitalPenEvent.TYPE_INTERNAL_DAEMON_ERROR:
        if (mInternalDaemonErrorListener != null) {
          mInternalDaemonErrorListener.onInternalDaemonError(event);
        }
        break;
      case DigitalPenEvent.TYPE_MIC_BLOCKED:
        if (mMicBlockStateChangedListener != null) {
          mMicBlockStateChangedListener.onMicBlockStateChanged(event);
        }
        break;
      default:
        break;
      }
    }
  };
  private IDataCallback mDataCb = new IDataCallback.Stub() {
    public void onDigitalPenPropData(DigitalPenData data) {
      if (mDigitalPenDataListener != null) {
        mDigitalPenDataListener.onDigitalPenData(data);
      }
    }
  };

  private ServiceConnection mConnection = new ServiceConnection() {
    /**
     * Called when the connection with the service is established
     */
    public void onServiceConnected(ComponentName className, IBinder service) {
      // Gets an instance of the IDigitalPen, which we can use to call on the service
      Log.d(TAG, "Connected to the service");
      mService = IDigitalPen.Stub.asInterface(service);
      try {
        // Register the data callback
        if (mDataCb != null) {
          mService.registerDataCallback(mDataCb);
        }
        // Register the event callback
        if (mEventCb != null) {
          mService.registerEventCallback(mEventCb);
        }
        if (!mService.isEnabled()) {
          // We don't want to continue if digital pen is not enabled
          return;
        }
        // Load the configuration
        if (mConfig != null) {
          mService.loadConfig(mConfig);
        }
      } catch (RemoteException e) {
        Log.e(TAG, "Remote service error: " + e.getMessage());
      }
    }

    /**
     * Called when the connection with the service disconnects unexpectedly
     */
    public void onServiceDisconnected(ComponentName className) {
      Log.e(TAG, "Fatal: Service disconnected");
      mService = null;
    }
  };

  /**
   * Constructs a new DigitalPenAdapter object from the given context.
   *
   * @param context The application's context.
   */
  public DigitalPenAdapter(Context context) {
    if (context == null) {
      throw new IllegalArgumentException("Context is null");
    }
    mContext = context;
    // Set configuration to null (no configuration will be loaded)
    mConfig  = null;
    Log.d(TAG, "Starting service");
    // Bind to the DigitalPen service
    mContext.bindService(new Intent(IDigitalPen.class.getName()),
                         mConnection,
                         Context.BIND_AUTO_CREATE);
  }

  /**
   * Checks whether DigitalPen framework is supported on this device.
   *
   * @return True if DigitalPen is supported on this device, false otherwise.
   */
  static public boolean isSupported() {
    String value = SystemProperties.get(DIGITAL_PEN_SYSTEM_PROP);
    return value.equals("1");
  }

  /**
   * Invokes <code>startService</code>, so that the service stays alives even
   * when no client is bound to it.
   * @hide
   */
  public void startService() {
    mContext.startService(new Intent(IDigitalPen.class.getName()));
  }

  /**
   * Invokes <code>stopService</code>, to stop the service after it has been
   * started by <code>startService</code>
   * @hide
   */
  public void stopService() {
    mContext.stopService(new Intent(IDigitalPen.class.getName()));
  }

  /**
    * Get the current power state of the DigitalPen.
   * <p>Possible return values are
   * {@link #STATE_ACTIVE},
   * {@link #STATE_STANDBY},
   * {@link #STATE_IDLE},
   * {@link #STATE_OFF}.
   * <p>Requires {@link android.Manifest.permission#DIGITALPEN}
   *
   * @return current state of DigitalPen
   * @hide
   */
  /*public int getPowerState() {
    if (null == mService) { // No connection establishted
      return 0;
    }
    try {
      return mService.getDigitalPenState();
    } catch (RemoteException e) {
      Log.e(TAG, "Digitalpen getState failed", e);
    }
    return 0;
  }*/

  /**
   * Checks whether DigitalPen framework is currently enabled and ready for use.
   *
   * @return true if the local adapter is turned on
   */
  public boolean isEnabled() {
    if (null == mService) { // No connection is established
      // isEnabled needs to a synchronic call, so when not connected to the
      // service, we check the flag.
      return !(SystemProperties.get("init.svc.usf_epos", "stopped")).
        equals("stopped");
    }
    try {
      return mService.isEnabled();
    } catch (RemoteException e) {
      Log.e(TAG, "Digitalpen isEnabled failed", e);
    }
    return false;
  }

  /**
   * Turn on the local DigitalPen framework
   * <p>This powers on the underlying DigitalPen framework.
   *
   * @return true to indicate framework startup has begun, or false on immediate
   * error
   */
  public boolean enable() {
    if (null == mService) { // No connection established
      return false;
    }
    try {
      return mService.enable();
    } catch (RemoteException e) {
      Log.e(TAG, "Digitalpen enable failed", e);
    }
    return false;
  }

  /**
   * Turn off the local DigitalPen framework.
   * <p>This powers off the underlying DigitalPen framework.
   *
   * @return true to indicate framework is stopping, or false on immediate error
   */
  public boolean disable() {
    if (null == mService) { // No connection established
      return false;
    }
    try {
      return mService.disable();
    } catch (RemoteException e) {
      Log.e(TAG, "Digitalpen disable failed", e);
    }
    return false;
  }

  /**
   * Load a new configuration in the DigitalPen framework.
   * <p>This updates the configuration of the underlying DigitalPen framework.
   *
   * <p>This is an asynchronous call: it will return immediately, after
   * sending a request to the framework to change the configuration it's
   * working with. If this call returns false then there was an immediate
   * problem that will prevent the framework from changing the configuration
   * it is currently working on.
   *
   * @param config The configuration to load.
   * @return true to indicate that a request was sent to the framework, or false
   * on immediate error.
   */
  public boolean loadConfig(DigitalPenConfig config) {
    if (null == mService) { // No connection established
      // mConfig will be loaded when the adapter gets connected to the service
      mConfig = config;
      return true;
    }
    try {
      return mService.loadConfig(config);
    } catch (RemoteException e) {
      Log.e(TAG, "Digitalpen loadConfig failed", e);
    } catch (Exception e) {} // hack
    return false;
  }

  /**
   * Unload the currently set configuration in the DigitalPen framework, this
   * results in changing the DigitalPen framework configuration to the default
   * configuration.
   * <p>This makes the underlying DigitalPen framework return to work with the
   *    default configuration.
   * <p>This is an asynchronous call: it will return immediately, after
   * sending a request to the framework to change the configuration it's
   * working with. If this call returns false then there was an immediate
   * problem that will prevent the framework from changing the configuration
   * it is currently working on.
   *
   * @return true to indicate that a request was sent to the framework, or false
   * on immediate error.
   */
  public boolean unloadConfig() {
    if (null == mService) { // No connection established
      return false;
    }
    try {
      return mService.unloadConfig();
    } catch (RemoteException e) {
      Log.e(TAG, "Digitalpen unloadConfig failed", e);
    }
    return false;
  }

  /**
   * Register a callback to be invoked when data is received from the digital pen
   *
   * @param l the digital pen data listener
   */
  public void setOnDigitalPenDataListener(OnDigitalPenDataListener l) {
    mDigitalPenDataListener = l;
  }

  /**
   * Unbind from the DigitalPen work service we are connected to.
   * <p>This makes the activity/service unbind from the DigitalPen service. The service
   * closes the daemon when no more clients are connected.
   */
  public void unbind() {
    if (null == mService) { // No connection established
      return;
    }
    try {
      if (mDataCb != null)
        mService.unregisterDataCallback(mDataCb);
      if (mEventCb != null)
        mService.unregisterEventCallback(mEventCb);
    } catch (RemoteException e) {
      Log.e(TAG, "unbind", e);
    }
    mContext.unbindService(mConnection);
  }

  /**
   * Register a callback to be invoked when the power state of the digital pen
   * has changed.
   *
   * @param l the power state changed listener
   */
  public void setOnPowerStateChangedListener(OnPowerStateChangedListener l) {
    mPowerStateChangedListener = l;
  }

  /**
   * Register a callback to be invoked when an error occurs with the internal
   * daemon of the digital pen.
   *
   * @param l the internal daemon error listener
   */
  public void setOnInternalDaemonErrorListener(OnInternalDaemonErrorListener l) {
    mInternalDaemonErrorListener = l;
  }

  /**
   * Register a callback to be invoked when the digital pen daemon reports that
   * a microphone got blocked/unblocked.
   *
   * @param l the mic block state changed listener
   */
  public void setOnMicBlockStateChangedListener(OnMicBlockStateChangedListener l) {
    mMicBlockStateChangedListener = l;
  }

  /**
   * Interface definition for a callback to be invoked when the power state of
   * the digital pen has changed.
   */
  public interface OnPowerStateChangedListener {
      /**
       * Called when the power state of the digital pen has changed.
       *
       * @param event The DigitalPenEvent object containing full information
       *        about the event
       */
      void onPowerStateChanged(DigitalPenEvent event);
  }

  /**
   * Interface definition for a callback to be invoked when an error occurs with
   * the internal daemon of the digital pen.
   */
  public interface OnInternalDaemonErrorListener {
      /**
       * Called when an error occurs with the internal daemon of the digital
       * pen.
       *
       * @param event The DigitalPenEvent object containing full information
       *        about the event
       */
      void onInternalDaemonError(DigitalPenEvent event);
  }

  /**
   * Interface definition for a callback to be invoked when the digital pen
   * daemon reports that a microphone got blocked/unblocked.
   */
  public interface OnMicBlockStateChangedListener {
      /**
       * Called when the digital pen daemon reports that a microphone got
       * blocked/unblocked.
       *
       * @param event The DigitalPenEvent object containing full information
       *        about the event
       */
      void onMicBlockStateChanged(DigitalPenEvent event);
  }

  /**
   * Interface definition for a callback to be invoked when data is recevied
   * from the digital pen
   */
  public interface OnDigitalPenDataListener {
    /**
     * Called when data is received from the digital pen, this occurs when the
     * digital pen moves inside a defined drawing surface area.
     *
     * @param data The DigitalPenData object containing full information
     *        about the data
     */
    void onDigitalPenData(DigitalPenData data);
  }
}
