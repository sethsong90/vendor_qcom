/******************************************************************************

                        D C C _ T A S K _ D E F S  . H

******************************************************************************/

/******************************************************************************

  @file    dcc_task_defs.h
  @brief   dcc task definitions for Linux platform

  DESCRIPTION
  Header file for dcc task definitions for Linux platform

  ---------------------------------------------------------------------------
  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id:$

when       who        what, where, why
--------   ---        -------------------------------------------------------
11/09/09   js         Created

******************************************************************************/

#ifndef __DCC_TASK_DEFS_H__
#define __DCC_TASK_DEFS_H__

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
                     GLOBAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

typedef enum
{
  DCC_MIN_CMD                                  = -1,

#ifdef FEATURE_QMI_CLIENT
  /*-------------------------------------------------------------------------
    Command for QMH logical iface module
  -------------------------------------------------------------------------*/
  DCC_QMH_PROXY_IFACE_MSG_CMD     =  1,
#endif /* FEATURE_QMI_CLIENT */

  /*-------------------------------------------------------------------------
    Network interface - command to handle WWAN Rm state machine events
  -------------------------------------------------------------------------*/
  DCC_RMNET_SM_EV_CMD = 5,

  /*-------------------------------------------------------------------------
    Network interface - commands for RmNet
  -------------------------------------------------------------------------*/
  DCC_RMNET_SM_CMD = 6,

#ifdef FEATURE_DATA_QMI
  /*-------------------------------------------------------------------------
    Network interface - QMI receive command
  -------------------------------------------------------------------------*/
  DCC_QMUX_RX_CMD = 10,

  /*-------------------------------------------------------------------------
    Network interface - command to QMI module
  -------------------------------------------------------------------------*/
  DCC_QMI_CMD = 11,

  /*-------------------------------------------------------------------------
    Network interface - command to QMI Charger module
  -------------------------------------------------------------------------*/
  DCC_QMI_CHARGER_CMD = 12,

  DCC_CM_MSG_IFACE_CMD = 13,
  
  DCC_QMI_VOICE_CMD = 14,

  DCC_QMI_VOICE_CM_IF_CMD = 15,
  
  DCC_QMI_PBM_CMD = 16,

  DCC_QMI_PBM_IF_CMD = 17,
#endif /* FEATURE_DATA_QMI */

  /*-------------------------------------------------------------------------
    MIP CCoA cmds
  -------------------------------------------------------------------------*/
#if defined(FEATURE_DS_MOBILE_IP) && defined(FEATURE_DATA_PS_MIP_CCOA)
   DCC_MIPCCOA_START_CMD                       = 20,
   DCC_MIPCCOA_SIP_IFACE_UP_CMD                = 21,
   DCC_MIPCCOA_SIP_IFACE_UNAVAILABLE_CMD       = 22,
   DCC_MIPCCOA_MIP_UP_CMD                      = 23,
   DCC_MIPCCOA_MIP_FAILURE_CMD                 = 24,
#ifdef FEATURE_DS_MOBILE_IP_DEREG
   DCC_MIPCCOA_MIP_DEREGED_CMD                 = 25,
#endif
   DCC_MIPCCOA_CLOSE_CMD                       = 26,
#endif /* defined(FEATURE_DS_MOBILE_IP) && defined(FEATURE_DATA_PS_MIP_CCOA) */

#ifdef FEATURE_DATA_PS_MIPV6
  DCC_MIP6_MSM_START_CMD                       = 30,
  DCC_MIP6_MSM_BOOTSTRAP_SUCCESS_CMD           = 31,
  DCC_MIP6_MSM_BOOTSTRAP_FAIL_CMD              = 32,
  DCC_MIP6_MSM_SIP6_IFACE_UP_CMD               = 33,
  DCC_MIP6_MSM_SIP6_IFACE_UNAVAILABLE_CMD      = 34,
  DCC_MIP6_MSM_SIP6_IFACE_CONFIGURING_CMD      = 35,
  DCC_MIP6_MSM_SIP6_IFACE_PREFIX_UPDATE_CMD    = 36,
  DCC_MIP6_MSM_IPSEC_TPORT_IFACE_UP_CMD        = 37,
  DCC_MIP6_MSM_IPSEC_TPORT_IFACE_DOWN_CMD      = 38,
  DCC_MIP6_MSM_IPSEC_TUNNEL_IFACE_UP_CMD       = 39,
  DCC_MIP6_MSM_IPSEC_TUNNEL_IFACE_DOWN_CMD     = 40,
  DCC_MIP6_MSM_RSM_UP_CMD                      = 41,
  DCC_MIP6_MSM_RSM_DOWN_CMD                    = 42,
  DCC_MIP6_MSM_CLOSE_CMD                       = 43,
#endif /* FEATURE_DATA_PS_MIPV6 */

#ifdef FEATURE_DATA_PS_IWLAN
  /*-------------------------------------------------------------------------
    IWLAN IFACE commands.
  -------------------------------------------------------------------------*/
  DCC_IWLAN_IFACE_BRING_UP_CMD      = 50,   /* bring up IWLAN iface      */
  DCC_IWLAN_IFACE_TEAR_DOWN_CMD     = 51,   /* tear down IWLAN iface     */
  DCC_IWLAN_IFACE_IPSEC_UP_EV_CMD   = 52,   /* IPSEC iface is UP         */
  DCC_IWLAN_IFACE_IPSEC_DOWN_EV_CMD = 53,   /* IPSEC iface is DOWN       */
  DCC_IWLAN_IFACE_WLAN_UP_EV_CMD    = 54,   /* WLAN iface is UP          */
  DCC_IWLAN_IFACE_WLAN_DOWN_EV_CMD  = 55,   /* WLAN iface is DOWN        */
  DCC_IWLAN_IFACE_DNS_CB_CMD        = 56,   /* DNS Callback              */
#ifdef FEATURE_DATA_PS_IWLAN_3GPP
  DCC_IWLAN_MMGSDI_CMD              = 57,   /* MMSGSDI API callbacks     */
#endif /* FEATURE_DATA_PS_IWLAN_3GPP */
#endif /* FEATURE_DATA_PS_IWLAN */

#ifdef FEATURE_DATA_PS_UICC
  /*-------------------------------------------------------------------------
    UICC command
  -------------------------------------------------------------------------*/
  DCC_UICC_CMD                       = 60,   /* UICC N/W interface cmds  */
  DCC_UICC_SM_CMD                    = 61,   /* UICC State Machine cmds  */
#endif /* FEATURE_DATA_PS_UICC */

#ifdef FEATURE_UW_FMC
  /*-------------------------------------------------------------------------
    UW FMC commands
  -------------------------------------------------------------------------*/
  DCC_UW_FMC_SM_START_EV_CMD        = 70,  /* Bring up UW FMC iface      */
  DCC_UW_FMC_SM_STOP_EV_CMD         = 71,  /* Tear down UW FMC iface     */
  DCC_UW_FMC_SM_ABORT_EV_CMD        = 72,  /* Abort UW FMC iface         */
  DCC_UW_FMC_NET_UP_WITH_SIP_EV_CMD = 73,  /* UW FMC PPP dev is UP       */
  DCC_UW_FMC_NET_DOWN_EV_CMD        = 74,  /* UW FMC PPP dev is DOWN     */
  DCC_UW_FMC_NET_RESYNC_EV_CMD      = 75,  /* UW FMC PPP dev is RESYNCing */
  DCC_UW_FMC_READ_EV_CMD            = 76,  /* Read data on UW FMC iface  */
  DCC_UW_FMC_WRITE_EV_CMD           = 77,  /* Write data on UW FMC iface */
#endif /* FEATURE_UW_FMC */

  /*-------------------------------------------------------------------------
    SLIP IFACE call control commands
  -------------------------------------------------------------------------*/
#ifdef FEATURE_DATA_PS_SLIP
  DCC_SLIP_IFACE_HDLR_CP_DEACTIVATING_EV_CMD     = 80,
  DCC_SLIP_IFACE_HDLR_CP_DOWN_EV_CMD             = 81,
  DCC_SLIP_IFACE_HDLR_CP_UP_EV_CMD               = 82,
  DCC_SLIP_IFACE_HDLR_ADDR_FAILURE_EV_CMD        = 83,
  DCC_SLIP_IFACE_HDLR_ADDR_RELEASED_EV_CMD       = 84,
  DCC_SLIP_IFACE_HDLR_ADDR_CONFIG_SUCCESS_EV_CMD = 85,
  
#endif /* FEATURE_DATA_PS_SLIP */

  /*-------------------------------------------------------------------------
    Command for processing Logical Iface Callback
  -------------------------------------------------------------------------*/
  DCC_LOGICAL_IFACE_ASSOC_IFACE_EV_CMD  = 86,

  /*-------------------------------------------------------------------------
    Command for processing associated flow events in the logical flow framework
  -------------------------------------------------------------------------*/
  DCC_LOGICAL_FLOW_ASSOC_FLOW_EV_CMD    = 87,

  /*-------------------------------------------------------------------------
    NAT iface control/ALG specific commands
  -------------------------------------------------------------------------*/
#ifdef FEATURE_DATA_PS_SOFTAP
  DCC_NAT_IFACE_UM_EV_CMD = 88,
  DCC_NAT_IFACE_AUTOCONNECT_TIMEOUT_INFO_CMD = 89,
#ifdef FEATURE_DATA_PS_FTP_ALG
  DCC_FTP_ALG_CLIENT_CMD  = 90,
#endif /*FEATURE_DATA_PS_FTP_ALG*/
#endif /* FEATURE_DATA_PS_SOFTAP */
  /*-------------------------------------------------------------------------
    QMI Service init and Request cmd
  -------------------------------------------------------------------------*/
  DCC_QMI_INIT_LEGACY_SERVICES_CMD     = 91,
  DCC_QMI_RECV_LEGACY_SERVICES_REQ_CMD = 92,
  DCC_RMNET_INST_OPEN_CLOSE_CMD        = 93,

#ifdef FEATURE_DATA_PS_SOFTAP
  DCC_NAT_IFACE_NEW_CM_SS_INFO_CMD = 94,
  DCC_NAT_IFACE_UPDATE_ROAMING_AUTOCONNECT_CONFIG_CMD = 95,
#endif /* FEATURE_DATA_PS_SOFTAP */

  DCC_DSNET_PROCESS_GENERIC_EVENT_CMD,
  DCC_DSSOCK_PROCESS_GENERIC_EVENT_CMD,
  DCC_DSSOCK_PROCESS_DOS_ACK_EVENT_CMD,
  DCC_DSS_NET_MGR_NET_UP_CMD,
  DCC_DSS_NET_MGR_NET_DOWN_CMD,
  DCC_DNS_RESOLVE_CMD,
  DCC_DNS_IO_MGR_SOCK_EVENT_CMD,
  DCC_DNS_RESOLVER_TIMEOUT_CMD,
  DCC_DNS_DELETE_SESSION_CMD,
  DCC_TIMER_CALLBACK_CMD,
  DCC_STAT_INST_GET_DESC_CMD,
  DCC_STAT_INST_GET_STAT_CMD,
  DCC_STAT_INST_RESET_STAT_CMD,
   
  /* Always must be last */
  DCC_MAX_DEFINED_CMD_TYPES
} dcc_cmd_enum_type;

/*--------------------------------------------------------------------------- 
   Structure representing a generic command
---------------------------------------------------------------------------*/
typedef struct dcc_cmd_data_buf_s {
    dcc_cmd_enum_type type;
    void            * data;
} dcc_cmd_data_buf_type;

typedef void (*dcc_cmd_handler_type)(dcc_cmd_enum_type, void *);

typedef struct {
  dcc_cmd_enum_type e;
  dcc_cmd_handler_type h;
} dcc_enum_handler_map_type;

/*===========================================================================
                     GLOBAL FUNCTION DECLARATIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  dcc_cmdq_enq
===========================================================================*/
/*!
@brief
  Used by clients to enqueue a command to the Command Thread's list of 
  pending commands and execute it in the Command Thread context.  

@return
  void 

@note

  - Dependencies
    - Assumes Command Thread has been initialized and is running.  

  - Side Effects
    - None
*/
/*=========================================================================*/
void dcc_cmdq_enq (const dcc_cmd_data_buf_type * cmd);

/*===========================================================================
  FUNCTION  dcc_set_cmd_handler
===========================================================================*/
/*!
@brief
  Sets internal mapping of e to h

@return
  dcc_cmd_handler_type - Previous handler value, NULL otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - This mapping is used by dcc_send_cmd to set dcc_cmd_execute_f
      in dcc_cmd_t before calling onto dcc_cmdq_enq
*/
/*=========================================================================*/
dcc_cmd_handler_type dcc_set_cmd_handler
(
  dcc_cmd_enum_type e,
  dcc_cmd_handler_type h
);

/*===========================================================================
  FUNCTION  dcc_get_cmd_data_buf
===========================================================================*/
/*!
@brief
  gets command of type dcc_cmd_t from global heap

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - allocates memory that needs to be freed later
*/
/*=========================================================================*/
dcc_cmd_data_buf_type * dcc_get_cmd_data_buf(void);

/*===========================================================================
  FUNCTION  dcc_free_cmd_data_buf
===========================================================================*/
/*!
@brief
  frees command of type dcc_cmd_t to global heap

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void dcc_free_cmd_data_buf(dcc_cmd_data_buf_type * cmd);

/*===========================================================================
  FUNCTION  dcc_send_cmd
===========================================================================*/
/*!
@brief
  Usees the dcc_e_h_map variable to find out execute_f to be used
  with dcc_cmd_t command to be sent to the command threaqd

@return
  void

@note

  - Dependencies
    - dcc_set_cmd_handler must have been called

  - Side Effects
    - None
*/
/*=========================================================================*/
void dcc_send_cmd(dcc_cmd_enum_type e, void *user_data);

/*===========================================================================

FUNCTION DCC_SEND_CMD_EX()

DESCRIPTION
  This function posts a cmd for processing in DCC task context.  The cmd is
  processed by calling the registered cmd handler, if any.

  This API is mainly for clients which allocates  user_data_ptr from their
  own memory [not by dcc_get_cmd_buf]. Henceforth, if the client uses this
  API, it must free the memory after processing the command.

  Eventually all clients must move to this configuration.

  NOTE: The passed command will be copied to a DCC task
  command buffer local to the DCC Task.

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.
===========================================================================*/
#define dcc_send_cmd_ex dcc_send_cmd


/*===========================================================================
  FUNCTION  dcc_cmdthrd_init
===========================================================================*/
/*!
@brief
  Function for initializing and starting Command Thread. Must be called 
  before clients can post commands for execution in Command Thread context. 

@return
  void 

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
void dcc_cmdthrd_init(void);

/*===========================================================================
  FUNCTION  dcc_cmdthrd_deinit
===========================================================================*/
/*!
@brief
  Function for teardown of Command Thread.

@return
  void 

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
void dcc_cmdthrd_deinit (void);

#ifdef __cplusplus
}
#endif

#endif /* __DCC_TASK_DEFS_H__ */
