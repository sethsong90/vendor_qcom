/* sensor_common.c
 *
 * Copyright (c) 2012-2013 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */


/* This file will be used to place all data structures and enums using which
   sensor sub modules -  sensor actuator, eeprom, flash, csid, csiphy will
   interact among themselves */

#ifndef __SENSOR_COMMON_H__
#define __SENSOR_COMMON_H__

#include <media/msm_cam_sensor.h>
#include "mct_list.h"
#include "mct_bus.h"
#include "sensor_dbg.h"
#include "cam_intf.h"
#include "mtype.h"
#include "sensor_lib.h"
#include "chromatix.h"
#include "chromatix_common.h"
#include "af_tuning.h"
#include "mct_stream.h"

#define SENSOR_SUCCESS 0
#define SENSOR_FAILURE -1
#define SENSOR_ERROR_IO -2
#define SENSOR_ERROR_NOMEM -3
#define SENSOR_ERROR_INVAL -4

#define MAX_SUBDEV_SIZE 32

typedef enum {
  /* Sensor framework enums */
  /* Get enums */
  SENSOR_GET_CAPABILITIES, /* struct msm_camera_csiphy_params * */
  SENSOR_GET_CUR_CSIPHY_CFG, /* struct msm_camera_csiphy_params * */
  SENSOR_GET_CUR_CSID_CFG, /* struct msm_camera_csid_params * */
  SENSOR_GET_CUR_CHROMATIX_NAME, /* char * */
  SENSOR_GET_CSI_LANE_PARAMS, /* struct csi_lane_params_t * */
  SENSOR_GET_CUR_FPS, /* uint32_t * */
  SENSOR_GET_RESOLUTION_INFO, /* */
  SENSOR_GET_RES_CFG_TABLE, /* struct sensor_res_cfg_table_t * */
  SENSOR_GET_SENSOR_PORT_INFO, /* struct sensor_port_into_array_t * */
  SENSOR_GET_DIGITAL_GAIN, /* float * */
  SENSOR_GET_SENSOR_FORMAT, /* sensor_output_format_t * */
  SENSOR_GET_WAIT_FRAMES,
  SENSOR_GET_LENS_INFO, /* sensor_lens_info_t */
  SENSOR_GET_RAW_DIMENSION, /* cam_dimension_t * */
  /* Set enums */
  SENSOR_SET_LIB_PARAMS, /* sensor_lib_params_t * */
  SENSOR_SET_INIT_PARAMS, /* struct msm_sensor_init_params * */
  SENSOR_SET_SUBDEV_INFO, /* struct msm_sensor_info_t * */
  SENSOR_INIT, /* NULL */
  SENSOR_STOP_STREAM, /* NULL */
  SENSOR_START_STREAM, /* NULL */
  SENSOR_SET_RESOLUTION, /* sensor_set_res_cfg_t * */
  SENSOR_SET_AEC_UPDATE, /* sensor_set_aec_data_t * */
  SENSOR_SET_AEC_INIT_SETTINGS, /* aec_get_t */
  SENSOR_SET_VFE_SOF, /* NULL */
  SENSOR_SET_FPS, /* uint16_t */
  SENSOR_SET_HFR_MODE, /* cam_hfr_mode_t */
  SENSOR_SET_WAIT_FRAMES,
  SENSOR_SET_HDR_AE_BRACKET, /* cam_exp_bracketing_t* */
  SENSOR_SET_VIDEO_HDR_ENABLE, /* int32_t * */
  SENSOR_SET_DIS_ENABLE, /* int32_t * */
  SENSOR_SET_OP_PIXEL_CLK_CHANGE, /* uint32_t * */
  SENSOR_SET_CALIBRATION_DATA, /* msm_camera_i2c_reg_setting * */
  SENSOR_SET_CONTRAST,
  SENSOR_SET_AUTOFOCUS,
  SENSOR_CANCEL_AUTOFOCUS,
  SENSOR_SET_SATURATION,
  SENSOR_SET_BRIGHTNESS,
  SENSOR_SET_SHARPNESS,
  SENSOR_SET_ISO,
  SENSOR_SET_EXPOSURE_COMPENSATION,
  SENSOR_SET_ANTIBANDING,
  SENSOR_SET_BESTSHOT_MODE,
  SENSOR_SET_EFFECT,
  SENSOR_SET_WHITE_BALANCE,
  SENSOR_SET_MAX_DIMENSION, /* cam_dimension_t * */
  /* End of Sensor framework enums */

  /* chromatix enums*/
  /* Get enums */
  CHROMATIX_GET_PTR, /* sensor_chromatix_params_t * */
  /* Set enums */
  CHROMATIX_OPEN_LIBRARY, /* const char * */
  CHROMATIX_OPEN_LIVESHOT_LIBRARY, /* sensor_chromatix_name_t * */
  CHROMATIX_CLOSE_LIVESHOT_LIBRARY, /* NULL */
  /* End of Actuator enums*/

  /* Actuator enums*/
  /* Get enums */
  ACTUATOR_GET_AF_TUNE_PTR, /* af_tune_parms_t ** */
  /* Set enums */
  ACTUATOR_INIT, /* NULL */
  ACTUATOR_MOVE_FOCUS, /* af_update_t * */
  ACTUATOR_LOAD_HEADER,
  ACTUATOR_SET_PARAMETERS,
  ACTUATOR_FOCUS_TUNING,
  /* End of Actuator enums*/

  /* EEPROM enums*/
  /* Get enums */
  EEPROM_READ_DATA, /*const char **/
  EEPROM_SET_BYTESTREAM,
  EEPROM_SET_CHROMA_AF_PTR,
  EEPROM_GET_ISINSENSOR_CALIB,
  EEPROM_GET_RAW_DATA,
  /* Set enums */
  EEPROM_OPEN_FD, /*const char **/
  EEPROM_CLOSE_FD, /*const char **/
  EEPROM_WRITE_DATA, /*const char **/
  /* End of EEEPROM enums*/
  /* LED flash enums*/
  /* Get enums */
  LED_FLASH_GET_RER_CHROMATIX,  /* NULL */
  /* Set enums */
  LED_FLASH_SET_RER_PARAMS,     /* NULL */
  LED_FLASH_SET_RER_PROCESS,    /* NULL */
  LED_FLASH_SET_OFF, /* NULL */
  LED_FLASH_SET_LOW, /* NULL */
  LED_FLASH_SET_HIGH, /* NULL */
  LED_FLASH_SET_RED_EYE_REDUCTION,
  LED_FLASH_GET_RED_EYE_CHROMATIX,
  LED_FLASH_SET_RED_EYE_SEQUENCE_PROCESS,
  /* End of LED flash enums*/

  /* Strobe flash enums*/
  /* Get enums */
  /* Set enums */
  /* End of Strobe flash enums*/

  /* CSIPHY enums*/
  /* Get enums */
  /* Set enums */
  CSIPHY_SET_LANE_PARAMS, /* struct csi_lane_params_t * */
  CSIPHY_SET_CFG, /* struct msm_camera_csiphy_params * */
  /* End of CSIPHY enums*/

  /* CSID enums*/
  CSID_GET_VERSION, /* uint32_t * */
  /* Get enums */
  /* Set enums */
  CSID_SET_LANE_PARAMS, /* struct csi_lane_params_t * */
  CSID_SET_CFG, /* struct msm_camera_csid_params * */
  /* End of CSID enums*/
  /* video hdr enums */
  SENSOR_SET_AWB_UPDATE, /* sensor_set_awb_data_t * */
} sensor_submodule_event_type_t;

typedef enum {
  ACTUATOR_TUNE_RELOAD_PARAMS,
  ACTUATOR_TUNE_TEST_LINEAR,
  ACTUATOR_TUNE_TEST_RING,
  ACTUATOR_TUNE_DEF_FOCUS,
  ACTUATOR_TUNE_MOVE_FOCUS,
}actuator_tuning_type_t;

typedef struct {
  /* Open func for sub module
     1st param -> Address of function table to be filled by sub module
     2nd param -> kernel sub device name
     return status -> success / failure */
  int32_t (*open)(void **, const char *);
  /* Set param for sub module
     1st param -> module pointer
     2nd param -> event type
     3rd param -> private data
     return status -> success / failure */
  int32_t (*process)(void *, sensor_submodule_event_type_t, void *);
  /* close func for sub module
     1st param -> module pointer
     return status -> success / failure */
  int32_t (*close)(void *);
} sensor_func_tbl_t;

typedef struct {
  chromatix_parms_type *chromatix_ptr;
  chromatix_VFE_common_type *common_chromatix_ptr;
  chromatix_parms_type *liveshot_chromatix_ptr;
  chromatix_parms_type *snapchromatix_ptr;
  uint32_t stream_mask;
  uint8_t chromatix_reloaded;
  uint8_t chromatix_common_reloaded;
} sensor_chromatix_params_t;

typedef struct {
  chromatix_parms_type *ls_chromatix_ptr;
} sensor_ls_chromatix_params_t;

typedef struct {
  uint32_t width;
  uint32_t height;
  uint32_t stream_mask;
} sensor_set_res_cfg_t;

typedef struct {
  char *chromatix;
  char *common_chromatix;
  char *liveshot_chromatix;
  char *snapshot_chromatix;
} sensor_chromatix_name_t;

typedef union {
  struct msm_camera_csiphy_params *csiphy_params;
  struct msm_camera_csid_params   *csid_params;
  sensor_chromatix_name_t          chromatix_name;
  struct csi_lane_params_t        *csi_lane_params;
} sensor_get_t;

typedef struct {
  sensor_func_tbl_t func_tbl;
  void *sub_module_private;
} module_sensor_params_t;

typedef struct {
  void         *sensor_lib_handle;
  sensor_lib_t *sensor_lib_ptr;
} sensor_lib_params_t;

typedef enum {
    ACTUATOR_CAM_MODE_CAMERA,
    ACTUATOR_CAM_MODE_CAMCORDER,
    ACTUATOR_CAM_MODE_MAX,
} actuator_cam_mode_t;

typedef struct {
  sensor_chromatix_params_t chromatix;
  af_tune_parms_t *af_tune_ptr;
} eeprom_set_chroma_af_t;

typedef struct {
  uint8_t is_supported;
  char eeprom_name[32];
  uint8_t *buffer;
  uint32_t num_bytes;
} eeprom_params_t;

typedef struct {
  void (*get_calibration_items)(void *);
  void (*format_calibration_data)(void *);
  void (*get_dpc_calibration_info) (void *, int, void *);
  void (*do_af_calibration) (void *);
  void (*do_wbc_calibration) (void *);
  void (*do_lsc_calibration) (void *);
  void (*do_dpc_calibration) (void *);
  int32_t (*get_raw_data) (void *, void *);
} eeprom_lib_func_t;

typedef struct {
  void *eeprom_lib_handle;
  eeprom_lib_func_t *func_tbl;
} eeprom_lib_params_t;

typedef struct {
  int is_insensor;
  int is_afc;
  int is_wbc;
  int is_lsc;
  int is_dpc;
} eeprom_calib_items_t;

typedef struct {
  int x;
  int y;
} pixel_t;

typedef struct {
  pixel_t *pix;
  int count;
} pixels_array_t;

typedef struct {
  float r_over_g[AGW_AWB_MAX_LIGHT];
  float b_over_g[AGW_AWB_MAX_LIGHT];
  float gr_over_gb;
} wbcalib_data_t;

typedef struct {
  uint16_t macro_dac;
  uint16_t infinity_dac;
  uint16_t starting_dac;
} afcalib_data_t;

typedef struct {
  mesh_rolloff_array_type lsc_calib[ROLLOFF_MAX_LIGHT];
} lsccalib_data_t;

typedef struct {
  uint16_t validcount;
  pixel_t snapshot_coord[256];
  pixel_t preview_coord[256];
  pixel_t video_coord[256];
} dpccalib_data_t;

typedef struct {
  eeprom_calib_items_t items;
  wbcalib_data_t wbc;
  afcalib_data_t afc;
  lsccalib_data_t lsc;
  dpccalib_data_t dpc;
} format_data_t;

typedef struct {
  int fd;
  eeprom_params_t eeprom_params;
  eeprom_set_chroma_af_t eeprom_afchroma;
  eeprom_lib_params_t eeprom_lib;
  format_data_t eeprom_data;
} sensor_eeprom_data_t;

typedef struct {
  void     *raw_dim;
  uint32_t  stream_mask;
} sensor_get_raw_dimension_t;

typedef struct {
  /* Entity to store subdev name for all imager modules */
  char sensor_sd_name[SUB_MODULE_MAX][MAX_SUBDEV_SIZE];
  /* Entity to store sensor name and index of its sub modules */
  struct msm_sensor_info_t      *sensor_info;
  /* Entity to store sensor init params */
  struct msm_sensor_init_params *sensor_init_params;
  /* Sensor library params */
  sensor_lib_params_t           *sensor_lib_params;
  /* Sensor module params, these params will to be created and destroyed
     during sensor open and release */
  module_sensor_params_t        *module_sensor_params[SUB_MODULE_MAX];
  /* stream ref count */
  uint16_t                       ref_count;
  /* max width of all streams */
  int32_t                        max_width;
  /* max height of all streams */
  int32_t                        max_height;
  /* stream mask of all streams */
  uint32_t                       stream_mask;
  /* refcount for non bundle stream on / off */
  int32_t                        stream_on_count;
  uint32_t                       last_idx;
  uint16_t                       num_skip;
  /* 1 started, 2, done, 3 done no led */
  int32_t                        state;
  int32_t                        regular_led_trigger;
  int32_t                        regular_led_af;
  /* store chromatix pointers to post to bus */
  mct_bus_msg_sensor_metadata_t    chromatix_metadata;
  /* store trigger update to post to bus */
  mct_bus_msg_stats_aec_metadata_t aec_metadata;
  sensor_eeprom_data_t            *eeprom_data;
  /* Store sensor_params to post to bus */
  cam_sensor_params_t            sensor_params;
  int32_t                        torch_on;
  int32_t                        longshot;
  cam_fps_range_t                fps_info;
} module_sensor_bundle_info_t;

typedef struct {
  module_sensor_bundle_info_t *s_bundle;
  uint32_t                     session_id;
  uint32_t                     stream_id;
} sensor_bundle_info_t;

typedef struct {
  /* Sensor module information for each sensor */
  mct_list_t                 *sensor_bundle; /* module_sensor_bundle_info_t * */
  /* Number of sensor bundle information - one per sensor successfully probed */
  uint8_t                     size;
  mct_stream_info_t           streaminfo;
  int32_t                     session_count;
  int                         pfd[2];
} module_sensor_ctrl_t;

int32_t sensor_load_library(const char *name, void *sensor_lib_params);
int32_t sensor_unload_library(sensor_lib_params_t *sensor_lib_params);

int32_t sensor_sub_module_init(sensor_func_tbl_t *func_tbl);
int32_t chromatix_sub_module_init(sensor_func_tbl_t *func_tbl);
int32_t eeprom_sub_module_init(sensor_func_tbl_t *func_tbl);
int32_t actuator_sub_module_init(sensor_func_tbl_t *func_tbl);
int32_t led_flash_sub_module_init(sensor_func_tbl_t *func_tbl);
int32_t strobe_flash_sub_module_init(sensor_func_tbl_t *func_tbl);
int32_t csiphy_sub_module_init(sensor_func_tbl_t *func_tbl);
int32_t csid_sub_module_init(sensor_func_tbl_t *func_tbl);
#endif
