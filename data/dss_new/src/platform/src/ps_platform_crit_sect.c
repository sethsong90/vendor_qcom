/*==========================================================================*/
/*!
  @file 
  ps_platform_crit_sect.c

  @brief
  This file provides Linux specific critical section implementation.

  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*==========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header:$
  $DateTime:$

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  05/03/2010 ar  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include <pthread.h>
#include <string.h>

#include "comdef.h"
#include "ds_util.h"
#include "ps_crit_sect.h"
#include "amssassert.h"


/*===========================================================================

                          PUBLIC FUNCTION DEFINITIONS

===========================================================================*/

/*---------------------------------------------------------------------------
  Linux specific critical section implementation
---------------------------------------------------------------------------*/
void ps_init_crit_section
(
  ps_crit_sect_type*  crit_sect_ptr
)
{
  pthread_mutex_t * mutex = NULL;
  pthread_mutexattr_t attr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /* Allocate dynamic memory for mutex; released in ps_destroy_crit_section */
  mutex = ds_malloc( sizeof(pthread_mutex_t) );
  if( NULL == mutex ) {
    MSG_ERROR( "Failure on ds_malloc(%d)",sizeof(pthread_mutex_t),0,0 );
    goto bail;
  }

  memset(&attr, 0, sizeof(pthread_mutexattr_t));

  if( 0 > pthread_mutexattr_init( &attr ) ) {
    MSG_ERROR( "pthread_mutexattr_init failed",0,0,0 );
    goto bail;
  }

#ifdef FEATURE_DATA_LINUX_LE
  if( 0 >  pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE_NP ) ) {
    MSG_ERROR( "pthread_mutexattr_settype failed",0,0,0 );
    goto bail;
  }
#else
  if( 0 >  pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE ) ) {
    MSG_ERROR( "pthread_mutexattr_settype failed",0,0,0 );
    goto bail;
  }
#endif

  if( 0 >  pthread_mutex_init( mutex, &attr ) ) {
    MSG_ERROR( "pthread_mutex_init failed",0,0,0 );
    goto bail;
  }

  crit_sect_ptr->handle = (void*)mutex;
  return;
  
 bail:
  if( mutex ) {
    ds_free( mutex );
  }
} /* ps_init_crit_section() */


void ps_enter_crit_section
(
  ps_crit_sect_type*  crit_sect_ptr
#ifdef FEATURE_DEBUG_CRIT_SECTS
  ,int                 line,
  const char         *filename  
#endif  
)
{
  int rc =0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  ds_assert(crit_sect_ptr);
#ifdef FEATURE_DEBUG_CRIT_SECTS
  (void)line; (void)filename;
#endif  
    
  if( 0 != (rc = pthread_mutex_lock( (pthread_mutex_t*)crit_sect_ptr->handle )) ) {
    MSG_ERROR( "pthread_mutex_lock failed",0,0,0 );
    ASSERT (0);
  }

} /* ps_enter_crit_section() */

void ps_leave_crit_section
(
  ps_crit_sect_type*  crit_sect_ptr
#ifdef FEATURE_DEBUG_CRIT_SECTS
  ,int                 line,
  const char         *filename  
#endif  
)
{
  int rc =0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  ds_assert(crit_sect_ptr);
#ifdef FEATURE_DEBUG_CRIT_SECTS
  (void)line; (void)filename;
#endif  

  if( 0 != (rc = pthread_mutex_unlock( (pthread_mutex_t*)crit_sect_ptr->handle )) ) {
    MSG_ERROR( "pthread_mutex_unlock failed",0,0,0 );
    ASSERT (0);
  }

} /* ps_leave_crit_section() */

void ps_destroy_crit_section
(
  ps_crit_sect_type*  crit_sect_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  ds_assert(crit_sect_ptr);

  /* Release dynamic memory for mutex */
  ds_free( crit_sect_ptr->handle );

} /* ps_destroy_crit_section() */


