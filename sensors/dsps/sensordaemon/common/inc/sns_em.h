#ifndef _SNS_EM_H
#define _SNS_EM_H

/*============================================================================
  @file sns_em.h

  The sensors subsystem event manager header. 

  This header file contains the public interface for the sensors event manager. 
  The Event Manager provides timer functionality to the various modules in the 
  sensor subsystem.

  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
============================================================================*/

/* $Header: //source/qcom/qct/core/sensors/dsps/common/main/latest/inc/sns_em.h#1 $ */
/* $DateTime: 2011/01/28 12:40:41 $ */
/* $Author: agurujee $ */

/*============================================================================
  EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order. 

  when        who  what, where, why
  ----------  ---  -----------------------------------------------------------
  2010-11-30  ad   Added function to convert time in seconds in Q16 format
                   to time in local ticks
  2010-11-08  jtl  Adding function to get 64-bit timestamp.
  2010-08-30  jtl  Moving init function decl to sns_init.h
  2010-08-24  hm   Supports both periodic and one shot timers 
  2010-07-19  hm   Initial version

============================================================================*/


/*---------------------------------------------------------------------------
* Include Files
* -------------------------------------------------------------------------*/
#include "sns_common.h"

/*---------------------------------------------------------------------------
* Preprocessor Definitions and Constants
* -------------------------------------------------------------------------*/

/** This is the sleep clock speed (32.768 Khz) */
#define DSPS_SLEEP_CLK      32768 

/*---------------------------------------------------------------------------
* Type Declarations
* -------------------------------------------------------------------------*/

/**
 Defines the timer type 
*/
typedef enum {
  SNS_EM_TIMER_TYPE_PERIODIC, /**< Periodic timer */
  SNS_EM_TIMER_TYPE_ONESHOT   /**< One shot timer */
} sns_em_timer_type_e;


/**
 Defines the timer (event manager) object 
*/
typedef struct sns_em_tmr_s * sns_em_timer_obj_t;

/*---------------------------------------------------------------------------
* Function Declarations and Documentation
* -------------------------------------------------------------------------*/

/*=====================================================================================
  FUNCTION:  sns_em_get_timestamp
=====================================================================================*/
/**
This API can be used by modules in apps, modules or DSPS to read the local timer value.
It reads the underlying hardware timer register and returns the value in ticks.

@retval uint32_t (ticks)
*/
uint32_t sns_em_get_timestamp( void );

/*=====================================================================================
  FUNCTION:  sns_em_get_timestamp
=====================================================================================*/
/**
This API can be used by modules in apps, modules or DSPS to read the local timer value.
It reads the underlying hardware timer register and returns the value in ticks.

@param timestamp_p  [OUT] Pointer to a 64-bit number to place the timestamp.

@retval SNS_SUCCESS if the timestamp is set successfully.
*/
sns_err_code_e
sns_em_get_timestamp64( uint64_t *timestamp_p );

/*=====================================================================================
  FUNCTION:  sns_em_create_timer_obj
=====================================================================================*/
/**
This API can be used to create a timer object. A module can subsequently use the timer 
object to register and cancel timer requests. This API should be called by modules 
before registering for a timer callback.

@retval sns_err_code_e

@see sns_em_delete_timer_obj()

@param timer_cb       [IN]  Address of the timer callback            
@param timer_cb_arg   [IN]  Parameters to be passed onto the timer callback 
@param timer_type     [IN]  Type of timer (periodic or single-shot)
@param timer_obj_ptr  [OUT] Pointer to the the timer object 
*/
sns_err_code_e sns_em_create_timer_obj( void (*timer_cb)(void *), 
                                        void *timer_cb_arg, 
                                        sns_em_timer_type_e timer_type,
                                        sns_em_timer_obj_t* timer_obj_ptr );

/*=====================================================================================
  FUNCTION:  sns_em_delete_timer_obj
=====================================================================================*/
/**
This API is used to delete the timer object. The timer cannot be used after calling 
this function and this should only be called when the application has no more timers 
to register. 

This API will free any memory associated with the timer object.

@retval sns_err_code_e

@param timer_obj_ptr [IN] The Timer object

@see sns_em_create_timer_obj()
*/
sns_err_code_e sns_em_delete_timer_obj( sns_em_timer_obj_t timer_obj );

/*=====================================================================================
  FUNCTION:  sns_em_register_timer
=====================================================================================*/
/**
This API is used to initiate a timer service. A module which wishes to be notified 
after expiration of 'delta_tick_time' should call this API to register with the 
event manager.

If a timer is already registered a subsequent call to this API will overwrite the 
current registered value.

@retval sns_err_code_e

@param timer_obj_ptr    [IN] The timer object             
@param delta_tick_time  [IN] The time after which the client needs to be notified. 
                              (measured in ticks)

@see sns_em_cancel_timer()
*/
sns_err_code_e sns_em_register_timer( sns_em_timer_obj_t  timer_obj, 
                                      uint32_t            delta_tick_time );


/*=====================================================================================
  FUNCTION:  sns_em_cancel_timer
=====================================================================================*/
/**
This API is used to cancel the requested timer service. This will preempt the calling
module from being notified. If this function is called just when the timer interrupt 
happens the client callback will still get called.

@retval sns_err_code_e

@param timer_obj_ptr  [IN] The timer object

@see sns_em_register_timer()
@see sns_em_delete_all_timers()
*/
sns_err_code_e sns_em_cancel_timer( sns_em_timer_obj_t timer_obj );


/*=====================================================================================
  FUNCTION:  sns_em_cancel_all_timers
=====================================================================================*/
/**
This API is used to cancel all remaining timers. 

It will cancel all the pending timers and empty the timer queue. This might be used 
by the power management module to shut down the DSPS.

@retval sns_err_code_e
*/
sns_err_code_e sns_em_cancel_all_timers ( void );


/*=====================================================================================
  FUNCTION:  sns_em_check_timer_pending
=====================================================================================*/
/**
This API is used to retrieve the time pending for the next timer interrupt. The 
pending time will be retrieved in ticks.

@retval uint32_t (time in ticks)
*/
uint32_t sns_em_check_timer_pending( void );


/*=====================================================================================
  FUNCTION:  sns_em_convert_dspstick_to_usec
=====================================================================================*/
/**
This is a utility API which can be used to by modules to covert the DSPS ticks into 
microsecond units.

@retval uint32_t (time in microseconds)

@param dsps_tick  [IN]  Time in DSPS ticks             

@see sns_em_convert_usec_to_dspstick()
*/
uint32_t sns_em_convert_dspstick_to_usec( uint32_t dsps_tick );

/*=====================================================================================
  FUNCTION:  sns_em_convert_localtick_to_usec
=====================================================================================*/
/**
This is a utility API which can be used to by modules to covert the local tick unit into 
microsecond units.

@retval uint32_t (time in microseconds)

@param local_tick  [IN]  Time in local processor ticks

*/
uint32_t sns_em_convert_localtick_to_usec( uint32_t local_tick );


/*=====================================================================================
  FUNCTION:  sns_em_convert_usec_to_dspstick
=====================================================================================*/
/**
This is a utility API which can be used to by modules to covert the microsecond time 
into DSPS ticks.

@retval uint32

@param microsec_time  [IN]  Time in microseconds           

@see sns_em_convert_dspstick_to_usec()
*/
uint32_t sns_em_convert_usec_to_dspstick( uint32_t microsec_time );

/*=====================================================================================
  FUNCTION:  sns_em_convert_usec_to_localtick
=====================================================================================*/
/**
This is a utility API which can be used to by modules to covert the microsecond time 
into local processor ticks.

@retval uint32_t representing ticks in local processor units

@param microsec_time  [IN]  Time in microseconds           

@see sns_em_convert_dspstick_to_usec()
*/
uint32_t sns_em_convert_usec_to_localtick( uint32_t microsec_time );


/*=====================================================================================
  FUNCTION:  sns_em_convert_dspstick_to_appstime
=====================================================================================*/
/**
This is a utility API which can be used to by modules to covert the DSPS ticks into 
Apps processor ticks.

@retval uint32_t (time in apps tick)

@param dsps_tick  [IN] Time in DSPS ticks
*/
uint32_t sns_em_convert_dspstick_to_appstime( uint32_t dsps_tick );


/*=====================================================================================
  FUNCTION:  sns_em_convert_dspstick_to_gpstime
=====================================================================================*/
/**
This is a utility API which can be used to by modules to covert the DSPS ticks into 
GPS time.

@retval uint32_t (GPS time)

@param dsps_tick  [IN] Time in DSPS ticks
*/
uint32_t sns_em_convert_dspstick_to_gpstime( uint32_t dsps_tick );


/*=====================================================================================
  FUNCTION:  sns_em_convert_sec_in_q16_to_localtick
=====================================================================================*/
/**
This is a utility API which can be used by modules to covert the time in seconds 
in Q16 format into local processor ticks. 
 
@retval uint32_t representing ticks in local processor units

@param time_sec_q16  [IN]  Time in seconds in Q16 format           
*/
uint32_t sns_em_convert_sec_in_q16_to_localtick( uint32_t time_sec_q16 );


#endif /*#ifndef _SNS_EM_H*/
