/**********************************************************************
* Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved. *
* Qualcomm Technologies Proprietary and Confidential.                 *
**********************************************************************/

#ifndef __HDR_COMP_H__
#define __HDR_COMP_H__

#include "img_comp_priv.h"

#if HDR_LIB_GHOSTBUSTER
#include "hdr_gb_lib.h"
#else
#include "hdr_lib.h"
#endif

#include "hdr.h"

/** hdr_comp_t
 *   @b: base image component
 *   @gamma: gamma tables for the HDR
 *   @param: hdr tables required for configuration
 *   @structHdrConfig: hdr configuration for the library
 *   @count: number of buffers send by the user
 *   @mode: hdr mode of operation
 *   @p_main_frame: pointer to array of main frame buffers
 *   @p_analysis_frame: pointer to array of analysis buffers
 *   @analyse_image: Flag to indicate if analysis frame is
 *                 present
 *   @main_count: number or main image buffers
 *   @out_index: Index of the buffer to be used as output buffer
 *   @out_crop: Output image crop
 *
 *   HDR component structure
 **/
typedef struct {
  /*base component*/
  img_component_t b;
  hdr_gamma_table_struct_t gamma;
  hdr_param_t param;
  hdr_config_t structHdrConfig;
  int count;
  hdr_mode_t mode;
  img_frame_t *p_main_frame[MAX_HDR_FRAMES];
  img_frame_t *p_analysis_frame[MAX_HDR_FRAMES];
  int analyse_image;
  int main_count;
  int out_index;
  hdr_crop_t out_crop;
} hdr_comp_t;

#endif //__HDR_COMP_H__

