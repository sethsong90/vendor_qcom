#ifndef DSM_LOCK_H
#define DSM_LOCK_H
/*===========================================================================

                             D S M _ L O C K. H

DESCRIPTION
  This file contains declarations for generalized locking functions in use
  in the DSM libraries.

-----------------------------------------------------------------------------
Copyright (c) 2007 Qualcomm Technologies, Inc.
All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
-----------------------------------------------------------------------------
===========================================================================*/


/*===========================================================================
                            EDIT HISTORY FOR FILE
                                      
  $Header: //linux/pkgs/proprietary/oncrpc/main/source/dsm/dsm_lock.h#3 $

===========================================================================*/

#include "comdef.h"
#include "customer.h"


#if defined FEATURE_DSM_WINCE
#include <windows.h>

#elif defined FEATURE_DSM_QUBE
#include "qmutex.h"

#elif defined FEATURE_DSM_NATIVE_LINUX
#include <pthread.h>

#else
#include "rex.h"
#endif /* FEATURE_DSM_WINCE etc. */



/*===========================================================================
                        DATA DECLARATIONS
===========================================================================*/
#ifdef FEATURE_DSM_WINCE
typedef HANDLE dsm_lock_type;

#elif defined FEATURE_DSM_QUBE
typedef qmutex_t dsm_lock_type;

#elif defined FEATURE_DSM_NATIVE_LINUX
#define  dsm_lock_type pthread_mutex_t

#else /*  defaulting to AMSS */
typedef dword dsm_lock_type;

#endif /* FEATURE_DSM_WINCE */

#if defined FEATURE_DSM_WINCE || \
    defined FEATURE_DSM_QUBE  || \
    defined FEATURE_DSM_NATIVE_LINUX

#define DSM_LOCK(lock) dsm_lock(lock)
#define DSM_UNLOCK(lock) dsm_unlock(lock)
#define DSM_LOCK_CREATE(lock) dsm_lock_create(lock)
#define DSM_LOCK_DESTROY(lock) dsm_lock_destroy(lock)

#elif defined FEATURE_L4

#define DSM_LOCK(lock)   INTLOCK()
#define DSM_UNLOCK(lock) INTFREE()
#define DSM_LOCK_CREATE(lock)
#define DSM_LOCK_DESTROY(lock)

#else

#define DSM_LOCK(lock)   INTLOCK();  TASKLOCK()
#define DSM_UNLOCK(lock) TASKFREE(); INTFREE()
#define DSM_LOCK_CREATE(lock)
#define DSM_LOCK_DESTROY(lock)

#endif  /* FEATURE_DSM_WINCE */


/*===========================================================================
                      FUNCTION DECLARATIONS
===========================================================================*/


/*===========================================================================
FUNCTION DSM_LOCK

DESCRIPTION
   This function acquires exclusive access to a critical resource.

DEPENDENCIES
 If FEATURE_DSM_WINCE || FEATURE_DSM_QUBE || FEATURE_DSM_NATIVE_LINUX
   The lock must have been initialized with dsm_create_lock.

 In all cases:  
   Must not be called from interrupt context.

PARAMETERS
   lock_ptr - Pointer to lock cookie

RETURN VALUE
   None

SIDE EFFECTS
   The current thread may be blocked until the lock is available.
   Depending on the OS, the lock may get a value by reference.
===========================================================================*/
extern void dsm_lock
(
  dsm_lock_type * lock_ptr
);



/*===========================================================================
FUNCTION DSM_UNLOCK

DESCRIPTION
   This function acquires exclusive access to a critical resource.

DEPENDENCIES
 If  FEATURE_DSM_WINCE || FEATURE_DSM_QUBE || FEATURE_DSM_NATIVE_LINUX
   The parameter must NOT be NULL

 In all cases:  
   Must not be called from interrupt context.

PARAMETERS
   lock_ptr -  Pointer to lock cookie

RETURN VALUE
   None

SIDE EFFECTS
   The current thread will become unlocked

===========================================================================*/
extern void dsm_unlock
(
   dsm_lock_type * lock_ptr
);



/*===========================================================================
FUNCTION DSM_LOCK_CREATE

DESCRIPTION
   This function initializes a locking mechanism.

DEPENDENCIES
   The parameter must NOT be NULL in the case of
   FEATURE_DSM_WINCE || FEATURE_DSM_QUBE || FEATURE_DSM_NATIVE_LINUX

 In all cases:  
   Must not be called from interrupt context.

PARAMETERS
   lock_ptr - Pointer to lock type.

RETURN VALUE
   None

SIDE EFFECTS
   The current thread will become unlocked

===========================================================================*/
void dsm_lock_create
(
   dsm_lock_type * lock_ptr
);

/*===========================================================================
FUNCTION DSM_LOCK_DESTROY

DESCRIPTION
   This function tears down a locking mechanism.

DEPENDENCIES
   The parameter must NOT be NULL in the case of
   FEATURE_DSM_WINCE || FEATURE_DSM_QUBE || FEATURE_DSM_NATIVE_LINUX

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
);

#endif /* DSM_LOCK_H */
