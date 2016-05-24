/******************************************************************************
  @file:  os_kf.h
  @brief: 1-State KF w/outlier detection

  DESCRIPTION

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
07/22/11   jb       Initial version

======================================================================*/
#ifndef __OS_KF_H__
#define __OS_KF_H__

#include <stdint.h>

typedef struct os_kf_meas
{
   double d_Meas;
   double d_MeasUnc;
} os_kf_meas;

typedef double (*noise_func)(double d_deltaTime);

typedef struct os_kf_ctor_type
{
   uint8_t outlier_limit;           /* Number of outliers before a reset */
   uint8_t n_var_thresh;            /* Expect/Meas variance threshold scalar */
   uint32_t min_unc_to_init;        /* Min unc allowed to initialize filter */
   uint32_t diverge_threshold;      /* Min Residual diff to check for outlier */
   uint64_t time_update_interval;   /* How often to update the filter */
   noise_func proc_noise_func;      /* Func used to calculate process noise */
   noise_func meas_noise_func;      /* Func used to calculate measurement noise */
} os_kf_ctor_type;

/*===========================================================================
FUNCTION    os_kf_ctor

DESCRIPTION
   Creates space and initializes the KF with the provided parameters.

DEPENDENCIES
   N/A

RETURN VALUE
   0: Success
   else failure

SIDE EFFECTS
   N/A

===========================================================================*/
int os_kf_ctor(void** os_kf_state, os_kf_ctor_type* init_params);

/*===========================================================================
FUNCTION    os_kf_destroy

DESCRIPTION
   Removes space allocated for the KF.

DEPENDENCIES
   N/A

RETURN VALUE
   0: Success
   Otherwise failure

SIDE EFFECTS
   N/A

===========================================================================*/
int os_kf_destroy(void** os_kf_state);

/*===========================================================================
FUNCTION    os_kf_init_filter

DESCRIPTION
   Initializes the filter with the given value if it is not initialized yet.

   unfilt_meas: Unfiltered measurement and uncertainty
   time_update: Time of this measurement in any time base as long as consistent.

DEPENDENCIES
   N/A

RETURN VALUE
   0: Success
   Otherwise failure

SIDE EFFECTS
   N/A

===========================================================================*/
int os_kf_init_filter(void* os_kf_state, os_kf_meas* unfilt_meas, uint64_t time_update);

/*===========================================================================
FUNCTION    os_kf_filter_update

DESCRIPTION
   Updates the filter given the provided unfiltered measurement

   unfilt_meas: Unfiltered measurement and uncertainty
   time_update: Time of this measurement in any time base as long as consistent.

DEPENDENCIES
   N/A

RETURN VALUE
   0: Success
   Otherwise failure

SIDE EFFECTS
   N/A

===========================================================================*/
int os_kf_filter_update(void* os_kf_state, os_kf_meas* unfilt_meas, uint64_t time_update);

/*===========================================================================
FUNCTION    os_kf_get_filt_meas

DESCRIPTION
   Utility function to return the current system time in microseconds.

   filt_meas: Output param for the current filtered measurement

DEPENDENCIES
   N/A

RETURN VALUE
   0: Success
   Otherwise failure

SIDE EFFECTS
   N/A

===========================================================================*/
int os_kf_get_filt_meas(void* os_kf_state, double* filt_meas);

/*===========================================================================
FUNCTION    os_kf_get_filt_meas_unc

DESCRIPTION
   Utility function to return the current system time in microseconds.

   filt_meas_unc: Output param for the current filtered measurement uncertainty

DEPENDENCIES
   N/A

RETURN VALUE
   0: Success
   Otherwise failure

SIDE EFFECTS
   N/A

===========================================================================*/
int os_kf_get_filt_meas_unc(void* os_kf_state, double* filt_meas_unc);

#endif /* __OS_KF_H__ */
