#ifndef DSQMH_LLIF_H
#define DSQMH_LLIF_H
/*===========================================================================


                       Q M I   M O D E   H A N D L E R

                   L O W E R   L A Y E R   I N T E R F A C E
                       
GENERAL DESCRIPTION
  This file contains data declarations and function prototypes for the
  QMI Proxy IFACE lower layer interface.  

EXTERNALIZED FUNCTIONS

INITIALIZATION AND SEQUENCING REQUIREMENTS

Copyright (c) 2008 - 2012 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

  $Header: //source/qcom/qct/modem/datahlos/interface/qmiifacectls/rel/11.01.01/src/ds_qmh_llif.h#3 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
04/13/11    hm     Multi-modem support merged from linux QMH
07/02/10    hm     Add support for Fusion target.
05/24/10    hm     Support QOS_CONFIGURE opcode with QOS_REQUEST_EX
04/15/09    ar     Added dsqmhllif_qos_conv_flow_params_to_ps prototype
05/06/08    ar     Created module/initial version.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"
#include "ds_qmhi.h"

#if defined (FEATURE_DATA_PS) && defined (FEATURE_QMI_CLIENT)

#include "qmi_qos_srvc_i.h"

/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/


/* MBMS operation selector */
typedef enum {
  DSQMHLLIF_MBMS_OP_INVALID,
  DSQMHLLIF_MBMS_OP_ACTIVATE,
  DSQMHLLIF_MBMS_OP_DEACTIVATE
} dsqmhllif_mbms_op;

/* BCMCS operation selector */
typedef enum {
  DSQMHLLIF_BCMCS_OP_INVALID,
  DSQMHLLIF_BCMCS_OP_DB_UPDATE,
  DSQMHLLIF_BCMCS_OP_HANDOFF_REG,
  DSQMHLLIF_BCMCS_OP_BOM_CACHING
} dsqmhllif_bcmcs_op;

/* Dormancy operation selector */
typedef enum {
  DSQMHLLIF_DORMANCY_OP_INVALID,
  DSQMHLLIF_DORMANCY_OP_GOACTIVE,
  DSQMHLLIF_DORMANCY_OP_GODORMANT
} dsqmhllif_dormancy_op;

/* QOS operation selector */
typedef enum {
  DSQMHLLIF_QOS_OP_INVALID,
  DSQMHLLIF_QOS_OP_RELEASE,
  DSQMHLLIF_QOS_OP_SUSPEND,
  DSQMHLLIF_QOS_OP_RESUME,
  DSQMHLLIF_QOS_OP_MODIFY,
  DSQMHLLIF_QOS_OP_MODIFY_PRI
} dsqmhllif_qos_op;

typedef enum {
  DSQMHLLIF_QOS_INFO_INVALID,
  DSQMHLLIF_QOS_INFO_NETWORK,
  DSQMHLLIF_QOS_INFO_STATUS
} dsqmhllif_qos_info_type;


/*===========================================================================
                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================
FUNCTION DSQMHLLIF_INIT_QMI_CONNECTIONS

DESCRIPTION
  This function initializes the QMI connections. Services are inited through
  dsqmhllif_init_qmi_services() later when all the connections have been
  successfully opened. 

PARAMETERS
  None. 

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on successful operation.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_init_qmi_connections
(
  void
);


/*===========================================================================
FUNCTION DSQMHLLIF_INIT_QMI_SERVICES

DESCRIPTION
  This function initializes the connection to the QMI services.  Each
  IFACE has a dedicated QMI connection and registeres for both WDS and
  QOS services.

PARAMETERS
  err_code_ptr  - Pointer to error code

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on successful operation.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_init_qmi_services
(
  uint32     iface_inst,
  int32     *err_code_ptr
);


/*===========================================================================
FUNCTION DSQMHLLIF_OPEN_TRANSPORT

DESCRIPTION
  This function opens the datapath transport (SMD port) specified in
  the IFACE control block.  The SMD port is configured to support
  packet transfer.

PARAMETERS
  iface_inst  - Index for iface table
  phys_link   - Index for phys link list

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on successful operation.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  SMD task will receive open command

===========================================================================*/
int dsqmhllif_open_transport
(
  uint32        iface_inst,                     /* Index for iface table   */
  uint32        phys_link                       /* Index for phys link list*/
);


/*===========================================================================
FUNCTION DSQMHLLIF_CLOSE_TRANSPORT

DESCRIPTION
  This function closes the datapath transport (SMD port) specified in
  the IFACE control block.
  
PARAMETERS
  iface_inst  - Index for iface table

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on successful operation.

SIDE EFFECTS
  SMD task will receive close command

===========================================================================*/
int dsqmhllif_close_transport
(
  uint32        iface_inst,                     /* Index for iface table   */
  uint32        phys_link                       /* Index for phys link list*/
);


/*===========================================================================
FUNCTION DSQMHLLIF_INIT_PHYS_LINK_QUEUES

DESCRIPTION
  This function initializes the specified physical link queueus.  DSM
  watermark queues are used between the Proxy IFACE and the Shared
  Memory Driver.  Both UL and DL direction queues are configured.

PARAMETERS
  iface_inst  - Index for iface table
  phys_link   - Index for phys link list

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/
void dsqmhllif_init_phys_link_queues
(
  uint32        iface_inst,                     /* Index for iface table   */
  uint32        phys_link                       /* Index for phys link list*/
);


/*===========================================================================
FUNCTION DSQMHLLIF_SET_TX_FLOW()

DESCRIPTION
  This function sets the flow control state on the SMD port UL stream.
  Whether to enable (FALSE) or disable (TRUE) flow is specified by
  'disable' parameter.

  The reason for the flow is specified by 'mask'. Multiple callers
  specifying different masks are supported.  Only when the mask is
  cleared (ie all callers want flow) will flow actually be enabled.
  
PARAMETERS
  handle  :  encoded iface and phys link identifiers
  mask    :  Bitmask indicating the reason for flow control
             (allows disabling flow if any of various reasons
             represented by separate bits has been specified)
  disable :  FALSE = enable flow for reasons in 'mask'
             TRUE  = disable flow for reasons in 'mask'

RETURN VALUE
  None

DEPENDENCIES
  dsqmhllif_init_phys_link_queues() must have been called previously

SIDE EFFECTS
  None
===========================================================================*/
void dsqmhllif_set_tx_flow
(
  void *  handle,
  uint32  mask,
  boolean disable
);


/*===========================================================================
FUNCTION DSQMHLLIF_SET_RX_FLOW()

DESCRIPTION
  This function sets the flow control state on the SMD port DL stream.
  Whether to enable (FALSE) or disable (TRUE) flow is specied by
  'disable' parameter.

  The reason for the flow is specified by 'mask'. Multiple callers
  specifying different masks are supported.  Only when the mask is
  cleared will flow actually be enabled.
  
PARAMETERS
  handle  :  encoded iface and phys link identifiers
  mask    :  Bitmask indicating the reason for flow control
             (allows disabling flow if any of various reasons
             represented by separate bits has been specified)
  disable :  FALSE = enable flow for reasons in 'mask'
             TRUE  = disable flow for reasons in 'mask'

RETURN VALUE
  None

DEPENDENCIES
  dsqmhllif_init_phys_link_queues() must have been called previously

SIDE EFFECTS
  None
===========================================================================*/
void dsqmhllif_set_rx_flow
(
  void *  handle,
  uint32  mask,
  boolean disable
);


/*===========================================================================
FUNCTION DSQMHLLIF_START_NETWORK_CMD

DESCRIPTION
  This function commands QMI to start a network interface.  The
  results of the mode processing are posted asynchronously via the
  callback registered with QMI WDS service.

PARAMETERS
  iface_inst  - Index for iface table

DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_start_network_cmd
(
  uint32        iface_inst                      /* Index for iface table   */
);


/*===========================================================================
FUNCTION DSQMHLLIF_STOP_NETWORK_CMD

DESCRIPTION
  This function commands QMI to stop a network interface.  The results
  of the mode processing are posted asynchronously via the callback
  registered with QMI WDS service.

PARAMETERS
  iface_inst  - Index for iface table

DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_stop_network_cmd
(
  uint32        iface_inst                      /* Index for iface table   */
);


/*===========================================================================
FUNCTION DSQMHLLIF_ABORT_CMD

DESCRIPTION
  This function commands QMI to abort the current pending transaction.

PARAMETERS
  iface_inst  - Index for iface table

DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_abort_cmd
(
  uint32        iface_inst                      /* Index for iface table   */
);


/*===========================================================================
FUNCTION DSQMHLLIF_QOS_REQUEST_CMD

DESCRIPTION
  This function commands QMI to request a new QOS flow.  The results
  of the mode processing are posted asynchronously via the callback
  registered with QMI QOS service.

PARAMETERS
  iface_inst     - Index for iface table
  opcode         - Configure QOS or Request QOS.
  qos_specs_ptr  - QOS specs list
  num_qos_specs  - QOS specs list length
  flows_pptr     - Pointer to PS flow list
  ps_errno       - Error code returned in case of failure (Error
                   values are those defined in dserrno.h)

DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_qos_request_cmd
(
  uint32         iface_inst,                    /* Index for iface table   */
  ps_iface_ioctl_qos_request_ex_opcode_enum_type  
                 opcode,                        /* Configure/request       */
  qos_spec_type *qos_specs_ptr,                 /* QOS specs list          */
  uint8          num_qos_specs,                 /* QOS specs list length   */
  ps_flow_type **flows_pptr,                    /* Pointer to PS flow list */
  int16         *ps_errno
);


/*===========================================================================
FUNCTION DSQMHLLIF_QOS_GET_GRANTED_CMD

DESCRIPTION
  This function commands QMI to request the granted QOS specificaiton
  for a given flow.  The results of the mode processing are posted
  asynchronously via the callback registered with QMI QOS service.

PARAMETERS
  flow_ptr      - Pointer to PS flow
  ioctl_name    - The operation name
  argval_ptr    - Pointer to PS QOS spec structure
  ps_errno      - Error code returned in case of failure

DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_qos_get_granted_cmd
(
  ps_flow_type       *flow_ptr,                      /* Pointer to PS flow      */
  ps_flow_ioctl_type  ioctl_name,
  void               *argval_ptr,
  int16              *ps_errno
);


/*===========================================================================
FUNCTION DSQMHLLIF_REFRESH_DHCP_CONFIG_INFO

DESCRIPTION
  This method handles the REFRESH_DHCP_CONFIG IOCTL. QMH is the only mode 
  handler that handles this "common" IOCTL. It sends the request over to 
  the modem to refresh the DHCP config. This is a synchronous message. 

PARAMETERS
  iface_inst  - Index for iface table
  ps_errno    - Error code returned in case of failure
  
DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_refresh_dhcp_config_info
(
  uint32    iface_inst,
  sint15   *ps_errno
);

/*===========================================================================
FUNCTION DSQMHLLIF_GET_NET_QOS_PROFILES

DESCRIPTION
  This function commands QMI to request the network supported QOS
  profiles for a given technology.  Note only CDMA technology is supported at thsi time.

PARAMETERS
  iface_inst  - Index for iface table
  net_tech    - Network technologies bitmask
  argval_ptr  - Pointer to QOS Profile structure
  ps_errno    - Error code returned in case of failure
  
DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_get_net_qos_profiles
(
  uint32    iface_inst,              /* Index for iface table     */
  uint32    net_tech,
  void     *argval_ptr,
  sint15   *ps_errno
);

/*===========================================================================
FUNCTION dsqmhllif_qos_flow_operation

DESCRIPTION
  This function commands QMI to execute get/get operations on existing
  QOS flow.

PARAMETERS
  flow_ptr    - Pointer to QOS flow
  req_ptr     - Pointer to operation request structure
  resp_ptr    - Pointer to operation response structure

DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_qos_flow_operation
(
  ps_flow_type                      *flow_ptr, 
  qmi_qos_perform_flow_op_req_type  *req_ptr,
  qmi_qos_perform_flow_op_resp_type *resp_ptr
);

/*===========================================================================
FUNCTION DSQMHLLIF_QOS_MANAGER

DESCRIPTION
  This function commands QMI to manipulate an existing QOS flow.  The
  results of the mode processing are posted asynchronously via the
  callback registered with QMI QOS service.

PARAMETERS
  operation   - Operation to be performed
  iface_inst  - Index for iface table
  flows_pptr  - Pointer to PS flow list
  num_flows   - PS flow list length
  argval_ptr  - IOCTL parameter
  ps_errno    - Error code returned in case of failure (Error
                values are those defined in dserrno.h)
  
DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_qos_manager
(
  dsqmhllif_qos_op   operation,               /* Operation to be performed */
  uint32             iface_inst,              /* Index for iface table     */
  ps_flow_type     **flows_pptr,              /* Pointer to PS flow list   */
  uint32             num_flows,               /* PS flow list length       */
  void              *argval_ptr,              /* IOCTL parameter           */
  int16             *ps_errno
);


/*===========================================================================
FUNCTION dsqmhllif_qos_enable_qmi_hdr

DESCRIPTION
  This function commands QMI to enable the QOS metadata header on each
  packet sent over the datapath transport. The header contains the QOS
  flow identifier so the metedata can be recovered on Modem side for
  RmNet IFACE.

PARAMETERS
  iface_inst  - Index for iface table
  flow_ptr    - Pointer to PS flow

DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_qos_enable_qmi_hdr
(
  uint32        iface_inst,                     /* Index for iface table   */
  ps_flow_type *flow_ptr                        /* Pointer to PS flow      */
);


/*===========================================================================
FUNCTION DSQMHLLIF_GET_FLOW_BY_QOS_ID

DESCRIPTION
  This function finds the PS flow based on the QMI QOS identifier.

PARAMETERS
  iface_inst  - Index for iface table
  qos_id      - QMI QOS flow identifier
  flow_pptr   - Pointer to PS flow

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on successful find.
  DSQMH_FAILED on miss.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_get_flow_by_qos_id
(
  uint32         iface_inst,                   /* Index for iface table   */
  uint32         qos_id,                       /* QMI QOS flow identifier */
  ps_flow_type **flow_pptr                     /* Pointer to PS flow      */
);

/*===========================================================================
FUNCTION DSQMHLLIF_PKT_STATS_CMD

DESCRIPTION
  This function commands QMI to query or reset the Modem statistics for the
  candidate IFACE to retreive the packet data statistics.

PARAMETERS 
  iface_ptr : ps iface ptr 
  data_stats: data statistics will be populated as response
  op_type   : get/reset statistics
 
DEPENDENCIES
  iface_ptr: iface ptr
  
RETURN VALUE
  None.
SIDE EFFECTS
  None.

===========================================================================*/
void
dsqmhllif_pkt_stats_cmd
(
  ps_iface_type                       *iface_ptr,
  ps_iface_stat_override_type         *data_stats,
  ps_iface_stat_override_op_enum_type  op_type
);

/*===========================================================================
FUNCTION DSQMHLLIF_QUERY_NET_SETTINGS

DESCRIPTION
  This function commands QMI to query the Modem for the network
  interface settings.  The QMI Messaging Library performs this as a
  synchronous operation from our perspectve.

PARAMETERS
  iface_inst          - Index for iface table
  req_param_mask      - Parameter request mask
  current_info_ptr    - Pointer to runtime settings structure
  
DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_query_net_settings
(
  uint32                       iface_inst,      /* Index for iface table   */
  qmi_wds_req_runtime_settings_params_type req_param_mask,
  dsqmh_runtime_info_type     *current_info_ptr
);

/*===========================================================================
FUNCTION DSQMHLLIF_QUERY_INTERNAL_SETTINGS

DESCRIPTION
  This function commands QMI to query the Modem for the internal
  runtime settings.  The QMI Messaging Library performs this as a
  synchronous operation from caller's perspectve.

PARAMETERS
  iface_inst    - Index for iface table
  req_ptr       - Pointer to requested parameter structure
  rsp_ptr       - Pointer to results structure

DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_query_internal_runtime_settings
(
  uint32                       iface_inst,      /* Index for iface table   */
  qmi_wds_internal_runtime_setings_params_type *req_ptr,
  qmi_wds_internal_runtime_settings_rsp_type   *rsp_ptr
);


/*===========================================================================
FUNCTION DSQMHLLIF_SET_INTERNAL_SETTINGS

DESCRIPTION
  This function commands QMI to set the Modem internal runtime
  settings.  The QMI Messaging Library performs this as a synchronous
  operation from caller's perspectve.

PARAMETERS
  iface_inst    - Index for iface table
  req_ptr       - Pointer to requested parameter structure
  rsp_ptr       - Pointer to results structure

DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_set_internal_runtime_settings
(
  uint32                       iface_inst,      /* Index for iface table   */
  qmi_wds_set_internal_runtime_settings_params_type   *req_ptr,
  qmi_wds_set_internal_runtime_settings_rsp_type      *rsp_ptr
);


/*===========================================================================
FUNCTION DSQMHLLIF_QUERY_BEARER_SETTINGS

DESCRIPTION
  This function commands QMI to query the Modem for the bearer
  technology.  The QMI Messaging Library performs this as a
  synchronous operation from caller's perspectve.

PARAMETERS
  iface_inst        - Index for iface table
  current_info_ptr  - Pointer to bearer settings structure

DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_query_bearer_settings
(
  uint32                       iface_inst,      /* Index for iface table   */
  dsqmh_runtime_info_type     *current_info_ptr
);


/*===========================================================================
FUNCTION DSQMHLLIF_QUERY_CHANNEL_SETTINGS

DESCRIPTION
  This function commands QMI to query the Modem for the channel
  information.  The QMI Messaging Library performs this as a
  synchronous operation from caller's perspectve.

PARAMETERS
  iface_inst        - Index for iface table
  current_info_ptr  - Pointer to channel settings structure

DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_query_channel_settings
(
  uint32                       iface_inst,      /* Index for iface table   */
  dsqmh_runtime_info_type     *current_info_ptr
);


/*===========================================================================
FUNCTION DSQMHLLIF_QUERY_QOS_SETTINGS

DESCRIPTION
  This function commands QMI to query the Modem for the QOS settings.
  The QMI Messaging Library performs this as a synchronous operation
  from caller's perspective.

PARAMETERS
  type              - Settings category to query
  obj_id            - IFACE/Flow identifier
  argval_ptr        - Pointer to result

DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_query_qos_settings
(
  dsqmhllif_qos_info_type  type,
  uint32                   iface_inst,          /* Index for iface table   */
  void                    *argval_ptr     
);


/*===========================================================================
FUNCTION DSQMHLLIF_MCAST_MANAGER

DESCRIPTION
  This function commands QMI to request the Modem to join, leave or
  register MCAST group(s).  The QMI Messaging Library performs this as
  a synchronous operation from caller's perspectve.

PARAMETERS
  ioctl_name        - PS IOCTL to be processed
  iface_inst        - Index for iface table
  mcast_info        - Pointer to MCAST specifications
  mcast_hndl_list   - Pointer to MCAST handles (OUT param)
  ps_errno          - Pointer to PS layer errno (OUT param)
  
DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_mcast_manager
(
  ps_iface_ioctl_type                  ioctl_name,
  uint32                               iface_inst, /* Index for iface table*/
  void                                *info_ptr,
  qmi_wds_mcast_hndl_list_type        *mcast_hndl_list,
  int16                               *ps_errno
);


/*===========================================================================
FUNCTION DSQMHLLIF_MBMS_MANAGER

DESCRIPTION
  This function commands QMI to request the Modem to activate or
  deactivate MBMS context.  The QMI Messaging Library performs this as
  a synchronous operation from caller's perspectve.

PARAMETERS
  operation         - Operation to be done
  iface_inst        - Index for iface table
  mbms_info         - Pointer to MBMS specification
  mbms_hndl         - Pointer to MBMS handle (OUT param)
  
DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_mbms_manager
(
  dsqmhllif_mbms_op                     operation,
  uint32                                iface_inst,/* Index for iface table*/
  qmi_wds_mbms_context_req_params_type *mbms_info,
  qmi_wds_mbms_context_handle          *mcast_hndl
);


/*===========================================================================
FUNCTION DSQMHLLIF_BCMCS_MANAGER

DESCRIPTION
  This function commands QMI to request the Modem to perform BCMCS
  operations.  The QMI Messaging Library performs this as a
  synchronous operation from caller's perspectve.

PARAMETERS
  operation         - Operation to be done
  iface_inst        - Index for iface table
  bcmcs_ptr         - Pointer to BCMCS info structure
  ps_errno          - Error code returned in case of failure
  
DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_bcmcs_manager
(
  dsqmhllif_bcmcs_op       operation,
  uint32                   iface_inst,/* Index for iface table*/
  void                    *bcmcs_info,
  sint15                  *ps_errno
);


/*===========================================================================
FUNCTION DSQMHLLIF_DORMANCY_MANAGER

DESCRIPTION
  This function commands QMI to request the Modem to enter/exit
  dormant state.  The QMI Messaging Library performs this as a
  synchronous operation from caller's perspectve.

PARAMETERS
  operation         - Operation to be done
  iface_inst        - Index for iface table
  
DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_dormancy_manager
(
  dsqmhllif_dormancy_op                operation,
  uint32                               iface_inst  /* Index for iface table*/
);

/*===========================================================================
FUNCTION DSQMHLLIF_CONFIGURE_IFACE_CMD

DESCRIPTION
  This function queries the Modem for the network interface settings,
  primiarily to retrieve the Um interface IP address.  An asynchronous
  message is posted to the host task for state machine processing.

PARAMETERS
  iface_inst  - Index for iface table

DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_configure_iface_cmd
(
  uint32        iface_inst                      /* Index for iface table   */
);
  

/*===========================================================================
FUNCTION DSQMHLLIF_IS_QMIPORT_INCALL

DESCRIPTION
  This function indirectly determines if QMI port is in data call.
  Using QMI channel rate query, the results are tested for zero values.
  Any non-zero value indicates active data call on QMI port.

PARAMETERS
  iface_inst  - Index for iface table
  inuse_ptr   - Pointer to query result

DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command processing.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_is_qmiport_incall
(
  uint32        iface_inst,                     /* Index for iface table   */
  boolean      *inuse_ptr
);


/*===========================================================================
FUNCTION DSQMHLLIF_QUERY_ROUTE_BY_POLICY

DESCRIPTION
  This function commands QMI to query the Modem for the candidate IFACE to 
  use for routing.  The best matching IFACE handle, priority, and RmNet 
  instance if IFACE is UP, are returned.

PARAMETERS
  proc_id           - Modem processor ID
  acl_pi_ptr        - Ptr to packet's policy information
  result_ptr        - Ptr to routing results

DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_query_route_by_policy
(
  acl_proc_id_type         proc_id,
  acl_policy_info_type    *acl_pi_ptr,
  acl_rt_result_type      *result_ptr
);


/*===========================================================================
FUNCTION DSQMHLLIF_NETPLATFORM_UP_CMD

DESCRIPTION
  This function commands the Network Platform layer to establish a
  connection instance for an active Modem connection.  The results of
  the mode processing are posted asynchronously via the callback
  registered with Platform layer.

PARAMETERS
  iface_inst  - Index for iface table

DEPENDENCIES
  dsqmhllif_init_netplatform() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_netplatform_up_cmd
(
  uint32        iface_inst                      /* Index for iface table   */
);


/*===========================================================================
FUNCTION DSQMHLLIF_NETPLATFORM_DOWN_CMD

DESCRIPTION
  This function commands the Network Platform layer to terminate a
  connection instance for an active Modem connection.  The results of
  the mode processing are posted asynchronously via the callback
  registered with Platform layer.

PARAMETERS
  iface_inst  - Index for iface table

DEPENDENCIES
  dsqmhllif_init_netplatform() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_netplatform_down_cmd
(
  uint32        iface_inst                      /* Index for iface table   */
);


/*===========================================================================
FUNCTION DSQMHLLIF_INIT_NETPLATFORM

DESCRIPTION
  This function initializes the Network Platform layer interface.

PARAMETERS
  iface_inst  - Index for iface table

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_init_netplatform
( 
  int   iface_inst
);


/*===========================================================================
FUNCTION dsqmhllif_qos_conv_flow_params_to_ps

DESCRIPTION
  This function converts the QMI Message Library flow parameters into that
  used by PS Flow QOS.

PARAMETERS
  qos_spec_tech    - Um IFACE Bearer technology (UMTS|CDMA)
  qmi_flow_ptr     - Pointer to QMI flow params (input)
  ip_flow_ptr      - Pointer to PS flow params (output)
  num_flows_ptr    - Pointer to number of requested flows (output)
  
DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/
void dsqmhllif_qos_conv_flow_params_to_ps
(
  qmi_qos_technology_type qos_spec_tech,     /* Bearer tech (UMTS|CDMA)    */
  const qmi_qos_flow_req_type *qmi_flow_ptr, /* Pointer to QMI flow params */
  ip_flow_type          *ip_flow_ptr,        /* Pointer to PS flow params  */
  uint32                *num_flows_ptr       /* Pointer to number of flows */
);


/*===========================================================================
FUNCTION DSQMHLLIF_DECODE_BEARER_TECH

DESCRIPTION
  This function determined the PS layer bearer technology info from QMI.

PARAMETERS
  current_nw   - Pointer to current network technology
  rat_mask     - Pointer to network radio access technology
  so_mask      - Pointer to network service option
  info_ptr     - Pointer to PS bearer technology info (OUT)

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_decode_bearer_tech
(
  const qmi_wds_data_bearer_type          *current_nw,
  const qmi_wds_db_rat_mask               *rat_mask,
  const qmi_wds_db_so_mask                *so_mask,
  ps_iface_bearer_technology_type   *info_ptr
);

/*===========================================================================
FUNCTION DSQMHLLIF_QUERY_INTERFACE_GW_ADDR

DESCRIPTION
  This function commands QMI to query the Modem for the Gateway
  IP address of the interface

PARAMETERS
  iface_inst          - Index for iface table
  gw_info             - Pointer to the gateway info
  ps_errno            - ps_errno is set in case of an error return,
                        correlates to values in dserrno.h
  
DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_query_interface_gw_addr
(
  uint32                       iface_inst,      /* Index for iface table   */
  ip_addr_type                 *gw_info,
  sint15                       *ps_errno
);

/*===========================================================================
FUNCTION DSQMHLLIF_INIT

DESCRIPTION
  This function initializes the lower-layer interface module.

PARAMETERS
  None.

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/
void dsqmhllif_init( void );

#endif /* FEATURE_DATA_PS && FEATURE_QMI_CLIENT */

#endif    /* DSQMH_LLIF_H */



