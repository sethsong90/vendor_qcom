/* led_flash.c
 *
 * Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include "led_flash.h"
#include "sensor_common.h"

/** led_flash_open:
 *    @led_flash_ctrl: address of pointer to
 *                   sensor_led_flash_data_t struct
 *    @subdev_name: LED flash subdev name
 *
 * 1) Allocates memory for LED flash control structure
 * 2) Opens LED flash subdev node
 * 3) Initialize LED hardware by passing control to kernel
 * driver
 *
 * This function executes in module sensor context
 *
 * Return:
 * Success - SENSOR_SUCCESS
 * Failure - SENSOR_FAILURE
 **/

static int32_t led_flash_open(void **led_flash_ctrl, const char *subdev_name)
{
  int32_t rc = SENSOR_SUCCESS;
  sensor_led_flash_data_t *ctrl = NULL;
  struct msm_camera_led_cfg_t cfg;
  char subdev_string[32];

  SLOW("Enter");
  if (!led_flash_ctrl || !subdev_name) {
    SERR("failed sctrl %p subdev name %p",
      led_flash_ctrl, subdev_name);
    return SENSOR_ERROR_INVAL;
  }
  ctrl = malloc(sizeof(sensor_led_flash_data_t));
  if (!ctrl) {
    SERR("failed");
    return SENSOR_FAILURE;
  }
  memset(ctrl, 0, sizeof(sensor_led_flash_data_t));

  snprintf(subdev_string, sizeof(subdev_string), "/dev/%s", subdev_name);
  SLOW("sd name %s", subdev_string);
  /* Open subdev */
  ctrl->fd = open(subdev_string, O_RDWR);
  if (ctrl->fd < 0) {
    SERR("failed");
    rc = SENSOR_FAILURE;
    goto ERROR;
  }

  ctrl->rer_cfg = malloc(sizeof(red_eye_reduction_type));
  if (!ctrl->rer_cfg) {
    SERR("Failed - malloc rer_cfg");
  } else {
    memset(ctrl->rer_cfg, 0, sizeof(red_eye_reduction_type));
  }

  cfg.cfgtype = MSM_CAMERA_LED_INIT;
  rc = ioctl(ctrl->fd, VIDIOC_MSM_FLASH_LED_DATA_CFG, &cfg);
  if (rc < 0) {
    SERR("VIDIOC_MSM_FLASH_LED_DATA_CFG failed %s", strerror(errno));
    goto ERROR;
  }

  *led_flash_ctrl = (void *)ctrl;
  SLOW("Exit");
  return rc;

ERROR:
  free(ctrl);
  return rc;
}

/** led_flash_process:
 *    @led_flash_ctrl: LED flash control handle
 *    @event: configuration event type
 *    @data: NULL
 *
 * Handled all LED flash trigger events and passes control to
 * kernel to configure LED hardware
 *
 * This function executes in sensor module context
 *
 * Return:
 * Success - SENSOR_SUCCESS
 * Failure - SENSOR_FAILURE
 **/

static int32_t led_flash_process(void *led_flash_ctrl,
  sensor_submodule_event_type_t event, void *data)
{
  int32_t rc = SENSOR_SUCCESS;
  sensor_led_flash_data_t *ctrl = (sensor_led_flash_data_t *)led_flash_ctrl;
  red_eye_reduction_type  *rer_cfg = NULL;
  module_sensor_params_t *led_module_params = NULL;
  red_eye_reduction_type  *rer_chromatix = NULL;
  int32_t *mode = 0;

  struct msm_camera_led_cfg_t cfg;

  if (!led_flash_ctrl) {
    SERR("failed");
    return SENSOR_FAILURE;
  }

  rer_cfg = ((sensor_led_flash_data_t *)led_flash_ctrl)->rer_cfg;

  switch (event) {
  case LED_FLASH_GET_RER_CHROMATIX: {
    rer_chromatix = (red_eye_reduction_type *)data;
    // Get (RER) data from chromatix
    rc = led_flash_rer_get_chromatix(rer_cfg, rer_chromatix);
    return rc;
  }
  case LED_FLASH_SET_RER_PARAMS: {
    mode = (int32_t *)data;
    rc = led_flash_rer_set_parm(rer_cfg, *mode);
    return rc;
  }
  case LED_FLASH_SET_RER_PROCESS: {
    led_module_params = (module_sensor_params_t *)data;
    rc = led_flash_rer_sequence_process(rer_cfg, led_module_params);
    return rc;
  }
  case LED_FLASH_SET_OFF:
    cfg.cfgtype = MSM_CAMERA_LED_OFF;
    break;
  case LED_FLASH_SET_LOW:
    cfg.cfgtype = MSM_CAMERA_LED_LOW;
    break;
  case LED_FLASH_SET_HIGH:
    cfg.cfgtype = MSM_CAMERA_LED_HIGH;
    break;
  default:
    SERR("invalid event %d", event);
    return SENSOR_FAILURE;
  }
  rc = ioctl(ctrl->fd, VIDIOC_MSM_FLASH_LED_DATA_CFG, &cfg);
  if (rc < 0) {
    SERR("VIDIOC_MSM_FLASH_LED_DATA_CFG failed %s", strerror(errno));
    rc = SENSOR_FAILURE;
  }
  return rc;
}

/** led_flash_close:
 *    @led_flash_ctrl: LED flash control handle
 *
 * 1) Release LED flash hardware
 * 2) Close fd
 * 3) Free LED flash control structure
 *
 * This function executes in sensor module context
 *
 * Return:
 * Success - SENSOR_SUCCESS
 * Failure - SENSOR_FAILURE
 **/

static int32_t led_flash_close(void *led_flash_ctrl)
{
  int32_t rc = SENSOR_SUCCESS;
  sensor_led_flash_data_t *ctrl = (sensor_led_flash_data_t *)led_flash_ctrl;
  struct msm_camera_led_cfg_t cfg;

  SLOW("Enter");
  cfg.cfgtype = MSM_CAMERA_LED_RELEASE;
  rc = ioctl(ctrl->fd, VIDIOC_MSM_FLASH_LED_DATA_CFG, &cfg);
  if (rc < 0) {
    SERR("VIDIOC_MSM_FLASH_LED_DATA_CFG failed %s",
      strerror(errno));
  }

  /* close subdev */
  close(ctrl->fd);

  // Free rer_cfg
  if (ctrl->rer_cfg != NULL) {
    free(ctrl->rer_cfg);
  }

  free(ctrl);
  SLOW("Exit");
  return rc;
}

/** led_flash_sub_module_init:
 *    @func_tbl: pointer to sensor function table
 *
 * Initialize function table for LED flash to be used
 *
 * This function executes in sensor module context
 *
 * Return:
 * Success - SENSOR_SUCCESS
 * Failure - SENSOR_FAILURE
 **/

int32_t led_flash_sub_module_init(sensor_func_tbl_t *func_tbl)
{
  if (!func_tbl) {
    SERR("failed");
    return SENSOR_FAILURE;
  }
  func_tbl->open = led_flash_open;
  func_tbl->process = led_flash_process;
  func_tbl->close = led_flash_close;
  return SENSOR_SUCCESS;
}

/** led_flash_rer_get_chromatix:
 *    @rer_cfg: Internal flash data for RER
 *    @rer_chromatix: RER data from Chromatix
 *
 * 1) Get Red eye reduction (RER) data from chromatix
 *
 * Return:
 * Success - SENSOR_SUCCESS
 * Failure - SENSOR_FAILURE
 **/

int32_t led_flash_rer_get_chromatix(
  red_eye_reduction_type *rer_cfg,
  red_eye_reduction_type *rer_chromatix)
{
  int led_flash_enable;

  if ((rer_cfg == NULL) ||
      (rer_chromatix == NULL)) {
    // Check input
    SERR("Red Eye Reduction process Skip ->\n \
          rer_cfg           = 0x%08x \n \
          rer_chromatix     = 0x%08x \n",
          (unsigned int)rer_cfg,
          (unsigned int)rer_chromatix);
    return SENSOR_FAILURE;
  }

  // Save red_eye_reduction_led_flash_enable state
  led_flash_enable = rer_cfg->red_eye_reduction_led_flash_enable;
  memcpy(rer_cfg, rer_chromatix, sizeof(red_eye_reduction_type));
  // Restore red_eye_reduction_led_flash_enable state
  rer_cfg->red_eye_reduction_led_flash_enable = led_flash_enable;

  return SENSOR_SUCCESS;
}

/** input_check:
 *    @value: Input value
 *    @min: Minimum value
 *    @max: Maximum value
 *
 * Return:
 * Success - SENSOR_SUCCESS
 * Failure - SENSOR_FAILURE
 **/

inline int input_check(float value, float min, float max)
{
  if(value < min)
    return SENSOR_FAILURE;
  else if(value > max)
    return SENSOR_FAILURE;
  else
    return SENSOR_SUCCESS;
}

/** led_flash_rer_sequence_process:
 *    @led_module_params: Led module parameters
 *    @rer_cfg: Internal flash data for RER
 *
 * 1) Execute Red eye reduction (RER) sequence
 *
 * Return:
 * Success - SENSOR_SUCCESS
 * Failure - SENSOR_FAILURE
 **/

int32_t led_flash_rer_sequence_process(
  red_eye_reduction_type *rer_cfg,
  module_sensor_params_t *led_module_params)
{
  int32_t rc = SENSOR_SUCCESS;
  int led_flash_enable;
  int preflash_cycles;
  int LED_pulse_duration_ms;
  int interval_pulese_ms;
  int LED_current_mA;

  if ((led_module_params == NULL) ||
      (rer_cfg == NULL)) {
    // Check input
    SERR("Red Eye Reduction process Skip ->\n \
        led_module_params = 0x%08x \n \
        rer_cfg           = 0x%08x \n",
        (unsigned int)led_module_params,
        (unsigned int)rer_cfg
    );
    return SENSOR_FAILURE;
  }

  // Get chromatix RER data from rer_cfg
  led_flash_enable = rer_cfg->red_eye_reduction_led_flash_enable;
  preflash_cycles  = rer_cfg->number_of_preflash_cycles;
  LED_pulse_duration_ms = rer_cfg->preflash_LED_pulse_duration;
  interval_pulese_ms = rer_cfg->preflash_interval_between_pulese;
  LED_current_mA = rer_cfg->preflash_LED_current;

  if (led_flash_enable == 1) {
    // Red eye procedure is Enabled
    int rc = SENSOR_SUCCESS;
    int sequence_time = ((preflash_cycles
        * (LED_pulse_duration_ms + interval_pulese_ms))
        + RER_PUPIL_CONTRACT_TIME);

    // Check Red Eye Tuning parameters
    rc += input_check(preflash_cycles, PREFLASH_CYCLES_MIN, PREFLASH_CYCLES_MAX);
    rc += input_check(LED_pulse_duration_ms, LED_ON_MS_MIN, LED_ON_MS_MAX);
    rc += input_check(interval_pulese_ms, LED_OFF_MS_MIN, LED_OFF_MS_MAX);
    rc += input_check(sequence_time, RER_DURATION_MS_MIN, RER_DURATION_MS_MAX);

    if (rc < 0) {
      SERR("Error: RER parameters out of range \n");
      return SENSOR_FAILURE;
    }

    // RER procedure
    while (preflash_cycles) {
      led_module_params->func_tbl.process(
        led_module_params->sub_module_private,
        LED_FLASH_SET_HIGH , NULL);

      usleep(LED_pulse_duration_ms*1000);

      led_module_params->func_tbl.process(
        led_module_params->sub_module_private,
        LED_FLASH_SET_OFF , NULL);

      usleep(interval_pulese_ms*1000);

      preflash_cycles--;
    }
    // Pupil contraction time
    usleep(RER_PUPIL_CONTRACT_TIME*1000);
  }

  return SENSOR_SUCCESS;
}

/** led_flash_rer_set_parm:
 *    @rer_cfg:
 *    @mode:
 *
 * 1) Execute Red eye reduction (RER) sequence
 *
 * Return:
 * Success - SENSOR_SUCCESS
 * Failure - SENSOR_FAILURE
 **/

int32_t led_flash_rer_set_parm(
  red_eye_reduction_type *rer_cfg,
  int32_t mode)
{
  if (rer_cfg) {
    // Enable/Disable - Red Eye Reduction procedure (RER)
    rer_cfg->red_eye_reduction_led_flash_enable = mode;
    rer_cfg->red_eye_reduction_xenon_strobe_enable = mode;
  } else {
    return SENSOR_FAILURE;
  }
  return SENSOR_SUCCESS;
}
