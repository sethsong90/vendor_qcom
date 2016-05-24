/**********************************************************************
* Copyright (c) 2010-2012 Qualcomm Technologies, Inc.  All Rights Reserved.*
* Qualcomm Technologies Proprietary and Confidential.                              *
**********************************************************************/

#ifndef __EZTUNE_PREVIEW_H__
#define __EZTUNE_PREVIEW_H__

#include <stdint.h>
#include "cam_types.h"
#include "mm_qcamera_app.h"
#pragma pack(push, 1)

#define EZTUNE_STATUS_MINUS_ONE    -1
#define EZTUNE_STATUS_ZERO          0
#define EZTUNE_STATUS_ONE           1
#define EZTUNE_STATUS_TWO           2

typedef enum {
  EZTUNE_FORMAT_JPG = 0,
  EZTUNE_FORMAT_YUV_422,
  EZTUNE_FORMAT_YUV_420,
  EZTUNE_FORMAT_YVU_422,
  EZTUNE_FORMAT_YVU_420,
  EZTUNE_FORMAT_YCrCb_422,
  EZTUNE_FORMAT_YCrCb_420,
  EZTUNE_FORMAT_YCbCr_422,
  EZTUNE_FORMAT_YCbCr_420,
  EZTUNE_FORMAT_INVALID
} eztune_prev_format_t;

typedef enum {
  EZTUNE_ORIGIN_TOP_LEFT = 1,
  EZTUNE_ORIGIN_BOTTOM_LEFT,
  EZTUNE_ORIGIN_INVALID
} eztune_prev_origin_t;

typedef struct {
  uint8_t      major_ver;
  uint8_t      minor_ver;
  uint16_t     header_size;
} eztune_preview_ver_t;

typedef struct {
  uint8_t           target_type;
  uint8_t           capabilities;
  uint32_t          cnk_size;
} eztune_preview_caps_t;

typedef struct {
  uint8_t           status;
  uint32_t          cnk_size;
} eztune_preview_ch_size_t;

typedef struct {
  uint8_t           status;
  uint16_t          width;
  uint16_t          height;
  uint8_t           format;
  uint8_t           origin;
  uint32_t          frame_size;
} eztune_preview_frame_hdr_t;

typedef struct {
  char             *data_buf;
} eztune_preview_frame_data_t;

typedef struct {
  int                          client_socket_id;
  volatile uint8_t             need_cp_frame;
  volatile uint8_t             frame_avail;
  eztune_preview_ver_t         info_ver;
  eztune_preview_caps_t        info_caps;
  eztune_preview_ch_size_t     ch_cnk_size;
  eztune_preview_frame_hdr_t   frame_header;
  eztune_preview_frame_data_t  prev_frame;
  uint32_t                     cur_frame_idx;
} eztune_preview_ctrl_t;

int eztune_preview_init(int client_socket_id);
int eztune_preview_deinit();
int eztune_preview_set_dimension(cam_dimension_t dim);
int eztune_prev_get_info(char **send_buf, uint32_t *send_len);
int eztune_prev_ch_cnk_size(uint32_t new_chunk_size, char **send_buf, uint32_t *send_len);
int eztune_get_preview_frame(char **send_buf, uint32_t *send_len);
int eztune_prev_write_status(char **send_buf, uint32_t *send_len);
void eztune_preview_usercb(mm_camera_buf_def_t *preview_frame);

#pragma pack(pop)
#endif /* __EZTUNE_PREVIEW_H__ */



