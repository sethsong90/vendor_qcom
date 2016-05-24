/* is_process.c
 *
 * Copyright (c) 2013 - 2014 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
#include "is.h"
/* This should be declared in sensor_lib.h */
void poke_gyro_sample(uint64_t t, int32_t gx, int32_t gy, int32_t gz);


#if 1
#undef CDBG
#define CDBG ALOGE
#endif


/** is_process_is_initialize:
 *    @private: IS port's internal variables
 *
 * This function initializes IS.
 **/
static void is_process_is_initialize(is_info_t *is_info)
{
  int rc;
  is_init_data_t is_init_data;

  is_init_data.frame_cfg.frame_fps = 30;
  is_init_data.frame_cfg.dis_frame_width = is_info->width;
  is_init_data.frame_cfg.dis_frame_height = is_info->height;
  is_init_data.frame_cfg.vfe_output_width = is_info->vfe_width;
  is_init_data.frame_cfg.vfe_output_height = is_info->vfe_height;
  is_init_data.rs_cs_config.num_row_sum = is_info->num_row_sum;
  is_init_data.rs_cs_config.num_col_sum = is_info->num_col_sum;
  is_init_data.is_mode = is_info->is_mode;
  is_init_data.sensor_mount_angle = is_info->sensor_mount_angle;
  is_init_data.camera_position = is_info->camera_position;

  /* For now, DIS and EIS initialization need to succeed */
  memset(&is_info->dis_context, 0, sizeof(dis_context_type));

  /* If IS is enabled but user did not select IS technology preference, default
     to DIS. */
  if (is_info->is_mode == IS_TYPE_NONE) {
    is_info->is_mode == IS_TYPE_DIS;
    CDBG_HIGH("%s: Default to DIS", __func__);
  }

  rc = dis_initialize(&is_info->dis_context, &is_init_data);
  if (rc == 0) {
    if (is_info->is_mode == IS_TYPE_EIS_1_0) {
      memset(&is_info->eis_context, 0, sizeof(eis_context_type));
      rc = eis_initialize(&is_info->eis_context, &is_init_data);
    } else if (is_info->is_mode == IS_TYPE_EIS_2_0) {
      memset(&is_info->eis2_context, 0, sizeof(eis2_context_type));
      rc = eis2_initialize(&is_info->eis2_context, &is_init_data);
    }
    if (rc == 0) {
      is_info->is_inited = 1;
      CDBG("%s: IS inited", __func__);
    }
    else {
      dis_exit(&is_info->dis_context);
    }
  }

  if (rc == 0 && is_info->is_mode != IS_TYPE_DIS) {
    sns_eis2_init(NULL);
    is_info->sns_lib_offset_set = 0;
  }

  if (rc != 0) {
    CDBG_ERROR("%s: IS initialization failed", __func__);
    /* Disable IS so we won't keep initializing and failing */
    is_info->is_enabled = 0;
  }
}


/** is_process is_deinitialize:
 *    @private: IS port's internal variables
 *
 * This function deinits IS.
 **/
static void is_process_is_deinitialize(is_info_t *is_info)
{
  if (is_info->is_mode != IS_TYPE_DIS) {
    sns_eis2_stop();
  }

  if (is_info->is_mode == IS_TYPE_EIS_1_0) {
    eis_deinitialize(&is_info->eis_context);
  } else if (is_info->is_mode == IS_TYPE_EIS_2_0) {
    eis2_deinitialize(&is_info->eis2_context);
  }
  dis_exit(&is_info->dis_context);
  is_info->is_inited = 0;
  CDBG("%s: IS deinited", __func__);
}


/** is_process_stats_event:
 *    @port: IS port
 *    @event: stats event
 *    @private: IS port's internal variables
 *
 * This function handles stats events associated with RS stats and CS stats.
 **/
static void is_process_stats_event(is_stats_data_t *stats_data,
  is_output_type *is_output)
{
  mct_event_t gyro_data_event;
  mct_event_gyro_data_t gyro_data;
  frame_times_t frame_times;
  struct timespec t_now;
  is_info_t *is_info = stats_data->is_info;

  if (!is_info->is_inited) {
    is_process_is_initialize(is_info);
  }

  if (is_info->is_inited) {
    clock_gettime( CLOCK_REALTIME, &t_now );
    CDBG("%s: RS(%u) & CS(%u) ready, time = %llu, ts = %llu, id = %u", __func__,
      stats_data->is_info->num_row_sum, stats_data->is_info->num_col_sum,
      ((int64_t)t_now.tv_sec * 1000 + t_now.tv_nsec/1000000),
      ((int64_t)stats_data->is_info->timestamp.tv_sec * 1000 +
                stats_data->is_info->timestamp.tv_usec/1000),
      stats_data->frame_id);
    is_output->frame_id = stats_data->frame_id;

    /* Get the gyro data only if IS mode is not DIS */
    if (is_info->is_mode != IS_TYPE_DIS) {
      /* We can remove this sleep when IS thread waits for gyro data instead
       * of polling for gyro data.  This requires rework of gyro module.
       */
      usleep(5000);
      gyro_data.ready = 0;
      gyro_data_event.type = MCT_EVENT_MODULE_EVENT;
      gyro_data_event.identity = stats_data->identity;
      gyro_data_event.direction = MCT_EVENT_UPSTREAM;
      gyro_data_event.u.module_event.type = MCT_EVENT_MODULE_GET_GYRO_DATA;
      gyro_data_event.u.module_event.module_event_data = &gyro_data;
      gyro_data.frame_id = stats_data->frame_id;
      mct_port_send_event_to_peer(stats_data->port, &gyro_data_event);

      memset(&frame_times, 0, sizeof(frame_times_t));
      if (gyro_data.ready) {
        unsigned int i;
#ifdef FEATURE_GYRO
        if (!is_info->sns_lib_offset_set) {
          set_sns_apps_offset(gyro_data.sample[0].timestamp);
          is_info->sns_lib_offset_set = 1;
        }

        CDBG("%s: num_samples %d", __func__, gyro_data.sample_len);
        /* Update the gyro buffer */
        for (i = 0; i < gyro_data.sample_len; i++) {
          CDBG("Poking in gyro sample, %llu, %d, %d, %d",
            gyro_data.sample[i].timestamp,
            gyro_data.sample[i].value[0],
            gyro_data.sample[i].value[1],
            gyro_data.sample[i].value[2]);
          poke_gyro_sample(gyro_data.sample[i].timestamp,
            gyro_data.sample[i].value[0],
            gyro_data.sample[i].value[1],
            gyro_data.sample[i].value[2]);
        }
#endif
        frame_times.sof = gyro_data.sof;
        frame_times.exposure_time = gyro_data.exposure_time;
        frame_times.frame_time = gyro_data.frame_time;
        CDBG("%s: gyro_data.sof = %llu", __func__, frame_times.sof);
        if (is_info->is_mode == IS_TYPE_EIS_1_0) {
          eis_process(&is_info->eis_context, &frame_times, is_output);
        } else if (is_info->is_mode == IS_TYPE_EIS_2_0) {
          eis2_process(&is_info->eis2_context, &frame_times, is_output);
        }
      }
      else {
        CDBG("%s: Gyro data not ready", __func__);
      }
    }

    dis_process(&is_info->dis_context, &is_info->rs_cs_data,
     &frame_times, is_output);
  }
}


/** is_process:
 *    @param: input event parameters
 *    @output: output of the event processing
 *
 * This function is the top level event handler.
 **/
void is_process(is_process_parameter_t *param, is_process_output_t *output)
{
  switch (param->type) {
  case IS_PROCESS_OUTPUT_RS_CS_STATS:
    CDBG("%s: IS_PROCESS_STREAM_RS_CS_STATS", __func__);
    output->type = IS_PROCESS_OUTPUT_RS_CS_STATS;
    if (param->u.stats_data.run_is) {
      is_process_stats_event(&param->u.stats_data, output->is_output);
    } else {
      output->is_output->frame_id = param->u.stats_data.frame_id;
    }
    break;

  case IS_PROCESS_STREAM_EVENT:
    CDBG("%s: IS_PROCESS_STREAM_EVENT, s = %d", __func__,
      param->u.stream_event_data.stream_event);
    output->type = IS_PROCESS_OUTPUT_STREAM_EVENT;
    if (param->u.stream_event_data.stream_event == IS_VIDEO_STREAM_ON) {
      output->video_stream_on = 1;
    } else {
      output->video_stream_on = 0;
      if (param->u.stream_event_data.is_info->is_inited) {
        is_process_is_deinitialize(param->u.stream_event_data.is_info);
      }
    }
    break;

  default:
    break;
  }
} /* is_process */
