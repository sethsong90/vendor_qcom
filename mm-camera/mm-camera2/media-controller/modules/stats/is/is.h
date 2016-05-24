/* is.h
 *
 * Copyright (c) 2013 - 2014 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef __IS_H__
#define __IS_H__
#include "dis_interface.h"
#include "eis_interface.h"
#include "eis2_interface.h"


/** _is_info:
 *    @is_mode: IS method (DIS, gyro-assisted DIS, EIS 1.0)
 *    @timestamp: RS/CS stats timestamp
 *    @rs_cs_data: RS/CS stats
 *    @dis_context: DIS context
 *    @eis_context: EIS 1.0 context
 *    @vfe_width: VFE width (image width + margin)
 *    @vfe_height: VFE height (image height + margin)
 *    @width: image width
 *    @height: image height
 *    @preview_width: preview width
 *    @preview_height: preview height
 *    @num_row_sum: number of row sums
 *    @num_col_sum: number of column sums
 *    @sensor_mount_angle: sensor mount angle (0, 90, 180, 270)
 *    @camera_position: camera position (front or back)
 *    @is_enabled: Indicates whether IS is enabled
 *    @is_inited: Indicates whether IS has been initialized
 *    @sns_lib_offset_set: Indicates whether time offset of gyro sensor library
 *                         has been set.
 *
 * This structure represents the IS internal variables.
 **/
typedef struct _is_info {
  cam_is_type_t is_mode;
  struct timeval timestamp;
  rs_cs_data_t rs_cs_data;
  dis_context_type dis_context;
  eis_context_type eis_context;
  eis2_context_type eis2_context;
  unsigned int transform_type;
  unsigned long vfe_width;
  unsigned long vfe_height;
  unsigned long width;
  unsigned long height;
  unsigned long preview_width;
  unsigned long preview_height;
  unsigned int num_row_sum;
  unsigned int num_col_sum;
  unsigned int sensor_mount_angle;
  enum camb_position_t camera_position;
  unsigned int is_enabled;
  unsigned int is_inited;
  unsigned int sns_lib_offset_set;
} is_info_t;


typedef enum {
  IS_PROCESS_STREAM_EVENT = 1,
  IS_PROCESS_RS_CS_STATS
} is_process_parameter_type;


typedef enum {
  IS_VIDEO_STREAM_OFF = 1,
  IS_VIDEO_STREAM_ON
} is_stream_event_t;


typedef struct _is_stream_event_data {
  is_stream_event_t stream_event;
  is_info_t *is_info;
} is_stream_event_data_t;


typedef struct _is_stats_data {
  int run_is;
  unsigned int frame_id;
  unsigned int identity;
  mct_port_t *port;
  is_info_t *is_info;
} is_stats_data_t;


typedef struct _is_process_parameter {
  is_process_parameter_type type;

  union {
    is_stream_event_data_t stream_event_data;
    is_stats_data_t stats_data;
  } u;
} is_process_parameter_t;


/** is_process_output_type
 **/
typedef enum {
  IS_PROCESS_OUTPUT_STREAM_EVENT = 1,
  IS_PROCESS_OUTPUT_RS_CS_STATS
} is_process_output_type_t;


typedef struct _is_process_output {
  is_process_output_type_t type;
  int video_stream_on;
  is_output_type *is_output;
} is_process_output_t;


typedef boolean (*is_set_parameters_func)(is_set_parameter_t *param,
  is_info_t *is_info);

typedef void (*is_process_func)(is_process_parameter_t *param,
  is_process_output_t *output);

typedef void (*is_callback_func)(mct_port_t *port,
  is_process_output_t *output);

boolean is_set_parameters(is_set_parameter_t *param, is_info_t *is_info);
void is_process(is_process_parameter_t *param, is_process_output_t *output);

#endif /* __IS_H__ */
