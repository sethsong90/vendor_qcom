/*
 * Copyright (c) 2012, Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 * Developed by QRD Engineering team.
 */
#include "utils.h"
#include "debug.h"
#if DEBUG_ON
int property_get(const char *key, char *value, const char *default_value)
{
    logd("Use default value:");
    int len=0;
    logd(default_value);
    logd(strcmp(default_value,"ap"));
    if(!strcmp(default_value,"ap"))
        strcpy(value,"MSM8X25_AI_1.0_PD_D111111_DE");
    if(!strcmp(default_value,"mp"))
       strcpy(value,"8X25-SENCKOLGM-PD1020D111111");
    logd(value);
    return len;
}
int property_set(const char *key, const char *value)
{
    return 0;
}
#endif
