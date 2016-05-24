/*
 * Copyright(c) 2013 Qualcomm Technologies, Inc.All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#pragma once

#ifdef __cplusplus
  extern "C" {
#endif

/**
 * Disable all registered subsystems that can be used to perform side channels
 * attacks on the Secure Touch.
 * This function MUST be called BEFORE any resource locking has taken place in
 * TrustZone. Failing to do so will cause device instability.
 *
 *
 * @returns 0     on success.
 * @returns ENODEV  if the Side Channels library failed to initialize
 * @returns EIO     if it fails to lock the internal mutex
 * @returns EBUSY   if channels are already disabled
 * @returns EINVAL  If a module is misconfigured
 *
 */
int stDisableChannels(void);

/**
 * Re-enable all registsred subsystems.
 * This function MUST be called AFTER any resource locked in TrustZone has been
 * released. After this call the subsystems are free to access their HW resources.
 *
 * @returns 0      on success.
 * @returns ENODEV  if the Side Channels library failed to initialize
 * @returns EIO     if it fails to lock the internal mutex or to wake up the
 *                  waiting thread
 */
int stEnableChannels(void);

/**
 * Wait for any event from any of the registsred subsystems.
 * This is a blocking call.
 * Subsystems are going to use this function to communicate to the controlling
 * layer that an event is waiting to be processed, and the processing requires
 * the current secure touch session to be aborted.
 *
 * @returns 0       if a signal was received from a channel
 * @returns ENODEV  if the Side Channels library failed to initialize
 * @returns EBADF   if the channels are enabled already
 * @returns EIO     if it fails to lock the internal mutex
 */
int stWaitForChannelEvent(void);

#ifdef __cplusplus
  }
#endif
