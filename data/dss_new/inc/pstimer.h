#ifndef PS_TIMERS_H
#define PS_TIMERS_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                              P S T I M E R . H

GENERAL DESCRIPTION
  This is the source file for managing the API timers. The implementation is
  a standard delta list.

INITIALIZATION AND SEQUENCING REQUIREMENTS
  ps_timer_init() MUST be called before any other timer functions are
  called.

EXTERNALIZED FUNCTIONS
  Initialization functions:
    -  ps_timer_tick_init(tick_timer_cb_type *timer_cb_ptr): Initialize
       the Tick Timer that is used to tick other API timers.

  API Functions to be called by applications using Timer:
    1) ps_timer_alloc():      Function to allocate a timer from free pool.
    2) ps_timer_start():      Start the timer using milliseconds
    3) ps_timer_start_secs(): Start the timer using seconds
    4) ps_timer_cancel():     Cancel the timer
    5) ps_timer_free() :      Free the timer
    6) ps_timer_is_running:   Returns if the timer is running or not.

  PS Task to Timer API interface functions:
    1) ps_timer_start_tick: Starts the tick timer
    2) ps_timer_cancel_tick: Cancels the tick timer.
    3) ps_timer_handler: Function to be executed every time the tick
       timer expires.

Copyright (c) 2000-2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/pstimer.h#1 $ $DateTime: 2011/01/10 09:44:56 $ $Author: maldinge $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
01/11/10    ss     Lint warning fixes.  
02/19/09    ar     Added PS_TIMER_FREE()
12/14/08    pp     Common Modem Interface: Public/Private API split.
05/10/06    mct    Now using a random cookie value for timers.
04/22/06    mct    Reworked entire PS timer library to use new timer.h API.
                   Removed all old pstimer tick functionality. Implemented
                   callback mechanism to support extended timers through
                   REX timers.
03/21/05    ssh    Changes pursuant to the new typedef ps_timer_handle_type
10/26/04    ifk    Replaced reassembly timer with fraghdl timers
10/25/04    msr    Increase number of timers per TCB from 4 to 5 to include
                   persist timer.
06/16/04    mvl    changed the timers to use an int64 rather than an int32 to
                   support IPv6 prefix lifetimes.  Also added
                   ps_timer_start_secs() macro allowing timers to be started
                   in seconds (rather than milli seconds.
04/30/04    usb    Added timers for IP reassembly, include file cleanup.
04/28/04    mct    Increased # of sockets timers to account for extra TCBs.
06/11/03    jd     Added ps_timer_remaining to check time left on timer
05/04/03    ss     Increased PS_TIMER_SOCKET_TIMERS to account for timer
                   needed to support SO_LINGER socket option
11/06/02    ifk    Included dssdns.h and incremented PS timers for DNS.
10/08/02    aku    Removed FEATURE_DATA_MM featurization.
09/10/02    mvl    fixed number of timers used for PPP.
08/13/02    mvl    Fixed MM featurization.
08/04/02    mvl    Simplified MM featurization.
08/01/02    usb    Merged in changes for multimode ps under FEATURE_DATA_MM
07/12/02    jd     Added pvcs header tag to edit history
06/06/02    jd     Keep track of elapsed ms instead of ticks, since timer
                   might expire much later than one tick (50ms) as seen in
                   the case of dormancy/mobile ip reregistration.
03/13/02    pjb    There are 4 timers per TCB now not 3.
01/25/02    ss     Increased the maximum timers to 8 to account for the timer
                   used by DNS
07/27/01    mvl    Added 4 extra timers for LCP and IPCP: 2 for each iface
06/07/01    jd     Modified the time argument to ps_start_timer() to a sint31
                   so that longer times can be specified.
05/18/01  mvl/na   Changed the way the max timers is defined to more cleanly
                   support multiple features.
02/06/01    snn    Modified the code to use the clock services instead of
                   maintaining the timer. This ensures that the timer is
                   more accurate. (specially since PS task has very low
                   priority).
11/27/00    na     Cleanup. Moved timer_entry_type to pstimer.c
11/02/00    snn    Kept PS_TIMER_MAX in #define for FEATURE_DS_SOCKETS
                   Initialized PS_TIMER_FREE to zero. This allows us to
                   memset the timer structure.
                   Deleted unwanted defines from ps_timer_error_type
09/11/00   snn/na  created module
===========================================================================*/


/*===========================================================================

                      INCLUDE FILES FOR MODULE

===========================================================================*/


/*---------------------------------------------------------------------------
  The type of the callback function: the signature of the function should be:
     void function(int)
---------------------------------------------------------------------------*/
typedef void(timer_cb_type)(void *);

/*---------------------------------------------------------------------------
  Errors that the API's are passed
  The values should be only 0 and -1. Donot add to these.
---------------------------------------------------------------------------*/
typedef enum
{
  PS_TIMER_FAILURE = 0,
  PS_TIMER_SUCCESS                 = 1,
  PS_TIMER_ERROR_TYPE_FORCE_32_BIT = 0x7FFFFFFF
} ps_timer_error_type;

typedef uint32 ps_timer_handle_type;

#define PS_TIMER_INVALID_HANDLE ((ps_timer_handle_type)0)

/*===========================================================================

                        PUBLIC FUNCTION DECLARATIONS

===========================================================================*/
/*===========================================================================
MACRO PS_TIMER_FREE_HANDLE()

DESCRIPTION
  This macro frees a timer from the delta list.  It also clears the handle.

PARAMETERS
  handle: timer handle value

RETURN VALUE
  None
===========================================================================*/
#define PS_TIMER_FREE_HANDLE(handle)                                     \
  if (PS_TIMER_INVALID_HANDLE != handle)                                 \
  {                                                                      \
    (void) ps_timer_free( handle);                                      \
    handle = PS_TIMER_INVALID_HANDLE;                                    \
  }

/*===========================================================================
FUNCTION PS_TIMER_ALLOC()

DESCRIPTION
  This function allocates a timer. Before any task uses the timer, it should
  first acquire a timer by calling alloc_timer. This function returns a timer
  handle to the caller. This timer handle can be used in the future to access
  this timer (until it is freed).

  It is assumed that every timer should have a valid call back associated
  with it. So, it takes the function callback as an argument.  The callback
  function must have the following signature: void fname(void *);

DEPENDENCIES
  After the timer is used, call FREE_TIMER. This will return the timer back
  to the free pool.

RETURN VALUE
   Success: Returns value between 1 to PS_TIMER_MAX.
   Failure: PS_TIMER_FAILURE

SIDE EFFECTS
  None
===========================================================================*/
extern ps_timer_handle_type ps_timer_alloc
(
  void (* callback) (void *),
  void *cb_param
);

/*===========================================================================
FUNCTION PS_TIMER_START()

DESCRIPTION
  This function takes a timer handle, the time in milli-seconds for the timer
  and a pointer to error number as arguments.

DEPENDENCIES
  None

RETURN VALUE
 PS_TIMER_FAILURE if there is an error setting the timer.
 PS_TIMER_SUCCESS: if the setting is success.

SIDE EFFECTS
  If the timer is the first timer in the list, then a PS timer is started
  which goes off every PS_TIMER_TICK_INTERVAL duration.
  If the the time_left is set to zero, the timer will be stopped i.e. removed
  from the list.
===========================================================================*/
extern ps_timer_error_type ps_timer_start
(
  ps_timer_handle_type handle,
  int64 time
);

/*===========================================================================
FUNCTION PS_TIMER_START_SECS()

DESCRIPTION
  This function starts the PS Timers

  It takes a timer handle, and the time in seconds for the timer

DEPENDENCIES
  None

RETURN VALUE
 PS_TIMER_FAILURE if there is an error setting the timer.
 PS_TIMER_SUCCESS: if the setting is success.

SIDE EFFECTS
  If the timer is the first timer in the list, then a PS timer is started
  which goes off every PS_TIMER_TICK_INTERVAL duration.
  If the the time_left is set to zero, the timer will be stopped i.e. removed
  from the list.
===========================================================================*/
#define ps_timer_start_secs( handle,  time_s )  \
  ps_timer_start( (handle), ((time_s)*1000))

/*===========================================================================
FUNCTION PS_TIMER_CANCEL()

DESCRIPTION
  This function cancels the timer.

DEPENDENCIES
  None

RETURN VALUE
 PS_TIMER_FAILURE if there is an error cancelling the timer.
 PS_TIMER_SUCCESS: if the cancelling is success.

SIDE EFFECTS

===========================================================================*/
extern ps_timer_error_type ps_timer_cancel
(
  ps_timer_handle_type handle
);

/*===========================================================================
FUNCTION PS_TIMER_FREE()

DESCRIPTION
  This function frees a timer from the delta list.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
extern ps_timer_error_type ps_timer_free
(
  ps_timer_handle_type handle
);

/*===========================================================================
FUNCTION PS_TIMER_IS_RUNNING()

DESCRIPTION
  This function can be used to check if a timer is running.

  Note that, even if a non valid handle is passed, this function returns
  FALSE, because that timer is not running.
  An example of where this function can be used is, in TCP delayed ack
  timers, always check if the timer is running before starting the timer
  again.

DEPENDENCIES
  None

RETURN VALUE
  TRUE: If the Timer is running
  False: If the Timer is not Running

SIDE EFFECTS
  None
===========================================================================*/
extern boolean ps_timer_is_running
(
  ps_timer_handle_type handle
);

/*===========================================================================
FUNCTION PS_TIMER_REMAINING()

DESCRIPTION
  This function is used to check the time left on a running timer.

DEPENDENCIES
  None

RETURN VALUE
  If timer is running, time left (in ms)
  If timer is stopped, 0
  If timer does not exist, -1

SIDE EFFECTS
  None
===========================================================================*/
extern int64 ps_timer_remaining
(
  ps_timer_handle_type handle
);


#endif /* PS_TIMERS_H */
