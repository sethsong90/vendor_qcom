/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                            P S _ I P 6 _ S M . C

DESCRIPTION
  Implementation of the IPv6 state machine.  This state machine is
  responsible for soliciting for Router Advertisements.  When a valid RA is
  received it will be in the Net Up state - which means that the IP interface
  it is associated with is in the up state.

  It provides a mechanism for a client to register for either UP or down
  indications - this will inform the client (or owner) that it has entered
  either of these states.

DEPENDENCIES
  The module MUST execute in the PS context.

EXTERNALIZED FUNCTIONS
  ip6_sm_create()
    Creates an instance of the state machine and associates it with a given
    PS Iface instance.
  ip6_sm_post_event()
    Used to post an event to the state machine - may cause state
    transitions.
    NOTE:  If this is called OUTSIDE of the PS context it will send a command
    to PS, otherwise it will execute in-line.
  ip6_sm_reg_ind()
    Used to register for the UP/DOWN indications.
  ip6_sm_is_ipv6_priv_ext_enabled()
    Access method for ipv6_priv_ext_enabled flag
  ip6_sm_get_priv_ext_lifetimes()
    Access method for priv_ext_lifetimes structure

Copyright (c) 2003-2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                            EDIT HISTORY FOR FILE

  $Header: //source/qcom/qct/modem/datamodem/protocols/inet/rel/11.03/src/ps_ip6_sm.c#1 $
  $Author: zhasan $ $DateTime: 2011/06/17 12:02:33 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
03/21/11    op     Data SU API cleanup
02/24/11    ash    Cleaning up global variables for Q6 free floating effort.
01/10/11    ss     Cleaning up of Globals for thread safety in Q6 Free 
                   Floating environment.
10/01/10    sa     Added appropriate log messages before ASSERT(0).
09/21/10    ash    Removed backoff algorithm for RA re-solicitaion
09/09/10    ssh    Avoid sending item with zero used bytes 
08/11/10    ash    Changed ra_resol_time to 
                   75% of MIN(router_lifetime,valid_lifetime)
10/12/09    ssh    Reporting state machine events and transitions to diag
06/24/09    am     Renamed hton/ntoh macros with ps_ prefix.
06/22/09    ssh    Fixed compiler warning.
05/22/09    kk     Q6 compiler warning fixes.
03/26/09    pp     CMI De-featurization.
01/18/07    mct    Fix to only tear down the call if the prefix goes away in
                   IPv6 SM UP state.
05/07/07    mct    IPv6 Neighbor Discovery
04/26/07    mct    Fixed issue wherein failure to receive RA after prefix
                   deprecateion causes call to end. Resetting timers and
                   resol attempts properly.
11/02/06    mct    Added support for RFC3041: IPv6 Privacy Extensions.
09/12/06    msr    Removed redundant state field in event_info structure
04/19/06    mct    Fixed crash when meta_info is not set during RS send.
                   Fixed initialization of initial # of RS solicitations.
04/19/06    rt     Removed ip_sec_reqd field from IP Control Block.
02/22/06    msr    Using single critical section
02/06/06    mct    Updated for L4 tasklock/crit sections.
04/18/05    ks     Added registration for PHYS_LINK_GONE_EV and change due to
                   PHYS_LINK_NULL state.
08/12/04    mvl    Fixed the RA resol algorythm and added much messaging.
08/02/04    mct    Fixed bug where events being registered before sm instance
                   and the down event cback executed w/ an invalid instance.
07/30/04    ifk    Fixed bugs to make SM work.
07/12/04    mvl    Updates to the state machine to reflect design changes:
                   specifically the addition of a new state which allows the
                   differentiation of resolicitation for RAs and Prefixes.
06/21/04    mct    Updated ip6 state machine to take RS configuration params
                   from NV instead of predefined values.
06/11/04    vp     Replaced byte ordering functions with their PS versions.
04/23/04    rc     Fixed multiple issues with sending out router solitcations
04/16/04    rc     Fixed bug where retry count was being incremented instead
                   of being decremented. Fixed bug where an incorrect
                   transition to up state instead of down state was made if
                   the retry count was less than 0.
03/02/04    rc     Added function ip6_smi_send_rs() to send Router
                   Solicitations.
01/12/04    mct    Fixed sm, down call was actually changing state to up.
01/08/04    mvl    Added some messages.
12/23/03    mvl    Created module
===========================================================================*/

/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"
#if defined (FEATURE_DATA_PS) && defined (FEATURE_DATA_PS_IPV6)
#include "pstimer.h"
#include "ps_iface.h"
#include "ps_crit_sect.h"
#include "ps_ip6_sm.h"
#include "ps_ip6i_sm.h"
#include "ps_svc.h"
#include "rex.h"
#include "task.h"
#include "ps_icmp6_msg.h"
#include "ps_icmp6.h"
#include "ps_icmp.h"
#include "ps_icmp6_nd.h"
#include "ps_utils.h"
#include "ps_ip6_events.h"
#include "ds_Utils_DebugMsg.h"
#include "ps_system_heap.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
                            FORWARD DECLARATIONS
=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void ip6_smi_transition_state
(
  ip6_sm_type       *instance,
  ip6_smi_state_type new_state
);

int ip6_smi_send_rs
(
  ip6_sm_type        *instance
);

void ip6_smi_ps_cmd_handler
(
  ps_cmd_enum_type cmd,
  void *user_data_ptr
);

void ip6_smi_rs_timer_handler
(
  void *instance
);

void ip6_smi_ra_timer_handler
(
  void *instance
);

void ip6_smi_prefix_update_cback
(
  ps_iface_type             *this_iface_ptr,
  ps_iface_event_enum_type   event,
  ps_iface_event_info_u_type event_info,
  void                      *user_data_ptr
);

void ip6_smi_phys_link_cback
(
  ps_iface_type             *this_iface_ptr,
  ps_iface_event_enum_type   event,
  ps_iface_event_info_u_type event_info,
  void                      *user_data_ptr
);

void ip6_smi_valid_ra_cback
(
  ps_iface_type             *this_iface_ptr,
  ps_iface_event_enum_type   event,
  ps_iface_event_info_u_type event_info,
  void                      *user_data_ptr
);

/*===========================================================================

                         INTERNAL DATA DECLARATIONS

===========================================================================*/
#ifndef T_ARM
/*---------------------------------------------------------------------------
  String names for states and events - used off target
---------------------------------------------------------------------------*/
static char *ip6_smi_state_names[IP6_MAX_STATE] =
{
  "Down",
  "Waiting to Solicit",
  "Soliciting",
  "Up",
  "Re-Soliciting for RA",
  "Re-Soliciting for Prefix"
};

static char *ip6_smi_event_names[IP6_SM_MAX_EV] =
{
  "Start",
  "RS Timer",
  "RA Timer",
  "Valid RA",
  "Stop"
};
#endif

/*---------------------------------------------------------------------------
  The pointer for the task in which the state machine will execute.
---------------------------------------------------------------------------*/
static rex_tcb_type *this_task_cb_ptr = NULL;

/*---------------------------------------------------------------------------
  The IPv6 Privacy Extensions NV item status
---------------------------------------------------------------------------*/
static boolean ipv6_priv_ext_enabled;

static priv_ext_lifetimes_type priv_ext_lifetimes;

/*---------------------------------------------------------------------------
MACRO IP6_SMI_IS_VALID

DESCRIPTION
  Make sure the SM instance passed in is valid
---------------------------------------------------------------------------*/
#define IP6_SMI_IS_VALID( instance )                                    \
  (((instance) != NULL) && ((instance->this_sm_ptr) == instance))


/*---------------------------------------------------------------------------
DEFINE IP6_RESOL_BACKOFF_MULT

DESCRIPTION
  the multiplier to handle the geometric backoff for re-solicitation.
---------------------------------------------------------------------------*/
#define IP6_RESOL_BACKOFF_MULT 2

/*---------------------------------------------------------------------------
DEFINE IP6_RESOL_MAX_BACKOFF

DESCRIPTION
  The maximum amount of time to back off - 32 seconds.
---------------------------------------------------------------------------*/
#define IP6_RESOL_MAX_BACKOFF 32000


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                             EXTERNAL FUNCTIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
/*===========================================================================
FUNCTION IP6_SM_POWERUP_INIT()

DESCRIPTION
  This function associates the SM with the task that initializes it, and
  registers the command handling functions.

PARAMETERS
  None

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ip6_sm_powerup_init
(
  void
)
{
  uint8 loop;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ASSERT(rex_self() == &ps_tcb);
  this_task_cb_ptr = &ps_tcb;

  /*-------------------------------------------------------------------------
    Register the command handlers for all of this state machine's related
    commands.
  -------------------------------------------------------------------------*/
  for(loop = (uint8)PS_IP6_SM_BASE_CMD; loop <= (uint8)PS_IP6_SM_MAX_CMD; loop +=1)
  {
    (void)ps_set_cmd_handler((ps_cmd_enum_type)loop, ip6_smi_ps_cmd_handler);
  }
} /* ps_ip6_sm_powerup_init() */


/*===========================================================================
FUNCTION IP6_SM_INIT()

DESCRIPTION
  This function performs NV related initializations of IP6 state machine.

PARAMETERS
  None

RETURN VALUE
  None

DEPENDENCIES
  This function is called after signalling PS task start.

SIDE EFFECTS
  None
===========================================================================*/
void ip6_sm_init
(
  void
)
{
  nv_item_type         *ps_nv_item_ptr;
  nd_config_items_type *nd_config_items_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Allocate temporary memory for the NV item
  -------------------------------------------------------------------------*/
  ps_nv_item_ptr = (nv_item_type *)
    ps_system_heap_mem_alloc(sizeof(nv_item_type));
  if( ps_nv_item_ptr == NULL )
  {
    LOG_MSG_FATAL_ERROR("Mem alloc from system heap failed.", 0, 0, 0 );
    ASSERT(0);
  }

  /*-------------------------------------------------------------------------
    Retrieve the value of IPV6 PRIVACY EXTENSIONS ENABLED FLAG and the
    Privacy Extensions timer settings.
  -------------------------------------------------------------------------*/
  if (NV_DONE_S !=
        ps_get_nv_item(NV_IPV6_PRIVACY_EXTENSIONS_ENABLED_I, ps_nv_item_ptr))
  {
    /*-----------------------------------------------------------------------
      Use default value of enabled.
    -----------------------------------------------------------------------*/
    ipv6_priv_ext_enabled = TRUE;
  }
  else
  {
    ipv6_priv_ext_enabled = ps_nv_item_ptr->ipv6_privacy_extensions_enabled;
  }

  if(ipv6_priv_ext_enabled)
  {
    if (NV_DONE_S !=
          ps_get_nv_item(NV_IPV6_PRIVATE_ADDRESS_CONFIG_I, ps_nv_item_ptr))
    {
      /*-----------------------------------------------------------------------
        Use default values.
      -----------------------------------------------------------------------*/
      priv_ext_lifetimes.pref_lifetime_timer =
        DEFAULT_IPV6_PRIV_IID_PREF_LIFETIME;
      priv_ext_lifetimes.valid_lifetime_timer =
        DEFAULT_IPV6_PRIV_IID_VALID_LIFETIME;
    }
    else
    {
      priv_ext_lifetimes.pref_lifetime_timer =
        ps_nv_item_ptr->ipv6_private_address_config.ipv6_priv_preferred_lifetime;
      priv_ext_lifetimes.valid_lifetime_timer =
        ps_nv_item_ptr->ipv6_private_address_config.ipv6_priv_valid_lifetime;
    }
  }

  nd_config_items_ptr = ps_icmp6_get_nd_config_items();

  /*-------------------------------------------------------------------------
    Retrieve the settings for IPv6 Neighbor Discovery.
  -------------------------------------------------------------------------*/
  if (NV_DONE_S != ps_get_nv_item(NV_ICMP6_ND_CONFIG_I, ps_nv_item_ptr))
  {
    /*-----------------------------------------------------------------------
      Use default value of enabled.
    -----------------------------------------------------------------------*/
    nd_config_items_ptr->max_nd_solicits        = ICMP6_ND_MAX_MULTICAST_SOLICT;
    nd_config_items_ptr->max_nd_retransmits     = ICMP6_ND_MAX_RETRANSMITS;
    nd_config_items_ptr->max_nd_reachable_time  = ICMP6_ND_DEFAULT_REACHABLE_TIME;
    nd_config_items_ptr->max_nd_delay_first_probe_time =
                                             ICMP6_ND_DELAY_FIRST_PROBE_TIME;
    nd_config_items_ptr->max_nd_retransmit_interval =
                                            ICMP6_ND_DEFAULT_RETRANSMIT_TIME;
  }
  else
  {
    nd_config_items_ptr->max_nd_solicits       =
      ps_nv_item_ptr->icmp6_nd_config.max_nd_solicits;
    nd_config_items_ptr->max_nd_retransmits    =
      ps_nv_item_ptr->icmp6_nd_config.max_nd_retransmits;
    nd_config_items_ptr->max_nd_reachable_time =
      ps_nv_item_ptr->icmp6_nd_config.max_nd_reachable_time;
    nd_config_items_ptr->max_nd_delay_first_probe_time =
      ps_nv_item_ptr->icmp6_nd_config.max_nd_delay_first_probe_time;
    nd_config_items_ptr->max_nd_retransmit_interval =
      ps_nv_item_ptr->icmp6_nd_config.max_nd_retransmit_interval;
  }

  PS_SYSTEM_HEAP_MEM_FREE(ps_nv_item_ptr);
} /* ip6_sm_init() */


/*===========================================================================
FUNCTION IP6_SM_CREATE()

DESCRIPTION
  This function creates an instance of the IPv6 state machine using memory
  passed in by the caller.  It will initialize the associated ps_iface to
  refer to it so that events (such as adding a prefix or lifetime expiry) can
  be posted to the SM.

PARAMETERS
  instance: pointer to the memory to be used to allocate the state machine.
  iface_ptr:  pointer to the associated PS Iface instance
  sm_config:  pointer to the ip6 state machine RS configuration

RETURN VALUE
  0 on succuss
 -1 on failure

DEPENDENCIES
  ip6_sm_init() needs to have been called first.

SIDE EFFECTS
  will set the ip6_sm_ptr value in the associated PS Iface pointer to it.
===========================================================================*/
int ip6_sm_create
(
  ip6_sm_type   *instance,
  ps_iface_type *iface_ptr,
  ip6_sm_config_type *sm_config
)
{
  void *iface_event_buf;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Make sure that the module has been initialized.
  -------------------------------------------------------------------------*/
  if(this_task_cb_ptr == NULL ||
     instance == NULL         ||
     iface_ptr == NULL        ||
     sm_config == NULL)
  {
    ASSERT(0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    Make sure we are not re-creating something that has been created.
  -------------------------------------------------------------------------*/
  if(instance->this_sm_ptr == instance)
  {
    return -1;
  }

  /*-------------------------------------------------------------------------
    Initialize static fields.
  -------------------------------------------------------------------------*/
  instance->state                        = IP6_DOWN_STATE;
  instance->ps_iface_ptr                 = iface_ptr;
  instance->params.init_sol_delay        = sm_config->init_sol_delay;
  instance->params.sol_interval          = sm_config->sol_interval;
  instance->params.max_sol_attempts      = sm_config->max_sol_attempts;
  instance->params.resol_interval        = sm_config->resol_interval;
  instance->params.init_resol_interval   = sm_config->resol_interval;
  instance->params.max_resol_attempts    = sm_config->max_resol_attempts;
  instance->tries                        = sm_config->max_sol_attempts;
  instance->params.pre_ra_exp_resol_time =
    sm_config->pre_ra_exp_resol_time * 60000; /* units are minutes */
  instance->params.ra_resol_time         = 0;
  instance->params.remaining_ra_lifetime = 0;
  instance->ind.cback                    = NULL;
  instance->ind.data                     = NULL;
  instance->this_sm_ptr                  = instance;
  instance->ip6_fltr                     = PS_IFACE_IPFLTR_INVALID_HANDLE;
  /*-------------------------------------------------------------------------
    Calculate the max_resol_time -> the maximum amount of time it will take
    for us to resolicit before we give up.

    NOTE: if we hit the maximum backoff time, then we will simply increment
    the max time by the max backoff value.
  -------------------------------------------------------------------------*/
  instance->params.max_resol_time = 
                   sm_config->resol_interval * sm_config->max_resol_attempts;
  MSG_4(
    MSG_SSID_DS,
    MSG_LEGACY_MED,
    "ip6_sm(0x%p) max_resol_time(interval: %d, tries: %d)=%d",
    instance,
    sm_config->resol_interval,
    sm_config->max_resol_attempts,
    instance->params.max_resol_time);

  /*-------------------------------------------------------------------------
    Allocate timers: RS timer, RA timer
  -------------------------------------------------------------------------*/
  instance->rs_timer = ps_timer_alloc(ip6_smi_rs_timer_handler, instance);
  if(instance->rs_timer == PS_TIMER_INVALID_HANDLE)
  {
    return -1;
  }

  instance->ra_timer = ps_timer_alloc(ip6_smi_ra_timer_handler, instance);
  if(instance->ra_timer == PS_TIMER_INVALID_HANDLE)
  {
    return -1;
  }

  /*-------------------------------------------------------------------------
    Allocate ps_iface buffers and register for:
      the prefix update event
  -------------------------------------------------------------------------*/
  iface_event_buf =
    ps_iface_alloc_event_cback_buf(ip6_smi_prefix_update_cback, instance);
  if(iface_event_buf == NULL)
  {
    ASSERT(0);
    return -1;
  }

  if(ps_iface_event_cback_reg(iface_ptr,
                              IFACE_PREFIX_UPDATE_EV,
                              iface_event_buf) < 0)
  {
    ASSERT(0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    the Valid RA event
  -------------------------------------------------------------------------*/
  iface_event_buf =
    ps_iface_alloc_event_cback_buf(ip6_smi_valid_ra_cback, instance);
  if(iface_event_buf == NULL)
  {
    ASSERT(0);
    return -1;
  }

  if(ps_iface_event_cback_reg(iface_ptr,
                              IFACE_VALID_RA_EV,
                              iface_event_buf) < 0)
  {
    ASSERT(0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    phys link down
  -------------------------------------------------------------------------*/
  iface_event_buf =
    ps_iface_alloc_event_cback_buf(ip6_smi_phys_link_cback, instance);
  if(iface_event_buf == NULL)
  {
    ASSERT(0);
    return -1;
  }

  if(ps_iface_event_cback_reg(iface_ptr,
                              IFACE_PHYS_LINK_DOWN_EV,
                              iface_event_buf) < 0)
  {
    ASSERT(0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    phys link GONE
  -------------------------------------------------------------------------*/
  iface_event_buf =
    ps_iface_alloc_event_cback_buf(ip6_smi_phys_link_cback, instance);
  if(iface_event_buf == NULL)
  {
    ASSERT(0);
    return -1;
  }

  if(ps_iface_event_cback_reg(iface_ptr,
                              IFACE_PHYS_LINK_GONE_EV,
                              iface_event_buf) < 0)
  {
    ASSERT(0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    phys link up
  -------------------------------------------------------------------------*/
  iface_event_buf =
    ps_iface_alloc_event_cback_buf(ip6_smi_phys_link_cback, instance);
  if(iface_event_buf == NULL)
  {
    ASSERT(0);
    return -1;
  }

  if(ps_iface_event_cback_reg(iface_ptr,
                              IFACE_PHYS_LINK_UP_EV,
                              iface_event_buf) < 0)
  {
    ASSERT(0);
    return -1;
  }

  return 0;

} /* ip6_sm_create() */



/*===========================================================================
FUNCTION IP6_SM_POST_EVENT()

DESCRIPTION
  This function is used to post an event to the state machine passed in.
  NOTE: if the event is posted outside PS context a command will be sent to
  PS to process that event, otherwise this will happen in line.

PARAMETERS
  instance: the instance to which the event is being posted.
  event: the event being posted

RETURN VALUE
  0 on success
 -1 on failure

DEPENDENCIES
  ip6_sm_create() needs to have been called first.

SIDE EFFECTS
  None
===========================================================================*/
int ip6_sm_post_event
(
  ip6_sm_type      *instance,
  ip6_sm_event_type event
)
{
  ps_timer_error_type error_ret;
  ps_cmd_enum_type cmd_id;
  ip6_sm_event_payload_type sm_event_payload;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  ASSERT(event < IP6_SM_MAX_EV);

  if( !IP6_SMI_IS_VALID(instance) )
  {
    LOG_MSG_ERROR("Tried posting to invalid IP6 instance %x", instance,0,0);
    return -1;
  }

  LOG_MSG_INFO2("Posting IP6 SM event %d in state %d", event, instance->state, 0);
  
  /*-------------------------------------------------------------------------
    Report the event to diag
  -------------------------------------------------------------------------*/
  memset(&sm_event_payload, 0, sizeof(ip6_sm_event_payload_type));
  sm_event_payload.iface_name = instance->ps_iface_ptr->name;
  sm_event_payload.iface_instance = instance->ps_iface_ptr->instance;
  sm_event_payload.event = event;
  event_report_payload( EVENT_IPV6_SM_EVENT,
                        sizeof( sm_event_payload ),
                        &sm_event_payload );

  /*-------------------------------------------------------------------------
    check if not running in initializing task context - if not send the
    command corresponding to the event being posted to PS
  -------------------------------------------------------------------------*/
  if(this_task_cb_ptr != rex_self())
  {
    LOG_MSG_INFO2("Sending PS cmd for IP6 SM event %d", event, 0, 0);
    switch(event)
    {
      case IP6_SM_START_EV:
        cmd_id = PS_IP6_SM_START_CMD;
        break;

      case IP6_SM_RS_TIMER_EV:
        cmd_id = PS_IP6_SM_RS_TIMER_CMD;
        break;

      case IP6_SM_RA_TIMER_EV:
        cmd_id = PS_IP6_SM_RA_TIMER_CMD;
        break;

      case IP6_SM_VALID_RA_EV:
        cmd_id = PS_IP6_SM_VALID_RA_CMD;
        break;

      case IP6_SM_LINK_CHANGED_EV:
        cmd_id = PS_IP6_SM_LINK_CHANGED_CMD;
        break;

      case IP6_SM_STOP_EV:
        cmd_id = PS_IP6_SM_STOP_CMD;
        break;

      default:
        /* unexpected! */
        LOG_MSG_ERROR("Unexpected event 0x%x posted", event, 0, 0);
        ASSERT(0);
        return -1;
    } /* switch(event) */

    ps_send_cmd(cmd_id, instance);
    return 0;
  }

  LOG_MSG_INFO2("Processing IP6 SM event %d", event, 0, 0);

  /*-------------------------------------------------------------------------
    At this point we know we are running in PS with a valid instance.
    Process the event.
  -------------------------------------------------------------------------*/
  switch(event)
  {
  case IP6_SM_START_EV:
    /*-----------------------------------------------------------------------
                                 START Event
    -----------------------------------------------------------------------*/
    switch(instance->state)
    {
    case IP6_DOWN_STATE:
      /*---------------------------------------------------------
                                DOWN

        Transition to the Waiting to Solicit state
      ---------------------------------------------------------*/
      ip6_smi_transition_state(instance, IP6_SOL_WAIT_STATE);
      break;

      /*---------------------------------------------------------
                              SOL_WAIT
                                SOL
                                 UP
                        RE_SOL_FOR_PREFIX
                          RE_SOL_FOR_RA
      ---------------------------------------------------------*/
    case IP6_SOL_WAIT_STATE:
    case IP6_SOL_STATE:
    case IP6_UP_STATE:
    case IP6_RESOL_FOR_PREFIX_STATE:
    case IP6_RESOL_FOR_RA_STATE:
    default:
      /*---------------------------------------------------------
        This event is ignored in all other states.
      ---------------------------------------------------------*/
      break;
    } /* switch(state) */
    break; /* START */


  case IP6_SM_RS_TIMER_EV:
    /*-----------------------------------------------------------------------
                               RS_TIMER Event

      NOTE: the order is intended to reflect the likelyhood of being in a
        particular state, in order to try and optimixet the switch().
    -----------------------------------------------------------------------*/
    switch(instance->state)
    {
    case IP6_RESOL_FOR_PREFIX_STATE:
      /*---------------------------------------------------------
                        RESOL_FOR_PREFIX

        If the number of tries has fallen below zero then just
        transition to UP state, otherwise return to the
        RE_SOL state.
      ---------------------------------------------------------*/
      if(instance->tries <= 0)
      {
        ip6_smi_transition_state(instance, IP6_UP_STATE);
      }
      else
      {
        ip6_smi_transition_state(instance, IP6_RESOL_FOR_PREFIX_STATE);
      }
      break;

    case IP6_RESOL_FOR_RA_STATE:
      /*---------------------------------------------------------
                          RESOL_FOR_RA

        Always transition back to the resol_for_ra state.
      ---------------------------------------------------------*/
      if(instance->tries > 0)
      {
        ip6_smi_transition_state(instance, IP6_RESOL_FOR_RA_STATE);
      }
      break;

    case IP6_SOL_STATE:
      /*---------------------------------------------------------
                                 SOL

        If the number of reties has fallen below zero then
        transition to the down state, otherwise the processing
        is the same as in the SOL_WAIT state - which is to
        transition to the Sol state.
      ---------------------------------------------------------*/
      if(instance->tries <= 0)
      {
        ip6_smi_transition_state(instance, IP6_DOWN_STATE);
        break;
      }
      /* fall-thru */

    case IP6_SOL_WAIT_STATE:
      /*---------------------------------------------------------
                              SOL_WAIT

        Reset the number of tries to the max solicitation
        attempts, and transition to the Soliciting state.
      ---------------------------------------------------------*/
      ip6_smi_transition_state(instance, IP6_SOL_STATE);
      break;

    case IP6_UP_STATE:
      /*---------------------------------------------------------
                                 UP

        Trnsition to the Re-Soliciting state.
      ---------------------------------------------------------*/
      instance->tries = instance->params.max_resol_attempts;
      ip6_smi_transition_state(instance, IP6_RESOL_FOR_PREFIX_STATE);
      break;

    case IP6_DOWN_STATE:
      /*---------------------------------------------------------
                                DOWN

        This event should NEVER happen in this state - however
        a race condition exists - so throw an error, but don't
        fail anything.
      ---------------------------------------------------------*/
      LOG_MSG_ERROR("RS Timer exipired in %d state", instance->state, 0, 0);
      break;

    default:
      break;

    } /* switch(state) */
    break; /* RS_TIMER */


  case IP6_SM_RA_TIMER_EV:
    /*-----------------------------------------------------------------------
                               RA_TIMER Event
    -----------------------------------------------------------------------*/
    switch(instance->state)
    {
    case IP6_UP_STATE:
    case IP6_RESOL_FOR_PREFIX_STATE:
      /*---------------------------------------------------------
                                     UP
                              RESOL_FOR_PREFIX

        In either of these states reset the tries count start the
        RA timer for the remaining time and transition to
        resol_for_ra.
      ---------------------------------------------------------*/
      instance->tries = instance->params.max_resol_attempts;
      LOG_MSG_INFO2("Starting RA timer(%d) for remaining RA lifetime %dms",
              instance->ra_timer,
              instance->params.remaining_ra_lifetime, 0);
      error_ret = ps_timer_start(instance->ra_timer,
                                 instance->params.remaining_ra_lifetime);
      if(PS_TIMER_SUCCESS != error_ret)
      {
        LOG_MSG_ERROR("RA lifetime timer start failure!",0,0,0);
        ASSERT(0);
        return -1;
      }
      ip6_smi_transition_state(instance, IP6_RESOL_FOR_RA_STATE);
      /*fall thru */

     case IP6_DOWN_STATE:
     case IP6_SOL_WAIT_STATE:
     case IP6_SOL_STATE:
       /*---------------------------------------------------------
         ignore this event in all other states.
       ---------------------------------------------------------*/
    break;

    case IP6_RESOL_FOR_RA_STATE:
      /*----------------------------------------------------------
        In this state the total lifetime of the RA has expired, so
        we give up (no router) and transition to the stop state.
      ----------------------------------------------------------*/
      LOG_MSG_INFO1("RA Timer expired - no longer have valid RA, closing...",
               0, 0, 0);
      ip6_smi_transition_state(instance, IP6_DOWN_STATE);
      break;

    default:
      break;

    } /* switch(state) */
    break; /* RA_TIMER */


  case IP6_SM_VALID_RA_EV:
    /*-----------------------------------------------------------------------
                              VALID_RA
    -----------------------------------------------------------------------*/
    switch(instance->state)
    {
    case IP6_UP_STATE:
      /*---------------------------------------------------------
                                 UP

        Loop back to the UP event which will restart the RA timer
      ---------------------------------------------------------*/
      ip6_smi_transition_state(instance, IP6_UP_STATE);
      /* fall through */

    case IP6_DOWN_STATE:
      /*---------------------------------------------------------
                              DOWN

        This event is ignored in down state as no action needs
       	to be taken.
      ---------------------------------------------------------*/
      break;

    case IP6_SOL_WAIT_STATE:
    case IP6_SOL_STATE:
      /*---------------------------------------------------------
                              SOL_WAIT
                                 SOL

        In both of thses states the Up indication needs to be
        posted - do this in a task lock to make sure that no one
        else gets to run until the state has been changed.  The
        subsequent processing is the same as the Re-Sol state
        (i.e. go up).
      ---------------------------------------------------------*/
      if(instance->ind.cback != NULL)
      {
        PS_ENTER_CRIT_SECTION(&global_ps_crit_section);
        instance->ind.cback(instance, IP6_SM_UP_IND, instance->ind.data);
        PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      }
      /* fall-thru */

    case IP6_RESOL_FOR_PREFIX_STATE:
    case IP6_RESOL_FOR_RA_STATE:
      /*---------------------------------------------------------
                        RESOL_FOR_PREFIX
                          RE_SOL_FOR_RA

        Transition to the Up state.
      ---------------------------------------------------------*/
      ip6_smi_transition_state(instance, IP6_UP_STATE);
      break;

    default:
      break;

    } /* switch(state) */
    break; /* VALID_RA */


  case IP6_SM_LINK_CHANGED_EV:
    /*-----------------------------------------------------------------------
                                LINK_CHANGED
    -----------------------------------------------------------------------*/
    switch(instance->state)
    {
    case IP6_RESOL_FOR_PREFIX_STATE:
      /*---------------------------------------------------------
                              RESOL_FOR_PREFIX

        Transition back to the resol_for_prefix state.
      ---------------------------------------------------------*/
      ip6_smi_transition_state(instance, IP6_RESOL_FOR_PREFIX_STATE);
      /* fall thru */

    case IP6_SOL_WAIT_STATE:
    case IP6_SOL_STATE:
    case IP6_UP_STATE:
    case IP6_RESOL_FOR_RA_STATE:
    case IP6_DOWN_STATE:
    default:
/*---------------------------------------------------------
        Ignore this event in all other states.
      ---------------------------------------------------------*/
      break;

    } /* switch(state) */
    break; /* LINK_CHANGED */


  case IP6_SM_STOP_EV:
    /*-----------------------------------------------------------------------
                              STOP
    -----------------------------------------------------------------------*/
    switch(instance->state)
    {
    case IP6_SOL_WAIT_STATE:
    case IP6_SOL_STATE:
    case IP6_UP_STATE:
    case IP6_RESOL_FOR_PREFIX_STATE:
    case IP6_RESOL_FOR_RA_STATE:
      /*---------------------------------------------------------
                              SOL_WAIT
                                 SOL
                                 UP
                          RESOL_FOR_RA
                        RESOL_FOR_PREFIX

        Transition to the Down state.
      ---------------------------------------------------------*/
      ip6_smi_transition_state(instance, IP6_DOWN_STATE);
      /* fall thru */

    case IP6_DOWN_STATE:
    default:
      /*---------------------------------------------------------
        This event is ignored in all other states.
      ---------------------------------------------------------*/
      break;

    } /* switch(state) */
    break; /* STOP */

  default:
    ASSERT(0);
    return -1;
  } /* switch(event) */

  return 0;

} /* ip6_post_event() */



/*===========================================================================
FUNCTION IP6_SM_IND_CBACK_REG()

DESCRIPTION
  This function is used to register for indications from the SM.

PARAMETERS
  instance: the instance of the SM to register with.
  cback: the callback being registered.
  user_data: data that is passed back in the callback.

RETURN VALUE
  0: on success
 -1: on failure

DEPENDENCIES
  ip6_sm_create() needs to have been called first.

SIDE EFFECTS
  None
===========================================================================*/
int ip6_sm_ind_cback_reg
(
  ip6_sm_type       *instance,
  ip6_sm_cback_type  cback,
  void              *user_data
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if( !IP6_SMI_IS_VALID(instance) )
  {
    return -1;
  }

  instance->ind.cback = cback;
  instance->ind.data  = user_data;

  return 0;

} /* ip6_sm_ind_cback_reg() */

/*===========================================================================
FUNCTION       IP6_SM_IS_PRIV_EXT_ENABLED

DESCRIPTION    Access method for ipv6_priv_ext_enabled flag

PARAMETERS
  None

RETURN VALUE   
  ipv6_priv_ext_enabled

DEPENDENCIES   
  None

SIDE EFFECTS   
  None
===========================================================================*/
boolean ip6_sm_is_priv_ext_enabled()
{
  return ipv6_priv_ext_enabled;
}

/*===========================================================================
FUNCTION       IP6_SM_GET_PRIV_EXT_LIFETIMES

DESCRIPTION    Access method for priv_ext_lifetimes structure

PARAMETERS
  None

RETURN VALUE   
  Pointer to priv_ext_lifetimes structure

DEPENDENCIES   
  None

SIDE EFFECTS   
  None
===========================================================================*/
priv_ext_lifetimes_type *ip6_sm_get_priv_ext_lifetimes()
{
  return &priv_ext_lifetimes;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                             INTERNAL FUNCTIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
/*===========================================================================
FUNCTION IP6_SMI_TRANSITION_STATE()

DESCRIPTION
  This function changes the state of the IPv6 state machine - it should only
  be called from the post event function.

PARAMETERS
  instance: the instance to operate on.
  new_state: the state to transition to.

RETURN VALUE
  None

DEPENDENCIES
  ip6_sm_create() needs to have been called first.

SIDE EFFECTS
  None
===========================================================================*/
void ip6_smi_transition_state
(
  ip6_sm_type       *instance,
  ip6_smi_state_type new_state
)
{
  ip6_smi_state_type old_state;
  int16 ps_errno;
  ps_timer_error_type error_ret;
  ip6_sm_transition_event_payload_type transition_event_payload;
  ip_filter_type icmp6_filter;
  ps_iface_ipfltr_add_param_type fltr_param;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  old_state = instance->state;
  instance->state = new_state;

#ifdef T_ARM
  LOG_MSG_INFO2("IPv6 SM from state %d to %d", old_state, new_state, 0);
#else
  printf("IPv6 SM from state %s to %s",
         ip6_smi_state_names[old_state],
         ip6_smi_state_names[new_state]);
#endif

  /*-------------------------------------------------------------------------
    do state based processing
  -------------------------------------------------------------------------*/
  switch(instance->state)
  {
  case IP6_DOWN_STATE:
    /*-----------------------------------------------------------------------
                                 DOWN STATE

      Since the down indication should only be posted when not previously
      down there is no reason to do any of the other state processing unless
      already down.
    -----------------------------------------------------------------------*/
    if(old_state != IP6_DOWN_STATE)
    {
      instance->tries = instance->params.max_sol_attempts;
      instance->params.resol_interval = instance->params.init_resol_interval;
      error_ret = ps_timer_cancel(instance->rs_timer);
      if(PS_TIMER_SUCCESS != error_ret)
      {
        LOG_MSG_ERROR("RS timer cancel failure!",0,0,0);
        ASSERT(0);
        return;
      }
      error_ret = ps_timer_cancel(instance->ra_timer);
      if(PS_TIMER_SUCCESS != error_ret)
      {
        LOG_MSG_ERROR("RA timer cancel failure!",0,0,0);
        ASSERT(0);
        return;
      }

      /*---------------------------------------------------------------------
        De-register IP6 filter
      ---------------------------------------------------------------------*/
      if( PS_IFACE_IS_VALID(instance->ps_iface_ptr) &&
          instance->ip6_fltr != PS_IFACE_IPFLTR_INVALID_HANDLE)
      {
        (void) ps_iface_ipfltr_delete(instance->ps_iface_ptr,
                                      IP_FLTR_CLIENT_SOCKETS,
                                      instance->ip6_fltr,
                                      &ps_errno);
      }
      /*---------------------------------------------------------------------
        Only call the callback if it is not null
      ---------------------------------------------------------------------*/
      if(instance->ind.cback != NULL)
      {
        instance->ind.cback(instance, IP6_SM_DOWN_IND, instance->ind.data);
      }
    }
    break;

  case IP6_SOL_WAIT_STATE:
    /*-----------------------------------------------------------------------
                          WAITING TO SOLICIT STATE

      Register IP filters on the bounded iface
      Start the initial delay timer.
    -----------------------------------------------------------------------*/
    /*---------------------------------------------------------------------
      Set up ICMP6 filter for Router Advertisements (RA type 134).
    ---------------------------------------------------------------------*/
    memset(&icmp6_filter, 0, sizeof(ip_filter_type));
    memset(&fltr_param, 0, sizeof(ps_iface_ipfltr_add_param_type));
    icmp6_filter.ip_vsn                  = IP_V6;
    icmp6_filter.ip_hdr.v6.field_mask    = IPFLTR_MASK_IP6_NEXT_HDR_PROT;
    icmp6_filter.ip_hdr.v6.next_hdr_prot = PS_IPPROTO_ICMP6;
    icmp6_filter.next_prot_hdr.icmp.field_mask = IPFLTR_MASK_ICMP_MSG_TYPE;
    icmp6_filter.next_prot_hdr.icmp.type = ICMP6_RTR_ADV;

    /*-------------------------------------------------------------------
      Register the filters with the bounded interface.
    -------------------------------------------------------------------*/
    fltr_param.enable             = TRUE;
    fltr_param.fi_result          = 7;    /* FI result should return 7 */
    fltr_param.fi_ptr_arr         = &icmp6_filter;
    fltr_param.num_filters        = 1;
    fltr_param.filter_type        = IPFLTR_DEFAULT_TYPE;
    fltr_param.is_validated       = FALSE;
    fltr_param.fltr_priority      = PS_IFACE_IPFLTR_PRIORITY_DEFAULT;
    fltr_param.fltr_compare_f_ptr = NULL;
    
    if((instance->ip6_fltr = 
          ps_iface_ipfltr_add(instance->ps_iface_ptr,
                              IP_FLTR_CLIENT_SOCKETS,
                              &fltr_param,
                              &ps_errno)) == PS_IFACE_IPFLTR_INVALID_HANDLE)
    {
      LOG_MSG_ERROR("ICMPv6 filter reg failure!",0,0,0);
      ASSERT(0);
      return;
    }

    error_ret = ps_timer_start(instance->rs_timer,
                               instance->params.init_sol_delay);
    if(PS_TIMER_SUCCESS != error_ret)
    {
      LOG_MSG_ERROR("RS timer start failure!",0,0,0);
      ASSERT(0);
      return;
    }
    break;

  case IP6_SOL_STATE:
    /*-----------------------------------------------------------------------
                              SOLICITING STATE

      Start the solicitation interval timer.
    -----------------------------------------------------------------------*/
    error_ret = ps_timer_start(instance->rs_timer,
                               instance->params.sol_interval);
    if(PS_TIMER_SUCCESS != error_ret)
    {
      LOG_MSG_ERROR("RS timer start failure!",0,0,0);
      ASSERT(0);
      return;
    }

    if(ip6_smi_send_rs(instance) == 0)
    {
      instance->tries--;
    }
    break;

  case IP6_RESOL_FOR_RA_STATE:
    /*-----------------------------------------------------------------------
                       RE-SOLICITING FOR RA STATE

      start the RS_Timer, and send an RS and update the interval
    -----------------------------------------------------------------------*/
    error_ret = ps_timer_start(instance->rs_timer,
                               instance->params.resol_interval);
    if(PS_TIMER_SUCCESS != error_ret)
    {
      LOG_MSG_ERROR("RS timer start failure!",0,0,0);
      ASSERT(0);
      return;
    }

    if(ip6_smi_send_rs(instance) == 0)
    {
      instance->tries--;
    }
    break;

  case IP6_RESOL_FOR_PREFIX_STATE:
    /*-----------------------------------------------------------------------
                       RE-SOLICITING FOR PREFIX STATE

      If the phys link is down, bring it up explicitly
      Otherwise, start the RS_Timer, and send an RS and update the interval.
    -----------------------------------------------------------------------*/
    if((ps_iface_phys_link_state(instance->ps_iface_ptr) == PHYS_LINK_DOWN) ||
       (ps_iface_phys_link_state(instance->ps_iface_ptr) == PHYS_LINK_NULL))
    {
      if(-1 ==
          ps_iface_phys_link_up_cmd(instance->ps_iface_ptr, &ps_errno, NULL))
      {
        if(ps_errno != DS_EWOULDBLOCK)
        {
          LOG_MSG_FATAL_ERROR("phy link up fail,%d phys_link ret" , ps_errno, 0, 0);
          return;
        }
      }
    }
    else
    {
      error_ret = ps_timer_start(instance->rs_timer,
                                 instance->params.resol_interval);
      if(PS_TIMER_SUCCESS != error_ret)
      {
        LOG_MSG_ERROR("RS timer start failure!",0,0,0);
        ASSERT(0);
        return;
      }

      if(ip6_smi_send_rs(instance) == 0)
      {
        instance->tries--;
      }
    } /* else(not dormant) */
    break;

  case IP6_UP_STATE:
    /*-----------------------------------------------------------------------
                                  UP STATE
    -----------------------------------------------------------------------*/
    error_ret = ps_timer_cancel(instance->rs_timer);
    if(PS_TIMER_SUCCESS != error_ret)
    {
      LOG_MSG_ERROR("RS timer cancel failure!",0,0,0);
      ASSERT(0);
      return;
    }

    instance->params.resol_interval = instance->params.init_resol_interval;
    instance->tries                 = instance->params.max_resol_attempts;

    break;

  default:
    break;

  } /* switch(state) */

  /*-------------------------------------------------------------------------
    Report the transition to diag
  -------------------------------------------------------------------------*/
  if(old_state != new_state)
  {
    memset(&transition_event_payload, 
           0, 
           sizeof(ip6_sm_transition_event_payload_type));
    transition_event_payload.iface_name = instance->ps_iface_ptr->name;
    transition_event_payload.iface_instance = 
      instance->ps_iface_ptr->instance;
    transition_event_payload.old_state = old_state;
    transition_event_payload.new_state = new_state;
    event_report_payload( EVENT_IPV6_SM_TRANSITION,
                          sizeof( transition_event_payload ),
                          &transition_event_payload );
  }

} /* ip6_smi_transition_state() */



/*==========================================================================
FUNCTION IP6_SMI_SEND_RS()

DESCRIPTION
  This function populates the router solicitation message  structure and
  calls icmp6_output() function to send it.

PARAMETERS
  instance: the instance to operate on.

RETURN VALUE
  0 on sucesss
 -1 on failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ip6_smi_send_rs
(
  ip6_sm_type               *instance
)
{
  ps_icmp6_msg_type  icmp6_msg;
  struct ps_in6_addr dest_addr;
  struct icmp_cb     icmp6_cb;
  uint16             length;
  dsm_item_type     *dsm_buf = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   LOG_MSG_INFO1("Sending RS Message",0,0,0);

  /*-----------------------------------------------------------------------
    Create Router Solicitation Message
  ------------------------------------------------------------------------*/
  memset(&icmp6_cb,0,sizeof(struct icmp_cb));
  icmp6_msg.cmn_hdr.type = (uint8)ICMP6_RTR_SOL;
  icmp6_msg.cmn_hdr.code = 0;

  icmp6_cb.ipcb.routing_cache = instance->ps_iface_ptr;
  icmp6_cb.ipcb.ttl = IP_DEF_TTL;
  icmp6_cb.ipcb.df = 0;
  icmp6_cb.ipcb.qos  = 0;

  /*-----------------------------------------------------------------------
    Currently no options are included in the RS message, so set it to
    FALSE.
  ------------------------------------------------------------------------*/
  icmp6_msg.msg_type.rs.src_link_addr_incl = FALSE;

  /*-----------------------------------------------------------------------
    Set the destination address to the all-routers multicast address
    FF02::2.
  ------------------------------------------------------------------------*/
  dest_addr.ps_s6_addr32[0] = ps_htonl(0xFF020000UL);
  dest_addr.ps_s6_addr32[1] = 0x0;
  dest_addr.ps_s6_addr32[2] = 0x0;
  dest_addr.ps_s6_addr32[3] = ps_htonl(0x2);

  /*-----------------------------------------------------------------------
    Optional Length of Data Portion of ICMP message.
  ------------------------------------------------------------------------*/
  length = 0;

  /*-----------------------------------------------------------------------
    Call icmp6_output() to add the imcp header and send the packet.
  ------------------------------------------------------------------------*/
  icmp6_output( &dest_addr,
                &icmp6_msg,
                dsm_buf,
                &icmp6_cb,
                length,
                instance->ps_iface_ptr
               );
  return 0;

} /* ip6_smi_send_rs() */



/*===========================================================================
FUNCTION IP6_SMI_PS_CMD_HANDLER()

DESCRIPTION
  This function will post an event to the state machine using the post even
  function.  It is registered with PS to handle all of the state machine
  related commands.

PARAMETERS
  cmd: the command that PS received.
  user_data_ptr: For all of these commands it should containt the instance.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ip6_smi_ps_cmd_handler
(
  ps_cmd_enum_type cmd,         /* Actual command to be processed         */
  void *user_data_ptr           /* Command specific user parameters       */
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  ASSERT( PS_IP6_SM_BASE_CMD <= cmd );
  ASSERT( PS_IP6_SM_MAX_CMD  >= cmd );

  /*-------------------------------------------------------------------------
    based on the command type post the appropriate event
  -------------------------------------------------------------------------*/
  if(-1 == ip6_sm_post_event((ip6_sm_type*)user_data_ptr,
               ((ip6_sm_event_type)((uint8)cmd - (uint8)PS_IP6_SM_BASE_CMD))))
  {
    LOG_MSG_ERROR("Couldn't post IPv6 event to ip6_sm!",0,0,0);
    return;
  }

} /* ip6_smi_ps_cmd_handler() */



/*===========================================================================
FUNCTION IP6_SMI_RS_TIMER_HANDLER()

DESCRIPTION
  This function is the timer handler for the RS timer.

PARAMETERS
  instance - this is the instance that of the SM to post the event to.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ip6_smi_rs_timer_handler
(
  void *instance
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if(-1 == ip6_sm_post_event((ip6_sm_type*)instance, IP6_SM_RS_TIMER_EV))
  {
    LOG_MSG_ERROR("Couldn't post IPv6 event to ip6_sm!",0,0,0);
    return;
  }

} /* ip6_smi_rs_timer_handler() */



/*===========================================================================
FUNCTION IP6_SMI_RA_TIMER_HANDLER()

DESCRIPTION
  This function is the timer handler for the RA timer.

PARAMETERA
  instance - this is the instance that of the SM to post the event to.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ip6_smi_ra_timer_handler
(
  void *instance
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if(-1 == ip6_sm_post_event((ip6_sm_type*)instance, IP6_SM_RA_TIMER_EV))
  {
    LOG_MSG_ERROR("Couldn't post IPv6 event to ip6_sm!",0,0,0);
    return;
  }

} /* ip6_smi_rs_timer_handler() */



/*===========================================================================
FUNCTION IP6_SMI_PHYS_LINK_CBACK()

DESCRIPTION
  This function is registered for the phys link up and down events.
  that the SM is associated with.

PARAMETERS
  None

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ip6_smi_phys_link_cback
(
  ps_iface_type             *this_iface_ptr,
  ps_iface_event_enum_type   event,
  ps_iface_event_info_u_type event_info,
  void                      *user_data_ptr
)
{
  ip6_sm_type *instance = (ip6_sm_type*)user_data_ptr;
  (void)this_iface_ptr;
  (void)event;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2("phys link event %d happened",
          event, 0, 0);

  if (ps_iface_phys_link_state(instance->ps_iface_ptr) !=
        event_info.phys_link_event_info.state)
  {
    if(-1 == ip6_sm_post_event((ip6_sm_type*)instance, IP6_SM_LINK_CHANGED_EV))
    {
      LOG_MSG_ERROR("Couldn't post IPv6 event to ip6_sm!",0,0,0);
      return;
    }
  }

} /* ip6_smi_phys_link_cback */



/*===========================================================================
FUNCTION IP6_SMI_VALID_RA_CBACK()

DESCRIPTION
  This function is registered for the VALID_RA event on the PS Iface
  that the SM is associated with.

PARAMETERS
  None

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ip6_smi_valid_ra_cback
(
  ps_iface_type             *this_iface_ptr,
  ps_iface_event_enum_type   event,
  ps_iface_event_info_u_type event_info,
  void                      *user_data_ptr
)
{
  ip6_sm_type *instance = (ip6_sm_type*)user_data_ptr;
  ps_timer_error_type error_ret;
  uint32              ra_lifetime;
  (void)this_iface_ptr;
  (void)event;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2("Valid RA indication; RA valid for: %ds",
          event_info.ra_lifetime, 0, 0);

  /*-------------------------------------------------------------------------
    Convert router life time to milliseconds
  -------------------------------------------------------------------------*/
  ra_lifetime = event_info.ra_lifetime * 1000;

  /*-------------------------------------------------------------------------
    Calculate the pre_ra_exp_resol_time.  It must fall in the following
    bounds:
    50% of RA Lifetime < pre_ra_exp_resol_time < RA Lifetime - max_resol_time
  -------------------------------------------------------------------------*/
  if(instance->params.pre_ra_exp_resol_time<instance->params.max_resol_time)
  {
    instance->params.remaining_ra_lifetime = instance->params.max_resol_time;
    LOG_MSG_INFO2("Max resol time(%d) > configured pre RA exp time(%d) using %d",
            instance->params.max_resol_time,
            instance->params.pre_ra_exp_resol_time,
            instance->params.remaining_ra_lifetime);
  }
  else
  {
    instance->params.remaining_ra_lifetime =
      instance->params.pre_ra_exp_resol_time;
    LOG_MSG_INFO2("Max resol time(%d) < configured pre RA exp time(%d) using %d",
            instance->params.max_resol_time,
            instance->params.pre_ra_exp_resol_time,
            instance->params.remaining_ra_lifetime);
    }

  if(instance->params.remaining_ra_lifetime > 
                                     ((ra_lifetime>>1) - (ra_lifetime>>2)))
  {
    instance->params.remaining_ra_lifetime = 
                                     ((ra_lifetime>>1) - (ra_lifetime>>2));
    LOG_MSG_INFO2("current pre RA exp time (%d) > 25%% of ra lifetime %d using %d",
                   instance->params.remaining_ra_lifetime,
                   ((ra_lifetime>>1) - (ra_lifetime>>2)),
                   instance->params.remaining_ra_lifetime);
  }
  else
  {
    LOG_MSG_INFO2("current pre RA exp time (%d) < 25%% of ra lifetime %d using %d",
                   instance->params.remaining_ra_lifetime,
                   ((ra_lifetime>>1) - (ra_lifetime>>2)),
                   instance->params.remaining_ra_lifetime);
  }

  instance->params.ra_resol_time =
    ra_lifetime - instance->params.remaining_ra_lifetime;

  LOG_MSG_INFO1("Will resolicit after %ds and %ds before RA expires.",
           instance->params.ra_resol_time/1000,
           instance->params.remaining_ra_lifetime/1000, 0);

  LOG_MSG_INFO1("Starting RA timer(%d) for %dms",
           instance->ra_timer,
           instance->params.ra_resol_time, 0);

  error_ret = ps_timer_start(instance->ra_timer,
                             instance->params.ra_resol_time);
  if(PS_TIMER_SUCCESS != error_ret)
  {
    LOG_MSG_ERROR("RA timer start failure!",0,0,0);
    ASSERT(0);
    return;
  }

  /*-------------------------------------------------------------------------
    post the VALID RA evant
  -------------------------------------------------------------------------*/
  if(-1 == ip6_sm_post_event((ip6_sm_type*)instance, IP6_SM_VALID_RA_EV))
  {
    LOG_MSG_ERROR("Couldn't post IPv6 event to ip6_sm!",0,0,0);
    return;
  }

} /* ip6_smi_valid_ra_cback() */



/*===========================================================================
FUNCTION IP6_SMI_PREFIX_UPDATE_CBACK()

DESCRIPTION
  This function is registered for the prefix_update event on the PS Iface
  that the SM is associated with.

PARAMETERS
  this_iface_ptr: interface calling the event.
  event: the event that occured
  event_info: information related to the event
  user_data_ptr: for this callback this should be the instance.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ip6_smi_prefix_update_cback
(
  ps_iface_type             *this_iface_ptr,
  ps_iface_event_enum_type   event,
  ps_iface_event_info_u_type event_info,
  void                      *user_data_ptr
)
{
  ip6_sm_type *instance = (ip6_sm_type*)user_data_ptr;
  (void)this_iface_ptr;
  (void)event;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2("Prefix Update indication %d for prefix 0x%llx.",
          event_info.prefix_info.kind,
          ps_ntohll(event_info.prefix_info.prefix.ps_s6_addr64[0]),
          0);

  switch(event_info.prefix_info.kind)
  {
    case PREFIX_REMOVED:
    /*-----------------------------------------------------------------------
      The prefix went away.  Since only 1 prefix is supported if we are in UP
      state tear down the call. If any other state do nothing as handoffs
      to new routers can cause addresses to be deleted, however, the IPv6 SM
      should be in DOWN state during resyncs.
    -----------------------------------------------------------------------*/
    switch( instance->state )
    {
      case IP6_UP_STATE:
        ip6_smi_transition_state(instance, IP6_DOWN_STATE);

      /* fall through */

      /*-----------------------------------------------------------------------
        Ignore in any state other than up
      -----------------------------------------------------------------------*/
      default:
        break;
    }
    break;

    case PREFIX_DEPRECATED:
    /*-----------------------------------------------------------------------
      Prefix Changed: post the RS Timer event
    -----------------------------------------------------------------------*/
    if(-1 == ip6_sm_post_event(instance, IP6_SM_RS_TIMER_EV))
    {
      LOG_MSG_ERROR("Couldn't post IPv6 event to ip6_sm!",0,0,0);
      return;
    }
    /* fall through */

    case PREFIX_ADDED:
    case PREFIX_UPDATED:
    /*-----------------------------------------------------------------------
      Don't do anything as the valid RA indication will bring up the SM if
      needed
    -----------------------------------------------------------------------*/
    break;

  default:
    LOG_MSG_ERROR("ip6_smi_prefix_update_cback(): Invalid prefix update event",0,0,0);
    ASSERT( 0 );
    break;
  } /* switch(update kind) */

} /* ip6_smi_prefix_update_cback() */

#endif /* FEATURE_DATA_PS && FEATURE_DATA_PS_IPV6 */
