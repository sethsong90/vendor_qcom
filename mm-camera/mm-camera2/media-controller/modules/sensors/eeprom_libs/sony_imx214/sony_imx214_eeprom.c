/*============================================================================

  Copyright (c) 2013-2014 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

============================================================================*/
#include <stdio.h>
#include "chromatix.h"
#include "eeprom.h"
#include "sensor_common.h"

static uint8_t bLscAwbValid;

/** sony_imx214_get_calibration_items:
 *    @e_ctrl: address of pointer to
 *                   chromatix struct
 *
 * Loads data structure for enabling / disabling parameters that can be
 * calibrated
 *
 * Return:
 * void
 **/
void sony_imx214_get_calibration_items( void *e_ctrl )
{
  sensor_eeprom_data_t *ectrl = (sensor_eeprom_data_t *)e_ctrl;
  eeprom_calib_items_t *e_items = &(ectrl->eeprom_data.items);

  e_items->is_insensor = FALSE;
  if (bLscAwbValid) {
    e_items->is_lsc = TRUE;
    SHIGH("WBC and LSC Available and loaded");
  } else {
    e_items->is_lsc = FALSE;
    SHIGH("WBC and LSC UNavailable and not loaded");
  }
  e_items->is_afc = FALSE;
  e_items->is_wbc = FALSE;
  e_items->is_dpc = FALSE;
}


void sony_imx214_eeprom_read_wbdata_groups(sensor_eeprom_data_t * ectrl,
  int offsetL0, int offsetL1, int offsetL2, int index)
{
  uint8_t * data = ectrl->eeprom_params.buffer;
  int offset = 0;
  float qvalue;
  float awb_r_over_gr, awb_b_over_gr, awb_gb_over_gr;

  if (data[offsetL2]) {
    offset = offsetL2;
  } else
  if (data[offsetL1]) {
    offset = offsetL1;
  } else
  if (data[offsetL0]) {
    offset = offsetL0;
  } else {
    SHIGH("OTP AWB data not present for LightType[%d]", index);
    return;
  }  
  offset += 15;
  bLscAwbValid = 1;

  awb_r_over_gr  = (float)((uint16_t)( (data[offset    ] << 8) | data[offset + 1] ));
  qvalue         = (float)((uint16_t)( (data[offset + 2] << 8) | data[offset + 3] ));
  awb_gb_over_gr = (float)((uint16_t)( (data[offset + 4] << 8) | data[offset + 5] ));
  awb_b_over_gr  = (float)((uint16_t)( (data[offset + 6] << 8) | data[offset + 7] ));

  if (awb_gb_over_gr == 0 || qvalue == 0) {
    awb_r_over_gr = 0;
    awb_b_over_gr = 0;
    awb_gb_over_gr = 0;
  } else {
    awb_r_over_gr = awb_r_over_gr / qvalue;
    awb_b_over_gr = awb_b_over_gr / qvalue;
    awb_gb_over_gr = qvalue / awb_gb_over_gr;
  }

  ectrl->eeprom_data.wbc.r_over_g[index] = awb_r_over_gr;
  ectrl->eeprom_data.wbc.b_over_g[index] = awb_b_over_gr;
  ectrl->eeprom_data.wbc.gr_over_gb = awb_gb_over_gr;
}

/** sony_imx214_format_calibration_data:
 *    @e_ctrl: address of pointer to
 *                   chromatix struct
 *
 *  This function call all the sub function to read chromatix data and calibrate
 *  the sensor
 *
 * Return:
 * void
 **/
void sony_imx214_format_calibration_data(void *e_ctrl)
{
  SLOW("Enter");

  sensor_eeprom_data_t *ectrl = (sensor_eeprom_data_t *)e_ctrl;
  uint8_t *data = ectrl->eeprom_params.buffer;
  int index;
  float * r_over_g = ectrl->eeprom_data.wbc.r_over_g;
  float * b_over_g = ectrl->eeprom_data.wbc.b_over_g;

  SHIGH("Total bytes in OTP buffer: %d", ectrl->eeprom_params.num_bytes);
  if (ectrl->eeprom_params.num_bytes < 279) {
    SHIGH("OTP buffer size is less than expected!");
  }
  sony_imx214_eeprom_read_wbdata_groups(ectrl,  93, 124, 155, AGW_AWB_WARM_FLO);
  sony_imx214_eeprom_read_wbdata_groups(ectrl, 186, 217, 248, AGW_AWB_A);
  sony_imx214_eeprom_read_wbdata_groups(ectrl,   0,  31,  62, AGW_AWB_D50);

  if (r_over_g[AGW_AWB_D50] == 0 ||
      r_over_g[AGW_AWB_WARM_FLO] == 0 ||
      r_over_g[AGW_AWB_A] == 0) {
    for (index = 0; index < AGW_AWB_MAX_LIGHT; index++) {
      r_over_g[index] = r_over_g[AGW_AWB_D50];
      b_over_g[index] = b_over_g[AGW_AWB_D50];
    }
  } else {
    r_over_g[AGW_AWB_D65]  = r_over_g[AGW_AWB_D50];
    r_over_g[AGW_AWB_D75]  = r_over_g[AGW_AWB_D50];
    r_over_g[AGW_AWB_NOON] = r_over_g[AGW_AWB_D50];
    r_over_g[AGW_AWB_CUSTOM_DAYLIGHT] = r_over_g[AGW_AWB_D50];

    b_over_g[AGW_AWB_D65]  = b_over_g[AGW_AWB_D50];
    b_over_g[AGW_AWB_D75]  = b_over_g[AGW_AWB_D50];
    b_over_g[AGW_AWB_NOON] = b_over_g[AGW_AWB_D50];
    b_over_g[AGW_AWB_CUSTOM_DAYLIGHT] = b_over_g[AGW_AWB_D50];

    r_over_g[AGW_AWB_COLD_FLO]   = r_over_g[AGW_AWB_WARM_FLO];
    r_over_g[AGW_AWB_CUSTOM_FLO] = r_over_g[AGW_AWB_WARM_FLO];

    b_over_g[AGW_AWB_COLD_FLO]   = b_over_g[AGW_AWB_WARM_FLO];
    b_over_g[AGW_AWB_CUSTOM_FLO] = b_over_g[AGW_AWB_WARM_FLO];

    r_over_g[AGW_AWB_HORIZON]  = r_over_g[AGW_AWB_A];
    r_over_g[AGW_AWB_CUSTOM_A] = r_over_g[AGW_AWB_A];
    r_over_g[AGW_AWB_U30]      = r_over_g[AGW_AWB_A];

    b_over_g[AGW_AWB_HORIZON]  = b_over_g[AGW_AWB_A];
    b_over_g[AGW_AWB_CUSTOM_A] = b_over_g[AGW_AWB_A];
    b_over_g[AGW_AWB_U30]      = b_over_g[AGW_AWB_A];
  }

  SLOW("Exit");
}

/** sony_imx214_lib_func_ptr:
 *  This structure creates the function pointer for imx135 eeprom lib
 **/
static eeprom_lib_func_t sony_imx214_lib_func_ptr = {
    .get_calibration_items = sony_imx214_get_calibration_items,
    .format_calibration_data = sony_imx214_format_calibration_data,
    .do_af_calibration = NULL,
    .do_wbc_calibration = NULL,
    .do_lsc_calibration = NULL,
    .do_dpc_calibration = NULL,
    .get_dpc_calibration_info = NULL,
    .get_raw_data = NULL,
};

/** sony_imx214_eeprom_open_lib:
 *    @e_ctrl: address of pointer to
 *                   chromatix struct
 *
 *  This function call all the sub function to read chromatix data and calibrate
 *  the sensor
 *
 * Return:
 * void* : Pinter to the sonyimx214 function table
 **/
void * sony_imx214_eeprom_open_lib(void) {
  return &sony_imx214_lib_func_ptr;
}
