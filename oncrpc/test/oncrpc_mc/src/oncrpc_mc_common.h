#ifndef __ONCRPC_MC_COMMON_H__
#define __ONCRPC_MC_COMMON_H__
/******************************************************************************
  @file oncrpc_mc_common.h
  @brief Contains common declarations for oncrpc_mc_test_xxxx

  -----------------------------------------------------------------------------
  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------
******************************************************************************/
/*=====================================================================

                     INCLUDE FILES FOR MODULE

======================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>
#include "ping_mdm_rpc.h"
#include "ping_mdm_rpc_rpc.h"
#include "oem_rapi.h"
#include "oem_rapi_rpc.h"

/*=====================================================================
      Typedefs
======================================================================*/
struct rpc_thread_arg {
    int test_duration_ms;
    int num_iterations;
    int data_size_words;
    int num_threads;
};

/*=====================================================================
      Constants and Macros
======================================================================*/

#define OEM_RAPI_STREAMING_TEST_DATA_SIZE_IN (OEM_RAPI_MAX_SERVER_INPUT_BUFFER_SIZE)
#define OEM_RAPI_STREAMING_TEST_DATA_SIZE_OUT (OEM_RAPI_MAX_CLIENT_OUTPUT_BUFFER_SIZE )
#define PING_MDM_CLIENT_RESTART_SIG        (0x01)
#define PING_MDM_CLIENT_TEST_CALLBACK_SIG  (0x02)
#define PING_MDM_CLIENT_TEST_END_SIG       (0x04)
#define PING_MAX_WORDS (4096-16)
#define NUM_OF_CORES 2
/*=====================================================================
      Variables
=====================================================================*/

#endif
