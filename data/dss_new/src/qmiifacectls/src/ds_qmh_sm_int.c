/*=============================================================================

    ds_qmh_sm_int.c

Description:
  This file contains the machine generated source file for the state machine
  specified in the file:
  ./ds_qmh_sm.stm

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
#include "ds_qmh_sm_ext.h"
#include "ds_qmh_sm_int.h"

/* Include INPUT_DEF_FILE specified files */
#include "ds_qmhi.h"

/* Begin machine generated internal source for state machine array: DSQMH_SM[] */

#ifndef STM_DATA_STRUCTURES_ONLY
/* Transition table */
static const stm_transition_fn_t
  DSQMH_SM_transitions[ DSQMH_SM_NUM_STATES * DSQMH_SM_NUM_INPUTS ] =
{
  /* Transition functions for state DSPROXY_IFACE_STATE_DISABLED */
  dsqmhsm_modem_init_ind_hdlr,    /* PROXY_IFACE_MODEM_INIT_IND */
  NULL,    /* PROXY_IFACE_BRING_UP_CMD */
  NULL,    /* PROXY_IFACE_TEARDOWN_CMD */
  NULL,    /* PROXY_IFACE_MODEM_QOS_IND */
  NULL,    /* PROXY_IFACE_MODEM_EVENT_IND */
  NULL,    /* PROXY_PHYS_LINK_UP_CMD */
  NULL,    /* PROXY_PHYS_LINK_DOWN_CMD */
  NULL,    /* PROXY_IFACE_MODEM_UP_IND */
  NULL,    /* PROXY_IFACE_MODEM_DOWN_IND */
  NULL,    /* PROXY_IFACE_PLATFORM_UP_IND */
  NULL,    /* PROXY_IFACE_PLATFORM_DOWN_IND */
  NULL,    /* PROXY_IFACE_CONFIGURED_IND */
  NULL,    /* PROXY_IFACE_MODEM_INTERNAL_IND */
  NULL,    /* PROXY_IFACE_MODEM_MCAST_IND */
  NULL,    /* PROXY_IFACE_MODEM_BCMCS_IND */
  NULL,    /* PROXY_IFACE_MODEM_MTREQ_IND */
  NULL,    /* PROXY_IFACE_PLATFORM_QOS_IND */

  /* Transition functions for state DSPROXY_IFACE_STATE_DOWN */
  NULL,    /* PROXY_IFACE_MODEM_INIT_IND */
  dsqmhsm_iface_bringup_hdlr,    /* PROXY_IFACE_BRING_UP_CMD */
  dsqmhsm_iface_teardown_hdlr,    /* PROXY_IFACE_TEARDOWN_CMD */
  dsqmhsm_modem_qos_ind_hdlr,    /* PROXY_IFACE_MODEM_QOS_IND */
  dsqmhsm_modem_event_ind_hdlr,    /* PROXY_IFACE_MODEM_EVENT_IND */
  NULL,    /* PROXY_PHYS_LINK_UP_CMD */
  NULL,    /* PROXY_PHYS_LINK_DOWN_CMD */
  NULL,    /* PROXY_IFACE_MODEM_UP_IND */
  NULL,    /* PROXY_IFACE_MODEM_DOWN_IND */
  NULL,    /* PROXY_IFACE_PLATFORM_UP_IND */
  NULL,    /* PROXY_IFACE_PLATFORM_DOWN_IND */
  NULL,    /* PROXY_IFACE_CONFIGURED_IND */
  NULL,    /* PROXY_IFACE_MODEM_INTERNAL_IND */
  NULL,    /* PROXY_IFACE_MODEM_MCAST_IND */
  NULL,    /* PROXY_IFACE_MODEM_BCMCS_IND */
  NULL,    /* PROXY_IFACE_MODEM_MTREQ_IND */
  NULL,    /* PROXY_IFACE_PLATFORM_QOS_IND */

  /* Transition functions for state DSPROXY_IFACE_STATE_COMING_UP */
  NULL,    /* PROXY_IFACE_MODEM_INIT_IND */
  NULL,    /* PROXY_IFACE_BRING_UP_CMD */
  dsqmhsm_iface_teardown_hdlr,    /* PROXY_IFACE_TEARDOWN_CMD */
  dsqmhsm_modem_qos_ind_hdlr,    /* PROXY_IFACE_MODEM_QOS_IND */
  dsqmhsm_modem_event_ind_hdlr,    /* PROXY_IFACE_MODEM_EVENT_IND */
  dsqmhsm_physlink_bringup_hdlr,    /* PROXY_PHYS_LINK_UP_CMD */
  dsqmhsm_physlink_teardown_hdlr,    /* PROXY_PHYS_LINK_DOWN_CMD */
  dsqmhsm_modem_up_ind_hdlr,    /* PROXY_IFACE_MODEM_UP_IND */
  dsqmhsm_modem_down_ind_hdlr,    /* PROXY_IFACE_MODEM_DOWN_IND */
  NULL,    /* PROXY_IFACE_PLATFORM_UP_IND */
  NULL,    /* PROXY_IFACE_PLATFORM_DOWN_IND */
  NULL,    /* PROXY_IFACE_CONFIGURED_IND */
  NULL,    /* PROXY_IFACE_MODEM_INTERNAL_IND */
  NULL,    /* PROXY_IFACE_MODEM_MCAST_IND */
  NULL,    /* PROXY_IFACE_MODEM_BCMCS_IND */
  NULL,    /* PROXY_IFACE_MODEM_MTREQ_IND */
  NULL,    /* PROXY_IFACE_PLATFORM_QOS_IND */

  /* Transition functions for state DSPROXY_IFACE_STATE_PLAT_COMING_UP */
  NULL,    /* PROXY_IFACE_MODEM_INIT_IND */
  NULL,    /* PROXY_IFACE_BRING_UP_CMD */
  dsqmhsm_iface_teardown_hdlr,    /* PROXY_IFACE_TEARDOWN_CMD */
  dsqmhsm_modem_qos_ind_hdlr,    /* PROXY_IFACE_MODEM_QOS_IND */
  dsqmhsm_modem_event_ind_hdlr,    /* PROXY_IFACE_MODEM_EVENT_IND */
  NULL,    /* PROXY_PHYS_LINK_UP_CMD */
  dsqmhsm_physlink_teardown_hdlr,    /* PROXY_PHYS_LINK_DOWN_CMD */
  NULL,    /* PROXY_IFACE_MODEM_UP_IND */
  dsqmhsm_modem_down_ind_hdlr,    /* PROXY_IFACE_MODEM_DOWN_IND */
  dsqmhsm_platform_up_ind_hdlr,    /* PROXY_IFACE_PLATFORM_UP_IND */
  dsqmhsm_platform_down_ind_hdlr,    /* PROXY_IFACE_PLATFORM_DOWN_IND */
  NULL,    /* PROXY_IFACE_CONFIGURED_IND */
  NULL,    /* PROXY_IFACE_MODEM_INTERNAL_IND */
  NULL,    /* PROXY_IFACE_MODEM_MCAST_IND */
  NULL,    /* PROXY_IFACE_MODEM_BCMCS_IND */
  NULL,    /* PROXY_IFACE_MODEM_MTREQ_IND */
  NULL,    /* PROXY_IFACE_PLATFORM_QOS_IND */

  /* Transition functions for state DSPROXY_IFACE_STATE_CONFIGURING */
  NULL,    /* PROXY_IFACE_MODEM_INIT_IND */
  NULL,    /* PROXY_IFACE_BRING_UP_CMD */
  dsqmhsm_iface_teardown_hdlr,    /* PROXY_IFACE_TEARDOWN_CMD */
  dsqmhsm_modem_qos_ind_hdlr,    /* PROXY_IFACE_MODEM_QOS_IND */
  dsqmhsm_modem_event_ind_hdlr,    /* PROXY_IFACE_MODEM_EVENT_IND */
  NULL,    /* PROXY_PHYS_LINK_UP_CMD */
  dsqmhsm_physlink_teardown_hdlr,    /* PROXY_PHYS_LINK_DOWN_CMD */
  NULL,    /* PROXY_IFACE_MODEM_UP_IND */
  dsqmhsm_modem_down_ind_hdlr,    /* PROXY_IFACE_MODEM_DOWN_IND */
  NULL,    /* PROXY_IFACE_PLATFORM_UP_IND */
  dsqmhsm_platform_down_ind_hdlr,    /* PROXY_IFACE_PLATFORM_DOWN_IND */
  dsqmhsm_configured_ind_hdlr,    /* PROXY_IFACE_CONFIGURED_IND */
  NULL,    /* PROXY_IFACE_MODEM_INTERNAL_IND */
  NULL,    /* PROXY_IFACE_MODEM_MCAST_IND */
  NULL,    /* PROXY_IFACE_MODEM_BCMCS_IND */
  NULL,    /* PROXY_IFACE_MODEM_MTREQ_IND */
  NULL,    /* PROXY_IFACE_PLATFORM_QOS_IND */

  /* Transition functions for state DSPROXY_IFACE_STATE_RECONFIGURING */
  NULL,    /* PROXY_IFACE_MODEM_INIT_IND */
  NULL,    /* PROXY_IFACE_BRING_UP_CMD */
  dsqmhsm_iface_teardown_hdlr,    /* PROXY_IFACE_TEARDOWN_CMD */
  dsqmhsm_modem_qos_ind_hdlr,    /* PROXY_IFACE_MODEM_QOS_IND */
  dsqmhsm_modem_event_ind_hdlr,    /* PROXY_IFACE_MODEM_EVENT_IND */
  NULL,    /* PROXY_PHYS_LINK_UP_CMD */
  dsqmhsm_physlink_teardown_hdlr,    /* PROXY_PHYS_LINK_DOWN_CMD */
  NULL,    /* PROXY_IFACE_MODEM_UP_IND */
  dsqmhsm_modem_down_ind_hdlr,    /* PROXY_IFACE_MODEM_DOWN_IND */
  NULL,    /* PROXY_IFACE_PLATFORM_UP_IND */
  dsqmhsm_platform_down_ind_hdlr,    /* PROXY_IFACE_PLATFORM_DOWN_IND */
  dsqmhsm_configured_ind_hdlr,    /* PROXY_IFACE_CONFIGURED_IND */
  dsqmhsm_modem_internal_ind_hdlr,    /* PROXY_IFACE_MODEM_INTERNAL_IND */
  dsqmhsm_modem_mcast_ind_hdlr,    /* PROXY_IFACE_MODEM_MCAST_IND */
  dsqmhsm_modem_bcmcs_ind_hdlr,    /* PROXY_IFACE_MODEM_BCMCS_IND */
  dsqmhsm_modem_mtreq_ind_hdlr,    /* PROXY_IFACE_MODEM_MTREQ_IND */
  dsqmhsm_platform_qos_ind_hdlr,    /* PROXY_IFACE_PLATFORM_QOS_IND */

  /* Transition functions for state DSPROXY_IFACE_STATE_UP */
  NULL,    /* PROXY_IFACE_MODEM_INIT_IND */
  NULL,    /* PROXY_IFACE_BRING_UP_CMD */
  dsqmhsm_iface_teardown_hdlr,    /* PROXY_IFACE_TEARDOWN_CMD */
  dsqmhsm_modem_qos_ind_hdlr,    /* PROXY_IFACE_MODEM_QOS_IND */
  dsqmhsm_modem_event_ind_hdlr,    /* PROXY_IFACE_MODEM_EVENT_IND */
  dsqmhsm_physlink_bringup_hdlr,    /* PROXY_PHYS_LINK_UP_CMD */
  dsqmhsm_physlink_teardown_hdlr,    /* PROXY_PHYS_LINK_DOWN_CMD */
  dsqmhsm_modem_up_ind_hdlr,    /* PROXY_IFACE_MODEM_UP_IND */
  dsqmhsm_modem_down_ind_hdlr,    /* PROXY_IFACE_MODEM_DOWN_IND */
  NULL,    /* PROXY_IFACE_PLATFORM_UP_IND */
  dsqmhsm_platform_down_ind_hdlr,    /* PROXY_IFACE_PLATFORM_DOWN_IND */
  dsqmhsm_configured_ind_hdlr,    /* PROXY_IFACE_CONFIGURED_IND */
  dsqmhsm_modem_internal_ind_hdlr,    /* PROXY_IFACE_MODEM_INTERNAL_IND */
  dsqmhsm_modem_mcast_ind_hdlr,    /* PROXY_IFACE_MODEM_MCAST_IND */
  dsqmhsm_modem_bcmcs_ind_hdlr,    /* PROXY_IFACE_MODEM_BCMCS_IND */
  dsqmhsm_modem_mtreq_ind_hdlr,    /* PROXY_IFACE_MODEM_MTREQ_IND */
  dsqmhsm_platform_qos_ind_hdlr,    /* PROXY_IFACE_PLATFORM_QOS_IND */

  /* Transition functions for state DSPROXY_IFACE_STATE_PLAT_GOING_DOWN */
  NULL,    /* PROXY_IFACE_MODEM_INIT_IND */
  NULL,    /* PROXY_IFACE_BRING_UP_CMD */
  NULL,    /* PROXY_IFACE_TEARDOWN_CMD */
  dsqmhsm_modem_qos_ind_hdlr,    /* PROXY_IFACE_MODEM_QOS_IND */
  dsqmhsm_modem_event_ind_hdlr,    /* PROXY_IFACE_MODEM_EVENT_IND */
  NULL,    /* PROXY_PHYS_LINK_UP_CMD */
  NULL,    /* PROXY_PHYS_LINK_DOWN_CMD */
  NULL,    /* PROXY_IFACE_MODEM_UP_IND */
  dsqmhsm_modem_down_ind_hdlr,    /* PROXY_IFACE_MODEM_DOWN_IND */
  NULL,    /* PROXY_IFACE_PLATFORM_UP_IND */
  dsqmhsm_platform_down_ind_hdlr,    /* PROXY_IFACE_PLATFORM_DOWN_IND */
  NULL,    /* PROXY_IFACE_CONFIGURED_IND */
  dsqmhsm_modem_internal_ind_hdlr,    /* PROXY_IFACE_MODEM_INTERNAL_IND */
  dsqmhsm_modem_mcast_ind_hdlr,    /* PROXY_IFACE_MODEM_MCAST_IND */
  dsqmhsm_modem_bcmcs_ind_hdlr,    /* PROXY_IFACE_MODEM_BCMCS_IND */
  dsqmhsm_modem_mtreq_ind_hdlr,    /* PROXY_IFACE_MODEM_MTREQ_IND */
  dsqmhsm_platform_qos_ind_hdlr,    /* PROXY_IFACE_PLATFORM_QOS_IND */

  /* Transition functions for state DSPROXY_IFACE_STATE_GOING_DOWN */
  NULL,    /* PROXY_IFACE_MODEM_INIT_IND */
  NULL,    /* PROXY_IFACE_BRING_UP_CMD */
  NULL,    /* PROXY_IFACE_TEARDOWN_CMD */
  dsqmhsm_modem_qos_ind_hdlr,    /* PROXY_IFACE_MODEM_QOS_IND */
  dsqmhsm_modem_event_ind_hdlr,    /* PROXY_IFACE_MODEM_EVENT_IND */
  NULL,    /* PROXY_PHYS_LINK_UP_CMD */
  dsqmhsm_physlink_teardown_hdlr,    /* PROXY_PHYS_LINK_DOWN_CMD */
  NULL,    /* PROXY_IFACE_MODEM_UP_IND */
  dsqmhsm_modem_down_ind_hdlr,    /* PROXY_IFACE_MODEM_DOWN_IND */
  NULL,    /* PROXY_IFACE_PLATFORM_UP_IND */
  dsqmhsm_platform_down_ind_hdlr,    /* PROXY_IFACE_PLATFORM_DOWN_IND */
  NULL,    /* PROXY_IFACE_CONFIGURED_IND */
  dsqmhsm_modem_internal_ind_hdlr,    /* PROXY_IFACE_MODEM_INTERNAL_IND */
  dsqmhsm_modem_mcast_ind_hdlr,    /* PROXY_IFACE_MODEM_MCAST_IND */
  dsqmhsm_modem_bcmcs_ind_hdlr,    /* PROXY_IFACE_MODEM_BCMCS_IND */
  dsqmhsm_modem_mtreq_ind_hdlr,    /* PROXY_IFACE_MODEM_MTREQ_IND */
  dsqmhsm_platform_qos_ind_hdlr,    /* PROXY_IFACE_PLATFORM_QOS_IND */

};
#endif /* !STM_DATA_STRUCTURES_ONLY */

/* State { name, entry, exit, child SM } table */
static const stm_state_map_t
  DSQMH_SM_states[ DSQMH_SM_NUM_STATES ] =
{
  {"DSPROXY_IFACE_STATE_DISABLED",
#ifndef STM_DATA_STRUCTURES_ONLY
    dsqmhsm_state_disabled_entry, dsqmhsm_state_disabled_exit,
#else /* STM_DATA_STRUCTURES_ONLY */
    NULL, NULL,
#endif /* STM_DATA_STRUCTURES_ONLY */
    NULL},
  {"DSPROXY_IFACE_STATE_DOWN",
#ifndef STM_DATA_STRUCTURES_ONLY
    dsqmhsm_state_down_entry, dsqmhsm_state_down_exit,
#else /* STM_DATA_STRUCTURES_ONLY */
    NULL, NULL,
#endif /* STM_DATA_STRUCTURES_ONLY */
    NULL},
  {"DSPROXY_IFACE_STATE_COMING_UP",
#ifndef STM_DATA_STRUCTURES_ONLY
    dsqmhsm_state_comingup_entry, dsqmhsm_state_comingup_exit,
#else /* STM_DATA_STRUCTURES_ONLY */
    NULL, NULL,
#endif /* STM_DATA_STRUCTURES_ONLY */
    NULL},
  {"DSPROXY_IFACE_STATE_PLAT_COMING_UP",
#ifndef STM_DATA_STRUCTURES_ONLY
    dsqmhsm_state_platform_comingup_entry, dsqmhsm_state_platform_comingup_exit,
#else /* STM_DATA_STRUCTURES_ONLY */
    NULL, NULL,
#endif /* STM_DATA_STRUCTURES_ONLY */
    NULL},
  {"DSPROXY_IFACE_STATE_CONFIGURING",
#ifndef STM_DATA_STRUCTURES_ONLY
    dsqmhsm_state_configuring_entry, dsqmhsm_state_configuring_exit,
#else /* STM_DATA_STRUCTURES_ONLY */
    NULL, NULL,
#endif /* STM_DATA_STRUCTURES_ONLY */
    NULL},
  {"DSPROXY_IFACE_STATE_RECONFIGURING",
#ifndef STM_DATA_STRUCTURES_ONLY
    dsqmhsm_state_reconfiguring_entry, dsqmhsm_state_reconfiguring_exit,
#else /* STM_DATA_STRUCTURES_ONLY */
    NULL, NULL,
#endif /* STM_DATA_STRUCTURES_ONLY */
    NULL},
  {"DSPROXY_IFACE_STATE_UP",
#ifndef STM_DATA_STRUCTURES_ONLY
    dsqmhsm_state_up_entry, dsqmhsm_state_up_exit,
#else /* STM_DATA_STRUCTURES_ONLY */
    NULL, NULL,
#endif /* STM_DATA_STRUCTURES_ONLY */
    NULL},
  {"DSPROXY_IFACE_STATE_PLAT_GOING_DOWN",
#ifndef STM_DATA_STRUCTURES_ONLY
    dsqmhsm_state_platform_goingdown_entry, dsqmhsm_state_platform_goingdown_exit,
#else /* STM_DATA_STRUCTURES_ONLY */
    NULL, NULL,
#endif /* STM_DATA_STRUCTURES_ONLY */
    NULL},
  {"DSPROXY_IFACE_STATE_GOING_DOWN",
#ifndef STM_DATA_STRUCTURES_ONLY
    dsqmhsm_state_goingdown_entry, dsqmhsm_state_goingdown_exit,
#else /* STM_DATA_STRUCTURES_ONLY */
    NULL, NULL,
#endif /* STM_DATA_STRUCTURES_ONLY */
    NULL},
};

/* Input { name, value } table */
static const stm_input_map_t
  DSQMH_SM_inputs[ DSQMH_SM_NUM_INPUTS ] =
{
  { "PROXY_IFACE_MODEM_INIT_IND" , (stm_input_t) PROXY_IFACE_MODEM_INIT_IND },
  { "PROXY_IFACE_BRING_UP_CMD" , (stm_input_t) PROXY_IFACE_BRING_UP_CMD },
  { "PROXY_IFACE_TEARDOWN_CMD" , (stm_input_t) PROXY_IFACE_TEARDOWN_CMD },
  { "PROXY_IFACE_MODEM_QOS_IND" , (stm_input_t) PROXY_IFACE_MODEM_QOS_IND },
  { "PROXY_IFACE_MODEM_EVENT_IND" , (stm_input_t) PROXY_IFACE_MODEM_EVENT_IND },
  { "PROXY_PHYS_LINK_UP_CMD" , (stm_input_t) PROXY_PHYS_LINK_UP_CMD },
  { "PROXY_PHYS_LINK_DOWN_CMD" , (stm_input_t) PROXY_PHYS_LINK_DOWN_CMD },
  { "PROXY_IFACE_MODEM_UP_IND" , (stm_input_t) PROXY_IFACE_MODEM_UP_IND },
  { "PROXY_IFACE_MODEM_DOWN_IND" , (stm_input_t) PROXY_IFACE_MODEM_DOWN_IND },
  { "PROXY_IFACE_PLATFORM_UP_IND" , (stm_input_t) PROXY_IFACE_PLATFORM_UP_IND },
  { "PROXY_IFACE_PLATFORM_DOWN_IND" , (stm_input_t) PROXY_IFACE_PLATFORM_DOWN_IND },
  { "PROXY_IFACE_CONFIGURED_IND" , (stm_input_t) PROXY_IFACE_CONFIGURED_IND },
  { "PROXY_IFACE_MODEM_INTERNAL_IND" , (stm_input_t) PROXY_IFACE_MODEM_INTERNAL_IND },
  { "PROXY_IFACE_MODEM_MCAST_IND" , (stm_input_t) PROXY_IFACE_MODEM_MCAST_IND },
  { "PROXY_IFACE_MODEM_BCMCS_IND" , (stm_input_t) PROXY_IFACE_MODEM_BCMCS_IND },
  { "PROXY_IFACE_MODEM_MTREQ_IND" , (stm_input_t) PROXY_IFACE_MODEM_MTREQ_IND },
  { "PROXY_IFACE_PLATFORM_QOS_IND" , (stm_input_t) PROXY_IFACE_PLATFORM_QOS_IND },
};


/* Constant all-instance state machine data */
static const stm_state_machine_constdata_t DSQMH_SM_constdata =
{
  DSQMH_SM_NUM_INSTANCES, /* number of state machine instances */
  DSQMH_SM_NUM_STATES, /* number of states */
  DSQMH_SM_states, /* array of state mappings */
  DSQMH_SM_NUM_INPUTS, /* number of inputs */
  DSQMH_SM_inputs, /* array of input mappings */
#ifndef STM_DATA_STRUCTURES_ONLY
  DSQMH_SM_transitions, /* array of transition function mappings */
  dsqmhsm_sm_entry, /* state machine entry function */
  dsqmhsm_sm_exit, /* state machine exit function */
  dsqmhsm_error_hook, /* state machine error hook function */
  dsqmhsm_debug_hook, /* state machine debug hook function */
  DSPROXY_IFACE_STATE_DISABLED /* state machine initial state */
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
  DSQMH_SM_perinst_constdata[ DSQMH_SM_NUM_INSTANCES ] =
{
  {
    &DSQMH_SM_constdata, /* state machine constant data */
    "DSQMH_SM[0]", /* state machine name */
    0xeb942998, /* state machine unique ID (md5("DSQMH_SM[0]") & 0xFFFFFFFF) */
    0  /* this state machine instance */
  },
  {
    &DSQMH_SM_constdata, /* state machine constant data */
    "DSQMH_SM[1]", /* state machine name */
    0x81b1eadc, /* state machine unique ID (md5("DSQMH_SM[1]") & 0xFFFFFFFF) */
    1  /* this state machine instance */
  },
  {
    &DSQMH_SM_constdata, /* state machine constant data */
    "DSQMH_SM[2]", /* state machine name */
    0x2222b70e, /* state machine unique ID (md5("DSQMH_SM[2]") & 0xFFFFFFFF) */
    2  /* this state machine instance */
  },
  {
    &DSQMH_SM_constdata, /* state machine constant data */
    "DSQMH_SM[3]", /* state machine name */
    0xa126ea5a, /* state machine unique ID (md5("DSQMH_SM[3]") & 0xFFFFFFFF) */
    3  /* this state machine instance */
  },
  {
    &DSQMH_SM_constdata, /* state machine constant data */
    "DSQMH_SM[4]", /* state machine name */
    0x68420b94, /* state machine unique ID (md5("DSQMH_SM[4]") & 0xFFFFFFFF) */
    4  /* this state machine instance */
  },
  {
    &DSQMH_SM_constdata, /* state machine constant data */
    "DSQMH_SM[5]", /* state machine name */
    0x647ee287, /* state machine unique ID (md5("DSQMH_SM[5]") & 0xFFFFFFFF) */
    5  /* this state machine instance */
  },
  {
    &DSQMH_SM_constdata, /* state machine constant data */
    "DSQMH_SM[6]", /* state machine name */
    0x64ba0790, /* state machine unique ID (md5("DSQMH_SM[6]") & 0xFFFFFFFF) */
    6  /* this state machine instance */
  },
  {
    &DSQMH_SM_constdata, /* state machine constant data */
    "DSQMH_SM[7]", /* state machine name */
    0x264d2556, /* state machine unique ID (md5("DSQMH_SM[7]") & 0xFFFFFFFF) */
    7  /* this state machine instance */
  },
  {
    &DSQMH_SM_constdata, /* state machine constant data */
    "DSQMH_SM[8]", /* state machine name */
    0x046b29d4, /* state machine unique ID (md5("DSQMH_SM[8]") & 0xFFFFFFFF) */
    8  /* this state machine instance */
  },
  {
    &DSQMH_SM_constdata, /* state machine constant data */
    "DSQMH_SM[9]", /* state machine name */
    0xfd160190, /* state machine unique ID (md5("DSQMH_SM[9]") & 0xFFFFFFFF) */
    9  /* this state machine instance */
  },
  {
    &DSQMH_SM_constdata, /* state machine constant data */
    "DSQMH_SM[10]", /* state machine name */
    0x9dff5970, /* state machine unique ID (md5("DSQMH_SM[10]") & 0xFFFFFFFF) */
    10  /* this state machine instance */
  },
  {
    &DSQMH_SM_constdata, /* state machine constant data */
    "DSQMH_SM[11]", /* state machine name */
    0x427fd7ad, /* state machine unique ID (md5("DSQMH_SM[11]") & 0xFFFFFFFF) */
    11  /* this state machine instance */
  },
  {
    &DSQMH_SM_constdata, /* state machine constant data */
    "DSQMH_SM[12]", /* state machine name */
    0xfd6e5e01, /* state machine unique ID (md5("DSQMH_SM[12]") & 0xFFFFFFFF) */
    12  /* this state machine instance */
  },
  {
    &DSQMH_SM_constdata, /* state machine constant data */
    "DSQMH_SM[13]", /* state machine name */
    0xb1f273a8, /* state machine unique ID (md5("DSQMH_SM[13]") & 0xFFFFFFFF) */
    13  /* this state machine instance */
  },
  {
    &DSQMH_SM_constdata, /* state machine constant data */
    "DSQMH_SM[14]", /* state machine name */
    0xfe2c4acf, /* state machine unique ID (md5("DSQMH_SM[14]") & 0xFFFFFFFF) */
    14  /* this state machine instance */
  },
  {
    &DSQMH_SM_constdata, /* state machine constant data */
    "DSQMH_SM[15]", /* state machine name */
    0x127d1540, /* state machine unique ID (md5("DSQMH_SM[15]") & 0xFFFFFFFF) */
    15  /* this state machine instance */
  },
};

/* State machine instance array definition */
stm_state_machine_t
  DSQMH_SM[ DSQMH_SM_NUM_INSTANCES ] =
{
  {
    &DSQMH_SM_perinst_constdata[ 0 ], /* per instance constant data array */
    STM_DEACTIVATED_STATE, /* current state */
    -1, /* current input index */
    NULL, /* calling SM instance */
    FALSE, /* propagate input to parent */
    FALSE, /* locked flag */
    NULL, /* user defined per-instance data */
    0, /* user defined debug mask */
  },
  {
    &DSQMH_SM_perinst_constdata[ 1 ], /* per instance constant data array */
    STM_DEACTIVATED_STATE, /* current state */
    -1, /* current input index */
    NULL, /* calling SM instance */
    FALSE, /* propagate input to parent */
    FALSE, /* locked flag */
    NULL, /* user defined per-instance data */
    0, /* user defined debug mask */
  },
  {
    &DSQMH_SM_perinst_constdata[ 2 ], /* per instance constant data array */
    STM_DEACTIVATED_STATE, /* current state */
    -1, /* current input index */
    NULL, /* calling SM instance */
    FALSE, /* propagate input to parent */
    FALSE, /* locked flag */
    NULL, /* user defined per-instance data */
    0, /* user defined debug mask */
  },
  {
    &DSQMH_SM_perinst_constdata[ 3 ], /* per instance constant data array */
    STM_DEACTIVATED_STATE, /* current state */
    -1, /* current input index */
    NULL, /* calling SM instance */
    FALSE, /* propagate input to parent */
    FALSE, /* locked flag */
    NULL, /* user defined per-instance data */
    0, /* user defined debug mask */
  },
  {
    &DSQMH_SM_perinst_constdata[ 4 ], /* per instance constant data array */
    STM_DEACTIVATED_STATE, /* current state */
    -1, /* current input index */
    NULL, /* calling SM instance */
    FALSE, /* propagate input to parent */
    FALSE, /* locked flag */
    NULL, /* user defined per-instance data */
    0, /* user defined debug mask */
  },
  {
    &DSQMH_SM_perinst_constdata[ 5 ], /* per instance constant data array */
    STM_DEACTIVATED_STATE, /* current state */
    -1, /* current input index */
    NULL, /* calling SM instance */
    FALSE, /* propagate input to parent */
    FALSE, /* locked flag */
    NULL, /* user defined per-instance data */
    0, /* user defined debug mask */
  },
  {
    &DSQMH_SM_perinst_constdata[ 6 ], /* per instance constant data array */
    STM_DEACTIVATED_STATE, /* current state */
    -1, /* current input index */
    NULL, /* calling SM instance */
    FALSE, /* propagate input to parent */
    FALSE, /* locked flag */
    NULL, /* user defined per-instance data */
    0, /* user defined debug mask */
  },
  {
    &DSQMH_SM_perinst_constdata[ 7 ], /* per instance constant data array */
    STM_DEACTIVATED_STATE, /* current state */
    -1, /* current input index */
    NULL, /* calling SM instance */
    FALSE, /* propagate input to parent */
    FALSE, /* locked flag */
    NULL, /* user defined per-instance data */
    0, /* user defined debug mask */
  },
  {
    &DSQMH_SM_perinst_constdata[ 8 ], /* per instance constant data array */
    STM_DEACTIVATED_STATE, /* current state */
    -1, /* current input index */
    NULL, /* calling SM instance */
    FALSE, /* propagate input to parent */
    FALSE, /* locked flag */
    NULL, /* user defined per-instance data */
    0, /* user defined debug mask */
  },
  {
    &DSQMH_SM_perinst_constdata[ 9 ], /* per instance constant data array */
    STM_DEACTIVATED_STATE, /* current state */
    -1, /* current input index */
    NULL, /* calling SM instance */
    FALSE, /* propagate input to parent */
    FALSE, /* locked flag */
    NULL, /* user defined per-instance data */
    0, /* user defined debug mask */
  },
  {
    &DSQMH_SM_perinst_constdata[ 10 ], /* per instance constant data array */
    STM_DEACTIVATED_STATE, /* current state */
    -1, /* current input index */
    NULL, /* calling SM instance */
    FALSE, /* propagate input to parent */
    FALSE, /* locked flag */
    NULL, /* user defined per-instance data */
    0, /* user defined debug mask */
  },
  {
    &DSQMH_SM_perinst_constdata[ 11 ], /* per instance constant data array */
    STM_DEACTIVATED_STATE, /* current state */
    -1, /* current input index */
    NULL, /* calling SM instance */
    FALSE, /* propagate input to parent */
    FALSE, /* locked flag */
    NULL, /* user defined per-instance data */
    0, /* user defined debug mask */
  },
  {
    &DSQMH_SM_perinst_constdata[ 12 ], /* per instance constant data array */
    STM_DEACTIVATED_STATE, /* current state */
    -1, /* current input index */
    NULL, /* calling SM instance */
    FALSE, /* propagate input to parent */
    FALSE, /* locked flag */
    NULL, /* user defined per-instance data */
    0, /* user defined debug mask */
  },
  {
    &DSQMH_SM_perinst_constdata[ 13 ], /* per instance constant data array */
    STM_DEACTIVATED_STATE, /* current state */
    -1, /* current input index */
    NULL, /* calling SM instance */
    FALSE, /* propagate input to parent */
    FALSE, /* locked flag */
    NULL, /* user defined per-instance data */
    0, /* user defined debug mask */
  },
  {
    &DSQMH_SM_perinst_constdata[ 14 ], /* per instance constant data array */
    STM_DEACTIVATED_STATE, /* current state */
    -1, /* current input index */
    NULL, /* calling SM instance */
    FALSE, /* propagate input to parent */
    FALSE, /* locked flag */
    NULL, /* user defined per-instance data */
    0, /* user defined debug mask */
  },
  {
    &DSQMH_SM_perinst_constdata[ 15 ], /* per instance constant data array */
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
void DSQMH_SM_reset(void)
{
  uint32 idx;
  void **tricky;

  /* Reset all the child SMs (if any) */
  

  /* Reset the parent */
  for( idx = 0; idx < DSQMH_SM_NUM_INSTANCES; idx++)
  {
    tricky = (void **)&DSQMH_SM[ idx ].pi_const_data; /* sleight of hand to assign to const ptr below */
    *tricky = (void *)&DSQMH_SM_perinst_constdata[ idx ]; /* per instance constant data array */
    DSQMH_SM[ idx ].current_state = STM_DEACTIVATED_STATE; /* current state */
    DSQMH_SM[ idx ].curr_input_index = -1; /* current input index */
    DSQMH_SM[ idx ].propagate_input = FALSE; /* propagate input to parent */
    DSQMH_SM[ idx ].is_locked = FALSE; /* locked flag */
    DSQMH_SM[ idx ].user_data = NULL; /* user defined per-instance data */
    DSQMH_SM[ idx ].debug_mask = 0; /* user defined debug mask */
  }

}
#endif /* !STM_DATA_STRUCTURES_ONLY */

/* End machine generated internal source for state machine array: DSQMH_SM[] */


