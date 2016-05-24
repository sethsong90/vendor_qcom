/******************************************************************************

                          D S _ F M C _ A P P _ E X E C . H

******************************************************************************/

/******************************************************************************

  @file    ds_fmc_app_exec.h
  @brief   DS_FMC_APP executive header file

  DESCRIPTION
  Header file for DS_FMC_APP executive control module.

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
05/13/10   scb        Initial version

******************************************************************************/

#ifndef __DS_FMC_APP_EXEC_H__
#define __DS_FMC_APP_EXEC_H__

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/
#include "comdef.h"

#include "ds_list.h"
#include "ds_cmdq.h"
#include "ds_fmc_app.h"
#include "ds_fmc_app_sm.h"

/*===========================================================================
                     GLOBAL DEFINITIONS AND DECLARATIONS
===========================================================================*/
/*--------------------------------------------------------------------------- 
   Type of a Executive event data
---------------------------------------------------------------------------*/
typedef struct ds_fmc_app_exec_cmd_data_s {
  ds_fmc_app_sm_events_t        type;                        /* Event type */
  boolean                       ext_data_path_conn_status; 
                           /* Conn status of the external data path entity */
  ds_fmc_app_tunnel_mgr_ds_type ds_fmc_app_tunnel_mgr_ds; 
                                                     /* Tunnel information */
} ds_fmc_app_exec_cmd_data_t;

/*--------------------------------------------------------------------------- 
   Type of a Executive command
---------------------------------------------------------------------------*/
typedef struct ds_fmc_app_exec_cmd_s {
  ds_cmd_t                      cmd;     /* Command object         */
  ds_fmc_app_exec_cmd_data_t    data;    /* Command data           */
} ds_fmc_app_exec_cmd_t;

/*--------------------------------------------------------------------------- 
   Type representing collection of state information for module
---------------------------------------------------------------------------*/
struct ds_fmc_app_exec_state_s {
  struct ds_cmdq_info_s       cmdq;  /* Command queue for async processing */  
};

/*===========================================================================
                            GLOBAL FUNCTION DECLARATIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  ds_fmc_app_exec_get_cmd
===========================================================================*/
/*!
@brief
  Function to get a command buffer for asynchronous processing

@return
  Returns the command pointer of type ds_fmc_app_exec_cmd_t*

@note

  - Dependencies
    - None  

  - Side Effects
    - Allocated heap memory
*/
/*=========================================================================*/
ds_fmc_app_exec_cmd_t * ds_fmc_app_exec_get_cmd ( void );


/*===========================================================================
  FUNCTION  ds_fmc_app_exec_release_cmd
===========================================================================*/
/*!
@brief
  Function to release a command buffer 

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - Returns memory to heap
*/
/*=========================================================================*/
void ds_fmc_app_exec_release_cmd ( ds_fmc_app_exec_cmd_t * );


/*===========================================================================
  FUNCTION  ds_fmc_app_exec_put_cmd
===========================================================================*/
/*!
@brief
  Function to post a command buffer for asynchronous processing

@return
  int - DS_FMC_APP_SUCCESS on successful operation, DS_FMC_APP_FAILURE otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
int ds_fmc_app_exec_put_cmd ( const ds_fmc_app_exec_cmd_t * cmdbuf );


/*===========================================================================
  FUNCTION  ds_fmc_app_exec_wait
===========================================================================*/
/*!
@brief
  Forces calling thread to wait on exit of command queue thread.

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - Calling thread is blocked indefinitely
*/
/*=========================================================================*/
void ds_fmc_app_exec_wait ( void );


/*===========================================================================
  FUNCTION  ds_fmc_app_exec_init
===========================================================================*/
/*!
@brief
 Main initialization routine of the executive control module. 

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
void ds_fmc_app_exec_init ( void );

#endif /* __DS_FMC_APP_EXEC_H__ */
