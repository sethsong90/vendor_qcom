/***************************************************************************
* Copyright (c) 2010-2013 Qualcomm Technologies, Inc.  All Rights Reserved.*
* Qualcomm Technologies Proprietary and Confidential.                      *
***************************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <utils/Log.h>
#include <sys/time.h>
#include <pthread.h>
#include "eztune.h"
#include "eztune_preview.h"

#define EZTUNE_DESIRED_CHUNK_SIZE    7168
#define EZTUNE_MAX_CHUNK_SIZE        10240

static eztune_preview_ctrl_t ez_preview_ctrl;

/** eztune_copy_preview_frame:
 *
 *  @frame: the new preveiw frame from preview_callback
 *
 *  Copy a new preveiw frame to local buffer, so we can send it
 *  to client when requested. This function will be called every
 *  time when a new preview frame is available from
 *  mm-camera-intf.
 *
 *  Return: void
**/
static void eztune_copy_preview_frame(mm_camera_buf_def_t *frame)
{
  int i = 0;
  eztune_preview_ctrl_t* ctrl = &ez_preview_ctrl;

  if (ctrl->frame_avail) {
    CDBG_EZ("%s, frame dropped, frame_idx: %d", __func__, frame->frame_idx);
    return;
  }

  if (ctrl->prev_frame.data_buf != NULL) {
    uint32_t offset = 0;
    for (i = 0; i < frame->num_planes; i++) {
      memcpy(ctrl->prev_frame.data_buf + offset,
        (uint8_t *)frame->buffer + offset, frame->planes[i].length);

      offset +=  frame->planes[i].length;
    }

    CDBG_EZ("%s, copy frame to eztune server, frame_idx: %d, frame_size: %d",
      __func__, frame->frame_idx, offset);

    ctrl->cur_frame_idx = frame->frame_idx;
    ctrl->frame_avail = 1;
  } else {
    CDBG_EZ("%s, [ERR] frame_ptr is NULL", __func__);
  }

  return;
}

/** eztune_preview_usercb:
 *
 *  @preview_frame: the new preveiw frame from preview_callback
 *
 *  When preview_callback is called, it'll call this custom callback function.
 *
 *  Return: void
**/
void eztune_preview_usercb(mm_camera_buf_def_t *preview_frame)
{
  if (ez_preview_ctrl.need_cp_frame)
    eztune_copy_preview_frame(preview_frame);

  return;
}

/** eztune_prev_write_status:
 *
 *  @void:
 *
 *  write the status to the host for the items which are not
 *  supported now.
 *
 *  Return: 0 for success and negative error on failure
**/
int eztune_prev_write_status(char **send_buf, uint32_t *send_len)
{
  int32_t rc = 0;
  int status = EZTUNE_STATUS_MINUS_ONE;
  eztune_preview_ctrl_t* ctrl = &ez_preview_ctrl;

  *send_buf = (char *)calloc(sizeof(status), sizeof(char *));
  if (*send_buf == NULL) {
    ALOGE("%s:Error allocating memory for send_buf %s\n",__func__,
      strerror(errno));
    return -1;
  }
  *send_len = sizeof(status);
  memcpy(*send_buf, &status, sizeof(status));

  return rc;
}

/** eztune_get_preview_frame:
 *
 *  @void:
 *
 *  write the preview frame data to the host.
 *
 *  Return: 0 for success and negative error on failure
**/
int eztune_get_preview_frame(char **send_buf, uint32_t *send_len)
{
  int32_t rc = 0;
  uint32_t cnk_size = 0;
  char *data_buf = NULL;
  uint32_t frame_size = 0;
  eztune_preview_ctrl_t* ctrl = &ez_preview_ctrl;
  eztune_prevcmd_rsp *rsp_ptr=NULL, *rspn_ptr=NULL, *head_ptr=NULL;

  cnk_size = ctrl->ch_cnk_size.cnk_size;
  data_buf = ctrl->prev_frame.data_buf;
  frame_size = ctrl->frame_header.frame_size;

  CDBG_EZ("%s %d\n", __func__, __LINE__);

  if(!ctrl->frame_avail) {
    ctrl->frame_header.status = 1;
    ctrl->frame_header.frame_size= 0;

    CDBG_EZ("%s data not available\n", __func__);

    CDBG_EZ("%s get frame %d %d %d %d %d %d\n",
         __func__,
         ctrl->frame_header.status,
         ctrl->frame_header.width,
         ctrl->frame_header.height,
         ctrl->frame_header.format,
         ctrl->frame_header.origin,
         ctrl->frame_header.frame_size);
    rsp_ptr = (eztune_prevcmd_rsp *)malloc(sizeof(eztune_prevcmd_rsp));
    if (!rsp_ptr) {
      ALOGE("%s:Error allocating memory for rsp_ptr1 %s\n",__func__,
        strerror(errno));
      return -1;
    }

    rsp_ptr->send_buf =  (char *)calloc(sizeof(ctrl->frame_header), sizeof(char *));
    if (!rsp_ptr->send_buf) {
      ALOGE("%s:Error allocating memory for send_buf %s\n",__func__,
        strerror(errno));
      return -1;
    }
    rsp_ptr->send_len = sizeof(ctrl->frame_header);
    memcpy(rsp_ptr->send_buf, &ctrl->frame_header, sizeof(ctrl->frame_header));
    rsp_ptr->next = NULL;
    *send_buf =  (char *)rsp_ptr;

    ctrl->frame_header.frame_size= frame_size;
    ctrl->frame_avail = 0;
  } else {
    ctrl->frame_header.status = 0;
    rsp_ptr = (eztune_prevcmd_rsp *)malloc(sizeof(eztune_prevcmd_rsp));
    if (!rsp_ptr) {
      ALOGE("%s:Error allocating memory for rsp_ptr2 %s\n",__func__,
        strerror(errno));
      return -1;
    }
    head_ptr = rsp_ptr;
    rsp_ptr->send_buf =  (char *)calloc(sizeof(ctrl->frame_header), sizeof(char *));
    if (!rsp_ptr->send_buf) {
      ALOGE("%s:Error allocating memory for send_buf %s\n",__func__,
        strerror(errno));
      return -1;
    }
    rsp_ptr->send_len = sizeof(ctrl->frame_header);
    memcpy(rsp_ptr->send_buf, &ctrl->frame_header, sizeof(ctrl->frame_header));
    rsp_ptr->next = NULL;
    CDBG_EZ("%s data available\n", __func__);
    CDBG_EZ("%s get frame %d %d %d %d %d %d\n",
         __func__,
         ctrl->frame_header.status,
         ctrl->frame_header.width,
         ctrl->frame_header.height,
         ctrl->frame_header.format,
         ctrl->frame_header.origin,
         ctrl->frame_header.frame_size);

    /* Write preview frame in multiples of chunk size */
    while(1) {
      if(frame_size > cnk_size) {
        rspn_ptr = (eztune_prevcmd_rsp *)malloc(sizeof(eztune_prevcmd_rsp));
        if (!rspn_ptr) {
          ALOGE("%s:Error allocating memory for rsp_ptr3 %s\n",__func__,
            strerror(errno));
          return -1;
        }
        rsp_ptr->next = (eztune_prevcmd_rsp *)rspn_ptr;
        rspn_ptr->next = NULL;
        rspn_ptr->send_buf =  (char *)calloc(cnk_size, sizeof(char *));
        if (!rspn_ptr->send_buf) {
          ALOGE("%s:Error allocating memory for send_buf %s\n",__func__,
            strerror(errno));
          return -1;
        }
        rspn_ptr->send_len = cnk_size;
        memcpy(rspn_ptr->send_buf, data_buf, cnk_size);
        rsp_ptr = rspn_ptr;
        data_buf += cnk_size;
        frame_size -= cnk_size;
      }
      else {
        rspn_ptr = (eztune_prevcmd_rsp *)malloc(sizeof(eztune_prevcmd_rsp));
        if (!rspn_ptr) {
          ALOGE("%s:Error allocating memory for rsp_ptr4 %s\n",__func__,
            strerror(errno));
          return -1;
        }
        rsp_ptr->next = (eztune_prevcmd_rsp *)rspn_ptr;
        rspn_ptr->next = NULL;
        rspn_ptr->send_buf =  (char *)calloc(frame_size, sizeof(char *));
        if (!rspn_ptr->send_buf) {
          ALOGE("%s:Error allocating memory for send_buf %s\n",__func__,
            strerror(errno));
          return -1;
        }
        rspn_ptr->send_len = frame_size;
        memcpy(rspn_ptr->send_buf, data_buf, frame_size);
        *send_buf =  (char *)head_ptr;
        break;
      }
    }

    ctrl->frame_avail = 0;
  }

  return rc;
}

/** eztune_prev_get_info:
 *
 *  @void:
 *
 *  write the capabilities and inforamtion to the host.
 *
 *  Return: 0 for success and negative error on failure
**/
int eztune_prev_get_info(char **send_buf, uint32_t *send_len)
{
  int32_t rc = 0;
  eztune_preview_ctrl_t* ctrl = &ez_preview_ctrl;
  eztune_prevcmd_rsp *rsp_ptr=NULL, *rsp1_ptr=NULL;

  CDBG_EZ("%s\n", __func__);
  CDBG_EZ("%s get info ver %d %d %d\n",
       __func__,
       ctrl->info_ver.major_ver,
       ctrl->info_ver.minor_ver,
       ctrl->info_ver.header_size);
  CDBG_EZ("%s get info caps %d %d %d\n",
       __func__,
       ctrl->info_caps.target_type,
       ctrl->info_caps.capabilities,
       ctrl->info_caps.cnk_size);

  rsp_ptr = (eztune_prevcmd_rsp *)malloc(sizeof(eztune_prevcmd_rsp));
  if (!rsp_ptr) {
    ALOGE("%s: Error allocating memory for rsp_ptr", __func__);
    return -1;
  }

  rsp_ptr->send_buf = (char *)calloc(sizeof(ctrl->info_ver), sizeof(char *));
  if (!rsp_ptr->send_buf) {
    ALOGE("%s: Error allocating memory for send_buf",__func__);
    return -1;
  }
  rsp_ptr->send_len = sizeof(ctrl->info_ver);
  memcpy(rsp_ptr->send_buf, &ctrl->info_ver, sizeof(ctrl->info_ver));

  rsp1_ptr = (eztune_prevcmd_rsp *)malloc(sizeof(eztune_prevcmd_rsp));
  if (!rsp1_ptr) {
    ALOGE("%s:Error allocating memory for rsp1_ptr",__func__);
    return -1;
  }

  rsp_ptr->next = rsp1_ptr;
  rsp1_ptr->next = NULL;
  rsp1_ptr->send_buf =  (char *)calloc(sizeof(ctrl->info_caps), sizeof(char *));
    if (!rsp1_ptr->send_buf) {
      ALOGE("%s:Error allocating memory for send_buf %s\n",__func__,
        strerror(errno));
      return -1;
    }
  rsp1_ptr->send_len = sizeof(ctrl->info_caps);
  memcpy(rsp1_ptr->send_buf, &ctrl->info_caps, sizeof(ctrl->info_caps));
  *send_buf =  (char *)rsp_ptr;

  return rc;
}

/** eztune_prev_ch_cnk_size:
 *
 *  @new_chunk_size: new chunksize to be set
 *
 *  write the current chunk size to the host.
 *
 *  Return: 0 for success and negative error on failure
**/
int eztune_prev_ch_cnk_size(uint32_t new_chunk_size, char **send_buf, uint32_t *send_len)
{
  int32_t rc = 0;
  eztune_preview_ctrl_t* ctrl = &ez_preview_ctrl;

  CDBG_EZ("%s\n", __func__);
  CDBG_EZ("%s new chunk size = %d\n", __func__,
                       new_chunk_size);

  if(new_chunk_size <= EZTUNE_MAX_CHUNK_SIZE) {
    ctrl->ch_cnk_size.status = EZTUNE_STATUS_ZERO;
    ctrl->ch_cnk_size.cnk_size = new_chunk_size;
  }
  else {
    ctrl->ch_cnk_size.status = EZTUNE_STATUS_TWO;
    ctrl->ch_cnk_size.cnk_size = EZTUNE_MAX_CHUNK_SIZE;
  }

  CDBG_EZ("%s cur chunk size = %d\n", __func__,
            ctrl->ch_cnk_size.cnk_size);

  CDBG_EZ("%s ch cnk size %d %d\n",
       __func__,
      ctrl->ch_cnk_size.status,
      ctrl->ch_cnk_size.cnk_size);

  *send_buf = (char *)calloc(sizeof(ctrl->ch_cnk_size), sizeof(char *));
  if (*send_buf == NULL) {
    ALOGE("%s:Error allocating memory for send_buf %s\n",__func__,
      strerror(errno));
    return -1;
  }
  *send_len = sizeof(ctrl->ch_cnk_size);
  memcpy(*send_buf, &ctrl->ch_cnk_size, sizeof(ctrl->ch_cnk_size));

  return rc;
}

/** eztune_preview_init_frame_setting
 *
 *  @dim: the dimension info of preview frame
 *
 *  Initialize preview frame info according to preveiw dimension.
 *  Also allocate  the memory for storing a preview frame locally.
 *
 *  Return: >=0 on success, -1 on failure.
 **/
static int eztune_preview_init_frame_setting(cam_dimension_t dim)
{
  eztune_preview_ctrl_t* ctrl = &ez_preview_ctrl;

  /* Initialize version info */
  ctrl->info_ver.major_ver     = 1;
  ctrl->info_ver.minor_ver     = 0;
  ctrl->info_ver.header_size   = sizeof(ctrl->info_caps);

  /* Initialize caps info */
  ctrl->info_caps.target_type  = 1;
  ctrl->info_caps.capabilities = 0x01;
  ctrl->info_caps.cnk_size     = EZTUNE_DESIRED_CHUNK_SIZE;

  /* Initiatlize ch_cnk_size */
  ctrl->ch_cnk_size.cnk_size   = ctrl->info_caps.cnk_size;

  /* Initialize frame_header */
  if (dim.width <= 0 || dim.height <= 0) {
    ctrl->frame_header.width  = DEFAULT_PREVIEW_WIDTH;  /* 640 */
    ctrl->frame_header.height = DEFAULT_PREVIEW_HEIGHT; /* 480 */
  } else {
    ctrl->frame_header.width  = dim.width;
    ctrl->frame_header.height = dim.height;
  }
  ctrl->frame_header.format = EZTUNE_FORMAT_YCrCb_420;

  ctrl->frame_header.origin     = EZTUNE_ORIGIN_BOTTOM_LEFT;
  ctrl->frame_header.frame_size =
    ((uint32_t)ctrl->frame_header.width * ctrl->frame_header.height * 3) / 2;

  CDBG_EZ("%s, frame header: w(%d), h(%d), size(%d), format(%d), origin(%d)",
    __func__,
    ctrl->frame_header.width,
    ctrl->frame_header.height,
    ctrl->frame_header.frame_size,
    ctrl->frame_header.format,
    ctrl->frame_header.origin);

  /* allocat frame_data buffer */
  ctrl->prev_frame.data_buf = (char *)malloc(ctrl->frame_header.frame_size);
  if (!ctrl->prev_frame.data_buf) {
    CDBG_EZ_ERROR("%s, [ERR] memory alloc failed\n", __func__);
    return -1;
  }

  return 0;
}

/** eztune_preview_init:
 *
 *  @pr_client_socket_id: client socket id
 *
 *  Initialization on connected to the host.
 *
 *  Return: 0 for success and negative error on failure
**/
int eztune_preview_init(int pr_client_socket_id)
{
  eztune_preview_ctrl_t *ctrl = &ez_preview_ctrl;
  ctrl->client_socket_id = pr_client_socket_id;

  return 0;
}

/** eztune_preview_set_dimension:
 *
 *  @dim: width and height of the preview frame
 *
 *  Initialize the dimension.
 *
 *  Return: 0 for success and negative error on failure
**/
int eztune_preview_set_dimension(cam_dimension_t dim)
{
  if (eztune_preview_init_frame_setting(dim) < 0) {
    CDBG_EZ_ERROR("%s,init_frame_setting() error\n", __func__);
    goto ERROR;
  }
  ez_preview_ctrl.need_cp_frame = 1;

  return 0;
ERROR:
  eztune_preview_deinit();
  return -1;
}

/** eztune_preview_deinit:
 *
 *  @void:
 *
 *  Deinitialize the connection from the host.
 *
 *  Return: 0 for success and negative error on failure
**/
int eztune_preview_deinit(void)
{
   eztune_preview_ctrl_t* ctrl = &ez_preview_ctrl;

  ez_preview_ctrl.client_socket_id = 0;

  CDBG_EZ("%s, E.", __func__);

  if (ctrl->prev_frame.data_buf != NULL) {
    CDBG_EZ("%s, free_buffer: frame_data_ptr", __func__);
    free(ctrl->prev_frame.data_buf);
    ctrl->prev_frame.data_buf = NULL;
  }

  return 0;
}
