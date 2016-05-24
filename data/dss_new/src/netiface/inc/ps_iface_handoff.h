#ifndef PS_IFACE_HANDOFF_H
#define PS_IFACE_HANDOFF_H
/*===========================================================================
  @file ps_iface_handoff.h

  TODO

  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datamodem/interface/netiface/rel/11.03/inc/ps_iface_handoff.h#1 $
  $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

===========================================================================*/

/*===========================================================================

                                INCLUDE FILES

===========================================================================*/
#include "ps_iface_defs.h"


/*===========================================================================

                              EXTERNAL FUNCTIONS

===========================================================================*/
int32 ps_iface_handoff_initiate
(
  ps_iface_type         * ps_iface_ptr,
  acl_policy_info_type  * acl_policy_ptr,
  int16                 * ps_errno_ptr
);

int32 ps_iface_handoff_swap_rat
(
  ps_iface_type  * ps_iface_ptr,
  int16          * ps_errno_ptr
);

int32 ps_iface_handoff_failure
(
  ps_iface_type  * ps_iface_ptr,
  int16          * ps_errno_ptr
);

#ifdef FEATURE_DATA_PS_IPV6
int32 ps_iface_handoff_transfer_sec_ipv6_addr
(
  ps_iface_type  * ps_iface_ptr,
  int16          * ps_errno_ptr
);
#endif /* FEATURE_DATA_PS_IPV6 */


#endif /* PS_IFACE_HANDOFF_H */
