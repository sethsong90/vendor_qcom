/*============================================================================

  Copyright (c) 2012-2013 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

============================================================================*/
#ifndef __LED_FLASH_H__
#define __LED_FLASH_H__

#include "sensor_common.h"

int32_t led_flash_rer_get_chromatix(
  red_eye_reduction_type *rer_cfg,
  red_eye_reduction_type *rer_chromatix);

int32_t led_flash_rer_sequence_process(
  red_eye_reduction_type *rer_cfg,
  module_sensor_params_t *led_module_params);

int32_t led_flash_rer_set_parm(
  red_eye_reduction_type *rer_cfg,
  int32_t mode);


// Red Eye Reduction (RER) Timing Limitations
#define PREFLASH_CYCLES_MIN       (1)       // [times]
#define PREFLASH_CYCLES_MAX       (200)     // [times]
#define LED_ON_MS_MIN             (1)       // [ms]
#define LED_ON_MS_MAX             (200)     // [ms]
#define LED_OFF_MS_MIN            (1)       // [ms]
#define LED_OFF_MS_MAX            (200)     // [ms]
#define RER_DURATION_MS_MIN       (10)      // [ms]
#define RER_DURATION_MS_MAX       (2000)    // [ms]
#define RER_PUPIL_CONTRACT_TIME   (15)     // [ms]

typedef struct {
  int            fd;
  red_eye_reduction_type *rer_cfg;
} sensor_led_flash_data_t;

#endif
