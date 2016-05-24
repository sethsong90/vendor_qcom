/******************************************************************************

                           D S _ F M C _ A P P  . H

******************************************************************************/

/******************************************************************************

  @file    ds_fmc_app.h
  @brief   DS_FMC_APP public header file

  DESCRIPTION
  Public header file for DS_FMC_APP daemon 

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
08/28/12   op         Changed socket locations for Jellybean

******************************************************************************/

#ifndef __DS_FMC_APP_H__
#define __DS_FMC_APP_H__

#include <sys/types.h>
#include <sys/socket.h>

/*===========================================================================
                     GLOBAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

#define DS_FMC_APP_SUCCESS (0)
#define DS_FMC_APP_FAILURE (-1)

#ifdef FEATURE_DS_LINUX_ANDROID
#define DS_FMC_APP_CALL_MGR_CONN_SOCKET_PATH   "/dev/socket/ds_fmc_app_call_mgr_sock"
#define DS_FMC_APP_CALL_MGR_CLIENT_SOCKET_PATH "/dev/socket/ds_fmc_app_call_mgr_client_sock"
#else
#define DS_FMC_APP_CALL_MGR_CONN_SOCKET_PATH   "/var/ds_fmc_app_call_mgr_sock"
#define DS_FMC_APP_CALL_MGR_CLIENT_SOCKET_PATH "/var/ds_fmc_app_call_mgr_client_sock"
#endif /*FEATURE_DS_LINUX_ANDROID*/


#ifdef FEATURE_DS_LINUX_ANDROID
#define DS_FMC_APP_TUNNEL_MGR_CONN_SOCKET_PATH   "/dev/socket/ds_fmc_app_tunnel_mgr_sock"
#define DS_FMC_APP_TUNNEL_MGR_CLIENT_SOCKET_PATH "/dev/socket/ds_fmc_app_tunnel_mgr_client_sock"
#else
#define DS_FMC_APP_TUNNEL_MGR_CONN_SOCKET_PATH   "/var/ds_fmc_app_tunnel_mgr_sock"
#define DS_FMC_APP_TUNNEL_MGR_CLIENT_SOCKET_PATH "/var/ds_fmc_app_tunnel_mgr_client_sock"
#endif /*FEATURE_DS_LINUX_ANDROID*/

/*--------------------------------------------------------------------------- 
  Type representing enumeration of FMC BEARER status
---------------------------------------------------------------------------*/
typedef enum {
  DS_FMC_APP_FMC_BEARER_INVALID = -1,
  DS_FMC_APP_FMC_BEARER_DISABLED,
  DS_FMC_APP_FMC_BEARER_ENABLED,
  DS_FMC_APP_FMC_BEARER_MAX
} ds_fmc_app_fmc_bearer_status_type_t;

/*--------------------------------------------------------------------------- 
  Type representing enumeration of tunnel status
---------------------------------------------------------------------------*/
typedef enum {
  DS_FMC_APP_TUNNEL_STATUS_INVALID = -1,
  DS_FMC_APP_TUNNEL_STATUS_RESET,
  DS_FMC_APP_TUNNEL_STATUS_SET,
  DS_FMC_APP_TUNNEL_STATUS_MAX
} ds_fmc_app_tunnel_status_type_t;


/*--------------------------------------------------------------------------- 
     ds_fmc_app_call_mgr_resp_type_t data structure to be
     sent along to the call manager.
---------------------------------------------------------------------------*/

typedef struct
{
  ds_fmc_app_fmc_bearer_status_type_t ds_fmc_app_fmc_bearer_status; 
                                    /* FMC bearer status*/
  struct sockaddr_storage             tunnel_dest_ip; 
                                    /* Tunnel destination IP addr*/
}ds_fmc_app_call_mgr_resp_type;


/*--------------------------------------------------------------------------- 
     ds_fmc_app_tunnel_mgr_ds_type data structure to be
     populated by the Tunnel manager.
---------------------------------------------------------------------------*/

typedef struct
{
  ds_fmc_app_tunnel_status_type_t is_tunnel_set ;      
                                                /* Tunnel set indicator */
  struct sockaddr_storage  tunnel_dest_ip;      /* destination addr */
  int                      dest_strm_id ;       /* stream id */
  char                     nat_present ;        /* NAT presence indicator */
} ds_fmc_app_tunnel_mgr_ds_type;

#endif /* __DS_FMC_APP_H__ */
