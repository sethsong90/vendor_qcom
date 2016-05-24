/*===========================================================================


                       Q M I   M O D E   H A N D L E R

             R O U T I N G   A C C E S S   C O N T R O L   L I S T

GENERAL DESCRIPTION
  This routing ACL is used to enable policy and address based routing
  across the QMI Proxy IFACE.

EXTERNALIZED FUNCTIONS

INITIALIZATION AND SEQUENCING REQUIREMENTS

Copyright (c) 2008 -  2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

  $Header: //source/qcom/qct/modem/datahlos/interface/qmiifacectls/rel/11.01.01/src/ds_qmh_acl.c#2 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
04/13/11    hm     Multi-modem support merged from linux QMH
09/18/09    ar     Assign proxy attributes in routing post-processing.
04/02/09    ar     Fix Modem results usage & use ACL_PROXY_CONFIG_CLASS
02/25/09    ar     Set iface addr family in post-processing based on policy.
02/03/09    ar     Use ACL_CONFIG_CLASS for IFACE ID routing query.
07/21/08    ar     Rework ACL definition to use iface control block.
05/06/08    ar     Created module/initial version.

===========================================================================*/


/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"

#if defined (FEATURE_DATA_PS) && defined (FEATURE_QMI_CLIENT)

#include "ps_acl.h"
#include "ps_aclrules.h"
#include "ps_iface.h"
#include "ds_qmhi.h"
#include "ds_qmh.h"
#include "ds_qmh_acl.h"
#include "ds_qmh_hdlr.h"
#include "ds_qmh_llif.h" 

/*===========================================================================

            LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE

This section contains local definitions for constants, macros, types,
variables and other items needed by this module.

===========================================================================*/

/*===========================================================================

                        FUNCTION DEFINITIONS

===========================================================================*/


/*===========================================================================
FUNCTION DSQMHACL_RT_POST_PROC

DESCRIPTION
   This function performs the post routing lookup actions.  This funtion is
   called for the interface which is picked up as the most preferred interface
   during lookup based upon client requirements (policy) and other system
   parameters.

PARAMETERS
  *ipfilter_info_ptr  - Ip filter information
  *policy_info_ptr    - Policy information
  *this_if_ptr        - Ptr to the interface

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/
ACL_POST_PROC_DEF( dsqmhacl_rt_post_proc )
{
  ps_iface_type         *if_ptr = (ps_iface_type *) this_if_ptr;
  dsqmh_iface_cblk_type *cblk_ptr = NULL;
  uint32   pdp_profile_num= 0;
  uint32   iface_inst;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  (void)pkt_info_ptr; /* quiet Lint */

  /* Find this interface's control block  */
  iface_inst =(uint32)if_ptr->client_data_ptr;
  cblk_ptr = DSQMH_GET_CBLK_PTR( iface_inst );

  DSQMH_MSG_HIGH( "Interface 0x%x::%d post proc",
                  this_if_ptr, iface_inst, 0);

  /*-------------------------------------------------------------------------
     Store the policy profile information specified in the control block
     if the interface is not currently in USE. This is to ensure that
     this is done ONLY when the interface is selected for the first time.
  -------------------------------------------------------------------------*/
  if( !PS_IFACE_IS_IN_USE(if_ptr) )
  {
    /* The policy info shall NOT be cached when policy lookup is IFACE
     * ID based.  This ensures the previous IFACE KIND lookup results
     * are preserved.  This is a hack to support IFACE ID lookup,
     * which is deprecated. */
    if( DSS_IFACE_ID != policy_info_ptr->iface.kind )
    {
      DSQMHLLIF_ENTER_SMD_CRIT_SECTION();
      cblk_ptr->policy_info = *policy_info_ptr;
      DSQMHLLIF_EXIT_SMD_CRIT_SECTION();
      /* Assign Proxy Iface name to be that from Modem Um IFACE */
      if_ptr->name = policy_info_ptr->rt_result.if_name;
      if(TRUE == dsqmhhdlr_iface_is_linger_supported(if_ptr))
      {
        if_ptr->iface_private.linger_timeout_val 
          = DSQMH_DEFAULT_LINGER_TIMEOUT;
      }

      /* Assign proxy attributes */
      switch( if_ptr->name )
      {
        /* Broadcast Ifaces */
        case CDMA_BCAST_IFACE:
        case MBMS_IFACE:
        case FLO_IFACE:
        case DVBH_IFACE:
          if_ptr->group_mask = (uint16)BCAST_MCAST_GROUP;
          PS_IFACE_SET_IS_BCAST_IFACE( if_ptr, TRUE );
          DSQMH_MSG_MED("Proxy for Broadcast iface: 0x%x", if_ptr->name, 0, 0);
          break;
          
        /* 3GPP2 Ifaces */
        case CDMA_SN_IFACE:
        case CDMA_AN_IFACE:
        case UW_FMC_IFACE:
        case IWLAN_3GPP2_IFACE:
          if_ptr->group_mask = (uint16)IFACE_3GPP2_GROUP;
          PS_IFACE_SET_IS_BCAST_IFACE( if_ptr, FALSE );
          DSQMH_MSG_MED("Proxy for 3GPP2 iface: 0x%x", if_ptr->name, 0, 0);
          break;
          
        /* 3GPP Ifaces */
        case UMTS_IFACE:
        case IWLAN_3GPP_IFACE:
          if_ptr->group_mask = (uint16)IFACE_3GPP_GROUP;
          PS_IFACE_SET_IS_BCAST_IFACE( if_ptr, FALSE );
          DSQMH_MSG_MED("Proxy for 3GPP iface: 0x%x", if_ptr->name, 0, 0);
          break;

        /* All other Ifaces */
        default:
          if_ptr->group_mask = (uint16)WWAN_GROUP;
          PS_IFACE_SET_IS_BCAST_IFACE( if_ptr, FALSE );
          DSQMH_MSG_MED("Proxy for iface: 0x%x", if_ptr->name, 0, 0);
          break;
      }
    }
    else
    {
      DSQMH_MSG_MED("Iface ID used, skipping policy update", 0, 0, 0);
    }

#ifdef FEATURE_DATA_PS_IPV6
    /* Assign address family based on the policy */
    if( IFACE_IPV6_ADDR_FAMILY == cblk_ptr->policy_info.ip_family )
    {
      (void)ps_iface_set_addr_family( if_ptr, IFACE_IPV6_ADDR_FAMILY );
    }
    else
#endif /* FEATURE_DATA_PS_IPV6 */
    {
      (void)ps_iface_set_addr_family( if_ptr, IFACE_IPV4_ADDR_FAMILY );
    }
  }
  else
  {
    DSQMH_MSG_MED("Interface already in use: prof:%d", pdp_profile_num, 0, 0);
  }
} /* dsqmhacl_rt_post_proc() */



/*===========================================================================

                 ACCESS CONTROL LIST NAME DEFINITIONS

===========================================================================*/

/*-------------------------------------------------------------------------
  QMH Proxy IFACE ACL

  Routing by policy for Proxy IFACE is performed as follows:

  1. Policy routing lookup is done on each remote processor (modem) to
     determine its best candidate IFACE for the client-specified
     policy.  The returned IFACE handle, RmNET instance, priority, and
     processor ID are preserved in the mode handler state as the
     routing result.

  2. Caller-specified policy is assigned the best routing result
     amongst all the remote processors.

  3. Policy routing lookup is done on the local processor to determine
     its best candidate IFACE over entire IFACE set (local and proxy).
     For Proxy IFACE:
     - Policy is a match if procesor ID, IFACE handle (if specified) &
       RmNET instance (if specified) are the same compared to mode handler
       routing result.
     - No match will return the priority from the caller-specified policy.

-------------------------------------------------------------------------*/

ACL_DEF( dsqmhacl_rt_acl )
ACL_START
{
  int32                   qmh_iface_inst = -1;
  int32                   iface_inst_from_rm_hndl = -1;
  acl_policy_info_type  * qmh_policy_info_ptr = NULL;
  int32                   iface_offset = 0;
  boolean                 qmh_port_inuse = FALSE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  (void) pkt_info_ptr;                                       /* quiet Lint */

#ifndef AR_HACK
/* Workaround for coexisting with Android background call on rmnet0.
 * Route lookup results will return some other Modem Um iface but
 * local routing algorithm will select proxy iface[0], issue start
 * network over it, and fail on Modem given RmNET iface[0] is already
 * in use.  This hack ensures proxy iface[0] is never used. */
#ifdef FEATURE_DS_LINUX_ANDROID
  qmh_iface_inst = (int32) ((ps_iface_type *) this_if_ptr)->client_data_ptr;
  DENY_WITH_REASON( (0 == qmh_iface_inst),
                    ACL_DENY_REASON_POL_NO_MATCH );
#endif /* FEATURE_DSS_LINUX */  
#endif // AR_HACK
  
  PASS_QUICK( ACL_DEFAULT_CLASS, IS_POL_INFO_NULL() );
  PASS_QUICK( ACL_CONFIG_CLASS, PROC_ID_ANY() );
  PASS_QUICK( ACL_CONFIG_CLASS, IFACE_ID_MATCH() );

  qmh_iface_inst = (int32) ((ps_iface_type *) this_if_ptr)->client_data_ptr;
  qmh_policy_info_ptr = DSQMH_GET_POLICY_INFO_PTR( qmh_iface_inst );

  /*-------------------------------------------------------------------------
    Deny ACL if iface is in use by processor A and if request came from
    processor B
  -------------------------------------------------------------------------*/
  DENY_WITH_REASON( DIFFERENT_PROC_ID_REQUIRED(),
                    ACL_DENY_REASON_POL_NO_MATCH );

  /*-------------------------------------------------------------------------
    Proxy ACL needs to be executed only if policy matched one of the Um ifaces
    on the modem, in which case if_hndl will be set to a valid iface ID
  -------------------------------------------------------------------------*/
  DENY_WITH_REASON( DSQMH_INVALID_IFACEID == policy_info_ptr->rt_result.if_hndl,
                    ACL_DENY_REASON_POL_NO_MATCH );
#if 0
  //TODO Add support for this when multiple modems are connected to one
  //apps proc. Also see if modem_id is the right word??
  /*-------------------------------------------------------------------------
    Deny ACL if iface is in use by processor A and if request came from
    processor B
  -------------------------------------------------------------------------*/
  DENY_WITH_REASON( DIFFERENT_MODEM_ID_REQUIRED( qmh_policy_info_ptr),
                    POL_NO_MATCH );
#endif

  /*-------------------------------------------------------------------------
    Verify Proxy IFACE handle match (if specified). This is case when IFACE
    is already in use.
  -------------------------------------------------------------------------*/
  if( qmh_policy_info_ptr &&
      DSQMH_INVALID_IFACEID != qmh_policy_info_ptr->rt_result.if_hndl)
  {
    DENY_WITH_REASON( PROXY_IFACE_HANDLE_NO_MATCH( qmh_policy_info_ptr ),
                      ACL_DENY_REASON_POL_NO_MATCH );
    /* Handles match, return priority */
    PASS_QUICK( ACL_PROXY_CONFIG_CLASS, TRUE );
  }

  /*-------------------------------------------------------------------------
    Verify QMI instance match (if specified). This is case when Modem Um 
    IFACE is already in use and can be shared.
  -------------------------------------------------------------------------*/
  if( DSQMH_INVALID_RMNET != policy_info_ptr->rt_result.rm_hndl )
  {
    iface_inst_from_rm_hndl =
      dsqmh_config_get_iface_inst_from_rm_handle 
      (
        policy_info_ptr->rt_result.rm_hndl,
        ((ps_iface_type *) this_if_ptr)->iface_private.proc_id
      );

    DENY_WITH_REASON( (qmh_iface_inst != iface_inst_from_rm_hndl),
                      ACL_DENY_REASON_POL_NO_MATCH );
    PASS_QUICK( ACL_PROXY_CONFIG_CLASS, TRUE );
  }

  /*-------------------------------------------------------------------------
    Verify RmNET interface is not in use by another process.  QMI
    requires StartNI be issued on the same port as that associated
    with the RmNET interface.  Modem will reject call with incompatible
    policy error if another process has it allocated yet route lookup
    did not resolve to share interface. So we skip any port already
    active in call.
  -------------------------------------------------------------------------*/
  if( DSQMH_SUCCESS ==
      dsqmhllif_is_qmiport_incall( qmh_iface_inst, &qmh_port_inuse ) )
  {
    DENY_WITH_REASON( qmh_port_inuse, ACL_DENY_REASON_IFACE_IN_USE );
  }

  /*-------------------------------------------------------------------------
    If the UM iface is getting arbitrated, it would return with negative 
    priority. QMH needs to convert it into positive priority and return
    back to local routing layer.
  -------------------------------------------------------------------------*/
  if (policy_info_ptr->rt_result.priority < 0)
  {
    acl_ret_val = (-1) * (policy_info_ptr->rt_result.priority);
  }
  else
  {
    acl_ret_val = policy_info_ptr->rt_result.priority;
  }
}
ACL_END

#endif /* FEATURE_DATA_PS && FEATURE_QMI_CLIENT */
