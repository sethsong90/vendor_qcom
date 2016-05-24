/*===========================================================================

                        P S _ I F A C E _ L O G G I N G . C

GENERAL DESCRIPTION
  This file defines external API used by mode handlers to enable logging and
  to provide description for various logging structures.

EXTERNAL FUNCTIONS
  ps_iface_dpl_get_iface_desc()
    Get ps_iface's description

  ps_iface_dpl_set_iface_desc()
    Set ps_iface's description

  ps_iface_dpl_get_flow_desc()
    Get a flow's(ps_phys_link) description

  ps_iface_dpl_set_flow_desc()
    Set a flow's(ps_phys_link) description

  ps_iface_dpl_get_phys_link_desc()
    Get a phys link's(for eg, RLP/RLC instance) description

  ps_iface_dpl_set_phys_link_desc()
    Set a phys link's(for eg, RLP/RLC instance) description

  ps_iface_dpl_support_network_logging()
    Support network logging on the specified interface

  ps_iface_dpl_support_flow_logging()
    Support flow logging on the specified interface

  ps_iface_dpl_support_link_logging()
    Support link logging on the specified interface

INITIALIZATION AND SEQUENCING REQUIREMENTS
  None

Copyright (c) 2004-2009 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary.
===========================================================================*/

/*===========================================================================
                        EDIT HISTORY FOR MODULE

 $Header: //source/qcom/qct/modem/datamodem/interface/netiface/rel/11.03/src/ps_iface_logging.c#1 $
 $Author: zhasan $ $DateTime: 2011/06/17 12:02:33 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
03/15/10    hs     Added DPL support on two new LTE Ifaces
11/21/08    am     Fixed incorrect setting of link bit in IID of
                   iface/flow/phys_link. Using new macro names for IID flags.
10/17/08    am     Fixed ANSI C warnings for bit-field usage.
12/25/06    msr    Fixed broken secondary link logging
07/17/06    mp     Fixed logging of zero length DPL packets
06/05/06    mp     Moved DPL link logging control block from ps_iface to
                   ps_phys_link and added defualt ppp logging settings
02/22/06    msr    Using single critical section
02/14/06    ks     fixed compile warnings.
02/13/06    msr    Remoed some of critical sections
02/06/06    msr    Updated for L4 tasklock/crit sections.
10/31/05    msr    Changed default snaplen to 80 for PDP context 0, 1, and 2
08/22/05    msr    Not allowing flow/link logging on logical ifaces
08/16/05    msr    Fixed PS_BRANCH_TASKFREE()
08/15/05    msr    Using ps_flows instead of ps_phys_links to support flow
                   logging
05/12/05    ks     Fixed lint errors.
03/14/05    ks     Fixed setting of link bit in
                   ps_iface_dpl_support_link_logging().
03/02/05    msr    Moved default settings to
                   ps_iface_dpl_set_network_logging().
02/03/05    msr    Settings IP logging by default for UMTS Um ifaces.
01/13/05    ks     fixed ps_iface_dpl_set_link_desc().
01/12/05    msr    Added code review comments.
01/10/05    msr    Added support for flow and link logging.
10/31/04   ks/msr  Created module.
===========================================================================*/

/*===========================================================================
                     INCLUDE FILES FOR MODULE
===========================================================================*/

#include "comdef.h"
#include "customer.h"

#ifdef FEATURE_DATA_PS
#ifdef FEATURE_DATA_PS_DATA_LOGGING

#include "ps_iface.h"
#include "ps_utils.h"
#include "ps_logging_defs.h"
#include "ps_iface_logging.h"
#include "ps_loggingi.h"
#include "AEEstd.h"
#include "ps_crit_sect.h"
#include "ds_Utils_DebugMsg.h"


/*===========================================================================

                          EXTERNAL FUNCTION DECLARATIONS

===========================================================================*/
/*===========================================================================
FUNCTION    PS_IFACE_DPL_GET_IFACE_DESC

DESCRIPTION
  Get ps_iface description from ps_iface structure.
  CALLER MUST NOT MODIFY MEMORY AT THE RETURN ADDRESS!

PARAMETERS
  ps_iface_ptr : pointer to ps_iface structure

RETURN VALUE
  Pointer to string containing description of specified ps_iface

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
const char *ps_iface_dpl_get_iface_desc
(
  ps_iface_type  * ps_iface_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (!PS_IFACE_IS_VALID(ps_iface_ptr))
  {
    LOG_MSG_FATAL_ERROR("Invalid iface, 0x%p, passed", ps_iface_ptr, 0, 0);
    ASSERT(0);
    return NULL;
  }

  return (const char *) ps_iface_ptr->dpl_net_cb.desc;

} /* ps_iface_dpl_get_iface_desc() */



/*===========================================================================
FUNCTION     PS_IFACE_DPL_SET_IFACE_DESC

DESCRIPTION
  Set ps_iface description in corresponding ps_iface structure

PARAMETERS
  ps_iface_ptr : pointer to ps_iface structure
  desc         : desc to be set

RETURN VALUE
  TRUE on sucess
  FALSE on failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
boolean ps_iface_dpl_set_iface_desc
(
  ps_iface_type  * ps_iface_ptr,
  char           * desc
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (!PS_IFACE_IS_VALID(ps_iface_ptr))
  {
    LOG_MSG_FATAL_ERROR("Invalid iface, 0x%p, passed", ps_iface_ptr, 0, 0);
    ASSERT(0);
    return FALSE;
  }

  /*-------------------------------------------------------------------------
    ifname must have been set if logging is enabled on this ps_iface
  -------------------------------------------------------------------------*/
  if (!(DPL_IID_IFNAME_MIN <= ps_iface_ptr->dpl_net_cb.tx_dpl_id.ifname &&
        DPL_IID_IFNAME_MAX > ps_iface_ptr->dpl_net_cb.tx_dpl_id.ifname))
  {
    LOG_MSG_INFO2("Network logging is not enabled on this iface 0x%p",
            ps_iface_ptr, 0, 0);
    return FALSE;
  }

  /*-------------------------------------------------------------------------
    If NULL is passed as parameter, get default description from
    dpl lookup table
  -------------------------------------------------------------------------*/
  if (NULL == desc)
  {
    desc = (char *) dpli_get_ps_iface_default_desc(
        (dpl_iid_ifname_enum_type)ps_iface_ptr->dpl_net_cb.tx_dpl_id.ifname);
  }

  /*-------------------------------------------------------------------------
    Ensure that desc is atmost DPL_IFNAME_DESC_S_LEN characters long and is
    NULL terminated
  -------------------------------------------------------------------------*/
  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);
  ps_iface_ptr->dpl_net_cb.desc[DPL_IFNAME_DESC_S_LEN - 1] = '\0';
  (void) std_strlprintf(ps_iface_ptr->dpl_net_cb.desc,
                        DPL_IFNAME_DESC_S_LEN - 1,
                        "%s:0x%X:%d",
                        desc,
                        ps_iface_ptr->name,
                        ps_iface_ptr->instance);
  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

  return TRUE;

} /* ps_iface_dpl_set_iface_desc() */



/*===========================================================================
FUNCTION    PS_IFACE_DPL_GET_FLOW_DESC

DESCRIPTION
  Get flow description from a given ps_flow structure.
  CALLER MUST NOT MODIFY MEMORY AT THE RETURN ADDRESS!

PARAMETERS
  flow_ptr : pointer to ps_flow structure

RETURN VALUE
  Pointer to string containing description of specified flow

DEPENDENCIES
  Must be called in TASKLOCK()

SIDE EFFECTS
  None
===========================================================================*/
const char *ps_iface_dpl_get_flow_desc
(
  ps_flow_type  * flow_ptr
)
{
 /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (!PS_FLOW_IS_VALID(flow_ptr))
  {
    LOG_MSG_FATAL_ERROR("Invalid flow, 0x%p, is passed", flow_ptr, 0, 0);
    ASSERT(0);
    return NULL;
  }

  return (const char *) flow_ptr->dpl_flow_cb.desc;

} /* ps_iface_dpl_get_flow_desc() */



/*===========================================================================
FUNCTION     PS_IFACE_DPL_SET_FLOW_DESC

DESCRIPTION
  Set flow description in corresponding ps_flow structure

PARAMETERS
  flow_ptr : pointer to ps_flow structure
  desc     : desc to be set

RETURN VALUE
  TRUE on success
  FALSE on failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
boolean ps_iface_dpl_set_flow_desc
(
  ps_flow_type  * flow_ptr,
  char          * desc
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  if (!PS_FLOW_IS_VALID(flow_ptr))
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_FATAL_ERROR("Invalid flow ptr, 0x%p, passed", flow_ptr, 0, 0);
    ASSERT(0);
    return FALSE;
  }

  /*-------------------------------------------------------------------------
    ifname must have been set if logging is enabled on this ps_flow
  -------------------------------------------------------------------------*/
  if (!(DPL_IID_IFNAME_MIN <= flow_ptr->dpl_flow_cb.tx_dpl_id.ifname &&
        DPL_IID_IFNAME_MAX > flow_ptr->dpl_flow_cb.tx_dpl_id.ifname))
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_INFO2("Flow logging is not enabled on ps_flow 0x%p", flow_ptr, 0, 0);
    return FALSE;
  }

  /*-------------------------------------------------------------------------
    Ensure that desc is atmost DPL_FLOW_DESC_S_LEN characters long and is
    NULL terminated
  -------------------------------------------------------------------------*/
  flow_ptr->dpl_flow_cb.desc[DPL_FLOW_DESC_S_LEN - 1] = 0;
  if (NULL != desc)
  {
    (void) std_strlprintf(flow_ptr->dpl_flow_cb.desc,
                          DPL_FLOW_DESC_S_LEN - 1,
                          "%s:0x%p",
                          desc,
                          flow_ptr);
  }
  else
  {
    /*-----------------------------------------------------------------------
      Use a default description
    -----------------------------------------------------------------------*/
    (void) std_strlprintf(flow_ptr->dpl_flow_cb.desc,
                          DPL_FLOW_DESC_S_LEN - 1,
                          "%s %d:0x%p",
                          "Flow",
                          PS_FLOWI_GET_COOKIE(flow_ptr),
                          flow_ptr);
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return TRUE;

} /* ps_iface_dpl_set_flow_desc() */



/*===========================================================================
FUNCTION    PS_IFACE_DPL_GET_PHYS_LINK_DESC

DESCRIPTION
  Get phys link description from phys link structure
  CALLER MUST NOT MODIFY MEMORY AT THE RETURN ADDRESS!

PARAMETERS
  phys_link_ptr : pointer to phys link

RETURN VALUE
  Pointer to string containing description of specified phys link

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
const char *ps_iface_dpl_get_phys_link_desc
(
  const ps_phys_link_type  * phys_link_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (!PS_PHYS_LINK_IS_VALID(phys_link_ptr))
  {
    LOG_MSG_FATAL_ERROR("Invalid phys link, 0x%p, is passed", phys_link_ptr, 0, 0);
    ASSERT(0);
    return NULL;
  }

  return (const char *) phys_link_ptr->dpl_link_cb.desc;

} /* ps_iface_dpl_get_phys_link_desc() */



/*===========================================================================
FUNCTION     PS_IFACE_DPL_SET_PHYS_LINK_DESC

DESCRIPTION
  Set phys link description in corresponding phys link structure

PARAMETERS
  phys_link_ptr : pointer to phys link
  desc          : desc to be set

RETURN VALUE
  TRUE on success
  FALSE on failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
boolean ps_iface_dpl_set_phys_link_desc
(
  ps_phys_link_type  * phys_link_ptr,
  char               * desc
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (!PS_PHYS_LINK_IS_VALID(phys_link_ptr))
  {
    LOG_MSG_FATAL_ERROR("Invalid phys link ptr, 0x%p, passed", phys_link_ptr, 0, 0);
    ASSERT(0);
    return FALSE;
  }

  /*-------------------------------------------------------------------------
    ifname must have been set if logging is enabled on this ps_iface
  -------------------------------------------------------------------------*/
  if (!(DPL_IID_IFNAME_MIN <= phys_link_ptr->dpl_link_cb.tx_dpl_id.ifname &&
        DPL_IID_IFNAME_MAX > phys_link_ptr->dpl_link_cb.tx_dpl_id.ifname))
  {
    LOG_MSG_INFO2("Link logging is not enabled on phys link 0x%p",
            phys_link_ptr, 0, 0);
    return FALSE;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    Ensure that desc is atmost DPL_LINK_DESC_S_LEN characters long and is
    NULL terminated
  -------------------------------------------------------------------------*/
  phys_link_ptr->dpl_link_cb.desc[DPL_LINK_DESC_S_LEN - 1] = '\0';
  if (NULL != desc)
  {
    (void) std_strlprintf(phys_link_ptr->dpl_link_cb.desc,
                          DPL_LINK_DESC_S_LEN - 1,
                          "%s",
                          desc);
  }
  else
  {
    /*-----------------------------------------------------------------------
      Use a default description
    -----------------------------------------------------------------------*/
    (void) std_strlprintf(phys_link_ptr->dpl_link_cb.desc,
                          DPL_LINK_DESC_S_LEN - 1,
                          "Link %d",
                          phys_link_ptr->phys_private.instance);
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

  return TRUE;

} /* ps_iface_dpl_set_phys_link_desc() */



/*===========================================================================
FUNCTION    PS_IFACE_DPL_SUPPORT_NETWORK_LOGGING

DESCRIPTION
  Populate ifname in logging control block in ps_iface to indicate that
  logging is supported on this interface.

PARAMETERs
  ps_iface_ptr : ptr to iface
  ifname       : IID ifname to be set

RETURN VALUE
  0 on success
  -1 on failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_dpl_support_network_logging
(
  ps_iface_type            * ps_iface_ptr,
  dpl_iid_ifname_enum_type   ifname
)
{
#ifdef FEATURE_DATA_PS_DEBUG
  ps_iface_type  ** if_ptr;
  int               i;
#endif /* FEATURE_DATA_PS_DEBUG */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (DPL_IID_IFNAME_MIN > ifname || DPL_IID_IFNAME_MAX <= ifname)
  {
    LOG_MSG_FATAL_ERROR("Invalid ifname (%d) passed", ifname, 0, 0);
    ASSERT(0);
    return -1;
  }

  if (!PS_IFACE_IS_VALID(ps_iface_ptr))
  {
    LOG_MSG_FATAL_ERROR("Invalid iface, 0x%p, is passed", ps_iface_ptr, 0, 0);
    ASSERT(0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    Do nothing if ifname is already set. Same ifname is set in both tx_cb
    and recv_cb.
  -------------------------------------------------------------------------*/
  if (DPL_IID_IFNAME_MIN <= ps_iface_ptr->dpl_net_cb.tx_dpl_id.ifname)
  {
    LOG_MSG_INFO2("Trying to set ifname for an already set iface_ptr 0x%p",
            ps_iface_ptr, 0, 0);
    return 0;
  }

#ifdef FEATURE_DATA_PS_DEBUG
  /*-------------------------------------------------------------------------
    Ensure that there is no iface with the same ifname as the one passed as
    parameter. Since Ifaces are never destroyed, there is no need to traverse
    global_iface_ptr_array once we hit a NULL.
  -------------------------------------------------------------------------*/
  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);
  if_ptr = global_iface_ptr_array;
  for (i = 0; (i < MAX_SYSTEM_IFACES) && (NULL != *if_ptr); i++, if_ptr++)
  {
    if (ifname == (*if_ptr)->dpl_net_cb.tx_dpl_id.ifname)
    {
      PS_BRANCH_LEAVE_CRIT_SECTION (&global_ps_crit_section);
      LOG_MSG_FATAL_ERROR("Duplicate ifname (%d) passed to iface_ptr 0x%p. The other "
          "iface_ptr is 0x%p", ifname, ps_iface_ptr, *if_ptr);
      ASSERT(0);
      return -1;
    }
  }
  PS_LEAVE_CRIT_SECTION (&global_ps_crit_section);
#endif /* FEATURE_DATA_PS_DEBUG */

  /*-------------------------------------------------------------------------
    Once logging is supported, the cached IID values are initialized in the
    log flags structs.  Hence, cached IID != 0 will indicate support.
  -------------------------------------------------------------------------*/
  ps_iface_ptr->dpl_net_cb.tx_dpl_id.ifname     = ifname;
  SET_DPL_IID_L_BIT_NETWORK(ps_iface_ptr->dpl_net_cb.tx_dpl_id);
  SET_DPL_IID_F_BIT_NONFLOW(ps_iface_ptr->dpl_net_cb.tx_dpl_id);
  SET_DPL_IID_DIR_TX(ps_iface_ptr->dpl_net_cb.tx_dpl_id);

  ps_iface_ptr->dpl_net_cb.recv_dpl_id.ifname     = ifname;
  SET_DPL_IID_L_BIT_NETWORK(ps_iface_ptr->dpl_net_cb.recv_dpl_id);
  SET_DPL_IID_F_BIT_NONFLOW(ps_iface_ptr->dpl_net_cb.recv_dpl_id);
  SET_DPL_IID_DIR_RX(ps_iface_ptr->dpl_net_cb.recv_dpl_id);

  dpli_set_ps_iface_ptr(ps_iface_ptr, ifname);

  /*-------------------------------------------------------------------------
    Set default log settings for UMTS Um ifaces
  -------------------------------------------------------------------------*/
#define DPL_UMTS_IP_DEFAULT_SNAPLEN (80)
  switch (ifname)
  {
    case DPL_IID_IFNAME_UMTS_PDP_CONTEXT_0:
    case DPL_IID_IFNAME_UMTS_PDP_CONTEXT_1:
    case DPL_IID_IFNAME_UMTS_PDP_CONTEXT_2:
    case DPL_IID_IFNAME_3GPP_CONTEXT_0_IPV4:
    case DPL_IID_IFNAME_3GPP_CONTEXT_0_IPV6:
    case DPL_IID_IFNAME_3GPP_CONTEXT_1_ANY:
    case DPL_IID_IFNAME_3GPP_CONTEXT_2_ANY:
    case DPL_IID_IFNAME_3GPP_CONTEXT_3_ANY:
    case DPL_IID_IFNAME_3GPP_CONTEXT_4_ANY:
    case DPL_IID_IFNAME_3GPP_CONTEXT_5_ANY:
    case DPL_IID_IFNAME_3GPP_CONTEXT_6_ANY:
      LOG_MSG_INFO2("Setting default settings for iface 0x%p", ps_iface_ptr, 0, 0);

      ps_iface_ptr->dpl_net_cb.recv_cb.mask |=
        (0x01 << (DPL_IID_NETPROT_IP - 1));
      ps_iface_ptr->dpl_net_cb.recv_cb.snaplen[DPL_IID_NETPROT_IP] =
        DPL_UMTS_IP_DEFAULT_SNAPLEN;

      ps_iface_ptr->dpl_net_cb.tx_cb.mask |=
        (0x01 << (DPL_IID_NETPROT_IP - 1));
      ps_iface_ptr->dpl_net_cb.tx_cb.snaplen[DPL_IID_NETPROT_IP] =
        DPL_UMTS_IP_DEFAULT_SNAPLEN;

      break;

    default:
      break;
  }

  return 0;
} /* ps_iface_dpl_support_network_logging() */



/*===========================================================================
FUNCTION    PS_IFACE_DPL_SUPPORT_FLOW_LOGGING

DESCRIPTION
  Populate ifname in logging control block in default ps_flow of an ps_iface
  to indicate that flow logging is supported on this ps_iface

PARAMETERs
  ps_iface_ptr : ptr to iface
  ifname       : IID ifname to be set

RETURN VALUE
   0 on success
  -1 on failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_dpl_support_flow_logging
(
  ps_iface_type            * ps_iface_ptr,
  dpl_iid_ifname_enum_type   ifname
)
{
  ps_flow_type   * flow_ptr;
#ifdef FEATURE_DATA_PS_DEBUG
  ps_iface_type  ** if_ptr;
  int              i;
#endif /* FEATURE_DATA_PS_DEBUG */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (DPL_IID_IFNAME_MIN > ifname || DPL_IID_IFNAME_MAX <= ifname)
  {
    LOG_MSG_FATAL_ERROR("Invalid ifname (%d) passed", ifname, 0, 0);
    ASSERT(0);
    return -1;
  }

  if (!PS_IFACE_IS_VALID(ps_iface_ptr))
  {
    LOG_MSG_FATAL_ERROR("Invalid iface_ptr, 0x%p, is passed", ps_iface_ptr, 0, 0);
    ASSERT(0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    As QoS is not supported on logical ifaces, they do not have secondary
    flows
  -------------------------------------------------------------------------*/
  if (PS_IFACEI_IS_LOGICAL(ps_iface_ptr))
  {
    LOG_MSG_ERROR("Flow logging is not supported on logical iface, 0x%x:%d",
              ps_iface_ptr->name, ps_iface_ptr->instance, 0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    Do nothing if ifname is already set.
  -------------------------------------------------------------------------*/
  flow_ptr = PS_IFACEI_GET_DEFAULT_FLOW(ps_iface_ptr);
  ASSERT(PS_FLOW_IS_VALID(flow_ptr));

  if (DPL_IID_IFNAME_MIN <= flow_ptr->dpl_flow_cb.tx_dpl_id.ifname)
  {
    LOG_MSG_INFO2("Trying to enable flow logging on an iface (0x%x:%d) which "
            "already supports it",
            ps_iface_ptr->name, ps_iface_ptr->instance, 0);
    return 0;
  }

#ifdef FEATURE_DATA_PS_DEBUG
  /*-------------------------------------------------------------------------
    Ensure that there is no iface with the same ifname as the one passed as
    parameter. Since Ifaces are never destroyed, there is no need to traverse
    global_iface_ptr_array once we hit a NULL.
  -------------------------------------------------------------------------*/
  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);
  if_ptr = global_iface_ptr_array;
  for (i = 0; (i < MAX_SYSTEM_IFACES) && (NULL != *if_ptr); i++, if_ptr++)
  {
    flow_ptr = PS_IFACEI_GET_DEFAULT_FLOW(*if_ptr);
    ASSERT(PS_FLOW_IS_VALID(flow_ptr));

    if (ifname == flow_ptr->dpl_flow_cb.tx_dpl_id.ifname)
    {
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      LOG_MSG_FATAL_ERROR("Duplicate ifname (%d) passed to iface_ptr 0x%p. The other "
          "iface_ptr is 0x%p", ifname, ps_iface_ptr, *if_ptr);
      ASSERT(0);
      return -1;
    }
  }
  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
#endif /* FEATURE_DATA_PS_DEBUG */

  /*-------------------------------------------------------------------------
    Once logging is supported, the cached IID values are initialized in the
    log flags structs.  Hence, cached IID != 0 will indicate support.
  -------------------------------------------------------------------------*/
  flow_ptr = PS_IFACEI_GET_DEFAULT_FLOW(ps_iface_ptr);
  ASSERT(PS_FLOW_IS_VALID(flow_ptr));

  flow_ptr->dpl_flow_cb.tx_dpl_id.ifname          = ifname;
  SET_DPL_IID_L_BIT_NETWORK(flow_ptr->dpl_flow_cb.tx_dpl_id);
  SET_DPL_IID_F_BIT_FLOW(flow_ptr->dpl_flow_cb.tx_dpl_id);
  flow_ptr->dpl_flow_cb.tx_dpl_id.link_instance   = 0;
  SET_DPL_IID_DIR_TX(flow_ptr->dpl_flow_cb.tx_dpl_id);

  dpli_set_ps_iface_ptr(ps_iface_ptr, ifname);
  return 0;

} /* ps_iface_dpl_support_flow_logging() */



/*===========================================================================
FUNCTION    PS_IFACE_DPL_SUPPORT_LINK_LOGGING

DESCRIPTION
  Populate ifname in logging control block in link_cb_ptr to indicate that
  logging is supported on this interface.

PARAMETERs
  ps_iface_ptr : ptr to iface
  ifname       : IID ifname to be set

RETURN VALUE
  0 on success
  -1 on failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_dpl_support_link_logging
(
  ps_iface_type            * ps_iface_ptr,
  dpl_iid_ifname_enum_type   ifname
)
{
  ps_phys_link_type  * phys_link_ptr;
  uint8                i;
#ifdef FEATURE_DATA_PS_DEBUG
  ps_iface_type     ** if_ptr;
#endif /* FEATURE_DATA_PS_DEBUG */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (DPL_IID_IFNAME_MIN > ifname || DPL_IID_IFNAME_MAX <= ifname)
  {
    LOG_MSG_FATAL_ERROR("Invalid ifname (%d) passed", ifname, 0, 0);
    ASSERT(0);
    return -1;
  }

  if (!PS_IFACE_IS_VALID(ps_iface_ptr))
  {
    LOG_MSG_FATAL_ERROR("Invalid iface, 0x%p, is passed", ps_iface_ptr, 0, 0);
    ASSERT(0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    Logical ifaces do not have any phys links
  -------------------------------------------------------------------------*/
  if (PS_IFACE_IS_LOGICAL(ps_iface_ptr))
  {
    LOG_MSG_ERROR("Link logging is not supported on logical iface, 0x%x:%d",
              ps_iface_ptr->name, ps_iface_ptr->instance, 0);
    return -1;
  }

  phys_link_ptr = PS_IFACEI_GET_PHYS_LINK(ps_iface_ptr);
  ASSERT(PS_PHYS_LINK_IS_VALID(phys_link_ptr));

  /*-------------------------------------------------------------------------
    Do nothing if ifname is already set.
  -------------------------------------------------------------------------*/
  if (DPL_IID_IFNAME_MIN <= phys_link_ptr->dpl_link_cb.tx_dpl_id.ifname)
  {
    LOG_MSG_INFO2("Trying to enable link logging on an iface (0x%x:%d) which "
            "already supports it",
            ps_iface_ptr->name, ps_iface_ptr->instance, 0);
    return 0;
  }

#ifdef FEATURE_DATA_PS_DEBUG
  /*-------------------------------------------------------------------------
    Ensure that there is no phys link with the same ifname as the one passed
    as parameter. Since ifaces are never destroyed, we know we've reached the
    last interface in global_iface_ptr_array when the value is NULL.
  -------------------------------------------------------------------------*/
  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  if_ptr = global_iface_ptr_array;
  for (i = 0; (i < MAX_SYSTEM_IFACES) && (NULL != *if_ptr); i++, if_ptr++)
  {
    phys_link_ptr = PS_IFACEI_GET_PHYS_LINK(*if_ptr);
    ASSERT(PS_PHYS_LINK_IS_VALID(phys_link_ptr));

    if (ifname == phys_link_ptr->dpl_link_cb.tx_dpl_id.ifname)
    {
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      LOG_MSG_FATAL_ERROR("Duplicate ifname (%d) passed to iface_ptr 0x%p. The other "
          "iface_ptr is 0x%p", ifname, ps_iface_ptr, *if_ptr);
      ASSERT(0);
      return -1;
    }
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
#endif /* FEATURE_DATA_PS_DEBUG */

  for (i = 0; i < PS_IFACE_GET_NUM_PHYS_LINKS(ps_iface_ptr); i++)
  {
    phys_link_ptr = PS_IFACEI_GET_PHYS_LINK_BY_INST(ps_iface_ptr, i);
    ASSERT(PS_PHYS_LINK_IS_VALID(phys_link_ptr));

    phys_link_ptr->dpl_link_cb.tx_dpl_id.ifname          = ifname;
    SET_DPL_IID_L_BIT_LINK(phys_link_ptr->dpl_link_cb.tx_dpl_id);
    SET_DPL_IID_DIR_TX(phys_link_ptr->dpl_link_cb.tx_dpl_id);
    phys_link_ptr->dpl_link_cb.tx_dpl_id.link_instance   =
      phys_link_ptr->phys_private.instance;

    phys_link_ptr->dpl_link_cb.recv_dpl_id.ifname          = ifname;
    SET_DPL_IID_L_BIT_LINK(phys_link_ptr->dpl_link_cb.recv_dpl_id);
    SET_DPL_IID_DIR_RX(phys_link_ptr->dpl_link_cb.recv_dpl_id);
    phys_link_ptr->dpl_link_cb.recv_dpl_id.link_instance   =
      phys_link_ptr->phys_private.instance;

    (void) ps_iface_dpl_set_phys_link_desc(phys_link_ptr, NULL);

    /*-----------------------------------------------------------------------
      Set default log settings for CDMA PPP Um ifaces
    -----------------------------------------------------------------------*/
#define DPL_CDMA_PPP_DEFAULT_SNAPLEN (45)
    switch (ifname)
    {
      case DPL_IID_IFNAME_CDMA_SN_IFACE_V4_PKT:
      case DPL_IID_IFNAME_CDMA_SN_IFACE_V6_PKT:
      case DPL_IID_IFNAME_CDMA_SN_IFACE_ANY_PKT:
      case DPL_IID_IFNAME_CDMA_SN_IFACE_ASYNC:
      case DPL_IID_IFNAME_CDMA_AN_IFACE:
        LOG_MSG_INFO2("Setting default dpl ppp logging (%d bytes) for iface 0x%p",
                DPL_CDMA_PPP_DEFAULT_SNAPLEN, ps_iface_ptr, 0);

        phys_link_ptr->dpl_link_cb.tx_cb.snaplen =
          DPL_CDMA_PPP_DEFAULT_SNAPLEN;
        phys_link_ptr->dpl_link_cb.tx_cb.is_logged = TRUE;

        phys_link_ptr->dpl_link_cb.recv_cb.snaplen =
          DPL_CDMA_PPP_DEFAULT_SNAPLEN;
        phys_link_ptr->dpl_link_cb.recv_cb.is_logged = TRUE;
        break;

      default:
        break;
    }
  }

  dpli_set_ps_iface_ptr(ps_iface_ptr, ifname);
  return 0;

} /* ps_iface_dpl_support_link_logging() */

#endif /* FEATURE_DATA_PS_DATA_LOGGING */
#endif /* FEATURE_DATA_PS */
