/**********************************************************************
* Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved. *
* Qualcomm Technologies Proprietary and Confidential.                 *
**********************************************************************/
#include <cutils/properties.h>
#include <linux/media.h>
#include "mct_module.h"
#include "module_faceproc.h"

#define FACEPROC_FACE_SAME   0
#define FACEPROC_FACE_BEFORE 1
#define FACEPROC_FACE_AFTER  2

/* Macro used to apply temporal filter */
#define FD_STAB_APPLY_TEMP_FILTER(first, weight_first, second, weight_second) \
  ((first) = (((second) * (weight_second)) + ((first) * (weight_first))) / \
  ((weight_first) + (weight_second)))

/* Macro used to calculate threshold from 0.1 percentage to value */
#define FD_STAB_CALC_THRESHOLD(value, per) \
  (((value) * (per)) / 1000)

/* Macro used to calculate distance between two pixels */
#define FD_STAB_CALC_DISTANCE(a, b) \
  (sqrt((uint32_t)(pow((abs((a).x - (b).x)), 2) + pow((abs((a).y - (b).y)), 2))))

  /**
* Function: module_faceproc_stab_check_face
*
* Description:
*   This function will check if the face is the same or new face is detected
*
* Arguments:
*   @roi - Face detection result
*   @history - Face detection history
*
* Return values:
*   FACEPROC_FACE_SAME - Faces are the same
*   FACEPROC_FACE_BEFORE - Face is before with coordinates then reference face
*   FACEPROC_FACE_AFTER - Face is after with coordinates then reference face
* Notes: none
**/
static int module_faceproc_stab_check_face(faceproc_info_t *roi,
  faceproc_faces_history_t *history)
 {
  int result;
  int treshold_x, treshold_y;
  int center1_x, center1_y, center2_x, center2_y;


  treshold_x = history->face_size.stable_entry.x;
  treshold_y = history->face_size.stable_entry.y;

  center1_x = history->face_position.stable_entry.x;
  center1_y = history->face_position.stable_entry.y;

  center2_x = roi->face_boundary.x + (roi->face_boundary.dx / 2);
  center2_y = roi->face_boundary.y + (roi->face_boundary.dy / 2);

  if ((abs((int)center1_x - (int)center2_x) < treshold_x) &&
      (abs((int)center1_y - (int)center2_y) < treshold_y)) {

    result = FACEPROC_FACE_SAME;

  } else if ((center1_x + (center1_y * MAX_FD_WIDTH)) <
             (center2_x + (center2_y * MAX_FD_WIDTH))) {

    result = FACEPROC_FACE_BEFORE;

  } else {

    result = FACEPROC_FACE_AFTER;

  }
  return result;
}

/**
* Function: module_faceproc_within_limit
*
* Description: Check if face is within the limit.
*
* Arguments:
*   @rect1 - First rectangle
*   @rect2 - Second rectangle
*   @threshold: Variation threshold
*
* Return values:
*  TRUE/FALSE
*
* Notes: none
**/
static boolean module_faceproc_within_limit(faceproc_history_entry_t *rect1,
  faceproc_history_entry_t *rect2, int threshold)
{
  if (abs((int)rect1->x - (int)rect2->x) < threshold &&
      abs((int)rect1->y - (int)rect2->y) < threshold) {
    return TRUE;
  }
  return FALSE;
}

/**
* Function: module_faceproc_stab_add_face
*
* Description: Add new face to the internal history,
*   internal history is ring buffer.
*
* Arguments:
*   @history - Fsces history
*   @roi - Result of face detection algo for this face
*   @p_fd_chromatix: faceproc chromatix pointer
*
* Return values:
*  0 - on success
* Notes: none
**/
static int module_faceproc_stab_add_face(faceproc_faces_history_t *history,
  faceproc_info_t *roi, fd_chromatix_t *p_fd_chromatix)
{
  history->id = abs(roi->unique_id);

  /* Fill face size */
  history->face_position.index = (history->face_position.index + 1) %
    p_fd_chromatix->stab_history;
  history->face_position.entry[history->face_position.index].x =
    roi->face_boundary.x + (roi->face_boundary.dx / 2);
  history->face_position.entry[history->face_position.index].y =
    roi->face_boundary.y + (roi->face_boundary.dy / 2);
  if (history->face_position.faces_inside < p_fd_chromatix->stab_history)
    history->face_position.faces_inside++;

  /* Fill face positions */
  history->face_size.index = (history->face_size.index + 1) %
    p_fd_chromatix->stab_history;
  history->face_size.entry[history->face_size.index].x = roi->face_boundary.dx;
  history->face_size.entry[history->face_size.index].y = roi->face_boundary.dy;
  if (history->face_size.faces_inside < p_fd_chromatix->stab_history)
    history->face_size.faces_inside++;

  /* Fill mouth position */
  history->mouth_position.index = (history->mouth_position.index + 1) %
    p_fd_chromatix->stab_history;
  history->mouth_position.entry[history->mouth_position.index].x =
    roi->fp.face_pt[FACE_PART_MOUTH].x;
  history->mouth_position.entry[history->mouth_position.index].y =
    roi->fp.face_pt[FACE_PART_MOUTH].y;
  if (history->mouth_position.faces_inside < p_fd_chromatix->stab_history)
    history->mouth_position.faces_inside++;

  return 0;
}

/**
* Function: module_faceproc_stab_fill_roi
*
* Description: Add new face to the internal history,
*   internal history is ring buffer.
*
* Arguments:
*   @faces - Faces history
*   @roi - New face which need to be added
*
* Return values:
*   0 - on success
* Notes: none
**/
static int module_faceproc_stab_fill_roi(faceproc_faces_history_t *faces,
  faceproc_info_t *roi)
{
  roi->face_boundary.dx = faces->face_size.stable_entry.x;
  roi->face_boundary.dy = faces->face_size.stable_entry.y;

  /* Face position is centered so move to begging of rectangle */
  roi->face_boundary.x = faces->face_position.stable_entry.x -
    (roi->face_boundary.dx / 2);
  roi->face_boundary.y = faces->face_position.stable_entry.y -
    (roi->face_boundary.dy / 2);

  /* Fill mouth position */
  roi->fp.face_pt[FACE_PART_MOUTH].x = faces->mouth_position.stable_entry.x;
  roi->fp.face_pt[FACE_PART_MOUTH].y = faces->mouth_position.stable_entry.y;

  return 0;
}

/**
* Function: module_faceproc_stab_init_face
*
* Description: Add new face and reset the faces history
*
* Arguments:
*   @history - Faces history
*   @roi - New face which need to be added
*   @p_fd_chromatix: faceproc chromatix pointer
*
* Return values:
*   0 - on success
* Notes: none
**/
static int module_faceproc_stab_init_face(faceproc_faces_history_t *history,
  faceproc_info_t *roi, fd_chromatix_t *p_fd_chromatix)
{
  history->face_position.index = 0;
  history->face_position.faces_inside = 0;
  history->face_position.max_state_count = p_fd_chromatix->stab_pos_state_cnt;
  history->face_position.state = FD_STAB_STATE_STABILIZE;

  history->face_size.index = 0;
  history->face_size.faces_inside = 0;
  history->face_size.max_state_count = p_fd_chromatix->stab_size_state_cnt;
  history->face_size.state = FD_STAB_STATE_STABILIZE;

  history->mouth_position.index = 0;
  history->mouth_position.faces_inside = 0;
  history->mouth_position.max_state_count = p_fd_chromatix->stab_mouth_state_cnt;
  history->mouth_position.state = FD_STAB_STATE_STABILIZE;

  module_faceproc_stab_add_face(history, roi, p_fd_chromatix);

  history->face_position.stable_entry =
    history->face_position.entry[history->face_position.index];
  history->face_size.stable_entry =
    history->face_size.entry[history->face_size.index];
  history->mouth_position.stable_entry =
    history->mouth_position.entry[history->mouth_position.index];

  return 0;
}

/**
* Function: module_faceproc_stab_is_continues
*
* Description: Return true if stabilization of the values is working in
*   continues mode
*
* Return values:
*   TRUE - If stabilization is operating in continues mode
*
* Notes: none
**/
static boolean module_faceproc_stab_is_continues(fd_face_stab_mode_t stab_mode)
{
  boolean mode_continues;
  switch (stab_mode) {
  case FD_STAB_CONTINUES_SMALLER:
  case FD_STAB_CONTINUES_BIGGER:
  case FD_STAB_CONTINUES_CLOSER_TO_REFERENCE:
    mode_continues = TRUE;
    break;
  case FD_STAB_BIGGER:
  case FD_STAB_SMALLER:
  case FD_STAB_EQUAL:
    return FALSE;
  default:
    mode_continues = TRUE;
    break;
  }
  return mode_continues;
}

/**
* Function: module_faceproc_stab_is_stable
*
* Description: Function which checks if values are stable
*
* Return values:
*   TRUE - if values are stable
* Notes: none
**/
static boolean module_faceproc_stab_is_stable(faceproc_history_holder_t *history,
  faceproc_history_entry_t *refer, fd_face_stab_mode_t stab_mode)
{
  boolean stable;

  switch (stab_mode) {
  case FD_STAB_EQUAL:
    stable = (history->stable_entry.x == history->entry[history->index].x &&
      history->stable_entry.y == history->entry[history->index].y);
    break;
  case FD_STAB_CONTINUES_SMALLER:
  case FD_STAB_SMALLER:
    stable = (history->stable_entry.x < history->entry[history->index].x &&
      history->stable_entry.y < history->entry[history->index].y);
    break;
  case FD_STAB_CONTINUES_BIGGER:
  case FD_STAB_BIGGER:
    stable = (history->stable_entry.x > history->entry[history->index].x &&
      history->stable_entry.y > history->entry[history->index].y);
    break;
  case FD_STAB_CONTINUES_CLOSER_TO_REFERENCE:
  case FD_STAB_CLOSER_TO_REFERENCE:
    stable = TRUE;
    if (refer) {
      stable = FD_STAB_CALC_DISTANCE(*refer, history->stable_entry) <
               FD_STAB_CALC_DISTANCE(*refer, history->entry[history->index]);
    }
    break;
  default:
    stable = TRUE;
    break;
  }
  return stable;
}

/**
* Function: module_faceproc_stab_filter
*
* Description: Stabilization it will stabilize the entry
*   based on reference entry it self, if reference is passed
*   coordinates change will be detected from them
*
* Arguments:
*   @history - History for this entry
*   @strength - Faces history
*   @threshold: Variation threshold
*   @refer: Reference coordinates for stabilization
*   @stable_compar: Stabilization compare parameter
*    0 - Face is stable when previous face is equal with new
*    1 - Face is stable when previous face is bigger then new
*    2 - Face is stable when previous face is smaller then new
*
* Return values:
*   0 - on success
* Notes: none
**/
static int module_faceproc_stab_filter(faceproc_history_holder_t *history,
  faceproc_history_entry_t *strength, uint32_t threshold,
  faceproc_history_entry_t *refer, fd_face_stab_mode_t stab_mode)
{
  faceproc_history_state_t new_state;
  boolean within_limit = FALSE;
  boolean face_stable = FALSE;
  uint32_t last_index;

  if (history->faces_inside == 1) {
    IDBG_MED("%s:%d] Only one face inside", __func__, __LINE__);
    return 0;
  }

  /* Do nothing if previous coordinates are the same as new
   * Sometimes stabilization is executed twice for same results */
  last_index = (history->index + history->faces_inside - 1) %  history->faces_inside;
  if ((history->entry[history->index].x != history->entry[last_index].x ||
       history->entry[history->index].y != history->entry[last_index].y) &&
      (history->entry[history->index].x == history->stable_entry.x &&
       history->entry[history->index].y == history->stable_entry.y)) {
    IDBG_MED("%s:%d] Stabilization executed twice", __func__, __LINE__);
    return 0;
  }

  /* If there is refer for stabilization use it */
  if (FD_STAB_STATE_STABILIZE != history->state) {
    if (refer) {
      within_limit = module_faceproc_within_limit(refer, &history->stable_refer,
        threshold);
    } else {
      within_limit = module_faceproc_within_limit(&history->entry[history->index],
        &history->stable_entry, threshold);
    }
  }

  new_state = history->state;
  switch (history->state) {
  case FD_STAB_STATE_UNSTABLE:
    if (history->state_count >= history->max_state_count) {
      new_state = FD_STAB_STATE_STABILIZE;
    } else if (TRUE == within_limit) {
      new_state = FD_STAB_STATE_STABLE;
    }
    break;
  case FD_STAB_STATE_STABLE:
    if (TRUE == within_limit) {
      if (module_faceproc_stab_is_continues(stab_mode)) {
        face_stable = module_faceproc_stab_is_stable(history, refer, stab_mode);
        if (FALSE == face_stable) {
          history->stable_entry.x = history->entry[history->index].x;
          history->stable_entry.y = history->entry[history->index].y;
        }
      }
      break;
    } else if (history->max_state_count) {
      new_state = FD_STAB_STATE_UNSTABLE;
      break;
    } else {
      /* do not brake here go to stabilize state directly */
      new_state = FD_STAB_STATE_STABILIZE;
    }
  case FD_STAB_STATE_STABILIZE:
    face_stable = module_faceproc_stab_is_stable(history, refer, stab_mode);
    /* If face is stabilized put to the stable state */
    if (face_stable) {
      if (refer) {
        history->stable_refer = *refer;
      }
      new_state = FD_STAB_STATE_STABLE;
    } else {
      history->state_count = 0;
    }

    if (strength->x != strength->y) {
      FD_STAB_APPLY_TEMP_FILTER(history->stable_entry.x, strength->x,
        history->entry[history->index].x, strength->y);
      FD_STAB_APPLY_TEMP_FILTER(history->stable_entry.y, strength->x,
        history->entry[history->index].y, strength->y);
    } else {
      history->stable_entry.x = history->entry[history->index].x;
      history->stable_entry.y = history->entry[history->index].y;
    }
    break;
  }

  /* Change state if requested */
  if (new_state != history->state) {
    history->state = new_state;
    history->state_count = 0;
  } else {
    history->state_count++;
  }

  return 0;
}

/**
* Function: module_faceproc_roi_sort_pos
*
* Description: CAllback function for qsort used for sorting
*   faces based on face x,y position
*
* Arguments:
*   @arg1 - faceproc_info_t * First Roi
*   @arg2 - faceproc_info_t * Second Roi
*
* Return values:
*   0 - equal
*   1 - position of rec1 is after position of rect2
*  -1 - position of rec1 is before position of rect2
* Notes: none
**/
static int module_faceproc_stab_roi_sort_pos(const void *arg1, const void *arg2)
{
  faceproc_info_t *first_roi = (faceproc_info_t *)arg1;
  faceproc_info_t *second_roi = (faceproc_info_t *)arg2;

  if ((first_roi->face_boundary.x +
      (first_roi->face_boundary.y * MAX_FD_WIDTH)) >
      (second_roi->face_boundary.x +
      (second_roi->face_boundary.y * MAX_FD_WIDTH)))
    return 1;
  else
    return -1;

  return 0;
}

/**
* Function: module_faceproc_faces_stabilization
*
* Description: Function to faces stabilization
*
* Arguments:
*   @p_client - Face proc client
*   @p_result - Face detection result on which filter will be applied
*
* Return values:
*   none
*
* Notes: none
**/
int module_faceproc_faces_stabilization(faceproc_client_t *p_client,
  faceproc_result_t *p_result)
{
  cam_face_detection_data_t faces_data;
  faceproc_stabilization_t *stab;
  faceproc_history_entry_t strength;
  faceproc_history_entry_t refer;
  faceproc_history_entry_t eyes_center;
  faceproc_history_entry_t *p_refer;
  uint32_t i, j, new_faces, threshold, eyes_distance;
  int position;

  if (!p_client) {
   IDBG_ERROR("%s:%d]Invalid input", __func__, __LINE__);
   return IMG_ERR_INVALID_INPUT;
  }

  stab = &p_client->stabilization;

  if (!p_result->num_faces_detected) {
      IDBG_MED("%s:%d] no faces reset the histories", __func__, __LINE__);
      stab->detected_faces = 0;
      return IMG_SUCCESS;
  }

  /* Sort the output ROI */
  qsort(p_result->roi, p_result->num_faces_detected, sizeof(p_result->roi[0]),
    module_faceproc_stab_roi_sort_pos);

  i = 0, j = 0;
  while (i < p_result->num_faces_detected) {
    /* If there are new faces put them them in history */
    if (i >= stab->detected_faces) {
      module_faceproc_stab_init_face(&stab->faces[j++],
        &p_result->roi[i++],
        p_client->p_fd_chromatix);
        stab->detected_faces++;
        continue;
    }

    /* Check boundary limit */
    position = module_faceproc_stab_check_face(&p_result->roi[i], &stab->faces[j]);

    if (FACEPROC_FACE_SAME == position) {

      module_faceproc_stab_add_face(&stab->faces[j++],
        &p_result->roi[i++], p_client->p_fd_chromatix);

    } else if (FACEPROC_FACE_BEFORE == position) {

       /* Move the faces to the right if it is not the last element  */
      if (i < (p_result->num_faces_detected - 1)) {
         memcpy(&stab->faces[j + 1], &stab->faces[j],
           sizeof(stab->faces[0]) * (stab->detected_faces - j));
         stab->detected_faces++;
      }

      /* Put the new face in place */
      module_faceproc_stab_init_face(&stab->faces[j++], &p_result->roi[i++],
        p_client->p_fd_chromatix);

    } else if (FACEPROC_FACE_AFTER == position) {

       /* Move faces to the left and remove current */
       memcpy(&stab->faces[j], &stab->faces[j + 1],
         sizeof(stab->faces[0]) * stab->detected_faces - j);
       stab->detected_faces--;
    }
  }

  if (stab->detected_faces > p_result->num_faces_detected) {
    stab->detected_faces = p_result->num_faces_detected;
  }

  /* Fill output with the filtered value */
  memset(&refer, 0x00, sizeof(refer));
  for (i = 0; i < p_result->num_faces_detected; i++) {
    /* Get eyes distance and center they will be used as reference */
    eyes_distance =
      FD_STAB_CALC_DISTANCE(p_result->roi[i].fp.face_pt[FACE_PART_LEFT_EYE],
      p_result->roi[i].fp.face_pt[FACE_PART_RIGHT_EYE]);
    eyes_center.x = (p_result->roi[i].fp.face_pt[FACE_PART_LEFT_EYE].x +
      p_result->roi[i].fp.face_pt[FACE_PART_RIGHT_EYE].x) / 2;
    eyes_center.y = (p_result->roi[i].fp.face_pt[FACE_PART_LEFT_EYE].y +
      p_result->roi[i].fp.face_pt[FACE_PART_RIGHT_EYE].y) / 2;

    /* Stabilize face size */
    p_refer = NULL;
    if (p_client->p_fd_chromatix->stab_size_use_reference) {
      refer.x = eyes_distance;
      refer.y = eyes_distance;
      p_refer = &refer;
      threshold = FD_STAB_CALC_THRESHOLD(eyes_distance,
        p_client->p_fd_chromatix->stab_size_threshold);
    } else {
      threshold = FD_STAB_CALC_THRESHOLD(stab->faces[i].face_size.stable_entry.x,
        p_client->p_fd_chromatix->stab_size_threshold);
    }
    strength.x = p_client->p_fd_chromatix->stab_size_strength_denum;
    strength.y = p_client->p_fd_chromatix->stab_size_strength_num;

    module_faceproc_stab_filter(&stab->faces[i].face_size, &strength,
      threshold, p_refer, p_client->p_fd_chromatix->stab_size_mode);

    /* Stabilize face position */
    p_refer = NULL;
    if (p_client->p_fd_chromatix->stab_pos_use_reference) {
      refer.x = eyes_center.x;
      refer.y = eyes_center.y;
      p_refer = &refer;
    }
    threshold = FD_STAB_CALC_THRESHOLD(p_client->main_dim.width,
      p_client->p_fd_chromatix->stab_pos_threshold);
    strength.x = p_client->p_fd_chromatix->stab_pos_strength_denum;
    strength.y = p_client->p_fd_chromatix->stab_pos_strength_num;

    module_faceproc_stab_filter(&stab->faces[i].face_position, &strength,
      threshold, p_refer, p_client->p_fd_chromatix->stab_pos_mode);

    /* Stabilize mouth */
    p_refer = NULL;
    if (p_client->p_fd_chromatix->stab_mouth_use_reference) {
      refer.x = eyes_center.x;
      refer.y = eyes_center.y;
      p_refer = &refer;
    }
    threshold = FD_STAB_CALC_THRESHOLD(p_client->main_dim.width,
      p_client->p_fd_chromatix->stab_mouth_threshold);
    strength.x = p_client->p_fd_chromatix->stab_mouth_strength_denum;
    strength.y = p_client->p_fd_chromatix->stab_mouth_strength_num;

    module_faceproc_stab_filter(&stab->faces[i].mouth_position, &strength,
      threshold, p_refer, p_client->p_fd_chromatix->stab_mouth_mode);

    module_faceproc_stab_fill_roi(&stab->faces[i], &p_result->roi[i]);
  }

 return IMG_SUCCESS;
}

