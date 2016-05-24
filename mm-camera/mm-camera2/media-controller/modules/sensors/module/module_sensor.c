/* module_sensor.c
 *
 * Copyright (c) 2012-2013 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include <linux/media.h>
#include "mct_module.h"
#include "module_sensor.h"
#include "modules.h"
#include "mct_stream.h"
#include "mct_pipeline.h"
#include "media_controller.h"
#include "mct_event_stats.h"
#include "af_tuning.h"
#include "port_sensor.h"
#include "sensor_util.h"
#include <poll.h>
#include <../stats/q3a/q3a_stats_hw.h>
#include "led_flash/led_flash.h"

/** Initialization table **/
static int32_t (*sub_module_init[])(sensor_func_tbl_t *) = {
  [SUB_MODULE_SENSOR]       = sensor_sub_module_init,
  [SUB_MODULE_CHROMATIX]    = chromatix_sub_module_init,
  [SUB_MODULE_ACTUATOR]     = actuator_sub_module_init,
  [SUB_MODULE_EEPROM]       = eeprom_sub_module_init,
  [SUB_MODULE_LED_FLASH]    = led_flash_sub_module_init,
  [SUB_MODULE_STROBE_FLASH] = strobe_flash_sub_module_init,
  [SUB_MODULE_CSIPHY]       = csiphy_sub_module_init,
  [SUB_MODULE_CSIPHY_3D]    = csiphy_sub_module_init,
  [SUB_MODULE_CSID]         = csid_sub_module_init,
  [SUB_MODULE_CSID_3D]      = csid_sub_module_init,
};


static boolean module_sensor_pass_op_clk_change(void *data1, void *data2)
{
  int32_t                     rc = 0;
  module_sensor_bundle_info_t *s_bundle = (module_sensor_bundle_info_t *)data1;
  module_sensor_params_t      *sensor = NULL;

  if (!s_bundle || !data2) {
    SERR("failed: s_bundle %p data2 %p", s_bundle, data2);
    /* Return TRUE here, else mct_list_traverse will terminate */
    return TRUE;
  }

  sensor = s_bundle->module_sensor_params[SUB_MODULE_SENSOR];
  rc = sensor->func_tbl.process(sensor->sub_module_private,
    SENSOR_SET_OP_PIXEL_CLK_CHANGE, data2);
  if (rc < 0) {
    SERR("failed");
  }

  return TRUE;
}



/** module_sensor_handle_pixel_clk_change: handle pixel clk
 *  change event sent by ISP
 *
 *  @module: sensor module
 *  @data: event control data
 *
 *  This function handles stores op pixel clk value in module
 *  private
 *
 *  Return: TRUE for success and FALSE for failure
 *  **/

boolean module_sensor_handle_pixel_clk_change(mct_module_t *module,
  uint32_t identity, void *data)
{
  int32_t                     rc = 0;
  boolean                     ret = TRUE;
  module_sensor_ctrl_t        *module_ctrl = NULL;
  uint32_t                    i = 0;
  module_sensor_params_t      *sensor = NULL;
  sensor_bundle_info_t         bundle_info;

  if (!module || !data) {
    SERR("failed: module %p data %p", module, data);
    return FALSE;
  }

  module_ctrl = (module_sensor_ctrl_t *)module->module_private;
  if (!module_ctrl) {
    SERR("failed: module_ctrl %p", module_ctrl);
    return FALSE;
  }

  mct_list_traverse(module_ctrl->sensor_bundle,
    module_sensor_pass_op_clk_change, data);

  return ret;
}

/** module_sensors_subinit: sensor module init function
 *
 *  @data: sensor bundle data for first sensor
 *  @user_data: NULL
 *
 *  Return: 0 for success and negative error for failure
 *
 *  This function allocates memory to hold module_sensor_prams_t struct for each
 *  sub module and calls init to initialize function table **/

static boolean module_sensors_subinit(void *data, void *user_data)
{
  int32_t rc = SENSOR_SUCCESS, i = 0;
  module_sensor_bundle_info_t *s_bundle = (module_sensor_bundle_info_t *)data;
  if (!s_bundle) {
    SERR("failed");
    return FALSE;
  }
  s_bundle->sensor_lib_params = malloc(sizeof(sensor_lib_params_t));
  if (!s_bundle->sensor_lib_params) {
    SERR("failed");
    return FALSE;
  }
  memset(s_bundle->sensor_lib_params, 0, sizeof(sensor_lib_params_t));

  for (i = 0; i < SUB_MODULE_MAX; i++) {
    s_bundle->module_sensor_params[i] = malloc(sizeof(module_sensor_params_t));
    if (!s_bundle->module_sensor_params[i]) {
      SERR("failed");
      goto ERROR;
    }
    memset(s_bundle->module_sensor_params[i], 0,
      sizeof(module_sensor_params_t));
    if (s_bundle->sensor_info->subdev_id[i] != -1) {
      SLOW("i %d subdev name %s strlen %d", i, s_bundle->sensor_sd_name[i],
        strlen(s_bundle->sensor_sd_name[i]));
      if (!strlen(s_bundle->sensor_sd_name[i]) &&
         ((i == SUB_MODULE_ACTUATOR) || (i == SUB_MODULE_EEPROM) ||
          (i == SUB_MODULE_LED_FLASH) || (i == SUB_MODULE_STROBE_FLASH))) {
        SERR("session %d subdev %d not present, reset to -1",
          s_bundle->sensor_info->session_id, i);
        s_bundle->sensor_info->subdev_id[i] = -1;
      } else {
        rc = sub_module_init[i](&s_bundle->module_sensor_params[i]->func_tbl);
        if (rc < 0 || !s_bundle->module_sensor_params[i]->func_tbl.open ||
            !s_bundle->module_sensor_params[i]->func_tbl.process ||
            !s_bundle->module_sensor_params[i]->func_tbl.close) {
          SERR("failed");
          goto ERROR;
        }
      }
    }
  }
  return TRUE;

ERROR:
  for (i--; i >= 0; i--)
    free(s_bundle->module_sensor_params[i]);
  SERR("failed");
  return FALSE;
}

/** module_sensor_init_session: init session function for sensor
 *
 *  @s_bundle: sensor bundle pointer pointing to the sensor
 *             for which stream is added
 *
 *  Return: 0 for success and negative error for failure
 *
 *  When called first time, this function
 *  1) opens all sub modules to open subdev node
 *  2) loads sensor library
 *  3) calls init on sensor, csiphy and csid. Has ref count to
 *  ensure that actual add stream sequence is executed only
 *  once **/

static boolean module_sensor_init_session(module_sensor_bundle_info_t *s_bundle)
{
  int32_t rc = SENSOR_SUCCESS, i = 0;
  module_sensor_params_t *module_sensor_params = NULL;
  module_sensor_params_t *actuator_module_params = NULL;
  module_sensor_params_t *csiphy_module_params = NULL;
  module_sensor_params_t *csid_module_params = NULL;
  module_sensor_params_t *eeprom_module_params = NULL;
  sensor_get_t sensor_get;
  sensor_lens_info_t lens_info;

  if (s_bundle->ref_count++) {
    SLOW("ref_count %d", s_bundle->ref_count);
    return TRUE;
  }

  /* Initialize max width, height and stream on count */
  s_bundle->max_width = 0;
  s_bundle->max_height = 0;
  s_bundle->stream_on_count = 0;
  s_bundle->stream_mask = 0;
  s_bundle->last_idx = 0;
  s_bundle->num_skip = 4;
  s_bundle->state = 0;
  s_bundle->regular_led_trigger = 0;
  s_bundle->regular_led_af = 0;
  s_bundle->torch_on = 0;

  for (i = 0; i < SUB_MODULE_MAX; i++) {
    SLOW("sensor_sd_name=%s", s_bundle->sensor_sd_name[i]);
    if (s_bundle->module_sensor_params[i]->func_tbl.open) {
      rc = s_bundle->module_sensor_params[i]->func_tbl.open(
        &s_bundle->module_sensor_params[i]->sub_module_private,
        s_bundle->sensor_sd_name[i]);
      if (rc < 0) {
        SERR("failed rc %d", rc);
        goto ERROR1;
      }
    }
  }

  /* Load sensor library*/
  rc = sensor_load_library(s_bundle->sensor_info->sensor_name,
    s_bundle->sensor_lib_params);
  if (rc < 0) {
    SERR("failed rc %d", rc);
    goto ERROR1;
  }

  module_sensor_params = s_bundle->module_sensor_params[SUB_MODULE_SENSOR];
  eeprom_module_params = s_bundle->module_sensor_params[SUB_MODULE_EEPROM];
  /* set sensor_lib_params to sensor sub module */
  rc = module_sensor_params->func_tbl.process(
    module_sensor_params->sub_module_private,
    SENSOR_SET_LIB_PARAMS, s_bundle->sensor_lib_params);
  if (rc < 0) {
    SERR("failed rc %d", rc);
    goto ERROR2;
  }

  /* set sensor_init_params to sensor sub module */
  rc = module_sensor_params->func_tbl.process(
    module_sensor_params->sub_module_private,
    SENSOR_SET_INIT_PARAMS, s_bundle->sensor_init_params);
  if (rc < 0) {
    SERR("failed rc %d", rc);
    goto ERROR2;
  }

  /* set sensor_info to sensor sub module */
  rc = module_sensor_params->func_tbl.process(
    module_sensor_params->sub_module_private,
    SENSOR_SET_SUBDEV_INFO, s_bundle->sensor_info);
  if (rc < 0) {
    SERR("failed rc %d", rc);
    goto ERROR2;
  }

  s_bundle->sensor_params.aperture_value = 0;
  memset(&lens_info, 0, sizeof(lens_info));
  /* get lens info to sensor sub module */
  rc = module_sensor_params->func_tbl.process(
    module_sensor_params->sub_module_private,
    SENSOR_GET_LENS_INFO, &lens_info);
  if (rc < 0) {
    SERR("failed rc %d", rc);
  } else {
    /* Fill aperture */
    s_bundle->sensor_params.aperture_value = lens_info.f_number;
    SLOW("aperture %f", s_bundle->sensor_params.aperture_value);
  }

   /* set eeeprom data */
/* need to check whether to set the data loaded from kernel
   to be user space should be used or eeprom gets data during open close??
   also */

  if (s_bundle->sensor_info->subdev_id[SUB_MODULE_EEPROM] != -1) {
    rc = eeprom_module_params->func_tbl.process(
      eeprom_module_params->sub_module_private,
      EEPROM_SET_BYTESTREAM, &(s_bundle->eeprom_data->eeprom_params));
      if (rc < 0) {
        SERR("failed rc %d", rc);
      }
  }

  /* get eeeprom data */
  /* af_actuator_init */
  /* get actuator header */
  /* sensor init */
  rc = module_sensor_params->func_tbl.process(
    module_sensor_params->sub_module_private,
    SENSOR_INIT, NULL);
  if (rc < 0) {
    SERR("failed rc %d", rc);
    goto ERROR2;
  }

  /* get eeprom data */

  if (s_bundle->sensor_info->subdev_id[SUB_MODULE_ACTUATOR] != -1) {
    actuator_module_params =
      s_bundle->module_sensor_params[SUB_MODULE_ACTUATOR];
    /* af_actuator_init */
    rc = actuator_module_params->func_tbl.process(
      actuator_module_params->sub_module_private,
      ACTUATOR_INIT, NULL);
    if (rc < 0) {
      SERR("failed rc %d", rc);
      goto ERROR2;
    }

  }
  /* get actuator header */

  /* Get CSI lane params from sensor */
  rc = module_sensor_params->func_tbl.process(
    module_sensor_params->sub_module_private,
    SENSOR_GET_CSI_LANE_PARAMS, &sensor_get);
  if (rc < 0) {
    SERR("failed rc %d", rc);
    goto ERROR2;
  }

  csiphy_module_params = s_bundle->module_sensor_params[SUB_MODULE_CSIPHY];
  /* Set csi lane params on csiphy */
  rc = csiphy_module_params->func_tbl.process(
    csiphy_module_params->sub_module_private,
    CSIPHY_SET_LANE_PARAMS, sensor_get.csi_lane_params);
  if (rc < 0) {
    SERR("failed rc %d", rc);
    goto ERROR2;
  }

  csid_module_params = s_bundle->module_sensor_params[SUB_MODULE_CSID];
  /* Set csi lane params on csid */
  rc = csid_module_params->func_tbl.process(
    csid_module_params->sub_module_private,
    CSID_SET_LANE_PARAMS, sensor_get.csi_lane_params);
  if (rc < 0) {
    SERR("failed rc %d", rc);
    goto ERROR2;
  }
  return TRUE;

ERROR2:
  sensor_unload_library(s_bundle->sensor_lib_params);
ERROR1:
  for (i--; i >= 0; i--) {
    if (s_bundle->module_sensor_params[i]->func_tbl.close) {
      s_bundle->module_sensor_params[i]->func_tbl.close(
        s_bundle->module_sensor_params[i]->sub_module_private);
    }
  }
  s_bundle->ref_count--;
  SERR("failed");
  return FALSE;
}

/** module_sensor_deinit_session: deinit session function for
 *  sensor
 *
 *  @s_bundle: sensor bundle pointer pointing to the sensor
 *             for which stream is added
 *  @port: port
 *
 *  Return: 0 for success and negative error for failure
 *
 *  This function calls close on all sub modules to close
 *  subdev node. Also, unloads sensor library. Has ref count
 *  to ensure that actual close happens only when last stream
 *  is removed */

static boolean module_sensor_deinit_session(
  module_sensor_bundle_info_t *s_bundle)
{
  uint16_t i = 0;

  if (!s_bundle->ref_count) {
    SERR("ref count 0");
    return FALSE;
  }

  if (--s_bundle->ref_count) {
    SLOW("ref_count %d", s_bundle->ref_count);
    return TRUE;
  }

  for (i = 0; i < SUB_MODULE_MAX; i++) {
    if (s_bundle->module_sensor_params[i]->func_tbl.close) {
      s_bundle->module_sensor_params[i]->func_tbl.close(
        s_bundle->module_sensor_params[i]->sub_module_private);
    }
    s_bundle->module_sensor_params[i]->sub_module_private = NULL;
  }
  sensor_unload_library(s_bundle->sensor_lib_params);
  return TRUE;
}

/** module_sensor_start_session:
 *
 *  @module: sensor module
 *  @sessionid: session id
 *
 *  Return: 0 for success and negative error on failure
 **/
static boolean module_sensor_start_session(
  mct_module_t *module, unsigned int sessionid)
{
  module_sensor_ctrl_t        *module_ctrl = NULL;
  mct_list_t                  *s_list = NULL;
  module_sensor_bundle_info_t *s_bundle = NULL;
  boolean                     ret = TRUE;

  SHIGH("session %d", sessionid);
  if (!module) {
    SERR("failed");
    return FALSE;
  }

  module_ctrl = (module_sensor_ctrl_t *)module->module_private;
  if (!module_ctrl) {
    SERR("failed");
    return FALSE;
  }

  /* get the s_bundle from session id */
  s_list = mct_list_find_custom(module_ctrl->sensor_bundle, &sessionid,
    sensor_util_find_bundle);
  if (!s_list) {
    SERR("failed");
    return FALSE;
  }
  s_bundle = (module_sensor_bundle_info_t *)s_list->data;
  if (!s_bundle) {
    SERR("failed");
    return FALSE;
  }

  /* initialize the "torch on" flag to 0 */
  s_bundle->torch_on = 0;

  /* this init session includes
     power up sensor, config init setting */
  ret = module_sensor_init_session(s_bundle);
  if (ret == FALSE) {
    SERR("failed");
    goto ERROR;
  }

  /* Create a Sensor thread */

  ret = sensor_thread_create(module);

  if(ret == FALSE) {
     SERR("failed");
     goto ERROR;
  }

 return TRUE;
ERROR:
  SERR("failed");
  return FALSE;
}

/** module_sensor_stop_session:
 *
 *  @module: sensor module
 *  @sessionid: session id
 *
 *  Return: 0 for success and negative error on failure
 **/
static boolean module_sensor_stop_session(
  mct_module_t *module, unsigned int sessionid)
{
  module_sensor_ctrl_t        *module_ctrl = NULL;
  mct_list_t                  *s_list = NULL;
  module_sensor_bundle_info_t *s_bundle = NULL;
  boolean                     ret = TRUE;

  SHIGH("session %d", sessionid);
  if (!module) {
    SERR("failed");
    return FALSE;
  }
  module_ctrl = (module_sensor_ctrl_t *)module->module_private;
  if (!module_ctrl) {
    SERR("failed");
    return FALSE;
  }

  /* get the s_bundle from session id */
  s_list = mct_list_find_custom(module_ctrl->sensor_bundle, &sessionid,
    sensor_util_find_bundle);
  if (!s_list) {
    SERR("failed");
    return FALSE;
  }
  s_bundle = (module_sensor_bundle_info_t *)s_list->data;
  if (!s_bundle) {
    SERR("failed");
    return FALSE;
  }

  /* this deinit session includes
     power off sensor */
  ret = module_sensor_deinit_session(s_bundle);
  if (ret == FALSE) {
    SERR("failed");
    goto ERROR;
  }
 /* Terminate sensor thread */
  sensor_thread_msg_t msg;
    msg.stop_thread = TRUE;
   int nwrite = 0;
   nwrite = write(module_ctrl->pfd[1], &msg, sizeof(sensor_thread_msg_t));
   if(nwrite < 0)
   {
     SERR("%s: Writing into fd failed",__func__);
   }

  return TRUE;
ERROR:
  SERR("failed");
  return FALSE;
}

static int32_t module_sensor_get_stats_data(mct_module_t *module,
  uint32_t identity, stats_get_data_t* stats_get)
{
  boolean rc;
  memset(stats_get, 0x00, sizeof(stats_get_data_t));
  mct_event_t new_event;
  new_event.type = MCT_EVENT_MODULE_EVENT;
  new_event.identity = identity;
  new_event.direction = MCT_EVENT_DOWNSTREAM;
  new_event.u.module_event.type = MCT_EVENT_MODULE_STATS_GET_DATA;
  new_event.u.module_event.module_event_data = (void *)stats_get;
  rc = sensor_util_post_event_on_src_port(module, &new_event);
  if (rc == FALSE){
    SERR("failed");
    return -EFAULT;
  }
  return 0;
}

static boolean module_sensor_is_ready_for_stream_on(mct_port_t *port,
  mct_event_t *event, sensor_set_res_cfg_t *res_cfg,
  module_sensor_bundle_info_t *s_bundle, int32_t bundle_id)
{
  boolean            is_bundle_started = TRUE;
  mct_stream_info_t* stream_info = (mct_stream_info_t*)
                              event->u.ctrl_event.control_event_data;
  uint32_t identity = event->identity;
  uint32_t session_id, stream_id;
  sensor_util_unpack_identity(identity, &session_id, &stream_id);

  SLOW("session_id=%d, stream_id=%d", session_id, stream_id);

  /* find whether there is any bundle for this session that is already
     streamed ON  */
  is_bundle_started = sensor_util_find_is_any_bundle_started(port);
  SHIGH("any bundle started %d", is_bundle_started);

  /* Find whether this stream belongs to bundle */
  if (bundle_id == -1) {
    /* This stream does not belong to any bundle */

    res_cfg->width = s_bundle->max_width;
    res_cfg->height = s_bundle->max_height;
    res_cfg->stream_mask = s_bundle->stream_mask;

  SHIGH("s_bundle->stream_on_count %d", s_bundle->stream_on_count);
  SHIGH("is_bundle_started %d", is_bundle_started);
    /* Start sensor streaming if this is the first stream ON and
       no other bundle has started */
    if ((s_bundle->stream_on_count == 0) && (is_bundle_started == FALSE)) {
      SLOW("non-bundled stream, count %d dim=%dx%d mask=0x%x",
        s_bundle->stream_on_count, res_cfg->width, res_cfg->height,
        res_cfg->stream_mask);
      s_bundle->stream_on_count++;
      return TRUE;
    } else {
      s_bundle->stream_on_count++;
      return FALSE;
    }
  }
  module_sensor_port_bundle_info_t *bundle_info;
  bundle_info = sensor_util_find_bundle_by_id(port, bundle_id);
  if (bundle_info == NULL) {
    SERR("can't find bundle with id=%d", bundle_id);
    return FALSE;
  }
  bundle_info->stream_on_count++;

  /* update the res_cfg with bundle dim and mask */
  res_cfg->width = s_bundle->max_width;
  res_cfg->height = s_bundle->max_height;
  res_cfg->stream_mask = s_bundle->stream_mask;

  SHIGH("bundle_info->stream_on_count %d", bundle_info->stream_on_count);
  SHIGH("bundle_info->bundle_config.num_of_streams %d",
    bundle_info->bundle_config.num_of_streams);
  SHIGH("s_bundle->stream_on_count %d", s_bundle->stream_on_count);
  SHIGH("is_bundle_started %d", is_bundle_started);
  /* If all streams in this bundle are on, we are ready to stream ON
     provided no other streams which is NOT part of bundle is STREAMED ON */
  if ((bundle_info->stream_on_count ==
       bundle_info->bundle_config.num_of_streams) &&
      (s_bundle->stream_on_count == 0) && (is_bundle_started == FALSE)) {
    SHIGH("stream_on_count=%d, w=%d, h=%d, stream_mask=%x",
      bundle_info->stream_on_count, res_cfg->width, res_cfg->height,
      res_cfg->stream_mask);
    return TRUE;
  }
  return FALSE;
}

static boolean module_sensor_is_ready_for_stream_off(mct_module_t *module,
  mct_event_t *event, module_sensor_bundle_info_t *s_bundle)
{
  boolean            is_bundle_started = TRUE;
  mct_stream_info_t* stream_info = (mct_stream_info_t*)
                              event->u.ctrl_event.control_event_data;
  uint32_t identity = event->identity;
  int32_t bundle_id = -1;
  uint32_t session_id, stream_id;
  sensor_util_unpack_identity(identity, &session_id, &stream_id);
  SLOW("session_id=%d, stream_id=%d",
         session_id, stream_id);
  mct_port_t *port = sensor_util_find_src_port_with_identity(
                          module, identity);
  if (!port) {
    SERR("cannot find matching port with identity=0x%x", identity);
    return FALSE;
  }
  bundle_id = sensor_util_find_bundle_id_for_stream(port, identity);
  /* Find whether this stream is part of bundle */
  if (bundle_id == -1) {
    /* This stream does NOT belong to any bundle */
    s_bundle->stream_on_count--;

    /* find whether there is any bundle for this session that is already
       streamed ON  */
    is_bundle_started = sensor_util_find_is_any_bundle_started(port);
    SHIGH("any bundle started %d", is_bundle_started);

    /* Call sensor stream OFF only when all non bundle streams are streamed
       off AND no bundle is streaming */
    if ((s_bundle->stream_on_count == 0) && (is_bundle_started == FALSE)) {
      SLOW("non-bundled stream, stream count %d",
        s_bundle->stream_on_count);
      return TRUE;
    } else {
      SLOW("non-bundled stream, stream count %d",
        s_bundle->stream_on_count);
      return FALSE;
    }
  }
  module_sensor_port_bundle_info_t* bundle_info;
  bundle_info = sensor_util_find_bundle_by_id(port, bundle_id);
  if (bundle_info == NULL) {
    SERR("can't find bundle with id=%d",
                bundle_id);
    return FALSE;
  }
  /* decrement the counter */
  bundle_info->stream_on_count--;

  /* find whether there is any other bundle for this session that is already
     streamed ON  */
  is_bundle_started = sensor_util_find_is_any_bundle_started(port);
  SHIGH("any bundle started %d", is_bundle_started);

  /* If this stream is the last stream in the bundle to get stream_off,
     do sensor stream off provided no non bundle streams are streaming */
  if ((bundle_info->stream_on_count == 0) && (s_bundle->stream_on_count == 0) &&
      (is_bundle_started == FALSE)) {
    return TRUE;
  }
  SLOW("not needed, count=%d", bundle_info->stream_on_count);
  return FALSE;
}

static boolean modules_sensor_set_new_resolution(mct_module_t *module,
  mct_event_t *event,
  module_sensor_bundle_info_t *s_bundle,
  module_sensor_params_t *module_sensor_params,
  sensor_set_res_cfg_t *stream_on_cfg,
  boolean *is_retry)
{
  int32_t                rc = 0;
  boolean                ret = TRUE;
  sensor_out_info_t      sensor_out_info;
  mct_event_t            new_event;
  uint8_t                i = 0;
  mct_port_t            *port = NULL;
  sensor_src_port_cap_t *port_cap = NULL;

  SHIGH("SENSOR_SET_RESOLUTION %d*%d mask %x", stream_on_cfg->width,
    stream_on_cfg->height, stream_on_cfg->stream_mask);
  rc = module_sensor_params->func_tbl.process(
    module_sensor_params->sub_module_private,
    SENSOR_SET_RESOLUTION, stream_on_cfg);
  if (rc < 0) {
    SERR("failed");
    return FALSE;
  }
  rc = module_sensor_params->func_tbl.process(
    module_sensor_params->sub_module_private,
    SENSOR_GET_RESOLUTION_INFO, &sensor_out_info);
  if (rc < 0) {
    SERR("failed");
    return FALSE;
  }
  sensor_out_info.position = s_bundle->sensor_init_params->position;
  sensor_out_info.sensor_mount_angle =
    s_bundle->sensor_init_params->sensor_mount_angle;
  sensor_out_info.af_lens_info.af_supported =
    (s_bundle->sensor_info->subdev_id[SUB_MODULE_ACTUATOR] != -1) ?
                                                 TRUE : FALSE;


  if (sensor_out_info.meta_cfg.num_meta) {
    new_event.type = MCT_EVENT_MODULE_EVENT;
    new_event.identity = event->identity;
    new_event.direction = MCT_EVENT_DOWNSTREAM;
    new_event.u.module_event.type = MCT_EVENT_MODULE_SENSOR_META_CONFIG;
    new_event.u.module_event.module_event_data =
      (void *)&(sensor_out_info.meta_cfg);
    ret = sensor_util_post_event_on_src_port(module, &new_event);
    if (ret == FALSE) {
      SERR("failed");
      return FALSE;
    }
  }

  sensor_out_info.is_retry = FALSE;
  /* Fill some default format */
  sensor_out_info.fmt = CAM_FORMAT_BAYER_MIPI_RAW_10BPP_GRBG;
  /* Fill format from source port */
  port = sensor_util_find_src_port_with_identity(module, event->identity);
  if (port && port->caps.u.data) {
    port_cap = (sensor_src_port_cap_t *)port->caps.u.data;
    if (port_cap->num_cid_ch == 1) {
      sensor_out_info.fmt = port_cap->sensor_cid_ch[0].fmt;
      SLOW("fmt %d", sensor_out_info.fmt);
    } else if (port_cap->num_cid_ch > 1) {
      for (i = 0; i < port_cap->num_cid_ch; i++) {
        if (port_cap->sensor_cid_ch[i].fmt != CAM_FORMAT_META_RAW_8BIT ||
          port_cap->sensor_cid_ch[i].fmt != CAM_FORMAT_META_RAW_10BIT) {
          sensor_out_info.fmt = port_cap->sensor_cid_ch[i].fmt;
          SLOW("fmt %d", sensor_out_info.fmt);
          break;
        }
      }
    }
  }
  if(s_bundle->regular_led_trigger == 1)
    sensor_out_info.prep_flash_on = TRUE;
  else
    sensor_out_info.prep_flash_on = FALSE;

  new_event.type = MCT_EVENT_MODULE_EVENT;
  new_event.identity = event->identity;
  new_event.direction = MCT_EVENT_DOWNSTREAM;
  new_event.u.module_event.type = MCT_EVENT_MODULE_SET_STREAM_CONFIG;
  new_event.u.module_event.module_event_data = (void *)&sensor_out_info;
  ret = sensor_util_post_event_on_src_port(module, &new_event);

  *is_retry = sensor_out_info.is_retry;

  return ret;
}
/** module_sensor_stream_on: sensor stream on
 *
 *  @module: mct module handle
 *  @event: event associated with stream on
 *  @s_bundle: sensor bundle handle
 *
 *  Return: 0 for success and negative error on failure
 *
 *  This function executes stream on sequence based on the
 *  order provided in sensor library. The possible sequences
 *  are listed in sensor_res_cfg_type_t enum **/

static boolean module_sensor_stream_on(mct_module_t *module,
  mct_event_t *event, module_sensor_bundle_info_t *s_bundle)
{
  boolean                        ret = TRUE;
  int32_t                        rc = SENSOR_SUCCESS;
  int32_t                        i = 0;
  module_sensor_params_t        *module_sensor_params = NULL;
  module_sensor_params_t        *csiphy_module_params = NULL;
  module_sensor_params_t        *csid_module_params = NULL;
  module_sensor_params_t        *chromatix_module_params = NULL;
  module_sensor_params_t        *actuator_module_params = NULL;
  sensor_get_t                   sensor_get;
  struct sensor_res_cfg_table_t *res_cfg_table = NULL;
  mct_stream_info_t* stream_info =
    (mct_stream_info_t*) event->u.ctrl_event.control_event_data;
  sensor_set_res_cfg_t stream_on_cfg;
  int32_t bundle_id = -1;

  SHIGH("ide %x SENSOR_START_STREAM", event->identity);
  mct_port_t *port = sensor_util_find_src_port_with_identity(
                          module, event->identity);
  if (!port) {
    SERR("cannot find matching port with identity=0x%x",
      event->identity);
    return FALSE;
  }
  sensor_util_dump_bundle_and_stream_lists(port, __func__, __LINE__);
  bundle_id = sensor_util_find_bundle_id_for_stream(port, event->identity);
  boolean stream_on_flag = module_sensor_is_ready_for_stream_on(port, event,
    &stream_on_cfg, s_bundle, bundle_id);
  if (!stream_on_flag) {
    SLOW("NO STREAM_ON, dummy excersice");
  } else {
    SLOW("REAL STREAM_ON");
  }

  SLOW("config: dim=%dx%d, mask=0x%x stream type: %d", stream_on_cfg.width,
    stream_on_cfg.height, stream_on_cfg.stream_mask, stream_info->stream_type);

  /* Check whether this is live snapshot stream ON */
  if ((stream_on_cfg.stream_mask & (1 << CAM_STREAM_TYPE_VIDEO)) &&
      (stream_info->stream_type == CAM_STREAM_TYPE_SNAPSHOT)) {
    /* Load live snapshot chromatix */
    ret = sensor_util_load_liveshot_chromatix(module, port, event, s_bundle);
    if (ret == FALSE) {
      SERR("failed");
    }
  }


  if (1 == s_bundle->torch_on) {
    module_sensor_params_t *led_module_params =
       s_bundle->module_sensor_params[SUB_MODULE_LED_FLASH];
    if (led_module_params->func_tbl.process != NULL) {
      rc = led_module_params->func_tbl.process(
        led_module_params->sub_module_private, LED_FLASH_SET_LOW , NULL);
      if (rc < 0) {
        SERR("failed: LED_FLASH_SET_LOW");
      } else {
        s_bundle->sensor_params.flash_mode = CAM_FLASH_MODE_TORCH;
        s_bundle->torch_on = 1;
        sensor_util_post_led_state_msg(module, s_bundle, event->identity);
      }
    }
  }

  if (bundle_id == -1 && stream_on_flag == FALSE) {
    SLOW("propogate stream on event");
    /* Call send_event to propogate event to next module*/
    ret = sensor_util_post_event_on_src_port(module, event);
    if (ret == FALSE) {
      SERR("failed");
      return FALSE;
    }
    return TRUE;
  }

  module_sensor_params = s_bundle->module_sensor_params[SUB_MODULE_SENSOR];
  csiphy_module_params = s_bundle->module_sensor_params[SUB_MODULE_CSIPHY];
  csid_module_params = s_bundle->module_sensor_params[SUB_MODULE_CSID];
  chromatix_module_params =
    s_bundle->module_sensor_params[SUB_MODULE_CHROMATIX];
  if (!module_sensor_params || !csiphy_module_params || !csid_module_params ||
    !chromatix_module_params) {
    SERR("failed");
    return FALSE;
  }
  rc = module_sensor_params->func_tbl.process(
    module_sensor_params->sub_module_private,
    SENSOR_GET_RES_CFG_TABLE, &res_cfg_table);
  if (rc < 0) {
    SERR("failed");
    return FALSE;
  }
  for (i = 0; i < res_cfg_table->size; i++) {
    SLOW("cfg type %d",
      res_cfg_table->res_cfg_type[i]);
    switch (res_cfg_table->res_cfg_type[i]) {
    case SENSOR_SET_STOP_STREAM: {
      rc = module_sensor_params->func_tbl.process(
        module_sensor_params->sub_module_private,
        SENSOR_STOP_STREAM, NULL);
      if (rc < 0) {
        SERR("failed");
        return FALSE;
      }
      break;
    }
    case SENSOR_SET_START_STREAM: {
      mct_event_t new_event;
      float digital_gain=0.0;
      sensor_output_format_t output_format;
      sensor_chromatix_params_t chromatix_params;

      rc = module_sensor_params->func_tbl.process(
        module_sensor_params->sub_module_private,
        SENSOR_GET_SENSOR_FORMAT, &output_format);
      if (rc < 0) {
          SERR("failed");
      } else if (output_format == SENSOR_BAYER) {
        stats_get_data_t stats_get;
        stats_get_data_t *dest_stats_get;
        memset(&stats_get, 0, sizeof(stats_get_data_t));
        /* get initial gain/linecount from AEC */
        rc = module_sensor_get_stats_data(module, event->identity, &stats_get);
        if (rc < 0) {
          SERR("failed");
          return FALSE;
        }
        if ((stats_get.flag & STATS_UPDATE_AEC) == 0x00) {
          /* non-fatal error */
          SERR("Invalid: No AEC update in stats_get");
        } else {
          mct_bus_msg_t bus_msg;
          int32_t bus_index;
          SLOW("bus msg");
          chromatix_params.stream_mask = s_bundle->stream_mask;
          rc = chromatix_module_params->func_tbl.process(
            chromatix_module_params->sub_module_private,
            CHROMATIX_GET_PTR, &chromatix_params);
          if (rc < 0) {
            SERR("failed");
            return FALSE;
          }
          if (!chromatix_params.common_chromatix_ptr ||
              !chromatix_params.chromatix_ptr) {
            SERR("failed common %s chromatix %s",
              sensor_get.chromatix_name.common_chromatix,
              sensor_get.chromatix_name.chromatix);
          } else {
            /* Send bus msg for passing chromatix pointers */
            s_bundle->chromatix_metadata.chromatix_ptr =
              (void *)chromatix_params.chromatix_ptr;
            s_bundle->chromatix_metadata.common_chromatix_ptr =
              (void *)chromatix_params.common_chromatix_ptr;
            bus_msg.sessionid = s_bundle->sensor_info->session_id;
            bus_msg.type = MCT_BUS_MSG_SET_SENSOR_INFO;
            bus_msg.msg = (void *)&s_bundle->chromatix_metadata;
            if(mct_module_post_bus_msg(module, &bus_msg) == FALSE)
              SERR("failed");
            /* Print chromatix pointers */
            SLOW("c %p com c %p", s_bundle->chromatix_metadata.chromatix_ptr,
              s_bundle->chromatix_metadata.common_chromatix_ptr);
            /* Send bus msg for passing AEC trigger update */
            if (sizeof(stats_get_data_t) >
                sizeof(s_bundle->aec_metadata.private_data)) {
              SERR("failed");
            } else {
              memcpy(s_bundle->aec_metadata.private_data, &stats_get,
                sizeof(stats_get_data_t));
              bus_msg.sessionid = s_bundle->sensor_info->session_id;
              bus_msg.type = MCT_BUS_MSG_SET_STATS_AEC_INFO;
              bus_msg.size = sizeof(stats_get_data_t);
              SLOW("bus msg size %d", bus_msg.size);
              bus_msg.msg = (void *)&s_bundle->aec_metadata;
              if(mct_module_post_bus_msg(module, &bus_msg) == FALSE)
                SERR("failed");
              /* Print source stats get data */
              SLOW("source valid entries %d", stats_get.aec_get.valid_entries);
              for (bus_index = 0; bus_index < stats_get.aec_get.valid_entries;
                   bus_index++) {
                SLOW("source g %f lux idx %f",
                  stats_get.aec_get.real_gain[bus_index],
                  stats_get.aec_get.lux_idx);
              }
              /* Print destination stats get data */
              dest_stats_get =
                (stats_get_data_t *)s_bundle->aec_metadata.private_data;
              SLOW("dest valid entries %d",
                dest_stats_get->aec_get.valid_entries);
              for (bus_index = 0;
                   bus_index < dest_stats_get->aec_get.valid_entries;
                   bus_index++) {
                SLOW("dest g %f lux idx %f",
                  dest_stats_get->aec_get.real_gain[bus_index],
                  dest_stats_get->aec_get.lux_idx);
              }
            }
          }
        }
        SLOW("ddd get stats led trigger %d ",s_bundle->regular_led_trigger);
        if (s_bundle->regular_led_trigger == 1)  {
          module_sensor_params_t        *led_module_params = NULL;
          led_module_params = s_bundle->module_sensor_params[SUB_MODULE_LED_FLASH];
          if (led_module_params != NULL &&
              led_module_params->func_tbl.process != NULL) {
            rc = led_module_params->func_tbl.process(
              led_module_params->sub_module_private,
              LED_FLASH_SET_HIGH , NULL);
            if (rc < 0) {
              s_bundle->sensor_params.flash_mode = CAM_FLASH_MODE_OFF;
              SERR("failed: LED_FLASH_SET_HIGH");
            } else {
              s_bundle->sensor_params.flash_mode = CAM_FLASH_MODE_ON;
              SHIGH("%s, post-flash: ON", __func__);
            }
          }
          sensor_util_post_led_state_msg(module, s_bundle, event->identity);
          s_bundle->regular_led_trigger = 0;
        }
        /* set initial exposure settings, before stream_on */
        rc = module_sensor_params->func_tbl.process(
                module_sensor_params->sub_module_private,
                SENSOR_SET_AEC_INIT_SETTINGS, (void*)(&(stats_get.aec_get)));
        if (rc < 0) {
          SERR("failed");
          return FALSE;
        }

        ret = sensor_util_set_digital_gain_to_isp(module, s_bundle,
          event->identity);
        if (ret == FALSE)
          SERR("can't set digital gain");
      } /* if bayer */
      if (stream_on_flag == TRUE) {
        SHIGH("ide %x SENSOR_START_STREAM", event->identity);
        rc = module_sensor_params->func_tbl.process(
          module_sensor_params->sub_module_private, SENSOR_START_STREAM, NULL);
          if (rc < 0) {
            SERR("failed");
            return FALSE;
          }
          mct_bus_msg_t bus_msg;
          bus_msg.sessionid = s_bundle->sensor_info->session_id;
          bus_msg.type = MCT_BUS_MSG_SENSOR_STARTING;
          bus_msg.msg = NULL;
          ALOGE("%s: Sending start bus message\n", __func__);
          if(mct_module_post_bus_msg(module, &bus_msg) == FALSE)
            SERR("failed");
      }
      break;
    }
    case SENSOR_SET_NEW_RESOLUTION: {
      boolean is_retry;
      ret = modules_sensor_set_new_resolution(module, event, s_bundle,
        module_sensor_params, &stream_on_cfg, &is_retry);
      if (ret == FALSE) {
        SERR("failed");
        return FALSE;
      } else {
        if (is_retry == TRUE) {
          ret = modules_sensor_set_new_resolution(module, event, s_bundle,
            module_sensor_params, &stream_on_cfg, &is_retry);
        }
      }
      break;
    }
    case SENSOR_SEND_EVENT:
      /* Call send_event to propogate event to next module*/
      ret = sensor_util_post_event_on_src_port(module, event);
      if (ret == FALSE) {
        SERR("failed");
        return FALSE;
      }
      break;
    case SENSOR_SET_CSIPHY_CFG:
      rc = module_sensor_params->func_tbl.process(
        module_sensor_params->sub_module_private,
        SENSOR_GET_CUR_CSIPHY_CFG, &sensor_get);
      if (rc < 0) {
        SERR("failed");
        return FALSE;
      }

      sensor_get.csiphy_params->csid_core =
      s_bundle->sensor_info->subdev_id[SUB_MODULE_CSID];

      rc = csiphy_module_params->func_tbl.process(
        csiphy_module_params->sub_module_private,
        CSIPHY_SET_CFG, sensor_get.csiphy_params);
      if (rc < 0) {
        SERR("failed");
        return FALSE;
      }
      break;
    case SENSOR_SET_CSID_CFG:

      rc = module_sensor_params->func_tbl.process(
        module_sensor_params->sub_module_private,
        SENSOR_GET_CUR_CSID_CFG, &sensor_get);
      if (rc < 0) {
        SERR("failed");
        return FALSE;
      }
      rc = csid_module_params->func_tbl.process(
        csid_module_params->sub_module_private,
        CSID_SET_CFG, sensor_get.csid_params);
      if (rc < 0) {
        SERR("failed");
        return FALSE;
      }
      break;
    case SENSOR_LOAD_CHROMATIX: {
      mct_event_t new_event;
      sensor_chromatix_params_t chromatix_params;
      modulesChromatix_t module_chromatix;
      mct_event_module_t event_module;
      af_tune_parms_t *af_tune_ptr = NULL;
      module_sensor_params_t *eeprom_module_params = NULL;
      module_sensor_params_t  *led_module_params = NULL;
      red_eye_reduction_type  *rer_chromatix = NULL;

      eeprom_module_params =
        s_bundle->module_sensor_params[SUB_MODULE_EEPROM];
      eeprom_set_chroma_af_t eeprom_set;
      rc = module_sensor_params->func_tbl.process(
        module_sensor_params->sub_module_private,
        SENSOR_GET_CUR_CHROMATIX_NAME, &sensor_get);
      if (rc < 0) {
        SERR("failed");
        return FALSE;
      }
      SLOW("chromatix name %s", sensor_get.chromatix_name.chromatix);
      if (!sensor_get.chromatix_name.chromatix ||
          !sensor_get.chromatix_name.common_chromatix) {
        SERR("failed common %s chromatix %s",
          sensor_get.chromatix_name.common_chromatix,
          sensor_get.chromatix_name.chromatix);
        return FALSE;
      }
      rc = chromatix_module_params->func_tbl.process(
        chromatix_module_params->sub_module_private,
        CHROMATIX_OPEN_LIBRARY, &sensor_get.chromatix_name);
      if (rc < 0) {
        SERR("failed");
        return FALSE;
      }
      chromatix_params.stream_mask = s_bundle->stream_mask;
      rc = chromatix_module_params->func_tbl.process(
        chromatix_module_params->sub_module_private,
        CHROMATIX_GET_PTR, &chromatix_params);
      if (rc < 0) {
        SERR("failed");
        return FALSE;
      }
      if (!chromatix_params.common_chromatix_ptr ||
          !chromatix_params.chromatix_ptr) {
        SERR("failed common %s chromatix %s",
          sensor_get.chromatix_name.common_chromatix,
          sensor_get.chromatix_name.chromatix);
        return FALSE;
      }
      if (s_bundle->sensor_info->subdev_id[SUB_MODULE_ACTUATOR] != -1) {
        actuator_module_params =
          s_bundle->module_sensor_params[SUB_MODULE_ACTUATOR];
        if (actuator_module_params->func_tbl.process) {
          actuator_cam_mode_t cam_mode = ACTUATOR_CAM_MODE_CAMERA;
          cam_stream_type_t stream_type = stream_info->stream_type;
          if(stream_on_cfg.stream_mask & (1 << CAM_STREAM_TYPE_VIDEO))
            cam_mode = ACTUATOR_CAM_MODE_CAMCORDER;

          SLOW("%s:%d stream_type rc %d cam_mode %d stream_type %d %d\n",
            __func__, __LINE__, rc,
            cam_mode, stream_type, stream_on_cfg.stream_mask);
          rc = actuator_module_params->func_tbl.process(
            actuator_module_params->sub_module_private,
            ACTUATOR_LOAD_HEADER, &cam_mode);
          if (rc < 0 ) {
            SERR("%s:%d failed rc %d ctrl_type %d\n", __func__,
            __LINE__, rc, cam_mode);
            return FALSE;
          }
          rc = actuator_module_params->func_tbl.process(
            actuator_module_params->sub_module_private,
            ACTUATOR_GET_AF_TUNE_PTR, &af_tune_ptr);
          if (rc < 0 || !af_tune_ptr) {
            SERR("failed rc %d af_tune_ptr %p", rc, af_tune_ptr);
            return FALSE;
          }
        }
      }
      if (s_bundle->sensor_info->subdev_id[SUB_MODULE_EEPROM] != -1) {
        struct msm_camera_i2c_reg_setting cal_setting;
        int is_insensor;
        eeprom_set.chromatix = chromatix_params;
        eeprom_set.af_tune_ptr = af_tune_ptr;
        rc = eeprom_module_params->func_tbl.process(
          eeprom_module_params->sub_module_private,
          EEPROM_SET_CHROMA_AF_PTR, &eeprom_set);
        if (rc < 0) {
          SERR("failed");
          return FALSE;
        }
        rc = eeprom_module_params->func_tbl.process(
          eeprom_module_params->sub_module_private,
          EEPROM_GET_ISINSENSOR_CALIB, &is_insensor);
        if (rc < 0) {
          SERR("failed");
          return FALSE;
        }
        if(is_insensor) {
          rc = eeprom_module_params->func_tbl.process(
            eeprom_module_params->sub_module_private,
            EEPROM_GET_RAW_DATA, &cal_setting);
          if (rc < 0) {
            SERR("Get raw data failed");
            return FALSE;
          }
          rc = module_sensor_params->func_tbl.process(
            module_sensor_params->sub_module_private,
            SENSOR_SET_CALIBRATION_DATA, &cal_setting);
          if (rc < 0) {
            SERR("Set calibration data failed");
            return FALSE;
          }
        }
      }

      if (s_bundle->sensor_info->subdev_id[SUB_MODULE_ACTUATOR] != -1) {
        actuator_module_params =
          s_bundle->module_sensor_params[SUB_MODULE_ACTUATOR];
        rc = actuator_module_params->func_tbl.process(
          actuator_module_params->sub_module_private,
          ACTUATOR_SET_PARAMETERS, NULL);
        if (rc < 0 ) {
          SERR("%s:%d failed rc %d\n", __func__,
          __LINE__, rc);
          return FALSE;
        }
      }

      if (s_bundle->sensor_info->subdev_id[SUB_MODULE_LED_FLASH] != -1) {
        led_module_params =
            s_bundle->module_sensor_params[SUB_MODULE_LED_FLASH];
        rer_chromatix =
            &(chromatix_params.chromatix_ptr->AEC_algo_data.red_eye_reduction);

        // Get (RER) data from chromatix
        if (led_module_params->func_tbl.process != NULL) {
          led_module_params->func_tbl.process(
            led_module_params->sub_module_private,
            LED_FLASH_GET_RER_CHROMATIX , rer_chromatix);
        }
      }

      module_chromatix.chromatixComPtr =
        chromatix_params.common_chromatix_ptr;
      module_chromatix.chromatixPtr = chromatix_params.chromatix_ptr;
      /* Send chromatix pointer downstream */
      new_event.type = MCT_EVENT_MODULE_EVENT;
      new_event.identity = event->identity;
      new_event.direction = MCT_EVENT_DOWNSTREAM;
      new_event.u.module_event.type = MCT_EVENT_MODULE_SET_CHROMATIX_PTR;
      new_event.u.module_event.module_event_data =
        (void *)&module_chromatix;
      ret = sensor_util_post_event_on_src_port(module, &new_event);
      if (ret == FALSE) {
        SERR("failed");
        return FALSE;
      }

      if (af_tune_ptr != NULL) {
        /* Send af tune pointer downstream */
        new_event.type = MCT_EVENT_MODULE_EVENT;
        new_event.identity = event->identity;
        new_event.direction = MCT_EVENT_DOWNSTREAM;
        new_event.u.module_event.type = MCT_EVENT_MODULE_SET_AF_TUNE_PTR;
        new_event.u.module_event.module_event_data = (void *)af_tune_ptr;
        ret = sensor_util_post_event_on_src_port(module, &new_event);
        if (ret == FALSE) {
         SERR("failed");
         return FALSE;
        }
      }
      break;
    }
    default:
      SERR("invalid event %d", res_cfg_table->res_cfg_type[i]);
      break;
    }
  }
  if (s_bundle->fps_info.max_fps) {
    rc = module_sensor_params->func_tbl.process(
        module_sensor_params->sub_module_private,
        SENSOR_SET_FPS, &s_bundle->fps_info);
    if (rc < 0) {
      SERR("failed");
      return FALSE;
    }
  }

  return TRUE;
}

/** module_sensor_hal_set_parm: process event for
 *  sensor module
 *
 *  @module_sensor_params: pointer to sensor module params
 *  @event_control: pointer to control data that is sent with
 *                 S_PARM
 *
 *  Return: TRUE / FALSE
 *
 *  This function handles  events associated with S_PARM * */
static boolean module_sensor_hal_set_parm(
   module_sensor_params_t *module_sensor_params,
   mct_event_control_parm_t *event_control)
{
   boolean   ret = TRUE;
   int32_t   rc = SENSOR_SUCCESS;

  switch(event_control->type){
    case CAM_INTF_PARM_SATURATION: {
      if (!event_control->parm_data) {
        SERR("failed parm_data NULL");
          ret = FALSE;
          break;
      }
      rc = module_sensor_params->func_tbl.process(
             module_sensor_params->sub_module_private,
             SENSOR_SET_SATURATION, event_control->parm_data);
      if (rc < 0) {
        SERR("failed");
        ret = FALSE;
        break;
      }
    }
      break;

    case CAM_INTF_PARM_CONTRAST: {
      if (!event_control->parm_data) {
        SERR("failed parm_data NULL");
        ret = FALSE;
        break;
      }
      rc = module_sensor_params->func_tbl.process(
             module_sensor_params->sub_module_private,
             SENSOR_SET_CONTRAST, event_control->parm_data);
      if (rc < 0) {
        SERR("failed");
        ret = FALSE;
        break;
      }
    }
      break;

    case CAM_INTF_PARM_SHARPNESS:{
      if (!event_control->parm_data) {
        SERR("failed parm_data NULL");
        ret = FALSE;
        break;
      }
      rc = module_sensor_params->func_tbl.process(
             module_sensor_params->sub_module_private,
             SENSOR_SET_SHARPNESS, event_control->parm_data);
      if (rc < 0) {
        SERR("failed");
        ret = FALSE;
        break;
      }
    }
      break;

    case CAM_INTF_PARM_ISO: {
      if (!event_control->parm_data) {
        SERR("failed parm_data NULL");
        ret = FALSE;
        break;
      }
      rc = module_sensor_params->func_tbl.process(
             module_sensor_params->sub_module_private,
             SENSOR_SET_ISO, event_control->parm_data);
      if (rc < 0) {
        SERR("failed");
        ret = FALSE;
        break;
      }
    }
      break;

    case CAM_INTF_PARM_EXPOSURE_COMPENSATION: {
      if (!event_control->parm_data) {
        SERR("failed parm_data NULL");
        ret = FALSE;
        break;
      }
      rc = module_sensor_params->func_tbl.process(
             module_sensor_params->sub_module_private,
             SENSOR_SET_EXPOSURE_COMPENSATION, event_control->parm_data);
      if (rc < 0) {
        SERR("failed");
        ret = FALSE;
        break;
      }
    }
      break;

    case CAM_INTF_PARM_ANTIBANDING: {
      if (!event_control->parm_data) {
        SERR("failed parm_data NULL");
        ret = FALSE;
        break;
      }
      rc = module_sensor_params->func_tbl.process(
             module_sensor_params->sub_module_private,
             SENSOR_SET_ANTIBANDING, event_control->parm_data);
      if (rc < 0) {
        SERR("failed");
        ret = FALSE;
        break;
      }
    }
      break;

    case CAM_INTF_PARM_BESTSHOT_MODE: {
      if (!event_control->parm_data) {
        SERR("failed parm_data NULL");
        ret = FALSE;
        break;
      }
      rc = module_sensor_params->func_tbl.process(
             module_sensor_params->sub_module_private,
             SENSOR_SET_BESTSHOT_MODE, event_control->parm_data);
      if (rc < 0) {
        SERR("failed");
        ret = FALSE;
        break;
      }
    }
      break;

    case CAM_INTF_PARM_EFFECT: {
      if (!event_control->parm_data) {
        SERR("failed parm_data NULL");
        ret = FALSE;
        break;
      }
      rc = module_sensor_params->func_tbl.process(
             module_sensor_params->sub_module_private,
             SENSOR_SET_EFFECT, event_control->parm_data);
      if (rc < 0) {
        SERR("failed");
        ret = FALSE;
        break;
      }
    }
      break;

    case CAM_INTF_PARM_WHITE_BALANCE: {
      if (!event_control->parm_data) {
        SERR("failed parm_data NULL");
        ret = FALSE;
        break;
      }
      rc = module_sensor_params->func_tbl.process(
             module_sensor_params->sub_module_private,
             SENSOR_SET_WHITE_BALANCE, event_control->parm_data);
      if (rc < 0) {
        SERR("failed");
        ret = FALSE;
        break;
      }
    }
      break;

       default:
       break;
      }
   return ret;
}

/** module_sensor_event_control_set_parm: process event for
 *  sensor module
 *
 *  @s_bundle: pointer to sensor bundle
 *  @control_data: pointer to control data that is sent with
 *               S_PARM
 *
 *  Return: TRUE / FALSE
 *
 *  This function handles all events associated with S_PARM * */

static boolean module_sensor_event_control_set_parm(
   mct_module_t *module, mct_event_t* event,
   sensor_bundle_info_t *bundle)
{
  boolean                      ret = TRUE;
  int32_t                      rc = SENSOR_SUCCESS;
  mct_list_t                  *s_list = NULL;
  module_sensor_bundle_info_t *s_bundle = bundle->s_bundle;
  module_sensor_params_t      *module_sensor_params =
    s_bundle->module_sensor_params[SUB_MODULE_SENSOR];
  if (!module_sensor_params || !event) {
    SERR("failed");
    ret = FALSE;
    goto ERROR;
  }
  mct_event_control_parm_t    *event_control =
    (mct_event_control_parm_t *)(event->u.ctrl_event.control_event_data);

  SLOW("event type =%d", event_control->type);
  switch (event_control->type) {
  case CAM_INTF_PARM_FPS_RANGE:

    {
    if (!event_control->parm_data) {
      SERR("failed parm_data NULL");
      ret = FALSE;
      break;
    }
    s_bundle->fps_info = *(cam_fps_range_t *)event_control->parm_data;
    rc = module_sensor_params->func_tbl.process(
      module_sensor_params->sub_module_private,
      SENSOR_SET_FPS, event_control->parm_data);
    if (rc < 0) {
      SERR("failed");
      ret = FALSE;
      break;
    }
    break;
  }
  case CAM_INTF_PARM_HFR:
    rc = module_sensor_params->func_tbl.process(
            module_sensor_params->sub_module_private,
            SENSOR_SET_HFR_MODE, event_control->parm_data);
    if (rc < 0) {
      SERR("failed");
      ret = FALSE;
    }
    break;
  case CAM_INTF_PARM_HDR:
    rc = module_sensor_params->func_tbl.process(
      module_sensor_params->sub_module_private,
      SENSOR_SET_HDR_AE_BRACKET, event_control->parm_data);
    if (rc < 0) {
      SERR("failed");
      ret = FALSE;
    }
    break;
  case CAM_INTF_PARM_VIDEO_HDR:{
    int32_t tmp = *(int32_t *)event_control->parm_data;
    boolean need_restart = TRUE;
      rc = module_sensor_params->func_tbl.process(
        module_sensor_params->sub_module_private,
        SENSOR_SET_VIDEO_HDR_ENABLE, event_control->parm_data);
      if (rc == SENSOR_FAILURE) {
        SERR("failed");
        ret = FALSE;
      } else if (rc == SENSOR_ERROR_INVAL) {
        /* This indicates that we are already in this mode
           so we do not need stream restart */
        need_restart = FALSE;
      }

      sensor_out_info_t sensor_out_info;
      mct_event_t new_event;
      rc = module_sensor_params->func_tbl.process(
        module_sensor_params->sub_module_private,
        SENSOR_GET_RESOLUTION_INFO, &sensor_out_info);
      if (rc < 0) {
        SERR("failed");
        return FALSE;
      }

      if((s_bundle->stream_on_count > 0) && need_restart) {
        mct_bus_msg_t bus_msg;
        mct_bus_msg_error_message_t cmd;

        cmd = MCT_ERROR_MSG_RSTART_VFE_STREAMING;

        bus_msg.sessionid = bundle->session_id;
        bus_msg.type = MCT_BUS_MSG_ERROR_MESSAGE;
        bus_msg.size = sizeof(cmd);
        bus_msg.msg = &cmd;
        ALOGE("psiven %s Restart",__func__);
        if(!mct_module_post_bus_msg(module, &bus_msg))
          SERR("Failed to send message to bus");
      }
    }
    break;
  case CAM_INTF_PARM_DIS_ENABLE:
    rc = module_sensor_params->func_tbl.process(
      module_sensor_params->sub_module_private,
      SENSOR_SET_DIS_ENABLE, event_control->parm_data);
    if (rc < 0) {
      SERR("failed");
      ret = FALSE;
    }
    break;
  case CAM_INTF_PARM_LED_MODE: {
    int32_t mode = *((int32_t *)event_control->parm_data);
    module_sensor_params_t        *led_module_params = NULL;
    led_module_params = s_bundle->module_sensor_params[SUB_MODULE_LED_FLASH];
    SERR("CAM_INTF_PARM_LED_MODE %d \n", mode);
    if (mode == LED_MODE_TORCH) {
      if (led_module_params->func_tbl.process != NULL) {
        rc = led_module_params->func_tbl.process(
          led_module_params->sub_module_private,
          LED_FLASH_SET_LOW , NULL);
        if (rc < 0) {
          SERR("failed: LED_FLASH_SET_LOW");
        } else {
          s_bundle->sensor_params.flash_mode = CAM_FLASH_MODE_TORCH;
          s_bundle->torch_on = 1;
          sensor_util_post_led_state_msg(module, s_bundle, event->identity);
        }
      }
    } else {
      if (led_module_params->func_tbl.process != NULL) {
        rc = led_module_params->func_tbl.process(
          led_module_params->sub_module_private,
          LED_FLASH_SET_OFF , NULL);
        if (rc < 0) {
          SERR("failed: LED_FLASH_SET_OFF");
        } else {
          s_bundle->sensor_params.flash_mode = CAM_FLASH_MODE_OFF;
          s_bundle->torch_on = 0;
          sensor_util_post_led_state_msg(module, s_bundle, event->identity);
        }
      }
    }
    break;
  }

  case CAM_INTF_PARM_MAX_DIMENSION:
    SERR("CAM_INTF_PARM_MAX_DIMENSION");
    rc = module_sensor_params->func_tbl.process(
      module_sensor_params->sub_module_private,
      SENSOR_SET_MAX_DIMENSION, event_control->parm_data);
    if (rc < 0) {
      SERR("failed");
      ret = FALSE;
    }
    break;

  case CAM_INTF_PARM_SET_AUTOFOCUSTUNING: {
    module_sensor_params_t      *actuator_module_params =
          s_bundle->module_sensor_params[SUB_MODULE_ACTUATOR];
    if (!actuator_module_params) {
      SERR("failed");
      ret = FALSE;
      goto ERROR;
    }
    if (s_bundle->sensor_info->subdev_id[SUB_MODULE_ACTUATOR] != -1) {
      actuator_module_params->func_tbl.process(
        actuator_module_params->sub_module_private,
        ACTUATOR_FOCUS_TUNING, event_control->parm_data);
      if (rc < 0) {
        SERR("failed");
        ret = FALSE;
      }
    }
  }
    break;

  case CAM_INTF_PARM_SET_RELOAD_CHROMATIX: {
    module_sensor_params_t      *chromatix_module_params =
      s_bundle->module_sensor_params[SUB_MODULE_CHROMATIX];
    sensor_chromatix_params_t chromatix_params;
    modulesChromatix_t module_chromatix;
    mct_event_module_t event_module;
    mct_event_t new_event;
    if (!event_control->parm_data) {
      SERR("failed parm_data NULL");
      ret = FALSE;
      break;
    }
    tune_chromatix_t *chromatix = (tune_chromatix_t *)event_control->parm_data;
    chromatix_params.stream_mask = s_bundle->stream_mask;
    rc = chromatix_module_params->func_tbl.process(
    chromatix_module_params->sub_module_private,
     CHROMATIX_GET_PTR, &chromatix_params);
    if (rc < 0) {
      SERR("failed");
      ret = FALSE;
      break;
    }

    memcpy(chromatix_params.chromatix_ptr,
      &chromatix->chromatixData, sizeof(chromatix_parms_type));
    memcpy(chromatix_params.snapchromatix_ptr,
      &chromatix->snapchromatixData, sizeof(chromatix_parms_type));
    memcpy(chromatix_params.common_chromatix_ptr,
      &chromatix->common_chromatixData, sizeof(chromatix_VFE_common_type));

    module_chromatix.chromatixComPtr =
      chromatix_params.common_chromatix_ptr;
    module_chromatix.chromatixPtr = chromatix_params.chromatix_ptr;
    /* Send chromatix pointer downstream */
    new_event.type = MCT_EVENT_MODULE_EVENT;
    new_event.identity = event->identity;
    new_event.direction = MCT_EVENT_DOWNSTREAM;
    new_event.u.module_event.type = MCT_EVENT_MODULE_SET_RELOAD_CHROMATIX;
    new_event.u.module_event.module_event_data =
      (void *)&module_chromatix;
    ret = sensor_util_post_event_on_src_port(module, &new_event);
    if (ret == FALSE) {
      SERR("failed");
      return FALSE;
    }
  break;
  }
  case CAM_INTF_PARM_SET_RELOAD_AFTUNE: {
    module_sensor_params_t      *actuator_module_params =
      s_bundle->module_sensor_params[SUB_MODULE_ACTUATOR];
    mct_event_module_t event_module;
    mct_event_t new_event;
    af_tune_parms_t *af_tune_ptr = NULL;
    if (!event_control->parm_data) {
      SERR("failed parm_data NULL");
      ret = FALSE;
      break;
    }
    tune_autofocus_t *afptr = (tune_autofocus_t *)event_control->parm_data;
    if (s_bundle->sensor_info->subdev_id[SUB_MODULE_ACTUATOR] != -1) {
      rc = actuator_module_params->func_tbl.process(
        actuator_module_params->sub_module_private,
        ACTUATOR_GET_AF_TUNE_PTR, &af_tune_ptr);
      if (rc < 0 || !af_tune_ptr) {
        SERR("failed rc %d af_tune_ptr %p", rc, af_tune_ptr);
        return FALSE;
      }

     memcpy(af_tune_ptr, &afptr->af_tuneData, sizeof(af_tune_parms_t));
      /* Send af tune pointer downstream */
      new_event.type = MCT_EVENT_MODULE_EVENT;
      new_event.identity = event->identity;
      new_event.direction = MCT_EVENT_DOWNSTREAM;
      new_event.u.module_event.type = MCT_EVENT_MODULE_SET_RELOAD_AFTUNE;
      new_event.u.module_event.module_event_data = (void *)af_tune_ptr;
      ret = sensor_util_post_event_on_src_port(module, &new_event);
      if (ret == FALSE) {
       SERR("failed");
       return FALSE;
      }
    }
  break;
  }
  case CAM_INTF_PARM_REDEYE_REDUCTION: {
    void *mode = event_control->parm_data;
    module_sensor_params_t        *led_module_params = NULL;
    led_module_params = s_bundle->module_sensor_params[SUB_MODULE_LED_FLASH];
    if (led_module_params->func_tbl.process != NULL) {
      rc = led_module_params->func_tbl.process(
        led_module_params->sub_module_private,
        LED_FLASH_SET_RER_PARAMS, mode);
    }
    if (rc < 0) {
      SERR("failed");
      ret = FALSE;
    }
  break;
  }
  default:{
    sensor_output_format_t output_format;
    rc = module_sensor_params->func_tbl.process(
      module_sensor_params->sub_module_private,
      SENSOR_GET_SENSOR_FORMAT, &output_format);
     if(output_format == SENSOR_YCBCR) {
     ret = module_sensor_hal_set_parm(
       module_sensor_params, event_control);
    }
   }
   break;
  }
ERROR:
  return ret;
}

/** module_sensor_handle_parm_raw_dimension: process event for
 *  sensor module
 *
 *  @module: pointer to sensor mct module
 *  @s_bundle: handle to sensor bundle
 *  @module_sensor_params: handle to sensor sub module
 *  @identity: identity of current stream
 *  @event_data: event data associated with this event
 *
 *  This function handles CAM_INTF_PARM_RAW_DIMENSION event
 *
 *  Return: TRUE for success
 *          FALSE for failure
 **/
static boolean module_sensor_handle_parm_raw_dimension(
  mct_module_t *module, module_sensor_bundle_info_t *s_bundle,
  uint32_t identity, void *event_data)
{
  int32_t                     rc = 0;
  boolean                     ret = FALSE;
  sensor_get_raw_dimension_t  sensor_get;
  module_sensor_params_t     *module_sensor_params = NULL;

  /* Validate input parameters */
  if (!s_bundle || !event_data) {
    SERR("failed: invalid params %p %p %p", s_bundle, module_sensor_params,
      event_data);
    return FALSE;
  }

  if (!s_bundle || !s_bundle->module_sensor_params[SUB_MODULE_SENSOR]) {
    SERR("failed: invalid sensor params");
    return FALSE;
  }

  module_sensor_params = s_bundle->module_sensor_params[SUB_MODULE_SENSOR];
  if (!module_sensor_params->func_tbl.process) {
    SERR("failed: invalid sensor function process pointer %p",
      module_sensor_params->func_tbl.process);
    return FALSE;
  }

  memset(&sensor_get, 0, sizeof(sensor_get));
  sensor_get.raw_dim = event_data;
  sensor_get.stream_mask = s_bundle->stream_mask;

  SLOW("stream mask %x", sensor_get.stream_mask);
  rc = module_sensor_params->func_tbl.process(
    module_sensor_params->sub_module_private,
    SENSOR_GET_RAW_DIMENSION, &sensor_get);
  if (rc < 0) {
    SERR("failed: SENSOR_GET_RAW_DIMENSION rc %d", rc);
    ret = FALSE;
  }

  return TRUE;
}

/** module_sensor_event_control_get_parm: process event for
 *  sensor module
 *
 *  @module: pointert to sensor mct module
 *  @event: event to be handled
 *  @s_bundle: pointer to sensor bundle for this sensor
 *
 *  This function handles all events associated with G_PARM
 *
 *  Return: TRUE for success
 *          FALSE for failure
 **/
static boolean module_sensor_event_control_get_parm(
   mct_module_t *module, mct_event_t* event,
   module_sensor_bundle_info_t *s_bundle)
{
  boolean                      ret = TRUE;
  int32_t                      rc = SENSOR_SUCCESS;
  mct_list_t                  *s_list = NULL;
  module_sensor_params_t      *module_sensor_params =
    s_bundle->module_sensor_params[SUB_MODULE_SENSOR];
  module_sensor_params_t      *chromatix_module_params =
    s_bundle->module_sensor_params[SUB_MODULE_CHROMATIX];
  module_sensor_params_t      *actuator_module_params =
          s_bundle->module_sensor_params[SUB_MODULE_ACTUATOR];
  if (!module_sensor_params || !event ||
    !chromatix_module_params || !actuator_module_params) {
    SERR("failed");
    ret = FALSE;
    goto ERROR;
  }
  mct_event_control_parm_t    *event_control =
    (mct_event_control_parm_t *)(event->u.ctrl_event.control_event_data);
  switch (event_control->type) {
    case CAM_INTF_PARM_GET_CHROMATIX: {
      sensor_chromatix_params_t chromatix_params;
      if (!event_control->parm_data) {
        SERR("failed parm_data NULL");
        ret = FALSE;
        break;
      }
      tune_chromatix_t *chromatix = (tune_chromatix_t *)event_control->parm_data;
      chromatix_params.stream_mask = s_bundle->stream_mask;
      rc = chromatix_module_params->func_tbl.process(
      chromatix_module_params->sub_module_private,
       CHROMATIX_GET_PTR, &chromatix_params);
      if (rc < 0) {
        SERR("failed");
        ret = FALSE;
        break;
      }
      memcpy(&chromatix->chromatixData, chromatix_params.chromatix_ptr,
        sizeof(chromatix_parms_type));
      memcpy(&chromatix->snapchromatixData, chromatix_params.snapchromatix_ptr,
        sizeof(chromatix_parms_type));
      memcpy(&chromatix->common_chromatixData, chromatix_params.common_chromatix_ptr,
        sizeof(chromatix_VFE_common_type));
    break;
  }
   case CAM_INTF_PARM_GET_AFTUNE: {
      af_tune_parms_t *af_tune_ptr = NULL;
      if (!event_control->parm_data) {
        SERR("failed parm_data NULL");
        ret = FALSE;
        break;
      }
      tune_autofocus_t *afptr = (tune_autofocus_t *)event_control->parm_data;
      if (s_bundle->sensor_info->subdev_id[SUB_MODULE_ACTUATOR] != -1) {
        rc = actuator_module_params->func_tbl.process(
          actuator_module_params->sub_module_private,
          ACTUATOR_GET_AF_TUNE_PTR, &af_tune_ptr);
        if (rc < 0 || !af_tune_ptr) {
          SERR("failed rc %d af_tune_ptr %p", rc, af_tune_ptr);
          return FALSE;
        }

       memcpy(&afptr->af_tuneData, af_tune_ptr, sizeof(af_tune_parms_t));
      }

    break;
  }

  case CAM_INTF_PARM_RAW_DIMENSION:
    ret = module_sensor_handle_parm_raw_dimension(module, s_bundle,
      event->identity, event_control->parm_data);
    if (ret == FALSE) {
      SERR("failed: module_sensor_handle_parm_raw_dimension");
    }
    break;

  default:
    break;
  }
ERROR:
  return ret;
}

/** module_sensor_event_control_parm_stream_buf: process event for
 *  sensor module
 *
 *  @s_bundle: pointer to sensor bundle
 *  @control_data: pointer to control data that is sent with
 *               S_PARM
 *
 *  Return: TRUE / FALSE
 *
 *  This function handles all events associated with S_PARM * */

static boolean module_sensor_event_control_parm_stream_buf(
   mct_module_t *module, mct_event_t* event,
   module_sensor_bundle_info_t *s_bundle)
{
  boolean                      ret = TRUE;

  if (!event) {
    SERR("failed");
    ret = FALSE;
    goto ERROR;
  }

  cam_stream_parm_buffer_t   *stream_parm =
    event->u.ctrl_event.control_event_data;

  switch (stream_parm->type) {

  case CAM_STREAM_PARAM_TYPE_SET_BUNDLE_INFO: {
    SLOW("CAM_STREAM_PARAM_TYPE_SET_BUNDLE_INFO");

    sensor_util_assign_bundle_id(module, event->identity,
       &stream_parm->bundleInfo);
    break;
  }
  default:
    break;
  }

ERROR:
  return ret;
}

/** module_sensor_process_event: process event for sensor
 *  module
 *
 *  @streamid: streamid associated with event
 *  @module: mct module handle
 *  @event: event to be processed
 *
 *  Return: 0 for success and negative error on failure
 *
 *  This function handles all events and sends those events
 *  downstream / upstream *   */

static boolean module_sensor_module_process_event(mct_module_t *module,
  mct_event_t *event)
{
  boolean                      ret = TRUE;
  int32_t                      rc = SENSOR_SUCCESS;
  module_sensor_ctrl_t        *module_ctrl = NULL;
  mct_event_control_t         *event_ctrl = NULL;
  sensor_bundle_info_t         bundle_info;

  if (!module || !event) {
    SERR("failed port %p event %p", module,
      event);
    return FALSE;
  }
  if (event->type != MCT_EVENT_CONTROL_CMD) {
    SERR("failed invalid event type %d",
      event->type);
    return FALSE;
  }

  module_ctrl = (module_sensor_ctrl_t *)module->module_private;
  if (!module_ctrl) {
    SERR("failed");
    return FALSE;
  }

  event_ctrl = &event->u.ctrl_event;

  memset(&bundle_info, 0, sizeof(sensor_bundle_info_t));
  ret = sensor_util_get_sbundle(module, event->identity, &bundle_info);
  if (ret == FALSE) {
    SERR("failed");
    return FALSE;
  }
  SLOW("event id %d", event_ctrl->type);

  if (event_ctrl->type == MCT_EVENT_CONTROL_PREPARE_SNAPSHOT) {
    sensor_output_format_t output_format;
    mct_bus_msg_t bus_msg;
    module_sensor_params_t *module_sensor_params = NULL;

    bundle_info.s_bundle->state = 0;
    bundle_info.s_bundle->regular_led_trigger = 0;
    module_sensor_params =
      bundle_info.s_bundle->module_sensor_params[SUB_MODULE_SENSOR];
    rc = module_sensor_params->func_tbl.process(
      module_sensor_params->sub_module_private,
      SENSOR_GET_SENSOR_FORMAT, &output_format);
    SLOW("in Prepare snapshot, sensor type is %d\n", output_format);
    if (output_format == SENSOR_YCBCR) {
      bus_msg.sessionid = bundle_info.s_bundle->sensor_info->session_id;
      bus_msg.type = MCT_BUS_MSG_PREPARE_HW_DONE;
      cam_prep_snapshot_state_t state;
      state = DO_NOT_NEED_FUTURE_FRAME;
      bus_msg.msg = &state;
      if (mct_module_post_bus_msg(module,&bus_msg)!=TRUE)
        SERR("Failure posting to the bus!");
      return TRUE;
    }
  }
  switch (event_ctrl->type) {
  case MCT_EVENT_CONTROL_STREAMON:
    SLOW("CT_EVENT_CONTROL_STREAMON");
    memcpy(&module_ctrl->streaminfo, event->u.ctrl_event.control_event_data, sizeof(mct_stream_info_t));
    ret = module_sensor_stream_on(module, event, bundle_info.s_bundle);
    if (ret == FALSE) {
      SERR("failed");
      break;
    }
    break;
  case MCT_EVENT_CONTROL_STREAMOFF: {
    mct_stream_info_t* stream_info = (mct_stream_info_t*)
        event->u.ctrl_event.control_event_data;
    SHIGH("ide %x MCT_EVENT_CONTROL_STREAMOFF", event->identity);
    SLOW("CT_EVENT_CONTROL_STREAMOFF");
    memcpy(&module_ctrl->streaminfo, event->u.ctrl_event.control_event_data, sizeof(mct_stream_info_t));
    module_sensor_params_t *module_sensor_params =
      bundle_info.s_bundle->module_sensor_params[SUB_MODULE_SENSOR];
    if (!module_sensor_params) {
      SERR("failed");
      ret = FALSE;
      break;
    }
        /* Call send_event to propogate event to next module*/
    ret = sensor_util_post_event_on_src_port(module, event);
    if (ret == FALSE) {
      SERR("failed");
      break;
    }
    if (TRUE == module_sensor_is_ready_for_stream_off(module, event, bundle_info.s_bundle)) {
      SHIGH("ide %x MCT_EVENT_CONTROL_STREAMOFF", event->identity);

    /* Check whether this is live snapshot stream ON */
    if ((bundle_info.s_bundle->stream_mask & (1 << CAM_STREAM_TYPE_VIDEO)) &&
        (stream_info->stream_type == CAM_STREAM_TYPE_SNAPSHOT)) {
      /* Unload live snapshot chromatix */
      module_sensor_params_t *chromatix_module_params = NULL;
      mct_event_t new_event;
      modules_liveshot_Chromatix_t module_chromatix;

      memset(&new_event, 0, sizeof(event));
      chromatix_module_params =
        bundle_info.s_bundle->module_sensor_params[SUB_MODULE_CHROMATIX];

      module_chromatix.liveshot_chromatix_ptr = NULL;
      /* Send chromatix pointer downstream */
      new_event.type = MCT_EVENT_MODULE_EVENT;
      new_event.identity = event->identity;
      new_event.direction = MCT_EVENT_DOWNSTREAM;
      new_event.u.module_event.type =
        MCT_EVENT_MODULE_SET_LIVESHOT_CHROMATIX_PTR;
      new_event.u.module_event.module_event_data = (void *)&module_chromatix;
      rc = sensor_util_post_event_on_src_port(module, &new_event);
      if (rc == FALSE) {
        SERR("failed");
      }
      rc = chromatix_module_params->func_tbl.process(
        chromatix_module_params->sub_module_private,
        CHROMATIX_CLOSE_LIVESHOT_LIBRARY, NULL);
      if (rc < 0) {
       SERR("failed");
      }
    }
      /*If streaming offf preview, then turn off LED*/
      if (stream_info->stream_type == CAM_STREAM_TYPE_SNAPSHOT ||
          stream_info->stream_type == CAM_STREAM_TYPE_POSTVIEW)
      {
        SLOW ("stream off preview or snapshot, turn off LED");
        module_sensor_params_t        *led_module_params = NULL;
        led_module_params = bundle_info.s_bundle->module_sensor_params[SUB_MODULE_LED_FLASH];
        if (led_module_params->func_tbl.process != NULL) {
          rc = led_module_params->func_tbl.process(
          led_module_params->sub_module_private,
          LED_FLASH_SET_OFF , NULL);
          if (rc < 0) {
            SERR("failed: LED_FLASH_SET_OFF");
          } else {
            bundle_info.s_bundle->sensor_params.flash_mode = CAM_FLASH_MODE_OFF;
            sensor_util_post_led_state_msg(module, bundle_info.s_bundle,
              event->identity);
          }
        }
      }
      mct_bus_msg_t bus_msg;
      bus_msg.sessionid = bundle_info.s_bundle->sensor_info->session_id;
      bus_msg.type = MCT_BUS_MSG_SENSOR_STOPPING;
      bus_msg.msg = NULL;
      ALOGE("%s: Sending stop bus message\n", __func__);
      if(mct_module_post_bus_msg(module, &bus_msg) == FALSE)
        SERR("failed");
      rc = module_sensor_params->func_tbl.process(
        module_sensor_params->sub_module_private,
        SENSOR_STOP_STREAM, NULL);
      if (rc < 0) {
        SERR("failed");
        ret = FALSE;
        break;
      }
    }
    break;
  }
  case MCT_EVENT_CONTROL_SET_PARM: {
    ret = module_sensor_event_control_set_parm(
       module, event, &bundle_info);

//      s_bundle, event->u.ctrl_event.control_event_data);
    if (ret == FALSE) {
      SERR("failed");
    }
    mct_event_control_parm_t    *event_control =
      (mct_event_control_parm_t *)(event->u.ctrl_event.control_event_data);

    module_sensor_params_t *module_sensor_params =
      bundle_info.s_bundle->module_sensor_params[SUB_MODULE_SENSOR];

    sensor_output_format_t output_format;
    rc = module_sensor_params->func_tbl.process(
      module_sensor_params->sub_module_private,
      SENSOR_GET_SENSOR_FORMAT, &output_format);
     if(output_format == SENSOR_BAYER ||
         (event_control->type == CAM_INTF_PARM_ZOOM ) ||
         (event_control->type == CAM_INTF_PARM_FD )) {
    /* Call send_event to propogate event to next module*/
       ret = sensor_util_post_event_on_src_port(module, event);
       if (ret == FALSE) {
          SERR("failed");
          return FALSE;
       }
     }
     break;
  }

  case MCT_EVENT_CONTROL_GET_PARM: {
    ret = module_sensor_event_control_get_parm(
       module, event, bundle_info.s_bundle);

    if (ret == FALSE) {
      SERR("failed");
    }
    /* Call send_event to propogate event to next module*/
    ret = sensor_util_post_event_on_src_port(module, event);
    if (ret == FALSE) {
      SERR("failed");
      return FALSE;
    }
    break;
  }

  case MCT_EVENT_CONTROL_PARM_STREAM_BUF: {
    ret = module_sensor_event_control_parm_stream_buf(
       module, event, bundle_info.s_bundle);

    if (ret == FALSE) {
      SERR("failed");
    }
    /* Call send_event to propogate event to next module*/
    ret = sensor_util_post_event_on_src_port(module, event);
    if (ret == FALSE) {
      SERR("failed");
      return FALSE;
    }
    break;
  }

  case MCT_EVENT_CONTROL_START_ZSL_SNAPSHOT: {
      stats_get_data_t stats_get;
      mct_event_t new_event;
      float digital_gain = 0;
      sensor_output_format_t output_format;
      memset(&stats_get, 0, sizeof(stats_get_data_t));
      module_sensor_params_t *module_sensor_params =
      bundle_info.s_bundle->module_sensor_params[SUB_MODULE_SENSOR];
      SERR("zsl capture start ");
      SLOW("propagate zsl capture downstream");
      ret = sensor_util_post_event_on_src_port(module, event);

      rc = module_sensor_params->func_tbl.process(
              module_sensor_params->sub_module_private,
              SENSOR_GET_SENSOR_FORMAT, &output_format);
      if (rc < 0) {
        SERR("failed");
      } else
      SERR  (" Get data from stats");
      new_event.type = MCT_EVENT_MODULE_EVENT;
      new_event.identity = event->identity;
      new_event.direction = MCT_EVENT_DOWNSTREAM;
      new_event.u.module_event.type = MCT_EVENT_MODULE_STATS_GET_DATA;
      new_event.u.module_event.module_event_data = (void *)&stats_get;
      ret = sensor_util_post_event_on_src_port(module, &new_event);
       if (ret == FALSE) {
          SERR("failed");
          /* Continue START STREAM since this is not FATAL error */
          ret = TRUE;
        } else if (stats_get.flag & STATS_UPDATE_AEC) {

         aec_update_t aec_update;
         aec_update.real_gain = stats_get.aec_get.real_gain[0];
         aec_update.linecount = stats_get.aec_get.linecount[0];
         SERR("get stats led trigger %d ",stats_get.aec_get.trigger_led);
            if (1)
            {
              module_sensor_params_t        *led_module_params = NULL;
              led_module_params = bundle_info.s_bundle->module_sensor_params[SUB_MODULE_LED_FLASH];
              if (led_module_params->func_tbl.process != NULL) {
                rc = led_module_params->func_tbl.process(
                  led_module_params->sub_module_private,
                  LED_FLASH_SET_HIGH , NULL);
                if (rc < 0) {
                  bundle_info.s_bundle->sensor_params.flash_mode =
                    CAM_FLASH_MODE_OFF;
                  SERR("failed: LED_FLASH_SET_HIGH");
                } else {
                  bundle_info.s_bundle->sensor_params.flash_mode =
                    CAM_FLASH_MODE_ON;
                }
                sensor_util_post_led_state_msg(module, bundle_info.s_bundle, event->identity);
              }
                bundle_info.s_bundle->regular_led_trigger = 0;
              SLOW(" led zsl capture start ");
              rc = module_sensor_params->func_tbl.process(
                module_sensor_params->sub_module_private,
              SENSOR_SET_WAIT_FRAMES, &bundle_info.s_bundle->num_skip);
              mct_bus_msg_t bus_msg;
              bus_msg.sessionid = bundle_info.session_id;
              bus_msg.type = MCT_BUS_MSG_ZSL_TAKE_PICT_DONE;
              cam_frame_idx_range_t range;
              SLOW ("last_idx %d", bundle_info.s_bundle->last_idx);
              range.min_frame_idx = bundle_info.s_bundle->last_idx+1+3;
              range.max_frame_idx = bundle_info.s_bundle->last_idx+1+2+10;
              bus_msg.msg = &range;
              if(mct_module_post_bus_msg(module,&bus_msg)!=TRUE)
                SERR("");
            }
          SLOW("led set to high");


          rc = module_sensor_params->func_tbl.process(
            module_sensor_params->sub_module_private,
            SENSOR_SET_AEC_UPDATE, &aec_update);
          if (1) {
            ret = sensor_util_set_digital_gain_to_isp(module,
              bundle_info.s_bundle, event->identity);
              if (ret == FALSE) {
                SERR("failed");
                /* Continue START STREAM since this is not FATAL error */
                ret = TRUE;
              }
            }
       }


  break;
  }

  case MCT_EVENT_CONTROL_STOP_ZSL_SNAPSHOT:{
     SERR("stop ZSL capture, led off\n");
        module_sensor_params_t        *led_module_params = NULL;
        led_module_params = bundle_info.s_bundle->module_sensor_params[SUB_MODULE_LED_FLASH];
        if (led_module_params->func_tbl.process != NULL) {
          rc = led_module_params->func_tbl.process(
            led_module_params->sub_module_private,
            LED_FLASH_SET_OFF , NULL);
          if (rc < 0) {
            SERR("failed: LED_FLASH_SET_OFF");
          } else {
            bundle_info.s_bundle->sensor_params.flash_mode = CAM_FLASH_MODE_OFF;
            sensor_util_post_led_state_msg(module, bundle_info.s_bundle,
              event->identity);
          }
        }
        SHIGH("%s, post-flash: OFF", __func__);
      ret = sensor_util_post_event_on_src_port(module, event);
    break;
  }
  case MCT_EVENT_CONTROL_DO_AF:{
       sensor_output_format_t output_format;
        module_sensor_params_t *module_sensor_params =
        bundle_info.s_bundle->module_sensor_params[SUB_MODULE_SENSOR];
        module_sensor_params->func_tbl.process(
              module_sensor_params->sub_module_private,
              SENSOR_GET_SENSOR_FORMAT, &output_format);
        if(output_format == SENSOR_YCBCR) {
          sensor_info_t sensor_info;
          sensor_info.module = module;
          sensor_info.session_id = bundle_info.s_bundle->sensor_info->session_id;
          module_sensor_params->func_tbl.process(
                module_sensor_params->sub_module_private,
                SENSOR_SET_AUTOFOCUS, &sensor_info);
          SHIGH("%s: Setting Auto Focus", __func__);
        }
        else {
          ret = sensor_util_post_event_on_src_port(module, event);
        }
    break;
  }
  case MCT_EVENT_CONTROL_CANCEL_AF:{
       sensor_output_format_t output_format;
        module_sensor_params_t *module_sensor_params =
        bundle_info.s_bundle->module_sensor_params[SUB_MODULE_SENSOR];
        module_sensor_params->func_tbl.process(
              module_sensor_params->sub_module_private,
              SENSOR_GET_SENSOR_FORMAT, &output_format);
        if(output_format == SENSOR_YCBCR) {
          module_sensor_params->func_tbl.process(
                module_sensor_params->sub_module_private,
                SENSOR_CANCEL_AUTOFOCUS, module);
        SHIGH("%s: Cancelling Auto Focus", __func__);
        sensor_cancel_autofocus_loop();
        }
        else {
          ret = sensor_util_post_event_on_src_port(module, event);
        }
    break;
  }
  default:
    /* Call send_event to propogate event to next module*/
    ret = sensor_util_post_event_on_src_port(module, event);
    if (ret == FALSE) {
      SERR("failed");
      return FALSE;
    }
    break;
  }
  return ret;
}

/** module_sensor_set_mod: set mod function for sensor module
 *
 *  This function handles set mod events sent by mct **/

static void module_sensor_set_mod(mct_module_t *module, unsigned int module_type,
  unsigned int identity)
{
  SLOW("Enter, module_type=%d", module_type);
  mct_module_add_type(module, module_type, identity);
  if (module_type == MCT_MODULE_FLAG_SOURCE) {
    mct_module_set_process_event_func(module,
      module_sensor_module_process_event);
  }
  return;
}

/** module_sensor_query_mod: query mod function for sensor
 *  module
 *
 *  @query_buf: pointer to module_sensor_query_caps_t struct
 *  @session: session id
 *  @s_module: mct module pointer for sensor
 *
 *  Return: 0 for success and negative error for failure
 *
 *  This function handles query module events to return
 *  information requested by mct any stream is created **/

static boolean module_sensor_query_mod(mct_module_t *module,
  void *buf, unsigned int sessionid)
{
  int32_t idx = 0, rc = SENSOR_SUCCESS;
  mct_pipeline_sensor_cap_t *sensor_cap = NULL;
  module_sensor_ctrl_t *module_ctrl = NULL;
  module_sensor_bundle_info_t *s_bundle = NULL;
  module_sensor_params_t *module_sensor_params = NULL;
  mct_list_t *s_list = NULL;

  mct_pipeline_cap_t *query_buf = (mct_pipeline_cap_t *)buf;
  if (!query_buf || !module) {
    SERR("failed query_buf %p s_module %p",
      query_buf, module);
    return FALSE;
  }

  sensor_cap = &query_buf->sensor_cap;
  module_ctrl = (module_sensor_ctrl_t *)module->module_private;
  if (!module_ctrl) {
    return FALSE;
  }

  s_list = mct_list_find_custom(module_ctrl->sensor_bundle, &sessionid,
    sensor_util_find_bundle);
  if (!s_list) {
    SERR("session_id doesn't match idx");
    return FALSE;
  }
  s_bundle = (module_sensor_bundle_info_t *)s_list->data;
  if (!s_bundle) {
    return FALSE;
  }
  module_sensor_params = s_bundle->module_sensor_params[SUB_MODULE_SENSOR];
  if (!module_sensor_params) {
    return FALSE;
  }
  SLOW("sensor name %s",
    s_bundle->sensor_info->sensor_name);

  rc = module_sensor_params->func_tbl.process(s_bundle->sensor_lib_params,
     SENSOR_GET_CAPABILITIES, sensor_cap);
  if (rc < 0) {
    SERR("failed rc %d", rc);
    return FALSE;
  }

  if (s_bundle->sensor_info->subdev_id[SUB_MODULE_LED_FLASH] != -1 ||
    s_bundle->sensor_info->subdev_id[SUB_MODULE_STROBE_FLASH] != -1) {
    sensor_cap->is_flash_supported = TRUE;
  } else {
    SERR("led flash is not supported for this sensor.");
    sensor_cap->is_flash_supported = FALSE;
  }

  sensor_cap->af_supported =
    (s_bundle->sensor_info->subdev_id[SUB_MODULE_ACTUATOR] != -1) ?
                                                      TRUE : FALSE;
  /* Fill sensor init params */
  sensor_cap->modes_supported =
    s_bundle->sensor_init_params->modes_supported;
  sensor_cap->position = s_bundle->sensor_init_params->position;
  sensor_cap->sensor_mount_angle =
    s_bundle->sensor_init_params->sensor_mount_angle;
  sensor_output_format_t output_format;
  rc = module_sensor_params->func_tbl.process(
          module_sensor_params->sub_module_private,
          SENSOR_GET_SENSOR_FORMAT, &output_format);
  if (rc < 0) {
    SERR("failed rc %d", rc);
    return FALSE;
    }
  if (output_format == SENSOR_YCBCR) {
    sensor_cap->sensor_format = FORMAT_YCBCR;
   }
   else if (output_format == SENSOR_BAYER) {
    sensor_cap->sensor_format = FORMAT_BAYER;
   }
   return TRUE;
}

static boolean module_sensor_delete_port(void *data, void *user_data)
{
  mct_port_t *s_port = (mct_port_t *)data;
  mct_module_t *module = (mct_module_t *)user_data;
  if (!s_port || !module) {
    SERR("failed s_port %p module %p",
      s_port, module);
    return TRUE;
  }
  free(s_port->caps.u.data);
  return TRUE;
}

/** module_sensor_free_bundle: free bundle function for
 *  sensor module
 *
 *  @data: sensor bundle pointer
 *  @user_data: NULL
 *
 *  Return: 0 for success and negative error on failure
 *
 *  This function is called for each sensor when module close is
 *  called. It releases all resources and memory held by each
 *  sensor */

static boolean module_sensor_free_bundle(void *data, void *user_data)
{
  uint32_t i = 0;
  module_sensor_bundle_info_t *s_bundle = (module_sensor_bundle_info_t *)data;
  mct_module_t *module = (mct_module_t *)user_data;
  if (!s_bundle || !module) {
    SERR("failed s_bundle %p, module %p",
      s_bundle, module);
    return TRUE;
  }
  mct_list_free_all(MCT_MODULE_SRCPORTS(module), module_sensor_delete_port);
  if (s_bundle->sensor_lib_params) {
    free(s_bundle->sensor_lib_params);
  }
  for (i = 0; i < SUB_MODULE_MAX; i++) {
    free(s_bundle->module_sensor_params[i]);
  }
  free(s_bundle->sensor_init_params);
  free(s_bundle->sensor_info);
  free(s_bundle);
  return TRUE;
}

/** module_sensor_free_mod: free module function for sensor
 *  module
 *
 *  @module: mct module pointer for sensor
 *
 *  This function releases all resources held by sensor mct
 *  module */

void module_sensor_deinit(mct_module_t *module)
{
  int32_t                      rc = SENSOR_SUCCESS;
  int32_t                      idx = -1, i = 0;
  module_sensor_ctrl_t        *module_ctrl = NULL;
  module_sensor_bundle_info_t *s_bundle = NULL;

  if (!module) {
    SERR("module NULL");
    return;
  }

  module_ctrl = (module_sensor_ctrl_t *)module->module_private;

  mct_list_traverse(module_ctrl->sensor_bundle, module_sensor_free_bundle,
    module);

  free(module);

  return;
}

/** module_sensor_find_sensor_subdev: find sensor subdevs
 *
 *  @module_ctrl: sensor ctrl pointer
 *
 *  Return: 0 for success and negative error on failure
 *
 *  This function finds all sensor subdevs, creates sensor
 *  bundle for each sensor subdev and gets init params and
 *  subdev info from the subdev **/

static void module_sensor_find_sensor_subdev(
  module_sensor_ctrl_t *module_ctrl)
{
  struct media_device_info mdev_info;
  int32_t num_media_devices = 0;
  char dev_name[32];
  char subdev_name[32];
  int32_t rc = 0, dev_fd = 0, sd_fd = 0;
  module_sensor_bundle_info_t *sensor_bundle = NULL;
  struct sensorb_cfg_data cfg;
  uint32_t i = 0;

  while (1) {
    int32_t num_entities = 1;
    snprintf(dev_name, sizeof(dev_name), "/dev/media%d", num_media_devices);
    dev_fd = open(dev_name, O_RDWR | O_NONBLOCK);
    if (dev_fd < 0) {
      SLOW("Done enumerating media devices");
      break;
    }
    num_media_devices++;
    rc = ioctl(dev_fd, MEDIA_IOC_DEVICE_INFO, &mdev_info);
    if (rc < 0) {
      SLOW("Done enumerating media devices");
      close(dev_fd);
      break;
    }

    if (strncmp(mdev_info.model, "msm_config", sizeof(mdev_info.model) != 0)) {
      close(dev_fd);
      continue;
    }

    while (1) {
      struct media_entity_desc entity;
      memset(&entity, 0, sizeof(entity));
      entity.id = num_entities++;
      SLOW("entity id %d", entity.id);
      rc = ioctl(dev_fd, MEDIA_IOC_ENUM_ENTITIES, &entity);
      if (rc < 0) {
        SLOW("Done enumerating media entities");
        rc = 0;
        break;
      }
      SLOW("entity name %s type %d group id %d",
        entity.name, entity.type, entity.group_id);
      if (entity.type == MEDIA_ENT_T_V4L2_SUBDEV &&
          entity.group_id == MSM_CAMERA_SUBDEV_SENSOR) {
        snprintf(subdev_name, sizeof(dev_name), "/dev/%s", entity.name);
        sd_fd = open(subdev_name, O_RDWR);
        if (sd_fd < 0) {
          SLOW("Open subdev failed");
          continue;
        }
        sensor_bundle = malloc(sizeof(module_sensor_bundle_info_t));
        if (!sensor_bundle) {
          SERR("failed");
          close(sd_fd);
          continue;
        }
        memset(sensor_bundle, 0, sizeof(module_sensor_bundle_info_t));

        cfg.cfgtype = CFG_GET_SENSOR_INFO;
        rc = ioctl(sd_fd, VIDIOC_MSM_SENSOR_CFG, &cfg);
        if (rc < 0) {
          SERR("failed rc %d", rc);
          close(sd_fd);
          continue;
        }

        sensor_bundle->sensor_info = malloc(sizeof(struct msm_sensor_info_t));
        if (!sensor_bundle->sensor_info) {
          free(sensor_bundle);
          close(sd_fd);
          continue;
        }
        memset(sensor_bundle->sensor_info, 0, sizeof(struct msm_sensor_info_t));

        /* Fill sensor info structure in sensor bundle */
        *sensor_bundle->sensor_info = cfg.cfg.sensor_info;

        SLOW("sensor name %s session %d",
          sensor_bundle->sensor_info->sensor_name,
          sensor_bundle->sensor_info->session_id);

        /* Initialize chroamtix subdevice id */
        sensor_bundle->sensor_info->subdev_id[SUB_MODULE_SENSOR] =
          sensor_bundle->sensor_info->session_id;
        sensor_bundle->sensor_info->subdev_id[SUB_MODULE_CHROMATIX] = 0;
        for (i = 0; i < SUB_MODULE_MAX; i++) {
          SLOW("subdev_id[%d] %d", i,
            sensor_bundle->sensor_info->subdev_id[i]);
        }
        cfg.cfgtype = CFG_GET_SENSOR_INIT_PARAMS;
        rc = ioctl(sd_fd, VIDIOC_MSM_SENSOR_CFG, &cfg);
        if (rc < 0) {
          SERR("failed rc %d", rc);
          free(sensor_bundle->sensor_info);
          free(sensor_bundle);
          close(sd_fd);
          continue;
        }

        sensor_bundle->sensor_init_params =
          malloc(sizeof(struct msm_sensor_init_params));
        if (!sensor_bundle->sensor_init_params) {
          close(sd_fd);
          continue;
        }
        memset(sensor_bundle->sensor_init_params, 0,
          sizeof(struct msm_sensor_init_params));

        /* Fill sensor init params structure in sensor bundle */
        *sensor_bundle->sensor_init_params = cfg.cfg.sensor_init_params;

        SLOW("modes supported %d, position %d, sensor mount angle %d",
          sensor_bundle->sensor_init_params->modes_supported,
          sensor_bundle->sensor_init_params->position,
          sensor_bundle->sensor_init_params->sensor_mount_angle);

        /* Copy sensor subdev name to open and use during camera session */
        memcpy(sensor_bundle->sensor_sd_name[SUB_MODULE_SENSOR], entity.name,
          MAX_SUBDEV_SIZE);

        SLOW("sensor sd name %s",
          sensor_bundle->sensor_sd_name[SUB_MODULE_SENSOR]);

        /* Add sensor_bundle to module_ctrl list */
        module_ctrl->sensor_bundle =
          mct_list_append(module_ctrl->sensor_bundle, sensor_bundle, NULL,
          NULL);

        /* Increment sensor bundle size */
        module_ctrl->size++;

        close(sd_fd);
      }
    }
    close(dev_fd);
  }
  return;
}

/** module_sensor_set_sub_module_id: set subdev id for sensor sub
 *  modules
 *
 *  @data: module_sensor_bundle_info_t pointer
 *  @user_data: module_sensor_match_id_params_t pointer
 *
 *  Return: 0 for success and negative error on failure
 *
 *  This function matches subdev id against subdev id present in
 *  sensor info, if both matches, copy the subdev name for this
 *  subdev which will be used later to open and communicate with
 *  kernel driver **/

static boolean module_sensor_set_sub_module_id(void *data, void *user_data)
{
  module_sensor_bundle_info_t *sensor_bundle =
    (module_sensor_bundle_info_t *)data;
  module_sensor_match_id_params_t *match_id_params =
    (module_sensor_match_id_params_t *)user_data;
  if (!sensor_bundle || !match_id_params) {
    SERR("failed data1 %p data2 %p", data,
      user_data);
    return FALSE;
  }
  SLOW("sub module %d id %d subdev name %s",
    match_id_params->sub_module, match_id_params->subdev_id,
    match_id_params->subdev_name);
  if (sensor_bundle->sensor_info->subdev_id[match_id_params->sub_module] ==
    match_id_params->subdev_id) {
    memcpy(sensor_bundle->sensor_sd_name[match_id_params->sub_module],
      match_id_params->subdev_name, MAX_SUBDEV_SIZE);
    SLOW("match found sub module %d session id %d subdev name %s",
      match_id_params->sub_module,
      sensor_bundle->sensor_info->session_id, match_id_params->subdev_name);
  }
  return TRUE;
}


static boolean module_sensor_init_eeprom(void *data, void *user_data) {
    int32_t rc = 0;
    module_sensor_bundle_info_t *s_bundle = (module_sensor_bundle_info_t *)data;
    mct_module_t                *s_module = (mct_module_t *)user_data;
    sensor_func_tbl_t func_tbl;
    module_sensor_params_t *module_eeprom_params = NULL;

    SLOW("Enter");
    if (s_bundle->sensor_info->subdev_id[SUB_MODULE_EEPROM] == -1)
     return TRUE;
    eeprom_sub_module_init(&func_tbl);
    s_bundle->eeprom_data = (sensor_eeprom_data_t *)
       malloc(sizeof(sensor_eeprom_data_t));
    if (!s_bundle->eeprom_data) {
        SERR("failed to allocate memory");
        return TRUE;
    }
    rc = func_tbl.process(s_bundle->eeprom_data, EEPROM_OPEN_FD,
      s_bundle->sensor_sd_name[SUB_MODULE_EEPROM]);
    if (rc < 0) {
        SERR("Failed EEPROM_OPEN_FD");
        return TRUE;
    }
    rc = func_tbl.process(s_bundle->eeprom_data, EEPROM_READ_DATA,
      NULL);
    if (rc < 0) {
        SERR("Failed EEPROM_READ_DATA");
        return TRUE;
    }
    rc = func_tbl.process(s_bundle->eeprom_data, EEPROM_CLOSE_FD,
      NULL);
    if (rc < 0) {
        SERR("Failed EEPROM_CLOSE_FD");
        return TRUE;
    }

  SLOW("Exit");
  return TRUE;
}

/** module_sensor_find_other_subdev: find subdevs other than
 *  sensor
 *
 *  @module_ctrl: module_sensor_ctrl_t pointer
 *
 *  Return: 0 for success and negative error on failure
 *
 *  This function finds all subdevs other than sensor and fills
 *  the subdev name in sensor bundle for those sensor whose
 *  subdev id matches with current subdev **/

static boolean module_sensor_find_other_subdev(
  module_sensor_ctrl_t *module_ctrl)
{
  struct media_device_info mdev_info;
  int32_t num_media_devices = 0;
  char dev_name[32];
  uint32_t subdev_id;
  char subdev_name[32];
  int32_t rc = 0, dev_fd = 0, sd_fd = 0;
  uint8_t session_id = 0;
  module_sensor_match_id_params_t match_id_params;
  if (!module_ctrl) {
    SLOW("failed");
    return FALSE;
  }
  while (1) {
    int32_t num_entities = 1;
    snprintf(dev_name, sizeof(dev_name), "/dev/media%d", num_media_devices);
    dev_fd = open(dev_name, O_RDWR | O_NONBLOCK);
    if (dev_fd < 0) {
      SLOW("Done enumerating media devices");
      break;
    }
    num_media_devices++;
    rc = ioctl(dev_fd, MEDIA_IOC_DEVICE_INFO, &mdev_info);
    if (rc < 0) {
      SERR("Error: ioctl media_dev failed: %s", strerror(errno));
      close(dev_fd);
      break;
    }

    if (strncmp(mdev_info.model, "msm_config", sizeof(mdev_info.model) != 0)) {
      close(dev_fd);
      continue;
    }

    while (1) {
      struct media_entity_desc entity;
      memset(&entity, 0, sizeof(entity));
      entity.id = num_entities++;
      SLOW("entity id %d", entity.id);
      rc = ioctl(dev_fd, MEDIA_IOC_ENUM_ENTITIES, &entity);
      if (rc < 0) {
        SLOW("Done enumerating media entities");
        rc = 0;
        break;
      }
      SLOW("entity name %s type %d group id %d",
        entity.name, entity.type, entity.group_id);
      if (entity.type == MEDIA_ENT_T_V4L2_SUBDEV &&
        (entity.group_id == MSM_CAMERA_SUBDEV_ACTUATOR ||
        entity.group_id == MSM_CAMERA_SUBDEV_EEPROM ||
        entity.group_id == MSM_CAMERA_SUBDEV_LED_FLASH ||
        entity.group_id == MSM_CAMERA_SUBDEV_STROBE_FLASH ||
        entity.group_id == MSM_CAMERA_SUBDEV_CSIPHY ||
        entity.group_id == MSM_CAMERA_SUBDEV_CSID)) {
        snprintf(subdev_name, sizeof(subdev_name), "/dev/%s", entity.name);
        sd_fd = open(subdev_name, O_RDWR);
        if (sd_fd < 0) {
          SLOW("Open subdev failed");
          continue;
        }
        /* Read subdev index */
        rc = ioctl(sd_fd, VIDIOC_MSM_SENSOR_GET_SUBDEV_ID, &subdev_id);
        if (rc < 0) {
          SERR("failed rc %d", rc);
          close(sd_fd);
          continue;
        }
        SLOW("subdev id %d", subdev_id);
        /* TODO: read id and fill in sensor_bundle.entity.actuator_name */
        switch (entity.group_id) {
           case MSM_CAMERA_SUBDEV_ACTUATOR:
            match_id_params.sub_module = SUB_MODULE_ACTUATOR;
            match_id_params.subdev_id = subdev_id;
            match_id_params.subdev_name = entity.name;
            mct_list_traverse(module_ctrl->sensor_bundle,
              module_sensor_set_sub_module_id, &match_id_params);
            break;
          case MSM_CAMERA_SUBDEV_EEPROM:
            match_id_params.sub_module = SUB_MODULE_EEPROM;
            match_id_params.subdev_id = subdev_id;
            match_id_params.subdev_name = entity.name;
            mct_list_traverse(module_ctrl->sensor_bundle,
              module_sensor_set_sub_module_id, &match_id_params);
            break;
          case MSM_CAMERA_SUBDEV_LED_FLASH:
            match_id_params.sub_module = SUB_MODULE_LED_FLASH;
            match_id_params.subdev_id = subdev_id;
            match_id_params.subdev_name = entity.name;
            mct_list_traverse(module_ctrl->sensor_bundle,
              module_sensor_set_sub_module_id, &match_id_params);
            break;
          case MSM_CAMERA_SUBDEV_STROBE_FLASH:
            match_id_params.sub_module = SUB_MODULE_STROBE_FLASH;
            match_id_params.subdev_id = subdev_id;
            match_id_params.subdev_name = entity.name;
            mct_list_traverse(module_ctrl->sensor_bundle,
              module_sensor_set_sub_module_id, &match_id_params);
            break;
          case MSM_CAMERA_SUBDEV_CSIPHY:
            match_id_params.sub_module = SUB_MODULE_CSIPHY;
            match_id_params.subdev_id = subdev_id;
            match_id_params.subdev_name = entity.name;
            mct_list_traverse(module_ctrl->sensor_bundle,
              module_sensor_set_sub_module_id, &match_id_params);
            break;
          case MSM_CAMERA_SUBDEV_CSID:
            match_id_params.sub_module = SUB_MODULE_CSID;
            match_id_params.subdev_id = subdev_id;
            match_id_params.subdev_name = entity.name;
            mct_list_traverse(module_ctrl->sensor_bundle,
              module_sensor_set_sub_module_id, &match_id_params);
            break;
          default:
            break;
        }
        close(sd_fd);
      }
    }
    close(dev_fd);
  }
  return TRUE;
};
/** module_sensor_init: sensor module init
 *
 *  Return: mct_module_t pointer corresponding to sensor
 *
 *  This function creates mct_module_t for sensor module,
 *  creates port, fills capabilities and add it to the sensor
 *  module **/

mct_module_t *module_sensor_init(const char *name)
{
  boolean                      ret = TRUE;
  mct_module_t                *s_module = NULL;
  module_sensor_ctrl_t        *module_ctrl = NULL;

  SHIGH("Enter");

  /* Create MCT module for sensor */
  s_module = mct_module_create(name);
  if (!s_module) {
    SERR("failed");
    return NULL;
  }

  /* Fill function table in MCT module */
  s_module->set_mod = module_sensor_set_mod;
  s_module->query_mod = module_sensor_query_mod;
  s_module->start_session = module_sensor_start_session;
  s_module->stop_session = module_sensor_stop_session;

  /* Create sensor module control structure that consists of bundle
     information */
  module_ctrl = malloc(sizeof(module_sensor_ctrl_t));
  if (!module_ctrl) {
    SERR("failed");
    goto ERROR1;
  }
  memset(module_ctrl, 0, sizeof(module_sensor_ctrl_t));

  s_module->module_private = (void *)module_ctrl;

  /* sensor module doesn't have sink port */
  s_module->numsinkports = 0;

  /* Fill all detected sensors */
  module_sensor_find_sensor_subdev(module_ctrl);

  /* find all the actuator, etc with sensor */
  ret = module_sensor_find_other_subdev(module_ctrl);
  if (ret == FALSE) {
    SERR("failed");
    goto ERROR1;
  }

  /* Init sensor modules */
  ret = mct_list_traverse(module_ctrl->sensor_bundle, module_sensors_subinit,
    NULL);
  if (ret == FALSE) {
    SERR("failed");
    goto ERROR1;
  }

  /* Create ports based on CID info */
  ret = mct_list_traverse(module_ctrl->sensor_bundle, port_sensor_create,
    s_module);
  if (ret == FALSE) {
    SERR("failed");
    goto ERROR1;
  }

  /* intiialize the eeprom */
  ret = mct_list_traverse(module_ctrl->sensor_bundle, module_sensor_init_eeprom,
    s_module);
  if (ret == FALSE) {
    SERR("failed");
    goto ERROR1;
  }

  SLOW("Exit");
  return s_module;

  SERR("failed");
ERROR1:
  mct_module_destroy(s_module);
  return NULL;
}
