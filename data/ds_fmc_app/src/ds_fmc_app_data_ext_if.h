#ifndef DS_FMC_APP_DATA_EXT_IF_H
#define DS_FMC_APP_DATA_EXT_IF_H

/******************************************************************************

              D S _ F M C _ A P P _ D A T A _ E X T _ I F . H

******************************************************************************/

/******************************************************************************
  @file    ds_fmc_app_data_ext_if.h
  @brief   The DS_FMC_APP DATA_EXT interface layer header file

  DESCRIPTION
  The DS_FMC_APP OTA interface.  This file will pull in
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
  FUNCTION  ds_fmc_app_data_ext_open_conn
===========================================================================*/
/*!
@brief 
  Function used to open a connection.  This function must be called
  prior to sending any messages or receiving any indications 
  
@return 
  DS_FMC_APP_SUCCESS if function is successful, DS_FMC_APP_FAILURE if not.

@note
  - Side Effects
    - Opens up a DATA_EXT socket and spawns a thread for RX handling.
    
*/    
/*=========================================================================*/
int
ds_fmc_app_data_ext_open_conn 
(
  int                 conn_id,
  struct sockaddr    *addr, 
  socklen_t           addrlen,
  unsigned char       *rx_buf
);

/*===========================================================================
  FUNCTION  ds_fmc_app_data_ext_init
===========================================================================*/
/*!
@brief 
  Initialization function to be called once at power-up.  Must be called
  prior to calling the ds_fmc_app_data_ext_open_conn() 
  
@return 
  - void

@note

  - Connection is assumed to be opened with valid data before this 
  function starts to execute

  - Side Effects
    - None
    
*/    
/*=========================================================================*/
void
ds_fmc_app_data_ext_init
(
  ds_fmc_app_data_ext_rx_cb_ptr  rx_func
);
 
/*===========================================================================
  FUNCTION  ds_fmc_app_data_ext_close_conn
===========================================================================*/
/*!
@brief 
  Function used to close a connection.  This function must be called
  once data transfer is complete and we are ready to initiate closure 
  of the DATA_EXT socket. 
  
@return 
  DS_FMC_APP_SUCCESS if function is successful, DS_FMC_APP_FAILURE if not.

@note
  - Side Effects
    - Closes the DATA_EXT socket and corresponding threads assoc. with
      RX handling.
    
*/    
/*=========================================================================*/
int
ds_fmc_app_data_ext_close_conn 
(
  int           conn_id
);

/*===========================================================================
  FUNCTION  ds_fmc_app_data_ext_conn_open_status
===========================================================================*/
/*!
@brief 
  Function used to query connection status. This function is invoked
  when a Tunnel closure event is received by the tunnel mgr module.
  
@return 
  Returns value of the conn_info assc. with conn_id.

@note
  - Side Effects
    - None.
    
*/    
/*=========================================================================*/
boolean
ds_fmc_app_data_ext_conn_open_status 
(
  int                 conn_id
);

#endif /* DS_FMC_APP_DATA_EXT_IF_H */

