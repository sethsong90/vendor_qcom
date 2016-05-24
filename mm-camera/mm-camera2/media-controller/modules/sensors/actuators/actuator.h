/*============================================================================

  Copyright (c) 2012-2013 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

============================================================================*/
#ifndef __ACTUATOR_H__
#define __ACTUATOR_H__

typedef struct {
  af_tune_parms_t af_tune;
} actuator_ctrl_t;

typedef struct {
  int32_t fd;
  actuator_ctrl_t *ctrl;
  int16_t curr_step_pos;
  int16_t cur_restore_pos;
  uint16_t total_steps;
  uint8_t is_af_supported;
  uint8_t cam_name;
  uint8_t load_params;
} actuator_data_t;

typedef struct {
  uint8_t af_support;
  af_tune_parms_t *af_tune_ptr;
} actuator_get_t;

#endif
