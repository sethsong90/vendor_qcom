/******************************************************************************

                          D S _ F M C _ A P P _ E X E C . C

******************************************************************************/

/******************************************************************************

  @file    ds_fmc_app_exec.c
  @brief   DS_FMC_APP executive

  DESCRIPTION
  Implementation of DS_FMC_APP executive control module.

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

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include <pthread.h>
#include <stdlib.h>
#include <assert.h>

#include "stm2.h"
#include "ds_list.h"
#include "ds_cmdq.h"
#include "ds_fmc_app_exec.h"
#include "ds_fmc_app_util.h"

/*===========================================================================
                     LOCAL DEFINITIONS AND DECLARATIONS
===========================================================================*/


/*--------------------------------------------------------------------------- 
   Constant representing maximum number of command buffers used by this 
   module
---------------------------------------------------------------------------*/
#define DS_FMC_APP_EXEC_MAX_CMDS 10

/*--------------------------------------------------------------------------- 
  Asynchronous event names for debug use.  This array must match order of
  ds_fmc_app_sm_events_t in ds_fmc_app_sm.h
---------------------------------------------------------------------------*/

#define DS_FMC_APP_EVENT_NAME_SIZ 50


LOCAL const char ds_fmc_app_exec_cmd_names[][DS_FMC_APP_EVENT_NAME_SIZ] = {
  "DS_FMC_APP_EXT_TRIG_DISABLE_EV",          /* External triggered disable */
  "DS_FMC_APP_EXT_TRIG_ENABLE_EV",           /* External triggered enable  */
  "DS_FMC_APP_TUNNEL_OPENED_EV",             /* Tunnel opened event        */
  "DS_FMC_APP_TUNNEL_CLOSED_EV",             /* Tunnel closed event        */
  "DS_FMC_APP_BEARER_UP_EV",                 /* Bearer up indication       */
  "DS_FMC_APP_BEARER_DOWN_EV"                /* Bearer down indication     */
};

/*--------------------------------------------------------------------------- 
   Executive control state information
---------------------------------------------------------------------------*/
LOCAL struct ds_fmc_app_exec_state_s  ds_fmc_app_exec_state_info;

/*===========================================================================
                            LOCAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  ds_fmc_app_exec_cmd_free
===========================================================================*/
/*!
@brief
 Virtual function registered with the Command Thread to free a  
 command buffer, after execution of the command is complete. 

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
ds_fmc_app_exec_cmd_free( ds_cmd_t * cmd, void * data )
{
  ds_fmc_app_exec_cmd_t * cmd_buf;

  DS_FMC_APP_LOG_FUNC_ENTRY;
  
  cmd_buf = (ds_fmc_app_exec_cmd_t *)data;

  /* Double check for debug purposes that the command is legit */
  DS_FMC_APP_ASSERT( &cmd_buf->cmd == cmd );

  /* Release dynamic memory */
  ds_fmc_app_free( cmd_buf );

  data = NULL;

  DS_FMC_APP_LOG_FUNC_EXIT;
  
  return;
} /* ds_fmc_app_exec_cmd_free */

/*===========================================================================
  FUNCTION  ds_fmc_app_exec_cmd_process
===========================================================================*/
/*!
@brief
 Virtual function registered with the Command Thread to process a  
 command buffer. 

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
ds_fmc_app_exec_cmd_process (ds_cmd_t * cmd, void * data)
{
  ds_fmc_app_exec_cmd_t * cmd_buf;
  stm_status_t            sm_result;

  DS_FMC_APP_LOG_FUNC_ENTRY;
  
  cmd_buf = (ds_fmc_app_exec_cmd_t *)data;

  /* Double check for debug purposes that the command is legit */
  DS_FMC_APP_ASSERT(&cmd_buf->cmd == cmd);

  ds_fmc_app_log_high("ds_fmc_app_exec_cmd_process: Process command:"
      " ID=%s, pthread_self() %d\n",
      ds_fmc_app_exec_cmd_names[cmd_buf->data.type], pthread_self());
  
  /* Process based on command type */
  sm_result = stm_instance_process_input( NULL,
                                          DS_FMC_APP_SM,
                                          0,
                                          (stm_input_t)cmd_buf->data.type,
                                          (void*)cmd_buf );
  if( STM_SUCCESS !=  sm_result )
  {
    if( STM_ENOTPROCESSED == sm_result )
    {
      ds_fmc_app_log_err("ds_fmc_app_exec_cmd_process: command not processed %d\n", 
                          cmd_buf->data.type);
    }
    else
    {
      ds_fmc_app_log_err("ds_fmc_app_exec_cmd_process: command processing error %d\n", 
                          sm_result);
    }
  }
  
  DS_FMC_APP_LOG_FUNC_EXIT;

  return;
} /* ds_fmc_app_exec_cmd_process */

/*===========================================================================
  FUNCTION  ds_fmc_app_exec_cleanup
===========================================================================*/
/*!
@brief
  Performs cleanup of executive module.  Invoked at process termination.

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void 
ds_fmc_app_exec_cleanup
(
  void
)
{
  /* Purge command queue to release heap memory */
  (void)ds_cmdq_deinit( &ds_fmc_app_exec_state_info.cmdq );
} /* ds_fmc_app_exec_cleanup */

/*===========================================================================
                            GLOBAL FUNCTION DEFINITIONS
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
    - None
*/
/*=========================================================================*/
ds_fmc_app_exec_cmd_t * ds_fmc_app_exec_get_cmd ( void )
{
  ds_fmc_app_exec_cmd_t * cmd_buf = NULL;

  DS_FMC_APP_LOG_FUNC_ENTRY;

  /* Allocate command buffer */
  if((cmd_buf = ds_fmc_app_malloc(sizeof(ds_fmc_app_exec_cmd_t))) == NULL )
  {
    DS_FMC_APP_STOP("ds_fmc_app_exec_cmd_alloc: ds_fmc_app_malloc failed\n");
    return NULL;
  }

  /* Assign self-reference in DS cmd payload */
  cmd_buf->cmd.data      = (void*)cmd_buf;

  /* Asssign default execution and free handlers */
  cmd_buf->cmd.execute_f = ds_fmc_app_exec_cmd_process;
  cmd_buf->cmd.free_f    = ds_fmc_app_exec_cmd_free;

  DS_FMC_APP_LOG_FUNC_EXIT;

  return cmd_buf;
} /* ds_fmc_app_exec_get_cmd */

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
void ds_fmc_app_exec_release_cmd ( ds_fmc_app_exec_cmd_t * cmd_buf )
{
  DS_FMC_APP_ASSERT( cmd_buf );
  
  DS_FMC_APP_LOG_FUNC_ENTRY;

  if( cmd_buf->cmd.free_f ) {

    cmd_buf->cmd.free_f( &cmd_buf->cmd, cmd_buf->cmd.data );

  } else {

    ds_fmc_app_log_err("ds_fmc_app_exec_release_cmd: "
            "Specified buffer not valid, ignoring\n");
  }

  DS_FMC_APP_LOG_FUNC_EXIT;

  return;
} /* ds_fmc_app_exec_release_cmd */

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
int ds_fmc_app_exec_put_cmd ( const ds_fmc_app_exec_cmd_t * cmdbuf )
{
  int result = DS_FMC_APP_SUCCESS;

  DS_FMC_APP_ASSERT( cmdbuf );
  
  DS_FMC_APP_LOG_FUNC_ENTRY;

  ds_fmc_app_log_high("Received command: ID=%s\n pthread_self() %d, "
                      "enqueueing", 
                      ds_fmc_app_exec_cmd_names[cmdbuf->data.type],
                      pthread_self());
  
  /* Append command buffer to the command queue */
  result = ds_cmdq_enq( &ds_fmc_app_exec_state_info.cmdq, &cmdbuf->cmd );
  
  DS_FMC_APP_LOG_FUNC_EXIT;
  
  return result;
} /* ds_fmc_app_exec_put_cmd */

/*===========================================================================
  FUNCTION  ds_fmc_app_exec_wait
===========================================================================*/
/*!
@brief
  Forces calling thread to wait on exit of command processing thread.

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - Calling thread is blocked indefinitely
*/
/*=========================================================================*/
void ds_fmc_app_exec_wait ( void )
{
  DS_FMC_APP_LOG_FUNC_ENTRY;
  /* 
     Join the command queue thread and wait until it exits,
     which it should do only during termination of the process 
  */
  ds_cmdq_join_thread( &ds_fmc_app_exec_state_info.cmdq );

  DS_FMC_APP_LOG_FUNC_EXIT;
} /* ds_fmc_app_exec_wait */


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
void
ds_fmc_app_exec_init ( void )
{
  
  DS_FMC_APP_LOG_FUNC_ENTRY;
  
  /* Register process termination cleanup handler */
  atexit( ds_fmc_app_exec_cleanup );
  
  /*-------------------------------------------------------------------------
    Initialize the state machine instance. 
  -------------------------------------------------------------------------*/

  if( STM_SUCCESS !=
    stm_instance_activate( DS_FMC_APP_SM, 0, (void*)(uint32)0 ) ) {
      ds_fmc_app_log_err("ds_fmc_app_exec_init: Failed to initialize"
                         " state machine\n");
  }

  /*-------------------------------------------------------------------------
    Initialize command queue for asynch processing
  -------------------------------------------------------------------------*/
  ds_cmdq_init( &ds_fmc_app_exec_state_info.cmdq, DS_FMC_APP_EXEC_MAX_CMDS );

  DS_FMC_APP_LOG_FUNC_EXIT;
  
  return;
} /* ds_fmc_app_exec_init */
