/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *         Modify History For This Module
 * When           Who             What,Where,Why
 * --------------------------------------------------------------------------------------
 * 13/09/21             Add driver for RoilTF's 1st sub CAM [can't work now]
 * --------------------------------------------------------------------------------------
*/

#include <stdio.h>
#include "sensor_lib.h"

static sensor_lib_t sensor_lib_ptr;

static sensor_output_t sensor_output = {
  .output_format = SENSOR_YCBCR,
  .connection_mode = SENSOR_MIPI_CSI,
  .raw_output = SENSOR_8_BIT_DIRECT,
};

#ifndef VFE_40
static struct csi_lane_params_t csi_lane_params = {
  .csi_lane_assign = 0xe4,
  .csi_lane_mask = 0x03,
  .csi_if = 1,
  .csid_core = {0},
  .csi_phy_sel = 0,
};
#else
static struct csi_lane_params_t csi_lane_params = {
  .csi_lane_assign = 0xE4,
  .csi_lane_mask = 0x1,
  .csi_if = 1,
  .csid_core = {0},
  .csi_phy_sel = 1,
};
#endif

static sensor_lens_info_t default_lens_info = {
  .focal_length = 2.64, //EFL
  .pix_size = 1.75,
  .f_number = 2.8, //F no
  .total_f_dist = 1.97,
  .hor_view_angle = 57.5,
  .ver_view_angle = 44.7,
};

static msm_sensor_dimension_t gc0310_scale_size_tbl[] = {
  { 640, 480}, // VGA
};
static struct msm_camera_csid_vc_cfg gc0310_cid_cfg[] = {
  {0, CSI_YUV422_8, CSI_DECODE_8BIT},
  {1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params gc0310_csi_params = {
  .csid_params = {
    .lane_cnt = 1,
    .lut_params = {
      .num_cid = ARRAY_SIZE(gc0310_cid_cfg),
      .vc_cfg = {
         &gc0310_cid_cfg[0],
         &gc0310_cid_cfg[1],
      },
    },
  },
  .csiphy_params = {
    .lane_cnt = 1,
    .settle_cnt = 20,// 100ns 0x14
  },
};

static struct sensor_pix_fmt_info_t gc0310_pix_fmt0_fourcc[] = {
  { V4L2_MBUS_FMT_YUYV8_2X8 },
};

static struct sensor_pix_fmt_info_t gc0310_pix_fmt1_fourcc[] = {
  { MSM_V4L2_PIX_FMT_META },
};

static sensor_stream_info_t gc0310_stream_info[] = {
  {1, &gc0310_cid_cfg[0], gc0310_pix_fmt0_fourcc},
  {1, &gc0310_cid_cfg[1], gc0310_pix_fmt1_fourcc},
};

static sensor_stream_info_array_t gc0310_stream_info_array = {
  .sensor_stream_info = gc0310_stream_info,
  .size = ARRAY_SIZE(gc0310_stream_info),
};

static struct msm_camera_csi2_params *csi_params[] = {
  &gc0310_csi_params, /* RES 0*/
  &gc0310_csi_params, /* RES 1*/
};

static struct sensor_lib_csi_params_array csi_params_array = {
  .csi2_params = &csi_params[0],
  .size = ARRAY_SIZE(csi_params),
};

static struct sensor_crop_parms_t crop_params[] = {
// top, bottom, left, right
  {0, 0, 0, 0}, /* RES 0 */
  {0, 0, 0, 0}, /* RES 1 */
};

static struct sensor_lib_crop_params_array crop_params_array = {
  .crop_params = crop_params,
  .size = ARRAY_SIZE(crop_params),
};

static int32_t gc0310_get_scale_tbl(msm_sensor_dimension_t * tbl)
{
  int i;
  if(sensor_lib_ptr.scale_tbl_cnt == 0)
    return -1;
  for(i = 0; i < sensor_lib_ptr.scale_tbl_cnt; i++){
    tbl[i] = gc0310_scale_size_tbl[i];  
  }

  return 0;
}

static struct sensor_lib_out_info_t sensor_out_info[] = {
  { /* For SNAPSHOT */
    .x_output = 640*2,
    .y_output = 480,
// no many use for YUV
    .line_length_pclk =640*2, /* 1600 */
    .frame_length_lines = 480, /* 1200 */
    .vt_pixel_clk = 144000000,
    .op_pixel_clk = 320000000,
	.binning_factor = 1,
    .max_fps = 15.0,
    .min_fps = 7.5,
        .mode = SENSOR_DEFAULT_MODE,
  },
{
    /* QTR size */

    .x_output = 640 * 2,
    .y_output = 480,
    .line_length_pclk = 640*2, /* 1600 */
    .frame_length_lines = 480, /* 1200 */
    .vt_pixel_clk = 144000000,
    .op_pixel_clk = 320000000,
    .binning_factor = 1,
    .max_fps = 30.0,
    .min_fps = 7.5,
    .mode = SENSOR_DEFAULT_MODE,
},
};

static struct sensor_lib_out_info_array out_info_array = {
  .out_info = sensor_out_info,
  .size = ARRAY_SIZE(sensor_out_info),
};

static sensor_res_cfg_type_t gc0310_res_cfg[] = {
  SENSOR_SET_STOP_STREAM,
  SENSOR_SET_NEW_RESOLUTION, /* set stream config */
  SENSOR_SET_CSIPHY_CFG,
  SENSOR_SET_CSID_CFG,
  SENSOR_SEND_EVENT, /* send event */
  SENSOR_SET_START_STREAM,
};

static struct sensor_res_cfg_table_t gc0310_res_table = {
  .res_cfg_type = gc0310_res_cfg,
  .size = ARRAY_SIZE(gc0310_res_cfg),
};

static sensor_lib_t sensor_lib_ptr = {
  /* sensor output settings */
  .sensor_output = &sensor_output,
  /* number of frames to skip after start stream */
  .sensor_num_frame_skip = 1,
  /* number of frames to skip after start HDR stream */
  .sensor_num_HDR_frame_skip = 1,
  /* sensor lens info */
  .default_lens_info = &default_lens_info,
  /* csi lane params */
  .csi_lane_params = &csi_lane_params,
  /* csi cid params */
  .csi_cid_params = gc0310_cid_cfg,
  /* csi csid params array size */
  .csi_cid_params_size = ARRAY_SIZE(gc0310_cid_cfg),
  /* resolution cfg table */
  .sensor_res_cfg_table = &gc0310_res_table,
  /* out info array */
  .out_info_array = &out_info_array,
  /* crop params array */
  .crop_params_array = &crop_params_array,
  /* csi params array */
  .csi_params_array = &csi_params_array,
  /* sensor port info array */
  .sensor_stream_info_array = &gc0310_stream_info_array,
    /* scale size table count*/
  .scale_tbl_cnt = ARRAY_SIZE(gc0310_scale_size_tbl),
  /*function to get scale size tbl */
  .get_scale_tbl = gc0310_get_scale_tbl,
};

/*===========================================================================
 * FUNCTION    - gc0310_open_lib -
 *
 * DESCRIPTION:
 *==========================================================================*/
void *gc0310_open_lib(void)
{
//    ALOGE("=====>in [gc0310_open_lib]\n");
  return &sensor_lib_ptr;
}
