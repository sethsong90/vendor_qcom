/******************************************************************************

                        D S _ L I N U X . H

******************************************************************************/

/******************************************************************************

  @file    ds_linux.h
  @brief   Feature definitions for Linux Data Services

  DESCRIPTION
  Header file containing feature definitions for Linux Data Services.

  ---------------------------------------------------------------------------
  Copyright (c) 2007 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //linux/pkgs/proprietary/data/main/source/dss/inc/ds_linux.h#2 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
10/05/07   ar         Add UMTS Circuit Switched Data macros.
09/28/07   vk         Initial version

******************************************************************************/

#ifndef __DS_LINUX_H__
#define __DS_LINUX_H__

#ifndef FEATURE_DATA
  #define FEATURE_DATA
#endif

#ifndef FEATURE_DATA_LINUX
  #define FEATURE_DATA_LINUX
#endif

#ifndef FEATURE_DS_SOCKETS
  #define FEATURE_DS_SOCKETS
#endif

#ifndef FEATURE_DATA_PS
  #define FEATURE_DATA_PS
#endif

#if 0
#undef FEATURE_DS_LINUX_NO_RPC
#undef FEATURE_DS_LINUX_NO_TARGET

#undef FEATURE_DS_NO_DCM
#undef FEATURE_DS_DEBUG_MTRACE
#undef FEATURE_DS_DEBUG_INTERACTIVE
#endif

/* UMTS Circuit Switched Data (UCSD) */
#ifdef FEATURE_GSM
  #ifndef FEATURE_DATA_GCSD
    #define FEATURE_DATA_GCSD
  #endif
#endif

#ifdef FEATURE_WCDMA
  #ifndef FEATURE_DATA_WCDMA_CS
    #define FEATURE_DATA_WCDMA_CS
  #endif
#endif

#if defined(FEATURE_DATA_GCSD) || defined(FEATURE_DATA_WCDMA_CS)
  #ifndef FEATURE_DATA_UCSD_SCUDIF_API
    #define FEATURE_DATA_UCSD_SCUDIF_API
  #endif

  #ifndef FEATURE_DATA_UCSD_UNIFIED_API
    #define FEATURE_DATA_UCSD_UNIFIED_API
  #endif
#endif

#endif /* __DS_LINUX_H__ */

