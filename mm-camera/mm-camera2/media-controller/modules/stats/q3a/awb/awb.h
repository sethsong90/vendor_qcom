/* awb.h
 *
 * Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef __AWB_H__
#define __AWB_H__
/* Data sturcture for stats: AWB */
#include "chromatix_common.h"
#include "q3a_stats.h"
#include "mct_event_stats.h"

#ifdef AWB_DEBG_HIGH
#define CDBG_AWB  CDBG_HIGH
#else
#undef CDBG_AWB
#define CDBG_AWB  CDBG
#endif


typedef enum {
  AWB_STATS_YUV,
  AWB_STATS_BAYER
} awb_stats_type_t;

typedef enum {
  CAMERA_WB_MIN_MINUS_1 = -1,
  CAMERA_WB_AUTO = 0,  /* This list must match aeecamera.h */
  CAMERA_WB_CUSTOM,
  CAMERA_WB_INCANDESCENT,
  CAMERA_WB_FLUORESCENT,
  CAMERA_WB_WARM_FLUORESCENT,
  CAMERA_WB_DAYLIGHT,
  CAMERA_WB_CLOUDY_DAYLIGHT,
  CAMERA_WB_TWILIGHT,
  CAMERA_WB_SHADE,
  CAMERA_WB_OFF,
  CAMERA_WB_MAX_PLUS_1
} awb_config3a_wb_t;

typedef enum {
  AWB_UPDATE,
  AWB_SEND_EVENT,
} awb_output_type_t;
#if 0 /*remove later: Marvin*/
typedef enum {
  AWB_BESTSHOT_OFF = 0,
  AWB_BESTSHOT_AUTO = 1,
  AWB_BESTSHOT_LANDSCAPE = 2,
  AWB_BESTSHOT_SNOW,
  AWB_BESTSHOT_BEACH,
  AWB_BESTSHOT_SUNSET,
  AWB_BESTSHOT_NIGHT,
  AWB_BESTSHOT_PORTRAIT,
  AWB_BESTSHOT_BACKLIGHT,
  AWB_BESTSHOT_SPORTS,
  AWB_BESTSHOT_ANTISHAKE,
  AWB_BESTSHOT_FLOWERS,
  AWB_BESTSHOT_CANDLELIGHT,
  AWB_BESTSHOT_FIREWORKS,
  AWB_BESTSHOT_PARTY,
  AWB_BESTSHOT_NIGHT_PORTRAIT,
  AWB_BESTSHOT_THEATRE,
  AWB_BESTSHOT_ACTION,
  AWB_BESTSHOT_AR,
  AWB_BESTSHOT_MAX
} awb_bestshot_mode_type_t;
#endif

typedef enum _awb_set_parameter_type {
  AWB_SET_PARAM_INIT_CHROMATIX_SENSOR   = 1,
  AWB_SET_PARAM_WHITE_BALANCE,
  AWB_SET_PARAM_RESTORE_LED_GAINS,
  AWB_SET_PARAM_LOCK,
  AWB_SET_PARAM_BESTSHOT,
  AWB_SET_PARAM_EZ_DISABLE,
  AWB_SET_PARAM_EZ_LOCK_OUTPUT,
  AWB_SET_PARAM_LINEAR_GAIN_ADJ,
  AWB_SET_PARAM_AEC_PARM,
  AWB_SET_PARAM_OP_MODE,
  AWB_SET_PARAM_VIDEO_HDR,
  AWB_SET_PARAM_STATS_DEBUG_MASK,
  AWB_SET_PARAM_ENABLE,
  AWB_SET_PARAM_EZ_TUNE_RUNNING,
} awb_set_parameter_type;

typedef q3a_operation_mode_t awb_operation_mode_t;

typedef struct _awb_set_parameter_init {
  awb_stats_type_t     stats_type;

  void                 *chromatix;
  void                 *comm_chromatix;

  /* op_mode can be derived from stream info */
  awb_operation_mode_t op_mode;

} awb_set_parameter_init_t;

typedef struct {
   int   exp_index;
   int   indoor_index;
   int   outdoor_index;
   float lux_idx;
   int   aec_settled;

   /* Luma */
   uint32_t target_luma;
   uint32_t cur_luma;
   uint32_t average_luma;
   /* exposure */
   uint32_t cur_line_cnt;
   float cur_real_gain;
   float stored_digital_gain;

   q3q_flash_sensitivity_t  flash_sensitivity;

   /*Led state*/
   int led_state;
   int use_led_estimation;
   int aec_flash_settled;
} awb_set_aec_parms;

typedef struct _awb_set_parameter {
  awb_set_parameter_type type;

  union {
    awb_set_parameter_init_t          init_param;
    int32_t                           awb_current_wb;
    int32_t                           awb_best_shot;
    int                               ez_disable;
    int                               ez_lock_output;
    int                               linear_gain_adj;
    awb_set_aec_parms                 aec_parms;
    boolean                           awb_lock;
    boolean                           awb_enable;
    int32_t                           video_hdr;
    uint32_t                          stats_debug_mask;
    boolean                           ez_running;
  } u;
} awb_set_parameter_t;

typedef struct {
  uint32_t t1;
  uint32_t t2;
  uint32_t t3;
  uint32_t t6;
  uint32_t t4;
  uint32_t mg;
  uint32_t t5;
}awb_exterme_col_param_t;

typedef struct {
  uint32_t regionW;
  uint32_t regionH;
  uint32_t regionHNum;
  uint32_t regionVNum;
  uint32_t regionHOffset;
  uint32_t regionVOffset;
}awb_stats_region_info_t;


typedef struct {
  chromatix_manual_white_balance_type gain;
  uint32_t color_temp;
  chromatix_wb_exp_stats_type bounding_box;
  //awb_stats_region_info_t region_info;
  awb_exterme_col_param_t exterme_col_param;
}stats_proc_awb_params_t;

typedef struct {
  chromatix_manual_white_balance_type curr_gains;
  uint32_t color_temp;
}stats_proc_awb_gains_t;

/* AWB GET DATA */
typedef enum {
  AWB_PARMS,
  AWB_GAINS,
} awb_get_t;

typedef struct {
  awb_get_t type;
  union {
    stats_proc_awb_params_t awb_params;
    stats_proc_awb_gains_t  awb_gains;
  } d;
} stats_proc_get_awb_data_t;


typedef struct _awb_get_parameter {
  awb_get_t type;
    union {
      stats_proc_awb_params_t awb_params;
      stats_proc_awb_gains_t  awb_gains;
    } u;
} awb_get_parameter_t;

typedef struct _awb_output_eztune_data {
  boolean  awb_enable;
  boolean  ez_running;
  int      prev_exp_index;
  int      valid_sample_cnt;
  int      n_outlier;
  float    day_rg_ratio;
  float    day_bg_ratio;
  int      day_cluster;
  int      day_cluster_weight_distance;
  int      day_cluster_weight_illuminant;
  int      day_cluster_weight_dis_ill;
  float    f_rg_ratio;
  float    f_bg_ratio;
  int      f_cluster;
  int      f_cluster_weight_distance;
  int      f_cluster_weight_illuminant;
  int      f_cluster_weight_dis_ill;
  float    a_rg_ratio;
  float    a_bg_ratio;
  int      a_cluster;
  int      a_cluster_weight_distance;
  int      a_cluster_weight_illuminant;
  int      a_cluster_weight_dis_ill;
  float    h_rg_ratio;
  float    h_bg_ratio;
  int      h_cluster;
  int      h_cluster_weight_distance;
  int      h_cluster_weight_illuminant;
  int      h_cluster_weight_dis_ill;
  int      sgw_cnt;
  float    sgw_rg_ratio;
  float    sgw_bg_ratio;
  int      green_line_mx;
  int      green_line_bx;
  int      green_zone_top;
  int      green_zone_bottom;
  int      green_zone_left;
  int      green_zone_right;
  float    outdoor_green_rg_ratio;
  float    outdoor_green_bg_ratio;
  float    outdoor_green_grey_rg_ratio;
  float    outdoor_green_grey_bg_ratio;
  int      outdoor_green_cnt;
  int      green_percent;
  float    slope_factor_m;
  int      extreme_b_mag;
  int      nonextreme_b_mag;
  int      ave_rg_ratio_x;
  int      ave_bg_ratio_x;
  int      weighted_sample_rg_grid;
  int      weighted_sample_bg_grid;
  float    weighted_sample_day_rg_ratio;
  float    weighted_sample_day_bg_ratio;
  float    weighted_sample_day_shade_rg_ratio;
  float    weighted_sample_day_shade_bg_ratio;
  float    weighted_sample_day_d50_rg_ratio;
  float    weighted_sample_day_d50_bg_ratio;
  float    weighted_sample_fah_rg_ratio;
  float    weighted_sample_fah_bg_ratio;
  float    white_rg_ratio;
  float    white_bg_ratio;
  int      white_stat_y_threshold_low;
  int      unsat_y_min_threshold;
  int      unsat_y_max;
  int      unsat_y_mid;
  int      unsat_y_day_max;
  int      unsat_y_f_max;
  int      unsat_y_a_max;
  int      unsat_y_h_max;
  float    sat_day_rg_ratio;
  float    sat_day_bg_ratio;
  int      sat_day_cluster;
  float    sat_f_rg_ratio;
  float    sat_f_bg_ratio;
  int      sat_f_cluster;
  float    sat_a_rg_ratio;
  float    sat_a_bg_ratio;
  int      sat_a_cluster;
  float    sat_h_rg_ratio;
  float    sat_h_bg_ratio;
  int      sat_h_cluster;
  float    max_compact_cluster;
  int      count_extreme_b_mcc;
  int      green_zone_right2;
  int      green_line_bx2;
  int      green_zone_bottom2;
  int      output_is_confident;
  int      output_sample_decision;
  float    output_wb_gain_r;
  float    output_wb_gain_g;
  float    output_wb_gain_b;
  float    regular_ave_rg_ratio;
  float    regular_ave_bg_ratio;
  float    cct_awb_bayer;
  int      count_extreme_b;
  float    r_gain;
  float    g_gain;
  float    b_gain;
  int      color_temp;
  int      decision;
  int      samp_decision[64];
  boolean  lock;
} awb_output_eztune_data_t;

/** _awb_output_data
 *
 **/
typedef struct _awb_output_data {
  stats_update_t  stats_update;
  float  r_gain;
  float  g_gain;
  float  b_gain;
  int    color_temp;
  int    awb_update;
  int    decision;
  int    samp_decision[64];
  int    wb_mode;
  int    best_mode;
  uint32_t frame_id;
  awb_output_type_t type;
  awb_output_eztune_data_t eztune_data;
} awb_output_data_t;
/*Data structure for awb ends */

typedef boolean (* awb_set_parameters_func)(awb_set_parameter_t *param,
  void *awb_obj);

typedef boolean (* awb_get_parameters_func)(awb_get_parameter_t *param,
  void *awb_obj);

typedef void    (* awb_process_func)(stats_t *stats,
  void *awb_obj, awb_output_data_t *output);

typedef struct {
  awb_set_parameters_func set_parameters;
  awb_get_parameters_func get_parameters;
  awb_process_func        process;
}awb_ops_t;

void *awb_init( awb_ops_t *awb_ops);
void awb_destroy(void *awb_obj);
#endif /* __AWB_H__ */
