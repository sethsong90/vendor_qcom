/*===========================================================================
  FILE: ps_iface_logicali.c

                    P S _ I F A C E _ L O G I C A L . C

  DEPENDENCIES: None

  Copyright (c) 2009-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                            EDIT HISTORY FOR FILE

  $Header: //source/qcom/qct/modem/datamodem/interface/netiface/rel/11.03/src/ps_iface_logicali.c#2 $
  $Author: hmurari $ $DateTime: 2011/07/10 15:18:14 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
10/23/09    msr    Created file
===========================================================================*/

/*===========================================================================

                                INCLUDE FILES

===========================================================================*/
#include "customer.h"
#include "comdef.h"

#ifdef FEATURE_DATA_PS
#include "amssassert.h"

#include "ds_flow_control.h"
#include "ds_Utils_DebugMsg.h"

#include "dserrno.h"
#include "ps_iface.h"
#include "ps_iface_logicali.h"
#include "ps_iface_logical.h"
#include "ps_iface_logical_flowi.h"
#include "ps_crit_sect.h"
#include "ps_ifacei_event.h"
#include "dcc_task_svc.h"
#include "dcc_task_defs.h"
#include "ps_iface_ipfltr.h"
#include "ps_system_heap.h"


/*===========================================================================

                             INTERNAL FUNCTIONS

===========================================================================*/
void ps_iface_logicali_assoc_iface_ev_cback
(
  ps_iface_type               * ps_iface_ptr,
  ps_iface_event_enum_type      ps_iface_event,
  ps_iface_event_info_u_type    ps_iface_event_info,
  void                        * user_data_ptr
)
{
  ps_logical_iface_ev_cb_cmd_type * assoc_iface_cmd_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Recv event %d on iface 0x%p",
                          ps_iface_event, ps_iface_ptr, 0);

  assoc_iface_cmd_ptr = ps_system_heap_mem_alloc(
                          sizeof(ps_logical_iface_ev_cb_cmd_type));

  if (NULL == assoc_iface_cmd_ptr)
  {
    LOG_MSG_ERROR( "Couldn't get DCC cmd buf", 0, 0, 0 );
    return;
  }

  assoc_iface_cmd_ptr->ps_iface_ptr        = ps_iface_ptr;
  assoc_iface_cmd_ptr->ps_iface_event      = ps_iface_event;
  assoc_iface_cmd_ptr->user_data_ptr       = user_data_ptr;
  assoc_iface_cmd_ptr->ps_iface_event_info = ps_iface_event_info;

  (void) dcc_set_cmd_handler
         (
           DCC_LOGICAL_IFACE_ASSOC_IFACE_EV_CMD,
           ps_iface_logicali_process_assoc_iface_ev
         );

  dcc_send_cmd_ex( DCC_LOGICAL_IFACE_ASSOC_IFACE_EV_CMD, assoc_iface_cmd_ptr);

  LOG_MSG_FUNCTION_EXIT( "Posted cmd 0x%p to DCC task", assoc_iface_cmd_ptr, 0, 0);
  return;

} /* ps_iface_logicali_assoc_iface_ev_cback() */


int32 ps_iface_logicali_handle_nw_init_qos_flow_enable
(
  ps_iface_type * ps_iface_ptr,
  int16         * ps_errno
)
{
  ps_iface_type  * assoc_iface_ptr;
  ps_flow_type   * assoc_flow_ptr;
  ps_flow_type   * logical_flow_ptr;
  int32            ret_val;
  boolean          logical_fltr_spec_enabled;
  boolean          assoc_fltr_spec_enabled;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Iface 0x%p", ps_iface_ptr, 0, 0);

  do
  {
    PS_ENTER_CRIT_SECTION( &global_ps_crit_section);
    if (!PS_IFACE_IS_VALID( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Invalid iface 0x%p", ps_iface_ptr, 0, 0);
      *ps_errno = DS_EINVAL;
      break;
    }

    if (!PS_IFACEI_IS_LOGICAL( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%d not logical",
                             ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      *ps_errno = DS_EINVAL;
      break;
    }

    assoc_iface_ptr = PS_IFACEI_GET_ASSOC_IFACE( ps_iface_ptr);
    if (!PS_IFACE_IS_VALID( assoc_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%p not associated",
                             ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      *ps_errno = DS_EINVAL;
      break;
    }

    logical_flow_ptr =
      list_peek_front( &(ps_iface_ptr->iface_private.flow.sec_flow_list));

    while (logical_flow_ptr != NULL)
    {
      assoc_flow_ptr = PS_FLOW_GET_ASSOC_PS_FLOW( logical_flow_ptr);

      if (!PS_FLOW_IS_VALID( assoc_flow_ptr))
      {
        LOG_MSG_INFO3( "Not processing non-logical flow 0x%p",
                       assoc_flow_ptr, 0, 0);

        logical_flow_ptr =
          list_peek_next(&(ps_iface_ptr->iface_private.flow.sec_flow_list),
                         &(logical_flow_ptr->link));

        continue;
      }

      ret_val = ps_iface_ipfltr_is_handle_enabled
                (
                  ps_iface_ptr,
                  IP_FLTR_CLIENT_QOS_OUTPUT,
                  PS_FLOWI_GET_TX_FLTR_HANDLE( logical_flow_ptr),
                  &logical_fltr_spec_enabled,
                  ps_errno
                );

      if (0 != ret_val)
      {
        LOG_MSG_INFO3_6( "ps_iface_ipfltr_is_handle_enabled() failed, "
                         "iface 0x%x:%d logical flow 0x%p with err %d",
                         ps_iface_ptr->name, ps_iface_ptr->instance,
                         logical_flow_ptr, *ps_errno, 0, 0);

        logical_flow_ptr =
          list_peek_next(&(ps_iface_ptr->iface_private.flow.sec_flow_list),
                         &(logical_flow_ptr->link));

        continue;
      }

      ret_val = ps_iface_ipfltr_is_handle_enabled
                (
                  assoc_iface_ptr,
                  IP_FLTR_CLIENT_QOS_OUTPUT,
                  PS_FLOWI_GET_TX_FLTR_HANDLE( assoc_flow_ptr),
                  &assoc_fltr_spec_enabled,
                  ps_errno
                );

      if (0 != ret_val)
      {
        LOG_MSG_INFO3_6( "ps_iface_ipfltr_is_handle_enabled() failed, "
                         "iface 0x%x:%d assoc flow 0x%p with err %d",
                         assoc_iface_ptr->name, assoc_iface_ptr->instance,
                         assoc_flow_ptr, *ps_errno, 0, 0);

        logical_flow_ptr =
          list_peek_next(&(ps_iface_ptr->iface_private.flow.sec_flow_list),
                         &(logical_flow_ptr->link));

        continue;
      }

      if ( assoc_fltr_spec_enabled != logical_fltr_spec_enabled)
      {
        ret_val = ps_iface_ipfltr_control
                  (
                    ps_iface_ptr,
                    IP_FLTR_CLIENT_QOS_OUTPUT,
                    PS_FLOWI_GET_TX_FLTR_HANDLE( logical_flow_ptr),
                    assoc_fltr_spec_enabled,
                    ps_errno
                  );

        if (0 != ret_val)
        {
          LOG_MSG_INFO3_6( "ps_iface_ipfltr_is_handle_enabled() failed, "
                           "iface 0x%x:%d flow handle 0x%p with err %d",
                           ps_iface_ptr->name, ps_iface_ptr->instance,
                           PS_FLOWI_GET_TX_FLTR_HANDLE( logical_flow_ptr),
                           *ps_errno, 0, 0);

          logical_flow_ptr =
            list_peek_next(&(ps_iface_ptr->iface_private.flow.sec_flow_list),
                           &(logical_flow_ptr->link));

          continue;
        }
      }

      logical_flow_ptr =
        list_peek_next(&(ps_iface_ptr->iface_private.flow.sec_flow_list),
                       &(logical_flow_ptr->link));
    }

    LOG_MSG_FUNCTION_EXIT( "Success, iface 0x%x:%d",
                           ps_iface_ptr->name, ps_iface_ptr->instance, 0);

    PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
    return 0;

  } while (0);

  LOG_MSG_FUNCTION_EXIT( "Fail, iface 0x%x:%d",
                         ps_iface_ptr->name, ps_iface_ptr->instance, 0);

  PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
  return -1;

} /* ps_iface_logicali_handle_nw_init_qos_flow_enable*/


void ps_iface_logicali_process_iface_down_ev
(
  ps_iface_type                  * ps_iface_ptr,
  ps_iface_net_down_reason_type    net_down_reason
)
{
  int16            ps_errno;
  ps_iface_type  * trat_iface_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Iface 0x%p reason %d",
                          ps_iface_ptr, net_down_reason, 0);

  do
  {
    PS_ENTER_CRIT_SECTION( &global_ps_crit_section);

    /*-----------------------------------------------------------------------
      Make sure that iface is valid
    -----------------------------------------------------------------------*/
    if (!PS_IFACE_IS_VALID( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Invalid iface 0x%p", ps_iface_ptr, 0, 0);
      break;
    }

    /*-----------------------------------------------------------------------
      De-register for all events on associated iface
    -----------------------------------------------------------------------*/
    if (-1 == ps_iface_logicali_dereg_iface_ev( ps_iface_ptr, &ps_errno))
    {
      LOG_MSG_INFO3( "Iface 0x%x:%d couldn't dereg for IFACE EV",
                     ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      break;
    }

    /*-----------------------------------------------------------------------
      Teardown the TRAT iface if it exists
    -----------------------------------------------------------------------*/
    trat_iface_ptr = PS_IFACE_GET_TRAT_IFACE( ps_iface_ptr);
    if (PS_IFACE_IS_VALID( trat_iface_ptr) &&
        trat_iface_ptr != PS_IFACE_GET_ASSOC_IFACE( ps_iface_ptr))
    {
      (void) ps_iface_go_null_cmd( trat_iface_ptr, &ps_errno, NULL);
    }

    /*-----------------------------------------------------------------------
      Set the associated iface and TRAT to NULL
    -----------------------------------------------------------------------*/
    PS_IFACE_SET_ASSOC_IFACE( ps_iface_ptr, NULL);
    PS_IFACE_SET_TRAT_IFACE( ps_iface_ptr, NULL);

    ps_iface_ptr->iface_private.inherit_ip_info = FALSE;

    /*-------------------------------------------------------------------
      Post DOWN IND on the logical iface
    -------------------------------------------------------------------*/
    ps_iface_down_ind_ex( ps_iface_ptr, net_down_reason);

    PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);

    LOG_MSG_FUNCTION_EXIT( "Success, iface 0x%x:%d",
                           ps_iface_ptr->name, ps_iface_ptr->instance, 0);

    return;

  } while ( 0);

  LOG_MSG_FUNCTION_EXIT( "Fail, iface 0x%x:%d",
                         ps_iface_ptr->name, ps_iface_ptr->instance, 0);

  PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
} /* ps_iface_logicali_process_iface_down_ev */


/*===========================================================================

                              EXTERNAL FUNCTIONS

===========================================================================*/
int32 ps_iface_logicali_reg_iface_all_ev
(
  ps_iface_type  * ps_iface_ptr,
  int16          * ps_errno_ptr
)
{
  ps_iface_type  * assoc_iface_ptr = NULL;
  int32            ret_val;
  boolean          is_iface_ev_cback_buf_ptr_alloced = FALSE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Iface 0x%p", ps_iface_ptr, 0, 0);

  if (NULL == ps_errno_ptr)
  {
    LOG_MSG_INVALID_INPUT( "NULL errno", 0, 0, 0);
    return -1;
  }

  do
  {
    PS_ENTER_CRIT_SECTION( &global_ps_crit_section);

    /*-----------------------------------------------------------------------
      Make sure that iface is valid and is a logical iface
    -----------------------------------------------------------------------*/
    if (!PS_IFACE_IS_VALID( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Invalid iface 0x%p", ps_iface_ptr, 0, 0);
      *ps_errno_ptr = DS_EINVAL;
      break;
    }

    if (!PS_IFACE_IS_LOGICAL( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%d not logical",
                             ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      *ps_errno_ptr = DS_EINVAL;
      break;
    }

    /*-----------------------------------------------------------------------
      Register for event only if it hadn't already been registered
    -----------------------------------------------------------------------*/
    if (NULL !=
          ps_iface_ptr->iface_private.logical_iface_info.iface_ev_cback_buf_ptr)
    {
      LOG_MSG_INFO1( "Iface 0x%x:%p registered for ev already",
                     ps_iface_ptr->name, ps_iface_ptr->instance, 0);

      PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
      return 0;
    }

    /*-----------------------------------------------------------------------
      Allocate an event buffer to register for all iface events on
      associated iface
    -----------------------------------------------------------------------*/
    ps_iface_ptr->iface_private.logical_iface_info.iface_ev_cback_buf_ptr =
      ps_iface_alloc_event_cback_buf( ps_iface_logicali_assoc_iface_ev_cback,
                                      ps_iface_ptr);
    if (NULL ==
          ps_iface_ptr->iface_private.logical_iface_info.iface_ev_cback_buf_ptr)
    {
      LOG_MSG_INFO3( "Couldn't get iface ev buf, iface 0x%x:%d",
                     ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      *ps_errno_ptr = DS_ENOMEM;
      break;
    }

    is_iface_ev_cback_buf_ptr_alloced = TRUE;

    /*-----------------------------------------------------------------------
      Register for all iface events on associated iface
    -----------------------------------------------------------------------*/
    assoc_iface_ptr = PS_IFACE_GET_ASSOC_IFACE( ps_iface_ptr);
    if (NULL == assoc_iface_ptr)
    {
      LOG_MSG_ERROR( "Assoc iface for 0x%x:%d NULL",
                      ps_iface_ptr->name,
                      ps_iface_ptr->instance,
                      0);
      *ps_errno_ptr = DS_EINVAL;
      break;
    }

    ret_val =
      ps_iface_event_cback_reg
      (
        assoc_iface_ptr,
        IFACE_ALL_EV,
        ps_iface_ptr->iface_private.logical_iface_info.iface_ev_cback_buf_ptr
      );

    if (-1 == ret_val)
    {
      LOG_MSG_INFO3( "Iface 0x%x:%d couldn't reg for ev on iface 0x%p",
                     ps_iface_ptr->name,
                     ps_iface_ptr->instance,
                     assoc_iface_ptr);
      *ps_errno_ptr = DS_EINVAL;
      break;
    }

    LOG_MSG_FUNCTION_EXIT_6( "Iface 0x%x:%d registered for ev on iface 0x%x:%d",
                             ps_iface_ptr->name,
                             ps_iface_ptr->instance,
                             assoc_iface_ptr->name,
                             assoc_iface_ptr->instance,
                             0,
                             0);

    PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
    return 0;

  } while (0);

  /*-------------------------------------------------------------------------
    Common error handling

    Free the allocated buffer
  -------------------------------------------------------------------------*/
  if (is_iface_ev_cback_buf_ptr_alloced)
  {
    PS_IFACE_LOGICALI_DEREG_EV_BUF
    (
      assoc_iface_ptr,
      IFACE_ALL_EV,
      ps_iface_ptr->iface_private.logical_iface_info.iface_ev_cback_buf_ptr
    );
  }

  LOG_MSG_FUNCTION_EXIT( "Fail, iface 0x%x:%d",
                         ps_iface_ptr->name, ps_iface_ptr->instance, 0);

  PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
  return -1;

} /* ps_iface_logicali_reg_iface_all_ev() */


int32 ps_iface_logicali_dereg_iface_ev
(
  ps_iface_type  * ps_iface_ptr,
  int16          * ps_errno_ptr
)
{
  ps_iface_type  * assoc_iface_ptr;
  int32            ret_val;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Iface 0x%p", ps_iface_ptr, 0, 0);

  if (NULL == ps_errno_ptr)
  {
    LOG_MSG_INVALID_INPUT( "NULL errno", 0, 0, 0);
    return -1;
  }

  do
  {
    PS_ENTER_CRIT_SECTION( &global_ps_crit_section);

    /*-----------------------------------------------------------------------
      Make sure that iface is valid and is a logical iface
    -----------------------------------------------------------------------*/
    if (!PS_IFACE_IS_VALID( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Invalid iface 0x%p", ps_iface_ptr, 0, 0);
      *ps_errno_ptr = DS_EINVAL;
      ret_val = -1;
      break;
    }

    if (!PS_IFACE_IS_LOGICAL( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%d not logical",
                             ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      *ps_errno_ptr = DS_EINVAL;
      ret_val = -1;
      break;
    }

    /*-----------------------------------------------------------------------
      Make sure that iface is associated to another iface
    -----------------------------------------------------------------------*/
    assoc_iface_ptr = PS_IFACE_GET_ASSOC_IFACE( ps_iface_ptr);
    if (NULL == assoc_iface_ptr)
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%p not associated",
                             ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      *ps_errno_ptr = DS_EINVAL;
      ret_val = -1;
      break;
    }

    /*-----------------------------------------------------------------------
      De-register IFACE_UP_EV
    -----------------------------------------------------------------------*/
    if (NULL !=
          ps_iface_ptr->iface_private.logical_iface_info.iface_up_ev_cback_buf_ptr)
    {
      PS_IFACE_LOGICALI_DEREG_EV_BUF
      (
        assoc_iface_ptr,
        IFACE_UP_EV,
        ps_iface_ptr->iface_private.logical_iface_info.iface_up_ev_cback_buf_ptr
      );
    }

    /*-----------------------------------------------------------------------
      De-register IFACE_DOWN_EV
    -----------------------------------------------------------------------*/
    if (NULL !=
          ps_iface_ptr->iface_private.logical_iface_info.iface_down_ev_cback_buf_ptr)
    {
      PS_IFACE_LOGICALI_DEREG_EV_BUF
      (
        assoc_iface_ptr,
        IFACE_DOWN_EV,
        ps_iface_ptr->iface_private.logical_iface_info.iface_down_ev_cback_buf_ptr
      );
    }

    /*-----------------------------------------------------------------------
      De-register IFACE_ALL_EV
    -----------------------------------------------------------------------*/
    if (NULL !=
          ps_iface_ptr->iface_private.logical_iface_info.iface_ev_cback_buf_ptr)
    {
      PS_IFACE_LOGICALI_DEREG_EV_BUF
      (
        assoc_iface_ptr,
        IFACE_ALL_EV,
        ps_iface_ptr->iface_private.logical_iface_info.iface_ev_cback_buf_ptr
      );
    }

    LOG_MSG_FUNCTION_EXIT_6( "Iface 0x%x:%d de-registered for ev on "
                             "iface 0x%x:%d",
                             ps_iface_ptr->name,
                             ps_iface_ptr->instance,
                             assoc_iface_ptr->name,
                             assoc_iface_ptr->instance,
                             0,
                             0);

    PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
    return 0;

  } while (0);

  /*-------------------------------------------------------------------------
    Common error handling
  -------------------------------------------------------------------------*/
  LOG_MSG_FUNCTION_EXIT( "Returning %d, iface 0x%x:%d",
                         ret_val,
                         ps_iface_ptr->name,
                         ps_iface_ptr->instance);

  PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
  return ret_val;

} /* ps_iface_logicali_dereg_iface_ev() */


void ps_iface_logicali_process_assoc_iface_ev
(
  dcc_cmd_enum_type   cmd,
  void              * user_data_ptr
)
{
  ps_logical_iface_ev_cb_cmd_type * assoc_iface_cmd_ptr;
  ps_iface_type                   * ps_iface_ptr;
  ps_iface_event_enum_type          ps_iface_event;
  ps_iface_event_info_u_type        ps_iface_event_info;
  ps_iface_type                   * ps_iface_logical_ptr;
  int16                             ps_errno;
  ps_flow_type                    * assoc_flow_ptr;
  int32                             ret_val;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "DCC cmd ptr 0x%p, cmd %d", user_data_ptr, cmd, 0);

  assoc_iface_cmd_ptr = (ps_logical_iface_ev_cb_cmd_type*)user_data_ptr;

  if (NULL == assoc_iface_cmd_ptr)
  {
    LOG_MSG_INVALID_INPUT( "NULL dcc cmd buf", 0, 0, 0);
    return;
  }

  if (DCC_LOGICAL_IFACE_ASSOC_IFACE_EV_CMD != cmd)
  {
    LOG_MSG_INVALID_INPUT( "Invalid DCC cmd id %d",
                           cmd, 0, 0);
    PS_SYSTEM_HEAP_MEM_FREE(user_data_ptr);
    return;
  }

  ps_iface_ptr         = assoc_iface_cmd_ptr->ps_iface_ptr;
  ps_iface_event       = assoc_iface_cmd_ptr->ps_iface_event;
  ps_iface_event_info  = assoc_iface_cmd_ptr->ps_iface_event_info;
  ps_iface_logical_ptr = (ps_iface_type *)
                           ( assoc_iface_cmd_ptr->user_data_ptr);
  PS_SYSTEM_HEAP_MEM_FREE(assoc_iface_cmd_ptr);

  LOG_MSG_INFO1( "Processing event %d on assoc iface 0x%p for "
                 "logical iface 0x%p",
                 ps_iface_event, ps_iface_ptr, ps_iface_logical_ptr);

  do
  {
    PS_ENTER_CRIT_SECTION( &global_ps_crit_section);

    /*-----------------------------------------------------------------------
      Make sure that iface is valid
    -----------------------------------------------------------------------*/
    if (!PS_IFACE_IS_VALID( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Invalid iface 0x%p", ps_iface_ptr, 0, 0);
      break;
    }

    /*-----------------------------------------------------------------------
      Make sure that ps_iface_logical_ptr is valid and is a logical iface
    -----------------------------------------------------------------------*/
    if (!PS_IFACE_IS_VALID( ps_iface_logical_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Invalid logical_iface 0x%p",
                             ps_iface_logical_ptr, 0, 0);
      break;
    }

    if (!PS_IFACE_IS_LOGICAL( ps_iface_logical_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%d not logical",
                             ps_iface_logical_ptr->name,
                             ps_iface_logical_ptr->instance,
                             0);
      break;
    }

    /*-----------------------------------------------------------------------
      Make sure that iface, on which event is received, is associated to the
      logical iface
    -----------------------------------------------------------------------*/
    if (ps_iface_ptr != PS_IFACE_GET_ASSOC_IFACE( ps_iface_logical_ptr))
    {
      LOG_MSG_ERROR_6( "Iface 0x%x:%d not associated to iface 0x%x:%d",
                       ps_iface_ptr->name,
                       ps_iface_ptr->instance,
                       ps_iface_logical_ptr->name,
                       ps_iface_logical_ptr->instance,
                       0,
                       0);
      break;
    }

    switch (ps_iface_event)
    {
      /*---------------------------------------------------------------------
        Ignore transient state events from associated iface as logical iface's
        mode handler is responsible for managing its state
      ---------------------------------------------------------------------*/
      case IFACE_COMING_UP_EV:
      case IFACE_CONFIGURING_EV:
      case IFACE_GOING_DOWN_EV:
      case IFACE_LINGERING_EV:
      {
        break;
      }

      case IFACE_IPFLTR_UPDATED_EV:
      {
        ret_val =
          ps_iface_logicali_handle_nw_init_qos_flow_enable( ps_iface_logical_ptr,
                                                            &ps_errno);

        if (0 != ret_val)
        {
          LOG_MSG_INFO3("Error enabling logical flows corresponding to network "
                        "initiated flows associated flows %d", ps_errno, 0, 0);
        }

        break;
      }

      case IFACE_FLOW_ADDED_EV:
      {
        assoc_flow_ptr = ps_iface_event_info.flow_ptr;

        ret_val = ps_iface_logical_flowi_handle_nw_init_qos_flow_selective
                  (
                    ps_iface_logical_ptr,
                    assoc_flow_ptr
                  );

        if (0 != ret_val)
        {
          LOG_MSG_INFO3("Error creating logical flows corresponding to network "
                        "initiated flows associated flows", 0, 0, 0);
        }

        break;
      }

      /*---------------------------------------------------------------------
        Ignore following events as they are local to associated iface
      ---------------------------------------------------------------------*/
      case IFACE_PRI_PHYS_LINK_CHANGED_EV:
      case IFACE_FLOW_DELETED_EV:
      case IFACE_ENABLED_EV:
      case IFACE_DISABLED_EV:
      {
        break;
      }

      case IFACE_UP_EV:
      {
        /*-------------------------------------------------------------------
          Swap RAT if the TRAT is the source of the event. The purpose of the
          following op is to swap trat(src of UP ev) to srat. No action is
          needed if swap already happened (e.g., during handoff swap)
        -------------------------------------------------------------------*/
        if (ps_iface_ptr == PS_IFACE_GET_TRAT_IFACE( ps_iface_logical_ptr))
        {
          ret_val = ps_iface_logical_swap_rat( ps_iface_logical_ptr, &ps_errno);
          if (-1 == ret_val)
          {
            LOG_MSG_ERROR( "Ignoring IFACE_UP_EV from assoc iface, "
                           "Iface 0x%x:%d with err %d",
                           ps_iface_logical_ptr->name,
                           ps_iface_logical_ptr->instance,
                           ps_errno);
            break;
          }

          ps_iface_up_ind( ps_iface_logical_ptr);
        }

        break;
      }

      case IFACE_ROUTEABLE_EV:
      {
        ps_iface_routeable_ind( ps_iface_logical_ptr);
        break;
      }

      case IFACE_DOWN_EV:
      {
        ps_iface_logicali_process_iface_down_ev
        (
          ps_iface_logical_ptr,
          ps_iface_event_info.iface_down_info.netdown_reason
        );

        break;
      }

      case IFACE_FLOW_ENABLED_EV:
      {
        ps_iface_enable_flow( ps_iface_logical_ptr, DS_FLOW_ASSOC_IFACE_MASK);
        break;
      }

      case IFACE_FLOW_DISABLED_EV:
      {
        ps_iface_disable_flow( ps_iface_logical_ptr, DS_FLOW_ASSOC_IFACE_MASK);
        break;
      }

      case IFACE_EXTENDED_IP_CONFIG_EV:
      case IFACE_ADDR_FAMILY_CHANGED_EV:
      case IFACE_ADDR_CHANGED_EV:
      case IFACE_PREFIX_UPDATE_EV:
      case IFACE_VALID_RA_EV:
      case IFACE_IPV6_PRIV_ADDR_GENERATED_EV:
      case IFACE_IPV6_PRIV_ADDR_DEPRECATED_EV:
      case IFACE_IPV6_PRIV_ADDR_EXPIRED_EV:
      case IFACE_IPV6_PRIV_ADDR_DELETED_EV:
      case IFACE_BEARER_TECH_CHANGED_EV:
      case IFACE_OUTAGE_NOTIFICATION_EV:
      case IFACE_RF_CONDITIONS_CHANGED_EV:
      case IFACE_AUTHENTICATING_EV:
      case IFACE_APP_PREEMPTED_EV:
      case IFACE_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_SUCCESS_EV:
      case IFACE_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_FAILURE_EV:
      case IFACE_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_SESSION_CHANGED_EV:
      case IFACE_ENABLE_HDR_REV0_RATE_INERTIA_SUCCESS_EV:
      case IFACE_ENABLE_HDR_REV0_RATE_INERTIA_FAILURE_EV:
      case IFACE_QOS_AWARE_SYSTEM_EV:
      case IFACE_QOS_UNAWARE_SYSTEM_EV:
      case IFACE_707_NETWORK_SUPPORTED_QOS_PROFILES_CHANGED_EV:
      case IFACE_MT_REQUEST_EV:
      case IFACE_MCAST_REGISTER_SUCCESS_EV:
      case IFACE_MCAST_REGISTER_FAILURE_EV:
      case IFACE_MCAST_DEREGISTERED_EV:
      case IFACE_MCAST_STATUS_EV:
      case IFACE_MBMS_CONTEXT_ACT_SUCCESS_EV:
      case IFACE_MBMS_CONTEXT_ACT_FAILURE_EV:
      case IFACE_MBMS_CONTEXT_DEACT_SUCCESS_EV:
      case IFACE_MBMS_CONTEXT_DEACT_FAILURE_EV:
      case IFACE_MTU_CHANGED_EV:
      case IFACE_IDLE_EV:
      {
        ps_ifacei_invoke_event_cbacks( ps_iface_logical_ptr,
                                       NULL,
                                       ps_iface_event,
                                       ps_iface_event_info);
        break;
      }

      default:
      {
        LOG_MSG_ERROR( "Unknown ev %d", ps_iface_event, 0, 0);
        break;
      }
    }

    LOG_MSG_FUNCTION_EXIT( "Success, iface 0x%x:%d",
                           ps_iface_ptr->name, ps_iface_ptr->instance, 0);

    PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
    return;

  } while ( 0);

  LOG_MSG_FUNCTION_EXIT( "Fail, iface 0x%x:%d",
                         ps_iface_ptr->name, ps_iface_ptr->instance, 0);

  PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
  return;

} /* ps_iface_logicali_process_assoc_iface_ev() */

#endif /* FEATURE_DATA_PS */
