#ifndef PING_CLIENT_COMMON_STATS_H
#define PING_CLIENT_COMMON_STATS_H
/******************************************************************************
  @file  ping_client_common_stats
  @brief common stats utilities for ping client programs

  DESCRIPTION

   Utilities for ping client programs to calculate and print stats

  INITIALIZATION AND SEQUENCING REQUIREMENTS

  -----------------------------------------------------------------------------
  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------
******************************************************************************/
/*=====================================================================
                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/modem-api/main/source/test/oncrpc/ping/src/ping_client_common_stats.h#1 $

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.


when       who      what, where, why
--------   ---      -------------------------------------------------------
09/29/2009   fr     Adapted for ping_apps test
05/02/2008   rr     Initial version

======================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "comdef.h"


typedef enum
{
  PING_STANDARD_PRINT_FORMAT,
  PING_SINGLE_LINE_HEADER_PRINT_FORMAT,
  PING_SINGLE_LINE_PRINT_FORMAT
}ping_print_format_enum;

typedef struct
{
  uint32 rc;
  uint32 test_duration_ms;
  uint32 num_loops_completed;
  uint32 num_fwd_bytes_per_call;
  uint32 num_ret_bytes_per_call;
  uint32 num_bytes_transfered;
  double data_transfer_rate_kbps;
  double rpc_call_duration_ms;
}ping_client_results_type;


/*===========================================================================
  FUNCTION  ping_client_common_calc_stats
===========================================================================*/
/*!
@brief
  Calculate Stats of test

@return
  returns statistics of test in  ping_client_results_type

@note
  - Dependencies  ping_client_results_type must be pre-allocated

  - Side Effects
*/
/*=========================================================================*/
void ping_client_common_calc_stats(
  uint32 num_loops,
  uint32 data_size_words,
  uint32 ret_data_size_words,
  uint32 test_duration_ms,
  ping_client_results_type *results);


/*===========================================================================
  FUNCTION  ping_client_common_calc_stats_timespec
===========================================================================*/
/*!
@brief
  Calculate Stats of test using timespec of start and end times

@return
  returns statistics of test in  ping_client_results_type

@note
  - Dependencies  ping_client_results_type must be pre-allocated

  - Side Effects
*/
/*=========================================================================*/
void ping_client_common_calc_stats_timespec(
  uint32 num_loops,
  uint32 data_size_words,
  uint32 ret_data_size_words,
  struct timespec *start_time_info,
  struct timespec *end_time_info,
  ping_client_results_type *results);


/*===========================================================================
  FUNCTION  ping_client_common_print_stats
===========================================================================*/
/*!
@brief
  Print Test Results

@return
  N/A

@note
  - Dependencies  ping_client_results_type must be pre-allocated

  - Side Effects
*/
/*=========================================================================*/
void ping_client_common_print_stats(ping_client_results_type *results, ping_print_format_enum format);

#endif /* PING_CLIENT_COMMON_STATS_H */
