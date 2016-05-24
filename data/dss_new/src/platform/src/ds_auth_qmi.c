/******************************************************************************
  @file    ds_auth_qmi.c
  @brief   

  DESCRIPTION
  DSS API for exposing modem AKA v1 and v2 implementation to QMI clients.

  INITIALIZATION AND SEQUENCING REQUIREMENTS
  ds_auth_init() must be called to initialize API.  Must be called once per
  process.

  ---------------------------------------------------------------------------
  Copyright (C) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------
****************************************************************************/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datacommon/dsprofile/rel/08.02.02/src/ds_profile_3gpp_qmi.c#2 $
  $DateTime: 2010/09/11 18:23:35 $
  $Author: smudired $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
11/08/10   mct     DS Api for AKAv2 algorithm.
===========================================================================*/
#include "comdef.h"
#include "msg.h"
#include "err.h"
#include "amssassert.h"

#include "dserrno.h"
#include "ds_Utils_DebugMsg.h"
#include "ds_auth_api.h"
#include "ds_auth_platform.h"
#include "qmi.h"
#include "qmi_eap_srvc.h"
#include "queue.h"
#include "ps_system_heap.h"

/*Global Definitions*/
static q_type                  ds_auth_ind_data_q;
static qmi_client_handle_type  qmi_auth_handle;

typedef struct {
 q_link_type                 link;
 ds_auth_aka_handle_type     aka_handle;
 ds_auth_aka_callback_type   aka_algo_cback;
 void                       *user_data;
} ds_auth_aka_ind_data_type;

/*===========================================================================
FUNCTION DS_AUTH_GET_AKA_IND_DATA

DESCRIPTION
  Helper function to retrieve the queued user indication data (ind callback,
  aka handle, and user data.
PARAMETERS 
  handle - The aka handle given in the QMI ind to match to the queued data.
DEPENDENCIES 
  
RETURN VALUE 
  A pointer to the queued user data.
SIDE EFFECTS 
  
===========================================================================*/
ds_auth_aka_ind_data_type * ds_auth_get_aka_ind_data
(
  ds_auth_aka_handle_type handle
)
{
  ds_auth_aka_ind_data_type *link_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*----------------------------------------------------------------------
    Find the queue item which contains the aka indication data, callback,
    etc and return it. If the item is not found return NULL.
  -----------------------------------------------------------------------*/
  link_ptr = (ds_auth_aka_ind_data_type *) q_check(&ds_auth_ind_data_q);
  while(link_ptr != NULL)
  {
    if(link_ptr->aka_handle == handle)
    {
      q_delete(&ds_auth_ind_data_q, &(link_ptr->link));
      return link_ptr;
    }

    link_ptr = (ds_auth_aka_ind_data_type *)
                 q_next(&ds_auth_ind_data_q, &(link_ptr->link));
  }
  return NULL;
} /* ds_auth_get_aka_ind_data() */

/*===========================================================================
FUNCTION DS_AUTH_QMI_AUTH_IND_HDLR

DESCRIPTION
  The QMI auth service indication handler. This processes incoming indications
  and calls the appropriate callbacks.

PARAMETERS 
  
DEPENDENCIES 
  
RETURN VALUE 
  None
SIDE EFFECTS 
  
===========================================================================*/
void ds_auth_qmi_auth_ind_hdlr
( 
  int                           user_handle,
  qmi_service_id_type           service_id,
  void                          *user_data,
  qmi_eap_indication_id_type    ind_id,
  qmi_eap_indication_data_type  *ind_data
)
{
  ds_auth_aka_ind_data_type *ind_user_data_ptr = NULL;
  (void) user_data;
  if( user_handle != qmi_auth_handle )
  {
    LOG_MSG_ERROR("ds_auth_qmi_auth_ind_hdlr: user_handles %d don't match %d! ", user_handle, qmi_auth_handle, 0);
    ASSERT(0);
    return;
  }

  if( service_id != QMI_EAP_SERVICE)
  {
    LOG_MSG_ERROR("ds_auth_qmi_auth_ind_hdlr: invalid service id %d! ", service_id, 0, 0);
    ASSERT(0);
    return;
  }

  ASSERT(ind_data);

  switch (ind_id)
  {
    case QMI_EAP_AKA_RESULT_IND_MSG:
      LOG_MSG_INFO2("Receive AKA result ind",0,0,0);
      ind_user_data_ptr = ds_auth_get_aka_ind_data(ind_data->aka_result.aka_handle);
      if(NULL == ind_user_data_ptr ||
         ind_data->aka_result.status != QMI_EAP_INITIATE_AKA_ALGORITHM_SUCCESS)
      {
        LOG_MSG_ERROR("ds_auth_qmi_auth_ind_hdlr: AKA ind failure!", 0, 0, 0);
        if (ind_user_data_ptr != NULL)
        {
          ind_user_data_ptr->aka_algo_cback(ind_data->aka_result.aka_handle,
                                            ind_data->aka_result.status,
                                            NULL,
                                            0,
                                            NULL,
                                            0, 
                                            ind_user_data_ptr->user_data);
        }
      }
      else
      {
        /* Call the user callback. */
        ind_user_data_ptr->aka_algo_cback(ind_data->aka_result.aka_handle,
                                          ind_data->aka_result.status,
                                          ind_data->aka_result.v1_or_v2_auth_params.digest,
                                          ind_data->aka_result.v1_or_v2_auth_params.digest_len,
                                          ind_data->aka_result.v1_or_v2_auth_params.aka_data,
                                          ind_data->aka_result.v1_or_v2_auth_params.aka_data_len,
                                          ind_user_data_ptr->user_data);
      }

      if(ind_user_data_ptr)
      {
        ps_system_heap_mem_free((void **)&ind_user_data_ptr);
      }
      break;

    default:
      LOG_MSG_ERROR("ds_auth_qmi_auth_ind_hdlr: Invalid ind type $d!", ind_id, 0, 0);
      return;
  }
  return;
} /* ds_auth_qmi_auth_ind_hdlr() */

/*===========================================================================
 FUNCTION:      DS_AUTH_PLATFORM_RUN_AKA_ALGO

 @brief        Runs AKAv1 or AKAv2 for the client over QMI. The result will
               be a handle that should be used to match the asynchronous 
               indication.
 
 @param[in]  aka_ver     AKA handle
 @param[in]  rand        RAND value
 @param[in]  rand_len    RAND length
 @param[in]  autn        AUTN value
 @param[in]  autn_len    AUTN length
 @param[in]  aka_algo_cback  Client callback that is called 
                             when the results are available
 @param[in]  user_data   User data, passed to the client callback function

 @dependencies None
 
 @return      AKA handle or Failure
 @retval <0   Failure, check ds_errno for more information.
 @retval >=0  AKA handle
 
 @sideeffect  None
===========================================================================*/
ds_auth_aka_handle_type ds_auth_platform_run_aka_algo
(
  ds_auth_aka_ver_enum_type   aka_ver,
  uint8*                      rand,
  uint8                       rand_len,
  uint8*                      autn,
  uint8                       autn_len,
  ds_auth_aka_callback_type   aka_algo_cback,
  void                       *user_data,
  sint15                     *ds_errno
)
{
  qmi_eap_initiate_aka_algorithm_type qmi_auth_data;
  unsigned long                       handle = -1;
  ds_auth_aka_ind_data_type          *ind_data_ptr = NULL;
  int                                 qmi_err_code = 0;

  /* QMI length is only 128 vs AKA length which is 255. Return an error
   * if the user passes a larger value than QMI can handle */
  if(rand_len > QMI_EAP_MAX_STR_LEN || autn_len > QMI_EAP_MAX_STR_LEN)
  {
    LOG_MSG_ERROR("ds_auth_platform_run_aka_algo: rand_len %d or autn_len %d too large for QMI!",
                  rand_len, autn_len, 0);
    *ds_errno = DS_EFAULT;
    return -1;
  }
  qmi_auth_data.aka_version = (ds_auth_aka_ver_enum_type) aka_ver;

  if(aka_ver != DS_AUTH_AKA_V1 && aka_ver != DS_AUTH_AKA_V2)
  {
    *ds_errno = DS_EOPNOTSUPP;
    LOG_MSG_ERROR("%s: AKA version %d not supported", __FUNCTION__, aka_ver, 0);
    return -1;
  }

  qmi_auth_data.param_mask  = QMI_EAP_AKA_V1_OR_V2_AUTH_PARAMS;
  
  qmi_auth_data.v1_or_v2_auth_params.rand_len = rand_len;
  qmi_auth_data.v1_or_v2_auth_params.auth_len = autn_len;
   
  memcpy(qmi_auth_data.v1_or_v2_auth_params.auth,
         autn,
         qmi_auth_data.v1_or_v2_auth_params.auth_len);

  memcpy(qmi_auth_data.v1_or_v2_auth_params.rand,
         rand,
         qmi_auth_data.v1_or_v2_auth_params.rand_len);

  /* Allocate memory for caching state first.  In case of failure QMI call is */
  /*   not made */
  ind_data_ptr = ps_system_heap_mem_alloc(sizeof(ds_auth_aka_ind_data_type));

  if(ind_data_ptr == NULL)
  {
    LOG_MSG_ERROR("ds_auth_platform_run_aka_algo: No mem to alloc ind data!", 
                  0, 0, 0);
    *ds_errno = DS_ENOMEM;
    return -1;
  }

  if(qmi_eap_auth_initiate_aka_algorithm(qmi_auth_handle,
                                         &qmi_auth_data,
                                         &handle,
                                         &qmi_err_code) < 0)
  {
    LOG_MSG_ERROR("ds_auth_platform_run_aka_algo: Invalid param!", 0, 0, 0);
    *ds_errno = DS_EFAULT;
    ps_system_heap_mem_free((void **)&ind_data_ptr);
    return -1;
  }

  /* Save the user's data while waiting for the ind callback. */
  ind_data_ptr->aka_handle     = handle;
  ind_data_ptr->aka_algo_cback = aka_algo_cback;
  ind_data_ptr->user_data      = user_data;

  q_put(&ds_auth_ind_data_q, &(ind_data_ptr->link));

  return (ds_auth_aka_handle_type) handle;
} /* ds_auth_platform_run_aka_algo() */

/*===========================================================================
FUNCTION DS_AUTH_PLATFORM_INIT

DESCRIPTION
  This function is called on the library init from libdssock. It initializes
  data structures and sets up a connection to QMI EAP service.
 
PARAMETERS 
  None

DEPENDENCIES
  Expects qmi_init() to be called from libdssock
  
RETURN VALUE
  None
 
SIDE EFFECTS
  
===========================================================================*/
void ds_auth_platform_init()
{
  int qmi_err;
  int ret;

  LOG_MSG_INFO2("ds_auth_init()", 0,0,0);

  (void)q_init(&ds_auth_ind_data_q);

  if ((ret = qmi_connection_init(QMI_PORT_RMNET_1, &qmi_err)) < 0) 
  {
    LOG_MSG_ERROR("ds_auth_platform_init(): QMI connection init failed with return %d qmi error %d ",
                  ret, qmi_err, 0);
    return;
  }

  if ((qmi_auth_handle = qmi_eap_srvc_init_client(QMI_PORT_RMNET_1, 
                                                  ds_auth_qmi_auth_ind_hdlr,
                                                  NULL, 
                                                  &qmi_err)) < 0) 
  {
    LOG_MSG_ERROR("ds_auth_platform_init(): Auth client init failed with ret: %d qmi error: %d ",
                  qmi_auth_handle, qmi_err, 0);
    return;
  }

  return;
} /* ds_auth_platform_init() */
