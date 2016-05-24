/*============================================================================

  Copyright (c) 2014 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

============================================================================*/
#include <stdio.h>
#include "sensor_lib.h"

#define FEATURE_CAPTURE_PREVIEW_RAW_IMAGE       0

#define SENSOR_MODEL_NO_IMX179_SUNNY_P8N10F "imx179_sunny_p8n10f"
#define IMX179_SUNNY_P8N10F_LOAD_CHROMATIX(n) \
  "libchromatix_"SENSOR_MODEL_NO_IMX179_SUNNY_P8N10F"_"#n".so"


static sensor_lib_t sensor_lib_ptr;

static sensor_output_t sensor_output = {
  .output_format = SENSOR_BAYER,
  .connection_mode = SENSOR_MIPI_CSI,
  .raw_output = SENSOR_10_BIT_DIRECT,
};

static struct msm_sensor_output_reg_addr_t output_reg_addr = {
  .x_output = 0x034c,
  .y_output = 0x034e,
  .line_length_pclk = 0x0342,
  .frame_length_lines = 0x0340,
};

static struct msm_sensor_exp_gain_info_t exp_gain_info = {
  .coarse_int_time_addr = 0x0202,
  .global_gain_addr = 0x0205,
  .vert_offset = 4,
};

static sensor_aec_data_t aec_info = {
  .max_gain = 8.0,
  .max_linecount = 30834, /* updated in PLD gating */
};

static sensor_lens_info_t default_lens_info = {
    .focal_length = 4.92,
    .pix_size = 1.4,
    .f_number = 2.65,
    .total_f_dist = 1.97,
    .hor_view_angle = 55.4,
    .ver_view_angle = 42.7,

};

#ifndef VFE_40
static struct csi_lane_params_t csi_lane_params = {
  .csi_lane_assign = 0xE4,
  .csi_lane_mask = 0x3,
  .csi_if = 1,
  .csid_core = {0},
  .csi_phy_sel = 0,
};
#else
static struct csi_lane_params_t csi_lane_params = {
  .csi_lane_assign = 0x4320,
  .csi_lane_mask = 0x7,
  .csi_if = 1,
  .csid_core = {0},
  .csi_phy_sel = 1,
};
#endif

static struct msm_camera_i2c_reg_array init_reg_array[] = {
};

static struct msm_camera_i2c_reg_setting init_reg_setting[] = {
  {
    .reg_setting = init_reg_array,
    .size = ARRAY_SIZE(init_reg_array),
    .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
    .data_type = MSM_CAMERA_I2C_BYTE_DATA,
    .delay = 0,
  },
};

static struct sensor_lib_reg_settings_array init_settings_array = {
  .reg_settings = init_reg_setting,
  .size = 0,
};

static struct msm_camera_i2c_reg_array start_reg_array[] = {
  {0x0100, 0x01},
};

static  struct msm_camera_i2c_reg_setting start_settings = {
  .reg_setting = start_reg_array,
  .size = ARRAY_SIZE(start_reg_array),
  .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 10,
};

static struct msm_camera_i2c_reg_array stop_reg_array[] = {
  {0x0100, 0x00},
};

static struct msm_camera_i2c_reg_setting stop_settings = {
  .reg_setting = stop_reg_array,
  .size = ARRAY_SIZE(stop_reg_array),
  .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 10,
};

static struct msm_camera_i2c_reg_array groupon_reg_array[] = {
  {0x0104, 0x01},
};

static struct msm_camera_i2c_reg_setting groupon_settings = {
  .reg_setting = groupon_reg_array,
  .size = ARRAY_SIZE(groupon_reg_array),
  .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 0,
};

static struct msm_camera_i2c_reg_array groupoff_reg_array[] = {
  {0x0104, 0x0},
};

static struct msm_camera_i2c_reg_setting groupoff_settings = {
  .reg_setting = groupoff_reg_array,
  .size = ARRAY_SIZE(groupoff_reg_array),
  .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 0,
};

static struct msm_camera_csid_vc_cfg imx179_sunny_p8n10f_cid_cfg[] = {
  {0, CSI_RAW10, CSI_DECODE_10BIT},
  {1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params imx179_sunny_p8n10f_csi_params = {
  .csid_params = {
    .lane_cnt = 2,
    .lut_params = {
      .num_cid = ARRAY_SIZE(imx179_sunny_p8n10f_cid_cfg),
      .vc_cfg = {
         &imx179_sunny_p8n10f_cid_cfg[0],
         &imx179_sunny_p8n10f_cid_cfg[1],
      },
    },
  },
  .csiphy_params = {
    .lane_cnt = 2,
    .settle_cnt = 14,
  },
};

static struct msm_camera_csi2_params *csi_params[] = {
  &imx179_sunny_p8n10f_csi_params, /* RES 0*/
  &imx179_sunny_p8n10f_csi_params, /* RES 1*/
};

static struct sensor_lib_csi_params_array csi_params_array = {
  .csi2_params = &csi_params[0],
  .size = ARRAY_SIZE(csi_params),
};


static struct sensor_pix_fmt_info_t imx179_sunny_p8n10f_pix_fmt0_fourcc[] = {
  { V4L2_PIX_FMT_SRGGB10 },
};

static struct sensor_pix_fmt_info_t imx179_sunny_p8n10f_pix_fmt1_fourcc[] = {
  { MSM_V4L2_PIX_FMT_META },
};

static sensor_stream_info_t imx179_sunny_p8n10f_stream_info[] = {
  {1, &imx179_sunny_p8n10f_cid_cfg[0], imx179_sunny_p8n10f_pix_fmt0_fourcc},
  {1, &imx179_sunny_p8n10f_cid_cfg[1], imx179_sunny_p8n10f_pix_fmt1_fourcc},
};

static sensor_stream_info_array_t imx179_sunny_p8n10f_stream_info_array = {
  .sensor_stream_info = imx179_sunny_p8n10f_stream_info,
  .size = ARRAY_SIZE(imx179_sunny_p8n10f_stream_info),
};

static struct msm_camera_i2c_reg_array res0_reg_array[] = {
    {0x0100,0x00},
    {0x0101,0x00},
    {0x0202,0x0D},
    {0x0203,0x5E},
    {0x0301,0x0A},
    {0x0303,0x01},
    {0x0305,0x06},
    {0x0309,0x0A},
    {0x030B,0x01},
    {0x030C,0x00},
    {0x030D,0xDD},
    {0x0340,0x0D},
    {0x0341,0x62},
    {0x0342,0x0D},
    {0x0343,0x70},
    {0x0344,0x00},
    {0x0345,0x08},
    {0x0346,0x00},
    {0x0347,0x08},
    {0x0348,0x0C},
    {0x0349,0xC7},
    {0x034A,0x09},
    {0x034B,0x97},
    {0x034C,0x0C},
    {0x034D,0xC0},
    {0x034E,0x09},
    {0x034F,0x90},
    {0x0383,0x01},
    {0x0387,0x01},
    {0x0390,0x00},
    {0x0401,0x00},
    {0x0405,0x10},
    {0x3020,0x10},
    {0x3041,0x15},
    {0x3042,0x87},
    {0x3089,0x4F},
    {0x3309,0x9A},
    {0x3344,0x6F},
    {0x3345,0x1F},
    {0x3362,0x0A},
    {0x3363,0x0A},
    {0x3364,0x02},
    {0x3368,0x18},
    {0x3369,0x00},
    {0x3370,0x7F},
    {0x3371,0x37},
    {0x3372,0x67},
    {0x3373,0x3F},
    {0x3374,0x3F},
    {0x3375,0x47},
    {0x3376,0xCF},
    {0x3377,0x47},
    {0x33C8,0x00},
    {0x33D4,0x0C},
    {0x33D5,0xC0},
    {0x33D6,0x09},
    {0x33D7,0x90},
    {0x4100,0x0E},
    {0x4108,0x01},
    {0x4109,0x7C},
};
static struct msm_camera_i2c_reg_array res1_reg_array[] = {
    {0x0100,0x00},
    {0x0101,0x00},
    {0x0202,0x06},
    {0x0203,0xAD},
    {0x0301,0x0A},
    {0x0303,0x01},
    {0x0305,0x06},
    {0x0309,0x0A},
    {0x030B,0x01},
    {0x030C,0x00},
    {0x030D,0xDD},
    {0x0340,0x06},
    {0x0341,0xB1},
    {0x0342,0x0D},
    {0x0343,0x70},
    {0x0344,0x00},
    {0x0345,0x08},
    {0x0346,0x00},
    {0x0347,0x08},
    {0x0348,0x0C},
    {0x0349,0xC7},
    {0x034A,0x09},
    {0x034B,0x97},
    {0x034C,0x06},
    {0x034D,0x60},
    {0x034E,0x04},
    {0x034F,0xC8},
    {0x0383,0x01},
    {0x0387,0x01},
    {0x0390,0x01},
    {0x0401,0x00},
    {0x0405,0x10},
    {0x3020,0x10},
    {0x3041,0x15},
    {0x3042,0x87},
    {0x3089,0x4F},
    {0x3309,0x9A},
    {0x3344,0x6F},
    {0x3345,0x1F},
    {0x3362,0x0A},
    {0x3363,0x0A},
    {0x3364,0x02},
    {0x3368,0x18},
    {0x3369,0x00},
    {0x3370,0x7F},
    {0x3371,0x37},
    {0x3372,0x67},
    {0x3373,0x3F},
    {0x3374,0x3F},
    {0x3375,0x47},
    {0x3376,0xCF},
    {0x3377,0x47},
    {0x33C8,0x00},
    {0x33D4,0x06},
    {0x33D5,0x60},
    {0x33D6,0x04},
    {0x33D7,0xC8},
    {0x4100,0x0E},
    {0x4108,0x01},
    {0x4109,0x7C},

};


static struct msm_camera_i2c_reg_setting res_settings[] = {
#if FEATURE_CAPTURE_PREVIEW_RAW_IMAGE
    {
      .reg_setting = res1_reg_array,
      .size = ARRAY_SIZE(res1_reg_array),
      .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
      .data_type = MSM_CAMERA_I2C_BYTE_DATA,
      .delay = 10,
    },
    {
      .reg_setting = res1_reg_array,
      .size = ARRAY_SIZE(res1_reg_array),
      .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
      .data_type = MSM_CAMERA_I2C_BYTE_DATA,
      .delay = 10,
    },
#else
    {
      .reg_setting = res0_reg_array,
      .size = ARRAY_SIZE(res0_reg_array),
      .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
      .data_type = MSM_CAMERA_I2C_BYTE_DATA,
      .delay = 10,
    },
    {
      .reg_setting = res1_reg_array,
      .size = ARRAY_SIZE(res1_reg_array),
      .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
      .data_type = MSM_CAMERA_I2C_BYTE_DATA,
      .delay = 10,
    },
#endif
};

static struct sensor_lib_reg_settings_array res_settings_array = {
  .reg_settings = res_settings,
  .size = ARRAY_SIZE(res_settings),
};

static struct sensor_crop_parms_t crop_params[] = {
  {0, 0, 0, 0}, /* RES 0 */
  {0, 0, 0, 0}, /* RES 1 */
};

static struct sensor_lib_crop_params_array crop_params_array = {
  .crop_params = crop_params,
  .size = ARRAY_SIZE(crop_params),
};

static struct sensor_lib_out_info_t sensor_out_info[] = {
#if FEATURE_CAPTURE_PREVIEW_RAW_IMAGE
  {/* 30 fps qtr size settings */
    .x_output = 1632,
    .y_output = 1224,
    .line_length_pclk = 3440,
    .frame_length_lines = 1713,
    .vt_pixel_clk = 176800000,
    .op_pixel_clk = 178000000,
    .binning_factor = 1,
    .max_fps = 30.0,
    .min_fps = 15.0,
    .mode = SENSOR_DEFAULT_MODE,
  },
  {/* 30 fps qtr size settings */
    .x_output = 1632,
    .y_output = 1224,
    .line_length_pclk = 3440,
    .frame_length_lines = 1713,
    .vt_pixel_clk = 176800000,
    .op_pixel_clk = 178000000,
    .binning_factor = 1,
    .max_fps = 30.0,
    .min_fps = 15.0,
    .mode = SENSOR_DEFAULT_MODE,
  },
#else
  {/* 15 fps full size settings */
    .x_output = 3264,
    .y_output = 2448,
    .line_length_pclk = 3440,
    .frame_length_lines = 3426,
    .vt_pixel_clk = 176800000,
    .op_pixel_clk = 176800000,
    .binning_factor = 0,
    .max_fps = 15.0,
    .min_fps = 7.5,
    .mode = SENSOR_DEFAULT_MODE,
  },
  {/* 30 fps qtr size settings */
    .x_output = 1632,
    .y_output = 1224,
    .line_length_pclk = 3440,
    .frame_length_lines = 1713,
    .vt_pixel_clk = 176800000,
    .op_pixel_clk = 176800000,
    .binning_factor = 1,
    .max_fps = 30.0,
    .min_fps = 15.0,
    .mode = SENSOR_DEFAULT_MODE,
  },
#endif
};

static struct sensor_lib_out_info_array out_info_array = {
  .out_info = sensor_out_info,
  .size = ARRAY_SIZE(sensor_out_info),
};

static sensor_res_cfg_type_t imx179_sunny_p8n10f_res_cfg[] = {
  SENSOR_SET_STOP_STREAM,
  SENSOR_SET_NEW_RESOLUTION, /* set stream config */
  SENSOR_SET_CSIPHY_CFG,
  SENSOR_SET_CSID_CFG,
  SENSOR_LOAD_CHROMATIX, /* set chromatix prt */
  SENSOR_SEND_EVENT, /* send event */
  SENSOR_SET_START_STREAM,
};

static struct sensor_res_cfg_table_t imx179_sunny_p8n10f_res_table = {
  .res_cfg_type = imx179_sunny_p8n10f_res_cfg,
  .size = ARRAY_SIZE(imx179_sunny_p8n10f_res_cfg),
};

static struct sensor_lib_chromatix_t imx179_sunny_p8n10f_chromatix[] = {
  {
    .common_chromatix = IMX179_SUNNY_P8N10F_LOAD_CHROMATIX(common),
    .camera_preview_chromatix = IMX179_SUNNY_P8N10F_LOAD_CHROMATIX(snapshot), /* RES0 */
    .camera_snapshot_chromatix = IMX179_SUNNY_P8N10F_LOAD_CHROMATIX(snapshot), /* RES0 */
    .camcorder_chromatix = IMX179_SUNNY_P8N10F_LOAD_CHROMATIX(default_video), /* RES0 */
    .liveshot_chromatix = IMX179_SUNNY_P8N10F_LOAD_CHROMATIX(liveshot), /* RES0 */
  },
  {
    .common_chromatix = IMX179_SUNNY_P8N10F_LOAD_CHROMATIX(common),
    .camera_preview_chromatix = IMX179_SUNNY_P8N10F_LOAD_CHROMATIX(preview), /* RES1 */
    .camera_snapshot_chromatix = IMX179_SUNNY_P8N10F_LOAD_CHROMATIX(preview), /* RES1 */
    .camcorder_chromatix = IMX179_SUNNY_P8N10F_LOAD_CHROMATIX(default_video), /* RES1 */
    .liveshot_chromatix = IMX179_SUNNY_P8N10F_LOAD_CHROMATIX(liveshot), /* RES1 */
  },
};

static struct sensor_lib_chromatix_array imx179_sunny_p8n10f_lib_chromatix_array = {
  .sensor_lib_chromatix = imx179_sunny_p8n10f_chromatix,
  .size = ARRAY_SIZE(imx179_sunny_p8n10f_chromatix),
};

/*===========================================================================
 * FUNCTION    - imx179_sunny_p8n10f_real_to_register_gain -
 *
 * DESCRIPTION:
 *==========================================================================*/
static uint16_t imx179_sunny_p8n10f_real_to_register_gain(float gain)
{
  uint16_t reg_gain;

  if (gain < 1.0)
    gain = 1.0;

  if (gain > 8.0)
    gain = 8.0;

  reg_gain = (uint16_t)(256.0 - 256.0 / gain);

  return reg_gain;
}

/*===========================================================================
 * FUNCTION    - imx179_sunny_p8n10f_register_to_real_gain -
 *
 * DESCRIPTION:
 *==========================================================================*/
static float imx179_sunny_p8n10f_register_to_real_gain(uint16_t reg_gain)
{
  float gain;

  if (reg_gain > 0x00E0)
    reg_gain = 0x00E0;

  gain = 256.0 /(256.0 - reg_gain);

  return gain;
}

/*===========================================================================
 * FUNCTION    - imx179_sunny_p8n10f_calculate_exposure -
 *
 * DESCRIPTION:
 *==========================================================================*/
static int32_t imx179_sunny_p8n10f_calculate_exposure(float real_gain,
  uint16_t line_count, sensor_exposure_info_t *exp_info)
{
  if (!exp_info) {
    return -1;
  }
  exp_info->reg_gain = imx179_sunny_p8n10f_real_to_register_gain(real_gain);
  float sensor_real_gain = imx179_sunny_p8n10f_register_to_real_gain(exp_info->reg_gain);
  exp_info->digital_gain = real_gain / sensor_real_gain;
  exp_info->line_count = line_count;
  return 0;
}

/*===========================================================================
 * FUNCTION    - imx179_sunny_p8n10f_fill_exposure_array -
 *
 * DESCRIPTION:
 *==========================================================================*/
static int32_t imx179_sunny_p8n10f_fill_exposure_array(uint16_t gain, uint32_t line,
  uint32_t fl_lines, int32_t luma_avg, uint32_t fgain,
  struct msm_camera_i2c_reg_setting* reg_setting)
{
  int32_t rc = 0;
  uint16_t reg_count = 0;
  uint16_t i = 0;
  static uint8_t skip = 0; //skip some frame for video blinking

  if (!reg_setting) {
    return -1;
  }

  if (1 == line) 
  return -1; 

  if (!(++skip % 4)) 
  return -1;

  for (i = 0; i < sensor_lib_ptr.groupon_settings->size; i++) {
    reg_setting->reg_setting[reg_count].reg_addr =
      sensor_lib_ptr.groupon_settings->reg_setting[i].reg_addr;
    reg_setting->reg_setting[reg_count].reg_data =
      sensor_lib_ptr.groupon_settings->reg_setting[i].reg_data;
    reg_count = reg_count + 1;
  }

  reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.output_reg_addr->frame_length_lines;
  reg_setting->reg_setting[reg_count].reg_data = (fl_lines & 0xFF00) >> 8;
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.output_reg_addr->frame_length_lines + 1;
  reg_setting->reg_setting[reg_count].reg_data = (fl_lines & 0xFF);
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.exp_gain_info->coarse_int_time_addr;
  reg_setting->reg_setting[reg_count].reg_data = (line& 0xFF00) >> 8 ;
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.exp_gain_info->coarse_int_time_addr + 1;
  reg_setting->reg_setting[reg_count].reg_data = (line & 0xFF);
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.exp_gain_info->global_gain_addr;
  reg_setting->reg_setting[reg_count].reg_data = (gain & 0xFF);
  reg_count++;

  for (i = 0; i < sensor_lib_ptr.groupoff_settings->size; i++) {
    reg_setting->reg_setting[reg_count].reg_addr =
      sensor_lib_ptr.groupoff_settings->reg_setting[i].reg_addr;
    reg_setting->reg_setting[reg_count].reg_data =
      sensor_lib_ptr.groupoff_settings->reg_setting[i].reg_data;
    reg_count = reg_count + 1;
  }

  reg_setting->size = reg_count;
  reg_setting->addr_type = MSM_CAMERA_I2C_WORD_ADDR;
  reg_setting->data_type = MSM_CAMERA_I2C_BYTE_DATA;
  reg_setting->delay = 0;

  return rc;
}

static sensor_exposure_table_t imx179_sunny_p8n10f_expsoure_tbl = {
  .sensor_calculate_exposure = imx179_sunny_p8n10f_calculate_exposure,
  .sensor_fill_exposure_array = imx179_sunny_p8n10f_fill_exposure_array,
};

static sensor_lib_t sensor_lib_ptr = {
  /* sensor output settings */
  .sensor_output = &sensor_output,
  /* sensor output register address */
  .output_reg_addr = &output_reg_addr,
  /* sensor exposure gain register address */
  .exp_gain_info = &exp_gain_info,
  /* sensor aec info */
  .aec_info = &aec_info,
  /* sensor snapshot exposure wait frames info */
  .snapshot_exp_wait_frames = 1,
  /* number of frames to skip after start stream */
  .sensor_num_frame_skip = 3,
  /* number of frames to skip after start HDR stream */
  .sensor_num_HDR_frame_skip = 2,
  /* sensor exposure table size */
  .exposure_table_size = 7,
  /* sensor lens info */
  .default_lens_info = &default_lens_info,
  /* csi lane params */
  .csi_lane_params = &csi_lane_params,
  /* csi cid params */
  .csi_cid_params = imx179_sunny_p8n10f_cid_cfg,
  /* csi csid params array size */
  .csi_cid_params_size = ARRAY_SIZE(imx179_sunny_p8n10f_cid_cfg),
  /* init settings */
  .init_settings_array = &init_settings_array,
  /* start settings */
  .start_settings = &start_settings,
  /* stop settings */
  .stop_settings = &stop_settings,
  /* group on settings */
  .groupon_settings = &groupon_settings,
  /* group off settings */
  .groupoff_settings = &groupoff_settings,
  /* resolution cfg table */
  .sensor_res_cfg_table = &imx179_sunny_p8n10f_res_table,
  /* res settings */
  .res_settings_array = &res_settings_array,
  /* out info array */
  .out_info_array = &out_info_array,
  /* crop params array */
  .crop_params_array = &crop_params_array,
  /* csi params array */
  .csi_params_array = &csi_params_array,
  /* sensor port info array */
  .sensor_stream_info_array = &imx179_sunny_p8n10f_stream_info_array,
  /* exposure funtion table */
  .exposure_func_table = &imx179_sunny_p8n10f_expsoure_tbl,
  /* chromatix array */
  .chromatix_array = &imx179_sunny_p8n10f_lib_chromatix_array,
};

/*===========================================================================
 * FUNCTION    - imx179_sunny_p8n10f_open_lib -
 *
 * DESCRIPTION:
 *==========================================================================*/
void *imx179_sunny_p8n10f_open_lib(void)
{
  return &sensor_lib_ptr;
}
