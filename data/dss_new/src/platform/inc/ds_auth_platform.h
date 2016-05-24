#ifndef DS_AUTH_PLATFORM_H
#define DS_AUTH_PLATFORM_H
/******************************************************************************
  @file    ds_auth_platform.h
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

  $Header: //source/qcom/qct/modem/datacommon/dsprofile/rel/08.02.02/src/ds_profile_3gpp_qmi.c#2 $ $DateTime: 2010/09/11 18:23:35 $ $Author: smudired $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
11/08/10   mct     DS Api for AKAv2 algorithm.
===========================================================================*/

#include "comdef.h"


/*===========================================================================*/
 /**
 FUNCTION:      DS_AUTH_PLATFORM_RUN_AKA_ALGO

 @brief        Runs AKAv1 or AKAv2 for the client & returns the result in the 
               callback function provided by the client
 
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
 */
/*===========================================================================*/
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
);

/*===========================================================================
FUNCTION DS_AUTH_PLATFORM_INIT

DESCRIPTION
  This function is called on the library init. It calls the appropriate
  functions to initialize the modem APIs or QMI-based APIs.
PARAMETERS 

DEPENDENCIES 
  
RETURN VALUE 
  None
SIDE EFFECTS 
  
===========================================================================*/
void ds_auth_platform_init(void);
#endif /* DS_AUTH_PLATFORM_H */

