#ifndef DS_FMC_APP_DATA_IF_H
#define DS_FMC_APP_DATA_IF_H

/******************************************************************************

               D S _ F M C _ A P P _ D A T A _ I F . H

******************************************************************************/

/******************************************************************************
  @file    ds_fmc_app_data_if.h
  @brief   The DS_FMC_APP Data interface layer header file

  DESCRIPTION
  The DS_FMC_APP data interface.  This file will pull in
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

#define DS_FMC_APP_MAX_MSG_SIZE    2048
#define DS_FMC_APP_DATA_CONN_ID_0  0
#define DS_FMC_APP_MAX_CONNECTIONS 1

unsigned char ds_fmc_app_data_mdm_rx_buf[DS_FMC_APP_MAX_MSG_SIZE];

unsigned char ds_fmc_app_data_ext_rx_buf[DS_FMC_APP_MAX_MSG_SIZE];

/* Typedef used by platform specific I/O layer to report incoming data */
typedef int (*ds_fmc_app_data_mdm_rx_cb_ptr) 
(
  int                     fd,
  unsigned char           *rx_buf_ptr,
  int                     rx_buf_len
);

/* Typedef used by platform specific I/O layer to report outgoing data */
typedef int (*ds_fmc_app_data_ext_rx_cb_ptr) 
(
  int                     fd,
  unsigned char           *rx_buf_ptr,
  int                     rx_buf_len
);
/*===========================================================================
  FUNCTION  ds_fmc_app_data_mdm_send_data
===========================================================================*/
/*!
@brief 
  Function to send data to SMD to be sent across to the modem
  
@return 
  DS_FMC_APP_SUCCESS if function is successful, DS_FMC_APP_FAILURE if not.

@note

  - Connection must have been previously opened.

  - Side Effects
    - None
    
*/    
/*=========================================================================*/
extern int
ds_fmc_app_data_mdm_send_data
(
  int                      conn_id,
  unsigned char           *msg_ptr,
  int                      msg_len
);


/*===========================================================================
  FUNCTION  ds_fmc_app_data_ext_send_data
===========================================================================*/
/*!
@brief 
  Function to send data over DATA_EXT sockets to be sent over the air
  
@return 
  DS_FMC_APP_SUCCESS if function is successful, DS_FMC_APP_FAILURE if not.

@note

  - Connection must have been previously opened.

  - Side Effects
    - None
    
*/    
/*=========================================================================*/
extern int
ds_fmc_app_data_ext_send_data
(
  int                      conn_id,
  unsigned char           *msg_ptr,
  int                      msg_len
);

#endif /* DS_FMC_APP_DATA_IF_H */
