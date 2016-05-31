/******************************************************************************

                        D S _ U T I L . H

******************************************************************************/

/******************************************************************************

  @file    ds_util.h
  @brief   Data Services Utility Functions Header File

  DESCRIPTION
  Header file for DS utility functions.

  ---------------------------------------------------------------------------
  Copyright (c) 2008,2010-2013 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: ds_util.h,v 1.3 2010/02/19 22:33:22 randrew Exp $

when       who        what, where, why
--------   ---        -------------------------------------------------------
04/04/08   vk         Added support for client control of logging in dss
04/01/08   vk         Initial version

******************************************************************************/

#ifndef __DS_UTIL_H__
#define __DS_UTIL_H__

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "comdef.h"

#ifdef FEATURE_DATA_LOG_ADB
#include <utils/Log.h>
#endif

#ifdef FEATURE_DATA_LOG_QXDM
#include <msg.h>
#include <msgcfg.h>
#include <diag_lsm.h>
#include <log.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
                     GLOBAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/*---------------------------------------------------------------------------
   Some general utility macros.
---------------------------------------------------------------------------*/
#ifndef MIN
#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b)    ((a) > (b) ? (a) : (b))
#endif

/*---------------------------------------------------------------------------
   Log file pointer. All log messages are written to this file.
   This variable is not meant to be used directly by clients.
---------------------------------------------------------------------------*/
extern FILE * ds_logFp;

/*---------------------------------------------------------------------------
   Definition of 'assert' macro. This is needed as ONCRPC/DSM hijacks path
   to include another file with name assert.h, so that the standard library
   assert is not available to ONCRPC clients.
---------------------------------------------------------------------------*/
/*
#ifndef FEATURE_DS_LINUX_NO_RPC
#ifndef NDEBUG
#define assert(a)                                               \
        if (!(a)) {                                             \
            fprintf(stderr, "%s, %d: assertion (a) failed!",    \
                    __FILE__,                                   \
                    __LINE__);\
            abort();                                            \
        }
#else
#define assert(a) if (!(a));
#endif
#endif
*/

#define ds_abort()  abort()

#define ds_assert(a)                                            \
        if (!(a)) {                                             \
            fprintf(stderr, "%s, %d: assertion (a) failed!",    \
                    __FILE__,                                   \
                    __LINE__);                                  \
            ds_abort();                                         \
        }

#ifdef FEATURE_DATA_LINUX_LE
#undef ASSERT
#define ASSERT(ex) ds_assert(ex)
#endif
/*---------------------------------------------------------------------------
   Macro for obtaining the cardinality of an array
---------------------------------------------------------------------------*/
#define ds_arrsize(a) (sizeof(a)/sizeof(a[0]))


/*---------------------------------------------------------------------------
   Type representing program's runtime debug level
---------------------------------------------------------------------------*/
typedef enum {
    DS_DBG_LEVEL_MIN    = 0,
    DS_DBG_LEVEL_LOW    = 0,
    DS_DBG_LEVEL_MEDIUM = 1,
    DS_DBG_LEVEL_HIGH   = 2,
    DS_DBG_LEVEL_ERROR  = 3,
    DS_DBG_LEVEL_DFLT   = DS_DBG_LEVEL_ERROR,
    DS_DBG_LEVEL_MAX    = 3
} ds_dbg_level_t;

/*---------------------------------------------------------------------------
   Type representing program's logging mode
---------------------------------------------------------------------------*/
typedef enum {
    DS_LOG_MODE_MIN     = 0,
    DS_LOG_MODE_DFLT    = 0,
    DS_LOG_MODE_FPRINTF = DS_LOG_MODE_DFLT,
    DS_LOG_MODE_SYSLOG  = 1,
    DS_LOG_MODE_MAX     = 1
} ds_log_mode_t;

/*---------------------------------------------------------------------------
   Type representing program's logging configuration
---------------------------------------------------------------------------*/
typedef struct {
    ds_dbg_level_t threshold; /* level at which to print */
    ds_log_mode_t  mode;      /* log method to use */
} ds_log_cfg_t;

/*---------------------------------------------------------------------------
   Macro to get log file ptr
---------------------------------------------------------------------------*/
#define ds_get_logfp() (ds_logFp)

/*---------------------------------------------------------------------------
   Helper macro for logging formatted string at specified log level
---------------------------------------------------------------------------*/
#ifdef FEATURE_DATA_LOG_SYSLOG

#define ds_log(a, ...) ds_log_write(a, __LINE__, __VA_ARGS__)

/*---------------------------------------------------------------------------
   Helper macro for logging formatted string at default log level
---------------------------------------------------------------------------*/
#define ds_log_dflt(...) ds_log_write(DS_DBG_LEVEL_DFLT, __LINE__, __VA_ARGS__)

/*---------------------------------------------------------------------------
   Helper macro for logging formatted string at "error" log level
---------------------------------------------------------------------------*/
#define ds_log_err(...) ds_log_write(DS_DBG_LEVEL_ERROR, __LINE__, __VA_ARGS__)

/*---------------------------------------------------------------------------
   Helper macro for logging formatted string at "high" log level
---------------------------------------------------------------------------*/
#define ds_log_high(...) ds_log_write(DS_DBG_LEVEL_HIGH,__LINE__, __VA_ARGS__)

/*---------------------------------------------------------------------------
   Helper macro for logging formatted string at "med" log level
---------------------------------------------------------------------------*/
#define ds_log_med(...) ds_log_write(DS_DBG_LEVEL_MEDIUM,__LINE__, __VA_ARGS__)

/*---------------------------------------------------------------------------
   Helper macro for logging formatted string at "low" log level
---------------------------------------------------------------------------*/
#define ds_log_low(...) ds_log_write(DS_DBG_LEVEL_LOW, __LINE__, __VA_ARGS__)

/*---------------------------------------------------------------------------
   Helper macro for logging formatted string at "error" log level, followed
   by the system error message (as obtained using std library's strerror())
---------------------------------------------------------------------------*/
#define ds_log_sys_err(a) \
        ds_log_write(DS_DBG_LEVEL_ERROR,__LINE__, a" (%d)%s", errno, strerror(errno))

#endif /* FEATURE_DATA_LOG_SYSLOG */


#ifdef FEATURE_DATA_LOG_ADB
#ifndef DS_LOG_TAG
#define DS_LOG_TAG "QC-DS-LIB"
#endif

#define ds_log(...) LOG(LOG_INFO, DS_LOG_TAG, __VA_ARGS__)

#define ds_log_dflt(...) LOG(LOG_ERROR, DS_LOG_TAG, __VA_ARGS__)

#define ds_log_err(...) LOG(LOG_ERROR, DS_LOG_TAG, __VA_ARGS__)

#define ds_log_high(...) LOG(LOG_INFO, DS_LOG_TAG, __VA_ARGS__)

#define ds_log_med(...) LOG(LOG_INFO, DS_LOG_TAG, __VA_ARGS__)

#define ds_log_low(...) LOG(LOG_DEBUG, DS_LOG_TAG, __VA_ARGS__)

#define ds_log_sys_err(a) LOG(LOG_ERROR, DS_LOG_TAG, a " (%d)%s", errno, strerror(errno) )

#else /* FEATURE_DATA_LOG_ADB */
#undef LOG
#define LOG(...)
#define LOG_ERROR
#undef  DS_LOG_TAG
#define DS_LOG_TAG

#endif /* FEATURE_DATA_LOG_ADB */


#ifdef FEATURE_DATA_LOG_QXDM

/* Maximum length of log message */
#define DS_MAX_DIAG_LOG_MSG_SIZE      512

/* Log message to Diag */
#define DS_LOG_MSG_DIAG( lvl, ... )                                              \
  {                                                                              \
    char buf[ DS_MAX_DIAG_LOG_MSG_SIZE ];                                        \
                                                                                 \
    /* Format message for logging */                                             \
    ds_format_log_msg( buf, DS_MAX_DIAG_LOG_MSG_SIZE, __VA_ARGS__ );             \
                                                                                 \
    /* Log message to Diag */                                                    \
    MSG_SPRINTF_1( MSG_SSID_LINUX_DATA, lvl, "%s", buf );                        \
  }

#define ds_log(...) DS_LOG_MSG_DIAG(MSG_LEGACY_HIGH, __VA_ARGS__)

#define ds_log_dflt(...) DS_LOG_MSG_DIAG(MSG_LEGACY_HIGH, __VA_ARGS__)

#define ds_log_err(...) DS_LOG_MSG_DIAG(MSG_LEGACY_ERROR, __VA_ARGS__)

#define ds_log_high(...) DS_LOG_MSG_DIAG(MSG_LEGACY_HIGH, __VA_ARGS__)

#define ds_log_med(...) DS_LOG_MSG_DIAG(MSG_LEGACY_MED, __VA_ARGS__)

#define ds_log_low(...) DS_LOG_MSG_DIAG(MSG_LEGACY_LOW, __VA_ARGS__)

#define ds_log_sys_err(a) DS_LOG_MSG_DIAG(MSG_LEGACY_ERROR,  a " (%d)%s", errno, strerror(errno))

#else /* FEATURE_DATA_LOG_QXDM */

#define MSG_SPRINTF_1(...)
#define MSG_SSID_LINUX_DATA
#define MSG_LEGACY_LOW

#endif /* FEATURE_DATA_LOG_QXDM */

/*---------------------------------------------------------------------------
   Default message macros
---------------------------------------------------------------------------*/
#ifdef FEATURE_DATA_LOG_STDERR

#define ds_log(...) \
  fprintf( stderr, "%s %d:", __FILE__, __LINE__); \
  fprintf( stderr, __VA_ARGS__ )

#define ds_log_dflt(...) \
  fprintf( stderr, "%s %d:", __FILE__, __LINE__); \
  fprintf( stderr, __VA_ARGS__ )

#define ds_log_err(...) \
  fprintf( stderr, "%s %d:", __FILE__, __LINE__); \
  fprintf( stderr, __VA_ARGS__ )

#define ds_log_high(...)    \
  fprintf( stderr, "%s %d:", __FILE__, __LINE__); \
  fprintf( stderr, __VA_ARGS__ )

#define ds_log_med(...) \
  fprintf( stderr, "%s %d:", __FILE__, __LINE__); \
  fprintf( stderr, __VA_ARGS__ )

#define ds_log_low(...)     \
  fprintf( stderr, "%s %d:", __FILE__, __LINE__); \
  fprintf( stderr, __VA_ARGS__ )

#define ds_log_sys_err(a) \
  fprintf( stderr, "%s %d:", __FILE__, __LINE__); \
  fprintf( stderr, a " (%d)%s\n", errno, strerror(errno))

#endif /* FEATURE_DATA_LOG_STDERR */

/*---------------------------------------------------------------------------
   Helper macro for logging function entry at low log level
---------------------------------------------------------------------------*/
#define ds_log_func_entry()        \
    ds_log_low                     \
    (                              \
        "Entering function %s\n",  \
        __FUNCTION__               \
    )

/*---------------------------------------------------------------------------
   Helper macro for logging function exit at low log level
---------------------------------------------------------------------------*/
#define ds_log_func_exit()         \
    ds_log_low                     \
    (                              \
        "Exiting function %s\n",   \
        __FUNCTION__               \
    )

#define DS_INET4_NTOP(level,prefix,data)                                              \
  ds_log_##level(prefix "IPv4 addr [%d.%d.%d.%d]",                                    \
               data[0], data[1], data[2], data[3])

#define DS_INET6_NTOP(level,prefix,data)                                              \
  ds_log_##level(prefix "IPv6 addr [%.2x%.2x:%.2x%.2x:%.2x%.2x:%.2x%.2x:"             \
                      "%.2x%.2x:%.2x%.2x:%.2x%.2x:%.2x%.2x]",                         \
               data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],       \
               data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15])


#define DS_LOG_IPV4_ADDR(level, prefix, ip_addr)                            \
        DS_INET4_NTOP(level, prefix, ((unsigned char*)&ip_addr))

#define DS_LOG_IPV6_ADDR(level, prefix, ip_addr)                            \
        DS_INET6_NTOP(level, prefix, ((unsigned char*)&ip_addr))

#define DS_LOG_MULTICAST_LOW(fmt, ...) \
  ds_log_multicast(DS_DBG_LEVEL_LOW, fmt, __VA_ARGS__)
#define DS_LOG_MULTICAST_MED(fmt, ...) \
  ds_log_multicast(DS_DBG_LEVEL_MEDIUM, fmt, __VA_ARGS__)
#define DS_LOG_MULTICAST_HIGH(fmt, ...) \
  ds_log_multicast(DS_DBG_LEVEL_HIGH, fmt, __VA_ARGS__)
#define DS_LOG_MULTICAST_ERR(fmt, ...) \
  ds_log_multicast(DS_DBG_LEVEL_ERROR, fmt, __VA_ARGS__)

/*===========================================================================
                            GLOBAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  ds_log_init
===========================================================================*/
/*!
@brief
  Initializes logging to use the specified log file.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void ds_log_init(FILE * logfp);

/*===========================================================================
  FUNCTION  ds_log_multicast_init
===========================================================================*/
/*!
@brief
  Initializes logging to use Android property persist.net.logmask to enable
  logging for various output streams. This function will read the property
  to set logmask bits.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - logmask bits are updated
*/
/*=========================================================================*/
void ds_log_multicast_init(void);

/*=========================================================================
  FUNCTION:  ds_format_log_msg
===========================================================================*/
/*!
    @brief
    Format debug message for logging.

    @return
    None
*/
/*=========================================================================*/
void ds_format_log_msg
(
  char *buf_ptr,
  int buf_size,
  char *fmt,
  ...
);

/*===========================================================================
  FUNCTION  ds_log_multicast
===========================================================================*/
/*!
@brief
  Log to various output streams based on the logmask. If n bits are set
  in the mask, enable corresponding streams for logging, thus the messages
  should be sent to n different output streams

  Example: if bit for ADB and STDOUT are set in the logmask, the log
  messages are sent to ADB as well as STDOUT.

@return
  none

@note

  - Dependencies
    - log bit mask must be set at power up

  - Side Effects
    - log messages are sent to one or more output streams
*/
/*=========================================================================*/
void ds_log_multicast(int lvl, char * fmt, ...);

/*===========================================================================
  FUNCTION  ds_log_write
===========================================================================*/
/*!
@brief
  Log printf-style formatted string using specified debug level.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void
ds_log_write
(
    ds_dbg_level_t dbglvl,
    int ln,
    const char * fmt,
    ...
);

/*===========================================================================
  FUNCTION  ds_atoi
===========================================================================*/
/*!
@brief
  since stdlib atoi and strtol can't distinguish between "0" and "invalid
  numeric string", and returns 0 in both the cases, we need our own
  version of atoi.

@return
  int - numeric value of string (>=0) on success, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int ds_atoi (const char * str);


/*===========================================================================
  FUNCTION  ds_malloc
===========================================================================*/
/*!
@brief
  A general purpose, reentrant memory allocator.

@return
  void * - pointer to memory block

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void * ds_malloc (size_t size);

/*===========================================================================
  FUNCTION  ds_free
===========================================================================*/
/*!
@brief
 Deallocates memory previously allocated using ds_malloc(). This is a
 reentrant function.

@return
  void

@note

  - Dependencies
    - Given memory block must have been allocated using dsc_malloc().

  - Side Effects
    - None
*/
/*=========================================================================*/
void   ds_free   (void * ptr);

/*===========================================================================
  FUNCTION  ds_system_call
===========================================================================*/
/*!
@brief
  Execute a shell command.

@return
  int - numeric value 0 on success, -1 otherwise

@note
  - Dependencies
    - None

  - Side Effects
   - None
*/
/*=========================================================================*/
int  ds_system_call( const char *command, unsigned int cmdlen );

/*===========================================================================
  FUNCTION  ds_system_call2
===========================================================================*/
/*!
@brief
  Execute a shell command with message logging control capability.

@return
  int - numeric value 0 on success, -1 otherwise

@note
  - Dependencies
    - None

  - Side Effects
   - None
*/
/*=========================================================================*/
int ds_system_call2( const char *command, unsigned int cmdlen, boolean logmsg);

/*===========================================================================
  FUNCTION  ds_change_user_cap
===========================================================================*/
/*!
@brief
  Changes the uid/gid and sets the capabilities. uid is a system user id,
  gid is a system group id.  Capabilities should be passed as per
  requirement of capset system call.

@return
  int - numeric value 0 on success, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int
ds_change_user_cap
(
  int uid,
  int gid,
  int caps
);

/*===========================================================================
  FUNCTION  ds_daemonize
===========================================================================*/
/*!
@brief
 Performs typical tasks required to run a program as a daemon process.

@return
  0 on Success, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - Original program will exit and a child is forked which will continue
      execution as the daemon.
*/
/*=========================================================================*/
int ds_daemonize (void);

/*===========================================================================
  FUNCTION  ds_get_num_bits_set_count
===========================================================================*/
/*!
@brief
 This function returns the count of bits that are set (1) in the given input
 parameter x

@return
  Count of bits set

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int ds_get_num_bits_set_count
(
  unsigned int x
);

/*===========================================================================
  FUNCTION  ds_hex_to_dec
===========================================================================*/
/*!
@brief
 Read a char and returns the decimal value.

@return
  int value

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int ds_hex_to_dec(char ch);

#ifdef __cplusplus
}
#endif

#endif /* __DS_UTIL_H__ */
