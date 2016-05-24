/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                                  D S M _ L O C K . C

GENERAL DESCRIPTION
  DMSS Data Services generalized locking functions.

EXTERNALIZED FUNCTIONS

  dsm_lock()
    Acquire exclusive access to critical resources.

  dsm_unlock()
    Release exclusive access to critical resoruces.

  dsm_lock_create()
    Create a locking mechanism.

  dsm_lock_destroy()
    Release a lock resource.
    
    
INITIALIZATION AND SEQUENCING REQUIREMENTS

  All WINCE locks MUST be initialized before using.
  ALL WINCE locks _should_ be destroyed after they are no longer needed.

-----------------------------------------------------------------------------
Copyright (c) 2007 Qualcomm Technologies, Inc.
All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
-----------------------------------------------------------------------------
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //linux/pkgs/proprietary/oncrpc/main/source/dsm/dsm_lock.c#3 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
09/06/07    mjb    Cleaning up comments and privatizing lock features
01/24/07    mjb    Generalized. Specific macros in dsm_pool.h & dsm_queue.c
12/27/06    rsb    Created file.
===========================================================================*/


/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
/* Target-independent Include files */
#include "comdef.h"
#include "customer.h"
#include "dsm_lock.h"
#include "err.h"
#include "msg.h"
#include "assert.h"
#ifdef FEATURE_DSM_QUBE
#include "qerror.h"
#endif


/*===========================================================================

                DEFINITIONS AND DECLARATIONS FOR MODULE

===========================================================================*/

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                           EXTERNALIZED FUNCTIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/*===========================================================================
FUNCTION DSM_LOCK

DESCRIPTION
   This function acquires exclusive access to a critical resource.

DEPENDENCIES
 If FEATURE_DSM_WINCE || FEATURE_DSM_QUBE || FEATURE_DSM_NATIVE_LINUX
   The parameter must NOT be NULL
   The lock must have been initialized with dsm_create_lock.

 In all cases:  
   Must not be called from interrupt context.

   lock_ptr  - Pointer to lock cookie

RETURN VALUE
   None

SIDE EFFECTS
   The current thread may be blocked until the lock is available.
   Depending on the OS, the lock may get a value by reference.
===========================================================================*/
void dsm_lock
(
   dsm_lock_type * lock_ptr
)
{

  ASSERT( lock_ptr != NULL );

#ifdef FEATURE_DSM_WINCE
  if ( WaitForSingleObject( (HANDLE)*lock_ptr,INFINITE ) != WAIT_OBJECT_0 )
   {
     ERR_FATAL( "dsm_lock: Failed trying to lock %x",
                (int)*lock_ptr, 0, 0 );
   }

#elif defined FEATURE_DSM_QUBE
  qmutex_lock( (qmutex_t)*lock_ptr);

#elif defined FEATURE_DSM_NATIVE_LINUX

  if( pthread_mutex_lock(lock_ptr) != 0 )
  {
    ERR_FATAL("failed on pthread_mutex_lock",0,0,0);
  } 

#endif /* FEATURE_DSM_WINCE etc. */

   return;
}

/*===========================================================================
FUNCTION DSM_UNLOCK

DESCRIPTION
   This function acquires exclusive access to a critical resource.

DEPENDENCIES
 If FEATURE_DSM_WINCE || FEATURE_DSM_QUBE || FEATURE_DSM_NATIVE_LINUX
   The parameter must NOT be NULL
   The queue is previously initialized using dsm_queue_init.

 In all cases:  
   Must not be called from interrupt context.

PARAMETERS
   lock_ptr - Pointer lock cookie

RETURN VALUE
   None

SIDE EFFECTS
   The current thread will become unlocked

===========================================================================*/
void dsm_unlock
(
   dsm_lock_type * lock_ptr
)
{
  ASSERT( lock_ptr != NULL );
#ifdef FEATURE_DSM_WINCE
  if ( ReleaseMutex( *lock_ptr ) == FALSE )
  {
    ERR_FATAL( "dsm_unlock: Failed trying to unlock %x",
               (int)*lock_ptr, 0, 0 );
  }
   
#elif defined FEATURE_DSM_QUBE
  qmutex_unlock( (qmutex_t)*lock_ptr);

#elif defined FEATURE_DSM_NATIVE_LINUX
  if(pthread_mutex_unlock(lock_ptr)!=0)
  {
    ERR_FATAL("failed on mutex_unlock",0,0,0);
  }

#endif /* FEATURE_DSM_WINCE etc. */

  return;
}


/*===========================================================================
FUNCTION DSM_LOCK_CREATE

DESCRIPTION
   This function initializes a locking mechanism.

DEPENDENCIES
 If FEATURE_DSM_WINCE  || FEATURE_DSM_QUBE || FEATURE_DSM_NATIVE_LINUX
   The parameter must NOT be NULL

PARAMETERS
   lock_ptr - Pointer to lock type.

RETURN VALUE
   None

SIDE EFFECTS
   A lock resource might be allocated in the OS, depending on the OS.
   The lock cookie will be put in the lock_ptr by reference.

===========================================================================*/
void dsm_lock_create
(
   dsm_lock_type * lock_ptr
)
{
  ASSERT( lock_ptr != NULL );

#ifdef FEATURE_DSM_WINCE
  *lock_ptr = (dsm_lock_type)CreateMutex( NULL, FALSE, NULL );
  if ( *lock_ptr == 0 )
  {
     ERR_FATAL("dsm_lock_create: Failed creating mutex.",0,0,0);
  }

#elif defined FEATURE_DSM_QUBE
  if ( qmutex_create ( lock_ptr, QMUTEX_LOCAL ) != EOK)
  {
    ERR_FATAL("dsm_lock_create: Failed creating qmutex.",0,0,0);
  }

#elif defined FEATURE_DSM_NATIVE_LINUX
   pthread_mutexattr_t attr;
   
   if( pthread_mutexattr_init(&attr) !=0 )
   {
     ERR_FATAL("failed on pthread_mutexattr_init",0,0,0);
   }
   if( pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) !=0 )
   {
     ERR_FATAL("failed on pthread_mutexattr_settype",0,0,0);
   }
   if( pthread_mutex_init(lock_ptr, &attr) != 0 )
   {
     ERR_FATAL("failed on thread_mutex_init",0,0,0);
   }
   if(pthread_mutexattr_destroy(&attr)!=0)
   {
     MSG_HIGH("WARNING: Potential memory leak. Could not destroy pthread_mutex_attr",0,0,0);
   }

#endif /* FEATURE_DSM_WINCE etc. */

  
   return;
}

/*===========================================================================
FUNCTION DSM_LOCK_DESTROY

DESCRIPTION
   This function tears down a locking mechanism.

DEPENDENCIES
 If FEATURE_WINCE
   The parameter must NOT be NULL
   The lock should be in existence.
   
PARAMETERS
   lock_ptr - Pointer to lock type.

RETURN VALUE
   None

SIDE EFFECTS
   The locking mechanism will cease to exist and OS resources might be
   freed. 

===========================================================================*/
void dsm_lock_destroy
(
   dsm_lock_type * lock_ptr
)
{

  ASSERT( lock_ptr != NULL );
#ifdef FEATURE_DSM_WINCE
  if( CloseHandle( *lock_ptr ) == FALSE )
  {
     ERR_FATAL("dsm_lock_destroy: Failed to close mutex 0x%x",*lock_ptr,0,0);
  }

#elif defined FEATURE_DSM_QUBE
  qmutex_delete(*lock_ptr);
  *lock_ptr = (qmutex_t)(0);

#elif defined FEATURE_DSM_NATIVE_LINUX
  pthread_mutex_destroy(lock_ptr);

#endif /* FEATURE_DSM_WINCE etc. */

   return;
}

