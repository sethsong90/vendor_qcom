/******************************************************************************

                      D S C _ C A L L . H

******************************************************************************/

/******************************************************************************

  @file    dsc_call.h
  @brief   DSC's call state machine header file

  DESCRIPTION
  Header file for PS call state machine in the DSC.

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2008 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //source/qcom/qct/platform/linux/common/main/latest/apps/dsc/src/dsc_call.h#3 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
03/15/08   vk         Incorporated code review comments
11/29/07   vk         Cleaned up lint warnings
11/17/07   vk         Added function headers and other comments
09/28/07   vk         Initial version

******************************************************************************/

#ifndef __DSC_CALL_H__
#define __DSC_CALL_H__

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include "dsci.h"
#include "dsc_dcmi.h"
#include "dsc_qmi_wds.h"

/*===========================================================================
                     GLOBAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/*--------------------------------------------------------------------------- 
   Constant representing an invalid call ID
---------------------------------------------------------------------------*/
#define DSC_CALLID_INVALID -1

/*--------------------------------------------------------------------------- 
   Type representing a call ID
---------------------------------------------------------------------------*/
typedef int dsc_callid_t;

/*--------------------------------------------------------------------------- 
   Enumeration of technology types
---------------------------------------------------------------------------*/
#if 0
typedef enum {
    DSC_CALL_TECH_INVALID = 0,
    DSC_CALL_TECH_CDMA, 
    DSC_CALL_TECH_UMTS
} dsc_call_tech_t;
#endif
/*--------------------------------------------------------------------------- 
   Type representing collection of configuration data of Call SM module
---------------------------------------------------------------------------*/
#if 0
typedef struct {
    // dsc_call_tech_t tech;
    int numts;
    int ncall;
} dsc_call_cfg_t;
#endif
/*--------------------------------------------------------------------------- 
   Enumeration of states of a primary call
---------------------------------------------------------------------------*/
typedef enum {
    DSC_PRICALL_IDLE            = 0,
    DSC_PRICALL_CONNECTING_QMI  = 1,
    DSC_PRICALL_CONNECTING_KIF  = 2,
    DSC_PRICALL_CONNECTED       = 3,
    DSC_PRICALL_DISCONNECTING_I = 4,
    DSC_PRICALL_DISCONNECTING_F = 5,
    DSC_PRICALL_RECONFIGURING_KIF = 6
} dsc_pricall_state_t;

/*--------------------------------------------------------------------------- 
   Type representing collection of call information/state for a primary call
---------------------------------------------------------------------------*/
typedef struct {
    
    dsc_callid_t             callid;
    dcm_iface_id_t           if_id;
    dsc_pricall_state_t      state;
    dsc_pricall_params_t     req_params;
    int                      call_end_reason_code;
    int                      link;
} dsc_pricall_info_t;

/*===========================================================================
                     GLOBAL FUNCTION DECLARATIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  dsc_call_init
===========================================================================*/
/*!
@brief
  Main initialization routine for the Call SM module.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void dsc_call_init(int npri);

int dsc_pricall_disconnect_req (dsc_callid_t callid);
int dsc_pricall_connect_req (dsc_callid_t callid);

#endif /* __DSC_CALL_H__ */
