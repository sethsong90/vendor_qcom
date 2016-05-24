/*
 * Copyright (c) 2012, Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 * Developed by QRD Engineering team.
 */
#if DEBUG_ON
int property_get(const char *key, char *value, const char *default_value);
int property_set(const char *key, const char *value);
#endif