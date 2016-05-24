/*=============================================================================

    ds_qmh_sm_int.h

Description:
  This file contains the machine generated header file for the state machine
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


#ifndef DS_QMH_SM_INT_H
#define DS_QMH_SM_INT_H

#ifdef __cplusplus
/* If compiled into a C++ file, ensure symbols names are not mangled */
extern "C"
{
#endif

/* Include external state machine header */
#include "ds_qmh_sm_ext.h"

/* Begin machine generated internal header for state machine array: DSQMH_SM[] */

/* Suppress Lint suggestions to const-ify state machine and payload ptrs */
/*lint -esym(818,sm,payload) */

/* Define a macro for the number of SM instances */
#define DSQMH_SM_NUM_INSTANCES 16

/* Define a macro for the number of SM states */
#define DSQMH_SM_NUM_STATES 9

/* Define a macro for the number of SM inputs */
#define DSQMH_SM_NUM_INPUTS 17

#ifndef STM_DATA_STRUCTURES_ONLY
/* State Machine entry/exit function prototypes */
void dsqmhsm_sm_entry(stm_state_machine_t *sm,void *payload);
void dsqmhsm_sm_exit(stm_state_machine_t *sm,void *payload);


/* State entry/exit function prototypes */
void dsqmhsm_state_disabled_entry(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void dsqmhsm_state_disabled_exit(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void dsqmhsm_state_down_entry(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void dsqmhsm_state_down_exit(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void dsqmhsm_state_comingup_entry(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void dsqmhsm_state_comingup_exit(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void dsqmhsm_state_platform_comingup_entry(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void dsqmhsm_state_platform_comingup_exit(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void dsqmhsm_state_configuring_entry(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void dsqmhsm_state_configuring_exit(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void dsqmhsm_state_reconfiguring_entry(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void dsqmhsm_state_reconfiguring_exit(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void dsqmhsm_state_up_entry(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void dsqmhsm_state_up_exit(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void dsqmhsm_state_platform_goingdown_entry(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void dsqmhsm_state_platform_goingdown_exit(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void dsqmhsm_state_goingdown_entry(stm_state_machine_t *sm,stm_state_t _state,void *payload);
void dsqmhsm_state_goingdown_exit(stm_state_machine_t *sm,stm_state_t _state,void *payload);


/* Transition function prototypes */
stm_state_t dsqmhsm_modem_init_ind_hdlr(stm_state_machine_t *sm, void *payload);
stm_state_t dsqmhsm_iface_bringup_hdlr(stm_state_machine_t *sm, void *payload);
stm_state_t dsqmhsm_iface_teardown_hdlr(stm_state_machine_t *sm, void *payload);
stm_state_t dsqmhsm_modem_qos_ind_hdlr(stm_state_machine_t *sm, void *payload);
stm_state_t dsqmhsm_modem_event_ind_hdlr(stm_state_machine_t *sm, void *payload);
stm_state_t dsqmhsm_physlink_bringup_hdlr(stm_state_machine_t *sm, void *payload);
stm_state_t dsqmhsm_physlink_teardown_hdlr(stm_state_machine_t *sm, void *payload);
stm_state_t dsqmhsm_modem_up_ind_hdlr(stm_state_machine_t *sm, void *payload);
stm_state_t dsqmhsm_modem_down_ind_hdlr(stm_state_machine_t *sm, void *payload);
stm_state_t dsqmhsm_platform_up_ind_hdlr(stm_state_machine_t *sm, void *payload);
stm_state_t dsqmhsm_platform_down_ind_hdlr(stm_state_machine_t *sm, void *payload);
stm_state_t dsqmhsm_configured_ind_hdlr(stm_state_machine_t *sm, void *payload);
stm_state_t dsqmhsm_modem_internal_ind_hdlr(stm_state_machine_t *sm, void *payload);
stm_state_t dsqmhsm_modem_mcast_ind_hdlr(stm_state_machine_t *sm, void *payload);
stm_state_t dsqmhsm_modem_bcmcs_ind_hdlr(stm_state_machine_t *sm, void *payload);
stm_state_t dsqmhsm_modem_mtreq_ind_hdlr(stm_state_machine_t *sm, void *payload);
stm_state_t dsqmhsm_platform_qos_ind_hdlr(stm_state_machine_t *sm, void *payload);


/* State enumeration */
enum
{
  DSPROXY_IFACE_STATE_DISABLED,
  DSPROXY_IFACE_STATE_DOWN,
  DSPROXY_IFACE_STATE_COMING_UP,
  DSPROXY_IFACE_STATE_PLAT_COMING_UP,
  DSPROXY_IFACE_STATE_CONFIGURING,
  DSPROXY_IFACE_STATE_RECONFIGURING,
  DSPROXY_IFACE_STATE_UP,
  DSPROXY_IFACE_STATE_PLAT_GOING_DOWN,
  DSPROXY_IFACE_STATE_GOING_DOWN,
};

#endif /* !STM_DATA_STRUCTURES_ONLY */

/* End machine generated internal header for state machine array: DSQMH_SM[] */


#ifdef __cplusplus
} /* extern "C" {...} */
#endif

#endif /* ! DS_QMH_SM_INT_H */
