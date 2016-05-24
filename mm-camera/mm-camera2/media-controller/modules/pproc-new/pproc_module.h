/*============================================================================

  Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

============================================================================*/
#ifndef __PPROC_MODULE_H__
#define __PPROC_MODULE_H__

#include "mct_module.h"

/* macros for unpacking identity */
#define PPROC_GET_STREAM_ID(identity) ((identity) & 0xFFFF)
#define PPROC_GET_SESSION_ID(identity) (((identity) & 0xFFFF0000) >> 16)

mct_module_t* pproc_module_get_sub_mod(mct_module_t *module, const char *name);

#endif /* __PPROC_MODULE_H__ */
