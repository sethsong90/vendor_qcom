/* is_set.c
 *
 * Copyright (c) 2013 - 2014 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include "is.h"


#if 1
#undef CDBG
#define CDBG ALOGE
#endif


/** is_set_parameters:
 *    @param: information about parameter to be set
 *    @is_info: IS information
 *
 * Returns TRUE ons success
 **/
boolean is_set_parameters(is_set_parameter_t *param, is_info_t *is_info)
{
  boolean rc = TRUE;

  switch (param->type) {
  case IS_SET_PARAM_STREAM_CONFIG:
    CDBG("%s: IS_SET_PARAM_STREAM_CONFIG, ma = %u, p = %d", __func__,
      param->u.is_sensor_info.sensor_mount_angle,
      param->u.is_sensor_info.camera_position);
    is_info->sensor_mount_angle = param->u.is_sensor_info.sensor_mount_angle;
    is_info->camera_position = param->u.is_sensor_info.camera_position;
    break;

  case IS_SET_PARAM_DIS_CONFIG:
    CDBG("%s: IS_SET_PARAM_DIS_CONFIG", __func__);
    break;

  case IS_SET_PARAM_IS_ENABLE:
    is_info->is_enabled = param->u.is_enable;
    CDBG("%s: IS_SET_PARAM_IS_ENABLE, IS enable = %u", __func__,
      is_info->is_enabled);
    break;

  default:
    rc = FALSE;
    break;
  }

  return rc;
} /* is_set_parameters */

