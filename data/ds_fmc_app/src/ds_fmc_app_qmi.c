/******************************************************************************

                          D S _ F M C _ A P P _ Q M I . C

******************************************************************************/

/******************************************************************************

  @file    ds_fmc_app_qmi.c
  @brief   DS_FMC_APP QMI Driver Interface

  DESCRIPTION
  Implementation of DS_FMC_APP QMI Driver interface.

******************************************************************************/
/*===========================================================================

  Copyright (c) 2010 - 2012 Qualcomm Technologies, Inc. All Rights Reserved

  Qualcomm Technologies Proprietary

  Export of this technology or software is regulated by the U.S. Government.
  Diversion contrary to U.S. law prohibited.

  All ideas, data and information contained in or disclosed by
  this document are confidential and proprietary information of
  Qualcomm Technologies, Inc. and all rights therein are expressly reserved.
  By accepting this material the recipient agrees that this material
  and the information contained therein are held in confidence and in
  trust and will not be used, copied, reproduced in whole or in part,
  nor its contents revealed in any manner to others without the express
  written permission of Qualcomm Technologies, Inc.

===========================================================================*/


/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id:$

when       who        what, where, why
--------   ---        -------------------------------------------------------
03/09/12   op         Added support for QMI WDS system status indication
02/13/10   scb        Initial version

******************************************************************************/

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ds_fmc_app_exec.h"
#include "ds_fmc_app_util.h"
#include "ds_fmc_app_qmi.h"
#include "ds_fmc_app_data_ext_if.h"

#define INETINADDR_LEN    4
#define INETIN6ADDR_LEN  16

/*===========================================================================
                     LOCAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

LOCAL struct ds_fmc_app_qmi_cfg_s ds_fmc_app_qmi_cfg;

/*qmi message library handle*/
int qmi_handle = QMI_INVALID_CLIENT_HANDLE;

/* QMI WDS version information */
#define IS_QMI_WDS_VER_WITH_DATA_CAP( info, major, minor )                             \
        ((info.major_ver*1000 + info.minor_ver) < (major*1000 + minor))

/* Data system status indication support was added in WDS version 1.14 */
#define QMI_WDS_DATA_SYS_STATUS_VERSION_MAJ (1)
#define QMI_WDS_DATA_SYS_STATUS_VERSION_MIN (14)
/*===========================================================================
                            LOCAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  ds_fmc_app_qmi_cleanup
===========================================================================*/
/*!
@brief
  Performs cleanup of QMI client connections.  Invoked at process
  termination.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
ds_fmc_app_qmi_cleanup
(
  void
)
{
  int qmi_err;

  if (qmi_handle < 0)
  {
    ds_fmc_app_log_err("QMI message library was never initialized. "
                                                  "invalid qmi handle");
    return;
  }
  /* Reset the device ID in the qmi_cfg control block*/
  memset(&ds_fmc_app_qmi_cfg.dev_id, 0, QMI_PORT_DEV_ID_LEN);

  /* Release client handles */
  qmi_wds_srvc_release_client(
    ds_fmc_app_qmi_cfg.wds_srvc.wds_info.clnt_hdl, &qmi_err );

  ds_fmc_app_log_high("Releasing the WDS qmi_client_handle 0x%08x \n",
                  ds_fmc_app_qmi_cfg.wds_srvc.wds_info.clnt_hdl);

  /* Release QMI library connection */
  qmi_release(qmi_handle);
}

/*===========================================================================
  FUNCTION  ds_fmc_app_qmi_reset_cfg_info
===========================================================================*/
/*!
@brief
 Helper function to reset state information of the
 ds_fmc_app_qmi_cfg structure.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
ds_fmc_app_qmi_reset_cfg_info
(
  void
)
{
  DS_FMC_APP_LOG_FUNC_ENTRY;
  /* Reset the device ID in the qmi_cfg control block*/
  memset(&ds_fmc_app_qmi_cfg.dev_id, 0, QMI_PORT_DEV_ID_LEN);
  /* Reset interface info */
  ds_fmc_app_qmi_cfg.wds_srvc.wds_info.clnt_hdl = -1;
  ds_fmc_app_qmi_cfg.fmc_bearer_status = DS_FMC_APP_FMC_BEARER_DISABLED;
  DS_FMC_APP_LOG_FUNC_EXIT;
  return;
}

/*===========================================================================
  FUNCTION  ds_fmc_app_qmi_wds_ind
===========================================================================*/
/*!
@brief
 Processes an incoming QMI WDS Indication. It posts a command to do the
 required processing in the Command Thread context.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
ds_fmc_app_qmi_wds_ind
(
  int                            user_handle,
  qmi_service_id_type            service_id,
  void                         * user_data,
  qmi_wds_indication_id_type     ind_id,
  qmi_wds_indication_data_type * ind_data
)
{
  ds_fmc_app_exec_cmd_t                     *cmd = NULL;
  ds_fmc_app_sm_events_t                     type;
  qmi_wds_event_report_type                 *ev_report = NULL;
  qmi_wds_data_sys_status_network_info_type  network_info;
  DS_FMC_APP_LOG_FUNC_ENTRY;
  (void)user_data;

  /* Verify service id before proceeding */
  if( user_handle < 0 || QMI_WDS_SERVICE != service_id )
  {
    ds_fmc_app_log_err("ds_fmc_app_qmi_wds_ind: Invalid handle %d"
                       " or received non-WDS indication %d, ignoring\n",
                        user_handle, service_id);

    DS_FMC_APP_LOG_FUNC_EXIT;
    return;
  }

  if( ind_data == NULL )
  {
    ds_fmc_app_log_err("ds_fmc_app_qmi_wds_ind: Invalid ind_data %d\n",
                        ind_data);
    DS_FMC_APP_LOG_FUNC_EXIT;
    return;
  }

  ds_fmc_app_log_high("ds_fmc_app_qmi_wds_ind: ind_id %d\n", ind_id);

  switch(ind_id)
  {
    case QMI_WDS_SRVC_EVENT_REPORT_IND_MSG:

      ev_report = (qmi_wds_event_report_type*)ind_data;

      ds_fmc_app_log_high("ds_fmc_app_qmi_wds_ind: ev_report->mask %d\n",
                          ev_report->event_mask);

      if ( ( ev_report->event_mask & QMI_WDS_EVENT_DATA_SYS_STATUS_IND ) &&
           ( NULL != ev_report->data_sys_status.network_info )
         )
      {
        network_info =
         (ev_report->data_sys_status.network_info[QMI_WDS_DATA_NETWORK_TYPE_3GPP2] );

        ds_fmc_app_log_high("ds_fmc_app_qmi_wds_ind: system_status "
                            "pref_network type %ld data_sys_status "
                            "network_info - network %ld network_info "
                            "- rat_mask 0x%x\n ",
                            ev_report->data_sys_status.pref_network,
                            network_info.network,
                            network_info.rat_mask );

        if( ( QMI_WDS_DATA_NETWORK_TYPE_3GPP2 == ev_report->data_sys_status.pref_network ) &&
            ( QMI_WDS_DATA_NETWORK_TYPE_3GPP2 == network_info.network ) &&
            ( network_info.rat_mask.cdma_rat_mask & CDMA_FMC )
          )
        {
          type = DS_FMC_APP_BEARER_UP_EV;
        }
        else
        {
          type = DS_FMC_APP_BEARER_DOWN_EV;
        }
      }
      else if ( ev_report->event_mask & QMI_WDS_EVENT_DATA_CAPABILITIES_IND )
      {
        ds_fmc_app_log_high("ds_fmc_app_qmi_wds_ind: data_capabilities"
                            " %ld\n", ev_report->data_capabilities[0]);

        if( ev_report->data_capabilities[0] &
            QMI_WDS_EVENT_REPORT_DATA_CAPABILITY_0_FMC )
        {
          type = DS_FMC_APP_BEARER_UP_EV;
        }
        else
        {
          type = DS_FMC_APP_BEARER_DOWN_EV;
        }
      }
      else
      {
        ds_fmc_app_log_err("ds_fmc_app_qmi_wds_ind: "
                           "Invalid QMI WDS event report parameters received");
        break;
      }

      /* Allocate a command object */
      cmd = ds_fmc_app_exec_get_cmd();

      DS_FMC_APP_ASSERT(cmd);
      /* Set the connection open status of the external datapath entity
         in the command structure */

      cmd->data.ext_data_path_conn_status =
        ds_fmc_app_data_ext_conn_open_status(DS_FMC_APP_DATA_CONN_ID_0);

      cmd->data.type = type;

      /* Post command for processing in the command thread context */
      if( DS_FMC_APP_SUCCESS != ds_fmc_app_exec_put_cmd( cmd ) )
      {
         DS_FMC_APP_STOP("ds_fmc_app_qmi_wds_ind: failed to put commmand\n");
      }
      break;

    default:
      break;
  }

  DS_FMC_APP_LOG_FUNC_EXIT;
  return;
}

/*===========================================================================
  FUNCTION  ds_fmc_app_qmi_service_init
===========================================================================*/
/*!
@brief
  Initializes the QMI WDS service.

@return
  int - DS_FMC_APP_SUCCESS on successful operation, DS_FMC_APP_FAILURE otherwise.

@note

  - Dependencies
    - ds_fmc_app_qmi_driver_init() must have been invoked during powerup.

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
ds_fmc_app_qmi_service_init
(
  char *dev_id
)
{
  int qmi_ret, qmi_err;
  int wds_clnt_id;
  qmi_wds_event_report_params_type  event_params;
  qmi_service_version_info          qmi_wds_version;

  DS_FMC_APP_LOG_FUNC_ENTRY;

  /* Initialize WDS service client */
  if ((wds_clnt_id =
       qmi_wds_srvc_init_client( dev_id,
                                 ds_fmc_app_qmi_wds_ind,
                                 (void *)0,
                                 &qmi_err ) ) < 0)
  {
    ds_fmc_app_log_err("ds_fmc_app_qmi_service_init: "
    "qmi_wds_srvc_init_client failed for qmi_cid %s with error %ld,\n",
                   dev_id, (long int)qmi_err);
    goto error;
  }

  /* Save the device ID and WDS client ID in internal state */
  memset(&ds_fmc_app_qmi_cfg.dev_id, 0, QMI_PORT_DEV_ID_LEN);
  memcpy(&ds_fmc_app_qmi_cfg.dev_id, dev_id, strlen(dev_id));

  ds_fmc_app_qmi_cfg.wds_srvc.wds_info.clnt_hdl = wds_clnt_id;

  ds_fmc_app_log_high("ds_fmc_app_qmi_service_init: The wds client id"
                      " 0x%08x \n",wds_clnt_id);

  /* Query QMI WDS version */
  memset( &qmi_wds_version, 0x0, sizeof(qmi_wds_version) );
  if( QMI_NO_ERR !=
      (qmi_ret = qmi_service_get_version( dev_id,
                                          QMI_WDS_SERVICE,
                                          &qmi_wds_version,
                                          &qmi_err )) )
  {
    ds_fmc_app_log_err( "ds_fmc_app_qmi_service_init: Failed to query "
                          "WDS version rc[%d] qmi_err[%d]",
                          qmi_ret, qmi_err );
    goto error;
  }
  else
  {
    ds_fmc_app_log_high( "ds_fmc_app_qmi_service_init: QMI WDS verions "
                          "reported [%d.%d]",
                          qmi_wds_version.major_ver,
                          qmi_wds_version.minor_ver );
  }

  memset(&event_params, 0, sizeof(qmi_wds_event_report_params_type));
  if( IS_QMI_WDS_VER_WITH_DATA_CAP( qmi_wds_version,
                          QMI_WDS_DATA_SYS_STATUS_VERSION_MAJ, QMI_WDS_DATA_SYS_STATUS_VERSION_MIN ) )
  {
    event_params.param_mask |= QMI_WDS_EVENT_DATA_CAPABILITIES_IND;
    event_params.report_data_capabilities = TRUE;
  }
  else
  {
    event_params.param_mask |= QMI_WDS_EVENT_DATA_SYS_STATUS_IND;
    event_params.report_data_sys_status = TRUE;
  }

  if(0 < qmi_wds_set_event_report(wds_clnt_id, &event_params, &qmi_err))
  {
    ds_fmc_app_log_high("ds_fmc_app_qmi_service_init: Error setting event"
                      "report param_mask 0x%x for wds_client_id 0x%08x \n",
                         event_params.param_mask, wds_clnt_id);
    goto error;
  }

  DS_FMC_APP_LOG_FUNC_EXIT;
  return DS_FMC_APP_SUCCESS;

error:
  DS_FMC_APP_LOG_FUNC_EXIT;
  return DS_FMC_APP_FAILURE;
}

/*===========================================================================
  FUNCTION  ds_fmc_app_qmi_driver_init
===========================================================================*/
/*!
@brief
 Initializes the QMI Driver for the WDS service utilization.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
ds_fmc_app_qmi_driver_init
(
  char *dev_id
)
{
  int qmi_err;

  DS_FMC_APP_LOG_FUNC_ENTRY;

  if(NULL == dev_id)
  {
    DS_FMC_APP_STOP("ds_fmc_app_qmi_driver_init: dev_id NULL! ");
    return;
  }

  qmi_handle = qmi_init( NULL, NULL );

  if (qmi_handle < 0)
  {
    DS_FMC_APP_STOP("ds_fmc_app_qmi_driver_init: qmi_init "
                     "failed for %s\n", dev_id);
    return;
  }

  ds_fmc_app_log_high( "ds_fmc_app_qmi_driver_init: QMI initing\n" );

  /* Initialize qmi connection for dev_id */
  if (qmi_connection_init(dev_id,
                       &qmi_err) < 0)
  {
    DS_FMC_APP_STOP("ds_fmc_app_qmi_driver_init: qmi_connection_init failed");
    return;
  }

  /* Perform QMI service init */
  if( DS_FMC_APP_SUCCESS != ds_fmc_app_qmi_service_init( dev_id ) )
  {
    DS_FMC_APP_STOP("ds_fmc_app_qmi_driver_init: failed on service init "
                   "for %s\n", dev_id);
    return;
  }

  ds_fmc_app_log_high( "ds_fmc_app_qmi_driver_init: QMI inited\n" );

  /* Register process exit cleanup handler */
  atexit(ds_fmc_app_qmi_cleanup);

  DS_FMC_APP_LOG_FUNC_EXIT;
  return;
}

/*===========================================================================
                            GLOBAL FUNCTION DEFINITIONS
===========================================================================*/


/*===========================================================================
  FUNCTION  ds_fmc_app_qmi_wds_set_tunnel_params
===========================================================================*/
/*!
@brief
  Sends a synchronous QMI message to the mdm with the required tunnel
  parameters as specified by ds_fmc_app_tunnel_mgr_ds.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - Upon failure, a failure code is returned.
*/
/*=========================================================================*/
int
ds_fmc_app_qmi_wds_set_tunnel_params
(
  ds_fmc_app_tunnel_mgr_ds_type *ds_fmc_app_tunnel_mgr_ds
)
{
  int qmi_err;

  qmi_wds_fmc_tunnel_params_type fmc_tunnel_params;

  struct sockaddr_in *addr = NULL;
  struct sockaddr_in6 *addr6 = NULL;

  DS_FMC_APP_LOG_FUNC_ENTRY;

  memset(&fmc_tunnel_params, 0, sizeof(qmi_wds_fmc_tunnel_params_type));

  ds_fmc_app_log_high("Calling qmi_wds_fmc_set_tunnel_params!");

  fmc_tunnel_params.tunnel_params.stream_id =
    ds_fmc_app_tunnel_mgr_ds->dest_strm_id;

  if(ds_fmc_app_tunnel_mgr_ds->nat_present)
  {
    fmc_tunnel_params.tunnel_params.nat_presence_indicator = NAT_PRESENT;
  }
  else
  {
    fmc_tunnel_params.tunnel_params.nat_presence_indicator = NAT_ABSENT;
  }

  if(ds_fmc_app_tunnel_mgr_ds->tunnel_dest_ip.ss_family == AF_INET)
  {
    addr = (struct sockaddr_in*)(&ds_fmc_app_tunnel_mgr_ds->tunnel_dest_ip);
    fmc_tunnel_params.param_mask =
      QMI_WDS_FMC_TUNNEL_PARAMS_IPV4_SOCKET_ADDR;
    fmc_tunnel_params.tunnel_params.port_id =
            (unsigned short)addr->sin_port;

    memcpy(&fmc_tunnel_params.v4_socket_addr, &addr->sin_addr.s_addr,
            INETINADDR_LEN);
  }
  else if(ds_fmc_app_tunnel_mgr_ds->tunnel_dest_ip.ss_family == AF_INET6)
  {
    addr6 = (struct sockaddr_in6*)
            (&ds_fmc_app_tunnel_mgr_ds->tunnel_dest_ip);
    fmc_tunnel_params.param_mask =
      QMI_WDS_FMC_TUNNEL_PARAMS_IPV6_SOCKET_ADDR;
    fmc_tunnel_params.tunnel_params.port_id =
            (unsigned short)addr6->sin6_port;
    memcpy(&fmc_tunnel_params.v6_socket_addr, &addr6->sin6_addr.s6_addr,
            INETIN6ADDR_LEN);
  }

  if( qmi_wds_fmc_set_tunnel_params(
        ds_fmc_app_qmi_cfg.wds_srvc.wds_info.clnt_hdl,
        &fmc_tunnel_params,
        &qmi_err
      ) < 0 )
  {
    ds_fmc_app_log_err("ds_fmc_app_qmi_wds_set_tunnel_params: "
                       "qmi_wds_fmc_set_tunnel_params() failed err %d\n",
                        qmi_err);

    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  ds_fmc_app_log_high("ds_fmc_app_qmi_wds_set_tunnel_params: "
                     "qmi_wds_fmc_set_tunnel_params() successful\n");

  DS_FMC_APP_LOG_FUNC_EXIT;
  return DS_FMC_APP_SUCCESS;

} /* ds_fmc_app_qmi_wds_set_tunnel_params */

/*===========================================================================
  FUNCTION  ds_fmc_app_qmi_wds_clear_tunnel_params
===========================================================================*/
/*!
@brief
  Sends a synchronous QMI message to the mdm to clear the FMC tunnel params.

@return
  DS_FMC_APP_SUCCESS on success, DS_FMC_APP_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - Upon failure, a failure code is returned.
*/
/*=========================================================================*/
int
ds_fmc_app_qmi_wds_clear_tunnel_params
(
  void
)
{
  int qmi_err;

  DS_FMC_APP_LOG_FUNC_ENTRY;

  ds_fmc_app_log_high("Calling ds_fmc_app_qmi_wds_clear_tunnel_params!");

  if( qmi_wds_fmc_clear_tunnel_params(
        ds_fmc_app_qmi_cfg.wds_srvc.wds_info.clnt_hdl,
        &qmi_err
      ) < 0 )
  {
    ds_fmc_app_log_err("ds_fmc_app_qmi_wds_clear_tunnel_params: "
                       "ds_fmc_app_wds_clear_tunnel_params() failed err %d\n",
                        qmi_err);
    DS_FMC_APP_LOG_FUNC_EXIT;
    return DS_FMC_APP_FAILURE;
  }

  DS_FMC_APP_LOG_FUNC_EXIT;
  return DS_FMC_APP_SUCCESS;
} /* ds_fmc_app_qmi_wds_clear_tunnel_params */

/*===========================================================================
  FUNCTION  ds_fmc_app_qmi_init
===========================================================================*/
/*!
@brief
 Main initialization routine of the QMI Interface module.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - Initializes the QMI Driver
*/
/*=========================================================================*/
void
ds_fmc_app_qmi_init ( char *dev_id)
{
  DS_FMC_APP_LOG_FUNC_ENTRY;

  if(dev_id == NULL)
  {
    DS_FMC_APP_STOP("ds_fmc_app_qmi_init: dev_id passed in is NULL;"
                    " stopping\n");
  }

  ds_fmc_app_log_high("ds_fmc_app_qmi_init: dev_id %s\n", dev_id);

  /* Initialize the DS_FMC_APP QMI configuration structure */
  ds_fmc_app_qmi_reset_cfg_info();

  /* Initialize the QMI Client Driver and start WDS clients for each
  ** interface.
  */
  ds_fmc_app_qmi_driver_init( dev_id );

  DS_FMC_APP_LOG_FUNC_EXIT;
  return;
}
