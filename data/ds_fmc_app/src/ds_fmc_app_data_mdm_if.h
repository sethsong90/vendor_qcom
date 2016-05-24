#ifndef DS_FMC_APP_DATA_MDM_IF_H
#define DS_FMC_APP_DATA_MDM_IF_H

/******************************************************************************

               D S _ F M C _ A P P _ D A T A _ M D M _ I F . H

******************************************************************************/

/******************************************************************************
  @file    ds_fmc_app_data_mdm_if.h
  @brief   The DS_FMC_APP Data Modem interface layer header file

  DESCRIPTION
  The DS_FMC_APP data modem interface.  This file will pull in
  the appropriate platform header file(s).

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
                       GLOBAL DEFINITIONS AND DECLARATIONS
******************************************************************************/

#include "ds_fmc_app_data_if.h"

/*===========================================================================
  FUNCTION  ds_fmc_app_data_mdm_open_conn
===========================================================================*/
/*!
@brief 
  Function used to open a connection.  This function must be called
  prior to sending any messages or receiving any indications 
  
@return 
  DS_FMC_APP_SUCCESS if function is successful, DS_FMC_APP_FAILURE if not.

@note
  - Side Effects
    - Opens up SMD port and spawns a thread for RX handling.
    
*/    
/*=========================================================================*/
extern int
ds_fmc_app_data_mdm_open_conn 
(
  int                      conn_id,
  unsigned char           *rx_buf
);

/*===========================================================================
  FUNCTION  ds_fmc_app_data_mdm_close_conn
===========================================================================*/
/*!

@brief 
  Function used to close a connection. This function must 
  be called when the controlling process gets killed . 
  
@return 
  - void

@note
  - Side Effects
    - Closes the SMD channel and corresponding threads assoc. with
      RX handling.
    
*/    
/*=========================================================================*/
void
ds_fmc_app_data_mdm_close_conn 
(
  void
);

/*===========================================================================
  FUNCTION  ds_fmc_app_data_mdm_init
===========================================================================*/
/*!
@brief 
  Initialization function to be called once at power-up.  Must be called
  prior to calling the ds_fmc_app_data_mdm_open_conn() 
  
@return 
  - void

@note

  - Side Effects
    - None
    
*/    
/*=========================================================================*/
extern void
ds_fmc_app_data_mdm_init
(
  ds_fmc_app_data_mdm_rx_cb_ptr  rx_func
);

#endif /* DS_FMC_APP_DATA_MDM_IF_H */
