/**********************************************************************
* Copyright (c) 2013 Qualcomm Technologies, Inc.  All Rights Reserved.*
* Qualcomm Technologies Proprietary and Confidential.                 *
**********************************************************************/
#ifndef __TUNE_PROCESS_H__
#define __TUNE_PROCESS_H__

#include "cam_types.h"
#include "eztune_diagnostics.h"
#define Q10 1024
#include "aec.h"
#include "awb.h"
#include "af.h"

typedef struct {
   tune_chromatix_t chromatix;
   tune_autofocus_t autofocus;
}tune_get_data_t;

typedef enum {
   TUNING_GET_CHROMATIX,
   TUNING_GET_AFTUNE,
}tune_get_t;

typedef struct {
   tune_chromatix_t *chromatix;
   tune_autofocus_t *autofocus;
   tune_actuator_t act_tuning;
   tune_cmd_t module_cmd;
   int32_t action;
   cam_eztune_cmd_data_t aaa_cmd;
}tune_set_data_t;

typedef enum {
   TUNING_SET_RELOAD_CHROMATIX,
   TUNING_SET_RELOAD_AFTUNE,
   TUNING_SET_AUTOFOCUS_TUNING,
   TUNING_SET_VFE_COMMAND,
   TUNING_SET_POSTPROC_COMMAND,
   TUNING_SET_3A_COMMAND,
   TUNING_SET_ACTION_JPEGSNAPSHOT,
   TUNING_SET_ACTION_RAWSNAPSHOT,
   TUNING_SET_AEC_LOCK,
   TUNING_SET_AEC_UNLOCK,
   TUNING_SET_AWB_LOCK,
   TUNING_SET_AWB_UNLOCK,
   TUNING_SET_MAX
}tune_set_t;


int tuning_process_get(mm_camera_lib_handle *lib_handle, tune_get_t item,
  tune_get_data_t *data);

int tuning_process_set(mm_camera_lib_handle *lib_handle, tune_set_t item,
  tune_set_data_t *data);

void tuning_set_autofocus(void *ctrl,
  aftuning_optype_t optype, uint8_t value);
void tuning_set_vfe(void *ctrl, vfemodule_t module,
  optype_t optype, int32_t value);
void tuning_set_3a(void *ctrl, aaa_set_optype_t optype,
  int32_t value);
void tuning_set_action(void *ctrl, action_set_optype_t optype,
  int32_t value);
void tuning_set_pp(void *ctrl, pp_module_t module,
  optype_t optype, int32_t value);
#endif //__TUNE_PROCESS_H__

