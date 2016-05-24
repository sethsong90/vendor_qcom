/******************************************************************************
  @file    ds_auth.c
  @brief   

  DESCRIPTION
  AKAv2 API wrapper implementation for modem and QMI based clients.

  INITIALIZATION AND SEQUENCING REQUIREMENTS
  N/A

  ---------------------------------------------------------------------------
  Copyright (C) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------
****************************************************************************/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ds_auth.c#1 $ $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
11/08/10   mct     DS Api for AKAv2 algorithm.
===========================================================================*/

#include "comdef.h"

#include "ds_Utils_DebugMsg.h"
#include "dserrno.h"
#include "ds_auth.h"
#include "ds_auth_platform.h"

/*===========================================================================
FUNCTION DS_AUTH_INIT

DESCRIPTION
  This function is called on the library init. It calls the appropriate
  functions to initialize the modem APIs or QMI-based APIs.
PARAMETERS 

DEPENDENCIES 
  
RETURN VALUE 
  None
SIDE EFFECTS 
  
===========================================================================*/
void ds_auth_init( )
{
  LOG_MSG_INFO2("Initializing DS auth module.",0,0,0);
  ds_auth_platform_init();
}

/*===========================================================================*/
 /**
 FUNCTION:      DS_AUTH_RUN_AKA_ALGO

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
ds_auth_aka_handle_type ds_auth_run_aka_algo
(
  ds_auth_aka_ver_enum_type   aka_ver,
  uint8*                      rand,
  uint8                       rand_len,
  uint8*                      autn,
  uint8                       autn_len,
  ds_auth_aka_callback_type   aka_algo_cback,
  void                       *user_data,
  sint15                     *ds_errno
)
{
  if (NULL == ds_errno)
  {
    LOG_MSG_ERROR("ds_errno is NULL",0,0,0);
    return -1;
  }

  if (aka_ver >= DS_AUTH_AKA_MAX_VER)
  {
    LOG_MSG_ERROR("Invalid AKA version %d",aka_ver,0,0);
    *ds_errno = DS_EINVAL;
    return -1;
  }

  if (rand_len == 0 || autn_len == 0)
  {
    LOG_MSG_ERROR("Invalid rand_len %d or autn_len %d",rand_len,autn_len,0);
    *ds_errno = DS_EINVAL;
    return -1;
  }

  if (NULL == rand || NULL == autn)
  {
    LOG_MSG_ERROR("rand %p or autn %p is NULL",rand,autn,0);
    *ds_errno = DS_EFAULT;
    return -1;
  }

  if (NULL == aka_algo_cback)
  {
    LOG_MSG_ERROR("No user specified callback provided!",0,0,0);
    *ds_errno = DS_EFAULT;
    return -1;
  }

  return ds_auth_platform_run_aka_algo
         (
           aka_ver,
           rand,
           rand_len,
           autn,
           autn_len,
           aka_algo_cback,
           user_data,
           ds_errno
         );
}
