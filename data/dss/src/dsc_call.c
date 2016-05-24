/******************************************************************************

                      D S C _ C A L L . C

******************************************************************************/

/******************************************************************************

  @file    dsc_call.c
  @brief   DSC's call state machine

  DESCRIPTION
  Implementation of PS call state machine in the DSC.

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2011 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //source/qcom/qct/platform/linux/common/main/latest/apps/dsc/src/dsc_call.c#8 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
04/08/09   SM         Added Support for physlink Evnt Indications
03/24/09   SM         Added Call End Reason Code Support
02/20/09   js         Copy username/pass/auth_pref from dcm_net_policy
                      to call_params structure
05/30/08   vk         Added support for GET_DEVICE_NAME IOCTL
05/28/08   vk         Added support for APN override
03/15/08   vk         Incorporated code review comments
11/29/07   vk         Cleaned up lint warnings
11/17/07   vk         Added function headers and other comments
09/28/07   vk         Initial version

******************************************************************************/

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include <assert.h>
#include <pthread.h>
#include "dsci.h"
#include "ds_list.h"
#include "dsc_util.h"
#include "dsc_call.h"
#include "dsc_qmi_wds.h"
#include "dsc_kif.h"
#include "dsc_cmd.h"
#include "dsc_dcmi.h"

/*===========================================================================
                     LOCAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

#define DSC_MAX_CDMA_CALL   1
#define DSC_MAX_UMTS_CALL   DSC_MAX_PRICALL
#define DSC_MAX_CALL        (DSC_MAX_CDMA_CALL + DSC_MAX_UMTS_CALL)

/*--------------------------------------------------------------------------- 
   Maximum no. of command buffers allocated for Call SM
---------------------------------------------------------------------------*/
#define DSC_CALL_MAX_CMDS 8

/*--------------------------------------------------------------------------- 
   Enumeration of command types defined by Call SM
---------------------------------------------------------------------------*/
typedef enum {
    DSC_CALL_CMD_MIN        = 0,
    DSC_CALL_IFACE_UP_CMD   = DSC_CALL_CMD_MIN,
    DSC_CALL_IFACE_DOWN_CMD,
    DSC_CALL_CMD_MAX
} dsc_call_cmd_type_t;

/*--------------------------------------------------------------------------- 
   Structure representing a Call SM command's data
---------------------------------------------------------------------------*/
typedef struct dsc_call_cmd_data_s {
    dsc_call_cmd_type_t type; /* Type of command */
    union {
        int call_id;
    } value;                  /* Command data */
} dsc_call_cmd_data_t;

/*--------------------------------------------------------------------------- 
   Structure representing a Call SM command
---------------------------------------------------------------------------*/
typedef struct dsc_call_cmd_s {
    dsc_cmd_t           cmd;      /* Base command object */
    dsc_call_cmd_data_t data;     /* Call SM specific command data */
    int                 tracker;  /* 1 if alloc, else 0 */
    ds_dll_el_t         frl_node; /* Memory for enqueuing command on 
                                     the free list as a generic list node */
} dsc_call_cmd_t;

/*--------------------------------------------------------------------------- 
   Collection of control info for managing Call SM commands
---------------------------------------------------------------------------*/
static struct {
    dsc_call_cmd_t  cmd_arr[DSC_CALL_MAX_CMDS]; /* Array of commands */
    ds_dll_el_t   * frl_head;                   /* Head node of free list */
    ds_dll_el_t   * frl_tail;                   /* Tail node of free list */
    pthread_mutex_t mutx;     /* Mutex for protecting cmd enq and deq ops */
} dsc_call_cmd_ctrl;

/*--------------------------------------------------------------------------- 
   Collection of configuration info for Call SM
---------------------------------------------------------------------------*/
//static dsc_call_cfg_t dsc_call_cfg;

/*--------------------------------------------------------------------------- 
   Table of state info for calls
---------------------------------------------------------------------------*/
static dsc_pricall_info_t dsc_pricall_tbl[DSC_MAX_CALL];
/*--------------------------------------------------------------------------- 
   Definitions for call_end_reason
---------------------------------------------------------------------------*/
#define CALL_END_REASON_VALUE_UNSET        0
/*--------------------------------------------------------------------------- 
   Forward function declarations needed for subsequent initialization of 
   the callback structure registered with QMI WDS Interface module
---------------------------------------------------------------------------*/
void dsc_pricall_wds_start_interface_cnf 
(
    int link, 
    dsc_op_status_t status,
    dsc_qmi_call_end_reason_type  call_end_reason,  
    void * clnt_hdl
);

void dsc_pricall_wds_stop_interface_cnf 
(
    int link, 
    dsc_op_status_t status,
    void * clnt_hdl
);

void dsc_pricall_wds_stop_interface_ind (int link, dsc_qmi_call_end_reason_type  call_end_reason, void * clnt_hdl);
void dsc_pricall_event_report_ind (int link, dsc_qmi_dorm_status_type dorm_status, void * clnt_hdl);
void dsc_pricall_reconfig_required_ind (int link, void * clnt_hdl);

/*--------------------------------------------------------------------------- 
   Callback structure registered with QMI WDS Interface module
---------------------------------------------------------------------------*/
static const dsc_wds_int_clntcb_t dsc_pricall_wds_cbs = {
    dsc_pricall_wds_start_interface_cnf,
    dsc_pricall_wds_stop_interface_cnf,
    dsc_pricall_wds_stop_interface_ind,
    dsc_pricall_event_report_ind,
    dsc_pricall_reconfig_required_ind
};

/*--------------------------------------------------------------------------- 
   Forward function declarations needed for subsequent initialization of 
   the callback structure registered with Kernel Interface module
---------------------------------------------------------------------------*/
void dsc_pricall_kif_opened (int link, dsc_op_status_t status, void * clnt_hdl);
void dsc_pricall_kif_closed (int link, dsc_op_status_t status, void * clnt_hdl);
void dsc_pricall_kif_reconfigured (int link, 
                                   dsc_op_status_t status,  
                                   void * clnt_hdl);

/*--------------------------------------------------------------------------- 
   Callback structure registered with Kernel Interface module
---------------------------------------------------------------------------*/
static const dsc_kif_clntcb_t dsc_pricall_kif_cbs = {
    dsc_pricall_kif_opened,
    dsc_pricall_kif_closed,
    dsc_pricall_kif_reconfigured
};

/*--------------------------------------------------------------------------- 
   Forward function declarations
---------------------------------------------------------------------------*/
static void dsc_call_cmd_exec (dsc_cmd_t * cmd, void * data);
static void dsc_call_cmd_free (dsc_cmd_t * cmd, void * data);

/*--------------------------------------------------------------------------- 
   Inline accessor for obtaining call table index for a given Call ID
---------------------------------------------------------------------------*/
static __inline__ int 
dsc_pricall_get_index (dsc_callid_t callid)
{
    return callid;
}

/*--------------------------------------------------------------------------- 
   Inline accessor for obtaining IFACE ID for a given call table index
---------------------------------------------------------------------------*/
static __inline__ dcm_iface_id_t 
dsc_pricall_get_if_id (int indx)
{
    return dsc_pricall_tbl[indx].if_id;
}

/*--------------------------------------------------------------------------- 
   Inline accessor for obtaining Link ID for a given call table index
---------------------------------------------------------------------------*/
static __inline__ int
dsc_pricall_get_link (int indx)
{
	return dsc_pricall_tbl[indx].link;
}

/*--------------------------------------------------------------------------- 
   Inline mutator for setting Link ID for a given call table index
---------------------------------------------------------------------------*/
static __inline__ void 
dsc_pricall_set_link (int indx, int link)
{
    dsc_pricall_tbl[indx].link = link;
}

/*--------------------------------------------------------------------------- 
   Inline accessor for obtaining call state for a given call table index
---------------------------------------------------------------------------*/
static __inline__ dsc_pricall_state_t
dsc_pricall_get_state (int indx)
{
    return dsc_pricall_tbl[indx].state;
}

/*--------------------------------------------------------------------------- 
   Inline mutator for setting call state for a given call table index
---------------------------------------------------------------------------*/
static __inline__ void
dsc_pricall_set_state (int indx, dsc_pricall_state_t state)
{
    dsc_pricall_tbl[indx].state = state;
}

/*--------------------------------------------------------------------------- 
   Inline accessor for obtaining pointer to call params structure for a 
   given call table index; note that a pointer to non-const is returned
---------------------------------------------------------------------------*/
static __inline__ dsc_pricall_params_t *
dsc_pricall_get_req_params (int indx)
{
    return &dsc_pricall_tbl[indx].req_params;
}
/*===========================================================================
                            LOCAL FUNCTION DEFINITIONS
===========================================================================*/

/*--------------------------------------------------------------------------- 
  Utility function to map dcm_net_policy auth_pref to pricall auth_pref
---------------------------------------------------------------------------*/
static qmi_wds_auth_pref_type
dsc_pricall_map_auth_pref (dss_auth_pref_type dcm_auth_pref)
{
  /* This function assumes that dcm_auth_pref 
     is within the given range of dss_auth_pref_type */
  ds_assert(DSS_AUTH_PREF_MAX > dcm_auth_pref);
  qmi_wds_auth_pref_type ret;

  switch(dcm_auth_pref)
  {
  case DSS_AUTH_PREF_PAP_CHAP_NOT_ALLOWED:
    ret = QMI_WDS_AUTH_PREF_PAP_CHAP_NOT_ALLOWED;
    break;
  case DSS_AUTH_PREF_PAP_ONLY_ALLOWED:
    ret = QMI_WDS_AUTH_PREF_PAP_ONLY_ALLOWED;
    break;
  case DSS_AUTH_PREF_CHAP_ONLY_ALLOWED:
    ret = QMI_WDS_AUTH_PREF_CHAP_ONLY_ALLOWED;
    break;
  case DSS_AUTH_PREF_PAP_CHAP_BOTH_ALLOWED:
    ret = QMI_WDS_AUTH_PREF_PAP_CHAP_BOTH_ALLOWED;
    break;
  default:
    ds_assert(0);
  }

  return ret;
}

/*--------------------------------------------------------------------------- 
  Utility function to map dcm_net_policy data call origin type 
  to pricall data call origin type
---------------------------------------------------------------------------*/

static qmi_wds_data_call_origin_type
dsc_pricall_map_data_call_origin( dss_data_call_origin_type data_call_type)
{
  ds_assert(DSS_DATA_CALL_ORIGIN_MAX > data_call_type);
  qmi_wds_data_call_origin_type ret;

  switch(data_call_type)
  {
    case DSS_DATA_CALL_ORIGIN_EMBEDDED:
      ret = QMI_WDS_DATA_CALL_ORIGIN_EMBEDDED;
      break;
    case DSS_DATA_CALL_ORIGIN_LAPTOP:
      ret = QMI_WDS_DATA_CALL_ORIGIN_LAPTOP;
      break;
    default:
      ds_assert(0);
  }
  return ret;
}

/*===========================================================================
  FUNCTION  dsc_pricall_verify_callid
===========================================================================*/
/*!
@brief
  Verifies a given call ID as being a valid one. Note that this function 
  does not check if the call ID belongs to a currently active call.

@return
  int - 0 if success, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static int
dsc_pricall_verify_callid (dsc_callid_t callid) 
{
    /* Range check the specified call id */
    if ((callid < 0) || (callid >= DSC_MAX_CALL)) {
        return -1;
    }
    return 0;
}

/*===========================================================================
  FUNCTION  dsc_call_cmd_init
===========================================================================*/
/*!
@brief
  Initializes the command buffers used by Call SM.

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
dsc_call_cmd_init (void)
{
    ds_dll_el_t * node;
    dsc_call_cmd_t * cmdp;
    int i;

    /* Initialize the mutex used for protecting command list enqueue and 
    ** dequeue operations. 
    */
    (void)pthread_mutex_init(&dsc_call_cmd_ctrl.mutx, NULL);

    /* Initialize the free list of commands */
    node = ds_dll_init(NULL);
    ds_assert(node);

    /* Initialize free list's head and tail ptrs */
    dsc_call_cmd_ctrl.frl_head = node;
    dsc_call_cmd_ctrl.frl_tail = node;

    /* Iterate over the command object array, adding each to the free list */
    for (i = 0; i < DSC_CALL_MAX_CMDS; ++i) {
        cmdp = &dsc_call_cmd_ctrl.cmd_arr[i];

        /* Set command data ptr to point to the call cmd object */
        cmdp->cmd.data = cmdp;

        /* Set command handler function ptrs */
        cmdp->cmd.execute_f = dsc_call_cmd_exec;
        cmdp->cmd.free_f = dsc_call_cmd_free;

        /* Enqueue command on the free list */
        node = ds_dll_enq(node, &cmdp->frl_node, cmdp);
        dsc_call_cmd_ctrl.frl_tail = node;
    }
    return;
}

/*===========================================================================
  FUNCTION  dsc_call_cmd_alloc
===========================================================================*/
/*!
@brief
  Allocates an available Call SM command buffer.

@return
  dsc_call_cmd_t * - pointer to Call SM command buffer, if available
                     NULL otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static dsc_call_cmd_t * 
dsc_call_cmd_alloc (void)
{
    dsc_call_cmd_t * cmd = NULL;
    ds_dll_el_t * node;

    /* Acquire command lock */
    ds_assert(pthread_mutex_lock(&dsc_call_cmd_ctrl.mutx) == 0);

    /* Dequeue command object from free list */
    node = ds_dll_deq
           (
               dsc_call_cmd_ctrl.frl_head, 
               &dsc_call_cmd_ctrl.frl_tail,
               (const void **)&cmd
           );

    /* For debug purposes, set tracker if valid command was obtained */
    if (node != NULL) {
        cmd->tracker = 1;
    }

    /* Release command lock */
    ds_assert(pthread_mutex_unlock(&dsc_call_cmd_ctrl.mutx) == 0);

    /* Return ptr to command, or NULL if none available */
    return cmd;
}

/*===========================================================================
  FUNCTION  dsc_call_cmd_release
===========================================================================*/
/*!
@brief
  Adds the given Call SM command buffer back to the unused pool.

@return
  void

@note

  - Dependencies
    - The command buffer must have been allocated using dsc_call_cmd_alloc()

  - Side Effects
    - None
*/
/*=========================================================================*/
static void 
dsc_call_cmd_release (dsc_call_cmd_t * cmd)
{
    ds_dll_el_t * node;

    /* Acquire command lock */
    ds_assert(pthread_mutex_lock(&dsc_call_cmd_ctrl.mutx) == 0);

    ds_assert(cmd);

    /* Unset tracker for debug purposes */
    cmd->tracker = 0;

    /* Enqueue command on the free list */
    node = ds_dll_enq(dsc_call_cmd_ctrl.frl_tail, &cmd->frl_node, cmd);
    dsc_call_cmd_ctrl.frl_tail = node;

    /* Release command lock */
    ds_assert(pthread_mutex_unlock(&dsc_call_cmd_ctrl.mutx) == 0);

    return;
}

/*===========================================================================
  FUNCTION  dsc_call_cmd_exec
===========================================================================*/
/*!
@brief
  Virtual function used by the Command Thread to execute the Call CM
  command.

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
dsc_call_cmd_exec (dsc_cmd_t * cmd, void * data)
{
    dsc_call_cmd_t * call_cmd;
    dsc_call_cmd_type_t cmd_type;
	int rval = -1;

    /* Get call command ptr from the user data ptr */
    call_cmd = (dsc_call_cmd_t *)data;

    /* Double check for debug purposes that the command is legit */
    ds_assert(&call_cmd->cmd == cmd);

    /* Get command type */
    cmd_type = call_cmd->data.type;

    /* Process based on command type */
    switch (cmd_type) {
    case DSC_CALL_IFACE_UP_CMD:
        /* Request to bring up iface */
        rval = dsc_pricall_connect_req(call_cmd->data.value.call_id);
        break;
    case DSC_CALL_IFACE_DOWN_CMD:
        /* Request to bring iface down */
        rval = dsc_pricall_disconnect_req(call_cmd->data.value.call_id);
        break;
    default:
        /* Unknown command type. Abort */
        dsc_log_err("received unknown command type %d\n", cmd_type);
        dsc_abort();
    }

	if (rval < 0) {
		dsc_log_err("Command %d failed!\n", cmd_type);
	}

    return;
}

/*===========================================================================
  FUNCTION  dsc_call_cmd_free
===========================================================================*/
/*!
@brief
  Virtual function used by the Command Thread to free the Call SM command,
  after execution of the command is complete.

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
dsc_call_cmd_free (dsc_cmd_t * cmd, void * data)
{
    dsc_call_cmd_t * call_cmd;

    /* Get call command ptr from the user data ptr */
    call_cmd = (dsc_call_cmd_t *)data;

    /* Double check for debug purposes that the command is legit */
    ds_assert(&call_cmd->cmd == cmd);

    /* Release command back into the pool of free commands */
    dsc_call_cmd_release(call_cmd);

    return;
}

/*===========================================================================
  FUNCTION  dsc_pricall_pdp_prof_query
===========================================================================*/
/*!
@brief
  Queries PDP profile parameters for the given profile ID.

@return
  dsc_op_status_t - DSC_OP_SUCCESS if success, DSC_OP_FAIL otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static dsc_op_status_t
dsc_pricall_pdp_prof_query 
(
    int                           prof_id, 
    qmi_wds_profile_params_type * qmi_prof_p,
    unsigned long                 param_mask
)
{
    qmi_wds_profile_id_type qmi_prof_id;

    /* Initialize profile and profile id before proceeding */
    memset(&qmi_prof_id, 0x0, sizeof(qmi_wds_profile_id_type));
    memset(qmi_prof_p, 0x0, sizeof(qmi_wds_profile_params_type));

    /* Set technology and profile index */
    qmi_prof_id.technology = QMI_WDS_PROFILE_TECH_3GPP;
    qmi_prof_id.profile_index = prof_id;

    /* Ask for parameters requested by the client */
    qmi_prof_p->umts_profile_params.param_mask = param_mask;

    /* Query profile */
    if (dsc_wds_query_profile(&qmi_prof_id, qmi_prof_p) != 0) 
    {
        dsc_log_err("pdp_prof_query: wds_query_profile failed for prof_id %d",
                    prof_id);
        return DSC_OP_FAIL;
    }

    return DSC_OP_SUCCESS;
}

/* The following functions are no longer being used anywhere so forcing them to
** not compile but leaving them in source in case they are needed later. 
*/
#if 0

/*===========================================================================
  FUNCTION  dsc_pricall_pdp_prof_compare
===========================================================================*/
/*!
@brief
  Compares two PDP profiles for the given parameter type.

@return
  dsc_op_status_t - DSC_OP_SUCCESS if the parameter value is the same in 
                    both profiles, DSC_OP_FAIL otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static dsc_op_status_t
dsc_pricall_pdp_prof_compare
(
    const qmi_wds_profile_params_type * qmi_prof_1_p,
    const qmi_wds_profile_params_type * qmi_prof_2_p,
    unsigned long param_type
)
{
    dsc_op_status_t status = DSC_OP_FAIL;

    if (!(param_type & qmi_prof_1_p->param_mask & qmi_prof_2_p->param_mask))
    {
        goto error;
    }

    switch (param_type) {
    case QMI_WDS_PROFILE_PARAM_PDP_TYPE:
        if (qmi_prof_1_p->pdp_type == qmi_prof_2_p->pdp_type) {
            status = DSC_OP_SUCCESS;
        }
        break;
    case QMI_WDS_PROFILE_PARAM_APN_NAME:
        if (strncasecmp
            (
                qmi_prof_1_p->apn_name,
                qmi_prof_2_p->apn_name,
                QMI_WDS_MAX_APN_STR_SIZE - 1
            ) == 0) 
        {
            status = DSC_OP_SUCCESS;
        }
        break;
    default:
        dsc_log_err("pdp_prof_compare: cannot compare param_type %lx",
                    param_type);
    }

error:
    return status;
}

/*===========================================================================
  FUNCTION  dsc_pricall_match_pdp_prof
===========================================================================*/
/*!
@brief
  Determines if for the specified PDP profile IDs, the PDP Type and APNs
  match.

@return
  dsc_op_status_t - DSC_OP_SUCCESS if the PDP Type and APNs match, 
                    DSC_OP_FAIL otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static dsc_op_status_t
dsc_pricall_match_pdp_prof (int prof_id_1, int prof_id_2)
{
    qmi_wds_profile_params_type qmi_prof_1, qmi_prof_2;
    unsigned long query_param_mask;
    dsc_op_status_t status = DSC_OP_FAIL;

    /* Quick check for both IDs being the same */

    if (prof_id_1 == prof_id_2) {
        status = DSC_OP_SUCCESS;
        goto error;
    }

    query_param_mask = QMI_WDS_PROFILE_PARAM_PDP_TYPE | 
                       QMI_WDS_PROFILE_PARAM_APN_NAME;

    if (dsc_pricall_pdp_prof_query(prof_id_1, &qmi_prof_1, query_param_mask) 
        != DSC_OP_SUCCESS)
    {
        dsc_log_err("match_pdp_prof: pdp_prof_query failed for prof_id %d",
                    prof_id_1);
        goto error;
    }

    if (dsc_pricall_pdp_prof_query(prof_id_2, &qmi_prof_2, query_param_mask) 
        != DSC_OP_SUCCESS)
    {
        dsc_log_err("match_pdp_prof: pdp_prof_query failed for prof_id %d",
                    prof_id_2);
        goto error;
    }

    if (dsc_pricall_pdp_prof_compare
        (
            &qmi_prof_1, 
            &qmi_prof_2, 
            QMI_WDS_UMTS_PROFILE_PDP_TYPE_PARAM_MASK
        ) != DSC_OP_SUCCESS)
    {
        goto error;
    }

    if (dsc_pricall_pdp_prof_compare
        (
            &qmi_prof_1, 
            &qmi_prof_2, 
            QMI_WDS_PROFILE_PARAM_APN_NAME
        ) != DSC_OP_SUCCESS)
    {
        goto error;
    }

    status = DSC_OP_SUCCESS;

error:
    return status;
}

#endif /* if 0 */

static dsc_op_status_t
dsc_pricall_compare_pdp_params
(
    const dsc_pricall_params_t * call_params,
    int                          prof_id,
    const dss_umts_apn_type    * apn
)
{
    qmi_wds_profile_params_type qmi_prof;
    dsc_op_status_t status = DSC_OP_FAIL;

    if (apn->length == 0) {
        /* Compare against APN specified in profile id */
        if (dsc_pricall_pdp_prof_query
            (
                prof_id,
                &qmi_prof, 
                QMI_WDS_UMTS_PROFILE_APN_NAME_PARAM_MASK
            ) != DSC_OP_SUCCESS)
        {
            dsc_log_err("compare_pdp_params: pdp_prof_query failed for prof_id %d",
                         prof_id);
            goto error;
        }
	if (strncasecmp
            (
                call_params->umts.apn_name,
                qmi_prof.umts_profile_params.apn_name,
                QMI_WDS_MAX_APN_STR_SIZE - 1
            ) != 0) 
        {
		    dsc_log_high("comapre_pdp_params: strncasecmp returned non-equal!\n");
            goto error;
        }
    } else {
        /* Compare against APN specified explicitly */
        if (strncasecmp
            (
                call_params->umts.apn_name,
                apn->name,
                DSC_QMI_WDS_MAX_APN_STR_SIZE
            ) != 0) 
        {
            goto error;
        }
    }

    status = DSC_OP_SUCCESS;

error:
    return status;
}

/*===========================================================================
  FUNCTION  dsc_pricall_if_set_config_f
===========================================================================*/
/*!
@brief
  Virtual function registered with PS IFACE for setting configuration 
  parameters for the IFACE.

@return
  dsc_op_status_t - DSC_OP_SUCCESS if successful, DSC_OP_FAIL otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static dsc_op_status_t 
dsc_pricall_if_set_config_f 
(
    dcm_iface_id_t if_id,
    void * call_hdl, 
    dcm_net_policy_info_t * net_policy
)
{
    dsc_callid_t call_id;
	int indx;
    dsc_pricall_params_t * call_params;
    dsc_op_status_t status = DSC_OP_FAIL;
    dcm_iface_id_name_t if_name;
    qmi_wds_profile_params_type qmi_prof;

    /* Get call id from user data ptr */
    call_id = (dsc_callid_t)call_hdl;

    /* Verify call id before proceeding */
    if (dsc_pricall_verify_callid(call_id) < 0) {
        dsc_log_err("if_set_config_f: invalid call_id %d received\n", call_id);
        goto error;
    }

    /* Get index into call table from call id */
    indx = dsc_pricall_get_index(call_id);

    /* Double check that the iface id corresponds to the call id */
    if (dsc_pricall_get_if_id(indx) != if_id) {
        dsc_log_err("if_set_config_f: if_id %d doesn't match for call id %d\n",
                    (int)if_id, call_id);
        goto error;
    }

    /* Get name of this iface */
	if_name = dsc_dcm_if_get_name(if_id);

    dsc_log_high("if_name is [%x]", if_name);

    /* get call params using indx */
    call_params = dsc_pricall_get_req_params(indx);

    /* reset username string */
    memset(call_params->username.value, 0, DSC_QMI_WDS_MAX_STR_SIZE);

    /* copy username from net_policy to call_params */
    ds_assert(net_policy->username.length <= DSC_QMI_WDS_MAX_STR_SIZE);

    call_params->username.length = net_policy->username.length;
    memcpy
    (
      call_params->username.value,
      net_policy->username.value,
      net_policy->username.length
    );

    /* reset password string */
    memset(call_params->password.value, 0, DSC_QMI_WDS_MAX_STR_SIZE);

    /* copy password from net_policy to call_params */
    ds_assert(net_policy->password.length <= DSC_QMI_WDS_MAX_STR_SIZE);
    call_params->password.length = net_policy->password.length;
    memcpy
    (
      call_params->password.value,
      net_policy->password.value,
      net_policy->password.length
    );

    /* copy auth_pref from net_policy to call_params */
    ds_assert(net_policy->auth_pref < DSS_AUTH_PREF_MAX);
    call_params->auth_pref_enabled = 0;

    if (net_policy->auth_pref > DSS_AUTH_PREF_NOT_SPECIFIED)
    {
      call_params->auth_pref = dsc_pricall_map_auth_pref
      (
       net_policy->auth_pref
       );
      call_params->auth_pref_enabled = 1;
    }

    /* copy data call origin type from net_policy to call_params */
    ds_assert(net_policy->data_call_origin < DSS_DATA_CALL_ORIGIN_MAX);
    call_params->data_call_origin = dsc_pricall_map_data_call_origin(
                                              net_policy->data_call_origin);

    /* Set call params for the call */
    if (if_name == DSS_IFACE_UMTS)
    {
        call_params->system_flag = DSC_PRICALL_UMTS;
        call_params->umts.profile_id = net_policy->umts.pdp_profile_num;

        /* copy apn from net_policy to call_params */
        ds_assert(net_policy->umts.apn.length <= DSC_QMI_WDS_MAX_APN_STR_SIZE);

        call_params->umts.apn_length = net_policy->umts.apn.length;

        /* For easier debugging, reset apn_name before copying */
        memset(call_params->umts.apn_name, 0, DSC_QMI_WDS_MAX_APN_STR_SIZE);

        memcpy
        (
            call_params->umts.apn_name, 
            net_policy->umts.apn.name,
            net_policy->umts.apn.length
        );

        /* Fetch APN and save it, if APN is not specified */
        if (call_params->umts.apn_length == 0) 
        {
            if (dsc_pricall_pdp_prof_query
                (
                    call_params->umts.profile_id, 
                    &qmi_prof, 
                    QMI_WDS_UMTS_PROFILE_APN_NAME_PARAM_MASK
                ) != DSC_OP_SUCCESS)
            {
                dsc_log_err("set_config: pdp_prof_query failed for prof_id %d",
                             call_params->umts.profile_id);
                goto error;
            }

            call_params->umts.apn_length = strlen(qmi_prof.umts_profile_params.apn_name);
            memcpy
            (
                call_params->umts.apn_name, 
                qmi_prof.umts_profile_params.apn_name, 
                call_params->umts.apn_length
            );
        }
    }
    else if (if_name == DSS_IFACE_CDMA_SN)
    {
        /* set cdma profile id in call_params */
        call_params->cdma.profile_id = net_policy->cdma.data_session_profile_id;
        call_params->system_flag = DSC_PRICALL_CDMA;
    } else {
        dsc_log_err("set_config: if_name %d unexpected", if_name);
        goto error;
    }
    status = DSC_OP_SUCCESS;

error:
    return status;
}

/*===========================================================================
  FUNCTION  dsc_pricall_if_match_f
===========================================================================*/
/*!
@brief
  Virtual function registered with PS IFACE for determining if the specified 
  network policy matches the interface's existing configuration. This 
  function is used during interface selection to determine an IFACE's 
  suitability for serving an application given the specified network policy. 

@return
  dsc_op_status_t - DSC_OP_SUCCESS if policy matches IFACE's configuration,
                    DSC_OP_FAIL otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static dsc_op_status_t
dsc_pricall_if_match_f 
(
    dcm_iface_id_t if_id, 
    void * call_hdl, 
    dcm_net_policy_info_t * net_policy
)
{
    dsc_callid_t call_id;
	int indx;
    dsc_pricall_params_t * call_params;
    dsc_pricall_state_t state;
    dsc_op_status_t status = DSC_OP_FAIL;
    dcm_iface_id_name_t if_name;
    dcm_iface_id_name_t this_if_name;

    /* Get call id from user data ptr */
    call_id = (dsc_callid_t)call_hdl;

    /* Verify call id before proceeding */
    if (dsc_pricall_verify_callid(call_id) < 0) {
        dsc_log_err("if_match_f: invalid call_id %d received\n", call_id);
        goto error;
    }

    /* Get index into call table from call id */
    indx = dsc_pricall_get_index(call_id);

    /* Double check that the iface id corresponds to the call id */
    if (dsc_pricall_get_if_id(indx) != if_id) {
        dsc_log_err("if_match_f: if_id %d doesn't match for call id %d\n",
                    (int)if_id, call_id);
        goto error;
    }

    /* Get if name from policy */
    if_name = net_policy->iface.info.name;

    /* Get name of current iface */
	this_if_name = dsc_dcm_if_get_name(if_id);

    /* Reject this iface if the requested iface type doesn't match. This 
    ** should not occur as DCM layer is supposed to guard against this. But 
    ** we still do this check again anyway. 
    */
    if ((if_name != this_if_name) && 
        (if_name != DSS_IFACE_WWAN) &&
        (if_name != DSS_IFACE_ANY) &&
        (if_name != DSS_IFACE_ANY_DEFAULT))
    {
        goto error;
    }

    /* Get current call state */
    state = dsc_pricall_get_state(indx);

    /* Return success if call is currently idle */
    if (state == DSC_PRICALL_IDLE) {
        status = DSC_OP_SUCCESS;
        goto error;
    }

    /* Return failure if call is being torn down */
    if ((state != DSC_PRICALL_CONNECTED) && 
        (state != DSC_PRICALL_CONNECTING_QMI) &&
        (state != DSC_PRICALL_CONNECTING_KIF))
    {
        goto error;
    }

    status = DSC_OP_SUCCESS;

    if (this_if_name == DSS_IFACE_UMTS)
    {
        /* Get current call params. These should have been set earlier using 
        ** if_set_config_f handler. 
        */
        call_params = dsc_pricall_get_req_params(indx);

        /* If call is already up or coming up, determine if the requested 
        ** params match the params used to bring up the call.
        */
        status = dsc_pricall_compare_pdp_params
                (
                      call_params,
                      net_policy->umts.pdp_profile_num,
                      &net_policy->umts.apn
                );
	printf("if_match_f: if_id = %ld, prof id = %d, status = %d\n",
		if_id, net_policy->umts.pdp_profile_num, status);
    }

error:
    return status;
}

/*===========================================================================
  FUNCTION  dsc_pricall_if_ioctl_f
===========================================================================*/
/*!
@brief
  Virtual function registered with PS IFACE as a generic IFACE IOCTL 
  handler. Any IOCTLs not handled by the generic IFACE layer are passed
  to the lower layers using this virtual function. 

@return
  dsc_op_status_t - DSC_OP_SUCCESS if IOCTL was successfully executed,
                    DSC_OP_FAIL otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static dsc_op_status_t
dsc_pricall_if_ioctl_f
(
    dcm_iface_id_t          if_id, 
    void                  * call_hdl,
    dsc_dcm_iface_ioctl_t * ioctl
)
{
    dsc_op_status_t status = DSC_OP_FAIL;
    dsc_callid_t call_id;
    dsc_pricall_state_t state;
    int indx;
    int link;

    /* Get call id from user data ptr */
    call_id = (dsc_callid_t)call_hdl;

    /* Verify call id before proceeding */
    if (dsc_pricall_verify_callid(call_id) < 0) {
        dsc_log_err("if_ioctl_f: invalid call_id %d received\n", call_id);
        goto error;
    }

    /* Get index into call table from call id */
    indx = dsc_pricall_get_index(call_id);

    /* Double check that the iface id corresponds to the call id */
    if (dsc_pricall_get_if_id(indx) != if_id) {
        dsc_log_err("if_ioctl_f: if_id %d doesn't match for call id %d\n",
                    (int)if_id, call_id);
        goto error;
    }

    /* Get current call state */
    state = dsc_pricall_get_state(indx);
    /* Verify that ioctl can be issued given the state of the call */
    switch (ioctl->name) {
    case DSS_IFACE_IOCTL_GET_IPV4_ADDR:
    case DSS_IFACE_IOCTL_GET_IPV4_GATEWAY_ADDR:
    case DSS_IFACE_IOCTL_GET_IPV4_PRIM_DNS_ADDR:
    case DSS_IFACE_IOCTL_GET_IPV4_SECO_DNS_ADDR:
    case DSS_IFACE_IOCTL_BIND_SOCK_TO_IFACE:
    case DSS_IFACE_IOCTL_GET_DEVICE_NAME:
    case DSS_IFACE_IOCTL_GO_DORMANT:
    case DSS_IFACE_IOCTL_GO_ACTIVE:
    case DSS_IFACE_IOCTL_DORMANCY_INDICATIONS_ON:
    case DSS_IFACE_IOCTL_DORMANCY_INDICATIONS_OFF:
    case DSS_IFACE_IOCTL_GET_CURRENT_DATA_BEARER:

        if (state != DSC_PRICALL_CONNECTED) {
            dsc_log_err("Rejecting ioctl %d received when call is not up",
                        ioctl->name);
            goto error;
        }
        break;
    default:
        dsc_log_err("if_ioctl_f: invalid ioctl %d received\n", ioctl->name);
        goto error;
    }

    /* Get link id for the call */
    link = dsc_pricall_get_link(indx);

    /* Process the ioctl */
    switch (ioctl->name) {
    case DSS_IFACE_IOCTL_GET_IPV4_ADDR:
    case DSS_IFACE_IOCTL_BIND_SOCK_TO_IFACE:
    case DSS_IFACE_IOCTL_GET_DEVICE_NAME:
        /* Issue ioctl to lower layer */
        if (dsc_kif_ioctl(link, ioctl) < 0) {
            goto error;
        }
        break;

    case DSS_IFACE_IOCTL_GO_DORMANT:
    case DSS_IFACE_IOCTL_GO_ACTIVE:
    case DSS_IFACE_IOCTL_DORMANCY_INDICATIONS_ON:
    case DSS_IFACE_IOCTL_DORMANCY_INDICATIONS_OFF:
    case DSS_IFACE_IOCTL_GET_MTU:
    case DSS_IFACE_IOCTL_GET_IPV4_GATEWAY_ADDR:
    case DSS_IFACE_IOCTL_GET_IPV4_PRIM_DNS_ADDR:
    case DSS_IFACE_IOCTL_GET_IPV4_SECO_DNS_ADDR:
    case DSS_IFACE_IOCTL_GET_CURRENT_DATA_BEARER:
        /*Issue ioctl to make the physlink go dormant*/
        if (dsc_qmi_ioctl(link, ioctl) <0)
        {
          goto error;
        }
        break;

    default:
        dsc_log_err("if_ioctl_f: invalid ioctl %d received\n", ioctl->name);
        goto error;
    }

    status = DSC_OP_SUCCESS;

error:
    return status;
}

/*===========================================================================
  FUNCTION  dsc_pricall_if_up_cmd
===========================================================================*/
/*!
@brief
  Virtual function registered with PS IFACE for handling IFACE UP command. 

@return
  dsc_op_status_t - DSC_OP_SUCCESS if command was successfully executed,
                    DSC_OP_FAIL otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static dsc_op_status_t  
dsc_pricall_if_up_cmd (dcm_iface_id_t if_id, void * call_hdl)
{
    dsc_callid_t call_id;
	int indx;
    dsc_call_cmd_t * cmd;

    /* Get call id from user data ptr */
    call_id = (dsc_callid_t)call_hdl;

    /* Verify call id before proceeding */
    if (dsc_pricall_verify_callid(call_id) < 0) {
        dsc_log_err("if_up_cmd: invalid call_id %d received\n", call_id);
        return DSC_OP_FAIL;
    }

    /* Get index into call table from call id */
    indx = dsc_pricall_get_index(call_id);

    /* Double check that the iface id corresponds to the call id */
    if (dsc_pricall_get_if_id(indx) != if_id) {
        dsc_log_err("if_up_cmd: if_id %d doesn't match for call id %d\n",
                    (int)if_id, call_id);
        return DSC_OP_FAIL;
    }

    /* Post a command to handle the iface up cmd in command thread context */
    cmd = dsc_call_cmd_alloc();
    ds_assert(cmd);

    cmd->data.type = DSC_CALL_IFACE_UP_CMD;
    cmd->data.value.call_id = call_id;

    dsc_cmdq_enq(&cmd->cmd);
    return DSC_OP_SUCCESS;
}

/*===========================================================================
  FUNCTION  dsc_pricall_if_down_cmd
===========================================================================*/
/*!
@brief
  Virtual function registered with PS IFACE for handling IFACE DOWN command. 

@return
  dsc_op_status_t - DSC_OP_SUCCESS if command was successfully executed,
                    DSC_OP_FAIL otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static dsc_op_status_t  
dsc_pricall_if_down_cmd (dcm_iface_id_t if_id, void * call_hdl)
{
    dsc_callid_t call_id;
	int indx;
    dsc_call_cmd_t * cmd;

    /* Get call id from user data ptr */
    call_id = (dsc_callid_t)call_hdl;

    /* Verify call id before proceeding */
    if (dsc_pricall_verify_callid(call_id) < 0) {
        dsc_log_err("if_down_cmd: invalid call_id %d received\n", call_id);
        return DSC_OP_FAIL;
    }

    /* Get index into call table from call id */
    indx = dsc_pricall_get_index(call_id);

    /* Double check that the iface id corresponds to the call id */
    if (dsc_pricall_get_if_id(indx) != if_id) {
        dsc_log_err("if_down_cmd: if_id %d doesn't match for call id %d\n",
                    (int)if_id, call_id);
        return DSC_OP_FAIL;
    }

    /* Post a command to handle the iface down cmd in command thread context */
    cmd = dsc_call_cmd_alloc();
    ds_assert(cmd);

    cmd->data.type = DSC_CALL_IFACE_DOWN_CMD;
    cmd->data.value.call_id = call_id;

    dsc_cmdq_enq(&cmd->cmd);
    return DSC_OP_SUCCESS;
}

#if 0
static dsc_call_tech_t 
dsc_call_get_tech_from_name(const char * tech_nm)
{
    dsc_call_tech_t tech = DSC_CALL_TECH_INVALID;

    if (strcasecmp(tech_nm, "cdma") == 0) {
        tech = DSC_CALL_TECH_CDMA;
    } else if (strcasecmp(tech_nm, "umts") == 0) {
        tech = DSC_CALL_TECH_UMTS;
    }

    return tech;
}
#endif
/*===========================================================================
  FUNCTION  dsc_pricall_init
===========================================================================*/
/*!
@brief
  Initialized and instantiates data structures and objects needed for 
  handling primary call requests. In particular, instantiates IFACEs and 
  registers them with DCM.

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
dsc_pricall_init(void)
{
    int i;
    dcm_iface_id_t if_id;
    dsc_dcm_if_op_tbl_t if_op_tbl;

    /* Set all handler function ptrs for each iface */
    if_op_tbl.if_match_f  = dsc_pricall_if_match_f;
    if_op_tbl.if_set_config_f = dsc_pricall_if_set_config_f;
    if_op_tbl.if_up_cmd   = dsc_pricall_if_up_cmd;
    if_op_tbl.if_down_cmd = dsc_pricall_if_down_cmd;
    if_op_tbl.if_ioctl_f = dsc_pricall_if_ioctl_f;

    /* Iterate over the call table, creating an iface for each entry */
//    for (i = 0; i < dsc_call_cfg.ncall; ++i) {
//        dsc_pricall_tbl[i].callid = i;
//
//       /* Request iface creation */
//        if (tech == DSC_CALL_TECH_CDMA)
//        {
//            if_id = dsc_dcm_if_create
//            (
//                CDMA_SN_IFACE, 
//                WWAN_GROUP | ANY_IFACE_GROUP | ANY_DEFAULT_GROUP, 
//                (void *)i, 
//                &if_op_tbl
//           );
//        }
//        else
//        {
//            if_id = dsc_dcm_if_create
//            (
//                UMTS_IFACE, 
//                WWAN_GROUP | ANY_IFACE_GROUP | ANY_DEFAULT_GROUP, 
//                (void *)i, 
//                &if_op_tbl
//            );
//        }
//        assert(if_id != DCM_IFACE_ID_INVALID);
//
//        /* Set iface id for the call entry to the assigned iface id */
//       dsc_pricall_tbl[i].if_id = if_id;
//   }

    /*Create UMTS IFACES*/    
    for (i = 0; i < DSC_MAX_UMTS_CALL; ++i) {
        dsc_pricall_tbl[i].callid = i;

        /* Request iface creation */
            if_id = dsc_dcm_if_create
            (
                UMTS_IFACE, 
                WWAN_GROUP | ANY_IFACE_GROUP | ANY_DEFAULT_GROUP, 
                (void *)i, 
                &if_op_tbl
            );
            assert(if_id != DCM_IFACE_ID_INVALID);
            dsc_pricall_tbl[i].if_id = if_id;
        }
    /*Create CDMA IFACE*/
            if_id = dsc_dcm_if_create
            (
                CDMA_SN_IFACE, 
                WWAN_GROUP | ANY_IFACE_GROUP | ANY_DEFAULT_GROUP, 
                (void *)i, 
                &if_op_tbl
            );
        assert(if_id != DCM_IFACE_ID_INVALID);
        dsc_pricall_tbl[i].if_id = if_id;

    return;
}

/*===========================================================================
  FUNCTION  dsc_pricall_connect_req
===========================================================================*/
/*!
@brief
  Handler for the IFACE_UP command.

@return
  int - 0 if command is handled successfully, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int 
dsc_pricall_connect_req (dsc_callid_t callid)
{
    int rval = -1;
    int indx;
    dsc_pricall_state_t state;
    int link;

    /* Verify call id before proceeding */
    if (dsc_pricall_verify_callid(callid) < 0) {
        goto error;
    }

    /* Get index into call table from call id */
    indx = dsc_pricall_get_index(callid);

    /* Process based on current call state */
    switch (state = dsc_pricall_get_state(indx)) {
    case DSC_PRICALL_IDLE:
        /* Call is currently idle. Reserve a link and bind it to the call */
        if ((link = dsc_wds_reserve_interface
                    (
                        &dsc_pricall_wds_cbs,
                        (void *)callid
                    )) < 0)
        {
            goto error;
        }
        dsc_pricall_set_link(indx, link);

        /* Send QMI WDS request to bring up the call */
        if (dsc_wds_start_interface_req(link, dsc_pricall_get_req_params(indx))
            == DSC_OP_FAIL)
        {
            /* Could not send qmi request for some reason. Unbind link from 
            ** call and return error.
            */
            dsc_wds_unreserve_interface(link);
            goto error;
        }
        break;
    default:
        dsc_log_err("dsc_pricall_connect_req called in state %d", state);
        goto error;
    }

    /* Change state to "connecting qmi link" */
    dsc_pricall_set_state(indx, DSC_PRICALL_CONNECTING_QMI);
    rval = 0;

error:
    return rval;
}

/*===========================================================================
  FUNCTION  dsc_pricall_disconnect_req
===========================================================================*/
/*!
@brief
  Handler for the IFACE_DOWN command.

@return
  int - 0 if command is handled successfully, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int 
dsc_pricall_disconnect_req (dsc_callid_t callid)
{
    int rval = -1;
    int indx;
    int link;
    dsc_pricall_state_t state;

    /* Verify call id before proceeding */
    if (dsc_pricall_verify_callid(callid) < 0) {
        goto error;
    }

    /* Get index into call table from call id */
    indx = dsc_pricall_get_index(callid);

    /* Get link id for the call */
    link = dsc_pricall_get_link(indx);

    /* Process based on current call state */
    switch (state = dsc_pricall_get_state(indx)) {
    case DSC_PRICALL_CONNECTED:
        /* Intentional fall through */
    case DSC_PRICALL_CONNECTING_KIF:
    case DSC_PRICALL_RECONFIGURING_KIF:
        /* Send QMI WDS command to bring call down */
        if (dsc_wds_stop_interface_req(link) == DSC_OP_FAIL)
        {
            /* Could not send QMI WDS request to bring call down. Abort for 
            ** debugging purposes. 
            */
            dsc_log_err("Cannot send wds stop int req");
            dsc_abort();
            goto error;
        }
        /* Change state to "disconnecting (initial)" */
        dsc_pricall_set_state(indx, DSC_PRICALL_DISCONNECTING_I);
        break;
    case DSC_PRICALL_CONNECTING_QMI:
        if (dsc_wds_stop_interface_req(link) == DSC_OP_FAIL)
        {
            /* Could not send QMI WDS request to bring call down. Abort for 
            ** debugging purposes. 
            */
            dsc_log_err("dsc_wds_stop_interface_req returned failure");
            dsc_abort();
            goto error;
        }
        /* Change state to "disconnecting (final)" as once call teardown is 
        ** is confirmed by qmi driver we can directly transition to idle state.
        */
        dsc_pricall_set_state(indx, DSC_PRICALL_DISCONNECTING_F);
        break;
    default:
        /* Ignore request in all other states, i.e. call not up or coming up */
        dsc_log_err("dsc_pricall_disconnect_req called in state %d", state);
        goto error;
    }

    rval = 0;

error:
    return rval;
}

/*===========================================================================
  FUNCTION  dsc_pricall_wds_start_interface_cnf
===========================================================================*/
/*!
@brief
  Callback function registered with the QMI Driver Interface module for 
  sending confirmation of response to start network interface back to 
  Call SM.

@return
  void

@note

  - Dependencies
    - Must be called in Command Thread context

  - Side Effects
    - None
*/
/*=========================================================================*/
void 
dsc_pricall_wds_start_interface_cnf 
(
    int link, 
    dsc_op_status_t status,
    dsc_qmi_call_end_reason_type  call_end_reason,  
    void * clnt_hdl
)
{
    dsc_callid_t callid;
    dsc_pricall_state_t state;
    int indx;

    /* Get call id for the link */
    callid = (dsc_callid_t)clnt_hdl;

    /* Verify call id before proceeding */
    if (dsc_pricall_verify_callid(callid) < 0) {
        dsc_log_err("Invalid link %d in dsc_pricall_wds_start_interface_cnf", link);
        goto error;
    }

    /* Get index into the call table */
    indx = dsc_pricall_get_index(callid);

    /* Double check link before proceeding */
    ds_assert(dsc_pricall_get_link(indx) == link);

    /* Process based on current call state */
    switch (state = dsc_pricall_get_state(indx)) {
    case DSC_PRICALL_CONNECTING_QMI:
        /* We are waiting for qmi response to call bring up request. If request 
        ** succeeded, issue command to bring up kernel if. 
        */
        if (status == DSC_OP_SUCCESS) {
            if (dsc_kif_open(link, &dsc_pricall_kif_cbs, clnt_hdl) < 0) {
                dsc_log_err("dsc_kif_open failed");
                dsc_abort();
                goto error;
            }
            /* Change state to "connecting kernel if" */
            dsc_pricall_set_state(indx, DSC_PRICALL_CONNECTING_KIF);
        } else {
            /* Qmi response is negative, i.e. call bring up failed. Transition 
            ** to idle state and send iface down indication. 
            */
            dsc_pricall_set_state(indx, DSC_PRICALL_IDLE);
            dsc_wds_unreserve_interface(link);
            dsc_dcm_if_down_ind((int)call_end_reason,dsc_pricall_get_if_id(indx));
                                //TODO:Change the call_end_reason to return qmi_error_code?
        }
        break;
    default:
        dsc_log_err("dsc_pricall_wds_start_interface_cnf called in state %d", state);
        break;
    }
error:
    dsc_log_high( "dsc_pricall_wds_start_interface_cnf: EXIT with err" );
    return;
}

/*===========================================================================
  FUNCTION  dsc_pricall_wds_stop_interface_cnf
===========================================================================*/
/*!
@brief
  Callback function registered with the QMI Driver Interface module for 
  sending confirmation of stop network interface back to Call SM.

@return
  void

@note

  - Dependencies
    - Must be called in Command Thread context

  - Side Effects
    - None
*/
/*=========================================================================*/
void 
dsc_pricall_wds_stop_interface_cnf 
(
    int link, 
    dsc_op_status_t status,
    void * clnt_hdl
)
{
    dsc_callid_t callid;
    dsc_pricall_state_t state;
    int indx;
    dsc_log_high( "dsc_pricall_wds_stop_interface_cnf: ENTRY" );
    /* Get call id for the link */
    callid = (dsc_callid_t)clnt_hdl;

    /* Verify call id before proceeding */
    if (dsc_pricall_verify_callid(callid) < 0) {
        dsc_log_err("Invalid link %d in dsc_pricall_wds_stop_interface_cnf", link);
        goto error;
    }

    /* Quick check for response being negative, i.e. call teardown failed. 
    ** If so, print error message and return. This should not occur. 
    */
    if (status != DSC_OP_SUCCESS) {
        dsc_log_err("wds_stop_interface_cnf called with failure status");
        goto error;
    }

    /* Get index into call table */
    indx = dsc_pricall_get_index(callid);

    /* Double check link before proceeding */
    ds_assert(dsc_pricall_get_link(indx) == link);

    /* Process based on current call state */
    switch (state = dsc_pricall_get_state(indx)) {
    case DSC_PRICALL_DISCONNECTING_I:
    case DSC_PRICALL_RECONFIGURING_KIF:
        /* We are waiting for qmi response to call termination request. Issue
        ** command to bring down kernel if, as the kernel if is either up or 
        ** coming up. 
        */
        if (dsc_kif_close(link) < 0) {
            dsc_log_err("dsc_kif_close returned failure");
            dsc_abort();
            goto error;
        }

        /* Change state to "disconnecting (final)" */
        dsc_pricall_set_state(indx, DSC_PRICALL_DISCONNECTING_F);
        break;
    case DSC_PRICALL_DISCONNECTING_F:
        /* We are waiting for qmi response to call termination request, and the 
        ** kernel if is already down. Change state to idle and send iface 
        ** down indication. 
        */
        dsc_pricall_set_state(indx, DSC_PRICALL_IDLE);
        dsc_wds_unreserve_interface(link);
        dsc_dcm_if_down_ind((int)CALL_END_REASON_VALUE_UNSET,dsc_pricall_get_if_id(indx));
        break;
    default:
        /* Ignore event in all other states. This should not occur */
        dsc_log_err("dsc_pricall_wds_stop_interface_cnf called in state %d", state);
        break;
    }
    dsc_log_high( "dsc_pricall_wds_stop_interface_cnf: EXIT with succ" );
    return;
error:
    dsc_log_high( "dsc_pricall_wds_stop_interface_cnf: EXIT with err" );
    return;
}

/*===========================================================================
  FUNCTION  dsc_pricall_wds_stop_interface_ind
===========================================================================*/
/*!
@brief
  Callback function registered with the QMI Driver Interface module for 
  sending indication of network interface stoppage from the modem side.

@return
  void

@note

  - Dependencies
    - Must be called in Command Thread context

  - Side Effects
    - None
*/
/*=========================================================================*/
void 
dsc_pricall_wds_stop_interface_ind (int link, dsc_qmi_call_end_reason_type call_end_reason, void * clnt_hdl)
{
    dsc_callid_t callid;
    dsc_pricall_state_t state;
    int indx;

    /* Get call id for the link */
    callid = (dsc_callid_t)clnt_hdl;

    /* Verify call id before proceeding */
    if (dsc_pricall_verify_callid(callid) < 0) {
        dsc_log_err("Invalid link %d in dsc_pricall_wds_stop_interface_ind", link);
        goto error;
    }

    /* Get index into call table */
    indx = dsc_pricall_get_index(callid);

    /* Double check link before proceeding */
    ds_assert(dsc_pricall_get_link(indx) == link);

    /* Process based on current call state */
    switch (state = dsc_pricall_get_state(indx)) {
    case DSC_PRICALL_CONNECTED:
        /* Intentional fall through */
    case DSC_PRICALL_CONNECTING_KIF:
    case DSC_PRICALL_RECONFIGURING_KIF:
        /* Call was earlier up and kernel if was either up or coming up. Send 
        ** command to bring down kernel if.
        */
        if (dsc_kif_close(link) < 0) {
            dsc_log_err("kern_close returned failure");
            dsc_abort();
            goto error;
        }
        /* Change state to "disconnecting (final)" */
        dsc_pricall_set_state(indx, DSC_PRICALL_DISCONNECTING_F);
        break;
    default:
        /* Ignore event in all other states. This should not occur */
        dsc_log_err("dsc_pricall_wds_stop_interface_ind called in state %d", state);
        break;
    }
  /*Get the index into the call Table and store the reason code in the call table.*/
  dsc_pricall_tbl[indx].call_end_reason_code =  call_end_reason; 

error:
    return;
}


/*===========================================================================
  FUNCTION  dsc_pricall_event_report_ind
===========================================================================*/
/*!
@brief
  Callback function registered with the QMI Driver Interface module for 
  sending Event report Indication on a Qmi Link. Currently Physlink events 
  are reported.

@return
  void

@note

  - Dependencies
    - Must be called in Command Thread context

  - Side Effects
    - None
*/
/*=========================================================================*/
void 
dsc_pricall_event_report_ind (int link, dsc_qmi_dorm_status_type dorm_status, void * clnt_hdl)
{
  dsc_callid_t callid;
  dsc_pricall_state_t state;
  int indx;
  dsc_log_func_entry();

  /* Get call id for the link */
  callid = (dsc_callid_t)clnt_hdl;

  /* Verify call id before proceeding */
  if (dsc_pricall_verify_callid(callid) < 0) {
      dsc_log_err("Invalid call id %d in dsc_pricall_event_report_ind", callid);
      dsc_abort();
      goto error;
  }

  /* Get index into call table */
  indx = dsc_pricall_get_index(callid);

  /* Double check link before proceeding */
  ds_assert(dsc_pricall_get_link(indx) == link);

  /* Process based on current call state */
  switch (state = dsc_pricall_get_state(indx)) {
    case DSC_PRICALL_CONNECTED:
    /*Handle: send indication to dcm*/
      dsc_dcm_physlink_state_change_ind((int)dorm_status,dsc_pricall_get_if_id(indx));
    break;
    /*Ignore physlink event in any other state*/  
    default:
      dsc_log_err("Ignoring physlink state change received in state %ld",state);
  }
error:
  dsc_log_func_exit();
  return;
}

/*===========================================================================
  FUNCTION  dsc_pricall_reconfig_required_ind
===========================================================================*/
/*!
@brief
  Callback function registered with the QMI Driver Interface module for 
  handling reconfig_required event.

@return
  void

@note

  - Dependencies
    - Must be called in Command Thread context

  - Side Effects
    - None
*/
/*=========================================================================*/
void 
dsc_pricall_reconfig_required_ind (int link, void * clnt_hdl)
{
  dsc_callid_t callid;
  dsc_pricall_state_t state;
  int indx;
  dsc_log_func_entry();

  /* Get call id for the link */
  callid = (dsc_callid_t)clnt_hdl;

  /* Verify call id before proceeding */
  if (dsc_pricall_verify_callid(callid) < 0) {
      dsc_log_err("Invalid call id %d in dsc_pricall_reconfig_required_ind", callid);
      dsc_abort();
      goto error;
  }

  /* Get index into call table */
  indx = dsc_pricall_get_index(callid);

  /* Double check link before proceeding */
  ds_assert(dsc_pricall_get_link(indx) == link);

  /* Process based on current call state */
  switch (state = dsc_pricall_get_state(indx)) {
    case DSC_PRICALL_CONNECTED:
        /*Handle: send indication to kif module*/
        dsc_pricall_set_state(indx, DSC_PRICALL_RECONFIGURING_KIF);
        dsc_kif_reconfigure(link);
        break;
    /*Ignore reconfig event in any other state*/  
    default:
        dsc_log_err("Ignoring reconfigure event received in state %ld",
                    state);
  }
error:
  dsc_log_func_exit();
  return;
}

/*===========================================================================
  FUNCTION  dsc_pricall_kif_opened
===========================================================================*/
/*!
@brief
  Callback function registered with the Kernel Interface module for sending
  confirmation of response to open network interface back to Call SM.

@return
  void

@note

  - Dependencies
    - Must be called in Command Thread context

  - Side Effects
    - None
*/
/*=========================================================================*/
void
dsc_pricall_kif_opened (int link, dsc_op_status_t status, void * clnt_hdl)
{
    dsc_callid_t callid;
    dsc_pricall_state_t state;
    int indx;

    /* Get call id for the link */
    callid = (dsc_callid_t)clnt_hdl;

    /* Verify call id before proceeding */
    if (dsc_pricall_verify_callid(callid) < 0) {
        dsc_log_err("Invalid link %d in dsc_pricall_kif_opened", link);
        dsc_abort();
        goto error;
    }

    /* Get index into call table */
    indx = dsc_pricall_get_index(callid);

    /* Double check link before proceeding */
    ds_assert(dsc_pricall_get_link(indx) == link);

    /* Process based on current call state */
    switch (state = dsc_pricall_get_state(indx)) {
    case DSC_PRICALL_CONNECTING_KIF:
        /* We were waiting for kernel if to come up */
        if (status == DSC_OP_SUCCESS) {
            /* Kernel if is up, so change state to connected and send iface up 
            ** indication. 
            */
            dsc_pricall_set_state(indx, DSC_PRICALL_CONNECTED);
    		dsc_dcm_if_up_ind(dsc_pricall_get_if_id(indx));
        } else {
            /* Could not bring up kernel if. This should not occur. Abort for 
            ** debug purposes. 
            */
            dsc_log_err("kif_opened indicated failure");
            dsc_abort();
            goto error;
        }
        break;
    default:
        dsc_log_err("dsc_pricall_kif_opened called in state %d", state);
        break;
    }

error:
    return;
}

/*===========================================================================
  FUNCTION  dsc_pricall_kif_closed
===========================================================================*/
/*!
@brief
  Callback function registered with the Kernel Interface module for sending
  confirmation of close network interface back to Call SM.

@return
  void

@note

  - Dependencies
    - Must be called in Command Thread context

  - Side Effects
    - None
*/
/*=========================================================================*/
void
dsc_pricall_kif_closed (int link, dsc_op_status_t status, void * clnt_hdl)
{
    dsc_callid_t callid;
    dsc_pricall_state_t state;
    int indx;

    /* Get call id for the link */
    callid = (dsc_callid_t)clnt_hdl;

    /* Verify call id before proceeding */
    if (dsc_pricall_verify_callid(callid) < 0) {
        dsc_log_err("Invalid call id %d in dsc_pricall_kif_closed", callid);
        dsc_abort();
        goto error;
    }

    /* If for some reason we received a failure status, abort for debugging 
    ** purposes as this should not occur. 
    */
    if (status != DSC_OP_SUCCESS) {
        dsc_log_err("kif_closed called with failure status");
        dsc_abort();
        goto error;
    }

    /* Get index into call table */
    indx = dsc_pricall_get_index(callid);

    /* Double check link before proceeding */
    ds_assert(dsc_pricall_get_link(indx) == link);

    /* Process based on current call state */
    switch (state = dsc_pricall_get_state(indx)) {
    case DSC_PRICALL_DISCONNECTING_I:
        /* We are waiting for qmi response to call disconnection, while kernel
        ** if is either up or coming up. Looks like it went down for some 
        ** reason by itself, so no need to request kernel if close again when 
        ** qmi response is received. For this, transition to disconnecting 
        ** (final) state.
        */
        dsc_pricall_set_state(indx, DSC_PRICALL_DISCONNECTING_F);
        break;
    case DSC_PRICALL_CONNECTED:
    case DSC_PRICALL_RECONFIGURING_KIF:
        /* Call is up. Send qmi wds request to bring call down as we lost the 
        ** kernel if for some reason. 
        */
        if (dsc_wds_stop_interface_req(link) == DSC_OP_FAIL) {
            dsc_log_err("Cannot send wds stop int req");
            dsc_abort();
            goto error;
        }
        /* Change state to disconnecting (final) to directly move to idle state
        ** once qmi response is received. 
        */
        dsc_pricall_set_state(indx, DSC_PRICALL_DISCONNECTING_F);
        break;
    case DSC_PRICALL_DISCONNECTING_F:
        /* We were waiting for the kernel interface to come down. It just did, 
        ** so change state to idle and send iface down indication. 
        */
        dsc_pricall_set_state(indx, DSC_PRICALL_IDLE);
        dsc_wds_unreserve_interface(link);
        dsc_dcm_if_down_ind((int)dsc_pricall_tbl[indx].call_end_reason_code,dsc_pricall_get_if_id(indx));
		break;
    default:
        dsc_log_err("dsc_pricall_kif_closed called in state %d", state);
        break;
    }

error:
    return;
}

/*===========================================================================
  FUNCTION  dsc_pricall_kif_reconfigured
===========================================================================*/
/*!
@brief
  Callback function registered with the Kernel Interface module for sending
  confirmation of response to recongigure interface back to Call SM.

@return
  void

@note

  - Dependencies
    - Must be called in Command Thread context

  - Side Effects
    - None
*/
/*=========================================================================*/
void
dsc_pricall_kif_reconfigured (int link, dsc_op_status_t status, void * clnt_hdl)
{
    dsc_callid_t callid;
    dsc_pricall_state_t state;
    int indx;

    /* Get call id for the link */
    callid = (dsc_callid_t)clnt_hdl;

    /* Verify call id before proceeding */
    if (dsc_pricall_verify_callid(callid) < 0) {
        dsc_log_err("Invalid link %d in dsc_pricall_kif_opened", link);
        dsc_abort();
        goto error;
    }

    /* Get index into call table */
    indx = dsc_pricall_get_index(callid);

    /* Double check link before proceeding */
    ds_assert(dsc_pricall_get_link(indx) == link);

    /* Process based on current call state */
    switch (state = dsc_pricall_get_state(indx)) {
    case DSC_PRICALL_RECONFIGURING_KIF:
        /* We were waiting for kernel if to come up */
        if (status == DSC_OP_SUCCESS) {
            /* Kernel if is reconfigured, so change state to connected */
            dsc_pricall_set_state(indx, DSC_PRICALL_CONNECTED);
            dsc_dcm_if_reconfigured_ind(dsc_pricall_get_if_id(indx));
        } else {
            /* Could not reconfigure kernel if. 
            ** This should not occur. During the code review
            ** we decided *not* to abort
            */
            dsc_log_err("kif_reconfigured indicated failure");
            goto error;
        }
        break;
    default:
        dsc_log_err("dsc_pricall_kif_reconfigured called in state %d", 
                    state);
        /* this should never happen, abort, and debug */
        dsc_abort();
        break;
    }

error:
    return;
}

/*===========================================================================
                            GLOBAL FUNCTION DEFINITIONS
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
void
dsc_call_init(int npri)
{
    (void)npri;
  
    /* Initialize Call SM data structures */
    dsc_pricall_init();
    dsc_call_cmd_init();

    return;
}
