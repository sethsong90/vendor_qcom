/******************************************************************************
  @file    qcril_qmi_pdc.c
  @brief   qcril qmi - PDC

  DESCRIPTION
    Handles RIL requests, Callbacks, indications for QMI PDC.

  ---------------------------------------------------------------------------

  Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------
******************************************************************************/


//===========================================================================
//
//                           INCLUDE FILES
//
//===========================================================================

#include <errno.h>
#include <cutils/memory.h>
#include <cutils/properties.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include "ril.h"
#include "comdef.h"
#include "qcrili.h"
#include "qmi_errors.h"
#include "qcril_reqlist.h"
#include "qcril_qmi_client.h"
#include "qcril_qmi_pdc.h"
#include "persistent_device_configuration_v01.h"


//===========================================================================
//
//                    INTERNAL DEFINITIONS AND TYPES
//
//===========================================================================

#define QCRIL_MODEM_MBN_FILE_PATH           "persist.radio.modem_test_mbn_path"
#define QCRIL_MODEM_MBN_DEFAULT_PATH        "/data/modem_config/"
#define QCRIL_MBN_FILE_PATH_LEN             255
#define QCRIL_PDC_FRAME_SIZE                1500

#define PDC_CONFIG_LOCK()    pthread_mutex_lock(&pdc_config_info.pdc_config_mutex)
#define PDC_CONFIG_UNLOCK()  pthread_mutex_unlock(&pdc_config_info.pdc_config_mutex)

// error code
#define QCRIL_PDC_NO_ERROR      0
#define QCRIL_PDC_GENERIC_FAIL  -1
#define QCRIL_PDC_NO_MEMORY     -2
#define QCRIL_PDC_NO_CONFIGS    -3

typedef struct
{
  // user set info
  uint8_t config_id[PDC_CONFIG_ID_SIZE_MAX_V01];
  uint32_t config_id_len;
  uint32_t conf_size;
  uint32_t load_size;
  int conf_fd;

  // for pretecting
  pthread_mutex_t        pdc_config_mutex;
  pthread_mutexattr_t    pdc_config_mutex_attr;
} pdc_config_info_type;

static pdc_config_info_type pdc_config_info;
// the file path that contains mbn files
static char mbn_file_dir[ PROPERTY_VALUE_MAX ];
static pdc_config_info_resp_type_v01 cur_config_lists[ PDC_CONFIG_LIST_SIZE_MAX_V01 ];
static uint32_t cur_config_len;
static uint8_t cur_delete_idx;


/*=========================================================================
  FUNCTION:  qcril_qmi_pdc_load_configuraiton

===========================================================================*/
/*!
    @brief
    Handle QCRIL_EVT_QMI_RIL_PDC_LOAD_CONFIGURATION

    @return
    None.
*/
/*=========================================================================*/
void qcril_qmi_pdc_load_configuration
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr // Output parameter
)
{
  pdc_load_config_req_msg_v01 qmi_request;
  pdc_load_config_resp_msg_v01 qmi_response;
  pdc_load_config_info_type_v01 *p_load_info;
  qmi_client_error_type qmi_error;
  RIL_Errno result = RIL_E_GENERIC_FAILURE;
  char payload;
  int rd_len;

  QCRIL_LOG_FUNC_ENTRY();

  memset( &qmi_request, 0, sizeof(qmi_request) );
  p_load_info = &qmi_request.load_config_info;
  p_load_info->config_type = PDC_CONFIG_TYPE_MODEM_SW_V01;
  p_load_info->total_config_size = pdc_config_info.conf_size;

  strlcpy(p_load_info->config_id, pdc_config_info.config_id, PDC_CONFIG_ID_SIZE_MAX_V01);
  p_load_info->config_id_len = strlen(p_load_info->config_id);

  // read loop in case that it is interrupt by signals
  // for example TASK_FREEZE when sleep */
  if ( pdc_config_info.conf_fd == -1)
  {
    rd_len = -1;
    QCRIL_LOG_ERROR("The MBN file descriptor is -1");
  }
  else
  {
    do
    {
      rd_len = read( pdc_config_info.conf_fd,
          p_load_info->config_frame, QCRIL_PDC_FRAME_SIZE );
    } while ( ( rd_len == -1 ) && ( errno == EINTR ) );
  }

  if ( rd_len == -1 ) // there is some error when read file
  {
    result = RIL_E_GENERIC_FAILURE;
    QCRIL_LOG_ERROR("failed to read MBN file");
  }
  else if (rd_len == 0) // reach file end
  {
    // should not reach here, since the indication handler
    // will not queue this event if all of this config has
    // been loaded. Treat it as a error (one case: the file
    // length is empty)
    result = RIL_E_GENERIC_FAILURE;
    QCRIL_LOG_ERROR("reach file end, should not happen");
  }
  else
  {
    p_load_info->config_frame_len = rd_len;
    pdc_config_info.load_size += rd_len;
    qmi_error = qmi_client_send_msg_sync( qcril_qmi_client_get_user_handle( QCRIL_QMI_CLIENT_PDC ),
                                        QMI_PDC_LOAD_CONFIG_REQ_V01,
                                        &qmi_request,
                                        sizeof( qmi_request ),
                                        &qmi_response,
                                        sizeof( qmi_response ),
                                        QCRIL_QMI_SYNC_REQ_DEF_TIMEOUT
                                        );
   result = qcril_qmi_util_convert_qmi_response_codes_to_ril_result( qmi_error, &qmi_response.resp );
   if ( pdc_config_info.load_size >= pdc_config_info.conf_size )
   {
     QCRIL_LOG_INFO("load_size is %d, conf_size is %d",
         pdc_config_info.load_size, pdc_config_info.conf_size);
     close( pdc_config_info.conf_fd );
     pdc_config_info.conf_fd = -1;
   }
  }

  // on failure, send an unsol message to indicate error
  // on success, do nothing here, the load indication handler
  // will handle the remaining things
  if ( result == RIL_E_GENERIC_FAILURE )
  {
    payload = QCRIL_PDC_GENERIC_FAIL;
    qcril_hook_unsol_response( QCRIL_DEFAULT_INSTANCE_ID, QCRIL_EVT_HOOK_UNSOL_PDC_CONFIG,
                                    (void *)(&payload), sizeof(payload));
  }

  QCRIL_LOG_FUNC_RETURN();
}


/*=========================================================================
  FUNCTION:  qcril_qmi_pdc_load_config_ind_hdlr

===========================================================================*/
/*!
    @brief
    Handle QMI_PDC_LOAD_CONFIG_IND_V01

    @return
    None.
*/
/*=========================================================================*/
void qcril_qmi_pdc_load_config_ind_hdlr
(
  void *ind_data_ptr,
  uint32 ind_data_len
)
{
  pdc_load_config_ind_msg_v01 *load_ind;
  char payload;

  QCRIL_LOG_FUNC_ENTRY();

  if ( ind_data_ptr != NULL )
  {
    load_ind = (pdc_load_config_ind_msg_v01*)ind_data_ptr;
    if ( load_ind->error == QMI_ERR_NONE_V01 )
    {
      // if modem pass the remaining info, we use it to determine
      // whether the load has completed
      if ( load_ind->remaining_config_size_valid )
      {
        QCRIL_LOG_INFO("The remaininng_config_size is %d", load_ind->remaining_config_size);
        QCRIL_LOG_INFO("The received_config_size is %d", load_ind->received_config_size);
        if ( load_ind->remaining_config_size == 0 )
        {
          // kick the select config start
          qcril_event_queue( QCRIL_DEFAULT_INSTANCE_ID,
                        QCRIL_DEFAULT_MODEM_ID,
                        QCRIL_DATA_ON_STACK,
                        QCRIL_EVT_QMI_RIL_PDC_SELECT_CONFIGURATION,
                        NULL,
                        QMI_RIL_ZERO,
                        (RIL_Token) QCRIL_TOKEN_ID_INTERNAL );
        }
        else
        {
          // continue loading config
          qcril_event_queue( QCRIL_DEFAULT_INSTANCE_ID,
                        QCRIL_DEFAULT_MODEM_ID,
                        QCRIL_DATA_ON_STACK,
                        QCRIL_EVT_QMI_RIL_PDC_LOAD_CONFIGURATION,
                        NULL,
                        QMI_RIL_ZERO,
                        (RIL_Token) QCRIL_TOKEN_ID_INTERNAL );
        }
      }
      else
      {
        if ( pdc_config_info.load_size >= pdc_config_info.conf_size )
        {
          // kick the select config start
          qcril_event_queue( QCRIL_DEFAULT_INSTANCE_ID,
                        QCRIL_DEFAULT_MODEM_ID,
                        QCRIL_DATA_ON_STACK,
                        QCRIL_EVT_QMI_RIL_PDC_SELECT_CONFIGURATION,
                        NULL,
                        QMI_RIL_ZERO,
                        (RIL_Token) QCRIL_TOKEN_ID_INTERNAL );
        }
        else
        {
          // continue loading config
          qcril_event_queue( QCRIL_DEFAULT_INSTANCE_ID,
                        QCRIL_DEFAULT_MODEM_ID,
                        QCRIL_DATA_ON_STACK,
                        QCRIL_EVT_QMI_RIL_PDC_LOAD_CONFIGURATION,
                        NULL,
                        QMI_RIL_ZERO,
                        (RIL_Token) QCRIL_TOKEN_ID_INTERNAL );
        }
      }
    }
    else // there is some error
    {
      // case 1: the config is already loaded
      if ( load_ind->error == QMI_ERR_INVALID_ID_V01 )
      {
        QCRIL_LOG_INFO("Invalid config id, maybe already exists");
        // stop loading and kick the select config start
        qcril_event_queue( QCRIL_DEFAULT_INSTANCE_ID,
                      QCRIL_DEFAULT_MODEM_ID,
                      QCRIL_DATA_ON_STACK,
                      QCRIL_EVT_QMI_RIL_PDC_SELECT_CONFIGURATION,
                      NULL,
                      QMI_RIL_ZERO,
                      (RIL_Token) QCRIL_TOKEN_ID_INTERNAL );
      }
      // case 2: no enough space
      else if ( load_ind->error == QMI_ERR_NO_MEMORY_V01 )
      {
        QCRIL_LOG_INFO("no memory in modem EFS");
        payload = QCRIL_PDC_NO_MEMORY;
        qcril_hook_unsol_response( QCRIL_DEFAULT_INSTANCE_ID, QCRIL_EVT_HOOK_UNSOL_PDC_CONFIG,
                                      (void *)(&payload), sizeof(payload) );
      }
      else
      {
        QCRIL_LOG_INFO("Failed to load configuration");
        payload = QCRIL_PDC_GENERIC_FAIL;
        qcril_hook_unsol_response( QCRIL_DEFAULT_INSTANCE_ID, QCRIL_EVT_HOOK_UNSOL_PDC_CONFIG,
                                      (void *)(&payload), sizeof(payload) );
      }
    }
  }
}

/*=========================================================================
  FUNCTION:  qcril_qmi_pdc_select_config_ind_hdlr

===========================================================================*/
/*!
    @brief
    Handle QMI_PDC_SET_SELECTED_CONFIG_IND_V01

    @return
    None.
*/
/*=========================================================================*/
void qcril_qmi_pdc_select_config_ind_hdlr
(
  void *ind_data_ptr,
  uint32 ind_data_len
)
{
  pdc_set_selected_config_ind_msg_v01 *set_ind;
  char payload;

  QCRIL_LOG_FUNC_ENTRY();

  payload = QCRIL_PDC_GENERIC_FAIL;
  if ( ind_data_ptr != NULL )
  {
    set_ind = (pdc_set_selected_config_ind_msg_v01*)ind_data_ptr;
    if ( set_ind->error == QMI_ERR_NONE_V01 )
    {
      QCRIL_LOG_INFO("select successful");
      payload = QCRIL_PDC_NO_ERROR;
    }
    else // there is some error
    {
      QCRIL_LOG_ERROR("select indication error");
      payload = QCRIL_PDC_GENERIC_FAIL;
    }
  }
  else
  {
    QCRIL_LOG_ERROR("select indication param NULL");
    payload = QCRIL_PDC_GENERIC_FAIL;
  }
  qcril_hook_unsol_response( QCRIL_DEFAULT_INSTANCE_ID, QCRIL_EVT_HOOK_UNSOL_PDC_CONFIG,
                                      (void *)(&payload), sizeof(payload) );
}

/*=========================================================================
  FUNCTION:  qcril_qmi_pdc_activate_config_ind_hdlr

===========================================================================*/
/*!
    @brief
    Handle QMI_PDC_ACTIVATE_CONFIG_IND_V01

    @return
    None.
*/
/*=========================================================================*/
void qcril_qmi_pdc_activate_config_ind_hdlr
(
  void *ind_data_ptr,
  uint32 ind_data_len
)
{
  pdc_activate_config_ind_msg_v01 *act_ind;
  char payload;

  QCRIL_LOG_FUNC_ENTRY();

  payload = QCRIL_PDC_GENERIC_FAIL;
  if ( ind_data_ptr != NULL )
  {
    act_ind = (pdc_activate_config_ind_msg_v01*)ind_data_ptr;
    if ( act_ind->error == QMI_ERR_NONE_V01 )
    {
      QCRIL_LOG_INFO("activate successful");
      payload = QCRIL_PDC_NO_ERROR;
    }
    else
    {
      QCRIL_LOG_ERROR("activate error, qmi error num: %d", act_ind->error);
    }
  }

  qcril_hook_unsol_response( QCRIL_DEFAULT_INSTANCE_ID, QCRIL_EVT_HOOK_UNSOL_PDC_CONFIG,
                                (void *)(&payload), sizeof(payload) );

  QCRIL_LOG_FUNC_RETURN();
}


/*=========================================================================
  FUNCTION:  qcril_qmi_pdc_delete_config_ind_hdlr

===========================================================================*/
/*!
    @brief
    Handle QMI_PDC_DELETE_CONFIG_IND_V01

    @return
    None.
*/
/*=========================================================================*/
void qcril_qmi_pdc_delete_config_ind_hdlr
(
  void *ind_data_ptr,
  uint32 ind_data_len
)
{
  pdc_delete_config_ind_msg_v01 *del_ind;
  qcril_reqlist_public_type req_info;
  qcril_request_resp_params_type resp;
  RIL_Errno result = RIL_E_SUCCESS;
  char payload;

  QCRIL_LOG_FUNC_ENTRY();
  if ( ind_data_ptr != NULL )
  {
    del_ind = (pdc_delete_config_ind_msg_v01*)ind_data_ptr;
    if ( del_ind->error == QMI_ERR_NONE_V01 )
    {
      QCRIL_LOG_INFO("delete one configuration successfully");
      cur_delete_idx++;
      // kick the delete task
      qcril_event_queue( QCRIL_DEFAULT_INSTANCE_ID,
                         QCRIL_DEFAULT_MODEM_ID,
                         QCRIL_DATA_ON_STACK,
                         QCRIL_EVT_QMI_RIL_PDC_DELETE_CONFIGURATION,
                         NULL,
                         QMI_RIL_ZERO,
                         (RIL_Token) QCRIL_TOKEN_ID_INTERNAL );
    }
    else
    {
      QCRIL_LOG_ERROR("failed to delete one configuration, error id = %d", del_ind->error);
      result = RIL_E_GENERIC_FAILURE;
    }
  }
  else // ind_data_ptr = NULL
  {
    QCRIL_LOG_ERROR("NULL parameter");
    result = RIL_E_GENERIC_FAILURE;
  }

  if ( result == RIL_E_GENERIC_FAILURE )
  {
    payload = QCRIL_PDC_GENERIC_FAIL;
    qcril_hook_unsol_response( QCRIL_DEFAULT_INSTANCE_ID, QCRIL_EVT_HOOK_UNSOL_PDC_CLEAR_CONFIGS,
                                        (void *)(&payload), sizeof(payload) );
  }
}

/*=========================================================================
  FUNCTION:  qcril_qmi_pdc_list_configs_ind_hdlr

===========================================================================*/
/*!
    @brief
    Handle QMI_PDC_LIST_CONFIGS_IND_V01

    @return
    None.
*/
/*=========================================================================*/
void qcril_qmi_pdc_list_configs_ind_hdlr
(
  void *ind_data_ptr,
  uint32 ind_data_len
)
{
  pdc_list_configs_ind_msg_v01 *list_ind;
  qcril_reqlist_public_type req_info;
  qcril_request_resp_params_type resp;
  RIL_Errno result = RIL_E_SUCCESS;
  char payload;

  QCRIL_LOG_FUNC_ENTRY();

  if ( ind_data_ptr != NULL )
  {
    list_ind = (pdc_list_configs_ind_msg_v01*)ind_data_ptr;
    if ( list_ind->error == QMI_ERR_NONE_V01 )
    {
      if ( (list_ind->config_list_valid) && (list_ind->config_list_len > 0)
            && (list_ind->config_list_len <= PDC_CONFIG_LIST_SIZE_MAX_V01) )
      {
        cur_config_len = list_ind->config_list_len;
        cur_delete_idx = 0;
        memcpy( cur_config_lists, list_ind->config_list,
                cur_config_len * sizeof(pdc_config_info_resp_type_v01));
        QCRIL_LOG_INFO("total configuration count %d", cur_config_len);
        // kick the delete task
        qcril_event_queue( QCRIL_DEFAULT_INSTANCE_ID,
                        QCRIL_DEFAULT_MODEM_ID,
                        QCRIL_DATA_ON_STACK,
                        QCRIL_EVT_QMI_RIL_PDC_DELETE_CONFIGURATION,
                        NULL,
                        QMI_RIL_ZERO,
                        (RIL_Token) QCRIL_TOKEN_ID_INTERNAL );
      }
      else
      {
        QCRIL_LOG_ERROR("no valid config lists available");
        result = QCRIL_PDC_NO_CONFIGS;
      }
    }
    else // QMI ERROR
    {
      QCRIL_LOG_ERROR("QMI error, error code %d", list_ind->error);
      result = RIL_E_GENERIC_FAILURE;
    }
  }
  else // ind_data_ptr = NULL
  {
    QCRIL_LOG_ERROR("NULL parameter");
    result = RIL_E_GENERIC_FAILURE;
  }

  if ( result == RIL_E_GENERIC_FAILURE )
  {
    payload = QCRIL_PDC_GENERIC_FAIL;
    qcril_hook_unsol_response( QCRIL_DEFAULT_INSTANCE_ID, QCRIL_EVT_HOOK_UNSOL_PDC_CLEAR_CONFIGS,
                                        (void *)(&payload), sizeof(payload) );
  }
}

/*=========================================================================
  FUNCTION:  qcril_qmi_pdc_deactivate_config_ind_hdlr

===========================================================================*/
/*!
    @brief
    Handle QMI_PDC_DEACTIVATE_CONFIG_IND_V01

    @return
    None.
*/
/*=========================================================================*/
void qcril_qmi_pdc_deactivate_config_ind_hdlr
(
  void *ind_data_ptr,
  uint32 ind_data_len
)
{
  pdc_deactivate_config_ind_msg_v01 *deact_ind;
  qcril_reqlist_public_type req_info;
  qcril_request_resp_params_type resp;
  RIL_Errno result;

  QCRIL_LOG_FUNC_ENTRY();

  if ( ind_data_ptr != NULL )
  {
    deact_ind = (pdc_deactivate_config_ind_msg_v01*)ind_data_ptr;
    if ( deact_ind->error == QMI_ERR_NONE_V01 )
    {
      result = RIL_E_SUCCESS;
    }
    else // QMI ERROR
    {
      QCRIL_LOG_ERROR("QMI error, error code %d", deact_ind->error);
      result = RIL_E_GENERIC_FAILURE;
    }
  }
  else // ind_data_ptr = NULL
  {
    QCRIL_LOG_ERROR("NULL parameter");
    result = RIL_E_GENERIC_FAILURE;
  }

  // if the there is the error, we still do the delete work anyway
  // kick the LIST_CONFIGS start
  qcril_event_queue( QCRIL_DEFAULT_INSTANCE_ID,
                     QCRIL_DEFAULT_MODEM_ID,
                     QCRIL_DATA_ON_STACK,
                     QCRIL_EVT_QMI_RIL_PDC_LIST_CONFIGURATION,
                     NULL,
                     QMI_RIL_ZERO,
                     (RIL_Token) QCRIL_TOKEN_ID_INTERNAL );

  QCRIL_LOG_FUNC_RETURN();
}

/*=========================================================================
  FUNCTION:  qcril_qmi_pdc_get_selected_config_ind_hdlr

===========================================================================*/
/*!
    @brief
    Handle QMI_PDC_GET_SELECTED_CONFIG_IND_V01

    @return
    None.
*/
/*=========================================================================*/
void qcril_qmi_pdc_get_selected_config_ind_hdlr
(
  void *ind_data_ptr,
  uint32 ind_data_len
)
{
  pdc_get_selected_config_ind_msg_v01 *sel_ind;
  qcril_reqlist_public_type req_info;
  qcril_request_resp_params_type resp;
  uint8_t config_id[PDC_CONFIG_ID_SIZE_MAX_V01];
  uint32_t config_id_len;
  RIL_Errno result = RIL_E_SUCCESS;

  QCRIL_LOG_FUNC_ENTRY();

  if ( ind_data_ptr != NULL )
  {
    sel_ind = (pdc_get_selected_config_ind_msg_v01*)ind_data_ptr;
    if ( sel_ind->error == QMI_ERR_NONE_V01 )
    {
      if ( sel_ind->active_config_id_valid ) // selected and activated
      {
        config_id_len = sel_ind->active_config_id_len;
        if ( config_id_len >= PDC_CONFIG_ID_SIZE_MAX_V01 )
        {
          result = RIL_E_GENERIC_FAILURE;
        }
        else
        {
          memcpy(config_id, sel_ind->active_config_id, config_id_len);
        }
        QCRIL_LOG_INFO("current active config id is %s", config_id);
      }
      else if ( sel_ind->pending_config_id_valid ) // selected but not activated
      {
        config_id_len = sel_ind->pending_config_id_len;
        if ( config_id_len >= PDC_CONFIG_ID_SIZE_MAX_V01 )
        {
          result = RIL_E_GENERIC_FAILURE;
        }
        else
        {
          memcpy(config_id, sel_ind->pending_config_id, config_id_len);
        }
         QCRIL_LOG_INFO("current select config id is %s", config_id);
      }
    }
    else // QMI ERROR
    {
      QCRIL_LOG_ERROR("no selected config id");
      result = RIL_E_GENERIC_FAILURE;
    }
  }
  else // ind_data_ptr = NULL
  {
    QCRIL_LOG_ERROR("NULL parameter");
    result = RIL_E_GENERIC_FAILURE;
  }

  // send response
  if ( qcril_reqlist_query_by_request( QCRIL_DEFAULT_INSTANCE_ID, RIL_REQUEST_OEM_HOOK_RAW, &req_info ) == E_SUCCESS )
  {
    qcril_default_request_resp_params( QCRIL_DEFAULT_INSTANCE_ID,
                                       req_info.t,
                                       req_info.request,
                                       result,
                                       &resp );
    resp.resp_pkt = (void *) config_id;
    resp.resp_len = config_id_len;
    qcril_send_request_response( &resp );
  }
  else
  {
    // FIXME: what need to do?
  }

  QCRIL_LOG_FUNC_RETURN();
}

/*=========================================================================
  FUNCTION:  qcril_qmi_pdc_select_configuration

===========================================================================*/
/*!
    @brief
    Handle QCRIL_EVT_QMI_RIL_PDC_SELECT_CONFIGURATION

    @return
    None.
*/
/*=========================================================================*/
void qcril_qmi_pdc_select_configuration
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr // Output parameter
)
{
  pdc_set_selected_config_req_msg_v01 qmi_request;
  pdc_set_selected_config_resp_msg_v01 qmi_response;
  pdc_config_info_req_type_v01 *p_config_info;
  qmi_client_error_type qmi_error;
  RIL_Errno ril_req_res;
  char payload;

  QCRIL_LOG_FUNC_ENTRY();

  memset( &qmi_request, 0, sizeof(qmi_request) );
  p_config_info = &qmi_request.new_config_info;
  p_config_info->config_type = PDC_CONFIG_TYPE_MODEM_SW_V01;
  p_config_info->config_id_len = pdc_config_info.config_id_len;
  strlcpy(p_config_info->config_id, pdc_config_info.config_id, PDC_CONFIG_ID_SIZE_MAX_V01);

  qmi_error = qmi_client_send_msg_sync( qcril_qmi_client_get_user_handle( QCRIL_QMI_CLIENT_PDC ),
                                        QMI_PDC_SET_SELECTED_CONFIG_REQ_V01,
                                        &qmi_request,
                                        sizeof( qmi_request ),
                                        &qmi_response,
                                        sizeof( qmi_response ),
                                        QCRIL_QMI_SYNC_REQ_DEF_TIMEOUT
                                        );

  ril_req_res = qcril_qmi_util_convert_qmi_response_codes_to_ril_result( qmi_error, &qmi_response.resp );

  // on failure, send an unsol message to indicate error
  // on success, do nothing here, the select indication handler
  // will handle the remaining things
  if ( ril_req_res != RIL_E_SUCCESS )
  {
    payload = QCRIL_PDC_GENERIC_FAIL;
    qcril_hook_unsol_response( QCRIL_DEFAULT_INSTANCE_ID, QCRIL_EVT_HOOK_UNSOL_PDC_CONFIG,
                                    (void *)(&payload), sizeof(payload) );
  }

  QCRIL_LOG_FUNC_RETURN();
}

/*=========================================================================
  FUNCTION:  qcril_qmi_pdc_activate_configuration

===========================================================================*/
/*!
    @brief
    Handle QCRIL_EVT_QMI_RIL_PDC_ACTIVATE_CONFIGURATION

    @return
    None.
*/
/*=========================================================================*/
void qcril_qmi_pdc_activate_configuration
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr // Output parameter
)
{
  pdc_activate_config_req_msg_v01 qmi_request;
  pdc_activate_config_resp_msg_v01 qmi_response;
  qmi_client_error_type qmi_error;
  RIL_Errno ril_req_res;
  char payload;

  QCRIL_LOG_FUNC_ENTRY();

  memset( &qmi_request, 0, sizeof(qmi_request) );
  qmi_request.config_type = PDC_CONFIG_TYPE_MODEM_SW_V01;

  qmi_error = qmi_client_send_msg_sync( qcril_qmi_client_get_user_handle( QCRIL_QMI_CLIENT_PDC ),
                                        QMI_PDC_ACTIVATE_CONFIG_REQ_V01,
                                        &qmi_request,
                                        sizeof( qmi_request ),
                                        &qmi_response,
                                        sizeof( qmi_response ),
                                        QCRIL_QMI_SYNC_REQ_DEF_TIMEOUT
                                        );

  ril_req_res = qcril_qmi_util_convert_qmi_response_codes_to_ril_result( qmi_error, &qmi_response.resp );

  // on failure, send an unsol message to indicate error
  // on success, do nothing here, the activate indication handler
  // will handle the remaining things
  if ( ril_req_res != RIL_E_SUCCESS )
  {
    payload = QCRIL_PDC_GENERIC_FAIL;
    qcril_hook_unsol_response( QCRIL_DEFAULT_INSTANCE_ID, QCRIL_EVT_HOOK_UNSOL_PDC_CONFIG,
                                    (void *)(&payload), sizeof(payload) );
  }

  QCRIL_LOG_FUNC_RETURN();
}


/*=========================================================================
  FUNCTION:  qcril_qmi_pdc_delete_configuration

===========================================================================*/
/*!
    @brief
    Handle QCRIL_EVT_QMI_RIL_PDC_DELETE_CONFIGURATION

    @return
    None.
*/
/*=========================================================================*/
void qcril_qmi_pdc_delete_configuration
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr // Output parameter
)
{
  pdc_delete_config_req_msg_v01 qmi_request;
  pdc_delete_config_resp_msg_v01 qmi_response;
  qcril_request_resp_params_type resp;
  qcril_reqlist_public_type req_info;
  qmi_client_error_type qmi_error;
  RIL_Errno result;
  char payload;

  QCRIL_LOG_FUNC_ENTRY();

  if ( cur_delete_idx == cur_config_len)
  {
    cur_delete_idx = 0;
    QCRIL_LOG_INFO("delete all loaded configuration");
    result = RIL_E_SUCCESS;
  }
  else
  {
    memset( &qmi_request, 0, sizeof(qmi_request) );
    qmi_request.config_type = PDC_CONFIG_TYPE_MODEM_SW_V01;
    qmi_request.config_id_valid = 1;
    qmi_request.config_id_len = cur_config_lists[cur_delete_idx].config_id_len;
    memcpy(qmi_request.config_id, cur_config_lists[cur_delete_idx].config_id,
                      qmi_request.config_id_len);
    QCRIL_LOG_INFO("request to delete config id: %s, index: %d", qmi_request.config_id, cur_delete_idx);
    qmi_error = qmi_client_send_msg_sync( qcril_qmi_client_get_user_handle( QCRIL_QMI_CLIENT_PDC ),
                                          QMI_PDC_DELETE_CONFIG_REQ_V01,
                                          &qmi_request,
                                          sizeof( qmi_request ),
                                          &qmi_response,
                                          sizeof( qmi_response ),
                                          QCRIL_QMI_SYNC_REQ_DEF_TIMEOUT
                                          );
    result = qcril_qmi_util_convert_qmi_response_codes_to_ril_result( qmi_error, &qmi_response.resp );
  }

  // send response, if all configurations are deleted or failed to delete
  if ( ( cur_delete_idx == 0 ) || ( result != RIL_E_SUCCESS ) )
  {
    payload = ( result == RIL_E_SUCCESS ) ? QCRIL_PDC_NO_ERROR : QCRIL_PDC_GENERIC_FAIL;
    qcril_hook_unsol_response( QCRIL_DEFAULT_INSTANCE_ID, QCRIL_EVT_HOOK_UNSOL_PDC_CLEAR_CONFIGS,
                                (void *)(&payload), sizeof(payload) );
  }

  QCRIL_LOG_FUNC_RETURN();
}

/*=========================================================================
  FUNCTION:  qcril_qmi_pdc_list_configuration

===========================================================================*/
/*!
    @brief
    Handle QCRIL_EVT_QMI_RIL_PDC_LIST_CONFIGURATION

    @return
    None.
*/
/*=========================================================================*/
void qcril_qmi_pdc_list_configuration
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr // Output parameter
)
{
  pdc_list_configs_req_msg_v01 qmi_request;
  pdc_list_configs_resp_msg_v01 qmi_response;
  qcril_request_resp_params_type resp;
  qmi_client_error_type qmi_error;
  RIL_Errno result = RIL_E_GENERIC_FAILURE;
  char payload;

  QCRIL_LOG_FUNC_ENTRY();

  memset( &qmi_request, 0, sizeof(qmi_request) );
  qmi_request.config_type = PDC_CONFIG_TYPE_MODEM_SW_V01;
  qmi_request.config_type_valid = 1;
  qmi_error = qmi_client_send_msg_sync( qcril_qmi_client_get_user_handle( QCRIL_QMI_CLIENT_PDC ),
                                        QMI_PDC_LIST_CONFIGS_REQ_V01,
                                        &qmi_request,
                                        sizeof( qmi_request ),
                                        &qmi_response,
                                        sizeof( qmi_response ),
                                        QCRIL_QMI_SYNC_REQ_DEF_TIMEOUT
                                        );
  result = qcril_qmi_util_convert_qmi_response_codes_to_ril_result( qmi_error, &qmi_response.resp );

  // on failure, send an unsol message to indicate error
  // on success, do nothing here, the list indication handler
  // will handle the remain things
  if ( result == RIL_E_GENERIC_FAILURE )
  {
    payload = QCRIL_PDC_GENERIC_FAIL;
    qcril_hook_unsol_response( QCRIL_DEFAULT_INSTANCE_ID, QCRIL_EVT_HOOK_UNSOL_PDC_CLEAR_CONFIGS,
                                (void *)(&payload), sizeof(payload));
  }

  QCRIL_LOG_FUNC_RETURN();
}

//===========================================================================
//QCRIL_EVT_HOOK_SET_MODEM_TEST_MODE
// param: config_id
// TODO: need to block the call if the previous set is not complete
//===========================================================================
void qcril_qmi_pdc_set_modem_test_mode
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr
)
{
  qcril_request_resp_params_type resp;
  char mbn_file_path[QCRIL_MBN_FILE_PATH_LEN];
  RIL_Errno result = RIL_E_SUCCESS;
  struct stat f_stat;

  QCRIL_LOG_FUNC_ENTRY();

  do
  {
    if ( qmi_ril_get_process_instance_id() != QCRIL_DEFAULT_INSTANCE_ID )
    {
      QCRIL_LOG_ERROR("QMI PDC client is only available on primary subscrition");
      result = RIL_E_GENERIC_FAILURE;
      break;
    }
    if ( NULL ==  params_ptr->data || 0 == params_ptr->datalen
            || PDC_CONFIG_ID_SIZE_MAX_V01 < params_ptr->datalen )
    {
      QCRIL_LOG_ERROR("invalid parameter");
      result = RIL_E_GENERIC_FAILURE;
      break;
    }

    memset(&pdc_config_info, 0, sizeof(pdc_config_info_type));
    pdc_config_info.conf_fd = -1;
    memcpy(pdc_config_info.config_id, (void*)params_ptr->data, params_ptr->datalen);
    pdc_config_info.config_id_len = params_ptr->datalen;

    // XXX:
    // here we get the config_id, should firstly query this config
    // is already loaded?

    // load the configuration
    // assume that the file name is "$config_id.mbn"
    strlcpy(mbn_file_path, mbn_file_dir, QCRIL_MBN_FILE_PATH_LEN);
    strncat(mbn_file_path, pdc_config_info.config_id,
                    QCRIL_MBN_FILE_PATH_LEN - strlen(mbn_file_dir));
    strncat(mbn_file_path, ".mbn",
            QCRIL_MBN_FILE_PATH_LEN - strlen(mbn_file_path));

    // fill the file descripter and the config size
    pdc_config_info.conf_fd = open( mbn_file_path, O_RDONLY );
    if ( pdc_config_info.conf_fd == -1 )
    {
      QCRIL_LOG_ERROR("Failed to open file %s: %s", mbn_file_path, strerror(errno));
      result = RIL_E_GENERIC_FAILURE;
      break;
    }
    if ( fstat( pdc_config_info.conf_fd, &f_stat ) == -1 )
    {
      QCRIL_LOG_ERROR("Failed to fstat file: %s", strerror(errno));
      close(pdc_config_info.conf_fd);
      result = RIL_E_GENERIC_FAILURE;
      break;
    }
    pdc_config_info.conf_size = f_stat.st_size;

    // kick load config start
    qcril_event_queue( QCRIL_DEFAULT_INSTANCE_ID,
                    QCRIL_DEFAULT_MODEM_ID,
                    QCRIL_DATA_ON_STACK,
                    QCRIL_EVT_QMI_RIL_PDC_LOAD_CONFIGURATION,
                    NULL,
                    QMI_RIL_ZERO,
                    (RIL_Token) QCRIL_TOKEN_ID_INTERNAL );

  } while (0);

  qcril_default_request_resp_params( QCRIL_DEFAULT_INSTANCE_ID,
                                     params_ptr->t,
                                     params_ptr->event_id,
                                     result,
                                     &resp );
  qcril_send_request_response( &resp );
  QCRIL_LOG_FUNC_RETURN();
}

//===========================================================================
//QCRIL_EVT_HOOK_QUERY_MODEM_TEST_MODE
//===========================================================================
void qcril_qmi_pdc_query_modem_test_mode
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr
)
{
  pdc_get_selected_config_req_msg_v01 qmi_request;
  pdc_get_selected_config_resp_msg_v01 qmi_response;
  qcril_request_resp_params_type resp;
  qmi_client_error_type qmi_error;
  RIL_Errno result = RIL_E_GENERIC_FAILURE;

  QCRIL_LOG_FUNC_ENTRY();

  if ( qmi_ril_get_process_instance_id() == QCRIL_DEFAULT_INSTANCE_ID )
  {
    memset( &qmi_request, 0, sizeof(qmi_request) );
    qmi_request.config_type = PDC_CONFIG_TYPE_MODEM_SW_V01;
    qmi_error = qmi_client_send_msg_sync( qcril_qmi_client_get_user_handle( QCRIL_QMI_CLIENT_PDC ),
                                        QMI_PDC_GET_SELECTED_CONFIG_REQ_V01,
                                        &qmi_request,
                                        sizeof( qmi_request ),
                                        &qmi_response,
                                        sizeof( qmi_response ),
                                        QCRIL_QMI_SYNC_REQ_DEF_TIMEOUT
                                        );
    result = qcril_qmi_util_convert_qmi_response_codes_to_ril_result( qmi_error, &qmi_response.resp );
  }
  else
  {
    QCRIL_LOG_ERROR("QMI PDC client is only available on primary subscrition");
  }

  if ( result == RIL_E_GENERIC_FAILURE )
  {
    qcril_default_request_resp_params( QCRIL_DEFAULT_INSTANCE_ID,
                                    params_ptr->t,
                                    params_ptr->event_id,
                                    result,
                                    &resp );
    qcril_send_request_response( &resp );
  }

  QCRIL_LOG_FUNC_RETURN();
}

//===========================================================================
// QCRIL_EVT_HOOK_CLEANUP_LOADED_CONFIGS
//===========================================================================
void qcril_qmi_pdc_cleanup_loaded_configs
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr
)
{
  pdc_deactivate_config_req_msg_v01 qmi_request;
  pdc_deactivate_config_resp_msg_v01 qmi_response;
  qmi_client_error_type qmi_error;
  qcril_request_resp_params_type resp;
  RIL_Errno result = RIL_E_GENERIC_FAILURE;

  QCRIL_LOG_FUNC_ENTRY();

  if ( qmi_ril_get_process_instance_id() != QCRIL_DEFAULT_INSTANCE_ID )
  {
    QCRIL_LOG_ERROR("QMI PDC client is only available on primary subscrition");
  }
  else
  {
    memset( &qmi_request, 0, sizeof(qmi_request) );
    qmi_request.config_type = PDC_CONFIG_TYPE_MODEM_SW_V01;
    qmi_error = qmi_client_send_msg_sync( qcril_qmi_client_get_user_handle( QCRIL_QMI_CLIENT_PDC ),
                                        QMI_PDC_DEACTIVATE_CONFIG_REQ_V01,
                                        &qmi_request,
                                        sizeof( qmi_request ),
                                        &qmi_response,
                                        sizeof( qmi_response ),
                                        QCRIL_QMI_SYNC_REQ_DEF_TIMEOUT
                                        );
    result = qcril_qmi_util_convert_qmi_response_codes_to_ril_result( qmi_error, &qmi_response.resp );
  }


  qcril_default_request_resp_params( QCRIL_DEFAULT_INSTANCE_ID,
                                     params_ptr->t,
                                     params_ptr->event_id,
                                     result,
                                     &resp );
  qcril_send_request_response( &resp );

  QCRIL_LOG_FUNC_RETURN();
}

/*=========================================================================
  FUNCTION:  qcril_qmi_pdc_unsol_ind_cb

===========================================================================*/
/*!
    @brief
    Callback for QMI PDC indications

    @return
    None.
*/
/*=========================================================================*/
void qcril_qmi_pdc_unsol_ind_cb
(
  qmi_client_type       user_handle,
  unsigned long         msg_id,
  unsigned char         *ind_buf,
  int                   ind_buf_len,
  void                  *ind_cb_data
)
{
  uint32_t decoded_payload_len = 0;
  qmi_client_error_type qmi_err;
  void* decoded_payload = NULL;

  QCRIL_LOG_FUNC_ENTRY();

  QCRIL_NOTUSED( user_handle );
  QCRIL_NOTUSED( ind_cb_data );

  qmi_err = qmi_idl_get_message_c_struct_len(qcril_qmi_client_get_service_object(QCRIL_QMI_CLIENT_PDC),
                                                QMI_IDL_INDICATION,
                                                msg_id,
                                                &decoded_payload_len);
  if ( qmi_err == QMI_NO_ERR )
  {
    decoded_payload = qcril_malloc( decoded_payload_len );
    if ( NULL != decoded_payload )
    {
      qmi_err = qmi_client_message_decode(qcril_qmi_client_get_user_handle(QCRIL_QMI_CLIENT_PDC),
                                          QMI_IDL_INDICATION,
                                          msg_id,
                                          ind_buf,
                                          ind_buf_len,
                                          decoded_payload,
                                          (int)decoded_payload_len);

      if (QMI_NO_ERR == qmi_err)
      {
        switch(msg_id)
        {
          case QMI_PDC_LOAD_CONFIG_IND_V01:
            // TODO: add load configure indication process
            qcril_qmi_pdc_load_config_ind_hdlr(decoded_payload, decoded_payload_len);
            break;

          case QMI_PDC_SET_SELECTED_CONFIG_IND_V01:
            qcril_qmi_pdc_select_config_ind_hdlr(decoded_payload, decoded_payload_len);
            break;

          case QMI_PDC_ACTIVATE_CONFIG_IND_V01:
            qcril_qmi_pdc_activate_config_ind_hdlr(decoded_payload, decoded_payload_len);
            break;

          case QMI_PDC_GET_SELECTED_CONFIG_IND_V01:
            qcril_qmi_pdc_get_selected_config_ind_hdlr(decoded_payload, decoded_payload_len);
            break;

          case QMI_PDC_DEACTIVATE_CONFIG_IND_V01:
            qcril_qmi_pdc_deactivate_config_ind_hdlr(decoded_payload, decoded_payload_len);
            break;

          case QMI_PDC_LIST_CONFIGS_IND_V01:
            qcril_qmi_pdc_list_configs_ind_hdlr(decoded_payload, decoded_payload_len);
            break;

          case QMI_PDC_DELETE_CONFIG_IND_V01:
            qcril_qmi_pdc_delete_config_ind_hdlr(decoded_payload, decoded_payload_len);
            break;

          default:
            QCRIL_LOG_INFO("Unsupported QMI PDC indication %x hex", msg_id);
            break;
        }
      }
      else
      {
        QCRIL_LOG_INFO("Indication decode failed for msg %d of svc %d with error %d", msg_id, QCRIL_QMI_CLIENT_PDC, qmi_err );
      }

      qcril_free(decoded_payload);
    }
  }

  QCRIL_LOG_FUNC_RETURN_WITH_RET( (int)qmi_err );
}

/*===========================================================================

  FUNCTION:  qcril_qmi_pdc_init

===========================================================================*/
/*!
    @brief
    Initialize the PDC subsystem of the RIL.

    @return
    None.
*/
 /*=========================================================================*/

qmi_client_error_type qcril_qmi_pdc_init
(
  void
)
{
  qmi_client_error_type qmi_err = QMI_NO_ERR;
  char property_name[ 40 ];

  QCRIL_LOG_FUNC_ENTRY();

  QCRIL_SNPRINTF( property_name, sizeof( property_name ), "%s", QCRIL_MODEM_MBN_FILE_PATH );
  property_get( property_name, mbn_file_dir, QCRIL_MODEM_MBN_DEFAULT_PATH );
  /* TODO: check if the data path is valid */

  pthread_mutexattr_init( &pdc_config_info.pdc_config_mutex_attr );
  pthread_mutex_init( &pdc_config_info.pdc_config_mutex, &pdc_config_info.pdc_config_mutex_attr );

  QCRIL_LOG_FUNC_RETURN_WITH_RET(qmi_err);

  return (qmi_err);
}
