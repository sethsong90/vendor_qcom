#ifndef IWLAN_3GPP_RT_PROF_H
#define IWLAN_3GPP_RT_PROF_H
/*===========================================================================

                IW L A N _ 3 G P P _ R T _ P R O F . H

DESCRIPTION
   This is IWLAN 3GPP iface Routing Profile API header file.


Copyright (c) 2009 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
                      EDIT HISTORY FOR FILE


  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.


  $Header: //source/qcom/qct/modem/datamodem/interface/api/rel/11.03/iwlan_3gpp_rt_prof.h#1 $ $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

===========================================================================*/

/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/

#include "comdef.h"
#include "customer.h"
#include "ps_acl.h"

boolean
iwlan_3gpp_rt_is_profile_in_use
( 
  uint32                   pdp_profile_num,     /* PDP profile number*/
  acl_policy_info_type   * policy_info_ptr      /* Policy Information */
);

#endif /* IWLAN_3GPP_RT_ACL_H */
