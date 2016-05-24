/******************************************************************************

                      D S C _ Q M I _ W D S . C

******************************************************************************/

/******************************************************************************

  @file    dsc_qmi_wds.c
  @brief   DSC's QMI Driver Interface for WDS Services

  DESCRIPTION
  Implementation of DSC's QMI Driver interface for WDS services.

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2011 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //source/qcom/qct/platform/linux/common/main/latest/apps/dsc/src/dsc_qmi_wds.c#8 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
08/06/10   AR         Added supprot for DSS_IFACE_IOCTL_GET_MTU
04/08/09   SM         Added Support for physlink Evnt Indications
03/24/09   SM         Added Call End Reason Code Support
05/28/08   vk         Added support for APN override
04/07/08   vk         Waiting for packet service status indication before 
                      indicating stop cnf to client
03/15/08   vk         Incorporated code review comments
11/30/07   vk         Cleaned up lint warnings
11/17/07   vk         Added function headers and other comments
09/28/07   vk         Initial version

******************************************************************************/

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include <arpa/inet.h>
#include "dsci.h"
#include "dsc_util.h"
#include "ds_list.h"
#include "dsc_cmd.h"
#include "dsc_qmi_wds.h"
#include "dsc_qmi_nasi.h"

/*===========================================================================
                     LOCAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/*--------------------------------------------------------------------------- 
   Constant representing maximum possible number of QMI Links
---------------------------------------------------------------------------*/
#define DSC_QMI_MAX_LINK DSC_MAX_PRICALL

/*--------------------------------------------------------------------------- 
   Constant representing an unreserved interface
---------------------------------------------------------------------------*/
#define DSC_WDS_INT_UNRES   0

/*--------------------------------------------------------------------------- 
   Constant representing a reserved interface
---------------------------------------------------------------------------*/
#define DSC_WDS_INT_RES     1

/*--------------------------------------------------------------------------- 
   Constant representing a reserved forever interface
---------------------------------------------------------------------------*/
#define DSC_WDS_INT_RES_FOREVER     2

/*--------------------------------------------------------------------------- 
   Constant representing maximum number of command buffers used by this 
   module
---------------------------------------------------------------------------*/
#define DSC_QMI_MAX_CMDS 8

/*--------------------------------------------------------------------------- 
   Type representing collection of configuration information for the module
---------------------------------------------------------------------------*/
typedef struct {
    int nlink; /* number of qmi links */
} dsc_qmi_cfg_t;

/*--------------------------------------------------------------------------- 
   Type representing enumeration of QMI WDS interface states
---------------------------------------------------------------------------*/
typedef enum {
    DSC_WDS_INT_DOWN        = 0, /* WDS Interface down */
    DSC_WDS_INT_STARTING    = 1, /* WDS Interface coming up */
    DSC_WDS_INT_UP          = 2, /* WDS Interface up */
    DSC_WDS_INT_STOPPING    = 3, /* WDS Interface going down */
    DSC_WDS_INT_ABORTING    = 4  /* WDS Interface aborting */
} dsc_wds_int_state_t;

/*--------------------------------------------------------------------------- 
   Type representing collection of control info related to the QMI Driver
---------------------------------------------------------------------------*/
typedef struct {
    int wds_clnt_hdl;     /* QMI WDS client handle */
    int start_if_req_tid; /* Transaction ID of Start Network If request. 
                             Used for abort request */
} dsc_wds_qmi_drv_info_t;

/*--------------------------------------------------------------------------- 
   Type representing collection of an interface's state information
---------------------------------------------------------------------------*/
typedef struct {
    int                          reserved; /* Flag indicating whether if is
                                              in use */
    dsc_wds_int_state_t          state;    /* State of the interface */
    const dsc_wds_int_clntcb_t * clntcb;   /* Client callback struct ptr */
    void                       * clnt_hdl; /* Client handle ptr */
    dsc_wds_qmi_drv_info_t       qmi_drv;  /* QMI Driver info */
} dsc_wds_int_info_t;

/*--------------------------------------------------------------------------- 
   Type of Command data for a QMI WDS Indication
---------------------------------------------------------------------------*/
typedef struct {
    int                             link;     /* Link id */
    qmi_wds_indication_id_type      ind_id;   /* Type of indication */
    qmi_wds_indication_data_type    ind_data; /* Message data */
} dsc_qmi_wds_ind_t;

/*--------------------------------------------------------------------------- 
   Type of Command data for a QMI WDS Start Network Response
---------------------------------------------------------------------------*/
typedef struct {
    int             link;       /* Link id */
    dsc_op_status_t status;     /* Success/Failure status */
    qmi_wds_call_end_reason_type ce_reason;  /*call_end_reason*/
} dsc_qmi_wds_start_nw_rsp_t;

/*--------------------------------------------------------------------------- 
   Type of Command data for a QMI WDS Abort Network Response
---------------------------------------------------------------------------*/
typedef dsc_qmi_wds_start_nw_rsp_t dsc_qmi_wds_abort_nw_rsp_t;

/*--------------------------------------------------------------------------- 
   Type representing enumeration of QMI WDS Interface Commands
---------------------------------------------------------------------------*/
typedef enum {
    DSC_QMI_CMD_MIN                         = 0,
    DSC_QMI_WDS_IND_CMD                     = DSC_QMI_CMD_MIN,
    DSC_QMI_WDS_START_INTERFACE_RSP_CMD,
    DSC_QMI_WDS_ABORT_INTERFACE_RSP_CMD,
    DSC_QMI_CMD_MAX
} dsc_qmi_cmd_type_t;

/*--------------------------------------------------------------------------- 
   Type of a QMI WDS Interface Command data
---------------------------------------------------------------------------*/
typedef struct dsc_qmi_cmd_data_s {
    dsc_qmi_cmd_type_t type; /* Command type */
    union {
        dsc_qmi_wds_ind_t               wds_ind;
        dsc_qmi_wds_start_nw_rsp_t      wds_start_nw_rsp;
        dsc_qmi_wds_abort_nw_rsp_t      wds_abort_nw_rsp;
    } value; /* Command data */
} dsc_qmi_cmd_data_t;

/*--------------------------------------------------------------------------- 
   Type representing a QMI WDS Interface Command
---------------------------------------------------------------------------*/
typedef struct dsc_qmi_cmd_s {
    dsc_cmd_t           cmd;      /* Command object */
    dsc_qmi_cmd_data_t  data;     /* QMI WDS command data */
    int                 tracker;  /* 1 if alloc, else 0 */
    ds_dll_el_t         frl_node; /* Memory for node in cmd free list */
} dsc_qmi_cmd_t;

/*--------------------------------------------------------------------------- 
   Collection of configuration information for the module
---------------------------------------------------------------------------*/
static dsc_qmi_cfg_t dsc_qmi_cfg;

/*--------------------------------------------------------------------------- 
   Array holding state information of interfaces
---------------------------------------------------------------------------*/
static dsc_wds_int_info_t dsc_wds_int_info[DSC_QMI_MAX_LINK];

/*--------------------------------------------------------------------------- 
   Const array providing a table for mapping QMI Link Id to QMI Port Number. 
   The former identifier is used locally within DSC. The latter is used by 
   the QMI Driver.
---------------------------------------------------------------------------*/
static const char* 
                dsc_qmi_link_to_conn_id_map[DSC_QMI_MAX_LINK] = 
{
    QMI_PORT_RMNET_0,
    QMI_PORT_RMNET_1,
    QMI_PORT_RMNET_2
};

/*--------------------------------------------------------------------------- 
   Collection of control information used for Command processing
---------------------------------------------------------------------------*/
struct {
    dsc_qmi_cmd_t   cmd_arr[DSC_QMI_MAX_CMDS]; /* Array of command objects */
    ds_dll_el_t   * frl_head; /* Head ptr of free list */
    ds_dll_el_t   * frl_tail; /* Tail ptr of free list */
    pthread_mutex_t mutx;     /* Mutex for protecting list operations */
} dsc_qmi_cmd_ctrl;

/*--------------------------------------------------------------------------- 
   Forward declaration of the virtual function used to execute Commands
---------------------------------------------------------------------------*/
static void dsc_qmi_cmd_exec (dsc_cmd_t * cmd, void * data);

/*--------------------------------------------------------------------------- 
   Forward declaration of the virtual function used to free Commands
---------------------------------------------------------------------------*/
static void dsc_qmi_cmd_free (dsc_cmd_t * cmd, void * data);

/*--------------------------------------------------------------------------- 
   Forward declaration of the function used to process QMI WDS Ind Command
---------------------------------------------------------------------------*/
static void 
dsc_wds_ind 
(
    int                                  link,
    qmi_wds_indication_id_type           ind_id, 
    const qmi_wds_indication_data_type * ind_data
);

/*--------------------------------------------------------------------------- 
   Forward declaration of the function used to process QMI WDS Start Network
   Interface Cnf Command
---------------------------------------------------------------------------*/
static void 
dsc_wds_start_interface_cnf 
(
  int link, 
  qmi_wds_call_end_reason_type ce_reason, 
  dsc_op_status_t status
);

/*--------------------------------------------------------------------------- 
   Forward declaration of the function used to process QMI WDS Abort Network
   Interface Cnf Command
---------------------------------------------------------------------------*/
static void dsc_wds_abort_interface_cnf (int link, dsc_op_status_t status);

/*--------------------------------------------------------------------------- 
   Inline accessor for getting the QMI Client ID for a given QMI Link
---------------------------------------------------------------------------*/
static __inline__ int 
dsc_qmi_wds_get_clnt_id (int link)
{
    return dsc_wds_int_info[link].qmi_drv.wds_clnt_hdl;
}

/*--------------------------------------------------------------------------- 
   Inline accessor for getting a pointer to the QMI Driver Control Info for 
   a given QMI Link 
---------------------------------------------------------------------------*/
static __inline__ dsc_wds_qmi_drv_info_t *
dsc_qmi_wds_get_qmi_drv_info (int link)
{
    return &dsc_wds_int_info[link].qmi_drv;
}

/*--------------------------------------------------------------------------- 
  QMI message library handle
---------------------------------------------------------------------------*/

static int qmi_handle = QMI_INVALID_CLIENT_HANDLE;

/*--------------------------------------------------------------------------- 
  QMI NAS related declarations
---------------------------------------------------------------------------*/

static int qmi_nas_client_handle;

static __inline__ int
dsc_qmi_nas_get_client_id(void)
{
  return qmi_nas_client_handle;
}

static __inline__ void
dsc_qmi_nas_set_client_id(int client_hndl)
{
  qmi_nas_client_handle = client_hndl;
}
/*--------------------------------------------------------------------------- 
  physlink dormnacy related declarations
---------------------------------------------------------------------------*/
#define DSC_QMI_WDS_INVALID     0

static short dorm_status[DSC_QMI_MAX_LINK];/*QMI_WDS_DORM_STATUS_DORMANT - Dormant,  
                                             QMI_WDS_DORM_STATUS_ACTIVE - Active,
                                             DSC_QMI_WDS_INVALID - Invalid
                                             */

/*===========================================================================
                            LOCAL FUNCTION DEFINITIONS
===========================================================================*/

static int qmi_client_list[3] = 
{
  0,
  0,
  0
};

void 
dsc_qmi_client_hdl_cleanup
(
  void
)
{
  int i=0,qmi_err; 
 // int *client_handle = (int*) arg2;
  int release_client;
  if (qmi_handle < 0)
  {
    dsc_log_high("Qmi message library was never initialized. \n");
    return;
  }
  for (;i<DSC_QMI_MAX_LINK;i++)
  {
    release_client = (int) *(qmi_client_list+i);
    dsc_log_high("Releasing the qmi_client_handle %d \n",release_client);
    if (release_client)
      qmi_wds_srvc_release_client(release_client,&qmi_err);
  }
  qmi_nas_srvc_release_client(dsc_qmi_nas_get_client_id(),&qmi_err);
  qmi_release(qmi_handle);        
}

/*--------------------------------------------------------------------------
  Utility Mapping routine for conversion from 
      qmi cause codes to ps spec defined cause codes
---------------------------------------------------------------------------*/
static int  
dsc_qmi_map_wds_call_end_reason
(
  qmi_wds_ce_reason_legacy_type call_end_reason_code
)
{
  switch (call_end_reason_code) 
  {
    case  QMI_WDS_CE_REASON_CLIENT_END:
      return DSC_PS_NET_DOWN_REASON_CLIENT_END;

    case QMI_WDS_CE_REASON_NO_SRV:
      return DSC_PS_NET_DOWN_REASON_NO_SRV;

    case QMI_WDS_CE_REASON_FADE:
      return DSC_PS_NET_DOWN_REASON_FADE; 

    case QMI_WDS_CE_REASON_REL_NORMAL:
      return DSC_PS_NET_DOWN_REASON_REL_NORMAL;

    case QMI_WDS_CE_REASON_ACC_IN_PROG:
      return DSC_PS_NET_DOWN_REASON_ACC_IN_PROG ;

    case QMI_WDS_CE_REASON_ACC_FAIL: 
      return DSC_PS_NET_DOWN_REASON_ACC_FAIL;

    case QMI_WDS_CE_REASON_REDIR_OR_HANDOFF:
      return DSC_PS_NET_DOWN_REASON_REDIR_OR_HANDOFF ;

    case QMI_WDS_CE_REASON_CLOSE_IN_PROGRESS :
      return DSC_PS_NET_DOWN_REASON_CLOSE_IN_PROGRESS;

    case QMI_WDS_CE_REASON_AUTH_FAILED :
      return DSC_PS_NET_DOWN_REASON_AUTH_FAILED;

    case QMI_WDS_CE_REASON_INTERNAL_CALL_END :
      return DSC_PS_NET_DOWN_REASON_INTERNAL_CALL_ENDED;

    case QMI_WDS_CE_REASON_CDMA_LOCK : 
      return DSC_PS_NET_DOWN_REASON_CDMA_LOCK;

    case QMI_WDS_CE_REASON_INTERCEPT :
      return DSC_PS_NET_DOWN_REASON_INTERCEPT;

    case QMI_WDS_CE_REASON_REORDER :
      return DSC_PS_NET_DOWN_REASON_REORDER;

    case QMI_WDS_CE_REASON_REL_SO_REJ :
      return DSC_PS_NET_DOWN_REASON_REL_SO_REJ;

    case QMI_WDS_CE_REASON_INCOM_CALL :
      return DSC_PS_NET_DOWN_REASON_INCOM_CALL;

    case QMI_WDS_CE_REASON_ALERT_STOP :
      return DSC_PS_NET_DOWN_REASON_ALERT_STOP;

    case QMI_WDS_CE_REASON_ACTIVATION :
      return DSC_PS_NET_DOWN_REASON_ACTIVATION;

    case QMI_WDS_CE_REASON_MAX_ACCESS_PROBE :
      return DSC_PS_NET_DOWN_REASON_MAX_ACCESS_PROBE;

    case QMI_WDS_CE_REASON_CCS_NOT_SUPPORTED_BY_BS :
      return DSC_PS_NET_DOWN_REASON_CCS_NOT_SUPPORTED_BY_BS;

    case QMI_WDS_CE_REASON_NO_RESPONSE_FROM_BS :
      return DSC_PS_NET_DOWN_REASON_NO_RESPONSE_FROM_BS;

    case QMI_WDS_CE_REASON_REJECTED_BY_BS :
      return DSC_PS_NET_DOWN_REASON_REJECTED_BY_BS ;

    case QMI_WDS_CE_REASON_INCOMPATIBLE :
      return DSC_PS_NET_DOWN_REASON_INCOMPATIBLE;

    case QMI_WDS_CE_REASON_ALREADY_IN_TC :
      return DSC_PS_NET_DOWN_REASON_ALREADY_IN_TC;

    case QMI_WDS_CE_REASON_USER_CALL_ORIG_DURING_GPS :
      return DSC_PS_NET_DOWN_REASON_USER_CALL_ORIG_DURING_GPS;

    case QMI_WDS_CE_REASON_USER_CALL_ORIG_DURING_SMS :
      return DSC_PS_NET_DOWN_REASON_USER_CALL_ORIG_DURING_SMS;
 
    case QMI_WDS_CE_REASON_NO_CDMA_SRV :
      return DSC_PS_NET_DOWN_REASON_NO_CDMA_SRV;

    case QMI_WDS_CE_REASON_CONF_FAILED :
      return DSC_PS_NET_DOWN_REASON_CONF_FAILED;

    case QMI_WDS_CE_REASON_INCOM_REJ :
      return DSC_PS_NET_DOWN_REASON_INCOM_REJ;

    case QMI_WDS_CE_REASON_NETWORK_END :
      return DSC_PS_NET_DOWN_REASON_NW_INITIATED_TERMINATION;

    case QMI_WDS_CE_REASON_NO_GW_SRV :
      return DSC_PS_NET_DOWN_REASON_NO_GW_SRV;

    case QMI_WDS_CE_REASON_LLC_SNDCP_FAILURE :
      return DSC_PS_NET_DOWN_REASON_LLC_SNDCP_FAILURE ;

    case QMI_WDS_CE_REASON_INSUFFICIENT_RESOURCES :
      return DSC_PS_NET_DOWN_REASON_INSUFFICIENT_RESOURCES ;

    case QMI_WDS_CE_REASON_OPTION_TEMP_OOO :
      return DSC_PS_NET_DOWN_REASON_OPTION_TEMP_OOO;

    case QMI_WDS_CE_REASON_NSAPI_ALREADY_USED :
      return DSC_PS_NET_DOWN_REASON_NSAPI_ALREADY_USED;

    case QMI_WDS_CE_REASON_REGULAR_DEACTIVATION :
      return DSC_PS_NET_DOWN_REASON_REGULAR_DEACTIVATION;

    case QMI_WDS_CE_REASON_NETWORK_FAILURE :
      return DSC_PS_NET_DOWN_REASON_NETWORK_FAILURE ;

    case QMI_WDS_CE_REASON_UMTS_REATTACH_REQ :
      return DSC_PS_NET_DOWN_REASON_UMTS_REATTACH_REQ;

    case QMI_WDS_CE_REASON_PROTOCOL_ERROR :
      return DSC_PS_NET_DOWN_REASON_PROTOCOL_ERROR;

    case QMI_WDS_CE_REASON_OPERATOR_DETERMINED_BARRING :
      return DSC_PS_NET_DOWN_REASON_OPERATOR_DETERMINED_BARRING;
        
    case QMI_WDS_CE_REASON_UNKNOWN_APN :
      return DSC_PS_NET_DOWN_REASON_UNKNOWN_APN ;

    case QMI_WDS_CE_REASON_UNKNOWN_PDP :
      return DSC_PS_NET_DOWN_REASON_UNKNOWN_PDP;
  
    case QMI_WDS_CE_REASON_GGSN_REJECT :
      return DSC_PS_NET_DOWN_REASON_GGSN_REJECT;

    case QMI_WDS_CE_REASON_ACTIVATION_REJECT :
      return DSC_PS_NET_DOWN_REASON_ACTIVATION_REJECT;

    case QMI_WDS_CE_REASON_OPTION_NOT_SUPPORTED :
      return DSC_PS_NET_DOWN_REASON_OPTION_NOT_SUPPORTED;

    case QMI_WDS_CE_REASON_OPTION_UNSUBSCRIBED :
      return DSC_PS_NET_DOWN_REASON_OPTION_UNSUBSCRIBED;

    case QMI_WDS_CE_REASON_QOS_NOT_ACCEPTED :
      return DSC_PS_NET_DOWN_REASON_QOS_NOT_ACCEPTED;

    case QMI_WDS_CE_REASON_TFT_SEMANTIC_ERROR :
      return DSC_PS_NET_DOWN_REASON_TFT_SEMANTIC_ERROR;

    case QMI_WDS_CE_REASON_TFT_SYNTAX_ERROR :
      return DSC_PS_NET_DOWN_REASON_TFT_SYNTAX_ERROR;

    case QMI_WDS_CE_REASON_UNKNOWN_PDP_CONTEXT :
      return DSC_PS_NET_DOWN_REASON_UNKNOWN_PDP_CONTEXT;

    case QMI_WDS_CE_REASON_FILTER_SEMANTIC_ERROR :
      return DSC_PS_NET_DOWN_REASON_FILTER_SEMANTIC_ERROR ;

    case QMI_WDS_CE_REASON_FILTER_SYNTAX_ERROR :
      return DSC_PS_NET_DOWN_REASON_FILTER_SYNTAX_ERROR ;

    case QMI_WDS_CE_REASON_PDP_WITHOUT_ACTIVE_TFT :
      return DSC_PS_NET_DOWN_REASON_PDP_WITHOUT_ACTIVE_TFT;

    case QMI_WDS_CE_REASON_INVALID_TRANSACTION_ID :
      return DSC_PS_NET_DOWN_REASON_INVALID_TRANSACTION_ID;

    case QMI_WDS_CE_REASON_MESSAGE_INCORRECT_SEMANTIC :
      return DSC_PS_NET_DOWN_REASON_MESSAGE_INCORRECT_SEMANTIC;

    case QMI_WDS_CE_REASON_INVALID_MANDATORY_INFO :
      return DSC_PS_NET_DOWN_REASON_INVALID_MANDATORY_INFO;

    case QMI_WDS_CE_REASON_MESSAGE_TYPE_UNSUPPORTED :
      return DSC_PS_NET_DOWN_REASON_MESSAGE_TYPE_UNSUPPORTED;

    case QMI_WDS_CE_REASON_MSG_TYPE_NONCOMPATIBLE_STATE :
      return DSC_PS_NET_DOWN_REASON_MSG_TYPE_NONCOMPATIBLE_STATE;

    case QMI_WDS_CE_REASON_UNKNOWN_INFO_ELEMENT :
      return DSC_PS_NET_DOWN_REASON_UNKNOWN_INFO_ELEMENT;

    case QMI_WDS_CE_REASON_CONDITIONAL_IE_ERROR :
      return DSC_PS_NET_DOWN_REASON_CONDITIONAL_IE_ERROR;

    case QMI_WDS_CE_REASON_MSG_AND_PROTOCOL_STATE_UNCOMPATIBLE :
      return DSC_PS_NET_DOWN_REASON_MSG_AND_PROTOCOL_STATE_UNCOMPATIBLE;

    case QMI_WDS_CE_REASON_APN_TYPE_CONFLICT :
      return DSC_PS_NET_DOWN_REASON_APN_TYPE_CONFLICT;

    case QMI_WDS_CE_REASON_NO_GPRS_CONTEXT :
      return DSC_PS_NET_DOWN_REASON_NO_GPRS_CONTEXT;

    case QMI_WDS_CE_REASON_FEATURE_NOT_SUPPORTED :
      return DSC_PS_NET_DOWN_REASON_FEATURE_NOT_SUPPORTED;

    case QMI_WDS_CE_REASON_ILLEGAL_MS :
      return DSC_PS_NET_DOWN_REASON_ILLEGAL_MS;

    case QMI_WDS_CE_REASON_ILLEGAL_ME :
      return DSC_PS_NET_DOWN_REASON_ILLEGAL_ME;

    case QMI_WDS_CE_REASON_GPRS_SERVICES_AND_NON_GPRS_SERVICES_NOT_ALLOWED :
      return DSC_PS_NET_DOWN_REASON_GPRS_SERVICES_AND_NON_GPRS_SERVICES_NOT_ALLOWED;

    case QMI_WDS_CE_REASON_GPRS_SERVICES_NOT_ALLOWED :
      return DSC_PS_NET_DOWN_REASON_GPRS_SERVICES_NOT_ALLOWED;

    case QMI_WDS_CE_REASON_MS_IDENTITY_CANNOT_BE_DERIVED_BY_THE_NETWORK :
      return DSC_PS_NET_DOWN_REASON_MS_IDENTITY_CANNOT_BE_DERIVED_BY_THE_NETWORK;

    case QMI_WDS_CE_REASON_IMPLICITLY_DETACHED :
      return DSC_PS_NET_DOWN_REASON_IMPLICITLY_DETACHED;

    case QMI_WDS_CE_REASON_PLMN_NOT_ALLOWED :
      return DSC_PS_NET_DOWN_REASON_PLMN_NOT_ALLOWED ;

    case QMI_WDS_CE_REASON_LA_NOT_ALLOWED :
      return DSC_PS_NET_DOWN_REASON_LA_NOT_ALLOWED ;

    case QMI_WDS_CE_REASON_GPRS_SERVICES_NOT_ALLOWED_IN_THIS_PLMN :
      return DSC_PS_NET_DOWN_REASON_GPRS_SERVICES_NOT_ALLOWED_IN_THIS_PLMN;

    case QMI_WDS_CE_REASON_PDP_DUPLICATE :
      return DSC_PS_NET_DOWN_REASON_PDP_DUPLICATE;

    case QMI_WDS_CE_REASON_UE_RAT_CHANGE :
      return DSC_PS_NET_DOWN_REASON_UE_RAT_CHANGE;

    case QMI_WDS_CE_REASON_CD_GEN_OR_BUSY :
      return DSC_PS_NET_DOWN_REASON_CD_GEN_OR_BUSY;

    case QMI_WDS_CE_REASON_CD_BILL_OR_AUTH :
      return DSC_PS_NET_DOWN_REASON_CD_BILL_OR_AUTH;

    case QMI_WDS_CE_REASON_CHG_HDR :
      return DSC_PS_NET_DOWN_REASON_CHG_HDR;

    case QMI_WDS_CE_REASON_EXIT_HDR :
      return DSC_PS_NET_DOWN_REASON_EXIT_HDR;

    case QMI_WDS_CE_REASON_HDR_NO_SESSION :
      return DSC_PS_NET_DOWN_REASON_HDR_NO_SESSION;

    case QMI_WDS_CE_REASON_HDR_ORIG_DURING_GPS_FIX :
      return DSC_PS_NET_DOWN_REASON_HDR_ORIG_DURING_GPS_FIX;

    case QMI_WDS_CE_REASON_HDR_CS_TIMEOUT :
      return DSC_PS_NET_DOWN_REASON_HDR_CS_TIMEOUT;

    case QMI_WDS_CE_REASON_HDR_RELEASED_BY_CM :
      return DSC_PS_NET_DOWN_REASON_HDR_RELEASED_BY_CM;

    default:
      return DSC_PS_NET_DOWN_REASON_NOT_SPECIFIED;

  } /* switch ( iface_call_end_reason ) */
} /* qmi_wdsi_map_wds_call_end_reason() */


/*--------------------------------------------------------------------------
 utility function to convert qmi bearer tech to dss bearer tech 
---------------------------------------------------------------------------*/

void
dsc_wds_convert_qmi_bearer_to_dss_bearer 
( 
  qmi_wds_data_bearer_tech_type    qmi_data_bearer_tech,
  dsc_wds_data_bearer_tech_t       *dss_data_bearer_tech 
)
{
  *dss_data_bearer_tech = DATA_BEARER_TECH_UNKNOWN;

  if (qmi_data_bearer_tech.current_db_nw == QMI_WDS_UMTS_TYPE)
  {
    if (qmi_data_bearer_tech.rat_mask.umts_rat_mask == UMTS_GPRS)
    {
      *dss_data_bearer_tech = DATA_BEARER_TECH_GPRS;
    }
    else if (qmi_data_bearer_tech.rat_mask.umts_rat_mask == UMTS_EDGE)
    {
      *dss_data_bearer_tech = DATA_BEARER_TECH_EDGE;
    }
    else if (qmi_data_bearer_tech.rat_mask.umts_rat_mask == UMTS_HSDPA)
    {
      *dss_data_bearer_tech = DATA_BEARER_TECH_HSDPA;
    }
    else if (qmi_data_bearer_tech.rat_mask.umts_rat_mask == UMTS_HSUPA)
    {
      *dss_data_bearer_tech = DATA_BEARER_TECH_HSUPA;
    }
    else if (qmi_data_bearer_tech.rat_mask.umts_rat_mask == UMTS_WCDMA)
    {
      *dss_data_bearer_tech = DATA_BEARER_TECH_UMTS;
    }
  }
  else if (qmi_data_bearer_tech.current_db_nw == QMI_WDS_CDMA_TYPE)
  {
    if (qmi_data_bearer_tech.rat_mask.cdma_rat_mask == CDMA_1X)
    {
      if (qmi_data_bearer_tech.db_so_mask.so_mask_1x == CDMA_1X_IS95)
      {
        *dss_data_bearer_tech = DATA_BEARER_TECH_IS95A;
      }
    }
    else if (qmi_data_bearer_tech.rat_mask.cdma_rat_mask == CDMA_EVDO_REV0)  
    {
      *dss_data_bearer_tech = DATA_BEARER_TECH_EVDO_0;
    }
    else if (qmi_data_bearer_tech.rat_mask.cdma_rat_mask == CDMA_EVDO_REVA)
    {
      *dss_data_bearer_tech = DATA_BEARER_TECH_EVDO_A;
    }
  }
  if (*dss_data_bearer_tech == DATA_BEARER_TECH_UNKNOWN)
  {
    dsc_log_err("dsc_wds_convert_qmi_bearer_to_dss_bearer: "
                "Technology could not be determined");
  }

  dsc_log_high("Current data bearer technology is %d",
               *dss_data_bearer_tech);
}


/*===========================================================================
  FUNCTION  dsc_qmi_cmd_init
===========================================================================*/
/*!
@brief
  Initializes the data structures used for command processing.

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static void 
dsc_qmi_cmd_init (void)
{
    ds_dll_el_t * node;
    dsc_qmi_cmd_t * cmdp;
    int i;

    /* Initialize the mutex used for protecting command list enqueue and 
    ** dequeue operations. 
    */
    (void)pthread_mutex_init(&dsc_qmi_cmd_ctrl.mutx, NULL);

    /* Initialize the free list of commands */
    node = ds_dll_init(NULL);
    ds_assert(node);

    /* Initialize free list's head and tail ptrs */
    dsc_qmi_cmd_ctrl.frl_head = node;
    dsc_qmi_cmd_ctrl.frl_tail = node;

    /* Iterate over the command object array, adding each to the free list */
    for (i = 0; i < DSC_QMI_MAX_CMDS; ++i) {
        cmdp = &dsc_qmi_cmd_ctrl.cmd_arr[i];

        /* Set command data ptr to point to the call cmd object */
        cmdp->cmd.data = cmdp;

        /* Set command handler function ptrs */
        cmdp->cmd.execute_f = dsc_qmi_cmd_exec;
        cmdp->cmd.free_f = dsc_qmi_cmd_free;

        /* Enqueue command on the free list */
        node = ds_dll_enq(node, &cmdp->frl_node, cmdp);
        dsc_qmi_cmd_ctrl.frl_tail = node;
    }

    return;
}

/*===========================================================================
  FUNCTION  dsc_qmi_cmd_alloc
===========================================================================*/
/*!
@brief
  Allocates a command buffer from the pool of unused buffers. 

@return
  dsc_qmi_cmd_t * - pointer to command buffer, if successful, 
                    NULL otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static dsc_qmi_cmd_t * 
dsc_qmi_cmd_alloc (void)
{
    dsc_qmi_cmd_t * cmd = NULL;
    ds_dll_el_t * node;

    /* Acquire command lock */
    ds_assert(pthread_mutex_lock(&dsc_qmi_cmd_ctrl.mutx) == 0);

    /* Dequeue command object from free list */
    node = ds_dll_deq
           (
               dsc_qmi_cmd_ctrl.frl_head, 
               &dsc_qmi_cmd_ctrl.frl_tail,
               (const void **)&cmd
           );

    /* For debug purposes, set tracker if valid command was obtained */
    if (node != NULL) {
        cmd->tracker = 1;
    }

    /* Release command lock */
    ds_assert(pthread_mutex_unlock(&dsc_qmi_cmd_ctrl.mutx) == 0);

    /* Return ptr to command, or NULL if none available */
    return cmd;
}

/*===========================================================================
  FUNCTION  dsc_qmi_cmd_release
===========================================================================*/
/*!
@brief
 Returns a command buffer to the pool of unused buffers. 

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static void 
dsc_qmi_cmd_release (dsc_qmi_cmd_t * cmd)
{
    ds_dll_el_t * node;

    /* Acquire command lock */
    ds_assert(pthread_mutex_lock(&dsc_qmi_cmd_ctrl.mutx) == 0);

    ds_assert(cmd);

    /* Unset tracker for debug purposes */
    cmd->tracker = 0;

    /* Enqueue command on the free list */
    node = ds_dll_enq(dsc_qmi_cmd_ctrl.frl_tail, &cmd->frl_node, cmd);
    dsc_qmi_cmd_ctrl.frl_tail = node;

    /* Release command lock */
    ds_assert(pthread_mutex_unlock(&dsc_qmi_cmd_ctrl.mutx) == 0);

    return;
}

/*===========================================================================
  FUNCTION  dsc_qmi_cmd_exec
===========================================================================*/
/*!
@brief
 Virtual function registered with the Command Thread to executes a QMI WDS 
 Interface command. 

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_qmi_cmd_exec (dsc_cmd_t * cmd, void * data)
{
    dsc_qmi_cmd_t * qmi_cmd;
    dsc_qmi_cmd_type_t cmd_type;

    /* Get qmi cmd ptr from user data ptr */
    qmi_cmd = (dsc_qmi_cmd_t *)data;

    /* Double check for debug purposes that the command is legit */
    ds_assert(&qmi_cmd->cmd == cmd);

    /* Get command type */
    cmd_type = qmi_cmd->data.type;

    /* Process based on command type */
    switch (cmd_type) {
    case DSC_QMI_WDS_IND_CMD:
        dsc_wds_ind
            (
                qmi_cmd->data.value.wds_ind.link,
                qmi_cmd->data.value.wds_ind.ind_id,
                &qmi_cmd->data.value.wds_ind.ind_data
            );
        break;
    case DSC_QMI_WDS_START_INTERFACE_RSP_CMD:
        dsc_wds_start_interface_cnf
            (
                qmi_cmd->data.value.wds_start_nw_rsp.link,
                qmi_cmd->data.value.wds_start_nw_rsp.ce_reason,
                qmi_cmd->data.value.wds_start_nw_rsp.status
            );
        break;
    case DSC_QMI_WDS_ABORT_INTERFACE_RSP_CMD:
        dsc_wds_abort_interface_cnf
            (
                qmi_cmd->data.value.wds_abort_nw_rsp.link,
                qmi_cmd->data.value.wds_abort_nw_rsp.status
            );
        break;
    default:
        dsc_log_err("received unknown command type %d\n", cmd_type);
        dsc_abort();
    }

    return;
}

/*===========================================================================
  FUNCTION  dsc_qmi_cmd_free
===========================================================================*/
/*!
@brief
 Virtual function registered with the Command Thread to free a QMI WDS 
 Interface command, after execution of the command is complete. 

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
dsc_qmi_cmd_free (dsc_cmd_t * cmd, void * data)
{
    dsc_qmi_cmd_t * qmi_cmd;

    /* Get qmi cmd ptr from user data ptr */
    qmi_cmd = (dsc_qmi_cmd_t *)data;

    /* Double check for debug purposes that the command is legit */
    ds_assert(&qmi_cmd->cmd == cmd);

    /* Free command object */
    dsc_qmi_cmd_release(qmi_cmd);

    return;
}

/*===========================================================================
  FUNCTION  dsc_qmi_verify_link
===========================================================================*/
/*!
@brief
 Helper function to verify that a given QMI link is valid. 

@return
  int - 0 if the link is valid, -1 otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static int
dsc_qmi_verify_link (int link)
{

    /* Range check */
    if ((link < 0) || (link >= DSC_QMI_MAX_LINK)) {
        return -1;
    }

    if (dsc_wds_int_info[link].reserved == DSC_WDS_INT_RES_FOREVER) {
        return -1;
    }

    return 0;
}

/*===========================================================================
  FUNCTION  dsc_qmi_get_conn_id_for_link
===========================================================================*/
/*!
@brief
 Helper function to return the QMI connection ID (port number) for a given
 QMI link. 

@return
  qmi_connection_id_type - QMI Connection ID for the link

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static const char*
dsc_qmi_get_conn_id_for_link (int link)
{
    /* Verify that link id is valid */
    if (dsc_qmi_verify_link(link) == 0) {
        /* Return qmi connection id for the link */
        return dsc_qmi_link_to_conn_id_map[link];
    }
    return NULL;
}

/*===========================================================================
  FUNCTION  dsc_qmi_get_default_conn_id
===========================================================================*/
/*!
@brief
 Helper function to return the default QMI connection ID (port number) 

 default here means the first link we find that is valid

@return
  on success, qmi_connection_id_type - QMI Connection ID for the link
  returns -1 otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static const char*
dsc_qmi_get_default_conn_id()
{
    int i=0;

    for (i=0; i<DSC_QMI_MAX_LINK; i++)
    {
        if (dsc_qmi_verify_link(i) == 0)
            return dsc_qmi_get_conn_id_for_link(i);
    }

    return NULL;
}

/*===========================================================================
  FUNCTION  dsc_wds_reset_int_info
===========================================================================*/
/*!
@brief
 Helper function to reset interface state information for a given QMI link. 

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_wds_reset_int_info (int link)
{
    /* Reset interface info */
    dsc_wds_int_info[link].reserved = DSC_WDS_INT_UNRES;
    dsc_wds_int_info[link].state = DSC_WDS_INT_DOWN;
    dsc_wds_int_info[link].clntcb = NULL;
    dsc_wds_int_info[link].clnt_hdl = NULL;

    return;
}

/*===========================================================================
  FUNCTION  dsc_wds_reserve_forever_int_info
===========================================================================*/
/*!
@brief
 Helper function to reserve interface state information for a given QMI link. 

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_wds_reserve_forever_int_info (int link)
{
    /* Reset interface info */
    dsc_wds_int_info[link].reserved = DSC_WDS_INT_RES_FOREVER;
    dsc_wds_int_info[link].state = DSC_WDS_INT_DOWN;
    dsc_wds_int_info[link].clntcb = NULL;
    dsc_wds_int_info[link].clnt_hdl = NULL;

    return;
}

/*===========================================================================
  FUNCTION  dsc_wds_init
===========================================================================*/
/*!
@brief
 Initializes WDS interface states. 

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_wds_init (int links[])
{
    int i;

    /* Iterate over the array of interfaces, initializing each one */
    for (i = 0; i < DSC_QMI_MAX_LINK; ++i) {
        if (links[i] == 1) 
        {
            /* Reset interface info */
            /* following interface is unreserved, hence
               can be used for subsequent data calls */
            dsc_wds_reset_int_info(i);
        }
        else
        {
            /* following interface is reserved forever at
               beginning, hence won't be used for
               subsequent data calls */
            dsc_wds_reserve_forever_int_info(i);
        }
    }
    return;
}

/*===========================================================================
  FUNCTION  dsc_qmi_wds_ind
===========================================================================*/
/*!
@brief
 Processes an incoming QMI WDS Indication. It posts a command to do the 
 required processing in the Command Thread context.

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static void 
dsc_qmi_wds_ind
( 
  int                            user_handle,
  qmi_service_id_type            service_id,
  void                         * user_data,
  qmi_wds_indication_id_type     ind_id,
  qmi_wds_indication_data_type * ind_data
)
{
    int link;
    dsc_qmi_cmd_t * cmd;
    (void)user_handle;
    
    /* Verify service id before proceeding */
    ds_assert(service_id == QMI_WDS_SERVICE);

    /* Get link id from user data ptr */
    link = (int)user_data;

    /* Verify link id */
    ds_assert(dsc_qmi_verify_link(link) == 0);

    /* Allocate a command object */
    cmd = dsc_qmi_cmd_alloc();
    ds_assert(cmd);

    /* Set command object parameters */
    cmd->data.type = DSC_QMI_WDS_IND_CMD;
    cmd->data.value.wds_ind.link = link;
    cmd->data.value.wds_ind.ind_id = ind_id;
    cmd->data.value.wds_ind.ind_data = *ind_data;

    /* Post command for processing in the command thread context */
    dsc_cmdq_enq(&cmd->cmd);

    return;
}

/*===========================================================================
  FUNCTION  dsc_qmi_wds_rsp
===========================================================================*/
/*!
@brief
 Processes an incoming QMI WDS Response. It posts a command of the appropriate
 type to do the required processing in the Command Thread context.

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static void 
dsc_qmi_wds_rsp
(
    int                          wds_clnt_id,
    qmi_service_id_type          service_id,
    int                          sys_err_code,
    int                          qmi_err_code,
    void                       * user_data,
    qmi_wds_async_rsp_id_type    rsp_id,
    qmi_wds_async_rsp_data_type* rsp_data
)
{
    int link;
    dsc_op_status_t status;
    dsc_qmi_cmd_t * cmd;

    /* Verify service id before proceeding */
    ds_assert(service_id == QMI_WDS_SERVICE);

    /* Get link id from user data ptr */
    link = (int)user_data;

    /* Verify link id */
    ds_assert(dsc_qmi_verify_link(link) == 0);

    /* Verify WDS client id */
    ds_assert(wds_clnt_id == dsc_qmi_wds_get_clnt_id(link));

    dsc_log_low("Rcvd Qmi Wds Rsp Id %d for link %d", rsp_id, link);

    /* Set status code in command based on qmi error code */
    if (sys_err_code == QMI_NO_ERR) {
        status = DSC_OP_SUCCESS;
    } else {
        status = DSC_OP_FAIL;
        dsc_log_high("Sys err code %d, qmi err code %d in rsp", 
                     sys_err_code, qmi_err_code);

    }

    dsc_log_low("Rcvd Qmi Wds Rsp Id %d, sys err code %d, qmi err code %d for link %d ", 
                rsp_id, sys_err_code, qmi_err_code, link);
    
    /* Allocate a command object */
    cmd = dsc_qmi_cmd_alloc();
    ds_assert(cmd);

    /* Populate command object based on type of response */
    switch (rsp_id) {
    case QMI_WDS_SRVC_START_NW_ASYNC_RSP_MSG:
        cmd->data.type = DSC_QMI_WDS_START_INTERFACE_RSP_CMD;
        cmd->data.value.wds_start_nw_rsp.link = link;
        cmd->data.value.wds_start_nw_rsp.status = status;
        cmd->data.value.wds_start_nw_rsp.ce_reason = rsp_data->start_nw_rsp.call_end_reason;
        break;
    case QMI_WDS_SRVC_ABORT_ASYNC_RSP_MSG:
        cmd->data.type = DSC_QMI_WDS_ABORT_INTERFACE_RSP_CMD;
        cmd->data.value.wds_abort_nw_rsp.link = link;
        cmd->data.value.wds_abort_nw_rsp.status = status;
        break;
    default:
        dsc_log_err("Unknown rsp_id %d received in dsc_qmi_wds_rsp", rsp_id);
        return;
    }

    /* Post command for processing in the command thread context */
    dsc_cmdq_enq(&cmd->cmd);

    return;
}

/*===========================================================================
  FUNCTION  dsc_wds_pkt_srvc_status_ind
===========================================================================*/
/*!
@brief
 Performs processing of an incoming WDS Packet Service Status Indication 
 message. This function is executed in the Command Thread context.

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_wds_pkt_srvc_status_ind
(
    int                                  link,
    const qmi_wds_indication_data_type * ind_data
)
{
    dsc_wds_int_info_t * int_info;
    qmi_wds_link_status_type link_status;
    void * clnt_hdl;
    qmi_wds_call_end_reason_type  call_end_reason;
    /* Verify link id */
    ds_assert(dsc_qmi_verify_link(link) == 0);

    /* Verify the contents of ind_data to be non NULL */
    ds_assert(ind_data != NULL);

    /* Get interface info ptr from link id */
    int_info = &dsc_wds_int_info[link];

    /* Get client handle ptr */
    clnt_hdl = int_info->clnt_hdl;

    /* Only link status indication we care about is disconnection */
    link_status = ind_data->pkt_srvc_status.link_status;

    if (link_status == QMI_WDS_PACKET_DATA_DISCONNECTED) {
        call_end_reason.legacy_reason = ind_data->pkt_srvc_status.\
          call_end_reason.legacy_reason;
        if (call_end_reason.legacy_reason < QMI_WDS_CE_REASON_UNSPECIFIED || 
            call_end_reason.legacy_reason > QMI_WDS_CE_REASON_MAX)
        {
          dsc_log_high("Invalid CALL_END_CODE received : %d\n", call_end_reason);
        }
        /* Process disconnection indication based on interface state */
        switch (int_info->state) {
        case DSC_WDS_INT_UP:
            /* Unsolicited link termination. Send stop indication to client and 
            ** transition to interface down state. 
            */
            int_info->state = DSC_WDS_INT_DOWN;
            (* int_info->clntcb->stop_if_ind_cb)(link, 
                dsc_qmi_map_wds_call_end_reason(call_end_reason.legacy_reason), clnt_hdl);
            break;
        case DSC_WDS_INT_ABORTING:
            /* Intentional fall through. Note that we do not expect to receive a
            ** call down indication in this state as we should have received a 
            ** call up first and transitioned to stopping state at that time. 
            ** Nevertheless, we change state anyway to down. 
            */
        case DSC_WDS_INT_STOPPING:
            /* As the client had also requested that the link be brought down, 
            ** send stop confirmation to client and transition to down state.
            */
            int_info->state = DSC_WDS_INT_DOWN;
            (* int_info->clntcb->stop_if_cnf_cb)(link, DSC_OP_SUCCESS, clnt_hdl);
            break;
        default:
            /* Ignore in all other states as this message is unexpected */
            dsc_log_err("dsc_wds_pkt_srvc_status_ind called in state %d\n",
                        int_info->state);
        }
    } else if (ind_data->pkt_srvc_status.reconfig_required == TRUE) {
        switch(int_info->state) {
        case DSC_WDS_INT_UP:
            /* notify dsc_call about reconfiguration */
            (* int_info->clntcb->reconfig_required_cb)(link, 
                                                       clnt_hdl);
            break;
        default:
            /* Ignore in all other states as this message is unexpected */
            dsc_log_err("dsc_wds_pkt_srvc_status_ind called in state %d\n",
                        int_info->state);
            break;
        }
    } else {
        dsc_log_high("we only support DATA_CALL_DISCONNECTED or reconfig_required" \
                      "pkt_srvc indications");
        dsc_log_high("Ignoring PKT SRVC STATUS IND with link status %d for" \
                     "link %d", link_status, link);
    }

    /* above logic in if..elseif..else is breaking instead of doing return 
     * i.e. it assumes there is no code executed after coming out of 
     * if..elseif..else blocks. If you intend to add code here, make sure
     * you know what you are doing
     */

    return;
}

/*===========================================================================
  FUNCTION  dsc_wds_event_report_ind
===========================================================================*/
/*!
@brief
 Performs processing of an incoming WDS Event Report Indication message. 
 This function is executed in the Command Thread context. This function
 currently reports 'physlink events' to clients.

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_wds_event_report_ind
(
    int                                  link,
    const qmi_wds_indication_data_type * ind_data
)
{
    dsc_wds_int_info_t * int_info;
    void * clnt_hdl;
    qmi_wds_dorm_status_type  dorm_status;
    /* Verify link id */
    
    ds_assert(dsc_qmi_verify_link(link) == 0);
    ds_assert(ind_data != NULL);
    /* Get interface info ptr from link id */
    int_info = &dsc_wds_int_info[link];

    /* Get client handle ptr */
    clnt_hdl = int_info->clnt_hdl;
    
    /* we also seem to getting data call  state change indication. 
     * We only care for dormancy indications.
     */
    if (!(ind_data->event_report.event_mask & QMI_WDS_EVENT_DORM_STATUS_IND))
    {
      dsc_log_high("Ignoring indication %x",ind_data->event_report.event_mask);
      return;
    }
    dorm_status = ind_data->event_report.dorm_status;

    switch (int_info->state) {
      case DSC_WDS_INT_UP:
        /*Call client call back to report events received on the link*/
        (* int_info->clntcb->event_report_ind_cb)(link, dorm_status, clnt_hdl);
        break;
    default:
        /* Ignore in all other states as this message is unexpected */
        dsc_log_err("Ignoring event_report_indication called in state %d\n",
                    int_info->state);
    }
  return;
}

/*===========================================================================
  FUNCTION  dsc_wds_ind
===========================================================================*/
/*!
@brief
 Performs processing of an incoming WDS Indication message. This function 
 is executed in the Command Thread context.

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static void 
dsc_wds_ind 
(
    int                                  link,
    qmi_wds_indication_id_type           ind_id, 
    const qmi_wds_indication_data_type * ind_data
)
{
    /* Verify link id is valid before proceeding */
    ds_assert(dsc_qmi_verify_link(link) == 0);
    
    /* Process based on indication type */
    switch (ind_id) {
    case QMI_WDS_SRVC_PKT_SRVC_STATUS_IND_MSG:
        /* Process packet service status indication */
        dsc_wds_pkt_srvc_status_ind(link, ind_data);
        break;
    case QMI_WDS_SRVC_EVENT_REPORT_IND_MSG:
        /* Process Event Report indication */
        dsc_wds_event_report_ind(link,ind_data);
        break;
    default:
        /* Ignore all other indications */
        dsc_log_high("Ignoring QMI WDS IND of type %d", ind_id);
    }

    return;
}

/*===========================================================================
  FUNCTION  dsc_wds_start_interface_cnf
===========================================================================*/
/*!
@brief
 Performs processing of an incoming Start Network Interface Response. This 
 function is executed in the Command Thread context.

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_wds_start_interface_cnf 
(
  int link, 
  qmi_wds_call_end_reason_type ce_reason, 
  dsc_op_status_t status
)
{
    dsc_wds_int_info_t * int_info;
    void * clnt_hdl;

    /* Verify that link id is valid before proceeding */
    ds_assert(dsc_qmi_verify_link(link) == 0);

    /* Get interface info ptr for link */
    int_info = &dsc_wds_int_info[link];

    /* Get client handle ptr */
    clnt_hdl = int_info->clnt_hdl;

    dsc_log_low("Processing qmi wds start nw if rsp for link %d\n", link);

    /* Process event based on interface state */
    switch (int_info->state) {
    case DSC_WDS_INT_STARTING:
        /* Transition to up state if response indicates success, otherwise 
        ** transition to down state. 
        */
        if (status == DSC_OP_SUCCESS) {
            int_info->state = DSC_WDS_INT_UP;
        } else {
            int_info->state = DSC_WDS_INT_DOWN;
        }

        /* Call client callback to send indication up */
        (* int_info->clntcb->start_if_cnf_cb)(link, 
                                              status, 
                                              dsc_qmi_map_wds_call_end_reason(ce_reason.legacy_reason), 
                                              clnt_hdl);
        break;
    case DSC_WDS_INT_ABORTING:
        /* In the middle of aborting the interface, but the interface came up
        ** before the modem processed the call abort request. Transition to 
        ** up state and fake client request to bring interface down. 
        */
        if (status == DSC_OP_SUCCESS) {
            int_info->state = DSC_WDS_INT_UP;
            ds_assert(dsc_wds_stop_interface_req(link) == 0);
        } else {
            /* Call did not come up. Transition to down state and call 
            ** client callback to indicate this.
            */
            int_info->state = DSC_WDS_INT_DOWN;
            (* int_info->clntcb->stop_if_cnf_cb)
            (
                link, 
                DSC_OP_SUCCESS,
                clnt_hdl
            );
        }
        break;
    default:
        /* Ignore message in all other states */
        dsc_log_err("dsc_wds_start_interface_cnf called in state %d\n",
                    int_info->state);
    }

    return;
}

/*===========================================================================
  FUNCTION  dsc_wds_abort_interface_cnf
===========================================================================*/
/*!
@brief
 Performs processing of an incoming Abort Network Interface Response. This 
 function is executed in the Command Thread context.

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static void 
dsc_wds_abort_interface_cnf (int link, dsc_op_status_t status)
{
    dsc_wds_int_info_t * int_info;
    void * clnt_hdl;

    /* Verify that link id is valid before proceeding */
    ds_assert(dsc_qmi_verify_link(link) == 0);

    /* Get interface info ptr for link */
    int_info = &dsc_wds_int_info[link];

    /* Get client handle ptr */
    clnt_hdl = int_info->clnt_hdl;

    dsc_log_low("Processing qmi wds abort rsp for link %d\n", link);

    /* Process based on current interface state */
    switch (int_info->state) {
    case DSC_WDS_INT_ABORTING:
        /* Waiting for a response to an abort request. If code indicates 
        ** success, transition to down state and call client's interface 
        ** stop confirmation handler. If code indicates failure, ignore. We
        ** should receive a start response with success, which is when we will
        ** bring the interface down. 
        */
        if (status == DSC_OP_SUCCESS) {
            int_info->state = DSC_WDS_INT_DOWN;
            (* int_info->clntcb->stop_if_cnf_cb)(link, status, clnt_hdl);
        } else {
            dsc_log_high("dsc_wds_abort_interface_cnf: abort failure received\n");
        }
        break;
    default:
        /* Ignore abort response in all other states */
        dsc_log_err("dsc_wds_abort_interface_cnf called in state %d\n",
                    int_info->state);
    }

    return;
}

/*===========================================================================
  FUNCTION  dsc_qmi_driver_init
===========================================================================*/
/*!
@brief
 Initializes the QMI Driver.

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_qmi_driver_init (int links[])
{
    int i;
    char * qmi_cid;
    int qmi_err;
    int wds_clnt_id;
    int nas_clnt_id;
    dsc_wds_qmi_drv_info_t * qmi_drv_info;
    qmi_wds_event_report_params_type event_report_params;
    int rc;
    event_report_params.param_mask = 0; 
    event_report_params.param_mask |= QMI_WDS_EVENT_DORM_STATUS_IND;
    const char * default_conn_id = NULL;

    qmi_handle = qmi_init(NULL,NULL);
    
    if (qmi_handle < 0) 
    {
      dsc_log_err("dsc_qmi_driver_init:Could not initialize qmi \n");
      dsc_abort();
    }    
    
    /* Iterate over the array of interfaces, initializing WDS client for each */
    for (i = 0; i < DSC_QMI_MAX_LINK; ++i) {

        /* continue to next link if this link is not enabled 
         *  i.e. we are not using default cfg, and dsc.cfg has 
         *  not enabled this link */
        if(links[i] == 0)
        {
            dsc_log_high("ignoring links[%d]=%d\n", i, links[i]);
            continue;
        }

        dsc_log_high("initing links[%d]=%d\n", i, links[i]);

        /* Get qmi connection id for the interface */
        qmi_cid = (char *)dsc_qmi_get_conn_id_for_link(i);

        dsc_log_high("qmi_cid = %s", qmi_cid);
        ds_assert(qmi_cid != NULL);

        /* Initialize qmi connection */
        if (qmi_connection_init(qmi_cid, &qmi_err) < 0) {
            dsc_log_err("dsc_qmi_driver_init: qmi_connection_init failed for qmi_cid %s\n", 
                        qmi_cid);
            dsc_abort();
        }

        /* Initialize WDS client */
        if ((wds_clnt_id = qmi_wds_srvc_init_client(qmi_cid, dsc_qmi_wds_ind, (void *)i, &qmi_err)) < 0) {
            dsc_log_err("dsc_qmi_driver_init: qmi_wds_srvc_init_client failed for qmi_cid %s with error %ld,\n", 
                        qmi_cid, (long int)qmi_err);
           dsc_abort();
        }
        qmi_client_list[i] = wds_clnt_id;
        dsc_log_high("The wds client id %d \n",wds_clnt_id);

        /* Save WDS client ID */
        qmi_drv_info = dsc_qmi_wds_get_qmi_drv_info(i);
        qmi_drv_info->wds_clnt_hdl = wds_clnt_id;

        /* Set Event Report for the current client ID */
        if ((rc = qmi_wds_set_event_report(wds_clnt_id,&event_report_params,&qmi_err) < 0))
        {
          dsc_log_err("dsc_qmi_driver_init: Set event report failed with  with error %ld\n", 
                       (long int)qmi_err);
          dsc_abort();
        }

    }

    /*Initialize a NAS client used to query the technology 
      on which the phone is currently camped*/
    if (dsc_qmi_cfg.nlink != 0)
    {
        if ((default_conn_id = dsc_qmi_get_default_conn_id()) == NULL)
        {
            dsc_log_err("dsc_qmi_driver_init failed. no connections enabled");
            dsc_abort();
        }

        if ((nas_clnt_id = qmi_nas_srvc_init_client(default_conn_id, NULL, NULL, &qmi_err)) < 0) 
        {
            dsc_log_err("dsc_qmi_driver_init: qmi_nas_srvc_init_client failed for qmi_cid %s with error %d\n", 
                        default_conn_id, qmi_err);
            dsc_abort();
        }

        /*save this NAS client Id for future 
          querying using this client*/
        dsc_qmi_nas_set_client_id(nas_clnt_id);
    }
  
    atexit(dsc_qmi_client_hdl_cleanup);

    return;
}

/*===========================================================================
                            GLOBAL FUNCTION DEFINITIONS
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
int 
dsc_wds_reserve_interface 
(
    const dsc_wds_int_clntcb_t * clntcb, 
    void * clnt_hdl
)
{
    int i;

    /* Find an unused interface */
    for (i = 0; i < DSC_QMI_MAX_LINK; ++i) {
        if (dsc_wds_int_info[i].reserved == DSC_WDS_INT_UNRES) {
            /* Got one. Reserve it and store client's callback struct ptr */
            dsc_wds_int_info[i].reserved = DSC_WDS_INT_RES;
            dsc_wds_int_info[i].clntcb = clntcb;
            dsc_wds_int_info[i].clnt_hdl = clnt_hdl;
            return i;
        }
    }
    return -1;
}

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
void 
dsc_wds_unreserve_interface (int link)
{
    /* Reset interface info */
    dsc_wds_reset_int_info(link);
    return;
}

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
dsc_op_status_t
dsc_wds_start_interface_req (int link, dsc_pricall_params_t * params)
{
    dsc_op_status_t status = DSC_OP_FAIL;
    dsc_wds_int_info_t * int_info;
    int wds_clnt_id;
    dsc_wds_qmi_drv_info_t * qmi_drv_info;
    int qmi_err;
    int tid;
    qmi_wds_start_nw_if_params_type start_nw_params;
    int apn_length;
    int length = 0;
    qmi_wds_call_end_reason_type    call_end_reason;
    dsc_log_high("In dsc_wds_start_interface_req for link %d", link);

    /* Verify link id before proceeding */
    if (dsc_qmi_verify_link(link) < 0) {
        dsc_log_err("dsc_wds_start_interface_req: invalid link %d\n", 
                    link);
        goto error;
    }

    /* Get interface info ptr for link */
    int_info = &dsc_wds_int_info[link];

    /* Process start request only if current interface state is down, otherwise
    ** ignore. 
    */
    switch (int_info->state) {
    case DSC_WDS_INT_DOWN:
        break;
    default:
        dsc_log_err("dsc_wds_start_interface_req called in state %d\n",
                    int_info->state);
        goto error;
    }

    /* Set start nw params to specified values */
    start_nw_params.params_mask = 0;

    /* copy username */
    if ((length = params->username.length) > 0) {
        if (length > QMI_WDS_MAX_USERNAME_PASS_STR_SIZE-1) {
            dsc_log_err("dsc_wds_start_interface_req: username length %d too long",
                        length);
            goto error;
        }
        memcpy
        (
            start_nw_params.username,
            params->username.value,
            length
        );
        start_nw_params.username[length] = '\0';
        start_nw_params.params_mask |= QMI_WDS_START_NW_USERNAME_PARAM;
    }

    /* copy password */
    if ((length = params->password.length) > 0) {
        if ( length > QMI_WDS_MAX_USERNAME_PASS_STR_SIZE-1) {
            dsc_log_err("dsc_wds_start_interface_req: password length %d too long",
                        length);
            goto error;
        }
        memcpy
        (
            start_nw_params.password,
            params->password.value,
            length
        );
        start_nw_params.password[length] = '\0';
        start_nw_params.params_mask |= QMI_WDS_START_NW_PASSWORD_PARAM;
    }

    /* copy auth */
    if (params->auth_pref_enabled) {
        start_nw_params.params_mask |= QMI_WDS_START_NW_AUTH_PREF_PARAM;
        start_nw_params.auth_pref = params->auth_pref;
    }

    if (params->system_flag == DSC_PRICALL_UMTS) {
        start_nw_params.params_mask |= QMI_WDS_START_NW_PROFILE_IDX_PARAM;
        start_nw_params.profile_index = (unsigned char)params->umts.profile_id;
        /* copy apn */
        if ((apn_length = params->umts.apn_length) > 0) {
            if (apn_length > QMI_WDS_MAX_APN_STR_SIZE-1) {
              dsc_log_err("dsc_wds_start_interface_req: APN length %d too long",
                          params->umts.apn_length);
              goto error;
            }
            start_nw_params.params_mask |= QMI_WDS_START_NW_APN_NAME_PARAM;

            memcpy
            (
                start_nw_params.apn_name,
                params->umts.apn_name, 
                apn_length
            );
            start_nw_params.apn_name[apn_length] = '\0';
        }
        start_nw_params.params_mask |= QMI_WDS_START_NW_TECH_PREF_PARAM;
        start_nw_params.tech_pref = QMI_WDS_START_NW_TECH_3GPP;
    } else if (params->system_flag == DSC_PRICALL_CDMA) {
       if (params->cdma.profile_id != DSS_CDMA_PROFILE_NOT_SPEC)
       {
          start_nw_params.params_mask |= QMI_WDS_START_NW_PROFILE_IDX_3GPP2_PARAM;  
          start_nw_params.profile_index_3gpp2 = (unsigned char)params->cdma.profile_id;
       }
       start_nw_params.params_mask |= QMI_WDS_START_NW_TECH_PREF_PARAM;
       start_nw_params.tech_pref = QMI_WDS_START_NW_TECH_3GPP2;
    } 

    /* 
     * At this point call params data_call_type is set to either of 
     * three possible types default/laptop/embedded
     */

    dsc_log_high("In dsc_wds_start_interface_req for link %d, \
                  data call origin type is %d", link, params->data_call_origin);
    start_nw_params.params_mask |= QMI_WDS_START_NW_DATA_CALL_ORIGIN_PARAM;
    start_nw_params.data_call_origin = params->data_call_origin;

    wds_clnt_id = dsc_qmi_wds_get_clnt_id(link);
    qmi_drv_info = dsc_qmi_wds_get_qmi_drv_info(link);

    
    /* Call QMI Client Driver API to start network interface */
    if ((tid = 
         qmi_wds_start_nw_if
        (
            wds_clnt_id, 
            &start_nw_params,
            dsc_qmi_wds_rsp, 
            (void *)link,
            &call_end_reason,
            &qmi_err
        )) < 0) 
    {
        dsc_log_err("qmi_wds_start_nw_if failed with error %ld\n", (long int)qmi_err);
        goto error;
    }

    /* Store TID of the message. This is required if the start request has 
    ** to be aborted. 
    */
    qmi_drv_info->start_if_req_tid = tid;

    /* Change interface state to starting */
    int_info->state = DSC_WDS_INT_STARTING;

    status = DSC_OP_SUCCESS;

error:
    return status;
}

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
dsc_op_status_t
dsc_wds_stop_interface_req (int link)
{
    dsc_op_status_t status = DSC_OP_FAIL;
    dsc_wds_int_info_t * int_info;
    int wds_clnt_id;
    dsc_wds_qmi_drv_info_t * qmi_drv_info;
    int qmi_err;

    dsc_log_high("In dsc_wds_stop_interface_req for link %d", link);

    /* Verify link id before proceeding */
    if (dsc_qmi_verify_link(link) < 0) {
        dsc_log_err("dsc_wds_stop_interface_req: invalid link %d\n", 
                    link);
        goto error;
    }

    /* Get interface info ptr for link */
    int_info = &dsc_wds_int_info[link];

    /* Process stop request only if current interface state is up or coming up,
    ** otherwise ignore. 
    */
    switch (int_info->state) {
    case DSC_WDS_INT_UP:
    case DSC_WDS_INT_STARTING:
        break;
    default:
        dsc_log_err("dsc_wds_stop_interface_req called in state %d\n",
                    int_info->state);
        goto error;
    }

    wds_clnt_id = dsc_qmi_wds_get_clnt_id(link);
    qmi_drv_info = dsc_qmi_wds_get_qmi_drv_info(link);

    /* Process based on current interface state */
    if (int_info->state == DSC_WDS_INT_UP) {
        /* Call QMI Client Driver API to stop network interface if the interface
        ** is currently up. 
        */
        if (qmi_wds_stop_nw_if
            (
                wds_clnt_id, 
                NULL, 
                (void *)link,
                &qmi_err
                ) < 0) 
        {
            dsc_log_err("qmi_wds_stop_nw_if failed with error %ld\n", (long int)qmi_err);
        }

        /* Change state to stopping */
        int_info->state = DSC_WDS_INT_STOPPING;
    } else {
        /* As interface is not yet up, call QMI Client Driver API to abort 
        ** start interface request. Note that this request may fail due to 
        ** race conditions and its failure must therefore also be handled. 
        */
        if (qmi_wds_abort
            (
                 wds_clnt_id, 
                 qmi_drv_info->start_if_req_tid,
                 dsc_qmi_wds_rsp, 
                 (void *)link,
                 &qmi_err
            ) < 0) 
        {
            dsc_log_err("qmi_wds_abort failed with error %ld\n", (long int)qmi_err);
            goto error;
        }

        /* Change state to aborting */
        int_info->state = DSC_WDS_INT_ABORTING;
    }

    status = DSC_OP_SUCCESS;

error:
    return status;
}

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
void
dsc_qmi_init (int nlink, int links[])
{
    /* If the number of links specified on the command line is invalid, 
    ** default to using the configured maximum number of links. 
    */
    if ((nlink <= 0) || (nlink > DSC_QMI_MAX_LINK)) {
        dsc_log_err("nlink range check fails. programming error, aborting");
        dsc_abort();
    }

    /* Set number of links in the configuration blob */
    dsc_qmi_cfg.nlink = nlink;

    /* Initialize data structures for QMI command processing */
    dsc_qmi_cmd_init();

    /* Initialize interface structures */
    dsc_wds_init(links);

    /* Initialize the QMI Client Driver and start WDS clients for each 
    ** interface.
    */
    dsc_qmi_driver_init(links);

    return;
}

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
)
{
    int wds_clnt_id;
    int qmi_err;

    /* Get WDS client ID for the default link, i.e. link ID 0 */
    wds_clnt_id = dsc_qmi_wds_get_clnt_id(0);

        dsc_log_high("Calling qmi_wds_query_profile!");

    /* Query profile parameters using the default link */
    if (qmi_wds_query_profile
                (
                    wds_clnt_id,
                    qmi_prof_id, 
                    qmi_prof_params,
                    &qmi_err
                ) != 0)
    {
        dsc_log_err("qmi_wds_query_profile failed with error %ld\n", (long int)qmi_err);
        return -1;
    }
        else {
                dsc_log_err("Returned from qmi_wds_query_profile!");
        }

    return 0;
}

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
)
{
  int nas_client_id,qmi_err_code;
  int i = 0;
  qmi_nas_serving_system_info_type   serving_system_info;

  nas_client_id = dsc_qmi_nas_get_client_id();

  /*Verify that the NAS client Id is a valid client ID*/
  assert(nas_client_id > 0);

  //dsc_log_err("Before NAS query \n");
  if (qmi_nas_get_serving_system(nas_client_id,&serving_system_info,&qmi_err_code) < 0)
  {
    dsc_log_err("qmi_nas_query_technology failed with error %d\n", qmi_err_code);
    return -1;
  }
  else 
  {     
    radio_info->regisration_state = serving_system_info.reg_state;
    radio_info->ps_attach_state   =  serving_system_info.ps_attach_state;
    radio_info->num_radio_ifaces  = serving_system_info.num_radio_interfaces;
    for(i = 0; i < serving_system_info.num_radio_interfaces; i++)
      radio_info->radio_if[i] = serving_system_info.radio_if[i];
  }
  return 0;
}
/*===========================================================================
  FUNCTION  dsc_qmi_wds_util_set_event_report
===========================================================================*/
/*!
@brief
  Helper  function to set event report, to turn on/off dormancy 
  indications on a particular wds link

@return
  int - 0 if successfully processed, -1 otherwise

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static int 
dsc_qmi_wds_util_set_event_report
(
  int lnk, 
  int dorm_ind_on
)
{
  int rc = -1, wds_clnt_id, qmi_err;
  qmi_wds_event_report_params_type event_report_params;

  dsc_log_high("In dsc_qmi_wds_util_set_event_report for link %d", lnk);

  /* Verify the link id first */
  if (dsc_qmi_verify_link(lnk) < 0) {
    dsc_log_err("dsc_qmi_wds_util_set_event_report called with invalid link %d", lnk);
    return DSC_OP_FAIL;
  }

  /* Get WDS client ID for the specified link  */
  wds_clnt_id = dsc_qmi_wds_get_clnt_id(lnk);

  event_report_params.param_mask = FALSE; 
  event_report_params.param_mask |= QMI_WDS_EVENT_DORM_STATUS_IND;
  /* Set Event Report to ON/OFF dormancy indications */
  event_report_params.report_dorm_status = dorm_ind_on;

  if ((rc = qmi_wds_set_event_report(wds_clnt_id, &event_report_params,&qmi_err) < 0))
  {
    dsc_log_err("dsc_qmi_wds_util_set_event_report: Set event report failed with  with error %ld\n", 
                       (long int)qmi_err);
    return DSC_OP_FAIL;
  }
  return  DSC_OP_SUCCESS;
}


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
dsc_qmi_ioctl (int lnk, dsc_dcm_iface_ioctl_t * iface_ioctl)
{
  int rval = -1, wds_clnt_id, qmi_err;

  dsc_log_high("In dsc_qmi_ioctl for link %d", lnk);

  /*assert on iface_ioctl*/
  ds_assert(iface_ioctl);

  /* Verify the link id first */
  if (dsc_qmi_verify_link(lnk) < 0) {
    dsc_log_err("dsc_qmi_ioctl called with invalid link %d", lnk);
    goto error;
  }

  /* Get WDS client ID for the specified link  */
  wds_clnt_id = dsc_qmi_wds_get_clnt_id(lnk);


  /*Now process based on Ioctl*/
  switch(iface_ioctl->name)
  {
    case DSS_IFACE_IOCTL_GO_DORMANT:
      {
        if ((rval = qmi_wds_go_dormant_req(wds_clnt_id,&qmi_err)) != 0)
        {
          dsc_log_err("dsc_qmi_ioctl: Go Dormant request failed for link %d, with return code %d, qmi error code %d", lnk, rval, qmi_err );
          rval = DSC_OP_FAIL;
          goto error;
        }
      }
    break;

    case DSS_IFACE_IOCTL_GO_ACTIVE:
      {
        if ((rval = qmi_wds_go_active_req(wds_clnt_id,&qmi_err)) != 0)
        {
          dsc_log_err("dsc_qmi_ioctl: Go active request failed for link %d, with return code %d, qmi error code %d", lnk, rval, qmi_err );
          rval = DSC_OP_FAIL;
          goto error;
        }
      }
    break;

    case DSS_IFACE_IOCTL_DORMANCY_INDICATIONS_OFF:
      {
        qmi_wds_dorm_status_type   dorm_st;

        if ( dsc_qmi_wds_util_set_event_report(lnk, FALSE) != DSC_OP_SUCCESS)
        {
          dsc_log_err("dsc_qmi_ioctl: Set event report failed\n");

          rval = DSC_OP_FAIL;
          goto error;
        }
        /*Fetch the dormancy status for this link and store it for later use*/
        if ((rval = qmi_wds_get_dormancy_status(wds_clnt_id,&dorm_st,&qmi_err) < 0))
        {
          dsc_log_err("dsc_qmi_ioctl: get dormancy indication failed  with error %ld\n", 
                       (long int)qmi_err);
          dorm_status[lnk]  = DSC_QMI_WDS_INVALID;
        }
        else if (rval == QMI_NO_ERR)
        {
          /*Store the dorm status for link*/
          dsc_log_high("dsc_qmi_ioctl: dormancy status is %d",(int)dorm_st);
          dorm_status[lnk] = (short)dorm_st;
        }
      }
    break;

    case DSS_IFACE_IOCTL_DORMANCY_INDICATIONS_ON:
      {
        qmi_wds_dorm_status_type   dorm_st;
        if ( dsc_qmi_wds_util_set_event_report(lnk, TRUE) != DSC_OP_SUCCESS)
        {
          dsc_log_err("dsc_qmi_ioctl: Set event report failed\n");
          rval = DSC_OP_FAIL;
          goto error;
        }
        /*
          Fetch the dormancy status to check if the dormancy state changed
          If the Dormacy state changes post dormancy state change 
          indication for the link.
        */
        if ((rval = qmi_wds_get_dormancy_status(wds_clnt_id,&dorm_st,&qmi_err) < 0))
        {
          dsc_log_err("dsc_qmi_ioctl: get dormancy indication failed with error %ld\n", 
                       (long int)qmi_err);
          dorm_status[lnk]  = DSC_QMI_WDS_INVALID;
        }
        else if (rval == QMI_NO_ERR)
        {
          if (dorm_status[lnk] == DSC_QMI_WDS_INVALID)
          {
            dsc_log_err("dsc_qmi_ioctl:dorm_status[%d] is in invalid state %d\n",lnk, dorm_status[lnk]); 
          }
          else if ((short)dorm_st == dorm_status[lnk])
          {
            dsc_log_low("dsc_qmi_ioctl:Physlink state has not changed"); 
          }
          else if ((short)dorm_st != dorm_status[lnk])
          {
            qmi_wds_indication_id_type     ind_id;
            qmi_wds_indication_data_type   ind_data;

            dsc_log_low("dsc_qmi_ioctl:Physlink state has changed prev_state = %d --> new state = %d",
                                                                          dorm_status[lnk], dorm_st); 
            /*
            Post an indication to ourselves(dss) 
            to send the physlink status back to the 
            client application
            */
            ind_id = QMI_WDS_SRVC_EVENT_REPORT_IND_MSG;
            ind_data.event_report.event_mask =  QMI_WDS_EVENT_DORM_STATUS_IND;
            ind_data.event_report.dorm_status = dorm_st;

            (void)dsc_qmi_wds_ind(wds_clnt_id,
                            QMI_WDS_SERVICE,
                            (void *)lnk,
                            ind_id,
                            &ind_data);

          }
        }
      }
      break;
      
    case DSS_IFACE_IOCTL_GET_MTU:
      {
        qmi_wds_profile_id_type            profile_id;
        qmi_wds_profile_params_type        profile_params;
        qmi_wds_curr_call_info_type        call_info;
        qmi_wds_req_runtime_settings_params_type req_mask;

        req_mask = QMI_WDS_GET_CURR_CALL_INFO_MTU_PARAM_MASK;
      
        if ((rval = qmi_wds_get_curr_call_info(wds_clnt_id,
                                               req_mask,
                                               &profile_id,
                                               &profile_params,
                                               &call_info,
                                               &qmi_err)) != QMI_NO_ERR)
        {
          dsc_log_err("dsc_qmi_ioctl: get_current_call_info failed for link %d,"
                      " with return code %d, qmi error code %d", lnk, rval, qmi_err );
          rval = DSC_OP_FAIL;
          goto error;
        } else {
          if( QMI_WDS_CURR_CALL_INFO_MTU & call_info.mask ) {
            iface_ioctl->info.mtu = (DSC_MTU_MAX < call_info.mtu)? DSC_MTU_MAX : call_info.mtu;
            dsc_log_low("dsc_qmi_ioctl: link MTU: %d\n", call_info.mtu );
          }
          else {
            dsc_log_err("dsc_qmi_ioctl: Link MTU undefined\n");
            rval = DSC_OP_FAIL;
            goto error;
          }
        }
      }
      break;

    case DSS_IFACE_IOCTL_GET_IPV4_GATEWAY_ADDR:
      {
        qmi_wds_profile_id_type        profile_id;
        qmi_wds_profile_params_type    profile_params;
        qmi_wds_curr_call_info_type    call_info;

        qmi_wds_req_runtime_settings_params_type req_mask_gtway;

        req_mask_gtway = QMI_WDS_GET_CURR_CALL_INFO_GATEWAY_INFO_PARAM_MASK;

        if ((rval = qmi_wds_get_curr_call_info(wds_clnt_id,
                                              req_mask_gtway,
                                              &profile_id,
                                              &profile_params,
                                              &call_info,
                                              &qmi_err)) != QMI_NO_ERR)
        {
          dsc_log_err("dsc_qmi_ioctl: get_current_call_info failed for link %d,"
                      " with return code %d, qmi error code %d", lnk, rval, qmi_err );
          rval = DSC_OP_FAIL;
          goto error;
        } else {
          if( QMI_WDS_CURR_CALL_INFO_IPV4_GATEWAY_ADDR & call_info.mask )
          {
            iface_ioctl->info.gateway.addr.v4 = htonl(call_info.ipv4_gateway_addr );
            iface_ioctl->info.gateway.type =IPV4_ADDR;
            dsc_log_low("dsc_qmi_ioctl: link GATEWAY: %x\n", call_info.ipv4_gateway_addr );
          } else {
            dsc_log_err("dsc_qmi_ioctl: Link GATEWAY undefined\n");
            rval = DSC_OP_FAIL;
            goto error;
          }
        }
      }
      break;

    case DSS_IFACE_IOCTL_GET_IPV4_PRIM_DNS_ADDR:
    case DSS_IFACE_IOCTL_GET_IPV4_SECO_DNS_ADDR:
      {
        qmi_wds_profile_id_type         profile_id;
        qmi_wds_profile_params_type     profile_params;
        qmi_wds_curr_call_info_type     call_info;

        qmi_wds_req_runtime_settings_params_type req_mask_dns;

        req_mask_dns = QMI_WDS_GET_CURR_CALL_INFO_DNS_ADDR_PARAM_MASK;

        if ((rval = qmi_wds_get_curr_call_info(wds_clnt_id,
                                              req_mask_dns,
                                              &profile_id,
                                              &profile_params,
                                              &call_info,
                                              &qmi_err)) != QMI_NO_ERR)
        {
          dsc_log_err("dsc_qmi_ioctl: get_current_call_info failed for link %d,"
                    " with return code %d, qmi error code %d", lnk, rval, qmi_err );
          rval = DSC_OP_FAIL;
          goto error;
        } else {
          if( QMI_WDS_CURR_CALL_INFO_PRIMARY_DNS_IPV4_ADDR & call_info.mask )
          {
            iface_ioctl->info.dns_addr.dns_primary.addr.v4 = htonl(call_info.primary_dns_ipv4_addr );
            iface_ioctl->info.dns_addr.dns_primary.type = IPV4_ADDR;
            dsc_log_low("dsc_qmi_ioctl: link  PRIMARY DNS: %x\n", call_info.primary_dns_ipv4_addr );
          } else {
            dsc_log_err("dsc_qmi_ioctl: Link PRIMARY DNS undefined\n");
            rval = DSC_OP_FAIL;
            goto error;
          }
          if( QMI_WDS_CURR_CALL_INFO_SECONDARY_DNS_IPV4_ADDR & call_info.mask )
          {
            iface_ioctl->info.dns_addr.dns_secondary.addr.v4 = htonl(call_info.secondary_dns_ipv4_addr );
            iface_ioctl->info.dns_addr.dns_secondary.type = IPV4_ADDR;
            dsc_log_low("dsc_qmi_ioctl: link SECONDRY DNS: %x\n", call_info.secondary_dns_ipv4_addr );
          } else {
            dsc_log_low("dsc_qmi_ioctl: Link SECONDRY DNS undefined\n");
            iface_ioctl->info.dns_addr.dns_secondary.addr.v4 = 0;
            iface_ioctl->info.dns_addr.dns_secondary.type = IPV4_ADDR;
          }
        }
      }
      break;

    case DSS_IFACE_IOCTL_GET_CURRENT_DATA_BEARER:
      {
        qmi_wds_data_bearer_tech_type  data_bearer_tech;    
        iface_ioctl->info.data_bearer_tech = DATA_BEARER_TECH_UNKNOWN;
        if ((rval = qmi_wds_get_current_bearer_tech(wds_clnt_id,&data_bearer_tech, &qmi_err)) != 0)
        {
          dsc_log_err("dsc_qmi_ioctl: Get current data bearer tech failed for link %d, "
                      "with return code %d, qmi error code %d", lnk, rval, qmi_err );
          rval = DSC_OP_FAIL;
          goto error;
        }
        dsc_wds_convert_qmi_bearer_to_dss_bearer(data_bearer_tech,&iface_ioctl->info.data_bearer_tech);
      }
    break;
    default:
      {
        dsc_log_err("dsc_qmi_ioctl: Invalid Ioctl received for link %d", lnk);
        dsc_abort();
      }
    break;
  }/*End Switch*/

  rval = DSC_OP_SUCCESS;
error:
  return rval;
}
