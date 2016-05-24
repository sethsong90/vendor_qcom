#ifndef DSQMHI_H
#define DSQMHI_H
/*===========================================================================

                 Q U A L C O M M   M S M   I N T E R F A C E

                            M O D E   H A N D L E R

                   I N T E R N A L  H E A D E R   F I L E


DESCRIPTION
  This file contains private data declarations and function prototypes
  for the QMI Mode Handler.

Copyright (c) 2008 -  2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datahlos/interface/qmiifacectls/rel/11.01.01/src/ds_qmhi.h#3 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
04/13/11    hm     Multi-modem support merged from linux QMH
10/19/10    sy     Replaced DCC cmd bufs with client allocated memory.
10/04/10    sy     Added dsqmh_qos_supported_enum_type.
07/02/10    hm     Add support for fusion target.
05/24/10    ss     Added SLIP Tech Pref in DSQMH_SET_TECH_PREF macro.
09/30/09    ar     Add IPV6 ND control block for ICMPv6 logic
06/24/09    am     Renamed hton/ntoh macros with ps_ prefix.
02/19/09    am     DS Task De-coupling effort and introduction of DCC task.
02/15/09    ar     Add IPv6 support.
01/16/09    ar     Fix Lint and MOB integration issues.
11/24/08    ar     Adjust for extended tech pref convention change
07/21/08    ar     Relocate typedefs from public header.
05/06/08    ar     Created module/initial version.

===========================================================================*/


/*===========================================================================

                           INCLUDE FILES

===========================================================================*/

#include "comdef.h"
#include "customer.h"


#if defined (FEATURE_DATA_PS) && defined (FEATURE_QMI_CLIENT)

#include "dsm.h"
#ifndef FEATURE_DSS_LINUX
#include "sio.h"
#include "smd.h"
#include "data_msg.h"
#include "dcc_task_svc.h"
#ifdef FEATURE_DATA_PS_IPV6
#include "ps_ip6_sm.h"
#include "ps_lan_llc.h" 
#endif
#endif /* FEATURE_DSS_LINUX */
#include "dssocket_defs.h"
#include "stm2.h"
#include "ps_mem.h"
#include "ps_svc.h"
#include "ps_iface.h"
#include "ps_aclrules.h"
#include "ps_route.h"
#include "ps_phys_link_ioctl.h"

#include "ds_Utils_DebugMsg.h"

#include "ds_qmh.h"
#include "ds_qmh_config.h"
#include "ds_qmh_netplat.h"
#include "qmi_platform_config.h"
#include "qmi.h"
#include "qmi_wds_srvc.h"
#include "qmi_qos_srvc.h"
#include "dcc_task_defs.h"
#include "ps_crit_sect.h"

/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/


/*---------------------------------------------------------------------------
  Module magic values macros.
---------------------------------------------------------------------------*/
#define DSQMH_INVALID_MODEMID  (-1)
#define DSQMH_INVALID_IFACEID  (-1)
#define DSQMH_INVALID_PROFID   (-1)
#define DSQMH_INVALID_RMNET    (-1)
#define DSQMH_INVALID_TXNID    (-1)
#define DSQMH_INVALID_HANDLE   (-1)
#define DSQMH_INVALID_TECHPREF (0xFF)

/*---------------------------------------------------------------------------
  Module DIAG message macros.
---------------------------------------------------------------------------*/
#define DSQMH_MSG3(xx_ss_mask, xx_fmt, xx_arg1, xx_arg2, xx_arg3)          \
    PRINT_MSG( xx_ss_mask, xx_fmt, xx_arg1, xx_arg2, xx_arg3 );
#define DSQMH_MSG6(xx_ss_mask, xx_fmt, xx_arg1, xx_arg2, xx_arg3,          \
                                       xx_arg4, xx_arg5, xx_arg6)          \
    PRINT_MSG_6( xx_ss_mask, xx_fmt, xx_arg1, xx_arg2, xx_arg3,            \
                                     xx_arg4, xx_arg5, xx_arg6 );


#define DSQMH_MSG_LOW(fmt,a,b,c)    DSQMH_MSG3(MSG_LEGACY_LOW,fmt,a,b,c)
#define DSQMH_MSG_MED(fmt,a,b,c)    DSQMH_MSG3(MSG_LEGACY_MED,fmt,a,b,c)
#define DSQMH_MSG_HIGH(fmt,a,b,c)   DSQMH_MSG3(MSG_LEGACY_HIGH,fmt,a,b,c)
#define DSQMH_MSG_ERROR(fmt,a,b,c)  DSQMH_MSG3(MSG_LEGACY_ERROR,fmt,a,b,c)
#define DSQMH_MSG_FATAL(fmt,a,b,c)  DSQMH_MSG3(MSG_LEGACY_FATAL,fmt,a,b,c)

#define DSQMH_MSG_IPV4_ADDR(prefix,ip_addr)                                \
        PRINT_MSG_6( LOG_MSG_INFO1_LEVEL, prefix                           \
                     "IPV4 Address is %d.%d.%d.%d",                        \
                     (unsigned char)(ip_addr),                             \
                     (unsigned char)(ip_addr >> 8),                        \
                     (unsigned char)(ip_addr >> 16) ,                      \
                     (unsigned char)(ip_addr >> 24),                       \
                     0,                                                    \
                     0 );                                                    

#define DSQMH_MSG_IPV6_ADDR(prefix,ip_addr)                                \
        MSG_8( MSG_SSID_DS_APPS,                                           \
               LOG_MSG_INFO1_LEVEL, prefix                                 \
               "IPV6 Address %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",     \
               (uint16)(ps_ntohs(ip_addr[0])),                             \
               (uint16)(ps_ntohs(ip_addr[0] >> 16)),                       \
               (uint16)(ps_ntohs(ip_addr[0] >> 32)) ,                      \
               (uint16)(ps_ntohs(ip_addr[0] >> 48)),                       \
               (uint16)(ps_ntohs(ip_addr[1])),                             \
               (uint16)(ps_ntohs(ip_addr[1] >> 16)),                       \
               (uint16)(ps_ntohs(ip_addr[1] >> 32)) ,                      \
               (uint16)(ps_ntohs(ip_addr[1] >> 48)) );                       


#define DSQMH_TEST(test,fmt)              \
          if( !(test) ) {                 \
            DSQMH_MSG_ERROR( fmt,0,0,0 ); \
            return DSQMH_FAILED;          \
          }

#define DSQMH_ASSERT(test,fmt)            \
          if( !(test) ) {                 \
            DSQMH_MSG_FATAL( fmt,0,0,0 ); \
            ASSERT( 0 ); \
          }

/*---------------------------------------------------------------------------
  Asynchronous command buffer macros.
---------------------------------------------------------------------------*/

#define DSQMH_GET_MSG_BUF(msg_ptr)                                          \
    msg_ptr = ps_mem_get_buf( PS_MEM_QMH_MSG_BUF_TYPE );                    \
    DSQMH_ASSERT( msg_ptr, "Failed to allocate message buffer");            \
    DSQMH_MSG_LOW( "DSQMH_GET_MSG_BUF: msg=0x%p", msg_ptr, 0, 0 );          

#define DSQMH_RELEASE_MSG_BUF(msg_ptr)                                      \
    if( msg_ptr ) {                                                         \
        DSQMH_MSG_LOW( "DSQMH_RELEASE_MSG_BUF: msg=0x%p", msg_ptr, 0, 0 );  \
        PS_MEM_FREE( msg_ptr );                                             \
      }

#define DSQMH_PUT_MSG_BUF(msg_ptr)                                       \
    if( msg_ptr ) {                                                      \
      dcc_send_cmd( DCC_QMH_PROXY_IFACE_MSG_CMD, msg_ptr );              \
    }

/*---------------------------------------------------------------------------
  IFACE configuration macros.
---------------------------------------------------------------------------*/
#define DSQMH_MAX_PS_IFACES                 8
#define DSQMH_MAX_PHYSLINKS_PER_IFACE      (1)
#define DSQMH_MAX_PHYSLINKS \
        (DSQMH_MAX_PS_IFACES * DSQMH_MAX_PHYSLINKS_PER_IFACE)
#define DSQMH_DEFAULT_IFACE_PHYSLINK       (0)

#define IS_IFACE_INST_VALID(i) (DSQMH_MAX_PS_IFACES > (uint32)i)

/* Define ACL routing Processor ID.  This is used for policy routing
 * requests.  It is offset into the routing layer callback table. */
#define ACL_PROC_QCTMSM0 (0)
#define ACL_PROC_QCTMSM1 (1)


#ifndef FEATURE_DSS_LINUX
/*---------------------------------------------------------------------------
  SIO watermark queue configuration macros.
---------------------------------------------------------------------------*/
#define DSQMH_QUEUE_UL_WM_LO     (1514)   /* 1 packet  */
#define DSQMH_QUEUE_UL_WM_HI     (3028)   /* 2 packets */
#define DSQMH_QUEUE_UL_WM_DNE    (6056)   /* 4 packets */
#define DSQMH_QUEUE_DL_WM_LO     (1514)   /* 1 packet  */
#define DSQMH_QUEUE_DL_WM_HI     (6056)   /* 4 packets */
#define DSQMH_QUEUE_DL_WM_DNE    (9084)   /* 6 packets */
#endif

/* Maximum number of QOS flow specs (CDMA supprots 10) */
#define DSQMH_MAX_QOS_FLOWS               (10)

/*---------------------------------------------------------------------------
  Data structures for QMI Mode Hander asynchronous messages
---------------------------------------------------------------------------*/

extern ps_crit_sect_type dsqmh_init_crit_section;

typedef enum
{
   PROXY_IFACE_NULL_CMD             = 0     /* Null command                */

  /* Client control commands */
  ,PROXY_IFACE_MODEM_INIT_CMD       = 1     /* Initialize modem            */
  ,PROXY_IFACE_BRING_UP_CMD         = 2     /* Bring up IFACE              */
  ,PROXY_IFACE_TEARDOWN_CMD         = 3     /* Teardown IFACE              */
  ,PROXY_PHYS_LINK_UP_CMD           = 4     /* Bring up Physical Link      */
  ,PROXY_PHYS_LINK_DOWN_CMD         = 5     /* Teardown Physical Link      */
  ,PROXY_PHYS_LINK_FLOW_DISABLE_CMD = 6     /* Suspend Physical Link flow  */
  ,PROXY_PHYS_LINK_FLOW_ENABLE_CMD  = 7     /* Resume Physical Link flow   */
  ,PROXY_QOS_FLOW_REQUEST_CMD       = 8     /* Create QOS flow             */
  ,PROXY_QOS_FLOW_RELEASE_CMD       = 9     /* Delete QOS flow             */
  ,PROXY_QOS_FLOW_SUSPEND_CMD       = 10     /* Suspend QOS flow            */
  ,PROXY_QOS_FLOW_RESUME_CMD        = 11    /* ResumeQOS flow              */

  ,PROXY_QMI_LIB_INIT_CMD           = 15    /* Initialize QMI Msg Library  */

  /* External component indications */
  ,PROXY_IFACE_MODEM_INIT_IND       = 20    /* Modem init complete         */
  ,PROXY_IFACE_MODEM_UP_IND         = 21    /* Modem channel activated     */
  ,PROXY_IFACE_MODEM_DOWN_IND       = 22    /* Modem channel deactivated   */
  ,PROXY_IFACE_PLATFORM_UP_IND      = 23    /* Network platform activated  */
  ,PROXY_IFACE_PLATFORM_DOWN_IND    = 24    /* Network platform deactivated*/
  ,PROXY_IFACE_CONFIGURED_IND       = 25    /* Modem IP address assigned   */
  ,PROXY_IFACE_MODEM_QOS_IND        = 26    /* Modem QOS notification      */
  ,PROXY_IFACE_MODEM_EVENT_IND      = 27    /* Modem WDS event report      */
  ,PROXY_IFACE_MODEM_INTERNAL_IND   = 28    /* Modem WDS int iface event   */
  ,PROXY_IFACE_MODEM_MCAST_IND      = 29    /* Modem WDS mcast event       */
  ,PROXY_IFACE_MODEM_BCMCS_IND      = 30    /* Modem WDS mcast event       */
  ,PROXY_IFACE_MODEM_MTREQ_IND      = 31    /* Modem WDS MT call request   */
  ,PROXY_IFACE_PLATFORM_QOS_IND     = 35    /* Network Platform QOS event  */

  ,PROXY_IFACE_MAX_CMD                      /* Internal use only           */
} dsqmh_msg_id_type;

typedef struct dsqmh_qmi_wds_ind_s {
  qmi_wds_indication_id_type   ind_id;      /* Indication identifier       */
  qmi_wds_indication_data_type info;        /* Indication info             */
} dsqmh_qmi_wds_ind_type;

typedef struct dsqmh_qmi_qos_ind_s {
  qmi_qos_indication_id_type   ind_id;      /* Indication identifier       */
  qmi_qos_indication_data_type info;        /* Indication info             */
} dsqmh_qmi_qos_ind_type;

typedef struct dsqmh_netplatform_ind_s {
  ds_netplat_ind_msg_type      ind_id;      /* Indication identifier       */
  ds_netplat_info_type         info;        /* Indication info             */
  ps_flow_type                *flow_ptr;    /* QOS Flow reference          */
} dsqmh_netplatform_ind_type;

typedef struct dsqmh_ioctl_info_s {
  ps_flow_type                *flows_pptr[DSQMH_MAX_QOS_FLOWS];   
                                            /* QOS Flow list               */
  uint32                       num_flows;   /* QOS Flow list length        */
} dsqmh_ioctl_info_type;

typedef enum {
  DSQMH_PHYSLINK_CMD_NULL         = 0xFF00,
  DSQMH_PHYSLINK_CMD_DOWN         = 0xFF01,
  DSQMH_PHYSLINK_CMD_DOWN_DORMANT = 0xFF02,
  DSQMH_PHYSLINK_CMD_UP           = 0xFF03,
  DSQMH_PHYSLINK_CMD_MAX          = 0x7FFFFFFF         /* Force 32bit enum */
} dsqmh_physlink_cmd_type;

typedef struct dsqmh_physlink_info_s {
  dsqmh_physlink_cmd_type cmd;              /* Phys Link action command    */
} dsqmh_physlink_info_type;

typedef struct dsqmh_msg_buf_s {
  dsqmh_msg_id_type    msg_id;              /* Input message identifier    */
  uint32               iface_inst;          /* IFACE instance identifier   */
  ps_phys_link_type   *phys_link_ptr;       /* Physical link reference     */
  void                *user_data_ptr;       /* User context value          */
  union info_u
  {
    dsqmh_qmi_wds_ind_type          qmi_wds;  /* QMI WDS indication info   */
    dsqmh_qmi_qos_ind_type          qmi_qos;  /* QMI QOS indication info   */
    dsqmh_netplatform_ind_type      netplat;  /* Netowork Platform info    */
    dsqmh_ioctl_info_type           ioctl;    /* IOCTL info                */
    dsqmh_physlink_info_type        physlink; /* Phys Link info            */
  } info;
} dsqmh_msg_buf_type;

typedef enum 
{
  DSQMH_QOS_SUPPORT_UNKNOWN     = 0,
  DSQMH_QOS_SUPPORT_AVAILABLE   = 1,
  DSQMH_QOS_SUPPORT_UNAVAILABLE = 2
} dsqmh_qos_supported_enum_type;


/*---------------------------------------------------------------------------
  Data structure for QMI Message Library state

  Connections to both the WDS and QOS services are established on
  IFACE initialization.  Each is mapped to a fixed RnNet IFACE
  instance on the Modem.  Most WDS operations are asynchronous so we
  track the transaction ID here.
---------------------------------------------------------------------------*/
typedef struct {
  char        *dev_id;                    /* QMI device ID                 */
  int          wds_handle;                /* WDS client ID                 */
  int          wds_txn_handle;            /* WDS pending transaction handle*/
  int          qos_handle;                /* QOS client ID                 */
  boolean      qoshdr_enabled;            /* QOS metadata header enabled   */
} dsqmh_msglib_info_type;


/*---------------------------------------------------------------------------
  Data structures for Network Platform Layer state
---------------------------------------------------------------------------*/
typedef struct {
  int32         conn_id;
} dsqmh_netplat_info_type;


#ifndef FEATURE_DSS_LINUX
/*---------------------------------------------------------------------------
  Data structures for datapath SMD information

  Each phys link uses a dedicated SMD port for datapath.  DSM watermark
  queues are used between SMD and PS task to buffer packets in both Tx and
  Rx directions.
---------------------------------------------------------------------------*/
typedef struct {
  sio_port_id_type   port_id;          /* SMD prot identifier              */
  sio_stream_id_type stream_id;        /* SIO stream identifier            */
  q_type             ps_tx_q;          /* PS to SMD Transmit Queue         */
  dsm_watermark_type ps_tx_wm;         /* PS to SMD Transmit watermark     */
  uint32             tx_flow_mask;     /* Track flow control requestors    */
  q_type             ps_rx_q;          /* SMD to PS Receive Queue          */
  dsm_watermark_type ps_rx_wm;         /* SMD to PS Receive watermark      */
  uint32             rx_flow_mask;     /* Track flow control requestors    */
} dsqmh_smd_info_type;
#endif

#ifndef FEATURE_DSS_LINUX
#ifdef FEATURE_DATA_PS_IPV6
/*---------------------------------------------------------------------------
  Data structure for per-Iface IPV6 information
---------------------------------------------------------------------------*/
typedef struct {
  ip6_sm_type               ip6_sm;           /* IPV6 state machine        */

  void                     *event_buf_ptr;    /* IPV6 address event buffer */

  boolean                   dad_req;          /* IPV6 DAD underway         */

  lan_lle_enum_type         llc_inst;         /* Logical Link Entity       */
  lan_llc_start_info_type   llc_info;         /* LLC info (for ND)         */
} dsqmh_ipv6_info_type;
#endif /* FEATURE_DATA_PS_IPV6 */
#endif /* FEATURE_DSS_LINUX */


/*---------------------------------------------------------------------------
  Data structures for Proxy IFACE control block.
  
  Each PS IFACE supports only one phys link, which represents the
  shared memory channel to the Modem.  A dedicated connection to the
  QMI messaging Library is maintained for each IFACE. 
---------------------------------------------------------------------------*/

typedef struct {
#ifndef FEATURE_DSS_LINUX
  dsqmh_smd_info_type       smd_info;         /* Shared Memory Driver info */
#endif

  boolean                   bringup_aborted;  /* Flag for aborted bringup  */
  boolean                   is_dormant;       /* Flag for dormant state    */
} dsqmh_phys_link_info_type;


typedef struct {
  ps_iface_type             ps_iface;         /* IFace state info          */
  
  acl_type                  acl;              /* ACL Info of the iface     */
  
  acl_policy_info_type      policy_info;      /* Sockets policy info       */

  ps_phys_link_type         phys_links[DSQMH_MAX_PHYSLINKS_PER_IFACE];
                                              /* PS Phys link info         */
  dsqmh_phys_link_info_type phys_link_info[DSQMH_MAX_PHYSLINKS_PER_IFACE];
                                              /* Proxy Phys link info      */
  
  stm_state_machine_t      *sm_ptr;           /* IFACE state machine       */
  
#ifndef FEATURE_DSS_LINUX
#ifdef FEATURE_DATA_PS_IPV6
  dsqmh_ipv6_info_type      ipv6_info;        /* IPV6 info                 */
#endif /* FEATURE_DATA_PS_IPV6 */
#endif /* FEATURE_DSS_LINUX */
  
  boolean                   reconfig_req;     /* IFACE Reconfig underway   */

  dsqmh_netplat_info_type   netplat_info;     /* Network Platform Layer info*/

  ps_iface_bearer_technology_type um_bearer_tech;
                                              /* Um IFACE bearer(UMTS|CDMA)*/

  dsqmh_msglib_info_type    qmi_msglib_info;  /* QMI Messaging Library info*/

  ps_iface_net_down_reason_type down_reason;  /* Mapped QMI Call end reason*/
  
  dsqmh_qos_supported_enum_type qos_supported;/* QoS aware/unaware         */

  boolean                   pkt_srvc_ind_expected; /* Ignore pkt srvc ind? */
} dsqmh_iface_cblk_type;


/*---------------------------------------------------------------------------
  Data structure for module state information.
  
  We support DSQMH_MAX_PS_IFACES number of PS ifaces. 
---------------------------------------------------------------------------*/
typedef struct {
  dsqmh_iface_cblk_type     proxy_iface_tbl[DSQMH_MAX_PS_IFACES];
                                            /* Proxy IFACE table           */

  int32                     supported_call_types; /* Local/Embedded/Teth   */
  int                       qmi_lib_hndl;   /* QMI library handle          */
  boolean                   self_init;      /* Powerup init done?          */
  boolean                   conn_init;      /* QMI Connection init done?   */
#ifndef FEATURE_DSS_LINUX
#ifdef FEATURE_DATA_PS_IPV6
  struct ipv6_info_s {
    boolean                 enabled;        /* IPV6 support enabled?       */
    ip6_sm_config_type      sm_config;      /* IPV6 state machine config   */
  } ipv6_info;
#endif /* FEATURE_DATA_PS_IPV6 */
#endif /* FEATURE_DSS_LINUX */
  
} dsqmh_module_info_type;    

extern dsqmh_module_info_type dsqmh_state_info;


/*---------------------------------------------------------------------------
  Data structures for current runtime settings results
---------------------------------------------------------------------------*/
typedef struct dsqmh_runtime_info_s {
  qmi_wds_profile_id_type            profile_id;
  qmi_wds_profile_params_type        profile_params;
  qmi_wds_curr_call_info_type        call_info;
  qmi_wds_data_bearer_tech_type      bearer_info;
  qmi_wds_channel_rate_type          channel_info;
} dsqmh_runtime_info_type;

/* Critical section for SMD port resources */
extern ps_crit_sect_type dsqmhllif_smd_crit_section;

#define DSQMHLLIF_ENTER_SMD_CRIT_SECTION()\
        PS_ENTER_CRIT_SECTION( &dsqmhllif_smd_crit_section );
#define DSQMHLLIF_EXIT_SMD_CRIT_SECTION()\
        PS_LEAVE_CRIT_SECTION( &dsqmhllif_smd_crit_section );


/*===========================================================================
MACRO DSQMH_IS_IFACE_INST_VALID

DESCRIPTION   
  Given the iface instance, ensure it is in the range of supported values.

DEPENDENCIES  
  None.

RETURN VALUE  
  TRUE is iface inst is valid, FALSE otherwise.
  
SIDE EFFECTS  
  None.
===========================================================================*/
#define DSQMH_IS_IFACE_INST_VALID(i)\
        ((uint32)i < DSQMH_MAX_PS_IFACES)



/*===========================================================================
MACRO DSQMH_GET_CBLK_PTR

DESCRIPTION   
  Given the iface instance, returns the control block pointer.

DEPENDENCIES  
  None.

RETURN VALUE  
  See Descritpion.
  
SIDE EFFECTS  
  None.
===========================================================================*/
#define DSQMH_GET_CBLK_PTR(i)\
        &(dsqmh_state_info.proxy_iface_tbl[i])


/*===========================================================================
MACRO DSQMH_GET_IFACE_PTR

DESCRIPTION   
  Given the iface instance, returns the ps iface_ptr.

DEPENDENCIES  
  None.

RETURN VALUE  
  See Descritpion.
  
SIDE EFFECTS  
  None.
===========================================================================*/
#define DSQMH_GET_IFACE_PTR(i)                                              \
        (DSQMH_IS_IFACE_INST_VALID(i)?                                      \
         &(dsqmh_state_info.proxy_iface_tbl[i].ps_iface) : NULL)

#define DSQMH_GET_IFACE_PTR_FROM_FLOW(fptr,iptr)                            \
    { ps_phys_link_type *phys_link_ptr = PS_FLOW_GET_PHYS_LINK((fptr));     \
      iptr = (phys_link_ptr)?                                               \
             DSQMH_GET_IFACE_PTR((uint32)(phys_link_ptr->client_data_ptr)): \
             NULL;                                                          \
    }

/*===========================================================================
MACRO DSQMH_GET_IFACE_INST_FROM_CONN_ID

DESCRIPTION
  Given the QMI connection ID, return the corresponding iface_inst. This
  is currently using an internal implementation detail. Need to correct
  this.

DEPENDENCIES
  None.

RETURN VALUE
  See Descritpion.

SIDE EFFECTS
  None.
===========================================================================*/
/* TODO: Using internal impl detail - need to fix this */
#define DSQMH_GET_IFACE_INST_FROM_CONN_ID(conn_id)  (conn_id)


/*===========================================================================
MACRO DSQMH_GET_PHYS_LINKS

DESCRIPTION   
  Given the iface instance, returns the pointer to the phys links array.

DEPENDENCIES  
  None.

RETURN VALUE  
  See Descritpion.
  
SIDE EFFECTS  
  None.
===========================================================================*/
#define DSQMH_GET_PHYS_LINKS(i)                                             \
        (DSQMH_IS_IFACE_INST_VALID(i)?                                      \
         dsqmh_state_info.proxy_iface_tbl[i].phys_links : NULL)


/*===========================================================================
MACRO DSQMH_GET_PHYS_LINK_PTR

DESCRIPTION   
  Given the iface instance and phys link index, returns the a pointer to
  the phys link.

DEPENDENCIES  
  None.

RETURN VALUE  
  See Descritpion.
  
SIDE EFFECTS  
  None.
===========================================================================*/
#define DSQMH_GET_PHYS_LINK_PTR(i,p)\
   ((DSQMH_MAX_PS_IFACES > i) && (DSQMH_MAX_PHYSLINKS_PER_IFACE > p))?   \
     &(dsqmh_state_info.proxy_iface_tbl[i].phys_links[p]) : \
     NULL

#define DSQMH_GET_PHYS_LINK_INFO_PTR(i,p)\
    &(dsqmh_state_info.proxy_iface_tbl[i].phys_link_info[p])


/*===========================================================================
MACRO DSQMH_GET_ACL_PTR

DESCRIPTION   
  Given the iface instance, returns the ACL ptr.

DEPENDENCIES  
  None.

RETURN VALUE  
  See Description.
  
SIDE EFFECTS  
  None.
===========================================================================*/
#define DSQMH_GET_ACL_PTR(i)                                                \
        (DSQMH_IS_IFACE_INST_VALID(i)?                                      \
         &(dsqmh_state_info.proxy_iface_tbl[i].acl) : NULL)



#ifndef FEATURE_DSS_LINUX
/*===========================================================================
MACRO DSQMH_GET_SMD_INFO_PTR

DESCRIPTION   
  Given the iface instance and phys link index, returns the a pointer to
  the datapath SMD info.

DEPENDENCIES  
  None.

RETURN VALUE  
  See Description.
  
LIMITATIONS
  None.
===========================================================================*/
#define DSQMH_GET_SMD_INFO_PTR(i,p)                                      \
   ((DSQMH_MAX_PS_IFACES > i) && (DSQMH_MAX_PHYSLINKS_PER_IFACE > p))?   \
     &(dsqmh_state_info.proxy_iface_tbl[i].phys_link_info[p].smd_info) : \
     NULL
#endif


/*===========================================================================
MACRO DSQMH_GET_QMI_INFO_PTR

DESCRIPTION   
  Given the iface instance index, returns the a pointer to the QMI
  Message Library info.

DEPENDENCIES  
  None.

RETURN VALUE  
  See Description.
  
LIMITATIONS
  None.
===========================================================================*/
#define DSQMH_GET_QMI_INFO_PTR(i)                                           \
        (DSQMH_IS_IFACE_INST_VALID(i)?                                      \
         &(dsqmh_state_info.proxy_iface_tbl[i].qmi_msglib_info) : NULL)



/*===========================================================================
MACRO DSQMH_GET_POLICY_INFO_PTR

DESCRIPTION   
  Given the iface instance index, returns the a pointer to the PS ACL
  policy info.

DEPENDENCIES  
  None.

RETURN VALUE  
  See Description.
  
LIMITATIONS
  None.
===========================================================================*/
#define DSQMH_GET_POLICY_INFO_PTR(i)                                        \
        (DSQMH_IS_IFACE_INST_VALID(i)?                                      \
         &(dsqmh_state_info.proxy_iface_tbl[i].policy_info) : NULL)


#ifdef FEATURE_DATA_PS_IPV6

/*===========================================================================
MACRO DSQMH_GET_IPV6_INFO_PTR

DESCRIPTION   
  Given the iface instance index, returns the a pointer to the IPV6 info.

DEPENDENCIES  
  None.

RETURN VALUE  
  See Description.
  
LIMITATIONS
  None.
===========================================================================*/
#define DSQMH_GET_IPV6_INFO_PTR(i)                                          \
        (DSQMH_IS_IFACE_INST_VALID(i)?                                      \
         &(dsqmh_state_info.proxy_iface_tbl[i].ipv6_info) : NULL)


/*===========================================================================
MACRO DSQMH_GET_IPV6_SM_PTR

DESCRIPTION   
  Given the iface instance index, returns the a pointer to the IPV6 state
  machine info.

DEPENDENCIES  
  None.

RETURN VALUE  
  See Description.
  
LIMITATIONS
  None.
===========================================================================*/
#define DSQMH_GET_IPV6_SM_PTR(i)                                            \
        (DSQMH_IS_IFACE_INST_VALID(i)?                                      \
         &(dsqmh_state_info.proxy_iface_tbl[i].ipv6_info.ip6_sm) : NULL)

#endif /* FEATURE_DATA_PS_IPV6 */


/*===========================================================================
MACRO DSQMH_GET_RX_SIGNAL

DESCRIPTION   
  Return the RX_SIGNAL associated with QMH instance. There is a one-to-one
  mapping between QMH instance and corresponding RX_SIGNAL. 
  Currently 5 QMH instances are supported at the max, if this number 
  is increased, we would have to correspondingly increase the RX signals 
  as well (in ps_svc.h)

   Inst   RX_SIGNAL
   0      PS_RX_PROXY1_SIGNAL 
   1      PS_RX_PROXY2_SIGNAL
   2      PS_RX_PROXY3_SIGNAL 
   3      PS_RX_PROXY4_SIGNAL
   4      PS_RX_PROXY5_SIGNAL

DEPENDENCIES
  None.

RETURN VALUE
  See Description.

LIMITATIONS
  None.
===========================================================================*/
#ifndef FEATURE_DSS_LINUX
#define DSQMH_GET_RX_SIGNAL dsqmh_get_rx_signal
INLINE ps_sig_enum_type dsqmh_get_rx_signal
(
  uint32 iface_inst 
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  switch (iface_inst)
  {
    case 0:
      return PS_RX_PROXY1_SIGNAL; 
      
    case 1:
      return PS_RX_PROXY2_SIGNAL;

    case 2:
      return PS_RX_PROXY3_SIGNAL;

    case 3:
      return PS_RX_PROXY4_SIGNAL;

    case 4:
      return PS_RX_PROXY5_SIGNAL;

    default:    
      DSQMH_ASSERT( NULL, "Invalid iface inst specified" );
      return PS_MAX_SIGNALS;
  }
  
} /* dsqmh_get_rx_signal() */

/*===========================================================================
MACRO DSQMH_GET_LLE_INSTANCE

DESCRIPTION
  Return the LLE instance associated with QMH instance. It is a 
  one-to-one mapping between QMH instance and LLE instance.
  
  Inst    LLE instance
   0      LAN_LLE_RMNET1 
   1      LAN_LLE_RMNET2
   2      LAN_LLE_RMNET3 
   3      LAN_LLE_RMNET4
   4      LAN_LLE_RMNET5

DEPENDENCIES  
  None.

RETURN VALUE  
  See Description.
  
LIMITATIONS
  None.
===========================================================================*/

#define DSQMH_GET_LLE_INSTANCE dsqmh_get_lle_instance

INLINE lan_lle_enum_type dsqmh_get_lle_instance
(
  uint32 iface_inst 
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  switch (iface_inst)
  {
    case 0:
      return LAN_LLE_RMNET1; 
      
    case 1:
      return LAN_LLE_RMNET2;

    case 2:
      return LAN_LLE_RMNET3;

    case 3:
      return LAN_LLE_RMNET4;

    case 4:
      return LAN_LLE_RMNET5;

    default:    
      DSQMH_ASSERT( NULL, "Invalid iface inst specified" );
      return LAN_LLE_MIN;
  }
  
} /* dsqmh_get_lle_instance() */
#endif

/*===========================================================================
MACRO DSQMH_GET_DPL_IID

DESCRIPTION
  Return the DPL IID based on the QMH iface instance. There is a 
  one-to-one mapping between QMH instance and DPL IID.
  
  Inst    DPL IID
   0      DPL_IID_IFNAME_PROXY_IFACE_0 
   1      DPL_IID_IFNAME_PROXY_IFACE_1 
   2      DPL_IID_IFNAME_PROXY_IFACE_2 
   3      DPL_IID_IFNAME_PROXY_IFACE_3 
   4      DPL_IID_IFNAME_PROXY_IFACE_4 

DEPENDENCIES
  None.

RETURN VALUE
  Returns the DPL IID.

LIMITATIONS
  None.
===========================================================================*/
#define DSQMH_GET_DPL_IID dsqmh_get_dpl_iid
INLINE dpl_iid_ifname_enum_type dsqmh_get_dpl_iid
(
  uint32 iface_inst 
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  switch (iface_inst)
  {
    case 0:
      return DPL_IID_IFNAME_PROXY_IFACE_0; 
      
    case 1:
      return DPL_IID_IFNAME_PROXY_IFACE_1;

    case 2:
      return DPL_IID_IFNAME_PROXY_IFACE_2;

    case 3:
      return DPL_IID_IFNAME_PROXY_IFACE_3;

    case 4:
      return DPL_IID_IFNAME_PROXY_IFACE_4;

    default:    
      DSQMH_ASSERT( NULL, "Invalid iface inst specified" );
      return DPL_IID_IFNAME_MAX;
  }
  
} /* dsqmh_get_dpl_iid() */



/*===========================================================================
MACRO DSQMH_GET_QOSHDR_ENABLED

DESCRIPTION   
  Given the iface instance and phys link index, returns the QMI QOS header
  enabled flag.

DEPENDENCIES  
  None.

RETURN VALUE  
  See Description.
  
LIMITATIONS
  None.
===========================================================================*/
#define DSQMH_GET_QOSHDR_ENABLED(i)                                         \
        (DSQMH_IS_IFACE_INST_VALID(i)?                                      \
         dsqmh_state_info.proxy_iface_tbl[i].qmi_msglib_info.               \
            qoshdr_enabled : FALSE)


/*===========================================================================
MACRO DSQMH_GET_PROFILE_NUMBER

DESCRIPTION   
  Given the call instance, returns the a profile number of the primary
  context

DEPENDENCIES  
  None.

RETURN VALUE  
  See Descritpion.
  
SIDE EFFECTS  
  None.
===========================================================================*/
#define DSQMH_GET_PROFILE_NUMBER(i)                                         \
        (DSQMH_IS_IFACE_INST_VALID(i)?                                      \
         dsqmh_state_info.proxy_iface_tbl[i].prof_number : 0)



/*===========================================================================
MACRO DSUMTSPS_GET_PSIFACE_INSTANCE

DESCRIPTION   
  Given the ps iface_ptr, extracts out the ps iface instance from table.

DEPENDENCIES  

RETURN VALUE  
  Returns the ps iface instance, which is the index into the 
  dsqmh_state_info.proxy_iface_tbl

SIDE EFFECTS  
  None.
===========================================================================*/
#define DSQMH_GET_PSIFACE_INST_FROM_PTR(x)\
        (uint32)(((ps_iface_type *)x)->client_data_ptr)



/*===========================================================================
MACRO DSQMH_ENCODE_IFACE_PHYSLINK_ID
      DSQMH_DECODE_IFACE_PHYSLINK_ID

DESCRIPTION   
  Encode/decode the iface instance and phys link index into a single value.

DEPENDENCIES
  Code parameter must be uint32 (void*) 

RETURN VALUE  
  Encode macro returns iface+physlink instance code

SIDE EFFECTS  
  None.
===========================================================================*/
#define DSQMH_ENCODE_IFACE_PHYSLINK_ID(i,p)\
         ((i<<16) | (p))
#define DSQMH_DECODE_IFACE_PHYSLINK_ID(i,p,code)\
         i = ((uint32)code & 0xFFFF0000)>>16; \
         p = ((uint32)code & 0x0000FFFF); 



/*===========================================================================
MACRO DSQMH_SET_TECH_PREF

DESCRIPTION   
  Macro to set the bearer technology preference.

DEPENDENCIES  
  None.

RETURN VALUE  
  None.
  
SIDE EFFECTS  
  None.
===========================================================================*/
#define QMI_WDS_TECH_TYPE_WWAN \
        (QMI_WDS_TECH_PREF_UMTS | QMI_WDS_TECH_PREF_CDMA)

#define DSQMH_SET_TECH_PREF(iface_type,pref,xpref)                    \
  if( DSS_IFACE_NAME == iface_type.kind )                             \
  {                                                                   \
    switch( iface_type.info.name )                                    \
    {                                                                 \
      case DSS_IFACE_CDMA_SN:                                         \
      case DSS_IFACE_CDMA_AN:                                         \
      case DSS_IFACE_3GPP2_ANY:                                       \
        xpref = (qmi_wds_technology_type)iface_type.info.name;        \
        pref = QMI_WDS_TECH_PREF_CDMA;                                \
        break;                                                        \
      case DSS_IFACE_UMTS:                                            \
      case DSS_IFACE_3GPP_ANY:                                        \
        xpref = (qmi_wds_technology_type)iface_type.info.name;        \
        pref = QMI_WDS_TECH_PREF_UMTS;                                \
        break;                                                        \
      case DSS_IFACE_MBMS:                                            \
        xpref = (qmi_wds_technology_type)iface_type.info.name;        \
        pref = QMI_WDS_TECH_PREF_MBMS;                                \
        break;                                                        \
      case DSS_IFACE_DVBH:                                            \
        xpref = (qmi_wds_technology_type)iface_type.info.name;        \
        pref = QMI_WDS_TECH_PREF_DVBH;                                \
        break;                                                        \
      case DSS_IFACE_CDMA_BCAST:                                      \
        xpref = (qmi_wds_technology_type)iface_type.info.name;        \
        pref = QMI_WDS_TECH_PREF_BCMCS;                               \
        break;                                                        \
      case DSS_IFACE_FLO:                                             \
        xpref = (qmi_wds_technology_type)iface_type.info.name;        \
        pref = QMI_WDS_TECH_TYPE_WWAN;                                \
        break;                                                        \
      case DSS_IFACE_SLIP:                                            \
        xpref = (qmi_wds_technology_type)iface_type.info.name;        \
        pref = QMI_WDS_TECH_PREF_SLIP;                                \
        break;                                                        \
      case DSS_IFACE_WWAN:                                            \
      case DSS_IFACE_ANY_DEFAULT:                                     \
      case DSS_IFACE_ANY:                                             \
        xpref = (qmi_wds_technology_type)iface_type.info.name;        \
        pref = QMI_WDS_TECH_TYPE_WWAN;                                \
        break;                                                        \
      default:                                                        \
        xpref = (qmi_wds_technology_type)DSQMH_INVALID_TECHPREF;      \
    }                                                                 \
  }                                                                   \
  else                                                                \
  {                                                                   \
    xpref = (qmi_wds_technology_type)DSS_IFACE_ANY;                   \
    pref = QMI_WDS_TECH_TYPE_WWAN;                                    \
  }



/*===========================================================================
MACRO DSQMH_SET_PROFILE_INDEX

DESCRIPTION   
  Macro to set the bearer technology preference.

DEPENDENCIES  
  None.

RETURN VALUE  
  None.
  
SIDE EFFECTS  
  None.
===========================================================================*/
#define DSQMH_SET_PROFILE_INDEX(policy,umts,cdma,pref,mask)              \
  umts = cdma = 0;                                                       \
  if( (QMI_WDS_TECH_PREF_CDMA & pref) &&                                 \
      policy->data_session_profile_id ) {                                \
    cdma =  (unsigned char)policy->data_session_profile_id;              \
    mask |= QMI_WDS_START_NW_PROFILE_IDX_3GPP2_PARAM; }                  \
  if( (QMI_WDS_TECH_PREF_UMTS & pref) &&                                 \
      (int32)policy->pdp_info ) {                                        \
    umts =  (unsigned char)((uint32)policy->pdp_info);                   \
    mask |= QMI_WDS_START_NW_PROFILE_IDX_PARAM; }



/*===========================================================================
MACRO DSQMH_HTON_IPV6_ADDR
      DSQMH_NTOH_IPV6_ADDR

DESCRIPTION   
  Macros to convernt Pv6 address to/from network format

DEPENDENCIES  
  None.

RETURN VALUE  
  None.
  
SIDE EFFECTS  
  None.
===========================================================================*/
#define DSQMH_HTON_IPV6_ADDR(src,dest) \
      (dest)->ps_s6_addr32[0] = ps_htonl((src)->ps_s6_addr32[0]);      \
      (dest)->ps_s6_addr32[1] = ps_htonl((src)->ps_s6_addr32[1]);      \
      (dest)->ps_s6_addr32[2] = ps_htonl((src)->ps_s6_addr32[2]);      \
      (dest)->ps_s6_addr32[3] = ps_htonl((src)->ps_s6_addr32[3]);      

#define DSQMH_NTOH_IPV6_ADDR(src,dest) \
      (dest)->ps_s6_addr32[0] = ps_ntohl((src)->ps_s6_addr32[0]);      \
      (dest)->ps_s6_addr32[1] = ps_ntohl((src)->ps_s6_addr32[1]);      \
      (dest)->ps_s6_addr32[2] = ps_ntohl((src)->ps_s6_addr32[2]);      \
      (dest)->ps_s6_addr32[3] = ps_ntohl((src)->ps_s6_addr32[3]);      



/*===========================================================================
MACRO DSQMH_SET_PS_ERRNO

DESCRIPTION   
  Macro to set PS layer error based on QMI error structure

DEPENDENCIES  
  None.

RETURN VALUE  
  None.
  
SIDE EFFECTS  
  None.
===========================================================================*/
#define DSQMH_SET_PS_ERRNO(qmi,ps)                                        \
     ps = (sint15)((QMI_WDS_EXTENDED_ERROR_INFO_PARAM & qmi.param_mask)?  \
                   qmi.dss_errno : 0);

/*===========================================================================
                      FUNCTION DECLARATIONS

===========================================================================*/

/* Utility function prototypes */
void dsqmhsm_error_hook(stm_status_t error, const char *filename,uint32 line, struct stm_state_machine_s *sm);
void dsqmhsm_debug_hook(stm_debug_event_t debug_event, struct stm_state_machine_s *sm, stm_state_t state_info, void *payload);

/*===========================================================================
FUNCTION DSQMHLLIF_DECODE_CALL_END

DESCRIPTION
  Map the QMI call end reason to the PS network down reason.

PARAMETERS
  qmi_code    - QMI call end reson
  ps_code_ptr - PS library netrok down code (OUT)

RETURN VALUE
  None

DEPENDENCIES
  None.

SIDE EFFECTS
  None
===========================================================================*/
void dsqmhllif_decode_call_end
(
  const qmi_wds_call_end_reason_type   qmi_code,
  ps_iface_net_down_reason_type *ps_code_ptr
);



/*---------------------------------------------------------------------------
  Configuration for Linux environment, single-modem
---------------------------------------------------------------------------*/
/* Number of QMH ifaces supported */
#define DSQMH_MODEM_CNT                        (1)

/* QMI platform constants definitions */
#define QMI_PLATFORM_MAX_PDP_CONNECTIONS       (8)
#define QMI_PLATFORM_MAX_CONNECTIONS           (8)
#define QMI_PLATFORM_MAX_CONNECTION_NON_BCAST  (8)

/* QMH default supported call_type definition */
#define DS_QMH_DEFAULT_SUPPORTED_CALL_TYPE   (DS_QMH_CALL_TYPE_LOCAL)

#endif /* FEATURE_DATA_PS && FEATURE_QMI_CLIENT */

#endif /* DSQMHI_H */
