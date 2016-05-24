/* port_sensor.c
 *
 * Copyright (c) 2012-2013 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include "media_controller.h"
#include "modules.h"
#include "mct_stream.h"
#include "port_sensor.h"
#include "sensor_util.h"
#include "led_flash/led_flash.h"

/** port_sensor_caps_reserve: check caps functiont of find
 *  compatible port
 *
 *  @port: port
 *  @peer_caps: peer port caps
 *  @stream_info: pointer to cam_stream_info_t struct
 *  @streamid: stream id for which this port is linked
 *  @sessionid: session id for which this port is linked
 *
 *  @Return: TRUE if this port can be used for linking, FALSE
 *           otherwise
 *
 *  This function validates port capablities against stream
 *  info and checks whether the format requested by
 *  application is compatible with the format suppport by
 *  given port and returns decision */

static boolean port_sensor_caps_reserve(mct_port_t *port, void *peer_caps,
   void *info)
{
  int32_t i = 0;
  boolean                           ret = FALSE;
  mct_module_t                     *s_module = NULL;
  module_sensor_ctrl_t             *module_ctrl = NULL;
  mct_list_t                       *port_parent = NULL;
  mct_stream_info_t                *stream_info = (mct_stream_info_t *)info;
  sensor_src_port_cap_t            *sensor_src_port_cap =
    (sensor_src_port_cap_t *)port->caps.u.data;
  cam_format_t                      fmt;
  module_sensor_port_stream_info_t *port_stream_info = NULL;
  module_sensor_port_data_t        *port_data = NULL;
  sensor_bundle_info_t              bundle_info;

  SLOW("Enter port %p", port);
  if (!stream_info || !sensor_src_port_cap) {
    SERR("failed stream_info %p, sensor_src_port_cap %p",
      stream_info, sensor_src_port_cap);
    return FALSE;
  }

  port_parent = (mct_list_t *)MCT_PORT_PARENT(port);
  if (!port_parent) {
    SERR("failed");
    return FALSE;
  }

  s_module = (mct_module_t *)port_parent->data;
  if (!s_module) {
    SERR("failed");
    return FALSE;
  }

  module_ctrl = (module_sensor_ctrl_t *)s_module->module_private;
  if (!module_ctrl) {
    SERR("failed");
    return FALSE;
  }

  memset(&bundle_info, 0, sizeof(sensor_bundle_info_t));
  ret = sensor_util_get_sbundle(s_module, stream_info->identity, &bundle_info);
  if (ret == FALSE) {
    SERR("failed");
    return FALSE;
  }

  /* Check whether session id of incoming request matches with current
     port's session id */
  SLOW("current session id %d port session id %d",
    bundle_info.session_id, sensor_src_port_cap->session_id);
  if (bundle_info.session_id != sensor_src_port_cap->session_id) {
    SLOW("session id doesn't match");
    return FALSE;
  }

  /* Check whether this incoming format is compatible with current port's
     supported format */
  ret = sensor_util_check_format(sensor_src_port_cap, stream_info);
  if (ret == FALSE) {
    SLOW("format is not compabible");
    return ret;
  }

  /* add some of stream info to port private
   this will be used for stream bundle */
  port_data = (module_sensor_port_data_t*)(port->port_private);
  port_stream_info = (module_sensor_port_stream_info_t *)
                        malloc(sizeof(module_sensor_port_stream_info_t));
  if (!port_stream_info) {
    SERR("failed");
    return FALSE;
  }
  memset(port_stream_info, 0, sizeof(module_sensor_port_stream_info_t));
  port_stream_info->stream_type = stream_info->stream_type;
  port_stream_info->width = stream_info->dim.width;
  port_stream_info->height = stream_info->dim.height;
  port_stream_info->identity = stream_info->identity;
  port_stream_info->is_stream_on = FALSE;
  port_stream_info->bundle_id = -1;
  port_data->stream_list = mct_list_append(port_data->stream_list,
                                            port_stream_info, NULL, NULL);
  sensor_util_dump_bundle_and_stream_lists(port, __func__, __LINE__);
  /* update non bundle width and height */
  if((stream_info->dis_enable) && (stream_info->stream_type == CAM_STREAM_TYPE_VIDEO)) {
    if (bundle_info.s_bundle->max_width < (stream_info->dim.width * 11) / 10)
      bundle_info.s_bundle->max_width = (stream_info->dim.width * 11) / 10;
    if (bundle_info.s_bundle->max_height < (stream_info->dim.height * 11) / 10)
      bundle_info.s_bundle->max_height = (stream_info->dim.height * 11) / 10;
  } else {
    if (stream_info->pp_config.rotation == ROTATE_90 ||
      stream_info->pp_config.rotation == ROTATE_270) {
        if (bundle_info.s_bundle->max_width < stream_info->dim.height)
          bundle_info.s_bundle->max_width = stream_info->dim.height;
        if (bundle_info.s_bundle->max_height < stream_info->dim.width)
          bundle_info.s_bundle->max_height = stream_info->dim.width;
    } else {
        if (bundle_info.s_bundle->max_width < stream_info->dim.width)
          bundle_info.s_bundle->max_width = stream_info->dim.width;
        if (bundle_info.s_bundle->max_height < stream_info->dim.height)
          bundle_info.s_bundle->max_height = stream_info->dim.height;
    }
  }
  bundle_info.s_bundle->stream_mask |= (1 << stream_info->stream_type);

  SLOW("port=%p, identity=0x%x", port, stream_info->identity);
  SHIGH("ide %x stream type %d w*h %d*%d", stream_info->identity,
    stream_info->stream_type, stream_info->dim.width, stream_info->dim.height);
  return TRUE;
}

/** port_sensor_caps_unreserve: check caps functiont of find
 *  compatible port
 *
 *  @port: port
 *  @streamid: stream id for which this port is linked
 *  @sessionid: session id for which this port is linked
 *
 *  This function releases resources locked during
 *  caps_reserve */

static boolean port_sensor_caps_unreserve(mct_port_t *port,
  unsigned int identity)
{
  boolean                      ret = FALSE;
  mct_module_t                *s_module = NULL;
  module_sensor_bundle_info_t *s_bundle = NULL;
  mct_list_t                  *port_parent = NULL;
  sensor_bundle_info_t         bundle_info;

  SLOW("port=%p, identity=0x%x", port, identity);
  if (!port) {
    SERR("failed");
    return FALSE;
  }
  port_parent = (mct_list_t *)MCT_PORT_PARENT(port);
  if (!port_parent) {
    SERR("failed");
    return FALSE;
  }

  s_module = (mct_module_t *)port_parent->data;
  if (!s_module) {
    SERR("failed");
    return FALSE;
  }

  memset(&bundle_info, 0, sizeof(sensor_bundle_info_t));
  ret = sensor_util_get_sbundle(s_module, identity, &bundle_info);
  if (ret == FALSE) {
    SERR("failed");
    return FALSE;
  }

  /* Remove entries in stream and bundle lists correspoding to identity */
  sensor_util_remove_list_entries_by_identity(port, identity);
  sensor_util_dump_bundle_and_stream_lists(port, __func__, __LINE__);
  /* Clear s_bundle max width, height and stream mask */
  bundle_info.s_bundle->max_width = 0;
  bundle_info.s_bundle->max_height = 0;
  bundle_info.s_bundle->stream_mask = 0;
  SHIGH("ide %x", identity);
  return TRUE;
}

static boolean port_sensor_ext_link_func(unsigned int identity,
 mct_port_t* port, mct_port_t *peer)
{
  boolean                      ret = TRUE;
  mct_module_t                *s_module = NULL;
  module_sensor_ctrl_t        *module_ctrl = NULL;
  module_sensor_bundle_info_t *s_bundle = NULL;
  mct_list_t                  *s_list = NULL;
  mct_list_t                  *port_parent = NULL;
  uint32_t                     session_id = 0, stream_id = 0;

  SHIGH("ide %x", identity);
  if (!port) {
    SERR("failed");
    return FALSE;
  }

  if (!MCT_PORT_PEER(port)) {
    MCT_PORT_PEER(port) = peer;
  } else { /*the link has already been established*/
    if ((MCT_PORT_PEER(port) != peer))
    goto ERROR;
  }
  SLOW("Exit");
  return TRUE;
ERROR:
  SERR("failed");
  return FALSE;
}

static void port_sensor_unlink_func(unsigned int identity,
 mct_port_t *port, mct_port_t *peer)
{
  SHIGH("ide %x", identity);
  if (!port) {
    SERR("failed");
    return;
  }

  if (MCT_PORT_PEER(port) != peer) {
    SERR("failed");
    return;
  }

  SLOW("Exit");
  return;
}

#if SENSOR_MEASURE_FPS
static void port_sensor_measure_fps()
{
  static struct timeval old_tv;
  struct timeval new_tv;
  static int count=0;
  static double fps_sum=0.0;
  gettimeofday(&new_tv, NULL);
  long timediff =
    (new_tv.tv_sec - old_tv.tv_sec)*1000000L +
    (new_tv.tv_usec - old_tv.tv_usec);
  fps_sum += 1000000.0/(timediff);
  old_tv = new_tv;
  count++;
  if (count == 10) {
    SERR("sensor_fps = %.2f", fps_sum/10);
    fps_sum = 0.0;
    count = 0;
  }
 return;
}
#else
static void port_sensor_measure_fps()
{
  return;
}
#endif

static int32_t port_sensor_handle_sof_notify(mct_module_t* module,
  mct_event_t* event, module_sensor_bundle_info_t* s_bundle)
{
  int32_t rc;
  module_sensor_params_t *module_sensor_params;
  if (!module || !event || !s_bundle) {
    SERR("failed");
    return -EINVAL;
  }
  mct_bus_msg_isp_sof_t* sof_event =
         (mct_bus_msg_isp_sof_t*) (event->u.module_event.module_event_data);
  if (!sof_event) {
    SERR("failed");
    return -EINVAL;
  }
  port_sensor_measure_fps();
  SLOW("identity=0x%x, frame_id=%d",
        event->identity, sof_event->frame_id);
  module_sensor_params = s_bundle->module_sensor_params[SUB_MODULE_SENSOR];
  rc = module_sensor_params->func_tbl.process(
            module_sensor_params->sub_module_private,
            SENSOR_SET_VFE_SOF, NULL);
  sensor_util_set_digital_gain_to_isp(module, s_bundle, event->identity);

  /* logic from led-zsl ----(needs clean-up)---------------------- */
  s_bundle->last_idx = sof_event->frame_id;
  uint16_t skip_count;
  rc = module_sensor_params->func_tbl.process(
        module_sensor_params->sub_module_private,
        SENSOR_GET_WAIT_FRAMES, &skip_count);
  SLOW("skip count %d",skip_count);
  if (skip_count >0 )
  {
    skip_count--;
    rc = module_sensor_params->func_tbl.process(
          module_sensor_params->sub_module_private,
          SENSOR_SET_WAIT_FRAMES, &skip_count);
  }

  /* port sensor params to bus if actuator is present */
  sensor_util_post_bus_sensor_params(module, s_bundle, event->identity);
  /* -------------------------------------------------------------*/
  return 0;
}

/** port_sensor_handle_aec_update: process event for sensor
 *  module
 *
 *  @module: mct module handle
 *  @port: mct port handle
 *  @event: event to be processed
 *  @bundle_info: sensor bundle info
 *  @event_module: event module handle
 *
 *  Return: 0 for success and negative error on failure
 *
 *  This function handles aec update event **/

static boolean port_sensor_handle_aec_update(
  mct_module_t *module, mct_port_t *port, mct_event_t *event,
  sensor_bundle_info_t *bundle_info, mct_event_module_t *event_module)
{
  int32_t rc = SENSOR_SUCCESS;
  boolean ret = TRUE;
  module_sensor_params_t *module_sensor_params =
    bundle_info->s_bundle->module_sensor_params[SUB_MODULE_SENSOR];
  stats_update_t *stats_update =
    (stats_update_t *)event_module->module_event_data;
  module_sensor_params_t *led_module_params =
    bundle_info->s_bundle->module_sensor_params[SUB_MODULE_LED_FLASH];
  float digital_gain = 0;
  mct_event_t new_event;
  mct_bus_msg_t bus_msg;
  stats_get_data_t *stats_get = NULL;

  if (!stats_update || !module_sensor_params) {
    SERR("failed");
    return FALSE;
  }
  SLOW("stats gain %f lnct %d", stats_update->aec_update.dig_gain,
    stats_update->aec_update.linecount);
  if (stats_update->flag & STATS_UPDATE_AEC) {
    SLOW("stats update aec");
    rc = module_sensor_params->func_tbl.process(
      module_sensor_params->sub_module_private,
      SENSOR_SET_AEC_UPDATE, &stats_update->aec_update);
    if (rc < 0) {
        SERR("failed");
        return FALSE;
    }
    rc = module_sensor_params->func_tbl.process(
      module_sensor_params->sub_module_private,
      SENSOR_GET_DIGITAL_GAIN, &digital_gain);
    if (rc < 0) {
        SERR("failed");
        return FALSE;
    }
    if (stats_update->aec_update.est_state == AEC_EST_NO_LED_DONE&&bundle_info->s_bundle->state!=3)
    {
       bundle_info->s_bundle->state = 3;
       SERR("AEC EST DONE, no led needed");
       bundle_info->s_bundle->regular_led_trigger = 0;
       bus_msg.sessionid = bundle_info->session_id;
       bus_msg.type = MCT_BUS_MSG_PREPARE_HW_DONE;
       cam_prep_snapshot_state_t state;
       state = DO_NOT_NEED_FUTURE_FRAME;
       bus_msg.msg = &state;
       if(mct_module_post_bus_msg(module,&bus_msg)!=TRUE)
           SERR("Failure posting to the bus!");
       if (rc < 0) {
            SERR("failed");
            return FALSE;
         }
    }
   else if (stats_update->aec_update.est_state == AEC_EST_DONE&&bundle_info->s_bundle->state!=2)
    {
      bundle_info->s_bundle->state =2 ;
      void *sub_module_private = (((module_sensor_params_t *)led_module_params)->sub_module_private);

      SLOW ("AEC EST DONE");
      if (led_module_params->func_tbl.process != NULL) {
        rc = led_module_params->func_tbl.process(
          led_module_params->sub_module_private,
          LED_FLASH_SET_OFF, NULL);
        SHIGH("%s, pre-flash: OFF", __func__);
        if (rc < 0) {
          SERR("failed: LED_FLASH_SET_OFF");
        } else {
          bundle_info->s_bundle->sensor_params.flash_mode = CAM_FLASH_MODE_OFF;
          sensor_util_post_led_state_msg(module, bundle_info->s_bundle,
            event->identity);
        }
      }
      if(bundle_info->s_bundle->regular_led_af == 0){
        /* Execute Red eye reduction (RER) sequence */
        if (led_module_params->func_tbl.process != NULL) {
          rc = led_module_params->func_tbl.process(
            led_module_params->sub_module_private,
            LED_FLASH_SET_RER_PROCESS, led_module_params);
        }
        bundle_info->s_bundle->regular_led_trigger = 1;
        bus_msg.sessionid = bundle_info->session_id;
        bus_msg.type = MCT_BUS_MSG_PREPARE_HW_DONE;
        cam_prep_snapshot_state_t state;
        state = NEED_FUTURE_FRAME;
        bus_msg.msg = &state;
        rc = mct_module_post_bus_msg(module,&bus_msg);
        if(rc != TRUE)
          SERR("Failure posting to the bus!");
      }
    }
    else if (stats_update->aec_update.est_state == AEC_EST_DONE_SKIP) {
      bundle_info->s_bundle->regular_led_af = 0;
      void *sub_module_private =
             (((module_sensor_params_t *)led_module_params)->sub_module_private);

      SLOW ("AEC EST DONE SKIP");
      if (led_module_params->func_tbl.process != NULL) {
        /* Execute Red eye reduction (RER) sequence */
        rc = led_module_params->func_tbl.process(
           led_module_params->sub_module_private,
           LED_FLASH_SET_RER_PROCESS, led_module_params);
      }

      bundle_info->s_bundle->regular_led_trigger = 1;
      bus_msg.sessionid = bundle_info->session_id;
      bus_msg.type = MCT_BUS_MSG_PREPARE_HW_DONE;
      cam_prep_snapshot_state_t state;
      state = NEED_FUTURE_FRAME;
      bus_msg.msg = &state;
      rc = mct_module_post_bus_msg(module,&bus_msg);
      if(rc != TRUE)
        SERR("Failure posting to the bus!");
    }
    else if (stats_update->aec_update.est_state == AEC_EST_START&&bundle_info->s_bundle->state!=1)
    {
        SLOW("AEC EST START");
        if (led_module_params->func_tbl.process != NULL) {
          rc = led_module_params->func_tbl.process(
            led_module_params->sub_module_private,
            LED_FLASH_SET_LOW, NULL);
          if (rc < 0) {
            SERR("failed: LED_FLASH_SET_LOW");
          } else {
            bundle_info->s_bundle->sensor_params.flash_mode =
              CAM_FLASH_MODE_TORCH;
            sensor_util_post_led_state_msg(module, bundle_info->s_bundle,
              event->identity);
          }
        }
        bundle_info->s_bundle->state = 1;
    }
    new_event.type = MCT_EVENT_MODULE_EVENT;
    new_event.identity = event->identity;
    new_event.direction = MCT_EVENT_DOWNSTREAM;
    new_event.u.module_event.type = MCT_EVENT_MODULE_SET_DIGITAL_GAIN;
    new_event.u.module_event.module_event_data = (void *)&digital_gain;
    ret = mct_port_send_event_to_peer(port, &new_event);
    if (ret == FALSE) {
      SERR("failed");
    }
    if (sizeof(stats_get_data_t) >
        sizeof(bundle_info->s_bundle->aec_metadata.private_data)) {
      SERR("failed");
    } else {
      stats_get =
        (stats_get_data_t *)bundle_info->s_bundle->aec_metadata.private_data;
      if (stats_get && ((stats_get->aec_get.real_gain[0] !=
          stats_update->aec_update.real_gain) ||
         (stats_get->aec_get.lux_idx != stats_update->aec_update.lux_idx))) {
        SLOW("g %f lux idx %f", stats_update->aec_update.real_gain,
          stats_update->aec_update.lux_idx);
        stats_get->aec_get.real_gain[0] = stats_update->aec_update.real_gain;
        stats_get->aec_get.lux_idx = stats_update->aec_update.lux_idx;
        stats_get->aec_get.valid_entries = 1;
        bus_msg.sessionid = bundle_info->s_bundle->sensor_info->session_id;
        bus_msg.type = MCT_BUS_MSG_SET_STATS_AEC_INFO;
        bus_msg.size = sizeof(stats_get_data_t);
        bus_msg.msg = (void *)&bundle_info->s_bundle->aec_metadata;
        if(mct_module_post_bus_msg(module, &bus_msg) == FALSE)
          SERR("failed");
      }
    }
  }
  return ret;
}

/** port_sensor_handle_upstream_module_event:
 *
 *  @module: module handle
 *  @port: port handle
 *  @event: event to be handled
 *  @bundle_info: sensor bundle info
 *
 *  Handle upstream module event
 *
 *  Return TRUE on success and FALSE on failure
 **/

static boolean port_sensor_handle_upstream_module_event(mct_module_t *module,
  mct_port_t *port, mct_event_t *event, sensor_bundle_info_t *bundle_info)
{
  int32_t             rc = 0;
  boolean             ret = TRUE;
  mct_event_module_t *event_module = NULL;

  if (!module || !port || !event || !bundle_info) {
    SERR("failed: %p %p %p %p", module, port, event, bundle_info);
    return FALSE;
  }
  event_module = &event->u.module_event;
  SLOW("event id %d", event_module->type);
  switch (event_module->type) {
  case MCT_EVENT_MODULE_GET_CHROMATIX_PTR:
    if (bundle_info->s_bundle->module_sensor_params[SUB_MODULE_CHROMATIX]) {
      event_module->module_event_data =
        bundle_info->s_bundle->module_sensor_params[SUB_MODULE_CHROMATIX]->
        sub_module_private;
    } else {
      SERR("failed");
      ret = FALSE;
    }
    break;
  case MCT_EVENT_MODULE_SOF_NOTIFY:
    if (port_sensor_handle_sof_notify(module, event, bundle_info->s_bundle) <
        0) {
      SERR("failed");
      ret = FALSE;
    }
    break;
  case MCT_EVENT_MODULE_STATS_AEC_UPDATE: {
    ret = port_sensor_handle_aec_update(module,
      port, event, bundle_info, event_module);
    if (ret == FALSE) {
      SERR("failed");
    }
    break;
  }
  case MCT_EVENT_MODULE_STATS_AWB_UPDATE: {
    module_sensor_params_t *module_sensor_params =
      bundle_info->s_bundle->module_sensor_params[SUB_MODULE_SENSOR];
    stats_update_t *stats_update =
      (stats_update_t *)event_module->module_event_data;
    mct_event_t new_event;

    if (!stats_update || !module_sensor_params) {
      SERR("failed");
      return FALSE;
    }

    if (stats_update->flag & STATS_UPDATE_AWB) {
      SLOW("stats update awb hdr");
      rc = module_sensor_params->func_tbl.process(
        module_sensor_params->sub_module_private,
        SENSOR_SET_AWB_UPDATE, &stats_update->awb_update);
      if (rc < 0) {
        SERR("failed");
        return FALSE;
      }
    }
    break;
  }
  case MCT_EVENT_MODULE_STATS_AF_UPDATE: {
    module_sensor_params_t *actuator_module_params =
      bundle_info->s_bundle->module_sensor_params[SUB_MODULE_ACTUATOR];
    stats_update_t *stats_update =
      (stats_update_t *)event_module->module_event_data;
    mct_event_t new_event;
    if (!stats_update || !actuator_module_params->func_tbl.process) {
      SERR("failed");
      ret = FALSE;
      break;
    }
    if (stats_update->flag & STATS_UPDATE_AF) {
      rc = actuator_module_params->func_tbl.process(
        actuator_module_params->sub_module_private,
        ACTUATOR_MOVE_FOCUS, &stats_update->af_update);
      if (rc < 0) {
          SERR("failed");
          ret = FALSE;
          break;
      }
    }
    break;
  }
  case MCT_EVENT_MODULE_3A_GET_CUR_FPS: {
    module_sensor_params_t *module_sensor_params =
      bundle_info->s_bundle->module_sensor_params[SUB_MODULE_SENSOR];
    if (module_sensor_params) {
      rc = module_sensor_params->func_tbl.process(
        module_sensor_params->sub_module_private,
        SENSOR_GET_CUR_FPS, event->u.module_event.module_event_data);
      if (rc < 0) {
          SERR("failed");
          ret = FALSE;
      }
    } else {
      SERR("failed");
      ret = FALSE;
    }
    break;
  }
  case MCT_EVENT_MODULE_ISP_CHANGE_OP_PIX_CLK: {
    ret = module_sensor_handle_pixel_clk_change(module, event->identity,
      event->u.module_event.module_event_data);
    if (ret == FALSE) {
      SERR("MCT_EVENT_MODULE_ISP_CHANGE_OP_PIX_CLK failed");
    }
    break;
  }

  case MCT_EVENT_MODULE_LED_STATE_TIMEOUT:
    SERR("Reset previously set LED state!");
    bundle_info->s_bundle->regular_led_trigger = 0;
    break;
  case  MCT_EVENT_MODULE_LED_AF_UPDATE:
    SERR("MCT_EVENT_MODULE_LED_AF_UPDATE");
    bundle_info->s_bundle->regular_led_af =
      *(int*)(event->u.module_event.module_event_data);
    break;
  default:
    break;
  }
  return ret;
}

/** port_sensor_port_process_event: process event for sensor
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

static boolean port_sensor_port_process_event(mct_port_t *port,
  mct_event_t *event)
{
  boolean                      ret = TRUE;
  int32_t                      rc = SENSOR_SUCCESS;
  module_sensor_ctrl_t        *module_ctrl = NULL;
  mct_module_t                *module = NULL;
  mct_event_module_t          *event_module = NULL;
  mct_event_control_t         *event_control = NULL;
  sensor_bundle_info_t         bundle_info;

  if (!port || !event) {
    SERR("failed port %p event %p", port,
      event);
    return FALSE;
  }
  module = MCT_MODULE_CAST((MCT_PORT_PARENT(port))->data);
  if (!module) {
    SERR("failed");
    return FALSE;
  }

  module_ctrl = (module_sensor_ctrl_t *)module->module_private;
  if (!module_ctrl) {
    SERR("failed");
    return FALSE;
  }

  memset(&bundle_info, 0, sizeof(sensor_bundle_info_t));
  ret = sensor_util_get_sbundle(module, event->identity, &bundle_info);
  if (ret == FALSE) {
    SERR("failed");
    return FALSE;
  }

  if (event->type == MCT_EVENT_MODULE_EVENT) {
    event_module = &event->u.module_event;
    if (event->direction == MCT_EVENT_DOWNSTREAM) {
      ret = mct_port_send_event_to_peer(port, event);
      if (ret == FALSE) {
        SERR("failed");
      }
    } else if (event->direction == MCT_EVENT_UPSTREAM) {
      ret = port_sensor_handle_upstream_module_event(module, port, event,
        &bundle_info);
      if (ret == FALSE) {
        SERR("failed: port_sensor_handle_upstream_module_event");
      }
    } else {
      SERR("failed: invalid module event dir %d", event->direction);
      ret = FALSE;

    }
  } else if (event->type == MCT_EVENT_CONTROL_CMD) {
    ret = mct_port_send_event_to_peer(port, event);
    if (ret == FALSE) {
      SERR("failed");
      /* TODO to make test app work */
      ret = TRUE;
    }
  }
  return ret;
}

/** port_sensor_create: create ports for sensor module
 *
 *  @data: module_sensor_bundle_info_t pointer
 *  @user_data: mct_module_t pointer
 *
 *  Return: 0 for success and negative error on failure
 *
 *  This function creates one source port and fills capabilities
 *  for the created port, adds it the sensor module **/

boolean port_sensor_create(void *data, void *user_data)
{
  boolean                      ret = TRUE;
  int32_t                      rc = SENSOR_SUCCESS;
  mct_port_t                  *s_port = NULL;
  module_sensor_bundle_info_t *s_bundle = (module_sensor_bundle_info_t *)data;
  sensor_lib_params_t         *sensor_lib_params = NULL;
  mct_module_t                *s_module = (mct_module_t *)user_data;
  module_sensor_ctrl_t        *module_ctrl = NULL;
  int32_t                      i = 0, j = 0;
  char                         port_name[32];
  module_sensor_params_t      *module_sensor_params = NULL;
  sensor_stream_info_array_t  *sensor_stream_info_array = NULL;
  sensor_src_port_cap_t       *sensor_src_port_cap = NULL;
  module_sensor_params_t      *csid_module_params = NULL;
  uint32_t                     csid_version = 0;
  uint8_t                      num_meta_ch = 0;

  if (!s_bundle || !s_module) {
    SERR("failed data1 %p data2 %p", data, user_data);
    return FALSE;
  }
  module_sensor_params = s_bundle->module_sensor_params[SUB_MODULE_SENSOR];
  csid_module_params = s_bundle->module_sensor_params[SUB_MODULE_CSID];
  if (!s_bundle->module_sensor_params[i]->func_tbl.open) {
    SERR("failed");
    return FALSE;
  }
  rc = csid_module_params->func_tbl.open(
    &csid_module_params->sub_module_private,
    s_bundle->sensor_sd_name[SUB_MODULE_CSID]);
  if (rc < 0) {
    SERR("failed");
    return FALSE;
  }

  rc = csid_module_params->func_tbl.process(
    csid_module_params->sub_module_private,
    CSID_GET_VERSION, &csid_version);
  if (rc < 0) {
    SERR("failed");
    return FALSE;
  }
  csid_module_params->func_tbl.close(
    csid_module_params->sub_module_private);

  module_ctrl = (module_sensor_ctrl_t *)s_module->module_private;
  sensor_lib_params = s_bundle->sensor_lib_params;
  rc = sensor_load_library(s_bundle->sensor_info->sensor_name,
    sensor_lib_params);
  if (rc < 0) {
    SERR("failed %d", rc);
    return FALSE;
  }
  rc = module_sensor_params->func_tbl.process(s_bundle->sensor_lib_params,
    SENSOR_GET_SENSOR_PORT_INFO, &sensor_stream_info_array);
  if (rc < 0) {
    SERR("failed %d", rc);
    goto ERROR1;
  }
  SLOW("sensor_stream_info_array->size %d", sensor_stream_info_array->size);
  for (j = 0; j < sensor_stream_info_array->size; j++) {
    if (SENSOR_CID_CH_MAX <
      sensor_stream_info_array->sensor_stream_info[j].vc_cfg_size) {
      SERR("vc_cfg_size out of range (%d) ",
        (int) sensor_stream_info_array->sensor_stream_info[j].vc_cfg_size);
      goto ERROR1;
    }
    snprintf(port_name, sizeof(port_name), "%s%d",
      s_bundle->sensor_info->sensor_name, j);
    s_port = mct_port_create(port_name);
    if (!s_port) {
      SERR("failed");
      goto ERROR1;
    }
    sensor_src_port_cap = malloc(sizeof(sensor_src_port_cap_t));
    if (!sensor_src_port_cap) {
      SERR("failed");
      goto ERROR2;
    }
    memset(sensor_src_port_cap, 0, sizeof(sensor_src_port_cap_t));

    sensor_src_port_cap->session_id = s_bundle->sensor_info->session_id;
    sensor_src_port_cap->num_cid_ch =
      sensor_stream_info_array->sensor_stream_info[j].vc_cfg_size;
    if (sensor_src_port_cap->num_cid_ch >= SENSOR_CID_CH_MAX ) {
      SERR("num_cid_ch size out of range (%d) ", sensor_src_port_cap->num_cid_ch);
      goto ERROR2;
    }
    for (i = 0; i < sensor_src_port_cap->num_cid_ch; i++) {
      sensor_src_port_cap->sensor_cid_ch[i].cid =
        sensor_stream_info_array->sensor_stream_info[j].vc_cfg[i].cid;
      sensor_src_port_cap->sensor_cid_ch[i].csid =
        s_bundle->sensor_info->subdev_id[SUB_MODULE_CSID];

      sensor_src_port_cap->sensor_cid_ch[i].csid_version = csid_version;
      /* TODO: don't hardcode this, get from user space driver */
      sensor_src_port_cap->sensor_cid_ch[i].is_bayer_sensor = 1;
      /* TODO: Create more enums in cam_intf.h for all bayer patterns */
      switch (sensor_stream_info_array->sensor_stream_info[j].pix_fmt_fourcc[i].fourcc) {
      case V4L2_PIX_FMT_SBGGR8:
        sensor_src_port_cap->sensor_cid_ch[i].fmt =
          CAM_FORMAT_BAYER_MIPI_RAW_8BPP_BGGR;
        break;
      case V4L2_PIX_FMT_SGBRG8:
        sensor_src_port_cap->sensor_cid_ch[i].fmt =
          CAM_FORMAT_BAYER_MIPI_RAW_8BPP_GBRG;
        break;
      case V4L2_PIX_FMT_SGRBG8:
        sensor_src_port_cap->sensor_cid_ch[i].fmt =
          CAM_FORMAT_BAYER_MIPI_RAW_8BPP_GRBG;
        break;
      case V4L2_PIX_FMT_SRGGB8:
        sensor_src_port_cap->sensor_cid_ch[i].fmt =
          CAM_FORMAT_BAYER_MIPI_RAW_8BPP_RGGB;
        break;

      case V4L2_PIX_FMT_SBGGR10:
        sensor_src_port_cap->sensor_cid_ch[i].fmt =
          CAM_FORMAT_BAYER_MIPI_RAW_10BPP_BGGR;
        break;
      case V4L2_PIX_FMT_SGBRG10:
        sensor_src_port_cap->sensor_cid_ch[i].fmt =
          CAM_FORMAT_BAYER_MIPI_RAW_10BPP_GBRG;
        break;
      case V4L2_PIX_FMT_SGRBG10:
        sensor_src_port_cap->sensor_cid_ch[i].fmt =
          CAM_FORMAT_BAYER_MIPI_RAW_10BPP_GRBG;
        break;
      case V4L2_PIX_FMT_SRGGB10:
        sensor_src_port_cap->sensor_cid_ch[i].fmt =
          CAM_FORMAT_BAYER_MIPI_RAW_10BPP_RGGB;
        break;

      case V4L2_PIX_FMT_SBGGR12:
        sensor_src_port_cap->sensor_cid_ch[i].fmt =
          CAM_FORMAT_BAYER_MIPI_RAW_12BPP_BGGR;
        break;
      case V4L2_PIX_FMT_SGBRG12:
        sensor_src_port_cap->sensor_cid_ch[i].fmt =
          CAM_FORMAT_BAYER_MIPI_RAW_12BPP_GBRG;
        break;
      case V4L2_PIX_FMT_SGRBG12:
        sensor_src_port_cap->sensor_cid_ch[i].fmt =
          CAM_FORMAT_BAYER_MIPI_RAW_12BPP_GRBG;
        break;
      case V4L2_PIX_FMT_SRGGB12:
        sensor_src_port_cap->sensor_cid_ch[i].fmt =
          CAM_FORMAT_BAYER_MIPI_RAW_12BPP_RGGB;
        break;

      case V4L2_PIX_FMT_NV12:
        sensor_src_port_cap->sensor_cid_ch[i].fmt = CAM_FORMAT_YUV_420_NV12;
        break;
      case V4L2_PIX_FMT_NV16:
        sensor_src_port_cap->sensor_cid_ch[i].fmt = CAM_FORMAT_YUV_422_NV16;
        break;
      case V4L2_PIX_FMT_NV61:
        sensor_src_port_cap->sensor_cid_ch[i].fmt = CAM_FORMAT_YUV_422_NV61;
        break;
      case V4L2_MBUS_FMT_YUYV8_2X8:
        sensor_src_port_cap->sensor_cid_ch[i].is_bayer_sensor = 0;
        sensor_src_port_cap->sensor_cid_ch[i].fmt =
          CAM_FORMAT_YUV_RAW_8BIT_YUYV;
      break;
      case V4L2_MBUS_FMT_YVYU8_2X8:
        sensor_src_port_cap->sensor_cid_ch[i].is_bayer_sensor = 0;
        sensor_src_port_cap->sensor_cid_ch[i].fmt =
          CAM_FORMAT_YUV_RAW_8BIT_YVYU;
      break;
      case V4L2_MBUS_FMT_UYVY8_2X8:
        sensor_src_port_cap->sensor_cid_ch[i].is_bayer_sensor = 0;
        sensor_src_port_cap->sensor_cid_ch[i].fmt =
          CAM_FORMAT_YUV_RAW_8BIT_UYVY;
      break;
      case V4L2_MBUS_FMT_VYUY8_2X8:
        sensor_src_port_cap->sensor_cid_ch[i].is_bayer_sensor = 0;
        sensor_src_port_cap->sensor_cid_ch[i].fmt =
          CAM_FORMAT_YUV_RAW_8BIT_VYUY;
      break;
      case MSM_V4L2_PIX_FMT_META:
        num_meta_ch = sensor_src_port_cap->num_meta_ch;

        sensor_src_port_cap->meta_ch[num_meta_ch].is_bayer_sensor = 0;
        sensor_src_port_cap->meta_ch[num_meta_ch].fmt =
          CAM_FORMAT_META_RAW_8BIT;
        sensor_src_port_cap->meta_ch[num_meta_ch].cid =
          sensor_stream_info_array->sensor_stream_info[j].vc_cfg[i].cid;
        sensor_src_port_cap->meta_ch[num_meta_ch].csid =
          s_bundle->sensor_info->subdev_id[SUB_MODULE_CSID];

        sensor_src_port_cap->num_meta_ch++;
        sensor_src_port_cap->num_cid_ch--;
      break;
      default:
        sensor_src_port_cap->sensor_cid_ch[i].fmt =
          CAM_FORMAT_MAX;
        break;
      }
    }
    s_port->direction = MCT_PORT_SRC;
    s_port->check_caps_reserve = port_sensor_caps_reserve;
    s_port->check_caps_unreserve = port_sensor_caps_unreserve;
    s_port->ext_link = port_sensor_ext_link_func;
    s_port->un_link = port_sensor_unlink_func;
    s_port->event_func = port_sensor_port_process_event;
    SLOW("s_port=%p event_func=%p", s_port, s_port->event_func);
    s_port->caps.u.data = (void *)sensor_src_port_cap;
    s_port->port_private = (module_sensor_port_data_t *)
      malloc(sizeof(module_sensor_port_data_t));
    if (!s_port->port_private) {
      SERR("failed");
      goto ERROR2;
    }
    memset(s_port->port_private, 0, sizeof(module_sensor_port_data_t));
    ret = mct_module_add_port(s_module, s_port);
    if (ret == FALSE) {
      SERR("failed");
      goto ERROR2;
    }
  }
  sensor_unload_library(s_bundle->sensor_lib_params);
  SLOW("Exit");
  return TRUE;
ERROR2:
  mct_port_destroy(s_port);
ERROR1:
  sensor_unload_library(s_bundle->sensor_lib_params);
  SERR("failed");
  return FALSE;
}
