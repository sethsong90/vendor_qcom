/*============================================================================

  Copyright (c) 2014 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

============================================================================*/

#include <stdio.h>
#include "sensor_lib.h"

#define SENSOR_MODEL_NO_IMX188_LITEON_13P "imx188_liteon_13p"
#define IMX188_LITEON_13P_LOAD_CHROMATIX(n) \
  "libchromatix_"SENSOR_MODEL_NO_IMX188_LITEON_13P"_"#n".so"

static sensor_lib_t sensor_lib_ptr;

static sensor_output_t sensor_output = {
  .output_format = SENSOR_BAYER,
  .connection_mode = SENSOR_MIPI_CSI,
  .raw_output = SENSOR_10_BIT_DIRECT,
};

static struct msm_sensor_output_reg_addr_t output_reg_addr = {
  .x_output = 0x034C,
  .y_output = 0x034E,
  .line_length_pclk = 0x0342,
  .frame_length_lines = 0x0340,
};

static struct msm_sensor_exp_gain_info_t exp_gain_info = {
  .coarse_int_time_addr = 0x0202,
  .global_gain_addr = 0x0205,
  .vert_offset = 5,
};

static sensor_aec_data_t aec_info = {
  .max_gain = 14.0,
  .max_linecount = 57888,
};

static sensor_lens_info_t default_lens_info = {
  .focal_length = 1.15,
  .pix_size = 1.4,
  .f_number = 2.6,
  .total_f_dist = 1.5,
  .hor_view_angle = 54.8,
  .ver_view_angle = 42.5,
};

static struct csi_lane_params_t csi_lane_params = {
  .csi_lane_assign = 0x4320,
  .csi_lane_mask = 0x3,
  .csi_if = 1,
  .csid_core = {0},
  .csi_phy_sel = 1,
};

static struct msm_camera_i2c_reg_array init_reg_array[] = {
  {0x309A,  0xA3},
  {0x309E,  0x00},
  {0x3166,  0x1C},
  {0x3167,  0x1B},
  {0x3168,  0x32},
  {0x3169,  0x31},
  {0x316A,  0x1C},
  {0x316B,  0x1B},
  {0x316C,  0x32},
  {0x316D,  0x31},
  {0x0305,  0x04},
  {0x0307,  0x87}, //0x87
  {0x303C,  0x4B},
  {0x30A4,  0x02},
  {0x0112,  0x0A},
  {0x0113,  0x0A},
  {0x0340,  0x03},
  {0x0341,  0x84},
  {0x0342,  0x0B}, //0x0B
  {0x0343,  0xB8}, //0xB8
  {0x0344,  0x00},
  {0x0345,  0x10},
  {0x0346,  0x00},
  {0x0347,  0x30},
  {0x0348,  0x05},
  {0x0349,  0x25},
  {0x034A,  0x03},
  {0x034B,  0x0F},
  {0x034C,  0x05},
  {0x034D,  0x00},
  {0x034E,  0x02},
  {0x034F,  0xD0},
  {0x0381,  0x01},
  {0x0383,  0x01},
  {0x0385,  0x01},
  {0x0387,  0x01},
  {0x3040,  0x08},
  {0x3041,  0x97},
  {0x3048,  0x00},
  {0x304E,  0x0A},
  {0x3050,  0x02},
  {0x309B,  0x00},
  {0x30D5,  0x00},
  {0x31A1,  0x00},
  {0x31B0,  0x00},
  {0x3318,  0x61},
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
  .size = 1,
};

static struct msm_camera_i2c_reg_array start_reg_array[] = {
  {0x0100, 0x01},
};

static  struct msm_camera_i2c_reg_setting start_settings = {
  .reg_setting = start_reg_array,
  .size = ARRAY_SIZE(start_reg_array),
  .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 0,
};

static struct msm_camera_i2c_reg_array stop_reg_array[] = {
  {0x0100, 0x00},
};

static struct msm_camera_i2c_reg_setting stop_settings = {
  .reg_setting = stop_reg_array,
  .size = ARRAY_SIZE(stop_reg_array),
  .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 0,
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
  {0x0104, 0x00},
};

static struct msm_camera_i2c_reg_setting groupoff_settings = {
  .reg_setting = groupoff_reg_array,
  .size = ARRAY_SIZE(groupoff_reg_array),
  .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 0,
};

static struct msm_camera_csid_vc_cfg imx188_liteon_13p_cid_cfg[] = {
  {0, CSI_RAW10, CSI_DECODE_10BIT},
  {1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params imx188_liteon_13p_csi_params = {
  .csid_params = {
    .lane_cnt = 1,
    .lut_params = {
      .num_cid = 2,
      .vc_cfg = {
        &imx188_liteon_13p_cid_cfg[0],
        &imx188_liteon_13p_cid_cfg[1],
      },
    },
  },
  .csiphy_params = {
    .lane_cnt = 1,
    .settle_cnt = 0x1B,
  },
};

static struct sensor_pix_fmt_info_t imx188_liteon_13p_pix_fmt0_fourcc[] = {
  { V4L2_PIX_FMT_SRGGB10 },
};

static struct sensor_pix_fmt_info_t imx188_liteon_13p_pix_fmt1_fourcc[] = {
  { MSM_V4L2_PIX_FMT_META },
};

static sensor_stream_info_t imx188_liteon_13p_stream_info[] = {
  {1, &imx188_liteon_13p_cid_cfg[0], imx188_liteon_13p_pix_fmt0_fourcc},
  {1, &imx188_liteon_13p_cid_cfg[1], imx188_liteon_13p_pix_fmt1_fourcc},
};

static sensor_stream_info_array_t imx188_liteon_13p_stream_info_array = {
  .sensor_stream_info = imx188_liteon_13p_stream_info,
  .size = ARRAY_SIZE(imx188_liteon_13p_stream_info),
};

static struct msm_camera_i2c_reg_array res0_reg_array[] = {
  /* full size */
  {0x0202, 0x02},
  {0x0203, 0xf0},
};

static struct msm_camera_i2c_reg_setting res_settings[] = {
  {
    .reg_setting = res0_reg_array,
    .size = ARRAY_SIZE(res0_reg_array),
    .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
    .data_type = MSM_CAMERA_I2C_BYTE_DATA,
    .delay = 0,
  },
};

static struct sensor_lib_reg_settings_array res_settings_array = {
  .reg_settings = res_settings,
  .size = ARRAY_SIZE(res_settings),
};

static struct msm_camera_csi2_params *csi_params[] = {
  &imx188_liteon_13p_csi_params, /* RES 0*/
};

static struct sensor_lib_csi_params_array csi_params_array = {
  .csi2_params = &csi_params[0],
  .size = ARRAY_SIZE(csi_params),
};

static struct sensor_crop_parms_t crop_params[] = {
  {0, 0, 0, 0}, /* RES 0 */
};

static struct sensor_lib_crop_params_array crop_params_array = {
  .crop_params = crop_params,
  .size = ARRAY_SIZE(crop_params),
};

static struct sensor_lib_out_info_t sensor_out_info[] = {
  {
    /* full size */
    .x_output = 1280,
    .y_output = 720,
    .line_length_pclk = 3000,
    .frame_length_lines = 900,
    .vt_pixel_clk = 81000000,
    .op_pixel_clk = 81000000,
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

static sensor_res_cfg_type_t imx188_liteon_13p_res_cfg[] = {
  SENSOR_SET_STOP_STREAM,
  SENSOR_SET_NEW_RESOLUTION, /* set stream config */
  SENSOR_SET_CSIPHY_CFG,
  SENSOR_SET_CSID_CFG,
  SENSOR_LOAD_CHROMATIX, /* set chromatix prt */
  SENSOR_SEND_EVENT, /* send event */
  SENSOR_SET_START_STREAM,
};

static struct sensor_res_cfg_table_t imx188_liteon_13p_res_table = {
  .res_cfg_type = imx188_liteon_13p_res_cfg,
  .size = ARRAY_SIZE(imx188_liteon_13p_res_cfg),
};

static struct sensor_lib_chromatix_t imx188_liteon_13p_chromatix[] = {
  {
    .common_chromatix = IMX188_LITEON_13P_LOAD_CHROMATIX(common),
    .camera_preview_chromatix = IMX188_LITEON_13P_LOAD_CHROMATIX(preview), /* RES0 */
    .camera_snapshot_chromatix = IMX188_LITEON_13P_LOAD_CHROMATIX(preview), /* RES0 */
    .camcorder_chromatix = IMX188_LITEON_13P_LOAD_CHROMATIX(default_video), /* RES0 */
  },
};

static struct sensor_lib_chromatix_array imx188_liteon_13p_lib_chromatix_array = {
  .sensor_lib_chromatix = imx188_liteon_13p_chromatix,
  .size = ARRAY_SIZE(imx188_liteon_13p_chromatix),
};

/*===========================================================================
 * FUNCTION    - imx188_liteon_13p_real_to_register_gain -
 *
 * DESCRIPTION:
 *==========================================================================*/
static uint16_t imx188_liteon_13p_real_to_register_gain(float gain)
{
  uint16_t reg_gain = 0;
  uint16_t ang_gain = 0;
  float dig_gain = 0;
  uint16_t dig_upper = 0;
  uint16_t dig_lower = 0;

  if(gain <= 8.0)
  {
    if (gain < 1.0)
      gain = 1.0;
    ang_gain = (256.0 - 256.0 / gain);
    reg_gain = ang_gain;
  }
  else
  {
    if(gain > 14.0)
      gain = 14.0;
    dig_gain = gain / 8.0f;
    if(dig_gain >= 1.0 && dig_gain < 2.0)
      dig_upper = 1;
    if(dig_gain >= 2.0 && dig_gain < 3.0)
      dig_upper = 2;

    dig_lower = 256 * (dig_gain - dig_upper);
    reg_gain = ((dig_upper & 0xFF) << 8) + (dig_lower & 0xFF);
  }

  return reg_gain;
}

/*===========================================================================
 * FUNCTION    - imx188_liteon_13p_register_to_real_gain -
 *
 * DESCRIPTION:
 *==========================================================================*/
static float imx188_liteon_13p_register_to_real_gain(uint16_t reg_gain)
{
  float gain = 0.0f;
  uint16_t ang_gain = 0;
  uint16_t dig_upper = 0;
  uint16_t dig_lower = 0;
  float dig_gain = 0.0f;

  if (reg_gain <= 0xE0)
  {
    ang_gain = reg_gain & 0xFFFF;
    gain = 256.0 / (256.0 - ang_gain);
  }
  else
  {
    dig_upper = (reg_gain & 0xFF00) >> 8;
    dig_lower = reg_gain & 0xFF;
    dig_gain = (1.0f * dig_upper) + ((1.0f * dig_lower)/256);
    gain = 8.0f * dig_gain;
  }

  return gain;
}

/*===========================================================================
 * FUNCTION    - imx188_liteon_13p_calculate_exposure -
 *
 * DESCRIPTION:
 *==========================================================================*/
static int32_t imx188_liteon_13p_calculate_exposure(float real_gain,
  uint16_t line_count, sensor_exposure_info_t *exp_info)
{
  if (!exp_info) {
    return -1;
  }

  exp_info->reg_gain = imx188_liteon_13p_real_to_register_gain(real_gain);
  float sensor_real_gain = imx188_liteon_13p_register_to_real_gain(exp_info->reg_gain);
  exp_info->digital_gain = real_gain / sensor_real_gain;
  exp_info->line_count = line_count;

  return 0;
}

/*===========================================================================
 * FUNCTION    - imx188_liteon_13p_fill_exposure_array -
 *
 * DESCRIPTION:
 *==========================================================================*/
static int32_t imx188_liteon_13p_fill_exposure_array(uint16_t gain, uint32_t line,
  uint32_t fl_lines, int32_t luma_avg, uint32_t fgain,
  struct msm_camera_i2c_reg_setting* reg_setting)
{
  uint16_t ang_gain = 0;
  uint16_t dig_upper = 0;
  uint16_t dig_lower = 0;
  int32_t rc = 0;
  uint16_t reg_count = 0;
  uint16_t i = 0;

  if (gain <= 0xE0)
  {
    ang_gain = (gain & 0xFFFF);
    dig_upper = 0x01;
    dig_lower = 0x00;
  }
  else
  {
    dig_upper = (gain & 0xFF00) >> 8;
    dig_lower = gain & 0xFF;
    ang_gain = 0xE0;
  }

  if (!reg_setting) {
    return -1;
  }

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
  reg_setting->reg_setting[reg_count].reg_data = (line & 0xFF00) >> 8;
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.exp_gain_info->coarse_int_time_addr + 1;
  reg_setting->reg_setting[reg_count].reg_data = (line & 0xFF);
  reg_count++;


  reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.exp_gain_info->global_gain_addr;
  reg_setting->reg_setting[reg_count].reg_data = (ang_gain & 0xFF);
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr = 0x020E;
  reg_setting->reg_setting[reg_count].reg_data = (dig_upper & 0xFF);
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr = 0x020F;
  reg_setting->reg_setting[reg_count].reg_data = (dig_lower & 0xFF);
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr = 0x0210;
  reg_setting->reg_setting[reg_count].reg_data = (dig_upper & 0xFF);
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr = 0x0211;
  reg_setting->reg_setting[reg_count].reg_data = (dig_lower & 0xFF);
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr = 0x0212;
  reg_setting->reg_setting[reg_count].reg_data = (dig_upper & 0xFF);
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr = 0x0213;
  reg_setting->reg_setting[reg_count].reg_data = (dig_lower & 0xFF);
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr = 0x0214;
  reg_setting->reg_setting[reg_count].reg_data = (dig_upper & 0xFF);
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr = 0x0215;
  reg_setting->reg_setting[reg_count].reg_data = (dig_lower & 0xFF);
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

static sensor_exposure_table_t imx188_liteon_13p_expsoure_tbl = {
  .sensor_calculate_exposure = imx188_liteon_13p_calculate_exposure,
  .sensor_fill_exposure_array = imx188_liteon_13p_fill_exposure_array,
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
  .sensor_num_frame_skip = 1,
  /* sensor exposure table size */
  .exposure_table_size = 15,
  /* sensor lens info */
  .default_lens_info = &default_lens_info,
  /* csi lane params */
  .csi_lane_params = &csi_lane_params,
  /* csi cid params */
  .csi_cid_params = imx188_liteon_13p_cid_cfg,
  /* csi csid params array size */
  .csi_cid_params_size = ARRAY_SIZE(imx188_liteon_13p_cid_cfg),
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
  .sensor_res_cfg_table = &imx188_liteon_13p_res_table,
  /* res settings */
  .res_settings_array = &res_settings_array,
  /* out info array */
  .out_info_array = &out_info_array,
  /* crop params array */
  .crop_params_array = &crop_params_array,
  /* csi params array */
  .csi_params_array = &csi_params_array,
  /* sensor port info array */
  .sensor_stream_info_array = &imx188_liteon_13p_stream_info_array,
  /* exposure funtion table */
  .exposure_func_table = &imx188_liteon_13p_expsoure_tbl,
  /* chromatix array */
  .chromatix_array = &imx188_liteon_13p_lib_chromatix_array,
};

/*===========================================================================
 * FUNCTION    - imx188_liteon_13p_open_lib -
 *
 * DESCRIPTION:
 *==========================================================================*/
void *imx188_liteon_13p_open_lib(void)
{
  return &sensor_lib_ptr;
}
