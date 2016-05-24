#ifndef PING_APPS_CLIENT_COMMON_H
#define PING_APPS_CLIENT_COMMON_H
/******************************************************************************
  @file  ping__mdm_client_common
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

  $Id: //linux/pkgs/proprietary/modem-api/main/source/test/oncrpc/ping/src/ping_mdm_client_common.h#1 $

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why
--------   ---      -------------------------------------------------------
09/29/2009   fr     Renamed the functions to ping_apps from ping_mdm for use with
		    ping_apps test.
04/18/2008   rr     Initial version

======================================================================*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "comdef.h"
#include "ping_client_common_stats.h"


/*===========================================================================
  FUNCTION  ping_apps_client_common_data_test
===========================================================================*/
/*!
@brief
  Main body of ping_apps_client_common_data_test

@return
  0 for pass, -1 for fail

@note
  - Dependencies
    - N/A
  - Side Effects
*/
/*=========================================================================*/
int ping_apps_client_common_data_test(
  uint32 test_length,
  uint32 data_size_words,
  uint32 test_duration_ms,
  boolean verbose,
  ping_client_results_type *results);


/*===========================================================================
  FUNCTION  ping_apps_client_common_data_test_random
===========================================================================*/
/*!
@brief
  Main body of ping_mdm_client_common_data_test

@return
  0 for pass, -1 for fail

@note
  - Dependencies
    - N/A
  - Side Effects
*/
/*=========================================================================*/
int ping_apps_client_common_data_test_random(
  uint32 test_duration_ms,
   boolean verbose,
  ping_client_results_type *results);

#endif /* PING_CLIENT_COMMON_H */
