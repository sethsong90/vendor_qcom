/******************************************************************************

                          D S _ F M C _ A P P _ Q M I . H

******************************************************************************/

/******************************************************************************

  @file    ds_fmc_app_qmi.h
  @brief   DS_FMC_APP QMI Driver Interface header file

  DESCRIPTION
  Header file for DS_FMC_APP QMI Driver interface.

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

#ifndef __DS_FMC_APP_QMI_H__
#define __DS_FMC_APP_QMI_H__

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

#define QMI_PORT_DEV_ID_LEN 32

/*--------------------------------------------------------------------------- 
  Type representing collection of WDS control info related to the QMI Driver
---------------------------------------------------------------------------*/
typedef struct {
  int                                 clnt_hdl; /* QMI WDS client handle */
} ds_fmc_app_wds_qmi_drv_info_t;

/*--------------------------------------------------------------------------- 
  Type representing collection of a link's state information
---------------------------------------------------------------------------*/
typedef struct {
  ds_fmc_app_wds_qmi_drv_info_t      wds_info;  /* WDS service info */
} ds_fmc_app_qmi_srvc_info_t;

/*--------------------------------------------------------------------------- 
  Collection of configuration information for the module
---------------------------------------------------------------------------*/
struct ds_fmc_app_qmi_cfg_s {
  char                                dev_id[QMI_PORT_DEV_ID_LEN];
  ds_fmc_app_qmi_srvc_info_t          wds_srvc;
                                 /* QMI WDS client handle */
  ds_fmc_app_fmc_bearer_status_type_t fmc_bearer_status;
                                 /* Status indicating enabled or disabled */
};

/*===========================================================================
                            GLOBAL FUNCTION DECLARATIONS
===========================================================================*/


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
void ds_fmc_app_qmi_init ();

/*===========================================================================
  FUNCTION  ds_fmc_app_qmi_wds_set_tunnel_params
===========================================================================*/
/*!
@brief
  Sends a synchronous QMI message to the mdm with the required tunnel 
  parameters as specified by ds_fmc_app_tunnel_mgr_ds.

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
ds_fmc_app_qmi_wds_set_tunnel_params 
(
  ds_fmc_app_tunnel_mgr_ds_type *ds_fmc_app_tunnel_mgr_ds
);

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
);

#endif /* __DS_FMC_APP_QMI_H__ */
