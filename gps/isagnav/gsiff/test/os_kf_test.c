/******************************************************************************
  @file:  os_kf_test.c

  DESCRIPTION
    Unit test for os_kf implementation.

  INITIALIZATION AND SEQUENCING REQUIREMENTS

  -----------------------------------------------------------------------------
  Copyright (c) 2011 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------

******************************************************************************/

/*=====================================================================
                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why
--------   ---      -------------------------------------------------------
05/01/11   jb       Initial version

======================================================================*/

#include "os_kf.h"

#define LOG_TAG "os_kf"
#include "log_util.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <stdbool.h>

/* The process noise for the TS Offset Kalman filter is defined as a
   ramp function with following parameters
*/
#define US_TO_SEC                     (1000000.0)
#define US_TO_MS                      (1000.0)
#define TEST_MIN_TS_OFFSET_Q_DT_US    (1 * US_TO_SEC)   /* time limit for minimum process noise */

#define TEST_MIN_TS_OFFSET_Q_US2      (250.0)      /* minimum process noise */
#define TEST_TS_OFFSET_Q_SF_US2US     (0.15)     /* process noise rate between time limits (us^2)/us */

#define TEST_TS_OFFSET_MEAS_NOISE_US2 (400.0*400.0)      /* measurement noise */
#define TEST_TS_OFFSET_R_SF_US2US     (0.55)

/* Parameters for outlier detection and reset for TS offset KF */
#define TEST_MIN_TS_OFFSET_DIVERGE_THRESHOLD_US (1.25 * US_TO_MS)
#define TEST_TS_OFFSET_N_VAR_THRESH             (25)
#define TEST_NUM_OUTLIER_KF_RESET               (15)
#define TEST_TIME_UPDATE_INTERVAL_US            (1 * US_TO_SEC)
#define TEST_TS_OFFSET_MIN_UNC_TO_INIT_US       (10)

static void* kf = NULL;

static double proc_noise(double d_DeltaTimeUs)
{
   /* Add process noise to input uncertainty. */
   if ( d_DeltaTimeUs <= TEST_MIN_TS_OFFSET_Q_DT_US )
   {
      return TEST_MIN_TS_OFFSET_Q_US2;
   }
   else
   {
      return(TEST_TS_OFFSET_Q_SF_US2US * (d_DeltaTimeUs - TEST_MIN_TS_OFFSET_Q_DT_US)) + TEST_MIN_TS_OFFSET_Q_US2;
   }
}

static double meas_noise(double d_DeltaTimeUs)
{
   /* Add measurement noise to input uncertainty. */
   if ( d_DeltaTimeUs <= TEST_MIN_TS_OFFSET_Q_DT_US )
   {
      return TEST_TS_OFFSET_MEAS_NOISE_US2;
   }
   else
   {
      return(TEST_TS_OFFSET_R_SF_US2US * (d_DeltaTimeUs - TEST_MIN_TS_OFFSET_Q_DT_US)) + TEST_TS_OFFSET_MEAS_NOISE_US2;
   }
}

int main (int argc, char *argv[])
{
   os_kf_ctor_type init_params;
   init_params.diverge_threshold = TEST_MIN_TS_OFFSET_DIVERGE_THRESHOLD_US;
   init_params.meas_noise_func = meas_noise;
   init_params.proc_noise_func = proc_noise;
   init_params.min_unc_to_init = TEST_TS_OFFSET_MIN_UNC_TO_INIT_US;
   init_params.n_var_thresh = TEST_TS_OFFSET_N_VAR_THRESH;
   init_params.outlier_limit = TEST_NUM_OUTLIER_KF_RESET;
   init_params.time_update_interval = TEST_TIME_UPDATE_INTERVAL_US;

   if ( os_kf_ctor(&kf, &init_params) != 0 )
   {
      return -1;
   }

   FILE* off_file = fopen("FLU_unfilt.m", "r");
   FILE* sys_file = fopen("FLU_unfilt_sys.m", "r");
   int off_rv;
   int sys_rv;

   do
   {
      double sns_ap_clock_offset = -1;
      uint64_t sys_time = 1;
      off_rv = fscanf(off_file, "%lf", &sns_ap_clock_offset);
      sys_rv = fscanf(sys_file, "%llu", &sys_time);

      os_kf_meas unfilt_meas;
      unfilt_meas.d_Meas = sns_ap_clock_offset;
      unfilt_meas.d_MeasUnc = 0;

      if( off_rv == 1 && sys_rv == 1 )
      {
         /* KF Initialize if not done */
         os_kf_init_filter(kf,
                           &unfilt_meas,
                           sys_time);

         /* KF Update */
         os_kf_filter_update(kf,
                             &unfilt_meas,
                             sys_time);
      }

   }while (off_rv == 1 && sys_rv == 1);

   if ( os_kf_destroy(&kf) != 0 )
   {
      return -1;
   }

   return(0);
}

