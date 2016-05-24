#ifndef SNS_MEMMGR_H
#define SNS_MEMMGR_H

/*============================================================================
  @file sns_memmgr.h

  @brief
  Defines the sensors memory manager interface.

  <br><br>

  DEPENDENCIES: 

Copyright (c) 2010, 2012-2013 Qualcomm Technologies, Inc. All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential
  ============================================================================*/

/*============================================================================
  EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order. Please
  use ISO format for dates.

  $Header: //source/qcom/qct/core/sensors/dsps/common/main/latest/util/memmgr/inc/sns_memmgr.h#4 $
  $DateTime: 2011/12/01 13:52:46 $

  when       who    what, where, why 
  ---------- --- -----------------------------------------------------------
  2011-11-30 jtl Updating macors for USE_NATIVE_MALLOC to work with heap
                 analysis changes
  2011-11-22 br  Differenciated between USE_NATIVE_MEMCPY and USE_NATIVE_MALLOC
  2011-11-14 jhh Added heap memory analysis feature
  2010-11-03 jtl Implementing USE_NATIVE_MALLOC macro handling
  2010-08-30 JTL Moving init function decl into sns_init.h
  2010-08-26 jtl Added header comments

  ============================================================================*/
/*=====================================================================
                       INCLUDES
=======================================================================*/
#include "sns_common.h"
#if (defined USE_NATIVE_MEMCPY || defined USE_NATIVE_MALLOC)
#  include "stdlib.h"
#  include "string.h"
#endif
#if !(defined USE_NATIVE_MEMCPY && defined USE_NATIVE_MALLOC)
#  include "mem_cfg.h"
#endif /* USE_NATIVE_MALLOC */


/*=====================================================================
                    DEFINITIONS AND TYPES
=======================================================================*/
/* used to set debugging level (0~2) */
#define SNS_DEBUG   0

/* verbose level set up */
#ifdef SNS_DEBUG
 #if SNS_DEBUG == 0
  #define DBG_PRINT1(x) (void)0
  #define DBG_PRINT2(x) (void)0
 #else
  #if SNS_DEBUG == 1
#include <stdio.h>
#ifdef _WIN32
   #define DBG_PRINT1(x) (void)(sns_trace x)
   #define DBG_PRINT2(x) (void)0
#else
   #define DBG_PRINT1(x) (void)(printf x)
   #define DBG_PRINT2(x) (void)0
#endif /* _WIN32 */
   #define OI_DEBUG
  #elif SNS_DEBUG == 2
#include <stdio.h>
#ifdef _WIN32
   #define DBG_PRINT1(x) (void)(sns_trace x)
   #define DBG_PRINT2(x) (void)0
#else
   #define DBG_PRINT1(x) (void)(printf x)
   #define DBG_PRINT2(x) (void)(printf x)
#endif /* _WIN32 */
   #define OI_DEBUG
  #endif
 #endif
#endif

#ifdef SNS_MEMMGR_PROFILE_ON
#define SNS_OS_MEMSTAT_SUMMARY_POOLS(pool_id)     SUMMARY_DUMP(pool_id)
#define SNS_OS_MEMSTAT_DETAIL_POOLS(pool_id)      DETAIL_DUMP(pool_id)

typedef enum {
  POOL_16_BYTE,
  POOL_24_BYTE,
  POOL_32_BYTE,
  POOL_64_BYTE,
  POOL_128_BYTE,
  POOL_256_BYTE,
  POOL_512_BYTE,
  POOL_1024_BYTE,
  POOL_ALL
} mem_pool_e;
#else
#define SNS_OS_MEMSTAT_SUMMARY_POOLS(pool_id)     (void)0
#define SNS_OS_MEMSTAT_DETAIL_POOLS(pool_id)      (void)0
#endif

#ifdef USE_NATIVE_MEMCPY
#  define SNS_OS_MEMCOPY(to,from,size)  memcpy(to, from, size)
#  define SNS_OS_MEMSET(block,val,size) memset(block, val, size)
#  define SNS_OS_MEMZERO(block,size)    memset(block, 0, size)
#  define SNS_OS_MEMCMP(s1,s2,n)        memcmp(s1, s2, n)
#else
#  define SNS_OS_MEMCOPY(to,from,size)  OI_MemCopy(to, from, size)
#  define SNS_OS_MEMSET(block,val,size) OI_MemSet(block, val, size)
#  define SNS_OS_MEMZERO(block,size)    OI_MemZero(block, size)
#  define SNS_OS_MEMCMP(s1,s2,n)        OI_MemCmp(s1, s2, n)
#endif

#ifdef USE_NATIVE_MALLOC
#  define SNS_OS_FREE(block)            free(block)
#  define SNS_OS_MALLOC(module,size)    malloc(size)
#  define SNS_OS_FREE(block)            free(block)
#  define SNS_OS_CHGLLOC(new_mod,block)
#else
#  define SNS_OS_MALLOC(module, size)    OI_Malloc(module, size)
#  define SNS_OS_FREE(block)             _OI_FreeIf(block)
#  define SNS_OS_CHGALLOC(new_mod,block) OI_Realloc(new_mod,block)
#endif /* USE_NATIVE_MALLOC */

typedef void (*sns_memmgr_lowmem_cb_t)(void);

/*=====================================================================
                          FUNCTIONS
=======================================================================*/

/*===========================================================================

  FUNCTION:   sns_memmgr_lowmem_cb_register

  ===========================================================================*/
/*!
  @brief Registers a callback to be called in out of memory conditions

  The implementation of this callback should free any unneeded or "low priority"
  memory.

  @param[i] cb: The callback function

  @return
  SNS_SUCCESS if the callback is successfully registered

*/
/*=========================================================================*/
sns_err_code_e
sns_memmgr_lowmem_cb_register( sns_memmgr_lowmem_cb_t cb );

#endif /* SNS_MEMMGR_H */
