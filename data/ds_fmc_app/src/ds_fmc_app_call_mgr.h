/******************************************************************************

          D S _ F M C _ A P P _ C A L L _ M G R . H

******************************************************************************/

/******************************************************************************

  @file    ds_fmc_app_call_mgr.h
  @brief   DS_FMC_APP Call Mgr entity Interface

  DESCRIPTION
  Implementation of DS_FMC_APP call mgr entity interface.

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

#ifndef __DS_FMC_APP_CALL_MGR_H__
#define __DS_FMC_APP_CALL_MGR_H__

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/
#include "comdef.h"
#include "ds_fmc_app.h"

/*===========================================================================
                     GLOBAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/*===========================================================================
                            GLOBAL FUNCTION DECLARATIONS
===========================================================================*/
/*===========================================================================
  FUNCTION  ds_fmc_app_cm_tx_msg
===========================================================================*/
/*!
@brief
  The function to send out messages via the listener fd to a client

@return
  DS_FMC_APP_SUCCESS - On Success DS_FMC_APP_FAILURE - On failure

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/

int ds_fmc_app_cm_tx_msg
(
  int fd,
  unsigned char *msg,
  int  msg_len
);

/*===========================================================================
  FUNCTION  ds_fmc_app_call_mgr_init
===========================================================================*/
/*!
@brief
  Starts the socket listener thread and associates it with the specified 
  handle.

@return
  - void

@note

  - Dependencies - None

  - Side Effects
    - Spawns a pthread for reading data received on associated sockets.
*/
/*=========================================================================*/
void ds_fmc_app_call_mgr_init
(
  void
);

/*===========================================================================
  FUNCTION  ds_fmc_app_call_mgr_close
===========================================================================*/
/*!
@brief
  Closes resources associated with the DS_FMC_APP call mgr module.

@return
   - void
@note

  - Dependencies - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
void ds_fmc_app_call_mgr_close
(
  void
);

/*===========================================================================
  FUNCTION  ds_fmc_app_tunnel_mgr_get_cm_client_fd
===========================================================================*/
/*!
@brief
  The function returns the client fd to the caller module

@return
  int - cm_client_fd

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/

int ds_fmc_app_cm_get_cm_client_fd
(
  void
);

#endif /* __DS_FMC_APP_CALL_MGR_H__ */
