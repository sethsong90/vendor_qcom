/******************************************************************************

                      D S C _ Q M I _ W D S . H

******************************************************************************/

/******************************************************************************

  @file    dsc_qmi_wds.h
  @brief   DSC's QMI Driver Interface for WDS Services header file

  DESCRIPTION
  Header file for DSC's QMI Driver interface for WDS services.

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2010 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //source/qcom/qct/platform/linux/common/main/latest/apps/dsc/src/dsc_qmi_wds.h#5 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
04/08/09   SM         Added Support for physlink Evnt Indications
03/24/09   SM         Added Call End Reason Code Support
05/28/08   vk         Added support for APN override
03/15/08   vk         Incorporated code review comments
11/17/07   vk         Added function headers and other comments
09/28/07   vk         Initial version

******************************************************************************/

#ifndef __DSC_QMI_WDS_H__
#define __DSC_QMI_WDS_H__

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/
#include "comdef.h"
#include "dssocket_defs_linux.h"
#include "dsci.h"
#include "qmi.h"
#include "dsc_dcmi.h"
#include "qmi_wds_srvc.h"

/*===========================================================================
                     GLOBAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/*--------------------------------------------------------------------------- 
 PS Spec defined NETDOWN REASON CODES taken from cdps/ps_iface_defs.h
 These defines are used for reverse mapping qmi cause codes to ps cause codes.
---------------------------------------------------------------------------*/

#define DSC_PS_NET_DOWN_REASON_NOT_SPECIFIED             0 /* Reason not known */
#define DSC_PS_NET_DOWN_REASON_CLOSE_IN_PROGRESS          1
#define DSC_PS_NET_DOWN_REASON_NW_INITIATED_TERMINATION    2
#define DSC_PS_NET_DOWN_REASON_OPERATOR_DETERMINED_BARRING 8
#define DSC_PS_NET_DOWN_REASON_LLC_SNDCP_FAILURE            25
#define DSC_PS_NET_DOWN_REASON_INSUFFICIENT_RESOURCES    26
#define DSC_PS_NET_DOWN_REASON_UNKNOWN_APN               27
#define DSC_PS_NET_DOWN_REASON_UNKNOWN_PDP               28
#define DSC_PS_NET_DOWN_REASON_AUTH_FAILED               29
#define DSC_PS_NET_DOWN_REASON_GGSN_REJECT               30
#define DSC_PS_NET_DOWN_REASON_ACTIVATION_REJECT         31
#define DSC_PS_NET_DOWN_REASON_OPTION_NOT_SUPPORTED      32
#define DSC_PS_NET_DOWN_REASON_OPTION_UNSUBSCRIBED       33
#define DSC_PS_NET_DOWN_REASON_OPTION_TEMP_OOO           34
#define DSC_PS_NET_DOWN_REASON_NSAPI_ALREADY_USED        35
#define DSC_PS_NET_DOWN_REASON_REGULAR_DEACTIVATION      36
#define DSC_PS_NET_DOWN_REASON_QOS_NOT_ACCEPTED          37
#define DSC_PS_NET_DOWN_REASON_NETWORK_FAILURE           38
#define DSC_PS_NET_DOWN_REASON_UMTS_REATTACH_REQ         39
#define DSC_PS_NET_DOWN_REASON_FEATURE_NOT_SUPPORTED     40
#define DSC_PS_NET_DOWN_REASON_TFT_SEMANTIC_ERROR        41
#define DSC_PS_NET_DOWN_REASON_TFT_SYNTAX_ERROR          42
#define DSC_PS_NET_DOWN_REASON_UNKNOWN_PDP_CONTEXT       43
#define DSC_PS_NET_DOWN_REASON_FILTER_SEMANTIC_ERROR     44
#define DSC_PS_NET_DOWN_REASON_FILTER_SYNTAX_ERROR       45
#define DSC_PS_NET_DOWN_REASON_PDP_WITHOUT_ACTIVE_TFT    46
#define DSC_PS_NET_DOWN_REASON_INVALID_TRANSACTION_ID        81
#define DSC_PS_NET_DOWN_REASON_MESSAGE_INCORRECT_SEMANTIC    95
#define DSC_PS_NET_DOWN_REASON_INVALID_MANDATORY_INFO        96
#define DSC_PS_NET_DOWN_REASON_MESSAGE_TYPE_UNSUPPORTED      97
#define DSC_PS_NET_DOWN_REASON_MSG_TYPE_NONCOMPATIBLE_STATE  98
#define DSC_PS_NET_DOWN_REASON_UNKNOWN_INFO_ELEMENT          99
#define DSC_PS_NET_DOWN_REASON_CONDITIONAL_IE_ERROR          100
#define DSC_PS_NET_DOWN_REASON_MSG_AND_PROTOCOL_STATE_UNCOMPATIBLE  101
#define DSC_PS_NET_DOWN_REASON_PROTOCOL_ERROR            111
#define DSC_PS_NET_DOWN_REASON_APN_TYPE_CONFLICT         112
#define DSC_PS_NET_DOWN_REASON_UNKNOWN_CAUSE_CODE

#define DSC_PS_NET_DOWN_REASON_INTERNAL_MIN                  200
#define DSC_PS_NET_DOWN_REASON_INTERNAL_ERROR                201
#define DSC_PS_NET_DOWN_REASON_INTERNAL_CALL_ENDED           202
#define DSC_PS_NET_DOWN_REASON_INTERNAL_UNKNOWN_CAUSE_CODE   203
#define DSC_PS_NET_DOWN_REASON_INTERNAL_MAX                  204

/* To map CDMA specific call-end reasons from CM */
#define DSC_PS_NET_DOWN_REASON_CDMA_LOCK                  500
#define DSC_PS_NET_DOWN_REASON_INTERCEPT                  501
#define DSC_PS_NET_DOWN_REASON_REORDER                    502
#define DSC_PS_NET_DOWN_REASON_REL_SO_REJ                 503
#define DSC_PS_NET_DOWN_REASON_INCOM_CALL                 504
#define DSC_PS_NET_DOWN_REASON_ALERT_STOP                 505
#define DSC_PS_NET_DOWN_REASON_ACTIVATION                 506
#define DSC_PS_NET_DOWN_REASON_MAX_ACCESS_PROBE           507
#define DSC_PS_NET_DOWN_REASON_CCS_NOT_SUPPORTED_BY_BS    508
#define DSC_PS_NET_DOWN_REASON_NO_RESPONSE_FROM_BS        509
#define DSC_PS_NET_DOWN_REASON_REJECTED_BY_BS             510
#define DSC_PS_NET_DOWN_REASON_INCOMPATIBLE               511
#define DSC_PS_NET_DOWN_REASON_ALREADY_IN_TC              512
#define DSC_PS_NET_DOWN_REASON_USER_CALL_ORIG_DURING_GPS  513
#define DSC_PS_NET_DOWN_REASON_USER_CALL_ORIG_DURING_SMS  514
#define DSC_PS_NET_DOWN_REASON_NO_CDMA_SRV                515

/* To map GSM/WCDMA specific call-end reasons from CM */
#define DSC_PS_NET_DOWN_REASON_CONF_FAILED                1000
#define DSC_PS_NET_DOWN_REASON_INCOM_REJ                  1001
#define DSC_PS_NET_DOWN_REASON_NO_GW_SRV                  1002
#define DSC_PS_NET_DOWN_REASON_NO_GPRS_CONTEXT            1003
#define DSC_PS_NET_DOWN_REASON_ILLEGAL_MS                 1004
#define DSC_PS_NET_DOWN_REASON_ILLEGAL_ME                 1005
#define DSC_PS_NET_DOWN_REASON_GPRS_SERVICES_AND_NON_GPRS_SERVICES_NOT_ALLOWED  1006
#define DSC_PS_NET_DOWN_REASON_GPRS_SERVICES_NOT_ALLOWED  1007
#define DSC_PS_NET_DOWN_REASON_MS_IDENTITY_CANNOT_BE_DERIVED_BY_THE_NETWORK  1008
#define DSC_PS_NET_DOWN_REASON_IMPLICITLY_DETACHED  1009
#define DSC_PS_NET_DOWN_REASON_PLMN_NOT_ALLOWED     1010
#define DSC_PS_NET_DOWN_REASON_LA_NOT_ALLOWED       1011
#define DSC_PS_NET_DOWN_REASON_GPRS_SERVICES_NOT_ALLOWED_IN_THIS_PLMN  1012
#define DSC_PS_NET_DOWN_REASON_PDP_DUPLICATE  1013
#define DSC_PS_NET_DOWN_REASON_UE_RAT_CHANGE  1014

/* To map HDR specific call-end reasons from CM */
#define DSC_PS_NET_DOWN_REASON_CD_GEN_OR_BUSY             1500
#define DSC_PS_NET_DOWN_REASON_CD_BILL_OR_AUTH            1501
#define DSC_PS_NET_DOWN_REASON_CHG_HDR                    1502
#define DSC_PS_NET_DOWN_REASON_EXIT_HDR                   1503
#define DSC_PS_NET_DOWN_REASON_HDR_NO_SESSION             1504
#define DSC_PS_NET_DOWN_REASON_HDR_ORIG_DURING_GPS_FIX    1505
#define DSC_PS_NET_DOWN_REASON_HDR_CS_TIMEOUT             1506
#define DSC_PS_NET_DOWN_REASON_HDR_RELEASED_BY_CM         1507

/* To map technology-agnostic call-end reasons from CM */
#define DSC_PS_NET_DOWN_REASON_CLIENT_END                2000
#define DSC_PS_NET_DOWN_REASON_NO_SRV                    2001
#define DSC_PS_NET_DOWN_REASON_FADE                      2002
#define DSC_PS_NET_DOWN_REASON_REL_NORMAL                2003
#define DSC_PS_NET_DOWN_REASON_ACC_IN_PROG               2004
#define DSC_PS_NET_DOWN_REASON_ACC_FAIL                  2005
#define DSC_PS_NET_DOWN_REASON_REDIR_OR_HANDOFF          2006

  /* EAP error codes. Start from 5000 */
#define DSC_PS_NET_DOWN_REASON_EAP_MIN  5000 /* Do not use. Used for bounds check. */
#define DSC_PS_NET_DOWN_REASON_EAP_CLIENT_ERR_UNABLE_TO_PROCESS
#define DSC_PS_NET_DOWN_REASON_EAP_CLIENT_ERR_UNSUPPORTED_VERS
#define DSC_PS_NET_DOWN_REASON_EAP_CLIENT_ERR_INSUFFICIENT_CHALLANGES
#define DSC_PS_NET_DOWN_REASON_EAP_CLIENT_ERR_RAND_NOT_FRESH
#define DSC_PS_NET_DOWN_REASON_EAP_NOTIFICATION_GENERAL_FAILURE_AFTER_AUTH /*EAP 0*/
#define DSC_PS_NET_DOWN_REASON_EAP_NOTIFICATION_GENERAL_FAILURE_BEFORE_AUTH /*EAP 16384*/
#define DSC_PS_NET_DOWN_REASON_EAP_NOTIFICATION_TEMP_DENIED_ACCESS /*EAP 1026*/
#define DSC_PS_NET_DOWN_REASON_EAP_NOTIFICATION_USER_NOT_SUBSCRIBED /*EAP 1031*/
#define DSC_PS_NET_DOWN_REASON_EAP_SUCCESS /*EAP 32768*/
#define DSC_PS_NET_DOWN_REASON_EAP_NOTIFICATION_REALM_UNAVAILABLE
#define DSC_PS_NET_DOWN_REASON_EAP_NOTIFICATION_USER_NAME_UNAVAILABLE
#define DSC_PS_NET_DOWN_REASON_EAP_NOTIFICATION_CALL_BARRED
#define DSC_PS_NET_DOWN_REASON_EAP_MAX /* Do not use. Used for bounds check. */

  /* IPSEC Error Codes. Start from 5100*/
#define DSC_PS_NET_DOWN_REASON_IPSEC_MIN  5100 /* Do not use. Used for bounds check. */

/*--------------------------------------------------------------------------- 
   Type representing primary data call parameters
---------------------------------------------------------------------------*/
typedef enum
{
  DSC_PRICALL_UMTS,
  DSC_PRICALL_CDMA
} dsc_pricall_params_sys_type;

/* #define QMI_WDS_MAX_APN_STR_SIZE    DSS_UMTS_APN_MAX_LEN */
#define DSC_QMI_WDS_MAX_APN_STR_SIZE        DSS_UMTS_APN_MAX_LEN
#define DSC_QMI_WDS_MAX_STR_SIZE            DSS_STRING_MAX_LEN 

#if ((DSC_QMI_WDS_MAX_APN_STR_SIZE >= QMI_WDS_MAX_APN_STR_SIZE) || (DSC_QMI_WDS_MAX_STR_SIZE >= QMI_WDS_MAX_USERNAME_PASS_STR_SIZE)) 
#error "DSS APN/USERNAME_PASS string length cannot be greater than the corresponding QMI string lengths."
#endif

/* initial value for cdma profile id */
#define DSS_CDMA_PROFILE_NOT_SPEC   -1

typedef struct {
  unsigned char length;
  char value[DSC_QMI_WDS_MAX_STR_SIZE];
} qmi_wds_string_type;

typedef struct {
    dsc_pricall_params_sys_type system_flag;
    qmi_wds_string_type username;
    qmi_wds_string_type password;
    qmi_wds_auth_pref_type auth_pref;
    unsigned char auth_pref_enabled;
    qmi_wds_data_call_origin_type data_call_origin;
    /* UMTS specific parameters*/
    struct dsc_pricall_params_umts_s
    {
      int  profile_id;
      int  apn_length;
      char apn_name[DSC_QMI_WDS_MAX_APN_STR_SIZE];
    } umts;                             

    /* CDMA specific parameters*/
    struct dsc_pricall_params_cdma_s
    {
      int profile_id;
    } cdma;       
} dsc_pricall_params_t;


typedef qmi_wds_ce_reason_legacy_type  dsc_qmi_call_end_reason_type;

/*--------------------------------------------------------------------------- 
   Type of callback function registered with QMI Driver Interface that 
   serves to notify the client of confirmation of Start Network Interface
---------------------------------------------------------------------------*/
typedef void (* dsc_wds_start_interface_cnf_f) 
(
    int link, 
    dsc_op_status_t status,
    dsc_qmi_call_end_reason_type    call_end_reason,
    void * clnt_hdl
);

/*--------------------------------------------------------------------------- 
   Type of callback function registered with QMI Driver Interface that 
   serves to notify the client of confirmation of Stop Network Interface
---------------------------------------------------------------------------*/
typedef void (* dsc_wds_stop_interface_cnf_f)  
(
    int link, 
    dsc_op_status_t status,
    void * clnt_hdl
);

/*--------------------------------------------------------------------------- 
   Type of callback function registered with QMI Driver Interface that 
   serves to notify the client of incoming Stop Network Interface indication
---------------------------------------------------------------------------*/
typedef void (* dsc_wds_stop_interface_ind_f) (int link, dsc_qmi_call_end_reason_type call_end_reason, void * clnt_hdl);

/*--------------------------------------------------------------------------- 
   Type of callback function registered with QMI Driver Interface that 
   serves to notify the client of incoming Event Report indication
---------------------------------------------------------------------------*/
typedef void (* dsc_wds_event_report_ind_f) (int link,qmi_wds_dorm_status_type dorm_status, void *clnt_hdl);

/*--------------------------------------------------------------------------- 
   Type of callback function registered with QMI Driver Interface that 
   handles the reconfig_required pkt_srvc_ind
---------------------------------------------------------------------------*/
typedef void (* dsc_wds_reconfig_required_f) (int link, void * clnt_hdl);

/*--------------------------------------------------------------------------- 
   Type of callback object registered with QMI Driver Interface that 
   serves to notify the client of QMI WDS events (responses & indications)
---------------------------------------------------------------------------*/
typedef struct {
    dsc_wds_start_interface_cnf_f start_if_cnf_cb;
    dsc_wds_stop_interface_cnf_f  stop_if_cnf_cb;
    dsc_wds_stop_interface_ind_f  stop_if_ind_cb;
    dsc_wds_event_report_ind_f    event_report_ind_cb;
    dsc_wds_reconfig_required_f   reconfig_required_cb;
} dsc_wds_int_clntcb_t;

typedef qmi_wds_dorm_status_type  dsc_qmi_dorm_status_type;
typedef dss_iface_ioctl_data_bearer_tech_type   dsc_wds_data_bearer_tech_t;
/*===========================================================================
                            GLOBAL FUNCTION DECLARATIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  dsc_wds_reserve_interface
===========================================================================*/
/*!
@brief
 Reserves a QMI Interface (i.e. QMI Link) for use by the client. Client 
 registers its callback object to be notified of network events. 

@return
  int - QMI Link ID if successful, -1 otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
int dsc_wds_reserve_interface 
(
    const dsc_wds_int_clntcb_t * clntcb,
    void * clnt_hdl
);

/*===========================================================================
  FUNCTION  dsc_wds_unreserve_interface
===========================================================================*/
/*!
@brief
 Unreserves a QMI Interface (i.e. QMI Link) previously reserved. 

@return
  void

@note

  - Dependencies
    - Assumes interface is in DOWN state.  

  - Side Effects
    - None
*/
/*=========================================================================*/
void dsc_wds_unreserve_interface (int link);

/*===========================================================================
  FUNCTION  dsc_wds_start_interface_req
===========================================================================*/
/*!
@brief
 Initiates signaling to bring up a specified QMI Data Interface. 

@return
  dsc_op_status_t - DSC_OP_SUCCESS if QMI Driver is successfully invoked to 
                    bring up the interface, DSC_OP_FAIL otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
dsc_op_status_t dsc_wds_start_interface_req 
(
    int link, 
    dsc_pricall_params_t * params
);

/*===========================================================================
  FUNCTION  dsc_wds_stop_interface_req
===========================================================================*/
/*!
@brief
 Initiates signaling to bring down a specified QMI Data Interface. 

@return
  dsc_op_status_t - DSC_OP_SUCCESS if QMI Driver is successfully invoked to 
                    bring down the interface, DSC_OP_FAIL otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
dsc_op_status_t dsc_wds_stop_interface_req (int link);

/*===========================================================================
  FUNCTION  dsc_qmi_init
===========================================================================*/
/*!
@brief
 Main initialization routine of the QMI WDS Interface module. 

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - Initializes the QMI Driver
*/
/*=========================================================================*/
void dsc_qmi_init (int nlink, int links[]);

/*===========================================================================
  FUNCTION  dsc_wds_query_profile
===========================================================================*/
/*!
@brief
 Queries Data Profile parameters for the specified profile ID.

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - Blocks execution of thread until the QMI message exchange to get the 
      profile parameters is complete.
*/
/*=========================================================================*/
int 
dsc_wds_query_profile 
(
    qmi_wds_profile_id_type     * qmi_prof_id,
    qmi_wds_profile_params_type * qmi_prof_params
);

/*===========================================================================
  FUNCTION  dsc_qmi_ioctl
===========================================================================*/
/*!
@brief
  Generic IOCTL handler of the qmi module. 

@return
  int - 0 if IOCTL is successfully processed, -1 otherwise

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
int 
dsc_qmi_ioctl 
(
  int lnk, 
  dsc_dcm_iface_ioctl_t * iface_ioctl
);
#endif /* __DSC_QMI_WDS_H__ */
