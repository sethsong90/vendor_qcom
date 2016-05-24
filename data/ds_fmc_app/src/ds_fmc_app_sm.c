/*!
  @file
  ds_fmc_app_sm.c

  @brief
  This module contains the entry, exit, and transition functions
  necessary to implement the following state machines:

  @detail
  DS_FMC_APP_SM ( 1 instance/s )


  OPTIONAL further detailed description of state machines
  - DELETE this section if unused.

*/

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


/*===========================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

$Header: $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
05/18/2010 scb     Initial version
===========================================================================*/

/*===========================================================================

                           INCLUDE FILES

===========================================================================*/

/* Include STM external API */
#include <stm2.h>


/*===========================================================================

         STM COMPILER GENERATED PROTOTYPES AND DATA STRUCTURES

===========================================================================*/

/* Include STM compiler generated internal data structure file */
#ifdef _DS_FMC_APP_DEBUG
#include <arpa/inet.h>
#endif /*_DS_FMC_APP_DEBUG*/
#include "ds_fmc_app_sm_int.h"
#include "ds_fmc_app_main.h"
#include "ds_fmc_app_util.h"
#include "ds_fmc_app_exec.h"
#include "ds_fmc_app_call_mgr.h"
#include "ds_fmc_app_tunnel_mgr.h"

/*===========================================================================

                         LOCAL VARIABLES

===========================================================================*/


/*! @brief Structure for state-machine per-instance local variables
*/
typedef struct
{
  int   sm_triggered;  /*! Variable to indicate if the SM is UP */
  int   modem_triggered; /*! Variable used to indicate if the
                             modem was triggered by the SM */
} ds_fmc_app_sm_type;


/*! @brief Variables internal to module ds_fmc_app_sm.c
*/
STATIC ds_fmc_app_sm_type ds_fmc_app_sm;


/*===========================================================================

                 STATE MACHINE: DS_FMC_APP_SM

===========================================================================*/

/*===========================================================================

  STATE MACHINE ENTRY FUNCTION:  ds_fmc_app_sm_entry

===========================================================================*/
/*!
    @brief
    Entry function for state machine DS_FMC_APP_SM

    @detail
    Called upon activation of this state machine, with optional
    user-passed payload pointer parameter.

    @return
    None

*/
/*=========================================================================*/
void ds_fmc_app_sm_entry
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{

  STM_UNUSED( payload );
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /*-----------------------------------------------------------------------*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  /*-----------------------------------------------------------------------*/

  DS_FMC_APP_LOG_FUNC_EXIT;

} /* ds_fmc_app_sm_entry() */


/*===========================================================================

  STATE MACHINE EXIT FUNCTION:  ds_fmc_app_sm_exit

===========================================================================*/
/*!
    @brief
    Exit function for state machine DS_FMC_APP_SM

    @detail
    Called upon deactivation of this state machine, with optional
    user-passed payload pointer parameter.

    @return
    None

*/
/*=========================================================================*/
void ds_fmc_app_sm_exit
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{

  STM_UNUSED( payload );
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /*-----------------------------------------------------------------------*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  /*-----------------------------------------------------------------------*/

  DS_FMC_APP_LOG_FUNC_EXIT;

} /* ds_fmc_app_sm_exit() */


/*===========================================================================

     (State Machine: DS_FMC_APP_SM)
     STATE ENTRY/EXIT/TRANSITION FUNCTIONS: DS_FMC_APP_STATE_DOWN

===========================================================================*/

/*===========================================================================

  STATE ENTRY FUNCTION:  ds_fmc_app_sm_state_down_entry

===========================================================================*/
/*!
    @brief
    Entry function for state machine DS_FMC_APP_SM,
    state DS_FMC_APP_STATE_DOWN

    @detail
    Called upon entry into this state of the state machine, with optional
    user-passed payload pointer parameter.  The prior state of the state
    machine is also passed as the prev_state parameter.

    @return
    None

*/
/*=========================================================================*/
void ds_fmc_app_sm_state_down_entry
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         prev_state,  /*!< Previous state */
  void                *payload     /*!< Payload pointer */
)
{

  STM_UNUSED( payload );
  STM_UNUSED( prev_state );
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /*-----------------------------------------------------------------------*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  /*-----------------------------------------------------------------------*/

  /*-----------------------------------------------------------------------
  Initialization of the data structure associated with the state machine
  is done here
  -----------------------------------------------------------------------*/
  ds_fmc_app_sm.sm_triggered = FALSE;
  ds_fmc_app_sm.modem_triggered = FALSE;

  DS_FMC_APP_LOG_FUNC_EXIT;

} /* ds_fmc_app_sm_state_down_entry() */


/*===========================================================================

  STATE EXIT FUNCTION:  ds_fmc_app_sm_state_down_exit

===========================================================================*/
/*!
    @brief
    Exit function for state machine DS_FMC_APP_SM,
    state DS_FMC_APP_STATE_DOWN

    @detail
    Called upon exit of this state of the state machine, with optional
    user-passed payload pointer parameter.  The impending state of the state
    machine is also passed as the next_state parameter.

    @return
    None

*/
/*=========================================================================*/
void ds_fmc_app_sm_state_down_exit
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         next_state,  /*!< Next state */
  void                *payload     /*!< Payload pointer */
)
{

  STM_UNUSED( payload );
  STM_UNUSED( next_state );
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /*-----------------------------------------------------------------------*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  /*-----------------------------------------------------------------------*/

  DS_FMC_APP_LOG_FUNC_EXIT;

} /* ds_fmc_app_sm_state_down_exit() */


/*===========================================================================

  TRANSITION FUNCTION:  ds_fmc_app_sm_handle_ext_trig_enable

===========================================================================*/
/*!
    @brief
    Transition function for state machine DS_FMC_APP_SM,
    state DS_FMC_APP_STATE_DOWN,
    upon receiving input DS_FMC_APP_EXT_TRIG_ENABLE_EV

    @detail
    Called upon receipt of input DS_FMC_APP_EXT_TRIG_ENABLE_EV, with optional
    user-passed payload pointer.

    @return
    Returns the next state that the state machine should transition to
    upon receipt of the input.  This state must be a valid state for this
    state machine.

*/
/*=========================================================================*/
stm_state_t ds_fmc_app_sm_handle_ext_trig_enable
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{
  stm_state_t next_state = STM_SAME_STATE; /* Default 'next' state */

  ds_fmc_app_call_mgr_resp_type *buffer = NULL;

  STM_UNUSED( payload );
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /*-----------------------------------------------------------------------*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  /*-----------------------------------------------------------------------*/

  if(ds_fmc_app_sm.sm_triggered )
  {
    ds_fmc_app_log_err("ds_fmc_app_sm_handle_ext_trig_enable"
   " FMC_ENABLE trigger on an already enabled state machine\n");
    DS_FMC_APP_LOG_FUNC_EXIT;
    return next_state;
  }

  /* Clear the buffer and set the bearer status to DISABLED initially. */

  buffer = (ds_fmc_app_call_mgr_resp_type*)
            ds_fmc_app_malloc(sizeof(ds_fmc_app_call_mgr_resp_type));

  DS_FMC_APP_ASSERT(buffer);

  memset(buffer, 0, sizeof(ds_fmc_app_call_mgr_resp_type));

  buffer->ds_fmc_app_fmc_bearer_status = DS_FMC_APP_FMC_BEARER_DISABLED;

  /*-----------------------------------------------------------------------
  Open a connection to the tunnel manager entity and pass the 
  DS_FMC_APP_FMC_BEARER_ENABLED message to open assoc. connections
  -----------------------------------------------------------------------*/

  if(ds_fmc_app_tunnel_mgr_open_conn() < 0)
  {
    ds_fmc_app_log_err("ds_fmc_app_sm_handle_ext_trig_enable: "
          "ds_fmc_app_tunnel_mgr_open_conn failed\n");

    /* Notify the call manager indicating failure */
    if (ds_fmc_app_cm_tx_msg(ds_fmc_app_cm_get_cm_client_fd(),
                             (unsigned char*)buffer,
                             sizeof(ds_fmc_app_call_mgr_resp_type)) < 0 )
    {
      ds_fmc_app_log_err("ds_fmc_app_sm_handle_ext_trig_enable: "
            "ds_fmc_app_cm_tx_msg %c failed\n", 
             buffer->ds_fmc_app_fmc_bearer_status);
    }

    ds_fmc_app_free(buffer);
    buffer = NULL;

    DS_FMC_APP_LOG_FUNC_EXIT;
    return( next_state );
  }

  /* Set the ds_fmc_app_fmc_bearer_status to ENABLED */

  buffer->ds_fmc_app_fmc_bearer_status = DS_FMC_APP_FMC_BEARER_ENABLED;

  /*-----------------------------------------------------------------------
    Call the tunnel manager function to enable tunnel.
  -----------------------------------------------------------------------*/

  if( ds_fmc_app_tunnel_mgr_client_tx_msg(
                      ds_fmc_app_tunnel_mgr_get_tunnel_mgr_client_fd(),
                    (unsigned char*)(&buffer->ds_fmc_app_fmc_bearer_status),
                      sizeof(buffer->ds_fmc_app_fmc_bearer_status)) < 0)
  {
    ds_fmc_app_log_err("ds_fmc_app_sm_handle_ext_trig_enable: "
          "ds_fmc_app_tunnel_mgr_client_tx_msg %c failed\n", 
          buffer->ds_fmc_app_fmc_bearer_status);

    /* Notify the call manager indicating failure */
    buffer->ds_fmc_app_fmc_bearer_status = DS_FMC_APP_FMC_BEARER_DISABLED;

    if (ds_fmc_app_cm_tx_msg(ds_fmc_app_cm_get_cm_client_fd(),
                             (unsigned char*)buffer,
                             sizeof(ds_fmc_app_call_mgr_resp_type)) < 0 )
    {
      ds_fmc_app_log_err("ds_fmc_app_sm_handle_ext_trig_enable: "
            "ds_fmc_app_cm_tx_msg %c failed\n",
            buffer->ds_fmc_app_fmc_bearer_status );
    }

#if 0
   /* Close the Tunnel manager connection previously opened. */
   if(ds_fmc_app_tunnel_mgr_close_conn(
      ds_fmc_app_tunnel_mgr_get_tunnel_mgr_client_fd()) < 0)
    {
      ds_fmc_app_log_err("ds_fmc_app_sm_handle_ext_trig_enable:"
           "ds_fmc_app_tunnel_mgr_close_conn failed\n");
    }
#endif/* 0 */
  }
  else
  {
    ds_fmc_app_log_high("ds_fmc_app_sm_handle_ext_trig_enable %c success\n",
                        buffer->ds_fmc_app_fmc_bearer_status);

    next_state = DS_FMC_APP_SM__DS_FMC_APP_STATE_ENABLING_TUNNEL;
    ds_fmc_app_sm.sm_triggered = TRUE;
  }

  ds_fmc_app_free(buffer);
  buffer = NULL;

  DS_FMC_APP_LOG_FUNC_EXIT;
  return( next_state );

} /* ds_fmc_app_sm_handle_ext_trig_enable() */


/*===========================================================================

     (State Machine: DS_FMC_APP_SM)
     STATE ENTRY/EXIT/TRANSITION FUNCTIONS: DS_FMC_APP_STATE_ENABLING_TUNNEL

===========================================================================*/

/*===========================================================================

  STATE ENTRY FUNCTION:  ds_fmc_app_sm_state_enabling_tunnel_mgr_entry

===========================================================================*/
/*!
    @brief
    Entry function for state machine DS_FMC_APP_SM,
    state DS_FMC_APP_STATE_ENABLING_TUNNEL

    @detail
    Called upon entry into this state of the state machine, with optional
    user-passed payload pointer parameter.  The prior state of the state
    machine is also passed as the prev_state parameter.

    @return
    None

*/
/*=========================================================================*/
void ds_fmc_app_sm_state_enabling_tunnel_mgr_entry
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         prev_state,  /*!< Previous state */
  void                *payload     /*!< Payload pointer */
)
{

  STM_UNUSED( payload );
  STM_UNUSED( prev_state );
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /*-----------------------------------------------------------------------*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  /*-----------------------------------------------------------------------*/

  DS_FMC_APP_LOG_FUNC_EXIT;

} /* ds_fmc_app_sm_state_enabling_tunnel_mgr_entry() */


/*===========================================================================

  STATE EXIT FUNCTION:  ds_fmc_app_sm_state_enabling_tunnel_mgr_exit

===========================================================================*/
/*!
    @brief
    Exit function for state machine DS_FMC_APP_SM,
    state DS_FMC_APP_STATE_ENABLING_TUNNEL

    @detail
    Called upon exit of this state of the state machine, with optional
    user-passed payload pointer parameter.  The impending state of the state
    machine is also passed as the next_state parameter.

    @return
    None

*/
/*=========================================================================*/
void ds_fmc_app_sm_state_enabling_tunnel_mgr_exit
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         next_state,  /*!< Next state */
  void                *payload     /*!< Payload pointer */
)
{

  STM_UNUSED( payload );
  STM_UNUSED( next_state );
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /*-----------------------------------------------------------------------*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  /*-----------------------------------------------------------------------*/

  DS_FMC_APP_LOG_FUNC_EXIT;

} /* ds_fmc_app_sm_state_enabling_tunnel_mgr_exit() */


/*===========================================================================

  TRANSITION FUNCTION:  ds_fmc_app_sm_tunnel_opened

===========================================================================*/
/*!
    @brief
    Transition function for state machine DS_FMC_APP_SM,
    state DS_FMC_APP_STATE_ENABLING_TUNNEL,
    upon receiving input DS_FMC_APP_TUNNEL_OPENED_EV

    @detail
    Called upon receipt of input DS_FMC_APP_TUNNEL_OPENED_EV, with optional
    user-passed payload pointer.

    @return
    Returns the next state that the state machine should transition to
    upon receipt of the input.  This state must be a valid state for this
    state machine.

*/
/*=========================================================================*/
stm_state_t ds_fmc_app_sm_tunnel_opened
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{
  stm_state_t next_state = STM_SAME_STATE; /* Default 'next' state */
  ds_fmc_app_exec_cmd_t *cmd_buf = NULL;   /* To retrieve tunnel params*/


  STM_UNUSED( payload );
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /*-----------------------------------------------------------------------*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  /*-----------------------------------------------------------------------*/

  cmd_buf = (ds_fmc_app_exec_cmd_t *)payload;

  DS_FMC_APP_ASSERT(cmd_buf);

  /*-----------------------------------------------------------------------
    Call the tunnel manager function to configure the modem with the tunnel
    params obtained previously from the tunnel manager
  -----------------------------------------------------------------------*/

  if(ds_fmc_app_tunnel_mgr_config_tunnel_params( 
      &cmd_buf->data.ds_fmc_app_tunnel_mgr_ds ) < 0)
  {
    ds_fmc_app_log_err("ds_fmc_app_sm_tunnel_opened:"
              "ds_fmc_app_tunnel_mgr_config_tunnel_params failed\n");
    next_state = DS_FMC_APP_SM__DS_FMC_APP_STATE_DISABLING_TUNNEL;
  }
  else
  {
    ds_fmc_app_sm.modem_triggered = TRUE;
    ds_fmc_app_log_high("ds_fmc_app_sm_tunnel_opened:"
              "ds_fmc_app_tunnel_mgr_config_tunnel_params Success\n");
    next_state = DS_FMC_APP_SM__DS_FMC_APP_STATE_CONFIGURING_TUNNEL_PARAMS;
  }

  DS_FMC_APP_LOG_FUNC_EXIT;

  return( next_state );

} /* ds_fmc_app_sm_tunnel_opened() */

/*===========================================================================

  TRANSITION FUNCTION:  ds_fmc_app_sm_handle_ext_trig_disable

===========================================================================*/
/*!
    @brief
    Transition function for state machine DS_FMC_APP_SM,
    state DS_FMC_APP_STATE_ENABLING_TUNNEL,
    upon receiving input DS_FMC_APP_EXT_TRIG_DISABLE_EV

    @detail
    Called upon receipt of input DS_FMC_APP_EXT_TRIG_DISABLE_EV, with optional
    user-passed payload pointer.

    @return
    Returns the next state that the state machine should transition to
    upon receipt of the input.  This state must be a valid state for this
    state machine.

*/
/*=========================================================================*/
stm_state_t ds_fmc_app_sm_handle_ext_trig_disable
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{
  stm_state_t next_state = STM_SAME_STATE; /* Default 'next' state */

  STM_UNUSED( payload );
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /*-----------------------------------------------------------------------*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  /*-----------------------------------------------------------------------*/

  /*-----------------------------------------------------------------------
    Move the SM to the DISABLING_TUNNEL state, whereupon, the tunnel mgr
    connections are forced to be disabled because the Call Mgr initiated
    a tear down of the FMC call.
  -----------------------------------------------------------------------*/
  if(!ds_fmc_app_sm.sm_triggered )
  {  
    ds_fmc_app_log_err("ds_fmc_app_sm_handle_ext_trig_disable"
    " FMC_DISABLE trigger on an already disabled state machine\n");
    DS_FMC_APP_LOG_FUNC_EXIT;
    return next_state;
  }

  next_state = DS_FMC_APP_SM__DS_FMC_APP_STATE_DISABLING_TUNNEL;

  DS_FMC_APP_LOG_FUNC_EXIT;

  return( next_state );

} /* ds_fmc_app_sm_handle_ext_trig_disable() */


/*===========================================================================

     (State Machine: DS_FMC_APP_SM)
     STATE ENTRY/EXIT/TRANSITION FUNCTIONS: DS_FMC_APP_STATE_CONFIGURING_TUNNEL_PARAMS

===========================================================================*/

/*===========================================================================

  STATE ENTRY FUNCTION:  ds_fmc_app_sm_state_configuring_tunnel_params_entry

===========================================================================*/
/*!
    @brief
    Entry function for state machine DS_FMC_APP_SM,
    state DS_FMC_APP_STATE_CONFIGURING_TUNNEL_PARAMS

    @detail
    Called upon entry into this state of the state machine, with optional
    user-passed payload pointer parameter.  The prior state of the state
    machine is also passed as the prev_state parameter.

    @return
    None

*/
/*=========================================================================*/
void ds_fmc_app_sm_state_configuring_tunnel_params_entry
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         prev_state,  /*!< Previous state */
  void                *payload     /*!< Payload pointer */
)
{

  STM_UNUSED( payload );
  STM_UNUSED( prev_state );
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /*-----------------------------------------------------------------------*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  /*-----------------------------------------------------------------------*/

  DS_FMC_APP_LOG_FUNC_EXIT;

} /* ds_fmc_app_sm_state_configuring_tunnel_params_entry() */


/*===========================================================================

  STATE EXIT FUNCTION:  ds_fmc_app_sm_state_configuring_tunnel_params_exit

===========================================================================*/
/*!
    @brief
    Exit function for state machine DS_FMC_APP_SM,
    state DS_FMC_APP_STATE_CONFIGURING_TUNNEL_PARAMS

    @detail
    Called upon exit of this state of the state machine, with optional
    user-passed payload pointer parameter.  The impending state of the state
    machine is also passed as the next_state parameter.

    @return
    None

*/
/*=========================================================================*/
void ds_fmc_app_sm_state_configuring_tunnel_params_exit
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         next_state,  /*!< Next state */
  void                *payload     /*!< Payload pointer */
)
{

  STM_UNUSED( payload );
  STM_UNUSED( next_state );
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /*-----------------------------------------------------------------------*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  /*-----------------------------------------------------------------------*/

  DS_FMC_APP_LOG_FUNC_EXIT;

} /* ds_fmc_app_sm_state_configuring_tunnel_params_exit() */


/*===========================================================================

  TRANSITION FUNCTION:  ds_fmc_app_sm_bearer_up

===========================================================================*/
/*!
    @brief
    Transition function for state machine DS_FMC_APP_SM,
    state DS_FMC_APP_STATE_CONFIGURING_TUNNEL_PARAMS,
    upon receiving input DS_FMC_APP_BEARER_UP_EV

    @detail
    Called upon receipt of input DS_FMC_APP_BEARER_UP_EV, with optional
    user-passed payload pointer.

    @return
    Returns the next state that the state machine should transition to
    upon receipt of the input.  This state must be a valid state for this
    state machine.

*/
/*=========================================================================*/
stm_state_t ds_fmc_app_sm_bearer_up
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{
  stm_state_t next_state = STM_SAME_STATE; /* Default 'next' state */

  ds_fmc_app_call_mgr_resp_type *buffer = NULL;

#ifdef _DS_FMC_APP_DEBUG
struct sockaddr_in *inet_addr; 
char addr_buffer[INET_ADDR_MAX_BUF_SIZE];
#endif /*_DS_FMC_APP_DEBUG*/
  STM_UNUSED( payload );
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /*-----------------------------------------------------------------------*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  /*-----------------------------------------------------------------------*/

  /* Clear the buffer and set the bearer status to DISABLED initially. */

  buffer = (ds_fmc_app_call_mgr_resp_type*)
            ds_fmc_app_malloc(sizeof(ds_fmc_app_call_mgr_resp_type));

  DS_FMC_APP_ASSERT(buffer);

  memset(buffer, 0, sizeof(ds_fmc_app_call_mgr_resp_type));
#ifdef _DS_FMC_APP_DEBUG
memset(addr_buffer, 0, INET_ADDR_MAX_BUF_SIZE);
#endif /*_DS_FMC_APP_DEBUG*/
  buffer->ds_fmc_app_fmc_bearer_status = DS_FMC_APP_FMC_BEARER_ENABLED;

  /*-----------------------------------------------------------------------
    Enable the data path for the external entity via the tunnel manager
    and send the corresponding FMC_BEARER_ENABLED indication back to the 
    call manager.
    Note: The tunnel_dest_ip gets automatically populated on Success of
    ds_fmc_app_tunnel_mgr_config_ext_data_path()
  -----------------------------------------------------------------------*/

  if(ds_fmc_app_tunnel_mgr_config_ext_data_path(
      buffer->ds_fmc_app_fmc_bearer_status,
      &buffer->tunnel_dest_ip) < 0)
  {
    ds_fmc_app_log_err("ds_fmc_app_sm_bearer_up:"
              "ds_fmc_app_tunnel_mgr_config_ext_data_path failed\n");
    next_state = DS_FMC_APP_SM__DS_FMC_APP_STATE_DISABLING_TUNNEL;
  }
  else
  {
    ds_fmc_app_log_high("ds_fmc_app_sm_bearer_up: "
       " ds_fmc_app_tunnel_mgr_config_ext_data_path %c successful\n", 
        buffer->ds_fmc_app_fmc_bearer_status);
#ifdef _DS_FMC_APP_DEBUG
    inet_addr = (struct sockaddr_in*) &(buffer->tunnel_dest_ip);

    inet_ntop(AF_INET, &inet_addr->sin_addr, addr_buffer, 
                            INET_ADDR_MAX_BUF_SIZE);

    ds_fmc_app_log_high("ds_fmc_app_tunnel_mgr_config_ext_data_path: "
                        "inet_addr->sin_family %d "
                        "orig inet_addr->sin_addr %d "
                        "inet_addr->sin_addr %s\n", inet_addr->sin_family,
                        inet_addr->sin_addr.s_addr, addr_buffer);
#endif /*_DS_FMC_APP_DEBUG*/
    /* Send the Call Manager, FMC_ENABLED event with an appr. 
       tunnel dset IP */

    if (ds_fmc_app_cm_tx_msg(ds_fmc_app_cm_get_cm_client_fd(),
                            (unsigned char*)buffer,
                             sizeof(ds_fmc_app_call_mgr_resp_type)) < 0 )
    {
      ds_fmc_app_log_err("ds_fmc_app_sm_bearer_up: ds_fmc_app_cm_tx_msg"
                  " %c failed\n", buffer->ds_fmc_app_fmc_bearer_status);
      next_state = DS_FMC_APP_SM__DS_FMC_APP_STATE_DISABLING_TUNNEL;
    }
    else
    {
      ds_fmc_app_log_high("ds_fmc_app_sm_bearer_up: ds_fmc_app_cm_tx_msg"
                  " %c success\n", buffer->ds_fmc_app_fmc_bearer_status);
      next_state = DS_FMC_APP_SM__DS_FMC_APP_STATE_UP;
    }
  }

  ds_fmc_app_free(buffer);
  buffer = NULL;

  DS_FMC_APP_LOG_FUNC_EXIT;

  return( next_state );

} /* ds_fmc_app_sm_bearer_up() */


/*===========================================================================

  TRANSITION FUNCTION:  ds_fmc_app_sm_bearer_down

===========================================================================*/
/*!
    @brief
    Transition function for state machine DS_FMC_APP_SM,
    state DS_FMC_APP_STATE_CONFIGURING_TUNNEL_PARAMS,
    upon receiving input DS_FMC_APP_BEARER_DOWN_EV

    @detail
    Called upon receipt of input DS_FMC_APP_BEARER_DOWN_EV, with optional
    user-passed payload pointer.

    @return
    Returns the next state that the state machine should transition to
    upon receipt of the input.  This state must be a valid state for this
    state machine.

*/
/*=========================================================================*/
stm_state_t ds_fmc_app_sm_bearer_down
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{
  stm_state_t next_state = STM_SAME_STATE; /* Default 'next' state */

  STM_UNUSED( payload );
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /*-----------------------------------------------------------------------*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  /*-----------------------------------------------------------------------*/

  /*-----------------------------------------------------------------------
    Move the SM to the DISABLING_TUNNEL state, whereupon, the tunnel mgr
    connections are forced to be disabled because the modem initiated
    a tear down of the FMC call.
  -----------------------------------------------------------------------*/

  if(!ds_fmc_app_sm.sm_triggered )
  {  
    ds_fmc_app_log_err("ds_fmc_app_sm_bearer_down"
    " FMC_DISABLE trigger on an already disabled state machine\n");
  }
  else
  {
    next_state = DS_FMC_APP_SM__DS_FMC_APP_STATE_DISABLING_TUNNEL;
  }
  DS_FMC_APP_LOG_FUNC_EXIT;

  return( next_state );

} /* ds_fmc_app_sm_bearer_down() */


/*===========================================================================

  TRANSITION FUNCTION:  ds_fmc_app_sm_tunnel_closed

===========================================================================*/
/*!
    @brief
    Transition function for state machine DS_FMC_APP_SM,
    state DS_FMC_APP_STATE_CONFIGURING_TUNNEL_PARAMS,
    upon receiving input DS_FMC_APP_TUNNEL_CLOSED_EV

    @detail
    Called upon receipt of input DS_FMC_APP_TUNNEL_CLOSED_EV, with optional
    user-passed payload pointer.

    @return
    Returns the next state that the state machine should transition to
    upon receipt of the input.  This state must be a valid state for this
    state machine.

*/
/*=========================================================================*/
stm_state_t ds_fmc_app_sm_tunnel_closed
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{
  stm_state_t next_state = STM_SAME_STATE; /* Default 'next' state */

  ds_fmc_app_exec_cmd_t *cmd_buf = NULL;

  ds_fmc_app_call_mgr_resp_type* buffer = NULL;

  STM_UNUSED( payload );
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /*-----------------------------------------------------------------------*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  /*-----------------------------------------------------------------------*/

  /* Verify of the sm hasn't already closed. */
  if(!ds_fmc_app_sm.sm_triggered )
  {  
    ds_fmc_app_log_err("ds_fmc_app_sm_tunnel_closed"
    " Tunnel_closed trigger on an already closed state machine\n");
    DS_FMC_APP_LOG_FUNC_EXIT;
    return( next_state );
  }

  buffer = (ds_fmc_app_call_mgr_resp_type*)
            ds_fmc_app_malloc(sizeof(ds_fmc_app_call_mgr_resp_type));

  DS_FMC_APP_ASSERT(buffer);

  memset(buffer, 0, sizeof(ds_fmc_app_call_mgr_resp_type));

  buffer->ds_fmc_app_fmc_bearer_status = DS_FMC_APP_FMC_BEARER_DISABLED;

  cmd_buf = (ds_fmc_app_exec_cmd_t *)payload;

  DS_FMC_APP_ASSERT(cmd_buf);

  ds_fmc_app_log_high("ds_fmc_app_sm_tunnel_closed:"
  "ds_fmc_app_sm.modem_triggered %d\n", ds_fmc_app_sm.modem_triggered);
  /*-----------------------------------------------------------------------
    Kill the tunnel_mgr_client thread and close the tunnel_mgr client
    socket connection using ds_fmc_app_tunnel_mgr_close_conn()
  -----------------------------------------------------------------------*/
#if 0
  if(ds_fmc_app_tunnel_mgr_close_conn(
      ds_fmc_app_tunnel_mgr_get_tunnel_mgr_client_fd())
      < 0)
  {
    ds_fmc_app_log_err("ds_fmc_app_sm_tunnel_closed:"
         "ds_fmc_app_tunnel_mgr_close_conn failed\n");
  }
#endif /* 0 */
  /*-----------------------------------------------------------------------
    Depending on the ds_fmc_app_sm.modem_triggered, take appr.
    action.

    If the ds_fmc_app_sm.modem_triggered is true, then, the 
    data path is up, for which, the state machine has to wait for the
    FMC_BEARER down indication from the modem via QMI; hence proceed to
    the DS_FMC_APP_SM__DS_FMC_APP_STATE_GOING_DOWN state.

    Else, the data path hasn't yet been brought up, but a tunnel closure
    was initiated. Proceed to inform the CM of the same and move to the
    DS_FMC_APP_SM__DS_FMC_APP_STATE_DOWN state.
  -----------------------------------------------------------------------*/

  if( FALSE == ds_fmc_app_sm.modem_triggered )
  {
    ds_fmc_app_log_high("ds_fmc_app_sm_tunnel_closed:"
              "ds_fmc_app_sm.modem_triggered false\n");

    if (ds_fmc_app_cm_tx_msg(ds_fmc_app_cm_get_cm_client_fd(),
                          (unsigned char*)buffer,
                          sizeof(ds_fmc_app_call_mgr_resp_type)) < 0 )
    {
      ds_fmc_app_log_err( "ds_fmc_app_sm_tunnel_closed:" 
                          " ds_fmc_app_cm_tx_msg %c failed\n", 
                          buffer->ds_fmc_app_fmc_bearer_status);
    }
    else
    {
      ds_fmc_app_log_high( "ds_fmc_app_sm_tunnel_closed: "
                           "ds_fmc_app_cm_tx_msg %c success\n",
                           buffer->ds_fmc_app_fmc_bearer_status );
    }

    next_state = DS_FMC_APP_SM__DS_FMC_APP_STATE_DOWN;
  }
  else
  {

    if(ds_fmc_app_tunnel_mgr_config_tunnel_params(
        &cmd_buf->data.ds_fmc_app_tunnel_mgr_ds) < 0)
    {
      ds_fmc_app_log_err("ds_fmc_app_sm_tunnel_closed:"
                "ds_fmc_app_tunnel_mgr_config_tunnel_params failed\n");

      if (ds_fmc_app_cm_tx_msg(ds_fmc_app_cm_get_cm_client_fd(),
                            (unsigned char*)buffer,
                            sizeof(ds_fmc_app_call_mgr_resp_type)) < 0 )
      {
        ds_fmc_app_log_err( "ds_fmc_app_sm_tunnel_closed:" 
                            " ds_fmc_app_cm_tx_msg %c failed\n", 
                            buffer->ds_fmc_app_fmc_bearer_status);
      }
      else
      {
        ds_fmc_app_log_high( "ds_fmc_app_sm_tunnel_closed:"
                             " ds_fmc_app_cm_tx_msg %c success\n",
                             buffer->ds_fmc_app_fmc_bearer_status );
      }

      next_state = DS_FMC_APP_SM__DS_FMC_APP_STATE_DOWN;

    }
    else
    {
      ds_fmc_app_log_high("ds_fmc_app_sm_tunnel_closed:"
                          "ds_fmc_app_tunnel_mgr_config_tunnel_params Success\n");
      next_state = DS_FMC_APP_SM__DS_FMC_APP_STATE_GOING_DOWN;
    }
  }

  ds_fmc_app_sm.modem_triggered = FALSE;
  ds_fmc_app_free(buffer);
  buffer = NULL;

  DS_FMC_APP_LOG_FUNC_EXIT;

  return( next_state );

} /* ds_fmc_app_sm_tunnel_closed() */


/*===========================================================================

     (State Machine: DS_FMC_APP_SM)
     STATE ENTRY/EXIT/TRANSITION FUNCTIONS: DS_FMC_APP_STATE_UP

===========================================================================*/

/*===========================================================================

  STATE ENTRY FUNCTION:  ds_fmc_app_sm_state_up_entry

===========================================================================*/
/*!
    @brief
    Entry function for state machine DS_FMC_APP_SM,
    state DS_FMC_APP_STATE_UP

    @detail
    Called upon entry into this state of the state machine, with optional
    user-passed payload pointer parameter.  The prior state of the state
    machine is also passed as the prev_state parameter.

    @return
    None

*/
/*=========================================================================*/
void ds_fmc_app_sm_state_up_entry
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         prev_state,  /*!< Previous state */
  void                *payload     /*!< Payload pointer */
)
{

  STM_UNUSED( payload );
  STM_UNUSED( prev_state );
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /*-----------------------------------------------------------------------*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  /*-----------------------------------------------------------------------*/

  DS_FMC_APP_LOG_FUNC_EXIT;

} /* ds_fmc_app_sm_state_up_entry() */


/*===========================================================================

  STATE EXIT FUNCTION:  ds_fmc_app_sm_state_up_exit

===========================================================================*/
/*!
    @brief
    Exit function for state machine DS_FMC_APP_SM,
    state DS_FMC_APP_STATE_UP

    @detail
    Called upon exit of this state of the state machine, with optional
    user-passed payload pointer parameter.  The impending state of the state
    machine is also passed as the next_state parameter.

    @return
    None

*/
/*=========================================================================*/
void ds_fmc_app_sm_state_up_exit
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         next_state,  /*!< Next state */
  void                *payload     /*!< Payload pointer */
)
{

  STM_UNUSED( payload );
  STM_UNUSED( next_state );
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /*-----------------------------------------------------------------------*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  /*-----------------------------------------------------------------------*/

  DS_FMC_APP_LOG_FUNC_EXIT;

} /* ds_fmc_app_sm_state_up_exit() */


/*===========================================================================

     (State Machine: DS_FMC_APP_SM)
     STATE ENTRY/EXIT/TRANSITION FUNCTIONS: DS_FMC_APP_STATE_DISABLING_TUNNEL

===========================================================================*/

/*===========================================================================

  STATE ENTRY FUNCTION:  ds_fmc_app_sm_state_disabling_tunnel_mgr_entry

===========================================================================*/
/*!
    @brief
    Entry function for state machine DS_FMC_APP_SM,
    state DS_FMC_APP_STATE_DISABLING_TUNNEL

    @detail
    Called upon entry into this state of the state machine, with optional
    user-passed payload pointer parameter.  The prior state of the state
    machine is also passed as the prev_state parameter.

    @return
    None

*/
/*=========================================================================*/
void ds_fmc_app_sm_state_disabling_tunnel_mgr_entry
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         prev_state,  /*!< Previous state */
  void                *payload     /*!< Payload pointer */
)
{

  ds_fmc_app_fmc_bearer_status_type_t buffer = DS_FMC_APP_FMC_BEARER_DISABLED;

  STM_UNUSED( payload );
  STM_UNUSED( prev_state );
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /*-----------------------------------------------------------------------*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  /*-----------------------------------------------------------------------*/

  /*-----------------------------------------------------------------------
    Call the tunnel manager module to initiate the disabling the tunnel
  -----------------------------------------------------------------------*/

  if( ds_fmc_app_tunnel_mgr_client_tx_msg(
                    ds_fmc_app_tunnel_mgr_get_tunnel_mgr_client_fd(),
                    (unsigned char*)&buffer,
                         sizeof(ds_fmc_app_fmc_bearer_status_type_t)) < 0)
  {
    ds_fmc_app_log_err("ds_fmc_app_sm_state_disabling_ims_tunnel_entry: "
          "ds_fmc_app_tunnel_mgr_client_tx_msg %c failed\n", buffer);
  }
  else
  {
    ds_fmc_app_log_high("ds_fmc_app_sm_state_disabling_ims_tunnel_entry:"
          " ds_fmc_app_tunnel_mgr_client_tx_msg %c success\n", buffer);

  }

  DS_FMC_APP_LOG_FUNC_EXIT;

} /* ds_fmc_app_sm_state_disabling_tunnel_mgr_entry() */


/*===========================================================================

  STATE EXIT FUNCTION:  ds_fmc_app_sm_state_disabling_tunnel_mgr_exit

===========================================================================*/
/*!
    @brief
    Exit function for state machine DS_FMC_APP_SM,
    state DS_FMC_APP_STATE_DISABLING_TUNNEL

    @detail
    Called upon exit of this state of the state machine, with optional
    user-passed payload pointer parameter.  The impending state of the state
    machine is also passed as the next_state parameter.

    @return
    None

*/
/*=========================================================================*/
void ds_fmc_app_sm_state_disabling_tunnel_mgr_exit
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         next_state,  /*!< Next state */
  void                *payload     /*!< Payload pointer */
)
{

  STM_UNUSED( payload );
  STM_UNUSED( next_state );
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /*-----------------------------------------------------------------------*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  /*-----------------------------------------------------------------------*/

  DS_FMC_APP_LOG_FUNC_EXIT;

} /* ds_fmc_app_sm_state_disabling_tunnel_mgr_exit() */


/*===========================================================================

     (State Machine: DS_FMC_APP_SM)
     STATE ENTRY/EXIT/TRANSITION FUNCTIONS: DS_FMC_APP_STATE_GOING_DOWN

===========================================================================*/

/*===========================================================================

  STATE ENTRY FUNCTION:  ds_fmc_app_sm_state_going_down_entry

===========================================================================*/
/*!
    @brief
    Entry function for state machine DS_FMC_APP_SM,
    state DS_FMC_APP_STATE_GOING_DOWN

    @detail
    Called upon entry into this state of the state machine, with optional
    user-passed payload pointer parameter.  The prior state of the state
    machine is also passed as the prev_state parameter.

    @return
    None

*/
/*=========================================================================*/
void ds_fmc_app_sm_state_going_down_entry
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         prev_state,  /*!< Previous state */
  void                *payload     /*!< Payload pointer */
)
{

  STM_UNUSED( payload );
  STM_UNUSED( prev_state );
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /*-----------------------------------------------------------------------*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  /*-----------------------------------------------------------------------*/

  DS_FMC_APP_LOG_FUNC_EXIT;

} /* ds_fmc_app_sm_state_going_down_entry() */


/*===========================================================================

  STATE EXIT FUNCTION:  ds_fmc_app_sm_state_going_down_exit

===========================================================================*/
/*!
    @brief
    Exit function for state machine DS_FMC_APP_SM,
    state DS_FMC_APP_STATE_GOING_DOWN

    @detail
    Called upon exit of this state of the state machine, with optional
    user-passed payload pointer parameter.  The impending state of the state
    machine is also passed as the next_state parameter.

    @return
    None

*/
/*=========================================================================*/
void ds_fmc_app_sm_state_going_down_exit
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         next_state,  /*!< Next state */
  void                *payload     /*!< Payload pointer */
)
{

  STM_UNUSED( payload );
  STM_UNUSED( next_state );
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /*-----------------------------------------------------------------------*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  /*-----------------------------------------------------------------------*/

  DS_FMC_APP_LOG_FUNC_EXIT;

} /* ds_fmc_app_sm_state_going_down_exit() */


/*===========================================================================

  TRANSITION FUNCTION:  ds_fmc_app_sm_bearer_down_complete

===========================================================================*/
/*!
    @brief
    Transition function for state machine DS_FMC_APP_SM,
    state DS_FMC_APP_STATE_GOING_DOWN,
    upon receiving input DS_FMC_APP_BEARER_DOWN_EV

    @detail
    Called upon receipt of input DS_FMC_APP_BEARER_DOWN_EV, with optional
    user-passed payload pointer.

    @return
    Returns the next state that the state machine should transition to
    upon receipt of the input.  This state must be a valid state for this
    state machine.

*/
/*=========================================================================*/
stm_state_t ds_fmc_app_sm_bearer_down_complete
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{
  stm_state_t next_state = STM_SAME_STATE; /* Default 'next' state */

  ds_fmc_app_call_mgr_resp_type* buffer = NULL;

  ds_fmc_app_exec_cmd_t *cmd_buf = NULL;
 
  STM_UNUSED( payload );
  DS_FMC_APP_LOG_FUNC_ENTRY;

  /*-----------------------------------------------------------------------*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );
  /*-----------------------------------------------------------------------*/

  if(!ds_fmc_app_sm.sm_triggered )
  {  
    ds_fmc_app_log_err("ds_fmc_app_sm_bearer_down_complete"
    " FMC_DISABLE trigger on an already disabled state machine\n");
    DS_FMC_APP_LOG_FUNC_EXIT;
    return next_state;
  }

  buffer = (ds_fmc_app_call_mgr_resp_type*)
            ds_fmc_app_malloc(sizeof(ds_fmc_app_call_mgr_resp_type));

  DS_FMC_APP_ASSERT(buffer);

  cmd_buf = (ds_fmc_app_exec_cmd_t *)payload;

  DS_FMC_APP_ASSERT(cmd_buf);


  memset(buffer, 0, sizeof(ds_fmc_app_call_mgr_resp_type));

  buffer->ds_fmc_app_fmc_bearer_status = DS_FMC_APP_FMC_BEARER_DISABLED;

  /*-----------------------------------------------------------------------
    When an FMC_BEARER_DOWN indication is received from the modem in the
    GOING_DOWN state, then the following actions have to be undertaken:
    a) The data path previously opened when a BEARER_UP indication was
       received has to be closed.
    b) The Call Mgr has to be conveyed the fact that the FMC_BEARER has
       gone down.
    c) The SM has to move to the DOWN state, where it will be 
       re-initailized for the next FMC bringup call.
  -----------------------------------------------------------------------*/
  if(cmd_buf->data.ext_data_path_conn_status)
  {
    if(ds_fmc_app_tunnel_mgr_config_ext_data_path(
        buffer->ds_fmc_app_fmc_bearer_status,
        &buffer->tunnel_dest_ip ) < 0)
    {
      ds_fmc_app_log_err("ds_fmc_app_sm_bearer_down_complete:"
                         "ds_fmc_app_tunnel_mgr_config_ext_data_path failed\n");
    }
    else
    {
      ds_fmc_app_log_high("ds_fmc_app_sm_bearer_down_complete: "
             " ds_fmc_app_tunnel_mgr_config_ext_data_path %c successful\n",
                   buffer->ds_fmc_app_fmc_bearer_status );
    }
  }

  if (ds_fmc_app_cm_tx_msg(ds_fmc_app_cm_get_cm_client_fd(),
                          (unsigned char*)buffer,
                           sizeof(ds_fmc_app_call_mgr_resp_type)) < 0 )
  {
      ds_fmc_app_log_err("ds_fmc_app_sm_bearer_down_complete:"
                         " ds_fmc_app_cm_tx_msg %c failed\n", 
                         buffer->ds_fmc_app_fmc_bearer_status);
  }
  else
  {
    ds_fmc_app_log_high("ds_fmc_app_sm_bearer_down_complete: "
                        "ds_fmc_app_cm_tx_msg %c success\n", 
                        buffer->ds_fmc_app_fmc_bearer_status);
  }

  /* Pretty much done at this point, the only way to go is DOWN...*/
  next_state = DS_FMC_APP_SM__DS_FMC_APP_STATE_DOWN;

  ds_fmc_app_free(buffer);
  buffer = NULL;

  DS_FMC_APP_LOG_FUNC_EXIT;

  return( next_state );

} /* ds_fmc_app_sm_bearer_down_complete() */


/*===========================================================================

  STATE MACHINE ERROR HOOK FUNCTION: ds_fmc_app_sm_error_hook

===========================================================================*/
void ds_fmc_app_sm_error_hook
(
  stm_status_t error,
  const char *filename,
  uint32 line,
  struct stm_state_machine_s *sm
)
{
  (void)error; (void)filename; (void)line; (void)sm;

  /* STUB */
  return;
}


/*===========================================================================

  STATE MACHINE DEBUG HOOK FUNCTION:  ds_fmc_app_sm_debug_hook

===========================================================================*/
void ds_fmc_app_sm_debug_hook
(
  stm_debug_event_t debug_event,
  struct stm_state_machine_s *sm,
  stm_state_t state_info,
  void *payload
)
{
  (void)payload;

  if( ds_fmc_app_main_cfg.debug ) {
    /* NOTE: the ENTRY/EXIT states seem reversed here but that is due
     * to STM2 providing the prev/next state value respectively. The
     * ordering here seems to make more sense. */
    switch( debug_event ) {
      case STM_STATE_EXIT_FN:
        ds_fmc_app_log_high("ds_fmc_app_sm_debug_hook: enter state %s\n",
                       stm_get_state_name(sm, state_info));
        break;
      case STM_STATE_ENTRY_FN:
        ds_fmc_app_log_high("ds_fmc_app_sm_debug_hook: exit state %s\n",
                       stm_get_state_name(sm, state_info));
        break;
      default:
        break;
    }
  }
  return;
}
