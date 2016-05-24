/******************************************************************************

                        D S C _ Q M I _ N A S I . H

******************************************************************************/

/******************************************************************************

  @file    dsc_qmi_nasi.h
  @brief   DSC's QMI NAS service internal Header file.

  DESCRIPTION
  Internal header file for DCM (Data Connection Manager) module.

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2008 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#ifndef __DSC_QMI_NASI_H__
#define __DSC_QMI_NASI_H__

#include "qmi_nas_srvc.h"

#define DSC_QMI_MAX_RADIO_IFACES    6

typedef enum
{
  DSC_QMI_NAS_NOT_REGISTERED            = 0,
  DSC_QMI_NAS_REGISTERED                = 1,
  DSC_QMI_NAS_NOT_REGISTERED_SEARCHING  = 2,
  DSC_QMI_NAS_REGISTRATION_DENIED       = 3,
  DSC_QMI_NAS_REGISTRATION_UNKNOWN      = 4
} dsc_nas_reg_state;

typedef enum
{
  DSC_QMI_NAS_PS_ATTACH_UNKNOWN = 0,
  DSC_QMI_NAS_PS_ATTACHED       = 1,
  DSC_QMI_NAS_PS_DETACHED       = 2
} dsc_nas_ps_attach_state;

typedef enum
{
  DSC_NAS_NO_SRVC_TECH    = 0x00,
  DSC_NAS_CDMA200_TECH    = 0x01,
  DSC_NAS_CDMA_HRPD_TECH  = 0x02,
  DSC_NAS_AMPS_TECH       = 0x03,
  DSC_NAS_GSM_TECH        = 0x04,
  DSC_NAS_UMTS_TECH       = 0x05
} dsc_nas_tech_type;

typedef struct 
{
  dsc_nas_reg_state           regisration_state;
  dsc_nas_ps_attach_state     ps_attach_state;
  unsigned short              num_radio_ifaces;
  dsc_nas_tech_type           radio_if[DSC_QMI_MAX_RADIO_IFACES];
} dsc_nas_radio_tech_info;


/*===========================================================================
                            GLOBAL FUNCTION DECLARATIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  dsc_nas_query_technology
===========================================================================*/
/*!
@brief
 Queries the NAS service for the current radio technology 
 on which the phone ir camped.

@return
  return value of -1 indicates an error returened from qmi
  return value of 0 indicated that the query was a success

@note

  - Dependencies
    - None  

  - Side Effects
    - Blocks execution of thread until the QMI message exchange to get the 
      profile parameters is complete.
*/
/*=========================================================================*/
int 
dsc_nas_query_technology 
(
  dsc_nas_radio_tech_info   *radio_info 
);
#endif
