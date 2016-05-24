/*
 * Copyright (c) 2014 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

/**
 * This header contains only the DTA variables.
 */

#ifndef DTA_FLAG_H
#define DTA_FLAG_H

// This value should only be read in the lower level. NativeNfcManager sets this
// through NFA API
extern int dta_Pattern_Number;

extern int dta_flag_all;
/**
 * Checks whether the DTA mode is enabled.
 *
 * @return true if the DTA mode is enabled, false otherwise.
 */
static BOOLEAN in_dta_mode() {
  return dta_Pattern_Number >= 0;
}

#endif // DTA_FLAG_H
