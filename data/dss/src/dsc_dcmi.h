/******************************************************************************

                        D S C _ D C M I . H

******************************************************************************/

/******************************************************************************

  @file    dsc_dcmi.h
  @brief   DSC's DCM (Connection Manager) Module header file

  DESCRIPTION
  Internal header file for DCM (Data Connection Manager) module.

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2008 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //source/qcom/qct/platform/linux/common/main/latest/apps/dsc/src/dsc_dcmi.h#4 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
04/08/09   SM         Added Support for physlink Evnt Indications
05/30/08   vk         Added support for GET_DEVICE_NAME IOCTL
03/15/08   vk         Incorporated code review comments
11/17/07   vk         Added function headers and other comments
09/28/07   vk         Initial version

******************************************************************************/

#ifndef __DSC_DCMI_H__
#define __DSC_DCMI_H__

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include "ds_list.h"
#include "dsc_dcm.h"

/*===========================================================================
                     GLOBAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/*--------------------------------------------------------------------------- 
   Defines for Dormant/active 
---------------------------------------------------------------------------*/
#define DSC_DCM_PHYSLINK_DORMANT   1
#define DSC_DCM_PHYSLINK_ACTIVE    2
/*--------------------------------------------------------------------------- 
   Type representing an IOCTL as passed to the lower layers
---------------------------------------------------------------------------*/
typedef struct dsc_dcm_iface_ioctl_s {
    dcm_iface_ioctl_name_t  name;
    union {
        ip_addr_type                            ipv4_addr;
        ip_addr_type                            gateway;
        dns_info                                dns_addr;
        dss_iface_ioctl_bind_sock_to_iface_type bind_info;
        dss_iface_ioctl_device_name_type        device_name_info;
        unsigned int                            mtu;
        dss_iface_ioctl_data_bearer_tech_type   data_bearer_tech;
    } info;
} dsc_dcm_iface_ioctl_t;

/*--------------------------------------------------------------------------- 
   Type of the virtual function registered by lower layers that is used by
   DCM to issue the command to bring an IFACE up
---------------------------------------------------------------------------*/
typedef dsc_op_status_t (* dsc_dcm_if_up_cmd_f) 
(
    dcm_iface_id_t if_id,
    void         * call_hdl 
);

/*--------------------------------------------------------------------------- 
   Type of the virtual function registered by lower layers that is used by
   DCM to issue the command to bring an IFACE down
---------------------------------------------------------------------------*/
typedef dsc_op_status_t (* dsc_dcm_if_down_cmd_f) 
(
    dcm_iface_id_t if_id,
    void         * call_hdl
);

/*--------------------------------------------------------------------------- 
   Type of the virtual function registered by lower layers that serves to 
   perform lower-layer specific matching of the IFACE to specified network 
   policy
---------------------------------------------------------------------------*/
typedef dsc_op_status_t (* dsc_dcm_if_match_f)
(
    dcm_iface_id_t if_id,
    void  * call_hdl,
    dcm_net_policy_info_t * net_policy
);

/*--------------------------------------------------------------------------- 
   Type of the virtual function registered by lower layers that is used by
   DCM to set configuration parameters for the IFACE during interface
   selection
---------------------------------------------------------------------------*/
typedef dsc_op_status_t (* dsc_dcm_if_set_config_f)
(
    dcm_iface_id_t if_id,
    void  * call_hdl,
    dcm_net_policy_info_t * net_policy
);

/*--------------------------------------------------------------------------- 
   Type of the virtual function registered by lower layers that serves as
   their generic IOCTL handler
---------------------------------------------------------------------------*/
typedef dsc_op_status_t (* dsc_dcm_if_ioctl_f)
(
    dcm_iface_id_t             		if_id, 
    void                    	  * call_hdl,
    struct dsc_dcm_iface_ioctl_s  * ioctl
);

/*--------------------------------------------------------------------------- 
   Type of the virtual function table registered by lower layers to provide
   lower layer specific implementation of IFACE operations
---------------------------------------------------------------------------*/
typedef struct dsc_dcm_if_op_tbl_s {
    dsc_dcm_if_match_f      if_match_f;
    dsc_dcm_if_set_config_f if_set_config_f;
    dsc_dcm_if_up_cmd_f     if_up_cmd;
    dsc_dcm_if_down_cmd_f   if_down_cmd;
    dsc_dcm_if_ioctl_f      if_ioctl_f;
} dsc_dcm_if_op_tbl_t;

/*===========================================================================
                            GLOBAL FUNCTION DECLARATIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  dsc_dcm_if_get_name
===========================================================================*/
/*!
@brief
  Returns name of the iface pointed to be the specified iface id.

@return
  dcm_iface_name_t - Name of this iface

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
dcm_iface_name_t
dsc_dcm_if_get_name (dcm_iface_id_t if_id);

/*===========================================================================
  FUNCTION  dsc_dcm_if_create
===========================================================================*/
/*!
@brief
  Creates an IFACE of the specified type in the system and registers the 
  lower-layer specified handlers. 

@return
  dcm_iface_id_t - IFACE ID if successful, DCM_IFACE_ID_INVALID otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
dcm_iface_id_t
dsc_dcm_if_create
(
    dcm_iface_name_t        if_name,
    dcm_iface_name_t        if_group, 
    void *                  call_hdl, 
    dsc_dcm_if_op_tbl_t   * if_op_tbl_p
);

/*===========================================================================
  FUNCTION  dsc_dcm_if_up_ind
===========================================================================*/
/*!
@brief
  Function used by the lower layers to indicate that the IFACE should be 
  moved to the UP state.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - Associated network callbacks of clients are called.
*/
/*=========================================================================*/
void
dsc_dcm_if_up_ind
(
    dcm_iface_id_t if_id
);

/*===========================================================================
  FUNCTION  dsc_dcm_if_down_ind
===========================================================================*/
/*!
@brief
  Function used by the lower layers to indicate that the IFACE should be 
  moved to the DOWN state.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - Associated network callbacks of clients are called.
*/
/*=========================================================================*/
void
dsc_dcm_if_down_ind
(
    int            call_end_reason,
    dcm_iface_id_t if_id
);

/*===========================================================================
  FUNCTION  dsc_dcm_if_reconfigured_ind
===========================================================================*/
/*!
@brief
  Function used by the lower layers to indicate that the IFACE is
  reconfigured.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - Associated network callbacks of clients are called.
*/
/*=========================================================================*/
void
dsc_dcm_if_reconfigured_ind
(
    dcm_iface_id_t if_id
);

/*===========================================================================
  FUNCTION  dsc_dcm_physlink_state_change_ind
===========================================================================*/
/*!
@brief
  Function used by the lower layers to indicate physlink state change.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - Associated network callbacks of clients are called.
*/
/*=========================================================================*/
void
dsc_dcm_physlink_state_change_ind
(
    int            dorm_status,
    dcm_iface_id_t if_id
);

/*===========================================================================
  FUNCTION  dsc_dcm_init
===========================================================================*/
/*!
@brief
  Initialization routine for the DCM module. 

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void
dsc_dcm_init 
(
    void
);

#endif /* __DSC_DCMI_H__ */
