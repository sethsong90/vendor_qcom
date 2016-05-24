/*===========================================================================
 * Copyright(c) 2013-2014 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
===========================================================================*/

#ifndef _SEC_UI_DEFS_H
#define _SEC_UI_DEFS_H

#include <stdint.h>

/* Error codes */
typedef enum {
    SEC_UI_SUCCESS                   = 0,
    SEC_UI_GENERAL_ERROR             = -1,
    SEC_UI_LISTENER_ERROR            = -2,
    SEC_UI_ALREADY_RUNNING           = -3,
    SEC_UI_IS_NOT_RUNNING            = -4,
    SEC_UI_NON_CONTIGUOUS_BUFFER     = -5,
    SEC_UI_COPY_TO_BUFFER_FAILED     = -6,
    SEC_UI_NON_SECURE_BUFFER         = -7,
    SEC_UI_QSEE_SECURE_BUFFER_ERROR  = -8,
    SEC_UI_NOT_SUPPORTED             = -9,
    SEC_UI_INVALID_INPUT             = -10,
    SEC_UI_TOUCH_DRIVER_ERROR        = -11,
    SEC_UI_TOUCH_TIMEOUT             = -12,
    SEC_UI_ABORTED                   = -13,
    SEC_UI_TOUCH_ABORTED             = -14,
    SEC_UI_SD_STATE_CHANGED          = -15,
    SEC_UI_SD_NOT_ACTIVE             = -16,
    SEC_UI_NO_MEMORY                 = -17,
    SEC_UI_NOT_ENOUGH_BUFFERS        = -18,
    SEC_UI_MORE_BUFFERS_AVAILABLE    = -19,
    SEC_UI_TAG_ERROR                 = -20,
    SEC_UI_OUT_OF_ORDER              = -22,
    SEC_UI_WRONG_BUFFER              = -23,
    SEC_UI_TOO_MANY_BUFFERS          = -24,
    SEC_UI_INVALID_BUFFER            = -25,
    SEC_UI_CALL_ACTIVE               = -26,
    SEC_UI_SCREEN_OFF                = -27,
    SEC_UI_ERR_SIZE                  = 0x7FFFFFFF
} sec_ui_err_t;

typedef enum {
    SEC_TOUCH_GOT_EVENT,    /* Normal touch event */
    SEC_TOUCH_TIMEOUT,      /* Wait() timed out */
    SEC_TOUCH_EXT_ABORT,    /* Aborted externally from HLOS side */
    SEC_TOUCH_STATUS_SIZE   = 0x7FFFFFFF
} touch_status_t;

#endif //_SEC_UI_DEFS_H

