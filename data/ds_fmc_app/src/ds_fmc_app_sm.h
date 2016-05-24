/******************************************************************************

                   D S _ F M C _ A P P _ S M . H

******************************************************************************/

/******************************************************************************

  @file    ds_fmc_app_sm.h
  @brief   DS_FMC_APP state machine header file

  DESCRIPTION
  Header file containing definition of DS_FMC_APP's state machine.

******************************************************************************/
/*===========================================================================

  Copyright (c) 2010 Qualcomm Technologies, Inc. All Rights Reserved

  Qualcomm Technologies Proprietary

  Export of this technology or software is regulated by the U.S. Government.
  Diversion contrary to U.S. law prohibited.

  All ideas, data and information contained in or disclosed by
  this document are confidential and proprietary information of
  Qualcomm Technologies, Inc. and all rights therein are expressly reserved.
  By accepting this material the recipient agrees that this material
  and the information contained therein are held in confidence and in
  trust and will not be used, copied, reproduced in whole or in part,
  nor its contents revealed in any manner to others without the express
  written permission of Qualcomm Technologies, Inc.

===========================================================================*/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id:$

when       who        what, where, why
--------   ---        -------------------------------------------------------
05/11/10   scb        Initial version

******************************************************************************/

#ifndef __DS_FMC_APP_SM_H__
#define __DS_FMC_APP_SM_H__

#include "ds_fmc_app_sm_int.h"

/*===========================================================================
                     GLOBAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

typedef enum ds_fmc_app_sm_events_e {
  DS_FMC_APP_INVALID_EV = -1,              /* Invalid value              */
  DS_FMC_APP_EXT_TRIG_DISABLE_EV,          /* External triggered disable */
  DS_FMC_APP_EXT_TRIG_ENABLE_EV,           /* External triggered enable  */
  DS_FMC_APP_TUNNEL_OPENED_EV,             /* Tunnel opened event        */
  DS_FMC_APP_TUNNEL_CLOSED_EV,             /* Tunnel closed event        */
  DS_FMC_APP_BEARER_UP_EV,                 /* Bearer up indication       */
  DS_FMC_APP_BEARER_DOWN_EV,               /* Bearer down indication     */
} ds_fmc_app_sm_events_t;


/*===========================================================================
                            GLOBAL FUNCTION DECLARATIONS
===========================================================================*/

/*===========================================================================

  STATE MACHINE ERROR HOOK FUNCTION: ds_fmc_app_sm_error_hook

===========================================================================*/
void ds_fmc_app_sm_error_hook
(
  stm_status_t     error,
  const char      *filename,
  uint32           line,
  struct stm_state_machine_s *sm
);


/*===========================================================================

  STATE MACHINE DEBUG HOOK FUNCTION:  ds_fmc_app_sm_debug_hook

===========================================================================*/
void ds_fmc_app_sm_debug_hook
(
  stm_debug_event_t debug_event,
  struct stm_state_machine_s *sm,
  stm_state_t state_info,
  void *payload
);

#endif /* __DS_FMC_APP_SM_H__ */
