#ifndef PS_IP6_SM
#define PS_IP6_SM
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                            P S _ I P 6 _ S M . H

DESCRIPTION
  Header file defining the API for the IPv6 state machine.  This state
  machine is responsible for soliciting for Router Advertisements.  When a
  valid RA is received it will be in the Net Up state - which means that the
  IP interface it is associated with is in the up state.

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
  ip6_sm_ind_cback_reg()
    Used to register for the UP/DOWN indications.
  ip6_sm_is_ipv6_priv_ext_enabled()
    Access method for ipv6_priv_ext_enabled flag
  ip6_sm_get_priv_ext_lifetimes()
    Access method for priv_ext_lifetimes structure

Copyright (c) 2003-2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                            EDIT HISTORY FOR FILE

  $Header: //source/qcom/qct/modem/datamodem/protocols/api/rel/11.03/ps_ip6_sm.h#1 $
  $Author: zhasan $ $DateTime: 2011/06/17 12:02:33 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
03/21/11    op     Data SU API cleanup
09/21/10    ash    Increased IP6_SM_DEFAULT_SOL_INTERVAL from 2 sec to 4 sec
                   and decreased IP6_SM_DEFAULT_MAX_RESOL_ATTEMPTS from 5 to 3
08/11/10    ash    Increased IP6_SM_DEFAULT_SOL_INTERVAL to 4000
05/07/09    pp     CMI Phase-4: SU Level API Effort.
11/02/06    mct    Added support for IPv6 Privacy Extensions.
02/22/06    msr    Using single critical section
02/06/06    mct    Updated for L4 tasklock/crit sections.
03/21/05    ssh    Changes pursuant to the new typedef ps_timer_handle_type
08/12/04    mvl    Changed some RA resol fields for clarity.
07/12/04    mvl    Updates to the state machine to reflect design changes:
                   specifically the addition of a new state which allows the
                   differentiation of resolicitation for RAs and Prefixes.
06/03/04    mct    Added config struct for ip6 state machine. Updated
                   __ip6_sm_cb_s to take new rs config values
04/05/04    sv     Featurized the file.
03/02/04    rc     Added PS IFACE pointer to __ip6_sm_cb_s.
01/08/04    mvl    Some include cleanup.
12/22/03    mvl    Created module
===========================================================================*/

/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "pstimer.h"
#include "ps_iface_defs.h"
#include "ps_iface_ipfltr.h"
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
                            FORWARD DECLARATIONS
=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
struct __ip6_sm_cb_s; /* the state machine data structure                */

/*===========================================================================

                         EXTERNAL DATA DECLARATIONS

===========================================================================*/
#define IP6_SM_DEFAULT_MAX_SOL_ATTEMPTS          3
#define IP6_SM_DEFAULT_MAX_RESOL_ATTEMPTS        3
#define IP6_SM_DEFAULT_INIT_SOL_DELAY          500
#define IP6_SM_DEFAULT_SOL_INTERVAL           4000
#define IP6_SM_DEFAULT_RESOL_INTERVAL         4000
#define IP6_SM_DEFAULT_PRE_RA_EXP_RESOL_TIME     0

/*---------------------------------------------------------------------------
  Default Privacy Extensions lifetimes:
  - Preferred IID lifetime: 1 day. (in seconds)
  - Valid IID lifetime:     1 week. (in seconds)
---------------------------------------------------------------------------*/
#define DEFAULT_IPV6_PRIV_IID_PREF_LIFETIME  86400
#define DEFAULT_IPV6_PRIV_IID_VALID_LIFETIME 604800

/*---------------------------------------------------------------------------
TYPEDEF IP6_SM_TYPE

DESCRIPTION
  This is the type of IPv6 State Machine
---------------------------------------------------------------------------*/
typedef struct __ip6_sm_cb_s ip6_sm_type;

/*---------------------------------------------------------------------------
TYPEDEF IP6_SM_CONFIG

DESCRIPTION
  This is the type of IPv6 State Machine Configuration Structure
---------------------------------------------------------------------------*/
typedef struct
{
  int   init_sol_delay;             /* The initial solicitation delay (ms) */
  int   sol_interval;               /* The solicitation interval (ms)      */
  int   resol_interval;             /* The resolicitation interval (ms)    */
  int   max_sol_attempts;           /* # of initial solicitation attempts  */
  int   max_resol_attempts;         /* # of resolicitation attempts        */
  int   pre_ra_exp_resol_time;      /* Pre-RA expiration resol time (ms)   */
} ip6_sm_config_type;

typedef struct
{
  uint32 pref_lifetime_timer;
  uint32 valid_lifetime_timer;
} priv_ext_lifetimes_type;

/*---------------------------------------------------------------------------
TYPEDEF IP6_SM_EVENT_TYPE

DESCRIPTION
  This is for IPv6 SM events.
  The possible events are:
  - Start: posted from the controller (e.g. DSSNet6)
  - RS_Timer: posted internally from the RS timer OR by PS Iface when
      preferred lifetime for a prefix has expired.
  - RA_Timer: posted internally by the RA timer
  - Valid_RA: posted from PS Iface whenever a valid RA is received (this is
      done from the ps_iface_apply_v6_prefix() call.
  - Line_Changed: posted from PS Iface when the phys link state changes.
  - Stop: posted either by the controller, or PS Iface when the lifetime for
      all prefixes for that interface have expired.
---------------------------------------------------------------------------*/
typedef enum
{
  IP6_SM_MIN_EV      = 0,
  IP6_SM_START_EV    = 0,
  IP6_SM_RS_TIMER_EV = 1,
  IP6_SM_RA_TIMER_EV = 2,
  IP6_SM_VALID_RA_EV = 3,
  IP6_SM_LINK_CHANGED_EV = 4,
  IP6_SM_STOP_EV     = 5,
  IP6_SM_MAX_EV
} ip6_sm_event_type;

/*---------------------------------------------------------------------------
TYPEDEF IP6_SM_INDICATION_TYPE

DESCRIPTION
  This is the type of indications that clients can register for from the SM.
---------------------------------------------------------------------------*/
typedef enum
{
  IP6_SM_MIN_IND  = 0,
  IP6_SM_DOWN_IND = 0,
  IP6_SM_UP_IND   = 1,
  IP6_SM_MAX_IND
} ip6_sm_indidcation_type;

/*---------------------------------------------------------------------------
TYPEDEF IP6_SM_CBACK_TYPE

DESCRIPTION
  This is the type for the callbacks which can be registered for indications

PARAMETERS
  instance: the instance that caused the indication.
  ind: the indication that occurred (will only be posted on an edge - e.g. UP
       will only happen when not already up).
  user_data: data that was given at registration time.

RETURN VALUE
  None
---------------------------------------------------------------------------*/
typedef void (*ip6_sm_cback_type)
(
  struct __ip6_sm_cb_s *instance,
  ip6_sm_indidcation_type ind,
  void *user_data
);

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
  init_sol_delay: initial delay for solicitations (ms)
  sol_interval:   interval between solicitations in Soliciting state (ms)
  re_sol_interval: interval between solicitations in RE-Soliciting state (ms)

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
  ip6_sm_type         *instance,
  ps_iface_type       *iface_ptr,
  ip6_sm_config_type  *sm_config
);


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
);


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
);

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
boolean ip6_sm_is_priv_ext_enabled(void);

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
priv_ext_lifetimes_type *ip6_sm_get_priv_ext_lifetimes(void);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                           PRIVATE DATA STRUCTURES

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/*---------------------------------------------------------------------------
TYPEDEF IP6_SMI_STATE_TYPE

DESCRIPTION
  This is the definition of the IPv6 SM states.
---------------------------------------------------------------------------*/
typedef enum
{
  IP6_DOWN_STATE     = 0,
  IP6_SOL_WAIT_STATE = 1,
  IP6_SOL_STATE      = 2,
  IP6_UP_STATE       = 3,
  IP6_RESOL_FOR_RA_STATE     = 4,
  IP6_RESOL_FOR_PREFIX_STATE = 5,
  IP6_MAX_STATE
} ip6_smi_state_type;


/*---------------------------------------------------------------------------
STRUCT __IP6_SM_CB_S

DESCRIPTION
  This is the type of IPv6 State Machine
---------------------------------------------------------------------------*/
struct __ip6_sm_cb_s
{
  ip6_smi_state_type state;
  ps_iface_type      *ps_iface_ptr;
  ps_timer_handle_type   rs_timer;
  ps_timer_handle_type   ra_timer;
  ps_iface_ipfltr_handle_type ip6_fltr;
  int tries;

  struct
  {
    int init_sol_delay;
    int sol_interval;
    int max_sol_attempts;
    int init_resol_interval;
    int resol_interval;
    int max_resol_attempts;
    uint32 max_resol_time;        /* the maximum time re-sol can take      */
    uint32 pre_ra_exp_resol_time; /* configured time to resolicit for an RA*/
    uint32 remaining_ra_lifetime; /* time btw re-sol and lifetime expiring */
    uint32 ra_resol_time;         /* when to solicit for an RA             */
  } params;

  struct
  {
    ip6_sm_cback_type cback;
    void             *data;
  } ind;
  ip6_sm_type *this_sm_ptr;
};
#endif /* PS_IP6_SM */
