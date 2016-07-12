/*============================================================================


  Copyright (c) 2013-2014 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.


============================================================================*/
#include <stdio.h>
#include <dlfcn.h>
#include <math.h>
#include <stdlib.h>
#include "chromatix.h"
#include "eeprom.h"
#include "sensor_common.h"


#define OTP_REG_ARRAY_SIZE 61

uint8_t f4k37ab_otp_data[64];
struct msm_camera_i2c_reg_array g_reg_array[OTP_REG_ARRAY_SIZE];
struct msm_camera_i2c_reg_setting g_reg_setting;


/** f4k37ab_qtech_t4k37_get_calibration_items:
 *    @e_ctrl: point to sensor_eeprom_data_t of the eeprom device
 *
 * Get calibration capabilities and mode items.
 *
 * This function executes in eeprom module context
 *
 * Return: void.
 **/
void f4k37ab_qtech_t4k37_get_calibration_items(void *e_ctrl)
{
  SHIGH("e_ctrl = %p ", e_ctrl);
  if (!e_ctrl) {
    SHIGH("ERROR: e_ctrl is NULL");
  } else {
    sensor_eeprom_data_t *ectrl = (sensor_eeprom_data_t *)e_ctrl;
    eeprom_calib_items_t *e_items = &(ectrl->eeprom_data.items);
    e_items->is_insensor = TRUE;
    e_items->is_afc = FALSE;
    e_items->is_wbc = TRUE;
    e_items->is_lsc = TRUE;
    e_items->is_dpc = FALSE;
  }
}


static void f4k37ab_qtech_t4k37_update_awb()
{
  uint32_t r_golden, g_golden, b_golden;
  uint32_t g_otp, r_otp, b_otp;
  uint32_t rg, bg;
  uint8_t flag, temp;
  int index;
  uint8_t * data = f4k37ab_otp_data;

  r_golden = 0x91;
  g_golden = 0xA6;
  b_golden = 0x81;
  
  g_otp =  data[57] + (data[56] << 8);
  g_otp += data[59] + (data[58] << 8);
  g_otp = (g_otp >> 1) & 0xFFFF;

  r_otp = data[55] + (data[54] << 8);
  b_otp = data[61] + (data[60] << 8);

  rg = ((r_golden << 8) * g_otp) / (g_golden * r_otp);
  bg = ((b_golden << 8) * g_otp) / (g_golden * b_otp);

  SLOW("r_golden=0x%x, g_golden=0x%x, b_golden=0x%x", r_golden, g_golden, b_golden);
  SLOW("r_otp=0x%x, g_otp=0x%x, b_otp=0x%x", r_otp, g_otp, b_otp);
  SLOW("rg=0x%x, bg=0x%x", rg, bg);

  g_reg_array[g_reg_setting.size].reg_addr = 0x212;
  g_reg_array[g_reg_setting.size].reg_data = rg >> 8;
  g_reg_setting.size++;
  g_reg_array[g_reg_setting.size].reg_addr = 0x213;
  g_reg_array[g_reg_setting.size].reg_data = (uint8_t)rg;
  g_reg_setting.size++;
  g_reg_array[g_reg_setting.size].reg_addr = 0x214;
  g_reg_array[g_reg_setting.size].reg_data = bg >> 8;
  g_reg_setting.size++;
  g_reg_array[g_reg_setting.size].reg_addr = 0x215;
  g_reg_array[g_reg_setting.size].reg_data = (uint8_t)bg;
  g_reg_setting.size++;
}

static int f4k37ab_qtech_t4k37_read_wbdata(sensor_eeprom_data_t *e_ctrl)
{
  int i;
  uint8_t sum = 0;
  uint8_t *data = e_ctrl->eeprom_params.buffer;

  for (i = 2; i < 64; i++) {
    sum += data[i];
  }
  if (data[1] == sum) {
    SHIGH("%s: otp lsc checksum ok! checksum is 0x%x", __func__, sum);
    for (i = 3; i < 64; i++) {
      f4k37ab_otp_data[i-2] = data[i];
    }
    return 1;
  } else {
    g_reg_array[g_reg_setting.size].reg_addr = 0x37;
    g_reg_array[g_reg_setting.size].reg_data = 0;
    g_reg_setting.size++;
    SERR("%s: ERROR: check_sum 0x%x expected 0x%x", __func__, sum, data[1]);
    return 0;
  }
}

/** f4k37ab_qtech_t4k37_format_wbdata:
 *    @e_ctrl: point to sensor_eeprom_data_t of the eeprom device
 *
 * Format the data structure of white balance calibration
 *
 * This function executes in eeprom module context
 *
 * Return: int.
 **/
int f4k37ab_qtech_t4k37_format_wbdata(sensor_eeprom_data_t *e_ctrl)
{
  int rc = 0;
  rc = f4k37ab_qtech_t4k37_read_wbdata(e_ctrl);
  if (rc < 0) {
    SERR("%s: read wbdata failed", __func__);
  } else {
    f4k37ab_qtech_t4k37_update_awb();
  }
  return rc;
}



/** f4k37ab_qtech_t4k37_format_lensshading:
 *    @e_ctrl: point to sensor_eeprom_data_t of the eeprom device
 *
 * Format the data structure of lens shading correction calibration
 *
 * This function executes in eeprom module context
 *
 * Return: void.
 **/
void f4k37ab_qtech_t4k37_format_lensshading(sensor_eeprom_data_t *e_ctrl)
{
  uint32_t index, addr, i;
	uint8_t val;

  SLOW("%s: Enter", __func__);
  g_reg_array[g_reg_setting.size].reg_addr = 0x323A;
  g_reg_array[g_reg_setting.size].reg_data = f4k37ab_otp_data[1];
  g_reg_setting.size++;
  addr = 0x323E;
  for (i=0; i < 52; i++) {
    val = f4k37ab_otp_data[i+2];
    SLOW("%s: SET LSC[%d], addr:0x%0x, val:0x%0x", __func__, i+1, addr, val);
    g_reg_array[g_reg_setting.size].reg_addr = addr;
    g_reg_array[g_reg_setting.size].reg_data = val;
    g_reg_setting.size++;
    addr++;
  }
  g_reg_array[g_reg_setting.size].reg_addr = 0x3237;
  g_reg_array[g_reg_setting.size].reg_data = 0x80;
  g_reg_setting.size++;
  SLOW("%s: Exit", __func__);
}


/** f4k37ab_qtech_t4k37_format_calibration_data:
 *    @e_ctrl: point to sensor_eeprom_data_t of the eeprom device
 *
 * Format all the data structure of calibration
 *
 * This function executes in eeprom module context and generate
 *   all the calibration registers setting of the sensor.
 *
 * Return: void.
 **/
void f4k37ab_qtech_t4k37_format_calibration_data(void *e_ctrl)
{
  int rc = 0;

  SHIGH("%s: Enter", __func__);
  sensor_eeprom_data_t *ectrl = (sensor_eeprom_data_t *)e_ctrl;
  uint8_t *data = ectrl->eeprom_params.buffer;

  g_reg_setting.addr_type = MSM_CAMERA_I2C_WORD_ADDR;
  g_reg_setting.data_type = MSM_CAMERA_I2C_BYTE_DATA;
  g_reg_setting.reg_setting = &g_reg_array[0];
  g_reg_setting.size = 0;
  g_reg_setting.delay = 0;

  SHIGH("%s: module vendor id is: 0x%x", __func__, (int)data[262]);
  rc = f4k37ab_qtech_t4k37_format_wbdata(ectrl);
  if (!rc) {
    SERR("%s: read OTP data failed", __func__);
  } else {
    f4k37ab_qtech_t4k37_format_lensshading(ectrl);
  }
  SHIGH("%s: Exit", __func__);
}


/** f4k37ab_qtech_t4k37_get_raw_data:
 *    @e_ctrl: point to sensor_eeprom_data_t of the eeprom device
 *    @data: point to the destination msm_camera_i2c_reg_setting
 *
 * Get the all the calibration registers setting of the sensor
 *
 * This function executes in eeprom module context.
 *
 * Return: void.
 **/
void f4k37ab_qtech_t4k37_get_raw_data(void *e_ctrl, void *data)
{
  if (!e_ctrl || !data) {
    SERR("%s: failed Null pointer", __func__);
    return;
  }
  memcpy(data, &g_reg_setting, sizeof(g_reg_setting));
}


static eeprom_lib_func_t f4k37ab_qtech_t4k37_lib_func_ptr = {
  .get_calibration_items = f4k37ab_qtech_t4k37_get_calibration_items,
  .format_calibration_data = f4k37ab_qtech_t4k37_format_calibration_data,
  .do_af_calibration = NULL,
  .do_wbc_calibration = NULL,
  .do_lsc_calibration = NULL,
  .do_dpc_calibration = NULL,
  .get_dpc_calibration_info = NULL,
  .get_raw_data = f4k37ab_qtech_t4k37_get_raw_data,
};


/** f4k37ab_qtech_t4k37_eeprom_open_lib:
 *
 * Get the funtion pointer of this lib.
 *
 * This function executes in eeprom module context.
 *
 * Return: eeprom_lib_func_t point to the function pointer.
 **/
void* f4k37ab_qtech_t4k37_eeprom_open_lib(void)
{
  return &f4k37ab_qtech_t4k37_lib_func_ptr;
}