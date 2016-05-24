#ifndef PS_IFACE_UTILS_H
#define PS_IFACE_UTILS_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        P S _ I F A C E _ U T I L S . H


GENERAL DESCRIPTION
  This file contains iface macros and other utilities used only by internal
  ps_iface functions.


Copyright (c) 2004-2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                            EDIT HISTORY FOR FILE

  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_iface_utils.h#2 $
  $Author: maldinge $ $DateTime: 2011/06/27 13:15:21 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
06/27/11    bvd    Converting PS_IFACEI_COMMON_IS_FLOW_ENABLED to PS_IFACE_COMMON_IS_FLOW_ENABLED
03/26/09    pp     CMI: Removed unused headers.
12/28/08    pp     Common Modem Interface: Public/Private API split
                   (split from ps_ifacei_utils.h).
09/12/08    pp     Metainfo optimizations.
04/25/06    msr    L4/Tasklock code review changes
02/22/06    msr    Using single critical section
02/06/06    msr    Updated for L4 tasklock/crit sections.
12/05/05    msr    Changed PS_IFACEI_NUM_FILTERS()
08/15/05    mct    Fixed naming causing some issues w/C++ compilers.
05/02/05    msr    Wrapped macros in parenthesis.
04/17/05    msr    Changed PS_IFACEI_IS_LOGICAL() and added
                   PS_IFACEI_GET_STATE().
04/16/05    ks     Checking for NULL state in PS_IFACEI_PHYS_LINK_IS_GONE()
03/01/05    sv     Fixed IFACEI_FLOW macros to return only the iface flow mask.
11/19/04    msr    Added more macros.
10/25/04    msr    Changed PS_IFACEI_[GS]ET_GRACEFUL_CLOSE macros to
                   manipulate "graceful_dormant_close" variable stored in
                   ps_iface. Fixed PS_IFACEI_GET_PHYS_LINK macro to inlude
                   the value of private.phys_link.primary
08/02/04    mct    created file.
===========================================================================*/

#include "queue.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                      PUBLIC MACRO/FUNCTION DEFINITIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
/*===========================================================================
MACRO PS_IFACEI_GET_STATE()

DESCRIPTION
  This macro returns iface's state

PARAMETERS
  iface_ptr : pointer to the interface in question.

RETURN VALUE
  iface state
===========================================================================*/
#define PS_IFACEI_GET_STATE(iface_ptr)  ((iface_ptr)->iface_private.state)

/*===========================================================================
MACRO PS_IFACEI_IS_LOGICAL()

DESCRIPTION
  This macro returns a boolean indicating whether the iface pointer passed in
  is a logical iface.

PARAMETERS
  iface_ptr: pointer to the interface in question.

RETURN VALUE
  TRUE: if iface_ptr is a logical one
  FALSE: otherwise.
===========================================================================*/
#define PS_IFACEI_IS_LOGICAL( iface_ptr )                                \
  ((iface_ptr)->iface_private.is_logical == TRUE)

/*===========================================================================
MACRO PS_IFACEI_GET_ASSOC_IFACE()

DESCRIPTION
  This macro returns a ps_iface_ptr pointing to the associated iface of the
  logical iface that is specified.

PARAMETERS
  iface_ptr: pointer to the interface in question.

RETURN VALUE
  Associated ps_iface_ptr if passed in iface_ptr is a logical iface.
  Null: otherwise.
===========================================================================*/
#define PS_IFACEI_GET_ASSOC_IFACE( iface_ptr )                           \
  (PS_IFACEI_IS_LOGICAL(iface_ptr)                                       \
     ? ( NULL != (iface_ptr)->iface_private.assoc_iface_ptr              \
          ? (iface_ptr)->iface_private.assoc_iface_ptr                   \
          : (iface_ptr)->iface_private.trat_iface_ptr )                  \
     : NULL)

/*===========================================================================
MACRO PS_IFACEI_GET_PHYS_LINK()

DESCRIPTION
  This macro will return the primary phys link associated with the specified
  iface.

PARAMETERS
  iface_ptr: pointer to the interface.

RETURN VALUE
  pointer to primary phys link.
===========================================================================*/
#define PS_IFACEI_GET_PHYS_LINK( iface_ptr )                             \
  (PS_IFACEI_IS_LOGICAL(iface_ptr)                                       \
     ? PS_IFACE_GET_PHYS_LINK(PS_IFACEI_GET_ASSOC_IFACE(iface_ptr))      \
     : ((iface_ptr)->iface_private.phys_link.array != NULL               \
        ? ((iface_ptr)->iface_private.phys_link.array +                  \
           (iface_ptr)->iface_private.phys_link.primary )                \
        : (PS_FLOW_GET_PHYS_LINK(PS_IFACEI_GET_DEFAULT_FLOW(iface_ptr)))))

/*===========================================================================
MACRO PS_IFACEI_GET_PHYS_LINK_BY_INST()

DESCRIPTION
  This macro will return the phys link associated with the specified
  iface for the particular instance passed.

  NOTE : This macro DOESN'T work for logical ifaces

PARAMETERS
  iface_ptr : pointer to the interface.
  instance  : phys_link instance

RETURN VALUE
  pointer to phys link.
===========================================================================*/
#define PS_IFACEI_GET_PHYS_LINK_BY_INST(iface_ptr, instance)             \
  ((iface_ptr)->iface_private.phys_link.array + instance)

/*===========================================================================
MACRO PS_IFACEI_IS_PHYS_LINK_VALID()

DESCRIPTION
  Validates whether the specified phys link belongs to the specified iface.

  NOTE : This macro DOESN'T work for logical ifaces

PARAMETERS
  iface_ptr     :  Pointer to interface
  phys_link_ptr :  Pointer to the phys link to validate

RETURN VALUE
  TRUE  : If phys link belongs to ps_iface
  FALSE : otherwise
===========================================================================*/
#define PS_IFACEI_IS_PHYS_LINK_VALID(iface_ptr, phys_link_ptr)           \
  (PS_IFACEI_GET_PHYS_LINK_BY_INST                                       \
   (                                                                     \
     iface_ptr,                                                          \
     PS_PHYS_LINKI_GET_INST(phys_link_ptr)                               \
   ) == (phys_link_ptr))

/*===========================================================================
MACRO PS_IFACEI_GET_DEFAULT_FLOW()

DESCRIPTION
  This macro will return the default flow associated with the specified
  iface.

PARAMETERS
  iface_ptr : pointer to the interface.

RETURN VALUE
  pointer to default flow
===========================================================================*/
#define PS_IFACEI_GET_DEFAULT_FLOW(iface_ptr)                            \
  ((iface_ptr)->iface_private.flow.default_flow_ptr)

/*===========================================================================
MACRO PS_IFACEI_FLOW_ENABLED()

DESCRIPTION
  This macro returns whether the interface is has flow disabled.

PARAMETERS
  iface_ptr: pointer to the interface whose state is requested.

RETURN VALUE
  TRUE: if flow is disabled
  FALSE: otherwise

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
#define PS_IFACEI_FLOW_ENABLED( iface_ptr )                             \
        (((iface_ptr)->iface_private.tx_flow_mask) == \
         ALL_FLOWS_ENABLED)

/*===========================================================================
MACRO PS_IFACEI_NUM_FILTERS()

DESCRIPTION
  This macro returns the current number of IP filters in the iface for the
  specified client.

PARAMETERS
  iface_ptr: pointer to the interface on which operation is requested.
  client: filtering client id for which the filter count is requested

RETURN VALUE
  As described above.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
#define PS_IFACEI_NUM_FILTERS( iface_ptr, client )                      \
  (q_cnt(&((iface_ptr)->iface_private.ipfltr_info[client])))

/*===========================================================================
MACRO PS_IFACEI_GET_NUM_SEC_FLOWS()

DESCRIPTION
  This macro returns the number of secondary flows associated with this
  iface.

PARAMETERS
  iface_ptr : Target iface ptr

RETURN VALUE
  Number of secondary flows
===========================================================================*/
#define PS_IFACEI_GET_NUM_SEC_FLOWS(iface_ptr)                           \
  ((iface_ptr)->iface_private.flow.num_sec_flows)

/*===========================================================================
MACRO PS_IFACEI_IP_INFO_IS_INHERITED

DESCRIPTION
  This macro returns TRUE if the inherit flag is TRUE *and* if there is
  an associated IFACE.

PARAMETERS
  iface_ptr: Target iface ptr

RETURN VALUE
  TRUE  - ip info inherited.
  FALSE - ip info not inherited.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
#define PS_IFACEI_IP_INFO_IS_INHERITED(iface_ptr)                        \
  ((iface_ptr)->iface_private.inherit_ip_info == TRUE)

/*===========================================================================
MACRO PS_IFACEI_COMMON_IS_FLOW_ENABLED()

DESCRIPTION
  This macro returns whether all of iface, flow, and phys_link in question
  are flow enabled.

PARAMETERS
  iface_ptr     : pointer to the interface
  flow_ptr      : ptr to a flow

RETURN VALUE
  TRUE  : if flow is enabled on all the entities
  FALSE : otherwise
===========================================================================*/
#define PS_IFACEI_COMMON_IS_FLOW_ENABLED(iface_ptr, flow_ptr)            \
  ((PS_FLOWI_GET_PHYS_LINK(flow_ptr) != NULL)                            \
      ? (PS_IFACEI_FLOW_ENABLED(iface_ptr) &&                            \
         PS_FLOWI_IS_TX_ENABLED(flow_ptr) &&                             \
         PS_PHYS_LINK_FLOW_ENABLED(PS_FLOWI_GET_PHYS_LINK(flow_ptr)))   \
      : (PS_IFACEI_FLOW_ENABLED(iface_ptr) &&                            \
         PS_FLOWI_IS_TX_ENABLED(flow_ptr)))

#endif /* PS_IFACE_UTILS_H */
