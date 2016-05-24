/*==========================================================================*/
/*!
  @file
  ds_Utils_MemManager.cpp

  @brief
  This file provides methods to initialize the ds::Utils PS Mem pools during
  powerup. It also exports a wrapper on top of ps_mem_get_buf() API.

  @see ps_mem_get_buf()
  @see ps_mem_pool_init()
  @see ds_Utils_MemManager.h

            Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*=========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ds_Utils_MemManager.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-06-30 hm  Created module.

===========================================================================*/

/*---------------------------------------------------------------------------
  Include Files
---------------------------------------------------------------------------*/
#include "comdef.h"
#include "ps_mem.h"
#include "AEEStdErr.h"
#include "ds_Utils_DebugMsg.h"
#include "ds_Utils_MemConfig.h"
#include "ds_Utils_MemManager.h"
#include "ds_Utils_Signal.h"
#include "ds_Utils_SignalCtl.h"
#include "ds_Utils_SignalBus.h"
#include "ds_Utils_SignalFactory.h"
#include "ds_Utils_CritSect.h"
#include "ds_Utils_FullPrivSet.h"

using namespace ds::Utils;


/*---------------------------------------------------------------------------
  Macros for sizes of objects of these classes.
---------------------------------------------------------------------------*/
#define SIGNAL_SIZE               ((sizeof(Signal) + 3) & ~3)
#define SIGNAL_CTL_SIZE           ((sizeof(SignalCtl) + 3) & ~3)
#define SIGNAL_BUS_SIZE           ((sizeof(SignalBus) + 3) & ~3)
#define SIGNAL_FACTORY_SIZE       ((sizeof(SignalFactory) + 3) & ~3)
#define CRIT_SECT_BUF_SIZE        ((sizeof(CritSect) + 3) & ~3)
#define FULL_PRIV_SET_BUF_SIZE    ((sizeof(FullPrivSet) + 3) & ~3)

/*---------------------------------------------------------------------------
  Macros for number of bufferes needed, high and low watermarks.
  These are valid for both high end and low end targets.
---------------------------------------------------------------------------*/
#define SIGNAL_NUM_BUFS                (MAX_SIGNAL_OBJS)
#define SIGNAL_CTL_NUM_BUFS            (MAX_SIGNAL_OBJS)
#define SIGNAL_BUS_NUM_BUFS            (MAX_SIGNAL_OBJS / 2)
#define SIGNAL_FACTORY_NUM_BUFS        (1)
#define CRIT_SECT_NUM_BUFS             (1000)
#define FULL_PRIV_SET_NUM_BUFS         (1)

#define SIGNAL_HIGH_WM                 (SIGNAL_NUM_BUFS - 5)
#define SIGNAL_CTL_HIGH_WM             (SIGNAL_CTL_NUM_BUFS - 5)
#define SIGNAL_BUS_HIGH_WM             (SIGNAL_BUS_NUM_BUFS - 5)
#define SIGNAL_FACTORY_HIGH_WM         (1)
#define CRIT_SECT_HIGH_WM              (800)
#define FULL_PRIV_SET_HIGH_WM          (1)

#define SIGNAL_LOW_WM                  (5)
#define SIGNAL_CTL_LOW_WM              (5)
#define SIGNAL_BUS_LOW_WM              (2)
#define SIGNAL_FACTORY_LOW_WM          (0)
#define CRIT_SECT_LOW_WM               (200)
#define FULL_PRIV_SET_LOW_WM           (0)

/*---------------------------------------------------------------------------
  Allocate memory to hold different ds Net objects along with ps_mem header.
---------------------------------------------------------------------------*/

static int signal_buf[PS_MEM_GET_TOT_SIZE_OPT(SIGNAL_NUM_BUFS,
                                              SIGNAL_SIZE)];
static int signal_ctl_buf[PS_MEM_GET_TOT_SIZE_OPT(SIGNAL_CTL_NUM_BUFS,
                                                  SIGNAL_CTL_SIZE)];
static int signal_bus_buf[PS_MEM_GET_TOT_SIZE_OPT(SIGNAL_BUS_NUM_BUFS,
                                                  SIGNAL_BUS_SIZE)];
static int signal_factory_buf[PS_MEM_GET_TOT_SIZE(SIGNAL_FACTORY_NUM_BUFS,
                                                  SIGNAL_FACTORY_SIZE)];
static int crit_sect_buf[PS_MEM_GET_TOT_SIZE_OPT (CRIT_SECT_NUM_BUFS,
                                                  CRIT_SECT_BUF_SIZE)];
static int full_priv_set_buf[PS_MEM_GET_TOT_SIZE_OPT (FULL_PRIV_SET_NUM_BUFS,
                                                  FULL_PRIV_SET_BUF_SIZE)];

#ifdef FEATURE_DATA_PS_MEM_DEBUG
/*---------------------------------------------------------------------------
  Array of pointers used to facilitate easy debugging.  The first one points
  to the ps_mem header and the latter ponts to actual object array.
---------------------------------------------------------------------------*/
static ps_mem_buf_hdr_type   * signal_buf_hdr_ptr[SIGNAL_NUM_BUFS];
static Signal                * signal_buf_ptr[SIGNAL_NUM_BUFS];

static ps_mem_buf_hdr_type   * signal_ctl_buf_hdr_ptr[SIGNAL_CTL_NUM_BUFS];
static SignalCtl             * signal_ctl_buf_ptr[SIGNAL_CTL_NUM_BUFS];

static ps_mem_buf_hdr_type   * signal_bus_buf_hdr_ptr[SIGNAL_BUS_NUM_BUFS];
static SignalBus             * signal_bus_buf_ptr[SIGNAL_BUS_NUM_BUFS];

static ps_mem_buf_hdr_type   * signal_factory_buf_hdr_ptr[SIGNAL_FACTORY_NUM_BUFS];
static SignalFactory         * signal_factory_buf_ptr[SIGNAL_FACTORY_NUM_BUFS];

static ps_mem_buf_hdr_type   * crit_sect_buf_hdr_ptr[CRIT_SECT_NUM_BUFS];
static SignalBus             * crit_sect_buf_ptr[CRIT_SECT_NUM_BUFS];

static ps_mem_buf_hdr_type   * full_priv_set_buf_hdr_ptr[FULL_PRIV_SET_NUM_BUFS];
static SignalBus             * full_priv_set_buf_ptr[FULL_PRIV_SET_NUM_BUFS];
#endif /* FEATURE_DATA_PS_MEM_DEBUG */


void MemoryManager::MemPoolInit
(
  void
)
throw()
{
/*-------------------------------------------------------------------------*/

  LOG_MSG_INFO1 ("Initing mem pools for DSUtils", 0, 0, 0);

  /*-------------------------------------------------------------------------
    Initialize DSUtils mem pools
  -------------------------------------------------------------------------*/
  if (PS_MEM_POOL_INIT_OPT(PS_MEM_DS_UTILS_SIGNAL,
                           signal_buf,
                           SIGNAL_SIZE,
                           SIGNAL_NUM_BUFS,
                           SIGNAL_HIGH_WM,
                           SIGNAL_LOW_WM,
                           NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                           (int *) signal_buf_hdr_ptr,
                           (int *) signal_buf_ptr
#else
                           NULL,
                           NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                          ) == -1)
  {
    goto bail;
  }


  if (PS_MEM_POOL_INIT_OPT(PS_MEM_DS_UTILS_SIGNAL_CTL,
                           signal_ctl_buf,
                           SIGNAL_CTL_SIZE,
                           SIGNAL_CTL_NUM_BUFS,
                           SIGNAL_CTL_HIGH_WM,
                           SIGNAL_CTL_LOW_WM,
                           NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                           (int *) signal_ctl_buf_hdr_ptr,
                           (int *) signal_ctl_buf_ptr
#else
                           NULL,
                           NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                          ) == -1)
  {
    goto bail;
  }

  if (PS_MEM_POOL_INIT_OPT(PS_MEM_DS_UTILS_SIGNAL_BUS,
                           signal_bus_buf,
                           SIGNAL_BUS_SIZE,
                           SIGNAL_BUS_NUM_BUFS,
                           SIGNAL_BUS_HIGH_WM,
                           SIGNAL_BUS_LOW_WM,
                           NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                           (int *) signal_bus_buf_hdr_ptr,
                           (int *) signal_bus_buf_ptr
#else
                           NULL,
                           NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                          ) == -1)
  {
    goto bail;
  }

  if (ps_mem_pool_init(PS_MEM_DS_UTILS_SIGNAL_FACTORY,
                       signal_factory_buf,
                       SIGNAL_FACTORY_SIZE,
                       SIGNAL_FACTORY_NUM_BUFS,
                       SIGNAL_FACTORY_HIGH_WM,
                       SIGNAL_FACTORY_LOW_WM,
                       NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                       (int *) signal_factory_buf_hdr_ptr,
                       (int *) signal_factory_buf_ptr
#else
                       NULL,
                       NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                      ) == -1)
  {
    goto bail;
  }

  if (PS_MEM_POOL_INIT_OPT(PS_MEM_DS_UTILS_CRIT_SECT,
                           crit_sect_buf,
                           CRIT_SECT_BUF_SIZE,
                           CRIT_SECT_NUM_BUFS,
                           CRIT_SECT_HIGH_WM,
                           CRIT_SECT_LOW_WM,
                       NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                           (int *) crit_sect_buf_hdr_ptr,
                           (int *) crit_sect_buf_ptr
#else
                       NULL,
                       NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                      ) == -1)
  {
    goto bail;
  }

  if (PS_MEM_POOL_INIT_OPT(PS_MEM_DS_UTILS_FULL_PRIV_SET,
                           full_priv_set_buf,
                           FULL_PRIV_SET_BUF_SIZE,
                           FULL_PRIV_SET_NUM_BUFS,
                           FULL_PRIV_SET_HIGH_WM,
                           FULL_PRIV_SET_LOW_WM,
                           NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                           (int *) full_priv_set_buf_hdr_ptr,
                           (int *) full_priv_set_buf_ptr
#else
                           NULL,
                           NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                          ) == -1)
  {
    goto bail;
  }


  return;

bail:
  LOG_MSG_FATAL_ERROR ("Cant init DSUTILS modules", 0, 0, 0);
  ASSERT (0);
  return;

} /* MemPoolInit() */
