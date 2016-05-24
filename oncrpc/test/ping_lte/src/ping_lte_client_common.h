#ifndef PING_MDM_CLIENT_COMMON_H
#define PING_MDM_CLIENT_COMMON_H
/******************************************************************************
  @file  ping__lte_client_common
  @brief common ping client functions
  DESCRIPTION
  Provide a more detailed description of the source file here
  INITIALIZATION AND SEQUENCING REQUIREMENTS
  Please detail this out here
  -----------------------------------------------------------------------------
  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------
******************************************************************************/

/*=====================================================================
                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/modem-api/main/source/test/oncrpc/ping/src/ping_lte_client_common.h#1 $

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why
--------   ---      -------------------------------------------------------
03/15/2010   aa     Changed for LTE Test
04/18/2008   rr     Initial version

======================================================================*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "comdef.h"
#include "ping_client_common_stats.h"

#define PING_LTE_MAX_WORDS 2048

/*===========================================================================
  FUNCTION  ping_lte_client_common_data_test
===========================================================================*/
/*!
@brief
  Main body of ping_lte_client_common_data_test

@return
  0 for pass, -1 for fail

@note
  - Dependencies
    - N/A
  - Side Effects
*/
/*=========================================================================*/
int ping_lte_client_common_data_test(
  uint32 test_length,
  uint32 data_size_words,
  uint32 test_duration_ms,
  boolean verbose,
  ping_client_results_type *results);


/*===========================================================================
  FUNCTION  ping_lte_client_common_data_test_random
===========================================================================*/
/*!
@brief
  Main body of ping_lte_client_common_data_test

@return
  0 for pass, -1 for fail

@note
  - Dependencies
    - N/A
  - Side Effects
*/
/*=========================================================================*/
int ping_lte_client_common_data_test_random(
  uint32 test_duration_ms,
   boolean verbose,
  ping_client_results_type *results);

#endif /* PING_CLIENT_COMMON_H */
