/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
                   P S _ P L A T F O R M _  T I M E R . C 

GENERAL DESCRIPTION
  This is the platform specific source file for managing the PS timer API. 
  The implementation utilizes the Linux Timer APIs.

Copyright (c) 2009 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                      INCLUDE FILES FOR MODULE

===========================================================================*/
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#include "assert.h"
#include "comdef.h"
#include "msg.h"
#include "amssassert.h"
#include "ds_list.h"
#include "ps_system_heap.h"
#include "ps_platform_timer.h"
#include "pstimer.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                        LOCAL DECLARATIONS FOR MODULE

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

#define TIMER_RELTIME 0x00

/* Timer callback function */
typedef void (*ps_timer_handler_f)(void*);

/* Timer definition data provided by caller */
struct ps_platform_timer_data_s {
  ps_timer_handler_f  ps_timer_handler_cb;
  void               *timer_handle;
  timer_t             timerid;
};

struct ps_platform_timer_s {
  ds_dll_el_t    *head;        /* List of timer definition data  */
  ds_dll_el_t    *tail;
  pthread_mutex_t mutx;        /* Mutex for protecting the list operations */
  boolean         init;        /* Ininitalization flag */
};

static struct ps_platform_timer_s
      ps_platform_timer_info = {NULL,NULL,PTHREAD_MUTEX_INITIALIZER,FALSE};

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                         LOCAL FUNCTION DECLARATIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/


/*===========================================================================
  FUNCTION  ps_platform_timer_signal_handler
===========================================================================*/
/*!
@brief
  Callback registered as OS handler for SIGALRM signal.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void ps_platform_timer_signal_handler
(
  int        sig,
  siginfo_t *info,
  void      *context
)
{
  struct ps_platform_timer_data_s * data = NULL;
  ds_dll_el_t * node = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ASSERT(info);
  (void)sig;
  (void)context;

  node = (ds_dll_el_t*)info->si_value.sival_ptr;
  ASSERT(node);
  data = (struct ps_platform_timer_data_s*)node->data;
  ASSERT(data);

  if( data->ps_timer_handler_cb ) {
    data->ps_timer_handler_cb( data->timer_handle );
  } else {
    MSG_ERROR( "Timer callback not specified, ignoring: %d",0,0,0 );
  }
}


/*===========================================================================
  FUNCTION  ps_platform_timer_signal_handler
===========================================================================*/
/*!
@brief
  Callback registered to compare timer data linked-list nodes.
  Used in delete operation.

@return
  long int - TBD

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
long int ps_platform_timer_compare_data (const void * first, const void * second)
{
  /* Return 0 on successful match */
  return !(first == second);
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                         GLOBAL FUNCTION DECLARATIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/*===========================================================================
FUNCTION PS_PLATFORM_TIMER_DEF()

DESCRIPTION
  This function takes a timer structure, callback pointer, and timer handle
  from the common layer.
  Defines/Initializes timer in platform specific layer.

DEPENDENCIES
  None

PARAMETERS
  timer: timer structure holding parameters required for platform timer APIs
  ps_timer_handler_cb_ptr: callback pointer
  timer_handle: timer handle used in platform specific layer

RETURN VALUE
  None

SIDE EFFECTS
  Timer buffer allocated from dynamic memory
===========================================================================*/
void ps_platform_timer_def
(
  void **  timer_ptr_ptr,
  void *   ps_timer_handler_cb,
  void *   timer_handle
)
{
  timer_t timerid;
  struct sigevent timer_sigevent;
  struct sigaction timer_sigaction;
  struct ps_platform_timer_data_s * data = NULL;
  ds_dll_el_t * node = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == timer_ptr_ptr)
  {
    MSG_ERROR( "NULL timer pointer specified",0,0,0 );
    ASSERT (0);
    return;
  }
  if (NULL == ps_timer_handler_cb)
  {
    MSG_ERROR( "NULL timer callback specified",0,0,0 );
    ASSERT (0);
    return;
  }
  if (NULL == timer_handle)
  {
    MSG_ERROR( "NULL timer handle specified",0,0,0 );
    ASSERT (0);
    return;
  }

  /* Check for module initialization completed, perform if not */
  if( !ps_platform_timer_info.init ) {
 
    /* Initialize linked-list for timer information */
    if( (ps_platform_timer_info.head = ds_dll_init(NULL)) == NULL) {
      MSG_ERROR( "Failed to initialize list",0,0,0 );
      ASSERT (0);
      return;
    }
    ps_platform_timer_info.tail = ps_platform_timer_info.head;

    /* Register SIGALRM signal handler for REALTIME timer */
    timer_sigaction.sa_flags = SA_SIGINFO;
    timer_sigaction.sa_sigaction = ps_platform_timer_signal_handler;
    if( (0 > sigemptyset( &timer_sigaction.sa_mask )) ||
        (0 > sigaction( SIGALRM, &timer_sigaction, NULL )) ) {
      MSG_ERROR( "Failed to register signal handler",0,0,0 );
      ASSERT (0);
      return;
    }

    ps_platform_timer_info.init = TRUE;
  }

  /* Allocate buffer to store timer definition data */
  data = ps_system_heap_mem_alloc( sizeof(struct ps_platform_timer_data_s) );
  if( NULL == data ) {
    MSG_ERROR( "Failed to allocation timer data buffer",0,0,0 );
    ASSERT (0);
    return;
  }

  data->ps_timer_handler_cb = ps_timer_handler_cb;
  data->timer_handle = timer_handle;

  /* Store timer data in list */
  if( 0 > pthread_mutex_lock(&ps_platform_timer_info.mutx) ) {
    MSG_ERROR( "pthread_mutex_lock failed",0,0,0 );
    ASSERT (0);
    goto bail;
  }

  if( NULL ==
      (node = ds_dll_enq( ps_platform_timer_info.tail, NULL, data )) ) {
    MSG_ERROR( "Failed to inset into list",0,0,0 );
    ASSERT (0);
    goto bail;
  }
  ps_platform_timer_info.tail = node;
  
  if( 0 > pthread_mutex_unlock(&ps_platform_timer_info.mutx) ) {
    MSG_ERROR( "pthread_mutex_unlock failed",0,0,0 );
    ASSERT (0);
    goto bail;
  }

  /* Preserve timer data list reference in OS timer spec */
  timer_sigevent.sigev_notify = SIGEV_SIGNAL;
  timer_sigevent.sigev_signo = SIGALRM;
  timer_sigevent.sigev_value.sival_ptr = node;

  /* Create actual timer using REALTIME clock */
  if( 0 > timer_create( CLOCK_REALTIME, &timer_sigevent, &timerid) ) {
    MSG_ERROR( "Failed to create timer",0,0,0 );
    ASSERT (0);
    goto bail;
  }
 
  data->timerid = timerid;
  *timer_ptr_ptr =  (void*)node;
  return;

bail:
  PS_SYSTEM_HEAP_MEM_FREE(data);
}

/*===========================================================================
FUNCTION PS_PLATFORM_TIMER_SET()

DESCRIPTION
  This function takes a timer structure from the common layer.
  Calls platform specific timer set API.  The void pointer is casted
  to platform specific timer structure in platform layer.

DEPENDENCIES
  None

PARAMETERS
  timer: timer structure holding parameters required for platform timer APIs
  tiemr_val: event delay value (in msec)

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_platform_timer_set
(
  void *                  timer, 
  uint32                  timer_val
)
{
  struct ps_platform_timer_data_s * data = NULL;
  ds_dll_el_t * node = NULL;
  struct itimerspec  timespec;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  node = (ds_dll_el_t*)timer;
  ASSERT( node );
  data = (struct ps_platform_timer_data_s*)node->data;
  ASSERT( data );

  /* Convert timer value units to local representation */
  memset( (void*)&timespec, 0x0, sizeof(timespec) );
  timespec.it_value.tv_sec = timer_val / 1000;
  timespec.it_value.tv_nsec =
    (timer_val - (timespec.it_value.tv_sec*1000)) * 1000000;
  MSG_LOW( "ps_platform_timer_set: timer val %dl conversion to %d sec $d nsec",
           timer_val, timespec.it_value.tv_sec, timespec.it_value.tv_nsec );

  /* Kickoff timer */
  /* TODO: Consider using absolute time for better accuracy */
  if( 0 > timer_settime( data->timerid, TIMER_RELTIME, &timespec, NULL) ) {
    MSG_ERROR( "Failed to set timer",0,0,0 );
    ASSERT (0);
    return;
  }
}

/*===========================================================================
FUNCTION PS_PLATFORM_TIMER_IS_ACTIVE()

DESCRIPTION
  This function takes a timer structure from the common layer.
  Returns true if timer is still running, returns false otherwise.

DEPENDENCIES
  None

PARAMETERS
  timer: timer structure holding parameters required for platform timer APIs

RETURN VALUE
  True: Timer is running
  False: Timer is stopped

SIDE EFFECTS
  None
===========================================================================*/
int ps_platform_timer_is_active
(
  void *                  timer
)
{
  struct ps_platform_timer_data_s * data = NULL;
  ds_dll_el_t * node = NULL;
  struct itimerspec  timespec;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  node = (ds_dll_el_t*)timer;
  ASSERT( node );
  data = (struct ps_platform_timer_data_s*)node->data;
  ASSERT( data );

  /* Query timer for remaining time */
  if( 0 > timer_gettime( data->timerid, &timespec ) ) {
    MSG_ERROR( "Failed to set timer",0,0,0 );
    ASSERT (0);
    return FALSE;
  }

  /* Report active if some time remaining */
  return ( timespec.it_value.tv_sec || timespec.it_value.tv_nsec );
}

/*===========================================================================
FUNCTION PS_PLATFORM_TIMER_CLR()

DESCRIPTION
  This function takes a timer structure from the common layer.
  Stops specified timer and remove it from the active timer list.

DEPENDENCIES
  None

PARAMETERS
  timer: timer structure holding parameters required for platform timer APIs

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_platform_timer_clr
(
  void *                  timer
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /* Clearing a timer will prevent its signal activation but leave it
   * otherwise intect.  For this platform, just set time to zero. */
  ps_platform_timer_set( timer, 0 );
}

/*=============================================================================
FUNCTION PS_PLATFORM_TIMER_GET_MAX_SUPPORTED_TIMER()

DESCRIPTION
  Returns maximum timer value supported by platform, in msec 

DEPENDENCIES
  Valid sclk estimate

RETURN VALUE
  Time in the unit requested

SIDE EFFECTS
  None

=============================================================================*/
uint32 ps_platform_timer_get_max_supported_timer( void )
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /* Timer values are specified via struct itimerspec, consisting of
   * secs and nsecs elements. Each is of time_t, which is long for ARM
   * architecture.  So maximum timer value is sum of both, 2**31 +
   * 2**31/10**9 seconds. Converted to msecs, this exceeds uint32 so use
   * max uint32 value. */
  return 0xFFFFFFFF;
}

/*===========================================================================
FUNCTION PS_PLATFORM_TIMER_GET()

DESCRIPTION
  This function takes a timer structure from the common layer.
  Checks the timer to determine how much time is left.

DEPENDENCIES
  None

PARAMETERS
  timer: timer structure holding parameters required for platform timer APIs

RETURN VALUE
  returns time left in the specified timer (in msec).

SIDE EFFECTS
  None
===========================================================================*/
int64 ps_platform_timer_get
(
  void *    timer
)
{
  struct ps_platform_timer_data_s * data = NULL;
  ds_dll_el_t * node = NULL;
  struct itimerspec  timespec;
  int64 result = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  node = (ds_dll_el_t*)timer;
  ASSERT( node );
  data = (struct ps_platform_timer_data_s*)node->data;
  ASSERT( data );

  /* Query timer for remaining time */
  if( 0 > timer_gettime( data->timerid, &timespec ) ) {
    MSG_ERROR( "Failed to set timer",0,0,0 );
    ASSERT (0);
    return FALSE;
  }

  /* Convert to msec units */
  result = (timespec.it_value.tv_sec * 1000) +
           (timespec.it_value.tv_nsec / 1000000 );
  MSG_LOW( "ps_platform_timer_get: %d sec $d nsec consersion to timer val %dll",
           imespec.it_value.tv_sec, timespec.it_value.tv_nsec, result );  
  
  return result;
}

/*===========================================================================
FUNCTION PS_PLATFORM_TIMER_FREE()

DESCRIPTION
  This function takes a timer structure from the common layer.
  Timer structure will be deallocated.
  
DEPENDENCIES
  None

PARAMETERS
  timer: timer structure holding parameters required for platform timer APIs

RETURN VALUE
  None

SIDE EFFECTS
  Timer buffer released to dynamic memory
===========================================================================*/
void ps_platform_timer_free
(
  void **      timer_ptr_ptr
)
{
  struct ps_platform_timer_data_s * data = NULL;
  ds_dll_el_t * node = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ASSERT( timer_ptr_ptr );

  node = (ds_dll_el_t*)*timer_ptr_ptr;
  ASSERT( node );
  data = (struct ps_platform_timer_data_s*)node->data;
  ASSERT( data );

  if( ps_platform_timer_is_active( *timer_ptr_ptr ) )
  {
    ps_platform_timer_clr( *timer_ptr_ptr );
  }

  /* Remove timer data from list */
  if( 0 > pthread_mutex_lock(&ps_platform_timer_info.mutx) ) {
    MSG_ERROR( "pthread_mutex_lock failed",0,0,0 );
    ASSERT (0);
    return;
  }

  if( NULL ==
      (node = ds_dll_delete( ps_platform_timer_info.head,
                             &ps_platform_timer_info.tail,
                             data,
                             ps_platform_timer_compare_data )) ) {
    MSG_ERROR( "Failed to delete from list",0,0,0 );
    ASSERT (0);
    return;
  }

  if( 0 > pthread_mutex_unlock(&ps_platform_timer_info.mutx) ) {
    MSG_ERROR( "pthread_mutex_unlock failed",0,0,0 );
    ASSERT (0);
    return;
  }

  /* Release timer data buffer back to dynamic memory */
  PS_SYSTEM_HEAP_MEM_FREE(*timer_ptr_ptr);
  return;
}
