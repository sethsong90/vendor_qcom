/*============================================================================

  Copyright (c) 2012-2013 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

============================================================================*/
#ifndef __AF_TUNING_H__
#define __AF_TUNING_H__

#include <media/msm_cam_sensor.h>


/******************************************************************************
AF Algorithm tuning parameters
******************************************************************************/
/** af_algo_type: Type of algorithm currently supported
**/
typedef enum {
  AF_PROCESS_DEFAULT   = -2,
  AF_PROCESS_UNCHANGED = -1,
  AF_EXHAUSTIVE_SEARCH = 0,
  AF_EXHAUSTIVE_FAST,
  AF_HILL_CLIMBING_CONSERVATIVE,
  AF_HILL_CLIMBING_DEFAULT,
  AF_HILL_CLIMBING_AGGRESSIVE,
  AF_FULL_SWEEP,
  AF_SLOPE_PREDICTIVE_SEARCH,
  AF_CONTINUOUS_SEARCH,
  AF_PROCESS_MAX
} af_algo_type;

/** _af_tuning_sp: AF tuning parameters specific to slope
*   predictive algorithm.

*    @fv_curve_flat_threshold: threshold to determine if FV
*                               curve is flat (def: 0.9)
*
*    @slope_threshold1: sp thresholds 1 (def: 0.9)

*    @slope_threshold2: sp thresholds 2 (def: 1.1)
*
*    @slope_threshold3: sp thresholds 3 (def: 0.5)
*
*    @slope_threshold4: sp thresholds 4 (def: 3)
*
*    @lens_pos_0: Lens poisiton when the object is at 3m
*
*    @lens_pos_1: Lens poisiton when the object is at 70cm
*
*    @lens_pos_2: Lens poisiton when the object is at 30cm
*
*    @lens_pos_3: Lens poisiton when the object is at 20cm
*
*    @lens_pos_4: Lens poisiton when the object is at 10cm
*
*    @lens_pos_5: Lens poisiton Macro
*
*    @base_frame_delay: sp frame delay (def: 1)
*
*    @downhill_allowance: max number of consecutive
*                             downhill in the first 4 or 6
*                             samples
*
*    @downhill_allowance_1: max number of consecutive
*                            downhill in 2 or 3 round

**/
typedef struct _af_tuning_sp {
  float fv_curve_flat_threshold;
  float slope_threshold1;
  float slope_threshold2;
  float slope_threshold3;
  float slope_threshold4;
  unsigned int lens_pos_0;
  unsigned int lens_pos_1;
  unsigned int lens_pos_2;
  unsigned int lens_pos_3;
  unsigned int lens_pos_4;
  unsigned int lens_pos_5;
  unsigned int base_frame_delay;
  int downhill_allowance;
  int downhill_allowance_1;
} af_tuning_sp_t;

/** _af_tuning_gyro: AF tuning parameters specific to gyro
 *  support.
 *
 *  @enable: enable/disable gyro assisted caf
 *
 *  @min_movement_threshold:  above this device is supposed
 *                         to be moving.
 *
 *  @stable_detected_threshold: less than this, device is
 *                            stable.
 *
 *  @fast_pan_threshold: above this threshold means we are
 *                     doing fast panning.
 *
 *  @slow_pan_threshold: below this panning is slow.
 *
 *  @fast_pan_count_threshold: number of frames device was
 *                           panning fast.
 *
 *  @sum_return_to_orig_pos_threshold:  if less than this
 *                                   threshold, device hasn't
 *                                   moved much from starting
 *                                   position.
 *
 *  @stable_count_delay: number of frames we need to be
 *                     stable after panning. */
typedef struct _af_tuning_gyro {
  boolean enable;
  float min_movement_threshold;
  float stable_detected_threshold;
  float fast_pan_threshold;
  float slow_pan_threshold;
  unsigned short fast_pan_count_threshold;
  unsigned short sum_return_to_orig_pos_threshold;
  unsigned short stable_count_delay;
} af_tuning_gyro_t;


/** _af_tuning_sad: AF tuning parameters specific to Sum of
 *  Absolute Difference (SAD) scene detection.
 *
 *  @enable: enable/disable sad scene detection
 *
 *  @gain_min: minimum gain
 *
 *  @gain_max: maximum gain
 *
 *  @ref_gain_min: minimum referece gain
 *
 *  @ref_gain_max: maximum reference gain
 *
 *  @threshold_min: threshold when current gain is less than
 *                minimum gain
 *
 *  @threshold_max: threshold when current gain is more than
 *                maximum gain.
 *
 *  @ref_threshold_min: threshold when current gain is less than
 *                minimum reference gain
 *
 *  @ref_threshold_max: threshold when current gain is more than
 *                maximum reference gain   */
typedef struct _af_tuning_sad {
  boolean enable;
  float gain_min;
  float gain_max;
  float ref_gain_min;
  float ref_gain_max;
  unsigned short threshold_min;
  unsigned short threshold_max;
  unsigned short ref_threshold_min;
  unsigned short ref_threshold_max;
  unsigned short frames_to_wait;
} af_tuning_sad_t;


/** _af_tuning_continuous: AF tuning parameters specific to
 *  Continuous AF
 *
 *  @enable: CAF is enabled/disabled (currently only used by
 *         ez-tune)
 *
 *  @scene_change_detection_ratio: fv change to trigger a
 *                                  target change
 *
 *  @panning_stable_fv_change_trigger: fv change vs past
 *                                      frame fv to trigger to
 *                                      determine if scene is
 *                                      stable
 *
 *  @panning_stable_fvavg_to_fv_change_trigger: fv change vs
 *                                               average of 10
 *                                               past frame's FV
 *
 *  @panning_unstable_trigger_cnt: how many panning unstabl
 *                                  detections before triggering
 *                                  a scene change.
 *
 *  @downhill_allowance: number of extra steps to search once
 *                     peak FV is found
 *
 *  @uphill_allowance: number of steps to move if FV keeps
 *                   increasing.
 *
 *  @base_frame_delay: number of frames to skip after lens
 *                   movement.
 *
 *  @lux_index_change_threshold: threshold above which a change
 *                             in lux index will trigger new
 *                             search.
 *
 *  @threshold_in_noise: determine if the fv variaion is due to
 *                     noise.
 *
 *  @search_step_size: size of each steps
 *
 *  @init_search_type: search algorithm to use before entering
 *                   monitor mode.
 *
 *  @search_type: search algorithm to use after monitor mode
 *              decides new search is required.
 *
 *  @ low_light_wait: how many frames to skip when in low light
 *
 *  @max_indecision_cnt: maximum number of times allowed to stay
 *                     in make_decision if it's not clear which
 *                     way to go.
 *
 *  @af_sp: af parameters for slope-predictive algorithm
 *
 *  @af_gyro: gyro parameters to assist AF
 **/
typedef struct _af_tuning_continuous {
  boolean enable;
  unsigned char   scene_change_detection_ratio;
  float           panning_stable_fv_change_trigger;
  float           panning_stable_fvavg_to_fv_change_trigger;
  unsigned short  panning_unstable_trigger_cnt;
  unsigned short  scene_change_trigger_cnt;
  unsigned long   downhill_allowance;
  unsigned short  uphill_allowance;
  unsigned short  base_frame_delay;
  unsigned short  lux_index_change_threshold;
  float           noise_level_th;
  unsigned short  search_step_size;
  unsigned short  init_search_type;
  unsigned short  search_type;
  unsigned short  low_light_wait;
  unsigned short  max_indecision_cnt;
  float flat_fv_confidence_level;
  float base_delay_adj_th;
  af_tuning_sad_t af_sad;
  af_tuning_gyro_t af_gyro;
}af_tuning_continuous_t;


/** _af_tuning_exhaustive: AF tuning parameters specific to
 *  exhaustive search AF
 *
 *  @num_gross_steps_between_stat_points:
 *  Used to control how rough initial AF search is.
 *
 *  @num_fine_steps_between_stat_points:
 *  control how precise fine search is
 *
 *  @num_fine_search_points: how many search points to gather in
 *                         fine search.
 *
 *  @downhill_allowance: number of extra steps to search once
 *                     peak FV is found
 *
 *  @uphill_allowance: number of steps to move if FV keeps
 *                   increasing.
 *
 *  @base_frame_delay: how many frames to delay after each lens movement
 *
 *  @coarse_frame_delay: how many frames to delay after lens movement in
 *                       coarse search
 *
 *  @fine_frame_delay: how many frames to delay after lens movement in
 *                     fine search
 *
 *  @coarse_to_fine_frame_delay: how many frames to delay after lens reaching
 *                               the end position for the coarse search and
 *                               before starting the fine search
 *
 *  @enable_multiwindow: weather to enable multiwindow AF
 *
 *  @mw_thresh_flat: The threshold to detect flat curve in multiwindow AF
 *
 *  @mw_thresh_1: Determine slope increase or decrease.
 *
 *  @mw_thresh_2: Determine last slope increase or decrease.
 *
 *  @mw_thresh_3: Determine super steap slope increase or decrease.
 *
 *  @gain_thresh: This will tell the gain beyond which the multiwindow
 *                AF should work if enabled
 **/
typedef struct _af_tuning_exhaustive {
  unsigned short num_gross_steps_between_stat_points;
  unsigned short num_fine_steps_between_stat_points;
  unsigned short num_fine_search_points;
  unsigned short downhill_allowance;
  unsigned short uphill_allowance;
  unsigned short base_frame_delay;
  unsigned short coarse_frame_delay;
  unsigned short fine_frame_delay;
  unsigned short coarse_to_fine_frame_delay;
  float noise_level_th;
  float flat_fv_confidence_level;
  float cum_fv_ratio_th;
  int low_light_luma_th;
  int            enable_multiwindow;
  float          mw_thresh_flat;
  float          mw_thresh_1;
  float          mw_thresh_2;
  float          mw_thresh_3;
  float          gain_thresh;
}af_tuning_exhaustive_t;

/** _af_tuning_fullsweep_t: AF tuning parameters specific to
 *  full sweep search AF
 *
 *  @num_steps_between_stat_points:
 *  Used to control how many steps to move the lens at a time during search.
 *
 *  @frame_delay_inf: how many frames to delay after resetting the lens in
 *                    initial position.
 *
 *  @frame_delay_norm: how many frames to delay after each lens movement
 *
 *  @frame_delay_final: how many frames to delay after reaching max position
 **/
typedef struct _af_tuning_fullsweep_t {
  unsigned short num_steps_between_stat_points;
  unsigned short frame_delay_inf;
  unsigned short frame_delay_norm;
  unsigned short frame_delay_final;
}af_tuning_fullsweep_t;

/** _af_shake_resistant: AF tuning parameters specific to af
 *  shake resistant.
 *
 * @enable: true if enabled
 *
 * @max_gain:
 *
 * @min_frame_luma:
 *
 * @tradeoff_ratio:
 *
 * toggle_frame_skip:
 **/
typedef struct _af_shake_resistant {
  boolean enable;
  float max_gain;
  unsigned char min_frame_luma;
  float tradeoff_ratio;
  unsigned char toggle_frame_skip;
}af_shake_resistant_t;


/** _af_motion_sensor: AF tuning parameters specific to af
 *  motion sensor. Controls how each component affect af scene
 *  change detection. Bigger the value less sensitive AF
 *  response is.
 *
 *  @af_gyro_trigger: control how sensitive AF is to gyro
 *                  trigger.
 *
 *  @af_accelerometer_trigger: controls how sensitive AF is to
 *                           accelerometer.
 *
 *  @af_magnetometer_trigger: controls how sensistive AF is to
 *                          magnetometer.
 *
 *  @af_dis_motion_vector_trigger: controls how sensitive AF is
 *                               to DIS motion vector.
 **/
typedef struct _af_motion_sensor {
  float af_gyro_trigger;
  float af_accelerometer_trigger;
  float af_magnetometer_trigger;
  float af_dis_motion_vector_trigger;
}af_motion_sensor_t;

/** _af_tuning_algo: AF tuning parameters specific to AF
 *  algorithm.
 *
 *  @af_process_type: af algorithm type used
 *
 *  @position_near_end: nearest position lens can move to
 *
 *  @position_default_in_macro: default lens rest position when
 *                            focus mode is Macro.
 *
 *  @position_boundary: determines near end of search range for
 *                    Normal focus mode.
 *
 *  @position_default_in_normal: default lens reset position
 *                             when focus mode is other than
 *                             macro.
 *
 *  @position_far_end: farthest point of the search range
 *
 *  @position_normal_hyperfocal: normal position of the lens when
 *                               focus fails
 *
 *  @undershoot_protect: when enabled, lens will be moved more
 *                     in either forward or backward direction.
 *  @undershoot_adjust: when undershoot_protect is enabled, lens
 *                    movement is adjusted by this amount.
 *
 *  @fv_drop_allowance: amount by which fv is allowed to drop
 *                    below max
 *
 *  @lef_af_assist_enable: enable/disable led assisted AF
 *
 *  @led_af_assist_trigger_idx: Lux Index<A0>at which LED assist
 *                            for autofocus is enabled.
 *
 *  @af_tuning_continuous_t: af parameters for continuous focus
 *
 *  @af_exh: af parameters for exhaustive af
 *
 *  @af_sad: sad related tuning parameters
 *
 *  @af_shake_resistant: tuning parameters for af shake
 *                     resistant.
 *
 *  @af_motion_sensor: trigger parameters for af motion sensor.
 **/
typedef struct _af_tuning_algo {
  unsigned short af_process_type;
  unsigned short position_near_end;
  unsigned short position_default_in_macro;
  unsigned short position_boundary;
  unsigned short position_default_in_normal;
  unsigned short position_far_end;
  unsigned short position_normal_hyperfocal;
  unsigned short position_macro_rgn;
  unsigned short undershoot_protect;
  unsigned short undershoot_adjust;
  float fv_drop_allowance;
  int lef_af_assist_enable;
  long led_af_assist_trigger_idx;
  int lens_reset_frame_skip_cnt;
  float low_light_gain_th;
  af_tuning_continuous_t af_cont;
  af_tuning_exhaustive_t af_exh;
  af_tuning_fullsweep_t af_full;
  af_tuning_sp_t af_sp;
  af_shake_resistant_t af_shake_resistant;
  af_motion_sensor_t af_motion_sensor;
}af_tuning_algo_t;

/******************************************************************************
AF Stats tuning parameters
******************************************************************************/
/** _af_vfe_config: vfe configuration info
**/
typedef struct _af_vfe_config {
  unsigned short fv_min;
  unsigned short max_h_num;
  unsigned short max_v_num;
  unsigned short max_block_width;
  unsigned short max_block_height;
  unsigned short min_block_width;
  unsigned short min_block_height;
  float h_offset_ratio_normal_light;
  float v_offset_ratio_normal_light;
  float h_clip_ratio_normal_light;
  float v_clip_ratio_normal_light;
  float h_offset_ratio_low_light;
  float v_offset_ratio_low_light;
  float h_clip_ratio_low_light;
  float v_clip_ratio_low_light;
  float touch_roi_scaling_factor;
} af_vfe_config_t;

/** _af_vfe_legacy_hpf: high pass filter coefficients for
 *  legacy YUV stats
 **/
typedef struct _af_vfe_legacy_hpf {
  char      a00;
  char      a02;
  char      a04;
  char      a20;
  char      a21;
  char      a22;
  char      a23;
  char      a24;
} af_vfe_legacy_hpf_t;

/** _af_vfe_bayer_hpf: high pass filter coefficients for
 *  bayer stats
 **/
typedef struct _af_vfe_bayer_hpf {
  char      a00;
  char      a01;
  char      a02;
  char      a03;
  char      a04;
  char      a10;
  char      a11;
  char      a12;
  char      a13;
  char      a14;
} af_vfe_bayer_hpf_t;

/** _af_vfe_hpf: high pass filter coefficients.
 *  @af_hpf: hpf for yuv
 *  @bf_hpf: hpf for bayer
 **/
typedef struct _af_vfe_hpf {
  af_vfe_legacy_hpf_t af_hpf;
  af_vfe_bayer_hpf_t bf_hpf;
} af_vfe_hpf_t;

/** _af_tuning_vfe: tuning parameters for AF stats.
 *  @af_config: vfe configuration info
 *
 *  @af_fv_metric: fv metric (0: sum of FV  1: max of FV)
 *
 *  @af_hpf: high pass filter coefficients
 *
 **/
typedef struct _af_tuning_vfe {
  unsigned short fv_metric;
  af_vfe_config_t config;
  af_vfe_hpf_t hpf;
} af_tuning_vfe_t;


/******************************************************************************
Actuator parameters
******************************************************************************/
/** msm_actuator_reg_tbl
**/
struct msm_actuator_reg_tbl_t {
  uint8_t reg_tbl_size;
  struct msm_actuator_reg_params_t reg_params[MAX_ACTUATOR_REG_TBL_SIZE];
};

/** damping_t:
 **/
struct damping_t {
  struct damping_params_t ringing_params[MAX_ACTUATOR_REGION];
};

/** _actuator_tuned_params:
 **/
typedef struct _actuator_tuned_params {
  uint16_t scenario_size[NUM_ACTUATOR_DIR];
  uint16_t ringing_scenario[NUM_ACTUATOR_DIR][MAX_ACTUATOR_SCENARIO];
  int16_t initial_code;
  uint16_t region_size;
  struct region_params_t region_params[MAX_ACTUATOR_REGION];
  struct damping_t damping[NUM_ACTUATOR_DIR][MAX_ACTUATOR_SCENARIO];
} actuator_tuned_params_t;

/** _actuator_params:
 **/
typedef struct _actuator_params {
  uint32_t i2c_addr;
  enum msm_actuator_data_type i2c_data_type;
  enum msm_actuator_addr_type i2c_addr_type;
  enum actuator_type act_type;
  uint16_t data_size;
  uint8_t af_restore_pos;
  struct msm_actuator_reg_tbl_t reg_tbl;
  uint16_t init_setting_size;
  struct reg_settings_t init_settings[MAX_ACTUATOR_INIT_SET];
} actuator_params_t;


/******************************************************************************
Main AF parameter structure
******************************************************************************/

/** _af_header_info:
 **/
typedef struct _af_header_info {
  uint16_t header_version;
  enum af_camera_name cam_name;
  char module_name[MAX_ACT_MOD_NAME_SIZE];
  char actuator_name[MAX_ACT_NAME_SIZE];
}af_header_info_t;


/** _af_tune_parms: Main tuning parameters exposed to outer
 *  world.
 * @af_header_info: version information
 *
 * @af_header_info: version information
 *
 * @af_algo: AF parameters specific to AF algorithm
 *
 * @af_vfe: AF parameters specific to VFE stats configuration
 *
 * @actuator_params: parameters specific to actuator
 *
 * @actuator_tuned_params:
**/
typedef struct _af_tune_parms {
  af_header_info_t af_header_info;
  af_tuning_algo_t af_algo;
  af_tuning_vfe_t af_vfe;
  actuator_params_t actuator_params;
  actuator_tuned_params_t actuator_tuned_params;
} af_tune_parms_t;

#endif /* __AF_TUNING_H__ */
