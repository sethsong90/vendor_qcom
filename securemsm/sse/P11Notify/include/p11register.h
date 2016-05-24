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

/* Callbacks */
typedef unsigned long int (*p11_session_notify_t) (
    unsigned long int sessionId,
    unsigned long int event,
    void *opaque);
typedef unsigned long int (*p11_slot_notify_t) (
    unsigned long int slotId,
    unsigned long int event,
    void *opaque);

#if defined(__cplusplus)
  extern "C" {
#endif

/* Callback registration */
int EXPORT_SPEC CALL_SPEC registerP11SesssionNotify(p11_session_notify_t cback);
int EXPORT_SPEC CALL_SPEC registerP11SlotNotify(p11_slot_notify_t cback);

#if defined(__cplusplus)
  }
#endif
