/**
 * Copyright (c) 2012 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Qualcomm Technologies Confidential and Proprietary.
 */

#pragma once

#ifdef _WIN32
#include "windows.h"
#undef uuid_t
#endif

#include <stdlib.h>
#include "platform.h"

#if defined(__cplusplus)
  extern "C" {
#endif

/* Notification from a slot */
unsigned long int EXPORT_SPEC CALL_SPEC P11SlotNotify(
    unsigned long int slotId,
    unsigned long int event,
    void * opaque
    );

/* Notification from a session */
unsigned long int EXPORT_SPEC CALL_SPEC P11SessionNotify(
    unsigned long int sessionId,
    unsigned long int event,
    void * opaque
    );

#if defined(__cplusplus)
  }
#endif
