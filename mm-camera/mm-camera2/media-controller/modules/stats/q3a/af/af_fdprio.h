/* af_fdprio.h
 *
 * Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef AF_FDPRIO_H_
#define AF_FDPRIO_H_

#define   AF_FDPRIO_NO_FACE_FRAME_COUNT_TRSHLD        0
#define   AF_FDPRIO_NUM_FRAMES_TO_WAIT                3

#define   AF_FDPRIO_NUM_HISTORY_ENTRIES               9
#define   AF_FDPRIO_NEW_FACE_STABILITY                10

/** af_fdprio_cmd_t:
 *
 * This enum defines the operations of the module
 *
 **/
typedef enum {
  AF_FDPRIO_CMD_INIT,
  AF_FDPRIO_CMD_PROC_COUNTERS,
  AF_FDPRIO_CMD_PROC_FD_ROI,
  AF_FDPRIO_CMD_PROC_FD_MAX
} af_fdprio_cmd_t;

/** stability_data_t
 *    @face_id:      The ID of the detected face
 *    @frame_count:  How many consequent frames the face has been detected
 *
 * This structure is used to track for how many consequent frames a face has
 * been detected.
 **/
typedef struct _stability_data {
  int32_t   face_id;
  uint32_t  frame_count;
} stability_data_t;

/** af_fdprio_t
 *    @fd_enabled:                 Is the face detect function enabled
 *    @roi_change:                 Flag to indicate if the ROI has changed
 *    @reset_history:              Flag to indicate if the history is to be
 *                                 reset
 *    @is_fdaf_start:              weather to send the initial FD coordinates
 *    @noface_cnt:                 how many consequent frames no faces are
 *                                 detected
 *    @waitframe_cnt:              how many frames to wait doing nothing
 *    @faces_detected:             how many faces were detected in the latest
 *                                 frame
 *    @info_pos:                   where to write the next biggest face info
 *    @tracked_face_history:       history of the tracked face
 *    @current_fd_roi:             The current ROI of the detected face
 *    @current_frame_id:           The ID of the current frame as sent by the
 *                                 face detection module
 *    @last_processed_frame_id:    The ID of the last processed frame
 *    @median_temporal_size:       The temporal value for the face size
 *    @last_median_temporal_size:  The last size to be checked against
 *    @ref_size:                   The reference face size
 *    @stable_size_cnt:            How many consequent frames the size of the
 *                                 tracked face is stable
 *    @median_temporal_pos_x:      The temporal value for the x position of the
 *                                 tracked face
 *    @last_median_temporal_pos_x: The last x position to be checked against
 *    @ref_pos_x:                  The reference x position
 *    @median_temporal_pos_y:      The temporal value for the y position of the
 *                                 tracked face
 *    @last_median_temporal_pos_y: The last y position to be checked against
 *    @ref_pos_y:                  The reference y position
 *    @stable_pos_cnt:             How many consequent frames the position of
 *                                 the tracked face is stable
 *    @curr_biggest_face_id:       The ID of the current biggest face
 *    @prev_biggest_face_id:       The ID of the previous biggest face
 *    @stable_data:                Structure containing the stability data
 *    @pface_info:                 info for the detected faces in the latest
 *                                 frame must not be NULL if the command is
 *                                 AF_FDPRIO_CMD_PROC_FD_ROI
 *    @thread_data:                The AF thread data needed to send commands
 *                                 to the library
 *
 * This is the private data structure of the module
 */
typedef struct _af_fdprio {
  boolean           fd_enabled;
  boolean           roi_change;
  boolean           reset_history;
  boolean           is_fdaf_start;
  uint8_t           noface_cnt;
  uint8_t           waitframe_cnt;
  uint8_t           faces_detected;
  uint8_t           info_pos;
  cam_rect_t        tracked_face_history[AF_FDPRIO_NUM_HISTORY_ENTRIES];
  cam_rect_t        current_fd_roi;
  uint32_t          current_frame_id;
  uint32_t          last_processed_frame_id;

  int32_t           median_temporal_size;
  int32_t           last_median_temporal_size;
  int32_t           ref_size;
  int32_t           stable_size_cnt;

  int32_t           median_temporal_pos_x;
  int32_t           last_median_temporal_pos_x;
  int32_t           ref_pos_x;
  int32_t           median_temporal_pos_y;
  int32_t           last_median_temporal_pos_y;
  int32_t           ref_pos_y;
  int32_t           stable_pos_cnt;

  int32_t           curr_biggest_face_id;
  int32_t           prev_biggest_face_id;
  stability_data_t  stable_data;
  mct_face_info_t   *pface_info;
  q3a_thread_data_t *thread_data;
  boolean           faces_detected_previously;
} af_fdprio_t;

/* API function(s) */
boolean af_fdprio_process(af_fdprio_t *af_fdprio_data, af_fdprio_cmd_t cmd);

#endif /* AF_FDPRIO_H_ */
