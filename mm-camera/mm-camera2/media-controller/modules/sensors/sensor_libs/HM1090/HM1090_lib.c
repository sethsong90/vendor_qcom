/*============================================================================

  Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

============================================================================*/
#include <stdio.h>
#include "sensor_lib.h"
//#include <sensor_util.h>
#include "camera_dbg.h"

#define SENSOR_MODEL_NO_HM1090 "HM1090"
#define HM1090_LOAD_CHROMATIX(n) \
  "libchromatix_"SENSOR_MODEL_NO_HM1090"_"#n".so"


static sensor_lib_t sensor_lib_ptr;

static sensor_output_t sensor_output = {
  .output_format = SENSOR_BAYER,
  .connection_mode = SENSOR_MIPI_CSI,
  .raw_output = SENSOR_10_BIT_DIRECT,
};

static struct msm_sensor_output_reg_addr_t output_reg_addr = {
  .x_output = 0xff,
  .y_output = 0xff,
  .line_length_pclk = 0xff,
  .frame_length_lines = 0xff,
};

static struct msm_sensor_exp_gain_info_t exp_gain_info = {
  .coarse_int_time_addr = 0x0015,
  .global_gain_addr = 0x0018,
  .vert_offset = 4,
};

static sensor_aec_data_t aec_info = {
  .max_gain = 8,  //real gain
  .max_linecount = 3696,  //7.14fps
};

static sensor_lens_info_t default_lens_info = {
  .focal_length = 2.93,
  .pix_size = 2.5,
  .f_number = 2.8,
  .total_f_dist = 1.2,
  .hor_view_angle = 54.8,
  .ver_view_angle = 42.5,
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
  .csi_lane_mask = 0x3,
  .csi_if = 1,
  .csid_core = {0},
  .csi_phy_sel = 1,
};
#endif

static struct msm_camera_i2c_reg_array init_reg_array0[] = {
{0x0022,0x00},
{0x0006,0x03},
{0x000F,0x10},
{0x0018,0x00},
{0x001D,0x40},
{0x0025,0x00},
{0x0026,0x07},
{0x007D,0x16},
{0x002A,0x07},
{0x002B,0x1F},//1F->3B->1F
{0x002C,0x05},
{0x0040,0x10},
{0x0044,0x07},
{0x0045,0x24},
{0x0046,0x01},
{0x004A,0x00},
{0x004E,0xFF},
{0x004F,0x02},
{0x0060,0x01},
{0x0061,0x02},
{0x0062,0x26},
{0x0064,0x2a},
{0x0065,0x20},
{0x0069,0x01},
{0x006a,0x03},
{0x006b,0x20},
{0x0070,0x5a},
{0x0071,0x87},
{0x0072,0x33},
{0x0073,0x53},
{0x0078,0x80},
{0x0079,0x11},
{0x007c,0x15},
{0x007D,0x16},
{0x007E,0x3C},
{0x0081,0x76},
{0x0080,0xE0},
{0x0082,0xB2},
{0x0083,0x98},
{0x0084,0x60},
{0x0085,0x3E},
{0x0086,0x37},
{0x0087,0x02},
{0x0089,0x2A},
{0x008E,0xC0},
{0x008F,0x01},
{0x0093,0x00},
{0x0094,0x00},
{0x0095,0x00},
{0x0096,0x00},
{0x0097,0x00},
{0x0098,0x00},
{0x0099,0x00},
{0x009A,0x00},
{0x009B,0x55},
{0x009C,0x78},
{0x0024,0x40},
{0x0027,0x23},
{0x0074,0x13},
{0x0131,0x01},
{0x0132,0x10},
{0x0b01,0x2B},
{0x0b08,0xdc},
{0x0b09,0x02},
{0x0b0a,0x10},
{0x0b0b,0x05},
{0x0b04,0x04},
{0x0b40,0x00},
{0x0b20,0xbe},
{0x0067,0x10},
{0x0028,0x00},
{0x0015,0x03},
{0x0013,0x02},//01->02
{0x0011,0x02},//03->02
{0x0012,0x02},//01->E0->02
{0x0122,0x01},
{0x0134,0x83},
{0x0135,0x00},
{0x0136,0x00},
{0x0137,0x83},
{0x012B,0x37},
{0x012C,0x00},
{0x0140,0x14},
{0x0141,0x14},
{0x0142,0x80},
{0x0143,0x0A},
{0x0144,0x0A},
{0x0145,0xC8},
{0x0146,0x96},
{0x0147,0x64},
{0x0148,0x32},
{0x0149,0x14},
{0x014A,0x14},
{0x014B,0x14},
{0x014C,0x08},
{0x014D,0x0F},
{0x014E,0x0F},
{0x014F,0x64},
{0x0150,0x20},
{0x0151,0xA2},
{0x0152,0x08},
{0x0153,0x02},
{0x0154,0x11},
{0x0155,0x10},
{0x0156,0x01},
{0x0157,0x07},
{0x0158,0x20},
{0x0159,0x10},
{0x015A,0x08},
{0x015B,0x20},
{0x015C,0x10},
{0x015D,0x08},
{0x0160,0x00},
{0x0161,0x1E},
{0x0162,0x14},
{0x0163,0x0A},
{0x0164,0x80},
{0x0165,0x08},
{0x0166,0x08},
{0x0167,0x08},
{0x0168,0xBB},
{0x0169,0x05},
{0x016A,0x15},
{0x016B,0x96},
{0x016C,0x96},
{0x0220,0x00},
{0x0221,0x00},
{0x0222,0x14},
{0x0223,0x00},
{0x0224,0x80},
{0x0225,0x18},
{0x0226,0x00},
{0x0227,0x00},
{0x0228,0x00},
{0x0229,0x18},
{0x022A,0x00},
{0x022B,0x80},
{0x022C,0x1C},
{0x022D,0x00},
{0x022E,0x00},
{0x022F,0x00},
{0x0230,0x20},
{0x0231,0x00},
{0x0232,0x80},
{0x0233,0x30},
{0x0234,0x00},
{0x0235,0x11},
{0x0236,0x10},
{0x0237,0x0A},
{0x0238,0x10},
{0x0239,0x0A},
{0x023A,0x11},
{0x023B,0x10},
{0x023C,0x0A},
{0x023D,0x10},
{0x023E,0x0A},
{0x023F,0x11},
{0x0240,0x10},
{0x0241,0x0A},
{0x0242,0x10},
{0x0243,0x0A},
{0x0244,0x8C},
{0x0245,0x02},
{0x0246,0x6C},
{0x0247,0x01},
{0x0248,0x8C},
{0x0249,0x02},
{0x024A,0x6C},
{0x024B,0x01},
{0x024C,0x8C},
{0x024D,0x02},
{0x024E,0x6C},
{0x024F,0x01},
{0x0250,0x8C},
{0x0251,0x02},
{0x0252,0x6C},
{0x0253,0x01},
{0x0254,0x00},
{0x0256,0x0C},
{0x0900,0x00},
{0x0901,0x24},
{0x0902,0xF2},
{0x098E,0x08},
{0x098F,0x03},
{0x0990,0x02},
{0x0991,0x03},
{0x0992,0x6F},
{0x0993,0x02},
{0x0994,0x16},
{0x0995,0x03},
{0x09A0,0x1B},
{0x0903,0x4F},
{0x0904,0x36},
{0x0905,0x01},
{0x0906,0x00},
{0x0907,0xFD},
{0x0908,0x03},
{0x0909,0xED},
{0x090A,0x03},
{0x090B,0x19},
{0x090C,0x00},
{0x090D,0x93},
{0x090E,0x00},
{0x090F,0x00},
{0x0910,0x00},
{0x0911,0xF8},
{0x0912,0x03},
{0x0913,0xCE},
{0x0914,0x03},
{0x0915,0x0E},
{0x0916,0x00},
{0x0917,0x0B},
{0x0918,0x00},
{0x0919,0x0A},
{0x091A,0x00},
{0x091B,0x0D},
{0x091C,0x00},
{0x091D,0xFE},
{0x091E,0x03},
{0x091F,0x00},
{0x0920,0x00},
{0x0921,0xE3},
{0x0922,0x03},
{0x0923,0x00},
{0x0924,0x00},
{0x0925,0xFD},
{0x0926,0x03},
{0x0927,0xF1},
{0x0928,0x03},
{0x0929,0x14},
{0x092A,0x00},
{0x092B,0x8B},
{0x092C,0x00},
{0x092D,0x01},
{0x092E,0x00},
{0x092F,0xF8},
{0x0930,0x03},
{0x0931,0xC9},
{0x0932,0x03},
{0x0933,0x0E},
{0x0934,0x00},
{0x0935,0x09},
{0x0936,0x00},
{0x0937,0x0D},
{0x0938,0x00},
{0x0939,0x1B},
{0x093A,0x00},
{0x093B,0xFE},
{0x093C,0x03},
{0x093D,0x00},
{0x093E,0x00},
{0x093F,0xE7},
{0x0940,0x03},
{0x0941,0x08},
{0x0942,0x00},
{0x0943,0xFE},
{0x0944,0x03},
{0x0945,0xF0},
{0x0946,0x03},
{0x0947,0x1D},
{0x0948,0x00},
{0x0949,0x40},
{0x094A,0x00},
{0x094B,0x03},
{0x094C,0x00},
{0x094D,0xF7},
{0x094E,0x03},
{0x094F,0x0A},
{0x0950,0x00},
{0x0951,0x0B},
{0x0952,0x00},
{0x0953,0x0E},
{0x0954,0x00},
{0x0955,0x0A},
{0x0956,0x00},
{0x0957,0x1B},
{0x0958,0x01},
{0x0959,0xFC},
{0x095A,0x03},
{0x095B,0x0A},
{0x095C,0x00},
{0x095D,0xE0},
{0x095E,0x03},
{0x095F,0x00},
{0x0960,0x00},
{0x0961,0xFC},
{0x0962,0x03},
{0x0963,0xF4},
{0x0964,0x03},
{0x0965,0x14},
{0x0966,0x00},
{0x0967,0x7C},
{0x0968,0x00},
{0x0969,0x02},
{0x096A,0x00},
{0x096B,0xF8},
{0x096C,0x03},
{0x096D,0xDA},
{0x096E,0x03},
{0x096F,0x0D},
{0x0970,0x00},
{0x0971,0x0C},
{0x0972,0x00},
{0x0973,0x0E},
{0x0974,0x00},
{0x0975,0x50},
{0x0976,0x00},
{0x0977,0xFD},
{0x0978,0x03},
{0x0979,0x02},
{0x097A,0x00},
{0x097B,0xE5},
{0x097C,0x03},
{0x0049,0xC4},
{0x097D,0xAA},
{0x097E,0x00},
{0x097F,0x88},
{0x0980,0x00},
{0x0981,0x86},
{0x0982,0x00},
{0x0983,0x86},
{0x0984,0x00},
{0x0000,0x01},
{0x0100,0x01},
{0x0101,0x01},
{0x0005,0x01},
};


static struct msm_camera_i2c_reg_setting init_reg_setting[] = {
  {
    .reg_setting = init_reg_array0,
    .size = ARRAY_SIZE(init_reg_array0),
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
 {0x0005,0x01},
};

static  struct msm_camera_i2c_reg_setting start_settings = {
  .reg_setting = start_reg_array,
  .size = ARRAY_SIZE(start_reg_array),
  .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 10,
};

static struct msm_camera_i2c_reg_array stop_reg_array[] = {
  {0x0005,0x00},
};

static struct msm_camera_i2c_reg_setting stop_settings = {
  .reg_setting = stop_reg_array,
  .size = ARRAY_SIZE(stop_reg_array),
  .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 10,
};

static struct msm_camera_i2c_reg_array groupon_reg_array[] = {
};

static struct msm_camera_i2c_reg_setting groupon_settings = {
  .reg_setting = groupon_reg_array,
  .size = ARRAY_SIZE(groupon_reg_array),
  .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 0,
};

static struct msm_camera_i2c_reg_array groupoff_reg_array[] = {
};

static struct msm_camera_i2c_reg_setting groupoff_settings = {
  .reg_setting = groupoff_reg_array,
  .size = ARRAY_SIZE(groupoff_reg_array),
  .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 0,
};

static struct msm_camera_csid_vc_cfg HM1090_cid_cfg[] = {
  {0, CSI_RAW10, CSI_DECODE_10BIT},
  {1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params HM1090_csi_params = {
  .csid_params = {
    .lane_cnt = 1,
    .lut_params = {
      .num_cid = ARRAY_SIZE(HM1090_cid_cfg),
      .vc_cfg = {
         &HM1090_cid_cfg[0],
		 &HM1090_cid_cfg[1],
      },
    },
  },
  .csiphy_params = {
    .lane_cnt = 1,
    .settle_cnt = 0x14,  //0x26  peter
  },
};

static struct sensor_pix_fmt_info_t HM1090_pix_fmt0_fourcc[] = {
  { V4L2_PIX_FMT_SRGGB10 },
};

static struct sensor_pix_fmt_info_t HM1090_pix_fmt1_fourcc[] = {
  { MSM_V4L2_PIX_FMT_META },
};

static sensor_stream_info_t HM1090_stream_info[] = {
  {1, &HM1090_cid_cfg[0], HM1090_pix_fmt0_fourcc},
  {1, &HM1090_cid_cfg[1], HM1090_pix_fmt1_fourcc},
};

static sensor_stream_info_array_t HM1090_stream_info_array = {
  .sensor_stream_info = HM1090_stream_info,
  .size = ARRAY_SIZE(HM1090_stream_info),
};


static struct msm_camera_i2c_reg_array res0_reg_array[] = {
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
  &HM1090_csi_params, /* RES 0*/
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
    .x_output = 1296,
    .y_output = 732,
    .line_length_pclk = 1582,
    .frame_length_lines = 819,  //N*3.33
    .vt_pixel_clk = 19200000,  //preview
    .op_pixel_clk = 19200000,  //96Mhz
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

static sensor_res_cfg_type_t HM1090_res_cfg[] = {
  SENSOR_SET_STOP_STREAM,
  SENSOR_SET_NEW_RESOLUTION, /* set stream config */
  SENSOR_SET_CSIPHY_CFG,
  SENSOR_SET_CSID_CFG,
  SENSOR_LOAD_CHROMATIX, /* set chromatix prt */
  SENSOR_SEND_EVENT, /* send event */
  SENSOR_SET_START_STREAM,
};

static struct sensor_res_cfg_table_t HM1090_res_table = {
  .res_cfg_type = HM1090_res_cfg,
  .size = ARRAY_SIZE(HM1090_res_cfg),
};

static struct sensor_lib_chromatix_t HM1090_chromatix[] = {
  {
    .common_chromatix = HM1090_LOAD_CHROMATIX(common),
    .camera_preview_chromatix = HM1090_LOAD_CHROMATIX(preview), /* RES0 */
    .camera_snapshot_chromatix = HM1090_LOAD_CHROMATIX(preview), /* RES0 */
    .camcorder_chromatix = HM1090_LOAD_CHROMATIX(preview), /* RES0 */
  },
};

static struct sensor_lib_chromatix_array HM1090_lib_chromatix_array = {
  .sensor_lib_chromatix = HM1090_chromatix,
  .size = ARRAY_SIZE(HM1090_chromatix),
};

/*===========================================================================
 * FUNCTION    - HM1090_real_to_register_gain -
 *
 * DESCRIPTION:
 *==========================================================================*/
static uint16_t HM1090_real_to_register_gain(float gain)
{
  uint16_t reg_gain;
  uint16_t gain_tmp0 = 0,Reg_Cgain=0,Reg_Fgain=0;
/*
  if(gain < 1.0) {
      gain = 1.0;
  } else if (gain > 4.0) {
      gain = 4.0;
  }
  gain = (gain) * 64.0;
  reg_gain = (uint16_t) gain - 1;
*/ 
  
    if(gain < 1.0) {
      gain = 1.0;
  } else if (gain > 16.0) {
      gain = 16.0;
  }
  
	//gain_tmp1=(gain % 2)
	
	if(gain < 2)
        {
	   Reg_Cgain=0;
	   Reg_Fgain=(uint16_t)((gain-1) * 16);
        }
	else if(gain < 4)
	{
	   Reg_Cgain=1;
          Reg_Fgain=(uint16_t)(((gain/2)-1) * 16);
	}
	else if(gain < 8)
	{
	Reg_Cgain=2;
	  Reg_Fgain=(uint16_t)(((gain/4)-1) * 16) ;
	}
	else if(gain < 16)
	{
		Reg_Cgain=3;
	  Reg_Fgain=(uint16_t)(((gain/8)-1) * 16);
	}
	else
	{
		Reg_Cgain=3;
	  Reg_Fgain=15;
	}	
	reg_gain = (Reg_Fgain & 0xF)<<4 | (Reg_Cgain & 0xF);
ALOGE("%s:real_gain=%f, Cgain=0x%x,Fgain=0x%x,reg_gain=0x%x",__func__,gain,Reg_Cgain,Reg_Fgain,reg_gain);
  
  

  return reg_gain;
}

/*===========================================================================
 * FUNCTION    - HM1090_register_to_real_gain -
 *
 * DESCRIPTION:
 *==========================================================================*/
static float HM1090_register_to_real_gain(uint16_t reg_gain)
{
  float real_gain;
/*
  if (reg_gain < 0x40) {
      reg_gain = 0x40;
  } else if (reg_gain > 0xff) {
      reg_gain = 0xff;
  }
  real_gain = (float) reg_gain / 64.0;

  return real_gain;
 */
 uint16_t gain_tmp0 =0,Reg_Cgain=0,Reg_Fgain=0,i=0;

	Reg_Fgain=(reg_gain >>4)& 0xF;
	Reg_Cgain=reg_gain&0xF;
	switch (Reg_Cgain)
        {
        case 0:
            real_gain=((float)Reg_Fgain/16 +1)*1;
            break;
        case 1:
            real_gain=((float)Reg_Fgain/16 +1)*2;
            break;
        case 2:
            real_gain=((float)Reg_Fgain/16 +1)*4;
            break;
        case 3:
            real_gain=((float)Reg_Fgain/16 +1)*8;
            break;
        default:
           real_gain=1;
	}

ALOGE("%s: reg_gain=0x%x,Cgain=0x%x,Fgain=0x%x,real_gain=%f",__func__,reg_gain,Reg_Cgain,Reg_Fgain,real_gain);
	return real_gain;
 
}

/*===========================================================================
 * FUNCTION    - HM1090_calculate_exposure -
 *
 * DESCRIPTION:
 *==========================================================================*/
static int32_t HM1090_calculate_exposure(float real_gain,
  uint16_t line_count, sensor_exposure_info_t *exp_info)
{
  if (!exp_info) {
    return -1;
  }
  exp_info->reg_gain = HM1090_real_to_register_gain(real_gain);
  float sensor_real_gain = HM1090_register_to_real_gain(exp_info->reg_gain);
  exp_info->digital_gain = real_gain / sensor_real_gain;
  exp_info->line_count = line_count;
ALOGE("%s: real_gain=%f,sensor_real_gain=%f,reg_gain=0x%x,digital_gain=%f,line_count=%u",__func__,real_gain,sensor_real_gain,exp_info->reg_gain,exp_info->digital_gain,line_count);
  return 0;
}

/*===========================================================================
 * FUNCTION    - HM1090_fill_exposure_array -
 *
 * DESCRIPTION:
 *==========================================================================*/
static int32_t HM1090_fill_exposure_array(uint16_t gain, uint32_t line,
  uint32_t fl_lines, int32_t luma_avg, uint32_t fgain,
  struct msm_camera_i2c_reg_setting *reg_setting)
{
  int32_t rc = 0;
  uint16_t reg_count = 0;
  uint16_t i = 0,Reg_Cgain=0,Reg_Fgain=0;

 // if (line > 2047)
 //   line = 2047;

  if (!reg_setting) {
    return -1;
  }

  for (i = 0; i < sensor_lib_ptr.groupon_settings->size; i++) {
    reg_setting->reg_setting[reg_count].reg_addr =
      sensor_lib_ptr.groupon_settings->reg_setting[i].reg_addr;
    reg_setting->reg_setting[reg_count].reg_data =
      sensor_lib_ptr.groupon_settings->reg_setting[i].reg_data;
    reg_count++;
  }

ALOGE("%s: gain=0x%x",__func__,gain);
  reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.exp_gain_info->global_gain_addr;//Again 0x18
  reg_setting->reg_setting[reg_count].reg_data = gain;
  reg_count++;


  reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.exp_gain_info->coarse_int_time_addr;//Int_H 0x0015
  reg_setting->reg_setting[reg_count].reg_data = (line & 0xFF00) >> 8;
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.exp_gain_info->coarse_int_time_addr + 1;//Int_L 0x0016
  reg_setting->reg_setting[reg_count].reg_data = line & 0xFF;
  reg_count++;

  for (i = 0; i < sensor_lib_ptr.groupoff_settings->size; i++) {
    reg_setting->reg_setting[reg_count].reg_addr =
      sensor_lib_ptr.groupoff_settings->reg_setting[i].reg_addr;
    reg_setting->reg_setting[reg_count].reg_data =
      sensor_lib_ptr.groupoff_settings->reg_setting[i].reg_data;
    reg_count++;
  }

  reg_setting->reg_setting[reg_count].reg_addr = 0x0000;//Dgain 0x1D
  reg_setting->reg_setting[reg_count].reg_data = 0x01 ;
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr = 0x0100;//Dgain 0x1D
  reg_setting->reg_setting[reg_count].reg_data = 0x01 ;
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr = 0x0101;//Dgain 0x1D
  reg_setting->reg_setting[reg_count].reg_data = 0x01 ;
  reg_count++;

  reg_setting->size = reg_count;
  reg_setting->addr_type = MSM_CAMERA_I2C_WORD_ADDR;
  reg_setting->data_type = MSM_CAMERA_I2C_BYTE_DATA;
  reg_setting->delay = 0;

  return rc;
}

static sensor_exposure_table_t HM1090_expsoure_tbl = {
  .sensor_calculate_exposure = HM1090_calculate_exposure,
  .sensor_fill_exposure_array = HM1090_fill_exposure_array,
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
  .exposure_table_size = 5,
  /* sensor lens info */
  .default_lens_info = &default_lens_info,
  /* csi lane params */
  .csi_lane_params = &csi_lane_params,
  /* csi cid params */
  .csi_cid_params = HM1090_cid_cfg,
  /* csi csid params array size */
  .csi_cid_params_size = ARRAY_SIZE(HM1090_cid_cfg),
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
  .sensor_res_cfg_table = &HM1090_res_table,
  /* res settings */
  .res_settings_array = &res_settings_array,
  /* out info array */
  .out_info_array = &out_info_array,
  /* crop params array */
  .crop_params_array = &crop_params_array,
  /* csi params array */
  .csi_params_array = &csi_params_array,
  /* sensor port info array */
  .sensor_stream_info_array = &HM1090_stream_info_array,
  /* exposure funtion table */
  .exposure_func_table = &HM1090_expsoure_tbl,
  /* chromatix array */
  .chromatix_array = &HM1090_lib_chromatix_array,
};

/*===========================================================================
 * FUNCTION    - SKUAA_ST_HM1090_open_lib -
 *
 * DESCRIPTION:
 *==========================================================================*/
void *HM1090_open_lib(void)
{
  return &sensor_lib_ptr;
}
