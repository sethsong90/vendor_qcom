/*=============================================================================

    ds_fmc_app_sm_int.c

Description:
  This file contains the machine generated source file for the state machine
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


/* Include STM compiler generated external and internal header files */
#include "ds_fmc_app_sm_ext.h"
#include "ds_fmc_app_sm_int.h"

/* Include INPUT_DEF_FILE specified files */
#include <ds_fmc_app_sm.h>

/* Begin machine generated internal source for state machine array: DS_FMC_APP_SM[] */

#ifndef STM_DATA_STRUCTURES_ONLY
/* Transition table */
static const stm_transition_fn_t
  DS_FMC_APP_SM_transitions[ DS_FMC_APP_SM_NUM_STATES * DS_FMC_APP_SM_NUM_INPUTS ] =
{
  /* Transition functions for state DS_FMC_APP_STATE_DOWN */
  ds_fmc_app_sm_handle_ext_trig_enable,    /* DS_FMC_APP_EXT_TRIG_ENABLE_EV */
  NULL,    /* DS_FMC_APP_TUNNEL_OPENED_EV */
  NULL,    /* DS_FMC_APP_TUNNEL_CLOSED_EV */
  NULL,    /* DS_FMC_APP_EXT_TRIG_DISABLE_EV */
  NULL,    /* DS_FMC_APP_BEARER_UP_EV */
  NULL,    /* DS_FMC_APP_BEARER_DOWN_EV */

  /* Transition functions for state DS_FMC_APP_STATE_ENABLING_TUNNEL */
  NULL,    /* DS_FMC_APP_EXT_TRIG_ENABLE_EV */
  ds_fmc_app_sm_tunnel_opened,    /* DS_FMC_APP_TUNNEL_OPENED_EV */
  ds_fmc_app_sm_tunnel_closed,    /* DS_FMC_APP_TUNNEL_CLOSED_EV */
  ds_fmc_app_sm_handle_ext_trig_disable,    /* DS_FMC_APP_EXT_TRIG_DISABLE_EV */
  ds_fmc_app_sm_bearer_up,    /* DS_FMC_APP_BEARER_UP_EV */
  NULL,    /* DS_FMC_APP_BEARER_DOWN_EV */

  /* Transition functions for state DS_FMC_APP_STATE_CONFIGURING_TUNNEL_PARAMS */
  NULL,    /* DS_FMC_APP_EXT_TRIG_ENABLE_EV */
  NULL,    /* DS_FMC_APP_TUNNEL_OPENED_EV */
  ds_fmc_app_sm_tunnel_closed,    /* DS_FMC_APP_TUNNEL_CLOSED_EV */
  ds_fmc_app_sm_handle_ext_trig_disable,    /* DS_FMC_APP_EXT_TRIG_DISABLE_EV */
  ds_fmc_app_sm_bearer_up,    /* DS_FMC_APP_BEARER_UP_EV */
  ds_fmc_app_sm_bearer_down,    /* DS_FMC_APP_BEARER_DOWN_EV */

  /* Transition functions for state DS_FMC_APP_STATE_UP */
  NULL,    /* DS_FMC_APP_EXT_TRIG_ENABLE_EV */
  NULL,    /* DS_FMC_APP_TUNNEL_OPENED_EV */
  ds_fmc_app_sm_tunnel_closed,    /* DS_FMC_APP_TUNNEL_CLOSED_EV */
  ds_fmc_app_sm_handle_ext_trig_disable,    /* DS_FMC_APP_EXT_TRIG_DISABLE_EV */
  NULL,    /* DS_FMC_APP_BEARER_UP_EV */
  ds_fmc_app_sm_bearer_down,    /* DS_FMC_APP_BEARER_DOWN_EV */

  /* Transition functions for state DS_FMC_APP_STATE_DISABLING_TUNNEL */
  NULL,    /* DS_FMC_APP_EXT_TRIG_ENABLE_EV */
  NULL,    /* DS_FMC_APP_TUNNEL_OPENED_EV */
  ds_fmc_app_sm_tunnel_closed,    /* DS_FMC_APP_TUNNEL_CLOSED_EV */
  NULL,    /* DS_FMC_APP_EXT_TRIG_DISABLE_EV */
  NULL,    /* DS_FMC_APP_BEARER_UP_EV */
  NULL,    /* DS_FMC_APP_BEARER_DOWN_EV */

  /* Transition functions for state DS_FMC_APP_STATE_GOING_DOWN */
  NULL,    /* DS_FMC_APP_EXT_TRIG_ENABLE_EV */
  NULL,    /* DS_FMC_APP_TUNNEL_OPENED_EV */
  NULL,    /* DS_FMC_APP_TUNNEL_CLOSED_EV */
  NULL,    /* DS_FMC_APP_EXT_TRIG_DISABLE_EV */
  NULL,    /* DS_FMC_APP_BEARER_UP_EV */
  ds_fmc_app_sm_bearer_down_complete,    /* DS_FMC_APP_BEARER_DOWN_EV */

};
#endif /* !STM_DATA_STRUCTURES_ONLY */

/* State { name, entry, exit, child SM } table */
static const stm_state_map_t
  DS_FMC_APP_SM_states[ DS_FMC_APP_SM_NUM_STATES ] =
{
  {"DS_FMC_APP_STATE_DOWN",
#ifndef STM_DATA_STRUCTURES_ONLY
    ds_fmc_app_sm_state_down_entry, ds_fmc_app_sm_state_down_exit,
#else /* STM_DATA_STRUCTURES_ONLY */
    NULL, NULL,
#endif /* STM_DATA_STRUCTURES_ONLY */
    NULL},
  {"DS_FMC_APP_STATE_ENABLING_TUNNEL",
#ifndef STM_DATA_STRUCTURES_ONLY
    ds_fmc_app_sm_state_enabling_tunnel_mgr_entry, ds_fmc_app_sm_state_enabling_tunnel_mgr_exit,
#else /* STM_DATA_STRUCTURES_ONLY */
    NULL, NULL,
#endif /* STM_DATA_STRUCTURES_ONLY */
    NULL},
  {"DS_FMC_APP_STATE_CONFIGURING_TUNNEL_PARAMS",
#ifndef STM_DATA_STRUCTURES_ONLY
    ds_fmc_app_sm_state_configuring_tunnel_params_entry, ds_fmc_app_sm_state_configuring_tunnel_params_exit,
#else /* STM_DATA_STRUCTURES_ONLY */
    NULL, NULL,
#endif /* STM_DATA_STRUCTURES_ONLY */
    NULL},
  {"DS_FMC_APP_STATE_UP",
#ifndef STM_DATA_STRUCTURES_ONLY
    ds_fmc_app_sm_state_up_entry, ds_fmc_app_sm_state_up_exit,
#else /* STM_DATA_STRUCTURES_ONLY */
    NULL, NULL,
#endif /* STM_DATA_STRUCTURES_ONLY */
    NULL},
  {"DS_FMC_APP_STATE_DISABLING_TUNNEL",
#ifndef STM_DATA_STRUCTURES_ONLY
    ds_fmc_app_sm_state_disabling_tunnel_mgr_entry, ds_fmc_app_sm_state_disabling_tunnel_mgr_exit,
#else /* STM_DATA_STRUCTURES_ONLY */
    NULL, NULL,
#endif /* STM_DATA_STRUCTURES_ONLY */
    NULL},
  {"DS_FMC_APP_STATE_GOING_DOWN",
#ifndef STM_DATA_STRUCTURES_ONLY
    ds_fmc_app_sm_state_going_down_entry, ds_fmc_app_sm_state_going_down_exit,
#else /* STM_DATA_STRUCTURES_ONLY */
    NULL, NULL,
#endif /* STM_DATA_STRUCTURES_ONLY */
    NULL},
};

/* Input { name, value } table */
static const stm_input_map_t
  DS_FMC_APP_SM_inputs[ DS_FMC_APP_SM_NUM_INPUTS ] =
{
  { "DS_FMC_APP_EXT_TRIG_ENABLE_EV" , (stm_input_t) DS_FMC_APP_EXT_TRIG_ENABLE_EV },
  { "DS_FMC_APP_TUNNEL_OPENED_EV" , (stm_input_t) DS_FMC_APP_TUNNEL_OPENED_EV },
  { "DS_FMC_APP_TUNNEL_CLOSED_EV" , (stm_input_t) DS_FMC_APP_TUNNEL_CLOSED_EV },
  { "DS_FMC_APP_EXT_TRIG_DISABLE_EV" , (stm_input_t) DS_FMC_APP_EXT_TRIG_DISABLE_EV },
  { "DS_FMC_APP_BEARER_UP_EV" , (stm_input_t) DS_FMC_APP_BEARER_UP_EV },
  { "DS_FMC_APP_BEARER_DOWN_EV" , (stm_input_t) DS_FMC_APP_BEARER_DOWN_EV },
};


/* Constant all-instance state machine data */
static const stm_state_machine_constdata_t DS_FMC_APP_SM_constdata =
{
  DS_FMC_APP_SM_NUM_INSTANCES, /* number of state machine instances */
  DS_FMC_APP_SM_NUM_STATES, /* number of states */
  DS_FMC_APP_SM_states, /* array of state mappings */
  DS_FMC_APP_SM_NUM_INPUTS, /* number of inputs */
  DS_FMC_APP_SM_inputs, /* array of input mappings */
#ifndef STM_DATA_STRUCTURES_ONLY
  DS_FMC_APP_SM_transitions, /* array of transition function mappings */
  ds_fmc_app_sm_entry, /* state machine entry function */
  ds_fmc_app_sm_exit, /* state machine exit function */
  ds_fmc_app_sm_error_hook, /* state machine error hook function */
  ds_fmc_app_sm_debug_hook, /* state machine debug hook function */
  DS_FMC_APP_STATE_DOWN /* state machine initial state */
#else /* STM_DATA_STRUCTURES_ONLY */
  NULL, /* array of transition function mappings */
  NULL, /* state machine entry function */
  NULL, /* state machine exit function */
  NULL, /* state machine error hook function */
  NULL, /* state machine debug hook function */
  0 /* state machine initial state */
#endif /* STM_DATA_STRUCTURES_ONLY */
};

/* Constant per-instance state machine data */
static const stm_state_machine_perinst_constdata_t
  DS_FMC_APP_SM_perinst_constdata[ DS_FMC_APP_SM_NUM_INSTANCES ] =
{
  {
    &DS_FMC_APP_SM_constdata, /* state machine constant data */
    "DS_FMC_APP_SM", /* state machine name */
    0x9d701258, /* state machine unique ID (md5("DS_FMC_APP_SM") & 0xFFFFFFFF) */
    0  /* this state machine instance */
  },
};

/* State machine instance array definition */
stm_state_machine_t
  DS_FMC_APP_SM[ DS_FMC_APP_SM_NUM_INSTANCES ] =
{
  {
    &DS_FMC_APP_SM_perinst_constdata[ 0 ], /* per instance constant data array */
    STM_DEACTIVATED_STATE, /* current state */
    -1, /* current input index */
    NULL, /* calling SM instance */
    FALSE, /* propagate input to parent */
    FALSE, /* locked flag */
    NULL, /* user defined per-instance data */
    0, /* user defined debug mask */
  },
};

#ifndef STM_DATA_STRUCTURES_ONLY
/* User called 'reset' routine.  Should never be needed, but can be used to
   effect a complete reset of all a given state machine's instances. */
void DS_FMC_APP_SM_reset(void)
{
  uint32 idx;
  void **tricky;

  /* Reset all the child SMs (if any) */
  

  /* Reset the parent */
  for( idx = 0; idx < DS_FMC_APP_SM_NUM_INSTANCES; idx++)
  {
    tricky = (void **)&DS_FMC_APP_SM[ idx ].pi_const_data; /* sleight of hand to assign to const ptr below */
    *tricky = (void *)&DS_FMC_APP_SM_perinst_constdata[ idx ]; /* per instance constant data array */
    DS_FMC_APP_SM[ idx ].current_state = STM_DEACTIVATED_STATE; /* current state */
    DS_FMC_APP_SM[ idx ].curr_input_index = -1; /* current input index */
    DS_FMC_APP_SM[ idx ].propagate_input = FALSE; /* propagate input to parent */
    DS_FMC_APP_SM[ idx ].is_locked = FALSE; /* locked flag */
    DS_FMC_APP_SM[ idx ].user_data = NULL; /* user defined per-instance data */
    DS_FMC_APP_SM[ idx ].debug_mask = 0; /* user defined debug mask */
  }

}
#endif /* !STM_DATA_STRUCTURES_ONLY */

/* End machine generated internal source for state machine array: DS_FMC_APP_SM[] */


