/*!
  @file
  ds_qmh_sm.c

  @brief
  This module contains the entry, exit, and transition functions
  necessary to implement the following state machines:

  @detail
  DSQMH_SM ( 5 instance/s )


  OPTIONAL further detailed description of state machines
  - DELETE this section if unused.

*/

/*===========================================================================

Copyright (c) 2008 -  2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

===========================================================================*/


/*===========================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

$Header: //source/qcom/qct/modem/datahlos/interface/qmiifacectls/rel/11.01.01/src/ds_qmh_sm.c#6 $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
10/19/10    sy     Replaced DCC cmd bufs with client allocated memory.
10/07/10   sy      Updated dsqmhsm_info_code_lookup with new info_codes.
10/07/10   sy      Assigning correct so_mask on receiving DATA_CAPABILITIES_IND
10/04/10   sy      Convert qmi info_code to ps_extended_info_code_enum_type.
10/04/10   sy      Added support for network supported profiles change ind.
10/04/10   sy      Add support for QMI_WDS_EVENT_DATA_CAPABILITIES_IND
                   and QMI_QOS_SRVC_NW_STATUS_REPORT_IND_MSG.
09/30/09   ar      Add IPV6 ND control block for ICMPv6 logic
05/28/09   ar      Add support for QMI_WDS_EVENT_DORM_STATUS_IND
04/15/09   ar      Set QOS flow granted spec on ACTIVATED/MODIFIED events
04/07/09   ar      Check flow is valid when status indication received. 
02/19/09   am      DS Task De-coupling effort and introduction of DCC task.
01/16/09   ar      Fix Lint and MOB integration issues.
01/09/09   ar      Added IPV6 address support.
07/21/08   ar      Fix iface ACL info init; save QMI QOS ID when posted.
05/06/08   ar      Created module/initial version.

===========================================================================*/

/*===========================================================================

                           INCLUDE FILES

===========================================================================*/

#include "comdef.h"
#include "customer.h"

#if defined (FEATURE_DATA_PS) && defined (FEATURE_QMI_CLIENT)

/* Include STM external API */
#include "stm2.h"

#include "ps_iface.h"
#include "ps_crit_sect.h"
#include "ps_route.h"
#include "ps_flowi.h"
#include "ps_phys_link.h"

#ifdef FEATURE_DATA_PS_IPV6
#include "ps_iface_addr_v6.h"
#endif

#include "ds_qmh_sm_ext.h"
#include "ds_qmhi.h"
#include "ds_qmh_llif.h"
#include "ds_qmh_hdlr.h"
#include "ds_qmh_netplat.h"
#include "ds_flow_control.h"

/*===========================================================================

         STM COMPILER GENERATED PROTOTYPES AND DATA STRUCTURES

===========================================================================*/
#include "ds_qmh_sm_int.h"

/*===========================================================================

                         LOCAL VARIABLES

===========================================================================*/


/*===========================================================================

                 STATE MACHINE: DSQMH_SM

===========================================================================*/

/*===========================================================================

  STATE MACHINE ENTRY FUNCTION:  dsqmhsm_sm_entry

===========================================================================*/
/*!
    @brief
    Entry function for state machine DSQMH_SM

    @detail
    Called upon activation of this state machine, with optional
    user-passed payload pointer parameter.

    @return
    None

*/
/*=========================================================================*/
void dsqmhsm_sm_entry
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{
  uint32 iface_inst = (uint32)payload;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_LOW( "DSQMHSM SM ENTRY: %d",iface_inst,0,0 );

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  /*-------------------------------------------------------------------------
    Initialize the specified Proxy IFACE instance
  -------------------------------------------------------------------------*/
  if( DSQMH_SUCCESS != dsqmhhdlr_init_iface_cmd( iface_inst ) )
  {
    DSQMH_MSG_FATAL( "DSQMHSM failed to initialize iface: %d",iface_inst,0,0);
  }

  /*-------------------------------------------------------------------------
    Preserve IFACE instance in SM user data for later reference
  -------------------------------------------------------------------------*/
  stm_set_user_data( sm, (void*)iface_inst );

} /* dsqmhsm_sm_entry() */


/*===========================================================================

  STATE MACHINE EXIT FUNCTION:  dsqmhsm_sm_exit

===========================================================================*/
/*!
    @brief
    Exit function for state machine DSQMH_SM

    @detail
    Called upon deactivation of this state machine, with optional
    user-passed payload pointer parameter.

    @return
    None

*/
/*=========================================================================*/
void dsqmhsm_sm_exit
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_UNUSED( payload );

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  DSQMH_MSG_LOW( "DSQMH SM exiting state machine: %d",
                 (uint32)stm_get_user_data(sm),0,0 );

} /* dsqmhsm_sm_exit() */


/*===========================================================================

  STATE MACHINE ERROR HOOK FUNCTION:  dsqmhsm_error_hook

===========================================================================*/
void dsqmhsm_error_hook
(
  stm_status_t error,
  const char *filename,
  uint32 line,
  struct stm_state_machine_s *sm
)
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_UNUSED( filename );
  STM_UNUSED( line );

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  DSQMH_MSG_FATAL( "DSQMH SM unsupported error:%d sm:%p userdata:%d",
                   error, sm, (uint32)stm_get_user_data(sm) );
}

/*===========================================================================

  STATE MACHINE DEBUG HOOK FUNCTION:  dsqmhsm_debug_hook

===========================================================================*/
void dsqmhsm_debug_hook
(
  stm_debug_event_t debug_event,
  struct stm_state_machine_s *sm,
  stm_state_t state_info,
  void *payload
)
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_UNUSED( debug_event );
  STM_UNUSED( state_info );
  STM_UNUSED( payload );

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );
}


/*===========================================================================

     (State Machine: DSQMH_SM)
     STATE ENTRY/EXIT/TRANSITION FUNCTIONS: DSPROXY_IFACE_STATE_DISABLED

===========================================================================*/

/*===========================================================================

  STATE ENTRY FUNCTION:  dsqmhsm_state_disabled_entry

===========================================================================*/
/*!
    @brief
    Entry function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_DISABLED

    @detail
    Called upon entry into this state of the state machine, with optional
    user-passed payload pointer parameter.  The prior state of the state
    machine is also passed as the prev_state parameter.

    @return
    None

*/
/*=========================================================================*/
void dsqmhsm_state_disabled_entry
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         prev_state,  /*!< Previous state */
  void                *payload     /*!< Payload pointer */
)
{

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );
  STM_UNUSED( prev_state );
  STM_UNUSED( payload );

  DSQMH_MSG_MED( "DSQMH SM entering state DISABLED",0,0,0 );

} /* dsqmhsm_state_disabled_entry() */


/*===========================================================================

  STATE EXIT FUNCTION:  dsqmhsm_state_disabled_exit

===========================================================================*/
/*!
    @brief
    Exit function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_DISABLED

    @detail
    Called upon exit of this state of the state machine, with optional
    user-passed payload pointer parameter.  The impending state of the state
    machine is also passed as the next_state parameter.

    @return
    None

*/
/*=========================================================================*/
void dsqmhsm_state_disabled_exit
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         next_state,  /*!< Next state */
  void                *payload     /*!< Payload pointer */
)
{

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_UNUSED( payload );
  STM_UNUSED( next_state );

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  DSQMH_MSG_LOW( "DSQMH SM exiting state DISABLED",0,0,0 );

} /* dsqmhsm_state_disabled_exit() */


/*===========================================================================

  TRANSITION FUNCTION:  dsqmhsm_modem_init_ind_hdlr

===========================================================================*/
/*!
    @brief
    Transition function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_DISABLED,
    upon receiving input PROXY_IFACE_MODEM_INIT_IND

    @detail
    Called upon receipt of input PROXY_IFACE_MODEM_INIT_IND, with optional
    user-passed payload pointer.

    @return
    Returns the next state that the state machine should transition to
    upon receipt of the input.  This state must be a valid state for this
    state machine.

*/
/*=========================================================================*/
 stm_state_t dsqmhsm_modem_init_ind_hdlr
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer     */
  void                *payload     /*!< Payload pointer                    */
)
{
  stm_state_t next_state = STM_SAME_STATE;  /* Default 'next' state        */
  dsqmh_msg_buf_type    *msg_ptr = NULL;
  ps_iface_type         *iface_ptr = NULL;
  ps_phys_link_type     *phys_link_ptr = NULL;
  int32                  err_code = 0;
  int                    result;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );
  STM_NULL_CHECK( payload );

  DSQMH_MSG_MED( "DSQMH SM iface modem init ind hdlr",0,0,0 );

  msg_ptr = (dsqmh_msg_buf_type*)payload;
  iface_ptr = DSQMH_GET_IFACE_PTR( msg_ptr->iface_inst );

  /*-----------------------------------------------------------------------
    Verify state of PS IFACE
  -----------------------------------------------------------------------*/
  if( !PS_IFACE_IS_VALID( iface_ptr ) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM iface_ptr invalid", 0, 0, 0 );
    return( next_state );
  }

  if( IFACE_DISABLED != ps_iface_state( iface_ptr ) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM invalid iface 0x%x state for inst: %d",
                     ps_iface_state (iface_ptr), msg_ptr->iface_inst, 0);
    return( next_state );
  }

  /*-----------------------------------------------------------------------
    Verify state of primary PS Phys Link
  -----------------------------------------------------------------------*/
  phys_link_ptr = PS_IFACE_GET_PHYS_LINK( iface_ptr );

  if( !PS_PHYS_LINK_IS_VALID( phys_link_ptr ) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM phys_link_ptr invalid", 0, 0, 0 );
    return( next_state );
  }

  /*-----------------------------------------------------------------------
    Initialize the QMI Message Library services. This must be done after
    all the QMIMSGLIB is initialized and all ports have been inited.
  -----------------------------------------------------------------------*/
  DSQMH_MSG_MED( "DSQMH MSG DISPATCHER: cmd id=%d",
                 msg_ptr->msg_id, 0, 0 );

  if( DSQMH_SUCCESS != dsqmhllif_init_qmi_services( msg_ptr->iface_inst,
                                                    &err_code ) )
  {
    DSQMH_MSG_FATAL( "DSQMH failed to initialize QMI services"
                     "Err code 0x%x", err_code, 0, 0);
    return( next_state );
  }

  /*-----------------------------------------------------------------------
    Post enable indication on associated PS iface.
  -----------------------------------------------------------------------*/
  ps_iface_enable_ind( iface_ptr );

  /*-----------------------------------------------------------------------
    Enable linger on the interface.
  -----------------------------------------------------------------------*/
  result = ps_iface_enable_iface_linger( iface_ptr );
  if(-1 == result)
  {
    DSQMH_MSG_ERROR( "QMH HDLR cannot enable linger", 0, 0, 0 );
    result = DSQMH_FAILED;
  }

  dsqmh_state_info.conn_init = TRUE;
  next_state = DSPROXY_IFACE_STATE_DOWN;

  return next_state;

} /* dsqmhsm_modem_init_ind_hdlr() */


/*===========================================================================

     (State Machine: DSQMH_SM)
     STATE ENTRY/EXIT/TRANSITION FUNCTIONS: DSPROXY_IFACE_STATE_DOWN

===========================================================================*/

/*===========================================================================

  STATE ENTRY FUNCTION:  dsqmhsm_state_down_entry

===========================================================================*/
/*!
    @brief
    Entry function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_DOWN

    @detail
    Called upon entry into this state of the state machine, with optional
    user-passed payload pointer parameter.  The prior state of the state
    machine is also passed as the prev_state parameter.

    @return
    None

*/
/*=========================================================================*/
void dsqmhsm_state_down_entry
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         prev_state,  /*!< Previous state */
  void                *payload     /*!< Payload pointer */
)
{
  dsqmh_msg_buf_type    *msg_ptr = NULL;
  dsqmh_iface_cblk_type *cblk_ptr = NULL;
  ps_iface_type         *iface_ptr = NULL;
#ifndef FEATURE_DSS_LINUX
  dsqmh_smd_info_type   *smd_ptr = NULL;
  uint32 phys_link = DSQMH_DEFAULT_IFACE_PHYSLINK;
#endif
  int iface_inst;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  DSQMH_MSG_MED( "DSQMH SM entering state DOWN",0,0,0 );

  /* Check for powerup condition. */
  if( STM_DEACTIVATED_STATE == prev_state)
  {
    iface_inst = (uint32)payload;

    DSQMHLLIF_ENTER_SMD_CRIT_SECTION();
    (DSQMH_GET_CBLK_PTR( iface_inst ))->down_reason =
      PS_NET_DOWN_REASON_NOT_SPECIFIED;
    DSQMHLLIF_EXIT_SMD_CRIT_SECTION();
  }
  else
  {
    STM_NULL_CHECK( payload );

    msg_ptr = (dsqmh_msg_buf_type*)payload;
    iface_inst = msg_ptr->iface_inst;
  }
  cblk_ptr = DSQMH_GET_CBLK_PTR( iface_inst );
  iface_ptr = DSQMH_GET_IFACE_PTR( iface_inst );
#ifndef FEATURE_DSS_LINUX
  smd_ptr = DSQMH_GET_SMD_INFO_PTR( iface_inst, phys_link );
  if( !iface_ptr || !smd_ptr )
#else
  if( !iface_ptr )
#endif
  {
    DSQMH_MSG_ERROR( "Invalid iface inst specified: %d", iface_inst, 0, 0 );
    return;
  }

  /*-----------------------------------------------------------------------
    Reset Proxy IFACE control block on DOWN transition
  -----------------------------------------------------------------------*/
  /* Processor and RmNET instance handle are fixed for each IFACE */
  DSQMHLLIF_ENTER_SMD_CRIT_SECTION();
  cblk_ptr->policy_info.proc_id             = dsqmh_config_get_proc_id(iface_inst);
  cblk_ptr->policy_info.rt_result.rm_hndl   = iface_inst; // same offset
  cblk_ptr->policy_info.rt_result.if_hndl   = DSQMH_INVALID_IFACEID;
  cblk_ptr->policy_info.rt_result.priority  = (uint32)ACL_DEFAULT_CLASS;
  cblk_ptr->reconfig_req                    = FALSE;
  cblk_ptr->phys_link_info[DSQMH_DEFAULT_IFACE_PHYSLINK].bringup_aborted = FALSE;
  cblk_ptr->phys_link_info[DSQMH_DEFAULT_IFACE_PHYSLINK].is_dormant = FALSE;
#ifndef FEATURE_DSS_LINUX
#ifdef FEATURE_DATA_PS_IPV6
  cblk_ptr->ipv6_info.dad_req               = FALSE;
#endif
#endif /* FEATURE_DSS_LINUX */
   DSQMHLLIF_EXIT_SMD_CRIT_SECTION();
  /* Reset IFACE name to default */
  iface_ptr->name = SIO_IFACE;

#ifndef FEATURE_DSS_LINUX
  /*-----------------------------------------------------------------------
    Clear watermark queues of any pending packets
  -----------------------------------------------------------------------*/
  dsm_empty_queue( &smd_ptr->ps_tx_wm );
  dsm_empty_queue( &smd_ptr->ps_rx_wm );
#endif

  DSQMH_MSG_HIGH( "DSQMH SM iface down: %d", iface_inst,0,0 );

} /* dsqmhsm_state_down_entry() */


/*===========================================================================

  STATE EXIT FUNCTION:  dsqmhsm_state_down_exit

===========================================================================*/
/*!
    @brief
    Exit function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_DOWN

    @detail
    Called upon exit of this state of the state machine, with optional
    user-passed payload pointer parameter.  The impending state of the state
    machine is also passed as the next_state parameter.

    @return
    None

*/
/*=========================================================================*/
void dsqmhsm_state_down_exit
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         next_state,  /*!< Next state */
  void                *payload     /*!< Payload pointer */
)
{
  dsqmh_iface_cblk_type *cblk_ptr = NULL;
  dsqmh_msg_buf_type * msg_ptr;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_UNUSED( payload );
  STM_UNUSED( next_state );

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  DSQMH_MSG_LOW( "DSQMH SM exiting state DOWN",0,0,0 );

  msg_ptr = (dsqmh_msg_buf_type*)payload;
  cblk_ptr = DSQMH_GET_CBLK_PTR( msg_ptr->iface_inst );

  /* Initialize the call end reason */
  DSQMHLLIF_ENTER_SMD_CRIT_SECTION();
  cblk_ptr->down_reason = PS_NET_DOWN_REASON_NOT_SPECIFIED;
  DSQMHLLIF_EXIT_SMD_CRIT_SECTION();

} /* dsqmhsm_state_down_exit() */


/*===========================================================================

  TRANSITION FUNCTION:  dsqmhsm_iface_bringup_hdlr

===========================================================================*/
/*!
    @brief
    Transition function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_DOWN,
    upon receiving input PROXY_IFACE_BRING_UP_CMD

    @detail
    Called upon receipt of input PROXY_IFACE_BRING_UP_CMD, with optional
    user-passed payload pointer.

    @return
    Returns the next state that the state machine should transition to
    upon receipt of the input.  This state must be a valid state for this
    state machine.

*/
/*=========================================================================*/
 stm_state_t dsqmhsm_iface_bringup_hdlr
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer     */
  void                *payload     /*!< Payload pointer                    */
)
{
  stm_state_t next_state = STM_SAME_STATE;  /* Default 'next' state        */
  dsqmh_msg_buf_type    *msg_ptr = NULL;
  ps_iface_type         *iface_ptr = NULL;
  ps_phys_link_type     *phys_link_ptr = NULL;
  dsqmh_iface_cblk_type *cblk_ptr = NULL;
  int32                  result;
  int16                  ps_errno;          /* Errno value                 */

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );
  STM_NULL_CHECK( payload );

  DSQMH_MSG_MED( "DSQMH SM iface bring up hdlr",0,0,0 );

  msg_ptr = (dsqmh_msg_buf_type*)payload;
  iface_ptr = DSQMH_GET_IFACE_PTR( msg_ptr->iface_inst );
  cblk_ptr = DSQMH_GET_CBLK_PTR( msg_ptr->iface_inst );

  /*-----------------------------------------------------------------------
    Verify state of PS IFACE
  -----------------------------------------------------------------------*/
  if( !PS_IFACE_IS_VALID( iface_ptr ) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM iface_ptr invalid", 0, 0, 0 );
    return( next_state );
  }

  if( IFACE_COMING_UP != ps_iface_state( iface_ptr ) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM invalid iface state for inst: %d",
                     msg_ptr->iface_inst,0,0 );
    return( next_state );
  }

  /*-----------------------------------------------------------------------
    Verify state of primary PS Phys Link
  -----------------------------------------------------------------------*/
  phys_link_ptr = PS_IFACE_GET_PHYS_LINK( iface_ptr );

  if( !PS_PHYS_LINK_IS_VALID( phys_link_ptr ) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM phys_link_ptr invalid", 0, 0, 0 );
    return( next_state );
  }

  if( PHYS_LINK_UP == PS_PHYS_LINK_GET_STATE( phys_link_ptr ) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM phys link already UP",0,0,0 );
    return( next_state );
  }

  /*-----------------------------------------------------------------------
    Command PS IFACE to bring up the phys link and default flow.
  -----------------------------------------------------------------------*/
  result = ps_phys_link_up_cmd( phys_link_ptr,
                                &ps_errno,
                                (void*)DSQMH_PHYSLINK_CMD_UP );
  if( 0 == result )
  {
    /* Unexpected phys link in UP state. */
    DSQMH_MSG_ERROR( "DSQMH SM phys link already up inst:%d",
                     (uint32)phys_link_ptr->client_data_ptr,0,0 );
    return( next_state );
  }
  else if ( DS_EWOULDBLOCK != ps_errno )
  {
    /* Any other error terminates processing. */
    ps_flow_go_null_ind( PS_IFACE_GET_DEFAULT_FLOW( iface_ptr ),
                         PS_EIC_NOT_SPECIFIED );

    ps_phys_link_gone_ind( phys_link_ptr );

    ps_iface_set_addr_family( iface_ptr, IP_ANY_ADDR );
    ps_iface_down_ind_ex( iface_ptr, cblk_ptr->down_reason );

    return( next_state );
  }

  /*-----------------------------------------------------------------------
    Transition Proxy IFACE to state COMING_UP
  -----------------------------------------------------------------------*/
  next_state = DSPROXY_IFACE_STATE_COMING_UP;

  return( next_state );

} /* dsqmhsm_iface_bringup_hdlr() */


/*===========================================================================

     (State Machine: DSQMH_SM)
     STATE ENTRY/EXIT/TRANSITION FUNCTIONS: DSPROXY_IFACE_STATE_COMING_UP

===========================================================================*/

/*===========================================================================

  STATE ENTRY FUNCTION:  dsqmhsm_state_comingup_entry

===========================================================================*/
/*!
    @brief
    Entry function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_COMING_UP

    @detail
    Called upon entry into this state of the state machine, with optional
    user-passed payload pointer parameter.  The prior state of the state
    machine is also passed as the prev_state parameter.

    @return
    None

*/
/*=========================================================================*/
void dsqmhsm_state_comingup_entry
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         prev_state,  /*!< Previous state */
  void                *payload     /*!< Payload pointer */
)
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_UNUSED( payload );
  STM_UNUSED( prev_state );

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  DSQMH_MSG_MED( "DSQMH SM entering state COMING_UP",0,0,0 );

} /* dsqmhsm_state_comingup_entry() */


/*===========================================================================

  STATE EXIT FUNCTION:  dsqmhsm_state_comingup_exit

===========================================================================*/
/*!
    @brief
    Exit function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_COMING_UP

    @detail
    Called upon exit of this state of the state machine, with optional
    user-passed payload pointer parameter.  The impending state of the state
    machine is also passed as the next_state parameter.

    @return
    None

*/
/*=========================================================================*/
void dsqmhsm_state_comingup_exit
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         next_state,  /*!< Next state */
  void                *payload     /*!< Payload pointer */
)
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_UNUSED( payload );
  STM_UNUSED( next_state );

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  DSQMH_MSG_LOW( "DSQMH SM exiting state COMING_UP",0,0,0 );

} /* dsqmhsm_state_comingup_exit() */


/*===========================================================================

  TRANSITION FUNCTION:  dsqmhsm_physlink_bringup_hdlr

===========================================================================*/
/*!
    @brief
    Transition function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_COMING_UP,
    upon receiving input PROXY_PHYS_LINK_UP_CMD

    @detail
    Called upon receipt of input PROXY_PHYS_LINK_UP_CMD, with optional
    user-passed payload pointer.

    @return
    Returns the next state that the state machine should transition to
    upon receipt of the input.  This state must be a valid state for this
    state machine.

*/
/*=========================================================================*/
stm_state_t dsqmhsm_physlink_bringup_hdlr
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{
  stm_state_t next_state = STM_SAME_STATE; /* Default 'next' state */
  dsqmh_msg_buf_type        *msg_ptr = NULL;
  ps_iface_type             *iface_ptr = NULL;
  dsqmh_iface_cblk_type     *cblk_ptr = NULL;
  dsqmh_phys_link_info_type *plink_ptr = NULL;
#if 0
  ps_flow_type              *flow_ptr  = NULL;
  void                      *handle= NULL, *new_handle = NULL;
  int16                      ps_errno;
#endif

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );
  STM_UNUSED( payload );

  DSQMH_MSG_MED( "DSQMH SM physlink bring up hdlr",0,0,0 );

  msg_ptr = (dsqmh_msg_buf_type*)payload;
  iface_ptr = DSQMH_GET_IFACE_PTR( msg_ptr->iface_inst );
  cblk_ptr = DSQMH_GET_CBLK_PTR( msg_ptr->iface_inst );
  plink_ptr = DSQMH_GET_PHYS_LINK_INFO_PTR( msg_ptr->iface_inst,
                                            DSQMH_DEFAULT_IFACE_PHYSLINK );

  /*-----------------------------------------------------------------------
    Verify state of PS IFACE.

    If IFACE is in GOING_DOWN state, ignore phys_link_up_cmd. This can
    happen in case when context is dormant and application calls
    dss_write followed by dss_pppclose back to back. Task will get
    phys_link_up_cmd followed by iface_down_cmd.  If IFACE had been
    set to GOING_DOWN state, we should not process phys_link_up_cmd.
  -----------------------------------------------------------------------*/
  if( !PS_IFACE_IS_VALID( iface_ptr ) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM iface_ptr invalid", 0, 0, 0 );
    return( next_state );
  }

  if( IFACE_GOING_DOWN == ps_iface_state( iface_ptr ) )
  {
    DSQMH_MSG_ERROR("DSQMH SM ignoring phys link up cmd, iface in going down state: %d",
                    msg_ptr->iface_inst,0,0);
    DSQMHLLIF_ENTER_SMD_CRIT_SECTION();
    cblk_ptr->phys_link_info[ DSQMH_DEFAULT_IFACE_PHYSLINK ].bringup_aborted = TRUE;
    DSQMHLLIF_EXIT_SMD_CRIT_SECTION();
    return( next_state );
  }

  /*-----------------------------------------------------------------------
    Verify state of specified PS Phys Link
  -----------------------------------------------------------------------*/
  if( !PS_PHYS_LINK_IS_VALID( msg_ptr->phys_link_ptr ) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM phys_link_ptr invalid", 0, 0, 0 );
    return( next_state );
  }

  if( PHYS_LINK_UP == PS_PHYS_LINK_GET_STATE( msg_ptr->phys_link_ptr ) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM phys link already UP",0,0,0 );
    return( next_state );
  }

  /*-----------------------------------------------------------------------
    Check for phys link dormant state. Do not perform QMI transaction
    when exiting dormant state.  Just post indications to properly
    notify client.
  -----------------------------------------------------------------------*/
  if( PHYS_LINK_RESUMING == PS_PHYS_LINK_GET_STATE(msg_ptr->phys_link_ptr) )
  {
    DSQMH_MSG_HIGH( "DSQMH SM physlink exit dormant state: iface=%d",
                    msg_ptr->iface_inst, 0, 0 );
    
#if 0  // TODO: Need to check on suspend requirement 
    /* Loop for all secondary flows; default flow not required */
    PS_ENTER_CRIT_SECTION( &global_ps_crit_section );
    handle = (ps_flow_type*)ps_iface_get_sec_flow_handle( iface_ptr );
    while( handle )
    {
      if( ps_iface_get_sec_flow_by_handle( iface_ptr, handle,
                                           &flow_ptr, &new_handle ) )
      {
        (void)ps_flow_activate_cmd( flow_ptr, &ps_errno, NULL );
        ps_flow_activate_ind( flow_ptr, PS_EIC_NOT_SPECIFIED );
      }
      handle = new_handle;
    }
    PS_LEAVE_CRIT_SECTION( &global_ps_crit_section );
#endif //0

    if( DSQMH_SUCCESS !=
        dsqmhllif_dormancy_manager
        ( 
          DSQMHLLIF_DORMANCY_OP_GOACTIVE,
          (uint32)iface_ptr->client_data_ptr 
        ))
    {
      DSQMH_MSG_ERROR( "DSQMH SM dsqmhllif_dormancy_manager fail, iface = %d",
                        iface_ptr->client_data_ptr, 0, 0);
      ps_phys_link_down_ind_ex( msg_ptr->phys_link_ptr, PS_EIC_NOT_SPECIFIED );
    }

    return( next_state );
  } 


  /*-----------------------------------------------------------------------
    Command QMI to establish network connection.
    QMI will asynchronously post status of modem operation to trigger
    change of state.  For now we will remain in the same state.
  -----------------------------------------------------------------------*/
  if( DSQMH_SUCCESS != dsqmhllif_start_network_cmd( msg_ptr->iface_inst ) )
  {
    DSQMH_MSG_ERROR("DSQMH SM failed to start network: %d",
                    msg_ptr->iface_inst,0,0);
    return( next_state );
  }

  /*-----------------------------------------------------------------------
    Enable flow on the default phys link
  -----------------------------------------------------------------------*/
  ps_phys_link_enable_flow( msg_ptr->phys_link_ptr, DS_FLOW_PROXY_MASK );

  return( next_state );

} /* dsqmhsm_physlink_bringup_hdlr() */


/*===========================================================================

  TRANSITION FUNCTION:  dsqmhsm_physlink_teardown_hdlr

===========================================================================*/
/*!
    @brief
    Transition function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_COMING_UP,
    upon receiving input PROXY_PHYS_LINK_DOWN_CMD

    @detail
    Called upon receipt of input PROXY_PHYS_LINK_DOWN_CMD, with optional
    user-passed payload pointer.

    @return
    Returns the next state that the state machine should transition to
    upon receipt of the input.  This state must be a valid state for this
    state machine.

*/
/*=========================================================================*/
stm_state_t dsqmhsm_physlink_teardown_hdlr
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{
  stm_state_t next_state = STM_SAME_STATE; /* Default 'next' state */
  dsqmh_msg_buf_type        *msg_ptr = NULL;
  ps_iface_type             *iface_ptr = NULL;
  dsqmh_msglib_info_type    *qmi_ptr = NULL;
  dsqmh_iface_cblk_type     *cblk_ptr = NULL;
  dsqmh_phys_link_info_type *plink_ptr = NULL;
  int                        result;
#if 0
  ps_flow_type              *flow_ptr  = NULL;
  void                      *handle= NULL, *new_handle = NULL;
  int16                      ps_errno;
#endif

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );
  STM_UNUSED( payload );

  DSQMH_MSG_MED( "DSQMH SM physlink teardown hdlr",0,0,0 );

  msg_ptr = (dsqmh_msg_buf_type*)payload;
  iface_ptr = DSQMH_GET_IFACE_PTR( msg_ptr->iface_inst );
  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( msg_ptr->iface_inst );
  cblk_ptr = DSQMH_GET_CBLK_PTR( msg_ptr->iface_inst );
  plink_ptr = DSQMH_GET_PHYS_LINK_INFO_PTR( msg_ptr->iface_inst,
                                            DSQMH_DEFAULT_IFACE_PHYSLINK );
  if( !iface_ptr || !qmi_ptr )
  {
    DSQMH_MSG_ERROR( "Invalid iface inst specified: %d",
                     msg_ptr->iface_inst, 0, 0 );
    return( next_state );
  }

  /*-----------------------------------------------------------------------
    Verify state of specified PS Phys Link
  -----------------------------------------------------------------------*/
  if( !PS_PHYS_LINK_IS_VALID( msg_ptr->phys_link_ptr ) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM phys_link_ptr invalid", 0, 0, 0 );
    return( next_state );
  }

  /*-----------------------------------------------------------------------
    Check for aborted Phys Link bringup, which happens when IFACE down
    is received just after Phys Link bringup.  In such cases, there is
    no call active on Modem and no pending QMI transaction.  Just post
    indications to properly cleanup & notify client.
  -----------------------------------------------------------------------*/
  if( cblk_ptr->phys_link_info[ DSQMH_DEFAULT_IFACE_PHYSLINK ].bringup_aborted )
  {
    DSQMHLLIF_ENTER_SMD_CRIT_SECTION();
    cblk_ptr->phys_link_info[ DSQMH_DEFAULT_IFACE_PHYSLINK ].bringup_aborted = FALSE;
    DSQMHLLIF_EXIT_SMD_CRIT_SECTION();

    DSQMH_MSG_HIGH( "DSQMH SM aborted physlink bringup", 0, 0, 0 );

    ps_flow_go_null_ind( PS_IFACE_GET_DEFAULT_FLOW( iface_ptr ),
                         PS_EIC_NOT_SPECIFIED );

    ps_phys_link_gone_ind( msg_ptr->phys_link_ptr );

    ps_iface_set_addr_family( iface_ptr, IP_ANY_ADDR );
    ps_iface_down_ind_ex( iface_ptr, PS_NET_DOWN_REASON_CLIENT_END );

    return( DSPROXY_IFACE_STATE_DOWN );
  }

  /*-----------------------------------------------------------------------
    Check for phys link dormant state. Do not perform QMI transaction
    when entering dormant state.  Just post indications to properly
    notify client. Skip this on FORCED phys link down operation.
  -----------------------------------------------------------------------*/
  if( DSQMH_PHYSLINK_CMD_DOWN_DORMANT == msg_ptr->info.physlink.cmd )
  {
    DSQMH_MSG_HIGH( "DSQMH SM physlink enter dormant state: iface=%d",
                    msg_ptr->iface_inst, 0, 0 );

#if 0  // TODO: Need to check on suspend requirement 
    /* Loop for all secondary flows; default flow not required */
    PS_ENTER_CRIT_SECTION( &global_ps_crit_section );
    handle = (ps_flow_type*)ps_iface_get_sec_flow_handle( iface_ptr );
    while( handle )
    {
      if( ps_iface_get_sec_flow_by_handle( iface_ptr, handle,
                                           &flow_ptr, &new_handle ) )
      {
        (void)ps_flow_suspend_cmd( flow_ptr, &ps_errno, NULL );
        ps_flow_suspend_ind( flow_ptr, PS_EIC_NOT_SPECIFIED );
      }
      handle = new_handle;
    }
    PS_LEAVE_CRIT_SECTION( &global_ps_crit_section );
#endif //0
    
    ps_phys_link_down_ind_ex( msg_ptr->phys_link_ptr, PS_EIC_NOT_SPECIFIED );
    return( next_state );
  }

  
  /*-----------------------------------------------------------------------
    Command QMI to terminate network connection.

    QMI will asynchronously post status of modem operation to trigger
    change of state.  For now we will remain in the same state.
  -----------------------------------------------------------------------*/
  /* Check for pending start network transaction */
  if( DSQMH_INVALID_TXNID != qmi_ptr->wds_txn_handle )
  {
    result = dsqmhllif_abort_cmd( msg_ptr->iface_inst );
  }
  else
  {
    result = dsqmhllif_stop_network_cmd( msg_ptr->iface_inst );
  }
  if( DSQMH_SUCCESS != result )
  {
    DSQMH_MSG_ERROR("DSQMH SM failed to stop network: %d",
                    msg_ptr->iface_inst,0,0);
    return( next_state );
  }

  return( next_state );
} /* dsqmhsm_physlink_teardown_hdlr() */


/*===========================================================================

  TRANSITION FUNCTION:  dsqmhsm_iface_teardown_hdlr

===========================================================================*/
/*!
    @brief
    Transition function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_COMING_UP,
    upon receiving input PROXY_IFACE_TEARDOWN_CMD

    @detail
    Called upon receipt of input PROXY_IFACE_TEARDOWN_CMD, with optional
    user-passed payload pointer.

    @return
    Returns the next state that the state machine should transition to
    upon receipt of the input.  This state must be a valid state for this
    state machine.

*/
/*=========================================================================*/
stm_state_t dsqmhsm_iface_teardown_hdlr
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{
  stm_state_t next_state = STM_SAME_STATE; /* Default 'next' state */
  dsqmh_msg_buf_type *msg_ptr = NULL;
  ps_iface_type      *iface_ptr = NULL;
  ps_phys_link_type  *phys_link_ptr = NULL;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_UNUSED( payload );

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );
  STM_NULL_CHECK( payload );

  DSQMH_MSG_MED( "DSQMH SM iface teardown hdlr",0,0,0 );

  msg_ptr = (dsqmh_msg_buf_type*)payload;
  iface_ptr = DSQMH_GET_IFACE_PTR( msg_ptr->iface_inst );
  phys_link_ptr = PS_IFACE_GET_PHYS_LINK( iface_ptr );

  /*-----------------------------------------------------------------------
    Validate inputs.
  -----------------------------------------------------------------------*/
  if( !PS_IFACE_IS_VALID( iface_ptr ) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM iface_ptr invalid", 0, 0, 0 );
    return( next_state );
  }

  if( !PS_PHYS_LINK_IS_VALID( phys_link_ptr ) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM phys_link_ptr invalid", 0, 0, 0 );
    return( next_state );
  }

  /*-----------------------------------------------------------------------
    Check for SM already in DOWN state.  This can happen when IFACE
    bringup command is immediately followed by teardown.  In such
    cases, the application needs to receive the IFACE DOWN indications.
  -----------------------------------------------------------------------*/
  if( DSPROXY_IFACE_STATE_DOWN == stm_get_state( sm ) )
  {
    DSQMH_MSG_MED( "DSQMH SM iface already in DOWN state", 0, 0, 0 );

    ps_flow_go_null_ind( PS_IFACE_GET_DEFAULT_FLOW( iface_ptr ),
                         PS_EIC_NOT_SPECIFIED );

    ps_phys_link_gone_ind( phys_link_ptr );

    ps_iface_set_addr_family( iface_ptr, IP_ANY_ADDR );
    ps_iface_down_ind_ex( iface_ptr, PS_NET_DOWN_REASON_CLIENT_END );

    return( next_state );
  }

  /*-----------------------------------------------------------------------
    Disable flow on the default phys link
  -----------------------------------------------------------------------*/
  ps_phys_link_disable_flow( phys_link_ptr, DS_FLOW_PROXY_MASK );

  /*-----------------------------------------------------------------------
    Command Network Platform Layer to go down.
    Network Platform Layer will asynchronously post status of operation.
  -----------------------------------------------------------------------*/
  if( DSQMH_SUCCESS != dsqmhllif_netplatform_down_cmd( msg_ptr->iface_inst ) )
  {
    DSQMH_MSG_ERROR("DSQMH SM failed on netplatform down: %d",
                    msg_ptr->iface_inst,0,0);
    return( next_state );
  }

  /*-----------------------------------------------------------------------
    Transition Proxy IFACE to state PLATFORM GOING_DOWN.
  -----------------------------------------------------------------------*/
  next_state = DSPROXY_IFACE_STATE_PLAT_GOING_DOWN;

  return( next_state );

} /* dsqmhsm_iface_teardown_hdlr() */


/*===========================================================================

  TRANSITION FUNCTION:  dsqmhsm_modem_up_ind_hdlr

===========================================================================*/
/*!
    @brief
    Transition function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_COMING_UP,
    upon receiving input PROXY_IFACE_MODEM_UP_IND

    @detail
    Called upon receipt of input PROXY_IFACE_MODEM_UP_IND, with optional
    user-passed payload pointer.

    @return
    Returns the next state that the state machine should transition to
    upon receipt of the input.  This state must be a valid state for this
    state machine.

*/
/*=========================================================================*/
stm_state_t dsqmhsm_modem_up_ind_hdlr
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{
  stm_state_t next_state = STM_SAME_STATE; /* Default 'next' state */
  dsqmh_msg_buf_type    *msg_ptr = NULL;
  ps_iface_type         *iface_ptr = NULL;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );
  STM_NULL_CHECK( payload );

  msg_ptr = (dsqmh_msg_buf_type*)payload;
  iface_ptr = DSQMH_GET_IFACE_PTR( msg_ptr->iface_inst );

  /*-----------------------------------------------------------------------
    Validate inputs.
  -----------------------------------------------------------------------*/
  if( !PS_IFACE_IS_VALID( iface_ptr ) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM iface_ptr invalid", 0, 0, 0 );
    return( next_state );
  }

  /*-----------------------------------------------------------------------
    Check for Modem interface reconfiguration indication.
  -----------------------------------------------------------------------*/
  if( (DSPROXY_IFACE_STATE_UP == stm_get_state( sm )) &&
      msg_ptr->info.qmi_wds.info.pkt_srvc_status.reconfig_required )
  {
    /*---------------------------------------------------------------------
      Transition Proxy IFACE to state CONFIGURING.
    ---------------------------------------------------------------------*/
    next_state = DSPROXY_IFACE_STATE_RECONFIGURING;
  }
  else
  {
    /* Initial UP transition. */
    ps_flow_activate_ind( PS_IFACE_GET_DEFAULT_FLOW( iface_ptr),
                          PS_EIC_NOT_SPECIFIED);
    ps_phys_link_up_ind( PS_IFACE_GET_PHYS_LINK( iface_ptr ) );

    /*---------------------------------------------------------------------
      Command Network Platform Layer to come up.
      Network Platform Layer will asynchronously post status of operation.
    ---------------------------------------------------------------------*/
    if( DSQMH_SUCCESS != dsqmhllif_netplatform_up_cmd( msg_ptr->iface_inst ) )
    {
      DSQMH_MSG_ERROR("DSQMH SM failed on netplatform up: %d",
                      msg_ptr->iface_inst,0,0);
      return( next_state );
    }

    /*-----------------------------------------------------------------------
      Transition Proxy IFACE to state PLATFORM COMING_UP.
    -----------------------------------------------------------------------*/
    next_state = DSPROXY_IFACE_STATE_PLAT_COMING_UP;
  }

  return( next_state );

} /* dsqmhsm_modem_up_ind_hdlr() */


/*===========================================================================

  TRANSITION FUNCTION:  dsqmhsm_modem_down_ind_hdlr

===========================================================================*/
/*!
    @brief
    Transition function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_COMING_UP,
    upon receiving input PROXY_IFACE_MODEM_DOWN_IND

    @detail
    Called upon receipt of input PROXY_IFACE_MODEM_DOWN_IND, with optional
    user-passed payload pointer.

    @return
    Returns the next state that the state machine should transition to
    upon receipt of the input.  This state must be a valid state for this
    state machine.

*/
/*=========================================================================*/
stm_state_t dsqmhsm_modem_down_ind_hdlr
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{
  stm_state_t next_state = STM_SAME_STATE; /* Default 'next' state */
  dsqmh_msg_buf_type * msg_ptr = NULL;
  ps_iface_type      *iface_ptr = NULL;
  ps_phys_link_type  *phys_link_ptr = NULL;
  ps_flow_type       *flow_ptr  = NULL;
  dsqmh_iface_cblk_type *cblk_ptr = NULL;
  void               *handle= NULL, *new_handle = NULL;
  int16               ps_errno;            /* Errno value                 */
  ps_iface_net_down_reason_type down_reason;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );
  STM_NULL_CHECK( payload );

  msg_ptr = (dsqmh_msg_buf_type*)payload;
  iface_ptr = DSQMH_GET_IFACE_PTR( msg_ptr->iface_inst );
  phys_link_ptr = PS_IFACE_GET_PHYS_LINK( iface_ptr );
  cblk_ptr = DSQMH_GET_CBLK_PTR( msg_ptr->iface_inst );
  dsqmhllif_decode_call_end(msg_ptr->info.qmi_wds.info.pkt_srvc_status. \
                            call_end_reason, &down_reason);

  DSQMH_MSG_MED( "DSQMH SM modem down ind hdlr: qmi_end_reason=%d",
                 down_reason,0,0 );

  /*-----------------------------------------------------------------------
    Validate inputs.
  -----------------------------------------------------------------------*/
  if( !PS_IFACE_IS_VALID( iface_ptr ) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM iface_ptr invalid", 0, 0, 0 );
    return( next_state );
  }

  if( !PS_PHYS_LINK_IS_VALID( phys_link_ptr ) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM phys_link_ptr invalid", 0, 0, 0 );
    return( next_state );
  }

#if defined(FEATURE_DATA_PS_IPV6) && defined(FEATURE_DATA_PS_ADDR_MGMT)
  if( ps_iface_addr_family_is_v6( iface_ptr ) )
  {
    DSQMH_MSG_MED("DSQMH SM starting IPV6 address cleanup", 0, 0, 0);
    
    if( DSQMH_SUCCESS !=
        dsqmhhdlr_ipv6_cleanup_hdlr( msg_ptr->iface_inst ) )
    {
      DSQMH_MSG_ERROR("DSQMH SM failed to cleanup: %d",
                      msg_ptr->iface_inst,0,0);
      return( next_state );
    }
  }
#endif /* FEATURE_DATA_PS_IPV6 && FEATURE_DATA_PS_ADDR_MGMT */

  /*-----------------------------------------------------------------------
    Check for secondary flows on the specified IFACE.
  -----------------------------------------------------------------------*/
  if( 0 != PS_IFACE_GET_NUM_SEC_FLOWS( iface_ptr ) )
  {
    PS_ENTER_CRIT_SECTION( &global_ps_crit_section );

    /*---------------------------------------------------------------------
      Terminate all secondary flows.
    ---------------------------------------------------------------------*/
    handle = (ps_flow_type*)ps_iface_get_sec_flow_handle( iface_ptr );
    while( handle )
    {
      if( ps_iface_get_sec_flow_by_handle( iface_ptr, handle,
                                           &flow_ptr, &new_handle ) )
      {
        ps_flow_go_null_ind( flow_ptr, PS_EIC_NOT_SPECIFIED );

        (void)ps_iface_delete_flow( iface_ptr,
                                    flow_ptr,
                                    &ps_errno );
      }
      else
      {
        DSQMH_MSG_ERROR( "DSQMH SM get sec flow error reported",0,0,0 );
        break;
      }
      handle = new_handle;
    }

    PS_LEAVE_CRIT_SECTION( &global_ps_crit_section );
  }

  /*-----------------------------------------------------------------------
    Force default flow on primary phys link to NULL.
  -----------------------------------------------------------------------*/
  ps_flow_go_null_ind( PS_IFACE_GET_DEFAULT_FLOW( iface_ptr ),
                       PS_EIC_NOT_SPECIFIED );

  ps_phys_link_gone_ind( phys_link_ptr );


  /*-----------------------------------------------------------------------
    Check for Modem initiated shutdown while in UP state.  This
    requires explicit Network Platform layer down command to ensure
    proper cleanup.
  -----------------------------------------------------------------------*/
  if( DSPROXY_IFACE_STATE_UP == stm_get_state( sm ) )
  {
    next_state = DSPROXY_IFACE_STATE_GOING_DOWN;

    if( DSQMH_SUCCESS != dsqmhllif_netplatform_down_cmd( msg_ptr->iface_inst ) )
    {
      DSQMH_MSG_ERROR("DSQMH SM failed on netplatform down: %d",
                      msg_ptr->iface_inst,0,0);
    }
  }
  else
  {
    next_state = DSPROXY_IFACE_STATE_DOWN;

    /*-----------------------------------------------------------------------
      Transition Proxy IFACE to state DOWN.
    -----------------------------------------------------------------------*/
    ps_iface_set_addr_family( iface_ptr, IP_ANY_ADDR );
    ps_iface_down_ind_ex( iface_ptr, cblk_ptr->down_reason );
  }

  return( next_state );

} /* dsqmhsm_modem_down_ind_hdlr() */


/*===========================================================================

  TRANSITION FUNCTION:  dsqmhsm_platform_down_ind_hdlr

===========================================================================*/
/*!
    @brief
    Transition function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_COMING_UP,
    upon receiving input PROXY_IFACE_PLATFORM_DOWN_IND

    @detail
    Called upon receipt of input PROXY_IFACE_PLATFORM_DOWN_IND, with optional
    user-passed payload pointer.

    @return
    Returns the next state that the state machine should transition to
    upon receipt of the input.  This state must be a valid state for this
    state machine.

*/
/*=========================================================================*/
stm_state_t dsqmhsm_platform_down_ind_hdlr
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{
  stm_state_t next_state = STM_SAME_STATE; /* Default 'next' state */
  dsqmh_msg_buf_type    *msg_ptr = NULL;
  ps_iface_type         *iface_ptr = NULL;
  ps_phys_link_type     *phys_link_ptr = NULL;
  dsqmh_iface_cblk_type *cblk_ptr = NULL;
  int32                  result;
  int16                  ps_errno = 0;      /* Errno value                 */

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );
  STM_NULL_CHECK( payload );

  DSQMH_MSG_MED( "DSQMH SM platform down ind hdlr",0,0,0 );

  msg_ptr = (dsqmh_msg_buf_type*)payload;
  iface_ptr = DSQMH_GET_IFACE_PTR( msg_ptr->iface_inst );
  cblk_ptr = DSQMH_GET_CBLK_PTR( msg_ptr->iface_inst );

  /*-----------------------------------------------------------------------
    Validate inputs.
  -----------------------------------------------------------------------*/
  if( !PS_IFACE_IS_VALID( iface_ptr ) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM iface_ptr invalid", 0, 0, 0 );
    return( next_state );
  }

  /*-----------------------------------------------------------------------
    Check for Modem initiated shutdown while in UP state case.
    Network Platform layer was sent down command to ensure proper cleanup.
  -----------------------------------------------------------------------*/
  if( DSPROXY_IFACE_STATE_GOING_DOWN == stm_get_state( sm ) )
  {
    /*---------------------------------------------------------------------
      Transition Proxy IFACE to state DOWN.
    ---------------------------------------------------------------------*/
    ps_iface_set_addr_family( iface_ptr, IP_ANY_ADDR );
    ps_iface_down_ind_ex( iface_ptr, cblk_ptr->down_reason );

    next_state = DSPROXY_IFACE_STATE_DOWN;
    return( next_state );
  }

  /*-----------------------------------------------------------------------
    Verify state of primary PS Phys Link
  -----------------------------------------------------------------------*/
  phys_link_ptr = PS_IFACE_GET_PHYS_LINK( iface_ptr );

  if( !PS_PHYS_LINK_IS_VALID( phys_link_ptr ) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM phys_link_ptr invalid", 0, 0, 0 );
    return( next_state );
  }

  if( PHYS_LINK_DOWN == PS_PHYS_LINK_GET_STATE( phys_link_ptr ) )
  {
    /*---------------------------------------------------------------------
      Check for dormant state condition, where Iface will not be in
      DOWN state.  Trigger Iface teardown in this case.
    ---------------------------------------------------------------------*/
    if( IFACE_DOWN != ps_iface_state( iface_ptr ) )
    {
      (void)dsqmhhdlr_phys_link_down_cmd( phys_link_ptr,
                                          (void*)DSQMH_PHYSLINK_CMD_DOWN );
    }
    else
    {
      DSQMH_MSG_ERROR( "DSQMH SM phys link already DOWN",0,0,0 );
      return( next_state );
    }
  }

  /*-----------------------------------------------------------------------
    Command PS IFACE to tear down the phys link and default flow.
  -----------------------------------------------------------------------*/
  result = ps_phys_link_down_cmd( phys_link_ptr,
                                  &ps_errno,
                                  (void*)DSQMH_PHYSLINK_CMD_NULL );
  if( (-1 == result) &&
      ((DS_EWOULDBLOCK != ps_errno) && (DS_ENETCLOSEINPROGRESS != ps_errno)) )
  {
    /* Unexpected phys link in DOWN state. */
    DSQMH_MSG_ERROR( "DSQMH SM error on ps_phys_link_down_cmd:%d",
                     (uint32)phys_link_ptr->client_data_ptr,0,0 );
    return( next_state );
  }

  /*-----------------------------------------------------------------------
    Transition Proxy IFACE to state GOING DOWN.
  -----------------------------------------------------------------------*/
  next_state = DSPROXY_IFACE_STATE_GOING_DOWN;

  return( next_state );

} /* dsqmhsm_platform_down_ind_hdlr() */


/*===========================================================================

     (State Machine: DSQMH_SM)
     STATE ENTRY/EXIT/TRANSITION FUNCTIONS: DSPROXY_IFACE_STATE_PLAT_COMING_UP

===========================================================================*/

/*===========================================================================

  STATE ENTRY FUNCTION:  dsqmhsm_state_platform_comingup_entry

===========================================================================*/
/*!
    @brief
    Entry function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_PLAT_COMING_UP

    @detail
    Called upon entry into this state of the state machine, with optional
    user-passed payload pointer parameter.  The prior state of the state
    machine is also passed as the prev_state parameter.

    @return
    None

*/
/*=========================================================================*/
void dsqmhsm_state_platform_comingup_entry
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         prev_state,  /*!< Previous state */
  void                *payload     /*!< Payload pointer */
)
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_UNUSED( payload );
  STM_UNUSED( prev_state );

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  DSQMH_MSG_MED( "DSQMH SM entering state PLATFORM_COMING_UP",0,0,0 );

} /* dsqmhsm_state_platform_comingup_entry() */


/*===========================================================================

  STATE EXIT FUNCTION:  dsqmhsm_state_platform_comingup_exit

===========================================================================*/
/*!
    @brief
    Exit function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_PLAT_COMING_UP

    @detail
    Called upon exit of this state of the state machine, with optional
    user-passed payload pointer parameter.  The impending state of the state
    machine is also passed as the next_state parameter.

    @return
    None

*/
/*=========================================================================*/
void dsqmhsm_state_platform_comingup_exit
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         next_state,  /*!< Next state */
  void                *payload     /*!< Payload pointer */
)
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_UNUSED( payload );
  STM_UNUSED( next_state );

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  DSQMH_MSG_LOW( "DSQMH SM exiting state PLATFORM_COMING_UP",0,0,0 );

} /* dsqmhsm_state_platform_comingup_exit() */


/*===========================================================================

  TRANSITION FUNCTION:  dsqmhsm_platform_up_ind_hdlr

===========================================================================*/
/*!
    @brief
    Transition function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_PLAT_COMING_UP,
    upon receiving input PROXY_IFACE_PLATFORM_UP_IND

    @detail
    Called upon receipt of input PROXY_IFACE_PLATFORM_UP_IND, with optional
    user-passed payload pointer.

    @return
    Returns the next state that the state machine should transition to
    upon receipt of the input.  This state must be a valid state for this
    state machine.

*/
/*=========================================================================*/
stm_state_t dsqmhsm_platform_up_ind_hdlr
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{
  stm_state_t next_state = STM_SAME_STATE; /* Default 'next' state */

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );
  STM_NULL_CHECK( payload );

  DSQMH_MSG_MED( "DSQMH SM platform up ind hdlr",0,0,0 );

  /*-----------------------------------------------------------------------
    Transition Proxy IFACE to state CONFIGURING.
  -----------------------------------------------------------------------*/
  next_state = DSPROXY_IFACE_STATE_CONFIGURING;
  return( next_state );

} /* dsqmhsm_platform_up_ind_hdlr() */


/*===========================================================================

     (State Machine: DSQMH_SM)
     STATE ENTRY/EXIT/TRANSITION FUNCTIONS: DSPROXY_IFACE_STATE_CONFIGURING

===========================================================================*/

/*===========================================================================

  STATE ENTRY FUNCTION:  dsqmhsm_state_configuring_entry

===========================================================================*/
/*!
    @brief
    Entry function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_CONFIGURING

    @detail
    Called upon entry into this state of the state machine, with optional
    user-passed payload pointer parameter.  The prior state of the state
    machine is also passed as the prev_state parameter.

    @return
    None

*/
/*=========================================================================*/
void dsqmhsm_state_configuring_entry
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         prev_state,  /*!< Previous state */
  void                *payload     /*!< Payload pointer */
)
{
  dsqmh_msg_buf_type    *msg_ptr = NULL;
  dsqmh_iface_cblk_type *cblk_ptr = NULL;
  ps_iface_type         *iface_ptr = NULL;
  dsqmh_runtime_info_type    current_info;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_UNUSED( prev_state );

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );
  STM_NULL_CHECK( payload );

  DSQMH_MSG_MED( "DSQMH SM entering state CONFIGURING",0,0,0 );

  msg_ptr = (dsqmh_msg_buf_type*)payload;
  iface_ptr = DSQMH_GET_IFACE_PTR( msg_ptr->iface_inst );
  cblk_ptr = DSQMH_GET_CBLK_PTR( msg_ptr->iface_inst );

  /*-----------------------------------------------------------------------
    Validate inputs.
  -----------------------------------------------------------------------*/
  if( !PS_IFACE_IS_VALID( iface_ptr ) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM iface_ptr invalid", 0, 0, 0 );
    return;
  }

  /*-----------------------------------------------------------------------
    Notify IFACE clients of configuration event.
  -----------------------------------------------------------------------*/
  ps_iface_configuring_ind( iface_ptr );

  /*-----------------------------------------------------------------------
    Command QMI to query Modem for Um IFACE bearer settings.
  -----------------------------------------------------------------------*/
  if( DSQMH_SUCCESS !=
      dsqmhllif_query_bearer_settings( (uint32)iface_ptr->client_data_ptr,
                                       &current_info ) )                           
  {
    DSQMH_MSG_MED("DSQMH SM failed to get bearer info: %d",
                  msg_ptr->iface_inst,0,0);
    /* Treat as warning, just clear the cached value and proceed. */
    memset((void*)&cblk_ptr->um_bearer_tech, 0x0, sizeof(cblk_ptr->um_bearer_tech)); 
  }
  else
  {
    /* Cache the Modem Um IFACE bearer technology. */
    if( DSQMH_SUCCESS !=
        dsqmhllif_decode_bearer_tech( &current_info.bearer_info.current_db_nw,
                                      &current_info.bearer_info.rat_mask,
                                      &current_info.bearer_info.db_so_mask,
                                      &cblk_ptr->um_bearer_tech ) )
    {
      DSQMH_MSG_ERROR("DSQMH SM failed to decode bearer info: %d",
                      msg_ptr->iface_inst,0,0);
      return;
    }
  }

  /*-----------------------------------------------------------------------
    Command QMI to query Modem for network interface settings.  This
    must be done before IPv6 configuration to set address family on
    Proxy Iface.
  -----------------------------------------------------------------------*/
  if( DSQMH_SUCCESS != dsqmhllif_configure_iface_cmd( msg_ptr->iface_inst ) )
  {
    /* Treat as warning, proceed. */
    DSQMH_MSG_LOW("DSQMH SM failed to configure iface: %d",
                  msg_ptr->iface_inst,0,0);
  }

  if( DSQMH_SUCCESS !=
      ds_qmh_netplat_configure_iface( cblk_ptr->netplat_info.conn_id ) )
  {
    DSQMH_MSG_ERROR("DSQMH SM failed to plaform configure iface: %d",
                    msg_ptr->iface_inst,0,0);
  }

#if defined(FEATURE_DATA_PS_IPV6) && defined(FEATURE_DATA_PS_ADDR_MGMT)
  if( ps_iface_addr_family_is_v6( iface_ptr ) )
  {
    DSQMH_MSG_MED("DSQMH SM starting IPV6 address procedure", 0, 0, 0);
    
    if( DSQMH_SUCCESS !=
        dsqmhhdlr_ipv6_configure_hdlr( msg_ptr->iface_inst, FALSE ) )
    {
      DSQMH_MSG_ERROR("DSQMH SM failed to configure: %d",
                      msg_ptr->iface_inst,0,0);
      return;
    }
  }
  else
#endif /* FEATURE_DATA_PS_IPV6 && FEATURE_DATA_PS_ADDR_MGMT */
  {
    /*-----------------------------------------------------------------------
      Send asynchronous message for cmd processing in host task context.
    -----------------------------------------------------------------------*/
    DSQMH_GET_MSG_BUF( msg_ptr );
    if( NULL != msg_ptr )
    {
      memset((void*)msg_ptr, 0x0, sizeof(dsqmh_msg_buf_type));

      /*---------------------------------------------------------------------
        Populate message buffer with context info.
        Note the Um IFACE IP address is retrived & saved in earlier step.
      ---------------------------------------------------------------------*/
      msg_ptr->msg_id = PROXY_IFACE_CONFIGURED_IND;
      msg_ptr->iface_inst = (uint32)iface_ptr->client_data_ptr;

      DSQMH_MSG_MED( "Posting DS cmd: PROXY_IFACE_CONFIGURED_IND",0,0,0); 
      DSQMH_PUT_MSG_BUF( msg_ptr );
    }
    else
    {
      DSQMH_MSG_FATAL( "Failed to allocate msg buffer, dropping msg",0,0,0 );
    }
  }

} /* dsqmhsm_state_configuring_entry() */


/*===========================================================================

  STATE EXIT FUNCTION:  dsqmhsm_state_configuring_exit

===========================================================================*/
/*!
    @brief
    Exit function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_CONFIGURING

    @detail
    Called upon exit of this state of the state machine, with optional
    user-passed payload pointer parameter.  The impending state of the state
    machine is also passed as the next_state parameter.

    @return
    None

*/
/*=========================================================================*/
void dsqmhsm_state_configuring_exit
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         next_state,  /*!< Next state */
  void                *payload     /*!< Payload pointer */
)
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_UNUSED( payload );
  STM_UNUSED( next_state );

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  DSQMH_MSG_LOW( "DSQMH SM exiting state CONFIGURING",0,0,0 );

} /* dsqmhsm_state_configuring_exit() */


/*===========================================================================

  TRANSITION FUNCTION:  dsqmhsm_configured_ind_hdlr

===========================================================================*/
/*!
    @brief
    Transition function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_CONFIGURING,
    upon receiving input PROXY_IFACE_CONFIGURED_IND

    @detail
    Called upon receipt of input PROXY_IFACE_CONFIGURED_IND, with optional
    user-passed payload pointer.

    @return
    Returns the next state that the state machine should transition to
    upon receipt of the input.  This state must be a valid state for this
    state machine.

*/
/*=========================================================================*/
stm_state_t dsqmhsm_configured_ind_hdlr
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{
  stm_state_t next_state = STM_SAME_STATE; /* Default 'next' state */

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_UNUSED( payload );

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  DSQMH_MSG_MED( "DSQMH SM configured ind hdlr",0,0,0 );

  /*-----------------------------------------------------------------------
    Transition Proxy IFACE to state UP.
  -----------------------------------------------------------------------*/
  next_state = DSPROXY_IFACE_STATE_UP;

  return( next_state );

} /* dsqmhsm_configured_ind_hdlr() */


/*===========================================================================

     (State Machine: DSQMH_SM)
     STATE ENTRY/EXIT/TRANSITION FUNCTIONS: DSPROXY_IFACE_STATE_RECONFIGURING

===========================================================================*/

/*===========================================================================

  STATE ENTRY FUNCTION:  dsqmhsm_state_reconfiguring_entry

===========================================================================*/
/*!
    @brief
    Entry function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_RECONFIGURING

    @detail
    Called upon entry into this state of the state machine, with optional
    user-passed payload pointer parameter.  The prior state of the state
    machine is also passed as the prev_state parameter.

    @return
    None

*/
/*=========================================================================*/
void dsqmhsm_state_reconfiguring_entry
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         prev_state,  /*!< Previous state */
  void                *payload     /*!< Payload pointer */
)
{
  dsqmh_msg_buf_type         *msg_ptr = NULL; 
  dsqmh_iface_cblk_type      *cblk_ptr = NULL;
  ps_iface_type              *iface_ptr = NULL;           
  dsqmh_runtime_info_type     current_info;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_UNUSED( prev_state );

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );
  STM_NULL_CHECK( payload );

  DSQMH_MSG_MED( "DSQMH SM entering state RECONFIGURING",0,0,0 );

  msg_ptr = (dsqmh_msg_buf_type*)payload;
  iface_ptr = DSQMH_GET_IFACE_PTR( msg_ptr->iface_inst );
  cblk_ptr = DSQMH_GET_CBLK_PTR( msg_ptr->iface_inst );
  
  /*-----------------------------------------------------------------------
    Validate inputs.
  -----------------------------------------------------------------------*/
  if( !PS_IFACE_IS_VALID( iface_ptr ) ) 
  {
    DSQMH_MSG_ERROR( "DSQMH SM iface_ptr invalid", 0, 0, 0 );
    return;
  }
  
  /*-----------------------------------------------------------------------
    Notify IFACE clients of configuration event.
  -----------------------------------------------------------------------*/
  ps_iface_configuring_ind( iface_ptr );

  /*-----------------------------------------------------------------------
    Command QMI to query Modem for Um IFACE bearer settings.
  -----------------------------------------------------------------------*/
  if( DSQMH_SUCCESS !=
      dsqmhllif_query_bearer_settings( (uint32)iface_ptr->client_data_ptr,
                                       &current_info ) )                           
  {
    DSQMH_MSG_ERROR("DSQMH SM failed to get bearer info: %d",
                    msg_ptr->iface_inst,0,0);
    return;
  }
  else
  {
    /* Cache the Modem Um IFACE bearer technology. */
    if( DSQMH_SUCCESS !=
        dsqmhllif_decode_bearer_tech( &current_info.bearer_info.current_db_nw,
                                      &current_info.bearer_info.rat_mask,
                                      &current_info.bearer_info.db_so_mask,
                                      &cblk_ptr->um_bearer_tech ) )
    {
      DSQMH_MSG_MED("DSQMH SM failed to get bearer info: %d",
                    msg_ptr->iface_inst,0,0);
      /* Treat as warning, just clear the cached value and proceed. */
      memset((void*)&cblk_ptr->um_bearer_tech, 0x0, sizeof(cblk_ptr->um_bearer_tech)); 
    }
  }
  
  /*-----------------------------------------------------------------------
    Command QMI to query Modem for network interface settings.
  -----------------------------------------------------------------------*/
  if( DSQMH_SUCCESS != dsqmhllif_configure_iface_cmd( msg_ptr->iface_inst ) )
  {
    /* Treat as warning, proceed. */
    DSQMH_MSG_LOW("DSQMH SM failed to configure iface: %d",
                  msg_ptr->iface_inst,0,0);
  }

#if defined(FEATURE_DATA_PS_IPV6) && defined(FEATURE_DATA_PS_ADDR_MGMT)
  if( ps_iface_addr_family_is_v6( iface_ptr ) )
  {
    DSQMH_MSG_MED("DSQMH LLIF restarting IPV6 address procedure", 0, 0, 0);
    
    if( DSQMH_SUCCESS !=
        dsqmhhdlr_ipv6_configure_hdlr( msg_ptr->iface_inst, TRUE ) )
    {
      DSQMH_MSG_ERROR("DSQMH SM failed to configure: %d",
                      msg_ptr->iface_inst,0,0);
      return;
    }
  }
  else
#endif /* FEATURE_DATA_PS_IPV6 && FEATURE_DATA_PS_ADDR_MGMT */
  {
    /*-----------------------------------------------------------------------
      Send asynchronous message for cmd processing in host task context.
    -----------------------------------------------------------------------*/
    DSQMH_GET_MSG_BUF( msg_ptr );
    if( NULL != msg_ptr )
    {
      memset((void*)msg_ptr, 0x0, sizeof(dsqmh_msg_buf_type));

      /*---------------------------------------------------------------------
        Populate message buffer with context info.
        Note the Um IFACE IP address is retrived & saved in earlier step.
      ---------------------------------------------------------------------*/
      msg_ptr->msg_id = PROXY_IFACE_CONFIGURED_IND;
      msg_ptr->iface_inst = (uint32)iface_ptr->client_data_ptr;

      DSQMH_MSG_MED( "Posting DS cmd: PROXY_IFACE_CONFIGURED_IND",0,0,0); 
      DSQMH_PUT_MSG_BUF( msg_ptr );
    }
    else
    {
      DSQMH_MSG_FATAL( "Failed to allocate msg buffer, dropping msg",0,0,0 );
    }
  }
} /* dsqmhsm_state_reconfiguring_entry() */


/*===========================================================================

  STATE EXIT FUNCTION:  dsqmhsm_state_reconfiguring_exit

===========================================================================*/
/*!
    @brief
    Exit function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_RECONFIGURING

    @detail
    Called upon exit of this state of the state machine, with optional
    user-passed payload pointer parameter.  The impending state of the state
    machine is also passed as the next_state parameter.

    @return
    None

*/
/*=========================================================================*/
void dsqmhsm_state_reconfiguring_exit
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         next_state,  /*!< Next state */
  void                *payload     /*!< Payload pointer */
)
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_UNUSED( payload );
  STM_UNUSED( next_state );

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  DSQMH_MSG_LOW( "DSQMH SM exiting state RECONFIGURING",0,0,0 );

} /* dsqmhsm_state_reconfiguring_exit() */

/*===========================================================================

     (State Machine: DSQMH_SM)
     STATE ENTRY/EXIT/TRANSITION FUNCTIONS: DSPROXY_IFACE_STATE_UP

===========================================================================*/

/*===========================================================================

  STATE ENTRY FUNCTION:  dsqmhsm_state_up_entry

===========================================================================*/
/*!
    @brief
    Entry function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_UP

    @detail
    Called upon entry into this state of the state machine, with optional
    user-passed payload pointer parameter.  The prior state of the state
    machine is also passed as the prev_state parameter.

    @return
    None

*/
/*=========================================================================*/
void dsqmhsm_state_up_entry
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         prev_state,  /*!< Previous state */
  void                *payload     /*!< Payload pointer */
)
{
  dsqmh_msg_buf_type *msg_ptr = NULL;
  ps_iface_type  * iface_ptr = NULL;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_UNUSED( prev_state );

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );
  STM_NULL_CHECK( payload );

  DSQMH_MSG_MED( "DSQMH SM entering state UP",0,0,0 );

  msg_ptr = (dsqmh_msg_buf_type*)payload;
  iface_ptr = DSQMH_GET_IFACE_PTR( msg_ptr->iface_inst );

  /*-----------------------------------------------------------------------
    Validate inputs.
  -----------------------------------------------------------------------*/
  if( !PS_IFACE_IS_VALID( iface_ptr ) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM iface_ptr invalid", 0, 0, 0 );
    return;
  }

  /*-----------------------------------------------------------------------
    Post notifications of PS IFACE in UP state.
  -----------------------------------------------------------------------*/
  DSQMH_MSG_HIGH( "DSQMH SM iface up: %d", msg_ptr->iface_inst,0,0 );
  ps_iface_up_ind( iface_ptr );

} /* dsqmhsm_state_up_entry() */


/*===========================================================================

  STATE EXIT FUNCTION:  dsqmhsm_state_up_exit

===========================================================================*/
/*!
    @brief
    Exit function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_UP

    @detail
    Called upon exit of this state of the state machine, with optional
    user-passed payload pointer parameter.  The impending state of the state
    machine is also passed as the next_state parameter.

    @return
    None

*/
/*=========================================================================*/
void dsqmhsm_state_up_exit
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         next_state,  /*!< Next state */
  void                *payload     /*!< Payload pointer */
)
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_UNUSED( payload );
  STM_UNUSED( next_state );

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  DSQMH_MSG_LOW( "DSQMH SM exiting state UP",0,0,0 );

} /* dsqmhsm_state_up_exit() */



/*===========================================================================

  TRANSITION FUNCTION:  dsqmhsm_modem_event_ind_hdlr

===========================================================================*/
/*!
    @brief
    Transition function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_UP,
    upon receiving input PROXY_IFACE_MODEM_EVENT_IND

    @detail
    Called upon receipt of input PROXY_IFACE_MODEM_EVENT_IND, with optional
    user-passed payload pointer.

    @return
    Returns the next state that the state machine should transition to
    upon receipt of the input.  This state must be a valid state for this
    state machine.

*/
/*=========================================================================*/
stm_state_t dsqmhsm_modem_event_ind_hdlr
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{
  stm_state_t next_state = STM_SAME_STATE; /* Default 'next' state */
  dsqmh_msg_buf_type        *msg_ptr = NULL;
  ps_iface_type             *iface_ptr = NULL;
  dsqmh_iface_cblk_type     *cblk_ptr = NULL;
  dsqmh_qmi_wds_ind_type    *info_ptr = NULL;
  ps_iface_mcast_event_info_type mcast_info;
  ps_iface_ioctl_bearer_tech_changed_type bchng_info;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_NULL_CHECK( sm );

  msg_ptr = (dsqmh_msg_buf_type*)payload;
  iface_ptr = DSQMH_GET_IFACE_PTR( msg_ptr->iface_inst );
  cblk_ptr = DSQMH_GET_CBLK_PTR( msg_ptr->iface_inst );
  info_ptr = &msg_ptr->info.qmi_wds;

  DSQMH_MSG_LOW( "DSQMH SM modem event ind hdlr: %d",info_ptr->ind_id,0,0 );

  switch( info_ptr->ind_id )
  {
    case QMI_WDS_SRVC_EVENT_REPORT_IND_MSG:
      if( QMI_WDS_EVENT_BEARER_TECH_IND & info_ptr->info.event_report.event_mask )
      {
        qmi_wds_data_bearer_tech_type *qmi_ptr = 
          &info_ptr->info.event_report.data_bearer_tech_type;

        DSQMH_MSG_MED( "DSQMH SM event report bearer change ind",0,0,0 );

        bchng_info.old_bearer_tech = cblk_ptr->um_bearer_tech;
        (void)dsqmhllif_decode_bearer_tech( &qmi_ptr->current_db_nw,
                                            &qmi_ptr->rat_mask,
                                            &qmi_ptr->db_so_mask,
                                            &bchng_info.new_bearer_tech );
        cblk_ptr->um_bearer_tech = bchng_info.new_bearer_tech;

        ps_iface_generic_ind( iface_ptr,
                              IFACE_BEARER_TECH_CHANGED_EV,
                              (void*)&bchng_info );
      }
      
      if( QMI_WDS_EVENT_DORM_STATUS_IND & info_ptr->info.event_report.event_mask )
      {
        qmi_wds_dorm_status_type *qmi_ptr =
          &info_ptr->info.event_report.dorm_status;
        ps_phys_link_type  *phys_link_ptr =
          PS_IFACE_GET_PHYS_LINK( iface_ptr );
        int16               ps_errno;
        
        DSQMH_MSG_MED( "DSQMH SM event report dormant status ind: state=%d",
                       *qmi_ptr,0,0 );
        /*-------------------------------------------------------------------------
          Verify IFACE in UP state for dormancy indications.
        -------------------------------------------------------------------------*/
        if(IFACE_UP == ps_iface_state( iface_ptr ))
        {
          switch( *qmi_ptr )
          {
            case QMI_WDS_DORM_STATUS_DORMANT:
              (void)ps_phys_link_down_cmd( phys_link_ptr, &ps_errno,
                                           (void*)DSQMH_PHYSLINK_CMD_DOWN_DORMANT );
              break;

            case QMI_WDS_DORM_STATUS_ACTIVE:
              ps_phys_link_up_ind_ex( phys_link_ptr, PS_EIC_NOT_SPECIFIED );
              break;

            default:
              DSQMH_MSG_ERROR( "QMH SM unsupported dormant status type:%d",
                               *qmi_ptr,0,0 );
              break;
          }
        }
        else
        {
          DSQMH_MSG_ERROR( "Ignoring dormancy ind: IFACE not in UP state",0,0,0 );
        }
      }

      if( QMI_WDS_EVENT_DATA_CAPABILITIES_IND & info_ptr->info.event_report.event_mask )
      {
        DSQMH_MSG_MED( "DSQMH SM event report data capabilities changed ind: "
                       "iface name 0x%x, caps mask=0x%x:%x",
                       info_ptr->info.event_report.iface_name,
                       info_ptr->info.event_report.data_capabilities[0],
                       info_ptr->info.event_report.data_capabilities[1] );
      
        memset (&bchng_info, 0, sizeof(bchng_info));
        bchng_info.old_bearer_tech = cblk_ptr->um_bearer_tech;

        /* Currently support only CDMA */
        if (CDMA_SN_IFACE == info_ptr->info.event_report.iface_name)
        {
          bchng_info.new_bearer_tech.current_network = PS_IFACE_NETWORK_CDMA;
          bchng_info.new_bearer_tech.data.cdma_type.so_mask = 0;
          bchng_info.new_bearer_tech.data.cdma_type.rat_mask = 0;

          if (QMI_WDS_EVENT_REPORT_DATA_CAPABILITY_0_CDMA_1X &
              info_ptr->info.event_report.data_capabilities[0])
          {
            bchng_info.new_bearer_tech.data.cdma_type.rat_mask |= PS_IFACE_CDMA_1X;
            bchng_info.new_bearer_tech.data.cdma_type.so_mask  |=
              info_ptr->info.event_report.data_bearer_tech_type.db_so_mask.so_mask_1x;
          }

          if (QMI_WDS_EVENT_REPORT_DATA_CAPABILITY_0_EVDO_REV_0 &
              info_ptr->info.event_report.data_capabilities[0])
          {
            bchng_info.new_bearer_tech.data.cdma_type.rat_mask |= PS_IFACE_CDMA_EVDO_REV0;
          }

          if (QMI_WDS_EVENT_REPORT_DATA_CAPABILITY_0_EVDO_REV_A &
              info_ptr->info.event_report.data_capabilities[0])
          {
            bchng_info.new_bearer_tech.data.cdma_type.rat_mask |= PS_IFACE_CDMA_EVDO_REVA;
            bchng_info.new_bearer_tech.data.cdma_type.so_mask  |=
              info_ptr->info.event_report.data_bearer_tech_type.db_so_mask.so_mask_evdo_reva;
          }

          if (QMI_WDS_EVENT_REPORT_DATA_CAPABILITY_0_EVDO_REV_B &
              info_ptr->info.event_report.data_capabilities[0])
          {
            bchng_info.new_bearer_tech.data.cdma_type.rat_mask |= PS_IFACE_CDMA_EVDO_REVB;
          }

          if (QMI_WDS_EVENT_REPORT_DATA_CAPABILITY_0_NULL_BEARER &
              info_ptr->info.event_report.data_capabilities[0])
          {
            bchng_info.new_bearer_tech.data.cdma_type.rat_mask |= PS_IFACE_CDMA_NULL_BEARER;
          }

          cblk_ptr->um_bearer_tech = bchng_info.new_bearer_tech;
        
          ps_iface_generic_ind( iface_ptr,
                                IFACE_BEARER_TECH_CHANGED_EV,
                                (void*)&bchng_info );

        } /* if (CDMA_SN_IFACE == iface_name) */
      }
      break;

    case QMI_WDS_SRVC_MT_REQUEST_IND_MSG:
      DSQMH_MSG_MED( "DSQMH SM MT call request ind: hndl=%d",info_ptr->info.mt_handle,0,0 );
      ps_iface_generic_ind( iface_ptr,
                            IFACE_MT_REQUEST_EV,
                            (void*)&info_ptr->info.mt_handle );

      break;

    case QMI_WDS_SRVC_MCAST_STATUS_IND_MSG:
      mcast_info.handle = info_ptr->info.mcast_status.multicast_status.mcast_handle;
      mcast_info.info_code = (ps_iface_mcast_info_code_enum_type)info_ptr->info.mcast_status.reason_code;
      mcast_info.force_dereg_cbacks = FALSE;

      switch( info_ptr->info.mcast_status.multicast_status.mcast_status )
      {
        case QMI_WDS_MCAST_REGISTER_SUCCESS:
          DSQMH_MSG_MED( "DSQMH SM MCAST status register success ind: %d",0,0,0 );
          ps_iface_generic_ind( iface_ptr,
                                IFACE_MCAST_REGISTER_SUCCESS_EV,
                                (void*)&mcast_info );
          break;

        case QMI_WDS_MCAST_REGISTER_FAILURE:
          DSQMH_MSG_MED( "DSQMH SM MCAST status register failure ind: %d",0,0,0 );
          ps_iface_generic_ind( iface_ptr,
                                IFACE_MCAST_REGISTER_FAILURE_EV,
                                (void*)&mcast_info );
          break;

        case QMI_WDS_MCAST_DEREGISTERED:
          DSQMH_MSG_MED( "DSQMH SM MCAST status deregister ind: %d",0,0,0 );
          ps_iface_generic_ind( iface_ptr,
                                IFACE_MCAST_DEREGISTERED_EV,
                                (void*)&mcast_info );
          break;

        case QMI_WDS_MCAST_STATUS_EX:
          DSQMH_MSG_MED( "DSQMH SM MCAST status EX ind: %d",0,0,0 );
          ps_iface_generic_ind( iface_ptr,
                                IFACE_MCAST_STATUS_EV,
                                (void*)&mcast_info );
          break;

        default:
          DSQMH_MSG_ERROR( "QMH SM unsupported multicase status type:%d",
                           info_ptr->info.mcast_status.multicast_status.mcast_status,0,0 );
          break;
      }
      break;

    default:
      DSQMH_MSG_ERROR( "QMH SM unsupported indication message:%d",
                       info_ptr->ind_id,0,0 );
      break;
  }

  return( next_state );

} /* dsqmhsm_modem_event_ind_hdlr() */

/*===========================================================================

  TRANSITION FUNCTION:  dsqmhsm_modem_internal_ind_hdlr

===========================================================================*/
/*!
    @brief
    Transition function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_UP,
    upon receiving input PROXY_IFACE_MODEM_INTERNAL_IND

    @detail
    Called upon receipt of input PROXY_IFACE_MODEM_INTERNAL_IND, with optional
    user-passed payload pointer.

    @return
    Returns the next state that the state machine should transition to
    upon receipt of the input.  This state must be a valid state for this
    state machine.

*/
/*=========================================================================*/
stm_state_t dsqmhsm_modem_internal_ind_hdlr
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{
  stm_state_t next_state = STM_SAME_STATE; /* Default 'next' state */
  dsqmh_msg_buf_type        *msg_ptr = NULL;
  ps_iface_type             *iface_ptr = NULL;
  ps_phys_link_type         *phys_link_ptr = NULL;
  dsqmh_iface_cblk_type     *cblk_ptr = NULL;   /* Pointer to control block  */
  qmi_wds_internal_iface_event_ind_data_type *info_ptr = NULL;
  ps_iface_ioctl_bearer_tech_changed_type  btech_info;
  ps_iface_ioctl_extended_ip_config_type   ipconfig_info;
  ps_iface_rf_conditions_info_type         rfcond_info;
  ps_iface_outage_notification_event_info_type outage_info;
  ps_phys_link_dos_ack_status_info_type        dosack_info;
  int32                                        dosack_handle;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_NULL_CHECK( sm );

  msg_ptr = (dsqmh_msg_buf_type*)payload;
  cblk_ptr = DSQMH_GET_CBLK_PTR( msg_ptr->iface_inst );
  iface_ptr = DSQMH_GET_IFACE_PTR( msg_ptr->iface_inst );
  phys_link_ptr = PS_IFACE_GET_PHYS_LINK( iface_ptr );
  info_ptr = &msg_ptr->info.qmi_wds.info.iface_event_report;

  DSQMH_MSG_LOW( "DSQMH SM modem internal ind hdlr",0,0,0 );

  switch( info_ptr->iface_event_name )
  {
    case EXTENDED_IP_CONFIG:
      DSQMH_MSG_MED( "DSQMH SM Extended IP config ind",0,0,0 );
      if( QMI_WDS_XTENDED_IP_CONFIG_PARAM_TYPE & info_ptr->param_mask )
      {

        if( SUCCESS == info_ptr->extended_ip_config_status )
        {
          /* Um IFACE configuration has changed (e.g. DHCP) so need to
           * update proxy IFACE cached parameters.
           */
          if( DSQMH_SUCCESS != dsqmhllif_configure_iface_cmd( msg_ptr->iface_inst ) )
          {
            /* Treat as warning, proceed. */
            DSQMH_MSG_LOW("DSQMH SM failed to configure iface: %d",
                          msg_ptr->iface_inst,0,0);
          }
        }

        /* Notify upper layer event listeners */
        ipconfig_info =
          (ps_iface_ioctl_extended_ip_config_type)info_ptr->extended_ip_config_status;
        ps_iface_generic_ind( iface_ptr,
                              IFACE_EXTENDED_IP_CONFIG_EV,
                              (void*)&ipconfig_info );

      }
      break;

    case RF_CONDITIONS_CHANGED:
      DSQMH_MSG_MED( "DSQMH SM RF Conditions ind",0,0,0 );
      if( QMI_WDS_RF_CONDITIONS_PARAM_TYPE & info_ptr->param_mask )
      {
        /* Post RF conditions indication */
        memset((void*)&rfcond_info, 0x0, sizeof(rfcond_info));
        rfcond_info.rf_conditions = (ps_iface_rf_conditions_enum_type)info_ptr->rf_conditions.rf_conditions;
        if( DSQMH_SUCCESS !=
            dsqmhllif_decode_bearer_tech( &info_ptr->rf_conditions.current_db_nw,
                                          &info_ptr->rf_conditions.rat_mask,
                                          &info_ptr->rf_conditions.db_so_mask,
                                          &cblk_ptr->um_bearer_tech ) )
        {
          DSQMH_MSG_MED("DSQMH SM failed to decode bearer info: %d",
                        msg_ptr->iface_inst,0,0);
        }

        ps_iface_generic_ind( iface_ptr,
                              IFACE_RF_CONDITIONS_CHANGED_EV,
                              (void*)&rfcond_info );

        /* Check for bearer change indication */
        if( !memcmp( (void*)&cblk_ptr->um_bearer_tech,
                     (void*)&info_ptr->rf_conditions,
                     sizeof(cblk_ptr->um_bearer_tech) ) )
        {
          memset((void*)&btech_info, 0x0, sizeof(btech_info));
          btech_info.old_bearer_tech = cblk_ptr->um_bearer_tech;
          btech_info.new_bearer_tech = rfcond_info.bearer_tech;
          cblk_ptr->um_bearer_tech   = rfcond_info.bearer_tech;
          ps_iface_generic_ind( iface_ptr,
                                IFACE_BEARER_TECH_CHANGED_EV,
                                (void*)&btech_info );
        }
      }
      break;

    case OUTAGE_NOTIFICATION:
      DSQMH_MSG_MED( "DSQMH SM Outage ind",0,0,0 );
      if( QMI_WDS_OUTAGE_INFO_PARAM_TYPE & info_ptr->param_mask )
      {
        memcpy((void*)&outage_info, (void*)&info_ptr->outage_information, sizeof(outage_info));
        ps_iface_generic_ind( iface_ptr,
                              IFACE_OUTAGE_NOTIFICATION_EV,
                              (void*)&outage_info );
      }
      break;

    case HDR_REV0_RATE_INERTIA_FAILURE:
      DSQMH_MSG_MED( "DSQMH SM HDR rev0 intertia failure ind",0,0,0 );
      if( QMI_WDS_HDR_REV0_INTERTIA_FAILURE_PARAM_TYPE & info_ptr->param_mask )
      {
        ps_iface_generic_ind( iface_ptr,
                              IFACE_ENABLE_HDR_REV0_RATE_INERTIA_FAILURE_EV,
                              (void*)&info_ptr->hdr_rev0_rate_inertia_failure_status );
      }
      break;

#ifdef FEATURE_EIDLE_SCI
    case HDR_REV0_SET_EIDLE_SLOTTED_MODE_FAILURE:
      DSQMH_MSG_MED( "DSQMH SM HDR set eidle slotted mode failure ind",0,0,0 );
      if( QMI_WDS_HDR_SET_EIDLE_SLOT_MODE_FAIL_PARAM_TYPE & info_ptr->param_mask )
      {
        ps_iface_generic_ind( iface_ptr,
                              IFACE_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_FAILURE_EV,
                              (void*)&info_ptr->hdr_set_eidle_slottedmode_failure_status );
      }
      break;

    case HDR_REV0_SLOTTED_MODE_SESSION_CHANGED:
      DSQMH_MSG_MED( "DSQMH SM HDR set eidle slot mode session changed ind",0,0,0 );
      if( QMI_WDS_HDR_SET_EIDLE_SLOT_MODE_SESSION_CHANGED_PARAM_TYPE & info_ptr->param_mask )
      {
        ps_iface_generic_ind( iface_ptr,
                              IFACE_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_SESSION_CHANGED_EV,
                              (void*)&info_ptr->hdr_set_eidle_slot_cycle_changed );
      }
      break;
#endif /* FEATURE_EIDLE_SCI */

    case DOS_ACK_INFORMATION:
      DSQMH_MSG_MED( "DSQMH SM 707 dos ack ind",0,0,0 );
      if( QMI_WDS_DOS_ACK_INFO_PARAM_TYPE & info_ptr->param_mask )
      {
        dosack_info.overflow = info_ptr->dos_ack_information.overflow;
        dosack_info.status = (ps_phys_link_dos_ack_status_enum_type) 
                             info_ptr->dos_ack_information.dos_ack_status;

        dosack_handle = dsqmhhdlr_get_dos_ack_handle(
                          info_ptr->dos_ack_information.handle);

        ps_phys_link_dos_ack_ind( phys_link_ptr,
                                  dosack_handle,
                                  &dosack_info );
      }
      break;

    default:
      DSQMH_MSG_ERROR( "QMH SM unsupported indication message:%d",
                       info_ptr->iface_event_name,0,0 );
      break;
  }

  return( next_state );
} /* dsqmhsm_modem_internal_ind_hdlr() */


/*===========================================================================

  TRANSITION FUNCTION:  dsqmhsm_modem_mcast_ind_hdlr

===========================================================================*/
/*!
    @brief
    Transition function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_UP,
    upon receiving input PROXY_IFACE_MODEM_MCAST_IND

    @detail
    Called upon receipt of input PROXY_IFACE_MODEM_MCAST_IND, with optional
    user-passed payload pointer.

    @return
    Returns the next state that the state machine should transition to
    upon receipt of the input.  This state must be a valid state for this
    state machine.

*/
/*=========================================================================*/
stm_state_t dsqmhsm_modem_mcast_ind_hdlr
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{
  stm_state_t next_state = STM_SAME_STATE; /* Default 'next' state */

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_UNUSED( payload );
  STM_NULL_CHECK( sm );

  DSQMH_MSG_LOW( "DSQMH SM modem mcast ind hdlr",0,0,0 );

  return( next_state );

} /* dsqmhsm_modem_mcast_ind_hdlr() */


/*===========================================================================

  TRANSITION FUNCTION:  dsqmhsm_modem_bcmcs_ind_hdlr

===========================================================================*/
/*!
    @brief
    Transition function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_UP,
    upon receiving input PROXY_IFACE_MODEM_BCMCS_IND

    @detail
    Called upon receipt of input PROXY_IFACE_MODEM_BCMCS_IND, with optional
    user-passed payload pointer.

    @return
    Returns the next state that the state machine should transition to
    upon receipt of the input.  This state must be a valid state for this
    state machine.

*/
/*=========================================================================*/
stm_state_t dsqmhsm_modem_bcmcs_ind_hdlr
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{
  stm_state_t next_state = STM_SAME_STATE; /* Default 'next' state */

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_UNUSED( payload );
  STM_NULL_CHECK( sm );

  DSQMH_MSG_LOW( "DSQMH SM modem bcmcs ind hdlr",0,0,0 );

  return( next_state );

} /* dsqmhsm_modem_bcmcs_ind_hdlr() */


/*===========================================================================

  TRANSITION FUNCTION:  dsqmhsm_modem_mtreq_ind_hdlr

===========================================================================*/
/*!
    @brief
    Transition function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_UP,
    upon receiving input PROXY_IFACE_MODEM_MTREQ_IND

    @detail
    Called upon receipt of input PROXY_IFACE_MODEM_MTREQ_IND, with optional
    user-passed payload pointer.

    @return
    Returns the next state that the state machine should transition to
    upon receipt of the input.  This state must be a valid state for this
    state machine.

*/
/*=========================================================================*/
stm_state_t dsqmhsm_modem_mtreq_ind_hdlr
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{
  stm_state_t next_state = STM_SAME_STATE; /* Default 'next' state */

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_UNUSED( payload );
  STM_NULL_CHECK( sm );

  DSQMH_MSG_LOW( "DSQMH SM modem MT request ind hdlr",0,0,0 );

  return( next_state );

} /* dsqmhsm_modem_mtreq_ind_hdlr() */

/*===========================================================================

  HELPER FUNCTION: dsqmhsm_modem_qos_network_status_report_ind_hdlr

===========================================================================*/
/*!
    @brief
    This funciton handles the QOS network status report indication message.

    @detail
    Called upon receipt of input PROXY_IFACE_MODEM_QOS_IND.

    @return
    DSQMH_SUCCESS on successful processing.
    DSQMH_FAILED on error.

*/
/*=========================================================================*/
static void dsqmhsm_modem_qos_network_status_report_ind_hdlr
(
  uint32                     iface_inst,        /* Index for iface table   */
  qmi_qos_nw_status_type    *info_ptr 
)
{
  ps_iface_type             *iface_ptr = NULL;           
  dsqmh_iface_cblk_type     *cblk_ptr = NULL;
  boolean                    invoke_ind = TRUE;
  ps_extended_info_code_enum_type  iface_event_info_code = PS_EIC_NOT_SPECIFIED;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_MED( "DSQMH SM QOS network status report indication:"
                 "iface inst 0x%x, iface name 0x%x, qos supported 0x%x",
                 iface_inst,
                 info_ptr->iface_name,
                 info_ptr->qos_supported );

  cblk_ptr = DSQMH_GET_CBLK_PTR( iface_inst );
  iface_ptr = DSQMH_GET_IFACE_PTR( iface_inst );
  if( NULL == iface_ptr )
  {
    DSQMH_MSG_ERROR("DSQMH SM QOS invalid iface instance %d", iface_inst, 0, 0);
    return;
  }

  /*-----------------------------------------------------------------------
    If indication is for different iface name, then ignore indication 
    completely. Currently CDMA is the only iface name supported. 
  -----------------------------------------------------------------------*/
  if (QMI_QOS_CDMA_SN_IFACE == info_ptr->iface_name &&
      CDMA_SN_IFACE != iface_ptr->name &&
      CDMA_AN_IFACE != iface_ptr->name)
  {
    return;
  } 

  
  /*-----------------------------------------------------------------------
    If QoS support is already registered to be true, and we are getting 
    another QoS aware indication, then do not post indication.
  -----------------------------------------------------------------------*/
  if (DSQMH_QOS_SUPPORT_AVAILABLE == cblk_ptr->qos_supported &&
      NW_SUPPORTS_QOS == info_ptr->qos_supported)
  {
    invoke_ind = FALSE;
  }

  /*-----------------------------------------------------------------------
    If QoS support is already registered to be false, and we are getting 
    another QoS unaware indication, then do not post indicatoin.
  -----------------------------------------------------------------------*/
  if (DSQMH_QOS_SUPPORT_UNAVAILABLE == cblk_ptr->qos_supported &&
      NW_DOESNT_SUPPORT_QOS == info_ptr->qos_supported)
  {
    invoke_ind = FALSE;
  }

  /*-----------------------------------------------------------------------
    Store the qos_supported information and post indication if required.
  -----------------------------------------------------------------------*/
  if (NW_SUPPORTS_QOS == info_ptr->qos_supported)
  {
    cblk_ptr->qos_supported = DSQMH_QOS_SUPPORT_AVAILABLE;

    if (TRUE == invoke_ind)
    {
      ps_iface_generic_ind (iface_ptr,
                            IFACE_QOS_AWARE_SYSTEM_EV,
                            &iface_event_info_code);
    }

  }
  else if (NW_DOESNT_SUPPORT_QOS == info_ptr->qos_supported)
  {
    cblk_ptr->qos_supported = DSQMH_QOS_SUPPORT_UNAVAILABLE;

    if (TRUE == invoke_ind)
    {
      ps_iface_generic_ind (iface_ptr,
                            IFACE_QOS_UNAWARE_SYSTEM_EV,
                            &iface_event_info_code);
    }

  }
  else
  {
    DSQMH_MSG_MED("Unknown parameter for qos_supported, 0x%x", 
                   info_ptr->qos_supported, 0, 0);
    return;
  }

  return;

} /* dsqmhsm_modem_qos_network_status_report_ind_hdlr() */


/*===========================================================================

  HELPER FUNCTION: dsqmhsm_modem_qos_network_qos_profiles_change_ind_hdlr

===========================================================================*/
/*!
    @brief
    This funciton handles the QOS network supported profiles change
        indication message.

    @detail
    Called upon receipt of input PROXY_IFACE_MODEM_QOS_IND.

    @return
    DSQMH_SUCCESS on successful processing.
    DSQMH_FAILED on error.

*/
/*=========================================================================*/
#ifndef FEATURE_DSS_LINUX
static void dsqmhsm_modem_qos_network_qos_profiles_change_ind_hdlr
(
  uint32                     iface_inst,        /* Index for iface table   */
  qmi_qos_event_report_state_type    *info_ptr 
)
{
  ps_iface_type             *iface_ptr = NULL;           
  dsqmh_iface_cblk_type     *cblk_ptr = NULL;
  boolean                    invoke_ind = TRUE;
  ps_extended_info_code_enum_type  iface_event_info_code = PS_EIC_NOT_SPECIFIED;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_MED( "DSQMH SM QOS network supported profiles change indication:"
                 "iface inst 0x%x, iface name 0x%x",
                 iface_inst,
                 info_ptr->profile_change_state.iface_type, 0);
                 
  cblk_ptr = DSQMH_GET_CBLK_PTR( iface_inst );
  iface_ptr = DSQMH_GET_IFACE_PTR( iface_inst );

  /*-----------------------------------------------------------------------
    If indication is for different iface name, then ignore indication 
    completely. Currently CDMA is the only iface name supported. 
  -----------------------------------------------------------------------*/
  if (QMI_QOS_CDMA_SN_IFACE == 
       info_ptr->profile_change_state.iface_type &&
      CDMA_SN_IFACE != iface_ptr->name &&
      CDMA_AN_IFACE != iface_ptr->name)
  {
    return;
  } 
  
  /*-----------------------------------------------------------------------
    post indication
  -----------------------------------------------------------------------*/
  
   ps_iface_generic_ind (iface_ptr,
                           IFACE_707_NETWORK_SUPPORTED_QOS_PROFILES_CHANGED_EV,
                           &iface_event_info_code);
   return;

} /* dsqmhsm_modem_qos_network_qos_profiles_change_ind_hdlr() */
#endif


/*===========================================================================
  FUNCTION dsqmhsm_info_code_lookup()

  DESCRIPTION
    Maps the qmi info code values to ps extended info codes.
    
  PARAMETERS
    qmi_reason_code : corresponding qmi reason code    
    
  RETURN VALUE
    info_code : qos status change reason code
    
  DEPENDENCIES
    None

  SIDE EFFECTS
    None
===========================================================================*/
static ps_extended_info_code_enum_type  dsqmhsm_info_code_lookup
(
  qmi_qos_reason_code  qmi_reason_code
)
{
  ps_extended_info_code_enum_type  info_code;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  switch(qmi_reason_code)
  {    
    case QMI_QOS_INVALID_PARAMS:    
      info_code = PS_EIC_QOS_INVALID_PARAMS;
      break;

    case QMI_QOS_INTERNAL_CALL_ENDED:
      info_code = PS_EIC_QOS_INTERNAL_CALL_ENDED;
      break;

    case QMI_QOS_INTERNAL_ERROR:
      info_code = PS_EIC_QOS_INTERNAL_ERROR;
      break;

    case QMI_QOS_INSUFFICIENT_LOCAL_RESOURCES:
      info_code = PS_EIC_QOS_INSUFFICIENT_LOCAL_RESOURCES;
      break;

    case QMI_QOS_TIMED_OUT_OPERATION:
      info_code = PS_EIC_QOS_TIMED_OUT_OPERATION;
      break;

    case QMI_QOS_INTERNAL_UNKNOWN_CAUSE_CODE:
      info_code = PS_EIC_QOS_INTERNAL_UNKNOWN_CAUSE_CODE;
      break;

    case QMI_QOS_INTERNAL_MODIFY_IN_PROGRESS:
      info_code = PS_EIC_QOS_INTERNAL_MODIFY_IN_PROGRESS;
      break;

    case QMI_QOS_NOT_SUPPORTED:
      info_code = PS_EIC_QOS_NOT_SUPPORTED;
      break;

    case QMI_QOS_NOT_AVAILABLE:
      info_code = PS_EIC_QOS_NOT_AVAILABLE;
      break;

    case QMI_QOS_NOT_GUARANTEED:
      info_code = PS_EIC_QOS_NOT_GUARANTEED;
      break;

    case QMI_QOS_INSUFFICIENT_NETWORK_RESOURCES:
      info_code = PS_EIC_QOS_INSUFFICIENT_NET_RESOURCES;
      break;

    case QMI_QOS_AWARE_SYSTEM:
      info_code = PS_EIC_QOS_AWARE_SYSTEM;
      break;

    case QMI_QOS_UNAWARE_SYSTEM:
      info_code = PS_EIC_QOS_UNAWARE_SYSTEM;
      break;

    case QMI_QOS_REJECTED_OPERATION:
      info_code = PS_EIC_QOS_REJECTED_OPERATION;
      break;

    case QMI_QOS_WILL_GRANT_WHEN_QOS_RESUMED:
      info_code = PS_EIC_QOS_WILL_GRANT_WHEN_QOS_RESUMED;
      break;

    case QMI_QOS_NETWORK_CALL_ENDED:
      info_code = PS_EIC_QOS_NETWORK_CALL_ENDED;
      break;

    case QMI_QOS_NETWORK_SERVICE_NOT_AVAILABLE:
      info_code = PS_EIC_QOS_NETWORK_SVC_NOT_AVAILABLE;
      break;

    case QMI_QOS_NETWORK_L2_LINK_RELEASED:
      info_code = PS_EIC_QOS_NETWORK_L2_LINK_RELEASED;
      break;

    case QMI_QOS_NETWORK_L2_LINK_REESTAB_REJ:
      info_code = PS_EIC_QOS_NETWORK_L2_LINK_REESTAB_REJ;
      break;

    case QMI_QOS_NETWORK_L2_LINK_REESTAB_IND:
      info_code = PS_EIC_QOS_NETWORK_L2_LINK_REESTAB_IND;
      break;

    case QMI_QOS_NETWORK_UNKNOWN_CAUSE_CODE:
      info_code = PS_EIC_QOS_NETWORK_UNKNOWN_CAUSE_CODE;
      break;

    case QMI_QOS_NETWORK_DISJOINT_PROFILE_SET_SUGGESTED:
      info_code = PS_EIC_QOS_NETWORK_DISJOINT_PROFILE_SET_SUGGESTED;

    case QMI_QOS_NETWORK_NULL_PROFILE_SUGGESTED:
      info_code = PS_EIC_QOS_NETWORK_NULL_PROFILE_SUGGESTED;
      break;

    case QMI_QOS_NETWORK_UE_NOT_AUTHORIZED:
      info_code = PS_EIC_QOS_NETWORK_UE_NOT_AUTHORIZED;

    default:
      DSQMH_MSG_ERROR( "DSQMH SM Unrecognized qmi reason code: %d",
                       qmi_reason_code, 0, 0 );    
      info_code = PS_EIC_NOT_SPECIFIED;
      break;
  }

  return info_code;
} /* dsqmhsm_info_code_lookup() */

/*===========================================================================

  HELPER FUNCTION: dsqmhsm_modem_qos_status_ind_hdlr

===========================================================================*/
/*!
    @brief
    This funciton handles the QOS status indication message.

    @detail
    Called upon receipt of input PROXY_IFACE_MODEM_QOS_IND.

    @return
    DSQMH_SUCCESS on successful processing.
    DSQMH_FAILED on error.

*/
/*=========================================================================*/
int dsqmhsm_modem_qos_status_ind_hdlr
(
  uint32                       iface_inst,      /* Index for iface table   */
  qmi_qos_status_report_type  *info_ptr
)
{
  ps_iface_type         *iface_ptr = NULL;           
  ps_flow_type          *flow_ptr  = NULL;
  int16                  ps_errno;
  ps_extended_info_code_enum_type     info_code;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_NULL_CHECK( info_ptr );

  DSQMH_MSG_MED( "DSQMH SM modem qos status ind hdlr: evt=%d",
                 info_ptr->qos_status_information.qos_event,0,0 );

  iface_ptr = DSQMH_GET_IFACE_PTR( iface_inst );
  
  /*-----------------------------------------------------------------------
    Lookup the PS flow for the QMI QOS identifier
  -----------------------------------------------------------------------*/
  if( DSQMH_SUCCESS !=
      dsqmhllif_get_flow_by_qos_id( iface_inst,
                                    info_ptr->qos_status_information.qos_identifier,
                                    &flow_ptr ) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM qos ind: failed flow lookup: iface=%d",
                     iface_inst,0,0 );
    return DSQMH_FAILED;
  }

  /*-----------------------------------------------------------------------
    Verify flow is still valid.  Asynch indication may arrive after
    Iface teardown so flow no longer exists.
  -----------------------------------------------------------------------*/
  if( !PS_FLOW_IS_VALID(flow_ptr) ||
      (NULL == PS_FLOW_GET_PHYS_LINK(flow_ptr)) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM qos ind: ignoring, no longer valid: flow=0x%p",
                     flow_ptr,0,0 );
    return DSQMH_FAILED;
  }
  
  /*-----------------------------------------------------------------------
    Lookup PS info_code value in mapping table 
  -----------------------------------------------------------------------*/  
  info_code = dsqmhsm_info_code_lookup( info_ptr->qos_reason );

  /*-----------------------------------------------------------------------
    Process status event.
  -----------------------------------------------------------------------*/
  switch( info_ptr->qos_status_information.qos_event )
  {
  case QMI_QOS_ACTIVATED_EV:
    /*---------------------------------------------------------------------
      QOS flow activated.
    ---------------------------------------------------------------------*/
    DSQMH_MSG_MED( "DSQMH SM qos flow activated: 0x%p",
                   (uint32)flow_ptr->client_data_ptr,0,0 );
    ps_flow_activate_ind( flow_ptr, info_code );
    break;

  case QMI_QOS_GONE_EV:
    /*---------------------------------------------------------------------
      QOS flow deleted.
    ---------------------------------------------------------------------*/
    DSQMH_MSG_MED( "DSQMH SM qos flow nulled: flow=0x%p",
                   (uint32)flow_ptr->client_data_ptr,0,0 );

    ps_flow_go_null_ind( flow_ptr, info_code );
    /* Delete secondary flows */
    if( flow_ptr != PS_IFACE_GET_DEFAULT_FLOW( iface_ptr ))
    {
      if( DSQMH_SUCCESS != ps_iface_delete_flow( iface_ptr,
                                                 flow_ptr,
                                                 &ps_errno) )
      {
        DSQMH_MSG_ERROR( "DSQMH SM qos ind: failed flow deletion: flow=0x%p",
                         flow_ptr,0,0 );
        return DSQMH_FAILED;
      }
    }
    break;

  case QMI_QOS_SUSPENDED_EV:
    /*---------------------------------------------------------------------
      QOS flow suspended.
    ---------------------------------------------------------------------*/
    DSQMH_MSG_MED( "DSQMH SM qos flow suspended: 0x%p",
                   (uint32)flow_ptr->client_data_ptr,0,0 );
    ps_flow_suspend_ind( flow_ptr, info_code );
    break;

  case QMI_QOS_MODIFY_ACCEPTED_EV:
    /*---------------------------------------------------------------------
      QOS flow modify accepted.
    ---------------------------------------------------------------------*/
    DSQMH_MSG_MED( "DSQMH SM qos flow modify accepted: 0x%p",
                   (uint32)flow_ptr->client_data_ptr,0,0 );
    if( DSQMH_SUCCESS != ps_iface_modify_flow_accepted( iface_ptr, 
                                                        flow_ptr, 
                                                        info_code,
                                                        &ps_errno ) )
    {
      DSQMH_MSG_ERROR( "DSQMH SM qos ind: failed flow modify accepted: flow=0x%p",
                       flow_ptr,0,0 );
      return DSQMH_FAILED;
    }
    break;

  case QMI_QOS_MODIFY_REJECTED_EV:
    /*---------------------------------------------------------------------
      QOS flow modify rejected.
    ---------------------------------------------------------------------*/
    DSQMH_MSG_MED( "DSQMH SM qos flow modify rejected: 0x%p",
                   (uint32)flow_ptr->client_data_ptr,0,0 );
    if( DSQMH_SUCCESS != ps_iface_modify_flow_rejected( iface_ptr, 
                                                        flow_ptr, 
                                                        info_code,
                                                        &ps_errno ) )
    {
      DSQMH_MSG_ERROR( "DSQMH SM qos ind: failed flow modify rejected: flow=0x%p",
                       flow_ptr,0,0 );
      return DSQMH_FAILED;
    }
    break;

  case QMI_QOS_INFO_CODE_UPDATED_EV:
    /*---------------------------------------------------------------------
      QOS flow information updated.
    ---------------------------------------------------------------------*/
    if( info_ptr->qos_reason_is_valid )
    {
      DSQMH_MSG_MED( "DSQMH SM qos flow info code updated: 0x%p",
                     (uint32)flow_ptr->client_data_ptr,0,0 );
      ps_flow_generic_ind( flow_ptr, FLOW_INFO_CODE_UPDATED_EV,
                           (void*)&info_code );
    }
    break;

  default:
    DSQMH_MSG_ERROR( "DSQMH SM unsupported Modem QOS status indication:%d",
                     info_ptr->qos_status_information.qos_status,0,0 );
    return DSQMH_FAILED;
  }

  return DSQMH_SUCCESS;

} /* dsqmhsm_modem_qos_status_ind_hdlr() */


/*===========================================================================

  HELPER FUNCTION: dsqmhsm_modem_qos_event_ind_hdlr

===========================================================================*/
/*!
    @brief
    This funciton handles the QOS event indication message.

    @detail
    Called upon receipt of input PROXY_IFACE_MODEM_QOS_IND.

    @return
    DSQMH_SUCCESS on successful processing.
    DSQMH_FAILED on error.

*/
/*=========================================================================*/
int dsqmhsm_modem_qos_event_ind_hdlr
(
  uint32                       iface_inst,      /* Index for iface table   */
  qmi_qos_event_report_type   *info_ptr
)
{
  ps_iface_type          *iface_ptr = NULL;
  ps_flow_type           *flow_ptr  = NULL;
  dsqmh_iface_cblk_type  *cblk_ptr = NULL;    /* Pointer to control block  */
  int16                   ps_errno;
  ip_flow_type            tx_ip_flow;
  ip_flow_type            rx_ip_flow;
  uint32                  num_flows = 0;
  qmi_qos_technology_type qos_spec_tech;
  int                     index = 0;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_NULL_CHECK( info_ptr );

  if ((info_ptr->num_flows < 0) 
      || (info_ptr->num_flows > QMI_QOS_MAX_FLOW_EVENTS))
  {
    DSQMH_MSG_LOW( "dsqmhsm_modem_qos_event_ind_hdlr: "
                   "Invalid number of flows passed.", 0, 0, 0 );
    return DSQMH_FAILED;
  }

  for (index = 0; index < info_ptr->num_flows; index ++)
  {
    DSQMH_MSG_MED( "DSQMH SM modem qos event ind hdlr: evt=%d",
                     info_ptr->flow_info[index].qos_flow_state.report_flow_state_chng,0,0 );

    iface_ptr = DSQMH_GET_IFACE_PTR( iface_inst );
    cblk_ptr = DSQMH_GET_CBLK_PTR( iface_inst );

    /*-----------------------------------------------------------------------
      Lookup the PS flow for the QMI QOS identifier
      -----------------------------------------------------------------------*/
    if( DSQMH_SUCCESS !=
        dsqmhllif_get_flow_by_qos_id( iface_inst,
                                     info_ptr->flow_info[index].qos_flow_state.qos_identifier,
                                      &flow_ptr ) )
    {
      DSQMH_MSG_ERROR( "DSQMH SM qos ind: failed flow lookup: iface=%d",
                       iface_inst,0,0 );
      return DSQMH_FAILED;
    }

    /*-----------------------------------------------------------------------
      Process event indication.
      -----------------------------------------------------------------------*/
    /* Handle QOS flow state change */
    if( NULL != flow_ptr )
    {
      switch( info_ptr->flow_info[index].qos_flow_state.report_flow_state_chng )
      {
        case QMI_QOS_FLOW_DELETED:
          (void)ps_iface_delete_flow( iface_ptr,
                                      flow_ptr,
                                      &ps_errno );
          DSQMH_MSG_MED( "DSQMH SM qos flow deleted: 0x%p",
                         (uint32)flow_ptr->client_data_ptr,0,0 );
          break;

        case QMI_QOS_FLOW_ACTIVATED:
        case QMI_QOS_FLOW_MODIFIED:
          if( info_ptr->flow_info[index].tx_granted_flow_data_is_valid ||
              info_ptr->flow_info[index].rx_granted_flow_data_is_valid )
          {
            /* Transfer the granted flow/filter spec to the PS flow object. */
            memset(&tx_ip_flow, 0x0, sizeof(tx_ip_flow));
            memset(&rx_ip_flow, 0x0, sizeof(rx_ip_flow));
            qos_spec_tech =
              (qmi_qos_technology_type)cblk_ptr->um_bearer_tech.current_network;
      
            if( info_ptr->flow_info[index].tx_granted_flow_data_is_valid )
            {
              dsqmhllif_qos_conv_flow_params_to_ps( qos_spec_tech,
                                                    &info_ptr->flow_info[index].tx_granted_flow_data.qos_flow_granted,
                                                    &tx_ip_flow,
                                                    &num_flows );
            }
            if( info_ptr->flow_info[index].rx_granted_flow_data_is_valid )
            {
              dsqmhllif_qos_conv_flow_params_to_ps( qos_spec_tech,
                                                    &info_ptr->flow_info[index].rx_granted_flow_data.qos_flow_granted,
                                                    &rx_ip_flow,
                                                    &num_flows );
            }
            DSQMH_MSG_MED( "DSQMH SM qos ind: setting granted flow info",0,0,0 );
            ps_flow_set_granted_flow( flow_ptr, &rx_ip_flow, &tx_ip_flow );
          }
          else
          {
            DSQMH_MSG_MED( "DSQMH SM qos ind: no flow info specified",0,0,0 );
          }
          break;
      
        case QMI_QOS_FLOW_SUSPENDED:
          /* Do nothing, processing done via STATUS indication. */
          break;

        case QMI_QOS_FLOW_ENABLED:
        case QMI_QOS_FLOW_DISABLED:
          /* Do nothing, processing should have been done in callback. */
        default:
          DSQMH_MSG_ERROR( "DSQMH SM unsupported Modem QOS event state:%d",
                           info_ptr->flow_info[index].qos_flow_state.report_flow_state_chng,0,0 );
          return DSQMH_FAILED;
      }
    }
  }
  
  return DSQMH_SUCCESS;
} /* dsqmhsm_modem_qos_event_ind_hdlr() */


/*===========================================================================

  HELPER FUNCTION: dsqmhsm_modem_primary_qos_event_ind_hdlr

===========================================================================*/
/*!
    @brief
    This funciton handles the primary QOS event indication message.

    @detail
    Called upon receipt of input PROXY_IFACE_MODEM_QOS_IND.

    @return
    DSQMH_SUCCESS on successful processing.
    DSQMH_FAILED on error.

*/
/*=========================================================================*/
int dsqmhsm_modem_primary_qos_event_ind_hdlr
(
  uint32                           iface_inst,  /* Index for iface table   */
  qmi_qos_primrary_qos_event_type *info_ptr
)
{
  ps_iface_type         *iface_ptr = NULL;
  ps_flow_type          *flow_ptr  = NULL;
  uint32                 modify_result;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_NULL_CHECK( info_ptr );

  DSQMH_MSG_MED( "DSQMH SM modem primary qos event ind hdlr: res=%d",
                 *info_ptr,0,0 );

  iface_ptr = DSQMH_GET_IFACE_PTR( iface_inst );
  flow_ptr  = PS_IFACE_GET_DEFAULT_FLOW( iface_ptr );

  /*-----------------------------------------------------------------------
    Process event indication.
  -----------------------------------------------------------------------*/
  modify_result =
    (QMI_QOS_PRIMARY_FLOW_MODIFY_SUCCESS == *info_ptr)? TRUE : FALSE;
  ps_flow_generic_ind( flow_ptr, FLOW_PRIMARY_MODIFY_RESULT_EV,
                       (void*)&modify_result );

  return DSQMH_SUCCESS;
} /* dsqmhsm_modem_primary_qos_event_ind_hdlr() */

/*===========================================================================

  TRANSITION FUNCTION:  dsqmhsm_modem_qos_ind_hdlr

===========================================================================*/
/*!
    @brief
    Transition function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_UP,
    upon receiving input PROXY_IFACE_MODEM_QOS_IND

    @detail
    Called upon receipt of input PROXY_IFACE_MODEM_QOS_IND, with optional
    user-passed payload pointer.

    @return
    Returns the next state that the state machine should transition to
    upon receipt of the input.  This state must be a valid state for this
    state machine.

*/
/*=========================================================================*/
stm_state_t dsqmhsm_modem_qos_ind_hdlr
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{
  stm_state_t next_state = STM_SAME_STATE; /* Default 'next' state */
  dsqmh_msg_buf_type    *msg_ptr = NULL;
  ps_iface_type         *iface_ptr = NULL;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );
  STM_NULL_CHECK( payload );

  DSQMH_MSG_LOW( "DSQMH SM modem qos ind hdlr",0,0,0 );

  msg_ptr = (dsqmh_msg_buf_type*)payload;
  iface_ptr = DSQMH_GET_IFACE_PTR( msg_ptr->iface_inst );

  if( !PS_IFACE_IS_VALID( iface_ptr ) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM iface_ptr invalid", 0, 0, 0 );
    return( next_state );
  }

  /*-------------------------------------------------------------------------
    Process QoS supported/unsupported (aware/unaware) indications. 
    Iface need not be UP for this.
  -------------------------------------------------------------------------*/
  if (QMI_QOS_SRVC_NW_STATUS_REPORT_IND_MSG == msg_ptr->info.qmi_qos.ind_id)
  {
    dsqmhsm_modem_qos_network_status_report_ind_hdlr
    ( 
      msg_ptr->iface_inst,
      &msg_ptr->info.qmi_qos.info.nw_report 
    );

    return( next_state );
  }

#ifndef FEATURE_DSS_LINUX  /* Not supported in QMI library yet */
  /*-------------------------------------------------------------------------
    Process QoS profiles change indications. 
    Iface need not be UP for this.
  -------------------------------------------------------------------------*/
  if (QMI_QOS_SRVC_PROFILES_CHANGE_EVENT_IND_MSG ==
       msg_ptr->info.qmi_qos.ind_id)
  {
    dsqmhsm_modem_qos_network_qos_profiles_change_ind_hdlr
    ( 
      msg_ptr->iface_inst,
      &msg_ptr->info.qmi_qos.info.event_report_state
    );

    return( next_state );
  }
#endif /* FEATURE_DSS_LINUX */


  /*-------------------------------------------------------------------------
    Verify IFACE in UP state for other QoS indications.
  -------------------------------------------------------------------------*/
  if( IFACE_UP != ps_iface_state( iface_ptr ) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM qos ind: IFACE not in UP state",0,0,0 );
    return( next_state );
  }

  /*-----------------------------------------------------------------------
    Process QOS  indication.
  -----------------------------------------------------------------------*/
  switch( msg_ptr->info.qmi_qos.ind_id )
  {
    case QMI_QOS_SRVC_STATUS_REPORT_IND_MSG:
      /*---------------------------------------------------------------------
        Process status indication.
      ---------------------------------------------------------------------*/
      if( DSQMH_FAILED ==
          dsqmhsm_modem_qos_status_ind_hdlr( msg_ptr->iface_inst,
                                             &msg_ptr->info.qmi_qos.info.status_report ))
      {
        DSQMH_MSG_ERROR( "DSQMH SM error processing Modem QOS status indication",
                         0,0,0 );
      }
      break;

    case QMI_QOS_SRVC_EVENT_REPORT_IND_MSG:
      /*---------------------------------------------------------------------
        Process event indication.
      ---------------------------------------------------------------------*/
      if( DSQMH_FAILED ==
          dsqmhsm_modem_qos_event_ind_hdlr( msg_ptr->iface_inst,
                                            &msg_ptr->info.qmi_qos.info.event_report ))
      {
        DSQMH_MSG_ERROR( "DSQMH SM error processing Modem QOS event indication",
                         0,0,0 );
      }
      break;

    case QMI_QOS_SRVC_PRIMARY_QOS_EVENT_IND_MSG:
      /*---------------------------------------------------------------------
        Process PRIMARY event indication.
      ---------------------------------------------------------------------*/
      if( DSQMH_FAILED ==
          dsqmhsm_modem_primary_qos_event_ind_hdlr( msg_ptr->iface_inst,
                                                    &msg_ptr->info.qmi_qos.info.primary_qos_modify_result ))
      {
        DSQMH_MSG_ERROR( "DSQMH SM error processing Modem primary QOS event indication",
                         0,0,0 );
      }

      break;

    default:
      DSQMH_MSG_ERROR( "DSQMH SM unsupported Modem QOS indication:%d",
                       msg_ptr->info.qmi_qos.ind_id,0,0 );
      break;
  }

  return( next_state );

} /* dsqmhsm_modem_qos_ind_hdlr() */


/*===========================================================================

  TRANSITION FUNCTION:  dsqmhsm_platform_qos_ind_hdlr

===========================================================================*/
/*!
    @brief
    Transition function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_UP,
    upon receiving input PROXY_IFACE_PLATFORM_QOS_IND

    @detail
    Called upon receipt of input PROXY_IFACE_PLATFORM_QOS_IND, with optional
    user-passed payload pointer.

    @return
    Returns the next state that the state machine should transition to
    upon receipt of the input.  This state must be a valid state for this
    state machine.

*/
/*=========================================================================*/
stm_state_t dsqmhsm_platform_qos_ind_hdlr
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  void                *payload     /*!< Payload pointer */
)
{
  stm_state_t next_state = STM_SAME_STATE; /* Default 'next' state */
  dsqmh_msg_buf_type    *msg_ptr = NULL;
  ps_iface_type         *iface_ptr = NULL;
  int16                  ps_errno;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );
  STM_NULL_CHECK( payload );

  DSQMH_MSG_LOW( "DSQMH SM platform qos ind hdlr",0,0,0 );

  msg_ptr = (dsqmh_msg_buf_type*)payload;
  iface_ptr = DSQMH_GET_IFACE_PTR( msg_ptr->iface_inst );

  /*-----------------------------------------------------------------------
    Process the QOS indication.
  -----------------------------------------------------------------------*/
  switch( msg_ptr->info.netplat.ind_id )
  {
  case DS_NETPLAT_IND_QOS_REQUEST:
    /*---------------------------------------------------------------------
      QOS flow activated.
    ---------------------------------------------------------------------*/
    ps_flow_activate_ind( msg_ptr->info.netplat.flow_ptr, PS_EIC_NOT_SPECIFIED );
    DSQMH_MSG_MED( "DSQMH SM net platform qos flow activated: 0x%p",
                   (uint32)msg_ptr->info.netplat.flow_ptr->client_data_ptr,0,0 );
    break;

  case DS_NETPLAT_IND_QOS_RELEASE:
    /*---------------------------------------------------------------------
      QOS flow deleted.
    ---------------------------------------------------------------------*/
    ps_flow_go_null_ind( msg_ptr->info.netplat.flow_ptr, PS_EIC_NOT_SPECIFIED );
    (void)ps_iface_delete_flow( iface_ptr,
                                msg_ptr->info.netplat.flow_ptr,
                                &ps_errno );
    DSQMH_MSG_MED( "DSQMH SM net platform qos flow deactivated: flow=0x%p errno=%d",
                   (uint32)msg_ptr->info.netplat.flow_ptr->client_data_ptr,
                   ps_errno,0 );
    break;

#if 0
  case DS_NETPLAT_IND_QOS_SUSPEND:
#ifdef FEATURE_DATA_PS_QOS
    /*---------------------------------------------------------------------
      QOS flow suspended.
    ---------------------------------------------------------------------*/
    ps_flow_suspend_ind( msg_ptr->flow_ptr, PS_EIC_NOT_SPECIFIED );
    DSQMH_MSG_MED( "DSQMH SM net platform qos flow suspended: 0x%p",
                   (uint32)msg_ptr->flow_ptr->client_data_ptr,0,0 );
    break;
#endif /* FEATURE_DATA_PS_QOS */
#endif /* 0 */

  default:
    DSQMH_MSG_ERROR( "DSQMH SM unsupported Net Platform QOS indication:%d",
                     msg_ptr->info.netplat.ind_id,0,0 );
    break;
  }

  return( next_state );

} /* dsqmhsm_platform_qos_ind_hdlr() */


/*===========================================================================

     (State Machine: DSQMH_SM)
     STATE ENTRY/EXIT/TRANSITION FUNCTIONS: DSPROXY_IFACE_STATE_PLAT_GOING_DOWN

===========================================================================*/

/*===========================================================================

  STATE ENTRY FUNCTION:  dsqmhsm_state_platform_goingdown_entry

===========================================================================*/
/*!
    @brief
    Entry function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_PLAT_GOING_DOWN

    @detail
    Called upon entry into this state of the state machine, with optional
    user-passed payload pointer parameter.  The prior state of the state
    machine is also passed as the prev_state parameter.

    @return
    None

*/
/*=========================================================================*/
void dsqmhsm_state_platform_goingdown_entry
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         prev_state,  /*!< Previous state */
  void                *payload     /*!< Payload pointer */
)
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_UNUSED( payload );
  STM_UNUSED( prev_state );

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  DSQMH_MSG_MED( "DSQMH SM entering state PLATFORM_GOING_DOWN",0,0,0 );

} /* dsqmhsm_state_platform_goingdown_entry() */


/*===========================================================================

  STATE EXIT FUNCTION:  dsqmhsm_state_platform_goingdown_exit

===========================================================================*/
/*!
    @brief
    Exit function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_PLAT_GOING_DOWN

    @detail
    Called upon exit of this state of the state machine, with optional
    user-passed payload pointer parameter.  The impending state of the state
    machine is also passed as the next_state parameter.

    @return
    None

*/
/*=========================================================================*/
void dsqmhsm_state_platform_goingdown_exit
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         next_state,  /*!< Next state */
  void                *payload     /*!< Payload pointer */
)
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_UNUSED( payload );
  STM_UNUSED( next_state );

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  DSQMH_MSG_LOW( "DSQMH SM exiting state PLATFORM_GOING_DOWN",0,0,0 );

} /* dsqmhsm_state_platform_goingdown_exit() */


/*===========================================================================

     (State Machine: DSQMH_SM)
     STATE ENTRY/EXIT/TRANSITION FUNCTIONS: DSPROXY_IFACE_STATE_GOING_DOWN

===========================================================================*/

/*===========================================================================

  STATE ENTRY FUNCTION:  dsqmhsm_state_goingdown_entry

===========================================================================*/
/*!
    @brief
    Entry function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_GOING_DOWN

    @detail
    Called upon entry into this state of the state machine, with optional
    user-passed payload pointer parameter.  The prior state of the state
    machine is also passed as the prev_state parameter.

    @return
    None

*/
/*=========================================================================*/
void dsqmhsm_state_goingdown_entry
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         prev_state,  /*!< Previous state */
  void                *payload     /*!< Payload pointer */
)
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_UNUSED( payload );
  STM_UNUSED( prev_state );

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  DSQMH_MSG_MED( "DSQMH SM entering state GOING_DOWN",0,0,0 );

} /* dsqmhsm_state_goingdown_entry() */


/*===========================================================================

  STATE EXIT FUNCTION:  dsqmhsm_state_goingdown_exit

===========================================================================*/
/*!
    @brief
    Exit function for state machine DSQMH_SM,
    state DSPROXY_IFACE_STATE_GOING_DOWN

    @detail
    Called upon exit of this state of the state machine, with optional
    user-passed payload pointer parameter.  The impending state of the state
    machine is also passed as the next_state parameter.

    @return
    None

*/
/*=========================================================================*/
void dsqmhsm_state_goingdown_exit
(
  stm_state_machine_t *sm,         /*!< State Machine instance pointer */
  stm_state_t         next_state,  /*!< Next state */
  void                *payload     /*!< Payload pointer */
)
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  STM_UNUSED( payload );
  STM_UNUSED( next_state );

  /* Ensure that the state machine instance pointer passed is valid */
  STM_NULL_CHECK( sm );

  DSQMH_MSG_LOW( "DSQMH SM exiting state GOING_DOWN",0,0,0 );

} /* dsqmhsm_state_goingdown_exit() */


#endif /* FEATURE_DATA_PS && FEATURE_QMI_CLIENT */


