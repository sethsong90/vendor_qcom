/*===========================================================================
  FILE: ps_iface_logical.c

  OVERVIEW: TODO

  DEPENDENCIES: None

  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/netiface/rel/11.03/src/ps_iface_logical.c#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-12-06 asn Fix latent bug and return -1 to indicate error
  2010-11-12 asn Propagate arbitration in progress flag to physical Ifaces
  2010-10-29 asn support stopping of active ifaces when in arb
  2009-10-23 msr Created file

===========================================================================*/

/*===========================================================================

                                INCLUDE FILES

===========================================================================*/
#include "customer.h"
#include "comdef.h"

#ifdef FEATURE_DATA_PS
#include "ps_iface_logical.h"
#include "msg.h"
#include "amssassert.h"

#include "dserrno.h"
#include "ds_Utils_DebugMsg.h"
#include "ds_flow_control.h"

#include "ps_iface_logical_flowi.h"
#include "ps_iface.h"
#include "ps_ifacei.h"
#include "ps_routei_acl.h"
#include "ps_crit_sect.h"
#include "ps_iface_logicali.h"
#include "ps_metai_info.h"
#include "ps_flowi_event.h"
#include "ps_ifacei_utils.h"

/*===========================================================================

                              EXTERNS

===========================================================================*/
extern int ps_iface_arbitration_tear_down_cmd
(
  ps_iface_type *this_iface_ptr,
  int16         *ps_errno,
  void          *client_data_ptr
);

/*===========================================================================

                              EXTERNAL FUNCTIONS

===========================================================================*/
int32 ps_iface_logical_create
(
  ps_iface_type            * this_iface_ptr,
  ps_iface_name_enum_type    name,
  acl_type                 * this_iface_outgoing_acl_ptr,
  acl_type                 * this_iface_incoming_acl_ptr,
  ps_phys_link_type        * phys_link_array,
  uint8                      num_phys_links,
  boolean                    inherit_ip_info
)
{
  int32  instance;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Iface 0x%p name 0x%x inherit_ip_info %d",
                          this_iface_ptr, name, inherit_ip_info);

  instance = ps_iface_create_internal( this_iface_ptr,
                                       name,
                                       phys_link_array,
                                       num_phys_links);
  if (-1 == instance)
  {
    LOG_MSG_INFO3( "ps_iface_create_internal failed on iface 0x%p",
                   this_iface_ptr, 0, 0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    Set up PS Iface's default input/output functions.
    If the iface being created is a logical iface set default tx cmd to the
    default logical iface tx cmd.
  -------------------------------------------------------------------------*/
#ifdef FEATURE_DATACOMMON_PS_IFACE_IO
  this_iface_ptr->iface_private.tx_cmd = ps_ifacei_logical_default_tx_cmd;
#else
  this_iface_ptr->iface_private.tx_cmd = NULL;
#endif /* FEATURE_DATACOMMON_PS_IFACE_IO */

  this_iface_ptr->iface_private.is_logical = TRUE;

  /*-------------------------------------------------------------------------
    Add the mode handler's outgoing ACL for this interface and also store it
    in this IFACE.
  -------------------------------------------------------------------------*/
  if (this_iface_outgoing_acl_ptr)
  {
    this_iface_outgoing_acl_ptr->if_ptr = (void *) this_iface_ptr;
    (void) route_acl_add( ROUTE_ACL_LOGICAL_OUTBOUND,
                          this_iface_outgoing_acl_ptr,
                          ROUTE_ACL_ANY_PRIORITY);
    this_iface_ptr->outgoing_acl_ptr = this_iface_outgoing_acl_ptr;
  }

  /*-------------------------------------------------------------------------
    Add the mode handler's incoming ACL for this interface
  -------------------------------------------------------------------------*/
  if (this_iface_incoming_acl_ptr)
  {
    this_iface_incoming_acl_ptr->if_ptr = (void *) this_iface_ptr;
    (void) route_acl_add( ROUTE_ACL_DEFAULT_INBOUND,
                          this_iface_incoming_acl_ptr,
                          ROUTE_ACL_ANY_PRIORITY);
  }

  this_iface_ptr->iface_private.inherit_ip_info = inherit_ip_info;

  LOG_MSG_FUNCTION_EXIT( "Success, iface 0x%x:%d",
                         this_iface_ptr->name, this_iface_ptr->instance, 0);
  return instance;

} /* ps_iface_logical_create() */

int32 ps_iface_logical_default_bring_up_cmd_ex_hdlr
(
  ps_iface_type  * ps_iface_ptr,
  int32            app_priority,
  void           * client_data_ptr
)
{
  ps_iface_type  * assoc_iface_ptr = NULL;
  int32            ret_val;
  int16            ps_errno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Iface 0x%p, app priority %d",
                          ps_iface_ptr, app_priority, 0);

  do
  {
    PS_ENTER_CRIT_SECTION( &global_ps_crit_section);

    /*-----------------------------------------------------------------------
      Make sure that iface is valid and is a logical iface
    -----------------------------------------------------------------------*/
    if (!PS_IFACE_IS_VALID( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Invalid iface 0x%p", ps_iface_ptr, 0, 0);
      break;
    }

    if (!PS_IFACE_IS_LOGICAL( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%d not logical",
                             ps_iface_ptr->name, ps_iface_ptr->instance, 0);
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
      break;
    }

    /*-----------------------------------------------------------------------
      Make sure that iface is in COMING_UP state as it is not valid to call
      bring_up_cmd_f_ptr in any other state
    -----------------------------------------------------------------------*/
    if (IFACE_COMING_UP != PS_IFACEI_GET_STATE( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%p in invalid state 0x%p",
                             ps_iface_ptr->name,
                             ps_iface_ptr->instance,
                             PS_IFACEI_GET_STATE( ps_iface_ptr));
      break;
    }

    ps_iface_ptr->iface_private.inherit_ip_info = TRUE;

    PS_IFACE_SET_ASSOC_IFACE( ps_iface_ptr,
                              PS_IFACE_GET_TRAT_IFACE( ps_iface_ptr));

    /*-----------------------------------------------------------------------
      Bring up the associated iface. This step must happen before the function
      registers for IFACE_DOWN_EV. Otherwise, IFACE_DOWN_EV will immediately
      be posted if associated iface is in IFACE_DOWN state and the logical
      iface module thinks that the bring up failed and aborts the connection
    -----------------------------------------------------------------------*/
    ret_val = ps_iface_bring_up_cmd_ex( assoc_iface_ptr,
                                        app_priority,
                                       &ps_errno,
                                        client_data_ptr);

    if (-1 == ret_val && DS_EWOULDBLOCK != ps_errno)
    {
      LOG_MSG_INFO3( "ps_iface_bring_up_cmd_ex failed on iface 0x%x:%d err %d",
                     assoc_iface_ptr->name,
                     assoc_iface_ptr->instance,
                     ps_errno);

      ps_iface_ptr->iface_private.ps_errno_for_bring_up_cmd_f_ptr_failure =
        ps_errno;

      if (DS_ENETCLOSEINPROGRESS == ps_errno)
      {
        ps_iface_ptr->iface_private.logical_iface_info.iface_down_ev_cback_buf_ptr =
          ps_iface_alloc_event_cback_buf( ps_iface_logicali_assoc_iface_ev_cback,
                                          ps_iface_ptr);

        if (NULL ==
            ps_iface_ptr->iface_private.logical_iface_info.iface_down_ev_cback_buf_ptr)
        {
          LOG_MSG_INFO3( "Couldn't get iface ev buf, iface 0x%x:%d",
                          ps_iface_ptr->name, ps_iface_ptr->instance, 0);
          break;
        }

        /*-----------------------------------------------------------------------
          Register for IFACE_DOWN_EV on associated iface
        -----------------------------------------------------------------------*/
        ret_val = ps_iface_event_cback_reg
                  (
                    assoc_iface_ptr,
                    IFACE_DOWN_EV,
                    ps_iface_ptr->iface_private.logical_iface_info.iface_down_ev_cback_buf_ptr
                  );

        if (-1 == ret_val)
        {
          LOG_MSG_INFO3_6( "Iface 0x%x:%d couldn't reg for IFACE_DOWN_EV "
                           "on iface 0x%x:%d",
                           ps_iface_ptr->name, ps_iface_ptr->instance,
                           assoc_iface_ptr->name, assoc_iface_ptr->instance, 0, 0);
          break;
        }

        PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
        return -1;
      }

      break;
    }

    /*-----------------------------------------------------------------------
      Allocate an event buffer to register for IFACE_UP_EV on associated iface
    -----------------------------------------------------------------------*/
    ps_iface_ptr->iface_private.logical_iface_info.iface_up_ev_cback_buf_ptr =
      ps_iface_alloc_event_cback_buf( ps_iface_logicali_assoc_iface_ev_cback,
                                      ps_iface_ptr);

    if (NULL ==
          ps_iface_ptr->iface_private.logical_iface_info.iface_up_ev_cback_buf_ptr)
    {
      LOG_MSG_INFO3( "Couldn't get iface ev buf, iface 0x%x:%d",
                     ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      break;
    }

    /*-----------------------------------------------------------------------
      Register for IFACE_UP_EV on associated iface
    -----------------------------------------------------------------------*/
    ret_val = ps_iface_event_cback_reg
              (
                assoc_iface_ptr,
                IFACE_UP_EV,
                ps_iface_ptr->iface_private.logical_iface_info.iface_up_ev_cback_buf_ptr
              );

    if (-1 == ret_val)
    {
      LOG_MSG_INFO3_6( "Iface 0x%x:%d couldn't reg for IFACE_UP_EV "
                       "on iface 0x%x:%d",
                       ps_iface_ptr->name, ps_iface_ptr->instance,
                       assoc_iface_ptr->name, assoc_iface_ptr->instance, 0, 0);
      break;
    }

    /*-----------------------------------------------------------------------
      Allocate an event buffer to register for IFACE_DOWN_EV on
      associated iface.

      Do not register for DOWN_EV on assoc iface when ps_iface is in UP state
      (possible if assoc_iface is active and the ev reg above for UP_EV would
       modify the state of ps_iface), since
        1. No need, end result is already known
        2. Avoid creating dangling down_ev buffer, since there is no more UP_EV
           from assoc iface would be posted for de-reg it.

           //MSR Dont like this model. One option is to register for
           //IFACE_ALL_EV from begining but got to be careful about MH
           //managing logical ifaces on their own
    -----------------------------------------------------------------------*/
    if (IFACE_UP != PS_IFACEI_GET_STATE( ps_iface_ptr))
    {
      ps_iface_ptr->iface_private.logical_iface_info.iface_down_ev_cback_buf_ptr =
        ps_iface_alloc_event_cback_buf( ps_iface_logicali_assoc_iface_ev_cback,
                                        ps_iface_ptr);

      if (NULL ==
          ps_iface_ptr->iface_private.logical_iface_info.iface_down_ev_cback_buf_ptr)
      {
        LOG_MSG_INFO3( "Couldn't get iface ev buf, iface 0x%x:%d",
                        ps_iface_ptr->name, ps_iface_ptr->instance, 0);
        break;
      }

      /*-----------------------------------------------------------------------
        Register for IFACE_DOWN_EV on associated iface
      -----------------------------------------------------------------------*/
      ret_val = ps_iface_event_cback_reg
                (
                  assoc_iface_ptr,
                  IFACE_DOWN_EV,
                  ps_iface_ptr->iface_private.logical_iface_info.iface_down_ev_cback_buf_ptr
                );

      if (-1 == ret_val)
      {
        LOG_MSG_INFO3_6( "Iface 0x%x:%d couldn't reg for IFACE_DOWN_EV "
                         "on iface 0x%x:%d",
                         ps_iface_ptr->name, ps_iface_ptr->instance,
                         assoc_iface_ptr->name, assoc_iface_ptr->instance, 0, 0);
        break;
      }
    }

    LOG_MSG_INFO1_6( "Iface 0x%x:%d registered for UP/DOWN ev on iface 0x%x:%d",
                     ps_iface_ptr->name, ps_iface_ptr->instance,
                     assoc_iface_ptr->name, assoc_iface_ptr->instance, 0, 0);

    LOG_MSG_FUNCTION_EXIT( "Success, iface 0x%x:%d",
                           ps_iface_ptr->name, ps_iface_ptr->instance, 0);

    PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
    return 0;

  } while (0);

  /*-------------------------------------------------------------------------
    Common error handling

    Free the allocated buffers
  -------------------------------------------------------------------------*/
  if (NULL != ps_iface_ptr->iface_private.logical_iface_info.iface_up_ev_cback_buf_ptr)
  {
    PS_IFACE_LOGICALI_DEREG_EV_BUF
    (
      assoc_iface_ptr,
      IFACE_UP_EV,
      ps_iface_ptr->iface_private.logical_iface_info.iface_up_ev_cback_buf_ptr
    );
  }

  if (NULL != ps_iface_ptr->iface_private.logical_iface_info.iface_down_ev_cback_buf_ptr)
  {
    PS_IFACE_LOGICALI_DEREG_EV_BUF
    (
      assoc_iface_ptr,
      IFACE_DOWN_EV,
      ps_iface_ptr->iface_private.logical_iface_info.iface_down_ev_cback_buf_ptr
    );
  }

  LOG_MSG_FUNCTION_EXIT( "Fail, iface 0x%x:%d",
                         ps_iface_ptr->name, ps_iface_ptr->instance, 0);

  PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
  return -1;

} /* ps_iface_logical_default_bring_up_cmd_hdlr_ex() */


int32 ps_iface_logical_default_bring_up_cmd_hdlr
(
  ps_iface_type  * ps_iface_ptr,
  void           * client_data_ptr
)
{
  LOG_MSG_INFO2( "calling ex version of bring up hdlr", 0, 0, 0 );
  return ps_iface_logical_default_bring_up_cmd_ex_hdlr( ps_iface_ptr,
                                                        PS_POLICY_MGR_LEGACY_PRIORITY,
                                                        client_data_ptr );
}

int32 ps_iface_logical_default_tear_down_cmd_ex_hdlr
(
  ps_iface_type  * ps_iface_ptr,
  int32            app_priority,
  void           * client_data_ptr
)
{
  ps_iface_type  * assoc_iface_ptr;
  ps_iface_type  * trat_iface_ptr;
  ps_flow_type   * ps_flow_ptr;
  ps_flow_type   * tmp_ps_flow_ptr;
  int32            ret_val;
  int16            ps_errno;
  int32            trat_iface_app_priority;
  int32            assoc_iface_app_priority;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Iface 0x%p, app priority %d",
                          ps_iface_ptr, app_priority, 0);

  do
  {
    PS_ENTER_CRIT_SECTION( &global_ps_crit_section);

    /*-----------------------------------------------------------------------
      Make sure that iface is valid and is a logical iface
    -----------------------------------------------------------------------*/
    if (!PS_IFACE_IS_VALID( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Invalid iface 0x%p", ps_iface_ptr, 0, 0);
      break;
    }

    if (!PS_IFACE_IS_LOGICAL( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%d not logical",
                             ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      break;
    }

    /*-----------------------------------------------------------------------
      Make sure that iface is in GOING_DOWN state as it is not valid to call
      tear_down_cmd_f_ptr in any other state
    -----------------------------------------------------------------------*/
    if (IFACE_GOING_DOWN != PS_IFACEI_GET_STATE( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%p in invalid state 0x%p",
                             ps_iface_ptr->name,
                             ps_iface_ptr->instance,
                             PS_IFACEI_GET_STATE( ps_iface_ptr));
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
      break;
    }

    /*-----------------------------------------------------------------------
      Teardown the TRAT iface if it already exists. This can happen if
      application calls dsnet_stop()
        1. in the middle of a handoff OR
        2. after handoff is completed and if partial context is maintained for
           the previously associated iface
      The arbitration flag is cleared in ps_iface_down_ind_ex()
    -----------------------------------------------------------------------*/
    trat_iface_ptr = PS_IFACE_GET_TRAT_IFACE( ps_iface_ptr);
    if (NULL != trat_iface_ptr && assoc_iface_ptr != trat_iface_ptr)
    {
      if ( PS_IFACEI_IS_IN_ARBITRATION( ps_iface_ptr ) )
      {
        LOG_MSG_INFO2( "In arb, set trat flag, call arb_tear_down on 0x%x:%d",
                       trat_iface_ptr->name,
                       trat_iface_ptr->instance, 0 );

        PS_IFACEI_SET_ARBITRATION_IN_PROGRESS(trat_iface_ptr);

        ret_val = ps_iface_arbitration_tear_down_cmd( trat_iface_ptr,
                                                     &ps_errno,
                                                      client_data_ptr );
      }
      else
      {
        trat_iface_app_priority = ps_iface_get_app_priority(trat_iface_ptr);
        if (PS_POLICY_MGR_PRIORITY_INVALID == trat_iface_app_priority)
        {
          LOG_MSG_INFO1( "Not calling tear_down_cmd_ex on 0x%x:%d as "
                         "iface priority is PS_POLICY_MGR_PRIORITY_INVALID",
                         trat_iface_ptr->name, trat_iface_ptr->instance, 0);
        }
        else
        {
          LOG_MSG_INFO2( "Calling tear_down_cmd_ex on 0x%x:%d, with pri 0x%x",
                         trat_iface_ptr->name,
                         trat_iface_ptr->instance,
                         trat_iface_app_priority);

          ret_val = ps_iface_tear_down_cmd_ex( trat_iface_ptr,
                                               trat_iface_app_priority,
                                               &ps_errno,
                                               client_data_ptr);

          if (-1 == ret_val && DS_EWOULDBLOCK != ps_errno)
          {
            LOG_MSG_INFO3( "ps_iface_tear_down_cmd_ex failed on iface 0x%x:%d "
                           "err %d",
                           trat_iface_ptr->name,
                           trat_iface_ptr->instance,
                           ps_errno);
            break;
          }
        }
      }
    } /* trat not NULL check */

    /*-----------------------------------------------------------------------
      Tear down associated iface. Check if arbitration is in progress:
      (yes) do ps_iface_go_null_cmd()/ps_iface_arb_tear_down_cmd()
      (no)  do ps_iface_tear_down_cmd_ex()
      The arbitration flag is cleared in ps_iface_down_ind_ex()
    -----------------------------------------------------------------------*/
    if ( PS_IFACEI_IS_IN_ARBITRATION( ps_iface_ptr ) )
    {
      LOG_MSG_INFO2( "In arb, set flag assoc, call arb_tear_down on 0x%x:%d",
                     assoc_iface_ptr->name,
                     assoc_iface_ptr->instance, 0 );

      PS_IFACEI_SET_ARBITRATION_IN_PROGRESS(assoc_iface_ptr);

      ret_val = ps_iface_arbitration_tear_down_cmd( assoc_iface_ptr,
                                                   &ps_errno,
                                                    client_data_ptr );
    }
    else if ( PS_IFACEI_GET_STATE(assoc_iface_ptr) == IFACE_DOWN )
    {
      LOG_MSG_FUNCTION_EXIT( "Success, iface 0x%x:%d in DOWN state",
                             ps_iface_ptr->name, ps_iface_ptr->instance, 0);

      PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
      return 0;
    }
    else
    {
      assoc_iface_app_priority = ps_iface_get_app_priority(assoc_iface_ptr);

      LOG_MSG_INFO2( "Calling tear_down_cmd_ex on 0x%x:%d, with pri 0x%x",
                     assoc_iface_ptr->name,
                     assoc_iface_ptr->instance,
                     assoc_iface_app_priority);

      ret_val = ps_iface_tear_down_cmd_ex( assoc_iface_ptr,
                                           assoc_iface_app_priority,
                                          &ps_errno,
                                           client_data_ptr);
    }

    if (-1 == ret_val && DS_EWOULDBLOCK != ps_errno)
    {
      LOG_MSG_INFO3( "ps_iface_tear_down_cmd failed on iface 0x%x:%d err %d",
                     assoc_iface_ptr->name,
                     assoc_iface_ptr->instance,
                     ps_errno);
      break;
    }
    else if (0 == ret_val && TRUE == PS_IFACE_GET_IS_ACTIVE(assoc_iface_ptr))
    {
      /*---------------------------------------------------------------------
        Clean up the logical iface's state as if IFACE_DOWN_EV is received.

        As ACTIVE ifaces doesn't go to IFACE_DOWN state when the last
        application exits, IFACE_DOWN_EV is not posted. So logical iface
        can't rely on IFACE_DOWN_EV to clean up.

        ret_val can be 0 for PASSIVE ifaces in a race condition where
        an application calls dsnet_stop as soon as network terminated a call
        and the MH posted ps_iface_down_ind(). In this case, logical iface
        framework receives IFACE_DOWN_EV and uses it as a trigger to clean
        itself. If a distinction is not made between
        ACTIVE and PASSIVE ifaces, logical iface framework will try to clean
        itself twice for PASSIVE ifaces which may lead to failures.
      ---------------------------------------------------------------------*/

      /*---------------------------------------------------------------------
        Clean up all secondary flows otherwise ps_iface_down_ind_ex() will
        assert. Even if the client called QOS_RELEASE ioctl on all flows prior
        to calling ps_iface_tear_down_cmd(), physical iface may take a while
        to do OTA signalling and to clean up QoS. Hence, logical iface can't
        rely on physical iface to clean up QoS in this case
      ---------------------------------------------------------------------*/
      ps_flow_ptr =
        (ps_flow_type *)
          list_peek_front( &( ps_iface_ptr->iface_private.flow.sec_flow_list));

      while ( NULL != ps_flow_ptr && PS_FLOW_IS_VALID(ps_flow_ptr) )
      {
        /*-------------------------------------------------------------------
          Fetch the next ps_flow in list before ps_flow is deleted from the
          list
        -------------------------------------------------------------------*/
        tmp_ps_flow_ptr =
          (ps_flow_type *)
            list_peek_next( &( ps_iface_ptr->iface_private.flow.sec_flow_list),
                            &( ps_flow_ptr->link));

        /*-------------------------------------------------------------------
          Delete ps_flow from the list
        -------------------------------------------------------------------*/
        ps_flow_go_null_ind( ps_flow_ptr, PS_EIC_NOT_SPECIFIED);
        (void) ps_iface_delete_flow( ps_iface_ptr, ps_flow_ptr, &ps_errno);

        ps_flow_ptr = tmp_ps_flow_ptr;
      }

      ps_iface_logicali_process_iface_down_ev( ps_iface_ptr,
                                               PS_NET_DOWN_REASON_CLIENT_END);
    }

    LOG_MSG_FUNCTION_EXIT( "Success, iface 0x%x:%d",
                           ps_iface_ptr->name, ps_iface_ptr->instance, 0);

    PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
    return 0;

  } while (0);

  /*-------------------------------------------------------------------------
    Common error handling
  -------------------------------------------------------------------------*/
  LOG_MSG_FUNCTION_EXIT( "Fail, iface 0x%x:%d",
                         ps_iface_ptr->name, ps_iface_ptr->instance, 0);

  PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
  return -1;

} /* ps_iface_logical_default_tear_down_cmd_hdlr_ex() */

int32 ps_iface_logical_default_tear_down_cmd_hdlr
(
  ps_iface_type  * ps_iface_ptr,
  void           * client_data_ptr
)
{
  LOG_MSG_INFO2( "calling ex version of tear down hdlr", 0, 0, 0 );
  return ps_iface_logical_default_tear_down_cmd_ex_hdlr( ps_iface_ptr,
                                                         PS_POLICY_MGR_LEGACY_PRIORITY,
                                                         client_data_ptr );
} /* ps_iface_logical_default_tear_down_cmd_hdlr() */

int32 ps_iface_logical_swap_rat
(
  ps_iface_type  * ps_iface_ptr,
  int16          * ps_errno_ptr
)
{
  ps_iface_type      * assoc_iface_ptr;
  ps_flow_type       * default_flow_ptr;
  ps_flow_type       * assoc_default_flow_ptr;
  ps_meta_info_type  * meta_info_ptr = NULL;
  ps_meta_info_type  * old_meta_info_ptr;
  int32                ret_val;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Iface 0x%p", ps_iface_ptr, 0, 0);

  do
  {
    PS_ENTER_CRIT_SECTION( &global_ps_crit_section);

    /*-----------------------------------------------------------------------
      Make sure that iface is valid and is a logical iface
    -----------------------------------------------------------------------*/
    if (!PS_IFACE_IS_VALID( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Invalid iface 0x%p", ps_iface_ptr, 0, 0);
      break;
    }

    if (!PS_IFACE_IS_LOGICAL( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%d not logical",
                             ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      break;
    }

    /*-----------------------------------------------------------------------
      Make sure that iface is associated to another iface
    -----------------------------------------------------------------------*/
    if (NULL == PS_IFACE_GET_ASSOC_IFACE( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%p not associated",
                             ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      break;
    }

    /*-----------------------------------------------------------------------
      Make sure that iface is in UP/COMING_UP state as it is not valid to call
      this function in any other state
    -----------------------------------------------------------------------*/
    if ( IFACE_COMING_UP != PS_IFACEI_GET_STATE( ps_iface_ptr) &&
         IFACE_UP != PS_IFACEI_GET_STATE( ps_iface_ptr) &&
         IFACE_ROUTEABLE != PS_IFACEI_GET_STATE( ps_iface_ptr) )
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%p in invalid state 0x%p",
                             ps_iface_ptr->name,
                             ps_iface_ptr->instance,
                             PS_IFACEI_GET_STATE( ps_iface_ptr));
      break;
    }

    /*-----------------------------------------------------------------------
      De-register for UP/DOWN events on associated iface
    -----------------------------------------------------------------------*/
    (void) ps_iface_logicali_dereg_iface_ev( ps_iface_ptr, ps_errno_ptr);

    /*-----------------------------------------------------------------------
      Swap SRAT and TRAT
    -----------------------------------------------------------------------*/
    PS_IFACE_SET_ASSOC_IFACE( ps_iface_ptr,
                              PS_IFACE_GET_TRAT_IFACE( ps_iface_ptr));
    PS_IFACE_SET_TRAT_IFACE( ps_iface_ptr, NULL);

    /*-----------------------------------------------------------------------
      Register for all events on associated iface
    -----------------------------------------------------------------------*/
    assoc_iface_ptr = PS_IFACE_GET_ASSOC_IFACE( ps_iface_ptr);
    if (-1 == ps_iface_logicali_reg_iface_all_ev( ps_iface_ptr, ps_errno_ptr))
    {
      LOG_MSG_INFO3_6( "Iface 0x%x:%d couldn't reg for IFACE_UP_EV "
                       "on iface 0x%x:%d",
                       ps_iface_ptr->name, ps_iface_ptr->instance,
                       assoc_iface_ptr->name, assoc_iface_ptr->instance, 0, 0);
      break;
    }

    /*-----------------------------------------------------------------------
      Set meta info in default flow
    -----------------------------------------------------------------------*/
    PS_TX_META_INFO_GET_ALL( meta_info_ptr);
    if (NULL == meta_info_ptr ||
        NULL == PS_TX_META_GET_RT_META_INFO_PTR( meta_info_ptr))
    {
      LOG_MSG_ERROR( "Couldn't alloc meta info", 0, 0, 0);
      break;
    }

    PS_TX_META_SET_ROUTING_CACHE( meta_info_ptr, assoc_iface_ptr);

    default_flow_ptr =  PS_IFACE_GET_DEFAULT_FLOW( ps_iface_ptr);
    old_meta_info_ptr = PS_FLOW_GET_META_INFO_FROM_FLOW( default_flow_ptr);
    PS_TX_META_INFO_FREE( &old_meta_info_ptr);

    PS_FLOW_SET_META_INFO_IN_FLOW( default_flow_ptr, meta_info_ptr);

    /*-----------------------------------------------------------------------
      Associate the default flow of the iface with the default flow of the
      associated iface

      TODO: ps_flowi_assoc_flow_ev_cback_dereg should be moved inside
            ps_flow_set_assoc_flow
    -----------------------------------------------------------------------*/
    assoc_default_flow_ptr = PS_FLOWI_GET_ASSOC_PS_FLOW( default_flow_ptr);
    if (NULL == assoc_default_flow_ptr ||
        !PS_FLOW_IS_VALID( assoc_default_flow_ptr) )
    {
      LOG_MSG_ERROR("Invalid assoc flow, 0x%p associated with flow, 0x%p",
                assoc_default_flow_ptr, default_flow_ptr, 0);
    }
    else
    {
      ps_flowi_assoc_flow_ev_cback_dereg(default_flow_ptr);
    }

    ret_val = ps_flow_set_assoc_flow
              (
                default_flow_ptr,
                PS_IFACE_GET_DEFAULT_FLOW(assoc_iface_ptr),
                NULL,
                ps_errno_ptr
              );

    if (0 != ret_val)
    {
      LOG_MSG_INFO3_6( "Iface 0x%x:%d couldn't be associated with the default"
                       " flow on iface 0x%x:%d",
                       ps_iface_ptr->name, ps_iface_ptr->instance,
                       assoc_iface_ptr->name, assoc_iface_ptr->instance, 0, 0);
      break;
    }
    /*-----------------------------------------------------------------------
      Create logical flows for all network initiated QoS flows on the
      associated iface
    -----------------------------------------------------------------------*/
    ret_val = ps_iface_logical_flowi_handle_nw_init_qos( ps_iface_ptr);
    if (0 != ret_val)
    {
      LOG_MSG_INFO3( "ps_iface_logical_flowi_handle_nw_init_qos() failed on "
                     "iface 0x%x:%d",
                     ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      break;
    }

    LOG_MSG_FUNCTION_EXIT( "Success, iface 0x%x:%d",
                           ps_iface_ptr->name, ps_iface_ptr->instance, 0);

    PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
    return 0;

  } while (0);

  /*-------------------------------------------------------------------------
    Common error handling
  -------------------------------------------------------------------------*/
  LOG_MSG_FUNCTION_EXIT( "Fail, iface 0x%x:%d",
                         ps_iface_ptr->name, ps_iface_ptr->instance, 0);

  PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
  return -1;

} /* ps_iface_logical_swap_rat() */

#endif /* FEATURE_DATA_PS  */
