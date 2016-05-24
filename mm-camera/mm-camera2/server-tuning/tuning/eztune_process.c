/***************************************************************************
* Copyright (c) 2011-2014 Qualcomm Technologies, Inc.  All Rights Reserved.*
* Qualcomm Technologies Proprietary and Confidential.                      *
***************************************************************************/
#include "eztune.h"
#include "eztune_process.h"
#include "sensor_common.h"
#include "actuator.h"
#include "mm_qcamera_app.h"

/** tuning_process_get:
 *
 *  @lib_handle: pointer camera library handle
 *  @item: tune_get item
 *  @tune_get_data_t: pointer to the get data
 *
 *  This function process the get commands from the
 *  eztune server
 *
 *  Return: 0 for success and negative error on failure
**/

int tuning_process_get(mm_camera_lib_handle *lib_handle, tune_get_t item,
  tune_get_data_t *data)
{
   int result = 0;
   switch(item) {
   case TUNING_GET_CHROMATIX: {
     result = mm_camera_lib_send_command(lib_handle,
       MM_CAMERA_LIB_GET_CHROMATIX, NULL, &data->chromatix);
      break;
   }

   case TUNING_GET_AFTUNE: {
     result = mm_camera_lib_send_command(lib_handle,
       MM_CAMERA_LIB_GET_AFTUNE, NULL, &data->autofocus);
      break;
   }

   default:
      break;
   }

   return result;
}

/** tuning_process_set:
 *
 *  @lib_handle: pointer camera library handle
 *  @item: tune_set item
 *  @tune_set_data_t: pointer to the set data
 *
 *  This function process the set commands from the eztune
 *  server
 *
 *  Return: 0 for success and negative error on failure
**/

int tuning_process_set(mm_camera_lib_handle *lib_handle, tune_set_t item,
    tune_set_data_t *data)
{

   int result = 0;
   switch(item) {
   case TUNING_SET_AUTOFOCUS_TUNING: {
     result = mm_camera_lib_send_command(lib_handle,
       MM_CAMERA_LIB_SET_AUTOFOCUS_TUNING, &data->act_tuning, NULL);
      break;
   }

   case TUNING_SET_RELOAD_CHROMATIX: {
     result = mm_camera_lib_send_command(lib_handle,
       MM_CAMERA_LIB_SET_RELOAD_CHROMATIX, data->chromatix, NULL);
      break;
   }

   case TUNING_SET_RELOAD_AFTUNE: {
     result = mm_camera_lib_send_command(lib_handle,
       MM_CAMERA_LIB_SET_RELOAD_AFTUNE, data->autofocus, NULL);
      break;
   }

   case TUNING_SET_VFE_COMMAND: {
     result = mm_camera_lib_send_command(lib_handle,
       MM_CAMERA_LIB_SET_VFE_COMMAND, &data->module_cmd, NULL);
      break;
   }

   case TUNING_SET_POSTPROC_COMMAND: {
     result = mm_camera_lib_send_command(lib_handle,
       MM_CAMERA_LIB_SET_POSTPROC_COMMAND, &data->module_cmd, NULL);
      break;
   }

   case TUNING_SET_3A_COMMAND: {
     result = mm_camera_lib_send_command(lib_handle,
       MM_CAMERA_LIB_SET_3A_COMMAND, &data->aaa_cmd, NULL);
      break;
   }

   case TUNING_SET_ACTION_JPEGSNAPSHOT: {
     result = mm_camera_lib_send_command(lib_handle,
       MM_CAMERA_LIB_JPEG_CAPTURE, NULL, NULL);
      break;
   }

   case TUNING_SET_ACTION_RAWSNAPSHOT: {
     result = mm_camera_lib_send_command(lib_handle,
       MM_CAMERA_LIB_RAW_CAPTURE, NULL, NULL);
      break;
   }

   case TUNING_SET_AEC_LOCK: {
     result = mm_camera_lib_send_command(lib_handle,
       MM_CAMERA_LIB_LOCK_AE, &data->action, NULL);
      break;
   }

   case TUNING_SET_AEC_UNLOCK: {
     result = mm_camera_lib_send_command(lib_handle,
       MM_CAMERA_LIB_UNLOCK_AE, &data->action, NULL);
      break;
   }

   case TUNING_SET_AWB_LOCK: {
     result = mm_camera_lib_send_command(lib_handle,
       MM_CAMERA_LIB_LOCK_AWB, &data->action, NULL);
      break;
   }

   case TUNING_SET_AWB_UNLOCK: {
     result = mm_camera_lib_send_command(lib_handle,
       MM_CAMERA_LIB_UNLOCK_AWB, &data->action, NULL);
      break;
   }

   default:
      break;
   }

   return result;
}

/** tuning_set_autofocus:
 *
 *  @ctrl: eztune control structure pointer
 *  @optype: operation type for autofocus
 *  @value: value to be the set
 *
 *  This function process the set commands from the eztune
 *  server for autofocus tuning
 *
 *  Return: void
**/

void tuning_set_autofocus(void *ctrl, aftuning_optype_t optype, uint8_t value)
{
  eztune_t *ezctrl =  (eztune_t *) ctrl;
  tune_set_data_t actuator_set;
  CDBG_EZ("%s %d optype =%d\n", __func__, __LINE__, optype);
  switch (optype) {
    case EZ_AF_LOADPARAMS:
      actuator_set.act_tuning.ttype = ACTUATOR_TUNE_RELOAD_PARAMS;
      break;
    case EZ_AF_LINEARTEST_ENABLE:
      actuator_set.act_tuning.ttype = ACTUATOR_TUNE_TEST_LINEAR;
      actuator_set.act_tuning.stepsize=
        ezctrl->af_tuning.linearstepsize;
      break;
    case EZ_AF_RINGTEST_ENABLE:
      actuator_set.act_tuning.stepsize=
        ezctrl->af_tuning.ringstepsize;
      actuator_set.act_tuning.ttype = ACTUATOR_TUNE_TEST_RING;
      break;
    case EZ_AF_MOVFOCUSTEST_ENABLE:
      actuator_set.act_tuning.ttype = ACTUATOR_TUNE_MOVE_FOCUS;
      actuator_set.act_tuning.direction =
        ezctrl->af_tuning.movfocdirection;
      actuator_set.act_tuning.num_steps =
        ezctrl->af_tuning.movfocsteps;
      break;
    case EZ_AF_DEFFOCUSTEST_ENABLE:
      actuator_set.act_tuning.ttype = ACTUATOR_TUNE_DEF_FOCUS;
      break;
  }
  CDBG_EZ("%s %d\n", __func__, __LINE__);
  tuning_process_set(ezctrl->lib_handle, TUNING_SET_AUTOFOCUS_TUNING,
    &actuator_set);
  return;
}

void tuning_set_vfe(void *ctrl, vfemodule_t module, optype_t optype, int32_t value)
{
  eztune_t *ezctrl =  (eztune_t *) ctrl;
  tune_set_data_t vfe_set;
  vfe_set.module_cmd.module = module;
  vfe_set.module_cmd.type = optype;
  vfe_set.module_cmd.value = value;
  CDBG_EZ("%s %d\n", __func__, __LINE__);
  tuning_process_set(ezctrl->lib_handle, TUNING_SET_VFE_COMMAND,
    &vfe_set);
  return;
}

void tuning_set_pp(void *ctrl, pp_module_t module,
  optype_t optype, int32_t value)
{
  eztune_t *ezctrl =  (eztune_t *) ctrl;
  tune_set_data_t pp_set;
  pp_set.module_cmd.module = module;
  pp_set.module_cmd.type = optype;
  pp_set.module_cmd.value = value;
  CDBG_EZ("%s %d\n", __func__, __LINE__);
  tuning_process_set(ezctrl->lib_handle, TUNING_SET_POSTPROC_COMMAND,
    &pp_set);
  return;
}

void tuning_set_3a(void *ctrl, aaa_set_optype_t optype, int32_t value)
{
  eztune_t *ezctrl =  (eztune_t *) ctrl;
  ez_3a_params_t *aaa_diagnostics = &(ezctrl->diagnostics_3a);
  tune_set_t set_type = TUNING_SET_MAX;
  tune_set_data_t aaa_set;

  switch (optype) {
    case EZ_STATUS:
      set_type = TUNING_SET_3A_COMMAND;
      aaa_set.aaa_cmd.cmd = CAM_EZTUNE_CMD_STATUS;
      aaa_set.aaa_cmd.u.running = value;
      break;

    case EZ_AEC_ENABLE:
      set_type = TUNING_SET_3A_COMMAND;
      aaa_set.aaa_cmd.cmd = CAM_EZTUNE_CMD_AEC_ENABLE;
      aaa_set.aaa_cmd.u.aec_enable =
        aaa_diagnostics->aec_params.enable = value;
      break;
    case EZ_AEC_TESTENABLE:
      aaa_diagnostics->aec_params.test_enable = value;
      break;
    case EZ_AEC_LOCK:
      if (value)
        set_type = TUNING_SET_AEC_LOCK;
      else
        set_type = TUNING_SET_AEC_UNLOCK;
      aaa_set.action =
        aaa_diagnostics->aec_params.lock = value;
      break;
    case EZ_AEC_FORCEPREVEXPOSURE:
      set_type = TUNING_SET_3A_COMMAND;
      aaa_set.aaa_cmd.cmd = CAM_EZTUNE_CMD_AEC_FORCE_EXP;
      aaa_set.aaa_cmd.u.ez_force_param.forced = 1;
      aaa_set.aaa_cmd.u.ez_force_param.u.force_exp_value =
        aaa_diagnostics->aec_params.prev_forceexp = (float)(value)/Q10;
      break;
    case EZ_AEC_FORCEPREVGAIN:
      set_type = TUNING_SET_3A_COMMAND;
      aaa_set.aaa_cmd.cmd = CAM_EZTUNE_CMD_AEC_FORCE_GAIN;
      aaa_set.aaa_cmd.u.ez_force_param.forced = 1;
      aaa_set.aaa_cmd.u.ez_force_param.u.force_gain_value =
        aaa_diagnostics->aec_params.force_prevgain = (float)(value)/Q10;
      break;
    case EZ_AEC_FORCEPREVLINECOUNT:
      set_type = TUNING_SET_3A_COMMAND;
      aaa_set.aaa_cmd.cmd = CAM_EZTUNE_CMD_AEC_FORCE_LINECOUNT;
      aaa_set.aaa_cmd.u.ez_force_param.forced = 1;
      aaa_set.aaa_cmd.u.ez_force_param.u.force_linecount_value =
        aaa_diagnostics->aec_params.force_prevlinecount = value;
      break;
    case EZ_AEC_FORCESNAPEXPOSURE:
      set_type = TUNING_SET_3A_COMMAND;
      aaa_set.aaa_cmd.cmd = CAM_EZTUNE_CMD_AEC_FORCE_SNAP_EXP;
      aaa_set.aaa_cmd.u.ez_force_param.forced = 1;
      aaa_set.aaa_cmd.u.ez_force_param.u.force_snap_exp_value =
        aaa_diagnostics->aec_params.snap_forceexp = (float)(value)/Q10;
      break;
    case EZ_AEC_FORCESNAPGAIN:
      set_type = TUNING_SET_3A_COMMAND;
      aaa_set.aaa_cmd.cmd = CAM_EZTUNE_CMD_AEC_FORCE_SNAP_GAIN;
      aaa_set.aaa_cmd.u.ez_force_param.forced = 1;
      aaa_set.aaa_cmd.u.ez_force_param.u.force_snap_gain_value =
        aaa_diagnostics->aec_params.force_snapgain = (float)(value)/Q10;
      break;
    case EZ_AEC_FORCESNAPLINECOUNT:
      set_type = TUNING_SET_3A_COMMAND;
      aaa_set.aaa_cmd.cmd = CAM_EZTUNE_CMD_AEC_FORCE_SNAP_LC;
      aaa_set.aaa_cmd.u.ez_force_param.forced = 1;
      aaa_set.aaa_cmd.u.ez_force_param.u.force_snap_linecount_value =
        aaa_diagnostics->aec_params.force_snaplinecount = value;
      break;
    case EZ_AWB_ENABLE:
      set_type = TUNING_SET_3A_COMMAND;
      aaa_set.aaa_cmd.cmd = CAM_EZTUNE_CMD_AWB_ENABLE;
      aaa_set.aaa_cmd.u.awb_enable =
        aaa_diagnostics->awb_params.enable = value;
      break;
    case EZ_AWB_LOCK:
      if (value)
        set_type = TUNING_SET_AWB_LOCK;
      else
        set_type = TUNING_SET_AWB_UNLOCK;
      aaa_set.action = aaa_diagnostics->awb_params.lock = value;
      break;
    case EZ_AF_ENABLE:
      set_type = TUNING_SET_3A_COMMAND;
      aaa_set.aaa_cmd.cmd = CAM_EZTUNE_CMD_AF_ENABLE;
      aaa_set.aaa_cmd.u.af_enable =
        aaa_diagnostics->af_params.enable = value;
      break;
    default:
     break;
  }

  CDBG_EZ("%s %d\n", __func__, __LINE__);
  tuning_process_set(ezctrl->lib_handle, set_type, &aaa_set);

   return;
}

void tuning_set_action(void *ctrl, action_set_optype_t optype,
  int32_t value)
{
  eztune_t *ezctrl =  (eztune_t *) ctrl;
  tune_set_t set_type = TUNING_SET_MAX;
  CDBG_EZ("%s %d\n", __func__, __LINE__);
  switch (optype) {
    case EZ_JPEG_SNAPSHOT:
      if (value)
        set_type = TUNING_SET_ACTION_JPEGSNAPSHOT;
      break;
    case EZ_RAW_SNAPSHOT:
      if (value)
        set_type = TUNING_SET_ACTION_RAWSNAPSHOT;
      break;

    default:
      break;
  }

  CDBG_EZ("%s %d\n", __func__, __LINE__);
  tuning_process_set(ezctrl->lib_handle, set_type, NULL);
}
