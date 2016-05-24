/*
 * Copyright(c) 2013 Qualcomm Technologies, Inc.All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#pragma once

#ifdef __cplusplus
  extern "C" {
#endif

/**
 * Disable function for current module. After this function returns the module
 * is supposed not to access any protected resource anymore.
 *
 * @param efd [out] eventfd associated with this module, for asynchronous
 *                  notifications. The module can signal this event to request
 *                  to be re-enabled.
 *                  A module can set it to a negative value if it does not
 *                  need to be re-enabled asynchronously.
 *
 * @return 0     on success.
 *
 */
typedef int (*stDisableChannel_t)(int * efd);

/**
 * Re-enable the channel
 * This function MUST be called AFTER any resource locked in TrustZone has been
 * released. After this call the subsystems are free to access their HW resources.
 *
 * param efd [in]  eventfd associated with the module. It is responsibility of
 *                 the module to close() the fd. When this call is made, the main
 *                 library is guaranteed not to be waiting on the event anymore.
 * @returns 0      on success.
 */
typedef int (*stEnableChannel_t)(int efd);

struct SideChannelModule {
  stDisableChannel_t const disableChannel;  /**< Disable function */
  stEnableChannel_t const enableChannel;    /**< Enable function */
  char const * const name;                  /**< Module's name, used for logging purposes '*/
};

#ifdef __cplusplus
  }
#endif
