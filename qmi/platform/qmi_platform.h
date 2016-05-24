
#ifndef QMI_PLATFORM_H
#define QMI_PLATFORM_H

/******************************************************************************
  @file    qmi_platform.h
  @brief   The QMI QMUX generic platform layer hearder file

  DESCRIPTION
  Interface definition for QMI QMUX platform layer.  This file will pull in
  the appropriate platform header file.

  INITIALIZATION AND SEQUENCING REQUIREMENTS
  None

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2010, 2013 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------
******************************************************************************/

/* Turn on/off MULTI-PD feature here */
#define QMI_MSGLIB_MULTI_PD 

/* Turn debugging on/off */
#ifndef QMI_DEBUG
#define QMI_DEBUG
#endif

#include <stdio.h>
#include <pthread.h>
#include <memory.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>


#ifndef FALSE
#define FALSE 0
#endif

#ifdef FEATURE_DATA_LOG_ADB
#define LOG_NDEBUG 0
#define LOG_NIDEBUG 0
#define LOG_NDDEBUG 0
#include "comdef.h"
#include <utils/Log.h>
#include "common_log.h"
#define QMI_LOG_TAG "QC-QMI"
#endif

#ifdef FEATURE_DATA_LOG_QXDM
#include "comdef.h"
#include <msg.h>
#include <msgcfg.h>
#include <diag_lsm.h>
#include <log.h>
#include "common_log.h"
#endif

#if defined(FEATURE_DATA_LOG_QXDM)
extern boolean qmi_platform_qxdm_init;
#endif

#ifdef FEATURE_DATA_LOG_STDERR
/* Debug and error messages */
#define QMI_ERR_MSG_0(str)                       fprintf (stderr,str)
#define QMI_ERR_MSG_1(str,arg1)                  fprintf (stderr,str,arg1)
#define QMI_ERR_MSG_2(str,arg1,arg2)             fprintf (stderr,str,arg1,arg2)
#define QMI_ERR_MSG_3(str,arg1,arg2,arg3)        fprintf (stderr,str,arg1,arg2,arg3)
#define QMI_ERR_MSG_4(str,arg1,arg2,arg3,arg4)   fprintf (stderr,str,arg1,arg2,arg3,arg4)

#define QMI_DEBUG_MSG_0(str)                       fprintf (stdout,str)
#define QMI_DEBUG_MSG_1(str,arg1)                  fprintf (stdout,str,arg1)
#define QMI_DEBUG_MSG_2(str,arg1,arg2)             fprintf (stdout,str,arg1,arg2)
#define QMI_DEBUG_MSG_3(str,arg1,arg2,arg3)        fprintf (stdout,str,arg1,arg2,arg3)
#define QMI_DEBUG_MSG_4(str,arg1,arg2,arg3,arg4)   fprintf (stdout,str,arg1,arg2,arg3,arg4)
#endif

#ifdef FEATURE_DATA_LOG_FILE
extern FILE *qmuxd_fptr;
extern pthread_mutex_t qmux_file_log_mutex;

/* Debug and error messages */
#define QMI_ERR_MSG_0(str)                                             \
  QMI_PLATFORM_MUTEX_LOCK(&qmux_file_log_mutex);                       \
  if (qmuxd_fptr)                                                      \
  {                                                                    \
    fprintf (qmuxd_fptr,"%s| " str "\n",__FILE__);                     \
    fflush(qmuxd_fptr);                                                \
  }                                                                    \
  QMI_PLATFORM_MUTEX_UNLOCK(&qmux_file_log_mutex)

#define QMI_ERR_MSG_1(str, arg1)                                       \
  QMI_PLATFORM_MUTEX_LOCK(&qmux_file_log_mutex);                       \
  if (qmuxd_fptr)                                                      \
  {                                                                    \
    fprintf (qmuxd_fptr,"%s| " str "\n",__FILE__,arg1);                \
    fflush(qmuxd_fptr);                                                \
  }                                                                    \
  QMI_PLATFORM_MUTEX_UNLOCK(&qmux_file_log_mutex)

#define QMI_ERR_MSG_2(str,arg1,arg2)                                   \
  QMI_PLATFORM_MUTEX_LOCK(&qmux_file_log_mutex);                       \
  if (qmuxd_fptr)                                                      \
  {                                                                    \
    fprintf (qmuxd_fptr,"%s| " str "\n",__FILE__,arg1,arg2);           \
    fflush(qmuxd_fptr);                                                \
  }                                                                    \
  QMI_PLATFORM_MUTEX_UNLOCK(&qmux_file_log_mutex)

#define QMI_ERR_MSG_3(str,arg1,arg2,arg3)                              \
  QMI_PLATFORM_MUTEX_LOCK(&qmux_file_log_mutex);                       \
  if (qmuxd_fptr)                                                      \
  {                                                                    \
    fprintf (qmuxd_fptr,"%s| " str "\n",__FILE__,arg1,arg2,arg3);      \
    fflush(qmuxd_fptr);                                                \
  }                                                                    \
  QMI_PLATFORM_MUTEX_UNLOCK(&qmux_file_log_mutex)

#define QMI_ERR_MSG_4(str,arg1,arg2,arg3,arg4)                         \
  QMI_PLATFORM_MUTEX_LOCK(&qmux_file_log_mutex);                       \
  if (qmuxd_fptr)                                                      \
  {                                                                    \
    fprintf (qmuxd_fptr,"%s| " str "\n",__FILE__,arg1,arg2,arg3,arg4); \
    fflush(qmuxd_fptr);                                                \
  }                                                                    \
  QMI_PLATFORM_MUTEX_UNLOCK(&qmux_file_log_mutex)


#define QMI_DEBUG_MSG_0(str)                      QMI_ERR_MSG_0(str)
#define QMI_DEBUG_MSG_1(str,arg1)                 QMI_ERR_MSG_1(str,arg1)
#define QMI_DEBUG_MSG_2(str,arg1,arg2)            QMI_ERR_MSG_2(str,arg1,arg2)
#define QMI_DEBUG_MSG_3(str,arg1,arg2,arg3)       QMI_ERR_MSG_3(str,arg1,arg2,arg3)
#define QMI_DEBUG_MSG_4(str,arg1,arg2,arg3,arg4)  QMI_ERR_MSG_4(str,arg1,arg2,arg3,arg4)

#elif defined(FEATURE_DATA_LOG_ADB)

#define LOG_TAG  QMI_LOG_TAG

#define QMI_ERR_MSG_0(...)                        LOGE(__VA_ARGS__)
#define QMI_ERR_MSG_1(...)                        LOGE(__VA_ARGS__)
#define QMI_ERR_MSG_2(...)                        LOGE(__VA_ARGS__)
#define QMI_ERR_MSG_3(...)                        LOGE(__VA_ARGS__)
#define QMI_ERR_MSG_4(...)                        LOGE(__VA_ARGS__)

#define QMI_DEBUG_MSG_0(...)                      LOGD(__VA_ARGS__)
#define QMI_DEBUG_MSG_1(...)                      LOGD(__VA_ARGS__)
#define QMI_DEBUG_MSG_2(...)                      LOGD(__VA_ARGS__)
#define QMI_DEBUG_MSG_3(...)                      LOGD(__VA_ARGS__)
#define QMI_DEBUG_MSG_4(...)                      LOGD(__VA_ARGS__)

#elif defined(FEATURE_DATA_LOG_QXDM)
/*Logging to Diag*/

/* Maximum length of log message */
#define QMI_MAX_DIAG_LOG_MSG_SIZE      512

#ifdef FEATURE_QMI_ANDROID
/* Log message to Diag or fallback to ADB */
#define QMI_LOG_MSG_DIAG( lvl, ... )                                             \
  {                                                                              \
    char buf[ QMI_MAX_DIAG_LOG_MSG_SIZE ];                                       \
                                                                                 \
    /* Format message for logging */                                             \
    qmi_format_diag_log_msg( buf, QMI_MAX_DIAG_LOG_MSG_SIZE, __VA_ARGS__ );      \
                                                                                 \
    if (TRUE == qmi_platform_qxdm_init)                                          \
    {                                                                            \
      /* Log message to Diag */                                                  \
      MSG_SPRINTF_1( MSG_SSID_LINUX_DATA, lvl, "%s", buf );                      \
    }                                                                            \
    else                                                                         \
    {                                                                            \
      LOGE("%s", buf);                                                           \
    }                                                                            \
  }
#else
/* Log message to Diag */
#define QMI_LOG_MSG_DIAG( lvl, ... )                                             \
  {                                                                              \
    char buf[ QMI_MAX_DIAG_LOG_MSG_SIZE ];                                       \
                                                                                 \
    /* Format message for logging */                                             \
    qmi_format_diag_log_msg( buf, QMI_MAX_DIAG_LOG_MSG_SIZE, __VA_ARGS__ );      \
                                                                                 \
    if (TRUE == qmi_platform_qxdm_init)                                          \
    {                                                                            \
      /* Log message to Diag */                                                  \
      MSG_SPRINTF_1( MSG_SSID_LINUX_DATA, lvl, "%s", buf );                      \
    }                                                                            \
  }
#endif

#define QMI_ERR_MSG_0(...)                        QMI_LOG_MSG_DIAG(MSG_LEGACY_ERROR, __VA_ARGS__)
#define QMI_ERR_MSG_1(...)                        QMI_LOG_MSG_DIAG(MSG_LEGACY_ERROR, __VA_ARGS__)
#define QMI_ERR_MSG_2(...)                        QMI_LOG_MSG_DIAG(MSG_LEGACY_ERROR, __VA_ARGS__)
#define QMI_ERR_MSG_3(...)                        QMI_LOG_MSG_DIAG(MSG_LEGACY_ERROR, __VA_ARGS__)
#define QMI_ERR_MSG_4(...)                        QMI_LOG_MSG_DIAG(MSG_LEGACY_ERROR, __VA_ARGS__)

#define QMI_DEBUG_MSG_0(...)                      QMI_LOG_MSG_DIAG(MSG_LEGACY_HIGH,  __VA_ARGS__)
#define QMI_DEBUG_MSG_1(...)                      QMI_LOG_MSG_DIAG(MSG_LEGACY_HIGH,  __VA_ARGS__)
#define QMI_DEBUG_MSG_2(...)                      QMI_LOG_MSG_DIAG(MSG_LEGACY_HIGH,  __VA_ARGS__)
#define QMI_DEBUG_MSG_3(...)                      QMI_LOG_MSG_DIAG(MSG_LEGACY_HIGH,  __VA_ARGS__)
#define QMI_DEBUG_MSG_4(...)                      QMI_LOG_MSG_DIAG(MSG_LEGACY_HIGH,  __VA_ARGS__)

#endif


/* Define signal data type */
typedef struct
{
  unsigned long             cond_predicate;
  pthread_mutex_t           cond_mutex;
  pthread_cond_t            cond_var;
} qmi_linux_signal_data_type;


#define QMI_PLATFORM_SIGNAL_DATA_TYPE qmi_linux_signal_data_type

/* Macro to initialize signal data */
#define QMI_PLATFORM_INIT_SIGNAL_DATA(signal_ptr) \
do \
{ \
  pthread_mutex_init (&(signal_ptr)->cond_mutex,NULL); \
  pthread_cond_init (&(signal_ptr)->cond_var,NULL); \
} while (0)

/* Macro to destroy signal data */
#define QMI_PLATFORM_DESTROY_SIGNAL_DATA(signal_ptr) \
do \
{ \
  pthread_cond_destroy (&(signal_ptr)->cond_var); \
  pthread_mutex_destroy (&(signal_ptr)->cond_mutex); \
} while (0)


#define QMI_PLATFORM_INIT_SIGNAL_FOR_WAIT(conn_id,signal_ptr) \
do \
{ \
  pthread_mutex_lock (&(signal_ptr)->cond_mutex); \
  (signal_ptr)->cond_predicate = FALSE; \
} while (0)

extern int
qmi_linux_wait_for_sig_with_timeout
(
  qmi_linux_signal_data_type  *signal_ptr,
  int                         timeout_secs
);

#define QMI_PLATFORM_WAIT_FOR_SIGNAL(conn_id, signal_ptr, timeout_milli_secs) \
  qmi_linux_wait_for_sig_with_timeout (signal_ptr,timeout_milli_secs)


#define QMI_PLATFORM_SEND_SIGNAL(conn_id,signal_ptr) \
do \
{ \
  pthread_mutex_lock (&(signal_ptr)->cond_mutex); \
  (signal_ptr)->cond_predicate = TRUE; \
  pthread_cond_signal (&(signal_ptr)->cond_var); \
  pthread_mutex_unlock (&(signal_ptr)->cond_mutex); \
} while (0)



/* Mutex related defines */
#define QMI_PLATFORM_MUTEX_DATA_TYPE pthread_mutex_t

#define QMI_PLATFORM_MUTEX_CREATE_AND_INIT_STATIC_MUTEX(mutex) \
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER

#define QMI_PLATFORM_MUTEX_INIT(mutex_ptr) \
  pthread_mutex_init(mutex_ptr,NULL)

#define QMI_PLATFORM_MUTEX_INIT_RECURSIVE(mutex_ptr) \
  do \
  { \
      pthread_mutexattr_t _attr; \
      pthread_mutexattr_init (&_attr); \
      pthread_mutexattr_settype (&_attr, PTHREAD_MUTEX_RECURSIVE); \
      pthread_mutex_init(mutex_ptr, &_attr); \
      pthread_mutexattr_destroy (&_attr); \
  } while (0)



#define QMI_PLATFORM_MUTEX_DESTROY(mutex_ptr) \
  pthread_mutex_destroy (mutex_ptr)

#define QMI_PLATFORM_MUTEX_LOCK(mutex_ptr) \
  pthread_mutex_lock (mutex_ptr)

#define QMI_PLATFORM_MUTEX_TRY_LOCK(mutex_ptr) \
  pthread_mutex_trylock (mutex_ptr)

#define QMI_PLATFORM_MUTEX_UNLOCK(mutex_ptr) \
  pthread_mutex_unlock (mutex_ptr)


extern qmi_connection_id_type 
qmi_linux_get_conn_id_by_name 
(
  const char *dev_id
);

extern const char *
qmi_linux_get_name_by_conn_id
(
  qmi_connection_id_type conn_id
);


#define QMI_PLATFORM_DEV_NAME_TO_CONN_ID(dev_id) \
   qmi_linux_get_conn_id_by_name (dev_id)


#define QMI_PLATFORM_CONN_ID_TO_DEV_NAME(conn_id) \
   qmi_linux_get_name_by_conn_id (conn_id)

#ifdef FEATURE_DATA_LOG_QXDM
/*=========================================================================
  FUNCTION:  qmi_format_diag_log_msg

===========================================================================*/
/*!
    @brief
    Format debug message for logging.

    @return
    None
*/
/*=========================================================================*/
void qmi_format_diag_log_msg
(
  char *buf_ptr,
  int buf_size,
  char *fmt,
  ...
);
#endif

#endif  /* QMI_PLATFORM_H */
