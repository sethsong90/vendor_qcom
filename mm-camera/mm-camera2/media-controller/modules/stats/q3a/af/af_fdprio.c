/* af_fdprio.c
 *
 * Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include "af_port.h"
#include "af_fdprio.h"

#ifdef  DEBUG_AF_FDPRIO
#undef  CDBG
#define CDBG CDBG_ERROR
#endif

#define AF_FDPRIO_INDEX_INVALID 0xFFFF

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define PERCENT_CHANGE(a, b) \
  (((float)MAX(a, b) / (float)MIN(a, b)) * 100.0 - 100.0)

#define AF_FDPRIO_MEDIAN_SIZE_CHANGE_WEIGHT   1
#define AF_FDPRIO_LAST_SIZE_CHANGE_WEIGHT     1
/* AF_FDPRIO_SIZE_CHANGE_TRESHLD is the difference in % */
#define AF_FDPRIO_SIZE_CHANGE_TRESHLD         20.0
#define AF_FDPRIO_SIZE_CHANGE_LOW_TRESHLD     8.0

#define AF_FDPRIO_MEDIAN_POS_CHANGE_WEIGHT    8
#define AF_FDPRIO_LAST_POS_CHANGE_WEIGHT      2
/* AF_FDPRIO_POS_CHANGE_TRESHLD is the position change in % */
#define AF_FDPRIO_POS_CHANGE_TRESHLD          12
#define AF_FDPRIO_POS_CHANGE_LOW_TRESHLD      4

#define AF_FDPRIO_STABLE_CNT_SIZE             0
#define AF_FDPRIO_STABLE_CNT_POS              3

#define AF_FDPRIO_OLD_NEW_DIFF_COEF           0.9f

/********************************************
     FACE DETECT PRIORITY ALGORITHM API
********************************************/

/* Internal helper functions */
static void    af_fdprio_init(af_fdprio_t *af_fdprio_data);
static boolean af_fdprio_process_counters(af_fdprio_t *af_fdprio_data);
static boolean af_fdprio_process_counters_int(af_fdprio_t *af_fdprio_data);
static boolean af_fdprio_process_fd_roi(af_fdprio_t *af_fdprio_data);
static int32_t af_fdprio_find_biggest_face(mct_face_info_t *face_info);
static boolean af_fdprio_find_old_biggest_face(af_fdprio_t *af_fdprio_data);
static boolean af_fdprio_send_default_roi(af_fdprio_t *af_fdprio_data);
static int32_t af_fdprio_get_index_by_id(af_fdprio_t *af_fdprio_data,
  int32_t face_id);
static boolean af_fdprio_is_old_face_big_enough(af_fdprio_t *af_fdprio_data,
  int32_t curr_biggest_face_id);
void           af_fdprio_gether_roi_info(af_fdprio_t *af_fdprio_data,
  int32_t curr_biggest_face_id);
static int32_t af_fdprio_get_median_size_change(af_fdprio_t *af_fdprio_data);
static int32_t af_fdprio_get_median_pos_change_x(af_fdprio_t *af_fdprio_data);
static int32_t af_fdprio_get_median_pos_change_y(af_fdprio_t *af_fdprio_data);
static boolean af_fdprio_stop_focus(af_fdprio_t *af_fdprio_data);
static boolean af_fdprio_start_focus(af_fdprio_t *af_fdprio_data);
static boolean af_fdprio_send_current_roi(af_fdprio_t *af_fdprio_data);
static boolean af_fdprio_send_initial_roi(af_fdprio_t *af_fdprio_data,
  int32_t curr_biggest_face_id);
static void    af_fdprio_begin_new_history(af_fdprio_t *af_fdprio_data);

/** af_fdprio_get_is_focus_mode_CAF
 *    @af_fdprio_data: internal data to control the FD priority AF feature
 *
 * This function is called to find whether the current focus mode is CAF or not
 *
 * Return TRUE if focus mode is CAF, FALSE otherwise.
 **/
static boolean af_fdprio_get_is_focus_mode_CAF(af_fdprio_t *af_fdprio_data)
{
  boolean rc = TRUE;
  /* Allocate memory to create AF message. we'll post it to AF thread.*/
  q3a_thread_af_msg_t *af_msg = (q3a_thread_af_msg_t *)
    malloc(sizeof(q3a_thread_af_msg_t));
  if (af_msg == NULL) {
    return FALSE;
  }
  memset(af_msg, 0, sizeof(q3a_thread_af_msg_t));

  af_msg->type = MSG_AF_GET;
  af_msg->u.af_set_parm.type = AF_GET_PARAM_FOCUS_MODE;
  af_msg->sync_flag = TRUE;
  rc = q3a_af_thread_en_q_msg(af_fdprio_data->thread_data, af_msg);
  if (rc) {
    int mode = af_msg->u.af_get_parm.u.af_mode;
    if((mode == AF_MODE_CAF) || (mode == AF_MODE_CAF_MACRO) ||
      (mode == AF_MODE_CAF_NORMAL))
      rc = TRUE;
    else
      rc = FALSE;
  }
  free(af_msg);
  return rc;
}


/** af_fdprio_init:
 *    @af_fdprio_data: internal data to control the FD priority AF feature
 *
 * Initialize the internal data for the FD priority AF feature
 *
 * Return nothing
 **/
static void af_fdprio_init(af_fdprio_t *af_fdprio_data)
{
  CDBG("%s:%d Init", __func__, __LINE__);
  memset(af_fdprio_data, 0, sizeof(af_fdprio_t));

  /* We need to initialize noface_cnt to threshold so that we don't
     trigger unnecessary "reset to default" search in the beginning.
     We need to reset it only we detect first face */
  af_fdprio_data->noface_cnt = AF_FDPRIO_NO_FACE_FRAME_COUNT_TRSHLD + 1;

}

/** af_fdprio_process_counters
 *    @af_fdprio_data: internal data to control the FD priority AF feature
 *
 * This function is called for each frame to drive the internal counters.
 *
 * Return TRUE on success, FALSE on failure.
 **/
static boolean af_fdprio_process_counters(af_fdprio_t *af_fdprio_data)
{
  boolean rc;

  if (af_fdprio_data->last_processed_frame_id ==
    af_fdprio_data->current_frame_id) {
    CDBG("%s:%d Already processed, returning...", __func__, __LINE__);
    return TRUE;
  }

  rc = af_fdprio_process_counters_int(af_fdprio_data);
  return rc;
}

/** af_fdprio_process_counters_int
 *    @af_fdprio_data: internal data to control the FD priority AF feature
 *
 * This function is called for each frame to drive the internal counters, but
 * only if the given frame is not processed already.
 *
 * Return TRUE on success, FALSE on failure.
 **/
static boolean af_fdprio_process_counters_int(af_fdprio_t *af_fdprio_data)
{
  CDBG("%s:%d Process counters", __func__, __LINE__);

  if (af_fdprio_data->faces_detected) {
    af_fdprio_data->noface_cnt = 0;
    if (af_fdprio_data->waitframe_cnt &&
      (af_fdprio_data->last_processed_frame_id !=
      af_fdprio_data->current_frame_id)) {
      af_fdprio_data->waitframe_cnt--;
      CDBG("%s:%d Decrement wait count", __func__, __LINE__);
    }
  } else {
    /* increment only once above threshold */
    if (af_fdprio_data->noface_cnt <= AF_FDPRIO_NO_FACE_FRAME_COUNT_TRSHLD) {
      af_fdprio_data->noface_cnt++;
      CDBG("%s:%d No face counter: %d", __func__, __LINE__,
        af_fdprio_data->noface_cnt);
      /* check if if the threshold is reached */
      if (af_fdprio_data->noface_cnt > AF_FDPRIO_NO_FACE_FRAME_COUNT_TRSHLD) {
        CDBG("%s:%d Trigger default ROI AF_START", __func__, __LINE__);
        if((af_fdprio_data->faces_detected_previously == TRUE) &&
          (af_fdprio_get_is_focus_mode_CAF(af_fdprio_data))) {
          af_fdprio_data->faces_detected_previously = FALSE;
          af_fdprio_send_default_roi(af_fdprio_data);
          af_fdprio_data->is_fdaf_start = FALSE;
        }
      }
    }
  }

  af_fdprio_data->last_processed_frame_id = af_fdprio_data->current_frame_id;

  return TRUE;
}

/** af_fdprio_process_fd_roi:
 *    @af_fdprio_data: internal data to control the FD priority AF feature
 *
 * This function will process the data for the detected faces (if any)
 *
 * Return TRUE on success, FALSE on failure.
 **/
static boolean af_fdprio_process_fd_roi(af_fdprio_t *af_fdprio_data)
{
  boolean         rc = FALSE;
  mct_face_info_t *face_info;
  int32_t         face_id_to_track;
  int32_t         curr_biggest_face_id;
  int32_t         median_size_change;
  int32_t         median_pos_change_x;
  int32_t         median_pos_change_y;
  double          weighted_change;
  double          weighted_change_x;
  double          weighted_change_y;
  boolean         stable = FALSE;
  boolean         stable_x = FALSE;
  boolean         stable_y = FALSE;

  CDBG("%s:%d Process FD_ROI data", __func__, __LINE__);

  if (!af_fdprio_data->pface_info) {
    CDBG_ERROR("%s: Null pointer passed for face_info", __func__);
    return FALSE;
  }

  face_info = af_fdprio_data->pface_info;

  af_fdprio_data->faces_detected = face_info->face_count;
  af_fdprio_process_counters_int(af_fdprio_data);
  if (af_fdprio_data->waitframe_cnt || !face_info->face_count) {
    return TRUE;
  }

  if (face_info->face_count == 1) {
    curr_biggest_face_id = af_fdprio_data->pface_info->faces[0].face_id;
    af_fdprio_data->curr_biggest_face_id = curr_biggest_face_id;
    af_fdprio_data->prev_biggest_face_id = curr_biggest_face_id;
  } else {
    curr_biggest_face_id = af_fdprio_find_biggest_face(face_info);
  }

  face_id_to_track = curr_biggest_face_id;

  if (af_fdprio_data->curr_biggest_face_id != curr_biggest_face_id) {
    /* We have new biggest face in the frame */
    if (af_fdprio_find_old_biggest_face(af_fdprio_data)) {
      /* our face is still detected, now check how big it is compared */
      /* to the current biggest face */
      if (!af_fdprio_is_old_face_big_enough(af_fdprio_data,
        curr_biggest_face_id)) {
        if (af_fdprio_data->stable_data.face_id == curr_biggest_face_id) {
          af_fdprio_data->stable_data.frame_count++;
        } else {
          af_fdprio_data->stable_data.face_id = curr_biggest_face_id;
          af_fdprio_data->stable_data.frame_count = 0;
          af_fdprio_data->is_fdaf_start = FALSE;
        }

        if (af_fdprio_data->stable_data.frame_count >
          AF_FDPRIO_NEW_FACE_STABILITY) {
          /* the new face is big enough and is stable, so begin tracking it */
          af_fdprio_begin_new_history(af_fdprio_data);
          af_fdprio_data->prev_biggest_face_id =
            af_fdprio_data->curr_biggest_face_id;
          af_fdprio_data->curr_biggest_face_id = curr_biggest_face_id;
        } else {
          /* New biggest face not stable enough, continue tracking old one */
          face_id_to_track = af_fdprio_data->prev_biggest_face_id;
        }
      } else {
        /* old biggest face is big enough - continue to track */
        face_id_to_track = af_fdprio_data->prev_biggest_face_id;
      }
    }

  }

  CDBG("%s: Tracking face id %d", __func__, face_id_to_track);

  if (!af_fdprio_data->is_fdaf_start) {
    CDBG_ERROR("%s: Need to send initial FD coordinates", __func__);
    af_fdprio_data->is_fdaf_start = TRUE;
    rc = af_fdprio_send_initial_roi(af_fdprio_data, face_id_to_track);
  }
  af_fdprio_gether_roi_info(af_fdprio_data, face_id_to_track);

  /* Begin size change detect */
  median_size_change = af_fdprio_get_median_size_change(af_fdprio_data);

  af_fdprio_data->median_temporal_size = (af_fdprio_data->median_temporal_size *
      AF_FDPRIO_MEDIAN_SIZE_CHANGE_WEIGHT + median_size_change *
      AF_FDPRIO_LAST_SIZE_CHANGE_WEIGHT) /
      (AF_FDPRIO_MEDIAN_SIZE_CHANGE_WEIGHT + AF_FDPRIO_LAST_SIZE_CHANGE_WEIGHT);

  if (MIN(af_fdprio_data->median_temporal_size, af_fdprio_data->ref_size)) {
    weighted_change = PERCENT_CHANGE(af_fdprio_data->median_temporal_size,
      af_fdprio_data->ref_size);
    if (weighted_change > AF_FDPRIO_SIZE_CHANGE_TRESHLD) {
      stable = FALSE;
      af_fdprio_data->ref_size = 0;
      af_fdprio_data->stable_size_cnt = 0;
    } else {
      stable = TRUE;
      af_fdprio_data->stable_size_cnt++;
      af_fdprio_data->reset_history = TRUE;
    }
  } else {
    if (MIN(af_fdprio_data->last_median_temporal_size,
      af_fdprio_data->median_temporal_size)) {
      weighted_change =
        PERCENT_CHANGE(af_fdprio_data->last_median_temporal_size,
      af_fdprio_data->median_temporal_size);
      if (weighted_change < AF_FDPRIO_SIZE_CHANGE_LOW_TRESHLD) {
        stable = TRUE;
        af_fdprio_data->stable_size_cnt++;
        af_fdprio_data->reset_history = TRUE;
        af_fdprio_data->ref_size = median_size_change;
      }
    } else {
      stable = FALSE;
      af_fdprio_data->stable_size_cnt = 0;
      weighted_change = 0;
    }
  }

  CDBG_ERROR("%s: Size change Ref: %d, last_med: %d, curr_med: %d, Weighted:"
    " %f, stable:%d", __func__,
    af_fdprio_data->ref_size,
    af_fdprio_data->last_median_temporal_size,
    af_fdprio_data->median_temporal_size,
    weighted_change,
    stable);

  af_fdprio_data->last_median_temporal_size =
   af_fdprio_data->median_temporal_size;

  if (!stable) {
    CDBG_ERROR("%s: Median Face size unstable, stop focus: %f", __func__,
      weighted_change);
    rc = af_fdprio_stop_focus(af_fdprio_data);
    return rc;
  }

  if (af_fdprio_data->stable_size_cnt < AF_FDPRIO_STABLE_CNT_SIZE) {
    return TRUE;
  }/* End size change detect */

  /* Begin position change detect */
  median_pos_change_x = af_fdprio_get_median_pos_change_x(af_fdprio_data);
  median_pos_change_y = af_fdprio_get_median_pos_change_y(af_fdprio_data);

  af_fdprio_data->median_temporal_pos_x =
    (af_fdprio_data->median_temporal_pos_x *
    AF_FDPRIO_MEDIAN_POS_CHANGE_WEIGHT + median_pos_change_x *
    AF_FDPRIO_LAST_POS_CHANGE_WEIGHT) /
    (AF_FDPRIO_MEDIAN_POS_CHANGE_WEIGHT + AF_FDPRIO_LAST_POS_CHANGE_WEIGHT);
  af_fdprio_data->median_temporal_pos_y =
    (af_fdprio_data->median_temporal_pos_y *
    AF_FDPRIO_MEDIAN_POS_CHANGE_WEIGHT + median_pos_change_y *
    AF_FDPRIO_LAST_POS_CHANGE_WEIGHT) /
    (AF_FDPRIO_MEDIAN_POS_CHANGE_WEIGHT + AF_FDPRIO_LAST_POS_CHANGE_WEIGHT);

  if (MIN(af_fdprio_data->median_temporal_pos_x, af_fdprio_data->ref_pos_x)) {
    weighted_change_x = PERCENT_CHANGE(af_fdprio_data->median_temporal_pos_x,
      af_fdprio_data->ref_pos_x);
    if (weighted_change_x > AF_FDPRIO_POS_CHANGE_TRESHLD) {
      stable_x = FALSE;
      af_fdprio_data->ref_pos_x = 0;
      af_fdprio_data->stable_pos_cnt = 0;
    } else {
      stable_x = TRUE;
      af_fdprio_data->stable_pos_cnt++;
    }
  } else {
    if (MIN(af_fdprio_data->last_median_temporal_pos_x,
      af_fdprio_data->median_temporal_pos_x)) {
      weighted_change_x =
        PERCENT_CHANGE(af_fdprio_data->last_median_temporal_pos_x,
      af_fdprio_data->median_temporal_pos_x);
      if (weighted_change_x < AF_FDPRIO_POS_CHANGE_LOW_TRESHLD) {
        stable_x = TRUE;
        af_fdprio_data->stable_pos_cnt++;
        af_fdprio_data->ref_pos_x = median_pos_change_x;
      }
    } else {
      stable_x = FALSE;
      af_fdprio_data->stable_pos_cnt = 0;
      weighted_change_x = 0;
    }
  }

  CDBG_ERROR("%s: XPos change Ref: %d, last_med: %d, curr_med: %d, Weighted:"
    " %f, stable:%d", __func__,
    af_fdprio_data->ref_pos_x,
    af_fdprio_data->last_median_temporal_pos_x,
    af_fdprio_data->median_temporal_pos_x,
    weighted_change_x,
    stable_x);

  af_fdprio_data->last_median_temporal_pos_x =
    af_fdprio_data->median_temporal_pos_x;

  /* The Y part*/
  if (MIN(af_fdprio_data->median_temporal_pos_y, af_fdprio_data->ref_pos_y)) {
    weighted_change_y = PERCENT_CHANGE(af_fdprio_data->median_temporal_pos_y,
      af_fdprio_data->ref_pos_y);
    if (weighted_change_y > AF_FDPRIO_POS_CHANGE_TRESHLD) {
      stable_y = FALSE;
      af_fdprio_data->ref_pos_y = 0;
      af_fdprio_data->stable_pos_cnt = 0;
    } else {
      stable_y = TRUE;
      af_fdprio_data->stable_pos_cnt++;
      //af_fdprio_data->reset_history = TRUE;
    }
  } else {
    if (MIN(af_fdprio_data->last_median_temporal_pos_y,
      af_fdprio_data->median_temporal_pos_y)) {
      weighted_change_y =
        PERCENT_CHANGE(af_fdprio_data->last_median_temporal_pos_y,
      af_fdprio_data->median_temporal_pos_y);
      if (weighted_change_y < AF_FDPRIO_POS_CHANGE_LOW_TRESHLD) {
        stable_y = TRUE;
        af_fdprio_data->stable_pos_cnt++;
        //af_fdprio_data->reset_history = TRUE;
        af_fdprio_data->ref_pos_y = median_pos_change_y;
      }
    } else {
      stable_y = FALSE;
      af_fdprio_data->stable_pos_cnt = 0;
      weighted_change_y = 0;
    }
  }

  CDBG_ERROR("%s: YPos change Ref: %d, last_med: %d, curr_med: %d, Weighted:"
    " %f, stable:%d, stable_cnt:%d", __func__,
    af_fdprio_data->ref_pos_y,
    af_fdprio_data->last_median_temporal_pos_y,
    af_fdprio_data->median_temporal_pos_y,
    weighted_change_y,
    stable_y,
    af_fdprio_data->stable_pos_cnt);

  af_fdprio_data->last_median_temporal_pos_y =
   af_fdprio_data->median_temporal_pos_y;

  if (!stable_x || !stable_y) {
    CDBG_ERROR("%s: Median Face position unstable, stop focus: %f", __func__,
      weighted_change_x);
    rc = af_fdprio_stop_focus(af_fdprio_data);
    return rc;
  } else {
    af_fdprio_data->stable_pos_cnt--;
  }

  if (af_fdprio_data->stable_pos_cnt < AF_FDPRIO_STABLE_CNT_POS) {
    return TRUE;
  }

  /* End position change detect */

  if (af_fdprio_data->roi_change == TRUE) {
    CDBG_ERROR("%s:[Was unstable, now stable: Refocus!!!", __func__);
    af_fdprio_data->roi_change = FALSE;
    af_fdprio_data->faces_detected_previously = TRUE;
    af_fdprio_send_current_roi(af_fdprio_data);
    //rc = af_fdprio_start_focus(af_fdprio_data);
  }

  return TRUE;
}

/** af_fdprio_process
 *    @af_fdprio_data: internal data to control the FD priority AF feature
 *    @cmd:            What command is to be processed by the module
 *
 * This is the processing function of the module
 *
 * Return TRUE on success, FALSE on failure
 **/
boolean af_fdprio_process(af_fdprio_t *af_fdprio_data, af_fdprio_cmd_t cmd)
{
  boolean rc = FALSE;

  if (!af_fdprio_data) {
    CDBG_ERROR("%s: Null pointer passed for af_fdprio_data", __func__);
    return FALSE;
  }

  CDBG("%s: Processing frame %d", __func__, af_fdprio_data->current_frame_id);

  switch(cmd) {
  case AF_FDPRIO_CMD_INIT: {
    af_fdprio_init(af_fdprio_data);
    rc = TRUE;
  }
    break;

  case AF_FDPRIO_CMD_PROC_COUNTERS: {
    rc = af_fdprio_process_counters(af_fdprio_data);
  }
    break;

  case AF_FDPRIO_CMD_PROC_FD_ROI: {
    rc = af_fdprio_process_fd_roi(af_fdprio_data);
  }
    break;

  default: {
  }
    break;
  }

  return rc;
}

/** af_fdprio_find_biggest_face
 *    @face_info: A pointer to the data with the detected faces (if any)
 *
 * This function will find the biggest detected face.
 *
 * Return the ID of the biggest detected face.
 **/
static int32_t af_fdprio_find_biggest_face(mct_face_info_t *face_info)
{
  int32_t biggest_face;
  cam_rect_t *biggest_fd_roi;
  int32_t i;

  biggest_fd_roi = &face_info->orig_faces[0].roi;
  biggest_face = face_info->faces[0].face_id;

  for (i = 1; i < face_info->face_count; i++) {
    if (biggest_fd_roi->width < face_info->orig_faces[i].roi.width ||
        biggest_fd_roi->height < face_info->orig_faces[i].roi.height) {
      biggest_fd_roi = &face_info->orig_faces[i].roi;
      biggest_face = face_info->faces[i].face_id;
    }
  }

  return biggest_face;
}

/** af_fdprio_find_old_biggest_face:
 *    @af_fdprio_data: internal data to control the FD priority AF feature
 *
 * Check if the current frame contains the same face that was biggest in the
 * previous frame.
 *
 * Return TRUE if the current frame contains the same face that was biggest in
 * the previous frame.
 **/
static boolean af_fdprio_find_old_biggest_face(af_fdprio_t *af_fdprio_data) {
  int32_t i;
  mct_face_info_t *face_info;

  face_info = af_fdprio_data->pface_info;

  for (i = 0; i < face_info->face_count; i++) {
    /* compare with the curr_biggest_face_id field because this is the id
     * we have been tracking so far
     **/
    if (face_info->faces[i].face_id == af_fdprio_data->curr_biggest_face_id) {
      return TRUE;
    }
  }

  return FALSE;
}

/** af_fdprio_get_index_by_id:
 *    @af_fdprio_data: internal data to control the FD priority AF feature
 *    @face_id:        The ID of the face
 *
 * Finds the index of the face in the array of faces by it's ID.
 *
 * Return the index of the face on success, invalid constant on failure.
 **/
static int32_t af_fdprio_get_index_by_id(af_fdprio_t *af_fdprio_data,
  int32_t face_id)
{
  int32_t i;
  mct_face_info_t *face_info;

  face_info = af_fdprio_data->pface_info;

  for (i = 0; i < face_info->face_count; i++) {
    if(face_info->faces[i].face_id == face_id) {
      return i;
    }
  }

  return AF_FDPRIO_INDEX_INVALID;
}

/** af_fdprio_is_old_face_big_enough:
 *    @af_fdprio_data:       internal data to control the FD priority AF feature
 *    @curr_biggest_face_id: ID of the current biggest face
 *
 * This function will check if the old biggest face is big enough compared
 * to the current biggest face.
 *
 * Return TRUE if the old biggest face is big enough, otherwise return FALSE.
 **/
static boolean af_fdprio_is_old_face_big_enough(af_fdprio_t *af_fdprio_data,
  int32_t curr_biggest_face_id)
{
  boolean         rc = FALSE;
  int32_t         old_face_idx;
  int32_t         new_face_idx;
  mct_face_info_t *face_info;

  face_info = af_fdprio_data->pface_info;

  old_face_idx = af_fdprio_get_index_by_id(af_fdprio_data,
    af_fdprio_data->curr_biggest_face_id);
  new_face_idx = af_fdprio_get_index_by_id(af_fdprio_data,
    curr_biggest_face_id);

  if (old_face_idx == AF_FDPRIO_INDEX_INVALID ||
    new_face_idx == AF_FDPRIO_INDEX_INVALID) {
    CDBG("%s: _face_index is invalid", __func__);
    return FALSE;
  }

  if (old_face_idx > (MAX_ROI - 1) ||
    new_face_idx > (MAX_ROI - 1)) {
    CDBG("%s: _face_index is bigger than MAX_ROI", __func__);
    return FALSE;
  }

  if ((float)face_info->orig_faces[old_face_idx].roi.width *
    (float)face_info->orig_faces[old_face_idx].roi.height >
    (float)face_info->orig_faces[new_face_idx].roi.width *
    (float)face_info->orig_faces[new_face_idx].roi.height *
    AF_FDPRIO_OLD_NEW_DIFF_COEF) {
    rc = TRUE;
  }

  return rc;
}

/** af_fdprio_gether_roi_info:
 *    @af_fdprio_data:       internal data to control the FD priority AF feature
 *    @curr_biggest_face_id: The ID of the current biggest face
 *
 * This function will save the roi of the current biggest face in the the
 * history in order to track the changes of the size and position
 *
 * Return nothing
 **/
void af_fdprio_gether_roi_info(af_fdprio_t *af_fdprio_data,
  int32_t curr_biggest_face_id)
{
  int32_t idx;
  int     i;

  idx = af_fdprio_get_index_by_id(af_fdprio_data, curr_biggest_face_id);

  if (idx > (MAX_ROI - 1)) {
    CDBG("%s: face_index is bigger than MAX_ROI", __func__);
    return;
  }

  if (af_fdprio_data->reset_history) {
    af_fdprio_data->reset_history = 0;
    for (i = 0; i < AF_FDPRIO_NUM_HISTORY_ENTRIES; i++) {
      af_fdprio_data->tracked_face_history[i] =
        af_fdprio_data->pface_info->orig_faces[idx].roi;
    }
  } else {
    af_fdprio_data->tracked_face_history[af_fdprio_data->info_pos++] =
    af_fdprio_data->pface_info->orig_faces[idx].roi;
  }

  if (af_fdprio_data->info_pos >= AF_FDPRIO_NUM_HISTORY_ENTRIES) {
    af_fdprio_data->info_pos = 0;
  }
}

/** af_fdprio_get_median_size_change:
 *    @af_fdprio_data: internal data to control the FD priority AF feature
 *
 * Get the change in the size of the tracked face using median filter.
 *
 * Return the median change of the size of the tracked face.
 **/
static int32_t af_fdprio_get_median_size_change(
  af_fdprio_t *af_fdprio_data)
{
  int32_t change_sizes[AF_FDPRIO_NUM_HISTORY_ENTRIES];
  int32_t curr_val;
  int32_t next_val;
  int32_t i;
  int32_t tmp_pos;
  int32_t tmp_val;

  for (i = 0; i < AF_FDPRIO_NUM_HISTORY_ENTRIES; i++) {
    curr_val = af_fdprio_data->tracked_face_history[i].height *
      af_fdprio_data->tracked_face_history[i].width;
    change_sizes[i] = curr_val;
  }

  /* Using insertion algorithm to sort the array */
  for (i = 1; i < AF_FDPRIO_NUM_HISTORY_ENTRIES; i++) {
    tmp_val = change_sizes[i];
    tmp_pos = i;
    while ((tmp_pos > 0) && (tmp_val < change_sizes[tmp_pos - 1])) {
      change_sizes[tmp_pos] = change_sizes[tmp_pos - 1];
      tmp_pos--;
    }
    change_sizes[tmp_pos] = tmp_val;
  }

  return change_sizes[AF_FDPRIO_NUM_HISTORY_ENTRIES / 2];
}

/** af_fdprio_get_median_pos_change_x:
 *    @af_fdprio_data: internal data to control the FD priority AF feature
 *
 * Get the change in the x position of the tracked face using median filter.
 *
 * Return the median change of the x position of the tracked face.
 **/
static int32_t af_fdprio_get_median_pos_change_x(af_fdprio_t *af_fdprio_data)
{
  int32_t change_pos_x[AF_FDPRIO_NUM_HISTORY_ENTRIES];
  int32_t curr_val_x;
  int32_t curr_dx;
  int32_t i;
  int32_t tmp_pos;
  int32_t tmp_val;

  for (i = 0; i < AF_FDPRIO_NUM_HISTORY_ENTRIES; i++) {
    curr_val_x = af_fdprio_data->tracked_face_history[i].left;
    curr_dx = af_fdprio_data->tracked_face_history[i].width;

    change_pos_x[i] = curr_val_x + curr_dx / 2;
  }

  /* Using insertion algorithm to sort the x array */
  for (i = 1; i < AF_FDPRIO_NUM_HISTORY_ENTRIES; i++) {
    tmp_val = change_pos_x[i];
    tmp_pos = i;
    while ((tmp_pos > 0) && (tmp_val < change_pos_x[tmp_pos - 1])) {
      change_pos_x[tmp_pos] = change_pos_x[tmp_pos - 1];
      tmp_pos--;
    }
    change_pos_x[tmp_pos] = tmp_val;
  }

  return change_pos_x[AF_FDPRIO_NUM_HISTORY_ENTRIES / 2];
}

/** af_fdprio_get_median_pos_change_y:
 *    @af_fdprio_data: internal data to control the FD priority AF feature
 *
 * Get the change in the y position of the tracked face using median filter.
 *
 * Return the median change of the y position of the tracked face.
 **/
static int32_t af_fdprio_get_median_pos_change_y(af_fdprio_t *af_fdprio_data)
{
  int32_t change_pos_y[AF_FDPRIO_NUM_HISTORY_ENTRIES];
  int32_t curr_val_y;
  int32_t curr_dy;
  int32_t i;
  int32_t tmp_pos;
  int32_t tmp_val;

  for (i = 0; i < AF_FDPRIO_NUM_HISTORY_ENTRIES; i++) {
    curr_val_y = af_fdprio_data->tracked_face_history[i].top;
    curr_dy = af_fdprio_data->tracked_face_history[i].height;

    change_pos_y[i] = curr_val_y + curr_dy / 2;
  }

  /* Using insertion algorithm to sort the y array */
  for(i = 1; i < AF_FDPRIO_NUM_HISTORY_ENTRIES; i++) {
    tmp_val = change_pos_y[i];
    tmp_pos = i;
    while((tmp_pos > 0) && (tmp_val < change_pos_y[tmp_pos - 1])) {
      change_pos_y[tmp_pos] = change_pos_y[tmp_pos - 1];
      tmp_pos--;
    }
    change_pos_y[tmp_pos] = tmp_val;
  }

  return change_pos_y[AF_FDPRIO_NUM_HISTORY_ENTRIES / 2];
}

/** af_fdprio_begin_new_history:
 *    @af_fdprio_data: internal data to control the FD priority AF feature
 *
 * Wipe out the tracking history.
 *
 * Return nothing
 **/
static void af_fdprio_begin_new_history(af_fdprio_t *af_fdprio_data)
{
  memset(af_fdprio_data->tracked_face_history, 0,
    sizeof(af_fdprio_data->tracked_face_history));
  af_fdprio_data->info_pos = 0;
  CDBG("%s:New_face!", __func__);
}

/** af_fdprio_send_default_roi:
 *    @af_fdprio_data: internal data to control the FD priority AF feature
 *
 * Ask the library to send the default ROI to the ISP when no faces are detected
 * for a given number of consequent frames.
 *
 * Return TRUE on success, FALSE on failure.
 **/
static boolean af_fdprio_send_default_roi(af_fdprio_t *af_fdprio_data)
{
  boolean rc = FALSE;
  /* Allocate memory to create AF message. we'll post it to AF thread. */

  q3a_thread_af_msg_t *af_msg = (q3a_thread_af_msg_t *)
    malloc(sizeof(q3a_thread_af_msg_t));
  if (af_msg == NULL) {
    return FALSE;
  }
  memset(af_msg, 0, sizeof(q3a_thread_af_msg_t));

  af_msg->u.af_set_parm.u.af_roi_info.roi_updated = TRUE;
  af_msg->u.af_set_parm.u.af_roi_info.frm_id = af_fdprio_data->current_frame_id;
  af_msg->u.af_set_parm.u.af_roi_info.num_roi = 0;
  af_msg->u.af_set_parm.u.af_roi_info.type = AF_ROI_TYPE_TOUCH;

  af_msg->u.af_set_parm.u.af_roi_info.roi[0].x = 0;
  af_msg->u.af_set_parm.u.af_roi_info.roi[0].y = 0;
  af_msg->u.af_set_parm.u.af_roi_info.roi[0].dx = 0;
  af_msg->u.af_set_parm.u.af_roi_info.roi[0].dy = 0;

  af_fdprio_data->current_fd_roi.left = 0;
  af_fdprio_data->current_fd_roi.top = 0;
  af_fdprio_data->current_fd_roi.width = 0;
  af_fdprio_data->current_fd_roi.height = 0;

  af_msg->type = MSG_AF_SET;
  af_msg->u.af_set_parm.type = AF_SET_PARAM_ROI;

  CDBG("%s: Sending default ROI", __func__);
  rc = q3a_af_thread_en_q_msg(af_fdprio_data->thread_data, af_msg);

  /* after setting the default ROI we also want to start the af */
  rc = af_fdprio_start_focus(af_fdprio_data);

  return rc;
}

/** af_fdprio_send_current_roi:
 *    @af_fdprio_data: internal data to control the FD priority AF feature
 *
 * If the ROI of the tracked face is changed enough, this function will be
 * called to send the new ROI.
 *
 * Return TRUE on success, FALSE on failure.
 **/
static boolean af_fdprio_send_current_roi(af_fdprio_t *af_fdprio_data) {
  boolean rc = FALSE;
  uint8_t roi_pos;

  /* Allocate memory to create AF message. we'll post it to AF thread.*/
  q3a_thread_af_msg_t *af_msg = (q3a_thread_af_msg_t *)
    malloc(sizeof(q3a_thread_af_msg_t));
  if (af_msg == NULL) {
    return FALSE;
  }
  memset(af_msg, 0, sizeof(q3a_thread_af_msg_t));

  af_msg->u.af_set_parm.u.af_roi_info.roi_updated = TRUE;
  af_msg->u.af_set_parm.u.af_roi_info.frm_id = af_fdprio_data->current_frame_id;
  af_msg->u.af_set_parm.u.af_roi_info.num_roi = 1;
  af_msg->u.af_set_parm.u.af_roi_info.type = AF_ROI_TYPE_FACE;

  if (af_fdprio_data->info_pos == 0) {
    roi_pos = AF_FDPRIO_NUM_HISTORY_ENTRIES - 1;
  } else {
    roi_pos = af_fdprio_data->info_pos - 1;
  }

  af_msg->u.af_set_parm.u.af_roi_info.roi[0].x =
    af_fdprio_data->tracked_face_history[roi_pos].left;
  af_msg->u.af_set_parm.u.af_roi_info.roi[0].y =
    af_fdprio_data->tracked_face_history[roi_pos].top;
  af_msg->u.af_set_parm.u.af_roi_info.roi[0].dx =
    af_fdprio_data->tracked_face_history[roi_pos].width;
  af_msg->u.af_set_parm.u.af_roi_info.roi[0].dy =
    af_fdprio_data->tracked_face_history[roi_pos].height;

  /*Save the current ROI that we set */
  af_fdprio_data->current_fd_roi.left =
    af_fdprio_data->tracked_face_history[roi_pos].left;
  af_fdprio_data->current_fd_roi.top =
    af_fdprio_data->tracked_face_history[roi_pos].top;
  af_fdprio_data->current_fd_roi.width =
    af_fdprio_data->tracked_face_history[roi_pos].width;
  af_fdprio_data->current_fd_roi.height =
    af_fdprio_data->tracked_face_history[roi_pos].height;

  af_msg->type = MSG_AF_SET;
  af_msg->u.af_set_parm.type = AF_SET_PARAM_ROI;

  CDBG("%s: Sending last Face ROI", __func__);
  rc = q3a_af_thread_en_q_msg(af_fdprio_data->thread_data, af_msg);

  return rc;
}

/** af_fdprio_send_initial_roi:
 *    @af_fdprio_data:       internal data to control the FD priority AF feature
 *    @curr_biggest_face_id: The ID of the current biggest face in the frame
 *
 * This function will send the ROI of the first detected biggest face.
 *
 * Return TRUE on success, FALSE on failure.
 **/
static boolean af_fdprio_send_initial_roi(af_fdprio_t *af_fdprio_data,
  int32_t curr_biggest_face_id)
{
  boolean rc = FALSE;
  int32_t idx;

  idx = af_fdprio_get_index_by_id(af_fdprio_data, curr_biggest_face_id);

  if ( (0 > idx) || (MAX_ROI <= idx) ) {
    CDBG_ERROR("%s: ROI index out of range (%d)", __func__, (int) idx);
    return FALSE;
  }

  /* Allocate memory to create AF message. we'll post it to AF thread.*/
  q3a_thread_af_msg_t *af_msg = (q3a_thread_af_msg_t *)
    malloc(sizeof(q3a_thread_af_msg_t));
  if (af_msg == NULL) {
    return FALSE;
  }
  memset(af_msg, 0, sizeof(q3a_thread_af_msg_t));

  af_msg->u.af_set_parm.u.af_roi_info.roi_updated = TRUE;
  af_msg->u.af_set_parm.u.af_roi_info.frm_id = af_fdprio_data->current_frame_id;
  af_msg->u.af_set_parm.u.af_roi_info.num_roi = 1;
  af_msg->u.af_set_parm.u.af_roi_info.type = AF_ROI_TYPE_FACE;

  af_msg->u.af_set_parm.u.af_roi_info.roi[0].x =
    af_fdprio_data->pface_info->orig_faces[idx].roi.left;
  af_msg->u.af_set_parm.u.af_roi_info.roi[0].y =
    af_fdprio_data->pface_info->orig_faces[idx].roi.top;
  af_msg->u.af_set_parm.u.af_roi_info.roi[0].dx =
    af_fdprio_data->pface_info->orig_faces[idx].roi.width;
  af_msg->u.af_set_parm.u.af_roi_info.roi[0].dy =
    af_fdprio_data->pface_info->orig_faces[idx].roi.height;

  af_msg->type = MSG_AF_SET;
  af_msg->u.af_set_parm.type = AF_SET_PARAM_ROI;

  CDBG("%s: Sending initial Face ROI", __func__);
  rc = q3a_af_thread_en_q_msg(af_fdprio_data->thread_data, af_msg);

  return rc;
}

/** af_fdprio_stop_focus:
 *    @af_fdprio_data: internal data to control the FD priority AF feature
 *
 * Indicates that the ROI has changed due to unstable data for the detected
 * face(s).
 *
 * Return always TRUE.
 **/
static boolean af_fdprio_stop_focus(af_fdprio_t *af_fdprio_data)
{
  boolean rc = TRUE;

  af_fdprio_data->roi_change = TRUE;
  af_fdprio_data->waitframe_cnt = AF_FDPRIO_NUM_FRAMES_TO_WAIT;

  return rc;
}

/** af_fdprio_start_focus:
 *    @af_fdprio_data: internal data to control the FD priority AF feature
 *
 * Ask the library to refocus.
 *
 * Return TRUE on success, FALSE on failure.
 **/
static boolean af_fdprio_start_focus(af_fdprio_t *af_fdprio_data)
{
  boolean rc = FALSE;
  /* Allocate memory to create AF message. we'll post it to AF thread.*/
  q3a_thread_af_msg_t *af_msg = (q3a_thread_af_msg_t *)
    malloc(sizeof(q3a_thread_af_msg_t));
  if (af_msg == NULL) {
    return FALSE;
  }
  memset(af_msg, 0, sizeof(q3a_thread_af_msg_t));

  af_msg->type = MSG_AF_START;
  af_msg->u.af_set_parm.type = AF_SET_PARAM_START;

  CDBG("%s: Sending Start focus cmd", __func__);
  rc = q3a_af_thread_en_q_msg(af_fdprio_data->thread_data, af_msg);

  return rc;
}
