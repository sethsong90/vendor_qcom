/******************************************************************************

                        D S C _ U T I L . C

******************************************************************************/

/******************************************************************************

  @file    dsc_util.c
  @brief   DSC's utility functions

  DESCRIPTION
  Collection of utlility functions used by DSC subcomponents. 

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2008 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //source/qcom/qct/platform/linux/common/main/latest/apps/dsc/src/dsc_util.c#3 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
03/18/08   vk         Incorporated code review comments
11/30/07   vk         Cleaned up lint warnings
11/17/07   vk         Added function headers and other comments
09/28/07   vk         Initial version

******************************************************************************/

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#ifdef FEATURE_DATA_LOG_SYSLOG
#include <syslog.h>
#endif
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include "stringl.h"
#include "dsc_util.h"
#include <ctype.h>

/*===========================================================================
                     LOCAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/*--------------------------------------------------------------------------- 
   Constant string prepended to every logged message
---------------------------------------------------------------------------*/
#define DSC_LOG_PREFIX "DSC: "

#define DSC_LOG_MAXLEN 128

/*--------------------------------------------------------------------------- 
   Collection of program's logging configuration information
---------------------------------------------------------------------------*/
static dsc_log_cfg_t dsc_log_cfg;

/*--------------------------------------------------------------------------- 
   Constant table mapping DSC debug levels to syslog message levels
---------------------------------------------------------------------------*/


#ifdef FEATURE_DATA_LOG_SYSLOG 
static const int dsc_log_syslog_level_map[] = {
    LOG_INFO,       /* DSC_DBG_LEVEL_LOW */
    LOG_NOTICE,     /* DSC_DBG_LEVEL_MEDIUM */
    LOG_WARNING,    /* DSC_DBG_LEVEL_HIGH */
    LOG_ERR         /* DSC_DBG_LEVEL_ERROR */
};
#endif

#ifdef FEATURE_DATA_LOG_ADB
static const int dsc_log_syslog_level_map[] = {
1,
2,
3,
4
};
#endif

/*===========================================================================
                            LOCAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  dsc_log_syslog_level
===========================================================================*/
/*!
@brief
  Returns the syslog logging level given a debug level.

@return
  int - syslog logging level

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
#ifndef FEATURE_DATA_LOG_QXDM

static __inline__ int 
dsc_log_syslog_level (dsc_dbg_level_t dbglvl)
{
    return dsc_log_syslog_level_map[dbglvl];
}
#endif
/*===========================================================================
  FUNCTION  dsc_socklthrd_main
===========================================================================*/
/*!
@brief
  The main function of a Socket Listener thread.

@return
  void * - Does not return

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static void *
dsc_socklthrd_main (void * _hdl)
{
    dsc_socklthrd_hdl_t * hdl;
    dsc_socklthrd_fdmap_t * fdmap;
    int ret;
    int i;

    hdl = _hdl;

    /* Make sure handle and handle contents are valid before proceeding. Note
    ** that this checks should all pass since the handle params are verified 
    ** in dsc_socklthrd_start routine. 
    */
  ds_assert(hdl);
  ds_assert(hdl->nfd > 0);

  fdmap = hdl->fdmap;

  /* Make sure that the fdmap is valid before proceeding */
  ds_assert(fdmap);

    for (;;) 
    {
        /* Call select to block on incoming message on all registered fds */
        if ((ret = select(hdl->maxfd+1, &hdl->fdset, NULL, NULL, NULL)) < 0) 
        {
            dsc_log_err("select failed!");
            dsc_abort();
        }

        if (ret > 0) 
        {
            /* One or more fds became readable. Call their respective 
            ** notification handlers. 
            */
            for (i = 0; (unsigned int)i < hdl->nfd; ++i) 
            {
                if (FD_ISSET((fdmap + i)->fd, &hdl->fdset)) 
                {
                    (* (fdmap + i)->read_f)((fdmap + i)->fd);
                }
            }
        } 
        else 
        {
            /* For some reason select returned 0 indicating nothing became 
            ** readable. Print debug message and continue. 
            */
            dsc_log_high("select returned with 0");
        }
    } /* end of for(;;) */
}

/*===========================================================================
                            GLOBAL FUNCTION DEFINITIONS
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
#ifdef FEATURE_DATA_LOG_SYSLOG
void 
dsc_log_init (int threshold, int mode)
{
    /* Use default debug level if an invalid debug level was specified on 
    ** the command line. 
    */
    threshold = (threshold < (int)DSC_DBG_LEVEL_MIN) ?
        (int)DSC_DBG_LEVEL_DFLT : threshold;
    threshold = (threshold > (int)DSC_DBG_LEVEL_MAX) ?
        (int)DSC_DBG_LEVEL_DFLT : threshold;

    /* Set debug level in configuration blob */
    dsc_log_cfg.threshold = (dsc_dbg_level_t)threshold;
    
    /* Use default logging mode if an invalid log mode was specified on the 
    ** command line. 
    */
    mode = (mode < (int)DSC_LOG_MODE_MIN) ? 
        (int)DSC_LOG_MODE_DFLT : mode;
    mode = (mode > (int)DSC_LOG_MODE_MAX) ? 
        (int)DSC_LOG_MODE_DFLT : mode;

    /* Set logging mode in configuration blob */
    dsc_log_cfg.mode = (dsc_log_mode_t)mode;

    switch (dsc_log_cfg.mode) 
    {
    case DSC_LOG_MODE_SYSLOG:
        /* If logging to syslog, initialize logging */
        openlog(DSC_LOG_PREFIX, LOG_NDELAY, LOG_USER);
        break;
    default:
        break;
    }

    /* Also initialize logging to stderr for ds utility functions; note that 
    ** this may be null device if dsc is running as a daemon process.
    */
    ds_log_init(stderr);

    return;
}
#endif
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
void
dsc_log_write 
(
    dsc_dbg_level_t dbglvl, 
    int ln,
    const char * fmt, 
    ...
)
{
    va_list arglist;
    char prep_fmt[DSC_LOG_MAXLEN];

    /* Only log message if logging threshold is hit or exceeded */
    if (dbglvl < dsc_log_cfg.threshold) 
    {
        return;
    }

    /* Prepend message with source filename and line number */
    snprintf(prep_fmt, DSC_LOG_MAXLEN, "%d: ", ln);
    strlcat(prep_fmt, fmt, DSC_LOG_MAXLEN);

    /* We get a compiler warning if the last name argument in the function 
    ** is not passed to va_start, so set fmt to prep_fmt. 
    */
    fmt = prep_fmt;

    va_start(arglist, fmt);

    /* Log message appropriately based on log mode */
    switch (dsc_log_cfg.mode) 
    {
    case DSC_LOG_MODE_FPRINTF:
        /* Print message on stderr if log mode is printf */
        (void)vfprintf(stderr, fmt, arglist);
		fprintf(stderr, "\n");
        break;
    case DSC_LOG_MODE_SYSLOG:
        /* Log to syslog if log mode is syslog */
#ifdef FEATURE_DATA_LOG_SYSLOG
        vsyslog(dsc_log_syslog_level(dbglvl), fmt, arglist);
#endif
#ifdef FEATURE_DATA_LOG_ADB
      switch (dsc_log_syslog_level(dbglvl))
      {
        case 0:
          dsc_log_low( fmt, arglist );
          break;

        case 1:
          dsc_log_high( fmt, arglist );
          break;

        case 2:
          dsc_log_high( fmt, arglist );
          break;

        case 3:
          dsc_log_err( fmt, arglist );
          break;

        default:
          dsc_log_dflt( fmt, arglist );
      }/* switch() */
#endif
        break;
    default:
        /* This should never be reached */
        dsc_abort();
    }

    va_end(arglist);

    return;
}
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
)
{
  va_list ap;

  /*-----------------------------------------------------------------------*/

  ds_assert( buf_ptr != NULL );
  ds_assert( buf_size > 0 );

  /*-----------------------------------------------------------------------*/

  va_start( ap, fmt );

  vsnprintf( buf_ptr, buf_size, fmt, ap );

  va_end( ap );

} /* dsc_format_log_msg */

#endif

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
void 
dsc_abort (void)
{
	abort();
}

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
void *
dsc_malloc (size_t size)
{
    return malloc(size);
}

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
void
dsc_free (void * ptr)
{
    free(ptr);
}

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
void 
dsc_daemonize (void)
{
	pid_t pid;
	pid_t sid;

    /* Fork and exit parent process to ensure that process is not a process
    ** group leader. 
    */
	if ((pid = fork()) > 0) 
        {
		exit(0);
	}

	if (pid < 0) 
        {
        /* Could not create child process. Exit */
		dsc_log_err("Could not create child process");
		dsc_abort();
	}

    /* Become session group leader to disassociate from controlling terminal */
	sid = setsid();

	if (sid < 0) 
        {
		dsc_log_err("setsid() failed");
		dsc_abort();
	}

    /* Set file mode creation mask to 0, to avoid having permissions of created
    ** files being inadvertently changed. 
    */
	(void)umask(0);

    /* Change directory to root */
	if ((chdir("/")) < 0) 
        {
		dsc_log_err("chdir to root failed");
		dsc_abort();
	}

    /* Redirect stdin, stdout and stderr to /dev/null. If running as a daemon,
    ** it is assumed that logging will be to syslog. 
    */
	if (freopen("/dev/null", "r", stdin) == NULL) 
        {
		dsc_log_err("freopen of stdin failed");
		dsc_abort();
	}

	if (freopen("/dev/null", "w", stdout) == NULL) 
        {
		dsc_log_err("freopen of stdout failed");
		dsc_abort();
	}

	if (freopen("/dev/null", "w", stderr) == NULL) 
        {
		dsc_log_err("freopen of stderr failed");
		dsc_abort();
	}

	return;
}

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
int 
dsc_socklthrd_init 
(
    dsc_socklthrd_hdl_t * hdl, 
    dsc_socklthrd_fdmap_t * fdmap, 
    unsigned int maxnfd
)
{
    /* Verify that handle and fdmap ptrs are valid before proceeding */
    if ((hdl == NULL) || (fdmap == NULL)) 
    {
        return -1;
    }

    /* Initialize the handle */
    hdl->fdmap = fdmap;
    hdl->maxnfd = maxnfd;
    hdl->nfd = hdl->maxfd = 0;
    FD_ZERO(&hdl->fdset);

    return 0;
}

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
int 
dsc_socklthrd_addfd 
(
    dsc_socklthrd_hdl_t * hdl, 
    int fd, 
    dsc_socklthrd_fd_read_f read_f
)
{
    /* Make sure passed parameters are valid and there is space in the fdmap 
    ** array for adding this new fd, before proceeding. 
    */
    if ((hdl == NULL) || 
        (hdl->fdmap == NULL) || 
        (read_f == NULL) || 
        (hdl->maxnfd == hdl->nfd)) 
    {
        return -1;
    }

    /* Set new fd in fdset */
    FD_SET(fd, &hdl->fdset);

    /* Add fd to fdmap array and store read handler function ptr */
    (hdl->fdmap + hdl->nfd)->fd = fd;
    (hdl->fdmap + hdl->nfd)->read_f = read_f;

    /* Increment number of fds stored in fdmap */
    ++(hdl->nfd);

    /* Change maxfd if this is the largest valued fd in fdmap */
    if (fd > hdl->maxfd) 
    {
        hdl->maxfd = fd;
    }

    return 0;
}

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
int 
dsc_socklthrd_start (dsc_socklthrd_hdl_t * hdl)
{
    /* Verify that handle is valid and at least one fd was registered with 
    ** handle before proceeding. 
    */
    if ((hdl == NULL) || (hdl->fdmap == NULL) || (hdl->nfd == 0)) 
    {
        return -1;
    }

    /* Create and start listener thread */
    if (pthread_create(&hdl->thrd, NULL, dsc_socklthrd_main, hdl) != 0) 
    {
        return -1;
    }

    return 0;
}

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
int dsc_atoi (const char * str)
{
    int ret = -1;
    char * tmp_str = (char *)str;

    if (NULL == tmp_str)
    {
        return ret;
    }

    ret = atoi(tmp_str);

    if (ret == 0)
    {
        ret = -1;
        /* if we find at least one digit in str, 
           that means atoi really converted it to 0,
           other wise atoi returned 0 because string
           did not have any digits */
        do
        {
            if (isdigit(*tmp_str))
            {
                ret = 0;
                break;
            }

            tmp_str++;
        } while (*tmp_str != '\0');

        if (ret == -1)
        {
            dsc_log_err("string %s does not contain any valid digits",
                        str);
        }

    }

    return ret;
}

