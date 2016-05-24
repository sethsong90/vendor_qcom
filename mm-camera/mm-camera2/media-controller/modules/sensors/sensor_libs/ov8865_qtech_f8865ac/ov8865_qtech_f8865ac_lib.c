/*============================================================================

  Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

============================================================================*/
#include <stdio.h>
#include "sensor_lib.h"
#include <utils/Log.h>

#define SENSOR_MODEL_NO_OV8865_QTECH "ov8865_qtech_f8865ac"
#define OV8865_QTECH_LOAD_CHROMATIX(n) \
"libchromatix_"SENSOR_MODEL_NO_OV8865_QTECH"_"#n".so"

#define SNAPSHOT_PARM 1
#define PREVIEW_PARM  1

static sensor_lib_t sensor_lib_ptr;

static sensor_output_t sensor_output = {
  .output_format = SENSOR_BAYER,
  .connection_mode = SENSOR_MIPI_CSI,
  .raw_output = SENSOR_10_BIT_DIRECT,
};

static struct msm_sensor_output_reg_addr_t output_reg_addr = {
  .x_output = 0x3808,
  .y_output = 0x380a,
  .line_length_pclk = 0x380c,
  .frame_length_lines = 0x380e,
};

static struct msm_sensor_exp_gain_info_t exp_gain_info = {
  .coarse_int_time_addr = 0x3501,
  .global_gain_addr = 0x3508,
  .vert_offset = 4,
};

static sensor_aec_data_t aec_info = {
  .max_gain = 16.0,
  .max_linecount = 400000,
};

static sensor_lens_info_t default_lens_info = {
  .focal_length = 2.93,
  .pix_size = 1.4,
  .f_number = 2.8,
  .total_f_dist = 1.2,
  .hor_view_angle = 54.8,
  .ver_view_angle = 42.5,
};

#ifndef VFE_40
static struct csi_lane_params_t csi_lane_params = {
  .csi_lane_assign = 0xe4,
  .csi_lane_mask = 0x3,
  .csi_if = 1,
  .csid_core = {0},
};
#else
static struct csi_lane_params_t csi_lane_params = {
  .csi_lane_assign = 0x4320,
  .csi_lane_mask = 0x7,
  .csi_if = 1,
  .csid_core = {0},
};
#endif

static struct msm_camera_i2c_reg_array init_reg_array0[] = {
  {0x0103, 0x01},
};

static struct msm_camera_i2c_reg_array init_reg_array1[] = {
  {0x0100, 0x00}, // software standby
  {0x0100, 0x00},
  {0x0100, 0x00},
  {0x0100, 0x00},

  {0x3638, 0xff},
  {0x0302, 0x1e},
  {0x0303, 0x00}, //0x00
  {0x0304, 0x03},
  {0x030e, 0x00},
  {0x030f, 0x09},
  {0x0312, 0x01},
  {0x031e, 0x0c},
  {0x3015, 0x01},
  {0x3018, 0x72}, //MIPI 4 lane
  {0x3020, 0x93},
  {0x3022, 0x01}, //pd_mini enable when rst_sync
  {0x3031, 0x0a}, //10-bit
  {0x3106, 0x01}, // PLL
  {0x3305, 0xf1},
  {0x3308, 0x00},
  {0x3309, 0x28},
  {0x330a, 0x00},
  {0x330b, 0x20},
  {0x330c, 0x00},
  {0x330d, 0x00},
  {0x330e, 0x00},
  {0x330f, 0x40},
  {0x3307, 0x04},
  {0x3604, 0x04}, // analog control
  {0x3602, 0x30},
  {0x3605, 0x00},
  {0x3607, 0x20},
  {0x3608, 0x11},
  {0x3609, 0x68},
  {0x360a, 0x40},
  {0x360c, 0xdd},
  {0x360e, 0x0c},
  {0x3610, 0x07},
  {0x3612, 0x86},
  {0x3613, 0x58},
  {0x3614, 0x28},
  {0x3617, 0x40},
  {0x3618, 0x5a},
  {0x3619, 0x9b},
  {0x361c, 0x00},

  {0x361d, 0x60},
  {0x3631, 0x60},
  {0x3633, 0x10},
  {0x3634, 0x10},
  {0x3635, 0x10},
  {0x3636, 0x10},
  {0x3641, 0x55}, // MIPI settings
  {0x3646, 0x86}, // MIPI settings
  {0x3647, 0x27}, // MIPI settings
  {0x364a, 0x1b}, // MIPI settings
  {0x3500, 0x00}, // exposurre HH
  {0x3501, 0x4c}, // expouere H
  {0x3502, 0x00}, // exposure L
  {0x3503, 0x00}, // gain no delay, exposure no delay
  {0x3508, 0x02}, // gain H
  {0x3509, 0x00}, // gain L
  {0x3700, 0x24}, // sensor control
  {0x3701, 0x0c},
  {0x3702, 0x28},
  {0x3703, 0x19},
  {0x3704, 0x14},
  {0x3705, 0x00},
  {0x3706, 0x38},
  {0x3707, 0x04},
  {0x3708, 0x24},
  {0x3709, 0x40},
  {0x370a, 0x00},
  {0x370b, 0xb8},
  {0x370c, 0x04},
  {0x3718, 0x12},
  {0x3719, 0x31},
  {0x3712, 0x42},
  {0x3714, 0x12},
  {0x371e, 0x19},
  {0x371f, 0x40},
  {0x3720, 0x05},
  {0x3721, 0x05},
  {0x3724, 0x02},
  {0x3725, 0x02},
  {0x3726, 0x06},
  {0x3728, 0x05},
  {0x3729, 0x02},
  {0x372a, 0x03},
  {0x372b, 0x53},
  {0x372c, 0xa3},
  {0x372d, 0x53},
  {0x372e, 0x06},
  {0x372f, 0x10},

  {0x3730, 0x01},
  {0x3731, 0x06},
  {0x3732, 0x14},
  {0x3733, 0x10},
  {0x3734, 0x40},
  {0x3736, 0x20},
  {0x373a, 0x02},
  {0x373b, 0x0c},
  {0x373c, 0x0a},
  {0x373e, 0x03},
  {0x3755, 0x40},
  {0x3758, 0x00},
  {0x3759, 0x4c},
  {0x375a, 0x06},
  {0x375b, 0x13},
  {0x375c, 0x20},
  {0x375d, 0x02},
  {0x375e, 0x00},
  {0x375f, 0x14},
  {0x3767, 0x04},
  {0x3768, 0x04},
  {0x3769, 0x20},
  {0x376c, 0x00},
  {0x376d, 0x00},
  {0x376a, 0x08},
  {0x3761, 0x00},
  {0x3762, 0x00},
  {0x3763, 0x00},
  {0x3766, 0xff},
  {0x376b, 0x42},
  {0x3772, 0x23},
  {0x3773, 0x02},
  {0x3774, 0x16},
  {0x3775, 0x12},
  {0x3776, 0x08},
  {0x37a0, 0x44},
  {0x37a1, 0x3d},
  {0x37a2, 0x3d},
  {0x37a3, 0x01},
  {0x37a4, 0x00},
  {0x37a5, 0x08},
  {0x37a6, 0x00},
  {0x37a7, 0x44},
  {0x37a8, 0x58},
  {0x37a9, 0x58},
  {0x3760, 0x00},
  {0x376f, 0x01},
  {0x37aa, 0x44},

  {0x37ab, 0x2e},
  {0x37ac, 0x2e},
  {0x37ad, 0x33},
  {0x37ae, 0x0d},
  {0x37af, 0x0d},
  {0x37b0, 0x00},
  {0x37b1, 0x00},
  {0x37b2, 0x00},
  {0x37b3, 0x42},
  {0x37b4, 0x42},
  {0x37b5, 0x33},
  {0x37b6, 0x00},
  {0x37b7, 0x00},
  {0x37b8, 0x00},
  {0x37b9, 0xff}, // sensor control
  {0x3800, 0x00}, // X start H
  {0x3801, 0x0c}, // X start L
  {0x3802, 0x00}, // Y start H
  {0x3803, 0x0c}, // Y start L
  {0x3804, 0x0c}, // X end H
  {0x3805, 0xd3}, // X end L
  {0x3806, 0x09},// Y end H
  {0x3807, 0xa3}, // Y end L
  {0x3808, 0x06}, // X output size H
  {0x3809, 0x60}, // X output size L
  {0x380a, 0x04}, // Y output size H
  {0x380b, 0xc8}, // Y output size L
  {0x380c, 0x07}, // HTS H
  {0x380d, 0x83}, // HTS L
  {0x380e, 0x04}, // VTS H
  {0x380f, 0xe0}, // VTS L
  {0x3810, 0x00}, // ISP X win H
  {0x3811, 0x04}, // ISP X win L
  {0x3813, 0x04}, // ISP Y win L
  {0x3814, 0x03}, // X inc odd
  {0x3815, 0x01}, // X inc even
  {0x3820, 0x00}, // flip off
  {0x3821, 0x67}, // hsync_en_o, fst_vbin, mirror on
  {0x382a, 0x03}, // Y inc odd
  {0x382b, 0x01}, // Y inc even
  {0x3830, 0x08}, // ablc_use_num[5:1]
  {0x3836, 0x02}, // zline_use_num[5:1]
  {0x3837, 0x18}, // vts_add_dis, cexp_gt_vts_offs=8
  {0x3841, 0xff}, // auto size
  {0x3846, 0x88}, // Y/X boundary pixel numbber for auto size mode
  {0x3f08, 0x0b},
  {0x4000, 0xf1}, // our range trig en, format chg en, gan chg en, exp chg en, median en
  {0x4001, 0x14}, // left 32 column, final BLC offset limitation enable

  {0x4005, 0x10}, // BLC target
  {0x400b, 0x0c}, // start line =0, offset limitation en, cut range function en
  {0x400d, 0x10}, // offset trigger threshold
  {0x401b, 0x00},
  {0x401d, 0x00},
  {0x4020, 0x01}, // anchor left start H
  {0x4021, 0x20}, // anchor left start L
  {0x4022, 0x01}, // anchor left end H
  {0x4023, 0x9f}, // anchor left end L
  {0x4024, 0x03}, // anchor right start H
  {0x4025, 0xe0}, // anchor right start L
  {0x4026, 0x04}, // anchor right end H
  {0x4027, 0x5f}, // anchor right end L
  {0x4028, 0x00}, // top zero line start
  {0x4029, 0x02}, // top zero line number
  {0x402a, 0x04}, // top black line start
  {0x402b, 0x04}, // top black line number
  {0x402c, 0x02}, // bottom zero line start
  {0x402d, 0x02}, // bottom zero line number
  {0x402e, 0x08}, // bottom black line start
  {0x402f, 0x02}, // bottom black line number
  {0x401f, 0x00}, // anchor one disable
  {0x4034, 0x3f}, // limitation BLC offset
  {0x4300, 0xff}, // clip max H
  {0x4301, 0x00}, // clip min H
  {0x4302, 0x0f}, // clip min L/clip max L
  {0x4500, 0x40}, // ADC sync control
  {0x4503, 0x10},
  {0x4601, 0x74}, // V FIFO control
  {0x481f, 0x32}, // clk_prepare_min
  {0x4837, 0x16}, //0x16 // clock period
  {0x4850, 0x10}, // lane select
  {0x4851, 0x32}, // lane select
  {0x4b00, 0x2a}, // LVDS settings
  {0x4b0d, 0x00}, // LVDS settings
  {0x4d00, 0x04}, // temperature sensor
  {0x4d01, 0x18}, // temperature sensor
  {0x4d02, 0xc3}, // temperature sensor
  {0x4d03, 0xff}, // temperature sensor
  {0x4d04, 0xff}, //temperature sensor
  {0x4d05, 0xff}, //temperature sensor
  {0x5000, 0x96}, // LENC on, MWB on, BPC on, WPC on
  {0x5001, 0x01}, // BLC on
  {0x5002, 0x08}, // vario pixel off
  {0x5901, 0x00},
  {0x5e00, 0x00}, // test pattern off
  {0x5e01, 0x41}, // window cut enable
  {0x030d, 0x1f}, //0x1e //new add
  {0x0100, 0x01}, // wake up, streaming
  // add 3d85/3d8c/3d85 for otp auto load at power on
  // add 5b00~5b05 for odpc related register control {0x3d85, 0x06},
  {0x3d8c, 0x75},
  {0x3d8d, 0xef},
  {0x5b00, 0x02},
  {0x5b01, 0xd0},
  {0x5b02, 0x03},
  {0x5b03, 0xff},
  {0x5b05, 0x6c},

  {0x5780, 0xfc}, // ; DPC
  {0x5781, 0xdf}, // ;
  {0x5782, 0x3f}, // ;
  {0x5783, 0x08}, // ;
  {0x5784, 0x0c}, // ;
  {0x5786, 0x20}, // ;
  {0x5787, 0x40}, // ;
  {0x5788, 0x08}, // ;
  {0x5789, 0x08}, // ;
  {0x578a, 0x02}, // ;
  {0x578b, 0x01}, // ;
  {0x578c, 0x01}, // ;
  {0x578d, 0x0c}, // ;
  {0x578e, 0x02}, // ;
  {0x578f, 0x01}, // ;
  {0x5790, 0x01}, // ; DPC

  {0x5800, 0x1d},// lens correction
  {0x5801, 0x0e},
  {0x5802, 0x0c},
  {0x5803, 0x0c},
  {0x5804, 0x0f},
  {0x5805, 0x22},
  {0x5806, 0x0a},
  {0x5807, 0x06},
  {0x5808, 0x05},
  {0x5809, 0x05},
  {0x580a, 0x07},
  {0x580b, 0x0a},
  {0x580c, 0x06},
  {0x580d, 0x02},
  {0x580e, 0x00},
  {0x580f, 0x00},
  {0x5810, 0x03},
  {0x5811, 0x07},
  {0x5812, 0x06},
  {0x5813, 0x02},
  {0x5814, 0x00},
  {0x5815, 0x00},
  {0x5816, 0x03},
  {0x5817, 0x07},
  {0x5818, 0x09},
  {0x5819, 0x06},
  {0x581a, 0x04},
  {0x581b, 0x04},
  {0x581c, 0x06},
  {0x581d, 0x0a},
  {0x581e, 0x19},
  {0x581f, 0x0d},
  {0x5820, 0x0b},
  {0x5821, 0x0b},
  {0x5822, 0x0e},
  {0x5823, 0x22},
  {0x5824, 0x23},
  {0x5825, 0x28},
  {0x5826, 0x29},
  {0x5827, 0x27},
  {0x5828, 0x13},
  {0x5829, 0x26},
  {0x582a, 0x33},
  {0x582b, 0x32},
  {0x582c, 0x33},
  {0x582d, 0x16},
  {0x582e, 0x14},

  {0x582f, 0x30},
  {0x5830, 0x31},
  {0x5831, 0x30},
  {0x5832, 0x15},
  {0x5833, 0x26},
  {0x5834, 0x23},
  {0x5835, 0x21},
  {0x5836, 0x23},
  {0x5837, 0x05},
  {0x5838, 0x36},
  {0x5839, 0x27},
  {0x583a, 0x28},
  {0x583b, 0x26},
  {0x583c, 0x24},
  {0x583d, 0xdf}, //lens correction
};

static struct msm_camera_i2c_reg_setting init_reg_setting[] = {
  {
    .reg_setting = init_reg_array0,
    .size = ARRAY_SIZE(init_reg_array0),
    .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
    .data_type = MSM_CAMERA_I2C_BYTE_DATA,
    .delay = 30,
  },
  {
    .reg_setting = init_reg_array1,
    .size = ARRAY_SIZE(init_reg_array1),
    .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
    .data_type = MSM_CAMERA_I2C_BYTE_DATA,
    .delay = 5,
  },
};

static struct sensor_lib_reg_settings_array init_settings_array = {
  .reg_settings = init_reg_setting,
  .size = 2,
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
  {0x3208, 0x00},
};

static struct msm_camera_i2c_reg_setting groupon_settings = {
  .reg_setting = groupon_reg_array,
  .size = ARRAY_SIZE(groupon_reg_array),
  .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 10,
};

static struct msm_camera_i2c_reg_array groupoff_reg_array[] = {
  {0x3208, 0x10},
  {0x3208, 0xA0},
};

static struct msm_camera_i2c_reg_setting groupoff_settings = {
  .reg_setting = groupoff_reg_array,
  .size = ARRAY_SIZE(groupoff_reg_array),
  .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 10,
};

static struct msm_camera_csid_vc_cfg ov8865_qtech_cid_cfg[] = {
  {0, CSI_RAW10, CSI_DECODE_10BIT},
  {1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params ov8865_qtech_csi_params = {
  .csid_params = {
    .lane_cnt = 2,
    .lut_params = {
      .num_cid = 2,
      .vc_cfg = {
         &ov8865_qtech_cid_cfg[0],
         &ov8865_qtech_cid_cfg[1],
      },
    },
  },
  .csiphy_params = {
    .lane_cnt = 2,
    .settle_cnt = 0x12, //1A
  },
};


static struct msm_camera_csi2_params *csi_params[] = {
  #if SNAPSHOT_PARM
  &ov8865_qtech_csi_params, /* RES 0*/
  #endif
  #if PREVIEW_PARM
  &ov8865_qtech_csi_params, /* RES 1*/
  #endif
};

static struct sensor_lib_csi_params_array csi_params_array = {
  .csi2_params = &csi_params[0],
  .size = ARRAY_SIZE(csi_params),
};

static struct sensor_pix_fmt_info_t ov8865_qtech_pix_fmt0_fourcc[] = {
  { V4L2_PIX_FMT_SBGGR10 },
};

static struct sensor_pix_fmt_info_t ov8865_qtech_pix_fmt1_fourcc[] = {
  { MSM_V4L2_PIX_FMT_META },
};

static sensor_stream_info_t ov8865_qtech_stream_info[] = {
  {1, &ov8865_qtech_cid_cfg[0], ov8865_qtech_pix_fmt0_fourcc},
  {1, &ov8865_qtech_cid_cfg[1], ov8865_qtech_pix_fmt1_fourcc},
};

static sensor_stream_info_array_t ov8865_qtech_stream_info_array = {
  .sensor_stream_info = ov8865_qtech_stream_info,
  .size = ARRAY_SIZE(ov8865_qtech_stream_info),
};

#if SNAPSHOT_PARM
static struct msm_camera_i2c_reg_array res0_reg_array[] = {
  /* 3264x2448_2lane_15fps_Sysclk=72M_720MBps/lane */
  //{0x0100, 0x00},
  {0x030f, 0x04},
  {0x3018, 0x32},
  {0x3106, 0x21},
  {0x3501, 0x98},
  {0x3502, 0x60},
  {0x3700, 0x24},
  {0x3701, 0x0c},
  {0x3702, 0x28},
  {0x3703, 0x19},
  {0x3704, 0x14},
  {0x3706, 0x38},
  {0x3707, 0x04},
  {0x3708, 0x24},
  {0x3709, 0x40},
  {0x370a, 0x00},
  {0x370b, 0xb8},
  {0x370c, 0x04},
  {0x3718, 0x12},
  {0x3712, 0x42},
  {0x371e, 0x19},
  {0x371f, 0x40},
  {0x3720, 0x05},
  {0x3721, 0x05},
  {0x3724, 0x02},
  {0x3725, 0x02},
  {0x3726, 0x06},

  {0x3728, 0x05},
  {0x3729, 0x02},
  {0x372a, 0x03},
  {0x372b, 0x53},
  {0x372c, 0xa3},
  {0x372d, 0x53},
  {0x372e, 0x06},
  {0x372f, 0x10},
  {0x3730, 0x01},
  {0x3731, 0x06},
  {0x3732, 0x14},
  {0x3736, 0x20},
  {0x373a, 0x02},
  {0x373b, 0x0c},
  {0x373c, 0x0a},
  {0x373e, 0x03},
  {0x375a, 0x06},
  {0x375b, 0x13},
  {0x375d, 0x02},
  {0x375f, 0x14},
  {0x3767, 0x04},
  {0x3769, 0x20},
  {0x3772, 0x23},
  {0x3773, 0x02},
  {0x3774, 0x16},
  {0x3775, 0x12},
  {0x3776, 0x08},
  {0x37a0, 0x44},
  {0x37a1, 0x3d},
  {0x37a2, 0x3d},
  {0x37a3, 0x02},
  {0x37a5, 0x09},
  {0x37a7, 0x44},
  {0x37a8, 0x58},
  {0x37a9, 0x58},
  {0x37aa, 0x44},
  {0x37ab, 0x2e},
  {0x37ac, 0x2e},
  {0x37ad, 0x33},
  {0x37ae, 0x0d},
  {0x37af, 0x0d},
  {0x37b3, 0x42},
  {0x37b4, 0x42},
  {0x37b5, 0x33},
  {0x3808, 0x0c},
  {0x3809, 0xc0},
  {0x380a, 0x09},
  {0x380b, 0x90},

  {0x380c, 0x07},
  {0x380d, 0x98},
  {0x380e, 0x09},
  {0x380f, 0xf4}, //0xa6
  {0x3813, 0x02},
  {0x3814, 0x01},
  {0x3821, 0x46},
  {0x382a, 0x01},
  {0x382b, 0x01},
  {0x3830, 0x04},
  {0x3836, 0x01},
  {0x3846, 0x48},
  {0x3f08, 0x0b},
  {0x4000, 0xf1},
  {0x4001, 0x04},
  {0x4020, 0x02},
  {0x4021, 0x40},
  {0x4022, 0x03},
  {0x4023, 0x3f},
  {0x4024, 0x07},
  {0x4025, 0xc0},
  {0x4026, 0x08},
  {0x4027, 0xbf},
  {0x402a, 0x04},
  {0x402c, 0x02},
  {0x402d, 0x02},
  {0x402e, 0x08},
  {0x4500, 0x68},
  {0x4601, 0x10},
  {0x5002, 0x08},
  {0x5901, 0x00},
  //{0x0100, 0x01},
};
#endif

#if PREVIEW_PARM
static struct msm_camera_i2c_reg_array res1_reg_array[] = {
  /* 1632X1224_2Lane_30fps_Sysclk=72M_720MBps/lane */
  //{0x0100, 0x00},
  {0x030f, 0x09},
  {0x3018, 0x32},
  {0x3106, 0x01},
  {0x3501, 0x4c},
  {0x3502, 0x00},
  {0x3700, 0x24},
  {0x3701, 0x0c},
  {0x3702, 0x28},
  {0x3703, 0x19},
  {0x3704, 0x14},
  {0x3706, 0x38},
  {0x3707, 0x04},
  {0x3708, 0x24},
  {0x3709, 0x40},
  {0x370a, 0x00},
  {0x370b, 0xb8},
  {0x370c, 0x04},
  {0x3718, 0x12},
  {0x3712, 0x42},
  {0x371e, 0x19},
  {0x371f, 0x40},
  {0x3720, 0x05},
  {0x3721, 0x05},
  {0x3724, 0x02},
  {0x3725, 0x02},
  {0x3726, 0x06},
  {0x3728, 0x05},
  {0x3729, 0x02},
  {0x372a, 0x03},
  {0x372b, 0x53},
  {0x372c, 0xa3},
  {0x372d, 0x53},
  {0x372e, 0x06},
  {0x372f, 0x10},
  {0x3730, 0x01},
  {0x3731, 0x06},
  {0x3732, 0x14},
  {0x3736, 0x20},
  {0x373a, 0x02},
  {0x373b, 0x0c},
  {0x373c, 0x0a},
  {0x373e, 0x03},
  {0x375a, 0x06},

  {0x375b, 0x13},
  {0x375d, 0x02},
  {0x375f, 0x14},
  {0x3767, 0x04},
  {0x3769, 0x20},
  {0x3772, 0x23},
  {0x3773, 0x02},
  {0x3774, 0x16},
  {0x3775, 0x12},
  {0x3776, 0x08},
  {0x37a0, 0x44},
  {0x37a1, 0x3d},
  {0x37a2, 0x3d},
  {0x37a3, 0x01},
  {0x37a5, 0x08},
  {0x37a7, 0x44},
  {0x37a8, 0x58},
  {0x37a9, 0x58},
  {0x37aa, 0x44},
  {0x37ab, 0x2e},
  {0x37ac, 0x2e},
  {0x37ad, 0x33},
  {0x37ae, 0x0d},
  {0x37af, 0x0d},
  {0x37b3, 0x42},
  {0x37b4, 0x42},
  {0x37b5, 0x33},
  {0x3808, 0x06},
  {0x3809, 0x60},
  {0x380a, 0x04},
  {0x380b, 0xc8},
  {0x380c, 0x07}, //0x07
  {0x380d, 0x83}, //0x83
  {0x380e, 0x05}, //04 //05
  {0x380f, 0x08}, //e0 //08
  {0x3813, 0x04},
  {0x3814, 0x03},
  {0x3821, 0x67},
  {0x382a, 0x03},
  {0x382b, 0x01},
  {0x3830, 0x08},
  {0x3836, 0x02},
  {0x3846, 0x88},
  {0x3f08, 0x0b},
  {0x4000, 0xf1},
  {0x4001, 0x14},
  {0x4020, 0x01},
  {0x4021, 0x20},

  {0x4022, 0x01},
  {0x4023, 0x9f},
  {0x4024, 0x03},
  {0x4025, 0xe0},
  {0x4026, 0x04},
  {0x4027, 0x5f},
  {0x402a, 0x04},
  {0x402c, 0x02},
  {0x402d, 0x02},
  {0x402e, 0x08},
  {0x4500, 0x40},
  {0x4601, 0x74},
  {0x5002, 0x08},
  {0x5901, 0x00},
  //{0x0100, 0x01},
};
#endif

static struct msm_camera_i2c_reg_setting res_settings[] = {
  #if SNAPSHOT_PARM
  {
    .reg_setting = res0_reg_array,
    .size = ARRAY_SIZE(res0_reg_array),
    .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
    .data_type = MSM_CAMERA_I2C_BYTE_DATA,
    .delay = 0,
  },
  #endif
  #if PREVIEW_PARM
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
  #if SNAPSHOT_PARM
  {0, 0, 0, 0}, /* RES 0 */
  #endif
  #if PREVIEW_PARM
  {0, 0, 0, 0}, /* RES 1 */
  #endif
};

static struct sensor_lib_crop_params_array crop_params_array = {
  .crop_params = crop_params,
  .size = ARRAY_SIZE(crop_params),
};

static struct sensor_lib_out_info_t sensor_out_info[] = {
  #if SNAPSHOT_PARM
  {
    .x_output = 3264,
    .y_output = 2448,
    .line_length_pclk = 1944 , //3888
    .frame_length_lines = 2548,//2600,//2470 * 2,
    .vt_pixel_clk = 74400000,
    .op_pixel_clk = 148000000,
    .binning_factor = 1,
    .max_fps = 15.0,
    .min_fps = 7.5,
    .mode = SENSOR_DEFAULT_MODE,
  },
  #endif
  #if PREVIEW_PARM
  {

    .x_output = 1632,
    .y_output = 1224,
    .line_length_pclk = 1923, //1923,//1923 * 2,//1923  //3516
    .frame_length_lines = 1288, //ng1250,//ng1252, //1288 30fps//1254,//1256,//1260, //1280, //1320,//1480 //1872,//1248*2,
    .vt_pixel_clk = 74400000,
    .op_pixel_clk = 148000000,
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

static sensor_res_cfg_type_t ov8865_qtech_res_cfg[] = {
  SENSOR_SET_STOP_STREAM,
  SENSOR_SET_NEW_RESOLUTION, /* set stream config */
  SENSOR_SET_CSIPHY_CFG,
  SENSOR_SET_CSID_CFG,
  SENSOR_LOAD_CHROMATIX, /* set chromatix prt */
  SENSOR_SEND_EVENT, /* send event */
  SENSOR_SET_START_STREAM,
};

static struct sensor_res_cfg_table_t ov8865_qtech_res_table = {
  .res_cfg_type = ov8865_qtech_res_cfg,
  .size = ARRAY_SIZE(ov8865_qtech_res_cfg),
};

static struct sensor_lib_chromatix_t ov8865_qtech_chromatix[] = {
  #if SNAPSHOT_PARM
  {
    .common_chromatix = OV8865_QTECH_LOAD_CHROMATIX(common),
    .camera_preview_chromatix = OV8865_QTECH_LOAD_CHROMATIX(snapshot), /* RES0 */
    .camera_snapshot_chromatix = OV8865_QTECH_LOAD_CHROMATIX(snapshot), /* RES0 */
    .camcorder_chromatix = OV8865_QTECH_LOAD_CHROMATIX(default_video), /* RES0 */
    .liveshot_chromatix = OV8865_QTECH_LOAD_CHROMATIX(liveshot), /* RES0 */
  },
  #endif
  #if PREVIEW_PARM
  {
    .common_chromatix = OV8865_QTECH_LOAD_CHROMATIX(common),
    .camera_preview_chromatix = OV8865_QTECH_LOAD_CHROMATIX(preview), /* RES1 */
    .camera_snapshot_chromatix = OV8865_QTECH_LOAD_CHROMATIX(preview), /* RES1 */
    .camcorder_chromatix = OV8865_QTECH_LOAD_CHROMATIX(preview), /* RES1 */
    .liveshot_chromatix = OV8865_QTECH_LOAD_CHROMATIX(liveshot), /* RES1 */
  },
  #endif
};

static struct sensor_lib_chromatix_array ov8865_qtech_lib_chromatix_array = {
  .sensor_lib_chromatix = ov8865_qtech_chromatix,
  .size = ARRAY_SIZE(ov8865_qtech_chromatix),
};

/*===========================================================================
* FUNCTION    - ov8865_qtech_real_to_register_gain -
*
* DESCRIPTION:
*==========================================================================*/
static uint16_t ov8865_qtech_real_to_register_gain(float gain)
{
  uint16_t reg_gain;

  if (gain < 1.0) {
      gain = 1.0;
  } else if (gain > 16.0) {
      gain = 16.0;
  }
  gain = (gain) * 128.0;
  reg_gain = (uint16_t) gain;

  return reg_gain;
}

/*===========================================================================
* FUNCTION    - ov8865_qtech_register_to_real_gain -
*
* DESCRIPTION:
*==========================================================================*/
static float ov8865_qtech_register_to_real_gain(uint16_t reg_gain)
{
  float real_gain;

  if (reg_gain < 0x80) {
      reg_gain = 0x80;
  } else if (reg_gain > 0x7ff) {
      reg_gain = 0x7ff;
  }
  real_gain = (float) reg_gain / 128.0;

  return real_gain;
}

/*===========================================================================
* FUNCTION    - ov8865_qtech_calculate_exposure -
*
* DESCRIPTION:
*==========================================================================*/
static int32_t ov8865_qtech_calculate_exposure(float real_gain,
uint16_t line_count, sensor_exposure_info_t *exp_info)
{
  if (!exp_info) {
    return -1;
  }

  exp_info->reg_gain = ov8865_qtech_real_to_register_gain(real_gain);
  float sensor_real_gain = ov8865_qtech_register_to_real_gain(exp_info->reg_gain);
  exp_info->digital_gain = real_gain / sensor_real_gain;
  exp_info->line_count = line_count;

  return 0;
}

/*===========================================================================
* FUNCTION    - ov8865_qtech_fill_exposure_array -
*
* DESCRIPTION:
*==========================================================================*/
static int32_t ov8865_qtech_fill_exposure_array(uint16_t gain, uint32_t line,
uint32_t fl_lines, int32_t luma_avg, uint32_t fgain,
struct msm_camera_i2c_reg_setting* reg_setting)
{
  int32_t rc = 0;
  #if 1
  uint16_t reg_count = 0;
  uint16_t i = 0;

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
  reg_setting->reg_setting[reg_count].reg_data = (fl_lines & 0x7F00) >> 8;
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr =
  sensor_lib_ptr.output_reg_addr->frame_length_lines + 1;
  reg_setting->reg_setting[reg_count].reg_data = (fl_lines & 0xFF);
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr =
  sensor_lib_ptr.exp_gain_info->coarse_int_time_addr - 1;
  reg_setting->reg_setting[reg_count].reg_data = line >> 12;
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr =
  sensor_lib_ptr.exp_gain_info->coarse_int_time_addr;
  reg_setting->reg_setting[reg_count].reg_data = line >> 4;
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr =
  sensor_lib_ptr.exp_gain_info->coarse_int_time_addr + 1;
  reg_setting->reg_setting[reg_count].reg_data = line << 4;
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr =
  sensor_lib_ptr.exp_gain_info->global_gain_addr;
  reg_setting->reg_setting[reg_count].reg_data = (gain & 0xFF00) >> 8;
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr =
  sensor_lib_ptr.exp_gain_info->global_gain_addr + 1;
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
  #endif
  return rc;
}

static sensor_exposure_table_t ov8865_qtech_expsoure_tbl = {
  .sensor_calculate_exposure = ov8865_qtech_calculate_exposure,
  .sensor_fill_exposure_array = ov8865_qtech_fill_exposure_array,
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
  .sensor_num_frame_skip = 2,
  /* sensor exposure table size */
  .exposure_table_size = 10,//10,
  /* sensor lens info */
  .default_lens_info = &default_lens_info,
  /* csi lane params */
  .csi_lane_params = &csi_lane_params,
  /* csi cid params */
  .csi_cid_params = ov8865_qtech_cid_cfg,
  /* csi csid params array size */
  .csi_cid_params_size = ARRAY_SIZE(ov8865_qtech_cid_cfg),
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
  .sensor_res_cfg_table = &ov8865_qtech_res_table,
  /* res settings */
  .res_settings_array = &res_settings_array,
  /* out info array */
  .out_info_array = &out_info_array,
  /* crop params array */
  .crop_params_array = &crop_params_array,
  /* csi params array */
  .csi_params_array = &csi_params_array,
  /* sensor port info array */
  .sensor_stream_info_array = &ov8865_qtech_stream_info_array,
  /* exposure funtion table */
  .exposure_func_table = &ov8865_qtech_expsoure_tbl,
  /* chromatix array */
  .chromatix_array = &ov8865_qtech_lib_chromatix_array,
};

/*===========================================================================
* FUNCTION    - ov8865_qtech_open_lib -
*
* DESCRIPTION:
*==========================================================================*/
void *ov8865_qtech_open_lib(void)
{
  ALOGE("======== niufangwei ov8865_qtech_open_lib ========/n");
  return &sensor_lib_ptr;
}

