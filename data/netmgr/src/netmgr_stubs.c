/******************************************************************************

                        N E T M G R _ S T U B S . C

******************************************************************************/

/******************************************************************************

  @file    netmgr_stubs.C
  @brief   Network Manager test stubs 

  DESCRIPTION
  NetMgr test stub functions.

******************************************************************************/
/*===========================================================================

  Copyright (c) 2010-2012 Qualcomm Technologies, Inc. All Rights Reserved

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
02/10/10   ar         Initial version

******************************************************************************/

#ifdef NETMGR_OFFTARGET

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "qmi.h"
#include "qmi_wds_srvc.h"
#include "qmi_qos_srvc.h"
#include "qmi_qos_srvc_i.h"
#include "netmgr_util.h"
#include "netmgr_exec.h"
#include "netmgr_stubs.h"

int atexit(void (*function)(void))
{
  return 0;
}

/*-----------------------------------------------------------------------*/

int test_mtu = 0;

int test_ioctl(int fd, int flag, struct ifreq *ifr)
{
  switch (flag) {
  case SIOCSIFMTU:
    test_mtu = (int)ifr->ifr_data;
    break;
  default:
    break;
  }
  return 0;
}

/*-----------------------------------------------------------------------*/

int ifc_reset_connections(const char *ifname, const int reset_mask)
{
}

int ifc_del_address(const char *name, const char * address, int prefixlen)
{
}

/*-----------------------------------------------------------------------*/

int qmi_wds_client = 0;
int qmi_qos_client = 0;

int
qmi_init 
(
  qmi_sys_event_rx_hdlr   event_rx_hdlr,
  void                    *event_user_data
)
{
  NETMGR_LOG_FUNC_ENTRY;
  NETMGR_LOG_FUNC_EXIT;
  return QMI_NO_ERR;
}

int
qmi_release 
(
  int init_handle
)
{
  NETMGR_LOG_FUNC_ENTRY;
  NETMGR_LOG_FUNC_EXIT;
  return QMI_NO_ERR;
}


int 
qmi_dev_connection_init
(
  const char  *dev_id,
  int         *qmi_err_code
)
{
  NETMGR_LOG_FUNC_ENTRY;
  NETMGR_LOG_FUNC_EXIT;
  return QMI_NO_ERR;
}

int
qmi_set_port_data_format
(
  const char                            *dev_id,
  qmi_data_format_qos_hdr_state_type    qos_hdr_state,
  qmi_link_layer_protocol_type          *link_protocol,
  int                                   *qmi_err_code
)
{
  NETMGR_LOG_FUNC_ENTRY;
  NETMGR_LOG_FUNC_EXIT;
  return QMI_NO_ERR;
}


qmi_client_handle_type
qmi_wds_srvc_init_client
(
  const char                    *dev_id,
  qmi_wds_indication_hdlr_type  user_ind_msg_hdlr,
  void                          *user_ind_msg_hdlr_user_data,
  int                           *qmi_err_code
)
{
  NETMGR_LOG_FUNC_ENTRY;
  NETMGR_LOG_FUNC_EXIT;
  return qmi_wds_client++;;
}

int 
qmi_wds_srvc_release_client
(
  int      user_handle,
  int      *qmi_err_code
)
{
  NETMGR_LOG_FUNC_ENTRY;
  NETMGR_LOG_FUNC_EXIT;
  return QMI_NO_ERR;
}


int
qmi_wds_query_profile
(
  int                         user_handle,
  qmi_wds_profile_id_type     *profile_id,
  qmi_wds_profile_params_type *profile_params,
  int                         *qmi_err_code
)
{
  NETMGR_LOG_FUNC_ENTRY;
  NETMGR_LOG_FUNC_EXIT;
  return QMI_NO_ERR;
}


int
qmi_wds_get_curr_call_info
(
  int                                        user_handle,
  qmi_wds_req_runtime_settings_params_type   requested_settings,
  qmi_wds_profile_id_type                    *profile_id,
  qmi_wds_profile_params_type                *profile_params,
  qmi_wds_curr_call_info_type                *call_settings,
  int                                        *qmi_err_code
)
{
  NETMGR_LOG_FUNC_ENTRY;

  if( 6 == user_handle )
  {
    call_settings->mask = (QMI_WDS_CURR_CALL_INFO_IP_FAMILY|
                           QMI_WDS_CURR_CALL_INFO_IPV6_ADDR|
                           QMI_WDS_CURR_CALL_INFO_IPV6_GTWY_ADDR|
                           QMI_WDS_CURR_CALL_INFO_MTU);
    call_settings->ip_family = QMI_WDS_IP_FAMILY_PREF_IPV6;
    inet_pton( AF_INET6, "2002:1234:5678:9876:5432:0000:1234:5678", call_settings->ipv6_addr_info.ipv6_addr );
    inet_pton( AF_INET6, "2002:5432:0000:1234:5678:1234:5678:9876", call_settings->ipv6_gateway_addr_info.ipv6_addr );
    call_settings->mtu       = MTU_IPV6;
  }
  else
  {
    call_settings->mask = (QMI_WDS_CURR_CALL_INFO_IP_FAMILY|
                           QMI_WDS_CURR_CALL_INFO_IPV4_ADDR|
                           QMI_WDS_CURR_CALL_INFO_IPV4_GATEWAY_ADDR|
                           QMI_WDS_CURR_CALL_INFO_MTU);
    call_settings->ip_family = QMI_WDS_IP_FAMILY_PREF_IPV4;
    inet_pton( AF_INET, "34.12.46.10", &call_settings->ipv4_addr );
    inet_pton( AF_INET,  "1.12.46.10", &call_settings->ipv4_gateway_addr );
    call_settings->mtu       = MTU_IPV4;
  }


  NETMGR_LOG_FUNC_EXIT;
  return QMI_NO_ERR;
}

int
qmi_wds_set_event_report
(
  int                               user_handle,
  qmi_wds_event_report_params_type  *event_params,
  int                               *qmi_err_code
)
{
  NETMGR_LOG_FUNC_ENTRY;
  NETMGR_LOG_FUNC_EXIT;
  return QMI_NO_ERR;
}

int
qmi_wds_set_client_ip_pref
(
  int                          user_handle,
  qmi_wds_ip_family_pref_type  ip_family,
  int                          *qmi_err_code
)
{
  NETMGR_LOG_FUNC_ENTRY;
  NETMGR_LOG_FUNC_EXIT;
  return QMI_NO_ERR;
}


qmi_client_handle_type
qmi_qos_srvc_init_client
(
  const char                    *dev_id,
  qmi_qos_indication_hdlr_type  ind_hdlr,
  void                          *ind_hdlr_user_data,
  int                           *qmi_err_code
)
{
  NETMGR_LOG_FUNC_ENTRY;
  NETMGR_LOG_FUNC_EXIT;
  return qmi_qos_client++;
}


int 
qmi_qos_srvc_release_client
(
  qmi_client_handle_type  client_handle,
  int                     *qmi_err_code
)
{
  NETMGR_LOG_FUNC_ENTRY;
  NETMGR_LOG_FUNC_EXIT;
  return QMI_NO_ERR;
}

int
qmi_qos_set_client_ip_pref
(
  int                          user_handle,
  qmi_ip_family_pref_type      ip_family,
  int                         *qmi_err_code
)
{
  NETMGR_LOG_FUNC_ENTRY;
  NETMGR_LOG_FUNC_EXIT;
  return QMI_NO_ERR;
}

int
qmi_qos_set_event_report_state
(
  int                               client_handle,
  qmi_qos_event_report_state_type   *report_state,
  int                               *qmi_err_code
)
{
  NETMGR_LOG_FUNC_ENTRY;
  NETMGR_LOG_FUNC_EXIT;
  return QMI_NO_ERR;
}

int
qmi_qos_get_primary_granted_qos_info
(
  qmi_client_handle_type                  client_handle,
  unsigned long                           qos_identifier,
  qmi_qos_granted_info_rsp_type           *granted_info,
  int                                     *qmi_err_code
)
{
  NETMGR_LOG_FUNC_ENTRY;

  // Fake CDMA profile
  granted_info->tx_granted_flow_data_is_valid = TRUE;
  granted_info->tx_granted_flow_data.qos_flow_granted.cdma_flow_desc.param_mask = QMI_QOS_CDMA_FLOW_PARAM_PROFILE_ID;
  granted_info->tx_granted_flow_data.qos_flow_granted.cdma_flow_desc.profile_id = 5;
  
  NETMGR_LOG_FUNC_EXIT;
  return QMI_NO_ERR;
}

int
qmi_qos_get_secondary_granted_qos_info
(
  qmi_client_handle_type                  client_handle,
  unsigned long                           qos_identifier,
  qmi_qos_granted_info_rsp_type           *granted_info,
  int                                     *qmi_err_code
)
{
  NETMGR_LOG_FUNC_ENTRY;

  // Fake CDMA profile
  granted_info->tx_granted_flow_data_is_valid = TRUE;
  granted_info->tx_granted_flow_data.qos_flow_granted.cdma_flow_desc.param_mask = QMI_QOS_CDMA_FLOW_PARAM_PROFILE_ID;
  granted_info->tx_granted_flow_data.qos_flow_granted.cdma_flow_desc.profile_id = 7;

  NETMGR_LOG_FUNC_EXIT;
  return QMI_NO_ERR;
}


#endif /* NETMGR_OFFTARGET */

