/**********************************************************************
* Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved. *
* Qualcomm Technologies Proprietary and Confidential.                 *
**********************************************************************/

#ifndef __IMG_COMP_H__
#define __IMG_COMP_H__

#include "img_common.h"
#include <dlfcn.h>

/** QIMG_DENOISE_CMD_OFF
 *
 *  Userdefined command offset for denoise component
 **/
#define QIMG_DENOISE_CMD_OFF   0x100

/** QIMG_FACEPROC_CMD_OFF
 *
 *  Userdefined command offset for faceproc component
 **/
#define QIMG_FACEPROC_CMD_OFF  0x200

/** QIMG_HDR_CMD_OFF
 *
 *  Userdefined command offset for HDR component
 **/
#define QIMG_HDR_CMD_OFF       0x300

/** QIMG_CAC_CMD_OFF
 *
 *  Userdefined command offset for CAC component
 **/
#define QIMG_CAC_CMD_OFF       0x400


/** QIMG_DENOISE_PARAM_OFF
 *
 *  Userdefined paramter offset for denoise component
 **/
#define QIMG_DENOISE_PARAM_OFF    0x100

/** QIMG_FACEPROC_PARAM_OFF
 *
 *  Userdefined paramter offset for faceproc component
 **/
#define QIMG_FACEPROC_PARAM_OFF   0x200

/** QIMG_HDR_PARAM_OFF
 *
 *  Userdefined paramter offset for HDR component
 **/
#define QIMG_HDR_PARAM_OFF        0x300

/** QIMG_CAC_PARAM_OFF
 *
 *  Userdefined paramter offset for CAC component
 **/
#define QIMG_CAC_PARAM_OFF        0x400


/**
 *
 *  Common parameters used by all components
 **/
#define QIMG_PARAM_FRAME_INFO 0
#define QIMG_PARAM_MODE       1
#define QIMG_CAMERA_DUMP      2

/** img_event_type
 *  QIMG_EVT_ERROR: error event
 *  QIMG_EVT_DONE: event called when the component has completed
 *                 execution
 *  QIMG_EVT_BUF_DONE: event called when the component has
 *                    completed processing the buffer
 *  QIMG_EVT_FACE_PROC: event called when the face processing is
 *                     completed
 *
 *  Imaging event type
 **/
typedef enum {
  QIMG_EVT_ERROR,
  QIMG_EVT_DONE,
  QIMG_EVT_BUF_DONE,
  QIMG_EVT_FACE_PROC,
  QIMG_EVT_EARLY_CB_DONE,
} img_event_type;

/** img_event_t
 *   @type: event type
 *   @status: event status
 *
 *   Event data structure
 **/
typedef struct {
  img_event_type type;
  union {
    int status;
  }d;
} img_event_t;

/** comp_state_t
 *   IMG_STATE_IDLE: Idle state, component is uninitialized
 *   IMG_STATE_INIT: State indicates that the component is
 *                   initialized
 *   IMG_STATE_STARTED: State indicates that the component is
 *                      active
 *   IMG_STATE_STOP_REQUESTED: State indicates that the stop is
 *                            is issued from the client and is
 *                            in the process of stopping
 *   IMG_STATE_STOPPED: State indicates that the component has
 *                      stopped execution
 **/
typedef enum {
  IMG_STATE_IDLE,
  IMG_STATE_INIT,
  IMG_STATE_STARTED,
  IMG_STATE_STOP_REQUESTED,
  IMG_STATE_STOPPED,
} comp_state_t;

/** img_type_t
 *   IMG_TYPE_MAIN: main image
 *   IMG_TYPE_THUMB: thumbnail image
 *
 **/
typedef enum {
  IMG_TYPE_MAIN,
  IMG_TYPE_THUMB,
} img_type_t;

/** img_cmd_type
 *
 *  command type
 **/
typedef int img_cmd_type;

/** img_param_type
 *
 *  paramter type
 **/
typedef int img_param_type;

/** notify_cb
 *
 *  Notify callback function
 **/
typedef int (*notify_cb) (void* p_appdata, img_event_t *p_event);

/** img_component_ops_t
*    @init: function pointer for initialization functionality
*    @deinit:  function pointer for uninitialization
*           functionality
*    @set_parm: function pointer for set parameter functionality
*    @get_parm: function pointer for get parameter functionality
*    @set_callback: function pointer for setcallback
*                 functionality
*    @start: function pointer for start functionality
*    @abort: function pointer for abort functionality
*    @process: function pointer for process functionality
*    @queue_buffer: function pointer for queue buffer
*    @deque_buffer: function pointer for dequeue buffer
*    @handle: component handle
*
*    Component ops table
**/
typedef struct {
  //
  int (*init)(void *handle, void* p_userdata, void *p_data);
  //
  int (*deinit)(void *handle);
  //
  int (*set_parm)(void *handle, img_param_type param, void *p_data);
  //
  int (*get_parm)(void *handle, img_param_type param, void* p_data);
  //
  int (*set_callback)(void *handle, notify_cb notify);
  //
  int (*start)(void *handle, void *p_data);
  //
  int (*abort)(void *handle, void *p_data);
  //
  int (*process)(void *handle, img_cmd_type cmd, void *p_data);
  //
  int (*queue_buffer)(void *handle, img_frame_t *p_frame, img_type_t type);
  //
  int (*deque_buffer)(void *handle, img_frame_t **pp_frame);
  //
  comp_state_t (*get_state)(void *handle);
  //
  void *handle;

} img_component_ops_t;

#endif //__IMG_COMP_H__
