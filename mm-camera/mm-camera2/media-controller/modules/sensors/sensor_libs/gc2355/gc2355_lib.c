/*============================================================================

  Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

============================================================================*/
#include <stdio.h>
#include "sensor_lib.h"
//#include "camera_dbg.h"


#define SENSOR_MODEL_NO_GC2355 "gc2355"
#define GC2355_LOAD_CHROMATIX(n) \
  "libchromatix_"SENSOR_MODEL_NO_GC2355"_"#n".so"

#define ANALOG_GAIN_1 64  // 1.00x
#define ANALOG_GAIN_2 88  // 1.41x
#define ANALOG_GAIN_3 122  // 2.00x
#define ANALOG_GAIN_4 168  // 2.78x
#define ANALOG_GAIN_5 239  // 3.85x
#define ANALOG_GAIN_6 330  // 5.18x
#define ANALOG_GAIN_7 470  // 6.80x

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
  .coarse_int_time_addr = 0x03,
  .global_gain_addr = 0xb2,
  .vert_offset = 0,
};

static sensor_aec_data_t aec_info = {
  .max_gain = 6.0,
  .max_linecount = 16383, 
};

static sensor_lens_info_t default_lens_info = {
  .focal_length = 2.93,
  .pix_size = 1.75,
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
  .csi_phy_sel = 1,
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
    /////////////////////////////////////////////////////
    //////////////////////     SYS   //////////////////////
    /////////////////////////////////////////////////////
	{0xfe,0x80},
	{0xfe,0x80},
	{0xfe,0x80},
	{0xf2,0x00}, 
	{0xf6,0x00}, 
	{0xf7,0x19}, 

	{0xf8,0x06}, 
	{0xf9,0x0e}, 
	{0xfa,0x00}, 
	{0xfc,0x06}, 
	{0xfe,0x00},
	
	/////////////////////////////////////////////////////
	///////////////    ANALOG & CISCTL    ///////////////
	/////////////////////////////////////////////////////
	{0x03,0x07}, 
	{0x04,0xd0}, 
	{0x05,0x03},
	{0x06,0x4c},
	{0x07,0x00},
	{0x08,0x12},
	{0x0a,0x00},
	{0x0c,0x04},
	{0x0d,0x04},
	{0x0e,0xc0},
	{0x0f,0x06},
	{0x10,0x50},
	{0x17,0x14},
	{0x19,0x0b},
	{0x1b,0x48},
	{0x1c,0x12},
	{0x1d,0x10},
	{0x1e,0xbc},
	{0x1f,0xc9},
	{0x20,0x71},
	{0x21,0x20},
	{0x22,0xa0},
	{0x23,0x51},
	{0x24,0x19},
	{0x27,0x20},
	{0x28,0x00},
	{0x2b,0x80}, // 0x81  20140926
	{0x2c,0x38},
	{0x2e,0x16},
	{0x2f,0x14},
	{0x30,0x00},
	{0x31,0x01},
	{0x32,0x02},
	{0x33,0x03},
	{0x34,0x07},
	{0x35,0x0b},
	{0x36,0x0f},
	
    /////////////////////////////////////////////////////
	//////////////////////   MIPI   /////////////////////
	/////////////////////////////////////////////////////
	{0xfe, 0x03},
	{0x10, 0x80},
	{0x01, 0x83},
	{0x22, 0x05},  
	{0x23, 0x30},
	{0x25, 0x15},
	{0x29, 0x06},
	{0x02, 0x00},
	{0x03, 0x90},
	{0x04, 0x01},
	{0x05, 0x00},
	{0x06, 0xa2},
	{0x11, 0x2b},
	{0x12, 0xd0}, 
	{0x13, 0x07}, 
	{0x15, 0x60},

	{0x21, 0x10},
	{0x24, 0x02},
	{0x26, 0x08},
	{0x27, 0x06},
	{0x2a, 0x0a}, 
	{0x2b, 0x08},
	
	{0x40, 0x00},
	{0x41, 0x00},    
	{0x42, 0x40},
	{0x43, 0x06},  
	{0xfe, 0x00},

	/////////////////////////////////////////////////////
	//////////////////////	 gain   /////////////////////
	/////////////////////////////////////////////////////
	{0xb0,0x50},
	{0xb1,0x01},
	{0xb2,0x00},
	{0xb3,0x40},
	{0xb4,0x40},
	{0xb5,0x40},
	{0xb6,0x00},

	/////////////////////////////////////////////////////
	//////////////////////   crop   /////////////////////
	/////////////////////////////////////////////////////
	{0x92,0x02},
	{0x94,0x01},
	{0x95,0x04},
	{0x96,0xb0},
	{0x97,0x06},
	{0x98,0x40}, 

	/////////////////////////////////////////////////////
	//////////////////////    BLK   /////////////////////
	/////////////////////////////////////////////////////
	{0x18,0x02},
	{0x1a,0x01},
	{0x40,0x42},
	{0x41,0x00}, 
	{0x44,0x00}, 
	{0x45,0x00},
	{0x46,0x00},	
	{0x47,0x00}, 
	{0x48,0x00}, 
	{0x49,0x00},
	{0x4a,0x00},	
	{0x4b,0x00},	
	{0x4e,0x3c}, 
	{0x4f,0x00}, 
	{0x5e,0x00},
	{0x66,0x20},
	{0x6a,0x02},
	{0x6b,0x02},
	{0x6c,0x02},
	{0x6d,0x02},
	{0x6e,0x02},
	{0x6f,0x02},
	{0x70,0x02},
	{0x71,0x02},

	/////////////////////////////////////////////////////
	////////////////////  dark sun  /////////////////////
	/////////////////////////////////////////////////////
	{0x87,0x03}, 
	{0xe0,0xe7}, 
	{0xe3,0xc0}, 

};


static struct msm_camera_i2c_reg_setting init_reg_setting[] = {
  {
    .reg_setting = init_reg_array0,
    .size = ARRAY_SIZE(init_reg_array0),
    .addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
    .data_type = MSM_CAMERA_I2C_BYTE_DATA,
    .delay = 50,//00
  },
};

static struct sensor_lib_reg_settings_array init_settings_array = {
  .reg_settings = init_reg_setting,
  .size = 1,
};

static struct msm_camera_i2c_reg_array start_reg_array[] = {
  {0xfe,0x03},
 {0x10,0x90},
  {0xfe,0x00},
};

static  struct msm_camera_i2c_reg_setting start_settings = {
  .reg_setting = start_reg_array,
  .size = ARRAY_SIZE(start_reg_array),
  .addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 10,
};

static struct msm_camera_i2c_reg_array stop_reg_array[] = {
     {0xfe,0x03},
    {0x10,0x80},
     {0xfe,0x00},

};

static struct msm_camera_i2c_reg_setting stop_settings = {
  .reg_setting = stop_reg_array,
  .size = ARRAY_SIZE(stop_reg_array),
  .addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 10,
};

static struct    msm_camera_i2c_reg_array groupon_reg_array[] = {
    {0xfe, 0x00},
};

static struct msm_camera_i2c_reg_setting groupon_settings = {
  .reg_setting = groupon_reg_array,
  .size = ARRAY_SIZE(groupon_reg_array),
  .addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 0,
};

static struct msm_camera_i2c_reg_array groupoff_reg_array[] = {
    {0xfe, 0x00},
};

static struct msm_camera_i2c_reg_setting groupoff_settings = {
  .reg_setting = groupoff_reg_array,
  .size = ARRAY_SIZE(groupoff_reg_array),
  .addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 0,
};

static struct msm_camera_csid_vc_cfg gc2355_cid_cfg[] = {
  {0, CSI_RAW10, CSI_DECODE_10BIT},
  {1, CSI_EMBED_DATA, CSI_DECODE_10BIT},
};

static struct msm_camera_csi2_params gc2355_csi_params = {
  .csid_params = {
    .lane_cnt = 1,
    .lut_params = {
      .num_cid = ARRAY_SIZE(gc2355_cid_cfg),
      .vc_cfg = {
         &gc2355_cid_cfg[0],
         &gc2355_cid_cfg[1],
      },
    },
  },
  .csiphy_params = {
    .lane_cnt = 1,
    .settle_cnt = 0x18,//120ns
  },
};

struct sensor_pix_fmt_info_t rgb10[] =
{  //only a simbol rgb10
    {V4L2_PIX_FMT_SGRBG10},
};

struct sensor_pix_fmt_info_t meta[] =
{//only a simbol meta
    {MSM_V4L2_PIX_FMT_META},
};



static sensor_stream_info_t gc2355_stream_info[] = {
  {1, &gc2355_cid_cfg[0], rgb10},
  {1, &gc2355_cid_cfg[1], meta},
};

static sensor_stream_info_array_t gc2355_stream_info_array = {
  .sensor_stream_info = gc2355_stream_info,
  .size = ARRAY_SIZE(gc2355_stream_info),
};


static struct msm_camera_i2c_reg_array res0_reg_array[] = {
/* lane snap */

    {0xfe, 0x00},
};

static struct msm_camera_i2c_reg_array res1_reg_array[] = {
/*  preveiw */
    {0xfe, 0x00},
};

static struct msm_camera_i2c_reg_setting res_settings[] = {
{  //capture
    .reg_setting = res0_reg_array,
    .size = ARRAY_SIZE(res0_reg_array),
    .addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
    .data_type = MSM_CAMERA_I2C_BYTE_DATA,
    .delay = 0,
  },
  {//preview
    .reg_setting = res1_reg_array,
    .size = ARRAY_SIZE(res1_reg_array),
    .addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
    .data_type = MSM_CAMERA_I2C_BYTE_DATA,
    .delay = 0,
  },
};

static struct sensor_lib_reg_settings_array res_settings_array = {
  .reg_settings = res_settings,
  .size = ARRAY_SIZE(res_settings),
};


static struct msm_camera_csi2_params *csi_params[] = {
  &gc2355_csi_params, /* RES 0*/
  &gc2355_csi_params, /* RES 1*/
};

static struct sensor_lib_csi_params_array csi_params_array = {
  .csi2_params = &csi_params[0],
  .size = ARRAY_SIZE(csi_params),
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
  {
/* For SNAPSHOT */
    .x_output = 1600,
    .y_output = 1200,
    .line_length_pclk = 3360,   
    .frame_length_lines = 833, 
    .vt_pixel_clk = 84000000,     
    .op_pixel_clk = 67200000,    
        .binning_factor    = 1,
    .max_fps = 30.0,              //Must 30 ,can't change
    .min_fps = 7.5,   
    .mode = SENSOR_DEFAULT_MODE,
  },
/* For PREVIEW */
  {

    .x_output = 1600,
    .y_output = 1200,
    .line_length_pclk = 3360,    
    .frame_length_lines = 833,  
    .vt_pixel_clk = 84000000,     
    .op_pixel_clk = 67200000,     
        .binning_factor    = 1,
    .max_fps = 30.0,              //Must 30 ,can't change
    .min_fps = 7.5,   
    .mode = SENSOR_DEFAULT_MODE,
  },
};

static struct sensor_lib_out_info_array out_info_array = {
  .out_info = sensor_out_info,
  .size = ARRAY_SIZE(sensor_out_info),
};

static sensor_res_cfg_type_t gc2355_res_cfg[] = {
  SENSOR_SET_STOP_STREAM,
  SENSOR_SET_NEW_RESOLUTION, /* set stream config */
  SENSOR_SET_CSIPHY_CFG,
  SENSOR_SET_CSID_CFG,
  SENSOR_LOAD_CHROMATIX, /* set chromatix prt */
  SENSOR_SEND_EVENT, /* send event */
  SENSOR_SET_START_STREAM,
};

static struct sensor_res_cfg_table_t gc2355_res_table = {
  .res_cfg_type = gc2355_res_cfg,
  .size = ARRAY_SIZE(gc2355_res_cfg),
};

static struct sensor_lib_chromatix_t gc2355_chromatix[] = {
  {
    .common_chromatix = GC2355_LOAD_CHROMATIX(common),
    .camera_preview_chromatix = GC2355_LOAD_CHROMATIX(preview), /* RES0 */
    .camera_snapshot_chromatix = GC2355_LOAD_CHROMATIX(preview), /* RES0 */
    .camcorder_chromatix = GC2355_LOAD_CHROMATIX(preview), /* RES0 */
  },
  {
    .common_chromatix = GC2355_LOAD_CHROMATIX(common),
    .camera_preview_chromatix = GC2355_LOAD_CHROMATIX(preview), /* RES0 */
    .camera_snapshot_chromatix = GC2355_LOAD_CHROMATIX(preview), /* RES0 */
    .camcorder_chromatix = GC2355_LOAD_CHROMATIX(preview), /* RES0 */
  },
};

static struct sensor_lib_chromatix_array gc2355_lib_chromatix_array = {
  .sensor_lib_chromatix = gc2355_chromatix,
  .size = ARRAY_SIZE(gc2355_chromatix),
};

/*===========================================================================
 * FUNCTION    - gc2355_real_to_register_gain -
 *
 * DESCRIPTION:
 *==========================================================================*/
static uint16_t gc2355_real_to_register_gain(float gain)
{
    uint16_t reg_gain;
    if (gain < 1.0)
        gain = 1.0;
    if (gain > 6.0)
        gain = 6.0;
//    ALOGE("gc2355_PETER,real_gain=%f" , gain);
    reg_gain = (uint16_t)(gain * 64.0f);
    return reg_gain;

}

/*===========================================================================
 * FUNCTION    - gc2355_register_to_real_gain -
 *
 * DESCRIPTION:
 *==========================================================================*/
static float gc2355_register_to_real_gain(uint16_t reg_gain)
{
    float gain;
    if (reg_gain > 0x0180)
        reg_gain = 0x0180;
//    ALOGE("gc2355_PETER register_gain=%u" , reg_gain);
    gain = (float)(reg_gain/64.0f);
    return gain;

}

/*===========================================================================
 * FUNCTION    - gc2355_calculate_exposure -
 *
 * DESCRIPTION:
 *==========================================================================*/
static int32_t gc2355_calculate_exposure(float real_gain,
  uint16_t line_count, sensor_exposure_info_t *exp_info)
{
  if (!exp_info) {
    return -1;
  }
  exp_info->reg_gain = gc2355_real_to_register_gain(real_gain);
  float sensor_real_gain = gc2355_register_to_real_gain(exp_info->reg_gain);
  exp_info->digital_gain = real_gain / sensor_real_gain;
  exp_info->line_count = line_count;
  return 0;
}

/*===========================================================================
 * FUNCTION    - gc2355_fill_exposure_array -
 *
 * DESCRIPTION:
 *==========================================================================*/

static int32_t gc2355_fill_exposure_array(uint16_t gain, uint32_t line,
  uint32_t fl_lines,int32_t luma_avg, uint32_t fgain,
   struct msm_camera_i2c_reg_setting *reg_setting)
{
   int32_t rc = 0;

  uint16_t reg_count = 0;
    uint16_t gain_b6,gain_b1,gain_b2;
     	uint16_t iReg,temp;
	int32_t i;


//    ALOGE("gc2355_11111,fl_lines=%d,gain=%d, line=%d\n",fl_lines,gain,line);
	
	if (1 == line && gain <= 64)
		return rc;	



  if (!reg_setting) {
    return -1;
  }

   iReg = gain;

  
	if(iReg < 0x40)
		iReg = 0x40;
	else if((ANALOG_GAIN_1<= iReg)&&(iReg < ANALOG_GAIN_2))  //ANALOG_GAIN_1 == 64
	{
		//analog gain
        gain_b6 = 0x00;   
        temp = iReg;     
        gain_b1 = temp>>6;
        gain_b2 = (temp<<2)&0xfc;			
	}
	else if((ANALOG_GAIN_2<= iReg)&&(iReg < ANALOG_GAIN_3))
	{
	//analog gain
  	gain_b6 = 0x01;   
  	temp = 64*iReg/ANALOG_GAIN_2;    
  	gain_b1 = temp>>6;
  	gain_b2 = (temp<<2)&0xfc;		        	
	}
  	else if((ANALOG_GAIN_3<= iReg)&&(iReg < ANALOG_GAIN_4))
	{
		//analog gain           
  	gain_b6 = 0x02;   
  	temp = 64*iReg/ANALOG_GAIN_3;   
  	gain_b1 = temp>>6;
  	gain_b2 = (temp<<2)&0xfc;
 	}
	else if(ANALOG_GAIN_4<= iReg)
	{
		//analog gain
  	gain_b6 = 0x03;   
  	temp = 64*iReg/ANALOG_GAIN_4;    
  	gain_b1 = temp>>6;
  	gain_b2 = (temp<<2)&0xfc;	        		
	}
 	

  


/***********************Shutter Start***************************************/
if (!line) line = 1; /* avoid 0 */
	
	if(line < 1) line = 1;
	if(line > 16383) line = 16383;

	//align 4


      reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.exp_gain_info->coarse_int_time_addr;
  reg_setting->reg_setting[reg_count].reg_data = (line & 0x3F00) >> 8;
  reg_count++;

  reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.exp_gain_info->coarse_int_time_addr + 1;
  reg_setting->reg_setting[reg_count].reg_data = line & 0xFF;
  reg_count++;




  /***********************Shutter End***************************************/

    reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.exp_gain_info->global_gain_addr + 4;
  reg_setting->reg_setting[reg_count].reg_data = gain_b6; //0xb6
  reg_count++;
  

  
    reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.exp_gain_info->global_gain_addr - 1;  //0xb1
  reg_setting->reg_setting[reg_count].reg_data = gain_b1;
  reg_count++;
  
  
    reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.exp_gain_info->global_gain_addr;
  reg_setting->reg_setting[reg_count].reg_data = gain_b2;  //0xb2
  reg_count++;
  
    
    reg_setting->size = reg_count;
    reg_setting->addr_type = MSM_CAMERA_I2C_BYTE_ADDR;
      reg_setting->data_type = MSM_CAMERA_I2C_BYTE_DATA;
    reg_setting->delay = 0;



  return rc;
}

static sensor_exposure_table_t gc2355_expsoure_tbl = {
  .sensor_calculate_exposure = gc2355_calculate_exposure,
  .sensor_fill_exposure_array = gc2355_fill_exposure_array,
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
  .sensor_num_frame_skip = 1,  //1  peter
  /* sensor exposure table size */
  .exposure_table_size = 6,
  /* sensor lens info */
  .default_lens_info = &default_lens_info,
  /* csi lane params */
  .csi_lane_params = &csi_lane_params,
  /* csi cid params */
  .csi_cid_params = gc2355_cid_cfg,
  /* csi csid params array size */
  .csi_cid_params_size = ARRAY_SIZE(gc2355_cid_cfg),
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
  .sensor_res_cfg_table = &gc2355_res_table,
  /* res settings */
  .res_settings_array = &res_settings_array,
  /* out info array */
  .out_info_array = &out_info_array,
  /* crop params array */
  .crop_params_array = &crop_params_array,
  /* csi params array */
  .csi_params_array = &csi_params_array,
  /* sensor port info array */
  .sensor_stream_info_array = &gc2355_stream_info_array,
  /* exposure funtion table */
  .exposure_func_table = &gc2355_expsoure_tbl,
  /* chromatix array */
  .chromatix_array = &gc2355_lib_chromatix_array,
};

/*===========================================================================
 * FUNCTION    - SKUAA_ST_gc2355_open_lib -
 *
 * DESCRIPTION:
 *==========================================================================*/
void *gc2355_open_lib(void)
{
 // CDBG("gc2355_open_lib is called");
  return &sensor_lib_ptr;
}
