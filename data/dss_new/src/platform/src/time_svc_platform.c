/*=============================================================================

                   T I M E   S E R V I C E   S U B S Y S T E M

GENERAL DESCRIPTION
  Implements time-keeping functions using various sources.


EXTERNALIZED FUNCTIONS
  time_init( )
    Initializes the time services subsystem

  time_to_ms( )
    Converts a time stamp into milliseconds.

  time_get( )
    Retrieves time according to CDMA, HDR, GSM, or slow-clock Time-of-Day

  time_get_ms( )
    Retrieves system time in milliseconds

  time_get_sec( )
    Retrieves system time, in seconds

  time_get_local_sec( )
    Retrieves the time-zone adjusted local time time, in seconds

  time_get_uptime_ms( )
    Retrieves the up-time, in milliseconds

  time_get_uptime_secs( )
    Retrieves the up-time, in seconds

  time_sync_time_of_day( )
    Syncs the current time, from CDMA, HDR, or GSM, to the slow clock based
    time-of-day.

  time_set_from_pmic( )
    Set the time-of-day from the PMIC's RTC (if present)

  time_to_ms_native()
    Convert the current time to milliseconds in native uint64 format.

  time_get_ms_native()
    Get the current time in milliseconds in native uint64 format.


REGIONAL FUNCTIONS
  time_compute_power_on( )
    Called when a discontinuity of time-of-day might occur, to compute the
    power-on time, so that "up-time" (system_time - power_on_time) may
    be computed.


INITIALIZATION AND SEQUENCING REQUIREMENTS
  time_init( ) must be called to initalize the time subsystem state variables
  and install necessary ISRs.


Copyright (c) 2010 by Qualcomm Technologies, Inc.  All Rights Reserved.

=============================================================================*/


/*=============================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

$Header:$

when       who     what, where, why
--------   ---     ------------------------------------------------------------
05/05/10   ar      Initial version

=============================================================================*/


/*=============================================================================

                           INCLUDE FILES

=============================================================================*/

#include <errno.h>
#include <time.h>
#include "msg.h"
#include "amssassert.h"
#include "time_svc.h"


/*=============================================================================

                           DATA DEFINITIONS

=============================================================================*/



/*=============================================================================

                           FUNCTION DEFINITIONS

=============================================================================*/


/*=============================================================================

FUNCTION TIME_INIT

DESCRIPTION
  Initialize Timekeeping Subsystem

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

=============================================================================*/
void time_init( void )
{
}



/*=============================================================================

FUNCTION TIME_TO_MS

DESCRIPTION
  Convert a timestamp from System Time to millisecond units

DEPENDENCIES
  None

RETURN VALUE
  The converted value is stored in the 'time' parameter.

SIDE EFFECTS
  None

=============================================================================*/
void time_to_ms
(
  /* In: Time in time-stamp format;  Out: Time in milliseconds */
  time_type                       time
)
{
  (void)time;
}



/*=============================================================================

FUNCTION TIME_GET

DESCRIPTION
  Returns the current time

DEPENDENCIES
  A valid CDMA time, or a valid HDR time, or a valid Time-of-Day, etc.

RETURN VALUE
  Time-stamp returned to ts_val parameter.
  Time source used to determine time is returned

SIDE EFFECTS
  None

=============================================================================*/

time_source_enum_type time_get
(
  /* OUT: The current time */
  time_type                       ts_val
)
{
  (void)ts_val;
  MSG_ERROR("Failed on time_get, unsupported",0,0,0);
  return TIME_SOURCE_PLATFORM;
}



/*=============================================================================

FUNCTION TIME_GET_MS

DESCRIPTION
  Get the system time, in # of milliseconds since "the beginning of time".

DEPENDENCIES
  A valid CDMA time, or a valid HDR time, or a valid Time-of-Day, etc.

RETURN VALUE
  Milliseconds since system epoch (00:00:00 on January 1, 1970) returned
  to the time parameter

SIDE EFFECTS
  None

=============================================================================*/
void time_get_ms
(
  /* Out: Time in milliseconds since 00:00:00 on January 1, 1970 */
  time_type                       time
)
{
  struct timespec  tspec;
  long long result;

  if( -1 == clock_gettime( CLOCK_REALTIME, &tspec ) )
  {
    MSG_ERROR("Failed on clock_gettime, errno %d",errno,0,0);
    ASSERT(0);
    return;
  }

  result = (tspec.tv_sec * 1000) + (tspec.tv_nsec / 1000000);
  
  /* Assign hi and lo words of return value */
  qw_set( time, (result>>32), (result &0xFFFFFFFF));
  return;
}



/*=============================================================================

FUNCTION TIME_GET_SECS

DESCRIPTION
  Get the system time, in # of seconds since "the beginning of time".
  136 year range, beginning 6 Jan 1980 00:00:00.

DEPENDENCIES
  A valid CDMA time, or a valid HDR time, or a valid Time-of-Day, etc.

RETURN VALUE
  Seconds since system epoch (00:00:00 on January 1, 1970)

SIDE EFFECTS
  None

=============================================================================*/
uint32 time_get_secs( void )
{
  return time( NULL );
}



/*=============================================================================

FUNCTION TIME_GET_LOCAL_SECS

DESCRIPTION
  Get the local time, in # of seconds since "the beginning of time".
  136 year range, beginning 6 Jan 1980 00:00:00.

DEPENDENCIES
  A valid CDMA time, or a valid HDR time, or a valid Time-of-Day, etc.
  LTM_OFF and LP_SEC must be set in the DB database.

RETURN VALUE
  Seconds since 6 Jan 1980 00:00:00, adjusted for the local time zone.

SIDE EFFECTS
  None

=============================================================================*/
uint32 time_get_local_secs( void )
{
  MSG_ERROR("Failed on time_get_local_secs, unsupported",0,0,0);
  return 0;
}



/*=============================================================================

FUNCTION TIME_GET_UPTIME_SECS

DESCRIPTION
  Get time the phone has been powered on for

DEPENDENCIES
  A valid CDMA time, or a valid HDR time, or a valid Time-of-Day, etc.

RETURN VALUE
  Number of seconds phone has been powered on for.

SIDE EFFECTS
  None

=============================================================================*/
uint32 time_get_uptime_secs( void )
{
  MSG_ERROR("Failed on time_get_uptime_secs, unsupported",0,0,0);
  return 0;
}



/*=============================================================================

FUNCTION TIME_GET_UPTIME_MS

DESCRIPTION
  Get time the phone has been powered on for

DEPENDENCIES
  A valid CDMA time, or a valid HDR time, or a valid Time-of-Day, etc.

RETURN VALUE
  Number of milliseconds phone has been powered on for returned to 'uptime_ms'

SIDE EFFECTS
  None

=============================================================================*/

void time_get_uptime_ms
(
  /* Output: Milliseconds phone has been powered on for */
  time_type                       uptime_ms
)
{
  MSG_ERROR("Failed on time_get_uptime_ms, unsupported",0,0,0);
  uptime_ms = 0;
  return;
}



/*=============================================================================

FUNCTION TIME_SYNC_TIME_OF_DAY

DESCRIPTION
  Synchronize the slow-clock based time-of-day to the current "ts" time.

DEPENDENCIES
  A valid CDMA, HDR, GSM, GPS, WCDMA (etc) time.  time_get( ) must not be
  using time_tod_get( ) when this function is called.

RETURN VALUE
  None

SIDE EFFECTS
  An diagnostic message is generated if there is a sudden jump in time-of-day.

=============================================================================*/
void time_sync_time_of_day( void )
{
  MSG_ERROR("Failed on time_sync_time_of_day, unsupported",0,0,0);
  return;
}



/*=============================================================================

FUNCTION TIME_SET_FROM_PMIC

DESCRIPTION
  Initialize the slow-clock based time-of-day to the PMIC's RTC time.

DEPENDENCIES
  Time-of-day must not have been initialized be another time source.

RETURN VALUE
  None

SIDE EFFECTS
  None

=============================================================================*/
void time_set_from_pmic( void )
{
  MSG_ERROR("Failed on time_set_from_pmic, unsupported",0,0,0);
  return;
}




/*=============================================================================

FUNCTION TIME_TO_MS_NATIVE

DESCRIPTION
  Convert a timestamp from System Time to millisecond units

DEPENDENCIES
  None

RETURN VALUE
  The converted value.

SIDE EFFECTS
  None

=============================================================================*/
uint64 time_to_ms_native
(
  /* In: The current time in timestamp format. */
  time_type                       time_stamp
)
{
  (void)time_stamp;
  MSG_ERROR("Failed on time_to_ms_native, unsupported",0,0,0);
  return 0;
}



/*=============================================================================

FUNCTION TIME_GET_MS_NATIVE

DESCRIPTION
  Get a timestamp from System Time in millisecond units from 6 Jan 1980 
  00:00:00.

DEPENDENCIES
  None

RETURN VALUE
  The time in ms from 6 Jan 1980 00:00:00.

SIDE EFFECTS
  None

=============================================================================*/
uint64 time_get_ms_native( void )
{
  MSG_ERROR("Failed on time_get_ms_native, unsupported",0,0,0);
  return 0;
}




/*=============================================================================

                        REGIONAL FUNCTION DEFINITIONS

=============================================================================*/



/*=============================================================================

FUNCTION TIME_COMPUTE_POWER_ON                                         REGIONAL

DESCRIPTION
  Compute the power-on time, for uptime determination

DEPENDENCIES
  time_init( ) must have been called, to initialize time.power_on_ms
  May only be called from time_tod_set( ).

RETURN VALUE
  None

SIDE EFFECTS
  Updates time.power_on_ms and time.power_on_sec values.

=============================================================================*/
void time_compute_power_on
(
  /* Time value prior to updating to the correct time */
  time_type                       old_time,

  /* Time value after updating to the correct time value */
  time_type                       new_time
)
{
  (void)old_time;
  (void)new_time;
  MSG_ERROR("Failed on time_compute_power_on, unsupported",0,0,0);
  return;
}



