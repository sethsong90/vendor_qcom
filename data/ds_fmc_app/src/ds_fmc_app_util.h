/******************************************************************************

                        D S _ F M C _ A P P _ U T I L . H

******************************************************************************/

/******************************************************************************

  @file    ds_fmc_app_util.h
  @brief   DS_FMC_APP Utility Functions Header File

  DESCRIPTION
  Header file for DS_FMC_APP utility functions.

******************************************************************************/
/*===========================================================================

  Copyright (c) 2010 Qualcomm Technologies, Inc. All Rights Reserved

  Qualcomm Technologies Proprietary

  Export of this technology or software is regulated by the U.S. Government.
  Diversion contrary to U.S. law prohibited.

  All ideas, data and information contained in or disclosed by
  this document are confidential and proprietary information of
  Qualcomm Technologies, Inc. and all rights therein are expressly reserved.
  By accepting this material the recipient agrees that this material
  and the information contained therein are held in confidence and in
  trust and will not be used, copied, reproduced in whole or in part,
  nor its contents revealed in any manner to others without the express
  written permission of Qualcomm Technologies, Inc.

===========================================================================*/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id:$

when       who        what, where, why
--------   ---        -------------------------------------------------------
05/23/10   scb        Initial version

******************************************************************************/

#ifndef __DS_FMC_APP_UTIL_H__
#define __DS_FMC_APP_UTIL_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/select.h>
#ifdef FEATURE_DATA_LOG_ADB
#include <log.h>
#endif /* FEATURE_DATA_LOG_ADB */
#include "comdef.h"
#include "ds_util.h"

#ifdef _DS_FMC_APP_DEBUG
#define INET_ADDR_MAX_BUF_SIZE   46
#endif /* _DS_FMC_APP_DEBUG */

/*===========================================================================
                     GLOBAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

#ifdef DS_FMC_APP_TEST
/* Suppress 'static' keyword in offtarget builds so testcases can
 * invoke internal functions. */
#define LOCAL 
#define ds_fmc_app_malloc(size)  ds_fmc_app_debug_malloc(size)
#define ds_fmc_app_free(ptr)     ds_fmc_app_debug_free(ptr)
#define LOG_ERROR 1
#else
#ifdef LOCAL
#undef LOCAL
#define LOCAL static
#endif
#define ds_fmc_app_malloc(size)  malloc(size)
#define ds_fmc_app_free(ptr)     if( ptr ){ free(ptr); }
#endif /* DS_FMC_APP_TEST */

/*--------------------------------------------------------------------------- 
   Definition of 'assert' macro. This is needed as ONCRPC/DSM hijacks path
   to include another file with name assert.h, so that the standard library
   assert is not available to ONCRPC clients.
---------------------------------------------------------------------------*/
#define DS_FMC_APP_ASSERT(a)   ds_assert(a)

/*---------------------------------------------------------------------------
  Definition of 'abort' macro.  Diagnostic messages are typically sent
  to QXDM but there is no guarantee message will be displayed.  Need
  to ensure abort related message is sent to persistent Android log.
---------------------------------------------------------------------------*/
#define DS_FMC_APP_ABORT(...)                            \
     LOG(LOG_ERROR, "QC-DS_FMC_APP", __VA_ARGS__);       \
     fprintf(stderr, __VA_ARGS__);                   \
     DS_FMC_APP_ASSERT(0);

/*---------------------------------------------------------------------------
  Definition of 'stop' macro.  Diagnostic messages are typically sent
  to QXDM but there is no guarantee message will be displayed.  Need
  to ensure abort related message is sent to persistent Android log.
---------------------------------------------------------------------------*/
#define DS_FMC_APP_STOP(...)                             \
     LOG(LOG_ERROR, "QC-DS_FMC_APP", __VA_ARGS__);       \
     fprintf(stderr, __VA_ARGS__);                   \
     while(1) { sleep(0xFFFFFFFF); }

/*--------------------------------------------------------------------------- 
   Logging macros
---------------------------------------------------------------------------*/
#define  ds_fmc_app_log               ds_log
#define  ds_fmc_app_log_err           ds_log_err
#define  ds_fmc_app_log_high          ds_log_high
#define  ds_fmc_app_log_low           ds_log_low
#define  ds_fmc_app_log_dflt          ds_log_dflt
#define  ds_fmc_app_log_sys_err       ds_log_sys_err

#define  ds_fmc_app_log_init          ds_log_init2

extern boolean function_debug;
#define  DS_FMC_APP_LOG_FUNC_ENTRY    if(function_debug){ ds_log_func_entry(); }
#define  DS_FMC_APP_LOG_FUNC_EXIT     if(function_debug){ ds_log_func_exit();  }

#define DS_FMC_APP_LOG_IPV4_ADDR(level, prefix, ip_addr)                        \
        DS_LOG_IPV4_ADDR(level, prefix, ip_addr)

#define DS_FMC_APP_LOG_IPV6_ADDR(level, prefix, ip_addr)                        \
        DS_LOG_IPV6_ADDR(level, prefix, ip_addr)


/*===========================================================================
                            GLOBAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  ds_fmc_app_daemonize
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
void ds_fmc_app_daemonize (void);

#endif /* __DS_FMC_APP_UTIL_H__ */
