/*===========================================================================


                       Q M I   M O D E   H A N D L E R

                   L O W E R   L A Y E R   I N T E R F A C E

GENERAL DESCRIPTION
  This file contains data declarations and function prototypes for the
  QMI Proxy IFACE lower layer interface.

EXTERNALIZED FUNCTIONS

INITIALIZATION AND SEQUENCING REQUIREMENTS

Copyright (c) 2008 -  2012 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

  $Header: //source/qcom/qct/modem/datahlos/interface/qmiifacectls/rel/11.01.01/src/ds_qmh_llif.c#8 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
06/22/11    bd     Fix for DoS  bundled packet when starting the call without 
                   Traffic Channel setup. Migrating CL 1674460
06/21/11    sm     Added support to retreive packet data statistics
04/13/11    hm     Multi-modem support merged from linux QMH
10/19/10    sy     Replaced DCC cmd bufs with client allocated memory.
10/04/10    sy     Added p-cscf ipv6 addr list support.
10/04/10    sy     Added dsqmhllif_qos_modify_load_qmi_spec.
10/04/10    sy     Added support for QOS_NW_SUPPORTED_PROFILE_CHANGE registration.
10/04/10    sy     Added support for data capabilities registration. Added
                   caching of qos_supported result in query qos settings.
                   Add Support for IPv6 QoS.
08/16/10    sy     Set NV_IPV6_SM_CONFIG_I to default if nv values read is 0
07/02/10    hm     Add support for Fusion MDM9K target.
05/24/10    hm     Support QOS_CONFIGURE opcode with QOS_REQUEST_EX
10/15/09    ar     Ensure PS filter conversion assigns error_mask.
06/24/09    am     Renamed hton/ntoh macros with ps_ prefix.
06/17/09    ar     Partition primary modify from secondary processing.
05/28/09    ar     Add support for QMI_WDS_EVENT_DORM_STATUS_IND
02/19/09    am     DS Task De-coupling effort and introduction of DCC task.
02/03/09    ar     Do not send local IFACE ID routing query to Modem
01/21/09    ar     Routing code error enhancements.
01/16/09    ar     Fix Lint and MOB integration issues.
01/09/09    ar     Added IPV6 address support.
12/16/08    ar     Fix QOS filter TOS parameter translation
11/24/08    ar     Adjust for extended tech pref convention change
09/24/08    sv     updated route lookup flag to indicate interface lookup or
                   datapath.
05/06/08    ar     Created module/initial version.

===========================================================================*/


/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"

#if defined (FEATURE_DATA_PS) && defined (FEATURE_QMI_CLIENT)

#define FEATURE_DSM_WM_CB (1)

#include "AEEstd.h"
#include "dsm.h"
#ifndef FEATURE_DSS_LINUX
#include "sio.h"
#include "smd.h"
#endif
#include "dcc_task_defs.h"
#include "dssocket_defs.h"
#include "ps_flowi.h"
#include "ps_iface_ipfltr.h"
#include "ps_utils.h"
#include "ps_crit_sect.h"
#include "ps_system_heap.h"
#include "ds_qmhi.h"
#include "ds_qmh_llif.h"
#include "ds_qmh_netplat.h"
#include "ds_qmh_config.h"
#include "ds_flow_control.h"
#include "ds_Utils_DebugMsg.h"

#ifdef FEATURE_DATA_PS_IPV6
#include "ps_iface_addr_v6.h"
#endif

#ifdef FEATURE_DSS_LINUX
#include <pthread.h>
#endif

/*===========================================================================

            LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE

This section contains local definitions for constants, macros, types,
variables and other items needed by this module.

===========================================================================*/

/* Critical section for SMD port resources */
ps_crit_sect_type dsqmhllif_smd_crit_section;

/* QMIMSGLIB handle */
static int qmimsglib_handle;

#define DSQMHLLIF_ENTER_SMD_CRIT_SECTION()\
        PS_ENTER_CRIT_SECTION( &dsqmhllif_smd_crit_section );
#define DSQMHLLIF_EXIT_SMD_CRIT_SECTION()\
        PS_LEAVE_CRIT_SECTION( &dsqmhllif_smd_crit_section );


#define DSQMHLLIF_VERIFY_INIT_COMPLETED()                              \
      if( !dsqmh_state_info.conn_init )                                \
      {                                                                \
        DSQMH_MSG_FATAL("QMI Mode Handler not completed init!",0,0,0); \
        return DSQMH_FAILED;                                           \
      }

/* Macro to conditionally set QOS spec parameters. */
#define DSQMH_SET_QMI_QOS_PARAM(qparam, qmask, qfield, \
                                fparam, fmask, ffield) \
      if( ffield & (uint32)fmask )                     \
      {                                                \
        qparam = fparam;                               \
        qfield |= (uint32)qmask;                       \
      }

/* Macro to conditionally set QOS v6 address. */
#define DSQMH_SET_QMI_QOS_V6_ADDR_PARAM(qparam, qmask, qfield,           \
                                        fparam, fmask, ffield)           \
      if( ffield & (uint32)fmask )                                       \
      {                                                                  \
        memcpy(qparam, fparam, sizeof(qmi_qos_ipv6_addr_type));          \
        qfield |= (uint32)qmask;                                         \
      }

/* Macros to set the IP address family preference. */
#define DSQMH_SET_IPADDR_PREF(family,fvalue,pref,pvalue,mask,mvalue)       \
  if( fvalue == family ) { pref = pvalue; mask |= mvalue; }

#define DSQMH_SET_IPADDR_FAMILY_PREF(family,pref)                                              \
  DSQMH_SET_IPADDR_PREF( family, IFACE_IPV4_ADDR_FAMILY,                                       \
                         pref, QMI_WDS_IP_FAMILY_PREF_IPV4,                                    \
                         req_params.params_mask, QMI_WDS_ROUTE_LOOK_UP_IP_FAMILY_PREF_PARAM ); \
  DSQMH_SET_IPADDR_PREF( family, IFACE_IPV6_ADDR_FAMILY,                                       \
                         pref, QMI_WDS_IP_FAMILY_PREF_IPV6,                                    \
                         req_params.params_mask, QMI_WDS_ROUTE_LOOK_UP_IP_FAMILY_PREF_PARAM ); \
  DSQMH_SET_IPADDR_PREF( family, IFACE_UNSPEC_ADDR_FAMILY,                                     \
                         pref, QMI_WDS_IP_FAMILY_PREF_UNSPECIFIED,                             \
                         req_params.params_mask, QMI_WDS_ROUTE_LOOK_UP_IP_FAMILY_PREF_PARAM );

/*---------------------------------------------------------------------------
  Macro to convert the QMIMSGLIB IP family to Iface IP family format 
  (Currently both use the same numeric values, just cast to correct type )
---------------------------------------------------------------------------*/
#define DSQMH_QMI_IP_FAMILY_TO_PS_IFACE_IP_FAMILY(qmi_ip_family)            \
  ((ps_iface_addr_family_type) qmi_ip_family)


/*---------------------------------------------------------------------------
  QOS paramaters and structures
---------------------------------------------------------------------------*/
#define DSQMH_MAX_QOS_SPECS                DSQMH_MAX_QOS_FLOWS
#define DSQMH_MAX_QOS_FILTERS              (8)
#define DSQMH_MAX_QOS_REQ_FLOWS            (1)
#define DSQMH_MAX_QOS_MIN_FLOWS            (1)
#define DSQMH_MAX_QOS_AUX_FLOWS            (8)
#define DSQMH_MAX_QOS_TOT_FLOWS            ( DSQMH_MAX_QOS_REQ_FLOWS + \
                                             DSQMH_MAX_QOS_MIN_FLOWS + \
                                             DSQMH_MAX_QOS_AUX_FLOWS )

/* Count number of flows in QOS specification */
#define DSQMH_FLOW_COUNT(dir,mask,aux_cnt)                              \
     ( (((uint32)QOS_MASK_##dir##_FLOW     & ps_spec_ptr->field_mask)? 1 : 0) + \
       (((uint32)QOS_MASK_##dir##_MIN_FLOW & ps_spec_ptr->field_mask)? 1 : 0) + \
       aux_cnt )

/* Structure used for QMI Msg Lib QOS request parameters */
typedef struct dsqmh_qmi_qos_spec_s
{
  qmi_qos_flow_req_type   tx_flow_reqs[DSQMH_MAX_QOS_TOT_FLOWS];
  qmi_qos_filter_req_type   tx_fltr_reqs[DSQMH_MAX_QOS_FILTERS];
  qmi_qos_flow_req_type   rx_flow_reqs[DSQMH_MAX_QOS_TOT_FLOWS];
  qmi_qos_filter_req_type   rx_fltr_reqs[DSQMH_MAX_QOS_FILTERS];
} dsqmh_qmi_qos_spec_s_type;


/*---------------------------------------------------------------------------
  PROC_ID definitions. 

  This parameter is passed as part of routing in acl_pi_ptr->proc_id. These
  values are currently not available as modem API, hence defining them 
  locally here. 
---------------------------------------------------------------------------*/
#define QMH_PROC_ID_LOCAL                     (0x00UL)
#define QMH_PROC_ID_RMNET_EMBEDDED            (0x02UL)
#define QMH_PROC_ID_RMNET_TETHERED            (0x03UL)



/*===========================================================================

                    INTERNAL FUNCTION DEFINITIONS

===========================================================================*/
/*===========================================================================
FUNCTION dsqmhllif_qmi_sys_event_handler

DESCRIPTION
  This method is a callback registered for handling sys events. The only
  events that QMH currently cares for is SYNC indication message.

PARAMETERS
  None.

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.
===========================================================================*/
LOCAL void dsqmhllif_qmi_sys_event_handler
(
  qmi_sys_event_type                event_id,
  const qmi_sys_event_info_type    *event_info,
  void                             *user_data
)
{
  dsqmh_msg_buf_type        *msg_ptr = NULL;  /* Pointer to message buffer */
  uint32                     iface_inst;
  ps_iface_type             *iface_ptr = NULL;
  ps_iface_state_enum_type   iface_state;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-----------------------------------------------------------------------
    Only sys event we are interested right now in is SYNC indication event.
    Ignore all other messages.
  -----------------------------------------------------------------------*/
  (void) user_data;
  ASSERT (NULL != event_info);

  DSQMH_MSG_MED( "dsqmhllif_qmi_sys_event_handler",0, 0, 0);

  if (QMI_SYS_EVENT_SYNC_IND == event_id)
  {

    iface_inst =
      DSQMH_GET_IFACE_INST_FROM_CONN_ID( event_info->qmi_sync_ind.conn_id );

    if(!(DSQMH_IS_IFACE_INST_VALID(iface_inst)))
    {
      DSQMH_MSG_MED( "iface instance not valid %d",iface_inst, 0, 0);
      return;
    }

    DSQMH_MSG_MED( "Recvd SYNC IND System event %d", event_id, 0, 0);

    if (FALSE == dsqmh_state_info.self_init)
    {
      DSQMH_MSG_HIGH ("QMH self init not done, ignore SYNC IND", 0, 0, 0);
      return;
    }

    if (TRUE == dsqmh_state_info.conn_init)
    {
      DSQMH_MSG_HIGH ("QMH connection inits already done, ignore SYNC IND",
                       0, 0, 0);
      return;
    }

    /*---------------------------------------------------------------------
      Regardless of which port SYNC_IND is received on, we need to ensure
      we move all QMH state machines forward. 
    ---------------------------------------------------------------------*/
    for (iface_inst = 0; iface_inst < DSQMH_MAX_PS_IFACES; iface_inst++)
    {
      iface_ptr = DSQMH_GET_IFACE_PTR(iface_inst);
      if (!PS_IFACE_IS_VALID(iface_ptr))
      {
        DSQMH_MSG_ERROR ("Invalid QMH iface, ignore SYNC IND", 0, 0, 0);
        return;
      }

      iface_state = ps_iface_state(iface_ptr);
      if (IFACE_DISABLED != iface_state)
      {
        DSQMH_MSG_HIGH ("Iface state %d, ignore SYNC IND", iface_state, 0, 0);
        return;
      }

      /*---------------------------------------------------------------------
        Post message to host task to complete initialization of the QMI
        mode handler.
      ---------------------------------------------------------------------*/
      DSQMH_GET_MSG_BUF( msg_ptr );
      if( NULL != msg_ptr )
      {
        memset((void*)msg_ptr, 0x0, sizeof(dsqmh_msg_buf_type));
        msg_ptr->msg_id     = PROXY_IFACE_MODEM_INIT_IND;
        msg_ptr->iface_inst = iface_inst;

        DSQMH_MSG_MED( "Posting DS cmd: PROXY_IFACE_MODEM_INIT_IND",0,0,0);
        DSQMH_PUT_MSG_BUF( msg_ptr );
      }
      else
      {
        DSQMH_MSG_FATAL( "Failed to allocate msg buffer, dropping msg",0,0,0 );
      }

    } /* for all QMH instances */

  }

  return;

} /* dsqmhllif_qmi_sys_event_handler() */

#ifndef FEATURE_DSS_LINUX

/*===========================================================================
FUNCTION DSQMHLLIF_SMD_TX_LOWATER_CB

DESCRIPTION
  Transmit watermark low-water callback.
  Reenables tx flow on the data link specified by user_data parameter.

PARAMETERS
  wm_ptr        -  DL watermark on which RX packets are enqueued
  user_data_ptr -  Handle with encoded iface and phys link identifiers

RETURN VALUE
  None

DEPENDENCIES
  dsqmhllif_init_phys_link_queues() must have been called previously

SIDE EFFECTS
  None
===========================================================================*/
LOCAL void  dsqmhllif_smd_tx_lowater_cb
(
  struct dsm_watermark_type_s *wm_ptr,
  void                        *user_data_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  dsqmhllif_set_tx_flow( user_data_ptr, DS_FLOW_PROXY_MASK, FALSE );
} /* dsqmhllif_smd_tx_lowater_cb() */



/*===========================================================================
FUNCTION DSQMHLLIF_SMD_TX_HIWATER_CB

DESCRIPTION
  Transmit watermark high-water callback.
  Disables tx flow on the data link specified by user_data parameter.

PARAMETERS
  wm_ptr        -  DL watermark on which RX packets are enqueued
  user_data_ptr -  Handle with encoded iface and phys link identifiers

RETURN VALUE
  None

DEPENDENCIES
  dsqmhllif_init_phys_link_queues() must have been called previously

SIDE EFFECTS
  None
===========================================================================*/
LOCAL void  dsqmhllif_smd_tx_hiwater_cb
(
  struct dsm_watermark_type_s *wm_ptr,
  void                        *user_data_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  dsqmhllif_set_tx_flow( user_data_ptr, DS_FLOW_PROXY_MASK, TRUE );
} /* dsqmhllif_smd_tx_hiwater_cb() */



/*===========================================================================
FUNCTION DSQMHLLIF_SMD_RX_LOWATER_CB

DESCRIPTION
  Receive watermark low-water callback.
  Reenables rx flow on the data link specified by user_data parameter.

PARAMETERS
  wm_ptr        -  DL watermark on which RX packets are enqueued
  user_data_ptr -  Handle with encoded iface and phys link identifiers

RETURN VALUE
  None

DEPENDENCIES
  dsqmhllif_init_phys_link_queues() must have been called previously

SIDE EFFECTS
  None
===========================================================================*/
LOCAL void  dsqmhllif_smd_rx_lowater_cb
(
  struct dsm_watermark_type_s *wm_ptr,
  void                        *user_data_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  dsqmhllif_set_rx_flow( user_data_ptr, DS_FLOW_PROXY_MASK, FALSE );
} /* dsqmhllif_smd_rx_lowater_cb() */



/*===========================================================================
FUNCTION DSQMHLLIF_SMD_RX_HIWATER_CB

DESCRIPTION
  Receive watermark high-water callback.
  Disables rx flow on the data link specified by user_data parameter.

PARAMETERS
  wm_ptr        -  DL watermark on which RX packets are enqueued
  user_data_ptr -  Handle with encoded iface and phys link identifiers

RETURN VALUE
  None

DEPENDENCIES
  dsqmhllif_init_phys_link_queues() must have been called previously

SIDE EFFECTS
  None
===========================================================================*/
LOCAL void  dsqmhllif_smd_rx_hiwater_cb
(
  struct dsm_watermark_type_s *wm_ptr,
  void                        *user_data_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  dsqmhllif_set_rx_flow( user_data_ptr, DS_FLOW_PROXY_MASK, TRUE );
} /* dsqmhllif_smd_rx_hiwater_cb() */



/*===========================================================================
FUNCTION DSQMHLLIF_SMD_RX_PKT_CB

DESCRIPTION
  SMD delivers new RX packet for IP stack.  Set host task signal.

PARAMETERS
  None.

RETURN VALUE
  None

DEPENDENCIES
  None.

SIDE EFFECTS
  None
===========================================================================*/
LOCAL void dsqmhllif_smd_rx_pkt_cb
(
  struct dsm_watermark_type_s *wm_ptr,
  void                        *user_data_ptr
)
{
  uint32                 iface_inst;            /* Index for iface table   */
  uint32                 phys_link;             /* Index for phys link list*/

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( wm_ptr, "DSQMH LLIF null watermark pointer passed" );

  DSQMH_DECODE_IFACE_PHYSLINK_ID( iface_inst, phys_link,
                                  ((uint32)user_data_ptr) );
  if(! ((DSQMH_MAX_PS_IFACES > iface_inst) &&
        (DSQMH_MAX_PHYSLINKS_PER_IFACE > phys_link)) )
  {
    DSQMH_MSG_ERROR("QMH LLIF Error decoding iface,physlink: 0x%x 0x%x 0x%x",
                    user_data_ptr, iface_inst, phys_link );
    return;
  }

  DSQMH_MSG_LOW( "QMH LLIF smd rx pkt cb: iface=%d physlink=%d",
                 iface_inst, phys_link, 0 );

  /* Signal host task (extended signal). */
  PS_SET_EXT1_SIGNAL( DSQMH_GET_RX_SIGNAL( iface_inst ) );

} /* dsqmhllif_smd_rx_pkt_cb() */




/*===========================================================================
FUNCTION DSQMHLLIF_SMD_CLOSE_DONE_CB

DESCRIPTION
  SMD port closed callback.

PARAMETERS
  None.

RETURN VALUE
  None

DEPENDENCIES
  None.

SIDE EFFECTS
  None
===========================================================================*/
LOCAL void  dsqmhllif_smd_close_done_cb( void )
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  DSQMH_MSG_LOW( "QMH LLIF close smd port completed", 0, 0, 0 );

  /* Do we need to do something here? */

} /* dsqmhllif_smd_rx_lowater_cb() */
#endif /* FEATURE_DSS_LINUX */



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
)
{
  DSQMH_ASSERT( ps_code_ptr, "DSQMH LLIF ps_iface_net_down_reason_type pointer passed" );
  *ps_code_ptr = TYPE_REASON_VAL(qmi_code.call_end_reason_verbose.verbose_reason_type,
                                 qmi_code.call_end_reason_verbose.verbose_reason);
  return;
} /* dsqmhllif_decode_call_end() */


/*===========================================================================
FUNCTION DSQMHLLIF_QMI_WDS_CMD_CB

DESCRIPTION
  QMI WDS service command callback.  Reports the status of
  asynchronous command processing.

PARAMETERS
  user_handle  - QMI Msg Lib client ID
  service_id   - QMI service ID
  sys_err_code - QMI Msg Lib error
  qmi_err_code - QMI error
  user_data    - Pointer to callback context
  rsp_id       - QMI Msg Lib txn ID
  rsp_data     - Pointer to QMI Msg Lib txn data

RETURN VALUE
  None

DEPENDENCIES
  None.

SIDE EFFECTS
  None
===========================================================================*/
/* ARGSUSED */
LOCAL void dsqmhllif_qmi_wds_cmd_cb
(
  int                           user_handle,     /* QMI Msg Lib client ID  */
  qmi_service_id_type           service_id,      /* QMI service ID         */
  int                           sys_err_code,    /* QMI Msg Lib error      */
  int                           qmi_err_code,    /* QMI error              */
  void                         *user_data,       /* Callback context       */
  qmi_wds_async_rsp_id_type     rsp_id,          /* QMI Msg Lib txn ID     */
  qmi_wds_async_rsp_data_type  *rsp_data         /* QMI Msg Lib txn data   */
)
{
  uint32  iface_inst = (uint32)user_data;       /* Index for iface table   */
  ps_iface_type           *iface_ptr = NULL;
  dsqmh_msglib_info_type  *qmi_ptr = NULL;
  dsqmh_msg_buf_type      *msg_ptr = NULL;    /* Pointer to message buffer */
  dsqmh_iface_cblk_type   *cblk_ptr = NULL;
  boolean  send_msg = FALSE;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  (void) service_id;
  DSQMH_ASSERT( rsp_data, "DSQMH LLIF null resp data pointer passed" );

  DSQMH_MSG_LOW( "QMH LLIF qmi wds cmd_cb: rsp_id=%d", rsp_id, 0, 0 );

  iface_ptr = DSQMH_GET_IFACE_PTR( iface_inst );
  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( iface_inst );
  cblk_ptr = DSQMH_GET_CBLK_PTR( iface_inst );

  /*-----------------------------------------------------------------------
    Verify PS IFACE.
  -----------------------------------------------------------------------*/
  if( !PS_IFACE_IS_VALID( iface_ptr ) )
  {
    DSQMH_MSG_ERROR( "QMH LLIF iface_ptr invalid:%d", iface_inst, 0, 0 );
    return;
  }

  /*-----------------------------------------------------------------------
    Verify callback is intended for this client.
  -----------------------------------------------------------------------*/
  if( user_handle != qmi_ptr->wds_handle )
  {
    DSQMH_MSG_ERROR( "QMH LLIF qmi cb invalid:%d", iface_inst, 0, 0 );
    return;
  }

  /*-----------------------------------------------------------------------
    Process outcome of QMI Message Library transaction.
  -----------------------------------------------------------------------*/
  if( ( QMI_NO_ERR == sys_err_code ) &&
      ( DSQMH_SUCCESS == qmi_err_code ) )
  {
    /* Successful command processing reported.  Operation outcome will
     * be posted via later asynchronous notification. */
    DSQMH_MSG_MED( "QMH LLIF qmi txn success: rsp=%d", rsp_id, 0,0 );

    /* For ABORT callback, need to post MODEM_DOWN indication to
     * cleanup upper layers. */
    send_msg = ( QMI_WDS_SRVC_ABORT_ASYNC_RSP_MSG == rsp_id )? TRUE : FALSE;
  }
  else
  {
    /* Error condition reported */
    DSQMH_MSG_ERROR( "QMH LLIF qmi txn err: sys=%d qmi=%d rsp=%d",
                     sys_err_code, qmi_err_code, rsp_id );
    send_msg = TRUE;

    /* Set the down reason from the QMI call end reason */
    dsqmhllif_decode_call_end( rsp_data->start_nw_rsp.call_end_reason,
                               &cblk_ptr->down_reason );
  }

  if( send_msg )
  {
    /*-----------------------------------------------------------------------
      Send MODEM_DOWN_IND to host task for state machine processing.
    -----------------------------------------------------------------------*/
    DSQMH_GET_MSG_BUF( msg_ptr );
    if( NULL != msg_ptr )
    {
      memset((void*)msg_ptr, 0x0, sizeof(dsqmh_msg_buf_type));

      msg_ptr->msg_id = PROXY_IFACE_MODEM_DOWN_IND;
      msg_ptr->iface_inst = iface_inst;

      DSQMH_MSG_MED( "Posting DS cmd: PROXY_IFACE_MODEM_DOWN_IND",0,0,0);
      DSQMH_PUT_MSG_BUF( msg_ptr );
    }
    else
    {
      DSQMH_MSG_FATAL( "Failed to allocate msg buffer, dropping msg",0,0,0 );
    }

    /* Clear pending transaction handle if set. */
    if( DSQMH_INVALID_TXNID != qmi_ptr->wds_txn_handle )
    {
      qmi_ptr->wds_txn_handle = DSQMH_INVALID_TXNID;
    }
  }
} /* dsqmhllif_qmi_wds_cmd_cb() */



/*===========================================================================
FUNCTION DSQMHLLIF_QMI_WDS_IND_CB

DESCRIPTION
  QMI WDS service indication callback.  Reports asynchronous modem
  notification.

PARAMETERS


RETURN VALUE
  None

DEPENDENCIES
  None.

SIDE EFFECTS
  None
===========================================================================*/
/* ARGSUSED */
LOCAL void dsqmhllif_qmi_wds_ind_cb
(
  int                            user_handle,
  qmi_service_id_type            service_id,
  void                          *user_data,
  qmi_wds_indication_id_type     ind_id,
  qmi_wds_indication_data_type  *ind_data
)
{
  dsqmh_msg_buf_type      *msg_ptr = NULL;    /* Pointer to message buffer */
  dsqmh_msglib_info_type  *qmi_ptr = NULL;
  dsqmh_iface_cblk_type   *cblk_ptr = NULL;
  boolean                  send_msg = TRUE;
  acl_policy_info_type    *policy_ptr = NULL; 

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void )user_handle;
  (void )service_id;
  DSQMH_ASSERT( ind_data, "DSQMH LLIF null ind data pointer passed" );

  DSQMH_MSG_LOW( "QMH LLIF qmi wds ind_cb: ind=%d iface=%d",
                 ind_id, (uint32)user_data, 0 );

  cblk_ptr = DSQMH_GET_CBLK_PTR( (uint32)user_data );
  policy_ptr = DSQMH_GET_POLICY_INFO_PTR ((uint32)user_data);


  /*-------------------------------------------------------------------------
   Special processing for packet status indication for dual-ip scenario.
  -------------------------------------------------------------------------*/
  if (QMI_WDS_SRVC_PKT_SRVC_STATUS_IND_MSG == ind_id)
  {

    DSQMH_MSG_MED ("Pkt srvc ind, QMH %d, pkt srvc status %d",
                    (uint32)user_data, ind_data->pkt_srvc_status.link_status, 0);

    /*-------------------------------------------------------------------------
      For Dual-IP over same port, we can have two QMH instances talking over
      same port. Both QMH instances would receive call connect/disconnect 
      messages even though it is not applicable to them. Ignore the indication 
      if it does not belong to current QMH instance.
    -------------------------------------------------------------------------*/
    /*
    if (0 != (QMI_WDS_PKT_SRVC_IND_IP_FAMILY & 
              ind_data->pkt_srvc_status.param_mask))
    {
        //Ignore indication if IP family mismatch.
      if (NULL != policy_ptr &&
           (policy_ptr->ip_family != 
            DSQMH_QMI_IP_FAMILY_TO_PS_IFACE_IP_FAMILY(
              ind_data->pkt_srvc_status.ip_family)))
      {
        DSQMH_MSG_HIGH ("Ignoring pkt srvc ind, iface fam %d, ind family %d",
                        (uint32)policy_ptr->ip_family, 
                        (uint32)ind_data->pkt_srvc_status.ip_family,
                        0); 
        return;
      }
    }
    */
  }

  /*-------------------------------------------------------------------------
   Send asynchronous message for cmd processing in host task context.
  -------------------------------------------------------------------------*/
  DSQMH_GET_MSG_BUF( msg_ptr );
  if( NULL != msg_ptr )
  {
    memset((void*)msg_ptr, 0x0, sizeof(dsqmh_msg_buf_type));

    /*-----------------------------------------------------------------------
      Populate message buffer with context info.
    -----------------------------------------------------------------------*/
    switch( ind_id )
    {
      case QMI_WDS_SRVC_INTERNAL_IFACE_EVNT_REG_MSG:
        /* Internal IFACE indication */
        msg_ptr->msg_id = PROXY_IFACE_MODEM_INTERNAL_IND;
        DSQMH_MSG_MED( "Posting DS cmd: PROXY_IFACE_MODEM_INTERNAL_IND",0,0,0);
        break;

      case QMI_WDS_SRVC_PKT_SRVC_STATUS_IND_MSG:
        /* Packet Service Status indication */
        switch( ind_data->pkt_srvc_status.link_status )
        {
          case QMI_WDS_PACKET_DATA_DISCONNECTED:
            /* Set the down reason from the QMI call end reason */
            dsqmhllif_decode_call_end( ind_data->pkt_srvc_status.call_end_reason,
                                       &cblk_ptr->down_reason );

            msg_ptr->msg_id = PROXY_IFACE_MODEM_DOWN_IND;
            DSQMH_MSG_MED( "Posting DS cmd: PROXY_IFACE_MODEM_DOWN_IND",0,0,0);
            break;

          case QMI_WDS_PACKET_DATA_CONNECTED:
            msg_ptr->msg_id = PROXY_IFACE_MODEM_UP_IND;
            DSQMH_MSG_MED( "Posting DS cmd: PROXY_IFACE_MODEM_UP_IND",0,0,0);
            break;

          default:
            DSQMH_MSG_ERROR( "QMH LLIF unsupported qmi pkt svc message",0,0,0 );
            send_msg = FALSE;
            break;
        }

        /* Clear pending transaction handle if set. */
        qmi_ptr = DSQMH_GET_QMI_INFO_PTR( (uint32)user_data );
        if( qmi_ptr &&
            DSQMH_INVALID_TXNID != qmi_ptr->wds_txn_handle )
        {
          qmi_ptr->wds_txn_handle = DSQMH_INVALID_TXNID;
        }
        break;

      case QMI_WDS_SRVC_EVENT_REPORT_IND_MSG:
      case QMI_WDS_SRVC_MT_REQUEST_IND_MSG:
      case QMI_WDS_SRVC_MCAST_STATUS_IND_MSG:
      case QMI_WDS_SRVC_MBMS_MCAST_CONTEXT_STATUS_IND_MSG:
        msg_ptr->msg_id = PROXY_IFACE_MODEM_EVENT_IND;
        DSQMH_MSG_MED( "Posting DS cmd: PROXY_IFACE_MODEM_EVENT_IND",0,0,0);
        break;

      default:
        DSQMH_MSG_ERROR( "QMH LLIF unsupported message",0,0,0 );
        send_msg = FALSE;
        break;
    }

    /*-----------------------------------------------------------------------
      Send asynchronous message to host task.
    -----------------------------------------------------------------------*/
    if( send_msg )
    {
      msg_ptr->iface_inst = (uint32)user_data;
      msg_ptr->info.qmi_wds.ind_id = ind_id;
      msg_ptr->info.qmi_wds.info = *ind_data;

      DSQMH_PUT_MSG_BUF( msg_ptr );
    }
    else
    {
      /* Release the message buffer on error condition. */
      DSQMH_RELEASE_MSG_BUF( msg_ptr );
    }
  }
  else
  {
    DSQMH_MSG_FATAL( "Failed to allocate msg buffer, dropping msg",0,0,0 );
  }

} /* dsqmhllif_qmi_wds_ind_cb() */



/*===========================================================================
FUNCTION DSQMHLLIF_QMI_QOS_IND_CB

DESCRIPTION
  QMI QOS service indication callback.  Reports asynchronous modem
  notification.

PARAMETERS


RETURN VALUE
  None

DEPENDENCIES
  None.

SIDE EFFECTS
  None
===========================================================================*/
/* ARGSUSED */
LOCAL void dsqmhllif_qmi_qos_ind_cb
(
  int                            user_handle,
  qmi_service_id_type            service_id,
  void                          *user_data,
  qmi_qos_indication_id_type     ind_id,
  qmi_qos_indication_data_type  *ind_data
)
{
  dsqmh_msg_buf_type      *msg_ptr = NULL;    /* Pointer to message buffer */
  ps_flow_type            *flow_ptr  = NULL;
  int                     index = 0;
  int                     i = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void )user_handle;
  (void )service_id;
  DSQMH_ASSERT( ind_data, "DSQMH LLIF null ind data pointer passed" );

  DSQMH_MSG_LOW( "QMH LLIF qmi qos ind_cb", 0, 0, 0 );


  /*-------------------------------------------------------------------------
    Process flow control in calling task context for optimal response time.
    -------------------------------------------------------------------------*/
  if( QMI_QOS_SRVC_EVENT_REPORT_IND_MSG == ind_id )
  {
    if ((ind_data->event_report.num_flows < 0) || 
          (ind_data->event_report.num_flows > QMI_QOS_MAX_FLOW_EVENTS))
    {
      DSQMH_MSG_ERROR( "dsqmhllif_qmi_qos_ind_cb: "
                       "Invalid number of flows passed. %d",
                       ind_data->event_report.num_flows, 0, 0 );
      return;
    }

    for (index = 0; index < ind_data->event_report.num_flows; index++)
    {
      /* Lookup the PS flow for the QMI QOS identifier */
      if( DSQMH_SUCCESS !=
          dsqmhllif_get_flow_by_qos_id( (uint32)user_data,
                                         ind_data->event_report.flow_info[index].
                                        qos_flow_state.qos_identifier,
                                        &flow_ptr ) )
      {
        DSQMH_MSG_ERROR( "DSQMH LLIF qos ind: failed flow lookup: iface=%d",
                         (uint32)user_data,0,0 );
        return;
      }

      if( NULL != flow_ptr )
      {
        switch( ind_data->event_report.flow_info[index].
                qos_flow_state.report_flow_state_chng )
        {
          case QMI_QOS_FLOW_ENABLED:
            DSQMH_MSG_MED( "DSQMH LLIF flow enable: %p",
                           (uint32)flow_ptr->client_data_ptr,0,0 );
            ps_flow_enable_tx( flow_ptr, DS_FLOW_PROXY_MASK );
            break;

          case QMI_QOS_FLOW_DISABLED:
            DSQMH_MSG_MED( "DSQMH LLIF flow disable: %p",
                           (uint32)flow_ptr->client_data_ptr,0,0 );
            ps_flow_disable_tx( flow_ptr, DS_FLOW_PROXY_MASK );
            break;

          default:
            /* No nothing, post message to host task. */
            break;
        }
      }
    }/*for(number of flows in the event report)*/
  }

  /*-------------------------------------------------------------------------
   Send asynchronous message for cmd processing in host task context.
  -------------------------------------------------------------------------*/
  DSQMH_GET_MSG_BUF( msg_ptr );
  if( NULL != msg_ptr )
  {
    memset((void*)msg_ptr, 0x0, sizeof(dsqmh_msg_buf_type));

    /*-----------------------------------------------------------------------
     Populate message buffer with context info
    -----------------------------------------------------------------------*/
    msg_ptr->msg_id = PROXY_IFACE_MODEM_QOS_IND;
    msg_ptr->iface_inst = (uint32)user_data;
    msg_ptr->info.qmi_qos.ind_id = ind_id;
    msg_ptr->info.qmi_qos.info = *ind_data;

    DSQMH_MSG_MED( "Posting DS cmd: PROXY_IFACE_MODEM_QOS_IND",0,0,0);
    DSQMH_PUT_MSG_BUF( msg_ptr );
  }
  else
  {
    DSQMH_MSG_FATAL( "Failed to allocate msg buffer, dropping msg",0,0,0 );
  }
} /* dsqmhllif_qmi_qos_ind_cb() */



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
  uint32         iface_inst,                    /* Index for iface table   */
  uint32         qos_id,                        /* QMI QOS flow identifier */
  ps_flow_type **flow_pptr                      /* Pointer to PS flow      */
)
{
  int                 result = DSQMH_FAILED;
  ps_iface_type      *iface_ptr = NULL;
  ps_flow_type       *flow_ptr  = NULL;
  void               *handle= NULL, *new_handle = NULL;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( flow_pptr, "DSQMH LLIF null flow list pointer passed" );

  iface_ptr = DSQMH_GET_IFACE_PTR( iface_inst );

  /* Check for primary flow with no flow ID */
  if( qos_id == 0)
  {
    *flow_pptr = PS_IFACE_GET_DEFAULT_FLOW( iface_ptr );;
    return DSQMH_SUCCESS;
  }

  PS_ENTER_CRIT_SECTION( &global_ps_crit_section );

  /*---------------------------------------------------------------------
    Iterate over IFACE secondary flow list.
  ---------------------------------------------------------------------*/
  handle = (ps_flow_type*)ps_iface_get_sec_flow_handle( iface_ptr );
  while( handle )
  {
    if( ps_iface_get_sec_flow_by_handle( iface_ptr, handle,
                                         &flow_ptr, &new_handle ) )
    {
      /* Check for QMI QOS ID match */
      if( qos_id == (uint32)flow_ptr->client_data_ptr )
      {
        *flow_pptr = flow_ptr;
        result = DSQMH_SUCCESS;
        break;
      }
    }
    handle = new_handle;
  }

  PS_LEAVE_CRIT_SECTION( &global_ps_crit_section );

  return result;
} /* dsqmhllif_get_flow_by_qos_id() */



/*===========================================================================
FUNCTION DSQMHLLIF_NETPLAT_CMD_CB

DESCRIPTION
  Network Platform layer command callback.  Reports results of
  asynchronous command processing.

PARAMETERS
  TBD

RETURN VALUE
  None

DEPENDENCIES
  None.

SIDE EFFECTS
  None
===========================================================================*/
LOCAL void dsqmhllif_netplat_cmd_cb
(
  ds_netplat_rsp_msg_type   rsp_id,
  ds_netplat_err_type       err_id,
  void                     *user_data
)
{
  dsqmh_msg_buf_type      *msg_ptr = NULL;    /* Pointer to message buffer */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_MED( "QMH LLIF net platform cmd cb:%d", (uint32)user_data, 0, 0 );

  /*-----------------------------------------------------------------------
    Process outcome of Network Platform Layer operation.
  -----------------------------------------------------------------------*/
  if( DS_NETPLAT_ERR_NONE == err_id )
  {
    /* Successful command processing reported.  Operation outcome will
     * be posted via later asynchronous notification. */
    DSQMH_MSG_MED( "QMH LLIF netplat op success: rsp=%d user=%d",
                   rsp_id, (uint32)user_data, 0 );
  }
  else
  {
    /* Error condition reported */
    DSQMH_MSG_ERROR( "QMH LLIF netplat op err: id=%d rsp=%d user=%d",
                     err_id, rsp_id, (uint32)user_data );

    /*-----------------------------------------------------------------------
      Send msg to host task for state machine processing.
    -----------------------------------------------------------------------*/
    DSQMH_GET_MSG_BUF( msg_ptr );
    if( NULL != msg_ptr )
    {
      boolean send_msg = TRUE;

      memset((void*)msg_ptr, 0x0, sizeof(dsqmh_msg_buf_type));

      /*---------------------------------------------------------------------
        Populate message buffer with context info
      ---------------------------------------------------------------------*/
      switch( rsp_id )
      {
      case DS_NETPLAT_RSP_BRINGUP:
      case DS_NETPLAT_RSP_TEARDOWN:
        msg_ptr->msg_id = PROXY_IFACE_PLATFORM_DOWN_IND;
        msg_ptr->iface_inst = (uint32)user_data;
        DSQMH_MSG_MED( "Posting DS cmd: PROXY_IFACE_PLATFORM_DOWN_IND",0,0,0);
        break;

      case DS_NETPLAT_RSP_QOS_REQUEST:
      case DS_NETPLAT_RSP_QOS_RELEASE:
        msg_ptr->msg_id = PROXY_IFACE_PLATFORM_QOS_IND;
        msg_ptr->info.netplat.flow_ptr = (ps_flow_type*)user_data;
        DSQMH_MSG_MED( "Posting DS cmd: PROXY_IFACE_PLATFORM_QOS_IND",0,0,0);
        break;

      default:
        DSQMH_MSG_ERROR( "QMH LLIF unsupported rsp: id=",rsp_id,0,0 );
        send_msg = FALSE;
        break;
      }

      if( send_msg )
      {
        DSQMH_PUT_MSG_BUF( msg_ptr );
      }
      else
      {
        /* Release the message buffer on error condition. */
        DSQMH_RELEASE_MSG_BUF( msg_ptr );
      }
    }
    else
    {
      DSQMH_MSG_FATAL( "Failed to allocate msg buffer, dropping msg",0,0,0 );
    }
  }

} /* dsqmhllif_netplatform_cmd_cb */


/*===========================================================================
FUNCTION DSQMHLLIF_NETPLAT_IND_CB

DESCRIPTION
  Network Platform layer indication callback.  Reports asynchronous
  event notification.

PARAMETERS
  TBD

RETURN VALUE
  None

DEPENDENCIES
  None.

SIDE EFFECTS
  None
===========================================================================*/
LOCAL void dsqmhllif_netplat_ind_cb
(
  ds_netplat_ind_msg_type   ind_id,
  ds_netplat_info_type     *ind_info_ptr,
  void                     *user_data
)
{
  dsqmh_msg_buf_type      *msg_ptr = NULL;    /* Pointer to message buffer */
  dsqmh_iface_cblk_type   *cblk_ptr = NULL;   /* Pointer to control block  */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( ind_info_ptr, "DSQMH LLIF null ind data pointer passed" );

  DSQMH_MSG_MED( "QMH LLIF net platform ind cb:%p ind=%d",
                 (uint32)user_data, ind_id, 0 );

  /*-------------------------------------------------------------------------
    Verify indication is for this client.
  -------------------------------------------------------------------------*/
  cblk_ptr = DSQMH_GET_CBLK_PTR( (int32)user_data );
  if( ind_info_ptr->conn_id != cblk_ptr->netplat_info.conn_id )
  {
    DSQMH_MSG_ERROR( "QMH LLIF ind not for this connection:%d",
                     ind_info_ptr->conn_id,0,0 );
    return;
  }

  /*-------------------------------------------------------------------------
    Send asynchronous message for notification to state machine.
  -------------------------------------------------------------------------*/
  DSQMH_GET_MSG_BUF( msg_ptr );
  if( NULL != msg_ptr )
  {
    boolean send_msg = TRUE;

    memset((void*)msg_ptr, 0x0, sizeof(dsqmh_msg_buf_type));

    /*-----------------------------------------------------------------------
     Populate message buffer with context info
    -----------------------------------------------------------------------*/
    switch( ind_id )
    {
    case DS_NETPLAT_IND_PLATFORM_UP:
      msg_ptr->msg_id = PROXY_IFACE_PLATFORM_UP_IND;
      DSQMH_MSG_MED( "Posting DS cmd: PROXY_IFACE_PLATFORM_UP_IND",0,0,0);
      break;

    case DS_NETPLAT_IND_PLATFORM_DOWN:
      msg_ptr->msg_id = PROXY_IFACE_PLATFORM_DOWN_IND;
      DSQMH_MSG_MED( "Posting DS cmd: PROXY_IFACE_PLATFORM_DOWN_IND",0,0,0);
      break;

    case  DS_NETPLAT_IND_QOS_REQUEST:
    case  DS_NETPLAT_IND_QOS_RELEASE:
      msg_ptr->msg_id = PROXY_IFACE_PLATFORM_QOS_IND;
      msg_ptr->info.netplat.flow_ptr = (ps_flow_type*)user_data;
      DSQMH_MSG_MED( "Posting DS cmd: PROXY_IFACE_PLATFORM_QOS_IND",0,0,0);
      break;

    default:
      DSQMH_MSG_ERROR( "QMH LLIF unsupported ind: id=",ind_id,0,0 );
      send_msg = FALSE;
      break;
    }

    if( send_msg )
    {
      msg_ptr->iface_inst = (uint32)ind_info_ptr->info_ptr;
      msg_ptr->info.netplat.ind_id = ind_id;
      msg_ptr->info.netplat.info = *ind_info_ptr;

      DSQMH_PUT_MSG_BUF( msg_ptr );
    }
    else
    {
      /* Release the message buffer on error condition. */
      DSQMH_RELEASE_MSG_BUF( msg_ptr );
    }
  }
  else
  {
    DSQMH_MSG_FATAL( "Failed to allocate msg buffer, dropping msg",0,0,0 );
  }

} /* dsqmhllif_netplatform_ind_cb */



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
)
{
  int ret_val;
  dsqmh_iface_cblk_type *cblk_ptr = NULL;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_MED( "QMH LLIF init netplatform", 0, 0, 0 );

  cblk_ptr = DSQMH_GET_CBLK_PTR( iface_inst );

  /*-----------------------------------------------------------------------
    Register as client of Network Platform Layer.
  -----------------------------------------------------------------------*/
  ret_val = ds_qmh_netplat_open( dsqmhllif_netplat_ind_cb,
                                 (void*)iface_inst,
                                 &cblk_ptr->netplat_info.conn_id );

  if( DSQMH_SUCCESS != ret_val )
  {
    DSQMH_MSG_ERROR( "QMH LLIF failed on netplat register client", 0, 0, 0 );
    return DSQMH_FAILED;
  }

  return DSQMH_SUCCESS;
} /* dsqmhllif_init_netplatform() */


/*===========================================================================
FUNCTION dsqmhllif_qos_conv_flow_params_to_qmi

DESCRIPTION
  This function converts the PS Flow QOS flow parameters into that
  used by QMI Message Library.

PARAMETERS
  qos_spec_tech    - Um IFACE Bearer technology (UMTS|CDMA)
  ip_flow_ptr      - Pointer to PS flow params (input)
  qmi_flow_ptr     - Pointer to QMI flow params (output)
  num_flows_ptr    - Pointer to number of requested flows (output)

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/
LOCAL void dsqmhllif_qos_conv_flow_params_to_qmi
(
  qmi_qos_technology_type        qos_spec_tech,    /* Bearer tech (UMTS|CDMA)    */
  const ip_flow_type      *ip_flow_ptr,      /* Pointer to PS flow params  */
  qmi_qos_flow_req_type   *qmi_flow_ptr,     /* Pointer to QMI flow params */
  uint32                  *num_flows_ptr     /* Pointer to number of flows */
)
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( ip_flow_ptr, "DSQMH LLIF null PS flow pointer passed" );
  DSQMH_ASSERT( qmi_flow_ptr, "DSQMH LLIF null qmi flow pointer passed" );
  DSQMH_ASSERT( num_flows_ptr, "DSQMH LLIF null num flos pointer passed" );

  (void)qos_spec_tech; /* Not used now. */

  /*-----------------------------------------------------------------------
    Process UMTS QOS Flow parameters.
  -----------------------------------------------------------------------*/
  DSQMH_SET_QMI_QOS_PARAM( qmi_flow_ptr->umts_flow_desc.traffic_class,
                           QMI_QOS_UMTS_FLOW_PARAM_TRAFFIC_CLASS,
                           qmi_flow_ptr->umts_flow_desc.param_mask,
                           (qmi_qos_umts_traffic_class_type)ip_flow_ptr->trf_class,
                           IPFLOW_MASK_TRF_CLASS,
                           ip_flow_ptr->field_mask );

  if( DATA_RATE_FORMAT_MIN_MAX_TYPE == ip_flow_ptr->data_rate.format_type )
  {
    DSQMH_SET_QMI_QOS_PARAM( qmi_flow_ptr->umts_flow_desc.data_rate.max_rate,
                             QMI_QOS_UMTS_FLOW_PARAM_DATA_RATE,
                             qmi_flow_ptr->umts_flow_desc.param_mask,
                             ip_flow_ptr->data_rate.format.min_max.max_rate,
                             IPFLOW_MASK_DATA_RATE,
                             ip_flow_ptr->field_mask );

    DSQMH_SET_QMI_QOS_PARAM( qmi_flow_ptr->umts_flow_desc.data_rate.guaranteed_rate,
                             QMI_QOS_UMTS_FLOW_PARAM_DATA_RATE,
                             qmi_flow_ptr->umts_flow_desc.param_mask,
                             ip_flow_ptr->data_rate.format.min_max.guaranteed_rate,
                             IPFLOW_MASK_DATA_RATE,
                             ip_flow_ptr->field_mask );
  }
  else
  {

    DSQMH_SET_QMI_QOS_PARAM( qmi_flow_ptr->umts_flow_desc.bucket_info.peak_rate,
                             QMI_QOS_UMTS_FLOW_PARAM_BUCKET_INFO,
                             qmi_flow_ptr->umts_flow_desc.param_mask,
                             ip_flow_ptr->data_rate.format.token_bucket.peak_rate,
                             IPFLOW_MASK_DATA_RATE,
                             ip_flow_ptr->field_mask );

    DSQMH_SET_QMI_QOS_PARAM( qmi_flow_ptr->umts_flow_desc.bucket_info.token_rate,
                             QMI_QOS_UMTS_FLOW_PARAM_BUCKET_INFO,
                             qmi_flow_ptr->umts_flow_desc.param_mask,
                             ip_flow_ptr->data_rate.format.token_bucket.token_rate,
                             IPFLOW_MASK_DATA_RATE,
                             ip_flow_ptr->field_mask );

    DSQMH_SET_QMI_QOS_PARAM( qmi_flow_ptr->umts_flow_desc.bucket_info.bucket_size,
                             QMI_QOS_UMTS_FLOW_PARAM_BUCKET_INFO,
                             qmi_flow_ptr->umts_flow_desc.param_mask,
                             ip_flow_ptr->data_rate.format.token_bucket.size,
                             IPFLOW_MASK_DATA_RATE,
                             ip_flow_ptr->field_mask );
  }

  DSQMH_SET_QMI_QOS_PARAM( qmi_flow_ptr->umts_flow_desc.max_delay,
                           QMI_QOS_UMTS_FLOW_PARAM_MAX_DELAY,
                           qmi_flow_ptr->umts_flow_desc.param_mask,
                           ip_flow_ptr->latency,
                           IPFLOW_MASK_LATENCY,
                           ip_flow_ptr->field_mask );

  DSQMH_SET_QMI_QOS_PARAM( qmi_flow_ptr->umts_flow_desc.max_jitter,
                           QMI_QOS_UMTS_FLOW_PARAM_MAX_JITTER,
                           qmi_flow_ptr->umts_flow_desc.param_mask,
                           ip_flow_ptr->latency_var,
                           IPFLOW_MASK_LATENCY_VAR,
                           ip_flow_ptr->field_mask );

  DSQMH_SET_QMI_QOS_PARAM( qmi_flow_ptr->umts_flow_desc.pkt_err_rate.multiplier,
                           QMI_QOS_UMTS_FLOW_PARAM_PKT_ERR_RATE,
                           qmi_flow_ptr->umts_flow_desc.param_mask,
                           ip_flow_ptr->pkt_err_rate.multiplier,
                           IPFLOW_MASK_PKT_ERR_RATE,
                           ip_flow_ptr->field_mask );
  DSQMH_SET_QMI_QOS_PARAM( qmi_flow_ptr->umts_flow_desc.pkt_err_rate.exponent,
                           QMI_QOS_UMTS_FLOW_PARAM_PKT_ERR_RATE,
                           qmi_flow_ptr->umts_flow_desc.param_mask,
                           ip_flow_ptr->pkt_err_rate.exponent,
                           IPFLOW_MASK_PKT_ERR_RATE,
                           ip_flow_ptr->field_mask );

  DSQMH_SET_QMI_QOS_PARAM( qmi_flow_ptr->umts_flow_desc.min_policed_pkt_sz,
                           QMI_QOS_UMTS_FLOW_PARAM_MIN_POL_PKT_SZ,
                           qmi_flow_ptr->umts_flow_desc.param_mask,
                           ip_flow_ptr->min_policed_pkt_size,
                           IPFLOW_MASK_MIN_POLICED_PKT_SIZE,
                           ip_flow_ptr->field_mask );

  DSQMH_SET_QMI_QOS_PARAM( qmi_flow_ptr->umts_flow_desc.max_allowed_pkt_sz,
                           QMI_QOS_UMTS_FLOW_PARAM_MAX_ALLOW_PKT_SZ,
                           qmi_flow_ptr->umts_flow_desc.param_mask,
                           ip_flow_ptr->max_allowed_pkt_size,
                           IPFLOW_MASK_MAX_ALLOWED_PKT_SIZE,
                           ip_flow_ptr->field_mask );

  DSQMH_SET_QMI_QOS_PARAM( qmi_flow_ptr->umts_flow_desc.residual_ber,
                           QMI_QOS_UMTS_FLOW_PARAM_RESIDUAL_BER,
                           qmi_flow_ptr->umts_flow_desc.param_mask,
                           (qmi_qos_umts_residual_ber_type)ip_flow_ptr->umts_params.res_ber,
                           IPFLOW_MASK_UMTS_RES_BER,
                           ip_flow_ptr->field_mask );

  DSQMH_SET_QMI_QOS_PARAM( qmi_flow_ptr->umts_flow_desc.handling_prio,
                           QMI_QOS_UMTS_FLOW_PARAM_HANDLING_PRIO,
                           qmi_flow_ptr->umts_flow_desc.param_mask,
                           (qmi_qos_umts_handling_prio_type)ip_flow_ptr->umts_params.trf_pri,
                           IPFLOW_MASK_UMTS_TRF_PRI,
                           ip_flow_ptr->field_mask );

  /*-----------------------------------------------------------------------
    Process CDMA QOS Flow parameters.
  -----------------------------------------------------------------------*/
  DSQMH_SET_QMI_QOS_PARAM( qmi_flow_ptr->cdma_flow_desc.profile_id,
                           QMI_QOS_CDMA_FLOW_PARAM_PROFILE_ID,
                           qmi_flow_ptr->cdma_flow_desc.param_mask,
                           (unsigned long)ip_flow_ptr->cdma_params.profile_id,
                           IPFLOW_MASK_CDMA_PROFILE_ID,
                           ip_flow_ptr->field_mask );

  /* Increment the flow counter */
  (*num_flows_ptr)++;

} /* dsqmhllif_qos_conv_flow_params_to_qmi() */

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
  qmi_qos_technology_type      qos_spec_tech,      /* Bearer tech (UMTS|CDMA)    */
  const qmi_qos_flow_req_type *qmi_flow_ptr, /* Pointer to QMI flow params */
  ip_flow_type          *ip_flow_ptr,        /* Pointer to PS flow params  */
  uint32                *num_flows_ptr       /* Pointer to number of flows */
)
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( ip_flow_ptr, "DSQMH LLIF null PS flow pointer passed" );
  DSQMH_ASSERT( qmi_flow_ptr, "DSQMH LLIF null qmi flow pointer passed" );
  DSQMH_ASSERT( num_flows_ptr, "DSQMH LLIF null num flos pointer passed" );

  memset((void*)ip_flow_ptr, 0x0, sizeof(ip_flow_type));
  (void)qos_spec_tech; /* Not used now. */

  /*-----------------------------------------------------------------------
    Process UMTS QOS Flow parameters.
  -----------------------------------------------------------------------*/
  DSQMH_SET_QMI_QOS_PARAM( ip_flow_ptr->trf_class,
                           IPFLOW_MASK_TRF_CLASS,
                           ip_flow_ptr->field_mask,
                           (ip_traffic_class_enum_type)qmi_flow_ptr->umts_flow_desc.traffic_class,
                           QMI_QOS_UMTS_FLOW_PARAM_TRAFFIC_CLASS,
                           qmi_flow_ptr->umts_flow_desc.param_mask );
    
  if( QMI_QOS_UMTS_FLOW_PARAM_DATA_RATE & qmi_flow_ptr->umts_flow_desc.param_mask )
  {
    ip_flow_ptr->data_rate.format_type = DATA_RATE_FORMAT_MIN_MAX_TYPE;
    
    DSQMH_SET_QMI_QOS_PARAM( ip_flow_ptr->data_rate.format.min_max.max_rate,
                             IPFLOW_MASK_DATA_RATE,
                             ip_flow_ptr->field_mask,
                             qmi_flow_ptr->umts_flow_desc.data_rate.max_rate,
                             QMI_QOS_UMTS_FLOW_PARAM_DATA_RATE,
                             qmi_flow_ptr->umts_flow_desc.param_mask );

    DSQMH_SET_QMI_QOS_PARAM( ip_flow_ptr->data_rate.format.min_max.guaranteed_rate,
                             IPFLOW_MASK_DATA_RATE,
                             ip_flow_ptr->field_mask,
                             qmi_flow_ptr->umts_flow_desc.data_rate.guaranteed_rate,
                             QMI_QOS_UMTS_FLOW_PARAM_DATA_RATE,
                             qmi_flow_ptr->umts_flow_desc.param_mask );
  }
  else
  {
    ip_flow_ptr->data_rate.format_type = DATA_RATE_FORMAT_TOKEN_BUCKET_TYPE;

    DSQMH_SET_QMI_QOS_PARAM( ip_flow_ptr->data_rate.format.token_bucket.peak_rate,
                             IPFLOW_MASK_DATA_RATE,
                             ip_flow_ptr->field_mask,
                             qmi_flow_ptr->umts_flow_desc.bucket_info.peak_rate,
                             QMI_QOS_UMTS_FLOW_PARAM_BUCKET_INFO,
                             qmi_flow_ptr->umts_flow_desc.param_mask );

    DSQMH_SET_QMI_QOS_PARAM( ip_flow_ptr->data_rate.format.token_bucket.token_rate,
                             IPFLOW_MASK_DATA_RATE,
                             ip_flow_ptr->field_mask,
                             qmi_flow_ptr->umts_flow_desc.bucket_info.token_rate,
                             QMI_QOS_UMTS_FLOW_PARAM_BUCKET_INFO,
                             qmi_flow_ptr->umts_flow_desc.param_mask );

    DSQMH_SET_QMI_QOS_PARAM( ip_flow_ptr->data_rate.format.token_bucket.size,
                             IPFLOW_MASK_DATA_RATE,
                             ip_flow_ptr->field_mask,
                             qmi_flow_ptr->umts_flow_desc.bucket_info.bucket_size ,
                             QMI_QOS_UMTS_FLOW_PARAM_BUCKET_INFO,
                             qmi_flow_ptr->umts_flow_desc.param_mask );

  }

  DSQMH_SET_QMI_QOS_PARAM( ip_flow_ptr->latency,
                           IPFLOW_MASK_LATENCY,
                           ip_flow_ptr->field_mask,
                           qmi_flow_ptr->umts_flow_desc.max_delay,
                           QMI_QOS_UMTS_FLOW_PARAM_MAX_DELAY,
                           qmi_flow_ptr->umts_flow_desc.param_mask );

  DSQMH_SET_QMI_QOS_PARAM( ip_flow_ptr->latency_var,
                           IPFLOW_MASK_LATENCY_VAR,
                           ip_flow_ptr->field_mask,
                           qmi_flow_ptr->umts_flow_desc.max_jitter,
                           QMI_QOS_UMTS_FLOW_PARAM_MAX_JITTER,
                           qmi_flow_ptr->umts_flow_desc.param_mask );

  DSQMH_SET_QMI_QOS_PARAM( ip_flow_ptr->pkt_err_rate.multiplier,
                           IPFLOW_MASK_PKT_ERR_RATE,
                           ip_flow_ptr->field_mask,
                           ((uint16)qmi_flow_ptr->umts_flow_desc.pkt_err_rate.multiplier),
                           QMI_QOS_UMTS_FLOW_PARAM_PKT_ERR_RATE,
                           qmi_flow_ptr->umts_flow_desc.param_mask );

  DSQMH_SET_QMI_QOS_PARAM( ip_flow_ptr->pkt_err_rate.exponent,
                           IPFLOW_MASK_PKT_ERR_RATE,
                           ip_flow_ptr->field_mask,
                           ((uint16)qmi_flow_ptr->umts_flow_desc.pkt_err_rate.exponent),
                           QMI_QOS_UMTS_FLOW_PARAM_PKT_ERR_RATE,
                           qmi_flow_ptr->umts_flow_desc.param_mask );

  DSQMH_SET_QMI_QOS_PARAM( ip_flow_ptr->min_policed_pkt_size,
                           IPFLOW_MASK_MIN_POLICED_PKT_SIZE,
                           ip_flow_ptr->field_mask,
                           qmi_flow_ptr->umts_flow_desc.min_policed_pkt_sz,
                           QMI_QOS_UMTS_FLOW_PARAM_MIN_POL_PKT_SZ,
                           qmi_flow_ptr->umts_flow_desc.param_mask );

  DSQMH_SET_QMI_QOS_PARAM( ip_flow_ptr->max_allowed_pkt_size,
                           IPFLOW_MASK_MAX_ALLOWED_PKT_SIZE,
                           ip_flow_ptr->field_mask,
                           qmi_flow_ptr->umts_flow_desc.max_allowed_pkt_sz,
                           QMI_QOS_UMTS_FLOW_PARAM_MAX_ALLOW_PKT_SZ,
                           qmi_flow_ptr->umts_flow_desc.param_mask );

  DSQMH_SET_QMI_QOS_PARAM( ip_flow_ptr->umts_params.res_ber,
                           IPFLOW_MASK_UMTS_RES_BER,
                           ip_flow_ptr->field_mask,
                           ((umts_residual_ber_enum_type)qmi_flow_ptr->umts_flow_desc.residual_ber),
                           QMI_QOS_UMTS_FLOW_PARAM_RESIDUAL_BER,
                           qmi_flow_ptr->umts_flow_desc.param_mask );

  DSQMH_SET_QMI_QOS_PARAM( ip_flow_ptr->umts_params.trf_pri,
                           IPFLOW_MASK_UMTS_TRF_PRI,
                           ip_flow_ptr->field_mask,
                           ((umts_trf_handling_pri_enum_type)qmi_flow_ptr->umts_flow_desc.handling_prio),
                           QMI_QOS_UMTS_FLOW_PARAM_HANDLING_PRIO,
                           qmi_flow_ptr->umts_flow_desc.param_mask );

  /*-----------------------------------------------------------------------
    Process CDMA QOS Flow parameters.
  -----------------------------------------------------------------------*/
  DSQMH_SET_QMI_QOS_PARAM( ip_flow_ptr->cdma_params.profile_id,
                           IPFLOW_MASK_CDMA_PROFILE_ID,
                           ip_flow_ptr->field_mask,
                           (cdma_flow_spec_profile_id_type)
                           qmi_flow_ptr->cdma_flow_desc.profile_id,
                           QMI_QOS_CDMA_FLOW_PARAM_PROFILE_ID,
                           qmi_flow_ptr->cdma_flow_desc.param_mask );

  (*num_flows_ptr)++;

} /* dsqmhllif_qos_conv_flow_params_to_ps() */

/*===========================================================================
FUNCTION dsqmhllif_qos_set_protocol_tcp_params

DESCRIPTION
  This function converts the PS Flow QOS filter TCP protocol parameters 
  into that used by QMI Message Library.

PARAMETERS
  fltr_buf_ptr        - Pointer to PS QOS spec
  qos_fltr_spec_ptr   - Pointer to QMI QOS specification (output)  

DEPENDENCIES
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/
LOCAL void dsqmhllif_qos_set_protocol_tcp_params
(
  ip_filter_type  *fltr_buf_ptr,               /* Pointer to PS QOS spec   */
  qmi_qos_filter_req_type *qos_fltr_spec_ptr   /* Pointer to QMI QOS spec  */  
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( fltr_buf_ptr, "DSQMH LLIF null qos filter pointer passed" );
  DSQMH_ASSERT( qos_fltr_spec_ptr, "DSQMH LLIF null qos spec pointer passed" );

  qos_fltr_spec_ptr->filter_desc.protocol = QMI_QOS_TRANS_PROT_TCP;
  qos_fltr_spec_ptr->filter_desc.param_mask |= QMI_QOS_FILTER_PARAM_TRANS_PROTOCOL;

  DSQMH_SET_QMI_QOS_PARAM( qos_fltr_spec_ptr->filter_desc.tcp_src_ports.start_port,
                           QMI_QOS_FILTER_PARAM_TCP_SRC_PORTS,
                           qos_fltr_spec_ptr->filter_desc.param_mask,
                           ps_ntohs( fltr_buf_ptr->next_prot_hdr.tcp.src.port ),
                           IPFLTR_MASK_TCP_SRC_PORT,
                           fltr_buf_ptr->next_prot_hdr.tcp.field_mask );

  DSQMH_SET_QMI_QOS_PARAM( qos_fltr_spec_ptr->filter_desc.tcp_src_ports.range,
                           QMI_QOS_FILTER_PARAM_TCP_SRC_PORTS,
                           qos_fltr_spec_ptr->filter_desc.param_mask,
                           fltr_buf_ptr->next_prot_hdr.tcp.src.range,
                           IPFLTR_MASK_TCP_SRC_PORT,
                           fltr_buf_ptr->next_prot_hdr.tcp.field_mask );

  DSQMH_SET_QMI_QOS_PARAM( qos_fltr_spec_ptr->filter_desc.tcp_dest_ports.start_port,
                           QMI_QOS_FILTER_PARAM_TCP_DEST_PORTS,
                           qos_fltr_spec_ptr->filter_desc.param_mask,
                           ps_ntohs( fltr_buf_ptr->next_prot_hdr.tcp.dst.port ),
                           IPFLTR_MASK_TCP_DST_PORT,
                           fltr_buf_ptr->next_prot_hdr.tcp.field_mask );

  DSQMH_SET_QMI_QOS_PARAM( qos_fltr_spec_ptr->filter_desc.tcp_dest_ports.range,
                           QMI_QOS_FILTER_PARAM_TCP_DEST_PORTS,
                           qos_fltr_spec_ptr->filter_desc.param_mask,
                           fltr_buf_ptr->next_prot_hdr.tcp.dst.range,
                           IPFLTR_MASK_TCP_DST_PORT,
                           fltr_buf_ptr->next_prot_hdr.tcp.field_mask );

} /* dsqmhllif_qos_set_protocol_tcp_params() */

/*===========================================================================
FUNCTION dsqmhllif_qos_set_protocol_udp_params

DESCRIPTION
  This function converts the PS Flow QOS filter UDP protocol parameters 
  into that used by QMI Message Library.

PARAMETERS
  fltr_buf_ptr        - Pointer to PS QOS spec
  qos_fltr_spec_ptr   - Pointer to QMI QOS specification (output)  

DEPENDENCIES
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/
LOCAL void dsqmhllif_qos_set_protocol_udp_params
(
  ip_filter_type  *fltr_buf_ptr,               /* Pointer to PS QOS spec   */
  qmi_qos_filter_req_type *qos_fltr_spec_ptr   /* Pointer to QMI QOS spec  */  
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( fltr_buf_ptr, "DSQMH LLIF null qos filter pointer passed" );
  DSQMH_ASSERT( qos_fltr_spec_ptr, "DSQMH LLIF null qos spec pointer passed" );

  qos_fltr_spec_ptr->filter_desc.protocol = QMI_QOS_TRANS_PROT_UDP;
  qos_fltr_spec_ptr->filter_desc.param_mask |= QMI_QOS_FILTER_PARAM_TRANS_PROTOCOL;

  DSQMH_SET_QMI_QOS_PARAM( qos_fltr_spec_ptr->filter_desc.udp_src_ports.start_port,
                           QMI_QOS_FILTER_PARAM_UDP_SRC_PORTS,
                           qos_fltr_spec_ptr->filter_desc.param_mask,
                           ps_ntohs( fltr_buf_ptr->next_prot_hdr.udp.src.port ),
                           IPFLTR_MASK_UDP_SRC_PORT,
                           fltr_buf_ptr->next_prot_hdr.udp.field_mask );

  DSQMH_SET_QMI_QOS_PARAM( qos_fltr_spec_ptr->filter_desc.udp_src_ports.range,
                           QMI_QOS_FILTER_PARAM_UDP_SRC_PORTS,
                           qos_fltr_spec_ptr->filter_desc.param_mask,
                           fltr_buf_ptr->next_prot_hdr.udp.src.range,
                           IPFLTR_MASK_UDP_SRC_PORT,
                           fltr_buf_ptr->next_prot_hdr.udp.field_mask );

  DSQMH_SET_QMI_QOS_PARAM( qos_fltr_spec_ptr->filter_desc.udp_dest_ports.start_port,
                           QMI_QOS_FILTER_PARAM_UDP_DEST_PORTS,
                           qos_fltr_spec_ptr->filter_desc.param_mask,
                           ps_ntohs( fltr_buf_ptr->next_prot_hdr.udp.dst.port ),
                           IPFLTR_MASK_UDP_DST_PORT,
                           fltr_buf_ptr->next_prot_hdr.udp.field_mask );

  DSQMH_SET_QMI_QOS_PARAM( qos_fltr_spec_ptr->filter_desc.udp_dest_ports.range,
                           QMI_QOS_FILTER_PARAM_UDP_DEST_PORTS,
                           qos_fltr_spec_ptr->filter_desc.param_mask,
                           fltr_buf_ptr->next_prot_hdr.udp.dst.range,
                           IPFLTR_MASK_UDP_DST_PORT,
                           fltr_buf_ptr->next_prot_hdr.udp.field_mask );

} /* dsqmhllif_qos_set_protocol_udp_params() */

/*===========================================================================
FUNCTION dsqmhllif_qos_conv_filter_params

DESCRIPTION
  This function converts the PS Flow QOS filter parameters into that
  used by QMI Message Library.

PARAMETERS
  fltr_buf_ptr        - Pointer to PS QOS spec
  qos_fltr_spec_ptr   - Pointer to QMI QOS specification (output)

DEPENDENCIES
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/
LOCAL int dsqmhllif_qos_conv_filter_params
(
  ip_filter_type  *fltr_buf_ptr,               /* Pointer to PS QOS spec   */
  qmi_qos_filter_req_type *qos_fltr_spec_ptr   /* Pointer to QMI QOS spec  */
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( fltr_buf_ptr, "DSQMH LLIF null qos filter pointer passed" );
  DSQMH_ASSERT( qos_fltr_spec_ptr, "DSQMH LLIF null qos spec pointer passed" );

  DSQMH_MSG_LOW( "QMH LLIF qos conv rx filter:0x%p", fltr_buf_ptr, 0, 0 );

  /*-----------------------------------------------------------------------
    Convert PS filter paramaters to QMI structure.    
  -----------------------------------------------------------------------*/
  qos_fltr_spec_ptr->ip_version = (qmi_qos_ip_version_type) fltr_buf_ptr->ip_vsn;

  if( IP_V4 == fltr_buf_ptr->ip_vsn )
  {    
    DSQMH_SET_QMI_QOS_PARAM( qos_fltr_spec_ptr->filter_desc.src_addr.ipv4_ip_addr,
                             QMI_QOS_FILTER_PARAM_SRC_ADDR,
                             qos_fltr_spec_ptr->filter_desc.param_mask,
                             ps_ntohl( fltr_buf_ptr->ip_hdr.v4.src.addr.ps_s_addr ),
                             IPFLTR_MASK_IP4_SRC_ADDR,
                             fltr_buf_ptr->ip_hdr.v4.field_mask );

    DSQMH_SET_QMI_QOS_PARAM( qos_fltr_spec_ptr->filter_desc.src_addr.ipv4_subnet_mask,
                             QMI_QOS_FILTER_PARAM_SRC_ADDR,
                             qos_fltr_spec_ptr->filter_desc.param_mask,
                             ps_ntohl( fltr_buf_ptr->ip_hdr.v4.src.subnet_mask.ps_s_addr ),
                             IPFLTR_MASK_IP4_SRC_ADDR,
                             fltr_buf_ptr->ip_hdr.v4.field_mask );


    DSQMH_SET_QMI_QOS_PARAM( qos_fltr_spec_ptr->filter_desc.dest_addr.ipv4_ip_addr,
                             QMI_QOS_FILTER_PARAM_DEST_ADDR,
                             qos_fltr_spec_ptr->filter_desc.param_mask,
                             ps_ntohl( fltr_buf_ptr->ip_hdr.v4.dst.addr.ps_s_addr ),
                             IPFLTR_MASK_IP4_DST_ADDR,
                             fltr_buf_ptr->ip_hdr.v4.field_mask );

    DSQMH_SET_QMI_QOS_PARAM( qos_fltr_spec_ptr->filter_desc.dest_addr.ipv4_subnet_mask,
                             QMI_QOS_FILTER_PARAM_DEST_ADDR,
                             qos_fltr_spec_ptr->filter_desc.param_mask,
                             ps_ntohl( fltr_buf_ptr->ip_hdr.v4.dst.subnet_mask.ps_s_addr ),
                             IPFLTR_MASK_IP4_DST_ADDR,
                             fltr_buf_ptr->ip_hdr.v4.field_mask );


    DSQMH_SET_QMI_QOS_PARAM( qos_fltr_spec_ptr->filter_desc.tos.tos_value,
                             QMI_QOS_FILTER_PARAM_TOS,
                             qos_fltr_spec_ptr->filter_desc.param_mask,
                             fltr_buf_ptr->ip_hdr.v4.tos.val,
                             IPFLTR_MASK_IP4_TOS,
                             fltr_buf_ptr->ip_hdr.v4.field_mask );

    DSQMH_SET_QMI_QOS_PARAM( qos_fltr_spec_ptr->filter_desc.tos.tos_mask,
                             QMI_QOS_FILTER_PARAM_TOS,
                             qos_fltr_spec_ptr->filter_desc.param_mask,
                             fltr_buf_ptr->ip_hdr.v4.tos.mask,
                             IPFLTR_MASK_IP4_TOS,
                             fltr_buf_ptr->ip_hdr.v4.field_mask );

    /*---------------------------------------------------------------------
      Check for next protocol header
    ---------------------------------------------------------------------*/
    if( (uint32)IPFLTR_MASK_IP4_NEXT_HDR_PROT & fltr_buf_ptr->ip_hdr.v4.field_mask )
    {
      if( (uint32)PS_IPPROTO_TCP == fltr_buf_ptr->ip_hdr.v4.next_hdr_prot )
      {
        dsqmhllif_qos_set_protocol_tcp_params(fltr_buf_ptr, qos_fltr_spec_ptr);
      }
      else if( (uint32)PS_IPPROTO_UDP == fltr_buf_ptr->ip_hdr.v4.next_hdr_prot )
      {
        dsqmhllif_qos_set_protocol_udp_params(fltr_buf_ptr, qos_fltr_spec_ptr);
      }
      else
      {
        DSQMH_MSG_ERROR( "QMH LLIF filter protocol not supported:%d",
                         fltr_buf_ptr->ip_hdr.v4.next_hdr_prot, 0, 0 );
        fltr_buf_ptr->ip_hdr.v4.err_mask |= (uint32)IPFLTR_MASK_IP4_NEXT_HDR_PROT;
        return DSQMH_FAILED;
      }
    }
  }
  /* support for IPv6 address */
  else if( IP_V6 == fltr_buf_ptr->ip_vsn )
  {    
    DSQMH_SET_QMI_QOS_V6_ADDR_PARAM( qos_fltr_spec_ptr->filter_desc.ipv6_src_addr.ipv6_ip_addr,
                             QMI_QOS_FILTER_PARAM_IPV6_SRC_ADDR,
                             qos_fltr_spec_ptr->filter_desc.param_mask,
                             fltr_buf_ptr->ip_hdr.v6.src.addr.ps_s6_addr,
                             IPFLTR_MASK_IP6_SRC_ADDR,
                             fltr_buf_ptr->ip_hdr.v6.field_mask );

    DSQMH_SET_QMI_QOS_PARAM( qos_fltr_spec_ptr->filter_desc.ipv6_src_addr.ipv6_filter_prefix_len,
                             QMI_QOS_FILTER_PARAM_IPV6_SRC_ADDR,
                             qos_fltr_spec_ptr->filter_desc.param_mask,
                             fltr_buf_ptr->ip_hdr.v6.src.prefix_len,
                             IPFLTR_MASK_IP6_SRC_ADDR,
                             fltr_buf_ptr->ip_hdr.v6.field_mask );

    DSQMH_SET_QMI_QOS_V6_ADDR_PARAM( qos_fltr_spec_ptr->filter_desc.ipv6_dest_addr.ipv6_ip_addr,
                             QMI_QOS_FILTER_PARAM_IPV6_DEST_ADDR,
                             qos_fltr_spec_ptr->filter_desc.param_mask,
                             fltr_buf_ptr->ip_hdr.v6.dst.addr.ps_s6_addr,
                             IPFLTR_MASK_IP6_DST_ADDR,
                             fltr_buf_ptr->ip_hdr.v6.field_mask );

    DSQMH_SET_QMI_QOS_PARAM( qos_fltr_spec_ptr->filter_desc.ipv6_dest_addr.ipv6_filter_prefix_len,
                             QMI_QOS_FILTER_PARAM_IPV6_DEST_ADDR,
                             qos_fltr_spec_ptr->filter_desc.param_mask,
                             fltr_buf_ptr->ip_hdr.v6.dst.prefix_len,
                             IPFLTR_MASK_IP6_DST_ADDR,
                             fltr_buf_ptr->ip_hdr.v6.field_mask );


    DSQMH_SET_QMI_QOS_PARAM( qos_fltr_spec_ptr->filter_desc.ipv6_traffic_class.traffic_class_value,
                             QMI_QOS_FILTER_PARAM_IPV6_TRAFFIC_CLASS,
                             qos_fltr_spec_ptr->filter_desc.param_mask,
                             fltr_buf_ptr->ip_hdr.v6.trf_cls.val,
                             IPFLTR_MASK_IP6_TRAFFIC_CLASS,
                             fltr_buf_ptr->ip_hdr.v6.field_mask );

    DSQMH_SET_QMI_QOS_PARAM( qos_fltr_spec_ptr->filter_desc.ipv6_traffic_class.traffic_class_mask,
                             QMI_QOS_FILTER_PARAM_IPV6_TRAFFIC_CLASS,
                             qos_fltr_spec_ptr->filter_desc.param_mask,
                             fltr_buf_ptr->ip_hdr.v6.trf_cls.mask,
                             IPFLTR_MASK_IP6_TRAFFIC_CLASS,
                             fltr_buf_ptr->ip_hdr.v6.field_mask );

        DSQMH_SET_QMI_QOS_PARAM( qos_fltr_spec_ptr->filter_desc.ipv6_flow_label,
                             QMI_QOS_FILTER_PARAM_IPV6_FLOW_LABEL,
                             qos_fltr_spec_ptr->filter_desc.param_mask,
                             fltr_buf_ptr->ip_hdr.v6.flow_label,
                             IPFLTR_MASK_IP6_FLOW_LABEL,
                             fltr_buf_ptr->ip_hdr.v6.field_mask );

    /*---------------------------------------------------------------------
      Check for next protocol header
    ---------------------------------------------------------------------*/
    if( (uint32)IPFLTR_MASK_IP6_NEXT_HDR_PROT & fltr_buf_ptr->ip_hdr.v6.field_mask )
    {
      if( (uint32)PS_IPPROTO_TCP == fltr_buf_ptr->ip_hdr.v6.next_hdr_prot )
      {
        dsqmhllif_qos_set_protocol_tcp_params(fltr_buf_ptr, qos_fltr_spec_ptr);
      }
      else if( (uint32)PS_IPPROTO_UDP == fltr_buf_ptr->ip_hdr.v6.next_hdr_prot )
      {
        dsqmhllif_qos_set_protocol_udp_params(fltr_buf_ptr, qos_fltr_spec_ptr);
      }
      else
      {
        DSQMH_MSG_ERROR( "QMH LLIF filter protocol not supported:%d",
                         fltr_buf_ptr->ip_hdr.v6.next_hdr_prot, 0, 0 );
        fltr_buf_ptr->ip_hdr.v6.err_mask |= (uint32)IPFLTR_MASK_IP6_NEXT_HDR_PROT;
        return DSQMH_FAILED;
      }
    }
  }
  else
  {
    DSQMH_MSG_ERROR( "QMH LLIF filters not supported", 0, 0, 0 );
    return DSQMH_FAILED;
  }

  /*---------------------------------------------------------------------
      Add common TLV, ESP SPI
  ---------------------------------------------------------------------*/
  DSQMH_SET_QMI_QOS_PARAM( qos_fltr_spec_ptr->filter_desc.esp_security_policy_index,
                           QMI_QOS_FILTER_PARAM_ESP_SECURITY_POLICY,
                           qos_fltr_spec_ptr->filter_desc.param_mask,
                           fltr_buf_ptr->next_prot_hdr.esp.spi,
                           IPFLTR_MASK_ESP_SPI,
                           fltr_buf_ptr->next_prot_hdr.esp.field_mask );

  return DSQMH_SUCCESS;
} /* dsqmhllif_qos_conv_filter_params() */


/*===========================================================================
FUNCTION dsqmhllif_qos_conv_filters

DESCRIPTION
  This function iterates of the specified PS Flow filter list to convert
  paramaters into that used by QMI Message Library.

  PARAMETERS
  flow_ptr          - Pointed to PS flow
  num_filts_ptr     - Pointer to filter list length (output)
  qos_spec_ptr      - Pointer to QMI QOS specification (output)

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
LOCAL int dsqmhllif_qos_conv_filters
(
  const ip_filter_spec_type *ps_fltr_spec_ptr,  /* Pointer to PS QOS specs */
  qmi_qos_filter_req_type   *qmi_fltr_spec_ptr  /* Pointer to QMI QOS specs*/
)
{
  uint8 index;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( ps_fltr_spec_ptr, "DSQMH LLIF null ps spec pointer passed" );
  DSQMH_ASSERT( qmi_fltr_spec_ptr, "DSQMH LLIF null qmi spec pointer passed" );
  DSQMH_TEST( (DSQMH_MAX_QOS_FILTERS>=ps_fltr_spec_ptr->num_filters),
              "num QOS filters exceeded" );

  DSQMH_MSG_LOW( "QMH LLIF qos conv filter list", 0, 0, 0 );

  for( index=0; index < ps_fltr_spec_ptr->num_filters; index++ )
  {
    /*---------------------------------------------------------------------
      Convert the filter parameters for QMI
    ---------------------------------------------------------------------*/
    if( DSQMH_SUCCESS !=
        dsqmhllif_qos_conv_filter_params( &ps_fltr_spec_ptr->list_ptr[index],
                                          &qmi_fltr_spec_ptr[index] ) )
    {
      DSQMH_MSG_LOW( "QMH LLIF filter conv failed:%d", index, 0, 0 );
      return DSQMH_FAILED;
    }
  }
  return DSQMH_SUCCESS;
} /* dsqmhllif_qos_conv_filters() */



/*===========================================================================
FUNCTION DSQMHLLIF_QOS_LOAD_QMI_SPEC

DESCRIPTION
  This function converts the QOS TX/Rx flow and filter specification
  into format used by the QMI Message Library.

PARAMETERS
  qos_spec_tech    - Um IFACE Bearer technology (UMTS|CDMA)

  ps_specs_ptr    - Pointer to PS QOS specifications

  qmi_specs_ptr    - Pointer to QMI QOS specifications [OUTPUT]

DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
LOCAL int dsqmhllif_qos_load_qmi_spec
(
  qmi_qos_technology_type  qos_spec_tech,       /* Bearer tech (UMTS|CDMA) */
  const qos_spec_type     *ps_spec_ptr,         /* Pointer to PS QOS spec  */
  qmi_qos_spec_type       *qmi_spec_ptr         /* Pointer to QMI QOS spec */
)
{
  uint8 flow_index;
  uint8 i;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( ps_spec_ptr, "DSQMH LLIF null ps spec pointer passed" );
  DSQMH_ASSERT( qmi_spec_ptr, "DSQMH LLIF null qmi spec pointer passed" );

  DSQMH_MSG_LOW( "QMH LLIF qos load qmi specs", 0, 0, 0 );

  /* Check for sufficient flow storage */
  DSQMH_TEST( (DSQMH_MAX_QOS_TOT_FLOWS >=
               DSQMH_FLOW_COUNT(TX,ps_spec_ptr->field_mask,ps_spec_ptr->
                                tx.flow_template.num_aux_flows)),
              "num QOS TX flows exceeded" );
  DSQMH_TEST( (DSQMH_MAX_QOS_TOT_FLOWS >=
               DSQMH_FLOW_COUNT(RX,ps_spec_ptr->field_mask,ps_spec_ptr->
                                rx.flow_template.num_aux_flows)),
              "num QOS RX flows exceeded" );

  /*---------------------------------------------------------------------
    Load the TX flow parameters.
    Note the ordering of flows must be: requested, auxillary, minimum
  ---------------------------------------------------------------------*/
  flow_index = 0;
  if( (uint32)QOS_MASK_TX_FLOW & ps_spec_ptr->field_mask )
  {
    dsqmhllif_qos_conv_flow_params_to_qmi( qos_spec_tech,
                                           &ps_spec_ptr->tx.flow_template.req_flow,
                                           &qmi_spec_ptr->tx_flow_req_array[flow_index++],
                                           &qmi_spec_ptr->num_tx_flow_req );

    /*-------------------------------------------------------------------
      Load the TX filter parameters.
    -------------------------------------------------------------------*/
    if( ps_spec_ptr->tx.fltr_template.num_filters )
    {
      if( DSQMH_SUCCESS ==
          dsqmhllif_qos_conv_filters( &ps_spec_ptr->tx.fltr_template,
                                      qmi_spec_ptr->tx_filter_req_array ) )
      {
        qmi_spec_ptr->num_tx_filter_req = ps_spec_ptr->tx.fltr_template.num_filters;
      }
      else
      {
        return DSQMH_FAILED;
      }
    }
  }

  if( (uint32)QOS_MASK_TX_AUXILIARY_FLOWS & ps_spec_ptr->field_mask )
  {
    for( i=0; i< ps_spec_ptr->tx.flow_template.num_aux_flows; i++ )
    {
      dsqmhllif_qos_conv_flow_params_to_qmi( qos_spec_tech,
                                             &ps_spec_ptr->tx.flow_template.aux_flow_list_ptr[i],
                                             &qmi_spec_ptr->tx_flow_req_array[flow_index++],
                                             &qmi_spec_ptr->num_tx_flow_req );
    }
  }

  if( (uint32)QOS_MASK_TX_MIN_FLOW & ps_spec_ptr->field_mask )
  {
    dsqmhllif_qos_conv_flow_params_to_qmi( qos_spec_tech,
                                           &ps_spec_ptr->tx.flow_template.min_req_flow,
                                           &qmi_spec_ptr->tx_flow_req_array[flow_index++],
                                           &qmi_spec_ptr->num_tx_flow_req );
  }

  /*---------------------------------------------------------------------
    Load the RX flow parameters.
    Note the ordering of flows must be: requested, auxillary, minimum
  ---------------------------------------------------------------------*/
  flow_index = 0;
  if( (uint32)QOS_MASK_RX_FLOW & ps_spec_ptr->field_mask )
  {
    dsqmhllif_qos_conv_flow_params_to_qmi( qos_spec_tech,
                                           &ps_spec_ptr->rx.flow_template.req_flow,
                                           &qmi_spec_ptr->rx_flow_req_array[flow_index++],
                                           &qmi_spec_ptr->num_rx_flow_req );

    /*-------------------------------------------------------------------
      Load the RX filter parameters.
    -------------------------------------------------------------------*/
    if( ps_spec_ptr->rx.fltr_template.num_filters )
    {
      if( DSQMH_SUCCESS ==
          dsqmhllif_qos_conv_filters( &ps_spec_ptr->rx.fltr_template,
                                      qmi_spec_ptr->rx_filter_req_array ) )
      {
        qmi_spec_ptr->num_rx_filter_req = ps_spec_ptr->rx.fltr_template.num_filters;
      }
      else
      {
        return DSQMH_FAILED;
      }
    }
  }

  if( (uint32)QOS_MASK_RX_AUXILIARY_FLOWS & ps_spec_ptr->field_mask )
  {
    for( i=0; i< ps_spec_ptr->rx.flow_template.num_aux_flows; i++ )
    {
      dsqmhllif_qos_conv_flow_params_to_qmi( qos_spec_tech,
                                             &ps_spec_ptr->rx.flow_template.aux_flow_list_ptr[i],
                                             &qmi_spec_ptr->rx_flow_req_array[flow_index++],
                                             &qmi_spec_ptr->num_rx_flow_req );
    }
  }

  if( (uint32)QOS_MASK_RX_MIN_FLOW & ps_spec_ptr->field_mask )
  {
    dsqmhllif_qos_conv_flow_params_to_qmi( qos_spec_tech,
                                           &ps_spec_ptr->rx.flow_template.min_req_flow,
                                           &qmi_spec_ptr->rx_flow_req_array[flow_index++],
                                           &qmi_spec_ptr->num_rx_flow_req );
  }

  return DSQMH_SUCCESS;
} /* dsqmhllif_qos_load_qmi_spec() */

/*===========================================================================
FUNCTION DSQMHLLIF_QOS_MODIFY_LOAD_QMI_SPEC

DESCRIPTION
  This function converts the QOS TX/Rx flow and filter specification
  into format used by the QMI Message Library.

PARAMETERS
  qos_spec_tech    - Um IFACE Bearer technology (UMTS|CDMA)

  ps_specs_ptr    - Pointer to PS QOS specifications

  qmi_specs_ptr    - Pointer to QMI QOS specifications [OUTPUT]

  flow_ptr        -  Pointer to flow

DEPENDENCIES
  dsqmhllif_init_qmi_services() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
LOCAL int dsqmhllif_qos_modify_load_qmi_spec
(
  qmi_qos_technology_type  qos_spec_tech,       /* Bearer tech (UMTS|CDMA) */
  const qos_spec_type     *ps_spec_ptr,         /* Pointer to PS QOS spec  */
  qmi_qos_spec_type       *qmi_spec_ptr,        /* Pointer to QMI QOS spec */
  const ps_flow_type      *flow_ptr             /* Pointer to flow         */
)
{
  uint8 flow_index;
  uint8 i;  
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( ps_spec_ptr, "DSQMH LLIF null ps spec pointer passed" );
  DSQMH_ASSERT( qmi_spec_ptr, "DSQMH LLIF null qmi spec pointer passed" );
  DSQMH_ASSERT( flow_ptr, "DSQMH LLIF null flow pointer passed" );

  DSQMH_MSG_LOW( "QMH LLIF qos load qmi specs", 0, 0, 0 );

  /* Check for sufficient flow storage */
  DSQMH_TEST( (DSQMH_MAX_QOS_TOT_FLOWS >=
               DSQMH_FLOW_COUNT(TX,ps_spec_ptr->field_mask,ps_spec_ptr->
                                tx.flow_template.num_aux_flows)),
              "num QOS TX flows exceeded" );
  DSQMH_TEST( (DSQMH_MAX_QOS_TOT_FLOWS >=
               DSQMH_FLOW_COUNT(RX,ps_spec_ptr->field_mask,ps_spec_ptr->
                                rx.flow_template.num_aux_flows)),
              "num QOS RX flows exceeded" );

  /*---------------------------------------------------------------------
    Load the TX flow parameters.
    Note the ordering of flows must be: requested, auxillary, minimum
  ---------------------------------------------------------------------*/
  flow_index = 0;
  if( (uint32)QOS_MASK_TX_FLOW & ps_spec_ptr->field_mask )
  {   
    /*---------------------------------------------------------------------
      Load TX required flow spec parameters.    
    ---------------------------------------------------------------------*/      
    dsqmhllif_qos_conv_flow_params_to_qmi( qos_spec_tech,
                                 &ps_spec_ptr->tx.flow_template.req_flow,
                                 &qmi_spec_ptr->tx_flow_req_array[flow_index++],
                                 &qmi_spec_ptr->num_tx_flow_req );

    /*---------------------------------------------------------------------
      Load TX auxillary flow spec parameters.    
    ---------------------------------------------------------------------*/    
    if( (uint32)QOS_MASK_TX_AUXILIARY_FLOWS & ps_spec_ptr->field_mask )
    {
      for( i = 0; i < ps_spec_ptr->tx.flow_template.num_aux_flows; i++ )
      {
        dsqmhllif_qos_conv_flow_params_to_qmi( qos_spec_tech,
                            &ps_spec_ptr->tx.flow_template.aux_flow_list_ptr[i],
                            &qmi_spec_ptr->tx_flow_req_array[flow_index++],
                            &qmi_spec_ptr->num_tx_flow_req );
      }
    }

    /*---------------------------------------------------------------------
      Load TX minimum required flow spec parameters.    
    ---------------------------------------------------------------------*/
    if( (uint32)QOS_MASK_TX_MIN_FLOW & ps_spec_ptr->field_mask )
    {
      dsqmhllif_qos_conv_flow_params_to_qmi( qos_spec_tech,
                                 &ps_spec_ptr->tx.flow_template.min_req_flow,
                                 &qmi_spec_ptr->tx_flow_req_array[flow_index++],
                                 &qmi_spec_ptr->num_tx_flow_req );
    }
  }

  /*-------------------------------------------------------------------
    Load the TX filter parameters if newly added or modified
  -------------------------------------------------------------------*/
  
  if ( ( ( ps_spec_ptr->field_mask & (uint32) QOS_MASK_TX_FLOW) &&
             !( PS_FLOWI_GET_QOS_FIELD_MASK(flow_ptr) & 
              (uint32) QOS_MASK_RX_FLOW ) ) ||
       ( ps_spec_ptr->field_mask & (uint32) QOS_MODIFY_MASK_RX_FLTR_MODIFY ) )  
  {
    if( ps_spec_ptr->tx.fltr_template.num_filters )
    {
      if( DSQMH_SUCCESS ==
          dsqmhllif_qos_conv_filters( &ps_spec_ptr->tx.fltr_template,
                                      qmi_spec_ptr->tx_filter_req_array ) )
      { 
        qmi_spec_ptr->num_tx_filter_req = 
          ps_spec_ptr->tx.fltr_template.num_filters;
      }
      else
      {
        return DSQMH_FAILED;
      }
    }
  }

  /*---------------------------------------------------------------------
    Load the RX flow parameters.
    Note the ordering of flows must be: requested, auxillary, minimum
  ---------------------------------------------------------------------*/
  flow_index = 0;
  if( (uint32)QOS_MASK_RX_FLOW & ps_spec_ptr->field_mask )
  {
    /*---------------------------------------------------------------------
      Load RX required flow spec parameters.    
    ---------------------------------------------------------------------*/
    dsqmhllif_qos_conv_flow_params_to_qmi( qos_spec_tech,
                                 &ps_spec_ptr->rx.flow_template.req_flow,
                                 &qmi_spec_ptr->rx_flow_req_array[flow_index++],
                                 &qmi_spec_ptr->num_rx_flow_req );

    /*---------------------------------------------------------------------
      Load RX auxillary flow spec parameters.    
    ---------------------------------------------------------------------*/    
    if( (uint32)QOS_MASK_RX_AUXILIARY_FLOWS & ps_spec_ptr->field_mask )
    {
      for( i = 0; i < ps_spec_ptr->rx.flow_template.num_aux_flows; i++ )
      {
        dsqmhllif_qos_conv_flow_params_to_qmi( qos_spec_tech,
                            &ps_spec_ptr->rx.flow_template.aux_flow_list_ptr[i],
                            &qmi_spec_ptr->rx_flow_req_array[flow_index++],
                            &qmi_spec_ptr->num_rx_flow_req );
      }
    }

    /*---------------------------------------------------------------------
      Load RX minimum required flow spec parameters.    
    ---------------------------------------------------------------------*/
    if( (uint32)QOS_MASK_RX_MIN_FLOW & ps_spec_ptr->field_mask )
    {
      dsqmhllif_qos_conv_flow_params_to_qmi( qos_spec_tech,
                                 &ps_spec_ptr->rx.flow_template.min_req_flow,
                                 &qmi_spec_ptr->rx_flow_req_array[flow_index++],
                                 &qmi_spec_ptr->num_rx_flow_req );
    }
  }

  /*-------------------------------------------------------------------
    Load the RX filter parameters if newly added or modified.
  -------------------------------------------------------------------*/
  if ( ( ( ps_spec_ptr->field_mask & (uint32) QOS_MASK_RX_FLOW) &&
             !( PS_FLOWI_GET_QOS_FIELD_MASK(flow_ptr) &
              (uint32) QOS_MASK_RX_FLOW ) ) ||
       ( ps_spec_ptr->field_mask & (uint32) QOS_MODIFY_MASK_RX_FLTR_MODIFY ) )  
  {
    if( ps_spec_ptr->rx.fltr_template.num_filters )
    {
      if( DSQMH_SUCCESS ==
          dsqmhllif_qos_conv_filters( &ps_spec_ptr->rx.fltr_template,
                                      qmi_spec_ptr->rx_filter_req_array ) )
      {
        qmi_spec_ptr->num_rx_filter_req = 
          ps_spec_ptr->rx.fltr_template.num_filters;
      }
      else
      {
        return DSQMH_FAILED;
      }
    }
  }

  return DSQMH_SUCCESS;
} /* dsqmhllif_qos_modify_load_qmi_spec() */

/*===========================================================================

                    EXTERNAL FUNCTION DEFINITIONS

===========================================================================*/



/*===========================================================================
FUNCTION DSQMHLLIF_OPEN_TRANSPORT

DESCRIPTION
  This function opens the datapath transport (SMD port) specified in
  the IFACE control block.  The transport is configured to support
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
)
{
#ifdef FEATURE_DSS_LINUX
  (void )iface_inst;
  (void )phys_link;
  DSQMH_MSG_ERROR("dsqmhllif_open_transport: functionality disabled on Linux",
                  0,0,0);
#else
  sio_open_type          open_params;
  dsqmh_smd_info_type   *smd_info_ptr = NULL;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if( (DSQMH_MAX_PS_IFACES <= iface_inst) ||
      (DSQMH_MAX_PHYSLINKS_PER_IFACE <= phys_link ) )
  {
    DSQMH_MSG_ERROR( "QMH LLIF invalid parameters: iface=%d plink=%d",
                     iface_inst, phys_link, 0 );
    return DSQMH_FAILED;
  }

  smd_info_ptr = DSQMH_GET_SMD_INFO_PTR( iface_inst, phys_link );

  /*---------------------------------------------------------------------
    Map phys link to the SMD port for datapath transport.
  ---------------------------------------------------------------------*/
  if( DSQMH_MAX_PHYSLINKS > iface_inst )
  {
    smd_info_ptr->port_id = dsqmh_config_get_sio_port (iface_inst);
  }
  else
  {
    DSQMH_MSG_ERROR( "QMH LLIF port may exeeeded", 0, 0, 0 );
    return DSQMH_FAILED;
  }

  DSQMH_MSG_LOW( "QMH LLIF open smd port: iface=%d port=%d",
                 iface_inst, smd_info_ptr->port_id, 0 );

  /*-----------------------------------------------------------------------
    Initialize the SIO open params strucuture
  -----------------------------------------------------------------------*/
  memset (&open_params, 0x0, sizeof( open_params ));

  open_params.port_id        = smd_info_ptr->port_id;
  open_params.stream_mode    = SIO_GENERIC_MODE;
  open_params.rx_func_ptr    = NULL;
  open_params.rx_queue       = &smd_info_ptr->ps_rx_wm;
  open_params.tx_queue       = &smd_info_ptr->ps_tx_wm;
  open_params.rx_bitrate     = SIO_BITRATE_BEST;
  open_params.tx_bitrate     = SIO_BITRATE_BEST;
  open_params.tail_char_used = FALSE;
  open_params.rx_flow        = SIO_FCTL_BEST;
  open_params.tx_flow        = SIO_FCTL_BEST;

  /*-------------------------------------------------------------------------
    Open the serial port, and store the assigned stream id. If the open was
    unsuccessful, log an error.
  -------------------------------------------------------------------------*/
  smd_info_ptr->stream_id = sio_open( &open_params );
  if( SIO_NO_STREAM_ID == smd_info_ptr->stream_id )
  {
    DSQMH_MSG_ERROR( "QMH LLIF open smd port failed", 0, 0, 0 );
    return DSQMH_FAILED;
  }

  /*-------------------------------------------------------------------------
    Enable flow on SIO port.
  -------------------------------------------------------------------------*/
  dsqmhllif_set_rx_flow(
    (void*)DSQMH_ENCODE_IFACE_PHYSLINK_ID( iface_inst, phys_link ),
    DS_FLOW_PROXY_MASK,
    FALSE );
#endif /* FEATURE_DSS_LINUX */

  return DSQMH_SUCCESS;
} /* dsqmhllif_open_transport() */


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
  DSQMH_FAILED on error.

SIDE EFFECTS
  SMD task will receive close command

===========================================================================*/
int dsqmhllif_close_transport
(
  uint32        iface_inst,                     /* Index for iface table   */
  uint32        phys_link                       /* Index for phys link list*/
)
{
#ifdef FEATURE_DSS_LINUX
  (void )iface_inst;
  (void )phys_link;
  DSQMH_MSG_ERROR("dsqmhllif_close_transport: functionality disabled on Linux",
                  0,0,0);
  return DSQMH_SUCCESS;
#else
  dsqmh_smd_info_type   *smd_info_ptr = NULL;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_LOW( "QMH LLIF close smd port", 0, 0, 0 );

  smd_info_ptr = DSQMH_GET_SMD_INFO_PTR( iface_inst, phys_link );
  if( NULL != smd_info_ptr)
  {
    /*-----------------------------------------------------------------------
      Close the serial port. The specified callback will be invoked once
      SIO driver has completed its operation.
    -----------------------------------------------------------------------*/
    sio_close( smd_info_ptr->stream_id, dsqmhllif_smd_close_done_cb );
    return DSQMH_SUCCESS;
  }
  else
  {
    DSQMH_MSG_ERROR( "QMH LLIF Invalid SMD info ptr", 0, 0, 0 );
    return DSQMH_FAILED;
  }
#endif /* FEATURE_DSS_LINUX */
  
} /* dsqmhllif_close_transport() */



/*===========================================================================
FUNCTION DSQMHLLIF_SET_TX_FLOW()

DESCRIPTION
  This function sets the flow control state on the SMD port UL stream.
  Whether to enable (FALSE) or disable (TRUE) flow is specified by
  'disable' parameter.

  The reason for the change is specified by 'mask'. Multiple callers
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
  void   *handle,
  uint32  mask,
  boolean disable
)
{
#ifdef FEATURE_DSS_LINUX
  (void )handle;
  (void )mask;
  (void )disable;
  DSQMH_MSG_ERROR("dsqmhllif_set_tx_flow: functionality disabled on Linux",
                  0,0,0);
#else
  dsqmh_smd_info_type   *smd_info_ptr = NULL;
  uint32                 iface_inst;            /* Index for iface table   */
  uint32                 phys_link;             /* Index for phys link list*/
  dsqmh_msg_buf_type    *msg_ptr = NULL;      /* Pointer to message buffer */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_DECODE_IFACE_PHYSLINK_ID( iface_inst, phys_link, ((uint32)handle) );
  DSQMH_ASSERT( (DSQMH_MAX_PS_IFACES > iface_inst), "QMH LLIF invalid iface ID" );
  DSQMH_ASSERT( (DSQMH_MAX_PHYSLINKS_PER_IFACE > phys_link), "QMH LLIF invalid iface ID" );

  smd_info_ptr = DSQMH_GET_SMD_INFO_PTR( iface_inst, phys_link );

  /*-------------------------------------------------------------------------
    Set bits specified by mask in flow state if disable.
    Clear bits specified by mask in flow state if enable (!disable).
    If mask is null, return immediately without change.
  -------------------------------------------------------------------------*/
  if (!mask)
  {
    DSQMH_MSG_ERROR( "QMH LLIF set_tx_flow called with 0 mask",0,0,0);
    return;
  }

  DSQMHLLIF_ENTER_SMD_CRIT_SECTION(); /* Needed for flow mask access */
  if (disable)
  {
    /*-----------------------------------------------------------------------
      If flow was previously enabled, send the command to disable flow
      through the phys link now.  If flow was previously disabled, do nothing
      as we're already flow controlled.
    -----------------------------------------------------------------------*/
    if( !smd_info_ptr->tx_flow_mask )
    {
      /*---------------------------------------------------------------------
        Send asynchronous message for cmd processing in host task context.
      ---------------------------------------------------------------------*/
      DSQMH_GET_MSG_BUF( msg_ptr );
      if( NULL != msg_ptr )
      {
        memset((void*)msg_ptr, 0x0, sizeof(dsqmh_msg_buf_type));

        /*-------------------------------------------------------------------
          Populate message buffer with context info.
        -------------------------------------------------------------------*/
        msg_ptr->msg_id = PROXY_PHYS_LINK_FLOW_DISABLE_CMD;
        msg_ptr->iface_inst = iface_inst;
        msg_ptr->phys_link_ptr = DSQMH_GET_PHYS_LINK_PTR( iface_inst,
                                                          phys_link );

        DSQMH_MSG_MED( "Posting DS cmd: PROXY_PHYS_LINK_FLOW_DISABLE_CMD",0,0,0);
        DSQMH_PUT_MSG_BUF( msg_ptr );
      }
      else
      {
        DSQMH_MSG_FATAL( "Failed to allocate msg buffer, dropping msg",0,0,0 );
      }
    }

    /*-----------------------------------------------------------------------
      Set the passed bitmask from the flow control mask.
    -----------------------------------------------------------------------*/
    smd_info_ptr->tx_flow_mask |= mask;
  }
  else /* enable */
  {
    /*-----------------------------------------------------------------------
      Clear the passed bitmask from the flow control mask.
    -----------------------------------------------------------------------*/
    smd_info_ptr->tx_flow_mask &= ~(mask);

    /*-----------------------------------------------------------------------
      If flow was previously disabled and is now enabled, send the command to
      enable flow through the phys link now.  If flow was previously enabled,
      do nothing as we're already flowing.
    -----------------------------------------------------------------------*/
    if( !smd_info_ptr->tx_flow_mask )
    {
      /*---------------------------------------------------------------------
        Send asynchronous message for cmd processing in host task context.
      ---------------------------------------------------------------------*/
      DSQMH_GET_MSG_BUF( msg_ptr );
      if( NULL != msg_ptr )
      {
        memset((void*)msg_ptr, 0x0, sizeof(dsqmh_msg_buf_type));

        /*-------------------------------------------------------------------
          Populate message buffer with context info.
        -------------------------------------------------------------------*/
        msg_ptr->msg_id = PROXY_PHYS_LINK_FLOW_ENABLE_CMD;
        msg_ptr->iface_inst = iface_inst;
        msg_ptr->phys_link_ptr = DSQMH_GET_PHYS_LINK_PTR( iface_inst,
                                                          phys_link );

        DSQMH_MSG_MED( "Posting DS cmd: PROXY_PHYS_LINK_FLOW_ENABLE_CMD",0,0,0);
        DSQMH_PUT_MSG_BUF( msg_ptr );
      }
      else
      {
        DSQMH_MSG_FATAL( "Failed to allocate msg buffer, dropping msg",0,0,0 );
      }
    }
    else
    {
      DSQMH_MSG_ERROR( "QMH LLIF tx flow ctrl skipped as mask not cleared",0,0,0);
    }
  }
  DSQMHLLIF_EXIT_SMD_CRIT_SECTION();

  DSQMH_MSG_MED( "QMH LLIF tx flow ctrl mask: %c0x%x = fctl mask 0x%x",
                 disable ? '+' : '-', mask, smd_info_ptr->tx_flow_mask);
#endif /* FEATURE_DSS_LINUX */

} /* dsqmhllif_set_tx_flow() */



/*===========================================================================
FUNCTION DSQMHLLIF_SET_RX_FLOW()

DESCRIPTION
  This function sets the flow control state on the SMD port DL stream.
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
void dsqmhllif_set_rx_flow
(
  void   *handle,
  uint32  mask,
  boolean disable
)
{
#ifdef FEATURE_DSS_LINUX
  (void )handle;
  (void )mask;
  (void )disable;
  DSQMH_MSG_ERROR("dsqmhllif_set_rx_flow: functionality disabled on Linux",
                  0,0,0);
#else
  dsqmh_smd_info_type   *smd_info_ptr = NULL;
  sio_ioctl_param_type   ioctl_param = { 0 };
  uint32                 iface_inst;            /* Index for iface table   */
  uint32                 phys_link;             /* Index for phys link list*/

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_DECODE_IFACE_PHYSLINK_ID( iface_inst, phys_link, ((uint32)handle) );
  smd_info_ptr = DSQMH_GET_SMD_INFO_PTR( iface_inst, phys_link );
  if( NULL == smd_info_ptr )
  {
    DSQMH_MSG_ERROR( "QMH LLIF Invalid SMD info ptr",0,0,0);
    return;
  }
  
  /*-------------------------------------------------------------------------
    Set bits specified by mask in flow state if disable.
    Clear bits specified by mask in flow state if enable (!disable).
    If mask is null, return immediately without change.
  -------------------------------------------------------------------------*/
  if (!mask)
  {
    DSQMH_MSG_ERROR( "QMH LLIF set_rx_flow called with 0 mask",0,0,0);
    return;
  }

  DSQMHLLIF_ENTER_SMD_CRIT_SECTION();
  if (disable)
  {
    /*-----------------------------------------------------------------------
      If flow was previously enabled, send the ioctl to disable flow
      through the device now.  If flow was previously disabled, do nothing
      as we're already flow controlled.
    -----------------------------------------------------------------------*/
    if( !smd_info_ptr->rx_flow_mask )
    {
      sio_ioctl( smd_info_ptr->stream_id,
                 SIO_IOCTL_INBOUND_FLOW_DISABLE,
                 &ioctl_param );
    }

    /*-----------------------------------------------------------------------
      Set the passed bitmask from the rmnetsio instance flow control mask
    -----------------------------------------------------------------------*/
    smd_info_ptr->rx_flow_mask |= mask;
  }
  else /* enable */
  {
    /*-----------------------------------------------------------------------
      Clear the passed bitmask from the rmnetsio instance flow control mask
    -----------------------------------------------------------------------*/
    smd_info_ptr->rx_flow_mask &= ~(mask);

    /*-----------------------------------------------------------------------
      If flow was previously disabled and is now enabled, send the ioctl to
      enable flow through the device now.  If flow was previously enabled,
      do nothing as we're already flowing.
    -----------------------------------------------------------------------*/
    if( !smd_info_ptr->rx_flow_mask )
    {
      sio_ioctl( smd_info_ptr->stream_id,
                 SIO_IOCTL_INBOUND_FLOW_ENABLE,
                 &ioctl_param );
    }
    else
    {
      DSQMH_MSG_ERROR( "QMH LLIF rx flow ctrl skipped as mask not cleared",0,0,0);
    }
  }
  DSQMHLLIF_EXIT_SMD_CRIT_SECTION();

  DSQMH_MSG6( MSG_LEGACY_MED,
              "QMH LLIF rx flow ctrl mask: %c0x%x = fctl mask 0x%x "
              "iface=%d wm=%p",
              disable ? '+' : '-', mask,
              smd_info_ptr->rx_flow_mask,
              iface_inst,
              &smd_info_ptr->ps_rx_wm, 0 );
#endif /* FEATURE_DSS_LINUX */

} /* dsqmhllif_set_rx_flow() */




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
)
{
#ifdef FEATURE_DSS_LINUX
  (void )iface_inst;
  (void )phys_link;
  DSQMH_MSG_ERROR("dsqmhllif_init_phys_link_queues: functionality disabled on Linux",
                  0,0,0);
#else

  dsqmh_smd_info_type * smd_info_ptr;
  uint32    ipcode = 0;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_LOW( "QMH LLIF init smd wms", 0, 0, 0 );

  ipcode = DSQMH_ENCODE_IFACE_PHYSLINK_ID( iface_inst, phys_link );
  smd_info_ptr = DSQMH_GET_SMD_INFO_PTR( iface_inst, phys_link );
  if( NULL == smd_info_ptr )
  {
    DSQMH_MSG_ERROR( "QMH LLIF Invalid SMD info ptr",0,0,0);
    return;
  }

  /*-----------------------------------------------------------------------
    Initialize the UL watermark queue.
  -----------------------------------------------------------------------*/
  dsm_queue_init( &smd_info_ptr->ps_tx_wm,
                  DSQMH_QUEUE_UL_WM_DNE,
                  &smd_info_ptr->ps_tx_q );

  /* Set flow control parameters. */
  smd_info_ptr->ps_tx_wm.lo_watermark = DSQMH_QUEUE_UL_WM_LO;
  smd_info_ptr->ps_tx_wm.lowater_func_ptr = dsqmhllif_smd_tx_lowater_cb;
  smd_info_ptr->ps_tx_wm.lowater_func_data = (void*)ipcode;

  smd_info_ptr->ps_tx_wm.hi_watermark = DSQMH_QUEUE_UL_WM_HI;
  smd_info_ptr->ps_tx_wm.hiwater_func_ptr = dsqmhllif_smd_tx_hiwater_cb;
  smd_info_ptr->ps_tx_wm.hiwater_func_data = (void*)ipcode;


  /*-----------------------------------------------------------------------
    Initialize the DL watermark queue.
  -----------------------------------------------------------------------*/
  dsm_queue_init( &smd_info_ptr->ps_rx_wm,
                  DSQMH_QUEUE_DL_WM_DNE,
                  &smd_info_ptr->ps_rx_q );

  /* Signal host task on new Rx packets. Assume processing one pkt at a
   * time, so need to signal task for each new arrival.  */
  smd_info_ptr->ps_rx_wm.each_enqueue_func_ptr = dsqmhllif_smd_rx_pkt_cb;
  smd_info_ptr->ps_rx_wm.each_enqueue_func_data = (void*)ipcode;

  /* Set flow control parameters. */
  smd_info_ptr->ps_rx_wm.lo_watermark = DSQMH_QUEUE_DL_WM_LO;
  smd_info_ptr->ps_rx_wm.lowater_func_ptr = dsqmhllif_smd_rx_lowater_cb;
  smd_info_ptr->ps_rx_wm.lowater_func_data = (void*)ipcode;

  smd_info_ptr->ps_rx_wm.hi_watermark = DSQMH_QUEUE_DL_WM_HI;
  smd_info_ptr->ps_rx_wm.hiwater_func_ptr = dsqmhllif_smd_rx_hiwater_cb;
  smd_info_ptr->ps_rx_wm.hiwater_func_data = (void*)ipcode;
#endif /* FEATURE_DSS_LINUX */

} /* dsqmh_init_phys_link_queues() */

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
)
{
  dsqmh_msglib_info_type  *qmi_ptr = NULL;
  int                      qmi_err_code;
  int                      ret_val;
  uint32                   iface_inst;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-----------------------------------------------------------------------
    Initialize the QMI Message Library.
    qmi_release() must be invoked on process shutdown (if ever required).
  -----------------------------------------------------------------------*/

  DSQMH_MSG_LOW( "QMH LLIF init qmi connections", 0, 0, 0 );

  dsqmh_state_info.qmi_lib_hndl = 
    qmi_init( dsqmhllif_qmi_sys_event_handler, NULL );

  if( dsqmh_state_info.qmi_lib_hndl < 0 )
  {
    DSQMH_MSG_ERROR( "DSQMH LLIF qmi init failed: ret=%d", qmimsglib_handle, 0, 0 );
    return DSQMH_FAILED;
  }

  /* Loop over each Proxy Iface instance */
  for( iface_inst=0; iface_inst < DSQMH_MAX_PS_IFACES; iface_inst++ )
  {
    qmi_ptr = DSQMH_GET_QMI_INFO_PTR( iface_inst );
    qmi_ptr->dev_id = dsqmh_config_get_qmi_device_id (iface_inst);

    /*-----------------------------------------------------------------------
      Open the QMI Message Library on dedicated connection.
    -----------------------------------------------------------------------*/
    ret_val = qmi_connection_init( qmi_ptr->dev_id,
                                   &qmi_err_code );
    if( QMI_NO_ERR != ret_val )
    {
      DSQMH_MSG_ERROR( "DSQMH LLIF qmi conn init failed: iface=%d ret=%d err=0x%p",
                       iface_inst, ret_val, qmi_err_code );
      return DSQMH_FAILED;
    }
  }

  return DSQMH_SUCCESS;
} /* dsqmhllif_init_qmi_connections() */


/*===========================================================================
FUNCTION DSQMHLLIF_DEINIT_QMI_CONNECTIONS

DESCRIPTION
  This function deinitializes the qmi msglib connections. 

PARAMETERS
  None.

DEPENDENCIES
  None.

RETURN VALUE
  None. 

SIDE EFFECTS
  None.

===========================================================================*/
void dsqmhllif_deinit_qmi_connections
(
  void
)
{
  (void) qmi_release(qmimsglib_handle);
}

/*===========================================================================
FUNCTION DSQMHLLIF_INIT_QMI_SERVICES

DESCRIPTION
  This function initializes the connection to the QMI services.  Each
  IFACE has a dedicated QMI connection and registers for both WDS and
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
)
{
  dsqmh_msglib_info_type  *qmi_ptr = NULL;
  int                      qmi_err_code;
  int                      ret_val;
  ps_iface_type           *iface_ptr = NULL;
  qmi_qos_event_report_state_type   qos_report_state;
  qmi_wds_event_report_params_type  wds_report_state;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Use error code to help in diagnostics during powerup.
  -------------------------------------------------------------------------*/
#define DSQMHLLIF_ERRCODE_STEP1 (1<<0)
#define DSQMHLLIF_ERRCODE_STEP2 (1<<1)
#define DSQMHLLIF_ERRCODE_STEP3 (1<<2)
#define DSQMHLLIF_ERRCODE_STEP4 (1<<3)
#define DSQMHLLIF_ERRCODE_STEP5 (1<<4)
#define DSQMHLLIF_ERRCODE_STEP6 (1<<5)
#define DSQMHLLIF_ERRCODE_STEP7 (1<<6)
#define DSQMHLLIF_ASSIGN_ERRCODE( iface, step ) \
  if(err_code_ptr) *err_code_ptr = ((iface<<16) | (step))
  
  DSQMH_MSG_LOW( "QMH LLIF init qmi services", 0, 0, 0 );
  
  
  /* Initialize all QMI services for this iface instance. */
  iface_ptr = DSQMH_GET_IFACE_PTR( iface_inst );
  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( iface_inst );
  qmi_ptr->dev_id = dsqmh_config_get_qmi_device_id (iface_inst);
  DSQMHLLIF_ASSIGN_ERRCODE( iface_inst, 0 );

  /*-------------------------------------------------------------------------
      Registed with QMI Message Library for WDS service.
  -------------------------------------------------------------------------*/
  ret_val = qmi_wds_srvc_init_client( qmi_ptr->dev_id,
                                      dsqmhllif_qmi_wds_ind_cb,
                                      (void*)iface_inst,
                                      &qmi_err_code );
  /* ret_val is handle in this case, zero is error case */
  if( QMI_NO_ERR >= ret_val )
  {
    DSQMH_MSG_ERROR( "DSQMH LLIF qmi wds init failed: iface=%d ret=%d err=0x%p",
                     iface_inst, ret_val, qmi_err_code );
    /*Remove iface from route lookup set on QMI error */
    (DSQMH_GET_ACL_PTR(iface_inst))->disabled = TRUE;
    DSQMHLLIF_ASSIGN_ERRCODE( iface_inst, DSQMHLLIF_ERRCODE_STEP2 );
    return DSQMH_FAILED;
  }

  qmi_ptr->wds_handle = ret_val;
  qmi_ptr->wds_txn_handle = DSQMH_INVALID_TXNID;

  /*-------------------------------------------------------------------------
      Registed with QMI Message Library for QOS service.
  -------------------------------------------------------------------------*/
  ret_val = qmi_qos_srvc_init_client( qmi_ptr->dev_id,
                                      dsqmhllif_qmi_qos_ind_cb,
                                      (void*)iface_inst,
                                      &qmi_err_code );
  /* ret_val is handle in this case, zero is error case */
  if( QMI_NO_ERR >= ret_val )
  {
    DSQMH_MSG_ERROR( "DSQMH LLIF qmi qos init failed: iface=%d ret=%d err=0x%p",
                       iface_inst, ret_val, qmi_err_code );
    /* Remove iface from route lookup set on QMI error */
    (DSQMH_GET_ACL_PTR(iface_inst))->disabled = TRUE;
    DSQMHLLIF_ASSIGN_ERRCODE( iface_inst, DSQMHLLIF_ERRCODE_STEP3 );
    return DSQMH_FAILED;
  }
  qmi_ptr->qos_handle = ret_val;


  /*-------------------------------------------------------------------------
      Set datapath frame format to always use QOS header.
  -------------------------------------------------------------------------*/
  if( DSQMH_SUCCESS !=
        dsqmhllif_qos_enable_qmi_hdr( iface_inst,
                                      PS_IFACEI_GET_DEFAULT_FLOW(iface_ptr) ))
  {
    DSQMH_MSG_MED( "DSQMH HDLR failed on enable_qmi_hdr: iface=%d",
                   iface_inst,0,0 );
    /* Remove iface from route lookup set on QMI error */
    (DSQMH_GET_ACL_PTR(iface_inst))->disabled = TRUE;
    DSQMHLLIF_ASSIGN_ERRCODE( iface_inst, DSQMHLLIF_ERRCODE_STEP4 );
    return DSQMH_FAILED;
  }
    
  /*-------------------------------------------------------------------------
      Set WDS service to report global events.
  -------------------------------------------------------------------------*/
#define QMI_WDS_STATE_REPORT_MASK                              \
        ( QMI_WDS_EVENT_BEARER_TECH_IND |                      \
          QMI_WDS_EVENT_DORM_STATUS_IND )

  wds_report_state.param_mask = QMI_WDS_STATE_REPORT_MASK;
  wds_report_state.report_bearer_tech = TRUE;
  wds_report_state.report_dorm_status = TRUE;
  wds_report_state.report_data_capabilities = TRUE;

  ret_val =
      qmi_wds_set_event_report( qmi_ptr->wds_handle,
                                &wds_report_state,
                                &qmi_err_code );
  if( QMI_NO_ERR != ret_val )
  {
    DSQMH_MSG_ERROR( "QMH LLIF wds set event report failed: iface=%d err=0x%p",
                       iface_inst, qmi_err_code, 0 );
      /* Remove iface from route lookup set on QMI error */
      (DSQMH_GET_ACL_PTR(iface_inst))->disabled = TRUE;
    DSQMHLLIF_ASSIGN_ERRCODE( iface_inst, DSQMHLLIF_ERRCODE_STEP5 );
    return DSQMH_FAILED;
  }

  /*-------------------------------------------------------------------------
      Set WDS service to report IFACE events.
  -------------------------------------------------------------------------*/
#define QMI_WDS_EVENT_REQ_MASK                                         \
        ( QMI_WDS_IFACE_EVENT_REG_OUTAGE_NOTIFICATION |                \
          QMI_WDS_IFACE_EVENT_REG_EXTENDED_IP_CONFIG |                 \
          QMI_WDS_IFACE_EVENT_REG_HDR_REV0_RATE_INERTIA_SUCCESS |      \
          QMI_WDS_IFACE_EVENT_REG_HDR_REV0_RATE_INERTIA_FAILURE |      \
          QMI_WDS_IFACE_EVENT_REG_HDR_SLOTTED_MODE_SUCCESS |           \
          QMI_WDS_IFACE_EVENT_REG_HDR_SLOTTED_MODE_FAILURE   |         \
          QMI_WDS_IFACE_EVENT_REG_HDR_SLOTTED_SESSION_CHANGED   |      \
          QMI_WDS_IFACE_EVENT_REG_RF_CONDITIONS   |                    \
          QMI_WDS_IFACE_EVENT_REG_DOS_ACK_EVENT  )

  ret_val =
      qmi_wds_internal_iface_event_reg_req( qmi_ptr->wds_handle,
                                            QMI_WDS_EVENT_REQ_MASK,
                                            &qmi_err_code );
  if( QMI_NO_ERR != ret_val )
  {
    DSQMH_MSG_ERROR( "QMH LLIF wds set iface reporting failed: iface=%d err=0x%p",
                       iface_inst, qmi_err_code, 0 );
      /* Remove iface from route lookup set on QMI error */
      (DSQMH_GET_ACL_PTR(iface_inst))->disabled = TRUE;
    DSQMHLLIF_ASSIGN_ERRCODE( iface_inst, DSQMHLLIF_ERRCODE_STEP6 );
    return DSQMH_FAILED;
  }

  /*-------------------------------------------------------------------------
      Set QOS service to report events for flow control.
  -------------------------------------------------------------------------*/
#define QMI_QOS_EVENT_REQ_MASK                                         \
        ( QMI_QOS_GLOBAL_FLOW_REPORTING_STATE_PARAM |                  \
          QMI_QOS_NW_SUPPORTED_PROFILE_CHANGE_STATE_PARAM ) 

  qos_report_state.param_mask = QMI_QOS_EVENT_REQ_MASK;
  qos_report_state.flow_state = QMI_QOS_EVENT_REPORTING_ON;

  qos_report_state.profile_change_state.profile_change_status = 
                QMI_QOS_PROFILE_CHANGE_REPORTING_ENABLE;
  qos_report_state.profile_change_state.iface_type =
    QMI_QOS_CDMA_SN_IFACE;
  ret_val = qmi_qos_set_event_report_state( qmi_ptr->qos_handle,
                                              &qos_report_state,
                                              &qmi_err_code );
  if( QMI_NO_ERR != ret_val )
  {
    DSQMH_MSG_ERROR( "QMH LLIF qos set event reporting failed: iface=%d err=0x%p",
                       iface_inst, qmi_err_code, 0 );
      /* Remove iface from route lookup set on QMI error */
      (DSQMH_GET_ACL_PTR(iface_inst))->disabled = TRUE;
    DSQMHLLIF_ASSIGN_ERRCODE( iface_inst, DSQMHLLIF_ERRCODE_STEP7 );
    return DSQMH_FAILED;
  }

  return DSQMH_SUCCESS;

} /* dsqmhllif_init_qmi_services() */




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
)
{
  int result = DSQMH_SUCCESS;
  dsqmh_msglib_info_type  *qmi_ptr = NULL;
  dsqmh_iface_cblk_type   *cblk_ptr = NULL;
  acl_policy_info_type    *policy_ptr = NULL;
  qmi_wds_start_nw_if_params_type  req_params;
  qmi_wds_ip_family_pref_type        ip_family_pref;
  qmi_wds_call_end_reason_type   end_reason;
  int                      qmi_err_code;
  int ret_val = 0;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_MED( "QMH LLIF qmi start network: %d", iface_inst, 0, 0 );

  cblk_ptr = DSQMH_GET_CBLK_PTR( iface_inst );
  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( iface_inst );
  policy_ptr = DSQMH_GET_POLICY_INFO_PTR( iface_inst );
  if( !qmi_ptr || !policy_ptr )
  {
    DSQMH_MSG_ERROR( "QMH LLIF iface inst invalid: %d",iface_inst,0,0 );
    return DSQMH_FAILED;
  }

  /* Setup QMI message */
  memset( (void*)&req_params, 0x0, sizeof(req_params) );

  /*-----------------------------------------------------------------------
    Convert the DSS IFACE Name to a technology preference.  The QMI
    layer may support a subset of all technologies, so we filter
    routing requests based on what is supproted.  Unsupported
    technologies will yeild a NULL routing result.
  -----------------------------------------------------------------------*/
  DSQMH_SET_TECH_PREF( policy_ptr->iface,
                       req_params.tech_pref, req_params.xtended_tech_pref );
  if( DSQMH_INVALID_TECHPREF == req_params.xtended_tech_pref )
  {
    DSQMH_MSG_MED( "QMH LLIF qmi start network for unsupported tech pref: %d %p",
                   policy_ptr->iface.kind, policy_ptr->iface.info.name, 0 );
    return DSQMH_FAILED;
  }
  req_params.params_mask |= QMI_WDS_START_NW_XTENDED_TECH_PREF_PARAM;

  DSQMH_SET_IPADDR_FAMILY_PREF( policy_ptr->ip_family,
                                req_params.ip_family_pref );

  DSQMH_SET_PROFILE_INDEX( policy_ptr,
                           req_params.profile_index,
                           req_params.profile_index_3gpp2,
                           req_params.tech_pref,
                           req_params.params_mask );

  if( DSQMH_INVALID_IFACEID != policy_ptr->rt_result.if_hndl )
  {
    req_params.iface_handle = policy_ptr->rt_result.if_hndl;
    req_params.params_mask |= QMI_WDS_START_NW_IFACE_HNDL_PARAM;
  }

  /*-----------------------------------------------------------------------
    This step is required for supporting DUAL IP calls. QMI WDS Service
    expects the IP family preference to be set in advance if Dual-IP 
    support is required. Since QMH is unsure if this particular call is
    followed by another call for dual-ip, just perform this step always.
  -----------------------------------------------------------------------*/
  DSQMH_SET_IPADDR_FAMILY_PREF( policy_ptr->ip_family, ip_family_pref );

  ret_val = qmi_wds_set_client_ip_pref( qmi_ptr->wds_handle,
                                        ip_family_pref,
                                        &qmi_err_code );
                                                
  if( 0 > ret_val )
  {
    /* Does not matter if there is an error */
    DSQMH_MSG_HIGH ( "QMH LLIF set client family preference: "
                     "iface=%d err=0x%p ip family %d",
                     iface_inst, qmi_err_code, ip_family_pref);
  }


  /*-----------------------------------------------------------------------
    Command QMI to start the network interface.  If successful, the
    command processing is performed asynchronously and the result is
    posted via the specified commmand callback. Note the final outcome
    of the operation is posted via later asynchronous indication.  If
    error on command submission, there will be no asynchronous responses.
  -----------------------------------------------------------------------*/
  ret_val = qmi_wds_start_nw_if( qmi_ptr->wds_handle,
                                 &req_params,
                                 dsqmhllif_qmi_wds_cmd_cb,
                                 (void*)iface_inst,
                                 &end_reason,
                                 &qmi_err_code );
  if( 0 > ret_val )
  {
#ifdef FEATURE_DSS_LINUX
    /* Check for case where another process has already brought up the
     * Modem Um interface.  In this case, QMI returns
     * QMI_ERR_NO_EFFECT but otherwise the command succeeded. */
    if( QMI_SERVICE_ERR_NO_EFFECT != qmi_err_code )
#endif
    {  
      /* Set the down reason from the QMI call end reason */
      dsqmhllif_decode_call_end( end_reason, &cblk_ptr->down_reason );

      DSQMH_MSG_ERROR( "QMH LLIF qmi start network failed: "
                       "iface=%d err=0x%p reason=%d",
                       iface_inst, qmi_err_code, 
                       cblk_ptr->down_reason );
      result = DSQMH_FAILED;
    }
  }
  else
  {
    /* Save txn handle for possible abort. */
    qmi_ptr->wds_txn_handle = ret_val;
  }

  return result;
} /* dsqmhllif_start_network_cmd() */


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
)
{
  int result = DSQMH_SUCCESS;
  dsqmh_msglib_info_type  *qmi_ptr = NULL;
  int                      qmi_err_code;
  int ret_val = 0;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_MED( "QMH LLIF qmi stop network: %d", iface_inst, 0, 0 );

  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( iface_inst );
  if( !qmi_ptr )
  {
    DSQMH_MSG_ERROR( "QMH LLIF iface inst invalid: %d",iface_inst,0,0 );
    return DSQMH_FAILED;
  }

  /*-----------------------------------------------------------------------
    Command QMI to stop the network interface.  If successful, the
    command processing is performed asynchronously and the result is
    posted via the specified commmand callback. Note the final outcome
    of the operation is posted via later asynchronous indication.  If
    error on command submission, there will be no asynchronous responses.
  -----------------------------------------------------------------------*/
  ret_val = qmi_wds_stop_nw_if( qmi_ptr->wds_handle,
                                dsqmhllif_qmi_wds_cmd_cb,
                                (void*)iface_inst,
                                &qmi_err_code );
  if( 0 > ret_val )
  {
    DSQMH_MSG_ERROR( "QMH LLIF qmi stop network failed: iface=%d err=%p",
                     iface_inst, qmi_err_code, 0 );
    result = DSQMH_FAILED;
  }

  return result;
} /* dsqmhllif_stop_network_cmd() */


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
)
{
  int result = DSQMH_SUCCESS;
  dsqmh_msglib_info_type  *qmi_ptr = NULL;
  int                      qmi_err_code;
  int ret_val = 0;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_MED( "QMH LLIF qmi abort network: %d", iface_inst, 0, 0 );

  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( iface_inst );
  if( !qmi_ptr )
  {
    DSQMH_MSG_ERROR( "QMH LLIF iface inst invalid: %d",iface_inst,0,0 );
    return DSQMH_FAILED;
  }

  if( DSQMH_INVALID_TXNID == qmi_ptr->wds_txn_handle )
  {
    DSQMH_MSG_ERROR( "QMH LLIF no pending qmi transaction handle",0,0,0);
    return DSQMH_FAILED;
  }

  /*-----------------------------------------------------------------------
    Command QMI to abort current operation.
  -----------------------------------------------------------------------*/
  ret_val = qmi_wds_abort( qmi_ptr->wds_handle,
                           qmi_ptr->wds_txn_handle,
                           dsqmhllif_qmi_wds_cmd_cb,
                           (void*)iface_inst,
                           &qmi_err_code );
  if( 0 > ret_val )
  {
    DSQMH_MSG_ERROR( "QMH LLIF qmi abort failed: iface=%d err=%p",
                     iface_inst, qmi_err_code, 0 );
    result = DSQMH_FAILED;
  }

  return result;
} /* dsqmhllif_abort_cmd() */


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
  num_flows      - PS flow list length
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
  uint32         iface_inst,               /* Index for iface table        */
  ps_iface_ioctl_qos_request_ex_opcode_enum_type
                 opcode,                   /* Configure/request            */
  qos_spec_type *qos_specs_ptr,            /* Pointer to PS QOS specs list */
  uint8          num_qos_specs,            /* QOS specs list length        */
  ps_flow_type **flows_pptr,               /* Pointer to PS flow list      */
  int16         *ps_errno
)
{
  int result = DSQMH_SUCCESS;
  dsqmh_iface_cblk_type  *cblk_ptr = NULL;
  dsqmh_msglib_info_type *qmi_ptr = NULL;
  dsqmh_qmi_qos_spec_s_type *qos_params_mem_ptr = NULL;
  dsqmh_qmi_qos_spec_s_type *qos_params_ptr = NULL;
  qmi_qos_spec_type   qmi_specs[DSQMH_MAX_QOS_SPECS];
  unsigned long           qos_identifiers[DSQMH_MAX_QOS_SPECS];
  qmi_qos_err_rsp_type    qos_spec_errs[DSQMH_MAX_QOS_SPECS];
  qmi_qos_technology_type       qos_spec_tech;
  int                     qmi_err_code = 0;
  int                     ret_val = 0;
  uint8                   index;
  uint32                  mem_siz;
  qmi_qos_req_opcode_type qmi_qos_opcode;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( flows_pptr, "DSQMH LLIF null flows pointer passed" );
  DSQMH_TEST( (DSQMH_MAX_QOS_SPECS>=num_qos_specs), "num QOS specs exceeded" );

  DSQMH_MSG_LOW( "QMH LLIF qmi qos request", 0, 0, 0 );

  cblk_ptr = DSQMH_GET_CBLK_PTR( iface_inst );
  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( iface_inst );
  if( !qmi_ptr )
  {
    DSQMH_MSG_ERROR( "QMH LLIF iface inst invalid: %d",iface_inst,0,0 );
    return DSQMH_FAILED;
  }
  *ps_errno = 0;

  /*---------------------------------------------------------------------
    Allocate storage for the QOS request structure.  If this fails,
    return error to client.  This may be a temporary failure if
    dynamic memory frees up.
  ---------------------------------------------------------------------*/
  mem_siz = sizeof(dsqmh_qmi_qos_spec_s_type) * num_qos_specs;
  qos_params_mem_ptr = (dsqmh_qmi_qos_spec_s_type*)
    ps_system_heap_mem_alloc( mem_siz );
  if( NULL == qos_params_mem_ptr )
  {
    DSQMH_MSG_ERROR( "QMH LLIF memory allocation failure: %d",
                     mem_siz, 0, 0 );
    *ps_errno = DS_ENOMEM;
    return DSQMH_FAILED;
  }

  /*---------------------------------------------------------------------
    Initialize the QOS flow and filter request structures.
  ---------------------------------------------------------------------*/
  memset( (void*)qos_identifiers, 0x0, sizeof(qos_identifiers) );
  memset( (void*)qmi_specs, 0x0, sizeof(qmi_specs) );
  memset( (void*)qos_spec_errs, 0x0, sizeof(qos_spec_errs) );

  qos_spec_tech = (qmi_qos_technology_type)cblk_ptr->um_bearer_tech.current_network;

  /*---------------------------------------------------------------------
    Process each QOS spec passed by caller.
  ---------------------------------------------------------------------*/
  for( index=0; index<num_qos_specs; index++ )
  {
    qos_params_ptr = (qos_params_mem_ptr + index);
    memset( (void*)qos_params_ptr, 0x0, sizeof(dsqmh_qmi_qos_spec_s_type) );

    /* Configure QOS spec storage */
    qmi_specs[index].tx_flow_req_array = qos_params_ptr->tx_flow_reqs;
    qmi_specs[index].rx_flow_req_array = qos_params_ptr->rx_flow_reqs;
    qmi_specs[index].tx_filter_req_array = qos_params_ptr->tx_fltr_reqs;
    qmi_specs[index].rx_filter_req_array = qos_params_ptr->rx_fltr_reqs;

    /* Convert paramaters to QMI Msg Lib format */
    if( DSQMH_SUCCESS != dsqmhllif_qos_load_qmi_spec( qos_spec_tech,
                                                      &qos_specs_ptr[index],
                                                      &qmi_specs[index] ) )
    {
      DSQMH_MSG_ERROR( "QMH LLIF qos spec load error",0,0,0 );
      PS_SYSTEM_HEAP_MEM_FREE( qos_params_mem_ptr );
      *ps_errno = DS_EINVAL;
      return DSQMH_FAILED;
    }
  }

  qmi_qos_opcode =  (PS_IFACE_IOCTL_QOS_CONFIGURE_OP == opcode) ? 
                    QMI_QOS_CONFIGURE:
                    QMI_QOS_REQUEST;

  /*-----------------------------------------------------------------------
    Command QMI to request a new QOS flow from network.  The result
    of the operation will be posted via asynchronous notification.
  -----------------------------------------------------------------------*/
  ret_val = qmi_qos_request_qos( qmi_ptr->qos_handle,
                                 num_qos_specs,
                                 qmi_specs,
                                 qmi_qos_opcode,
                                 qos_identifiers,
                                 qos_spec_errs,
                                 &qmi_err_code );

  if( QMI_NO_ERR == ret_val )
  {
    /* Preserve the QOS identifier in the flow's client_data. */
    for( index=0; index<num_qos_specs; index++ )
    {
      flows_pptr[index]->client_data_ptr = (void*)qos_identifiers[index];
      LOG_MSG_INFO2("Mapped flow[%d] to QMI handle: %p -> %p",
              index, flows_pptr[index], qos_identifiers[index]);
    }
  }
  else
  {
    uint8 j;

    DSQMH_MSG_ERROR( "QMH LLIF qos request error: iface=%d ret=%d err=%p",
                     iface_inst, ret_val, qmi_err_code );

    for( j=0; j<num_qos_specs; j++ )
    {
      /* Report the QMI QOS error details if available. */
      if( qos_spec_errs[j].errs_present )
      {
        for( index=0; index<QMI_QOS_MAX_FLOW_FILTER; index++ )
        {
          if( qos_spec_errs[j].tx_flow_req_err_mask[index] ||
              qos_spec_errs[j].rx_flow_req_err_mask[index] )
          {
            DSQMH_MSG_ERROR( "QMH LLIF flow err mask[%d]: TX=%p RX=%p",
                             index,
                             qos_spec_errs[j].tx_flow_req_err_mask[index],
                             qos_spec_errs[j].rx_flow_req_err_mask[index] );
          }
          if( qos_spec_errs[j].tx_filter_req_err_mask[index] ||
              qos_spec_errs[j].rx_filter_req_err_mask[index] )
          {
            DSQMH_MSG_ERROR( "QMH LLIF filter err mask[%d]: TX=%p RX=%p",
                             index,
                             qos_spec_errs[j].tx_filter_req_err_mask[index],
                             qos_spec_errs[j].rx_filter_req_err_mask[index] );
          }
        }
      }
    }
    *ps_errno = DS_EINVAL;
    result = DSQMH_FAILED;
  }

  /* Release dynamic storage */
  PS_SYSTEM_HEAP_MEM_FREE( qos_params_mem_ptr );

  return result;
} /* dsqmhllif_qos_request_cmd() */


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
  ps_flow_type       *flow_ptr,                 /* Pointer to PS flow      */
  ps_flow_ioctl_type  ioctl_name,
  void               *argval_ptr,
  sint15             *ps_errno
)
{
  int result = DSQMH_SUCCESS;
  ps_iface_type             *iface_ptr = NULL;
  dsqmh_iface_cblk_type     *cblk_ptr = NULL;   /* Pointer to control block  */
  dsqmh_msglib_info_type    *qmi_ptr = NULL;
  int                        qmi_err_code;
  qmi_qos_granted_info_rsp_type  qmi_granted_info;
  qmi_qos_technology_type        qos_spec_tech;
  ps_flow_ioctl_primary_qos_get_granted_flow_spec_type *info_ptr =
    (ps_flow_ioctl_primary_qos_get_granted_flow_spec_type*)argval_ptr;
  uint32                     num_flows = 0;
  int                        ret_val = 0;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( flow_ptr, "DSQMH LLIF null flow pointer passed" );
  DSQMH_ASSERT( info_ptr, "DSQMH LLIF null QOS info pointer passed" );

  DSQMH_MSG_LOW( "QMH LLIF qmi qos get granted request", 0, 0, 0 );

  DSQMH_GET_IFACE_PTR_FROM_FLOW( flow_ptr, iface_ptr );
  if( !iface_ptr )
  {
    DSQMH_MSG_ERROR( "QMH LLIF flow iface invalid: %p",
                     flow_ptr,0,0 );
    return DSQMH_FAILED;
  }
  cblk_ptr = DSQMH_GET_CBLK_PTR( (uint32)iface_ptr->client_data_ptr );
  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( (uint32)iface_ptr->client_data_ptr );
  if( !qmi_ptr )
  {
    DSQMH_MSG_ERROR( "QMH LLIF iface inst invalid: %d",
                     (uint32)iface_ptr->client_data_ptr,0,0 );
    return DSQMH_FAILED;
  }

  /*-----------------------------------------------------------------------
    Command QMI to request granted QOS specification.
  -----------------------------------------------------------------------*/
  if( PS_FLOW_IOCTL_PRIMARY_QOS_GET_GRANTED_FLOW_SPEC == ioctl_name )
  {
    ret_val =
      qmi_qos_get_primary_granted_qos_info( qmi_ptr->qos_handle,
                                            (unsigned long)flow_ptr->client_data_ptr,
                                            &qmi_granted_info,
                                            &qmi_err_code );
  }
  else
  {
  ret_val =
      qmi_qos_get_secondary_granted_qos_info( qmi_ptr->qos_handle,
                                              (unsigned long)flow_ptr->client_data_ptr,
                                  &qmi_granted_info,
                                  &qmi_err_code );
  }


  if( QMI_NO_ERR == ret_val )
  {
    qos_spec_tech = (qmi_qos_technology_type)cblk_ptr->um_bearer_tech.current_network;

    if( qmi_granted_info.tx_granted_flow_data_is_valid )
    {
      dsqmhllif_qos_conv_flow_params_to_ps( qos_spec_tech,
                                            &qmi_granted_info.tx_granted_flow_data.qos_flow_granted,
                                            &info_ptr->tx_ip_flow,
                                            &num_flows );
    }
    if( qmi_granted_info.rx_granted_flow_data_is_valid )
    {
      dsqmhllif_qos_conv_flow_params_to_ps( qos_spec_tech,
                                            &qmi_granted_info.rx_granted_flow_data.qos_flow_granted,
                                            &info_ptr->rx_ip_flow,
                                            &num_flows );
    }
  }
  else
  {
    DSQMH_MSG_ERROR( "QMH LLIF get granted qos error: iface=%d flow=%p err=%p",
                     (uint32)iface_ptr->client_data_ptr, flow_ptr, qmi_err_code );
    *ps_errno = DS_EFAULT;
    result = DSQMH_FAILED;
  }
  return result;
} /* dsqmhllif_qos_get_granted_cmd() */


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
)
{
  dsqmh_msglib_info_type    *qmi_ptr = NULL;
  int                        qmi_err_code = 0;
  int                        result = DSQMH_SUCCESS;
  int                        ret_val = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( ps_errno, "DSQMH LLIF null ps_errno ptr passed" );

  DSQMH_MSG_LOW( "QMH LLIF Refresh DHCP config info handler, iface %d",
                 iface_inst, 0, 0 );
  *ps_errno = DS_ENOERR;

  if( TRUE != DSQMH_IS_IFACE_INST_VALID( iface_inst ))
  {
    DSQMH_MSG_ERROR( "QMH LLIF invalid iface inst %d", iface_inst, 0, 0 );
    *ps_errno = DS_EINVAL;
    return DSQMH_FAILED;
  }

  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( iface_inst );

  ret_val = qmi_wds_refresh_dhcp_config_info( qmi_ptr->wds_handle,
                                              &qmi_err_code );

  if( 0 > ret_val )
  {
    DSQMH_MSG_ERROR( "QMH LLIF refresh DHCP failed, iface=%d err=%x",
                     iface_inst, qmi_err_code, 0 );
    result = DSQMH_FAILED;
  }

  return result;

} /* dsqmhllif_refresh_dhcp_config_info() */


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
)
{
  int                        result = DSQMH_SUCCESS;
  dsqmh_msglib_info_type    *qmi_ptr = NULL;
  int                        qmi_err_code;
  int                        ret_val = 0;
  qmi_qos_iface_name_type    qos_iface_name;
  qmi_qos_nw_supported_qos_profiles_rsp_type nw_sup_rsp;
  ps_iface_ioctl_get_network_supported_qos_profiles_type *info_ptr =
    (ps_iface_ioctl_get_network_supported_qos_profiles_type*)argval_ptr;
  uint32                     i,j=0;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( info_ptr, "DSQMH LLIF null QOS info pointer passed" );
  DSQMH_ASSERT( ps_errno, "DSQMH LLIF null ps_error pointer passed" );

  DSQMH_MSG_LOW( "QMH LLIF qmi get net qos profiles", 0, 0, 0 );

  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( iface_inst );
  if( !qmi_ptr )
  {
    DSQMH_MSG_ERROR( "QMH LLIF iface inst invalid: %d",iface_inst,0,0 );
    return DSQMH_FAILED;
  }

  /* Select QOS network technology */
  if( (uint32)PS_IFACE_NETWORK_CDMA == net_tech )
  {
    qos_iface_name = QMI_QOS_CDMA_SN_IFACE;
  }
  else
  {
    DSQMH_MSG_LOW( "QMH LLIF unsupported net tech: %d", net_tech, 0, 0 );
    *ps_errno = DS_EOPNOTSUPP;
    return DSQMH_FAILED;
  }

  /*-----------------------------------------------------------------------
    Command QMI to request the network supported QOS profiles.
  -----------------------------------------------------------------------*/
  ret_val =
    qmi_qos_get_nw_supported_qos_profiles( qmi_ptr->qos_handle,
                                           qos_iface_name,
                                           &nw_sup_rsp,
                                           &qmi_err_code );

  if( QMI_NO_ERR == ret_val )
  {
    info_ptr->profile_count = 0;

    /* Loop over all technologies */
    for( i=0; i<nw_sup_rsp.qos_profiles.num_instances; i++ )
    {
      /* Check for requested mask.  NOTE: PS library has CDMA valiue 0 */
      if( qos_iface_name ==
          nw_sup_rsp.qos_profiles.profile_info[i].iface_type )
      {
        /* Loop over all profiles for one technology */
        for( j=0; (j < nw_sup_rsp.qos_profiles.profile_info[i].num_profiles) &&
                  (MAX_NETWORK_SUPPORTED_PROFILE_COUNT > info_ptr->profile_count);
             j++ )
        {
          info_ptr->profile_value[ info_ptr->profile_count++ ] =
            nw_sup_rsp.qos_profiles.profile_info[i].profile[j];
        }
      }
    }
  }
  else
  {
    DSQMH_MSG_ERROR( "QMH LLIF get net qos profiles: iface=%d err=%p",
                     iface_inst, qmi_err_code, 0 );
    DSQMH_SET_PS_ERRNO( nw_sup_rsp.dss_err, (*ps_errno) );
    result = DSQMH_FAILED;
  }

  return result;
} /* dsqmhllif_get_net_qos_profiles() */



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
)
{
  int result = DSQMH_SUCCESS;
  ps_iface_type           *iface_ptr = NULL;
  dsqmh_msglib_info_type  *qmi_ptr = NULL;
  int                      qmi_err_code;
  int ret_val = 0;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( req_ptr, "DSQMH LLIF null request pointer passed" );
  DSQMH_ASSERT( resp_ptr, "DSQMH LLIF null request pointer passed" );

  DSQMH_MSG_LOW( "QMH LLIF qmi qos flow operation", 0, 0, 0 );

  DSQMH_GET_IFACE_PTR_FROM_FLOW( flow_ptr, iface_ptr );
  if( !iface_ptr )
  {
    DSQMH_MSG_ERROR( "QMH LLIF flow iface invalid: %p",
                     flow_ptr,0,0 );
    return DSQMH_FAILED;
  }
  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( (uint32)iface_ptr->client_data_ptr );
  if( !qmi_ptr )
  {
    DSQMH_MSG_ERROR( "QMH LLIF flow iface inst invalid: %d",
                     (uint32)iface_ptr->client_data_ptr,0,0 );
    return DSQMH_FAILED;
  }

  /* Command QMI to perform flow operation. */
  ret_val = qmi_qos_perform_flow_operation( qmi_ptr->qos_handle,
                                            req_ptr,
                                            resp_ptr,
                                            &qmi_err_code );

  if( QMI_NO_ERR != ret_val )
  {
    DSQMH_MSG_ERROR( "QMH LLIF qos flow operation error: flow=%p err=%p",
                     (uint32)flow_ptr->client_data_ptr, qmi_err_code, 0 );
    result = DSQMH_FAILED;
  }

  return result;
} /* dsqmhllif_qos_flow_operation() */


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
)
{
  int result = DSQMH_SUCCESS;
  dsqmh_iface_cblk_type   *cblk_ptr = NULL;
  dsqmh_msglib_info_type  *qmi_ptr = NULL;
  int                      qmi_err_code = 0;
  int ret_val = 0;
  uint8 i;
  unsigned long qmi_flow_ids[DSQMH_MAX_QOS_FLOWS];
  ps_flow_ioctl_qos_modify_type *qos_modify_ptr = NULL;
  ps_flow_ioctl_primary_qos_modify_type* qos_pmodify_ptr = NULL;
  qos_spec_type            ps_qos_spec;
  qmi_qos_spec_type        qos_spec;
  qmi_qos_err_rsp_type     qos_spec_err;
  dsqmh_qmi_qos_spec_s_type *qos_params_mem_ptr = NULL;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( flows_pptr, "DSQMH LLIF null flows pointer passed" );
  DSQMH_ASSERT( (DSQMH_MAX_QOS_FLOWS>=num_flows), "QOS flow num exceeded" );

  DSQMH_MSG_LOW( "QMH LLIF qmi qos release", 0, 0, 0 );

  cblk_ptr = DSQMH_GET_CBLK_PTR( iface_inst );
  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( iface_inst );
  if( !qmi_ptr )
  {
    DSQMH_MSG_ERROR( "QMH LLIF iface inst invalid: %d",iface_inst,0,0 );
    return DSQMH_FAILED;
  }

  /*-----------------------------------------------------------------------
    Get list of QMI flow IDs, stored in each PS flow's client data.
  -----------------------------------------------------------------------*/
  for( i=0; i<num_flows; i++ )
  {
    qmi_flow_ids[i] = (unsigned long)flows_pptr[i]->client_data_ptr;
  }

  switch( operation )
  {
    case DSQMHLLIF_QOS_OP_RELEASE:
      /* Command QMI to release an existing QOS flow from network. */
      ret_val = qmi_qos_release_qos( qmi_ptr->qos_handle,
                                     num_flows,
                                     qmi_flow_ids,
                                     &qmi_err_code );
      break;

    case DSQMHLLIF_QOS_OP_SUSPEND:
      /* Command QMI to suspend an existing QOS flow from network. */
      ret_val = qmi_qos_suspend_qos( qmi_ptr->qos_handle,
                                             (unsigned char)num_flows,
                                             qmi_flow_ids,
                                             &qmi_err_code );
      break;

    case DSQMHLLIF_QOS_OP_RESUME:
      /* Command QMI to resume an existing QOS flow from network. */
      ret_val = qmi_qos_resume_qos( qmi_ptr->qos_handle,
                                            (unsigned char)num_flows,
                                            qmi_flow_ids,
                                            &qmi_err_code );
      break;

    case DSQMHLLIF_QOS_OP_MODIFY:
      qos_modify_ptr = (ps_flow_ioctl_qos_modify_type*)argval_ptr;

      /*---------------------------------------------------------------------
        Allocate storage for the QOS request structure.  If this fails,
        return error to client.  This may be a temporary failure if
        dynamic memory frees up.
      ---------------------------------------------------------------------*/
      qos_params_mem_ptr = (dsqmh_qmi_qos_spec_s_type*)
        ps_system_heap_mem_alloc( sizeof(dsqmh_qmi_qos_spec_s_type) );
      if( NULL == qos_params_mem_ptr )
      {
        DSQMH_MSG_ERROR( "QMH LLIF memory allocation failure: %d",
                         sizeof(dsqmh_qmi_qos_spec_s_type), 0, 0 );
        *ps_errno = DS_ENOMEM;
        return DSQMH_FAILED;
      }
      /* Initialize the QOS flow and filter request structures. */
      memset( (void*)&qos_spec, 0x0, sizeof(qos_spec) );
      memset( (void*)qos_params_mem_ptr, 0x0, sizeof(dsqmh_qmi_qos_spec_s_type) );
      qos_spec.tx_flow_req_array = qos_params_mem_ptr->tx_flow_reqs;
      qos_spec.rx_flow_req_array = qos_params_mem_ptr->rx_flow_reqs;
      qos_spec.tx_filter_req_array = qos_params_mem_ptr->tx_fltr_reqs;
      qos_spec.rx_filter_req_array = qos_params_mem_ptr->rx_fltr_reqs;
      qos_spec.qos_identifier = (unsigned long)flows_pptr[0]->client_data_ptr;

      /* Convert QOS spec parameters. */
      if( DSQMH_SUCCESS !=
          dsqmhllif_qos_modify_load_qmi_spec( (qmi_qos_technology_type)cblk_ptr->
                                              um_bearer_tech.current_network,
                                              qos_modify_ptr->qos_ptr,
                                              &qos_spec,
                                              flows_pptr[0] ) )
      {
        DSQMH_MSG_ERROR( "QMH LLIF qos spec load error",0,0,0 );
        *ps_errno = DS_EINVAL;
        result = DSQMH_FAILED;
      }
      else
      {
        /* Command QMI to modify secondary QOS flow. */
        ret_val = qmi_qos_modify_secondary_qos( qmi_ptr->qos_handle,
                                                num_flows,
                                                &qos_spec,
                                                &qos_spec_err,
                                                &qmi_err_code );
      }
      PS_SYSTEM_HEAP_MEM_FREE( qos_params_mem_ptr );
      break;

    case DSQMHLLIF_QOS_OP_MODIFY_PRI:
      qos_pmodify_ptr = (ps_flow_ioctl_primary_qos_modify_type*)argval_ptr;

      /*---------------------------------------------------------------------
        Allocate storage for the QOS request structure.  If this fails,
        return error to client.  This may be a temporary failure if
        dynamic memory frees up.
      ---------------------------------------------------------------------*/
      qos_params_mem_ptr = (dsqmh_qmi_qos_spec_s_type*)
        ps_system_heap_mem_alloc( sizeof(dsqmh_qmi_qos_spec_s_type) );
      if( NULL == qos_params_mem_ptr )
      {
        DSQMH_MSG_ERROR( "QMH LLIF memory allocation failure: %d",
                         sizeof(dsqmh_qmi_qos_spec_s_type), 0, 0 );
        *ps_errno = DS_ENOMEM;
        return DSQMH_FAILED;
      }
      /* Initialize the QOS flow and filter request structures. */
      memset( (void*)&qos_spec, 0x0, sizeof(qos_spec) );
      memset( (void*)qos_params_mem_ptr, 0x0, sizeof(dsqmh_qmi_qos_spec_s_type) );
      memset( (void*)&ps_qos_spec, 0x0, sizeof(ps_qos_spec) );

      qos_spec.tx_flow_req_array = qos_params_mem_ptr->tx_flow_reqs;
      qos_spec.rx_flow_req_array = qos_params_mem_ptr->rx_flow_reqs;
      qos_spec.tx_filter_req_array = qos_params_mem_ptr->tx_fltr_reqs;
      qos_spec.rx_filter_req_array = qos_params_mem_ptr->rx_fltr_reqs;
      qos_spec.qos_identifier = (unsigned long)flows_pptr[0]->client_data_ptr;

      /* Convert QOS spec parameters. */
      ps_qos_spec.field_mask       =
        qos_pmodify_ptr->primary_qos_spec_ptr->field_mask;
      ps_qos_spec.rx.flow_template =
        qos_pmodify_ptr->primary_qos_spec_ptr->rx_flow_template;
      ps_qos_spec.tx.flow_template =
        qos_pmodify_ptr->primary_qos_spec_ptr->tx_flow_template;
        
      if( DSQMH_SUCCESS !=
          dsqmhllif_qos_load_qmi_spec( (qmi_qos_technology_type)cblk_ptr->
                                         um_bearer_tech.current_network,
                                       &ps_qos_spec,
                                       &qos_spec ) )
      {
        DSQMH_MSG_ERROR( "QMH LLIF qos spec load error",0,0,0 );
        *ps_errno = DS_EINVAL;
        result = DSQMH_FAILED;
      }
      else
      {
        /* Command QMI to modify primary QOS flow. */
        ret_val = qmi_qos_modify_primary_qos( qmi_ptr->qos_handle,
                                              &qos_spec,
                                              &qos_spec_err,
                                              &qmi_err_code );
      }
      PS_SYSTEM_HEAP_MEM_FREE( qos_params_mem_ptr );
      break;

    default:
      DSQMH_MSG_ERROR( "QMH LLIF unsupported QOS operation: op=%d iface=%d",
                       operation, iface_inst, 0 );
      *ps_errno = DS_EINVAL;
      break;

  }

  if( QMI_NO_ERR != ret_val )
  {
    DSQMH_MSG_ERROR( "QMH LLIF qos operation error: op=%d iface=%d err=%p",
                     operation, iface_inst, qmi_err_code );
    *ps_errno = DS_EINVAL;
    result = DSQMH_FAILED;
  }

  return result;
} /* dsqmhllif_qos_manager() */



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
)
{
  int result = DSQMH_SUCCESS;
  qmi_link_layer_protocol_type  ll_prot;
  dsqmh_msglib_info_type  *qmi_ptr = NULL;
  int                      qmi_err_code;
  int ret_val = DSQMH_FAILED;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#ifdef FEATURE_DSS_LINUX
  (void) ret_val;
  (void) qmi_err_code;
  (void) qmi_ptr;
  (void) ll_prot;
  (void) flow_ptr;
  (void) iface_inst;
  DSQMH_MSG_LOW("dsqmhllif_qos_enable_qmi_hdr: functionality disabled on Linux",
                0,0,0);
#else

  DSQMH_ASSERT( flow_ptr, "DSQMH LLIF null flow pointer passed" );

  DSQMH_MSG_LOW( "QMH LLIF enable qmi qos hdr", 0, 0, 0 );

  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( iface_inst );

  /*-----------------------------------------------------------------------
    Command QMI to enable the QOS metadata packet header.  Specify
    link layer protocol as IP packets.
  -----------------------------------------------------------------------*/
  if( qmi_ptr )
  {

    ll_prot = QMI_DATA_FORMAT_LINK_PROTOCOL_IP;

    ret_val = qmi_set_port_data_format( qmi_ptr->dev_id,
                                        QMI_DATA_FORMAT_WITH_QOS_HDR,
                                        &ll_prot,
                                        &qmi_err_code );
    if( QMI_NO_ERR != ret_val )
    {
      DSQMH_MSG_ERROR( "QMH LLIF qos enable hdr failed: iface=%d flow=%d err=%p",
                       iface_inst,(uint32)flow_ptr->client_data_ptr,qmi_err_code );
      result = DSQMH_FAILED;
    }
    else
    {
      /* Update QMI Msg Library state. */
      qmi_ptr->qoshdr_enabled = TRUE;
    }
  }
#endif /* FEATURE_DS_LINUX_DRIVER_LEGACY */
  
  return result;
} /* dsqmhllif_qos_enable_qmi_hdr() */

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
)
{
  int result = DSQMH_SUCCESS;
  dsqmh_msglib_info_type *qmi_ptr = NULL;
  int qmi_err_code;
  int ret_val = 0;
  uint32  iface_inst;
  unsigned long stats_mask;
  qmi_wds_xfer_stats stats_response;
  /*-----------------------------------------------------------------------*/
  if (iface_ptr == NULL)
  {
    DSQMH_MSG_ERROR( "dsqmhllif_pkt_stats_cmd:  "
                     "Invalid/NULL input", 0, 0, 0 );
    return;
  }

  iface_inst = (uint32)iface_ptr->client_data_ptr;
  DSQMH_MSG_LOW( "dsqmhllif_pkt_stats_cmd: iface instance%d",
                 iface_inst, 0, 0 );
  
  if( TRUE != DSQMH_IS_IFACE_INST_VALID( iface_inst ))
  {
    DSQMH_MSG_ERROR( "dsqmhllif_pkt_stats_cmd: invalid iface "
                     "inst %d", iface_inst, 0, 0 );
    return;
  }

  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( iface_inst );

  if (qmi_ptr == NULL)
  {
    DSQMH_MSG_ERROR( "dsqmhllif_pkt_stats_cmd:  "
                     "Qmi ptr is not valid", 0, 0, 0 );
    return;
  }

  if (op_type == PS_IFACE_STAT_OP_TYPE_GET)
  {
    if (data_stats == NULL)
    {
      DSQMH_MSG_ERROR( "dsqmhllif_pkt_stats_cmd: PS_IFACE_STAT_OP_TYPE_GET"
                       "failed NULL Input", 0, 0, 0 );
      return;
    }

    /*-----------------------------------------------------------------------
      Request the packet data statistics
    -----------------------------------------------------------------------*/
    stats_mask = QMI_WDS_XFER_STATS_TX_PKTS_GOOD         |
                 QMI_WDS_XFER_STATS_RX_PKTS_GOOD         |
                 QMI_WDS_XFER_STATS_TX_BYTES_OK          |
                 QMI_WDS_XFER_STATS_RX_BYTES_OK          |
                 QMI_WDS_XFER_STATS_TX_PACKETS_DROPPED   |
                 QMI_WDS_XFER_STATS_RX_PACKETS_DROPPED;


    ret_val = qmi_wds_get_pkt_statistics( qmi_ptr->wds_handle,
                                          stats_mask,
                                          &stats_response,
                                          &qmi_err_code );
    if(0 > ret_val)
    {
      DSQMH_MSG_ERROR( "dsqmhllif_get_pkt_stats_cmd: iface=%d err=0x%p",
                       iface_inst, qmi_err_code, 0 );
      return;
    }

    /*-----------------------------------------------------------------------
      QMI message returned successfully.Fill in the statistics and return
    -----------------------------------------------------------------------*/
    data_stats->pkts_tx = stats_response.tx_good_pkt_cnt;
    data_stats->pkts_rx = stats_response.rx_good_pkt_cnt;
    data_stats->bytes_tx = stats_response.tx_good_byte_cnt;
    data_stats->bytes_rx = stats_response.rx_good_byte_cnt;
    data_stats->pkts_dropped_tx = stats_response.tx_pkts_dropped;
    data_stats->pkts_dropped_rx = stats_response.rx_pkts_dropped;
  }
  else if (op_type == PS_IFACE_STAT_OP_TYPE_RESET)
  {
    ret_val = qmi_wds_reset_pkt_statistics( qmi_ptr->wds_handle,
                                            &qmi_err_code );
    if(0 > ret_val)
    {
      DSQMH_MSG_ERROR( "dsqmhllif_pkt_stats_cmd: reset stats err, "
                       "iface=%d err=0x%p", iface_inst, qmi_err_code, 0);
      return;
    }

  }
  else
  {
    DSQMH_MSG_ERROR( "dsqmhllif_pkt_stats_cmd: Unknow stats operation "
                     "requested op_type: %x iface=%d", op_type, iface_inst, 0);
  }

  return;
}

/*===========================================================================
FUNCTION DSQMHLLIF_QUERY_NET_SETTINGS

DESCRIPTION
  This function commands QMI to query the Modem for the network
  interface settings.  The QMI Messaging Library performs this as a
  synchronous operation from caller's perspectve.

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
)
{
  int result = DSQMH_SUCCESS;
  dsqmh_msglib_info_type      *qmi_ptr = NULL;
  int qmi_err_code;
  int ret_val = 0;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_LOW( "QMH LLIF query net settings:%d", iface_inst, 0, 0 );

  if( TRUE != DSQMH_IS_IFACE_INST_VALID( iface_inst ))
  {
    DSQMH_MSG_ERROR( "QMH LLIF invalid iface inst", 0, 0, 0 );
    return DSQMH_FAILED;
  }

  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( iface_inst );

  /*-----------------------------------------------------------------------
    Request the network interface settings.
  -----------------------------------------------------------------------*/
  ret_val = qmi_wds_get_curr_call_info( qmi_ptr->wds_handle,
                                        req_param_mask,
                                        &current_info_ptr->profile_id,
                                        &current_info_ptr->profile_params,
                                        &current_info_ptr->call_info,
                                        &qmi_err_code );
  if( 0 > ret_val )
  {
    DSQMH_MSG_ERROR( "QMH LLIF qmi get curr call info failed: iface=%d err=%p",
                     iface_inst, qmi_err_code, 0 );
    result = DSQMH_FAILED;
  }

  return result;
} /* dsqmhllif_query_net_settings() */


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
)
{
  int result = DSQMH_SUCCESS;
  dsqmh_msglib_info_type      *qmi_ptr = NULL;
  int qmi_err_code;
  int ret_val = 0;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_LOW( "QMH LLIF query net settings:%d", iface_inst, 0, 0 );

  if( TRUE != DSQMH_IS_IFACE_INST_VALID( iface_inst ))
  {
    DSQMH_MSG_ERROR( "QMH LLIF invalid iface inst", 0, 0, 0 );
    return DSQMH_FAILED;
  }

  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( iface_inst );

  /*-----------------------------------------------------------------------
    Request the internal runtime settings.
  -----------------------------------------------------------------------*/
  ret_val = qmi_wds_get_internal_runtime_settings( qmi_ptr->wds_handle,
                                                   req_ptr,
                                                   rsp_ptr,
                                                   &qmi_err_code );
  if( 0 > ret_val )
  {
    DSQMH_MSG_ERROR( "QMH LLIF qmi get int runtime settings failed: iface=%d err=%p",
                     iface_inst, qmi_err_code, 0 );
    result = DSQMH_FAILED;
  }

  return result;
} /* dsqmhllif_query_internal_runtime_settings() */


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
)
{
  int result = DSQMH_SUCCESS;
  dsqmh_msglib_info_type      *qmi_ptr = NULL;
  int qmi_err_code;
  int ret_val = 0;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_LOW( "QMH LLIF set net settings:%d", iface_inst, 0, 0 );

  if( TRUE != DSQMH_IS_IFACE_INST_VALID( iface_inst ))
  {
    DSQMH_MSG_ERROR( "QMH LLIF invalid iface inst", 0, 0, 0 );
    return DSQMH_FAILED;
  }

  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( iface_inst );

  /*-----------------------------------------------------------------------
    Request the internal runtime settings.
  -----------------------------------------------------------------------*/
  ret_val = qmi_wds_set_internal_runtime_settings( qmi_ptr->wds_handle,
                                                   req_ptr,
                                                   rsp_ptr,
                                                   &qmi_err_code );
  if( 0 > ret_val )
  {
    DSQMH_MSG_ERROR( "QMH LLIF qmi get int runtime settings failed: iface=%d err=%p",
                     iface_inst, qmi_err_code, 0 );
    result = DSQMH_FAILED;
  }

  return result;
} /* dsqmhllif_set_internal_runtime_settings() */



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
)
{
  int result = DSQMH_SUCCESS;
  dsqmh_msglib_info_type      *qmi_ptr = NULL;
  int qmi_err_code;
  int ret_val = 0;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_LOW( "QMH LLIF query bearer settings:%d", iface_inst, 0, 0 );

  if( TRUE != DSQMH_IS_IFACE_INST_VALID( iface_inst ))
  {
    DSQMH_MSG_ERROR( "QMH LLIF invalid iface inst", 0, 0, 0 );
    return DSQMH_FAILED;
  }

  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( iface_inst );

  /*-----------------------------------------------------------------------
    Request the bearer technology settings.
  -----------------------------------------------------------------------*/
  ret_val = qmi_wds_get_current_bearer_tech( qmi_ptr->wds_handle,
                                             &current_info_ptr->bearer_info,
                                             &qmi_err_code );
  if( 0 > ret_val )
  {
    DSQMH_MSG_ERROR( "QMH LLIF qmi get curr call info failed: iface=%d err=%p",
                     iface_inst, qmi_err_code, 0 );
    result = DSQMH_FAILED;
  }

  return result;
} /* dsqmhllif_query_bearer_settings() */



/*===========================================================================
FUNCTION DSQMHLLIF_QUERY_CHANNEL_SETTINGS

DESCRIPTION
  This function commands QMI to query the Modem for the channel
  information.  The QMI Messaging Library performs this as a
  synchronous operation from caller's perspective.

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
)
{
  int result = DSQMH_SUCCESS;
  dsqmh_msglib_info_type      *qmi_ptr = NULL;
  int qmi_err_code;
  int ret_val = 0;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_LOW( "QMH LLIF query channel settings:%d", iface_inst, 0, 0 );

  if( TRUE != DSQMH_IS_IFACE_INST_VALID( iface_inst ))
  {
    DSQMH_MSG_ERROR( "QMH LLIF invalid iface inst", 0, 0, 0 );
    return DSQMH_FAILED;
  }

  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( iface_inst );

  /*-----------------------------------------------------------------------
    Request the channel rate.
  -----------------------------------------------------------------------*/
  ret_val = qmi_wds_get_current_channel_rate( qmi_ptr->wds_handle,
                                              &current_info_ptr->channel_info,
                                              &qmi_err_code );
  if( 0 > ret_val )
  {
    DSQMH_MSG_ERROR( "QMH LLIF qmi get curr channel rate failed: iface=%d err=%p",
                     iface_inst, qmi_err_code, 0 );
    result = DSQMH_FAILED;
  }

  return result;
} /* dsqmhllif_query_channel_settings() */


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
  uint32                   obj_id,
  void                    *argval_ptr
)
{
  int result = DSQMH_SUCCESS;
  ps_iface_type              *iface_ptr = NULL;
  ps_flow_type               *flow_ptr = NULL;
  dsqmh_msglib_info_type     *qmi_ptr = NULL;
  dsqmh_iface_cblk_type      *cblk_ptr = NULL;
  qmi_qos_status_info         stat_resp_data;
  qmi_qos_nw_status_type      nw_resp_data;
  int qmi_err_code = 0;
  int ret_val = 0;
  /* Map QMI flow status to PS layer values */
  ps_flow_state_enum_type map_flow_stat_qmi2ps[] =
    { FLOW_STATE_INVALID,  /* NULL                     (0x0) */
      FLOW_ACTIVATED,      /* QMI_QOS_STATUS_ACTIVATED (0x1) */
      FLOW_SUSPENDED,      /* QMI_QOS_STATUS_SUSPENDED (0x2) */
      FLOW_NULL };         /* QMI_QOS_STATUS_GONE      (0x3) */

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_TEST(argval_ptr, "DSQMH LLIF null argval pointer passed");

  DSQMH_MSG_LOW( "QMH LLIF query qos settings:%d type=%d", obj_id, type, 0 );


  /*-------------------------------------------------------------------
    Request the QOS setting info.
  -------------------------------------------------------------------*/
  switch( type )
  {
    case DSQMHLLIF_QOS_INFO_NETWORK:
      cblk_ptr = DSQMH_GET_CBLK_PTR( obj_id );

      if ( DSQMH_QOS_SUPPORT_AVAILABLE == cblk_ptr->qos_supported )
      {
        *(ps_iface_ioctl_on_qos_aware_system_type*) argval_ptr = TRUE;
      }
      else if ( DSQMH_QOS_SUPPORT_UNAVAILABLE == cblk_ptr->qos_supported )
      {
        *(ps_iface_ioctl_on_qos_aware_system_type*) argval_ptr = FALSE;
      }
      else if ( DSQMH_QOS_SUPPORT_UNKNOWN == cblk_ptr->qos_supported )
      {
        qmi_ptr = DSQMH_GET_QMI_INFO_PTR( obj_id );
        if( !qmi_ptr )
        {
          DSQMH_MSG_ERROR( "QMH LLIF flow iface inst invalid: %d",
                           obj_id,0,0 );
          return DSQMH_FAILED;
        }

        ret_val = qmi_qos_does_nw_support_qos( qmi_ptr->qos_handle,
                                               &nw_resp_data, 
                                               &qmi_err_code );
        if( !ret_val )
        {
          *(ps_iface_ioctl_on_qos_aware_system_type*) argval_ptr =
            (ps_iface_ioctl_on_qos_aware_system_type)
             nw_resp_data.qos_supported;
        }
      }
      else
      {
        DSQMH_MSG_ERROR( "QMH LLIF invalid qos_supported value : %d",
                         cblk_ptr->qos_supported, 0, 0 );
        return DSQMH_FAILED;
      }

      break;

    case DSQMHLLIF_QOS_INFO_STATUS:
      flow_ptr = (ps_flow_type*)obj_id;
      DSQMH_GET_IFACE_PTR_FROM_FLOW( flow_ptr, iface_ptr );
      if( !iface_ptr )
      {
        DSQMH_MSG_ERROR( "QMH LLIF flow iface invalid: %p",
                         flow_ptr,0,0 );
        return DSQMH_FAILED;
      }
      qmi_ptr = DSQMH_GET_QMI_INFO_PTR( (uint32)iface_ptr->client_data_ptr );
      if( !qmi_ptr )
      {
        DSQMH_MSG_ERROR( "QMH LLIF iface inst invalid: %d",
                         (uint32)iface_ptr->client_data_ptr,0,0 );
        return DSQMH_FAILED;
      }
      
      ret_val = qmi_qos_get_status( qmi_ptr->qos_handle,
                                         (unsigned long)flow_ptr->client_data_ptr,
                                         &stat_resp_data,
                                         &qmi_err_code );
      if( !ret_val )
      {
        *(ps_flow_state_enum_type*)argval_ptr =
          map_flow_stat_qmi2ps[ stat_resp_data ];
      }
      break;

    default:
      DSQMH_MSG_ERROR( "QMH LLIF unsupported type:%d",type,0,0);
      result = DSQMH_FAILED;
      break;
  }

  if( 0 > ret_val )
  {
    DSQMH_MSG_ERROR( "QMH LLIF qmi get QOS settings: err=%p",
                     qmi_err_code, 0, 0 );
    result = DSQMH_FAILED;
  }

  return result;
} /* dsqmhllif_query_qos_settings() */



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
  mcast_hndl_list   - Pointer to MCAST handles
  ps_errno          - Error code returned in case of failure

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
  sint15                              *ps_errno
)
{
  int result = DSQMH_SUCCESS;
  dsqmh_msglib_info_type                  *qmi_ptr = NULL;
  qmi_wds_mcast_ipv4_join_info_param_type *join_ptr = NULL; 
  qmi_wds_mcast_hndl_rsp_type              join_rsp;
  qmi_wds_mcast_join_req_params_type      *join_ex_ptr = NULL;
  qmi_wds_initiate_mcast_join_ex_rsp_type  join_ex_rsp;
  qmi_wds_initiate_mcast_leave_ex_rsp_type leave_rsp;
  qmi_wds_initiate_mcast_reg_ex_rsp_type   reg_rsp;
  int qmi_err_code = 0;
  int ret_val = 0;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( info_ptr, "DSQMH LLIF null info ptr passed" );
  DSQMH_ASSERT( mcast_hndl_list, "DSQMH LLIF null mcast hdnl ptr passed" );
  DSQMH_ASSERT( ps_errno, "DSQMH LLIF null ps_errno ptr passed" );

  DSQMH_MSG_LOW( "QMH LLIF MCAST manager:%d ioctl=%d",
                 iface_inst, ioctl_name, 0 );
  *ps_errno = DS_ENOERR;
    
  if( TRUE != DSQMH_IS_IFACE_INST_VALID( iface_inst ))
  {
    DSQMH_MSG_ERROR( "QMH LLIF invalid iface inst", 0, 0, 0 );
    return DSQMH_FAILED;
  }

  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( iface_inst );

  switch( ioctl_name )
  {
    case PS_IFACE_IOCTL_MCAST_JOIN:
      join_ptr = (qmi_wds_mcast_ipv4_join_info_param_type*)info_ptr;
      memset((void*)&join_rsp, 0x0, sizeof(join_rsp));
      ret_val = qmi_wds_initiate_mcast_join( qmi_ptr->wds_handle,
                                             join_ptr,
                                             &join_rsp,
                                             &qmi_err_code );
      if( QMI_NO_ERR == ret_val )
      {
        mcast_hndl_list->num_handles = 1;
        mcast_hndl_list->mcast_handle_list[0] = join_rsp;
      }
      else
      {
        mcast_hndl_list->num_handles = 0;
        *ps_errno = DS_EINVAL;
      }
      break;
      
    case PS_IFACE_IOCTL_MCAST_LEAVE:
      ret_val = qmi_wds_initiate_mcast_leave( qmi_ptr->wds_handle,
                                              &mcast_hndl_list->mcast_handle_list[0],
                                              &qmi_err_code );
      if( QMI_NO_ERR != ret_val )
      {
        *ps_errno = DS_EINVAL;
      }
      break;

    case PS_IFACE_IOCTL_MCAST_JOIN_EX:
      join_ex_ptr = (qmi_wds_mcast_join_req_params_type*)info_ptr;
      ret_val = qmi_wds_initiate_mcast_join_ex( qmi_ptr->wds_handle,
                                                join_ex_ptr,
                                                &join_ex_rsp,
                                                &qmi_err_code );
      *mcast_hndl_list = join_ex_rsp.hndl_list;
      DSQMH_SET_PS_ERRNO( join_ex_rsp.dss_errno, (*ps_errno) );
      break;

    case PS_IFACE_IOCTL_MCAST_LEAVE_EX:
      ret_val = qmi_wds_initiate_mcast_leave_ex( qmi_ptr->wds_handle,
                                                 mcast_hndl_list,
                                                 &leave_rsp,
                                                 &qmi_err_code );
      DSQMH_SET_PS_ERRNO( leave_rsp, (*ps_errno) );
      break;

    case PS_IFACE_IOCTL_MCAST_REGISTER_EX:
      ret_val = qmi_wds_initiate_mcast_register_ex( qmi_ptr->wds_handle,
                                                    mcast_hndl_list,
                                                    &reg_rsp,
                                                    &qmi_err_code  );
      DSQMH_SET_PS_ERRNO( reg_rsp, (*ps_errno) );
      break;

    default:
      break;
  }

  if( 0 > ret_val )
  {
    DSQMH_MSG_ERROR( "QMH LLIF qmi MCAST operation failed:%d ioctl=%d err=%p",
                     iface_inst, ioctl_name, qmi_err_code );
    result = DSQMH_FAILED;
  }

  return result;
} /* dsqmhllif_mcast_manager() */


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
  qmi_wds_mbms_context_handle          *mbms_hndl
)
{
  int result = DSQMH_SUCCESS;
  dsqmh_msglib_info_type      *qmi_ptr = NULL;
  int qmi_err_code = 0;
  int ret_val = 0;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_LOW( "QMH LLIF MBMS manager:%d iface=%d",
                 operation, iface_inst, 0 );

  if( TRUE != DSQMH_IS_IFACE_INST_VALID( iface_inst ))
  {
    DSQMH_MSG_ERROR( "QMH LLIF invalid iface inst", 0, 0, 0 );
    return DSQMH_FAILED;
  }

  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( iface_inst );

  switch( operation )
  {
    case DSQMHLLIF_MBMS_OP_ACTIVATE:
      ret_val = qmi_wds_initiate_mbms_context_activate( qmi_ptr->wds_handle,
                                                        mbms_info,
                                                        mbms_hndl,
                                                        &qmi_err_code );
      break;

    case DSQMHLLIF_MBMS_OP_DEACTIVATE:
      ret_val = qmi_wds_initiate_mbms_context_deactivate( qmi_ptr->wds_handle,
                                                          mbms_hndl,
                                                          &qmi_err_code );
      break;

    default:
      break;
  }

  if( 0 > ret_val )
  {
    DSQMH_MSG_ERROR( "QMH LLIF qmi MBMS operation failed: op=%d iface=%d err=%p",
                     operation, iface_inst, qmi_err_code );
    result = DSQMH_FAILED;
  }

  return result;
} /* dsqmhllif_mbms_manager() */



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
)
{
  int result = DSQMH_SUCCESS;
  dsqmh_msglib_info_type      *qmi_ptr = NULL;
  qmi_wds_bcmcs_db_updt_rsp_type            db_updt_rsp;
  qmi_wds_bcmcs_enable_handoff_reg_rsp_type ho_reg_rsp;
  qmi_wds_bcmcs_bom_caching_setup_rsp_type  bom_cs_rsp;
  int qmi_err_code = 0;
  int ret_val = 0;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_LOW( "QMH LLIF BCMCS manager:%d iface=%d",
                 operation, iface_inst, 0 );

  if( TRUE != DSQMH_IS_IFACE_INST_VALID( iface_inst ))
  {
    DSQMH_MSG_ERROR( "QMH LLIF invalid iface inst", 0, 0, 0 );
    return DSQMH_FAILED;
  }

  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( iface_inst );

  switch( operation )
  {
    case DSQMHLLIF_BCMCS_OP_DB_UPDATE:
      ret_val =
        qmi_wds_bcmcs_db_update_req( qmi_ptr->wds_handle,
                                     (qmi_wds_bcmcs_db_updt_params_type*)bcmcs_info,
                                     &db_updt_rsp,
                                     &qmi_err_code );
      DSQMH_SET_PS_ERRNO( db_updt_rsp, (*ps_errno) );
      break;

    case DSQMHLLIF_BCMCS_OP_HANDOFF_REG:
      ret_val =
        qmi_wds_bcmcs_enable_handoff_reg_req( qmi_ptr->wds_handle,
                                              (qmi_wds_bcmcs_handoff_optimization_info*)bcmcs_info,
                                              &ho_reg_rsp,
                                              &qmi_err_code );
      DSQMH_SET_PS_ERRNO( ho_reg_rsp, (*ps_errno) );
      break;

    case DSQMHLLIF_BCMCS_OP_BOM_CACHING:
      ret_val =
        qmi_wds_bcmcs_bom_caching_setup_req( qmi_ptr->wds_handle,
                                             (qmi_wds_bom_caching_setup_req_param_type*)bcmcs_info,
                                             &bom_cs_rsp,
                                             &qmi_err_code );
      DSQMH_SET_PS_ERRNO( bom_cs_rsp, (*ps_errno) );
      break;

    default:
      break;
  }

  if( 0 > ret_val )
  {
    DSQMH_MSG_ERROR( "QMH LLIF qmi BCMCS operation failed: op=%d iface=%d err=%p",
                     operation, iface_inst, qmi_err_code );
    result = DSQMH_FAILED;
  }

  return result;
} /* dsqmhllif_bcmcs_manager() */


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
)
{
  int result = DSQMH_SUCCESS;
  dsqmh_msglib_info_type      *qmi_ptr = NULL;
  int qmi_err_code = 0;
  int ret_val = 0;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_LOW( "QMH LLIF MCAST manager:%d iface=%d",
                 operation, iface_inst, 0 );

  if( TRUE != DSQMH_IS_IFACE_INST_VALID( iface_inst ))
  {
    DSQMH_MSG_ERROR( "QMH LLIF invalid iface inst", 0, 0, 0 );
    return DSQMH_FAILED;
  }

  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( iface_inst );

  switch( operation )
  {
    case DSQMHLLIF_DORMANCY_OP_GOACTIVE:
      ret_val = qmi_wds_go_active_req( qmi_ptr->wds_handle,
                                       &qmi_err_code );
      break;

    case DSQMHLLIF_DORMANCY_OP_GODORMANT:
      ret_val = qmi_wds_go_dormant_req( qmi_ptr->wds_handle,
                                        &qmi_err_code );
      break;

    default:
      break;
  }

  if( 0 > ret_val )
  {
    DSQMH_MSG_ERROR( "QMH LLIF qmi dormancy operation failed: op=%d iface=%d err=%p",
                     operation, iface_inst, qmi_err_code );
    result = DSQMH_FAILED;
  }

  return result;
} /* dsqmhllif_dormancy_manager() */


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
)
{
  dsqmh_runtime_info_type     current_info;
  qmi_wds_req_runtime_settings_params_type req_param_mask;
  ps_ip_addr_type             *gw_ip;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_LOW( "QMH LLIF QUERY INTERFACE GW %d", iface_inst, 0, 0 );

  /*-----------------------------------------------------------------------
    Request the network interface settings (GW IP address).
  -----------------------------------------------------------------------*/
  memset( (void*)&current_info, 0x0, sizeof(current_info) );

  /* Indicate settings of interest. These values will be cached in the
   * PS IFACE datastructure. They are negotiated at IFACE bringup,
   * when reconfiguration is performed, and on EXTENDED_IP_CONFIG event.
   */
  req_param_mask =
      QMI_WDS_GET_CURR_CALL_INFO_GATEWAY_INFO_PARAM_MASK;

  if( DSQMH_SUCCESS != dsqmhllif_query_net_settings( iface_inst,
                                                     req_param_mask,
                                                     &current_info ) )
  {
    DSQMH_MSG_ERROR( "QMH LLIF failed to query nw settings", 0, 0, 0 );
    *ps_errno = DS_EINVAL;
    return DSQMH_FAILED;
  }
  else
  {
    if( QMI_WDS_CURR_CALL_INFO_IPV4_GATEWAY_ADDR & current_info.call_info.mask )
    {  //Change the family type here
      gw_info->type = IFACE_IPV4_ADDR_FAMILY;
      gw_info->addr.v4 = ps_htonl(current_info.call_info.ipv4_gateway_addr);
      DSQMH_MSG_IPV4_ADDR("Gateway Info", gw_info->addr.v4);
    } 
    else 
    {
      DSQMH_MSG_ERROR( "QMH LLIF  GATEWAY INFO undefined iface %d", iface_inst, 0, 0 );
      *ps_errno = DS_EINVAL;
      return DSQMH_FAILED;
    }
  }

  return DSQMH_SUCCESS;
}


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
)
{
  ps_iface_type              *iface_ptr = NULL;
  ps_ip_addr_type             ip_addr;
  ps_ip_addr_type             prim_dns_addr;
  ps_ip_addr_type             sec_dns_addr;
  dsqmh_runtime_info_type     current_info;
  qmi_wds_req_runtime_settings_params_type req_param_mask;
  uint32                      count;
  uint32                      maxnamelen = PS_IFACE_MAX_DOMAIN_NAME_SIZE;
  errno_enum_type             ps_errno;
  int result = DSQMH_SUCCESS;
#ifdef FEATURE_DATA_PS_IPV6
  struct ps_in6_addr         *ipv6_addr = NULL;
#endif

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_LOW( "QMH LLIF configure iface:%d", iface_inst, 0, 0 );

  iface_ptr = DSQMH_GET_IFACE_PTR( iface_inst );

  /*-----------------------------------------------------------------------
    Request the network interface settings (IP address).
  -----------------------------------------------------------------------*/
  memset( (void*)&current_info, 0x0, sizeof(current_info) );
  (void)result;

  /* Indicate settings of interest. These values will be cached in the
   * PS IFACE datastructure. They are negotiated at IFACE bringup,
   * when reconfiguration is performed, and on EXTENDED_IP_CONFIG event.
   */
  req_param_mask =
    ( QMI_WDS_GET_CURR_CALL_INFO_IP_ADDRESS_PARAM_MASK              |
      QMI_WDS_GET_CURR_CALL_INFO_IP_FAMILY_PARAM_MASK               |
      QMI_WDS_GET_CURR_CALL_INFO_DNS_ADDR_PARAM_MASK                |
      QMI_WDS_GET_CURR_CALL_INFO_DOMAIN_NAME_LIST_PARAM_MASK        |
      QMI_WDS_GET_CURR_CALL_INFO_PCSCF_SERV_ADDR_PARAM_MASK         |
      QMI_WDS_GET_CURR_CALL_INFO_PCSCF_DOMAIN_NAME_LIST_PARAM_MASK  |
      QMI_WDS_GET_CURR_CALL_INFO_TECHNOLOGY_NAME_PARAM_MASK         |
      QMI_WDS_GET_CURR_CALL_INFO_MTU_PARAM_MASK
    );

  if( DSQMH_SUCCESS != dsqmhllif_query_net_settings( iface_inst,
                                                     req_param_mask,
                                                     &current_info ) )
  {
    DSQMH_MSG_ERROR( "QMH LLIF failed to query nw settings", 0, 0, 0 );
    return DSQMH_FAILED;
  }
  else
  {
    /*---------------------------------------------------------------------
      Preserve the retrieved Um IFACE attributes.
    ---------------------------------------------------------------------*/

    /* IP Address Family */
    if( QMI_WDS_CURR_CALL_INFO_IP_FAMILY &
        current_info.call_info.mask )
    {
#ifndef FEATURE_DSS_LINUX   /* Done in platform layer */
      if( DSQMH_FAILED ==
          ps_iface_set_addr_family( iface_ptr,
                                    (ps_iface_addr_family_type)current_info.call_info.ip_family ) )
      {
        DSQMH_MSG_ERROR( "QMH LLIF failed to set addr family", 0, 0, 0 );
        return DSQMH_FAILED;
      }
#endif /* FEATURE_DSS_LINUX */
      DSQMH_MSG_MED( "addrfamily=%d",
                     current_info.call_info.ip_family, 0, 0 );
    }

    /* IPv4 Address */
    if( QMI_WDS_CURR_CALL_INFO_IPV4_ADDR &
        current_info.call_info.mask )
    {
      ip_addr.type = IFACE_IPV4_ADDR_FAMILY;
      ip_addr.addr.v4.ps_s_addr =
        ps_htonl( current_info.call_info.ipv4_addr );
#ifndef FEATURE_DSS_LINUX   /* Done in platform layer */
      (void)ps_iface_set_v4_addr( iface_ptr, &ip_addr );
#endif /* FEATURE_DSS_LINUX */
      DSQMH_MSG_IPV4_ADDR( "Modem iface ", 
                           ip_addr.addr.v4.ps_s_addr );
    }

    /* IPv4 DNS Primary & Secondary Addresses */
    if( QMI_WDS_CURR_CALL_INFO_PRIMARY_DNS_IPV4_ADDR &
         current_info.call_info.mask)
    {
      prim_dns_addr.type = IFACE_IPV4_ADDR_FAMILY;
      prim_dns_addr.addr.v4.ps_s_addr =
        ps_htonl( current_info.call_info.primary_dns_ipv4_addr );
      DSQMH_MSG_IPV4_ADDR( "Primary DNS Addr is  ",
                           prim_dns_addr.addr.v4.ps_s_addr );

      if( QMI_WDS_CURR_CALL_INFO_SECONDARY_DNS_IPV4_ADDR &
         current_info.call_info.mask )
      {
        sec_dns_addr.type = IFACE_IPV4_ADDR_FAMILY;
        sec_dns_addr.addr.v4.ps_s_addr =
        ps_htonl( current_info.call_info.secondary_dns_ipv4_addr );

        (void)ps_iface_set_v4_dns_addrs( iface_ptr,
                                         &prim_dns_addr, 
                                         &sec_dns_addr );

        DSQMH_MSG_IPV4_ADDR( "Sec DNS Addr is ",
                           sec_dns_addr.addr.v4.ps_s_addr );
      }
      else
      {
        (void)ps_iface_set_v4_dns_addrs( iface_ptr, &prim_dns_addr, NULL );
      }      
    }

    /* Domain Name search list */
    if( QMI_WDS_CURR_CALL_INFO_DOMAIN_NAME_LIST &
        current_info.call_info.mask )
    {
      uint32 i;
      ps_iface_domain_name_type   name_list[QMI_WDS_MAX_DOMAIN_NAMES];

      /* Load the domain name list */
      count = MIN( current_info.call_info.domain_name_list.num_instances,
                   QMI_WDS_MAX_DOMAIN_NAMES );
      for( i=0; i<count; i++ )
      {
        qmi_wds_domain_names *name_ptr =
          &current_info.call_info.domain_name_list.domain_names[i];

        /* Filter for name length in limits */
        if( PS_IFACE_MAX_DOMAIN_NAME_SIZE > name_ptr->domain_name_len )
        {
          memcpy( (void*)name_list[i].domain_name,
                  name_ptr->domain_name,
                  MIN( maxnamelen, sizeof(name_ptr->domain_name) ) );
        }
      }
      (void)ps_iface_set_domain_name_search_list( iface_ptr,
                                                  name_list,
                                                  count,
                                                  &ps_errno );
      DSQMH_MSG_MED( "dns_list=%d", count, 0, 0 );
    }

    /* SIP server list for v4 */
    if( QMI_WDS_CURR_CALL_INFO_PCSCF_IPV4_ADDR_LIST &
        current_info.call_info.mask )
    {
      uint32 i;
      ps_ip_addr_type  ip_addr_list[QMI_WDS_MAX_P_CSCF_IPV4_ADDRS];

      /* Load the domain name list */
      count = MIN( current_info.call_info.p_cscf_ipv4_addrs.num_instances,
                   QMI_WDS_MAX_P_CSCF_IPV4_ADDRS );
      for( i=0; i<count; i++ )
      {
        ip_addr_list[i].type = IFACE_IPV4_ADDR_FAMILY;
        ip_addr_list[i].addr.v4.ps_s_addr =
          ps_htonl( current_info.call_info.p_cscf_ipv4_addrs.p_cscf_ipv4_addr[i] );
      }
      (void)ps_iface_set_sip_serv_addr( iface_ptr,
                                        ip_addr_list,
                                        count,
                                        &ps_errno );
      DSQMH_MSG_MED( "sip_srvr_list=%d", count, 0, 0 );
    }

    /* SIP server list for v6 */
    if( QMI_WDS_CURR_CALL_INFO_PCSCF_IPV6_ADDR_LIST &
        current_info.call_info.mask )
    {
      uint32 i;
      ps_ip_addr_type  ip_addr_list[QMI_WDS_MAX_P_CSCF_IPV6_ADDRS];

      /* Load the domain name list */
      count = MIN( current_info.call_info.p_cscf_ipv6_addrs.num_instances,
                   QMI_WDS_MAX_P_CSCF_IPV6_ADDRS );
      for( i=0; i<count; i++ )
      {
        ip_addr_list[i].type = IFACE_IPV6_ADDR_FAMILY;
        memcpy( ip_addr_list[i].addr.v6.ps_s6_addr, 
                 current_info.call_info.p_cscf_ipv6_addrs.p_cscf_ipv6_addr[i],
                   sizeof( ipv6_addr_type ) );
      }
      (void)ps_iface_set_sip_serv_addr( iface_ptr,
                                        ip_addr_list,
                                        count,
                                        &ps_errno );
      DSQMH_MSG_MED( "sip_srvr_list=%d", count, 0, 0 );
    }

    /* SIP Domain Name list */
    if( QMI_WDS_CURR_CALL_INFO_PCSCF_FQDN_LIST &
        current_info.call_info.mask )
    {
      uint32 i;
      ps_iface_domain_name_type   name_list[QMI_WDS_MAX_FQDN_STRINGS];

      /* Load the domain name list */
      count = MIN( current_info.call_info.fqdn_list.num_instances,
                   QMI_WDS_MAX_FQDN_STRINGS );
      for( i=0; i<count; i++ )
      {
        qmi_wds_fqdn_strings *name_ptr =
          &current_info.call_info.fqdn_list.fqdn_strings[i];

        /* Filter for name length in limits */
        if( PS_IFACE_MAX_DOMAIN_NAME_SIZE > name_ptr->fqdn_length )
        {
          memcpy( (void*)name_list[i].domain_name,
                  name_ptr->fqdn_string,
                  MIN( maxnamelen, sizeof(name_ptr->fqdn_string) ) );
        }
      }
      (void)ps_iface_set_sip_domain_names( iface_ptr,
                                           name_list,
                                           count,
                                           &ps_errno );
      DSQMH_MSG_MED( "sip_dns_list=%d", count, 0, 0 );
    }

    /* MTU */
    if( QMI_WDS_CURR_CALL_INFO_MTU &
        current_info.call_info.mask )
    {
      iface_ptr->net_info.mtu = (uint16)current_info.call_info.mtu;
      DSQMH_MSG_MED( "mtu=%d", current_info.call_info.mtu, 0, 0 );
    }

#ifdef FEATURE_DATA_PS_IPV6
    /* IPv6 DNS Primary & Secondary Addresses */
    if( QMI_WDS_CURR_CALL_INFO_PRIMARY_DNS_IPV6_ADDR &
        current_info.call_info.mask )
    {
      ipv6_addr = (struct ps_in6_addr*)current_info.call_info.primary_dns_ipv6_addr;
      DSQMH_HTON_IPV6_ADDR( ipv6_addr, ipv6_addr );
      prim_dns_addr.type = IFACE_IPV6_ADDR_FAMILY;
      prim_dns_addr.addr.v6.ps_s6_addr64[0] = ipv6_addr->ps_s6_addr64[0];
      prim_dns_addr.addr.v6.ps_s6_addr64[1] = ipv6_addr->ps_s6_addr64[1];

      if( QMI_WDS_CURR_CALL_INFO_SECONDARY_DNS_IPV6_ADDR &
          current_info.call_info.mask )
      {
        ipv6_addr = (struct ps_in6_addr*)current_info.call_info.secondary_dns_ipv6_addr;
        DSQMH_HTON_IPV6_ADDR( ipv6_addr, ipv6_addr );
        sec_dns_addr.type = IFACE_IPV6_ADDR_FAMILY;
        sec_dns_addr.addr.v6.ps_s6_addr64[0] = ipv6_addr->ps_s6_addr64[0];
        sec_dns_addr.addr.v6.ps_s6_addr64[1] = ipv6_addr->ps_s6_addr64[1];

        (void)ps_iface_set_v6_dns_addrs( iface_ptr, &prim_dns_addr, &sec_dns_addr );
        DSQMH_MSG_IPV6_ADDR("Primary IPV6 DNS Addr is ",
                            prim_dns_addr.addr.v6.ps_s6_addr64);
        DSQMH_MSG_IPV6_ADDR("Sec IPV6 DNS Addr is ",
                            sec_dns_addr.addr.v6.ps_s6_addr64);
      }
      else
      {
        (void)ps_iface_set_v6_dns_addrs( iface_ptr, &prim_dns_addr, NULL );
        DSQMH_MSG_IPV6_ADDR("Primary IPV6 DNS Addr is ",
                            prim_dns_addr.addr.v6.ps_s6_addr64);
      }
    }
#endif /* FEATURE_DATA_PS_IPV6 */
  }

  return result;
} /* dsqmhllif_configure_iface_cmd() */



/*===========================================================================
FUNCTION DSQMHLLIF_IS_QMIPORT_INCALL

DESCRIPTION
  This function indirectly determines if QMI port is in data call.
  Uses QMI packet service status query.

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
)
{
  dsqmh_msglib_info_type   *qmi_ptr = NULL;
  qmi_wds_link_status_type  link_state = QMI_WDS_PACKET_DATA_DISCONNECTED;
  int qmi_ret, qmi_err_code;

  DSQMH_ASSERT( inuse_ptr, "null inuse pointer passed" );

  /* Validate input parameter */
  if(!DSQMH_IS_IFACE_INST_VALID( iface_inst ) )
  {
    DSQMH_MSG_ERROR( "Invalid interface specified: %d", iface_inst, 0, 0 );
    return DSQMH_FAILED;
  }

  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( iface_inst );
  if( !qmi_ptr )
  {
    DSQMH_MSG_ERROR( "Invalid interface specified: %d", iface_inst, 0, 0 );
    return DSQMH_FAILED;
  }
  
  /* Query QMI packet service status */
  qmi_ret = qmi_wds_get_pkt_srvc_status( qmi_ptr->wds_handle,
                                         &link_state,
                                         &qmi_err_code );
  if( QMI_NO_ERR !=qmi_ret )
  {
    DSQMH_MSG_ERROR("qmi_wds_get_pkt_srvc_status failed "\
                    "on iface [%d] with err [%d][%d]",
                    iface_inst, qmi_ret, qmi_err_code);
    return DSQMH_FAILED;
  }

  /* Check QMI link status for disconnect state */
  if( QMI_WDS_PACKET_DATA_DISCONNECTED != link_state )
  {
    *inuse_ptr = TRUE;
  }

  DSQMH_MSG_MED( "QMI port state for iface[%d]: inuse=%d",
                 iface_inst, *inuse_ptr, 0 );
  return DSQMH_SUCCESS;
} /* dsqmhllif_is_qmiport_incall */


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
)
{
  dsqmh_msglib_info_type      *qmi_ptr = NULL;
  qmi_wds_route_look_up_params_type  req_params;
  qmi_wds_route_look_up_rsp_type     rsp_params;
  uint32        iface_inst = 0;
  int result = DSQMH_SUCCESS;
  int qmi_err_code;
  int ret_val = 0;
  uint32                               supported_call_types = 0;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_MED( "QMH LLIF query route by policy", 0, 0, 0 );

  if (NULL == acl_pi_ptr)
  {
    DSQMH_MSG_ERROR ("NULL ACL policy, ignore route request", 0, 0, 0);
    return DSQMH_FAILED;
  }

  /*-----------------------------------------------------------------------
    Check if call type is supported.
  -----------------------------------------------------------------------*/
  supported_call_types = dsqmh_config_get_supported_call_types();

  switch (acl_pi_ptr->proc_id)
  {
    case QMH_PROC_ID_LOCAL:
      if (0 == (supported_call_types & DS_QMH_CALL_TYPE_LOCAL))
      {
        result = DSQMH_FAILED;
      }
      break;

    case QMH_PROC_ID_RMNET_TETHERED:
      if (0 == (supported_call_types & DS_QMH_CALL_TYPE_RMNET_TETHERED))
      {
        result = DSQMH_FAILED;
      }
      break;

    case QMH_PROC_ID_RMNET_EMBEDDED:
      if (0 == (supported_call_types & DS_QMH_CALL_TYPE_RMNET_EMBEDDED))
      {
        result = DSQMH_FAILED;
      }
      break;

    default:
      result = DSQMH_FAILED;
      break;
  }

  if (DSQMH_FAILED == result)
  {
    DSQMH_MSG_HIGH ("Unsupported Proc ID %d, supported call types 0x%x", 
                     acl_pi_ptr->proc_id, supported_call_types, 0);
    return DSQMH_FAILED;
  }


  /*-----------------------------------------------------------------------
    Check for policy with IFACE ID specified.  This will be handled in
    ACL logic.  It makes no sense to send route query to Modem so exit.
  -----------------------------------------------------------------------*/
  if( DSS_IFACE_ID == acl_pi_ptr->iface.kind )
  {
    DSQMH_MSG_LOW( "QMH LLIF query route by IFACE ID, exiting", 0, 0, 0 );
    result_ptr->priority = 0;
    result_ptr->rm_hndl = result_ptr->if_hndl = DSQMH_INVALID_HANDLE;
    return DSQMH_SUCCESS;
  }

  DSQMH_MSG_MED( "ACL dump: Proc ID=%d, policy flag=%d, data path flag=%d ",
                  acl_pi_ptr->proc_id,
                  acl_pi_ptr->policy_flag,
                  acl_pi_ptr->data_path_policy_flag );
  DSQMH_MSG_MED( "Iface name 0x%x, app_id=0x%x, ip_family=%d",
                  acl_pi_ptr->iface.info.name,
                  acl_pi_ptr->app_identifier,
                  acl_pi_ptr->ip_family );
  DSQMH_MSG_MED( "is_sock_orig=%d, is_routeable=%d",
                  acl_pi_ptr->is_sock_orig,
                  acl_pi_ptr->is_routeable,
                  0);
  DSQMH_MSG_MED( "Lookup only %d, bring_up %d",
                  acl_pi_ptr->lookup_only,
                  acl_pi_ptr->bring_up,
                  0 );

  DSQMH_MSG_HIGH( "QMH State: self init %d, conn init %d, iface state %d",
                  dsqmh_state_info.self_init,
                  dsqmh_state_info.conn_init,
                  ps_iface_state(DSQMH_GET_IFACE_PTR(iface_inst)) );
  
  DSQMHLLIF_VERIFY_INIT_COMPLETED();
  memset( (void*)&req_params, 0x0, sizeof(req_params) );

  /* Use first QMH instance on specified modem to perform route lookup */
  iface_inst = dsqmh_config_get_iface_inst_from_rm_handle (0, proc_id);
  if ( -1 == (int)iface_inst )
  {
    DSQMH_MSG_ERROR( "QMH LLIF query route by IFACE ID, bad iface map", 0, 0, 0 );
    result_ptr->priority = 0;
    result_ptr->rm_hndl = result_ptr->if_hndl = DSQMH_INVALID_HANDLE;
    return DSQMH_FAILED;
  }

  qmi_ptr = DSQMH_GET_QMI_INFO_PTR( iface_inst );

  /*-----------------------------------------------------------------------
    Convert the DSS IFACE Name to a technology preference.  The QMI
    layer may support a subset of all technologies, so we filter
    routing requests based on what is supproted.  Unsupported
    technologies will yeild a NULL routing result.
  -----------------------------------------------------------------------*/
  DSQMH_SET_TECH_PREF( acl_pi_ptr->iface,
                       req_params.tech_pref, req_params.xtended_tech_pref );
  if( DSQMH_INVALID_TECHPREF == req_params.xtended_tech_pref )
  {
    DSQMH_MSG_ERROR( "QMH LLIF qmi query route for unsupported tech pref: %d 0x%x",
                     acl_pi_ptr->iface.kind, acl_pi_ptr->iface.info.name, 0 );
    result_ptr->rm_hndl = DSQMH_INVALID_RMNET;
    return DSQMH_SUCCESS;
  }
  req_params.params_mask |= QMI_WDS_ROUTE_LOOK_UP_TECH_PREF_PARAM;

  DSQMH_SET_IPADDR_FAMILY_PREF( acl_pi_ptr->ip_family,
                                req_params.ip_family_pref );

  DSQMH_SET_PROFILE_INDEX( acl_pi_ptr,
                           req_params.profile_index,
                           req_params.profile_index_3gpp2,
                           req_params.tech_pref,
                           req_params.params_mask );

  if(acl_pi_ptr->lookup_only || acl_pi_ptr->bring_up)
  {
    req_params.route_lookup = INTERFACE_LOOKUP;
  }
  else
  {
    req_params.route_lookup = DATAPATH_LOOKUP;
  }

  /*-----------------------------------------------------------------------
    Request the routing candidate IFACE.
  -----------------------------------------------------------------------*/
  ret_val = qmi_wds_route_look_up( qmi_ptr->wds_handle,
                                   &req_params,
                                   &rsp_params,
                                   &qmi_err_code );
  if( 0 > ret_val )
  {
    DSQMH_MSG_ERROR( "QMH LLIF qmi query route by policy failed: "
                     "iface=%d ret=%d qmi=0x%p",
                     iface_inst, ret_val, qmi_err_code);
    result = DSQMH_FAILED;
  }
  else
  {
    result_ptr->priority = rsp_params.priority;
    result_ptr->if_hndl  = rsp_params.iface_handle;
    result_ptr->if_name  = (ps_iface_name_enum_type)rsp_params.tech_name;
    result_ptr->rm_hndl  = (rsp_params.qmi_inst_is_valid)?
                           (int)rsp_params.qmi_inst : DSQMH_INVALID_RMNET;
    acl_pi_ptr->proc_id  = proc_id;

    PRINT_MSG_6( LOG_MSG_INFO1_LEVEL,
                 "QMH LLIF qmi query route by policy: "
                 "Um-iface=0x%x rmnet=%d priority=%d proc=%d",
                 result_ptr->if_hndl, result_ptr->rm_hndl,
                 result_ptr->priority, acl_pi_ptr->proc_id, 0, 0 );
  }
  return result;
} /* dsqmhllif_query_route_by_policy() */



/*===========================================================================
FUNCTION DSQMHLLIF_NETPLATFORM_UP_CMD

DESCRIPTION
  This function commands the Network Platform layer to establish a
  connection instance.  The results of the mode processing are posted
  asynchronously via the callback registered with Platform layer.

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
)
{
  dsqmh_iface_cblk_type   *cblk_ptr = NULL;   /* Pointer to control block  */
  int ret_val;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_MED( "QMH LLIF net platform up:%d", iface_inst, 0, 0 );

  /*-----------------------------------------------------------------------
    Command Network Platform Layer to bring up connection for IFACE.
  -----------------------------------------------------------------------*/
  cblk_ptr = DSQMH_GET_CBLK_PTR( iface_inst );
  ret_val = ds_qmh_netplat_bringup( cblk_ptr->netplat_info.conn_id,
                                    dsqmhllif_netplat_cmd_cb,
                                    (void*)iface_inst );
  if( 0 > ret_val )
  {
    DSQMH_MSG_ERROR( "QMH LLIF failed on netplat bringup:%d", iface_inst,0,0 );
    return DSQMH_FAILED;
  }

  return DSQMH_SUCCESS;

} /* dsqmhllif_netplatform_up_cmd() */



/*===========================================================================
FUNCTION DSQMHLLIF_NETPLATFORM_DOWN_CMD

DESCRIPTION
  This function commands the Network Platform layer to terminate a
  connection instance for an active connection.  The results of the
  mode processing are posted asynchronously via the callback
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
)
{
  int ret_val;
  dsqmh_iface_cblk_type *cblk_ptr = NULL;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_LOW( "QMH LLIF net platform down:%d", iface_inst, 0, 0 );

  cblk_ptr = DSQMH_GET_CBLK_PTR( iface_inst );

  /*-----------------------------------------------------------------------
    Command Network Platform Layer to teardown connection for IFACE.
  -----------------------------------------------------------------------*/
  ret_val = ds_qmh_netplat_teardown( cblk_ptr->netplat_info.conn_id,
                                     dsqmhllif_netplat_cmd_cb,
                                     (void*)iface_inst );
  if( 0 > ret_val )
  {
    DSQMH_MSG_ERROR( "QMH LLIF failed on netplat teardown:%d", iface_inst,0,0 );
    return DSQMH_FAILED;
  }

  return DSQMH_SUCCESS;

} /* dsqmhllif_netplatform_down_cmd() */



/*===========================================================================
FUNCTION DSQMHLLIF_NETPLATFORM_QOS_REQUEST_CMD

DESCRIPTION
  This function commands the Network Platform layer to establish a new
  QOS flow for an active connection.  The results of the mode
  processing are posted asynchronously via the callback registered
  with Platform layer.

PARAMETERS
  iface_inst  - Index for iface table
  flow_ptr    - Pointer to PS flow

DEPENDENCIES
  dsqmhllif_init_netplatform() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_netplatform_qos_request_cmd
(
  uint32          iface_inst,                   /* Index for iface table   */
  ps_flow_type   *flow_ptr                      /* Pointer to PS flow      */
)
{
  int ret_val;
  dsqmh_iface_cblk_type *cblk_ptr = NULL;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( flow_ptr, "DSQMH LLIF null flow pointer passed" );

  DSQMH_MSG_LOW( "QMH LLIF net platform qos request:%d", iface_inst, 0, 0 );

  cblk_ptr = DSQMH_GET_CBLK_PTR( iface_inst );

  /*-----------------------------------------------------------------------
    Command Network Platform Layer to create new QOS flow.
  -----------------------------------------------------------------------*/
  ret_val = ds_qmh_netplat_qos_request( cblk_ptr->netplat_info.conn_id,
                                        dsqmhllif_netplat_cmd_cb,
                                        (void*)flow_ptr );
  if( 0 > ret_val )
  {
    DSQMH_MSG_ERROR( "QMH LLIF failed on netplat qos request:%d", iface_inst,0,0 );
    return DSQMH_FAILED;
  }

  return DSQMH_SUCCESS;

} /* dsqmhllif_netplatform_qos_request_cmd() */



/*===========================================================================
FUNCTION DSQMHLLIF_NETPLATFORM_QOS_RELEASE_CMD

DESCRIPTION
  This function commands the Network Platform layer to delete an
  existing QOS flow for an active connection.  The results of the mode
  processing are posted asynchronously via the callback registered
  with Platform layer.

PARAMETERS
  iface_inst  - Index for iface table
  flow_ptr    - Pointer to PS flow

DEPENDENCIES
  dsqmhllif_init_netplatform() must be been already invoked.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int dsqmhllif_netplatform_qos_release_cmd
(
  uint32          iface_inst,                   /* Index for iface table   */
  ps_flow_type   *flow_ptr                      /* Pointer to PS flow      */
)
{
  dsqmh_iface_cblk_type   *cblk_ptr = NULL;   /* Pointer to control block  */
  int ret_val;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( flow_ptr, "DSQMH LLIF null flow pointer passed" );

  DSQMH_MSG_LOW( "QMH LLIF net platform qos release:d", iface_inst, 0, 0 );

  /*-----------------------------------------------------------------------
    Command Network Platform Layer to delete existing QOS flow.
  -----------------------------------------------------------------------*/
  cblk_ptr = DSQMH_GET_CBLK_PTR( iface_inst );
  ret_val = ds_qmh_netplat_qos_release( cblk_ptr->netplat_info.conn_id,
                                        0,               /* Default flow */
                                        dsqmhllif_netplat_cmd_cb,
                                        (void*)flow_ptr );
  if( 0 > ret_val )
  {
    DSQMH_MSG_ERROR( "QMH LLIF failed on netplat qos release:%d", iface_inst,0,0 );
    return DSQMH_FAILED;
  }

  return DSQMH_SUCCESS;

} /* dsqmhllif_netplatform_qos_release_cmd() */



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
  ps_iface_bearer_technology_type         *info_ptr
)
{
  DSQMH_ASSERT( current_nw, "DSQMH LLIF null current_nw pointer passed" );
  DSQMH_ASSERT( rat_mask, "DSQMH LLIF null rat_mask pointer passed" );
  DSQMH_ASSERT( so_mask, "DSQMH LLIF null so_mask pointer passed" );
  DSQMH_ASSERT( info_ptr, "DSQMH LLIF null info pointer passed" );

  DSQMH_MSG_LOW( "QMH LLIF decode bearer tech", 0,0,0 );

  info_ptr->current_network = (ps_iface_network_type)*current_nw;
  switch( info_ptr->current_network )
  {
    case PS_IFACE_NETWORK_CDMA:
      info_ptr->data.cdma_type.rat_mask = (uint32)rat_mask->cdma_rat_mask;
      if( PS_IFACE_CDMA_1X == info_ptr->data.cdma_type.rat_mask )
      {
        info_ptr->data.cdma_type.so_mask = (uint32)so_mask->so_mask_1x;
      }
      else
      {
        info_ptr->data.cdma_type.so_mask = (uint32)so_mask->so_mask_evdo_reva;
      }
      break;

    case PS_IFACE_NETWORK_UMTS:
      info_ptr->data.cdma_type.rat_mask = (uint32)rat_mask->umts_rat_mask;
      break;

    case PS_IFACE_NETWORK_MIN:  /* No service */
      memset((void*)&info_ptr->data, 0x0, sizeof(info_ptr->data));
      break;

    default:
      DSQMH_MSG_ERROR( "QMH SM unsupported network:%d",
                       info_ptr->current_network,0,0 );
      return DSQMH_FAILED;
  }

  return DSQMH_SUCCESS;
} /* dsqmhllif_decode_bearer_tech () */


/*===========================================================================
FUNCTION DSQMHLLIF_DEINIT

DESCRIPTION
  This function de-initializes the lower-layer interface module.

PARAMETERS
  None.

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/
void dsqmhllif_deinit( void )
{
  DSQMH_MSG_LOW( "QMH LLIF deinit", 0, 0, 0 );
  
  /* Release QMI Message Library */
  if( QMI_INVALID_CLIENT_HANDLE != dsqmh_state_info.qmi_lib_hndl )
  {
    if( 0 != qmi_release( dsqmh_state_info.qmi_lib_hndl ) )
    {
      DSQMH_MSG_ERROR( "Failed on qmi_release",0,0,0 );
    }
  }
}


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
void dsqmhllif_init( void )
{
  dsqmh_msg_buf_type      *msg_ptr = NULL;    /* Pointer to message buffer */
#ifndef FEATURE_DSS_LINUX
#ifdef FEATURE_DATA_PS_IPV6
  nv_item_type             nv_item;
  nv_stat_enum_type        status;
  ip6_sm_config_type      *config_ptr = NULL;
#endif /* FEATURE_DATA_PS_IPV6 */
#endif /* FEATURE_DSS_LINUX */


  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_LOW( "QMH LLIF init", 0, 0, 0 );
  
#ifdef FEATURE_DSS_LINUX
  atexit( dsqmhllif_deinit );
#endif /* FEATURE_DSS_LINUX */
  
  /*-----------------------------------------------------------------------
    Initialize the SMD port resource critical section
  -----------------------------------------------------------------------*/
  PS_INIT_CRIT_SECTION( &dsqmhllif_smd_crit_section );

  /*-----------------------------------------------------------------------
    Initialize the Network Platform Layer.
  -----------------------------------------------------------------------*/
  ds_qmh_netplat_init();

#ifndef FEATURE_DSS_LINUX
#ifdef FEATURE_DATA_PS_IPV6
  /*-------------------------------------------------------------------------
    Determine if IPv6 is enabled in this build.
  -------------------------------------------------------------------------*/
  status = dcc_get_nv_item( NV_IPV6_ENABLED_I, &nv_item );

  if( status == NV_DONE_S)
  {
    dsqmh_state_info.ipv6_info.enabled = nv_item.ipv6_enabled;
  }
  else if( status != NV_NOTACTIVE_S )
  {
    DSQMH_MSG_ERROR( "QMH Bad NV_IPV6_ENABLED read status: %d", status, 0, 0 );
    dsqmh_state_info.ipv6_info.enabled = FALSE;
  }

  if( dsqmh_state_info.ipv6_info.enabled )
  {
    /*-----------------------------------------------------------------------
      Initialize the IP6 State Machine configuration parameters
    -----------------------------------------------------------------------*/
    /*lint -e(64) */
    status = dcc_get_nv_item( NV_IPV6_SM_CONFIG_I, &nv_item);
    if( NV_DONE_S != status )
    {
      /* Use defaults if NV read fails */
      nv_item.ipv6_sm_config.init_sol_delay   =
        IP6_SM_DEFAULT_INIT_SOL_DELAY;
      nv_item.ipv6_sm_config.sol_interval     =
        IP6_SM_DEFAULT_SOL_INTERVAL;
      nv_item.ipv6_sm_config.resol_interval   =
        IP6_SM_DEFAULT_RESOL_INTERVAL;
      nv_item.ipv6_sm_config.max_sol_attempts =
        IP6_SM_DEFAULT_MAX_SOL_ATTEMPTS;
      nv_item.ipv6_sm_config.max_resol_attempts =
        IP6_SM_DEFAULT_MAX_RESOL_ATTEMPTS;
      nv_item.ipv6_sm_config.pre_ra_exp_resol_time =
        IP6_SM_DEFAULT_PRE_RA_EXP_RESOL_TIME;
      
      /* Writeback defaults if NV item was uninitialized. */
      if( NV_NOTACTIVE_S == status )
      {
        /*lint -e(64) */
        status = dcc_put_nv_item( NV_IPV6_SM_CONFIG_I, &nv_item);
        if( NV_DONE_S != status )
        {
          DSQMH_MSG_ERROR( "QMH LLIF could not write IPV6 SM defaults to NV ",
                           0, 0, 0 );
        }
      }
    }
    
    config_ptr = &dsqmh_state_info.ipv6_info.sm_config;

    if( 0 < nv_item.ipv6_sm_config.init_sol_delay )
    {
        config_ptr->init_sol_delay =
          nv_item.ipv6_sm_config.init_sol_delay;
    }
    else
    {
        config_ptr->init_sol_delay =
          IP6_SM_DEFAULT_INIT_SOL_DELAY;
    }

    if( 0 < nv_item.ipv6_sm_config.sol_interval )
    {
        config_ptr->sol_interval =
          nv_item.ipv6_sm_config.sol_interval;
    }
    else
    {
        config_ptr->sol_interval =
          IP6_SM_DEFAULT_SOL_INTERVAL;
    }

    if( 0 < nv_item.ipv6_sm_config.resol_interval )
    {
        config_ptr->resol_interval =
          nv_item.ipv6_sm_config.resol_interval;
    }
    else
    {
        config_ptr->resol_interval =
          IP6_SM_DEFAULT_RESOL_INTERVAL;
    }
    
    if( 0 < nv_item.ipv6_sm_config.max_sol_attempts )
    {
        config_ptr->max_sol_attempts =
          nv_item.ipv6_sm_config.max_sol_attempts;
    }
    else
    {
       config_ptr->max_sol_attempts =
         IP6_SM_DEFAULT_MAX_SOL_ATTEMPTS;
    }

    if( 0 < nv_item.ipv6_sm_config.max_resol_attempts )
    {
    config_ptr->max_resol_attempts =
      nv_item.ipv6_sm_config.max_resol_attempts;
    }
    else
    {
        config_ptr->max_resol_attempts =
          IP6_SM_DEFAULT_MAX_RESOL_ATTEMPTS;
    }

    if( 0 < nv_item.ipv6_sm_config.pre_ra_exp_resol_time )
    {
    config_ptr->pre_ra_exp_resol_time =
      nv_item.ipv6_sm_config.pre_ra_exp_resol_time;
    }
    else
    {
        config_ptr->pre_ra_exp_resol_time =
          IP6_SM_DEFAULT_PRE_RA_EXP_RESOL_TIME;
    }   

  }
#endif /* FEATURE_DATA_PS_IPV6 */
#endif /* FEATURE_DSS_LINUX */
  /*-----------------------------------------------------------------------
    Post message to host task to initialize the QMI Message Library.
    This is done asynchronously to ensure the host task is in its
    main loop to handle the initial QMI message exchange during
    Library initialization process.
  -----------------------------------------------------------------------*/
  DSQMH_GET_MSG_BUF( msg_ptr );
  if( NULL != msg_ptr )
  {
    memset((void*)msg_ptr, 0x0, sizeof(dsqmh_msg_buf_type));
    msg_ptr->msg_id = PROXY_QMI_LIB_INIT_CMD;

    DSQMH_MSG_MED( "Posting DS cmd: PROXY_QMI_LIB_INIT_CMD",0,0,0);
    DSQMH_PUT_MSG_BUF( msg_ptr );
  }
  else
  {
    DSQMH_MSG_FATAL( "Failed to allocate msg buffer, dropping msg",0,0,0 );
  }  

} /* dsqmhllif_init */

#endif /* FEATURE_DATA_PS && FEATURE_QMI_CLIENT */
