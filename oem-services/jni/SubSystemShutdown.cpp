/******************************************************************************
  @file	 SubSystemShutdown.cpp
  @brief   Qualcomm Shutdown specific code.

  ---------------------------------------------------------------------------
  Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------
 ******************************************************************************/

#define LOG_TAG "SubSystemShutdown"

#include "SubSystemShutdown.h"

extern "C" {
#include <subsystem_control.h>
}

int shutdown()
{
    int rc;

    rc = subsystem_control_shutdown((unsigned)PROC_MSM);

    return rc;
}

/*
 * Class:     com_qti_server_power_SubSystemShutdown
 * Method:    shutdown
 * Signature: ()
 */
JNIEXPORT jint JNICALL Java_com_qti_server_power_SubSystemShutdown_shutdown
(JNIEnv *, jclass)
{
    return shutdown();
}
