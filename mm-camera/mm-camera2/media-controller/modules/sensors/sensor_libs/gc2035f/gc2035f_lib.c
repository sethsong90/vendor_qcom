/* hi256_lib.c
 *
 * Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
   
#include <stdio.h>
#include "sensor_lib.h"
//#include "cam_types.h"

static sensor_lib_t sensor_lib_ptr;

static sensor_output_t sensor_output = {
  .output_format = SENSOR_YCBCR,
  .connection_mode = SENSOR_MIPI_CSI,
  .raw_output = SENSOR_8_BIT_DIRECT,
};

static sensor_lens_info_t default_lens_info = {
  .focal_length = 4.6,
  .pix_size = 1.4,
  .f_number = 2.65,
  .total_f_dist = 1.97,
  .hor_view_angle = 54.8,
  .ver_view_angle = 42.5,
};
#ifndef VFE_40
static struct csi_lane_params_t csi_lane_params = {
  .csi_lane_assign = 0xE4,
  .csi_lane_mask = 0x3,
  .csi_if = 1,
  .csid_core = {1},
  .csi_phy_sel = 1,
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
/*yuzhaohua gc2035 change 2014.1.28 start */

static msm_sensor_dimension_t gc2035_scale_size_tbl[] = {
  {1600, 1200},
};
/*yuzhaohua gc2035 change 2014.1.28 end */
static struct msm_camera_csid_vc_cfg gc2035_cid_cfg[] = {
  {0, CSI_YUV422_8, CSI_DECODE_8BIT},
  {1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params gc2035_csi_params = {
  .csid_params = {
    .lane_cnt = 1,
    .lut_params = {
      .num_cid = ARRAY_SIZE(gc2035_cid_cfg),
      .vc_cfg = {
         &gc2035_cid_cfg[0],
         &gc2035_cid_cfg[1],
      },
    },
  },
  .csiphy_params = {
    .lane_cnt = 1,//1
    .settle_cnt = 0x1c,
  },
};

static struct sensor_pix_fmt_info_t gc2035_pix_fmt0_fourcc[] = {
  { V4L2_MBUS_FMT_YUYV8_2X8 },
};

static struct sensor_pix_fmt_info_t gc2035_pix_fmt1_fourcc[] = {
  { MSM_V4L2_PIX_FMT_META },
};

static sensor_stream_info_t gc2035_stream_info[] = {
  {1, &gc2035_cid_cfg[0], gc2035_pix_fmt0_fourcc},
  {1, &gc2035_cid_cfg[1], gc2035_pix_fmt1_fourcc},
};

static sensor_stream_info_array_t gc2035_stream_info_array = {
  .sensor_stream_info = gc2035_stream_info,
  .size = ARRAY_SIZE(gc2035_stream_info),
};

static struct msm_camera_csi2_params *csi_params[] = {
  &gc2035_csi_params, /* RES 0*/
  //&gc2035_csi_params, /* RES 1*/
};

static struct sensor_lib_csi_params_array csi_params_array = {
  .csi2_params = &csi_params[0],
  .size = ARRAY_SIZE(csi_params),
};

static struct sensor_crop_parms_t crop_params[] = {
  {0, 0, 0, 0}, /* RES 0 */
//  {0, 0, 0, 0}, /* RES 1 */
};

static struct sensor_lib_crop_params_array crop_params_array = {
  .crop_params = crop_params,
  .size = ARRAY_SIZE(crop_params),
};

static struct sensor_lib_out_info_t sensor_out_info[] = {
#if 1
  {
    /* full size */
    .x_output = 0x640*2,
    .y_output = 0x4b0,
    .line_length_pclk = 0x793*2, /* 1600 */
    .frame_length_lines = 0x4d4, /* 1200 */
    .vt_pixel_clk = 18000000,
    .op_pixel_clk = 18000000,
    .binning_factor = 1,
    .max_fps = 30.0,
    .min_fps = 7.5,
    .mode = SENSOR_DEFAULT_MODE,
  },
  #else
  {
    /* full size */
    ..x_output = 0x640,
    .y_output = 0x4b0,
    .line_length_pclk = 0x793, /* 1600 */
    .frame_length_lines = 0x4d4, /* 1200 */
    .vt_pixel_clk = 18000000,
    .op_pixel_clk = 18000000,
    .binning_factor = 1,
    .max_fps = 30.0,
    .min_fps = 7.5,
  }
 #endif
};

static struct sensor_lib_out_info_array out_info_array = {
  .out_info = sensor_out_info,
  .size = ARRAY_SIZE(sensor_out_info),
};

static sensor_res_cfg_type_t gc2035_res_cfg[] = {
  SENSOR_SET_STOP_STREAM,
  SENSOR_SET_NEW_RESOLUTION, /* set stream config */
  SENSOR_SET_CSIPHY_CFG,
  SENSOR_SET_CSID_CFG,
  SENSOR_SEND_EVENT, /* send event */
  SENSOR_SET_START_STREAM,
};

static struct sensor_res_cfg_table_t gc2035_res_table = {
  .res_cfg_type = gc2035_res_cfg,
  .size = ARRAY_SIZE(gc2035_res_cfg),
};

/*yuzhaohua gc2035 change 2014.1.28 start */
static int32_t gc2035_get_scale_tbl(msm_sensor_dimension_t * tbl)
{
  int i;
  if(sensor_lib_ptr.scale_tbl_cnt == 0)
    return -1;
  for(i = 0; i < sensor_lib_ptr.scale_tbl_cnt; i++){
    tbl[i] = gc2035_scale_size_tbl[i];
  }

  return 0;
}
#ifndef LOCAL_TYPE_CAM
#define LOCAL_TYPE_CAM
typedef enum {
    CAM_SCENE_MODE_OFF,
    CAM_SCENE_MODE_AUTO,
    CAM_SCENE_MODE_LANDSCAPE,
    CAM_SCENE_MODE_SNOW,
    CAM_SCENE_MODE_BEACH,
    CAM_SCENE_MODE_SUNSET,
    CAM_SCENE_MODE_NIGHT,
    CAM_SCENE_MODE_PORTRAIT,
    CAM_SCENE_MODE_BACKLIGHT,
    CAM_SCENE_MODE_SPORTS,
    CAM_SCENE_MODE_ANTISHAKE,
    CAM_SCENE_MODE_FLOWERS,
    CAM_SCENE_MODE_CANDLELIGHT,
    CAM_SCENE_MODE_FIREWORKS,
    CAM_SCENE_MODE_PARTY,
    CAM_SCENE_MODE_NIGHT_PORTRAIT,
    CAM_SCENE_MODE_THEATRE,
    CAM_SCENE_MODE_ACTION,
    CAM_SCENE_MODE_AR,
    CAM_SCENE_MODE_FACE_PRIORITY,
    CAM_SCENE_MODE_BARCODE,
    CAM_SCENE_MODE_HDR,
    CAM_SCENE_MODE_MAX
} cam_scene_mode_type;

typedef enum {
    CAM_EFFECT_MODE_OFF,
    CAM_EFFECT_MODE_MONO,
    CAM_EFFECT_MODE_NEGATIVE,
    CAM_EFFECT_MODE_SOLARIZE,
    CAM_EFFECT_MODE_SEPIA,
    CAM_EFFECT_MODE_POSTERIZE,
    CAM_EFFECT_MODE_WHITEBOARD,
    CAM_EFFECT_MODE_BLACKBOARD,
    CAM_EFFECT_MODE_AQUA,
    CAM_EFFECT_MODE_EMBOSS,
    CAM_EFFECT_MODE_SKETCH,
    CAM_EFFECT_MODE_NEON,
    CAM_EFFECT_MODE_MAX
} cam_effect_mode_type;
#endif
static uint32_t sensor_supported_scene_mode =
  1 << CAM_SCENE_MODE_OFF |
  0 << CAM_SCENE_MODE_AUTO |
  1 << CAM_SCENE_MODE_LANDSCAPE |
  0 << CAM_SCENE_MODE_SNOW |
  0 << CAM_SCENE_MODE_BEACH |
  0 << CAM_SCENE_MODE_SUNSET |
  1 << CAM_SCENE_MODE_NIGHT |
  1 << CAM_SCENE_MODE_PORTRAIT |
  0 << CAM_SCENE_MODE_BACKLIGHT |
  0 << CAM_SCENE_MODE_SPORTS |
  0 << CAM_SCENE_MODE_ANTISHAKE |
  0 << CAM_SCENE_MODE_FLOWERS |
  0 << CAM_SCENE_MODE_CANDLELIGHT |
  0 << CAM_SCENE_MODE_FIREWORKS |
  0 << CAM_SCENE_MODE_PARTY |
  0 << CAM_SCENE_MODE_NIGHT_PORTRAIT |
  0 << CAM_SCENE_MODE_THEATRE |
  0 << CAM_SCENE_MODE_ACTION |
  0 << CAM_SCENE_MODE_AR |
  0 >> CAM_SCENE_MODE_FACE_PRIORITY |
  0 >> CAM_SCENE_MODE_BARCODE;

static uint32_t sensor_supported_effect_mode =
  1 << CAM_EFFECT_MODE_OFF |
  1 << CAM_EFFECT_MODE_MONO |
  1 << CAM_EFFECT_MODE_NEGATIVE |
  1 << CAM_EFFECT_MODE_SOLARIZE |
  1 << CAM_EFFECT_MODE_SEPIA |
  0 << CAM_EFFECT_MODE_POSTERIZE |
  0  << CAM_EFFECT_MODE_WHITEBOARD |
  0 << CAM_EFFECT_MODE_BLACKBOARD |
  0 << CAM_EFFECT_MODE_AQUA |
  0 << CAM_EFFECT_MODE_EMBOSS |
  0 << CAM_EFFECT_MODE_SKETCH |
  0 << CAM_EFFECT_MODE_NEON;
/*yuzhaohua gc2035 change 2014.1.28 end */

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
  .csi_cid_params = gc2035_cid_cfg,
  /* csi csid params array size */
  .csi_cid_params_size = ARRAY_SIZE(gc2035_cid_cfg),
  /* resolution cfg table */
  .sensor_res_cfg_table = &gc2035_res_table,
  /* out info array */
  .out_info_array = &out_info_array,
  /* crop params array */
  .crop_params_array = &crop_params_array,
  /* csi params array */
  .csi_params_array = &csi_params_array,
  /* sensor port info array */
  .sensor_stream_info_array = &gc2035_stream_info_array,
  /* sensor supported scene mode, optional */
 .sensor_supported_scene_mode = &sensor_supported_scene_mode,
  /* sensor supported effect mode, optional */
  .sensor_supported_effect_mode = &sensor_supported_effect_mode,
    /* scale size table count*/
  .scale_tbl_cnt = ARRAY_SIZE(gc2035_scale_size_tbl),
  /*function to get scale size tbl */
  .get_scale_tbl = gc2035_get_scale_tbl,
};


void *gc2035f_open_lib(void)
{
  return &sensor_lib_ptr;
}

