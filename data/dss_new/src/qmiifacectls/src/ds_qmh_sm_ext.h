/*=============================================================================

    ds_qmh_sm_ext.h

Description:
  This file contains the machine generated header file for the state machine
  specified in the file:
        ds_qmh_sm.stm

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


#ifndef DS_QMH_SM_EXT_H
#define DS_QMH_SM_EXT_H

#ifdef __cplusplus
/* If compiled into a C++ file, ensure symbols names are not mangled */
extern "C"
{
#endif

/* Include STM framework header */
#include "stm2.h"

/* Begin machine generated code for state machine array: DSQMH_SM[] */

/* Define a macro for the number of SM instances */
#define DSQMH_SM_NUM_INSTANCES 16

/* External reference to state machine structure */
extern stm_state_machine_t DSQMH_SM[ DSQMH_SM_NUM_INSTANCES ];

/* External enumeration representing state machine's states */
enum
{
  DSQMH_SM__DSPROXY_IFACE_STATE_DISABLED,
  DSQMH_SM__DSPROXY_IFACE_STATE_DOWN,
  DSQMH_SM__DSPROXY_IFACE_STATE_COMING_UP,
  DSQMH_SM__DSPROXY_IFACE_STATE_PLAT_COMING_UP,
  DSQMH_SM__DSPROXY_IFACE_STATE_CONFIGURING,
  DSQMH_SM__DSPROXY_IFACE_STATE_RECONFIGURING,
  DSQMH_SM__DSPROXY_IFACE_STATE_UP,
  DSQMH_SM__DSPROXY_IFACE_STATE_PLAT_GOING_DOWN,
  DSQMH_SM__DSPROXY_IFACE_STATE_GOING_DOWN,
};

#ifndef STM_DATA_STRUCTURES_ONLY
/* User called 'reset' routine.  Should never be needed, but can be used to
   effect a complete reset of all a given state machine's instances. */
extern void DSQMH_SM_reset(void);
#endif /* !STM_DATA_STRUCTURES_ONLY */

/* End machine generated code for state machine array: DSQMH_SM[] */


#ifdef __cplusplus
} /* extern "C" {...} */
#endif

#endif /* DS_QMH_SM_EXT_H */
