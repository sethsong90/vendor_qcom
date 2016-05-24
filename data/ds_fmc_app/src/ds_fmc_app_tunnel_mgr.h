/******************************************************************************

              D S _ F M C _ A P P _ T U N N E L _ M G R . H

******************************************************************************/

/******************************************************************************

  @file    ds_fmc_app_tunnel_mgr.h
  @brief   DS_FMC_APP Tunnel Manager Interface header file

  DESCRIPTION
  Header file for DS_FMC_APP Tunnel Manager interface.

******************************************************************************/
/*===========================================================================

  Copyright (c) 2010 Qualcomm Technologies, Inc. All Rights Reserved

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
02/13/10   scb        Initial version

******************************************************************************/

#ifndef __DS_FMC_APP_TUNNEL_MGR_H__
#define __DS_FMC_APP_TUNNEL_MGR_H__

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/
#include "comdef.h"
#include "qmi.h"
#include "qmi_wds_srvc.h"
#include "ds_fmc_app.h"

/*===========================================================================
                     GLOBAL DEFINITIONS AND DECLARATIONS
===========================================================================*/


/*===========================================================================
                            GLOBAL FUNCTION DECLARATIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  ds_fmc_app_tunnel_mgr_get_tunnel_mgr_client_fd
===========================================================================*/
/*!
@brief
  The function returns the client fd to the caller module

@return
  int - tunnel_mgr_client_fd

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/

int ds_fmc_app_tunnel_mgr_get_tunnel_mgr_client_fd
(
  void
);

/*===========================================================================
  FUNCTION  ds_fmc_app_tunnel_mgr_init
===========================================================================*/
/*!
@brief
  Tunnel Manager initialization function to initialize the mutex.

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
void ds_fmc_app_tunnel_mgr_init
(
  void
);

/*===========================================================================
  FUNCTION  ds_fmc_app_tunnel_mgr_config_ext_data_path
===========================================================================*/
/*!
@brief
  The function opens/closes a WLAN socket with appropriate tunnel params.
  parameters

@return
  int - DS_FMC_APP_SUCCESS on Success, DS_FMC_APP_FAILURE on Failure

@note

  - Dependencies
    - None  

  - Side Effects
    - Populates the tunnel_dest_ip appr. into the OUT param, before
      return
*/
/*=========================================================================*/

int ds_fmc_app_tunnel_mgr_config_ext_data_path
(
  ds_fmc_app_fmc_bearer_status_type_t bearer_status,
  struct sockaddr_storage             *addr
);

/*===========================================================================
  FUNCTION  ds_fmc_app_tunnel_mgr_config_tunnel_params
===========================================================================*/
/*!
@brief
  This function calls the QMI set tunnel params function to set tunnel
  parameters

@return
  int - DS_FMC_APP_SUCCESS on Success DS_FMC_APP_FAILURE otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/

int ds_fmc_app_tunnel_mgr_config_tunnel_params
(
  ds_fmc_app_tunnel_mgr_ds_type *ds_fmc_app_tunnel_mgr_ds
);

/*===========================================================================
  FUNCTION  ds_fmc_app_tunnel_mgr_client_tx_msg
===========================================================================*/
/*!
@brief
  Sends a message to the corresponding server.

@return
  int - DS_FMC_APP_SUCCESS on success, DS_FMC_APP_FAILURE otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/

int ds_fmc_app_tunnel_mgr_client_tx_msg
(
  int            client_fd,
  unsigned char *msg,
  int            msg_len
);

/*===========================================================================
  FUNCTION  ds_fmc_app_tunnel_mgr_open_conn
===========================================================================*/
/*!
@brief
 Opens a tunnel manager connection.

@return
  int - DS_FMC_APP_SUCCESS on success, DS_FMC_APP_FAILURE otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - populates the tunnel_mgr_client_fd with relevant file descriptor
      information
*/
/*=========================================================================*/

int ds_fmc_app_tunnel_mgr_open_conn
(
  void
);

/*===========================================================================
  FUNCTION  ds_fmc_app_tunnel_mgr_close_conn
===========================================================================*/
/*!
@brief
  Closes the file descriptor pointed to by client_fd.

@return
 - void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/

void
ds_fmc_app_tunnel_mgr_close_conn
(
  int client_fd
);

#endif /* __DS_FMC_APP_TUNNEL_MGR_H__ */
