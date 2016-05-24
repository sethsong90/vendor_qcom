/* Copyright (c) 2009 by Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef AUDIOEQ_H
#define AUDIOEQ_H

void audioeq_calccoefs(
        int32_t  V0,
        int32_t  fc,
        int32_t  fs,
        uint16_t type,
        int32_t  Q,
        int32_t  *bassNum,
        int32_t  *bassDen,
        uint16_t *shiftQ);

#endif /* AUDIOEQ_H */
