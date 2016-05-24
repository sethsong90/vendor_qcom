#ifndef SEC_UI_H_
#define SEC_UI_H_

/* @file SecureUI.h
 * @brief
 * This file contains the interfaces to abort the Secure User Interface services
 */

/*===========================================================================
 * Copyright(c) 2013 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

when       who     what, where, why
--------   ---     ----------------------------------------------------------
05/13/13   sn      Initial Version.

===========================================================================*/

#ifdef __cplusplus
  extern "C" {
#endif

/**
  @brief Stop secure display
  This function will trigger tearing down of the
  secure display service, including freeing buffers,
  unlocking, enabling overlays, etc.

  @return
  None.

  @dependencies
  None.

  @sideeffects
  None.
*/

void abort_secure_ui();

#ifdef __cplusplus
  }
#endif


#endif /*SEC_UI_H_*/
