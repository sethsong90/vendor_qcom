/******************************************************************************
  @file  ping_client_common
  @brief common ping client functions
  DESCRIPTION
  Provide a more detailed description of the source file here
  INITIALIZATION AND SEQUENCING REQUIREMENTS
  Please detail this out here
  -----------------------------------------------------------------------------
  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------
******************************************************************************/

/*=====================================================================
                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/modem-api/main/source/test/oncrpc/ping/src/ping_client_common_stats.c#4 $

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.


when       who      what, where, why
--------   ---      -------------------------------------------------------
09/29/2009   fr     Adapted for ping_apps test
10/01/2008   rr     Remove oncrpc_os.h no longer needed
05/02/2008   rr     Initial version

======================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "oncrpc.h"
#include "ping_apps.h"
#include "ping_apps_rpc.h"
#include "ping_client_common_stats.h"



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
  ping_client_results_type *results)
{
  uint32 num_bytes;
  num_bytes = (data_size_words*4 + 4)*num_loops;
  results->data_transfer_rate_kbps = (double)(num_bytes ) / (double)test_duration_ms;
  results->num_bytes_transfered = (data_size_words*4 + 4)*num_loops;
  results->num_fwd_bytes_per_call = data_size_words*4;
  results->num_loops_completed = num_loops;
  results->num_ret_bytes_per_call = ret_data_size_words*4;
  results->rpc_call_duration_ms = (double)test_duration_ms/(double)num_loops;
  results->test_duration_ms=test_duration_ms;
}


/*===========================================================================
  FUNCTION  ping_mdm_client_common_print_stats
===========================================================================*/
/*!
@brief
  Print Test Results

@return
  N/A

@note
  - Dependencies  ping_mdm_client_results_type must be pre-allocated

  - Side Effects
*/
/*=========================================================================*/
void ping_client_common_print_stats(ping_client_results_type *results,
  ping_print_format_enum format)
{
  if( results )
    switch( format )
    {
      case   PING_STANDARD_PRINT_FORMAT:
        printf("Test Duration          :%d ms\n",(int)results->test_duration_ms);
        printf("Num RPC Calls completed:%d\n",(int)results->num_loops_completed);
        printf("Num fwd bytes per call :%d\n",(int)results->num_fwd_bytes_per_call);
        printf("Num ret bytes per call :%d\n",(int)results->num_ret_bytes_per_call);
        printf("Num bytes transferred  :%d\n",(int)results->num_bytes_transfered);
        printf("RPC Call Duration      :%05.3f ms\n",results->rpc_call_duration_ms);
        printf("Data Transfer Rate     :%05.3f kbytes/sec\n",results->data_transfer_rate_kbps);
        break;
      case PING_SINGLE_LINE_HEADER_PRINT_FORMAT:
        printf("Test(ms) #Calls  CallTime(ms) FwdBytes   RetBytes  TotKBytes     DataRate(kbps) \n");
        break;
      case PING_SINGLE_LINE_PRINT_FORMAT:
        printf("%5d    %5d   %8.3f      %5d    %5d  %10d       %8.3f \n",
          (int)results->test_duration_ms,
          (int)results->num_loops_completed,
           results->rpc_call_duration_ms,
          (int)results->num_fwd_bytes_per_call,
          (int)results->num_ret_bytes_per_call,
          (int)results->num_bytes_transfered/1000,
        results->data_transfer_rate_kbps);
        break;
      default:
        printf("ERROR default case not handled in function %s \n",__FUNCTION__);
        break;
    }
};

