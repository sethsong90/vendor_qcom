/*=============================================================================

    ds_fmc_app_sm_int.h

Description:
  This file contains the machine generated header file for the state machine
  specified in the file:
  ./ds_fmc_app_sm.stm

=============================================================================*/

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


#ifndef DS_FMC_APP_SM_INT_H
#define DS_FMC_APP_SM_INT_H

#ifdef __cplusplus
/* If compiled into a C++ file, ensure symbols names are not mangled */
extern "C"
{
#endif

/* Include external state machine header */
#include "ds_fmc_app_sm_ext.h"

/* Begin machine generated internal header for state machine array: DS_FMC_APP_SM[] */

/* Suppress Lint suggestions to const-ify state machine and payload ptrs */
/*lint -esym(818,sm,payload) */

/* Define a macro for the number of SM instances */
#define DS_FMC_APP_SM_NUM_INSTANCES 1

/* Define a macro for the number of SM states */
#define DS_FMC_APP_SM_NUM_STATES 6

/* Define a macro for the number of SM inputs */
#define DS_FMC_APP_SM_NUM_INPUTS 6

#ifndef STM_DATA_STRUCTURES_ONLY
/* State Machine entry/exit function prototypes */
void ds_fmc_app_sm_entry(stm_state_machine_t *sm,void *payload);
void ds_fmc_app_sm_exit(stm_state_machine_t *sm,void *payload);


/* State entry/exit function prototypes */
void ds_fmc_app_sm_state_down_entry(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void ds_fmc_app_sm_state_down_exit(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void ds_fmc_app_sm_state_enabling_tunnel_mgr_entry(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void ds_fmc_app_sm_state_enabling_tunnel_mgr_exit(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void ds_fmc_app_sm_state_configuring_tunnel_params_entry(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void ds_fmc_app_sm_state_configuring_tunnel_params_exit(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void ds_fmc_app_sm_state_up_entry(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void ds_fmc_app_sm_state_up_exit(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void ds_fmc_app_sm_state_disabling_tunnel_mgr_entry(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void ds_fmc_app_sm_state_disabling_tunnel_mgr_exit(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void ds_fmc_app_sm_state_going_down_entry(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void ds_fmc_app_sm_state_going_down_exit(stm_state_machine_t *sm,stm_state_t _state,void *payload);


/* Transition function prototypes */
stm_state_t ds_fmc_app_sm_handle_ext_trig_enable(stm_state_machine_t *sm, void *payload);
stm_state_t ds_fmc_app_sm_tunnel_opened(stm_state_machine_t *sm, void *payload);
stm_state_t ds_fmc_app_sm_bearer_down_complete(stm_state_machine_t *sm, void *payload);
stm_state_t ds_fmc_app_sm_handle_ext_trig_disable(stm_state_machine_t *sm, void *payload);
stm_state_t ds_fmc_app_sm_bearer_up(stm_state_machine_t *sm, void *payload);
stm_state_t ds_fmc_app_sm_bearer_down(stm_state_machine_t *sm, void *payload);
stm_state_t ds_fmc_app_sm_tunnel_closed(stm_state_machine_t *sm, void *payload);


/* State enumeration */
enum
{
  DS_FMC_APP_STATE_DOWN,
  DS_FMC_APP_STATE_ENABLING_TUNNEL,
  DS_FMC_APP_STATE_CONFIGURING_TUNNEL_PARAMS,
  DS_FMC_APP_STATE_UP,
  DS_FMC_APP_STATE_DISABLING_TUNNEL,
  DS_FMC_APP_STATE_GOING_DOWN,
};

#endif /* !STM_DATA_STRUCTURES_ONLY */

/* End machine generated internal header for state machine array: DS_FMC_APP_SM[] */


#ifdef __cplusplus
} /* extern "C" {...} */
#endif

#endif /* ! DS_FMC_APP_SM_INT_H */
