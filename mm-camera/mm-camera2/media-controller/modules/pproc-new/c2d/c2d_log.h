/*============================================================================

  Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

============================================================================*/
#ifndef C2D_LOG_H
#define C2D_LOG_H

#include "camera_dbg.h"

#define C2D_LOG_SILENT   0
#define C2D_LOG_NORMAL   1
#define C2D_LOG_DEBUG    2
#define C2D_LOG_VERBOSE  3

/* ------------- change this macro to change the logging level -------------*/
#define C2D_LOG_LEVEL    1

/* ------- change this macro to enable mutex debugging for deadlock --------*/
#define C2D_DEBUG_MUTEX  0

/* -------------------------------------------------------------------------*/

#undef CDBG_LOW
#define CDBG_LOW(fmt, args...) do {} while(0)

#if (C2D_LOG_LEVEL == C2D_LOG_SILENT)
  #undef CDBG_HIGH
  #define CDBG_HIGH CDBG
#elif (C2D_LOG_LEVEL == C2D_LOG_NORMAL)
  #undef CDBG_HIGH
  #define CDBG_HIGH CDBG_ERROR
#elif (C2D_LOG_LEVEL == C2D_LOG_DEBUG)
  #undef CDBG_HIGH
  #define CDBG_HIGH CDBG_ERROR
  #undef CDBG
  #define CDBG CDBG_ERROR
#elif (C2D_LOG_LEVEL == C2D_LOG_VERBOSE)
  #undef CDBG_HIGH
  #define CDBG_HIGH CDBG_ERROR
  #undef CDBG
  #define CDBG CDBG_ERROR
  #undef CDBG_LOW
  #define CDBG_LOW CDBG_ERROR
#endif

#undef PTHREAD_MUTEX_LOCK
#undef PTHREAD_MUTEX_UNLOCK

#if (C2D_DEBUG_MUTEX == 1)
  #define PTHREAD_MUTEX_LOCK(m) do { \
    CDBG_HIGH("%s:%d [c2d_mutex_log] before pthread_mutex_lock(%p)\n", \
      __func__, __LINE__, m); \
    pthread_mutex_lock(m); \
    CDBG_HIGH("%s:%d [c2d_mutex_log] after pthread_mutex_lock(%p)\n", \
      __func__, __LINE__, m); \
  } while(0)

  #define PTHREAD_MUTEX_UNLOCK(m) do { \
    CDBG_HIGH("%s:%d [c2d_mutex_log] before pthread_mutex_unlock(%p)\n", \
      __func__, __LINE__, m); \
    pthread_mutex_unlock(m); \
    CDBG_HIGH("%s:%d [c2d_mutex_log] after pthread_mutex_unlock(%p)\n", \
      __func__, __LINE__, m); \
  } while(0)
#else
  #define PTHREAD_MUTEX_LOCK(m)   pthread_mutex_lock(m)
  #define PTHREAD_MUTEX_UNLOCK(m) pthread_mutex_unlock(m)
#endif

#endif
