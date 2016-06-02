/* awb_port.c
 *
 * Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include "awb_port.h"
#include "awb.h"

#include "q3a_thread.h"
#include "q3a_port.h"
#include "mct_stream.h"
#include "mct_module.h"
#include "modules.h"

#if 0
#undef CDBG
#define CDBG CDBG_ERROR
#endif
/* Every AWB sink port ONLY corresponds to ONE session */

typedef enum {
  AWB_PORT_STATE_CREATED,
  AWB_PORT_STATE_RESERVED,
  AWB_PORT_STATE_LINKED,
  AWB_PORT_STATE_UNLINKED,
  AWB_PORT_STATE_UNRESERVED
} awb_port_state_t;

/** awb_port_private
 *    @awb_object: session index
 *    @port:       stream index
 *
 * Each awb moduld object should be used ONLY for one Bayer
 * serssin/stream set - use this structure to store session
 * and stream indices information.
 **/
typedef struct _awb_port_private {
  unsigned int      reserved_id;
  cam_stream_type_t stream_type;
  awb_port_state_t  state;
  stats_update_t    awb_update_data;
  boolean           awb_update_flag;
  awb_object_t      awb_object;
  q3a_thread_data_t *thread_data;
  uint32_t          cur_sof_id;
  uint8_t           awb_stats_proc_freq;
  uint8_t           awb_stats_frame_count;
  uint32_t          algo_opt_mask;
} awb_port_private_t;


/** awb_send_event
 *
 **/
static void awb_send_events(mct_port_t *port, stats_update_t *awb_update_data)
{
  awb_port_private_t *private = (awb_port_private_t *)(port->port_private);
  mct_event_t event;
  MCT_OBJECT_LOCK(port);
  if (private->awb_update_flag == FALSE) {
    CDBG("No AWB update event to send");
    MCT_OBJECT_UNLOCK(port);
    return;
  }else
    private->awb_update_flag = FALSE;
  MCT_OBJECT_UNLOCK(port);
  /* pack it in a event and send out*/
  event.direction = MCT_EVENT_UPSTREAM;
  event.identity = private->reserved_id;
  event.type = MCT_EVENT_MODULE_EVENT;
  event.u.module_event.type = MCT_EVENT_MODULE_STATS_AWB_UPDATE;
  event.u.module_event.module_event_data = (void *)(awb_update_data);
  CDBG("%s: send MCT_EVENT_MODULE_STATS_AWB_UPDATE to port =%p, event =%p",
        __func__,port, &event);
  MCT_PORT_EVENT_FUNC(port)(port, &event);
  return;
}
#if 0
static void awb_port_send_exif_info_update(
   mct_port_t  *port,
   awb_output_data_t *output)
{
  mct_event_t event;
  mct_bus_msg_t bus_msg;
  unsigned short q3a_msg[sizeofAWBExifDataArray];
  awb_port_private_t *private;
  int size;
  if (!output || !port) {
    ALOGE("%s: input error", __func__);
    return;
  }
  private = (awb_port_private_t *)(port->port_private);
  bus_msg.sessionid = (private->reserved_id >> 16);
  bus_msg.type = MCT_BUS_MSG_AWB_EXIF_INFO;
  bus_msg.msg = (void *)&q3a_msg;
  size = ((int)sizeof(q3a_msg) < output->exif_info_size)?
    (int)sizeof(q3a_msg) : output->exif_info_size;
  bus_msg.size = size;
  memcpy(&q3a_msg[0], output->pexif_info, size);
  output->exif_info_size = 0;
  event.direction = MCT_EVENT_UPSTREAM;
  event.identity = private->reserved_id;
  event.type = MCT_EVENT_MODULE_EVENT;
  event.u.module_event.type = MCT_EVENT_MODULE_STATS_POST_TO_BUS;
  event.u.module_event.module_event_data = (void *)(&bus_msg);
  CDBG("%s: send AWB exif to port", __func__);
  MCT_PORT_EVENT_FUNC(port)(port, &event);
}
#endif

/** awb_port_send_awb_info_to_metadata
 *  update awb info which required by eztuing
 **/

static void awb_port_send_awb_info_to_metadata(
  mct_port_t  *port,
  awb_output_data_t *output)
{
  mct_event_t               event;
  mct_bus_msg_t             bus_msg;
  awb_output_eztune_data_t  awb_info;
  awb_port_private_t        *private;
  int                       size;

  if (!output || !port) {
    ALOGE("%s: input error", __func__);
    return;
  }

  /* If eztune is not running, no need to send eztune metadata */
  if (FALSE == output->eztune_data.ez_running) {
    return;
  }

  private = (awb_port_private_t *)(port->port_private);
  bus_msg.sessionid = (private->reserved_id >> 16);
  bus_msg.type = MCT_BUS_MSG_AWB_EZTUNING_INFO;
  bus_msg.msg = (void *)&awb_info;
  size = (int)sizeof(awb_output_eztune_data_t);
  bus_msg.size = size;

  memcpy(&awb_info, &output->eztune_data,
    sizeof(awb_output_eztune_data_t));

  CDBG("%s: r_gain:%f, g_gain:%d, luma:%d, b_gain:%f,color_temp:%d", __func__,
    awb_info.r_gain, awb_info.g_gain,awb_info.luma, awb_info.b_gain, awb_info.color_temp);
  event.direction = MCT_EVENT_UPSTREAM;
  event.identity = private->reserved_id;
  event.type = MCT_EVENT_MODULE_EVENT;
  event.u.module_event.type = MCT_EVENT_MODULE_STATS_POST_TO_BUS;
  event.u.module_event.module_event_data = (void *)(&bus_msg);
  MCT_PORT_EVENT_FUNC(port)(port, &event);
}


/** awb_port_callback
 *
 **/
static void awb_port_callback(awb_output_data_t *output, void *p)
{
  mct_port_t  *port = (mct_port_t *)p;
  awb_gain_t rgb_gain;
  awb_port_private_t *private = NULL;

  if (!output || !port) {
    ALOGE("%s: input error", __func__);
    return;
  }
  private = (awb_port_private_t *)(port->port_private);
  if (!private)
      return;
  /* populate the stats_upate object to be sent out*/
    CDBG("%s: STATS_UPDATE_AWB", __func__);
    if (AWB_UPDATE == output->type) {
      if (output->eztune_data.ez_running == TRUE) {
        awb_port_send_awb_info_to_metadata(port, output);
      }
      /* Update stats_proc_freq with a valid value only */
      if (output->awb_stats_proc_freq) {
        private->awb_stats_proc_freq = output->awb_stats_proc_freq;
      }
      output->stats_update.flag = STATS_UPDATE_AWB;
      /* memset the output struct */
      memset(&output->stats_update.awb_update, 0,
        sizeof(output->stats_update.awb_update));
      /*RGB gain*/
      rgb_gain.r_gain = output->r_gain;
      rgb_gain.g_gain = output->g_gain;
      rgb_gain.b_gain = output->b_gain;
      /* color_temp */

      output->stats_update.awb_update.gain = rgb_gain;
      output->stats_update.awb_update.color_temp = output->color_temp;
      output->stats_update.awb_update.wb_mode = output->wb_mode;
      output->stats_update.awb_update.best_mode = output->best_mode;

      /* TBD: grey_world_stats is always true for bayer. For YUV change later */
      output->stats_update.awb_update.grey_world_stats = TRUE;
      memcpy(output->stats_update.awb_update.sample_decision,
        output->samp_decision, sizeof(int) * 64);
      CDBG("%s: awb update: (r,g,b gain)= (%f, %f, %f), color_temp=%d",
            __func__, output->stats_update.awb_update.gain.r_gain,
             output->stats_update.awb_update.gain.g_gain,
            output->stats_update.awb_update.gain.b_gain,
            output->stats_update.awb_update.color_temp);

      if(output->frame_id != private->cur_sof_id) {
        /* This will be called when the events happen in the the order
          STAT1 SOF2 OUTPUT1. It ensures the AWB outputs
          are always synchronouse with SOFs  */
        MCT_OBJECT_LOCK(port);
        private->awb_update_flag = TRUE;
        MCT_OBJECT_UNLOCK(port);
        awb_send_events(port, &(output->stats_update));
        return;
      }
       MCT_OBJECT_LOCK(port);
      memcpy(&private->awb_update_data, &output->stats_update,
        sizeof(stats_update_t));
      private->awb_update_flag = TRUE;
      MCT_OBJECT_UNLOCK(port);
    } else if(AWB_SEND_EVENT == output->type)
      awb_send_events(port, &(private->awb_update_data));
  return;
}

/** port_stats_check_identity
 *    @data1: port's existing identity;
 *    @data2: new identity to compare.
 *
 *  Return TRUE if two session index in the dentities are equalequal,
 *  Stats modules are linked ONLY under one session.
 **/
static boolean awb_port_check_identity(void *data1, void *data2)
{
  return (((*(unsigned int *)data1) ==
          (*(unsigned int *)data2)) ? TRUE : FALSE);
}

/** awb_port_check_session
 *
 **/
static boolean awb_port_check_session(void *data1, void *data2)
{
  return (((*(unsigned int *)data1) & 0xFFFF0000) ==
          ((*(unsigned int *)data2) & 0xFFFF0000) ?
          TRUE : FALSE);
}

/** awb_port_check_stream
 *
 **/
static boolean awb_port_check_stream(void *data1, void *data2)
{
  return ( ((*(unsigned int *)data1) & 0x0000FFFF )==
           ((*(unsigned int *)data2) & 0x0000FFFF) ?
          TRUE : FALSE);
}

/** awb_port_check_port_availability
 *
 *
 *
 **/
boolean awb_port_check_port_availability(mct_port_t *port, unsigned int *session)
{
  if (port->port_private == NULL)
    return TRUE;

  if (mct_list_find_custom(MCT_OBJECT_CHILDREN(port), session,
      awb_port_check_session) != NULL)
    return TRUE;

  return FALSE;
}

static boolean awb_port_event_sof( awb_port_private_t *port_private,
                               mct_event_t *event)
{
  int rc =  TRUE;
  q3a_thread_aecawb_msg_t * awb_msg = (q3a_thread_aecawb_msg_t *)
      malloc(sizeof(q3a_thread_aecawb_msg_t));
    if (awb_msg == NULL)
      return rc;
    memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
    awb_msg->type = MSG_AWB_SEND_EVENT;
   rc = q3a_aecawb_thread_en_q_msg(port_private->thread_data, awb_msg);
   return rc;
}

static boolean awb_port_event_stats_data( awb_port_private_t *port_private,
                               mct_event_t *event)
{
  boolean rc = TRUE;
  mct_event_module_t *mod_evt = &(event->u.module_event);
  mct_event_stats_isp_t *stats_event ;
  stats_t  * awb_stats;
  stats_event =(mct_event_stats_isp_t *)(mod_evt->module_event_data);
  awb_stats = &(port_private->awb_object.stats);

  awb_stats->stats_type_mask = 0; /*clear the mask*/
  boolean  send_flag = FALSE;

  if (stats_event) {
    if (!((stats_event->stats_mask & (1 << MSM_ISP_STATS_AWB) &&
      stats_event->stats_data[MSM_ISP_STATS_AWB].stats_buf) ||
      (stats_event->stats_mask & (1 << MSM_ISP_STATS_BG) &&
      stats_event->stats_data[MSM_ISP_STATS_BG].stats_buf))){
      CDBG_ERROR("invalid stats mask. Return back");
      return 1;
    }
    port_private->awb_stats_frame_count++;
    if (port_private->awb_stats_frame_count % port_private->awb_stats_proc_freq) {
      CDBG("skip awb stats. frame id: %d, stats_rec: %d frame_proc_freq: %d",
        stats_event->frame_id, port_private->awb_stats_frame_count,
        port_private->awb_stats_proc_freq);
      return 1;
    }
    CDBG("don't skip awb stats. frame id: %d, stats_rec: %d frame_proc_freq: %d",
      stats_event->frame_id, port_private->awb_stats_frame_count,
      port_private->awb_stats_proc_freq);

    q3a_thread_aecawb_msg_t * awb_msg = (q3a_thread_aecawb_msg_t *)
      malloc(sizeof(q3a_thread_aecawb_msg_t));
    if (awb_msg != NULL) {
      memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
      awb_msg->u.stats = awb_stats;
      awb_stats->frame_id = stats_event->frame_id;
      if ((stats_event->stats_mask & (1 << MSM_ISP_STATS_AWB)) &&
           stats_event->stats_data[MSM_ISP_STATS_AWB].stats_buf) {
        awb_msg->type = MSG_AWB_STATS;
        awb_stats->stats_type_mask |= STATS_AWB;
        CDBG("got awb stats");
        send_flag = TRUE;
        memcpy(&awb_stats->yuv_stats.q3a_awb_stats,
               stats_event->stats_data[MSM_ISP_STATS_AWB].stats_buf,
               sizeof(q3a_awb_stats_t));
      } else if ((stats_event->stats_mask & (1 << MSM_ISP_STATS_BG)) &&
                  stats_event->stats_data[MSM_ISP_STATS_BG].stats_buf) {

        CDBG("got bg stats");
        awb_stats->stats_type_mask |= STATS_BG;
        awb_msg->type = MSG_BG_AWB_STATS;
        send_flag = TRUE;
        memcpy(&awb_stats->bayer_stats.q3a_bg_stats,
               stats_event->stats_data[MSM_ISP_STATS_BG].stats_buf,
               sizeof(q3a_bg_stats_t));
      }
      if (send_flag) {
        CDBG("%s: msg type=%d, stats=%p, mask=0x%x mask addr=%p", __func__,awb_msg->type, awb_msg->u.stats,
              awb_msg->u.stats->stats_type_mask, &(awb_msg->u.stats->stats_type_mask));
        rc = q3a_aecawb_thread_en_q_msg(port_private->thread_data, awb_msg);
      } else {
        free(awb_msg);
      }
    }
  }
  return rc;
}
/** awb_port_proc_get_aec_data
 *
 *
 *
 **/
static boolean awb_port_proc_get_awb_data(mct_port_t *port,
  stats_get_data_t *stats_get_data)
{
    q3a_thread_aecawb_msg_t *awb_msg   = (q3a_thread_aecawb_msg_t *)
    malloc(sizeof(q3a_thread_aecawb_msg_t));
    awb_port_private_t *private = (awb_port_private_t *)(port->port_private);
    boolean rc = FALSE;
    if (awb_msg) {
      awb_msg->sync_flag = TRUE;
      memset(awb_msg, 0 , sizeof(q3a_thread_aecawb_msg_t));
      awb_msg->sync_flag = TRUE;
      awb_msg->type = MSG_AWB_GET;
      awb_msg->u.awb_get_parm.type = AWB_PARMS;
      rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
      stats_get_data->flag = STATS_UPDATE_AWB;
      stats_get_data->awb_get.g_gain = awb_msg->u.awb_get_parm.u.awb_gains.curr_gains.g_gain;
      stats_get_data->awb_get.r_gain = awb_msg->u.awb_get_parm.u.awb_gains.curr_gains.r_gain;
      stats_get_data->awb_get.b_gain = awb_msg->u.awb_get_parm.u.awb_gains.curr_gains.b_gain;
      free(awb_msg);
    } else {
      CDBG_ERROR("%s:%d Not enough memory", __func__, __LINE__);
    }
  return rc;
}

/** awb_process_downstream_mod_event
 *    @port:
 *    @event:
 **/
static boolean awb_process_downstream_mod_event( mct_port_t *port,
                                                  mct_event_t *event)
{
  boolean rc = TRUE;
  q3a_thread_aecawb_msg_t * awb_msg = NULL;
  mct_event_module_t *mod_evt = &(event->u.module_event);
  awb_port_private_t * private = (awb_port_private_t *)(port->port_private);

  CDBG("%s: Proceess module event of type: %d", __func__, mod_evt->type);

  switch (mod_evt->type) {
  case MCT_EVENT_MODULE_STATS_GET_THREAD_OBJECT: {
    q3a_thread_aecawb_data_t *data =
      (q3a_thread_aecawb_data_t *)(mod_evt->module_event_data);

    private->thread_data = data->thread_data;

    data->awb_port = port;
    data->awb_cb   = awb_port_callback;
    data->awb_obj  = &(private->awb_object);
    rc = TRUE;
  } /* case MCT_EVENT_MODULE_STATS_GET_THREAD_OBJECT */
    break;

  case MCT_EVENT_MODULE_STATS_DATA: {
    rc = awb_port_event_stats_data(private, event);
  } /* case MCT_EVENT_MODULE_STATS_DATA */
    break;

  case MCT_EVENT_MODULE_SOF_NOTIFY: {
    mct_bus_msg_isp_sof_t *sof_event;
      sof_event =(mct_bus_msg_isp_sof_t *)(mod_evt->module_event_data);
    private->cur_sof_id = sof_event->frame_id;
    rc = awb_port_event_sof(private, event);
  }
  break;

  case MCT_EVENT_MODULE_SET_RELOAD_CHROMATIX:
  case MCT_EVENT_MODULE_SET_CHROMATIX_PTR: {
    modulesChromatix_t *mod_chrom =
      (modulesChromatix_t *)mod_evt->module_event_data;
    awb_msg = (q3a_thread_aecawb_msg_t *)
      malloc(sizeof(q3a_thread_aecawb_msg_t));
    if (awb_msg != NULL ) {
      memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
      awb_msg->type = MSG_AWB_SET;
      awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_INIT_CHROMATIX_SENSOR;
      /*To Do: for now hard-code the stats type and op_mode for now.*/
      awb_msg->u.awb_set_parm.u.init_param.stats_type = AWB_STATS_BAYER;
      awb_msg->u.awb_set_parm.u.init_param.chromatix = mod_chrom->chromatixPtr;
      awb_msg->u.awb_set_parm.u.init_param.comm_chromatix = mod_chrom->chromatixComPtr;
      CDBG("%s:stream_type=%d op_mode=%d", __func__,
        private->stream_type, awb_msg->u.awb_set_parm.u.init_param.op_mode);

      awb_msg->u.awb_set_parm.u.init_param.awb_tuning_params.awb_subsampling_factor =
        ((private->algo_opt_mask & STATS_MASK_AWB) ?
        (AWB_SUBSAMPLE) : (MIN_AWB_SUBSAMPLE));

      if (CAM_STREAM_TYPE_VIDEO == private->stream_type) {
        awb_msg->u.awb_set_parm.u.init_param.awb_tuning_params.awb_stats_proc_freq =
          AWB_STATS_PROC_FREQ_VIDEO;
      } else if (CAM_STREAM_TYPE_PREVIEW == private->stream_type) {
        awb_msg->u.awb_set_parm.u.init_param.awb_tuning_params.awb_stats_proc_freq =
          AWB_STATS_PROC_FREQ_PREVIEW;
      }

      rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
      CDBG("%s: Enqueing AWB message returned: %d", __func__, rc);
    }
    else {
      CDBG_ERROR("%s: Failure allocating memory for AWB msg!", __func__);
    }
  }
    break;

  case MCT_EVENT_MODULE_STATS_AEC_UPDATE: {
    stats_update_t *stats_update =
      (stats_update_t *)mod_evt->module_event_data;

    awb_msg = (q3a_thread_aecawb_msg_t *)
      malloc(sizeof(q3a_thread_aecawb_msg_t));
    if (awb_msg != NULL ) {
      memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
      awb_msg->type = MSG_AWB_SET;
      awb_msg->is_priority = TRUE;
      awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_AEC_PARM;

      awb_msg->u.awb_set_parm.u.aec_parms.exp_index =
        stats_update->aec_update.exp_index_for_awb;
      awb_msg->u.awb_set_parm.u.aec_parms.indoor_index =
        stats_update->aec_update.indoor_index;
      awb_msg->u.awb_set_parm.u.aec_parms.outdoor_index =
        stats_update->aec_update.outdoor_index;
      awb_msg->u.awb_set_parm.u.aec_parms.lux_idx =
        stats_update->aec_update.lux_idx;
      awb_msg->u.awb_set_parm.u.aec_parms.aec_settled =
        stats_update->aec_update.settled;
      awb_msg->u.awb_set_parm.u.aec_parms.cur_luma =
        stats_update->aec_update.cur_luma;
      awb_msg->u.awb_set_parm.u.aec_parms.target_luma =
        stats_update->aec_update.target_luma;
      awb_msg->u.awb_set_parm.u.aec_parms.cur_line_cnt =
        stats_update->aec_update.linecount;
      awb_msg->u.awb_set_parm.u.aec_parms.cur_real_gain =
        stats_update->aec_update.real_gain;
      awb_msg->u.awb_set_parm.u.aec_parms.stored_digital_gain =
        stats_update->aec_update.stored_digital_gain;
      awb_msg->u.awb_set_parm.u.aec_parms.flash_sensitivity =
        *(q3q_flash_sensitivity_t *)stats_update->aec_update.flash_sensitivity;
      awb_msg->u.awb_set_parm.u.aec_parms.led_state =
        stats_update->aec_update.led_state;
      awb_msg->u.awb_set_parm.u.aec_parms.use_led_estimation  =
        stats_update->aec_update.use_led_estimation;
      rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
    }
  }
  break;
  case  MCT_EVENT_MODULE_STATS_GET_DATA: {
  }
  break;

  case MCT_EVENT_MODULE_SET_STREAM_CONFIG: {
    CDBG("ddd MCT_EVENT_MODULE_SET_STREAM_CONFIG");
    q3a_thread_aecawb_msg_t *awb_msg = (q3a_thread_aecawb_msg_t *)
      malloc(sizeof(q3a_thread_aecawb_msg_t));
    if (awb_msg == NULL )
      break;
    memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
    awb_msg->type = MSG_AWB_SET;
    awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_OP_MODE;
    switch (private->stream_type) {
      case CAM_STREAM_TYPE_VIDEO: {
         awb_msg->u.awb_set_parm.u.init_param.op_mode =
           Q3A_OPERATION_MODE_CAMCORDER;
      }
        break;

      case CAM_STREAM_TYPE_PREVIEW: {
        awb_msg->u.awb_set_parm.u.init_param.op_mode =
           Q3A_OPERATION_MODE_PREVIEW;
      }
        break;
      case CAM_STREAM_TYPE_RAW:
      case CAM_STREAM_TYPE_SNAPSHOT: {
        awb_msg->u.awb_set_parm.u.init_param.op_mode =
           Q3A_OPERATION_MODE_SNAPSHOT;
      }
        break;


      default:
        awb_msg->u.awb_set_parm.u.init_param.op_mode =
           Q3A_OPERATION_MODE_PREVIEW;
        break;
      } /* switch (private->stream_type) */

     rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
     break;
   }/* MCT_EVENT_MODULE_SET_STREAM_CONFIG*/

   case MCT_EVENT_MODULE_MODE_CHANGE: {
        //Stream mode has changed
        private->stream_type =
          ((stats_mode_change_event_data*)
          (event->u.module_event.module_event_data))->stream_type;
        private->reserved_id =
          ((stats_mode_change_event_data*)
          (event->u.module_event.module_event_data))->reserved_id;
        break;
    }
    case MCT_EVENT_MODULE_PPROC_GET_AWB_UPDATE: {
        stats_get_data_t *stats_get_data =
          (stats_get_data_t *)mod_evt->module_event_data;
        if (!stats_get_data) {
          CDBG_ERROR("%s:%d failed\n", __func__, __LINE__);
          break;
          }
        awb_port_proc_get_awb_data(port, stats_get_data);
        break;
       } /* MCT_EVENT_MODULE_PPROC_GET_AEC_UPDATE X*/
    default:
    break;
  } /* switch (mod_evt->type) */
  return rc;
}

static boolean awb_port_proc_downstream_ctrl(mct_port_t *port, mct_event_t *event)
{
  boolean rc = TRUE;

  awb_port_private_t *private = (awb_port_private_t *)(port->port_private);
  mct_event_control_t *mod_ctrl = (mct_event_control_t *)&(event->u.ctrl_event);
  switch (mod_ctrl->type) {
  case MCT_EVENT_CONTROL_SET_PARM: {
    /*TO DO: some logic shall be handled by stats and q3a port
      to acheive that, we need to add the function to find the desired sub port;
      however since it is not in place, for now, handle it here
     */
    stats_set_params_type *stat_parm = (stats_set_params_type *)mod_ctrl->control_event_data;
    if (stat_parm->param_type == STATS_SET_Q3A_PARAM) {
      q3a_set_params_type  *q3a_param = &(stat_parm->u.q3a_param);

      q3a_thread_aecawb_msg_t *awb_msg = (q3a_thread_aecawb_msg_t *)
        malloc(sizeof(q3a_thread_aecawb_msg_t));
      if (awb_msg != NULL ) {
        memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
        if (q3a_param->type == Q3A_SET_AWB_PARAM) {
          awb_msg->type = MSG_AWB_SET;
          awb_msg->u.awb_set_parm = q3a_param->u.awb_param;
          rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
        } else if (q3a_param->type == Q3A_ALL_SET_PARAM) {
          switch (q3a_param->u.q3a_all_param.type) {
          case Q3A_ALL_SET_EZTUNE_RUNNIG: {
            awb_msg->type = MSG_AWB_SET;
            awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_EZ_TUNE_RUNNING;
            awb_msg->u.awb_set_parm.u.ez_running =
              q3a_param->u.q3a_all_param.u.ez_runnig;
            rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
          }
            break;
          default: {
          }
            break;
          }
        } else {
          free(awb_msg);
          awb_msg = NULL;
        }
      }
    } else if (stat_parm->param_type == STATS_SET_COMMON_PARAM) {
      stats_common_set_parameter_t *common_param =
        &(stat_parm->u.common_param);
      switch (common_param->type) {
      case COMMON_SET_PARAM_BESTSHOT: {
        q3a_thread_aecawb_msg_t *awb_msg = (q3a_thread_aecawb_msg_t *)
          malloc(sizeof(q3a_thread_aecawb_msg_t));
        if (awb_msg != NULL ) {
          memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
          awb_msg->type = MSG_AWB_SET;
          awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_BESTSHOT;
          awb_msg->u.awb_set_parm.u.awb_best_shot =
            common_param->u.bestshot_mode;
          rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
        }
      }
      break;
      case COMMON_SET_PARAM_VIDEO_HDR: {
        q3a_thread_aecawb_msg_t *awb_msg = (q3a_thread_aecawb_msg_t *)
          malloc(sizeof(q3a_thread_aecawb_msg_t));
        if (awb_msg != NULL ) {
          memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
          awb_msg->type = MSG_AWB_SET;
          awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_VIDEO_HDR;
          awb_msg->u.awb_set_parm.u.video_hdr =
            common_param->u.video_hdr;
          rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
        }
      }
      break;
      case COMMON_SET_PARAM_STATS_DEBUG_MASK: {
        q3a_thread_aecawb_msg_t *awb_msg = (q3a_thread_aecawb_msg_t *)
          malloc(sizeof(q3a_thread_aecawb_msg_t));
        if (awb_msg != NULL ) {
          memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
          awb_msg->type = MSG_AWB_SET;
          awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_STATS_DEBUG_MASK;
          awb_msg->u.awb_set_parm.u.stats_debug_mask =
            common_param->u.stats_debug_mask;
          rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
        }
      }
        break;
      case COMMON_SET_PARAM_ALGO_OPTIMIZATIONS_MASK: {
        /* Save the algo opt mask in port private. It will be used to send the
           mask value along with init chromatix*/
        private->algo_opt_mask = common_param->u.algo_opt_mask;
      }
        break;
      default:
        break;
      }
    }
  }
    break;
  case MCT_EVENT_CONTROL_STREAMON: {
    mct_event_t event;
    stats_update_t stats_update;
    q3a_thread_aecawb_msg_t *awb_msg   = (q3a_thread_aecawb_msg_t *)
    malloc(sizeof(q3a_thread_aecawb_msg_t));
    if (awb_msg != NULL ) {
      memset(awb_msg, 0 , sizeof(q3a_thread_aecawb_msg_t));
      memset(&stats_update, 0, sizeof(stats_update_t));
      awb_msg->sync_flag = TRUE;
      awb_msg->type = MSG_AWB_GET;
      awb_msg->u.awb_get_parm.type = AWB_GAINS;
      rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
      if (awb_msg) {
        event.direction = MCT_EVENT_UPSTREAM;
        event.identity = private->reserved_id;
        event.type = MCT_EVENT_MODULE_EVENT;
        event.u.module_event.type = MCT_EVENT_MODULE_STATS_AWB_UPDATE;
        event.u.module_event.module_event_data = (void *)(&stats_update);;

        stats_update.flag = STATS_UPDATE_AWB;
        stats_update.awb_update.gain.r_gain =
          awb_msg->u.awb_get_parm.u.awb_gains.curr_gains.r_gain;
        stats_update.awb_update.gain.g_gain =
          awb_msg->u.awb_get_parm.u.awb_gains.curr_gains.g_gain;
        stats_update.awb_update.gain.b_gain =
         awb_msg->u.awb_get_parm.u.awb_gains.curr_gains.b_gain;
        stats_update.awb_update.color_temp =
          awb_msg->u.awb_get_parm.u.awb_gains.color_temp;

        CDBG("%s: send AWB_UPDATE to port =%p, event =%p",
              __func__,port, &event);
        MCT_PORT_EVENT_FUNC(port)(port, &event);
        free(awb_msg);
        awb_msg = NULL;
      }
    } /* if (awb_msg != NULL ) */
  }
    break;
    case MCT_EVENT_CONTROL_START_ZSL_SNAPSHOT: {
    q3a_thread_aecawb_msg_t *awb_msg = (q3a_thread_aecawb_msg_t *)
      malloc(sizeof(q3a_thread_aecawb_msg_t));
    if (awb_msg == NULL )
      break;
    memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
    awb_msg->type = MSG_AWB_SET;
    awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_OP_MODE;
    awb_msg->u.awb_set_parm.u.init_param.op_mode =
      Q3A_OPERATION_MODE_SNAPSHOT;
    rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
    } /* MCT_EVENT_CONTROL_START_ZSL_SNAPSHOT */
    break;
    case MCT_EVENT_CONTROL_STOP_ZSL_SNAPSHOT:{
      q3a_thread_aecawb_msg_t *awb_msg = (q3a_thread_aecawb_msg_t *)
        malloc(sizeof(q3a_thread_aecawb_msg_t));
      if (awb_msg == NULL )
        break;
      memset(awb_msg, 0, sizeof(q3a_thread_aecawb_msg_t));
      awb_msg->type = MSG_AWB_SET;
      awb_msg->u.awb_set_parm.type = AWB_SET_PARAM_OP_MODE;
      awb_msg->u.awb_set_parm.u.init_param.op_mode =
        Q3A_OPERATION_MODE_PREVIEW;
      rc = q3a_aecawb_thread_en_q_msg(private->thread_data, awb_msg);
      } /* MCT_EVENT_CONTROL_STOP_ZSL_SNAPSHOT */
      break;

  default:
    break;
  }
  return rc;
}


/** awb_port_process_upstream_mod_event
 *    @port:
 *    @event:
 **/
static boolean awb_port_process_upstream_mod_event(mct_port_t *port,
                                                    mct_event_t *event)
{
  boolean rc = FALSE;
  q3a_thread_aecawb_msg_t * awb_msg = NULL;
  mct_event_module_t *mod_evt = &(event->u.module_event);
  awb_port_private_t * private = (awb_port_private_t *)(port->port_private);
  mct_port_t *peer;
  switch (mod_evt->type) {
  case MCT_EVENT_MODULE_STATS_AWB_UPDATE:
  case MCT_EVENT_MODULE_STATS_POST_TO_BUS:
    peer = MCT_PORT_PEER(port);
    rc = MCT_PORT_EVENT_FUNC(peer)(peer, event);
    break;

  default: /*shall not get here*/
    break;
  }
  return rc;
}

/** awb_port_event
 *    @port:
 *    @event:
 *
 * awb sink module's event processing function. Received events could be:
 * AEC/AWB/AF Bayer stats;
 * Gyro sensor stat;
 * Information request event from other module(s);
 * Informatin update event from other module(s);
 * It ONLY takes MCT_EVENT_DOWNSTREAM event.
 *
 * Return TRUE if the event is processed successfully.
 **/
static boolean awb_port_event(mct_port_t *port, mct_event_t *event)
{
  boolean rc = FALSE;
  awb_port_private_t *private;

  CDBG("%s: port =%p, evt_type: %d direction: %d", __func__, port, event->type,
    MCT_EVENT_DIRECTION(event));
  /* sanity check */
  if (!port || !event){
    CDBG_ERROR("%s: port or event NULL", __func__);
    return FALSE;
  }

  private = (awb_port_private_t *)(port->port_private);
  if (!private){
    ALOGE("%s: AWB private pointer NULL", __func__);
    return FALSE;
  }

  /* sanity check: ensure event is meant for port with same identity*/
  if ((private->reserved_id & 0xFFFF0000) != (event->identity & 0xFFFF0000)){
    ALOGE("%s: AWB identity didn't match!", __func__);
    return FALSE;
  }

  switch (MCT_EVENT_DIRECTION(event)) {
  case MCT_EVENT_DOWNSTREAM: {
    switch (event->type) {
    case MCT_EVENT_MODULE_EVENT: {
      rc = awb_process_downstream_mod_event( port, event);
    } /* case MCT_EVENT_MODULE_EVENT */
      break;

    case MCT_EVENT_CONTROL_CMD: {
      rc = awb_port_proc_downstream_ctrl(port,event);
    }
      break;

    default:
      break;
    }
  } /* case MCT_EVENT_DOWNSTREAM */
    break;


  case MCT_EVENT_UPSTREAM: {

    switch (event->type) {
    case MCT_EVENT_MODULE_EVENT: {
      rc = awb_port_process_upstream_mod_event(port, event);
    } /*case MCT_EVENT_MODULE_EVENT*/
    break;

    default:
      break;
    }
  } /* MCT_EVENT_UPSTREAM */
  break ;

  default:
    rc = FALSE;
    break;
  }

  CDBG("%s: X rc:%d", __func__, rc);
  return rc;
}

/** awb_port_ext_link
 *    @identity: session id + stream id
 *    @port:  awb module's sink port
 *    @peer:  q3a module's sink port
 **/
static boolean awb_port_ext_link(unsigned int identity,
  mct_port_t *port, mct_port_t *peer)
{
  boolean rc = FALSE;
  awb_port_private_t  *private;
  CDBG("%s:%d", __func__, __LINE__);

  /* awb sink port's external peer is always q3a module's sink port */
  if (!port || !peer ||
      strcmp(MCT_OBJECT_NAME(port), "awb_sink") ||
      strcmp(MCT_OBJECT_NAME(peer), "q3a_sink")) {
    CDBG_ERROR("%s: Invalid Port/Peer!", __func__);
    return FALSE;
  }

  private = (awb_port_private_t *)port->port_private;
  if (!private) {
    CDBG_ERROR("%s: Private port NULL!", __func__);
    return FALSE;
  }

  MCT_OBJECT_LOCK(port);
  switch (private->state) {
  case AWB_PORT_STATE_RESERVED:
  case AWB_PORT_STATE_UNLINKED:
  case AWB_PORT_STATE_LINKED:
    if ( (private->reserved_id & 0xFFFF0000) != (identity & 0xFFFF0000)) {
      break;
    }
  case AWB_PORT_STATE_CREATED:
    rc = TRUE;
    break;

  default:
    break;
  }

  if (rc == TRUE) {
    private->state = AWB_PORT_STATE_LINKED;
    MCT_PORT_PEER(port) = peer;
    MCT_OBJECT_REFCOUNT(port) += 1;
  }
  MCT_OBJECT_UNLOCK(port);

  return rc;
}

/** awb_port_ext_unlink
 *
 **/
static void awb_port_ext_unlink(unsigned int identity,
  mct_port_t *port, mct_port_t *peer)
{
  awb_port_private_t *private;

  if (!port || !peer || MCT_PORT_PEER(port) != peer)
    return;

  private = (awb_port_private_t *)port->port_private;
  if (!private)
    return;

  MCT_OBJECT_LOCK(port);
  if ((private->state == AWB_PORT_STATE_LINKED) &&
      (private->reserved_id & 0xFFFF0000) == (identity & 0xFFFF0000)) {

    MCT_OBJECT_REFCOUNT(port) -= 1;
    if (!MCT_OBJECT_REFCOUNT(port))
      private->state = AWB_PORT_STATE_UNLINKED;
  }
  MCT_OBJECT_UNLOCK(port);

  return;
}

/** awb_port_set_caps
 *
 **/
static boolean awb_port_set_caps(mct_port_t *port,
  mct_port_caps_t *caps)
{
  if (strcmp(MCT_PORT_NAME(port), "awb_sink"))
    return FALSE;

  port->caps = *caps;
  return TRUE;
}

/** awb_port_check_caps_reserve
 *
 *
 *  AWB sink port can ONLY be re-used by ONE session. If this port
 *  has been in use, AWB module has to add an extra port to support
 *  any new session(via module_awb_request_new_port).
 **/
static boolean awb_port_check_caps_reserve(mct_port_t *port, void *caps,
  void *stream)
{
  boolean            rc = FALSE;
  mct_port_caps_t    *port_caps;
  awb_port_private_t *private;
  mct_stream_info_t *stream_info = (mct_stream_info_t *)stream;

  CDBG("%s:\n", __func__);
  MCT_OBJECT_LOCK(port);
  if (!port || !caps || !stream_info ||
      strcmp(MCT_OBJECT_NAME(port), "awb_sink")) {
    rc = FALSE;
    goto reserve_done;
  }

  port_caps = (mct_port_caps_t *)caps;
  if (port_caps->port_caps_type != MCT_PORT_CAPS_STATS) {
    rc = FALSE;
    goto reserve_done;
  }

  private = (awb_port_private_t *)port->port_private;
  switch (private->state) {
  case AWB_PORT_STATE_LINKED:
    if ((private->reserved_id & 0xFFFF0000) ==
        (stream_info->identity & 0xFFFF0000))
      rc = TRUE;
    break;

  case AWB_PORT_STATE_CREATED:
  case AWB_PORT_STATE_UNRESERVED: {

    private->reserved_id = stream_info->identity;
    private->stream_type = stream_info->stream_type;
    private->state       = AWB_PORT_STATE_RESERVED;
    rc = TRUE;
  }
    break;

  case AWB_PORT_STATE_RESERVED:
    if ((private->reserved_id & 0xFFFF0000) ==
        (stream_info->identity & 0xFFFF0000))
      rc = TRUE;
    break;

  default:
    rc = FALSE;
    break;
  }

reserve_done:
  MCT_OBJECT_UNLOCK(port);
  return rc;
}

/** awb_port_check_caps_unreserve:
 *
 *
 *
 **/
static boolean awb_port_check_caps_unreserve(mct_port_t *port,
  unsigned int identity)
{
  awb_port_private_t *private;

  if (!port || strcmp(MCT_OBJECT_NAME(port), "awb_sink"))
    return FALSE;

  private = (awb_port_private_t *)port->port_private;
  if (!private)
    return FALSE;

  MCT_OBJECT_LOCK(port);
  if ((private->state == AWB_PORT_STATE_UNLINKED   ||
       private->state == AWB_PORT_STATE_LINKED ||
       private->state == AWB_PORT_STATE_RESERVED) &&
      ((private->reserved_id & 0xFFFF0000) == (identity & 0xFFFF0000))) {

    if (!MCT_OBJECT_REFCOUNT(port)) {
      private->state       = AWB_PORT_STATE_UNRESERVED;
      private->reserved_id = (private->reserved_id & 0xFFFF0000);
    }
  }
  MCT_OBJECT_UNLOCK(port);

  return TRUE;
}

/** awb_port_find_identity
 *
 **/
boolean awb_port_find_identity(mct_port_t *port, unsigned int identity)
{
  awb_port_private_t *private;

  if ( !port || strcmp(MCT_OBJECT_NAME(port), "awb_sink"))
    return FALSE;

  private = port->port_private;

  if (private) {
    return ((private->reserved_id & 0xFFFF0000) == (identity & 0xFFFF0000) ?
            TRUE : FALSE);
  }

  return FALSE;
}

/** awb_port_deinit
 *    @port:
 **/
void awb_port_deinit(mct_port_t *port)
{
  awb_port_private_t *private;

  if (!port || strcmp(MCT_OBJECT_NAME(port), "awb_sink"))
      return;

  private = port->port_private;
  if (private) {
  AWB_DESTROY_LOCK((&private->awb_object));
  awb_destroy(private->awb_object.awb);
    free(private);
    private = NULL;
  }
}

/** awb_port_init:
 *    @port: awb's sink port to be initialized
 *
 *  awb port initialization entry point. Becase AWB module/port is
 *  pure software object, defer awb_port_init when session starts.
 **/
boolean awb_port_init(mct_port_t *port, unsigned int *session_id)
{
  boolean            rc = TRUE;
  mct_port_caps_t    caps;
  unsigned int       *session;
  mct_list_t         *list;
  awb_port_private_t *private;

  if (!port || strcmp(MCT_OBJECT_NAME(port), "awb_sink"))
    return FALSE;

  private = (void *)malloc(sizeof(awb_port_private_t));
  if (!private)
    return FALSE;
  memset(private, 0 , sizeof(awb_port_private_t));

  /* initialize AWB object */
  AWB_INITIALIZE_LOCK(&private->awb_object);
  private->awb_object.awb = awb_init(&(private->awb_object.awb_ops));

  if (private->awb_object.awb == NULL) {
    free(private);
    return FALSE;
  }

  private->reserved_id = *session_id;
  private->state       = AWB_PORT_STATE_CREATED;
  private->awb_stats_proc_freq = 1;

  port->port_private   = private;
  port->direction      = MCT_PORT_SINK;
  caps.port_caps_type  = MCT_PORT_CAPS_STATS;
  caps.u.stats.flag    = (MCT_PORT_CAP_STATS_Q3A | MCT_PORT_CAP_STATS_CS_RS);

  /* this is sink port of awb module */
  mct_port_set_event_func(port, awb_port_event);
  mct_port_set_ext_link_func(port, awb_port_ext_link);
  mct_port_set_unlink_func(port, awb_port_ext_unlink);
  mct_port_set_set_caps_func(port, awb_port_set_caps);
  mct_port_set_check_caps_reserve_func(port, awb_port_check_caps_reserve);
  mct_port_set_check_caps_unreserve_func(port, awb_port_check_caps_unreserve);

  if (port->set_caps)
    port->set_caps(port, &caps);

  return TRUE;
}
