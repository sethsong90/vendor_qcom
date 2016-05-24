/*===========================================================================

                   D S M  R E X 

  This contains rex support for DSM in LINUX.

 Copyright (c) 2007 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential.
  
===========================================================================*/

/*===========================================================================

                           EDIT HISTORY FOR FILE

$Header: //linux/pkgs/proprietary/oncrpc/main/source/dsm/dsm_rex.c#4 $ $DateTime: 2008/07/17 20:50:08 $ $Author: rruigrok $
 
when       who     what, where, why
--------   ---     ----------------------------------------------------------
07/17/08    rr     Add Android support
01/30/08   rr      Initial version, INTLOCK and INTFREE Support
===========================================================================*/
#include <pthread.h>
#include "err.h"
#include "comdef.h"




extern int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);

static pthread_mutex_t oncrpc_rex_intlock_mutex;
static volatile int    oncrpc_rex_intlock_mutex_initialized = 0;
static pthread_mutex_t dsm_rex_intlock_mutex_init_mutex = PTHREAD_MUTEX_INITIALIZER;

/*===========================================================================
  FUNCTION: rex_intlock_init

  DESCRIPTION:
  Initialization of pthread mutex used for rex_intlock

  RESULT: N/A
  
 ===========================================================================*/
static void rex_intlock_init(void)
{
   pthread_mutexattr_t attr;
   int                 rc;

   rc = pthread_mutexattr_init(&attr);
   if (rc)
   {
      ERR_FATAL("failed to initialize rex_intlock mutex attribute: %d\n",
                rc, 0, 0);
   }

#ifdef FEATURE_ANDROID
   rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
#else 
   rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
#endif
   if (rc)
   {
      ERR_FATAL("failed to adjust rex_intlock mutex attribute: %d\n", rc, 0, 0);
   }

   rc = pthread_mutex_lock(&dsm_rex_intlock_mutex_init_mutex);
   if (rc)
   {
      ERR_FATAL("failed to lock rex_intlock mutex initialization mutex: %d\n",
                rc, 0, 0);
   }

   if (!oncrpc_rex_intlock_mutex_initialized)
   {
      rc = pthread_mutex_init(&oncrpc_rex_intlock_mutex, &attr);
      if (rc)
      {
         ERR_FATAL("failed to initialize rex_intlock mutex: %d\n", rc, 0, 0);
      }

      oncrpc_rex_intlock_mutex_initialized = 1;
   }

   rc = pthread_mutex_unlock(&dsm_rex_intlock_mutex_init_mutex);
   if (rc)
   {
      ERR_FATAL("failed to unlock rex_intlock mutex initialization mutex: %d\n",
                rc, 0, 0);
   }
}

/*===========================================================================
  FUNCTION: rex_int_free

  DESCRIPTION:
  Pthreads implementation of rex_int_free.

  RESULT: 0
  
===========================================================================*/
uint32 rex_int_free(void)
{
   int rc;

   if (!oncrpc_rex_intlock_mutex_initialized)
   {
      ERR_FATAL("rex_intlock mutex has not been initialized\n", 0, 0, 0);
      /* which also means rex_int_lock has never been called */
   }

   rc = pthread_mutex_unlock(&oncrpc_rex_intlock_mutex);
   if (rc)
   {
      ERR_FATAL("failed to unlock rex_intlock mutex: %d\n", rc, 0, 0);
   }

   return 0;
}

/*===========================================================================
  FUNCTION: rex_int_lock

  DESCRIPTION:
  Pthreads implementation of rex_int_lock.

  RESULT: 0
  
===========================================================================*/
uint32 rex_int_lock(void)
{
   int rc;

   if (!oncrpc_rex_intlock_mutex_initialized)
   {
      rex_intlock_init();
   }

   rc = pthread_mutex_lock(&oncrpc_rex_intlock_mutex);
   if (rc)
   {
      ERR_FATAL("failed to lock rex_intlock mutex: %d\n", rc, 0, 0);
   }

   return 0;
}


