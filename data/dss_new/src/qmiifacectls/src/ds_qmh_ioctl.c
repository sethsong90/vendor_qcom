/*===========================================================================


                       Q M I   M O D E   H A N D L E R

           M O D E  - S P E C I F I C   I O C T L   H A N D L E R S
                       
GENERAL DESCRIPTION
  This file contains function implementations for the QMI Proxy IFACE
  mode-specific IOCTL handlers.

EXTERNALIZED FUNCTIONS

INITIALIZATION AND SEQUENCING REQUIREMENTS
  dsqmh_ioctl_init() should be called at startup. 

Copyright (c) 2008 -  2012 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

  $Header: //source/qcom/qct/modem/datahlos/interface/qmiifacectls/rel/11.01.01/src/ds_qmh_ioctl.c#6 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
06/22/11    bd     Fix for DoS  bundled packet when starting the call without 
                   Traffic Channel setup. Migrating CL 1674460
04/13/11    hm     Multi-modem support merged from linux QMH
10/04/10    sy     Add requested operation tlv in set inactivity timer ioctl
10/04/10    sy     Add support to return cached bearer tech.
06/24/09    am     Renamed hton/ntoh macros with ps_ prefix.
06/01/09    ar     Remove handling for common IOCTLs.
01/16/09    ar     Fix Lint and MOB integration issues.
01/09/09    ar     Added IPV6 address support.
11/24/08    ar     Adjust for extended tech pref convention change
08/11/08    ar     Created module/initial version.

===========================================================================*/


/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"

#if defined (FEATURE_DATA_PS) && defined (FEATURE_QMI_CLIENT)

#include "dssocket_defs.h"
#include "ps_flow.h"
#include "ds_qmhi.h"
#include "ds_qmh_llif.h"
#include "dss_iface_ioctl.h"
#ifdef FEATURE_DATA_PS_IPV6
#include "ps_ifacei_addr_v6.h"
#endif
#include "AEEstd.h"

/*===========================================================================

            LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE

This section contains local definitions for constants, macros, types,
variables and other items needed by this module.

===========================================================================*/


#ifndef FEATURE_DSS_LINUX
/*===========================================================================

                    INTERNAL FUNCTION DEFINITIONS

===========================================================================*/

LOCAL void dsqmhioctl_populate_umts_session_settings
( 
  dsqmh_runtime_info_type *qmi_curr_call_info, 
  ps_iface_ioctl_3gpp_session_params_info_type *umts_session_info
)
{
  int umts_params_str_len;

  if (qmi_curr_call_info == NULL || umts_session_info == NULL)
  {
    DSQMH_MSG_ERROR( "populate_umts_session_settings: Bad Input Params ",0,0,0);
    return;
  }
  
  memset(umts_session_info, 
	     0, 
		 sizeof(ps_iface_ioctl_3gpp_session_params_info_type));

  /*Profile ID*/
  umts_session_info->profile_number = 
    qmi_curr_call_info ->profile_id.profile_index;

  /*Profile Name*/
  umts_params_str_len =  
    std_strlen(qmi_curr_call_info->profile_params.umts_profile_params.profile_name);

  std_strlcpy(umts_session_info->profile_name, 
              qmi_curr_call_info->profile_params.umts_profile_params.profile_name,
              umts_params_str_len + 1);
  
  /*auth*/
  umts_session_info->auth.auth_type = 
    qmi_curr_call_info->profile_params.umts_profile_params.auth_pref;

  umts_params_str_len =  
    std_strlen(qmi_curr_call_info->profile_params.umts_profile_params.username);

  std_strlcpy(umts_session_info->auth.username, 
              qmi_curr_call_info->profile_params.umts_profile_params.username,
              umts_params_str_len + 1);

  /*apn*/
  umts_params_str_len =  
    std_strlen(qmi_curr_call_info->profile_params.umts_profile_params.apn_name);

  std_strlcpy(umts_session_info->apn, 
              qmi_curr_call_info->profile_params.umts_profile_params.apn_name,
              umts_params_str_len + 1);

  /*PDP Type*/
  umts_session_info->pdp_type = 
    qmi_curr_call_info->profile_params.umts_profile_params.pdp_type;

  /*GPRS QOS*/
  umts_session_info->gprs_qos.precedence = 
    (uint32) qmi_curr_call_info->profile_params.umts_profile_params.
    gprs_requested_qos.precedence_class;
  umts_session_info->gprs_qos.delay = 
    (uint32) qmi_curr_call_info->profile_params.umts_profile_params.
    gprs_requested_qos.delay_class;
  umts_session_info->gprs_qos.reliability = 
    (uint32) qmi_curr_call_info->profile_params.umts_profile_params.
    gprs_requested_qos.reliability_class;
  umts_session_info->gprs_qos.peak  = 
    (uint32) qmi_curr_call_info->profile_params.umts_profile_params.
    gprs_requested_qos.peak_throughput_class;
  umts_session_info->gprs_qos.mean  = 
    (uint32) qmi_curr_call_info->profile_params.umts_profile_params.
    gprs_requested_qos.mean_throughput_class;

  /*UMTS QOS*/
  umts_session_info->umts_qos.traffic_class =
    qmi_curr_call_info->profile_params.umts_profile_params.
    umts_requested_qos.traffic_class;
  umts_session_info->umts_qos.max_ul_bitrate =
    qmi_curr_call_info->profile_params.umts_profile_params.
    umts_requested_qos.max_ul_bitrate;
  umts_session_info->umts_qos.max_dl_bitrate =
    qmi_curr_call_info->profile_params.umts_profile_params.
    umts_requested_qos.max_dl_bitrate;
  umts_session_info->umts_qos.gtd_ul_bitrate =
    qmi_curr_call_info->profile_params.umts_profile_params.
    umts_requested_qos.guaranteed_ul_bitrate;
  umts_session_info->umts_qos.gtd_dl_bitrate =
    qmi_curr_call_info->profile_params.umts_profile_params.
    umts_requested_qos.guaranteed_dl_bitrate;
  umts_session_info->umts_qos.dlvry_order =
    qmi_curr_call_info->profile_params.umts_profile_params.
    umts_requested_qos.qos_delivery_order;
  umts_session_info->umts_qos.max_sdu_size =
    qmi_curr_call_info->profile_params.umts_profile_params.
    umts_requested_qos.max_sdu_size;
  umts_session_info->umts_qos.sdu_err =
    qmi_curr_call_info->profile_params.umts_profile_params.
    umts_requested_qos.sdu_error_ratio;
  umts_session_info->umts_qos.res_biterr =
    qmi_curr_call_info->profile_params.umts_profile_params.
    umts_requested_qos.residual_ber_ratio;
  umts_session_info->umts_qos.dlvr_err_sdu =
    qmi_curr_call_info->profile_params.umts_profile_params.
    umts_requested_qos.deliver_error_sdu;
  umts_session_info->umts_qos.trans_delay =
    qmi_curr_call_info->profile_params.umts_profile_params.
    umts_requested_qos.transfer_delay;
  umts_session_info->umts_qos.thandle_prio = 
    qmi_curr_call_info->profile_params.umts_profile_params.
    umts_requested_qos.traffic_handling_prio;
  umts_session_info->umts_qos.sig_ind =
    qmi_curr_call_info->profile_params.umts_profile_params.
    umts_requested_qos.signal_indication;

  /* TODO: LTE QOS is still not avaible in qmi and qmimsglib.*/

  /*pcscf flag*/
  umts_session_info->request_pcscf_address_flag = 
    qmi_curr_call_info->call_info.p_cscf_addr_using_pco;
}

#endif
/*===========================================================================
FUNCTION  DSQMHIOCTL_IFACE_CURRENT_SETTINGS_HDLR

DESCRIPTION
  This function manages the query for current settings commands.

PARAMETERS
  iface_ptr         - Pointer to ps_iface
  ioctl_name        - The operation name
  argval_ptr        - Pointer to operation specific structure
  ps_errno          - Error code returned in case of failure (Error
                      values are those defined in dserrno.h)

DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  Message buffer memory is returned to task heap.
===========================================================================*/
LOCAL int dsqmhioctl_iface_current_settings_hdlr
(
  ps_iface_type           *iface_ptr,
  ps_iface_ioctl_type      ioctl_name,
  void                    *argval_ptr,
  sint15                  *ps_errno
)
{
  int32                         result = DSQMH_SUCCESS;
  dsqmh_runtime_info_type       current_info;
  ps_iface_data_bearer_rate    *rate_ptr = NULL;
  dsqmh_iface_cblk_type        *cblk_ptr = NULL;
  
  qmi_wds_internal_runtime_setings_params_type       req_param;
  qmi_wds_internal_runtime_settings_rsp_type         rsp_data;
  qmi_wds_set_internal_runtime_settings_params_type  set_req_param;
  qmi_wds_set_internal_runtime_settings_rsp_type     set_rsp_data;
  
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#define DSQMH_IOCTL_GET_CURRENT_SETTING(name,param)                         \
  DSQMH_MSG_MED( "DSQMH IOCTL IFACE get "name": iface=%d",                  \
                 (uint32)iface_ptr->client_data_ptr, 0, 0 );                \
  if( DSQMH_FAILED ==                                                       \
      dsqmhllif_query_net_settings( (uint32)iface_ptr->client_data_ptr,     \
                                    param,                                  \
                                    &current_info ) )                       \
  {                                                                         \
    DSQMH_MSG_ERROR( "QMH IOCTL failed: "name,0,0,0);                       \
    result = DSQMH_FAILED;                                                  \
  }
#define DSQMH_IOCTL_GET_INTERNAL_SETTING(name)                              \
  DSQMH_MSG_MED( "DSQMH IOCTL IFACE get "name": iface=%d",                  \
                 (uint32)iface_ptr->client_data_ptr, 0, 0 );                \
  if( DSQMH_FAILED ==                                                       \
      dsqmhllif_query_internal_runtime_settings(                            \
                                    (uint32)iface_ptr->client_data_ptr,     \
                                    &req_param,                             \
                                    &rsp_data ) )                           \
  {                                                                         \
    DSQMH_MSG_ERROR( "QMH IOCTL get failed: "name,0,0,0);                   \
    result = DSQMH_FAILED;                                                  \
  }
#define DSQMH_IOCTL_SET_INTERNAL_SETTING(name)                              \
  DSQMH_MSG_MED( "DSQMH IOCTL IFACE set "name": iface=%d",                  \
                 (uint32)iface_ptr->client_data_ptr, 0, 0 );                \
  if( DSQMH_FAILED ==                                                       \
      dsqmhllif_set_internal_runtime_settings(                              \
                                    (uint32)iface_ptr->client_data_ptr,     \
                                    &set_req_param,                         \
                                    &set_rsp_data ) )                       \
  {                                                                         \
    DSQMH_MSG_ERROR( "QMH IOCTL set failed: "name,0,0,0);                   \
    result = DSQMH_FAILED;                                                  \
  }


  switch( ioctl_name )
  {
    case PS_IFACE_IOCTL_GET_BEARER_TECHNOLOGY:
      cblk_ptr = DSQMH_GET_CBLK_PTR( (uint32) iface_ptr->client_data_ptr );
      DSQMH_MSG_MED( "DSQMH IOCTL IFACE GET BEARER TECH: iface=%d",
                     (uint32) cblk_ptr, 0, 0 );

      memcpy( argval_ptr, 
             (void*)&cblk_ptr->um_bearer_tech, 
              sizeof(cblk_ptr->um_bearer_tech));  

      break;

    case PS_IFACE_IOCTL_GET_DATA_BEARER_RATE:
      DSQMH_MSG_MED( "DSQMH IOCTL IFACE GET DATA BEARER RATE: iface=%d",
                     (uint32)iface_ptr->client_data_ptr, 0, 0 );
      if( DSQMH_FAILED ==
          dsqmhllif_query_channel_settings( (uint32)iface_ptr->client_data_ptr,
                                            &current_info ) )
      {
        DSQMH_MSG_ERROR( "QMH IOCTL failed: dsqmhllif_query_channel_settings",0,0,0);
        result = DSQMH_FAILED;
      }
      else
      {
        rate_ptr = (ps_iface_data_bearer_rate*)argval_ptr;
        rate_ptr->current_tx_bearer_data_rate =
          current_info.channel_info.current_channel_tx_rate;
        rate_ptr->current_rx_bearer_data_rate =
          current_info.channel_info.current_channel_rx_rate;
        rate_ptr->max_tx_bearer_data_rate =
          current_info.channel_info.max_channel_tx_rate;
        rate_ptr->max_rx_bearer_data_rate =
          current_info.channel_info.max_channel_rx_rate;
        rate_ptr->avg_tx_bearer_data_rate = 0;
        rate_ptr->avg_rx_bearer_data_rate = 0;
      }
      break;
      
    case PS_IFACE_IOCTL_GET_RF_CONDITIONS:
      req_param.params_mask = QMI_WDS_REQ_INTERNAL_SETTINGS_PARAM;
      req_param.req_internal_settings =
        QMI_WDS_RUNTIME_SETTINGS_RF_CONDITIONS_REQ_PARAM;
      DSQMH_IOCTL_GET_INTERNAL_SETTING( "RF_CONDITIONS" );
      *(uint32*)argval_ptr = (uint32)rsp_data.rf_conditions.rf_conditions;
      break;

    case PS_IFACE_IOCTL_REFRESH_DHCP_CONFIG_INFO:
      DSQMH_MSG_MED( "DSQMH IOCTL IFACE REFRESH DHCP CONFIG INFO"
                     "iface=%d",
                     (uint32)iface_ptr->client_data_ptr, 0, 0 );

      if( DSQMH_FAILED == 
          dsqmhllif_refresh_dhcp_config_info( (uint32)iface_ptr->client_data_ptr,
                                              ps_errno) )
      {
        DSQMH_MSG_ERROR ( "QMH IOCTL failed: REFRESH_DHCP_CONFIG_INFO", 
                          0, 0, 0 );
        return DSQMH_FAILED;
      }
      break;

    case PS_IFACE_IOCTL_ON_QOS_AWARE_SYSTEM:
      if( DSQMH_FAILED ==                                                  
          dsqmhllif_query_qos_settings( DSQMHLLIF_QOS_INFO_NETWORK,
                                        (uint32)iface_ptr->client_data_ptr,
                                        argval_ptr ) )
      {
        DSQMH_MSG_ERROR( "QMH IOCTL failed: ON_QOS_AWARE_SYSTEM",0,0,0);
        result = DSQMH_FAILED;                                             
      }
      break;
      
    case PS_IFACE_IOCTL_707_GET_DORM_TIMER:
      req_param.params_mask = QMI_WDS_REQ_INTERNAL_SETTINGS_PARAM;
      req_param.req_internal_settings =
        QMI_WDS_RUNTIME_SETTINGS_DORM_TIMER_1X_REQ_PARAM;
      DSQMH_IOCTL_GET_INTERNAL_SETTING( "1X DORM TIMER" );
      *(uint32*)argval_ptr = rsp_data.dorm_timer_for_1x;
      break;
      
    case PS_IFACE_IOCTL_707_SET_DORM_TIMER:
      set_req_param.params_mask = QMI_WDS_SET_INTERNAL_RUNTIME_1X_DORM_TIMER_PARAM;
      set_req_param.dorm_timer_for_1x = *(unsigned long*)argval_ptr;
      DSQMH_IOCTL_SET_INTERNAL_SETTING( "1X DORM TIMER" );
      break;
      
    case PS_IFACE_IOCTL_707_SDB_SUPPORT_QUERY:
    {
        dss_iface_ioctl_707_sdb_support_query_type * sdb_ptr = argval_ptr;
        memset(&req_param, 0, sizeof(req_param));
        req_param.params_mask = QMI_WDS_REQ_INTERNAL_SETTINGS_PARAM | 
                                QMI_WDS_REQ_SDB_FLAGS_PARAM;
      req_param.req_internal_settings =
        QMI_WDS_RUNTIME_SETTINGS_SDB_SUPPORT_REQ_PARAM;
        req_param.sdb_flags = sdb_ptr->flags;
      DSQMH_IOCTL_GET_INTERNAL_SETTING( "SDB SUPPORT" );
        sdb_ptr->can_do_sdb = (uint32)rsp_data.sdb_support;
    }
      break;

    case PS_IFACE_IOCTL_707_ENABLE_HOLDDOWN:
      set_req_param.params_mask =
        QMI_WDS_SET_INTERNAL_RUNTIME_ENABLE_HOLDDOWN_PARAM;
      set_req_param.holddown_enable = *(qmi_wds_enable_holdown*)argval_ptr;
      DSQMH_IOCTL_SET_INTERNAL_SETTING( "ENABLE HOLDDOWN" );
      break;
      
    case PS_IFACE_IOCTL_707_ENABLE_HDR_HPT_MODE:
      set_req_param.params_mask =
        QMI_WDS_SET_INTERNAL_RUNTIME_ENABLE_HDR_HPT_MODE_PARAM;
      set_req_param.hdr_htp_mode_enable =
        *(qmi_wds_enable_hdr_hpt_mode*)argval_ptr;
      DSQMH_IOCTL_SET_INTERNAL_SETTING( "HDR HPT MODE" );
      break;
      
    case PS_IFACE_IOCTL_707_ENABLE_HDR_REV0_RATE_INERTIA:
    { 
      set_req_param.params_mask =
        QMI_WDS_SET_INTERNAL_RUNTIME_ENABLE_REV0_RATE_INERTIA_PARAM;
      set_req_param.hdr_rev0_rate_inertia_enable =
        *(qmi_wds_enable_hdr_rev0_rate_inertia*)argval_ptr;
      DSQMH_IOCTL_SET_INTERNAL_SETTING( "HDR REV0 RATE INERTIA" );
    }
      break;
#ifdef FEATURE_EIDLE_SCI    
    case PS_IFACE_IOCTL_707_ENABLE_HDR_SLOTTED_MODE:
    {
        ps_iface_ioctl_enable_hdr_slotted_mode *slot_mode_ptr = argval_ptr;
        if( slot_mode_ptr->get_slotted_mode == TRUE )
        {
           req_param.params_mask = QMI_WDS_REQ_INTERNAL_SETTINGS_PARAM;
           req_param.req_internal_settings =
        QMI_WDS_RUNTIME_SETTINGS_HDR_EIDLE_SLOTTED_MODE_OPT_REQ_PARAM;
           DSQMH_IOCTL_GET_INTERNAL_SETTING( "HDR SLOTTED MODE" );
           slot_mode_ptr->get_slotted_mode = 
             rsp_data.hdr_eidle_slot_cycle_value;
        }
        else if( slot_mode_ptr->enable == TRUE )
        {
           set_req_param.params_mask =
             QMI_WDS_SET_INTERNAL_RUNTIME_HDR_SLOTTED_MODE_PARAM;
           set_req_param.slot_cycle_value = 
             slot_mode_ptr->slotted_mode_option;
      DSQMH_IOCTL_SET_INTERNAL_SETTING( "HDR SLOTTED MODE" );
        }
    }
      break;
#endif /* FEATURE_EIDLE_SCI */      
      
    case PS_IFACE_IOCTL_GET_SESSION_TIMER:
      req_param.params_mask = QMI_WDS_REQ_INTERNAL_SETTINGS_PARAM;
      req_param.req_internal_settings =
        QMI_WDS_RUNTIME_SETTINGS_SESSION_TIMER_1X_REQ_PARAM;
      DSQMH_IOCTL_GET_INTERNAL_SETTING( "1X SESSION TIMER" );
      *(uint32*)argval_ptr = rsp_data.dorm_timer_for_1x;
      break;

    case PS_IFACE_IOCTL_SET_SESSION_TIMER:
      set_req_param.params_mask =
        QMI_WDS_SET_INTERNAL_RUNTIME_1X_SESSION_TIMER_PARAM;
      set_req_param.session_timer_1x.timer_select = SESSION_TIMER_1X;
      set_req_param.session_timer_1x.timer_value  = *(int*)argval_ptr;
      DSQMH_IOCTL_SET_INTERNAL_SETTING( "1X SESSION TIMER" );
      break;
      
    case PS_IFACE_IOCTL_707_GET_HDR_1X_HANDDOWN_OPTION:
      req_param.params_mask = QMI_WDS_REQ_INTERNAL_SETTINGS_PARAM;
      req_param.req_internal_settings =
        QMI_WDS_RUNTIME_SETTINGS_HANDDOWN_OPT_HDR_1X_REQ_PARAM;
      DSQMH_IOCTL_GET_INTERNAL_SETTING( "HDR 1X HANDDOWN OPTION" );
      *(uint32*)argval_ptr = (uint32)rsp_data.hdr_handdown_option;
      break;

    case PS_IFACE_IOCTL_707_SET_HDR_1X_HANDDOWN_OPTION:
      set_req_param.params_mask =
        QMI_WDS_SET_INTERNAL_RUNTIME_HDR_1X_HANDDOWN_OPT_PARAM;
      set_req_param.hdr_handdown_option =
        *(qmi_wds_hdr_handdown_option_1x*)argval_ptr;
      DSQMH_IOCTL_SET_INTERNAL_SETTING( "HDR 1X HANDDOWN OPTION" );
      break;
      
    case PS_IFACE_IOCTL_707_GET_HYSTERESIS_ACT_TIMER:
      req_param.params_mask = QMI_WDS_REQ_INTERNAL_SETTINGS_PARAM;
      req_param.req_internal_settings =
        QMI_WDS_RUNTIME_SETTINGS_HYSTERISIS_ACT_TIMER_REQ_PARAM;
      DSQMH_IOCTL_GET_INTERNAL_SETTING( "HYSTERESIS ACT TIMER" );
      *(uint32*)argval_ptr = rsp_data.hysteresis_activation_timer;
      break;

    case PS_IFACE_IOCTL_707_SET_HYSTERESIS_ACT_TIMER:
      set_req_param.params_mask =
        QMI_WDS_SET_INTERNAL_RUNTIME_HYSTERISIS_ACTIVATION_TIMER_PARAM;
      set_req_param.hysteresis_activation_timer = *(unsigned long*)argval_ptr;
      DSQMH_IOCTL_SET_INTERNAL_SETTING( "HYSTERESIS ACT TIMER" );
      break;
      
    case PS_IFACE_IOCTL_UMTS_GET_IM_CN_FLAG:
      DSQMH_IOCTL_GET_CURRENT_SETTING( "IM CN FLAG",
                                       QMI_WDS_CURR_CALL_INFO_IM_CN_FLAG );
      *(uint32*)argval_ptr = (uint32)current_info.call_info.im_cn_flag;
      break;

    case PS_IFACE_IOCTL_GET_NETWORK_SUPPORTED_QOS_PROFILES:
      DSQMH_MSG_MED( "DSQMH IOCTL IFACE GET NETWORK SUPPORTED "
                     "QOS PROFILES: iface=%d",
                     (uint32)iface_ptr->client_data_ptr, 0, 0 );
      if( DSQMH_FAILED ==                                                  
          dsqmhllif_get_net_qos_profiles( (uint32)iface_ptr->client_data_ptr,
                                          (uint32)PS_IFACE_NETWORK_CDMA,
                                          argval_ptr,
                                          ps_errno ) )
      {
        DSQMH_MSG_ERROR( "QMH IOCTL failed: GET NETWORK SUPPORTED "
                         "QOS PROFILES",0,0,0);
        result = DSQMH_FAILED;                                             
      }
      break;
#ifndef FEATURE_DSS_LINUX
    case PS_IFACE_IOCTL_UMTS_GET_SESSION_PARAMS:
    {
      int request_mask =  QMI_WDS_GET_CURR_CALL_INFO_PROFILE_ID_PARAM_MASK | 
                          QMI_WDS_GET_CURR_CALL_INFO_PROFILE_NAME_PARAM_MASK | 
                          QMI_WDS_GET_CURR_CALL_INFO_AUTH_PROTOCOL_PARAM_MASK | 
                          QMI_WDS_GET_CURR_CALL_INFO_USERNAME_PARAM_MASK |
                          QMI_WDS_GET_CURR_CALL_INFO_APN_NAME_PARAM_MASK | 
                          QMI_WDS_GET_CURR_CALL_INFO_PDP_TYPE_PARAM_MASK |
                          QMI_WDS_GET_CURR_CALL_INFO_UMTS_GPRS_GRNTD_QOS_PARAM_MASK | 
                          QMI_WDS_GET_CURR_CALL_INFO_PCSCF_ADDR_USING_PCO_PARAM_MASK;

      DSQMH_IOCTL_GET_CURRENT_SETTING( "UMTS SESSION SETTINGS",
                                        request_mask );
      dsqmhioctl_populate_umts_session_settings(&current_info, argval_ptr);
      break;  
    }
#endif
    default:
      DSQMH_MSG_ERROR( "QMH IOCTL unsupported cmd:%d", ioctl_name, 0, 0 );
      *ps_errno = DS_EINVAL;
      result = DSQMH_FAILED;
      break;
  }

  return result;
} /* dsqmhioctl_iface_current_settings_hdlr() */



/*===========================================================================
FUNCTION  DSQMHIOCTL_IFACE_MT_REGISTER_HDLR

DESCRIPTION
  This function manages the mobile-terminate PS call registration commands.

PARAMETERS
  iface_ptr         - Pointer to ps_iface
  ioctl_name        - The operation name
  argval_ptr        - Pointer to operation specific structure
  ps_errno          - Error code returned in case of failure (Error
                      values are those defined in dserrno.h)

DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  Message buffer memory is returned to task heap.
===========================================================================*/
LOCAL int dsqmhioctl_iface_mt_register_hdlr
(
  ps_iface_type           *iface_ptr,
  ps_iface_ioctl_type      ioctl_name,
  void                    *argval_ptr,
  sint15                  *ps_errno
)
{
  int32 result = DSQMH_SUCCESS;
  qmi_wds_reg_mob_term_call_req_params   reg_params;
  qmi_wds_reg_mob_terminated_call_rsp_type  mt_handle; 
  qmi_wds_de_reg_mob_terminated_call_rsp_type      rsp_info;                       
  int                                    qmi_err_code;
  ps_iface_ioctl_mt_reg_cb_type         *mt_reg_ptr;
  ps_iface_ioctl_mt_dereg_cb_type       *mt_dereg_ptr;
  acl_policy_info_type                  *policy_ptr;
  dsqmh_msglib_info_type                *qmi_ptr = NULL;
  int32                                  ret_val;
  uint8                                  dummy = 0;
  
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( (uint32)iface_ptr->client_data_ptr );
  if( !qmi_ptr )
  {
    DSQMH_MSG_ERROR( "Invalid iface inst specified: %d",
		     (uint32)iface_ptr->client_data_ptr, 0, 0 );
    return DSQMH_FAILED;
  }

  switch( ioctl_name )
  {
    case PS_IFACE_IOCTL_MT_REG_CB:
      DSQMH_MSG_MED( "DSQMH IOCTL IFACE MT REGISTER: iface=%d",
                     (uint32)iface_ptr->client_data_ptr, 0, 0 );
      memset( (void*)&reg_params, 0x0, sizeof(reg_params) );
      
      mt_reg_ptr = (ps_iface_ioctl_mt_reg_cb_type*)argval_ptr;
      policy_ptr = (acl_policy_info_type*)mt_reg_ptr->acl_pol_ptr;
        
      /* Setup QMI message structure */
      DSQMH_SET_TECH_PREF( policy_ptr->iface, dummy, reg_params.xtended_tech_pref );
      if( DSQMH_INVALID_TECHPREF == reg_params.xtended_tech_pref )
      {
        DSQMH_MSG_ERROR( "DSQMH IOCTL unsupported tech pref: kind=%d name=%d",
                         policy_ptr->iface.kind, policy_ptr->iface.info.name, 0 );
        *ps_errno = DS_EINVAL;
        return DSQMH_FAILED;
      }
      reg_params.params_mask |= QMI_WDS_MT_REG_CB_TECH_PREF_PARAM;
      (void)dummy;
      
      if( (UMTS_IFACE       == 
           (ps_iface_name_enum_type)reg_params.xtended_tech_pref) ||
          (IFACE_3GPP_GROUP == 
           (ps_iface_name_enum_type)reg_params.xtended_tech_pref) )
      {
        reg_params.profile_index = (unsigned char)((uint32)policy_ptr->pdp_info);
        reg_params.params_mask |= QMI_WDS_MT_REG_CB_PROFILE_INDEX_PARAM;
      }
      else
      {
        DSQMH_MSG_ERROR( "DSQMH IOCTL unsupported tech pref for profile: kind=%d name=%d",
                         policy_ptr->iface.kind, policy_ptr->iface.info.name, 0 );
        *ps_errno = DS_EINVAL;
        return DSQMH_FAILED;
      }

      if( (IFACE_IPV4_ADDR_FAMILY   == policy_ptr->ip_family) ||
          (IFACE_IPV6_ADDR_FAMILY   == policy_ptr->ip_family) ||
          (IFACE_UNSPEC_ADDR_FAMILY == policy_ptr->ip_family) )
      {
        reg_params.ip_family = (qmi_wds_ip_family_pref_type)policy_ptr->ip_family;
        reg_params.params_mask |= QMI_WDS_MT_REG_CB_IP_FAMILY_PARAM;
      }
      else
      {
        DSQMH_MSG_ERROR( "DSQMH IOCTL unsupported addr family: family=%d",
                         policy_ptr->ip_family, 0, 0 );
        *ps_errno = DS_EINVAL;
        return DSQMH_FAILED;
      }

      /* Generate QMI request */
      ret_val = qmi_wds_reg_mobile_terminated_call_req( qmi_ptr->wds_handle,
                                                        &reg_params,
                                                        &mt_handle,
                                                        &qmi_err_code );
      if( 0 > ret_val )
      {
        DSQMH_MSG_ERROR( "QMH IOCTL qmi reg mobile terminated call failed: iface=%d qmi=%d ps=%d",
                         (uint32)iface_ptr->client_data_ptr, qmi_err_code, mt_handle.dss_errno );
        *ps_errno = (sint15)mt_handle.dss_errno;
        result = DSQMH_FAILED;
      }
      else
      {
        /* Pass handle back to client. */
        mt_reg_ptr->handle = (ps_iface_mt_handle_type*)mt_handle.pkt_hndl;
      }
      break;
      
    case PS_IFACE_IOCTL_MT_DEREG_CB:
      DSQMH_MSG_MED( "DSQMH IOCTL IFACE MT DEREGISTER: iface=%d",
                     (uint32)iface_ptr->client_data_ptr, 0, 0 );
      mt_dereg_ptr = (ps_iface_ioctl_mt_dereg_cb_type*)argval_ptr;

      ret_val = qmi_wds_dereg_mobile_terminated_call_req( qmi_ptr->wds_handle,
                                                          &mt_dereg_ptr->handle,
                                                          &rsp_info,
                                                          &qmi_err_code );
      if( 0 > ret_val )
      {
        DSQMH_MSG_ERROR( "QMH IOCTL qmi dereg mobile terminated call failed: iface=%d err=%d",
                         (uint32)iface_ptr->client_data_ptr, qmi_err_code, 0 );
        DSQMH_SET_PS_ERRNO( rsp_info, (*ps_errno) );
        result = DSQMH_FAILED;
      }
      break;

    default:
      DSQMH_MSG_ERROR( "QMH IOCTL unsupported cmd:%d", ioctl_name, 0, 0 );
      *ps_errno = DS_EINVAL;
      result = DSQMH_FAILED;
      break;
  }

  return result;
} /* dsqmhioctl_iface_mt_register_hdlr() */




/*===========================================================================
FUNCTION  DSQMHIOCTL_LLIF_HDLR

DESCRIPTION
  This function manages the ioctls pertaining to LLIF.

PARAMETERS
  iface_ptr         - Pointer to ps_iface
  ioctl_name        - The operation name
  argval_ptr        - Pointer to operation specific structure
  ps_errno          - Error code returned in case of failure (Error
                      values are those defined in dserrno.h)

DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition
===========================================================================*/
LOCAL int dsqmhioctl_llif_hdlr
(
  ps_iface_type           *iface_ptr,
  ps_iface_ioctl_type     ioctl_name,
  void                    *argval_ptr,
  sint15                  *ps_errno
)
{
  int32                        result = DSQMH_SUCCESS;
  uint32                       i = 0;
  uint32                       iface_inst; /* Index for iface table*/

  DSQMH_ASSERT( iface_ptr, "DSQMH LLIF null iface ptr passed" );
  DSQMH_ASSERT( ps_errno, "DSQMH LLIF null ps_errno ptr passed" );

  *ps_errno = DS_ENOERR;

  iface_inst = (uint32)iface_ptr->client_data_ptr;
  
  if( TRUE != DSQMH_IS_IFACE_INST_VALID( iface_inst ))
  {
    DSQMH_MSG_ERROR( "dsqmhioctl_llif_hdlr: invalid iface inst", 0, 0, 0 );
    return DSQMH_FAILED;
  }

  DSQMH_MSG_LOW( "QMH IOCTL LLIF handler iface:%d ioctl=%d",
                 iface_inst, ioctl_name, 0 );

  switch( ioctl_name )
  {
    case PS_IFACE_IOCTL_GET_INTERFACE_GATEWAY_V4_ADDR:
      {
        ip_addr_type *gw_info;

        DSQMH_ASSERT( argval_ptr, "DSQMH LLIF null ioctl argval ptr passed" );
        DSQMH_MSG_HIGH( "PS_IFACE_IOCTL_GET_INTERFACE_GATEWAY_V4_ADDR: iface=%d",
                       iface_inst, 0, 0 );

        gw_info = argval_ptr;
        result = dsqmhllif_query_interface_gw_addr(iface_inst, 
                                                   gw_info, 
                                                   ps_errno);
      }
      break;

    default:
      {
        DSQMH_MSG_MED( "Unsupported  llif ioctl:%d", ioctl_name, 0, 0 );
        return DSQMH_FAILED;
      }
      break;
  }

  return result;

}


/*===========================================================================
FUNCTION  DSQMHIOCTL_NETPLAT_HDLR

DESCRIPTION
  This function manages the ioctls pertaining to netplatform.

PARAMETERS
  iface_ptr         - Pointer to ps_iface
  ioctl_name        - The operation name
  argval_ptr        - Pointer to operation specific structure
  ps_errno          - Error code returned in case of failure (Error
                      values are those defined in dserrno.h)

DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition
===========================================================================*/
LOCAL int dsqmhioctl_netplat_hdlr
(
  ps_iface_type           *iface_ptr,
  ps_iface_ioctl_type     ioctl_name,
  void                    *argval_ptr,
  sint15                  *ps_errno
)
{
  int32                        result = DSQMH_SUCCESS;
  uint32                       i = 0;
  uint32                       iface_inst; /* Index for iface table*/
  dsqmh_iface_cblk_type        *cblk_ptr = NULL;

  DSQMH_ASSERT( iface_ptr, "DSQMH LLIF null iface ptr passed" );
  DSQMH_ASSERT( ps_errno, "DSQMH LLIF null ps_errno ptr passed" );

  *ps_errno = DS_ENOERR;

  iface_inst = (uint32)iface_ptr->client_data_ptr;
  
  if( TRUE != DSQMH_IS_IFACE_INST_VALID( iface_inst ))
  {
    DSQMH_MSG_ERROR( "dsqmhioctl_netplat_hdlr: invalid iface inst", 0, 0, 0 );
    return DSQMH_FAILED;
  }

  DSQMH_MSG_LOW( "QMH IOCTL NETPLAT handler iface:%d ioctl=%d",
                 iface_inst, ioctl_name, 0 );

  switch( ioctl_name )
  {
    case PS_IFACE_IOCTL_GET_DEVICE_INFO:
      {
        ps_iface_ioctl_device_info_type *device_info;

        DSQMH_ASSERT( argval_ptr, "DSQMH LLIF null ioctl argval ptr passed" );
        DSQMH_MSG_HIGH( "DSQMH IOCTL IFACE GET DEVICE INFO: iface=%d",
                       iface_inst, 0, 0 );

        device_info = argval_ptr;
        cblk_ptr = DSQMH_GET_CBLK_PTR( (uint32) iface_ptr->client_data_ptr);
        result = ds_qmh_netplat_get_device_info(cblk_ptr->netplat_info.conn_id, 
                                                device_info, 
                                                ps_errno);
      }
      break;

    default:
      DSQMH_MSG_MED( "Unsupported  netplat tioctl:%d", ioctl_name, 0, 0 );
      break;
  }

  return result;

}
/*===========================================================================
FUNCTION  DSQMHIOCTL_IFACE_BCAST_SVCS_HDLR

DESCRIPTION
  This function manages the broadcast services commands.

PARAMETERS
  iface_ptr         - Pointer to ps_iface
  ioctl_name        - The operation name
  argval_ptr        - Pointer to operation specific structure
  ps_errno          - Error code returned in case of failure (Error
                      values are those defined in dserrno.h)

DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  Message buffer memory is returned to task heap.
===========================================================================*/
LOCAL int dsqmhioctl_iface_bcast_svcs_hdlr
(
  ps_iface_type           *iface_ptr,
  ps_iface_ioctl_type      ioctl_name,
  void                    *argval_ptr,
  sint15                  *ps_errno
)
{
  int32 result = DSQMH_SUCCESS;
  qmi_wds_mcast_ipv4_join_info_param_type   join_info;
  qmi_wds_mcast_join_req_params_type        join_ex_info;
  qmi_wds_mcast_hndl_list_type              mcast_hndl_list;
  qmi_wds_mbms_context_req_params_type      mbms_info;
  qmi_wds_mbms_context_handle               mbms_hndl;
  ps_iface_ioctl_mcast_join_type        *join_ptr = NULL;
  ps_iface_ioctl_mcast_join_ex_type     *join_ex_ptr = NULL;
  ps_iface_ioctl_mbms_mcast_param_type  *param_ptr = NULL;
  ps_iface_ioctl_mcast_leave_type       *leave_ptr = NULL;
  ps_iface_ioctl_mcast_leave_ex_type    *leave_ex_ptr = NULL;
  ps_iface_ioctl_mcast_register_ex_type *register_ex_ptr = NULL;
  ps_iface_ioctl_mbms_mcast_context_act_type   *ctx_act_ptr = NULL;
  ps_iface_ioctl_mbms_mcast_context_deact_type *ctx_dact_ptr = NULL;
#ifdef FEATURE_BCMCS
  qmi_wds_bom_caching_setup_req_param_type  bom_setup;
  qmi_wds_bcmcs_handoff_optimization_info   ho_reg;
  qmi_wds_bcmcs_db_updt_params_type         db_update;
  ps_iface_ioctl_bcmcs_enable_handoff_reg_type *ho_reg_ptr = NULL;
  ps_iface_ioctl_bcmcs_bom_caching_setup_type  *bom_setup_ptr = NULL;
  ps_iface_ioctl_bcmcs_db_update_type          *db_update_ptr = NULL;
  ip_addr_type temp_addr;
#endif /* FEATURE_BCMCS */ 
  struct ps_in6_addr *ipv6_addr_ptr = NULL;
  uint32 i = 0;
  
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void)ipv6_addr_ptr;

  switch( ioctl_name )
  {
    case PS_IFACE_IOCTL_MCAST_JOIN:
      DSQMH_MSG_MED( "DSQMH IOCTL IFACE MCAST JOIN: iface=%d",
                     (uint32)iface_ptr->client_data_ptr, 0, 0 );
      
      join_ptr = (ps_iface_ioctl_mcast_join_type*)argval_ptr;
      param_ptr = (ps_iface_ioctl_mbms_mcast_param_type*)join_ptr->mcast_param_ptr;
        
      /* Setup QMI message structure */
      join_info.params_mask = QMI_WDS_MCAST_JOIN_IPV4_ADDR_INFO_PARAM;
      join_info.mcast_ip_addr_info.mcast_port = ps_ntohs( join_ptr->port ); 
      join_info.mcast_ip_addr_info.mcast_ip_addr =
        ps_ntohl( join_ptr->ip_addr.addr.v4 );                
        
      if( param_ptr )
      {
        join_info.params_mask |= QMI_WDS_MCAST_JOIN_MBMS_SPECIFIC_JOIN_INFO_PARAM;
        memcpy( (void*)join_info.mbms_specific_join_info.tmgi,
                &param_ptr->tmgi,
                sizeof(join_info.mbms_specific_join_info.tmgi) );
        memcpy( (void*)join_info.mbms_specific_join_info.session_start_timer,
                &param_ptr->session_start_time,
                sizeof(join_info.mbms_specific_join_info.session_start_timer) );
        memcpy( (void*)join_info.mbms_specific_join_info.session_end_timer,
                &param_ptr->session_end_time,
                sizeof(join_info.mbms_specific_join_info.session_end_timer) );
        join_info.mbms_specific_join_info.priority = param_ptr->priority;
        join_info.mbms_specific_join_info.service_method =
          (qmi_wds_mbms_service_method)param_ptr->service_method;
        join_info.mbms_specific_join_info.service_type =
          (qmi_wds_mbms_service_type)param_ptr->service_type;
        join_info.mbms_specific_join_info.selected_service =
          (qmi_wds_bool_type)param_ptr->selected_service;
        join_info.mbms_specific_join_info.service_security =
          (qmi_wds_bool_type)param_ptr->service_security;
      }
      
      if( DSQMH_SUCCESS !=
          dsqmhllif_mcast_manager( ioctl_name,
                                   (uint32)iface_ptr->client_data_ptr,
                                   (void*)&join_info,
                                   &mcast_hndl_list,
                                   ps_errno ) )
      {
        DSQMH_MSG_ERROR( "QMH IOCTL qmi mcast join failed: iface=%d",
                         (uint32)iface_ptr->client_data_ptr, 0, 0 );
        result = DSQMH_FAILED;
      }
      else
      {
        /* Return handle to client. */
        if( 1 == mcast_hndl_list.num_handles )
        {
          join_ptr->handle = mcast_hndl_list.mcast_handle_list[0];
        }
        else
        {
          DSQMH_MSG_ERROR( "QMH IOCTL qmi mcast join returned too many handles: %d",
                           mcast_hndl_list.num_handles, 0, 0 );
          result = DSQMH_FAILED;
        }
      }
      break;

    case PS_IFACE_IOCTL_MCAST_JOIN_EX:
      DSQMH_MSG_MED( "DSQMH IOCTL IFACE MCAST JOIN EX: iface=%d",
                     (uint32)iface_ptr->client_data_ptr, 0, 0 );
      
      if( NULL == argval_ptr)
      {
        DSQMH_MSG_ERROR( "QMH IOCTL qmi mcast join invalid argument", 0, 0, 0 );
        return DSQMH_FAILED;
      }
      join_ex_ptr = (ps_iface_ioctl_mcast_join_ex_type*)argval_ptr;

      /* Validate number of flows */
      join_ex_info.num_flows = (unsigned char)join_ex_ptr->num_flows;
      if( QMI_WDS_MAX_MCAST_ADDRS < join_ex_info.num_flows )
      {
        DSQMH_MSG_ERROR( "QMH IOCTL qmi mcast join too many flows requested: "
                         "iface=%d req=%d max=%d",
                         (uint32)iface_ptr->client_data_ptr, 
                         join_ex_info.num_flows, QMI_WDS_MAX_MCAST_ADDRS );
        return DSQMH_FAILED;
      }
      
      /* Setup QMI message structure */
      for( i=0; i < join_ex_info.num_flows; i++ )                                         
      {                                                                  
        if( IFACE_IPV4_ADDR_FAMILY == join_ex_ptr->ip_addr[i].type )
        {
          DSQMH_MSG_IPV4_ADDR( "DSQMH IOCTL IFACE MCAST JOIN EX: ipv4addr=", 
                               join_ex_ptr->ip_addr[i].addr.v4 );
          join_ex_info.ip_info[i].mcast_ip_info.ip_addr.ipv4 =
            ps_ntohl( join_ex_ptr->ip_addr[i].addr.v4 );
        }
        else
        {
          DSQMH_MSG_IPV6_ADDR( "DSQMH IOCTL IFACE MCAST JOIN EX: ipv6addr=",
                               join_ex_ptr->ip_addr[i].addr.v6 );
          memcpy((void*)&join_ex_info.ip_info[i].mcast_ip_info.ip_addr,      
                 &join_ex_ptr->ip_addr[i].addr,                                  
                 sizeof(join_ex_info.ip_info[i].mcast_ip_info.ip_addr) );    
        }
        
        join_ex_info.ip_info[i].mcast_ip_info.ip_family =                  
          (multicast_ip_family)join_ex_ptr->ip_addr[i].type;                   
        join_ex_info.ip_info[i].mcast_ip_info.mcast_port =
	  ps_ntohs( join_ex_ptr->port[i] ); 
        join_ex_info.ip_info[i].config_flag =
          (qmi_wds_mcast_req_config_flag)join_ex_ptr->mcast_request_flags[i];                        
      }
      
      if( DSQMH_SUCCESS !=
          dsqmhllif_mcast_manager( ioctl_name,
                                   (uint32)iface_ptr->client_data_ptr,
                                   (void*)&join_ex_info,
                                   &mcast_hndl_list,
                                   ps_errno ) )
      {
        DSQMH_MSG_ERROR( "QMH IOCTL qmi mcast join failed: iface=%d",
                         (uint32)iface_ptr->client_data_ptr, 0, 0 );
        result = DSQMH_FAILED;
      }
      else
      {
        /* Return handles to client. */
        if(PS_IFACE_MAX_MCAST_FLOWS <= mcast_hndl_list.num_handles )
        {
          (void)memcpy( (void*)join_ex_ptr->handle,
                        mcast_hndl_list.mcast_handle_list,
                        MIN((sizeof(ps_iface_mcast_handle_type) *
                             mcast_hndl_list.num_handles),
                            sizeof(mcast_hndl_list.mcast_handle_list)) );
        }
        else
        {
          DSQMH_MSG_ERROR( "QMH IOCTL qmi mcast join returned too many handles: %d",
                           mcast_hndl_list.num_handles, 0, 0 );
          result = DSQMH_FAILED;
        }
      }
      break;
      
    case PS_IFACE_IOCTL_MCAST_LEAVE:
      DSQMH_MSG_MED( "DSQMH IOCTL IFACE MCAST LEAVE: iface=%d",
                     (uint32)iface_ptr->client_data_ptr, 0, 0 );

      if( NULL == argval_ptr)
      {
        DSQMH_MSG_ERROR( "QMH IOCTL qmi mcast leave invalid argument", 0, 0, 0 );
        return DSQMH_FAILED;
      }
      leave_ptr = (ps_iface_ioctl_mcast_leave_type*)argval_ptr;

      /* Setup QMI message structure */
      mcast_hndl_list.num_handles = 1;
      mcast_hndl_list.mcast_handle_list[0] = leave_ptr->handle;

      if( DSQMH_SUCCESS !=
          dsqmhllif_mcast_manager( ioctl_name,
                                   (uint32)iface_ptr->client_data_ptr,
                                   NULL,
                                   &mcast_hndl_list,
                                   ps_errno ) )
      {
        DSQMH_MSG_ERROR( "QMH IOCTL qmi mcast leave failed: iface=%d",
                         (uint32)iface_ptr->client_data_ptr, 0, 0 );
        result = DSQMH_FAILED;
      }
      break;
      
    case PS_IFACE_IOCTL_MCAST_LEAVE_EX:
      DSQMH_MSG_MED( "DSQMH IOCTL IFACE MCAST LEAVE EX: iface=%d",
                     (uint32)iface_ptr->client_data_ptr, 0, 0 );

      if( NULL == argval_ptr)
      {
        DSQMH_MSG_ERROR( "QMH IOCTL qmi mcast leave_ex invalid argument", 0, 0, 0 );
        return DSQMH_FAILED;
      }
      leave_ex_ptr = (ps_iface_ioctl_mcast_leave_ex_type*)argval_ptr;

      /* Setup QMI message structure */
      mcast_hndl_list.num_handles = leave_ex_ptr->num_flows;
      if( QMI_WDS_MAX_MCAST_HNDLS < mcast_hndl_list.num_handles )
      {
        DSQMH_MSG_ERROR( "QMH IOCTL qmi mcast leave too many handles requested: "
                         "iface=%d req=%d max=%d",
                         (uint32)iface_ptr->client_data_ptr, 
                         mcast_hndl_list.num_handles, QMI_WDS_MAX_MCAST_HNDLS );
        return DSQMH_FAILED;
      }

      (void)memcpy( (void*)mcast_hndl_list.mcast_handle_list,
                    leave_ex_ptr->handle,
                    sizeof(unsigned long) *
                    mcast_hndl_list.num_handles );

      if( DSQMH_SUCCESS !=
          dsqmhllif_mcast_manager( ioctl_name,
                                   (uint32)iface_ptr->client_data_ptr,
                                   NULL,
                                   &mcast_hndl_list,
                                   ps_errno ) )
      {
        DSQMH_MSG_ERROR( "QMH IOCTL qmi mcast leave failed: iface=%d",
                         (uint32)iface_ptr->client_data_ptr, 0, 0 );
        result = DSQMH_FAILED;
      }
      break;

    case PS_IFACE_IOCTL_MCAST_REGISTER_EX:
      DSQMH_MSG_MED( "DSQMH IOCTL IFACE MCAST REGISTER EX: iface=%d",
                     (uint32)iface_ptr->client_data_ptr, 0, 0 );

      if( NULL == argval_ptr)
      {
        DSQMH_MSG_ERROR( "QMH IOCTL qmi mcast register_ex invalid argument", 0, 0, 0 );
        return DSQMH_FAILED;
      }
      register_ex_ptr = (ps_iface_ioctl_mcast_register_ex_type*)argval_ptr;

      /* Setup QMI message structure */
      mcast_hndl_list.num_handles = register_ex_ptr->num_flows;
      if( QMI_WDS_MAX_MCAST_HNDLS < mcast_hndl_list.num_handles )
      {
        DSQMH_MSG_ERROR( "QMH IOCTL qmi mcast register too many handles requested: "
                         "iface=%d req=%d max=%d",
                         (uint32)iface_ptr->client_data_ptr, 
                         mcast_hndl_list.num_handles, QMI_WDS_MAX_MCAST_HNDLS );
        return DSQMH_FAILED;
      }

      (void)memcpy( (void*)mcast_hndl_list.mcast_handle_list,
                    register_ex_ptr->handle,
                    sizeof(unsigned long) *
                    mcast_hndl_list.num_handles );

      if( DSQMH_SUCCESS !=
          dsqmhllif_mcast_manager( ioctl_name,
                                   (uint32)iface_ptr->client_data_ptr,
                                   NULL,
                                   &mcast_hndl_list,
                                   ps_errno ) )
      {
        DSQMH_MSG_ERROR( "QMH IOCTL qmi mcast register failed: iface=%d",
                         (uint32)iface_ptr->client_data_ptr, 0, 0 );
        result = DSQMH_FAILED;
      }
      break;
      
    case PS_IFACE_IOCTL_MBMS_MCAST_CONTEXT_ACTIVATE:
      DSQMH_MSG_MED( "DSQMH IOCTL IFACE MBMS CONTEXT_ACTIVATE: iface=%d",
                     (uint32)iface_ptr->client_data_ptr, 0, 0 );

      ctx_act_ptr = (ps_iface_ioctl_mbms_mcast_context_act_type*)argval_ptr;

      DSQMH_MSG_IPV4_ADDR( "DSQMH IOCTL IFACE MBMS CONTEXT_ACTIVATE: ipv4addr=", 
                           ctx_act_ptr->ip_addr.addr.v4 );
      
      /* Setup QMI message structure */
      mbms_info.param_mask = QMI_WDS_IPV4_CONTEXT_ACTIVATION_INFO_PARAM;
      mbms_info.ipv4_context_activate_info.profile_id =
        (unsigned char)ctx_act_ptr->profile_id;
      mbms_info.ipv4_context_activate_info.multicast_ip_addr =
        ps_ntohl( ctx_act_ptr->ip_addr.addr.v4 );
        
      if( DSQMH_SUCCESS !=
          dsqmhllif_mbms_manager( DSQMHLLIF_MBMS_OP_ACTIVATE,
                                   (uint32)iface_ptr->client_data_ptr,
                                   &mbms_info,
                                   &mbms_hndl ) )
      {
        DSQMH_MSG_ERROR( "QMH IOCTL qmi mbms context_activate: iface=%d",
                         (uint32)iface_ptr->client_data_ptr, 0, 0 );
        *ps_errno = DS_EINVAL; 
        result = DSQMH_FAILED;
      }
      else
      {
        /* Return handle to client. */
        ctx_act_ptr->handle = mbms_hndl;
      }
      break;

    case PS_IFACE_IOCTL_MBMS_MCAST_CONTEXT_DEACTIVATE:
      DSQMH_MSG_MED( "DSQMH IOCTL IFACE MBMS CONTEXT_DEACTIVATE: iface=%d",
                     (uint32)iface_ptr->client_data_ptr, 0, 0 );

      ctx_dact_ptr = (ps_iface_ioctl_mbms_mcast_context_deact_type*)argval_ptr;

      /* Setup QMI message structure */
      mbms_hndl = ctx_dact_ptr->handle;
        
      if( DSQMH_SUCCESS !=
          dsqmhllif_mbms_manager( DSQMHLLIF_MBMS_OP_DEACTIVATE,
                                  (uint32)iface_ptr->client_data_ptr,
                                  NULL,
                                  &mbms_hndl ) )
      {
        DSQMH_MSG_ERROR( "QMH IOCTL qmi mbms context_deactivate: iface=%d",
                         (uint32)iface_ptr->client_data_ptr, 0, 0 );
        *ps_errno = DS_EINVAL; 
        result = DSQMH_FAILED;
      }
      break;

#ifdef FEATURE_BCMCS
    case PS_IFACE_IOCTL_BCMCS_DB_UPDATE:
      db_update_ptr = (ps_iface_ioctl_bcmcs_db_update_type*)argval_ptr;
      memset((void*)&db_update, 0x0, sizeof(db_update));
      db_update.prog_id                = db_update_ptr->program_id;
      db_update.prog_id_len            = db_update_ptr->program_id_len;
      db_update.flow_discrim_len       = db_update_ptr->flow_discrim_len;
      db_update.flow_discrim           = db_update_ptr->flow_discrim;
      db_update.port                   = ps_ntohs( db_update_ptr->port );
      db_update.frame_type             = (qmi_wds_bcmcs_framing_type)db_update_ptr->framing;
      db_update.protocol_type          = (qmi_wds_bcmcs_protocol_type)db_update_ptr->protocol;
      db_update.crc_length             = db_update_ptr->crc_len;
      db_update.flow_format            = db_update_ptr->flow_format;
      db_update.flow_id                = db_update_ptr->flow_id;
      db_update.overwrite              = (qmi_wds_bcmcs_overwrite)db_update_ptr->overwrite;
      db_update.flow_priority          = db_update_ptr->flow_priority;
      db_update.ip_family              = (multicast_ip_family)db_update_ptr->multicast_ip.type;
      db_update.zone_type              = (qmi_wds_dsbcmcs_zone_type)db_update_ptr->zone.type;
      if( DSBCMCS_ZONE_HDR == db_update_ptr->zone.type )
      {
        memcpy((void*)db_update.subnet_hdr,
               db_update_ptr->zone.zone.subnet,
               sizeof(db_update.subnet_hdr));
      }
      if( IFACE_IPV4_ADDR_FAMILY == db_update_ptr->multicast_ip.type )
      {
        temp_addr.addr.v4 = ps_ntohl( db_update_ptr->multicast_ip.addr.v4 );
        memcpy((void*)db_update.ip_addr.ipv4,
               (void*)&temp_addr.addr.v4,
               sizeof( db_update.ip_addr.ipv4 ) );
      }
      else
      {
        ipv6_addr_ptr =
          (struct ps_in6_addr*)db_update_ptr->multicast_ip.addr.v6;
        memcpy((void*)temp_addr.addr.v6, db_update.ip_addr.ipv6, sizeof(temp_addr.addr.v6));
        DSQMH_NTOH_IPV6_ADDR( (struct ps_in6_addr*)temp_addr.addr.v6,
                              (struct ps_in6_addr*)temp_addr.addr.v6 );
        ipv6_addr_ptr->ps_s6_addr64[0] = ((struct ps_in6_addr*)&temp_addr.addr)->ps_s6_addr64[0];
        ipv6_addr_ptr->ps_s6_addr64[1] = ((struct ps_in6_addr*)&temp_addr.addr)->ps_s6_addr64[1];
      }
      
      if( DSQMH_SUCCESS !=
          dsqmhllif_bcmcs_manager( DSQMHLLIF_BCMCS_OP_DB_UPDATE,
                                   (uint32)iface_ptr->client_data_ptr,
                                   (void*)&ho_reg,
                                   ps_errno ) )
      {
        DSQMH_MSG_ERROR( "QMH IOCTL qmi bcmcs bom db update failed: iface=%d",
                         (uint32)iface_ptr->client_data_ptr, 0, 0 );
        result = DSQMH_FAILED;
      }
      break;
      
    case PS_IFACE_IOCTL_BCMCS_ENABLE_HANDOFF_REG:
      ho_reg_ptr = (ps_iface_ioctl_bcmcs_enable_handoff_reg_type*)argval_ptr;
      ho_reg.num_mcast_addr = ho_reg_ptr->num_mcast_addr;
      for( i=0; i < ho_reg.num_mcast_addr; i++ )
      {
        ho_reg.ip_info[i].mcast_port =
	  ps_ntohs( ho_reg_ptr->mcast_addr_info[i].port );
        ho_reg.ip_info[i].ip_family =
	  (multicast_ip_family)ho_reg_ptr->mcast_addr_info[i].ip_addr.type;
        if( IFACE_IPV4_ADDR_FAMILY ==
	    ho_reg_ptr->mcast_addr_info[i].ip_addr.type )
        {
          temp_addr.addr.v4 =
	    ps_ntohl( ho_reg_ptr->mcast_addr_info[i].ip_addr.addr.v4 );
          memcpy((void*)ho_reg.ip_info[i].ip_addr.ipv4,
                 (void*)&temp_addr.addr.v4,
                 sizeof( ho_reg.ip_info[i].ip_addr.ipv4) );
        }
        else
        {
          ipv6_addr_ptr =
            (struct ps_in6_addr*)ho_reg.ip_info[i].ip_addr.ipv6;
          temp_addr = ho_reg_ptr->mcast_addr_info[i].ip_addr;
          DSQMH_NTOH_IPV6_ADDR( (struct ps_in6_addr*)temp_addr.addr.v6,
                                (struct ps_in6_addr*)temp_addr.addr.v6 );
          ipv6_addr_ptr->ps_s6_addr64[0] = ((struct ps_in6_addr*)&temp_addr.addr)->ps_s6_addr64[0];
          ipv6_addr_ptr->ps_s6_addr64[1] = ((struct ps_in6_addr*)&temp_addr.addr)->ps_s6_addr64[1];
        }
      }
      if( DSQMH_SUCCESS !=
          dsqmhllif_bcmcs_manager( DSQMHLLIF_BCMCS_OP_HANDOFF_REG,
                                   (uint32)iface_ptr->client_data_ptr,
                                   (void*)&ho_reg,
                                   ps_errno ) )
      {
        DSQMH_MSG_ERROR( "QMH IOCTL qmi bcmcs enable HO failed: iface=%d",
                         (uint32)iface_ptr->client_data_ptr, 0, 0 );
        result = DSQMH_FAILED;
      }
      break;
    
    case PS_IFACE_IOCTL_BCMCS_BOM_CACHING_SETUP:
      bom_setup_ptr = (ps_iface_ioctl_bcmcs_bom_caching_setup_type*)argval_ptr;
      bom_setup.bom_enbl_dsbl = (PS_IFACE_IOCTL_BCMCS_ENABLE_STRICT_BOM_CACHING ==
                                 bom_setup_ptr->bom_caching_setup)? BOM_ENABLE : BOM_DISABLE;
      bom_setup.bom_cache_timeout =  bom_setup_ptr->bom_cache_timeout;
      if( DSQMH_SUCCESS !=
          dsqmhllif_bcmcs_manager( DSQMHLLIF_BCMCS_OP_BOM_CACHING,
                                   (uint32)iface_ptr->client_data_ptr,
                                   (void*)&bom_setup,
                                   ps_errno ) )
      {
        DSQMH_MSG_ERROR( "QMH IOCTL qmi bcmcs bom caching setup failed: iface=%d",
                         (uint32)iface_ptr->client_data_ptr, 0, 0 );
        result = DSQMH_FAILED;
      }
      break;
#endif /* FEATURE_BCMCS */ 
      
    default:
      DSQMH_MSG_ERROR( "QMH IOCTL unsupported cmd:%d", ioctl_name, 0, 0 );
      result = DSQMH_FAILED;
      break;
  }

  return result;
} /* dsqmhioctl_iface_bcast_svcs_hdlr() */


/*===========================================================================
FUNCTION  DSQMHIOCTL_IFACE_CURRENT_SETTINGS_HDLR

DESCRIPTION
  This function manages the query for current settings commands.

PARAMETERS
  iface_ptr         - Pointer to ps_iface
  ioctl_name        - The operation name
  argval_ptr        - Pointer to operation specific structure
  ps_errno          - Error code returned in case of failure (Error
                      values are those defined in dserrno.h)

DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  Message buffer memory is returned to task heap.
===========================================================================*/
LOCAL int dsqmhioctl_flow_current_settings_hdlr
(
  ps_flow_type            *flow_ptr,
  ps_flow_ioctl_type       ioctl_name,
  void                    *argval_ptr,
  sint15                  *ps_errno
)
{
  int32 result = DSQMH_SUCCESS;
  qmi_qos_perform_flow_op_req_type                   flow_req_param;
  qmi_qos_perform_flow_op_resp_type                  flow_rsp_data;
  ps_flow_capability_enum_type cap = PS_FLOW_CAPABILITY_DEFAULT;  
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#define DSQMH_IOCTL_PERFORM_FLOW_OPERATION(name)                            \
  DSQMH_MSG_MED( "DSQMH IOCTL QOS operation "name": flow=0x%p",             \
                 (uint32)flow_ptr->client_data_ptr, 0, 0 );                 \
  if( DSQMH_FAILED ==                                                       \
      dsqmhllif_qos_flow_operation( flow_ptr,                               \
                                    &flow_req_param,                        \
                                    &flow_rsp_data ) )                      \
  {                                                                         \
    DSQMH_MSG_ERROR( "QMH IOCTL QOS operation failed: "name,0,0,0);         \
    if( flow_rsp_data.op_fail_info.num_failures )                           \
    {                                                                       \
      *ps_errno = (sint15)flow_rsp_data.op_fail_info.fail_info[0].dss_errno;\
    }                                                                       \
    result = DSQMH_FAILED;                                                  \
  }
#define QOS_PERFORM_FLOW_OP_TX_QUEUE_LEVEL                  (1<<0)
#define QOS_PERFORM_FLOW_OP_RMAC3_INFO                      (1<<1)
#define QOS_PERFORM_FLOW_OP_TX_STATUS                       (1<<2)
#define QOS_PERFORM_FLOW_OP_GET_INACTIVITY_TIMER            (1<<3)
#define QOS_PERFORM_FLOW_OP_SET_INACTIVITY_TIMER            (1<<4)

  memset((void*)&flow_req_param, 0x0, sizeof(flow_req_param));
  *ps_errno = 0;

  /* Check if flow is default on IFACE */
  if( PS_FLOW_GET_CAPABILITY( flow_ptr, cap ) )
  {
    flow_req_param.params_mask |= QMI_QOS_PERFORM_FLOW_OP_PRIMARY_FLOW_OP_PARAM;
    flow_req_param.primary_flow_op = TRUE;
  }
  
  switch( ioctl_name )
  {
   case PS_FLOW_IOCTL_GET_TX_QUEUE_LEVEL:
      flow_req_param.params_mask |= QMI_QOS_PERFORM_FLOW_OP_REQ_OPERATIONS_PARAM;
      flow_req_param.qos_identifier = (uint32)flow_ptr->client_data_ptr;
      flow_req_param.requested_operation = QOS_PERFORM_FLOW_OP_TX_QUEUE_LEVEL;
      DSQMH_IOCTL_PERFORM_FLOW_OPERATION( "GET TX QUEUE LEVEL" );
      ((ps_flow_ioctl_tx_queue_level_type*)argval_ptr)->current_new_data_cnt =
        flow_rsp_data.tx_queue_level.current_new_data_cnt;
      ((ps_flow_ioctl_tx_queue_level_type*)argval_ptr)->wm_free_cnt =
        flow_rsp_data.tx_queue_level.wm_free_cnt;
      ((ps_flow_ioctl_tx_queue_level_type*)argval_ptr)->total_pending_cnt =
        flow_rsp_data.tx_queue_level.total_pending_cnt;
      break;
      
   case PS_FLOW_IOCTL_HDR_GET_RMAC3_INFO:
      flow_req_param.params_mask |= QMI_QOS_PERFORM_FLOW_OP_REQ_OPERATIONS_PARAM;
      flow_req_param.qos_identifier = (uint32)flow_ptr->client_data_ptr;
      flow_req_param.requested_operation = QOS_PERFORM_FLOW_OP_RMAC3_INFO;
      DSQMH_IOCTL_PERFORM_FLOW_OPERATION( "GET HDR RMAC3 INFO" );
      ((ps_flow_ioctl_707_hdr_rmac3_info_type*)argval_ptr)->ps_headroom_payload_size =
        flow_rsp_data.rmac3_info.ps_headroom_payload_size;
      ((ps_flow_ioctl_707_hdr_rmac3_info_type*)argval_ptr)->bucket_level_payload_size =
        flow_rsp_data.rmac3_info.bucket_level_payload_size;
      ((ps_flow_ioctl_707_hdr_rmac3_info_type*)argval_ptr)->t2p_inflow_payload_size =
        flow_rsp_data.rmac3_info.t2p_inflow_payload_size;
      
      break;

    case PS_FLOW_IOCTL_707_GET_TX_STATUS:
      flow_req_param.params_mask |= QMI_QOS_PERFORM_FLOW_OP_REQ_OPERATIONS_PARAM;
      flow_req_param.qos_identifier = (uint32)flow_ptr->client_data_ptr;
      flow_req_param.requested_operation = QOS_PERFORM_FLOW_OP_TX_STATUS;
      DSQMH_IOCTL_PERFORM_FLOW_OPERATION( "GET TX STATUS" );
      *(ps_flow_ioctl_707_tx_status_type*)argval_ptr =
        (ps_flow_ioctl_707_tx_status_type)flow_rsp_data.flow_status;
      break;
      
    case PS_FLOW_IOCTL_707_GET_INACTIVITY_TIMER:
      flow_req_param.params_mask |= QMI_QOS_PERFORM_FLOW_OP_REQ_OPERATIONS_PARAM;
      flow_req_param.qos_identifier = (uint32)flow_ptr->client_data_ptr;
      flow_req_param.requested_operation = QOS_PERFORM_FLOW_OP_GET_INACTIVITY_TIMER;
      DSQMH_IOCTL_PERFORM_FLOW_OPERATION( "GET INACTIVITY TIMER" );
      *(ps_flow_ioctl_707_inactivity_timer_type*)argval_ptr =
        flow_rsp_data.inactivity_timer;
      break;

    case PS_FLOW_IOCTL_707_SET_INACTIVITY_TIMER:
      flow_req_param.params_mask |= QMI_QOS_PERFORM_FLOW_OP_SET_INACTIVITY_TIMER_PARAM |
                                      QMI_QOS_PERFORM_FLOW_OP_REQ_OPERATIONS_PARAM;
      flow_req_param.qos_identifier = (uint32)flow_ptr->client_data_ptr;
      flow_req_param.requested_operation = QOS_PERFORM_FLOW_OP_SET_INACTIVITY_TIMER;
      flow_req_param.set_inactivity_timer = (unsigned long)argval_ptr;
      DSQMH_IOCTL_PERFORM_FLOW_OPERATION( "SET INACTIVITY TIMER" );
      break;

    case PS_FLOW_IOCTL_QOS_GET_STATUS:
#if 0 /* Modem mode handlers do not all support this IOCTL */
      if( DSQMH_FAILED ==                                                  
          dsqmhllif_query_qos_settings( DSQMHLLIF_QOS_INFO_STATUS,
                                        (uint32)flow_ptr,
                                        argval_ptr ) )
      {
        DSQMH_MSG_ERROR( "QMH IOCTL failed: QOS_GET_STATUS",0,0,0);
        result = DSQMH_FAILED;                                             
      }
#else
      *(ps_flow_state_enum_type*)argval_ptr = PS_FLOW_GET_STATE( flow_ptr );
#endif
      break;

    default:
      DSQMH_MSG_ERROR( "QMH IOCTL unsupported cmd:%d", ioctl_name, 0, 0 );
      *ps_errno = DS_EINVAL;
      result = DSQMH_FAILED;
      break;
  }

  return result;
}


/*===========================================================================
  
                    EXTERNAL FUNCTION DEFINITIONS

===========================================================================*/

/*===========================================================================
FUNCTION  DSQMHIOCTL_IFACE_DISPATCHER

DESCRIPTION
  This function is the dispatcher for IFACE IOCTLs sent by clients. Any
  IOCTL not supported will be treated as an error.

PARAMETERS
  iface_ptr         - Pointer to ps_iface

  ioctl_name        - The operation name

  argval_ptr        - Pointer to operation specific structure

  ps_errno          - Error code returned in case of failure (Error
                      values are those defined in dserrno.h)

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  Message buffer memory is returned to task heap.
===========================================================================*/
int dsqmhioctl_iface_dispatcher
(
  ps_iface_type           *iface_ptr,
  ps_iface_ioctl_type      ioctl_name,
  void                    *argval_ptr,
  sint15                  *ps_errno
)
{
  int32 result = DSQMH_FAILED;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_LOW( "DSQMH IOCTL IFACE DISPATCHER: iface_inst=%d cmd id=%d",
                 iface_ptr->client_data_ptr, ioctl_name, 0 );
  
  *ps_errno = 0;
  
  /*-------------------------------------------------------------------------
    Dispatch the message to the specific handler.
  -------------------------------------------------------------------------*/
  switch( ioctl_name )
  {
    case PS_IFACE_IOCTL_GET_BEARER_TECHNOLOGY:
    case PS_IFACE_IOCTL_REFRESH_DHCP_CONFIG_INFO:
    case PS_IFACE_IOCTL_GET_DATA_BEARER_RATE:
    case PS_IFACE_IOCTL_GET_RF_CONDITIONS:
    case PS_IFACE_IOCTL_ON_QOS_AWARE_SYSTEM:
    case PS_IFACE_IOCTL_707_ENABLE_HDR_HPT_MODE:
    case PS_IFACE_IOCTL_707_ENABLE_HDR_REV0_RATE_INERTIA:
#ifdef FEATURE_EIDLE_SCI
    case PS_IFACE_IOCTL_707_ENABLE_HDR_SLOTTED_MODE:
#endif      
    case PS_IFACE_IOCTL_707_ENABLE_HOLDDOWN:
    case PS_IFACE_IOCTL_707_GET_DORM_TIMER:
    case PS_IFACE_IOCTL_707_GET_HDR_1X_HANDDOWN_OPTION:
    case PS_IFACE_IOCTL_707_GET_HYSTERESIS_ACT_TIMER:
    case PS_IFACE_IOCTL_707_SDB_SUPPORT_QUERY:
    case PS_IFACE_IOCTL_707_SET_DORM_TIMER:
    case PS_IFACE_IOCTL_707_SET_HDR_1X_HANDDOWN_OPTION:
    case PS_IFACE_IOCTL_707_SET_HYSTERESIS_ACT_TIMER:
    case PS_IFACE_IOCTL_GET_SESSION_TIMER:
    case PS_IFACE_IOCTL_SET_SESSION_TIMER:
    case PS_IFACE_IOCTL_UMTS_GET_IM_CN_FLAG:
    case PS_IFACE_IOCTL_GET_NETWORK_SUPPORTED_QOS_PROFILES:
#ifndef FEATURE_DSS_LINUX
    case PS_IFACE_IOCTL_UMTS_GET_SESSION_PARAMS:
#endif
      result = dsqmhioctl_iface_current_settings_hdlr( iface_ptr, ioctl_name,
                                                       argval_ptr, ps_errno );
      break;

    case PS_IFACE_IOCTL_MT_REG_CB:
    case PS_IFACE_IOCTL_MT_DEREG_CB:
      result = dsqmhioctl_iface_mt_register_hdlr( iface_ptr, ioctl_name,
                                                  argval_ptr, ps_errno );
      break;
    
    case PS_IFACE_IOCTL_MCAST_JOIN:
    case PS_IFACE_IOCTL_MCAST_JOIN_EX:
    case PS_IFACE_IOCTL_MCAST_LEAVE:
    case PS_IFACE_IOCTL_MCAST_LEAVE_EX:
    case PS_IFACE_IOCTL_MCAST_REGISTER_EX:
    case PS_IFACE_IOCTL_MBMS_MCAST_CONTEXT_ACTIVATE:
    case PS_IFACE_IOCTL_MBMS_MCAST_CONTEXT_DEACTIVATE:
    case PS_IFACE_IOCTL_BCMCS_DB_UPDATE:
    case PS_IFACE_IOCTL_BCMCS_ENABLE_HANDOFF_REG:
    case PS_IFACE_IOCTL_BCMCS_BOM_CACHING_SETUP:
      result = dsqmhioctl_iface_bcast_svcs_hdlr( iface_ptr, ioctl_name,
                                                 argval_ptr, ps_errno );
      break;

    case PS_IFACE_IOCTL_GET_DEVICE_INFO:
      result = dsqmhioctl_netplat_hdlr( iface_ptr, ioctl_name,
                                        argval_ptr, ps_errno );
      break;

    case PS_IFACE_IOCTL_GET_INTERFACE_GATEWAY_V4_ADDR:
      result = dsqmhioctl_llif_hdlr( iface_ptr, ioctl_name,
                                        argval_ptr, ps_errno );
      break;


    default:
      DSQMH_MSG_ERROR( "DSQMH IOCTL DISPATCHER: unsupported cmd id=0x%p",
                       ioctl_name,0,0 );
      *ps_errno = DS_EOPNOTSUPP;
      break;
  }

  return result;
} /* dsqmhioctl_iface_dispatcher() */


/*===========================================================================
FUNCTION  DSQMHIOCTL_FLOW_DISPATCHER

DESCRIPTION
  This function is the dispatcher for QOS FLOW IOCTLs sent by clients. Any
  IOCTL not supported will be treated as an error.

PARAMETERS
  flow_ptr          - Pointer to ps_flow

  ioctl_name        - The operation name

  argval_ptr        - Pointer to operation specific structure

  ps_errno          - Error code returned in case of failure (Error
                      values are those defined in dserrno.h)

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  Message buffer memory is returned to task heap.
===========================================================================*/
int dsqmhioctl_flow_dispatcher
(
  ps_flow_type            *flow_ptr,
  ps_flow_ioctl_type       ioctl_name,
  void                    *argval_ptr,
  sint15                  *ps_errno
)
{
  int32 result = DSQMH_FAILED;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_LOW( "DSQMH IOCTL FLOW DISPATCHER: flow=0x%p cmd id=%d",
                 (uint32)flow_ptr->client_data_ptr, ioctl_name, 0 );
  
  *ps_errno = 0;
  
  /*-------------------------------------------------------------------------
    Dispatch the message to the specific handler.
  -------------------------------------------------------------------------*/
  switch( ioctl_name )
  {
    case PS_FLOW_IOCTL_GET_TX_QUEUE_LEVEL:
    case PS_FLOW_IOCTL_HDR_GET_RMAC3_INFO:
    case PS_FLOW_IOCTL_707_GET_TX_STATUS:
    case PS_FLOW_IOCTL_707_GET_INACTIVITY_TIMER:
    case PS_FLOW_IOCTL_707_SET_INACTIVITY_TIMER:
    case PS_FLOW_IOCTL_QOS_GET_STATUS:
      result = dsqmhioctl_flow_current_settings_hdlr( flow_ptr, ioctl_name,
                                                      argval_ptr, ps_errno );
      break;
      
    default:
      DSQMH_MSG_ERROR( "DSQMH IOCTL DISPATCHER: unsupported cmd id=0x%p",
                       ioctl_name,0,0 );
      *ps_errno = DS_EOPNOTSUPP;
      break;
  }

  return result;
} /* dsqmhioctl_flow_dispatcher() */


/*===========================================================================
FUNCTION DSQMHIOCTL_INIT

DESCRIPTION
  This function initializes the QMI Modem Hander mode-specific IOCTL
  handlers. It is invoked during power-up.
  
PARAMETERS
  None.
  
DEPENDENCIES
  None.
  
RETURN VALUE
  None.
  
SIDE EFFECTS 
  None.
  
===========================================================================*/
void dsqmhioctl_init( void )
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/


} /* dsqmhioctl_init() */

#endif /* FEATURE_DATA_PS && FEATURE_QMI_CLIENT */
