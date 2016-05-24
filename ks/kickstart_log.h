/*================================================================================================
 *
 * FILE:        kickstart_log.h
 *
 * DESCRIPTION:
 *   Declares debug log macro and log function for supporting EVENT, INFO, ERROR and WARN
 *   messages
 *
 *        Copyright © 2009-2012 Qualcomm Technologies, Inc.
 *               All Rights Reserved.
 *            Qualcomm Technologies Proprietary/GTDR
 *===============================================================================================
 *
 *
 *  kickstart_log.h : Declares debug log macro and log function for supporting EVENT, INFO, ERROR
 *  and WARN messages
 * ==========================================================================================
 *   $Header: //source/qcom/qct/core/storage/tools/kickstart/kickstart_log.h#4 $
 *   $DateTime: 2010/09/28 12:17:11 $
 *   $Author: niting $
 *
 *  Edit History:
 *  YYYY-MM-DD		who		why
 *  -----------------------------------------------------------------------------
 *  2010-09-28       ng      Added command mode support
 *
 *  Copyright 2012 by Qualcomm Technologies, Inc.  All Rights Reserved.
 *
 *==========================================================================================
 */

#ifndef KICKSTART_LOG_H
#define KICKSTART_LOG_H

/* definitions for logging */

#if defined(LINUXPC)
    #define LOGE printf
    #define LOGI printf
    //#define _LARGEFILE64_SOURCE
    //#include <inttypes.h>
#else
    #define LOG_TAG "kickstart"
    #include "cutils/log.h"
    #include "common_log.h"
#endif

/*Logging level */
enum {
    ERROR,
    INFO,
    WARN,
    EVENT,
    STATUS
} LOG_LEVEL;


/*macro for logging */
#define dbg(log_level, fmt ...) kickstart_log (log_level, __FUNCTION__, __LINE__, fmt)

/******************************************************************************
* Name: kickstart_log
*
* Description:
*    This function does the app level logging
*
* Arguments:
*    log              -   type of log INFO/ERROR/WARN
*    function         -   function name
*    line_number      -   line number of the log
*    format           -   format
*    ...              -   variable list argumet
*
* Returns:
*    None
*
* Note:
*
******************************************************************************/
void kickstart_log (int log, const char *function, int line_number, const char *format, ...);

/* Set to TRUE by main program for verbose logging */
extern int verbose;

#endif
