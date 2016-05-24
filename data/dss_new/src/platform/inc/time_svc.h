#ifndef TIME_SVC_H
#define TIME_SVC_H
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


Copyright (c) 2003 - 2005 by Qualcomm Technologies, Inc.  All Rights Reserved.

=============================================================================*/


/*=============================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

$Header: //depot/asic/sandbox/users/hmurari/qcmapi_porting/stubs/inc/time_svc.h#1 $

when       who     what, where, why
--------   ---     ------------------------------------------------------------
11/18/05   mod/ajn Added MediaFLO time support.
05/10/05   ajn     Added time_set_from_pmic( ) function, to correct init order.
11/24/04   grl     Added support for getting time in ms in native uint64 types.
02/09/04   ajn     Renamed, to avoid conflict with WinCE test environment
08/08/03   ajn     Changed timestamp type from qword to time_type (a qword)
08/01/03   ajn     Moved _get_sec, get_ms from time_tod.  Added _get_local_sec
07/24/03   ajn     File created.

=============================================================================*/



/*=============================================================================

                           INCLUDE FILES

=============================================================================*/

#include "qw.h"


/*=============================================================================

                           DATA DEFINITIONS

=============================================================================*/


/*-----------------------------------------------------------------------------

                      SYSTEM TIMESTAMP FORMAT


   |<------------ 48 bits --------------->|<----- 16 bits ------->|
   +--------------------------------------+-----------------------+
   |      1.25 ms counter                 |   1/32 chip counter   |
   +--------------------------------------+-----------------------+
         (11K years of dynamic range)          (1.25 ms dynamic
                                                range. Rolls over
                                                at count 49152)

-----------------------------------------------------------------------------*/

/* Time is represented as a 64-bit value ... currently a qword */
typedef qword time_type;


/*-----------------------------------------------------------------------------
  Time Source, returned by time_get( )
-----------------------------------------------------------------------------*/

typedef enum
{
  TIME_SOURCE_32KHZ,              /* Time was from 32kHz slow clock */
  TIME_SOURCE_CDMA,               /* Time was from CDMA's concept of time */
  TIME_SOURCE_HDR,                /* Time was from HDR's concept of time */
  TIME_SOURCE_GSM,                /* Time was from GSM's concept of time */
  TIME_SOURCE_WCDMA,              /* Time was from WCDMA's concept of time */
  TIME_SOURCE_GPS,                /* Time was from GPS's concept of time */
  TIME_SOURCE_MFLO,                /* Time was from MFLO's concept of time */
  TIME_SOURCE_PLATFORM            /* Time was from HLOS concept of time */
}
time_source_enum_type;



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
  Installs timekeeping ISR, timers, etc.

=============================================================================*/

void time_init( void );



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
);



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
);



/*=============================================================================

FUNCTION TIME_GET_MS

DESCRIPTION
  Get the system time, in # of milliseconds since "the beginning of time".

DEPENDENCIES
  A valid CDMA time, or a valid HDR time, or a valid Time-of-Day, etc.

RETURN VALUE
  Milliseconds since 6 Jan 1980 00:00:00 returned to the time parameter

SIDE EFFECTS
  None

=============================================================================*/

void time_get_ms
(
  /* Out: Time in milliseconds since 6 Jan 1980 00:00:00 */
  time_type                       time
);



/*=============================================================================

FUNCTION TIME_GET_SECS

DESCRIPTION
  Get the system time, in # of seconds since "the beginning of time".
  136 year range, beginning 6 Jan 1980 00:00:00.

DEPENDENCIES
  A valid CDMA time, or a valid HDR time, or a valid Time-of-Day, etc.

RETURN VALUE
  Seconds since 6 Jan 1980 00:00:00

SIDE EFFECTS
  None

=============================================================================*/

uint32 time_get_secs( void );



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

uint32 time_get_local_secs( void );



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

uint32 time_get_uptime_secs( void );



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
);



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

void time_sync_time_of_day( void );



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

void time_set_from_pmic( void );



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
);



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

uint64 time_get_ms_native( void );




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
);


#endif /* TIME_H */

