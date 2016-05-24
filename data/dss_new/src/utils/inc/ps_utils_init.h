#ifndef PS_UTILS_INIT_H
#define PS_UTILS_INIT_H

/*=========================================================================*/
/*!
  @file 
  ps_utils_init.h

  @brief
  This file provides functions that are used to perform initializations 
  of UTILS modules.

  @detail
  This file provides methods to initialize the utils module. For AMSS
  environments the PS task initialization module calls these methods. 
  For 3rd party platforms, these methods need to be called by the platform
  specific initialization module before using utils.
 
  Exported functions:
  ps_utils_powerup() 
  Initialization of UTILS libraries that happen before task start is
  signalled to the task main controller.

  ps_utils_init()
  Initialization of UTILS that require other modules/tasks to be 
  initialized. This function gets called after the caller's task start is 
  signalled.

  @note
  Both these functions can be called at-most once.

  @note
  Utils module initializes PS critical sections, timers, PS mem etc, that
  may be used by other modules during thier initialization. It is highly
  recommended that Utils initialations happen before other modules are 
  intialized.

  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*=========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/inc/ps_utils_init.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2009-07-14 hm  Created module.

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
  @brief
  Provides powerup initialization of UTILS modules.

  @detail
  This function initializes the UTILS modules during task powerup.
  The initializations that happen here are not dependent upon any other
  tasks being initialized. After this function UTILS modules are in
  ready state to recieve requests from other tasks.

  @note
  Utils module initializes PS critical sections, timers, PS mem etc, that
  may be used by other modules during thier initialization. It is highly
  recommended that Utils initialations happen before other modules are 
  intialized.

  @param      None.
  @see        psi_powerup() for reference task initializing this module.
  @see        psi_init()
  @return     None.
*/
void ps_utils_powerup
(
  void
);

/*!
  @brief
  Provides powerdown cleanup of UTILS modules.

  @detail
  This function cleans up the UTILS modules

  @note
  
  @param      None.
  @see        ps_utils_powerup()
  @return     None.
*/
void ps_utils_powerdown
(
  void
);

/*!
  @brief
  This function performs power-after initialization of UTILS moudules.

  @detail
  This function initializes the UTILS modules after the task-start 
  has been signalled. This means that we can perform initialization of 
  UTILS that are dependent upon other tasks.

  @param      None.
  @see        psi_powerup() for reference task initializing this module.
  @see        psi_init()
  @return     None.
*/
void ps_utils_init
(
  void
);

/*!
  @brief
  This function performs powerdown-before cleanup of UTILS moudules.

  @detail
  This function cleans up the UTILS modules before the task-end 
  has been signalled.

  @param      None.
  @see        ps_utils_init()
  @return     None.
*/
void ps_utils_deinit
(
  void
);

#ifdef __cplusplus
}
#endif

#endif /* PS_UTILS_INIT_H */



