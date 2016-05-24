/******************************************************************************

                        D S C _ U T I L . H

******************************************************************************/

/******************************************************************************

  @file    dsc_util.c
  @brief   DSC's utility functions header file

  DESCRIPTION
  Header file for utlility functions used by DSC subcomponents. 

  ---------------------------------------------------------------------------
  Copyright (c) 2007 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //source/qcom/qct/platform/linux/common/main/latest/apps/dsc/src/dsc_util.h#4 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
11/17/07   vk         Added function headers and other comments
09/28/07   vk         Initial version

******************************************************************************/

#ifndef __DSC_UTIL_H__
#define __DSC_UTIL_H__

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/select.h>
#include "ds_util.h"

#ifdef FEATURE_DATA_LOG_ADB
#include <utils/Log.h>
#endif

#ifdef FEATURE_DATA_LOG_QXDM
#include "comdef.h"
#include <msg.h>
#include <msgcfg.h>
#include <diag_lsm.h>
#include <log.h>
#endif

/*===========================================================================
                     GLOBAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/*--------------------------------------------------------------------------- 
   Type representing program's runtime debug level
---------------------------------------------------------------------------*/
typedef enum {
    DSC_DBG_LEVEL_MIN    = 0,
    DSC_DBG_LEVEL_LOW    = 0,
    DSC_DBG_LEVEL_MEDIUM = 1,
    DSC_DBG_LEVEL_HIGH   = 2,
    DSC_DBG_LEVEL_ERROR  = 3,
    DSC_DBG_LEVEL_DFLT   = DSC_DBG_LEVEL_ERROR,
    DSC_DBG_LEVEL_MAX    = 3
} dsc_dbg_level_t;

/*--------------------------------------------------------------------------- 
   Type representing program's logging mode
---------------------------------------------------------------------------*/
typedef enum {
    DSC_LOG_MODE_MIN     = 0,
    DSC_LOG_MODE_DFLT    = 0,
    DSC_LOG_MODE_FPRINTF = DSC_LOG_MODE_DFLT,
    DSC_LOG_MODE_SYSLOG  = 1,
    DSC_LOG_MODE_MAX     = 1
} dsc_log_mode_t;

/*--------------------------------------------------------------------------- 
   Type representing program's logging configuration
---------------------------------------------------------------------------*/
typedef struct {
    dsc_dbg_level_t threshold; /* level at which to print */
    dsc_log_mode_t  mode;      /* log method to use */
} dsc_log_cfg_t;

/*--------------------------------------------------------------------------- 
   Type representing function callback registered with a socket listener 
   thread for reading from a socket on receipt of an incoming message
---------------------------------------------------------------------------*/
typedef void (* dsc_socklthrd_fd_read_f) (int fd);

/*--------------------------------------------------------------------------- 
   Type representing collection of info registered with a socket listener 
   thread by clients
---------------------------------------------------------------------------*/
typedef struct {
    int fd;                         /* Socket descriptor */
    dsc_socklthrd_fd_read_f read_f; /* Incoming data  notification handler */
} dsc_socklthrd_fdmap_t;

/*--------------------------------------------------------------------------- 
   Type representing a handle to a socket listener thread. Clients must 
   use this as an opaque pointer. 
---------------------------------------------------------------------------*/
typedef struct {
    pthread_t               thrd;   /* Pthread object for the thread */
    dsc_socklthrd_fdmap_t * fdmap;  /* Ptr to array of fdmap structs */
    unsigned int            nfd;    /* Number of valid fds in fdmap */
    unsigned int            maxnfd; /* Size of fdmap array */
    int                     maxfd;  /* Maximum valued fd */
    fd_set                  fdset;  /* fdset used in select() operation */
} dsc_socklthrd_hdl_t;

/*--------------------------------------------------------------------------- 
   Helper macro for logging function entry at low log level
---------------------------------------------------------------------------*/
#define dsc_log_func_entry()    \
    dsc_log_write               \
    (                           \
        DSC_DBG_LEVEL_LOW,      \
        __LINE__,               \
        "Entering function %s", \
        __FUNCTION__            \
    )

/*--------------------------------------------------------------------------- 
   Helper macro for logging function exit at low log level
---------------------------------------------------------------------------*/
#define dsc_log_func_exit()     \
    dsc_log_write               \
    (                           \
        DSC_DBG_LEVEL_LOW,      \
        __LINE__,               \
        "Exiting function %s",  \
        __FUNCTION__            \
    )


/*--------------------------------------------------------------------------- 
   Helper macro for logging formatted string at specified log level 
---------------------------------------------------------------------------*/

#ifdef FEATURE_DATA_LOG_SYSLOG

#define dsc_log(a, ...) dsc_log_write(a, __LINE__, __VA_ARGS__)

/*--------------------------------------------------------------------------- 
   Helper macro for logging formatted string at default log level 
---------------------------------------------------------------------------*/
#define dsc_log_dflt(...) dsc_log_write(DSC_DBG_LEVEL_DFLT, __LINE__, __VA_ARGS__)

/*--------------------------------------------------------------------------- 
   Helper macro for logging formatted string at "error" log level 
---------------------------------------------------------------------------*/
#define dsc_log_err(...) dsc_log_write(DSC_DBG_LEVEL_ERROR, __LINE__, __VA_ARGS__)

/*--------------------------------------------------------------------------- 
   Helper macro for logging formatted string at "high" log level 
---------------------------------------------------------------------------*/
#define dsc_log_high(...) dsc_log_write(DSC_DBG_LEVEL_HIGH,__LINE__, __VA_ARGS__)

/*--------------------------------------------------------------------------- 
   Helper macro for logging formatted string at "low" log level 
---------------------------------------------------------------------------*/
#define dsc_log_low(...) dsc_log_write(DSC_DBG_LEVEL_LOW, __LINE__, __VA_ARGS__)

/*--------------------------------------------------------------------------- 
   Helper macro for logging formatted string at "error" log level, followed
   by the system error message (as obtained using std library's strerror())
---------------------------------------------------------------------------*/
#define dsc_log_sys_err(a) \
        dsc_log_write(DSC_DBG_LEVEL_ERROR,__LINE__, a" %s", strerror(errno))

#endif

#ifdef FEATURE_DATA_LOG_ADB

#define DSS_LOG_TAG "QC-DSS-LIB"

#define dsc_log(a, ...) LOG(LOG_INFO, DSS_LOG_TAG, __VA_ARGS__)

#define dsc_log_dflt(...) LOG(LOG_ERROR, DSS_LOG_TAG, __VA_ARGS__)	

#define dsc_log_err(...) LOG(LOG_ERROR, DSS_LOG_TAG, __VA_ARGS__)

#define dsc_log_high(...) LOG(LOG_INFO, DSS_LOG_TAG, __VA_ARGS__)

#define dsc_log_low(...) LOG(LOG_DEBUG, DSS_LOG_TAG, __VA_ARGS__)

#define dsc_log_sys_err(a) LOG(LOG_ERROR, DSS_LOG_TAG, a " %s", strerror(errno) )

#endif

#ifdef FEATURE_DATA_LOG_QXDM

/* Maximum length of log message */
#define DSC_MAX_DIAG_LOG_MSG_SIZE      512

/* Log message to Diag */
#define DSC_LOG_MSG_DIAG( lvl, ... )                                             \
  {                                                                              \
    char buf[ DSC_MAX_DIAG_LOG_MSG_SIZE ];                                       \
                                                                                 \
    /* Format message for logging */                                             \
    dsc_format_log_msg( buf, DSC_MAX_DIAG_LOG_MSG_SIZE, __VA_ARGS__ );           \
                                                                                 \
    /* Log message to Diag */                                                    \
    MSG_SPRINTF_1( MSG_SSID_DIAG, lvl, "%s", buf );                              \
  }


#define dsc_log(a, ...) DSC_LOG_MSG_DIAG(MSG_LEGACY_HIGH, __VA_ARGS__)

#define dsc_log_dflt(...) DSC_LOG_MSG_DIAG(MSG_LEGACY_HIGH, __VA_ARGS__)	

#define dsc_log_err(...) DSC_LOG_MSG_DIAG(MSG_LEGACY_ERROR, __VA_ARGS__)

#define dsc_log_high(...) DSC_LOG_MSG_DIAG(MSG_LEGACY_HIGH, __VA_ARGS__)

#define dsc_log_low(...) DSC_LOG_MSG_DIAG(MSG_LEGACY_HIGH, __VA_ARGS__)

#define dsc_log_sys_err(a) DSC_LOG_MSG_DIAG(MSG_LEGACY_ERROR,  a " %s", strerror(errno))

#endif

/*===========================================================================
                            GLOBAL FUNCTION DECLARATIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  dsc_log_init
===========================================================================*/
/*!
@brief
  Initialization routine for logging functionality.

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
void dsc_log_init (int threshold, int mode);

/*===========================================================================
  FUNCTION  dsc_log_write
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
void dsc_log_write 
(
    dsc_dbg_level_t dbglvl, 
    int ln,
    const char * fmt, 
    ...
);

/*===========================================================================
  FUNCTION  dsc_abort
===========================================================================*/
/*!
@brief
  Aborts program.

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
void dsc_abort (void);

/*===========================================================================
  FUNCTION  dsc_malloc
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
void * dsc_malloc (size_t size);

/*===========================================================================
  FUNCTION  dsc_free
===========================================================================*/
/*!
@brief
 Deallocates memory previously allocated using dsc_malloc(). This is a 
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
void   dsc_free   (void * ptr);

/*===========================================================================
  FUNCTION  dsc_daemonize
===========================================================================*/
/*!
@brief
 Performs typical tasks required to run a program as a daemon process.

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - Original program will exit and a child is forked which will continue
      execution as the daemon.
*/
/*=========================================================================*/
void dsc_daemonize (void);

/*===========================================================================
  FUNCTION  dsc_socklthrd_init
===========================================================================*/
/*!
@brief
  Initializes a Socket Listener thread handle.

@return
  int - 0 on success, -1 otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
int dsc_socklthrd_init (dsc_socklthrd_hdl_t * hdl, dsc_socklthrd_fdmap_t * fdmap, unsigned int maxfd);

/*===========================================================================
  FUNCTION  dsc_socklthrd_addfd
===========================================================================*/
/*!
@brief
  Adds a socket descriptor to the list of descriptors to read from for a 
  socket listener thread represented by the given handle.

@return
  int - 0 on success, -1 otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
int dsc_socklthrd_addfd (dsc_socklthrd_hdl_t * hdl, int fd, dsc_socklthrd_fd_read_f read_f);

/*===========================================================================
  FUNCTION  dsc_socklthrd_start
===========================================================================*/
/*!
@brief
  Starts the socket listener thread and associates it with the specified 
  handle.

@return
  int - 0 on success, -1 otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - Spawns a pthread for reading data received on associated sockets.
*/
/*=========================================================================*/
int dsc_socklthrd_start (dsc_socklthrd_hdl_t * hdl);

/*===========================================================================
  FUNCTION  dsc_atoi
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
int dsc_atoi (const char *);

#ifdef FEATURE_DATA_LOG_QXDM
/*=========================================================================
  FUNCTION:  dsc_format_log_msg

===========================================================================*/
/*!
    @brief
    Format debug message for logging.

    @return
    None
*/
/*=========================================================================*/
void dsc_format_log_msg
(
  char *buf_ptr,
  int buf_size,
  char *fmt,
  ...
);
#endif

#endif /* __DSC_UTIL_H__ */
