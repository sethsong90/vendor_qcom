/*===========================================================================


                       Q M I   M O D E   H A N D L E R

                C O M M A N D  &  E V E N T   H A N D L E R S
                       
GENERAL DESCRIPTION
  This file contains function implementations for the QMI Proxy IFACE
  command and event handlers.

EXTERNALIZED FUNCTIONS

INITIALIZATION AND SEQUENCING REQUIREMENTS
  dsqmh_hdlr_init() should be called at startup. 

Copyright (c) 2008 - 2012 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

  $Header: //source/qcom/qct/modem/datahlos/interface/qmiifacectls/rel/11.01.01/src/ds_qmh_hdlr.c#6 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
06/22/11    bd     Fix for DoS  bundled packet when starting the call without 
                   Traffic Channel setup. Migrating CL 1674460
06/21/11    sm     Added support to retreive packet data statistics
04/13/11    hm     Multi-modem support merged from linux QMH
10/19/10    sy     Replaced DCC cmd bufs with client allocated memory.
10/04/10    sy     Init qos_supported to DSQMH_QOS_SUPPORT_UNKNOWN
07/02/10    hm     Add support for Fusion target.
05/24/10    hm     Support QOS_CONFIGURE opcode with QOS_REQUEST_EX
09/30/09    ar     Add IPV6 ND control block for ICMPv6 logic
02/19/09    am     DS Task De-coupling effort and introduction of DCC task.
01/21/09    ar     Move routing callback parameter list into structure
01/09/09    ar     Added IPV6 address support.
07/21/08    ar     Code review action items.
05/06/08    ar     Created module/initial version.

===========================================================================*/


/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"

#if defined (FEATURE_DATA_PS) && defined (FEATURE_QMI_CLIENT)

#include "ps_flow.h"
#include "ps_routei_acl.h"
#include "ds_flow_control.h"
#ifdef FEATURE_DATA_PS_IPV6
#include "ps_ifacei_addr_v6.h"
#include "ps_iface_addr_mgmt.h"
#include "ps_icmp6_nd.h"
#endif
#include "ds_qmh_sm_int.h"
#include "ds_qmhi.h"
#include "ds_qmh_acl.h"
#include "ds_qmh_llif.h"
#include "ds_qmh_netplat.h"
#include "ds_qmh_ioctl.h"
#include "ds_qmh_hdlr.h"
#include "ds_qmh_config.h"

#ifdef FEATURE_DATA_PS_DATA_LOGGING
#include "ps_logging_defs.h"
#include "ps_iface_logging.h"
#endif /* FEATURE_DATA_PS_DATA_LOGGING */

#include "dcc_task_defs.h"
#include "ps_meta_info.h"

/*===========================================================================

            LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE

This section contains local definitions for constants, macros, types,
variables and other items needed by this module.

===========================================================================*/
#define DSQMH_MAX_DOS_ACK_HANDLES (10)

#define DSQMH_INVALID_IP_IDENTIFIER         0xFFFF
#define DSQMH_INVALID_DOS_ACK_HANDLE    0xFFFFFFFF

static struct dos_ack_handle_to_ip_id_mapping_s
{
  uint16     ip_identifier;
  int32      dos_ack_handle;
} dos_ack_handle_tbl[DSQMH_MAX_DOS_ACK_HANDLES] = { {DSQMH_INVALID_IP_IDENTIFIER, DSQMH_INVALID_DOS_ACK_HANDLE},
                                                  {DSQMH_INVALID_IP_IDENTIFIER, DSQMH_INVALID_DOS_ACK_HANDLE},
                                                  {DSQMH_INVALID_IP_IDENTIFIER, DSQMH_INVALID_DOS_ACK_HANDLE},
                                                  {DSQMH_INVALID_IP_IDENTIFIER, DSQMH_INVALID_DOS_ACK_HANDLE},
                                                  {DSQMH_INVALID_IP_IDENTIFIER, DSQMH_INVALID_DOS_ACK_HANDLE},
                                                  {DSQMH_INVALID_IP_IDENTIFIER, DSQMH_INVALID_DOS_ACK_HANDLE},
                                                  {DSQMH_INVALID_IP_IDENTIFIER, DSQMH_INVALID_DOS_ACK_HANDLE},
                                                  {DSQMH_INVALID_IP_IDENTIFIER, DSQMH_INVALID_DOS_ACK_HANDLE},
                                                  {DSQMH_INVALID_IP_IDENTIFIER, DSQMH_INVALID_DOS_ACK_HANDLE},
                                                  {DSQMH_INVALID_IP_IDENTIFIER, DSQMH_INVALID_DOS_ACK_HANDLE}};


#define DSQMH_MAX_IPV6_DAD_RETRIES (1)


/*===========================================================================

                    INTERNAL FUNCTION DEFINITIONS

===========================================================================*/

/* Forward prototype. */
LOCAL int dsqmhhdlr_flow_ioctl_cmd
(
  ps_flow_type            *flow_ptr,
  ps_flow_ioctl_type       ioctl_name,
  void                    *argval_ptr,
  int16                   *ps_errno
);

/*===========================================================================
FUNCTION  DSQMHHDLR_GET_DOS_ACK_HANDLE

DESCRIPTION
  This function gets the DOS ACK handle from IP identifier. 

PARAMETERS
  ip_identifier - IP identifier.

DEPENDENCIES
  The IP identifier/DOS Ack handle pair are removed from map. 

RETURN VALUE
  DOS ack handle if entry is found
  0 if not found if no entry is found.  

SIDE EFFECTS
  DOS ACK handle entry is removed from the table. 
===========================================================================*/
int32 dsqmhhdlr_get_dos_ack_handle
(
  uint16 ip_identifier 
)
{
  uint32   index;
  int32    dos_ack_handle = 0; 
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  DSQMH_MSG_LOW ("Get dos ack handle for IP id 0x%x", ip_identifier, 0, 0);

  PS_ENTER_CRIT_SECTION (&global_ps_crit_section);
  for (index = 0; index < DSQMH_MAX_DOS_ACK_HANDLES; index++)
  {
    if (ip_identifier == dos_ack_handle_tbl[index].ip_identifier)
    {
      dos_ack_handle = dos_ack_handle_tbl[index].dos_ack_handle;

      /* Reset the table entry */
      dos_ack_handle_tbl[index].ip_identifier  = DSQMH_INVALID_IP_IDENTIFIER;
      dos_ack_handle_tbl[index].dos_ack_handle = DSQMH_INVALID_DOS_ACK_HANDLE;
      
      break;
    }
  }

  PS_LEAVE_CRIT_SECTION (&global_ps_crit_section);

  DSQMH_MSG_MED ("Get dos ack handle 0x%x, for IP id 0x%x", 
                 dos_ack_handle, ip_identifier, 0);

  return dos_ack_handle;

} /* dsqmhhdlr_get_dos_ack_handle() */

/*===========================================================================
FUNCTION  DSQMHHDLR_SET_DOS_ACK_HANDLE

DESCRIPTION
  Retrieve the IP identifier from the packet info. Replace the DOS
  ACK handle in pkt meta info with IP identifier.  

PARAMETERS
  pkt_chain_ptr - Pointer to DSM chain containing IP packet.
  meta_info_ptr - Meta info.

DEPENDENCIES
  None. 

RETURN VALUE
  None. 

SIDE EFFECTS
  Packet meta info is updated.  
===========================================================================*/
void dsqmhhdlr_set_dos_ack_handle
(
  dsm_item_type             *pkt_chain_ptr,
  ps_meta_info_type         *meta_info_ptr
)
{
  int32                      index;
  int32                      dos_ack_handle;
  uint16                     len;
  uint8                      ip_version = 0;
  uint16                     ip_identifier = 0;
  uint16                     offset = 0; 
  ps_pkt_meta_info_type     *pkt_info_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
#ifdef FEATURE_DSS_LINUX

  DSQMH_MSG_HIGH ("dsqmhhdlr_set_dos_ack_handle: currently "
                  "not supported on LA/E",0, 0, 0);
  return;
#else

  if (NULL == pkt_chain_ptr || NULL == meta_info_ptr)
  {
    DSQMH_MSG_ERROR ("NULL args, pkt ptr %p, meta info %p", 
                      pkt_chain_ptr, meta_info_ptr, 0);
    return;
  }

  pkt_info_ptr = PS_META_GET_PKT_META_INFO_PTR(meta_info_ptr);
  if (NULL == pkt_info_ptr)
  {
    DSQMH_MSG_ERROR ("NULL args, pkt info ptr %p", pkt_info_ptr, 0, 0);
    return;
  }

  /*-----------------------------------------------------------------------
    Extract the first byte from the packet to determine IP version
  -----------------------------------------------------------------------*/
  len = dsm_extract (pkt_chain_ptr, 
                     offset, 
                     (void *) &ip_version,
                     1);
  if (1 != len)
  {
    DSQMH_MSG_ERROR ("Can't extract IP version", 0, 0, 0);
    return;
  }

  /*-----------------------------------------------------------------------
    Just Need the higher order four bits of first byte of the IP header
  -----------------------------------------------------------------------*/
  ip_version >>= 4;

  if ((uint8) IP_V4 == ip_version)
  {
    offset += 4;
    len = dsm_extract (pkt_chain_ptr,
                       offset,
                       (void *) &ip_identifier,
                       2);
    if (2 != len)
    {
      DSQMH_MSG_ERROR ("Cant extract IP identifier", 0, 0, 0);
      return;
    }
  }
  else if ((uint8)IP_V6 == ip_version)
  {
    DSQMH_MSG_ERROR ("IPv6 Dos ack not supported yet", 0, 0, 0);
    return;
  }

  /*-----------------------------------------------------------------------
    Extract DOS ACK handle from pkt meta info. 
    Store the IP identifier as DOS ack handle in the pkt meta info.
  -----------------------------------------------------------------------*/
  dos_ack_handle = PS_PKT_META_GET_DOS_ACK_HANDLE(pkt_info_ptr);
  PS_PKT_META_SET_DOS_ACK_HANDLE(pkt_info_ptr, (int32)ip_identifier);

  PS_ENTER_CRIT_SECTION (&global_ps_crit_section);
  /*-----------------------------------------------------------------------
    Store the mapping of <ip_identifier, dos_ack_handle>.
    Ensure same IP identifier does not exist in the table. 
  -----------------------------------------------------------------------*/
  for (index = 0; index < DSQMH_MAX_DOS_ACK_HANDLES; index++)
  {
    if (ip_identifier == dos_ack_handle_tbl[index].ip_identifier)
    {
      DSQMH_MSG_ERROR ("IP identifier 0x%x already exists in dos ack tbl",
                       ip_identifier, 0, 0);
      PS_LEAVE_CRIT_SECTION (&global_ps_crit_section);
      return;
    }
  }

  for (index = 0; index < DSQMH_MAX_DOS_ACK_HANDLES; index++)
  {
    if (DSQMH_INVALID_IP_IDENTIFIER == 
          dos_ack_handle_tbl[index].ip_identifier &&
        DSQMH_INVALID_DOS_ACK_HANDLE == 
          (unsigned int)dos_ack_handle_tbl[index].dos_ack_handle)
    {
      DSQMH_MSG_MED ("Put dos ack handle 0x%x for IP id 0x%x at index %d",
       	              dos_ack_handle, ip_identifier, index);
      dos_ack_handle_tbl[index].dos_ack_handle = dos_ack_handle;
      dos_ack_handle_tbl[index].ip_identifier  = ip_identifier;
      break;
    }
  }
  PS_LEAVE_CRIT_SECTION (&global_ps_crit_section);

  if (index == DSQMH_MAX_DOS_ACK_HANDLES)
  {
    DSQMH_MSG_HIGH (" DOS ACK HANDLE not cached, No empty slot available", 
                     0, 0, 0);	  
  }
  return;
#endif

} /* dsqmhhdlr_set_dos_ack_handle() */

/*===========================================================================
FUNCTION      DSQMHHDLR_IFACE_LINGER_CMD

DESCRIPTION
  Called when iface goes lingering.

PARAMETERS
  iface_ptr     - Pointer to ps_iface
  info_ptr      - not used

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  None
===========================================================================*/
LOCAL int dsqmhhdlr_iface_linger_cmd
(
  ps_iface_type     *iface_ptr,
  void              *info_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  DSQMH_ASSERT( iface_ptr, "DSQMH HDLR null iface pointer passed" );

  return DSQMH_SUCCESS;
}

/*===========================================================================
FUNCTION  DSQMHHDLR_IFACE_IS_LINGER_SUPPORTED

DESCRIPTION
  This function checks if provided iface supports linger feature

PARAMETERS

  iface_ptr         - Pointer to ps_iface

DEPENDENCIES
  iface_ptr shall be valid
  ps_iface_name_enum_type

RETURN VALUE
  TRUE provided iface supports linger
  FALSE provided iface does not support linger

SIDE EFFECTS
  None.
===========================================================================*/
boolean dsqmhhdlr_iface_is_linger_supported
(
  ps_iface_type  *iface_ptr
)
{
  boolean result = FALSE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if( CDMA_SN_IFACE == iface_ptr->name ||
      UMTS_IFACE == iface_ptr->name)
  {
    result = TRUE;
  }

  return result;
}

#ifndef FEATURE_DSS_LINUX
/*===========================================================================
FUNCTION  DSQMHHDLR_RX_SIG_HDLR

DESCRIPTION
  This function processes the RX task signal set by the IFACE phys
  link watermark queue when it goes non-empty.

PARAMETERS
  sig              - Signal to be processed
  user_data_ptr    - Parameter carrying user data

DEPENDENCIES
  None.

RETURN VALUE
  TRUE if signal should be cleared from the set signal mask.
  FALSE if further signal processing is needed and hence signal
  should not be cleared.

SIDE EFFECTS
  None.
===========================================================================*/
/* ARGSUSED */
LOCAL boolean dsqmhhdlr_rx_sig_hdlr
(
  ps_sig_enum_type sig,                    /* Signal to be processed       */
  void *user_data_ptr                      /* Parameter carrying user data */
)
{
  uint32               iface_inst;              /* Index for iface table   */
  uint32               phys_link;               /* Index for phys link list*/
  dsm_item_type       *pkt_ptr = NULL;          /* DSM packet pointer      */
  ps_iface_type       *iface_ptr = NULL;        /* Proxy IFACE pointer     */
  ps_phys_link_type   *phys_link_ptr = NULL;    /* Primary physlink pointer*/
  dsqmh_smd_info_type *smd_info_ptr = NULL;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Determine the watermark queue.  The passed user_data_ptr is encoded with
    IFACE and PHYS LINK identifiers.
  -------------------------------------------------------------------------*/
  DSQMH_DECODE_IFACE_PHYSLINK_ID( iface_inst, phys_link, user_data_ptr );
  if(! ((DSQMH_MAX_PS_IFACES > iface_inst) &&
        (DSQMH_MAX_PHYSLINKS_PER_IFACE > phys_link)) )
  {
    DSQMH_MSG_ERROR("QMH HDLR Error decoding iface,physlink: 0x%x 0x%x 0x%x",
                    user_data_ptr, iface_inst, phys_link );
    return FALSE;
  }

                                             
  iface_ptr = DSQMH_GET_IFACE_PTR( iface_inst );
  phys_link_ptr = DSQMH_GET_PHYS_LINK_PTR( iface_inst, phys_link );
  smd_info_ptr = DSQMH_GET_SMD_INFO_PTR( iface_inst, phys_link );
  
 /*-------------------------------------------------------------------------
    Dequeue packet from watermark and post to IP stack
  -------------------------------------------------------------------------*/
  pkt_ptr = dsm_dequeue( &smd_info_ptr->ps_rx_wm );
  if( NULL == pkt_ptr )
  {
    return TRUE;
  }

  DSQMH_MSG_MED( "DSQMH RX SIG:iface=%d physlink=%d", iface_inst, phys_link, 0 );
  /*-----------------------------------------------------------------------
    Verify state of PS IFACE
  -----------------------------------------------------------------------*/
  if( !PS_IFACE_IS_VALID( iface_ptr ) ) 
  {
    DSQMH_MSG_ERROR( "DSQMH RX SIG iface_ptr invalid", 0, 0, 0 );
    dsm_free_packet(&pkt_ptr);
    return FALSE;
  }
  
  if(!((IFACE_UP          == ps_iface_state( iface_ptr )) ||
       (IFACE_CONFIGURING == ps_iface_state( iface_ptr )) ||
       (IFACE_ROUTEABLE   == ps_iface_state( iface_ptr ))  )
    )
  {
    DSQMH_MSG_ERROR( "DSQMH RX SIG invalid iface state for inst: %d",
                     iface_inst,0,0 );
    dsm_free_packet(&pkt_ptr);
    return FALSE;
  }

  /* Send packet to IP stack.  There is no meta info for RX packets. */
  (void)ps_phys_link_input( phys_link_ptr, &pkt_ptr, NULL );
 
  return FALSE;
} /* dsqmhhdlr_rx_sig_hdlr() */
#endif /* FEATURE_DSS_LINUX */


/*===========================================================================
FUNCTION DSQMHHDLR_TX_CMD_CB()

DESCRIPTION
  Receive TX packet from protocol stack callback.  This function frees
  the metadata memory that is passed to it (meta_info_ptr).

PARAMETERS
  iface_ptr     - Ptr to interface that this pkt was received on - and to
                  operate on.
  pkt_chain_ptr - Ptr to chain of dsm items that hold the payload that needs
                  to be transmitted.
  meta_info_ptr - Ptr to dsm item that contains the meta info for this pkt.
  tx_cmd_info   - Info to pass to registered cmd handler

RETURN VALUE
  DSQMH_SUCCESS on operation success.
  DSQMH_FAILED on error condition.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
LOCAL int dsqmhhdlr_tx_cmd_cb
(
  ps_iface_type      *iface_ptr,
  dsm_item_type     **pkt_chain_ptr,
  ps_meta_info_type  *meta_info_ptr,
  void               *tx_cmd_info
)
{
#ifdef FEATURE_DSS_LINUX
  (void) iface_ptr;
  (void) pkt_chain_ptr;
  (void) meta_info_ptr;
  (void) tx_cmd_info;
  DSQMH_MSG_ERROR("dsqmhhdlr_tx_cmd_cb: functionality disabled on Linux",0,0,0);
  return DSQMH_SUCCESS;
#else

  uint32                 iface_inst = (uint32)tx_cmd_info;
  uint32                 physlink = DSQMH_DEFAULT_IFACE_PHYSLINK;
  dsqmh_smd_info_type   *smd_info_ptr = NULL;
  ps_flow_type          *flow_ptr = NULL;   
  int                    result = DSQMH_SUCCESS;

  /* QMI QoS flow header per 80-VF536-2 rev.B.  Default flow ID assumed here. */
  typedef PACKED struct PACKED_POST
  {
     uint8           version;
     uint8           flags;
     uint32          flow_id;
  } qos_hdr_type;

  qos_hdr_type  qos_hdr;

  #define DSQMH_TX_PKT_FLAGS_DOS   (0x01)

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( iface_ptr, "DSQMH HDLR null iface pointer passed" );
  DSQMH_ASSERT( pkt_chain_ptr, "DSQMH HDLR null pkt chain pointer passed" );
  

  if( DS_QMH_TARGET_CONFIG_SVLTE_TYPE_2_FUSION ==
      dsqmh_config_get_target_config() )
  {
    DSQMH_MSG_ERROR( "Data path not supported over Fusion2 configuration:"
                     "Dropping packet!", 0, 0, 0);
    dsm_free_packet( pkt_chain_ptr );
    PS_META_INFO_FREE( &meta_info_ptr );
    return DSQMH_FAILED;
  }


  memset (&qos_hdr, 0, sizeof(qos_hdr));
  qos_hdr.version = 1;

  /*-----------------------------------------------------------------------
    For QOS flows, need to prepend the meta info to the packet.
  -----------------------------------------------------------------------*/
  if( DSQMH_GET_QOSHDR_ENABLED( iface_inst ) )
  {
    /* Check for valid metadata packet. */
    if( meta_info_ptr )
    {
      /* Update flow ID based on meta info. */
      flow_ptr = PS_FLOW_GET_FLOW_FROM_META_INFO( meta_info_ptr );
      qos_hdr.flow_id = (flow_ptr)? (uint32)flow_ptr->client_data_ptr :
                                    PS_IFACE_DEFAULT_FLOW_INST;

      if (NULL != meta_info_ptr->pkt_meta_info_ptr)
      {
        if(0 != ((MSG_EXPEDITE | MSG_FAST_EXPEDITE) & 
                 (PS_META_GET_TX_FLAGS(meta_info_ptr))))
        {
          qos_hdr.flags = DSQMH_TX_PKT_FLAGS_DOS;

          /*-----------------------------------------------------------------------
            Get the IP identifier associated with this packet and set it in 
            the meta info. Also store that <dos_ack_handle, ip_id> pair, so
            that when WDS DOS ACK indication is received, we have a handle to
            post the event on. 
          -----------------------------------------------------------------------*/
          dsqmhhdlr_set_dos_ack_handle(*pkt_chain_ptr, meta_info_ptr); 
        }
      }

    }
    else
    {
      /* Use default flow. */
      qos_hdr.flow_id = PS_IFACE_DEFAULT_FLOW_INST;
    }

    DSQMH_MSG_MED( "DSQMH TX CB qos hdr: flow=0x%x",qos_hdr.flow_id,0,0 );
      
    if( sizeof(qos_hdr) != dsm_pushdown_packed( pkt_chain_ptr,
                                                (void*)&qos_hdr, 
                                                sizeof(qos_hdr),
                                                DSM_DS_SMALL_ITEM_POOL ) )
    {
      DSQMH_MSG_ERROR( "DSQMH qos hdr prepend failed",0,0,0 );
      result = DSQMH_FAILED;
    }
  }
  
  /*-----------------------------------------------------------------------
    Send packets to datapath transport.
  -----------------------------------------------------------------------*/
  smd_info_ptr = DSQMH_GET_SMD_INFO_PTR( iface_inst, physlink );
  if( NULL != smd_info_ptr )
  {
    DSQMH_MSG_MED( "DSQMH HDLR tx packet: iface=%d flow=0x%x SIOport=%d",
                   iface_inst, qos_hdr.flow_id, smd_info_ptr->port_id );
  
    sio_transmit( smd_info_ptr->stream_id, *pkt_chain_ptr);
  }
  else
  {
    DSQMH_MSG_ERROR( "DSQMH HDLR Invalid SMD pointer, transmit aborted",
                     0,0,0);
  }
  
  PS_META_INFO_FREE( &meta_info_ptr );  

  return result;
#endif /* FEATURE_DSS_LINUX */

} /* dsqmhhdlr_tx_cmd_cb() */



/*===========================================================================
FUNCTION  dsqmhhdlr_policy_routing_hdlr

DESCRIPTION
  This function triggers QMI to perform policy-based routing on the Modem, 
  to return the best candidate IFACE satisfying the passed ACL policy 
  information.  The QMI results include IFACE handle, priority, and 
  possibly RmNet instance if Um IFACE is UP.  These attributes are stored 
  in the Proxy IFACE control block for use in later policy-based routing 
  on the local processor.

PARAMETERS
  params_ptr         - Ptr to route lookup input parameters
  acl_match_ptr      - Ptr to ACL with highest prioity (output)
  result_ptr         - Ptr to detailed routing results
  proc_id            - Modem processor ID for tagging routing results
  
DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  None.
===========================================================================*/
/* ARGSUSED */
LOCAL int dsqmhhdlr_policy_routing_hdlr
(
  ps_route_lookup_params_type * params_ptr,
  acl_type**                    acl_match_ptr,
  acl_rt_result_type*           result_ptr,
  acl_proc_id_type              proc_id          
)
{
  int  ret_val = DSQMH_FAILED;
  int32  iface_inst = 0;
  int32  iface_offset = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( params_ptr, "DSQMH HDLR null params pointer passed" );
  DSQMH_ASSERT( acl_match_ptr, "DSQMH HDLR null acl_match pointer passed" );
  DSQMH_ASSERT( result_ptr, "DSQMH HDLR null result pointer passed" );
  
  DSQMH_MSG_MED ("DSQMH HDLR policy routing, proc %d", proc_id, 0, 0);

  /*-----------------------------------------------------------------------
    Command QMI to do policy-based routing on Modem.
  -----------------------------------------------------------------------*/
  ret_val = dsqmhllif_query_route_by_policy( proc_id,
                                             params_ptr->acl_pi_ptr,
                                             result_ptr );
  if( DSQMH_SUCCESS != ret_val )
  {
    DSQMH_MSG_ERROR("DSQMH HDLR failed route query",0,0,0);
    return ret_val;
  }

  DSQMH_MSG_MED ("DSQMH HDLR qmi routing results: hndl=0x%x prio=%d rmnet=%d",
                 result_ptr->if_hndl,
                 result_ptr->priority,
                 result_ptr->rm_hndl );
  

  iface_inst = 
    dsqmh_config_get_iface_inst_from_rm_handle (result_ptr->rm_hndl,
                                                proc_id);
  if (-1 != iface_inst)
  {
    /* Use Proxy IFACE mapped to iface inst. */
    *acl_match_ptr = DSQMH_GET_ACL_PTR( iface_inst );

    if ( ACL_IF_PTR(*acl_match_ptr)->name != result_ptr->if_name )
    {
      DSQMH_MSG_ERROR("DSQMH HDLR iface doesn't match 0x%x:0x%x invalidate rm_hndl",
                      ACL_IF_PTR(*acl_match_ptr)->name, result_ptr->if_name, 0);
      *acl_match_ptr = NULL;
      result_ptr->rm_hndl = -1;
    }
    else
    {
      DSQMH_MSG_MED( "DSQMH HDLR using inst=%d iface=%p acl_match=%p ",
                     iface_inst, ACL_IF_PTR( *acl_match_ptr ), *acl_match_ptr );
    }
  }
  else
  {
    *acl_match_ptr = NULL;
  }
  
  return DSQMH_SUCCESS;
} /* dsqmhhdlr_policy_routing_hdlr()*/

#define DSQMHHDLR_POLICY_ROUTING_HDLR( modem )                      \
LOCAL int dsqmhhdlr_policy_routing_hdlr##modem                      \
(                                                                   \
  ps_route_lookup_params_type * params_ptr,                         \
  acl_type**                    acl_match_ptr,                      \
  acl_rt_result_type*           result_ptr                          \
)                                                                   \
{                                                                   \
  return dsqmhhdlr_policy_routing_hdlr( params_ptr,                 \
                                        acl_match_ptr,              \
                                        result_ptr,                 \
                                        ACL_PROC_QCTMSM##modem );   \
}
DSQMHHDLR_POLICY_ROUTING_HDLR( 0 );
#ifdef FEATURE_DSS_LINUX_MULTI_MODEM
DSQMHHDLR_POLICY_ROUTING_HDLR( 1 );
#endif /* FEATURE_DSS_LINUX_MULTI_MODEM */



/*===========================================================================
FUNCTION  dsqmhhdlr_qos_create_ps_flow

DESCRIPTION
  This function creates a PS flow using specification passed from caller.

PARAMETERS
  iface_ptr         - Pointer to ps_iface

  phys_link_ptr     - Pointer to IFACE phys link
  
  subset_id         - Subset ID associated with filters in QOS spec
  
  qos_ptr           - Pointer to QOS spec
  
  flow_pptr         - Pointer to PS flow (output)

  ps_errno          - Error code returned in case of failure (Error
                      values are those defined in dserrno.h)

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  None.
===========================================================================*/
LOCAL int dsqmhhdlr_qos_create_ps_flow
(
  ps_iface_type                   *iface_ptr,
  ps_phys_link_type               *phys_link_ptr,
  ps_iface_ipfltr_subset_id_type   subset_id,
  qos_spec_type                   *qos_ptr,
  ps_flow_type                   **flow_pptr,
  int16                           *ps_errno
)
{
  ps_flow_create_param_type  qos_param;
  int ret_val;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  DSQMH_ASSERT( iface_ptr, "DSQMH HDLR null iface pointer passed" );
  DSQMH_ASSERT( phys_link_ptr, "DSQMH HDLR null physlink pointer passed" );
  DSQMH_ASSERT( qos_ptr, "DSQMH HDLR null qos pointer passed" );
  DSQMH_ASSERT( flow_pptr, "DSQMH HDLR null flows pointer passed" );
  DSQMH_ASSERT( ps_errno, "DSQMH HDLR null ps errno pointer passed" );

  DSQMH_MSG_LOW("DSQMH HDLR qos create ps flow:%d",iface_ptr->client_data_ptr,0,0);

  /*-----------------------------------------------------------------------
    Create a new flow with the IFACE on the specified phys link.
  -----------------------------------------------------------------------*/
  qos_param.qos_spec            = qos_ptr;
  qos_param.subset_id           = subset_id;
  qos_param.fltr_priority       = PS_IFACE_IPFLTR_PRIORITY_FCFS;
  qos_param.enable_filtering    = TRUE;
  qos_param.flow_validate_f_ptr = NULL;
  qos_param.fltr_validate_f_ptr = NULL;

  ret_val = ps_iface_create_flow( iface_ptr,
                                  phys_link_ptr,
                                  &qos_param,
                                  flow_pptr,
                                  ps_errno );

  if( DSQMH_SUCCESS != ret_val )
  {
    DSQMH_MSG_ERROR( "DSQMH HDLR error creating PS flow:%d",
                     iface_ptr->client_data_ptr,0,0 );
    return DSQMH_FAILED;
  }

  /*-----------------------------------------------------------------------
    Configure new flow.
  -----------------------------------------------------------------------*/
  (*flow_pptr)->ps_flow_ioctl_f_ptr =  dsqmhhdlr_flow_ioctl_cmd;
  (*flow_pptr)->client_data_ptr = (void*)0; /* QMI flow ID assigned later*/
  
  return DSQMH_SUCCESS;
} /* dsqmhhdlr_qos_create_ps_flow() */



/*===========================================================================
FUNCTION  dsqmhhdlr_qos_request_hdlr

DESCRIPTION
  This function processes the request for a new QOS flow on the
  specified IFACE default phys link.  The PS flow is created in
  callers context to return any error condition synchronously.  An
  asynchronous message is posted to our host task to perform actual
  Modem operations in our task context.

PARAMETERS
  iface_ptr         - Pointer to ps_iface

  argval_ptr        - Pointer to operation specific structure

  ps_errno          - Error code returned in case of failure (Error
                      values are those defined in dserrno.h)

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  None.
===========================================================================*/
LOCAL int dsqmhhdlr_qos_request_hdlr
(
  ps_iface_type           *iface_ptr,
  void                    *argval_ptr,
  int16                   *ps_errno
)
{
#ifdef FEATURE_DATA_PS_QOS
  ps_phys_link_type  *phys_link_ptr = NULL;
  ps_iface_ioctl_qos_request_type*  ioctl_ptr = NULL;
  int ret_val;
  
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( iface_ptr, "DSQMH HDLR null iface pointer passed" );
  DSQMH_ASSERT( argval_ptr, "DSQMH HDLR null ioctl arg pointer passed" );
  DSQMH_ASSERT( ps_errno, "DSQMH HDLR null ps_errno pointer passed" );

  DSQMH_MSG_MED("DSQMH HDLR qos request:%d",iface_ptr->client_data_ptr,0,0);
  
  ioctl_ptr = (ps_iface_ioctl_qos_request_type*)argval_ptr;
  
  /*-------------------------------------------------------------------------
    Verify IFACE in UP state.
  -------------------------------------------------------------------------*/
  if( !PS_IFACE_IS_VALID( iface_ptr ) ) 
  {
    DSQMH_MSG_ERROR( "DSQMH HDLR iface_ptr invalid", 0, 0, 0 );
    *ps_errno = DS_EFAULT;
    return DSQMH_FAILED;
  }

  if( IFACE_UP != ps_iface_state( iface_ptr ) )
  {
    DSQMH_MSG_ERROR("DSQMH HDLR qos request: IFACE not in UP state",0,0,0);
    *ps_errno = DS_EFAULT;
    return DSQMH_FAILED;
  }

  /*-----------------------------------------------------------------------
    Verify state of primary PS Phys Link
  -----------------------------------------------------------------------*/
  phys_link_ptr = PS_IFACE_GET_PHYS_LINK( iface_ptr );

  if( !PS_PHYS_LINK_IS_VALID( phys_link_ptr ) ) 
  {
    DSQMH_MSG_ERROR( "DSQMH HDLR phys_link_ptr invalid", 0, 0, 0 );
    *ps_errno = DS_EFAULT;
    return DSQMH_FAILED;
  }

  /*-----------------------------------------------------------------------
    Create a new flow with the IFACE on the default phys link.
  -----------------------------------------------------------------------*/
  ret_val = dsqmhhdlr_qos_create_ps_flow( iface_ptr,
                                          phys_link_ptr,
                                          ioctl_ptr->subset_id,
                                          ioctl_ptr->qos_ptr,
                                          &ioctl_ptr->flow_ptr,
                                          ps_errno );

  if( DSQMH_SUCCESS != ret_val )
  {
    DSQMH_MSG_ERROR( "DSQMH HDLR error creating PS flow:%d",
                     iface_ptr->client_data_ptr,0,0 );
    return DSQMH_FAILED;
  }
  
  /* Advise upper layer flow is activating */
  (void)ps_flow_activate_cmd( ioctl_ptr->flow_ptr, ps_errno,
                              iface_ptr->client_data_ptr );
  
  /*-----------------------------------------------------------------------
    Command QMI to request a new QOS flow.
    QMI will asynchronously post status of modem operation to trigger 
    change of state.  For now we will remain in the same state.
  -----------------------------------------------------------------------*/
  if( DSQMH_SUCCESS !=
      dsqmhllif_qos_request_cmd( (uint32)iface_ptr->client_data_ptr,
                                 PS_IFACE_IOCTL_QOS_REQUEST_OP,
                                 ioctl_ptr->qos_ptr,
                                 1,
                                 &ioctl_ptr->flow_ptr,
                                 ps_errno ) )
  {
    DSQMH_MSG_ERROR("DSQMH HDLR failed qos request: %d",
                    (uint32)iface_ptr->client_data_ptr,0,0);
    *ps_errno = DS_EFAULT;
    return DSQMH_FAILED;
  }
 
    
  return DSQMH_SUCCESS;

#else

  DSQMH_MSG_ERROR( "DSQMH HDLR qos support not enabled",0,0,0 );
  *ps_errno = DS_EOPNOTSUPP;
  return DSQMH_FAILED;

#endif /* FEATURE_DATA_PS_QOS */
} /* dsqmhhdlr_qos_request_hdlr() */




/*===========================================================================
FUNCTION  dsqmhhdlr_qos_request_ex_hdlr

DESCRIPTION
  This function processes the request for new QOS flows on the
  specified IFACE default phys link.  The PS flows are created in
  callers context to return any error condition synchronously.  An
  asynchronous message is posted to our host task to perform actual
  Modem operations in our task context.

PARAMETERS
  iface_ptr         - Pointer to ps_iface

  argval_ptr        - Pointer to operation specific structure

  ps_errno          - Error code returned in case of failure (Error
                      values are those defined in dserrno.h)

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  None.
===========================================================================*/
LOCAL int dsqmhhdlr_qos_request_ex_hdlr
(
  ps_iface_type           *iface_ptr,
  void                    *argval_ptr,
  int16                   *ps_errno
)
{
#ifdef FEATURE_DATA_PS_QOS
  ps_phys_link_type  *phys_link_ptr = NULL;
  ps_iface_ioctl_qos_request_ex_type*  ioctl_ptr = NULL;
  int ret_val;
  uint8  i,j;
  uint8  flows=0;
  
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( iface_ptr, "DSQMH HDLR null iface pointer passed" );
  DSQMH_ASSERT( argval_ptr, "DSQMH HDLR null ioctl arg pointer passed" );
  DSQMH_ASSERT( ps_errno, "DSQMH HDLR null ps_errno pointer passed" );

  ioctl_ptr = (ps_iface_ioctl_qos_request_ex_type*)argval_ptr;
  
  DSQMH_MSG_MED("DSQMH HDLR qos request ex: iface=%d nspec=%d",
        iface_ptr->client_data_ptr,ioctl_ptr->num_qos_specs,0);
  
  /*-------------------------------------------------------------------------
    Verify IFACE in UP state.
  -------------------------------------------------------------------------*/
  if( !PS_IFACE_IS_VALID( iface_ptr ) ) 
  {
    DSQMH_MSG_ERROR( "DSQMH HDLR iface_ptr invalid", 0, 0, 0 );
    *ps_errno = DS_EFAULT;
    return DSQMH_FAILED;
  }

  if( IFACE_UP != ps_iface_state( iface_ptr ) )
  {
    DSQMH_MSG_ERROR("DSQMH HDLR qos request: IFACE not in UP state",0,0,0);
    *ps_errno = DS_EFAULT;
    return DSQMH_FAILED;
  }

  /*-----------------------------------------------------------------------
    Verify state of primary PS Phys Link
  -----------------------------------------------------------------------*/
  phys_link_ptr = PS_IFACE_GET_PHYS_LINK( iface_ptr );

  if( !PS_PHYS_LINK_IS_VALID( phys_link_ptr ) ) 
  {
    DSQMH_MSG_ERROR( "DSQMH HDLR phys_link_ptr invalid", 0, 0, 0 );
    *ps_errno = DS_EFAULT;
    return DSQMH_FAILED;
  }

  /*-----------------------------------------------------------------------
    Loop over list of QOS flows
  -----------------------------------------------------------------------*/
  for( i=0; i<ioctl_ptr->num_qos_specs; i++)
  {
    /*---------------------------------------------------------------------
      Create a new flow with the IFACE on the default phys link.
    ---------------------------------------------------------------------*/
    ret_val = dsqmhhdlr_qos_create_ps_flow( iface_ptr,
                                            phys_link_ptr,
                                            ioctl_ptr->subset_id,
                                            &ioctl_ptr->qos_specs_ptr[i],
                                            &ioctl_ptr->flows_ptr[i],
                                            ps_errno );

    if( DSQMH_SUCCESS != ret_val )
    {
      DSQMH_MSG_ERROR( "DSQMH HDLR error creating qos flow:%d flow=%d",
                       iface_ptr->client_data_ptr,i,0 );

      /* Treat as transaction, undo any previously created flows */
      for( j=0; j<flows; j++ )
      {
        (void)ps_flow_go_null_cmd( ioctl_ptr->flows_ptr[j], ps_errno, NULL );
        (void)ps_iface_delete_flow( iface_ptr, ioctl_ptr->flows_ptr[j], ps_errno );
      }
      return DSQMH_FAILED;
    }
    flows++;
  }
  
  /*-----------------------------------------------------------------------
    Command QMI to request a new QOS flows.
    QMI will asynchronously post status of modem operation to trigger 
    change of state.  For now we will remain in the same state.
  -----------------------------------------------------------------------*/
  if( DSQMH_SUCCESS !=
      dsqmhllif_qos_request_cmd( (uint32)iface_ptr->client_data_ptr,
                                 ioctl_ptr->opcode,
                                 ioctl_ptr->qos_specs_ptr,
                                 ioctl_ptr->num_qos_specs,
                 ioctl_ptr->flows_ptr,
                 ps_errno ) )
  {
    DSQMH_MSG_ERROR("DSQMH HDLR failed qos request: %d",
                    (uint32)iface_ptr->client_data_ptr,0,0);
    return DSQMH_FAILED;
  }
 
  if (PS_IFACE_IOCTL_QOS_CONFIGURE_OP == ioctl_ptr->opcode)
  {
    /* Advise upper layers flows are configuring */
    for( i=0; i<ioctl_ptr->num_qos_specs; i++)
    {
      (void)ps_flow_configure_cmd( ioctl_ptr->flows_ptr[i], ps_errno,
                                   iface_ptr->client_data_ptr );
    }

  }
  else
  {
    /* Advise upper layer flows are activating */
    for( i=0; i<ioctl_ptr->num_qos_specs; i++)
    {
      (void)ps_flow_activate_cmd( ioctl_ptr->flows_ptr[i], ps_errno,
                                  iface_ptr->client_data_ptr );
    }
  }

  return DSQMH_SUCCESS;

#else

  DSQMH_MSG_ERROR( "DSQMH HDLR qos support not enabled",0,0,0 );
  *ps_errno = DS_EOPNOTSUPP;
  return DSQMH_FAILED;

#endif /* FEATURE_DATA_PS_QOS */
} /* dsqmhhdlr_qos_request_ex_hdlr() */


/*===========================================================================
FUNCTION  dsqmhhdlr_qos_ioctl_hdlr

DESCRIPTION
  This function processes the IOCTL on existing QOS flows for the
  specified IFACE default phys link.  An asynchronous message is
  posted to our host task to perform actual Modem operations in host
  task context.

PARAMETERS
  ioctl_name        - Operation to be performed
  flows_pptr        - Pointer to ps_flow list
  num_flows         - Count of ps_flows in list
  argval_ptr        - Pointer to operation specific structure
  ps_errno          - Error code returned in case of failure (Error
                      values are those defined in dserrno.h)

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  None.
===========================================================================*/
LOCAL int dsqmhhdlr_qos_ioctl_hdlr
(
  ps_flow_ioctl_type       ioctl_name,
  ps_flow_type           **flows_pptr,
  uint8                    num_flows,
  void                    *argval_ptr,
  int16                   *ps_errno
)
{
#ifdef FEATURE_DATA_PS_QOS
  ps_iface_type                         *iface_ptr = NULL;
  uint8               i;  
  dsqmhllif_qos_op    operation;
  ps_flow_ioctl_qos_modify_type         *qosmod_ptr = NULL;
  ps_flow_ioctl_primary_qos_modify_type *pri_qosmod_ptr = NULL;
  ps_flow_modify_param_type              modify_param;
  qos_spec_type                          qos_spec;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( flows_pptr, "DSQMH HDLR null flows pointer passed" );
  DSQMH_ASSERT( ps_errno, "DSQMH HDLR null ps_errno pointer passed" );
  DSQMH_ASSERT( (DSQMH_MAX_QOS_FLOWS>=num_flows), "QOS flow num exceeded" );

  DSQMH_MSG_MED( "DSQMH HDLR qos IOCTL: ioctl=0x%p num=%d list=0x%p",
                 ioctl_name, num_flows, flows_pptr );
  *ps_errno = 0;
  
  DSQMH_GET_IFACE_PTR_FROM_FLOW( *flows_pptr, iface_ptr );
  if( !iface_ptr )
  {
    DSQMH_MSG_ERROR( "QMH HDLR flow iface invalid: 0x%p",
                     *flows_pptr,0,0 );
    return DSQMH_FAILED;
  }

  /*-----------------------------------------------------------------------
    Process the requested operation.
  -----------------------------------------------------------------------*/
  switch( ioctl_name )
  {
    case PS_FLOW_IOCTL_QOS_RELEASE:
    case PS_FLOW_IOCTL_QOS_RELEASE_EX:
      operation = DSQMHLLIF_QOS_OP_RELEASE;
      /* Advise upper layer flows are going null */
      for( i=0; i<num_flows; i++)
      {
        (void)ps_flow_go_null_cmd( flows_pptr[i], ps_errno,
                                   flows_pptr[i]->client_data_ptr );
      }
      break;

    case PS_FLOW_IOCTL_QOS_SUSPEND:
    case PS_FLOW_IOCTL_QOS_SUSPEND_EX:
      operation = DSQMHLLIF_QOS_OP_SUSPEND;
      /* Advise upper layer flows are suspending */
      for( i=0; i<num_flows; i++)
      {
        (void)ps_flow_suspend_cmd( flows_pptr[i], ps_errno,
                                   flows_pptr[i]->client_data_ptr );
      }
      break;

    case PS_FLOW_IOCTL_QOS_RESUME:
    case PS_FLOW_IOCTL_QOS_RESUME_EX:
      operation = DSQMHLLIF_QOS_OP_RESUME;
      /* Advise upper layer flows are resuming */
      for( i=0; i<num_flows; i++)
      {
        (void)ps_flow_resume_cmd( flows_pptr[i], ps_errno,
                                  flows_pptr[i]->client_data_ptr );
      }
      break;

    case PS_FLOW_IOCTL_QOS_MODIFY:
      if( num_flows == 1 )
      {
        qosmod_ptr = (ps_flow_ioctl_qos_modify_type*)argval_ptr;
      
        /* Update PS QOS object. */
        memset((void*)&modify_param, 0x0, sizeof(modify_param));
        modify_param.qos_spec       = qosmod_ptr->qos_ptr;
        modify_param.subset_id      = qosmod_ptr->subset_id;
        modify_param.fltr_priority  = PS_IFACE_IPFLTR_PRIORITY_FCFS;
     
        if( DSQMH_SUCCESS !=
            ps_iface_modify_flow( iface_ptr, flows_pptr[0], &modify_param, ps_errno) )
        {
          DSQMH_MSG_ERROR( "QMH HDLR error modifying PS flow: %d",
                           (uint32)iface_ptr->client_data_ptr,0,0 );
          return DSQMH_FAILED;
        }
        operation = DSQMHLLIF_QOS_OP_MODIFY;
      }
      else
      {
        DSQMH_MSG_ERROR( "QMH HDLR modify supports one flow only",0,0,0 );
        *ps_errno = DS_EFAULT;
        return DSQMH_FAILED;
      }
      break;

    case PS_FLOW_IOCTL_PRIMARY_QOS_MODIFY:
      if( num_flows == 1 )
      {
        /* Check that specified flow is primary on IFACE */
        if( flows_pptr[0] != PS_IFACEI_GET_DEFAULT_FLOW(iface_ptr) )
        {
          DSQMH_MSG_ERROR( "QMH HDLR specified flow not primary on iface: %d",
                           (uint32)iface_ptr->client_data_ptr,0,0 );
          return DSQMH_FAILED;
        }
        
        pri_qosmod_ptr = (ps_flow_ioctl_primary_qos_modify_type*)argval_ptr;
      
        /* Update PS QOS object. */
        memset((void*)&modify_param, 0x0, sizeof(modify_param));
        memset((void*)&qos_spec, 0x0, sizeof(qos_spec));
        modify_param.qos_spec = &qos_spec;
        modify_param.qos_spec->field_mask =
          pri_qosmod_ptr->primary_qos_spec_ptr->field_mask;
        modify_param.qos_spec->tx.flow_template =
          pri_qosmod_ptr->primary_qos_spec_ptr->tx_flow_template;
        modify_param.qos_spec->rx.flow_template =
          pri_qosmod_ptr->primary_qos_spec_ptr->rx_flow_template;
     
        operation = DSQMHLLIF_QOS_OP_MODIFY_PRI;
      }
      else
      {
        DSQMH_MSG_ERROR( "QMH HDLR modify supports one flow only",0,0,0 );
        *ps_errno = DS_EFAULT;
        return DSQMH_FAILED;
      }
      break;

    default:
      DSQMH_MSG_ERROR( "QMH HDLR unsupported IOCTL:%d",ioctl_name,0,0 );
      *ps_errno = DS_EFAULT;
      return DSQMH_FAILED;
  }
  
  if( DSQMH_SUCCESS != dsqmhllif_qos_manager( operation,
                                              (uint32)iface_ptr->client_data_ptr,
                                              flows_pptr,
                                              num_flows,
                                              argval_ptr,
                                              ps_errno ) )
  {
    DSQMH_MSG_ERROR( "DSQMH SM failed qos request: %d",
                     (uint32)iface_ptr->client_data_ptr,0,0 );
    return DSQMH_FAILED;
  }
    
  return DSQMH_SUCCESS;

#else

  DSQMH_MSG_ERROR( "DSQMH HDLR qos support not enabled",0,0,0 );
  *ps_errno = DS_EOPNOTSUPP;
  return DSQMH_FAILED;

#endif /* FEATURE_DATA_PS_QOS */
} /* dsqmhhdlr_qos_ioctl_hdlr() */



/*===========================================================================
FUNCTION      DSQMHHDLR_FLOW_IOCTL_CMD

DESCRIPTION
  Process the IOCTL for specified FLOW.

PARAMETERS
  flow_ptr          - Pointer to ps_flow

  ioctl_name        - The operation name

  argval_ptr        - Pointer to operation specific structure

  ps_errno          - Error code returned in case of failure (Error
                      values are those defined in dserrno.h)

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  None.
===========================================================================*/
LOCAL int dsqmhhdlr_flow_ioctl_cmd
(
  ps_flow_type            *flow_ptr,
  ps_flow_ioctl_type       ioctl_name,
  void                    *argval_ptr,
  int16                   *ps_errno
)
{
  int result = DSQMH_SUCCESS;
  ps_flow_ioctl_qos_release_ex_type   *release_ex_arg_ptr;
  ps_flow_ioctl_qos_suspend_ex_type   *suspend_ex_arg_ptr;
  ps_flow_ioctl_qos_resume_ex_type    *resume_ex_arg_ptr;
  
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_TEST( flow_ptr, "DSQMH HDLR null flows pointer passed" );
  DSQMH_TEST( ps_errno, "DSQMH HDLR null ps_errno pointer passed" );

  DSQMH_MSG_HIGH("DSQMH FLOW ioctl:0x%p flow:0x%p",ioctl_name,flow_ptr,0);

  /*---------------------------------------------------------------------
    Process the IOCTL request.
  ---------------------------------------------------------------------*/
  switch( ioctl_name )
  {
  case PS_FLOW_IOCTL_QOS_RELEASE:
    /*-------------------------------------------------------------------
      Release an existing QOS flow on IFACE default phys link.
    -------------------------------------------------------------------*/
    if( DSQMH_SUCCESS != dsqmhhdlr_qos_ioctl_hdlr( ioctl_name,
                                                   &flow_ptr,
                                                   1,
                                                   argval_ptr,
                                                   ps_errno ) )
    {
      DSQMH_MSG_ERROR( "QMH HDLR failed on PS_FLOW_IOCTL_QOS_RELEASE",0,0,0 );
      result = DSQMH_FAILED;
    }
    break;
    
  case PS_FLOW_IOCTL_QOS_RELEASE_EX:
    /*-------------------------------------------------------------------
      Release existing QOS flows on IFACE default phys link.
    -------------------------------------------------------------------*/
    release_ex_arg_ptr = (ps_flow_ioctl_qos_release_ex_type*)argval_ptr;
    if( DSQMH_SUCCESS != 
        dsqmhhdlr_qos_ioctl_hdlr( ioctl_name,
                                  release_ex_arg_ptr->flows_ptr,
                                  release_ex_arg_ptr->num_flows,
                                  argval_ptr,
                                  ps_errno ) )
    {
      DSQMH_MSG_ERROR( "QMH HDLR failed on PS_FLOW_IOCTL_QOS_RELEASE_EX",0,0,0 );
      result = DSQMH_FAILED;
    }
    break;

  case PS_FLOW_IOCTL_QOS_SUSPEND:
    /*-------------------------------------------------------------------
      Suspend QOS flow on IFACE default phys link.
    -------------------------------------------------------------------*/
    if( DSQMH_SUCCESS != dsqmhhdlr_qos_ioctl_hdlr( ioctl_name,
                                                   &flow_ptr,
                                                   1,
                                                   argval_ptr,
                                                   ps_errno ) )
    {
      DSQMH_MSG_ERROR( "QMH HDLR failed on PS_FLOW_IOCTL_QOS_SUSPEND",0,0,0 );
      result = DSQMH_FAILED;
    }
    break;

  case PS_FLOW_IOCTL_QOS_SUSPEND_EX:
    /*-------------------------------------------------------------------
      Suspend existing QOS flows on IFACE default phys link.
    -------------------------------------------------------------------*/
    suspend_ex_arg_ptr = (ps_flow_ioctl_qos_suspend_ex_type*)argval_ptr;
    if( DSQMH_SUCCESS != dsqmhhdlr_qos_ioctl_hdlr( ioctl_name,
                                                   suspend_ex_arg_ptr->flows_ptr,
                                                   suspend_ex_arg_ptr->num_flows,
                                                   argval_ptr,
                                                   ps_errno ) )
    {
      DSQMH_MSG_ERROR( "QMH HDLR failed on PS_FLOW_IOCTL_QOS_SUSPEND_EX",0,0,0 );
      result = DSQMH_FAILED;
    }
    break;

  case PS_FLOW_IOCTL_QOS_RESUME:
    /*-------------------------------------------------------------------
      Resume QOS flow on IFACE default phys link.
    -------------------------------------------------------------------*/
    if( DSQMH_SUCCESS != dsqmhhdlr_qos_ioctl_hdlr( ioctl_name,
                                                   &flow_ptr,
                                                   1,
                                                   argval_ptr,
                                                   ps_errno ) )
    {
      DSQMH_MSG_ERROR( "QMH HDLR failed on PS_FLOW_IOCTL_QOS_RESUME",0,0,0 );
      result = DSQMH_FAILED;
    }
    break;

  case PS_FLOW_IOCTL_QOS_RESUME_EX:
    /*-------------------------------------------------------------------
      Resume existing QOS flows on IFACE default phys link.
    -------------------------------------------------------------------*/
    resume_ex_arg_ptr = (ps_flow_ioctl_qos_resume_ex_type*)argval_ptr;
    if( DSQMH_SUCCESS != dsqmhhdlr_qos_ioctl_hdlr( ioctl_name,
                                                   resume_ex_arg_ptr->flows_ptr,
                                                   resume_ex_arg_ptr->num_flows,
                                                   argval_ptr,
                                                   ps_errno ) )
    {
      DSQMH_MSG_ERROR( "QMH HDLR failed on PS_FLOW_IOCTL_QOS_RESUME_EX",0,0,0 );
      result = DSQMH_FAILED;
    }
    break;

  case PS_FLOW_IOCTL_QOS_MODIFY:
  case PS_FLOW_IOCTL_PRIMARY_QOS_MODIFY:
    /*-------------------------------------------------------------------
      Modify an existing QOS flow.
    -------------------------------------------------------------------*/
    if( DSQMH_SUCCESS != dsqmhhdlr_qos_ioctl_hdlr( ioctl_name,
                                                   &flow_ptr,
                                                   1,
                                                   argval_ptr,
                                                   ps_errno ) )
    {
      DSQMH_MSG_ERROR( "QMH HDLR failed on PS_FLOW_IOCTL_QOS_MODIFY"
                       "/PS_FLOW_IOCTL_PRIMARY_QOS_MODIFY",0,0,0 );
      result = DSQMH_FAILED;
    }
    break;
    
  case PS_FLOW_IOCTL_PRIMARY_QOS_GET_GRANTED_FLOW_SPEC:
    /*-------------------------------------------------------------------
      Query granted QOS flow specification.
    -------------------------------------------------------------------*/
    if( DSQMH_SUCCESS != dsqmhllif_qos_get_granted_cmd( flow_ptr,
                                                        ioctl_name,
                                                        argval_ptr,
                                                        ps_errno ) )
    {
      DSQMH_MSG_ERROR( "QMH HDLR failed on PS_FLOW_IOCTL_QOS_RESUME_EX",0,0,0 );
      result = DSQMH_FAILED;
    }
    break;

  default:
    /*-------------------------------------------------------------------
      Post to IOCTL dispatch.
    -------------------------------------------------------------------*/
    result =
      dsqmhioctl_flow_dispatcher( flow_ptr, ioctl_name, argval_ptr, ps_errno );
  }
  
  return result;
} /* dsqmhhdlr_flow_ioctl_cmd() */



/*===========================================================================
FUNCTION      DSQMHHDLR_IFACE_UP_CMD

DESCRIPTION
  Send asynchronous command to bring up the Proxy IFACE.

PARAMETERS
  iface_ptr     - Pointer to ps_iface
  info_ptr      - not used

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  None.
===========================================================================*/
LOCAL int dsqmhhdlr_iface_up_cmd
(
  ps_iface_type     *iface_ptr,
  void              *info_ptr
)
{
  dsqmh_msg_buf_type *msg_ptr = NULL;         /* Pointer to message buffer */
  
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void) info_ptr;

  DSQMH_ASSERT( iface_ptr, "DSQMH HDLR null iface pointer passed" );

  DSQMH_MSG_HIGH( "DSQMH IFACE up cmd:%d",
                  (uint32)iface_ptr->client_data_ptr,0,0 );

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
    msg_ptr->msg_id = PROXY_IFACE_BRING_UP_CMD;
    msg_ptr->iface_inst = (uint32)iface_ptr->client_data_ptr; 

    DSQMH_MSG_MED( "Posting cmd: PROXY_IFACE_BRING_UP_CMD",0,0,0); 
    DSQMH_PUT_MSG_BUF( msg_ptr );
  }
  else
  {
    DSQMH_MSG_FATAL( "Failed to allocate msg buffer, dropping msg",0,0,0 );
    return DSQMH_FAILED;
  }

  return DSQMH_SUCCESS;
} /* dsqmhhdlr_iface_up_cmd() */



/*===========================================================================
FUNCTION      DSQMHHDLR_IFACE_DOWN_CMD

DESCRIPTION
  Send asynchronous command to teardown the Proxy IFACE.

PARAMETERS
  iface_ptr     - Pointer to ps_iface
  info_ptr      - not used

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  None.
===========================================================================*/
/* ARGSUSED */
LOCAL int dsqmhhdlr_iface_down_cmd
(
  ps_iface_type     *iface_ptr,
  void              *info_ptr
)
{
  dsqmh_msg_buf_type *msg_ptr = NULL;         /* Pointer to message buffer */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void) info_ptr;

  DSQMH_ASSERT( iface_ptr, "DSQMH HDLR null iface pointer passed" );

  DSQMH_MSG_HIGH( "DSQMH IFACE down cmd:%d",
                  (uint32)iface_ptr->client_data_ptr,0,0 );
  
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
    msg_ptr->msg_id = PROXY_IFACE_TEARDOWN_CMD;
    msg_ptr->iface_inst = (uint32)iface_ptr->client_data_ptr; 

    DSQMH_MSG_MED( "Posting cmd: PROXY_IFACE_TEARDOWN_CMD",0,0,0); 
    DSQMH_PUT_MSG_BUF( msg_ptr );
  }
  else
  {
    DSQMH_MSG_FATAL( "Failed to allocate msg buffer, dropping msg",0,0,0 );
    return DSQMH_FAILED;
  }

  return DSQMH_SUCCESS;
} /* dsqmhhdlr_iface_down_cmd() */



/*===========================================================================
FUNCTION      DSQMHHDLR_IFACE_IOCTL_CMD

DESCRIPTION
  Process the IOCTL for specified IFACE

PARAMETERS
  iface_ptr         - Pointer to ps_iface

  ioctl_name        - The operation name

  argval_ptr        - Pointer to operation specific structure

  ps_errno          - Error code returned in case of failure (Error
                      values are those defined in dserrno.h)

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  None.
===========================================================================*/
LOCAL int dsqmhhdlr_iface_ioctl_cmd
(
  ps_iface_type           *iface_ptr,
  ps_iface_ioctl_type      ioctl_name,
  void                    *argval_ptr,
  sint15                  *ps_errno
)
{
  int result = DSQMH_SUCCESS;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_TEST( iface_ptr, "DSQMH HDLR null iface pointer passed" );
  DSQMH_TEST( argval_ptr, "DSQMH HDLR null ioctl arg pointer passed" );
  DSQMH_TEST( ps_errno, "DSQMH HDLR null ps_errno pointer passed" );

  DSQMH_MSG_MED( "DSQMH IFACE ioctl: iface=%d ioctl=0x%x",
                 iface_ptr->client_data_ptr, ioctl_name, 0 );

  /*---------------------------------------------------------------------
    Process the IOCTL request.
  ---------------------------------------------------------------------*/
  switch( ioctl_name )
  {
  case PS_IFACE_IOCTL_QOS_REQUEST:
    /*-------------------------------------------------------------------
      Request new QOS flow for IFACE default phys link.
    -------------------------------------------------------------------*/
    result = dsqmhhdlr_qos_request_hdlr( iface_ptr, argval_ptr, ps_errno );
    break;
    
  case PS_IFACE_IOCTL_QOS_REQUEST_EX:
    /*-------------------------------------------------------------------
      Request new QOS flows for IFACE default phys link.
    -------------------------------------------------------------------*/
    result = dsqmhhdlr_qos_request_ex_hdlr( iface_ptr, argval_ptr, ps_errno );
    break;

  case PS_IFACE_IOCTL_GO_NULL:
    result = dsqmhhdlr_iface_down_cmd( iface_ptr, NULL );
    break;
            
  default:
    /*-------------------------------------------------------------------
      Post to IOCTL dispatch.
    -------------------------------------------------------------------*/
    result =
      dsqmhioctl_iface_dispatcher( iface_ptr, ioctl_name, argval_ptr, ps_errno );
  }
  
  return result;
} /* dsqmhhdlr_iface_ioctl_cmd() */



/*===========================================================================
FUNCTION      DSQMHHDLR_PHYS_LINK_UP_CMD

DESCRIPTION
  Send asynchronous command to bring up the physical link to Modem processor. 

PARAMETERS
  phys_link_ptr - phys link ptr for the connection
  info_ptr      - phys link command

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  None.
===========================================================================*/
LOCAL int dsqmhhdlr_phys_link_up_cmd
(
  ps_phys_link_type *phys_link_ptr,
  void              *info_ptr
)
{
  dsqmh_msg_buf_type *msg_ptr = NULL;         /* Pointer to message buffer */
  
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( phys_link_ptr, "DSQMH HDLR null physlink pointer passed" );

  DSQMH_MSG_HIGH("DSQMH PHYS LINK up cmd:0x%p",phys_link_ptr,0,0);
  
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
    msg_ptr->msg_id = PROXY_PHYS_LINK_UP_CMD;
    msg_ptr->iface_inst = (uint32)phys_link_ptr->client_data_ptr;
    msg_ptr->phys_link_ptr = phys_link_ptr;
    msg_ptr->info.physlink.cmd = (dsqmh_physlink_cmd_type)((uint32)info_ptr);

    DSQMH_MSG_MED( "Posting cmd: PROXY_PHYS_LINK_UP_CMD",0,0,0); 
    DSQMH_PUT_MSG_BUF( msg_ptr );
  }
  else
  {
    DSQMH_MSG_FATAL( "Failed to allocate msg buffer, dropping msg",0,0,0 );
    return DSQMH_FAILED;
  }

  return DSQMH_SUCCESS;
} /* dsqmhhdlr_phys_link_up_cmd() */



/*===========================================================================
FUNCTION      DSQMHHDLR_PHYS_LINK_DOWN_CMD

DESCRIPTION
  Send asynchronous command to teardown the physical link to Modem
  processor.  Note this DOWN command is used for both GO_NULL and DOWN
  (dormancy) transitions on PS phys_link objects.

PARAMETERS
  phys_link_ptr - phys link ptr for the connection
  info_ptr      - phys link command

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  None.
===========================================================================*/
/* ARGSUSED */
int dsqmhhdlr_phys_link_down_cmd
(
  ps_phys_link_type *phys_link_ptr,
  void              *info_ptr
)
{
  dsqmh_msg_buf_type *msg_ptr = NULL;         /* Pointer to message buffer */
  
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( phys_link_ptr, "DSQMH HDLR null physlink pointer passed" );
  
  DSQMH_MSG_HIGH("DSQMH PHYS LINK down cmd:0x%p",phys_link_ptr,0,0);
  
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
    msg_ptr->msg_id = PROXY_PHYS_LINK_DOWN_CMD;
    msg_ptr->iface_inst = (uint32)phys_link_ptr->client_data_ptr;
    msg_ptr->phys_link_ptr = phys_link_ptr;
    msg_ptr->info.physlink.cmd = (dsqmh_physlink_cmd_type)((uint32)info_ptr);

    DSQMH_MSG_MED( "Posting cmd: PROXY_PHYS_LINK_DOWN_CMD",0,0,0); 
    DSQMH_PUT_MSG_BUF( msg_ptr );
  }
  else
  {
    DSQMH_MSG_FATAL( "Failed to allocate msg buffer, dropping msg",0,0,0 );
    return DSQMH_FAILED;
  }

  return DSQMH_SUCCESS;
} /* dsqmhhdlr_phys_link_down_cmd() */



/*===========================================================================
FUNCTION      DSQMHHDLR_PHYSLINK_FLOWCTL_HDLR

DESCRIPTION
  Process phys link flow control directives.

PARAMETERS
  phys_link_ptr - phys link ptr for the connection
  info_ptr      - not used

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  None.
===========================================================================*/
LOCAL int dsqmhhdlr_physlink_flowctl_hdlr
(
  ps_iface_type     *iface_ptr,
  ps_phys_link_type *phys_link_ptr,
  dsqmh_msg_id_type  cmd
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( iface_ptr, "DSQMH HDLR null iface pointer passed" );
  DSQMH_ASSERT( phys_link_ptr, "DSQMH HDLR null physlink pointer passed" );

  if( PROXY_PHYS_LINK_FLOW_DISABLE_CMD == cmd )
  {
    DSQMH_MSG_MED( "DSQMH PHYS LINK flow disable:0x%p", phys_link_ptr,0,0 );
    ps_phys_link_disable_flow( phys_link_ptr, DS_FLOW_PROXY_MASK );
  }
  else
  {
    DSQMH_MSG_MED( "DSQMH PHYS LINK flow enable:0x%p", phys_link_ptr,0,0 );
    ps_phys_link_enable_flow( phys_link_ptr, DS_FLOW_PROXY_MASK );
  }
  return DSQMH_SUCCESS;
} /* dsqmhhdlr_physlink_flowctl_hdlr() */




/*===========================================================================
FUNCTION    DSQMHHDLR_PHYS_LINK_IOCTL_CMD

DESCRIPTION
  Process the IOCTL for specified phys link.

PARAMETERS
  phys_link_ptr     - Pointer to phys link

  ioctl_name        - The operation name

  argval_ptr        - Pointer to operation specific structure

  ps_errno          - Error code returned in case of failure (Error
                      values are those defined in dserrno.h)

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  None.
===========================================================================*/
LOCAL int dsqmhhdlr_phys_link_ioctl_cmd
(
  ps_phys_link_type        *phys_link_ptr,
  ps_phys_link_ioctl_type  ioctl_name,
  void                     *argval_ptr,
  int16                    *ps_errno
)
{
  int result = DSQMH_SUCCESS;
  ps_iface_type           *iface_ptr = NULL;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( phys_link_ptr, "DSQMH HDLR null physlink pointer passed" );
  DSQMH_ASSERT( ps_errno, "DSQMH HDLR null ps_errno pointer passed" );

  DSQMH_MSG_HIGH( "DSQMH PHYSLINK ioctl: physlink=0x%p ioctl=%d",
                  phys_link_ptr, ioctl_name, 0 );

  iface_ptr = DSQMH_GET_IFACE_PTR( (uint32)phys_link_ptr->client_data_ptr );
  if( !iface_ptr )
  {
    DSQMH_MSG_ERROR( "Invalid iface inst specified: %d",
                     (uint32)phys_link_ptr->client_data_ptr, 0, 0 );
    return DSQMH_FAILED;
  }
  
  /*---------------------------------------------------------------------
    Process the IOCTL request.
  ---------------------------------------------------------------------*/
  switch( ioctl_name )
  {
  case PS_PHYS_LINK_IOCTL_GO_ACTIVE:
    if( DSQMH_SUCCESS !=
        dsqmhllif_dormancy_manager( DSQMHLLIF_DORMANCY_OP_GOACTIVE,
                                    (uint32)iface_ptr->client_data_ptr ) )
    {
      DSQMH_MSG_ERROR( "QMH SM error on PS_PHYS_LINK_IOCTL_GO_ACTIVE",0,0,0 );
      *ps_errno = DS_EFAULT;
      result = DSQMH_FAILED;
    }
    break;                              
      
  case PS_PHYS_LINK_IOCTL_GO_DORMANT:
    if( DSQMH_SUCCESS !=
        dsqmhllif_dormancy_manager( DSQMHLLIF_DORMANCY_OP_GODORMANT,
                                    (uint32)iface_ptr->client_data_ptr ) )
    {
      DSQMH_MSG_ERROR( "QMH SM error on PS_PHYS_LINK_IOCTL_GO_DORMANT",0,0,0 );
      *ps_errno = DS_EFAULT;
      result = DSQMH_FAILED;
    }
    break;
    
  case PS_PHYS_LINK_IOCTL_GET_STATE:
    /*-------------------------------------------------------------------
      Return state of phys link.
    -------------------------------------------------------------------*/
    DSQMH_ASSERT( argval_ptr, "DSQMH HDLR null ioctl arg pointer passed" );
    *(phys_link_state_type*)argval_ptr =
                                 PS_PHYS_LINK_GET_STATE( phys_link_ptr );
    break;
    
  default:
    DSQMH_MSG_ERROR( "QMH SM unsupported physlink IOCTL:%d", ioctl_name,0,0 );
    *ps_errno = DS_EOPNOTSUPP;
    result = DSQMH_FAILED;
  }
  
  return result;
} /* dsqmhhdlr_phys_link_ioctl_cmd() */


/*===========================================================================
FUNCTION  DSQMHHDLR_ASYNC_MSG_DISPATCHER

DESCRIPTION
  This function is the dispatcher for asynchronous messages posted to
  the host task.  Individual messages are passed to internal handlers
  for actual processing.  The passed message buffer memory is freed in
  this function.

PARAMETERS
  user_data_ptr        - Pointer to asynchronous message

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  Message buffer memory is returned to task heap.
===========================================================================*/
void dsqmhhdlr_async_msg_dispatcher
(
  dcc_cmd_enum_type  cmd,
  void             * user_data_ptr
)
{
  dsqmh_msg_buf_type * msg_ptr = NULL;
  uint32               sm_inst;                        /* Index for SM list*/
  stm_status_t         sm_result;
  
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  msg_ptr = (dsqmh_msg_buf_type*)user_data_ptr;
  DSQMH_ASSERT( msg_ptr, "DSQMH MSG_DISPATCHER null user_data_ptr" );    

  /*-------------------------------------------------------------------------
    Validate input parameters. 
  -------------------------------------------------------------------------*/
  if( DCC_QMH_PROXY_IFACE_MSG_CMD != cmd )
  {
    DSQMH_MSG_ERROR( "DSQMH MSG cmd unsupported:%d",cmd,0,0 );
    return;
  }

  /*-------------------------------------------------------------------------
    Dispatch the message to the specific handler.
  -------------------------------------------------------------------------*/
  switch( msg_ptr->msg_id )
  {
  case PROXY_IFACE_MODEM_INIT_IND:
  case PROXY_IFACE_BRING_UP_CMD:
  case PROXY_IFACE_TEARDOWN_CMD:    
  case PROXY_PHYS_LINK_UP_CMD:    
  case PROXY_PHYS_LINK_DOWN_CMD:
  case PROXY_IFACE_MODEM_UP_IND:
  case PROXY_IFACE_MODEM_DOWN_IND:
  case PROXY_IFACE_PLATFORM_UP_IND:
  case PROXY_IFACE_PLATFORM_DOWN_IND:
  case PROXY_IFACE_CONFIGURED_IND:
  case PROXY_IFACE_MODEM_EVENT_IND:
  case PROXY_IFACE_MODEM_QOS_IND:
  case PROXY_IFACE_PLATFORM_QOS_IND:
    DSQMH_MSG_MED( "DSQMH MSG DISPATCHER: supported cmd id=%d iface=%d physlink=0x%p",
                   msg_ptr->msg_id,msg_ptr->iface_inst,msg_ptr->phys_link_ptr );

    /*-----------------------------------------------------------------------
      Inject message into state machine instance.
    -----------------------------------------------------------------------*/
    sm_inst = msg_ptr->iface_inst;
    sm_result = stm_instance_process_input( NULL,
                                            DSQMH_SM,
                                            sm_inst,
                                            (stm_input_t)msg_ptr->msg_id,
                                            (void*)msg_ptr );
    if( STM_SUCCESS !=  sm_result )
    {
      if( STM_ENOTPROCESSED == sm_result )
      {
        DSQMH_MSG_MED( "DSQMH SM msg not processed: msg=%d",
                       msg_ptr->msg_id,0,0 );
      }
      else
      {
        DSQMH_MSG_ERROR( "DSQMH SM error reported: %d",sm_result,0,0 );
      }
    }
    break;

  case PROXY_PHYS_LINK_FLOW_DISABLE_CMD:    
  case PROXY_PHYS_LINK_FLOW_ENABLE_CMD:
    DSQMH_MSG_MED( "DSQMH MSG DISPATCHER: supported cmd id=%d iface=%d physlink=0x%p",
                   msg_ptr->msg_id,msg_ptr->iface_inst,msg_ptr->phys_link_ptr );
    
    /*-----------------------------------------------------------------------
      Process flow control command.
    -----------------------------------------------------------------------*/
    if( DSQMH_SUCCESS !=
        dsqmhhdlr_physlink_flowctl_hdlr( DSQMH_GET_IFACE_PTR(msg_ptr->iface_inst),
                                         msg_ptr->phys_link_ptr,
                                         msg_ptr->msg_id ) )
    {
      DSQMH_MSG_ERROR( "DSQMH flowctl error reported",0,0,0 );
    }
    break;

  case PROXY_QMI_LIB_INIT_CMD:
    /*-----------------------------------------------------------------------
      Initialize the QMI Message Library connection.  This must be done
      after the physlink queues have been configured.
    -----------------------------------------------------------------------*/
    DSQMH_MSG_MED( "DSQMH MSG DISPATCHER: supported cmd id=%d",
                   msg_ptr->msg_id, 0, 0 );
    if( DSQMH_SUCCESS != dsqmhllif_init_qmi_connections() )
    {
      DSQMH_MSG_FATAL( "DSQMH failed to initialize QMI connections", 0, 0, 0);
    }
    else
    {
      DSQMH_MSG_MED( "self_init completed",0,0,0);
      dsqmh_state_info.self_init = TRUE;
    }
    break;

  default:
    DSQMH_MSG_ERROR( "DSQMH MSG DISPATCHER: unsupported cmd id=%d",
                     msg_ptr->msg_id,0,0 );
    break;
  }
  
  /*-------------------------------------------------------------------------
    Release PS memory buffer; note caller manages host task command buffer.
  -------------------------------------------------------------------------*/
  DSQMH_RELEASE_MSG_BUF( msg_ptr );

} /* dsqmhhdlr_async_msg_dispatcher() */


#if defined(FEATURE_DATA_PS_IPV6) && defined(FEATURE_DATA_PS_ADDR_MGMT)

/*===========================================================================
FUNCTION      DSHQMHDLR_IPV6_ADDR_EVENTS_CB

DESCRIPTION
  This function handles the callback events generated for additional IPV6 
  privacy addresses.

PARAMETERS
  ip_addr     - IP address structure
  addr_event  - Event indication
  user_data   - Client context data
  
DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  None.
===========================================================================*/
LOCAL int dshqmhdlr_ipv6_addr_events_cb
(
  ps_ip_addr_type                  ip_addr,
  ps_iface_addr_mgmt_event_type    addr_event,
  void                            *user_data
)
{
  ps_iface_type           *iface_ptr = NULL;  /* Proxy IFACE pointer       */
  dsqmh_msg_buf_type      *msg_ptr = NULL;    /* Pointer to message buffer */
  ps_ifacei_v6_addr_type  *v6_addr_ptr = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_LOW( "QMH HDLR IPV6 addr events cb: %d",
                 (uint32)user_data, 0, 0 );
  
  iface_ptr = DSQMH_GET_IFACE_PTR( (uint32)user_data );
  
  /*-------------------------------------------------------------------------
    Check if event for PUBLIC address. For private addresses, only the
    application need be concerned.
  -------------------------------------------------------------------------*/
  (void)ps_iface_find_ipv6_addr( &ip_addr.addr.v6, &v6_addr_ptr, &iface_ptr );
  if( v6_addr_ptr == iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX] )
  {
    switch(addr_event)
    {
      case IFACE_ADDR_MGMT_ADDR_DELETED:
        DSQMH_MSG_HIGH("QMH HDLR received IFACE_ADDR_MGMT_ADDR_DELETED",0,0,0);

        /*-------------------------------------------------------------------
          PUBLIC address has been deleted.  This will trigger iface teardown.
        -------------------------------------------------------------------*/
        DSQMH_GET_MSG_BUF( msg_ptr );
        if( NULL != msg_ptr )
        {
          memset((void*)msg_ptr, 0x0, sizeof(dsqmh_msg_buf_type));
          msg_ptr->msg_id = PROXY_IFACE_PLATFORM_DOWN_IND;
          msg_ptr->iface_inst = (uint32)user_data;

          DSQMH_MSG_MED( "Posting cmd: PROXY_IFACE_PLATFORM_DOWN_IND",0,0,0); 
          DSQMH_PUT_MSG_BUF( msg_ptr );
        }
        else
        {
          DSQMH_MSG_FATAL( "Failed to allocate msg buffer, dropping msg",0,0,0 );
        }
        break;

      case IFACE_ADDR_MGMT_ADDR_UPDATED:
        DSQMH_MSG_HIGH("QMH HDLR received IFACE_ADDR_MGMT_ADDR_UPDATED",0,0,0);
        break;

      case IFACE_ADDR_MGMT_ADDR_ADDED:
        DSQMH_MSG_HIGH("QMH HDLR received IFACE_ADDR_MGMT_ADDR_ADDED",0,0,0);
        /* IID generation & DAD triggered in SM configure state. */
        break;

      default:
        DSQMH_MSG_ERROR("QMH HDLR received unsupported event",0,0,0);
        return DSQMH_FAILED;
    }
  }

  return DSQMH_SUCCESS;
} /* dshqmhdlr_ipv6_addr_events_cb() */




/*===========================================================================
FUNCTION      DSQMHHDLR_IPV6_ADDR_MGMT_CB

DESCRIPTION
  This function is registered with the PS IFACE to be invoked for IPV6
  address state management indications.  Proxy IFACE indications are
  generated in response.

PARAMETERS
  iface_ptr       - Pointer to PS iface
  event           - Event ID
  event_info      - Event payload structure
  user_data_ptr   - Client context data
  
DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.
===========================================================================*/
LOCAL void dsqmhhdlr_ipv6_addr_mgmt_cb
(
  ps_iface_type             *iface_ptr,
  ps_iface_event_enum_type   event,
  ps_iface_event_info_u_type event_info,
  void                      *user_data_ptr
)
{
  dsqmh_msg_buf_type      *msg_ptr = NULL;    /* Pointer to message buffer */
  dsqmh_ipv6_info_type    *v6_ptr = NULL;     /* Pointer to IPV6 info block*/
  uint32                   iface_inst;        /* Index for iface table     */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  iface_inst = (uint32)user_data_ptr;
  if( !IS_IFACE_INST_VALID( iface_inst ) )
  {                
    DSQMH_MSG_ERROR( "QMH HDLR Invalid iface instance: %d", iface_inst,0,0 );
    return;        
  }
  v6_ptr = DSQMH_GET_IPV6_INFO_PTR( iface_inst );

  DSQMH_MSG_MED( "QMH HDLR IPV6 addr mgmt cb: iface=%d event=%d kind=%d",
                 iface_inst, event, event_info.prefix_info.kind );
  
  switch( event )
  {
    case IFACE_PREFIX_UPDATE_EV:
    {
      switch( event_info.prefix_info.kind )
      {
        case PREFIX_ADDED:
          /*-----------------------------------------------------------------
            Notify Iface the IPv6 public address is valid.  Note the
            event kind will be PREFIX_ADDED even though the Prefix was
            previously assigned via RS/RA exchange, before DAD was initiated.
          -----------------------------------------------------------------*/
          if( v6_ptr->dad_req )
          {
            /* Clear the DAD request pending flag */
            v6_ptr->dad_req = FALSE;
            
            /*---------------------------------------------------------------
              Send asynchronous message for cmd processing in host
              task context.
            ---------------------------------------------------------------*/
            DSQMH_GET_MSG_BUF( msg_ptr );
            if( NULL != msg_ptr )
            {
              memset((void*)msg_ptr, 0x0, sizeof(dsqmh_msg_buf_type));

              /*-------------------------------------------------------------
                Populate message buffer with context info.
              -------------------------------------------------------------*/
              DSQMH_MSG_MED( "Posting cmd: PROXY_IFACE_CONFIGURED_IND",0,0,0 );
              msg_ptr->msg_id = PROXY_IFACE_CONFIGURED_IND;
              msg_ptr->iface_inst = iface_inst;
              DSQMH_PUT_MSG_BUF( msg_ptr );
            }
            else
            {
              DSQMH_MSG_FATAL( "Failed to allocate msg buffer, dropping msg",0,0,0 );
            }
          }
          else
          {
            DSQMH_MSG_MED( "Prefix added event without DAD, ignoring",0,0,0 );
          }
          break;

        case PREFIX_DEPRECATED:
        case PREFIX_UPDATED:
        case PREFIX_REMOVED:
          /* Ignore these events, either mode handler does not care or
           * processing handled elsewhere. */
          break;
          
        default:
          DSQMH_MSG_ERROR( "QMH HDLR unsupported event kind: %d",
                           event_info.prefix_info.kind,0,0 ); 
          break;
      }
    }
    break;

    default:
      /* Ignore these events, either mode handler does not care or
       * processing handled elsewhere. */
      DSQMH_MSG_ERROR( "QMH HDLR received unsupported IPV6 addr event: %d",event,0,0);
      break;
  }
} /* dsqmhhdlr_ipv6_addr_mgmt_cb() */


/*===========================================================================
FUNCTION      DSQMHHDLR_IPV6_SM_IND_CB

DESCRIPTION
  This function is registered with the IPV6 State Machine to be invoked for
  state transition indications.  Proxy IFACE indications are generated in
  response.

PARAMETERS
  instance    - Pointer to IPV6 State Machine
  ind         - Event indication
  user_data   - Client context data
  
DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.
===========================================================================*/
LOCAL void dsqmhhdlr_ipv6_sm_ind_cb
(
  struct __ip6_sm_cb_s *instance,
  ip6_sm_indidcation_type ind,
  void *user_data
)
{
  ps_iface_type         *iface_ptr = NULL;    /* Proxy IFACE pointer       */
  dsqmh_msg_buf_type    *msg_ptr = NULL;      /* Pointer to message buffer */
  dsqmh_iface_cblk_type *cblk_ptr = NULL;     /* Pointer to control block  */
  ps_iface_addr_mgmt_handle_type    handle;
  struct ps_in6_addr     ip_addr;
  ps_ip_addr_type        ps_ip_addr;
  int16                  ps_errno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_MED( "QMH HDLR IPV6 sm ind_cb: ind=%d iface=%d",
                 ind, (uint32)user_data, 0 );

  cblk_ptr = DSQMH_GET_CBLK_PTR( (uint32)user_data );
  iface_ptr = DSQMH_GET_IFACE_PTR( (uint32)user_data );
  if( !iface_ptr )
  {
    DSQMH_MSG_ERROR( "Invalid iface inst specified: %d",
                     (uint32)user_data, 0, 0 );
    return;
  }
  
  /*-----------------------------------------------------------------------
    Process IPV6 SM indication
  -----------------------------------------------------------------------*/
  switch( ind )
  {
    case IP6_SM_UP_IND:
      /*-------------------------------------------------------------------
        Kickoff Neighbor Discovery (ND) process.  This will perform
        Duplicate Address Detection (DAD), outcome of which will trigger 
        IFACE_PREFIX_UPDATE_EV.
      -------------------------------------------------------------------*/
      if( PS_IFACE_IPV6_DEFAULT_ADDR_STATE_VALID( iface_ptr ) )
      {
        ps_ip_addr.type = IPV6_ADDR;
        ps_iface_get_addr( iface_ptr, &ps_ip_addr);

        ip_addr.ps_s6_addr64[0] = ps_ip_addr.addr.v6.ps_s6_addr64[0];
        ip_addr.ps_s6_addr64[1] = ps_ip_addr.addr.v6.ps_s6_addr64[1];
    
        handle = ps_iface_addr_mgmt_get_handle_from_ip( iface_ptr, &ip_addr );
        if( DSQMH_INVALID_HANDLE != handle )
        {
          DSQMH_MSG_IPV6_ADDR("IPV6 address: ", ip_addr.ps_s6_addr64);
          (void)ps_iface_addr_mgmt_ipv6_do_dad( iface_ptr, &handle, NULL, &ps_errno );
        }
        break;
      }
      else
      {
        DSQMH_MSG_FATAL( "QMH HDLR IPV6 address invalid",0,0,0 );
      }
      /*lint -fallthrough */

    case IP6_SM_DOWN_IND:
      if( DSPROXY_IFACE_STATE_RECONFIGURING == stm_get_state( cblk_ptr->sm_ptr ) )
      {
        /*-------------------------------------------------------------------
          In IFACE RECONFIGURING state, assume Modem has triggered address
          reconfiguration processing.  IPV6 SM was sent STOP event so
          that START can be processed to aquire new address prefix.
        -------------------------------------------------------------------*/
        if( DSQMH_SUCCESS != ip6_sm_post_event( instance, IP6_SM_START_EV ) ) 
        {
          DSQMH_MSG_ERROR("DSQMH LLIF failed posting IPV6 sm event", 0, 0, 0);
          return;
        }
      }
      else
      {
        /*-------------------------------------------------------------------
          Send asynchronous message for cmd processing in host task context.
          Only do this if Iface not already in orderly teardown.
        -------------------------------------------------------------------*/
        if( IFACE_GOING_DOWN != ps_iface_state( iface_ptr ) )
        {
          DSQMH_GET_MSG_BUF( msg_ptr );
          if( NULL != msg_ptr )
          {
            DSQMH_MSG_MED( "Posting cmd: PROXY_IFACE_PLATFORM_DOWN_IND",0,0,0); 
            memset((void*)msg_ptr, 0x0, sizeof(dsqmh_msg_buf_type));
            msg_ptr->msg_id = PROXY_IFACE_PLATFORM_DOWN_IND;
            msg_ptr->iface_inst = (uint32)user_data;
            DSQMH_PUT_MSG_BUF( msg_ptr );
            break;
          }
          else
          {
            DSQMH_MSG_FATAL( "Failed to allocate msg buffer, dropping msg",0,0,0 );
          }
        }
      }
      break;
        
    default:
      DSQMH_MSG_ERROR( "QMH HDLR unsupported message",0,0,0 );
      break;
  }
  
  return;
} /* dsqmhhdlr_ipv6_sm_ind_cb() */



/*===========================================================================
FUNCTION DSQMHHDLR_IPV6_ADDR_DAD_CB

DESCRIPTION
  This function is registered with the Proxy IFACE to perform
  Duplicate Address Detection (DAD) for IPV6 address.  It queies the
  tentative address and invokes the system default DAD function.

PARAMETERS
  iface_ptr   - Pointer to PS iface instance
  handle      - Pointer to IPV6 address structure
  user_data   - Client context data

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
LOCAL int dsqmhhdlr_ipv6_addr_dad_cb
(
  ps_iface_type                  * iface_ptr,
  ps_iface_addr_mgmt_handle_type * handle,
  void                           * user_data
)
{
  dsqmh_ipv6_info_type    *v6_ptr = NULL;     /* Pointer to IPV6 info block*/
  ps_iface_addr_mgmt_addr_info_type addr_info;
  struct ps_in6_addr ip_addr;
  int16              ps_errno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_MED( "QMH HDLR IPV6 addr dad cb: iface=%d handle=%d",
                 (uint32)user_data, *handle, 0 );
  
  v6_ptr = DSQMH_GET_IPV6_INFO_PTR( (uint32)user_data );
  if( !v6_ptr )
  {
    DSQMH_MSG_ERROR( "Invalid iface inst specified: %d",
                     (uint32)user_data, 0, 0 );
    return DSQMH_FAILED;
  }

  /*-------------------------------------------------------------------------
    Query the specified IPV6 address structure.
  -------------------------------------------------------------------------*/
  if( DSQMH_SUCCESS !=
      ps_iface_addr_mgmt_get_addr_info( handle,
                                        &addr_info,
                                        &ps_errno ) )
  {
    DSQMH_MSG_ERROR("QMH HDLR failed IPV6 get addr", 0, 0, 0);
    return DSQMH_FAILED;
  }

  ip_addr.ps_s6_addr64[0] = addr_info.prefix;
  ip_addr.ps_s6_addr64[1] = addr_info.iid;
  
  /*-------------------------------------------------------------------------
    Adjust the max DAD retries value.  Given the Modem is the routing
    entity and the SMEM link is reliable, only one Neighbor Solicitation is
    required to determine if tentative address is unique.
  -------------------------------------------------------------------------*/
  addr_info.addr_mask = PS_IFACE_ADDR_MGMT_MASK_DAD_RETRIES;
  addr_info.dad_retries = DSQMH_MAX_IPV6_DAD_RETRIES;

  if( DSQMH_SUCCESS !=
      ps_iface_addr_mgmt_set_addr_info( iface_ptr,
                                        handle,
                                        &addr_info,
                                        &ps_errno ) )
  {
    DSQMH_MSG_ERROR("QMH HDLR failed IPV6 set addr", 0, 0, 0);
    return DSQMH_FAILED;
  }

  /* Set the DAD request pending flag */
  v6_ptr->dad_req = TRUE;
  
  /*-------------------------------------------------------------------------
    Kickoff default DAD process.
  -------------------------------------------------------------------------*/
  if( DSQMH_SUCCESS !=
      ps_iface_addr_mgmt_ipv6_default_dad_f( iface_ptr,
                                             &ip_addr,
                                             &ps_errno ) )
  {
    DSQMH_MSG_ERROR("QMH HDLR failed IPV6 DAD process", 0, 0, 0);
    return DSQMH_FAILED;
  }
   
  return DSQMH_SUCCESS;
} /* dsqmhhdlr_ipv6_addr_dad_cb() */



/*===========================================================================
FUNCTION      DSQMHHDLR_IPV6_CONFIGURE_HDLR

DESCRIPTION
  This function performs the operations necessary to configure iface
  public IPV6 address.

PARAMETERS
  iface_inst      - Index for iface table
  is_reconfigure  - Reconfiguration flag
  
DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  None.
===========================================================================*/
int dsqmhhdlr_ipv6_configure_hdlr
(
  uint32     iface_inst,                        /* Index for iface table   */
  boolean    is_reconfigure
)
{
  ps_iface_type         *iface_ptr = NULL;
  dsqmh_ipv6_info_type  *v6_ptr = NULL;       /* Pointer to IPV6 info block*/
  ip6_sm_type           *ipv6_sm_ptr = NULL;
  uint64                 iid;
  int16                  ipv6_errno;
  int                    result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if( !IS_IFACE_INST_VALID( iface_inst ) )
  {                
    DSQMH_MSG_ERROR( "QMH HDLR Invalid iface instance: %d", iface_inst,0,0 );
    return DSQMH_FAILED;        
  }
  
  DSQMH_MSG_MED( "QMH HDLR IPV6 configure hdlr: iface=%d reconfig=%d",
                 iface_inst, is_reconfigure, 0 );

  iface_ptr = DSQMH_GET_IFACE_PTR( iface_inst );
  v6_ptr = DSQMH_GET_IPV6_INFO_PTR( iface_inst );
  ipv6_sm_ptr = DSQMH_GET_IPV6_SM_PTR( iface_inst );

  if( !is_reconfigure )
  {
    /*---------------------------------------------------------------------
      Generate IPV6 address unique Interface Identifier.
    ---------------------------------------------------------------------*/
    result = ps_iface_generate_ipv6_iid( iface_ptr,
                                         &iid,
                                         &ipv6_errno );
    if( !result )
    {
      (void)PS_IFACE_SET_IP_V6_IID( iface_ptr, iid );
    }
    else
    {
      DSQMH_MSG_ERROR( "DSQMH LLIF failed on generate IPv6 IID: %d",
                       ipv6_errno,0,0 );
      return DSQMH_FAILED;
    }
    
    /*---------------------------------------------------------------------
      Register IPV6 Address prefix update event callback.  This will
      be triggered after Duplicate Address Detection (DAD) on IPV6
      address buffers.
    ---------------------------------------------------------------------*/
    v6_ptr->event_buf_ptr =
      ps_iface_alloc_event_cback_buf( dsqmhhdlr_ipv6_addr_mgmt_cb,
                                      iface_ptr->client_data_ptr );
    if( v6_ptr->event_buf_ptr )
    {
      if( DSQMH_SUCCESS !=
          ps_iface_event_cback_reg( iface_ptr,
                                    IFACE_PREFIX_UPDATE_EV,
                                    v6_ptr->event_buf_ptr ) )
      {
        DSQMH_MSG_FATAL("QMH SM Could not register IPV6 address callback",
                        0, 0, 0);
        ASSERT(0);
        return DSQMH_FAILED;
      }
    }
    else
    {
      DSQMH_MSG_FATAL("QMH SM Could not allocate event buffer", 0, 0, 0);
      ASSERT(0);
      return DSQMH_FAILED;
    }
  }
  
  /*-----------------------------------------------------------------------
    For IPv6 address family, configure Neighbor Discovery module
  -----------------------------------------------------------------------*/
  v6_ptr->llc_inst = DSQMH_GET_LLE_INSTANCE( iface_inst );

  /* Only need ND control block, no event processing required. */
  v6_ptr->llc_info.nd_config.nd_cback_f_ptr = NULL;
  v6_ptr->llc_info.nd_config.usr_data_ptr   = NULL;

  ps_icmp6_nd_start( v6_ptr->llc_inst,
                     &v6_ptr->llc_info.nd_config,
                     iface_ptr,
                     NULL );
  
  /*-----------------------------------------------------------------------
    Kickoff IPV6 Router Solicitation process.  The outcome will be
    an asynchronous indication via dsqmhhdlr_ipv6_sm_ind_cb() for IPV6 SM
    UP or DOWN, which will transition the Proxy IFACE SM.
  -----------------------------------------------------------------------*/
  if( DSQMH_SUCCESS != ip6_sm_post_event( ipv6_sm_ptr, IP6_SM_START_EV ) ) 
  {
    DSQMH_MSG_ERROR("DSQMH LLIF failed posting IPV6 sm event", 0, 0, 0);
    return DSQMH_FAILED;
  }

  return DSQMH_SUCCESS;
} /* dsqmhhdlr_ipv6_configure_hdlr() */


/*===========================================================================
FUNCTION      DSQMHHDLR_IPV6_CLEANUP_HDLR

DESCRIPTION
  This function performs the operations necessary to cleanup iface
  public IPV6 address.

PARAMETERS
  iface_inst      - Index for iface table
  
DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  None.
===========================================================================*/
int dsqmhhdlr_ipv6_cleanup_hdlr
(
  uint32     iface_inst                         /* Index for iface table   */
)
{
  ps_iface_type         *iface_ptr = NULL;
  dsqmh_ipv6_info_type  *v6_ptr = NULL;       /* Pointer to IPV6 info block*/
  ip6_sm_type           *ipv6_sm_ptr = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_MED( "QMH HDLR IPV6 cleanup hdlr: iface=%d", iface_inst, 0, 0 );

  iface_ptr = DSQMH_GET_IFACE_PTR( iface_inst );
  ipv6_sm_ptr = DSQMH_GET_IPV6_SM_PTR( iface_inst );
  v6_ptr = DSQMH_GET_IPV6_INFO_PTR( iface_inst );
  if( !iface_ptr || !ipv6_sm_ptr || !v6_ptr )
  {
    DSQMH_MSG_ERROR( "Invalid iface inst specified: %d", iface_inst, 0, 0 );
    return DSQMH_FAILED;
  }

  /*-----------------------------------------------------------------------
    For IPv6 address family, stop Neighbour Discovery module
  -----------------------------------------------------------------------*/
  ps_icmp6_nd_stop( v6_ptr->llc_inst );
  
  /*-----------------------------------------------------------------------
    For IPv6 address family, post stop event to IPV6 state machine
  -----------------------------------------------------------------------*/
  if( DSQMH_SUCCESS != ip6_sm_post_event( ipv6_sm_ptr, IP6_SM_STOP_EV ) ) 
  {
    DSQMH_MSG_ERROR("DSQMH SM failed posting IPV6 sm event", 0, 0, 0);
    return DSQMH_FAILED;
  }

  /* Release the callback buffer to suppress further events */
  if( v6_ptr->event_buf_ptr )
  {
    ps_iface_event_cback_dereg( iface_ptr,
                                IFACE_PREFIX_UPDATE_EV,
                                v6_ptr->event_buf_ptr );
    ps_iface_free_event_cback_buf( v6_ptr->event_buf_ptr );
  }
    
  return DSQMH_SUCCESS;
} /* dsqmhhdlr_ipv6_cleanup_hdlr() */


/*===========================================================================
FUNCTION      DSQMHHDLR_INIT_IFACE_IPV6

DESCRIPTION
  This function configures the Proxy IFACE for IPV6 address support.
  The IPV6 state machines are created and initilized.

PARAMETERS
  iface_inst      - Index for iface table

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  None.
===========================================================================*/
LOCAL int dsqmhhdlr_init_iface_ipv6
(
  uint32 iface_inst                             /* Index for iface table   */
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  /*---------------------------------------------------------------------
    Create the IPv6 state machine.
  ---------------------------------------------------------------------*/
  if( DSQMH_SUCCESS !=
      ip6_sm_create( DSQMH_GET_IPV6_SM_PTR( iface_inst ),
                     DSQMH_GET_IFACE_PTR( iface_inst ),
                     &dsqmh_state_info.ipv6_info.sm_config ) )
  {
    DSQMH_MSG_FATAL("QMH HDLR Could not create IPV6 sm", 0, 0, 0);
    ASSERT(0);
    return DSQMH_FAILED;
  }
  
  /* Register IPV6 SM indication callback. This will be triggered
   * after Router Solicitation/Advertisement (RS/RA). */
  if( DSQMH_SUCCESS !=
      ip6_sm_ind_cback_reg( DSQMH_GET_IPV6_SM_PTR( iface_inst ),
                            dsqmhhdlr_ipv6_sm_ind_cb,
                            (void*)iface_inst ) )
  {
    DSQMH_MSG_FATAL("QMH HDLR Could not register IPV6 sm callback", 0, 0, 0);
    ASSERT(0);
    return DSQMH_FAILED;
  }

  return DSQMH_SUCCESS;
} /* dsqmhhdlr_init_iface_ipv6() */
#endif /* FEATURE_DATA_PS_IPV6 && FEATURE_DATA_PS_ADDR_MGMT */


/*===========================================================================
  
                    EXTERNAL FUNCTION DEFINITIONS

===========================================================================*/



/*===========================================================================
FUNCTION      DSQMHHDLR_INIT_IFACE_CMD

DESCRIPTION
  This function performs initialization of the specified Proxy IFACE
  instance.  The PS IFACE and Phys Links are configured for client access.
  This routine is meant to be called from the state machine entry function. 

PARAMETERS
  iface_inst      - Index for iface table

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on operation success
  DSQMH_FAILED on error condition

SIDE EFFECTS
  None.
===========================================================================*/
int dsqmhhdlr_init_iface_cmd
(
  uint32 iface_inst                             /* Index for iface table   */
)
{
  acl_type      *acl_ptr = NULL;                 /* ACL ptr                */
  ps_iface_type *iface_ptr = NULL;
  ps_phys_link_type *phys_link_ptr = NULL;
  ps_phys_link_link_protocol_handle_type ll_handle;
  ps_phys_link_capability_enum_type cap = PS_PHYS_LINK_CAPABILITY_FLOW_DECOUPLED;
  dsqmh_iface_cblk_type *cblk_ptr = NULL;
  int           iface_instance;
  uint32        phys_link;                      /* Index for phys link list*/
  int           result = DSQMH_SUCCESS;  
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_HIGH( "DSQMH INIT IFACE:%d",iface_inst,0,0 );
  
  /*-----------------------------------------------------------------------
    Initialize the specified Proxy IFACE instance
  -----------------------------------------------------------------------*/
  
  /*-----------------------------------------------------------------------
    Configure the acl_ptr.
  -----------------------------------------------------------------------*/
  acl_ptr   = DSQMH_GET_ACL_PTR( iface_inst );
  iface_ptr = DSQMH_GET_IFACE_PTR( iface_inst );
  if( !iface_ptr || !acl_ptr )
  {
    DSQMH_MSG_ERROR( "Invalid iface inst specified: %d", iface_inst, 0, 0 );
    return DSQMH_FAILED;
  }
  
  acl_ptr->acl_fptr = dsqmhacl_rt_acl;
  acl_ptr->post_proc_fptr = dsqmhacl_rt_post_proc;
  
  /*-----------------------------------------------------------------------
    Save the PS iface control block instance reference.
  -----------------------------------------------------------------------*/
  iface_ptr->client_data_ptr = (void*)iface_inst;

  /*-----------------------------------------------------------------------
    Save the State Machine instance reference.
  -----------------------------------------------------------------------*/
  DSQMHLLIF_ENTER_SMD_CRIT_SECTION();
  cblk_ptr = DSQMH_GET_CBLK_PTR( iface_inst );
  cblk_ptr->sm_ptr = &DSQMH_SM[ iface_inst ];
  cblk_ptr->qos_supported = DSQMH_QOS_SUPPORT_UNKNOWN; 
  DSQMHLLIF_EXIT_SMD_CRIT_SECTION();

  /*-----------------------------------------------------------------------
    Initialize the PS_IFACE info for this instance. Set the client data
    pointer to point to the instance.
  -----------------------------------------------------------------------*/
  iface_instance = ps_iface_create( iface_ptr,
                                    SIO_IFACE,
                                    acl_ptr,
                                    NULL,
                                    DSQMH_GET_PHYS_LINKS( iface_inst ),
                                    DSQMH_MAX_PHYSLINKS_PER_IFACE );
    
  DSQMH_MSG_HIGH( "Proxy IFACE[%d] created: 0x%p",
                  iface_inst, iface_instance, 0 );
  ASSERT( iface_instance >= 0 );

  PS_IFACE_SET_IS_PROXY_IFACE( iface_ptr, TRUE );

  iface_ptr->iface_private.proc_id = dsqmh_config_get_proc_id (iface_inst);
 
  /*-----------------------------------------------------------------------
    These bring up/tear down the Interface.
  -----------------------------------------------------------------------*/
  iface_ptr->bring_up_cmd_f_ptr   = dsqmhhdlr_iface_up_cmd;
  iface_ptr->tear_down_cmd_f_ptr  = dsqmhhdlr_iface_down_cmd;

  /*-----------------------------------------------------------------------
    This used for Interface's linger.
  -----------------------------------------------------------------------*/
  iface_ptr->linger_cmd_f_ptr     = dsqmhhdlr_iface_linger_cmd;

  /*-----------------------------------------------------------------------
    IOCTL callbacks.
  -----------------------------------------------------------------------*/
  iface_ptr->iface_ioctl_f_ptr = dsqmhhdlr_iface_ioctl_cmd;
  PS_IFACEI_GET_DEFAULT_FLOW(iface_ptr)->ps_flow_ioctl_f_ptr =
    dsqmhhdlr_flow_ioctl_cmd;
  
#ifndef FEATURE_DSS_LINUX
  /*-----------------------------------------------------------------------
    Packet TX callback.
  -----------------------------------------------------------------------*/
  ps_iface_set_tx_function( iface_ptr, dsqmhhdlr_tx_cmd_cb, (void*)iface_inst );
#endif



  /*-----------------------------------------------------------------------
    Initialize the Network Platform Layer interface.
  -----------------------------------------------------------------------*/
  (void)dsqmhllif_init_netplatform( iface_inst );

#ifdef FEATURE_DATA_PS_DATA_LOGGING
  /*-------------------------------------------------------------------------
    Enable DPL logging.  
  -------------------------------------------------------------------------*/
  (void) ps_iface_dpl_support_network_logging
         ( 
           iface_ptr,
           DSQMH_GET_DPL_IID( iface_inst )
         ); 

  (void) ps_iface_dpl_set_iface_desc( iface_ptr, NULL );

  (void) ps_iface_dpl_support_link_logging
         ( 
           iface_ptr,
           DSQMH_GET_DPL_IID( iface_inst )
         );

  (void) ps_iface_dpl_support_flow_logging
         ( 
           iface_ptr,
           DSQMH_GET_DPL_IID( iface_inst )
         );

#endif /* FEATURE_DATA_PS_DATA_LOGGING */

#if defined(FEATURE_DATA_PS_IPV6) && defined(FEATURE_DATA_PS_ADDR_MGMT)
  /*-------------------------------------------------------------------------
    Configure IPV6 support elements.
  -------------------------------------------------------------------------*/
  if( dsqmh_state_info.ipv6_info.enabled )
  {
    (void)dsqmhhdlr_init_iface_ipv6( iface_inst );
  }

  /* Register IPV6 address management callbacks */
  iface_ptr->addr_cb_f_ptr        = dshqmhdlr_ipv6_addr_events_cb;
  iface_ptr->dad_f_ptr            = dsqmhhdlr_ipv6_addr_dad_cb;
#endif /* FEATURE_DATA_PS_IPV6 && FEATURE_DATA_PS_ADDR_MGMT */

#ifdef FEATURE_DSS_LINUX
  /*-----------------------------------------------------------------------
    Iface statistics ioctl function override.
  -----------------------------------------------------------------------*/
  iface_ptr->ps_iface_stat_override_f_ptr = dsqmhllif_pkt_stats_cmd;
#endif

  /*-----------------------------------------------------------------------
    Assign the maximum allowed (DSQMH_MAX_PHYS_LINK_PER_IFACE) phys links
    to this ps iface. For each phys link enable flow on the interface.
  -----------------------------------------------------------------------*/
  for( phys_link = 0; phys_link < DSQMH_MAX_PHYSLINKS_PER_IFACE; phys_link++ )
  {
    phys_link_ptr = DSQMH_GET_PHYS_LINK_PTR( iface_inst, phys_link );
    
    /*---------------------------------------------------------------------
      Currently we use one phys link to represent multiple flows
      (i.e. one data pipe to the RmNet IFACE, with each QOS packet
      identified by metadata header).  Therefore, the phys link
      needs to be in DECOUPLED mode.  This will change if more than
      one phys link is ever used...
    ---------------------------------------------------------------------*/
    PS_PHYS_LINK_SET_CAPABILITY( phys_link_ptr, (int)cap );

    if( NULL != phys_link_ptr )
    {
      /*-----------------------------------------------------------------------
        Save the PS iface control block instance reference.
      -----------------------------------------------------------------------*/
      phys_link_ptr->client_data_ptr = (void*)iface_inst;
    
      /*---------------------------------------------------------------------
        These bring up/tear down the physical link.
      ---------------------------------------------------------------------*/
      phys_link_ptr->phys_link_down_cmd_f_ptr = dsqmhhdlr_phys_link_down_cmd;

      phys_link_ptr->phys_link_go_null_cmd_f_ptr = dsqmhhdlr_phys_link_down_cmd;

      phys_link_ptr->phys_link_up_cmd_f_ptr = dsqmhhdlr_phys_link_up_cmd;

      /*---------------------------------------------------------------------
        IOCTL Callback for bringing down or modify secondary context.
      ---------------------------------------------------------------------*/
      phys_link_ptr->ioctl_f_ptr = dsqmhhdlr_phys_link_ioctl_cmd;
      
#ifndef FEATURE_DSS_LINUX
      /*---------------------------------------------------------------------
        Initialize the UL and DL watermarks.
      ---------------------------------------------------------------------*/
      dsqmhllif_init_phys_link_queues( iface_inst, phys_link );

      /*---------------------------------------------------------------------
        Open datapath transport.
      ---------------------------------------------------------------------*/
      if( DSQMH_SUCCESS != dsqmhllif_open_transport( iface_inst, phys_link ) )
      {
        DSQMH_MSG_ERROR( "QMH open smd port failed", 0, 0, 0 );
        result = DSQMH_FAILED;
      }
      else
      {
        /*-------------------------------------------------------------------
          Initialize the datapath Rx packets signal.
        -------------------------------------------------------------------*/
        if( (DSQMH_MAX_PHYSLINKS > iface_inst) &&
            (DSQMH_MAX_PHYSLINKS_PER_IFACE > phys_link) )
        {
      
          (void) ps_set_sig_handler
                 (
                   DSQMH_GET_RX_SIGNAL( iface_inst ),
                   dsqmhhdlr_rx_sig_hdlr,
                   (void*)DSQMH_ENCODE_IFACE_PHYSLINK_ID( iface_inst,
                                                           phys_link ) 
                 );

          ps_enable_sig( DSQMH_GET_RX_SIGNAL( iface_inst ) );
        }
        else
        {
          DSQMH_MSG_ERROR( "QMH HDLR rx sig map exceeded", 0, 0, 0 );
          result = DSQMH_FAILED;
        }
      
        /*-------------------------------------------------------------------
          Set IP link protocol on phys link.
        -------------------------------------------------------------------*/
        ll_handle.none_handle.high_protocol = PS_PHYS_LINK_HIGHER_LAYER_PROTOCOL_IP;
        ll_handle.none_handle.handle.ip_proto_handle.v4_iface_ptr = iface_ptr; 
        ll_handle.none_handle.handle.ip_proto_handle.v6_iface_ptr = iface_ptr; 
        (void)ps_phys_link_set_link_protocol( phys_link_ptr,
                                              PS_PHYS_LINK_LINK_PROTOCOL_NONE,
                                              ll_handle );
      
        /*-------------------------------------------------------------------
          Enable flow on the phys link.
        -------------------------------------------------------------------*/
        ps_phys_link_enable_flow( phys_link_ptr, DS_FLOW_PROXY_MASK );
      }
#endif /* FEATURE_DSS_LINUX */

    }   
  }/* for (DSUMTS_MAX_PHYS_LINK_PER_IFACE) */
  
  return result;
} /* dsqmhhdlr_init_iface_cmd() */



/*===========================================================================
FUNCTION DSQMHHDLR_INIT

DESCRIPTION
  This function initializes the QMI mode-specific handler handlers. It
  is invoked during power-up.  It registers asynchronous message
  handler and signal handler with the host task.
  
PARAMETERS
  None.
  
DEPENDENCIES
  None.
  
RETURN VALUE
  None.
  
SIDE EFFECTS 
  None.
  
===========================================================================*/
void dsqmhhdlr_init( void )
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Register Modem IFACE policy routing callback with PS library.
  -------------------------------------------------------------------------*/
  (void)ps_route_register_acl_policy_routing_func( ACL_PROC_QCTMSM0,
                                                   dsqmhhdlr_policy_routing_hdlr0 );
#ifdef FEATURE_DSS_LINUX_MULTI_MODEM
  (void)ps_route_register_acl_policy_routing_func( ACL_PROC_QCTMSM1,
                                                   dsqmhhdlr_policy_routing_hdlr1 );
#endif
} /* dsqmhhdlr_init() */

#endif /* FEATURE_DATA_PS && FEATURE_QMI_CLIENT */
