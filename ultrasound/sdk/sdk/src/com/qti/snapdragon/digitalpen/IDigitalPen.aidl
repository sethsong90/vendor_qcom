/*===========================================================================
                           IDigitalPen.aidl

Copyright (c) 2012-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
=============================================================================*/
package com.qti.snapdragon.digitalpen;

import android.os.Bundle;
import com.qti.snapdragon.digitalpen.util.DigitalPenConfig;
import com.qti.snapdragon.digitalpen.IDataCallback;
import com.qti.snapdragon.digitalpen.IEventCallback;
/*
 * IDigitalPen interface:
 *   This interface defines functions to communicate with the DigitalPen
 *   framework service.
 */
interface IDigitalPen {
  /*
   * Enable DigitalPen framework
   */
  boolean enable();
  /*
   * Disable DigitalPen framework
   */
  boolean disable();
  /*
   * Changes the default configuration of the DigitalPen framework
   */
  boolean setDefaultConfig(in DigitalPenConfig config);
  /*
   * Checks whether the framework is enabled or not
   */
  boolean isEnabled();
  /*
   * Returns the state of the DigitalPen framework
   */
  int getDigitalPenState();
  /*
   * Load the configuration specified into the DigitalPen framework
   */
  boolean loadConfig(in DigitalPenConfig config);
  /*
   * Unloads the currently set configuration (gets the framework back to the
   * default configuration
   */
  boolean unloadConfig();
  /*
   * Registers a callback function for proprietary data
   */
  boolean registerDataCallback(IDataCallback cb);
  /*
   * Registers a callback function for proprietary event
   */
  boolean registerEventCallback(IEventCallback cb);
  /*
   * Unregisters a registered callback function for proprietary data
   */
  boolean unregisterDataCallback(IDataCallback cb);
  /*
   * Unregisters a registered callback function for proprietary event
   */
  boolean unregisterEventCallback(IEventCallback cb);

  /*
   * APPLICATION INTERFACE METHODS
   */

  boolean applyConfig(in Bundle config);
  boolean releaseActivity();
  boolean setOnScreenDataCallback(IDataCallback cb);
}
