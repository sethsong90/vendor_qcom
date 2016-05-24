/******************************************************************************

                        D S C _ D C M . H

******************************************************************************/

/******************************************************************************

  @file    dsc_dcm.h
  @brief   DSC's DCM (Connection Manager) Module header file

  DESCRIPTION
  External header file for DCM (Data Connection Manager) module.

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2010 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //source/qcom/qct/platform/linux/common/main/latest/apps/dsc/inc/dsc_dcm.h#5 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
03/24/09   SM         Added Call End Reason Code Support
02/20/09   js         Added username/password support in net policy
05/30/08   vk         Added support for GET_DEVICE_NAME IOCTL
05/28/08   vk         Added support for APN override
12/18/07   vk         Support for automatic client deregistration
11/17/07   vk         Added function headers and other comments
09/28/07   vk         Initial version

******************************************************************************/

#ifndef __DSC_DCM_H__
#define __DSC_DCM_H__

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include "comdef.h"
#include "ds_linux.h"
#include "dsci.h"
#include "dssocket_defs_linux.h"
#include "ds_fd_pass.h"

/*===========================================================================
                     GLOBAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/*--------------------------------------------------------------------------- 
   Constant representing invalid DCM Client Handle
---------------------------------------------------------------------------*/
#define DCM_CLIENT_HDL_INVALID -1

/*--------------------------------------------------------------------------- 
   Constant representing invalid IFACE ID
---------------------------------------------------------------------------*/
#define DCM_IFACE_ID_INVALID DSS_IFACE_INVALID_ID

/*--------------------------------------------------------------------------- 
   Constant representing DCM ERROR VALUE
---------------------------------------------------------------------------*/
#define DSC_DCM_ERR_VAL    -1
#define DSC_DCM_NO_ERR      0
/*--------------------------------------------------------------------------- 
   Type representing IFACE ID
---------------------------------------------------------------------------*/
typedef dss_iface_id_type dcm_iface_id_t;

/*--------------------------------------------------------------------------- 
   Type representing IFACE ID kind (enumeration)
---------------------------------------------------------------------------*/
typedef dss_iface_id_kind_enum_type dcm_iface_id_kind_t;

/*--------------------------------------------------------------------------- 
   Type representing IFACE ID name (enumeration)
---------------------------------------------------------------------------*/
typedef dss_iface_name_enum_type dcm_iface_id_name_t;

/*--------------------------------------------------------------------------- 
   Type representing an IPv4 address
---------------------------------------------------------------------------*/
typedef dss_iface_ioctl_ipv4_addr_type dcm_iface_ipv4_addr_t;

/*--------------------------------------------------------------------------- 
   Type representing an IFACE name
---------------------------------------------------------------------------*/
typedef ps_iface_name_enum_type dcm_iface_name_t;

/*--------------------------------------------------------------------------- 
   Type representing an IFACE's state
---------------------------------------------------------------------------*/
typedef ps_iface_state_enum_type dcm_iface_state_t;

/*--------------------------------------------------------------------------- 
   Type representing a PHYS LINK's state
---------------------------------------------------------------------------*/
typedef phys_link_state_type dcm_phys_link_state_t;

/*--------------------------------------------------------------------------- 
   Type representing name of an IFACE event (enumeration)
---------------------------------------------------------------------------*/
typedef dss_iface_ioctl_event_enum_type dcm_iface_ioctl_event_name_t;

/*--------------------------------------------------------------------------- 
   Type representing collection of info related to an IFACE event
---------------------------------------------------------------------------*/
typedef dss_iface_ioctl_event_info_union_type dcm_iface_ioctl_event_info_t;

/*--------------------------------------------------------------------------- 
   Type representing name of an IFACE IOCTL
---------------------------------------------------------------------------*/
typedef dss_iface_ioctl_type dcm_iface_ioctl_name_t;

/*--------------------------------------------------------------------------- 
   Collection of info related to an IFACE event passed in an IOCTL
---------------------------------------------------------------------------*/
typedef struct dcm_iface_ioctl_event_s {
    dcm_iface_ioctl_event_name_t    name;
    dcm_iface_ioctl_event_info_t    info;
    int                             dcm_nethandle;
    /*~ FIELD dcm_iface_ioctl_event_s.info DISC dcm_iface_ioctl_event_s.name */
} dcm_iface_ioctl_event_t;

/*--------------------------------------------------------------------------- 
   Type of function callback used to notify clients of network events
---------------------------------------------------------------------------*/
typedef void (*dcm_net_cb_fcn)
(
  int               dcm_nethandle,                               /* Application id */
  dcm_iface_id_t    iface_id,                            /* Interfcae id structure */
  int               dss_errno,     /* type of network error, ENETISCONN, ENETNONET.*/
  void            * net_cb_user_data                       /* Call back User data  */
);

/*--------------------------------------------------------------------------- 
   Type of function callback used to notify clients of IFACE events; 
   these are events that require clients to explicitly register for using 
   the IOCTL-based event registration mechanism
---------------------------------------------------------------------------*/
typedef void (*dcm_iface_ioctl_event_cb_fcn)
(
  int                              dcm_nethandle,
  dcm_iface_id_t                   iface_id,
  dcm_iface_ioctl_event_t        * event,
  void                           * user_data
);
/*~ PARAM IN event POINTER */

/*--------------------------------------------------------------------------- 
   Type representing a type of IFACE in client's network policy
---------------------------------------------------------------------------*/
typedef struct dcm_net_policy_iface_s {
    dcm_iface_id_kind_t kind;
    union
    {
      dcm_iface_id_t id;
      dcm_iface_id_name_t name;
    } info;
    /*~ FIELD dcm_net_policy_iface_s.info DISC dcm_net_policy_iface_s.kind */
    /*~ CASE DSS_IFACE_ID dcm_net_policy_iface_s.info.id */
    /*~ CASE DSS_IFACE_NAME dcm_net_policy_iface_s.info.name */
} dcm_net_policy_iface_t;

/*--------------------------------------------------------------------------- 
   Type representing client's network policy
---------------------------------------------------------------------------*/
typedef struct dcm_net_policy_info_s {
    dss_iface_policy_flags_enum_type policy_flag;
    dcm_net_policy_iface_t iface;
    dss_string_type username;
    dss_string_type password;
    dss_auth_pref_type auth_pref; /* none/pap/chap/both */
    dss_data_call_origin_type  data_call_origin;
    struct dcm_net_policy_info_umts_s
    {
      int pdp_profile_num;
      int im_cn_flag;                /* IM-CN flag for IMS               */
      dss_umts_apn_type apn;
    } umts;                              /* UMTS specific policy information */

    struct dcm_net_policy_info_cdma_s
    {
      int data_session_profile_id;
    } cdma;                 /* CDMA specific data session policy information */
} dcm_net_policy_info_t;

/*--------------------------------------------------------------------------- 
   Type representing an IOCTL
---------------------------------------------------------------------------*/
typedef struct dcm_iface_ioctl_s {
    dcm_iface_ioctl_name_t name;
    union {
        dss_iface_ioctl_ipv4_addr_type          ipv4_addr;
        dss_iface_ioctl_ipv4_prim_dns_addr_type ipv4_prim_dns_addr;
        dss_iface_ioctl_ipv4_seco_dns_addr_type ipv4_seco_dns_addr;
        dss_iface_ioctl_ipv4_gateway_addr_type  ipv4_gateway_addr;
        dss_iface_ioctl_state_type              if_state;
        dss_iface_ioctl_phys_link_state_type    phys_link_state;
        dss_iface_ioctl_iface_name_type         if_name;
        dcm_iface_ioctl_event_t                 event_info;
        dss_iface_ioctl_bind_sock_to_iface_type bind_sock_to_iface_info;
        dss_iface_ioctl_device_name_type        device_name_info;
        dss_iface_ioctl_data_bearer_tech_type   data_bearer_tech;
    } info;
    /*~ FIELD dcm_iface_ioctl_s.info DISC dcm_iface_ioctl_s.name */
    /*~ CASE DSS_IFACE_IOCTL_GET_IPV4_ADDR dcm_iface_ioctl_s.info.ipv4_addr */
    /*~ CASE DSS_IFACE_IOCTL_GET_IPV4_PRIM_DNS_ADDR dcm_iface_ioctl_s.info.ipv4_prim_dns_addr */
    /*~ CASE DSS_IFACE_IOCTL_GET_IPV4_SECO_DNS_ADDR dcm_iface_ioctl_s.info.ipv4_seco_dns_addr */
    /*~ CASE DSS_IFACE_IOCTL_GET_IPV4_GATEWAY_ADDR dcm_iface_ioctl_s.info.ipv4_gateway_addr */
    /*~ CASE DSS_IFACE_IOCTL_GET_STATE dcm_iface_ioctl_s.info.if_state */
    /*~ CASE DSS_IFACE_IOCTL_GET_PHYS_LINK_STATE dcm_iface_ioctl_s.info.phys_link_state */
    /*~ CASE DSS_IFACE_IOCTL_GET_IFACE_NAME dcm_iface_ioctl_s.info.if_name */
    /*~ CASE DSS_IFACE_IOCTL_REG_EVENT_CB dcm_iface_ioctl_s.info.event_info */
    /*~ CASE DSS_IFACE_IOCTL_DEREG_EVENT_CB dcm_iface_ioctl_s.info.event_info */
    /*~ CASE DSS_IFACE_IOCTL_BIND_SOCK_TO_IFACE dcm_iface_ioctl_s.info.bind_sock_to_iface_info */
    /*~ CASE DSS_IFACE_IOCTL_GET_DEVICE_NAME dcm_iface_ioctl_s.info.device_name_info */
} dcm_iface_ioctl_t;

/*===========================================================================
                            GLOBAL FUNCTION DECLARATIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  dcm_get_clnt_handle
===========================================================================*/
/*!
@brief
  Allocates a client handle for the client.

@return
  int - client handle if allocation is successful, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int
dcm_get_clnt_handle
(
    void
);
/*~ FUNCTION dcm_get_clnt_handle
    RELEASE_FUNC dcm_release_clnt_handle(_RESULT_) */

/*===========================================================================
  FUNCTION  dcm_release_clnt_handle
===========================================================================*/
/*!
@brief
  Deallocates a previously assigned client handle. 

@return
  dsc_op_status_t - DSC_OP_SUCCESS if client handle is released, DSC_OP_FAIL
                    otherwise.

@note

  - Dependencies
    - None

  - Side Effects
    - Releases all network handles opened by the client
*/
/*=========================================================================*/
dsc_op_status_t
dcm_release_clnt_handle
(
    int dcm_clnt_hdl
);
/*~ FUNCTION dcm_release_clnt_handle */

/*===========================================================================
  FUNCTION  dcm_get_net_handle
===========================================================================*/
/*!
@brief
  Allocates a network handle to the client. 

@return
  int - network handle if allocation is successful, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int
dcm_get_net_handle
(
    int                             dcm_clnt_hdl,
    dcm_net_cb_fcn                  net_cb,
    void                          * net_cb_user_data,
    dcm_iface_ioctl_event_cb_fcn    ev_cb,
    void                          * ev_cb_user_data,
    int                           * dss_errno
);
/*~ FUNCTION dcm_get_net_handle */
/*~ PARAM OUT dss_errno POINTER */

/*===========================================================================
  FUNCTION  dcm_release_net_handle
===========================================================================*/
/*!
@brief
  Deallocates a previously assigned network handle. 

@return
  int - network handle if allocation is successful, -1 otherwise

@note

  - Dependencies
    - Client must have closed network by calling dcm_net_close() and the 
      network handle must be unbound (i.e. not associated with any IFACE),
      otherwise the call will fail.

  - Side Effects
    - None
*/
/*=========================================================================*/
dsc_op_status_t
dcm_release_net_handle
(
    int         dcm_nethandle,
    int       * dss_errno
);
/*~ FUNCTION dcm_release_net_handle */
/*~ PARAM OUT dss_errno POINTER */

/*===========================================================================
  FUNCTION  dcm_net_open
===========================================================================*/
/*!
@brief
  Bring up network using the specified network policy. Note that if the 
  operation fails, the applicable error code is returned in integer pointed
  to by dss_errno. 

@return
  dsc_op_status_t - DSC_OP_SUCCESS if network is successfully brought up, 
                    DSC_OP_FAIL otherwise 
@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
dsc_op_status_t
dcm_net_open
(
    int                     dcm_nethandle,
    dcm_net_policy_info_t * net_policy,
    int                   * dss_errno
);
/*~ FUNCTION dcm_net_open */
/*~ PARAM IN net_policy POINTER */
/*~ PARAM OUT dss_errno POINTER */

/*===========================================================================
  FUNCTION  dcm_net_close
===========================================================================*/
/*!
@brief
  Bring down the network/IFACE that the specified network handle is bound to. 
  Note that if the operation fails, the applicable error code is returned in
  the integer pointed to by dss_errno. 

@return
  dsc_op_status_t - DSC_OP_SUCCESS if network is successfully brought down, 
                    DSC_OP_FAIL otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
dsc_op_status_t
dcm_net_close
(
    int                     dcm_nethandle,
    int                   * dss_errno
);
/*~ FUNCTION dcm_net_close */
/*~ PARAM OUT dss_errno POINTER */

/*===========================================================================
  FUNCTION  dcm_get_net_status
===========================================================================*/
/*!
@brief
  Queries the network state and returns the corresponding error code in the 
  variable pointed to by dss_errno. 

@return
  dsc_op_status_t - DSC_OP_SUCCESS if operation is successful, 
                    DSC_OP_FAIL otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
dsc_op_status_t
dcm_get_net_status
(
    int                     dcm_nethandle,
    int                   * dss_errno
);
/*~ FUNCTION dcm_get_net_status */
/*~ PARAM OUT dss_errno POINTER */

/*===========================================================================
  FUNCTION  dcm_get_iface_id
===========================================================================*/
/*!
@brief
  Returns the IFACE ID that the specified network handle is bound to. 

@return
  dcm_iface_id_t - IFACE ID if the network handle is bound to an IFACE,
                   DCM_IFACE_ID_INVALID otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
dcm_iface_id_t
dcm_get_iface_id
(
    int     dcm_nethandle
);
/*~ FUNCTION dcm_get_iface_id */

/*===========================================================================
  FUNCTION  dcm_get_iface_id_by_policy
===========================================================================*/
/*!
@brief
  Returns the IFACE ID of the IFACE compatible with (i.e. matching) the 
  specified network policy. 

@return
  dcm_iface_id_t - IFACE ID if an IFACE exists that matches the policy, 
                   DCM_IFACE_ID_INVALID otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
dcm_iface_id_t
dcm_get_iface_id_by_policy
(
    dcm_net_policy_info_t * net_policy,
    int                   * dss_errno
);
/*~ FUNCTION dcm_get_iface_id_by_policy */
/*~ PARAM IN net_policy POINTER */
/*~ PARAM OUT dss_errno POINTER */

/*===========================================================================
  FUNCTION  dcm_iface_ioctl
===========================================================================*/
/*!
@brief
  Generic IOCTL handler.  

@return
  dsc_op_status_t - DSC_OP_SUCCESS, if IOCTL is successfully processed, 
                    DSC_OP_FAIL otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
dsc_op_status_t
dcm_iface_ioctl
(
    dcm_iface_id_t           iface_id,
    dcm_iface_ioctl_t      * ioctl,
    int                    * dss_errno
);
/*~ FUNCTION dcm_iface_ioctl */
/*~ PARAM INOUT ioctl POINTER */
/*~ PARAM OUT dss_errno POINTER */

/*===========================================================================
  FUNCTION  dcm_debug_print_iface_array
===========================================================================*/
/*!
@brief
  debugging 

@return
  
@note

  - Dependencies
    - None

  - Side Effects
    - 
*/
/*=========================================================================*/
void 
dcm_debug_print_iface_array(void);  
/*===========================================================================
  FUNCTION  dsc_dcm_get_reason_code
===========================================================================*/
/*!
@brief
    Returns the last netdown reason code from the nethandle info structure.
@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int     
dsc_dcm_get_reason_code
(       
  int net_hdl,
  int *reason_code
);
#endif /* __DSC_DCM_H__ */
