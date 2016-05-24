/******************************************************************************

                          N E T M G R _ T C . H

******************************************************************************/

/******************************************************************************

  @file    netmgr_tc.h
  @brief   Network Manager traffic control header file

  DESCRIPTION
  Header file for NetMgr Linux traffic control interface.

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
02/23/10   ar         Initial version

******************************************************************************/

#ifndef __NETMGR_TC_H__
#define __NETMGR_TC_H__

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/
#include "comdef.h"
#include "ds_list.h"

/*===========================================================================
                     GLOBAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/*--------------------------------------------------------------------------- 
   Constant used when QoS flow specification not available
---------------------------------------------------------------------------*/
#define NETMGR_TC_DEFAULT_PRIORITY   NETMGR_TC_CLASS_PRIO_BESTEFFORT 
#define NETMGR_TC_DEFAULT_DATARATE   (8UL)  /* bps units; tc rejects 0 */
/* Maximum bandwidth for network interface root qdisc. */
/* Note: each kernel interface will use same value, but underlying
 * transport may not be able to support n*MAX bandwidth. */
#define NETMGR_TC_MAX_DATARATE    (800000000UL)  /* bps units */


/*--------------------------------------------------------------------------- 
   Type representing enumeration of traffic control flow states
---------------------------------------------------------------------------*/
typedef enum {
  NETMGR_TC_FLOW_NULL,                   /* Internal value           */
  NETMGR_TC_FLOW_INIT,                   /* Initialization state     */
  NETMGR_TC_FLOW_ACTIVE,                 /* QoS scheduling active    */
  NETMGR_TC_FLOW_SUSPENDED,              /* QoS scheduling inactive  */
  NETMGR_TC_FLOW_DISABLED                /* Datapath flow controlled */
} netmgr_tc_flow_state_t;


/*--------------------------------------------------------------------------- 
  Type representing enumeration of TC class priority.
  Note: Precedence values are in descending value order  
---------------------------------------------------------------------------*/
typedef enum {
  NETMGR_TC_CLASS_PRIO_MIN             = 7,
  NETMGR_TC_CLASS_PRIO_BESTEFFORT      = NETMGR_TC_CLASS_PRIO_MIN,
  NETMGR_TC_CLASS_PRIO_BACKGROUND      = 4,
  NETMGR_TC_CLASS_PRIO_INTERACTIVE     = 3,
  NETMGR_TC_CLASS_PRIO_STREAMING       = 2,
  NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  = 1,
  NETMGR_TC_CLASS_PRIO_MAX             = 0
} netmgr_tc_class_priority_type_t;


/*===========================================================================
                            GLOBAL FUNCTION DECLARATIONS
===========================================================================*/
/*===========================================================================
  FUNCTION  netmgr_tc_init
===========================================================================*/
/*!
@brief
 Main initialization routine of the traffic control module. 

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - Initializes the QMI Driver
*/
/*=========================================================================*/
void netmgr_tc_init (int nlink, netmgr_ctl_port_config_type links[]);


/*===========================================================================
  FUNCTION  netmgr_tc_get_qos_params_by_profile_id
===========================================================================*/
/*!
@brief
  Lookup the datarate and priority QoS parameters based on CDMA profile ID. 

@return
  int - NETMGR_SUCCESS on successful operations, NETMGR_FAILURE otherwise.

@note

  - Dependencies
    - None  

  - Side Effects
    - Initializes the QMI Driver
*/
/*=========================================================================*/
int netmgr_tc_get_qos_params_by_profile_id
(
  uint16      profile_id,
  uint32    * datarate,
  uint8     * priority
);

#endif /* __NETMGR_TC_H__ */
