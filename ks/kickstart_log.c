/*================================================================================================
 *
 * FILE:        kickstart_log.c
 *
 * DESCRIPTION:
 *    This module implements the logic for logging EVENT, INFO, WARNING, ERROR messages
 *
 *
 *        Copyright © 2009-2012 Qualcomm Technologies, Inc.
 *               All Rights Reserved.
 *            Qualcomm Technologies Proprietary/GTDR
 *===============================================================================================
 *
 *
 *  kickstart_log.c : This module implements the logic for logging EVENT, INFO, WARNING, ERROR
 *  messages
 * ==========================================================================================
 *   $Header: //source/qcom/qct/core/storage/tools/kickstart/kickstart_log.c#5 $
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
#include "kickstart_log.h"
#include <stdarg.h>
#include <stdio.h>

/*===========================================================================
 *  FUNCTION:  kickstart_log
 *
 *  DESCRIPTION
 *  Kickstart logger, prints the function name, line number and the logs
 *
 *  PARAMETERS
 *  func name      - function name
 *  line_number    - line number
 *  format         - variable list of arguments
 *
 *  RETURN VALUE
 *  NONE
 *
 *  SIDE EFFECTS
 *  NONE
 *  ===========================================================================*/
void kickstart_log (int log_level, const char *func_name, int line_number, const char *format, ...)
{
    va_list args;
    char *log_type;
	char log[1024];

	va_start (args, format);
	vsnprintf(log, sizeof(log), format, args);
	va_end (args);

    if ((log_level == ERROR) ||
        log_level == WARN ||
        log_level == STATUS ||
        verbose) {
        switch (log_level) {
        case ERROR:
            LOGE("ERROR: function: %s:%d %s", func_name, line_number, log);
            break;

        case INFO:
            LOGI("INFO: function: %s:%d %s", func_name, line_number, log);
            break;

        case WARN:
            LOGE("WARNING: function: %s:%d %s", func_name, line_number, log);
            break;

        case EVENT:
            LOGE("EVENT: %s", log);
            break;

        case STATUS:
            LOGE("%s", log);
            break;
        }
    }
}
